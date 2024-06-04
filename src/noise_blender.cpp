#include "noise_blender.h"
#include <iostream>
#include <math.h>

#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

MyNoise::MyNoise(const char* string, int seed)
{
    noise = FastNoise::NewFromEncodedNodeTree(string);
    this->seed = seed;
}

MyNoise::MyNoise()
{
}

MyNoise::~MyNoise()
{
}

float MyNoise::noise2d(float x, float y)
{
    return noise->GenSingle2D(x, y, seed);
}

void MyNoise::noise2d(float* data, float x, float y, float w, float h)
{
    noise->GenUniformGrid2D(data, x, y, w, h, 1.0, seed);
}

void GDNoiseBlender::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("set_dryness", "encoded", "seed"), &GDNoiseBlender::set_dryness);
    ClassDB::bind_method(D_METHOD("set_temperature", "encoded", "seed"), &GDNoiseBlender::set_temperature);
    ClassDB::bind_method(D_METHOD("add_biome", "terrain", "seed", "curve", "location", "color"), &GDNoiseBlender::add_biome);
    ClassDB::bind_method(D_METHOD("height", "x", "y"), &GDNoiseBlender::height);
    ClassDB::bind_method(D_METHOD("compute_biome_stats", "x", "y"), &GDNoiseBlender::compute_biome_stats);
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
    terrains = std::vector<MyNoise>();
    curves = std::vector<Curve*>();
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

void GDNoiseBlender::set_temperature(String encoded, int seed)
{
    temperature = MyNoise(encoded.utf8().get_data(), seed);
}

void GDNoiseBlender::set_dryness(String encoded, int seed)
{
    dryness = MyNoise(encoded.utf8().get_data(), seed);
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
    noise.noise2d(floats.data(), X, Y, W, H);

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
    return fast_texture(this->dryness, x, y, w, h, scale);
}

ImageTexture* GDNoiseBlender::temperature_texture(double x, double y, double w, double h, double scale)
{
    return fast_texture(this->temperature, x, y, w, h, scale);
}

void GDNoiseBlender::add_biome(String terrain, int seed, Curve* curve, Vector2 location, Vector3 color)
{
    terrains.push_back(MyNoise(terrain.utf8().get_data(), seed));
    curves.push_back(curve);
    locations.push_back(location);
    colors.push_back(color);
    distances.push_back(0.0);
}

void GDNoiseBlender::compute_biome_stats(double x, double y)
{
    double X = UtilityFunctions::snappedf(x, 0.0001);
    double Y = UtilityFunctions::snappedf(y, 0.0001);
    auto d = dryness.noise2d(X, Y) / 2.0 + 0.5;
    auto t = temperature.noise2d(X, Y) / 2.0 + 0.5;

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
        // profile_get_noise.start();
        double e = terrains[i].noise2d(X, Y) / 2.0 + 0.5;
        // profile_get_noise.lap();
        // profile_curve_sample.start();
        e = ((Curve*)(Object*)curves[i])->sample(e);
        // profile_curve_sample.lap();
        double m = powf(1.0 - distances[i] / total_distance, 10.0);
        result += e * m;
    }
    // result += 400.0;
    profile_sum_distances.lap();

    return result;
}

double GDNoiseBlender::grass_height(int biome, double x, double y)
{
    auto n = terrains.at(biome).noise2d(x, y) / 2.0 + 0.5;
    auto e = curves.at(biome)->sample(n) / curves.at(biome)->get_max_value();
    auto s = UtilityFunctions::smoothstep(0.25, 1.0, e);
    if (s == 0.0) {
        return UtilityFunctions::snappedf(e * 4, 0.1);
    } else {
        return 0.5 + s;
    }
}