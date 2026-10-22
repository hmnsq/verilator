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
#অন্তর্ভাব "V3Ast.h"  // This helps code analysis tools pick up symbols in V3Ast.h
#define VL_NOT_FINAL  // This #define fixes broken code folding in the CLion IDE
#endif

// === Abstract base node types (AstNode*) =====================================

class Ast_গ্রন্থি_DType VL_NOT_FINAL : সর্বজনীন Ast_গ্রন্থি {
    // Ideally width() would migrate to BasicDType as that's where it makes sense,
    // but it's currently so prevalent in the code we leave it here.
    // Note the below members are included in AstTypeTable::Key lookups
    // dist-ast-dump-suppress  // Part of dumpSmall
    পূর্ণসংখ্যা m_width = 0;  // (also in AstTypeTable::Key) Bit width of operation
    // (also in AstTypeTable::Key) If unsized, bitwidth of minimum implementation
    // dist-ast-dump-suppress  // Part of dumpSmall
    পূর্ণসংখ্যা m_widthMin = 0;
    // dist-ast-dump-suppress  // Part of dumpSmall
    VSigning m_numeric;  // (also in AstTypeTable::Key) Node is signed
    // dist-ast-dump-suppress  // Part of dumpSmall
    bool m_generic = false;  // Simple globally referenced type, don't garbage collect
    // Unique number assigned to each dtype during creation for IEEE matching
    স্থির পূর্ণসংখ্যা s_unique_অঙ্ক;

সুরক্ষিত:
    // CONSTRUCTORS
    Ast_গ্রন্থি_DType(VNType t, File_সারি* fl)
        : Ast_গ্রন্থি{t, fl} {}

ব্যক্তিগত:
    // METHODS
    ধ্রুবক Ast_গ্রন্থি_DType* skipRefIterp(bool skipConst, bool skipEnum,
                                     bool assertOn = true) ধ্রুবক VL_MT_STABLE;

সুরক্ষিত:
    // METHODS
    আভাসী bool similar_DType_গ্রন্থি(ধ্রুবক Ast_গ্রন্থি_DType* samep) ধ্রুবক = 0;

সর্বজনীন:
    ASTGEN_MEMBERS_Ast_গ্রন্থি_DType;
    // ACCESSORS
    শূন্য dump(std::ostream& str) ধ্রুবক override;
    শূন্য dumpJson(std::ostream& str) ধ্রুবক override;
    আভাসী শূন্য dumpSmall(std::ostream& str) ধ্রুবক VL_MT_STABLE;
    bool hasDType() ধ্রুবক override VL_MT_SAFE { ফেরত true; }
    /// Require VlUnpacked, instead of [] for POD elements.
    /// A non-POD object is always compound, but some POD elements
    /// are compound when methods calls operate on object, or when
    /// under another compound-requiring object e.g. class
    আভাসী bool isCompound() ধ্রুবক = 0;
    // Integral or packed, allowed inside an unpacked union/struct
    আভাসী bool isIntegralOrPacked() ধ্রুবক { ফেরত !isCompound(); }
    // (Slow) recurse down to find basic data type
    আভাসী AstBasicDType* basicp() ধ্রুবক VL_MT_STABLE = 0;
    // (Slow) Recurse over MemberDType|ParamTypeDType|RefDType|ConstDType|EnumDType to other type
    ধ্রুবক Ast_গ্রন্থি_DType* skipRefp() ধ্রুবক VL_MT_STABLE { ফেরত skipRefIterp(true, true); }
    Ast_গ্রন্থি_DType* skipRefp() VL_MT_STABLE {
        ফেরত const_cast<Ast_গ্রন্থি_DType*>(
            static_cast<ধ্রুবক Ast_গ্রন্থি_DType*>(this)->skipRefIterp(true, true));
    }
    // (Slow) Recurse over MemberDType|ParamTypeDType|RefDType|ConstDType|EnumDType to other type,
    // Returns null if not resolved
    ধ্রুবক Ast_গ্রন্থি_DType* skipRefOrNullp() ধ্রুবক VL_MT_STABLE {
        ফেরত skipRefIterp(true, true, false);
    }
    Ast_গ্রন্থি_DType* skipRefOrNullp() VL_MT_STABLE {
        ফেরত const_cast<Ast_গ্রন্থি_DType*>(
            static_cast<ধ্রুবক Ast_গ্রন্থি_DType*>(this)->skipRefIterp(true, true, false));
    }
    // (Slow) Recurse over MemberDType|ParamTypeDType|RefDType|EnumDType to ConstDType
    ধ্রুবক Ast_গ্রন্থি_DType* skipRefToConstp() ধ্রুবক { ফেরত skipRefIterp(false, true); }
    Ast_গ্রন্থি_DType* skipRefToConstp() {
        ফেরত const_cast<Ast_গ্রন্থি_DType*>(
            static_cast<ধ্রুবক Ast_গ্রন্থি_DType*>(this)->skipRefIterp(false, true));
    }
    // (Slow) Recurse over MemberDType|ParamTypeDType|RefDType|ConstDType to EnumDType
    ধ্রুবক Ast_গ্রন্থি_DType* skipRefToEnump() ধ্রুবক { ফেরত skipRefIterp(true, false); }
    Ast_গ্রন্থি_DType* skipRefToEnump() {
        ফেরত const_cast<Ast_গ্রন্থি_DType*>(
            static_cast<ধ্রুবক Ast_গ্রন্থি_DType*>(this)->skipRefIterp(true, false));
    }
    // (Slow) Recurse over MemberDType|ParamTypeDType|RefDType|ConstDType to EnumDType,
    // Returns null if not resolved
    ধ্রুবক Ast_গ্রন্থি_DType* skipRefToEnumOrNullp() ধ্রুবক { ফেরত skipRefIterp(true, false, false); }
    // (Slow) Recurse over MemberDType|ParamTypeDType|RefDType to other type
    ধ্রুবক Ast_গ্রন্থি_DType* skipRefToNonRefp() ধ্রুবক { ফেরত skipRefIterp(false, false); }
    Ast_গ্রন্থি_DType* skipRefToNonRefp() {
        ফেরত const_cast<Ast_গ্রন্থি_DType*>(
            static_cast<ধ্রুবক Ast_গ্রন্থি_DType*>(this)->skipRefIterp(false, false));
    }
    // If array, returns element dtype, otherwise returns skipRef dtype
    // If skipRef is false, RefDTypes are not followed (safe before typedef linking)
    ধ্রুবক Ast_গ্রন্থি_DType* elemDTypep(bool skipRef = true) ধ্রুবক VL_MT_STABLE;
    Ast_গ্রন্থি_DType* elemDTypep(bool skipRef = true) VL_MT_STABLE {
        ফেরত const_cast<Ast_গ্রন্থি_DType*>(
            static_cast<ধ্রুবক Ast_গ্রন্থি_DType*>(this)->elemDTypep(skipRef));
    }
    // (Slow) recurses - Structure alignment 1,2,4 or 8 bytes (arrays affect this)
    আভাসী পূর্ণসংখ্যা widthAlignBytes() ধ্রুবক = 0;
    // (Slow) recurses - Width in bytes rounding up 1,2,4,8,12,...
    আভাসী পূর্ণসংখ্যা width_যোগ_Bytes() ধ্রুবক = 0;
    bool maybePointedTo() ধ্রুবক override VL_MT_SAFE { ফেরত true; }
    // Iff has a non-null refDTypep(), as generic node function
    আভাসী Ast_গ্রন্থি_DType* virtRefDTypep() ধ্রুবক { ফেরত nullptr; }
    // Iff has refDTypep(), set as generic node function
    আভাসী শূন্য virtRefDTypep(Ast_গ্রন্থি_DType* nodep) {}
    // Iff has a non-null second dtypep, as generic node function
    আভাসী Ast_গ্রন্থি_DType* virtRefDType2p() ধ্রুবক { ফেরত nullptr; }
    // Iff has second dtype, set as generic node function
    আভাসী শূন্য virtRefDType2p(Ast_গ্রন্থি_DType* nodep) {}
    // Assignable equivalence.  Calls skipRefToNonRefp() during comparisons.
    bool similarDType(ধ্রুবক Ast_গ্রন্থি_DType* samep) ধ্রুবক;
    // Iff has a non-null subDTypep(), as generic node function
    আভাসী Ast_গ্রন্থি_DType* subDTypep() ধ্রুবক VL_MT_STABLE { ফেরত nullptr; }
    আভাসী Ast_গ্রন্থি_DType* subDType2p() ধ্রুবক VL_MT_STABLE { ফেরত nullptr; }
    আভাসী bool isAggregateType() ধ্রুবক { ফেরত false; }
    // True for unpacked, dynamic, queue, and associative arrays (not packed arrays)
    bool isNonPackedArray() ধ্রুবক;
    আভাসী bool isFourstate() ধ্রুবক;
    // Ideally an IEEE $typename
    আভাসী string prettyDTypeName(bool) ধ্রুবক { ফেরত prettyTypeName(); }
    string prettyDTypeNameQ(bool full = false) ধ্রুবক { ফেরত "'" + prettyDTypeName(full) + "'"; }
    string stateDTypeName() ধ্রুবক { ফেরত this->isFourstate() ? "(4-state)" : "(2-state)"; }
    //
    // Changing the width may confuse the data type resolution, so must clear
    // TypeTable cache after use.
    শূন্য widthForce(পূর্ণসংখ্যা width, পূর্ণসংখ্যা widthMin) {
        m_width = width;
        m_widthMin = widthMin;
    }
    // For backward compatibility inherit width and signing from the subDType/base type
    শূন্য widthFromSub(ধ্রুবক Ast_গ্রন্থি_DType* nodep) {
        m_width = nodep->m_width;
        m_widthMin = nodep->m_widthMin;
        m_numeric = nodep->m_numeric;
    }
    //
    পূর্ণসংখ্যা width() ধ্রুবক VL_MT_STABLE { ফেরত m_width; }
    শূন্য numeric(VSigning flag) { m_numeric = flag; }
    bool isSigned() ধ্রুবক VL_MT_STABLE { ফেরত m_numeric.isSigned(); }
    bool isNosign() ধ্রুবক VL_MT_SAFE { ফেরত m_numeric.isNosign(); }
    VSigning numeric() ধ্রুবক VL_MT_STABLE { ফেরত m_numeric; }
    পূর্ণসংখ্যা width_শব্দগুলি() ধ্রুবক VL_MT_STABLE { ফেরত VL_শব্দগুলি_I(width()); }
    পূর্ণসংখ্যা widthMin() ধ্রুবক VL_MT_STABLE {  // If sized, the size,
                                         // if unsized the min digits to represent it
        ফেরত m_widthMin ? m_widthMin : m_width;
    }
    পূর্ণসংখ্যা widthPow2() ধ্রুবক;
    শূন্য widthMinFromWidth() { m_widthMin = m_width; }
    bool widthSized() ধ্রুবক VL_MT_SAFE { ফেরত !m_widthMin || m_widthMin == m_width; }
    bool generic() ধ্রুবক VL_MT_SAFE { ফেরত m_generic; }
    শূন্য generic(bool flag) { m_generic = flag; }
    std::pair<uint32_t, uint32_t> dimensions(bool includeBasic) ধ্রুবক;
    uint32_t arrayUnpackedElements() ধ্রুবক;  // 1, or total multiplication of all dimensions
    // Fixed aggregate streaming properties
    bool isStreamableFixedAggregate() ধ্রুবক;
    // Whether is a string, or an unpacked array or structure containing one
    bool containsString() ধ্রুবক;
    bool containsUnpackedStruct() ধ্রুবক;
    পূর্ণসংখ্যা widthStream() ধ্রুবক;
    string vlEnumType() ধ্রুবক;  // Return VerilatedVarType: VLVT_UINT32, etc
    string vlEnumPackedAgg() ধ্রুবক;  // Return VLVF_PACKED_AGG etc flags, or empty
    স্থির পূর্ণসংখ্যা unique_অঙ্ক_Inc() { ফেরত ++s_unique_অঙ্ক; }
    ধ্রুবক অক্ষর* charIQWN() ধ্রুবক {
        ফেরত (isString() ? "N" : isWide() ? "W" : isDouble() ? "D" : isQuad() ? "Q" : "I");
    }
    string cType(ধ্রুবক string& name, bool forFunc, bool isRef, bool packed = false) ধ্রুবক;
    // Represents a C++ LiteralType? (can be constexpr)
    bool isLiteralType() ধ্রুবক VL_MT_STABLE;
    // C++ type (as emitted by cType) is trivially default constructible and destructible?
    bool isTriviallyConstructible() ধ্রুবক VL_MT_STABLE;
    আভাসী bool isDynamicallySized() ধ্রুবক { ফেরত false; }

ব্যক্তিগত:
    class CTypeRecursed;
    CTypeRecursed cTypeRecurse(bool compound, bool packed) ধ্রুবক;
};
class Ast_গ্রন্থি_Array_DType VL_NOT_FINAL : সর্বজনীন Ast_গ্রন্থি_DType {
    // Array data type, ie "some_dtype var_name [2:0]"
    // @astgen op1 := childDTypep : Optional[AstNodeDType] // moved to refDTypep() in V3Width
    // @astgen op2 := rangep : Optional[AstRange] // array bounds
    //
    // @astgen ptr := m_refDTypep : Optional[AstNodeDType]  // Elements of this type (post-width)
    Ast_গ্রন্থি* rangenp() ধ্রুবক { ফেরত reinterpret_cast<Ast_গ্রন্থি*>(rangep()); }

সুরক্ষিত:
    Ast_গ্রন্থি_Array_DType(VNType t, File_সারি* fl)
        : Ast_গ্রন্থি_DType{t, fl} {}

সর্বজনীন:
    ASTGEN_MEMBERS_Ast_গ্রন্থি_Array_DType;
    শূন্য dump(std::ostream& str) ধ্রুবক override;
    শূন্য dumpJson(std::ostream& str) ধ্রুবক override;
    শূন্য dumpSmall(std::ostream& str) ধ্রুবক override;
    ধ্রুবক অক্ষর* broken() ধ্রুবক override {
        BROKEN_RTN(!((m_refDTypep && !childDTypep()) || (!m_refDTypep && childDTypep())));
        ফেরত nullptr;
    }
    bool same_গ্রন্থি(ধ্রুবক Ast_গ্রন্থি* samep) ধ্রুবক override {
        ধ্রুবক Ast_গ্রন্থি_Array_DType* ধ্রুবক asamep = VN_DBG_AS(samep, গ্রন্থি_Array_DType);
        ফেরত hi() == asamep->hi() && rangenp()->same_বৃক্ষ(asamep->rangenp())
               && subDTypep() == asamep->subDTypep();
    }  // HashedDT doesn't recurse, so need to check children
    bool similar_DType_গ্রন্থি(ধ্রুবক Ast_গ্রন্থি_DType* samep) ধ্রুবক override {
        ধ্রুবক Ast_গ্রন্থি_Array_DType* ধ্রুবক asamep = VN_DBG_AS(samep, গ্রন্থি_Array_DType);
        ফেরত elementsConst() == asamep->elementsConst()
               && subDTypep()->similarDType(asamep->subDTypep());
    }
    Ast_গ্রন্থি_DType* getChildDTypep() ধ্রুবক override { ফেরত childDTypep(); }
    Ast_গ্রন্থি_DType* subDTypep() ধ্রুবক override VL_MT_STABLE {
        ফেরত m_refDTypep ? m_refDTypep : childDTypep();
    }
    শূন্য refDTypep(Ast_গ্রন্থি_DType* nodep) { m_refDTypep = nodep; }
    Ast_গ্রন্থি_DType* virtRefDTypep() ধ্রুবক override { ফেরত m_refDTypep; }
    শূন্য virtRefDTypep(Ast_গ্রন্থি_DType* nodep) override { refDTypep(nodep); }
    // METHODS
    AstBasicDType* basicp() ধ্রুবক override VL_MT_STABLE {
        ফেরত subDTypep()->basicp();
    }  // (Slow) recurse down to find basic data type
    পূর্ণসংখ্যা widthAlignBytes() ধ্রুবক override { ফেরত subDTypep()->widthAlignBytes(); }
    পূর্ণসংখ্যা width_যোগ_Bytes() ধ্রুবক override {
        ফেরত elementsConst() * subDTypep()->width_যোগ_Bytes();
    }
    অন্তঃরেখ পূর্ণসংখ্যা বাম() ধ্রুবক VL_MT_STABLE;
    অন্তঃরেখ পূর্ণসংখ্যা ডান() ধ্রুবক VL_MT_STABLE;
    অন্তঃরেখ পূর্ণসংখ্যা hi() ধ্রুবক VL_MT_STABLE;
    অন্তঃরেখ পূর্ণসংখ্যা lo() ধ্রুবক VL_MT_STABLE;
    অন্তঃরেখ পূর্ণসংখ্যা elementsConst() ধ্রুবক VL_MT_STABLE;
    অন্তঃরেখ VNumRange declRange() ধ্রুবক VL_MT_STABLE;
};
class Ast_গ্রন্থি_UOr_Struct_DType VL_NOT_FINAL : সর্বজনীন Ast_গ্রন্থি_DType {
    // A struct or union; common handling
    // @astgen op1 := membersp : List[AstMemberDType]
    //
    // @astgen ptr := m_classOrPackagep : Optional[AstNodeModule]  // Package emitted with
    string m_name;  // Name from upper typedef, if any
    string m_typedefName;  // Typedef's dtypeName() once moved off the typedef declaring it
    ধ্রুবক পূর্ণসংখ্যা m_unique_অঙ্ক;  // Unique ID distinguishing this dtype instance, for hashing/naming
    bool m_packed;  // Packed struct/union, else unpacked
    bool m_isFourstate = false;  // V3Width computes; true if any member is 4-state
    bool m_constrainedRand = false;  // True if struct has constraint expression
    bool m_emitToString = false;  // Generate to_string() for this struct/union if set

সুরক্ষিত:
    Ast_গ্রন্থি_UOr_Struct_DType(VNType t, File_সারি* fl, VSigning numericUnpack)
        : Ast_গ্রন্থি_DType{t, fl}
        , m_unique_অঙ্ক{unique_অঙ্ক_Inc()} {
        // VSigning::NOSIGN overloaded to indicate not packed
        m_packed = (numericUnpack != VSigning::NOSIGN);
        numeric(VSigning::fromBool(numericUnpack.isSigned()));
    }
    Ast_গ্রন্থি_UOr_Struct_DType(ধ্রুবক Ast_গ্রন্থি_UOr_Struct_DType& other)
        : Ast_গ্রন্থি_DType{other}
        , m_name{other.m_name}
        , m_typedefName{other.m_typedefName}
        , m_unique_অঙ্ক{unique_অঙ্ক_Inc()}
        , m_packed{other.m_packed}
        , m_isFourstate{other.m_isFourstate} {}

সর্বজনীন:
    ASTGEN_MEMBERS_Ast_গ্রন্থি_UOr_Struct_DType;
    পূর্ণসংখ্যা unique_অঙ্ক() ধ্রুবক { ফেরত m_unique_অঙ্ক; }
    শূন্য dump(std::ostream& str) ধ্রুবক override;
    শূন্য dumpJson(std::ostream& str) ধ্রুবক override;
    string prettyDTypeName(bool) ধ্রুবক override;
    bool isCompound() ধ্রুবক override { ফেরত !packed(); }
    // For basicp() we reuse the size to indicate a "fake" basic type of same size
    AstBasicDType* basicp() ধ্রুবক override VL_MT_STABLE {
        যদি (!m_packed) ফেরত nullptr;
        ফেরত (isFourstate()
                    ? VN_AS(findLogicRangeDType(VNumRange{width() - 1, 0}, width(), numeric()),
                            BasicDType)
                    : VN_AS(findBitRangeDType(VNumRange{width() - 1, 0}, width(), numeric()),
                            BasicDType));
    }
    // (Slow) recurses - Structure alignment 1,2,4 or 8 bytes (arrays affect this)
    পূর্ণসংখ্যা widthAlignBytes() ধ্রুবক override;
    // (Slow) recurses - Width in bytes rounding up 1,2,4,8,12,...
    পূর্ণসংখ্যা width_যোগ_Bytes() ধ্রুবক override;
    bool similar_DType_গ্রন্থি(ধ্রুবক Ast_গ্রন্থি_DType* samep) ধ্রুবক override;
    string name() ধ্রুবক override VL_MT_STABLE { ফেরত m_name; }
    শূন্য name(ধ্রুবক string& flag) override { m_name = flag; }
    // Keep the name the typedef declaring it gives, as it moves off it to the type table
    শূন্য typedefName(ধ্রুবক string& name) { m_typedefName = name; }
    // Name of the C structure for DPI, empty if no typedef names it, so declared where used
    string dpiName() ধ্রুবক;
    // C typedef of the unpacked structure for DPI (IEEE 1800-2023 H.7.3)
    string dpiTypedef() ধ্রুবক;
    bool packed() ধ্রুবক VL_MT_SAFE { ফেরত m_packed; }
    শূন্য packed(bool flag) { m_packed = flag; }
    // packed() but as don't support unpacked, presently all structs
    স্থির bool packedUnsup() { ফেরত true; }
    শূন্য isFourstate(bool flag) { m_isFourstate = flag; }
    bool isFourstate() ধ্রুবক override VL_MT_SAFE { ফেরত m_isFourstate; }
    স্থির পূর্ণসংখ্যা lo() VL_MT_STABLE { ফেরত 0; }
    পূর্ণসংখ্যা hi() ধ্রুবক VL_MT_STABLE {
        ফেরত dtypep()->width() - 1;
    }  // Packed classes look like arrays
    VNumRange declRange() ধ্রুবক VL_MT_STABLE { ফেরত VNumRange{hi(), lo()}; }
    Ast_গ্রন্থি_Module* classOrPackagep() ধ্রুবক { ফেরত m_classOrPackagep; }
    শূন্য classOrPackagep(Ast_গ্রন্থি_Module* classpackagep) { m_classOrPackagep = classpackagep; }
    bool isConstrainedRand() ধ্রুবক { ফেরত m_constrainedRand; }
    শূন্য markConstrainedRand(bool flag) { m_constrainedRand = flag; }
    bool emitToString() ধ্রুবক { ফেরত m_emitToString; }
    শূন্য setEmitToString() { m_emitToString = true; }
    bool isAggregateType() ধ্রুবক override { ফেরত !packed(); }
};

