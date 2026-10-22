// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: AstNode sub-types representing data types
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
// This files contains all 'AstNode' sub-types that relate to the
// representation of data types.
//
//*************************************************************************

#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ VERILATOR_V3ASTNODEDTYPE_H_
#ವ್ಯಾಖ್ಯಾನಿಸು VERILATOR_V3ASTNODEDTYPE_H_

#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ VERILATOR_V3AST_H_
#ದೋಷ "Use V3Ast.h as the include"
#ಅನ್ತರ್ಭಾವ "V3Ast.h"  // This helps code analysis tools pick up symbols in V3Ast.h
#ವ್ಯಾಖ್ಯಾನಿಸು VL_NOT_FINAL  // This #define fixes broken code folding in the CLion IDE
#ಆದರೆ_ಅಂತ್ಯ

// === Abstract base node types (AstNode*) =====================================

class Ast_ಗಂಟು_DType VL_NOT_FINAL : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    // Ideally width() would migrate to BasicDType as that's where it makes sense,
    // but it's currently so prevalent in the code we leave it here.
    // Note the below members are included in AstTypeTable::Key lookups
    // dist-ast-dump-suppress  // Part of dumpSmall
    ಪೂರ್ಣಾಂಕ m_ಅಗಲ = 0;  // (also in AstTypeTable::Key) Bit width of operation
    // (also in AstTypeTable::Key) If unsized, bitwidth of minimum implementation
    // dist-ast-dump-suppress  // Part of dumpSmall
    ಪೂರ್ಣಾಂಕ m_ಅಗಲ_Min = 0;
    // dist-ast-dump-suppress  // Part of dumpSmall
    VSigning m_numeric;  // (also in AstTypeTable::Key) Node is signed
    // dist-ast-dump-suppress  // Part of dumpSmall
    bool m_generic = false;  // Simple globally referenced type, don't garbage collect
    // Unique number assigned to each dtype during creation for IEEE matching
    ಸ್ಥಿರ ಪೂರ್ಣಾಂಕ s_unique_ಅಂಕೆ;

ಸಂರಕ್ಷಿತ:
    // CONSTRUCTORS
    Ast_ಗಂಟು_DType(VNType t, File_ಸಾಲು* fl)
        : Ast_ಗಂಟು{t, fl} {}

ಖಾಸಗಿ:
    // METHODS
    ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* skipRefIterp(bool skipConst, bool skipEnum,
                                     bool assertOn = true) ಸ್ಥಿರಾಂಕ VL_MT_STABLE;

ಸಂರಕ್ಷಿತ:
    // METHODS
    ವಾಸ್ತವಿಕ bool similar_DType_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* samep) ಸ್ಥಿರಾಂಕ = 0;

ಸಾರ್ವಜನಿಕ:
    ASTGEN_MEMBERS_Ast_ಗಂಟು_DType;
    // ACCESSORS
    ಶೂನ್ಯ dump(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ವಾಸ್ತವಿಕ ಶೂನ್ಯ dumpSmall(std::ostream& str) ಸ್ಥಿರಾಂಕ VL_MT_STABLE;
    bool hasDType() ಸ್ಥಿರಾಂಕ override VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ true; }
    /// Require VlUnpacked, instead of [] for POD elements.
    /// A non-POD object is always compound, but some POD elements
    /// are compound when methods calls operate on object, or when
    /// under another compound-requiring object e.g. class
    ವಾಸ್ತವಿಕ bool isCompound() ಸ್ಥಿರಾಂಕ = 0;
    // Integral or packed, allowed inside an unpacked union/struct
    ವಾಸ್ತವಿಕ bool isIntegralOrPacked() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ !isCompound(); }
    // (Slow) recurse down to find basic data type
    ವಾಸ್ತವಿಕ AstBasicDType* basicp() ಸ್ಥಿರಾಂಕ VL_MT_STABLE = 0;
    // (Slow) Recurse over MemberDType|ParamTypeDType|RefDType|ConstDType|EnumDType to other type
    ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* skipRefp() ಸ್ಥಿರಾಂಕ VL_MT_STABLE { ಹಿಂತಿರುಗಿ skipRefIterp(true, true); }
    Ast_ಗಂಟು_DType* skipRefp() VL_MT_STABLE {
        ಹಿಂತಿರುಗಿ const_cast<Ast_ಗಂಟು_DType*>(
            static_cast<ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType*>(this)->skipRefIterp(true, true));
    }
    // (Slow) Recurse over MemberDType|ParamTypeDType|RefDType|ConstDType|EnumDType to other type,
    // Returns null if not resolved
    ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* skipRefOrNullp() ಸ್ಥಿರಾಂಕ VL_MT_STABLE {
        ಹಿಂತಿರುಗಿ skipRefIterp(true, true, false);
    }
    Ast_ಗಂಟು_DType* skipRefOrNullp() VL_MT_STABLE {
        ಹಿಂತಿರುಗಿ const_cast<Ast_ಗಂಟು_DType*>(
            static_cast<ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType*>(this)->skipRefIterp(true, true, false));
    }
    // (Slow) Recurse over MemberDType|ParamTypeDType|RefDType|EnumDType to ConstDType
    ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* skip_Ref_ಗಮ್ಯ_Constp() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ skipRefIterp(false, true); }
    Ast_ಗಂಟು_DType* skip_Ref_ಗಮ್ಯ_Constp() {
        ಹಿಂತಿರುಗಿ const_cast<Ast_ಗಂಟು_DType*>(
            static_cast<ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType*>(this)->skipRefIterp(false, true));
    }
    // (Slow) Recurse over MemberDType|ParamTypeDType|RefDType|ConstDType to EnumDType
    ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* skip_Ref_ಗಮ್ಯ_Enump() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ skipRefIterp(true, false); }
    Ast_ಗಂಟು_DType* skip_Ref_ಗಮ್ಯ_Enump() {
        ಹಿಂತಿರುಗಿ const_cast<Ast_ಗಂಟು_DType*>(
            static_cast<ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType*>(this)->skipRefIterp(true, false));
    }
    // (Slow) Recurse over MemberDType|ParamTypeDType|RefDType|ConstDType to EnumDType,
    // Returns null if not resolved
    ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* skip_Ref_ಗಮ್ಯ_Enum_Or_Nullp() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ skipRefIterp(true, false, false); }
    // (Slow) Recurse over MemberDType|ParamTypeDType|RefDType to other type
    ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* skip_Ref_ಗಮ್ಯ_Non_Refp() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ skipRefIterp(false, false); }
    Ast_ಗಂಟು_DType* skip_Ref_ಗಮ್ಯ_Non_Refp() {
        ಹಿಂತಿರುಗಿ const_cast<Ast_ಗಂಟು_DType*>(
            static_cast<ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType*>(this)->skipRefIterp(false, false));
    }
    // If array, returns element dtype, otherwise returns skipRef dtype
    // If skipRef is false, RefDTypes are not followed (safe before typedef linking)
    ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* elemDTypep(bool skipRef = true) ಸ್ಥಿರಾಂಕ VL_MT_STABLE;
    Ast_ಗಂಟು_DType* elemDTypep(bool skipRef = true) VL_MT_STABLE {
        ಹಿಂತಿರುಗಿ const_cast<Ast_ಗಂಟು_DType*>(
            static_cast<ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType*>(this)->elemDTypep(skipRef));
    }
    // (Slow) recurses - Structure alignment 1,2,4 or 8 bytes (arrays affect this)
    ವಾಸ್ತವಿಕ ಪೂರ್ಣಾಂಕ ಅಗಲ_Align_ಬೈಟುಗಳು() ಸ್ಥಿರಾಂಕ = 0;
    // (Slow) recurses - Width in bytes rounding up 1,2,4,8,12,...
    ವಾಸ್ತವಿಕ ಪೂರ್ಣಾಂಕ ಅಗಲ_ಒಟ್ಟು_ಬೈಟುಗಳು() ಸ್ಥಿರಾಂಕ = 0;
    bool maybe_Pointed_ಗಮ್ಯ() ಸ್ಥಿರಾಂಕ override VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ true; }
    // Iff has a non-null refDTypep(), as generic node function
    ವಾಸ್ತವಿಕ Ast_ಗಂಟು_DType* virtRefDTypep() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ nullptr; }
    // Iff has refDTypep(), set as generic node function
    ವಾಸ್ತವಿಕ ಶೂನ್ಯ virtRefDTypep(Ast_ಗಂಟು_DType* nodep) {}
    // Iff has a non-null second dtypep, as generic node function
    ವಾಸ್ತವಿಕ Ast_ಗಂಟು_DType* virtRefDType2p() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ nullptr; }
    // Iff has second dtype, set as generic node function
    ವಾಸ್ತವಿಕ ಶೂನ್ಯ virtRefDType2p(Ast_ಗಂಟು_DType* nodep) {}
    // Assignable equivalence.  Calls skipRefToNonRefp() during comparisons.
    bool similarDType(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* samep) ಸ್ಥಿರಾಂಕ;
    // Iff has a non-null subDTypep(), as generic node function
    ವಾಸ್ತವಿಕ Ast_ಗಂಟು_DType* subDTypep() ಸ್ಥಿರಾಂಕ VL_MT_STABLE { ಹಿಂತಿರುಗಿ nullptr; }
    ವಾಸ್ತವಿಕ Ast_ಗಂಟು_DType* subDType2p() ಸ್ಥಿರಾಂಕ VL_MT_STABLE { ಹಿಂತಿರುಗಿ nullptr; }
    ವಾಸ್ತವಿಕ bool isAggregateType() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ false; }
    // True for unpacked, dynamic, queue, and associative arrays (not packed arrays)
    bool isNonPackedArray() ಸ್ಥಿರಾಂಕ;
    ವಾಸ್ತವಿಕ bool isFourstate() ಸ್ಥಿರಾಂಕ;
    // Ideally an IEEE $typename
    ವಾಸ್ತವಿಕ string prettyDTypeName(bool) ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ prettyTypeName(); }
    string prettyDTypeNameQ(bool ತುಂಬಿದ_2 = false) ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ "'" + prettyDTypeName(ತುಂಬಿದ_2) + "'"; }
    string stateDTypeName() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ this->isFourstate() ? "(4-state)" : "(2-state)"; }
    //
    // Changing the width may confuse the data type resolution, so must clear
    // TypeTable cache after use.
    ಶೂನ್ಯ ಅಗಲ_Force(ಪೂರ್ಣಾಂಕ ಅಗಲ, ಪೂರ್ಣಾಂಕ ಅಗಲ_Min) {
        m_ಅಗಲ = ಅಗಲ;
        m_ಅಗಲ_Min = ಅಗಲ_Min;
    }
    // For backward compatibility inherit width and signing from the subDType/base type
    ಶೂನ್ಯ ಅಗಲ_From_Sub(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* nodep) {
        m_ಅಗಲ = nodep->m_ಅಗಲ;
        m_ಅಗಲ_Min = nodep->m_ಅಗಲ_Min;
        m_numeric = nodep->m_numeric;
    }
    //
    ಪೂರ್ಣಾಂಕ ಅಗಲ() ಸ್ಥಿರಾಂಕ VL_MT_STABLE { ಹಿಂತಿರುಗಿ m_ಅಗಲ; }
    ಶೂನ್ಯ numeric(VSigning flag) { m_numeric = flag; }
    bool isSigned() ಸ್ಥಿರಾಂಕ VL_MT_STABLE { ಹಿಂತಿರುಗಿ m_numeric.isSigned(); }
    bool isNosign() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ m_numeric.isNosign(); }
    VSigning numeric() ಸ್ಥಿರಾಂಕ VL_MT_STABLE { ಹಿಂತಿರುಗಿ m_numeric; }
    ಪೂರ್ಣಾಂಕ ಅಗಲ_ಪದಗಳು() ಸ್ಥಿರಾಂಕ VL_MT_STABLE { ಹಿಂತಿರುಗಿ VL_ಪದಗಳು_I(ಅಗಲ()); }
    ಪೂರ್ಣಾಂಕ ಅಗಲ_Min() ಸ್ಥಿರಾಂಕ VL_MT_STABLE {  // If sized, the size,
                                         // if unsized the min digits to represent it
        ಹಿಂತಿರುಗಿ m_ಅಗಲ_Min ? m_ಅಗಲ_Min : m_ಅಗಲ;
    }
    ಪೂರ್ಣಾಂಕ ಅಗಲ_Pow2() ಸ್ಥಿರಾಂಕ;
    ಶೂನ್ಯ ಅಗಲ_Min_From_ಅಗಲ() { m_ಅಗಲ_Min = m_ಅಗಲ; }
    bool ಅಗಲ_Sized() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ !m_ಅಗಲ_Min || m_ಅಗಲ_Min == m_ಅಗಲ; }
    bool generic() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ m_generic; }
    ಶೂನ್ಯ generic(bool flag) { m_generic = flag; }
    std::pair<uint32_t, uint32_t> dimensions(bool includeBasic) ಸ್ಥಿರಾಂಕ;
    uint32_t arrayUnpackedElements() ಸ್ಥಿರಾಂಕ;  // 1, or total multiplication of all dimensions
    // Fixed aggregate streaming properties
    bool isStreamableFixedAggregate() ಸ್ಥಿರಾಂಕ;
    // Whether is a string, or an unpacked array or structure containing one
    bool ಒಳಗೊಂಡಿದೆ_String() ಸ್ಥಿರಾಂಕ;
    bool ಒಳಗೊಂಡಿದೆ_Unpacked_Struct() ಸ್ಥಿರಾಂಕ;
    ಪೂರ್ಣಾಂಕ ಅಗಲ_ಹರಿವು() ಸ್ಥಿರಾಂಕ;
    string vlEnumType() ಸ್ಥಿರಾಂಕ;  // Return VerilatedVarType: VLVT_UINT32, etc
    string vlEnumPackedAgg() ಸ್ಥಿರಾಂಕ;  // Return VLVF_PACKED_AGG etc flags, or empty
    ಸ್ಥಿರ ಪೂರ್ಣಾಂಕ unique_ಅಂಕೆ_Inc() { ಹಿಂತಿರುಗಿ ++s_unique_ಅಂಕೆ; }
    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* charIQWN() ಸ್ಥಿರಾಂಕ {
        ಹಿಂತಿರುಗಿ (isString() ? "N" : isWide() ? "W" : isDouble() ? "D" : isQuad() ? "Q" : "I");
    }
    string cType(ಸ್ಥಿರಾಂಕ string& name, bool forFunc, bool isRef, bool packed = false) ಸ್ಥಿರಾಂಕ;
    // Represents a C++ LiteralType? (can be constexpr)
    bool isLiteralType() ಸ್ಥಿರಾಂಕ VL_MT_STABLE;
    // C++ type (as emitted by cType) is trivially default constructible and destructible?
    bool isTriviallyConstructible() ಸ್ಥಿರಾಂಕ VL_MT_STABLE;
    ವಾಸ್ತವಿಕ bool isDynamicallySized() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ false; }

