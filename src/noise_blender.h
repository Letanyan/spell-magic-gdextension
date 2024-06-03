#ifndef GDNOISE_BLENDER_H
#define GDNOISE_BLENDER_H

#include <godot_cpp/classes/curve.hpp>
#include <godot_cpp/classes/fast_noise_lite.hpp>
#include <godot_cpp/classes/noise_texture2d.hpp>

#include <functional>
#include <string>
#include <vector>

namespace godot {

class GDNoiseBlender : public Object {
    GDCLASS(GDNoiseBlender, Object)

private:
protected:
    static void _bind_methods();

public:
    GDNoiseBlender();
    ~GDNoiseBlender();

    std::vector<FastNoiseLite*> terrains;
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

    void compute_biome_stats(double x, double y);
    double height(double x, double y);

    NoiseTexture2D* texture(FastNoiseLite* noise, double x, double y, double w, double h, double scale);
    NoiseTexture2D* dryness_texture(double x, double y, double w, double h, double scale);
    NoiseTexture2D* temperature_texture(double x, double y, double w, double h, double scale);

    double grass_height(int biome, double x, double y);
};

}

#endif