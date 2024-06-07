#include "fast_noise_2.h"
#include <godot_cpp/variant/utility_functions.hpp>

using namespace FastNoise;

FN::FN(Generator* generator)
    : value(0.0)
{
    this->generator = generator;
}

FN::FN(SmartNode<Generator> node)
    : value(0.0)
{
    this->generator = node.get();
}

FN::FN(float value)
    : generator(nullptr)
    , value(value)
{
}

FN::FN()
    : generator(nullptr)
    , value(0.0)
{
    UtilityFunctions::push_error("create of null FN node is not allowed");
}

FN::~FN()
{
    UtilityFunctions::print("destruct");
}

SmartNode<Generator> FN::node()
{
    auto result = FastNoise::SmartNode();
    result.reset(generator);
    return result;
}

void FN::_bind_methods()
{
    godot::ClassDB::bind_method(D_METHOD("single_noise2d", "x", "y", "seed"), &FN::single_noise2d);
    godot::ClassDB::bind_method(D_METHOD("single_noise3d", "x", "y", "z", "seed"), &FN::single_noise3d);
    godot::ClassDB::bind_method(D_METHOD("single_noise4d", "x", "y", "z", "w", "seed"), &FN::single_noise4d);
    godot::ClassDB::bind_method(D_METHOD("noise2d", "x", "y", "width", "height", "seed"), &FN::noise2d);
    godot::ClassDB::bind_method(D_METHOD("noise3d", "x", "y", "z", "width", "height", "depth", "seed"), &FN::noise3d);
    godot::ClassDB::bind_method(D_METHOD("noise4d", "x", "y", "z", "w", "width", "height", "depth", "duration"
                                                                                                    "seed"),
        &FN::noise4d);

    godot::ClassDB::bind_static_method("FN", D_METHOD("number", "value"), &FN::number);
    godot::ClassDB::bind_static_method("FN", D_METHOD("simplex"), &FN::simplex);

    godot::ClassDB::bind_static_method("FN", D_METHOD("add", "lhs", "rhs"), &FN::add);
    godot::ClassDB::bind_static_method("FN", D_METHOD("subtract", "lhs", "rhs"), &FN::subtract);
    godot::ClassDB::bind_static_method("FN", D_METHOD("multiply", "lhs", "rhs"), &FN::multiply);
    godot::ClassDB::bind_static_method("FN", D_METHOD("divide", "lhs", "rhs"), &FN::divide);
    godot::ClassDB::bind_static_method("FN", D_METHOD("min", "lhs", "rhs"), &FN::min);
    godot::ClassDB::bind_static_method("FN", D_METHOD("max", "lhs", "rhs"), &FN::max);
    godot::ClassDB::bind_static_method("FN", D_METHOD("pow_float", "value", "pow"), &FN::pow_float);
    godot::ClassDB::bind_static_method("FN", D_METHOD("pow_int", "value", "pow"), &FN::pow_int);
    godot::ClassDB::bind_static_method("FN", D_METHOD("min_smooth", "lhs", "rhs", "smoothness"), &FN::min_smooth);
    godot::ClassDB::bind_static_method("FN", D_METHOD("max_smooth", "lhs", "rhs", "smoothness"), &FN::max_smooth);
    godot::ClassDB::bind_static_method("FN", D_METHOD("fade", "a", "b", "fade"), &FN::fade);
}

float FN::single_noise2d(float x, float y, int seed)
{
    auto result = node()->GenSingle2D(x, y, seed);
    return result;
}
float FN::single_noise3d(float x, float y, float z, int seed)
{
    return node()->GenSingle3D(x, y, z, seed);
}
float FN::single_noise4d(float x, float y, float z, float w, int seed)
{
    return node()->GenSingle4D(x, y, z, w, seed);
}
PackedFloat32Array FN::noise2d(float x, float y, float width, float height, int seed)
{
    auto data = std::vector<float>();
    data.resize(width * height);
    node()->GenUniformGrid2D(data.data(), x, y, width, height, 1.0, seed);
    auto result = godot::PackedFloat32Array();
    for (auto& v : data) {
        result.push_back(v);
    }
    return result;
}
PackedFloat32Array FN::noise3d(float x, float y, float z, float width, float height, float depth, int seed)
{
    auto data = std::vector<float>();
    data.resize(width * height * depth);
    node()->GenUniformGrid3D(data.data(), x, y, z, width, height, depth, 1.0, seed);
    auto result = godot::PackedFloat32Array();
    for (auto& v : data) {
        result.push_back(v);
    }
    return result;
}
PackedFloat32Array FN::noise4d(float x, float y, float z, float w, float width, float height, float depth, float duration, int seed)
{
    auto data = std::vector<float>();
    data.resize(width * height * depth * duration);
    node()->GenUniformGrid4D(data.data(), x, y, z, w, width, height, depth, duration, 1.0, seed);
    auto result = godot::PackedFloat32Array();
    for (auto& v : data) {
        result.push_back(v);
    }
    return result;
}

