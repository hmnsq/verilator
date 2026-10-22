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
#ضمن "V3Ast.h"  // This helps code analysis tools pick up symbols in V3Ast.h
#define VL_NOT_FINAL  // This #define fixes broken code folding in the CLion IDE
#endif

// === Abstract base node types (AstNode*) =====================================

class Ast_عقدة_DType VL_NOT_FINAL : عام Ast_عقدة {
    // Ideally width() would migrate to BasicDType as that's where it makes sense,
    // but it's currently so prevalent in the code we leave it here.
    // Note the below members are included in AstTypeTable::Key lookups
    // dist-ast-dump-suppress  // Part of dumpSmall
    صحيح m_width = 0;  // (also in AstTypeTable::Key) Bit width of operation
    // (also in AstTypeTable::Key) If unsized, bitwidth of minimum implementation
    // dist-ast-dump-suppress  // Part of dumpSmall
    صحيح m_widthMin = 0;
    // dist-ast-dump-suppress  // Part of dumpSmall
    VSigning m_numeric;  // (also in AstTypeTable::Key) Node is signed
    // dist-ast-dump-suppress  // Part of dumpSmall
    bool m_generic = false;  // Simple globally referenced type, don't garbage collect
    // Unique number assigned to each dtype during creation for IEEE matching
    ساكن صحيح s_unique_رقم;

محمي:
    // CONSTRUCTORS
    Ast_عقدة_DType(VNType t, File_سطر* fl)
        : Ast_عقدة{t, fl} {}

خاص:
    // METHODS
    ثابت Ast_عقدة_DType* skipRefIterp(bool skipConst, bool skipEnum,
                                     bool assertOn = true) ثابت VL_MT_STABLE;

محمي:
    // METHODS
    virtual bool similar_DType_عقدة(ثابت Ast_عقدة_DType* samep) ثابت = 0;

عام:
    ASTGEN_MEMBERS_Ast_عقدة_DType;
    // ACCESSORS
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    virtual فراغ dumpSmall(std::ostream& str) ثابت VL_MT_STABLE;
    bool hasDType() ثابت override VL_MT_SAFE { أرجع true; }
    /// Require VlUnpacked, instead of [] for POD elements.
    /// A non-POD object is always compound, but some POD elements
    /// are compound when methods calls operate on object, or when
    /// under another compound-requiring object e.g. class
    virtual bool isCompound() ثابت = 0;
    // Integral or packed, allowed inside an unpacked union/struct
    virtual bool isIntegralOrPacked() ثابت { أرجع !isCompound(); }
    // (Slow) recurse down to find basic data type
    virtual AstBasicDType* basicp() ثابت VL_MT_STABLE = 0;
    // (Slow) Recurse over MemberDType|ParamTypeDType|RefDType|ConstDType|EnumDType to other type
    ثابت Ast_عقدة_DType* skipRefp() ثابت VL_MT_STABLE { أرجع skipRefIterp(true, true); }
    Ast_عقدة_DType* skipRefp() VL_MT_STABLE {
        أرجع const_cast<Ast_عقدة_DType*>(
            static_cast<ثابت Ast_عقدة_DType*>(this)->skipRefIterp(true, true));
    }
    // (Slow) Recurse over MemberDType|ParamTypeDType|RefDType|ConstDType|EnumDType to other type,
    // Returns null if not resolved
    ثابت Ast_عقدة_DType* skipRefOrNullp() ثابت VL_MT_STABLE {
        أرجع skipRefIterp(true, true, false);
    }
    Ast_عقدة_DType* skipRefOrNullp() VL_MT_STABLE {
        أرجع const_cast<Ast_عقدة_DType*>(
            static_cast<ثابت Ast_عقدة_DType*>(this)->skipRefIterp(true, true, false));
    }
    // (Slow) Recurse over MemberDType|ParamTypeDType|RefDType|EnumDType to ConstDType
    ثابت Ast_عقدة_DType* skipRefToConstp() ثابت { أرجع skipRefIterp(false, true); }
    Ast_عقدة_DType* skipRefToConstp() {
        أرجع const_cast<Ast_عقدة_DType*>(
            static_cast<ثابت Ast_عقدة_DType*>(this)->skipRefIterp(false, true));
    }
    // (Slow) Recurse over MemberDType|ParamTypeDType|RefDType|ConstDType to EnumDType
    ثابت Ast_عقدة_DType* skipRefToEnump() ثابت { أرجع skipRefIterp(true, false); }
    Ast_عقدة_DType* skipRefToEnump() {
        أرجع const_cast<Ast_عقدة_DType*>(
            static_cast<ثابت Ast_عقدة_DType*>(this)->skipRefIterp(true, false));
    }
    // (Slow) Recurse over MemberDType|ParamTypeDType|RefDType|ConstDType to EnumDType,
    // Returns null if not resolved
    ثابت Ast_عقدة_DType* skipRefToEnumOrNullp() ثابت { أرجع skipRefIterp(true, false, false); }
    // (Slow) Recurse over MemberDType|ParamTypeDType|RefDType to other type
    ثابت Ast_عقدة_DType* skipRefToNonRefp() ثابت { أرجع skipRefIterp(false, false); }
    Ast_عقدة_DType* skipRefToNonRefp() {
        أرجع const_cast<Ast_عقدة_DType*>(
            static_cast<ثابت Ast_عقدة_DType*>(this)->skipRefIterp(false, false));
    }
    // If array, returns element dtype, otherwise returns skipRef dtype
    // If skipRef is false, RefDTypes are not followed (safe before typedef linking)
    ثابت Ast_عقدة_DType* elemDTypep(bool skipRef = true) ثابت VL_MT_STABLE;
    Ast_عقدة_DType* elemDTypep(bool skipRef = true) VL_MT_STABLE {
        أرجع const_cast<Ast_عقدة_DType*>(
            static_cast<ثابت Ast_عقدة_DType*>(this)->elemDTypep(skipRef));
    }
    // (Slow) recurses - Structure alignment 1,2,4 or 8 bytes (arrays affect this)
    virtual صحيح widthAlignBytes() ثابت = 0;
    // (Slow) recurses - Width in bytes rounding up 1,2,4,8,12,...
    virtual صحيح widthTotalBytes() ثابت = 0;
    bool maybePointedTo() ثابت override VL_MT_SAFE { أرجع true; }
    // Iff has a non-null refDTypep(), as generic node function
    virtual Ast_عقدة_DType* virtRefDTypep() ثابت { أرجع nullptr; }
    // Iff has refDTypep(), set as generic node function
    virtual فراغ virtRefDTypep(Ast_عقدة_DType* nodep) {}
    // Iff has a non-null second dtypep, as generic node function
    virtual Ast_عقدة_DType* virtRefDType2p() ثابت { أرجع nullptr; }
    // Iff has second dtype, set as generic node function
    virtual فراغ virtRefDType2p(Ast_عقدة_DType* nodep) {}
    // Assignable equivalence.  Calls skipRefToNonRefp() during comparisons.
    bool similarDType(ثابت Ast_عقدة_DType* samep) ثابت;
    // Iff has a non-null subDTypep(), as generic node function
    virtual Ast_عقدة_DType* subDTypep() ثابت VL_MT_STABLE { أرجع nullptr; }
    virtual Ast_عقدة_DType* subDType2p() ثابت VL_MT_STABLE { أرجع nullptr; }
    virtual bool isAggregateType() ثابت { أرجع false; }
    // True for unpacked, dynamic, queue, and associative arrays (not packed arrays)
    bool isNonPackedArray() ثابت;
    virtual bool isFourstate() ثابت;
    // Ideally an IEEE $typename
    virtual string prettyDTypeName(bool) ثابت { أرجع prettyTypeName(); }
    string prettyDTypeNameQ(bool full = false) ثابت { أرجع "'" + prettyDTypeName(full) + "'"; }
    string stateDTypeName() ثابت { أرجع this->isFourstate() ? "(4-state)" : "(2-state)"; }
    //
    // Changing the width may confuse the data type resolution, so must clear
    // TypeTable cache after use.
    فراغ widthForce(صحيح width, صحيح widthMin) {
        m_width = width;
        m_widthMin = widthMin;
    }
    // For backward compatibility inherit width and signing from the subDType/base type
    فراغ widthFromSub(ثابت Ast_عقدة_DType* nodep) {
        m_width = nodep->m_width;
        m_widthMin = nodep->m_widthMin;
        m_numeric = nodep->m_numeric;
    }
    //
    صحيح width() ثابت VL_MT_STABLE { أرجع m_width; }
    فراغ numeric(VSigning flag) { m_numeric = flag; }
    bool isSigned() ثابت VL_MT_STABLE { أرجع m_numeric.isSigned(); }
    bool isNosign() ثابت VL_MT_SAFE { أرجع m_numeric.isNosign(); }
    VSigning numeric() ثابت VL_MT_STABLE { أرجع m_numeric; }
    صحيح width_كلمات() ثابت VL_MT_STABLE { أرجع VL_كلمات_I(width()); }
    صحيح widthMin() ثابت VL_MT_STABLE {  // If sized, the size,
                                         // if unsized the min digits to represent it
        أرجع m_widthMin ? m_widthMin : m_width;
    }
    صحيح widthPow2() ثابت;
    فراغ widthMinFromWidth() { m_widthMin = m_width; }
    bool widthSized() ثابت VL_MT_SAFE { أرجع !m_widthMin || m_widthMin == m_width; }
    bool generic() ثابت VL_MT_SAFE { أرجع m_generic; }
    فراغ generic(bool flag) { m_generic = flag; }
    std::pair<uint32_t, uint32_t> dimensions(bool includeBasic) ثابت;
    uint32_t arrayUnpackedElements() ثابت;  // 1, or total multiplication of all dimensions
    // Fixed aggregate streaming properties
    bool isStreamableFixedAggregate() ثابت;
    // Whether is a string, or an unpacked array or structure containing one
    bool containsString() ثابت;
    bool containsUnpackedStruct() ثابت;
    صحيح widthStream() ثابت;
    string vlEnumType() ثابت;  // Return VerilatedVarType: VLVT_UINT32, etc
    string vlEnumPackedAgg() ثابت;  // Return VLVF_PACKED_AGG etc flags, or empty
    ساكن صحيح unique_رقم_Inc() { أرجع ++s_unique_رقم; }
    ثابت حرف* charIQWN() ثابت {
        أرجع (isString() ? "N" : isWide() ? "W" : isDouble() ? "D" : isQuad() ? "Q" : "I");
    }
    string cType(ثابت string& name, bool forFunc, bool isRef, bool packed = false) ثابت;
    // Represents a C++ LiteralType? (can be constexpr)
    bool isLiteralType() ثابت VL_MT_STABLE;
    // C++ type (as emitted by cType) is trivially default constructible and destructible?
    bool isTriviallyConstructible() ثابت VL_MT_STABLE;
    virtual bool isDynamicallySized() ثابت { أرجع false; }

خاص:
    class CTypeRecursed;
    CTypeRecursed cTypeRecurse(bool compound, bool packed) ثابت;
};
class Ast_عقدة_Array_DType VL_NOT_FINAL : عام Ast_عقدة_DType {
    // Array data type, ie "some_dtype var_name [2:0]"
    // @astgen op1 := childDTypep : Optional[AstNodeDType] // moved to refDTypep() in V3Width
    // @astgen op2 := rangep : Optional[AstRange] // array bounds
    //
    // @astgen ptr := m_refDTypep : Optional[AstNodeDType]  // Elements of this type (post-width)
    Ast_عقدة* rangenp() ثابت { أرجع reinterpret_cast<Ast_عقدة*>(rangep()); }

محمي:
    Ast_عقدة_Array_DType(VNType t, File_سطر* fl)
        : Ast_عقدة_DType{t, fl} {}

عام:
    ASTGEN_MEMBERS_Ast_عقدة_Array_DType;
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    فراغ dumpSmall(std::ostream& str) ثابت override;
    ثابت حرف* broken() ثابت override {
        BROKEN_RTN(!((m_refDTypep && !childDTypep()) || (!m_refDTypep && childDTypep())));
        أرجع nullptr;
    }
    bool same_عقدة(ثابت Ast_عقدة* samep) ثابت override {
        ثابت Ast_عقدة_Array_DType* ثابت asamep = VN_DBG_AS(samep, عقدة_Array_DType);
        أرجع hi() == asamep->hi() && rangenp()->same_شجرة(asamep->rangenp())
               && subDTypep() == asamep->subDTypep();
    }  // HashedDT doesn't recurse, so need to check children
    bool similar_DType_عقدة(ثابت Ast_عقدة_DType* samep) ثابت override {
        ثابت Ast_عقدة_Array_DType* ثابت asamep = VN_DBG_AS(samep, عقدة_Array_DType);
        أرجع elementsConst() == asamep->elementsConst()
               && subDTypep()->similarDType(asamep->subDTypep());
    }
    Ast_عقدة_DType* getChildDTypep() ثابت override { أرجع childDTypep(); }
    Ast_عقدة_DType* subDTypep() ثابت override VL_MT_STABLE {
        أرجع m_refDTypep ? m_refDTypep : childDTypep();
    }
    فراغ refDTypep(Ast_عقدة_DType* nodep) { m_refDTypep = nodep; }
    Ast_عقدة_DType* virtRefDTypep() ثابت override { أرجع m_refDTypep; }
    فراغ virtRefDTypep(Ast_عقدة_DType* nodep) override { refDTypep(nodep); }
    // METHODS
    AstBasicDType* basicp() ثابت override VL_MT_STABLE {
        أرجع subDTypep()->basicp();
    }  // (Slow) recurse down to find basic data type
    صحيح widthAlignBytes() ثابت override { أرجع subDTypep()->widthAlignBytes(); }
    صحيح widthTotalBytes() ثابت override {
        أرجع elementsConst() * subDTypep()->widthTotalBytes();
    }
    مضمن صحيح يسار() ثابت VL_MT_STABLE;
    مضمن صحيح يمين() ثابت VL_MT_STABLE;
    مضمن صحيح hi() ثابت VL_MT_STABLE;
    مضمن صحيح lo() ثابت VL_MT_STABLE;
    مضمن صحيح elementsConst() ثابت VL_MT_STABLE;
    مضمن VNumRange declRange() ثابت VL_MT_STABLE;
};
class Ast_عقدة_UOr_Struct_DType VL_NOT_FINAL : عام Ast_عقدة_DType {
    // A struct or union; common handling
    // @astgen op1 := membersp : List[AstMemberDType]
    //
    // @astgen ptr := m_classOrPackagep : Optional[AstNodeModule]  // Package emitted with
    string m_name;  // Name from upper typedef, if any
    string m_typedefName;  // Typedef's dtypeName() once moved off the typedef declaring it
    ثابت صحيح m_unique_رقم;  // Unique ID distinguishing this dtype instance, for hashing/naming
    bool m_packed;  // Packed struct/union, else unpacked
    bool m_isFourstate = false;  // V3Width computes; true if any member is 4-state
    bool m_constrainedRand = false;  // True if struct has constraint expression
    bool m_emitToString = false;  // Generate to_string() for this struct/union if set

محمي:
    Ast_عقدة_UOr_Struct_DType(VNType t, File_سطر* fl, VSigning numericUnpack)
        : Ast_عقدة_DType{t, fl}
        , m_unique_رقم{unique_رقم_Inc()} {
        // VSigning::NOSIGN overloaded to indicate not packed
        m_packed = (numericUnpack != VSigning::NOSIGN);
        numeric(VSigning::fromBool(numericUnpack.isSigned()));
    }
    Ast_عقدة_UOr_Struct_DType(ثابت Ast_عقدة_UOr_Struct_DType& other)
        : Ast_عقدة_DType{other}
        , m_name{other.m_name}
        , m_typedefName{other.m_typedefName}
        , m_unique_رقم{unique_رقم_Inc()}
        , m_packed{other.m_packed}
        , m_isFourstate{other.m_isFourstate} {}

عام:
    ASTGEN_MEMBERS_Ast_عقدة_UOr_Struct_DType;
    صحيح unique_رقم() ثابت { أرجع m_unique_رقم; }
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    string prettyDTypeName(bool) ثابت override;
    bool isCompound() ثابت override { أرجع !packed(); }
    // For basicp() we reuse the size to indicate a "fake" basic type of same size
    AstBasicDType* basicp() ثابت override VL_MT_STABLE {
        إذا (!m_packed) أرجع nullptr;
        أرجع (isFourstate()
                    ? VN_AS(findLogicRangeDType(VNumRange{width() - 1, 0}, width(), numeric()),
                            BasicDType)
                    : VN_AS(findBitRangeDType(VNumRange{width() - 1, 0}, width(), numeric()),
                            BasicDType));
    }
    // (Slow) recurses - Structure alignment 1,2,4 or 8 bytes (arrays affect this)
    صحيح widthAlignBytes() ثابت override;
    // (Slow) recurses - Width in bytes rounding up 1,2,4,8,12,...
    صحيح widthTotalBytes() ثابت override;
    bool similar_DType_عقدة(ثابت Ast_عقدة_DType* samep) ثابت override;
    string name() ثابت override VL_MT_STABLE { أرجع m_name; }
    فراغ name(ثابت string& flag) override { m_name = flag; }
    // Keep the name the typedef declaring it gives, as it moves off it to the type table
    فراغ typedefName(ثابت string& name) { m_typedefName = name; }
    // Name of the C structure for DPI, empty if no typedef names it, so declared where used
    string dpiName() ثابت;
    // C typedef of the unpacked structure for DPI (IEEE 1800-2023 H.7.3)
    string dpiTypedef() ثابت;
    bool packed() ثابت VL_MT_SAFE { أرجع m_packed; }
    فراغ packed(bool flag) { m_packed = flag; }
    // packed() but as don't support unpacked, presently all structs
    ساكن bool packedUnsup() { أرجع true; }
    فراغ isFourstate(bool flag) { m_isFourstate = flag; }
    bool isFourstate() ثابت override VL_MT_SAFE { أرجع m_isFourstate; }
    ساكن صحيح lo() VL_MT_STABLE { أرجع 0; }
    صحيح hi() ثابت VL_MT_STABLE {
        أرجع dtypep()->width() - 1;
    }  // Packed classes look like arrays
    VNumRange declRange() ثابت VL_MT_STABLE { أرجع VNumRange{hi(), lo()}; }
    Ast_عقدة_Module* classOrPackagep() ثابت { أرجع m_classOrPackagep; }
    فراغ classOrPackagep(Ast_عقدة_Module* classpackagep) { m_classOrPackagep = classpackagep; }
    bool isConstrainedRand() ثابت { أرجع m_constrainedRand; }
    فراغ markConstrainedRand(bool flag) { m_constrainedRand = flag; }
    bool emitToString() ثابت { أرجع m_emitToString; }
    فراغ setEmitToString() { m_emitToString = true; }
    bool isAggregateType() ثابت override { أرجع !packed(); }
};