ಖಾಸಗಿ:
    class CTypeRecursed;
    CTypeRecursed cTypeRecurse(bool compound, bool packed) ಸ್ಥಿರಾಂಕ;
};
class Ast_ಗಂಟು_Array_DType VL_NOT_FINAL : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_DType {
    // Array data type, ie "some_dtype var_name [2:0]"
    // @astgen op1 := childDTypep : Optional[AstNodeDType] // moved to refDTypep() in V3Width
    // @astgen op2 := rangep : Optional[AstRange] // array bounds
    //
    // @astgen ptr := m_refDTypep : Optional[AstNodeDType]  // Elements of this type (post-width)
    Ast_ಗಂಟು* rangenp() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ reinterpret_cast<Ast_ಗಂಟು*>(rangep()); }

ಸಂರಕ್ಷಿತ:
    Ast_ಗಂಟು_Array_DType(VNType t, File_ಸಾಲು* fl)
        : Ast_ಗಂಟು_DType{t, fl} {}

ಸಾರ್ವಜನಿಕ:
    ASTGEN_MEMBERS_Ast_ಗಂಟು_Array_DType;
    ಶೂನ್ಯ dump(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpSmall(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* broken() ಸ್ಥಿರಾಂಕ override {
        BROKEN_RTN(!((m_refDTypep && !ಮಗು_DTypep()) || (!m_refDTypep && ಮಗು_DTypep())));
        ಹಿಂತಿರುಗಿ nullptr;
    }
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* samep) ಸ್ಥಿರಾಂಕ override {
        ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Array_DType* ಸ್ಥಿರಾಂಕ asamep = VN_DBG_AS(samep, ಗಂಟು_Array_DType);
        ಹಿಂತಿರುಗಿ hi() == asamep->hi() && rangenp()->same_ವೃಕ್ಷ(asamep->rangenp())
               && subDTypep() == asamep->subDTypep();
    }  // HashedDT doesn't recurse, so need to check children
    bool similar_DType_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* samep) ಸ್ಥಿರಾಂಕ override {
        ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Array_DType* ಸ್ಥಿರಾಂಕ asamep = VN_DBG_AS(samep, ಗಂಟು_Array_DType);
        ಹಿಂತಿರುಗಿ elementsConst() == asamep->elementsConst()
               && subDTypep()->similarDType(asamep->subDTypep());
    }
    Ast_ಗಂಟು_DType* get_ಮಗು_DTypep() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ ಮಗು_DTypep(); }
    Ast_ಗಂಟು_DType* subDTypep() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE {
        ಹಿಂತಿರುಗಿ m_refDTypep ? m_refDTypep : ಮಗು_DTypep();
    }
    ಶೂನ್ಯ refDTypep(Ast_ಗಂಟು_DType* nodep) { m_refDTypep = nodep; }
    Ast_ಗಂಟು_DType* virtRefDTypep() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ m_refDTypep; }
    ಶೂನ್ಯ virtRefDTypep(Ast_ಗಂಟು_DType* nodep) override { refDTypep(nodep); }
    // METHODS
    AstBasicDType* basicp() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE {
        ಹಿಂತಿರುಗಿ subDTypep()->basicp();
    }  // (Slow) recurse down to find basic data type
    ಪೂರ್ಣಾಂಕ ಅಗಲ_Align_ಬೈಟುಗಳು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ subDTypep()->ಅಗಲ_Align_ಬೈಟುಗಳು(); }
    ಪೂರ್ಣಾಂಕ ಅಗಲ_ಒಟ್ಟು_ಬೈಟುಗಳು() ಸ್ಥಿರಾಂಕ override {
        ಹಿಂತಿರುಗಿ elementsConst() * subDTypep()->ಅಗಲ_ಒಟ್ಟು_ಬೈಟುಗಳು();
    }
    ಸಾಲೊಳಗೆ ಪೂರ್ಣಾಂಕ ಎಡ_2() ಸ್ಥಿರಾಂಕ VL_MT_STABLE;
    ಸಾಲೊಳಗೆ ಪೂರ್ಣಾಂಕ ಬಲ_2() ಸ್ಥಿರಾಂಕ VL_MT_STABLE;
    ಸಾಲೊಳಗೆ ಪೂರ್ಣಾಂಕ hi() ಸ್ಥಿರಾಂಕ VL_MT_STABLE;
    ಸಾಲೊಳಗೆ ಪೂರ್ಣಾಂಕ lo() ಸ್ಥಿರಾಂಕ VL_MT_STABLE;
    ಸಾಲೊಳಗೆ ಪೂರ್ಣಾಂಕ elementsConst() ಸ್ಥಿರಾಂಕ VL_MT_STABLE;
    ಸಾಲೊಳಗೆ VNumRange declRange() ಸ್ಥಿರಾಂಕ VL_MT_STABLE;
};
class Ast_ಗಂಟು_UOr_Struct_DType VL_NOT_FINAL : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_DType {
    // A struct or union; common handling
    // @astgen op1 := membersp : List[AstMemberDType]
    //
    // @astgen ptr := m_classOrPackagep : Optional[AstNodeModule]  // Package emitted with
    string m_name;  // Name from upper typedef, if any
    string m_typedefName;  // Typedef's dtypeName() once moved off the typedef declaring it
    ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ m_unique_ಅಂಕೆ;  // Unique ID distinguishing this dtype instance, for hashing/naming
    bool m_packed;  // Packed struct/union, else unpacked
    bool m_isFourstate = false;  // V3Width computes; true if any member is 4-state
    bool m_constrainedRand = false;  // True if struct has constraint expression
    bool m_emit_ಗಮ್ಯ_String = false;  // Generate to_string() for this struct/union if set

ಸಂರಕ್ಷಿತ:
    Ast_ಗಂಟು_UOr_Struct_DType(VNType t, File_ಸಾಲು* fl, VSigning numericUnpack)
        : Ast_ಗಂಟು_DType{t, fl}
        , m_unique_ಅಂಕೆ{unique_ಅಂಕೆ_Inc()} {
        // VSigning::NOSIGN overloaded to indicate not packed
        m_packed = (numericUnpack != VSigning::NOSIGN);
        numeric(VSigning::fromBool(numericUnpack.isSigned()));
    }
    Ast_ಗಂಟು_UOr_Struct_DType(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_UOr_Struct_DType& other)
        : Ast_ಗಂಟು_DType{other}
        , m_name{other.m_name}
        , m_typedefName{other.m_typedefName}
        , m_unique_ಅಂಕೆ{unique_ಅಂಕೆ_Inc()}
        , m_packed{other.m_packed}
        , m_isFourstate{other.m_isFourstate} {}

ಸಾರ್ವಜನಿಕ:
    ASTGEN_MEMBERS_Ast_ಗಂಟು_UOr_Struct_DType;
    ಪೂರ್ಣಾಂಕ unique_ಅಂಕೆ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_unique_ಅಂಕೆ; }
    ಶೂನ್ಯ dump(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    string prettyDTypeName(bool) ಸ್ಥಿರಾಂಕ override;
    bool isCompound() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ !packed(); }
    // For basicp() we reuse the size to indicate a "fake" basic type of same size
    AstBasicDType* basicp() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE {
        ಆದರೆ (!m_packed) ಹಿಂತಿರುಗಿ nullptr;
        ಹಿಂತಿರುಗಿ (isFourstate()
                    ? VN_AS(ಕಂಡುಹಿಡಿ_Logic_Range_DType(VNumRange{ಅಗಲ() - 1, 0}, ಅಗಲ(), numeric()),
                            BasicDType)
                    : VN_AS(ಕಂಡುಹಿಡಿ_ಬಿಟ್_Range_DType(VNumRange{ಅಗಲ() - 1, 0}, ಅಗಲ(), numeric()),
                            BasicDType));
    }
    // (Slow) recurses - Structure alignment 1,2,4 or 8 bytes (arrays affect this)
    ಪೂರ್ಣಾಂಕ ಅಗಲ_Align_ಬೈಟುಗಳು() ಸ್ಥಿರಾಂಕ override;
    // (Slow) recurses - Width in bytes rounding up 1,2,4,8,12,...
    ಪೂರ್ಣಾಂಕ ಅಗಲ_ಒಟ್ಟು_ಬೈಟುಗಳು() ಸ್ಥಿರಾಂಕ override;
    bool similar_DType_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* samep) ಸ್ಥಿರಾಂಕ override;
    string name() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE { ಹಿಂತಿರುಗಿ m_name; }
    ಶೂನ್ಯ name(ಸ್ಥಿರಾಂಕ string& flag) override { m_name = flag; }
    // Keep the name the typedef declaring it gives, as it moves off it to the type table
    ಶೂನ್ಯ typedefName(ಸ್ಥಿರಾಂಕ string& name) { m_typedefName = name; }
    // Name of the C structure for DPI, empty if no typedef names it, so declared where used
    string dpiName() ಸ್ಥಿರಾಂಕ;
    // C typedef of the unpacked structure for DPI (IEEE 1800-2023 H.7.3)
    string dpiTypedef() ಸ್ಥಿರಾಂಕ;
    bool packed() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ m_packed; }
    ಶೂನ್ಯ packed(bool flag) { m_packed = flag; }
    // packed() but as don't support unpacked, presently all structs
    ಸ್ಥಿರ bool packedUnsup() { ಹಿಂತಿರುಗಿ true; }
    ಶೂನ್ಯ isFourstate(bool flag) { m_isFourstate = flag; }
    bool isFourstate() ಸ್ಥಿರಾಂಕ override VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ m_isFourstate; }
    ಸ್ಥಿರ ಪೂರ್ಣಾಂಕ lo() VL_MT_STABLE { ಹಿಂತಿರುಗಿ 0; }
    ಪೂರ್ಣಾಂಕ hi() ಸ್ಥಿರಾಂಕ VL_MT_STABLE {
        ಹಿಂತಿರುಗಿ dtypep()->ಅಗಲ() - 1;
    }  // Packed classes look like arrays
    VNumRange declRange() ಸ್ಥಿರಾಂಕ VL_MT_STABLE { ಹಿಂತಿರುಗಿ VNumRange{hi(), lo()}; }
    Ast_ಗಂಟು_Module* classOrPackagep() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_classOrPackagep; }
    ಶೂನ್ಯ classOrPackagep(Ast_ಗಂಟು_Module* classpackagep) { m_classOrPackagep = classpackagep; }
    bool isConstrainedRand() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_constrainedRand; }
    ಶೂನ್ಯ markConstrainedRand(bool flag) { m_constrainedRand = flag; }
    bool emit_ಗಮ್ಯ_String() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_emit_ಗಮ್ಯ_String; }
    ಶೂನ್ಯ ಗಣ_Emit_ಗಮ್ಯ_String() { m_emit_ಗಮ್ಯ_String = true; }
    bool isAggregateType() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ !packed(); }
};

// === Concrete node types =====================================================

// === AstNode ===
class Ast_Enum_ಅಂಶ final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    // @astgen op1 := rangep : Optional[AstRange] // Range for name appending
    // @astgen op2 := valuep : Optional[AstNodeExpr]
    string m_name;  // Name of item

ಸಾರ್ವಜನಿಕ:
    // Parents: ENUM
    Ast_Enum_ಅಂಶ(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ string& name, AstRange* rangep, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* valuep)
        : ASTGEN_SUPER_Enum_ಅಂಶ(fl)
        , m_name{name} {
        this->rangep(rangep);
        this->valuep(valuep);
    }
    ASTGEN_MEMBERS_Ast_Enum_ಅಂಶ;
    string name() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE { ಹಿಂತಿರುಗಿ m_name; }
    bool maybe_Pointed_ಗಮ್ಯ() ಸ್ಥಿರಾಂಕ override VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ true; }
    bool hasDType() ಸ್ಥಿರಾಂಕ override VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ true; }
    ಶೂನ್ಯ name(ಸ್ಥಿರಾಂಕ string& flag) override { m_name = flag; }
};

