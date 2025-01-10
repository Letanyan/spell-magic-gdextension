#include "token.h"

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <vector>

using namespace godot;

GDToken::GDToken()
{
    kind = tkNONE;
    sub_kind = -1;
    is_var_vec = false;
    raw = "";
}

GDToken::GDToken(godot::String _raw)
{
    raw = _raw;
    is_var_vec = false;
    sub_kind = __func_kind_from_string___(_raw);
    if (sub_kind != -1) {
        kind = tkFUNC;
    } else {
        kind = tkVAR;
        sub_kind = __var_scalar_kind_from_string___(_raw);
        if (sub_kind == tkvNONE) {
            sub_kind = __var_vec_kind_from_string___(_raw);
            is_var_vec = sub_kind != -1;
        } else {
            is_var_vec = false;
        }
    }
}

GDToken::GDToken(GDTokenKind _kind, godot::String _raw)
{
    kind = _kind;
    raw = _raw;
    is_var_vec = false;
    if (kind == tkFUNC) {
        sub_kind = __func_kind_from_string___(_raw);
    } else if (kind == tkVAR) {
        sub_kind = __var_scalar_kind_from_string___(_raw);
        if (sub_kind == tkvNONE) {
            sub_kind = __var_vec_kind_from_string___(_raw);
            is_var_vec = sub_kind != -1;
        } else {
            is_var_vec = false;
        }
    } else {
        sub_kind = -1;
    }
}

GDToken::GDToken(GDTokenKind _kind, char32_t _raw)
{
    kind = _kind;
    raw = String();
    raw += _raw;
    sub_kind = __op_kind_from_char___(_raw);
}

bool is_func(godot::String name)
{
    return number_of_func_arguments(name) > 0;
}

int godot::number_of_func_arguments(godot::String name)
{
    if (name == "sin" || name == "cos" || name == "tan" || name == "asin" || name == "acos" || name == "atan") {
        return 1;
    }
    if (name == "sinh" || name == "cosh" || name == "tanh" || name == "asinh" || name == "acosh" || name == "atanh") {
        return 1;
    }
    if (name == "inv" || name == "floor" || name == "ceil" || name == "round" || name == "not" || name == "log10" || name == "logN") {
        return 1;
    }
    if (name == "abs" || name == "sqrt" || name == "cbrt" || name == "sqr" || name == "cube") {
        return 1;
    }
    if (name == "unit" || name == "cube" || name == "len" || name == "perp") {
        return 1;
    }
    if (name == "atan2" || name == "mod" || name == "div" || name == "pow" || name == "len2") {
        return 2;
    }
    if (name == "max" || name == "min" || name == "lt" || name == "gt" || name == "lte" || name == "gte" || name == "eq" || name == "neq") {
        return 2;
    }
    if (name == "cross" || name == "dot" || name == "proj") {
        return 2;
    }
    if (name == "if" || name == "clamp" || name == "lerp" || name == "unit_x" || name == "unit_y" || name == "unit_z") {
        return 3;
    }
    if (name == "rot" || name == "vec" || name == "len3" || name == "perp_x" || name == "perp_y" || name == "perp_z") {
        return 3;
    }
    if (name == "quad" || name == "segment2" || name == "dot2") {
        return 4;
    }
    if (name == "cubic") {
        return 5;
    }
    if (name == "segment3" || name == "dot3" || name == "cross_x" || name == "cross_y" || name == "cross_z" || name == "proj_x" || name == "proj_y" || name == "proj_z") {
        return 6;
    }
    if (name == "rot_x" || name == "rot_y" || name == "rot_z") {
        return 7;
    }
    if (name == "segment4") {
        return 8;
    }
    if (name == "segment5") {
        return 10;
    }
    return 0;
}

bool __is_alpha__(char c)
{
    return isalpha(c) || c == '_';
}

bool __is_alpha_num__(char c)
{
    return __is_alpha__(c) || isdigit(c);
}

bool __is_digit___(char c)
{
    return isdigit(c);
}

