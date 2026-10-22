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
#ଅନ୍ତର୍ଭାଵ "V3Ast.h"  // This helps code analysis tools pick up symbols in V3Ast.h
#define VL_NOT_FINAL  // This #define fixes broken code folding in the CLion IDE
#endif

// === Abstract base node types (AstNode*) =====================================

class Ast_ଗଣ୍ଠି_DType VL_NOT_FINAL : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି {
    // Ideally width() would migrate to BasicDType as that's where it makes sense,
    // but it's currently so prevalent in the code we leave it here.
    // Note the below members are included in AstTypeTable::Key lookups
    // dist-ast-dump-suppress  // Part of dumpSmall
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_width = 0;  // (also in AstTypeTable::Key) Bit width of operation
    // (also in AstTypeTable::Key) If unsized, bitwidth of minimum implementation
    // dist-ast-dump-suppress  // Part of dumpSmall
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_widthMin = 0;
    // dist-ast-dump-suppress  // Part of dumpSmall
    VSigning m_numeric;  // (also in AstTypeTable::Key) Node is signed
    // dist-ast-dump-suppress  // Part of dumpSmall
    bool m_generic = false;  // Simple globally referenced type, don't garbage collect
    // Unique number assigned to each dtype during creation for IEEE matching
    ସ୍ଥିର ପୂର୍ଣ୍ଣସଂଖ୍ୟା s_unique_ଅଙ୍କ;

ସୁରକ୍ଷିତ:
    // CONSTRUCTORS
    Ast_ଗଣ୍ଠି_DType(VNType t, File_ଧାଡ଼ି* fl)
        : Ast_ଗଣ୍ଠି{t, fl} {}

ବ୍ୟକ୍ତିଗତ:
    // METHODS
    ଅଚଳ Ast_ଗଣ୍ଠି_DType* skipRefIterp(bool skipConst, bool skipEnum,
                                     bool assertOn = true) ଅଚଳ VL_MT_STABLE;

ସୁରକ୍ଷିତ:
    // METHODS
    ଆଭାସୀ bool similar_DType_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି_DType* samep) ଅଚଳ = 0;

ସର୍ବସାଧାରଣ:
    ASTGEN_MEMBERS_Ast_ଗଣ୍ଠି_DType;
    // ACCESSORS
    ଶୂନ୍ୟ dump(std::ostream& str) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str) ଅଚଳ override;
    ଆଭାସୀ ଶୂନ୍ୟ dumpSmall(std::ostream& str) ଅଚଳ VL_MT_STABLE;
    bool hasDType() ଅଚଳ override VL_MT_SAFE { ଫେରନ୍ତୁ true; }
    /// Require VlUnpacked, instead of [] for POD elements.
    /// A non-POD object is always compound, but some POD elements
    /// are compound when methods calls operate on object, or when
    /// under another compound-requiring object e.g. class
    ଆଭାସୀ bool isCompound() ଅଚଳ = 0;
    // Integral or packed, allowed inside an unpacked union/struct
    ଆଭାସୀ bool isIntegralOrPacked() ଅଚଳ { ଫେରନ୍ତୁ !isCompound(); }
    // (Slow) recurse down to find basic data type
    ଆଭାସୀ AstBasicDType* basicp() ଅଚଳ VL_MT_STABLE = 0;
    // (Slow) Recurse over MemberDType|ParamTypeDType|RefDType|ConstDType|EnumDType to other type
    ଅଚଳ Ast_ଗଣ୍ଠି_DType* skipRefp() ଅଚଳ VL_MT_STABLE { ଫେରନ୍ତୁ skipRefIterp(true, true); }
    Ast_ଗଣ୍ଠି_DType* skipRefp() VL_MT_STABLE {
        ଫେରନ୍ତୁ const_cast<Ast_ଗଣ୍ଠି_DType*>(
            static_cast<ଅଚଳ Ast_ଗଣ୍ଠି_DType*>(this)->skipRefIterp(true, true));
    }
    // (Slow) Recurse over MemberDType|ParamTypeDType|RefDType|ConstDType|EnumDType to other type,
    // Returns null if not resolved
    ଅଚଳ Ast_ଗଣ୍ଠି_DType* skipRefOrNullp() ଅଚଳ VL_MT_STABLE {
        ଫେରନ୍ତୁ skipRefIterp(true, true, false);
    }
    Ast_ଗଣ୍ଠି_DType* skipRefOrNullp() VL_MT_STABLE {
        ଫେରନ୍ତୁ const_cast<Ast_ଗଣ୍ଠି_DType*>(
            static_cast<ଅଚଳ Ast_ଗଣ୍ଠି_DType*>(this)->skipRefIterp(true, true, false));
    }
    // (Slow) Recurse over MemberDType|ParamTypeDType|RefDType|EnumDType to ConstDType
    ଅଚଳ Ast_ଗଣ୍ଠି_DType* skipRefToConstp() ଅଚଳ { ଫେରନ୍ତୁ skipRefIterp(false, true); }
    Ast_ଗଣ୍ଠି_DType* skipRefToConstp() {
        ଫେରନ୍ତୁ const_cast<Ast_ଗଣ୍ଠି_DType*>(
            static_cast<ଅଚଳ Ast_ଗଣ୍ଠି_DType*>(this)->skipRefIterp(false, true));
    }
    // (Slow) Recurse over MemberDType|ParamTypeDType|RefDType|ConstDType to EnumDType
    ଅଚଳ Ast_ଗଣ୍ଠି_DType* skipRefToEnump() ଅଚଳ { ଫେରନ୍ତୁ skipRefIterp(true, false); }
    Ast_ଗଣ୍ଠି_DType* skipRefToEnump() {
        ଫେରନ୍ତୁ const_cast<Ast_ଗଣ୍ଠି_DType*>(
            static_cast<ଅଚଳ Ast_ଗଣ୍ଠି_DType*>(this)->skipRefIterp(true, false));
    }
    // (Slow) Recurse over MemberDType|ParamTypeDType|RefDType|ConstDType to EnumDType,
    // Returns null if not resolved
    ଅଚଳ Ast_ଗଣ୍ଠି_DType* skipRefToEnumOrNullp() ଅଚଳ { ଫେରନ୍ତୁ skipRefIterp(true, false, false); }
    // (Slow) Recurse over MemberDType|ParamTypeDType|RefDType to other type
    ଅଚଳ Ast_ଗଣ୍ଠି_DType* skipRefToNonRefp() ଅଚଳ { ଫେରନ୍ତୁ skipRefIterp(false, false); }
    Ast_ଗଣ୍ଠି_DType* skipRefToNonRefp() {
        ଫେରନ୍ତୁ const_cast<Ast_ଗଣ୍ଠି_DType*>(
            static_cast<ଅଚଳ Ast_ଗଣ୍ଠି_DType*>(this)->skipRefIterp(false, false));
    }
    // If array, returns element dtype, otherwise returns skipRef dtype
    // If skipRef is false, RefDTypes are not followed (safe before typedef linking)
    ଅଚଳ Ast_ଗଣ୍ଠି_DType* elemDTypep(bool skipRef = true) ଅଚଳ VL_MT_STABLE;
    Ast_ଗଣ୍ଠି_DType* elemDTypep(bool skipRef = true) VL_MT_STABLE {
        ଫେରନ୍ତୁ const_cast<Ast_ଗଣ୍ଠି_DType*>(
            static_cast<ଅଚଳ Ast_ଗଣ୍ଠି_DType*>(this)->elemDTypep(skipRef));
    }
    // (Slow) recurses - Structure alignment 1,2,4 or 8 bytes (arrays affect this)
    ଆଭାସୀ ପୂର୍ଣ୍ଣସଂଖ୍ୟା widthAlignBytes() ଅଚଳ = 0;
    // (Slow) recurses - Width in bytes rounding up 1,2,4,8,12,...
    ଆଭାସୀ ପୂର୍ଣ୍ଣସଂଖ୍ୟା widthTotalBytes() ଅଚଳ = 0;
    bool maybePointedTo() ଅଚଳ override VL_MT_SAFE { ଫେରନ୍ତୁ true; }
    // Iff has a non-null refDTypep(), as generic node function
    ଆଭାସୀ Ast_ଗଣ୍ଠି_DType* virtRefDTypep() ଅଚଳ { ଫେରନ୍ତୁ nullptr; }
    // Iff has refDTypep(), set as generic node function
    ଆଭାସୀ ଶୂନ୍ୟ virtRefDTypep(Ast_ଗଣ୍ଠି_DType* nodep) {}
    // Iff has a non-null second dtypep, as generic node function
    ଆଭାସୀ Ast_ଗଣ୍ଠି_DType* virtRefDType2p() ଅଚଳ { ଫେରନ୍ତୁ nullptr; }
    // Iff has second dtype, set as generic node function
    ଆଭାସୀ ଶୂନ୍ୟ virtRefDType2p(Ast_ଗଣ୍ଠି_DType* nodep) {}
    // Assignable equivalence.  Calls skipRefToNonRefp() during comparisons.
    bool similarDType(ଅଚଳ Ast_ଗଣ୍ଠି_DType* samep) ଅଚଳ;
    // Iff has a non-null subDTypep(), as generic node function
    ଆଭାସୀ Ast_ଗଣ୍ଠି_DType* subDTypep() ଅଚଳ VL_MT_STABLE { ଫେରନ୍ତୁ nullptr; }
    ଆଭାସୀ Ast_ଗଣ୍ଠି_DType* subDType2p() ଅଚଳ VL_MT_STABLE { ଫେରନ୍ତୁ nullptr; }
    ଆଭାସୀ bool isAggregateType() ଅଚଳ { ଫେରନ୍ତୁ false; }
    // True for unpacked, dynamic, queue, and associative arrays (not packed arrays)
    bool isNonPackedArray() ଅଚଳ;
    ଆଭାସୀ bool isFourstate() ଅଚଳ;
    // Ideally an IEEE $typename
    ଆଭାସୀ string prettyDTypeName(bool) ଅଚଳ { ଫେରନ୍ତୁ prettyTypeName(); }
    string prettyDTypeNameQ(bool full = false) ଅଚଳ { ଫେରନ୍ତୁ "'" + prettyDTypeName(full) + "'"; }
    string stateDTypeName() ଅଚଳ { ଫେରନ୍ତୁ this->isFourstate() ? "(4-state)" : "(2-state)"; }
    //
    // Changing the width may confuse the data type resolution, so must clear
    // TypeTable cache after use.
    ଶୂନ୍ୟ widthForce(ପୂର୍ଣ୍ଣସଂଖ୍ୟା width, ପୂର୍ଣ୍ଣସଂଖ୍ୟା widthMin) {
        m_width = width;
        m_widthMin = widthMin;
    }
    // For backward compatibility inherit width and signing from the subDType/base type
    ଶୂନ୍ୟ widthFromSub(ଅଚଳ Ast_ଗଣ୍ଠି_DType* nodep) {
        m_width = nodep->m_width;
        m_widthMin = nodep->m_widthMin;
        m_numeric = nodep->m_numeric;
    }
    //
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା width() ଅଚଳ VL_MT_STABLE { ଫେରନ୍ତୁ m_width; }
    ଶୂନ୍ୟ numeric(VSigning flag) { m_numeric = flag; }
    bool isSigned() ଅଚଳ VL_MT_STABLE { ଫେରନ୍ତୁ m_numeric.isSigned(); }
    bool isNosign() ଅଚଳ VL_MT_SAFE { ଫେରନ୍ତୁ m_numeric.isNosign(); }
    VSigning numeric() ଅଚଳ VL_MT_STABLE { ଫେରନ୍ତୁ m_numeric; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା width_ଶବ୍ଦଗୁଡ଼ିକ() ଅଚଳ VL_MT_STABLE { ଫେରନ୍ତୁ VL_ଶବ୍ଦଗୁଡ଼ିକ_I(width()); }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା widthMin() ଅଚଳ VL_MT_STABLE {  // If sized, the size,
                                         // if unsized the min digits to represent it
        ଫେରନ୍ତୁ m_widthMin ? m_widthMin : m_width;
    }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା widthPow2() ଅଚଳ;
    ଶୂନ୍ୟ widthMinFromWidth() { m_widthMin = m_width; }
    bool widthSized() ଅଚଳ VL_MT_SAFE { ଫେରନ୍ତୁ !m_widthMin || m_widthMin == m_width; }
    bool generic() ଅଚଳ VL_MT_SAFE { ଫେରନ୍ତୁ m_generic; }
    ଶୂନ୍ୟ generic(bool flag) { m_generic = flag; }
    std::pair<uint32_t, uint32_t> dimensions(bool includeBasic) ଅଚଳ;
    uint32_t arrayUnpackedElements() ଅଚଳ;  // 1, or total multiplication of all dimensions
    // Fixed aggregate streaming properties
    bool isStreamableFixedAggregate() ଅଚଳ;
    // Whether is a string, or an unpacked array or structure containing one
    bool containsString() ଅଚଳ;
    bool containsUnpackedStruct() ଅଚଳ;
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା widthStream() ଅଚଳ;
    string vlEnumType() ଅଚଳ;  // Return VerilatedVarType: VLVT_UINT32, etc
    string vlEnumPackedAgg() ଅଚଳ;  // Return VLVF_PACKED_AGG etc flags, or empty
    ସ୍ଥିର ପୂର୍ଣ୍ଣସଂଖ୍ୟା unique_ଅଙ୍କ_Inc() { ଫେରନ୍ତୁ ++s_unique_ଅଙ୍କ; }
    ଅଚଳ ଅକ୍ଷର* charIQWN() ଅଚଳ {
        ଫେରନ୍ତୁ (isString() ? "N" : isWide() ? "W" : isDouble() ? "D" : isQuad() ? "Q" : "I");
    }
    string cType(ଅଚଳ string& name, bool forFunc, bool isRef, bool packed = false) ଅଚଳ;
    // Represents a C++ LiteralType? (can be constexpr)
    bool isLiteralType() ଅଚଳ VL_MT_STABLE;
    // C++ type (as emitted by cType) is trivially default constructible and destructible?
    bool isTriviallyConstructible() ଅଚଳ VL_MT_STABLE;
    ଆଭାସୀ bool isDynamicallySized() ଅଚଳ { ଫେରନ୍ତୁ false; }

ବ୍ୟକ୍ତିଗତ:
    class CTypeRecursed;
    CTypeRecursed cTypeRecurse(bool compound, bool packed) ଅଚଳ;
};
class Ast_ଗଣ୍ଠି_Array_DType VL_NOT_FINAL : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_DType {
    // Array data type, ie "some_dtype var_name [2:0]"
    // @astgen op1 := childDTypep : Optional[AstNodeDType] // moved to refDTypep() in V3Width
    // @astgen op2 := rangep : Optional[AstRange] // array bounds
    //
    // @astgen ptr := m_refDTypep : Optional[AstNodeDType]  // Elements of this type (post-width)
    Ast_ଗଣ୍ଠି* rangenp() ଅଚଳ { ଫେରନ୍ତୁ reinterpret_cast<Ast_ଗଣ୍ଠି*>(rangep()); }

ସୁରକ୍ଷିତ:
    Ast_ଗଣ୍ଠି_Array_DType(VNType t, File_ଧାଡ଼ି* fl)
        : Ast_ଗଣ୍ଠି_DType{t, fl} {}

ସର୍ବସାଧାରଣ:
    ASTGEN_MEMBERS_Ast_ଗଣ୍ଠି_Array_DType;
    ଶୂନ୍ୟ dump(std::ostream& str) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str) ଅଚଳ override;
    ଶୂନ୍ୟ dumpSmall(std::ostream& str) ଅଚଳ override;
    ଅଚଳ ଅକ୍ଷର* broken() ଅଚଳ override {
        BROKEN_RTN(!((m_refDTypep && !childDTypep()) || (!m_refDTypep && childDTypep())));
        ଫେରନ୍ତୁ nullptr;
    }
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* samep) ଅଚଳ override {
        ଅଚଳ Ast_ଗଣ୍ଠି_Array_DType* ଅଚଳ asamep = VN_DBG_AS(samep, ଗଣ୍ଠି_Array_DType);
        ଫେରନ୍ତୁ hi() == asamep->hi() && rangenp()->same_ବୃକ୍ଷ(asamep->rangenp())
               && subDTypep() == asamep->subDTypep();
    }  // HashedDT doesn't recurse, so need to check children
    bool similar_DType_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି_DType* samep) ଅଚଳ override {
        ଅଚଳ Ast_ଗଣ୍ଠି_Array_DType* ଅଚଳ asamep = VN_DBG_AS(samep, ଗଣ୍ଠି_Array_DType);
        ଫେରନ୍ତୁ elementsConst() == asamep->elementsConst()
               && subDTypep()->similarDType(asamep->subDTypep());
    }
    Ast_ଗଣ୍ଠି_DType* getChildDTypep() ଅଚଳ override { ଫେରନ୍ତୁ childDTypep(); }
    Ast_ଗଣ୍ଠି_DType* subDTypep() ଅଚଳ override VL_MT_STABLE {
        ଫେରନ୍ତୁ m_refDTypep ? m_refDTypep : childDTypep();
    }
    ଶୂନ୍ୟ refDTypep(Ast_ଗଣ୍ଠି_DType* nodep) { m_refDTypep = nodep; }
    Ast_ଗଣ୍ଠି_DType* virtRefDTypep() ଅଚଳ override { ଫେରନ୍ତୁ m_refDTypep; }
    ଶୂନ୍ୟ virtRefDTypep(Ast_ଗଣ୍ଠି_DType* nodep) override { refDTypep(nodep); }
    // METHODS
    AstBasicDType* basicp() ଅଚଳ override VL_MT_STABLE {
        ଫେରନ୍ତୁ subDTypep()->basicp();
    }  // (Slow) recurse down to find basic data type
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା widthAlignBytes() ଅଚଳ override { ଫେରନ୍ତୁ subDTypep()->widthAlignBytes(); }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା widthTotalBytes() ଅଚଳ override {
        ଫେରନ୍ତୁ elementsConst() * subDTypep()->widthTotalBytes();
    }
    ଧାଡ଼ିଭିତରେ ପୂର୍ଣ୍ଣସଂଖ୍ୟା ବାମ() ଅଚଳ VL_MT_STABLE;
    ଧାଡ଼ିଭିତରେ ପୂର୍ଣ୍ଣସଂଖ୍ୟା ଡାହାଣ() ଅଚଳ VL_MT_STABLE;
    ଧାଡ଼ିଭିତରେ ପୂର୍ଣ୍ଣସଂଖ୍ୟା hi() ଅଚଳ VL_MT_STABLE;
    ଧାଡ଼ିଭିତରେ ପୂର୍ଣ୍ଣସଂଖ୍ୟା lo() ଅଚଳ VL_MT_STABLE;
    ଧାଡ଼ିଭିତରେ ପୂର୍ଣ୍ଣସଂଖ୍ୟା elementsConst() ଅଚଳ VL_MT_STABLE;
    ଧାଡ଼ିଭିତରେ VNumRange declRange() ଅଚଳ VL_MT_STABLE;
};
class Ast_ଗଣ୍ଠି_UOr_Struct_DType VL_NOT_FINAL : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_DType {
    // A struct or union; common handling
    // @astgen op1 := membersp : List[AstMemberDType]
    //
    // @astgen ptr := m_classOrPackagep : Optional[AstNodeModule]  // Package emitted with
    string m_name;  // Name from upper typedef, if any
    string m_typedefName;  // Typedef's dtypeName() once moved off the typedef declaring it
    ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_unique_ଅଙ୍କ;  // Unique ID distinguishing this dtype instance, for hashing/naming
    bool m_packed;  // Packed struct/union, else unpacked
    bool m_isFourstate = false;  // V3Width computes; true if any member is 4-state
    bool m_constrainedRand = false;  // True if struct has constraint expression
    bool m_emitToString = false;  // Generate to_string() for this struct/union if set

ସୁରକ୍ଷିତ:
    Ast_ଗଣ୍ଠି_UOr_Struct_DType(VNType t, File_ଧାଡ଼ି* fl, VSigning numericUnpack)
        : Ast_ଗଣ୍ଠି_DType{t, fl}
        , m_unique_ଅଙ୍କ{unique_ଅଙ୍କ_Inc()} {
        // VSigning::NOSIGN overloaded to indicate not packed
        m_packed = (numericUnpack != VSigning::NOSIGN);
        numeric(VSigning::fromBool(numericUnpack.isSigned()));
    }
    Ast_ଗଣ୍ଠି_UOr_Struct_DType(ଅଚଳ Ast_ଗଣ୍ଠି_UOr_Struct_DType& other)
        : Ast_ଗଣ୍ଠି_DType{other}
        , m_name{other.m_name}
        , m_typedefName{other.m_typedefName}
        , m_unique_ଅଙ୍କ{unique_ଅଙ୍କ_Inc()}
        , m_packed{other.m_packed}
        , m_isFourstate{other.m_isFourstate} {}

ସର୍ବସାଧାରଣ:
    ASTGEN_MEMBERS_Ast_ଗଣ୍ଠି_UOr_Struct_DType;
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା unique_ଅଙ୍କ() ଅଚଳ { ଫେରନ୍ତୁ m_unique_ଅଙ୍କ; }
    ଶୂନ୍ୟ dump(std::ostream& str) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str) ଅଚଳ override;
    string prettyDTypeName(bool) ଅଚଳ override;
    bool isCompound() ଅଚଳ override { ଫେରନ୍ତୁ !packed(); }
    // For basicp() we reuse the size to indicate a "fake" basic type of same size
    AstBasicDType* basicp() ଅଚଳ override VL_MT_STABLE {
        ଯଦି (!m_packed) ଫେରନ୍ତୁ nullptr;
        ଫେରନ୍ତୁ (isFourstate()
                    ? VN_AS(findLogicRangeDType(VNumRange{width() - 1, 0}, width(), numeric()),
                            BasicDType)
                    : VN_AS(findBitRangeDType(VNumRange{width() - 1, 0}, width(), numeric()),
                            BasicDType));
    }
    // (Slow) recurses - Structure alignment 1,2,4 or 8 bytes (arrays affect this)
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା widthAlignBytes() ଅଚଳ override;
    // (Slow) recurses - Width in bytes rounding up 1,2,4,8,12,...
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା widthTotalBytes() ଅଚଳ override;
    bool similar_DType_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି_DType* samep) ଅଚଳ override;
    string name() ଅଚଳ override VL_MT_STABLE { ଫେରନ୍ତୁ m_name; }
    ଶୂନ୍ୟ name(ଅଚଳ string& flag) override { m_name = flag; }
    // Keep the name the typedef declaring it gives, as it moves off it to the type table
    ଶୂନ୍ୟ typedefName(ଅଚଳ string& name) { m_typedefName = name; }
    // Name of the C structure for DPI, empty if no typedef names it, so declared where used
    string dpiName() ଅଚଳ;
    // C typedef of the unpacked structure for DPI (IEEE 1800-2023 H.7.3)
    string dpiTypedef() ଅଚଳ;
    bool packed() ଅଚଳ VL_MT_SAFE { ଫେରନ୍ତୁ m_packed; }
    ଶୂନ୍ୟ packed(bool flag) { m_packed = flag; }
    // packed() but as don't support unpacked, presently all structs
    ସ୍ଥିର bool packedUnsup() { ଫେରନ୍ତୁ true; }
    ଶୂନ୍ୟ isFourstate(bool flag) { m_isFourstate = flag; }
    bool isFourstate() ଅଚଳ override VL_MT_SAFE { ଫେରନ୍ତୁ m_isFourstate; }
    ସ୍ଥିର ପୂର୍ଣ୍ଣସଂଖ୍ୟା lo() VL_MT_STABLE { ଫେରନ୍ତୁ 0; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା hi() ଅଚଳ VL_MT_STABLE {
        ଫେରନ୍ତୁ dtypep()->width() - 1;
    }  // Packed classes look like arrays
    VNumRange declRange() ଅଚଳ VL_MT_STABLE { ଫେରନ୍ତୁ VNumRange{hi(), lo()}; }
    Ast_ଗଣ୍ଠି_Module* classOrPackagep() ଅଚଳ { ଫେରନ୍ତୁ m_classOrPackagep; }
    ଶୂନ୍ୟ classOrPackagep(Ast_ଗଣ୍ଠି_Module* classpackagep) { m_classOrPackagep = classpackagep; }
    bool isConstrainedRand() ଅଚଳ { ଫେରନ୍ତୁ m_constrainedRand; }
    ଶୂନ୍ୟ markConstrainedRand(bool flag) { m_constrainedRand = flag; }
    bool emitToString() ଅଚଳ { ଫେରନ୍ତୁ m_emitToString; }
    ଶୂନ୍ୟ setEmitToString() { m_emitToString = true; }
    bool isAggregateType() ଅଚଳ override { ଫେରନ୍ତୁ !packed(); }
};

