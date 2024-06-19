#include "terrain.h"
#include "inout.h"
#include "my_profiler.h"
#include "navigator.h"
#include "noise_blender.h"

#include <godot_cpp/classes/geometry2d.hpp>
#include <godot_cpp/classes/world3d.hpp>

using namespace godot;

void GDTerrain::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("init", "b", "cs", "r", "subdivide"), &GDTerrain::init);
    ClassDB::bind_method(D_METHOD("set_biome_shader", "biome_shader"), &GDTerrain::set_biome_shader);
    ClassDB::bind_method(D_METHOD("set_water_shader", "water_shader"), &GDTerrain::set_water_shader);
    ClassDB::bind_method(D_METHOD("set_water_noise", "water_noise"), &GDTerrain::set_water_noise);
    ClassDB::bind_method(D_METHOD("set_water_ripples_noise", "water_ripples_noise"), &GDTerrain::set_water_ripples_noise);
    ClassDB::bind_method(D_METHOD("set_sea_level", "level"), &GDTerrain::set_sea_level);

    ClassDB::bind_method(D_METHOD("init_chunks_of_size", "chunks", "locations", "x", "y", "cs", "r", "subdivide", "is_water"), &GDTerrain::init_chunks_of_size);
    ClassDB::bind_method(D_METHOD("init_chunks", "x", "y", "grass_mesh"), &GDTerrain::init_chunks);
    ClassDB::bind_method(D_METHOD("update_chunks_with_size", "chunks", "index", "x", "y", "cs", "r", "subdivide", "is_water"), &GDTerrain::update_chunks_with_size);
    ClassDB::bind_method(D_METHOD("update_chunks", "x", "y"), &GDTerrain::update_chunks);
    ClassDB::bind_method(D_METHOD("create_mesh", "x", "y", "size", "r", "subdivide"), &GDTerrain::create_mesh);
    ClassDB::bind_method(D_METHOD("create_water_mesh", "x", "y", "size"), &GDTerrain::create_water_mesh);
    ClassDB::bind_method(D_METHOD("create_chunk_with_size", "chunks", "locations", "x", "y", "cs", "r", "subdivide", "is_water"), &GDTerrain::create_chunk_with_size);
    ClassDB::bind_method(D_METHOD("update_mesh", "mi", "x", "y", "size", "r", "subdivide"), &GDTerrain::update_mesh);
    ClassDB::bind_method(D_METHOD("update_water_mesh", "mi", "x", "y", "size", "r", "subdivide"), &GDTerrain::update_water_mesh);
    ClassDB::bind_method(D_METHOD("update_chunk_with_size", "node", "x", "y", "size", "r", "subdivide", "is_water"), &GDTerrain::update_chunk_with_size);
    ClassDB::bind_method(D_METHOD("update_environment", "x", "y"), &GDTerrain::update_environment);
    ClassDB::bind_method(D_METHOD("update_chunk_environment", "node"), &GDTerrain::update_chunk_environment);
    ClassDB::bind_method(D_METHOD("place_grass", "delta"), &GDTerrain::place_grass);
    ClassDB::bind_method(D_METHOD("init_grass"), &GDTerrain::init_grass);
    ClassDB::bind_method(D_METHOD("hide_water", "y"), &GDTerrain::hide_water);
    ClassDB::bind_method(D_METHOD("set_player_coord_using_position", "x", "y", "cs"), &GDTerrain::set_player_coord_using_position);
    ClassDB::bind_method(D_METHOD("convert_position_to_coord", "x", "y", "cs"), &GDTerrain::convert_position_to_coord);

    ClassDB::bind_method(D_METHOD("get_max_height_position"), &GDTerrain::get_max_height_position);
    ClassDB::bind_method(D_METHOD("get_loaded_chunks_location"), &GDTerrain::get_loaded_chunks_location);
}

GDTerrain::GDTerrain()
{
    first_run = false;
}

GDTerrain::~GDTerrain()
{
}

void GDTerrain::init(GDNoiseBlender* b, double cs, double r, double subdivide)
{
    this->subdivide_percent = subdivide;
    this->blender = b;
    this->chunk_size = cs;
    this->grass_size = cs * 0.5;
    this->radius = r;
    chunk_vertices = PackedVector3Array();
}

void GDTerrain::set_biome_shader(Shader* biome_shader)
{
    this->biome_shader = biome_shader;
}

