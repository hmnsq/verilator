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
#অন্তর্ভাৱ "V3Ast.h"  // This helps code analysis tools pick up symbols in V3Ast.h
#define VL_NOT_FINAL  // This #define fixes broken code folding in the CLion IDE
#endif

// === Abstract base node types (AstNode*) =====================================

class Ast_গ্ৰন্থি_DType VL_NOT_FINAL : সার্ৱজনিক Ast_গ্ৰন্থি {
    // Ideally width() would migrate to BasicDType as that's where it makes sense,
    // but it's currently so prevalent in the code we leave it here.
    // Note the below members are included in AstTypeTable::Key lookups
    // dist-ast-dump-suppress  // Part of dumpSmall
    পূর্ণাঙ্ক m_width = 0;  // (also in AstTypeTable::Key) Bit width of operation
    // (also in AstTypeTable::Key) If unsized, bitwidth of minimum implementation
    // dist-ast-dump-suppress  // Part of dumpSmall
    পূর্ণাঙ্ক m_widthMin = 0;
    // dist-ast-dump-suppress  // Part of dumpSmall
    VSigning m_numeric;  // (also in AstTypeTable::Key) Node is signed
    // dist-ast-dump-suppress  // Part of dumpSmall
    bool m_generic = false;  // Simple globally referenced type, don't garbage collect
    // Unique number assigned to each dtype during creation for IEEE matching
    স্থির পূর্ণাঙ্ক s_unique_অঙ্ক;

রক্ষিত:
    // CONSTRUCTORS
    Ast_গ্ৰন্থি_DType(VNType t, File_সাৰি* fl)
        : Ast_গ্ৰন্থি{t, fl} {}

গুপ্ত:
    // METHODS
    ধ্রুৱ Ast_গ্ৰন্থি_DType* skipRefIterp(bool skipConst, bool skipEnum,
                                     bool assertOn = true) ধ্রুৱ VL_MT_STABLE;

রক্ষিত:
    // METHODS
    আভাস bool similar_DType_গ্ৰন্থি(ধ্রুৱ Ast_গ্ৰন্থি_DType* samep) ধ্রুৱ = 0;

সার্ৱজনিক:
    ASTGEN_MEMBERS_Ast_গ্ৰন্থি_DType;
    // ACCESSORS
    শূন্য dump(std::ostream& str) ধ্রুৱ override;
    শূন্য dumpJson(std::ostream& str) ধ্রুৱ override;
    আভাস শূন্য dumpSmall(std::ostream& str) ধ্রুৱ VL_MT_STABLE;
    bool hasDType() ধ্রুৱ override VL_MT_SAFE { ঘূৰাই true; }
    /// Require VlUnpacked, instead of [] for POD elements.
    /// A non-POD object is always compound, but some POD elements
    /// are compound when methods calls operate on object, or when
    /// under another compound-requiring object e.g. class
    আভাস bool isCompound() ধ্রুৱ = 0;
    // Integral or packed, allowed inside an unpacked union/struct
    আভাস bool isIntegralOrPacked() ধ্রুৱ { ঘূৰাই !isCompound(); }
    // (Slow) recurse down to find basic data type
    আভাস AstBasicDType* basicp() ধ্রুৱ VL_MT_STABLE = 0;
    // (Slow) Recurse over MemberDType|ParamTypeDType|RefDType|ConstDType|EnumDType to other type
    ধ্রুৱ Ast_গ্ৰন্থি_DType* skipRefp() ধ্রুৱ VL_MT_STABLE { ঘূৰাই skipRefIterp(true, true); }
    Ast_গ্ৰন্থি_DType* skipRefp() VL_MT_STABLE {
        ঘূৰাই const_cast<Ast_গ্ৰন্থি_DType*>(
            static_cast<ধ্রুৱ Ast_গ্ৰন্থি_DType*>(this)->skipRefIterp(true, true));
    }
    // (Slow) Recurse over MemberDType|ParamTypeDType|RefDType|ConstDType|EnumDType to other type,
    // Returns null if not resolved
    ধ্রুৱ Ast_গ্ৰন্থি_DType* skipRefOrNullp() ধ্রুৱ VL_MT_STABLE {
        ঘূৰাই skipRefIterp(true, true, false);
    }
    Ast_গ্ৰন্থি_DType* skipRefOrNullp() VL_MT_STABLE {
        ঘূৰাই const_cast<Ast_গ্ৰন্থি_DType*>(
            static_cast<ধ্রুৱ Ast_গ্ৰন্থি_DType*>(this)->skipRefIterp(true, true, false));
    }
    // (Slow) Recurse over MemberDType|ParamTypeDType|RefDType|EnumDType to ConstDType
    ধ্রুৱ Ast_গ্ৰন্থি_DType* skipRefToConstp() ধ্রুৱ { ঘূৰাই skipRefIterp(false, true); }
    Ast_গ্ৰন্থি_DType* skipRefToConstp() {
        ঘূৰাই const_cast<Ast_গ্ৰন্থি_DType*>(
            static_cast<ধ্রুৱ Ast_গ্ৰন্থি_DType*>(this)->skipRefIterp(false, true));
    }
    // (Slow) Recurse over MemberDType|ParamTypeDType|RefDType|ConstDType to EnumDType
    ধ্রুৱ Ast_গ্ৰন্থি_DType* skipRefToEnump() ধ্রুৱ { ঘূৰাই skipRefIterp(true, false); }
    Ast_গ্ৰন্থি_DType* skipRefToEnump() {
        ঘূৰাই const_cast<Ast_গ্ৰন্থি_DType*>(
            static_cast<ধ্রুৱ Ast_গ্ৰন্থি_DType*>(this)->skipRefIterp(true, false));
    }
    // (Slow) Recurse over MemberDType|ParamTypeDType|RefDType|ConstDType to EnumDType,
    // Returns null if not resolved
    ধ্রুৱ Ast_গ্ৰন্থি_DType* skipRefToEnumOrNullp() ধ্রুৱ { ঘূৰাই skipRefIterp(true, false, false); }
    // (Slow) Recurse over MemberDType|ParamTypeDType|RefDType to other type
    ধ্রুৱ Ast_গ্ৰন্থি_DType* skipRefToNonRefp() ধ্রুৱ { ঘূৰাই skipRefIterp(false, false); }
    Ast_গ্ৰন্থি_DType* skipRefToNonRefp() {
        ঘূৰাই const_cast<Ast_গ্ৰন্থি_DType*>(
            static_cast<ধ্রুৱ Ast_গ্ৰন্থি_DType*>(this)->skipRefIterp(false, false));
    }
    // If array, returns element dtype, otherwise returns skipRef dtype
    // If skipRef is false, RefDTypes are not followed (safe before typedef linking)
    ধ্রুৱ Ast_গ্ৰন্থি_DType* elemDTypep(bool skipRef = true) ধ্রুৱ VL_MT_STABLE;
    Ast_গ্ৰন্থি_DType* elemDTypep(bool skipRef = true) VL_MT_STABLE {
        ঘূৰাই const_cast<Ast_গ্ৰন্থি_DType*>(
            static_cast<ধ্রুৱ Ast_গ্ৰন্থি_DType*>(this)->elemDTypep(skipRef));
    }
    // (Slow) recurses - Structure alignment 1,2,4 or 8 bytes (arrays affect this)
    আভাস পূর্ণাঙ্ক widthAlignBytes() ধ্রুৱ = 0;
    // (Slow) recurses - Width in bytes rounding up 1,2,4,8,12,...
    আভাস পূর্ণাঙ্ক widthTotalBytes() ধ্রুৱ = 0;
    bool maybePointedTo() ধ্রুৱ override VL_MT_SAFE { ঘূৰাই true; }
    // Iff has a non-null refDTypep(), as generic node function
    আভাস Ast_গ্ৰন্থি_DType* virtRefDTypep() ধ্রুৱ { ঘূৰাই nullptr; }
    // Iff has refDTypep(), set as generic node function
    আভাস শূন্য virtRefDTypep(Ast_গ্ৰন্থি_DType* nodep) {}
    // Iff has a non-null second dtypep, as generic node function
    আভাস Ast_গ্ৰন্থি_DType* virtRefDType2p() ধ্রুৱ { ঘূৰাই nullptr; }
    // Iff has second dtype, set as generic node function
    আভাস শূন্য virtRefDType2p(Ast_গ্ৰন্থি_DType* nodep) {}
    // Assignable equivalence.  Calls skipRefToNonRefp() during comparisons.
    bool similarDType(ধ্রুৱ Ast_গ্ৰন্থি_DType* samep) ধ্রুৱ;
    // Iff has a non-null subDTypep(), as generic node function
    আভাস Ast_গ্ৰন্থি_DType* subDTypep() ধ্রুৱ VL_MT_STABLE { ঘূৰাই nullptr; }
    আভাস Ast_গ্ৰন্থি_DType* subDType2p() ধ্রুৱ VL_MT_STABLE { ঘূৰাই nullptr; }
    আভাস bool isAggregateType() ধ্রুৱ { ঘূৰাই false; }
    // True for unpacked, dynamic, queue, and associative arrays (not packed arrays)
    bool isNonPackedArray() ধ্রুৱ;
    আভাস bool isFourstate() ধ্রুৱ;
    // Ideally an IEEE $typename
    আভাস string prettyDTypeName(bool) ধ্রুৱ { ঘূৰাই prettyTypeName(); }
    string prettyDTypeNameQ(bool full = false) ধ্রুৱ { ঘূৰাই "'" + prettyDTypeName(full) + "'"; }
    string stateDTypeName() ধ্রুৱ { ঘূৰাই this->isFourstate() ? "(4-state)" : "(2-state)"; }
    //
    // Changing the width may confuse the data type resolution, so must clear
    // TypeTable cache after use.
    শূন্য widthForce(পূর্ণাঙ্ক width, পূর্ণাঙ্ক widthMin) {
        m_width = width;
        m_widthMin = widthMin;
    }
    // For backward compatibility inherit width and signing from the subDType/base type
    শূন্য widthFromSub(ধ্রুৱ Ast_গ্ৰন্থি_DType* nodep) {
        m_width = nodep->m_width;
        m_widthMin = nodep->m_widthMin;
        m_numeric = nodep->m_numeric;
    }
    //
    পূর্ণাঙ্ক width() ধ্রুৱ VL_MT_STABLE { ঘূৰাই m_width; }
    শূন্য numeric(VSigning flag) { m_numeric = flag; }
    bool isSigned() ধ্রুৱ VL_MT_STABLE { ঘূৰাই m_numeric.isSigned(); }
    bool isNosign() ধ্রুৱ VL_MT_SAFE { ঘূৰাই m_numeric.isNosign(); }
    VSigning numeric() ধ্রুৱ VL_MT_STABLE { ঘূৰাই m_numeric; }
    পূর্ণাঙ্ক width_শব্দগুলি() ধ্রুৱ VL_MT_STABLE { ঘূৰাই VL_শব্দগুলি_I(width()); }
    পূর্ণাঙ্ক widthMin() ধ্রুৱ VL_MT_STABLE {  // If sized, the size,
                                         // if unsized the min digits to represent it
        ঘূৰাই m_widthMin ? m_widthMin : m_width;
    }
    পূর্ণাঙ্ক widthPow2() ধ্রুৱ;
    শূন্য widthMinFromWidth() { m_widthMin = m_width; }
    bool widthSized() ধ্রুৱ VL_MT_SAFE { ঘূৰাই !m_widthMin || m_widthMin == m_width; }
    bool generic() ধ্রুৱ VL_MT_SAFE { ঘূৰাই m_generic; }
    শূন্য generic(bool flag) { m_generic = flag; }
    std::pair<uint32_t, uint32_t> dimensions(bool includeBasic) ধ্রুৱ;
    uint32_t arrayUnpackedElements() ধ্রুৱ;  // 1, or total multiplication of all dimensions
    // Fixed aggregate streaming properties
    bool isStreamableFixedAggregate() ধ্রুৱ;
    // Whether is a string, or an unpacked array or structure containing one
    bool containsString() ধ্রুৱ;
    bool containsUnpackedStruct() ধ্রুৱ;
    পূর্ণাঙ্ক widthStream() ধ্রুৱ;
    string vlEnumType() ধ্রুৱ;  // Return VerilatedVarType: VLVT_UINT32, etc
    string vlEnumPackedAgg() ধ্রুৱ;  // Return VLVF_PACKED_AGG etc flags, or empty
    স্থির পূর্ণাঙ্ক unique_অঙ্ক_Inc() { ঘূৰাই ++s_unique_অঙ্ক; }
    ধ্রুৱ ৱর্ণ* charIQWN() ধ্রুৱ {
        ঘূৰাই (isString() ? "N" : isWide() ? "W" : isDouble() ? "D" : isQuad() ? "Q" : "I");
    }
    string cType(ধ্রুৱ string& name, bool forFunc, bool isRef, bool packed = false) ধ্রুৱ;
    // Represents a C++ LiteralType? (can be constexpr)
    bool isLiteralType() ধ্রুৱ VL_MT_STABLE;
    // C++ type (as emitted by cType) is trivially default constructible and destructible?
    bool isTriviallyConstructible() ধ্রুৱ VL_MT_STABLE;
    আভাস bool isDynamicallySized() ধ্রুৱ { ঘূৰাই false; }

গুপ্ত:
    class CTypeRecursed;
    CTypeRecursed cTypeRecurse(bool compound, bool packed) ধ্রুৱ;
};
class Ast_গ্ৰন্থি_Array_DType VL_NOT_FINAL : সার্ৱজনিক Ast_গ্ৰন্থি_DType {
    // Array data type, ie "some_dtype var_name [2:0]"
    // @astgen op1 := childDTypep : Optional[AstNodeDType] // moved to refDTypep() in V3Width
    // @astgen op2 := rangep : Optional[AstRange] // array bounds
    //
    // @astgen ptr := m_refDTypep : Optional[AstNodeDType]  // Elements of this type (post-width)
    Ast_গ্ৰন্থি* rangenp() ধ্রুৱ { ঘূৰাই reinterpret_cast<Ast_গ্ৰন্থি*>(rangep()); }

রক্ষিত:
    Ast_গ্ৰন্থি_Array_DType(VNType t, File_সাৰি* fl)
        : Ast_গ্ৰন্থি_DType{t, fl} {}

সার্ৱজনিক:
    ASTGEN_MEMBERS_Ast_গ্ৰন্থি_Array_DType;
    শূন্য dump(std::ostream& str) ধ্রুৱ override;
    শূন্য dumpJson(std::ostream& str) ধ্রুৱ override;
    শূন্য dumpSmall(std::ostream& str) ধ্রুৱ override;
    ধ্রুৱ ৱর্ণ* broken() ধ্রুৱ override {
        BROKEN_RTN(!((m_refDTypep && !childDTypep()) || (!m_refDTypep && childDTypep())));
        ঘূৰাই nullptr;
    }
    bool same_গ্ৰন্থি(ধ্রুৱ Ast_গ্ৰন্থি* samep) ধ্রুৱ override {
        ধ্রুৱ Ast_গ্ৰন্থি_Array_DType* ধ্রুৱ asamep = VN_DBG_AS(samep, গ্ৰন্থি_Array_DType);
        ঘূৰাই hi() == asamep->hi() && rangenp()->same_বৃক্ষ(asamep->rangenp())
               && subDTypep() == asamep->subDTypep();
    }  // HashedDT doesn't recurse, so need to check children
    bool similar_DType_গ্ৰন্থি(ধ্রুৱ Ast_গ্ৰন্থি_DType* samep) ধ্রুৱ override {
        ধ্রুৱ Ast_গ্ৰন্থি_Array_DType* ধ্রুৱ asamep = VN_DBG_AS(samep, গ্ৰন্থি_Array_DType);
        ঘূৰাই elementsConst() == asamep->elementsConst()
               && subDTypep()->similarDType(asamep->subDTypep());
    }
    Ast_গ্ৰন্থি_DType* getChildDTypep() ধ্রুৱ override { ঘূৰাই childDTypep(); }
    Ast_গ্ৰন্থি_DType* subDTypep() ধ্রুৱ override VL_MT_STABLE {
        ঘূৰাই m_refDTypep ? m_refDTypep : childDTypep();
    }
    শূন্য refDTypep(Ast_গ্ৰন্থি_DType* nodep) { m_refDTypep = nodep; }
    Ast_গ্ৰন্থি_DType* virtRefDTypep() ধ্রুৱ override { ঘূৰাই m_refDTypep; }
    শূন্য virtRefDTypep(Ast_গ্ৰন্থি_DType* nodep) override { refDTypep(nodep); }
    // METHODS
    AstBasicDType* basicp() ধ্রুৱ override VL_MT_STABLE {
        ঘূৰাই subDTypep()->basicp();
    }  // (Slow) recurse down to find basic data type
    পূর্ণাঙ্ক widthAlignBytes() ধ্রুৱ override { ঘূৰাই subDTypep()->widthAlignBytes(); }
    পূর্ণাঙ্ক widthTotalBytes() ধ্রুৱ override {
        ঘূৰাই elementsConst() * subDTypep()->widthTotalBytes();
    }
    অন্তঃস্থ পূর্ণাঙ্ক বাম() ধ্রুৱ VL_MT_STABLE;
    অন্তঃস্থ পূর্ণাঙ্ক ডান() ধ্রুৱ VL_MT_STABLE;
    অন্তঃস্থ পূর্ণাঙ্ক hi() ধ্রুৱ VL_MT_STABLE;
    অন্তঃস্থ পূর্ণাঙ্ক lo() ধ্রুৱ VL_MT_STABLE;
    অন্তঃস্থ পূর্ণাঙ্ক elementsConst() ধ্রুৱ VL_MT_STABLE;
    অন্তঃস্থ VNumRange declRange() ধ্রুৱ VL_MT_STABLE;
};
class Ast_গ্ৰন্থি_UOr_Struct_DType VL_NOT_FINAL : সার্ৱজনিক Ast_গ্ৰন্থি_DType {
    // A struct or union; common handling
    // @astgen op1 := membersp : List[AstMemberDType]
    //
    // @astgen ptr := m_classOrPackagep : Optional[AstNodeModule]  // Package emitted with
    string m_name;  // Name from upper typedef, if any
    string m_typedefName;  // Typedef's dtypeName() once moved off the typedef declaring it
    ধ্রুৱ পূর্ণাঙ্ক m_unique_অঙ্ক;  // Unique ID distinguishing this dtype instance, for hashing/naming
    bool m_packed;  // Packed struct/union, else unpacked
    bool m_isFourstate = false;  // V3Width computes; true if any member is 4-state
    bool m_constrainedRand = false;  // True if struct has constraint expression
    bool m_emitToString = false;  // Generate to_string() for this struct/union if set

রক্ষিত:
    Ast_গ্ৰন্থি_UOr_Struct_DType(VNType t, File_সাৰি* fl, VSigning numericUnpack)
        : Ast_গ্ৰন্থি_DType{t, fl}
        , m_unique_অঙ্ক{unique_অঙ্ক_Inc()} {
        // VSigning::NOSIGN overloaded to indicate not packed
        m_packed = (numericUnpack != VSigning::NOSIGN);
        numeric(VSigning::fromBool(numericUnpack.isSigned()));
    }
    Ast_গ্ৰন্থি_UOr_Struct_DType(ধ্রুৱ Ast_গ্ৰন্থি_UOr_Struct_DType& other)
        : Ast_গ্ৰন্থি_DType{other}
        , m_name{other.m_name}
        , m_typedefName{other.m_typedefName}
        , m_unique_অঙ্ক{unique_অঙ্ক_Inc()}
        , m_packed{other.m_packed}
        , m_isFourstate{other.m_isFourstate} {}

সার্ৱজনিক:
    ASTGEN_MEMBERS_Ast_গ্ৰন্থি_UOr_Struct_DType;
    পূর্ণাঙ্ক unique_অঙ্ক() ধ্রুৱ { ঘূৰাই m_unique_অঙ্ক; }
    শূন্য dump(std::ostream& str) ধ্রুৱ override;
    শূন্য dumpJson(std::ostream& str) ধ্রুৱ override;
    string prettyDTypeName(bool) ধ্রুৱ override;
    bool isCompound() ধ্রুৱ override { ঘূৰাই !packed(); }
    // For basicp() we reuse the size to indicate a "fake" basic type of same size
    AstBasicDType* basicp() ধ্রুৱ override VL_MT_STABLE {
        যদি (!m_packed) ঘূৰাই nullptr;
        ঘূৰাই (isFourstate()
                    ? VN_AS(findLogicRangeDType(VNumRange{width() - 1, 0}, width(), numeric()),
                            BasicDType)
                    : VN_AS(findBitRangeDType(VNumRange{width() - 1, 0}, width(), numeric()),
                            BasicDType));
    }
    // (Slow) recurses - Structure alignment 1,2,4 or 8 bytes (arrays affect this)
    পূর্ণাঙ্ক widthAlignBytes() ধ্রুৱ override;
    // (Slow) recurses - Width in bytes rounding up 1,2,4,8,12,...
    পূর্ণাঙ্ক widthTotalBytes() ধ্রুৱ override;
    bool similar_DType_গ্ৰন্থি(ধ্রুৱ Ast_গ্ৰন্থি_DType* samep) ধ্রুৱ override;
    string name() ধ্রুৱ override VL_MT_STABLE { ঘূৰাই m_name; }
    শূন্য name(ধ্রুৱ string& flag) override { m_name = flag; }
    // Keep the name the typedef declaring it gives, as it moves off it to the type table
    শূন্য typedefName(ধ্রুৱ string& name) { m_typedefName = name; }
    // Name of the C structure for DPI, empty if no typedef names it, so declared where used
    string dpiName() ধ্রুৱ;
    // C typedef of the unpacked structure for DPI (IEEE 1800-2023 H.7.3)
    string dpiTypedef() ধ্রুৱ;
    bool packed() ধ্রুৱ VL_MT_SAFE { ঘূৰাই m_packed; }
    শূন্য packed(bool flag) { m_packed = flag; }
    // packed() but as don't support unpacked, presently all structs
    স্থির bool packedUnsup() { ঘূৰাই true; }
    শূন্য isFourstate(bool flag) { m_isFourstate = flag; }
    bool isFourstate() ধ্রুৱ override VL_MT_SAFE { ঘূৰাই m_isFourstate; }
    স্থির পূর্ণাঙ্ক lo() VL_MT_STABLE { ঘূৰাই 0; }
    পূর্ণাঙ্ক hi() ধ্রুৱ VL_MT_STABLE {
        ঘূৰাই dtypep()->width() - 1;
    }  // Packed classes look like arrays
    VNumRange declRange() ধ্রুৱ VL_MT_STABLE { ঘূৰাই VNumRange{hi(), lo()}; }
    Ast_গ্ৰন্থি_Module* classOrPackagep() ধ্রুৱ { ঘূৰাই m_classOrPackagep; }
    শূন্য classOrPackagep(Ast_গ্ৰন্থি_Module* classpackagep) { m_classOrPackagep = classpackagep; }
    bool isConstrainedRand() ধ্রুৱ { ঘূৰাই m_constrainedRand; }
    শূন্য markConstrainedRand(bool flag) { m_constrainedRand = flag; }
    bool emitToString() ধ্রুৱ { ঘূৰাই m_emitToString; }
    শূন্য setEmitToString() { m_emitToString = true; }
    bool isAggregateType() ধ্রুৱ override { ঘূৰাই !packed(); }
};

