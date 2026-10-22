// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Type system used by DFG
//
// Code available from: https://verilator.org
//
//*************************************************************************
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of either the GNU Lesser General Public License Version 3
// or the Perl Artistic License Version 2.0.
// SPDX-FileCopyrightText: 2003-2026 Wilson Snyder
// SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0
//
//*************************************************************************
//
// DFG uses a restriced and simplified type system compared to Ast. Each
// DfgVertex has a type, describing the type of the value it represents.
// The DFG types are represented by an instance of DfgDataType defined below.
//
// The type can be one of
// - Null: The vertex doesn't actually represent a value directly
// - Packed: All bit-vector like types, including all packed construct
//   and binary encoded numbers. Packed types are always 1-dimensional
//   with no sub-type. E.g.: a packed array of N copies of M-wide elements
//   is simply represented as a Packed type of size N*M.
// - Array: An unpacked array of elements with the same type.
//
// Each type has a size:
// - Null: 0, though this is largely irrelevant
// - Packed: number of bits in the packed value (minimum 1)
// - Array: number of elements in the arrray
//
// The indexing of Packed and Array are both zero based and 'descending' in
// the Systemverilog sense (index 0 is the LSB of a Packed, index 0 is at the
// lowest memory address for Array).
//
// All types are 'interned', that is, there can only be one instance of
// DfgDataType which represents that type. This means two DfgDataType are
// equal if and only if they are the same DfgDataType instance.
//
//*************************************************************************

#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ VERILATOR_V3DFGDATATYPE_H_
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VERILATOR_V3DFGDATATYPE_H_

#ਅਨ੍ਤਰ੍ਭਾਵ "config_build.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "verilatedos.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Ast.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Error.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Global.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Hash.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <memory>
#ਅਨ੍ਤਰ੍ਭਾਵ <unordered_map>

class Dfg_ਅੰਕੜੇ_Type final {
    ਗਿਣਤੀ class ਕਿਸਮ_2 : uint8_t {
        Packed,
        Array,
        // Tuple - to model unpacked structs in the future
        Null  // No type (like 'void', but that's a C++ keyword)
    };

    // STATE
    ਅਚਲ ਕਿਸਮ_2 m_ਕਿਸਮ;  // The type category
    uint32_t m_ਮਾਪ;  // The number of elements in this type
    Ast_ਗੰਢ_DType* ਅਚਲ m_astDtypep;  // Equivalent canonical AstNodeDType
    ਅਚਲ Dfg_ਅੰਕੜੇ_Type* ਅਚਲ m_elemDtypep;  // Type of elements - for arrays only
    // Static singleton Null type
    ਸਥਿਰ ਅਚਲ Dfg_ਅੰਕੜੇ_Type* s_nullTypep;
    // Static map from 'width' -> 'interned Packed DfgDataType with that width'
    ਸਥਿਰ std::unordered_map<uint32_t, ਅਚਲ Dfg_ਅੰਕੜੇ_Type*> s_packedTypes;
    // Map from 'elements' -> 'interned Array DfgDataType with that many elements of *this* type'
    mutable std::unordered_map<uint32_t, ਅਚਲ Dfg_ਅੰਕੜੇ_Type*> m_arrayTypes;

    // METHODS
    ਸਥਿਰ Ast_ਗੰਢ_DType* canonicalPackedDType(uint32_t ਚੌੜਾਈ) {
        ਵਾਪਸ v3Global.rootp()->typeTablep()->ਲੱਭੋ_ਦੋਆਧਾਰੀ_ਅੰਕ_DType(ਚੌੜਾਈ, ਚੌੜਾਈ, VSigning::UNSIGNED);
    }
    ਸਥਿਰ Ast_ਗੰਢ_DType* canonicalArrayDType(uint32_t ਮਾਪ, ਅਚਲ Dfg_ਅੰਕੜੇ_Type& elemType) {
        Ast_ਗੰਢ_DType* ਅਚਲ elemDTypep = elemType.m_astDtypep;
        File_ਸਤਰ* ਅਚਲ flp = elemDTypep->fileline();
        AstRange* ਅਚਲ rangep = new AstRange{flp, static_cast<ਪੂਰਨ>(ਮਾਪ - 1), 0};
        Ast_ਗੰਢ_DType* ਅਚਲ dtypep = new AstUnpackArrayDType{flp, elemDTypep, rangep};
        v3Global.rootp()->typeTablep()->ਜੋੜੋ_Typesp(dtypep);
        ਵਾਪਸ dtypep;
    }