// === AstNodeDType ===
class AstAssocArrayDType final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_DType {
    // Associative array data type, ie "[some_dtype]"
    // @astgen op1 := childDTypep : Optional[AstNodeDType] // moved to refDTypep() in V3Width
    // @astgen op2 := keyChildDTypep : Optional[AstNodeDType]
    //
    // @astgen ptr := m_refDTypep : Optional[AstNodeDType]  // Elements of this type (post-width)
    // @astgen ptr := m_keyDTypep : Optional[AstNodeDType]  // Keys of this type (post-width)
ಸಾರ್ವಜನಿಕ:
    AstAssocArrayDType(File_ಸಾಲು* fl, VFlag_ಮಗು_DType, Ast_ಗಂಟು_DType* dtp, Ast_ಗಂಟು_DType* ಕೀಲಿ_Dtp)
        : ASTGEN_SUPER_AssocArrayDType(fl) {
        ಮಗು_DTypep(dtp);  // Only for parser
        ಕೀಲಿ_ಮಗು_DTypep(ಕೀಲಿ_Dtp);  // Only for parser
        refDTypep(nullptr);
        ಕೀಲಿ_DTypep(nullptr);
        dtypep(nullptr);  // V3Width will resolve
    }
    AstAssocArrayDType(File_ಸಾಲು* fl, Ast_ಗಂಟು_DType* dtp, Ast_ಗಂಟು_DType* ಕೀಲಿ_Dtp)
        : ASTGEN_SUPER_AssocArrayDType(fl) {
        refDTypep(dtp);
        ಕೀಲಿ_DTypep(ಕೀಲಿ_Dtp);
        dtypep(dtp);
    }
    ASTGEN_MEMBERS_AstAssocArrayDType;
    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* broken() ಸ್ಥಿರಾಂಕ override {
        BROKEN_RTN(!((m_refDTypep && !ಮಗು_DTypep()) || (!m_refDTypep && ಮಗು_DTypep())));
        BROKEN_RTN(!((m_ಕೀಲಿ_DTypep && !ಮಗು_DTypep()) || (!m_ಕೀಲಿ_DTypep && ಮಗು_DTypep())));
        ಹಿಂತಿರುಗಿ nullptr;
    }
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* samep) ಸ್ಥಿರಾಂಕ override {
        ಸ್ಥಿರಾಂಕ AstAssocArrayDType* ಸ್ಥಿರಾಂಕ asamep = VN_DBG_AS(samep, AssocArrayDType);
        ಆದರೆ (!asamep->subDTypep()) ಹಿಂತಿರುಗಿ false;
        ಆದರೆ (!asamep->ಕೀಲಿ_DTypep()) ಹಿಂತಿರುಗಿ false;
        ಹಿಂತಿರುಗಿ (subDTypep() == asamep->subDTypep() && ಕೀಲಿ_DTypep() == asamep->ಕೀಲಿ_DTypep());
    }
    bool similar_DType_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* samep) ಸ್ಥಿರಾಂಕ override {
        ಸ್ಥಿರಾಂಕ AstAssocArrayDType* ಸ್ಥಿರಾಂಕ asamep = VN_DBG_AS(samep, AssocArrayDType);
        ಹಿಂತಿರುಗಿ asamep->subDTypep() && subDTypep()->similarDType(asamep->subDTypep())
               && asamep->ಕೀಲಿ_DTypep() && ಕೀಲಿ_DTypep()->similarDType(asamep->ಕೀಲಿ_DTypep());
    }
    string prettyDTypeName(bool ತುಂಬಿದ_2) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpSmall(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    Ast_ಗಂಟು_DType* get_ಮಗು_DTypep() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ ಮಗು_DTypep(); }
    Ast_ಗಂಟು_DType* getChild2DTypep() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ ಕೀಲಿ_ಮಗು_DTypep(); }
    Ast_ಗಂಟು_DType* subDTypep() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE {
        ಹಿಂತಿರುಗಿ m_refDTypep ? m_refDTypep : ಮಗು_DTypep();
    }
    Ast_ಗಂಟು_DType* subDType2p() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE {
        ಹಿಂತಿರುಗಿ m_ಕೀಲಿ_DTypep ? m_ಕೀಲಿ_DTypep : ಕೀಲಿ_ಮಗು_DTypep();
    }
    bool isAggregateType() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    ಶೂನ್ಯ refDTypep(Ast_ಗಂಟು_DType* nodep) { m_refDTypep = nodep; }
    Ast_ಗಂಟು_DType* virtRefDTypep() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ m_refDTypep; }
    ಶೂನ್ಯ virtRefDTypep(Ast_ಗಂಟು_DType* nodep) override { refDTypep(nodep); }
    Ast_ಗಂಟು_DType* virtRefDType2p() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ m_ಕೀಲಿ_DTypep; }
    ಶೂನ್ಯ virtRefDType2p(Ast_ಗಂಟು_DType* nodep) override { ಕೀಲಿ_DTypep(nodep); }
    //
    Ast_ಗಂಟು_DType* ಕೀಲಿ_DTypep() ಸ್ಥಿರಾಂಕ VL_MT_STABLE {
        ಹಿಂತಿರುಗಿ m_ಕೀಲಿ_DTypep ? m_ಕೀಲಿ_DTypep : ಕೀಲಿ_ಮಗು_DTypep();
    }
    ಶೂನ್ಯ ಕೀಲಿ_DTypep(Ast_ಗಂಟು_DType* nodep) { m_ಕೀಲಿ_DTypep = nodep; }
    // METHODS
    AstBasicDType* basicp() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE { ಹಿಂತಿರುಗಿ nullptr; }
    ಪೂರ್ಣಾಂಕ ಅಗಲ_Align_ಬೈಟುಗಳು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ subDTypep()->ಅಗಲ_Align_ಬೈಟುಗಳು(); }
    ಪೂರ್ಣಾಂಕ ಅಗಲ_ಒಟ್ಟು_ಬೈಟುಗಳು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ subDTypep()->ಅಗಲ_ಒಟ್ಟು_ಬೈಟುಗಳು(); }
    bool isCompound() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool isDynamicallySized() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
};
class AstBasicDType final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_DType {
    // Builtin atomic/vectored data type
    // @astgen op1 := rangep : Optional[AstRange] // Range of variable
    ರಚನೆ Members final {
        VBasicDTypeKwd m_keyword;  // (also in VBasicTypeKey) What keyword created basic type
        // dist-ast-dump-suppress  // Part of dumpSmall
        VNumRange m_nrange;  // (also in VBasicTypeKey) Numeric msb/lsb (if non-opaque keyword)
        bool ಕ್ರಿಯಾಕಾರಕ==(ಸ್ಥಿರಾಂಕ Members& rhs) ಸ್ಥಿರಾಂಕ {
            ಹಿಂತಿರುಗಿ rhs.m_keyword == m_keyword && rhs.m_nrange == m_nrange;
        }
    } m;
    // See also in AstNodeDType: m_width, m_widthMin, m_numeric(issigned)
ಸಾರ್ವಜನಿಕ:
    AstBasicDType(File_ಸಾಲು* fl, VBasicDTypeKwd kwd, ಸ್ಥಿರಾಂಕ VSigning& signst = VSigning::NOSIGN)
        : ASTGEN_SUPER_BasicDType(fl) {
        init(kwd, signst, 0, -1, nullptr);
    }
    AstBasicDType(File_ಸಾಲು* fl, VFlagLogicPacked, ಪೂರ್ಣಾಂಕ wantwidth)
        : ASTGEN_SUPER_BasicDType(fl) {
        init(VBasicDTypeKwd::LOGIC, VSigning::NOSIGN, wantwidth, -1, nullptr);
    }
    AstBasicDType(File_ಸಾಲು* fl, VFlagLogic2StatePacked, ಪೂರ್ಣಾಂಕ wantwidth)
        : ASTGEN_SUPER_BasicDType(fl) {
        init(VBasicDTypeKwd::LOGIC2STATE, VSigning::NOSIGN, wantwidth, -1, nullptr);
    }
    AstBasicDType(File_ಸಾಲು* fl, VFlag_ಬಿಟ್_Packed, ಪೂರ್ಣಾಂಕ wantwidth)
        : ASTGEN_SUPER_BasicDType(fl) {
        init(VBasicDTypeKwd::ಬಿಟ್, VSigning::NOSIGN, wantwidth, -1, nullptr);
    }
    AstBasicDType(File_ಸಾಲು* fl, VBasicDTypeKwd kwd, VSigning numer, ಪೂರ್ಣಾಂಕ wantwidth, ಪೂರ್ಣಾಂಕ widthmin)
        : ASTGEN_SUPER_BasicDType(fl) {
        init(kwd, numer, wantwidth, widthmin, nullptr);
    }
    AstBasicDType(File_ಸಾಲು* fl, VBasicDTypeKwd kwd, VSigning numer, VNumRange range, ಪೂರ್ಣಾಂಕ widthmin)
        : ASTGEN_SUPER_BasicDType(fl) {
        init(kwd, numer, range.elements(), widthmin, nullptr);
        m.m_nrange = range;  // as init() presumes lsb==0, but range.lsb() might not be
    }
    // See also addRange in verilog.y
ಖಾಸಗಿ:
    ಶೂನ್ಯ init(VBasicDTypeKwd kwd, VSigning numer, ಪೂರ್ಣಾಂಕ wantwidth, ಪೂರ್ಣಾಂಕ wantwidthmin,
              AstRange* rangep);

ಸಾರ್ವಜನಿಕ:
    ASTGEN_MEMBERS_AstBasicDType;
    ಶೂನ್ಯ dump(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    // width/widthMin/numeric compared elsewhere
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* samep) ಸ್ಥಿರಾಂಕ override;
    bool similar_DType_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* samep) ಸ್ಥಿರಾಂಕ override;
    string name() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE { ಹಿಂತಿರುಗಿ m.m_keyword.typeName(); }
    string prettyDTypeName(bool ತುಂಬಿದ_2) ಸ್ಥಿರಾಂಕ override;
    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* broken() ಸ್ಥಿರಾಂಕ override {
        BROKEN_RTN(dtypep() != this);
        BROKEN_RTN(v3Global.ಅಗಲ_Min_Usage() == VWidthMinUsage::VERILOG_ಅಗಲ
                   && ಅಗಲ_Min() > ಅಗಲ());
        ಹಿಂತಿರುಗಿ nullptr;
    }
    ಶೂನ್ಯ ಗಣ_Signed_State(ಸ್ಥಿರಾಂಕ VSigning& signst) {
        // Note NOSIGN does NOT change the state; this is required by the parser
        ಆದರೆ (signst == VSigning::UNSIGNED) {
            numeric(signst);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (signst == VSigning::SIGNED) {
            numeric(signst);
        }
    }
    // METHODS
    AstBasicDType* basicp() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE { ಹಿಂತಿರುಗಿ (AstBasicDType*)this; }
    // (Slow) recurses - Structure alignment 1,2,4 or 8 bytes (arrays affect this)
    ಪೂರ್ಣಾಂಕ ಅಗಲ_Align_ಬೈಟುಗಳು() ಸ್ಥಿರಾಂಕ override;
    // (Slow) recurses - Width in bytes rounding up 1,2,4,8,12,...
    ಪೂರ್ಣಾಂಕ ಅಗಲ_ಒಟ್ಟು_ಬೈಟುಗಳು() ಸ್ಥಿರಾಂಕ override;
    bool isFourstate() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ keyword().isFourstate(); }
    VBasicDTypeKwd keyword() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ {  // Avoid using - use isSomething accessors instead
        ಹಿಂತಿರುಗಿ m.m_keyword;
    }
    bool is_ಬಿಟ್_Logic() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ keyword().is_ಬಿಟ್_Logic(); }
    bool is_ಬಿಟ್() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ keyword().is_ಬಿಟ್(); }
    bool isCHandle() ಸ್ಥಿರಾಂಕ VL_MT_STABLE { ಹಿಂತಿರುಗಿ keyword().isCHandle(); }
    bool isDouble() ಸ್ಥಿರಾಂಕ VL_MT_STABLE { ಹಿಂತಿರುಗಿ keyword().isDouble(); }
    bool isEvent() ಸ್ಥಿರಾಂಕ VL_MT_STABLE { ಹಿಂತಿರುಗಿ keyword() == VBasicDTypeKwd::EVENT; }
    bool isForkSync() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ keyword() == VBasicDTypeKwd::FORK_SYNC; }
    bool isProcessRef() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ keyword() == VBasicDTypeKwd::PROCESS_REFERENCE; }
    bool isDelayScheduler() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ {
        ಹಿಂತಿರುಗಿ keyword() == VBasicDTypeKwd::DELAY_SCHEDULER;
    }
    bool isTriggerScheduler() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ {
        ಹಿಂತಿರುಗಿ keyword() == VBasicDTypeKwd::TRIGGER_SCHEDULER;
    }
    bool isDynamicTriggerScheduler() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ {
        ಹಿಂತಿರುಗಿ keyword() == VBasicDTypeKwd::DYNAMIC_TRIGGER_SCHEDULER;
    }
    bool isRandomGenerator() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ {
        ಹಿಂತಿರುಗಿ keyword() == VBasicDTypeKwd::RANDOM_GENERATOR;
    }
    bool isStdRandomGenerator() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ {
        ಹಿಂತಿರುಗಿ keyword() == VBasicDTypeKwd::RANDOM_STDGENERATOR;
    }
    bool isCovergroupInstHandle() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ {
        ಹಿಂತಿರುಗಿ keyword() == VBasicDTypeKwd::COVERGROUP_INSTHANDLE;
    }
    bool isOpaque() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ keyword().isOpaque(); }
    bool isString() ಸ್ಥಿರಾಂಕ VL_MT_STABLE { ಹಿಂತಿರುಗಿ keyword().isString(); }
    bool isZeroInit() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ keyword().isZeroInit(); }
    bool isRanged() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ rangep() || m.m_nrange.ranged(); }
    bool is_Dpi_ಬಿಟ್_Vec() ಸ್ಥಿರಾಂಕ {  // DPI uses svBitVecVal
        ಹಿಂತಿರುಗಿ keyword() == VBasicDTypeKwd::ಬಿಟ್ && isRanged();
    }
    bool isDpiLogicVec() ಸ್ಥಿರಾಂಕ {  // DPI uses svLogicVecVal
        ಹಿಂತಿರುಗಿ keyword().isFourstate() && !(keyword() == VBasicDTypeKwd::LOGIC && !isRanged());
    }
    bool isDpiCLayout() ಸ್ಥಿರಾಂಕ {
        ಹಿಂತಿರುಗಿ keyword().isDpiCLayout()
               || (keyword().isSameish(VBasicDTypeKwd::LOGIC) && !isRanged());
    }
    bool isDpiPrimitive() ಸ್ಥಿರಾಂಕ {  // DPI uses a primitive type
        ಹಿಂತಿರುಗಿ !is_Dpi_ಬಿಟ್_Vec() && !isDpiLogicVec();
    }
    // Generally the lo/hi/left/right funcs should be used instead of nrange()
    ಸ್ಥಿರಾಂಕ VNumRange& nrange() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m.m_nrange; }
    ಸಾಲೊಳಗೆ ಪೂರ್ಣಾಂಕ hi() ಸ್ಥಿರಾಂಕ;
    ಸಾಲೊಳಗೆ ಪೂರ್ಣಾಂಕ lo() ಸ್ಥಿರಾಂಕ;
    ಸಾಲೊಳಗೆ ಪೂರ್ಣಾಂಕ elements() ಸ್ಥಿರಾಂಕ;
    ಪೂರ್ಣಾಂಕ ಎಡ_2() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ ascending() ? lo() : hi(); }  // How to show a declaration
    ಪೂರ್ಣಾಂಕ ಬಲ_2() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ ascending() ? hi() : lo(); }
    ಸಾಲೊಳಗೆ bool ascending() ಸ್ಥಿರಾಂಕ;
    bool implicit() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ keyword() == VBasicDTypeKwd::LOGIC_IMPLICIT; }
    bool untyped() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ keyword() == VBasicDTypeKwd::UNTYPED; }
    VNumRange declRange() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ isRanged() ? VNumRange{ಎಡ_2(), ಬಲ_2()} : VNumRange{}; }
    ಶೂನ್ಯ cvtRangeConst();  // Convert to smaller representation
    bool isCompound() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ isString(); }
    bool isIntegralOrPacked() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ keyword().isIntNumeric(); }
};
class AstBracketArrayDType final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_DType {
    // Associative/Queue/Normal array data type, ie "[dtype_or_expr]"
    // only for early parsing then becomes another data type
    // @astgen op1 := childDTypep : Optional[AstNodeDType] // moved to refDTypep() in V3Width
    // @astgen op2 := elementsp : AstNode  // Number of elements in array
ಸಾರ್ವಜನಿಕ:
    AstBracketArrayDType(File_ಸಾಲು* fl, VFlag_ಮಗು_DType, Ast_ಗಂಟು_DType* ಮಗು_DTypep,
                         Ast_ಗಂಟು* elementsp)
        : ASTGEN_SUPER_BracketArrayDType(fl) {
        this->ಮಗು_DTypep(ಮಗು_DTypep);
        this->elementsp(elementsp);
    }
    ASTGEN_MEMBERS_AstBracketArrayDType;
    bool similar_DType_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* samep) ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ this == samep; }
    Ast_ಗಂಟು_DType* subDTypep() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE { ಹಿಂತಿರುಗಿ ಮಗು_DTypep(); }
    // METHODS
    // Will be removed in V3Width, which relies on this
    // being a child not a dtype pointed node
    bool maybe_Pointed_ಗಮ್ಯ() ಸ್ಥಿರಾಂಕ override VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ false; }
    AstBasicDType* basicp() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE { ಹಿಂತಿರುಗಿ nullptr; }
    ಪೂರ್ಣಾಂಕ ಅಗಲ_Align_ಬೈಟುಗಳು() ಸ್ಥಿರಾಂಕ override { V3ERROR_NA_RETURN(0); }
    ಪೂರ್ಣಾಂಕ ಅಗಲ_ಒಟ್ಟು_ಬೈಟುಗಳು() ಸ್ಥಿರಾಂಕ override { V3ERROR_NA_RETURN(0); }
    bool isCompound() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
};
class AstCDType final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_DType {
    // Raw "C" data type passed directly to output
    string m_name;  // Name of data type, printed when do V3EmitC
ಸಾರ್ವಜನಿಕ:
    AstCDType(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ string& name)
        : ASTGEN_SUPER_CDType(fl)
        , m_name{name} {
        dtypep(this);
    }

