#include "chunker.h"
#include "inout.h"
#include "int_fastdiv.h"
#include "my_profiler.h"
#include "navigator.h"
#include "noise_blender.h"

#include <godot_cpp/classes/geometry2d.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/classes/time.hpp>
#include <godot_cpp/classes/world3d.hpp>

using namespace godot;

ChunkUpdateParameters::ChunkUpdateParameters()
{
    coord0 = Vector2i();
    coord1 = Vector2i();
    res0 = 0.0;
    res1 = 0.0;
    delta = Vector2i();
    saved_player_coord = Vector2i();
    is_swap = false;
}

GDChunker::GDChunker()
{
}

GDChunker::~GDChunker()
{
}

void GDChunker::init(float chunk_width, float chunk_resolution, float sea_level, float grass_size, GDNoiseBlender* blender, TypedArray<int> lods, bool find_bound_coords)
{
    this->update_chunk_array = PackedFloat32Array();

    this->chunk_width = chunk_width;
    this->chunk_resolution = chunk_resolution;
    this->sea_level = sea_level;
    this->grass_size = grass_size;
    this->lod_levels = lods;
    this->blender = blender;
    this->find_bound_coords = find_bound_coords;
    this->track_biomes_upto_lod = 2;

    min_height_position = Vector3(NAN, NAN, NAN);
    max_height_position = Vector3(NAN, NAN, NAN);
    chunk_vertices = std::vector<PackedVector3Array>();
    chunk_vertices.reserve(lods.size());
    chunk_mesh_data = std::vector<Array>();
    chunk_mesh_data.reserve(lods.size());
    for (int i = 0; i < lods.size(); i++) {
        chunk_vertices.push_back(PackedVector3Array());
        chunk_mesh_data.push_back(Array());
    }

    player_coord = Vector2i();
    chunk_update_queue = std::queue<ChunkUpdateParameters>();

    this->chunk_lods = Dictionary();
    this->chunk_rids = Dictionary();
    this->mesh_rids = Dictionary();
    this->mats = Dictionary();
    this->water_chunk_rids = Dictionary();
    this->water_mesh_rids = Dictionary();
    this->water_mats = Dictionary();
    this->chunk_positions = Dictionary();
    this->bodies = Dictionary();
    this->height_maps = Dictionary();
    this->biome_maps = Dictionary();
    this->color_maps = Dictionary();

    grass_size = 1.0;
    grass_mesh_instance = nullptr;
    grass_mesh = nullptr;
    grass_coords = PackedVector3Array();
}

void GDChunker::set_biome_shader(Shader* biome_shader)
{
    this->biome_shader = biome_shader;
}

void GDChunker::set_water_shader(Shader* water_shader)
{
    this->water_shader = water_shader;
}

void GDChunker::set_water_noise(NoiseTexture2D* water_noise)
{
    this->water_noise = water_noise;
}

void GDChunker::set_water_ripples_noise(NoiseTexture2D* water_ripples_noise)
{
    this->water_ripples_noise = water_ripples_noise;
}

void GDChunker::set_noise_texture(NoiseTexture2D* noise_texture)
{
    this->noise_texture = noise_texture;
}

void GDChunker::set_grass_mesh(Mesh* grass_mesh_instance)
{
    this->grass_mesh_instance = grass_mesh_instance;
}

void GDChunker::init_chunks(float x, float z)
{
    player_coord = convert_position_to_coord(x, z);
    int64_t level = 0;
    auto res = chunk_resolution;
    for (size_t vidx = 0; vidx < lod_levels.size(); vidx++) {
        auto value = (int64_t)lod_levels[vidx];
        for (int64_t row = -value + player_coord.y; row < value + player_coord.y; row++) {
            for (int64_t col = -value + player_coord.x; col < value + player_coord.x; col++) {
                auto coord = Vector2i(col, row);
                if (chunk_lods.has(coord)) {
                    continue;
                }
                chunk_lods[coord] = level;
                auto chunk = create_mesh(chunk_width, res);
                auto position = convert_coord_to_position(col, row);
                chunk_rids[coord] = chunk[0];
                mesh_rids[coord] = chunk[1];
                auto shader_mat = new ShaderMaterial();
                shader_mat->set_shader(biome_shader);
                mats[coord] = shader_mat;

                auto water_chunk = create_mesh(chunk_width, res * 0.5);
                water_chunk_rids[coord] = water_chunk[0];
                water_mesh_rids[coord] = water_chunk[1];
                auto water_shader_mat = new ShaderMaterial();
                water_shader_mat->set_shader(water_shader);
                water_mats[coord] = water_shader_mat;
                water_shader_mat->set_shader_parameter("noise", water_noise);
                water_shader_mat->set_shader_parameter("water_ripples_noise", water_ripples_noise);
                RenderingServer::get_singleton()->mesh_surface_set_material(water_chunk[1], 0, water_shader_mat->get_rid());

                chunk_positions[coord] = position;
                auto map = create_height_map_shape(chunk_width, level < track_biomes_upto_lod ? chunk_resolution : res);
                height_maps[coord] = map;
                if (level == 0) {
                    auto body = create_static_body(chunk_width, chunk_resolution, map);
                    bodies[coord] = body;
                }
                update_chunk(coord, coord, res, player_coord);
                update_water_chunk(coord, coord);
            }
        }
        res *= 0.5;
        level += 1;
    }

    if (HAS_GRASS && grass_size > 0.0) {
        auto gm = new MultiMesh();
        gm->set_transform_format(MultiMesh::TRANSFORM_3D);
        gm->set_use_custom_data(true);
        gm->set_instance_count(count_grass_instances());
        gm->set_visible_instance_count(0);
        gm->set_mesh(grass_mesh_instance);
        this->grass_mesh = new MultiMeshInstance3D();
        this->grass_mesh->set_multimesh(gm);
        this->grass_mesh->set_cast_shadows_setting(GeometryInstance3D::SHADOW_CASTING_SETTING_OFF);
    }
}

