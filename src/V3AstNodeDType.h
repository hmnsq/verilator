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

#ifndef VERILATOR_V3ASTNODEDTYPE_H_
#define VERILATOR_V3ASTNODEDTYPE_H_

#ifndef VERILATOR_V3AST_H_
#error "Use V3Ast.h as the include"
#ಅನ್ತರ್ಭಾವ "V3Ast.h"  // This helps code analysis tools pick up symbols in V3Ast.h
#define VL_NOT_FINAL  // This #define fixes broken code folding in the CLion IDE
#endif

// === Abstract base node types (AstNode*) =====================================

class Ast_ಗಂಟು_DType VL_NOT_FINAL : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    // Ideally width() would migrate to BasicDType as that's where it makes sense,
    // but it's currently so prevalent in the code we leave it here.
    // Note the below members are included in AstTypeTable::Key lookups
    // dist-ast-dump-suppress  // Part of dumpSmall
    ಪೂರ್ಣಾಙ್ಕ m_width = 0;  // (also in AstTypeTable::Key) Bit width of operation
    // (also in AstTypeTable::Key) If unsized, bitwidth of minimum implementation
    // dist-ast-dump-suppress  // Part of dumpSmall
    ಪೂರ್ಣಾಙ್ಕ m_widthMin = 0;
    // dist-ast-dump-suppress  // Part of dumpSmall
    VSigning m_numeric;  // (also in AstTypeTable::Key) Node is signed
    // dist-ast-dump-suppress  // Part of dumpSmall
    bool m_generic = false;  // Simple globally referenced type, don't garbage collect
    // Unique number assigned to each dtype during creation for IEEE matching
    ಸ್ಥಿರ ಪೂರ್ಣಾಙ್ಕ s_unique_ಅಂಕೆ;

ರಕ್ಷಿತ:
    // CONSTRUCTORS
    Ast_ಗಂಟು_DType(VNType t, File_ಸಾಲು* fl)
        : Ast_ಗಂಟು{t, fl} {}

ಗುಪ್ತ:
    // METHODS
    ಧ್ರುವ Ast_ಗಂಟು_DType* skipRefIterp(bool skipConst, bool skipEnum,
                                     bool assertOn = true) ಧ್ರುವ VL_MT_STABLE;

ರಕ್ಷಿತ:
    // METHODS
    ಆಭಾಸ bool similar_DType_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು_DType* samep) ಧ್ರುವ = 0;

ಸಾರ್ವಜನಿಕ:
    ASTGEN_MEMBERS_Ast_ಗಂಟು_DType;
    // ACCESSORS
    ಶೂನ್ಯ dump(std::ostream& str) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಧ್ರುವ override;
    ಆಭಾಸ ಶೂನ್ಯ dumpSmall(std::ostream& str) ಧ್ರುವ VL_MT_STABLE;
    bool hasDType() ಧ್ರುವ override VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ true; }
    /// Require VlUnpacked, instead of [] for POD elements.
    /// A non-POD object is always compound, but some POD elements
    /// are compound when methods calls operate on object, or when
    /// under another compound-requiring object e.g. class
    ಆಭಾಸ bool isCompound() ಧ್ರುವ = 0;
    // Integral or packed, allowed inside an unpacked union/struct
    ಆಭಾಸ bool isIntegralOrPacked() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ !isCompound(); }
    // (Slow) recurse down to find basic data type
    ಆಭಾಸ AstBasicDType* basicp() ಧ್ರುವ VL_MT_STABLE = 0;
    // (Slow) Recurse over MemberDType|ParamTypeDType|RefDType|ConstDType|EnumDType to other type
    ಧ್ರುವ Ast_ಗಂಟು_DType* skipRefp() ಧ್ರುವ VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ skipRefIterp(true, true); }
    Ast_ಗಂಟು_DType* skipRefp() VL_MT_STABLE {
        ಪ್ರತ್ಯಾಗಮ const_cast<Ast_ಗಂಟು_DType*>(
            static_cast<ಧ್ರುವ Ast_ಗಂಟು_DType*>(this)->skipRefIterp(true, true));
    }
    // (Slow) Recurse over MemberDType|ParamTypeDType|RefDType|ConstDType|EnumDType to other type,
    // Returns null if not resolved
    ಧ್ರುವ Ast_ಗಂಟು_DType* skipRefOrNullp() ಧ್ರುವ VL_MT_STABLE {
        ಪ್ರತ್ಯಾಗಮ skipRefIterp(true, true, false);
    }
    Ast_ಗಂಟು_DType* skipRefOrNullp() VL_MT_STABLE {
        ಪ್ರತ್ಯಾಗಮ const_cast<Ast_ಗಂಟು_DType*>(
            static_cast<ಧ್ರುವ Ast_ಗಂಟು_DType*>(this)->skipRefIterp(true, true, false));
    }
    // (Slow) Recurse over MemberDType|ParamTypeDType|RefDType|EnumDType to ConstDType
    ಧ್ರುವ Ast_ಗಂಟು_DType* skipRefToConstp() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ skipRefIterp(false, true); }
    Ast_ಗಂಟು_DType* skipRefToConstp() {
        ಪ್ರತ್ಯಾಗಮ const_cast<Ast_ಗಂಟು_DType*>(
            static_cast<ಧ್ರುವ Ast_ಗಂಟು_DType*>(this)->skipRefIterp(false, true));
    }
    // (Slow) Recurse over MemberDType|ParamTypeDType|RefDType|ConstDType to EnumDType
    ಧ್ರುವ Ast_ಗಂಟು_DType* skipRefToEnump() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ skipRefIterp(true, false); }
    Ast_ಗಂಟು_DType* skipRefToEnump() {
        ಪ್ರತ್ಯಾಗಮ const_cast<Ast_ಗಂಟು_DType*>(
            static_cast<ಧ್ರುವ Ast_ಗಂಟು_DType*>(this)->skipRefIterp(true, false));
    }
    // (Slow) Recurse over MemberDType|ParamTypeDType|RefDType|ConstDType to EnumDType,
    // Returns null if not resolved
    ಧ್ರುವ Ast_ಗಂಟು_DType* skipRefToEnumOrNullp() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ skipRefIterp(true, false, false); }
    // (Slow) Recurse over MemberDType|ParamTypeDType|RefDType to other type
    ಧ್ರುವ Ast_ಗಂಟು_DType* skipRefToNonRefp() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ skipRefIterp(false, false); }
    Ast_ಗಂಟು_DType* skipRefToNonRefp() {
        ಪ್ರತ್ಯಾಗಮ const_cast<Ast_ಗಂಟು_DType*>(
            static_cast<ಧ್ರುವ Ast_ಗಂಟು_DType*>(this)->skipRefIterp(false, false));
    }
    // If array, returns element dtype, otherwise returns skipRef dtype
    // If skipRef is false, RefDTypes are not followed (safe before typedef linking)
    ಧ್ರುವ Ast_ಗಂಟು_DType* elemDTypep(bool skipRef = true) ಧ್ರುವ VL_MT_STABLE;
    Ast_ಗಂಟು_DType* elemDTypep(bool skipRef = true) VL_MT_STABLE {
        ಪ್ರತ್ಯಾಗಮ const_cast<Ast_ಗಂಟು_DType*>(
            static_cast<ಧ್ರುವ Ast_ಗಂಟು_DType*>(this)->elemDTypep(skipRef));
    }
    // (Slow) recurses - Structure alignment 1,2,4 or 8 bytes (arrays affect this)
    ಆಭಾಸ ಪೂರ್ಣಾಙ್ಕ widthAlignBytes() ಧ್ರುವ = 0;
    // (Slow) recurses - Width in bytes rounding up 1,2,4,8,12,...
    ಆಭಾಸ ಪೂರ್ಣಾಙ್ಕ widthTotalBytes() ಧ್ರುವ = 0;
    bool maybePointedTo() ಧ್ರುವ override VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ true; }
    // Iff has a non-null refDTypep(), as generic node function
    ಆಭಾಸ Ast_ಗಂಟು_DType* virtRefDTypep() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ nullptr; }
    // Iff has refDTypep(), set as generic node function
    ಆಭಾಸ ಶೂನ್ಯ virtRefDTypep(Ast_ಗಂಟು_DType* nodep) {}
    // Iff has a non-null second dtypep, as generic node function
    ಆಭಾಸ Ast_ಗಂಟು_DType* virtRefDType2p() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ nullptr; }
    // Iff has second dtype, set as generic node function
    ಆಭಾಸ ಶೂನ್ಯ virtRefDType2p(Ast_ಗಂಟು_DType* nodep) {}
    // Assignable equivalence.  Calls skipRefToNonRefp() during comparisons.
    bool similarDType(ಧ್ರುವ Ast_ಗಂಟು_DType* samep) ಧ್ರುವ;
    // Iff has a non-null subDTypep(), as generic node function
    ಆಭಾಸ Ast_ಗಂಟು_DType* subDTypep() ಧ್ರುವ VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ nullptr; }
    ಆಭಾಸ Ast_ಗಂಟು_DType* subDType2p() ಧ್ರುವ VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ nullptr; }
    ಆಭಾಸ bool isAggregateType() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ false; }
    // True for unpacked, dynamic, queue, and associative arrays (not packed arrays)
    bool isNonPackedArray() ಧ್ರುವ;
    ಆಭಾಸ bool isFourstate() ಧ್ರುವ;
    // Ideally an IEEE $typename
    ಆಭಾಸ string prettyDTypeName(bool) ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ prettyTypeName(); }
    string prettyDTypeNameQ(bool full = false) ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ "'" + prettyDTypeName(full) + "'"; }
    string stateDTypeName() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ this->isFourstate() ? "(4-state)" : "(2-state)"; }
    //
    // Changing the width may confuse the data type resolution, so must clear
    // TypeTable cache after use.
    ಶೂನ್ಯ widthForce(ಪೂರ್ಣಾಙ್ಕ width, ಪೂರ್ಣಾಙ್ಕ widthMin) {
        m_width = width;
        m_widthMin = widthMin;
    }
    // For backward compatibility inherit width and signing from the subDType/base type
    ಶೂನ್ಯ widthFromSub(ಧ್ರುವ Ast_ಗಂಟು_DType* nodep) {
        m_width = nodep->m_width;
        m_widthMin = nodep->m_widthMin;
        m_numeric = nodep->m_numeric;
    }
    //
    ಪೂರ್ಣಾಙ್ಕ width() ಧ್ರುವ VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ m_width; }
    ಶೂನ್ಯ numeric(VSigning flag) { m_numeric = flag; }
    bool isSigned() ಧ್ರುವ VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ m_numeric.isSigned(); }
    bool isNosign() ಧ್ರುವ VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ m_numeric.isNosign(); }
    VSigning numeric() ಧ್ರುವ VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ m_numeric; }
    ಪೂರ್ಣಾಙ್ಕ width_ಪದಗಳು() ಧ್ರುವ VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ VL_ಪದಗಳು_I(width()); }
    ಪೂರ್ಣಾಙ್ಕ widthMin() ಧ್ರುವ VL_MT_STABLE {  // If sized, the size,
                                         // if unsized the min digits to represent it
        ಪ್ರತ್ಯಾಗಮ m_widthMin ? m_widthMin : m_width;
    }
    ಪೂರ್ಣಾಙ್ಕ widthPow2() ಧ್ರುವ;
    ಶೂನ್ಯ widthMinFromWidth() { m_widthMin = m_width; }
    bool widthSized() ಧ್ರುವ VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ !m_widthMin || m_widthMin == m_width; }
    bool generic() ಧ್ರುವ VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ m_generic; }
    ಶೂನ್ಯ generic(bool flag) { m_generic = flag; }
    std::pair<uint32_t, uint32_t> dimensions(bool includeBasic) ಧ್ರುವ;
    uint32_t arrayUnpackedElements() ಧ್ರುವ;  // 1, or total multiplication of all dimensions
    // Fixed aggregate streaming properties
    bool isStreamableFixedAggregate() ಧ್ರುವ;
    // Whether is a string, or an unpacked array or structure containing one
    bool containsString() ಧ್ರುವ;
    bool containsUnpackedStruct() ಧ್ರುವ;
    ಪೂರ್ಣಾಙ್ಕ widthStream() ಧ್ರುವ;
    string vlEnumType() ಧ್ರುವ;  // Return VerilatedVarType: VLVT_UINT32, etc
    string vlEnumPackedAgg() ಧ್ರುವ;  // Return VLVF_PACKED_AGG etc flags, or empty
    ಸ್ಥಿರ ಪೂರ್ಣಾಙ್ಕ unique_ಅಂಕೆ_Inc() { ಪ್ರತ್ಯಾಗಮ ++s_unique_ಅಂಕೆ; }
    ಧ್ರುವ ವರ್ಣ* charIQWN() ಧ್ರುವ {
        ಪ್ರತ್ಯಾಗಮ (isString() ? "N" : isWide() ? "W" : isDouble() ? "D" : isQuad() ? "Q" : "I");
    }
    string cType(ಧ್ರುವ string& name, bool forFunc, bool isRef, bool packed = false) ಧ್ರುವ;
    // Represents a C++ LiteralType? (can be constexpr)
    bool isLiteralType() ಧ್ರುವ VL_MT_STABLE;
    // C++ type (as emitted by cType) is trivially default constructible and destructible?
    bool isTriviallyConstructible() ಧ್ರುವ VL_MT_STABLE;
    ಆಭಾಸ bool isDynamicallySized() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ false; }