// === Concrete node types =====================================================

// === AstNode ===
class AstEnumItem final : সর্বজনীন Ast_গ্রন্থি {
    // @astgen op1 := rangep : Optional[AstRange] // Range for name appending
    // @astgen op2 := valuep : Optional[AstNodeExpr]
    string m_name;  // Name of item

সর্বজনীন:
    // Parents: ENUM
    AstEnumItem(File_সারি* fl, ধ্রুবক string& name, AstRange* rangep, Ast_গ্রন্থি_রাশিমালা* valuep)
        : ASTGEN_SUPER_EnumItem(fl)
        , m_name{name} {
        this->rangep(rangep);
        this->valuep(valuep);
    }
    ASTGEN_MEMBERS_AstEnumItem;
    string name() ধ্রুবক override VL_MT_STABLE { ফেরত m_name; }
    bool maybePointedTo() ধ্রুবক override VL_MT_SAFE { ফেরত true; }
    bool hasDType() ধ্রুবক override VL_MT_SAFE { ফেরত true; }
    শূন্য name(ধ্রুবক string& flag) override { m_name = flag; }
};

// === AstNodeDType ===
class AstAssocArrayDType final : সর্বজনীন Ast_গ্রন্থি_DType {
    // Associative array data type, ie "[some_dtype]"
    // @astgen op1 := childDTypep : Optional[AstNodeDType] // moved to refDTypep() in V3Width
    // @astgen op2 := keyChildDTypep : Optional[AstNodeDType]
    //
    // @astgen ptr := m_refDTypep : Optional[AstNodeDType]  // Elements of this type (post-width)
    // @astgen ptr := m_keyDTypep : Optional[AstNodeDType]  // Keys of this type (post-width)
সর্বজনীন:
    AstAssocArrayDType(File_সারি* fl, VFlagChildDType, Ast_গ্রন্থি_DType* dtp, Ast_গ্রন্থি_DType* চাবি_Dtp)
        : ASTGEN_SUPER_AssocArrayDType(fl) {
        childDTypep(dtp);  // Only for parser
        চাবি_Child_DTypep(চাবি_Dtp);  // Only for parser
        refDTypep(nullptr);
        চাবি_DTypep(nullptr);
        dtypep(nullptr);  // V3Width will resolve
    }
    AstAssocArrayDType(File_সারি* fl, Ast_গ্রন্থি_DType* dtp, Ast_গ্রন্থি_DType* চাবি_Dtp)
        : ASTGEN_SUPER_AssocArrayDType(fl) {
        refDTypep(dtp);
        চাবি_DTypep(চাবি_Dtp);
        dtypep(dtp);
    }
    ASTGEN_MEMBERS_AstAssocArrayDType;
    ধ্রুবক অক্ষর* broken() ধ্রুবক override {
        BROKEN_RTN(!((m_refDTypep && !childDTypep()) || (!m_refDTypep && childDTypep())));
        BROKEN_RTN(!((m_চাবি_DTypep && !childDTypep()) || (!m_চাবি_DTypep && childDTypep())));
        ফেরত nullptr;
    }
    bool same_গ্রন্থি(ধ্রুবক Ast_গ্রন্থি* samep) ধ্রুবক override {
        ধ্রুবক AstAssocArrayDType* ধ্রুবক asamep = VN_DBG_AS(samep, AssocArrayDType);
        যদি (!asamep->subDTypep()) ফেরত false;
        যদি (!asamep->চাবি_DTypep()) ফেরত false;
        ফেরত (subDTypep() == asamep->subDTypep() && চাবি_DTypep() == asamep->চাবি_DTypep());
    }
    bool similar_DType_গ্রন্থি(ধ্রুবক Ast_গ্রন্থি_DType* samep) ধ্রুবক override {
        ধ্রুবক AstAssocArrayDType* ধ্রুবক asamep = VN_DBG_AS(samep, AssocArrayDType);
        ফেরত asamep->subDTypep() && subDTypep()->similarDType(asamep->subDTypep())
               && asamep->চাবি_DTypep() && চাবি_DTypep()->similarDType(asamep->চাবি_DTypep());
    }
    string prettyDTypeName(bool full) ধ্রুবক override;
    শূন্য dumpSmall(std::ostream& str) ধ্রুবক override;
    Ast_গ্রন্থি_DType* getChildDTypep() ধ্রুবক override { ফেরত childDTypep(); }
    Ast_গ্রন্থি_DType* getChild2DTypep() ধ্রুবক override { ফেরত চাবি_Child_DTypep(); }
    Ast_গ্রন্থি_DType* subDTypep() ধ্রুবক override VL_MT_STABLE {
        ফেরত m_refDTypep ? m_refDTypep : childDTypep();
    }
    Ast_গ্রন্থি_DType* subDType2p() ধ্রুবক override VL_MT_STABLE {
        ফেরত m_চাবি_DTypep ? m_চাবি_DTypep : চাবি_Child_DTypep();
    }
    bool isAggregateType() ধ্রুবক override { ফেরত true; }
    শূন্য refDTypep(Ast_গ্রন্থি_DType* nodep) { m_refDTypep = nodep; }
    Ast_গ্রন্থি_DType* virtRefDTypep() ধ্রুবক override { ফেরত m_refDTypep; }
    শূন্য virtRefDTypep(Ast_গ্রন্থি_DType* nodep) override { refDTypep(nodep); }
    Ast_গ্রন্থি_DType* virtRefDType2p() ধ্রুবক override { ফেরত m_চাবি_DTypep; }
    শূন্য virtRefDType2p(Ast_গ্রন্থি_DType* nodep) override { চাবি_DTypep(nodep); }
    //
    Ast_গ্রন্থি_DType* চাবি_DTypep() ধ্রুবক VL_MT_STABLE {
        ফেরত m_চাবি_DTypep ? m_চাবি_DTypep : চাবি_Child_DTypep();
    }
    শূন্য চাবি_DTypep(Ast_গ্রন্থি_DType* nodep) { m_চাবি_DTypep = nodep; }
    // METHODS
    AstBasicDType* basicp() ধ্রুবক override VL_MT_STABLE { ফেরত nullptr; }
    পূর্ণসংখ্যা widthAlignBytes() ধ্রুবক override { ফেরত subDTypep()->widthAlignBytes(); }
    পূর্ণসংখ্যা width_যোগ_Bytes() ধ্রুবক override { ফেরত subDTypep()->width_যোগ_Bytes(); }
    bool isCompound() ধ্রুবক override { ফেরত true; }
    bool isDynamicallySized() ধ্রুবক override { ফেরত true; }
};
class AstBasicDType final : সর্বজনীন Ast_গ্রন্থি_DType {
    // Builtin atomic/vectored data type
    // @astgen op1 := rangep : Optional[AstRange] // Range of variable
    কাঠামো Members final {
        VBasicDTypeKwd m_keyword;  // (also in VBasicTypeKey) What keyword created basic type
        // dist-ast-dump-suppress  // Part of dumpSmall
        VNumRange m_nrange;  // (also in VBasicTypeKey) Numeric msb/lsb (if non-opaque keyword)
        bool operator==(ধ্রুবক Members& rhs) ধ্রুবক {
            ফেরত rhs.m_keyword == m_keyword && rhs.m_nrange == m_nrange;
        }
    } m;
    // See also in AstNodeDType: m_width, m_widthMin, m_numeric(issigned)
সর্বজনীন:
    AstBasicDType(File_সারি* fl, VBasicDTypeKwd kwd, ধ্রুবক VSigning& signst = VSigning::NOSIGN)
        : ASTGEN_SUPER_BasicDType(fl) {
        init(kwd, signst, 0, -1, nullptr);
    }
    AstBasicDType(File_সারি* fl, VFlagLogicPacked, পূর্ণসংখ্যা wantwidth)
        : ASTGEN_SUPER_BasicDType(fl) {
        init(VBasicDTypeKwd::LOGIC, VSigning::NOSIGN, wantwidth, -1, nullptr);
    }
    AstBasicDType(File_সারি* fl, VFlagLogic2StatePacked, পূর্ণসংখ্যা wantwidth)
        : ASTGEN_SUPER_BasicDType(fl) {
        init(VBasicDTypeKwd::LOGIC2STATE, VSigning::NOSIGN, wantwidth, -1, nullptr);
    }
    AstBasicDType(File_সারি* fl, VFlagBitPacked, পূর্ণসংখ্যা wantwidth)
        : ASTGEN_SUPER_BasicDType(fl) {
        init(VBasicDTypeKwd::BIT, VSigning::NOSIGN, wantwidth, -1, nullptr);
    }
    AstBasicDType(File_সারি* fl, VBasicDTypeKwd kwd, VSigning numer, পূর্ণসংখ্যা wantwidth, পূর্ণসংখ্যা widthmin)
        : ASTGEN_SUPER_BasicDType(fl) {
        init(kwd, numer, wantwidth, widthmin, nullptr);
    }
    AstBasicDType(File_সারি* fl, VBasicDTypeKwd kwd, VSigning numer, VNumRange range, পূর্ণসংখ্যা widthmin)
        : ASTGEN_SUPER_BasicDType(fl) {
        init(kwd, numer, range.elements(), widthmin, nullptr);
        m.m_nrange = range;  // as init() presumes lsb==0, but range.lsb() might not be
    }
    // See also addRange in verilog.y
ব্যক্তিগত:
    শূন্য init(VBasicDTypeKwd kwd, VSigning numer, পূর্ণসংখ্যা wantwidth, পূর্ণসংখ্যা wantwidthmin,
              AstRange* rangep);

সর্বজনীন:
    ASTGEN_MEMBERS_AstBasicDType;
    শূন্য dump(std::ostream& str) ধ্রুবক override;
    শূন্য dumpJson(std::ostream& str) ধ্রুবক override;
    // width/widthMin/numeric compared elsewhere
    bool same_গ্রন্থি(ধ্রুবক Ast_গ্রন্থি* samep) ধ্রুবক override;
    bool similar_DType_গ্রন্থি(ধ্রুবক Ast_গ্রন্থি_DType* samep) ধ্রুবক override;
    string name() ধ্রুবক override VL_MT_STABLE { ফেরত m.m_keyword.typeName(); }
    string prettyDTypeName(bool full) ধ্রুবক override;
    ধ্রুবক অক্ষর* broken() ধ্রুবক override {
        BROKEN_RTN(dtypep() != this);
        BROKEN_RTN(v3Global.widthMinUsage() == VWidthMinUsage::VERILOG_WIDTH
                   && widthMin() > width());
        ফেরত nullptr;
    }
    শূন্য setSignedState(ধ্রুবক VSigning& signst) {
        // Note NOSIGN does NOT change the state; this is required by the parser
        যদি (signst == VSigning::UNSIGNED) {
            numeric(signst);
        } অন্যথা যদি (signst == VSigning::SIGNED) {
            numeric(signst);
        }
    }
    // METHODS
    AstBasicDType* basicp() ধ্রুবক override VL_MT_STABLE { ফেরত (AstBasicDType*)this; }
    // (Slow) recurses - Structure alignment 1,2,4 or 8 bytes (arrays affect this)
    পূর্ণসংখ্যা widthAlignBytes() ধ্রুবক override;
    // (Slow) recurses - Width in bytes rounding up 1,2,4,8,12,...
    পূর্ণসংখ্যা width_যোগ_Bytes() ধ্রুবক override;
    bool isFourstate() ধ্রুবক override { ফেরত keyword().isFourstate(); }
    VBasicDTypeKwd keyword() ধ্রুবক VL_MT_SAFE {  // Avoid using - use isSomething accessors instead
        ফেরত m.m_keyword;
    }
    bool isBitLogic() ধ্রুবক { ফেরত keyword().isBitLogic(); }
    bool isBit() ধ্রুবক { ফেরত keyword().isBit(); }
    bool isCHandle() ধ্রুবক VL_MT_STABLE { ফেরত keyword().isCHandle(); }
    bool isDouble() ধ্রুবক VL_MT_STABLE { ফেরত keyword().isDouble(); }
    bool isEvent() ধ্রুবক VL_MT_STABLE { ফেরত keyword() == VBasicDTypeKwd::EVENT; }
    bool isForkSync() ধ্রুবক VL_MT_SAFE { ফেরত keyword() == VBasicDTypeKwd::FORK_SYNC; }
    bool isProcessRef() ধ্রুবক VL_MT_SAFE { ফেরত keyword() == VBasicDTypeKwd::PROCESS_REFERENCE; }
    bool isDelayScheduler() ধ্রুবক VL_MT_SAFE {
        ফেরত keyword() == VBasicDTypeKwd::DELAY_SCHEDULER;
    }
    bool isTriggerScheduler() ধ্রুবক VL_MT_SAFE {
        ফেরত keyword() == VBasicDTypeKwd::TRIGGER_SCHEDULER;
    }
    bool isDynamicTriggerScheduler() ধ্রুবক VL_MT_SAFE {
        ফেরত keyword() == VBasicDTypeKwd::DYNAMIC_TRIGGER_SCHEDULER;
    }
    bool isRandomGenerator() ধ্রুবক VL_MT_SAFE {
        ফেরত keyword() == VBasicDTypeKwd::RANDOM_GENERATOR;
    }
    bool isStdRandomGenerator() ধ্রুবক VL_MT_SAFE {
        ফেরত keyword() == VBasicDTypeKwd::RANDOM_STDGENERATOR;
    }
    bool isCovergroupInstHandle() ধ্রুবক VL_MT_SAFE {
        ফেরত keyword() == VBasicDTypeKwd::COVERGROUP_INSTHANDLE;
    }
    bool isOpaque() ধ্রুবক VL_MT_SAFE { ফেরত keyword().isOpaque(); }
    bool isString() ধ্রুবক VL_MT_STABLE { ফেরত keyword().isString(); }
    bool isZeroInit() ধ্রুবক { ফেরত keyword().isZeroInit(); }
    bool isRanged() ধ্রুবক { ফেরত rangep() || m.m_nrange.ranged(); }
    bool isDpiBitVec() ধ্রুবক {  // DPI uses svBitVecVal
        ফেরত keyword() == VBasicDTypeKwd::BIT && isRanged();
    }
    bool isDpiLogicVec() ধ্রুবক {  // DPI uses svLogicVecVal
        ফেরত keyword().isFourstate() && !(keyword() == VBasicDTypeKwd::LOGIC && !isRanged());
    }
    bool isDpiCLayout() ধ্রুবক {
        ফেরত keyword().isDpiCLayout()
               || (keyword().isSameish(VBasicDTypeKwd::LOGIC) && !isRanged());
    }
    bool isDpiPrimitive() ধ্রুবক {  // DPI uses a primitive type
        ফেরত !isDpiBitVec() && !isDpiLogicVec();
    }
    // Generally the lo/hi/left/right funcs should be used instead of nrange()
    ধ্রুবক VNumRange& nrange() ধ্রুবক { ফেরত m.m_nrange; }
    অন্তঃরেখ পূর্ণসংখ্যা hi() ধ্রুবক;
    অন্তঃরেখ পূর্ণসংখ্যা lo() ধ্রুবক;
    অন্তঃরেখ পূর্ণসংখ্যা elements() ধ্রুবক;
    পূর্ণসংখ্যা বাম() ধ্রুবক { ফেরত ascending() ? lo() : hi(); }  // How to show a declaration
    পূর্ণসংখ্যা ডান() ধ্রুবক { ফেরত ascending() ? hi() : lo(); }
    অন্তঃরেখ bool ascending() ধ্রুবক;
    bool implicit() ধ্রুবক { ফেরত keyword() == VBasicDTypeKwd::LOGIC_IMPLICIT; }
    bool untyped() ধ্রুবক { ফেরত keyword() == VBasicDTypeKwd::UNTYPED; }
    VNumRange declRange() ধ্রুবক { ফেরত isRanged() ? VNumRange{বাম(), ডান()} : VNumRange{}; }
    শূন্য cvtRangeConst();  // Convert to smaller representation
    bool isCompound() ধ্রুবক override { ফেরত isString(); }
    bool isIntegralOrPacked() ধ্রুবক override { ফেরত keyword().isIntNumeric(); }
};
class AstBracketArrayDType final : সর্বজনীন Ast_গ্রন্থি_DType {
    // Associative/Queue/Normal array data type, ie "[dtype_or_expr]"
    // only for early parsing then becomes another data type
    // @astgen op1 := childDTypep : Optional[AstNodeDType] // moved to refDTypep() in V3Width
    // @astgen op2 := elementsp : AstNode  // Number of elements in array
সর্বজনীন:
    AstBracketArrayDType(File_সারি* fl, VFlagChildDType, Ast_গ্রন্থি_DType* childDTypep,
                         Ast_গ্রন্থি* elementsp)
        : ASTGEN_SUPER_BracketArrayDType(fl) {
        this->childDTypep(childDTypep);
        this->elementsp(elementsp);
    }
    ASTGEN_MEMBERS_AstBracketArrayDType;
    bool similar_DType_গ্রন্থি(ধ্রুবক Ast_গ্রন্থি_DType* samep) ধ্রুবক override { ফেরত this == samep; }
    Ast_গ্রন্থি_DType* subDTypep() ধ্রুবক override VL_MT_STABLE { ফেরত childDTypep(); }
    // METHODS
    // Will be removed in V3Width, which relies on this
    // being a child not a dtype pointed node
    bool maybePointedTo() ধ্রুবক override VL_MT_SAFE { ফেরত false; }
    AstBasicDType* basicp() ধ্রুবক override VL_MT_STABLE { ফেরত nullptr; }
    পূর্ণসংখ্যা widthAlignBytes() ধ্রুবক override { V3ERROR_NA_RETURN(0); }
    পূর্ণসংখ্যা width_যোগ_Bytes() ধ্রুবক override { V3ERROR_NA_RETURN(0); }
    bool isCompound() ধ্রুবক override { ফেরত true; }
};
class AstCDType final : সর্বজনীন Ast_গ্রন্থি_DType {
    // Raw "C" data type passed directly to output
    string m_name;  // Name of data type, printed when do V3EmitC
সর্বজনীন:
    AstCDType(File_সারি* fl, ধ্রুবক string& name)
        : ASTGEN_SUPER_CDType(fl)
        , m_name{name} {
        dtypep(this);
    }

