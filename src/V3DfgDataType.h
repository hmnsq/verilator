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

#অন্তর্ভাৱ "config_build.h"
#অন্তর্ভাৱ "verilatedos.h"

#অন্তর্ভাৱ "V3Ast.h"
#অন্তর্ভাৱ "V3Error.h"
#অন্তর্ভাৱ "V3Global.h"
#অন্তর্ভাৱ "V3Hash.h"

#অন্তর্ভাৱ <memory>
#অন্তর্ভাৱ <unordered_map>

class DfgDataType final {
    গণনা class Kind : uint8_t {
        Packed,
        Array,
        // Tuple - to model unpacked structs in the future
        Null  // No type (like 'void', but that's a C++ keyword)
    };

    // STATE
    ধ্রুৱ Kind m_kind;  // The type category
    uint32_t m_size;  // The number of elements in this type
    Ast_গ্ৰন্থি_DType* ধ্রুৱ m_astDtypep;  // Equivalent canonical AstNodeDType
    ধ্রুৱ DfgDataType* ধ্রুৱ m_elemDtypep;  // Type of elements - for arrays only
    // Static singleton Null type
    স্থির ধ্রুৱ DfgDataType* s_nullTypep;
    // Static map from 'width' -> 'interned Packed DfgDataType with that width'
    স্থির std::unordered_map<uint32_t, ধ্রুৱ DfgDataType*> s_packedTypes;
    // Map from 'elements' -> 'interned Array DfgDataType with that many elements of *this* type'
    mutable std::unordered_map<uint32_t, ধ্রুৱ DfgDataType*> m_arrayTypes;