void GDChunker::deinit()
{
    for (size_t i = 0; i < chunk_rids.values().size(); i++)
        RenderingServer::get_singleton()->free_rid(chunk_rids.values()[i]);
    for (size_t i = 0; i < mesh_rids.values().size(); i++)
        RenderingServer::get_singleton()->free_rid(mesh_rids.values()[i]);
    for (size_t i = 0; i < water_chunk_rids.values().size(); i++)
        RenderingServer::get_singleton()->free_rid(water_chunk_rids.values()[i]);
    for (size_t i = 0; i < water_mesh_rids.values().size(); i++)
        RenderingServer::get_singleton()->free_rid(water_mesh_rids.values()[i]);
}

void GDChunker::set_world(Node3D* world)
{
    for (size_t cidx = 0; cidx < chunk_rids.keys().size(); cidx++) {
        auto coord = (Vector2i)chunk_rids.keys()[cidx];
        auto rid = (RID)chunk_rids[coord];
        RenderingServer::get_singleton()->instance_set_scenario(rid, world->get_world_3d()->get_scenario());
        auto wid = (RID)water_chunk_rids[coord];
        RenderingServer::get_singleton()->instance_set_scenario(wid, world->get_world_3d()->get_scenario());
        if (bodies.has(coord)) {
            auto body = (StaticBody3D*)(Object*)bodies[coord];
            world->add_child(body);
        }
    }
    if (grass_mesh != nullptr) {
        world->add_child(this->grass_mesh);
    }
}

TypedArray<RID> GDChunker::create_mesh(float size, float res)
{
    auto plane = new PlaneMesh();
    plane->set_size(Vector2(size, size));
    auto subdivide = subdivisions(res);
    plane->set_subdivide_depth(subdivide);
    plane->set_subdivide_width(subdivide);
    auto mesh = RenderingServer::get_singleton()->mesh_create();
    RenderingServer::get_singleton()->mesh_add_surface_from_arrays(mesh, RenderingServer::PRIMITIVE_TRIANGLES, plane->get_mesh_arrays());
    auto rid = RenderingServer::get_singleton()->instance_create();
    RenderingServer::get_singleton()->instance_set_base(rid, mesh);
    auto result = TypedArray<RID>();
    result.append(rid);
    result.append(mesh);
    return result;
}

HeightMapShape3D* GDChunker::create_height_map_shape(float size, float res)
{
    auto subdivide = subdivisions(res);
    auto map = new HeightMapShape3D();
    map->set_map_width(subdivide + 2);
    map->set_map_depth(subdivide + 2);
    return map;
}

StaticBody3D* GDChunker::create_static_body(float size, float res, HeightMapShape3D* map)
{
    auto subdivide = subdivisions(res);
    auto body = new StaticBody3D();
    body->set_name("static");
    auto collision = new CollisionShape3D();
    collision->set_shape(map);
    collision->set_name("collision");
    collision->set_scale(Vector3(size / (subdivide + 1.0), size / (subdivide + 1.0), size / (subdivide + 1.0)));
    collision->rotate_y(Math_PI);
    body->add_child(collision);
    return body;
}

