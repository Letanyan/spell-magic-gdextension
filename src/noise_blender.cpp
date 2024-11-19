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

void MyNoise::noise2d(float* data, float x, float y, float w, float h, float frequency)
{
    noise->GenUniformGrid2D(data, x, y, w, h, frequency, seed);
}

void MyNoise::noise2d_inset(float* data, float x, float y, float w, float h, float frequency, int stride)
{
    auto holder = std::vector<float>();
    size_t W = (size_t)w;
    size_t H = (size_t)h;
    size_t map_size = (W + stride * 2) * (H + stride * 2);
    holder.resize(map_size);
    noise->GenUniformGrid2D(holder.data(), x - stride, y - stride, w + stride * 2, h + stride * 2, frequency, seed);
    size_t j = 0;
    for (size_t c = stride; c < h + stride; c++) {
        memcpy(&data[j], &holder.data()[c * (W + stride * 2) + stride], W * sizeof(float));
        j += W;
    }
}

void GDNoiseBlender::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("set_biome_noise", "encoded", "seed", "axis"), &GDNoiseBlender::set_biome_noise);
    ClassDB::bind_method(D_METHOD("set_elevation_mix_exp", "value"), &GDNoiseBlender::set_elevation_mix_exp);
    ClassDB::bind_method(D_METHOD("add_biome", "terrain", "seed", "curve", "location", "color"), &GDNoiseBlender::add_biome);
    // ClassDB::bind_method(D_METHOD("height", "x", "y"), &GDNoiseBlender::height);
    ClassDB::bind_method(D_METHOD("compute_biome_stats", "x", "y", "scale"), &GDNoiseBlender::compute_biome_stats);
    ClassDB::bind_method(D_METHOD("compute_biome_map_stats", "x", "y", "w", "h", "s"), &GDNoiseBlender::compute_biome_map_stats);
    ClassDB::bind_method(D_METHOD("get_total_distance"), &GDNoiseBlender::get_total_distance);
    ClassDB::bind_method(D_METHOD("get_biome"), &GDNoiseBlender::get_biome);
    ClassDB::bind_method(D_METHOD("get_biomes_map"), &GDNoiseBlender::get_biomes_map);
    ClassDB::bind_method(D_METHOD("get_color"), &GDNoiseBlender::get_color);
    ClassDB::bind_method(D_METHOD("get_distances"), &GDNoiseBlender::get_distances);

    ClassDB::bind_method(D_METHOD("texture", "noise", "x", "y", "w", "h"), &GDNoiseBlender::texture);
    ClassDB::bind_method(D_METHOD("biome_texture", "x", "y", "w", "h", "scale", "axis"), &GDNoiseBlender::biome_texture);
    ClassDB::bind_method(D_METHOD("grass_height", "biome", "x", "y"), &GDNoiseBlender::grass_height);
    ClassDB::bind_method(D_METHOD("height_map", "x", "y", "w", "h", "scale"), &GDNoiseBlender::height_map);
    ClassDB::bind_method(D_METHOD("get_locations"), &GDNoiseBlender::get_locations);
}