    ASTGEN_MEMBERS_AstCDType;
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* samep) ಸ್ಥಿರಾಂಕ override {
        ಸ್ಥಿರಾಂಕ AstCDType* ಸ್ಥಿರಾಂಕ asamep = VN_DBG_AS(samep, CDType);
        ಹಿಂತಿರುಗಿ m_name == asamep->m_name;
    }
    bool similar_DType_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* samep) ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ same_ಗಂಟು(samep); }
    string name() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE { ಹಿಂತಿರುಗಿ m_name; }
    string prettyDTypeName(bool) ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ m_name; }
    // METHODS
    AstBasicDType* basicp() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE { ಹಿಂತಿರುಗಿ nullptr; }
    ಪೂರ್ಣಾಂಕ ಅಗಲ_Align_ಬೈಟುಗಳು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ 8; }  // Assume
    ಪೂರ್ಣಾಂಕ ಅಗಲ_ಒಟ್ಟು_ಬೈಟುಗಳು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ 8; }  // Assume
    bool isCompound() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    // Return the global runtime type storing the requested number of bits.
    ಸ್ಥಿರ string type_ಗಮ್ಯ_Hold(ಪೂರ್ಣಾಂಕ ಅಗಲ) {
        ಆದರೆ (ಅಗಲ <= 8) ಹಿಂತಿರುಗಿ "::CData";
        ಆದರೆ (ಅಗಲ <= 16) ಹಿಂತಿರುಗಿ "::SData";
        ಆದರೆ (ಅಗಲ <= VL_IDATASIZE) ಹಿಂತಿರುಗಿ "::IData";
        ಆದರೆ (ಅಗಲ <= VL_QUADSIZE) ಹಿಂತಿರುಗಿ "::QData";

        ಹಿಂತಿರುಗಿ "::VlWide<" + std::ಗಮ್ಯ_string(VL_ಪದಗಳು_I(ಅಗಲ)) + ">";
    }
};
class AstClassRefDType final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_DType {
    // Reference to a class
    // @astgen op1 := paramsp: List[AstPin]
    //
    // @astgen ptr := m_classp : Optional[AstClass]  // data type pointed to, BELOW the AstTypedef
    // @astgen ptr := m_classOrPackagep : Optional[AstNodeModule]  // Package hierarchy
    bool m_raw_ಸೂಚಕ = false;  // Emit as a non-owning C++ pointer rather than VlClassRef
ಸಾರ್ವಜನಿಕ:
    AstClassRefDType(File_ಸಾಲು* fl, AstClass* classp, AstPin* paramsp)
        : ASTGEN_SUPER_ClassRefDType(fl)
        , m_classp{classp} {
        dtypep(this);
        ಕೂಡಿಸು_Paramsp(paramsp);
    }
    ASTGEN_MEMBERS_AstClassRefDType;
    // METHODS
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* samep) ಸ್ಥಿರಾಂಕ override {
        ಸ್ಥಿರಾಂಕ AstClassRefDType* ಸ್ಥಿರಾಂಕ asamep = VN_DBG_AS(samep, ClassRefDType);
        ಹಿಂತಿರುಗಿ (m_classp == asamep->m_classp && m_classOrPackagep == asamep->m_classOrPackagep
                && m_raw_ಸೂಚಕ == asamep->m_raw_ಸೂಚಕ);
    }
    bool similar_DType_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* samep) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dump(std::ostream& str = std::cout) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str = std::cout) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpSmall(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    string prettyDTypeName(bool ತುಂಬಿದ_2) ಸ್ಥಿರಾಂಕ override;
    string pretty_Name_ಸಂದೇಶ() ಸ್ಥಿರಾಂಕ override;
    string name() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE;
    AstBasicDType* basicp() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE { ಹಿಂತಿರುಗಿ nullptr; }
    ಪೂರ್ಣಾಂಕ ಅಗಲ_Align_ಬೈಟುಗಳು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ 0; }
    ಪೂರ್ಣಾಂಕ ಅಗಲ_ಒಟ್ಟು_ಬೈಟುಗಳು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ 0; }
    Ast_ಗಂಟು_DType* virtRefDTypep() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ nullptr; }
    ಶೂನ್ಯ virtRefDTypep(Ast_ಗಂಟು_DType* nodep) override {}
    Ast_ಗಂಟು_DType* subDTypep() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE { ಹಿಂತಿರುಗಿ nullptr; }
    Ast_ಗಂಟು_Module* classOrPackagep() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_classOrPackagep; }
    ಶೂನ್ಯ classOrPackagep(Ast_ಗಂಟು_Module* nodep) { m_classOrPackagep = nodep; }
    AstClass* classp() ಸ್ಥಿರಾಂಕ VL_MT_STABLE { ಹಿಂತಿರುಗಿ m_classp; }
    ಶೂನ್ಯ classp(AstClass* nodep) { m_classp = nodep; }
    bool raw_ಸೂಚಕ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_raw_ಸೂಚಕ; }
    ಶೂನ್ಯ raw_ಸೂಚಕ(bool flag) { m_raw_ಸೂಚಕ = flag; }
    ಸ್ಥಿರ ಶೂನ್ಯ selfTest();
    bool isCompound() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
};
class AstConstDType final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_DType {
    // const data type, ie "const some_dtype var_name [2:0]"
    // ConstDType are removed in V3LinkLValue and become AstVar::isConst.
    // When more generic types are supported AstConstDType will be propagated further.
    // @astgen op1 := childDTypep : Optional[AstNodeDType]
    //
    // @astgen ptr := m_refDTypep : Optional[AstNodeDType]  // Inherit from this base data type
ಸಾರ್ವಜನಿಕ:
    AstConstDType(File_ಸಾಲು* fl, VFlag_ಮಗು_DType, Ast_ಗಂಟು_DType* dtp)
        : ASTGEN_SUPER_ConstDType(fl) {
        ಮಗು_DTypep(dtp);  // Only for parser
        refDTypep(nullptr);  // V3Width will resolve
        dtypep(nullptr);  // V3Width will resolve
        ಅಗಲ_From_Sub(subDTypep());
    }
    ASTGEN_MEMBERS_AstConstDType;
    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* broken() ಸ್ಥಿರಾಂಕ override {
        BROKEN_RTN(!((m_refDTypep && !ಮಗು_DTypep()) || (!m_refDTypep && ಮಗು_DTypep())));
        ಹಿಂತಿರುಗಿ nullptr;
    }
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* samep) ಸ್ಥಿರಾಂಕ override {
        ಸ್ಥಿರಾಂಕ AstConstDType* ಸ್ಥಿರಾಂಕ sp = VN_DBG_AS(samep, ConstDType);
        ಹಿಂತಿರುಗಿ (m_refDTypep == sp->m_refDTypep);
    }
    bool similar_DType_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* samep) ಸ್ಥಿರಾಂಕ override {
        ಹಿಂತಿರುಗಿ skipRefp()->similarDType(samep->skipRefp());
    }
    Ast_ಗಂಟು_DType* get_ಮಗು_DTypep() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ ಮಗು_DTypep(); }
    Ast_ಗಂಟು_DType* subDTypep() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE {
        ಹಿಂತಿರುಗಿ m_refDTypep ? m_refDTypep : ಮಗು_DTypep();
    }
    ಶೂನ್ಯ refDTypep(Ast_ಗಂಟು_DType* nodep) { m_refDTypep = nodep; }
    Ast_ಗಂಟು_DType* virtRefDTypep() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ m_refDTypep; }
    ಶೂನ್ಯ virtRefDTypep(Ast_ಗಂಟು_DType* nodep) override { refDTypep(nodep); }
    // METHODS
    AstBasicDType* basicp() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE { ಹಿಂತಿರುಗಿ subDTypep()->basicp(); }
    ಪೂರ್ಣಾಂಕ ಅಗಲ_Align_ಬೈಟುಗಳು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ subDTypep()->ಅಗಲ_Align_ಬೈಟುಗಳು(); }
    ಪೂರ್ಣಾಂಕ ಅಗಲ_ಒಟ್ಟು_ಬೈಟುಗಳು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ subDTypep()->ಅಗಲ_ಒಟ್ಟು_ಬೈಟುಗಳು(); }
    bool isCompound() ಸ್ಥಿರಾಂಕ override {
        v3fatalSrc("call isCompound on subdata type, not reference");
        ಹಿಂತಿರುಗಿ false;
    }
};
class AstConstraintRefDType final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_DType {
    // For e.g. a reference to constraint for constraint_mode
ಸಾರ್ವಜನಿಕ:
    explicit AstConstraintRefDType(File_ಸಾಲು* fl)
        : ASTGEN_SUPER_ConstraintRefDType(fl) {
        dtypep(this);
    }
    ASTGEN_MEMBERS_AstConstraintRefDType;
    bool hasDType() ಸ್ಥಿರಾಂಕ override VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ true; }
    bool maybe_Pointed_ಗಮ್ಯ() ಸ್ಥಿರಾಂಕ override VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ true; }
    bool undead() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    Ast_ಗಂಟು_DType* subDTypep() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE { ಹಿಂತಿರುಗಿ nullptr; }
    Ast_ಗಂಟು_DType* virtRefDTypep() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ nullptr; }
    ಶೂನ್ಯ virtRefDTypep(Ast_ಗಂಟು_DType* nodep) override {}
    bool similar_DType_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* samep) ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ this == samep; }
    AstBasicDType* basicp() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE { ಹಿಂತಿರುಗಿ nullptr; }
    ಪೂರ್ಣಾಂಕ ಅಗಲ_Align_ಬೈಟುಗಳು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ 1; }
    ಪೂರ್ಣಾಂಕ ಅಗಲ_ಒಟ್ಟು_ಬೈಟುಗಳು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ 1; }
    bool isCompound() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
};
class AstCoverCrossDType final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_DType {
    // Borrowed pointer to VlCoverCrossT<...> or construction-time VlCoverCrossDyn.
    ಸ್ಥಿರಾಂಕ uint32_t m_dimensions;
    ಸ್ಥಿರಾಂಕ uint32_t m_tuples;  // Fixed capacity number of cross tuples (VlCoverCrossT's Tuples)
    ಸ್ಥಿರಾಂಕ uint32_t m_bins;  // Fixed capacity number of explicit bins (VlCoverCrossT's Bins)
    ಸ್ಥಿರಾಂಕ uint32_t m_autoBins;  // Fixed capacity number of auto bins (VlCoverCrossT's AutoBins)
    ಸ್ಥಿರಾಂಕ uint64_t m_bin_ಪದಗಳು;  // Fixed capacity selection words (VlCoverCrossT's BinWords)
    ಸ್ಥಿರಾಂಕ bool m_dynamic;  // Bin layout is determined at covergroup construction

ಸಾರ್ವಜನಿಕ:
    AstCoverCrossDType(File_ಸಾಲು* fl, uint32_t dimensions, uint32_t tuples, uint32_t bins,
                       uint32_t autoBins, uint64_t bin_ಪದಗಳು, bool dynamic = false)
        : ASTGEN_SUPER_CoverCrossDType(fl)
        , m_dimensions{dimensions}
        , m_tuples{tuples}
        , m_bins{bins}
        , m_autoBins{autoBins}
        , m_bin_ಪದಗಳು{bin_ಪದಗಳು}
        , m_dynamic{dynamic} {
        dtypep(this);
    }
    ASTGEN_MEMBERS_AstCoverCrossDType;
    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* broken() ಸ್ಥಿರಾಂಕ override {
        BROKEN_RTN(m_dimensions == 0);
        ಹಿಂತಿರುಗಿ nullptr;
    }
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* samep) ಸ್ಥಿರಾಂಕ override {  // LCOV_EXCL_START
        ಸ್ಥಿರಾಂಕ AstCoverCrossDType* ಸ್ಥಿರಾಂಕ sp = VN_DBG_AS(samep, CoverCrossDType);
        ಹಿಂತಿರುಗಿ dimensions() == sp->dimensions() && tuples() == sp->tuples() && bins() == sp->bins()
               && autoBins() == sp->autoBins() && bin_ಪದಗಳು() == sp->bin_ಪದಗಳು()
               && isDynamic() == sp->isDynamic();
    }  // LCOV_EXCL_STOP
    bool similar_DType_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* samep) ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ this == samep; }
    ಶೂನ್ಯ dump(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpSmall(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    uint32_t dimensions() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_dimensions; }
    uint32_t tuples() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_tuples; }
    uint32_t bins() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_bins; }
    uint32_t autoBins() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_autoBins; }
    uint64_t bin_ಪದಗಳು() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_bin_ಪದಗಳು; }
    bool isDynamic() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_dynamic; }
    string cpp_Template_ವಾದಗಳು() ಸ್ಥಿರಾಂಕ;
    AstBasicDType* basicp() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE { ಹಿಂತಿರುಗಿ nullptr; }
    ಪೂರ್ಣಾಂಕ ಅಗಲ_Align_ಬೈಟುಗಳು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ ಗಾತ್ರ(ಶೂನ್ಯ*); }
    ಪೂರ್ಣಾಂಕ ಅಗಲ_ಒಟ್ಟು_ಬೈಟುಗಳು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ ಗಾತ್ರ(ಶೂನ್ಯ*); }
    bool isCompound() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
};
class AstCoverpointDType final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_DType {
    // Borrowed pointer to a covergroup coverpoint runtime, 'VlCoverpointT<hitBound>*'.
    // Follows pattern of AstQueueDType in capturing the compile-time max bin overlap
    // template argument.
    uint32_t m_ದೊರಕಿತು_Bound;  // VlCoverpointT<> template argument; hit list size, >= 1
ಸಾರ್ವಜನಿಕ:
    AstCoverpointDType(File_ಸಾಲು* fl, uint32_t ದೊರಕಿತು_Bound)
        : ASTGEN_SUPER_CoverpointDType(fl)
        , m_ದೊರಕಿತು_Bound{ದೊರಕಿತು_Bound} {
        dtypep(this);
    }
    ASTGEN_MEMBERS_AstCoverpointDType;
    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* broken() ಸ್ಥಿರಾಂಕ override {
        BROKEN_RTN(m_ದೊರಕಿತು_Bound < 1);
        ಹಿಂತಿರುಗಿ nullptr;
    }
    // V3Covergroup interns these one-per-hitBound into the type table, so identity is
    // equality; there is never a second node with the same bound to compare against.
    bool similar_DType_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* samep) ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ this == samep; }
    ಶೂನ್ಯ dumpSmall(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    // ACCESSORS
    uint32_t ದೊರಕಿತು_Bound() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ದೊರಕಿತು_Bound; }
    // METHODS
    AstBasicDType* basicp() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE { ಹಿಂತಿರುಗಿ nullptr; }
    ಪೂರ್ಣಾಂಕ ಅಗಲ_Align_ಬೈಟುಗಳು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ ಗಾತ್ರ(ಶೂನ್ಯ*); }
    ಪೂರ್ಣಾಂಕ ಅಗಲ_ಒಟ್ಟು_ಬೈಟುಗಳು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ ಗಾತ್ರ(ಶೂನ್ಯ*); }
    bool isCompound() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
};
class AstDefImplicitDType final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_DType {
    // For parsing enum/struct/unions that are declared with a variable rather than typedef
    // This allows "var enum {...} a,b" to share the enum definition for both variables
    // After link, these become typedefs
    // @astgen op1 := childDTypep : Optional[AstNodeDType]
    string m_name;  // Data type name
    ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ m_unique_ಅಂಕೆ;  // Unique ID distinguishing this dtype instance, for hashing/naming

ಸಾರ್ವಜನಿಕ:
    AstDefImplicitDType(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ string& name, VFlag_ಮಗು_DType, Ast_ಗಂಟು_DType* dtp)
        : ASTGEN_SUPER_DefImplicitDType(fl)
        , m_name{name}
        , m_unique_ಅಂಕೆ{unique_ಅಂಕೆ_Inc()} {
        ಮಗು_DTypep(dtp);  // Only for parser
        dtypep(nullptr);  // V3Width will resolve
    }
    AstDefImplicitDType(ಸ್ಥಿರಾಂಕ AstDefImplicitDType& other)
        : Ast_ಗಂಟು_DType(other)
        , m_name{other.m_name}
        , m_unique_ಅಂಕೆ{unique_ಅಂಕೆ_Inc()} {}
    ASTGEN_MEMBERS_AstDefImplicitDType;
    ಪೂರ್ಣಾಂಕ unique_ಅಂಕೆ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_unique_ಅಂಕೆ; }
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* samep) ಸ್ಥಿರಾಂಕ override {
        ಸ್ಥಿರಾಂಕ AstDefImplicitDType* ಸ್ಥಿರಾಂಕ sp = VN_DBG_AS(samep, DefImplicitDType);
        ಹಿಂತಿರುಗಿ unique_ಅಂಕೆ() == sp->unique_ಅಂಕೆ();
    }
    bool similar_DType_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* samep) ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ same_ಗಂಟು(samep); }
    Ast_ಗಂಟು_DType* get_ಮಗು_DTypep() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ ಮಗು_DTypep(); }
    Ast_ಗಂಟು_DType* subDTypep() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE {
        ಹಿಂತಿರುಗಿ dtypep() ? dtypep() : ಮಗು_DTypep();
    }
    // METHODS
    // op1 = Range of variable
    Ast_ಗಂಟು_DType* dtypeSkipRefp() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ dtypep()->skipRefp(); }
    AstBasicDType* basicp() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE { ಹಿಂತಿರುಗಿ subDTypep()->basicp(); }
    ಪೂರ್ಣಾಂಕ ಅಗಲ_Align_ಬೈಟುಗಳು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ dtypep()->ಅಗಲ_Align_ಬೈಟುಗಳು(); }
    ಪೂರ್ಣಾಂಕ ಅಗಲ_ಒಟ್ಟು_ಬೈಟುಗಳು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ dtypep()->ಅಗಲ_ಒಟ್ಟು_ಬೈಟುಗಳು(); }
    string name() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE { ಹಿಂತಿರುಗಿ m_name; }
    ಶೂನ್ಯ name(ಸ್ಥಿರಾಂಕ string& flag) override { m_name = flag; }
    bool isCompound() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
};
class AstDynArrayDType final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_DType {
    // Dynamic array data type, ie "[]"
    // @astgen op1 := childDTypep : Optional[AstNodeDType] // moved to refDTypep() in V3Width
    //
    // @astgen ptr := m_refDTypep : Optional[AstNodeDType]  // Elements of this type (post-width)
ಸಾರ್ವಜನಿಕ:
    AstDynArrayDType(File_ಸಾಲು* fl, VFlag_ಮಗು_DType, Ast_ಗಂಟು_DType* dtp)
        : ASTGEN_SUPER_DynArrayDType(fl) {
        ಮಗು_DTypep(dtp);  // Only for parser
        refDTypep(nullptr);
        dtypep(nullptr);  // V3Width will resolve
    }
    AstDynArrayDType(File_ಸಾಲು* fl, Ast_ಗಂಟು_DType* dtp)
        : ASTGEN_SUPER_DynArrayDType(fl) {
        refDTypep(dtp);
        dtypep(nullptr);  // V3Width will resolve
    }
    ASTGEN_MEMBERS_AstDynArrayDType;
    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* broken() ಸ್ಥಿರಾಂಕ override {
        BROKEN_RTN(!((m_refDTypep && !ಮಗು_DTypep()) || (!m_refDTypep && ಮಗು_DTypep())));
        ಹಿಂತಿರುಗಿ nullptr;
    }
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* samep) ಸ್ಥಿರಾಂಕ override {
        ಸ್ಥಿರಾಂಕ AstDynArrayDType* ಸ್ಥಿರಾಂಕ asamep = VN_DBG_AS(samep, DynArrayDType);
        ಆದರೆ (!asamep->subDTypep()) ಹಿಂತಿರುಗಿ false;
        ಹಿಂತಿರುಗಿ subDTypep() == asamep->subDTypep();
    }
    bool similar_DType_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* samep) ಸ್ಥಿರಾಂಕ override {
        ಸ್ಥಿರಾಂಕ AstDynArrayDType* ಸ್ಥಿರಾಂಕ asamep = VN_DBG_AS(samep, DynArrayDType);
        ಹಿಂತಿರುಗಿ asamep->subDTypep() && subDTypep()->similarDType(asamep->subDTypep());
    }
    string prettyDTypeName(bool ತುಂಬಿದ_2) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpSmall(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    Ast_ಗಂಟು_DType* get_ಮಗು_DTypep() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ ಮಗು_DTypep(); }
    Ast_ಗಂಟು_DType* subDTypep() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE {
        ಹಿಂತಿರುಗಿ m_refDTypep ? m_refDTypep : ಮಗು_DTypep();
    }
    bool isAggregateType() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    ಶೂನ್ಯ refDTypep(Ast_ಗಂಟು_DType* nodep) { m_refDTypep = nodep; }
    Ast_ಗಂಟು_DType* virtRefDTypep() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ m_refDTypep; }
    ಶೂನ್ಯ virtRefDTypep(Ast_ಗಂಟು_DType* nodep) override { refDTypep(nodep); }
    // METHODS
    AstBasicDType* basicp() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE { ಹಿಂತಿರುಗಿ nullptr; }
    ಪೂರ್ಣಾಂಕ ಅಗಲ_Align_ಬೈಟುಗಳು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ subDTypep()->ಅಗಲ_Align_ಬೈಟುಗಳು(); }
    ಪೂರ್ಣಾಂಕ ಅಗಲ_ಒಟ್ಟು_ಬೈಟುಗಳು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ subDTypep()->ಅಗಲ_ಒಟ್ಟು_ಬೈಟುಗಳು(); }
    bool isCompound() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool isDynamicallySized() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
};
class Ast_ಖಾಲಿಗಳು_ಸರತಿ_DType final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_DType {
    // For EmptyQueue
ಸಾರ್ವಜನಿಕ:
    explicit Ast_ಖಾಲಿಗಳು_ಸರತಿ_DType(File_ಸಾಲು* fl)
        : ASTGEN_SUPER_ಖಾಲಿಗಳು_ಸರತಿ_DType(fl) {
        dtypep(this);
    }
    ASTGEN_MEMBERS_Ast_ಖಾಲಿಗಳು_ಸರತಿ_DType;
    ಶೂನ್ಯ dumpSmall(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    bool hasDType() ಸ್ಥಿರಾಂಕ override VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ true; }
    bool maybe_Pointed_ಗಮ್ಯ() ಸ್ಥಿರಾಂಕ override VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ true; }
    bool undead() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    Ast_ಗಂಟು_DType* subDTypep() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE { ಹಿಂತಿರುಗಿ nullptr; }
    Ast_ಗಂಟು_DType* virtRefDTypep() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ nullptr; }
    ಶೂನ್ಯ virtRefDTypep(Ast_ಗಂಟು_DType* nodep) override {}
    bool similar_DType_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* samep) ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ this == samep; }
    AstBasicDType* basicp() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE { ಹಿಂತಿರುಗಿ nullptr; }
    ಪೂರ್ಣಾಂಕ ಅಗಲ_Align_ಬೈಟುಗಳು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ 1; }
    ಪೂರ್ಣಾಂಕ ಅಗಲ_ಒಟ್ಟು_ಬೈಟುಗಳು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ 1; }
    bool isCompound() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
};
class AstEnumDType final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_DType {
    // Parents: TYPEDEF/MODULE
    // @astgen op1 := childDTypep : Optional[AstNodeDType]
    // @astgen op2 := itemsp : List[AstEnumItem]
    //
    // @astgen ptr := m_refDTypep : Optional[AstNodeDType]  // Elements of this type (post-width)
    string m_name;  // Name from upper typedef, if any
    string m_typedefName;  // Typedef's dtypeName() once moved off the typedef declaring it
    ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ m_unique_ಅಂಕೆ;  // Unique ID distinguishing this dtype instance, for hashing/naming

ಸಾರ್ವಜನಿಕ:
    AstEnumDType(File_ಸಾಲು* fl, VFlag_ಮಗು_DType, Ast_ಗಂಟು_DType* dtp, Ast_Enum_ಅಂಶ* itemsp)
        : ASTGEN_SUPER_EnumDType(fl)
        , m_unique_ಅಂಕೆ{unique_ಅಂಕೆ_Inc()} {
        ಮಗು_DTypep(dtp);  // Only for parser
        refDTypep(nullptr);
        ಕೂಡಿಸು_Itemsp(itemsp);
        dtypep(nullptr);  // V3Width will resolve
        ಅಗಲ_From_Sub(subDTypep());
    }
    AstEnumDType(ಸ್ಥಿರಾಂಕ AstEnumDType& other)
        : Ast_ಗಂಟು_DType{other}
        , m_name{other.m_name}
        , m_typedefName{other.m_typedefName}
        , m_unique_ಅಂಕೆ{unique_ಅಂಕೆ_Inc()} {}
    ASTGEN_MEMBERS_AstEnumDType;

    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* broken() ಸ್ಥಿರಾಂಕ override;
    ಪೂರ್ಣಾಂಕ unique_ಅಂಕೆ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_unique_ಅಂಕೆ; }
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* samep) ಸ್ಥಿರಾಂಕ override {
        ಸ್ಥಿರಾಂಕ AstEnumDType* ಸ್ಥಿರಾಂಕ sp = VN_DBG_AS(samep, EnumDType);
        ಹಿಂತಿರುಗಿ unique_ಅಂಕೆ() == sp->unique_ಅಂಕೆ();
    }
    bool similar_DType_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* samep) ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ same_ಗಂಟು(samep); }
    Ast_ಗಂಟು_DType* get_ಮಗು_DTypep() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ ಮಗು_DTypep(); }
    Ast_ಗಂಟು_DType* subDTypep() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE {
        ಹಿಂತಿರುಗಿ m_refDTypep ? m_refDTypep : ಮಗು_DTypep();
    }
    ಶೂನ್ಯ refDTypep(Ast_ಗಂಟು_DType* nodep) { m_refDTypep = nodep; }
    Ast_ಗಂಟು_DType* virtRefDTypep() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ m_refDTypep; }
    ಶೂನ್ಯ virtRefDTypep(Ast_ಗಂಟು_DType* nodep) override { refDTypep(nodep); }
    string name() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE { ಹಿಂತಿರುಗಿ m_name; }
    ಶೂನ್ಯ name(ಸ್ಥಿರಾಂಕ string& flag) override { m_name = flag; }
    // Keep the name the typedef declaring it gives, as it moves off it to the type table
    ಶೂನ್ಯ typedefName(ಸ್ಥಿರಾಂಕ string& name) { m_typedefName = name; }
    ಶೂನ್ಯ dump(std::ostream& str = std::cout) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str = std::cout) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpSmall(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    string prettyDTypeName(bool ತುಂಬಿದ_2) ಸ್ಥಿರಾಂಕ override;
    // METHODS
    AstBasicDType* basicp() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE { ಹಿಂತಿರುಗಿ subDTypep()->basicp(); }
    ಪೂರ್ಣಾಂಕ ಅಗಲ_Align_ಬೈಟುಗಳು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ subDTypep()->ಅಗಲ_Align_ಬೈಟುಗಳು(); }
    ಪೂರ್ಣಾಂಕ ಅಗಲ_ಒಟ್ಟು_ಬೈಟುಗಳು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ subDTypep()->ಅಗಲ_ಒಟ್ಟು_ಬೈಟುಗಳು(); }
    ಗಾತ್ರ_t ಅಂಶ_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ {
        ಗಾತ್ರ_t ಎಣಿಕೆ_2 = 0;
        ಗಾಗಿ (Ast_ಗಂಟು* itemp = itemsp(); itemp; itemp = itemp->nextp()) ++ಎಣಿಕೆ_2;
        ಹಿಂತಿರುಗಿ ಎಣಿಕೆ_2;
    }
    bool isCompound() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
};