    // METHODS
    স্থির Ast_গ্ৰন্থি_DType* canonicalPackedDType(uint32_t width) {
        ঘূৰাই v3Global.rootp()->typeTablep()->findBitDType(width, width, VSigning::UNSIGNED);
    }
    স্থির Ast_গ্ৰন্থি_DType* canonicalArrayDType(uint32_t size, ধ্রুৱ DfgDataType& elemType) {
        Ast_গ্ৰন্থি_DType* ধ্রুৱ elemDTypep = elemType.m_astDtypep;
        File_সাৰি* ধ্রুৱ flp = elemDTypep->fileline();
        AstRange* ধ্রুৱ rangep = new AstRange{flp, static_cast<পূর্ণাঙ্ক>(size - 1), 0};
        Ast_গ্ৰন্থি_DType* ধ্রুৱ dtypep = new AstUnpackArrayDType{flp, elemDTypep, rangep};
        v3Global.rootp()->typeTablep()->addTypesp(dtypep);
        ঘূৰাই dtypep;
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
    DfgDataType(uint32_t size, ধ্রুৱ DfgDataType& elemType)
        : m_kind{Kind::Array}
        , m_size{size}
        , m_astDtypep{canonicalArrayDType(size, elemType)}
        , m_elemDtypep{&elemType} {}

    VL_UNCOPYABLE(DfgDataType);
    VL_UNMOVABLE(DfgDataType);
    ~DfgDataType() {
        বাবে (ধ্রুৱ auto& pair : m_arrayTypes) VL_DO_DANGLING(delete pair.second, pair.second);
    }

সার্ৱজনিক:
    //-----------------------------------------------------------------------
    // Properties

    bool isNull() ধ্রুৱ { ঘূৰাই m_kind == Kind::Null; }
    bool isPacked() ধ্রুৱ { ঘূৰাই m_kind == Kind::Packed; }
    bool isArray() ধ্রুৱ { ঘূৰাই m_kind == Kind::Array; }
    // Size of type (this is 'width' for Ppacked, 'elements' for Array, 0 for Null)
    uint32_t size() ধ্রুৱ { ঘূৰাই m_size; }
    Ast_গ্ৰন্থি_DType* astDtypep() ধ্রুৱ { ঘূৰাই m_astDtypep; }

    // Thanks to the interning, equality is identity
    bool operator==(ধ্রুৱ DfgDataType& that) ধ্রুৱ { ঘূৰাই this == &that; }
    bool operator!=(ধ্রুৱ DfgDataType& that) ধ্রুৱ { ঘূৰাই this != &that; }
    // Similarly for hash
    V3Hash hash() ধ্রুৱ { ঘূৰাই V3Hash{this}; }

    // Type of elements, for arrays only
    ধ্রুৱ DfgDataType& elemDtype() ধ্রুৱ {
        UASSERT(isArray(), "Non-array has no 'elemDType'");
        ঘূৰাই *m_elemDtypep;
    }

    //-----------------------------------------------------------------------
    // Static factory and management functions

    // Returns a Packed type of the given width
    স্থির ধ্রুৱ DfgDataType& packed(uint32_t width) {
        UASSERT(width > 0, "Width must be positive");
        // Find or create the right sized packed type
        ধ্রুৱ DfgDataType*& entryr = s_packedTypes[width];
        যদি (!entryr) entryr = new DfgDataType{width};
        ঘূৰাই *entryr;
    }

    // Returns an Array type of the given size with the given elements
    স্থির ধ্রুৱ DfgDataType& array(ধ্রুৱ DfgDataType& elemType, uint32_t size) {
        UASSERT(elemType.isPacked(), "Cannot create multi-dimensional arrays yet");
        // Find or create the right sized array type with this as elements
        ধ্রুৱ DfgDataType*& entryr = elemType.m_arrayTypes[size];
        যদি (!entryr) entryr = new DfgDataType{size, elemType};
        ঘূৰাই *entryr;
    }

    // Returns the singleton Null type
    স্থির ধ্রুৱ DfgDataType& null() {
        যদি (!s_nullTypep) s_nullTypep = new DfgDataType{};
        ঘূৰাই *s_nullTypep;
    }

    // Returns the data type of selecting the range [lo + size - 1 : lo] from this type
    স্থির ধ্রুৱ DfgDataType& select(ধ্রুৱ DfgDataType& dtype, uint32_t lo, uint32_t size) {
        ৱিকল্প (dtype.m_kind) {
        পক্ষ Kind::Packed: {
            UASSERT(lo + size - 1 < dtype.size(), "Out of range");
            ঘূৰাই DfgDataType::packed(size);
        }
        পক্ষ Kind::Array: {
            UASSERT(lo + size - 1 < dtype.size(), "Out of range");
            ঘূৰাই DfgDataType::array(dtype.elemDtype(), size);
        }
        পক্ষ Kind::Null: v3fatal("Type cannot be selected from");  // LCOV_EXCL_LINE
        }
        VL_UNREACHABLE;
    }

    // Construct a DfgDataType that represents the given AstNodeDType.
    // Returns nullptr if AstNodeDType is not representable by DfgDataType.
    স্থির ধ্রুৱ DfgDataType* fromAst(ধ্রুৱ Ast_গ্ৰন্থি_DType* dtypep) {
        dtypep = dtypep->skipRefp();

        যদি (ধ্রুৱ AstUnpackArrayDType* ধ্রুৱ typep = VN_CAST(dtypep, UnpackArrayDType)) {
            // Convert the element type
            ধ্রুৱ DfgDataType* ধ্রুৱ elemp = fromAst(typep->subDTypep());
            // If element type is not supported, we cannot handle the array either
            যদি (!elemp) ঘূৰাই nullptr;
            // Currently restricted to 1-dimensional arrays, just assumptions in the code
            যদি (elemp->isArray()) ঘূৰাই nullptr;
            // Unpacked array maps to Array
            ঘূৰাই &array(*elemp, typep->elementsConst());
        }

        যদি (ধ্রুৱ AstPackArrayDType* ধ্রুৱ typep = VN_CAST(dtypep, PackArrayDType)) {
            // Packed array maps to Packed
            ঘূৰাই &packed(typep->width());
        }

        যদি (ধ্রুৱ AstStructDType* ধ্রুৱ typep = VN_CAST(dtypep, StructDType)) {
            // Unpacked structs currently not supported
            যদি (!typep->packed()) ঘূৰাই nullptr;
            // Packed struct maps to Packed
            ঘূৰাই &packed(typep->width());
        }

        যদি (ধ্রুৱ AstUnionDType* ধ্রুৱ typep = VN_CAST(dtypep, UnionDType)) {
            // Unpacked unions not supported
            যদি (!typep->packed()) ঘূৰাই nullptr;
            // Packed union maps to Packed
            ঘূৰাই &packed(typep->width());
        }

        যদি (ধ্রুৱ AstBasicDType* ধ্রুৱ typep = VN_CAST(dtypep, BasicDType)) {
            // Anything not resembling a binary encoded number is not supported
            যদি (!typep->keyword().isIntNumeric()) ঘূৰাই nullptr;
            // Basic numeric types map to Packed
            ঘূৰাই &packed(typep->width());
        }

        // Anything else is not representable
        ঘূৰাই nullptr;
    }

    // Reset interned types
    স্থির শূন্য reset() {
        বাবে (ধ্রুৱ auto& pair : s_packedTypes) VL_DO_DANGLING(delete pair.second, pair.second);
        s_packedTypes.clear();
        delete s_nullTypep;
        s_nullTypep = nullptr;
    }
};

#endif
