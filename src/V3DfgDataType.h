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

#جيڪڏهن_وصف_نه_ٿيل VERILATOR_V3DFGDATATYPE_H_
#وصف_ڪريو VERILATOR_V3DFGDATATYPE_H_

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن "V3Ast.h"
#ضمن "V3Error.h"
#ضمن "V3Global.h"
#ضمن "V3Hash.h"

#ضمن <memory>
#ضمن <unordered_map>

class Dfg_معلومات_Type final {
    تعداد class قسم_2 : uint8_t {
        Packed,
        Array,
        // Tuple - to model unpacked structs in the future
        Null  // No type (like 'void', but that's a C++ keyword)
    };

    // STATE
    ثابت قسم_2 m_قسم;  // The type category
    uint32_t m_سائيز;  // The number of elements in this type
    Ast_ڳنڍ_DType* ثابت m_astDtypep;  // Equivalent canonical AstNodeDType
    ثابت Dfg_معلومات_Type* ثابت m_elemDtypep;  // Type of elements - for arrays only
    // Static singleton Null type
    ساكن ثابت Dfg_معلومات_Type* s_nullTypep;
    // Static map from 'width' -> 'interned Packed DfgDataType with that width'
    ساكن std::unordered_map<uint32_t, ثابت Dfg_معلومات_Type*> s_packedTypes;
    // Map from 'elements' -> 'interned Array DfgDataType with that many elements of *this* type'
    mutable std::unordered_map<uint32_t, ثابت Dfg_معلومات_Type*> m_arrayTypes;

    // METHODS
    ساكن Ast_ڳنڍ_DType* canonicalPackedDType(uint32_t ويڪر) {
        أرجع v3Global.rootp()->typeTablep()->ڳوليو_ٻٽو_انگ_DType(ويڪر, ويڪر, VSigning::UNSIGNED);
    }
    ساكن Ast_ڳنڍ_DType* canonicalArrayDType(uint32_t سائيز, ثابت Dfg_معلومات_Type& elemType) {
        Ast_ڳنڍ_DType* ثابت elemDTypep = elemType.m_astDtypep;
        File_سٽ* ثابت flp = elemDTypep->fileline();
        AstRange* ثابت rangep = new AstRange{flp, static_cast<صحيح>(سائيز - 1), 0};
        Ast_ڳنڍ_DType* ثابت dtypep = new AstUnpackArrayDType{flp, elemDTypep, rangep};
        v3Global.rootp()->typeTablep()->جوڙيو_Typesp(dtypep);
        أرجع dtypep;
    }

    // CONSTRUCTOR - use 'fromAst' instead
    Dfg_معلومات_Type()
        : m_قسم{قسم_2::Null}
        , m_سائيز{0}
        , m_astDtypep{v3Global.rootp()->ڳوليو_Void_DType()}
        , m_elemDtypep{nullptr} {}
    explicit Dfg_معلومات_Type(uint32_t سائيز)
        : m_قسم{قسم_2::Packed}
        , m_سائيز{سائيز}
        , m_astDtypep{canonicalPackedDType(سائيز)}
        , m_elemDtypep{nullptr} {}
    Dfg_معلومات_Type(uint32_t سائيز, ثابت Dfg_معلومات_Type& elemType)
        : m_قسم{قسم_2::Array}
        , m_سائيز{سائيز}
        , m_astDtypep{canonicalArrayDType(سائيز, elemType)}
        , m_elemDtypep{&elemType} {}

    VL_UNCOPYABLE(Dfg_معلومات_Type);
    VL_UNMOVABLE(Dfg_معلومات_Type);
    ~Dfg_معلومات_Type() {
        لكل (ثابت خودڪار& pair : m_arrayTypes) VL_DO_DANGLING(delete pair.ٻيو, pair.ٻيو);
    }

عام:
    //-----------------------------------------------------------------------
    // Properties

    bool isNull() ثابت { أرجع m_قسم == قسم_2::Null; }
    bool isPacked() ثابت { أرجع m_قسم == قسم_2::Packed; }
    bool isArray() ثابت { أرجع m_قسم == قسم_2::Array; }
    // Size of type (this is 'width' for Ppacked, 'elements' for Array, 0 for Null)
    uint32_t سائيز() ثابت { أرجع m_سائيز; }
    Ast_ڳنڍ_DType* astDtypep() ثابت { أرجع m_astDtypep; }

    // Thanks to the interning, equality is identity
    bool عامل==(ثابت Dfg_معلومات_Type& that) ثابت { أرجع this == &that; }
    bool عامل!=(ثابت Dfg_معلومات_Type& that) ثابت { أرجع this != &that; }
    // Similarly for hash
    V3Hash hash() ثابت { أرجع V3Hash{this}; }