void GDChunker::update_chunk(Vector2i coord, Vector2i new_coord, float res, Vector2i saved_player_coord)
{
    auto mesh = (RID)mesh_rids[coord];
    PackedVector3Array vertices;
    Array mesh_data;
    auto lod_level = (int64_t)chunk_lods[coord];
    if (chunk_vertices[lod_level].is_empty()) {
        auto plane = new PlaneMesh();
        plane->set_size(Vector2(chunk_width, chunk_width));
        auto subdivide = subdivisions(res);
        plane->set_subdivide_depth(subdivide);
        plane->set_subdivide_width(subdivide);

        chunk_mesh_data[lod_level] = plane->get_mesh_arrays();
        chunk_vertices[lod_level] = chunk_mesh_data[lod_level][Mesh::ArrayType::ARRAY_VERTEX];
    }
    vertices = chunk_vertices[lod_level];
    mesh_data = chunk_mesh_data[lod_level];
    // chunk_width / (subdivisions(resolution(0)) + 1)
    auto subdivide = subdivisions(res);
    auto R = chunk_width / (subdivide + 1);
    auto W = subdivide + 2;
    auto x = new_coord.x * chunk_width;
    auto z = new_coord.y * chunk_width;
    auto X = x / R - W / 2.0;
    auto Z = z / R - W / 2.0;
    X = UtilityFunctions::snappedf(X, 1.0);
    Z = UtilityFunctions::snappedf(Z, 1.0);

    auto biome_x_texture = blender->biome_texture(X, Z, W, W, R, 0);
    auto biome_z_texture = blender->biome_texture(X, Z, W, W, R, 1);
    auto A = Vector3();
    auto lod = (int64_t)chunk_lods[coord];
    auto ys = blender->height_map(X, Z, W, W, R, lod == 0);
    auto yss = PackedFloat32Array();

    if (lod == 0) {
        biome_maps[coord] = blender->get_biomes_map();
        color_maps[coord] = blender->get_colors_map();
    } else if (lod < track_biomes_upto_lod) {
        auto newS = subdivisions(resolution(0));
        auto newR = chunk_width / (newS + 1);
        auto newW = newS + 2;
        auto newX = x / newR - newW / 2.0;
        auto newZ = z / newR - newW / 2.0;
        newX = UtilityFunctions::snappedf(newX, 1.0);
        newZ = UtilityFunctions::snappedf(newZ, 1.0);
        yss = blender->height_map(newX, newZ, newW, newW, newR, true);
        biome_maps[coord] = blender->get_biomes_map();
        color_maps[coord] = blender->get_colors_map();
    } else {
        biome_maps[coord] = PackedInt32Array();
        color_maps[coord] = PackedColorArray();
    }

    auto w = (int_fastdiv)UtilityFunctions::floori(W);
    auto S = 0.0001;
    auto pos = convert_coord_to_position(new_coord.x, new_coord.y);
    auto hmap = (HeightMapShape3D*)(Object*)height_maps[coord];
    auto hmap_scale = height_map_scale(lod);
    update_chunk_array.resize(hmap->get_map_data().size());
    auto manhattan = UtilityFunctions::maxf(UtilityFunctions::absf(coord.x - saved_player_coord.x), UtilityFunctions::absf(coord.y - saved_player_coord.y));
    auto is_central = manhattan < 1;
    auto is_central_layer2 = manhattan < 2;
    auto edges = edges_part_of_transition(coord, new_coord, saved_player_coord);
    auto is_internal_chunk = lod < lod_levels.size() - 1;

    for (int i = 0; i < vertices.size(); i++) {
        A = vertices[i];
        auto row = i / w;
        auto col = i % w;

        if ((row == 0 && edges.y == 1) || (row == w - 1 && edges.y == -1)) {
            if (col % 2 == 1 && is_internal_chunk) {
                auto oj = w * (w - row - 1) + (w - (col - 1) - 1);
                auto nj = w * (w - row - 1) + (w - (col + 1) - 1);
                A.y = UtilityFunctions::snappedf((ys[oj] + ys[nj]) / 2.0, S);
            } else {
                auto j = w * (w - row - 1) + (w - col - 1);
                A.y = UtilityFunctions::snappedf(ys[j], S);
            }
        } else if ((col == 0 && edges.x == 1) || (col == w - 1 && edges.x == -1)) {
            if (row % 2 == 1 && is_internal_chunk) {
                auto oj = w * (w - (row - 1) - 1) + (w - col - 1);
                auto nj = w * (w - (row + 1) - 1) + (w - col - 1);
                A.y = UtilityFunctions::snappedf((ys[oj] + ys[nj]) / 2.0, S);
            } else {
                auto j = w * (w - row - 1) + (w - col - 1);
                A.y = UtilityFunctions::snappedf(ys[j], S);
            }
        } else {
            auto j = w * (w - row - 1) + (w - col - 1);
            A.y = UtilityFunctions::snappedf(ys[j], S);
        }

        vertices[i].y = A.y;
        if (lod == 0 || !(lod < track_biomes_upto_lod)) {
            update_chunk_array.set(i, A.y / hmap_scale);
        }
        if (find_bound_coords) {
            if (std::isnan(max_height_position.y) || (A.y > max_height_position.y && is_central)) {
                max_height_position = Vector3(A.x + x, A.y, A.z + z);
            }
            if (std::isnan(min_height_position.y) || (A.y < min_height_position.y && is_central_layer2)) {
                min_height_position = Vector3(A.x + x, A.y, A.z + z);
            }
        }
    }

    if (lod != 0 && lod < track_biomes_upto_lod) {
        hmap_scale = height_map_scale(0);
        auto newS = subdivisions(resolution(0));
        w = newS + 2;
        for (int i = 0; i < update_chunk_array.size(); i++) {
            auto row = i / w;
            auto col = i % w;
            auto j = w * (w - row - 1) + (w - col - 1);
            update_chunk_array.set(i, UtilityFunctions::snappedf(yss[j], S) / hmap_scale);
        }
    }
    hmap->set_map_data(update_chunk_array);

    RenderingServer::get_singleton()->mesh_clear(mesh);
    mesh_data[Mesh::ArrayType::ARRAY_VERTEX] = vertices;
    RenderingServer::get_singleton()->mesh_add_surface_from_arrays(mesh, RenderingServer::PRIMITIVE_TRIANGLES, mesh_data);

    auto mat = (ShaderMaterial*)(Object*)mats[coord];
    mat->set_shader_parameter("biome_x", biome_x_texture);
    mat->set_shader_parameter("biome_y", biome_z_texture);
    mat->set_shader_parameter("noise", noise_texture);
    mat->set_shader_parameter("locations", blender->get_locations());
    RenderingServer::get_singleton()->mesh_surface_set_material(mesh, 0, mat->get_rid());

    chunk_positions[coord] = pos;
    if (bodies.has(coord)) {
        auto body = (StaticBody3D*)(Object*)bodies[coord];
        body->set_position(Vector3(pos.x, 0, pos.y));
    }

    auto rid = (RID)chunk_rids[coord];
    RenderingServer::get_singleton()->instance_set_transform(rid, Transform3D().translated(Vector3(x, 0, z)));
}

void GDChunker::update_water_chunk(Vector2i coord, Vector2i new_coord)
{
    auto x = new_coord.x * chunk_width;
    auto z = new_coord.y * chunk_width;
    auto rid = (RID)water_chunk_rids[coord];
    RenderingServer::get_singleton()->instance_set_transform(rid, Transform3D().translated(Vector3(x, sea_level, z)));
}

bool GDChunker::has_chunks_to_update()
{
    return !(chunk_update_queue.empty());
}

