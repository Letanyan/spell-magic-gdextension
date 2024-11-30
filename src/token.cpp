#include "token.h"

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <vector>

using namespace godot;

GDTokenFuncKind __func_kind_from_string___(String str)
{
    if (str == "segment5")
        return tkfn_segment5;
    if (str == "segment4")
        return tkfn_segment4;
    if (str == "rot_x")
        return tkfn_rot_x;
    if (str == "rot_y")
        return tkfn_rot_y;
    if (str == "rot_z")
        return tkfn_rot_z;
    if (str == "segment3")
        return tkfn_segment3;
    if (str == "dot3")
        return tkfn_dot3;
    if (str == "cross_x")
        return tkfn_cross_x;
    if (str == "cross_y")
        return tkfn_cross_y;
    if (str == "cross_z")
        return tkfn_cross_z;
    if (str == "proj_x")
        return tkfn_proj_x;
    if (str == "proj_y")
        return tkfn_proj_y;
    if (str == "proj_z")
        return tkfn_proj_z;
    if (str == "cubic")
        return tkfn_cubic;
    if (str == "quad")
        return tkfn_quad;
    if (str == "segment2")
        return tkfn_segment2;
    if (str == "dot2")
        return tkfn_dot2;
    if (str == "rot")
        return tkfn_rot;
    if (str == "vec")
        return tkfn_vec;
    if (str == "len3")
        return tkfn_len3;
    if (str == "perp_x")
        return tkfn_perp_x;
    if (str == "perp_y")
        return tkfn_perp_y;
    if (str == "perp_z")
        return tkfn_perp_z;
    if (str == "if")
        return tkfn_if;
    if (str == "clamp")
        return tkfn_clamp;
    if (str == "lerp")
        return tkfn_lerp;
    if (str == "unit_x")
        return tkfn_unit_x;
    if (str == "unit_y")
        return tkfn_unit_y;
    if (str == "unit_z")
        return tkfn_unit_z;
    if (str == "cross")
        return tkfn_cross;
    if (str == "dot")
        return tkfn_dot;
    if (str == "proj")
        return tkfn_proj;
    if (str == "max")
        return tkfn_max;
    if (str == "min")
        return tkfn_min;
    if (str == "lt")
        return tkfn_lt;
    if (str == "gt")
        return tkfn_gt;
    if (str == "lte")
        return tkfn_lte;
    if (str == "gte")
        return tkfn_gte;
    if (str == "eq")
        return tkfn_eq;
    if (str == "neq")
        return tkfn_neq;
    if (str == "atan2")
        return tkfn_atan2;
    if (str == "mod")
        return tkfn_mod;
    if (str == "div")
        return tkfn_div;
    if (str == "pow")
        return tkfn_pow;
    if (str == "len2")
        return tkfn_len2;
    if (str == "unit")
        return tkfn_unit;
    if (str == "len")
        return tkfn_len;
    if (str == "perp")
        return tkfn_perp;
    if (str == "abs")
        return tkfn_abs;
    if (str == "sqrt")
        return tkfn_sqrt;
    if (str == "cbrt")
        return tkfn_cbrt;
    if (str == "sqr")
        return tkfn_sqr;
    if (str == "cube")
        return tkfn_cube;
    if (str == "inv")
        return tkfn_inv;
    if (str == "floor")
        return tkfn_floor;
    if (str == "ceil")
        return tkfn_ceil;
    if (str == "round")
        return tkfn_round;
    if (str == "not")
        return tkfn_not;
    if (str == "log10")
        return tkfn_log10;
    if (str == "logN")
        return tkfn_logN;
    if (str == "sinh")
        return tkfn_sinh;
    if (str == "cosh")
        return tkfn_cosh;
    if (str == "tanh")
        return tkfn_tanh;
    if (str == "asinh")
        return tkfn_asinh;
    if (str == "acosh")
        return tkfn_acosh;
    if (str == "atanh")
        return tkfn_atanh;
    if (str == "sin")
        return tkfn_sin;
    if (str == "cos")
        return tkfn_cos;
    if (str == "tan")
        return tkfn_tan;
    if (str == "asin")
        return tkfn_asin;
    if (str == "acos")
        return tkfn_acos;
    if (str == "atan")
        return tkfn_atan;
    return tkfn_NONE;
}

GDTokenOpKind __op_kind_from_char___(char32_t c)
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

GDToken::GDToken()
{
    kind = tkNONE;
    sub_kind = -1;
    raw = "";
}

GDToken::GDToken(GDTokenKind _kind, godot::String _raw)
{
    kind = _kind;
    raw = _raw;
    if (kind == tkFUNC) {
        sub_kind = __func_kind_from_string___(_raw);
    } else {
        sub_kind = -1;
    }
}

GDToken::GDToken(GDTokenKind _kind, GDTokenFuncKind _sub_kind, godot::String _raw)
{
    kind = _kind;
    raw = _raw;
    sub_kind = _sub_kind;
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
                GDTokenFuncKind sk = __func_kind_from_string___(current);
                GDTokenKind k = sk == tkfn_NONE ? tkVAR : tkFUNC;
                result.emplace_back(k, sk, current);
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
            GDTokenFuncKind sk = __func_kind_from_string___(current);
            GDTokenKind k = sk == tkfn_NONE ? tkVAR : tkFUNC;
            result.emplace_back(k, sk, current);
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