void GDTerrain::set_water_shader(Shader* water_shader)
{
    this->water_shader = water_shader;
}

void GDTerrain::set_water_noise(NoiseTexture2D* water_noise)
{
    this->water_noise = water_noise;
}

void GDTerrain::set_water_ripples_noise(NoiseTexture2D* water_ripples_noise)
{
    this->water_ripples_noise = water_ripples_noise;
}

void GDTerrain::set_sea_level(double level)
{
    this->sea_level = level;
}

TypedArray<Node3D> GDTerrain::init_chunks_of_size(TypedArray<Node3D> chunks, TypedArray<Vector2> locations, double x, double y, double cs, double r, double subdivide, bool is_water)
{
    this->set_player_coord_using_position(x, y, cs);
    auto rad = (int)(r / 2.0);
    auto result = TypedArray<Node3D>();
    for (int w = -rad; w < rad + 1; w++) {
        for (int h = -rad; h < rad + 1; h++) {
            auto p = Vector2((player_coord.x + w) * cs, (player_coord.y + h) * cs);
            auto node = create_chunk_with_size(chunks, locations, p.x, p.y, cs, r, subdivide, is_water);
            update_chunk_with_size(node, p.x, p.y, cs, r, subdivide, is_water);
            result.append(node);
        }
    }
    return result;
}

TypedArray<Node3D> GDTerrain::init_chunks(double x, double y, Mesh* grass_mesh)
{
    auto ref = TypedArray<Vector2>();
    auto result = init_chunks_of_size(loaded_chunks, ref, x, y, chunk_size, radius, subdivide_percent, false);
    loaded_chunks_location.append_array(ref);
    ref.clear();
    medium_chunk_width = radius * radius * 4;
    if (HAS_MEDIUM) {
        auto medium = init_chunks_of_size(medium_chunks, ref, x, y, chunk_size, medium_chunk_width, subdivide_percent, false);
        medium_chunks_location.append_array(ref);
        ref.clear();
        result.append_array(medium);
    }
    if (HAS_WATER) {
        auto water = init_chunks_of_size(water_chunks, ref, x, y, chunk_size, medium_chunk_width, 16.0 / chunk_size, true);
        water_chunks_location.append_array(ref);
        ref.clear();
        result.append_array(water);
    }
    if (HAS_GRASS) {
        auto gm = new MultiMesh();
        gm->set_transform_format(MultiMesh::TRANSFORM_3D);
        gm->set_use_custom_data(true);
        gm->set_instance_count(32175);
        gm->set_visible_instance_count(0);
        gm->set_mesh(grass_mesh);
        this->grass_mesh = new MultiMeshInstance3D();
        this->grass_mesh->set_multimesh(gm);
        this->grass_mesh->set_cast_shadows_setting(GeometryInstance3D::SHADOW_CASTING_SETTING_OFF);
        result.append(this->grass_mesh);
    }
    return result;
}