// === Concrete node types =====================================================

// === AstNode ===
class AstEnumItem final : সার্ৱজনিক Ast_গ্ৰন্থি {
    // @astgen op1 := rangep : Optional[AstRange] // Range for name appending
    // @astgen op2 := valuep : Optional[AstNodeExpr]
    string m_name;  // Name of item

সার্ৱজনিক:
    // Parents: ENUM
    AstEnumItem(File_সাৰি* fl, ধ্রুৱ string& name, AstRange* rangep, Ast_গ্ৰন্থি_ৰাশিমালা* valuep)
        : ASTGEN_SUPER_EnumItem(fl)
        , m_name{name} {
        this->rangep(rangep);
        this->valuep(valuep);
    }
    ASTGEN_MEMBERS_AstEnumItem;
    string name() ধ্রুৱ override VL_MT_STABLE { ঘূৰাই m_name; }
    bool maybePointedTo() ধ্রুৱ override VL_MT_SAFE { ঘূৰাই true; }
    bool hasDType() ধ্রুৱ override VL_MT_SAFE { ঘূৰাই true; }
    শূন্য name(ধ্রুৱ string& flag) override { m_name = flag; }
};

// === AstNodeDType ===
class AstAssocArrayDType final : সার্ৱজনিক Ast_গ্ৰন্থি_DType {
    // Associative array data type, ie "[some_dtype]"
    // @astgen op1 := childDTypep : Optional[AstNodeDType] // moved to refDTypep() in V3Width
    // @astgen op2 := keyChildDTypep : Optional[AstNodeDType]
    //
    // @astgen ptr := m_refDTypep : Optional[AstNodeDType]  // Elements of this type (post-width)
    // @astgen ptr := m_keyDTypep : Optional[AstNodeDType]  // Keys of this type (post-width)
সার্ৱজনিক:
    AstAssocArrayDType(File_সাৰি* fl, VFlagChildDType, Ast_গ্ৰন্থি_DType* dtp, Ast_গ্ৰন্থি_DType* চাবি_Dtp)
        : ASTGEN_SUPER_AssocArrayDType(fl) {
        childDTypep(dtp);  // Only for parser
        চাবি_Child_DTypep(চাবি_Dtp);  // Only for parser
        refDTypep(nullptr);
        চাবি_DTypep(nullptr);
        dtypep(nullptr);  // V3Width will resolve
    }
    AstAssocArrayDType(File_সাৰি* fl, Ast_গ্ৰন্থি_DType* dtp, Ast_গ্ৰন্থি_DType* চাবি_Dtp)
        : ASTGEN_SUPER_AssocArrayDType(fl) {
        refDTypep(dtp);
        চাবি_DTypep(চাবি_Dtp);
        dtypep(dtp);
    }
    ASTGEN_MEMBERS_AstAssocArrayDType;
    ধ্রুৱ ৱর্ণ* broken() ধ্রুৱ override {
        BROKEN_RTN(!((m_refDTypep && !childDTypep()) || (!m_refDTypep && childDTypep())));
        BROKEN_RTN(!((m_চাবি_DTypep && !childDTypep()) || (!m_চাবি_DTypep && childDTypep())));
        ঘূৰাই nullptr;
    }
    bool same_গ্ৰন্থি(ধ্রুৱ Ast_গ্ৰন্থি* samep) ধ্রুৱ override {
        ধ্রুৱ AstAssocArrayDType* ধ্রুৱ asamep = VN_DBG_AS(samep, AssocArrayDType);
        যদি (!asamep->subDTypep()) ঘূৰাই false;
        যদি (!asamep->চাবি_DTypep()) ঘূৰাই false;
        ঘূৰাই (subDTypep() == asamep->subDTypep() && চাবি_DTypep() == asamep->চাবি_DTypep());
    }
    bool similar_DType_গ্ৰন্থি(ধ্রুৱ Ast_গ্ৰন্থি_DType* samep) ধ্রুৱ override {
        ধ্রুৱ AstAssocArrayDType* ধ্রুৱ asamep = VN_DBG_AS(samep, AssocArrayDType);
        ঘূৰাই asamep->subDTypep() && subDTypep()->similarDType(asamep->subDTypep())
               && asamep->চাবি_DTypep() && চাবি_DTypep()->similarDType(asamep->চাবি_DTypep());
    }
    string prettyDTypeName(bool full) ধ্রুৱ override;
    শূন্য dumpSmall(std::ostream& str) ধ্রুৱ override;
    Ast_গ্ৰন্থি_DType* getChildDTypep() ধ্রুৱ override { ঘূৰাই childDTypep(); }
    Ast_গ্ৰন্থি_DType* getChild2DTypep() ধ্রুৱ override { ঘূৰাই চাবি_Child_DTypep(); }
    Ast_গ্ৰন্থি_DType* subDTypep() ধ্রুৱ override VL_MT_STABLE {
        ঘূৰাই m_refDTypep ? m_refDTypep : childDTypep();
    }
    Ast_গ্ৰন্থি_DType* subDType2p() ধ্রুৱ override VL_MT_STABLE {
        ঘূৰাই m_চাবি_DTypep ? m_চাবি_DTypep : চাবি_Child_DTypep();
    }
    bool isAggregateType() ধ্রুৱ override { ঘূৰাই true; }
    শূন্য refDTypep(Ast_গ্ৰন্থি_DType* nodep) { m_refDTypep = nodep; }
    Ast_গ্ৰন্থি_DType* virtRefDTypep() ধ্রুৱ override { ঘূৰাই m_refDTypep; }
    শূন্য virtRefDTypep(Ast_গ্ৰন্থি_DType* nodep) override { refDTypep(nodep); }
    Ast_গ্ৰন্থি_DType* virtRefDType2p() ধ্রুৱ override { ঘূৰাই m_চাবি_DTypep; }
    শূন্য virtRefDType2p(Ast_গ্ৰন্থি_DType* nodep) override { চাবি_DTypep(nodep); }
    //
    Ast_গ্ৰন্থি_DType* চাবি_DTypep() ধ্রুৱ VL_MT_STABLE {
        ঘূৰাই m_চাবি_DTypep ? m_চাবি_DTypep : চাবি_Child_DTypep();
    }
    শূন্য চাবি_DTypep(Ast_গ্ৰন্থি_DType* nodep) { m_চাবি_DTypep = nodep; }
    // METHODS
    AstBasicDType* basicp() ধ্রুৱ override VL_MT_STABLE { ঘূৰাই nullptr; }
    পূর্ণাঙ্ক widthAlignBytes() ধ্রুৱ override { ঘূৰাই subDTypep()->widthAlignBytes(); }
    পূর্ণাঙ্ক widthTotalBytes() ধ্রুৱ override { ঘূৰাই subDTypep()->widthTotalBytes(); }
    bool isCompound() ধ্রুৱ override { ঘূৰাই true; }
    bool isDynamicallySized() ধ্রুৱ override { ঘূৰাই true; }
};
class AstBasicDType final : সার্ৱজনিক Ast_গ্ৰন্থি_DType {
    // Builtin atomic/vectored data type
    // @astgen op1 := rangep : Optional[AstRange] // Range of variable
    সংরচনা Members final {
        VBasicDTypeKwd m_keyword;  // (also in VBasicTypeKey) What keyword created basic type
        // dist-ast-dump-suppress  // Part of dumpSmall
        VNumRange m_nrange;  // (also in VBasicTypeKey) Numeric msb/lsb (if non-opaque keyword)
        bool operator==(ধ্রুৱ Members& rhs) ধ্রুৱ {
            ঘূৰাই rhs.m_keyword == m_keyword && rhs.m_nrange == m_nrange;
        }
    } m;
    // See also in AstNodeDType: m_width, m_widthMin, m_numeric(issigned)
সার্ৱজনিক:
    AstBasicDType(File_সাৰি* fl, VBasicDTypeKwd kwd, ধ্রুৱ VSigning& signst = VSigning::NOSIGN)
        : ASTGEN_SUPER_BasicDType(fl) {
        init(kwd, signst, 0, -1, nullptr);
    }
    AstBasicDType(File_সাৰি* fl, VFlagLogicPacked, পূর্ণাঙ্ক wantwidth)
        : ASTGEN_SUPER_BasicDType(fl) {
        init(VBasicDTypeKwd::LOGIC, VSigning::NOSIGN, wantwidth, -1, nullptr);
    }
    AstBasicDType(File_সাৰি* fl, VFlagLogic2StatePacked, পূর্ণাঙ্ক wantwidth)
        : ASTGEN_SUPER_BasicDType(fl) {
        init(VBasicDTypeKwd::LOGIC2STATE, VSigning::NOSIGN, wantwidth, -1, nullptr);
    }
    AstBasicDType(File_সাৰি* fl, VFlagBitPacked, পূর্ণাঙ্ক wantwidth)
        : ASTGEN_SUPER_BasicDType(fl) {
        init(VBasicDTypeKwd::BIT, VSigning::NOSIGN, wantwidth, -1, nullptr);
    }
    AstBasicDType(File_সাৰি* fl, VBasicDTypeKwd kwd, VSigning numer, পূর্ণাঙ্ক wantwidth, পূর্ণাঙ্ক widthmin)
        : ASTGEN_SUPER_BasicDType(fl) {
        init(kwd, numer, wantwidth, widthmin, nullptr);
    }
    AstBasicDType(File_সাৰি* fl, VBasicDTypeKwd kwd, VSigning numer, VNumRange range, পূর্ণাঙ্ক widthmin)
        : ASTGEN_SUPER_BasicDType(fl) {
        init(kwd, numer, range.elements(), widthmin, nullptr);
        m.m_nrange = range;  // as init() presumes lsb==0, but range.lsb() might not be
    }
    // See also addRange in verilog.y
গুপ্ত:
    শূন্য init(VBasicDTypeKwd kwd, VSigning numer, পূর্ণাঙ্ক wantwidth, পূর্ণাঙ্ক wantwidthmin,
              AstRange* rangep);

সার্ৱজনিক:
    ASTGEN_MEMBERS_AstBasicDType;
    শূন্য dump(std::ostream& str) ধ্রুৱ override;
    শূন্য dumpJson(std::ostream& str) ধ্রুৱ override;
    // width/widthMin/numeric compared elsewhere
    bool same_গ্ৰন্থি(ধ্রুৱ Ast_গ্ৰন্থি* samep) ধ্রুৱ override;
    bool similar_DType_গ্ৰন্থি(ধ্রুৱ Ast_গ্ৰন্থি_DType* samep) ধ্রুৱ override;
    string name() ধ্রুৱ override VL_MT_STABLE { ঘূৰাই m.m_keyword.typeName(); }
    string prettyDTypeName(bool full) ধ্রুৱ override;
    ধ্রুৱ ৱর্ণ* broken() ধ্রুৱ override {
        BROKEN_RTN(dtypep() != this);
        BROKEN_RTN(v3Global.widthMinUsage() == VWidthMinUsage::VERILOG_WIDTH
                   && widthMin() > width());
        ঘূৰাই nullptr;
    }
    শূন্য setSignedState(ধ্রুৱ VSigning& signst) {
        // Note NOSIGN does NOT change the state; this is required by the parser
        যদি (signst == VSigning::UNSIGNED) {
            numeric(signst);
        } অন্যথা যদি (signst == VSigning::SIGNED) {
            numeric(signst);
        }
    }
    // METHODS
    AstBasicDType* basicp() ধ্রুৱ override VL_MT_STABLE { ঘূৰাই (AstBasicDType*)this; }
    // (Slow) recurses - Structure alignment 1,2,4 or 8 bytes (arrays affect this)
    পূর্ণাঙ্ক widthAlignBytes() ধ্রুৱ override;
    // (Slow) recurses - Width in bytes rounding up 1,2,4,8,12,...
    পূর্ণাঙ্ক widthTotalBytes() ধ্রুৱ override;
    bool isFourstate() ধ্রুৱ override { ঘূৰাই keyword().isFourstate(); }
    VBasicDTypeKwd keyword() ধ্রুৱ VL_MT_SAFE {  // Avoid using - use isSomething accessors instead
        ঘূৰাই m.m_keyword;
    }
    bool isBitLogic() ধ্রুৱ { ঘূৰাই keyword().isBitLogic(); }
    bool isBit() ধ্রুৱ { ঘূৰাই keyword().isBit(); }
    bool isCHandle() ধ্রুৱ VL_MT_STABLE { ঘূৰাই keyword().isCHandle(); }
    bool isDouble() ধ্রুৱ VL_MT_STABLE { ঘূৰাই keyword().isDouble(); }
    bool isEvent() ধ্রুৱ VL_MT_STABLE { ঘূৰাই keyword() == VBasicDTypeKwd::EVENT; }
    bool isForkSync() ধ্রুৱ VL_MT_SAFE { ঘূৰাই keyword() == VBasicDTypeKwd::FORK_SYNC; }
    bool isProcessRef() ধ্রুৱ VL_MT_SAFE { ঘূৰাই keyword() == VBasicDTypeKwd::PROCESS_REFERENCE; }
    bool isDelayScheduler() ধ্রুৱ VL_MT_SAFE {
        ঘূৰাই keyword() == VBasicDTypeKwd::DELAY_SCHEDULER;
    }
    bool isTriggerScheduler() ধ্রুৱ VL_MT_SAFE {
        ঘূৰাই keyword() == VBasicDTypeKwd::TRIGGER_SCHEDULER;
    }
    bool isDynamicTriggerScheduler() ধ্রুৱ VL_MT_SAFE {
        ঘূৰাই keyword() == VBasicDTypeKwd::DYNAMIC_TRIGGER_SCHEDULER;
    }
    bool isRandomGenerator() ধ্রুৱ VL_MT_SAFE {
        ঘূৰাই keyword() == VBasicDTypeKwd::RANDOM_GENERATOR;
    }
    bool isStdRandomGenerator() ধ্রুৱ VL_MT_SAFE {
        ঘূৰাই keyword() == VBasicDTypeKwd::RANDOM_STDGENERATOR;
    }
    bool isCovergroupInstHandle() ধ্রুৱ VL_MT_SAFE {
        ঘূৰাই keyword() == VBasicDTypeKwd::COVERGROUP_INSTHANDLE;
    }
    bool isOpaque() ধ্রুৱ VL_MT_SAFE { ঘূৰাই keyword().isOpaque(); }
    bool isString() ধ্রুৱ VL_MT_STABLE { ঘূৰাই keyword().isString(); }
    bool isZeroInit() ধ্রুৱ { ঘূৰাই keyword().isZeroInit(); }
    bool isRanged() ধ্রুৱ { ঘূৰাই rangep() || m.m_nrange.ranged(); }
    bool isDpiBitVec() ধ্রুৱ {  // DPI uses svBitVecVal
        ঘূৰাই keyword() == VBasicDTypeKwd::BIT && isRanged();
    }
    bool isDpiLogicVec() ধ্রুৱ {  // DPI uses svLogicVecVal
        ঘূৰাই keyword().isFourstate() && !(keyword() == VBasicDTypeKwd::LOGIC && !isRanged());
    }
    bool isDpiCLayout() ধ্রুৱ {
        ঘূৰাই keyword().isDpiCLayout()
               || (keyword().isSameish(VBasicDTypeKwd::LOGIC) && !isRanged());
    }
    bool isDpiPrimitive() ধ্রুৱ {  // DPI uses a primitive type
        ঘূৰাই !isDpiBitVec() && !isDpiLogicVec();
    }
    // Generally the lo/hi/left/right funcs should be used instead of nrange()
    ধ্রুৱ VNumRange& nrange() ধ্রুৱ { ঘূৰাই m.m_nrange; }
    অন্তঃস্থ পূর্ণাঙ্ক hi() ধ্রুৱ;
    অন্তঃস্থ পূর্ণাঙ্ক lo() ধ্রুৱ;
    অন্তঃস্থ পূর্ণাঙ্ক elements() ধ্রুৱ;
    পূর্ণাঙ্ক বাম() ধ্রুৱ { ঘূৰাই ascending() ? lo() : hi(); }  // How to show a declaration
    পূর্ণাঙ্ক ডান() ধ্রুৱ { ঘূৰাই ascending() ? hi() : lo(); }
    অন্তঃস্থ bool ascending() ধ্রুৱ;
    bool implicit() ধ্রুৱ { ঘূৰাই keyword() == VBasicDTypeKwd::LOGIC_IMPLICIT; }
    bool untyped() ধ্রুৱ { ঘূৰাই keyword() == VBasicDTypeKwd::UNTYPED; }
    VNumRange declRange() ধ্রুৱ { ঘূৰাই isRanged() ? VNumRange{বাম(), ডান()} : VNumRange{}; }
    শূন্য cvtRangeConst();  // Convert to smaller representation
    bool isCompound() ধ্রুৱ override { ঘূৰাই isString(); }
    bool isIntegralOrPacked() ধ্রুৱ override { ঘূৰাই keyword().isIntNumeric(); }
};
class AstBracketArrayDType final : সার্ৱজনিক Ast_গ্ৰন্থি_DType {
    // Associative/Queue/Normal array data type, ie "[dtype_or_expr]"
    // only for early parsing then becomes another data type
    // @astgen op1 := childDTypep : Optional[AstNodeDType] // moved to refDTypep() in V3Width
    // @astgen op2 := elementsp : AstNode  // Number of elements in array
সার্ৱজনিক:
    AstBracketArrayDType(File_সাৰি* fl, VFlagChildDType, Ast_গ্ৰন্থি_DType* childDTypep,
                         Ast_গ্ৰন্থি* elementsp)
        : ASTGEN_SUPER_BracketArrayDType(fl) {
        this->childDTypep(childDTypep);
        this->elementsp(elementsp);
    }
    ASTGEN_MEMBERS_AstBracketArrayDType;
    bool similar_DType_গ্ৰন্থি(ধ্রুৱ Ast_গ্ৰন্থি_DType* samep) ধ্রুৱ override { ঘূৰাই this == samep; }
    Ast_গ্ৰন্থি_DType* subDTypep() ধ্রুৱ override VL_MT_STABLE { ঘূৰাই childDTypep(); }
    // METHODS
    // Will be removed in V3Width, which relies on this
    // being a child not a dtype pointed node
    bool maybePointedTo() ধ্রুৱ override VL_MT_SAFE { ঘূৰাই false; }
    AstBasicDType* basicp() ধ্রুৱ override VL_MT_STABLE { ঘূৰাই nullptr; }
    পূর্ণাঙ্ক widthAlignBytes() ধ্রুৱ override { V3ERROR_NA_RETURN(0); }
    পূর্ণাঙ্ক widthTotalBytes() ধ্রুৱ override { V3ERROR_NA_RETURN(0); }
    bool isCompound() ধ্রুৱ override { ঘূৰাই true; }
};
class AstCDType final : সার্ৱজনিক Ast_গ্ৰন্থি_DType {
    // Raw "C" data type passed directly to output
    string m_name;  // Name of data type, printed when do V3EmitC
সার্ৱজনিক:
    AstCDType(File_সাৰি* fl, ধ্রুৱ string& name)
        : ASTGEN_SUPER_CDType(fl)
        , m_name{name} {
        dtypep(this);
    }

