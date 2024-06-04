#ifndef GDNOISE_BLENDER_H
#define GDNOISE_BLENDER_H

#include "my_profiler.h"
#include <FastNoise/FastNoise.h>
#include <godot_cpp/classes/curve.hpp>
#include <godot_cpp/classes/fast_noise_lite.hpp>
#include <godot_cpp/classes/image_texture.hpp>
#include <godot_cpp/classes/noise_texture2d.hpp>

#include <functional>
#include <string>
#include <vector>

// namespace FastNoise {
// class Generator;
// template <typename T>
// class SmartNode;
// }

namespace godot {

class MyNoise {
public:
    FastNoise::SmartNode<> noise;
    float frequency;
    int seed;

    MyNoise(const char* string, double freq, int seed);
    MyNoise();
    ~MyNoise();
};

class GDNoiseBlender : public Object {
    GDCLASS(GDNoiseBlender, Object)

private:
protected:
    static void _bind_methods();

    void add_noise_terrain(const char* string, float freq, int seed);
    float get_noise_2d(float x, float y);

    MyNoise fast_dryness;
    MyNoise fast_temperature;

    ImageTexture* fast_texture(MyNoise noise, double x, double y, double w, double h, double scale);

public:
    GDProfiler profile_compute;
    GDProfiler profile_sum_distances;
    GDProfiler profile_curve_sample;
    GDProfiler profile_get_noise;

    GDNoiseBlender();
    ~GDNoiseBlender();

    std::vector<FastNoiseLite*> terrains;
    std::vector<std::tuple<FastNoise::SmartNode<FastNoise::Generator>, double, int>> _terrains;
    std::vector<Curve*> curves;
    std::vector<Vector2> locations;
    std::vector<Vector3> colors;

    FastNoiseLite* dryness;
    FastNoiseLite* temperature;

    int biome;
    Color color;
    std::vector<double> distances;
    double total_distance;

    Color get_color();
    PackedFloat64Array get_distances();
    double get_total_distance();
    int get_biome();

    void set_temperature(FastNoiseLite* _temperature);
    void set_dryness(FastNoiseLite* _dryness);

    void add_biome(FastNoiseLite* terrain, Curve* curve, Vector2 location, Vector3 color);

    void compute_biome_stats(double x, double y, bool logging = false);
    double height(double x, double y);

    NoiseTexture2D* texture(FastNoiseLite* noise, double x, double y, double w, double h, double scale);
    ImageTexture* dryness_texture(double x, double y, double w, double h, double scale);
    ImageTexture* temperature_texture(double x, double y, double w, double h, double scale);

    double grass_height(int biome, double x, double y);
};

}

#endif