    ASTGEN_MEMBERS_AstCDType;
    bool same_গ্রন্থি(ধ্রুবক Ast_গ্রন্থি* samep) ধ্রুবক override {
        ধ্রুবক AstCDType* ধ্রুবক asamep = VN_DBG_AS(samep, CDType);
        ফেরত m_name == asamep->m_name;
    }
    bool similar_DType_গ্রন্থি(ধ্রুবক Ast_গ্রন্থি_DType* samep) ধ্রুবক override { ফেরত same_গ্রন্থি(samep); }
    string name() ধ্রুবক override VL_MT_STABLE { ফেরত m_name; }
    string prettyDTypeName(bool) ধ্রুবক override { ফেরত m_name; }
    // METHODS
    AstBasicDType* basicp() ধ্রুবক override VL_MT_STABLE { ফেরত nullptr; }
    পূর্ণসংখ্যা widthAlignBytes() ধ্রুবক override { ফেরত 8; }  // Assume
    পূর্ণসংখ্যা width_যোগ_Bytes() ধ্রুবক override { ফেরত 8; }  // Assume
    bool isCompound() ধ্রুবক override { ফেরত true; }
    // Return the global runtime type storing the requested number of bits.
    স্থির string typeToHold(পূর্ণসংখ্যা width) {
        যদি (width <= 8) ফেরত "::CData";
        যদি (width <= 16) ফেরত "::SData";
        যদি (width <= VL_IDATASIZE) ফেরত "::IData";
        যদি (width <= VL_QUADSIZE) ফেরত "::QData";

        ফেরত "::VlWide<" + std::to_string(VL_শব্দগুলি_I(width)) + ">";
    }
};
class AstClassRefDType final : সর্বজনীন Ast_গ্রন্থি_DType {
    // Reference to a class
    // @astgen op1 := paramsp: List[AstPin]
    //
    // @astgen ptr := m_classp : Optional[AstClass]  // data type pointed to, BELOW the AstTypedef
    // @astgen ptr := m_classOrPackagep : Optional[AstNodeModule]  // Package hierarchy
    bool m_rawPointer = false;  // Emit as a non-owning C++ pointer rather than VlClassRef
সর্বজনীন:
    AstClassRefDType(File_সারি* fl, AstClass* classp, AstPin* paramsp)
        : ASTGEN_SUPER_ClassRefDType(fl)
        , m_classp{classp} {
        dtypep(this);
        addParamsp(paramsp);
    }
    ASTGEN_MEMBERS_AstClassRefDType;
    // METHODS
    bool same_গ্রন্থি(ধ্রুবক Ast_গ্রন্থি* samep) ধ্রুবক override {
        ধ্রুবক AstClassRefDType* ধ্রুবক asamep = VN_DBG_AS(samep, ClassRefDType);
        ফেরত (m_classp == asamep->m_classp && m_classOrPackagep == asamep->m_classOrPackagep
                && m_rawPointer == asamep->m_rawPointer);
    }
    bool similar_DType_গ্রন্থি(ধ্রুবক Ast_গ্রন্থি_DType* samep) ধ্রুবক override;
    শূন্য dump(std::ostream& str = std::cout) ধ্রুবক override;
    শূন্য dumpJson(std::ostream& str = std::cout) ধ্রুবক override;
    শূন্য dumpSmall(std::ostream& str) ধ্রুবক override;
    string prettyDTypeName(bool full) ধ্রুবক override;
    string pretty_Name_বার্তা() ধ্রুবক override;
    string name() ধ্রুবক override VL_MT_STABLE;
    AstBasicDType* basicp() ধ্রুবক override VL_MT_STABLE { ফেরত nullptr; }
    পূর্ণসংখ্যা widthAlignBytes() ধ্রুবক override { ফেরত 0; }
    পূর্ণসংখ্যা width_যোগ_Bytes() ধ্রুবক override { ফেরত 0; }
    Ast_গ্রন্থি_DType* virtRefDTypep() ধ্রুবক override { ফেরত nullptr; }
    শূন্য virtRefDTypep(Ast_গ্রন্থি_DType* nodep) override {}
    Ast_গ্রন্থি_DType* subDTypep() ধ্রুবক override VL_MT_STABLE { ফেরত nullptr; }
    Ast_গ্রন্থি_Module* classOrPackagep() ধ্রুবক { ফেরত m_classOrPackagep; }
    শূন্য classOrPackagep(Ast_গ্রন্থি_Module* nodep) { m_classOrPackagep = nodep; }
    AstClass* classp() ধ্রুবক VL_MT_STABLE { ফেরত m_classp; }
    শূন্য classp(AstClass* nodep) { m_classp = nodep; }
    bool rawPointer() ধ্রুবক { ফেরত m_rawPointer; }
    শূন্য rawPointer(bool flag) { m_rawPointer = flag; }
    স্থির শূন্য selfTest();
    bool isCompound() ধ্রুবক override { ফেরত true; }
};
class AstConstDType final : সর্বজনীন Ast_গ্রন্থি_DType {
    // const data type, ie "const some_dtype var_name [2:0]"
    // ConstDType are removed in V3LinkLValue and become AstVar::isConst.
    // When more generic types are supported AstConstDType will be propagated further.
    // @astgen op1 := childDTypep : Optional[AstNodeDType]
    //
    // @astgen ptr := m_refDTypep : Optional[AstNodeDType]  // Inherit from this base data type
সর্বজনীন:
    AstConstDType(File_সারি* fl, VFlagChildDType, Ast_গ্রন্থি_DType* dtp)
        : ASTGEN_SUPER_ConstDType(fl) {
        childDTypep(dtp);  // Only for parser
        refDTypep(nullptr);  // V3Width will resolve
        dtypep(nullptr);  // V3Width will resolve
        widthFromSub(subDTypep());
    }
    ASTGEN_MEMBERS_AstConstDType;
    ধ্রুবক অক্ষর* broken() ধ্রুবক override {
        BROKEN_RTN(!((m_refDTypep && !childDTypep()) || (!m_refDTypep && childDTypep())));
        ফেরত nullptr;
    }
    bool same_গ্রন্থি(ধ্রুবক Ast_গ্রন্থি* samep) ধ্রুবক override {
        ধ্রুবক AstConstDType* ধ্রুবক sp = VN_DBG_AS(samep, ConstDType);
        ফেরত (m_refDTypep == sp->m_refDTypep);
    }
    bool similar_DType_গ্রন্থি(ধ্রুবক Ast_গ্রন্থি_DType* samep) ধ্রুবক override {
        ফেরত skipRefp()->similarDType(samep->skipRefp());
    }
    Ast_গ্রন্থি_DType* getChildDTypep() ধ্রুবক override { ফেরত childDTypep(); }
    Ast_গ্রন্থি_DType* subDTypep() ধ্রুবক override VL_MT_STABLE {
        ফেরত m_refDTypep ? m_refDTypep : childDTypep();
    }
    শূন্য refDTypep(Ast_গ্রন্থি_DType* nodep) { m_refDTypep = nodep; }
    Ast_গ্রন্থি_DType* virtRefDTypep() ধ্রুবক override { ফেরত m_refDTypep; }
    শূন্য virtRefDTypep(Ast_গ্রন্থি_DType* nodep) override { refDTypep(nodep); }
    // METHODS
    AstBasicDType* basicp() ধ্রুবক override VL_MT_STABLE { ফেরত subDTypep()->basicp(); }
    পূর্ণসংখ্যা widthAlignBytes() ধ্রুবক override { ফেরত subDTypep()->widthAlignBytes(); }
    পূর্ণসংখ্যা width_যোগ_Bytes() ধ্রুবক override { ফেরত subDTypep()->width_যোগ_Bytes(); }
    bool isCompound() ধ্রুবক override {
        v3fatalSrc("call isCompound on subdata type, not reference");
        ফেরত false;
    }
};
class AstConstraintRefDType final : সর্বজনীন Ast_গ্রন্থি_DType {
    // For e.g. a reference to constraint for constraint_mode
সর্বজনীন:
    explicit AstConstraintRefDType(File_সারি* fl)
        : ASTGEN_SUPER_ConstraintRefDType(fl) {
        dtypep(this);
    }
    ASTGEN_MEMBERS_AstConstraintRefDType;
    bool hasDType() ধ্রুবক override VL_MT_SAFE { ফেরত true; }
    bool maybePointedTo() ধ্রুবক override VL_MT_SAFE { ফেরত true; }
    bool undead() ধ্রুবক override { ফেরত true; }
    Ast_গ্রন্থি_DType* subDTypep() ধ্রুবক override VL_MT_STABLE { ফেরত nullptr; }
    Ast_গ্রন্থি_DType* virtRefDTypep() ধ্রুবক override { ফেরত nullptr; }
    শূন্য virtRefDTypep(Ast_গ্রন্থি_DType* nodep) override {}
    bool similar_DType_গ্রন্থি(ধ্রুবক Ast_গ্রন্থি_DType* samep) ধ্রুবক override { ফেরত this == samep; }
    AstBasicDType* basicp() ধ্রুবক override VL_MT_STABLE { ফেরত nullptr; }
    পূর্ণসংখ্যা widthAlignBytes() ধ্রুবক override { ফেরত 1; }
    পূর্ণসংখ্যা width_যোগ_Bytes() ধ্রুবক override { ফেরত 1; }
    bool isCompound() ধ্রুবক override { ফেরত false; }
};
class AstCoverCrossDType final : সর্বজনীন Ast_গ্রন্থি_DType {
    // Borrowed pointer to VlCoverCrossT<...> or construction-time VlCoverCrossDyn.
    ধ্রুবক uint32_t m_dimensions;
    ধ্রুবক uint32_t m_tuples;  // Fixed capacity number of cross tuples (VlCoverCrossT's Tuples)
    ধ্রুবক uint32_t m_bins;  // Fixed capacity number of explicit bins (VlCoverCrossT's Bins)
    ধ্রুবক uint32_t m_autoBins;  // Fixed capacity number of auto bins (VlCoverCrossT's AutoBins)
    ধ্রুবক uint64_t m_bin_শব্দগুলি;  // Fixed capacity selection words (VlCoverCrossT's BinWords)
    ধ্রুবক bool m_dynamic;  // Bin layout is determined at covergroup construction

সর্বজনীন:
    AstCoverCrossDType(File_সারি* fl, uint32_t dimensions, uint32_t tuples, uint32_t bins,
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
    ধ্রুবক অক্ষর* broken() ধ্রুবক override {
        BROKEN_RTN(m_dimensions == 0);
        ফেরত nullptr;
    }
    bool same_গ্রন্থি(ধ্রুবক Ast_গ্রন্থি* samep) ধ্রুবক override {  // LCOV_EXCL_START
        ধ্রুবক AstCoverCrossDType* ধ্রুবক sp = VN_DBG_AS(samep, CoverCrossDType);
        ফেরত dimensions() == sp->dimensions() && tuples() == sp->tuples() && bins() == sp->bins()
               && autoBins() == sp->autoBins() && bin_শব্দগুলি() == sp->bin_শব্দগুলি()
               && isDynamic() == sp->isDynamic();
    }  // LCOV_EXCL_STOP
    bool similar_DType_গ্রন্থি(ধ্রুবক Ast_গ্রন্থি_DType* samep) ধ্রুবক override { ফেরত this == samep; }
    শূন্য dump(std::ostream& str) ধ্রুবক override;
    শূন্য dumpJson(std::ostream& str) ধ্রুবক override;
    শূন্য dumpSmall(std::ostream& str) ধ্রুবক override;
    uint32_t dimensions() ধ্রুবক { ফেরত m_dimensions; }
    uint32_t tuples() ধ্রুবক { ফেরত m_tuples; }
    uint32_t bins() ধ্রুবক { ফেরত m_bins; }
    uint32_t autoBins() ধ্রুবক { ফেরত m_autoBins; }
    uint64_t bin_শব্দগুলি() ধ্রুবক { ফেরত m_bin_শব্দগুলি; }
    bool isDynamic() ধ্রুবক { ফেরত m_dynamic; }
    string cppTemplateArgs() ধ্রুবক;
    AstBasicDType* basicp() ধ্রুবক override VL_MT_STABLE { ফেরত nullptr; }
    পূর্ণসংখ্যা widthAlignBytes() ধ্রুবক override { ফেরত আকার(শূন্য*); }
    পূর্ণসংখ্যা width_যোগ_Bytes() ধ্রুবক override { ফেরত আকার(শূন্য*); }
    bool isCompound() ধ্রুবক override { ফেরত true; }
};
class AstCoverpointDType final : সর্বজনীন Ast_গ্রন্থি_DType {
    // Borrowed pointer to a covergroup coverpoint runtime, 'VlCoverpointT<hitBound>*'.
    // Follows pattern of AstQueueDType in capturing the compile-time max bin overlap
    // template argument.
    uint32_t m_hitBound;  // VlCoverpointT<> template argument; hit list size, >= 1
সর্বজনীন:
    AstCoverpointDType(File_সারি* fl, uint32_t hitBound)
        : ASTGEN_SUPER_CoverpointDType(fl)
        , m_hitBound{hitBound} {
        dtypep(this);
    }
    ASTGEN_MEMBERS_AstCoverpointDType;
    ধ্রুবক অক্ষর* broken() ধ্রুবক override {
        BROKEN_RTN(m_hitBound < 1);
        ফেরত nullptr;
    }
    // V3Covergroup interns these one-per-hitBound into the type table, so identity is
    // equality; there is never a second node with the same bound to compare against.
    bool similar_DType_গ্রন্থি(ধ্রুবক Ast_গ্রন্থি_DType* samep) ধ্রুবক override { ফেরত this == samep; }
    শূন্য dumpSmall(std::ostream& str) ধ্রুবক override;
    // ACCESSORS
    uint32_t hitBound() ধ্রুবক { ফেরত m_hitBound; }
    // METHODS
    AstBasicDType* basicp() ধ্রুবক override VL_MT_STABLE { ফেরত nullptr; }
    পূর্ণসংখ্যা widthAlignBytes() ধ্রুবক override { ফেরত আকার(শূন্য*); }
    পূর্ণসংখ্যা width_যোগ_Bytes() ধ্রুবক override { ফেরত আকার(শূন্য*); }
    bool isCompound() ধ্রুবক override { ফেরত true; }
};
class AstDefImplicitDType final : সর্বজনীন Ast_গ্রন্থি_DType {
    // For parsing enum/struct/unions that are declared with a variable rather than typedef
    // This allows "var enum {...} a,b" to share the enum definition for both variables
    // After link, these become typedefs
    // @astgen op1 := childDTypep : Optional[AstNodeDType]
    string m_name;  // Data type name
    ধ্রুবক পূর্ণসংখ্যা m_unique_অঙ্ক;  // Unique ID distinguishing this dtype instance, for hashing/naming

সর্বজনীন:
    AstDefImplicitDType(File_সারি* fl, ধ্রুবক string& name, VFlagChildDType, Ast_গ্রন্থি_DType* dtp)
        : ASTGEN_SUPER_DefImplicitDType(fl)
        , m_name{name}
        , m_unique_অঙ্ক{unique_অঙ্ক_Inc()} {
        childDTypep(dtp);  // Only for parser
        dtypep(nullptr);  // V3Width will resolve
    }
    AstDefImplicitDType(ধ্রুবক AstDefImplicitDType& other)
        : Ast_গ্রন্থি_DType(other)
        , m_name{other.m_name}
        , m_unique_অঙ্ক{unique_অঙ্ক_Inc()} {}
    ASTGEN_MEMBERS_AstDefImplicitDType;
    পূর্ণসংখ্যা unique_অঙ্ক() ধ্রুবক { ফেরত m_unique_অঙ্ক; }
    bool same_গ্রন্থি(ধ্রুবক Ast_গ্রন্থি* samep) ধ্রুবক override {
        ধ্রুবক AstDefImplicitDType* ধ্রুবক sp = VN_DBG_AS(samep, DefImplicitDType);
        ফেরত unique_অঙ্ক() == sp->unique_অঙ্ক();
    }
    bool similar_DType_গ্রন্থি(ধ্রুবক Ast_গ্রন্থি_DType* samep) ধ্রুবক override { ফেরত same_গ্রন্থি(samep); }
    Ast_গ্রন্থি_DType* getChildDTypep() ধ্রুবক override { ফেরত childDTypep(); }
    Ast_গ্রন্থি_DType* subDTypep() ধ্রুবক override VL_MT_STABLE {
        ফেরত dtypep() ? dtypep() : childDTypep();
    }
    // METHODS
    // op1 = Range of variable
    Ast_গ্রন্থি_DType* dtypeSkipRefp() ধ্রুবক { ফেরত dtypep()->skipRefp(); }
    AstBasicDType* basicp() ধ্রুবক override VL_MT_STABLE { ফেরত subDTypep()->basicp(); }
    পূর্ণসংখ্যা widthAlignBytes() ধ্রুবক override { ফেরত dtypep()->widthAlignBytes(); }
    পূর্ণসংখ্যা width_যোগ_Bytes() ধ্রুবক override { ফেরত dtypep()->width_যোগ_Bytes(); }
    string name() ধ্রুবক override VL_MT_STABLE { ফেরত m_name; }
    শূন্য name(ধ্রুবক string& flag) override { m_name = flag; }
    bool isCompound() ধ্রুবক override { ফেরত false; }
};
class AstDynArrayDType final : সর্বজনীন Ast_গ্রন্থি_DType {
    // Dynamic array data type, ie "[]"
    // @astgen op1 := childDTypep : Optional[AstNodeDType] // moved to refDTypep() in V3Width
    //
    // @astgen ptr := m_refDTypep : Optional[AstNodeDType]  // Elements of this type (post-width)
সর্বজনীন:
    AstDynArrayDType(File_সারি* fl, VFlagChildDType, Ast_গ্রন্থি_DType* dtp)
        : ASTGEN_SUPER_DynArrayDType(fl) {
        childDTypep(dtp);  // Only for parser
        refDTypep(nullptr);
        dtypep(nullptr);  // V3Width will resolve
    }
    AstDynArrayDType(File_সারি* fl, Ast_গ্রন্থি_DType* dtp)
        : ASTGEN_SUPER_DynArrayDType(fl) {
        refDTypep(dtp);
        dtypep(nullptr);  // V3Width will resolve
    }
    ASTGEN_MEMBERS_AstDynArrayDType;
    ধ্রুবক অক্ষর* broken() ধ্রুবক override {
        BROKEN_RTN(!((m_refDTypep && !childDTypep()) || (!m_refDTypep && childDTypep())));
        ফেরত nullptr;
    }
    bool same_গ্রন্থি(ধ্রুবক Ast_গ্রন্থি* samep) ধ্রুবক override {
        ধ্রুবক AstDynArrayDType* ধ্রুবক asamep = VN_DBG_AS(samep, DynArrayDType);
        যদি (!asamep->subDTypep()) ফেরত false;
        ফেরত subDTypep() == asamep->subDTypep();
    }
    bool similar_DType_গ্রন্থি(ধ্রুবক Ast_গ্রন্থি_DType* samep) ধ্রুবক override {
        ধ্রুবক AstDynArrayDType* ধ্রুবক asamep = VN_DBG_AS(samep, DynArrayDType);
        ফেরত asamep->subDTypep() && subDTypep()->similarDType(asamep->subDTypep());
    }
    string prettyDTypeName(bool full) ধ্রুবক override;
    শূন্য dumpSmall(std::ostream& str) ধ্রুবক override;
    Ast_গ্রন্থি_DType* getChildDTypep() ধ্রুবক override { ফেরত childDTypep(); }
    Ast_গ্রন্থি_DType* subDTypep() ধ্রুবক override VL_MT_STABLE {
        ফেরত m_refDTypep ? m_refDTypep : childDTypep();
    }
    bool isAggregateType() ধ্রুবক override { ফেরত true; }
    শূন্য refDTypep(Ast_গ্রন্থি_DType* nodep) { m_refDTypep = nodep; }
    Ast_গ্রন্থি_DType* virtRefDTypep() ধ্রুবক override { ফেরত m_refDTypep; }
    শূন্য virtRefDTypep(Ast_গ্রন্থি_DType* nodep) override { refDTypep(nodep); }
    // METHODS
    AstBasicDType* basicp() ধ্রুবক override VL_MT_STABLE { ফেরত nullptr; }
    পূর্ণসংখ্যা widthAlignBytes() ধ্রুবক override { ফেরত subDTypep()->widthAlignBytes(); }
    পূর্ণসংখ্যা width_যোগ_Bytes() ধ্রুবক override { ফেরত subDTypep()->width_যোগ_Bytes(); }
    bool isCompound() ধ্রুবক override { ফেরত true; }
    bool isDynamicallySized() ধ্রুবক override { ফেরত true; }
};
class AstEmptyQueueDType final : সর্বজনীন Ast_গ্রন্থি_DType {
    // For EmptyQueue
সর্বজনীন:
    explicit AstEmptyQueueDType(File_সারি* fl)
        : ASTGEN_SUPER_EmptyQueueDType(fl) {
        dtypep(this);
    }
    ASTGEN_MEMBERS_AstEmptyQueueDType;
    শূন্য dumpSmall(std::ostream& str) ধ্রুবক override;
    bool hasDType() ধ্রুবক override VL_MT_SAFE { ফেরত true; }
    bool maybePointedTo() ধ্রুবক override VL_MT_SAFE { ফেরত true; }
    bool undead() ধ্রুবক override { ফেরত true; }
    Ast_গ্রন্থি_DType* subDTypep() ধ্রুবক override VL_MT_STABLE { ফেরত nullptr; }
    Ast_গ্রন্থি_DType* virtRefDTypep() ধ্রুবক override { ফেরত nullptr; }
    শূন্য virtRefDTypep(Ast_গ্রন্থি_DType* nodep) override {}
    bool similar_DType_গ্রন্থি(ধ্রুবক Ast_গ্রন্থি_DType* samep) ধ্রুবক override { ফেরত this == samep; }
    AstBasicDType* basicp() ধ্রুবক override VL_MT_STABLE { ফেরত nullptr; }
    পূর্ণসংখ্যা widthAlignBytes() ধ্রুবক override { ফেরত 1; }
    পূর্ণসংখ্যা width_যোগ_Bytes() ধ্রুবক override { ফেরত 1; }
    bool isCompound() ধ্রুবক override { ফেরত false; }
};
class AstEnumDType final : সর্বজনীন Ast_গ্রন্থি_DType {
    // Parents: TYPEDEF/MODULE
    // @astgen op1 := childDTypep : Optional[AstNodeDType]
    // @astgen op2 := itemsp : List[AstEnumItem]
    //
    // @astgen ptr := m_refDTypep : Optional[AstNodeDType]  // Elements of this type (post-width)
    string m_name;  // Name from upper typedef, if any
    string m_typedefName;  // Typedef's dtypeName() once moved off the typedef declaring it
    ধ্রুবক পূর্ণসংখ্যা m_unique_অঙ্ক;  // Unique ID distinguishing this dtype instance, for hashing/naming

সর্বজনীন:
    AstEnumDType(File_সারি* fl, VFlagChildDType, Ast_গ্রন্থি_DType* dtp, AstEnumItem* itemsp)
        : ASTGEN_SUPER_EnumDType(fl)
        , m_unique_অঙ্ক{unique_অঙ্ক_Inc()} {
        childDTypep(dtp);  // Only for parser
        refDTypep(nullptr);
        addItemsp(itemsp);
        dtypep(nullptr);  // V3Width will resolve
        widthFromSub(subDTypep());
    }
    AstEnumDType(ধ্রুবক AstEnumDType& other)
        : Ast_গ্রন্থি_DType{other}
        , m_name{other.m_name}
        , m_typedefName{other.m_typedefName}
        , m_unique_অঙ্ক{unique_অঙ্ক_Inc()} {}
    ASTGEN_MEMBERS_AstEnumDType;