ಗುಪ್ತ:
    class CTypeRecursed;
    CTypeRecursed cTypeRecurse(bool compound, bool packed) ಧ್ರುವ;
};
class Ast_ಗಂಟು_Array_DType VL_NOT_FINAL : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_DType {
    // Array data type, ie "some_dtype var_name [2:0]"
    // @astgen op1 := childDTypep : Optional[AstNodeDType] // moved to refDTypep() in V3Width
    // @astgen op2 := rangep : Optional[AstRange] // array bounds
    //
    // @astgen ptr := m_refDTypep : Optional[AstNodeDType]  // Elements of this type (post-width)
    Ast_ಗಂಟು* rangenp() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ reinterpret_cast<Ast_ಗಂಟು*>(rangep()); }

ರಕ್ಷಿತ:
    Ast_ಗಂಟು_Array_DType(VNType t, File_ಸಾಲು* fl)
        : Ast_ಗಂಟು_DType{t, fl} {}

ಸಾರ್ವಜನಿಕ:
    ASTGEN_MEMBERS_Ast_ಗಂಟು_Array_DType;
    ಶೂನ್ಯ dump(std::ostream& str) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpSmall(std::ostream& str) ಧ್ರುವ override;
    ಧ್ರುವ ವರ್ಣ* broken() ಧ್ರುವ override {
        BROKEN_RTN(!((m_refDTypep && !childDTypep()) || (!m_refDTypep && childDTypep())));
        ಪ್ರತ್ಯಾಗಮ nullptr;
    }
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* samep) ಧ್ರುವ override {
        ಧ್ರುವ Ast_ಗಂಟು_Array_DType* ಧ್ರುವ asamep = VN_DBG_AS(samep, ಗಂಟು_Array_DType);
        ಪ್ರತ್ಯಾಗಮ hi() == asamep->hi() && rangenp()->same_ವೃಕ್ಷ(asamep->rangenp())
               && subDTypep() == asamep->subDTypep();
    }  // HashedDT doesn't recurse, so need to check children
    bool similar_DType_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು_DType* samep) ಧ್ರುವ override {
        ಧ್ರುವ Ast_ಗಂಟು_Array_DType* ಧ್ರುವ asamep = VN_DBG_AS(samep, ಗಂಟು_Array_DType);
        ಪ್ರತ್ಯಾಗಮ elementsConst() == asamep->elementsConst()
               && subDTypep()->similarDType(asamep->subDTypep());
    }
    Ast_ಗಂಟು_DType* getChildDTypep() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ childDTypep(); }
    Ast_ಗಂಟು_DType* subDTypep() ಧ್ರುವ override VL_MT_STABLE {
        ಪ್ರತ್ಯಾಗಮ m_refDTypep ? m_refDTypep : childDTypep();
    }
    ಶೂನ್ಯ refDTypep(Ast_ಗಂಟು_DType* nodep) { m_refDTypep = nodep; }
    Ast_ಗಂಟು_DType* virtRefDTypep() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ m_refDTypep; }
    ಶೂನ್ಯ virtRefDTypep(Ast_ಗಂಟು_DType* nodep) override { refDTypep(nodep); }
    // METHODS
    AstBasicDType* basicp() ಧ್ರುವ override VL_MT_STABLE {
        ಪ್ರತ್ಯಾಗಮ subDTypep()->basicp();
    }  // (Slow) recurse down to find basic data type
    ಪೂರ್ಣಾಙ್ಕ widthAlignBytes() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ subDTypep()->widthAlignBytes(); }
    ಪೂರ್ಣಾಙ್ಕ widthTotalBytes() ಧ್ರುವ override {
        ಪ್ರತ್ಯಾಗಮ elementsConst() * subDTypep()->widthTotalBytes();
    }
    ಅನ್ತಃಸ್ಥ ಪೂರ್ಣಾಙ್ಕ ಎಡ() ಧ್ರುವ VL_MT_STABLE;
    ಅನ್ತಃಸ್ಥ ಪೂರ್ಣಾಙ್ಕ ಬಲ() ಧ್ರುವ VL_MT_STABLE;
    ಅನ್ತಃಸ್ಥ ಪೂರ್ಣಾಙ್ಕ hi() ಧ್ರುವ VL_MT_STABLE;
    ಅನ್ತಃಸ್ಥ ಪೂರ್ಣಾಙ್ಕ lo() ಧ್ರುವ VL_MT_STABLE;
    ಅನ್ತಃಸ್ಥ ಪೂರ್ಣಾಙ್ಕ elementsConst() ಧ್ರುವ VL_MT_STABLE;
    ಅನ್ತಃಸ್ಥ VNumRange declRange() ಧ್ರುವ VL_MT_STABLE;
};
class Ast_ಗಂಟು_UOr_Struct_DType VL_NOT_FINAL : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_DType {
    // A struct or union; common handling
    // @astgen op1 := membersp : List[AstMemberDType]
    //
    // @astgen ptr := m_classOrPackagep : Optional[AstNodeModule]  // Package emitted with
    string m_name;  // Name from upper typedef, if any
    string m_typedefName;  // Typedef's dtypeName() once moved off the typedef declaring it
    ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ m_unique_ಅಂಕೆ;  // Unique ID distinguishing this dtype instance, for hashing/naming
    bool m_packed;  // Packed struct/union, else unpacked
    bool m_isFourstate = false;  // V3Width computes; true if any member is 4-state
    bool m_constrainedRand = false;  // True if struct has constraint expression
    bool m_emitToString = false;  // Generate to_string() for this struct/union if set

ರಕ್ಷಿತ:
    Ast_ಗಂಟು_UOr_Struct_DType(VNType t, File_ಸಾಲು* fl, VSigning numericUnpack)
        : Ast_ಗಂಟು_DType{t, fl}
        , m_unique_ಅಂಕೆ{unique_ಅಂಕೆ_Inc()} {
        // VSigning::NOSIGN overloaded to indicate not packed
        m_packed = (numericUnpack != VSigning::NOSIGN);
        numeric(VSigning::fromBool(numericUnpack.isSigned()));
    }
    Ast_ಗಂಟು_UOr_Struct_DType(ಧ್ರುವ Ast_ಗಂಟು_UOr_Struct_DType& other)
        : Ast_ಗಂಟು_DType{other}
        , m_name{other.m_name}
        , m_typedefName{other.m_typedefName}
        , m_unique_ಅಂಕೆ{unique_ಅಂಕೆ_Inc()}
        , m_packed{other.m_packed}
        , m_isFourstate{other.m_isFourstate} {}

ಸಾರ್ವಜನಿಕ:
    ASTGEN_MEMBERS_Ast_ಗಂಟು_UOr_Struct_DType;
    ಪೂರ್ಣಾಙ್ಕ unique_ಅಂಕೆ() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_unique_ಅಂಕೆ; }
    ಶೂನ್ಯ dump(std::ostream& str) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಧ್ರುವ override;
    string prettyDTypeName(bool) ಧ್ರುವ override;
    bool isCompound() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ !packed(); }
    // For basicp() we reuse the size to indicate a "fake" basic type of same size
    AstBasicDType* basicp() ಧ್ರುವ override VL_MT_STABLE {
        ಯದಿ (!m_packed) ಪ್ರತ್ಯಾಗಮ nullptr;
        ಪ್ರತ್ಯಾಗಮ (isFourstate()
                    ? VN_AS(findLogicRangeDType(VNumRange{width() - 1, 0}, width(), numeric()),
                            BasicDType)
                    : VN_AS(findBitRangeDType(VNumRange{width() - 1, 0}, width(), numeric()),
                            BasicDType));
    }
    // (Slow) recurses - Structure alignment 1,2,4 or 8 bytes (arrays affect this)
    ಪೂರ್ಣಾಙ್ಕ widthAlignBytes() ಧ್ರುವ override;
    // (Slow) recurses - Width in bytes rounding up 1,2,4,8,12,...
    ಪೂರ್ಣಾಙ್ಕ widthTotalBytes() ಧ್ರುವ override;
    bool similar_DType_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು_DType* samep) ಧ್ರುವ override;
    string name() ಧ್ರುವ override VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ m_name; }
    ಶೂನ್ಯ name(ಧ್ರುವ string& flag) override { m_name = flag; }
    // Keep the name the typedef declaring it gives, as it moves off it to the type table
    ಶೂನ್ಯ typedefName(ಧ್ರುವ string& name) { m_typedefName = name; }
    // Name of the C structure for DPI, empty if no typedef names it, so declared where used
    string dpiName() ಧ್ರುವ;
    // C typedef of the unpacked structure for DPI (IEEE 1800-2023 H.7.3)
    string dpiTypedef() ಧ್ರುವ;
    bool packed() ಧ್ರುವ VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ m_packed; }
    ಶೂನ್ಯ packed(bool flag) { m_packed = flag; }
    // packed() but as don't support unpacked, presently all structs
    ಸ್ಥಿರ bool packedUnsup() { ಪ್ರತ್ಯಾಗಮ true; }
    ಶೂನ್ಯ isFourstate(bool flag) { m_isFourstate = flag; }
    bool isFourstate() ಧ್ರುವ override VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ m_isFourstate; }
    ಸ್ಥಿರ ಪೂರ್ಣಾಙ್ಕ lo() VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ 0; }
    ಪೂರ್ಣಾಙ್ಕ hi() ಧ್ರುವ VL_MT_STABLE {
        ಪ್ರತ್ಯಾಗಮ dtypep()->width() - 1;
    }  // Packed classes look like arrays
    VNumRange declRange() ಧ್ರುವ VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ VNumRange{hi(), lo()}; }
    Ast_ಗಂಟು_Module* classOrPackagep() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_classOrPackagep; }
    ಶೂನ್ಯ classOrPackagep(Ast_ಗಂಟು_Module* classpackagep) { m_classOrPackagep = classpackagep; }
    bool isConstrainedRand() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_constrainedRand; }
    ಶೂನ್ಯ markConstrainedRand(bool flag) { m_constrainedRand = flag; }
    bool emitToString() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_emitToString; }
    ಶೂನ್ಯ setEmitToString() { m_emitToString = true; }
    bool isAggregateType() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ !packed(); }
};

// === Concrete node types =====================================================

