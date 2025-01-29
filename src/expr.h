#ifndef GDEXPR_H
#define GDEXPR_H

#include <godot_cpp/classes/ref_counted.hpp>

#include "token.h"
#include "variable_stash.h"
#include <vector>

#include "my_profiler.h"

namespace godot {

class GDExpr : public RefCounted {
    GDCLASS(GDExpr, RefCounted)

private:
protected:
    static void _bind_methods();

    GDProfiler prof_number;
    GDProfiler prof_var;
    GDProfiler prof_binop;
    GDProfiler prof_func;

public:
    GDExpr();
    ~GDExpr();

    GDExpr* build_in(const String& expr);

    std::vector<GDToken> expression;
    String error;

    String get_error();
    void set_error(const String& err_message);

    void build_from_tokens(const std::vector<GDToken>& tokens);
    void build(const String& expr);

    Variant compute(const Vars* map, const Dictionary& user_funcs, bool debug);
    float compute_value(const Vars* map, const Dictionary& user_funcs, bool debug);

    bool contains_variable(const String& var_name);
    String all_variables_is_contained(const Dictionary& dict, const Dictionary& user_funcs);
    static String bake(const String& expr, const Dictionary& map);

    void copy_from(GDExpr* expr);

    String token_description();

    void print_profiling();
};

double clerpf(double a, double b, double t);

}

#endif