    // CONSTRUCTOR - use 'fromAst' instead
    Dfg_ਅੰਕੜੇ_Type()
        : m_ਕਿਸਮ{ਕਿਸਮ_2::Null}
        , m_ਮਾਪ{0}
        , m_astDtypep{v3Global.rootp()->ਲੱਭੋ_Void_DType()}
        , m_elemDtypep{nullptr} {}
    explicit Dfg_ਅੰਕੜੇ_Type(uint32_t ਮਾਪ)
        : m_ਕਿਸਮ{ਕਿਸਮ_2::Packed}
        , m_ਮਾਪ{ਮਾਪ}
        , m_astDtypep{canonicalPackedDType(ਮਾਪ)}
        , m_elemDtypep{nullptr} {}
    Dfg_ਅੰਕੜੇ_Type(uint32_t ਮਾਪ, ਅਚਲ Dfg_ਅੰਕੜੇ_Type& elemType)
        : m_ਕਿਸਮ{ਕਿਸਮ_2::Array}
        , m_ਮਾਪ{ਮਾਪ}
        , m_astDtypep{canonicalArrayDType(ਮਾਪ, elemType)}
        , m_elemDtypep{&elemType} {}

    VL_UNCOPYABLE(Dfg_ਅੰਕੜੇ_Type);
    VL_UNMOVABLE(Dfg_ਅੰਕੜੇ_Type);
    ~Dfg_ਅੰਕੜੇ_Type() {
        ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& pair : m_arrayTypes) VL_DO_DANGLING(delete pair.ਦੂਜਾ, pair.ਦੂਜਾ);
    }