// === AstNode ===
class AstEnumItem final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು {
    // @astgen op1 := rangep : Optional[AstRange] // Range for name appending
    // @astgen op2 := valuep : Optional[AstNodeExpr]
    string m_name;  // Name of item

ಸಾರ್ವಜನಿಕ:
    // Parents: ENUM
    AstEnumItem(File_ಸಾಲು* fl, ಧ್ರುವ string& name, AstRange* rangep, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* valuep)
        : ASTGEN_SUPER_EnumItem(fl)
        , m_name{name} {
        this->rangep(rangep);
        this->valuep(valuep);
    }
    ASTGEN_MEMBERS_AstEnumItem;
    string name() ಧ್ರುವ override VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ m_name; }
    bool maybePointedTo() ಧ್ರುವ override VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ true; }
    bool hasDType() ಧ್ರುವ override VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ true; }
    ಶೂನ್ಯ name(ಧ್ರುವ string& flag) override { m_name = flag; }
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
    AstAssocArrayDType(File_ಸಾಲು* fl, VFlagChildDType, Ast_ಗಂಟು_DType* dtp, Ast_ಗಂಟು_DType* ಕೀಲಿ_Dtp)
        : ASTGEN_SUPER_AssocArrayDType(fl) {
        childDTypep(dtp);  // Only for parser
        ಕೀಲಿ_Child_DTypep(ಕೀಲಿ_Dtp);  // Only for parser
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
    ಧ್ರುವ ವರ್ಣ* broken() ಧ್ರುವ override {
        BROKEN_RTN(!((m_refDTypep && !childDTypep()) || (!m_refDTypep && childDTypep())));
        BROKEN_RTN(!((m_ಕೀಲಿ_DTypep && !childDTypep()) || (!m_ಕೀಲಿ_DTypep && childDTypep())));
        ಪ್ರತ್ಯಾಗಮ nullptr;
    }
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* samep) ಧ್ರುವ override {
        ಧ್ರುವ AstAssocArrayDType* ಧ್ರುವ asamep = VN_DBG_AS(samep, AssocArrayDType);
        ಯದಿ (!asamep->subDTypep()) ಪ್ರತ್ಯಾಗಮ false;
        ಯದಿ (!asamep->ಕೀಲಿ_DTypep()) ಪ್ರತ್ಯಾಗಮ false;
        ಪ್ರತ್ಯಾಗಮ (subDTypep() == asamep->subDTypep() && ಕೀಲಿ_DTypep() == asamep->ಕೀಲಿ_DTypep());
    }
    bool similar_DType_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು_DType* samep) ಧ್ರುವ override {
        ಧ್ರುವ AstAssocArrayDType* ಧ್ರುವ asamep = VN_DBG_AS(samep, AssocArrayDType);
        ಪ್ರತ್ಯಾಗಮ asamep->subDTypep() && subDTypep()->similarDType(asamep->subDTypep())
               && asamep->ಕೀಲಿ_DTypep() && ಕೀಲಿ_DTypep()->similarDType(asamep->ಕೀಲಿ_DTypep());
    }
    string prettyDTypeName(bool full) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpSmall(std::ostream& str) ಧ್ರುವ override;
    Ast_ಗಂಟು_DType* getChildDTypep() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ childDTypep(); }
    Ast_ಗಂಟು_DType* getChild2DTypep() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ ಕೀಲಿ_Child_DTypep(); }
    Ast_ಗಂಟು_DType* subDTypep() ಧ್ರುವ override VL_MT_STABLE {
        ಪ್ರತ್ಯಾಗಮ m_refDTypep ? m_refDTypep : childDTypep();
    }
    Ast_ಗಂಟು_DType* subDType2p() ಧ್ರುವ override VL_MT_STABLE {
        ಪ್ರತ್ಯಾಗಮ m_ಕೀಲಿ_DTypep ? m_ಕೀಲಿ_DTypep : ಕೀಲಿ_Child_DTypep();
    }
    bool isAggregateType() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    ಶೂನ್ಯ refDTypep(Ast_ಗಂಟು_DType* nodep) { m_refDTypep = nodep; }
    Ast_ಗಂಟು_DType* virtRefDTypep() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ m_refDTypep; }
    ಶೂನ್ಯ virtRefDTypep(Ast_ಗಂಟು_DType* nodep) override { refDTypep(nodep); }
    Ast_ಗಂಟು_DType* virtRefDType2p() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ m_ಕೀಲಿ_DTypep; }
    ಶೂನ್ಯ virtRefDType2p(Ast_ಗಂಟು_DType* nodep) override { ಕೀಲಿ_DTypep(nodep); }
    //
    Ast_ಗಂಟು_DType* ಕೀಲಿ_DTypep() ಧ್ರುವ VL_MT_STABLE {
        ಪ್ರತ್ಯಾಗಮ m_ಕೀಲಿ_DTypep ? m_ಕೀಲಿ_DTypep : ಕೀಲಿ_Child_DTypep();
    }
    ಶೂನ್ಯ ಕೀಲಿ_DTypep(Ast_ಗಂಟು_DType* nodep) { m_ಕೀಲಿ_DTypep = nodep; }
    // METHODS
    AstBasicDType* basicp() ಧ್ರುವ override VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ nullptr; }
    ಪೂರ್ಣಾಙ್ಕ widthAlignBytes() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ subDTypep()->widthAlignBytes(); }
    ಪೂರ್ಣಾಙ್ಕ widthTotalBytes() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ subDTypep()->widthTotalBytes(); }
    bool isCompound() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool isDynamicallySized() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
};
class AstBasicDType final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_DType {
    // Builtin atomic/vectored data type
    // @astgen op1 := rangep : Optional[AstRange] // Range of variable
    ಸಂರಚನಾ Members final {
        VBasicDTypeKwd m_keyword;  // (also in VBasicTypeKey) What keyword created basic type
        // dist-ast-dump-suppress  // Part of dumpSmall
        VNumRange m_nrange;  // (also in VBasicTypeKey) Numeric msb/lsb (if non-opaque keyword)
        bool operator==(ಧ್ರುವ Members& rhs) ಧ್ರುವ {
            ಪ್ರತ್ಯಾಗಮ rhs.m_keyword == m_keyword && rhs.m_nrange == m_nrange;
        }
    } m;
    // See also in AstNodeDType: m_width, m_widthMin, m_numeric(issigned)
ಸಾರ್ವಜನಿಕ:
    AstBasicDType(File_ಸಾಲು* fl, VBasicDTypeKwd kwd, ಧ್ರುವ VSigning& signst = VSigning::NOSIGN)
        : ASTGEN_SUPER_BasicDType(fl) {
        init(kwd, signst, 0, -1, nullptr);
    }
    AstBasicDType(File_ಸಾಲು* fl, VFlagLogicPacked, ಪೂರ್ಣಾಙ್ಕ wantwidth)
        : ASTGEN_SUPER_BasicDType(fl) {
        init(VBasicDTypeKwd::LOGIC, VSigning::NOSIGN, wantwidth, -1, nullptr);
    }
    AstBasicDType(File_ಸಾಲು* fl, VFlagLogic2StatePacked, ಪೂರ್ಣಾಙ್ಕ wantwidth)
        : ASTGEN_SUPER_BasicDType(fl) {
        init(VBasicDTypeKwd::LOGIC2STATE, VSigning::NOSIGN, wantwidth, -1, nullptr);
    }
    AstBasicDType(File_ಸಾಲು* fl, VFlagBitPacked, ಪೂರ್ಣಾಙ್ಕ wantwidth)
        : ASTGEN_SUPER_BasicDType(fl) {
        init(VBasicDTypeKwd::BIT, VSigning::NOSIGN, wantwidth, -1, nullptr);
    }
    AstBasicDType(File_ಸಾಲು* fl, VBasicDTypeKwd kwd, VSigning numer, ಪೂರ್ಣಾಙ್ಕ wantwidth, ಪೂರ್ಣಾಙ್ಕ widthmin)
        : ASTGEN_SUPER_BasicDType(fl) {
        init(kwd, numer, wantwidth, widthmin, nullptr);
    }
    AstBasicDType(File_ಸಾಲು* fl, VBasicDTypeKwd kwd, VSigning numer, VNumRange range, ಪೂರ್ಣಾಙ್ಕ widthmin)
        : ASTGEN_SUPER_BasicDType(fl) {
        init(kwd, numer, range.elements(), widthmin, nullptr);
        m.m_nrange = range;  // as init() presumes lsb==0, but range.lsb() might not be
    }
    // See also addRange in verilog.y
ಗುಪ್ತ:
    ಶೂನ್ಯ init(VBasicDTypeKwd kwd, VSigning numer, ಪೂರ್ಣಾಙ್ಕ wantwidth, ಪೂರ್ಣಾಙ್ಕ wantwidthmin,
              AstRange* rangep);

ಸಾರ್ವಜನಿಕ:
    ASTGEN_MEMBERS_AstBasicDType;
    ಶೂನ್ಯ dump(std::ostream& str) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಧ್ರುವ override;
    // width/widthMin/numeric compared elsewhere
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* samep) ಧ್ರುವ override;
    bool similar_DType_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು_DType* samep) ಧ್ರುವ override;
    string name() ಧ್ರುವ override VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ m.m_keyword.typeName(); }
    string prettyDTypeName(bool full) ಧ್ರುವ override;
    ಧ್ರುವ ವರ್ಣ* broken() ಧ್ರುವ override {
        BROKEN_RTN(dtypep() != this);
        BROKEN_RTN(v3Global.widthMinUsage() == VWidthMinUsage::VERILOG_WIDTH
                   && widthMin() > width());
        ಪ್ರತ್ಯಾಗಮ nullptr;
    }
    ಶೂನ್ಯ setSignedState(ಧ್ರುವ VSigning& signst) {
        // Note NOSIGN does NOT change the state; this is required by the parser
        ಯದಿ (signst == VSigning::UNSIGNED) {
            numeric(signst);
        } ಅನ್ಯಥಾ ಯದಿ (signst == VSigning::SIGNED) {
            numeric(signst);
        }
    }
    // METHODS
    AstBasicDType* basicp() ಧ್ರುವ override VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ (AstBasicDType*)this; }
    // (Slow) recurses - Structure alignment 1,2,4 or 8 bytes (arrays affect this)
    ಪೂರ್ಣಾಙ್ಕ widthAlignBytes() ಧ್ರುವ override;
    // (Slow) recurses - Width in bytes rounding up 1,2,4,8,12,...
    ಪೂರ್ಣಾಙ್ಕ widthTotalBytes() ಧ್ರುವ override;
    bool isFourstate() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ keyword().isFourstate(); }
    VBasicDTypeKwd keyword() ಧ್ರುವ VL_MT_SAFE {  // Avoid using - use isSomething accessors instead
        ಪ್ರತ್ಯಾಗಮ m.m_keyword;
    }
    bool isBitLogic() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ keyword().isBitLogic(); }
    bool isBit() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ keyword().isBit(); }
    bool isCHandle() ಧ್ರುವ VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ keyword().isCHandle(); }
    bool isDouble() ಧ್ರುವ VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ keyword().isDouble(); }
    bool isEvent() ಧ್ರುವ VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ keyword() == VBasicDTypeKwd::EVENT; }
    bool isForkSync() ಧ್ರುವ VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ keyword() == VBasicDTypeKwd::FORK_SYNC; }
    bool isProcessRef() ಧ್ರುವ VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ keyword() == VBasicDTypeKwd::PROCESS_REFERENCE; }
    bool isDelayScheduler() ಧ್ರುವ VL_MT_SAFE {
        ಪ್ರತ್ಯಾಗಮ keyword() == VBasicDTypeKwd::DELAY_SCHEDULER;
    }
    bool isTriggerScheduler() ಧ್ರುವ VL_MT_SAFE {
        ಪ್ರತ್ಯಾಗಮ keyword() == VBasicDTypeKwd::TRIGGER_SCHEDULER;
    }
    bool isDynamicTriggerScheduler() ಧ್ರುವ VL_MT_SAFE {
        ಪ್ರತ್ಯಾಗಮ keyword() == VBasicDTypeKwd::DYNAMIC_TRIGGER_SCHEDULER;
    }
    bool isRandomGenerator() ಧ್ರುವ VL_MT_SAFE {
        ಪ್ರತ್ಯಾಗಮ keyword() == VBasicDTypeKwd::RANDOM_GENERATOR;
    }
    bool isStdRandomGenerator() ಧ್ರುವ VL_MT_SAFE {
        ಪ್ರತ್ಯಾಗಮ keyword() == VBasicDTypeKwd::RANDOM_STDGENERATOR;
    }
    bool isCovergroupInstHandle() ಧ್ರುವ VL_MT_SAFE {
        ಪ್ರತ್ಯಾಗಮ keyword() == VBasicDTypeKwd::COVERGROUP_INSTHANDLE;
    }
    bool isOpaque() ಧ್ರುವ VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ keyword().isOpaque(); }
    bool isString() ಧ್ರುವ VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ keyword().isString(); }
    bool isZeroInit() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ keyword().isZeroInit(); }
    bool isRanged() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ rangep() || m.m_nrange.ranged(); }
    bool isDpiBitVec() ಧ್ರುವ {  // DPI uses svBitVecVal
        ಪ್ರತ್ಯಾಗಮ keyword() == VBasicDTypeKwd::BIT && isRanged();
    }
    bool isDpiLogicVec() ಧ್ರುವ {  // DPI uses svLogicVecVal
        ಪ್ರತ್ಯಾಗಮ keyword().isFourstate() && !(keyword() == VBasicDTypeKwd::LOGIC && !isRanged());
    }
    bool isDpiCLayout() ಧ್ರುವ {
        ಪ್ರತ್ಯಾಗಮ keyword().isDpiCLayout()
               || (keyword().isSameish(VBasicDTypeKwd::LOGIC) && !isRanged());
    }
    bool isDpiPrimitive() ಧ್ರುವ {  // DPI uses a primitive type
        ಪ್ರತ್ಯಾಗಮ !isDpiBitVec() && !isDpiLogicVec();
    }
    // Generally the lo/hi/left/right funcs should be used instead of nrange()
    ಧ್ರುವ VNumRange& nrange() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m.m_nrange; }
    ಅನ್ತಃಸ್ಥ ಪೂರ್ಣಾಙ್ಕ hi() ಧ್ರುವ;
    ಅನ್ತಃಸ್ಥ ಪೂರ್ಣಾಙ್ಕ lo() ಧ್ರುವ;
    ಅನ್ತಃಸ್ಥ ಪೂರ್ಣಾಙ್ಕ elements() ಧ್ರುವ;
    ಪೂರ್ಣಾಙ್ಕ ಎಡ() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ ascending() ? lo() : hi(); }  // How to show a declaration
    ಪೂರ್ಣಾಙ್ಕ ಬಲ() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ ascending() ? hi() : lo(); }
    ಅನ್ತಃಸ್ಥ bool ascending() ಧ್ರುವ;
    bool implicit() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ keyword() == VBasicDTypeKwd::LOGIC_IMPLICIT; }
    bool untyped() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ keyword() == VBasicDTypeKwd::UNTYPED; }
    VNumRange declRange() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ isRanged() ? VNumRange{ಎಡ(), ಬಲ()} : VNumRange{}; }
    ಶೂನ್ಯ cvtRangeConst();  // Convert to smaller representation
    bool isCompound() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ isString(); }
    bool isIntegralOrPacked() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ keyword().isIntNumeric(); }
};
class AstBracketArrayDType final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_DType {
    // Associative/Queue/Normal array data type, ie "[dtype_or_expr]"
    // only for early parsing then becomes another data type
    // @astgen op1 := childDTypep : Optional[AstNodeDType] // moved to refDTypep() in V3Width
    // @astgen op2 := elementsp : AstNode  // Number of elements in array
ಸಾರ್ವಜನಿಕ:
    AstBracketArrayDType(File_ಸಾಲು* fl, VFlagChildDType, Ast_ಗಂಟು_DType* childDTypep,
                         Ast_ಗಂಟು* elementsp)
        : ASTGEN_SUPER_BracketArrayDType(fl) {
        this->childDTypep(childDTypep);
        this->elementsp(elementsp);
    }
    ASTGEN_MEMBERS_AstBracketArrayDType;
    bool similar_DType_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು_DType* samep) ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ this == samep; }
    Ast_ಗಂಟು_DType* subDTypep() ಧ್ರುವ override VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ childDTypep(); }
    // METHODS
    // Will be removed in V3Width, which relies on this
    // being a child not a dtype pointed node
    bool maybePointedTo() ಧ್ರುವ override VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ false; }
    AstBasicDType* basicp() ಧ್ರುವ override VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ nullptr; }
    ಪೂರ್ಣಾಙ್ಕ widthAlignBytes() ಧ್ರುವ override { V3ERROR_NA_RETURN(0); }
    ಪೂರ್ಣಾಙ್ಕ widthTotalBytes() ಧ್ರುವ override { V3ERROR_NA_RETURN(0); }
    bool isCompound() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
};
class AstCDType final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_DType {
    // Raw "C" data type passed directly to output
    string m_name;  // Name of data type, printed when do V3EmitC
ಸಾರ್ವಜನಿಕ:
    AstCDType(File_ಸಾಲು* fl, ಧ್ರುವ string& name)
        : ASTGEN_SUPER_CDType(fl)
        , m_name{name} {
        dtypep(this);
    }

