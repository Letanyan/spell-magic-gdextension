#ifndef GDFAST_NOISE_2_H
#define GDFAST_NOISE_2_H

#include <FastNoise/FastNoise.h>
#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/classes/resource.hpp>

using namespace godot;
using namespace FastNoise;

class FN : public godot::Resource {
    GDCLASS(FN, godot::Resource)
protected:
    static void _bind_methods();

    SmartNode<Generator> node;
    float value;
    FN(SmartNode<Generator> node);
    FN(float value);

public:
    FN();
    ~FN();
    float single_noise2d(float x, float y, int seed);
    float single_noise3d(float x, float y, float z, int seed);
    float single_noise4d(float x, float y, float z, float w, int seed);
    PackedFloat32Array noise2d(float x, float y, float width, float height, int seed);
    PackedFloat32Array noise3d(float x, float y, float z, float width, float height, float depth, int seed);
    PackedFloat32Array noise4d(float x, float y, float z, float w, float width, float height, float depth, float duration, int seed);

    void number(float value);
    void basic_constant(float value);
    void basic_white();
    void basic_checkerboard(float size);
    void basic_sine_wave(float scale);

    void source_simplex();
    void source_open_simplex2();
    void source_open_simplex2s();
    void source_value();
    void source_perlin();

    void add(Ref<FN> lhs, Ref<FN> rhs);
    void subtract(Ref<FN> lhs, Ref<FN> rhs);
    void multiply(Ref<FN> lhs, Ref<FN> rhs);
    void divide(Ref<FN> lhs, Ref<FN> rhs);
    void min(Ref<FN> lhs, Ref<FN> rhs);
    void max(Ref<FN> lhs, Ref<FN> rhs);
    void pow_float(Ref<FN> value, Ref<FN> pow);
    void pow_int(Ref<FN> value, Ref<FN> pow);
    void min_smooth(Ref<FN> lhs, Ref<FN> rhs, Ref<FN> smoothness);
    void max_smooth(Ref<FN> lhs, Ref<FN> rhs, Ref<FN> smoothness);
    void fade(Ref<FN> a, Ref<FN> b, Ref<FN> fade);
};

#endif