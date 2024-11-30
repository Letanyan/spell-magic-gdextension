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
    tkVECTOR,
    tkNONE,
    tkERROR
};

enum GDTokenOpKind : int16_t {
    tkopPLUS,
    tkopMINUS,
    tkopMULT,
    tkopDIV,
    tkopEXP,
    tkopDOT,

    tkopNONE = -1
};

enum GDTokenFuncKind : int16_t {
    tkfn_segment5,
    tkfn_segment4,
    tkfn_rot_x,
    tkfn_rot_y,
    tkfn_rot_z,
    tkfn_segment3,
    tkfn_dot3,
    tkfn_cross_x,
    tkfn_cross_y,
    tkfn_cross_z,
    tkfn_proj_x,
    tkfn_proj_y,
    tkfn_proj_z,
    tkfn_cubic,
    tkfn_quad,
    tkfn_segment2,
    tkfn_dot2,
    tkfn_rot,
    tkfn_vec,
    tkfn_len3,
    tkfn_perp_x,
    tkfn_perp_y,
    tkfn_perp_z,
    tkfn_if,
    tkfn_clamp,
    tkfn_lerp,
    tkfn_unit_x,
    tkfn_unit_y,
    tkfn_unit_z,
    tkfn_cross,
    tkfn_dot,
    tkfn_proj,
    tkfn_max,
    tkfn_min,
    tkfn_lt,
    tkfn_gt,
    tkfn_lte,
    tkfn_gte,
    tkfn_eq,
    tkfn_neq,
    tkfn_atan2,
    tkfn_mod,
    tkfn_div,
    tkfn_pow,
    tkfn_len2,
    tkfn_unit,
    tkfn_len,
    tkfn_perp,
    tkfn_abs,
    tkfn_sqrt,
    tkfn_cbrt,
    tkfn_sqr,
    tkfn_cube,
    tkfn_inv,
    tkfn_floor,
    tkfn_ceil,
    tkfn_round,
    tkfn_not,
    tkfn_log10,
    tkfn_logN,
    tkfn_sinh,
    tkfn_cosh,
    tkfn_tanh,
    tkfn_asinh,
    tkfn_acosh,
    tkfn_atanh,
    tkfn_sin,
    tkfn_cos,
    tkfn_tan,
    tkfn_asin,
    tkfn_acos,
    tkfn_atan,

    tkfn_NONE = -1
};

struct GDToken {
public:
    GDToken();
    GDToken(GDTokenKind _kind, godot::String _raw);
    GDToken(GDTokenKind _kind, GDTokenFuncKind _sub_kind, godot::String _raw);
    GDToken(GDTokenKind _kind, char32_t _raw);

    godot::String raw;
    GDTokenKind kind;
    int16_t sub_kind;
};

std::vector<GDToken> tokenize(godot::String expr);
godot::String build_string_from_tokens(const std::vector<GDToken>& tokens);
bool gd_operator_precedes(GDToken op1, GDToken op2);
int gd_operator_precedence(GDToken op);
bool gd_operator_is_right_associative(GDToken op);
int number_of_func_arguments(godot::String name);

}

#endif