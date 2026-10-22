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
#شامل "V3Ast.h"  // This helps code analysis tools pick up symbols in V3Ast.h
#define VL_NOT_FINAL  // This #define fixes broken code folding in the CLion IDE
#endif

// === Abstract base node types (AstNode*) =====================================

class Ast_گره_DType VL_NOT_FINAL : عمومی Ast_گره {
    // Ideally width() would migrate to BasicDType as that's where it makes sense,
    // but it's currently so prevalent in the code we leave it here.
    // Note the below members are included in AstTypeTable::Key lookups
    // dist-ast-dump-suppress  // Part of dumpSmall
    صحیح m_width = 0;  // (also in AstTypeTable::Key) Bit width of operation
    // (also in AstTypeTable::Key) If unsized, bitwidth of minimum implementation
    // dist-ast-dump-suppress  // Part of dumpSmall
    صحیح m_widthMin = 0;
    // dist-ast-dump-suppress  // Part of dumpSmall
    VSigning m_numeric;  // (also in AstTypeTable::Key) Node is signed
    // dist-ast-dump-suppress  // Part of dumpSmall
    bool m_generic = false;  // Simple globally referenced type, don't garbage collect
    // Unique number assigned to each dtype during creation for IEEE matching
    ایستا صحیح s_unique_رقم;

محافظتشده:
    // CONSTRUCTORS
    Ast_گره_DType(VNType t, File_سطر* fl)
        : Ast_گره{t, fl} {}

خصوصی:
    // METHODS
    ثابت Ast_گره_DType* skipRefIterp(bool skipConst, bool skipEnum,
                                     bool assertOn = true) ثابت VL_MT_STABLE;

محافظتشده:
    // METHODS
    مجازی bool similar_DType_گره(ثابت Ast_گره_DType* samep) ثابت = 0;

عمومی:
    ASTGEN_MEMBERS_Ast_گره_DType;
    // ACCESSORS
    تهی dump(std::ostream& str) ثابت override;
    تهی dumpJson(std::ostream& str) ثابت override;
    مجازی تهی dumpSmall(std::ostream& str) ثابت VL_MT_STABLE;
    bool hasDType() ثابت override VL_MT_SAFE { بازگشت true; }
    /// Require VlUnpacked, instead of [] for POD elements.
    /// A non-POD object is always compound, but some POD elements
    /// are compound when methods calls operate on object, or when
    /// under another compound-requiring object e.g. class
    مجازی bool isCompound() ثابت = 0;
    // Integral or packed, allowed inside an unpacked union/struct
    مجازی bool isIntegralOrPacked() ثابت { بازگشت !isCompound(); }
    // (Slow) recurse down to find basic data type
    مجازی AstBasicDType* basicp() ثابت VL_MT_STABLE = 0;
    // (Slow) Recurse over MemberDType|ParamTypeDType|RefDType|ConstDType|EnumDType to other type
    ثابت Ast_گره_DType* skipRefp() ثابت VL_MT_STABLE { بازگشت skipRefIterp(true, true); }
    Ast_گره_DType* skipRefp() VL_MT_STABLE {
        بازگشت const_cast<Ast_گره_DType*>(
            static_cast<ثابت Ast_گره_DType*>(this)->skipRefIterp(true, true));
    }
    // (Slow) Recurse over MemberDType|ParamTypeDType|RefDType|ConstDType|EnumDType to other type,
    // Returns null if not resolved
    ثابت Ast_گره_DType* skipRefOrNullp() ثابت VL_MT_STABLE {
        بازگشت skipRefIterp(true, true, false);
    }
    Ast_گره_DType* skipRefOrNullp() VL_MT_STABLE {
        بازگشت const_cast<Ast_گره_DType*>(
            static_cast<ثابت Ast_گره_DType*>(this)->skipRefIterp(true, true, false));
    }
    // (Slow) Recurse over MemberDType|ParamTypeDType|RefDType|EnumDType to ConstDType
    ثابت Ast_گره_DType* skipRefToConstp() ثابت { بازگشت skipRefIterp(false, true); }
    Ast_گره_DType* skipRefToConstp() {
        بازگشت const_cast<Ast_گره_DType*>(
            static_cast<ثابت Ast_گره_DType*>(this)->skipRefIterp(false, true));
    }
    // (Slow) Recurse over MemberDType|ParamTypeDType|RefDType|ConstDType to EnumDType
    ثابت Ast_گره_DType* skipRefToEnump() ثابت { بازگشت skipRefIterp(true, false); }
    Ast_گره_DType* skipRefToEnump() {
        بازگشت const_cast<Ast_گره_DType*>(
            static_cast<ثابت Ast_گره_DType*>(this)->skipRefIterp(true, false));
    }
    // (Slow) Recurse over MemberDType|ParamTypeDType|RefDType|ConstDType to EnumDType,
    // Returns null if not resolved
    ثابت Ast_گره_DType* skipRefToEnumOrNullp() ثابت { بازگشت skipRefIterp(true, false, false); }
    // (Slow) Recurse over MemberDType|ParamTypeDType|RefDType to other type
    ثابت Ast_گره_DType* skipRefToNonRefp() ثابت { بازگشت skipRefIterp(false, false); }
    Ast_گره_DType* skipRefToNonRefp() {
        بازگشت const_cast<Ast_گره_DType*>(
            static_cast<ثابت Ast_گره_DType*>(this)->skipRefIterp(false, false));
    }
    // If array, returns element dtype, otherwise returns skipRef dtype
    // If skipRef is false, RefDTypes are not followed (safe before typedef linking)
    ثابت Ast_گره_DType* elemDTypep(bool skipRef = true) ثابت VL_MT_STABLE;
    Ast_گره_DType* elemDTypep(bool skipRef = true) VL_MT_STABLE {
        بازگشت const_cast<Ast_گره_DType*>(
            static_cast<ثابت Ast_گره_DType*>(this)->elemDTypep(skipRef));
    }
    // (Slow) recurses - Structure alignment 1,2,4 or 8 bytes (arrays affect this)
    مجازی صحیح widthAlignBytes() ثابت = 0;
    // (Slow) recurses - Width in bytes rounding up 1,2,4,8,12,...
    مجازی صحیح widthTotalBytes() ثابت = 0;
    bool maybePointedTo() ثابت override VL_MT_SAFE { بازگشت true; }
    // Iff has a non-null refDTypep(), as generic node function
    مجازی Ast_گره_DType* virtRefDTypep() ثابت { بازگشت nullptr; }
    // Iff has refDTypep(), set as generic node function
    مجازی تهی virtRefDTypep(Ast_گره_DType* nodep) {}
    // Iff has a non-null second dtypep, as generic node function
    مجازی Ast_گره_DType* virtRefDType2p() ثابت { بازگشت nullptr; }
    // Iff has second dtype, set as generic node function
    مجازی تهی virtRefDType2p(Ast_گره_DType* nodep) {}
    // Assignable equivalence.  Calls skipRefToNonRefp() during comparisons.
    bool similarDType(ثابت Ast_گره_DType* samep) ثابت;
    // Iff has a non-null subDTypep(), as generic node function
    مجازی Ast_گره_DType* subDTypep() ثابت VL_MT_STABLE { بازگشت nullptr; }
    مجازی Ast_گره_DType* subDType2p() ثابت VL_MT_STABLE { بازگشت nullptr; }
    مجازی bool isAggregateType() ثابت { بازگشت false; }
    // True for unpacked, dynamic, queue, and associative arrays (not packed arrays)
    bool isNonPackedArray() ثابت;
    مجازی bool isFourstate() ثابت;
    // Ideally an IEEE $typename
    مجازی string prettyDTypeName(bool) ثابت { بازگشت prettyTypeName(); }
    string prettyDTypeNameQ(bool full = false) ثابت { بازگشت "'" + prettyDTypeName(full) + "'"; }
    string stateDTypeName() ثابت { بازگشت this->isFourstate() ? "(4-state)" : "(2-state)"; }
    //
    // Changing the width may confuse the data type resolution, so must clear
    // TypeTable cache after use.
    تهی widthForce(صحیح width, صحیح widthMin) {
        m_width = width;
        m_widthMin = widthMin;
    }
    // For backward compatibility inherit width and signing from the subDType/base type
    تهی widthFromSub(ثابت Ast_گره_DType* nodep) {
        m_width = nodep->m_width;
        m_widthMin = nodep->m_widthMin;
        m_numeric = nodep->m_numeric;
    }
    //
    صحیح width() ثابت VL_MT_STABLE { بازگشت m_width; }
    تهی numeric(VSigning flag) { m_numeric = flag; }
    bool isSigned() ثابت VL_MT_STABLE { بازگشت m_numeric.isSigned(); }
    bool isNosign() ثابت VL_MT_SAFE { بازگشت m_numeric.isNosign(); }
    VSigning numeric() ثابت VL_MT_STABLE { بازگشت m_numeric; }
    صحیح width_واژگان() ثابت VL_MT_STABLE { بازگشت VL_واژگان_I(width()); }
    صحیح widthMin() ثابت VL_MT_STABLE {  // If sized, the size,
                                         // if unsized the min digits to represent it
        بازگشت m_widthMin ? m_widthMin : m_width;
    }
    صحیح widthPow2() ثابت;
    تهی widthMinFromWidth() { m_widthMin = m_width; }
    bool widthSized() ثابت VL_MT_SAFE { بازگشت !m_widthMin || m_widthMin == m_width; }
    bool generic() ثابت VL_MT_SAFE { بازگشت m_generic; }
    تهی generic(bool flag) { m_generic = flag; }
    std::pair<uint32_t, uint32_t> dimensions(bool includeBasic) ثابت;
    uint32_t arrayUnpackedElements() ثابت;  // 1, or total multiplication of all dimensions
    // Fixed aggregate streaming properties
    bool isStreamableFixedAggregate() ثابت;
    // Whether is a string, or an unpacked array or structure containing one
    bool containsString() ثابت;
    bool containsUnpackedStruct() ثابت;
    صحیح widthStream() ثابت;
    string vlEnumType() ثابت;  // Return VerilatedVarType: VLVT_UINT32, etc
    string vlEnumPackedAgg() ثابت;  // Return VLVF_PACKED_AGG etc flags, or empty
    ایستا صحیح unique_رقم_Inc() { بازگشت ++s_unique_رقم; }
    ثابت نویسه* charIQWN() ثابت {
        بازگشت (isString() ? "N" : isWide() ? "W" : isDouble() ? "D" : isQuad() ? "Q" : "I");
    }
    string cType(ثابت string& name, bool forFunc, bool isRef, bool packed = false) ثابت;
    // Represents a C++ LiteralType? (can be constexpr)
    bool isLiteralType() ثابت VL_MT_STABLE;
    // C++ type (as emitted by cType) is trivially default constructible and destructible?
    bool isTriviallyConstructible() ثابت VL_MT_STABLE;
    مجازی bool isDynamicallySized() ثابت { بازگشت false; }

خصوصی:
    class CTypeRecursed;
    CTypeRecursed cTypeRecurse(bool compound, bool packed) ثابت;
};
class Ast_گره_Array_DType VL_NOT_FINAL : عمومی Ast_گره_DType {
    // Array data type, ie "some_dtype var_name [2:0]"
    // @astgen op1 := childDTypep : Optional[AstNodeDType] // moved to refDTypep() in V3Width
    // @astgen op2 := rangep : Optional[AstRange] // array bounds
    //
    // @astgen ptr := m_refDTypep : Optional[AstNodeDType]  // Elements of this type (post-width)
    Ast_گره* rangenp() ثابت { بازگشت reinterpret_cast<Ast_گره*>(rangep()); }

محافظتشده:
    Ast_گره_Array_DType(VNType t, File_سطر* fl)
        : Ast_گره_DType{t, fl} {}

عمومی:
    ASTGEN_MEMBERS_Ast_گره_Array_DType;
    تهی dump(std::ostream& str) ثابت override;
    تهی dumpJson(std::ostream& str) ثابت override;
    تهی dumpSmall(std::ostream& str) ثابت override;
    ثابت نویسه* broken() ثابت override {
        BROKEN_RTN(!((m_refDTypep && !childDTypep()) || (!m_refDTypep && childDTypep())));
        بازگشت nullptr;
    }
    bool same_گره(ثابت Ast_گره* samep) ثابت override {
        ثابت Ast_گره_Array_DType* ثابت asamep = VN_DBG_AS(samep, گره_Array_DType);
        بازگشت hi() == asamep->hi() && rangenp()->same_درخت(asamep->rangenp())
               && subDTypep() == asamep->subDTypep();
    }  // HashedDT doesn't recurse, so need to check children
    bool similar_DType_گره(ثابت Ast_گره_DType* samep) ثابت override {
        ثابت Ast_گره_Array_DType* ثابت asamep = VN_DBG_AS(samep, گره_Array_DType);
        بازگشت elementsConst() == asamep->elementsConst()
               && subDTypep()->similarDType(asamep->subDTypep());
    }
    Ast_گره_DType* getChildDTypep() ثابت override { بازگشت childDTypep(); }
    Ast_گره_DType* subDTypep() ثابت override VL_MT_STABLE {
        بازگشت m_refDTypep ? m_refDTypep : childDTypep();
    }
    تهی refDTypep(Ast_گره_DType* nodep) { m_refDTypep = nodep; }
    Ast_گره_DType* virtRefDTypep() ثابت override { بازگشت m_refDTypep; }
    تهی virtRefDTypep(Ast_گره_DType* nodep) override { refDTypep(nodep); }
    // METHODS
    AstBasicDType* basicp() ثابت override VL_MT_STABLE {
        بازگشت subDTypep()->basicp();
    }  // (Slow) recurse down to find basic data type
    صحیح widthAlignBytes() ثابت override { بازگشت subDTypep()->widthAlignBytes(); }
    صحیح widthTotalBytes() ثابت override {
        بازگشت elementsConst() * subDTypep()->widthTotalBytes();
    }
    درونخطی صحیح چپ() ثابت VL_MT_STABLE;
    درونخطی صحیح راست() ثابت VL_MT_STABLE;
    درونخطی صحیح hi() ثابت VL_MT_STABLE;
    درونخطی صحیح lo() ثابت VL_MT_STABLE;
    درونخطی صحیح elementsConst() ثابت VL_MT_STABLE;
    درونخطی VNumRange declRange() ثابت VL_MT_STABLE;
};
class Ast_گره_UOr_Struct_DType VL_NOT_FINAL : عمومی Ast_گره_DType {
    // A struct or union; common handling
    // @astgen op1 := membersp : List[AstMemberDType]
    //
    // @astgen ptr := m_classOrPackagep : Optional[AstNodeModule]  // Package emitted with
    string m_name;  // Name from upper typedef, if any
    string m_typedefName;  // Typedef's dtypeName() once moved off the typedef declaring it
    ثابت صحیح m_unique_رقم;  // Unique ID distinguishing this dtype instance, for hashing/naming
    bool m_packed;  // Packed struct/union, else unpacked
    bool m_isFourstate = false;  // V3Width computes; true if any member is 4-state
    bool m_constrainedRand = false;  // True if struct has constraint expression
    bool m_emitToString = false;  // Generate to_string() for this struct/union if set

محافظتشده:
    Ast_گره_UOr_Struct_DType(VNType t, File_سطر* fl, VSigning numericUnpack)
        : Ast_گره_DType{t, fl}
        , m_unique_رقم{unique_رقم_Inc()} {
        // VSigning::NOSIGN overloaded to indicate not packed
        m_packed = (numericUnpack != VSigning::NOSIGN);
        numeric(VSigning::fromBool(numericUnpack.isSigned()));
    }
    Ast_گره_UOr_Struct_DType(ثابت Ast_گره_UOr_Struct_DType& other)
        : Ast_گره_DType{other}
        , m_name{other.m_name}
        , m_typedefName{other.m_typedefName}
        , m_unique_رقم{unique_رقم_Inc()}
        , m_packed{other.m_packed}
        , m_isFourstate{other.m_isFourstate} {}

عمومی:
    ASTGEN_MEMBERS_Ast_گره_UOr_Struct_DType;
    صحیح unique_رقم() ثابت { بازگشت m_unique_رقم; }
    تهی dump(std::ostream& str) ثابت override;
    تهی dumpJson(std::ostream& str) ثابت override;
    string prettyDTypeName(bool) ثابت override;
    bool isCompound() ثابت override { بازگشت !packed(); }
    // For basicp() we reuse the size to indicate a "fake" basic type of same size
    AstBasicDType* basicp() ثابت override VL_MT_STABLE {
        اگر (!m_packed) بازگشت nullptr;
        بازگشت (isFourstate()
                    ? VN_AS(findLogicRangeDType(VNumRange{width() - 1, 0}, width(), numeric()),
                            BasicDType)
                    : VN_AS(findBitRangeDType(VNumRange{width() - 1, 0}, width(), numeric()),
                            BasicDType));
    }
    // (Slow) recurses - Structure alignment 1,2,4 or 8 bytes (arrays affect this)
    صحیح widthAlignBytes() ثابت override;
    // (Slow) recurses - Width in bytes rounding up 1,2,4,8,12,...
    صحیح widthTotalBytes() ثابت override;
    bool similar_DType_گره(ثابت Ast_گره_DType* samep) ثابت override;
    string name() ثابت override VL_MT_STABLE { بازگشت m_name; }
    تهی name(ثابت string& flag) override { m_name = flag; }
    // Keep the name the typedef declaring it gives, as it moves off it to the type table
    تهی typedefName(ثابت string& name) { m_typedefName = name; }
    // Name of the C structure for DPI, empty if no typedef names it, so declared where used
    string dpiName() ثابت;
    // C typedef of the unpacked structure for DPI (IEEE 1800-2023 H.7.3)
    string dpiTypedef() ثابت;
    bool packed() ثابت VL_MT_SAFE { بازگشت m_packed; }
    تهی packed(bool flag) { m_packed = flag; }
    // packed() but as don't support unpacked, presently all structs
    ایستا bool packedUnsup() { بازگشت true; }
    تهی isFourstate(bool flag) { m_isFourstate = flag; }
    bool isFourstate() ثابت override VL_MT_SAFE { بازگشت m_isFourstate; }
    ایستا صحیح lo() VL_MT_STABLE { بازگشت 0; }
    صحیح hi() ثابت VL_MT_STABLE {
        بازگشت dtypep()->width() - 1;
    }  // Packed classes look like arrays
    VNumRange declRange() ثابت VL_MT_STABLE { بازگشت VNumRange{hi(), lo()}; }
    Ast_گره_Module* classOrPackagep() ثابت { بازگشت m_classOrPackagep; }
    تهی classOrPackagep(Ast_گره_Module* classpackagep) { m_classOrPackagep = classpackagep; }
    bool isConstrainedRand() ثابت { بازگشت m_constrainedRand; }
    تهی markConstrainedRand(bool flag) { m_constrainedRand = flag; }
    bool emitToString() ثابت { بازگشت m_emitToString; }
    تهی setEmitToString() { m_emitToString = true; }
    bool isAggregateType() ثابت override { بازگشت !packed(); }
};