bool __is_op__(char c)
{
    // we don't include operator ^ because fuck c++ and its bullshit inconsistencies.
    // c == ^
    return c == '+' || c == '-' || c == '/' || c == '*' || c == '.';
}

std::vector<GDToken> godot::tokenize(godot::String expr)
{
    GDTokenKind state = tkNONE;
    auto result = std::vector<GDToken>();
    result.reserve(8);

    bool last_was_op = true;
    godot::String current = "";

    const char32_t* expr_ptr = expr.ptr();
    // for (char c : expr) {
    for (int i = 0; i < expr.length(); i++) {
        char32_t c = expr_ptr[i];
        switch (state) {
        case tkNUMBER:
            if (__is_digit___(c) || c == '.') {
                current += c;
            } else {
                result.emplace_back(tkNUMBER, current);
                last_was_op = false;
                current = "";
                state = tkNONE;
            }
            break;
        case tkWORD:
            if (__is_alpha_num__(c)) {
                current += c;
            } else {
                result.emplace_back(current);
                last_was_op = false;
                current = "";
                state = tkNONE;
            }
        default:
            break;
        }

        switch (state) {
        case tkNONE:
            if (__is_digit___(c)) {
                state = tkNUMBER;
                current += c;
            } else if (__is_alpha__(c)) {
                state = tkWORD;
                current += c;
            } else if (__is_op__(c)) {
                current += c;
                if (last_was_op && (c == '+' || c == '-')) {
                    result.emplace_back(tkPREFIX_OP, c);
                } else {
                    result.emplace_back(tkOP, c);
                    last_was_op = true;
                }
                current = "";
            } else if (c == '(') {
                current += c;
                result.emplace_back(tkOPEN, current);
                current = "";
                last_was_op = true;
            } else if (c == ')') {
                current += c;
                result.emplace_back(tkCLOSE, current);
                current = "";
                last_was_op = false;
            } else if (c == ',') {
                current += c;
                result.emplace_back(tkCOMMA, current);
                current = "";
                last_was_op = true;
            } else if (c == ' ') {

            } else {
                current += c;
                result.emplace_back(tkERROR, current);
                current = "";
                last_was_op = false;
            }
        default:
            break;
        }
    }

    if (!current.is_empty()) {
        switch (state) {
        case tkNUMBER:
            result.emplace_back(tkNUMBER, current);
            break;
        case tkWORD: {
            result.emplace_back(current);
        } break;
        default:
            break;
        }
    }

    return result;
}

String godot::build_string_from_tokens(const std::vector<GDToken>& tokens)
{
    String result = "";
    for (auto& t : tokens) {
        switch (t.kind) {
        case tkNUMBER:
        case tkVAR:
        case tkFUNC:
        case tkOP:
        case tkWORD:
        case tkOPEN:
        case tkCLOSE:
        case tkPREFIX_OP:
        case tkCOMMA: {
            result += t.raw;
        } break;
        }
    }
    return result;
}

bool godot::gd_operator_precedes(GDToken op1, GDToken op2)
{
    if (op1.kind == tkVAR) {
        return true;
    }
    if (("." == op1.raw || "^" == op1.raw) && ("*" == op2.raw || "/" == op2.raw)) {
        return true;
    }
    if (("." == op1.raw || "^" == op1.raw || "*" == op1.raw || "/" == op1.raw) && ("+" == op2.raw || "-" == op2.raw)) {
        return true;
    }

    // handle left association
    if (("*" == op1.raw || "/" == op1.raw) && ("*" == op2.raw || "/" == op2.raw)) {
        return true;
    }
    if (("+" == op1.raw || "-" == op1.raw) && ("+" == op2.raw || "-" == op2.raw)) {
        return true;
    }

    return false;
}