Dictionary GDTerrain::update_chunks_with_size(TypedArray<Node3D> chunks, uint64_t index, double x, double y, double cs, double r, double subdivide, bool is_water)
{
    auto old_coord = player_coord;
    auto current_coord = convert_position_to_coord(x, y, cs);
    auto delta = current_coord - old_coord;
    if (delta == Vector2()) {
        return Dictionary();
    }
    auto removed_locations = PackedVector2Array();
    auto updated_locations = PackedVector2Array();
    auto rad = (int)(r / 2.0);
    auto should_update = false;
    auto loc = Vector2();
    auto origin_delta = Vector2();
    auto should_exclude_update = false;
    for (int i = 0; i < chunks.size(); i++) {
        switch (index) {
        case lciMAIN: {
            loc = loaded_chunks_location[i];
        } break;
        case lciMED: {
            loc = medium_chunks_location[i];
        } break;
        case lciWATER: {
            loc = water_chunks_location[i];
        } break;
        }
        should_update = false;
        if (delta.x == -1 && loc.x == (old_coord.x + rad) * cs) {
            loc.x = (current_coord.x + -rad) * cs;
            should_update = true;
        }
        if (delta.x == 1 && loc.x == (old_coord.x - rad) * cs) {
            loc.x = (current_coord.x + rad) * cs;
            should_update = true;
        }
        if (delta.y == -1 && loc.y == (old_coord.y + rad) * cs) {
            loc.y = (current_coord.y + -rad) * cs;
            should_update = true;
        }
        if (delta.y == 1 && loc.y == (old_coord.y - rad) * cs) {
            loc.y = (current_coord.y + rad) * cs;
            should_update = true;
        }

        should_exclude_update = false;
        if (r > radius) {
            origin_delta = current_coord - convert_position_to_coord(loc.x, loc.y, cs);
            if (!is_water && abs(origin_delta.x) <= (int)(radius / 2.0) && abs(origin_delta.y) <= (int)(radius / 2.0)) {
                should_exclude_update = true;
                auto pos = ((Node3D*)(Object*)chunks[i])->get_position();
                pos.y = -1000;
                ((Node3D*)(Object*)chunks[i])->set_position(pos);
            } else {
                auto pos = ((Node3D*)(Object*)chunks[i])->get_position();
                pos.y = !is_water ? 0 : sea_level;
                ((Node3D*)(Object*)chunks[i])->set_position(pos);
            }
        }

        if (should_update) {
            switch (index) {
            case lciMAIN: {
                removed_locations.append(loaded_chunks_location[i]);
                loaded_chunks_location.set(i, loc);
            } break;
            case lciMED: {
                removed_locations.append(medium_chunks_location[i]);
                medium_chunks_location.set(i, loc);
            } break;
            case lciWATER: {
                removed_locations.append(water_chunks_location[i]);
                water_chunks_location.set(i, loc);
            } break;
            }
            updated_locations.append(loc);
            if (!should_exclude_update) {
                update_chunk_with_size((Node3D*)(Object*)chunks[i], loc.x, loc.y, cs, r, subdivide, is_water);
            }
        }
    }

    auto result = Dictionary();
    result["removed"] = removed_locations;
    result["updated"] = updated_locations;
    return result;
}

Dictionary GDTerrain::update_chunks(double x, double y)
{
    auto high = update_chunks_with_size(loaded_chunks, lciMAIN, x, y, chunk_size, radius, subdivide_percent, false);
    auto removed = (PackedVector2Array)high.get("removed", PackedVector2Array());
    auto updated = (PackedVector2Array)high.get("updated", PackedVector2Array());
    if (HAS_MEDIUM) {
        update_chunks_with_size(medium_chunks, lciMED, x, y, chunk_size, medium_chunk_width, subdivide_percent, false);
    }
    if (HAS_WATER) {
        update_chunks_with_size(water_chunks, lciWATER, x, y, chunk_size, medium_chunk_width, 16.0 / chunk_size, true);
    }
    set_player_coord_using_position(x, y, chunk_size);
    auto result = Dictionary();
    result["removed"] = removed;
    result["updated"] = updated;
    return result;
}

MeshInstance3D* GDTerrain::create_mesh(double x, double y, double size, double r, double subdivide)
{
    auto mesh = new ArrayMesh();
    auto plane = new PlaneMesh();
    plane->set_size(Vector2(size, size));
    auto subs = (int)(size * subdivide);
    plane->set_subdivide_depth(subs);
    plane->set_subdivide_width(subs);
    mesh->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, plane->get_mesh_arrays());
    mesh->surface_set_material(0, new ShaderMaterial());

    auto mi = new MeshInstance3D();
    mi->set_mesh(mesh);
    mi->set_name("mesh");

    if (r <= radius) {
        auto static_body = new StaticBody3D();
        static_body->set_name("static");
        auto collision_shape = new CollisionShape3D();
        auto hmap = new HeightMapShape3D();
        hmap->set_map_width(subs + 2);
        hmap->set_map_depth(subs + 2);
        collision_shape->set_shape(hmap);
        collision_shape->set_name("collision");
        collision_shape->set_scale(Vector3(size / (subs + 1.0), size / (subs + 1.0), size / (subs + 1.0)));
        collision_shape->rotate_y(Math_PI);
        static_body->add_child(collision_shape);
        mi->add_child(static_body);
    }

    return mi;
}

MeshInstance3D* GDTerrain::create_water_mesh(double x, double y, double size)
{
    auto mesh = new ArrayMesh();
    auto plane = new PlaneMesh();
    plane->set_size(Vector2(size, size));
    plane->set_subdivide_depth(8);
    plane->set_subdivide_width(8);
    mesh->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, plane->get_mesh_arrays());
    mesh->surface_set_material(0, new ShaderMaterial());

    auto mi = new MeshInstance3D();
    mi->set_mesh(mesh);
    mi->set_name("mesh");
    return mi;
}

