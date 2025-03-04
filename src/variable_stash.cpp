#include "variable_stash.h"
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

Vars::Vars()
{
    scalars.fill(NAN);
    vectors.fill(Vector3(NAN, NAN, NAN));
    other = Dictionary();
}

Vars::~Vars()
{
}

bool Vars::has(const GDToken& token) const
{
    if (token.kind != tkVAR) {
        return false;
    }

    if (!token.is_var_vec && token.sub_kind != -1) {
        return !std::isnan(scalars[token.sub_kind]);
    }

    if (token.is_var_vec && token.sub_kind != -1) {
        return vectors[token.sub_kind].is_finite();
    }

    return other.has(token.raw);
}

Variant Vars::get(GDToken token) const
{
    if (token.kind != tkVAR) {
        return 0.0;
    }

    Variant result = 0.0;
    if (!token.is_var_vec && token.sub_kind != -1) {
        return scalars[token.sub_kind];
    }

    if (token.is_var_vec && token.sub_kind != -1) {
        return vectors[token.sub_kind];
    }

    if (other.has(token.raw)) {
        return other[token.raw];
    }

    return nullptr;
    // if (result.get_type() == Variant::Type::FLOAT || result.get_type() == Variant::Type::INT) {
    //     if (std::isnan((float)result)) {
    //         return 0.0;
    //     } else {
    //         return result;
    //     }
    // } else if (result.get_type() == Variant::VECTOR3) {
    //     if (((Vector3)result).is_finite()) {
    //         return result;
    //     } else {
    //         return Vector3();
    //     }
    // }
    // return 0.0;
}

void Vars::set(GDToken token, Variant value)
{
    if (token.kind != tkVAR) {
        return;
    }

    if (!token.is_var_vec && token.sub_kind != -1 && (value.get_type() == Variant::Type::FLOAT || value.get_type() == Variant::Type::INT)) {
        scalars[token.sub_kind] = value;
    }

    if (token.is_var_vec && token.sub_kind != -1 && value.get_type() == Variant::Type::VECTOR3) {
        vectors[token.sub_kind] = value;
    }

    other[token.raw] = value;
}

bool Vars::has_nok(String var) const
{
    GDTokenScalarVarKind scalar_kind = __var_scalar_kind_from_string___(var);
    if (scalar_kind != -1) {
        return !std::isnan(scalars[scalar_kind]);
    } else {
        GDTokenVecVarKind vec_kind = __var_vec_kind_from_string___(var);
        if (vec_kind != -1) {
            return vectors[vec_kind].is_finite();
        } else {
            return other.has(var);
        }
    }
}

Variant Vars::get_nok(String var) const
{
    GDTokenScalarVarKind scalar_kind = __var_scalar_kind_from_string___(var);
    Variant result = 0.0;
    if (scalar_kind != -1) {
        result = scalars[scalar_kind];
    } else {
        GDTokenVecVarKind vec_kind = __var_vec_kind_from_string___(var);
        if (vec_kind != -1) {
            result = vectors[vec_kind];
        } else if (other.has(var)) {
            result = other[var];
        } else {
            return 0.0;
        }
    }

    if (result.get_type() == Variant::Type::FLOAT || result.get_type() == Variant::Type::INT) {
        if (std::isnan((float)result)) {
            return 0.0;
        } else {
            return result;
        }
    } else if (result.get_type() == Variant::VECTOR3) {
        if (((Vector3)result).is_finite()) {
            return result;
        } else {
            return Vector3();
        }
    }
    return 0.0;
}

void Vars::set_nok(String var, Variant value)
{
    GDTokenScalarVarKind scalar_kind = __var_scalar_kind_from_string___(var);
    if ((value.get_type() == Variant::Type::FLOAT || value.get_type() == Variant::Type::INT) && scalar_kind != -1) {
        scalars[scalar_kind] = value;
    } else {
        GDTokenVecVarKind vec_kind = __var_vec_kind_from_string___(var);
        if (value.get_type() == Variant::Type::VECTOR3 && vec_kind != -1) {
            vectors[vec_kind] = value;
        } else {
            other[var] = value;
        }
    }
}