// === Concrete node types =====================================================

// === AstNode ===
class AstEnumItem final : عمومی Ast_گره {
    // @astgen op1 := rangep : Optional[AstRange] // Range for name appending
    // @astgen op2 := valuep : Optional[AstNodeExpr]
    string m_name;  // Name of item

عمومی:
    // Parents: ENUM
    AstEnumItem(File_سطر* fl, ثابت string& name, AstRange* rangep, Ast_گره_عبارت* valuep)
        : ASTGEN_SUPER_EnumItem(fl)
        , m_name{name} {
        this->rangep(rangep);
        this->valuep(valuep);
    }
    ASTGEN_MEMBERS_AstEnumItem;
    string name() ثابت override VL_MT_STABLE { بازگشت m_name; }
    bool maybePointedTo() ثابت override VL_MT_SAFE { بازگشت true; }
    bool hasDType() ثابت override VL_MT_SAFE { بازگشت true; }
    تهی name(ثابت string& flag) override { m_name = flag; }
};

// === AstNodeDType ===
class AstAssocArrayDType final : عمومی Ast_گره_DType {
    // Associative array data type, ie "[some_dtype]"
    // @astgen op1 := childDTypep : Optional[AstNodeDType] // moved to refDTypep() in V3Width
    // @astgen op2 := keyChildDTypep : Optional[AstNodeDType]
    //
    // @astgen ptr := m_refDTypep : Optional[AstNodeDType]  // Elements of this type (post-width)
    // @astgen ptr := m_keyDTypep : Optional[AstNodeDType]  // Keys of this type (post-width)
عمومی:
    AstAssocArrayDType(File_سطر* fl, VFlagChildDType, Ast_گره_DType* dtp, Ast_گره_DType* کلید_Dtp)
        : ASTGEN_SUPER_AssocArrayDType(fl) {
        childDTypep(dtp);  // Only for parser
        کلید_Child_DTypep(کلید_Dtp);  // Only for parser
        refDTypep(nullptr);
        کلید_DTypep(nullptr);
        dtypep(nullptr);  // V3Width will resolve
    }
    AstAssocArrayDType(File_سطر* fl, Ast_گره_DType* dtp, Ast_گره_DType* کلید_Dtp)
        : ASTGEN_SUPER_AssocArrayDType(fl) {
        refDTypep(dtp);
        کلید_DTypep(کلید_Dtp);
        dtypep(dtp);
    }
    ASTGEN_MEMBERS_AstAssocArrayDType;
    ثابت نویسه* broken() ثابت override {
        BROKEN_RTN(!((m_refDTypep && !childDTypep()) || (!m_refDTypep && childDTypep())));
        BROKEN_RTN(!((m_کلید_DTypep && !childDTypep()) || (!m_کلید_DTypep && childDTypep())));
        بازگشت nullptr;
    }
    bool same_گره(ثابت Ast_گره* samep) ثابت override {
        ثابت AstAssocArrayDType* ثابت asamep = VN_DBG_AS(samep, AssocArrayDType);
        اگر (!asamep->subDTypep()) بازگشت false;
        اگر (!asamep->کلید_DTypep()) بازگشت false;
        بازگشت (subDTypep() == asamep->subDTypep() && کلید_DTypep() == asamep->کلید_DTypep());
    }
    bool similar_DType_گره(ثابت Ast_گره_DType* samep) ثابت override {
        ثابت AstAssocArrayDType* ثابت asamep = VN_DBG_AS(samep, AssocArrayDType);
        بازگشت asamep->subDTypep() && subDTypep()->similarDType(asamep->subDTypep())
               && asamep->کلید_DTypep() && کلید_DTypep()->similarDType(asamep->کلید_DTypep());
    }
    string prettyDTypeName(bool full) ثابت override;
    تهی dumpSmall(std::ostream& str) ثابت override;
    Ast_گره_DType* getChildDTypep() ثابت override { بازگشت childDTypep(); }
    Ast_گره_DType* getChild2DTypep() ثابت override { بازگشت کلید_Child_DTypep(); }
    Ast_گره_DType* subDTypep() ثابت override VL_MT_STABLE {
        بازگشت m_refDTypep ? m_refDTypep : childDTypep();
    }
    Ast_گره_DType* subDType2p() ثابت override VL_MT_STABLE {
        بازگشت m_کلید_DTypep ? m_کلید_DTypep : کلید_Child_DTypep();
    }
    bool isAggregateType() ثابت override { بازگشت true; }
    تهی refDTypep(Ast_گره_DType* nodep) { m_refDTypep = nodep; }
    Ast_گره_DType* virtRefDTypep() ثابت override { بازگشت m_refDTypep; }
    تهی virtRefDTypep(Ast_گره_DType* nodep) override { refDTypep(nodep); }
    Ast_گره_DType* virtRefDType2p() ثابت override { بازگشت m_کلید_DTypep; }
    تهی virtRefDType2p(Ast_گره_DType* nodep) override { کلید_DTypep(nodep); }
    //
    Ast_گره_DType* کلید_DTypep() ثابت VL_MT_STABLE {
        بازگشت m_کلید_DTypep ? m_کلید_DTypep : کلید_Child_DTypep();
    }
    تهی کلید_DTypep(Ast_گره_DType* nodep) { m_کلید_DTypep = nodep; }
    // METHODS
    AstBasicDType* basicp() ثابت override VL_MT_STABLE { بازگشت nullptr; }
    صحیح widthAlignBytes() ثابت override { بازگشت subDTypep()->widthAlignBytes(); }
    صحیح widthTotalBytes() ثابت override { بازگشت subDTypep()->widthTotalBytes(); }
    bool isCompound() ثابت override { بازگشت true; }
    bool isDynamicallySized() ثابت override { بازگشت true; }
};
class AstBasicDType final : عمومی Ast_گره_DType {
    // Builtin atomic/vectored data type
    // @astgen op1 := rangep : Optional[AstRange] // Range of variable
    ساختار Members final {
        VBasicDTypeKwd m_keyword;  // (also in VBasicTypeKey) What keyword created basic type
        // dist-ast-dump-suppress  // Part of dumpSmall
        VNumRange m_nrange;  // (also in VBasicTypeKey) Numeric msb/lsb (if non-opaque keyword)
        bool operator==(ثابت Members& rhs) ثابت {
            بازگشت rhs.m_keyword == m_keyword && rhs.m_nrange == m_nrange;
        }
    } m;
    // See also in AstNodeDType: m_width, m_widthMin, m_numeric(issigned)
عمومی:
    AstBasicDType(File_سطر* fl, VBasicDTypeKwd kwd, ثابت VSigning& signst = VSigning::NOSIGN)
        : ASTGEN_SUPER_BasicDType(fl) {
        init(kwd, signst, 0, -1, nullptr);
    }
    AstBasicDType(File_سطر* fl, VFlagLogicPacked, صحیح wantwidth)
        : ASTGEN_SUPER_BasicDType(fl) {
        init(VBasicDTypeKwd::LOGIC, VSigning::NOSIGN, wantwidth, -1, nullptr);
    }
    AstBasicDType(File_سطر* fl, VFlagLogic2StatePacked, صحیح wantwidth)
        : ASTGEN_SUPER_BasicDType(fl) {
        init(VBasicDTypeKwd::LOGIC2STATE, VSigning::NOSIGN, wantwidth, -1, nullptr);
    }
    AstBasicDType(File_سطر* fl, VFlagBitPacked, صحیح wantwidth)
        : ASTGEN_SUPER_BasicDType(fl) {
        init(VBasicDTypeKwd::BIT, VSigning::NOSIGN, wantwidth, -1, nullptr);
    }
    AstBasicDType(File_سطر* fl, VBasicDTypeKwd kwd, VSigning numer, صحیح wantwidth, صحیح widthmin)
        : ASTGEN_SUPER_BasicDType(fl) {
        init(kwd, numer, wantwidth, widthmin, nullptr);
    }
    AstBasicDType(File_سطر* fl, VBasicDTypeKwd kwd, VSigning numer, VNumRange range, صحیح widthmin)
        : ASTGEN_SUPER_BasicDType(fl) {
        init(kwd, numer, range.elements(), widthmin, nullptr);
        m.m_nrange = range;  // as init() presumes lsb==0, but range.lsb() might not be
    }
    // See also addRange in verilog.y
خصوصی:
    تهی init(VBasicDTypeKwd kwd, VSigning numer, صحیح wantwidth, صحیح wantwidthmin,
              AstRange* rangep);

عمومی:
    ASTGEN_MEMBERS_AstBasicDType;
    تهی dump(std::ostream& str) ثابت override;
    تهی dumpJson(std::ostream& str) ثابت override;
    // width/widthMin/numeric compared elsewhere
    bool same_گره(ثابت Ast_گره* samep) ثابت override;
    bool similar_DType_گره(ثابت Ast_گره_DType* samep) ثابت override;
    string name() ثابت override VL_MT_STABLE { بازگشت m.m_keyword.typeName(); }
    string prettyDTypeName(bool full) ثابت override;
    ثابت نویسه* broken() ثابت override {
        BROKEN_RTN(dtypep() != this);
        BROKEN_RTN(v3Global.widthMinUsage() == VWidthMinUsage::VERILOG_WIDTH
                   && widthMin() > width());
        بازگشت nullptr;
    }
    تهی setSignedState(ثابت VSigning& signst) {
        // Note NOSIGN does NOT change the state; this is required by the parser
        اگر (signst == VSigning::UNSIGNED) {
            numeric(signst);
        } وگرنه اگر (signst == VSigning::SIGNED) {
            numeric(signst);
        }
    }
    // METHODS
    AstBasicDType* basicp() ثابت override VL_MT_STABLE { بازگشت (AstBasicDType*)this; }
    // (Slow) recurses - Structure alignment 1,2,4 or 8 bytes (arrays affect this)
    صحیح widthAlignBytes() ثابت override;
    // (Slow) recurses - Width in bytes rounding up 1,2,4,8,12,...
    صحیح widthTotalBytes() ثابت override;
    bool isFourstate() ثابت override { بازگشت keyword().isFourstate(); }
    VBasicDTypeKwd keyword() ثابت VL_MT_SAFE {  // Avoid using - use isSomething accessors instead
        بازگشت m.m_keyword;
    }
    bool isBitLogic() ثابت { بازگشت keyword().isBitLogic(); }
    bool isBit() ثابت { بازگشت keyword().isBit(); }
    bool isCHandle() ثابت VL_MT_STABLE { بازگشت keyword().isCHandle(); }
    bool isDouble() ثابت VL_MT_STABLE { بازگشت keyword().isDouble(); }
    bool isEvent() ثابت VL_MT_STABLE { بازگشت keyword() == VBasicDTypeKwd::EVENT; }
    bool isForkSync() ثابت VL_MT_SAFE { بازگشت keyword() == VBasicDTypeKwd::FORK_SYNC; }
    bool isProcessRef() ثابت VL_MT_SAFE { بازگشت keyword() == VBasicDTypeKwd::PROCESS_REFERENCE; }
    bool isDelayScheduler() ثابت VL_MT_SAFE {
        بازگشت keyword() == VBasicDTypeKwd::DELAY_SCHEDULER;
    }
    bool isTriggerScheduler() ثابت VL_MT_SAFE {
        بازگشت keyword() == VBasicDTypeKwd::TRIGGER_SCHEDULER;
    }
    bool isDynamicTriggerScheduler() ثابت VL_MT_SAFE {
        بازگشت keyword() == VBasicDTypeKwd::DYNAMIC_TRIGGER_SCHEDULER;
    }
    bool isRandomGenerator() ثابت VL_MT_SAFE {
        بازگشت keyword() == VBasicDTypeKwd::RANDOM_GENERATOR;
    }
    bool isStdRandomGenerator() ثابت VL_MT_SAFE {
        بازگشت keyword() == VBasicDTypeKwd::RANDOM_STDGENERATOR;
    }
    bool isCovergroupInstHandle() ثابت VL_MT_SAFE {
        بازگشت keyword() == VBasicDTypeKwd::COVERGROUP_INSTHANDLE;
    }
    bool isOpaque() ثابت VL_MT_SAFE { بازگشت keyword().isOpaque(); }
    bool isString() ثابت VL_MT_STABLE { بازگشت keyword().isString(); }
    bool isZeroInit() ثابت { بازگشت keyword().isZeroInit(); }
    bool isRanged() ثابت { بازگشت rangep() || m.m_nrange.ranged(); }
    bool isDpiBitVec() ثابت {  // DPI uses svBitVecVal
        بازگشت keyword() == VBasicDTypeKwd::BIT && isRanged();
    }
    bool isDpiLogicVec() ثابت {  // DPI uses svLogicVecVal
        بازگشت keyword().isFourstate() && !(keyword() == VBasicDTypeKwd::LOGIC && !isRanged());
    }
    bool isDpiCLayout() ثابت {
        بازگشت keyword().isDpiCLayout()
               || (keyword().isSameish(VBasicDTypeKwd::LOGIC) && !isRanged());
    }
    bool isDpiPrimitive() ثابت {  // DPI uses a primitive type
        بازگشت !isDpiBitVec() && !isDpiLogicVec();
    }
    // Generally the lo/hi/left/right funcs should be used instead of nrange()
    ثابت VNumRange& nrange() ثابت { بازگشت m.m_nrange; }
    درونخطی صحیح hi() ثابت;
    درونخطی صحیح lo() ثابت;
    درونخطی صحیح elements() ثابت;
    صحیح چپ() ثابت { بازگشت ascending() ? lo() : hi(); }  // How to show a declaration
    صحیح راست() ثابت { بازگشت ascending() ? hi() : lo(); }
    درونخطی bool ascending() ثابت;
    bool implicit() ثابت { بازگشت keyword() == VBasicDTypeKwd::LOGIC_IMPLICIT; }
    bool untyped() ثابت { بازگشت keyword() == VBasicDTypeKwd::UNTYPED; }
    VNumRange declRange() ثابت { بازگشت isRanged() ? VNumRange{چپ(), راست()} : VNumRange{}; }
    تهی cvtRangeConst();  // Convert to smaller representation
    bool isCompound() ثابت override { بازگشت isString(); }
    bool isIntegralOrPacked() ثابت override { بازگشت keyword().isIntNumeric(); }
};
class AstBracketArrayDType final : عمومی Ast_گره_DType {
    // Associative/Queue/Normal array data type, ie "[dtype_or_expr]"
    // only for early parsing then becomes another data type
    // @astgen op1 := childDTypep : Optional[AstNodeDType] // moved to refDTypep() in V3Width
    // @astgen op2 := elementsp : AstNode  // Number of elements in array
عمومی:
    AstBracketArrayDType(File_سطر* fl, VFlagChildDType, Ast_گره_DType* childDTypep,
                         Ast_گره* elementsp)
        : ASTGEN_SUPER_BracketArrayDType(fl) {
        this->childDTypep(childDTypep);
        this->elementsp(elementsp);
    }
    ASTGEN_MEMBERS_AstBracketArrayDType;
    bool similar_DType_گره(ثابت Ast_گره_DType* samep) ثابت override { بازگشت this == samep; }
    Ast_گره_DType* subDTypep() ثابت override VL_MT_STABLE { بازگشت childDTypep(); }
    // METHODS
    // Will be removed in V3Width, which relies on this
    // being a child not a dtype pointed node
    bool maybePointedTo() ثابت override VL_MT_SAFE { بازگشت false; }
    AstBasicDType* basicp() ثابت override VL_MT_STABLE { بازگشت nullptr; }
    صحیح widthAlignBytes() ثابت override { V3ERROR_NA_RETURN(0); }
    صحیح widthTotalBytes() ثابت override { V3ERROR_NA_RETURN(0); }
    bool isCompound() ثابت override { بازگشت true; }
};
class AstCDType final : عمومی Ast_گره_DType {
    // Raw "C" data type passed directly to output
    string m_name;  // Name of data type, printed when do V3EmitC
عمومی:
    AstCDType(File_سطر* fl, ثابت string& name)
        : ASTGEN_SUPER_CDType(fl)
        , m_name{name} {
        dtypep(this);
    }

