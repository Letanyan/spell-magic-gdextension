#include "expr.h"
#include "expr_optimizer.h"
#include "my_profiler.h"
#include "token.h"
#include <math.h>

#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

void GDExpr::_bind_methods()
{
    ClassDB::bind_static_method("GDExpr", D_METHOD("bake", "expr", "map"), &GDExpr::bake);

    ClassDB::bind_method(D_METHOD("build_in", "expr"), &GDExpr::build_in);
    ClassDB::bind_method(D_METHOD("copy_from", "expr"), &GDExpr::copy_from);

    ClassDB::bind_method(D_METHOD("build", "expression"), &GDExpr::build);
    ClassDB::bind_method(D_METHOD("compute", "variables", "user_funcs"), &GDExpr::compute);
    ClassDB::bind_method(D_METHOD("contains_variable", "variable_name"), &GDExpr::contains_variable);

    ClassDB::bind_method(D_METHOD("get_error"), &GDExpr::get_error);
    ClassDB::bind_method(D_METHOD("set_error", "error_message"), &GDExpr::set_error);
    ClassDB::add_property("GDExpr", PropertyInfo(Variant::STRING, "error"), "set_error", "get_error");
}

String GDExpr::get_error()
{
    return error;
}

void GDExpr::set_error(String err_message)
{
    error = err_message;
}

GDExpr::GDExpr()
{
}

GDExpr::~GDExpr()
{
}

GDExpr* GDExpr::build_in(godot::String expr)
{
    this->build(expr);
    return this;
}

void GDExpr::build(godot::String expr)
{
    auto tokens = godot::tokenize(expr);
    build_from_tokens(tokens);
}

void GDExpr::build_from_tokens(std::vector<GDToken> tokens)
{
    error = "";
    expression = std::vector<GDToken>(tokens.size());
    auto operators = std::vector<GDToken>(tokens.size());
    int i = 0;
    while (i < tokens.size()) {
        auto token = tokens[i];
        i += 1;

        if (token.kind == tkNUMBER) {
            expression.push_back(token);
        } else if (token.kind == tkFUNC || token.kind == tkVAR) {
            operators.push_back(token);
        } else if (token.kind == tkOP) {
            if (operators.size() > 0) {
                auto op2 = operators.back();
                while (op2.kind != tkOPEN && gd_operator_precedes(op2, token)) {
                    expression.push_back(op2);
                    operators.pop_back();
                    if (operators.size() > 0) {
                        op2 = operators.back();
                    } else {
                        break;
                    }
                }
            }
            operators.push_back(token);
        } else if (token.kind == tkPREFIX_OP) {
            operators.push_back(token);
        } else if (token.kind == tkOPEN) {
            operators.push_back(token);
        } else if (token.kind == tkCLOSE) {
            if (operators.size() > 0) {
                auto op2 = operators.back();
                while (op2.kind != tkOPEN) {
                    expression.push_back(op2);
                    operators.pop_back();
                    if (operators.size() > 0) {
                        op2 = operators.back();
                    } else {
                        break;
                    }
                }
                if (operators.size() <= 0) {
                    error = "Missing Opening Paren";
                    expression.push_back(GDToken(tkERROR, "Missing Opening Paren"));
                    return;
                }
                operators.pop_back();
                if (operators.size() > 0 && (operators.back().kind == tkFUNC || operators.back().kind == tkVAR)) {
                    auto op = operators.back();
                    operators.pop_back();
                    expression.push_back(op);
                }
            } else {
                error = "Missing Opening Paren";
                expression.push_back(GDToken(tkERROR, "Missing Opening Paren"));
                return;
            }
        } else if (token.kind == tkCOMMA) {
            if (operators.size() > 0) {
                auto op = operators.back();
                while (op.kind != tkOPEN) {
                    expression.push_back(op);
                    operators.pop_back();
                    if (operators.size() > 0) {
                        op = operators.back();
                    } else {
                        break;
                    }
                }
            }
        }
    }

    while (operators.size() > 0) {
        auto op = operators.back();
        operators.pop_back();
        if (op.kind == tkOPEN) {
            error = "Missing Closing Paren";
            expression.push_back(GDToken(tkERROR, "Missing Closing Paren"));
            return;
        }
        expression.push_back(op);
    }
}

double clerpf(double a, double b, double t)
{
    return UtilityFunctions::lerpf(a, b, UtilityFunctions::clampf(t, 0.0, 1.0));
}