    ASTGEN_MEMBERS_AstCDType;
    bool same_গ্ৰন্থি(ধ্রুৱ Ast_গ্ৰন্থি* samep) ধ্রুৱ override {
        ধ্রুৱ AstCDType* ধ্রুৱ asamep = VN_DBG_AS(samep, CDType);
        ঘূৰাই m_name == asamep->m_name;
    }
    bool similar_DType_গ্ৰন্থি(ধ্রুৱ Ast_গ্ৰন্থি_DType* samep) ধ্রুৱ override { ঘূৰাই same_গ্ৰন্থি(samep); }
    string name() ধ্রুৱ override VL_MT_STABLE { ঘূৰাই m_name; }
    string prettyDTypeName(bool) ধ্রুৱ override { ঘূৰাই m_name; }
    // METHODS
    AstBasicDType* basicp() ধ্রুৱ override VL_MT_STABLE { ঘূৰাই nullptr; }
    পূর্ণাঙ্ক widthAlignBytes() ধ্রুৱ override { ঘূৰাই 8; }  // Assume
    পূর্ণাঙ্ক widthTotalBytes() ধ্রুৱ override { ঘূৰাই 8; }  // Assume
    bool isCompound() ধ্রুৱ override { ঘূৰাই true; }
    // Return the global runtime type storing the requested number of bits.
    স্থির string typeToHold(পূর্ণাঙ্ক width) {
        যদি (width <= 8) ঘূৰাই "::CData";
        যদি (width <= 16) ঘূৰাই "::SData";
        যদি (width <= VL_IDATASIZE) ঘূৰাই "::IData";
        যদি (width <= VL_QUADSIZE) ঘূৰাই "::QData";

        ঘূৰাই "::VlWide<" + std::to_string(VL_শব্দগুলি_I(width)) + ">";
    }
};
class AstClassRefDType final : সার্ৱজনিক Ast_গ্ৰন্থি_DType {
    // Reference to a class
    // @astgen op1 := paramsp: List[AstPin]
    //
    // @astgen ptr := m_classp : Optional[AstClass]  // data type pointed to, BELOW the AstTypedef
    // @astgen ptr := m_classOrPackagep : Optional[AstNodeModule]  // Package hierarchy
    bool m_rawPointer = false;  // Emit as a non-owning C++ pointer rather than VlClassRef
সার্ৱজনিক:
    AstClassRefDType(File_সাৰি* fl, AstClass* classp, AstPin* paramsp)
        : ASTGEN_SUPER_ClassRefDType(fl)
        , m_classp{classp} {
        dtypep(this);
        addParamsp(paramsp);
    }
    ASTGEN_MEMBERS_AstClassRefDType;
    // METHODS
    bool same_গ্ৰন্থি(ধ্রুৱ Ast_গ্ৰন্থি* samep) ধ্রুৱ override {
        ধ্রুৱ AstClassRefDType* ধ্রুৱ asamep = VN_DBG_AS(samep, ClassRefDType);
        ঘূৰাই (m_classp == asamep->m_classp && m_classOrPackagep == asamep->m_classOrPackagep
                && m_rawPointer == asamep->m_rawPointer);
    }
    bool similar_DType_গ্ৰন্থি(ধ্রুৱ Ast_গ্ৰন্থি_DType* samep) ধ্রুৱ override;
    শূন্য dump(std::ostream& str = std::cout) ধ্রুৱ override;
    শূন্য dumpJson(std::ostream& str = std::cout) ধ্রুৱ override;
    শূন্য dumpSmall(std::ostream& str) ধ্রুৱ override;
    string prettyDTypeName(bool full) ধ্রুৱ override;
    string pretty_Name_বাৰ্তা() ধ্রুৱ override;
    string name() ধ্রুৱ override VL_MT_STABLE;
    AstBasicDType* basicp() ধ্রুৱ override VL_MT_STABLE { ঘূৰাই nullptr; }
    পূর্ণাঙ্ক widthAlignBytes() ধ্রুৱ override { ঘূৰাই 0; }
    পূর্ণাঙ্ক widthTotalBytes() ধ্রুৱ override { ঘূৰাই 0; }
    Ast_গ্ৰন্থি_DType* virtRefDTypep() ধ্রুৱ override { ঘূৰাই nullptr; }
    শূন্য virtRefDTypep(Ast_গ্ৰন্থি_DType* nodep) override {}
    Ast_গ্ৰন্থি_DType* subDTypep() ধ্রুৱ override VL_MT_STABLE { ঘূৰাই nullptr; }
    Ast_গ্ৰন্থি_Module* classOrPackagep() ধ্রুৱ { ঘূৰাই m_classOrPackagep; }
    শূন্য classOrPackagep(Ast_গ্ৰন্থি_Module* nodep) { m_classOrPackagep = nodep; }
    AstClass* classp() ধ্রুৱ VL_MT_STABLE { ঘূৰাই m_classp; }
    শূন্য classp(AstClass* nodep) { m_classp = nodep; }
    bool rawPointer() ধ্রুৱ { ঘূৰাই m_rawPointer; }
    শূন্য rawPointer(bool flag) { m_rawPointer = flag; }
    স্থির শূন্য selfTest();
    bool isCompound() ধ্রুৱ override { ঘূৰাই true; }
};
class AstConstDType final : সার্ৱজনিক Ast_গ্ৰন্থি_DType {
    // const data type, ie "const some_dtype var_name [2:0]"
    // ConstDType are removed in V3LinkLValue and become AstVar::isConst.
    // When more generic types are supported AstConstDType will be propagated further.
    // @astgen op1 := childDTypep : Optional[AstNodeDType]
    //
    // @astgen ptr := m_refDTypep : Optional[AstNodeDType]  // Inherit from this base data type
সার্ৱজনিক:
    AstConstDType(File_সাৰি* fl, VFlagChildDType, Ast_গ্ৰন্থি_DType* dtp)
        : ASTGEN_SUPER_ConstDType(fl) {
        childDTypep(dtp);  // Only for parser
        refDTypep(nullptr);  // V3Width will resolve
        dtypep(nullptr);  // V3Width will resolve
        widthFromSub(subDTypep());
    }
    ASTGEN_MEMBERS_AstConstDType;
    ধ্রুৱ ৱর্ণ* broken() ধ্রুৱ override {
        BROKEN_RTN(!((m_refDTypep && !childDTypep()) || (!m_refDTypep && childDTypep())));
        ঘূৰাই nullptr;
    }
    bool same_গ্ৰন্থি(ধ্রুৱ Ast_গ্ৰন্থি* samep) ধ্রুৱ override {
        ধ্রুৱ AstConstDType* ধ্রুৱ sp = VN_DBG_AS(samep, ConstDType);
        ঘূৰাই (m_refDTypep == sp->m_refDTypep);
    }
    bool similar_DType_গ্ৰন্থি(ধ্রুৱ Ast_গ্ৰন্থি_DType* samep) ধ্রুৱ override {
        ঘূৰাই skipRefp()->similarDType(samep->skipRefp());
    }
    Ast_গ্ৰন্থি_DType* getChildDTypep() ধ্রুৱ override { ঘূৰাই childDTypep(); }
    Ast_গ্ৰন্থি_DType* subDTypep() ধ্রুৱ override VL_MT_STABLE {
        ঘূৰাই m_refDTypep ? m_refDTypep : childDTypep();
    }
    শূন্য refDTypep(Ast_গ্ৰন্থি_DType* nodep) { m_refDTypep = nodep; }
    Ast_গ্ৰন্থি_DType* virtRefDTypep() ধ্রুৱ override { ঘূৰাই m_refDTypep; }
    শূন্য virtRefDTypep(Ast_গ্ৰন্থি_DType* nodep) override { refDTypep(nodep); }
    // METHODS
    AstBasicDType* basicp() ধ্রুৱ override VL_MT_STABLE { ঘূৰাই subDTypep()->basicp(); }
    পূর্ণাঙ্ক widthAlignBytes() ধ্রুৱ override { ঘূৰাই subDTypep()->widthAlignBytes(); }
    পূর্ণাঙ্ক widthTotalBytes() ধ্রুৱ override { ঘূৰাই subDTypep()->widthTotalBytes(); }
    bool isCompound() ধ্রুৱ override {
        v3fatalSrc("call isCompound on subdata type, not reference");
        ঘূৰাই false;
    }
};
class AstConstraintRefDType final : সার্ৱজনিক Ast_গ্ৰন্থি_DType {
    // For e.g. a reference to constraint for constraint_mode
সার্ৱজনিক:
    explicit AstConstraintRefDType(File_সাৰি* fl)
        : ASTGEN_SUPER_ConstraintRefDType(fl) {
        dtypep(this);
    }
    ASTGEN_MEMBERS_AstConstraintRefDType;
    bool hasDType() ধ্রুৱ override VL_MT_SAFE { ঘূৰাই true; }
    bool maybePointedTo() ধ্রুৱ override VL_MT_SAFE { ঘূৰাই true; }
    bool undead() ধ্রুৱ override { ঘূৰাই true; }
    Ast_গ্ৰন্থি_DType* subDTypep() ধ্রুৱ override VL_MT_STABLE { ঘূৰাই nullptr; }
    Ast_গ্ৰন্থি_DType* virtRefDTypep() ধ্রুৱ override { ঘূৰাই nullptr; }
    শূন্য virtRefDTypep(Ast_গ্ৰন্থি_DType* nodep) override {}
    bool similar_DType_গ্ৰন্থি(ধ্রুৱ Ast_গ্ৰন্থি_DType* samep) ধ্রুৱ override { ঘূৰাই this == samep; }
    AstBasicDType* basicp() ধ্রুৱ override VL_MT_STABLE { ঘূৰাই nullptr; }
    পূর্ণাঙ্ক widthAlignBytes() ধ্রুৱ override { ঘূৰাই 1; }
    পূর্ণাঙ্ক widthTotalBytes() ধ্রুৱ override { ঘূৰাই 1; }
    bool isCompound() ধ্রুৱ override { ঘূৰাই false; }
};
class AstCoverCrossDType final : সার্ৱজনিক Ast_গ্ৰন্থি_DType {
    // Borrowed pointer to VlCoverCrossT<...> or construction-time VlCoverCrossDyn.
    ধ্রুৱ uint32_t m_dimensions;
    ধ্রুৱ uint32_t m_tuples;  // Fixed capacity number of cross tuples (VlCoverCrossT's Tuples)
    ধ্রুৱ uint32_t m_bins;  // Fixed capacity number of explicit bins (VlCoverCrossT's Bins)
    ধ্রুৱ uint32_t m_autoBins;  // Fixed capacity number of auto bins (VlCoverCrossT's AutoBins)
    ধ্রুৱ uint64_t m_bin_শব্দগুলি;  // Fixed capacity selection words (VlCoverCrossT's BinWords)
    ধ্রুৱ bool m_dynamic;  // Bin layout is determined at covergroup construction

সার্ৱজনিক:
    AstCoverCrossDType(File_সাৰি* fl, uint32_t dimensions, uint32_t tuples, uint32_t bins,
                       uint32_t autoBins, uint64_t bin_শব্দগুলি, bool dynamic = false)
        : ASTGEN_SUPER_CoverCrossDType(fl)
        , m_dimensions{dimensions}
        , m_tuples{tuples}
        , m_bins{bins}
        , m_autoBins{autoBins}
        , m_bin_শব্দগুলি{bin_শব্দগুলি}
        , m_dynamic{dynamic} {
        dtypep(this);
    }
    ASTGEN_MEMBERS_AstCoverCrossDType;
    ধ্রুৱ ৱর্ণ* broken() ধ্রুৱ override {
        BROKEN_RTN(m_dimensions == 0);
        ঘূৰাই nullptr;
    }
    bool same_গ্ৰন্থি(ধ্রুৱ Ast_গ্ৰন্থি* samep) ধ্রুৱ override {  // LCOV_EXCL_START
        ধ্রুৱ AstCoverCrossDType* ধ্রুৱ sp = VN_DBG_AS(samep, CoverCrossDType);
        ঘূৰাই dimensions() == sp->dimensions() && tuples() == sp->tuples() && bins() == sp->bins()
               && autoBins() == sp->autoBins() && bin_শব্দগুলি() == sp->bin_শব্দগুলি()
               && isDynamic() == sp->isDynamic();
    }  // LCOV_EXCL_STOP
    bool similar_DType_গ্ৰন্থি(ধ্রুৱ Ast_গ্ৰন্থি_DType* samep) ধ্রুৱ override { ঘূৰাই this == samep; }
    শূন্য dump(std::ostream& str) ধ্রুৱ override;
    শূন্য dumpJson(std::ostream& str) ধ্রুৱ override;
    শূন্য dumpSmall(std::ostream& str) ধ্রুৱ override;
    uint32_t dimensions() ধ্রুৱ { ঘূৰাই m_dimensions; }
    uint32_t tuples() ধ্রুৱ { ঘূৰাই m_tuples; }
    uint32_t bins() ধ্রুৱ { ঘূৰাই m_bins; }
    uint32_t autoBins() ধ্রুৱ { ঘূৰাই m_autoBins; }
    uint64_t bin_শব্দগুলি() ধ্রুৱ { ঘূৰাই m_bin_শব্দগুলি; }
    bool isDynamic() ধ্রুৱ { ঘূৰাই m_dynamic; }
    string cppTemplateArgs() ধ্রুৱ;
    AstBasicDType* basicp() ধ্রুৱ override VL_MT_STABLE { ঘূৰাই nullptr; }
    পূর্ণাঙ্ক widthAlignBytes() ধ্রুৱ override { ঘূৰাই পরিমাণ(শূন্য*); }
    পূর্ণাঙ্ক widthTotalBytes() ধ্রুৱ override { ঘূৰাই পরিমাণ(শূন্য*); }
    bool isCompound() ধ্রুৱ override { ঘূৰাই true; }
};
class AstCoverpointDType final : সার্ৱজনিক Ast_গ্ৰন্থি_DType {
    // Borrowed pointer to a covergroup coverpoint runtime, 'VlCoverpointT<hitBound>*'.
    // Follows pattern of AstQueueDType in capturing the compile-time max bin overlap
    // template argument.
    uint32_t m_hitBound;  // VlCoverpointT<> template argument; hit list size, >= 1
সার্ৱজনিক:
    AstCoverpointDType(File_সাৰি* fl, uint32_t hitBound)
        : ASTGEN_SUPER_CoverpointDType(fl)
        , m_hitBound{hitBound} {
        dtypep(this);
    }
    ASTGEN_MEMBERS_AstCoverpointDType;
    ধ্রুৱ ৱর্ণ* broken() ধ্রুৱ override {
        BROKEN_RTN(m_hitBound < 1);
        ঘূৰাই nullptr;
    }
    // V3Covergroup interns these one-per-hitBound into the type table, so identity is
    // equality; there is never a second node with the same bound to compare against.
    bool similar_DType_গ্ৰন্থি(ধ্রুৱ Ast_গ্ৰন্থি_DType* samep) ধ্রুৱ override { ঘূৰাই this == samep; }
    শূন্য dumpSmall(std::ostream& str) ধ্রুৱ override;
    // ACCESSORS
    uint32_t hitBound() ধ্রুৱ { ঘূৰাই m_hitBound; }
    // METHODS
    AstBasicDType* basicp() ধ্রুৱ override VL_MT_STABLE { ঘূৰাই nullptr; }
    পূর্ণাঙ্ক widthAlignBytes() ধ্রুৱ override { ঘূৰাই পরিমাণ(শূন্য*); }
    পূর্ণাঙ্ক widthTotalBytes() ধ্রুৱ override { ঘূৰাই পরিমাণ(শূন্য*); }
    bool isCompound() ধ্রুৱ override { ঘূৰাই true; }
};
class AstDefImplicitDType final : সার্ৱজনিক Ast_গ্ৰন্থি_DType {
    // For parsing enum/struct/unions that are declared with a variable rather than typedef
    // This allows "var enum {...} a,b" to share the enum definition for both variables
    // After link, these become typedefs
    // @astgen op1 := childDTypep : Optional[AstNodeDType]
    string m_name;  // Data type name
    ধ্রুৱ পূর্ণাঙ্ক m_unique_অঙ্ক;  // Unique ID distinguishing this dtype instance, for hashing/naming

সার্ৱজনিক:
    AstDefImplicitDType(File_সাৰি* fl, ধ্রুৱ string& name, VFlagChildDType, Ast_গ্ৰন্থি_DType* dtp)
        : ASTGEN_SUPER_DefImplicitDType(fl)
        , m_name{name}
        , m_unique_অঙ্ক{unique_অঙ্ক_Inc()} {
        childDTypep(dtp);  // Only for parser
        dtypep(nullptr);  // V3Width will resolve
    }
    AstDefImplicitDType(ধ্রুৱ AstDefImplicitDType& other)
        : Ast_গ্ৰন্থি_DType(other)
        , m_name{other.m_name}
        , m_unique_অঙ্ক{unique_অঙ্ক_Inc()} {}
    ASTGEN_MEMBERS_AstDefImplicitDType;
    পূর্ণাঙ্ক unique_অঙ্ক() ধ্রুৱ { ঘূৰাই m_unique_অঙ্ক; }
    bool same_গ্ৰন্থি(ধ্রুৱ Ast_গ্ৰন্থি* samep) ধ্রুৱ override {
        ধ্রুৱ AstDefImplicitDType* ধ্রুৱ sp = VN_DBG_AS(samep, DefImplicitDType);
        ঘূৰাই unique_অঙ্ক() == sp->unique_অঙ্ক();
    }
    bool similar_DType_গ্ৰন্থি(ধ্রুৱ Ast_গ্ৰন্থি_DType* samep) ধ্রুৱ override { ঘূৰাই same_গ্ৰন্থি(samep); }
    Ast_গ্ৰন্থি_DType* getChildDTypep() ধ্রুৱ override { ঘূৰাই childDTypep(); }
    Ast_গ্ৰন্থি_DType* subDTypep() ধ্রুৱ override VL_MT_STABLE {
        ঘূৰাই dtypep() ? dtypep() : childDTypep();
    }
    // METHODS
    // op1 = Range of variable
    Ast_গ্ৰন্থি_DType* dtypeSkipRefp() ধ্রুৱ { ঘূৰাই dtypep()->skipRefp(); }
    AstBasicDType* basicp() ধ্রুৱ override VL_MT_STABLE { ঘূৰাই subDTypep()->basicp(); }
    পূর্ণাঙ্ক widthAlignBytes() ধ্রুৱ override { ঘূৰাই dtypep()->widthAlignBytes(); }
    পূর্ণাঙ্ক widthTotalBytes() ধ্রুৱ override { ঘূৰাই dtypep()->widthTotalBytes(); }
    string name() ধ্রুৱ override VL_MT_STABLE { ঘূৰাই m_name; }
    শূন্য name(ধ্রুৱ string& flag) override { m_name = flag; }
    bool isCompound() ধ্রুৱ override { ঘূৰাই false; }
};
class AstDynArrayDType final : সার্ৱজনিক Ast_গ্ৰন্থি_DType {
    // Dynamic array data type, ie "[]"
    // @astgen op1 := childDTypep : Optional[AstNodeDType] // moved to refDTypep() in V3Width
    //
    // @astgen ptr := m_refDTypep : Optional[AstNodeDType]  // Elements of this type (post-width)
সার্ৱজনিক:
    AstDynArrayDType(File_সাৰি* fl, VFlagChildDType, Ast_গ্ৰন্থি_DType* dtp)
        : ASTGEN_SUPER_DynArrayDType(fl) {
        childDTypep(dtp);  // Only for parser
        refDTypep(nullptr);
        dtypep(nullptr);  // V3Width will resolve
    }
    AstDynArrayDType(File_সাৰি* fl, Ast_গ্ৰন্থি_DType* dtp)
        : ASTGEN_SUPER_DynArrayDType(fl) {
        refDTypep(dtp);
        dtypep(nullptr);  // V3Width will resolve
    }
    ASTGEN_MEMBERS_AstDynArrayDType;
    ধ্রুৱ ৱর্ণ* broken() ধ্রুৱ override {
        BROKEN_RTN(!((m_refDTypep && !childDTypep()) || (!m_refDTypep && childDTypep())));
        ঘূৰাই nullptr;
    }
    bool same_গ্ৰন্থি(ধ্রুৱ Ast_গ্ৰন্থি* samep) ধ্রুৱ override {
        ধ্রুৱ AstDynArrayDType* ধ্রুৱ asamep = VN_DBG_AS(samep, DynArrayDType);
        যদি (!asamep->subDTypep()) ঘূৰাই false;
        ঘূৰাই subDTypep() == asamep->subDTypep();
    }
    bool similar_DType_গ্ৰন্থি(ধ্রুৱ Ast_গ্ৰন্থি_DType* samep) ধ্রুৱ override {
        ধ্রুৱ AstDynArrayDType* ধ্রুৱ asamep = VN_DBG_AS(samep, DynArrayDType);
        ঘূৰাই asamep->subDTypep() && subDTypep()->similarDType(asamep->subDTypep());
    }
    string prettyDTypeName(bool full) ধ্রুৱ override;
    শূন্য dumpSmall(std::ostream& str) ধ্রুৱ override;
    Ast_গ্ৰন্থি_DType* getChildDTypep() ধ্রুৱ override { ঘূৰাই childDTypep(); }
    Ast_গ্ৰন্থি_DType* subDTypep() ধ্রুৱ override VL_MT_STABLE {
        ঘূৰাই m_refDTypep ? m_refDTypep : childDTypep();
    }
    bool isAggregateType() ধ্রুৱ override { ঘূৰাই true; }
    শূন্য refDTypep(Ast_গ্ৰন্থি_DType* nodep) { m_refDTypep = nodep; }
    Ast_গ্ৰন্থি_DType* virtRefDTypep() ধ্রুৱ override { ঘূৰাই m_refDTypep; }
    শূন্য virtRefDTypep(Ast_গ্ৰন্থি_DType* nodep) override { refDTypep(nodep); }
    // METHODS
    AstBasicDType* basicp() ধ্রুৱ override VL_MT_STABLE { ঘূৰাই nullptr; }
    পূর্ণাঙ্ক widthAlignBytes() ধ্রুৱ override { ঘূৰাই subDTypep()->widthAlignBytes(); }
    পূর্ণাঙ্ক widthTotalBytes() ধ্রুৱ override { ঘূৰাই subDTypep()->widthTotalBytes(); }
    bool isCompound() ধ্রুৱ override { ঘূৰাই true; }
    bool isDynamicallySized() ধ্রুৱ override { ঘূৰাই true; }
};
class AstEmptyQueueDType final : সার্ৱজনিক Ast_গ্ৰন্থি_DType {
    // For EmptyQueue
সার্ৱজনিক:
    explicit AstEmptyQueueDType(File_সাৰি* fl)
        : ASTGEN_SUPER_EmptyQueueDType(fl) {
        dtypep(this);
    }
    ASTGEN_MEMBERS_AstEmptyQueueDType;
    শূন্য dumpSmall(std::ostream& str) ধ্রুৱ override;
    bool hasDType() ধ্রুৱ override VL_MT_SAFE { ঘূৰাই true; }
    bool maybePointedTo() ধ্রুৱ override VL_MT_SAFE { ঘূৰাই true; }
    bool undead() ধ্রুৱ override { ঘূৰাই true; }
    Ast_গ্ৰন্থি_DType* subDTypep() ধ্রুৱ override VL_MT_STABLE { ঘূৰাই nullptr; }
    Ast_গ্ৰন্থি_DType* virtRefDTypep() ধ্রুৱ override { ঘূৰাই nullptr; }
    শূন্য virtRefDTypep(Ast_গ্ৰন্থি_DType* nodep) override {}
    bool similar_DType_গ্ৰন্থি(ধ্রুৱ Ast_গ্ৰন্থি_DType* samep) ধ্রুৱ override { ঘূৰাই this == samep; }
    AstBasicDType* basicp() ধ্রুৱ override VL_MT_STABLE { ঘূৰাই nullptr; }
    পূর্ণাঙ্ক widthAlignBytes() ধ্রুৱ override { ঘূৰাই 1; }
    পূর্ণাঙ্ক widthTotalBytes() ধ্রুৱ override { ঘূৰাই 1; }
    bool isCompound() ধ্রুৱ override { ঘূৰাই false; }
};
class AstEnumDType final : সার্ৱজনিক Ast_গ্ৰন্থি_DType {
    // Parents: TYPEDEF/MODULE
    // @astgen op1 := childDTypep : Optional[AstNodeDType]
    // @astgen op2 := itemsp : List[AstEnumItem]
    //
    // @astgen ptr := m_refDTypep : Optional[AstNodeDType]  // Elements of this type (post-width)
    string m_name;  // Name from upper typedef, if any
    string m_typedefName;  // Typedef's dtypeName() once moved off the typedef declaring it
    ধ্রুৱ পূর্ণাঙ্ক m_unique_অঙ্ক;  // Unique ID distinguishing this dtype instance, for hashing/naming

সার্ৱজনিক:
    AstEnumDType(File_সাৰি* fl, VFlagChildDType, Ast_গ্ৰন্থি_DType* dtp, AstEnumItem* itemsp)
        : ASTGEN_SUPER_EnumDType(fl)
        , m_unique_অঙ্ক{unique_অঙ্ক_Inc()} {
        childDTypep(dtp);  // Only for parser
        refDTypep(nullptr);
        addItemsp(itemsp);
        dtypep(nullptr);  // V3Width will resolve
        widthFromSub(subDTypep());
    }
    AstEnumDType(ধ্রুৱ AstEnumDType& other)
        : Ast_গ্ৰন্থি_DType{other}
        , m_name{other.m_name}
        , m_typedefName{other.m_typedefName}
        , m_unique_অঙ্ক{unique_অঙ্ক_Inc()} {}
    ASTGEN_MEMBERS_AstEnumDType;