    ASTGEN_MEMBERS_AstCDType;
    bool same_گره(ثابت Ast_گره* samep) ثابت override {
        ثابت AstCDType* ثابت asamep = VN_DBG_AS(samep, CDType);
        بازگشت m_name == asamep->m_name;
    }
    bool similar_DType_گره(ثابت Ast_گره_DType* samep) ثابت override { بازگشت same_گره(samep); }
    string name() ثابت override VL_MT_STABLE { بازگشت m_name; }
    string prettyDTypeName(bool) ثابت override { بازگشت m_name; }
    // METHODS
    AstBasicDType* basicp() ثابت override VL_MT_STABLE { بازگشت nullptr; }
    صحیح widthAlignBytes() ثابت override { بازگشت 8; }  // Assume
    صحیح widthTotalBytes() ثابت override { بازگشت 8; }  // Assume
    bool isCompound() ثابت override { بازگشت true; }
    // Return the global runtime type storing the requested number of bits.
    ایستا string typeToHold(صحیح width) {
        اگر (width <= 8) بازگشت "::CData";
        اگر (width <= 16) بازگشت "::SData";
        اگر (width <= VL_IDATASIZE) بازگشت "::IData";
        اگر (width <= VL_QUADSIZE) بازگشت "::QData";

        بازگشت "::VlWide<" + std::to_string(VL_واژگان_I(width)) + ">";
    }
};
class AstClassRefDType final : عمومی Ast_گره_DType {
    // Reference to a class
    // @astgen op1 := paramsp: List[AstPin]
    //
    // @astgen ptr := m_classp : Optional[AstClass]  // data type pointed to, BELOW the AstTypedef
    // @astgen ptr := m_classOrPackagep : Optional[AstNodeModule]  // Package hierarchy
    bool m_rawPointer = false;  // Emit as a non-owning C++ pointer rather than VlClassRef
عمومی:
    AstClassRefDType(File_سطر* fl, AstClass* classp, AstPin* paramsp)
        : ASTGEN_SUPER_ClassRefDType(fl)
        , m_classp{classp} {
        dtypep(this);
        addParamsp(paramsp);
    }
    ASTGEN_MEMBERS_AstClassRefDType;
    // METHODS
    bool same_گره(ثابت Ast_گره* samep) ثابت override {
        ثابت AstClassRefDType* ثابت asamep = VN_DBG_AS(samep, ClassRefDType);
        بازگشت (m_classp == asamep->m_classp && m_classOrPackagep == asamep->m_classOrPackagep
                && m_rawPointer == asamep->m_rawPointer);
    }
    bool similar_DType_گره(ثابت Ast_گره_DType* samep) ثابت override;
    تهی dump(std::ostream& str = std::cout) ثابت override;
    تهی dumpJson(std::ostream& str = std::cout) ثابت override;
    تهی dumpSmall(std::ostream& str) ثابت override;
    string prettyDTypeName(bool full) ثابت override;
    string pretty_Name_پیام() ثابت override;
    string name() ثابت override VL_MT_STABLE;
    AstBasicDType* basicp() ثابت override VL_MT_STABLE { بازگشت nullptr; }
    صحیح widthAlignBytes() ثابت override { بازگشت 0; }
    صحیح widthTotalBytes() ثابت override { بازگشت 0; }
    Ast_گره_DType* virtRefDTypep() ثابت override { بازگشت nullptr; }
    تهی virtRefDTypep(Ast_گره_DType* nodep) override {}
    Ast_گره_DType* subDTypep() ثابت override VL_MT_STABLE { بازگشت nullptr; }
    Ast_گره_Module* classOrPackagep() ثابت { بازگشت m_classOrPackagep; }
    تهی classOrPackagep(Ast_گره_Module* nodep) { m_classOrPackagep = nodep; }
    AstClass* classp() ثابت VL_MT_STABLE { بازگشت m_classp; }
    تهی classp(AstClass* nodep) { m_classp = nodep; }
    bool rawPointer() ثابت { بازگشت m_rawPointer; }
    تهی rawPointer(bool flag) { m_rawPointer = flag; }
    ایستا تهی selfTest();
    bool isCompound() ثابت override { بازگشت true; }
};
class AstConstDType final : عمومی Ast_گره_DType {
    // const data type, ie "const some_dtype var_name [2:0]"
    // ConstDType are removed in V3LinkLValue and become AstVar::isConst.
    // When more generic types are supported AstConstDType will be propagated further.
    // @astgen op1 := childDTypep : Optional[AstNodeDType]
    //
    // @astgen ptr := m_refDTypep : Optional[AstNodeDType]  // Inherit from this base data type
عمومی:
    AstConstDType(File_سطر* fl, VFlagChildDType, Ast_گره_DType* dtp)
        : ASTGEN_SUPER_ConstDType(fl) {
        childDTypep(dtp);  // Only for parser
        refDTypep(nullptr);  // V3Width will resolve
        dtypep(nullptr);  // V3Width will resolve
        widthFromSub(subDTypep());
    }
    ASTGEN_MEMBERS_AstConstDType;
    ثابت نویسه* broken() ثابت override {
        BROKEN_RTN(!((m_refDTypep && !childDTypep()) || (!m_refDTypep && childDTypep())));
        بازگشت nullptr;
    }
    bool same_گره(ثابت Ast_گره* samep) ثابت override {
        ثابت AstConstDType* ثابت sp = VN_DBG_AS(samep, ConstDType);
        بازگشت (m_refDTypep == sp->m_refDTypep);
    }
    bool similar_DType_گره(ثابت Ast_گره_DType* samep) ثابت override {
        بازگشت skipRefp()->similarDType(samep->skipRefp());
    }
    Ast_گره_DType* getChildDTypep() ثابت override { بازگشت childDTypep(); }
    Ast_گره_DType* subDTypep() ثابت override VL_MT_STABLE {
        بازگشت m_refDTypep ? m_refDTypep : childDTypep();
    }
    تهی refDTypep(Ast_گره_DType* nodep) { m_refDTypep = nodep; }
    Ast_گره_DType* virtRefDTypep() ثابت override { بازگشت m_refDTypep; }
    تهی virtRefDTypep(Ast_گره_DType* nodep) override { refDTypep(nodep); }
    // METHODS
    AstBasicDType* basicp() ثابت override VL_MT_STABLE { بازگشت subDTypep()->basicp(); }
    صحیح widthAlignBytes() ثابت override { بازگشت subDTypep()->widthAlignBytes(); }
    صحیح widthTotalBytes() ثابت override { بازگشت subDTypep()->widthTotalBytes(); }
    bool isCompound() ثابت override {
        v3fatalSrc("call isCompound on subdata type, not reference");
        بازگشت false;
    }
};
class AstConstraintRefDType final : عمومی Ast_گره_DType {
    // For e.g. a reference to constraint for constraint_mode
عمومی:
    explicit AstConstraintRefDType(File_سطر* fl)
        : ASTGEN_SUPER_ConstraintRefDType(fl) {
        dtypep(this);
    }
    ASTGEN_MEMBERS_AstConstraintRefDType;
    bool hasDType() ثابت override VL_MT_SAFE { بازگشت true; }
    bool maybePointedTo() ثابت override VL_MT_SAFE { بازگشت true; }
    bool undead() ثابت override { بازگشت true; }
    Ast_گره_DType* subDTypep() ثابت override VL_MT_STABLE { بازگشت nullptr; }
    Ast_گره_DType* virtRefDTypep() ثابت override { بازگشت nullptr; }
    تهی virtRefDTypep(Ast_گره_DType* nodep) override {}
    bool similar_DType_گره(ثابت Ast_گره_DType* samep) ثابت override { بازگشت this == samep; }
    AstBasicDType* basicp() ثابت override VL_MT_STABLE { بازگشت nullptr; }
    صحیح widthAlignBytes() ثابت override { بازگشت 1; }
    صحیح widthTotalBytes() ثابت override { بازگشت 1; }
    bool isCompound() ثابت override { بازگشت false; }
};
class AstCoverCrossDType final : عمومی Ast_گره_DType {
    // Borrowed pointer to VlCoverCrossT<...> or construction-time VlCoverCrossDyn.
    ثابت uint32_t m_dimensions;
    ثابت uint32_t m_tuples;  // Fixed capacity number of cross tuples (VlCoverCrossT's Tuples)
    ثابت uint32_t m_bins;  // Fixed capacity number of explicit bins (VlCoverCrossT's Bins)
    ثابت uint32_t m_autoBins;  // Fixed capacity number of auto bins (VlCoverCrossT's AutoBins)
    ثابت uint64_t m_bin_واژگان;  // Fixed capacity selection words (VlCoverCrossT's BinWords)
    ثابت bool m_dynamic;  // Bin layout is determined at covergroup construction

عمومی:
    AstCoverCrossDType(File_سطر* fl, uint32_t dimensions, uint32_t tuples, uint32_t bins,
                       uint32_t autoBins, uint64_t bin_واژگان, bool dynamic = false)
        : ASTGEN_SUPER_CoverCrossDType(fl)
        , m_dimensions{dimensions}
        , m_tuples{tuples}
        , m_bins{bins}
        , m_autoBins{autoBins}
        , m_bin_واژگان{bin_واژگان}
        , m_dynamic{dynamic} {
        dtypep(this);
    }
    ASTGEN_MEMBERS_AstCoverCrossDType;
    ثابت نویسه* broken() ثابت override {
        BROKEN_RTN(m_dimensions == 0);
        بازگشت nullptr;
    }
    bool same_گره(ثابت Ast_گره* samep) ثابت override {  // LCOV_EXCL_START
        ثابت AstCoverCrossDType* ثابت sp = VN_DBG_AS(samep, CoverCrossDType);
        بازگشت dimensions() == sp->dimensions() && tuples() == sp->tuples() && bins() == sp->bins()
               && autoBins() == sp->autoBins() && bin_واژگان() == sp->bin_واژگان()
               && isDynamic() == sp->isDynamic();
    }  // LCOV_EXCL_STOP
    bool similar_DType_گره(ثابت Ast_گره_DType* samep) ثابت override { بازگشت this == samep; }
    تهی dump(std::ostream& str) ثابت override;
    تهی dumpJson(std::ostream& str) ثابت override;
    تهی dumpSmall(std::ostream& str) ثابت override;
    uint32_t dimensions() ثابت { بازگشت m_dimensions; }
    uint32_t tuples() ثابت { بازگشت m_tuples; }
    uint32_t bins() ثابت { بازگشت m_bins; }
    uint32_t autoBins() ثابت { بازگشت m_autoBins; }
    uint64_t bin_واژگان() ثابت { بازگشت m_bin_واژگان; }
    bool isDynamic() ثابت { بازگشت m_dynamic; }
    string cppTemplateArgs() ثابت;
    AstBasicDType* basicp() ثابت override VL_MT_STABLE { بازگشت nullptr; }
    صحیح widthAlignBytes() ثابت override { بازگشت اندازه(تهی*); }
    صحیح widthTotalBytes() ثابت override { بازگشت اندازه(تهی*); }
    bool isCompound() ثابت override { بازگشت true; }
};
class AstCoverpointDType final : عمومی Ast_گره_DType {
    // Borrowed pointer to a covergroup coverpoint runtime, 'VlCoverpointT<hitBound>*'.
    // Follows pattern of AstQueueDType in capturing the compile-time max bin overlap
    // template argument.
    uint32_t m_hitBound;  // VlCoverpointT<> template argument; hit list size, >= 1
عمومی:
    AstCoverpointDType(File_سطر* fl, uint32_t hitBound)
        : ASTGEN_SUPER_CoverpointDType(fl)
        , m_hitBound{hitBound} {
        dtypep(this);
    }
    ASTGEN_MEMBERS_AstCoverpointDType;
    ثابت نویسه* broken() ثابت override {
        BROKEN_RTN(m_hitBound < 1);
        بازگشت nullptr;
    }
    // V3Covergroup interns these one-per-hitBound into the type table, so identity is
    // equality; there is never a second node with the same bound to compare against.
    bool similar_DType_گره(ثابت Ast_گره_DType* samep) ثابت override { بازگشت this == samep; }
    تهی dumpSmall(std::ostream& str) ثابت override;
    // ACCESSORS
    uint32_t hitBound() ثابت { بازگشت m_hitBound; }
    // METHODS
    AstBasicDType* basicp() ثابت override VL_MT_STABLE { بازگشت nullptr; }
    صحیح widthAlignBytes() ثابت override { بازگشت اندازه(تهی*); }
    صحیح widthTotalBytes() ثابت override { بازگشت اندازه(تهی*); }
    bool isCompound() ثابت override { بازگشت true; }
};
class AstDefImplicitDType final : عمومی Ast_گره_DType {
    // For parsing enum/struct/unions that are declared with a variable rather than typedef
    // This allows "var enum {...} a,b" to share the enum definition for both variables
    // After link, these become typedefs
    // @astgen op1 := childDTypep : Optional[AstNodeDType]
    string m_name;  // Data type name
    ثابت صحیح m_unique_رقم;  // Unique ID distinguishing this dtype instance, for hashing/naming

عمومی:
    AstDefImplicitDType(File_سطر* fl, ثابت string& name, VFlagChildDType, Ast_گره_DType* dtp)
        : ASTGEN_SUPER_DefImplicitDType(fl)
        , m_name{name}
        , m_unique_رقم{unique_رقم_Inc()} {
        childDTypep(dtp);  // Only for parser
        dtypep(nullptr);  // V3Width will resolve
    }
    AstDefImplicitDType(ثابت AstDefImplicitDType& other)
        : Ast_گره_DType(other)
        , m_name{other.m_name}
        , m_unique_رقم{unique_رقم_Inc()} {}
    ASTGEN_MEMBERS_AstDefImplicitDType;
    صحیح unique_رقم() ثابت { بازگشت m_unique_رقم; }
    bool same_گره(ثابت Ast_گره* samep) ثابت override {
        ثابت AstDefImplicitDType* ثابت sp = VN_DBG_AS(samep, DefImplicitDType);
        بازگشت unique_رقم() == sp->unique_رقم();
    }
    bool similar_DType_گره(ثابت Ast_گره_DType* samep) ثابت override { بازگشت same_گره(samep); }
    Ast_گره_DType* getChildDTypep() ثابت override { بازگشت childDTypep(); }
    Ast_گره_DType* subDTypep() ثابت override VL_MT_STABLE {
        بازگشت dtypep() ? dtypep() : childDTypep();
    }
    // METHODS
    // op1 = Range of variable
    Ast_گره_DType* dtypeSkipRefp() ثابت { بازگشت dtypep()->skipRefp(); }
    AstBasicDType* basicp() ثابت override VL_MT_STABLE { بازگشت subDTypep()->basicp(); }
    صحیح widthAlignBytes() ثابت override { بازگشت dtypep()->widthAlignBytes(); }
    صحیح widthTotalBytes() ثابت override { بازگشت dtypep()->widthTotalBytes(); }
    string name() ثابت override VL_MT_STABLE { بازگشت m_name; }
    تهی name(ثابت string& flag) override { m_name = flag; }
    bool isCompound() ثابت override { بازگشت false; }
};
class AstDynArrayDType final : عمومی Ast_گره_DType {
    // Dynamic array data type, ie "[]"
    // @astgen op1 := childDTypep : Optional[AstNodeDType] // moved to refDTypep() in V3Width
    //
    // @astgen ptr := m_refDTypep : Optional[AstNodeDType]  // Elements of this type (post-width)
عمومی:
    AstDynArrayDType(File_سطر* fl, VFlagChildDType, Ast_گره_DType* dtp)
        : ASTGEN_SUPER_DynArrayDType(fl) {
        childDTypep(dtp);  // Only for parser
        refDTypep(nullptr);
        dtypep(nullptr);  // V3Width will resolve
    }
    AstDynArrayDType(File_سطر* fl, Ast_گره_DType* dtp)
        : ASTGEN_SUPER_DynArrayDType(fl) {
        refDTypep(dtp);
        dtypep(nullptr);  // V3Width will resolve
    }
    ASTGEN_MEMBERS_AstDynArrayDType;
    ثابت نویسه* broken() ثابت override {
        BROKEN_RTN(!((m_refDTypep && !childDTypep()) || (!m_refDTypep && childDTypep())));
        بازگشت nullptr;
    }
    bool same_گره(ثابت Ast_گره* samep) ثابت override {
        ثابت AstDynArrayDType* ثابت asamep = VN_DBG_AS(samep, DynArrayDType);
        اگر (!asamep->subDTypep()) بازگشت false;
        بازگشت subDTypep() == asamep->subDTypep();
    }
    bool similar_DType_گره(ثابت Ast_گره_DType* samep) ثابت override {
        ثابت AstDynArrayDType* ثابت asamep = VN_DBG_AS(samep, DynArrayDType);
        بازگشت asamep->subDTypep() && subDTypep()->similarDType(asamep->subDTypep());
    }
    string prettyDTypeName(bool full) ثابت override;
    تهی dumpSmall(std::ostream& str) ثابت override;
    Ast_گره_DType* getChildDTypep() ثابت override { بازگشت childDTypep(); }
    Ast_گره_DType* subDTypep() ثابت override VL_MT_STABLE {
        بازگشت m_refDTypep ? m_refDTypep : childDTypep();
    }
    bool isAggregateType() ثابت override { بازگشت true; }
    تهی refDTypep(Ast_گره_DType* nodep) { m_refDTypep = nodep; }
    Ast_گره_DType* virtRefDTypep() ثابت override { بازگشت m_refDTypep; }
    تهی virtRefDTypep(Ast_گره_DType* nodep) override { refDTypep(nodep); }
    // METHODS
    AstBasicDType* basicp() ثابت override VL_MT_STABLE { بازگشت nullptr; }
    صحیح widthAlignBytes() ثابت override { بازگشت subDTypep()->widthAlignBytes(); }
    صحیح widthTotalBytes() ثابت override { بازگشت subDTypep()->widthTotalBytes(); }
    bool isCompound() ثابت override { بازگشت true; }
    bool isDynamicallySized() ثابت override { بازگشت true; }
};
class AstEmptyQueueDType final : عمومی Ast_گره_DType {
    // For EmptyQueue
عمومی:
    explicit AstEmptyQueueDType(File_سطر* fl)
        : ASTGEN_SUPER_EmptyQueueDType(fl) {
        dtypep(this);
    }
    ASTGEN_MEMBERS_AstEmptyQueueDType;
    تهی dumpSmall(std::ostream& str) ثابت override;
    bool hasDType() ثابت override VL_MT_SAFE { بازگشت true; }
    bool maybePointedTo() ثابت override VL_MT_SAFE { بازگشت true; }
    bool undead() ثابت override { بازگشت true; }
    Ast_گره_DType* subDTypep() ثابت override VL_MT_STABLE { بازگشت nullptr; }
    Ast_گره_DType* virtRefDTypep() ثابت override { بازگشت nullptr; }
    تهی virtRefDTypep(Ast_گره_DType* nodep) override {}
    bool similar_DType_گره(ثابت Ast_گره_DType* samep) ثابت override { بازگشت this == samep; }
    AstBasicDType* basicp() ثابت override VL_MT_STABLE { بازگشت nullptr; }
    صحیح widthAlignBytes() ثابت override { بازگشت 1; }
    صحیح widthTotalBytes() ثابت override { بازگشت 1; }
    bool isCompound() ثابت override { بازگشت false; }
};
class AstEnumDType final : عمومی Ast_گره_DType {
    // Parents: TYPEDEF/MODULE
    // @astgen op1 := childDTypep : Optional[AstNodeDType]
    // @astgen op2 := itemsp : List[AstEnumItem]
    //
    // @astgen ptr := m_refDTypep : Optional[AstNodeDType]  // Elements of this type (post-width)
    string m_name;  // Name from upper typedef, if any
    string m_typedefName;  // Typedef's dtypeName() once moved off the typedef declaring it
    ثابت صحیح m_unique_رقم;  // Unique ID distinguishing this dtype instance, for hashing/naming

عمومی:
    AstEnumDType(File_سطر* fl, VFlagChildDType, Ast_گره_DType* dtp, AstEnumItem* itemsp)
        : ASTGEN_SUPER_EnumDType(fl)
        , m_unique_رقم{unique_رقم_Inc()} {
        childDTypep(dtp);  // Only for parser
        refDTypep(nullptr);
        addItemsp(itemsp);
        dtypep(nullptr);  // V3Width will resolve
        widthFromSub(subDTypep());
    }
    AstEnumDType(ثابت AstEnumDType& other)
        : Ast_گره_DType{other}
        , m_name{other.m_name}
        , m_typedefName{other.m_typedefName}
        , m_unique_رقم{unique_رقم_Inc()} {}
    ASTGEN_MEMBERS_AstEnumDType;