int godot::gd_operator_precedence(GDToken op)
{

    if (op.kind == tkPREFIX_OP) {
        return 999;
    }
    if (op.raw == "#") {
        return 1000;
    }
    if (op.raw == ".") {
        return 110;
    }
    if (op.raw == "^") {
        return 100;
    }
    if (op.raw == "*" || op.raw == "/") {
        return 90;
    }
    if (op.raw == "+" || op.raw == "-") {
        return 80;
    }
    if (op.raw == ",") {
        return 1;
    }

    return 0;
}

bool godot::gd_operator_is_right_associative(GDToken op)
{
    if (op.raw == "^") {
        return true;
    }

    return false;
}

bool godot::siseq(String a, const char* other)
{
    return a == other;
}

GDTokenOpKind godot::__op_kind_from_char___(char32_t c)
{
    if (c == '+') {
        return tkopPLUS;
    } else if (c == '-') {
        return tkopMINUS;
    } else if (c == '/') {
        return tkopDIV;
    } else if (c == '*') {
        return tkopMULT;
    } else if (c == '.') {
        return tkopDOT;
    }
    return tkopNONE;
}

GDTokenFuncKind godot::__func_kind_from_string___(String str)
{
    if (siseq(str, "segment5"))
        return tkfn_segment5;
    if (siseq(str, "segment4"))
        return tkfn_segment4;
    if (siseq(str, "rot_x"))
        return tkfn_rot_x;
    if (siseq(str, "rot_y"))
        return tkfn_rot_y;
    if (siseq(str, "rot_z"))
        return tkfn_rot_z;
    if (siseq(str, "segment3"))
        return tkfn_segment3;
    if (siseq(str, "dot3"))
        return tkfn_dot3;
    if (siseq(str, "cross_x"))
        return tkfn_cross_x;
    if (siseq(str, "cross_y"))
        return tkfn_cross_y;
    if (siseq(str, "cross_z"))
        return tkfn_cross_z;
    if (siseq(str, "proj_x"))
        return tkfn_proj_x;
    if (siseq(str, "proj_y"))
        return tkfn_proj_y;
    if (siseq(str, "proj_z"))
        return tkfn_proj_z;
    if (siseq(str, "cubic"))
        return tkfn_cubic;
    if (siseq(str, "quad"))
        return tkfn_quad;
    if (siseq(str, "segment2"))
        return tkfn_segment2;
    if (siseq(str, "dot2"))
        return tkfn_dot2;
    if (siseq(str, "rot"))
        return tkfn_rot;
    if (siseq(str, "vec"))
        return tkfn_vec;
    if (siseq(str, "len3"))
        return tkfn_len3;
    if (siseq(str, "perp_x"))
        return tkfn_perp_x;
    if (siseq(str, "perp_y"))
        return tkfn_perp_y;
    if (siseq(str, "perp_z"))
        return tkfn_perp_z;
    if (siseq(str, "if"))
        return tkfn_if;
    if (siseq(str, "clamp"))
        return tkfn_clamp;
    if (siseq(str, "lerp"))
        return tkfn_lerp;
    if (siseq(str, "unit_x"))
        return tkfn_unit_x;
    if (siseq(str, "unit_y"))
        return tkfn_unit_y;
    if (siseq(str, "unit_z"))
        return tkfn_unit_z;
    if (siseq(str, "cross"))
        return tkfn_cross;
    if (siseq(str, "dot"))
        return tkfn_dot;
    if (siseq(str, "proj"))
        return tkfn_proj;
    if (siseq(str, "max"))
        return tkfn_max;
    if (siseq(str, "min"))
        return tkfn_min;
    if (siseq(str, "lt"))
        return tkfn_lt;
    if (siseq(str, "gt"))
        return tkfn_gt;
    if (siseq(str, "lte"))
        return tkfn_lte;
    if (siseq(str, "gte"))
        return tkfn_gte;
    if (siseq(str, "eq"))
        return tkfn_eq;
    if (siseq(str, "neq"))
        return tkfn_neq;
    if (siseq(str, "atan2"))
        return tkfn_atan2;
    if (siseq(str, "mod"))
        return tkfn_mod;
    if (siseq(str, "div"))
        return tkfn_div;
    if (siseq(str, "pow"))
        return tkfn_pow;
    if (siseq(str, "len2"))
        return tkfn_len2;
    if (siseq(str, "unit"))
        return tkfn_unit;
    if (siseq(str, "len"))
        return tkfn_len;
    if (siseq(str, "perp"))
        return tkfn_perp;
    if (siseq(str, "abs"))
        return tkfn_abs;
    if (siseq(str, "sqrt"))
        return tkfn_sqrt;
    if (siseq(str, "cbrt"))
        return tkfn_cbrt;
    if (siseq(str, "sqr"))
        return tkfn_sqr;
    if (siseq(str, "cube"))
        return tkfn_cube;
    if (siseq(str, "inv"))
        return tkfn_inv;
    if (siseq(str, "floor"))
        return tkfn_floor;
    if (siseq(str, "ceil"))
        return tkfn_ceil;
    if (siseq(str, "round"))
        return tkfn_round;
    if (siseq(str, "not"))
        return tkfn_not;
    if (siseq(str, "log10"))
        return tkfn_log10;
    if (siseq(str, "logN"))
        return tkfn_logN;
    if (siseq(str, "sinh"))
        return tkfn_sinh;
    if (siseq(str, "cosh"))
        return tkfn_cosh;
    if (siseq(str, "tanh"))
        return tkfn_tanh;
    if (siseq(str, "asinh"))
        return tkfn_asinh;
    if (siseq(str, "acosh"))
        return tkfn_acosh;
    if (siseq(str, "atanh"))
        return tkfn_atanh;
    if (siseq(str, "sin"))
        return tkfn_sin;
    if (siseq(str, "cos"))
        return tkfn_cos;
    if (siseq(str, "tan"))
        return tkfn_tan;
    if (siseq(str, "asin"))
        return tkfn_asin;
    if (siseq(str, "acos"))
        return tkfn_acos;
    if (siseq(str, "atan"))
        return tkfn_atan;
    return tkfnNONE;
}

