#include "inout.h"

using namespace godot;

void GDInOut::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("get_data"), &GDInOut::get_data);
    ClassDB::bind_method(D_METHOD("set_data", "data"), &GDInOut::set_data);
    ClassDB::add_property("GDInOut", PropertyInfo(Variant::STRING, "data"), "set_data", "get_data");
}

Variant GDInOut::get_data()
{
    return data;
}

void GDInOut::set_data(Variant data)
{
    this->data = data;
}

GDInOut::GDInOut()
{
    // Initialize any variables here.
}

GDInOut::~GDInOut()
{
    // Add your cleanup here.
}