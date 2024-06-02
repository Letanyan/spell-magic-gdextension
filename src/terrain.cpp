#include "terrain.h"
#include "noise_blender.h"

// #include <godot_cpp/core/class_db.hpp>

using namespace godot;

void GDTerrain::_bind_methods()
{
    // ClassDB::bind_static_method("GDExpr", D_METHOD("bake", "expr", "map"), &GDExpr::bake);

    // ClassDB::bind_method(D_METHOD("build_in", "expr"), &GDExpr::build_in);

    // ClassDB::bind_method(D_METHOD("build", "expression"), &GDExpr::build);
    // ClassDB::bind_method(D_METHOD("compute", "variables"), &GDExpr::compute);
    // ClassDB::bind_method(D_METHOD("contains_variable", "variable_name"), &GDExpr::contains_variable);

    // ClassDB::bind_method(D_METHOD("get_error"), &GDExpr::get_error);
    // ClassDB::bind_method(D_METHOD("set_error", "error_message"), &GDExpr::set_error);
    // ClassDB::add_property("GDExpr", PropertyInfo(Variant::STRING, "error"), "set_error", "get_error");
}

GDTerrain::GDTerrain()
{
}

GDTerrain::~GDTerrain()
{
}