// === Concrete node types =====================================================

// === AstNode ===
class AstEnumItem final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି {
    // @astgen op1 := rangep : Optional[AstRange] // Range for name appending
    // @astgen op2 := valuep : Optional[AstNodeExpr]
    string m_name;  // Name of item

ସର୍ବସାଧାରଣ:
    // Parents: ENUM
    AstEnumItem(File_ଧାଡ଼ି* fl, ଅଚଳ string& name, AstRange* rangep, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* valuep)
        : ASTGEN_SUPER_EnumItem(fl)
        , m_name{name} {
        this->rangep(rangep);
        this->valuep(valuep);
    }
    ASTGEN_MEMBERS_AstEnumItem;
    string name() ଅଚଳ override VL_MT_STABLE { ଫେରନ୍ତୁ m_name; }
    bool maybePointedTo() ଅଚଳ override VL_MT_SAFE { ଫେରନ୍ତୁ true; }
    bool hasDType() ଅଚଳ override VL_MT_SAFE { ଫେରନ୍ତୁ true; }
    ଶୂନ୍ୟ name(ଅଚଳ string& flag) override { m_name = flag; }
};

// === AstNodeDType ===
class AstAssocArrayDType final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_DType {
    // Associative array data type, ie "[some_dtype]"
    // @astgen op1 := childDTypep : Optional[AstNodeDType] // moved to refDTypep() in V3Width
    // @astgen op2 := keyChildDTypep : Optional[AstNodeDType]
    //
    // @astgen ptr := m_refDTypep : Optional[AstNodeDType]  // Elements of this type (post-width)
    // @astgen ptr := m_keyDTypep : Optional[AstNodeDType]  // Keys of this type (post-width)
ସର୍ବସାଧାରଣ:
    AstAssocArrayDType(File_ଧାଡ଼ି* fl, VFlagChildDType, Ast_ଗଣ୍ଠି_DType* dtp, Ast_ଗଣ୍ଠି_DType* ଚାବି_Dtp)
        : ASTGEN_SUPER_AssocArrayDType(fl) {
        childDTypep(dtp);  // Only for parser
        ଚାବି_Child_DTypep(ଚାବି_Dtp);  // Only for parser
        refDTypep(nullptr);
        ଚାବି_DTypep(nullptr);
        dtypep(nullptr);  // V3Width will resolve
    }
    AstAssocArrayDType(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_DType* dtp, Ast_ଗଣ୍ଠି_DType* ଚାବି_Dtp)
        : ASTGEN_SUPER_AssocArrayDType(fl) {
        refDTypep(dtp);
        ଚାବି_DTypep(ଚାବି_Dtp);
        dtypep(dtp);
    }
    ASTGEN_MEMBERS_AstAssocArrayDType;
    ଅଚଳ ଅକ୍ଷର* broken() ଅଚଳ override {
        BROKEN_RTN(!((m_refDTypep && !childDTypep()) || (!m_refDTypep && childDTypep())));
        BROKEN_RTN(!((m_ଚାବି_DTypep && !childDTypep()) || (!m_ଚାବି_DTypep && childDTypep())));
        ଫେରନ୍ତୁ nullptr;
    }
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* samep) ଅଚଳ override {
        ଅଚଳ AstAssocArrayDType* ଅଚଳ asamep = VN_DBG_AS(samep, AssocArrayDType);
        ଯଦି (!asamep->subDTypep()) ଫେରନ୍ତୁ false;
        ଯଦି (!asamep->ଚାବି_DTypep()) ଫେରନ୍ତୁ false;
        ଫେରନ୍ତୁ (subDTypep() == asamep->subDTypep() && ଚାବି_DTypep() == asamep->ଚାବି_DTypep());
    }
    bool similar_DType_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି_DType* samep) ଅଚଳ override {
        ଅଚଳ AstAssocArrayDType* ଅଚଳ asamep = VN_DBG_AS(samep, AssocArrayDType);
        ଫେରନ୍ତୁ asamep->subDTypep() && subDTypep()->similarDType(asamep->subDTypep())
               && asamep->ଚାବି_DTypep() && ଚାବି_DTypep()->similarDType(asamep->ଚାବି_DTypep());
    }
    string prettyDTypeName(bool full) ଅଚଳ override;
    ଶୂନ୍ୟ dumpSmall(std::ostream& str) ଅଚଳ override;
    Ast_ଗଣ୍ଠି_DType* getChildDTypep() ଅଚଳ override { ଫେରନ୍ତୁ childDTypep(); }
    Ast_ଗଣ୍ଠି_DType* getChild2DTypep() ଅଚଳ override { ଫେରନ୍ତୁ ଚାବି_Child_DTypep(); }
    Ast_ଗଣ୍ଠି_DType* subDTypep() ଅଚଳ override VL_MT_STABLE {
        ଫେରନ୍ତୁ m_refDTypep ? m_refDTypep : childDTypep();
    }
    Ast_ଗଣ୍ଠି_DType* subDType2p() ଅଚଳ override VL_MT_STABLE {
        ଫେରନ୍ତୁ m_ଚାବି_DTypep ? m_ଚାବି_DTypep : ଚାବି_Child_DTypep();
    }
    bool isAggregateType() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    ଶୂନ୍ୟ refDTypep(Ast_ଗଣ୍ଠି_DType* nodep) { m_refDTypep = nodep; }
    Ast_ଗଣ୍ଠି_DType* virtRefDTypep() ଅଚଳ override { ଫେରନ୍ତୁ m_refDTypep; }
    ଶୂନ୍ୟ virtRefDTypep(Ast_ଗଣ୍ଠି_DType* nodep) override { refDTypep(nodep); }
    Ast_ଗଣ୍ଠି_DType* virtRefDType2p() ଅଚଳ override { ଫେରନ୍ତୁ m_ଚାବି_DTypep; }
    ଶୂନ୍ୟ virtRefDType2p(Ast_ଗଣ୍ଠି_DType* nodep) override { ଚାବି_DTypep(nodep); }
    //
    Ast_ଗଣ୍ଠି_DType* ଚାବି_DTypep() ଅଚଳ VL_MT_STABLE {
        ଫେରନ୍ତୁ m_ଚାବି_DTypep ? m_ଚାବି_DTypep : ଚାବି_Child_DTypep();
    }
    ଶୂନ୍ୟ ଚାବି_DTypep(Ast_ଗଣ୍ଠି_DType* nodep) { m_ଚାବି_DTypep = nodep; }
    // METHODS
    AstBasicDType* basicp() ଅଚଳ override VL_MT_STABLE { ଫେରନ୍ତୁ nullptr; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା widthAlignBytes() ଅଚଳ override { ଫେରନ୍ତୁ subDTypep()->widthAlignBytes(); }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା widthTotalBytes() ଅଚଳ override { ଫେରନ୍ତୁ subDTypep()->widthTotalBytes(); }
    bool isCompound() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool isDynamicallySized() ଅଚଳ override { ଫେରନ୍ତୁ true; }
};
class AstBasicDType final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_DType {
    // Builtin atomic/vectored data type
    // @astgen op1 := rangep : Optional[AstRange] // Range of variable
    ସଂରଚନା Members final {
        VBasicDTypeKwd m_keyword;  // (also in VBasicTypeKey) What keyword created basic type
        // dist-ast-dump-suppress  // Part of dumpSmall
        VNumRange m_nrange;  // (also in VBasicTypeKey) Numeric msb/lsb (if non-opaque keyword)
        bool operator==(ଅଚଳ Members& rhs) ଅଚଳ {
            ଫେରନ୍ତୁ rhs.m_keyword == m_keyword && rhs.m_nrange == m_nrange;
        }
    } m;
    // See also in AstNodeDType: m_width, m_widthMin, m_numeric(issigned)
ସର୍ବସାଧାରଣ:
    AstBasicDType(File_ଧାଡ଼ି* fl, VBasicDTypeKwd kwd, ଅଚଳ VSigning& signst = VSigning::NOSIGN)
        : ASTGEN_SUPER_BasicDType(fl) {
        init(kwd, signst, 0, -1, nullptr);
    }
    AstBasicDType(File_ଧାଡ଼ି* fl, VFlagLogicPacked, ପୂର୍ଣ୍ଣସଂଖ୍ୟା wantwidth)
        : ASTGEN_SUPER_BasicDType(fl) {
        init(VBasicDTypeKwd::LOGIC, VSigning::NOSIGN, wantwidth, -1, nullptr);
    }
    AstBasicDType(File_ଧାଡ଼ି* fl, VFlagLogic2StatePacked, ପୂର୍ଣ୍ଣସଂଖ୍ୟା wantwidth)
        : ASTGEN_SUPER_BasicDType(fl) {
        init(VBasicDTypeKwd::LOGIC2STATE, VSigning::NOSIGN, wantwidth, -1, nullptr);
    }
    AstBasicDType(File_ଧାଡ଼ି* fl, VFlagBitPacked, ପୂର୍ଣ୍ଣସଂଖ୍ୟା wantwidth)
        : ASTGEN_SUPER_BasicDType(fl) {
        init(VBasicDTypeKwd::BIT, VSigning::NOSIGN, wantwidth, -1, nullptr);
    }
    AstBasicDType(File_ଧାଡ଼ି* fl, VBasicDTypeKwd kwd, VSigning numer, ପୂର୍ଣ୍ଣସଂଖ୍ୟା wantwidth, ପୂର୍ଣ୍ଣସଂଖ୍ୟା widthmin)
        : ASTGEN_SUPER_BasicDType(fl) {
        init(kwd, numer, wantwidth, widthmin, nullptr);
    }
    AstBasicDType(File_ଧାଡ଼ି* fl, VBasicDTypeKwd kwd, VSigning numer, VNumRange range, ପୂର୍ଣ୍ଣସଂଖ୍ୟା widthmin)
        : ASTGEN_SUPER_BasicDType(fl) {
        init(kwd, numer, range.elements(), widthmin, nullptr);
        m.m_nrange = range;  // as init() presumes lsb==0, but range.lsb() might not be
    }
    // See also addRange in verilog.y
ବ୍ୟକ୍ତିଗତ:
    ଶୂନ୍ୟ init(VBasicDTypeKwd kwd, VSigning numer, ପୂର୍ଣ୍ଣସଂଖ୍ୟା wantwidth, ପୂର୍ଣ୍ଣସଂଖ୍ୟା wantwidthmin,
              AstRange* rangep);

ସର୍ବସାଧାରଣ:
    ASTGEN_MEMBERS_AstBasicDType;
    ଶୂନ୍ୟ dump(std::ostream& str) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str) ଅଚଳ override;
    // width/widthMin/numeric compared elsewhere
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* samep) ଅଚଳ override;
    bool similar_DType_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି_DType* samep) ଅଚଳ override;
    string name() ଅଚଳ override VL_MT_STABLE { ଫେରନ୍ତୁ m.m_keyword.typeName(); }
    string prettyDTypeName(bool full) ଅଚଳ override;
    ଅଚଳ ଅକ୍ଷର* broken() ଅଚଳ override {
        BROKEN_RTN(dtypep() != this);
        BROKEN_RTN(v3Global.widthMinUsage() == VWidthMinUsage::VERILOG_WIDTH
                   && widthMin() > width());
        ଫେରନ୍ତୁ nullptr;
    }
    ଶୂନ୍ୟ setSignedState(ଅଚଳ VSigning& signst) {
        // Note NOSIGN does NOT change the state; this is required by the parser
        ଯଦି (signst == VSigning::UNSIGNED) {
            numeric(signst);
        } ଅନ୍ୟଥା ଯଦି (signst == VSigning::SIGNED) {
            numeric(signst);
        }
    }
    // METHODS
    AstBasicDType* basicp() ଅଚଳ override VL_MT_STABLE { ଫେରନ୍ତୁ (AstBasicDType*)this; }
    // (Slow) recurses - Structure alignment 1,2,4 or 8 bytes (arrays affect this)
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା widthAlignBytes() ଅଚଳ override;
    // (Slow) recurses - Width in bytes rounding up 1,2,4,8,12,...
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା widthTotalBytes() ଅଚଳ override;
    bool isFourstate() ଅଚଳ override { ଫେରନ୍ତୁ keyword().isFourstate(); }
    VBasicDTypeKwd keyword() ଅଚଳ VL_MT_SAFE {  // Avoid using - use isSomething accessors instead
        ଫେରନ୍ତୁ m.m_keyword;
    }
    bool isBitLogic() ଅଚଳ { ଫେରନ୍ତୁ keyword().isBitLogic(); }
    bool isBit() ଅଚଳ { ଫେରନ୍ତୁ keyword().isBit(); }
    bool isCHandle() ଅଚଳ VL_MT_STABLE { ଫେରନ୍ତୁ keyword().isCHandle(); }
    bool isDouble() ଅଚଳ VL_MT_STABLE { ଫେରନ୍ତୁ keyword().isDouble(); }
    bool isEvent() ଅଚଳ VL_MT_STABLE { ଫେରନ୍ତୁ keyword() == VBasicDTypeKwd::EVENT; }
    bool isForkSync() ଅଚଳ VL_MT_SAFE { ଫେରନ୍ତୁ keyword() == VBasicDTypeKwd::FORK_SYNC; }
    bool isProcessRef() ଅଚଳ VL_MT_SAFE { ଫେରନ୍ତୁ keyword() == VBasicDTypeKwd::PROCESS_REFERENCE; }
    bool isDelayScheduler() ଅଚଳ VL_MT_SAFE {
        ଫେରନ୍ତୁ keyword() == VBasicDTypeKwd::DELAY_SCHEDULER;
    }
    bool isTriggerScheduler() ଅଚଳ VL_MT_SAFE {
        ଫେରନ୍ତୁ keyword() == VBasicDTypeKwd::TRIGGER_SCHEDULER;
    }
    bool isDynamicTriggerScheduler() ଅଚଳ VL_MT_SAFE {
        ଫେରନ୍ତୁ keyword() == VBasicDTypeKwd::DYNAMIC_TRIGGER_SCHEDULER;
    }
    bool isRandomGenerator() ଅଚଳ VL_MT_SAFE {
        ଫେରନ୍ତୁ keyword() == VBasicDTypeKwd::RANDOM_GENERATOR;
    }
    bool isStdRandomGenerator() ଅଚଳ VL_MT_SAFE {
        ଫେରନ୍ତୁ keyword() == VBasicDTypeKwd::RANDOM_STDGENERATOR;
    }
    bool isCovergroupInstHandle() ଅଚଳ VL_MT_SAFE {
        ଫେରନ୍ତୁ keyword() == VBasicDTypeKwd::COVERGROUP_INSTHANDLE;
    }
    bool isOpaque() ଅଚଳ VL_MT_SAFE { ଫେରନ୍ତୁ keyword().isOpaque(); }
    bool isString() ଅଚଳ VL_MT_STABLE { ଫେରନ୍ତୁ keyword().isString(); }
    bool isZeroInit() ଅଚଳ { ଫେରନ୍ତୁ keyword().isZeroInit(); }
    bool isRanged() ଅଚଳ { ଫେରନ୍ତୁ rangep() || m.m_nrange.ranged(); }
    bool isDpiBitVec() ଅଚଳ {  // DPI uses svBitVecVal
        ଫେରନ୍ତୁ keyword() == VBasicDTypeKwd::BIT && isRanged();
    }
    bool isDpiLogicVec() ଅଚଳ {  // DPI uses svLogicVecVal
        ଫେରନ୍ତୁ keyword().isFourstate() && !(keyword() == VBasicDTypeKwd::LOGIC && !isRanged());
    }
    bool isDpiCLayout() ଅଚଳ {
        ଫେରନ୍ତୁ keyword().isDpiCLayout()
               || (keyword().isSameish(VBasicDTypeKwd::LOGIC) && !isRanged());
    }
    bool isDpiPrimitive() ଅଚଳ {  // DPI uses a primitive type
        ଫେରନ୍ତୁ !isDpiBitVec() && !isDpiLogicVec();
    }
    // Generally the lo/hi/left/right funcs should be used instead of nrange()
    ଅଚଳ VNumRange& nrange() ଅଚଳ { ଫେରନ୍ତୁ m.m_nrange; }
    ଧାଡ଼ିଭିତରେ ପୂର୍ଣ୍ଣସଂଖ୍ୟା hi() ଅଚଳ;
    ଧାଡ଼ିଭିତରେ ପୂର୍ଣ୍ଣସଂଖ୍ୟା lo() ଅଚଳ;
    ଧାଡ଼ିଭିତରେ ପୂର୍ଣ୍ଣସଂଖ୍ୟା elements() ଅଚଳ;
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା ବାମ() ଅଚଳ { ଫେରନ୍ତୁ ascending() ? lo() : hi(); }  // How to show a declaration
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା ଡାହାଣ() ଅଚଳ { ଫେରନ୍ତୁ ascending() ? hi() : lo(); }
    ଧାଡ଼ିଭିତରେ bool ascending() ଅଚଳ;
    bool implicit() ଅଚଳ { ଫେରନ୍ତୁ keyword() == VBasicDTypeKwd::LOGIC_IMPLICIT; }
    bool untyped() ଅଚଳ { ଫେରନ୍ତୁ keyword() == VBasicDTypeKwd::UNTYPED; }
    VNumRange declRange() ଅଚଳ { ଫେରନ୍ତୁ isRanged() ? VNumRange{ବାମ(), ଡାହାଣ()} : VNumRange{}; }
    ଶୂନ୍ୟ cvtRangeConst();  // Convert to smaller representation
    bool isCompound() ଅଚଳ override { ଫେରନ୍ତୁ isString(); }
    bool isIntegralOrPacked() ଅଚଳ override { ଫେରନ୍ତୁ keyword().isIntNumeric(); }
};
class AstBracketArrayDType final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_DType {
    // Associative/Queue/Normal array data type, ie "[dtype_or_expr]"
    // only for early parsing then becomes another data type
    // @astgen op1 := childDTypep : Optional[AstNodeDType] // moved to refDTypep() in V3Width
    // @astgen op2 := elementsp : AstNode  // Number of elements in array
ସର୍ବସାଧାରଣ:
    AstBracketArrayDType(File_ଧାଡ଼ି* fl, VFlagChildDType, Ast_ଗଣ୍ଠି_DType* childDTypep,
                         Ast_ଗଣ୍ଠି* elementsp)
        : ASTGEN_SUPER_BracketArrayDType(fl) {
        this->childDTypep(childDTypep);
        this->elementsp(elementsp);
    }
    ASTGEN_MEMBERS_AstBracketArrayDType;
    bool similar_DType_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି_DType* samep) ଅଚଳ override { ଫେରନ୍ତୁ this == samep; }
    Ast_ଗଣ୍ଠି_DType* subDTypep() ଅଚଳ override VL_MT_STABLE { ଫେରନ୍ତୁ childDTypep(); }
    // METHODS
    // Will be removed in V3Width, which relies on this
    // being a child not a dtype pointed node
    bool maybePointedTo() ଅଚଳ override VL_MT_SAFE { ଫେରନ୍ତୁ false; }
    AstBasicDType* basicp() ଅଚଳ override VL_MT_STABLE { ଫେରନ୍ତୁ nullptr; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା widthAlignBytes() ଅଚଳ override { V3ERROR_NA_RETURN(0); }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା widthTotalBytes() ଅଚଳ override { V3ERROR_NA_RETURN(0); }
    bool isCompound() ଅଚଳ override { ଫେରନ୍ତୁ true; }
};
class AstCDType final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_DType {
    // Raw "C" data type passed directly to output
    string m_name;  // Name of data type, printed when do V3EmitC
ସର୍ବସାଧାରଣ:
    AstCDType(File_ଧାଡ଼ି* fl, ଅଚଳ string& name)
        : ASTGEN_SUPER_CDType(fl)
        , m_name{name} {
        dtypep(this);
    }

