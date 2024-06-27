#include "navigator.h"
#include "inout.h"
#include <bit>
#include <iostream>
#include <math.h>
#include <queue>

#include <godot_cpp/classes/box_shape3d.hpp>
#include <godot_cpp/classes/capsule_shape3d.hpp>
#include <godot_cpp/classes/collision_object3d.hpp>
#include <godot_cpp/classes/collision_shape3d.hpp>
#include <godot_cpp/classes/sphere_shape3d.hpp>
#include <godot_cpp/classes/world3d.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp\classes\physics_direct_space_state3d.hpp>
#include <godot_cpp\classes\physics_point_query_parameters3d.hpp>
#include <godot_cpp\classes\physics_ray_query_parameters3d.hpp>
#include <godot_cpp\classes\physics_shape_query_parameters3d.hpp>

using namespace godot;

void GDNavigator::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("popcnt", "number"), &GDNavigator::popcnt);
    ClassDB::bind_method(D_METHOD("shape_max_bound", "shape"), &GDNavigator::shape_max_bound);
    ClassDB::bind_method(D_METHOD("shape_height", "shape"), &GDNavigator::shape_height);
    ClassDB::bind_method(D_METHOD("shape_increase", "shape", "amount"), &GDNavigator::shape_increase);
    ClassDB::bind_method(D_METHOD("get_point_intersection", "p", "target"), &GDNavigator::get_point_intersection);
    ClassDB::bind_method(D_METHOD("get_collisions_from_shape", "p", "shape", "transform", "mask", "exclude"), &GDNavigator::get_collisions_from_shape);
    ClassDB::bind_method(D_METHOD("get_intersections_from_shape", "p", "shape", "transform", "mask", "exclude"), &GDNavigator::get_intersections_from_shape);
    ClassDB::bind_method(D_METHOD("rotation_vector", "from", "to"), &GDNavigator::rotation_vector);
    ClassDB::bind_method(D_METHOD("get_ray_collision", "p", "from", "direction", "mask"), &GDNavigator::get_ray_collision);
    ClassDB::bind_method(D_METHOD("get_ray_intersection", "p", "from", "target"), &GDNavigator::get_ray_intersection);
    ClassDB::bind_method(D_METHOD("get_shape_intersection", "p", "from", "target", "shape", "exclude_ground"), &GDNavigator::get_shape_intersection);
    ClassDB::bind_method(D_METHOD("get_shape_distance_away", "p", "from", "target", "shape", "exclude_ground"), &GDNavigator::get_shape_distance_away);
    ClassDB::bind_method(D_METHOD("get_shape_collides", "p", "from", "target", "shape", "exclude_ground"), &GDNavigator::get_shape_collides);
    ClassDB::bind_method(D_METHOD("get_world_height_from_node", "p", "x", "z", "no_hit"), &GDNavigator::get_world_height_from_node, DEFVAL(nullptr));
    ClassDB::bind_method(D_METHOD("get_world_height", "space_state", "x", "z", "no_hit"), &GDNavigator::get_world_height, DEFVAL(nullptr));
    ClassDB::bind_method(D_METHOD("get_world_normal_height", "space_state", "x", "z", "no_hit"), &GDNavigator::get_world_normal_height, DEFVAL(nullptr));
    ClassDB::bind_method(D_METHOD("minimum_score", "nodes", "scores"), &GDNavigator::minimum_score);
    ClassDB::bind_method(D_METHOD("reconstruct_path", "came_from", "target"), &GDNavigator::reconstruct_path);
    ClassDB::bind_method(D_METHOD("neighbours", "p", "from", "directions", "distance", "target", "shape", "options"), &GDNavigator::neighbours);
    ClassDB::bind_method(D_METHOD("astar", "p", "target", "shape", "options", "search_radius", "margin_from_obs"), &GDNavigator::astar, DEFVAL(0.5));
    ClassDB::bind_method(D_METHOD("will_collide", "p", "shape", "exclude_ground"), &GDNavigator::will_collide);
    ClassDB::bind_method(D_METHOD("find_target_path", "p", "target", "shape", "options", "search_radius", "margin_from_obs"), &GDNavigator::find_target_path, DEFVAL(0.5));
    ClassDB::bind_method(D_METHOD("find_target", "p", "target", "shape", "options", "search_radius", "margin_from_obs"), &GDNavigator::find_target, DEFVAL(0.5));
}

GDNavigator::GDNavigator()
{
}

