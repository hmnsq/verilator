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

#अन्तर्भाव "config_build.h"
#अन्तर्भाव "verilatedos.h"

#अन्तर्भाव "V3Ast.h"
#अन्तर्भाव "V3Error.h"
#अन्तर्भाव "V3Global.h"
#अन्तर्भाव "V3Hash.h"

#अन्तर्भाव <memory>
#अन्तर्भाव <unordered_map>

class DfgDataType final {
    गणना class Kind : uint8_t {
        Packed,
        Array,
        // Tuple - to model unpacked structs in the future
        Null  // No type (like 'void', but that's a C++ keyword)
    };

    // STATE
    ध्रुव Kind m_kind;  // The type category
    uint32_t m_size;  // The number of elements in this type
    Ast_गाँठ_DType* ध्रुव m_astDtypep;  // Equivalent canonical AstNodeDType
    ध्रुव DfgDataType* ध्रुव m_elemDtypep;  // Type of elements - for arrays only
    // Static singleton Null type
    स्थिर ध्रुव DfgDataType* s_nullTypep;
    // Static map from 'width' -> 'interned Packed DfgDataType with that width'
    स्थिर std::unordered_map<uint32_t, ध्रुव DfgDataType*> s_packedTypes;
    // Map from 'elements' -> 'interned Array DfgDataType with that many elements of *this* type'
    mutable std::unordered_map<uint32_t, ध्रुव DfgDataType*> m_arrayTypes;

