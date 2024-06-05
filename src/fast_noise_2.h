#ifndef GDFAST_NOISE_2_H
#define GDFAST_NOISE_2_H

#include <FastNoise/FastNoise.h>
#include <godot_cpp/classes/resource.hpp>

using namespace godot;

class FastNoiseObject : public Resource {
    GDCLASS(FastNoiseObject, Resource)
protected:
    static void _bind_methods();

public:
    FastNoiseObject() = default;
    ~FastNoiseObject() = default;
};

class FastNoiseNode : public FastNoiseObject {
    GDCLASS(FastNoiseNode, FastNoiseObject)
protected:
    static void _bind_methods();

public:
    FastNoise::SmartNode<> node;
    FastNoiseNode() = default;
    ~FastNoiseNode() = default;
    float single_noise2d(float x, float y, int seed);
    float single_noise3d(float x, float y, float z, int seed);
    float single_noise4d(float x, float y, float z, float w, int seed);
    PackedFloat32Array noise2d(float x, float y, float width, float height, int seed);
    PackedFloat32Array noise3d(float x, float y, float z, float width, float height, float depth, int seed);
    PackedFloat32Array noise4d(float x, float y, float z, float w, float width, float height, float depth, float duration, int seed);
};

class FastNoiseValue : public FastNoiseObject {
    GDCLASS(FastNoiseValue, FastNoiseObject)
protected:
    static void _bind_methods();

public:
    float value;
    FastNoiseValue() = default;
    ~FastNoiseValue() = default;
    void set_value(float value);
};

class FNSimplex : public FastNoiseNode {
    GDCLASS(FNSimplex, FastNoiseNode)
protected:
    static void _bind_methods();

public:
    FastNoise::SmartNode<FastNoise::Simplex> node;
    FNSimplex() = default;
    ~FNSimplex() = default;
};

class FNAdd : public FastNoiseNode {
    GDCLASS(FNAdd, FastNoiseNode)
protected:
    static void _bind_methods();

public:
    FastNoise::SmartNode<FastNoise::Add> node;
    FNAdd() = default;
    ~FNAdd() = default;
    void set_lhs(FastNoiseNode* node);
    void set_rhs(FastNoiseObject* obj);
};

class FNSubtract : public FastNoiseNode {
    GDCLASS(FNSubtract, FastNoiseNode)
protected:
    static void _bind_methods();

public:
    FastNoise::SmartNode<FastNoise::Subtract> node;
    FNSubtract() = default;
    ~FNSubtract() = default;
    void set_lhs(FastNoiseNode* node);
    void set_rhs(FastNoiseObject* obj);
};

#endif