Dictionary GDChunker::update_chunks_in_queue(int64_t start, int64_t limit)
{
    auto updated = TypedArray<Vector4i>();
    auto removed = TypedArray<Vector4i>();
    auto duration = Time::get_singleton()->get_ticks_usec() - start;

    while (duration < limit && !chunk_update_queue.empty()) {
        auto params = chunk_update_queue.front();
        chunk_update_queue.pop();
        auto coord0 = params.coord0;
        auto coord1 = params.coord1;
        auto res0 = params.res0;
        auto saved_player_coord = params.saved_player_coord;
        update_chunk(coord0, coord1, res0, saved_player_coord);
        update_water_chunk(coord0, coord1);

        if (params.is_swap) {
            auto res1 = params.res1;
            update_chunk(coord1, coord0, res1, saved_player_coord);
            update_water_chunk(coord1, coord0);
            updated.append(Vector4i(coord1.x, coord1.y, UtilityFunctions::roundi(chunk_resolution / res0 - 1), UtilityFunctions::roundi(chunk_resolution / res1 - 1)));
            removed.append(Vector4i(coord0.x, coord0.y, UtilityFunctions::roundi(chunk_resolution / res0 - 1), UtilityFunctions::roundi(chunk_resolution / res1 - 1)));
            updated.append(Vector4i(coord0.x, coord0.y, UtilityFunctions::roundi(chunk_resolution / res1 - 1), UtilityFunctions::roundi(chunk_resolution / res0 - 1)));
            removed.append(Vector4i(coord1.x, coord1.y, UtilityFunctions::roundi(chunk_resolution / res1 - 1), UtilityFunctions::roundi(chunk_resolution / res0 - 1)));
            auto delta = params.delta;
            update_chunk(coord0 + delta, coord0 + delta, resolution(chunk_lods[coord0 + delta]), saved_player_coord);
            update_chunk(coord1 - delta, coord1 - delta, resolution(chunk_lods[coord0 + delta]), saved_player_coord);
            swap_keys(chunk_lods, coord0, coord1);
            swap_keys(chunk_rids, coord0, coord1);
            swap_keys(mesh_rids, coord0, coord1);
            swap_keys(mats, coord0, coord1);
            swap_keys(chunk_positions, coord0, coord1);
            if (bodies.has(coord0) && bodies.has(coord1)) {
                swap_keys(bodies, coord0, coord1);
            } else if (bodies.has(coord0)) {
                move_key(bodies, coord0, coord1);
            } else if (bodies.has(coord1)) {
                move_key(bodies, coord1, coord0);
            }
            swap_keys(height_maps, coord0, coord1);
            swap_keys(biome_maps, coord0, coord1);
            swap_keys(color_maps, coord0, coord1);
            swap_keys(water_chunk_rids, coord0, coord1);
            swap_keys(water_mesh_rids, coord0, coord1);
            swap_keys(water_mats, coord0, coord1);

        } else {
            updated.append(Vector4i(coord1.x, coord1.y, UtilityFunctions::roundi(chunk_resolution / res0 - 1), -1));
            removed.append(Vector4i(coord0.x, coord0.y, UtilityFunctions::roundi(chunk_resolution / res0 - 1), -1));
            move_key(chunk_lods, coord0, coord1);
            move_key(chunk_rids, coord0, coord1);
            move_key(mesh_rids, coord0, coord1);
            move_key(mats, coord0, coord1);
            move_key(chunk_positions, coord0, coord1);
            if (bodies.has(coord0)) {
                move_key(bodies, coord0, coord1);
            }
            move_key(height_maps, coord0, coord1);
            move_key(biome_maps, coord0, coord1);
            move_key(color_maps, coord0, coord1);
            move_key(water_chunk_rids, coord0, coord1);
            move_key(water_mesh_rids, coord0, coord1);
            move_key(water_mats, coord0, coord1);
        }
        duration = Time::get_singleton()->get_ticks_usec() - start;
    }

    auto result = Dictionary();
    result["updated"] = updated;
    result["removed"] = removed;
    return result;
}

void GDChunker::swap_keys(Dictionary dict, Variant key0, Variant key1)
{
    auto temp = dict[key0];
    dict[key0] = dict[key1];
    dict[key1] = temp;
}

void GDChunker::move_key(Dictionary dict, Variant from, Variant to)
{
    dict[to] = dict[from];
    dict.erase(from);
}

TypedArray<Vector2i> GDChunker::update_chunks(float x, float z)
{
    auto old_coord = player_coord;
    auto current_coord = convert_position_to_coord(x, z);
    auto delta = current_coord - old_coord;
    auto result = TypedArray<Vector2i>();
    if (delta == Vector2i()) {
        return result;
    }

    if (delta.length() > 1) {
        delta.y = 0;
    }

    result.append_array(update_chunks_impl(delta));
    player_coord += delta;
    return result;
}

TypedArray<Vector2i> GDChunker::update_chunks_impl(Vector2i delta)
{
    auto result = TypedArray<Vector2i>();
    for (size_t lod = 0; lod < lod_levels.size(); lod++) {
        auto to_flip = edges_at_direction(lod, delta * -1);
        auto into = edges_at_direction(lod, delta);
        for (size_t i = 0; i < to_flip.size(); i++) {
            if (lod < track_biomes_upto_lod) {
                result.append(to_flip[i]);
            }
            if (lod + 1 < lod_levels.size()) {
                auto params = ChunkUpdateParameters();
                params.coord0 = to_flip[i];
                params.coord1 = (Vector2i)into[i] + delta;
                params.res0 = resolution(lod);
                params.res1 = resolution(lod + 1);
                params.delta = delta;
                params.saved_player_coord = player_coord + delta;
                params.is_swap = true;
                if (lod + 1 < track_biomes_upto_lod) {
                    result.append((Vector2i)into[i] + delta);
                }
                chunk_update_queue.push(params);
            } else {
                auto params = ChunkUpdateParameters();
                params.coord0 = to_flip[i];
                params.coord1 = (Vector2i)into[i] + delta;
                params.res0 = resolution(lod);
                params.saved_player_coord = player_coord + delta;
                params.is_swap = false;
                chunk_update_queue.push(params);
            }
        }
    }
    return result;
}