// === Concrete node types =====================================================

// === AstNode ===
class AstEnumItem final : عام Ast_عقدة {
    // @astgen op1 := rangep : Optional[AstRange] // Range for name appending
    // @astgen op2 := valuep : Optional[AstNodeExpr]
    string m_name;  // Name of item

عام:
    // Parents: ENUM
    AstEnumItem(File_سطر* fl, ثابت string& name, AstRange* rangep, Ast_عقدة_تعبير* valuep)
        : ASTGEN_SUPER_EnumItem(fl)
        , m_name{name} {
        this->rangep(rangep);
        this->valuep(valuep);
    }
    ASTGEN_MEMBERS_AstEnumItem;
    string name() ثابت override VL_MT_STABLE { أرجع m_name; }
    bool maybePointedTo() ثابت override VL_MT_SAFE { أرجع true; }
    bool hasDType() ثابت override VL_MT_SAFE { أرجع true; }
    فراغ name(ثابت string& flag) override { m_name = flag; }
};

// === AstNodeDType ===
class AstAssocArrayDType final : عام Ast_عقدة_DType {
    // Associative array data type, ie "[some_dtype]"
    // @astgen op1 := childDTypep : Optional[AstNodeDType] // moved to refDTypep() in V3Width
    // @astgen op2 := keyChildDTypep : Optional[AstNodeDType]
    //
    // @astgen ptr := m_refDTypep : Optional[AstNodeDType]  // Elements of this type (post-width)
    // @astgen ptr := m_keyDTypep : Optional[AstNodeDType]  // Keys of this type (post-width)
عام:
    AstAssocArrayDType(File_سطر* fl, VFlagChildDType, Ast_عقدة_DType* dtp, Ast_عقدة_DType* مفتاح_Dtp)
        : ASTGEN_SUPER_AssocArrayDType(fl) {
        childDTypep(dtp);  // Only for parser
        مفتاح_Child_DTypep(مفتاح_Dtp);  // Only for parser
        refDTypep(nullptr);
        مفتاح_DTypep(nullptr);
        dtypep(nullptr);  // V3Width will resolve
    }
    AstAssocArrayDType(File_سطر* fl, Ast_عقدة_DType* dtp, Ast_عقدة_DType* مفتاح_Dtp)
        : ASTGEN_SUPER_AssocArrayDType(fl) {
        refDTypep(dtp);
        مفتاح_DTypep(مفتاح_Dtp);
        dtypep(dtp);
    }
    ASTGEN_MEMBERS_AstAssocArrayDType;
    ثابت حرف* broken() ثابت override {
        BROKEN_RTN(!((m_refDTypep && !childDTypep()) || (!m_refDTypep && childDTypep())));
        BROKEN_RTN(!((m_مفتاح_DTypep && !childDTypep()) || (!m_مفتاح_DTypep && childDTypep())));
        أرجع nullptr;
    }
    bool same_عقدة(ثابت Ast_عقدة* samep) ثابت override {
        ثابت AstAssocArrayDType* ثابت asamep = VN_DBG_AS(samep, AssocArrayDType);
        إذا (!asamep->subDTypep()) أرجع false;
        إذا (!asamep->مفتاح_DTypep()) أرجع false;
        أرجع (subDTypep() == asamep->subDTypep() && مفتاح_DTypep() == asamep->مفتاح_DTypep());
    }
    bool similar_DType_عقدة(ثابت Ast_عقدة_DType* samep) ثابت override {
        ثابت AstAssocArrayDType* ثابت asamep = VN_DBG_AS(samep, AssocArrayDType);
        أرجع asamep->subDTypep() && subDTypep()->similarDType(asamep->subDTypep())
               && asamep->مفتاح_DTypep() && مفتاح_DTypep()->similarDType(asamep->مفتاح_DTypep());
    }
    string prettyDTypeName(bool full) ثابت override;
    فراغ dumpSmall(std::ostream& str) ثابت override;
    Ast_عقدة_DType* getChildDTypep() ثابت override { أرجع childDTypep(); }
    Ast_عقدة_DType* getChild2DTypep() ثابت override { أرجع مفتاح_Child_DTypep(); }
    Ast_عقدة_DType* subDTypep() ثابت override VL_MT_STABLE {
        أرجع m_refDTypep ? m_refDTypep : childDTypep();
    }
    Ast_عقدة_DType* subDType2p() ثابت override VL_MT_STABLE {
        أرجع m_مفتاح_DTypep ? m_مفتاح_DTypep : مفتاح_Child_DTypep();
    }
    bool isAggregateType() ثابت override { أرجع true; }
    فراغ refDTypep(Ast_عقدة_DType* nodep) { m_refDTypep = nodep; }
    Ast_عقدة_DType* virtRefDTypep() ثابت override { أرجع m_refDTypep; }
    فراغ virtRefDTypep(Ast_عقدة_DType* nodep) override { refDTypep(nodep); }
    Ast_عقدة_DType* virtRefDType2p() ثابت override { أرجع m_مفتاح_DTypep; }
    فراغ virtRefDType2p(Ast_عقدة_DType* nodep) override { مفتاح_DTypep(nodep); }
    //
    Ast_عقدة_DType* مفتاح_DTypep() ثابت VL_MT_STABLE {
        أرجع m_مفتاح_DTypep ? m_مفتاح_DTypep : مفتاح_Child_DTypep();
    }
    فراغ مفتاح_DTypep(Ast_عقدة_DType* nodep) { m_مفتاح_DTypep = nodep; }
    // METHODS
    AstBasicDType* basicp() ثابت override VL_MT_STABLE { أرجع nullptr; }
    صحيح widthAlignBytes() ثابت override { أرجع subDTypep()->widthAlignBytes(); }
    صحيح widthTotalBytes() ثابت override { أرجع subDTypep()->widthTotalBytes(); }
    bool isCompound() ثابت override { أرجع true; }
    bool isDynamicallySized() ثابت override { أرجع true; }
};
class AstBasicDType final : عام Ast_عقدة_DType {
    // Builtin atomic/vectored data type
    // @astgen op1 := rangep : Optional[AstRange] // Range of variable
    بنية Members final {
        VBasicDTypeKwd m_keyword;  // (also in VBasicTypeKey) What keyword created basic type
        // dist-ast-dump-suppress  // Part of dumpSmall
        VNumRange m_nrange;  // (also in VBasicTypeKey) Numeric msb/lsb (if non-opaque keyword)
        bool operator==(ثابت Members& rhs) ثابت {
            أرجع rhs.m_keyword == m_keyword && rhs.m_nrange == m_nrange;
        }
    } m;
    // See also in AstNodeDType: m_width, m_widthMin, m_numeric(issigned)
عام:
    AstBasicDType(File_سطر* fl, VBasicDTypeKwd kwd, ثابت VSigning& signst = VSigning::NOSIGN)
        : ASTGEN_SUPER_BasicDType(fl) {
        init(kwd, signst, 0, -1, nullptr);
    }
    AstBasicDType(File_سطر* fl, VFlagLogicPacked, صحيح wantwidth)
        : ASTGEN_SUPER_BasicDType(fl) {
        init(VBasicDTypeKwd::LOGIC, VSigning::NOSIGN, wantwidth, -1, nullptr);
    }
    AstBasicDType(File_سطر* fl, VFlagLogic2StatePacked, صحيح wantwidth)
        : ASTGEN_SUPER_BasicDType(fl) {
        init(VBasicDTypeKwd::LOGIC2STATE, VSigning::NOSIGN, wantwidth, -1, nullptr);
    }
    AstBasicDType(File_سطر* fl, VFlagBitPacked, صحيح wantwidth)
        : ASTGEN_SUPER_BasicDType(fl) {
        init(VBasicDTypeKwd::BIT, VSigning::NOSIGN, wantwidth, -1, nullptr);
    }
    AstBasicDType(File_سطر* fl, VBasicDTypeKwd kwd, VSigning numer, صحيح wantwidth, صحيح widthmin)
        : ASTGEN_SUPER_BasicDType(fl) {
        init(kwd, numer, wantwidth, widthmin, nullptr);
    }
    AstBasicDType(File_سطر* fl, VBasicDTypeKwd kwd, VSigning numer, VNumRange range, صحيح widthmin)
        : ASTGEN_SUPER_BasicDType(fl) {
        init(kwd, numer, range.elements(), widthmin, nullptr);
        m.m_nrange = range;  // as init() presumes lsb==0, but range.lsb() might not be
    }
    // See also addRange in verilog.y
خاص:
    فراغ init(VBasicDTypeKwd kwd, VSigning numer, صحيح wantwidth, صحيح wantwidthmin,
              AstRange* rangep);

عام:
    ASTGEN_MEMBERS_AstBasicDType;
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    // width/widthMin/numeric compared elsewhere
    bool same_عقدة(ثابت Ast_عقدة* samep) ثابت override;
    bool similar_DType_عقدة(ثابت Ast_عقدة_DType* samep) ثابت override;
    string name() ثابت override VL_MT_STABLE { أرجع m.m_keyword.typeName(); }
    string prettyDTypeName(bool full) ثابت override;
    ثابت حرف* broken() ثابت override {
        BROKEN_RTN(dtypep() != this);
        BROKEN_RTN(v3Global.widthMinUsage() == VWidthMinUsage::VERILOG_WIDTH
                   && widthMin() > width());
        أرجع nullptr;
    }
    فراغ setSignedState(ثابت VSigning& signst) {
        // Note NOSIGN does NOT change the state; this is required by the parser
        إذا (signst == VSigning::UNSIGNED) {
            numeric(signst);
        } وإلا إذا (signst == VSigning::SIGNED) {
            numeric(signst);
        }
    }
    // METHODS
    AstBasicDType* basicp() ثابت override VL_MT_STABLE { أرجع (AstBasicDType*)this; }
    // (Slow) recurses - Structure alignment 1,2,4 or 8 bytes (arrays affect this)
    صحيح widthAlignBytes() ثابت override;
    // (Slow) recurses - Width in bytes rounding up 1,2,4,8,12,...
    صحيح widthTotalBytes() ثابت override;
    bool isFourstate() ثابت override { أرجع keyword().isFourstate(); }
    VBasicDTypeKwd keyword() ثابت VL_MT_SAFE {  // Avoid using - use isSomething accessors instead
        أرجع m.m_keyword;
    }
    bool isBitLogic() ثابت { أرجع keyword().isBitLogic(); }
    bool isBit() ثابت { أرجع keyword().isBit(); }
    bool isCHandle() ثابت VL_MT_STABLE { أرجع keyword().isCHandle(); }
    bool isDouble() ثابت VL_MT_STABLE { أرجع keyword().isDouble(); }
    bool isEvent() ثابت VL_MT_STABLE { أرجع keyword() == VBasicDTypeKwd::EVENT; }
    bool isForkSync() ثابت VL_MT_SAFE { أرجع keyword() == VBasicDTypeKwd::FORK_SYNC; }
    bool isProcessRef() ثابت VL_MT_SAFE { أرجع keyword() == VBasicDTypeKwd::PROCESS_REFERENCE; }
    bool isDelayScheduler() ثابت VL_MT_SAFE {
        أرجع keyword() == VBasicDTypeKwd::DELAY_SCHEDULER;
    }
    bool isTriggerScheduler() ثابت VL_MT_SAFE {
        أرجع keyword() == VBasicDTypeKwd::TRIGGER_SCHEDULER;
    }
    bool isDynamicTriggerScheduler() ثابت VL_MT_SAFE {
        أرجع keyword() == VBasicDTypeKwd::DYNAMIC_TRIGGER_SCHEDULER;
    }
    bool isRandomGenerator() ثابت VL_MT_SAFE {
        أرجع keyword() == VBasicDTypeKwd::RANDOM_GENERATOR;
    }
    bool isStdRandomGenerator() ثابت VL_MT_SAFE {
        أرجع keyword() == VBasicDTypeKwd::RANDOM_STDGENERATOR;
    }
    bool isCovergroupInstHandle() ثابت VL_MT_SAFE {
        أرجع keyword() == VBasicDTypeKwd::COVERGROUP_INSTHANDLE;
    }
    bool isOpaque() ثابت VL_MT_SAFE { أرجع keyword().isOpaque(); }
    bool isString() ثابت VL_MT_STABLE { أرجع keyword().isString(); }
    bool isZeroInit() ثابت { أرجع keyword().isZeroInit(); }
    bool isRanged() ثابت { أرجع rangep() || m.m_nrange.ranged(); }
    bool isDpiBitVec() ثابت {  // DPI uses svBitVecVal
        أرجع keyword() == VBasicDTypeKwd::BIT && isRanged();
    }
    bool isDpiLogicVec() ثابت {  // DPI uses svLogicVecVal
        أرجع keyword().isFourstate() && !(keyword() == VBasicDTypeKwd::LOGIC && !isRanged());
    }
    bool isDpiCLayout() ثابت {
        أرجع keyword().isDpiCLayout()
               || (keyword().isSameish(VBasicDTypeKwd::LOGIC) && !isRanged());
    }
    bool isDpiPrimitive() ثابت {  // DPI uses a primitive type
        أرجع !isDpiBitVec() && !isDpiLogicVec();
    }
    // Generally the lo/hi/left/right funcs should be used instead of nrange()
    ثابت VNumRange& nrange() ثابت { أرجع m.m_nrange; }
    مضمن صحيح hi() ثابت;
    مضمن صحيح lo() ثابت;
    مضمن صحيح elements() ثابت;
    صحيح يسار() ثابت { أرجع ascending() ? lo() : hi(); }  // How to show a declaration
    صحيح يمين() ثابت { أرجع ascending() ? hi() : lo(); }
    مضمن bool ascending() ثابت;
    bool implicit() ثابت { أرجع keyword() == VBasicDTypeKwd::LOGIC_IMPLICIT; }
    bool untyped() ثابت { أرجع keyword() == VBasicDTypeKwd::UNTYPED; }
    VNumRange declRange() ثابت { أرجع isRanged() ? VNumRange{يسار(), يمين()} : VNumRange{}; }
    فراغ cvtRangeConst();  // Convert to smaller representation
    bool isCompound() ثابت override { أرجع isString(); }
    bool isIntegralOrPacked() ثابت override { أرجع keyword().isIntNumeric(); }
};
class AstBracketArrayDType final : عام Ast_عقدة_DType {
    // Associative/Queue/Normal array data type, ie "[dtype_or_expr]"
    // only for early parsing then becomes another data type
    // @astgen op1 := childDTypep : Optional[AstNodeDType] // moved to refDTypep() in V3Width
    // @astgen op2 := elementsp : AstNode  // Number of elements in array
عام:
    AstBracketArrayDType(File_سطر* fl, VFlagChildDType, Ast_عقدة_DType* childDTypep,
                         Ast_عقدة* elementsp)
        : ASTGEN_SUPER_BracketArrayDType(fl) {
        this->childDTypep(childDTypep);
        this->elementsp(elementsp);
    }
    ASTGEN_MEMBERS_AstBracketArrayDType;
    bool similar_DType_عقدة(ثابت Ast_عقدة_DType* samep) ثابت override { أرجع this == samep; }
    Ast_عقدة_DType* subDTypep() ثابت override VL_MT_STABLE { أرجع childDTypep(); }
    // METHODS
    // Will be removed in V3Width, which relies on this
    // being a child not a dtype pointed node
    bool maybePointedTo() ثابت override VL_MT_SAFE { أرجع false; }
    AstBasicDType* basicp() ثابت override VL_MT_STABLE { أرجع nullptr; }
    صحيح widthAlignBytes() ثابت override { V3ERROR_NA_RETURN(0); }
    صحيح widthTotalBytes() ثابت override { V3ERROR_NA_RETURN(0); }
    bool isCompound() ثابت override { أرجع true; }
};
class AstCDType final : عام Ast_عقدة_DType {
    // Raw "C" data type passed directly to output
    string m_name;  // Name of data type, printed when do V3EmitC
عام:
    AstCDType(File_سطر* fl, ثابت string& name)
        : ASTGEN_SUPER_CDType(fl)
        , m_name{name} {
        dtypep(this);
    }

