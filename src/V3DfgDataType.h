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

#ifndef VERILATOR_V3DFGDATATYPE_H_
#define VERILATOR_V3DFGDATATYPE_H_

#ಅನ್ತರ್ಭಾವ "config_build.h"
#ಅನ್ತರ್ಭಾವ "verilatedos.h"

#ಅನ್ತರ್ಭಾವ "V3Ast.h"
#ಅನ್ತರ್ಭಾವ "V3Error.h"
#ಅನ್ತರ್ಭಾವ "V3Global.h"
#ಅನ್ತರ್ಭಾವ "V3Hash.h"

#ಅನ್ತರ್ಭಾವ <memory>
#ಅನ್ತರ್ಭಾವ <unordered_map>

class DfgDataType final {
    ಗಣನಾ class Kind : uint8_t {
        Packed,
        Array,
        // Tuple - to model unpacked structs in the future
        Null  // No type (like 'void', but that's a C++ keyword)
    };

    // STATE
    ಧ್ರುವ Kind m_kind;  // The type category
    uint32_t m_size;  // The number of elements in this type
    Ast_ಗಂಟು_DType* ಧ್ರುವ m_astDtypep;  // Equivalent canonical AstNodeDType
    ಧ್ರುವ DfgDataType* ಧ್ರುವ m_elemDtypep;  // Type of elements - for arrays only
    // Static singleton Null type
    ಸ್ಥಿರ ಧ್ರುವ DfgDataType* s_nullTypep;
    // Static map from 'width' -> 'interned Packed DfgDataType with that width'
    ಸ್ಥಿರ std::unordered_map<uint32_t, ಧ್ರುವ DfgDataType*> s_packedTypes;
    // Map from 'elements' -> 'interned Array DfgDataType with that many elements of *this* type'
    mutable std::unordered_map<uint32_t, ಧ್ರುವ DfgDataType*> m_arrayTypes;

    // METHODS
    ಸ್ಥಿರ Ast_ಗಂಟು_DType* canonicalPackedDType(uint32_t width) {
        ಪ್ರತ್ಯಾಗಮ v3Global.rootp()->typeTablep()->findBitDType(width, width, VSigning::UNSIGNED);
    }
    ಸ್ಥಿರ Ast_ಗಂಟು_DType* canonicalArrayDType(uint32_t size, ಧ್ರುವ DfgDataType& elemType) {
        Ast_ಗಂಟು_DType* ಧ್ರುವ elemDTypep = elemType.m_astDtypep;
        File_ಸಾಲು* ಧ್ರುವ flp = elemDTypep->fileline();
        AstRange* ಧ್ರುವ rangep = new AstRange{flp, static_cast<ಪೂರ್ಣಾಙ್ಕ>(size - 1), 0};
        Ast_ಗಂಟು_DType* ಧ್ರುವ dtypep = new AstUnpackArrayDType{flp, elemDTypep, rangep};
        v3Global.rootp()->typeTablep()->addTypesp(dtypep);
        ಪ್ರತ್ಯಾಗಮ dtypep;
    }

    // CONSTRUCTOR - use 'fromAst' instead
    DfgDataType()
        : m_kind{Kind::Null}
        , m_size{0}
        , m_astDtypep{v3Global.rootp()->findVoidDType()}
        , m_elemDtypep{nullptr} {}
    explicit DfgDataType(uint32_t size)
        : m_kind{Kind::Packed}
        , m_size{size}
        , m_astDtypep{canonicalPackedDType(size)}
        , m_elemDtypep{nullptr} {}
    DfgDataType(uint32_t size, ಧ್ರುವ DfgDataType& elemType)
        : m_kind{Kind::Array}
        , m_size{size}
        , m_astDtypep{canonicalArrayDType(size, elemType)}
        , m_elemDtypep{&elemType} {}

    VL_UNCOPYABLE(DfgDataType);
    VL_UNMOVABLE(DfgDataType);
    ~DfgDataType() {
        ಪ್ರತಿ (ಧ್ರುವ auto& pair : m_arrayTypes) VL_DO_DANGLING(delete pair.second, pair.second);
    }

ಸಾರ್ವಜನಿಕ:
    //-----------------------------------------------------------------------
    // Properties