TypedArray<Vector2i> GDChunker::edges_at_direction(int64_t lod, Vector2i direction)
{
    auto value = (int64_t)lod_levels[lod];
    auto result = TypedArray<Vector2i>();
    if (direction.x == -1) {
        for (int64_t i = -value; i < value; i++) {
            result.append(Vector2i(-value + player_coord.x, i + player_coord.y));
        }
    }
    if (direction.x == 1) {
        for (int64_t i = -value; i < value; i++) {
            result.append(Vector2i(value - 1 + player_coord.x, i + player_coord.y));
        }
    }
    if (direction.y == -1) {
        for (int64_t i = -value; i < value; i++) {
            result.append(Vector2i(i + player_coord.x, -value + player_coord.y));
        }
    }
    if (direction.y == 1) {
        for (int64_t i = -value; i < value; i++) {
            result.append(Vector2i(i + player_coord.x, value - 1 + player_coord.y));
        }
    }
    return result;
}

Vector2i GDChunker::edges_part_of_transition(Vector2i old_coord, Vector2i new_coord, Vector2i saved_player_coord)
{
    auto lod = (int64_t)chunk_lods[old_coord];
    auto value = (int64_t)lod_levels[lod];
    auto result = Vector2i();
    if (new_coord.x == -value + saved_player_coord.x) {
        result.x = -1;
    } else if (new_coord.x == value - 1 + saved_player_coord.x) {
        result.x = 1;
    }
    if (new_coord.y == -value + saved_player_coord.y) {
        result.y = -1;
    } else if (new_coord.y == value - 1 + saved_player_coord.y) {
        result.y = 1;
    }
    return result;
}

Vector2i GDChunker::convert_position_to_coord(double x, double z)
{
    return Vector2i(
        UtilityFunctions::floori((x + chunk_width / 2.0) / chunk_width),
        UtilityFunctions::floori((z + chunk_width / 2.0) / chunk_width));
}

Vector2 GDChunker::convert_coord_to_position(int64_t x, int64_t z)
{
    return Vector2(x * chunk_width, z * chunk_width);
}

int64_t GDChunker::subdivisions(float res)
{
    return UtilityFunctions::floori(chunk_width * res) - 1;
}

float GDChunker::resolution(int64_t lod)
{
    return chunk_resolution / pow(2, lod);
}

float GDChunker::height_map_scale(int64_t lod)
{
    return chunk_width / (subdivisions(resolution(lod)) + 1.0);
}

Vector3 GDChunker::get_max_height_position()
{
    return max_height_position;
}

Vector3 GDChunker::get_min_height_position()
{
    return min_height_position;
}

Vector2i GDChunker::get_player_coord()
{
    return player_coord;
}

PackedVector3Array GDChunker::get_chunk_vertices()
{
    return chunk_vertices[0];
}

int64_t GDChunker::get_track_biomes_upto_lod()
{
    return track_biomes_upto_lod;
}

Dictionary GDChunker::get_chunk_lods()
{
    return chunk_lods;
}

float GDChunker::get_chunk_width()
{
    return chunk_width;
}

Vector4 GDChunker::height_at_position(Vector2i coord, double x, double z)
{
    if (!height_maps.has(coord)) {
        return Vector4(NAN, NAN, NAN, NAN);
    }

    auto hmap = (HeightMapShape3D*)(Object*)height_maps[coord];
    auto lod = (int64_t)chunk_lods[coord];
    auto scale = height_map_scale(lod < track_biomes_upto_lod ? 0 : lod);
    auto pos = (Vector2)chunk_positions[coord];

    auto w = (hmap->get_map_width() - 1) * scale;
    auto d = (hmap->get_map_depth() - 1) * scale;

    // top-left corner of map
    auto base_x = pos.x - w * 0.5;
    auto base_z = pos.y - d * 0.5;

    // top-left and bottom-right row and col coordinates
    // *** Subtract from the width/depth because of noise texture mapping
    auto c0 = UtilityFunctions::floorf((w - (x - base_x)) / scale);
    auto r0 = UtilityFunctions::floorf((d - (z - base_z)) / scale);
    auto c1 = UtilityFunctions::ceilf((w - (x - base_x)) / scale);
    auto r1 = UtilityFunctions::ceilf((d - (z - base_z)) / scale);

    // if we are on a vertex move up/down for a coord
    if (c0 == c1) {
        if (c1 < hmap->get_map_width() - 1) {
            c1 += 1;
        } else {
            c0 -= 1;
        }
    }
    if (r0 == r1) {
        if (r1 < hmap->get_map_depth() - 1) {
            r1 += 1;
        } else {
            r0 -= 1;
        }
    }

    // if outside bounds return NAN
    if (c0 < 0 || c0 >= hmap->get_map_width() || c1 < 0 || c1 >= hmap->get_map_width()) {
        UtilityFunctions::print("x: ", x, " :: ", c0, "|", c1, " >= ", hmap->get_map_width());
        return Vector4(NAN, NAN, NAN, NAN);
    }
    if (r0 < 0 || r0 >= hmap->get_map_depth() || r1 < 0 || r1 >= hmap->get_map_depth()) {
        UtilityFunctions::print("z: ", z, " :: ", r0, "|", r1, " >= ", hmap->get_map_depth());
        return Vector4(NAN, NAN, NAN, NAN);
    }

    // convert (x, z) world coords that are passed in to the function into local space
    // *** Subtract from the width/depth because of noise texture mapping
    auto tX = (w - (x - base_x)) - w * 0.5;
    auto tZ = (d - (z - base_z)) - d * 0.5;

    // top-left and bottom-right coords in local space
    auto x0 = c0 * scale - w * 0.5;
    auto x1 = c1 * scale - w * 0.5;
    auto z0 = r0 * scale - d * 0.5;
    auto z1 = r1 * scale - d * 0.5;

    // find the 3 points that form the triangle (x,z) pas through
    auto s = Vector2(tX, tZ);
    auto s1 = Vector2(x0, z1);
    auto s2 = Vector2(x1, z0);
    auto s8 = Vector2(x0, z0);
    auto s9 = Vector2(x1, z1);
    auto is_low_point = s8.distance_squared_to(s) < s9.distance_squared_to(s);
    auto s0 = is_low_point ? s8 : s9;

    // find the heights of the triangle
    auto yB = hmap->get_map_data()[c0 + r1 * hmap->get_map_width()];
    auto yC = hmap->get_map_data()[c1 + r0 * hmap->get_map_width()];
    auto yA = is_low_point
        ? (hmap->get_map_data()[c0 + r0 * hmap->get_map_width()])
        : (hmap->get_map_data()[c1 + r1 * hmap->get_map_width()]);

    // build the final 3D triangle with scaled y's
    auto p0 = Vector3(s0.x, yA * scale, s0.y);
    auto p1 = Vector3(s1.x, yB * scale, s1.y);
    auto p2 = Vector3(s2.x, yC * scale, s2.y);

    // normal for the plane of the triangle defined by the equation [dot(p-p0,N)] where p is some point
    auto N = (p1 - p0).cross(p2 - p0);

    // ray cast line defined as a parametric equation [q0 + t * (q1 - q0)]
    auto q0 = Vector3(s.x, 5000, s.y);
    auto q1 = Vector3(s.x, -5000, s.y);

    // found by inserting the ray cast line into the plane equation
    //      dot(q0 + t*(q1-q0) - p0, N) = 0
    // =>   dot(q0-p0,N) + t dot(q1-q0,N) = 0
    // =>   t = -dot(q0-p0,N)/dot(q1-q0,N)
    auto t = -(q0 - p0).dot(N) / (q1 - q0).dot(N);

    // plug t back into the parametric line equation
    auto y = q0.lerp(q1, t).y;

    N = N.normalized() * (is_low_point ? 1.0 : -1.0);
    // Negate xz because of the noise texture
    return Vector4(-N.x, N.y, -N.z, y);
}

