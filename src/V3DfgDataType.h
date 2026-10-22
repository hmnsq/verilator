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
    स्थिरांक Kind m_kind;  // The type category
    uint32_t m_size;  // The number of elements in this type
    Ast_गाठ_DType* स्थिरांक m_astDtypep;  // Equivalent canonical AstNodeDType
    स्थिरांक DfgDataType* स्थिरांक m_elemDtypep;  // Type of elements - for arrays only
    // Static singleton Null type
    स्थिर स्थिरांक DfgDataType* s_nullTypep;
    // Static map from 'width' -> 'interned Packed DfgDataType with that width'
    स्थिर std::unordered_map<uint32_t, स्थिरांक DfgDataType*> s_packedTypes;
    // Map from 'elements' -> 'interned Array DfgDataType with that many elements of *this* type'
    mutable std::unordered_map<uint32_t, स्थिरांक DfgDataType*> m_arrayTypes;

    // METHODS
    स्थिर Ast_गाठ_DType* canonicalPackedDType(uint32_t width) {
        परत v3Global.rootp()->typeTablep()->findBitDType(width, width, VSigning::UNSIGNED);
    }
    स्थिर Ast_गाठ_DType* canonicalArrayDType(uint32_t size, स्थिरांक DfgDataType& elemType) {
        Ast_गाठ_DType* स्थिरांक elemDTypep = elemType.m_astDtypep;
        File_ओळ* स्थिरांक flp = elemDTypep->fileline();
        AstRange* स्थिरांक rangep = new AstRange{flp, static_cast<पूर्णांक>(size - 1), 0};
        Ast_गाठ_DType* स्थिरांक dtypep = new AstUnpackArrayDType{flp, elemDTypep, rangep};
        v3Global.rootp()->typeTablep()->addTypesp(dtypep);
        परत dtypep;
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
    DfgDataType(uint32_t size, स्थिरांक DfgDataType& elemType)
        : m_kind{Kind::Array}
        , m_size{size}
        , m_astDtypep{canonicalArrayDType(size, elemType)}
        , m_elemDtypep{&elemType} {}

    VL_UNCOPYABLE(DfgDataType);
    VL_UNMOVABLE(DfgDataType);
    ~DfgDataType() {
        साठी (स्थिरांक स्वतः& pair : m_arrayTypes) VL_DO_DANGLING(delete pair.second, pair.second);
    }

सार्वजनिक:
    //-----------------------------------------------------------------------
    // Properties

    bool isNull() स्थिरांक { परत m_kind == Kind::Null; }
    bool isPacked() स्थिरांक { परत m_kind == Kind::Packed; }
    bool isArray() स्थिरांक { परत m_kind == Kind::Array; }
    // Size of type (this is 'width' for Ppacked, 'elements' for Array, 0 for Null)
    uint32_t size() स्थिरांक { परत m_size; }
    Ast_गाठ_DType* astDtypep() स्थिरांक { परत m_astDtypep; }

    // Thanks to the interning, equality is identity
    bool चालक==(स्थिरांक DfgDataType& that) स्थिरांक { परत this == &that; }
    bool चालक!=(स्थिरांक DfgDataType& that) स्थिरांक { परत this != &that; }
    // Similarly for hash
    V3Hash hash() स्थिरांक { परत V3Hash{this}; }

    // Type of elements, for arrays only
    स्थिरांक DfgDataType& elemDtype() स्थिरांक {
        UASSERT(isArray(), "Non-array has no 'elemDType'");
        परत *m_elemDtypep;
    }

    //-----------------------------------------------------------------------
    // Static factory and management functions

    // Returns a Packed type of the given width
    स्थिर स्थिरांक DfgDataType& packed(uint32_t width) {
        UASSERT(width > 0, "Width must be positive");
        // Find or create the right sized packed type
        स्थिरांक DfgDataType*& entryr = s_packedTypes[width];
        जर (!entryr) entryr = new DfgDataType{width};
        परत *entryr;
    }

    // Returns an Array type of the given size with the given elements
    स्थिर स्थिरांक DfgDataType& array(स्थिरांक DfgDataType& elemType, uint32_t size) {
        UASSERT(elemType.isPacked(), "Cannot create multi-dimensional arrays yet");
        // Find or create the right sized array type with this as elements
        स्थिरांक DfgDataType*& entryr = elemType.m_arrayTypes[size];
        जर (!entryr) entryr = new DfgDataType{size, elemType};
        परत *entryr;
    }

    // Returns the singleton Null type
    स्थिर स्थिरांक DfgDataType& null() {
        जर (!s_nullTypep) s_nullTypep = new DfgDataType{};
        परत *s_nullTypep;
    }

    // Returns the data type of selecting the range [lo + size - 1 : lo] from this type
    स्थिर स्थिरांक DfgDataType& select(स्थिरांक DfgDataType& dtype, uint32_t lo, uint32_t size) {
        निवड (dtype.m_kind) {
        स्थिती Kind::Packed: {
            UASSERT(lo + size - 1 < dtype.size(), "Out of range");
            परत DfgDataType::packed(size);
        }
        स्थिती Kind::Array: {
            UASSERT(lo + size - 1 < dtype.size(), "Out of range");
            परत DfgDataType::array(dtype.elemDtype(), size);
        }
        स्थिती Kind::Null: v3fatal("Type cannot be selected from");  // LCOV_EXCL_LINE
        }
        VL_UNREACHABLE;
    }

    // Construct a DfgDataType that represents the given AstNodeDType.
    // Returns nullptr if AstNodeDType is not representable by DfgDataType.
    स्थिर स्थिरांक DfgDataType* fromAst(स्थिरांक Ast_गाठ_DType* dtypep) {
        dtypep = dtypep->skipRefp();

        जर (स्थिरांक AstUnpackArrayDType* स्थिरांक typep = VN_CAST(dtypep, UnpackArrayDType)) {
            // Convert the element type
            स्थिरांक DfgDataType* स्थिरांक elemp = fromAst(typep->subDTypep());
            // If element type is not supported, we cannot handle the array either
            जर (!elemp) परत nullptr;
            // Currently restricted to 1-dimensional arrays, just assumptions in the code
            जर (elemp->isArray()) परत nullptr;
            // Unpacked array maps to Array
            परत &array(*elemp, typep->elementsConst());
        }

        जर (स्थिरांक AstPackArrayDType* स्थिरांक typep = VN_CAST(dtypep, PackArrayDType)) {
            // Packed array maps to Packed
            परत &packed(typep->width());
        }

        जर (स्थिरांक AstStructDType* स्थिरांक typep = VN_CAST(dtypep, StructDType)) {
            // Unpacked structs currently not supported
            जर (!typep->packed()) परत nullptr;
            // Packed struct maps to Packed
            परत &packed(typep->width());
        }

        जर (स्थिरांक AstUnionDType* स्थिरांक typep = VN_CAST(dtypep, UnionDType)) {
            // Unpacked unions not supported
            जर (!typep->packed()) परत nullptr;
            // Packed union maps to Packed
            परत &packed(typep->width());
        }

        जर (स्थिरांक AstBasicDType* स्थिरांक typep = VN_CAST(dtypep, BasicDType)) {
            // Anything not resembling a binary encoded number is not supported
            जर (!typep->keyword().isIntNumeric()) परत nullptr;
            // Basic numeric types map to Packed
            परत &packed(typep->width());
        }

        // Anything else is not representable
        परत nullptr;
    }

    // Reset interned types
    स्थिर शून्य reset() {
        साठी (स्थिरांक स्वतः& pair : s_packedTypes) VL_DO_DANGLING(delete pair.second, pair.second);
        s_packedTypes.clear();
        delete s_nullTypep;
        s_nullTypep = nullptr;
    }
};

#endif
