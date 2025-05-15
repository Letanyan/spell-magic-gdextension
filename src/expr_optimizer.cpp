#include "expr_optimizer.h"
#include "expr.h"
#include "token.h"
#include <math.h>

#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

GDExprTree::GDExprTree()
{
    this->data = GDToken();
    this->left = nullptr;
    this->right = nullptr;
}

GDExprTree::GDExprTree(GDToken data)
{
    this->data = data;
    this->left = nullptr;
    this->right = nullptr;
}

GDExprTree::GDExprTree(GDToken data, GDToken left, GDToken right)
{
    this->data = data;
    this->left = new GDExprTree(left);
    this->right = new GDExprTree(right);
}

GDExprTree::GDExprTree(GDToken data, GDExprTree* left, GDExprTree* right)
{
    this->data = data;
    this->left = left;
    this->right = right;
}

GDExprTree::~GDExprTree()
{
    if (this->left != nullptr) {
        delete this->left;
    }
    if (this->right != nullptr) {
        delete this->right;
    }
}

String GDExprTree::display(String header, String padding)
{
    String result = "";
    result += padding;
    result += header;
    result += data.raw;
    result += "\n";

    padding += "  ";

    if (left != nullptr) {
        result += left->display(right != nullptr ? "|-" : "--", padding);
    }
    if (right != nullptr) {
        result += right->display("--", padding);
    }

    return result;
}

String format_as_expression_helper(GDExprTree* tree)
{
    String result = "";
    if (tree->data.kind == tkOP || tree->data.kind == tkCOMMA) {
        if (tree->data.raw == "#") {
            if (tree->left != nullptr) {
                result += format_as_expression_helper(tree->left) + "(";
            }
            if (tree->right != nullptr) {
                result += format_as_expression_helper(tree->right) + ")";
            }
        } else if (tree->data.raw == "(") {
            if (tree->right != nullptr) {
                result += "(" + format_as_expression_helper(tree->right) + ")";
            }
        } else {
            if (tree->left != nullptr) {
                result += format_as_expression_helper(tree->left);
            }
            result += tree->data.raw;
            if (tree->right != nullptr) {
                result += format_as_expression_helper(tree->right);
            }
        }
    } else if (tree->data.kind == tkPREFIX_OP) {
        if (tree->right != nullptr) {
            auto kind = tree->right->data.kind;
            auto raw = tree->right->data.raw;
            if (kind == tkNUMBER || kind == tkFUNC || kind == tkVAR || kind == tkOPEN || (kind == tkOP && raw == "#")) {
                result += tree->data.raw + format_as_expression_helper(tree->right);
            } else {
                result += tree->data.raw + "(" + format_as_expression_helper(tree->right) + ")";
            }
        }
    } else {
        result += tree->data.raw;
    }
    return result;
}

String GDExprTree::format_as_expression()
{
    return format_as_expression_helper(this);
}

GDExprTree* build_right_tree(const std::vector<GDToken>& tokens, int& cursor)
{
    GDExprTree* left = new GDExprTree(tokens[cursor++]);
    if (tokens[cursor].kind == tkOP) {
        GDToken op = tokens[cursor++];
        GDExprTree* tree = new GDExprTree(op, left, build_right_tree(tokens, cursor));
        return tree;
    } else {
        return left;
    }
}

GDExprTree* make_single_binary_tree(GDExprTree* left, const std::vector<GDToken>& tokens, int& cursor)
{
    GDToken op = tokens[cursor++];
    if (op.kind == tkOP) {
        GDExprTree* right = new GDExprTree(tokens[cursor++]);
        return new GDExprTree(op, left, right);
    } else {
        return left;
    }
}

GDExprTree* build_left_tree(const std::vector<GDToken>& tokens, int& cursor)
{
    GDExprTree* left = new GDExprTree(tokens[cursor++]);

    while (cursor < tokens.size()) {
        auto new_tree = make_single_binary_tree(left, tokens, cursor);
        if (new_tree == left) {
            return left;
        }
        left = new_tree;
    }

    return left;
}

GDExprTree* godot::parse_sub_expr_tree(GDToken next, const std::vector<GDToken>& tokens, int& cursor)
{
    GDExprTree* result;
    if (next.kind == tkOPEN) {
        result = parse_expr_tree(tokens, cursor, 0);
        GDToken close_paren_token = tokens[cursor++];
        result = new GDExprTree(GDToken(tkOP, "("), new GDExprTree(GDToken(tkNONE, "")), result);
    } else if (next.kind == tkFUNC) {
        GDToken open_paren_token = tokens[cursor++];
        result = parse_expr_tree(tokens, cursor, 0);
        GDToken close_paren_token = tokens[cursor++];
        result = new GDExprTree(GDToken(tkOP, "#"), new GDExprTree(next), result);
    } else if (next.kind == tkPREFIX_OP) {
        result = parse_expr_tree(tokens, cursor, gd_operator_precedence(next));
        result = new GDExprTree(next, new GDExprTree(GDToken(tkNONE, "")), result);
    } else {
        result = new GDExprTree(next);
    }
    return result;
}