    ASTGEN_MEMBERS_AstCDType;
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* samep) ଅଚଳ override {
        ଅଚଳ AstCDType* ଅଚଳ asamep = VN_DBG_AS(samep, CDType);
        ଫେରନ୍ତୁ m_name == asamep->m_name;
    }
    bool similar_DType_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି_DType* samep) ଅଚଳ override { ଫେରନ୍ତୁ same_ଗଣ୍ଠି(samep); }
    string name() ଅଚଳ override VL_MT_STABLE { ଫେରନ୍ତୁ m_name; }
    string prettyDTypeName(bool) ଅଚଳ override { ଫେରନ୍ତୁ m_name; }
    // METHODS
    AstBasicDType* basicp() ଅଚଳ override VL_MT_STABLE { ଫେରନ୍ତୁ nullptr; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା widthAlignBytes() ଅଚଳ override { ଫେରନ୍ତୁ 8; }  // Assume
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା widthTotalBytes() ଅଚଳ override { ଫେରନ୍ତୁ 8; }  // Assume
    bool isCompound() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    // Return the global runtime type storing the requested number of bits.
    ସ୍ଥିର string typeToHold(ପୂର୍ଣ୍ଣସଂଖ୍ୟା width) {
        ଯଦି (width <= 8) ଫେରନ୍ତୁ "::CData";
        ଯଦି (width <= 16) ଫେରନ୍ତୁ "::SData";
        ଯଦି (width <= VL_IDATASIZE) ଫେରନ୍ତୁ "::IData";
        ଯଦି (width <= VL_QUADSIZE) ଫେରନ୍ତୁ "::QData";

        ଫେରନ୍ତୁ "::VlWide<" + std::to_string(VL_ଶବ୍ଦଗୁଡ଼ିକ_I(width)) + ">";
    }
};
class AstClassRefDType final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_DType {
    // Reference to a class
    // @astgen op1 := paramsp: List[AstPin]
    //
    // @astgen ptr := m_classp : Optional[AstClass]  // data type pointed to, BELOW the AstTypedef
    // @astgen ptr := m_classOrPackagep : Optional[AstNodeModule]  // Package hierarchy
    bool m_rawPointer = false;  // Emit as a non-owning C++ pointer rather than VlClassRef
ସର୍ବସାଧାରଣ:
    AstClassRefDType(File_ଧାଡ଼ି* fl, AstClass* classp, AstPin* paramsp)
        : ASTGEN_SUPER_ClassRefDType(fl)
        , m_classp{classp} {
        dtypep(this);
        addParamsp(paramsp);
    }
    ASTGEN_MEMBERS_AstClassRefDType;
    // METHODS
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* samep) ଅଚଳ override {
        ଅଚଳ AstClassRefDType* ଅଚଳ asamep = VN_DBG_AS(samep, ClassRefDType);
        ଫେରନ୍ତୁ (m_classp == asamep->m_classp && m_classOrPackagep == asamep->m_classOrPackagep
                && m_rawPointer == asamep->m_rawPointer);
    }
    bool similar_DType_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି_DType* samep) ଅଚଳ override;
    ଶୂନ୍ୟ dump(std::ostream& str = std::cout) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str = std::cout) ଅଚଳ override;
    ଶୂନ୍ୟ dumpSmall(std::ostream& str) ଅଚଳ override;
    string prettyDTypeName(bool full) ଅଚଳ override;
    string pretty_Name_ସନ୍ଦେଶ() ଅଚଳ override;
    string name() ଅଚଳ override VL_MT_STABLE;
    AstBasicDType* basicp() ଅଚଳ override VL_MT_STABLE { ଫେରନ୍ତୁ nullptr; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା widthAlignBytes() ଅଚଳ override { ଫେରନ୍ତୁ 0; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା widthTotalBytes() ଅଚଳ override { ଫେରନ୍ତୁ 0; }
    Ast_ଗଣ୍ଠି_DType* virtRefDTypep() ଅଚଳ override { ଫେରନ୍ତୁ nullptr; }
    ଶୂନ୍ୟ virtRefDTypep(Ast_ଗଣ୍ଠି_DType* nodep) override {}
    Ast_ଗଣ୍ଠି_DType* subDTypep() ଅଚଳ override VL_MT_STABLE { ଫେରନ୍ତୁ nullptr; }
    Ast_ଗଣ୍ଠି_Module* classOrPackagep() ଅଚଳ { ଫେରନ୍ତୁ m_classOrPackagep; }
    ଶୂନ୍ୟ classOrPackagep(Ast_ଗଣ୍ଠି_Module* nodep) { m_classOrPackagep = nodep; }
    AstClass* classp() ଅଚଳ VL_MT_STABLE { ଫେରନ୍ତୁ m_classp; }
    ଶୂନ୍ୟ classp(AstClass* nodep) { m_classp = nodep; }
    bool rawPointer() ଅଚଳ { ଫେରନ୍ତୁ m_rawPointer; }
    ଶୂନ୍ୟ rawPointer(bool flag) { m_rawPointer = flag; }
    ସ୍ଥିର ଶୂନ୍ୟ selfTest();
    bool isCompound() ଅଚଳ override { ଫେରନ୍ତୁ true; }
};
class AstConstDType final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_DType {
    // const data type, ie "const some_dtype var_name [2:0]"
    // ConstDType are removed in V3LinkLValue and become AstVar::isConst.
    // When more generic types are supported AstConstDType will be propagated further.
    // @astgen op1 := childDTypep : Optional[AstNodeDType]
    //
    // @astgen ptr := m_refDTypep : Optional[AstNodeDType]  // Inherit from this base data type
ସର୍ବସାଧାରଣ:
    AstConstDType(File_ଧାଡ଼ି* fl, VFlagChildDType, Ast_ଗଣ୍ଠି_DType* dtp)
        : ASTGEN_SUPER_ConstDType(fl) {
        childDTypep(dtp);  // Only for parser
        refDTypep(nullptr);  // V3Width will resolve
        dtypep(nullptr);  // V3Width will resolve
        widthFromSub(subDTypep());
    }
    ASTGEN_MEMBERS_AstConstDType;
    ଅଚଳ ଅକ୍ଷର* broken() ଅଚଳ override {
        BROKEN_RTN(!((m_refDTypep && !childDTypep()) || (!m_refDTypep && childDTypep())));
        ଫେରନ୍ତୁ nullptr;
    }
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* samep) ଅଚଳ override {
        ଅଚଳ AstConstDType* ଅଚଳ sp = VN_DBG_AS(samep, ConstDType);
        ଫେରନ୍ତୁ (m_refDTypep == sp->m_refDTypep);
    }
    bool similar_DType_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି_DType* samep) ଅଚଳ override {
        ଫେରନ୍ତୁ skipRefp()->similarDType(samep->skipRefp());
    }
    Ast_ଗଣ୍ଠି_DType* getChildDTypep() ଅଚଳ override { ଫେରନ୍ତୁ childDTypep(); }
    Ast_ଗଣ୍ଠି_DType* subDTypep() ଅଚଳ override VL_MT_STABLE {
        ଫେରନ୍ତୁ m_refDTypep ? m_refDTypep : childDTypep();
    }
    ଶୂନ୍ୟ refDTypep(Ast_ଗଣ୍ଠି_DType* nodep) { m_refDTypep = nodep; }
    Ast_ଗଣ୍ଠି_DType* virtRefDTypep() ଅଚଳ override { ଫେରନ୍ତୁ m_refDTypep; }
    ଶୂନ୍ୟ virtRefDTypep(Ast_ଗଣ୍ଠି_DType* nodep) override { refDTypep(nodep); }
    // METHODS
    AstBasicDType* basicp() ଅଚଳ override VL_MT_STABLE { ଫେରନ୍ତୁ subDTypep()->basicp(); }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା widthAlignBytes() ଅଚଳ override { ଫେରନ୍ତୁ subDTypep()->widthAlignBytes(); }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା widthTotalBytes() ଅଚଳ override { ଫେରନ୍ତୁ subDTypep()->widthTotalBytes(); }
    bool isCompound() ଅଚଳ override {
        v3fatalSrc("call isCompound on subdata type, not reference");
        ଫେରନ୍ତୁ false;
    }
};
class AstConstraintRefDType final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_DType {
    // For e.g. a reference to constraint for constraint_mode
ସର୍ବସାଧାରଣ:
    explicit AstConstraintRefDType(File_ଧାଡ଼ି* fl)
        : ASTGEN_SUPER_ConstraintRefDType(fl) {
        dtypep(this);
    }
    ASTGEN_MEMBERS_AstConstraintRefDType;
    bool hasDType() ଅଚଳ override VL_MT_SAFE { ଫେରନ୍ତୁ true; }
    bool maybePointedTo() ଅଚଳ override VL_MT_SAFE { ଫେରନ୍ତୁ true; }
    bool undead() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    Ast_ଗଣ୍ଠି_DType* subDTypep() ଅଚଳ override VL_MT_STABLE { ଫେରନ୍ତୁ nullptr; }
    Ast_ଗଣ୍ଠି_DType* virtRefDTypep() ଅଚଳ override { ଫେରନ୍ତୁ nullptr; }
    ଶୂନ୍ୟ virtRefDTypep(Ast_ଗଣ୍ଠି_DType* nodep) override {}
    bool similar_DType_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି_DType* samep) ଅଚଳ override { ଫେରନ୍ତୁ this == samep; }
    AstBasicDType* basicp() ଅଚଳ override VL_MT_STABLE { ଫେରନ୍ତୁ nullptr; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା widthAlignBytes() ଅଚଳ override { ଫେରନ୍ତୁ 1; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା widthTotalBytes() ଅଚଳ override { ଫେରନ୍ତୁ 1; }
    bool isCompound() ଅଚଳ override { ଫେରନ୍ତୁ false; }
};
class AstCoverCrossDType final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_DType {
    // Borrowed pointer to VlCoverCrossT<...> or construction-time VlCoverCrossDyn.
    ଅଚଳ uint32_t m_dimensions;
    ଅଚଳ uint32_t m_tuples;  // Fixed capacity number of cross tuples (VlCoverCrossT's Tuples)
    ଅଚଳ uint32_t m_bins;  // Fixed capacity number of explicit bins (VlCoverCrossT's Bins)
    ଅଚଳ uint32_t m_autoBins;  // Fixed capacity number of auto bins (VlCoverCrossT's AutoBins)
    ଅଚଳ uint64_t m_bin_ଶବ୍ଦଗୁଡ଼ିକ;  // Fixed capacity selection words (VlCoverCrossT's BinWords)
    ଅଚଳ bool m_dynamic;  // Bin layout is determined at covergroup construction

ସର୍ବସାଧାରଣ:
    AstCoverCrossDType(File_ଧାଡ଼ି* fl, uint32_t dimensions, uint32_t tuples, uint32_t bins,
                       uint32_t autoBins, uint64_t bin_ଶବ୍ଦଗୁଡ଼ିକ, bool dynamic = false)
        : ASTGEN_SUPER_CoverCrossDType(fl)
        , m_dimensions{dimensions}
        , m_tuples{tuples}
        , m_bins{bins}
        , m_autoBins{autoBins}
        , m_bin_ଶବ୍ଦଗୁଡ଼ିକ{bin_ଶବ୍ଦଗୁଡ଼ିକ}
        , m_dynamic{dynamic} {
        dtypep(this);
    }
    ASTGEN_MEMBERS_AstCoverCrossDType;
    ଅଚଳ ଅକ୍ଷର* broken() ଅଚଳ override {
        BROKEN_RTN(m_dimensions == 0);
        ଫେରନ୍ତୁ nullptr;
    }
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* samep) ଅଚଳ override {  // LCOV_EXCL_START
        ଅଚଳ AstCoverCrossDType* ଅଚଳ sp = VN_DBG_AS(samep, CoverCrossDType);
        ଫେରନ୍ତୁ dimensions() == sp->dimensions() && tuples() == sp->tuples() && bins() == sp->bins()
               && autoBins() == sp->autoBins() && bin_ଶବ୍ଦଗୁଡ଼ିକ() == sp->bin_ଶବ୍ଦଗୁଡ଼ିକ()
               && isDynamic() == sp->isDynamic();
    }  // LCOV_EXCL_STOP
    bool similar_DType_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି_DType* samep) ଅଚଳ override { ଫେରନ୍ତୁ this == samep; }
    ଶୂନ୍ୟ dump(std::ostream& str) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str) ଅଚଳ override;
    ଶୂନ୍ୟ dumpSmall(std::ostream& str) ଅଚଳ override;
    uint32_t dimensions() ଅଚଳ { ଫେରନ୍ତୁ m_dimensions; }
    uint32_t tuples() ଅଚଳ { ଫେରନ୍ତୁ m_tuples; }
    uint32_t bins() ଅଚଳ { ଫେରନ୍ତୁ m_bins; }
    uint32_t autoBins() ଅଚଳ { ଫେରନ୍ତୁ m_autoBins; }
    uint64_t bin_ଶବ୍ଦଗୁଡ଼ିକ() ଅଚଳ { ଫେରନ୍ତୁ m_bin_ଶବ୍ଦଗୁଡ଼ିକ; }
    bool isDynamic() ଅଚଳ { ଫେରନ୍ତୁ m_dynamic; }
    string cppTemplateArgs() ଅଚଳ;
    AstBasicDType* basicp() ଅଚଳ override VL_MT_STABLE { ଫେରନ୍ତୁ nullptr; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା widthAlignBytes() ଅଚଳ override { ଫେରନ୍ତୁ ଆକାର(ଶୂନ୍ୟ*); }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା widthTotalBytes() ଅଚଳ override { ଫେରନ୍ତୁ ଆକାର(ଶୂନ୍ୟ*); }
    bool isCompound() ଅଚଳ override { ଫେରନ୍ତୁ true; }
};
class AstCoverpointDType final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_DType {
    // Borrowed pointer to a covergroup coverpoint runtime, 'VlCoverpointT<hitBound>*'.
    // Follows pattern of AstQueueDType in capturing the compile-time max bin overlap
    // template argument.
    uint32_t m_hitBound;  // VlCoverpointT<> template argument; hit list size, >= 1
ସର୍ବସାଧାରଣ:
    AstCoverpointDType(File_ଧାଡ଼ି* fl, uint32_t hitBound)
        : ASTGEN_SUPER_CoverpointDType(fl)
        , m_hitBound{hitBound} {
        dtypep(this);
    }
    ASTGEN_MEMBERS_AstCoverpointDType;
    ଅଚଳ ଅକ୍ଷର* broken() ଅଚଳ override {
        BROKEN_RTN(m_hitBound < 1);
        ଫେରନ୍ତୁ nullptr;
    }
    // V3Covergroup interns these one-per-hitBound into the type table, so identity is
    // equality; there is never a second node with the same bound to compare against.
    bool similar_DType_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି_DType* samep) ଅଚଳ override { ଫେରନ୍ତୁ this == samep; }
    ଶୂନ୍ୟ dumpSmall(std::ostream& str) ଅଚଳ override;
    // ACCESSORS
    uint32_t hitBound() ଅଚଳ { ଫେରନ୍ତୁ m_hitBound; }
    // METHODS
    AstBasicDType* basicp() ଅଚଳ override VL_MT_STABLE { ଫେରନ୍ତୁ nullptr; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା widthAlignBytes() ଅଚଳ override { ଫେରନ୍ତୁ ଆକାର(ଶୂନ୍ୟ*); }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା widthTotalBytes() ଅଚଳ override { ଫେରନ୍ତୁ ଆକାର(ଶୂନ୍ୟ*); }
    bool isCompound() ଅଚଳ override { ଫେରନ୍ତୁ true; }
};
class AstDefImplicitDType final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_DType {
    // For parsing enum/struct/unions that are declared with a variable rather than typedef
    // This allows "var enum {...} a,b" to share the enum definition for both variables
    // After link, these become typedefs
    // @astgen op1 := childDTypep : Optional[AstNodeDType]
    string m_name;  // Data type name
    ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_unique_ଅଙ୍କ;  // Unique ID distinguishing this dtype instance, for hashing/naming

ସର୍ବସାଧାରଣ:
    AstDefImplicitDType(File_ଧାଡ଼ି* fl, ଅଚଳ string& name, VFlagChildDType, Ast_ଗଣ୍ଠି_DType* dtp)
        : ASTGEN_SUPER_DefImplicitDType(fl)
        , m_name{name}
        , m_unique_ଅଙ୍କ{unique_ଅଙ୍କ_Inc()} {
        childDTypep(dtp);  // Only for parser
        dtypep(nullptr);  // V3Width will resolve
    }
    AstDefImplicitDType(ଅଚଳ AstDefImplicitDType& other)
        : Ast_ଗଣ୍ଠି_DType(other)
        , m_name{other.m_name}
        , m_unique_ଅଙ୍କ{unique_ଅଙ୍କ_Inc()} {}
    ASTGEN_MEMBERS_AstDefImplicitDType;
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା unique_ଅଙ୍କ() ଅଚଳ { ଫେରନ୍ତୁ m_unique_ଅଙ୍କ; }
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* samep) ଅଚଳ override {
        ଅଚଳ AstDefImplicitDType* ଅଚଳ sp = VN_DBG_AS(samep, DefImplicitDType);
        ଫେରନ୍ତୁ unique_ଅଙ୍କ() == sp->unique_ଅଙ୍କ();
    }
    bool similar_DType_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି_DType* samep) ଅଚଳ override { ଫେରନ୍ତୁ same_ଗଣ୍ଠି(samep); }
    Ast_ଗଣ୍ଠି_DType* getChildDTypep() ଅଚଳ override { ଫେରନ୍ତୁ childDTypep(); }
    Ast_ଗଣ୍ଠି_DType* subDTypep() ଅଚଳ override VL_MT_STABLE {
        ଫେରନ୍ତୁ dtypep() ? dtypep() : childDTypep();
    }
    // METHODS
    // op1 = Range of variable
    Ast_ଗଣ୍ଠି_DType* dtypeSkipRefp() ଅଚଳ { ଫେରନ୍ତୁ dtypep()->skipRefp(); }
    AstBasicDType* basicp() ଅଚଳ override VL_MT_STABLE { ଫେରନ୍ତୁ subDTypep()->basicp(); }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା widthAlignBytes() ଅଚଳ override { ଫେରନ୍ତୁ dtypep()->widthAlignBytes(); }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା widthTotalBytes() ଅଚଳ override { ଫେରନ୍ତୁ dtypep()->widthTotalBytes(); }
    string name() ଅଚଳ override VL_MT_STABLE { ଫେରନ୍ତୁ m_name; }
    ଶୂନ୍ୟ name(ଅଚଳ string& flag) override { m_name = flag; }
    bool isCompound() ଅଚଳ override { ଫେରନ୍ତୁ false; }
};
class AstDynArrayDType final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_DType {
    // Dynamic array data type, ie "[]"
    // @astgen op1 := childDTypep : Optional[AstNodeDType] // moved to refDTypep() in V3Width
    //
    // @astgen ptr := m_refDTypep : Optional[AstNodeDType]  // Elements of this type (post-width)
ସର୍ବସାଧାରଣ:
    AstDynArrayDType(File_ଧାଡ଼ି* fl, VFlagChildDType, Ast_ଗଣ୍ଠି_DType* dtp)
        : ASTGEN_SUPER_DynArrayDType(fl) {
        childDTypep(dtp);  // Only for parser
        refDTypep(nullptr);
        dtypep(nullptr);  // V3Width will resolve
    }
    AstDynArrayDType(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_DType* dtp)
        : ASTGEN_SUPER_DynArrayDType(fl) {
        refDTypep(dtp);
        dtypep(nullptr);  // V3Width will resolve
    }
    ASTGEN_MEMBERS_AstDynArrayDType;
    ଅଚଳ ଅକ୍ଷର* broken() ଅଚଳ override {
        BROKEN_RTN(!((m_refDTypep && !childDTypep()) || (!m_refDTypep && childDTypep())));
        ଫେରନ୍ତୁ nullptr;
    }
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* samep) ଅଚଳ override {
        ଅଚଳ AstDynArrayDType* ଅଚଳ asamep = VN_DBG_AS(samep, DynArrayDType);
        ଯଦି (!asamep->subDTypep()) ଫେରନ୍ତୁ false;
        ଫେରନ୍ତୁ subDTypep() == asamep->subDTypep();
    }
    bool similar_DType_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି_DType* samep) ଅଚଳ override {
        ଅଚଳ AstDynArrayDType* ଅଚଳ asamep = VN_DBG_AS(samep, DynArrayDType);
        ଫେରନ୍ତୁ asamep->subDTypep() && subDTypep()->similarDType(asamep->subDTypep());
    }
    string prettyDTypeName(bool full) ଅଚଳ override;
    ଶୂନ୍ୟ dumpSmall(std::ostream& str) ଅଚଳ override;
    Ast_ଗଣ୍ଠି_DType* getChildDTypep() ଅଚଳ override { ଫେରନ୍ତୁ childDTypep(); }
    Ast_ଗଣ୍ଠି_DType* subDTypep() ଅଚଳ override VL_MT_STABLE {
        ଫେରନ୍ତୁ m_refDTypep ? m_refDTypep : childDTypep();
    }
    bool isAggregateType() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    ଶୂନ୍ୟ refDTypep(Ast_ଗଣ୍ଠି_DType* nodep) { m_refDTypep = nodep; }
    Ast_ଗଣ୍ଠି_DType* virtRefDTypep() ଅଚଳ override { ଫେରନ୍ତୁ m_refDTypep; }
    ଶୂନ୍ୟ virtRefDTypep(Ast_ଗଣ୍ଠି_DType* nodep) override { refDTypep(nodep); }
    // METHODS
    AstBasicDType* basicp() ଅଚଳ override VL_MT_STABLE { ଫେରନ୍ତୁ nullptr; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା widthAlignBytes() ଅଚଳ override { ଫେରନ୍ତୁ subDTypep()->widthAlignBytes(); }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା widthTotalBytes() ଅଚଳ override { ଫେରନ୍ତୁ subDTypep()->widthTotalBytes(); }
    bool isCompound() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool isDynamicallySized() ଅଚଳ override { ଫେରନ୍ତୁ true; }
};
class AstEmptyQueueDType final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_DType {
    // For EmptyQueue
ସର୍ବସାଧାରଣ:
    explicit AstEmptyQueueDType(File_ଧାଡ଼ି* fl)
        : ASTGEN_SUPER_EmptyQueueDType(fl) {
        dtypep(this);
    }
    ASTGEN_MEMBERS_AstEmptyQueueDType;
    ଶୂନ୍ୟ dumpSmall(std::ostream& str) ଅଚଳ override;
    bool hasDType() ଅଚଳ override VL_MT_SAFE { ଫେରନ୍ତୁ true; }
    bool maybePointedTo() ଅଚଳ override VL_MT_SAFE { ଫେରନ୍ତୁ true; }
    bool undead() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    Ast_ଗଣ୍ଠି_DType* subDTypep() ଅଚଳ override VL_MT_STABLE { ଫେରନ୍ତୁ nullptr; }
    Ast_ଗଣ୍ଠି_DType* virtRefDTypep() ଅଚଳ override { ଫେରନ୍ତୁ nullptr; }
    ଶୂନ୍ୟ virtRefDTypep(Ast_ଗଣ୍ଠି_DType* nodep) override {}
    bool similar_DType_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି_DType* samep) ଅଚଳ override { ଫେରନ୍ତୁ this == samep; }
    AstBasicDType* basicp() ଅଚଳ override VL_MT_STABLE { ଫେରନ୍ତୁ nullptr; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା widthAlignBytes() ଅଚଳ override { ଫେରନ୍ତୁ 1; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା widthTotalBytes() ଅଚଳ override { ଫେରନ୍ତୁ 1; }
    bool isCompound() ଅଚଳ override { ଫେରନ୍ତୁ false; }
};
class AstEnumDType final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_DType {
    // Parents: TYPEDEF/MODULE
    // @astgen op1 := childDTypep : Optional[AstNodeDType]
    // @astgen op2 := itemsp : List[AstEnumItem]
    //
    // @astgen ptr := m_refDTypep : Optional[AstNodeDType]  // Elements of this type (post-width)
    string m_name;  // Name from upper typedef, if any
    string m_typedefName;  // Typedef's dtypeName() once moved off the typedef declaring it
    ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_unique_ଅଙ୍କ;  // Unique ID distinguishing this dtype instance, for hashing/naming

ସର୍ବସାଧାରଣ:
    AstEnumDType(File_ଧାଡ଼ି* fl, VFlagChildDType, Ast_ଗଣ୍ଠି_DType* dtp, AstEnumItem* itemsp)
        : ASTGEN_SUPER_EnumDType(fl)
        , m_unique_ଅଙ୍କ{unique_ଅଙ୍କ_Inc()} {
        childDTypep(dtp);  // Only for parser
        refDTypep(nullptr);
        addItemsp(itemsp);
        dtypep(nullptr);  // V3Width will resolve
        widthFromSub(subDTypep());
    }
    AstEnumDType(ଅଚଳ AstEnumDType& other)
        : Ast_ଗଣ୍ଠି_DType{other}
        , m_name{other.m_name}
        , m_typedefName{other.m_typedefName}
        , m_unique_ଅଙ୍କ{unique_ଅଙ୍କ_Inc()} {}
    ASTGEN_MEMBERS_AstEnumDType;

    ଅଚଳ ଅକ୍ଷର* broken() ଅଚଳ override;
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା unique_ଅଙ୍କ() ଅଚଳ { ଫେରନ୍ତୁ m_unique_ଅଙ୍କ; }
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* samep) ଅଚଳ override {
        ଅଚଳ AstEnumDType* ଅଚଳ sp = VN_DBG_AS(samep, EnumDType);
        ଫେରନ୍ତୁ unique_ଅଙ୍କ() == sp->unique_ଅଙ୍କ();
    }
    bool similar_DType_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି_DType* samep) ଅଚଳ override { ଫେରନ୍ତୁ same_ଗଣ୍ଠି(samep); }
    Ast_ଗଣ୍ଠି_DType* getChildDTypep() ଅଚଳ override { ଫେରନ୍ତୁ childDTypep(); }
    Ast_ଗଣ୍ଠି_DType* subDTypep() ଅଚଳ override VL_MT_STABLE {
        ଫେରନ୍ତୁ m_refDTypep ? m_refDTypep : childDTypep();
    }
    ଶୂନ୍ୟ refDTypep(Ast_ଗଣ୍ଠି_DType* nodep) { m_refDTypep = nodep; }
    Ast_ଗଣ୍ଠି_DType* virtRefDTypep() ଅଚଳ override { ଫେରନ୍ତୁ m_refDTypep; }
    ଶୂନ୍ୟ virtRefDTypep(Ast_ଗଣ୍ଠି_DType* nodep) override { refDTypep(nodep); }
    string name() ଅଚଳ override VL_MT_STABLE { ଫେରନ୍ତୁ m_name; }
    ଶୂନ୍ୟ name(ଅଚଳ string& flag) override { m_name = flag; }
    // Keep the name the typedef declaring it gives, as it moves off it to the type table
    ଶୂନ୍ୟ typedefName(ଅଚଳ string& name) { m_typedefName = name; }
    ଶୂନ୍ୟ dump(std::ostream& str = std::cout) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str = std::cout) ଅଚଳ override;
    ଶୂନ୍ୟ dumpSmall(std::ostream& str) ଅଚଳ override;
    string prettyDTypeName(bool full) ଅଚଳ override;
    // METHODS
    AstBasicDType* basicp() ଅଚଳ override VL_MT_STABLE { ଫେରନ୍ତୁ subDTypep()->basicp(); }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା widthAlignBytes() ଅଚଳ override { ଫେରନ୍ତୁ subDTypep()->widthAlignBytes(); }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା widthTotalBytes() ଅଚଳ override { ଫେରନ୍ତୁ subDTypep()->widthTotalBytes(); }
    size_t item_ଗଣନା() ଅଚଳ {
        size_t ଗଣନା_2 = 0;
        ପାଇଁ (Ast_ଗଣ୍ଠି* itemp = itemsp(); itemp; itemp = itemp->nextp()) ++ଗଣନା_2;
        ଫେରନ୍ତୁ ଗଣନା_2;
    }
    bool isCompound() ଅଚଳ override { ଫେରନ୍ତୁ false; }
};