    ثابت نویسه* broken() ثابت override;
    صحیح unique_رقم() ثابت { بازگشت m_unique_رقم; }
    bool same_گره(ثابت Ast_گره* samep) ثابت override {
        ثابت AstEnumDType* ثابت sp = VN_DBG_AS(samep, EnumDType);
        بازگشت unique_رقم() == sp->unique_رقم();
    }
    bool similar_DType_گره(ثابت Ast_گره_DType* samep) ثابت override { بازگشت same_گره(samep); }
    Ast_گره_DType* getChildDTypep() ثابت override { بازگشت childDTypep(); }
    Ast_گره_DType* subDTypep() ثابت override VL_MT_STABLE {
        بازگشت m_refDTypep ? m_refDTypep : childDTypep();
    }
    تهی refDTypep(Ast_گره_DType* nodep) { m_refDTypep = nodep; }
    Ast_گره_DType* virtRefDTypep() ثابت override { بازگشت m_refDTypep; }
    تهی virtRefDTypep(Ast_گره_DType* nodep) override { refDTypep(nodep); }
    string name() ثابت override VL_MT_STABLE { بازگشت m_name; }
    تهی name(ثابت string& flag) override { m_name = flag; }
    // Keep the name the typedef declaring it gives, as it moves off it to the type table
    تهی typedefName(ثابت string& name) { m_typedefName = name; }
    تهی dump(std::ostream& str = std::cout) ثابت override;
    تهی dumpJson(std::ostream& str = std::cout) ثابت override;
    تهی dumpSmall(std::ostream& str) ثابت override;
    string prettyDTypeName(bool full) ثابت override;
    // METHODS
    AstBasicDType* basicp() ثابت override VL_MT_STABLE { بازگشت subDTypep()->basicp(); }
    صحیح widthAlignBytes() ثابت override { بازگشت subDTypep()->widthAlignBytes(); }
    صحیح widthTotalBytes() ثابت override { بازگشت subDTypep()->widthTotalBytes(); }
    size_t item_شمار() ثابت {
        size_t شمار = 0;
        برای (Ast_گره* itemp = itemsp(); itemp; itemp = itemp->nextp()) ++شمار;
        بازگشت شمار;
    }
    bool isCompound() ثابت override { بازگشت false; }
};

