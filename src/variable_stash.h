#ifndef GDVARIABLE_STASH_H
#define GDVARIABLE_STASH_H

#include "token.h"
#include <godot_cpp/classes/ref_counted.hpp>
#include <vector>

namespace godot {

class Vars : public RefCounted {
    GDCLASS(Vars, RefCounted)

protected:
    std::array<float, tkvSIZE> scalars;
    std::array<Vector3, tkvvSIZE> vectors;
    Dictionary other;
    static void _bind_methods();

public:
    Vars();
    ~Vars();

    bool has(const GDToken& token) const;
    Variant get(GDToken token) const;
    void set(GDToken token, Variant value);

    bool has_nok(String var) const;
    Variant get_nok(String var) const;
    void set_nok(String var, Variant value);

    Variant get_raw(String token, Variant def) const;
    Variant get_raw_forced(String token) const;
    void set_raw(String token, Variant value);

    // float get_value(String var) const;
    // void set_value(String var, float value);
    bool has_value(GDTokenScalarVarKind var) const;
    float get_value(GDTokenScalarVarKind var) const;
    void set_value(GDTokenScalarVarKind var, float value);

    // Vector3 get_vector(String var) const;
    // void set_vector(String var, Vector3 value);
    bool has_vector(GDTokenVecVarKind var) const;
    Vector3 get_vector(GDTokenVecVarKind var) const;
    void set_vector(GDTokenVecVarKind var, Vector3 value);

    void copy_from(const Vars* other);
    void merge(const Vars* other, bool overwrite);

    void print_values();
};

}

VARIANT_ENUM_CAST(GDTokenScalarVarKind);
VARIANT_ENUM_CAST(GDTokenVecVarKind);

#endif