class AstIfaceGenericDType final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_DType {
    // Generic interface that will be replaced with AstIfaceRefDType
    File_ଧାଡ଼ି* m_modportFileline;  // Where modport token was
    // dist-ast-dump-suppress  // Part of name()
    string m_modportName;  // "" = no modport
ସର୍ବସାଧାରଣ:
    explicit AstIfaceGenericDType(File_ଧାଡ଼ି* fl)
        : ASTGEN_SUPER_IfaceGenericDType(fl) {
        dtypep(this);
    }
    AstIfaceGenericDType(File_ଧାଡ଼ି* fl, File_ଧାଡ଼ି* modportFl, ଅଚଳ string& modport)
        : ASTGEN_SUPER_IfaceGenericDType(fl)
        , m_modportFileline{modportFl}
        , m_modportName{modport} {
        dtypep(this);
    }
    ASTGEN_MEMBERS_AstIfaceGenericDType;
    ଶୂନ୍ୟ dumpSmall(std::ostream& str) ଅଚଳ override;
    bool hasDType() ଅଚଳ override VL_MT_SAFE { ଫେରନ୍ତୁ true; }
    bool maybePointedTo() ଅଚଳ override VL_MT_SAFE { ଫେରନ୍ତୁ true; }
    bool undead() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    Ast_ଗଣ୍ଠି_DType* subDTypep() ଅଚଳ override VL_MT_STABLE { ଫେରନ୍ତୁ nullptr; }
    Ast_ଗଣ୍ଠି_DType* virtRefDTypep() ଅଚଳ override { ଫେରନ୍ତୁ nullptr; }
    ଶୂନ୍ୟ virtRefDTypep(Ast_ଗଣ୍ଠି_DType* nodep) override {}
    bool similar_DType_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି_DType* samep) ଅଚଳ override { ଫେରନ୍ତୁ this == samep; }
    AstBasicDType* basicp() ଅଚଳ override VL_MT_STABLE { ଫେରନ୍ତୁ nullptr; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା widthAlignBytes() ଅଚଳ override { ଫେରନ୍ତୁ 1; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା widthTotalBytes() ଅଚଳ override { ଫେରନ୍ତୁ 1; }
    string modportName() ଅଚଳ { ଫେରନ୍ତୁ m_modportName; }
    bool isModport() { ଫେରନ୍ତୁ !m_modportName.empty(); }
    bool isCompound() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    File_ଧାଡ଼ି* modportFileline() ଅଚଳ { ଫେରନ୍ତୁ m_modportFileline; }
    string name() ଅଚଳ override { ଫେରନ୍ତୁ m_modportName; }
};

class AstIfaceRefDType final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_DType {
    // Reference to an interface, either for a port, or inside parent cell
    // @astgen op1 := paramsp : List[AstPin]
    //
    // @astgen ptr := m_ifacep : Optional[AstIface]  // Interface; cellp() should override
    // @astgen ptr := m_cellp : Optional[AstCell]  // When exact parent cell known; not a guess
    // @astgen ptr := m_modportp : Optional[AstModport]  // nullptr = unlinked or no modport
    File_ଧାଡ଼ି* m_modportFileline;  // Where modport token was
    string m_cellName;  // "" = no cell, such as when connects to 'input' iface
    string m_ifaceName;  // Interface name
    string m_modportName;  // "" = no modport
    bool m_virtual = false;  // True if virtual interface
ସର୍ବସାଧାରଣ:
    AstIfaceRefDType(File_ଧାଡ଼ି* fl, ଅଚଳ string& cellName, ଅଚଳ string& ifaceName)
        : ASTGEN_SUPER_IfaceRefDType(fl)
        , m_modportFileline{nullptr}
        , m_cellName{cellName}
        , m_ifaceName{ifaceName}
        , m_modportName{""} {}
    AstIfaceRefDType(File_ଧାଡ଼ି* fl, File_ଧାଡ଼ି* modportFl, ଅଚଳ string& cellName,
                     ଅଚଳ string& ifaceName, ଅଚଳ string& modport)
        : ASTGEN_SUPER_IfaceRefDType(fl)
        , m_modportFileline{modportFl}
        , m_cellName{cellName}
        , m_ifaceName{ifaceName}
        , m_modportName{modport} {}
    AstIfaceRefDType(File_ଧାଡ଼ି* fl, File_ଧାଡ଼ି* modportFl, ଅଚଳ string& cellName,
                     ଅଚଳ string& ifaceName, ଅଚଳ string& modport, AstPin* paramsp)
        : ASTGEN_SUPER_IfaceRefDType(fl)
        , m_modportFileline{modportFl}
        , m_cellName{cellName}
        , m_ifaceName{ifaceName}
        , m_modportName{modport} {
        addParamsp(paramsp);
    }
    ASTGEN_MEMBERS_AstIfaceRefDType;

    // METHODS
    ଶୂନ୍ୟ dump(std::ostream& str = std::cout) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str = std::cout) ଅଚଳ override;
    ଶୂନ୍ୟ dumpSmall(std::ostream& str) ଅଚଳ override;
    string prettyDTypeName(bool full) ଅଚଳ override;
    AstBasicDType* basicp() ଅଚଳ override VL_MT_STABLE { ଫେରନ୍ତୁ nullptr; }
    bool similar_DType_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି_DType* samep) ଅଚଳ override {
        // Each occurrence of a virtual interface type parses to its own node,
        // so pointer identity is insufficient; compare the referenced
        // interface instead, same equivalence as AstNode::computeCastable uses.
        ଅଚଳ AstIfaceRefDType* ଅଚଳ asamep = VN_DBG_AS(samep, IfaceRefDType);
        ଫେରନ୍ତୁ ifaceViaCellp() && ifaceViaCellp() == asamep->ifaceViaCellp()
               && modportName() == asamep->modportName() && isVirtual() == asamep->isVirtual();
    }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା widthAlignBytes() ଅଚଳ override { ଫେରନ୍ତୁ 0; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା widthTotalBytes() ଅଚଳ override { ଫେରନ୍ତୁ 0; }
    bool isVirtual() ଅଚଳ { ଫେରନ୍ତୁ m_virtual; }
    ଶୂନ୍ୟ isVirtual(bool flag) {
        m_virtual = flag;
        ଯଦି (flag) v3Global.setHasVirtIfaces();
    }
    File_ଧାଡ଼ି* modportFileline() ଅଚଳ { ଫେରନ୍ତୁ m_modportFileline; }
    ଶୂନ୍ୟ modportFileline(File_ଧାଡ଼ି* ଅଚଳ modportFileline) { m_modportFileline = modportFileline; }
    string cellName() ଅଚଳ { ଫେରନ୍ତୁ m_cellName; }
    ଶୂନ୍ୟ cellName(ଅଚଳ string& name) { m_cellName = name; }
    string ifaceName() ଅଚଳ { ଫେରନ୍ତୁ m_ifaceName; }
    string ifaceNameQ() ଅଚଳ { ଫେରନ୍ତୁ "'" + prettyName(ifaceName()) + "'"; }
    ଶୂନ୍ୟ ifaceName(ଅଚଳ string& name) { m_ifaceName = name; }
    ଶୂନ୍ୟ modportName(ଅଚଳ string& modportName) { m_modportName = modportName; }
    string modportName() ଅଚଳ { ଫେରନ୍ତୁ m_modportName; }
    AstIface* ifaceViaCellp() ଅଚଳ;  // Use cellp or ifacep
    AstIface* ifacep() ଅଚଳ { ଫେରନ୍ତୁ m_ifacep; }
    ଶୂନ୍ୟ ifacep(AstIface* nodep) { m_ifacep = nodep; }
    AstCell* cellp() ଅଚଳ { ଫେରନ୍ତୁ m_cellp; }
    ଶୂନ୍ୟ cellp(AstCell* nodep) { m_cellp = nodep; }
    AstModport* modportp() ଅଚଳ { ଫେରନ୍ତୁ m_modportp; }
    ଶୂନ୍ୟ modportp(AstModport* modportp) { m_modportp = modportp; }
    bool isModport() { ଫେରନ୍ତୁ !m_modportName.empty(); }
    bool isCompound() ଅଚଳ override { ଫେରନ୍ତୁ true; }  // But not relevant
};
class AstMemberDType final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_DType {
    // A member of a struct/union
    // PARENT: AstNodeUOrStructDType
    // @astgen op1 := childDTypep : Optional[AstNodeDType]
    // @astgen op3 := valuep : Optional[AstNode]
    //
    // @astgen ptr := m_refDTypep : Optional[AstNodeDType]  // Elements of this type (post-width)
    string m_name;  // Name of variable
    string m_tag;  // Holds the string of the verilator tag -- used in JSON output.
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_lsb = -1;  // Within this level's packed struct, the LSB of the first bit of the member
    bool m_constrainedRand = false;  // Member has a constraint expression
    VRandAttr m_rand;  // Randomizability of this member (rand, randc, etc)
ସର୍ବସାଧାରଣ:
    AstMemberDType(File_ଧାଡ଼ି* fl, ଅଚଳ string& name, VFlagChildDType, Ast_ଗଣ୍ଠି_DType* dtp,
                   Ast_ଗଣ୍ଠି* valuep)
        : ASTGEN_SUPER_MemberDType(fl)
        , m_name{name} {
        childDTypep(dtp);  // Only for parser
        this->valuep(valuep);
        dtypep(nullptr);  // V3Width will resolve
        refDTypep(nullptr);
    }
    AstMemberDType(File_ଧାଡ଼ି* fl, ଅଚଳ string& name, Ast_ଗଣ୍ଠି_DType* dtp)
        : ASTGEN_SUPER_MemberDType(fl)
        , m_name{name} {
        UASSERT(dtp, "AstMember created with no dtype");
        refDTypep(dtp);
        dtypep(this);
        widthFromSub(subDTypep());
    }
    ASTGEN_MEMBERS_AstMemberDType;
    ଶୂନ୍ୟ dump(std::ostream& str = std::cout) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str = std::cout) ଅଚଳ override;
    ଶୂନ୍ୟ dumpSmall(std::ostream& str) ଅଚଳ override;
    string prettyDTypeName(bool full) ଅଚଳ override;
    string name() ଅଚଳ override VL_MT_STABLE { ଫେରନ୍ତୁ m_name; }  // * = Var name
    bool hasDType() ଅଚଳ override VL_MT_SAFE { ଫେରନ୍ତୁ true; }
    bool maybePointedTo() ଅଚଳ override VL_MT_SAFE { ଫେରନ୍ତୁ true; }
    Ast_ଗଣ୍ଠି_DType* getChildDTypep() ଅଚଳ override { ଫେରନ୍ତୁ childDTypep(); }
    Ast_ଗଣ୍ଠି_UOr_Struct_DType* getChildStructp();
    Ast_ଗଣ୍ଠି_DType* subDTypep() ଅଚଳ override VL_MT_STABLE {
        ଫେରନ୍ତୁ m_refDTypep ? m_refDTypep : childDTypep();
    }
    ଶୂନ୍ୟ refDTypep(Ast_ଗଣ୍ଠି_DType* nodep) { m_refDTypep = nodep; }
    Ast_ଗଣ୍ଠି_DType* virtRefDTypep() ଅଚଳ override { ଫେରନ୍ତୁ m_refDTypep; }
    ଶୂନ୍ୟ virtRefDTypep(Ast_ଗଣ୍ଠି_DType* nodep) override { refDTypep(nodep); }
    bool similar_DType_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି_DType* samep) ଅଚଳ override { ଫେରନ୍ତୁ this == samep; }
    //
    // (Slow) recurse down to find basic data type (Note don't need virtual -
    // AstVar isn't a NodeDType)
    AstBasicDType* basicp() ଅଚଳ override VL_MT_STABLE { ଫେରନ୍ତୁ subDTypep()->basicp(); }
    // op1 = Range of variable (Note don't need virtual - AstVar isn't a NodeDType)
    Ast_ଗଣ୍ଠି_DType* dtypeSkipRefp() ଅଚଳ { ଫେରନ୍ତୁ subDTypep()->skipRefp(); }
    // (Slow) recurses - Structure alignment 1,2,4 or 8 bytes (arrays affect this)
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା widthAlignBytes() ଅଚଳ override { ଫେରନ୍ତୁ subDTypep()->widthAlignBytes(); }
    // (Slow) recurses - Width in bytes rounding up 1,2,4,8,12,...
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା widthTotalBytes() ଅଚଳ override { ଫେରନ୍ତୁ subDTypep()->widthTotalBytes(); }
    // METHODS
    ଶୂନ୍ୟ name(ଅଚଳ string& name) override { m_name = name; }
    string tag() ଅଚଳ override { ଫେରନ୍ତୁ m_tag; }
    ଶୂନ୍ୟ tag(ଅଚଳ string& text) override { m_tag = text; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା lsb() ଅଚଳ { ଫେରନ୍ତୁ m_lsb; }
    ଶୂନ୍ୟ lsb(ପୂର୍ଣ୍ଣସଂଖ୍ୟା lsb) { m_lsb = lsb; }
    bool isCompound() ଅଚଳ override {
        v3fatalSrc("call isCompound on subdata type, not reference");
        ଫେରନ୍ତୁ false;
    }
    bool isConstrainedRand() ଅଚଳ { ଫେରନ୍ତୁ m_constrainedRand; }
    ଶୂନ୍ୟ markConstrainedRand(bool flag) { m_constrainedRand = flag; }
    VRandAttr rand() ଅଚଳ { ଫେରନ୍ତୁ m_rand; }
    ଶୂନ୍ୟ rand(ଅଚଳ VRandAttr flag) { m_rand = flag; }
};
class AstNBACommitQueueDType final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_DType {
    // @astgen ptr := m_subDTypep : AstNodeDType  // Type of the corresponding variable
    ଅଚଳ bool m_partial;  // Partial element update required

ସର୍ବସାଧାରଣ:
    AstNBACommitQueueDType(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_DType* subDTypep, bool partial)
        : ASTGEN_SUPER_NBACommitQueueDType(fl)
        , m_partial{partial}
        , m_subDTypep{subDTypep} {
        dtypep(this);
    }
    ASTGEN_MEMBERS_AstNBACommitQueueDType;
    ଶୂନ୍ୟ dump(std::ostream& str) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str) ଅଚଳ override;
    Ast_ଗଣ୍ଠି_DType* subDTypep() ଅଚଳ override VL_MT_STABLE { ଫେରନ୍ତୁ m_subDTypep; }
    bool partial() ଅଚଳ { ଫେରନ୍ତୁ m_partial; }
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* samep) ଅଚଳ override {
        ଅଚଳ AstNBACommitQueueDType* ଅଚଳ asamep = VN_DBG_AS(samep, NBACommitQueueDType);
        ଫେରନ୍ତୁ m_partial == asamep->m_partial;
    }
    bool similar_DType_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି_DType* samep) ଅଚଳ override { ଫେରନ୍ତୁ this == samep; }
    AstBasicDType* basicp() ଅଚଳ override VL_MT_STABLE { ଫେରନ୍ତୁ nullptr; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା widthAlignBytes() ଅଚଳ override { ଫେରନ୍ତୁ 1; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା widthTotalBytes() ଅଚଳ override { ଫେରନ୍ତୁ 24; }
    bool isCompound() ଅଚଳ override { ଫେରନ୍ତୁ true; }
};
class AstParamTypeDType final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_DType {
    // Parents: MODULE
    // A parameter type statement; much like a var or typedef
    // @astgen op1 := childDTypep : Optional[AstNodeDType]
    ଅଚଳ VVarType m_varType;  // Type of variable (for localparam vs. param)
    ଅଚଳ VFwdType m_fwdType;  // Forward type for lint check
    string m_name;  // Name of variable
ସର୍ବସାଧାରଣ:
    AstParamTypeDType(File_ଧାଡ଼ି* fl, VVarType type, VFwdType fwdType, ଅଚଳ string& name,
                      VFlagChildDType, Ast_ଗଣ୍ଠି_DType* dtp)
        : ASTGEN_SUPER_ParamTypeDType(fl)
        , m_varType{type}
        , m_fwdType{fwdType}
        , m_name{name} {
        childDTypep(dtp);  // Only for parser
        dtypep(nullptr);  // V3Width will resolve
    }
    ASTGEN_MEMBERS_AstParamTypeDType;
    ଶୂନ୍ୟ dump(std::ostream& str = std::cout) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str = std::cout) ଅଚଳ override;
    string prettyDTypeName(bool full) ଅଚଳ override;
    Ast_ଗଣ୍ଠି_DType* getChildDTypep() ଅଚଳ override { ଫେରନ୍ତୁ childDTypep(); }
    Ast_ଗଣ୍ଠି_DType* subDTypep() ଅଚଳ override VL_MT_STABLE {
        ଫେରନ୍ତୁ dtypep() ? dtypep() : childDTypep();
    }
    AstBasicDType* basicp() ଅଚଳ override VL_MT_STABLE { ଫେରନ୍ତୁ subDTypep()->basicp(); }
    bool similar_DType_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି_DType* samep) ଅଚଳ override {
        ଅଚଳ AstParamTypeDType* ଅଚଳ sp = VN_DBG_AS(samep, ParamTypeDType);
        ଫେରନ୍ତୁ this->subDTypep()->similarDType(sp->subDTypep());
    }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା widthAlignBytes() ଅଚଳ override { ଫେରନ୍ତୁ dtypep()->widthAlignBytes(); }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା widthTotalBytes() ଅଚଳ override { ଫେରନ୍ତୁ dtypep()->widthTotalBytes(); }
    // METHODS
    string name() ଅଚଳ override VL_MT_STABLE { ଫେରନ୍ତୁ m_name; }
    bool maybePointedTo() ଅଚଳ override VL_MT_SAFE { ଫେରନ୍ତୁ true; }
    bool hasDType() ଅଚଳ override VL_MT_SAFE { ଫେରନ୍ତୁ true; }
    ଶୂନ୍ୟ name(ଅଚଳ string& flag) override { m_name = flag; }
    VVarType varType() ଅଚଳ { ଫେରନ୍ତୁ m_varType; }  // * = Type of variable
    VFwdType fwdType() ଅଚଳ { ଫେରନ୍ତୁ m_fwdType; }
    bool isParam() ଅଚଳ { ଫେରନ୍ତୁ true; }
    bool isGParam() ଅଚଳ { ଫେରନ୍ତୁ (varType() == VVarType::GPARAM); }
    bool isCompound() ଅଚଳ override {
        v3fatalSrc("call isCompound on subdata type, not reference");
        ଫେରନ୍ତୁ false;
    }
};
class AstParseTypeDType final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_DType {
    // Parents: VAR
    // During parsing, this indicates the type of a parameter is a "parameter type"
    // e.g. the data type is a container of any data type
    ଅଚଳ VFwdType m_fwdType;  // Forward type for lint check
ସର୍ବସାଧାରଣ:
    explicit AstParseTypeDType(File_ଧାଡ଼ି* fl, VFwdType fwdType = VFwdType::NONE)
        : ASTGEN_SUPER_ParseTypeDType(fl)
        , m_fwdType{fwdType} {}
    ASTGEN_MEMBERS_AstParseTypeDType;
    Ast_ଗଣ୍ଠି_DType* dtypep() ଅଚଳ VL_MT_STABLE { ଫେରନ୍ତୁ nullptr; }
    // METHODS
    ଶୂନ୍ୟ dump(std::ostream& str = std::cout) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str = std::cout) ଅଚଳ override;
    bool similar_DType_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି_DType* samep) ଅଚଳ override { ଫେରନ୍ତୁ this == samep; }
    AstBasicDType* basicp() ଅଚଳ override VL_MT_STABLE { ଫେରନ୍ତୁ nullptr; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା widthAlignBytes() ଅଚଳ override { ଫେରନ୍ତୁ 0; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା widthTotalBytes() ଅଚଳ override { ଫେରନ୍ତୁ 0; }
    bool isCompound() ଅଚଳ override {
        v3fatalSrc("call isCompound on subdata type, not reference");
        ଫେରନ୍ତୁ false;
    }
    VFwdType fwdType() ଅଚଳ { ଫେରନ୍ତୁ m_fwdType; }
};
class AstQueueDType final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_DType {
    // Queue array data type, ie "[ $ ]"
    // @astgen op1 := childDTypep : Optional[AstNodeDType] // moved to refDTypep() in V3Width
    // @astgen op2 := boundp : Optional[AstNodeExpr]
    //
    // @astgen ptr := m_refDTypep : Optional[AstNodeDType]  // Elements of this type (post-width)
ସର୍ବସାଧାରଣ:
    AstQueueDType(File_ଧାଡ଼ି* fl, VFlagChildDType, Ast_ଗଣ୍ଠି_DType* dtp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* boundp)
        : ASTGEN_SUPER_QueueDType(fl) {
        childDTypep(dtp);
        this->boundp(boundp);
        refDTypep(nullptr);
        dtypep(nullptr);  // V3Width will resolve
    }
    AstQueueDType(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_DType* dtp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* boundp)
        : ASTGEN_SUPER_QueueDType(fl) {
        this->boundp(boundp);
        refDTypep(dtp);
        dtypep(dtp);
    }
    ASTGEN_MEMBERS_AstQueueDType;
    ଅଚଳ ଅକ୍ଷର* broken() ଅଚଳ override {
        BROKEN_RTN(!((m_refDTypep && !childDTypep()) || (!m_refDTypep && childDTypep())));
        ଫେରନ୍ତୁ nullptr;
    }
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* samep) ଅଚଳ override {
        ଅଚଳ AstQueueDType* ଅଚଳ asamep = VN_DBG_AS(samep, QueueDType);
        ଯଦି (!asamep->subDTypep()) ଫେରନ୍ତୁ false;
        ଫେରନ୍ତୁ (subDTypep() == asamep->subDTypep());
    }
    bool similar_DType_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି_DType* samep) ଅଚଳ override {
        ଅଚଳ AstQueueDType* ଅଚଳ asamep = VN_DBG_AS(samep, QueueDType);
        ଫେରନ୍ତୁ asamep->subDTypep() && subDTypep()->similarDType(asamep->subDTypep());
    }
    ଶୂନ୍ୟ dumpSmall(std::ostream& str) ଅଚଳ override;
    string prettyDTypeName(bool full) ଅଚଳ override;
    Ast_ଗଣ୍ଠି_DType* getChildDTypep() ଅଚଳ override { ଫେରନ୍ତୁ childDTypep(); }
    Ast_ଗଣ୍ଠି_DType* subDTypep() ଅଚଳ override VL_MT_STABLE {
        ଫେରନ୍ତୁ m_refDTypep ? m_refDTypep : childDTypep();
    }
    bool isAggregateType() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    ଶୂନ୍ୟ refDTypep(Ast_ଗଣ୍ଠି_DType* nodep) { m_refDTypep = nodep; }
    ଧାଡ଼ିଭିତରେ ପୂର୍ଣ୍ଣସଂଖ୍ୟା boundConst() ଅଚଳ VL_MT_STABLE;
    Ast_ଗଣ୍ଠି_DType* virtRefDTypep() ଅଚଳ override { ଫେରନ୍ତୁ m_refDTypep; }
    ଶୂନ୍ୟ virtRefDTypep(Ast_ଗଣ୍ଠି_DType* nodep) override { refDTypep(nodep); }
    // METHODS
    AstBasicDType* basicp() ଅଚଳ override VL_MT_STABLE { ଫେରନ୍ତୁ nullptr; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା widthAlignBytes() ଅଚଳ override { ଫେରନ୍ତୁ subDTypep()->widthAlignBytes(); }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା widthTotalBytes() ଅଚଳ override { ଫେରନ୍ତୁ subDTypep()->widthTotalBytes(); }
    bool isCompound() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool isDynamicallySized() ଅଚଳ override { ଫେରନ୍ତୁ true; }
};
class AstRefDType final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_DType {
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
    ଅଚଳ VIfaceCaptureTag* m_captureTagp = nullptr;

ସର୍ବସାଧାରଣ:
    AstRefDType(File_ଧାଡ଼ି* fl, ଅଚଳ string& name)
        : ASTGEN_SUPER_RefDType(fl)
        , m_name{name} {}
    AstRefDType(File_ଧାଡ଼ି* fl, ଅଚଳ string& name, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* classOrPackagep, AstPin* paramsp)
        : ASTGEN_SUPER_RefDType(fl)
        , m_name{name} {
        classOrPackageOpp(classOrPackagep);
        addParamsp(paramsp);
    }
    class Flag_Type_Of_ପଦସମଷ୍ଟି {};  // type(expr) for parser only
    AstRefDType(File_ଧାଡ଼ି* fl, Flag_Type_Of_ପଦସମଷ୍ଟି, Ast_ଗଣ୍ଠି* typeofp)
        : ASTGEN_SUPER_RefDType(fl) {
        this->typeofp(typeofp);
        ଯଦି (Ast_ଗଣ୍ଠି_DType* ଅଚଳ dtp = VN_CAST(typeofp, ଗଣ୍ଠି_DType)) refDTypep(dtp);
    }
    ASTGEN_MEMBERS_AstRefDType;
    // METHODS
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* samep) ଅଚଳ override {
        ଅଚଳ AstRefDType* ଅଚଳ asamep = VN_DBG_AS(samep, RefDType);
        ଫେରନ୍ତୁ (m_typedefp == asamep->m_typedefp && m_refDTypep == asamep->m_refDTypep
                && m_name == asamep->m_name && m_classOrPackagep == asamep->m_classOrPackagep);
    }
    bool similar_DType_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି_DType* samep) ଅଚଳ override {
        ଫେରନ୍ତୁ subDTypep()->similarDType(samep->subDTypep());
    }
    ଅଚଳ ଅକ୍ଷର* broken() ଅଚଳ override;
    ଶୂନ୍ୟ dump(std::ostream& str = std::cout) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str = std::cout) ଅଚଳ override;
    ଶୂନ୍ୟ dumpSmall(std::ostream& str) ଅଚଳ override;
    string name() ଅଚଳ override VL_MT_STABLE { ଫେରନ୍ତୁ m_name; }
    string prettyDTypeName(bool full) ଅଚଳ override {
        ଫେରନ୍ତୁ subDTypep() ? subDTypep()->prettyDTypeName(full) : prettyName();
    }
    AstBasicDType* basicp() ଅଚଳ override VL_MT_STABLE {
        ଫେରନ୍ତୁ subDTypep() ? subDTypep()->basicp() : nullptr;
    }
    Ast_ଗଣ୍ଠି_DType* subDTypep() ଅଚଳ override VL_MT_STABLE;
    Ast_ଗଣ୍ଠି_DType* getChildDTypep() ଅଚଳ override { ଫେରନ୍ତୁ VN_CAST(typeofp(), ଗଣ୍ଠି_DType); }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା widthAlignBytes() ଅଚଳ override { ଫେରନ୍ତୁ dtypeSkipRefp()->widthAlignBytes(); }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା widthTotalBytes() ଅଚଳ override { ଫେରନ୍ତୁ dtypeSkipRefp()->widthTotalBytes(); }
    ଶୂନ୍ୟ name(ଅଚଳ string& flag) override { m_name = flag; }
    Ast_ଗଣ୍ଠି_DType* dtypeSkipRefp() ଅଚଳ { ଫେରନ୍ତୁ subDTypep()->skipRefp(); }
    AstTypedef* typedefp() ଅଚଳ VL_MT_SAFE { ଫେରନ୍ତୁ m_typedefp; }
    ଶୂନ୍ୟ typedefp(AstTypedef* nodep) { m_typedefp = nodep; }
    Ast_ଗଣ୍ଠି_DType* refDTypep() ଅଚଳ VL_MT_SAFE { ଫେରନ୍ତୁ m_refDTypep; }
    ଶୂନ୍ୟ refDTypep(Ast_ଗଣ୍ଠି_DType* nodep) { m_refDTypep = nodep; }
    Ast_ଗଣ୍ଠି_DType* virtRefDTypep() ଅଚଳ override { ଫେରନ୍ତୁ refDTypep(); }
    ଶୂନ୍ୟ virtRefDTypep(Ast_ଗଣ୍ଠି_DType* nodep) override { refDTypep(nodep); }
    Ast_ଗଣ୍ଠି_Module* classOrPackagep() ଅଚଳ { ଫେରନ୍ତୁ m_classOrPackagep; }
    ଶୂନ୍ୟ classOrPackagep(Ast_ଗଣ୍ଠି_Module* nodep) { m_classOrPackagep = nodep; }
    ଅଚଳ VIfaceCaptureTag* captureTagp() ଅଚଳ { ଫେରନ୍ତୁ m_captureTagp; }
    ଶୂନ୍ୟ captureTagp(ଅଚଳ VIfaceCaptureTag* tagp) { m_captureTagp = tagp; }
    bool isCompound() ଅଚଳ override {
        v3fatalSrc("call isCompound on subdata type, not reference");
        ଫେରନ୍ତୁ false;
    }
};
class AstRequireDType final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_DType {
    // @astgen op1 := lhsp : Optional[AstNode<AstNodeExpr|AstNodeDType>]
    //
    // Require a generic node type (typically AstParseRef become a type.
ସର୍ବସାଧାରଣ:
    AstRequireDType(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି* lhsp)
        : ASTGEN_SUPER_RequireDType(fl) {
        this->lhsp(lhsp);
    }
    ASTGEN_MEMBERS_AstRequireDType;
    // METHODS
    bool similar_DType_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି_DType* samep) ଅଚଳ override {
        ଅଚଳ AstRequireDType* ଅଚଳ asamep = VN_DBG_AS(samep, RequireDType);
        ଫେରନ୍ତୁ subDTypep()->similarDType(asamep->subDTypep());
    }
    AstBasicDType* basicp() ଅଚଳ override VL_MT_STABLE { ଫେରନ୍ତୁ nullptr; }
    Ast_ଗଣ୍ଠି_DType* subDTypep() ଅଚଳ override VL_MT_STABLE {
        // Used for recursive definition checking
        ଯଦି (Ast_ଗଣ୍ଠି_DType* ଅଚଳ dtp = VN_CAST(lhsp(), ଗଣ୍ଠି_DType)) ଫେରନ୍ତୁ dtp;

        ଫେରନ୍ତୁ nullptr;
    }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା widthAlignBytes() ଅଚଳ override { V3ERROR_NA_RETURN(1); }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା widthTotalBytes() ଅଚଳ override { V3ERROR_NA_RETURN(1); }
    bool isCompound() ଅଚଳ override { V3ERROR_NA_RETURN(false); }
};
class AstSampleQueueDType final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_DType {
    // @astgen op1 := childDTypep : Optional[AstNodeDType] // moved to refDTypep() in V3Width
    //
    // @astgen ptr := m_refDTypep : Optional[AstNodeDType]  // Elements of this type (post-width)
ସର୍ବସାଧାରଣ:
    AstSampleQueueDType(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_DType* dtp)
        : ASTGEN_SUPER_SampleQueueDType(fl) {
        refDTypep(dtp);
        dtypep(dtp);
    }
    ASTGEN_MEMBERS_AstSampleQueueDType;
    ଅଚଳ ଅକ୍ଷର* broken() ଅଚଳ override {
        BROKEN_RTN(!((m_refDTypep && !childDTypep()) || (!m_refDTypep && childDTypep())));
        ଫେରନ୍ତୁ nullptr;
    }
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* samep) ଅଚଳ override {
        ଅଚଳ AstSampleQueueDType* ଅଚଳ asamep = VN_DBG_AS(samep, SampleQueueDType);
        ଯଦି (!asamep->subDTypep()) ଫେରନ୍ତୁ false;
        ଫେରନ୍ତୁ (subDTypep() == asamep->subDTypep());
    }
    bool similar_DType_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି_DType* samep) ଅଚଳ override {
        ଅଚଳ AstSampleQueueDType* ଅଚଳ asamep = VN_DBG_AS(samep, SampleQueueDType);
        ଫେରନ୍ତୁ asamep->subDTypep() && subDTypep()->similarDType(asamep->subDTypep());
    }
    ଶୂନ୍ୟ dumpSmall(std::ostream& str) ଅଚଳ override;
    Ast_ଗଣ୍ଠି_DType* getChildDTypep() ଅଚଳ override { ଫେରନ୍ତୁ childDTypep(); }
    // op1 = Range of variable
    Ast_ଗଣ୍ଠି_DType* subDTypep() ଅଚଳ override VL_MT_STABLE {
        ଫେରନ୍ତୁ m_refDTypep ? m_refDTypep : childDTypep();
    }
    ଶୂନ୍ୟ refDTypep(Ast_ଗଣ୍ଠି_DType* nodep) { m_refDTypep = nodep; }
    Ast_ଗଣ୍ଠି_DType* virtRefDTypep() ଅଚଳ override { ଫେରନ୍ତୁ m_refDTypep; }
    ଶୂନ୍ୟ virtRefDTypep(Ast_ଗଣ୍ଠି_DType* nodep) override { refDTypep(nodep); }
    // METHODS
    AstBasicDType* basicp() ଅଚଳ override VL_MT_STABLE { ଫେରନ୍ତୁ subDTypep()->basicp(); }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା widthAlignBytes() ଅଚଳ override { ଫେରନ୍ତୁ ଆକାର(std::map<std::string, std::string>); }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା widthTotalBytes() ଅଚଳ override { ଫେରନ୍ତୁ ଆକାର(std::map<std::string, std::string>); }
    bool isCompound() ଅଚଳ override { ଫେରନ୍ତୁ true; }
};
class AstStreamDType final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_DType {
    // Stream data type, used only as data type of stream operations
    // Should behave like AstPackArrayDType, but it doesn't have a size
ସର୍ବସାଧାରଣ:
    explicit AstStreamDType(File_ଧାଡ଼ି* fl)
        : ASTGEN_SUPER_StreamDType(fl) {
        dtypep(this);
    }
    ASTGEN_MEMBERS_AstStreamDType;
    ଶୂନ୍ୟ dumpSmall(std::ostream& str) ଅଚଳ override;
    bool hasDType() ଅଚଳ override VL_MT_SAFE { ଫେରନ୍ତୁ true; }
    bool maybePointedTo() ଅଚଳ override VL_MT_SAFE { ଫେରନ୍ତୁ true; }
    bool undead() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    Ast_ଗଣ୍ଠି_DType* subDTypep() ଅଚଳ override VL_MT_STABLE { ଫେରନ୍ତୁ nullptr; }
    Ast_ଗଣ୍ଠି_DType* virtRefDTypep() ଅଚଳ override { ଫେରନ୍ତୁ nullptr; }
    ଶୂନ୍ୟ virtRefDTypep(Ast_ଗଣ୍ଠି_DType* nodep) override {}
    bool similar_DType_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି_DType* samep) ଅଚଳ override { ଫେରନ୍ତୁ this == samep; }
    AstBasicDType* basicp() ଅଚଳ override VL_MT_STABLE { ଫେରନ୍ତୁ nullptr; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା widthAlignBytes() ଅଚଳ override { ଫେରନ୍ତୁ 1; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା widthTotalBytes() ଅଚଳ override { ଫେରନ୍ତୁ 1; }
    bool isCompound() ଅଚଳ override { ଫେରନ୍ତୁ false; }
};
class AstUnsizedArrayDType final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_DType {
    // Unsized/open-range Array data type, ie "some_dtype var_name []"
    // @astgen op1 := childDTypep : Optional[AstNodeDType] // moved to refDTypep() in V3Width
    //
    // @astgen ptr := m_refDTypep : Optional[AstNodeDType]  // Elements of this type (post-width)
ସର୍ବସାଧାରଣ:
    AstUnsizedArrayDType(File_ଧାଡ଼ି* fl, VFlagChildDType, Ast_ଗଣ୍ଠି_DType* dtp)
        : ASTGEN_SUPER_UnsizedArrayDType(fl) {
        childDTypep(dtp);  // Only for parser
        refDTypep(nullptr);
        dtypep(nullptr);  // V3Width will resolve
    }
    AstUnsizedArrayDType(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_DType* dtp)
        : ASTGEN_SUPER_UnsizedArrayDType(fl) {
        refDTypep(dtp);
        dtypep(nullptr);  // V3Width will resolve
    }
    ASTGEN_MEMBERS_AstUnsizedArrayDType;
    ଅଚଳ ଅକ୍ଷର* broken() ଅଚଳ override {
        BROKEN_RTN(!((m_refDTypep && !childDTypep()) || (!m_refDTypep && childDTypep())));
        ଫେରନ୍ତୁ nullptr;
    }
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* samep) ଅଚଳ override;
    bool similar_DType_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି_DType* samep) ଅଚଳ override;
    ଶୂନ୍ୟ dumpSmall(std::ostream& str) ଅଚଳ override;
    Ast_ଗଣ୍ଠି_DType* getChildDTypep() ଅଚଳ override { ଫେରନ୍ତୁ childDTypep(); }
    Ast_ଗଣ୍ଠି_DType* subDTypep() ଅଚଳ override VL_MT_STABLE {
        ଫେରନ୍ତୁ m_refDTypep ? m_refDTypep : childDTypep();
    }
    ଶୂନ୍ୟ refDTypep(Ast_ଗଣ୍ଠି_DType* nodep) { m_refDTypep = nodep; }
    Ast_ଗଣ୍ଠି_DType* virtRefDTypep() ଅଚଳ override { ଫେରନ୍ତୁ m_refDTypep; }
    ଶୂନ୍ୟ virtRefDTypep(Ast_ଗଣ୍ଠି_DType* nodep) override { refDTypep(nodep); }
    // METHODS
    AstBasicDType* basicp() ଅଚଳ override VL_MT_STABLE { ଫେରନ୍ତୁ subDTypep()->basicp(); }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା widthAlignBytes() ଅଚଳ override { ଫେରନ୍ତୁ subDTypep()->widthAlignBytes(); }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା widthTotalBytes() ଅଚଳ override { ଫେରନ୍ତୁ subDTypep()->widthTotalBytes(); }
    bool isCompound() ଅଚଳ override { ଫେରନ୍ତୁ true; }
};
class AstVoidDType final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_DType {
    // For e.g. a function returning void
ସର୍ବସାଧାରଣ:
    explicit AstVoidDType(File_ଧାଡ଼ି* fl)
        : ASTGEN_SUPER_VoidDType(fl) {
        dtypep(this);
    }
    ASTGEN_MEMBERS_AstVoidDType;
    ଶୂନ୍ୟ dumpSmall(std::ostream& str) ଅଚଳ override;
    bool hasDType() ଅଚଳ override VL_MT_SAFE { ଫେରନ୍ତୁ true; }
    bool maybePointedTo() ଅଚଳ override VL_MT_SAFE { ଫେରନ୍ତୁ true; }
    bool undead() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    Ast_ଗଣ୍ଠି_DType* subDTypep() ଅଚଳ override VL_MT_STABLE { ଫେରନ୍ତୁ nullptr; }
    Ast_ଗଣ୍ଠି_DType* virtRefDTypep() ଅଚଳ override { ଫେରନ୍ତୁ nullptr; }
    ଶୂନ୍ୟ virtRefDTypep(Ast_ଗଣ୍ଠି_DType* nodep) override {}
    bool similar_DType_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି_DType* samep) ଅଚଳ override { ଫେରନ୍ତୁ true; }
    AstBasicDType* basicp() ଅଚଳ override VL_MT_STABLE { ଫେରନ୍ତୁ nullptr; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା widthAlignBytes() ଅଚଳ override { ଫେରନ୍ତୁ 1; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା widthTotalBytes() ଅଚଳ override { ଫେରନ୍ତୁ 1; }
    bool isCompound() ଅଚଳ override { ଫେରନ୍ତୁ false; }
};
class AstWildcardArrayDType final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_DType {
    // Wildcard index type associative array data type, ie "some_dtype var_name [*]"
    // @astgen op1 := childDTypep : Optional[AstNodeDType] // moved to refDTypep() in V3Width
    //
    // @astgen ptr := m_refDTypep : Optional[AstNodeDType]  // Elements of this type (post-width)
ସର୍ବସାଧାରଣ:
    AstWildcardArrayDType(File_ଧାଡ଼ି* fl, VFlagChildDType, Ast_ଗଣ୍ଠି_DType* dtp)
        : ASTGEN_SUPER_WildcardArrayDType(fl) {
        childDTypep(dtp);  // Only for parser
        refDTypep(nullptr);
        dtypep(nullptr);  // V3Width will resolve
    }
    ASTGEN_MEMBERS_AstWildcardArrayDType;
    ଅଚଳ ଅକ୍ଷର* broken() ଅଚଳ override {
        BROKEN_RTN(!((m_refDTypep && !childDTypep()) || (!m_refDTypep && childDTypep())));
        ଫେରନ୍ତୁ nullptr;
    }
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* samep) ଅଚଳ override;
    bool similar_DType_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି_DType* samep) ଅଚଳ override;
    ଶୂନ୍ୟ dumpSmall(std::ostream& str) ଅଚଳ override;
    string prettyDTypeName(bool full) ଅଚଳ override;
    Ast_ଗଣ୍ଠି_DType* getChildDTypep() ଅଚଳ override { ଫେରନ୍ତୁ childDTypep(); }
    Ast_ଗଣ୍ଠି_DType* subDTypep() ଅଚଳ override VL_MT_STABLE {
        ଫେରନ୍ତୁ m_refDTypep ? m_refDTypep : childDTypep();
    }
    ଶୂନ୍ୟ refDTypep(Ast_ଗଣ୍ଠି_DType* nodep) { m_refDTypep = nodep; }
    Ast_ଗଣ୍ଠି_DType* virtRefDTypep() ଅଚଳ override { ଫେରନ୍ତୁ m_refDTypep; }
    ଶୂନ୍ୟ virtRefDTypep(Ast_ଗଣ୍ଠି_DType* nodep) override { refDTypep(nodep); }
    // METHODS
    AstBasicDType* basicp() ଅଚଳ override VL_MT_STABLE { ଫେରନ୍ତୁ subDTypep()->basicp(); }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା widthAlignBytes() ଅଚଳ override { ଫେରନ୍ତୁ ଆକାର(std::map<std::string, std::string>); }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା widthTotalBytes() ଅଚଳ override { ଫେରନ୍ତୁ ଆକାର(std::map<std::string, std::string>); }
    bool isCompound() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool isDynamicallySized() ଅଚଳ override { ଫେରନ୍ତୁ true; }
};