GDNavigator::~GDNavigator()
{
    // Add your cleanup here.
}

int GDNavigator::popcnt(unsigned int number)
{
    return __popcnt(number);
}

float GDNavigator::shape_max_bound(Shape3D* shape)
{
    // auto s = (Shape3D*)(Object*)shape;
    auto s = shape;
    if (dynamic_cast<BoxShape3D*>(s) != nullptr) {
        auto box = dynamic_cast<BoxShape3D*>(s);
        auto x = box->get_size().x;
        auto y = box->get_size().y;
        auto z = box->get_size().z;
        auto result = x > y ? x : y;
        return result > z ? result : z;
    } else if (dynamic_cast<SphereShape3D*>(s) != nullptr) {
        auto sphere = dynamic_cast<SphereShape3D*>(s);
        return sphere->get_radius() * 2.0;
    } else if (dynamic_cast<CapsuleShape3D*>(s) != nullptr) {
        auto capsule = dynamic_cast<CapsuleShape3D*>(s);
        auto h = capsule->get_height();
        auto r = capsule->get_radius() * 2.0;
        return h > r ? h : r;
    }
    return 1.0;
}

float GDNavigator::shape_height(Shape3D* shape)
{
    auto s = (Shape3D*)(Object*)shape;
    if (dynamic_cast<BoxShape3D*>(s) != nullptr) {
        auto box = dynamic_cast<BoxShape3D*>(s);
        auto y = box->get_size().y;
        return y;
    } else if (dynamic_cast<SphereShape3D*>(s) != nullptr) {
        auto sphere = dynamic_cast<SphereShape3D*>(s);
        return sphere->get_radius() * 2.0;
    } else if (dynamic_cast<CapsuleShape3D*>(s) != nullptr) {
        auto capsule = dynamic_cast<CapsuleShape3D*>(s);
        auto h = capsule->get_height();
        return h;
    }
    return 1.0;
}

Shape3D* GDNavigator::shape_increase(Shape3D* shape, float amount)
{
    auto s = (Shape3D*)*((Shape3D*)(Object*)shape)->duplicate();
    if (dynamic_cast<BoxShape3D*>(s) != nullptr) {
        auto box = dynamic_cast<BoxShape3D*>(s);
        auto size = box->get_size();
        size.x += amount;
        size.y += amount;
        size.z += amount;
        box->set_size(size);
        return box;
    } else if (dynamic_cast<SphereShape3D*>(s) != nullptr) {
        auto sphere = dynamic_cast<SphereShape3D*>(s);
        sphere->set_radius(sphere->get_radius() + amount);
        return sphere;
    } else if (dynamic_cast<CapsuleShape3D*>(s) != nullptr) {
        auto capsule = dynamic_cast<CapsuleShape3D*>(s);
        capsule->set_height(capsule->get_height() + amount);
        capsule->set_radius(capsule->get_radius() + amount);
        return capsule;
    }
    return shape;
}

CollisionShape3D* GDNavigator::get_point_intersection(CollisionObject3D* p, Vector3 target)
{
    auto space_state = p->get_world_3d()->get_direct_space_state();
    auto query = new PhysicsPointQueryParameters3D();
    query->set_position(target);
    query->set_collision_mask(~1);
    auto exclude = TypedArray<RID>();
    exclude.append(p->get_rid());
    query->set_exclude(exclude);
    TypedArray<Dictionary> result = space_state->intersect_point(query);
    if (result.is_empty()) {
        return nullptr;
    }
    if (((Dictionary)result[0]).is_empty()) {
        return nullptr;
    }
    auto object = (CollisionObject3D*)(Object*)result[0].get("collider");
    if (object == nullptr) {
        return nullptr;
    }
    CollisionShape3D* c = nullptr;
    for (int i = 0; i < object->get_child_count(); i++) {
        auto o = (CollisionShape3D*)object->get_child(i);
        if (o->get_name() == StringName("shape")) {
            c = o;
            break;
        }
    }
    return c;
}

PackedVector3Array GDNavigator::get_collisions_from_shape(CollisionObject3D* p, Shape3D* shape, Transform3D transform, int mask, TypedArray<RID> exclude)
{
    auto space_state = p->get_world_3d()->get_direct_space_state();
    auto query = new PhysicsShapeQueryParameters3D();
    query->set_collision_mask(mask);
    query->set_shape(shape);
    query->set_transform(transform);
    exclude.append(p->get_rid());
    query->set_exclude(exclude);
    return space_state->collide_shape(query);
}

