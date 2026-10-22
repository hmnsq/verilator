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

#अगर_अघोषित VERILATOR_V3DFGDATATYPE_H_
#घोषणा VERILATOR_V3DFGDATATYPE_H_

#समावेश "config_build.h"
#समावेश "verilatedos.h"

#समावेश "V3Ast.h"
#समावेश "V3Error.h"
#समावेश "V3Global.h"
#समावेश "V3Hash.h"

#समावेश <memory>
#समावेश <unordered_map>

class Dfg_तथ्य_Type final {
    क्रमागत class Kind : uint8_t {
        Packed,
        Array,
        // Tuple - to model unpacked structs in the future
        Null  // No type (like 'void', but that's a C++ keyword)
    };

    // STATE
    स्थिर Kind m_kind;  // The type category
    uint32_t m_आकार;  // The number of elements in this type
    Ast_गाँठ_DType* स्थिर m_astDtypep;  // Equivalent canonical AstNodeDType
    स्थिर Dfg_तथ्य_Type* स्थिर m_elemDtypep;  // Type of elements - for arrays only
    // Static singleton Null type
    अटल स्थिर Dfg_तथ्य_Type* s_nullTypep;
    // Static map from 'width' -> 'interned Packed DfgDataType with that width'
    अटल std::unordered_map<uint32_t, स्थिर Dfg_तथ्य_Type*> s_packedTypes;
    // Map from 'elements' -> 'interned Array DfgDataType with that many elements of *this* type'
    mutable std::unordered_map<uint32_t, स्थिर Dfg_तथ्य_Type*> m_arrayTypes;

    // METHODS
    अटल Ast_गाँठ_DType* canonicalPackedDType(uint32_t width) {
        वापस v3Global.rootp()->typeTablep()->findBitDType(width, width, VSigning::UNSIGNED);
    }
    अटल Ast_गाँठ_DType* canonicalArrayDType(uint32_t आकार, स्थिर Dfg_तथ्य_Type& elemType) {
        Ast_गाँठ_DType* स्थिर elemDTypep = elemType.m_astDtypep;
        File_पंक्ति* स्थिर flp = elemDTypep->fileline();
        AstRange* स्थिर rangep = new AstRange{flp, static_cast<पूर्णांक>(आकार - 1), 0};
        Ast_गाँठ_DType* स्थिर dtypep = new AstUnpackArrayDType{flp, elemDTypep, rangep};
        v3Global.rootp()->typeTablep()->addTypesp(dtypep);
        वापस dtypep;
    }

    // CONSTRUCTOR - use 'fromAst' instead
    Dfg_तथ्य_Type()
        : m_kind{Kind::Null}
        , m_आकार{0}
        , m_astDtypep{v3Global.rootp()->findVoidDType()}
        , m_elemDtypep{nullptr} {}
    explicit Dfg_तथ्य_Type(uint32_t आकार)
        : m_kind{Kind::Packed}
        , m_आकार{आकार}
        , m_astDtypep{canonicalPackedDType(आकार)}
        , m_elemDtypep{nullptr} {}
    Dfg_तथ्य_Type(uint32_t आकार, स्थिर Dfg_तथ्य_Type& elemType)
        : m_kind{Kind::Array}
        , m_आकार{आकार}
        , m_astDtypep{canonicalArrayDType(आकार, elemType)}
        , m_elemDtypep{&elemType} {}

    VL_UNCOPYABLE(Dfg_तथ्य_Type);
    VL_UNMOVABLE(Dfg_तथ्य_Type);
    ~Dfg_तथ्य_Type() {
        क्रम (स्थिर स्वतः& pair : m_arrayTypes) VL_DO_DANGLING(delete pair.second, pair.second);
    }

खुला:
    //-----------------------------------------------------------------------
    // Properties

    bool isNull() स्थिर { वापस m_kind == Kind::Null; }
    bool isPacked() स्थिर { वापस m_kind == Kind::Packed; }
    bool isArray() स्थिर { वापस m_kind == Kind::Array; }
    // Size of type (this is 'width' for Ppacked, 'elements' for Array, 0 for Null)
    uint32_t आकार() स्थिर { वापस m_आकार; }
    Ast_गाँठ_DType* astDtypep() स्थिर { वापस m_astDtypep; }

    // Thanks to the interning, equality is identity
    bool चालक==(स्थिर Dfg_तथ्य_Type& that) स्थिर { वापस this == &that; }
    bool चालक!=(स्थिर Dfg_तथ्य_Type& that) स्थिर { वापस this != &that; }
    // Similarly for hash
    V3Hash hash() स्थिर { वापस V3Hash{this}; }

