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

#ଅନ୍ତର୍ଭାଵ "config_build.h"
#ଅନ୍ତର୍ଭାଵ "verilatedos.h"

#ଅନ୍ତର୍ଭାଵ "V3Ast.h"
#ଅନ୍ତର୍ଭାଵ "V3Error.h"
#ଅନ୍ତର୍ଭାଵ "V3Global.h"
#ଅନ୍ତର୍ଭାଵ "V3Hash.h"

#ଅନ୍ତର୍ଭାଵ <memory>
#ଅନ୍ତର୍ଭାଵ <unordered_map>

class DfgDataType final {
    ଗଣନା class Kind : uint8_t {
        Packed,
        Array,
        // Tuple - to model unpacked structs in the future
        Null  // No type (like 'void', but that's a C++ keyword)
    };

    // STATE
    ଅଚଳ Kind m_kind;  // The type category
    uint32_t m_size;  // The number of elements in this type
    Ast_ଗଣ୍ଠି_DType* ଅଚଳ m_astDtypep;  // Equivalent canonical AstNodeDType
    ଅଚଳ DfgDataType* ଅଚଳ m_elemDtypep;  // Type of elements - for arrays only
    // Static singleton Null type
    ସ୍ଥିର ଅଚଳ DfgDataType* s_nullTypep;
    // Static map from 'width' -> 'interned Packed DfgDataType with that width'
    ସ୍ଥିର std::unordered_map<uint32_t, ଅଚଳ DfgDataType*> s_packedTypes;
    // Map from 'elements' -> 'interned Array DfgDataType with that many elements of *this* type'
    mutable std::unordered_map<uint32_t, ଅଚଳ DfgDataType*> m_arrayTypes;