TypedArray<Dictionary> GDNavigator::get_intersections_from_shape(CollisionObject3D* p, Shape3D* shape, Transform3D transform, int mask, TypedArray<RID> exclude)
{
    auto space_state = p->get_world_3d()->get_direct_space_state();
    auto query = new PhysicsShapeQueryParameters3D();
    query->set_collision_mask(mask);
    query->set_shape(shape);
    query->set_transform(transform);
    exclude.append(p->get_rid());
    query->set_exclude(exclude);
    return space_state->intersect_shape(query);
}

Quaternion GDNavigator::rotation_vector(Vector3 from, Vector3 to)
{
    auto q = Quaternion();
    auto a = from.cross(to);
    q.x = a.x;
    q.y = a.y;
    q.z = a.z;
    q.w = sqrtf(from.length_squared() * to.length_squared()) + from.dot(to);
    q.normalize();
    return q;
}

Vector3 GDNavigator::get_ray_collision(CollisionObject3D* p, Vector3 from, Vector3 direction, int mask)
{
    auto space_state = p->get_world_3d()->get_direct_space_state();
    auto query = new PhysicsRayQueryParameters3D();
    query->set_from(from);
    query->set_to(from + direction);
    query->set_collision_mask(mask);
    auto exclude = TypedArray<RID>();
    query->set_exclude(exclude);
    auto result = space_state->intersect_ray(query);
    if (result.is_empty()) {
        return Vector3();
    }
    return result.get("position", Vector3());
}

CollisionShape3D* GDNavigator::get_ray_intersection(CollisionObject3D* p, Vector3 from, Vector3 target)
{
    auto obj = (CollisionObject3D*)(Object*)p;
    auto space_state = obj->get_world_3d()->get_direct_space_state();
    auto query = new PhysicsRayQueryParameters3D();
    query->set_from(from + Vector3(0, 1, 0));
    query->set_to(target + Vector3(0, 1, 0));
    query->set_collision_mask(~1);
    auto exclude = TypedArray<RID>();
    exclude.append(obj->get_rid());
    query->set_exclude(exclude);
    auto result = space_state->intersect_ray(query);
    if (result.is_empty()) {
        return nullptr;
    }
    auto object = (CollisionObject3D*)(Object*)result.get("collider", nullptr);
    if (object == nullptr) {
        return nullptr;
    }
    CollisionShape3D* c = nullptr;
    for (int i = 0; i < object->get_child_count(); i++) {
        auto o = (CollisionShape3D*)object->get_child(i);
        if (o->get_name() == StringName("shape") || o->get_name() == StringName("Collision")) {
            c = o;
            break;
        }
    }
    return c;
}

bool GDNavigator::get_shape_intersection(CollisionObject3D* p, Vector3 from, Vector3 target, Shape3D* shape, bool exclude_ground)
{
    auto space_state = p->get_world_3d()->get_direct_space_state();
    auto query = new PhysicsShapeQueryParameters3D();
    query->set_collision_mask(~(exclude_ground ? 1 : 0));
    auto exclude = TypedArray<RID>();
    exclude.append(p->get_rid());
    query->set_exclude(exclude);
    query->set_shape(shape);
    query->set_transform(Transform3D().translated(target));
    auto result = space_state->intersect_shape(query, 4);
    return !result.is_empty();
}

PackedFloat32Array GDNavigator::get_shape_distance_away(CollisionObject3D* p, Vector3 from, Vector3 target, Shape3D* shape, bool exclude_ground)
{
    auto space_state = p->get_world_3d()->get_direct_space_state();
    auto query = new PhysicsShapeQueryParameters3D();
    query->set_collision_mask(~(exclude_ground ? 1 : 0));
    auto exclude = TypedArray<RID>();
    exclude.append(p->get_rid());
    query->set_exclude(exclude);
    query->set_shape(shape);
    query->set_transform(Transform3D().translated(from));
    query->set_motion(target - from);
    auto result = space_state->cast_motion(query);
    return result;
}

bool GDNavigator::get_shape_collides(CollisionObject3D* p, Vector3 from, Vector3 target, Shape3D* shape, bool exclude_ground)
{
    auto result = get_shape_distance_away(p, from, target, shape, exclude_ground);
    return !(result[0] == 1.0 && result[1] == 1.0);
}

