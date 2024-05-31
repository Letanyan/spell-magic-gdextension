#include "expr_optimizer.h"
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

GDExprTree* build_right_tree(std::vector<GDToken> tokens, int& cursor)
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

GDExprTree* make_single_binary_tree(GDExprTree* left, std::vector<GDToken> tokens, int& cursor)
{
    GDToken op = tokens[cursor++];
    if (op.kind == tkOP) {
        GDExprTree* right = new GDExprTree(tokens[cursor++]);
        return new GDExprTree(op, left, right);
    } else {
        return left;
    }
}

GDExprTree* build_left_tree(std::vector<GDToken> tokens, int& cursor)
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

GDExprTree* godot::parse_sub_expr_tree(GDToken next, std::vector<GDToken> tokens, int& cursor)
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

GDExprTree* godot::parse_binary_tree(GDToken op, GDExprTree* left, std::vector<GDToken> tokens, int& cursor)
{
    auto prec = gd_operator_precedence(op);
    if (gd_operator_is_right_associative(op)) {
        prec -= 1;
    }
    auto right = parse_expr_tree(tokens, cursor, prec);
    return new GDExprTree(op, left, right);
}

GDExprTree* godot::parse_expr_tree(std::vector<GDToken> tokens, int& cursor, int min_prec)
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

String godot::optimize(std::vector<GDToken> tokens)
{
    int cursor = 0;
    auto tree = parse_expr_tree(tokens, cursor, 0);
    tree = expr_tree_constant_folding(tree);
    // String result = tree->display();
    String result = tree->format_as_expression();
    delete tree;
    return result;
}