class AstIfaceGenericDType final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_DType {
    // Generic interface that will be replaced with AstIfaceRefDType
    File_ಸಾಲು* m_modportFileline;  // Where modport token was
    // dist-ast-dump-suppress  // Part of name()
    string m_modportName;  // "" = no modport
ಸಾರ್ವಜನಿಕ:
    explicit AstIfaceGenericDType(File_ಸಾಲು* fl)
        : ASTGEN_SUPER_IfaceGenericDType(fl) {
        dtypep(this);
    }
    AstIfaceGenericDType(File_ಸಾಲು* fl, File_ಸಾಲು* modportFl, ಸ್ಥಿರಾಂಕ string& modport)
        : ASTGEN_SUPER_IfaceGenericDType(fl)
        , m_modportFileline{modportFl}
        , m_modportName{modport} {
        dtypep(this);
    }
    ASTGEN_MEMBERS_AstIfaceGenericDType;
    ಶೂನ್ಯ dumpSmall(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    bool hasDType() ಸ್ಥಿರಾಂಕ override VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ true; }
    bool maybe_Pointed_ಗಮ್ಯ() ಸ್ಥಿರಾಂಕ override VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ true; }
    bool undead() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    Ast_ಗಂಟು_DType* subDTypep() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE { ಹಿಂತಿರುಗಿ nullptr; }
    Ast_ಗಂಟು_DType* virtRefDTypep() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ nullptr; }
    ಶೂನ್ಯ virtRefDTypep(Ast_ಗಂಟು_DType* nodep) override {}
    bool similar_DType_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* samep) ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ this == samep; }
    AstBasicDType* basicp() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE { ಹಿಂತಿರುಗಿ nullptr; }
    ಪೂರ್ಣಾಂಕ ಅಗಲ_Align_ಬೈಟುಗಳು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ 1; }
    ಪೂರ್ಣಾಂಕ ಅಗಲ_ಒಟ್ಟು_ಬೈಟುಗಳು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ 1; }
    string modportName() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_modportName; }
    bool isModport() { ಹಿಂತಿರುಗಿ !m_modportName.ಖಾಲಿಗಳು(); }
    bool isCompound() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    File_ಸಾಲು* modportFileline() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_modportFileline; }
    string name() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ m_modportName; }
};