class AstIfaceGenericDType final : عمومی Ast_گره_DType {
    // Generic interface that will be replaced with AstIfaceRefDType
    File_سطر* m_modportFileline;  // Where modport token was
    // dist-ast-dump-suppress  // Part of name()
    string m_modportName;  // "" = no modport
عمومی:
    explicit AstIfaceGenericDType(File_سطر* fl)
        : ASTGEN_SUPER_IfaceGenericDType(fl) {
        dtypep(this);
    }
    AstIfaceGenericDType(File_سطر* fl, File_سطر* modportFl, ثابت string& modport)
        : ASTGEN_SUPER_IfaceGenericDType(fl)
        , m_modportFileline{modportFl}
        , m_modportName{modport} {
        dtypep(this);
    }
    ASTGEN_MEMBERS_AstIfaceGenericDType;
    تهی dumpSmall(std::ostream& str) ثابت override;
    bool hasDType() ثابت override VL_MT_SAFE { بازگشت true; }
    bool maybePointedTo() ثابت override VL_MT_SAFE { بازگشت true; }
    bool undead() ثابت override { بازگشت true; }
    Ast_گره_DType* subDTypep() ثابت override VL_MT_STABLE { بازگشت nullptr; }
    Ast_گره_DType* virtRefDTypep() ثابت override { بازگشت nullptr; }
    تهی virtRefDTypep(Ast_گره_DType* nodep) override {}
    bool similar_DType_گره(ثابت Ast_گره_DType* samep) ثابت override { بازگشت this == samep; }
    AstBasicDType* basicp() ثابت override VL_MT_STABLE { بازگشت nullptr; }
    صحیح widthAlignBytes() ثابت override { بازگشت 1; }
    صحیح widthTotalBytes() ثابت override { بازگشت 1; }
    string modportName() ثابت { بازگشت m_modportName; }
    bool isModport() { بازگشت !m_modportName.empty(); }
    bool isCompound() ثابت override { بازگشت true; }
    File_سطر* modportFileline() ثابت { بازگشت m_modportFileline; }
    string name() ثابت override { بازگشت m_modportName; }
};

class AstIfaceRefDType final : عمومی Ast_گره_DType {
    // Reference to an interface, either for a port, or inside parent cell
    // @astgen op1 := paramsp : List[AstPin]
    //
    // @astgen ptr := m_ifacep : Optional[AstIface]  // Interface; cellp() should override
    // @astgen ptr := m_cellp : Optional[AstCell]  // When exact parent cell known; not a guess
    // @astgen ptr := m_modportp : Optional[AstModport]  // nullptr = unlinked or no modport
    File_سطر* m_modportFileline;  // Where modport token was
    string m_cellName;  // "" = no cell, such as when connects to 'input' iface
    string m_ifaceName;  // Interface name
    string m_modportName;  // "" = no modport
    bool m_virtual = false;  // True if virtual interface
عمومی:
    AstIfaceRefDType(File_سطر* fl, ثابت string& cellName, ثابت string& ifaceName)
        : ASTGEN_SUPER_IfaceRefDType(fl)
        , m_modportFileline{nullptr}
        , m_cellName{cellName}
        , m_ifaceName{ifaceName}
        , m_modportName{""} {}
    AstIfaceRefDType(File_سطر* fl, File_سطر* modportFl, ثابت string& cellName,
                     ثابت string& ifaceName, ثابت string& modport)
        : ASTGEN_SUPER_IfaceRefDType(fl)
        , m_modportFileline{modportFl}
        , m_cellName{cellName}
        , m_ifaceName{ifaceName}
        , m_modportName{modport} {}
    AstIfaceRefDType(File_سطر* fl, File_سطر* modportFl, ثابت string& cellName,
                     ثابت string& ifaceName, ثابت string& modport, AstPin* paramsp)
        : ASTGEN_SUPER_IfaceRefDType(fl)
        , m_modportFileline{modportFl}
        , m_cellName{cellName}
        , m_ifaceName{ifaceName}
        , m_modportName{modport} {
        addParamsp(paramsp);
    }
    ASTGEN_MEMBERS_AstIfaceRefDType;

