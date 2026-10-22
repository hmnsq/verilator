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

#include "config_build.h"
#include "verilatedos.h"

#include "V3Ast.h"
#include "V3Error.h"
#include "V3Global.h"
#include "V3Hash.h"

#include <memory>
#include <unordered_map>

class DfgDataType final {
    enum class Kind : uint8_t {
        Packed,
        Array,
        // Tuple - to model unpacked structs in the future
        Null  // No type (like 'void', but that's a C++ keyword)
    };

    // STATE
    const Kind m_kind;  // The type category
    uint32_t m_size;  // The number of elements in this type
    AstNodeDType* const m_astDtypep;  // Equivalent canonical AstNodeDType
    const DfgDataType* const m_elemDtypep;  // Type of elements - for arrays only
    // Static singleton Null type
    static const DfgDataType* s_nullTypep;
    // Static map from 'width' -> 'interned Packed DfgDataType with that width'
    static std::unordered_map<uint32_t, const DfgDataType*> s_packedTypes;
    // Map from 'elements' -> 'interned Array DfgDataType with that many elements of *this* type'
    mutable std::unordered_map<uint32_t, const DfgDataType*> m_arrayTypes;

    // METHODS
    static AstNodeDType* canonicalPackedDType(uint32_t width) {
        반환 v3Global.rootp()->typeTablep()->findBitDType(width, width, VSigning::UNSIGNED);
    }
    static AstNodeDType* canonicalArrayDType(uint32_t size, const DfgDataType& elemType) {
        AstNodeDType* const elemDTypep = elemType.m_astDtypep;
        FileLine* const flp = elemDTypep->fileline();
        AstRange* const rangep = new AstRange{flp, static_cast<정수>(size - 1), 0};
        AstNodeDType* const dtypep = new AstUnpackArrayDType{flp, elemDTypep, rangep};
        v3Global.rootp()->typeTablep()->addTypesp(dtypep);
        반환 dtypep;
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
    DfgDataType(uint32_t size, const DfgDataType& elemType)
        : m_kind{Kind::Array}
        , m_size{size}
        , m_astDtypep{canonicalArrayDType(size, elemType)}
        , m_elemDtypep{&elemType} {}

    VL_UNCOPYABLE(DfgDataType);
    VL_UNMOVABLE(DfgDataType);
    ~DfgDataType() {
        위해 (const auto& pair : m_arrayTypes) VL_DO_DANGLING(delete pair.second, pair.second);
    }

public:
    //-----------------------------------------------------------------------
    // Properties

    bool isNull() const { 반환 m_kind == Kind::Null; }
    bool isPacked() const { 반환 m_kind == Kind::Packed; }
    bool isArray() const { 반환 m_kind == Kind::Array; }
    // Size of type (this is 'width' for Ppacked, 'elements' for Array, 0 for Null)
    uint32_t size() const { 반환 m_size; }
    AstNodeDType* astDtypep() const { 반환 m_astDtypep; }

    // Thanks to the interning, equality is identity
    bool operator==(const DfgDataType& that) const { 반환 this == &that; }
    bool operator!=(const DfgDataType& that) const { 반환 this != &that; }
    // Similarly for hash
    V3Hash hash() const { 반환 V3Hash{this}; }

    // Type of elements, for arrays only
    const DfgDataType& elemDtype() const {
        UASSERT(isArray(), "Non-array has no 'elemDType'");
        반환 *m_elemDtypep;
    }

    //-----------------------------------------------------------------------
    // Static factory and management functions

    // Returns a Packed type of the given width
    static const DfgDataType& packed(uint32_t width) {
        UASSERT(width > 0, "Width must be positive");
        // Find or create the right sized packed type
        const DfgDataType*& entryr = s_packedTypes[width];
        만약 (!entryr) entryr = new DfgDataType{width};
        반환 *entryr;
    }

    // Returns an Array type of the given size with the given elements
    static const DfgDataType& array(const DfgDataType& elemType, uint32_t size) {
        UASSERT(elemType.isPacked(), "Cannot create multi-dimensional arrays yet");
        // Find or create the right sized array type with this as elements
        const DfgDataType*& entryr = elemType.m_arrayTypes[size];
        만약 (!entryr) entryr = new DfgDataType{size, elemType};
        반환 *entryr;
    }

    // Returns the singleton Null type
    static const DfgDataType& null() {
        만약 (!s_nullTypep) s_nullTypep = new DfgDataType{};
        반환 *s_nullTypep;
    }

    // Returns the data type of selecting the range [lo + size - 1 : lo] from this type
    static const DfgDataType& select(const DfgDataType& dtype, uint32_t lo, uint32_t size) {
        switch (dtype.m_kind) {
        case Kind::Packed: {
            UASSERT(lo + size - 1 < dtype.size(), "Out of range");
            반환 DfgDataType::packed(size);
        }
        case Kind::Array: {
            UASSERT(lo + size - 1 < dtype.size(), "Out of range");
            반환 DfgDataType::array(dtype.elemDtype(), size);
        }
        case Kind::Null: v3fatal("Type cannot be selected from");  // LCOV_EXCL_LINE
        }
        VL_UNREACHABLE;
    }

    // Construct a DfgDataType that represents the given AstNodeDType.
    // Returns nullptr if AstNodeDType is not representable by DfgDataType.
    static const DfgDataType* fromAst(const AstNodeDType* dtypep) {
        dtypep = dtypep->skipRefp();

        만약 (const AstUnpackArrayDType* const typep = VN_CAST(dtypep, UnpackArrayDType)) {
            // Convert the element type
            const DfgDataType* const elemp = fromAst(typep->subDTypep());
            // If element type is not supported, we cannot handle the array either
            만약 (!elemp) 반환 nullptr;
            // Currently restricted to 1-dimensional arrays, just assumptions in the code
            만약 (elemp->isArray()) 반환 nullptr;
            // Unpacked array maps to Array
            반환 &array(*elemp, typep->elementsConst());
        }

        만약 (const AstPackArrayDType* const typep = VN_CAST(dtypep, PackArrayDType)) {
            // Packed array maps to Packed
            반환 &packed(typep->width());
        }

        만약 (const AstStructDType* const typep = VN_CAST(dtypep, StructDType)) {
            // Unpacked structs currently not supported
            만약 (!typep->packed()) 반환 nullptr;
            // Packed struct maps to Packed
            반환 &packed(typep->width());
        }

        만약 (const AstUnionDType* const typep = VN_CAST(dtypep, UnionDType)) {
            // Unpacked unions not supported
            만약 (!typep->packed()) 반환 nullptr;
            // Packed union maps to Packed
            반환 &packed(typep->width());
        }

        만약 (const AstBasicDType* const typep = VN_CAST(dtypep, BasicDType)) {
            // Anything not resembling a binary encoded number is not supported
            만약 (!typep->keyword().isIntNumeric()) 반환 nullptr;
            // Basic numeric types map to Packed
            반환 &packed(typep->width());
        }

        // Anything else is not representable
        반환 nullptr;
    }

    // Reset interned types
    static void reset() {
        위해 (const auto& pair : s_packedTypes) VL_DO_DANGLING(delete pair.second, pair.second);
        s_packedTypes.clear();
        delete s_nullTypep;
        s_nullTypep = nullptr;
    }
};

#endif