    ধ্রুবক অক্ষর* broken() ধ্রুবক override;
    পূর্ণসংখ্যা unique_অঙ্ক() ধ্রুবক { ফেরত m_unique_অঙ্ক; }
    bool same_গ্রন্থি(ধ্রুবক Ast_গ্রন্থি* samep) ধ্রুবক override {
        ধ্রুবক AstEnumDType* ধ্রুবক sp = VN_DBG_AS(samep, EnumDType);
        ফেরত unique_অঙ্ক() == sp->unique_অঙ্ক();
    }
    bool similar_DType_গ্রন্থি(ধ্রুবক Ast_গ্রন্থি_DType* samep) ধ্রুবক override { ফেরত same_গ্রন্থি(samep); }
    Ast_গ্রন্থি_DType* getChildDTypep() ধ্রুবক override { ফেরত childDTypep(); }
    Ast_গ্রন্থি_DType* subDTypep() ধ্রুবক override VL_MT_STABLE {
        ফেরত m_refDTypep ? m_refDTypep : childDTypep();
    }
    শূন্য refDTypep(Ast_গ্রন্থি_DType* nodep) { m_refDTypep = nodep; }
    Ast_গ্রন্থি_DType* virtRefDTypep() ধ্রুবক override { ফেরত m_refDTypep; }
    শূন্য virtRefDTypep(Ast_গ্রন্থি_DType* nodep) override { refDTypep(nodep); }
    string name() ধ্রুবক override VL_MT_STABLE { ফেরত m_name; }
    শূন্য name(ধ্রুবক string& flag) override { m_name = flag; }
    // Keep the name the typedef declaring it gives, as it moves off it to the type table
    শূন্য typedefName(ধ্রুবক string& name) { m_typedefName = name; }
    শূন্য dump(std::ostream& str = std::cout) ধ্রুবক override;
    শূন্য dumpJson(std::ostream& str = std::cout) ধ্রুবক override;
    শূন্য dumpSmall(std::ostream& str) ধ্রুবক override;
    string prettyDTypeName(bool full) ধ্রুবক override;
    // METHODS
    AstBasicDType* basicp() ধ্রুবক override VL_MT_STABLE { ফেরত subDTypep()->basicp(); }
    পূর্ণসংখ্যা widthAlignBytes() ধ্রুবক override { ফেরত subDTypep()->widthAlignBytes(); }
    পূর্ণসংখ্যা width_যোগ_Bytes() ধ্রুবক override { ফেরত subDTypep()->width_যোগ_Bytes(); }
    আকার_t item_গুনতি() ধ্রুবক {
        আকার_t গুনতি = 0;
        জন্য (Ast_গ্রন্থি* itemp = itemsp(); itemp; itemp = itemp->nextp()) ++গুনতি;
        ফেরত গুনতি;
    }
    bool isCompound() ধ্রুবক override { ফেরত false; }
};

class AstIfaceGenericDType final : সর্বজনীন Ast_গ্রন্থি_DType {
    // Generic interface that will be replaced with AstIfaceRefDType
    File_সারি* m_modportFileline;  // Where modport token was
    // dist-ast-dump-suppress  // Part of name()
    string m_modportName;  // "" = no modport
সর্বজনীন:
    explicit AstIfaceGenericDType(File_সারি* fl)
        : ASTGEN_SUPER_IfaceGenericDType(fl) {
        dtypep(this);
    }
    AstIfaceGenericDType(File_সারি* fl, File_সারি* modportFl, ধ্রুবক string& modport)
        : ASTGEN_SUPER_IfaceGenericDType(fl)
        , m_modportFileline{modportFl}
        , m_modportName{modport} {
        dtypep(this);
    }
    ASTGEN_MEMBERS_AstIfaceGenericDType;
    শূন্য dumpSmall(std::ostream& str) ধ্রুবক override;
    bool hasDType() ধ্রুবক override VL_MT_SAFE { ফেরত true; }
    bool maybePointedTo() ধ্রুবক override VL_MT_SAFE { ফেরত true; }
    bool undead() ধ্রুবক override { ফেরত true; }
    Ast_গ্রন্থি_DType* subDTypep() ধ্রুবক override VL_MT_STABLE { ফেরত nullptr; }
    Ast_গ্রন্থি_DType* virtRefDTypep() ধ্রুবক override { ফেরত nullptr; }
    শূন্য virtRefDTypep(Ast_গ্রন্থি_DType* nodep) override {}
    bool similar_DType_গ্রন্থি(ধ্রুবক Ast_গ্রন্থি_DType* samep) ধ্রুবক override { ফেরত this == samep; }
    AstBasicDType* basicp() ধ্রুবক override VL_MT_STABLE { ফেরত nullptr; }
    পূর্ণসংখ্যা widthAlignBytes() ধ্রুবক override { ফেরত 1; }
    পূর্ণসংখ্যা width_যোগ_Bytes() ধ্রুবক override { ফেরত 1; }
    string modportName() ধ্রুবক { ফেরত m_modportName; }
    bool isModport() { ফেরত !m_modportName.empty(); }
    bool isCompound() ধ্রুবক override { ফেরত true; }
    File_সারি* modportFileline() ধ্রুবক { ফেরত m_modportFileline; }
    string name() ধ্রুবক override { ফেরত m_modportName; }
};

class AstIfaceRefDType final : সর্বজনীন Ast_গ্রন্থি_DType {
    // Reference to an interface, either for a port, or inside parent cell
    // @astgen op1 := paramsp : List[AstPin]
    //
    // @astgen ptr := m_ifacep : Optional[AstIface]  // Interface; cellp() should override
    // @astgen ptr := m_cellp : Optional[AstCell]  // When exact parent cell known; not a guess
    // @astgen ptr := m_modportp : Optional[AstModport]  // nullptr = unlinked or no modport
    File_সারি* m_modportFileline;  // Where modport token was
    string m_cellName;  // "" = no cell, such as when connects to 'input' iface
    string m_ifaceName;  // Interface name
    string m_modportName;  // "" = no modport
    bool m_virtual = false;  // True if virtual interface
সর্বজনীন:
    AstIfaceRefDType(File_সারি* fl, ধ্রুবক string& cellName, ধ্রুবক string& ifaceName)
        : ASTGEN_SUPER_IfaceRefDType(fl)
        , m_modportFileline{nullptr}
        , m_cellName{cellName}
        , m_ifaceName{ifaceName}
        , m_modportName{""} {}
    AstIfaceRefDType(File_সারি* fl, File_সারি* modportFl, ধ্রুবক string& cellName,
                     ধ্রুবক string& ifaceName, ধ্রুবক string& modport)
        : ASTGEN_SUPER_IfaceRefDType(fl)
        , m_modportFileline{modportFl}
        , m_cellName{cellName}
        , m_ifaceName{ifaceName}
        , m_modportName{modport} {}
    AstIfaceRefDType(File_সারি* fl, File_সারি* modportFl, ধ্রুবক string& cellName,
                     ধ্রুবক string& ifaceName, ধ্রুবক string& modport, AstPin* paramsp)
        : ASTGEN_SUPER_IfaceRefDType(fl)
        , m_modportFileline{modportFl}
        , m_cellName{cellName}
        , m_ifaceName{ifaceName}
        , m_modportName{modport} {
        addParamsp(paramsp);
    }
    ASTGEN_MEMBERS_AstIfaceRefDType;

