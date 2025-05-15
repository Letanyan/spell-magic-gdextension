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

enum GDTokenOpKind;
enum GDTokenFuncKind;
enum GDTokenScalarVarKind;
enum GDTokenVecVarKind;

struct GDToken {
public:
    GDToken();
    GDToken(GDTokenKind _kind, godot::String _raw);
    GDToken(GDTokenKind _kind, char32_t _raw);
    GDToken(godot::String _raw);

    godot::String raw;
    GDTokenKind kind;
    int16_t sub_kind;
    bool is_var_vec;
};

std::vector<GDToken> tokenize(godot::String expr);
godot::String build_string_from_tokens(const std::vector<GDToken>& tokens);
bool gd_operator_precedes(GDToken op1, GDToken op2);
int gd_operator_precedence(GDToken op);
bool gd_operator_is_right_associative(GDToken op);
int number_of_func_arguments(godot::String name);
bool siseq(String a, const char* other);

enum GDTokenOpKind : int16_t {
    tkopNONE = -1,
    tkopPLUS,
    tkopMINUS,
    tkopMULT,
    tkopDIV,
    tkopEXP,
    tkopDOT,

    tkopSIZE
};

enum GDTokenFuncKind : int16_t {
    tkfnNONE = -1,
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
    tkfn_snap,

    tkfnSIZE
};

enum GDTokenScalarVarKind : int16_t {
    tkvNONE = -1,
    tkv_r0,
    tkv_r1,
    tkv_r2,
    tkv_r3,
    tkv_r4,
    tkv_r5,
    tkv_r6,
    tkv_r7,
    tkv_r8,
    tkv_r9,
    tkv_pi,
    tkv_tau,
    tkv_N,
    tkv_M,
    tkv_C,
    tkv_L,
    tkv_T,
    tkv_P,
    tkv_CR,
    tkv_CD,
    tkv_x,
    tkv_y,
    tkv_z,
    tkv_r,
    tkv_rn0,
    tkv_rn1,
    tkv_rn2,
    tkv_rn3,
    tkv_rn4,
    tkv_rn5,
    tkv_rn6,
    tkv_rn7,
    tkv_rn8,
    tkv_rn9,
    tkv_n,
    tkv_D,
    tkv_l,
    tkv_fl,
    tkv_Bx,
    tkv_By,
    tkv_Bz,
    tkv_Br,

    tkv_u,
    tkv_v,
    tkv_w,
    tkv_ru,
    tkv_rv,
    tkv_rw,
    tkv_U,
    tkv_V,
    tkv_W,
    tkv_rU,
    tkv_rV,
    tkv_rW,
    tkv_i,
    tkv_j,
    tkv_k,
    tkv_ri,
    tkv_rj,
    tkv_rk,
    tkv_I,
    tkv_J,
    tkv_K,
    tkv_rI,
    tkv_rJ,
    tkv_rK,

    tkv_t,
    tkv_tu,
    tkv_tv,
    tkv_tw,
    tkv_tru,
    tkv_trv,
    tkv_trw,
    tkv_tU,
    tkv_tV,
    tkv_tW,
    tkv_trU,
    tkv_trV,
    tkv_trW,
    tkv_ti,
    tkv_tj,
    tkv_tk,
    tkv_tri,
    tkv_trj,
    tkv_trk,
    tkv_tI,
    tkv_tJ,
    tkv_tK,
    tkv_trI,
    tkv_trJ,
    tkv_trK,
    tkv_tC,

    tkv_Tu,
    tkv_Tv,
    tkv_Tw,
    tkv_Tru,
    tkv_Trv,
    tkv_Trw,
    tkv_TU,
    tkv_TV,
    tkv_TW,
    tkv_TrU,
    tkv_TrV,
    tkv_TrW,
    tkv_Ti,
    tkv_Tj,
    tkv_Tk,
    tkv_Tri,
    tkv_Trj,
    tkv_Trk,
    tkv_TI,
    tkv_TJ,
    tkv_TK,
    tkv_TrI,
    tkv_TrJ,
    tkv_TrK,
    tkv_TC,

    tkv_frame_time,

    tkvSIZE
};

enum GDTokenVecVarKind : int64_t {
    tkvvNONE = -1,

    tkvv_tuvw,
    tkvv_truvw,
    tkvv_tUVW,
    tkvv_trUVW,
    tkvv_tijk,
    tkvv_trijk,
    tkvv_tIJK,
    tkvv_trIJK,

    tkvv_uvw,
    tkvv_ruvw,
    tkvv_UVW,
    tkvv_rUVW,
    tkvv_ijk,
    tkvv_rijk,
    tkvv_IJK,
    tkvv_rIJK,

    tkvv_Tuvw,
    tkvv_Truvw,
    tkvv_TUVW,
    tkvv_TrUVW,
    tkvv_Tijk,
    tkvv_Trijk,
    tkvv_TIJK,
    tkvv_TrIJK,

    tkvv_Bxyz,
    tkvv_old_pos,
    tkvv_rel_pos,
    tkvv_abs_pos,

    tkvvSIZE
};

GDTokenFuncKind __func_kind_from_string___(String str);
GDTokenOpKind __op_kind_from_char___(char32_t c);
GDTokenScalarVarKind __var_scalar_kind_from_string___(String str);
GDTokenVecVarKind __var_vec_kind_from_string___(String str);

}

#endif