    ASTGEN_MEMBERS_AstCDType;
    bool same_عقدة(ثابت Ast_عقدة* samep) ثابت override {
        ثابت AstCDType* ثابت asamep = VN_DBG_AS(samep, CDType);
        أرجع m_name == asamep->m_name;
    }
    bool similar_DType_عقدة(ثابت Ast_عقدة_DType* samep) ثابت override { أرجع same_عقدة(samep); }
    string name() ثابت override VL_MT_STABLE { أرجع m_name; }
    string prettyDTypeName(bool) ثابت override { أرجع m_name; }
    // METHODS
    AstBasicDType* basicp() ثابت override VL_MT_STABLE { أرجع nullptr; }
    صحيح widthAlignBytes() ثابت override { أرجع 8; }  // Assume
    صحيح widthTotalBytes() ثابت override { أرجع 8; }  // Assume
    bool isCompound() ثابت override { أرجع true; }
    // Return the global runtime type storing the requested number of bits.
    ساكن string typeToHold(صحيح width) {
        إذا (width <= 8) أرجع "::CData";
        إذا (width <= 16) أرجع "::SData";
        إذا (width <= VL_IDATASIZE) أرجع "::IData";
        إذا (width <= VL_QUADSIZE) أرجع "::QData";

        أرجع "::VlWide<" + std::to_string(VL_كلمات_I(width)) + ">";
    }
};
class AstClassRefDType final : عام Ast_عقدة_DType {
    // Reference to a class
    // @astgen op1 := paramsp: List[AstPin]
    //
    // @astgen ptr := m_classp : Optional[AstClass]  // data type pointed to, BELOW the AstTypedef
    // @astgen ptr := m_classOrPackagep : Optional[AstNodeModule]  // Package hierarchy
    bool m_rawPointer = false;  // Emit as a non-owning C++ pointer rather than VlClassRef
عام:
    AstClassRefDType(File_سطر* fl, AstClass* classp, AstPin* paramsp)
        : ASTGEN_SUPER_ClassRefDType(fl)
        , m_classp{classp} {
        dtypep(this);
        addParamsp(paramsp);
    }
    ASTGEN_MEMBERS_AstClassRefDType;
    // METHODS
    bool same_عقدة(ثابت Ast_عقدة* samep) ثابت override {
        ثابت AstClassRefDType* ثابت asamep = VN_DBG_AS(samep, ClassRefDType);
        أرجع (m_classp == asamep->m_classp && m_classOrPackagep == asamep->m_classOrPackagep
                && m_rawPointer == asamep->m_rawPointer);
    }
    bool similar_DType_عقدة(ثابت Ast_عقدة_DType* samep) ثابت override;
    فراغ dump(std::ostream& str = std::cout) ثابت override;
    فراغ dumpJson(std::ostream& str = std::cout) ثابت override;
    فراغ dumpSmall(std::ostream& str) ثابت override;
    string prettyDTypeName(bool full) ثابت override;
    string pretty_Name_رسالة() ثابت override;
    string name() ثابت override VL_MT_STABLE;
    AstBasicDType* basicp() ثابت override VL_MT_STABLE { أرجع nullptr; }
    صحيح widthAlignBytes() ثابت override { أرجع 0; }
    صحيح widthTotalBytes() ثابت override { أرجع 0; }
    Ast_عقدة_DType* virtRefDTypep() ثابت override { أرجع nullptr; }
    فراغ virtRefDTypep(Ast_عقدة_DType* nodep) override {}
    Ast_عقدة_DType* subDTypep() ثابت override VL_MT_STABLE { أرجع nullptr; }
    Ast_عقدة_Module* classOrPackagep() ثابت { أرجع m_classOrPackagep; }
    فراغ classOrPackagep(Ast_عقدة_Module* nodep) { m_classOrPackagep = nodep; }
    AstClass* classp() ثابت VL_MT_STABLE { أرجع m_classp; }
    فراغ classp(AstClass* nodep) { m_classp = nodep; }
    bool rawPointer() ثابت { أرجع m_rawPointer; }
    فراغ rawPointer(bool flag) { m_rawPointer = flag; }
    ساكن فراغ selfTest();
    bool isCompound() ثابت override { أرجع true; }
};
class AstConstDType final : عام Ast_عقدة_DType {
    // const data type, ie "const some_dtype var_name [2:0]"
    // ConstDType are removed in V3LinkLValue and become AstVar::isConst.
    // When more generic types are supported AstConstDType will be propagated further.
    // @astgen op1 := childDTypep : Optional[AstNodeDType]
    //
    // @astgen ptr := m_refDTypep : Optional[AstNodeDType]  // Inherit from this base data type
عام:
    AstConstDType(File_سطر* fl, VFlagChildDType, Ast_عقدة_DType* dtp)
        : ASTGEN_SUPER_ConstDType(fl) {
        childDTypep(dtp);  // Only for parser
        refDTypep(nullptr);  // V3Width will resolve
        dtypep(nullptr);  // V3Width will resolve
        widthFromSub(subDTypep());
    }
    ASTGEN_MEMBERS_AstConstDType;
    ثابت حرف* broken() ثابت override {
        BROKEN_RTN(!((m_refDTypep && !childDTypep()) || (!m_refDTypep && childDTypep())));
        أرجع nullptr;
    }
    bool same_عقدة(ثابت Ast_عقدة* samep) ثابت override {
        ثابت AstConstDType* ثابت sp = VN_DBG_AS(samep, ConstDType);
        أرجع (m_refDTypep == sp->m_refDTypep);
    }
    bool similar_DType_عقدة(ثابت Ast_عقدة_DType* samep) ثابت override {
        أرجع skipRefp()->similarDType(samep->skipRefp());
    }
    Ast_عقدة_DType* getChildDTypep() ثابت override { أرجع childDTypep(); }
    Ast_عقدة_DType* subDTypep() ثابت override VL_MT_STABLE {
        أرجع m_refDTypep ? m_refDTypep : childDTypep();
    }
    فراغ refDTypep(Ast_عقدة_DType* nodep) { m_refDTypep = nodep; }
    Ast_عقدة_DType* virtRefDTypep() ثابت override { أرجع m_refDTypep; }
    فراغ virtRefDTypep(Ast_عقدة_DType* nodep) override { refDTypep(nodep); }
    // METHODS
    AstBasicDType* basicp() ثابت override VL_MT_STABLE { أرجع subDTypep()->basicp(); }
    صحيح widthAlignBytes() ثابت override { أرجع subDTypep()->widthAlignBytes(); }
    صحيح widthTotalBytes() ثابت override { أرجع subDTypep()->widthTotalBytes(); }
    bool isCompound() ثابت override {
        v3fatalSrc("call isCompound on subdata type, not reference");
        أرجع false;
    }
};
class AstConstraintRefDType final : عام Ast_عقدة_DType {
    // For e.g. a reference to constraint for constraint_mode
عام:
    explicit AstConstraintRefDType(File_سطر* fl)
        : ASTGEN_SUPER_ConstraintRefDType(fl) {
        dtypep(this);
    }
    ASTGEN_MEMBERS_AstConstraintRefDType;
    bool hasDType() ثابت override VL_MT_SAFE { أرجع true; }
    bool maybePointedTo() ثابت override VL_MT_SAFE { أرجع true; }
    bool undead() ثابت override { أرجع true; }
    Ast_عقدة_DType* subDTypep() ثابت override VL_MT_STABLE { أرجع nullptr; }
    Ast_عقدة_DType* virtRefDTypep() ثابت override { أرجع nullptr; }
    فراغ virtRefDTypep(Ast_عقدة_DType* nodep) override {}
    bool similar_DType_عقدة(ثابت Ast_عقدة_DType* samep) ثابت override { أرجع this == samep; }
    AstBasicDType* basicp() ثابت override VL_MT_STABLE { أرجع nullptr; }
    صحيح widthAlignBytes() ثابت override { أرجع 1; }
    صحيح widthTotalBytes() ثابت override { أرجع 1; }
    bool isCompound() ثابت override { أرجع false; }
};
class AstCoverCrossDType final : عام Ast_عقدة_DType {
    // Borrowed pointer to VlCoverCrossT<...> or construction-time VlCoverCrossDyn.
    ثابت uint32_t m_dimensions;
    ثابت uint32_t m_tuples;  // Fixed capacity number of cross tuples (VlCoverCrossT's Tuples)
    ثابت uint32_t m_bins;  // Fixed capacity number of explicit bins (VlCoverCrossT's Bins)
    ثابت uint32_t m_autoBins;  // Fixed capacity number of auto bins (VlCoverCrossT's AutoBins)
    ثابت uint64_t m_bin_كلمات;  // Fixed capacity selection words (VlCoverCrossT's BinWords)
    ثابت bool m_dynamic;  // Bin layout is determined at covergroup construction

عام:
    AstCoverCrossDType(File_سطر* fl, uint32_t dimensions, uint32_t tuples, uint32_t bins,
                       uint32_t autoBins, uint64_t bin_كلمات, bool dynamic = false)
        : ASTGEN_SUPER_CoverCrossDType(fl)
        , m_dimensions{dimensions}
        , m_tuples{tuples}
        , m_bins{bins}
        , m_autoBins{autoBins}
        , m_bin_كلمات{bin_كلمات}
        , m_dynamic{dynamic} {
        dtypep(this);
    }
    ASTGEN_MEMBERS_AstCoverCrossDType;
    ثابت حرف* broken() ثابت override {
        BROKEN_RTN(m_dimensions == 0);
        أرجع nullptr;
    }
    bool same_عقدة(ثابت Ast_عقدة* samep) ثابت override {  // LCOV_EXCL_START
        ثابت AstCoverCrossDType* ثابت sp = VN_DBG_AS(samep, CoverCrossDType);
        أرجع dimensions() == sp->dimensions() && tuples() == sp->tuples() && bins() == sp->bins()
               && autoBins() == sp->autoBins() && bin_كلمات() == sp->bin_كلمات()
               && isDynamic() == sp->isDynamic();
    }  // LCOV_EXCL_STOP
    bool similar_DType_عقدة(ثابت Ast_عقدة_DType* samep) ثابت override { أرجع this == samep; }
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    فراغ dumpSmall(std::ostream& str) ثابت override;
    uint32_t dimensions() ثابت { أرجع m_dimensions; }
    uint32_t tuples() ثابت { أرجع m_tuples; }
    uint32_t bins() ثابت { أرجع m_bins; }
    uint32_t autoBins() ثابت { أرجع m_autoBins; }
    uint64_t bin_كلمات() ثابت { أرجع m_bin_كلمات; }
    bool isDynamic() ثابت { أرجع m_dynamic; }
    string cpp_Template_وسائط() ثابت;
    AstBasicDType* basicp() ثابت override VL_MT_STABLE { أرجع nullptr; }
    صحيح widthAlignBytes() ثابت override { أرجع حجم(فراغ*); }
    صحيح widthTotalBytes() ثابت override { أرجع حجم(فراغ*); }
    bool isCompound() ثابت override { أرجع true; }
};
class AstCoverpointDType final : عام Ast_عقدة_DType {
    // Borrowed pointer to a covergroup coverpoint runtime, 'VlCoverpointT<hitBound>*'.
    // Follows pattern of AstQueueDType in capturing the compile-time max bin overlap
    // template argument.
    uint32_t m_hitBound;  // VlCoverpointT<> template argument; hit list size, >= 1
عام:
    AstCoverpointDType(File_سطر* fl, uint32_t hitBound)
        : ASTGEN_SUPER_CoverpointDType(fl)
        , m_hitBound{hitBound} {
        dtypep(this);
    }
    ASTGEN_MEMBERS_AstCoverpointDType;
    ثابت حرف* broken() ثابت override {
        BROKEN_RTN(m_hitBound < 1);
        أرجع nullptr;
    }
    // V3Covergroup interns these one-per-hitBound into the type table, so identity is
    // equality; there is never a second node with the same bound to compare against.
    bool similar_DType_عقدة(ثابت Ast_عقدة_DType* samep) ثابت override { أرجع this == samep; }
    فراغ dumpSmall(std::ostream& str) ثابت override;
    // ACCESSORS
    uint32_t hitBound() ثابت { أرجع m_hitBound; }
    // METHODS
    AstBasicDType* basicp() ثابت override VL_MT_STABLE { أرجع nullptr; }
    صحيح widthAlignBytes() ثابت override { أرجع حجم(فراغ*); }
    صحيح widthTotalBytes() ثابت override { أرجع حجم(فراغ*); }
    bool isCompound() ثابت override { أرجع true; }
};
class AstDefImplicitDType final : عام Ast_عقدة_DType {
    // For parsing enum/struct/unions that are declared with a variable rather than typedef
    // This allows "var enum {...} a,b" to share the enum definition for both variables
    // After link, these become typedefs
    // @astgen op1 := childDTypep : Optional[AstNodeDType]
    string m_name;  // Data type name
    ثابت صحيح m_unique_رقم;  // Unique ID distinguishing this dtype instance, for hashing/naming

عام:
    AstDefImplicitDType(File_سطر* fl, ثابت string& name, VFlagChildDType, Ast_عقدة_DType* dtp)
        : ASTGEN_SUPER_DefImplicitDType(fl)
        , m_name{name}
        , m_unique_رقم{unique_رقم_Inc()} {
        childDTypep(dtp);  // Only for parser
        dtypep(nullptr);  // V3Width will resolve
    }
    AstDefImplicitDType(ثابت AstDefImplicitDType& other)
        : Ast_عقدة_DType(other)
        , m_name{other.m_name}
        , m_unique_رقم{unique_رقم_Inc()} {}
    ASTGEN_MEMBERS_AstDefImplicitDType;
    صحيح unique_رقم() ثابت { أرجع m_unique_رقم; }
    bool same_عقدة(ثابت Ast_عقدة* samep) ثابت override {
        ثابت AstDefImplicitDType* ثابت sp = VN_DBG_AS(samep, DefImplicitDType);
        أرجع unique_رقم() == sp->unique_رقم();
    }
    bool similar_DType_عقدة(ثابت Ast_عقدة_DType* samep) ثابت override { أرجع same_عقدة(samep); }
    Ast_عقدة_DType* getChildDTypep() ثابت override { أرجع childDTypep(); }
    Ast_عقدة_DType* subDTypep() ثابت override VL_MT_STABLE {
        أرجع dtypep() ? dtypep() : childDTypep();
    }
    // METHODS
    // op1 = Range of variable
    Ast_عقدة_DType* dtypeSkipRefp() ثابت { أرجع dtypep()->skipRefp(); }
    AstBasicDType* basicp() ثابت override VL_MT_STABLE { أرجع subDTypep()->basicp(); }
    صحيح widthAlignBytes() ثابت override { أرجع dtypep()->widthAlignBytes(); }
    صحيح widthTotalBytes() ثابت override { أرجع dtypep()->widthTotalBytes(); }
    string name() ثابت override VL_MT_STABLE { أرجع m_name; }
    فراغ name(ثابت string& flag) override { m_name = flag; }
    bool isCompound() ثابت override { أرجع false; }
};
class AstDynArrayDType final : عام Ast_عقدة_DType {
    // Dynamic array data type, ie "[]"
    // @astgen op1 := childDTypep : Optional[AstNodeDType] // moved to refDTypep() in V3Width
    //
    // @astgen ptr := m_refDTypep : Optional[AstNodeDType]  // Elements of this type (post-width)
عام:
    AstDynArrayDType(File_سطر* fl, VFlagChildDType, Ast_عقدة_DType* dtp)
        : ASTGEN_SUPER_DynArrayDType(fl) {
        childDTypep(dtp);  // Only for parser
        refDTypep(nullptr);
        dtypep(nullptr);  // V3Width will resolve
    }
    AstDynArrayDType(File_سطر* fl, Ast_عقدة_DType* dtp)
        : ASTGEN_SUPER_DynArrayDType(fl) {
        refDTypep(dtp);
        dtypep(nullptr);  // V3Width will resolve
    }
    ASTGEN_MEMBERS_AstDynArrayDType;
    ثابت حرف* broken() ثابت override {
        BROKEN_RTN(!((m_refDTypep && !childDTypep()) || (!m_refDTypep && childDTypep())));
        أرجع nullptr;
    }
    bool same_عقدة(ثابت Ast_عقدة* samep) ثابت override {
        ثابت AstDynArrayDType* ثابت asamep = VN_DBG_AS(samep, DynArrayDType);
        إذا (!asamep->subDTypep()) أرجع false;
        أرجع subDTypep() == asamep->subDTypep();
    }
    bool similar_DType_عقدة(ثابت Ast_عقدة_DType* samep) ثابت override {
        ثابت AstDynArrayDType* ثابت asamep = VN_DBG_AS(samep, DynArrayDType);
        أرجع asamep->subDTypep() && subDTypep()->similarDType(asamep->subDTypep());
    }
    string prettyDTypeName(bool full) ثابت override;
    فراغ dumpSmall(std::ostream& str) ثابت override;
    Ast_عقدة_DType* getChildDTypep() ثابت override { أرجع childDTypep(); }
    Ast_عقدة_DType* subDTypep() ثابت override VL_MT_STABLE {
        أرجع m_refDTypep ? m_refDTypep : childDTypep();
    }
    bool isAggregateType() ثابت override { أرجع true; }
    فراغ refDTypep(Ast_عقدة_DType* nodep) { m_refDTypep = nodep; }
    Ast_عقدة_DType* virtRefDTypep() ثابت override { أرجع m_refDTypep; }
    فراغ virtRefDTypep(Ast_عقدة_DType* nodep) override { refDTypep(nodep); }
    // METHODS
    AstBasicDType* basicp() ثابت override VL_MT_STABLE { أرجع nullptr; }
    صحيح widthAlignBytes() ثابت override { أرجع subDTypep()->widthAlignBytes(); }
    صحيح widthTotalBytes() ثابت override { أرجع subDTypep()->widthTotalBytes(); }
    bool isCompound() ثابت override { أرجع true; }
    bool isDynamicallySized() ثابت override { أرجع true; }
};
class AstEmptyQueueDType final : عام Ast_عقدة_DType {
    // For EmptyQueue
عام:
    explicit AstEmptyQueueDType(File_سطر* fl)
        : ASTGEN_SUPER_EmptyQueueDType(fl) {
        dtypep(this);
    }
    ASTGEN_MEMBERS_AstEmptyQueueDType;
    فراغ dumpSmall(std::ostream& str) ثابت override;
    bool hasDType() ثابت override VL_MT_SAFE { أرجع true; }
    bool maybePointedTo() ثابت override VL_MT_SAFE { أرجع true; }
    bool undead() ثابت override { أرجع true; }
    Ast_عقدة_DType* subDTypep() ثابت override VL_MT_STABLE { أرجع nullptr; }
    Ast_عقدة_DType* virtRefDTypep() ثابت override { أرجع nullptr; }
    فراغ virtRefDTypep(Ast_عقدة_DType* nodep) override {}
    bool similar_DType_عقدة(ثابت Ast_عقدة_DType* samep) ثابت override { أرجع this == samep; }
    AstBasicDType* basicp() ثابت override VL_MT_STABLE { أرجع nullptr; }
    صحيح widthAlignBytes() ثابت override { أرجع 1; }
    صحيح widthTotalBytes() ثابت override { أرجع 1; }
    bool isCompound() ثابت override { أرجع false; }
};
class AstEnumDType final : عام Ast_عقدة_DType {
    // Parents: TYPEDEF/MODULE
    // @astgen op1 := childDTypep : Optional[AstNodeDType]
    // @astgen op2 := itemsp : List[AstEnumItem]
    //
    // @astgen ptr := m_refDTypep : Optional[AstNodeDType]  // Elements of this type (post-width)
    string m_name;  // Name from upper typedef, if any
    string m_typedefName;  // Typedef's dtypeName() once moved off the typedef declaring it
    ثابت صحيح m_unique_رقم;  // Unique ID distinguishing this dtype instance, for hashing/naming

عام:
    AstEnumDType(File_سطر* fl, VFlagChildDType, Ast_عقدة_DType* dtp, AstEnumItem* itemsp)
        : ASTGEN_SUPER_EnumDType(fl)
        , m_unique_رقم{unique_رقم_Inc()} {
        childDTypep(dtp);  // Only for parser
        refDTypep(nullptr);
        addItemsp(itemsp);
        dtypep(nullptr);  // V3Width will resolve
        widthFromSub(subDTypep());
    }
    AstEnumDType(ثابت AstEnumDType& other)
        : Ast_عقدة_DType{other}
        , m_name{other.m_name}
        , m_typedefName{other.m_typedefName}
        , m_unique_رقم{unique_رقم_Inc()} {}
    ASTGEN_MEMBERS_AstEnumDType;

