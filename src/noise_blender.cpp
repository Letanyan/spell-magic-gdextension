#include "noise_blender.h"
#include <iostream>
#include <math.h>

#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

void GDNoiseBlender::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("set_dryness", "dryness"), &GDNoiseBlender::set_dryness);
    ClassDB::bind_method(D_METHOD("set_temperature", "temperature"), &GDNoiseBlender::set_temperature);
    ClassDB::bind_method(D_METHOD("add_biome", "terrain", "curve", "location", "color"), &GDNoiseBlender::add_biome);
    ClassDB::bind_method(D_METHOD("height", "x", "y"), &GDNoiseBlender::height);
    ClassDB::bind_method(D_METHOD("compute_biome_stats", "x", "y"), &GDNoiseBlender::compute_biome_stats);
    ClassDB::bind_method(D_METHOD("get_total_distance"), &GDNoiseBlender::get_total_distance);
    ClassDB::bind_method(D_METHOD("get_biome"), &GDNoiseBlender::get_biome);
    ClassDB::bind_method(D_METHOD("get_color"), &GDNoiseBlender::get_color);
    ClassDB::bind_method(D_METHOD("get_distances"), &GDNoiseBlender::get_distances);
}

GDNoiseBlender::GDNoiseBlender()
{
    terrains = std::vector<Variant>();
    curves = std::vector<Variant>();
    locations = std::vector<Vector2>();
    colors = std::vector<Vector3>();
    distances = std::vector<double>();
}

GDNoiseBlender::~GDNoiseBlender()
{
    // Add your cleanup here.
}

Color GDNoiseBlender::get_color()
{
    return color;
}

PackedFloat64Array GDNoiseBlender::get_distances()
{
    auto result = PackedFloat64Array();
    for (auto& d : distances) {
        result.push_back(d);
    }
    return result;
}

double GDNoiseBlender::get_total_distance()
{
    return total_distance;
}

int GDNoiseBlender::get_biome()
{
    return biome;
}

void GDNoiseBlender::set_temperature(Variant _temperature)
{
    temperature = _temperature;
}

void GDNoiseBlender::set_dryness(Variant _dryness)
{
    dryness = _dryness;
}

void GDNoiseBlender::add_biome(Variant terrain, Variant curve, Vector2 location, Vector3 color)
{
    terrains.push_back(terrain);
    curves.push_back(curve);
    locations.push_back(location);
    colors.push_back(color);
    distances.push_back(0.0);
}

void GDNoiseBlender::compute_biome_stats(double x, double y)
{
    auto d = ((FastNoiseLite*)(Object*)dryness)->get_noise_2d(x, y) / 2.0 + 0.5;
    auto t = ((FastNoiseLite*)(Object*)temperature)->get_noise_2d(x, y) / 2.0 + 0.5;

    auto p = Vector2(d, t);
    auto min_distance = INFINITY;
    auto pos = 0;
    total_distance = 0.0;
    auto clr = Vector3(1, 1, 1);
    auto dist = 0.0;
    auto c = Vector3(0, 0, 0);
    for (size_t i = 0; i < locations.size(); i++) {
        dist = p.distance_to(locations[i]);
        distances[i] = dist;
        total_distance += dist;
        c = colors[i].lerp(Vector3(1, 1, 1), dist);
        if (dist <= 1.0) {
            clr = clr * c;
        }
        if (dist < min_distance) {
            min_distance = dist;
            pos = i;
        }
    }
    biome = pos;
    color = Color(clr.x, clr.y, clr.z);
}

double GDNoiseBlender::height(double x, double y)
{
    double result = 0.0;
    compute_biome_stats(x, y);

    double X = UtilityFunctions::snappedf(x, 0.0001);
    double Y = UtilityFunctions::snappedf(y, 0.0001);

    for (int i = 0; i < distances.size(); i++) {
        double e = ((FastNoiseLite*)(Object*)terrains[i])->get_noise_2d(X, Y) / 2.0 + 0.5;
        e = ((Curve*)(Object*)curves[i])->sample(e);
        double m = 1.0 - distances[i] / total_distance;
        result += e * m;
    }

    return result;
}