    // METHODS
    ସ୍ଥିର Ast_ଗଣ୍ଠି_DType* canonicalPackedDType(uint32_t width) {
        ଫେରନ୍ତୁ v3Global.rootp()->typeTablep()->findBitDType(width, width, VSigning::UNSIGNED);
    }
    ସ୍ଥିର Ast_ଗଣ୍ଠି_DType* canonicalArrayDType(uint32_t size, ଅଚଳ DfgDataType& elemType) {
        Ast_ଗଣ୍ଠି_DType* ଅଚଳ elemDTypep = elemType.m_astDtypep;
        File_ଧାଡ଼ି* ଅଚଳ flp = elemDTypep->fileline();
        AstRange* ଅଚଳ rangep = new AstRange{flp, static_cast<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>(size - 1), 0};
        Ast_ଗଣ୍ଠି_DType* ଅଚଳ dtypep = new AstUnpackArrayDType{flp, elemDTypep, rangep};
        v3Global.rootp()->typeTablep()->addTypesp(dtypep);
        ଫେରନ୍ତୁ dtypep;
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
    DfgDataType(uint32_t size, ଅଚଳ DfgDataType& elemType)
        : m_kind{Kind::Array}
        , m_size{size}
        , m_astDtypep{canonicalArrayDType(size, elemType)}
        , m_elemDtypep{&elemType} {}

    VL_UNCOPYABLE(DfgDataType);
    VL_UNMOVABLE(DfgDataType);
    ~DfgDataType() {
        ପାଇଁ (ଅଚଳ auto& pair : m_arrayTypes) VL_DO_DANGLING(delete pair.second, pair.second);
    }

ସର୍ବସାଧାରଣ:
    //-----------------------------------------------------------------------
    // Properties

    bool isNull() ଅଚଳ { ଫେରନ୍ତୁ m_kind == Kind::Null; }
    bool isPacked() ଅଚଳ { ଫେରନ୍ତୁ m_kind == Kind::Packed; }
    bool isArray() ଅଚଳ { ଫେରନ୍ତୁ m_kind == Kind::Array; }
    // Size of type (this is 'width' for Ppacked, 'elements' for Array, 0 for Null)
    uint32_t size() ଅଚଳ { ଫେରନ୍ତୁ m_size; }
    Ast_ଗଣ୍ଠି_DType* astDtypep() ଅଚଳ { ଫେରନ୍ତୁ m_astDtypep; }

    // Thanks to the interning, equality is identity
    bool operator==(ଅଚଳ DfgDataType& that) ଅଚଳ { ଫେରନ୍ତୁ this == &that; }
    bool operator!=(ଅଚଳ DfgDataType& that) ଅଚଳ { ଫେରନ୍ତୁ this != &that; }
    // Similarly for hash
    V3Hash hash() ଅଚଳ { ଫେରନ୍ତୁ V3Hash{this}; }

    // Type of elements, for arrays only
    ଅଚଳ DfgDataType& elemDtype() ଅଚଳ {
        UASSERT(isArray(), "Non-array has no 'elemDType'");
        ଫେରନ୍ତୁ *m_elemDtypep;
    }

    //-----------------------------------------------------------------------
    // Static factory and management functions

    // Returns a Packed type of the given width
    ସ୍ଥିର ଅଚଳ DfgDataType& packed(uint32_t width) {
        UASSERT(width > 0, "Width must be positive");
        // Find or create the right sized packed type
        ଅଚଳ DfgDataType*& entryr = s_packedTypes[width];
        ଯଦି (!entryr) entryr = new DfgDataType{width};
        ଫେରନ୍ତୁ *entryr;
    }

    // Returns an Array type of the given size with the given elements
    ସ୍ଥିର ଅଚଳ DfgDataType& array(ଅଚଳ DfgDataType& elemType, uint32_t size) {
        UASSERT(elemType.isPacked(), "Cannot create multi-dimensional arrays yet");
        // Find or create the right sized array type with this as elements
        ଅଚଳ DfgDataType*& entryr = elemType.m_arrayTypes[size];
        ଯଦି (!entryr) entryr = new DfgDataType{size, elemType};
        ଫେରନ୍ତୁ *entryr;
    }

    // Returns the singleton Null type
    ସ୍ଥିର ଅଚଳ DfgDataType& null() {
        ଯଦି (!s_nullTypep) s_nullTypep = new DfgDataType{};
        ଫେରନ୍ତୁ *s_nullTypep;
    }

    // Returns the data type of selecting the range [lo + size - 1 : lo] from this type
    ସ୍ଥିର ଅଚଳ DfgDataType& select(ଅଚଳ DfgDataType& dtype, uint32_t lo, uint32_t size) {
        ବାଛ (dtype.m_kind) {
        ସ୍ଥିତି Kind::Packed: {
            UASSERT(lo + size - 1 < dtype.size(), "Out of range");
            ଫେରନ୍ତୁ DfgDataType::packed(size);
        }
        ସ୍ଥିତି Kind::Array: {
            UASSERT(lo + size - 1 < dtype.size(), "Out of range");
            ଫେରନ୍ତୁ DfgDataType::array(dtype.elemDtype(), size);
        }
        ସ୍ଥିତି Kind::Null: v3fatal("Type cannot be selected from");  // LCOV_EXCL_LINE
        }
        VL_UNREACHABLE;
    }

    // Construct a DfgDataType that represents the given AstNodeDType.
    // Returns nullptr if AstNodeDType is not representable by DfgDataType.
    ସ୍ଥିର ଅଚଳ DfgDataType* fromAst(ଅଚଳ Ast_ଗଣ୍ଠି_DType* dtypep) {
        dtypep = dtypep->skipRefp();

        ଯଦି (ଅଚଳ AstUnpackArrayDType* ଅଚଳ typep = VN_CAST(dtypep, UnpackArrayDType)) {
            // Convert the element type
            ଅଚଳ DfgDataType* ଅଚଳ elemp = fromAst(typep->subDTypep());
            // If element type is not supported, we cannot handle the array either
            ଯଦି (!elemp) ଫେରନ୍ତୁ nullptr;
            // Currently restricted to 1-dimensional arrays, just assumptions in the code
            ଯଦି (elemp->isArray()) ଫେରନ୍ତୁ nullptr;
            // Unpacked array maps to Array
            ଫେରନ୍ତୁ &array(*elemp, typep->elementsConst());
        }

        ଯଦି (ଅଚଳ AstPackArrayDType* ଅଚଳ typep = VN_CAST(dtypep, PackArrayDType)) {
            // Packed array maps to Packed
            ଫେରନ୍ତୁ &packed(typep->width());
        }

        ଯଦି (ଅଚଳ AstStructDType* ଅଚଳ typep = VN_CAST(dtypep, StructDType)) {
            // Unpacked structs currently not supported
            ଯଦି (!typep->packed()) ଫେରନ୍ତୁ nullptr;
            // Packed struct maps to Packed
            ଫେରନ୍ତୁ &packed(typep->width());
        }

        ଯଦି (ଅଚଳ AstUnionDType* ଅଚଳ typep = VN_CAST(dtypep, UnionDType)) {
            // Unpacked unions not supported
            ଯଦି (!typep->packed()) ଫେରନ୍ତୁ nullptr;
            // Packed union maps to Packed
            ଫେରନ୍ତୁ &packed(typep->width());
        }

        ଯଦି (ଅଚଳ AstBasicDType* ଅଚଳ typep = VN_CAST(dtypep, BasicDType)) {
            // Anything not resembling a binary encoded number is not supported
            ଯଦି (!typep->keyword().isIntNumeric()) ଫେରନ୍ତୁ nullptr;
            // Basic numeric types map to Packed
            ଫେରନ୍ତୁ &packed(typep->width());
        }

        // Anything else is not representable
        ଫେରନ୍ତୁ nullptr;
    }

    // Reset interned types
    ସ୍ଥିର ଶୂନ୍ୟ reset() {
        ପାଇଁ (ଅଚଳ auto& pair : s_packedTypes) VL_DO_DANGLING(delete pair.second, pair.second);
        s_packedTypes.clear();
        delete s_nullTypep;
        s_nullTypep = nullptr;
    }
};

#endif