    ধ্রুৱ ৱর্ণ* broken() ধ্রুৱ override;
    পূর্ণাঙ্ক unique_অঙ্ক() ধ্রুৱ { ঘূৰাই m_unique_অঙ্ক; }
    bool same_গ্ৰন্থি(ধ্রুৱ Ast_গ্ৰন্থি* samep) ধ্রুৱ override {
        ধ্রুৱ AstEnumDType* ধ্রুৱ sp = VN_DBG_AS(samep, EnumDType);
        ঘূৰাই unique_অঙ্ক() == sp->unique_অঙ্ক();
    }
    bool similar_DType_গ্ৰন্থি(ধ্রুৱ Ast_গ্ৰন্থি_DType* samep) ধ্রুৱ override { ঘূৰাই same_গ্ৰন্থি(samep); }
    Ast_গ্ৰন্থি_DType* getChildDTypep() ধ্রুৱ override { ঘূৰাই childDTypep(); }
    Ast_গ্ৰন্থি_DType* subDTypep() ধ্রুৱ override VL_MT_STABLE {
        ঘূৰাই m_refDTypep ? m_refDTypep : childDTypep();
    }
    শূন্য refDTypep(Ast_গ্ৰন্থি_DType* nodep) { m_refDTypep = nodep; }
    Ast_গ্ৰন্থি_DType* virtRefDTypep() ধ্রুৱ override { ঘূৰাই m_refDTypep; }
    শূন্য virtRefDTypep(Ast_গ্ৰন্থি_DType* nodep) override { refDTypep(nodep); }
    string name() ধ্রুৱ override VL_MT_STABLE { ঘূৰাই m_name; }
    শূন্য name(ধ্রুৱ string& flag) override { m_name = flag; }
    // Keep the name the typedef declaring it gives, as it moves off it to the type table
    শূন্য typedefName(ধ্রুৱ string& name) { m_typedefName = name; }
    শূন্য dump(std::ostream& str = std::cout) ধ্রুৱ override;
    শূন্য dumpJson(std::ostream& str = std::cout) ধ্রুৱ override;
    শূন্য dumpSmall(std::ostream& str) ধ্রুৱ override;
    string prettyDTypeName(bool full) ধ্রুৱ override;
    // METHODS
    AstBasicDType* basicp() ধ্রুৱ override VL_MT_STABLE { ঘূৰাই subDTypep()->basicp(); }
    পূর্ণাঙ্ক widthAlignBytes() ধ্রুৱ override { ঘূৰাই subDTypep()->widthAlignBytes(); }
    পূর্ণাঙ্ক widthTotalBytes() ধ্রুৱ override { ঘূৰাই subDTypep()->widthTotalBytes(); }
    size_t item_গুনতি() ধ্রুৱ {
        size_t গুনতি = 0;
        বাবে (Ast_গ্ৰন্থি* itemp = itemsp(); itemp; itemp = itemp->nextp()) ++গুনতি;
        ঘূৰাই গুনতি;
    }
    bool isCompound() ধ্রুৱ override { ঘূৰাই false; }
};

