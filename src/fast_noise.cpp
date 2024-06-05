#include "fast_noise_2.h"
#include <godot_cpp\classes\physics_shape_query_parameters3d.hpp>

void FastNoiseObject::_bind_methods()
{
}

void FastNoiseNode::_bind_methods()
{
    godot::ClassDB::bind_method(D_METHOD("single_noise2d", "x", "y", "seed"), &FastNoiseNode::single_noise2d);
    godot::ClassDB::bind_method(D_METHOD("single_noise3d", "x", "y", "z", "seed"), &FastNoiseNode::single_noise3d);
    godot::ClassDB::bind_method(D_METHOD("single_noise4d", "x", "y", "z", "w", "seed"), &FastNoiseNode::single_noise4d);
    godot::ClassDB::bind_method(D_METHOD("noise2d", "x", "y", "width", "height", "seed"), &FastNoiseNode::noise2d);
    godot::ClassDB::bind_method(D_METHOD("noise3d", "x", "y", "z", "width", "height", "depth", "seed"), &FastNoiseNode::noise3d);
    godot::ClassDB::bind_method(D_METHOD("noise4d", "x", "y", "z", "w", "width", "height", "depth", "duration"
                                                                                                    "seed"),
        &FastNoiseNode::noise4d);
}

float FastNoiseNode::single_noise2d(float x, float y, int seed)
{
    return node->GenSingle2D(x, y, seed);
}
float FastNoiseNode::single_noise3d(float x, float y, float z, int seed)
{
    return node->GenSingle3D(x, y, z, seed);
}
float FastNoiseNode::single_noise4d(float x, float y, float z, float w, int seed)
{
    return node->GenSingle4D(x, y, z, w, seed);
}
PackedFloat32Array FastNoiseNode::noise2d(float x, float y, float width, float height, int seed)
{
    auto data = std::vector<float>();
    data.resize(width * height);
    node->GenUniformGrid2D(data.data(), x, y, width, height, 1.0, seed);
    auto result = godot::PackedFloat32Array();
    for (auto& v : data) {
        result.push_back(v);
    }
    return result;
}
PackedFloat32Array FastNoiseNode::noise3d(float x, float y, float z, float width, float height, float depth, int seed)
{
    auto data = std::vector<float>();
    data.resize(width * height * depth);
    node->GenUniformGrid3D(data.data(), x, y, z, width, height, depth, 1.0, seed);
    auto result = godot::PackedFloat32Array();
    for (auto& v : data) {
        result.push_back(v);
    }
    return result;
}
PackedFloat32Array FastNoiseNode::noise4d(float x, float y, float z, float w, float width, float height, float depth, float duration, int seed)
{
    auto data = std::vector<float>();
    data.resize(width * height * depth * duration);
    node->GenUniformGrid4D(data.data(), x, y, z, w, width, height, depth, duration, 1.0, seed);
    auto result = godot::PackedFloat32Array();
    for (auto& v : data) {
        result.push_back(v);
    }
    return result;
}

void FastNoiseValue::_bind_methods()
{
    godot::ClassDB::bind_method(D_METHOD("set_value", "value"), &FastNoiseValue::set_value);
}

void FastNoiseValue::set_value(float value)
{
    this->value = value;
}

void FNAdd::_bind_methods()
{
    godot::ClassDB::bind_method(D_METHOD("set_lhs", "lhs"), &FNAdd::set_lhs);
    godot::ClassDB::bind_method(D_METHOD("set_rhs", "rhs"), &FNAdd::set_rhs);
}

void FNAdd::set_lhs(FastNoiseNode* lhs)
{
    node->SetLHS(lhs->node);
}

void FNAdd::set_rhs(FastNoiseObject* rhs)
{
    if (dynamic_cast<FastNoiseNode*>(rhs) != nullptr) {
        node->SetRHS(dynamic_cast<FastNoiseNode*>(rhs)->node);
    } else if (dynamic_cast<FastNoiseValue*>(rhs) != nullptr) {
        node->SetRHS(dynamic_cast<FastNoiseValue*>(rhs)->value);
    }
}

void FNSubtract::_bind_methods()
{
    godot::ClassDB::bind_method(D_METHOD("set_lhs", "lhs"), &FNSubtract::set_lhs);
    godot::ClassDB::bind_method(D_METHOD("set_rhs", "rhs"), &FNSubtract::set_rhs);
}

void FNSubtract::set_lhs(FastNoiseNode* lhs)
{
    node->SetLHS(lhs->node);
}

void FNSubtract::set_rhs(FastNoiseObject* rhs)
{
    if (dynamic_cast<FastNoiseNode*>(rhs) != nullptr) {
        node->SetRHS(dynamic_cast<FastNoiseNode*>(rhs)->node);
    } else if (dynamic_cast<FastNoiseValue*>(rhs) != nullptr) {
        node->SetRHS(dynamic_cast<FastNoiseValue*>(rhs)->value);
    }
}