    // Type of elements, for arrays only
    स्थिर Dfg_तथ्य_Type& elemDtype() स्थिर {
        UASSERT(isArray(), "Non-array has no 'elemDType'");
        वापस *m_elemDtypep;
    }

    //-----------------------------------------------------------------------
    // Static factory and management functions

    // Returns a Packed type of the given width
    अटल स्थिर Dfg_तथ्य_Type& packed(uint32_t width) {
        UASSERT(width > 0, "Width must be positive");
        // Find or create the right sized packed type
        स्थिर Dfg_तथ्य_Type*& entryr = s_packedTypes[width];
        अगर (!entryr) entryr = new Dfg_तथ्य_Type{width};
        वापस *entryr;
    }

    // Returns an Array type of the given size with the given elements
    अटल स्थिर Dfg_तथ्य_Type& array(स्थिर Dfg_तथ्य_Type& elemType, uint32_t आकार) {
        UASSERT(elemType.isPacked(), "Cannot create multi-dimensional arrays yet");
        // Find or create the right sized array type with this as elements
        स्थिर Dfg_तथ्य_Type*& entryr = elemType.m_arrayTypes[आकार];
        अगर (!entryr) entryr = new Dfg_तथ्य_Type{आकार, elemType};
        वापस *entryr;
    }

    // Returns the singleton Null type
    अटल स्थिर Dfg_तथ्य_Type& null() {
        अगर (!s_nullTypep) s_nullTypep = new Dfg_तथ्य_Type{};
        वापस *s_nullTypep;
    }

    // Returns the data type of selecting the range [lo + size - 1 : lo] from this type
    अटल स्थिर Dfg_तथ्य_Type& select(स्थिर Dfg_तथ्य_Type& dtype, uint32_t lo, uint32_t आकार) {
        चयन (dtype.m_kind) {
        हाल Kind::Packed: {
            UASSERT(lo + आकार - 1 < dtype.आकार(), "Out of range");
            वापस Dfg_तथ्य_Type::packed(आकार);
        }
        हाल Kind::Array: {
            UASSERT(lo + आकार - 1 < dtype.आकार(), "Out of range");
            वापस Dfg_तथ्य_Type::array(dtype.elemDtype(), आकार);
        }
        हाल Kind::Null: v3fatal("Type cannot be selected from");  // LCOV_EXCL_LINE
        }
        VL_UNREACHABLE;
    }

    // Construct a DfgDataType that represents the given AstNodeDType.
    // Returns nullptr if AstNodeDType is not representable by DfgDataType.
    अटल स्थिर Dfg_तथ्य_Type* fromAst(स्थिर Ast_गाँठ_DType* dtypep) {
        dtypep = dtypep->skipRefp();

        अगर (स्थिर AstUnpackArrayDType* स्थिर typep = VN_CAST(dtypep, UnpackArrayDType)) {
            // Convert the element type
            स्थिर Dfg_तथ्य_Type* स्थिर elemp = fromAst(typep->subDTypep());
            // If element type is not supported, we cannot handle the array either
            अगर (!elemp) वापस nullptr;
            // Currently restricted to 1-dimensional arrays, just assumptions in the code
            अगर (elemp->isArray()) वापस nullptr;
            // Unpacked array maps to Array
            वापस &array(*elemp, typep->elementsConst());
        }

        अगर (स्थिर AstPackArrayDType* स्थिर typep = VN_CAST(dtypep, PackArrayDType)) {
            // Packed array maps to Packed
            वापस &packed(typep->width());
        }

        अगर (स्थिर AstStructDType* स्थिर typep = VN_CAST(dtypep, StructDType)) {
            // Unpacked structs currently not supported
            अगर (!typep->packed()) वापस nullptr;
            // Packed struct maps to Packed
            वापस &packed(typep->width());
        }

        अगर (स्थिर AstUnionDType* स्थिर typep = VN_CAST(dtypep, UnionDType)) {
            // Unpacked unions not supported
            अगर (!typep->packed()) वापस nullptr;
            // Packed union maps to Packed
            वापस &packed(typep->width());
        }

        अगर (स्थिर AstBasicDType* स्थिर typep = VN_CAST(dtypep, BasicDType)) {
            // Anything not resembling a binary encoded number is not supported
            अगर (!typep->keyword().isIntNumeric()) वापस nullptr;
            // Basic numeric types map to Packed
            वापस &packed(typep->width());
        }

        // Anything else is not representable
        वापस nullptr;
    }

    // Reset interned types
    अटल व्योम reset() {
        क्रम (स्थिर स्वतः& pair : s_packedTypes) VL_DO_DANGLING(delete pair.second, pair.second);
        s_packedTypes.clear();
        delete s_nullTypep;
        s_nullTypep = nullptr;
    }
};

#पूर्ण_अगर