GDNoiseBlender::GDNoiseBlender()
{
    biome = 0;

    terrains = std::vector<MyNoise>();
    curves = std::vector<Curve*>();
    locations = PackedVector2Array();
    colors = std::vector<Vector3>();
    distances = std::vector<double>();

    distances_map = std::vector<float>();
    biome_noise_x_map = std::vector<float>();
    biome_noise_y_map = std::vector<float>();
    biome_noise_z_map = std::vector<float>();
    biome_noise_w_map = std::vector<float>();
    total_distances_map = std::vector<float>();
    min_distances_map = std::vector<float>();
    min_distances_index_map = std::vector<size_t>();
    height_map_store = PackedFloat32Array();
    colors_map = std::vector<Color>();
    biomes_map = PackedInt32Array();
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

PackedVector2Array GDNoiseBlender::get_locations()
{
    return locations;
}

PackedInt32Array GDNoiseBlender::get_biomes_map()
{
    return biomes_map;
}

void GDNoiseBlender::set_biome_noise(String encoded, int seed, int axis)
{
    switch (axis) {
    case 0:
        biome_noise_x = MyNoise(encoded.utf8().get_data(), seed);
        break;
    case 1:
        biome_noise_y = MyNoise(encoded.utf8().get_data(), seed);
        break;
    }
}

void GDNoiseBlender::set_elevation_mix_exp(double value)
{
    elevation_mix_exp = value;
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
    // float W = w + 2;
    // float H = h + 2;
    // float X = x / scale - W / 2.0 + (x / scale / scale);
    // float Y = y / scale - H / 2.0 + (y / scale / scale);
    floats.resize(w * h);
    noise.noise2d(floats.data(), x, y, w, h, scale);

    auto bytes = PackedByteArray();
    for (int i = 0; i < floats.size(); i++) {
        bytes.append((uint8_t)((floats[i] * 0.5 + 0.5) * 255));
    }

    auto result = Image::create_from_data(w, h, false, Image::Format::FORMAT_L8, bytes);
    auto res = new ImageTexture();
    res->set_image(result);
    return res;
}

ImageTexture* GDNoiseBlender::biome_texture(double x, double y, double w, double h, double scale, int axis)
{
    switch (axis) {
    case 0:
        return fast_texture(this->biome_noise_x, x, y, w, h, scale);
    case 1:
        return fast_texture(this->biome_noise_y, x, y, w, h, scale);
    }
    return fast_texture(this->biome_noise_x, x, y, w, h, scale);
}

ImageTexture* GDNoiseBlender::height_texture(PackedFloat32Array data, float w, float h)
{
    auto bytes = PackedByteArray();
    for (size_t i = 0; i < w * h; i++) {
        auto word = reinterpret_cast<uint8_t*>(&data[i]);
        bytes.append(word[0]);
        bytes.append(word[1]);
        bytes.append(word[2]);
        bytes.append(word[3]);
    }

    auto result = Image::create_from_data(w, h, false, Image::Format::FORMAT_RF, bytes);
    auto res = new ImageTexture();
    res->set_image(result);
    return res;
}

void GDNoiseBlender::add_biome(String terrain, int seed, Curve* curve, Vector2 location, Vector3 color)
{
    terrains.push_back(MyNoise(terrain.utf8().get_data(), seed));
    curves.push_back(curve);
    locations.append(location);
    colors.push_back(color);
    distances.push_back(0.0);
}

float axial_dependant_distance(Vector2 a, Vector2 b)
{
    return abs(a.x - b.x) * axial_weight + abs(a.y - b.y);
}

Vector3 oklab_mix(Vector3 colA, Vector3 colB, float h)
{
    // https://bottosson.github.io/posts/oklab
    const Basis kCONEtoLMS = Basis(
        Vector3(0.4121656120, 0.2118591070, 0.0883097947),
        Vector3(0.5362752080, 0.6807189584, 0.2818474174),
        Vector3(0.0514575653, 0.1074065790, 0.6302613616));
    const Basis kLMStoCONE = Basis(
        Vector3(4.0767245293, -1.2681437731, -0.0041119885),
        Vector3(-3.3072168827, 2.6093323231, -0.7034763098),
        Vector3(0.2307590544, -0.3411344290, 1.7068625689));

    // rgb to cone (arg of pow can't be negative)
    // Vector3 lmsA = pow( kCONEtoLMS.xform(colA), Vector3(1.0/3.0) );
    // Vector3 lmsB = pow( kCONEtoLMS.xform(colB), Vector3(1.0/3.0) );
    Vector3 lmsA = kCONEtoLMS.xform(colA);
    Vector3 lmsB = kCONEtoLMS.xform(colB);
    lmsA.x = powf(lmsA.x, 1.0 / 3.0);
    lmsA.y = powf(lmsA.y, 1.0 / 3.0);
    lmsA.z = powf(lmsA.z, 1.0 / 3.0);
    lmsB.x = powf(lmsB.x, 1.0 / 3.0);
    lmsB.y = powf(lmsB.y, 1.0 / 3.0);
    lmsB.z = powf(lmsB.z, 1.0 / 3.0);
    // lerp
    Vector3 lms = lmsA.lerp(lmsB, h);
    // gain in the middle (no oaklab anymore, but looks better?)
    // lms *= 1.0+0.2*h*(1.0-h);
    // cone to rgb
    return kLMStoCONE.xform(lms * lms * lms);
}

float step(float a, float b)
{
    return b < a ? 0.0 : 1.0;
}

Vector3 rgb2hsv(Vector3 c)
{
    Vector4 K = Vector4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
    Vector4 p = Vector4(c.y, c.z, K.w, K.z).lerp(Vector4(c.y, c.z, K.x, K.y), step(c.z, c.y));
    Vector4 q = Vector4(p.x, p.y, p.w, c.x).lerp(Vector4(c.x, p.y, p.z, p.x), step(p.x, c.x));

    float d = q.x - UtilityFunctions::minf(q.w, q.y);
    float e = 1.0e-10;
    return Vector3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
}

Vector3 fract(Vector3 v)
{
    return v - Vector3(UtilityFunctions::floorf(v.x), UtilityFunctions::floorf(v.y), UtilityFunctions::floorf(v.z));
}

Vector3 abs(Vector3 v)
{
    return Vector3(UtilityFunctions::abs(v.x), UtilityFunctions::abs(v.y), UtilityFunctions::abs(v.z));
}

Vector3 hsv2rgb(Vector3 c)
{
    Vector4 K = Vector4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    Vector3 p = abs(fract(Vector3(c.x, c.x, c.x) + Vector3(K.x, K.y, K.z)) * 6.0 - Vector3(K.w, K.w, K.w));
    return c.z * Vector3(K.x, K.x, K.x).lerp((p - Vector3(K.x, K.x, K.x)).clamp(Vector3(0, 0, 0), Vector3(1, 1, 1)), c.y);
}

void GDNoiseBlender::compute_biome_stats(double x, double y, double scale)
{
    x = x + (x / scale);
    y = y + (y / scale);
    double X = UtilityFunctions::snappedf(x, 0.0001);
    double Y = UtilityFunctions::snappedf(y, 0.0001);
    auto bx = biome_noise_x.noise2d(X, Y) * 0.5 + 0.5;
    auto by = biome_noise_y.noise2d(X, Y) * 0.5 + 0.5;

    auto p = Vector2(bx, by);
    auto min_distance = INFINITY;
    auto pos = 0;
    total_distance = 0.0;
    auto clr = Vector3(1, 1, 1);
    auto dist = 0.0;
    auto c = Vector3(0, 0, 0);
    for (size_t i = 0; i < locations.size(); i++) {
        dist = axial_dependant_distance(p, locations[i]);
        distances[i] = dist;
        total_distance += dist;
        if (dist <= (axial_weight + 1.0)) {
            float mix_alpha = pow(1.0 - dist / (axial_weight + 1.0), 3.0);
            c = oklab_mix(Vector3(1, 1, 1), colors[i], mix_alpha);
            clr = clr * c;
        }
        if (dist < min_distance) {
            min_distance = dist;
            pos = i;
        }
    }
    biome = pos;

    Vector3 temp_color = rgb2hsv(Vector3(clr.x, clr.y, clr.z));
    temp_color.z = 1.0 - pow(1.0 - temp_color.z, 2.0);
    temp_color = hsv2rgb(temp_color);

    color = Color(temp_color.x, temp_color.y, temp_color.z);
}

void GDNoiseBlender::compute_biome_map_stats(double x, double y, double w, double h, double scale)
{
    size_t map_size = (size_t)(w * h);
    biome_noise_x_map.resize(map_size);
    biome_noise_y_map.resize(map_size);
    biome_noise_z_map.resize(map_size);
    biome_noise_w_map.resize(map_size);

    biome_noise_x.noise2d(biome_noise_x_map.data(), x, y, w, h, scale);
    biome_noise_y.noise2d(biome_noise_y_map.data(), x, y, w, h, scale);

    distances_map.resize(map_size * locations.size());
    total_distances_map.resize(map_size);
    min_distances_map.resize(map_size);
    min_distances_index_map.resize(map_size);
    biomes_map.resize(map_size);
    // colors_map.resize(map_size);
    for (size_t r = 0; r < w * h; r++) {
        auto min_distance = INFINITY;
        auto pos = 0;
        auto p = Vector2(biome_noise_x_map[r] * 0.5 + 0.5, biome_noise_y_map[r] * 0.5 + 0.5);
        auto dist = 0.0;
        // auto clr = Vector3(1, 1, 1);
        // auto c = Vector3(0, 0, 0);
        total_distance = 0.0;
        for (size_t i = 0; i < locations.size(); i++) {
            dist = axial_dependant_distance(p, locations[i]);
            distances_map[r * locations.size() + i] = dist;
            total_distance += dist;
            // c = colors[i].lerp(Vector3(1, 1, 1), dist);
            // if (dist <= (axial_weight + 1.0)) {
            //     float mix_alpha = pow(1.0 - dist / (axial_weight + 1.0), 3.0);
            //     c = oklab_mix(Vector3(1, 1, 1), colors[i], mix_alpha);
            //     clr = clr * c;
            // }
            if (dist < min_distance) {
                min_distances_map[r] = dist;
                min_distances_index_map[r] = i;
                min_distance = dist;
                pos = i;
            }
        }
        // Vector3 temp_color = rgb2hsv(Vector3(clr.x, clr.y, clr.z));
        // temp_color.z = 1.0 - pow(1.0 - temp_color.z, 2.0);
        // temp_color = hsv2rgb(temp_color);

        total_distances_map[r] = total_distance;
        biomes_map[r] = pos + 1;
        // colors_map[r] = Color(temp_color.x, temp_color.y, temp_color.z);
    }
}

double GDNoiseBlender::height(double x, double y, double scale)
{
    double result = 0.0;
    profile_compute.start();
    compute_biome_stats(x, y, scale);
    profile_compute.lap();

    double X = UtilityFunctions::snappedf(x, 0.0001);
    double Y = UtilityFunctions::snappedf(y, 0.0001);

    profile_sum_distances.start();
    for (int i = 0; i < distances.size(); i++) {
        double e = terrains[i].noise2d(X, Y) / 2.0 + 0.5;
        e = ((Curve*)(Object*)curves[i])->sample(e);
        double m = powf(1.0 - distances[i] / total_distance, 20.0);
        result += e * m;
    }
    // result += 400.0;
    profile_sum_distances.lap();

    return result;
}

PackedFloat32Array GDNoiseBlender::height_map(double x, double y, double w, double h, double scale)
{
    size_t map_size = (size_t)(w * h);

    height_map_store.resize(map_size);
    height_map_store.fill(0.0);
    compute_biome_map_stats(x, y, w, h, scale);

    std::vector<float> terrain_noise = {};
    terrain_noise.resize(map_size);
    // auto prof = GDProfiler();
    // prof.start();
    for (int i = 0; i < locations.size(); i++) {
        terrains[i].noise2d(terrain_noise.data(), x, y, w, h, scale);
        auto curve = curves[i];
        for (int j = 0; j < terrain_noise.size(); j++) {
            double e = terrain_noise[j] * 0.5 + 0.5;
            e = curve->sample_baked(e);
            double m = powf(1.0 - distances_map[j * locations.size() + i] / total_distances_map[j], elevation_mix_exp);
            height_map_store[j] += e * m;
        }
    }
    // prof.stop();
    // UtilityFunctions::print("curve: ", prof.elapsed);
    // for (int j = 0; j < map_size; j++) {
    //     // double e = terrains[min_distances_index_map[j]].noise2d(X, Y) * 0.5 + 0.5;
    //     double e = terrain_noise[j] * 0.5 + 0.5;
    //     e = ((Curve*)(Object*)curves[min_distances_index_map[j]])->sample(e);
    //     double m = powf(1.0 - min_distances_map[j] / total_distances_map[j], 1.0);
    //     result[j] = UtilityFunctions::lerp(result[j], e * m, 0.75);
    // }

    return height_map_store;
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