class AstIfaceGenericDType final : সার্ৱজনিক Ast_গ্ৰন্থি_DType {
    // Generic interface that will be replaced with AstIfaceRefDType
    File_সাৰি* m_modportFileline;  // Where modport token was
    // dist-ast-dump-suppress  // Part of name()
    string m_modportName;  // "" = no modport
সার্ৱজনিক:
    explicit AstIfaceGenericDType(File_সাৰি* fl)
        : ASTGEN_SUPER_IfaceGenericDType(fl) {
        dtypep(this);
    }
    AstIfaceGenericDType(File_সাৰি* fl, File_সাৰি* modportFl, ধ্রুৱ string& modport)
        : ASTGEN_SUPER_IfaceGenericDType(fl)
        , m_modportFileline{modportFl}
        , m_modportName{modport} {
        dtypep(this);
    }
    ASTGEN_MEMBERS_AstIfaceGenericDType;
    শূন্য dumpSmall(std::ostream& str) ধ্রুৱ override;
    bool hasDType() ধ্রুৱ override VL_MT_SAFE { ঘূৰাই true; }
    bool maybePointedTo() ধ্রুৱ override VL_MT_SAFE { ঘূৰাই true; }
    bool undead() ধ্রুৱ override { ঘূৰাই true; }
    Ast_গ্ৰন্থি_DType* subDTypep() ধ্রুৱ override VL_MT_STABLE { ঘূৰাই nullptr; }
    Ast_গ্ৰন্থি_DType* virtRefDTypep() ধ্রুৱ override { ঘূৰাই nullptr; }
    শূন্য virtRefDTypep(Ast_গ্ৰন্থি_DType* nodep) override {}
    bool similar_DType_গ্ৰন্থি(ধ্রুৱ Ast_গ্ৰন্থি_DType* samep) ধ্রুৱ override { ঘূৰাই this == samep; }
    AstBasicDType* basicp() ধ্রুৱ override VL_MT_STABLE { ঘূৰাই nullptr; }
    পূর্ণাঙ্ক widthAlignBytes() ধ্রুৱ override { ঘূৰাই 1; }
    পূর্ণাঙ্ক widthTotalBytes() ধ্রুৱ override { ঘূৰাই 1; }
    string modportName() ধ্রুৱ { ঘূৰাই m_modportName; }
    bool isModport() { ঘূৰাই !m_modportName.empty(); }
    bool isCompound() ধ্রুৱ override { ঘূৰাই true; }
    File_সাৰি* modportFileline() ধ্রুৱ { ঘূৰাই m_modportFileline; }
    string name() ধ্রুৱ override { ঘূৰাই m_modportName; }
};

class AstIfaceRefDType final : সার্ৱজনিক Ast_গ্ৰন্থি_DType {
    // Reference to an interface, either for a port, or inside parent cell
    // @astgen op1 := paramsp : List[AstPin]
    //
    // @astgen ptr := m_ifacep : Optional[AstIface]  // Interface; cellp() should override
    // @astgen ptr := m_cellp : Optional[AstCell]  // When exact parent cell known; not a guess
    // @astgen ptr := m_modportp : Optional[AstModport]  // nullptr = unlinked or no modport
    File_সাৰি* m_modportFileline;  // Where modport token was
    string m_cellName;  // "" = no cell, such as when connects to 'input' iface
    string m_ifaceName;  // Interface name
    string m_modportName;  // "" = no modport
    bool m_virtual = false;  // True if virtual interface
সার্ৱজনিক:
    AstIfaceRefDType(File_সাৰি* fl, ধ্রুৱ string& cellName, ধ্রুৱ string& ifaceName)
        : ASTGEN_SUPER_IfaceRefDType(fl)
        , m_modportFileline{nullptr}
        , m_cellName{cellName}
        , m_ifaceName{ifaceName}
        , m_modportName{""} {}
    AstIfaceRefDType(File_সাৰি* fl, File_সাৰি* modportFl, ধ্রুৱ string& cellName,
                     ধ্রুৱ string& ifaceName, ধ্রুৱ string& modport)
        : ASTGEN_SUPER_IfaceRefDType(fl)
        , m_modportFileline{modportFl}
        , m_cellName{cellName}
        , m_ifaceName{ifaceName}
        , m_modportName{modport} {}
    AstIfaceRefDType(File_সাৰি* fl, File_সাৰি* modportFl, ধ্রুৱ string& cellName,
                     ধ্রুৱ string& ifaceName, ধ্রুৱ string& modport, AstPin* paramsp)
        : ASTGEN_SUPER_IfaceRefDType(fl)
        , m_modportFileline{modportFl}
        , m_cellName{cellName}
        , m_ifaceName{ifaceName}
        , m_modportName{modport} {
        addParamsp(paramsp);
    }
    ASTGEN_MEMBERS_AstIfaceRefDType;

