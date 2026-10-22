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

#اگر_غیر_تعریف_شدہ VERILATOR_V3DFGDATATYPE_H_
#تعریف_کرو VERILATOR_V3DFGDATATYPE_H_

#شامل "config_build.h"
#شامل "verilatedos.h"

#شامل "V3Ast.h"
#شامل "V3Error.h"
#شامل "V3Global.h"
#شامل "V3Hash.h"

#شامل <memory>
#شامل <unordered_map>

class Dfg_ڈیٹا_Type final {
    شمار class قسم_2 : uint8_t {
        Packed,
        Array,
        // Tuple - to model unpacked structs in the future
        Null  // No type (like 'void', but that's a C++ keyword)
    };

    // STATE
    ثابت قسم_2 m_قسم;  // The type category
    uint32_t m_سائز;  // The number of elements in this type
    Ast_گرہ_DType* ثابت m_astDtypep;  // Equivalent canonical AstNodeDType
    ثابت Dfg_ڈیٹا_Type* ثابت m_elemDtypep;  // Type of elements - for arrays only
    // Static singleton Null type
    ساکن ثابت Dfg_ڈیٹا_Type* s_nullTypep;
    // Static map from 'width' -> 'interned Packed DfgDataType with that width'
    ساکن std::unordered_map<uint32_t, ثابت Dfg_ڈیٹا_Type*> s_packedTypes;
    // Map from 'elements' -> 'interned Array DfgDataType with that many elements of *this* type'
    mutable std::unordered_map<uint32_t, ثابت Dfg_ڈیٹا_Type*> m_arrayTypes;

    // METHODS
    ساکن Ast_گرہ_DType* canonicalPackedDType(uint32_t چوڑائی) {
        واپس v3Global.rootp()->typeTablep()->ڈھونڈو_ثنائی_ہندسہ_DType(چوڑائی, چوڑائی, VSigning::UNSIGNED);
    }
    ساکن Ast_گرہ_DType* canonicalArrayDType(uint32_t سائز, ثابت Dfg_ڈیٹا_Type& elemType) {
        Ast_گرہ_DType* ثابت elemDTypep = elemType.m_astDtypep;
        File_سطر* ثابت flp = elemDTypep->fileline();
        AstRange* ثابت rangep = new AstRange{flp, static_cast<صحیح>(سائز - 1), 0};
        Ast_گرہ_DType* ثابت dtypep = new AstUnpackArrayDType{flp, elemDTypep, rangep};
        v3Global.rootp()->typeTablep()->جوڑو_Typesp(dtypep);
        واپس dtypep;
    }

    // CONSTRUCTOR - use 'fromAst' instead
    Dfg_ڈیٹا_Type()
        : m_قسم{قسم_2::Null}
        , m_سائز{0}
        , m_astDtypep{v3Global.rootp()->ڈھونڈو_Void_DType()}
        , m_elemDtypep{nullptr} {}
    explicit Dfg_ڈیٹا_Type(uint32_t سائز)
        : m_قسم{قسم_2::Packed}
        , m_سائز{سائز}
        , m_astDtypep{canonicalPackedDType(سائز)}
        , m_elemDtypep{nullptr} {}
    Dfg_ڈیٹا_Type(uint32_t سائز, ثابت Dfg_ڈیٹا_Type& elemType)
        : m_قسم{قسم_2::Array}
        , m_سائز{سائز}
        , m_astDtypep{canonicalArrayDType(سائز, elemType)}
        , m_elemDtypep{&elemType} {}

    VL_UNCOPYABLE(Dfg_ڈیٹا_Type);
    VL_UNMOVABLE(Dfg_ڈیٹا_Type);
    ~Dfg_ڈیٹا_Type() {
        برائے (ثابت خودکار& pair : m_arrayTypes) VL_DO_DANGLING(delete pair.دوسرا, pair.دوسرا);
    }

عوامی:
    //-----------------------------------------------------------------------
    // Properties

    bool isNull() ثابت { واپس m_قسم == قسم_2::Null; }
    bool isPacked() ثابت { واپس m_قسم == قسم_2::Packed; }
    bool isArray() ثابت { واپس m_قسم == قسم_2::Array; }
    // Size of type (this is 'width' for Ppacked, 'elements' for Array, 0 for Null)
    uint32_t سائز() ثابت { واپس m_سائز; }
    Ast_گرہ_DType* astDtypep() ثابت { واپس m_astDtypep; }

    // Thanks to the interning, equality is identity
    bool عامل==(ثابت Dfg_ڈیٹا_Type& that) ثابت { واپس this == &that; }
    bool عامل!=(ثابت Dfg_ڈیٹا_Type& that) ثابت { واپس this != &that; }
    // Similarly for hash
    V3Hash hash() ثابت { واپس V3Hash{this}; }