// === AstNodeArrayDType ===
class AstPackArrayDType final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Array_DType {
    // Packed array data type, ie "some_dtype [2:0] var_name"
ସର୍ବସାଧାରଣ:
    ଧାଡ଼ିଭିତରେ AstPackArrayDType(File_ଧାଡ଼ି* fl, VFlagChildDType, Ast_ଗଣ୍ଠି_DType* dtp, AstRange* rangep);
    ଧାଡ଼ିଭିତରେ AstPackArrayDType(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_DType* dtp, AstRange* rangep);
    ASTGEN_MEMBERS_AstPackArrayDType;
    string prettyDTypeName(bool full) ଅଚଳ override;
    bool isCompound() ଅଚଳ override { ଫେରନ୍ତୁ false; }
};
class AstUnpackArrayDType final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Array_DType {
    // Array data type, ie "some_dtype var_name [2:0]"
    bool m_isCompound = false;  // Non-POD subDType, or parent requires compound
ସର୍ବସାଧାରଣ:
    AstUnpackArrayDType(File_ଧାଡ଼ି* fl, VFlagChildDType, Ast_ଗଣ୍ଠି_DType* dtp, AstRange* rangep)
        : ASTGEN_SUPER_UnpackArrayDType(fl) {
        childDTypep(dtp);  // Only for parser
        this->rangep(rangep);
        refDTypep(nullptr);
        dtypep(nullptr);  // V3Width will resolve
        // For backward compatibility AstNodeArrayDType and others inherit
        // width and signing from the subDType/base type
        widthFromSub(subDTypep());
    }
    AstUnpackArrayDType(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_DType* dtp, AstRange* rangep)
        : ASTGEN_SUPER_UnpackArrayDType(fl) {
        this->rangep(rangep);
        refDTypep(dtp);
        dtypep(this);
        // For backward compatibility AstNodeArrayDType and others inherit
        // width and signing from the subDType/base type
        widthFromSub(subDTypep());
    }
    ASTGEN_MEMBERS_AstUnpackArrayDType;
    ଶୂନ୍ୟ dump(std::ostream& str) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str) ଅଚଳ override;
    string prettyDTypeName(bool full) ଅଚଳ override;
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* samep) ଅଚଳ override {
        ଅଚଳ AstUnpackArrayDType* ଅଚଳ sp = VN_DBG_AS(samep, UnpackArrayDType);
        ଫେରନ୍ତୁ m_isCompound == sp->m_isCompound;
    }
    bool similar_DType_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି_DType* samep) ଅଚଳ override;
    bool isAggregateType() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    // True iff 'otherp' is an array of the same shape, with
    // at least one dimension reversed ascending/descending.
    bool isReverseOf(ଅଚଳ Ast_ଗଣ୍ଠି_DType* otherp) ଅଚଳ;
    // Outer dimension comes first. The first element is this node.
    std::vector<AstUnpackArrayDType*> unpackDimensions();
    ଶୂନ୍ୟ isCompound(bool flag) { m_isCompound = flag; }
    bool isCompound() ଅଚଳ override VL_MT_SAFE { ଫେରନ୍ତୁ m_isCompound; }
    bool isIntegralOrPacked() ଅଚଳ override { ଫେରନ୍ତୁ false; }
};

