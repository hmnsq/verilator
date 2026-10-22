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

#অন্তর্ভাব "config_build.h"
#অন্তর্ভাব "verilatedos.h"

#অন্তর্ভাব "V3Ast.h"
#অন্তর্ভাব "V3Error.h"
#অন্তর্ভাব "V3Global.h"
#অন্তর্ভাব "V3Hash.h"

#অন্তর্ভাব <memory>
#অন্তর্ভাব <unordered_map>

class Dfg_তথ্য_Type final {
    গণনা class Kind : uint8_t {
        Packed,
        Array,
        // Tuple - to model unpacked structs in the future
        Null  // No type (like 'void', but that's a C++ keyword)
    };

    // STATE
    ধ্রুবক Kind m_kind;  // The type category
    uint32_t m_আকার;  // The number of elements in this type
    Ast_গ্রন্থি_DType* ধ্রুবক m_astDtypep;  // Equivalent canonical AstNodeDType
    ধ্রুবক Dfg_তথ্য_Type* ধ্রুবক m_elemDtypep;  // Type of elements - for arrays only
    // Static singleton Null type
    স্থির ধ্রুবক Dfg_তথ্য_Type* s_nullTypep;
    // Static map from 'width' -> 'interned Packed DfgDataType with that width'
    স্থির std::unordered_map<uint32_t, ধ্রুবক Dfg_তথ্য_Type*> s_packedTypes;
    // Map from 'elements' -> 'interned Array DfgDataType with that many elements of *this* type'
    mutable std::unordered_map<uint32_t, ধ্রুবক Dfg_তথ্য_Type*> m_arrayTypes;

    // METHODS
    স্থির Ast_গ্রন্থি_DType* canonicalPackedDType(uint32_t width) {
        ফেরত v3Global.rootp()->typeTablep()->findBitDType(width, width, VSigning::UNSIGNED);
    }
    স্থির Ast_গ্রন্থি_DType* canonicalArrayDType(uint32_t আকার_2, ধ্রুবক Dfg_তথ্য_Type& elemType) {
        Ast_গ্রন্থি_DType* ধ্রুবক elemDTypep = elemType.m_astDtypep;
        File_সারি* ধ্রুবক flp = elemDTypep->fileline();
        AstRange* ধ্রুবক rangep = new AstRange{flp, static_cast<পূর্ণসংখ্যা>(আকার_2 - 1), 0};
        Ast_গ্রন্থি_DType* ধ্রুবক dtypep = new AstUnpackArrayDType{flp, elemDTypep, rangep};
        v3Global.rootp()->typeTablep()->addTypesp(dtypep);
        ফেরত dtypep;
    }

    // CONSTRUCTOR - use 'fromAst' instead
    Dfg_তথ্য_Type()
        : m_kind{Kind::Null}
        , m_আকার{0}
        , m_astDtypep{v3Global.rootp()->findVoidDType()}
        , m_elemDtypep{nullptr} {}
    explicit Dfg_তথ্য_Type(uint32_t আকার_2)
        : m_kind{Kind::Packed}
        , m_আকার{আকার_2}
        , m_astDtypep{canonicalPackedDType(আকার_2)}
        , m_elemDtypep{nullptr} {}
    Dfg_তথ্য_Type(uint32_t আকার_2, ধ্রুবক Dfg_তথ্য_Type& elemType)
        : m_kind{Kind::Array}
        , m_আকার{আকার_2}
        , m_astDtypep{canonicalArrayDType(আকার_2, elemType)}
        , m_elemDtypep{&elemType} {}

    VL_UNCOPYABLE(Dfg_তথ্য_Type);
    VL_UNMOVABLE(Dfg_তথ্য_Type);
    ~Dfg_তথ্য_Type() {
        জন্য (ধ্রুবক auto& pair : m_arrayTypes) VL_DO_DANGLING(delete pair.second, pair.second);
    }

সর্বজনীন:
    //-----------------------------------------------------------------------
    // Properties

    bool isNull() ধ্রুবক { ফেরত m_kind == Kind::Null; }
    bool isPacked() ধ্রুবক { ফেরত m_kind == Kind::Packed; }
    bool isArray() ধ্রুবক { ফেরত m_kind == Kind::Array; }
    // Size of type (this is 'width' for Ppacked, 'elements' for Array, 0 for Null)
    uint32_t আকার_2() ধ্রুবক { ফেরত m_আকার; }
    Ast_গ্রন্থি_DType* astDtypep() ধ্রুবক { ফেরত m_astDtypep; }

    // Thanks to the interning, equality is identity
    bool operator==(ধ্রুবক Dfg_তথ্য_Type& that) ধ্রুবক { ফেরত this == &that; }
    bool operator!=(ধ্রুবক Dfg_তথ্য_Type& that) ধ্রুবক { ফেরত this != &that; }
    // Similarly for hash
    V3Hash hash() ধ্রুবক { ফেরত V3Hash{this}; }

