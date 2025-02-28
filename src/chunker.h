#ifndef GDCHUNKER_H
#define GDCHUNKER_H

#include "inout.h"
#include "noise_blender.h"
#include "ring_buffer.h"
#include <godot_cpp/classes/array_mesh.hpp>
#include <godot_cpp/classes/collision_shape3d.hpp>
#include <godot_cpp/classes/fast_noise_lite.hpp>
#include <godot_cpp/classes/height_map_shape3d.hpp>
#include <godot_cpp/classes/mesh_data_tool.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/multi_mesh.hpp>
#include <godot_cpp/classes/multi_mesh_instance3d.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/noise.hpp>
#include <godot_cpp/classes/plane_mesh.hpp>
#include <godot_cpp/classes/random_number_generator.hpp>
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/classes/shader.hpp>
#include <godot_cpp/classes/shader_material.hpp>
#include <godot_cpp/classes/static_body3d.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <queue>

#ifndef HAS_MEDIUM
#define HAS_MEDIUM 1
#endif
#ifndef MEDIUM_SCALE
#define MEDIUM_SCALE 1.0
#endif

#ifndef HAS_WATER
#define HAS_WATER 1
#endif
#ifndef HAS_GRASS
#define HAS_GRASS 1
#endif

namespace godot {

MAKE_TYPED_ARRAY(Vector4i, Variant::VECTOR4I)

struct ChunkUpdateParameters {
public:
    ChunkUpdateParameters();

    Vector2i coord0;
    Vector2i coord1;
    Vector2i delta;
    Vector2i saved_player_coord;
    float res0;
    float res1;
    bool is_swap;
};

class GDChunker : public RefCounted {
    GDCLASS(GDChunker, RefCounted)

private:
    PackedFloat32Array update_chunk_array;

protected:
    static void _bind_methods();

    Shader* biome_shader;
    Shader* water_shader;
    NoiseTexture2D* water_noise;
    NoiseTexture2D* water_ripples_noise;
    NoiseTexture2D* noise_texture;

    // keys are the LOD level. 1 << (value << 1) - (sum of all previous number of chunks) represents the number of chunks for this LOD level
    TypedArray<int64_t> lod_levels;
    int64_t track_biomes_upto_lod; // lods below will have full hi-res biome maps
    GDNoiseBlender* blender;

    Dictionary chunk_lods; // [Vector2i(coord)]int(LOD)
    Dictionary chunk_rids; // [Vector2i(coord)]RID(instance)
    Dictionary mesh_rids; // [Vector2i(coord)]RID(mesh)
    Dictionary mats; // [Vector2i(coord)]ShaderMaterial
    Dictionary water_chunk_rids; // [Vector2i(coord)]RID(instance)
    Dictionary water_mesh_rids; // [Vector2i(coord)]RID(mesh)
    Dictionary water_mats; // [Vector2i(coord)]ShaderMaterial
    Dictionary chunk_positions; // [Vector2i(coord)]Vector2
    Dictionary bodies; // [Vector2i(coord)]StaticBody
    Dictionary height_maps; // [Vector2i(coord)]HeightMapShape3D
    Dictionary biome_maps; // [Vector2i(coord)]PackedInt32Array(biome)
    Dictionary color_maps; // [Vector2i(coord)]PackedColorArray
    Dictionary distances_maps; // [Vector2i(coord)]TypedArray<PackedFloat32Array>

    double grass_size;
    MultiMeshInstance3D* grass_mesh;
    Mesh* grass_mesh_instance;
    PackedVector3Array grass_coords;

    bool find_bound_coords;
    Vector3 min_height_position;
    Vector3 max_height_position;
    std::vector<PackedVector3Array> chunk_vertices;
    std::vector<Array> chunk_mesh_data;

    Vector2i player_coord;
    Vector2 player_position;
    std::queue<ChunkUpdateParameters> chunk_update_queue;

    float chunk_width;
    float chunk_resolution;
    float sea_level;

public:
    GDChunker();
    ~GDChunker();

    void init(float chunk_width, float chunk_resolution, float sea_level, float grass_size, GDNoiseBlender* blender, TypedArray<int> lods, bool find_bound_coords);
    void set_biome_shader(Shader* biome_shader);
    void set_water_shader(Shader* water_shader);
    void set_water_noise(NoiseTexture2D* water_noise);
    void set_water_ripples_noise(NoiseTexture2D* water_ripples_noise);
    void set_noise_texture(NoiseTexture2D* noise_texture);
    void set_grass_mesh(Mesh* grass_mesh_instance);

    void init_chunks(float x, float z);
    void deinit();
    void set_world(Node3D* world);
    TypedArray<RID> create_mesh(float size, float res);
    HeightMapShape3D* create_height_map_shape(float size, float res);
    StaticBody3D* create_static_body(float size, float res, HeightMapShape3D* map);
    void update_chunk(Vector2i coord, Vector2i new_coord, float res, Vector2i saved_player_coord);
    void update_water_chunk(Vector2i coord, Vector2i new_coord);
    bool has_chunks_to_update();
    Dictionary update_chunks_in_queue(int64_t start, int64_t limit);
    void swap_keys(Dictionary dict, Variant key0, Variant key1);
    void move_key(Dictionary dict, Variant from, Variant to);
    TypedArray<Vector2i> update_chunks(float x, float z);
    TypedArray<Vector2i> update_chunks_impl(Vector2i delta);
    TypedArray<Vector2i> edges_at_direction(int64_t lod, Vector2i direction);
    Vector2i edges_part_of_transition(Vector2i old_coord, Vector2i new_coord, Vector2i saved_player_coord);

    Vector2i convert_position_to_coord(double x, double z);
    Vector2 convert_coord_to_position(int64_t x, int64_t z);
    int64_t subdivisions(float res);
    float resolution(int64_t lod);
    float height_map_scale(int64_t lod);
    Vector3 get_max_height_position();
    Vector3 get_min_height_position();
    Vector2i get_player_coord();

    PackedVector3Array get_chunk_vertices();
    int64_t get_track_biomes_upto_lod();
    Dictionary get_chunk_lods();
    float get_chunk_width();

    Vector4 height_at_position(Vector2i coord, double x, double z);
    Dictionary terrain_normal(double x, double z);
    bool terrain_normal_inplace(double x, double z, Dictionary result);
    float get_noise_scale();
    static bool contains_neighbour_point(const PackedVector2Array& collection, Vector2 point, float spacing);
    Dictionary group_spawn_points(Vector2i coord, float spacing);
    int32_t get_biome_at_position(double x, double z);
    Color get_color_at_position(double x, double z);
    PackedFloat32Array get_distances_at_position(double x, double z);

    void update_environment(double x, double y);
    void place_grass(Vector2 delta);
    void init_grass();
    int count_grass_instances();
    void hide_water(float y, bool force_update);
};

}

#endif