    // METHODS
    स्थिर Ast_गाँठ_DType* canonicalPackedDType(uint32_t width) {
        प्रत्यागम v3Global.rootp()->typeTablep()->findBitDType(width, width, VSigning::UNSIGNED);
    }
    स्थिर Ast_गाँठ_DType* canonicalArrayDType(uint32_t size, ध्रुव DfgDataType& elemType) {
        Ast_गाँठ_DType* ध्रुव elemDTypep = elemType.m_astDtypep;
        File_पंक्ति* ध्रुव flp = elemDTypep->fileline();
        AstRange* ध्रुव rangep = new AstRange{flp, static_cast<पूर्णाङ्क>(size - 1), 0};
        Ast_गाँठ_DType* ध्रुव dtypep = new AstUnpackArrayDType{flp, elemDTypep, rangep};
        v3Global.rootp()->typeTablep()->addTypesp(dtypep);
        प्रत्यागम dtypep;
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
    DfgDataType(uint32_t size, ध्रुव DfgDataType& elemType)
        : m_kind{Kind::Array}
        , m_size{size}
        , m_astDtypep{canonicalArrayDType(size, elemType)}
        , m_elemDtypep{&elemType} {}

    VL_UNCOPYABLE(DfgDataType);
    VL_UNMOVABLE(DfgDataType);
    ~DfgDataType() {
        प्रति (ध्रुव स्वतः& pair : m_arrayTypes) VL_DO_DANGLING(delete pair.second, pair.second);
    }

सार्वजनिक:
    //-----------------------------------------------------------------------
    // Properties

    bool isNull() ध्रुव { प्रत्यागम m_kind == Kind::Null; }
    bool isPacked() ध्रुव { प्रत्यागम m_kind == Kind::Packed; }
    bool isArray() ध्रुव { प्रत्यागम m_kind == Kind::Array; }
    // Size of type (this is 'width' for Ppacked, 'elements' for Array, 0 for Null)
    uint32_t size() ध्रुव { प्रत्यागम m_size; }
    Ast_गाँठ_DType* astDtypep() ध्रुव { प्रत्यागम m_astDtypep; }

    // Thanks to the interning, equality is identity
    bool चालक==(ध्रुव DfgDataType& that) ध्रुव { प्रत्यागम this == &that; }
    bool चालक!=(ध्रुव DfgDataType& that) ध्रुव { प्रत्यागम this != &that; }
    // Similarly for hash
    V3Hash hash() ध्रुव { प्रत्यागम V3Hash{this}; }

    // Type of elements, for arrays only
    ध्रुव DfgDataType& elemDtype() ध्रुव {
        UASSERT(isArray(), "Non-array has no 'elemDType'");
        प्रत्यागम *m_elemDtypep;
    }

    //-----------------------------------------------------------------------
    // Static factory and management functions

    // Returns a Packed type of the given width
    स्थिर ध्रुव DfgDataType& packed(uint32_t width) {
        UASSERT(width > 0, "Width must be positive");
        // Find or create the right sized packed type
        ध्रुव DfgDataType*& entryr = s_packedTypes[width];
        यदि (!entryr) entryr = new DfgDataType{width};
        प्रत्यागम *entryr;
    }

    // Returns an Array type of the given size with the given elements
    स्थिर ध्रुव DfgDataType& array(ध्रुव DfgDataType& elemType, uint32_t size) {
        UASSERT(elemType.isPacked(), "Cannot create multi-dimensional arrays yet");
        // Find or create the right sized array type with this as elements
        ध्रुव DfgDataType*& entryr = elemType.m_arrayTypes[size];
        यदि (!entryr) entryr = new DfgDataType{size, elemType};
        प्रत्यागम *entryr;
    }

    // Returns the singleton Null type
    स्थिर ध्रुव DfgDataType& null() {
        यदि (!s_nullTypep) s_nullTypep = new DfgDataType{};
        प्रत्यागम *s_nullTypep;
    }

    // Returns the data type of selecting the range [lo + size - 1 : lo] from this type
    स्थिर ध्रुव DfgDataType& select(ध्रुव DfgDataType& dtype, uint32_t lo, uint32_t size) {
        विकल्प (dtype.m_kind) {
        पक्ष Kind::Packed: {
            UASSERT(lo + size - 1 < dtype.size(), "Out of range");
            प्रत्यागम DfgDataType::packed(size);
        }
        पक्ष Kind::Array: {
            UASSERT(lo + size - 1 < dtype.size(), "Out of range");
            प्रत्यागम DfgDataType::array(dtype.elemDtype(), size);
        }
        पक्ष Kind::Null: v3fatal("Type cannot be selected from");  // LCOV_EXCL_LINE
        }
        VL_UNREACHABLE;
    }

    // Construct a DfgDataType that represents the given AstNodeDType.
    // Returns nullptr if AstNodeDType is not representable by DfgDataType.
    स्थिर ध्रुव DfgDataType* fromAst(ध्रुव Ast_गाँठ_DType* dtypep) {
        dtypep = dtypep->skipRefp();

        यदि (ध्रुव AstUnpackArrayDType* ध्रुव typep = VN_CAST(dtypep, UnpackArrayDType)) {
            // Convert the element type
            ध्रुव DfgDataType* ध्रुव elemp = fromAst(typep->subDTypep());
            // If element type is not supported, we cannot handle the array either
            यदि (!elemp) प्रत्यागम nullptr;
            // Currently restricted to 1-dimensional arrays, just assumptions in the code
            यदि (elemp->isArray()) प्रत्यागम nullptr;
            // Unpacked array maps to Array
            प्रत्यागम &array(*elemp, typep->elementsConst());
        }

        यदि (ध्रुव AstPackArrayDType* ध्रुव typep = VN_CAST(dtypep, PackArrayDType)) {
            // Packed array maps to Packed
            प्रत्यागम &packed(typep->width());
        }

        यदि (ध्रुव AstStructDType* ध्रुव typep = VN_CAST(dtypep, StructDType)) {
            // Unpacked structs currently not supported
            यदि (!typep->packed()) प्रत्यागम nullptr;
            // Packed struct maps to Packed
            प्रत्यागम &packed(typep->width());
        }

        यदि (ध्रुव AstUnionDType* ध्रुव typep = VN_CAST(dtypep, UnionDType)) {
            // Unpacked unions not supported
            यदि (!typep->packed()) प्रत्यागम nullptr;
            // Packed union maps to Packed
            प्रत्यागम &packed(typep->width());
        }

        यदि (ध्रुव AstBasicDType* ध्रुव typep = VN_CAST(dtypep, BasicDType)) {
            // Anything not resembling a binary encoded number is not supported
            यदि (!typep->keyword().isIntNumeric()) प्रत्यागम nullptr;
            // Basic numeric types map to Packed
            प्रत्यागम &packed(typep->width());
        }

        // Anything else is not representable
        प्रत्यागम nullptr;
    }

    // Reset interned types
    स्थिर शून्य reset() {
        प्रति (ध्रुव स्वतः& pair : s_packedTypes) VL_DO_DANGLING(delete pair.second, pair.second);
        s_packedTypes.clear();
        delete s_nullTypep;
        s_nullTypep = nullptr;
    }
};

#endif