class AstIfaceRefDType final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_DType {
    // Reference to an interface, either for a port, or inside parent cell
    // @astgen op1 := paramsp : List[AstPin]
    //
    // @astgen ptr := m_ifacep : Optional[AstIface]  // Interface; cellp() should override
    // @astgen ptr := m_cellp : Optional[AstCell]  // When exact parent cell known; not a guess
    // @astgen ptr := m_modportp : Optional[AstModport]  // nullptr = unlinked or no modport
    File_ಸಾಲು* m_modportFileline;  // Where modport token was
    string m_cellName;  // "" = no cell, such as when connects to 'input' iface
    string m_ifaceName;  // Interface name
    string m_modportName;  // "" = no modport
    bool m_virtual = false;  // True if virtual interface
ಸಾರ್ವಜನಿಕ:
    AstIfaceRefDType(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ string& cellName, ಸ್ಥಿರಾಂಕ string& ifaceName)
        : ASTGEN_SUPER_IfaceRefDType(fl)
        , m_modportFileline{nullptr}
        , m_cellName{cellName}
        , m_ifaceName{ifaceName}
        , m_modportName{""} {}
    AstIfaceRefDType(File_ಸಾಲು* fl, File_ಸಾಲು* modportFl, ಸ್ಥಿರಾಂಕ string& cellName,
                     ಸ್ಥಿರಾಂಕ string& ifaceName, ಸ್ಥಿರಾಂಕ string& modport)
        : ASTGEN_SUPER_IfaceRefDType(fl)
        , m_modportFileline{modportFl}
        , m_cellName{cellName}
        , m_ifaceName{ifaceName}
        , m_modportName{modport} {}
    AstIfaceRefDType(File_ಸಾಲು* fl, File_ಸಾಲು* modportFl, ಸ್ಥಿರಾಂಕ string& cellName,
                     ಸ್ಥಿರಾಂಕ string& ifaceName, ಸ್ಥಿರಾಂಕ string& modport, AstPin* paramsp)
        : ASTGEN_SUPER_IfaceRefDType(fl)
        , m_modportFileline{modportFl}
        , m_cellName{cellName}
        , m_ifaceName{ifaceName}
        , m_modportName{modport} {
        ಕೂಡಿಸು_Paramsp(paramsp);
    }
    ASTGEN_MEMBERS_AstIfaceRefDType;

