#include "fast_noise_2.h"
#include <godot_cpp/variant/utility_functions.hpp>

using namespace FastNoise;

FN::FN(SmartNode<Generator> node)
    : value(0.0)
{
    this->node = node;
}

FN::FN(float value)
    : node(nullptr)
    , value(value)
{
}

FN::FN()
    : node(nullptr)
    , value(0.0)
{
}

FN::~FN()
{
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

    godot::ClassDB::bind_method(D_METHOD("number", "value"), &FN::number);

    godot::ClassDB::bind_method(D_METHOD("basic_constant", "value"), &FN::basic_constant);
    godot::ClassDB::bind_method(D_METHOD("basic_white"), &FN::basic_white);
    godot::ClassDB::bind_method(D_METHOD("basic_checkerboard", "size"), &FN::basic_checkerboard);
    godot::ClassDB::bind_method(D_METHOD("basic_sine_wave", "scale"), &FN::basic_sine_wave);

    godot::ClassDB::bind_method(D_METHOD("source_simplex"), &FN::source_simplex);
    godot::ClassDB::bind_method(D_METHOD("source_open_simplex2"), &FN::source_open_simplex2);
    godot::ClassDB::bind_method(D_METHOD("source_open_simplex2s"), &FN::source_open_simplex2s);
    godot::ClassDB::bind_method(D_METHOD("source_value"), &FN::source_value);
    godot::ClassDB::bind_method(D_METHOD("source_perlin"), &FN::source_perlin);

    godot::ClassDB::bind_method(D_METHOD("add", "lhs", "rhs"), &FN::add);
    godot::ClassDB::bind_method(D_METHOD("subtract", "lhs", "rhs"), &FN::subtract);
    godot::ClassDB::bind_method(D_METHOD("multiply", "lhs", "rhs"), &FN::multiply);
    godot::ClassDB::bind_method(D_METHOD("divide", "lhs", "rhs"), &FN::divide);
    godot::ClassDB::bind_method(D_METHOD("min", "lhs", "rhs"), &FN::min);
    godot::ClassDB::bind_method(D_METHOD("max", "lhs", "rhs"), &FN::max);
    godot::ClassDB::bind_method(D_METHOD("pow_float", "value", "pow"), &FN::pow_float);
    godot::ClassDB::bind_method(D_METHOD("pow_int", "value", "pow"), &FN::pow_int);
    godot::ClassDB::bind_method(D_METHOD("min_smooth", "lhs", "rhs", "smoothness"), &FN::min_smooth);
    godot::ClassDB::bind_method(D_METHOD("max_smooth", "lhs", "rhs", "smoothness"), &FN::max_smooth);
    godot::ClassDB::bind_method(D_METHOD("fade", "a", "b", "fade"), &FN::fade);
}

float FN::single_noise2d(float x, float y, int seed)
{
    auto result = node->GenSingle2D(x, y, seed);
    return result;
}
float FN::single_noise3d(float x, float y, float z, int seed)
{
    return node->GenSingle3D(x, y, z, seed);
}
float FN::single_noise4d(float x, float y, float z, float w, int seed)
{
    return node->GenSingle4D(x, y, z, w, seed);
}
PackedFloat32Array FN::noise2d(float x, float y, float width, float height, int seed)
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
PackedFloat32Array FN::noise3d(float x, float y, float z, float width, float height, float depth, int seed)
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
PackedFloat32Array FN::noise4d(float x, float y, float z, float w, float width, float height, float depth, float duration, int seed)
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

void FN::number(float value)
{
    node = nullptr;
    this->value = value;
}

void FN::basic_constant(float value)
{
    auto node = New<Constant>();
    node->SetValue(value);
    this->node = node;
}

void FN::basic_white()
{
    auto node = New<White>();
    this->node = node;
}
void FN::basic_checkerboard(float size)
{
    auto node = New<Checkerboard>();
    node->SetSize(size);
    this->node = node;
}
void FN::basic_sine_wave(float scale)
{
    auto node = New<SineWave>();
    node->SetScale(scale);
    this->node = node;
}

