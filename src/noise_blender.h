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

#define axial_weight 2.0

namespace godot {

class MyNoise {
public:
    FastNoise::SmartNode<> noise;
    int seed;

    MyNoise(const char* string, int seed);
    MyNoise();
    ~MyNoise();

    float noise2d(float x, float y);
    void noise2d(float* data, float x, float y, float w, float h, float frequency);
    void noise2d_inset(float* data, float x, float y, float w, float h, float frequency, int stride);
};

class GDNoiseBlender : public RefCounted {
    GDCLASS(GDNoiseBlender, RefCounted)

private:
protected:
    static void _bind_methods();

    float get_noise_2d(float x, float y);

    std::vector<float> distances_map;
    std::vector<float> biome_noise_x_map;
    std::vector<float> biome_noise_y_map;
    std::vector<float> biome_noise_z_map;
    std::vector<float> biome_noise_w_map;
    std::vector<float> total_distances_map;
    std::vector<float> min_distances_map;
    std::vector<Color> colors_map;
    PackedInt32Array biomes_map;

public:
    GDProfiler profile_compute;
    GDProfiler profile_sum_distances;
    GDProfiler profile_curve_sample;
    GDProfiler profile_get_noise;

    GDNoiseBlender();
    ~GDNoiseBlender();

    std::vector<MyNoise> terrains;
    std::vector<Curve*> curves;
    PackedVector2Array locations;
    std::vector<Vector3> colors;
    PackedFloat32Array height_map_store;
    std::vector<size_t> min_distances_index_map;

    MyNoise biome_noise_x;
    MyNoise biome_noise_y;

    int biome;
    Color color;
    std::vector<double> distances;
    double total_distance;
    double elevation_mix_exp;

    Color get_color();
    PackedFloat64Array get_distances();
    double get_total_distance();
    int get_biome();
    PackedInt32Array get_biomes_map();

    void set_biome_noise(String encoded, int seed, int axis);
    void set_elevation_mix_exp(double value);

    void add_biome(String terrain, int seed, Curve* curve, Vector2 location, Vector3 color);

    void compute_biome_stats(double x, double y, double scale);
    void compute_biome_map_stats(double x, double y, double w, double h, double scale);
    double height(double x, double y, double scale);
    PackedFloat32Array height_map(double x, double y, double w, double h, double scale);

    NoiseTexture2D* texture(FastNoiseLite* noise, double x, double y, double w, double h, double scale);
    ImageTexture* fast_texture(MyNoise noise, double x, double y, double w, double h, double scale);
    ImageTexture* biome_texture(double x, double y, double w, double h, double scale, int axis);
    ImageTexture* height_texture(PackedFloat32Array data, float w, float h);
    PackedVector2Array get_locations();

    double grass_height(int biome, double x, double y);
};

}

#endif