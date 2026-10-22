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

#அந்தர்பாவ "config_build.h"
#அந்தர்பாவ "verilatedos.h"

#அந்தர்பாவ "V3Ast.h"
#அந்தர்பாவ "V3Error.h"
#அந்தர்பாவ "V3Global.h"
#அந்தர்பாவ "V3Hash.h"

#அந்தர்பாவ <memory>
#அந்தர்பாவ <unordered_map>

class DfgDataType final {
    எண்ணிக்கை class Kind : uint8_t {
        Packed,
        Array,
        // Tuple - to model unpacked structs in the future
        Null  // No type (like 'void', but that's a C++ keyword)
    };

    // STATE
    மாறிலி Kind m_kind;  // The type category
    uint32_t m_size;  // The number of elements in this type
    Ast_முனை_DType* மாறிலி m_astDtypep;  // Equivalent canonical AstNodeDType
    மாறிலி DfgDataType* மாறிலி m_elemDtypep;  // Type of elements - for arrays only
    // Static singleton Null type
    நிலையான மாறிலி DfgDataType* s_nullTypep;
    // Static map from 'width' -> 'interned Packed DfgDataType with that width'
    நிலையான std::unordered_map<uint32_t, மாறிலி DfgDataType*> s_packedTypes;
    // Map from 'elements' -> 'interned Array DfgDataType with that many elements of *this* type'
    mutable std::unordered_map<uint32_t, மாறிலி DfgDataType*> m_arrayTypes;