    // METHODS
    تهی dump(std::ostream& str = std::cout) ثابت override;
    تهی dumpJson(std::ostream& str = std::cout) ثابت override;
    تهی dumpSmall(std::ostream& str) ثابت override;
    string prettyDTypeName(bool full) ثابت override;
    AstBasicDType* basicp() ثابت override VL_MT_STABLE { بازگشت nullptr; }
    bool similar_DType_گره(ثابت Ast_گره_DType* samep) ثابت override {
        // Each occurrence of a virtual interface type parses to its own node,
        // so pointer identity is insufficient; compare the referenced
        // interface instead, same equivalence as AstNode::computeCastable uses.
        ثابت AstIfaceRefDType* ثابت asamep = VN_DBG_AS(samep, IfaceRefDType);
        بازگشت ifaceViaCellp() && ifaceViaCellp() == asamep->ifaceViaCellp()
               && modportName() == asamep->modportName() && isVirtual() == asamep->isVirtual();
    }
    صحیح widthAlignBytes() ثابت override { بازگشت 0; }
    صحیح widthTotalBytes() ثابت override { بازگشت 0; }
    bool isVirtual() ثابت { بازگشت m_virtual; }
    تهی isVirtual(bool flag) {
        m_virtual = flag;
        اگر (flag) v3Global.setHasVirtIfaces();
    }
    File_سطر* modportFileline() ثابت { بازگشت m_modportFileline; }
    تهی modportFileline(File_سطر* ثابت modportFileline) { m_modportFileline = modportFileline; }
    string cellName() ثابت { بازگشت m_cellName; }
    تهی cellName(ثابت string& name) { m_cellName = name; }
    string ifaceName() ثابت { بازگشت m_ifaceName; }
    string ifaceNameQ() ثابت { بازگشت "'" + prettyName(ifaceName()) + "'"; }
    تهی ifaceName(ثابت string& name) { m_ifaceName = name; }
    تهی modportName(ثابت string& modportName) { m_modportName = modportName; }
    string modportName() ثابت { بازگشت m_modportName; }
    AstIface* ifaceViaCellp() ثابت;  // Use cellp or ifacep
    AstIface* ifacep() ثابت { بازگشت m_ifacep; }
    تهی ifacep(AstIface* nodep) { m_ifacep = nodep; }
    AstCell* cellp() ثابت { بازگشت m_cellp; }
    تهی cellp(AstCell* nodep) { m_cellp = nodep; }
    AstModport* modportp() ثابت { بازگشت m_modportp; }
    تهی modportp(AstModport* modportp) { m_modportp = modportp; }
    bool isModport() { بازگشت !m_modportName.empty(); }
    bool isCompound() ثابت override { بازگشت true; }  // But not relevant
};
class AstMemberDType final : عمومی Ast_گره_DType {
    // A member of a struct/union
    // PARENT: AstNodeUOrStructDType
    // @astgen op1 := childDTypep : Optional[AstNodeDType]
    // @astgen op3 := valuep : Optional[AstNode]
    //
    // @astgen ptr := m_refDTypep : Optional[AstNodeDType]  // Elements of this type (post-width)
    string m_name;  // Name of variable
    string m_tag;  // Holds the string of the verilator tag -- used in JSON output.
    صحیح m_lsb = -1;  // Within this level's packed struct, the LSB of the first bit of the member
    bool m_constrainedRand = false;  // Member has a constraint expression
    VRandAttr m_rand;  // Randomizability of this member (rand, randc, etc)
عمومی:
    AstMemberDType(File_سطر* fl, ثابت string& name, VFlagChildDType, Ast_گره_DType* dtp,
                   Ast_گره* valuep)
        : ASTGEN_SUPER_MemberDType(fl)
        , m_name{name} {
        childDTypep(dtp);  // Only for parser
        this->valuep(valuep);
        dtypep(nullptr);  // V3Width will resolve
        refDTypep(nullptr);
    }
    AstMemberDType(File_سطر* fl, ثابت string& name, Ast_گره_DType* dtp)
        : ASTGEN_SUPER_MemberDType(fl)
        , m_name{name} {
        UASSERT(dtp, "AstMember created with no dtype");
        refDTypep(dtp);
        dtypep(this);
        widthFromSub(subDTypep());
    }
    ASTGEN_MEMBERS_AstMemberDType;
    تهی dump(std::ostream& str = std::cout) ثابت override;
    تهی dumpJson(std::ostream& str = std::cout) ثابت override;
    تهی dumpSmall(std::ostream& str) ثابت override;
    string prettyDTypeName(bool full) ثابت override;
    string name() ثابت override VL_MT_STABLE { بازگشت m_name; }  // * = Var name
    bool hasDType() ثابت override VL_MT_SAFE { بازگشت true; }
    bool maybePointedTo() ثابت override VL_MT_SAFE { بازگشت true; }
    Ast_گره_DType* getChildDTypep() ثابت override { بازگشت childDTypep(); }
    Ast_گره_UOr_Struct_DType* getChildStructp();
    Ast_گره_DType* subDTypep() ثابت override VL_MT_STABLE {
        بازگشت m_refDTypep ? m_refDTypep : childDTypep();
    }
    تهی refDTypep(Ast_گره_DType* nodep) { m_refDTypep = nodep; }
    Ast_گره_DType* virtRefDTypep() ثابت override { بازگشت m_refDTypep; }
    تهی virtRefDTypep(Ast_گره_DType* nodep) override { refDTypep(nodep); }
    bool similar_DType_گره(ثابت Ast_گره_DType* samep) ثابت override { بازگشت this == samep; }
    //
    // (Slow) recurse down to find basic data type (Note don't need virtual -
    // AstVar isn't a NodeDType)
    AstBasicDType* basicp() ثابت override VL_MT_STABLE { بازگشت subDTypep()->basicp(); }
    // op1 = Range of variable (Note don't need virtual - AstVar isn't a NodeDType)
    Ast_گره_DType* dtypeSkipRefp() ثابت { بازگشت subDTypep()->skipRefp(); }
    // (Slow) recurses - Structure alignment 1,2,4 or 8 bytes (arrays affect this)
    صحیح widthAlignBytes() ثابت override { بازگشت subDTypep()->widthAlignBytes(); }
    // (Slow) recurses - Width in bytes rounding up 1,2,4,8,12,...
    صحیح widthTotalBytes() ثابت override { بازگشت subDTypep()->widthTotalBytes(); }
    // METHODS
    تهی name(ثابت string& name) override { m_name = name; }
    string tag() ثابت override { بازگشت m_tag; }
    تهی tag(ثابت string& text) override { m_tag = text; }
    صحیح lsb() ثابت { بازگشت m_lsb; }
    تهی lsb(صحیح lsb) { m_lsb = lsb; }
    bool isCompound() ثابت override {
        v3fatalSrc("call isCompound on subdata type, not reference");
        بازگشت false;
    }
    bool isConstrainedRand() ثابت { بازگشت m_constrainedRand; }
    تهی markConstrainedRand(bool flag) { m_constrainedRand = flag; }
    VRandAttr rand() ثابت { بازگشت m_rand; }
    تهی rand(ثابت VRandAttr flag) { m_rand = flag; }
};
class AstNBACommitQueueDType final : عمومی Ast_گره_DType {
    // @astgen ptr := m_subDTypep : AstNodeDType  // Type of the corresponding variable
    ثابت bool m_partial;  // Partial element update required

عمومی:
    AstNBACommitQueueDType(File_سطر* fl, Ast_گره_DType* subDTypep, bool partial)
        : ASTGEN_SUPER_NBACommitQueueDType(fl)
        , m_partial{partial}
        , m_subDTypep{subDTypep} {
        dtypep(this);
    }
    ASTGEN_MEMBERS_AstNBACommitQueueDType;
    تهی dump(std::ostream& str) ثابت override;
    تهی dumpJson(std::ostream& str) ثابت override;
    Ast_گره_DType* subDTypep() ثابت override VL_MT_STABLE { بازگشت m_subDTypep; }
    bool partial() ثابت { بازگشت m_partial; }
    bool same_گره(ثابت Ast_گره* samep) ثابت override {
        ثابت AstNBACommitQueueDType* ثابت asamep = VN_DBG_AS(samep, NBACommitQueueDType);
        بازگشت m_partial == asamep->m_partial;
    }
    bool similar_DType_گره(ثابت Ast_گره_DType* samep) ثابت override { بازگشت this == samep; }
    AstBasicDType* basicp() ثابت override VL_MT_STABLE { بازگشت nullptr; }
    صحیح widthAlignBytes() ثابت override { بازگشت 1; }
    صحیح widthTotalBytes() ثابت override { بازگشت 24; }
    bool isCompound() ثابت override { بازگشت true; }
};
class AstParamTypeDType final : عمومی Ast_گره_DType {
    // Parents: MODULE
    // A parameter type statement; much like a var or typedef
    // @astgen op1 := childDTypep : Optional[AstNodeDType]
    ثابت VVarType m_varType;  // Type of variable (for localparam vs. param)
    ثابت VFwdType m_fwdType;  // Forward type for lint check
    string m_name;  // Name of variable
عمومی:
    AstParamTypeDType(File_سطر* fl, VVarType type, VFwdType fwdType, ثابت string& name,
                      VFlagChildDType, Ast_گره_DType* dtp)
        : ASTGEN_SUPER_ParamTypeDType(fl)
        , m_varType{type}
        , m_fwdType{fwdType}
        , m_name{name} {
        childDTypep(dtp);  // Only for parser
        dtypep(nullptr);  // V3Width will resolve
    }
    ASTGEN_MEMBERS_AstParamTypeDType;
    تهی dump(std::ostream& str = std::cout) ثابت override;
    تهی dumpJson(std::ostream& str = std::cout) ثابت override;
    string prettyDTypeName(bool full) ثابت override;
    Ast_گره_DType* getChildDTypep() ثابت override { بازگشت childDTypep(); }
    Ast_گره_DType* subDTypep() ثابت override VL_MT_STABLE {
        بازگشت dtypep() ? dtypep() : childDTypep();
    }
    AstBasicDType* basicp() ثابت override VL_MT_STABLE { بازگشت subDTypep()->basicp(); }
    bool similar_DType_گره(ثابت Ast_گره_DType* samep) ثابت override {
        ثابت AstParamTypeDType* ثابت sp = VN_DBG_AS(samep, ParamTypeDType);
        بازگشت this->subDTypep()->similarDType(sp->subDTypep());
    }
    صحیح widthAlignBytes() ثابت override { بازگشت dtypep()->widthAlignBytes(); }
    صحیح widthTotalBytes() ثابت override { بازگشت dtypep()->widthTotalBytes(); }
    // METHODS
    string name() ثابت override VL_MT_STABLE { بازگشت m_name; }
    bool maybePointedTo() ثابت override VL_MT_SAFE { بازگشت true; }
    bool hasDType() ثابت override VL_MT_SAFE { بازگشت true; }
    تهی name(ثابت string& flag) override { m_name = flag; }
    VVarType varType() ثابت { بازگشت m_varType; }  // * = Type of variable
    VFwdType fwdType() ثابت { بازگشت m_fwdType; }
    bool isParam() ثابت { بازگشت true; }
    bool isGParam() ثابت { بازگشت (varType() == VVarType::GPARAM); }
    bool isCompound() ثابت override {
        v3fatalSrc("call isCompound on subdata type, not reference");
        بازگشت false;
    }
};
class AstParseTypeDType final : عمومی Ast_گره_DType {
    // Parents: VAR
    // During parsing, this indicates the type of a parameter is a "parameter type"
    // e.g. the data type is a container of any data type
    ثابت VFwdType m_fwdType;  // Forward type for lint check
عمومی:
    explicit AstParseTypeDType(File_سطر* fl, VFwdType fwdType = VFwdType::NONE)
        : ASTGEN_SUPER_ParseTypeDType(fl)
        , m_fwdType{fwdType} {}
    ASTGEN_MEMBERS_AstParseTypeDType;
    Ast_گره_DType* dtypep() ثابت VL_MT_STABLE { بازگشت nullptr; }
    // METHODS
    تهی dump(std::ostream& str = std::cout) ثابت override;
    تهی dumpJson(std::ostream& str = std::cout) ثابت override;
    bool similar_DType_گره(ثابت Ast_گره_DType* samep) ثابت override { بازگشت this == samep; }
    AstBasicDType* basicp() ثابت override VL_MT_STABLE { بازگشت nullptr; }
    صحیح widthAlignBytes() ثابت override { بازگشت 0; }
    صحیح widthTotalBytes() ثابت override { بازگشت 0; }
    bool isCompound() ثابت override {
        v3fatalSrc("call isCompound on subdata type, not reference");
        بازگشت false;
    }
    VFwdType fwdType() ثابت { بازگشت m_fwdType; }
};
class AstQueueDType final : عمومی Ast_گره_DType {
    // Queue array data type, ie "[ $ ]"
    // @astgen op1 := childDTypep : Optional[AstNodeDType] // moved to refDTypep() in V3Width
    // @astgen op2 := boundp : Optional[AstNodeExpr]
    //
    // @astgen ptr := m_refDTypep : Optional[AstNodeDType]  // Elements of this type (post-width)
عمومی:
    AstQueueDType(File_سطر* fl, VFlagChildDType, Ast_گره_DType* dtp, Ast_گره_عبارت* boundp)
        : ASTGEN_SUPER_QueueDType(fl) {
        childDTypep(dtp);
        this->boundp(boundp);
        refDTypep(nullptr);
        dtypep(nullptr);  // V3Width will resolve
    }
    AstQueueDType(File_سطر* fl, Ast_گره_DType* dtp, Ast_گره_عبارت* boundp)
        : ASTGEN_SUPER_QueueDType(fl) {
        this->boundp(boundp);
        refDTypep(dtp);
        dtypep(dtp);
    }
    ASTGEN_MEMBERS_AstQueueDType;
    ثابت نویسه* broken() ثابت override {
        BROKEN_RTN(!((m_refDTypep && !childDTypep()) || (!m_refDTypep && childDTypep())));
        بازگشت nullptr;
    }
    bool same_گره(ثابت Ast_گره* samep) ثابت override {
        ثابت AstQueueDType* ثابت asamep = VN_DBG_AS(samep, QueueDType);
        اگر (!asamep->subDTypep()) بازگشت false;
        بازگشت (subDTypep() == asamep->subDTypep());
    }
    bool similar_DType_گره(ثابت Ast_گره_DType* samep) ثابت override {
        ثابت AstQueueDType* ثابت asamep = VN_DBG_AS(samep, QueueDType);
        بازگشت asamep->subDTypep() && subDTypep()->similarDType(asamep->subDTypep());
    }
    تهی dumpSmall(std::ostream& str) ثابت override;
    string prettyDTypeName(bool full) ثابت override;
    Ast_گره_DType* getChildDTypep() ثابت override { بازگشت childDTypep(); }
    Ast_گره_DType* subDTypep() ثابت override VL_MT_STABLE {
        بازگشت m_refDTypep ? m_refDTypep : childDTypep();
    }
    bool isAggregateType() ثابت override { بازگشت true; }
    تهی refDTypep(Ast_گره_DType* nodep) { m_refDTypep = nodep; }
    درونخطی صحیح boundConst() ثابت VL_MT_STABLE;
    Ast_گره_DType* virtRefDTypep() ثابت override { بازگشت m_refDTypep; }
    تهی virtRefDTypep(Ast_گره_DType* nodep) override { refDTypep(nodep); }
    // METHODS
    AstBasicDType* basicp() ثابت override VL_MT_STABLE { بازگشت nullptr; }
    صحیح widthAlignBytes() ثابت override { بازگشت subDTypep()->widthAlignBytes(); }
    صحیح widthTotalBytes() ثابت override { بازگشت subDTypep()->widthTotalBytes(); }
    bool isCompound() ثابت override { بازگشت true; }
    bool isDynamicallySized() ثابت override { بازگشت true; }
};
class AstRefDType final : عمومی Ast_گره_DType {
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
    ثابت VIfaceCaptureTag* m_captureTagp = nullptr;

عمومی:
    AstRefDType(File_سطر* fl, ثابت string& name)
        : ASTGEN_SUPER_RefDType(fl)
        , m_name{name} {}
    AstRefDType(File_سطر* fl, ثابت string& name, Ast_گره_عبارت* classOrPackagep, AstPin* paramsp)
        : ASTGEN_SUPER_RefDType(fl)
        , m_name{name} {
        classOrPackageOpp(classOrPackagep);
        addParamsp(paramsp);
    }
    class Flag_Type_Of_عبارت {};  // type(expr) for parser only
    AstRefDType(File_سطر* fl, Flag_Type_Of_عبارت, Ast_گره* typeofp)
        : ASTGEN_SUPER_RefDType(fl) {
        this->typeofp(typeofp);
        اگر (Ast_گره_DType* ثابت dtp = VN_CAST(typeofp, گره_DType)) refDTypep(dtp);
    }
    ASTGEN_MEMBERS_AstRefDType;
    // METHODS
    bool same_گره(ثابت Ast_گره* samep) ثابت override {
        ثابت AstRefDType* ثابت asamep = VN_DBG_AS(samep, RefDType);
        بازگشت (m_typedefp == asamep->m_typedefp && m_refDTypep == asamep->m_refDTypep
                && m_name == asamep->m_name && m_classOrPackagep == asamep->m_classOrPackagep);
    }
    bool similar_DType_گره(ثابت Ast_گره_DType* samep) ثابت override {
        بازگشت subDTypep()->similarDType(samep->subDTypep());
    }
    ثابت نویسه* broken() ثابت override;
    تهی dump(std::ostream& str = std::cout) ثابت override;
    تهی dumpJson(std::ostream& str = std::cout) ثابت override;
    تهی dumpSmall(std::ostream& str) ثابت override;
    string name() ثابت override VL_MT_STABLE { بازگشت m_name; }
    string prettyDTypeName(bool full) ثابت override {
        بازگشت subDTypep() ? subDTypep()->prettyDTypeName(full) : prettyName();
    }
    AstBasicDType* basicp() ثابت override VL_MT_STABLE {
        بازگشت subDTypep() ? subDTypep()->basicp() : nullptr;
    }
    Ast_گره_DType* subDTypep() ثابت override VL_MT_STABLE;
    Ast_گره_DType* getChildDTypep() ثابت override { بازگشت VN_CAST(typeofp(), گره_DType); }
    صحیح widthAlignBytes() ثابت override { بازگشت dtypeSkipRefp()->widthAlignBytes(); }
    صحیح widthTotalBytes() ثابت override { بازگشت dtypeSkipRefp()->widthTotalBytes(); }
    تهی name(ثابت string& flag) override { m_name = flag; }
    Ast_گره_DType* dtypeSkipRefp() ثابت { بازگشت subDTypep()->skipRefp(); }
    AstTypedef* typedefp() ثابت VL_MT_SAFE { بازگشت m_typedefp; }
    تهی typedefp(AstTypedef* nodep) { m_typedefp = nodep; }
    Ast_گره_DType* refDTypep() ثابت VL_MT_SAFE { بازگشت m_refDTypep; }
    تهی refDTypep(Ast_گره_DType* nodep) { m_refDTypep = nodep; }
    Ast_گره_DType* virtRefDTypep() ثابت override { بازگشت refDTypep(); }
    تهی virtRefDTypep(Ast_گره_DType* nodep) override { refDTypep(nodep); }
    Ast_گره_Module* classOrPackagep() ثابت { بازگشت m_classOrPackagep; }
    تهی classOrPackagep(Ast_گره_Module* nodep) { m_classOrPackagep = nodep; }
    ثابت VIfaceCaptureTag* captureTagp() ثابت { بازگشت m_captureTagp; }
    تهی captureTagp(ثابت VIfaceCaptureTag* tagp) { m_captureTagp = tagp; }
    bool isCompound() ثابت override {
        v3fatalSrc("call isCompound on subdata type, not reference");
        بازگشت false;
    }
};
class AstRequireDType final : عمومی Ast_گره_DType {
    // @astgen op1 := lhsp : Optional[AstNode<AstNodeExpr|AstNodeDType>]
    //
    // Require a generic node type (typically AstParseRef become a type.
عمومی:
    AstRequireDType(File_سطر* fl, Ast_گره* lhsp)
        : ASTGEN_SUPER_RequireDType(fl) {
        this->lhsp(lhsp);
    }
    ASTGEN_MEMBERS_AstRequireDType;
    // METHODS
    bool similar_DType_گره(ثابت Ast_گره_DType* samep) ثابت override {
        ثابت AstRequireDType* ثابت asamep = VN_DBG_AS(samep, RequireDType);
        بازگشت subDTypep()->similarDType(asamep->subDTypep());
    }
    AstBasicDType* basicp() ثابت override VL_MT_STABLE { بازگشت nullptr; }
    Ast_گره_DType* subDTypep() ثابت override VL_MT_STABLE {
        // Used for recursive definition checking
        اگر (Ast_گره_DType* ثابت dtp = VN_CAST(lhsp(), گره_DType)) بازگشت dtp;

        بازگشت nullptr;
    }
    صحیح widthAlignBytes() ثابت override { V3ERROR_NA_RETURN(1); }
    صحیح widthTotalBytes() ثابت override { V3ERROR_NA_RETURN(1); }
    bool isCompound() ثابت override { V3ERROR_NA_RETURN(false); }
};
class AstSampleQueueDType final : عمومی Ast_گره_DType {
    // @astgen op1 := childDTypep : Optional[AstNodeDType] // moved to refDTypep() in V3Width
    //
    // @astgen ptr := m_refDTypep : Optional[AstNodeDType]  // Elements of this type (post-width)
عمومی:
    AstSampleQueueDType(File_سطر* fl, Ast_گره_DType* dtp)
        : ASTGEN_SUPER_SampleQueueDType(fl) {
        refDTypep(dtp);
        dtypep(dtp);
    }
    ASTGEN_MEMBERS_AstSampleQueueDType;
    ثابت نویسه* broken() ثابت override {
        BROKEN_RTN(!((m_refDTypep && !childDTypep()) || (!m_refDTypep && childDTypep())));
        بازگشت nullptr;
    }
    bool same_گره(ثابت Ast_گره* samep) ثابت override {
        ثابت AstSampleQueueDType* ثابت asamep = VN_DBG_AS(samep, SampleQueueDType);
        اگر (!asamep->subDTypep()) بازگشت false;
        بازگشت (subDTypep() == asamep->subDTypep());
    }
    bool similar_DType_گره(ثابت Ast_گره_DType* samep) ثابت override {
        ثابت AstSampleQueueDType* ثابت asamep = VN_DBG_AS(samep, SampleQueueDType);
        بازگشت asamep->subDTypep() && subDTypep()->similarDType(asamep->subDTypep());
    }
    تهی dumpSmall(std::ostream& str) ثابت override;
    Ast_گره_DType* getChildDTypep() ثابت override { بازگشت childDTypep(); }
    // op1 = Range of variable
    Ast_گره_DType* subDTypep() ثابت override VL_MT_STABLE {
        بازگشت m_refDTypep ? m_refDTypep : childDTypep();
    }
    تهی refDTypep(Ast_گره_DType* nodep) { m_refDTypep = nodep; }
    Ast_گره_DType* virtRefDTypep() ثابت override { بازگشت m_refDTypep; }
    تهی virtRefDTypep(Ast_گره_DType* nodep) override { refDTypep(nodep); }
    // METHODS
    AstBasicDType* basicp() ثابت override VL_MT_STABLE { بازگشت subDTypep()->basicp(); }
    صحیح widthAlignBytes() ثابت override { بازگشت اندازه(std::map<std::string, std::string>); }
    صحیح widthTotalBytes() ثابت override { بازگشت اندازه(std::map<std::string, std::string>); }
    bool isCompound() ثابت override { بازگشت true; }
};
class AstStreamDType final : عمومی Ast_گره_DType {
    // Stream data type, used only as data type of stream operations
    // Should behave like AstPackArrayDType, but it doesn't have a size
عمومی:
    explicit AstStreamDType(File_سطر* fl)
        : ASTGEN_SUPER_StreamDType(fl) {
        dtypep(this);
    }
    ASTGEN_MEMBERS_AstStreamDType;
    تهی dumpSmall(std::ostream& str) ثابت override;
    bool hasDType() ثابت override VL_MT_SAFE { بازگشت true; }
    bool maybePointedTo() ثابت override VL_MT_SAFE { بازگشت true; }
    bool undead() ثابت override { بازگشت true; }
    Ast_گره_DType* subDTypep() ثابت override VL_MT_STABLE { بازگشت nullptr; }
    Ast_گره_DType* virtRefDTypep() ثابت override { بازگشت nullptr; }
    تهی virtRefDTypep(Ast_گره_DType* nodep) override {}
    bool similar_DType_گره(ثابت Ast_گره_DType* samep) ثابت override { بازگشت this == samep; }
    AstBasicDType* basicp() ثابت override VL_MT_STABLE { بازگشت nullptr; }
    صحیح widthAlignBytes() ثابت override { بازگشت 1; }
    صحیح widthTotalBytes() ثابت override { بازگشت 1; }
    bool isCompound() ثابت override { بازگشت false; }
};
class AstUnsizedArrayDType final : عمومی Ast_گره_DType {
    // Unsized/open-range Array data type, ie "some_dtype var_name []"
    // @astgen op1 := childDTypep : Optional[AstNodeDType] // moved to refDTypep() in V3Width
    //
    // @astgen ptr := m_refDTypep : Optional[AstNodeDType]  // Elements of this type (post-width)
عمومی:
    AstUnsizedArrayDType(File_سطر* fl, VFlagChildDType, Ast_گره_DType* dtp)
        : ASTGEN_SUPER_UnsizedArrayDType(fl) {
        childDTypep(dtp);  // Only for parser
        refDTypep(nullptr);
        dtypep(nullptr);  // V3Width will resolve
    }
    AstUnsizedArrayDType(File_سطر* fl, Ast_گره_DType* dtp)
        : ASTGEN_SUPER_UnsizedArrayDType(fl) {
        refDTypep(dtp);
        dtypep(nullptr);  // V3Width will resolve
    }
    ASTGEN_MEMBERS_AstUnsizedArrayDType;
    ثابت نویسه* broken() ثابت override {
        BROKEN_RTN(!((m_refDTypep && !childDTypep()) || (!m_refDTypep && childDTypep())));
        بازگشت nullptr;
    }
    bool same_گره(ثابت Ast_گره* samep) ثابت override;
    bool similar_DType_گره(ثابت Ast_گره_DType* samep) ثابت override;
    تهی dumpSmall(std::ostream& str) ثابت override;
    Ast_گره_DType* getChildDTypep() ثابت override { بازگشت childDTypep(); }
    Ast_گره_DType* subDTypep() ثابت override VL_MT_STABLE {
        بازگشت m_refDTypep ? m_refDTypep : childDTypep();
    }
    تهی refDTypep(Ast_گره_DType* nodep) { m_refDTypep = nodep; }
    Ast_گره_DType* virtRefDTypep() ثابت override { بازگشت m_refDTypep; }
    تهی virtRefDTypep(Ast_گره_DType* nodep) override { refDTypep(nodep); }
    // METHODS
    AstBasicDType* basicp() ثابت override VL_MT_STABLE { بازگشت subDTypep()->basicp(); }
    صحیح widthAlignBytes() ثابت override { بازگشت subDTypep()->widthAlignBytes(); }
    صحیح widthTotalBytes() ثابت override { بازگشت subDTypep()->widthTotalBytes(); }
    bool isCompound() ثابت override { بازگشت true; }
};
class AstVoidDType final : عمومی Ast_گره_DType {
    // For e.g. a function returning void
عمومی:
    explicit AstVoidDType(File_سطر* fl)
        : ASTGEN_SUPER_VoidDType(fl) {
        dtypep(this);
    }
    ASTGEN_MEMBERS_AstVoidDType;
    تهی dumpSmall(std::ostream& str) ثابت override;
    bool hasDType() ثابت override VL_MT_SAFE { بازگشت true; }
    bool maybePointedTo() ثابت override VL_MT_SAFE { بازگشت true; }
    bool undead() ثابت override { بازگشت true; }
    Ast_گره_DType* subDTypep() ثابت override VL_MT_STABLE { بازگشت nullptr; }
    Ast_گره_DType* virtRefDTypep() ثابت override { بازگشت nullptr; }
    تهی virtRefDTypep(Ast_گره_DType* nodep) override {}
    bool similar_DType_گره(ثابت Ast_گره_DType* samep) ثابت override { بازگشت true; }
    AstBasicDType* basicp() ثابت override VL_MT_STABLE { بازگشت nullptr; }
    صحیح widthAlignBytes() ثابت override { بازگشت 1; }
    صحیح widthTotalBytes() ثابت override { بازگشت 1; }
    bool isCompound() ثابت override { بازگشت false; }
};
class AstWildcardArrayDType final : عمومی Ast_گره_DType {
    // Wildcard index type associative array data type, ie "some_dtype var_name [*]"
    // @astgen op1 := childDTypep : Optional[AstNodeDType] // moved to refDTypep() in V3Width
    //
    // @astgen ptr := m_refDTypep : Optional[AstNodeDType]  // Elements of this type (post-width)
عمومی:
    AstWildcardArrayDType(File_سطر* fl, VFlagChildDType, Ast_گره_DType* dtp)
        : ASTGEN_SUPER_WildcardArrayDType(fl) {
        childDTypep(dtp);  // Only for parser
        refDTypep(nullptr);
        dtypep(nullptr);  // V3Width will resolve
    }
    ASTGEN_MEMBERS_AstWildcardArrayDType;
    ثابت نویسه* broken() ثابت override {
        BROKEN_RTN(!((m_refDTypep && !childDTypep()) || (!m_refDTypep && childDTypep())));
        بازگشت nullptr;
    }
    bool same_گره(ثابت Ast_گره* samep) ثابت override;
    bool similar_DType_گره(ثابت Ast_گره_DType* samep) ثابت override;
    تهی dumpSmall(std::ostream& str) ثابت override;
    string prettyDTypeName(bool full) ثابت override;
    Ast_گره_DType* getChildDTypep() ثابت override { بازگشت childDTypep(); }
    Ast_گره_DType* subDTypep() ثابت override VL_MT_STABLE {
        بازگشت m_refDTypep ? m_refDTypep : childDTypep();
    }
    تهی refDTypep(Ast_گره_DType* nodep) { m_refDTypep = nodep; }
    Ast_گره_DType* virtRefDTypep() ثابت override { بازگشت m_refDTypep; }
    تهی virtRefDTypep(Ast_گره_DType* nodep) override { refDTypep(nodep); }
    // METHODS
    AstBasicDType* basicp() ثابت override VL_MT_STABLE { بازگشت subDTypep()->basicp(); }
    صحیح widthAlignBytes() ثابت override { بازگشت اندازه(std::map<std::string, std::string>); }
    صحیح widthTotalBytes() ثابت override { بازگشت اندازه(std::map<std::string, std::string>); }
    bool isCompound() ثابت override { بازگشت true; }
    bool isDynamicallySized() ثابت override { بازگشت true; }
};