Variant Vars::get_raw(String var, Variant def) const
{
    if (other.has(var)) {
        return other[var];
    } else {
        return def;
    }
}

Variant Vars::get_raw_forced(String var) const
{
    return other[var];
}

void Vars::set_raw(String var, Variant value)
{
    other[var] = value;
}

// float Vars::get_value(String var) const
// {
//     GDTokenScalarVarKind kind = __var_scalar_kind_from_string___(var);
//     float result = 0.0;
//     if (kind != -1) {
//         result = scalars[kind];
//     } else if (other.has(var)) {
//         result = other[var];
//     } else {
//         return 0.0;
//     }

//     if (std::isnan(result)) {
//         return 0.0;
//     } else {
//         return result;
//     }
// }

// void Vars::set_value(String var, float value)
// {
//     GDTokenScalarVarKind kind = __var_scalar_kind_from_string___(var);
//     if (kind != -1) {
//         scalars[kind] = value;
//     } else {
//         other[var] = value;
//     }
// }

bool Vars::has_value(GDTokenScalarVarKind var) const
{
    return !std::isnan(scalars[var]);
}

float Vars::get_value(GDTokenScalarVarKind var) const
{
    float result = scalars[var];

    if (std::isnan(result)) {
        return 0.0;
    } else {
        return result;
    }
}

void Vars::set_value(GDTokenScalarVarKind var, float value)
{
    scalars[var] = value;
}

// Vector3 Vars::get_vector(String var) const
// {
//     GDTokenVecVarKind kind = __var_vec_kind_from_string___(var);
//     Vector3 result = Vector3();
//     if (kind != -1) {
//         result = vectors[kind];
//     } else if (other.has(var)) {
//         result = other[var];
//     } else {
//         return Vector3();
//     }

//     if (result.is_finite()) {
//         return result;
//     } else {
//         return Vector3();
//     }
// }

// void Vars::set_vector(String var, Vector3 value)
// {
//     GDTokenVecVarKind kind = __var_vec_kind_from_string___(var);
//     if (kind != -1) {
//         vectors[kind] = value;
//     } else if (other.has(var)) {
//         other[var] = value;
//     }
// }

bool Vars::has_vector(GDTokenVecVarKind var) const
{
    return vectors[var].is_finite();
}

Vector3 Vars::get_vector(GDTokenVecVarKind var) const
{
    Vector3 result = vectors[var];

    if (result.is_finite()) {
        return result;
    } else {
        return Vector3();
    }
}

void Vars::set_vector(GDTokenVecVarKind var, Vector3 value)
{
    vectors[var] = value;
}

void Vars::copy_from(const Vars* other)
{
    this->scalars = other->scalars;
    this->vectors = other->vectors;
    this->other = other->other.duplicate();
}

void Vars::merge(const Vars* other, bool overwrite)
{
    if (overwrite) {
        for (int16_t i = 0; i < tkvSIZE; i += 1) {
            if (!std::isnan(other->scalars[i])) {
                scalars[i] = other->scalars[i];
            }
        }
        for (int16_t i = 0; i < tkvvSIZE; i += 1) {
            if (other->vectors[i].is_finite()) {
                vectors[i] = other->vectors[i];
            }
        }
    } else {
        for (int16_t i = 0; i < tkvSIZE; i += 1) {
            if (std::isnan(scalars[i])) {
                scalars[i] = other->scalars[i];
            }
        }
        for (int16_t i = 0; i < tkvvSIZE; i += 1) {
            if (!vectors[i].is_finite()) {
                vectors[i] = other->vectors[i];
            }
        }
    }
    this->other.merge(other->other, overwrite);
}