GDExprTree* godot::parse_binary_tree(GDToken op, GDExprTree* left, const std::vector<GDToken>& tokens, int& cursor)
{
    auto prec = gd_operator_precedence(op);
    if (gd_operator_is_right_associative(op)) {
        prec -= 1;
    }
    auto right = parse_expr_tree(tokens, cursor, prec);
    return new GDExprTree(op, left, right);
}

GDExprTree* godot::parse_expr_tree(const std::vector<GDToken>& tokens, int& cursor, int min_prec)
{
    GDToken next_token = tokens[cursor++];
    GDExprTree* left_node = parse_sub_expr_tree(next_token, tokens, cursor);

    while (cursor < tokens.size()) {
        GDToken op_token = tokens[cursor];
        auto next_prec = gd_operator_precedence(op_token);
        if (min_prec >= next_prec) {
            break;
        }
        cursor++;
        left_node = parse_binary_tree(op_token, left_node, tokens, cursor);
    }
    return left_node;
}

GDExprTree* expr_tree_constant_folding(GDExprTree* tree)
{
    if (tree->left != nullptr && tree->right != nullptr) {
        if (tree->data.kind == tkOP) {
            if (tree->left->data.kind == tkNUMBER && tree->right->data.kind == tkNUMBER) {
                float number = 0.0;
                String op = tree->data.raw;
                if (op == "+") {
                    number = tree->left->data.raw.to_float() + tree->right->data.raw.to_float();
                } else if (op == "-") {
                    number = tree->left->data.raw.to_float() - tree->right->data.raw.to_float();
                } else if (op == "*") {
                    number = tree->left->data.raw.to_float() * tree->right->data.raw.to_float();
                } else if (op == "/") {
                    number = tree->left->data.raw.to_float() / tree->right->data.raw.to_float();
                } else if (op == "^") {
                    number = powf(tree->left->data.raw.to_float(), tree->right->data.raw.to_float());
                }
                auto dict = Dictionary();
                dict["number"] = number;
                tree->data = GDToken(tkNUMBER, String("{number}").format(dict));
                delete tree->left;
                delete tree->right;
                tree->left = NULL;
                tree->right = NULL;
            } else if (tree->left->data.kind == tkNUMBER) {
                bool modified = false;
                if ((tree->data.raw == "*" || tree->data.raw == "/" || tree->data.raw == "^") && tree->left->data.raw.to_float() == 0.0) {
                    tree->data = GDToken(tkNUMBER, "0");
                    delete tree->left;
                    delete tree->right;
                    tree->left = NULL;
                    tree->right = NULL;
                    modified = true;
                } else if ((tree->data.raw == "+") && tree->left->data.raw.to_float() == 0.0) {
                    tree = tree->right;
                    delete tree->left;
                    tree->left = NULL;
                    modified = true;
                } else if ((tree->data.raw == "*") && tree->left->data.raw.to_float() == 1.0) {
                    tree = tree->right;
                    delete tree->left;
                    tree->left = NULL;
                    modified = true;
                }
                auto new_right = expr_tree_constant_folding(tree->right);
                if (modified || tree->right != new_right) {
                    tree->right = new_right;
                    tree = expr_tree_constant_folding(tree);
                } else {
                    tree->right = new_right;
                }
            } else if (tree->right->data.kind == tkNUMBER) {
                bool modified = false;
                if ((tree->data.raw == "*" || tree->data.raw == "/") && tree->right->data.raw.to_float() == 0.0) {
                    tree->data = GDToken(tkNUMBER, "0");
                    delete tree->left;
                    delete tree->right;
                    tree->left = NULL;
                    tree->right = NULL;
                    modified = true;
                } else if ((tree->data.raw == "+") && tree->right->data.raw.to_float() == 0.0) {
                    tree = tree->left;
                    delete tree->right;
                    tree->right = NULL;
                    modified = true;
                } else if ((tree->data.raw == "*") && tree->right->data.raw.to_float() == 1.0) {
                    tree = tree->right;
                    delete tree->right;
                    tree->right = NULL;
                    modified = true;
                } else if ((tree->data.raw == "^") && tree->right->data.raw.to_float() == 1.0) {
                    tree->data = GDToken(tkNUMBER, "1");
                    delete tree->left;
                    delete tree->right;
                    tree->left = NULL;
                    tree->right = NULL;
                    modified = true;
                }
                auto new_left = expr_tree_constant_folding(tree->left);
                if (modified || tree->left != new_left) {
                    tree->left = new_left;
                    tree = expr_tree_constant_folding(tree);
                } else {
                    tree->left = new_left;
                }
            } else {
                auto new_right = expr_tree_constant_folding(tree->right);
                auto new_left = expr_tree_constant_folding(tree->left);
                if (new_right != tree->right || new_left != tree->left) {
                    tree->left = new_left;
                    tree->right = new_right;
                    tree = expr_tree_constant_folding(tree);
                } else {
                    tree->left = new_left;
                    tree->right = new_right;
                }
            }
        }
    }
    return tree;
}

