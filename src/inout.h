#ifndef GDINOUT_H
#define GDINOUT_H

#include <godot_cpp\classes\ref.hpp>

#include <functional>
#include <string>
#include <vector>

namespace godot {

class GDInOut : public Object {
    GDCLASS(GDInOut, Object)

private:
protected:
    static void _bind_methods();
    Variant data;

public:
    GDInOut();
    ~GDInOut();

    Variant get_data();
    void set_data(Variant data);
};

}

#endif