float GDNavigator::get_world_height_from_node(CollisionObject3D* p, float x, float z, GDInOut* no_hit)
{
    auto space_state = p->get_world_3d()->get_direct_space_state();
    auto query = new PhysicsRayQueryParameters3D();
    query->set_from(Vector3(x, 5000, z));
    query->set_to(Vector3(x, -5000, z));
    query->set_collision_mask(1);
    auto result = space_state->intersect_ray(query);
    if (result.is_empty()) {
        if (no_hit != nullptr)
            no_hit->set_data(true);
        return 0;
    } else {
        if (no_hit != nullptr)
            no_hit->set_data(false);
        return ((Vector3)result.get("position", Vector3())).y;
    }
}

float GDNavigator::get_world_height(PhysicsDirectSpaceState3D* space_state, float x, float z, GDInOut* no_hit)
{
    auto query = new PhysicsRayQueryParameters3D();
    query->set_from(Vector3(x, 5000, z));
    query->set_to(Vector3(x, -5000, z));
    query->set_collision_mask(1);
    auto result = space_state->intersect_ray(query);
    if (result.is_empty()) {
        if (no_hit != nullptr)
            no_hit->set_data(true);
        return 0;
    } else {
        if (no_hit != nullptr)
            no_hit->set_data(false);
        return ((Vector3)result.get("position", Vector3())).y;
    }
}

Dictionary GDNavigator::get_world_normal_height(PhysicsDirectSpaceState3D* space_state, float x, float z, GDInOut* no_hit)
{
    auto query = new PhysicsRayQueryParameters3D();
    query->set_from(Vector3(x, 5000, z));
    query->set_to(Vector3(x, -5000, z));
    query->set_collision_mask(1);
    auto result = space_state->intersect_ray(query);
    if (result.is_empty()) {
        if (no_hit != nullptr)
            no_hit->set_data(true);
        return Dictionary();
    } else {
        if (no_hit != nullptr)
            no_hit->set_data(false);
        return result;
    }
}

Vector3 GDNavigator::minimum_score(Dictionary nodes, Dictionary scores)
{
    auto result = Vector3();
    float best = INFINITY;
    for (int i = 0; i < nodes.size(); i++) {
        auto v = (Vector3)nodes.keys()[i];
        auto dist = (float)scores.get(v, INFINITY);
        if (dist < best) {
            best = dist;
            result = v;
        }
    }
    return result;
}

PackedVector3Array GDNavigator::reconstruct_path(Dictionary came_from, Vector3 target)
{
    auto result = PackedVector3Array();
    result.append(target);
    auto current = target;
    while (came_from.has(current)) {
        current = came_from[current];
        result.insert(0, current);
    }
    return result;
}

PackedVector3Array GDNavigator::neighbours(CollisionObject3D* p, Vector3 from, int directions, float distance, Vector3 target, Shape3D* shape, uint64_t options)
{
    auto result = PackedVector3Array();
    auto direction = target - from;
    direction.y = 0.0;
    direction.normalize();
    auto angle = 2 * Math_PI / (float)directions;
    for (int a = 0; a < directions; a++) {
        auto to = from + direction * distance;
        if (options & MovementOptions::CAN_FLY == 0) {
            to.y = get_world_height_from_node(p, to.x, to.z) + shape_height(shape) / 2.0 + 0.05;
        }
        to.snap(Vector3(distance, distance, distance));
        auto distance_away = get_shape_distance_away(p, from, to, shape, (options & MovementOptions::UNDERGROUND) != 0);
        if (distance_away[0] == 1.0 && distance_away[1] == 1.0) {
            result.append(to);
        } else if (distance_away[0] >= 0.1 && !get_shape_collides(p, from, to, shape, (options & MovementOptions::UNDERGROUND) != 0)) {
            result.append(from.lerp(to, distance_away[0]));
        }
        direction.rotate(Vector3(0, 1, 0), angle);
    }

    auto final_result = PackedVector3Array();
    if ((options & MovementOptions::CAN_FLY) != 0) {
        for (int i = 0; i < result.size(); i++) {
            auto r = (Vector3)result[i];
            final_result.append(r + Vector3(0, distance, 0));
        }
    }
    if ((options & MovementOptions::UNDERGROUND) != 0) {
        for (int i = 0; i < result.size(); i++) {
            auto r = (Vector3)result[i];
            final_result.append(r + Vector3(0, -distance, 0));
        }
    }
    for (int i = 0; i < result.size(); i++) {
        auto r = (Vector3)result[i];
        final_result.append(r);
    }
    return final_result;
}