String godot::optimize(const std::vector<GDToken>& tokens)
{
    int cursor = 0;
    auto tree = parse_expr_tree(tokens, cursor, 0);
    tree = expr_tree_constant_folding(tree);
    // String result = tree->display();
    String result = tree->format_as_expression();
    delete tree;
    return result;
}

String godot::constant_folding(const std::vector<GDToken>& expression, Dictionary map)
{
    // parameters are in reverse order. For example the first popped var is the last parameter:
    // f(..., z, ..., c, b, a)
#undef POP_VAR
#define POP_VAR(name, error_message) \
    tape_index -= 1;                 \
    if (tape_index < 0) {            \
        return String("");           \
    }                                \
    auto name = std::move(tape[tape_index]);

#undef PUSH_VAR
#define PUSH_VAR(name)                      \
    if (tape_index == tape.size()) {        \
        tape.push_back(std::move(name));    \
    } else {                                \
        tape[tape_index] = std::move(name); \
    }                                       \
    tape_index += 1;

    auto tape = std::vector<GDToken>();
    tape.reserve(8);
    auto vector_map = Dictionary();
    long long tape_index = 0;
    for (auto& e : expression) {
        if (e.kind == tkERROR) {
            return String("");
        }

        if (e.kind == tkNUMBER) {
            PUSH_VAR(e)
        } else if (e.kind == tkVAR) {
            GDToken value = GDToken();
            if (map.has(e.raw)) {
                auto eraw = map[e.raw];
                if (eraw.get_type() == Variant::Type::VECTOR3) {
                    value = e;
                } else {
                    value.raw = UtilityFunctions::str(map.get(e.raw, "0"));
                    value.kind = tkNUMBER;
                }
            } else {
                value = e;
            }
            PUSH_VAR(value)
        } else if (e.kind == tkOP) {
            POP_VAR(b, "OP: Incomplete Expression: " + e.raw)
            POP_VAR(a, "OP: Incomplete Expression: " + e.raw)
            auto value = GDToken();
            if (a.kind == tkNUMBER && b.kind == tkNUMBER) {
                double temp = 0.0;
                if (e.raw == "+") {
                    temp = a.raw.to_float() + b.raw.to_float();
                } else if (e.raw == "-") {
                    temp = a.raw.to_float() - b.raw.to_float();
                } else if (e.raw == "*") {
                    temp = a.raw.to_float() * b.raw.to_float();
                } else if (e.raw == "/") {
                    if (b.raw.to_float() == 0.0) {
                        temp = 0.0;
                    } else {
                        temp = a.raw.to_float() / b.raw.to_float();
                    }
                } else if (e.raw == "^") {
                    temp = pow(a.raw.to_float(), b.raw.to_float());
                }
                if (std::isnan(temp)) {
                    value.raw = String("0");
                } else {
                    value.raw = UtilityFunctions::str(temp);
                }
                value.kind = tkNUMBER;
                PUSH_VAR(value)
            } else if (a.kind == tkNUMBER && b.kind == tkVAR) {
                double temp = a.raw.to_float();
                if (e.raw == "+" && UtilityFunctions::is_zero_approx(temp)) {
                    PUSH_VAR(b)
                } else if (e.raw == "*" && UtilityFunctions::is_equal_approx(temp, 1.0)) {
                    PUSH_VAR(b)
                } else if (e.raw == "^" && UtilityFunctions::is_equal_approx(temp, 1.0)) {
                    auto value = GDToken();
                    value.kind = tkNUMBER;
                    value.raw = String("1");
                    PUSH_VAR(value)
                } else if (e.raw == "^" && UtilityFunctions::is_zero_approx(temp)) {
                    auto value = GDToken();
                    value.kind = tkNUMBER;
                    value.raw = String("0");
                    PUSH_VAR(value)
                } else {
                    PUSH_VAR(a)
                    PUSH_VAR(b)
                    PUSH_VAR(e)
                }
            } else if (a.kind == tkVAR && b.kind == tkNUMBER) {
                double temp = b.raw.to_float();
                if ((e.raw == "+" || e.raw == "-") && UtilityFunctions::is_zero_approx(temp)) {
                    PUSH_VAR(a)
                } else if (e.raw == "*" && UtilityFunctions::is_equal_approx(temp, 1.0)) {
                    PUSH_VAR(a)
                } else if (e.raw == "^" && UtilityFunctions::is_equal_approx(temp, 1.0)) {
                    PUSH_VAR(a)
                } else if (e.raw == "^" && UtilityFunctions::is_zero_approx(temp)) {
                    auto value = GDToken();
                    value.kind = tkNUMBER;
                    value.raw = String("1");
                    PUSH_VAR(value)
                } else {
                    PUSH_VAR(a)
                    PUSH_VAR(b)
                    PUSH_VAR(e)
                }
            } else {
                PUSH_VAR(a)
                PUSH_VAR(b)
                PUSH_VAR(e)
            }
        } else if (e.kind == tkPREFIX_OP) {
            POP_VAR(a, "Incomplete Expression")
            if (a.kind == tkNUMBER) {
                double temp = a.raw.to_float();
                auto value = GDToken();
                if (e.raw == "-") {
                    temp = -temp;
                }
                if (std::isnan(temp)) {
                    value.raw = String("0");
                } else {
                    value.raw = UtilityFunctions::str(temp);
                }
                value.kind = tkNUMBER;
                PUSH_VAR(value)
            } else {
                PUSH_VAR(a)
                PUSH_VAR(e)
            }
        } else if (e.kind == tkFUNC) {
            int arg_count = godot::number_of_func_arguments(e.raw);

            switch (arg_count) {
            case 0: {
                PUSH_VAR(e)
            } break;

            case 1: {
                POP_VAR(tok_a, e.raw + " requires at least 1 parameter")
                double temp = 0.0;
                if (tok_a.kind == tkNUMBER) {
                    double a = tok_a.raw.to_float();
                    if (e.raw == "sin") {
                        temp = sin(a);
                    } else if (e.raw == "cos") {
                        temp = cos(a);
                    } else if (e.raw == "tan") {
                        temp = tan(a);
                    } else if (e.raw == "sinh") {
                        temp = sinh(a);
                    } else if (e.raw == "cosh") {
                        temp = cosh(a);
                    } else if (e.raw == "tanh") {
                        temp = tanh(a);
                    } else if (e.raw == "asin") {
                        temp = asin(a);
                    } else if (e.raw == "acos") {
                        temp = acos(a);
                    } else if (e.raw == "atan") {
                        temp = atan(a);
                    } else if (e.raw == "asinh") {
                        temp = asinh(a);
                    } else if (e.raw == "acosh") {
                        temp = acosh(a);
                    } else if (e.raw == "atanh") {
                        temp = atanh(a);
                    } else if (e.raw == "inv") {
                        temp = a != 0 ? (1 / a) : 0;
                    } else if (e.raw == "floor") {
                        temp = floor(a);
                    } else if (e.raw == "ceil") {
                        temp = ceil(a);
                    } else if (e.raw == "round") {
                        temp = round(a);
                    } else if (e.raw == "not") {
                        temp = a == 1 ? 0 : 1;
                    } else if (e.raw == "log10") {
                        temp = log10f(a);
                    } else if (e.raw == "logN") {
                        temp = log(a);
                    } else if (e.raw == "abs") {
                        temp = abs(a);
                    } else if (e.raw == "sqrt") {
                        temp = sqrtf(a);
                    } else if (e.raw == "cbrt") {
                        temp = powf(a, 1.0 / 3.0);
                    } else if (e.raw == "sqr") {
                        temp = a * a;
                    } else if (e.raw == "cube") {
                        temp = a * a * a;
                    }
                    auto value = GDToken(tkNUMBER, UtilityFunctions::str(temp));
                    PUSH_VAR(value)
                } else {
                    PUSH_VAR(tok_a)
                    PUSH_VAR(e)
                }
            } break;

            case 2: {
                POP_VAR(tok_a, e.raw + " requires at least 2 parameter")
                POP_VAR(tok_b, e.raw + " requires at least 2 parameter")
                double temp = 0.0;
                if (tok_a.kind == tkNUMBER && tok_b.kind == tkNUMBER) {
                    double a = tok_a.raw.to_float();
                    double b = tok_b.raw.to_float();
                    if (e.raw == "mod") {
                        temp = a != 0 ? fmod(b, a) : 0;
                    } else if (e.raw == "div") {
                        temp = a != 0 ? floor(b / a) : 0;
                    } else if (e.raw == "max") {
                        temp = a < b ? b : a;
                    } else if (e.raw == "min") {
                        temp = a < b ? a : b;
                    } else if (e.raw == "lt") {
                        temp = b < a ? 1 : 0;
                    } else if (e.raw == "gt") {
                        temp = b > a ? 1 : 0;
                    } else if (e.raw == "lte") {
                        temp = b <= a ? 1 : 0;
                    } else if (e.raw == "gte") {
                        temp = b >= a ? 1 : 0;
                    } else if (e.raw == "eq") {
                        temp = a == b ? 1 : 0;
                    } else if (e.raw == "neq") {
                        temp = a != b ? 1 : 0;
                    } else if (e.raw == "pow") {
                        temp = pow(b, a);
                    } else if (e.raw == "atan2") {
                        temp = atan2(a, b);
                    } else if (e.raw == "snap") {
                        temp = UtilityFunctions::snappedf(b, a);
                    }
                    auto value = GDToken(tkNUMBER, UtilityFunctions::str(temp));
                    PUSH_VAR(value)
                } else {
                    PUSH_VAR(tok_b)
                    PUSH_VAR(tok_a)
                    PUSH_VAR(e)
                }
            } break;

            case 3: {
                POP_VAR(tok_a, e.raw + " requires at least 3 parameter")
                POP_VAR(tok_b, e.raw + " requires at least 3 parameter")
                POP_VAR(tok_c, e.raw + " requires at least 3 parameter")
                double temp = 0.0;
                if (tok_a.kind == tkNUMBER && tok_b.kind == tkNUMBER && tok_c.kind == tkNUMBER) {
                    double a = tok_a.raw.to_float();
                    double b = tok_b.raw.to_float();
                    double c = tok_c.raw.to_float();
                    if (e.raw == "lerp") {
                        temp = UtilityFunctions::lerpf(b, a, c);
                    } else if (e.raw == "if") {
                        temp = c != 0.0 ? b : a;
                    } else if (e.raw == "clamp") {
                        if (c < b) {
                            temp = b;
                        } else if (c > a) {
                            temp = a;
                        } else {
                            temp = c;
                        }
                    } else if (e.raw == "unit_x") {
                        temp = Vector3(c, b, a).normalized().x;
                    } else if (e.raw == "unit_y") {
                        temp = Vector3(c, b, a).normalized().y;
                    } else if (e.raw == "unit_z") {
                        temp = Vector3(c, b, a).normalized().z;
                    } else if (e.raw == "perp_x") {
                        auto v = Vector3(a, b, c);
                        if (Vector3(0, 1, 0).cross(v.normalized()).is_zero_approx()) {
                            temp = Vector3(1, 0, 0).slide(v.normalized()).x;
                        } else {
                            temp = Vector3(0, 1, 0).slide(v.normalized()).x;
                        }
                    } else if (e.raw == "perp_y") {
                        auto v = Vector3(a, b, c);
                        if (Vector3(0, 1, 0).cross(v.normalized()).is_zero_approx()) {
                            temp = Vector3(1, 0, 0).slide(v.normalized()).y;
                        } else {
                            temp = Vector3(0, 1, 0).slide(v.normalized()).y;
                        }
                    } else if (e.raw == "perp_z") {
                        auto v = Vector3(a, b, c);
                        if (Vector3(0, 1, 0).cross(v.normalized()).is_zero_approx()) {
                            temp = Vector3(1, 0, 0).slide(v.normalized()).z;
                        } else {
                            temp = Vector3(0, 1, 0).slide(v.normalized()).z;
                        }
                    } else if (e.raw == "vec") {
                        PUSH_VAR(tok_c)
                        PUSH_VAR(tok_b)
                        PUSH_VAR(tok_a)
                        PUSH_VAR(e)
                    }
                    if (e.raw != "vec") {
                        auto value = GDToken(tkNUMBER, UtilityFunctions::str(temp));
                        PUSH_VAR(value)
                    }
                } else {
                    PUSH_VAR(tok_c)
                    PUSH_VAR(tok_b)
                    PUSH_VAR(tok_a)
                    PUSH_VAR(e)
                }
            } break;

            case 4: {
                POP_VAR(tok_a, e.raw + " requires at least 4 parameter")
                POP_VAR(tok_b, e.raw + " requires at least 4 parameter")
                POP_VAR(tok_c, e.raw + " requires at least 4 parameter")
                POP_VAR(tok_d, e.raw + " requires at least 4 parameter")
                double temp = 0.0;
                if (tok_a.kind == tkNUMBER && tok_b.kind == tkNUMBER && tok_c.kind == tkNUMBER && tok_d.kind == tkNUMBER) {
                    double a = tok_a.raw.to_float();
                    double b = tok_b.raw.to_float();
                    double c = tok_c.raw.to_float();
                    double d = tok_d.raw.to_float();
                    if (e.raw == "quad") {
                        float x1 = UtilityFunctions::lerpf(c, b, d);
                        float x2 = UtilityFunctions::lerpf(b, a, d);
                        temp = UtilityFunctions::lerpf(x1, x2, d);
                    } else if (e.raw == "segment2") {
                        temp = clerpf(b, c, d / a);
                    } else if (e.raw == "dot2") {
                        temp = Vector2(d, c).dot(Vector2(b, a));
                    }
                    auto value = GDToken(tkNUMBER, UtilityFunctions::str(temp));
                    PUSH_VAR(value)
                } else {
                    PUSH_VAR(tok_d)
                    PUSH_VAR(tok_c)
                    PUSH_VAR(tok_b)
                    PUSH_VAR(tok_a)
                    PUSH_VAR(e)
                }

            } break;

            case 5: {
                POP_VAR(tok_a, e.raw + " requires at least 5 parameter")
                POP_VAR(tok_b, e.raw + " requires at least 5 parameter")
                POP_VAR(tok_c, e.raw + " requires at least 5 parameter")
                POP_VAR(tok_d, e.raw + " requires at least 5 parameter")
                POP_VAR(tok_e, e.raw + " requires at least 5 parameter")
                double temp = 0.0;
                if (tok_a.kind == tkNUMBER && tok_b.kind == tkNUMBER && tok_c.kind == tkNUMBER && tok_d.kind == tkNUMBER && tok_e.kind == tkNUMBER) {
                    double a = tok_a.raw.to_float();
                    double b = tok_b.raw.to_float();
                    double c = tok_c.raw.to_float();
                    double d = tok_d.raw.to_float();
                    double _e = tok_e.raw.to_float();
                    if (e.raw == "cubic") {
                        float x1 = UtilityFunctions::lerpf(d, c, _e);
                        float y1 = UtilityFunctions::lerpf(c, a, _e);
                        float z1 = UtilityFunctions::lerpf(x1, y1, _e);
                        float x2 = UtilityFunctions::lerpf(c, b, _e);
                        float y2 = UtilityFunctions::lerpf(b, a, _e);
                        float z2 = UtilityFunctions::lerpf(x2, y2, _e);
                        temp = UtilityFunctions::lerpf(z1, z2, _e);
                    }

                    auto value = GDToken(tkNUMBER, UtilityFunctions::str(temp));
                    PUSH_VAR(value)
                } else {
                    PUSH_VAR(tok_e)
                    PUSH_VAR(tok_d)
                    PUSH_VAR(tok_c)
                    PUSH_VAR(tok_b)
                    PUSH_VAR(tok_a)
                    PUSH_VAR(e)
                }
            } break;

            case 6: {
                POP_VAR(tok_a, e.raw + " requires at least 6 parameter")
                POP_VAR(tok_b, e.raw + " requires at least 6 parameter")
                POP_VAR(tok_c, e.raw + " requires at least 6 parameter")
                POP_VAR(tok_d, e.raw + " requires at least 6 parameter")
                POP_VAR(tok_e, e.raw + " requires at least 6 parameter")
                POP_VAR(tok_f, e.raw + " requires at least 6 parameter")
                double temp = 0.0;
                if (tok_a.kind == tkNUMBER && tok_b.kind == tkNUMBER && tok_c.kind == tkNUMBER && tok_d.kind == tkNUMBER && tok_e.kind == tkNUMBER && tok_f.kind == tkNUMBER) {
                    double a = tok_a.raw.to_float();
                    double b = tok_b.raw.to_float();
                    double c = tok_c.raw.to_float();
                    double d = tok_d.raw.to_float();
                    double _e = tok_e.raw.to_float();
                    double f = tok_f.raw.to_float();
                    if (e.raw == "segment3") {
                        // f=t, e d c, b=d1, a=d2
                        if (f <= b) {
                            temp = clerpf(_e, d, f / b);
                        } else {
                            temp = clerpf(d, c, (f - b) / a);
                        }
                    } else if (e.raw == "dot3") {
                        temp = Vector3(f, _e, d).dot(Vector3(c, b, a));
                    } else if (e.raw == "cross_x") {
                        temp = Vector3(f, _e, d).cross(Vector3(c, b, a)).x;
                    } else if (e.raw == "cross_y") {
                        temp = Vector3(f, _e, d).cross(Vector3(c, b, a)).y;
                    } else if (e.raw == "cross_z") {
                        temp = Vector3(f, _e, d).cross(Vector3(c, b, a)).z;
                    } else if (e.raw == "proj_x") {
                        auto n = Vector3(f, _e, d).normalized();
                        auto v = Vector3(c, b, a);
                        temp = (v - v.dot(n) * n).x;
                    } else if (e.raw == "proj_y") {
                        auto n = Vector3(f, _e, d).normalized();
                        auto v = Vector3(c, b, a);
                        temp = (v - v.dot(n) * n).y;
                    } else if (e.raw == "proj_z") {
                        auto n = Vector3(f, _e, d).normalized();
                        auto v = Vector3(c, b, a);
                        temp = (v - v.dot(n) * n).z;
                    }
                    auto value = GDToken(tkNUMBER, UtilityFunctions::str(temp));
                    PUSH_VAR(value)
                } else {
                    PUSH_VAR(tok_f)
                    PUSH_VAR(tok_e)
                    PUSH_VAR(tok_d)
                    PUSH_VAR(tok_c)
                    PUSH_VAR(tok_b)
                    PUSH_VAR(tok_a)
                    PUSH_VAR(e)
                }
            } break;

            case 7: {
                POP_VAR(tok_a, e.raw + " requires at least 7 parameter")
                POP_VAR(tok_b, e.raw + " requires at least 7 parameter")
                POP_VAR(tok_c, e.raw + " requires at least 7 parameter")
                POP_VAR(tok_d, e.raw + " requires at least 7 parameter")
                POP_VAR(tok_e, e.raw + " requires at least 7 parameter")
                POP_VAR(tok_f, e.raw + " requires at least 7 parameter")
                POP_VAR(tok_g, e.raw + " requires at least 7 parameter")
                double temp = 0.0;
                if (tok_a.kind == tkNUMBER && tok_b.kind == tkNUMBER && tok_c.kind == tkNUMBER && tok_d.kind == tkNUMBER && tok_e.kind == tkNUMBER && tok_f.kind == tkNUMBER && tok_g.kind == tkNUMBER) {
                    double a = tok_a.raw.to_float();
                    double b = tok_b.raw.to_float();
                    double c = tok_c.raw.to_float();
                    double d = tok_d.raw.to_float();
                    double _e = tok_e.raw.to_float();
                    double f = tok_f.raw.to_float();
                    double g = tok_g.raw.to_float();
                    if (e.raw == "rot_x") {
                        temp = Vector3(c, b, a).rotated(Vector3(f, _e, d), g).x;
                    } else if (e.raw == "rot_y") {
                        temp = Vector3(c, b, a).rotated(Vector3(f, _e, d), g).y;
                    } else if (e.raw == "rot_z") {
                        temp = Vector3(c, b, a).rotated(Vector3(f, _e, d), g).z;
                    }
                    auto value = GDToken(tkNUMBER, UtilityFunctions::str(temp));
                    PUSH_VAR(value)
                } else {
                    PUSH_VAR(tok_f)
                    PUSH_VAR(tok_e)
                    PUSH_VAR(tok_d)
                    PUSH_VAR(tok_c)
                    PUSH_VAR(tok_b)
                    PUSH_VAR(tok_a)
                    PUSH_VAR(e)
                }
            } break;

            case 8: {
                POP_VAR(tok_a, e.raw + " requires at least 8 parameter")
                POP_VAR(tok_b, e.raw + " requires at least 8 parameter")
                POP_VAR(tok_c, e.raw + " requires at least 8 parameter")
                POP_VAR(tok_d, e.raw + " requires at least 8 parameter")
                POP_VAR(tok_e, e.raw + " requires at least 8 parameter")
                POP_VAR(tok_f, e.raw + " requires at least 8 parameter")
                POP_VAR(tok_g, e.raw + " requires at least 8 parameter")
                POP_VAR(tok_h, e.raw + " requires at least 8 parameter")
                double temp = 0.0;
                if (tok_a.kind == tkNUMBER && tok_b.kind == tkNUMBER && tok_c.kind == tkNUMBER && tok_d.kind == tkNUMBER && tok_e.kind == tkNUMBER && tok_f.kind == tkNUMBER && tok_g.kind == tkNUMBER && tok_h.kind == tkNUMBER) {
                    double a = tok_a.raw.to_float();
                    double b = tok_b.raw.to_float();
                    double c = tok_c.raw.to_float();
                    double d = tok_d.raw.to_float();
                    double _e = tok_e.raw.to_float();
                    double f = tok_f.raw.to_float();
                    double g = tok_g.raw.to_float();
                    double h = tok_h.raw.to_float();
                    if (e.raw == "segment4") {
                        // h=t, g f e d, c=d1, b=d2, a=d3
                        if (h <= c) {
                            temp = clerpf(g, f, h / c);
                        } else if (h <= c + b) {
                            temp = clerpf(f, _e, (h - c) / b);
                        } else {
                            temp = clerpf(_e, d, (h - c - b) / a);
                        }
                    }
                    auto value = GDToken(tkNUMBER, UtilityFunctions::str(temp));
                    PUSH_VAR(value)
                } else {
                    PUSH_VAR(tok_h)
                    PUSH_VAR(tok_g)
                    PUSH_VAR(tok_f)
                    PUSH_VAR(tok_e)
                    PUSH_VAR(tok_d)
                    PUSH_VAR(tok_c)
                    PUSH_VAR(tok_b)
                    PUSH_VAR(tok_a)
                    PUSH_VAR(e)
                }
            } break;

            case 10: {
                POP_VAR(tok_a, e.raw + " requires at least 10 parameter")
                POP_VAR(tok_b, e.raw + " requires at least 10 parameter")
                POP_VAR(tok_c, e.raw + " requires at least 10 parameter")
                POP_VAR(tok_d, e.raw + " requires at least 10 parameter")
                POP_VAR(tok_e, e.raw + " requires at least 10 parameter")
                POP_VAR(tok_f, e.raw + " requires at least 10 parameter")
                POP_VAR(tok_g, e.raw + " requires at least 10 parameter")
                POP_VAR(tok_h, e.raw + " requires at least 10 parameter")
                POP_VAR(tok_i, e.raw + " requires at least 10 parameter")
                POP_VAR(tok_j, e.raw + " requires at least 10 parameter")
                double temp = 0.0;
                if (tok_a.kind == tkNUMBER && tok_b.kind == tkNUMBER && tok_c.kind == tkNUMBER && tok_d.kind == tkNUMBER && tok_e.kind == tkNUMBER && tok_f.kind == tkNUMBER && tok_g.kind == tkNUMBER && tok_h.kind == tkNUMBER && tok_i.kind == tkNUMBER && tok_j.kind == tkNUMBER) {
                    double a = tok_a.raw.to_float();
                    double b = tok_b.raw.to_float();
                    double c = tok_c.raw.to_float();
                    double d = tok_d.raw.to_float();
                    double _e = tok_e.raw.to_float();
                    double f = tok_f.raw.to_float();
                    double g = tok_g.raw.to_float();
                    double h = tok_h.raw.to_float();
                    double i = tok_i.raw.to_float();
                    double j = tok_j.raw.to_float();
                    if (e.raw == "segment5") {
                        // j=t, h i g f e, d=d1, c=d2, b=d3, a=d4
                        if (j <= d) {
                            temp = clerpf(h, i, j / d);
                        } else if (j <= d + c) {
                            temp = clerpf(i, g, (j - d) / c);
                        } else if (j <= d + c + b) {
                            temp = clerpf(g, f, (j - d - c) / b);
                        } else {
                            temp = clerpf(f, _e, (j - d - c - b) / a);
                        }
                    }
                    auto value = GDToken(tkNUMBER, UtilityFunctions::str(temp));
                    PUSH_VAR(value)
                } else {
                    PUSH_VAR(tok_j)
                    PUSH_VAR(tok_i)
                    PUSH_VAR(tok_h)
                    PUSH_VAR(tok_g)
                    PUSH_VAR(tok_f)
                    PUSH_VAR(tok_e)
                    PUSH_VAR(tok_d)
                    PUSH_VAR(tok_c)
                    PUSH_VAR(tok_b)
                    PUSH_VAR(tok_a)
                    PUSH_VAR(e)
                }
            } break;
            }
        }
    }

    if (tape.empty()) {
        return String("");
    }

    tape_index -= 1;
    if (tape_index < 0) {
        return String("");
    }

    while (tape.size() > tape_index + 1) {
        tape.pop_back();
    }

    return rpn_to_infix(tape);
}

