#ifndef GDFAST_NOISE_2_H
#define GDFAST_NOISE_2_H

#include <FastNoise/FastNoise.h>
#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/classes/resource.hpp>

using namespace godot;
using namespace FastNoise;

class FN : public godot::RefCounted {
    GDCLASS(FN, godot::RefCounted)
protected:
    static void _bind_methods();

    Generator* generator;
    float value;
    FN(Generator* generator);
    FN(SmartNode<Generator> node);
    FN(float value);

    SmartNode<Generator> node();

public:
    FN();
    ~FN();
    float single_noise2d(float x, float y, int seed);
    float single_noise3d(float x, float y, float z, int seed);
    float single_noise4d(float x, float y, float z, float w, int seed);
    PackedFloat32Array noise2d(float x, float y, float width, float height, int seed);
    PackedFloat32Array noise3d(float x, float y, float z, float width, float height, float depth, int seed);
    PackedFloat32Array noise4d(float x, float y, float z, float w, float width, float height, float depth, float duration, int seed);

    static FN* number(float value);
    static FN* simplex();

    static FN* add(FN* lhs, FN* rhs);
    static FN* subtract(FN* lhs, FN* rhs);
    static FN* multiply(FN* lhs, FN* rhs);
    static FN* divide(FN* lhs, FN* rhs);
    static FN* min(FN* lhs, FN* rhs);
    static FN* max(FN* lhs, FN* rhs);
    static FN* pow_float(FN* value, FN* pow);
    static FN* pow_int(FN* value, FN* pow);
    static FN* min_smooth(FN* lhs, FN* rhs, FN* smoothness);
    static FN* max_smooth(FN* lhs, FN* rhs, FN* smoothness);
    static FN* fade(FN* a, FN* b, FN* fade);
};

#endif