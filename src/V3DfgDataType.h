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

#నిర్వచితం_కాకపోతే VERILATOR_V3DFGDATATYPE_H_
#నిర్వచించు VERILATOR_V3DFGDATATYPE_H_

#అన్తర్భావ "config_build.h"
#అన్తర్భావ "verilatedos.h"

#అన్తర్భావ "V3Ast.h"
#అన్తర్భావ "V3Error.h"
#అన్తర్భావ "V3Global.h"
#అన్తర్భావ "V3Hash.h"

#అన్తర్భావ <memory>
#అన్తర్భావ <unordered_map>

class Dfg_దత్తాంశం_Type final {
    గణన class రకం_2 : uint8_t {
        Packed,
        Array,
        // Tuple - to model unpacked structs in the future
        Null  // No type (like 'void', but that's a C++ keyword)
    };

    // STATE
    స్థిరరాశి రకం_2 m_రకం;  // The type category
    uint32_t m_కొలత;  // The number of elements in this type
    Ast_కణుపు_DType* స్థిరరాశి m_astDtypep;  // Equivalent canonical AstNodeDType
    స్థిరరాశి Dfg_దత్తాంశం_Type* స్థిరరాశి m_elemDtypep;  // Type of elements - for arrays only
    // Static singleton Null type
    స్థిర స్థిరరాశి Dfg_దత్తాంశం_Type* s_nullTypep;
    // Static map from 'width' -> 'interned Packed DfgDataType with that width'
    స్థిర std::unordered_map<uint32_t, స్థిరరాశి Dfg_దత్తాంశం_Type*> s_packedTypes;
    // Map from 'elements' -> 'interned Array DfgDataType with that many elements of *this* type'
    mutable std::unordered_map<uint32_t, స్థిరరాశి Dfg_దత్తాంశం_Type*> m_arrayTypes;

    // METHODS
    స్థిర Ast_కణుపు_DType* canonicalPackedDType(uint32_t వెడల్పు) {
        తిరిగి v3Global.rootp()->typeTablep()->కనుగొను_బిట్టు_DType(వెడల్పు, వెడల్పు, VSigning::UNSIGNED);
    }
    స్థిర Ast_కణుపు_DType* canonicalArrayDType(uint32_t కొలత, స్థిరరాశి Dfg_దత్తాంశం_Type& elemType) {
        Ast_కణుపు_DType* స్థిరరాశి elemDTypep = elemType.m_astDtypep;
        File_పంక్తి* స్థిరరాశి flp = elemDTypep->fileline();
        AstRange* స్థిరరాశి rangep = new AstRange{flp, static_cast<పూర్ణాంకం>(కొలత - 1), 0};
        Ast_కణుపు_DType* స్థిరరాశి dtypep = new AstUnpackArrayDType{flp, elemDTypep, rangep};
        v3Global.rootp()->typeTablep()->కూడు_Typesp(dtypep);
        తిరిగి dtypep;
    }

    // CONSTRUCTOR - use 'fromAst' instead
    Dfg_దత్తాంశం_Type()
        : m_రకం{రకం_2::Null}
        , m_కొలత{0}
        , m_astDtypep{v3Global.rootp()->కనుగొను_Void_DType()}
        , m_elemDtypep{nullptr} {}
    explicit Dfg_దత్తాంశం_Type(uint32_t కొలత)
        : m_రకం{రకం_2::Packed}
        , m_కొలత{కొలత}
        , m_astDtypep{canonicalPackedDType(కొలత)}
        , m_elemDtypep{nullptr} {}
    Dfg_దత్తాంశం_Type(uint32_t కొలత, స్థిరరాశి Dfg_దత్తాంశం_Type& elemType)
        : m_రకం{రకం_2::Array}
        , m_కొలత{కొలత}
        , m_astDtypep{canonicalArrayDType(కొలత, elemType)}
        , m_elemDtypep{&elemType} {}

    VL_UNCOPYABLE(Dfg_దత్తాంశం_Type);
    VL_UNMOVABLE(Dfg_దత్తాంశం_Type);
    ~Dfg_దత్తాంశం_Type() {
        కొరకు (స్థిరరాశి స్వయంచాలక& pair : m_arrayTypes) VL_DO_DANGLING(delete pair.రెండవ, pair.రెండవ);
    }

బహిరంగ:
    //-----------------------------------------------------------------------
    // Properties

    bool isNull() స్థిరరాశి { తిరిగి m_రకం == రకం_2::Null; }
    bool isPacked() స్థిరరాశి { తిరిగి m_రకం == రకం_2::Packed; }
    bool isArray() స్థిరరాశి { తిరిగి m_రకం == రకం_2::Array; }
    // Size of type (this is 'width' for Ppacked, 'elements' for Array, 0 for Null)
    uint32_t కొలత() స్థిరరాశి { తిరిగి m_కొలత; }
    Ast_కణుపు_DType* astDtypep() స్థిరరాశి { తిరిగి m_astDtypep; }

    // Thanks to the interning, equality is identity
    bool క్రియాకారకం==(స్థిరరాశి Dfg_దత్తాంశం_Type& that) స్థిరరాశి { తిరిగి this == &that; }
    bool క్రియాకారకం!=(స్థిరరాశి Dfg_దత్తాంశం_Type& that) స్థిరరాశి { తిరిగి this != &that; }
    // Similarly for hash
    V3Hash hash() స్థిరరాశి { తిరిగి V3Hash{this}; }

