#ifndef GDTERRAIN_H
#define GDTERRAIN_H

#include "noise_blender.h"
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

#ifndef HAS_MEDIUM
#define HAS_MEDIUM true
#endif
#ifndef HAS_WATER
#define HAS_WATER 1
#endif
#ifndef HAS_GRASS
#define HAS_GRASS 1
#endif

namespace godot {

class GDTerrain : public RefCounted {
    GDCLASS(GDTerrain, RefCounted)

    bool first_run;

private:
protected:
    static void _bind_methods();

    double sea_level;

    GDNoiseBlender* blender;
    double chunk_size;
    double radius;
    double subdivide_percent;

    double grass_size;

    double medium_chunk_width;

    Vector2 player_position;
    Vector2 player_coord;
    Vector3 max_height_position;

    Shader* biome_shader;
    Shader* water_shader;
    NoiseTexture2D* water_noise;
    NoiseTexture2D* water_ripples_noise;

    PackedVector2Array loaded_chunks_location;
    TypedArray<Node3D> loaded_chunks;
    PackedVector2Array medium_chunks_location;
    TypedArray<Node3D> medium_chunks;
    PackedVector2Array water_chunks_location;
    TypedArray<Node3D> water_chunks;

    PackedVector3Array chunk_vertices;

    MultiMeshInstance3D* grass_mesh;
    PackedVector3Array grass_coords;

public:
    enum LoadedChunkIndex {
        lciMAIN,
        lciMED,
        lciWATER
    };

    GDTerrain();
    ~GDTerrain();

    void init(GDNoiseBlender* b, double cs = 256, double r = 3, double subdivide = 0.0625, double medium_chunk_width = 18);
    void set_biome_shader(Shader* biome_shader);
    void set_water_shader(Shader* water_shader);
    void set_water_noise(NoiseTexture2D* water_noise);
    void set_water_ripples_noise(NoiseTexture2D* water_ripples_noise);
    void set_sea_level(double level);

    TypedArray<Node3D> init_chunks_of_size(TypedArray<Node3D> chunks, TypedArray<Vector2> locations, double x, double y, double cs, double r, double subdivide, bool is_water);
    TypedArray<Node3D> init_chunks(double x, double y, Mesh* grass_mesh);
    Dictionary update_chunks_with_size(TypedArray<Node3D> chunks, uint64_t index, double x, double y, double cs, double r, double subdivide, bool is_water);
    Dictionary update_chunks(double x, double y);
    MeshInstance3D* create_mesh(double x, double y, double size, double r, double subdivide);
    MeshInstance3D* create_water_mesh(double x, double y, double size);
    Node3D* create_chunk_with_size(TypedArray<Node3D> chunks, TypedArray<Vector2> locations, double x, double y, double cs, double r, double subdivide, bool is_water);
    void update_mesh(MeshInstance3D* mi, double x, double y, double size, double r, double subdivide);
    void update_water_mesh(MeshInstance3D* mi, double x, double y, double size, double r, double subdivide);
    void update_chunk_with_size(Node3D* node, double x, double y, double cs, double r, double subdivide, bool is_water);
    void update_environment(double x, double y);
    void update_chunk_environment(Node3D* node);
    void place_grass(Vector2 delta);
    void init_grass();
    void hide_water(float y);
    void set_player_coord_using_position(double x, double y, double cs);
    Vector2 convert_position_to_coord(double x, double y, double cs);

    Vector3 get_max_height_position();
    PackedVector2Array get_loaded_chunks_location();
};

}

VARIANT_ENUM_CAST(GDTerrain::LoadedChunkIndex);

#endif