Node3D* GDTerrain::create_chunk_with_size(TypedArray<Node3D> chunks, TypedArray<Vector2> locations, double x, double y, double cs, double r, double subdivide, bool is_water)
{
    auto node = new Node3D();
    if (!is_water) {
        node->add_child(create_mesh(x, y, cs, r, subdivide));
    } else {
        node->add_child(create_water_mesh(x, y, cs));
    }

    auto pos = node->get_position();
    pos.x = x;
    pos.z = y;

    if (r > radius) {
        auto origin_delta = convert_position_to_coord(x, y, cs);
        if (!is_water && abs(origin_delta.x) <= (int)(radius / 2.0) && abs(origin_delta.y) <= (int)(radius / 2)) {
            pos.y = -1000.0;
        } else {
            pos.y = !is_water ? 0.0 : sea_level;
        }
    }
    node->set_position(pos);

    locations.append(Vector2(x, y));
    chunks.append(node);

    return node;
}

void GDTerrain::update_mesh(MeshInstance3D* mi, double x, double y, double size, double r, double subdivide)
{
    auto mesh = (ArrayMesh*)*mi->get_mesh();
    auto mesh_data = mi->get_mesh()->surface_get_arrays(0);
    if (chunk_vertices.is_empty()) {
        auto positions = (PackedVector3Array)mesh_data[Mesh::ArrayType::ARRAY_VERTEX];
        for (int i = 0; i < positions.size(); i++) {
            chunk_vertices.append(positions[i]);
        }
    }

    auto R = size / (float)((int)(size * subdivide_percent));
    auto texture_size = size / R;
    auto biome_x_texture = blender->biome_texture(x / R, y / R, texture_size, texture_size, R, 0);
    auto biome_y_texture = blender->biome_texture(x / R, y / R, texture_size, texture_size, R, 1);

    auto A = Vector3();
    auto ys = blender->height_map(x, y, texture_size, texture_size, R);
    auto w = (size_t)texture_size + 2;
    if (r <= radius && mi->has_node("static")) {
        auto static_body = mi->get_node<StaticBody3D>("static");
        auto collision_shape = static_body->get_node<CollisionShape3D>("collision");
        auto hmap = (HeightMapShape3D*)*collision_shape->get_shape();
        auto array = PackedFloat32Array();
        array.resize(hmap->get_map_data().size());
        for (int i = 0; i < chunk_vertices.size(); i++) {
            A = chunk_vertices[i];
            size_t r = i / w;
            size_t c = i % w;
            size_t j = w * (w - r - 1) + (w - c - 1);
            A.y = ys[j];
            chunk_vertices[i].y = ys[j];
            array.set(i, A.y / collision_shape->get_scale().y);
            if (A.y > max_height_position.y && r <= radius && abs(A.x) < size / 2.0 && abs(A.z) < size / 2.0) {
                max_height_position = Vector3(A.x + x, A.y, A.z + y);
            }
        }
        hmap->set_map_data(array);
    } else {
        for (int i = 0; i < chunk_vertices.size(); i++) {
            A = chunk_vertices[i];
            size_t r = i / w;
            size_t c = i % w;
            size_t j = w * (w - r - 1) + (w - c - 1);
            A.y = ys[j];
            chunk_vertices[i].y = ys[j];
        }
    }

    mesh->clear_surfaces();
    mesh_data[Mesh::ArrayType::ARRAY_VERTEX] = chunk_vertices;
    mesh->add_surface_from_arrays(Mesh::PrimitiveType::PRIMITIVE_TRIANGLES, mesh_data);
    mesh->surface_set_material(0, new ShaderMaterial());

    auto mat = (ShaderMaterial*)*mesh->surface_get_material(0);
    mat->set_shader(biome_shader);
    mat->set_shader_parameter("texture_width", texture_size);
    mat->set_shader_parameter("texture_depth", texture_size);
    mat->set_shader_parameter("biome_x", biome_x_texture);
    mat->set_shader_parameter("biome_y", biome_y_texture);
    // mat->set_shader_parameter("height", height_texture);
}

void GDTerrain::update_water_mesh(MeshInstance3D* mi, double x, double y, double size, double r, double subdivide)
{
    auto mesh = mi->get_mesh();
    auto mat = (ShaderMaterial*)*mesh->surface_get_material(0);
    mat->set_shader(water_shader);
    mat->set_shader_parameter("noise", water_noise);
    mat->set_shader_parameter("ripples", water_ripples_noise);
}

