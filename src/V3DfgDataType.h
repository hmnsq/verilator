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
    अचर Kind m_kind;  // The type category
    uint32_t m_size;  // The number of elements in this type
    Ast_गाँठो_DType* अचर m_astDtypep;  // Equivalent canonical AstNodeDType
    अचर DfgDataType* अचर m_elemDtypep;  // Type of elements - for arrays only
    // Static singleton Null type
    स्थिर अचर DfgDataType* s_nullTypep;
    // Static map from 'width' -> 'interned Packed DfgDataType with that width'
    स्थिर std::unordered_map<uint32_t, अचर DfgDataType*> s_packedTypes;
    // Map from 'elements' -> 'interned Array DfgDataType with that many elements of *this* type'
    mutable std::unordered_map<uint32_t, अचर DfgDataType*> m_arrayTypes;

    // METHODS
    स्थिर Ast_गाँठो_DType* canonicalPackedDType(uint32_t width) {
        फर्काऊ v3Global.rootp()->typeTablep()->findBitDType(width, width, VSigning::UNSIGNED);
    }
    स्थिर Ast_गाँठो_DType* canonicalArrayDType(uint32_t size, अचर DfgDataType& elemType) {
        Ast_गाँठो_DType* अचर elemDTypep = elemType.m_astDtypep;
        File_पङ्क्ति* अचर flp = elemDTypep->fileline();
        AstRange* अचर rangep = new AstRange{flp, static_cast<पूर्णाङ्क>(size - 1), 0};
        Ast_गाँठो_DType* अचर dtypep = new AstUnpackArrayDType{flp, elemDTypep, rangep};
        v3Global.rootp()->typeTablep()->addTypesp(dtypep);
        फर्काऊ dtypep;
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
    DfgDataType(uint32_t size, अचर DfgDataType& elemType)
        : m_kind{Kind::Array}
        , m_size{size}
        , m_astDtypep{canonicalArrayDType(size, elemType)}
        , m_elemDtypep{&elemType} {}

    VL_UNCOPYABLE(DfgDataType);
    VL_UNMOVABLE(DfgDataType);
    ~DfgDataType() {
        लागि (अचर स्वतः& pair : m_arrayTypes) VL_DO_DANGLING(delete pair.second, pair.second);
    }

सार्वजनिक:
    //-----------------------------------------------------------------------
    // Properties

    bool isNull() अचर { फर्काऊ m_kind == Kind::Null; }
    bool isPacked() अचर { फर्काऊ m_kind == Kind::Packed; }
    bool isArray() अचर { फर्काऊ m_kind == Kind::Array; }
    // Size of type (this is 'width' for Ppacked, 'elements' for Array, 0 for Null)
    uint32_t size() अचर { फर्काऊ m_size; }
    Ast_गाँठो_DType* astDtypep() अचर { फर्काऊ m_astDtypep; }

    // Thanks to the interning, equality is identity
    bool चालक==(अचर DfgDataType& that) अचर { फर्काऊ this == &that; }
    bool चालक!=(अचर DfgDataType& that) अचर { फर्काऊ this != &that; }
    // Similarly for hash
    V3Hash hash() अचर { फर्काऊ V3Hash{this}; }

    // Type of elements, for arrays only
    अचर DfgDataType& elemDtype() अचर {
        UASSERT(isArray(), "Non-array has no 'elemDType'");
        फर्काऊ *m_elemDtypep;
    }

    //-----------------------------------------------------------------------
    // Static factory and management functions

    // Returns a Packed type of the given width
    स्थिर अचर DfgDataType& packed(uint32_t width) {
        UASSERT(width > 0, "Width must be positive");
        // Find or create the right sized packed type
        अचर DfgDataType*& entryr = s_packedTypes[width];
        यदि (!entryr) entryr = new DfgDataType{width};
        फर्काऊ *entryr;
    }

    // Returns an Array type of the given size with the given elements
    स्थिर अचर DfgDataType& array(अचर DfgDataType& elemType, uint32_t size) {
        UASSERT(elemType.isPacked(), "Cannot create multi-dimensional arrays yet");
        // Find or create the right sized array type with this as elements
        अचर DfgDataType*& entryr = elemType.m_arrayTypes[size];
        यदि (!entryr) entryr = new DfgDataType{size, elemType};
        फर्काऊ *entryr;
    }

    // Returns the singleton Null type
    स्थिर अचर DfgDataType& null() {
        यदि (!s_nullTypep) s_nullTypep = new DfgDataType{};
        फर्काऊ *s_nullTypep;
    }

    // Returns the data type of selecting the range [lo + size - 1 : lo] from this type
    स्थिर अचर DfgDataType& select(अचर DfgDataType& dtype, uint32_t lo, uint32_t size) {
        विकल्प (dtype.m_kind) {
        पक्ष Kind::Packed: {
            UASSERT(lo + size - 1 < dtype.size(), "Out of range");
            फर्काऊ DfgDataType::packed(size);
        }
        पक्ष Kind::Array: {
            UASSERT(lo + size - 1 < dtype.size(), "Out of range");
            फर्काऊ DfgDataType::array(dtype.elemDtype(), size);
        }
        पक्ष Kind::Null: v3fatal("Type cannot be selected from");  // LCOV_EXCL_LINE
        }
        VL_UNREACHABLE;
    }

    // Construct a DfgDataType that represents the given AstNodeDType.
    // Returns nullptr if AstNodeDType is not representable by DfgDataType.
    स्थिर अचर DfgDataType* fromAst(अचर Ast_गाँठो_DType* dtypep) {
        dtypep = dtypep->skipRefp();

        यदि (अचर AstUnpackArrayDType* अचर typep = VN_CAST(dtypep, UnpackArrayDType)) {
            // Convert the element type
            अचर DfgDataType* अचर elemp = fromAst(typep->subDTypep());
            // If element type is not supported, we cannot handle the array either
            यदि (!elemp) फर्काऊ nullptr;
            // Currently restricted to 1-dimensional arrays, just assumptions in the code
            यदि (elemp->isArray()) फर्काऊ nullptr;
            // Unpacked array maps to Array
            फर्काऊ &array(*elemp, typep->elementsConst());
        }

        यदि (अचर AstPackArrayDType* अचर typep = VN_CAST(dtypep, PackArrayDType)) {
            // Packed array maps to Packed
            फर्काऊ &packed(typep->width());
        }

        यदि (अचर AstStructDType* अचर typep = VN_CAST(dtypep, StructDType)) {
            // Unpacked structs currently not supported
            यदि (!typep->packed()) फर्काऊ nullptr;
            // Packed struct maps to Packed
            फर्काऊ &packed(typep->width());
        }

        यदि (अचर AstUnionDType* अचर typep = VN_CAST(dtypep, UnionDType)) {
            // Unpacked unions not supported
            यदि (!typep->packed()) फर्काऊ nullptr;
            // Packed union maps to Packed
            फर्काऊ &packed(typep->width());
        }

        यदि (अचर AstBasicDType* अचर typep = VN_CAST(dtypep, BasicDType)) {
            // Anything not resembling a binary encoded number is not supported
            यदि (!typep->keyword().isIntNumeric()) फर्काऊ nullptr;
            // Basic numeric types map to Packed
            फर्काऊ &packed(typep->width());
        }

        // Anything else is not representable
        फर्काऊ nullptr;
    }

    // Reset interned types
    स्थिर शून्य reset() {
        लागि (अचर स्वतः& pair : s_packedTypes) VL_DO_DANGLING(delete pair.second, pair.second);
        s_packedTypes.clear();
        delete s_nullTypep;
        s_nullTypep = nullptr;
    }
};

#endif
