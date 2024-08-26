#ifndef GDSTYLEMAT_H
#define GDSTYLEMAT_H

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/classes/texture2d.hpp>

namespace godot {

class GDStyleMat : public RefCounted {
    GDCLASS(GDStyleMat, RefCounted)

private:
protected:
    static void _bind_methods();

public:
    GDStyleMat();
    ~GDStyleMat();

    static void render_rect(RID canvas_id, Rect2 rect, Texture2D* texture, Vector4 corner_radius, float border_width, float corner_detail);
};

}

#endif