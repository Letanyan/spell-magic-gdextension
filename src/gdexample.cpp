#include "gdexample.h"

#include <godot_cpp/classes/multi_mesh.hpp>
#include <godot_cpp/classes/multi_mesh_instance3d.hpp>

#include <godot_cpp/classes/world3d.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/vector2.hpp>
#include <godot_cpp/variant/vector3.hpp>

using namespace godot;

void GDExample::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("place_grass"), &GDExample::place_grass);
}

GDExample::GDExample()
{
    // Initialize any variables here.
}

GDExample::~GDExample()
{
    // Add your cleanup here.
}

void GDExample::place_grass(
    Vector2 delta,
    Array grass_coords,
    Variant grass_mesh,
    Variant blender,
    Variant navigator,
    Vector2 player_position,
    float grass_size,
    bool ignore_delta)
{
    /*
        var mm: MultiMesh = grass_mesh.multimesh

        for i in range(mm.visible_instance_count):
            var pos: Vector3 = grass_coords[i]
            var horz = pos.x > player_position.x + grass_size or pos.x < player_position.x - grass_size
            var vert = pos.z > player_position.y + grass_size or pos.z < player_position.y - grass_size
            var dist = Vector2(pos.x, pos.z).distance_to(player_position) / grass_size
            if dist < 1.0:
                dist = 1.0
            else:
                dist = (1.0 - (dist - 1.0))

            if horz or vert or ignore_delta:
                var p = Vector3(pos.x + delta.x * (1 if horz else 0), 0, pos.z + delta.y * (1 if vert else 0))
                var biome_dict = blender.compute_biome_distances(p.x, p.z)
                if biome_dict["biome"] != World.Biome.GRASSLAND:
                    p.y = -1000
                else:
                    var no_hit = Ptr.new(false)
                    var wh = Navigator.get_world_height(grass_mesh.get_world_3d().direct_space_state, p.x, p.z, no_hit)
                    if no_hit.data or wh < Globals.sea_level():
                        p.y = -1000
                    else:
                        p.y = wh
                mm.set_instance_custom_data(i, biome_dict["color"])
                grass_coords[i] = p
                var t = Transform3D(Basis(), p)
                t = t.scaled_local(Vector3(100, 100, 100) * 2 * dist)
                mm.set_instance_transform(i, t)
            else:
                var p = pos
                grass_coords[i] = p
                var t = Transform3D(Basis(), p)
                t = t.scaled_local(Vector3(100, 100, 100) * 2 * dist)
                mm.set_instance_transform(i, t)
    */

    Ref<MultiMesh> mm = ((Object*)grass_mesh)->call("get_multimesh");
    Ref<World3D> world = ((Object*)grass_mesh)->call("get_world_3d");

    for (int i = 0; i < mm->get_visible_instance_count(); i++) {
        Vector3 pos = grass_coords[i];
        bool horz = pos.x > player_position.x + grass_size
            || pos.x < player_position.x - grass_size;
        bool vert = pos.z > player_position.y + grass_size
            || pos.z < player_position.y - grass_size;
        auto dist
            = Vector2(pos.x, pos.z).distance_to(player_position) / grass_size;
        if (dist < 1.0) {
            dist = 1.0;
        } else {
            dist = (1.0 - (dist - 1.0));
        }

        if (horz || vert || ignore_delta) {
            auto p = Vector3(pos.x + delta.x * (horz ? 1 : 0), 0, pos.z + delta.y * (vert ? 1 : 0));
            Dictionary biome_dict = ((Object*)blender)->call("compute_biome_distances", p.x, p.z);
            int biome = biome_dict["biome"];
            if (biome != 1) {
                p.y = -1000;
            } else {
                float wh = ((Object*)navigator)->call("get_world_height_", world->get_direct_space_state(), p.x, p.z);
                if (wh < 200.0) {
                    p.y = -1000;
                } else {
                    p.y = wh;
                }
            }
            Color color = biome_dict["color"];
            mm->set_instance_custom_data(i, color);
            grass_coords[i] = p;
            auto t = Transform3D(Basis(), p);
            t = t.scaled_local(Vector3(100, 100, 100) * 2 * dist);
            mm->set_instance_transform(i, t);
        } else {
            auto t = Transform3D(Basis(), pos);
            t = t.scaled_local(Vector3(100, 100, 100) * 2 * dist);
            mm->set_instance_transform(i, t);
        }
    }
}