Dictionary GDChunker::terrain_normal(double x, double z)
{
    auto coord = convert_position_to_coord(x, z);
    auto V = height_at_position(coord, x, z);
    auto result = Dictionary();
    if (V.is_finite()) {
        result["position"] = Vector3(x, V.w, z);
        result["normal"] = Vector3(V.x, V.y, V.z);
    }
    return result;
}

bool GDChunker::terrain_normal_inplace(double x, double z, Dictionary result)
{
    auto coord = convert_position_to_coord(x, z);
    auto V = height_at_position(coord, x, z);
    if (V.is_finite()) {
        result["position"] = Vector3(x, V.w, z);
        result["normal"] = Vector3(V.x, V.y, V.z);
        return true;
    }
    return false;
}

float GDChunker::get_noise_scale()
{
    return chunk_width / (subdivisions(resolution(0)) + 1);
}

bool GDChunker::contains_neighbour_point(const PackedVector2Array& collection, Vector2 point, float spacing)
{
    long long pidx = collection.size() - 1;
    while (pidx >= 0) {
        if (((Vector2)collection[pidx]).distance_squared_to(point) <= spacing * spacing) {
            return true;
        }
        pidx -= 1;
    }
    return false;
}

Dictionary GDChunker::group_spawn_points(Vector2i coord, float spacing)
{
    auto areas = TypedArray<PackedVector2Array>();
    auto biomes = TypedArray<int>();
    auto points = TypedArray<Vector2>();
    auto b = 0;
    auto offsetv = coord * chunk_width;
    auto biome_map = (PackedInt32Array)biome_maps[coord];
    auto height_map_scale_ = height_map_scale((int64_t)chunk_lods[coord]);
    auto point_offset = Vector2(height_map_scale_, height_map_scale_);
    bool const DEBUG = false;
    auto found_subsets = std::vector<int>();
    found_subsets.reserve(2);
    for (size_t vidx = 0; vidx < chunk_vertices[0].size(); vidx++) {
        for (float scale_offset = 1.0; scale_offset <= 1.0; scale_offset += 0.25) {
            auto vp = chunk_vertices[0][vidx];
            auto p = -Vector2(vp.x, vp.z) + point_offset * scale_offset + offsetv;
            points.append(p);
            auto biome = (int)biome_map[b];
            found_subsets.clear();
            auto areas_size = areas.size();
            for (size_t i = 0; i < areas_size; i++) {
                if ((int)biomes[i] == biome && GDChunker::contains_neighbour_point(areas[i], p, spacing)) {
                    found_subsets.push_back(i);
                }
            }
            if (found_subsets.empty()) {
                auto n = PackedVector2Array();
                n.append(p);
                areas.append(n);
                biomes.append(biome);
            } else if (found_subsets.size() == 1) {
                ((PackedVector2Array)areas[found_subsets[0]]).append(p);
            } else {
                std::sort(found_subsets.begin(), found_subsets.end(), std::greater<int>());
                auto new_pack = PackedVector2Array();
                for (size_t sidx = 0; sidx < found_subsets.size(); sidx++) {
                    auto subset = found_subsets[sidx];
                    new_pack.append_array(areas[subset]);
                    areas.remove_at(subset);
                    biomes.remove_at(subset);
                }
                areas.append(new_pack);
                biomes.append(biome);
            }
        }
        b++;
    }
    auto result = Dictionary();
    result["points"] = areas;
    result["biomes"] = biomes;
    return result;
}

