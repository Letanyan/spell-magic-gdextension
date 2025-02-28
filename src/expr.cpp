#include "expr.h"
#include "expr_optimizer.h"
#include "token.h"
#include <math.h>

#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

String GDExpr::get_error()
{
    return error;
}

void GDExpr::set_error(const String& err_message)
{
    error = err_message;
}

GDExpr::GDExpr()
{
}

GDExpr::~GDExpr()
{
}

GDExpr* GDExpr::build_in(const godot::String& expr)
{
    this->build(expr);
    return this;
}

void GDExpr::build(const godot::String& expr)
{
    auto tokens = tokenize(expr);
    build_from_tokens(tokens);
}

void GDExpr::build_from_tokens(const std::vector<GDToken>& tokens)
{
    error = "";
    expression = std::vector<GDToken>();
    expression.reserve(tokens.size());
    auto operators = std::vector<GDToken>();
    operators.reserve(tokens.size());
    int i = 0;
    variable_update_set = 0;
    while (i < tokens.size()) {
        auto token = tokens[i];
        i += 1;

        if (token.kind == tkNUMBER) {
            expression.push_back(std::move(token));
        } else if (token.kind == tkFUNC || token.kind == tkVAR) {
            if (token.kind == tkVAR) {
                if (token.sub_kind >= tkv_t && token.sub_kind <= tkv_tC) {
                    contains_time_dependent = true;
                    if (token.sub_kind >= tkv_tu && token.sub_kind <= tkv_tw) {
                        variable_update_set |= 1 << tkvv_tuvw;
                    }
                    if (token.sub_kind >= tkv_tru && token.sub_kind <= tkv_trw) {
                        variable_update_set |= 1 << tkvv_truvw;
                    }
                    if (token.sub_kind >= tkv_tU && token.sub_kind <= tkv_tW) {
                        variable_update_set |= 1 << tkvv_tUVW;
                    }
                    if (token.sub_kind >= tkv_trU && token.sub_kind <= tkv_trW) {
                        variable_update_set |= 1 << tkvv_trUVW;
                    }
                    if (token.sub_kind >= tkv_ti && token.sub_kind <= tkv_tj) {
                        variable_update_set |= 1 << tkvv_tijk;
                    }
                    if (token.sub_kind >= tkv_tri && token.sub_kind <= tkv_trj) {
                        variable_update_set |= 1 << tkvv_trijk;
                    }
                    if (token.sub_kind >= tkv_tI && token.sub_kind <= tkv_tJ) {
                        variable_update_set |= 1 << tkvv_tIJK;
                    }
                    if (token.sub_kind >= tkv_trI && token.sub_kind <= tkv_trJ) {
                        variable_update_set |= 1 << tkvv_trIJK;
                    }
                    if (token.sub_kind == tkv_C) {
                        variable_update_set |= 1 << 8;
                    }
                }
            }
            operators.push_back(std::move(token));
        } else if (token.kind == tkOP) {
            if (operators.size() > 0) {
                auto op2 = operators.back();
                while (op2.kind != tkOPEN && gd_operator_precedes(op2, token)) {
                    expression.push_back(std::move(op2));
                    operators.pop_back();
                    if (operators.size() > 0) {
                        op2 = operators.back();
                    } else {
                        break;
                    }
                }
            }
            operators.push_back(std::move(token));
        } else if (token.kind == tkPREFIX_OP) {
            operators.push_back(std::move(token));
        } else if (token.kind == tkOPEN) {
            operators.push_back(std::move(token));
        } else if (token.kind == tkCLOSE) {
            if (operators.size() > 0) {
                auto op2 = operators.back();
                while (op2.kind != tkOPEN) {
                    expression.push_back(std::move(op2));
                    operators.pop_back();
                    if (operators.size() > 0) {
                        op2 = operators.back();
                    } else {
                        break;
                    }
                }
                if (operators.size() <= 0) {
                    error = "Missing Opening Paren";
                    expression.emplace_back(tkERROR, "Missing Opening Paren");
                    return;
                }
                operators.pop_back();
                if (operators.size() > 0 && (operators.back().kind == tkFUNC || operators.back().kind == tkVAR)) {
                    auto op = operators.back();
                    operators.pop_back();
                    expression.push_back(std::move(op));
                }
            } else {
                error = "Missing Opening Paren";
                expression.emplace_back(tkERROR, "Missing Opening Paren");
                return;
            }
        } else if (token.kind == tkCOMMA) {
            if (operators.size() > 0) {
                auto op = operators.back();
                while (op.kind != tkOPEN) {
                    expression.push_back(std::move(op));
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
            expression.emplace_back(tkERROR, "Missing Closing Paren");
            return;
        }
        expression.push_back(std::move(op));
    }
}

String GDExpr::token_description()
{
    String result = "";
    for (auto tok : expression) {
        result += tok.raw + "(" + UtilityFunctions::str(tok.kind) + ") ";
    }
    return result;
}

double godot::clerpf(double a, double b, double t)
{
    return UtilityFunctions::lerpf(a, b, UtilityFunctions::clampf(t, 0.0, 1.0));
}

Variant GDExpr::compute(const Vars* map, const Dictionary& user_funcs, bool debug)
{
    // parameters are in reverse order. For example the first popped var is the last parameter:
    // f(..., z, ..., c, b, a)
#undef POP_VAR
#define POP_VAR(name, error_message) \
    tape_index -= 1;                 \
    if (tape_index < 0) {            \
        error = error_message;       \
        return 0.0;                  \
    }                                \
    auto name = tape[tape_index];

#undef BINOP
#define BINOP(op)                             \
    if (std::isnan(a) && std::isnan(b)) {     \
        auto va = vector_map[tape_index + 0]; \
        auto vb = vector_map[tape_index + 1]; \
        vans = va op vb;                      \
        value = NAN;                          \
    } else if (std::isnan(a)) {               \
        auto va = vector_map[tape_index + 0]; \
        vans = va op Vector3(b, b, b);        \
        value = NAN;                          \
    } else if (std::isnan(b)) {               \
        auto vb = vector_map[tape_index + 1]; \
        vans = Vector3(a, a, a) op vb;        \
        value = NAN;                          \
    } else {                                  \
        value = a op b;                       \
        if (std::isnan(value)) {              \
            value = 0.0;                      \
        }                                     \
    }

    auto tape = std::vector<float>();
    tape.reserve(8);
    auto vector_map = std::vector<Vector3>();
    vector_map.reserve(8);
    long long tape_index = 0;
    for (auto& e : expression) {
        if (e.kind == tkERROR) {
            error = e.raw;
            return 0.0;
        }

        if (e.kind == tkNUMBER) {
            if (tape_index == tape.size()) {
                float val = e.raw.to_float();
                tape.push_back(val);
                vector_map.push_back(std::move(Vector3(val, val, val)));
            } else {
                float val = e.raw.to_float();
                tape[tape_index] = val;
                vector_map[tape_index] = std::move(Vector3(val, val, val));
            }
            if (debug)
                UtilityFunctions::print("NUMBER: ", e.raw.to_float());
            tape_index += 1;
        } else if (e.kind == tkVAR) {
            float value = 0.0;
            auto vans = Vector3();
            auto map_get = map->get(e);
            if (map_get.get_type() != Variant::NIL) {
                if (map_get.get_type() == Variant::Type::VECTOR3) {
                    value = NAN;
                    vans = (Vector3)map_get;
                } else {
                    value = (float)map_get;
                    if (std::isnan(value)) {
                        value = 0.0;
                    }
                }
                if (debug)
                    UtilityFunctions::print("VAR: ", e.raw, " = ", value);
            } else if (user_funcs.has(e.raw)) {
                auto func = (Dictionary)user_funcs[e.raw];
                auto args = (PackedStringArray)func[String("args")];
                auto expr = (GDExpr*)(Object*)func[String("expr")];
                auto var_maps = new Vars();
                for (auto v : args) {
                    POP_VAR(x, e.raw + " requires " + UtilityFunctions::str(args.size()) + " parameters")
                    var_maps->set_nok(v, x);
                }
                auto eraw = expr->compute(var_maps, user_funcs, debug);
                delete var_maps;
                if (eraw.get_type() == Variant::Type::VECTOR3) {
                    value = NAN;
                    vans = (Vector3)eraw;
                } else {
                    value = (float)eraw;
                    if (std::isnan(value)) {
                        value = 0.0;
                    }
                }
                if (debug)
                    UtilityFunctions::print("USER_FUNCS: ", value);
            } else {
                if (debug)
                    UtilityFunctions::print("err VAR: ", e.raw, " ", e.sub_kind, " ", e.is_var_vec);
            }
            if (tape_index == tape.size()) {
                tape.push_back(value);
                vector_map.push_back(std::move(vans));
            } else {
                tape[tape_index] = value;
                vector_map[tape_index] = std::move(vans);
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
            auto vans = Vector3();
            if (e.sub_kind == tkopPLUS) {
                BINOP(+)
            } else if (e.sub_kind == tkopMINUS) {
                BINOP(-)
            } else if (e.sub_kind == tkopMULT) {
                BINOP(*)
            } else if (e.sub_kind == tkopDIV) {
                if (b == 0.0) {
                    value = 0.0;
                } else {
                    BINOP(/)
                }
            } else if (e.sub_kind == tkopEXP) {
                value = pow(a, b);
            } else if (e.sub_kind == tkopDOT) {
                if (std::isnan(a)) {
                    auto va = vector_map[tape_index];
                    if (int(b) % 3 == 0) {
                        value = va.x;
                    } else if (int(b) % 3 == 1) {
                        value = va.y;
                    } else if (int(b) % 3 == 2) {
                        value = va.z;
                    }
                } else {
                    value = a;
                }
            }
            if (debug)
                UtilityFunctions::print("BINOP: ", a, " ", e.raw, " ", b, " = ", value);
            // if (std::isnan(value)) {
            //     value = 0.0;
            // }
            if (tape_index == tape.size()) {
                tape.push_back(value);
                vector_map.push_back(std::move(vans));
            } else {
                tape[tape_index] = value;
                vector_map[tape_index] = std::move(vans);
            }
            tape_index += 1;
        } else if (e.kind == tkPREFIX_OP) {
            tape_index -= 1;
            if (tape_index < 0) {
                error = "Incomplete Expression";
                return 0.0;
            }
            double value = tape[tape_index];
            auto vans = vector_map[tape_index];
            if (e.sub_kind == tkopMINUS) {
                if (std::isnan(value)) {
                    vans = -vans;
                } else {
                    value = -value;
                }
            }
            if (tape_index == tape.size()) {
                tape.push_back(value);
                vector_map.push_back(std::move(vans));
            } else {
                tape[tape_index] = value;
                vector_map[tape_index] = std::move(vans);
            }
            if (debug)
                UtilityFunctions::print("PREFIX_OP: ", value);
            tape_index += 1;
        } else if (e.kind == tkFUNC) {
            tape_index -= 1;
            if (tape_index < 0) {
                error = e.raw + " requires at least 1 parameter";
                return 0.0;
            }
            auto a = tape[tape_index];
            double value = 0.0;
            auto vans = Vector3();
            if (e.sub_kind == tkfn_sin) {
                value = sin(a);
            } else if (e.sub_kind == tkfn_cos) {
                value = cos(a);
            } else if (e.sub_kind == tkfn_tan) {
                value = tan(a);
            } else if (e.sub_kind == tkfn_sinh) {
                value = sinh(a);
            } else if (e.sub_kind == tkfn_cosh) {
                value = cosh(a);
            } else if (e.sub_kind == tkfn_tanh) {
                value = tanh(a);
            } else if (e.sub_kind == tkfn_asin) {
                value = asin(a);
            } else if (e.sub_kind == tkfn_acos) {
                value = acos(a);
            } else if (e.sub_kind == tkfn_atan) {
                value = atan(a);
            } else if (e.sub_kind == tkfn_atan2) {
                POP_VAR(b, "mod requires 2 parameters")
                value = atan2(a, b);
            } else if (e.sub_kind == tkfn_asinh) {
                value = asinh(a);
            } else if (e.sub_kind == tkfn_acosh) {
                value = acosh(a);
            } else if (e.sub_kind == tkfn_atanh) {
                value = atanh(a);
            } else if (e.sub_kind == tkfn_inv) {
                value = a != 0 ? (1 / a) : 0;
            } else if (e.sub_kind == tkfn_mod) {
                POP_VAR(b, "mod requires 2 parameters")
                value = a != 0 ? fmod(b, a) : 0;
            } else if (e.sub_kind == tkfn_div) {
                POP_VAR(b, "div requires 2 parameters")
                value = a != 0 ? floor(b / a) : 0;
            } else if (e.sub_kind == tkfn_floor) {
                value = floor(a);
            } else if (e.sub_kind == tkfn_ceil) {
                value = ceil(a);
            } else if (e.sub_kind == tkfn_round) {
                value = round(a);
            } else if (e.sub_kind == tkfn_max) {
                POP_VAR(b, "max requires 2 parameters")
                value = a < b ? b : a;
            } else if (e.sub_kind == tkfn_min) {
                POP_VAR(b, "min requires 2 parameters")
                value = a < b ? a : b;
            } else if (e.sub_kind == tkfn_lt) {
                POP_VAR(b, "lt requires 2 parameters")
                value = b < a ? 1 : 0;
            } else if (e.sub_kind == tkfn_gt) {
                POP_VAR(b, "gt requires 2 parameters")
                value = b > a ? 1 : 0;
            } else if (e.sub_kind == tkfn_lte) {
                POP_VAR(b, "lte requires 2 parameters")
                value = b <= a ? 1 : 0;
            } else if (e.sub_kind == tkfn_gte) {
                POP_VAR(b, "gte requires 2 parameters")
                value = b >= a ? 1 : 0;
            } else if (e.sub_kind == tkfn_eq) {
                POP_VAR(b, "eq requires 2 parameters")
                value = a == b ? 1 : 0;
            } else if (e.sub_kind == tkfn_neq) {
                POP_VAR(b, "neq requires 2 parameters")
                value = a != b ? 1 : 0;
            } else if (e.sub_kind == tkfn_not) {
                value = a == 1 ? 0 : 1;
            } else if (e.sub_kind == tkfn_pow) {
                POP_VAR(b, "pow requires 2 parameters")
                value = pow(b, a);
            } else if (e.sub_kind == tkfn_log10) {
                value = log10f(a);
            } else if (e.sub_kind == tkfn_logN) {
                value = log(a);
            } else if (e.sub_kind == tkfn_abs) {
                value = abs(a);
            } else if (e.sub_kind == tkfn_sqrt) {
                value = sqrtf(a);
            } else if (e.sub_kind == tkfn_cbrt) {
                value = powf(a, 1.0 / 3.0);
            } else if (e.sub_kind == tkfn_sqr) {
                value = a * a;
            } else if (e.sub_kind == tkfn_cube) {
                value = a * a * a;
            } else if (e.sub_kind == tkfn_lerp) {
                POP_VAR(b, "lerp requires 3 parameters")
                POP_VAR(c, "lerp requires 3 parameters")
                value = UtilityFunctions::lerpf(b, a, c);
            } else if (e.sub_kind == tkfn_if) {
                POP_VAR(b, "if requires 3 parameters")
                POP_VAR(c, "if requires 3 parameters")
                value = c != 0.0 ? b : a;
            } else if (e.sub_kind == tkfn_clamp) {
                POP_VAR(b, "clamp requires 3 parameters")
                POP_VAR(c, "clamp requires 3 parameters")
                if (c < b) {
                    value = b;
                } else if (c > a) {
                    value = a;
                } else {
                    value = c;
                }
            } else if (e.sub_kind == tkfn_quad) {
                POP_VAR(b, "quad requires 4 parameters")
                POP_VAR(c, "quad requires 4 parameters")
                POP_VAR(d, "quad requires 4 parameters")

                float x1 = UtilityFunctions::lerpf(c, b, d);
                float x2 = UtilityFunctions::lerpf(b, a, d);
                value = UtilityFunctions::lerpf(x1, x2, d);
            } else if (e.sub_kind == tkfn_cubic) {
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
            } else if (e.sub_kind == tkfn_segment2) {
                POP_VAR(b, "segment2 requires 4 parameters")
                POP_VAR(c, "segment2 requires 4 parameters")
                POP_VAR(d, "segment2 requires 4 parameters")
                value = clerpf(b, c, d / a);
            } else if (e.sub_kind == tkfn_segment3) {
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
            } else if (e.sub_kind == tkfn_segment4) {
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
            } else if (e.sub_kind == tkfn_segment5) {
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
            } else if (e.sub_kind == tkfn_dot2) {
                POP_VAR(b, "dot2 requires 4 parameters")
                POP_VAR(c, "dot2 requires 4 parameters")
                POP_VAR(d, "dot2 requires 4 parameters")
                value = Vector2(d, c).dot(Vector2(b, a));
            } else if (e.sub_kind == tkfn_dot) {
                POP_VAR(b, "dot requires 2 parameters")
                auto va = vector_map[tape_index + 1];
                auto vb = vector_map[tape_index + 0];
                value = vb.dot(va);
            } else if (e.sub_kind == tkfn_dot3) {
                POP_VAR(b, "dot3 requires 6 parameters")
                POP_VAR(c, "dot3 requires 6 parameters")
                POP_VAR(d, "dot3 requires 6 parameters")
                POP_VAR(e, "dot3 requires 6 parameters")
                POP_VAR(f, "dot3 requires 6 parameters")
                value = Vector3(f, e, d).dot(Vector3(c, b, a));
            } else if (e.sub_kind == tkfn_len) {
                auto va = vector_map[tape_index + 0];
                value = va.length();
            } else if (e.sub_kind == tkfn_len2) {
                POP_VAR(b, "len2 requires 2 parameters")
                value = sqrt(a * a + b * b);
            } else if (e.sub_kind == tkfn_len3) {
                POP_VAR(b, "len3 requires 3 parameters")
                POP_VAR(c, "len3 requires 3 parameters")
                value = sqrt(a * a + b * b + c * c);
            } else if (e.sub_kind == tkfn_cross) {
                POP_VAR(b, "cross requires 2 parameters")
                auto va = vector_map[tape_index + 1];
                auto vb = vector_map[tape_index + 0];
                vans = vb.cross(va);
                value = NAN;
            } else if (e.sub_kind == tkfn_cross_x) {
                POP_VAR(b, "cross_x requires 6 parameters")
                POP_VAR(c, "cross_x requires 6 parameters")
                POP_VAR(d, "cross_x requires 6 parameters")
                POP_VAR(e, "cross_x requires 6 parameters")
                POP_VAR(f, "cross_x requires 6 parameters")
                value = Vector3(f, e, d).cross(Vector3(c, b, a)).x;
            } else if (e.sub_kind == tkfn_cross_y) {
                POP_VAR(b, "cross_y requires 6 parameters")
                POP_VAR(c, "cross_y requires 6 parameters")
                POP_VAR(d, "cross_y requires 6 parameters")
                POP_VAR(e, "cross_y requires 6 parameters")
                POP_VAR(f, "cross_y requires 6 parameters")
                value = Vector3(f, e, d).cross(Vector3(c, b, a)).y;
            } else if (e.sub_kind == tkfn_cross_z) {
                POP_VAR(b, "cross_z requires 6 parameters")
                POP_VAR(c, "cross_z requires 6 parameters")
                POP_VAR(d, "cross_z requires 6 parameters")
                POP_VAR(e, "cross_z requires 6 parameters")
                POP_VAR(f, "cross_z requires 6 parameters")
                value = Vector3(f, e, d).cross(Vector3(c, b, a)).z;
            } else if (e.sub_kind == tkfn_proj) {
                POP_VAR(b, "proj requires 2 parameters")
                auto va = vector_map[tape_index + 1];
                auto vb = vector_map[tape_index + 0];
                vans = va.slide(vb.normalized());
                value = NAN;
            } else if (e.sub_kind == tkfn_proj_x) {
                POP_VAR(b, "proj_x requires 6 parameters")
                POP_VAR(c, "proj_x requires 6 parameters")
                POP_VAR(d, "proj_x requires 6 parameters")
                POP_VAR(e, "proj_x requires 6 parameters")
                POP_VAR(f, "proj_x requires 6 parameters")
                auto n = Vector3(f, e, d).normalized();
                auto v = Vector3(c, b, a);
                value = (v.slide(n)).x;
            } else if (e.sub_kind == tkfn_proj_y) {
                POP_VAR(b, "proj_y requires 6 parameters")
                POP_VAR(c, "proj_y requires 6 parameters")
                POP_VAR(d, "proj_y requires 6 parameters")
                POP_VAR(e, "proj_y requires 6 parameters")
                POP_VAR(f, "proj_y requires 6 parameters")
                auto n = Vector3(f, e, d).normalized();
                auto v = Vector3(c, b, a);
                value = (v.slide(n)).y;
            } else if (e.sub_kind == tkfn_proj_z) {
                POP_VAR(b, "proj_z requires 6 parameters")
                POP_VAR(c, "proj_z requires 6 parameters")
                POP_VAR(d, "proj_z requires 6 parameters")
                POP_VAR(e, "proj_z requires 6 parameters")
                POP_VAR(f, "proj_z requires 6 parameters")
                auto n = Vector3(f, e, d).normalized();
                auto v = Vector3(c, b, a);
                value = (v.slide(n)).z;
            } else if (e.sub_kind == tkfn_unit) {
                auto va = vector_map[tape_index + 0];
                vans = va.normalized();
                value = NAN;
            } else if (e.sub_kind == tkfn_unit_x) {
                POP_VAR(b, "unit_x requires 3 parameters")
                POP_VAR(c, "unit_x requires 3 parameters")
                value = Vector3(c, b, a).normalized().x;
            } else if (e.sub_kind == tkfn_unit_y) {
                POP_VAR(b, "unit_y requires 3 parameters")
                POP_VAR(c, "unit_y requires 3 parameters")
                value = Vector3(c, b, a).normalized().y;
            } else if (e.sub_kind == tkfn_unit_z) {
                POP_VAR(b, "unit_z requires 3 parameters")
                POP_VAR(c, "unit_z requires 3 parameters")
                value = Vector3(c, b, a).normalized().z;
            } else if (e.sub_kind == tkfn_rot) {
                POP_VAR(b, "rot requires 3 parameters")
                POP_VAR(c, "rot requires 3 parameters")
                auto va = vector_map[tape_index + 2];
                auto vb = vector_map[tape_index + 1];
                vans = va.rotated(vb.normalized(), c);
                value = NAN;
            } else if (e.sub_kind == tkfn_rot_x) {
                POP_VAR(b, "rot_x requires 7 parameters")
                POP_VAR(c, "rot_x requires 7 parameters")
                POP_VAR(d, "rot_x requires 7 parameters")
                POP_VAR(e, "rot_x requires 7 parameters")
                POP_VAR(f, "rot_x requires 7 parameters")
                POP_VAR(g, "rot_x requires 7 parameters")
                value = Vector3(c, b, a).rotated(Vector3(f, e, d).normalized(), g).x;
            } else if (e.sub_kind == tkfn_rot_y) {
                POP_VAR(b, "rot_y requires 7 parameters")
                POP_VAR(c, "rot_y requires 7 parameters")
                POP_VAR(d, "rot_y requires 7 parameters")
                POP_VAR(e, "rot_y requires 7 parameters")
                POP_VAR(f, "rot_y requires 7 parameters")
                POP_VAR(g, "rot_y requires 7 parameters")
                value = Vector3(c, b, a).rotated(Vector3(f, e, d).normalized(), g).y;
            } else if (e.sub_kind == tkfn_rot_z) {
                POP_VAR(b, "rot_z requires 7 parameters")
                POP_VAR(c, "rot_z requires 7 parameters")
                POP_VAR(d, "rot_z requires 7 parameters")
                POP_VAR(e, "rot_z requires 7 parameters")
                POP_VAR(f, "rot_z requires 7 parameters")
                POP_VAR(g, "rot_z requires 7 parameters")
                value = Vector3(c, b, a).rotated(Vector3(f, e, d).normalized(), g).z;
            } else if (e.sub_kind == tkfn_vec) {
                POP_VAR(b, "vec requires 3 parameters")
                POP_VAR(c, "vec requires 3 parameters")
                value = NAN;
                vans = Vector3(c, b, a);
            } else if (e.sub_kind == tkfn_perp) {
                auto va = vector_map[tape_index + 0];
                if (Vector3(0, 1, 0).cross(va.normalized()).is_zero_approx()) {
                    vans = Vector3(1, 0, 0).slide(va.normalized());
                } else {
                    vans = Vector3(0, 1, 0).slide(va.normalized());
                }
                value = NAN;
            } else if (e.sub_kind == tkfn_perp_x) {
                POP_VAR(b, "perp_x requires 3 parameters")
                POP_VAR(c, "perp_x requires 3 parameters")
                auto v = Vector3(c, b, a);
                if (Vector3(0, 1, 0).cross(v.normalized()).is_zero_approx()) {
                    value = Vector3(1, 0, 0).slide(v.normalized()).x;
                } else {
                    value = Vector3(0, 1, 0).slide(v.normalized()).x;
                }
            } else if (e.sub_kind == tkfn_perp_y) {
                POP_VAR(b, "perp_y requires 3 parameters")
                POP_VAR(c, "perp_y requires 3 parameters")
                auto v = Vector3(c, b, a);
                if (Vector3(0, 1, 0).cross(v.normalized()).is_zero_approx()) {
                    value = Vector3(1, 0, 0).slide(v.normalized()).y;
                } else {
                    value = Vector3(0, 1, 0).slide(v.normalized()).y;
                }
            } else if (e.sub_kind == tkfn_perp_z) {
                POP_VAR(b, "perp_x requires 3 parameters")
                POP_VAR(c, "perp_x requires 3 parameters")
                auto v = Vector3(c, b, a);
                if (Vector3(0, 1, 0).cross(v.normalized()).is_zero_approx()) {
                    value = Vector3(1, 0, 0).slide(v.normalized()).z;
                } else {
                    value = Vector3(0, 1, 0).slide(v.normalized()).z;
                }
            }
            if (debug)
                UtilityFunctions::print("FUNC: ", e.raw, " = ", value);
            if (tape_index == tape.size()) {
                tape.push_back(value);
                vector_map.push_back(std::move(vans));
            } else {
                tape[tape_index] = value;
                vector_map[tape_index] = std::move(vans);
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
    if (debug) {
        UtilityFunctions::print("tape: ------------");
        UtilityFunctions::print("index: ", tape_index);
        for (auto v : tape) {
            UtilityFunctions::print(v);
        }
        UtilityFunctions::print("vector tape: ------------");
        for (auto v : vector_map) {
            UtilityFunctions::print(v);
        }
        UtilityFunctions::print("================");
    }
    auto result = tape[tape_index];
    if (std::isnan(result)) {
        return (Vector3)vector_map[tape_index];
    } else {
        return result;
    }
}

float GDExpr::compute_value(const Vars* map, const Dictionary& user_funcs, bool debug)
{
    Variant value = compute(map, user_funcs, debug);
    if (value.get_type() == Variant::VECTOR3) {
        return 0.0;
    } else {
        return value;
    }
}

bool GDExpr::contains_variable(const godot::String& var_name)
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

bool GDExpr::contains_variable_token(const int16_t var_token)
{
    for (auto& e : expression) {
        if (e.kind == tkVAR && e.sub_kind == var_token) {
            return true;
        }
    }
    return false;
}

String GDExpr::all_variables_is_contained(const Dictionary& dict, const Dictionary& user_funcs)
{
    for (auto& e : expression) {
        if (e.kind == tkVAR) {
            if (!dict.has(e.raw) && !user_funcs.has(e.raw)) {
                return e.raw;
            }
        }
    }
    return "";
}

String GDExpr::all_variable_tokens_is_contained(const Vars& dict, const Dictionary& user_funcs)
{
    for (auto& e : expression) {
        if (!dict.has(e) && (e.kind == tkVAR && !user_funcs.has(e.raw))) {
            return e.raw;
        }
    }
    return "";
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

void bake_into_vector(const std::vector<GDToken>& tokens, std::vector<GDToken>* result, Dictionary map, std::vector<String>* chain)
{
    auto buffer = std::vector<GDToken>();
    for (auto& e : tokens) {
        if (e.kind == tkVAR) {
            String sub_expr = String(map.get(e.raw, ""));
            if (sub_expr.length() > 0) {
                auto sub_tokens = godot::tokenize("(" + sub_expr + ")");
                if (vector_contains_string(chain, e.raw)) {
                    result->emplace_back(tkNUMBER, "0");
                } else {
                    chain->emplace_back(e.raw);
                    bake_into_vector(sub_tokens, &buffer, map, chain);
                    chain->pop_back();
                }
            } else {
                buffer.push_back(std::move(e));
            }
        } else {
            buffer.push_back(std::move(e));
        }
    }
    for (auto& e : buffer) {
        result->push_back(std::move(e));
    }
}

String GDExpr::bake(const String& expr, const Dictionary& map)
{
    auto tokens = godot::tokenize(expr);
    auto output = std::vector<GDToken>();
    auto chain = std::vector<String>();
    bake_into_vector(tokens, &output, map, &chain);
    auto e = GDExpr();
    e.build_from_tokens(output);
    return godot::constant_folding(e.expression, map);
}

bool GDExpr::get_contains_time_dependent()
{
    return contains_time_dependent;
}

int32_t GDExpr::get_variable_update_set()
{
    return variable_update_set;
}

void GDExpr::copy_from(GDExpr* expr)
{
    for (auto e : this->expression) {
        expr->expression.push_back(e);
    }
    this->error = expr->error;
}

String GDExpr::infix_description()
{
    return godot::rpn_to_infix(expression);
}

void GDExpr::print_profiling()
{
}

void GDExpr::_bind_methods()
{
    ClassDB::bind_static_method("GDExpr", D_METHOD("bake", "expr", "map"), &GDExpr::bake);

    ClassDB::bind_method(D_METHOD("build_in", "expr"), &GDExpr::build_in);
    ClassDB::bind_method(D_METHOD("copy_from", "expr"), &GDExpr::copy_from);

    ClassDB::bind_method(D_METHOD("build", "expression"), &GDExpr::build);
    ClassDB::bind_method(D_METHOD("compute", "variables", "user_funcs", "debug"), &GDExpr::compute);
    ClassDB::bind_method(D_METHOD("compute_value", "variables", "user_funcs", "debug"), &GDExpr::compute_value);
    ClassDB::bind_method(D_METHOD("contains_variable", "variable_name"), &GDExpr::contains_variable);
    ClassDB::bind_method(D_METHOD("contains_variable_token", "variable_token"), &GDExpr::contains_variable_token);
    ClassDB::bind_method(D_METHOD("all_variables_is_contained", "dict", "user_funcs"), &GDExpr::all_variables_is_contained);

    ClassDB::bind_method(D_METHOD("get_contains_time_dependent"), &GDExpr::get_contains_time_dependent);
    ClassDB::bind_method(D_METHOD("get_variable_update_set"), &GDExpr::get_variable_update_set);

    ClassDB::bind_method(D_METHOD("get_error"), &GDExpr::get_error);
    ClassDB::bind_method(D_METHOD("set_error", "error_message"), &GDExpr::set_error);
    ClassDB::add_property("GDExpr", PropertyInfo(Variant::STRING, "error"), "set_error", "get_error");

    ClassDB::bind_method(D_METHOD("token_description"), &GDExpr::token_description);
    ClassDB::bind_method(D_METHOD("infix_description"), &GDExpr::infix_description);

    ClassDB::bind_method(D_METHOD("print_profiling"), &GDExpr::print_profiling);
}