GDTokenScalarVarKind godot::__var_scalar_kind_from_string___(String str)
{
    if (siseq(str, "r0"))
        return tkv_r0;
    if (siseq(str, "r1"))
        return tkv_r1;
    if (siseq(str, "r2"))
        return tkv_r2;
    if (siseq(str, "r3"))
        return tkv_r3;
    if (siseq(str, "r4"))
        return tkv_r4;
    if (siseq(str, "r5"))
        return tkv_r5;
    if (siseq(str, "r6"))
        return tkv_r6;
    if (siseq(str, "r7"))
        return tkv_r7;
    if (siseq(str, "r8"))
        return tkv_r8;
    if (siseq(str, "r9"))
        return tkv_r9;
    if (siseq(str, "pi"))
        return tkv_pi;
    if (siseq(str, "N"))
        return tkv_N;
    if (siseq(str, "M"))
        return tkv_M;
    if (siseq(str, "C"))
        return tkv_C;
    if (siseq(str, "L"))
        return tkv_L;
    if (siseq(str, "T"))
        return tkv_T;
    if (siseq(str, "P"))
        return tkv_P;
    if (siseq(str, "CR"))
        return tkv_CR;
    if (siseq(str, "CD"))
        return tkv_CD;
    if (siseq(str, "x"))
        return tkv_x;
    if (siseq(str, "y"))
        return tkv_y;
    if (siseq(str, "z"))
        return tkv_z;
    if (siseq(str, "r"))
        return tkv_r;
    if (siseq(str, "rn0"))
        return tkv_rn0;
    if (siseq(str, "rn1"))
        return tkv_rn1;
    if (siseq(str, "rn2"))
        return tkv_rn2;
    if (siseq(str, "rn3"))
        return tkv_rn3;
    if (siseq(str, "rn4"))
        return tkv_rn4;
    if (siseq(str, "rn5"))
        return tkv_rn5;
    if (siseq(str, "rn6"))
        return tkv_rn6;
    if (siseq(str, "rn7"))
        return tkv_rn7;
    if (siseq(str, "rn8"))
        return tkv_rn8;
    if (siseq(str, "rn9"))
        return tkv_rn9;
    if (siseq(str, "n"))
        return tkv_n;
    if (siseq(str, "D"))
        return tkv_D;
    if (siseq(str, "spinrate"))
        return tkv_spinrate;
    if (siseq(str, "t"))
        return tkv_t;
    if (siseq(str, "l"))
        return tkv_l;
    if (siseq(str, "fl"))
        return tkv_fl;
    if (siseq(str, "Bx"))
        return tkv_Bx;
    if (siseq(str, "By"))
        return tkv_By;
    if (siseq(str, "Bz"))
        return tkv_Bz;
    if (siseq(str, "Br"))
        return tkv_Br;
    if (siseq(str, "tC"))
        return tkv_tC;
    if (siseq(str, "TC"))
        return tkv_TC;
    if (siseq(str, "u"))
        return tkv_u;
    if (siseq(str, "v"))
        return tkv_v;
    if (siseq(str, "w"))
        return tkv_w;
    if (siseq(str, "tu"))
        return tkv_tu;
    if (siseq(str, "tv"))
        return tkv_tv;
    if (siseq(str, "tw"))
        return tkv_tw;
    if (siseq(str, "Tu"))
        return tkv_Tu;
    if (siseq(str, "Tv"))
        return tkv_Tv;
    if (siseq(str, "Tw"))
        return tkv_Tw;
    if (siseq(str, "ru"))
        return tkv_ru;
    if (siseq(str, "rv"))
        return tkv_rv;
    if (siseq(str, "rw"))
        return tkv_rw;
    if (siseq(str, "tru"))
        return tkv_tru;
    if (siseq(str, "trv"))
        return tkv_trv;
    if (siseq(str, "trw"))
        return tkv_trw;
    if (siseq(str, "Tru"))
        return tkv_Tru;
    if (siseq(str, "Trv"))
        return tkv_Trv;
    if (siseq(str, "Trw"))
        return tkv_Trw;
    if (siseq(str, "U"))
        return tkv_U;
    if (siseq(str, "V"))
        return tkv_V;
    if (siseq(str, "W"))
        return tkv_W;
    if (siseq(str, "tU"))
        return tkv_tU;
    if (siseq(str, "tV"))
        return tkv_tV;
    if (siseq(str, "tW"))
        return tkv_tW;
    if (siseq(str, "TU"))
        return tkv_TU;
    if (siseq(str, "TV"))
        return tkv_TV;
    if (siseq(str, "TW"))
        return tkv_TW;
    if (siseq(str, "rU"))
        return tkv_rU;
    if (siseq(str, "rV"))
        return tkv_rV;
    if (siseq(str, "rW"))
        return tkv_rW;
    if (siseq(str, "trU"))
        return tkv_trU;
    if (siseq(str, "trV"))
        return tkv_trV;
    if (siseq(str, "trW"))
        return tkv_trW;
    if (siseq(str, "TrU"))
        return tkv_TrU;
    if (siseq(str, "TrV"))
        return tkv_TrV;
    if (siseq(str, "TrW"))
        return tkv_TrW;
    if (siseq(str, "i"))
        return tkv_i;
    if (siseq(str, "j"))
        return tkv_j;
    if (siseq(str, "k"))
        return tkv_k;
    if (siseq(str, "ti"))
        return tkv_ti;
    if (siseq(str, "tj"))
        return tkv_tj;
    if (siseq(str, "tk"))
        return tkv_tk;
    if (siseq(str, "Ti"))
        return tkv_Ti;
    if (siseq(str, "Tj"))
        return tkv_Tj;
    if (siseq(str, "Tk"))
        return tkv_Tk;
    if (siseq(str, "ri"))
        return tkv_ri;
    if (siseq(str, "rj"))
        return tkv_rj;
    if (siseq(str, "rk"))
        return tkv_rk;
    if (siseq(str, "tri"))
        return tkv_tri;
    if (siseq(str, "trj"))
        return tkv_trj;
    if (siseq(str, "trk"))
        return tkv_trk;
    if (siseq(str, "Tri"))
        return tkv_Tri;
    if (siseq(str, "Trj"))
        return tkv_Trj;
    if (siseq(str, "Trk"))
        return tkv_Trk;
    if (siseq(str, "I"))
        return tkv_I;
    if (siseq(str, "J"))
        return tkv_J;
    if (siseq(str, "K"))
        return tkv_K;
    if (siseq(str, "tI"))
        return tkv_tI;
    if (siseq(str, "tJ"))
        return tkv_tJ;
    if (siseq(str, "tK"))
        return tkv_tK;
    if (siseq(str, "TI"))
        return tkv_TI;
    if (siseq(str, "TJ"))
        return tkv_TJ;
    if (siseq(str, "TK"))
        return tkv_TK;
    if (siseq(str, "rI"))
        return tkv_rI;
    if (siseq(str, "rJ"))
        return tkv_rJ;
    if (siseq(str, "rK"))
        return tkv_rK;
    if (siseq(str, "trI"))
        return tkv_trI;
    if (siseq(str, "trJ"))
        return tkv_trJ;
    if (siseq(str, "trK"))
        return tkv_trK;
    if (siseq(str, "TrI"))
        return tkv_TrI;
    if (siseq(str, "TrJ"))
        return tkv_TrJ;
    if (siseq(str, "TrK"))
        return tkv_TrK;
    if (siseq(str, "~~frame_time"))
        return tkv_frame_time;

    return tkvNONE;
}

