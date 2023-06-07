#ifndef GDEXAMPLE_H
#define GDEXAMPLE_H

#include <godot_cpp/classes/fast_noise_lite.hpp>
#include <godot_cpp/classes/multi_mesh_instance3d.hpp>
#include <godot_cpp/classes/ref_counted.hpp>

namespace godot {

class GDExample : public Object {
    GDCLASS(GDExample, Object)

private:
protected:
    static void _bind_methods();

public:
    GDExample();
    ~GDExample();

    void place_grass(
        Vector2 delta,
        Array grass_coords,
        Variant grass_mesh,
        Variant blender,
        Variant navigator,
        Vector2 player_position,
        float grass_size,
        bool ignore_delta);
};

}

#endif