    // METHODS
    ಶೂನ್ಯ dump(std::ostream& str = std::cout) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str = std::cout) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpSmall(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    string prettyDTypeName(bool ತುಂಬಿದ_2) ಸ್ಥಿರಾಂಕ override;
    AstBasicDType* basicp() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE { ಹಿಂತಿರುಗಿ nullptr; }
    bool similar_DType_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* samep) ಸ್ಥಿರಾಂಕ override {
        // Each occurrence of a virtual interface type parses to its own node,
        // so pointer identity is insufficient; compare the referenced
        // interface instead, same equivalence as AstNode::computeCastable uses.
        ಸ್ಥಿರಾಂಕ AstIfaceRefDType* ಸ್ಥಿರಾಂಕ asamep = VN_DBG_AS(samep, IfaceRefDType);
        ಹಿಂತಿರುಗಿ iface_ಮೂಲಕ_Cellp() && iface_ಮೂಲಕ_Cellp() == asamep->iface_ಮೂಲಕ_Cellp()
               && modportName() == asamep->modportName() && isVirtual() == asamep->isVirtual();
    }
    ಪೂರ್ಣಾಂಕ ಅಗಲ_Align_ಬೈಟುಗಳು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ 0; }
    ಪೂರ್ಣಾಂಕ ಅಗಲ_ಒಟ್ಟು_ಬೈಟುಗಳು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ 0; }
    bool isVirtual() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_virtual; }
    ಶೂನ್ಯ isVirtual(bool flag) {
        m_virtual = flag;
        ಆದರೆ (flag) v3Global.ಗಣ_Has_Virt_Ifaces();
    }
    File_ಸಾಲು* modportFileline() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_modportFileline; }
    ಶೂನ್ಯ modportFileline(File_ಸಾಲು* ಸ್ಥಿರಾಂಕ modportFileline) { m_modportFileline = modportFileline; }
    string cellName() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_cellName; }
    ಶೂನ್ಯ cellName(ಸ್ಥಿರಾಂಕ string& name) { m_cellName = name; }
    string ifaceName() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ifaceName; }
    string ifaceNameQ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ "'" + prettyName(ifaceName()) + "'"; }
    ಶೂನ್ಯ ifaceName(ಸ್ಥಿರಾಂಕ string& name) { m_ifaceName = name; }
    ಶೂನ್ಯ modportName(ಸ್ಥಿರಾಂಕ string& modportName) { m_modportName = modportName; }
    string modportName() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_modportName; }
    AstIface* iface_ಮೂಲಕ_Cellp() ಸ್ಥಿರಾಂಕ;  // Use cellp or ifacep
    AstIface* ifacep() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ifacep; }
    ಶೂನ್ಯ ifacep(AstIface* nodep) { m_ifacep = nodep; }
    AstCell* cellp() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_cellp; }
    ಶೂನ್ಯ cellp(AstCell* nodep) { m_cellp = nodep; }
    AstModport* modportp() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_modportp; }
    ಶೂನ್ಯ modportp(AstModport* modportp) { m_modportp = modportp; }
    bool isModport() { ಹಿಂತಿರುಗಿ !m_modportName.ಖಾಲಿಗಳು(); }
    bool isCompound() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }  // But not relevant
};
class AstMemberDType final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_DType {
    // A member of a struct/union
    // PARENT: AstNodeUOrStructDType
    // @astgen op1 := childDTypep : Optional[AstNodeDType]
    // @astgen op3 := valuep : Optional[AstNode]
    //
    // @astgen ptr := m_refDTypep : Optional[AstNodeDType]  // Elements of this type (post-width)
    string m_name;  // Name of variable
    string m_ಟ್ಯಾಗ್;  // Holds the string of the verilator tag -- used in JSON output.
    ಪೂರ್ಣಾಂಕ m_lsb = -1;  // Within this level's packed struct, the LSB of the first bit of the member
    bool m_constrainedRand = false;  // Member has a constraint expression
    VRandAttr m_rand;  // Randomizability of this member (rand, randc, etc)
ಸಾರ್ವಜನಿಕ:
    AstMemberDType(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ string& name, VFlag_ಮಗು_DType, Ast_ಗಂಟು_DType* dtp,
                   Ast_ಗಂಟು* valuep)
        : ASTGEN_SUPER_MemberDType(fl)
        , m_name{name} {
        ಮಗು_DTypep(dtp);  // Only for parser
        this->valuep(valuep);
        dtypep(nullptr);  // V3Width will resolve
        refDTypep(nullptr);
    }
    AstMemberDType(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ string& name, Ast_ಗಂಟು_DType* dtp)
        : ASTGEN_SUPER_MemberDType(fl)
        , m_name{name} {
        UASSERT(dtp, "AstMember created with no dtype");
        refDTypep(dtp);
        dtypep(this);
        ಅಗಲ_From_Sub(subDTypep());
    }
    ASTGEN_MEMBERS_AstMemberDType;
    ಶೂನ್ಯ dump(std::ostream& str = std::cout) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str = std::cout) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpSmall(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    string prettyDTypeName(bool ತುಂಬಿದ_2) ಸ್ಥಿರಾಂಕ override;
    string name() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE { ಹಿಂತಿರುಗಿ m_name; }  // * = Var name
    bool hasDType() ಸ್ಥಿರಾಂಕ override VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ true; }
    bool maybe_Pointed_ಗಮ್ಯ() ಸ್ಥಿರಾಂಕ override VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ true; }
    Ast_ಗಂಟು_DType* get_ಮಗು_DTypep() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ ಮಗು_DTypep(); }
    Ast_ಗಂಟು_UOr_Struct_DType* get_ಮಗು_Structp();
    Ast_ಗಂಟು_DType* subDTypep() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE {
        ಹಿಂತಿರುಗಿ m_refDTypep ? m_refDTypep : ಮಗು_DTypep();
    }
    ಶೂನ್ಯ refDTypep(Ast_ಗಂಟು_DType* nodep) { m_refDTypep = nodep; }
    Ast_ಗಂಟು_DType* virtRefDTypep() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ m_refDTypep; }
    ಶೂನ್ಯ virtRefDTypep(Ast_ಗಂಟು_DType* nodep) override { refDTypep(nodep); }
    bool similar_DType_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* samep) ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ this == samep; }
    //
    // (Slow) recurse down to find basic data type (Note don't need virtual -
    // AstVar isn't a NodeDType)
    AstBasicDType* basicp() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE { ಹಿಂತಿರುಗಿ subDTypep()->basicp(); }
    // op1 = Range of variable (Note don't need virtual - AstVar isn't a NodeDType)
    Ast_ಗಂಟು_DType* dtypeSkipRefp() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ subDTypep()->skipRefp(); }
    // (Slow) recurses - Structure alignment 1,2,4 or 8 bytes (arrays affect this)
    ಪೂರ್ಣಾಂಕ ಅಗಲ_Align_ಬೈಟುಗಳು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ subDTypep()->ಅಗಲ_Align_ಬೈಟುಗಳು(); }
    // (Slow) recurses - Width in bytes rounding up 1,2,4,8,12,...
    ಪೂರ್ಣಾಂಕ ಅಗಲ_ಒಟ್ಟು_ಬೈಟುಗಳು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ subDTypep()->ಅಗಲ_ಒಟ್ಟು_ಬೈಟುಗಳು(); }
    // METHODS
    ಶೂನ್ಯ name(ಸ್ಥಿರಾಂಕ string& name) override { m_name = name; }
    string ಟ್ಯಾಗ್() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ m_ಟ್ಯಾಗ್; }
    ಶೂನ್ಯ ಟ್ಯಾಗ್(ಸ್ಥಿರಾಂಕ string& ಪಠ್ಯ) override { m_ಟ್ಯಾಗ್ = ಪಠ್ಯ; }
    ಪೂರ್ಣಾಂಕ lsb() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_lsb; }
    ಶೂನ್ಯ lsb(ಪೂರ್ಣಾಂಕ lsb) { m_lsb = lsb; }
    bool isCompound() ಸ್ಥಿರಾಂಕ override {
        v3fatalSrc("call isCompound on subdata type, not reference");
        ಹಿಂತಿರುಗಿ false;
    }
    bool isConstrainedRand() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_constrainedRand; }
    ಶೂನ್ಯ markConstrainedRand(bool flag) { m_constrainedRand = flag; }
    VRandAttr rand() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_rand; }
    ಶೂನ್ಯ rand(ಸ್ಥಿರಾಂಕ VRandAttr flag) { m_rand = flag; }
};
class Ast_NBACommit_ಸರತಿ_DType final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_DType {
    // @astgen ptr := m_subDTypep : AstNodeDType  // Type of the corresponding variable
    ಸ್ಥಿರಾಂಕ bool m_partial;  // Partial element update required

ಸಾರ್ವಜನಿಕ:
    Ast_NBACommit_ಸರತಿ_DType(File_ಸಾಲು* fl, Ast_ಗಂಟು_DType* subDTypep, bool partial)
        : ASTGEN_SUPER_NBACommit_ಸರತಿ_DType(fl)
        , m_partial{partial}
        , m_subDTypep{subDTypep} {
        dtypep(this);
    }
    ASTGEN_MEMBERS_Ast_NBACommit_ಸರತಿ_DType;
    ಶೂನ್ಯ dump(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    Ast_ಗಂಟು_DType* subDTypep() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE { ಹಿಂತಿರುಗಿ m_subDTypep; }
    bool partial() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_partial; }
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* samep) ಸ್ಥಿರಾಂಕ override {
        ಸ್ಥಿರಾಂಕ Ast_NBACommit_ಸರತಿ_DType* ಸ್ಥಿರಾಂಕ asamep = VN_DBG_AS(samep, NBACommit_ಸರತಿ_DType);
        ಹಿಂತಿರುಗಿ m_partial == asamep->m_partial;
    }
    bool similar_DType_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* samep) ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ this == samep; }
    AstBasicDType* basicp() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE { ಹಿಂತಿರುಗಿ nullptr; }
    ಪೂರ್ಣಾಂಕ ಅಗಲ_Align_ಬೈಟುಗಳು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ 1; }
    ಪೂರ್ಣಾಂಕ ಅಗಲ_ಒಟ್ಟು_ಬೈಟುಗಳು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ 24; }
    bool isCompound() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
};
class AstParamTypeDType final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_DType {
    // Parents: MODULE
    // A parameter type statement; much like a var or typedef
    // @astgen op1 := childDTypep : Optional[AstNodeDType]
    ಸ್ಥಿರಾಂಕ VVarType m_varType;  // Type of variable (for localparam vs. param)
    ಸ್ಥಿರಾಂಕ VFwdType m_fwdType;  // Forward type for lint check
    string m_name;  // Name of variable
ಸಾರ್ವಜನಿಕ:
    AstParamTypeDType(File_ಸಾಲು* fl, VVarType type, VFwdType fwdType, ಸ್ಥಿರಾಂಕ string& name,
                      VFlag_ಮಗು_DType, Ast_ಗಂಟು_DType* dtp)
        : ASTGEN_SUPER_ParamTypeDType(fl)
        , m_varType{type}
        , m_fwdType{fwdType}
        , m_name{name} {
        ಮಗು_DTypep(dtp);  // Only for parser
        dtypep(nullptr);  // V3Width will resolve
    }
    ASTGEN_MEMBERS_AstParamTypeDType;
    ಶೂನ್ಯ dump(std::ostream& str = std::cout) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str = std::cout) ಸ್ಥಿರಾಂಕ override;
    string prettyDTypeName(bool ತುಂಬಿದ_2) ಸ್ಥಿರಾಂಕ override;
    Ast_ಗಂಟು_DType* get_ಮಗು_DTypep() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ ಮಗು_DTypep(); }
    Ast_ಗಂಟು_DType* subDTypep() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE {
        ಹಿಂತಿರುಗಿ dtypep() ? dtypep() : ಮಗು_DTypep();
    }
    AstBasicDType* basicp() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE { ಹಿಂತಿರುಗಿ subDTypep()->basicp(); }
    bool similar_DType_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* samep) ಸ್ಥಿರಾಂಕ override {
        ಸ್ಥಿರಾಂಕ AstParamTypeDType* ಸ್ಥಿರಾಂಕ sp = VN_DBG_AS(samep, ParamTypeDType);
        ಹಿಂತಿರುಗಿ this->subDTypep()->similarDType(sp->subDTypep());
    }
    ಪೂರ್ಣಾಂಕ ಅಗಲ_Align_ಬೈಟುಗಳು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ dtypep()->ಅಗಲ_Align_ಬೈಟುಗಳು(); }
    ಪೂರ್ಣಾಂಕ ಅಗಲ_ಒಟ್ಟು_ಬೈಟುಗಳು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ dtypep()->ಅಗಲ_ಒಟ್ಟು_ಬೈಟುಗಳು(); }
    // METHODS
    string name() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE { ಹಿಂತಿರುಗಿ m_name; }
    bool maybe_Pointed_ಗಮ್ಯ() ಸ್ಥಿರಾಂಕ override VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ true; }
    bool hasDType() ಸ್ಥಿರಾಂಕ override VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ true; }
    ಶೂನ್ಯ name(ಸ್ಥಿರಾಂಕ string& flag) override { m_name = flag; }
    VVarType varType() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_varType; }  // * = Type of variable
    VFwdType fwdType() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_fwdType; }
    bool isParam() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ true; }
    bool isGParam() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ (varType() == VVarType::GPARAM); }
    bool isCompound() ಸ್ಥಿರಾಂಕ override {
        v3fatalSrc("call isCompound on subdata type, not reference");
        ಹಿಂತಿರುಗಿ false;
    }
};
class AstParseTypeDType final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_DType {
    // Parents: VAR
    // During parsing, this indicates the type of a parameter is a "parameter type"
    // e.g. the data type is a container of any data type
    ಸ್ಥಿರಾಂಕ VFwdType m_fwdType;  // Forward type for lint check
ಸಾರ್ವಜನಿಕ:
    explicit AstParseTypeDType(File_ಸಾಲು* fl, VFwdType fwdType = VFwdType::NONE)
        : ASTGEN_SUPER_ParseTypeDType(fl)
        , m_fwdType{fwdType} {}
    ASTGEN_MEMBERS_AstParseTypeDType;
    Ast_ಗಂಟು_DType* dtypep() ಸ್ಥಿರಾಂಕ VL_MT_STABLE { ಹಿಂತಿರುಗಿ nullptr; }
    // METHODS
    ಶೂನ್ಯ dump(std::ostream& str = std::cout) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str = std::cout) ಸ್ಥಿರಾಂಕ override;
    bool similar_DType_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* samep) ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ this == samep; }
    AstBasicDType* basicp() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE { ಹಿಂತಿರುಗಿ nullptr; }
    ಪೂರ್ಣಾಂಕ ಅಗಲ_Align_ಬೈಟುಗಳು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ 0; }
    ಪೂರ್ಣಾಂಕ ಅಗಲ_ಒಟ್ಟು_ಬೈಟುಗಳು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ 0; }
    bool isCompound() ಸ್ಥಿರಾಂಕ override {
        v3fatalSrc("call isCompound on subdata type, not reference");
        ಹಿಂತಿರುಗಿ false;
    }
    VFwdType fwdType() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_fwdType; }
};
class Ast_ಸರತಿ_DType final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_DType {
    // Queue array data type, ie "[ $ ]"
    // @astgen op1 := childDTypep : Optional[AstNodeDType] // moved to refDTypep() in V3Width
    // @astgen op2 := boundp : Optional[AstNodeExpr]
    //
    // @astgen ptr := m_refDTypep : Optional[AstNodeDType]  // Elements of this type (post-width)
ಸಾರ್ವಜನಿಕ:
    Ast_ಸರತಿ_DType(File_ಸಾಲು* fl, VFlag_ಮಗು_DType, Ast_ಗಂಟು_DType* dtp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* boundp)
        : ASTGEN_SUPER_ಸರತಿ_DType(fl) {
        ಮಗು_DTypep(dtp);
        this->boundp(boundp);
        refDTypep(nullptr);
        dtypep(nullptr);  // V3Width will resolve
    }
    Ast_ಸರತಿ_DType(File_ಸಾಲು* fl, Ast_ಗಂಟು_DType* dtp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* boundp)
        : ASTGEN_SUPER_ಸರತಿ_DType(fl) {
        this->boundp(boundp);
        refDTypep(dtp);
        dtypep(dtp);
    }
    ASTGEN_MEMBERS_Ast_ಸರತಿ_DType;
    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* broken() ಸ್ಥಿರಾಂಕ override {
        BROKEN_RTN(!((m_refDTypep && !ಮಗು_DTypep()) || (!m_refDTypep && ಮಗು_DTypep())));
        ಹಿಂತಿರುಗಿ nullptr;
    }
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* samep) ಸ್ಥಿರಾಂಕ override {
        ಸ್ಥಿರಾಂಕ Ast_ಸರತಿ_DType* ಸ್ಥಿರಾಂಕ asamep = VN_DBG_AS(samep, ಸರತಿ_DType);
        ಆದರೆ (!asamep->subDTypep()) ಹಿಂತಿರುಗಿ false;
        ಹಿಂತಿರುಗಿ (subDTypep() == asamep->subDTypep());
    }
    bool similar_DType_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* samep) ಸ್ಥಿರಾಂಕ override {
        ಸ್ಥಿರಾಂಕ Ast_ಸರತಿ_DType* ಸ್ಥಿರಾಂಕ asamep = VN_DBG_AS(samep, ಸರತಿ_DType);
        ಹಿಂತಿರುಗಿ asamep->subDTypep() && subDTypep()->similarDType(asamep->subDTypep());
    }
    ಶೂನ್ಯ dumpSmall(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    string prettyDTypeName(bool ತುಂಬಿದ_2) ಸ್ಥಿರಾಂಕ override;
    Ast_ಗಂಟು_DType* get_ಮಗು_DTypep() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ ಮಗು_DTypep(); }
    Ast_ಗಂಟು_DType* subDTypep() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE {
        ಹಿಂತಿರುಗಿ m_refDTypep ? m_refDTypep : ಮಗು_DTypep();
    }
    bool isAggregateType() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    ಶೂನ್ಯ refDTypep(Ast_ಗಂಟು_DType* nodep) { m_refDTypep = nodep; }
    ಸಾಲೊಳಗೆ ಪೂರ್ಣಾಂಕ boundConst() ಸ್ಥಿರಾಂಕ VL_MT_STABLE;
    Ast_ಗಂಟು_DType* virtRefDTypep() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ m_refDTypep; }
    ಶೂನ್ಯ virtRefDTypep(Ast_ಗಂಟು_DType* nodep) override { refDTypep(nodep); }
    // METHODS
    AstBasicDType* basicp() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE { ಹಿಂತಿರುಗಿ nullptr; }
    ಪೂರ್ಣಾಂಕ ಅಗಲ_Align_ಬೈಟುಗಳು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ subDTypep()->ಅಗಲ_Align_ಬೈಟುಗಳು(); }
    ಪೂರ್ಣಾಂಕ ಅಗಲ_ಒಟ್ಟು_ಬೈಟುಗಳು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ subDTypep()->ಅಗಲ_ಒಟ್ಟು_ಬೈಟುಗಳು(); }
    bool isCompound() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool isDynamicallySized() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
};
class AstRefDType final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_DType {
    // @astgen op1 := typeofp : Optional[AstNode<AstNodeExpr|AstNodeDType>]
    // @astgen op2 := classOrPackageOpp : Optional[AstNodeExpr]
    // @astgen op3 := paramsp : List[AstPin]
    //
    // Pre-Width must reference the Typeref, not what it points to, as some child
    // types like AstBracketArrayType will disappear and can't lose the handle
    //
    // @astgen ptr := m_typedefp : Optional[AstTypedef]  // Referenced type
    // Post-width typedefs are removed and point to type directly
    // @astgen ptr := m_refDTypep : Optional[AstNodeDType]  // Data type references
    // @astgen ptr := m_classOrPackagep : Optional[AstNodeModule]  // Class/package defined in
    string m_name;  // Name of an AstTypedef
    // Interface capture tag, owned by V3LinkDotIfaceCapture; cloning copies it
    ಸ್ಥಿರಾಂಕ VIface_Capture_ಟ್ಯಾಗ್* m_captureTagp = nullptr;

ಸಾರ್ವಜನಿಕ:
    AstRefDType(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ string& name)
        : ASTGEN_SUPER_RefDType(fl)
        , m_name{name} {}
    AstRefDType(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ string& name, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* classOrPackagep, AstPin* paramsp)
        : ASTGEN_SUPER_RefDType(fl)
        , m_name{name} {
        classOrPackageOpp(classOrPackagep);
        ಕೂಡಿಸು_Paramsp(paramsp);
    }
    class Flag_Type_ದ_ಅಭಿವ್ಯಕ್ತಿ {};  // type(expr) for parser only
    AstRefDType(File_ಸಾಲು* fl, Flag_Type_ದ_ಅಭಿವ್ಯಕ್ತಿ, Ast_ಗಂಟು* typeofp)
        : ASTGEN_SUPER_RefDType(fl) {
        this->typeofp(typeofp);
        ಆದರೆ (Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ dtp = VN_CAST(typeofp, ಗಂಟು_DType)) refDTypep(dtp);
    }
    ASTGEN_MEMBERS_AstRefDType;
    // METHODS
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* samep) ಸ್ಥಿರಾಂಕ override {
        ಸ್ಥಿರಾಂಕ AstRefDType* ಸ್ಥಿರಾಂಕ asamep = VN_DBG_AS(samep, RefDType);
        ಹಿಂತಿರುಗಿ (m_typedefp == asamep->m_typedefp && m_refDTypep == asamep->m_refDTypep
                && m_name == asamep->m_name && m_classOrPackagep == asamep->m_classOrPackagep);
    }
    bool similar_DType_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* samep) ಸ್ಥಿರಾಂಕ override {
        ಹಿಂತಿರುಗಿ subDTypep()->similarDType(samep->subDTypep());
    }
    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* broken() ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dump(std::ostream& str = std::cout) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str = std::cout) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpSmall(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    string name() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE { ಹಿಂತಿರುಗಿ m_name; }
    string prettyDTypeName(bool ತುಂಬಿದ_2) ಸ್ಥಿರಾಂಕ override {
        ಹಿಂತಿರುಗಿ subDTypep() ? subDTypep()->prettyDTypeName(ತುಂಬಿದ_2) : prettyName();
    }
    AstBasicDType* basicp() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE {
        ಹಿಂತಿರುಗಿ subDTypep() ? subDTypep()->basicp() : nullptr;
    }
    Ast_ಗಂಟು_DType* subDTypep() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE;
    Ast_ಗಂಟು_DType* get_ಮಗು_DTypep() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ VN_CAST(typeofp(), ಗಂಟು_DType); }
    ಪೂರ್ಣಾಂಕ ಅಗಲ_Align_ಬೈಟುಗಳು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ dtypeSkipRefp()->ಅಗಲ_Align_ಬೈಟುಗಳು(); }
    ಪೂರ್ಣಾಂಕ ಅಗಲ_ಒಟ್ಟು_ಬೈಟುಗಳು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ dtypeSkipRefp()->ಅಗಲ_ಒಟ್ಟು_ಬೈಟುಗಳು(); }
    ಶೂನ್ಯ name(ಸ್ಥಿರಾಂಕ string& flag) override { m_name = flag; }
    Ast_ಗಂಟು_DType* dtypeSkipRefp() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ subDTypep()->skipRefp(); }
    AstTypedef* typedefp() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ m_typedefp; }
    ಶೂನ್ಯ typedefp(AstTypedef* nodep) { m_typedefp = nodep; }
    Ast_ಗಂಟು_DType* refDTypep() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ m_refDTypep; }
    ಶೂನ್ಯ refDTypep(Ast_ಗಂಟು_DType* nodep) { m_refDTypep = nodep; }
    Ast_ಗಂಟು_DType* virtRefDTypep() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ refDTypep(); }
    ಶೂನ್ಯ virtRefDTypep(Ast_ಗಂಟು_DType* nodep) override { refDTypep(nodep); }
    Ast_ಗಂಟು_Module* classOrPackagep() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_classOrPackagep; }
    ಶೂನ್ಯ classOrPackagep(Ast_ಗಂಟು_Module* nodep) { m_classOrPackagep = nodep; }
    ಸ್ಥಿರಾಂಕ VIface_Capture_ಟ್ಯಾಗ್* captureTagp() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_captureTagp; }
    ಶೂನ್ಯ captureTagp(ಸ್ಥಿರಾಂಕ VIface_Capture_ಟ್ಯಾಗ್* tagp) { m_captureTagp = tagp; }
    bool isCompound() ಸ್ಥಿರಾಂಕ override {
        v3fatalSrc("call isCompound on subdata type, not reference");
        ಹಿಂತಿರುಗಿ false;
    }
};
class AstRequireDType final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_DType {
    // @astgen op1 := lhsp : Optional[AstNode<AstNodeExpr|AstNodeDType>]
    //
    // Require a generic node type (typically AstParseRef become a type.
ಸಾರ್ವಜನಿಕ:
    AstRequireDType(File_ಸಾಲು* fl, Ast_ಗಂಟು* lhsp)
        : ASTGEN_SUPER_RequireDType(fl) {
        this->lhsp(lhsp);
    }
    ASTGEN_MEMBERS_AstRequireDType;
    // METHODS
    bool similar_DType_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* samep) ಸ್ಥಿರಾಂಕ override {
        ಸ್ಥಿರಾಂಕ AstRequireDType* ಸ್ಥಿರಾಂಕ asamep = VN_DBG_AS(samep, RequireDType);
        ಹಿಂತಿರುಗಿ subDTypep()->similarDType(asamep->subDTypep());
    }
    AstBasicDType* basicp() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE { ಹಿಂತಿರುಗಿ nullptr; }
    Ast_ಗಂಟು_DType* subDTypep() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE {
        // Used for recursive definition checking
        ಆದರೆ (Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ dtp = VN_CAST(lhsp(), ಗಂಟು_DType)) ಹಿಂತಿರುಗಿ dtp;

        ಹಿಂತಿರುಗಿ nullptr;
    }
    ಪೂರ್ಣಾಂಕ ಅಗಲ_Align_ಬೈಟುಗಳು() ಸ್ಥಿರಾಂಕ override { V3ERROR_NA_RETURN(1); }
    ಪೂರ್ಣಾಂಕ ಅಗಲ_ಒಟ್ಟು_ಬೈಟುಗಳು() ಸ್ಥಿರಾಂಕ override { V3ERROR_NA_RETURN(1); }
    bool isCompound() ಸ್ಥಿರಾಂಕ override { V3ERROR_NA_RETURN(false); }
};
class Ast_Sample_ಸರತಿ_DType final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_DType {
    // @astgen op1 := childDTypep : Optional[AstNodeDType] // moved to refDTypep() in V3Width
    //
    // @astgen ptr := m_refDTypep : Optional[AstNodeDType]  // Elements of this type (post-width)
ಸಾರ್ವಜನಿಕ:
    Ast_Sample_ಸರತಿ_DType(File_ಸಾಲು* fl, Ast_ಗಂಟು_DType* dtp)
        : ASTGEN_SUPER_Sample_ಸರತಿ_DType(fl) {
        refDTypep(dtp);
        dtypep(dtp);
    }
    ASTGEN_MEMBERS_Ast_Sample_ಸರತಿ_DType;
    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* broken() ಸ್ಥಿರಾಂಕ override {
        BROKEN_RTN(!((m_refDTypep && !ಮಗು_DTypep()) || (!m_refDTypep && ಮಗು_DTypep())));
        ಹಿಂತಿರುಗಿ nullptr;
    }
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* samep) ಸ್ಥಿರಾಂಕ override {
        ಸ್ಥಿರಾಂಕ Ast_Sample_ಸರತಿ_DType* ಸ್ಥಿರಾಂಕ asamep = VN_DBG_AS(samep, Sample_ಸರತಿ_DType);
        ಆದರೆ (!asamep->subDTypep()) ಹಿಂತಿರುಗಿ false;
        ಹಿಂತಿರುಗಿ (subDTypep() == asamep->subDTypep());
    }
    bool similar_DType_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* samep) ಸ್ಥಿರಾಂಕ override {
        ಸ್ಥಿರಾಂಕ Ast_Sample_ಸರತಿ_DType* ಸ್ಥಿರಾಂಕ asamep = VN_DBG_AS(samep, Sample_ಸರತಿ_DType);
        ಹಿಂತಿರುಗಿ asamep->subDTypep() && subDTypep()->similarDType(asamep->subDTypep());
    }
    ಶೂನ್ಯ dumpSmall(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    Ast_ಗಂಟು_DType* get_ಮಗು_DTypep() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ ಮಗು_DTypep(); }
    // op1 = Range of variable
    Ast_ಗಂಟು_DType* subDTypep() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE {
        ಹಿಂತಿರುಗಿ m_refDTypep ? m_refDTypep : ಮಗು_DTypep();
    }
    ಶೂನ್ಯ refDTypep(Ast_ಗಂಟು_DType* nodep) { m_refDTypep = nodep; }
    Ast_ಗಂಟು_DType* virtRefDTypep() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ m_refDTypep; }
    ಶೂನ್ಯ virtRefDTypep(Ast_ಗಂಟು_DType* nodep) override { refDTypep(nodep); }
    // METHODS
    AstBasicDType* basicp() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE { ಹಿಂತಿರುಗಿ subDTypep()->basicp(); }
    ಪೂರ್ಣಾಂಕ ಅಗಲ_Align_ಬೈಟುಗಳು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ ಗಾತ್ರ(std::map<std::string, std::string>); }
    ಪೂರ್ಣಾಂಕ ಅಗಲ_ಒಟ್ಟು_ಬೈಟುಗಳು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ ಗಾತ್ರ(std::map<std::string, std::string>); }
    bool isCompound() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
};
class Ast_ಹರಿವು_DType final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_DType {
    // Stream data type, used only as data type of stream operations
    // Should behave like AstPackArrayDType, but it doesn't have a size
ಸಾರ್ವಜನಿಕ:
    explicit Ast_ಹರಿವು_DType(File_ಸಾಲು* fl)
        : ASTGEN_SUPER_ಹರಿವು_DType(fl) {
        dtypep(this);
    }
    ASTGEN_MEMBERS_Ast_ಹರಿವು_DType;
    ಶೂನ್ಯ dumpSmall(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    bool hasDType() ಸ್ಥಿರಾಂಕ override VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ true; }
    bool maybe_Pointed_ಗಮ್ಯ() ಸ್ಥಿರಾಂಕ override VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ true; }
    bool undead() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    Ast_ಗಂಟು_DType* subDTypep() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE { ಹಿಂತಿರುಗಿ nullptr; }
    Ast_ಗಂಟು_DType* virtRefDTypep() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ nullptr; }
    ಶೂನ್ಯ virtRefDTypep(Ast_ಗಂಟು_DType* nodep) override {}
    bool similar_DType_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* samep) ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ this == samep; }
    AstBasicDType* basicp() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE { ಹಿಂತಿರುಗಿ nullptr; }
    ಪೂರ್ಣಾಂಕ ಅಗಲ_Align_ಬೈಟುಗಳು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ 1; }
    ಪೂರ್ಣಾಂಕ ಅಗಲ_ಒಟ್ಟು_ಬೈಟುಗಳು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ 1; }
    bool isCompound() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
};
class AstUnsizedArrayDType final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_DType {
    // Unsized/open-range Array data type, ie "some_dtype var_name []"
    // @astgen op1 := childDTypep : Optional[AstNodeDType] // moved to refDTypep() in V3Width
    //
    // @astgen ptr := m_refDTypep : Optional[AstNodeDType]  // Elements of this type (post-width)
ಸಾರ್ವಜನಿಕ:
    AstUnsizedArrayDType(File_ಸಾಲು* fl, VFlag_ಮಗು_DType, Ast_ಗಂಟು_DType* dtp)
        : ASTGEN_SUPER_UnsizedArrayDType(fl) {
        ಮಗು_DTypep(dtp);  // Only for parser
        refDTypep(nullptr);
        dtypep(nullptr);  // V3Width will resolve
    }
    AstUnsizedArrayDType(File_ಸಾಲು* fl, Ast_ಗಂಟು_DType* dtp)
        : ASTGEN_SUPER_UnsizedArrayDType(fl) {
        refDTypep(dtp);
        dtypep(nullptr);  // V3Width will resolve
    }
    ASTGEN_MEMBERS_AstUnsizedArrayDType;
    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* broken() ಸ್ಥಿರಾಂಕ override {
        BROKEN_RTN(!((m_refDTypep && !ಮಗು_DTypep()) || (!m_refDTypep && ಮಗು_DTypep())));
        ಹಿಂತಿರುಗಿ nullptr;
    }
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* samep) ಸ್ಥಿರಾಂಕ override;
    bool similar_DType_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* samep) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpSmall(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    Ast_ಗಂಟು_DType* get_ಮಗು_DTypep() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ ಮಗು_DTypep(); }
    Ast_ಗಂಟು_DType* subDTypep() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE {
        ಹಿಂತಿರುಗಿ m_refDTypep ? m_refDTypep : ಮಗು_DTypep();
    }
    ಶೂನ್ಯ refDTypep(Ast_ಗಂಟು_DType* nodep) { m_refDTypep = nodep; }
    Ast_ಗಂಟು_DType* virtRefDTypep() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ m_refDTypep; }
    ಶೂನ್ಯ virtRefDTypep(Ast_ಗಂಟು_DType* nodep) override { refDTypep(nodep); }
    // METHODS
    AstBasicDType* basicp() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE { ಹಿಂತಿರುಗಿ subDTypep()->basicp(); }
    ಪೂರ್ಣಾಂಕ ಅಗಲ_Align_ಬೈಟುಗಳು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ subDTypep()->ಅಗಲ_Align_ಬೈಟುಗಳು(); }
    ಪೂರ್ಣಾಂಕ ಅಗಲ_ಒಟ್ಟು_ಬೈಟುಗಳು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ subDTypep()->ಅಗಲ_ಒಟ್ಟು_ಬೈಟುಗಳು(); }
    bool isCompound() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
};
class AstVoidDType final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_DType {
    // For e.g. a function returning void
ಸಾರ್ವಜನಿಕ:
    explicit AstVoidDType(File_ಸಾಲು* fl)
        : ASTGEN_SUPER_VoidDType(fl) {
        dtypep(this);
    }
    ASTGEN_MEMBERS_AstVoidDType;
    ಶೂನ್ಯ dumpSmall(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    bool hasDType() ಸ್ಥಿರಾಂಕ override VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ true; }
    bool maybe_Pointed_ಗಮ್ಯ() ಸ್ಥಿರಾಂಕ override VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ true; }
    bool undead() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    Ast_ಗಂಟು_DType* subDTypep() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE { ಹಿಂತಿರುಗಿ nullptr; }
    Ast_ಗಂಟು_DType* virtRefDTypep() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ nullptr; }
    ಶೂನ್ಯ virtRefDTypep(Ast_ಗಂಟು_DType* nodep) override {}
    bool similar_DType_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* samep) ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    AstBasicDType* basicp() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE { ಹಿಂತಿರುಗಿ nullptr; }
    ಪೂರ್ಣಾಂಕ ಅಗಲ_Align_ಬೈಟುಗಳು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ 1; }
    ಪೂರ್ಣಾಂಕ ಅಗಲ_ಒಟ್ಟು_ಬೈಟುಗಳು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ 1; }
    bool isCompound() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
};
class AstWildcardArrayDType final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_DType {
    // Wildcard index type associative array data type, ie "some_dtype var_name [*]"
    // @astgen op1 := childDTypep : Optional[AstNodeDType] // moved to refDTypep() in V3Width
    //
    // @astgen ptr := m_refDTypep : Optional[AstNodeDType]  // Elements of this type (post-width)
ಸಾರ್ವಜನಿಕ:
    AstWildcardArrayDType(File_ಸಾಲು* fl, VFlag_ಮಗು_DType, Ast_ಗಂಟು_DType* dtp)
        : ASTGEN_SUPER_WildcardArrayDType(fl) {
        ಮಗು_DTypep(dtp);  // Only for parser
        refDTypep(nullptr);
        dtypep(nullptr);  // V3Width will resolve
    }
    ASTGEN_MEMBERS_AstWildcardArrayDType;
    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* broken() ಸ್ಥಿರಾಂಕ override {
        BROKEN_RTN(!((m_refDTypep && !ಮಗು_DTypep()) || (!m_refDTypep && ಮಗು_DTypep())));
        ಹಿಂತಿರುಗಿ nullptr;
    }
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* samep) ಸ್ಥಿರಾಂಕ override;
    bool similar_DType_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* samep) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpSmall(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    string prettyDTypeName(bool ತುಂಬಿದ_2) ಸ್ಥಿರಾಂಕ override;
    Ast_ಗಂಟು_DType* get_ಮಗು_DTypep() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ ಮಗು_DTypep(); }
    Ast_ಗಂಟು_DType* subDTypep() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE {
        ಹಿಂತಿರುಗಿ m_refDTypep ? m_refDTypep : ಮಗು_DTypep();
    }
    ಶೂನ್ಯ refDTypep(Ast_ಗಂಟು_DType* nodep) { m_refDTypep = nodep; }
    Ast_ಗಂಟು_DType* virtRefDTypep() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ m_refDTypep; }
    ಶೂನ್ಯ virtRefDTypep(Ast_ಗಂಟು_DType* nodep) override { refDTypep(nodep); }
    // METHODS
    AstBasicDType* basicp() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE { ಹಿಂತಿರುಗಿ subDTypep()->basicp(); }
    ಪೂರ್ಣಾಂಕ ಅಗಲ_Align_ಬೈಟುಗಳು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ ಗಾತ್ರ(std::map<std::string, std::string>); }
    ಪೂರ್ಣಾಂಕ ಅಗಲ_ಒಟ್ಟು_ಬೈಟುಗಳು() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ ಗಾತ್ರ(std::map<std::string, std::string>); }
    bool isCompound() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    bool isDynamicallySized() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
};

