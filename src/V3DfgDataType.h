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

#അന്തര്ഭാവ "config_build.h"
#അന്തര്ഭാവ "verilatedos.h"

#അന്തര്ഭാവ "V3Ast.h"
#അന്തര്ഭാവ "V3Error.h"
#അന്തര്ഭാവ "V3Global.h"
#അന്തര്ഭാവ "V3Hash.h"

#അന്തര്ഭാവ <memory>
#അന്തര്ഭാവ <unordered_map>

class DfgDataType final {
    എണ്ണൽ class Kind : uint8_t {
        Packed,
        Array,
        // Tuple - to model unpacked structs in the future
        Null  // No type (like 'void', but that's a C++ keyword)
    };

    // STATE
    സ്ഥിരം Kind m_kind;  // The type category
    uint32_t m_size;  // The number of elements in this type
    Ast_കണ്ണി_DType* സ്ഥിരം m_astDtypep;  // Equivalent canonical AstNodeDType
    സ്ഥിരം DfgDataType* സ്ഥിരം m_elemDtypep;  // Type of elements - for arrays only
    // Static singleton Null type
    സ്ഥിതം സ്ഥിരം DfgDataType* s_nullTypep;
    // Static map from 'width' -> 'interned Packed DfgDataType with that width'
    സ്ഥിതം std::unordered_map<uint32_t, സ്ഥിരം DfgDataType*> s_packedTypes;
    // Map from 'elements' -> 'interned Array DfgDataType with that many elements of *this* type'
    mutable std::unordered_map<uint32_t, സ്ഥിരം DfgDataType*> m_arrayTypes;