void Vars::print_values()
{
    UtilityFunctions::print("---------------------");
    for (int16_t i = 0; i < tkvSIZE; i += 1) {
        UtilityFunctions::print(i, ": ", scalars[i]);
    }
    for (int16_t i = 0; i < tkvvSIZE; i += 1) {
        UtilityFunctions::print(i, ": ", vectors[i]);
    }
    UtilityFunctions::print(other);
}

void Vars::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("print_values"), &Vars::print_values);

    ClassDB::bind_method(D_METHOD("get_raw", "name", "default"), &Vars::get_raw);
    ClassDB::bind_method(D_METHOD("get_raw_now", "name"), &Vars::get_raw_forced);
    ClassDB::bind_method(D_METHOD("set_raw", "name", "value"), &Vars::set_raw);

    ClassDB::bind_method(D_METHOD("has", "var"), &Vars::has_nok);
    ClassDB::bind_method(D_METHOD("get", "name"), &Vars::get_nok);
    ClassDB::bind_method(D_METHOD("set", "name", "value"), &Vars::set_nok);

    ClassDB::bind_method(D_METHOD("has_value", "name"), &Vars::has_value);
    ClassDB::bind_method(D_METHOD("get_value", "name"), &Vars::get_value);
    ClassDB::bind_method(D_METHOD("set_value", "name", "value"), &Vars::set_value);

    ClassDB::bind_method(D_METHOD("has_vector", "name"), &Vars::has_vector);
    ClassDB::bind_method(D_METHOD("get_vector", "name"), &Vars::get_vector);
    ClassDB::bind_method(D_METHOD("set_vector", "name", "value"), &Vars::set_vector);

    ClassDB::bind_method(D_METHOD("copy_from", "other"), &Vars::copy_from);
    ClassDB::bind_method(D_METHOD("merge", "other", "overwrite"), &Vars::merge);

    ClassDB::bind_integer_constant("Vars", "tkvv", "Bxyz", tkvv_Bxyz);
    ClassDB::bind_integer_constant("Vars", "tkvv", "uvw", tkvv_uvw);
    ClassDB::bind_integer_constant("Vars", "tkvv", "tuvw", tkvv_tuvw);
    ClassDB::bind_integer_constant("Vars", "tkvv", "Tuvw", tkvv_Tuvw);
    ClassDB::bind_integer_constant("Vars", "tkvv", "ruvw", tkvv_ruvw);
    ClassDB::bind_integer_constant("Vars", "tkvv", "truvw", tkvv_truvw);
    ClassDB::bind_integer_constant("Vars", "tkvv", "Truvw", tkvv_Truvw);
    ClassDB::bind_integer_constant("Vars", "tkvv", "UVW", tkvv_UVW);
    ClassDB::bind_integer_constant("Vars", "tkvv", "tUVW", tkvv_tUVW);
    ClassDB::bind_integer_constant("Vars", "tkvv", "TUVW", tkvv_TUVW);
    ClassDB::bind_integer_constant("Vars", "tkvv", "rUVW", tkvv_rUVW);
    ClassDB::bind_integer_constant("Vars", "tkvv", "trUVW", tkvv_trUVW);
    ClassDB::bind_integer_constant("Vars", "tkvv", "TrUVW", tkvv_TrUVW);
    ClassDB::bind_integer_constant("Vars", "tkvv", "ijk", tkvv_ijk);
    ClassDB::bind_integer_constant("Vars", "tkvv", "tijk", tkvv_tijk);
    ClassDB::bind_integer_constant("Vars", "tkvv", "Tijk", tkvv_Tijk);
    ClassDB::bind_integer_constant("Vars", "tkvv", "rijk", tkvv_rijk);
    ClassDB::bind_integer_constant("Vars", "tkvv", "trijk", tkvv_trijk);
    ClassDB::bind_integer_constant("Vars", "tkvv", "Trijk", tkvv_Trijk);
    ClassDB::bind_integer_constant("Vars", "tkvv", "IJK", tkvv_IJK);
    ClassDB::bind_integer_constant("Vars", "tkvv", "tIJK", tkvv_tIJK);
    ClassDB::bind_integer_constant("Vars", "tkvv", "TIJK", tkvv_TIJK);
    ClassDB::bind_integer_constant("Vars", "tkvv", "rIJK", tkvv_rIJK);
    ClassDB::bind_integer_constant("Vars", "tkvv", "trIJK", tkvv_trIJK);
    ClassDB::bind_integer_constant("Vars", "tkvv", "TrIJK", tkvv_TrIJK);
    ClassDB::bind_integer_constant("Vars", "tkvv", "rel_pos", tkvv_rel_pos);
    ClassDB::bind_integer_constant("Vars", "tkvv", "abs_pos", tkvv_abs_pos);
    ClassDB::bind_integer_constant("Vars", "tkvv", "old_pos", tkvv_old_pos);

    ClassDB::bind_integer_constant("Vars", "tkv", "r0", tkv_r0);
    ClassDB::bind_integer_constant("Vars", "tkv", "r1", tkv_r1);
    ClassDB::bind_integer_constant("Vars", "tkv", "r2", tkv_r2);
    ClassDB::bind_integer_constant("Vars", "tkv", "r3", tkv_r3);
    ClassDB::bind_integer_constant("Vars", "tkv", "r4", tkv_r4);
    ClassDB::bind_integer_constant("Vars", "tkv", "r5", tkv_r5);
    ClassDB::bind_integer_constant("Vars", "tkv", "r6", tkv_r6);
    ClassDB::bind_integer_constant("Vars", "tkv", "r7", tkv_r7);
    ClassDB::bind_integer_constant("Vars", "tkv", "r8", tkv_r8);
    ClassDB::bind_integer_constant("Vars", "tkv", "r9", tkv_r9);
    ClassDB::bind_integer_constant("Vars", "tkv", "pi", tkv_pi);
    ClassDB::bind_integer_constant("Vars", "tkv", "tau", tkv_tau);
    ClassDB::bind_integer_constant("Vars", "tkv", "N", tkv_N);
    ClassDB::bind_integer_constant("Vars", "tkv", "M", tkv_M);
    ClassDB::bind_integer_constant("Vars", "tkv", "C", tkv_C);
    ClassDB::bind_integer_constant("Vars", "tkv", "L", tkv_L);
    ClassDB::bind_integer_constant("Vars", "tkv", "T", tkv_T);
    ClassDB::bind_integer_constant("Vars", "tkv", "P", tkv_P);
    ClassDB::bind_integer_constant("Vars", "tkv", "CR", tkv_CR);
    ClassDB::bind_integer_constant("Vars", "tkv", "CD", tkv_CD);
    ClassDB::bind_integer_constant("Vars", "tkv", "x", tkv_x);
    ClassDB::bind_integer_constant("Vars", "tkv", "y", tkv_y);
    ClassDB::bind_integer_constant("Vars", "tkv", "z", tkv_z);
    ClassDB::bind_integer_constant("Vars", "tkv", "r", tkv_r);
    ClassDB::bind_integer_constant("Vars", "tkv", "rn0", tkv_rn0);
    ClassDB::bind_integer_constant("Vars", "tkv", "rn1", tkv_rn1);
    ClassDB::bind_integer_constant("Vars", "tkv", "rn2", tkv_rn2);
    ClassDB::bind_integer_constant("Vars", "tkv", "rn3", tkv_rn3);
    ClassDB::bind_integer_constant("Vars", "tkv", "rn4", tkv_rn4);
    ClassDB::bind_integer_constant("Vars", "tkv", "rn5", tkv_rn5);
    ClassDB::bind_integer_constant("Vars", "tkv", "rn6", tkv_rn6);
    ClassDB::bind_integer_constant("Vars", "tkv", "rn7", tkv_rn7);
    ClassDB::bind_integer_constant("Vars", "tkv", "rn8", tkv_rn8);
    ClassDB::bind_integer_constant("Vars", "tkv", "rn9", tkv_rn9);
    ClassDB::bind_integer_constant("Vars", "tkv", "n", tkv_n);
    ClassDB::bind_integer_constant("Vars", "tkv", "D", tkv_D);
    ClassDB::bind_integer_constant("Vars", "tkv", "t", tkv_t);
    ClassDB::bind_integer_constant("Vars", "tkv", "l", tkv_l);
    ClassDB::bind_integer_constant("Vars", "tkv", "fl", tkv_fl);
    ClassDB::bind_integer_constant("Vars", "tkv", "Bx", tkv_Bx);
    ClassDB::bind_integer_constant("Vars", "tkv", "By", tkv_By);
    ClassDB::bind_integer_constant("Vars", "tkv", "Bz", tkv_Bz);
    ClassDB::bind_integer_constant("Vars", "tkv", "Br", tkv_Br);
    ClassDB::bind_integer_constant("Vars", "tkv", "tC", tkv_tC);
    ClassDB::bind_integer_constant("Vars", "tkv", "TC", tkv_TC);
    ClassDB::bind_integer_constant("Vars", "tkv", "u", tkv_u);
    ClassDB::bind_integer_constant("Vars", "tkv", "v", tkv_v);
    ClassDB::bind_integer_constant("Vars", "tkv", "w", tkv_w);
    ClassDB::bind_integer_constant("Vars", "tkv", "tu", tkv_tu);
    ClassDB::bind_integer_constant("Vars", "tkv", "tv", tkv_tv);
    ClassDB::bind_integer_constant("Vars", "tkv", "tw", tkv_tw);
    ClassDB::bind_integer_constant("Vars", "tkv", "Tu", tkv_Tu);
    ClassDB::bind_integer_constant("Vars", "tkv", "Tv", tkv_Tv);
    ClassDB::bind_integer_constant("Vars", "tkv", "Tw", tkv_Tw);
    ClassDB::bind_integer_constant("Vars", "tkv", "ru", tkv_ru);
    ClassDB::bind_integer_constant("Vars", "tkv", "rv", tkv_rv);
    ClassDB::bind_integer_constant("Vars", "tkv", "rw", tkv_rw);
    ClassDB::bind_integer_constant("Vars", "tkv", "tru", tkv_tru);
    ClassDB::bind_integer_constant("Vars", "tkv", "trv", tkv_trv);
    ClassDB::bind_integer_constant("Vars", "tkv", "trw", tkv_trw);
    ClassDB::bind_integer_constant("Vars", "tkv", "Tru", tkv_Tru);
    ClassDB::bind_integer_constant("Vars", "tkv", "Trv", tkv_Trv);
    ClassDB::bind_integer_constant("Vars", "tkv", "Trw", tkv_Trw);
    ClassDB::bind_integer_constant("Vars", "tkv", "U", tkv_U);
    ClassDB::bind_integer_constant("Vars", "tkv", "V", tkv_V);
    ClassDB::bind_integer_constant("Vars", "tkv", "W", tkv_W);
    ClassDB::bind_integer_constant("Vars", "tkv", "tU", tkv_tU);
    ClassDB::bind_integer_constant("Vars", "tkv", "tV", tkv_tV);
    ClassDB::bind_integer_constant("Vars", "tkv", "tW", tkv_tW);
    ClassDB::bind_integer_constant("Vars", "tkv", "TU", tkv_TU);
    ClassDB::bind_integer_constant("Vars", "tkv", "TV", tkv_TV);
    ClassDB::bind_integer_constant("Vars", "tkv", "TW", tkv_TW);
    ClassDB::bind_integer_constant("Vars", "tkv", "rU", tkv_rU);
    ClassDB::bind_integer_constant("Vars", "tkv", "rV", tkv_rV);
    ClassDB::bind_integer_constant("Vars", "tkv", "rW", tkv_rW);
    ClassDB::bind_integer_constant("Vars", "tkv", "trU", tkv_trU);
    ClassDB::bind_integer_constant("Vars", "tkv", "trV", tkv_trV);
    ClassDB::bind_integer_constant("Vars", "tkv", "trW", tkv_trW);
    ClassDB::bind_integer_constant("Vars", "tkv", "TrU", tkv_TrU);
    ClassDB::bind_integer_constant("Vars", "tkv", "TrV", tkv_TrV);
    ClassDB::bind_integer_constant("Vars", "tkv", "TrW", tkv_TrW);
    ClassDB::bind_integer_constant("Vars", "tkv", "i", tkv_i);
    ClassDB::bind_integer_constant("Vars", "tkv", "j", tkv_j);
    ClassDB::bind_integer_constant("Vars", "tkv", "k", tkv_k);
    ClassDB::bind_integer_constant("Vars", "tkv", "ti", tkv_ti);
    ClassDB::bind_integer_constant("Vars", "tkv", "tj", tkv_tj);
    ClassDB::bind_integer_constant("Vars", "tkv", "tk", tkv_tk);
    ClassDB::bind_integer_constant("Vars", "tkv", "Ti", tkv_Ti);
    ClassDB::bind_integer_constant("Vars", "tkv", "Tj", tkv_Tj);
    ClassDB::bind_integer_constant("Vars", "tkv", "Tk", tkv_Tk);
    ClassDB::bind_integer_constant("Vars", "tkv", "ri", tkv_ri);
    ClassDB::bind_integer_constant("Vars", "tkv", "rj", tkv_rj);
    ClassDB::bind_integer_constant("Vars", "tkv", "rk", tkv_rk);
    ClassDB::bind_integer_constant("Vars", "tkv", "tri", tkv_tri);
    ClassDB::bind_integer_constant("Vars", "tkv", "trj", tkv_trj);
    ClassDB::bind_integer_constant("Vars", "tkv", "trk", tkv_trk);
    ClassDB::bind_integer_constant("Vars", "tkv", "Tri", tkv_Tri);
    ClassDB::bind_integer_constant("Vars", "tkv", "Trj", tkv_Trj);
    ClassDB::bind_integer_constant("Vars", "tkv", "Trk", tkv_Trk);
    ClassDB::bind_integer_constant("Vars", "tkv", "I", tkv_I);
    ClassDB::bind_integer_constant("Vars", "tkv", "J", tkv_J);
    ClassDB::bind_integer_constant("Vars", "tkv", "K", tkv_K);
    ClassDB::bind_integer_constant("Vars", "tkv", "tI", tkv_tI);
    ClassDB::bind_integer_constant("Vars", "tkv", "tJ", tkv_tJ);
    ClassDB::bind_integer_constant("Vars", "tkv", "tK", tkv_tK);
    ClassDB::bind_integer_constant("Vars", "tkv", "TI", tkv_TI);
    ClassDB::bind_integer_constant("Vars", "tkv", "TJ", tkv_TJ);
    ClassDB::bind_integer_constant("Vars", "tkv", "TK", tkv_TK);
    ClassDB::bind_integer_constant("Vars", "tkv", "rI", tkv_rI);
    ClassDB::bind_integer_constant("Vars", "tkv", "rJ", tkv_rJ);
    ClassDB::bind_integer_constant("Vars", "tkv", "rK", tkv_rK);
    ClassDB::bind_integer_constant("Vars", "tkv", "trI", tkv_trI);
    ClassDB::bind_integer_constant("Vars", "tkv", "trJ", tkv_trJ);
    ClassDB::bind_integer_constant("Vars", "tkv", "trK", tkv_trK);
    ClassDB::bind_integer_constant("Vars", "tkv", "TrI", tkv_TrI);
    ClassDB::bind_integer_constant("Vars", "tkv", "TrJ", tkv_TrJ);
    ClassDB::bind_integer_constant("Vars", "tkv", "TrK", tkv_TrK);
    ClassDB::bind_integer_constant("Vars", "tkv", "frame_time", tkv_frame_time);
}