    ASTGEN_MEMBERS_AstCDType;
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* samep) ಧ್ರುವ override {
        ಧ್ರುವ AstCDType* ಧ್ರುವ asamep = VN_DBG_AS(samep, CDType);
        ಪ್ರತ್ಯಾಗಮ m_name == asamep->m_name;
    }
    bool similar_DType_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು_DType* samep) ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ same_ಗಂಟು(samep); }
    string name() ಧ್ರುವ override VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ m_name; }
    string prettyDTypeName(bool) ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ m_name; }
    // METHODS
    AstBasicDType* basicp() ಧ್ರುವ override VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ nullptr; }
    ಪೂರ್ಣಾಙ್ಕ widthAlignBytes() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ 8; }  // Assume
    ಪೂರ್ಣಾಙ್ಕ widthTotalBytes() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ 8; }  // Assume
    bool isCompound() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    // Return the global runtime type storing the requested number of bits.
    ಸ್ಥಿರ string typeToHold(ಪೂರ್ಣಾಙ್ಕ width) {
        ಯದಿ (width <= 8) ಪ್ರತ್ಯಾಗಮ "::CData";
        ಯದಿ (width <= 16) ಪ್ರತ್ಯಾಗಮ "::SData";
        ಯದಿ (width <= VL_IDATASIZE) ಪ್ರತ್ಯಾಗಮ "::IData";
        ಯದಿ (width <= VL_QUADSIZE) ಪ್ರತ್ಯಾಗಮ "::QData";

        ಪ್ರತ್ಯಾಗಮ "::VlWide<" + std::to_string(VL_ಪದಗಳು_I(width)) + ">";
    }
};
class AstClassRefDType final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_DType {
    // Reference to a class
    // @astgen op1 := paramsp: List[AstPin]
    //
    // @astgen ptr := m_classp : Optional[AstClass]  // data type pointed to, BELOW the AstTypedef
    // @astgen ptr := m_classOrPackagep : Optional[AstNodeModule]  // Package hierarchy
    bool m_rawPointer = false;  // Emit as a non-owning C++ pointer rather than VlClassRef
ಸಾರ್ವಜನಿಕ:
    AstClassRefDType(File_ಸಾಲು* fl, AstClass* classp, AstPin* paramsp)
        : ASTGEN_SUPER_ClassRefDType(fl)
        , m_classp{classp} {
        dtypep(this);
        addParamsp(paramsp);
    }
    ASTGEN_MEMBERS_AstClassRefDType;
    // METHODS
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* samep) ಧ್ರುವ override {
        ಧ್ರುವ AstClassRefDType* ಧ್ರುವ asamep = VN_DBG_AS(samep, ClassRefDType);
        ಪ್ರತ್ಯಾಗಮ (m_classp == asamep->m_classp && m_classOrPackagep == asamep->m_classOrPackagep
                && m_rawPointer == asamep->m_rawPointer);
    }
    bool similar_DType_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು_DType* samep) ಧ್ರುವ override;
    ಶೂನ್ಯ dump(std::ostream& str = std::cout) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str = std::cout) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpSmall(std::ostream& str) ಧ್ರುವ override;
    string prettyDTypeName(bool full) ಧ್ರುವ override;
    string pretty_Name_ಸಂದೇಶ() ಧ್ರುವ override;
    string name() ಧ್ರುವ override VL_MT_STABLE;
    AstBasicDType* basicp() ಧ್ರುವ override VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ nullptr; }
    ಪೂರ್ಣಾಙ್ಕ widthAlignBytes() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ 0; }
    ಪೂರ್ಣಾಙ್ಕ widthTotalBytes() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ 0; }
    Ast_ಗಂಟು_DType* virtRefDTypep() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ nullptr; }
    ಶೂನ್ಯ virtRefDTypep(Ast_ಗಂಟು_DType* nodep) override {}
    Ast_ಗಂಟು_DType* subDTypep() ಧ್ರುವ override VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ nullptr; }
    Ast_ಗಂಟು_Module* classOrPackagep() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_classOrPackagep; }
    ಶೂನ್ಯ classOrPackagep(Ast_ಗಂಟು_Module* nodep) { m_classOrPackagep = nodep; }
    AstClass* classp() ಧ್ರುವ VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ m_classp; }
    ಶೂನ್ಯ classp(AstClass* nodep) { m_classp = nodep; }
    bool rawPointer() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_rawPointer; }
    ಶೂನ್ಯ rawPointer(bool flag) { m_rawPointer = flag; }
    ಸ್ಥಿರ ಶೂನ್ಯ selfTest();
    bool isCompound() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
};
class AstConstDType final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_DType {
    // const data type, ie "const some_dtype var_name [2:0]"
    // ConstDType are removed in V3LinkLValue and become AstVar::isConst.
    // When more generic types are supported AstConstDType will be propagated further.
    // @astgen op1 := childDTypep : Optional[AstNodeDType]
    //
    // @astgen ptr := m_refDTypep : Optional[AstNodeDType]  // Inherit from this base data type
ಸಾರ್ವಜನಿಕ:
    AstConstDType(File_ಸಾಲು* fl, VFlagChildDType, Ast_ಗಂಟು_DType* dtp)
        : ASTGEN_SUPER_ConstDType(fl) {
        childDTypep(dtp);  // Only for parser
        refDTypep(nullptr);  // V3Width will resolve
        dtypep(nullptr);  // V3Width will resolve
        widthFromSub(subDTypep());
    }
    ASTGEN_MEMBERS_AstConstDType;
    ಧ್ರುವ ವರ್ಣ* broken() ಧ್ರುವ override {
        BROKEN_RTN(!((m_refDTypep && !childDTypep()) || (!m_refDTypep && childDTypep())));
        ಪ್ರತ್ಯಾಗಮ nullptr;
    }
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* samep) ಧ್ರುವ override {
        ಧ್ರುವ AstConstDType* ಧ್ರುವ sp = VN_DBG_AS(samep, ConstDType);
        ಪ್ರತ್ಯಾಗಮ (m_refDTypep == sp->m_refDTypep);
    }
    bool similar_DType_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು_DType* samep) ಧ್ರುವ override {
        ಪ್ರತ್ಯಾಗಮ skipRefp()->similarDType(samep->skipRefp());
    }
    Ast_ಗಂಟು_DType* getChildDTypep() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ childDTypep(); }
    Ast_ಗಂಟು_DType* subDTypep() ಧ್ರುವ override VL_MT_STABLE {
        ಪ್ರತ್ಯಾಗಮ m_refDTypep ? m_refDTypep : childDTypep();
    }
    ಶೂನ್ಯ refDTypep(Ast_ಗಂಟು_DType* nodep) { m_refDTypep = nodep; }
    Ast_ಗಂಟು_DType* virtRefDTypep() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ m_refDTypep; }
    ಶೂನ್ಯ virtRefDTypep(Ast_ಗಂಟು_DType* nodep) override { refDTypep(nodep); }
    // METHODS
    AstBasicDType* basicp() ಧ್ರುವ override VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ subDTypep()->basicp(); }
    ಪೂರ್ಣಾಙ್ಕ widthAlignBytes() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ subDTypep()->widthAlignBytes(); }
    ಪೂರ್ಣಾಙ್ಕ widthTotalBytes() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ subDTypep()->widthTotalBytes(); }
    bool isCompound() ಧ್ರುವ override {
        v3fatalSrc("call isCompound on subdata type, not reference");
        ಪ್ರತ್ಯಾಗಮ false;
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
    bool hasDType() ಧ್ರುವ override VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ true; }
    bool maybePointedTo() ಧ್ರುವ override VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ true; }
    bool undead() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    Ast_ಗಂಟು_DType* subDTypep() ಧ್ರುವ override VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ nullptr; }
    Ast_ಗಂಟು_DType* virtRefDTypep() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ nullptr; }
    ಶೂನ್ಯ virtRefDTypep(Ast_ಗಂಟು_DType* nodep) override {}
    bool similar_DType_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು_DType* samep) ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ this == samep; }
    AstBasicDType* basicp() ಧ್ರುವ override VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ nullptr; }
    ಪೂರ್ಣಾಙ್ಕ widthAlignBytes() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ 1; }
    ಪೂರ್ಣಾಙ್ಕ widthTotalBytes() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ 1; }
    bool isCompound() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
};
class AstCoverCrossDType final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_DType {
    // Borrowed pointer to VlCoverCrossT<...> or construction-time VlCoverCrossDyn.
    ಧ್ರುವ uint32_t m_dimensions;
    ಧ್ರುವ uint32_t m_tuples;  // Fixed capacity number of cross tuples (VlCoverCrossT's Tuples)
    ಧ್ರುವ uint32_t m_bins;  // Fixed capacity number of explicit bins (VlCoverCrossT's Bins)
    ಧ್ರುವ uint32_t m_autoBins;  // Fixed capacity number of auto bins (VlCoverCrossT's AutoBins)
    ಧ್ರುವ uint64_t m_bin_ಪದಗಳು;  // Fixed capacity selection words (VlCoverCrossT's BinWords)
    ಧ್ರುವ bool m_dynamic;  // Bin layout is determined at covergroup construction

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
    ಧ್ರುವ ವರ್ಣ* broken() ಧ್ರುವ override {
        BROKEN_RTN(m_dimensions == 0);
        ಪ್ರತ್ಯಾಗಮ nullptr;
    }
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* samep) ಧ್ರುವ override {  // LCOV_EXCL_START
        ಧ್ರುವ AstCoverCrossDType* ಧ್ರುವ sp = VN_DBG_AS(samep, CoverCrossDType);
        ಪ್ರತ್ಯಾಗಮ dimensions() == sp->dimensions() && tuples() == sp->tuples() && bins() == sp->bins()
               && autoBins() == sp->autoBins() && bin_ಪದಗಳು() == sp->bin_ಪದಗಳು()
               && isDynamic() == sp->isDynamic();
    }  // LCOV_EXCL_STOP
    bool similar_DType_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು_DType* samep) ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ this == samep; }
    ಶೂನ್ಯ dump(std::ostream& str) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpSmall(std::ostream& str) ಧ್ರುವ override;
    uint32_t dimensions() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_dimensions; }
    uint32_t tuples() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_tuples; }
    uint32_t bins() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_bins; }
    uint32_t autoBins() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_autoBins; }
    uint64_t bin_ಪದಗಳು() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_bin_ಪದಗಳು; }
    bool isDynamic() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_dynamic; }
    string cppTemplateArgs() ಧ್ರುವ;
    AstBasicDType* basicp() ಧ್ರುವ override VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ nullptr; }
    ಪೂರ್ಣಾಙ್ಕ widthAlignBytes() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ ಪರಿಮಾಣ(ಶೂನ್ಯ*); }
    ಪೂರ್ಣಾಙ್ಕ widthTotalBytes() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ ಪರಿಮಾಣ(ಶೂನ್ಯ*); }
    bool isCompound() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
};
class AstCoverpointDType final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_DType {
    // Borrowed pointer to a covergroup coverpoint runtime, 'VlCoverpointT<hitBound>*'.
    // Follows pattern of AstQueueDType in capturing the compile-time max bin overlap
    // template argument.
    uint32_t m_hitBound;  // VlCoverpointT<> template argument; hit list size, >= 1
ಸಾರ್ವಜನಿಕ:
    AstCoverpointDType(File_ಸಾಲು* fl, uint32_t hitBound)
        : ASTGEN_SUPER_CoverpointDType(fl)
        , m_hitBound{hitBound} {
        dtypep(this);
    }
    ASTGEN_MEMBERS_AstCoverpointDType;
    ಧ್ರುವ ವರ್ಣ* broken() ಧ್ರುವ override {
        BROKEN_RTN(m_hitBound < 1);
        ಪ್ರತ್ಯಾಗಮ nullptr;
    }
    // V3Covergroup interns these one-per-hitBound into the type table, so identity is
    // equality; there is never a second node with the same bound to compare against.
    bool similar_DType_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು_DType* samep) ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ this == samep; }
    ಶೂನ್ಯ dumpSmall(std::ostream& str) ಧ್ರುವ override;
    // ACCESSORS
    uint32_t hitBound() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_hitBound; }
    // METHODS
    AstBasicDType* basicp() ಧ್ರುವ override VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ nullptr; }
    ಪೂರ್ಣಾಙ್ಕ widthAlignBytes() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ ಪರಿಮಾಣ(ಶೂನ್ಯ*); }
    ಪೂರ್ಣಾಙ್ಕ widthTotalBytes() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ ಪರಿಮಾಣ(ಶೂನ್ಯ*); }
    bool isCompound() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
};
class AstDefImplicitDType final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_DType {
    // For parsing enum/struct/unions that are declared with a variable rather than typedef
    // This allows "var enum {...} a,b" to share the enum definition for both variables
    // After link, these become typedefs
    // @astgen op1 := childDTypep : Optional[AstNodeDType]
    string m_name;  // Data type name
    ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ m_unique_ಅಂಕೆ;  // Unique ID distinguishing this dtype instance, for hashing/naming

ಸಾರ್ವಜನಿಕ:
    AstDefImplicitDType(File_ಸಾಲು* fl, ಧ್ರುವ string& name, VFlagChildDType, Ast_ಗಂಟು_DType* dtp)
        : ASTGEN_SUPER_DefImplicitDType(fl)
        , m_name{name}
        , m_unique_ಅಂಕೆ{unique_ಅಂಕೆ_Inc()} {
        childDTypep(dtp);  // Only for parser
        dtypep(nullptr);  // V3Width will resolve
    }
    AstDefImplicitDType(ಧ್ರುವ AstDefImplicitDType& other)
        : Ast_ಗಂಟು_DType(other)
        , m_name{other.m_name}
        , m_unique_ಅಂಕೆ{unique_ಅಂಕೆ_Inc()} {}
    ASTGEN_MEMBERS_AstDefImplicitDType;
    ಪೂರ್ಣಾಙ್ಕ unique_ಅಂಕೆ() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_unique_ಅಂಕೆ; }
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* samep) ಧ್ರುವ override {
        ಧ್ರುವ AstDefImplicitDType* ಧ್ರುವ sp = VN_DBG_AS(samep, DefImplicitDType);
        ಪ್ರತ್ಯಾಗಮ unique_ಅಂಕೆ() == sp->unique_ಅಂಕೆ();
    }
    bool similar_DType_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು_DType* samep) ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ same_ಗಂಟು(samep); }
    Ast_ಗಂಟು_DType* getChildDTypep() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ childDTypep(); }
    Ast_ಗಂಟು_DType* subDTypep() ಧ್ರುವ override VL_MT_STABLE {
        ಪ್ರತ್ಯಾಗಮ dtypep() ? dtypep() : childDTypep();
    }
    // METHODS
    // op1 = Range of variable
    Ast_ಗಂಟು_DType* dtypeSkipRefp() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ dtypep()->skipRefp(); }
    AstBasicDType* basicp() ಧ್ರುವ override VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ subDTypep()->basicp(); }
    ಪೂರ್ಣಾಙ್ಕ widthAlignBytes() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ dtypep()->widthAlignBytes(); }
    ಪೂರ್ಣಾಙ್ಕ widthTotalBytes() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ dtypep()->widthTotalBytes(); }
    string name() ಧ್ರುವ override VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ m_name; }
    ಶೂನ್ಯ name(ಧ್ರುವ string& flag) override { m_name = flag; }
    bool isCompound() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
};
class AstDynArrayDType final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_DType {
    // Dynamic array data type, ie "[]"
    // @astgen op1 := childDTypep : Optional[AstNodeDType] // moved to refDTypep() in V3Width
    //
    // @astgen ptr := m_refDTypep : Optional[AstNodeDType]  // Elements of this type (post-width)
ಸಾರ್ವಜನಿಕ:
    AstDynArrayDType(File_ಸಾಲು* fl, VFlagChildDType, Ast_ಗಂಟು_DType* dtp)
        : ASTGEN_SUPER_DynArrayDType(fl) {
        childDTypep(dtp);  // Only for parser
        refDTypep(nullptr);
        dtypep(nullptr);  // V3Width will resolve
    }
    AstDynArrayDType(File_ಸಾಲು* fl, Ast_ಗಂಟು_DType* dtp)
        : ASTGEN_SUPER_DynArrayDType(fl) {
        refDTypep(dtp);
        dtypep(nullptr);  // V3Width will resolve
    }
    ASTGEN_MEMBERS_AstDynArrayDType;
    ಧ್ರುವ ವರ್ಣ* broken() ಧ್ರುವ override {
        BROKEN_RTN(!((m_refDTypep && !childDTypep()) || (!m_refDTypep && childDTypep())));
        ಪ್ರತ್ಯಾಗಮ nullptr;
    }
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* samep) ಧ್ರುವ override {
        ಧ್ರುವ AstDynArrayDType* ಧ್ರುವ asamep = VN_DBG_AS(samep, DynArrayDType);
        ಯದಿ (!asamep->subDTypep()) ಪ್ರತ್ಯಾಗಮ false;
        ಪ್ರತ್ಯಾಗಮ subDTypep() == asamep->subDTypep();
    }
    bool similar_DType_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು_DType* samep) ಧ್ರುವ override {
        ಧ್ರುವ AstDynArrayDType* ಧ್ರುವ asamep = VN_DBG_AS(samep, DynArrayDType);
        ಪ್ರತ್ಯಾಗಮ asamep->subDTypep() && subDTypep()->similarDType(asamep->subDTypep());
    }
    string prettyDTypeName(bool full) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpSmall(std::ostream& str) ಧ್ರುವ override;
    Ast_ಗಂಟು_DType* getChildDTypep() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ childDTypep(); }
    Ast_ಗಂಟು_DType* subDTypep() ಧ್ರುವ override VL_MT_STABLE {
        ಪ್ರತ್ಯಾಗಮ m_refDTypep ? m_refDTypep : childDTypep();
    }
    bool isAggregateType() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    ಶೂನ್ಯ refDTypep(Ast_ಗಂಟು_DType* nodep) { m_refDTypep = nodep; }
    Ast_ಗಂಟು_DType* virtRefDTypep() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ m_refDTypep; }
    ಶೂನ್ಯ virtRefDTypep(Ast_ಗಂಟು_DType* nodep) override { refDTypep(nodep); }
    // METHODS
    AstBasicDType* basicp() ಧ್ರುವ override VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ nullptr; }
    ಪೂರ್ಣಾಙ್ಕ widthAlignBytes() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ subDTypep()->widthAlignBytes(); }
    ಪೂರ್ಣಾಙ್ಕ widthTotalBytes() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ subDTypep()->widthTotalBytes(); }
    bool isCompound() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool isDynamicallySized() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
};
class AstEmptyQueueDType final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_DType {
    // For EmptyQueue
ಸಾರ್ವಜನಿಕ:
    explicit AstEmptyQueueDType(File_ಸಾಲು* fl)
        : ASTGEN_SUPER_EmptyQueueDType(fl) {
        dtypep(this);
    }
    ASTGEN_MEMBERS_AstEmptyQueueDType;
    ಶೂನ್ಯ dumpSmall(std::ostream& str) ಧ್ರುವ override;
    bool hasDType() ಧ್ರುವ override VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ true; }
    bool maybePointedTo() ಧ್ರುವ override VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ true; }
    bool undead() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    Ast_ಗಂಟು_DType* subDTypep() ಧ್ರುವ override VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ nullptr; }
    Ast_ಗಂಟು_DType* virtRefDTypep() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ nullptr; }
    ಶೂನ್ಯ virtRefDTypep(Ast_ಗಂಟು_DType* nodep) override {}
    bool similar_DType_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು_DType* samep) ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ this == samep; }
    AstBasicDType* basicp() ಧ್ರುವ override VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ nullptr; }
    ಪೂರ್ಣಾಙ್ಕ widthAlignBytes() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ 1; }
    ಪೂರ್ಣಾಙ್ಕ widthTotalBytes() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ 1; }
    bool isCompound() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
};
class AstEnumDType final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_DType {
    // Parents: TYPEDEF/MODULE
    // @astgen op1 := childDTypep : Optional[AstNodeDType]
    // @astgen op2 := itemsp : List[AstEnumItem]
    //
    // @astgen ptr := m_refDTypep : Optional[AstNodeDType]  // Elements of this type (post-width)
    string m_name;  // Name from upper typedef, if any
    string m_typedefName;  // Typedef's dtypeName() once moved off the typedef declaring it
    ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ m_unique_ಅಂಕೆ;  // Unique ID distinguishing this dtype instance, for hashing/naming

ಸಾರ್ವಜನಿಕ:
    AstEnumDType(File_ಸಾಲು* fl, VFlagChildDType, Ast_ಗಂಟು_DType* dtp, AstEnumItem* itemsp)
        : ASTGEN_SUPER_EnumDType(fl)
        , m_unique_ಅಂಕೆ{unique_ಅಂಕೆ_Inc()} {
        childDTypep(dtp);  // Only for parser
        refDTypep(nullptr);
        addItemsp(itemsp);
        dtypep(nullptr);  // V3Width will resolve
        widthFromSub(subDTypep());
    }
    AstEnumDType(ಧ್ರುವ AstEnumDType& other)
        : Ast_ಗಂಟು_DType{other}
        , m_name{other.m_name}
        , m_typedefName{other.m_typedefName}
        , m_unique_ಅಂಕೆ{unique_ಅಂಕೆ_Inc()} {}
    ASTGEN_MEMBERS_AstEnumDType;