double GDExpr::compute(Dictionary map, Dictionary user_funcs)
{
    // parameters are in reverse order. For example the first popped var is the last parameter:
    // f(..., z, ..., c, b, a)
#define POP_VAR(name, error_message) \
    tape_index -= 1;                 \
    if (tape_index < 0) {            \
        error = error_message;       \
        return 0.0;                  \
    }                                \
    auto name = tape[tape_index];

    auto tape = std::vector<float>(8);
    long long tape_index = 0;
    for (auto& e : expression) {
        if (e.kind == tkERROR) {
            error = e.raw;
            return 0.0;
        }

        if (e.kind == tkNUMBER) {
            if (tape_index == tape.size()) {
                tape.push_back(e.raw.to_float());
            } else {
                tape[tape_index] = e.raw.to_float();
            }
            tape_index += 1;
        } else if (e.kind == tkVAR) {
            float value = 0.0;
            if (map.has(e.raw)) {
                value = map.get(e.raw, 0.0);
            } else if (user_funcs.has(e.raw)) {
                auto func = (Dictionary)user_funcs[e.raw];
                auto args = (PackedStringArray)func[String("args")];
                auto expr = (GDExpr*)(Object*)func[String("expr")];
                auto var_maps = Dictionary();
                for (auto v : args) {
                    POP_VAR(x, e.raw + " requires " + UtilityFunctions::str(args.size()) + " parameters")
                    var_maps[v] = x;
                }
                value = expr->compute(var_maps, user_funcs);
                if (std::isnan(value)) {
                    value = 0.0;
                }
            }
            if (tape_index == tape.size()) {
                tape.push_back(value);
            } else {
                tape[tape_index] = value;
            }
            tape_index += 1;
        } else if (e.kind == tkOP) {
            tape_index -= 1;
            if (tape_index < 0) {
                error = "Incomplete Expression";
                return 0.0;
            }
            auto b = tape[tape_index];
            tape_index -= 1;
            if (tape_index < 0) {
                error = "Incomplete Expression";
                return 0.0;
            }
            auto a = tape[tape_index];
            double value = 0.0;
            if (e.raw == "+") {
                value = a + b;
            } else if (e.raw == "-") {
                value = a - b;
            } else if (e.raw == "*") {
                value = a * b;
            } else if (e.raw == "/") {
                if (b == 0.0) {
                    value = 0.0;
                } else {
                    value = a / b;
                }
            } else if (e.raw == "^") {
                value = pow(a, b);
            }
            if (std::isnan(value)) {
                value = 0.0;
            }
            if (tape_index == tape.size()) {
                tape.push_back(value);
            } else {
                tape[tape_index] = value;
            }
            tape_index += 1;
        } else if (e.kind == tkPREFIX_OP) {
            tape_index -= 1;
            if (tape_index < 0) {
                error = "Incomplete Expression";
                return 0.0;
            }
            auto a = tape[tape_index];
            double value = a;
            if (e.raw == "-") {
                value = -a;
            }
            if (std::isnan(value)) {
                value = 0.0;
            }
            if (tape_index == tape.size()) {
                tape.push_back(value);
            } else {
                tape[tape_index] = value;
            }
            tape_index += 1;
        } else if (e.kind == tkFUNC) {
            tape_index -= 1;
            if (tape_index < 0) {
                error = e.raw + " requires at least 1 parameter";
                return 0.0;
            }
            auto a = tape[tape_index];
            double value = 0.0;
            if (e.raw == "sin") {
                value = sin(a);
            } else if (e.raw == "cos") {
                value = cos(a);
            } else if (e.raw == "tan") {
                value = tan(a);
            } else if (e.raw == "sinh") {
                value = sinh(a);
            } else if (e.raw == "cosh") {
                value = cosh(a);
            } else if (e.raw == "tanh") {
                value = tanh(a);
            } else if (e.raw == "asin") {
                value = asin(a);
            } else if (e.raw == "acos") {
                value = acos(a);
            } else if (e.raw == "atan") {
                value = atan(a);
            } else if (e.raw == "asinh") {
                value = asinh(a);
            } else if (e.raw == "acosh") {
                value = acosh(a);
            } else if (e.raw == "atanh") {
                value = atanh(a);
            } else if (e.raw == "inv") {
                value = a != 0 ? (1 / a) : 0;
            } else if (e.raw == "mod") {
                POP_VAR(b, "mod requires 2 parameters")
                value = a != 0 ? fmod(b, a) : 0;
            } else if (e.raw == "div") {
                POP_VAR(b, "div requires 2 parameters")
                value = a != 0 ? floor(b / a) : 0;
            } else if (e.raw == "floor") {
                value = floor(a);
            } else if (e.raw == "ceil") {
                value = ceil(a);
            } else if (e.raw == "round") {
                value = round(a);
            } else if (e.raw == "max") {
                POP_VAR(b, "max requires 2 parameters")
                value = a < b ? b : a;
            } else if (e.raw == "min") {
                POP_VAR(b, "min requires 2 parameters")
                value = a < b ? a : b;
            } else if (e.raw == "lt") {
                value = a < 0 ? 1 : 0;
            } else if (e.raw == "gt") {
                value = a > 0 ? 1 : 0;
            } else if (e.raw == "lte") {
                value = a <= 0 ? 1 : 0;
            } else if (e.raw == "gte") {
                value = a >= 0 ? 1 : 0;
            } else if (e.raw == "eq") {
                value = a == 0 ? 1 : 0;
            } else if (e.raw == "neq") {
                value = a != 0 ? 1 : 0;
            } else if (e.raw == "pow") {
                POP_VAR(b, "pow requires 2 parameters")
                value = pow(b, a);
            } else if (e.raw == "log10") {
                value = log10f(a);
            } else if (e.raw == "logN") {
                value = log(a);
            } else if (e.raw == "abs") {
                value = abs(a);
            } else if (e.raw == "sqrt") {
                value = sqrtf(a);
            } else if (e.raw == "cbrt") {
                value = powf(a, 1.0 / 3.0);
            } else if (e.raw == "sqr") {
                value = a * a;
            } else if (e.raw == "cube") {
                value = a * a * a;
            } else if (e.raw == "lerp") {
                POP_VAR(b, "lerp requires 3 parameters")
                POP_VAR(c, "lerp requires 3 parameters")
                value = UtilityFunctions::lerpf(b, a, c);
            } else if (e.raw == "if") {
                POP_VAR(b, "if requires 3 parameters")
                POP_VAR(c, "if requires 3 parameters")
                value = c != 0.0 ? b : a;
            } else if (e.raw == "clamp") {
                POP_VAR(b, "clamp requires 3 parameters")
                POP_VAR(c, "clamp requires 3 parameters")
                if (c < b) {
                    value = b;
                } else if (c > a) {
                    value = a;
                } else {
                    value = c;
                }
            } else if (e.raw == "quad") {
                POP_VAR(b, "quad requires 4 parameters")
                POP_VAR(c, "quad requires 4 parameters")
                POP_VAR(d, "quad requires 4 parameters")

                float x1 = UtilityFunctions::lerpf(c, b, d);
                float x2 = UtilityFunctions::lerpf(b, a, d);
                value = UtilityFunctions::lerpf(x1, x2, d);
            } else if (e.raw == "cubic") {
                POP_VAR(b, "cubic requires 5 parameters")
                POP_VAR(c, "cubic requires 5 parameters")
                POP_VAR(d, "cubic requires 5 parameters")
                POP_VAR(e, "cubic requires 5 parameters")

                float x1 = UtilityFunctions::lerpf(d, c, e);
                float y1 = UtilityFunctions::lerpf(c, a, e);
                float z1 = UtilityFunctions::lerpf(x1, y1, e);
                float x2 = UtilityFunctions::lerpf(c, b, e);
                float y2 = UtilityFunctions::lerpf(b, a, e);
                float z2 = UtilityFunctions::lerpf(x2, y2, e);
                value = UtilityFunctions::lerpf(z1, z2, e);
            } else if (e.raw == "segment2") {
                POP_VAR(b, "segment2 requires 4 parameters")
                POP_VAR(c, "segment2 requires 4 parameters")
                POP_VAR(d, "segment2 requires 4 parameters")
                value = clerpf(b, c, d / a);
            } else if (e.raw == "segment3") {
                POP_VAR(b, "segment3 requires 6 parameters")
                POP_VAR(c, "segment3 requires 6 parameters")
                POP_VAR(d, "segment3 requires 6 parameters")
                POP_VAR(e, "segment3 requires 6 parameters")
                POP_VAR(f, "segment3 requires 6 parameters")

                // f=t, e d c, b=d1, a=d2
                if (f <= b) {
                    value = clerpf(e, d, f / b);
                } else {
                    value = clerpf(d, c, (f - b) / a);
                }
            } else if (e.raw == "segment4") {
                POP_VAR(b, "segment4 requires 8 parameters")
                POP_VAR(c, "segment4 requires 8 parameters")
                POP_VAR(d, "segment4 requires 8 parameters")
                POP_VAR(e, "segment4 requires 8 parameters")
                POP_VAR(f, "segment4 requires 8 parameters")
                POP_VAR(g, "segment4 requires 8 parameters")
                POP_VAR(h, "segment4 requires 8 parameters")

                // h=t, g f e d, c=d1, b=d2, a=d3
                if (h <= c) {
                    value = clerpf(g, f, h / c);
                } else if (h <= c + b) {
                    value = clerpf(f, e, (h - c) / b);
                } else {
                    value = clerpf(e, d, (h - c - b) / a);
                }
            } else if (e.raw == "segment5") {
                POP_VAR(b, "segment5 requires 10 parameters")
                POP_VAR(c, "segment5 requires 10 parameters")
                POP_VAR(d, "segment5 requires 10 parameters")
                POP_VAR(e, "segment5 requires 10 parameters")
                POP_VAR(f, "segment5 requires 10 parameters")
                POP_VAR(g, "segment5 requires 10 parameters")
                POP_VAR(h, "segment5 requires 10 parameters")
                POP_VAR(i, "segment5 requires 10 parameters")
                POP_VAR(j, "segment5 requires 10 parameters")

                // j=t, h i g f e, d=d1, c=d2, b=d3, a=d4
                if (j <= d) {
                    value = clerpf(h, i, j / d);
                } else if (j <= d + c) {
                    value = clerpf(i, g, (j - d) / c);
                } else if (j <= d + c + b) {
                    value = clerpf(g, f, (j - d - c) / b);
                } else {
                    value = clerpf(f, e, (j - d - c - b) / a);
                }
            } else if (e.raw == "dot2") {
                POP_VAR(b, "dot2 requires 4 parameters")
                POP_VAR(c, "dot2 requires 4 parameters")
                POP_VAR(d, "dot2 requires 4 parameters")
                value = Vector2(d, c).dot(Vector2(b, a));
            } else if (e.raw == "dot3") {
                POP_VAR(b, "dot3 requires 6 parameters")
                POP_VAR(c, "dot3 requires 6 parameters")
                POP_VAR(d, "dot3 requires 6 parameters")
                POP_VAR(e, "dot3 requires 6 parameters")
                POP_VAR(f, "dot3 requires 6 parameters")
                value = Vector3(f, e, d).dot(Vector3(c, b, a));
            } else if (e.raw == "cross_x") {
                POP_VAR(b, "cross_x requires 6 parameters")
                POP_VAR(c, "cross_x requires 6 parameters")
                POP_VAR(d, "cross_x requires 6 parameters")
                POP_VAR(e, "cross_x requires 6 parameters")
                POP_VAR(f, "cross_x requires 6 parameters")
                value = Vector3(f, e, d).cross(Vector3(c, b, a)).x;
            } else if (e.raw == "cross_y") {
                POP_VAR(b, "cross_y requires 6 parameters")
                POP_VAR(c, "cross_y requires 6 parameters")
                POP_VAR(d, "cross_y requires 6 parameters")
                POP_VAR(e, "cross_y requires 6 parameters")
                POP_VAR(f, "cross_y requires 6 parameters")
                value = Vector3(f, e, d).cross(Vector3(c, b, a)).y;
            } else if (e.raw == "cross_z") {
                POP_VAR(b, "cross_z requires 6 parameters")
                POP_VAR(c, "cross_z requires 6 parameters")
                POP_VAR(d, "cross_z requires 6 parameters")
                POP_VAR(e, "cross_z requires 6 parameters")
                POP_VAR(f, "cross_z requires 6 parameters")
                value = Vector3(f, e, d).cross(Vector3(c, b, a)).z;
            } else if (e.raw == "proj_x") {
                POP_VAR(b, "proj_x requires 6 parameters")
                POP_VAR(c, "proj_x requires 6 parameters")
                POP_VAR(d, "proj_x requires 6 parameters")
                POP_VAR(e, "proj_x requires 6 parameters")
                POP_VAR(f, "proj_x requires 6 parameters")
                auto n = Vector3(f, e, d).normalized();
                auto v = Vector3(c, b, a);
                value = (v - v.dot(n) * n).x;
            } else if (e.raw == "proj_y") {
                POP_VAR(b, "proj_y requires 6 parameters")
                POP_VAR(c, "proj_y requires 6 parameters")
                POP_VAR(d, "proj_y requires 6 parameters")
                POP_VAR(e, "proj_y requires 6 parameters")
                POP_VAR(f, "proj_y requires 6 parameters")
                auto n = Vector3(f, e, d).normalized();
                auto v = Vector3(c, b, a);
                value = (v - v.dot(n) * n).y;
            } else if (e.raw == "proj_z") {
                POP_VAR(b, "proj_z requires 6 parameters")
                POP_VAR(c, "proj_z requires 6 parameters")
                POP_VAR(d, "proj_z requires 6 parameters")
                POP_VAR(e, "proj_z requires 6 parameters")
                POP_VAR(f, "proj_z requires 6 parameters")
                auto n = Vector3(f, e, d).normalized();
                auto v = Vector3(c, b, a);
                value = (v - v.dot(n) * n).z;
            } else if (e.raw == "unit_x") {
                POP_VAR(b, "unit_x requires 6 parameters")
                POP_VAR(c, "unit_x requires 6 parameters")
                value = Vector3(c, b, a).normalized().x;
            } else if (e.raw == "unit_y") {
                POP_VAR(b, "unit_y requires 6 parameters")
                POP_VAR(c, "unit_y requires 6 parameters")
                value = Vector3(c, b, a).normalized().y;
            } else if (e.raw == "unit_z") {
                POP_VAR(b, "unit_z requires 6 parameters")
                POP_VAR(c, "unit_z requires 6 parameters")
                value = Vector3(c, b, a).normalized().z;
            }
            if (std::isnan(value)) {
                value = 0.0;
            }
            if (tape_index == tape.size()) {
                tape.push_back(value);
            } else {
                tape[tape_index] = value;
            }
            tape_index += 1;
        }
    }

    if (tape.empty()) {
        return 0.0;
    }

    tape_index -= 1;
    if (tape_index < 0) {
        error = "Incomplete Expression";
        return 0.0;
    }
    return tape[tape_index];
}