void FN::source_simplex()
{
    node = New<Simplex>();
}
void FN::source_open_simplex2()
{
    node = New<OpenSimplex2>();
}
void FN::source_open_simplex2s()
{
    node = New<OpenSimplex2S>();
}
void FN::source_value()
{
    node = New<Value>();
}
void FN::source_perlin()
{
    node = New<Perlin>();
}

void FN::add(Ref<FN> lhs, Ref<FN> rhs)
{
    auto node = New<Add>();
    node->SetLHS(lhs->node);
    rhs->node ? node->SetRHS(rhs->node) : node->SetRHS(rhs->value);
    this->node = node;
}

void FN::subtract(Ref<FN> lhs, Ref<FN> rhs)
{
    auto node = New<Subtract>();
    lhs->node ? node->SetLHS(lhs->node) : node->SetLHS(lhs->value);
    rhs->node ? node->SetRHS(rhs->node) : node->SetRHS(rhs->value);
    this->node = node;
}

void FN::multiply(Ref<FN> lhs, Ref<FN> rhs)
{
    auto node = New<Multiply>();
    node->SetLHS(lhs->node);
    rhs->node ? node->SetRHS(rhs->node) : node->SetRHS(rhs->value);
    this->node = node;
}

void FN::divide(Ref<FN> lhs, Ref<FN> rhs)
{
    auto node = New<Divide>();
    lhs->node ? node->SetLHS(lhs->node) : node->SetLHS(lhs->value);
    rhs->node ? node->SetRHS(rhs->node) : node->SetRHS(rhs->value);
    this->node = node;
}

void FN::min(Ref<FN> lhs, Ref<FN> rhs)
{
    auto node = New<Min>();
    node->SetLHS(lhs->node);
    rhs->node ? node->SetRHS(rhs->node) : node->SetRHS(rhs->value);
    this->node = node;
}

void FN::max(Ref<FN> lhs, Ref<FN> rhs)
{
    auto node = New<Max>();
    node->SetLHS(lhs->node);
    rhs->node ? node->SetRHS(rhs->node) : node->SetRHS(rhs->value);
    this->node = node;
}

void FN::pow_float(Ref<FN> value, Ref<FN> pow)
{
    auto node = New<PowFloat>();
    value->node ? node->SetValue(value->node) : node->SetValue(value->value);
    pow->node ? node->SetPow(pow->node) : node->SetValue(pow->value);
    this->node = node;
}

/// @brief
/// @param value must be source
/// @param pow must be int
/// @return
void FN::pow_int(Ref<FN> value, Ref<FN> pow)
{
    auto node = New<PowInt>();
    node->SetValue(value->node);
    node->SetPow(pow->value);
    this->node = node;
}

void FN::min_smooth(Ref<FN> lhs, Ref<FN> rhs, Ref<FN> smooth)
{
    auto node = New<MinSmooth>();
    node->SetLHS(lhs->node);
    rhs->node ? node->SetRHS(rhs->node) : node->SetRHS(rhs->value);
    smooth->node ? node->SetSmoothness(smooth->node) : node->SetSmoothness(smooth->value);
    this->node = node;
}

void FN::max_smooth(Ref<FN> lhs, Ref<FN> rhs, Ref<FN> smooth)
{
    auto node = New<MaxSmooth>();
    node->SetLHS(lhs->node);
    rhs->node ? node->SetRHS(rhs->value) : node->SetRHS(rhs->node);
    smooth->node ? node->SetSmoothness(smooth->node) : node->SetSmoothness(smooth->node);
    this->node = node;
}

void FN::fade(Ref<FN> a, Ref<FN> b, Ref<FN> fade)
{
    auto node = New<Fade>();
    node->SetA(a->node);
    node->SetB(b->node);
    fade->node ? node->SetFade(fade->node) : node->SetFade(fade->value);
    this->node = node;
}