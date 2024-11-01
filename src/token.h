#ifndef GDTOKEN_H
#define GDTOKEN_H

#include <godot_cpp/classes/ref_counted.hpp>
#include <vector>

namespace godot {

enum GDTokenKind {
    tkNUMBER,
    tkWORD,
    tkVAR,
    tkFUNC,
    tkOP,
    tkOPEN,
    tkCLOSE,
    tkCOMMA,
    tkPREFIX_OP,
    tkNONE,
    tkERROR
};

struct GDToken {
public:
    GDToken();
    GDToken(GDTokenKind _kind, godot::String _raw);

    godot::String raw;
    GDTokenKind kind;
};

std::vector<GDToken> tokenize(godot::String expr);
godot::String build_string_from_tokens(const std::vector<GDToken>& tokens);
bool gd_operator_precedes(GDToken op1, GDToken op2);
int gd_operator_precedence(GDToken op);
bool gd_operator_is_right_associative(GDToken op);
int number_of_func_arguments(godot::String name);

}

#endif