    ಧ್ರುವ ವರ್ಣ* broken() ಧ್ರುವ override;
    ಪೂರ್ಣಾಙ್ಕ unique_ಅಂಕೆ() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_unique_ಅಂಕೆ; }
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* samep) ಧ್ರುವ override {
        ಧ್ರುವ AstEnumDType* ಧ್ರುವ sp = VN_DBG_AS(samep, EnumDType);
        ಪ್ರತ್ಯಾಗಮ unique_ಅಂಕೆ() == sp->unique_ಅಂಕೆ();
    }
    bool similar_DType_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು_DType* samep) ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ same_ಗಂಟು(samep); }
    Ast_ಗಂಟು_DType* getChildDTypep() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ childDTypep(); }
    Ast_ಗಂಟು_DType* subDTypep() ಧ್ರುವ override VL_MT_STABLE {
        ಪ್ರತ್ಯಾಗಮ m_refDTypep ? m_refDTypep : childDTypep();
    }
    ಶೂನ್ಯ refDTypep(Ast_ಗಂಟು_DType* nodep) { m_refDTypep = nodep; }
    Ast_ಗಂಟು_DType* virtRefDTypep() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ m_refDTypep; }
    ಶೂನ್ಯ virtRefDTypep(Ast_ಗಂಟು_DType* nodep) override { refDTypep(nodep); }
    string name() ಧ್ರುವ override VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ m_name; }
    ಶೂನ್ಯ name(ಧ್ರುವ string& flag) override { m_name = flag; }
    // Keep the name the typedef declaring it gives, as it moves off it to the type table
    ಶೂನ್ಯ typedefName(ಧ್ರುವ string& name) { m_typedefName = name; }
    ಶೂನ್ಯ dump(std::ostream& str = std::cout) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str = std::cout) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpSmall(std::ostream& str) ಧ್ರುವ override;
    string prettyDTypeName(bool full) ಧ್ರುವ override;
    // METHODS
    AstBasicDType* basicp() ಧ್ರುವ override VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ subDTypep()->basicp(); }
    ಪೂರ್ಣಾಙ್ಕ widthAlignBytes() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ subDTypep()->widthAlignBytes(); }
    ಪೂರ್ಣಾಙ್ಕ widthTotalBytes() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ subDTypep()->widthTotalBytes(); }
    size_t item_ಎಣಿಕೆ() ಧ್ರುವ {
        size_t ಎಣಿಕೆ = 0;
        ಪ್ರತಿ (Ast_ಗಂಟು* itemp = itemsp(); itemp; itemp = itemp->nextp()) ++ಎಣಿಕೆ;
        ಪ್ರತ್ಯಾಗಮ ಎಣಿಕೆ;
    }
    bool isCompound() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
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
    AstIfaceGenericDType(File_ಸಾಲು* fl, File_ಸಾಲು* modportFl, ಧ್ರುವ string& modport)
        : ASTGEN_SUPER_IfaceGenericDType(fl)
        , m_modportFileline{modportFl}
        , m_modportName{modport} {
        dtypep(this);
    }
    ASTGEN_MEMBERS_AstIfaceGenericDType;
    ಶೂನ್ಯ dumpSmall(std::ostream& str) ಧ್ರುವ override;
    bool hasDType() ಧ್ರುವ override VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ true; }
    bool maybePointedTo() ಧ್ರುವ override VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ true; }
    bool undead() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    Ast_ಗಂಟು_DType* subDTypep() ಧ್ರುವ override VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ nullptr; }
    Ast_ಗಂಟು_DType* virtRefDTypep() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ nullptr; }
    ಶೂನ್ಯ virtRefDTypep(Ast_ಗಂಟು_DType* nodep) override {}
    bool similar_DType_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು_DType* samep) ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ this == samep; }
    AstBasicDType* basicp() ಧ್ರುವ override VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ nullptr; }
    ಪೂರ್ಣಾಙ್ಕ widthAlignBytes() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ 1; }
    ಪೂರ್ಣಾಙ್ಕ widthTotalBytes() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ 1; }
    string modportName() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_modportName; }
    bool isModport() { ಪ್ರತ್ಯಾಗಮ !m_modportName.empty(); }
    bool isCompound() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    File_ಸಾಲು* modportFileline() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_modportFileline; }
    string name() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ m_modportName; }
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
    AstIfaceRefDType(File_ಸಾಲು* fl, ಧ್ರುವ string& cellName, ಧ್ರುವ string& ifaceName)
        : ASTGEN_SUPER_IfaceRefDType(fl)
        , m_modportFileline{nullptr}
        , m_cellName{cellName}
        , m_ifaceName{ifaceName}
        , m_modportName{""} {}
    AstIfaceRefDType(File_ಸಾಲು* fl, File_ಸಾಲು* modportFl, ಧ್ರುವ string& cellName,
                     ಧ್ರುವ string& ifaceName, ಧ್ರುವ string& modport)
        : ASTGEN_SUPER_IfaceRefDType(fl)
        , m_modportFileline{modportFl}
        , m_cellName{cellName}
        , m_ifaceName{ifaceName}
        , m_modportName{modport} {}
    AstIfaceRefDType(File_ಸಾಲು* fl, File_ಸಾಲು* modportFl, ಧ್ರುವ string& cellName,
                     ಧ್ರುವ string& ifaceName, ಧ್ರುವ string& modport, AstPin* paramsp)
        : ASTGEN_SUPER_IfaceRefDType(fl)
        , m_modportFileline{modportFl}
        , m_cellName{cellName}
        , m_ifaceName{ifaceName}
        , m_modportName{modport} {
        addParamsp(paramsp);
    }
    ASTGEN_MEMBERS_AstIfaceRefDType;

    // METHODS
    ಶೂನ್ಯ dump(std::ostream& str = std::cout) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str = std::cout) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpSmall(std::ostream& str) ಧ್ರುವ override;
    string prettyDTypeName(bool full) ಧ್ರುವ override;
    AstBasicDType* basicp() ಧ್ರುವ override VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ nullptr; }
    bool similar_DType_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು_DType* samep) ಧ್ರುವ override {
        // Each occurrence of a virtual interface type parses to its own node,
        // so pointer identity is insufficient; compare the referenced
        // interface instead, same equivalence as AstNode::computeCastable uses.
        ಧ್ರುವ AstIfaceRefDType* ಧ್ರುವ asamep = VN_DBG_AS(samep, IfaceRefDType);
        ಪ್ರತ್ಯಾಗಮ ifaceViaCellp() && ifaceViaCellp() == asamep->ifaceViaCellp()
               && modportName() == asamep->modportName() && isVirtual() == asamep->isVirtual();
    }
    ಪೂರ್ಣಾಙ್ಕ widthAlignBytes() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ 0; }
    ಪೂರ್ಣಾಙ್ಕ widthTotalBytes() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ 0; }
    bool isVirtual() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_virtual; }
    ಶೂನ್ಯ isVirtual(bool flag) {
        m_virtual = flag;
        ಯದಿ (flag) v3Global.setHasVirtIfaces();
    }
    File_ಸಾಲು* modportFileline() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_modportFileline; }
    ಶೂನ್ಯ modportFileline(File_ಸಾಲು* ಧ್ರುವ modportFileline) { m_modportFileline = modportFileline; }
    string cellName() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_cellName; }
    ಶೂನ್ಯ cellName(ಧ್ರುವ string& name) { m_cellName = name; }
    string ifaceName() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_ifaceName; }
    string ifaceNameQ() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ "'" + prettyName(ifaceName()) + "'"; }
    ಶೂನ್ಯ ifaceName(ಧ್ರುವ string& name) { m_ifaceName = name; }
    ಶೂನ್ಯ modportName(ಧ್ರುವ string& modportName) { m_modportName = modportName; }
    string modportName() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_modportName; }
    AstIface* ifaceViaCellp() ಧ್ರುವ;  // Use cellp or ifacep
    AstIface* ifacep() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_ifacep; }
    ಶೂನ್ಯ ifacep(AstIface* nodep) { m_ifacep = nodep; }
    AstCell* cellp() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_cellp; }
    ಶೂನ್ಯ cellp(AstCell* nodep) { m_cellp = nodep; }
    AstModport* modportp() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_modportp; }
    ಶೂನ್ಯ modportp(AstModport* modportp) { m_modportp = modportp; }
    bool isModport() { ಪ್ರತ್ಯಾಗಮ !m_modportName.empty(); }
    bool isCompound() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }  // But not relevant
};
class AstMemberDType final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_DType {
    // A member of a struct/union
    // PARENT: AstNodeUOrStructDType
    // @astgen op1 := childDTypep : Optional[AstNodeDType]
    // @astgen op3 := valuep : Optional[AstNode]
    //
    // @astgen ptr := m_refDTypep : Optional[AstNodeDType]  // Elements of this type (post-width)
    string m_name;  // Name of variable
    string m_tag;  // Holds the string of the verilator tag -- used in JSON output.
    ಪೂರ್ಣಾಙ್ಕ m_lsb = -1;  // Within this level's packed struct, the LSB of the first bit of the member
    bool m_constrainedRand = false;  // Member has a constraint expression
    VRandAttr m_rand;  // Randomizability of this member (rand, randc, etc)
ಸಾರ್ವಜನಿಕ:
    AstMemberDType(File_ಸಾಲು* fl, ಧ್ರುವ string& name, VFlagChildDType, Ast_ಗಂಟು_DType* dtp,
                   Ast_ಗಂಟು* valuep)
        : ASTGEN_SUPER_MemberDType(fl)
        , m_name{name} {
        childDTypep(dtp);  // Only for parser
        this->valuep(valuep);
        dtypep(nullptr);  // V3Width will resolve
        refDTypep(nullptr);
    }
    AstMemberDType(File_ಸಾಲು* fl, ಧ್ರುವ string& name, Ast_ಗಂಟು_DType* dtp)
        : ASTGEN_SUPER_MemberDType(fl)
        , m_name{name} {
        UASSERT(dtp, "AstMember created with no dtype");
        refDTypep(dtp);
        dtypep(this);
        widthFromSub(subDTypep());
    }
    ASTGEN_MEMBERS_AstMemberDType;
    ಶೂನ್ಯ dump(std::ostream& str = std::cout) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str = std::cout) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpSmall(std::ostream& str) ಧ್ರುವ override;
    string prettyDTypeName(bool full) ಧ್ರುವ override;
    string name() ಧ್ರುವ override VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ m_name; }  // * = Var name
    bool hasDType() ಧ್ರುವ override VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ true; }
    bool maybePointedTo() ಧ್ರುವ override VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ true; }
    Ast_ಗಂಟು_DType* getChildDTypep() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ childDTypep(); }
    Ast_ಗಂಟು_UOr_Struct_DType* getChildStructp();
    Ast_ಗಂಟು_DType* subDTypep() ಧ್ರುವ override VL_MT_STABLE {
        ಪ್ರತ್ಯಾಗಮ m_refDTypep ? m_refDTypep : childDTypep();
    }
    ಶೂನ್ಯ refDTypep(Ast_ಗಂಟು_DType* nodep) { m_refDTypep = nodep; }
    Ast_ಗಂಟು_DType* virtRefDTypep() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ m_refDTypep; }
    ಶೂನ್ಯ virtRefDTypep(Ast_ಗಂಟು_DType* nodep) override { refDTypep(nodep); }
    bool similar_DType_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು_DType* samep) ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ this == samep; }
    //
    // (Slow) recurse down to find basic data type (Note don't need virtual -
    // AstVar isn't a NodeDType)
    AstBasicDType* basicp() ಧ್ರುವ override VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ subDTypep()->basicp(); }
    // op1 = Range of variable (Note don't need virtual - AstVar isn't a NodeDType)
    Ast_ಗಂಟು_DType* dtypeSkipRefp() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ subDTypep()->skipRefp(); }
    // (Slow) recurses - Structure alignment 1,2,4 or 8 bytes (arrays affect this)
    ಪೂರ್ಣಾಙ್ಕ widthAlignBytes() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ subDTypep()->widthAlignBytes(); }
    // (Slow) recurses - Width in bytes rounding up 1,2,4,8,12,...
    ಪೂರ್ಣಾಙ್ಕ widthTotalBytes() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ subDTypep()->widthTotalBytes(); }
    // METHODS
    ಶೂನ್ಯ name(ಧ್ರುವ string& name) override { m_name = name; }
    string tag() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ m_tag; }
    ಶೂನ್ಯ tag(ಧ್ರುವ string& text) override { m_tag = text; }
    ಪೂರ್ಣಾಙ್ಕ lsb() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_lsb; }
    ಶೂನ್ಯ lsb(ಪೂರ್ಣಾಙ್ಕ lsb) { m_lsb = lsb; }
    bool isCompound() ಧ್ರುವ override {
        v3fatalSrc("call isCompound on subdata type, not reference");
        ಪ್ರತ್ಯಾಗಮ false;
    }
    bool isConstrainedRand() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_constrainedRand; }
    ಶೂನ್ಯ markConstrainedRand(bool flag) { m_constrainedRand = flag; }
    VRandAttr rand() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_rand; }
    ಶೂನ್ಯ rand(ಧ್ರುವ VRandAttr flag) { m_rand = flag; }
};
class AstNBACommitQueueDType final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_DType {
    // @astgen ptr := m_subDTypep : AstNodeDType  // Type of the corresponding variable
    ಧ್ರುವ bool m_partial;  // Partial element update required

ಸಾರ್ವಜನಿಕ:
    AstNBACommitQueueDType(File_ಸಾಲು* fl, Ast_ಗಂಟು_DType* subDTypep, bool partial)
        : ASTGEN_SUPER_NBACommitQueueDType(fl)
        , m_partial{partial}
        , m_subDTypep{subDTypep} {
        dtypep(this);
    }
    ASTGEN_MEMBERS_AstNBACommitQueueDType;
    ಶೂನ್ಯ dump(std::ostream& str) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಧ್ರುವ override;
    Ast_ಗಂಟು_DType* subDTypep() ಧ್ರುವ override VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ m_subDTypep; }
    bool partial() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_partial; }
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* samep) ಧ್ರುವ override {
        ಧ್ರುವ AstNBACommitQueueDType* ಧ್ರುವ asamep = VN_DBG_AS(samep, NBACommitQueueDType);
        ಪ್ರತ್ಯಾಗಮ m_partial == asamep->m_partial;
    }
    bool similar_DType_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು_DType* samep) ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ this == samep; }
    AstBasicDType* basicp() ಧ್ರುವ override VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ nullptr; }
    ಪೂರ್ಣಾಙ್ಕ widthAlignBytes() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ 1; }
    ಪೂರ್ಣಾಙ್ಕ widthTotalBytes() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ 24; }
    bool isCompound() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
};
class AstParamTypeDType final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_DType {
    // Parents: MODULE
    // A parameter type statement; much like a var or typedef
    // @astgen op1 := childDTypep : Optional[AstNodeDType]
    ಧ್ರುವ VVarType m_varType;  // Type of variable (for localparam vs. param)
    ಧ್ರುವ VFwdType m_fwdType;  // Forward type for lint check
    string m_name;  // Name of variable
ಸಾರ್ವಜನಿಕ:
    AstParamTypeDType(File_ಸಾಲು* fl, VVarType type, VFwdType fwdType, ಧ್ರುವ string& name,
                      VFlagChildDType, Ast_ಗಂಟು_DType* dtp)
        : ASTGEN_SUPER_ParamTypeDType(fl)
        , m_varType{type}
        , m_fwdType{fwdType}
        , m_name{name} {
        childDTypep(dtp);  // Only for parser
        dtypep(nullptr);  // V3Width will resolve
    }
    ASTGEN_MEMBERS_AstParamTypeDType;
    ಶೂನ್ಯ dump(std::ostream& str = std::cout) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str = std::cout) ಧ್ರುವ override;
    string prettyDTypeName(bool full) ಧ್ರುವ override;
    Ast_ಗಂಟು_DType* getChildDTypep() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ childDTypep(); }
    Ast_ಗಂಟು_DType* subDTypep() ಧ್ರುವ override VL_MT_STABLE {
        ಪ್ರತ್ಯಾಗಮ dtypep() ? dtypep() : childDTypep();
    }
    AstBasicDType* basicp() ಧ್ರುವ override VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ subDTypep()->basicp(); }
    bool similar_DType_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು_DType* samep) ಧ್ರುವ override {
        ಧ್ರುವ AstParamTypeDType* ಧ್ರುವ sp = VN_DBG_AS(samep, ParamTypeDType);
        ಪ್ರತ್ಯಾಗಮ this->subDTypep()->similarDType(sp->subDTypep());
    }
    ಪೂರ್ಣಾಙ್ಕ widthAlignBytes() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ dtypep()->widthAlignBytes(); }
    ಪೂರ್ಣಾಙ್ಕ widthTotalBytes() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ dtypep()->widthTotalBytes(); }
    // METHODS
    string name() ಧ್ರುವ override VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ m_name; }
    bool maybePointedTo() ಧ್ರುವ override VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ true; }
    bool hasDType() ಧ್ರುವ override VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ true; }
    ಶೂನ್ಯ name(ಧ್ರುವ string& flag) override { m_name = flag; }
    VVarType varType() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_varType; }  // * = Type of variable
    VFwdType fwdType() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_fwdType; }
    bool isParam() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ true; }
    bool isGParam() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ (varType() == VVarType::GPARAM); }
    bool isCompound() ಧ್ರುವ override {
        v3fatalSrc("call isCompound on subdata type, not reference");
        ಪ್ರತ್ಯಾಗಮ false;
    }
};
class AstParseTypeDType final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_DType {
    // Parents: VAR
    // During parsing, this indicates the type of a parameter is a "parameter type"
    // e.g. the data type is a container of any data type
    ಧ್ರುವ VFwdType m_fwdType;  // Forward type for lint check
ಸಾರ್ವಜನಿಕ:
    explicit AstParseTypeDType(File_ಸಾಲು* fl, VFwdType fwdType = VFwdType::NONE)
        : ASTGEN_SUPER_ParseTypeDType(fl)
        , m_fwdType{fwdType} {}
    ASTGEN_MEMBERS_AstParseTypeDType;
    Ast_ಗಂಟು_DType* dtypep() ಧ್ರುವ VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ nullptr; }
    // METHODS
    ಶೂನ್ಯ dump(std::ostream& str = std::cout) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str = std::cout) ಧ್ರುವ override;
    bool similar_DType_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು_DType* samep) ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ this == samep; }
    AstBasicDType* basicp() ಧ್ರುವ override VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ nullptr; }
    ಪೂರ್ಣಾಙ್ಕ widthAlignBytes() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ 0; }
    ಪೂರ್ಣಾಙ್ಕ widthTotalBytes() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ 0; }
    bool isCompound() ಧ್ರುವ override {
        v3fatalSrc("call isCompound on subdata type, not reference");
        ಪ್ರತ್ಯಾಗಮ false;
    }
    VFwdType fwdType() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_fwdType; }
};
class AstQueueDType final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_DType {
    // Queue array data type, ie "[ $ ]"
    // @astgen op1 := childDTypep : Optional[AstNodeDType] // moved to refDTypep() in V3Width
    // @astgen op2 := boundp : Optional[AstNodeExpr]
    //
    // @astgen ptr := m_refDTypep : Optional[AstNodeDType]  // Elements of this type (post-width)
ಸಾರ್ವಜನಿಕ:
    AstQueueDType(File_ಸಾಲು* fl, VFlagChildDType, Ast_ಗಂಟು_DType* dtp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* boundp)
        : ASTGEN_SUPER_QueueDType(fl) {
        childDTypep(dtp);
        this->boundp(boundp);
        refDTypep(nullptr);
        dtypep(nullptr);  // V3Width will resolve
    }
    AstQueueDType(File_ಸಾಲು* fl, Ast_ಗಂಟು_DType* dtp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* boundp)
        : ASTGEN_SUPER_QueueDType(fl) {
        this->boundp(boundp);
        refDTypep(dtp);
        dtypep(dtp);
    }
    ASTGEN_MEMBERS_AstQueueDType;
    ಧ್ರುವ ವರ್ಣ* broken() ಧ್ರುವ override {
        BROKEN_RTN(!((m_refDTypep && !childDTypep()) || (!m_refDTypep && childDTypep())));
        ಪ್ರತ್ಯಾಗಮ nullptr;
    }
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* samep) ಧ್ರುವ override {
        ಧ್ರುವ AstQueueDType* ಧ್ರುವ asamep = VN_DBG_AS(samep, QueueDType);
        ಯದಿ (!asamep->subDTypep()) ಪ್ರತ್ಯಾಗಮ false;
        ಪ್ರತ್ಯಾಗಮ (subDTypep() == asamep->subDTypep());
    }
    bool similar_DType_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು_DType* samep) ಧ್ರುವ override {
        ಧ್ರುವ AstQueueDType* ಧ್ರುವ asamep = VN_DBG_AS(samep, QueueDType);
        ಪ್ರತ್ಯಾಗಮ asamep->subDTypep() && subDTypep()->similarDType(asamep->subDTypep());
    }
    ಶೂನ್ಯ dumpSmall(std::ostream& str) ಧ್ರುವ override;
    string prettyDTypeName(bool full) ಧ್ರುವ override;
    Ast_ಗಂಟು_DType* getChildDTypep() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ childDTypep(); }
    Ast_ಗಂಟು_DType* subDTypep() ಧ್ರುವ override VL_MT_STABLE {
        ಪ್ರತ್ಯಾಗಮ m_refDTypep ? m_refDTypep : childDTypep();
    }
    bool isAggregateType() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    ಶೂನ್ಯ refDTypep(Ast_ಗಂಟು_DType* nodep) { m_refDTypep = nodep; }
    ಅನ್ತಃಸ್ಥ ಪೂರ್ಣಾಙ್ಕ boundConst() ಧ್ರುವ VL_MT_STABLE;
    Ast_ಗಂಟು_DType* virtRefDTypep() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ m_refDTypep; }
    ಶೂನ್ಯ virtRefDTypep(Ast_ಗಂಟು_DType* nodep) override { refDTypep(nodep); }
    // METHODS
    AstBasicDType* basicp() ಧ್ರುವ override VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ nullptr; }
    ಪೂರ್ಣಾಙ್ಕ widthAlignBytes() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ subDTypep()->widthAlignBytes(); }
    ಪೂರ್ಣಾಙ್ಕ widthTotalBytes() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ subDTypep()->widthTotalBytes(); }
    bool isCompound() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool isDynamicallySized() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
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
    ಧ್ರುವ VIfaceCaptureTag* m_captureTagp = nullptr;