    // METHODS
    நிலையான Ast_முனை_DType* canonicalPackedDType(uint32_t width) {
        திரும்பு v3Global.rootp()->typeTablep()->findBitDType(width, width, VSigning::UNSIGNED);
    }
    நிலையான Ast_முனை_DType* canonicalArrayDType(uint32_t size, மாறிலி DfgDataType& elemType) {
        Ast_முனை_DType* மாறிலி elemDTypep = elemType.m_astDtypep;
        File_வரி* மாறிலி flp = elemDTypep->fileline();
        AstRange* மாறிலி rangep = new AstRange{flp, static_cast<முழு>(size - 1), 0};
        Ast_முனை_DType* மாறிலி dtypep = new AstUnpackArrayDType{flp, elemDTypep, rangep};
        v3Global.rootp()->typeTablep()->addTypesp(dtypep);
        திரும்பு dtypep;
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
    DfgDataType(uint32_t size, மாறிலி DfgDataType& elemType)
        : m_kind{Kind::Array}
        , m_size{size}
        , m_astDtypep{canonicalArrayDType(size, elemType)}
        , m_elemDtypep{&elemType} {}

    VL_UNCOPYABLE(DfgDataType);
    VL_UNMOVABLE(DfgDataType);
    ~DfgDataType() {
        க்கு (மாறிலி auto& pair : m_arrayTypes) VL_DO_DANGLING(delete pair.second, pair.second);
    }

பொது:
    //-----------------------------------------------------------------------
    // Properties

    bool isNull() மாறிலி { திரும்பு m_kind == Kind::Null; }
    bool isPacked() மாறிலி { திரும்பு m_kind == Kind::Packed; }
    bool isArray() மாறிலி { திரும்பு m_kind == Kind::Array; }
    // Size of type (this is 'width' for Ppacked, 'elements' for Array, 0 for Null)
    uint32_t size() மாறிலி { திரும்பு m_size; }
    Ast_முனை_DType* astDtypep() மாறிலி { திரும்பு m_astDtypep; }

    // Thanks to the interning, equality is identity
    bool operator==(மாறிலி DfgDataType& that) மாறிலி { திரும்பு this == &that; }
    bool operator!=(மாறிலி DfgDataType& that) மாறிலி { திரும்பு this != &that; }
    // Similarly for hash
    V3Hash hash() மாறிலி { திரும்பு V3Hash{this}; }

    // Type of elements, for arrays only
    மாறிலி DfgDataType& elemDtype() மாறிலி {
        UASSERT(isArray(), "Non-array has no 'elemDType'");
        திரும்பு *m_elemDtypep;
    }

    //-----------------------------------------------------------------------
    // Static factory and management functions

    // Returns a Packed type of the given width
    நிலையான மாறிலி DfgDataType& packed(uint32_t width) {
        UASSERT(width > 0, "Width must be positive");
        // Find or create the right sized packed type
        மாறிலி DfgDataType*& entryr = s_packedTypes[width];
        எனில் (!entryr) entryr = new DfgDataType{width};
        திரும்பு *entryr;
    }

    // Returns an Array type of the given size with the given elements
    நிலையான மாறிலி DfgDataType& array(மாறிலி DfgDataType& elemType, uint32_t size) {
        UASSERT(elemType.isPacked(), "Cannot create multi-dimensional arrays yet");
        // Find or create the right sized array type with this as elements
        மாறிலி DfgDataType*& entryr = elemType.m_arrayTypes[size];
        எனில் (!entryr) entryr = new DfgDataType{size, elemType};
        திரும்பு *entryr;
    }

    // Returns the singleton Null type
    நிலையான மாறிலி DfgDataType& null() {
        எனில் (!s_nullTypep) s_nullTypep = new DfgDataType{};
        திரும்பு *s_nullTypep;
    }

    // Returns the data type of selecting the range [lo + size - 1 : lo] from this type
    நிலையான மாறிலி DfgDataType& select(மாறிலி DfgDataType& dtype, uint32_t lo, uint32_t size) {
        தேர்வு (dtype.m_kind) {
        நிலை Kind::Packed: {
            UASSERT(lo + size - 1 < dtype.size(), "Out of range");
            திரும்பு DfgDataType::packed(size);
        }
        நிலை Kind::Array: {
            UASSERT(lo + size - 1 < dtype.size(), "Out of range");
            திரும்பு DfgDataType::array(dtype.elemDtype(), size);
        }
        நிலை Kind::Null: v3fatal("Type cannot be selected from");  // LCOV_EXCL_LINE
        }
        VL_UNREACHABLE;
    }

    // Construct a DfgDataType that represents the given AstNodeDType.
    // Returns nullptr if AstNodeDType is not representable by DfgDataType.
    நிலையான மாறிலி DfgDataType* fromAst(மாறிலி Ast_முனை_DType* dtypep) {
        dtypep = dtypep->skipRefp();

        எனில் (மாறிலி AstUnpackArrayDType* மாறிலி typep = VN_CAST(dtypep, UnpackArrayDType)) {
            // Convert the element type
            மாறிலி DfgDataType* மாறிலி elemp = fromAst(typep->subDTypep());
            // If element type is not supported, we cannot handle the array either
            எனில் (!elemp) திரும்பு nullptr;
            // Currently restricted to 1-dimensional arrays, just assumptions in the code
            எனில் (elemp->isArray()) திரும்பு nullptr;
            // Unpacked array maps to Array
            திரும்பு &array(*elemp, typep->elementsConst());
        }

        எனில் (மாறிலி AstPackArrayDType* மாறிலி typep = VN_CAST(dtypep, PackArrayDType)) {
            // Packed array maps to Packed
            திரும்பு &packed(typep->width());
        }

        எனில் (மாறிலி AstStructDType* மாறிலி typep = VN_CAST(dtypep, StructDType)) {
            // Unpacked structs currently not supported
            எனில் (!typep->packed()) திரும்பு nullptr;
            // Packed struct maps to Packed
            திரும்பு &packed(typep->width());
        }

        எனில் (மாறிலி AstUnionDType* மாறிலி typep = VN_CAST(dtypep, UnionDType)) {
            // Unpacked unions not supported
            எனில் (!typep->packed()) திரும்பு nullptr;
            // Packed union maps to Packed
            திரும்பு &packed(typep->width());
        }

        எனில் (மாறிலி AstBasicDType* மாறிலி typep = VN_CAST(dtypep, BasicDType)) {
            // Anything not resembling a binary encoded number is not supported
            எனில் (!typep->keyword().isIntNumeric()) திரும்பு nullptr;
            // Basic numeric types map to Packed
            திரும்பு &packed(typep->width());
        }

        // Anything else is not representable
        திரும்பு nullptr;
    }

    // Reset interned types
    நிலையான வெறுமை reset() {
        க்கு (மாறிலி auto& pair : s_packedTypes) VL_DO_DANGLING(delete pair.second, pair.second);
        s_packedTypes.clear();
        delete s_nullTypep;
        s_nullTypep = nullptr;
    }
};

#endif