GDTokenVecVarKind godot::__var_vec_kind_from_string___(String str)
{
    if (siseq(str, "Bxyz"))
        return tkvv_Bxyz;
    if (siseq(str, "uvw"))
        return tkvv_uvw;
    if (siseq(str, "tuvw"))
        return tkvv_tuvw;
    if (siseq(str, "Tuvw"))
        return tkvv_Tuvw;
    if (siseq(str, "ruvw"))
        return tkvv_ruvw;
    if (siseq(str, "truvw"))
        return tkvv_truvw;
    if (siseq(str, "Truvw"))
        return tkvv_Truvw;
    if (siseq(str, "UVW"))
        return tkvv_UVW;
    if (siseq(str, "tUVW"))
        return tkvv_tUVW;
    if (siseq(str, "TUVW"))
        return tkvv_TUVW;
    if (siseq(str, "rUVW"))
        return tkvv_rUVW;
    if (siseq(str, "trUVW"))
        return tkvv_trUVW;
    if (siseq(str, "TrUVW"))
        return tkvv_TrUVW;
    if (siseq(str, "ijk"))
        return tkvv_ijk;
    if (siseq(str, "tijk"))
        return tkvv_tijk;
    if (siseq(str, "Tijk"))
        return tkvv_Tijk;
    if (siseq(str, "rijk"))
        return tkvv_rijk;
    if (siseq(str, "trijk"))
        return tkvv_trijk;
    if (siseq(str, "Trijk"))
        return tkvv_Trijk;
    if (siseq(str, "IJK"))
        return tkvv_IJK;
    if (siseq(str, "tIJK"))
        return tkvv_tIJK;
    if (siseq(str, "TIJK"))
        return tkvv_TIJK;
    if (siseq(str, "rIJK"))
        return tkvv_rIJK;
    if (siseq(str, "trIJK"))
        return tkvv_trIJK;
    if (siseq(str, "TrIJK"))
        return tkvv_TrIJK;
    if (siseq(str, "~~old_pos"))
        return tkvv_old_pos;
    if (siseq(str, "~~rel_pos"))
        return tkvv_rel_pos;
    if (siseq(str, "position"))
        return tkvv_abs_pos;

    return tkvvNONE;
}