// === AstNodeArrayDType ===
class AstPackArrayDType final : عمومی Ast_گره_Array_DType {
    // Packed array data type, ie "some_dtype [2:0] var_name"
عمومی:
    درونخطی AstPackArrayDType(File_سطر* fl, VFlagChildDType, Ast_گره_DType* dtp, AstRange* rangep);
    درونخطی AstPackArrayDType(File_سطر* fl, Ast_گره_DType* dtp, AstRange* rangep);
    ASTGEN_MEMBERS_AstPackArrayDType;
    string prettyDTypeName(bool full) ثابت override;
    bool isCompound() ثابت override { بازگشت false; }
};
class AstUnpackArrayDType final : عمومی Ast_گره_Array_DType {
    // Array data type, ie "some_dtype var_name [2:0]"
    bool m_isCompound = false;  // Non-POD subDType, or parent requires compound
عمومی:
    AstUnpackArrayDType(File_سطر* fl, VFlagChildDType, Ast_گره_DType* dtp, AstRange* rangep)
        : ASTGEN_SUPER_UnpackArrayDType(fl) {
        childDTypep(dtp);  // Only for parser
        this->rangep(rangep);
        refDTypep(nullptr);
        dtypep(nullptr);  // V3Width will resolve
        // For backward compatibility AstNodeArrayDType and others inherit
        // width and signing from the subDType/base type
        widthFromSub(subDTypep());
    }
    AstUnpackArrayDType(File_سطر* fl, Ast_گره_DType* dtp, AstRange* rangep)
        : ASTGEN_SUPER_UnpackArrayDType(fl) {
        this->rangep(rangep);
        refDTypep(dtp);
        dtypep(this);
        // For backward compatibility AstNodeArrayDType and others inherit
        // width and signing from the subDType/base type
        widthFromSub(subDTypep());
    }
    ASTGEN_MEMBERS_AstUnpackArrayDType;
    تهی dump(std::ostream& str) ثابت override;
    تهی dumpJson(std::ostream& str) ثابت override;
    string prettyDTypeName(bool full) ثابت override;
    bool same_گره(ثابت Ast_گره* samep) ثابت override {
        ثابت AstUnpackArrayDType* ثابت sp = VN_DBG_AS(samep, UnpackArrayDType);
        بازگشت m_isCompound == sp->m_isCompound;
    }
    bool similar_DType_گره(ثابت Ast_گره_DType* samep) ثابت override;
    bool isAggregateType() ثابت override { بازگشت true; }
    // True iff 'otherp' is an array of the same shape, with
    // at least one dimension reversed ascending/descending.
    bool isReverseOf(ثابت Ast_گره_DType* otherp) ثابت;
    // Outer dimension comes first. The first element is this node.
    std::vector<AstUnpackArrayDType*> unpackDimensions();
    تهی isCompound(bool flag) { m_isCompound = flag; }
    bool isCompound() ثابت override VL_MT_SAFE { بازگشت m_isCompound; }
    bool isIntegralOrPacked() ثابت override { بازگشت false; }
};