ಸಾರ್ವಜನಿಕ:
    AstRefDType(File_ಸಾಲು* fl, ಧ್ರುವ string& name)
        : ASTGEN_SUPER_RefDType(fl)
        , m_name{name} {}
    AstRefDType(File_ಸಾಲು* fl, ಧ್ರುವ string& name, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* classOrPackagep, AstPin* paramsp)
        : ASTGEN_SUPER_RefDType(fl)
        , m_name{name} {
        classOrPackageOpp(classOrPackagep);
        addParamsp(paramsp);
    }
    class Flag_Type_Of_ಅಭಿವ್ಯಕ್ತಿ {};  // type(expr) for parser only
    AstRefDType(File_ಸಾಲು* fl, Flag_Type_Of_ಅಭಿವ್ಯಕ್ತಿ, Ast_ಗಂಟು* typeofp)
        : ASTGEN_SUPER_RefDType(fl) {
        this->typeofp(typeofp);
        ಯದಿ (Ast_ಗಂಟು_DType* ಧ್ರುವ dtp = VN_CAST(typeofp, ಗಂಟು_DType)) refDTypep(dtp);
    }
    ASTGEN_MEMBERS_AstRefDType;
    // METHODS
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* samep) ಧ್ರುವ override {
        ಧ್ರುವ AstRefDType* ಧ್ರುವ asamep = VN_DBG_AS(samep, RefDType);
        ಪ್ರತ್ಯಾಗಮ (m_typedefp == asamep->m_typedefp && m_refDTypep == asamep->m_refDTypep
                && m_name == asamep->m_name && m_classOrPackagep == asamep->m_classOrPackagep);
    }
    bool similar_DType_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು_DType* samep) ಧ್ರುವ override {
        ಪ್ರತ್ಯಾಗಮ subDTypep()->similarDType(samep->subDTypep());
    }
    ಧ್ರುವ ವರ್ಣ* broken() ಧ್ರುವ override;
    ಶೂನ್ಯ dump(std::ostream& str = std::cout) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str = std::cout) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpSmall(std::ostream& str) ಧ್ರುವ override;
    string name() ಧ್ರುವ override VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ m_name; }
    string prettyDTypeName(bool full) ಧ್ರುವ override {
        ಪ್ರತ್ಯಾಗಮ subDTypep() ? subDTypep()->prettyDTypeName(full) : prettyName();
    }
    AstBasicDType* basicp() ಧ್ರುವ override VL_MT_STABLE {
        ಪ್ರತ್ಯಾಗಮ subDTypep() ? subDTypep()->basicp() : nullptr;
    }
    Ast_ಗಂಟು_DType* subDTypep() ಧ್ರುವ override VL_MT_STABLE;
    Ast_ಗಂಟು_DType* getChildDTypep() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ VN_CAST(typeofp(), ಗಂಟು_DType); }
    ಪೂರ್ಣಾಙ್ಕ widthAlignBytes() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ dtypeSkipRefp()->widthAlignBytes(); }
    ಪೂರ್ಣಾಙ್ಕ widthTotalBytes() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ dtypeSkipRefp()->widthTotalBytes(); }
    ಶೂನ್ಯ name(ಧ್ರುವ string& flag) override { m_name = flag; }
    Ast_ಗಂಟು_DType* dtypeSkipRefp() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ subDTypep()->skipRefp(); }
    AstTypedef* typedefp() ಧ್ರುವ VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ m_typedefp; }
    ಶೂನ್ಯ typedefp(AstTypedef* nodep) { m_typedefp = nodep; }
    Ast_ಗಂಟು_DType* refDTypep() ಧ್ರುವ VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ m_refDTypep; }
    ಶೂನ್ಯ refDTypep(Ast_ಗಂಟು_DType* nodep) { m_refDTypep = nodep; }
    Ast_ಗಂಟು_DType* virtRefDTypep() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ refDTypep(); }
    ಶೂನ್ಯ virtRefDTypep(Ast_ಗಂಟು_DType* nodep) override { refDTypep(nodep); }
    Ast_ಗಂಟು_Module* classOrPackagep() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_classOrPackagep; }
    ಶೂನ್ಯ classOrPackagep(Ast_ಗಂಟು_Module* nodep) { m_classOrPackagep = nodep; }
    ಧ್ರುವ VIfaceCaptureTag* captureTagp() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_captureTagp; }
    ಶೂನ್ಯ captureTagp(ಧ್ರುವ VIfaceCaptureTag* tagp) { m_captureTagp = tagp; }
    bool isCompound() ಧ್ರುವ override {
        v3fatalSrc("call isCompound on subdata type, not reference");
        ಪ್ರತ್ಯಾಗಮ false;
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
    bool similar_DType_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು_DType* samep) ಧ್ರುವ override {
        ಧ್ರುವ AstRequireDType* ಧ್ರುವ asamep = VN_DBG_AS(samep, RequireDType);
        ಪ್ರತ್ಯಾಗಮ subDTypep()->similarDType(asamep->subDTypep());
    }
    AstBasicDType* basicp() ಧ್ರುವ override VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ nullptr; }
    Ast_ಗಂಟು_DType* subDTypep() ಧ್ರುವ override VL_MT_STABLE {
        // Used for recursive definition checking
        ಯದಿ (Ast_ಗಂಟು_DType* ಧ್ರುವ dtp = VN_CAST(lhsp(), ಗಂಟು_DType)) ಪ್ರತ್ಯಾಗಮ dtp;

        ಪ್ರತ್ಯಾಗಮ nullptr;
    }
    ಪೂರ್ಣಾಙ್ಕ widthAlignBytes() ಧ್ರುವ override { V3ERROR_NA_RETURN(1); }
    ಪೂರ್ಣಾಙ್ಕ widthTotalBytes() ಧ್ರುವ override { V3ERROR_NA_RETURN(1); }
    bool isCompound() ಧ್ರುವ override { V3ERROR_NA_RETURN(false); }
};
class AstSampleQueueDType final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_DType {
    // @astgen op1 := childDTypep : Optional[AstNodeDType] // moved to refDTypep() in V3Width
    //
    // @astgen ptr := m_refDTypep : Optional[AstNodeDType]  // Elements of this type (post-width)
ಸಾರ್ವಜನಿಕ:
    AstSampleQueueDType(File_ಸಾಲು* fl, Ast_ಗಂಟು_DType* dtp)
        : ASTGEN_SUPER_SampleQueueDType(fl) {
        refDTypep(dtp);
        dtypep(dtp);
    }
    ASTGEN_MEMBERS_AstSampleQueueDType;
    ಧ್ರುವ ವರ್ಣ* broken() ಧ್ರುವ override {
        BROKEN_RTN(!((m_refDTypep && !childDTypep()) || (!m_refDTypep && childDTypep())));
        ಪ್ರತ್ಯಾಗಮ nullptr;
    }
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* samep) ಧ್ರುವ override {
        ಧ್ರುವ AstSampleQueueDType* ಧ್ರುವ asamep = VN_DBG_AS(samep, SampleQueueDType);
        ಯದಿ (!asamep->subDTypep()) ಪ್ರತ್ಯಾಗಮ false;
        ಪ್ರತ್ಯಾಗಮ (subDTypep() == asamep->subDTypep());
    }
    bool similar_DType_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು_DType* samep) ಧ್ರುವ override {
        ಧ್ರುವ AstSampleQueueDType* ಧ್ರುವ asamep = VN_DBG_AS(samep, SampleQueueDType);
        ಪ್ರತ್ಯಾಗಮ asamep->subDTypep() && subDTypep()->similarDType(asamep->subDTypep());
    }
    ಶೂನ್ಯ dumpSmall(std::ostream& str) ಧ್ರುವ override;
    Ast_ಗಂಟು_DType* getChildDTypep() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ childDTypep(); }
    // op1 = Range of variable
    Ast_ಗಂಟು_DType* subDTypep() ಧ್ರುವ override VL_MT_STABLE {
        ಪ್ರತ್ಯಾಗಮ m_refDTypep ? m_refDTypep : childDTypep();
    }
    ಶೂನ್ಯ refDTypep(Ast_ಗಂಟು_DType* nodep) { m_refDTypep = nodep; }
    Ast_ಗಂಟು_DType* virtRefDTypep() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ m_refDTypep; }
    ಶೂನ್ಯ virtRefDTypep(Ast_ಗಂಟು_DType* nodep) override { refDTypep(nodep); }
    // METHODS
    AstBasicDType* basicp() ಧ್ರುವ override VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ subDTypep()->basicp(); }
    ಪೂರ್ಣಾಙ್ಕ widthAlignBytes() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ ಪರಿಮಾಣ(std::map<std::string, std::string>); }
    ಪೂರ್ಣಾಙ್ಕ widthTotalBytes() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ ಪರಿಮಾಣ(std::map<std::string, std::string>); }
    bool isCompound() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
};
class AstStreamDType final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_DType {
    // Stream data type, used only as data type of stream operations
    // Should behave like AstPackArrayDType, but it doesn't have a size
ಸಾರ್ವಜನಿಕ:
    explicit AstStreamDType(File_ಸಾಲು* fl)
        : ASTGEN_SUPER_StreamDType(fl) {
        dtypep(this);
    }
    ASTGEN_MEMBERS_AstStreamDType;
    ಶೂನ್ಯ dumpSmall(std::ostream& str) ಧ್ರುವ override;
    bool hasDType() ಧ್ರುವ override VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ true; }
    bool maybePointedTo() ಧ್ರುವ override VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ true; }
    bool undead() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    Ast_ಗಂಟು_DType* subDTypep() ಧ್ರುವ override VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ nullptr; }
    Ast_ಗಂಟು_DType* virtRefDTypep() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ nullptr; }
    ಶೂನ್ಯ virtRefDTypep(Ast_ಗಂಟು_DType* nodep) override {}
    bool similar_DType_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು_DType* samep) ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ this == samep; }
    AstBasicDType* basicp() ಧ್ರುವ override VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ nullptr; }
    ಪೂರ್ಣಾಙ್ಕ widthAlignBytes() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ 1; }
    ಪೂರ್ಣಾಙ್ಕ widthTotalBytes() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ 1; }
    bool isCompound() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
};
class AstUnsizedArrayDType final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_DType {
    // Unsized/open-range Array data type, ie "some_dtype var_name []"
    // @astgen op1 := childDTypep : Optional[AstNodeDType] // moved to refDTypep() in V3Width
    //
    // @astgen ptr := m_refDTypep : Optional[AstNodeDType]  // Elements of this type (post-width)
ಸಾರ್ವಜನಿಕ:
    AstUnsizedArrayDType(File_ಸಾಲು* fl, VFlagChildDType, Ast_ಗಂಟು_DType* dtp)
        : ASTGEN_SUPER_UnsizedArrayDType(fl) {
        childDTypep(dtp);  // Only for parser
        refDTypep(nullptr);
        dtypep(nullptr);  // V3Width will resolve
    }
    AstUnsizedArrayDType(File_ಸಾಲು* fl, Ast_ಗಂಟು_DType* dtp)
        : ASTGEN_SUPER_UnsizedArrayDType(fl) {
        refDTypep(dtp);
        dtypep(nullptr);  // V3Width will resolve
    }
    ASTGEN_MEMBERS_AstUnsizedArrayDType;
    ಧ್ರುವ ವರ್ಣ* broken() ಧ್ರುವ override {
        BROKEN_RTN(!((m_refDTypep && !childDTypep()) || (!m_refDTypep && childDTypep())));
        ಪ್ರತ್ಯಾಗಮ nullptr;
    }
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* samep) ಧ್ರುವ override;
    bool similar_DType_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು_DType* samep) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpSmall(std::ostream& str) ಧ್ರುವ override;
    Ast_ಗಂಟು_DType* getChildDTypep() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ childDTypep(); }
    Ast_ಗಂಟು_DType* subDTypep() ಧ್ರುವ override VL_MT_STABLE {
        ಪ್ರತ್ಯಾಗಮ m_refDTypep ? m_refDTypep : childDTypep();
    }
    ಶೂನ್ಯ refDTypep(Ast_ಗಂಟು_DType* nodep) { m_refDTypep = nodep; }
    Ast_ಗಂಟು_DType* virtRefDTypep() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ m_refDTypep; }
    ಶೂನ್ಯ virtRefDTypep(Ast_ಗಂಟು_DType* nodep) override { refDTypep(nodep); }
    // METHODS
    AstBasicDType* basicp() ಧ್ರುವ override VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ subDTypep()->basicp(); }
    ಪೂರ್ಣಾಙ್ಕ widthAlignBytes() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ subDTypep()->widthAlignBytes(); }
    ಪೂರ್ಣಾಙ್ಕ widthTotalBytes() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ subDTypep()->widthTotalBytes(); }
    bool isCompound() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
};
class AstVoidDType final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_DType {
    // For e.g. a function returning void
ಸಾರ್ವಜನಿಕ:
    explicit AstVoidDType(File_ಸಾಲು* fl)
        : ASTGEN_SUPER_VoidDType(fl) {
        dtypep(this);
    }
    ASTGEN_MEMBERS_AstVoidDType;
    ಶೂನ್ಯ dumpSmall(std::ostream& str) ಧ್ರುವ override;
    bool hasDType() ಧ್ರುವ override VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ true; }
    bool maybePointedTo() ಧ್ರುವ override VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ true; }
    bool undead() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    Ast_ಗಂಟು_DType* subDTypep() ಧ್ರುವ override VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ nullptr; }
    Ast_ಗಂಟು_DType* virtRefDTypep() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ nullptr; }
    ಶೂನ್ಯ virtRefDTypep(Ast_ಗಂಟು_DType* nodep) override {}
    bool similar_DType_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು_DType* samep) ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    AstBasicDType* basicp() ಧ್ರುವ override VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ nullptr; }
    ಪೂರ್ಣಾಙ್ಕ widthAlignBytes() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ 1; }
    ಪೂರ್ಣಾಙ್ಕ widthTotalBytes() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ 1; }
    bool isCompound() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
};
class AstWildcardArrayDType final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_DType {
    // Wildcard index type associative array data type, ie "some_dtype var_name [*]"
    // @astgen op1 := childDTypep : Optional[AstNodeDType] // moved to refDTypep() in V3Width
    //
    // @astgen ptr := m_refDTypep : Optional[AstNodeDType]  // Elements of this type (post-width)
ಸಾರ್ವಜನಿಕ:
    AstWildcardArrayDType(File_ಸಾಲು* fl, VFlagChildDType, Ast_ಗಂಟು_DType* dtp)
        : ASTGEN_SUPER_WildcardArrayDType(fl) {
        childDTypep(dtp);  // Only for parser
        refDTypep(nullptr);
        dtypep(nullptr);  // V3Width will resolve
    }
    ASTGEN_MEMBERS_AstWildcardArrayDType;
    ಧ್ರುವ ವರ್ಣ* broken() ಧ್ರುವ override {
        BROKEN_RTN(!((m_refDTypep && !childDTypep()) || (!m_refDTypep && childDTypep())));
        ಪ್ರತ್ಯಾಗಮ nullptr;
    }
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* samep) ಧ್ರುವ override;
    bool similar_DType_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು_DType* samep) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpSmall(std::ostream& str) ಧ್ರುವ override;
    string prettyDTypeName(bool full) ಧ್ರುವ override;
    Ast_ಗಂಟು_DType* getChildDTypep() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ childDTypep(); }
    Ast_ಗಂಟು_DType* subDTypep() ಧ್ರುವ override VL_MT_STABLE {
        ಪ್ರತ್ಯಾಗಮ m_refDTypep ? m_refDTypep : childDTypep();
    }
    ಶೂನ್ಯ refDTypep(Ast_ಗಂಟು_DType* nodep) { m_refDTypep = nodep; }
    Ast_ಗಂಟು_DType* virtRefDTypep() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ m_refDTypep; }
    ಶೂನ್ಯ virtRefDTypep(Ast_ಗಂಟು_DType* nodep) override { refDTypep(nodep); }
    // METHODS
    AstBasicDType* basicp() ಧ್ರುವ override VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ subDTypep()->basicp(); }
    ಪೂರ್ಣಾಙ್ಕ widthAlignBytes() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ ಪರಿಮಾಣ(std::map<std::string, std::string>); }
    ಪೂರ್ಣಾಙ್ಕ widthTotalBytes() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ ಪರಿಮಾಣ(std::map<std::string, std::string>); }
    bool isCompound() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    bool isDynamicallySized() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
};