// === AstNodeUOrStructDType ===
class AstStructDType final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_UOr_Struct_DType {
ସର୍ବସାଧାରଣ:
    // VSigning below is mispurposed to indicate if packed or not
    AstStructDType(File_ଧାଡ଼ି* fl, VSigning numericUnpack)
        : ASTGEN_SUPER_StructDType(fl, numericUnpack) {}
    ASTGEN_MEMBERS_AstStructDType;
    string verilogKwd() ଅଚଳ override { ଫେରନ୍ତୁ "struct"; }
};
class AstUnionDType final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_UOr_Struct_DType {
    bool m_isSoft;  // Is a "union soft"
    bool m_isTagged;  // Is a "union tagged"

ସର୍ବସାଧାରଣ:
    // VSigning below is mispurposed to indicate if packed or not
    // isSoft implies packed
    AstUnionDType(File_ଧାଡ଼ି* fl, bool isSoft, bool isTagged, VSigning numericUnpack)
        : ASTGEN_SUPER_UnionDType(fl, numericUnpack)
        , m_isSoft{isSoft}
        , m_isTagged{isTagged} {
        packed(packed() | m_isSoft);
    }
    ASTGEN_MEMBERS_AstUnionDType;
    string verilogKwd() ଅଚଳ override { ଫେରନ୍ତୁ "union"; }
    bool isSoft() ଅଚଳ { ଫେରନ୍ତୁ m_isSoft; }
    bool isTagged() ଅଚଳ { ଫେରନ୍ତୁ m_isTagged; }
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* samep) ଅଚଳ override;
    ଶୂନ୍ୟ dump(std::ostream& str) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str) ଅଚଳ override;
};

#endif  // Guard