// === AstNodeUOrStructDType ===
class AstStructDType final : عمومی Ast_گره_UOr_Struct_DType {
عمومی:
    // VSigning below is mispurposed to indicate if packed or not
    AstStructDType(File_سطر* fl, VSigning numericUnpack)
        : ASTGEN_SUPER_StructDType(fl, numericUnpack) {}
    ASTGEN_MEMBERS_AstStructDType;
    string verilogKwd() ثابت override { بازگشت "struct"; }
};
class AstUnionDType final : عمومی Ast_گره_UOr_Struct_DType {
    bool m_isSoft;  // Is a "union soft"
    bool m_isTagged;  // Is a "union tagged"

عمومی:
    // VSigning below is mispurposed to indicate if packed or not
    // isSoft implies packed
    AstUnionDType(File_سطر* fl, bool isSoft, bool isTagged, VSigning numericUnpack)
        : ASTGEN_SUPER_UnionDType(fl, numericUnpack)
        , m_isSoft{isSoft}
        , m_isTagged{isTagged} {
        packed(packed() | m_isSoft);
    }
    ASTGEN_MEMBERS_AstUnionDType;
    string verilogKwd() ثابت override { بازگشت "union"; }
    bool isSoft() ثابت { بازگشت m_isSoft; }
    bool isTagged() ثابت { بازگشت m_isTagged; }
    bool same_گره(ثابت Ast_گره* samep) ثابت override;
    تهی dump(std::ostream& str) ثابت override;
    تهی dumpJson(std::ostream& str) ثابت override;
};

#endif  // Guard