    // METHODS
    শূন্য dump(std::ostream& str = std::cout) ধ্রুবক override;
    শূন্য dumpJson(std::ostream& str = std::cout) ধ্রুবক override;
    শূন্য dumpSmall(std::ostream& str) ধ্রুবক override;
    string prettyDTypeName(bool full) ধ্রুবক override;
    AstBasicDType* basicp() ধ্রুবক override VL_MT_STABLE { ফেরত nullptr; }
    bool similar_DType_গ্রন্থি(ধ্রুবক Ast_গ্রন্থি_DType* samep) ধ্রুবক override {
        // Each occurrence of a virtual interface type parses to its own node,
        // so pointer identity is insufficient; compare the referenced
        // interface instead, same equivalence as AstNode::computeCastable uses.
        ধ্রুবক AstIfaceRefDType* ধ্রুবক asamep = VN_DBG_AS(samep, IfaceRefDType);
        ফেরত ifaceViaCellp() && ifaceViaCellp() == asamep->ifaceViaCellp()
               && modportName() == asamep->modportName() && isVirtual() == asamep->isVirtual();
    }
    পূর্ণসংখ্যা widthAlignBytes() ধ্রুবক override { ফেরত 0; }
    পূর্ণসংখ্যা width_যোগ_Bytes() ধ্রুবক override { ফেরত 0; }
    bool isVirtual() ধ্রুবক { ফেরত m_virtual; }
    শূন্য isVirtual(bool flag) {
        m_virtual = flag;
        যদি (flag) v3Global.setHasVirtIfaces();
    }
    File_সারি* modportFileline() ধ্রুবক { ফেরত m_modportFileline; }
    শূন্য modportFileline(File_সারি* ধ্রুবক modportFileline) { m_modportFileline = modportFileline; }
    string cellName() ধ্রুবক { ফেরত m_cellName; }
    শূন্য cellName(ধ্রুবক string& name) { m_cellName = name; }
    string ifaceName() ধ্রুবক { ফেরত m_ifaceName; }
    string ifaceNameQ() ধ্রুবক { ফেরত "'" + prettyName(ifaceName()) + "'"; }
    শূন্য ifaceName(ধ্রুবক string& name) { m_ifaceName = name; }
    শূন্য modportName(ধ্রুবক string& modportName) { m_modportName = modportName; }
    string modportName() ধ্রুবক { ফেরত m_modportName; }
    AstIface* ifaceViaCellp() ধ্রুবক;  // Use cellp or ifacep
    AstIface* ifacep() ধ্রুবক { ফেরত m_ifacep; }
    শূন্য ifacep(AstIface* nodep) { m_ifacep = nodep; }
    AstCell* cellp() ধ্রুবক { ফেরত m_cellp; }
    শূন্য cellp(AstCell* nodep) { m_cellp = nodep; }
    AstModport* modportp() ধ্রুবক { ফেরত m_modportp; }
    শূন্য modportp(AstModport* modportp) { m_modportp = modportp; }
    bool isModport() { ফেরত !m_modportName.empty(); }
    bool isCompound() ধ্রুবক override { ফেরত true; }  // But not relevant
};
class AstMemberDType final : সর্বজনীন Ast_গ্রন্থি_DType {
    // A member of a struct/union
    // PARENT: AstNodeUOrStructDType
    // @astgen op1 := childDTypep : Optional[AstNodeDType]
    // @astgen op3 := valuep : Optional[AstNode]
    //
    // @astgen ptr := m_refDTypep : Optional[AstNodeDType]  // Elements of this type (post-width)
    string m_name;  // Name of variable
    string m_tag;  // Holds the string of the verilator tag -- used in JSON output.
    পূর্ণসংখ্যা m_lsb = -1;  // Within this level's packed struct, the LSB of the first bit of the member
    bool m_constrainedRand = false;  // Member has a constraint expression
    VRandAttr m_rand;  // Randomizability of this member (rand, randc, etc)
সর্বজনীন:
    AstMemberDType(File_সারি* fl, ধ্রুবক string& name, VFlagChildDType, Ast_গ্রন্থি_DType* dtp,
                   Ast_গ্রন্থি* valuep)
        : ASTGEN_SUPER_MemberDType(fl)
        , m_name{name} {
        childDTypep(dtp);  // Only for parser
        this->valuep(valuep);
        dtypep(nullptr);  // V3Width will resolve
        refDTypep(nullptr);
    }
    AstMemberDType(File_সারি* fl, ধ্রুবক string& name, Ast_গ্রন্থি_DType* dtp)
        : ASTGEN_SUPER_MemberDType(fl)
        , m_name{name} {
        UASSERT(dtp, "AstMember created with no dtype");
        refDTypep(dtp);
        dtypep(this);
        widthFromSub(subDTypep());
    }
    ASTGEN_MEMBERS_AstMemberDType;
    শূন্য dump(std::ostream& str = std::cout) ধ্রুবক override;
    শূন্য dumpJson(std::ostream& str = std::cout) ধ্রুবক override;
    শূন্য dumpSmall(std::ostream& str) ধ্রুবক override;
    string prettyDTypeName(bool full) ধ্রুবক override;
    string name() ধ্রুবক override VL_MT_STABLE { ফেরত m_name; }  // * = Var name
    bool hasDType() ধ্রুবক override VL_MT_SAFE { ফেরত true; }
    bool maybePointedTo() ধ্রুবক override VL_MT_SAFE { ফেরত true; }
    Ast_গ্রন্থি_DType* getChildDTypep() ধ্রুবক override { ফেরত childDTypep(); }
    Ast_গ্রন্থি_UOr_Struct_DType* getChildStructp();
    Ast_গ্রন্থি_DType* subDTypep() ধ্রুবক override VL_MT_STABLE {
        ফেরত m_refDTypep ? m_refDTypep : childDTypep();
    }
    শূন্য refDTypep(Ast_গ্রন্থি_DType* nodep) { m_refDTypep = nodep; }
    Ast_গ্রন্থি_DType* virtRefDTypep() ধ্রুবক override { ফেরত m_refDTypep; }
    শূন্য virtRefDTypep(Ast_গ্রন্থি_DType* nodep) override { refDTypep(nodep); }
    bool similar_DType_গ্রন্থি(ধ্রুবক Ast_গ্রন্থি_DType* samep) ধ্রুবক override { ফেরত this == samep; }
    //
    // (Slow) recurse down to find basic data type (Note don't need virtual -
    // AstVar isn't a NodeDType)
    AstBasicDType* basicp() ধ্রুবক override VL_MT_STABLE { ফেরত subDTypep()->basicp(); }
    // op1 = Range of variable (Note don't need virtual - AstVar isn't a NodeDType)
    Ast_গ্রন্থি_DType* dtypeSkipRefp() ধ্রুবক { ফেরত subDTypep()->skipRefp(); }
    // (Slow) recurses - Structure alignment 1,2,4 or 8 bytes (arrays affect this)
    পূর্ণসংখ্যা widthAlignBytes() ধ্রুবক override { ফেরত subDTypep()->widthAlignBytes(); }
    // (Slow) recurses - Width in bytes rounding up 1,2,4,8,12,...
    পূর্ণসংখ্যা width_যোগ_Bytes() ধ্রুবক override { ফেরত subDTypep()->width_যোগ_Bytes(); }
    // METHODS
    শূন্য name(ধ্রুবক string& name) override { m_name = name; }
    string tag() ধ্রুবক override { ফেরত m_tag; }
    শূন্য tag(ধ্রুবক string& text) override { m_tag = text; }
    পূর্ণসংখ্যা lsb() ধ্রুবক { ফেরত m_lsb; }
    শূন্য lsb(পূর্ণসংখ্যা lsb) { m_lsb = lsb; }
    bool isCompound() ধ্রুবক override {
        v3fatalSrc("call isCompound on subdata type, not reference");
        ফেরত false;
    }
    bool isConstrainedRand() ধ্রুবক { ফেরত m_constrainedRand; }
    শূন্য markConstrainedRand(bool flag) { m_constrainedRand = flag; }
    VRandAttr rand() ধ্রুবক { ফেরত m_rand; }
    শূন্য rand(ধ্রুবক VRandAttr flag) { m_rand = flag; }
};
class AstNBACommitQueueDType final : সর্বজনীন Ast_গ্রন্থি_DType {
    // @astgen ptr := m_subDTypep : AstNodeDType  // Type of the corresponding variable
    ধ্রুবক bool m_partial;  // Partial element update required

সর্বজনীন:
    AstNBACommitQueueDType(File_সারি* fl, Ast_গ্রন্থি_DType* subDTypep, bool partial)
        : ASTGEN_SUPER_NBACommitQueueDType(fl)
        , m_partial{partial}
        , m_subDTypep{subDTypep} {
        dtypep(this);
    }
    ASTGEN_MEMBERS_AstNBACommitQueueDType;
    শূন্য dump(std::ostream& str) ধ্রুবক override;
    শূন্য dumpJson(std::ostream& str) ধ্রুবক override;
    Ast_গ্রন্থি_DType* subDTypep() ধ্রুবক override VL_MT_STABLE { ফেরত m_subDTypep; }
    bool partial() ধ্রুবক { ফেরত m_partial; }
    bool same_গ্রন্থি(ধ্রুবক Ast_গ্রন্থি* samep) ধ্রুবক override {
        ধ্রুবক AstNBACommitQueueDType* ধ্রুবক asamep = VN_DBG_AS(samep, NBACommitQueueDType);
        ফেরত m_partial == asamep->m_partial;
    }
    bool similar_DType_গ্রন্থি(ধ্রুবক Ast_গ্রন্থি_DType* samep) ধ্রুবক override { ফেরত this == samep; }
    AstBasicDType* basicp() ধ্রুবক override VL_MT_STABLE { ফেরত nullptr; }
    পূর্ণসংখ্যা widthAlignBytes() ধ্রুবক override { ফেরত 1; }
    পূর্ণসংখ্যা width_যোগ_Bytes() ধ্রুবক override { ফেরত 24; }
    bool isCompound() ধ্রুবক override { ফেরত true; }
};
class AstParamTypeDType final : সর্বজনীন Ast_গ্রন্থি_DType {
    // Parents: MODULE
    // A parameter type statement; much like a var or typedef
    // @astgen op1 := childDTypep : Optional[AstNodeDType]
    ধ্রুবক VVarType m_varType;  // Type of variable (for localparam vs. param)
    ধ্রুবক VFwdType m_fwdType;  // Forward type for lint check
    string m_name;  // Name of variable
সর্বজনীন:
    AstParamTypeDType(File_সারি* fl, VVarType type, VFwdType fwdType, ধ্রুবক string& name,
                      VFlagChildDType, Ast_গ্রন্থি_DType* dtp)
        : ASTGEN_SUPER_ParamTypeDType(fl)
        , m_varType{type}
        , m_fwdType{fwdType}
        , m_name{name} {
        childDTypep(dtp);  // Only for parser
        dtypep(nullptr);  // V3Width will resolve
    }
    ASTGEN_MEMBERS_AstParamTypeDType;
    শূন্য dump(std::ostream& str = std::cout) ধ্রুবক override;
    শূন্য dumpJson(std::ostream& str = std::cout) ধ্রুবক override;
    string prettyDTypeName(bool full) ধ্রুবক override;
    Ast_গ্রন্থি_DType* getChildDTypep() ধ্রুবক override { ফেরত childDTypep(); }
    Ast_গ্রন্থি_DType* subDTypep() ধ্রুবক override VL_MT_STABLE {
        ফেরত dtypep() ? dtypep() : childDTypep();
    }
    AstBasicDType* basicp() ধ্রুবক override VL_MT_STABLE { ফেরত subDTypep()->basicp(); }
    bool similar_DType_গ্রন্থি(ধ্রুবক Ast_গ্রন্থি_DType* samep) ধ্রুবক override {
        ধ্রুবক AstParamTypeDType* ধ্রুবক sp = VN_DBG_AS(samep, ParamTypeDType);
        ফেরত this->subDTypep()->similarDType(sp->subDTypep());
    }
    পূর্ণসংখ্যা widthAlignBytes() ধ্রুবক override { ফেরত dtypep()->widthAlignBytes(); }
    পূর্ণসংখ্যা width_যোগ_Bytes() ধ্রুবক override { ফেরত dtypep()->width_যোগ_Bytes(); }
    // METHODS
    string name() ধ্রুবক override VL_MT_STABLE { ফেরত m_name; }
    bool maybePointedTo() ধ্রুবক override VL_MT_SAFE { ফেরত true; }
    bool hasDType() ধ্রুবক override VL_MT_SAFE { ফেরত true; }
    শূন্য name(ধ্রুবক string& flag) override { m_name = flag; }
    VVarType varType() ধ্রুবক { ফেরত m_varType; }  // * = Type of variable
    VFwdType fwdType() ধ্রুবক { ফেরত m_fwdType; }
    bool isParam() ধ্রুবক { ফেরত true; }
    bool isGParam() ধ্রুবক { ফেরত (varType() == VVarType::GPARAM); }
    bool isCompound() ধ্রুবক override {
        v3fatalSrc("call isCompound on subdata type, not reference");
        ফেরত false;
    }
};
class AstParseTypeDType final : সর্বজনীন Ast_গ্রন্থি_DType {
    // Parents: VAR
    // During parsing, this indicates the type of a parameter is a "parameter type"
    // e.g. the data type is a container of any data type
    ধ্রুবক VFwdType m_fwdType;  // Forward type for lint check
সর্বজনীন:
    explicit AstParseTypeDType(File_সারি* fl, VFwdType fwdType = VFwdType::NONE)
        : ASTGEN_SUPER_ParseTypeDType(fl)
        , m_fwdType{fwdType} {}
    ASTGEN_MEMBERS_AstParseTypeDType;
    Ast_গ্রন্থি_DType* dtypep() ধ্রুবক VL_MT_STABLE { ফেরত nullptr; }
    // METHODS
    শূন্য dump(std::ostream& str = std::cout) ধ্রুবক override;
    শূন্য dumpJson(std::ostream& str = std::cout) ধ্রুবক override;
    bool similar_DType_গ্রন্থি(ধ্রুবক Ast_গ্রন্থি_DType* samep) ধ্রুবক override { ফেরত this == samep; }
    AstBasicDType* basicp() ধ্রুবক override VL_MT_STABLE { ফেরত nullptr; }
    পূর্ণসংখ্যা widthAlignBytes() ধ্রুবক override { ফেরত 0; }
    পূর্ণসংখ্যা width_যোগ_Bytes() ধ্রুবক override { ফেরত 0; }
    bool isCompound() ধ্রুবক override {
        v3fatalSrc("call isCompound on subdata type, not reference");
        ফেরত false;
    }
    VFwdType fwdType() ধ্রুবক { ফেরত m_fwdType; }
};
class AstQueueDType final : সর্বজনীন Ast_গ্রন্থি_DType {
    // Queue array data type, ie "[ $ ]"
    // @astgen op1 := childDTypep : Optional[AstNodeDType] // moved to refDTypep() in V3Width
    // @astgen op2 := boundp : Optional[AstNodeExpr]
    //
    // @astgen ptr := m_refDTypep : Optional[AstNodeDType]  // Elements of this type (post-width)
সর্বজনীন:
    AstQueueDType(File_সারি* fl, VFlagChildDType, Ast_গ্রন্থি_DType* dtp, Ast_গ্রন্থি_রাশিমালা* boundp)
        : ASTGEN_SUPER_QueueDType(fl) {
        childDTypep(dtp);
        this->boundp(boundp);
        refDTypep(nullptr);
        dtypep(nullptr);  // V3Width will resolve
    }
    AstQueueDType(File_সারি* fl, Ast_গ্রন্থি_DType* dtp, Ast_গ্রন্থি_রাশিমালা* boundp)
        : ASTGEN_SUPER_QueueDType(fl) {
        this->boundp(boundp);
        refDTypep(dtp);
        dtypep(dtp);
    }
    ASTGEN_MEMBERS_AstQueueDType;
    ধ্রুবক অক্ষর* broken() ধ্রুবক override {
        BROKEN_RTN(!((m_refDTypep && !childDTypep()) || (!m_refDTypep && childDTypep())));
        ফেরত nullptr;
    }
    bool same_গ্রন্থি(ধ্রুবক Ast_গ্রন্থি* samep) ধ্রুবক override {
        ধ্রুবক AstQueueDType* ধ্রুবক asamep = VN_DBG_AS(samep, QueueDType);
        যদি (!asamep->subDTypep()) ফেরত false;
        ফেরত (subDTypep() == asamep->subDTypep());
    }
    bool similar_DType_গ্রন্থি(ধ্রুবক Ast_গ্রন্থি_DType* samep) ধ্রুবক override {
        ধ্রুবক AstQueueDType* ধ্রুবক asamep = VN_DBG_AS(samep, QueueDType);
        ফেরত asamep->subDTypep() && subDTypep()->similarDType(asamep->subDTypep());
    }
    শূন্য dumpSmall(std::ostream& str) ধ্রুবক override;
    string prettyDTypeName(bool full) ধ্রুবক override;
    Ast_গ্রন্থি_DType* getChildDTypep() ধ্রুবক override { ফেরত childDTypep(); }
    Ast_গ্রন্থি_DType* subDTypep() ধ্রুবক override VL_MT_STABLE {
        ফেরত m_refDTypep ? m_refDTypep : childDTypep();
    }
    bool isAggregateType() ধ্রুবক override { ফেরত true; }
    শূন্য refDTypep(Ast_গ্রন্থি_DType* nodep) { m_refDTypep = nodep; }
    অন্তঃরেখ পূর্ণসংখ্যা boundConst() ধ্রুবক VL_MT_STABLE;
    Ast_গ্রন্থি_DType* virtRefDTypep() ধ্রুবক override { ফেরত m_refDTypep; }
    শূন্য virtRefDTypep(Ast_গ্রন্থি_DType* nodep) override { refDTypep(nodep); }
    // METHODS
    AstBasicDType* basicp() ধ্রুবক override VL_MT_STABLE { ফেরত nullptr; }
    পূর্ণসংখ্যা widthAlignBytes() ধ্রুবক override { ফেরত subDTypep()->widthAlignBytes(); }
    পূর্ণসংখ্যা width_যোগ_Bytes() ধ্রুবক override { ফেরত subDTypep()->width_যোগ_Bytes(); }
    bool isCompound() ধ্রুবক override { ফেরত true; }
    bool isDynamicallySized() ধ্রুবক override { ফেরত true; }
};
class AstRefDType final : সর্বজনীন Ast_গ্রন্থি_DType {
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
    ধ্রুবক VIfaceCaptureTag* m_captureTagp = nullptr;

সর্বজনীন:
    AstRefDType(File_সারি* fl, ধ্রুবক string& name)
        : ASTGEN_SUPER_RefDType(fl)
        , m_name{name} {}
    AstRefDType(File_সারি* fl, ধ্রুবক string& name, Ast_গ্রন্থি_রাশিমালা* classOrPackagep, AstPin* paramsp)
        : ASTGEN_SUPER_RefDType(fl)
        , m_name{name} {
        classOrPackageOpp(classOrPackagep);
        addParamsp(paramsp);
    }
    class Flag_Type_Of_রাশিমালা {};  // type(expr) for parser only
    AstRefDType(File_সারি* fl, Flag_Type_Of_রাশিমালা, Ast_গ্রন্থি* typeofp)
        : ASTGEN_SUPER_RefDType(fl) {
        this->typeofp(typeofp);
        যদি (Ast_গ্রন্থি_DType* ধ্রুবক dtp = VN_CAST(typeofp, গ্রন্থি_DType)) refDTypep(dtp);
    }
    ASTGEN_MEMBERS_AstRefDType;
    // METHODS
    bool same_গ্রন্থি(ধ্রুবক Ast_গ্রন্থি* samep) ধ্রুবক override {
        ধ্রুবক AstRefDType* ধ্রুবক asamep = VN_DBG_AS(samep, RefDType);
        ফেরত (m_typedefp == asamep->m_typedefp && m_refDTypep == asamep->m_refDTypep
                && m_name == asamep->m_name && m_classOrPackagep == asamep->m_classOrPackagep);
    }
    bool similar_DType_গ্রন্থি(ধ্রুবক Ast_গ্রন্থি_DType* samep) ধ্রুবক override {
        ফেরত subDTypep()->similarDType(samep->subDTypep());
    }
    ধ্রুবক অক্ষর* broken() ধ্রুবক override;
    শূন্য dump(std::ostream& str = std::cout) ধ্রুবক override;
    শূন্য dumpJson(std::ostream& str = std::cout) ধ্রুবক override;
    শূন্য dumpSmall(std::ostream& str) ধ্রুবক override;
    string name() ধ্রুবক override VL_MT_STABLE { ফেরত m_name; }
    string prettyDTypeName(bool full) ধ্রুবক override {
        ফেরত subDTypep() ? subDTypep()->prettyDTypeName(full) : prettyName();
    }
    AstBasicDType* basicp() ধ্রুবক override VL_MT_STABLE {
        ফেরত subDTypep() ? subDTypep()->basicp() : nullptr;
    }
    Ast_গ্রন্থি_DType* subDTypep() ধ্রুবক override VL_MT_STABLE;
    Ast_গ্রন্থি_DType* getChildDTypep() ধ্রুবক override { ফেরত VN_CAST(typeofp(), গ্রন্থি_DType); }
    পূর্ণসংখ্যা widthAlignBytes() ধ্রুবক override { ফেরত dtypeSkipRefp()->widthAlignBytes(); }
    পূর্ণসংখ্যা width_যোগ_Bytes() ধ্রুবক override { ফেরত dtypeSkipRefp()->width_যোগ_Bytes(); }
    শূন্য name(ধ্রুবক string& flag) override { m_name = flag; }
    Ast_গ্রন্থি_DType* dtypeSkipRefp() ধ্রুবক { ফেরত subDTypep()->skipRefp(); }
    AstTypedef* typedefp() ধ্রুবক VL_MT_SAFE { ফেরত m_typedefp; }
    শূন্য typedefp(AstTypedef* nodep) { m_typedefp = nodep; }
    Ast_গ্রন্থি_DType* refDTypep() ধ্রুবক VL_MT_SAFE { ফেরত m_refDTypep; }
    শূন্য refDTypep(Ast_গ্রন্থি_DType* nodep) { m_refDTypep = nodep; }
    Ast_গ্রন্থি_DType* virtRefDTypep() ধ্রুবক override { ফেরত refDTypep(); }
    শূন্য virtRefDTypep(Ast_গ্রন্থি_DType* nodep) override { refDTypep(nodep); }
    Ast_গ্রন্থি_Module* classOrPackagep() ধ্রুবক { ফেরত m_classOrPackagep; }
    শূন্য classOrPackagep(Ast_গ্রন্থি_Module* nodep) { m_classOrPackagep = nodep; }
    ধ্রুবক VIfaceCaptureTag* captureTagp() ধ্রুবক { ফেরত m_captureTagp; }
    শূন্য captureTagp(ধ্রুবক VIfaceCaptureTag* tagp) { m_captureTagp = tagp; }
    bool isCompound() ধ্রুবক override {
        v3fatalSrc("call isCompound on subdata type, not reference");
        ফেরত false;
    }
};
class AstRequireDType final : সর্বজনীন Ast_গ্রন্থি_DType {
    // @astgen op1 := lhsp : Optional[AstNode<AstNodeExpr|AstNodeDType>]
    //
    // Require a generic node type (typically AstParseRef become a type.
সর্বজনীন:
    AstRequireDType(File_সারি* fl, Ast_গ্রন্থি* lhsp)
        : ASTGEN_SUPER_RequireDType(fl) {
        this->lhsp(lhsp);
    }
    ASTGEN_MEMBERS_AstRequireDType;
    // METHODS
    bool similar_DType_গ্রন্থি(ধ্রুবক Ast_গ্রন্থি_DType* samep) ধ্রুবক override {
        ধ্রুবক AstRequireDType* ধ্রুবক asamep = VN_DBG_AS(samep, RequireDType);
        ফেরত subDTypep()->similarDType(asamep->subDTypep());
    }
    AstBasicDType* basicp() ধ্রুবক override VL_MT_STABLE { ফেরত nullptr; }
    Ast_গ্রন্থি_DType* subDTypep() ধ্রুবক override VL_MT_STABLE {
        // Used for recursive definition checking
        যদি (Ast_গ্রন্থি_DType* ধ্রুবক dtp = VN_CAST(lhsp(), গ্রন্থি_DType)) ফেরত dtp;

        ফেরত nullptr;
    }
    পূর্ণসংখ্যা widthAlignBytes() ধ্রুবক override { V3ERROR_NA_RETURN(1); }
    পূর্ণসংখ্যা width_যোগ_Bytes() ধ্রুবক override { V3ERROR_NA_RETURN(1); }
    bool isCompound() ধ্রুবক override { V3ERROR_NA_RETURN(false); }
};
class AstSampleQueueDType final : সর্বজনীন Ast_গ্রন্থি_DType {
    // @astgen op1 := childDTypep : Optional[AstNodeDType] // moved to refDTypep() in V3Width
    //
    // @astgen ptr := m_refDTypep : Optional[AstNodeDType]  // Elements of this type (post-width)
সর্বজনীন:
    AstSampleQueueDType(File_সারি* fl, Ast_গ্রন্থি_DType* dtp)
        : ASTGEN_SUPER_SampleQueueDType(fl) {
        refDTypep(dtp);
        dtypep(dtp);
    }
    ASTGEN_MEMBERS_AstSampleQueueDType;
    ধ্রুবক অক্ষর* broken() ধ্রুবক override {
        BROKEN_RTN(!((m_refDTypep && !childDTypep()) || (!m_refDTypep && childDTypep())));
        ফেরত nullptr;
    }
    bool same_গ্রন্থি(ধ্রুবক Ast_গ্রন্থি* samep) ধ্রুবক override {
        ধ্রুবক AstSampleQueueDType* ধ্রুবক asamep = VN_DBG_AS(samep, SampleQueueDType);
        যদি (!asamep->subDTypep()) ফেরত false;
        ফেরত (subDTypep() == asamep->subDTypep());
    }
    bool similar_DType_গ্রন্থি(ধ্রুবক Ast_গ্রন্থি_DType* samep) ধ্রুবক override {
        ধ্রুবক AstSampleQueueDType* ধ্রুবক asamep = VN_DBG_AS(samep, SampleQueueDType);
        ফেরত asamep->subDTypep() && subDTypep()->similarDType(asamep->subDTypep());
    }
    শূন্য dumpSmall(std::ostream& str) ধ্রুবক override;
    Ast_গ্রন্থি_DType* getChildDTypep() ধ্রুবক override { ফেরত childDTypep(); }
    // op1 = Range of variable
    Ast_গ্রন্থি_DType* subDTypep() ধ্রুবক override VL_MT_STABLE {
        ফেরত m_refDTypep ? m_refDTypep : childDTypep();
    }
    শূন্য refDTypep(Ast_গ্রন্থি_DType* nodep) { m_refDTypep = nodep; }
    Ast_গ্রন্থি_DType* virtRefDTypep() ধ্রুবক override { ফেরত m_refDTypep; }
    শূন্য virtRefDTypep(Ast_গ্রন্থি_DType* nodep) override { refDTypep(nodep); }
    // METHODS
    AstBasicDType* basicp() ধ্রুবক override VL_MT_STABLE { ফেরত subDTypep()->basicp(); }
    পূর্ণসংখ্যা widthAlignBytes() ধ্রুবক override { ফেরত আকার(std::map<std::string, std::string>); }
    পূর্ণসংখ্যা width_যোগ_Bytes() ধ্রুবক override { ফেরত আকার(std::map<std::string, std::string>); }
    bool isCompound() ধ্রুবক override { ফেরত true; }
};
class AstStreamDType final : সর্বজনীন Ast_গ্রন্থি_DType {
    // Stream data type, used only as data type of stream operations
    // Should behave like AstPackArrayDType, but it doesn't have a size
সর্বজনীন:
    explicit AstStreamDType(File_সারি* fl)
        : ASTGEN_SUPER_StreamDType(fl) {
        dtypep(this);
    }
    ASTGEN_MEMBERS_AstStreamDType;
    শূন্য dumpSmall(std::ostream& str) ধ্রুবক override;
    bool hasDType() ধ্রুবক override VL_MT_SAFE { ফেরত true; }
    bool maybePointedTo() ধ্রুবক override VL_MT_SAFE { ফেরত true; }
    bool undead() ধ্রুবক override { ফেরত true; }
    Ast_গ্রন্থি_DType* subDTypep() ধ্রুবক override VL_MT_STABLE { ফেরত nullptr; }
    Ast_গ্রন্থি_DType* virtRefDTypep() ধ্রুবক override { ফেরত nullptr; }
    শূন্য virtRefDTypep(Ast_গ্রন্থি_DType* nodep) override {}
    bool similar_DType_গ্রন্থি(ধ্রুবক Ast_গ্রন্থি_DType* samep) ধ্রুবক override { ফেরত this == samep; }
    AstBasicDType* basicp() ধ্রুবক override VL_MT_STABLE { ফেরত nullptr; }
    পূর্ণসংখ্যা widthAlignBytes() ধ্রুবক override { ফেরত 1; }
    পূর্ণসংখ্যা width_যোগ_Bytes() ধ্রুবক override { ফেরত 1; }
    bool isCompound() ধ্রুবক override { ফেরত false; }
};
class AstUnsizedArrayDType final : সর্বজনীন Ast_গ্রন্থি_DType {
    // Unsized/open-range Array data type, ie "some_dtype var_name []"
    // @astgen op1 := childDTypep : Optional[AstNodeDType] // moved to refDTypep() in V3Width
    //
    // @astgen ptr := m_refDTypep : Optional[AstNodeDType]  // Elements of this type (post-width)
সর্বজনীন:
    AstUnsizedArrayDType(File_সারি* fl, VFlagChildDType, Ast_গ্রন্থি_DType* dtp)
        : ASTGEN_SUPER_UnsizedArrayDType(fl) {
        childDTypep(dtp);  // Only for parser
        refDTypep(nullptr);
        dtypep(nullptr);  // V3Width will resolve
    }
    AstUnsizedArrayDType(File_সারি* fl, Ast_গ্রন্থি_DType* dtp)
        : ASTGEN_SUPER_UnsizedArrayDType(fl) {
        refDTypep(dtp);
        dtypep(nullptr);  // V3Width will resolve
    }
    ASTGEN_MEMBERS_AstUnsizedArrayDType;
    ধ্রুবক অক্ষর* broken() ধ্রুবক override {
        BROKEN_RTN(!((m_refDTypep && !childDTypep()) || (!m_refDTypep && childDTypep())));
        ফেরত nullptr;
    }
    bool same_গ্রন্থি(ধ্রুবক Ast_গ্রন্থি* samep) ধ্রুবক override;
    bool similar_DType_গ্রন্থি(ধ্রুবক Ast_গ্রন্থি_DType* samep) ধ্রুবক override;
    শূন্য dumpSmall(std::ostream& str) ধ্রুবক override;
    Ast_গ্রন্থি_DType* getChildDTypep() ধ্রুবক override { ফেরত childDTypep(); }
    Ast_গ্রন্থি_DType* subDTypep() ধ্রুবক override VL_MT_STABLE {
        ফেরত m_refDTypep ? m_refDTypep : childDTypep();
    }
    শূন্য refDTypep(Ast_গ্রন্থি_DType* nodep) { m_refDTypep = nodep; }
    Ast_গ্রন্থি_DType* virtRefDTypep() ধ্রুবক override { ফেরত m_refDTypep; }
    শূন্য virtRefDTypep(Ast_গ্রন্থি_DType* nodep) override { refDTypep(nodep); }
    // METHODS
    AstBasicDType* basicp() ধ্রুবক override VL_MT_STABLE { ফেরত subDTypep()->basicp(); }
    পূর্ণসংখ্যা widthAlignBytes() ধ্রুবক override { ফেরত subDTypep()->widthAlignBytes(); }
    পূর্ণসংখ্যা width_যোগ_Bytes() ধ্রুবক override { ফেরত subDTypep()->width_যোগ_Bytes(); }
    bool isCompound() ধ্রুবক override { ফেরত true; }
};
class AstVoidDType final : সর্বজনীন Ast_গ্রন্থি_DType {
    // For e.g. a function returning void
সর্বজনীন:
    explicit AstVoidDType(File_সারি* fl)
        : ASTGEN_SUPER_VoidDType(fl) {
        dtypep(this);
    }
    ASTGEN_MEMBERS_AstVoidDType;
    শূন্য dumpSmall(std::ostream& str) ধ্রুবক override;
    bool hasDType() ধ্রুবক override VL_MT_SAFE { ফেরত true; }
    bool maybePointedTo() ধ্রুবক override VL_MT_SAFE { ফেরত true; }
    bool undead() ধ্রুবক override { ফেরত true; }
    Ast_গ্রন্থি_DType* subDTypep() ধ্রুবক override VL_MT_STABLE { ফেরত nullptr; }
    Ast_গ্রন্থি_DType* virtRefDTypep() ধ্রুবক override { ফেরত nullptr; }
    শূন্য virtRefDTypep(Ast_গ্রন্থি_DType* nodep) override {}
    bool similar_DType_গ্রন্থি(ধ্রুবক Ast_গ্রন্থি_DType* samep) ধ্রুবক override { ফেরত true; }
    AstBasicDType* basicp() ধ্রুবক override VL_MT_STABLE { ফেরত nullptr; }
    পূর্ণসংখ্যা widthAlignBytes() ধ্রুবক override { ফেরত 1; }
    পূর্ণসংখ্যা width_যোগ_Bytes() ধ্রুবক override { ফেরত 1; }
    bool isCompound() ধ্রুবক override { ফেরত false; }
};
class AstWildcardArrayDType final : সর্বজনীন Ast_গ্রন্থি_DType {
    // Wildcard index type associative array data type, ie "some_dtype var_name [*]"
    // @astgen op1 := childDTypep : Optional[AstNodeDType] // moved to refDTypep() in V3Width
    //
    // @astgen ptr := m_refDTypep : Optional[AstNodeDType]  // Elements of this type (post-width)
সর্বজনীন:
    AstWildcardArrayDType(File_সারি* fl, VFlagChildDType, Ast_গ্রন্থি_DType* dtp)
        : ASTGEN_SUPER_WildcardArrayDType(fl) {
        childDTypep(dtp);  // Only for parser
        refDTypep(nullptr);
        dtypep(nullptr);  // V3Width will resolve
    }
    ASTGEN_MEMBERS_AstWildcardArrayDType;
    ধ্রুবক অক্ষর* broken() ধ্রুবক override {
        BROKEN_RTN(!((m_refDTypep && !childDTypep()) || (!m_refDTypep && childDTypep())));
        ফেরত nullptr;
    }
    bool same_গ্রন্থি(ধ্রুবক Ast_গ্রন্থি* samep) ধ্রুবক override;
    bool similar_DType_গ্রন্থি(ধ্রুবক Ast_গ্রন্থি_DType* samep) ধ্রুবক override;
    শূন্য dumpSmall(std::ostream& str) ধ্রুবক override;
    string prettyDTypeName(bool full) ধ্রুবক override;
    Ast_গ্রন্থি_DType* getChildDTypep() ধ্রুবক override { ফেরত childDTypep(); }
    Ast_গ্রন্থি_DType* subDTypep() ধ্রুবক override VL_MT_STABLE {
        ফেরত m_refDTypep ? m_refDTypep : childDTypep();
    }
    শূন্য refDTypep(Ast_গ্রন্থি_DType* nodep) { m_refDTypep = nodep; }
    Ast_গ্রন্থি_DType* virtRefDTypep() ধ্রুবক override { ফেরত m_refDTypep; }
    শূন্য virtRefDTypep(Ast_গ্রন্থি_DType* nodep) override { refDTypep(nodep); }
    // METHODS
    AstBasicDType* basicp() ধ্রুবক override VL_MT_STABLE { ফেরত subDTypep()->basicp(); }
    পূর্ণসংখ্যা widthAlignBytes() ধ্রুবক override { ফেরত আকার(std::map<std::string, std::string>); }
    পূর্ণসংখ্যা width_যোগ_Bytes() ধ্রুবক override { ফেরত আকার(std::map<std::string, std::string>); }
    bool isCompound() ধ্রুবক override { ফেরত true; }
    bool isDynamicallySized() ধ্রুবক override { ফেরত true; }
};

// === AstNodeArrayDType ===
class AstPackArrayDType final : সর্বজনীন Ast_গ্রন্থি_Array_DType {
    // Packed array data type, ie "some_dtype [2:0] var_name"
সর্বজনীন:
    অন্তঃরেখ AstPackArrayDType(File_সারি* fl, VFlagChildDType, Ast_গ্রন্থি_DType* dtp, AstRange* rangep);
    অন্তঃরেখ AstPackArrayDType(File_সারি* fl, Ast_গ্রন্থি_DType* dtp, AstRange* rangep);
    ASTGEN_MEMBERS_AstPackArrayDType;
    string prettyDTypeName(bool full) ধ্রুবক override;
    bool isCompound() ধ্রুবক override { ফেরত false; }
};
class AstUnpackArrayDType final : সর্বজনীন Ast_গ্রন্থি_Array_DType {
    // Array data type, ie "some_dtype var_name [2:0]"
    bool m_isCompound = false;  // Non-POD subDType, or parent requires compound
সর্বজনীন:
    AstUnpackArrayDType(File_সারি* fl, VFlagChildDType, Ast_গ্রন্থি_DType* dtp, AstRange* rangep)
        : ASTGEN_SUPER_UnpackArrayDType(fl) {
        childDTypep(dtp);  // Only for parser
        this->rangep(rangep);
        refDTypep(nullptr);
        dtypep(nullptr);  // V3Width will resolve
        // For backward compatibility AstNodeArrayDType and others inherit
        // width and signing from the subDType/base type
        widthFromSub(subDTypep());
    }
    AstUnpackArrayDType(File_সারি* fl, Ast_গ্রন্থি_DType* dtp, AstRange* rangep)
        : ASTGEN_SUPER_UnpackArrayDType(fl) {
        this->rangep(rangep);
        refDTypep(dtp);
        dtypep(this);
        // For backward compatibility AstNodeArrayDType and others inherit
        // width and signing from the subDType/base type
        widthFromSub(subDTypep());
    }
    ASTGEN_MEMBERS_AstUnpackArrayDType;
    শূন্য dump(std::ostream& str) ধ্রুবক override;
    শূন্য dumpJson(std::ostream& str) ধ্রুবক override;
    string prettyDTypeName(bool full) ধ্রুবক override;
    bool same_গ্রন্থি(ধ্রুবক Ast_গ্রন্থি* samep) ধ্রুবক override {
        ধ্রুবক AstUnpackArrayDType* ধ্রুবক sp = VN_DBG_AS(samep, UnpackArrayDType);
        ফেরত m_isCompound == sp->m_isCompound;
    }
    bool similar_DType_গ্রন্থি(ধ্রুবক Ast_গ্রন্থি_DType* samep) ধ্রুবক override;
    bool isAggregateType() ধ্রুবক override { ফেরত true; }
    // True iff 'otherp' is an array of the same shape, with
    // at least one dimension reversed ascending/descending.
    bool isReverseOf(ধ্রুবক Ast_গ্রন্থি_DType* otherp) ধ্রুবক;
    // Outer dimension comes first. The first element is this node.
    std::vector<AstUnpackArrayDType*> unpackDimensions();
    শূন্য isCompound(bool flag) { m_isCompound = flag; }
    bool isCompound() ধ্রুবক override VL_MT_SAFE { ফেরত m_isCompound; }
    bool isIntegralOrPacked() ধ্রুবক override { ফেরত false; }
};

// === AstNodeUOrStructDType ===
class AstStructDType final : সর্বজনীন Ast_গ্রন্থি_UOr_Struct_DType {
সর্বজনীন:
    // VSigning below is mispurposed to indicate if packed or not
    AstStructDType(File_সারি* fl, VSigning numericUnpack)
        : ASTGEN_SUPER_StructDType(fl, numericUnpack) {}
    ASTGEN_MEMBERS_AstStructDType;
    string verilogKwd() ধ্রুবক override { ফেরত "struct"; }
};
class AstUnionDType final : সর্বজনীন Ast_গ্রন্থি_UOr_Struct_DType {
    bool m_isSoft;  // Is a "union soft"
    bool m_isTagged;  // Is a "union tagged"

সর্বজনীন:
    // VSigning below is mispurposed to indicate if packed or not
    // isSoft implies packed
    AstUnionDType(File_সারি* fl, bool isSoft, bool isTagged, VSigning numericUnpack)
        : ASTGEN_SUPER_UnionDType(fl, numericUnpack)
        , m_isSoft{isSoft}
        , m_isTagged{isTagged} {
        packed(packed() | m_isSoft);
    }
    ASTGEN_MEMBERS_AstUnionDType;
    string verilogKwd() ধ্রুবক override { ফেরত "union"; }
    bool isSoft() ধ্রুবক { ফেরত m_isSoft; }
    bool isTagged() ধ্রুবক { ফেরত m_isTagged; }
    bool same_গ্রন্থি(ধ্রুবক Ast_গ্রন্থি* samep) ধ্রুবক override;
    শূন্য dump(std::ostream& str) ধ্রুবক override;
    শূন্য dumpJson(std::ostream& str) ধ্রুবক override;
};

#endif  // Guard
