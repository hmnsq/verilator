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

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن "V3Ast.h"
#ضمن "V3Error.h"
#ضمن "V3Global.h"
#ضمن "V3Hash.h"

#ضمن <memory>
#ضمن <unordered_map>

class DfgDataType final {
    تعداد class Kind : uint8_t {
        Packed,
        Array,
        // Tuple - to model unpacked structs in the future
        Null  // No type (like 'void', but that's a C++ keyword)
    };

    // STATE
    ثابت Kind m_kind;  // The type category
    uint32_t m_size;  // The number of elements in this type
    Ast_عقدة_DType* ثابت m_astDtypep;  // Equivalent canonical AstNodeDType
    ثابت DfgDataType* ثابت m_elemDtypep;  // Type of elements - for arrays only
    // Static singleton Null type
    ساكن ثابت DfgDataType* s_nullTypep;
    // Static map from 'width' -> 'interned Packed DfgDataType with that width'
    ساكن std::unordered_map<uint32_t, ثابت DfgDataType*> s_packedTypes;
    // Map from 'elements' -> 'interned Array DfgDataType with that many elements of *this* type'
    mutable std::unordered_map<uint32_t, ثابت DfgDataType*> m_arrayTypes;

    // METHODS
    ساكن Ast_عقدة_DType* canonicalPackedDType(uint32_t width) {
        إرجاع v3Global.rootp()->typeTablep()->findBitDType(width, width, VSigning::UNSIGNED);
    }
    ساكن Ast_عقدة_DType* canonicalArrayDType(uint32_t size, ثابت DfgDataType& elemType) {
        Ast_عقدة_DType* ثابت elemDTypep = elemType.m_astDtypep;
        File_سطر* ثابت flp = elemDTypep->fileline();
        AstRange* ثابت rangep = new AstRange{flp, static_cast<صحيح>(size - 1), 0};
        Ast_عقدة_DType* ثابت dtypep = new AstUnpackArrayDType{flp, elemDTypep, rangep};
        v3Global.rootp()->typeTablep()->addTypesp(dtypep);
        إرجاع dtypep;
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
    DfgDataType(uint32_t size, ثابت DfgDataType& elemType)
        : m_kind{Kind::Array}
        , m_size{size}
        , m_astDtypep{canonicalArrayDType(size, elemType)}
        , m_elemDtypep{&elemType} {}

    VL_UNCOPYABLE(DfgDataType);
    VL_UNMOVABLE(DfgDataType);
    ~DfgDataType() {
        لكل (ثابت auto& pair : m_arrayTypes) VL_DO_DANGLING(delete pair.second, pair.second);
    }

عام:
    //-----------------------------------------------------------------------
    // Properties

    bool isNull() ثابت { إرجاع m_kind == Kind::Null; }
    bool isPacked() ثابت { إرجاع m_kind == Kind::Packed; }
    bool isArray() ثابت { إرجاع m_kind == Kind::Array; }
    // Size of type (this is 'width' for Ppacked, 'elements' for Array, 0 for Null)
    uint32_t size() ثابت { إرجاع m_size; }
    Ast_عقدة_DType* astDtypep() ثابت { إرجاع m_astDtypep; }

    // Thanks to the interning, equality is identity
    bool operator==(ثابت DfgDataType& that) ثابت { إرجاع this == &that; }
    bool operator!=(ثابت DfgDataType& that) ثابت { إرجاع this != &that; }
    // Similarly for hash
    V3Hash hash() ثابت { إرجاع V3Hash{this}; }

    // Type of elements, for arrays only
    ثابت DfgDataType& elemDtype() ثابت {
        UASSERT(isArray(), "Non-array has no 'elemDType'");
        إرجاع *m_elemDtypep;
    }

    //-----------------------------------------------------------------------
    // Static factory and management functions

    // Returns a Packed type of the given width
    ساكن ثابت DfgDataType& packed(uint32_t width) {
        UASSERT(width > 0, "Width must be positive");
        // Find or create the right sized packed type
        ثابت DfgDataType*& entryr = s_packedTypes[width];
        إذا (!entryr) entryr = new DfgDataType{width};
        إرجاع *entryr;
    }

    // Returns an Array type of the given size with the given elements
    ساكن ثابت DfgDataType& array(ثابت DfgDataType& elemType, uint32_t size) {
        UASSERT(elemType.isPacked(), "Cannot create multi-dimensional arrays yet");
        // Find or create the right sized array type with this as elements
        ثابت DfgDataType*& entryr = elemType.m_arrayTypes[size];
        إذا (!entryr) entryr = new DfgDataType{size, elemType};
        إرجاع *entryr;
    }

    // Returns the singleton Null type
    ساكن ثابت DfgDataType& null() {
        إذا (!s_nullTypep) s_nullTypep = new DfgDataType{};
        إرجاع *s_nullTypep;
    }

    // Returns the data type of selecting the range [lo + size - 1 : lo] from this type
    ساكن ثابت DfgDataType& select(ثابت DfgDataType& dtype, uint32_t lo, uint32_t size) {
        بدل (dtype.m_kind) {
        حالة Kind::Packed: {
            UASSERT(lo + size - 1 < dtype.size(), "Out of range");
            إرجاع DfgDataType::packed(size);
        }
        حالة Kind::Array: {
            UASSERT(lo + size - 1 < dtype.size(), "Out of range");
            إرجاع DfgDataType::array(dtype.elemDtype(), size);
        }
        حالة Kind::Null: v3fatal("Type cannot be selected from");  // LCOV_EXCL_LINE
        }
        VL_UNREACHABLE;
    }

    // Construct a DfgDataType that represents the given AstNodeDType.
    // Returns nullptr if AstNodeDType is not representable by DfgDataType.
    ساكن ثابت DfgDataType* fromAst(ثابت Ast_عقدة_DType* dtypep) {
        dtypep = dtypep->skipRefp();

        إذا (ثابت AstUnpackArrayDType* ثابت typep = VN_CAST(dtypep, UnpackArrayDType)) {
            // Convert the element type
            ثابت DfgDataType* ثابت elemp = fromAst(typep->subDTypep());
            // If element type is not supported, we cannot handle the array either
            إذا (!elemp) إرجاع nullptr;
            // Currently restricted to 1-dimensional arrays, just assumptions in the code
            إذا (elemp->isArray()) إرجاع nullptr;
            // Unpacked array maps to Array
            إرجاع &array(*elemp, typep->elementsConst());
        }

        إذا (ثابت AstPackArrayDType* ثابت typep = VN_CAST(dtypep, PackArrayDType)) {
            // Packed array maps to Packed
            إرجاع &packed(typep->width());
        }

        إذا (ثابت AstStructDType* ثابت typep = VN_CAST(dtypep, StructDType)) {
            // Unpacked structs currently not supported
            إذا (!typep->packed()) إرجاع nullptr;
            // Packed struct maps to Packed
            إرجاع &packed(typep->width());
        }

        إذا (ثابت AstUnionDType* ثابت typep = VN_CAST(dtypep, UnionDType)) {
            // Unpacked unions not supported
            إذا (!typep->packed()) إرجاع nullptr;
            // Packed union maps to Packed
            إرجاع &packed(typep->width());
        }

        إذا (ثابت AstBasicDType* ثابت typep = VN_CAST(dtypep, BasicDType)) {
            // Anything not resembling a binary encoded number is not supported
            إذا (!typep->keyword().isIntNumeric()) إرجاع nullptr;
            // Basic numeric types map to Packed
            إرجاع &packed(typep->width());
        }

        // Anything else is not representable
        إرجاع nullptr;
    }

    // Reset interned types
    ساكن فراغ reset() {
        لكل (ثابت auto& pair : s_packedTypes) VL_DO_DANGLING(delete pair.second, pair.second);
        s_packedTypes.clear();
        delete s_nullTypep;
        s_nullTypep = nullptr;
    }
};

#endif
