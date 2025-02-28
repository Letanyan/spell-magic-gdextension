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
    bool contains_time_dependent;
    int32_t variable_update_set;

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
    bool contains_variable_token(const int16_t var_token);
    String all_variables_is_contained(const Dictionary& dict, const Dictionary& user_funcs);
    String all_variable_tokens_is_contained(const Vars& dict, const Dictionary& user_funcs);
    static String bake(const String& expr, const Dictionary& map);

    bool get_contains_time_dependent();
    int32_t get_variable_update_set();

    void copy_from(GDExpr* expr);

    String infix_description();

    String token_description();

    void print_profiling();
};

double clerpf(double a, double b, double t);

}

#endif