#define DEBUG false

PackedVector3Array GDNavigator::astar(CollisionObject3D* p, Vector3 target, Shape3D* shape, uint64_t options, float search_radius, float margin_from_obs)
{
    auto start = p->get_global_position();
    auto open_container = Dictionary();
    open_container[start] = true;
    auto came_from = Dictionary();
    auto g_score = Dictionary();
    g_score[start] = 0.0;
    auto f_score = Dictionary();
    f_score[start] = start.distance_to(target);
    auto distance = shape_max_bound(shape);
    auto max_look_up = search_radius / distance;

    auto cmp = [&f_score](Vector3 lhs, Vector3 rhs) -> bool { return (float)f_score.get(lhs, INFINITY) > (float)f_score.get(rhs, INFINITY); };
    std::priority_queue<Vector3, std::vector<Vector3>, decltype(cmp)> open(cmp);
    open.push(start);

    auto best_distance = INFINITY;
    auto closest_point = start;
    while (open_container.size() > 0) {
        // auto current = minimum_score(open_container, f_score);
        auto current = open.top();

        auto current_distance = current.distance_to(target);
        if (current_distance < best_distance) {
            best_distance = current_distance;
            closest_point = current;
        }
        if (current_distance <= distance) {
            if (DEBUG)
                UtilityFunctions::print("astar: A");
            return reconstruct_path(came_from, current);
        }

        open.pop();
        open_container.erase(current);
        auto new_points = neighbours(p, current, 8, distance, target, shape, options);
        for (int i = 0; i < new_points.size(); i++) {
            auto n = (Vector3)new_points[i];
            auto tentative = (float)g_score[current] + distance;
            if (tentative < (float)g_score.get(n, INFINITY)) {
                came_from[n] = current;
                g_score[n] = tentative;
                f_score[n] = tentative + n.distance_to(target);
                if (!open_container.has(n)) {
                    open.push(n);
                    open_container[n] = true;
                }
            }
        }

        max_look_up -= 1;
        if (max_look_up <= 0) {
            if (DEBUG)
                UtilityFunctions::print("astar: B");
            return reconstruct_path(came_from, closest_point);
        }
    }

    if (DEBUG)
        UtilityFunctions::print("astar: C");
    auto result = PackedVector3Array();
    result.append(target);
    return result;
}

bool GDNavigator::will_collide(CollisionObject3D* p, Shape3D* shape, Vector3 target, bool exclude_ground)
{
    if (DEBUG)
        UtilityFunctions::print("will_collide: ", target, exclude_ground);
    return get_shape_collides(p, p->get_global_position(), target, shape, exclude_ground);
}

PackedVector3Array GDNavigator::find_target_path(CollisionObject3D* p, Vector3 target, Shape3D* shape, uint64_t options, float search_radius, float margin_from_obs)
{
    auto new_shape = shape_increase(shape, margin_from_obs);
    auto result = PackedVector3Array();
    result.append(target);
    if (p->get_global_position().distance_to(target) > 100.0) {
        if (DEBUG)
            UtilityFunctions::print("find_target_path: A");
        return result;
    }
    if (!will_collide(p, new_shape, target, (options & MovementOptions::UNDERGROUND) != 0)) {
        if (DEBUG)
            UtilityFunctions::print("find_target_path: B");
        return result;
    }

    auto path = astar(p, target, new_shape, options, search_radius, margin_from_obs);
    if (path.is_empty()) {
        if (DEBUG)
            UtilityFunctions::print("find_target_path: C");
        return result;
    }

    if (DEBUG)
        UtilityFunctions::print("find_target_path: D");
    return path;
}

Vector3 GDNavigator::find_target(CollisionObject3D* p, Vector3 target, Shape3D* shape, uint64_t options, float search_radius, float margin_from_obs)
{
    auto path = find_target_path(p, target, shape, options, search_radius, margin_from_obs);
    if (path.is_empty()) {
        return target;
    }
    auto next = (Vector3)path[0];
    float max_distance = shape_max_bound(shape);
    while (!path.is_empty()) {
        auto x = Vector2(p->get_global_position().x, p->get_global_position().z);
        auto y = Vector2(next.x, next.z);
        if (x.distance_squared_to(y) > max_distance * max_distance) {
            break;
        }
        next = path[0];
        path.remove_at(0);
    }
    return next;
}