    ثابت حرف* broken() ثابت override;
    صحيح unique_رقم() ثابت { أرجع m_unique_رقم; }
    bool same_عقدة(ثابت Ast_عقدة* samep) ثابت override {
        ثابت AstEnumDType* ثابت sp = VN_DBG_AS(samep, EnumDType);
        أرجع unique_رقم() == sp->unique_رقم();
    }
    bool similar_DType_عقدة(ثابت Ast_عقدة_DType* samep) ثابت override { أرجع same_عقدة(samep); }
    Ast_عقدة_DType* getChildDTypep() ثابت override { أرجع childDTypep(); }
    Ast_عقدة_DType* subDTypep() ثابت override VL_MT_STABLE {
        أرجع m_refDTypep ? m_refDTypep : childDTypep();
    }
    فراغ refDTypep(Ast_عقدة_DType* nodep) { m_refDTypep = nodep; }
    Ast_عقدة_DType* virtRefDTypep() ثابت override { أرجع m_refDTypep; }
    فراغ virtRefDTypep(Ast_عقدة_DType* nodep) override { refDTypep(nodep); }
    string name() ثابت override VL_MT_STABLE { أرجع m_name; }
    فراغ name(ثابت string& flag) override { m_name = flag; }
    // Keep the name the typedef declaring it gives, as it moves off it to the type table
    فراغ typedefName(ثابت string& name) { m_typedefName = name; }
    فراغ dump(std::ostream& str = std::cout) ثابت override;
    فراغ dumpJson(std::ostream& str = std::cout) ثابت override;
    فراغ dumpSmall(std::ostream& str) ثابت override;
    string prettyDTypeName(bool full) ثابت override;
    // METHODS
    AstBasicDType* basicp() ثابت override VL_MT_STABLE { أرجع subDTypep()->basicp(); }
    صحيح widthAlignBytes() ثابت override { أرجع subDTypep()->widthAlignBytes(); }
    صحيح widthTotalBytes() ثابت override { أرجع subDTypep()->widthTotalBytes(); }
    size_t item_عدد() ثابت {
        size_t عدد = 0;
        لكل (Ast_عقدة* itemp = itemsp(); itemp; itemp = itemp->nextp()) ++عدد;
        أرجع عدد;
    }
    bool isCompound() ثابت override { أرجع false; }
};

