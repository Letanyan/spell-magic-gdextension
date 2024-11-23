#ifndef GDEXPR_H
#define GDEXPR_H

#include <godot_cpp/classes/ref_counted.hpp>

#include "token.h"
#include <vector>

namespace godot {

class GDExpr : public RefCounted {
    GDCLASS(GDExpr, RefCounted)

private:
protected:
    static void _bind_methods();

public:
    GDExpr();
    ~GDExpr();

    GDExpr* build_in(String expr);

    std::vector<GDToken> expression;
    String error;

    String get_error();
    void set_error(String err_message);

    void build_from_tokens(const std::vector<GDToken>& tokens);
    void build(String expr);

    Variant compute(Dictionary map, Dictionary user_funcs);

    bool contains_variable(String var_name);
    static String bake(String expr, Dictionary map);

    void copy_from(GDExpr* expr);
};

double clerpf(double a, double b, double t);

}

#endif