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

    GDToken data;
    GDExprTree* left;
    GDExprTree* right;
};

GDExprTree* parse_sub_expr_tree(GDToken next, std::vector<GDToken> tokens, int& cursor);
GDExprTree* parse_binary_tree(GDToken op, GDExprTree* left, std::vector<GDToken> tokens, int& cursor);
GDExprTree* parse_expr_tree(std::vector<GDToken> tokens, int& cursor, int min_prec);
String optimize(std::vector<GDToken> tokens);
}

#endif