class AstIfaceGenericDType final : عام Ast_عقدة_DType {
    // Generic interface that will be replaced with AstIfaceRefDType
    File_سطر* m_modportFileline;  // Where modport token was
    // dist-ast-dump-suppress  // Part of name()
    string m_modportName;  // "" = no modport
عام:
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
    فراغ dumpSmall(std::ostream& str) ثابت override;
    bool hasDType() ثابت override VL_MT_SAFE { أرجع true; }
    bool maybePointedTo() ثابت override VL_MT_SAFE { أرجع true; }
    bool undead() ثابت override { أرجع true; }
    Ast_عقدة_DType* subDTypep() ثابت override VL_MT_STABLE { أرجع nullptr; }
    Ast_عقدة_DType* virtRefDTypep() ثابت override { أرجع nullptr; }
    فراغ virtRefDTypep(Ast_عقدة_DType* nodep) override {}
    bool similar_DType_عقدة(ثابت Ast_عقدة_DType* samep) ثابت override { أرجع this == samep; }
    AstBasicDType* basicp() ثابت override VL_MT_STABLE { أرجع nullptr; }
    صحيح widthAlignBytes() ثابت override { أرجع 1; }
    صحيح widthTotalBytes() ثابت override { أرجع 1; }
    string modportName() ثابت { أرجع m_modportName; }
    bool isModport() { أرجع !m_modportName.empty(); }
    bool isCompound() ثابت override { أرجع true; }
    File_سطر* modportFileline() ثابت { أرجع m_modportFileline; }
    string name() ثابت override { أرجع m_modportName; }
};

