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

#شامل "config_build.h"
#شامل "verilatedos.h"

#شامل "V3Ast.h"
#شامل "V3Error.h"
#شامل "V3Global.h"
#شامل "V3Hash.h"

#شامل <memory>
#شامل <unordered_map>

class DfgDataType final {
    شمارش class Kind : uint8_t {
        Packed,
        Array,
        // Tuple - to model unpacked structs in the future
        Null  // No type (like 'void', but that's a C++ keyword)
    };

    // STATE
    ثابت Kind m_kind;  // The type category
    uint32_t m_size;  // The number of elements in this type
    Ast_گره_DType* ثابت m_astDtypep;  // Equivalent canonical AstNodeDType
    ثابت DfgDataType* ثابت m_elemDtypep;  // Type of elements - for arrays only
    // Static singleton Null type
    ایستا ثابت DfgDataType* s_nullTypep;
    // Static map from 'width' -> 'interned Packed DfgDataType with that width'
    ایستا std::unordered_map<uint32_t, ثابت DfgDataType*> s_packedTypes;
    // Map from 'elements' -> 'interned Array DfgDataType with that many elements of *this* type'
    mutable std::unordered_map<uint32_t, ثابت DfgDataType*> m_arrayTypes;

    // METHODS
    ایستا Ast_گره_DType* canonicalPackedDType(uint32_t width) {
        بازگشت v3Global.rootp()->typeTablep()->findBitDType(width, width, VSigning::UNSIGNED);
    }
    ایستا Ast_گره_DType* canonicalArrayDType(uint32_t size, ثابت DfgDataType& elemType) {
        Ast_گره_DType* ثابت elemDTypep = elemType.m_astDtypep;
        File_سطر* ثابت flp = elemDTypep->fileline();
        AstRange* ثابت rangep = new AstRange{flp, static_cast<صحیح>(size - 1), 0};
        Ast_گره_DType* ثابت dtypep = new AstUnpackArrayDType{flp, elemDTypep, rangep};
        v3Global.rootp()->typeTablep()->addTypesp(dtypep);
        بازگشت dtypep;
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
        برای (ثابت auto& pair : m_arrayTypes) VL_DO_DANGLING(delete pair.second, pair.second);
    }

عمومی:
    //-----------------------------------------------------------------------
    // Properties

    bool isNull() ثابت { بازگشت m_kind == Kind::Null; }
    bool isPacked() ثابت { بازگشت m_kind == Kind::Packed; }
    bool isArray() ثابت { بازگشت m_kind == Kind::Array; }
    // Size of type (this is 'width' for Ppacked, 'elements' for Array, 0 for Null)
    uint32_t size() ثابت { بازگشت m_size; }
    Ast_گره_DType* astDtypep() ثابت { بازگشت m_astDtypep; }

    // Thanks to the interning, equality is identity
    bool operator==(ثابت DfgDataType& that) ثابت { بازگشت this == &that; }
    bool operator!=(ثابت DfgDataType& that) ثابت { بازگشت this != &that; }
    // Similarly for hash
    V3Hash hash() ثابت { بازگشت V3Hash{this}; }

    // Type of elements, for arrays only
    ثابت DfgDataType& elemDtype() ثابت {
        UASSERT(isArray(), "Non-array has no 'elemDType'");
        بازگشت *m_elemDtypep;
    }

    //-----------------------------------------------------------------------
    // Static factory and management functions

    // Returns a Packed type of the given width
    ایستا ثابت DfgDataType& packed(uint32_t width) {
        UASSERT(width > 0, "Width must be positive");
        // Find or create the right sized packed type
        ثابت DfgDataType*& entryr = s_packedTypes[width];
        اگر (!entryr) entryr = new DfgDataType{width};
        بازگشت *entryr;
    }

    // Returns an Array type of the given size with the given elements
    ایستا ثابت DfgDataType& array(ثابت DfgDataType& elemType, uint32_t size) {
        UASSERT(elemType.isPacked(), "Cannot create multi-dimensional arrays yet");
        // Find or create the right sized array type with this as elements
        ثابت DfgDataType*& entryr = elemType.m_arrayTypes[size];
        اگر (!entryr) entryr = new DfgDataType{size, elemType};
        بازگشت *entryr;
    }

    // Returns the singleton Null type
    ایستا ثابت DfgDataType& null() {
        اگر (!s_nullTypep) s_nullTypep = new DfgDataType{};
        بازگشت *s_nullTypep;
    }

    // Returns the data type of selecting the range [lo + size - 1 : lo] from this type
    ایستا ثابت DfgDataType& select(ثابت DfgDataType& dtype, uint32_t lo, uint32_t size) {
        انتخاب (dtype.m_kind) {
        حالت Kind::Packed: {
            UASSERT(lo + size - 1 < dtype.size(), "Out of range");
            بازگشت DfgDataType::packed(size);
        }
        حالت Kind::Array: {
            UASSERT(lo + size - 1 < dtype.size(), "Out of range");
            بازگشت DfgDataType::array(dtype.elemDtype(), size);
        }
        حالت Kind::Null: v3fatal("Type cannot be selected from");  // LCOV_EXCL_LINE
        }
        VL_UNREACHABLE;
    }

    // Construct a DfgDataType that represents the given AstNodeDType.
    // Returns nullptr if AstNodeDType is not representable by DfgDataType.
    ایستا ثابت DfgDataType* fromAst(ثابت Ast_گره_DType* dtypep) {
        dtypep = dtypep->skipRefp();

        اگر (ثابت AstUnpackArrayDType* ثابت typep = VN_CAST(dtypep, UnpackArrayDType)) {
            // Convert the element type
            ثابت DfgDataType* ثابت elemp = fromAst(typep->subDTypep());
            // If element type is not supported, we cannot handle the array either
            اگر (!elemp) بازگشت nullptr;
            // Currently restricted to 1-dimensional arrays, just assumptions in the code
            اگر (elemp->isArray()) بازگشت nullptr;
            // Unpacked array maps to Array
            بازگشت &array(*elemp, typep->elementsConst());
        }

        اگر (ثابت AstPackArrayDType* ثابت typep = VN_CAST(dtypep, PackArrayDType)) {
            // Packed array maps to Packed
            بازگشت &packed(typep->width());
        }

        اگر (ثابت AstStructDType* ثابت typep = VN_CAST(dtypep, StructDType)) {
            // Unpacked structs currently not supported
            اگر (!typep->packed()) بازگشت nullptr;
            // Packed struct maps to Packed
            بازگشت &packed(typep->width());
        }

        اگر (ثابت AstUnionDType* ثابت typep = VN_CAST(dtypep, UnionDType)) {
            // Unpacked unions not supported
            اگر (!typep->packed()) بازگشت nullptr;
            // Packed union maps to Packed
            بازگشت &packed(typep->width());
        }

        اگر (ثابت AstBasicDType* ثابت typep = VN_CAST(dtypep, BasicDType)) {
            // Anything not resembling a binary encoded number is not supported
            اگر (!typep->keyword().isIntNumeric()) بازگشت nullptr;
            // Basic numeric types map to Packed
            بازگشت &packed(typep->width());
        }

        // Anything else is not representable
        بازگشت nullptr;
    }

    // Reset interned types
    ایستا تهی reset() {
        برای (ثابت auto& pair : s_packedTypes) VL_DO_DANGLING(delete pair.second, pair.second);
        s_packedTypes.clear();
        delete s_nullTypep;
        s_nullTypep = nullptr;
    }
};

#endif
