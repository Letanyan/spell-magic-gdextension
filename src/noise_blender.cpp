#include "noise_blender.h"
#include <iostream>
#include <math.h>

#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

MyNoise::MyNoise(const char* string, double freq, int seed)
{
    noise = FastNoise::NewFromEncodedNodeTree(string);
    frequency = freq;
    this->seed = seed;
}

MyNoise::MyNoise()
{
}

MyNoise::~MyNoise()
{
}

void GDNoiseBlender::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("set_dryness", "dryness"), &GDNoiseBlender::set_dryness);
    ClassDB::bind_method(D_METHOD("set_temperature", "temperature"), &GDNoiseBlender::set_temperature);
    ClassDB::bind_method(D_METHOD("add_biome", "terrain", "curve", "location", "color"), &GDNoiseBlender::add_biome);
    ClassDB::bind_method(D_METHOD("height", "x", "y"), &GDNoiseBlender::height);
    ClassDB::bind_method(D_METHOD("compute_biome_stats", "x", "y", "logging"), &GDNoiseBlender::compute_biome_stats, DEFVAL(false));
    ClassDB::bind_method(D_METHOD("get_total_distance"), &GDNoiseBlender::get_total_distance);
    ClassDB::bind_method(D_METHOD("get_biome"), &GDNoiseBlender::get_biome);
    ClassDB::bind_method(D_METHOD("get_color"), &GDNoiseBlender::get_color);
    ClassDB::bind_method(D_METHOD("get_distances"), &GDNoiseBlender::get_distances);

    ClassDB::bind_method(D_METHOD("texture", "noise", "x", "y", "w", "h"), &GDNoiseBlender::texture);
    ClassDB::bind_method(D_METHOD("dryness_texture", "x", "y", "w", "h"), &GDNoiseBlender::dryness_texture);
    ClassDB::bind_method(D_METHOD("temperature_texture", "x", "y", "w", "h"), &GDNoiseBlender::temperature_texture);
    ClassDB::bind_method(D_METHOD("grass_height", "biome", "x", "y"), &GDNoiseBlender::grass_height);
}

GDNoiseBlender::GDNoiseBlender()
{
    terrains = std::vector<FastNoiseLite*>();
    curves = std::vector<Curve*>();
    locations = std::vector<Vector2>();
    colors = std::vector<Vector3>();
    distances = std::vector<double>();

    fast_dryness = MyNoise("EwDNzMw9EwBvEoM6DwADAAAAAAAAQAgAAAAAAD8AAAAAAA==", 1.0, 0);
    fast_temperature = MyNoise("EwDNzMw9EwBvEoM6DQAGAAAAAAAAQAgAAAAAAD8AAAAAAA==", 1.0, 32);

    _terrains = std::vector<std::tuple<FastNoise::SmartNode<>, double, int>>();
    add_noise_terrain("EQAFAAAAAAAAQBAAzcxMPQ0ABQAAAAAAEEEGAAAAAAAAAAAAgD8BCAAAzczMPQAAAAAA", 0.1, 0); // desert
}

GDNoiseBlender::~GDNoiseBlender()
{
    // Add your cleanup here.
}