    // Type of elements, for arrays only
    ধ্রুবক Dfg_তথ্য_Type& elemDtype() ধ্রুবক {
        UASSERT(isArray(), "Non-array has no 'elemDType'");
        ফেরত *m_elemDtypep;
    }

    //-----------------------------------------------------------------------
    // Static factory and management functions

    // Returns a Packed type of the given width
    স্থির ধ্রুবক Dfg_তথ্য_Type& packed(uint32_t width) {
        UASSERT(width > 0, "Width must be positive");
        // Find or create the right sized packed type
        ধ্রুবক Dfg_তথ্য_Type*& entryr = s_packedTypes[width];
        যদি (!entryr) entryr = new Dfg_তথ্য_Type{width};
        ফেরত *entryr;
    }

    // Returns an Array type of the given size with the given elements
    স্থির ধ্রুবক Dfg_তথ্য_Type& array(ধ্রুবক Dfg_তথ্য_Type& elemType, uint32_t আকার_2) {
        UASSERT(elemType.isPacked(), "Cannot create multi-dimensional arrays yet");
        // Find or create the right sized array type with this as elements
        ধ্রুবক Dfg_তথ্য_Type*& entryr = elemType.m_arrayTypes[আকার_2];
        যদি (!entryr) entryr = new Dfg_তথ্য_Type{আকার_2, elemType};
        ফেরত *entryr;
    }

    // Returns the singleton Null type
    স্থির ধ্রুবক Dfg_তথ্য_Type& null() {
        যদি (!s_nullTypep) s_nullTypep = new Dfg_তথ্য_Type{};
        ফেরত *s_nullTypep;
    }

    // Returns the data type of selecting the range [lo + size - 1 : lo] from this type
    স্থির ধ্রুবক Dfg_তথ্য_Type& select(ধ্রুবক Dfg_তথ্য_Type& dtype, uint32_t lo, uint32_t আকার_2) {
        নির্বাচন (dtype.m_kind) {
        ক্ষেত্র Kind::Packed: {
            UASSERT(lo + আকার_2 - 1 < dtype.আকার_2(), "Out of range");
            ফেরত Dfg_তথ্য_Type::packed(আকার_2);
        }
        ক্ষেত্র Kind::Array: {
            UASSERT(lo + আকার_2 - 1 < dtype.আকার_2(), "Out of range");
            ফেরত Dfg_তথ্য_Type::array(dtype.elemDtype(), আকার_2);
        }
        ক্ষেত্র Kind::Null: v3fatal("Type cannot be selected from");  // LCOV_EXCL_LINE
        }
        VL_UNREACHABLE;
    }

    // Construct a DfgDataType that represents the given AstNodeDType.
    // Returns nullptr if AstNodeDType is not representable by DfgDataType.
    স্থির ধ্রুবক Dfg_তথ্য_Type* fromAst(ধ্রুবক Ast_গ্রন্থি_DType* dtypep) {
        dtypep = dtypep->skipRefp();

        যদি (ধ্রুবক AstUnpackArrayDType* ধ্রুবক typep = VN_CAST(dtypep, UnpackArrayDType)) {
            // Convert the element type
            ধ্রুবক Dfg_তথ্য_Type* ধ্রুবক elemp = fromAst(typep->subDTypep());
            // If element type is not supported, we cannot handle the array either
            যদি (!elemp) ফেরত nullptr;
            // Currently restricted to 1-dimensional arrays, just assumptions in the code
            যদি (elemp->isArray()) ফেরত nullptr;
            // Unpacked array maps to Array
            ফেরত &array(*elemp, typep->elementsConst());
        }

        যদি (ধ্রুবক AstPackArrayDType* ধ্রুবক typep = VN_CAST(dtypep, PackArrayDType)) {
            // Packed array maps to Packed
            ফেরত &packed(typep->width());
        }

        যদি (ধ্রুবক AstStructDType* ধ্রুবক typep = VN_CAST(dtypep, StructDType)) {
            // Unpacked structs currently not supported
            যদি (!typep->packed()) ফেরত nullptr;
            // Packed struct maps to Packed
            ফেরত &packed(typep->width());
        }

        যদি (ধ্রুবক AstUnionDType* ধ্রুবক typep = VN_CAST(dtypep, UnionDType)) {
            // Unpacked unions not supported
            যদি (!typep->packed()) ফেরত nullptr;
            // Packed union maps to Packed
            ফেরত &packed(typep->width());
        }

        যদি (ধ্রুবক AstBasicDType* ধ্রুবক typep = VN_CAST(dtypep, BasicDType)) {
            // Anything not resembling a binary encoded number is not supported
            যদি (!typep->keyword().isIntNumeric()) ফেরত nullptr;
            // Basic numeric types map to Packed
            ফেরত &packed(typep->width());
        }

        // Anything else is not representable
        ফেরত nullptr;
    }

    // Reset interned types
    স্থির শূন্য reset() {
        জন্য (ধ্রুবক auto& pair : s_packedTypes) VL_DO_DANGLING(delete pair.second, pair.second);
        s_packedTypes.clear();
        delete s_nullTypep;
        s_nullTypep = nullptr;
    }
};

#endif
