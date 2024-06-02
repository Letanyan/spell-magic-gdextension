#ifndef GDTERRAIN_H
#define GDTERRAIN_H

#include "noise_blender.h"
#include <godot_cpp/classes/array_mesh.hpp>
#include <godot_cpp/classes/collision_shape3d.hpp>
#include <godot_cpp/classes/fast_noise_lite.hpp>
#include <godot_cpp/classes/height_map_shape3d.hpp>
#include <godot_cpp/classes/mesh_data_tool.hpp>
#include <godot_cpp/classes/multi_mesh_instance3d.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/noise.hpp>
#include <godot_cpp/classes/plane_mesh.hpp>
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/classes/shader.hpp>
#include <godot_cpp/classes/shader_material.hpp>
#include <godot_cpp/classes/static_body3d.hpp>

namespace godot {

class GDTerrain : public Object {
    GDCLASS(GDTerrain, Object)

private:
protected:
    static void _bind_methods();
    GDNoiseBlender blender;

public:
    GDTerrain();
    ~GDTerrain();
};

}

#endif