String godot::rpn_to_infix(const std::vector<GDToken>& tokens)
{
    struct Sf {
        int prec;
        String expr;

        Sf(int p, String e)
            : expr(e)
            , prec(p)
        {
        }
    };
    auto stack = std::vector<Sf>();
    for (auto tok : tokens) {
        if (tok.kind == tkOP) {
            if (stack.size() <= 1) {
                return String("");
            }
            auto rhs = std::move(stack[stack.size() - 1]);
            stack.pop_back();
            auto lhs = std::move(stack[stack.size() - 1]);
            auto tok_prec = godot::gd_operator_precedence(tok);
            if (lhs.prec < tok_prec || (lhs.prec == tok_prec && godot::gd_operator_is_right_associative(tok))) {
                lhs.expr = String("(") + lhs.expr + String(")");
                lhs.prec = 1000;
            }
            if (tok.raw == ".") {
                lhs.expr += tok.raw;
            } else {
                lhs.expr += " " + tok.raw + " ";
            }
            if (rhs.prec < tok_prec || (rhs.prec == tok_prec && !godot::gd_operator_is_right_associative(tok))) {
                lhs.expr += String("(") + rhs.expr + String(")");
                lhs.prec = 1000;
            } else {
                lhs.expr += rhs.expr;
                lhs.prec = tok_prec;
            }
            stack[stack.size() - 1] = std::move(lhs);
        } else if (tok.kind == tkPREFIX_OP) {
            if (stack.size() <= 0) {
                return String("");
            }
            auto rhs = std::move(stack[stack.size() - 1]);
            if (rhs.prec < 1000) {
                rhs.expr = "-(" + rhs.expr + ")";
            } else {
                rhs.expr = "-" + rhs.expr;
            }
            rhs.prec = 1000;
            stack[stack.size() - 1] = std::move(rhs);
        } else if (tok.kind == tkFUNC) {
            int arg_count = godot::number_of_func_arguments(tok.raw);
            if (stack.size() < arg_count) {
                return String("");
            }
            auto minor_stack = std::vector<Sf>();
            minor_stack.reserve(arg_count);
            String minor_result = tok.raw + "(";
            int i = 0;
            while (i < arg_count) {
                auto arg = std::move(stack[stack.size() - arg_count + i]);
                if (i < arg_count - 1) {
                    minor_result += arg.expr + ", ";
                } else {
                    minor_result += arg.expr + ")";
                }
                i += 1;
            }
            i = 0;
            while (i < arg_count) {
                stack.pop_back();
                i += 1;
            }
            // Sf n;
            // n.prec = 1000;
            // n.expr = minor_result;
            stack.emplace_back(1000, minor_result);
        } else {
            // Sf n;
            // n.prec = 1000;
            // n.expr = tok.raw;
            stack.emplace_back(1000, tok.raw);
        }
    }
    if (stack.size() > 0) {
        return stack[0].expr;
    } else {
        return String("");
    }
}