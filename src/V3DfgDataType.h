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

#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ VERILATOR_V3DFGDATATYPE_H_
#ವ್ಯಾಖ್ಯಾನಿಸು VERILATOR_V3DFGDATATYPE_H_

#ಅನ್ತರ್ಭಾವ "config_build.h"
#ಅನ್ತರ್ಭಾವ "verilatedos.h"

#ಅನ್ತರ್ಭಾವ "V3Ast.h"
#ಅನ್ತರ್ಭಾವ "V3Error.h"
#ಅನ್ತರ್ಭಾವ "V3Global.h"
#ಅನ್ತರ್ಭಾವ "V3Hash.h"

#ಅನ್ತರ್ಭಾವ <memory>
#ಅನ್ತರ್ಭಾವ <unordered_map>

class Dfg_ದತ್ತಾಂಶ_Type final {
    ಎಣಿಕೆ class ಬಗೆ_2 : uint8_t {
        Packed,
        Array,
        // Tuple - to model unpacked structs in the future
        Null  // No type (like 'void', but that's a C++ keyword)
    };

    // STATE
    ಸ್ಥಿರಾಂಕ ಬಗೆ_2 m_ಬಗೆ;  // The type category
    uint32_t m_ಗಾತ್ರ;  // The number of elements in this type
    Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ m_astDtypep;  // Equivalent canonical AstNodeDType
    ಸ್ಥಿರಾಂಕ Dfg_ದತ್ತಾಂಶ_Type* ಸ್ಥಿರಾಂಕ m_elemDtypep;  // Type of elements - for arrays only
    // Static singleton Null type
    ಸ್ಥಿರ ಸ್ಥಿರಾಂಕ Dfg_ದತ್ತಾಂಶ_Type* s_nullTypep;
    // Static map from 'width' -> 'interned Packed DfgDataType with that width'
    ಸ್ಥಿರ std::unordered_map<uint32_t, ಸ್ಥಿರಾಂಕ Dfg_ದತ್ತಾಂಶ_Type*> s_packedTypes;
    // Map from 'elements' -> 'interned Array DfgDataType with that many elements of *this* type'
    mutable std::unordered_map<uint32_t, ಸ್ಥಿರಾಂಕ Dfg_ದತ್ತಾಂಶ_Type*> m_arrayTypes;

    // METHODS
    ಸ್ಥಿರ Ast_ಗಂಟು_DType* canonicalPackedDType(uint32_t ಅಗಲ) {
        ಹಿಂತಿರುಗಿ v3Global.rootp()->typeTablep()->ಕಂಡುಹಿಡಿ_ಬಿಟ್_DType(ಅಗಲ, ಅಗಲ, VSigning::UNSIGNED);
    }
    ಸ್ಥಿರ Ast_ಗಂಟು_DType* canonicalArrayDType(uint32_t ಗಾತ್ರ_2, ಸ್ಥಿರಾಂಕ Dfg_ದತ್ತಾಂಶ_Type& elemType) {
        Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ elemDTypep = elemType.m_astDtypep;
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = elemDTypep->fileline();
        AstRange* ಸ್ಥಿರಾಂಕ rangep = new AstRange{flp, static_cast<ಪೂರ್ಣಾಂಕ>(ಗಾತ್ರ_2 - 1), 0};
        Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ dtypep = new AstUnpackArrayDType{flp, elemDTypep, rangep};
        v3Global.rootp()->typeTablep()->ಕೂಡಿಸು_Typesp(dtypep);
        ಹಿಂತಿರುಗಿ dtypep;
    }

    // CONSTRUCTOR - use 'fromAst' instead
    Dfg_ದತ್ತಾಂಶ_Type()
        : m_ಬಗೆ{ಬಗೆ_2::Null}
        , m_ಗಾತ್ರ{0}
        , m_astDtypep{v3Global.rootp()->ಕಂಡುಹಿಡಿ_Void_DType()}
        , m_elemDtypep{nullptr} {}
    explicit Dfg_ದತ್ತಾಂಶ_Type(uint32_t ಗಾತ್ರ_2)
        : m_ಬಗೆ{ಬಗೆ_2::Packed}
        , m_ಗಾತ್ರ{ಗಾತ್ರ_2}
        , m_astDtypep{canonicalPackedDType(ಗಾತ್ರ_2)}
        , m_elemDtypep{nullptr} {}
    Dfg_ದತ್ತಾಂಶ_Type(uint32_t ಗಾತ್ರ_2, ಸ್ಥಿರಾಂಕ Dfg_ದತ್ತಾಂಶ_Type& elemType)
        : m_ಬಗೆ{ಬಗೆ_2::Array}
        , m_ಗಾತ್ರ{ಗಾತ್ರ_2}
        , m_astDtypep{canonicalArrayDType(ಗಾತ್ರ_2, elemType)}
        , m_elemDtypep{&elemType} {}