class AstIfaceRefDType final : عام Ast_عقدة_DType {
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
عام:
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
    فراغ dump(std::ostream& str = std::cout) ثابت override;
    فراغ dumpJson(std::ostream& str = std::cout) ثابت override;
    فراغ dumpSmall(std::ostream& str) ثابت override;
    string prettyDTypeName(bool full) ثابت override;
    AstBasicDType* basicp() ثابت override VL_MT_STABLE { أرجع nullptr; }
    bool similar_DType_عقدة(ثابت Ast_عقدة_DType* samep) ثابت override {
        // Each occurrence of a virtual interface type parses to its own node,
        // so pointer identity is insufficient; compare the referenced
        // interface instead, same equivalence as AstNode::computeCastable uses.
        ثابت AstIfaceRefDType* ثابت asamep = VN_DBG_AS(samep, IfaceRefDType);
        أرجع ifaceViaCellp() && ifaceViaCellp() == asamep->ifaceViaCellp()
               && modportName() == asamep->modportName() && isVirtual() == asamep->isVirtual();
    }
    صحيح widthAlignBytes() ثابت override { أرجع 0; }
    صحيح widthTotalBytes() ثابت override { أرجع 0; }
    bool isVirtual() ثابت { أرجع m_virtual; }
    فراغ isVirtual(bool flag) {
        m_virtual = flag;
        إذا (flag) v3Global.setHasVirtIfaces();
    }
    File_سطر* modportFileline() ثابت { أرجع m_modportFileline; }
    فراغ modportFileline(File_سطر* ثابت modportFileline) { m_modportFileline = modportFileline; }
    string cellName() ثابت { أرجع m_cellName; }
    فراغ cellName(ثابت string& name) { m_cellName = name; }
    string ifaceName() ثابت { أرجع m_ifaceName; }
    string ifaceNameQ() ثابت { أرجع "'" + prettyName(ifaceName()) + "'"; }
    فراغ ifaceName(ثابت string& name) { m_ifaceName = name; }
    فراغ modportName(ثابت string& modportName) { m_modportName = modportName; }
    string modportName() ثابت { أرجع m_modportName; }
    AstIface* ifaceViaCellp() ثابت;  // Use cellp or ifacep
    AstIface* ifacep() ثابت { أرجع m_ifacep; }
    فراغ ifacep(AstIface* nodep) { m_ifacep = nodep; }
    AstCell* cellp() ثابت { أرجع m_cellp; }
    فراغ cellp(AstCell* nodep) { m_cellp = nodep; }
    AstModport* modportp() ثابت { أرجع m_modportp; }
    فراغ modportp(AstModport* modportp) { m_modportp = modportp; }
    bool isModport() { أرجع !m_modportName.empty(); }
    bool isCompound() ثابت override { أرجع true; }  // But not relevant
};
class AstMemberDType final : عام Ast_عقدة_DType {
    // A member of a struct/union
    // PARENT: AstNodeUOrStructDType
    // @astgen op1 := childDTypep : Optional[AstNodeDType]
    // @astgen op3 := valuep : Optional[AstNode]
    //
    // @astgen ptr := m_refDTypep : Optional[AstNodeDType]  // Elements of this type (post-width)
    string m_name;  // Name of variable
    string m_tag;  // Holds the string of the verilator tag -- used in JSON output.
    صحيح m_lsb = -1;  // Within this level's packed struct, the LSB of the first bit of the member
    bool m_constrainedRand = false;  // Member has a constraint expression
    VRandAttr m_rand;  // Randomizability of this member (rand, randc, etc)
عام:
    AstMemberDType(File_سطر* fl, ثابت string& name, VFlagChildDType, Ast_عقدة_DType* dtp,
                   Ast_عقدة* valuep)
        : ASTGEN_SUPER_MemberDType(fl)
        , m_name{name} {
        childDTypep(dtp);  // Only for parser
        this->valuep(valuep);
        dtypep(nullptr);  // V3Width will resolve
        refDTypep(nullptr);
    }
    AstMemberDType(File_سطر* fl, ثابت string& name, Ast_عقدة_DType* dtp)
        : ASTGEN_SUPER_MemberDType(fl)
        , m_name{name} {
        UASSERT(dtp, "AstMember created with no dtype");
        refDTypep(dtp);
        dtypep(this);
        widthFromSub(subDTypep());
    }
    ASTGEN_MEMBERS_AstMemberDType;
    فراغ dump(std::ostream& str = std::cout) ثابت override;
    فراغ dumpJson(std::ostream& str = std::cout) ثابت override;
    فراغ dumpSmall(std::ostream& str) ثابت override;
    string prettyDTypeName(bool full) ثابت override;
    string name() ثابت override VL_MT_STABLE { أرجع m_name; }  // * = Var name
    bool hasDType() ثابت override VL_MT_SAFE { أرجع true; }
    bool maybePointedTo() ثابت override VL_MT_SAFE { أرجع true; }
    Ast_عقدة_DType* getChildDTypep() ثابت override { أرجع childDTypep(); }
    Ast_عقدة_UOr_Struct_DType* getChildStructp();
    Ast_عقدة_DType* subDTypep() ثابت override VL_MT_STABLE {
        أرجع m_refDTypep ? m_refDTypep : childDTypep();
    }
    فراغ refDTypep(Ast_عقدة_DType* nodep) { m_refDTypep = nodep; }
    Ast_عقدة_DType* virtRefDTypep() ثابت override { أرجع m_refDTypep; }
    فراغ virtRefDTypep(Ast_عقدة_DType* nodep) override { refDTypep(nodep); }
    bool similar_DType_عقدة(ثابت Ast_عقدة_DType* samep) ثابت override { أرجع this == samep; }
    //
    // (Slow) recurse down to find basic data type (Note don't need virtual -
    // AstVar isn't a NodeDType)
    AstBasicDType* basicp() ثابت override VL_MT_STABLE { أرجع subDTypep()->basicp(); }
    // op1 = Range of variable (Note don't need virtual - AstVar isn't a NodeDType)
    Ast_عقدة_DType* dtypeSkipRefp() ثابت { أرجع subDTypep()->skipRefp(); }
    // (Slow) recurses - Structure alignment 1,2,4 or 8 bytes (arrays affect this)
    صحيح widthAlignBytes() ثابت override { أرجع subDTypep()->widthAlignBytes(); }
    // (Slow) recurses - Width in bytes rounding up 1,2,4,8,12,...
    صحيح widthTotalBytes() ثابت override { أرجع subDTypep()->widthTotalBytes(); }
    // METHODS
    فراغ name(ثابت string& name) override { m_name = name; }
    string tag() ثابت override { أرجع m_tag; }
    فراغ tag(ثابت string& text) override { m_tag = text; }
    صحيح lsb() ثابت { أرجع m_lsb; }
    فراغ lsb(صحيح lsb) { m_lsb = lsb; }
    bool isCompound() ثابت override {
        v3fatalSrc("call isCompound on subdata type, not reference");
        أرجع false;
    }
    bool isConstrainedRand() ثابت { أرجع m_constrainedRand; }
    فراغ markConstrainedRand(bool flag) { m_constrainedRand = flag; }
    VRandAttr rand() ثابت { أرجع m_rand; }
    فراغ rand(ثابت VRandAttr flag) { m_rand = flag; }
};
class AstNBACommitQueueDType final : عام Ast_عقدة_DType {
    // @astgen ptr := m_subDTypep : AstNodeDType  // Type of the corresponding variable
    ثابت bool m_partial;  // Partial element update required

عام:
    AstNBACommitQueueDType(File_سطر* fl, Ast_عقدة_DType* subDTypep, bool partial)
        : ASTGEN_SUPER_NBACommitQueueDType(fl)
        , m_partial{partial}
        , m_subDTypep{subDTypep} {
        dtypep(this);
    }
    ASTGEN_MEMBERS_AstNBACommitQueueDType;
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    Ast_عقدة_DType* subDTypep() ثابت override VL_MT_STABLE { أرجع m_subDTypep; }
    bool partial() ثابت { أرجع m_partial; }
    bool same_عقدة(ثابت Ast_عقدة* samep) ثابت override {
        ثابت AstNBACommitQueueDType* ثابت asamep = VN_DBG_AS(samep, NBACommitQueueDType);
        أرجع m_partial == asamep->m_partial;
    }
    bool similar_DType_عقدة(ثابت Ast_عقدة_DType* samep) ثابت override { أرجع this == samep; }
    AstBasicDType* basicp() ثابت override VL_MT_STABLE { أرجع nullptr; }
    صحيح widthAlignBytes() ثابت override { أرجع 1; }
    صحيح widthTotalBytes() ثابت override { أرجع 24; }
    bool isCompound() ثابت override { أرجع true; }
};
class AstParamTypeDType final : عام Ast_عقدة_DType {
    // Parents: MODULE
    // A parameter type statement; much like a var or typedef
    // @astgen op1 := childDTypep : Optional[AstNodeDType]
    ثابت VVarType m_varType;  // Type of variable (for localparam vs. param)
    ثابت VFwdType m_fwdType;  // Forward type for lint check
    string m_name;  // Name of variable
عام:
    AstParamTypeDType(File_سطر* fl, VVarType type, VFwdType fwdType, ثابت string& name,
                      VFlagChildDType, Ast_عقدة_DType* dtp)
        : ASTGEN_SUPER_ParamTypeDType(fl)
        , m_varType{type}
        , m_fwdType{fwdType}
        , m_name{name} {
        childDTypep(dtp);  // Only for parser
        dtypep(nullptr);  // V3Width will resolve
    }
    ASTGEN_MEMBERS_AstParamTypeDType;
    فراغ dump(std::ostream& str = std::cout) ثابت override;
    فراغ dumpJson(std::ostream& str = std::cout) ثابت override;
    string prettyDTypeName(bool full) ثابت override;
    Ast_عقدة_DType* getChildDTypep() ثابت override { أرجع childDTypep(); }
    Ast_عقدة_DType* subDTypep() ثابت override VL_MT_STABLE {
        أرجع dtypep() ? dtypep() : childDTypep();
    }
    AstBasicDType* basicp() ثابت override VL_MT_STABLE { أرجع subDTypep()->basicp(); }
    bool similar_DType_عقدة(ثابت Ast_عقدة_DType* samep) ثابت override {
        ثابت AstParamTypeDType* ثابت sp = VN_DBG_AS(samep, ParamTypeDType);
        أرجع this->subDTypep()->similarDType(sp->subDTypep());
    }
    صحيح widthAlignBytes() ثابت override { أرجع dtypep()->widthAlignBytes(); }
    صحيح widthTotalBytes() ثابت override { أرجع dtypep()->widthTotalBytes(); }
    // METHODS
    string name() ثابت override VL_MT_STABLE { أرجع m_name; }
    bool maybePointedTo() ثابت override VL_MT_SAFE { أرجع true; }
    bool hasDType() ثابت override VL_MT_SAFE { أرجع true; }
    فراغ name(ثابت string& flag) override { m_name = flag; }
    VVarType varType() ثابت { أرجع m_varType; }  // * = Type of variable
    VFwdType fwdType() ثابت { أرجع m_fwdType; }
    bool isParam() ثابت { أرجع true; }
    bool isGParam() ثابت { أرجع (varType() == VVarType::GPARAM); }
    bool isCompound() ثابت override {
        v3fatalSrc("call isCompound on subdata type, not reference");
        أرجع false;
    }
};
class AstParseTypeDType final : عام Ast_عقدة_DType {
    // Parents: VAR
    // During parsing, this indicates the type of a parameter is a "parameter type"
    // e.g. the data type is a container of any data type
    ثابت VFwdType m_fwdType;  // Forward type for lint check
عام:
    explicit AstParseTypeDType(File_سطر* fl, VFwdType fwdType = VFwdType::NONE)
        : ASTGEN_SUPER_ParseTypeDType(fl)
        , m_fwdType{fwdType} {}
    ASTGEN_MEMBERS_AstParseTypeDType;
    Ast_عقدة_DType* dtypep() ثابت VL_MT_STABLE { أرجع nullptr; }
    // METHODS
    فراغ dump(std::ostream& str = std::cout) ثابت override;
    فراغ dumpJson(std::ostream& str = std::cout) ثابت override;
    bool similar_DType_عقدة(ثابت Ast_عقدة_DType* samep) ثابت override { أرجع this == samep; }
    AstBasicDType* basicp() ثابت override VL_MT_STABLE { أرجع nullptr; }
    صحيح widthAlignBytes() ثابت override { أرجع 0; }
    صحيح widthTotalBytes() ثابت override { أرجع 0; }
    bool isCompound() ثابت override {
        v3fatalSrc("call isCompound on subdata type, not reference");
        أرجع false;
    }
    VFwdType fwdType() ثابت { أرجع m_fwdType; }
};
class AstQueueDType final : عام Ast_عقدة_DType {
    // Queue array data type, ie "[ $ ]"
    // @astgen op1 := childDTypep : Optional[AstNodeDType] // moved to refDTypep() in V3Width
    // @astgen op2 := boundp : Optional[AstNodeExpr]
    //
    // @astgen ptr := m_refDTypep : Optional[AstNodeDType]  // Elements of this type (post-width)
عام:
    AstQueueDType(File_سطر* fl, VFlagChildDType, Ast_عقدة_DType* dtp, Ast_عقدة_تعبير* boundp)
        : ASTGEN_SUPER_QueueDType(fl) {
        childDTypep(dtp);
        this->boundp(boundp);
        refDTypep(nullptr);
        dtypep(nullptr);  // V3Width will resolve
    }
    AstQueueDType(File_سطر* fl, Ast_عقدة_DType* dtp, Ast_عقدة_تعبير* boundp)
        : ASTGEN_SUPER_QueueDType(fl) {
        this->boundp(boundp);
        refDTypep(dtp);
        dtypep(dtp);
    }
    ASTGEN_MEMBERS_AstQueueDType;
    ثابت حرف* broken() ثابت override {
        BROKEN_RTN(!((m_refDTypep && !childDTypep()) || (!m_refDTypep && childDTypep())));
        أرجع nullptr;
    }
    bool same_عقدة(ثابت Ast_عقدة* samep) ثابت override {
        ثابت AstQueueDType* ثابت asamep = VN_DBG_AS(samep, QueueDType);
        إذا (!asamep->subDTypep()) أرجع false;
        أرجع (subDTypep() == asamep->subDTypep());
    }
    bool similar_DType_عقدة(ثابت Ast_عقدة_DType* samep) ثابت override {
        ثابت AstQueueDType* ثابت asamep = VN_DBG_AS(samep, QueueDType);
        أرجع asamep->subDTypep() && subDTypep()->similarDType(asamep->subDTypep());
    }
    فراغ dumpSmall(std::ostream& str) ثابت override;
    string prettyDTypeName(bool full) ثابت override;
    Ast_عقدة_DType* getChildDTypep() ثابت override { أرجع childDTypep(); }
    Ast_عقدة_DType* subDTypep() ثابت override VL_MT_STABLE {
        أرجع m_refDTypep ? m_refDTypep : childDTypep();
    }
    bool isAggregateType() ثابت override { أرجع true; }
    فراغ refDTypep(Ast_عقدة_DType* nodep) { m_refDTypep = nodep; }
    مضمن صحيح boundConst() ثابت VL_MT_STABLE;
    Ast_عقدة_DType* virtRefDTypep() ثابت override { أرجع m_refDTypep; }
    فراغ virtRefDTypep(Ast_عقدة_DType* nodep) override { refDTypep(nodep); }
    // METHODS
    AstBasicDType* basicp() ثابت override VL_MT_STABLE { أرجع nullptr; }
    صحيح widthAlignBytes() ثابت override { أرجع subDTypep()->widthAlignBytes(); }
    صحيح widthTotalBytes() ثابت override { أرجع subDTypep()->widthTotalBytes(); }
    bool isCompound() ثابت override { أرجع true; }
    bool isDynamicallySized() ثابت override { أرجع true; }
};
class AstRefDType final : عام Ast_عقدة_DType {
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

عام:
    AstRefDType(File_سطر* fl, ثابت string& name)
        : ASTGEN_SUPER_RefDType(fl)
        , m_name{name} {}
    AstRefDType(File_سطر* fl, ثابت string& name, Ast_عقدة_تعبير* classOrPackagep, AstPin* paramsp)
        : ASTGEN_SUPER_RefDType(fl)
        , m_name{name} {
        classOrPackageOpp(classOrPackagep);
        addParamsp(paramsp);
    }
    class Flag_Type_Of_تعبير {};  // type(expr) for parser only
    AstRefDType(File_سطر* fl, Flag_Type_Of_تعبير, Ast_عقدة* typeofp)
        : ASTGEN_SUPER_RefDType(fl) {
        this->typeofp(typeofp);
        إذا (Ast_عقدة_DType* ثابت dtp = VN_CAST(typeofp, عقدة_DType)) refDTypep(dtp);
    }
    ASTGEN_MEMBERS_AstRefDType;
    // METHODS
    bool same_عقدة(ثابت Ast_عقدة* samep) ثابت override {
        ثابت AstRefDType* ثابت asamep = VN_DBG_AS(samep, RefDType);
        أرجع (m_typedefp == asamep->m_typedefp && m_refDTypep == asamep->m_refDTypep
                && m_name == asamep->m_name && m_classOrPackagep == asamep->m_classOrPackagep);
    }
    bool similar_DType_عقدة(ثابت Ast_عقدة_DType* samep) ثابت override {
        أرجع subDTypep()->similarDType(samep->subDTypep());
    }
    ثابت حرف* broken() ثابت override;
    فراغ dump(std::ostream& str = std::cout) ثابت override;
    فراغ dumpJson(std::ostream& str = std::cout) ثابت override;
    فراغ dumpSmall(std::ostream& str) ثابت override;
    string name() ثابت override VL_MT_STABLE { أرجع m_name; }
    string prettyDTypeName(bool full) ثابت override {
        أرجع subDTypep() ? subDTypep()->prettyDTypeName(full) : prettyName();
    }
    AstBasicDType* basicp() ثابت override VL_MT_STABLE {
        أرجع subDTypep() ? subDTypep()->basicp() : nullptr;
    }
    Ast_عقدة_DType* subDTypep() ثابت override VL_MT_STABLE;
    Ast_عقدة_DType* getChildDTypep() ثابت override { أرجع VN_CAST(typeofp(), عقدة_DType); }
    صحيح widthAlignBytes() ثابت override { أرجع dtypeSkipRefp()->widthAlignBytes(); }
    صحيح widthTotalBytes() ثابت override { أرجع dtypeSkipRefp()->widthTotalBytes(); }
    فراغ name(ثابت string& flag) override { m_name = flag; }
    Ast_عقدة_DType* dtypeSkipRefp() ثابت { أرجع subDTypep()->skipRefp(); }
    AstTypedef* typedefp() ثابت VL_MT_SAFE { أرجع m_typedefp; }
    فراغ typedefp(AstTypedef* nodep) { m_typedefp = nodep; }
    Ast_عقدة_DType* refDTypep() ثابت VL_MT_SAFE { أرجع m_refDTypep; }
    فراغ refDTypep(Ast_عقدة_DType* nodep) { m_refDTypep = nodep; }
    Ast_عقدة_DType* virtRefDTypep() ثابت override { أرجع refDTypep(); }
    فراغ virtRefDTypep(Ast_عقدة_DType* nodep) override { refDTypep(nodep); }
    Ast_عقدة_Module* classOrPackagep() ثابت { أرجع m_classOrPackagep; }
    فراغ classOrPackagep(Ast_عقدة_Module* nodep) { m_classOrPackagep = nodep; }
    ثابت VIfaceCaptureTag* captureTagp() ثابت { أرجع m_captureTagp; }
    فراغ captureTagp(ثابت VIfaceCaptureTag* tagp) { m_captureTagp = tagp; }
    bool isCompound() ثابت override {
        v3fatalSrc("call isCompound on subdata type, not reference");
        أرجع false;
    }
};
class AstRequireDType final : عام Ast_عقدة_DType {
    // @astgen op1 := lhsp : Optional[AstNode<AstNodeExpr|AstNodeDType>]
    //
    // Require a generic node type (typically AstParseRef become a type.
عام:
    AstRequireDType(File_سطر* fl, Ast_عقدة* lhsp)
        : ASTGEN_SUPER_RequireDType(fl) {
        this->lhsp(lhsp);
    }
    ASTGEN_MEMBERS_AstRequireDType;
    // METHODS
    bool similar_DType_عقدة(ثابت Ast_عقدة_DType* samep) ثابت override {
        ثابت AstRequireDType* ثابت asamep = VN_DBG_AS(samep, RequireDType);
        أرجع subDTypep()->similarDType(asamep->subDTypep());
    }
    AstBasicDType* basicp() ثابت override VL_MT_STABLE { أرجع nullptr; }
    Ast_عقدة_DType* subDTypep() ثابت override VL_MT_STABLE {
        // Used for recursive definition checking
        إذا (Ast_عقدة_DType* ثابت dtp = VN_CAST(lhsp(), عقدة_DType)) أرجع dtp;

        أرجع nullptr;
    }
    صحيح widthAlignBytes() ثابت override { V3ERROR_NA_RETURN(1); }
    صحيح widthTotalBytes() ثابت override { V3ERROR_NA_RETURN(1); }
    bool isCompound() ثابت override { V3ERROR_NA_RETURN(false); }
};
class AstSampleQueueDType final : عام Ast_عقدة_DType {
    // @astgen op1 := childDTypep : Optional[AstNodeDType] // moved to refDTypep() in V3Width
    //
    // @astgen ptr := m_refDTypep : Optional[AstNodeDType]  // Elements of this type (post-width)
عام:
    AstSampleQueueDType(File_سطر* fl, Ast_عقدة_DType* dtp)
        : ASTGEN_SUPER_SampleQueueDType(fl) {
        refDTypep(dtp);
        dtypep(dtp);
    }
    ASTGEN_MEMBERS_AstSampleQueueDType;
    ثابت حرف* broken() ثابت override {
        BROKEN_RTN(!((m_refDTypep && !childDTypep()) || (!m_refDTypep && childDTypep())));
        أرجع nullptr;
    }
    bool same_عقدة(ثابت Ast_عقدة* samep) ثابت override {
        ثابت AstSampleQueueDType* ثابت asamep = VN_DBG_AS(samep, SampleQueueDType);
        إذا (!asamep->subDTypep()) أرجع false;
        أرجع (subDTypep() == asamep->subDTypep());
    }
    bool similar_DType_عقدة(ثابت Ast_عقدة_DType* samep) ثابت override {
        ثابت AstSampleQueueDType* ثابت asamep = VN_DBG_AS(samep, SampleQueueDType);
        أرجع asamep->subDTypep() && subDTypep()->similarDType(asamep->subDTypep());
    }
    فراغ dumpSmall(std::ostream& str) ثابت override;
    Ast_عقدة_DType* getChildDTypep() ثابت override { أرجع childDTypep(); }
    // op1 = Range of variable
    Ast_عقدة_DType* subDTypep() ثابت override VL_MT_STABLE {
        أرجع m_refDTypep ? m_refDTypep : childDTypep();
    }
    فراغ refDTypep(Ast_عقدة_DType* nodep) { m_refDTypep = nodep; }
    Ast_عقدة_DType* virtRefDTypep() ثابت override { أرجع m_refDTypep; }
    فراغ virtRefDTypep(Ast_عقدة_DType* nodep) override { refDTypep(nodep); }
    // METHODS
    AstBasicDType* basicp() ثابت override VL_MT_STABLE { أرجع subDTypep()->basicp(); }
    صحيح widthAlignBytes() ثابت override { أرجع حجم(std::map<std::string, std::string>); }
    صحيح widthTotalBytes() ثابت override { أرجع حجم(std::map<std::string, std::string>); }
    bool isCompound() ثابت override { أرجع true; }
};
class AstStreamDType final : عام Ast_عقدة_DType {
    // Stream data type, used only as data type of stream operations
    // Should behave like AstPackArrayDType, but it doesn't have a size
عام:
    explicit AstStreamDType(File_سطر* fl)
        : ASTGEN_SUPER_StreamDType(fl) {
        dtypep(this);
    }
    ASTGEN_MEMBERS_AstStreamDType;
    فراغ dumpSmall(std::ostream& str) ثابت override;
    bool hasDType() ثابت override VL_MT_SAFE { أرجع true; }
    bool maybePointedTo() ثابت override VL_MT_SAFE { أرجع true; }
    bool undead() ثابت override { أرجع true; }
    Ast_عقدة_DType* subDTypep() ثابت override VL_MT_STABLE { أرجع nullptr; }
    Ast_عقدة_DType* virtRefDTypep() ثابت override { أرجع nullptr; }
    فراغ virtRefDTypep(Ast_عقدة_DType* nodep) override {}
    bool similar_DType_عقدة(ثابت Ast_عقدة_DType* samep) ثابت override { أرجع this == samep; }
    AstBasicDType* basicp() ثابت override VL_MT_STABLE { أرجع nullptr; }
    صحيح widthAlignBytes() ثابت override { أرجع 1; }
    صحيح widthTotalBytes() ثابت override { أرجع 1; }
    bool isCompound() ثابت override { أرجع false; }
};
class AstUnsizedArrayDType final : عام Ast_عقدة_DType {
    // Unsized/open-range Array data type, ie "some_dtype var_name []"
    // @astgen op1 := childDTypep : Optional[AstNodeDType] // moved to refDTypep() in V3Width
    //
    // @astgen ptr := m_refDTypep : Optional[AstNodeDType]  // Elements of this type (post-width)
عام:
    AstUnsizedArrayDType(File_سطر* fl, VFlagChildDType, Ast_عقدة_DType* dtp)
        : ASTGEN_SUPER_UnsizedArrayDType(fl) {
        childDTypep(dtp);  // Only for parser
        refDTypep(nullptr);
        dtypep(nullptr);  // V3Width will resolve
    }
    AstUnsizedArrayDType(File_سطر* fl, Ast_عقدة_DType* dtp)
        : ASTGEN_SUPER_UnsizedArrayDType(fl) {
        refDTypep(dtp);
        dtypep(nullptr);  // V3Width will resolve
    }
    ASTGEN_MEMBERS_AstUnsizedArrayDType;
    ثابت حرف* broken() ثابت override {
        BROKEN_RTN(!((m_refDTypep && !childDTypep()) || (!m_refDTypep && childDTypep())));
        أرجع nullptr;
    }
    bool same_عقدة(ثابت Ast_عقدة* samep) ثابت override;
    bool similar_DType_عقدة(ثابت Ast_عقدة_DType* samep) ثابت override;
    فراغ dumpSmall(std::ostream& str) ثابت override;
    Ast_عقدة_DType* getChildDTypep() ثابت override { أرجع childDTypep(); }
    Ast_عقدة_DType* subDTypep() ثابت override VL_MT_STABLE {
        أرجع m_refDTypep ? m_refDTypep : childDTypep();
    }
    فراغ refDTypep(Ast_عقدة_DType* nodep) { m_refDTypep = nodep; }
    Ast_عقدة_DType* virtRefDTypep() ثابت override { أرجع m_refDTypep; }
    فراغ virtRefDTypep(Ast_عقدة_DType* nodep) override { refDTypep(nodep); }
    // METHODS
    AstBasicDType* basicp() ثابت override VL_MT_STABLE { أرجع subDTypep()->basicp(); }
    صحيح widthAlignBytes() ثابت override { أرجع subDTypep()->widthAlignBytes(); }
    صحيح widthTotalBytes() ثابت override { أرجع subDTypep()->widthTotalBytes(); }
    bool isCompound() ثابت override { أرجع true; }
};
class AstVoidDType final : عام Ast_عقدة_DType {
    // For e.g. a function returning void
عام:
    explicit AstVoidDType(File_سطر* fl)
        : ASTGEN_SUPER_VoidDType(fl) {
        dtypep(this);
    }
    ASTGEN_MEMBERS_AstVoidDType;
    فراغ dumpSmall(std::ostream& str) ثابت override;
    bool hasDType() ثابت override VL_MT_SAFE { أرجع true; }
    bool maybePointedTo() ثابت override VL_MT_SAFE { أرجع true; }
    bool undead() ثابت override { أرجع true; }
    Ast_عقدة_DType* subDTypep() ثابت override VL_MT_STABLE { أرجع nullptr; }
    Ast_عقدة_DType* virtRefDTypep() ثابت override { أرجع nullptr; }
    فراغ virtRefDTypep(Ast_عقدة_DType* nodep) override {}
    bool similar_DType_عقدة(ثابت Ast_عقدة_DType* samep) ثابت override { أرجع true; }
    AstBasicDType* basicp() ثابت override VL_MT_STABLE { أرجع nullptr; }
    صحيح widthAlignBytes() ثابت override { أرجع 1; }
    صحيح widthTotalBytes() ثابت override { أرجع 1; }
    bool isCompound() ثابت override { أرجع false; }
};
class AstWildcardArrayDType final : عام Ast_عقدة_DType {
    // Wildcard index type associative array data type, ie "some_dtype var_name [*]"
    // @astgen op1 := childDTypep : Optional[AstNodeDType] // moved to refDTypep() in V3Width
    //
    // @astgen ptr := m_refDTypep : Optional[AstNodeDType]  // Elements of this type (post-width)
عام:
    AstWildcardArrayDType(File_سطر* fl, VFlagChildDType, Ast_عقدة_DType* dtp)
        : ASTGEN_SUPER_WildcardArrayDType(fl) {
        childDTypep(dtp);  // Only for parser
        refDTypep(nullptr);
        dtypep(nullptr);  // V3Width will resolve
    }
    ASTGEN_MEMBERS_AstWildcardArrayDType;
    ثابت حرف* broken() ثابت override {
        BROKEN_RTN(!((m_refDTypep && !childDTypep()) || (!m_refDTypep && childDTypep())));
        أرجع nullptr;
    }
    bool same_عقدة(ثابت Ast_عقدة* samep) ثابت override;
    bool similar_DType_عقدة(ثابت Ast_عقدة_DType* samep) ثابت override;
    فراغ dumpSmall(std::ostream& str) ثابت override;
    string prettyDTypeName(bool full) ثابت override;
    Ast_عقدة_DType* getChildDTypep() ثابت override { أرجع childDTypep(); }
    Ast_عقدة_DType* subDTypep() ثابت override VL_MT_STABLE {
        أرجع m_refDTypep ? m_refDTypep : childDTypep();
    }
    فراغ refDTypep(Ast_عقدة_DType* nodep) { m_refDTypep = nodep; }
    Ast_عقدة_DType* virtRefDTypep() ثابت override { أرجع m_refDTypep; }
    فراغ virtRefDTypep(Ast_عقدة_DType* nodep) override { refDTypep(nodep); }
    // METHODS
    AstBasicDType* basicp() ثابت override VL_MT_STABLE { أرجع subDTypep()->basicp(); }
    صحيح widthAlignBytes() ثابت override { أرجع حجم(std::map<std::string, std::string>); }
    صحيح widthTotalBytes() ثابت override { أرجع حجم(std::map<std::string, std::string>); }
    bool isCompound() ثابت override { أرجع true; }
    bool isDynamicallySized() ثابت override { أرجع true; }
};