ਜਨਤਕ:
    //-----------------------------------------------------------------------
    // Properties

    bool isNull() ਅਚਲ { ਵਾਪਸ m_ਕਿਸਮ == ਕਿਸਮ_2::Null; }
    bool isPacked() ਅਚਲ { ਵਾਪਸ m_ਕਿਸਮ == ਕਿਸਮ_2::Packed; }
    bool isArray() ਅਚਲ { ਵਾਪਸ m_ਕਿਸਮ == ਕਿਸਮ_2::Array; }
    // Size of type (this is 'width' for Ppacked, 'elements' for Array, 0 for Null)
    uint32_t ਮਾਪ() ਅਚਲ { ਵਾਪਸ m_ਮਾਪ; }
    Ast_ਗੰਢ_DType* astDtypep() ਅਚਲ { ਵਾਪਸ m_astDtypep; }

    // Thanks to the interning, equality is identity
    bool ਸੰਚਾਲਕ==(ਅਚਲ Dfg_ਅੰਕੜੇ_Type& that) ਅਚਲ { ਵਾਪਸ this == &that; }
    bool ਸੰਚਾਲਕ!=(ਅਚਲ Dfg_ਅੰਕੜੇ_Type& that) ਅਚਲ { ਵਾਪਸ this != &that; }
    // Similarly for hash
    V3Hash hash() ਅਚਲ { ਵਾਪਸ V3Hash{this}; }

    // Type of elements, for arrays only
    ਅਚਲ Dfg_ਅੰਕੜੇ_Type& elemDtype() ਅਚਲ {
        UASSERT(isArray(), "Non-array has no 'elemDType'");
        ਵਾਪਸ *m_elemDtypep;
    }

    //-----------------------------------------------------------------------
    // Static factory and management functions

    // Returns a Packed type of the given width
    ਸਥਿਰ ਅਚਲ Dfg_ਅੰਕੜੇ_Type& packed(uint32_t ਚੌੜਾਈ) {
        UASSERT(ਚੌੜਾਈ > 0, "Width must be positive");
        // Find or create the right sized packed type
        ਅਚਲ Dfg_ਅੰਕੜੇ_Type*& entryr = s_packedTypes[ਚੌੜਾਈ];
        ਜੇ (!entryr) entryr = new Dfg_ਅੰਕੜੇ_Type{ਚੌੜਾਈ};
        ਵਾਪਸ *entryr;
    }

    // Returns an Array type of the given size with the given elements
    ਸਥਿਰ ਅਚਲ Dfg_ਅੰਕੜੇ_Type& array(ਅਚਲ Dfg_ਅੰਕੜੇ_Type& elemType, uint32_t ਮਾਪ) {
        UASSERT(elemType.isPacked(), "Cannot create multi-dimensional arrays yet");
        // Find or create the right sized array type with this as elements
        ਅਚਲ Dfg_ਅੰਕੜੇ_Type*& entryr = elemType.m_arrayTypes[ਮਾਪ];
        ਜੇ (!entryr) entryr = new Dfg_ਅੰਕੜੇ_Type{ਮਾਪ, elemType};
        ਵਾਪਸ *entryr;
    }

    // Returns the singleton Null type
    ਸਥਿਰ ਅਚਲ Dfg_ਅੰਕੜੇ_Type& null() {
        ਜੇ (!s_nullTypep) s_nullTypep = new Dfg_ਅੰਕੜੇ_Type{};
        ਵਾਪਸ *s_nullTypep;
    }

    // Returns the data type of selecting the range [lo + size - 1 : lo] from this type
    ਸਥਿਰ ਅਚਲ Dfg_ਅੰਕੜੇ_Type& select(ਅਚਲ Dfg_ਅੰਕੜੇ_Type& dtype, uint32_t lo, uint32_t ਮਾਪ) {
        ਚੋਣ (dtype.m_ਕਿਸਮ) {
        ਹਾਲਤ ਕਿਸਮ_2::Packed: {
            UASSERT(lo + ਮਾਪ - 1 < dtype.ਮਾਪ(), "Out of range");
            ਵਾਪਸ Dfg_ਅੰਕੜੇ_Type::packed(ਮਾਪ);
        }
        ਹਾਲਤ ਕਿਸਮ_2::Array: {
            UASSERT(lo + ਮਾਪ - 1 < dtype.ਮਾਪ(), "Out of range");
            ਵਾਪਸ Dfg_ਅੰਕੜੇ_Type::array(dtype.elemDtype(), ਮਾਪ);
        }
        ਹਾਲਤ ਕਿਸਮ_2::Null: v3fatal("Type cannot be selected from");  // LCOV_EXCL_LINE
        }
        VL_UNREACHABLE;
    }

    // Construct a DfgDataType that represents the given AstNodeDType.
    // Returns nullptr if AstNodeDType is not representable by DfgDataType.
    ਸਥਿਰ ਅਚਲ Dfg_ਅੰਕੜੇ_Type* fromAst(ਅਚਲ Ast_ਗੰਢ_DType* dtypep) {
        dtypep = dtypep->skipRefp();

        ਜੇ (ਅਚਲ AstUnpackArrayDType* ਅਚਲ typep = VN_CAST(dtypep, UnpackArrayDType)) {
            // Convert the element type
            ਅਚਲ Dfg_ਅੰਕੜੇ_Type* ਅਚਲ elemp = fromAst(typep->subDTypep());
            // If element type is not supported, we cannot handle the array either
            ਜੇ (!elemp) ਵਾਪਸ nullptr;
            // Currently restricted to 1-dimensional arrays, just assumptions in the code
            ਜੇ (elemp->isArray()) ਵਾਪਸ nullptr;
            // Unpacked array maps to Array
            ਵਾਪਸ &array(*elemp, typep->elementsConst());
        }

        ਜੇ (ਅਚਲ AstPackArrayDType* ਅਚਲ typep = VN_CAST(dtypep, PackArrayDType)) {
            // Packed array maps to Packed
            ਵਾਪਸ &packed(typep->ਚੌੜਾਈ());
        }

        ਜੇ (ਅਚਲ AstStructDType* ਅਚਲ typep = VN_CAST(dtypep, StructDType)) {
            // Unpacked structs currently not supported
            ਜੇ (!typep->packed()) ਵਾਪਸ nullptr;
            // Packed struct maps to Packed
            ਵਾਪਸ &packed(typep->ਚੌੜਾਈ());
        }

        ਜੇ (ਅਚਲ AstUnionDType* ਅਚਲ typep = VN_CAST(dtypep, UnionDType)) {
            // Unpacked unions not supported
            ਜੇ (!typep->packed()) ਵਾਪਸ nullptr;
            // Packed union maps to Packed
            ਵਾਪਸ &packed(typep->ਚੌੜਾਈ());
        }

        ਜੇ (ਅਚਲ AstBasicDType* ਅਚਲ typep = VN_CAST(dtypep, BasicDType)) {
            // Anything not resembling a binary encoded number is not supported
            ਜੇ (!typep->keyword().isIntNumeric()) ਵਾਪਸ nullptr;
            // Basic numeric types map to Packed
            ਵਾਪਸ &packed(typep->ਚੌੜਾਈ());
        }

        // Anything else is not representable
        ਵਾਪਸ nullptr;
    }

    // Reset interned types
    ਸਥਿਰ ਖਾਲੀ reset() {
        ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& pair : s_packedTypes) VL_DO_DANGLING(delete pair.ਦੂਜਾ, pair.ਦੂਜਾ);
        s_packedTypes.clear();
        delete s_nullTypep;
        s_nullTypep = nullptr;
    }
};

#ਜੇ_ਅੰਤ
