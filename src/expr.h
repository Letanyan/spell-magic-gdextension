#ifndef GDEXPR_H
#define GDEXPR_H

#include <godot_cpp/classes/ref_counted.hpp>
// #include <godot_cpp/variant/array.hpp>

#include "token.h"
#include <functional>
#include <string>
#include <vector>

namespace godot {

class GDExpr : public Object {
    GDCLASS(GDExpr, Object)

private:
protected:
    static void _bind_methods();

public:
    GDExpr();
    ~GDExpr();

    std::vector<GDToken> expression;
    std::string error;

    String get_error();
    void set_error(String err_message);

    void build_base(std::string expr);
    void build(String expr);

    double compute_base(std::function<double(char*, double)> map);

    double compute(Dictionary map);
};

bool gd_operator_precedes(GDToken op1, GDToken op2);

}

#endif