FN* FN::number(float value)
{
    UtilityFunctions::print("number");
    return new FN(value);
}

FN* FN::simplex()
{
    auto result = new FN(FastNoise::New<Simplex>());
    return result;
}

FN* FN::add(FN* lhs, FN* rhs)
{
    auto node = SmartNode<Add>();
    node->SetLHS(lhs->node());
    rhs->generator == nullptr ? node->SetRHS(rhs->value) : node->SetRHS(rhs->node());
    return new FN(node);
}

FN* FN::subtract(FN* lhs, FN* rhs)
{
    auto node = SmartNode<Subtract>();
    lhs->generator == nullptr ? node->SetLHS(lhs->value) : node->SetLHS(lhs->node());
    rhs->generator == nullptr ? node->SetRHS(rhs->value) : node->SetRHS(rhs->node());
    return new FN(node);
}

FN* FN::multiply(FN* lhs, FN* rhs)
{
    auto node = SmartNode<Multiply>();
    node->SetLHS(lhs->node());
    rhs->generator == nullptr ? node->SetRHS(rhs->value) : node->SetRHS(rhs->node());
    return new FN(node);
}

FN* FN::divide(FN* lhs, FN* rhs)
{
    auto node = SmartNode<Divide>();
    lhs->generator == nullptr ? node->SetLHS(lhs->value) : node->SetLHS(lhs->node());
    rhs->generator == nullptr ? node->SetRHS(rhs->value) : node->SetRHS(rhs->node());
    return new FN(node);
}

FN* FN::min(FN* lhs, FN* rhs)
{
    auto node = SmartNode<Min>();
    node->SetLHS(lhs->node());
    rhs->generator == nullptr ? node->SetRHS(rhs->value) : node->SetRHS(rhs->node());
    return new FN(node);
}

FN* FN::max(FN* lhs, FN* rhs)
{
    auto node = SmartNode<Max>();
    node->SetLHS(lhs->node());
    rhs->generator == nullptr ? node->SetRHS(rhs->value) : node->SetRHS(rhs->node());
    return new FN(node);
}

FN* FN::pow_float(FN* value, FN* pow)
{
    auto node = SmartNode<PowFloat>();
    if (value->generator == nullptr) {
        node->SetValue(value->value);
    } else {
        node->SetValue(value->node());
    }
    if (pow->generator == nullptr) {
        node->SetPow(pow->value);
    } else {
        node->SetPow(pow->node());
    }
    return new FN(node);
}

/// @brief
/// @param value must be source
/// @param pow must be int
/// @return
FN* FN::pow_int(FN* value, FN* pow)
{
    auto node = SmartNode<PowInt>();
    node->SetValue(value->node());
    node->SetPow(pow->value);
    return new FN(node);
}

FN* FN::min_smooth(FN* lhs, FN* rhs, FN* smooth)
{
    auto node = SmartNode<MinSmooth>();
    node->SetLHS(lhs->node());
    rhs->generator == nullptr ? node->SetRHS(rhs->value) : node->SetRHS(rhs->node());
    if (smooth->generator == nullptr) {
        node->SetSmoothness(smooth->value);
    } else {
        node->SetSmoothness(smooth->node());
    }
    return new FN(node);
}

FN* FN::max_smooth(FN* lhs, FN* rhs, FN* smooth)
{
    auto node = SmartNode<MaxSmooth>();
    node->SetLHS(lhs->node());
    rhs->generator == nullptr ? node->SetRHS(rhs->value) : node->SetRHS(rhs->node());
    if (smooth->generator == nullptr) {
        node->SetSmoothness(smooth->value);
    } else {
        node->SetSmoothness(smooth->node());
    }
    return new FN(node);
}

FN* FN::fade(FN* a, FN* b, FN* fade)
{
    auto node = SmartNode<Fade>();
    node->SetA(a->node());
    node->SetB(b->node());
    if (fade->generator == nullptr) {
        node->SetFade(fade->value);
    } else {
        node->SetFade(fade->node());
    }
    return new FN(node);
}