// === AstNodeArrayDType ===
class AstPackArrayDType final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Array_DType {
    // Packed array data type, ie "some_dtype [2:0] var_name"
ಸಾರ್ವಜನಿಕ:
    ಅನ್ತಃಸ್ಥ AstPackArrayDType(File_ಸಾಲು* fl, VFlagChildDType, Ast_ಗಂಟು_DType* dtp, AstRange* rangep);
    ಅನ್ತಃಸ್ಥ AstPackArrayDType(File_ಸಾಲು* fl, Ast_ಗಂಟು_DType* dtp, AstRange* rangep);
    ASTGEN_MEMBERS_AstPackArrayDType;
    string prettyDTypeName(bool full) ಧ್ರುವ override;
    bool isCompound() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
};
class AstUnpackArrayDType final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_Array_DType {
    // Array data type, ie "some_dtype var_name [2:0]"
    bool m_isCompound = false;  // Non-POD subDType, or parent requires compound
ಸಾರ್ವಜನಿಕ:
    AstUnpackArrayDType(File_ಸಾಲು* fl, VFlagChildDType, Ast_ಗಂಟು_DType* dtp, AstRange* rangep)
        : ASTGEN_SUPER_UnpackArrayDType(fl) {
        childDTypep(dtp);  // Only for parser
        this->rangep(rangep);
        refDTypep(nullptr);
        dtypep(nullptr);  // V3Width will resolve
        // For backward compatibility AstNodeArrayDType and others inherit
        // width and signing from the subDType/base type
        widthFromSub(subDTypep());
    }
    AstUnpackArrayDType(File_ಸಾಲು* fl, Ast_ಗಂಟು_DType* dtp, AstRange* rangep)
        : ASTGEN_SUPER_UnpackArrayDType(fl) {
        this->rangep(rangep);
        refDTypep(dtp);
        dtypep(this);
        // For backward compatibility AstNodeArrayDType and others inherit
        // width and signing from the subDType/base type
        widthFromSub(subDTypep());
    }
    ASTGEN_MEMBERS_AstUnpackArrayDType;
    ಶೂನ್ಯ dump(std::ostream& str) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಧ್ರುವ override;
    string prettyDTypeName(bool full) ಧ್ರುವ override;
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* samep) ಧ್ರುವ override {
        ಧ್ರುವ AstUnpackArrayDType* ಧ್ರುವ sp = VN_DBG_AS(samep, UnpackArrayDType);
        ಪ್ರತ್ಯಾಗಮ m_isCompound == sp->m_isCompound;
    }
    bool similar_DType_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು_DType* samep) ಧ್ರುವ override;
    bool isAggregateType() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ true; }
    // True iff 'otherp' is an array of the same shape, with
    // at least one dimension reversed ascending/descending.
    bool isReverseOf(ಧ್ರುವ Ast_ಗಂಟು_DType* otherp) ಧ್ರುವ;
    // Outer dimension comes first. The first element is this node.
    std::vector<AstUnpackArrayDType*> unpackDimensions();
    ಶೂನ್ಯ isCompound(bool flag) { m_isCompound = flag; }
    bool isCompound() ಧ್ರುವ override VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ m_isCompound; }
    bool isIntegralOrPacked() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ false; }
};

// === AstNodeUOrStructDType ===
class AstStructDType final : ಸಾರ್ವಜನಿಕ Ast_ಗಂಟು_UOr_Struct_DType {
ಸಾರ್ವಜನಿಕ:
    // VSigning below is mispurposed to indicate if packed or not
    AstStructDType(File_ಸಾಲು* fl, VSigning numericUnpack)
        : ASTGEN_SUPER_StructDType(fl, numericUnpack) {}
    ASTGEN_MEMBERS_AstStructDType;
    string verilogKwd() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ "struct"; }
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
    string verilogKwd() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ "union"; }
    bool isSoft() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_isSoft; }
    bool isTagged() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_isTagged; }
    bool same_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು* samep) ಧ್ರುವ override;
    ಶೂನ್ಯ dump(std::ostream& str) ಧ್ರುವ override;
    ಶೂನ್ಯ dumpJson(std::ostream& str) ಧ್ರುವ override;
};

#endif  // Guard
