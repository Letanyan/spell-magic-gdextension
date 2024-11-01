#ifndef GDEXPR_OPTIMIZER_H
#define GDEXPR_OPTIMIZER_H

#include <godot_cpp/classes/ref_counted.hpp>

#include "token.h"
#include <vector>

namespace godot {

class GDExprTree {
public:
    ~GDExprTree();
    GDExprTree();
    GDExprTree(GDToken data);
    GDExprTree(GDToken data, GDToken left, GDToken right);
    GDExprTree(GDToken data, GDExprTree* left, GDExprTree* right);

    String display(String header = "", String padding = "");
    String format_as_expression();

    GDToken data;
    GDExprTree* left;
    GDExprTree* right;
};

GDExprTree* parse_sub_expr_tree(GDToken next, const std::vector<GDToken>& tokens, int& cursor);
GDExprTree* parse_binary_tree(GDToken op, GDExprTree* left, const std::vector<GDToken>& tokens, int& cursor);
GDExprTree* parse_expr_tree(const std::vector<GDToken>& tokens, int& cursor, int min_prec);
String optimize(const std::vector<GDToken>& tokens);

String constant_folding(const std::vector<GDToken>& expression, Dictionary map);
String rpn_to_infix(const std::vector<GDToken>& tokens);
}

#endif