bool GDExpr::contains_variable(godot::String var_name)
{
    for (auto& e : expression) {
        if (e.kind == tkVAR) {
            if (e.raw == var_name) {
                return true;
            }
        }
    }
    return false;
}

bool vector_contains_string(std::vector<godot::String>* vec, godot::String needle)
{
    for (auto& s : *vec) {
        if (s == needle) {
            return true;
        }
    }
    return false;
}

void bake_into_vector(std::vector<GDToken> tokens, std::vector<GDToken>* result, Dictionary map, std::vector<String>* chain)
{
    auto buffer = std::vector<GDToken>();
    for (auto& e : tokens) {
        if (e.kind == tkVAR) {
            String sub_expr = map.get(e.raw, "");
            if (sub_expr.length() > 0) {
                auto sub_tokens = godot::tokenize(sub_expr);
                if (vector_contains_string(chain, e.raw)) {
                    result->push_back(GDToken(tkNUMBER, "0"));
                } else {
                    chain->push_back(e.raw);
                    bake_into_vector(sub_tokens, &buffer, map, chain);
                    chain->pop_back();
                }
            } else {
                buffer.push_back(e);
            }
        } else {
            buffer.push_back(e);
        }
    }
    bool wrap = chain->size() > 1 && (buffer.size() > 1 && !((buffer[0].kind == tkFUNC || buffer[0].kind == tkOPEN) && buffer.back().kind == tkCLOSE));
    if (wrap) {
        result->push_back(GDToken(tkOPEN, "("));
        for (auto& e : buffer) {
            result->push_back(e);
        }
        result->push_back(GDToken(tkCLOSE, ")"));
    } else {
        for (auto& e : buffer) {
            result->push_back(e);
        }
    }
}

String GDExpr::bake(String expr, Dictionary map)
{
    auto tokens = godot::tokenize(expr);
    auto output = std::vector<GDToken>();
    auto chain = std::vector<String>();
    bake_into_vector(tokens, &output, map, &chain);
    return godot::optimize(output);
    // return godot::build_string_from_tokens(output);
}

void GDExpr::copy_from(GDExpr* expr)
{
    for (auto e : this->expression) {
        expr->expression.push_back(e);
    }
    this->error = expr->error;
}