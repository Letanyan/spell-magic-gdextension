#include "token.h"

#include <vector>

using namespace godot;

GDToken::GDToken(GDTokenKind _kind, std::string _raw)
{
    // Initialize any variables here.
    kind = _kind;
    raw = _raw;
}

GDToken::~GDToken()
{
    // Add your cleanup here.
}

bool is_func(std::string name)
{
    if (name == "sin" || name == "cos" || name == "tan" || name == "asin" || name == "acos" || name == "atan") {
        return true;
    }
    if (name == "sinh" || name == "cosh" || name == "tanh" || name == "asinh" || name == "acosh" || name == "atanh") {
        return true;
    }
    if (name == "inv" || name == "mod" || name == "div" || name == "floor" || name == "ceil" || name == "round") {
        return true;
    }
    if (name == "max" || name == "min" || name == "lt" || name == "gt" || name == "lte" || name == "gte" || name == "eq" || name == "neq") {
        return true;
    }
    return false;
}

std::vector<GDToken> godot::tokenize(std::string expr)
{
    GDTokenKind state = tkNONE;
    auto result = std::vector<GDToken>();

    std::string current = "";
    bool last_was_op = true;
    std::string digits = "1234567890";
    std::string alpha_num = "qwertyuiopasdfghjklzxcvbnm1234567890QWERTYUIOPASDFGHJKLZXCVBNM";
    std::string alpha = "qwertyuiopasdfghjklzxcvbnmQWERTYUIOPASDFGHJKLZXCVBNM";
    std::string operators = "+-/*^";
    for (char& c : expr) {
        switch (state) {
        case tkNUMBER:
            if (digits.find(c) != std::string::npos || c == '.') {
                current.push_back(c);
            } else {
                result.push_back(GDToken(tkNUMBER, std::string(current)));
                last_was_op = false;
                current.clear();
                state = tkNONE;
            }
            break;
        case tkWORD:
            if (alpha_num.find(c) != std::string::npos) {
                current.push_back(c);
            } else {
                GDTokenKind k = is_func(current) ? tkFUNC : tkVAR;
                result.push_back(GDToken(k, std::string(current)));
                last_was_op = false;
                current.clear();
                state = tkNONE;
            }
        default:
            break;
        }

        switch (state) {
        case tkNONE:
            if (digits.find(c) != std::string::npos) {
                state = tkNUMBER;
                current.push_back(c);
            } else if (alpha.find(c) != std::string::npos) {
                state = tkWORD;
                current.push_back(c);
            } else if (operators.find(c) != std::string::npos) {
                current.push_back(c);
                if (last_was_op && (c == '+' || c == '-')) {
                    result.push_back(GDToken(tkPREFIX_OP, std::string(current)));
                } else {
                    result.push_back(GDToken(tkOP, std::string(current)));
                    last_was_op = true;
                }
                current.clear();
            } else if (c == '(') {
                current.push_back(c);
                result.push_back(GDToken(tkOPEN, std::string(current)));
                current.clear();
                last_was_op = true;
            } else if (c == ')') {
                current.push_back(c);
                result.push_back(GDToken(tkCLOSE, std::string(current)));
                current.clear();
                last_was_op = false;
            } else if (c == ',') {
                current.push_back(c);
                result.push_back(GDToken(tkCOMMA, std::string(current)));
                current.clear();
                last_was_op = true;
            } else if (c == ' ') {

            } else {
                current.push_back(c);
                result.push_back(GDToken(tkERROR, std::string(current)));
                current.clear();
                last_was_op = false;
            }
        default:
            break;
        }
    }

    if (current.size() > 0) {
        switch (state) {
        case tkNUMBER:
            result.push_back(GDToken(tkNUMBER, std::string(current)));
            break;
        case tkWORD: {
            GDTokenKind k = is_func(current) ? tkFUNC : tkVAR;
            result.push_back(GDToken(k, std::string(current)));
        } break;
        default:
            break;
        }
    }

    return result;
}