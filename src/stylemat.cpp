#include "stylemat.h"
#include <godot_cpp/classes/geometry2d.hpp>
#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

void GDStyleMat::_bind_methods()
{
    ClassDB::bind_static_method("GDStyleMat", D_METHOD("render_rect", "canvas_id", "rect", "texture", "corner_radius", "border_width", "corner_detail"), &GDStyleMat::render_rect);
}

GDStyleMat::GDStyleMat()
{
}

GDStyleMat::~GDStyleMat()
{
}

// Color get_color(Texture2D* tex, Vector2 coord)
// {
//     if (tex != nullptr) {
//         Ref<Image> img = tex->get_image();
//         return img->get_pixel(round(coord.x * (img->get_width() - 1)), round(coord.y * (img->get_height() - 1)));
//     }
//     return Color(1, 0, 0);
// }

Color get_color(Ref<Image> img, Vector2 coord)
{
    if (img == nullptr) {
        return Color(0, 0, 0, 0);
    } else {
        return img->get_pixel(round(coord.x * (img->get_width() - 1)), round(coord.y * (img->get_height() - 1)));
    }
}

void GDStyleMat::render_rect(RID canvas_id, Rect2 rect, Texture2D* texture, Vector4 corner_radius, float border_width, float corner_detail)
{
    UtilityFunctions::print("rendering");
    auto vertices = PackedVector2Array();
    auto uvs = PackedVector2Array();
    auto colors = PackedColorArray();

    auto top_right_corner_vertex = Vector2();
    auto top_right_corner_uv = Vector2();
    auto top_right_corner_color = Color(1, 1, 1);

    Ref<Image> img = nullptr;
    if (texture != nullptr) {
        img = texture->get_image();
    }

    if (corner_radius.x != 0.0) {
        auto center = rect.position + Vector2(rect.size.x - corner_radius.x, 0 + corner_radius.x);
        auto angle = 0.0f;
        top_right_corner_vertex = center + Vector2(cos(angle) * corner_radius.x, -sin(angle) * corner_radius.x);
        top_right_corner_uv = Vector2((top_right_corner_vertex.x - rect.position.x) / rect.size.x, (top_right_corner_vertex.y - rect.position.y) / rect.size.y);
        top_right_corner_color = get_color(img, top_right_corner_uv);
        for (int i = 0; i < corner_radius.x * corner_detail + 1; i++) {
            auto point = center + Vector2(cos(angle) * corner_radius.x, -sin(angle) * corner_radius.x);
            if (vertices.is_empty() || vertices[vertices.size() - 1] != point) {
                vertices.append(point);
                auto u = Vector2((point.x - rect.position.x) / rect.size.x, (point.y - rect.position.y) / rect.size.y);
                uvs.append(u);
                colors.append(get_color(img, u));
            }
            angle += (Math_PI / 2.0) / (corner_radius.x * corner_detail);
        }
    } else {
        top_right_corner_vertex = rect.position + Vector2(rect.size.x, 0);
        top_right_corner_uv = Vector2(1, 0);
        top_right_corner_color = get_color(img, Vector2(1, 0));
        vertices.append(top_right_corner_vertex);
        uvs.append(top_right_corner_uv);
        colors.append(top_right_corner_color);
    }

    if (corner_radius.y != 0) {
        auto center = rect.position + Vector2(0 + corner_radius.y, 0 + corner_radius.y);
        auto angle = Math_PI / 2.0;
        for (int i = 0; i < corner_radius.y * corner_detail + 1; i++) {
            auto point = center + Vector2(cos(angle) * corner_radius.y, -sin(angle) * corner_radius.y);
            if (vertices[vertices.size() - 1] != point) {
                vertices.append(point);
                auto u = Vector2((point.x - rect.position.x) / rect.size.x, (point.y - rect.position.y) / rect.size.y);
                uvs.append(u);
                colors.append(get_color(img, u));
            }
            angle += (Math_PI / 2.0) / (corner_radius.y * corner_detail);
        }
    } else {
        vertices.append(rect.position + Vector2(0, 0));
        uvs.append(Vector2(0, 0));
        colors.append(get_color(img, Vector2(0, 0)));
    }

    if (corner_radius.z != 0) {
        auto center = rect.position + Vector2(0 + corner_radius.z, rect.size.y - corner_radius.z);
        auto angle = Math_PI;
        for (int i = 0; i < corner_radius.z * corner_detail + 1; i++) {
            auto point = center + Vector2(cos(angle) * corner_radius.z, -sin(angle) * corner_radius.z);
            if (vertices[vertices.size() - 1] != point) {
                vertices.append(point);
                auto u = Vector2((point.x - rect.position.x) / rect.size.x, (point.y - rect.position.y) / rect.size.y);
                uvs.append(u);
                colors.append(get_color(img, u));
            }
            angle += (Math_PI / 2.0) / (corner_radius.z * corner_detail);
        }
    } else {
        vertices.append(rect.position + Vector2(0, rect.size.y));
        uvs.append(Vector2(0, 1));
        colors.append(get_color(img, Vector2(0, 1)));
    }

    if (corner_radius.w != 0) {
        auto center = rect.position + Vector2(rect.size.x - corner_radius.w, rect.size.y - corner_radius.w);
        auto angle = Math_PI / 2.0 * 3.0;
        for (int i = 0; i < corner_radius.w * corner_detail + 1; i++) {
            auto point = center + Vector2(cos(angle) * corner_radius.w, -sin(angle) * corner_radius.w);
            if (vertices[vertices.size() - 1] != point) {
                vertices.append(point);
                auto u = Vector2((point.x - rect.position.x) / rect.size.x, (point.y - rect.position.y) / rect.size.y);
                uvs.append(u);
                colors.append(get_color(img, u));
            }
            angle += (Math_PI / 2.0) / (corner_radius.w * corner_detail);
        }
    } else {
        vertices.append(rect.position + Vector2(rect.size.x, rect.size.y));
        uvs.append(Vector2(1, 1));
        colors.append(get_color(img, Vector2(1, 1)));
    }

    if (vertices[vertices.size() - 1] != top_right_corner_vertex) {
        vertices.append(top_right_corner_vertex);
        uvs.append(top_right_corner_uv);
        colors.append(top_right_corner_color);
    }

    if (!Geometry2D::get_singleton()->triangulate_polygon(vertices).is_empty()) {
        RenderingServer::get_singleton()->canvas_item_add_polygon(canvas_id, vertices, colors, uvs);
    } else {
        RenderingServer::get_singleton()->canvas_item_add_rect(canvas_id, rect, Color(1, 0, 0, 1));
    }
}