// === AstNodeArrayDType ===
class AstPackArrayDType final : عام Ast_عقدة_Array_DType {
    // Packed array data type, ie "some_dtype [2:0] var_name"
عام:
    مضمن AstPackArrayDType(File_سطر* fl, VFlagChildDType, Ast_عقدة_DType* dtp, AstRange* rangep);
    مضمن AstPackArrayDType(File_سطر* fl, Ast_عقدة_DType* dtp, AstRange* rangep);
    ASTGEN_MEMBERS_AstPackArrayDType;
    string prettyDTypeName(bool full) ثابت override;
    bool isCompound() ثابت override { أرجع false; }
};
class AstUnpackArrayDType final : عام Ast_عقدة_Array_DType {
    // Array data type, ie "some_dtype var_name [2:0]"
    bool m_isCompound = false;  // Non-POD subDType, or parent requires compound
عام:
    AstUnpackArrayDType(File_سطر* fl, VFlagChildDType, Ast_عقدة_DType* dtp, AstRange* rangep)
        : ASTGEN_SUPER_UnpackArrayDType(fl) {
        childDTypep(dtp);  // Only for parser
        this->rangep(rangep);
        refDTypep(nullptr);
        dtypep(nullptr);  // V3Width will resolve
        // For backward compatibility AstNodeArrayDType and others inherit
        // width and signing from the subDType/base type
        widthFromSub(subDTypep());
    }
    AstUnpackArrayDType(File_سطر* fl, Ast_عقدة_DType* dtp, AstRange* rangep)
        : ASTGEN_SUPER_UnpackArrayDType(fl) {
        this->rangep(rangep);
        refDTypep(dtp);
        dtypep(this);
        // For backward compatibility AstNodeArrayDType and others inherit
        // width and signing from the subDType/base type
        widthFromSub(subDTypep());
    }
    ASTGEN_MEMBERS_AstUnpackArrayDType;
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    string prettyDTypeName(bool full) ثابت override;
    bool same_عقدة(ثابت Ast_عقدة* samep) ثابت override {
        ثابت AstUnpackArrayDType* ثابت sp = VN_DBG_AS(samep, UnpackArrayDType);
        أرجع m_isCompound == sp->m_isCompound;
    }
    bool similar_DType_عقدة(ثابت Ast_عقدة_DType* samep) ثابت override;
    bool isAggregateType() ثابت override { أرجع true; }
    // True iff 'otherp' is an array of the same shape, with
    // at least one dimension reversed ascending/descending.
    bool isReverseOf(ثابت Ast_عقدة_DType* otherp) ثابت;
    // Outer dimension comes first. The first element is this node.
    std::vector<AstUnpackArrayDType*> unpackDimensions();
    فراغ isCompound(bool flag) { m_isCompound = flag; }
    bool isCompound() ثابت override VL_MT_SAFE { أرجع m_isCompound; }
    bool isIntegralOrPacked() ثابت override { أرجع false; }
};

