#ifndef GDNAVIGATOR_H
#define GDNAVIGATOR_H

#include <godot_cpp/classes/collision_object3d.hpp>
#include <godot_cpp/classes/collision_shape3d.hpp>
#include <godot_cpp/classes/curve.hpp>
#include <godot_cpp/classes/shape3d.hpp>
#include <godot_cpp\classes\physics_direct_space_state3d.hpp>

#include <functional>
#include <string>
#include <vector>

#include "inout.h"

namespace godot {

enum MovementOptions { CAN_FLY = 1 << 0,
    UNDERGROUND = 1 << 1 };

class GDNavigator : public Object {
    GDCLASS(GDNavigator, Object)

private:
protected:
    static void _bind_methods();

public:
    GDNavigator();
    ~GDNavigator();

    float shape_max_bound(Shape3D* shape);
    float shape_height(Shape3D* shape);
    Shape3D* shape_increase(Shape3D* shape, float amount);
    CollisionShape3D* get_point_intersection(CollisionObject3D* p, Vector3 target);
    PackedVector3Array get_collisions_from_shape(CollisionObject3D* p, Shape3D* shape, Transform3D transform, int mask, TypedArray<RID> exclude);
    TypedArray<Dictionary> get_intersections_from_shape(CollisionObject3D* p, Shape3D* shape, Transform3D transform, int mask, TypedArray<RID> exclude);
    Quaternion rotation_vector(Vector3 from, Vector3 to);
    Vector3 get_ray_collision(CollisionObject3D* p, Vector3 from, Vector3 direction, int mask);
    CollisionShape3D* get_ray_intersection(CollisionObject3D* p, Vector3 from, Vector3 target);
    bool get_shape_intersection(CollisionObject3D* p, Vector3 from, Vector3 target, Shape3D* shape, bool exclude_ground);
    PackedFloat32Array get_shape_distance_away(CollisionObject3D* p, Vector3 from, Vector3 target, Shape3D* shape, bool exclude_ground);
    bool get_shape_collides(CollisionObject3D* p, Vector3 from, Vector3 target, Shape3D* shape, bool exclude_ground);
    float get_world_height_from_node(CollisionObject3D* p, float x, float z, GDInOut* no_hit = nullptr);
    float get_world_height(PhysicsDirectSpaceState3D* space_state, float x, float z, GDInOut* no_hit = nullptr);
    Dictionary get_world_normal_height(PhysicsDirectSpaceState3D* space_state, float x, float z, GDInOut* no_hit = nullptr);
    Vector3 minimum_score(Dictionary nodes, Dictionary scores);
    PackedVector3Array reconstruct_path(Dictionary came_from, Vector3 target);
    PackedVector3Array neighbours(CollisionObject3D* p, Vector3 from, int directions, float distance, Vector3 target, Shape3D* shape, uint64_t options);
    PackedColorArray all_neighbours(CollisionObject3D* p, Vector3 from, int directions, float distance, Vector3 target, Shape3D* shape, uint64_t options);
    PackedVector3Array astar(CollisionObject3D* p, Vector3 target, Shape3D* shape, uint64_t options, float search_radius, float margin_from_obs = 0.5);
    bool will_collide(CollisionObject3D* p, Shape3D* shape, Vector3 target, bool exclude_ground);
    PackedVector3Array find_target_path(CollisionObject3D* p, Vector3 target, Shape3D* shape, uint64_t options, float search_radius, float margin_from_obs = 0.5);
    Vector3 find_target(CollisionObject3D* p, Vector3 target, Shape3D* shape, uint64_t options, float search_radius, float margin_from_obs = 0.5);
    Vector3 nearest_non_colliding_position(CollisionObject3D* p, Vector3 from, Vector3 target, Shape3D* shape, uint64_t options);
};

}

#endif