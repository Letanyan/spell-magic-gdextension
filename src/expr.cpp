#include "expr.h"
#include <iostream>
#include <math.h>

// #include <godot_cpp/core/class_db.hpp>

using namespace godot;

void GDExpr::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("build", "expression"), &GDExpr::build);
    ClassDB::bind_method(D_METHOD("compute", "variables"), &GDExpr::compute);

    ClassDB::bind_method(D_METHOD("get_error"), &GDExpr::get_error);
    ClassDB::bind_method(D_METHOD("set_error", "error_message"), &GDExpr::set_error);
    ClassDB::add_property("GDExpr", PropertyInfo(Variant::STRING, "error"), "set_error", "get_error");
}

String GDExpr::get_error()
{
    return String(error.data());
}

void GDExpr::set_error(String err_message)
{
    error = std::string(err_message.utf8().get_data());
}

GDExpr::GDExpr()
{
    // Initialize any variables here.
}

GDExpr::~GDExpr()
{
    // Add your cleanup here.
}

void GDExpr::build_base(std::string expr)
{
    auto tokens = godot::tokenize(expr);

    error = "";
    expression = std::vector<GDToken>();
    auto operators = std::vector<GDToken>();
    int i = 0;
    while (i < tokens.size()) {
        auto token = tokens[i];
        i += 1;

        if (token.kind == tkNUMBER || token.kind == tkVAR) {
            expression.push_back(token);
        } else if (token.kind == tkFUNC) {
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
                if (operators.size() > 0 && operators.back().kind == tkFUNC) {
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

void GDExpr::build(String expr)
{
    build_base(expr.utf8().get_data());
}

bool godot::gd_operator_precedes(GDToken op1, GDToken op2)
{
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

double str_to_double(std::string str)
{
    return atof(str.data());
}

double GDExpr::compute_base(std::function<double(char*, double)> map)
{
    auto tape = std::vector<double>(8);
    long long tape_index = 0;
    for (auto& e : expression) {
        if (e.kind == tkERROR) {
            error = e.raw;
            return 0.0;
        }

        if (e.kind == tkNUMBER) {
            if (tape_index == tape.size()) {
                tape.push_back(str_to_double(e.raw));
            } else {
                tape[tape_index] = str_to_double(e.raw);
            }
            tape_index += 1;
        } else if (e.kind == tkVAR) {
            double value = map(e.raw.data(), 0.0);
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
                value = a / b;
            } else if (e.raw == "^") {
                value = pow(a, b);
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
            if (tape_index == tape.size()) {
                tape.push_back(value);
            } else {
                tape[tape_index] = value;
            }
            tape_index += 1;
        } else if (e.kind == tkFUNC) {
            tape_index -= 1;
            if (tape_index < 0) {
                error = "Incomplete Expression";
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
                tape_index -= 1;
                if (tape_index < 0) {
                    error = "Incomplete Expression";
                    return 0.0;
                }
                auto b = tape[tape_index];
                value = a != 0 ? fmod(b, a) : 0;
            } else if (e.raw == "div") {
                tape_index -= 1;
                if (tape_index < 0) {
                    error = "Incomplete Expression";
                    return 0.0;
                }
                auto b = tape[tape_index];
                value = a != 0 ? floor(b / a) : 0;
            } else if (e.raw == "floor") {
                value = floor(a);
            } else if (e.raw == "ceil") {
                value = ceil(a);
            } else if (e.raw == "round") {
                value = round(a);
            } else if (e.raw == "max") {
                tape_index -= 1;
                if (tape_index < 0) {
                    error = "Incomplete Expression";
                    return 0.0;
                }
                auto b = tape[tape_index];
                value = a < b ? b : a;
            } else if (e.raw == "min") {
                tape_index -= 1;
                if (tape_index < 0) {
                    error = "Incomplete Expression";
                    return 0.0;
                }
                auto b = tape[tape_index];
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

double GDExpr::compute(Dictionary variables)
{
    std::function<double(char*, double)> my_map = [variables](char* str, double def) -> double {
        return variables.get(String(str), def);
    };
    return compute_base(my_map);
}