int32_t GDChunker::get_biome_at_position(double x, double z)
{
    auto coord = convert_position_to_coord(x, z);
    auto lod = (int32_t)chunk_lods[coord];
    if (lod >= track_biomes_upto_lod) {
        auto lod_width = (int64_t)lod_levels[track_biomes_upto_lod - 1];
        if (abs(coord.x - player_coord.x) >= lod_width) {
            coord.x += UtilityFunctions::signi(player_coord.x - coord.x);
        }
        if (abs(coord.y - player_coord.y) >= lod_width) {
            coord.y += UtilityFunctions::signi(player_coord.y - coord.y);
        }
    }
    auto biome_map = (PackedInt32Array)biome_maps[coord];
    auto pos = (Vector2)chunk_positions[coord];

    auto subdivide = subdivisions(resolution(0));
    auto scale = chunk_width / (subdivide + 1);
    auto W = subdivide + 2;

    auto base_x = pos.x - chunk_width * 0.5;
    auto base_z = pos.y - chunk_width * 0.5;

    auto c0 = UtilityFunctions::clampf(UtilityFunctions::roundf((x - base_x) / scale), 0, W - 1);
    auto r0 = UtilityFunctions::clampf(UtilityFunctions::roundf((z - base_z) / scale), 0, W - 1);

    if (c0 + r0 * W > biome_map.size()) {
        return 1;
    }
    return biome_map[c0 + r0 * W];
}

Color GDChunker::get_color_at_position(double x, double z)
{
    auto coord = convert_position_to_coord(x, z);
    auto lod = (int32_t)chunk_lods[coord];
    if (lod >= track_biomes_upto_lod) {
        auto lod_width = (int64_t)lod_levels[track_biomes_upto_lod - 1];
        if (abs(coord.x - player_coord.x) >= lod_width) {
            coord.x += UtilityFunctions::signi(player_coord.x - coord.x);
        }
        if (abs(coord.y - player_coord.y) >= lod_width) {
            coord.y += UtilityFunctions::signi(player_coord.y - coord.y);
        }
    }
    auto color_map = (PackedColorArray)color_maps[coord];
    auto pos = (Vector2)chunk_positions[coord];

    auto subdivide = subdivisions(resolution(0));
    auto scale = chunk_width / (subdivide + 1);
    auto W = subdivide + 2;

    auto base_x = pos.x - chunk_width * 0.5;
    auto base_z = pos.y - chunk_width * 0.5;

    auto c0 = UtilityFunctions::clampf(UtilityFunctions::roundf((x - base_x) / scale), 0, W - 1);
    auto r0 = UtilityFunctions::clampf(UtilityFunctions::roundf((z - base_z) / scale), 0, W - 1);

    if (c0 + r0 * W >= color_map.size()) {
        return Color(0, 0, 0);
    }
    return color_map[c0 + r0 * W];
}

void GDChunker::update_environment(double x, double y)
{
    auto old_position = player_position;
    player_position = Vector2(x, y);
    auto delta = player_position - old_position;
    place_grass(Vector2(grass_size * UtilityFunctions::signf(delta.x) * 2, grass_size * UtilityFunctions::signf(delta.y) * 2));
}

void GDChunker::place_grass(Vector2 delta)
{
    if (!HAS_GRASS) {
        return;
    }
    auto ignore_delta = false;
    if (grass_coords.is_empty()) {
        init_grass();
        ignore_delta = true;
    }

    auto mm = grass_mesh->get_multimesh();
    auto t = Transform3D(Basis(), Vector3());
    t = t.scaled_local(Vector3(1, 1, 1) * 200);
    auto hit_terrain = false;
    auto nt = t;
    auto horz = false;
    auto vert = false;
    auto pos = Vector3();
    auto p = Vector3();
    auto whn = Vector3();
    auto wh = 0.0;
    auto custom = Color(1, 1, 1, 1);
    auto R = get_noise_scale();
    auto normal_height = Dictionary();
    auto fade_time = (Time::get_singleton()->get_ticks_msec()) / 1000.0 + 0.5;
    normal_height["position"] = Vector3();
    normal_height["normal"] = Vector3();
    for (int i = 0; i < mm->get_visible_instance_count(); i++) {
        pos = grass_coords[i];
        horz = pos.x > player_position.x + grass_size || pos.x < player_position.x - grass_size;
        vert = pos.z > player_position.y + grass_size || pos.z < player_position.y - grass_size;

        if (horz || vert || ignore_delta) {
            p.x = pos.x + delta.x * (horz ? 1 : 0);
            p.z = pos.z + delta.y * (vert ? 1 : 0);
            hit_terrain = terrain_normal_inplace(p.x, p.z, normal_height);
            wh = ((Vector3)normal_height["position"]).y;
            whn = normal_height["normal"];
            nt = t;
            if (!hit_terrain || whn.distance_to(Vector3(0, 1, 0)) > 1 / sqrt(2.0)) {
                p.y = -10000;
            } else {
                p.y = wh;
                // blender->compute_biome_stats(p.x, p.z, R, chunk_width);
                auto h = blender->grass_height(blender->biome, -p.x, -p.y); // use p.y for more consistency
                if (h == 0) {
                    p.y = -10000;
                } else {
                    auto new_y = whn.normalized();
                    auto basis = nt.get_basis();
                    basis.set_column(1, new_y);
                    basis.set_column(0, -basis.get_column(2).cross(new_y));
                    nt.set_basis(basis.orthonormalized());
                    nt = nt.rotated_local(Vector3(0, 1, 0), UtilityFunctions::randf() * 2 * Math_PI);
                    nt = nt.scaled_local(Vector3(1, h, 1) * 200);
                    custom = get_color_at_position(p.x, p.z);
                    custom.a = p.z;
                    mm->set_instance_custom_data(i, custom);
                }
            }
            grass_coords[i] = p;
            mm->set_instance_transform(i, nt.translated(p));
        }
    }
}