// === AstNodeArrayDType ===
class AstPackArrayDType final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Array_DType {
    // Packed array data type, ie "some_dtype [2:0] var_name"
ಸಾರ್ವಜನಿಕ:
    ಸಾಲೊಳಗೆ AstPackArrayDType(File_ಸಾಲು* fl, VFlag_ಮಗು_DType, Ast_ಗಂಟು_DType* dtp, AstRange* rangep);
    ಸಾಲೊಳಗೆ AstPackArrayDType(File_ಸಾಲು* fl, Ast_ಗಂಟು_DType* dtp, AstRange* rangep);
    ASTGEN_MEMBERS_AstPackArrayDType;
    string prettyDTypeName(bool ತುಂಬಿದ_2) ಸ್ಥಿರಾಂಕ override;
    bool isCompound() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
};
class AstUnpackArrayDType final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Array_DType {
    // Array data type, ie "some_dtype var_name [2:0]"
    bool m_isCompound = false;  // Non-POD subDType, or parent requires compound
ಸಾರ್ವಜನಿಕ:
    AstUnpackArrayDType(File_ಸಾಲು* fl, VFlag_ಮಗು_DType, Ast_ಗಂಟು_DType* dtp, AstRange* rangep)
        : ASTGEN_SUPER_UnpackArrayDType(fl) {
        ಮಗು_DTypep(dtp);  // Only for parser
        this->rangep(rangep);
        refDTypep(nullptr);
        dtypep(nullptr);  // V3Width will resolve
        // For backward compatibility AstNodeArrayDType and others inherit
        // width and signing from the subDType/base type
        ಅಗಲ_From_Sub(subDTypep());
    }
    AstUnpackArrayDType(File_ಸಾಲು* fl, Ast_ಗಂಟು_DType* dtp, AstRange* rangep)
        : ASTGEN_SUPER_UnpackArrayDType(fl) {
        this->rangep(rangep);
        refDTypep(dtp);
        dtypep(this);
        // For backward compatibility AstNodeArrayDType and others inherit
        // width and signing from the subDType/base type
        ಅಗಲ_From_Sub(subDTypep());
    }
    ASTGEN_MEMBERS_AstUnpackArrayDType;
    ಶೂನ್ಯ dump(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    string prettyDTypeName(bool ತುಂಬಿದ_2) ಸ್ಥಿರಾಂಕ override;
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* samep) ಸ್ಥಿರಾಂಕ override {
        ಸ್ಥಿರಾಂಕ AstUnpackArrayDType* ಸ್ಥಿರಾಂಕ sp = VN_DBG_AS(samep, UnpackArrayDType);
        ಹಿಂತಿರುಗಿ m_isCompound == sp->m_isCompound;
    }
    bool similar_DType_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* samep) ಸ್ಥಿರಾಂಕ override;
    bool isAggregateType() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ true; }
    // True iff 'otherp' is an array of the same shape, with
    // at least one dimension reversed ascending/descending.
    bool is_ಹಿಮ್ಮುಖ_ದ(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* otherp) ಸ್ಥಿರಾಂಕ;
    // Outer dimension comes first. The first element is this node.
    std::ಸದಿಶ<AstUnpackArrayDType*> unpackDimensions();
    ಶೂನ್ಯ isCompound(bool flag) { m_isCompound = flag; }
    bool isCompound() ಸ್ಥಿರಾಂಕ override VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ m_isCompound; }
    bool isIntegralOrPacked() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ false; }
};

// === AstNodeUOrStructDType ===
class AstStructDType final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_UOr_Struct_DType {
ಸಾರ್ವಜನಿಕ:
    // VSigning below is mispurposed to indicate if packed or not
    AstStructDType(File_ಸಾಲು* fl, VSigning numericUnpack)
        : ASTGEN_SUPER_StructDType(fl, numericUnpack) {}
    ASTGEN_MEMBERS_AstStructDType;
    string verilogKwd() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ "struct"; }
};
class AstUnionDType final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_UOr_Struct_DType {
    bool m_isSoft;  // Is a "union soft"
    bool m_isTagged;  // Is a "union tagged"

ಸಾರ್ವಜನಿಕ:
    // VSigning below is mispurposed to indicate if packed or not
    // isSoft implies packed
    AstUnionDType(File_ಸಾಲು* fl, bool isSoft, bool isTagged, VSigning numericUnpack)
        : ASTGEN_SUPER_UnionDType(fl, numericUnpack)
        , m_isSoft{isSoft}
        , m_isTagged{isTagged} {
        packed(packed() | m_isSoft);
    }
    ASTGEN_MEMBERS_AstUnionDType;
    string verilogKwd() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ "union"; }
    bool isSoft() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_isSoft; }
    bool isTagged() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_isTagged; }
    bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* samep) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dump(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಸ್ಥಿರಾಂಕ override;
};

#ಆದರೆ_ಅಂತ್ಯ  // Guard