    // METHODS
    സ്ഥിതം Ast_കണ്ണി_DType* canonicalPackedDType(uint32_t width) {
        തിരികെ v3Global.rootp()->typeTablep()->findBitDType(width, width, VSigning::UNSIGNED);
    }
    സ്ഥിതം Ast_കണ്ണി_DType* canonicalArrayDType(uint32_t size, സ്ഥിരം DfgDataType& elemType) {
        Ast_കണ്ണി_DType* സ്ഥിരം elemDTypep = elemType.m_astDtypep;
        File_വരി* സ്ഥിരം flp = elemDTypep->fileline();
        AstRange* സ്ഥിരം rangep = new AstRange{flp, static_cast<പൂർണ്ണസംഖ്യ>(size - 1), 0};
        Ast_കണ്ണി_DType* സ്ഥിരം dtypep = new AstUnpackArrayDType{flp, elemDTypep, rangep};
        v3Global.rootp()->typeTablep()->addTypesp(dtypep);
        തിരികെ dtypep;
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
    DfgDataType(uint32_t size, സ്ഥിരം DfgDataType& elemType)
        : m_kind{Kind::Array}
        , m_size{size}
        , m_astDtypep{canonicalArrayDType(size, elemType)}
        , m_elemDtypep{&elemType} {}

    VL_UNCOPYABLE(DfgDataType);
    VL_UNMOVABLE(DfgDataType);
    ~DfgDataType() {
        വേണ്ടി (സ്ഥിരം auto& pair : m_arrayTypes) VL_DO_DANGLING(delete pair.second, pair.second);
    }

പൊതു:
    //-----------------------------------------------------------------------
    // Properties

    bool isNull() സ്ഥിരം { തിരികെ m_kind == Kind::Null; }
    bool isPacked() സ്ഥിരം { തിരികെ m_kind == Kind::Packed; }
    bool isArray() സ്ഥിരം { തിരികെ m_kind == Kind::Array; }
    // Size of type (this is 'width' for Ppacked, 'elements' for Array, 0 for Null)
    uint32_t size() സ്ഥിരം { തിരികെ m_size; }
    Ast_കണ്ണി_DType* astDtypep() സ്ഥിരം { തിരികെ m_astDtypep; }

    // Thanks to the interning, equality is identity
    bool operator==(സ്ഥിരം DfgDataType& that) സ്ഥിരം { തിരികെ this == &that; }
    bool operator!=(സ്ഥിരം DfgDataType& that) സ്ഥിരം { തിരികെ this != &that; }
    // Similarly for hash
    V3Hash hash() സ്ഥിരം { തിരികെ V3Hash{this}; }

    // Type of elements, for arrays only
    സ്ഥിരം DfgDataType& elemDtype() സ്ഥിരം {
        UASSERT(isArray(), "Non-array has no 'elemDType'");
        തിരികെ *m_elemDtypep;
    }

    //-----------------------------------------------------------------------
    // Static factory and management functions

    // Returns a Packed type of the given width
    സ്ഥിതം സ്ഥിരം DfgDataType& packed(uint32_t width) {
        UASSERT(width > 0, "Width must be positive");
        // Find or create the right sized packed type
        സ്ഥിരം DfgDataType*& entryr = s_packedTypes[width];
        എങ്കിൽ (!entryr) entryr = new DfgDataType{width};
        തിരികെ *entryr;
    }

    // Returns an Array type of the given size with the given elements
    സ്ഥിതം സ്ഥിരം DfgDataType& array(സ്ഥിരം DfgDataType& elemType, uint32_t size) {
        UASSERT(elemType.isPacked(), "Cannot create multi-dimensional arrays yet");
        // Find or create the right sized array type with this as elements
        സ്ഥിരം DfgDataType*& entryr = elemType.m_arrayTypes[size];
        എങ്കിൽ (!entryr) entryr = new DfgDataType{size, elemType};
        തിരികെ *entryr;
    }

    // Returns the singleton Null type
    സ്ഥിതം സ്ഥിരം DfgDataType& null() {
        എങ്കിൽ (!s_nullTypep) s_nullTypep = new DfgDataType{};
        തിരികെ *s_nullTypep;
    }

    // Returns the data type of selecting the range [lo + size - 1 : lo] from this type
    സ്ഥിതം സ്ഥിരം DfgDataType& select(സ്ഥിരം DfgDataType& dtype, uint32_t lo, uint32_t size) {
        തിരഞ്ഞെടുപ്പ് (dtype.m_kind) {
        അവസ്ഥ Kind::Packed: {
            UASSERT(lo + size - 1 < dtype.size(), "Out of range");
            തിരികെ DfgDataType::packed(size);
        }
        അവസ്ഥ Kind::Array: {
            UASSERT(lo + size - 1 < dtype.size(), "Out of range");
            തിരികെ DfgDataType::array(dtype.elemDtype(), size);
        }
        അവസ്ഥ Kind::Null: v3fatal("Type cannot be selected from");  // LCOV_EXCL_LINE
        }
        VL_UNREACHABLE;
    }

    // Construct a DfgDataType that represents the given AstNodeDType.
    // Returns nullptr if AstNodeDType is not representable by DfgDataType.
    സ്ഥിതം സ്ഥിരം DfgDataType* fromAst(സ്ഥിരം Ast_കണ്ണി_DType* dtypep) {
        dtypep = dtypep->skipRefp();

        എങ്കിൽ (സ്ഥിരം AstUnpackArrayDType* സ്ഥിരം typep = VN_CAST(dtypep, UnpackArrayDType)) {
            // Convert the element type
            സ്ഥിരം DfgDataType* സ്ഥിരം elemp = fromAst(typep->subDTypep());
            // If element type is not supported, we cannot handle the array either
            എങ്കിൽ (!elemp) തിരികെ nullptr;
            // Currently restricted to 1-dimensional arrays, just assumptions in the code
            എങ്കിൽ (elemp->isArray()) തിരികെ nullptr;
            // Unpacked array maps to Array
            തിരികെ &array(*elemp, typep->elementsConst());
        }

        എങ്കിൽ (സ്ഥിരം AstPackArrayDType* സ്ഥിരം typep = VN_CAST(dtypep, PackArrayDType)) {
            // Packed array maps to Packed
            തിരികെ &packed(typep->width());
        }

        എങ്കിൽ (സ്ഥിരം AstStructDType* സ്ഥിരം typep = VN_CAST(dtypep, StructDType)) {
            // Unpacked structs currently not supported
            എങ്കിൽ (!typep->packed()) തിരികെ nullptr;
            // Packed struct maps to Packed
            തിരികെ &packed(typep->width());
        }

        എങ്കിൽ (സ്ഥിരം AstUnionDType* സ്ഥിരം typep = VN_CAST(dtypep, UnionDType)) {
            // Unpacked unions not supported
            എങ്കിൽ (!typep->packed()) തിരികെ nullptr;
            // Packed union maps to Packed
            തിരികെ &packed(typep->width());
        }

        എങ്കിൽ (സ്ഥിരം AstBasicDType* സ്ഥിരം typep = VN_CAST(dtypep, BasicDType)) {
            // Anything not resembling a binary encoded number is not supported
            എങ്കിൽ (!typep->keyword().isIntNumeric()) തിരികെ nullptr;
            // Basic numeric types map to Packed
            തിരികെ &packed(typep->width());
        }

        // Anything else is not representable
        തിരികെ nullptr;
    }

    // Reset interned types
    സ്ഥിതം ശൂന്യം reset() {
        വേണ്ടി (സ്ഥിരം auto& pair : s_packedTypes) VL_DO_DANGLING(delete pair.second, pair.second);
        s_packedTypes.clear();
        delete s_nullTypep;
        s_nullTypep = nullptr;
    }
};

#endif