    // Type of elements, for arrays only
    ثابت Dfg_معلومات_Type& elemDtype() ثابت {
        UASSERT(isArray(), "Non-array has no 'elemDType'");
        أرجع *m_elemDtypep;
    }

    //-----------------------------------------------------------------------
    // Static factory and management functions

    // Returns a Packed type of the given width
    ساكن ثابت Dfg_معلومات_Type& packed(uint32_t ويڪر) {
        UASSERT(ويڪر > 0, "Width must be positive");
        // Find or create the right sized packed type
        ثابت Dfg_معلومات_Type*& entryr = s_packedTypes[ويڪر];
        إذا (!entryr) entryr = new Dfg_معلومات_Type{ويڪر};
        أرجع *entryr;
    }

    // Returns an Array type of the given size with the given elements
    ساكن ثابت Dfg_معلومات_Type& array(ثابت Dfg_معلومات_Type& elemType, uint32_t سائيز) {
        UASSERT(elemType.isPacked(), "Cannot create multi-dimensional arrays yet");
        // Find or create the right sized array type with this as elements
        ثابت Dfg_معلومات_Type*& entryr = elemType.m_arrayTypes[سائيز];
        إذا (!entryr) entryr = new Dfg_معلومات_Type{سائيز, elemType};
        أرجع *entryr;
    }

    // Returns the singleton Null type
    ساكن ثابت Dfg_معلومات_Type& null() {
        إذا (!s_nullTypep) s_nullTypep = new Dfg_معلومات_Type{};
        أرجع *s_nullTypep;
    }

    // Returns the data type of selecting the range [lo + size - 1 : lo] from this type
    ساكن ثابت Dfg_معلومات_Type& select(ثابت Dfg_معلومات_Type& dtype, uint32_t lo, uint32_t سائيز) {
        بدل (dtype.m_قسم) {
        حالة قسم_2::Packed: {
            UASSERT(lo + سائيز - 1 < dtype.سائيز(), "Out of range");
            أرجع Dfg_معلومات_Type::packed(سائيز);
        }
        حالة قسم_2::Array: {
            UASSERT(lo + سائيز - 1 < dtype.سائيز(), "Out of range");
            أرجع Dfg_معلومات_Type::array(dtype.elemDtype(), سائيز);
        }
        حالة قسم_2::Null: v3fatal("Type cannot be selected from");  // LCOV_EXCL_LINE
        }
        VL_UNREACHABLE;
    }

    // Construct a DfgDataType that represents the given AstNodeDType.
    // Returns nullptr if AstNodeDType is not representable by DfgDataType.
    ساكن ثابت Dfg_معلومات_Type* fromAst(ثابت Ast_ڳنڍ_DType* dtypep) {
        dtypep = dtypep->skipRefp();

        إذا (ثابت AstUnpackArrayDType* ثابت typep = VN_CAST(dtypep, UnpackArrayDType)) {
            // Convert the element type
            ثابت Dfg_معلومات_Type* ثابت elemp = fromAst(typep->subDTypep());
            // If element type is not supported, we cannot handle the array either
            إذا (!elemp) أرجع nullptr;
            // Currently restricted to 1-dimensional arrays, just assumptions in the code
            إذا (elemp->isArray()) أرجع nullptr;
            // Unpacked array maps to Array
            أرجع &array(*elemp, typep->elementsConst());
        }

        إذا (ثابت AstPackArrayDType* ثابت typep = VN_CAST(dtypep, PackArrayDType)) {
            // Packed array maps to Packed
            أرجع &packed(typep->ويڪر());
        }

        إذا (ثابت AstStructDType* ثابت typep = VN_CAST(dtypep, StructDType)) {
            // Unpacked structs currently not supported
            إذا (!typep->packed()) أرجع nullptr;
            // Packed struct maps to Packed
            أرجع &packed(typep->ويڪر());
        }

        إذا (ثابت AstUnionDType* ثابت typep = VN_CAST(dtypep, UnionDType)) {
            // Unpacked unions not supported
            إذا (!typep->packed()) أرجع nullptr;
            // Packed union maps to Packed
            أرجع &packed(typep->ويڪر());
        }

        إذا (ثابت AstBasicDType* ثابت typep = VN_CAST(dtypep, BasicDType)) {
            // Anything not resembling a binary encoded number is not supported
            إذا (!typep->keyword().isIntNumeric()) أرجع nullptr;
            // Basic numeric types map to Packed
            أرجع &packed(typep->ويڪر());
        }

        // Anything else is not representable
        أرجع nullptr;
    }

    // Reset interned types
    ساكن فراغ reset() {
        لكل (ثابت خودڪار& pair : s_packedTypes) VL_DO_DANGLING(delete pair.ٻيو, pair.ٻيو);
        s_packedTypes.clear();
        delete s_nullTypep;
        s_nullTypep = nullptr;
    }
};

#جيڪڏهن_پورو