void GDTerrain::update_chunk_with_size(Node3D* node, double x, double y, double cs, double r, double subdivide, bool is_water)
{
    auto mi = node->get_node<MeshInstance3D>("mesh");
    if (is_water) {
        update_water_mesh(mi, x, y, cs, r, subdivide);
    } else {
        update_mesh(mi, x, y, cs, r, subdivide);
    }
    auto pos = node->get_position();
    pos.x = x;
    pos.z = y;
    node->set_position(pos);
}

void GDTerrain::update_environment(double x, double y)
{
    auto old_position = player_position;
    player_position = Vector2(x, y);
    auto delta = player_position - old_position;
    place_grass(Vector2(grass_size * UtilityFunctions::signf(delta.x) * 2, grass_size * UtilityFunctions::signf(delta.y) * 2));
}

void GDTerrain::update_chunk_environment(Node3D* node)
{
}

void GDTerrain::place_grass(Vector2 delta)
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
    auto no_hit = new GDInOut();
    no_hit->set_data(false);
    auto t = Transform3D(Basis(), Vector3());
    t = t.scaled_local(Vector3(1, 1, 1) * 200);
    auto nt = t;
    auto horz = false;
    auto vert = false;
    auto pos = Vector3();
    auto p = Vector3();
    auto whn = Vector3();
    auto wh = 0.0;
    auto clr = Color(1, 1, 1, 1);
    auto nav = new GDNavigator();
    for (int i = 0; i < mm->get_visible_instance_count(); i++) {
        pos = grass_coords[i];
        horz = pos.x > player_position.x + grass_size || pos.x < player_position.x - grass_size;
        vert = pos.z > player_position.y + grass_size || pos.z < player_position.y - grass_size;

        if (horz || vert || ignore_delta) {
            p.x = pos.x + delta.x * (horz ? 1 : 0);
            p.z = pos.z + delta.y * (vert ? 1 : 0);
            blender->compute_biome_stats(p.x, p.z);
            no_hit->set_data(false);
            auto normal_height = nav->get_world_normal_height(grass_mesh->get_world_3d()->get_direct_space_state(), p.x, p.z, no_hit);
            wh = ((Vector3)normal_height.get("position", Vector3())).y;
            whn = normal_height.get("normal", Vector3());
            if (no_hit->get_data() || wh < sea_level || whn.distance_to(Vector3(0, 1, 0)) > 1 / sqrt(2.0)) {
                p.y = -1000;
            } else {
                p.y = wh;
            }
            clr = blender->color;
            clr.a = p.z;
            mm->set_instance_custom_data(i, clr);
            grass_coords[i] = p;
            nt = t;
            if (!no_hit->get_data() && !whn.is_zero_approx()) {
                auto new_y = whn.normalized();
                auto basis = nt.get_basis();
                basis.set_column(1, new_y);
                basis.set_column(0, -basis.get_column(2).cross(new_y));
                nt.set_basis(basis.orthonormalized());
                nt = nt.rotated_local(Vector3(0, 1, 0), UtilityFunctions::randf() * 2 * Math_PI);
                auto h = blender->grass_height(blender->biome, -p.x, -p.y);
                if (h == 0) {
                    p.y = -10000;
                }
                nt = nt.scaled_local(Vector3(1, h, 1) * 200);
            }
            mm->set_instance_transform(i, nt.translated(p));
        }
    }

    delete no_hit;
    delete nav;
}

void GDTerrain::init_grass()
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

void GDTerrain::hide_water(float y)
{
    if (y < sea_level - 1.5) {
        for (size_t i = 0; i < water_chunks.size(); i++) {
            ((Node3D*)(Object*)water_chunks[i])->set_visible(false);
        }
    } else {
        for (size_t i = 0; i < water_chunks.size(); i++) {
            ((Node3D*)(Object*)water_chunks[i])->set_visible(true);
        }
    }
}

void GDTerrain::set_player_coord_using_position(double x, double y, double cs)
{
    player_coord = convert_position_to_coord(x, y, cs);
}

Vector2 GDTerrain::convert_position_to_coord(double x, double y, double cs)
{
    return Vector2(floorf((x + cs / 2.0) / cs), floorf((y + cs / 2.0) / cs));
}

Vector3 GDTerrain::get_max_height_position()
{
    return max_height_position;
}

PackedVector2Array GDTerrain::get_loaded_chunks_location()
{
    return loaded_chunks_location;
}