void GDChunker::init_grass()
{
    if (!HAS_GRASS) {
        return;
    }
    auto mm = grass_mesh->get_multimesh();
    auto i = 0;
    UtilityFunctions::seed(0);
    const int R = 4;
    auto grass_store = std::vector<Vector3>();
    for (int _X = -grass_size; _X < grass_size + 1; _X += R * 2) {
        for (int _Y = -grass_size; _Y < grass_size + 1; _Y += R) {
            auto x = _X + ((_Y / R) % 2 == 0 ? 1 : 0) * R + player_position.x;
            auto y = _Y + player_position.y;
            for (int r = 0; r < R + 1; r += 2) {
                auto a = 0.0;
                while (a < Math_PI * 2) {
                    a += Math_PI / 4.0 * (1.0 / (floor(r / 4.0) + 1));
                    auto nx = cos(a) * r + x;
                    auto ny = sin(a) * r + y;
                    auto is_top_left = Geometry2D::get_singleton()->is_point_in_circle(Vector2(nx, ny), Vector2(x - R, y - R), R);
                    auto is_top_right = Geometry2D::get_singleton()->is_point_in_circle(Vector2(nx, ny), Vector2(x + R, y - R), R);
                    if (is_top_left || is_top_right) {
                        continue;
                    }
                    auto p = Vector3(nx, 1000, ny) + Vector3(UtilityFunctions::randf() - 0.5, 0, UtilityFunctions::randf() - 0.5);
                    grass_store.push_back(p);
                    i += 1;
                    if (r == 0) {
                        break;
                    }
                }
            }
        }
    }
    for (auto& v : grass_store) {
        grass_coords.append(v);
    }
    mm->set_visible_instance_count(i);
}

int GDChunker::count_grass_instances()
{
    const int R = 4;
    int i = 0;
    for (int _X = -grass_size; _X < grass_size + 1; _X += R * 2) {
        for (int _Y = -grass_size; _Y < grass_size + 1; _Y += R) {
            auto x = _X + ((_Y / R) % 2 == 0 ? 1 : 0) * R;
            auto y = _Y;
            for (int r = 0; r < R + 1; r += 2) {
                auto a = 0.0;
                while (a < Math_PI * 2) {
                    a += Math_PI / 4.0 * (1.0 / (floor(r / 4.0) + 1));
                    auto nx = cos(a) * r + x;
                    auto ny = sin(a) * r + y;
                    auto is_top_left = Geometry2D::get_singleton()->is_point_in_circle(Vector2(nx, ny), Vector2(x - R, y - R), R);
                    auto is_top_right = Geometry2D::get_singleton()->is_point_in_circle(Vector2(nx, ny), Vector2(x + R, y - R), R);
                    if (is_top_left || is_top_right) {
                        continue;
                    }
                    i += 1;
                    if (r == 0) {
                        break;
                    }
                }
            }
        }
    }
    return UtilityFunctions::ceili(i * 1.1);
}

void GDChunker::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("init", "chunk_width", "chunk_resolution", "sea_level", "grass_size", "blender", "lods", "find_bound_coords"), &GDChunker::init);
    ClassDB::bind_method(D_METHOD("deinit"), &GDChunker::deinit);
    ClassDB::bind_method(D_METHOD("set_biome_shader", "biome_shader"), &GDChunker::set_biome_shader);
    ClassDB::bind_method(D_METHOD("set_water_shader", "water_shader"), &GDChunker::set_water_shader);
    ClassDB::bind_method(D_METHOD("set_water_noise", "water_noise"), &GDChunker::set_water_noise);
    ClassDB::bind_method(D_METHOD("set_water_ripples_noise", "water_ripples_noise"), &GDChunker::set_water_ripples_noise);
    ClassDB::bind_method(D_METHOD("set_noise_texture", "noise_texture"), &GDChunker::set_noise_texture);
    ClassDB::bind_method(D_METHOD("set_grass_mesh", "mesh"), &GDChunker::set_grass_mesh);

    ClassDB::bind_method(D_METHOD("get_noise_scale"), &GDChunker::get_noise_scale);
    ClassDB::bind_method(D_METHOD("get_chunk_vertices"), &GDChunker::get_chunk_vertices);
    ClassDB::bind_method(D_METHOD("get_track_biomes_upto_lod"), &GDChunker::get_track_biomes_upto_lod);
    ClassDB::bind_method(D_METHOD("get_chunk_lods"), &GDChunker::get_chunk_lods);
    ClassDB::bind_method(D_METHOD("get_chunk_width"), &GDChunker::get_chunk_width);
    ClassDB::bind_method(D_METHOD("get_player_coord"), &GDChunker::get_player_coord);

    ClassDB::bind_method(D_METHOD("update_environment", "x", "z"), &GDChunker::update_environment);
    ClassDB::bind_method(D_METHOD("terrain_normal", "x", "z"), &GDChunker::terrain_normal);

    ClassDB::bind_method(D_METHOD("update_chunks", "x", "z"), &GDChunker::update_chunks);
    ClassDB::bind_method(D_METHOD("init_chunks", "x", "z"), &GDChunker::init_chunks);
    ClassDB::bind_method(D_METHOD("update_chunks_in_queue", "start", "limit"), &GDChunker::update_chunks_in_queue);
    ClassDB::bind_method(D_METHOD("has_chunks_to_update"), &GDChunker::has_chunks_to_update);
    ClassDB::bind_method(D_METHOD("set_world", "world"), &GDChunker::set_world);

    ClassDB::bind_method(D_METHOD("get_max_height_position"), &GDChunker::get_max_height_position);
    ClassDB::bind_method(D_METHOD("get_min_height_position"), &GDChunker::get_min_height_position);

    ClassDB::bind_method(D_METHOD("group_spawn_points", "coord", "spacing"), &GDChunker::group_spawn_points);
    ClassDB::bind_method(D_METHOD("get_biome_at_position", "x", "z"), &GDChunker::get_biome_at_position);
    ClassDB::bind_method(D_METHOD("get_color_at_position", "x", "z"), &GDChunker::get_color_at_position);
}