    // Type of elements, for arrays only
    స్థిరరాశి Dfg_దత్తాంశం_Type& elemDtype() స్థిరరాశి {
        UASSERT(isArray(), "Non-array has no 'elemDType'");
        తిరిగి *m_elemDtypep;
    }

    //-----------------------------------------------------------------------
    // Static factory and management functions

    // Returns a Packed type of the given width
    స్థిర స్థిరరాశి Dfg_దత్తాంశం_Type& packed(uint32_t వెడల్పు) {
        UASSERT(వెడల్పు > 0, "Width must be positive");
        // Find or create the right sized packed type
        స్థిరరాశి Dfg_దత్తాంశం_Type*& entryr = s_packedTypes[వెడల్పు];
        ఒకవేళ (!entryr) entryr = new Dfg_దత్తాంశం_Type{వెడల్పు};
        తిరిగి *entryr;
    }

    // Returns an Array type of the given size with the given elements
    స్థిర స్థిరరాశి Dfg_దత్తాంశం_Type& array(స్థిరరాశి Dfg_దత్తాంశం_Type& elemType, uint32_t కొలత) {
        UASSERT(elemType.isPacked(), "Cannot create multi-dimensional arrays yet");
        // Find or create the right sized array type with this as elements
        స్థిరరాశి Dfg_దత్తాంశం_Type*& entryr = elemType.m_arrayTypes[కొలత];
        ఒకవేళ (!entryr) entryr = new Dfg_దత్తాంశం_Type{కొలత, elemType};
        తిరిగి *entryr;
    }

    // Returns the singleton Null type
    స్థిర స్థిరరాశి Dfg_దత్తాంశం_Type& null() {
        ఒకవేళ (!s_nullTypep) s_nullTypep = new Dfg_దత్తాంశం_Type{};
        తిరిగి *s_nullTypep;
    }

    // Returns the data type of selecting the range [lo + size - 1 : lo] from this type
    స్థిర స్థిరరాశి Dfg_దత్తాంశం_Type& select(స్థిరరాశి Dfg_దత్తాంశం_Type& dtype, uint32_t lo, uint32_t కొలత) {
        ఎంపిక (dtype.m_రకం) {
        స్థితి రకం_2::Packed: {
            UASSERT(lo + కొలత - 1 < dtype.కొలత(), "Out of range");
            తిరిగి Dfg_దత్తాంశం_Type::packed(కొలత);
        }
        స్థితి రకం_2::Array: {
            UASSERT(lo + కొలత - 1 < dtype.కొలత(), "Out of range");
            తిరిగి Dfg_దత్తాంశం_Type::array(dtype.elemDtype(), కొలత);
        }
        స్థితి రకం_2::Null: v3fatal("Type cannot be selected from");  // LCOV_EXCL_LINE
        }
        VL_UNREACHABLE;
    }

    // Construct a DfgDataType that represents the given AstNodeDType.
    // Returns nullptr if AstNodeDType is not representable by DfgDataType.
    స్థిర స్థిరరాశి Dfg_దత్తాంశం_Type* fromAst(స్థిరరాశి Ast_కణుపు_DType* dtypep) {
        dtypep = dtypep->skipRefp();

        ఒకవేళ (స్థిరరాశి AstUnpackArrayDType* స్థిరరాశి typep = VN_CAST(dtypep, UnpackArrayDType)) {
            // Convert the element type
            స్థిరరాశి Dfg_దత్తాంశం_Type* స్థిరరాశి elemp = fromAst(typep->subDTypep());
            // If element type is not supported, we cannot handle the array either
            ఒకవేళ (!elemp) తిరిగి nullptr;
            // Currently restricted to 1-dimensional arrays, just assumptions in the code
            ఒకవేళ (elemp->isArray()) తిరిగి nullptr;
            // Unpacked array maps to Array
            తిరిగి &array(*elemp, typep->elementsConst());
        }

        ఒకవేళ (స్థిరరాశి AstPackArrayDType* స్థిరరాశి typep = VN_CAST(dtypep, PackArrayDType)) {
            // Packed array maps to Packed
            తిరిగి &packed(typep->వెడల్పు());
        }

        ఒకవేళ (స్థిరరాశి AstStructDType* స్థిరరాశి typep = VN_CAST(dtypep, StructDType)) {
            // Unpacked structs currently not supported
            ఒకవేళ (!typep->packed()) తిరిగి nullptr;
            // Packed struct maps to Packed
            తిరిగి &packed(typep->వెడల్పు());
        }

        ఒకవేళ (స్థిరరాశి AstUnionDType* స్థిరరాశి typep = VN_CAST(dtypep, UnionDType)) {
            // Unpacked unions not supported
            ఒకవేళ (!typep->packed()) తిరిగి nullptr;
            // Packed union maps to Packed
            తిరిగి &packed(typep->వెడల్పు());
        }

        ఒకవేళ (స్థిరరాశి AstBasicDType* స్థిరరాశి typep = VN_CAST(dtypep, BasicDType)) {
            // Anything not resembling a binary encoded number is not supported
            ఒకవేళ (!typep->keyword().isIntNumeric()) తిరిగి nullptr;
            // Basic numeric types map to Packed
            తిరిగి &packed(typep->వెడల్పు());
        }

        // Anything else is not representable
        తిరిగి nullptr;
    }

    // Reset interned types
    స్థిర శూన్య reset() {
        కొరకు (స్థిరరాశి స్వయంచాలక& pair : s_packedTypes) VL_DO_DANGLING(delete pair.రెండవ, pair.రెండవ);
        s_packedTypes.clear();
        delete s_nullTypep;
        s_nullTypep = nullptr;
    }
};

#అయితే_ముగింపు