    VL_UNCOPYABLE(Dfg_ದತ್ತಾಂಶ_Type);
    VL_UNMOVABLE(Dfg_ದತ್ತಾಂಶ_Type);
    ~Dfg_ದತ್ತಾಂಶ_Type() {
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& pair : m_arrayTypes) VL_DO_DANGLING(delete pair.ಎರಡನೇ, pair.ಎರಡನೇ);
    }

ಸಾರ್ವಜನಿಕ:
    //-----------------------------------------------------------------------
    // Properties

    bool isNull() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಬಗೆ == ಬಗೆ_2::Null; }
    bool isPacked() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಬಗೆ == ಬಗೆ_2::Packed; }
    bool isArray() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಬಗೆ == ಬಗೆ_2::Array; }
    // Size of type (this is 'width' for Ppacked, 'elements' for Array, 0 for Null)
    uint32_t ಗಾತ್ರ_2() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಗಾತ್ರ; }
    Ast_ಗಂಟು_DType* astDtypep() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_astDtypep; }

    // Thanks to the interning, equality is identity
    bool ಕ್ರಿಯಾಕಾರಕ==(ಸ್ಥಿರಾಂಕ Dfg_ದತ್ತಾಂಶ_Type& that) ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ this == &that; }
    bool ಕ್ರಿಯಾಕಾರಕ!=(ಸ್ಥಿರಾಂಕ Dfg_ದತ್ತಾಂಶ_Type& that) ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ this != &that; }
    // Similarly for hash
    V3Hash hash() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ V3Hash{this}; }

    // Type of elements, for arrays only
    ಸ್ಥಿರಾಂಕ Dfg_ದತ್ತಾಂಶ_Type& elemDtype() ಸ್ಥಿರಾಂಕ {
        UASSERT(isArray(), "Non-array has no 'elemDType'");
        ಹಿಂತಿರುಗಿ *m_elemDtypep;
    }

    //-----------------------------------------------------------------------
    // Static factory and management functions

    // Returns a Packed type of the given width
    ಸ್ಥಿರ ಸ್ಥಿರಾಂಕ Dfg_ದತ್ತಾಂಶ_Type& packed(uint32_t ಅಗಲ) {
        UASSERT(ಅಗಲ > 0, "Width must be positive");
        // Find or create the right sized packed type
        ಸ್ಥಿರಾಂಕ Dfg_ದತ್ತಾಂಶ_Type*& entryr = s_packedTypes[ಅಗಲ];
        ಆದರೆ (!entryr) entryr = new Dfg_ದತ್ತಾಂಶ_Type{ಅಗಲ};
        ಹಿಂತಿರುಗಿ *entryr;
    }

    // Returns an Array type of the given size with the given elements
    ಸ್ಥಿರ ಸ್ಥಿರಾಂಕ Dfg_ದತ್ತಾಂಶ_Type& array(ಸ್ಥಿರಾಂಕ Dfg_ದತ್ತಾಂಶ_Type& elemType, uint32_t ಗಾತ್ರ_2) {
        UASSERT(elemType.isPacked(), "Cannot create multi-dimensional arrays yet");
        // Find or create the right sized array type with this as elements
        ಸ್ಥಿರಾಂಕ Dfg_ದತ್ತಾಂಶ_Type*& entryr = elemType.m_arrayTypes[ಗಾತ್ರ_2];
        ಆದರೆ (!entryr) entryr = new Dfg_ದತ್ತಾಂಶ_Type{ಗಾತ್ರ_2, elemType};
        ಹಿಂತಿರುಗಿ *entryr;
    }

    // Returns the singleton Null type
    ಸ್ಥಿರ ಸ್ಥಿರಾಂಕ Dfg_ದತ್ತಾಂಶ_Type& null() {
        ಆದರೆ (!s_nullTypep) s_nullTypep = new Dfg_ದತ್ತಾಂಶ_Type{};
        ಹಿಂತಿರುಗಿ *s_nullTypep;
    }

    // Returns the data type of selecting the range [lo + size - 1 : lo] from this type
    ಸ್ಥಿರ ಸ್ಥಿರಾಂಕ Dfg_ದತ್ತಾಂಶ_Type& select(ಸ್ಥಿರಾಂಕ Dfg_ದತ್ತಾಂಶ_Type& dtype, uint32_t lo, uint32_t ಗಾತ್ರ_2) {
        ಆಯ್ಕೆ (dtype.m_ಬಗೆ) {
        ಸ್ಥಿತಿ ಬಗೆ_2::Packed: {
            UASSERT(lo + ಗಾತ್ರ_2 - 1 < dtype.ಗಾತ್ರ_2(), "Out of range");
            ಹಿಂತಿರುಗಿ Dfg_ದತ್ತಾಂಶ_Type::packed(ಗಾತ್ರ_2);
        }
        ಸ್ಥಿತಿ ಬಗೆ_2::Array: {
            UASSERT(lo + ಗಾತ್ರ_2 - 1 < dtype.ಗಾತ್ರ_2(), "Out of range");
            ಹಿಂತಿರುಗಿ Dfg_ದತ್ತಾಂಶ_Type::array(dtype.elemDtype(), ಗಾತ್ರ_2);
        }
        ಸ್ಥಿತಿ ಬಗೆ_2::Null: v3fatal("Type cannot be selected from");  // LCOV_EXCL_LINE
        }
        VL_UNREACHABLE;
    }

    // Construct a DfgDataType that represents the given AstNodeDType.
    // Returns nullptr if AstNodeDType is not representable by DfgDataType.
    ಸ್ಥಿರ ಸ್ಥಿರಾಂಕ Dfg_ದತ್ತಾಂಶ_Type* fromAst(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* dtypep) {
        dtypep = dtypep->skipRefp();

        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstUnpackArrayDType* ಸ್ಥಿರಾಂಕ typep = VN_CAST(dtypep, UnpackArrayDType)) {
            // Convert the element type
            ಸ್ಥಿರಾಂಕ Dfg_ದತ್ತಾಂಶ_Type* ಸ್ಥಿರಾಂಕ elemp = fromAst(typep->subDTypep());
            // If element type is not supported, we cannot handle the array either
            ಆದರೆ (!elemp) ಹಿಂತಿರುಗಿ nullptr;
            // Currently restricted to 1-dimensional arrays, just assumptions in the code
            ಆದರೆ (elemp->isArray()) ಹಿಂತಿರುಗಿ nullptr;
            // Unpacked array maps to Array
            ಹಿಂತಿರುಗಿ &array(*elemp, typep->elementsConst());
        }

        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstPackArrayDType* ಸ್ಥಿರಾಂಕ typep = VN_CAST(dtypep, PackArrayDType)) {
            // Packed array maps to Packed
            ಹಿಂತಿರುಗಿ &packed(typep->ಅಗಲ());
        }

        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstStructDType* ಸ್ಥಿರಾಂಕ typep = VN_CAST(dtypep, StructDType)) {
            // Unpacked structs currently not supported
            ಆದರೆ (!typep->packed()) ಹಿಂತಿರುಗಿ nullptr;
            // Packed struct maps to Packed
            ಹಿಂತಿರುಗಿ &packed(typep->ಅಗಲ());
        }

        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstUnionDType* ಸ್ಥಿರಾಂಕ typep = VN_CAST(dtypep, UnionDType)) {
            // Unpacked unions not supported
            ಆದರೆ (!typep->packed()) ಹಿಂತಿರುಗಿ nullptr;
            // Packed union maps to Packed
            ಹಿಂತಿರುಗಿ &packed(typep->ಅಗಲ());
        }

        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstBasicDType* ಸ್ಥಿರಾಂಕ typep = VN_CAST(dtypep, BasicDType)) {
            // Anything not resembling a binary encoded number is not supported
            ಆದರೆ (!typep->keyword().isIntNumeric()) ಹಿಂತಿರುಗಿ nullptr;
            // Basic numeric types map to Packed
            ಹಿಂತಿರುಗಿ &packed(typep->ಅಗಲ());
        }

        // Anything else is not representable
        ಹಿಂತಿರುಗಿ nullptr;
    }

    // Reset interned types
    ಸ್ಥಿರ ಶೂನ್ಯ reset() {
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& pair : s_packedTypes) VL_DO_DANGLING(delete pair.ಎರಡನೇ, pair.ಎರಡನೇ);
        s_packedTypes.clear();
        delete s_nullTypep;
        s_nullTypep = nullptr;
    }
};

#ಆದರೆ_ಅಂತ್ಯ