// === AstNodeUOrStructDType ===
class AstStructDType final : عام Ast_عقدة_UOr_Struct_DType {
عام:
    // VSigning below is mispurposed to indicate if packed or not
    AstStructDType(File_سطر* fl, VSigning numericUnpack)
        : ASTGEN_SUPER_StructDType(fl, numericUnpack) {}
    ASTGEN_MEMBERS_AstStructDType;
    string verilogKwd() ثابت override { أرجع "struct"; }
};
class AstUnionDType final : عام Ast_عقدة_UOr_Struct_DType {
    bool m_isSoft;  // Is a "union soft"
    bool m_isTagged;  // Is a "union tagged"

عام:
    // VSigning below is mispurposed to indicate if packed or not
    // isSoft implies packed
    AstUnionDType(File_سطر* fl, bool isSoft, bool isTagged, VSigning numericUnpack)
        : ASTGEN_SUPER_UnionDType(fl, numericUnpack)
        , m_isSoft{isSoft}
        , m_isTagged{isTagged} {
        packed(packed() | m_isSoft);
    }
    ASTGEN_MEMBERS_AstUnionDType;
    string verilogKwd() ثابت override { أرجع "union"; }
    bool isSoft() ثابت { أرجع m_isSoft; }
    bool isTagged() ثابت { أرجع m_isTagged; }
    bool same_عقدة(ثابت Ast_عقدة* samep) ثابت override;
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
};

#endif  // Guard