    // METHODS
    শূন্য dump(std::ostream& str = std::cout) ধ্রুৱ override;
    শূন্য dumpJson(std::ostream& str = std::cout) ধ্রুৱ override;
    শূন্য dumpSmall(std::ostream& str) ধ্রুৱ override;
    string prettyDTypeName(bool full) ধ্রুৱ override;
    AstBasicDType* basicp() ধ্রুৱ override VL_MT_STABLE { ঘূৰাই nullptr; }
    bool similar_DType_গ্ৰন্থি(ধ্রুৱ Ast_গ্ৰন্থি_DType* samep) ধ্রুৱ override {
        // Each occurrence of a virtual interface type parses to its own node,
        // so pointer identity is insufficient; compare the referenced
        // interface instead, same equivalence as AstNode::computeCastable uses.
        ধ্রুৱ AstIfaceRefDType* ধ্রুৱ asamep = VN_DBG_AS(samep, IfaceRefDType);
        ঘূৰাই ifaceViaCellp() && ifaceViaCellp() == asamep->ifaceViaCellp()
               && modportName() == asamep->modportName() && isVirtual() == asamep->isVirtual();
    }
    পূর্ণাঙ্ক widthAlignBytes() ধ্রুৱ override { ঘূৰাই 0; }
    পূর্ণাঙ্ক widthTotalBytes() ধ্রুৱ override { ঘূৰাই 0; }
    bool isVirtual() ধ্রুৱ { ঘূৰাই m_virtual; }
    শূন্য isVirtual(bool flag) {
        m_virtual = flag;
        যদি (flag) v3Global.setHasVirtIfaces();
    }
    File_সাৰি* modportFileline() ধ্রুৱ { ঘূৰাই m_modportFileline; }
    শূন্য modportFileline(File_সাৰি* ধ্রুৱ modportFileline) { m_modportFileline = modportFileline; }
    string cellName() ধ্রুৱ { ঘূৰাই m_cellName; }
    শূন্য cellName(ধ্রুৱ string& name) { m_cellName = name; }
    string ifaceName() ধ্রুৱ { ঘূৰাই m_ifaceName; }
    string ifaceNameQ() ধ্রুৱ { ঘূৰাই "'" + prettyName(ifaceName()) + "'"; }
    শূন্য ifaceName(ধ্রুৱ string& name) { m_ifaceName = name; }
    শূন্য modportName(ধ্রুৱ string& modportName) { m_modportName = modportName; }
    string modportName() ধ্রুৱ { ঘূৰাই m_modportName; }
    AstIface* ifaceViaCellp() ধ্রুৱ;  // Use cellp or ifacep
    AstIface* ifacep() ধ্রুৱ { ঘূৰাই m_ifacep; }
    শূন্য ifacep(AstIface* nodep) { m_ifacep = nodep; }
    AstCell* cellp() ধ্রুৱ { ঘূৰাই m_cellp; }
    শূন্য cellp(AstCell* nodep) { m_cellp = nodep; }
    AstModport* modportp() ধ্রুৱ { ঘূৰাই m_modportp; }
    শূন্য modportp(AstModport* modportp) { m_modportp = modportp; }
    bool isModport() { ঘূৰাই !m_modportName.empty(); }
    bool isCompound() ধ্রুৱ override { ঘূৰাই true; }  // But not relevant
};
class AstMemberDType final : সার্ৱজনিক Ast_গ্ৰন্থি_DType {
    // A member of a struct/union
    // PARENT: AstNodeUOrStructDType
    // @astgen op1 := childDTypep : Optional[AstNodeDType]
    // @astgen op3 := valuep : Optional[AstNode]
    //
    // @astgen ptr := m_refDTypep : Optional[AstNodeDType]  // Elements of this type (post-width)
    string m_name;  // Name of variable
    string m_tag;  // Holds the string of the verilator tag -- used in JSON output.
    পূর্ণাঙ্ক m_lsb = -1;  // Within this level's packed struct, the LSB of the first bit of the member
    bool m_constrainedRand = false;  // Member has a constraint expression
    VRandAttr m_rand;  // Randomizability of this member (rand, randc, etc)
সার্ৱজনিক:
    AstMemberDType(File_সাৰি* fl, ধ্রুৱ string& name, VFlagChildDType, Ast_গ্ৰন্থি_DType* dtp,
                   Ast_গ্ৰন্থি* valuep)
        : ASTGEN_SUPER_MemberDType(fl)
        , m_name{name} {
        childDTypep(dtp);  // Only for parser
        this->valuep(valuep);
        dtypep(nullptr);  // V3Width will resolve
        refDTypep(nullptr);
    }
    AstMemberDType(File_সাৰি* fl, ধ্রুৱ string& name, Ast_গ্ৰন্থি_DType* dtp)
        : ASTGEN_SUPER_MemberDType(fl)
        , m_name{name} {
        UASSERT(dtp, "AstMember created with no dtype");
        refDTypep(dtp);
        dtypep(this);
        widthFromSub(subDTypep());
    }
    ASTGEN_MEMBERS_AstMemberDType;
    শূন্য dump(std::ostream& str = std::cout) ধ্রুৱ override;
    শূন্য dumpJson(std::ostream& str = std::cout) ধ্রুৱ override;
    শূন্য dumpSmall(std::ostream& str) ধ্রুৱ override;
    string prettyDTypeName(bool full) ধ্রুৱ override;
    string name() ধ্রুৱ override VL_MT_STABLE { ঘূৰাই m_name; }  // * = Var name
    bool hasDType() ধ্রুৱ override VL_MT_SAFE { ঘূৰাই true; }
    bool maybePointedTo() ধ্রুৱ override VL_MT_SAFE { ঘূৰাই true; }
    Ast_গ্ৰন্থি_DType* getChildDTypep() ধ্রুৱ override { ঘূৰাই childDTypep(); }
    Ast_গ্ৰন্থি_UOr_Struct_DType* getChildStructp();
    Ast_গ্ৰন্থি_DType* subDTypep() ধ্রুৱ override VL_MT_STABLE {
        ঘূৰাই m_refDTypep ? m_refDTypep : childDTypep();
    }
    শূন্য refDTypep(Ast_গ্ৰন্থি_DType* nodep) { m_refDTypep = nodep; }
    Ast_গ্ৰন্থি_DType* virtRefDTypep() ধ্রুৱ override { ঘূৰাই m_refDTypep; }
    শূন্য virtRefDTypep(Ast_গ্ৰন্থি_DType* nodep) override { refDTypep(nodep); }
    bool similar_DType_গ্ৰন্থি(ধ্রুৱ Ast_গ্ৰন্থি_DType* samep) ধ্রুৱ override { ঘূৰাই this == samep; }
    //
    // (Slow) recurse down to find basic data type (Note don't need virtual -
    // AstVar isn't a NodeDType)
    AstBasicDType* basicp() ধ্রুৱ override VL_MT_STABLE { ঘূৰাই subDTypep()->basicp(); }
    // op1 = Range of variable (Note don't need virtual - AstVar isn't a NodeDType)
    Ast_গ্ৰন্থি_DType* dtypeSkipRefp() ধ্রুৱ { ঘূৰাই subDTypep()->skipRefp(); }
    // (Slow) recurses - Structure alignment 1,2,4 or 8 bytes (arrays affect this)
    পূর্ণাঙ্ক widthAlignBytes() ধ্রুৱ override { ঘূৰাই subDTypep()->widthAlignBytes(); }
    // (Slow) recurses - Width in bytes rounding up 1,2,4,8,12,...
    পূর্ণাঙ্ক widthTotalBytes() ধ্রুৱ override { ঘূৰাই subDTypep()->widthTotalBytes(); }
    // METHODS
    শূন্য name(ধ্রুৱ string& name) override { m_name = name; }
    string tag() ধ্রুৱ override { ঘূৰাই m_tag; }
    শূন্য tag(ধ্রুৱ string& text) override { m_tag = text; }
    পূর্ণাঙ্ক lsb() ধ্রুৱ { ঘূৰাই m_lsb; }
    শূন্য lsb(পূর্ণাঙ্ক lsb) { m_lsb = lsb; }
    bool isCompound() ধ্রুৱ override {
        v3fatalSrc("call isCompound on subdata type, not reference");
        ঘূৰাই false;
    }
    bool isConstrainedRand() ধ্রুৱ { ঘূৰাই m_constrainedRand; }
    শূন্য markConstrainedRand(bool flag) { m_constrainedRand = flag; }
    VRandAttr rand() ধ্রুৱ { ঘূৰাই m_rand; }
    শূন্য rand(ধ্রুৱ VRandAttr flag) { m_rand = flag; }
};
class AstNBACommitQueueDType final : সার্ৱজনিক Ast_গ্ৰন্থি_DType {
    // @astgen ptr := m_subDTypep : AstNodeDType  // Type of the corresponding variable
    ধ্রুৱ bool m_partial;  // Partial element update required

সার্ৱজনিক:
    AstNBACommitQueueDType(File_সাৰি* fl, Ast_গ্ৰন্থি_DType* subDTypep, bool partial)
        : ASTGEN_SUPER_NBACommitQueueDType(fl)
        , m_partial{partial}
        , m_subDTypep{subDTypep} {
        dtypep(this);
    }
    ASTGEN_MEMBERS_AstNBACommitQueueDType;
    শূন্য dump(std::ostream& str) ধ্রুৱ override;
    শূন্য dumpJson(std::ostream& str) ধ্রুৱ override;
    Ast_গ্ৰন্থি_DType* subDTypep() ধ্রুৱ override VL_MT_STABLE { ঘূৰাই m_subDTypep; }
    bool partial() ধ্রুৱ { ঘূৰাই m_partial; }
    bool same_গ্ৰন্থি(ধ্রুৱ Ast_গ্ৰন্থি* samep) ধ্রুৱ override {
        ধ্রুৱ AstNBACommitQueueDType* ধ্রুৱ asamep = VN_DBG_AS(samep, NBACommitQueueDType);
        ঘূৰাই m_partial == asamep->m_partial;
    }
    bool similar_DType_গ্ৰন্থি(ধ্রুৱ Ast_গ্ৰন্থি_DType* samep) ধ্রুৱ override { ঘূৰাই this == samep; }
    AstBasicDType* basicp() ধ্রুৱ override VL_MT_STABLE { ঘূৰাই nullptr; }
    পূর্ণাঙ্ক widthAlignBytes() ধ্রুৱ override { ঘূৰাই 1; }
    পূর্ণাঙ্ক widthTotalBytes() ধ্রুৱ override { ঘূৰাই 24; }
    bool isCompound() ধ্রুৱ override { ঘূৰাই true; }
};
class AstParamTypeDType final : সার্ৱজনিক Ast_গ্ৰন্থি_DType {
    // Parents: MODULE
    // A parameter type statement; much like a var or typedef
    // @astgen op1 := childDTypep : Optional[AstNodeDType]
    ধ্রুৱ VVarType m_varType;  // Type of variable (for localparam vs. param)
    ধ্রুৱ VFwdType m_fwdType;  // Forward type for lint check
    string m_name;  // Name of variable
সার্ৱজনিক:
    AstParamTypeDType(File_সাৰি* fl, VVarType type, VFwdType fwdType, ধ্রুৱ string& name,
                      VFlagChildDType, Ast_গ্ৰন্থি_DType* dtp)
        : ASTGEN_SUPER_ParamTypeDType(fl)
        , m_varType{type}
        , m_fwdType{fwdType}
        , m_name{name} {
        childDTypep(dtp);  // Only for parser
        dtypep(nullptr);  // V3Width will resolve
    }
    ASTGEN_MEMBERS_AstParamTypeDType;
    শূন্য dump(std::ostream& str = std::cout) ধ্রুৱ override;
    শূন্য dumpJson(std::ostream& str = std::cout) ধ্রুৱ override;
    string prettyDTypeName(bool full) ধ্রুৱ override;
    Ast_গ্ৰন্থি_DType* getChildDTypep() ধ্রুৱ override { ঘূৰাই childDTypep(); }
    Ast_গ্ৰন্থি_DType* subDTypep() ধ্রুৱ override VL_MT_STABLE {
        ঘূৰাই dtypep() ? dtypep() : childDTypep();
    }
    AstBasicDType* basicp() ধ্রুৱ override VL_MT_STABLE { ঘূৰাই subDTypep()->basicp(); }
    bool similar_DType_গ্ৰন্থি(ধ্রুৱ Ast_গ্ৰন্থি_DType* samep) ধ্রুৱ override {
        ধ্রুৱ AstParamTypeDType* ধ্রুৱ sp = VN_DBG_AS(samep, ParamTypeDType);
        ঘূৰাই this->subDTypep()->similarDType(sp->subDTypep());
    }
    পূর্ণাঙ্ক widthAlignBytes() ধ্রুৱ override { ঘূৰাই dtypep()->widthAlignBytes(); }
    পূর্ণাঙ্ক widthTotalBytes() ধ্রুৱ override { ঘূৰাই dtypep()->widthTotalBytes(); }
    // METHODS
    string name() ধ্রুৱ override VL_MT_STABLE { ঘূৰাই m_name; }
    bool maybePointedTo() ধ্রুৱ override VL_MT_SAFE { ঘূৰাই true; }
    bool hasDType() ধ্রুৱ override VL_MT_SAFE { ঘূৰাই true; }
    শূন্য name(ধ্রুৱ string& flag) override { m_name = flag; }
    VVarType varType() ধ্রুৱ { ঘূৰাই m_varType; }  // * = Type of variable
    VFwdType fwdType() ধ্রুৱ { ঘূৰাই m_fwdType; }
    bool isParam() ধ্রুৱ { ঘূৰাই true; }
    bool isGParam() ধ্রুৱ { ঘূৰাই (varType() == VVarType::GPARAM); }
    bool isCompound() ধ্রুৱ override {
        v3fatalSrc("call isCompound on subdata type, not reference");
        ঘূৰাই false;
    }
};
class AstParseTypeDType final : সার্ৱজনিক Ast_গ্ৰন্থি_DType {
    // Parents: VAR
    // During parsing, this indicates the type of a parameter is a "parameter type"
    // e.g. the data type is a container of any data type
    ধ্রুৱ VFwdType m_fwdType;  // Forward type for lint check
সার্ৱজনিক:
    explicit AstParseTypeDType(File_সাৰি* fl, VFwdType fwdType = VFwdType::NONE)
        : ASTGEN_SUPER_ParseTypeDType(fl)
        , m_fwdType{fwdType} {}
    ASTGEN_MEMBERS_AstParseTypeDType;
    Ast_গ্ৰন্থি_DType* dtypep() ধ্রুৱ VL_MT_STABLE { ঘূৰাই nullptr; }
    // METHODS
    শূন্য dump(std::ostream& str = std::cout) ধ্রুৱ override;
    শূন্য dumpJson(std::ostream& str = std::cout) ধ্রুৱ override;
    bool similar_DType_গ্ৰন্থি(ধ্রুৱ Ast_গ্ৰন্থি_DType* samep) ধ্রুৱ override { ঘূৰাই this == samep; }
    AstBasicDType* basicp() ধ্রুৱ override VL_MT_STABLE { ঘূৰাই nullptr; }
    পূর্ণাঙ্ক widthAlignBytes() ধ্রুৱ override { ঘূৰাই 0; }
    পূর্ণাঙ্ক widthTotalBytes() ধ্রুৱ override { ঘূৰাই 0; }
    bool isCompound() ধ্রুৱ override {
        v3fatalSrc("call isCompound on subdata type, not reference");
        ঘূৰাই false;
    }
    VFwdType fwdType() ধ্রুৱ { ঘূৰাই m_fwdType; }
};
class AstQueueDType final : সার্ৱজনিক Ast_গ্ৰন্থি_DType {
    // Queue array data type, ie "[ $ ]"
    // @astgen op1 := childDTypep : Optional[AstNodeDType] // moved to refDTypep() in V3Width
    // @astgen op2 := boundp : Optional[AstNodeExpr]
    //
    // @astgen ptr := m_refDTypep : Optional[AstNodeDType]  // Elements of this type (post-width)
সার্ৱজনিক:
    AstQueueDType(File_সাৰি* fl, VFlagChildDType, Ast_গ্ৰন্থি_DType* dtp, Ast_গ্ৰন্থি_ৰাশিমালা* boundp)
        : ASTGEN_SUPER_QueueDType(fl) {
        childDTypep(dtp);
        this->boundp(boundp);
        refDTypep(nullptr);
        dtypep(nullptr);  // V3Width will resolve
    }
    AstQueueDType(File_সাৰি* fl, Ast_গ্ৰন্থি_DType* dtp, Ast_গ্ৰন্থি_ৰাশিমালা* boundp)
        : ASTGEN_SUPER_QueueDType(fl) {
        this->boundp(boundp);
        refDTypep(dtp);
        dtypep(dtp);
    }
    ASTGEN_MEMBERS_AstQueueDType;
    ধ্রুৱ ৱর্ণ* broken() ধ্রুৱ override {
        BROKEN_RTN(!((m_refDTypep && !childDTypep()) || (!m_refDTypep && childDTypep())));
        ঘূৰাই nullptr;
    }
    bool same_গ্ৰন্থি(ধ্রুৱ Ast_গ্ৰন্থি* samep) ধ্রুৱ override {
        ধ্রুৱ AstQueueDType* ধ্রুৱ asamep = VN_DBG_AS(samep, QueueDType);
        যদি (!asamep->subDTypep()) ঘূৰাই false;
        ঘূৰাই (subDTypep() == asamep->subDTypep());
    }
    bool similar_DType_গ্ৰন্থি(ধ্রুৱ Ast_গ্ৰন্থি_DType* samep) ধ্রুৱ override {
        ধ্রুৱ AstQueueDType* ধ্রুৱ asamep = VN_DBG_AS(samep, QueueDType);
        ঘূৰাই asamep->subDTypep() && subDTypep()->similarDType(asamep->subDTypep());
    }
    শূন্য dumpSmall(std::ostream& str) ধ্রুৱ override;
    string prettyDTypeName(bool full) ধ্রুৱ override;
    Ast_গ্ৰন্থি_DType* getChildDTypep() ধ্রুৱ override { ঘূৰাই childDTypep(); }
    Ast_গ্ৰন্থি_DType* subDTypep() ধ্রুৱ override VL_MT_STABLE {
        ঘূৰাই m_refDTypep ? m_refDTypep : childDTypep();
    }
    bool isAggregateType() ধ্রুৱ override { ঘূৰাই true; }
    শূন্য refDTypep(Ast_গ্ৰন্থি_DType* nodep) { m_refDTypep = nodep; }
    অন্তঃস্থ পূর্ণাঙ্ক boundConst() ধ্রুৱ VL_MT_STABLE;
    Ast_গ্ৰন্থি_DType* virtRefDTypep() ধ্রুৱ override { ঘূৰাই m_refDTypep; }
    শূন্য virtRefDTypep(Ast_গ্ৰন্থি_DType* nodep) override { refDTypep(nodep); }
    // METHODS
    AstBasicDType* basicp() ধ্রুৱ override VL_MT_STABLE { ঘূৰাই nullptr; }
    পূর্ণাঙ্ক widthAlignBytes() ধ্রুৱ override { ঘূৰাই subDTypep()->widthAlignBytes(); }
    পূর্ণাঙ্ক widthTotalBytes() ধ্রুৱ override { ঘূৰাই subDTypep()->widthTotalBytes(); }
    bool isCompound() ধ্রুৱ override { ঘূৰাই true; }
    bool isDynamicallySized() ধ্রুৱ override { ঘূৰাই true; }
};
class AstRefDType final : সার্ৱজনিক Ast_গ্ৰন্থি_DType {
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
    ধ্রুৱ VIfaceCaptureTag* m_captureTagp = nullptr;

সার্ৱজনিক:
    AstRefDType(File_সাৰি* fl, ধ্রুৱ string& name)
        : ASTGEN_SUPER_RefDType(fl)
        , m_name{name} {}
    AstRefDType(File_সাৰি* fl, ধ্রুৱ string& name, Ast_গ্ৰন্থি_ৰাশিমালা* classOrPackagep, AstPin* paramsp)
        : ASTGEN_SUPER_RefDType(fl)
        , m_name{name} {
        classOrPackageOpp(classOrPackagep);
        addParamsp(paramsp);
    }
    class Flag_Type_Of_ৰাশিমালা {};  // type(expr) for parser only
    AstRefDType(File_সাৰি* fl, Flag_Type_Of_ৰাশিমালা, Ast_গ্ৰন্থি* typeofp)
        : ASTGEN_SUPER_RefDType(fl) {
        this->typeofp(typeofp);
        যদি (Ast_গ্ৰন্থি_DType* ধ্রুৱ dtp = VN_CAST(typeofp, গ্ৰন্থি_DType)) refDTypep(dtp);
    }
    ASTGEN_MEMBERS_AstRefDType;
    // METHODS
    bool same_গ্ৰন্থি(ধ্রুৱ Ast_গ্ৰন্থি* samep) ধ্রুৱ override {
        ধ্রুৱ AstRefDType* ধ্রুৱ asamep = VN_DBG_AS(samep, RefDType);
        ঘূৰাই (m_typedefp == asamep->m_typedefp && m_refDTypep == asamep->m_refDTypep
                && m_name == asamep->m_name && m_classOrPackagep == asamep->m_classOrPackagep);
    }
    bool similar_DType_গ্ৰন্থি(ধ্রুৱ Ast_গ্ৰন্থি_DType* samep) ধ্রুৱ override {
        ঘূৰাই subDTypep()->similarDType(samep->subDTypep());
    }
    ধ্রুৱ ৱর্ণ* broken() ধ্রুৱ override;
    শূন্য dump(std::ostream& str = std::cout) ধ্রুৱ override;
    শূন্য dumpJson(std::ostream& str = std::cout) ধ্রুৱ override;
    শূন্য dumpSmall(std::ostream& str) ধ্রুৱ override;
    string name() ধ্রুৱ override VL_MT_STABLE { ঘূৰাই m_name; }
    string prettyDTypeName(bool full) ধ্রুৱ override {
        ঘূৰাই subDTypep() ? subDTypep()->prettyDTypeName(full) : prettyName();
    }
    AstBasicDType* basicp() ধ্রুৱ override VL_MT_STABLE {
        ঘূৰাই subDTypep() ? subDTypep()->basicp() : nullptr;
    }
    Ast_গ্ৰন্থি_DType* subDTypep() ধ্রুৱ override VL_MT_STABLE;
    Ast_গ্ৰন্থি_DType* getChildDTypep() ধ্রুৱ override { ঘূৰাই VN_CAST(typeofp(), গ্ৰন্থি_DType); }
    পূর্ণাঙ্ক widthAlignBytes() ধ্রুৱ override { ঘূৰাই dtypeSkipRefp()->widthAlignBytes(); }
    পূর্ণাঙ্ক widthTotalBytes() ধ্রুৱ override { ঘূৰাই dtypeSkipRefp()->widthTotalBytes(); }
    শূন্য name(ধ্রুৱ string& flag) override { m_name = flag; }
    Ast_গ্ৰন্থি_DType* dtypeSkipRefp() ধ্রুৱ { ঘূৰাই subDTypep()->skipRefp(); }
    AstTypedef* typedefp() ধ্রুৱ VL_MT_SAFE { ঘূৰাই m_typedefp; }
    শূন্য typedefp(AstTypedef* nodep) { m_typedefp = nodep; }
    Ast_গ্ৰন্থি_DType* refDTypep() ধ্রুৱ VL_MT_SAFE { ঘূৰাই m_refDTypep; }
    শূন্য refDTypep(Ast_গ্ৰন্থি_DType* nodep) { m_refDTypep = nodep; }
    Ast_গ্ৰন্থি_DType* virtRefDTypep() ধ্রুৱ override { ঘূৰাই refDTypep(); }
    শূন্য virtRefDTypep(Ast_গ্ৰন্থি_DType* nodep) override { refDTypep(nodep); }
    Ast_গ্ৰন্থি_Module* classOrPackagep() ধ্রুৱ { ঘূৰাই m_classOrPackagep; }
    শূন্য classOrPackagep(Ast_গ্ৰন্থি_Module* nodep) { m_classOrPackagep = nodep; }
    ধ্রুৱ VIfaceCaptureTag* captureTagp() ধ্রুৱ { ঘূৰাই m_captureTagp; }
    শূন্য captureTagp(ধ্রুৱ VIfaceCaptureTag* tagp) { m_captureTagp = tagp; }
    bool isCompound() ধ্রুৱ override {
        v3fatalSrc("call isCompound on subdata type, not reference");
        ঘূৰাই false;
    }
};
class AstRequireDType final : সার্ৱজনিক Ast_গ্ৰন্থি_DType {
    // @astgen op1 := lhsp : Optional[AstNode<AstNodeExpr|AstNodeDType>]
    //
    // Require a generic node type (typically AstParseRef become a type.
সার্ৱজনিক:
    AstRequireDType(File_সাৰি* fl, Ast_গ্ৰন্থি* lhsp)
        : ASTGEN_SUPER_RequireDType(fl) {
        this->lhsp(lhsp);
    }
    ASTGEN_MEMBERS_AstRequireDType;
    // METHODS
    bool similar_DType_গ্ৰন্থি(ধ্রুৱ Ast_গ্ৰন্থি_DType* samep) ধ্রুৱ override {
        ধ্রুৱ AstRequireDType* ধ্রুৱ asamep = VN_DBG_AS(samep, RequireDType);
        ঘূৰাই subDTypep()->similarDType(asamep->subDTypep());
    }
    AstBasicDType* basicp() ধ্রুৱ override VL_MT_STABLE { ঘূৰাই nullptr; }
    Ast_গ্ৰন্থি_DType* subDTypep() ধ্রুৱ override VL_MT_STABLE {
        // Used for recursive definition checking
        যদি (Ast_গ্ৰন্থি_DType* ধ্রুৱ dtp = VN_CAST(lhsp(), গ্ৰন্থি_DType)) ঘূৰাই dtp;

        ঘূৰাই nullptr;
    }
    পূর্ণাঙ্ক widthAlignBytes() ধ্রুৱ override { V3ERROR_NA_RETURN(1); }
    পূর্ণাঙ্ক widthTotalBytes() ধ্রুৱ override { V3ERROR_NA_RETURN(1); }
    bool isCompound() ধ্রুৱ override { V3ERROR_NA_RETURN(false); }
};
class AstSampleQueueDType final : সার্ৱজনিক Ast_গ্ৰন্থি_DType {
    // @astgen op1 := childDTypep : Optional[AstNodeDType] // moved to refDTypep() in V3Width
    //
    // @astgen ptr := m_refDTypep : Optional[AstNodeDType]  // Elements of this type (post-width)
সার্ৱজনিক:
    AstSampleQueueDType(File_সাৰি* fl, Ast_গ্ৰন্থি_DType* dtp)
        : ASTGEN_SUPER_SampleQueueDType(fl) {
        refDTypep(dtp);
        dtypep(dtp);
    }
    ASTGEN_MEMBERS_AstSampleQueueDType;
    ধ্রুৱ ৱর্ণ* broken() ধ্রুৱ override {
        BROKEN_RTN(!((m_refDTypep && !childDTypep()) || (!m_refDTypep && childDTypep())));
        ঘূৰাই nullptr;
    }
    bool same_গ্ৰন্থি(ধ্রুৱ Ast_গ্ৰন্থি* samep) ধ্রুৱ override {
        ধ্রুৱ AstSampleQueueDType* ধ্রুৱ asamep = VN_DBG_AS(samep, SampleQueueDType);
        যদি (!asamep->subDTypep()) ঘূৰাই false;
        ঘূৰাই (subDTypep() == asamep->subDTypep());
    }
    bool similar_DType_গ্ৰন্থি(ধ্রুৱ Ast_গ্ৰন্থি_DType* samep) ধ্রুৱ override {
        ধ্রুৱ AstSampleQueueDType* ধ্রুৱ asamep = VN_DBG_AS(samep, SampleQueueDType);
        ঘূৰাই asamep->subDTypep() && subDTypep()->similarDType(asamep->subDTypep());
    }
    শূন্য dumpSmall(std::ostream& str) ধ্রুৱ override;
    Ast_গ্ৰন্থি_DType* getChildDTypep() ধ্রুৱ override { ঘূৰাই childDTypep(); }
    // op1 = Range of variable
    Ast_গ্ৰন্থি_DType* subDTypep() ধ্রুৱ override VL_MT_STABLE {
        ঘূৰাই m_refDTypep ? m_refDTypep : childDTypep();
    }
    শূন্য refDTypep(Ast_গ্ৰন্থি_DType* nodep) { m_refDTypep = nodep; }
    Ast_গ্ৰন্থি_DType* virtRefDTypep() ধ্রুৱ override { ঘূৰাই m_refDTypep; }
    শূন্য virtRefDTypep(Ast_গ্ৰন্থি_DType* nodep) override { refDTypep(nodep); }
    // METHODS
    AstBasicDType* basicp() ধ্রুৱ override VL_MT_STABLE { ঘূৰাই subDTypep()->basicp(); }
    পূর্ণাঙ্ক widthAlignBytes() ধ্রুৱ override { ঘূৰাই পরিমাণ(std::map<std::string, std::string>); }
    পূর্ণাঙ্ক widthTotalBytes() ধ্রুৱ override { ঘূৰাই পরিমাণ(std::map<std::string, std::string>); }
    bool isCompound() ধ্রুৱ override { ঘূৰাই true; }
};
class AstStreamDType final : সার্ৱজনিক Ast_গ্ৰন্থি_DType {
    // Stream data type, used only as data type of stream operations
    // Should behave like AstPackArrayDType, but it doesn't have a size
সার্ৱজনিক:
    explicit AstStreamDType(File_সাৰি* fl)
        : ASTGEN_SUPER_StreamDType(fl) {
        dtypep(this);
    }
    ASTGEN_MEMBERS_AstStreamDType;
    শূন্য dumpSmall(std::ostream& str) ধ্রুৱ override;
    bool hasDType() ধ্রুৱ override VL_MT_SAFE { ঘূৰাই true; }
    bool maybePointedTo() ধ্রুৱ override VL_MT_SAFE { ঘূৰাই true; }
    bool undead() ধ্রুৱ override { ঘূৰাই true; }
    Ast_গ্ৰন্থি_DType* subDTypep() ধ্রুৱ override VL_MT_STABLE { ঘূৰাই nullptr; }
    Ast_গ্ৰন্থি_DType* virtRefDTypep() ধ্রুৱ override { ঘূৰাই nullptr; }
    শূন্য virtRefDTypep(Ast_গ্ৰন্থি_DType* nodep) override {}
    bool similar_DType_গ্ৰন্থি(ধ্রুৱ Ast_গ্ৰন্থি_DType* samep) ধ্রুৱ override { ঘূৰাই this == samep; }
    AstBasicDType* basicp() ধ্রুৱ override VL_MT_STABLE { ঘূৰাই nullptr; }
    পূর্ণাঙ্ক widthAlignBytes() ধ্রুৱ override { ঘূৰাই 1; }
    পূর্ণাঙ্ক widthTotalBytes() ধ্রুৱ override { ঘূৰাই 1; }
    bool isCompound() ধ্রুৱ override { ঘূৰাই false; }
};
class AstUnsizedArrayDType final : সার্ৱজনিক Ast_গ্ৰন্থি_DType {
    // Unsized/open-range Array data type, ie "some_dtype var_name []"
    // @astgen op1 := childDTypep : Optional[AstNodeDType] // moved to refDTypep() in V3Width
    //
    // @astgen ptr := m_refDTypep : Optional[AstNodeDType]  // Elements of this type (post-width)
সার্ৱজনিক:
    AstUnsizedArrayDType(File_সাৰি* fl, VFlagChildDType, Ast_গ্ৰন্থি_DType* dtp)
        : ASTGEN_SUPER_UnsizedArrayDType(fl) {
        childDTypep(dtp);  // Only for parser
        refDTypep(nullptr);
        dtypep(nullptr);  // V3Width will resolve
    }
    AstUnsizedArrayDType(File_সাৰি* fl, Ast_গ্ৰন্থি_DType* dtp)
        : ASTGEN_SUPER_UnsizedArrayDType(fl) {
        refDTypep(dtp);
        dtypep(nullptr);  // V3Width will resolve
    }
    ASTGEN_MEMBERS_AstUnsizedArrayDType;
    ধ্রুৱ ৱর্ণ* broken() ধ্রুৱ override {
        BROKEN_RTN(!((m_refDTypep && !childDTypep()) || (!m_refDTypep && childDTypep())));
        ঘূৰাই nullptr;
    }
    bool same_গ্ৰন্থি(ধ্রুৱ Ast_গ্ৰন্থি* samep) ধ্রুৱ override;
    bool similar_DType_গ্ৰন্থি(ধ্রুৱ Ast_গ্ৰন্থি_DType* samep) ধ্রুৱ override;
    শূন্য dumpSmall(std::ostream& str) ধ্রুৱ override;
    Ast_গ্ৰন্থি_DType* getChildDTypep() ধ্রুৱ override { ঘূৰাই childDTypep(); }
    Ast_গ্ৰন্থি_DType* subDTypep() ধ্রুৱ override VL_MT_STABLE {
        ঘূৰাই m_refDTypep ? m_refDTypep : childDTypep();
    }
    শূন্য refDTypep(Ast_গ্ৰন্থি_DType* nodep) { m_refDTypep = nodep; }
    Ast_গ্ৰন্থি_DType* virtRefDTypep() ধ্রুৱ override { ঘূৰাই m_refDTypep; }
    শূন্য virtRefDTypep(Ast_গ্ৰন্থি_DType* nodep) override { refDTypep(nodep); }
    // METHODS
    AstBasicDType* basicp() ধ্রুৱ override VL_MT_STABLE { ঘূৰাই subDTypep()->basicp(); }
    পূর্ণাঙ্ক widthAlignBytes() ধ্রুৱ override { ঘূৰাই subDTypep()->widthAlignBytes(); }
    পূর্ণাঙ্ক widthTotalBytes() ধ্রুৱ override { ঘূৰাই subDTypep()->widthTotalBytes(); }
    bool isCompound() ধ্রুৱ override { ঘূৰাই true; }
};
class AstVoidDType final : সার্ৱজনিক Ast_গ্ৰন্থি_DType {
    // For e.g. a function returning void
সার্ৱজনিক:
    explicit AstVoidDType(File_সাৰি* fl)
        : ASTGEN_SUPER_VoidDType(fl) {
        dtypep(this);
    }
    ASTGEN_MEMBERS_AstVoidDType;
    শূন্য dumpSmall(std::ostream& str) ধ্রুৱ override;
    bool hasDType() ধ্রুৱ override VL_MT_SAFE { ঘূৰাই true; }
    bool maybePointedTo() ধ্রুৱ override VL_MT_SAFE { ঘূৰাই true; }
    bool undead() ধ্রুৱ override { ঘূৰাই true; }
    Ast_গ্ৰন্থি_DType* subDTypep() ধ্রুৱ override VL_MT_STABLE { ঘূৰাই nullptr; }
    Ast_গ্ৰন্থি_DType* virtRefDTypep() ধ্রুৱ override { ঘূৰাই nullptr; }
    শূন্য virtRefDTypep(Ast_গ্ৰন্থি_DType* nodep) override {}
    bool similar_DType_গ্ৰন্থি(ধ্রুৱ Ast_গ্ৰন্থি_DType* samep) ধ্রুৱ override { ঘূৰাই true; }
    AstBasicDType* basicp() ধ্রুৱ override VL_MT_STABLE { ঘূৰাই nullptr; }
    পূর্ণাঙ্ক widthAlignBytes() ধ্রুৱ override { ঘূৰাই 1; }
    পূর্ণাঙ্ক widthTotalBytes() ধ্রুৱ override { ঘূৰাই 1; }
    bool isCompound() ধ্রুৱ override { ঘূৰাই false; }
};
class AstWildcardArrayDType final : সার্ৱজনিক Ast_গ্ৰন্থি_DType {
    // Wildcard index type associative array data type, ie "some_dtype var_name [*]"
    // @astgen op1 := childDTypep : Optional[AstNodeDType] // moved to refDTypep() in V3Width
    //
    // @astgen ptr := m_refDTypep : Optional[AstNodeDType]  // Elements of this type (post-width)
সার্ৱজনিক:
    AstWildcardArrayDType(File_সাৰি* fl, VFlagChildDType, Ast_গ্ৰন্থি_DType* dtp)
        : ASTGEN_SUPER_WildcardArrayDType(fl) {
        childDTypep(dtp);  // Only for parser
        refDTypep(nullptr);
        dtypep(nullptr);  // V3Width will resolve
    }
    ASTGEN_MEMBERS_AstWildcardArrayDType;
    ধ্রুৱ ৱর্ণ* broken() ধ্রুৱ override {
        BROKEN_RTN(!((m_refDTypep && !childDTypep()) || (!m_refDTypep && childDTypep())));
        ঘূৰাই nullptr;
    }
    bool same_গ্ৰন্থি(ধ্রুৱ Ast_গ্ৰন্থি* samep) ধ্রুৱ override;
    bool similar_DType_গ্ৰন্থি(ধ্রুৱ Ast_গ্ৰন্থি_DType* samep) ধ্রুৱ override;
    শূন্য dumpSmall(std::ostream& str) ধ্রুৱ override;
    string prettyDTypeName(bool full) ধ্রুৱ override;
    Ast_গ্ৰন্থি_DType* getChildDTypep() ধ্রুৱ override { ঘূৰাই childDTypep(); }
    Ast_গ্ৰন্থি_DType* subDTypep() ধ্রুৱ override VL_MT_STABLE {
        ঘূৰাই m_refDTypep ? m_refDTypep : childDTypep();
    }
    শূন্য refDTypep(Ast_গ্ৰন্থি_DType* nodep) { m_refDTypep = nodep; }
    Ast_গ্ৰন্থি_DType* virtRefDTypep() ধ্রুৱ override { ঘূৰাই m_refDTypep; }
    শূন্য virtRefDTypep(Ast_গ্ৰন্থি_DType* nodep) override { refDTypep(nodep); }
    // METHODS
    AstBasicDType* basicp() ধ্রুৱ override VL_MT_STABLE { ঘূৰাই subDTypep()->basicp(); }
    পূর্ণাঙ্ক widthAlignBytes() ধ্রুৱ override { ঘূৰাই পরিমাণ(std::map<std::string, std::string>); }
    পূর্ণাঙ্ক widthTotalBytes() ধ্রুৱ override { ঘূৰাই পরিমাণ(std::map<std::string, std::string>); }
    bool isCompound() ধ্রুৱ override { ঘূৰাই true; }
    bool isDynamicallySized() ধ্রুৱ override { ঘূৰাই true; }
};

