#ifndef GDNOISE_BLENDER_H
#define GDNOISE_BLENDER_H

#include <godot_cpp/classes/curve.hpp>
#include <godot_cpp/classes/fast_noise_lite.hpp>

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

    std::vector<Variant> terrains;
    std::vector<Variant> curves;
    std::vector<Vector2> locations;
    std::vector<Vector3> colors;

    Variant dryness;
    Variant temperature;

    int biome;
    Color color;
    std::vector<double> distances;
    double total_distance;

    Color get_color();
    PackedFloat64Array get_distances();
    double get_total_distance();
    int get_biome();

    void set_temperature(Variant _temperature);
    void set_dryness(Variant _dryness);

    void add_biome(Variant terrain, Variant curve, Vector2 location, Vector3 color);

    void compute_biome_stats(double x, double y);
    double height(double x, double y);
};

}

#endif