    bool isNull() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_kind == Kind::Null; }
    bool isPacked() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_kind == Kind::Packed; }
    bool isArray() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_kind == Kind::Array; }
    // Size of type (this is 'width' for Ppacked, 'elements' for Array, 0 for Null)
    uint32_t size() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_size; }
    Ast_ಗಂಟು_DType* astDtypep() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_astDtypep; }

    // Thanks to the interning, equality is identity
    bool operator==(ಧ್ರುವ DfgDataType& that) ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ this == &that; }
    bool operator!=(ಧ್ರುವ DfgDataType& that) ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ this != &that; }
    // Similarly for hash
    V3Hash hash() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ V3Hash{this}; }

    // Type of elements, for arrays only
    ಧ್ರುವ DfgDataType& elemDtype() ಧ್ರುವ {
        UASSERT(isArray(), "Non-array has no 'elemDType'");
        ಪ್ರತ್ಯಾಗಮ *m_elemDtypep;
    }

    //-----------------------------------------------------------------------
    // Static factory and management functions

    // Returns a Packed type of the given width
    ಸ್ಥಿರ ಧ್ರುವ DfgDataType& packed(uint32_t width) {
        UASSERT(width > 0, "Width must be positive");
        // Find or create the right sized packed type
        ಧ್ರುವ DfgDataType*& entryr = s_packedTypes[width];
        ಯದಿ (!entryr) entryr = new DfgDataType{width};
        ಪ್ರತ್ಯಾಗಮ *entryr;
    }

    // Returns an Array type of the given size with the given elements
    ಸ್ಥಿರ ಧ್ರುವ DfgDataType& array(ಧ್ರುವ DfgDataType& elemType, uint32_t size) {
        UASSERT(elemType.isPacked(), "Cannot create multi-dimensional arrays yet");
        // Find or create the right sized array type with this as elements
        ಧ್ರುವ DfgDataType*& entryr = elemType.m_arrayTypes[size];
        ಯದಿ (!entryr) entryr = new DfgDataType{size, elemType};
        ಪ್ರತ್ಯಾಗಮ *entryr;
    }

    // Returns the singleton Null type
    ಸ್ಥಿರ ಧ್ರುವ DfgDataType& null() {
        ಯದಿ (!s_nullTypep) s_nullTypep = new DfgDataType{};
        ಪ್ರತ್ಯಾಗಮ *s_nullTypep;
    }

    // Returns the data type of selecting the range [lo + size - 1 : lo] from this type
    ಸ್ಥಿರ ಧ್ರುವ DfgDataType& select(ಧ್ರುವ DfgDataType& dtype, uint32_t lo, uint32_t size) {
        ವಿಕಲ್ಪ (dtype.m_kind) {
        ಪಕ್ಷ Kind::Packed: {
            UASSERT(lo + size - 1 < dtype.size(), "Out of range");
            ಪ್ರತ್ಯಾಗಮ DfgDataType::packed(size);
        }
        ಪಕ್ಷ Kind::Array: {
            UASSERT(lo + size - 1 < dtype.size(), "Out of range");
            ಪ್ರತ್ಯಾಗಮ DfgDataType::array(dtype.elemDtype(), size);
        }
        ಪಕ್ಷ Kind::Null: v3fatal("Type cannot be selected from");  // LCOV_EXCL_LINE
        }
        VL_UNREACHABLE;
    }

    // Construct a DfgDataType that represents the given AstNodeDType.
    // Returns nullptr if AstNodeDType is not representable by DfgDataType.
    ಸ್ಥಿರ ಧ್ರುವ DfgDataType* fromAst(ಧ್ರುವ Ast_ಗಂಟು_DType* dtypep) {
        dtypep = dtypep->skipRefp();

        ಯದಿ (ಧ್ರುವ AstUnpackArrayDType* ಧ್ರುವ typep = VN_CAST(dtypep, UnpackArrayDType)) {
            // Convert the element type
            ಧ್ರುವ DfgDataType* ಧ್ರುವ elemp = fromAst(typep->subDTypep());
            // If element type is not supported, we cannot handle the array either
            ಯದಿ (!elemp) ಪ್ರತ್ಯಾಗಮ nullptr;
            // Currently restricted to 1-dimensional arrays, just assumptions in the code
            ಯದಿ (elemp->isArray()) ಪ್ರತ್ಯಾಗಮ nullptr;
            // Unpacked array maps to Array
            ಪ್ರತ್ಯಾಗಮ &array(*elemp, typep->elementsConst());
        }

        ಯದಿ (ಧ್ರುವ AstPackArrayDType* ಧ್ರುವ typep = VN_CAST(dtypep, PackArrayDType)) {
            // Packed array maps to Packed
            ಪ್ರತ್ಯಾಗಮ &packed(typep->width());
        }

        ಯದಿ (ಧ್ರುವ AstStructDType* ಧ್ರುವ typep = VN_CAST(dtypep, StructDType)) {
            // Unpacked structs currently not supported
            ಯದಿ (!typep->packed()) ಪ್ರತ್ಯಾಗಮ nullptr;
            // Packed struct maps to Packed
            ಪ್ರತ್ಯಾಗಮ &packed(typep->width());
        }

        ಯದಿ (ಧ್ರುವ AstUnionDType* ಧ್ರುವ typep = VN_CAST(dtypep, UnionDType)) {
            // Unpacked unions not supported
            ಯದಿ (!typep->packed()) ಪ್ರತ್ಯಾಗಮ nullptr;
            // Packed union maps to Packed
            ಪ್ರತ್ಯಾಗಮ &packed(typep->width());
        }

        ಯದಿ (ಧ್ರುವ AstBasicDType* ಧ್ರುವ typep = VN_CAST(dtypep, BasicDType)) {
            // Anything not resembling a binary encoded number is not supported
            ಯದಿ (!typep->keyword().isIntNumeric()) ಪ್ರತ್ಯಾಗಮ nullptr;
            // Basic numeric types map to Packed
            ಪ್ರತ್ಯಾಗಮ &packed(typep->width());
        }

        // Anything else is not representable
        ಪ್ರತ್ಯಾಗಮ nullptr;
    }

    // Reset interned types
    ಸ್ಥಿರ ಶೂನ್ಯ reset() {
        ಪ್ರತಿ (ಧ್ರುವ auto& pair : s_packedTypes) VL_DO_DANGLING(delete pair.second, pair.second);
        s_packedTypes.clear();
        delete s_nullTypep;
        s_nullTypep = nullptr;
    }
};

#endif