// === AstNodeArrayDType ===
class AstPackArrayDType final : সার্ৱজনিক Ast_গ্ৰন্থি_Array_DType {
    // Packed array data type, ie "some_dtype [2:0] var_name"
সার্ৱজনিক:
    অন্তঃস্থ AstPackArrayDType(File_সাৰি* fl, VFlagChildDType, Ast_গ্ৰন্থি_DType* dtp, AstRange* rangep);
    অন্তঃস্থ AstPackArrayDType(File_সাৰি* fl, Ast_গ্ৰন্থি_DType* dtp, AstRange* rangep);
    ASTGEN_MEMBERS_AstPackArrayDType;
    string prettyDTypeName(bool full) ধ্রুৱ override;
    bool isCompound() ধ্রুৱ override { ঘূৰাই false; }
};
class AstUnpackArrayDType final : সার্ৱজনিক Ast_গ্ৰন্থি_Array_DType {
    // Array data type, ie "some_dtype var_name [2:0]"
    bool m_isCompound = false;  // Non-POD subDType, or parent requires compound
সার্ৱজনিক:
    AstUnpackArrayDType(File_সাৰি* fl, VFlagChildDType, Ast_গ্ৰন্থি_DType* dtp, AstRange* rangep)
        : ASTGEN_SUPER_UnpackArrayDType(fl) {
        childDTypep(dtp);  // Only for parser
        this->rangep(rangep);
        refDTypep(nullptr);
        dtypep(nullptr);  // V3Width will resolve
        // For backward compatibility AstNodeArrayDType and others inherit
        // width and signing from the subDType/base type
        widthFromSub(subDTypep());
    }
    AstUnpackArrayDType(File_সাৰি* fl, Ast_গ্ৰন্থি_DType* dtp, AstRange* rangep)
        : ASTGEN_SUPER_UnpackArrayDType(fl) {
        this->rangep(rangep);
        refDTypep(dtp);
        dtypep(this);
        // For backward compatibility AstNodeArrayDType and others inherit
        // width and signing from the subDType/base type
        widthFromSub(subDTypep());
    }
    ASTGEN_MEMBERS_AstUnpackArrayDType;
    শূন্য dump(std::ostream& str) ধ্রুৱ override;
    শূন্য dumpJson(std::ostream& str) ধ্রুৱ override;
    string prettyDTypeName(bool full) ধ্রুৱ override;
    bool same_গ্ৰন্থি(ধ্রুৱ Ast_গ্ৰন্থি* samep) ধ্রুৱ override {
        ধ্রুৱ AstUnpackArrayDType* ধ্রুৱ sp = VN_DBG_AS(samep, UnpackArrayDType);
        ঘূৰাই m_isCompound == sp->m_isCompound;
    }
    bool similar_DType_গ্ৰন্থি(ধ্রুৱ Ast_গ্ৰন্থি_DType* samep) ধ্রুৱ override;
    bool isAggregateType() ধ্রুৱ override { ঘূৰাই true; }
    // True iff 'otherp' is an array of the same shape, with
    // at least one dimension reversed ascending/descending.
    bool isReverseOf(ধ্রুৱ Ast_গ্ৰন্থি_DType* otherp) ধ্রুৱ;
    // Outer dimension comes first. The first element is this node.
    std::vector<AstUnpackArrayDType*> unpackDimensions();
    শূন্য isCompound(bool flag) { m_isCompound = flag; }
    bool isCompound() ধ্রুৱ override VL_MT_SAFE { ঘূৰাই m_isCompound; }
    bool isIntegralOrPacked() ধ্রুৱ override { ঘূৰাই false; }
};