void GDNoiseBlender::add_noise_terrain(const char* string, float freq, int seed)
{
    auto n = FastNoise::NewFromEncodedNodeTree(string);
    _terrains.push_back(std::tuple<FastNoise::SmartNode<>, double, int>(n, freq, seed));
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

void GDNoiseBlender::set_temperature(FastNoiseLite* _temperature)
{
    temperature = _temperature;
}

void GDNoiseBlender::set_dryness(FastNoiseLite* _dryness)
{
    dryness = _dryness;
}

NoiseTexture2D* GDNoiseBlender::texture(FastNoiseLite* noise, double x, double y, double w, double h, double scale)
{
    auto result = new NoiseTexture2D();
    auto res_noise = (Ref<FastNoiseLite>)(noise->duplicate(true));
    res_noise->set_frequency(res_noise->get_frequency() * scale);
    res_noise->set_offset(Vector3(x - w / 2.0, y - h / 2.0, 0));
    result->set_noise(res_noise);
    result->set_width(w + 2);
    result->set_height(h + 2);
    result->set_normalize(false);
    return result;
}

ImageTexture* GDNoiseBlender::fast_texture(MyNoise noise, double x, double y, double w, double h, double scale)
{
    auto floats = std::vector<float>();
    float X = x - w / 2.0;
    float Y = y - h / 2.0;
    float W = w + 2;
    float H = h + 2;
    floats.resize(W * H);
    auto minmax = noise.noise->GenUniformGrid2D(floats.data(), X, Y, W, H, noise.frequency * scale, noise.seed);

    auto bytes = PackedByteArray();
    for (int i = 0; i < floats.size(); i++) {
        bytes.append((uint8_t)((floats[i] * 0.5 + 0.5) * 255));
    }

    auto result = Image::create_from_data(W, H, false, Image::Format::FORMAT_L8, bytes);
    auto res = new ImageTexture();
    res->set_image(result);
    return res;
}

ImageTexture* GDNoiseBlender::dryness_texture(double x, double y, double w, double h, double scale)
{
    return fast_texture(this->fast_dryness, x, y, w, h, scale);
}

ImageTexture* GDNoiseBlender::temperature_texture(double x, double y, double w, double h, double scale)
{
    return fast_texture(this->fast_temperature, x, y, w, h, scale);
}

void GDNoiseBlender::add_biome(FastNoiseLite* terrain, Curve* curve, Vector2 location, Vector3 color)
{
    terrains.push_back(terrain);
    curves.push_back(curve);
    locations.push_back(location);
    colors.push_back(color);
    distances.push_back(0.0);
}

void GDNoiseBlender::compute_biome_stats(double x, double y, bool logging)
{
    double X = UtilityFunctions::snappedf(x, 0.0001);
    double Y = UtilityFunctions::snappedf(y, 0.0001);
    // auto d = ((FastNoiseLite*)(Object*)dryness)->get_noise_2d(x, y) / 2.0 + 0.5;
    // auto t = ((FastNoiseLite*)(Object*)temperature)->get_noise_2d(x, y) / 2.0 + 0.5;
    float vd[1] = {};
    float vt[1] = {};

    // fast_dryness.noise->GenUniformGrid2D(vd, X, Y, 1.0, 1.0, fast_dryness.frequency, fast_dryness.seed);
    // fast_temperature.noise->GenUniformGrid2D(vt, X, Y, 1.0, 1.0, fast_temperature.frequency, fast_temperature.seed);
    vd[0] = fast_dryness.noise->GenSingle2D(X, Y, fast_dryness.seed) * fast_dryness.frequency;
    vt[0] = fast_temperature.noise->GenSingle2D(X, Y, fast_temperature.seed) * fast_temperature.frequency;

    auto d = vd[0] / 2.0 + 0.5;
    auto t = vt[0] / 2.0 + 0.5;

    if (logging) {
        UtilityFunctions::print(X, ":", Y, " = ", d);
        UtilityFunctions::print(X, ":", Y, " = ", t);
    }

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
    profile_compute.start();
    compute_biome_stats(x, y);
    profile_compute.lap();

    double X = UtilityFunctions::snappedf(x, 0.0001);
    double Y = UtilityFunctions::snappedf(y, 0.0001);

    profile_sum_distances.start();
    for (int i = 0; i < distances.size(); i++) {
        profile_get_noise.start();
        double e = ((FastNoiseLite*)(Object*)terrains[i])->get_noise_2d(X, Y) / 2.0 + 0.5;
        profile_get_noise.lap();
        profile_curve_sample.start();
        e = ((Curve*)(Object*)curves[i])->sample(e);
        profile_curve_sample.lap();
        double m = 1.0 - distances[i] / total_distance;
        result += e * m;
    }
    profile_sum_distances.lap();

    return result;
}

double GDNoiseBlender::grass_height(int biome, double x, double y)
{
    auto n = terrains.at(biome)->get_noise_2d(x, y) / 2.0 + 0.5;
    auto e = curves.at(biome)->sample(n) / curves.at(biome)->get_max_value();
    auto s = UtilityFunctions::smoothstep(0.25, 1.0, e);
    if (s == 0.0) {
        return UtilityFunctions::snappedf(e * 4, 0.1);
    } else {
        return 0.5 + s;
    }
}