#ifndef GDTOKEN_H
#define GDTOKEN_H

#include <string>
#include <vector>

namespace godot {

enum GDTokenKind {
    tkNUMBER,
    tkWORD,
    tkVAR,
    tkFUNC,
    tkOP,
    tkOPEN,
    tkCLOSE,
    tkCOMMA,
    tkPREFIX_OP,
    tkNONE,
    tkERROR
};

struct GDToken {
public:
    GDToken(GDTokenKind _kind, std::string _raw);
    ~GDToken();

    GDTokenKind kind;
    std::string raw;
};

std::vector<GDToken> tokenize(std::string expr);

}

#endif