// === AstNodeUOrStructDType ===
class AstStructDType final : সার্ৱজনিক Ast_গ্ৰন্থি_UOr_Struct_DType {
সার্ৱজনিক:
    // VSigning below is mispurposed to indicate if packed or not
    AstStructDType(File_সাৰি* fl, VSigning numericUnpack)
        : ASTGEN_SUPER_StructDType(fl, numericUnpack) {}
    ASTGEN_MEMBERS_AstStructDType;
    string verilogKwd() ধ্রুৱ override { ঘূৰাই "struct"; }
};
class AstUnionDType final : সার্ৱজনিক Ast_গ্ৰন্থি_UOr_Struct_DType {
    bool m_isSoft;  // Is a "union soft"
    bool m_isTagged;  // Is a "union tagged"

সার্ৱজনিক:
    // VSigning below is mispurposed to indicate if packed or not
    // isSoft implies packed
    AstUnionDType(File_সাৰি* fl, bool isSoft, bool isTagged, VSigning numericUnpack)
        : ASTGEN_SUPER_UnionDType(fl, numericUnpack)
        , m_isSoft{isSoft}
        , m_isTagged{isTagged} {
        packed(packed() | m_isSoft);
    }
    ASTGEN_MEMBERS_AstUnionDType;
    string verilogKwd() ধ্রুৱ override { ঘূৰাই "union"; }
    bool isSoft() ধ্রুৱ { ঘূৰাই m_isSoft; }
    bool isTagged() ধ্রুৱ { ঘূৰাই m_isTagged; }
    bool same_গ্ৰন্থি(ধ্রুৱ Ast_গ্ৰন্থি* samep) ধ্রুৱ override;
    শূন্য dump(std::ostream& str) ধ্রুৱ override;
    শূন্য dumpJson(std::ostream& str) ধ্রুৱ override;
};

#endif  // Guard
