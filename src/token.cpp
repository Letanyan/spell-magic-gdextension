#include "token.h"

#include <godot_cpp/classes/ref_counted.hpp>
#include <vector>

using namespace godot;

GDToken::GDToken()
{
    kind = tkNONE;
    raw = "";
}

GDToken::GDToken(GDTokenKind _kind, godot::String _raw)
{
    kind = _kind;
    raw = _raw;
}

bool is_func(godot::String name)
{
    if (name == "sin" || name == "cos" || name == "tan" || name == "asin" || name == "acos" || name == "atan" || name == "atan2") {
        return true;
    }
    if (name == "sinh" || name == "cosh" || name == "tanh" || name == "asinh" || name == "acosh" || name == "atanh") {
        return true;
    }
    if (name == "inv" || name == "mod" || name == "div" || name == "floor" || name == "ceil" || name == "round") {
        return true;
    }
    if (name == "max" || name == "min" || name == "lt" || name == "gt" || name == "lte" || name == "gte" || name == "eq" || name == "neq" || name == "not") {
        return true;
    }
    if (name == "lerp" || name == "pow" || name == "log10" || name == "logN" || name == "abs" || name == "sqrt" || name == "cbrt" || name == "sqr" || name == "cube") {
        return true;
    }
    if (name == "if" || name == "clamp" || name == "quad" || name == "cubic") {
        return true;
    }
    // segments work as follows f(a, b1, b2, ..., bn, c1, c2, ..., cn-1).
    // a is the time parameter. b1...bn are the values to interpolate between
    // c1...cn-1 are the durations between each b-pair values. For example
    // f(a, b1, b2, c1) = f(2, 100, 250, 10) = lerp(a/c1, b1, b2) = lerp(2/10, 100, 250)
    // f(a, b1, b2, b3, c1, c2) = f(2, 100, 250, 50, 1, 4) = lerp((a-c1)/c2, b2, b3) = lerp((2-1)/4 250, 50)
    if (name == "segment2" || name == "segment3" || name == "segment4" || name == "segment5") {
        return true;
    }
    if (name == "dot2" || name == "dot3" || name == "cross_x" || name == "cross_y" || name == "cross_z") {
        return true;
    }
    if (name == "proj_x" || name == "proj_y" || name == "proj_z" || name == "unit_x" || name == "unit_y" || name == "unit_z") {
        return true;
    }
    if (name == "rot_x" || name == "rot_y" || name == "rot_z") {
        return true;
    }
    return false;
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
    if (name == "atan2" || name == "mod" || name == "div" || name == "pow") {
        return 2;
    }
    if (name == "max" || name == "min" || name == "lt" || name == "gt" || name == "lte" || name == "gte" || name == "eq" || name == "neq") {
        return 2;
    }
    if (name == "if" || name == "clamp" || name == "lerp" || name == "unit_x" || name == "unit_y" || name == "unit_z") {
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
    return c == '+' || c == '-' || c == '/' || c == '*' || c == '^';
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
                result.push_back(GDToken(tkNUMBER, current));
                last_was_op = false;
                current = "";
                state = tkNONE;
            }
            break;
        case tkWORD:
            if (__is_alpha_num__(c)) {
                current += c;
            } else {
                GDTokenKind k = is_func(current) ? tkFUNC : tkVAR;
                result.push_back(GDToken(k, current));
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
                    result.push_back(GDToken(tkPREFIX_OP, current));
                } else {
                    result.push_back(GDToken(tkOP, current));
                    last_was_op = true;
                }
                current = "";
            } else if (c == '(') {
                current += c;
                result.push_back(GDToken(tkOPEN, current));
                current = "";
                last_was_op = true;
            } else if (c == ')') {
                current += c;
                result.push_back(GDToken(tkCLOSE, current));
                current = "";
                last_was_op = false;
            } else if (c == ',') {
                current += c;
                result.push_back(GDToken(tkCOMMA, current));
                current = "";
                last_was_op = true;
            } else if (c == ' ') {

            } else {
                current += c;
                result.push_back(GDToken(tkERROR, current));
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
            result.push_back(GDToken(tkNUMBER, current));
            break;
        case tkWORD: {
            GDTokenKind k = is_func(current) ? tkFUNC : tkVAR;
            result.push_back(GDToken(k, current));
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
    if ("^" == op1.raw && ("*" == op2.raw || "/" == op2.raw)) {
        return true;
    }
    if (("^" == op1.raw || "*" == op1.raw || "/" == op1.raw) && ("+" == op2.raw || "-" == op2.raw)) {
        return true;
    }

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