    // Type of elements, for arrays only
    ثابت Dfg_ڈیٹا_Type& elemDtype() ثابت {
        UASSERT(isArray(), "Non-array has no 'elemDType'");
        واپس *m_elemDtypep;
    }

    //-----------------------------------------------------------------------
    // Static factory and management functions

    // Returns a Packed type of the given width
    ساکن ثابت Dfg_ڈیٹا_Type& packed(uint32_t چوڑائی) {
        UASSERT(چوڑائی > 0, "Width must be positive");
        // Find or create the right sized packed type
        ثابت Dfg_ڈیٹا_Type*& entryr = s_packedTypes[چوڑائی];
        اگر (!entryr) entryr = new Dfg_ڈیٹا_Type{چوڑائی};
        واپس *entryr;
    }

    // Returns an Array type of the given size with the given elements
    ساکن ثابت Dfg_ڈیٹا_Type& array(ثابت Dfg_ڈیٹا_Type& elemType, uint32_t سائز) {
        UASSERT(elemType.isPacked(), "Cannot create multi-dimensional arrays yet");
        // Find or create the right sized array type with this as elements
        ثابت Dfg_ڈیٹا_Type*& entryr = elemType.m_arrayTypes[سائز];
        اگر (!entryr) entryr = new Dfg_ڈیٹا_Type{سائز, elemType};
        واپس *entryr;
    }

    // Returns the singleton Null type
    ساکن ثابت Dfg_ڈیٹا_Type& null() {
        اگر (!s_nullTypep) s_nullTypep = new Dfg_ڈیٹا_Type{};
        واپس *s_nullTypep;
    }

    // Returns the data type of selecting the range [lo + size - 1 : lo] from this type
    ساکن ثابت Dfg_ڈیٹا_Type& select(ثابت Dfg_ڈیٹا_Type& dtype, uint32_t lo, uint32_t سائز) {
        انتخاب (dtype.m_قسم) {
        حالت قسم_2::Packed: {
            UASSERT(lo + سائز - 1 < dtype.سائز(), "Out of range");
            واپس Dfg_ڈیٹا_Type::packed(سائز);
        }
        حالت قسم_2::Array: {
            UASSERT(lo + سائز - 1 < dtype.سائز(), "Out of range");
            واپس Dfg_ڈیٹا_Type::array(dtype.elemDtype(), سائز);
        }
        حالت قسم_2::Null: v3fatal("Type cannot be selected from");  // LCOV_EXCL_LINE
        }
        VL_UNREACHABLE;
    }

    // Construct a DfgDataType that represents the given AstNodeDType.
    // Returns nullptr if AstNodeDType is not representable by DfgDataType.
    ساکن ثابت Dfg_ڈیٹا_Type* fromAst(ثابت Ast_گرہ_DType* dtypep) {
        dtypep = dtypep->skipRefp();

        اگر (ثابت AstUnpackArrayDType* ثابت typep = VN_CAST(dtypep, UnpackArrayDType)) {
            // Convert the element type
            ثابت Dfg_ڈیٹا_Type* ثابت elemp = fromAst(typep->subDTypep());
            // If element type is not supported, we cannot handle the array either
            اگر (!elemp) واپس nullptr;
            // Currently restricted to 1-dimensional arrays, just assumptions in the code
            اگر (elemp->isArray()) واپس nullptr;
            // Unpacked array maps to Array
            واپس &array(*elemp, typep->elementsConst());
        }

        اگر (ثابت AstPackArrayDType* ثابت typep = VN_CAST(dtypep, PackArrayDType)) {
            // Packed array maps to Packed
            واپس &packed(typep->چوڑائی());
        }

        اگر (ثابت AstStructDType* ثابت typep = VN_CAST(dtypep, StructDType)) {
            // Unpacked structs currently not supported
            اگر (!typep->packed()) واپس nullptr;
            // Packed struct maps to Packed
            واپس &packed(typep->چوڑائی());
        }

        اگر (ثابت AstUnionDType* ثابت typep = VN_CAST(dtypep, UnionDType)) {
            // Unpacked unions not supported
            اگر (!typep->packed()) واپس nullptr;
            // Packed union maps to Packed
            واپس &packed(typep->چوڑائی());
        }

        اگر (ثابت AstBasicDType* ثابت typep = VN_CAST(dtypep, BasicDType)) {
            // Anything not resembling a binary encoded number is not supported
            اگر (!typep->keyword().isIntNumeric()) واپس nullptr;
            // Basic numeric types map to Packed
            واپس &packed(typep->چوڑائی());
        }

        // Anything else is not representable
        واپس nullptr;
    }

    // Reset interned types
    ساکن خالی reset() {
        برائے (ثابت خودکار& pair : s_packedTypes) VL_DO_DANGLING(delete pair.دوسرا, pair.دوسرا);
        s_packedTypes.clear();
        delete s_nullTypep;
        s_nullTypep = nullptr;
    }
};

#اگر_ختم
