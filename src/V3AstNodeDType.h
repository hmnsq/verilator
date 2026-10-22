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
#அந்தர்பாவ "V3Ast.h"  // This helps code analysis tools pick up symbols in V3Ast.h
#define VL_NOT_FINAL  // This #define fixes broken code folding in the CLion IDE
#endif

// === Abstract base node types (AstNode*) =====================================

class Ast_முனை_DType VL_NOT_FINAL : பொது Ast_முனை {
    // Ideally width() would migrate to BasicDType as that's where it makes sense,
    // but it's currently so prevalent in the code we leave it here.
    // Note the below members are included in AstTypeTable::Key lookups
    // dist-ast-dump-suppress  // Part of dumpSmall
    முழு m_width = 0;  // (also in AstTypeTable::Key) Bit width of operation
    // (also in AstTypeTable::Key) If unsized, bitwidth of minimum implementation
    // dist-ast-dump-suppress  // Part of dumpSmall
    முழு m_widthMin = 0;
    // dist-ast-dump-suppress  // Part of dumpSmall
    VSigning m_numeric;  // (also in AstTypeTable::Key) Node is signed
    // dist-ast-dump-suppress  // Part of dumpSmall
    bool m_generic = false;  // Simple globally referenced type, don't garbage collect
    // Unique number assigned to each dtype during creation for IEEE matching
    நிலையான முழு s_unique_இலக்கம்;

பாதுகாக்கப்பட்ட:
    // CONSTRUCTORS
    Ast_முனை_DType(VNType t, File_வரி* fl)
        : Ast_முனை{t, fl} {}

தனிப்பட்ட:
    // METHODS
    மாறிலி Ast_முனை_DType* skipRefIterp(bool skipConst, bool skipEnum,
                                     bool assertOn = true) மாறிலி VL_MT_STABLE;

பாதுகாக்கப்பட்ட:
    // METHODS
    மெய்நிகர் bool similar_DType_முனை(மாறிலி Ast_முனை_DType* samep) மாறிலி = 0;

பொது:
    ASTGEN_MEMBERS_Ast_முனை_DType;
    // ACCESSORS
    வெறுமை dump(std::ostream& str) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str) மாறிலி override;
    மெய்நிகர் வெறுமை dumpSmall(std::ostream& str) மாறிலி VL_MT_STABLE;
    bool hasDType() மாறிலி override VL_MT_SAFE { திரும்பு true; }
    /// Require VlUnpacked, instead of [] for POD elements.
    /// A non-POD object is always compound, but some POD elements
    /// are compound when methods calls operate on object, or when
    /// under another compound-requiring object e.g. class
    மெய்நிகர் bool isCompound() மாறிலி = 0;
    // Integral or packed, allowed inside an unpacked union/struct
    மெய்நிகர் bool isIntegralOrPacked() மாறிலி { திரும்பு !isCompound(); }
    // (Slow) recurse down to find basic data type
    மெய்நிகர் AstBasicDType* basicp() மாறிலி VL_MT_STABLE = 0;
    // (Slow) Recurse over MemberDType|ParamTypeDType|RefDType|ConstDType|EnumDType to other type
    மாறிலி Ast_முனை_DType* skipRefp() மாறிலி VL_MT_STABLE { திரும்பு skipRefIterp(true, true); }
    Ast_முனை_DType* skipRefp() VL_MT_STABLE {
        திரும்பு const_cast<Ast_முனை_DType*>(
            static_cast<மாறிலி Ast_முனை_DType*>(this)->skipRefIterp(true, true));
    }
    // (Slow) Recurse over MemberDType|ParamTypeDType|RefDType|ConstDType|EnumDType to other type,
    // Returns null if not resolved
    மாறிலி Ast_முனை_DType* skipRefOrNullp() மாறிலி VL_MT_STABLE {
        திரும்பு skipRefIterp(true, true, false);
    }
    Ast_முனை_DType* skipRefOrNullp() VL_MT_STABLE {
        திரும்பு const_cast<Ast_முனை_DType*>(
            static_cast<மாறிலி Ast_முனை_DType*>(this)->skipRefIterp(true, true, false));
    }
    // (Slow) Recurse over MemberDType|ParamTypeDType|RefDType|EnumDType to ConstDType
    மாறிலி Ast_முனை_DType* skipRefToConstp() மாறிலி { திரும்பு skipRefIterp(false, true); }
    Ast_முனை_DType* skipRefToConstp() {
        திரும்பு const_cast<Ast_முனை_DType*>(
            static_cast<மாறிலி Ast_முனை_DType*>(this)->skipRefIterp(false, true));
    }
    // (Slow) Recurse over MemberDType|ParamTypeDType|RefDType|ConstDType to EnumDType
    மாறிலி Ast_முனை_DType* skipRefToEnump() மாறிலி { திரும்பு skipRefIterp(true, false); }
    Ast_முனை_DType* skipRefToEnump() {
        திரும்பு const_cast<Ast_முனை_DType*>(
            static_cast<மாறிலி Ast_முனை_DType*>(this)->skipRefIterp(true, false));
    }
    // (Slow) Recurse over MemberDType|ParamTypeDType|RefDType|ConstDType to EnumDType,
    // Returns null if not resolved
    மாறிலி Ast_முனை_DType* skipRefToEnumOrNullp() மாறிலி { திரும்பு skipRefIterp(true, false, false); }
    // (Slow) Recurse over MemberDType|ParamTypeDType|RefDType to other type
    மாறிலி Ast_முனை_DType* skipRefToNonRefp() மாறிலி { திரும்பு skipRefIterp(false, false); }
    Ast_முனை_DType* skipRefToNonRefp() {
        திரும்பு const_cast<Ast_முனை_DType*>(
            static_cast<மாறிலி Ast_முனை_DType*>(this)->skipRefIterp(false, false));
    }
    // If array, returns element dtype, otherwise returns skipRef dtype
    // If skipRef is false, RefDTypes are not followed (safe before typedef linking)
    மாறிலி Ast_முனை_DType* elemDTypep(bool skipRef = true) மாறிலி VL_MT_STABLE;
    Ast_முனை_DType* elemDTypep(bool skipRef = true) VL_MT_STABLE {
        திரும்பு const_cast<Ast_முனை_DType*>(
            static_cast<மாறிலி Ast_முனை_DType*>(this)->elemDTypep(skipRef));
    }
    // (Slow) recurses - Structure alignment 1,2,4 or 8 bytes (arrays affect this)
    மெய்நிகர் முழு widthAlignBytes() மாறிலி = 0;
    // (Slow) recurses - Width in bytes rounding up 1,2,4,8,12,...
    மெய்நிகர் முழு widthTotalBytes() மாறிலி = 0;
    bool maybePointedTo() மாறிலி override VL_MT_SAFE { திரும்பு true; }
    // Iff has a non-null refDTypep(), as generic node function
    மெய்நிகர் Ast_முனை_DType* virtRefDTypep() மாறிலி { திரும்பு nullptr; }
    // Iff has refDTypep(), set as generic node function
    மெய்நிகர் வெறுமை virtRefDTypep(Ast_முனை_DType* nodep) {}
    // Iff has a non-null second dtypep, as generic node function
    மெய்நிகர் Ast_முனை_DType* virtRefDType2p() மாறிலி { திரும்பு nullptr; }
    // Iff has second dtype, set as generic node function
    மெய்நிகர் வெறுமை virtRefDType2p(Ast_முனை_DType* nodep) {}
    // Assignable equivalence.  Calls skipRefToNonRefp() during comparisons.
    bool similarDType(மாறிலி Ast_முனை_DType* samep) மாறிலி;
    // Iff has a non-null subDTypep(), as generic node function
    மெய்நிகர் Ast_முனை_DType* subDTypep() மாறிலி VL_MT_STABLE { திரும்பு nullptr; }
    மெய்நிகர் Ast_முனை_DType* subDType2p() மாறிலி VL_MT_STABLE { திரும்பு nullptr; }
    மெய்நிகர் bool isAggregateType() மாறிலி { திரும்பு false; }
    // True for unpacked, dynamic, queue, and associative arrays (not packed arrays)
    bool isNonPackedArray() மாறிலி;
    மெய்நிகர் bool isFourstate() மாறிலி;
    // Ideally an IEEE $typename
    மெய்நிகர் string prettyDTypeName(bool) மாறிலி { திரும்பு prettyTypeName(); }
    string prettyDTypeNameQ(bool full = false) மாறிலி { திரும்பு "'" + prettyDTypeName(full) + "'"; }
    string stateDTypeName() மாறிலி { திரும்பு this->isFourstate() ? "(4-state)" : "(2-state)"; }
    //
    // Changing the width may confuse the data type resolution, so must clear
    // TypeTable cache after use.
    வெறுமை widthForce(முழு width, முழு widthMin) {
        m_width = width;
        m_widthMin = widthMin;
    }
    // For backward compatibility inherit width and signing from the subDType/base type
    வெறுமை widthFromSub(மாறிலி Ast_முனை_DType* nodep) {
        m_width = nodep->m_width;
        m_widthMin = nodep->m_widthMin;
        m_numeric = nodep->m_numeric;
    }
    //
    முழு width() மாறிலி VL_MT_STABLE { திரும்பு m_width; }
    வெறுமை numeric(VSigning flag) { m_numeric = flag; }
    bool isSigned() மாறிலி VL_MT_STABLE { திரும்பு m_numeric.isSigned(); }
    bool isNosign() மாறிலி VL_MT_SAFE { திரும்பு m_numeric.isNosign(); }
    VSigning numeric() மாறிலி VL_MT_STABLE { திரும்பு m_numeric; }
    முழு width_சொற்கள்() மாறிலி VL_MT_STABLE { திரும்பு VL_சொற்கள்_I(width()); }
    முழு widthMin() மாறிலி VL_MT_STABLE {  // If sized, the size,
                                         // if unsized the min digits to represent it
        திரும்பு m_widthMin ? m_widthMin : m_width;
    }
    முழு widthPow2() மாறிலி;
    வெறுமை widthMinFromWidth() { m_widthMin = m_width; }
    bool widthSized() மாறிலி VL_MT_SAFE { திரும்பு !m_widthMin || m_widthMin == m_width; }
    bool generic() மாறிலி VL_MT_SAFE { திரும்பு m_generic; }
    வெறுமை generic(bool flag) { m_generic = flag; }
    std::pair<uint32_t, uint32_t> dimensions(bool includeBasic) மாறிலி;
    uint32_t arrayUnpackedElements() மாறிலி;  // 1, or total multiplication of all dimensions
    // Fixed aggregate streaming properties
    bool isStreamableFixedAggregate() மாறிலி;
    // Whether is a string, or an unpacked array or structure containing one
    bool containsString() மாறிலி;
    bool containsUnpackedStruct() மாறிலி;
    முழு widthStream() மாறிலி;
    string vlEnumType() மாறிலி;  // Return VerilatedVarType: VLVT_UINT32, etc
    string vlEnumPackedAgg() மாறிலி;  // Return VLVF_PACKED_AGG etc flags, or empty
    நிலையான முழு unique_இலக்கம்_Inc() { திரும்பு ++s_unique_இலக்கம்; }
    மாறிலி எழுத்து* charIQWN() மாறிலி {
        திரும்பு (isString() ? "N" : isWide() ? "W" : isDouble() ? "D" : isQuad() ? "Q" : "I");
    }
    string cType(மாறிலி string& name, bool forFunc, bool isRef, bool packed = false) மாறிலி;
    // Represents a C++ LiteralType? (can be constexpr)
    bool isLiteralType() மாறிலி VL_MT_STABLE;
    // C++ type (as emitted by cType) is trivially default constructible and destructible?
    bool isTriviallyConstructible() மாறிலி VL_MT_STABLE;
    மெய்நிகர் bool isDynamicallySized() மாறிலி { திரும்பு false; }

தனிப்பட்ட:
    class CTypeRecursed;
    CTypeRecursed cTypeRecurse(bool compound, bool packed) மாறிலி;
};
class Ast_முனை_Array_DType VL_NOT_FINAL : பொது Ast_முனை_DType {
    // Array data type, ie "some_dtype var_name [2:0]"
    // @astgen op1 := childDTypep : Optional[AstNodeDType] // moved to refDTypep() in V3Width
    // @astgen op2 := rangep : Optional[AstRange] // array bounds
    //
    // @astgen ptr := m_refDTypep : Optional[AstNodeDType]  // Elements of this type (post-width)
    Ast_முனை* rangenp() மாறிலி { திரும்பு reinterpret_cast<Ast_முனை*>(rangep()); }

பாதுகாக்கப்பட்ட:
    Ast_முனை_Array_DType(VNType t, File_வரி* fl)
        : Ast_முனை_DType{t, fl} {}

பொது:
    ASTGEN_MEMBERS_Ast_முனை_Array_DType;
    வெறுமை dump(std::ostream& str) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str) மாறிலி override;
    வெறுமை dumpSmall(std::ostream& str) மாறிலி override;
    மாறிலி எழுத்து* broken() மாறிலி override {
        BROKEN_RTN(!((m_refDTypep && !childDTypep()) || (!m_refDTypep && childDTypep())));
        திரும்பு nullptr;
    }
    bool same_முனை(மாறிலி Ast_முனை* samep) மாறிலி override {
        மாறிலி Ast_முனை_Array_DType* மாறிலி asamep = VN_DBG_AS(samep, முனை_Array_DType);
        திரும்பு hi() == asamep->hi() && rangenp()->same_மரம்(asamep->rangenp())
               && subDTypep() == asamep->subDTypep();
    }  // HashedDT doesn't recurse, so need to check children
    bool similar_DType_முனை(மாறிலி Ast_முனை_DType* samep) மாறிலி override {
        மாறிலி Ast_முனை_Array_DType* மாறிலி asamep = VN_DBG_AS(samep, முனை_Array_DType);
        திரும்பு elementsConst() == asamep->elementsConst()
               && subDTypep()->similarDType(asamep->subDTypep());
    }
    Ast_முனை_DType* getChildDTypep() மாறிலி override { திரும்பு childDTypep(); }
    Ast_முனை_DType* subDTypep() மாறிலி override VL_MT_STABLE {
        திரும்பு m_refDTypep ? m_refDTypep : childDTypep();
    }
    வெறுமை refDTypep(Ast_முனை_DType* nodep) { m_refDTypep = nodep; }
    Ast_முனை_DType* virtRefDTypep() மாறிலி override { திரும்பு m_refDTypep; }
    வெறுமை virtRefDTypep(Ast_முனை_DType* nodep) override { refDTypep(nodep); }
    // METHODS
    AstBasicDType* basicp() மாறிலி override VL_MT_STABLE {
        திரும்பு subDTypep()->basicp();
    }  // (Slow) recurse down to find basic data type
    முழு widthAlignBytes() மாறிலி override { திரும்பு subDTypep()->widthAlignBytes(); }
    முழு widthTotalBytes() மாறிலி override {
        திரும்பு elementsConst() * subDTypep()->widthTotalBytes();
    }
    உள்வரி முழு இடது() மாறிலி VL_MT_STABLE;
    உள்வரி முழு வலது() மாறிலி VL_MT_STABLE;
    உள்வரி முழு hi() மாறிலி VL_MT_STABLE;
    உள்வரி முழு lo() மாறிலி VL_MT_STABLE;
    உள்வரி முழு elementsConst() மாறிலி VL_MT_STABLE;
    உள்வரி VNumRange declRange() மாறிலி VL_MT_STABLE;
};
class Ast_முனை_UOr_Struct_DType VL_NOT_FINAL : பொது Ast_முனை_DType {
    // A struct or union; common handling
    // @astgen op1 := membersp : List[AstMemberDType]
    //
    // @astgen ptr := m_classOrPackagep : Optional[AstNodeModule]  // Package emitted with
    string m_name;  // Name from upper typedef, if any
    string m_typedefName;  // Typedef's dtypeName() once moved off the typedef declaring it
    மாறிலி முழு m_unique_இலக்கம்;  // Unique ID distinguishing this dtype instance, for hashing/naming
    bool m_packed;  // Packed struct/union, else unpacked
    bool m_isFourstate = false;  // V3Width computes; true if any member is 4-state
    bool m_constrainedRand = false;  // True if struct has constraint expression
    bool m_emitToString = false;  // Generate to_string() for this struct/union if set

பாதுகாக்கப்பட்ட:
    Ast_முனை_UOr_Struct_DType(VNType t, File_வரி* fl, VSigning numericUnpack)
        : Ast_முனை_DType{t, fl}
        , m_unique_இலக்கம்{unique_இலக்கம்_Inc()} {
        // VSigning::NOSIGN overloaded to indicate not packed
        m_packed = (numericUnpack != VSigning::NOSIGN);
        numeric(VSigning::fromBool(numericUnpack.isSigned()));
    }
    Ast_முனை_UOr_Struct_DType(மாறிலி Ast_முனை_UOr_Struct_DType& other)
        : Ast_முனை_DType{other}
        , m_name{other.m_name}
        , m_typedefName{other.m_typedefName}
        , m_unique_இலக்கம்{unique_இலக்கம்_Inc()}
        , m_packed{other.m_packed}
        , m_isFourstate{other.m_isFourstate} {}

பொது:
    ASTGEN_MEMBERS_Ast_முனை_UOr_Struct_DType;
    முழு unique_இலக்கம்() மாறிலி { திரும்பு m_unique_இலக்கம்; }
    வெறுமை dump(std::ostream& str) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str) மாறிலி override;
    string prettyDTypeName(bool) மாறிலி override;
    bool isCompound() மாறிலி override { திரும்பு !packed(); }
    // For basicp() we reuse the size to indicate a "fake" basic type of same size
    AstBasicDType* basicp() மாறிலி override VL_MT_STABLE {
        எனில் (!m_packed) திரும்பு nullptr;
        திரும்பு (isFourstate()
                    ? VN_AS(findLogicRangeDType(VNumRange{width() - 1, 0}, width(), numeric()),
                            BasicDType)
                    : VN_AS(findBitRangeDType(VNumRange{width() - 1, 0}, width(), numeric()),
                            BasicDType));
    }
    // (Slow) recurses - Structure alignment 1,2,4 or 8 bytes (arrays affect this)
    முழு widthAlignBytes() மாறிலி override;
    // (Slow) recurses - Width in bytes rounding up 1,2,4,8,12,...
    முழு widthTotalBytes() மாறிலி override;
    bool similar_DType_முனை(மாறிலி Ast_முனை_DType* samep) மாறிலி override;
    string name() மாறிலி override VL_MT_STABLE { திரும்பு m_name; }
    வெறுமை name(மாறிலி string& flag) override { m_name = flag; }
    // Keep the name the typedef declaring it gives, as it moves off it to the type table
    வெறுமை typedefName(மாறிலி string& name) { m_typedefName = name; }
    // Name of the C structure for DPI, empty if no typedef names it, so declared where used
    string dpiName() மாறிலி;
    // C typedef of the unpacked structure for DPI (IEEE 1800-2023 H.7.3)
    string dpiTypedef() மாறிலி;
    bool packed() மாறிலி VL_MT_SAFE { திரும்பு m_packed; }
    வெறுமை packed(bool flag) { m_packed = flag; }
    // packed() but as don't support unpacked, presently all structs
    நிலையான bool packedUnsup() { திரும்பு true; }
    வெறுமை isFourstate(bool flag) { m_isFourstate = flag; }
    bool isFourstate() மாறிலி override VL_MT_SAFE { திரும்பு m_isFourstate; }
    நிலையான முழு lo() VL_MT_STABLE { திரும்பு 0; }
    முழு hi() மாறிலி VL_MT_STABLE {
        திரும்பு dtypep()->width() - 1;
    }  // Packed classes look like arrays
    VNumRange declRange() மாறிலி VL_MT_STABLE { திரும்பு VNumRange{hi(), lo()}; }
    Ast_முனை_Module* classOrPackagep() மாறிலி { திரும்பு m_classOrPackagep; }
    வெறுமை classOrPackagep(Ast_முனை_Module* classpackagep) { m_classOrPackagep = classpackagep; }
    bool isConstrainedRand() மாறிலி { திரும்பு m_constrainedRand; }
    வெறுமை markConstrainedRand(bool flag) { m_constrainedRand = flag; }
    bool emitToString() மாறிலி { திரும்பு m_emitToString; }
    வெறுமை setEmitToString() { m_emitToString = true; }
    bool isAggregateType() மாறிலி override { திரும்பு !packed(); }
};

// === Concrete node types =====================================================

// === AstNode ===
class AstEnumItem final : பொது Ast_முனை {
    // @astgen op1 := rangep : Optional[AstRange] // Range for name appending
    // @astgen op2 := valuep : Optional[AstNodeExpr]
    string m_name;  // Name of item

பொது:
    // Parents: ENUM
    AstEnumItem(File_வரி* fl, மாறிலி string& name, AstRange* rangep, Ast_முனை_கோவை* valuep)
        : ASTGEN_SUPER_EnumItem(fl)
        , m_name{name} {
        this->rangep(rangep);
        this->valuep(valuep);
    }
    ASTGEN_MEMBERS_AstEnumItem;
    string name() மாறிலி override VL_MT_STABLE { திரும்பு m_name; }
    bool maybePointedTo() மாறிலி override VL_MT_SAFE { திரும்பு true; }
    bool hasDType() மாறிலி override VL_MT_SAFE { திரும்பு true; }
    வெறுமை name(மாறிலி string& flag) override { m_name = flag; }
};

// === AstNodeDType ===
class AstAssocArrayDType final : பொது Ast_முனை_DType {
    // Associative array data type, ie "[some_dtype]"
    // @astgen op1 := childDTypep : Optional[AstNodeDType] // moved to refDTypep() in V3Width
    // @astgen op2 := keyChildDTypep : Optional[AstNodeDType]
    //
    // @astgen ptr := m_refDTypep : Optional[AstNodeDType]  // Elements of this type (post-width)
    // @astgen ptr := m_keyDTypep : Optional[AstNodeDType]  // Keys of this type (post-width)
பொது:
    AstAssocArrayDType(File_வரி* fl, VFlagChildDType, Ast_முனை_DType* dtp, Ast_முனை_DType* சாவி_Dtp)
        : ASTGEN_SUPER_AssocArrayDType(fl) {
        childDTypep(dtp);  // Only for parser
        சாவி_Child_DTypep(சாவி_Dtp);  // Only for parser
        refDTypep(nullptr);
        சாவி_DTypep(nullptr);
        dtypep(nullptr);  // V3Width will resolve
    }
    AstAssocArrayDType(File_வரி* fl, Ast_முனை_DType* dtp, Ast_முனை_DType* சாவி_Dtp)
        : ASTGEN_SUPER_AssocArrayDType(fl) {
        refDTypep(dtp);
        சாவி_DTypep(சாவி_Dtp);
        dtypep(dtp);
    }
    ASTGEN_MEMBERS_AstAssocArrayDType;
    மாறிலி எழுத்து* broken() மாறிலி override {
        BROKEN_RTN(!((m_refDTypep && !childDTypep()) || (!m_refDTypep && childDTypep())));
        BROKEN_RTN(!((m_சாவி_DTypep && !childDTypep()) || (!m_சாவி_DTypep && childDTypep())));
        திரும்பு nullptr;
    }
    bool same_முனை(மாறிலி Ast_முனை* samep) மாறிலி override {
        மாறிலி AstAssocArrayDType* மாறிலி asamep = VN_DBG_AS(samep, AssocArrayDType);
        எனில் (!asamep->subDTypep()) திரும்பு false;
        எனில் (!asamep->சாவி_DTypep()) திரும்பு false;
        திரும்பு (subDTypep() == asamep->subDTypep() && சாவி_DTypep() == asamep->சாவி_DTypep());
    }
    bool similar_DType_முனை(மாறிலி Ast_முனை_DType* samep) மாறிலி override {
        மாறிலி AstAssocArrayDType* மாறிலி asamep = VN_DBG_AS(samep, AssocArrayDType);
        திரும்பு asamep->subDTypep() && subDTypep()->similarDType(asamep->subDTypep())
               && asamep->சாவி_DTypep() && சாவி_DTypep()->similarDType(asamep->சாவி_DTypep());
    }
    string prettyDTypeName(bool full) மாறிலி override;
    வெறுமை dumpSmall(std::ostream& str) மாறிலி override;
    Ast_முனை_DType* getChildDTypep() மாறிலி override { திரும்பு childDTypep(); }
    Ast_முனை_DType* getChild2DTypep() மாறிலி override { திரும்பு சாவி_Child_DTypep(); }
    Ast_முனை_DType* subDTypep() மாறிலி override VL_MT_STABLE {
        திரும்பு m_refDTypep ? m_refDTypep : childDTypep();
    }
    Ast_முனை_DType* subDType2p() மாறிலி override VL_MT_STABLE {
        திரும்பு m_சாவி_DTypep ? m_சாவி_DTypep : சாவி_Child_DTypep();
    }
    bool isAggregateType() மாறிலி override { திரும்பு true; }
    வெறுமை refDTypep(Ast_முனை_DType* nodep) { m_refDTypep = nodep; }
    Ast_முனை_DType* virtRefDTypep() மாறிலி override { திரும்பு m_refDTypep; }
    வெறுமை virtRefDTypep(Ast_முனை_DType* nodep) override { refDTypep(nodep); }
    Ast_முனை_DType* virtRefDType2p() மாறிலி override { திரும்பு m_சாவி_DTypep; }
    வெறுமை virtRefDType2p(Ast_முனை_DType* nodep) override { சாவி_DTypep(nodep); }
    //
    Ast_முனை_DType* சாவி_DTypep() மாறிலி VL_MT_STABLE {
        திரும்பு m_சாவி_DTypep ? m_சாவி_DTypep : சாவி_Child_DTypep();
    }
    வெறுமை சாவி_DTypep(Ast_முனை_DType* nodep) { m_சாவி_DTypep = nodep; }
    // METHODS
    AstBasicDType* basicp() மாறிலி override VL_MT_STABLE { திரும்பு nullptr; }
    முழு widthAlignBytes() மாறிலி override { திரும்பு subDTypep()->widthAlignBytes(); }
    முழு widthTotalBytes() மாறிலி override { திரும்பு subDTypep()->widthTotalBytes(); }
    bool isCompound() மாறிலி override { திரும்பு true; }
    bool isDynamicallySized() மாறிலி override { திரும்பு true; }
};
class AstBasicDType final : பொது Ast_முனை_DType {
    // Builtin atomic/vectored data type
    // @astgen op1 := rangep : Optional[AstRange] // Range of variable
    கட்டமைப்பு Members final {
        VBasicDTypeKwd m_keyword;  // (also in VBasicTypeKey) What keyword created basic type
        // dist-ast-dump-suppress  // Part of dumpSmall
        VNumRange m_nrange;  // (also in VBasicTypeKey) Numeric msb/lsb (if non-opaque keyword)
        bool operator==(மாறிலி Members& rhs) மாறிலி {
            திரும்பு rhs.m_keyword == m_keyword && rhs.m_nrange == m_nrange;
        }
    } m;
    // See also in AstNodeDType: m_width, m_widthMin, m_numeric(issigned)
பொது:
    AstBasicDType(File_வரி* fl, VBasicDTypeKwd kwd, மாறிலி VSigning& signst = VSigning::NOSIGN)
        : ASTGEN_SUPER_BasicDType(fl) {
        init(kwd, signst, 0, -1, nullptr);
    }
    AstBasicDType(File_வரி* fl, VFlagLogicPacked, முழு wantwidth)
        : ASTGEN_SUPER_BasicDType(fl) {
        init(VBasicDTypeKwd::LOGIC, VSigning::NOSIGN, wantwidth, -1, nullptr);
    }
    AstBasicDType(File_வரி* fl, VFlagLogic2StatePacked, முழு wantwidth)
        : ASTGEN_SUPER_BasicDType(fl) {
        init(VBasicDTypeKwd::LOGIC2STATE, VSigning::NOSIGN, wantwidth, -1, nullptr);
    }
    AstBasicDType(File_வரி* fl, VFlagBitPacked, முழு wantwidth)
        : ASTGEN_SUPER_BasicDType(fl) {
        init(VBasicDTypeKwd::BIT, VSigning::NOSIGN, wantwidth, -1, nullptr);
    }
    AstBasicDType(File_வரி* fl, VBasicDTypeKwd kwd, VSigning numer, முழு wantwidth, முழு widthmin)
        : ASTGEN_SUPER_BasicDType(fl) {
        init(kwd, numer, wantwidth, widthmin, nullptr);
    }
    AstBasicDType(File_வரி* fl, VBasicDTypeKwd kwd, VSigning numer, VNumRange range, முழு widthmin)
        : ASTGEN_SUPER_BasicDType(fl) {
        init(kwd, numer, range.elements(), widthmin, nullptr);
        m.m_nrange = range;  // as init() presumes lsb==0, but range.lsb() might not be
    }
    // See also addRange in verilog.y
தனிப்பட்ட:
    வெறுமை init(VBasicDTypeKwd kwd, VSigning numer, முழு wantwidth, முழு wantwidthmin,
              AstRange* rangep);

பொது:
    ASTGEN_MEMBERS_AstBasicDType;
    வெறுமை dump(std::ostream& str) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str) மாறிலி override;
    // width/widthMin/numeric compared elsewhere
    bool same_முனை(மாறிலி Ast_முனை* samep) மாறிலி override;
    bool similar_DType_முனை(மாறிலி Ast_முனை_DType* samep) மாறிலி override;
    string name() மாறிலி override VL_MT_STABLE { திரும்பு m.m_keyword.typeName(); }
    string prettyDTypeName(bool full) மாறிலி override;
    மாறிலி எழுத்து* broken() மாறிலி override {
        BROKEN_RTN(dtypep() != this);
        BROKEN_RTN(v3Global.widthMinUsage() == VWidthMinUsage::VERILOG_WIDTH
                   && widthMin() > width());
        திரும்பு nullptr;
    }
    வெறுமை setSignedState(மாறிலி VSigning& signst) {
        // Note NOSIGN does NOT change the state; this is required by the parser
        எனில் (signst == VSigning::UNSIGNED) {
            numeric(signst);
        } இல்லையெனில் எனில் (signst == VSigning::SIGNED) {
            numeric(signst);
        }
    }
    // METHODS
    AstBasicDType* basicp() மாறிலி override VL_MT_STABLE { திரும்பு (AstBasicDType*)this; }
    // (Slow) recurses - Structure alignment 1,2,4 or 8 bytes (arrays affect this)
    முழு widthAlignBytes() மாறிலி override;
    // (Slow) recurses - Width in bytes rounding up 1,2,4,8,12,...
    முழு widthTotalBytes() மாறிலி override;
    bool isFourstate() மாறிலி override { திரும்பு keyword().isFourstate(); }
    VBasicDTypeKwd keyword() மாறிலி VL_MT_SAFE {  // Avoid using - use isSomething accessors instead
        திரும்பு m.m_keyword;
    }
    bool isBitLogic() மாறிலி { திரும்பு keyword().isBitLogic(); }
    bool isBit() மாறிலி { திரும்பு keyword().isBit(); }
    bool isCHandle() மாறிலி VL_MT_STABLE { திரும்பு keyword().isCHandle(); }
    bool isDouble() மாறிலி VL_MT_STABLE { திரும்பு keyword().isDouble(); }
    bool isEvent() மாறிலி VL_MT_STABLE { திரும்பு keyword() == VBasicDTypeKwd::EVENT; }
    bool isForkSync() மாறிலி VL_MT_SAFE { திரும்பு keyword() == VBasicDTypeKwd::FORK_SYNC; }
    bool isProcessRef() மாறிலி VL_MT_SAFE { திரும்பு keyword() == VBasicDTypeKwd::PROCESS_REFERENCE; }
    bool isDelayScheduler() மாறிலி VL_MT_SAFE {
        திரும்பு keyword() == VBasicDTypeKwd::DELAY_SCHEDULER;
    }
    bool isTriggerScheduler() மாறிலி VL_MT_SAFE {
        திரும்பு keyword() == VBasicDTypeKwd::TRIGGER_SCHEDULER;
    }
    bool isDynamicTriggerScheduler() மாறிலி VL_MT_SAFE {
        திரும்பு keyword() == VBasicDTypeKwd::DYNAMIC_TRIGGER_SCHEDULER;
    }
    bool isRandomGenerator() மாறிலி VL_MT_SAFE {
        திரும்பு keyword() == VBasicDTypeKwd::RANDOM_GENERATOR;
    }
    bool isStdRandomGenerator() மாறிலி VL_MT_SAFE {
        திரும்பு keyword() == VBasicDTypeKwd::RANDOM_STDGENERATOR;
    }
    bool isCovergroupInstHandle() மாறிலி VL_MT_SAFE {
        திரும்பு keyword() == VBasicDTypeKwd::COVERGROUP_INSTHANDLE;
    }
    bool isOpaque() மாறிலி VL_MT_SAFE { திரும்பு keyword().isOpaque(); }
    bool isString() மாறிலி VL_MT_STABLE { திரும்பு keyword().isString(); }
    bool isZeroInit() மாறிலி { திரும்பு keyword().isZeroInit(); }
    bool isRanged() மாறிலி { திரும்பு rangep() || m.m_nrange.ranged(); }
    bool isDpiBitVec() மாறிலி {  // DPI uses svBitVecVal
        திரும்பு keyword() == VBasicDTypeKwd::BIT && isRanged();
    }
    bool isDpiLogicVec() மாறிலி {  // DPI uses svLogicVecVal
        திரும்பு keyword().isFourstate() && !(keyword() == VBasicDTypeKwd::LOGIC && !isRanged());
    }
    bool isDpiCLayout() மாறிலி {
        திரும்பு keyword().isDpiCLayout()
               || (keyword().isSameish(VBasicDTypeKwd::LOGIC) && !isRanged());
    }
    bool isDpiPrimitive() மாறிலி {  // DPI uses a primitive type
        திரும்பு !isDpiBitVec() && !isDpiLogicVec();
    }
    // Generally the lo/hi/left/right funcs should be used instead of nrange()
    மாறிலி VNumRange& nrange() மாறிலி { திரும்பு m.m_nrange; }
    உள்வரி முழு hi() மாறிலி;
    உள்வரி முழு lo() மாறிலி;
    உள்வரி முழு elements() மாறிலி;
    முழு இடது() மாறிலி { திரும்பு ascending() ? lo() : hi(); }  // How to show a declaration
    முழு வலது() மாறிலி { திரும்பு ascending() ? hi() : lo(); }
    உள்வரி bool ascending() மாறிலி;
    bool implicit() மாறிலி { திரும்பு keyword() == VBasicDTypeKwd::LOGIC_IMPLICIT; }
    bool untyped() மாறிலி { திரும்பு keyword() == VBasicDTypeKwd::UNTYPED; }
    VNumRange declRange() மாறிலி { திரும்பு isRanged() ? VNumRange{இடது(), வலது()} : VNumRange{}; }
    வெறுமை cvtRangeConst();  // Convert to smaller representation
    bool isCompound() மாறிலி override { திரும்பு isString(); }
    bool isIntegralOrPacked() மாறிலி override { திரும்பு keyword().isIntNumeric(); }
};
class AstBracketArrayDType final : பொது Ast_முனை_DType {
    // Associative/Queue/Normal array data type, ie "[dtype_or_expr]"
    // only for early parsing then becomes another data type
    // @astgen op1 := childDTypep : Optional[AstNodeDType] // moved to refDTypep() in V3Width
    // @astgen op2 := elementsp : AstNode  // Number of elements in array
பொது:
    AstBracketArrayDType(File_வரி* fl, VFlagChildDType, Ast_முனை_DType* childDTypep,
                         Ast_முனை* elementsp)
        : ASTGEN_SUPER_BracketArrayDType(fl) {
        this->childDTypep(childDTypep);
        this->elementsp(elementsp);
    }
    ASTGEN_MEMBERS_AstBracketArrayDType;
    bool similar_DType_முனை(மாறிலி Ast_முனை_DType* samep) மாறிலி override { திரும்பு this == samep; }
    Ast_முனை_DType* subDTypep() மாறிலி override VL_MT_STABLE { திரும்பு childDTypep(); }
    // METHODS
    // Will be removed in V3Width, which relies on this
    // being a child not a dtype pointed node
    bool maybePointedTo() மாறிலி override VL_MT_SAFE { திரும்பு false; }
    AstBasicDType* basicp() மாறிலி override VL_MT_STABLE { திரும்பு nullptr; }
    முழு widthAlignBytes() மாறிலி override { V3ERROR_NA_RETURN(0); }
    முழு widthTotalBytes() மாறிலி override { V3ERROR_NA_RETURN(0); }
    bool isCompound() மாறிலி override { திரும்பு true; }
};
class AstCDType final : பொது Ast_முனை_DType {
    // Raw "C" data type passed directly to output
    string m_name;  // Name of data type, printed when do V3EmitC
பொது:
    AstCDType(File_வரி* fl, மாறிலி string& name)
        : ASTGEN_SUPER_CDType(fl)
        , m_name{name} {
        dtypep(this);
    }

    ASTGEN_MEMBERS_AstCDType;
    bool same_முனை(மாறிலி Ast_முனை* samep) மாறிலி override {
        மாறிலி AstCDType* மாறிலி asamep = VN_DBG_AS(samep, CDType);
        திரும்பு m_name == asamep->m_name;
    }
    bool similar_DType_முனை(மாறிலி Ast_முனை_DType* samep) மாறிலி override { திரும்பு same_முனை(samep); }
    string name() மாறிலி override VL_MT_STABLE { திரும்பு m_name; }
    string prettyDTypeName(bool) மாறிலி override { திரும்பு m_name; }
    // METHODS
    AstBasicDType* basicp() மாறிலி override VL_MT_STABLE { திரும்பு nullptr; }
    முழு widthAlignBytes() மாறிலி override { திரும்பு 8; }  // Assume
    முழு widthTotalBytes() மாறிலி override { திரும்பு 8; }  // Assume
    bool isCompound() மாறிலி override { திரும்பு true; }
    // Return the global runtime type storing the requested number of bits.
    நிலையான string typeToHold(முழு width) {
        எனில் (width <= 8) திரும்பு "::CData";
        எனில் (width <= 16) திரும்பு "::SData";
        எனில் (width <= VL_IDATASIZE) திரும்பு "::IData";
        எனில் (width <= VL_QUADSIZE) திரும்பு "::QData";

        திரும்பு "::VlWide<" + std::to_string(VL_சொற்கள்_I(width)) + ">";
    }
};
class AstClassRefDType final : பொது Ast_முனை_DType {
    // Reference to a class
    // @astgen op1 := paramsp: List[AstPin]
    //
    // @astgen ptr := m_classp : Optional[AstClass]  // data type pointed to, BELOW the AstTypedef
    // @astgen ptr := m_classOrPackagep : Optional[AstNodeModule]  // Package hierarchy
    bool m_rawPointer = false;  // Emit as a non-owning C++ pointer rather than VlClassRef
பொது:
    AstClassRefDType(File_வரி* fl, AstClass* classp, AstPin* paramsp)
        : ASTGEN_SUPER_ClassRefDType(fl)
        , m_classp{classp} {
        dtypep(this);
        addParamsp(paramsp);
    }
    ASTGEN_MEMBERS_AstClassRefDType;
    // METHODS
    bool same_முனை(மாறிலி Ast_முனை* samep) மாறிலி override {
        மாறிலி AstClassRefDType* மாறிலி asamep = VN_DBG_AS(samep, ClassRefDType);
        திரும்பு (m_classp == asamep->m_classp && m_classOrPackagep == asamep->m_classOrPackagep
                && m_rawPointer == asamep->m_rawPointer);
    }
    bool similar_DType_முனை(மாறிலி Ast_முனை_DType* samep) மாறிலி override;
    வெறுமை dump(std::ostream& str = std::cout) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str = std::cout) மாறிலி override;
    வெறுமை dumpSmall(std::ostream& str) மாறிலி override;
    string prettyDTypeName(bool full) மாறிலி override;
    string pretty_Name_செய்தி() மாறிலி override;
    string name() மாறிலி override VL_MT_STABLE;
    AstBasicDType* basicp() மாறிலி override VL_MT_STABLE { திரும்பு nullptr; }
    முழு widthAlignBytes() மாறிலி override { திரும்பு 0; }
    முழு widthTotalBytes() மாறிலி override { திரும்பு 0; }
    Ast_முனை_DType* virtRefDTypep() மாறிலி override { திரும்பு nullptr; }
    வெறுமை virtRefDTypep(Ast_முனை_DType* nodep) override {}
    Ast_முனை_DType* subDTypep() மாறிலி override VL_MT_STABLE { திரும்பு nullptr; }
    Ast_முனை_Module* classOrPackagep() மாறிலி { திரும்பு m_classOrPackagep; }
    வெறுமை classOrPackagep(Ast_முனை_Module* nodep) { m_classOrPackagep = nodep; }
    AstClass* classp() மாறிலி VL_MT_STABLE { திரும்பு m_classp; }
    வெறுமை classp(AstClass* nodep) { m_classp = nodep; }
    bool rawPointer() மாறிலி { திரும்பு m_rawPointer; }
    வெறுமை rawPointer(bool flag) { m_rawPointer = flag; }
    நிலையான வெறுமை selfTest();
    bool isCompound() மாறிலி override { திரும்பு true; }
};
class AstConstDType final : பொது Ast_முனை_DType {
    // const data type, ie "const some_dtype var_name [2:0]"
    // ConstDType are removed in V3LinkLValue and become AstVar::isConst.
    // When more generic types are supported AstConstDType will be propagated further.
    // @astgen op1 := childDTypep : Optional[AstNodeDType]
    //
    // @astgen ptr := m_refDTypep : Optional[AstNodeDType]  // Inherit from this base data type
பொது:
    AstConstDType(File_வரி* fl, VFlagChildDType, Ast_முனை_DType* dtp)
        : ASTGEN_SUPER_ConstDType(fl) {
        childDTypep(dtp);  // Only for parser
        refDTypep(nullptr);  // V3Width will resolve
        dtypep(nullptr);  // V3Width will resolve
        widthFromSub(subDTypep());
    }
    ASTGEN_MEMBERS_AstConstDType;
    மாறிலி எழுத்து* broken() மாறிலி override {
        BROKEN_RTN(!((m_refDTypep && !childDTypep()) || (!m_refDTypep && childDTypep())));
        திரும்பு nullptr;
    }
    bool same_முனை(மாறிலி Ast_முனை* samep) மாறிலி override {
        மாறிலி AstConstDType* மாறிலி sp = VN_DBG_AS(samep, ConstDType);
        திரும்பு (m_refDTypep == sp->m_refDTypep);
    }
    bool similar_DType_முனை(மாறிலி Ast_முனை_DType* samep) மாறிலி override {
        திரும்பு skipRefp()->similarDType(samep->skipRefp());
    }
    Ast_முனை_DType* getChildDTypep() மாறிலி override { திரும்பு childDTypep(); }
    Ast_முனை_DType* subDTypep() மாறிலி override VL_MT_STABLE {
        திரும்பு m_refDTypep ? m_refDTypep : childDTypep();
    }
    வெறுமை refDTypep(Ast_முனை_DType* nodep) { m_refDTypep = nodep; }
    Ast_முனை_DType* virtRefDTypep() மாறிலி override { திரும்பு m_refDTypep; }
    வெறுமை virtRefDTypep(Ast_முனை_DType* nodep) override { refDTypep(nodep); }
    // METHODS
    AstBasicDType* basicp() மாறிலி override VL_MT_STABLE { திரும்பு subDTypep()->basicp(); }
    முழு widthAlignBytes() மாறிலி override { திரும்பு subDTypep()->widthAlignBytes(); }
    முழு widthTotalBytes() மாறிலி override { திரும்பு subDTypep()->widthTotalBytes(); }
    bool isCompound() மாறிலி override {
        v3fatalSrc("call isCompound on subdata type, not reference");
        திரும்பு false;
    }
};
class AstConstraintRefDType final : பொது Ast_முனை_DType {
    // For e.g. a reference to constraint for constraint_mode
பொது:
    explicit AstConstraintRefDType(File_வரி* fl)
        : ASTGEN_SUPER_ConstraintRefDType(fl) {
        dtypep(this);
    }
    ASTGEN_MEMBERS_AstConstraintRefDType;
    bool hasDType() மாறிலி override VL_MT_SAFE { திரும்பு true; }
    bool maybePointedTo() மாறிலி override VL_MT_SAFE { திரும்பு true; }
    bool undead() மாறிலி override { திரும்பு true; }
    Ast_முனை_DType* subDTypep() மாறிலி override VL_MT_STABLE { திரும்பு nullptr; }
    Ast_முனை_DType* virtRefDTypep() மாறிலி override { திரும்பு nullptr; }
    வெறுமை virtRefDTypep(Ast_முனை_DType* nodep) override {}
    bool similar_DType_முனை(மாறிலி Ast_முனை_DType* samep) மாறிலி override { திரும்பு this == samep; }
    AstBasicDType* basicp() மாறிலி override VL_MT_STABLE { திரும்பு nullptr; }
    முழு widthAlignBytes() மாறிலி override { திரும்பு 1; }
    முழு widthTotalBytes() மாறிலி override { திரும்பு 1; }
    bool isCompound() மாறிலி override { திரும்பு false; }
};
class AstCoverCrossDType final : பொது Ast_முனை_DType {
    // Borrowed pointer to VlCoverCrossT<...> or construction-time VlCoverCrossDyn.
    மாறிலி uint32_t m_dimensions;
    மாறிலி uint32_t m_tuples;  // Fixed capacity number of cross tuples (VlCoverCrossT's Tuples)
    மாறிலி uint32_t m_bins;  // Fixed capacity number of explicit bins (VlCoverCrossT's Bins)
    மாறிலி uint32_t m_autoBins;  // Fixed capacity number of auto bins (VlCoverCrossT's AutoBins)
    மாறிலி uint64_t m_bin_சொற்கள்;  // Fixed capacity selection words (VlCoverCrossT's BinWords)
    மாறிலி bool m_dynamic;  // Bin layout is determined at covergroup construction

பொது:
    AstCoverCrossDType(File_வரி* fl, uint32_t dimensions, uint32_t tuples, uint32_t bins,
                       uint32_t autoBins, uint64_t bin_சொற்கள், bool dynamic = false)
        : ASTGEN_SUPER_CoverCrossDType(fl)
        , m_dimensions{dimensions}
        , m_tuples{tuples}
        , m_bins{bins}
        , m_autoBins{autoBins}
        , m_bin_சொற்கள்{bin_சொற்கள்}
        , m_dynamic{dynamic} {
        dtypep(this);
    }
    ASTGEN_MEMBERS_AstCoverCrossDType;
    மாறிலி எழுத்து* broken() மாறிலி override {
        BROKEN_RTN(m_dimensions == 0);
        திரும்பு nullptr;
    }
    bool same_முனை(மாறிலி Ast_முனை* samep) மாறிலி override {  // LCOV_EXCL_START
        மாறிலி AstCoverCrossDType* மாறிலி sp = VN_DBG_AS(samep, CoverCrossDType);
        திரும்பு dimensions() == sp->dimensions() && tuples() == sp->tuples() && bins() == sp->bins()
               && autoBins() == sp->autoBins() && bin_சொற்கள்() == sp->bin_சொற்கள்()
               && isDynamic() == sp->isDynamic();
    }  // LCOV_EXCL_STOP
    bool similar_DType_முனை(மாறிலி Ast_முனை_DType* samep) மாறிலி override { திரும்பு this == samep; }
    வெறுமை dump(std::ostream& str) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str) மாறிலி override;
    வெறுமை dumpSmall(std::ostream& str) மாறிலி override;
    uint32_t dimensions() மாறிலி { திரும்பு m_dimensions; }
    uint32_t tuples() மாறிலி { திரும்பு m_tuples; }
    uint32_t bins() மாறிலி { திரும்பு m_bins; }
    uint32_t autoBins() மாறிலி { திரும்பு m_autoBins; }
    uint64_t bin_சொற்கள்() மாறிலி { திரும்பு m_bin_சொற்கள்; }
    bool isDynamic() மாறிலி { திரும்பு m_dynamic; }
    string cppTemplateArgs() மாறிலி;
    AstBasicDType* basicp() மாறிலி override VL_MT_STABLE { திரும்பு nullptr; }
    முழு widthAlignBytes() மாறிலி override { திரும்பு அளவு(வெறுமை*); }
    முழு widthTotalBytes() மாறிலி override { திரும்பு அளவு(வெறுமை*); }
    bool isCompound() மாறிலி override { திரும்பு true; }
};
class AstCoverpointDType final : பொது Ast_முனை_DType {
    // Borrowed pointer to a covergroup coverpoint runtime, 'VlCoverpointT<hitBound>*'.
    // Follows pattern of AstQueueDType in capturing the compile-time max bin overlap
    // template argument.
    uint32_t m_hitBound;  // VlCoverpointT<> template argument; hit list size, >= 1
பொது:
    AstCoverpointDType(File_வரி* fl, uint32_t hitBound)
        : ASTGEN_SUPER_CoverpointDType(fl)
        , m_hitBound{hitBound} {
        dtypep(this);
    }
    ASTGEN_MEMBERS_AstCoverpointDType;
    மாறிலி எழுத்து* broken() மாறிலி override {
        BROKEN_RTN(m_hitBound < 1);
        திரும்பு nullptr;
    }
    // V3Covergroup interns these one-per-hitBound into the type table, so identity is
    // equality; there is never a second node with the same bound to compare against.
    bool similar_DType_முனை(மாறிலி Ast_முனை_DType* samep) மாறிலி override { திரும்பு this == samep; }
    வெறுமை dumpSmall(std::ostream& str) மாறிலி override;
    // ACCESSORS
    uint32_t hitBound() மாறிலி { திரும்பு m_hitBound; }
    // METHODS
    AstBasicDType* basicp() மாறிலி override VL_MT_STABLE { திரும்பு nullptr; }
    முழு widthAlignBytes() மாறிலி override { திரும்பு அளவு(வெறுமை*); }
    முழு widthTotalBytes() மாறிலி override { திரும்பு அளவு(வெறுமை*); }
    bool isCompound() மாறிலி override { திரும்பு true; }
};
class AstDefImplicitDType final : பொது Ast_முனை_DType {
    // For parsing enum/struct/unions that are declared with a variable rather than typedef
    // This allows "var enum {...} a,b" to share the enum definition for both variables
    // After link, these become typedefs
    // @astgen op1 := childDTypep : Optional[AstNodeDType]
    string m_name;  // Data type name
    மாறிலி முழு m_unique_இலக்கம்;  // Unique ID distinguishing this dtype instance, for hashing/naming

பொது:
    AstDefImplicitDType(File_வரி* fl, மாறிலி string& name, VFlagChildDType, Ast_முனை_DType* dtp)
        : ASTGEN_SUPER_DefImplicitDType(fl)
        , m_name{name}
        , m_unique_இலக்கம்{unique_இலக்கம்_Inc()} {
        childDTypep(dtp);  // Only for parser
        dtypep(nullptr);  // V3Width will resolve
    }
    AstDefImplicitDType(மாறிலி AstDefImplicitDType& other)
        : Ast_முனை_DType(other)
        , m_name{other.m_name}
        , m_unique_இலக்கம்{unique_இலக்கம்_Inc()} {}
    ASTGEN_MEMBERS_AstDefImplicitDType;
    முழு unique_இலக்கம்() மாறிலி { திரும்பு m_unique_இலக்கம்; }
    bool same_முனை(மாறிலி Ast_முனை* samep) மாறிலி override {
        மாறிலி AstDefImplicitDType* மாறிலி sp = VN_DBG_AS(samep, DefImplicitDType);
        திரும்பு unique_இலக்கம்() == sp->unique_இலக்கம்();
    }
    bool similar_DType_முனை(மாறிலி Ast_முனை_DType* samep) மாறிலி override { திரும்பு same_முனை(samep); }
    Ast_முனை_DType* getChildDTypep() மாறிலி override { திரும்பு childDTypep(); }
    Ast_முனை_DType* subDTypep() மாறிலி override VL_MT_STABLE {
        திரும்பு dtypep() ? dtypep() : childDTypep();
    }
    // METHODS
    // op1 = Range of variable
    Ast_முனை_DType* dtypeSkipRefp() மாறிலி { திரும்பு dtypep()->skipRefp(); }
    AstBasicDType* basicp() மாறிலி override VL_MT_STABLE { திரும்பு subDTypep()->basicp(); }
    முழு widthAlignBytes() மாறிலி override { திரும்பு dtypep()->widthAlignBytes(); }
    முழு widthTotalBytes() மாறிலி override { திரும்பு dtypep()->widthTotalBytes(); }
    string name() மாறிலி override VL_MT_STABLE { திரும்பு m_name; }
    வெறுமை name(மாறிலி string& flag) override { m_name = flag; }
    bool isCompound() மாறிலி override { திரும்பு false; }
};
class AstDynArrayDType final : பொது Ast_முனை_DType {
    // Dynamic array data type, ie "[]"
    // @astgen op1 := childDTypep : Optional[AstNodeDType] // moved to refDTypep() in V3Width
    //
    // @astgen ptr := m_refDTypep : Optional[AstNodeDType]  // Elements of this type (post-width)
பொது:
    AstDynArrayDType(File_வரி* fl, VFlagChildDType, Ast_முனை_DType* dtp)
        : ASTGEN_SUPER_DynArrayDType(fl) {
        childDTypep(dtp);  // Only for parser
        refDTypep(nullptr);
        dtypep(nullptr);  // V3Width will resolve
    }
    AstDynArrayDType(File_வரி* fl, Ast_முனை_DType* dtp)
        : ASTGEN_SUPER_DynArrayDType(fl) {
        refDTypep(dtp);
        dtypep(nullptr);  // V3Width will resolve
    }
    ASTGEN_MEMBERS_AstDynArrayDType;
    மாறிலி எழுத்து* broken() மாறிலி override {
        BROKEN_RTN(!((m_refDTypep && !childDTypep()) || (!m_refDTypep && childDTypep())));
        திரும்பு nullptr;
    }
    bool same_முனை(மாறிலி Ast_முனை* samep) மாறிலி override {
        மாறிலி AstDynArrayDType* மாறிலி asamep = VN_DBG_AS(samep, DynArrayDType);
        எனில் (!asamep->subDTypep()) திரும்பு false;
        திரும்பு subDTypep() == asamep->subDTypep();
    }
    bool similar_DType_முனை(மாறிலி Ast_முனை_DType* samep) மாறிலி override {
        மாறிலி AstDynArrayDType* மாறிலி asamep = VN_DBG_AS(samep, DynArrayDType);
        திரும்பு asamep->subDTypep() && subDTypep()->similarDType(asamep->subDTypep());
    }
    string prettyDTypeName(bool full) மாறிலி override;
    வெறுமை dumpSmall(std::ostream& str) மாறிலி override;
    Ast_முனை_DType* getChildDTypep() மாறிலி override { திரும்பு childDTypep(); }
    Ast_முனை_DType* subDTypep() மாறிலி override VL_MT_STABLE {
        திரும்பு m_refDTypep ? m_refDTypep : childDTypep();
    }
    bool isAggregateType() மாறிலி override { திரும்பு true; }
    வெறுமை refDTypep(Ast_முனை_DType* nodep) { m_refDTypep = nodep; }
    Ast_முனை_DType* virtRefDTypep() மாறிலி override { திரும்பு m_refDTypep; }
    வெறுமை virtRefDTypep(Ast_முனை_DType* nodep) override { refDTypep(nodep); }
    // METHODS
    AstBasicDType* basicp() மாறிலி override VL_MT_STABLE { திரும்பு nullptr; }
    முழு widthAlignBytes() மாறிலி override { திரும்பு subDTypep()->widthAlignBytes(); }
    முழு widthTotalBytes() மாறிலி override { திரும்பு subDTypep()->widthTotalBytes(); }
    bool isCompound() மாறிலி override { திரும்பு true; }
    bool isDynamicallySized() மாறிலி override { திரும்பு true; }
};
class AstEmptyQueueDType final : பொது Ast_முனை_DType {
    // For EmptyQueue
பொது:
    explicit AstEmptyQueueDType(File_வரி* fl)
        : ASTGEN_SUPER_EmptyQueueDType(fl) {
        dtypep(this);
    }
    ASTGEN_MEMBERS_AstEmptyQueueDType;
    வெறுமை dumpSmall(std::ostream& str) மாறிலி override;
    bool hasDType() மாறிலி override VL_MT_SAFE { திரும்பு true; }
    bool maybePointedTo() மாறிலி override VL_MT_SAFE { திரும்பு true; }
    bool undead() மாறிலி override { திரும்பு true; }
    Ast_முனை_DType* subDTypep() மாறிலி override VL_MT_STABLE { திரும்பு nullptr; }
    Ast_முனை_DType* virtRefDTypep() மாறிலி override { திரும்பு nullptr; }
    வெறுமை virtRefDTypep(Ast_முனை_DType* nodep) override {}
    bool similar_DType_முனை(மாறிலி Ast_முனை_DType* samep) மாறிலி override { திரும்பு this == samep; }
    AstBasicDType* basicp() மாறிலி override VL_MT_STABLE { திரும்பு nullptr; }
    முழு widthAlignBytes() மாறிலி override { திரும்பு 1; }
    முழு widthTotalBytes() மாறிலி override { திரும்பு 1; }
    bool isCompound() மாறிலி override { திரும்பு false; }
};
class AstEnumDType final : பொது Ast_முனை_DType {
    // Parents: TYPEDEF/MODULE
    // @astgen op1 := childDTypep : Optional[AstNodeDType]
    // @astgen op2 := itemsp : List[AstEnumItem]
    //
    // @astgen ptr := m_refDTypep : Optional[AstNodeDType]  // Elements of this type (post-width)
    string m_name;  // Name from upper typedef, if any
    string m_typedefName;  // Typedef's dtypeName() once moved off the typedef declaring it
    மாறிலி முழு m_unique_இலக்கம்;  // Unique ID distinguishing this dtype instance, for hashing/naming

பொது:
    AstEnumDType(File_வரி* fl, VFlagChildDType, Ast_முனை_DType* dtp, AstEnumItem* itemsp)
        : ASTGEN_SUPER_EnumDType(fl)
        , m_unique_இலக்கம்{unique_இலக்கம்_Inc()} {
        childDTypep(dtp);  // Only for parser
        refDTypep(nullptr);
        addItemsp(itemsp);
        dtypep(nullptr);  // V3Width will resolve
        widthFromSub(subDTypep());
    }
    AstEnumDType(மாறிலி AstEnumDType& other)
        : Ast_முனை_DType{other}
        , m_name{other.m_name}
        , m_typedefName{other.m_typedefName}
        , m_unique_இலக்கம்{unique_இலக்கம்_Inc()} {}
    ASTGEN_MEMBERS_AstEnumDType;

    மாறிலி எழுத்து* broken() மாறிலி override;
    முழு unique_இலக்கம்() மாறிலி { திரும்பு m_unique_இலக்கம்; }
    bool same_முனை(மாறிலி Ast_முனை* samep) மாறிலி override {
        மாறிலி AstEnumDType* மாறிலி sp = VN_DBG_AS(samep, EnumDType);
        திரும்பு unique_இலக்கம்() == sp->unique_இலக்கம்();
    }
    bool similar_DType_முனை(மாறிலி Ast_முனை_DType* samep) மாறிலி override { திரும்பு same_முனை(samep); }
    Ast_முனை_DType* getChildDTypep() மாறிலி override { திரும்பு childDTypep(); }
    Ast_முனை_DType* subDTypep() மாறிலி override VL_MT_STABLE {
        திரும்பு m_refDTypep ? m_refDTypep : childDTypep();
    }
    வெறுமை refDTypep(Ast_முனை_DType* nodep) { m_refDTypep = nodep; }
    Ast_முனை_DType* virtRefDTypep() மாறிலி override { திரும்பு m_refDTypep; }
    வெறுமை virtRefDTypep(Ast_முனை_DType* nodep) override { refDTypep(nodep); }
    string name() மாறிலி override VL_MT_STABLE { திரும்பு m_name; }
    வெறுமை name(மாறிலி string& flag) override { m_name = flag; }
    // Keep the name the typedef declaring it gives, as it moves off it to the type table
    வெறுமை typedefName(மாறிலி string& name) { m_typedefName = name; }
    வெறுமை dump(std::ostream& str = std::cout) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str = std::cout) மாறிலி override;
    வெறுமை dumpSmall(std::ostream& str) மாறிலி override;
    string prettyDTypeName(bool full) மாறிலி override;
    // METHODS
    AstBasicDType* basicp() மாறிலி override VL_MT_STABLE { திரும்பு subDTypep()->basicp(); }
    முழு widthAlignBytes() மாறிலி override { திரும்பு subDTypep()->widthAlignBytes(); }
    முழு widthTotalBytes() மாறிலி override { திரும்பு subDTypep()->widthTotalBytes(); }
    size_t item_மொத்தம்() மாறிலி {
        size_t மொத்தம் = 0;
        க்கு (Ast_முனை* itemp = itemsp(); itemp; itemp = itemp->nextp()) ++மொத்தம்;
        திரும்பு மொத்தம்;
    }
    bool isCompound() மாறிலி override { திரும்பு false; }
};

class AstIfaceGenericDType final : பொது Ast_முனை_DType {
    // Generic interface that will be replaced with AstIfaceRefDType
    File_வரி* m_modportFileline;  // Where modport token was
    // dist-ast-dump-suppress  // Part of name()
    string m_modportName;  // "" = no modport
பொது:
    explicit AstIfaceGenericDType(File_வரி* fl)
        : ASTGEN_SUPER_IfaceGenericDType(fl) {
        dtypep(this);
    }
    AstIfaceGenericDType(File_வரி* fl, File_வரி* modportFl, மாறிலி string& modport)
        : ASTGEN_SUPER_IfaceGenericDType(fl)
        , m_modportFileline{modportFl}
        , m_modportName{modport} {
        dtypep(this);
    }
    ASTGEN_MEMBERS_AstIfaceGenericDType;
    வெறுமை dumpSmall(std::ostream& str) மாறிலி override;
    bool hasDType() மாறிலி override VL_MT_SAFE { திரும்பு true; }
    bool maybePointedTo() மாறிலி override VL_MT_SAFE { திரும்பு true; }
    bool undead() மாறிலி override { திரும்பு true; }
    Ast_முனை_DType* subDTypep() மாறிலி override VL_MT_STABLE { திரும்பு nullptr; }
    Ast_முனை_DType* virtRefDTypep() மாறிலி override { திரும்பு nullptr; }
    வெறுமை virtRefDTypep(Ast_முனை_DType* nodep) override {}
    bool similar_DType_முனை(மாறிலி Ast_முனை_DType* samep) மாறிலி override { திரும்பு this == samep; }
    AstBasicDType* basicp() மாறிலி override VL_MT_STABLE { திரும்பு nullptr; }
    முழு widthAlignBytes() மாறிலி override { திரும்பு 1; }
    முழு widthTotalBytes() மாறிலி override { திரும்பு 1; }
    string modportName() மாறிலி { திரும்பு m_modportName; }
    bool isModport() { திரும்பு !m_modportName.empty(); }
    bool isCompound() மாறிலி override { திரும்பு true; }
    File_வரி* modportFileline() மாறிலி { திரும்பு m_modportFileline; }
    string name() மாறிலி override { திரும்பு m_modportName; }
};

class AstIfaceRefDType final : பொது Ast_முனை_DType {
    // Reference to an interface, either for a port, or inside parent cell
    // @astgen op1 := paramsp : List[AstPin]
    //
    // @astgen ptr := m_ifacep : Optional[AstIface]  // Interface; cellp() should override
    // @astgen ptr := m_cellp : Optional[AstCell]  // When exact parent cell known; not a guess
    // @astgen ptr := m_modportp : Optional[AstModport]  // nullptr = unlinked or no modport
    File_வரி* m_modportFileline;  // Where modport token was
    string m_cellName;  // "" = no cell, such as when connects to 'input' iface
    string m_ifaceName;  // Interface name
    string m_modportName;  // "" = no modport
    bool m_virtual = false;  // True if virtual interface
பொது:
    AstIfaceRefDType(File_வரி* fl, மாறிலி string& cellName, மாறிலி string& ifaceName)
        : ASTGEN_SUPER_IfaceRefDType(fl)
        , m_modportFileline{nullptr}
        , m_cellName{cellName}
        , m_ifaceName{ifaceName}
        , m_modportName{""} {}
    AstIfaceRefDType(File_வரி* fl, File_வரி* modportFl, மாறிலி string& cellName,
                     மாறிலி string& ifaceName, மாறிலி string& modport)
        : ASTGEN_SUPER_IfaceRefDType(fl)
        , m_modportFileline{modportFl}
        , m_cellName{cellName}
        , m_ifaceName{ifaceName}
        , m_modportName{modport} {}
    AstIfaceRefDType(File_வரி* fl, File_வரி* modportFl, மாறிலி string& cellName,
                     மாறிலி string& ifaceName, மாறிலி string& modport, AstPin* paramsp)
        : ASTGEN_SUPER_IfaceRefDType(fl)
        , m_modportFileline{modportFl}
        , m_cellName{cellName}
        , m_ifaceName{ifaceName}
        , m_modportName{modport} {
        addParamsp(paramsp);
    }
    ASTGEN_MEMBERS_AstIfaceRefDType;

    // METHODS
    வெறுமை dump(std::ostream& str = std::cout) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str = std::cout) மாறிலி override;
    வெறுமை dumpSmall(std::ostream& str) மாறிலி override;
    string prettyDTypeName(bool full) மாறிலி override;
    AstBasicDType* basicp() மாறிலி override VL_MT_STABLE { திரும்பு nullptr; }
    bool similar_DType_முனை(மாறிலி Ast_முனை_DType* samep) மாறிலி override {
        // Each occurrence of a virtual interface type parses to its own node,
        // so pointer identity is insufficient; compare the referenced
        // interface instead, same equivalence as AstNode::computeCastable uses.
        மாறிலி AstIfaceRefDType* மாறிலி asamep = VN_DBG_AS(samep, IfaceRefDType);
        திரும்பு ifaceViaCellp() && ifaceViaCellp() == asamep->ifaceViaCellp()
               && modportName() == asamep->modportName() && isVirtual() == asamep->isVirtual();
    }
    முழு widthAlignBytes() மாறிலி override { திரும்பு 0; }
    முழு widthTotalBytes() மாறிலி override { திரும்பு 0; }
    bool isVirtual() மாறிலி { திரும்பு m_virtual; }
    வெறுமை isVirtual(bool flag) {
        m_virtual = flag;
        எனில் (flag) v3Global.setHasVirtIfaces();
    }
    File_வரி* modportFileline() மாறிலி { திரும்பு m_modportFileline; }
    வெறுமை modportFileline(File_வரி* மாறிலி modportFileline) { m_modportFileline = modportFileline; }
    string cellName() மாறிலி { திரும்பு m_cellName; }
    வெறுமை cellName(மாறிலி string& name) { m_cellName = name; }
    string ifaceName() மாறிலி { திரும்பு m_ifaceName; }
    string ifaceNameQ() மாறிலி { திரும்பு "'" + prettyName(ifaceName()) + "'"; }
    வெறுமை ifaceName(மாறிலி string& name) { m_ifaceName = name; }
    வெறுமை modportName(மாறிலி string& modportName) { m_modportName = modportName; }
    string modportName() மாறிலி { திரும்பு m_modportName; }
    AstIface* ifaceViaCellp() மாறிலி;  // Use cellp or ifacep
    AstIface* ifacep() மாறிலி { திரும்பு m_ifacep; }
    வெறுமை ifacep(AstIface* nodep) { m_ifacep = nodep; }
    AstCell* cellp() மாறிலி { திரும்பு m_cellp; }
    வெறுமை cellp(AstCell* nodep) { m_cellp = nodep; }
    AstModport* modportp() மாறிலி { திரும்பு m_modportp; }
    வெறுமை modportp(AstModport* modportp) { m_modportp = modportp; }
    bool isModport() { திரும்பு !m_modportName.empty(); }
    bool isCompound() மாறிலி override { திரும்பு true; }  // But not relevant
};
class AstMemberDType final : பொது Ast_முனை_DType {
    // A member of a struct/union
    // PARENT: AstNodeUOrStructDType
    // @astgen op1 := childDTypep : Optional[AstNodeDType]
    // @astgen op3 := valuep : Optional[AstNode]
    //
    // @astgen ptr := m_refDTypep : Optional[AstNodeDType]  // Elements of this type (post-width)
    string m_name;  // Name of variable
    string m_tag;  // Holds the string of the verilator tag -- used in JSON output.
    முழு m_lsb = -1;  // Within this level's packed struct, the LSB of the first bit of the member
    bool m_constrainedRand = false;  // Member has a constraint expression
    VRandAttr m_rand;  // Randomizability of this member (rand, randc, etc)
பொது:
    AstMemberDType(File_வரி* fl, மாறிலி string& name, VFlagChildDType, Ast_முனை_DType* dtp,
                   Ast_முனை* valuep)
        : ASTGEN_SUPER_MemberDType(fl)
        , m_name{name} {
        childDTypep(dtp);  // Only for parser
        this->valuep(valuep);
        dtypep(nullptr);  // V3Width will resolve
        refDTypep(nullptr);
    }
    AstMemberDType(File_வரி* fl, மாறிலி string& name, Ast_முனை_DType* dtp)
        : ASTGEN_SUPER_MemberDType(fl)
        , m_name{name} {
        UASSERT(dtp, "AstMember created with no dtype");
        refDTypep(dtp);
        dtypep(this);
        widthFromSub(subDTypep());
    }
    ASTGEN_MEMBERS_AstMemberDType;
    வெறுமை dump(std::ostream& str = std::cout) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str = std::cout) மாறிலி override;
    வெறுமை dumpSmall(std::ostream& str) மாறிலி override;
    string prettyDTypeName(bool full) மாறிலி override;
    string name() மாறிலி override VL_MT_STABLE { திரும்பு m_name; }  // * = Var name
    bool hasDType() மாறிலி override VL_MT_SAFE { திரும்பு true; }
    bool maybePointedTo() மாறிலி override VL_MT_SAFE { திரும்பு true; }
    Ast_முனை_DType* getChildDTypep() மாறிலி override { திரும்பு childDTypep(); }
    Ast_முனை_UOr_Struct_DType* getChildStructp();
    Ast_முனை_DType* subDTypep() மாறிலி override VL_MT_STABLE {
        திரும்பு m_refDTypep ? m_refDTypep : childDTypep();
    }
    வெறுமை refDTypep(Ast_முனை_DType* nodep) { m_refDTypep = nodep; }
    Ast_முனை_DType* virtRefDTypep() மாறிலி override { திரும்பு m_refDTypep; }
    வெறுமை virtRefDTypep(Ast_முனை_DType* nodep) override { refDTypep(nodep); }
    bool similar_DType_முனை(மாறிலி Ast_முனை_DType* samep) மாறிலி override { திரும்பு this == samep; }
    //
    // (Slow) recurse down to find basic data type (Note don't need virtual -
    // AstVar isn't a NodeDType)
    AstBasicDType* basicp() மாறிலி override VL_MT_STABLE { திரும்பு subDTypep()->basicp(); }
    // op1 = Range of variable (Note don't need virtual - AstVar isn't a NodeDType)
    Ast_முனை_DType* dtypeSkipRefp() மாறிலி { திரும்பு subDTypep()->skipRefp(); }
    // (Slow) recurses - Structure alignment 1,2,4 or 8 bytes (arrays affect this)
    முழு widthAlignBytes() மாறிலி override { திரும்பு subDTypep()->widthAlignBytes(); }
    // (Slow) recurses - Width in bytes rounding up 1,2,4,8,12,...
    முழு widthTotalBytes() மாறிலி override { திரும்பு subDTypep()->widthTotalBytes(); }
    // METHODS
    வெறுமை name(மாறிலி string& name) override { m_name = name; }
    string tag() மாறிலி override { திரும்பு m_tag; }
    வெறுமை tag(மாறிலி string& text) override { m_tag = text; }
    முழு lsb() மாறிலி { திரும்பு m_lsb; }
    வெறுமை lsb(முழு lsb) { m_lsb = lsb; }
    bool isCompound() மாறிலி override {
        v3fatalSrc("call isCompound on subdata type, not reference");
        திரும்பு false;
    }
    bool isConstrainedRand() மாறிலி { திரும்பு m_constrainedRand; }
    வெறுமை markConstrainedRand(bool flag) { m_constrainedRand = flag; }
    VRandAttr rand() மாறிலி { திரும்பு m_rand; }
    வெறுமை rand(மாறிலி VRandAttr flag) { m_rand = flag; }
};
class AstNBACommitQueueDType final : பொது Ast_முனை_DType {
    // @astgen ptr := m_subDTypep : AstNodeDType  // Type of the corresponding variable
    மாறிலி bool m_partial;  // Partial element update required

பொது:
    AstNBACommitQueueDType(File_வரி* fl, Ast_முனை_DType* subDTypep, bool partial)
        : ASTGEN_SUPER_NBACommitQueueDType(fl)
        , m_partial{partial}
        , m_subDTypep{subDTypep} {
        dtypep(this);
    }
    ASTGEN_MEMBERS_AstNBACommitQueueDType;
    வெறுமை dump(std::ostream& str) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str) மாறிலி override;
    Ast_முனை_DType* subDTypep() மாறிலி override VL_MT_STABLE { திரும்பு m_subDTypep; }
    bool partial() மாறிலி { திரும்பு m_partial; }
    bool same_முனை(மாறிலி Ast_முனை* samep) மாறிலி override {
        மாறிலி AstNBACommitQueueDType* மாறிலி asamep = VN_DBG_AS(samep, NBACommitQueueDType);
        திரும்பு m_partial == asamep->m_partial;
    }
    bool similar_DType_முனை(மாறிலி Ast_முனை_DType* samep) மாறிலி override { திரும்பு this == samep; }
    AstBasicDType* basicp() மாறிலி override VL_MT_STABLE { திரும்பு nullptr; }
    முழு widthAlignBytes() மாறிலி override { திரும்பு 1; }
    முழு widthTotalBytes() மாறிலி override { திரும்பு 24; }
    bool isCompound() மாறிலி override { திரும்பு true; }
};
class AstParamTypeDType final : பொது Ast_முனை_DType {
    // Parents: MODULE
    // A parameter type statement; much like a var or typedef
    // @astgen op1 := childDTypep : Optional[AstNodeDType]
    மாறிலி VVarType m_varType;  // Type of variable (for localparam vs. param)
    மாறிலி VFwdType m_fwdType;  // Forward type for lint check
    string m_name;  // Name of variable
பொது:
    AstParamTypeDType(File_வரி* fl, VVarType type, VFwdType fwdType, மாறிலி string& name,
                      VFlagChildDType, Ast_முனை_DType* dtp)
        : ASTGEN_SUPER_ParamTypeDType(fl)
        , m_varType{type}
        , m_fwdType{fwdType}
        , m_name{name} {
        childDTypep(dtp);  // Only for parser
        dtypep(nullptr);  // V3Width will resolve
    }
    ASTGEN_MEMBERS_AstParamTypeDType;
    வெறுமை dump(std::ostream& str = std::cout) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str = std::cout) மாறிலி override;
    string prettyDTypeName(bool full) மாறிலி override;
    Ast_முனை_DType* getChildDTypep() மாறிலி override { திரும்பு childDTypep(); }
    Ast_முனை_DType* subDTypep() மாறிலி override VL_MT_STABLE {
        திரும்பு dtypep() ? dtypep() : childDTypep();
    }
    AstBasicDType* basicp() மாறிலி override VL_MT_STABLE { திரும்பு subDTypep()->basicp(); }
    bool similar_DType_முனை(மாறிலி Ast_முனை_DType* samep) மாறிலி override {
        மாறிலி AstParamTypeDType* மாறிலி sp = VN_DBG_AS(samep, ParamTypeDType);
        திரும்பு this->subDTypep()->similarDType(sp->subDTypep());
    }
    முழு widthAlignBytes() மாறிலி override { திரும்பு dtypep()->widthAlignBytes(); }
    முழு widthTotalBytes() மாறிலி override { திரும்பு dtypep()->widthTotalBytes(); }
    // METHODS
    string name() மாறிலி override VL_MT_STABLE { திரும்பு m_name; }
    bool maybePointedTo() மாறிலி override VL_MT_SAFE { திரும்பு true; }
    bool hasDType() மாறிலி override VL_MT_SAFE { திரும்பு true; }
    வெறுமை name(மாறிலி string& flag) override { m_name = flag; }
    VVarType varType() மாறிலி { திரும்பு m_varType; }  // * = Type of variable
    VFwdType fwdType() மாறிலி { திரும்பு m_fwdType; }
    bool isParam() மாறிலி { திரும்பு true; }
    bool isGParam() மாறிலி { திரும்பு (varType() == VVarType::GPARAM); }
    bool isCompound() மாறிலி override {
        v3fatalSrc("call isCompound on subdata type, not reference");
        திரும்பு false;
    }
};
class AstParseTypeDType final : பொது Ast_முனை_DType {
    // Parents: VAR
    // During parsing, this indicates the type of a parameter is a "parameter type"
    // e.g. the data type is a container of any data type
    மாறிலி VFwdType m_fwdType;  // Forward type for lint check
பொது:
    explicit AstParseTypeDType(File_வரி* fl, VFwdType fwdType = VFwdType::NONE)
        : ASTGEN_SUPER_ParseTypeDType(fl)
        , m_fwdType{fwdType} {}
    ASTGEN_MEMBERS_AstParseTypeDType;
    Ast_முனை_DType* dtypep() மாறிலி VL_MT_STABLE { திரும்பு nullptr; }
    // METHODS
    வெறுமை dump(std::ostream& str = std::cout) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str = std::cout) மாறிலி override;
    bool similar_DType_முனை(மாறிலி Ast_முனை_DType* samep) மாறிலி override { திரும்பு this == samep; }
    AstBasicDType* basicp() மாறிலி override VL_MT_STABLE { திரும்பு nullptr; }
    முழு widthAlignBytes() மாறிலி override { திரும்பு 0; }
    முழு widthTotalBytes() மாறிலி override { திரும்பு 0; }
    bool isCompound() மாறிலி override {
        v3fatalSrc("call isCompound on subdata type, not reference");
        திரும்பு false;
    }
    VFwdType fwdType() மாறிலி { திரும்பு m_fwdType; }
};
class AstQueueDType final : பொது Ast_முனை_DType {
    // Queue array data type, ie "[ $ ]"
    // @astgen op1 := childDTypep : Optional[AstNodeDType] // moved to refDTypep() in V3Width
    // @astgen op2 := boundp : Optional[AstNodeExpr]
    //
    // @astgen ptr := m_refDTypep : Optional[AstNodeDType]  // Elements of this type (post-width)
பொது:
    AstQueueDType(File_வரி* fl, VFlagChildDType, Ast_முனை_DType* dtp, Ast_முனை_கோவை* boundp)
        : ASTGEN_SUPER_QueueDType(fl) {
        childDTypep(dtp);
        this->boundp(boundp);
        refDTypep(nullptr);
        dtypep(nullptr);  // V3Width will resolve
    }
    AstQueueDType(File_வரி* fl, Ast_முனை_DType* dtp, Ast_முனை_கோவை* boundp)
        : ASTGEN_SUPER_QueueDType(fl) {
        this->boundp(boundp);
        refDTypep(dtp);
        dtypep(dtp);
    }
    ASTGEN_MEMBERS_AstQueueDType;
    மாறிலி எழுத்து* broken() மாறிலி override {
        BROKEN_RTN(!((m_refDTypep && !childDTypep()) || (!m_refDTypep && childDTypep())));
        திரும்பு nullptr;
    }
    bool same_முனை(மாறிலி Ast_முனை* samep) மாறிலி override {
        மாறிலி AstQueueDType* மாறிலி asamep = VN_DBG_AS(samep, QueueDType);
        எனில் (!asamep->subDTypep()) திரும்பு false;
        திரும்பு (subDTypep() == asamep->subDTypep());
    }
    bool similar_DType_முனை(மாறிலி Ast_முனை_DType* samep) மாறிலி override {
        மாறிலி AstQueueDType* மாறிலி asamep = VN_DBG_AS(samep, QueueDType);
        திரும்பு asamep->subDTypep() && subDTypep()->similarDType(asamep->subDTypep());
    }
    வெறுமை dumpSmall(std::ostream& str) மாறிலி override;
    string prettyDTypeName(bool full) மாறிலி override;
    Ast_முனை_DType* getChildDTypep() மாறிலி override { திரும்பு childDTypep(); }
    Ast_முனை_DType* subDTypep() மாறிலி override VL_MT_STABLE {
        திரும்பு m_refDTypep ? m_refDTypep : childDTypep();
    }
    bool isAggregateType() மாறிலி override { திரும்பு true; }
    வெறுமை refDTypep(Ast_முனை_DType* nodep) { m_refDTypep = nodep; }
    உள்வரி முழு boundConst() மாறிலி VL_MT_STABLE;
    Ast_முனை_DType* virtRefDTypep() மாறிலி override { திரும்பு m_refDTypep; }
    வெறுமை virtRefDTypep(Ast_முனை_DType* nodep) override { refDTypep(nodep); }
    // METHODS
    AstBasicDType* basicp() மாறிலி override VL_MT_STABLE { திரும்பு nullptr; }
    முழு widthAlignBytes() மாறிலி override { திரும்பு subDTypep()->widthAlignBytes(); }
    முழு widthTotalBytes() மாறிலி override { திரும்பு subDTypep()->widthTotalBytes(); }
    bool isCompound() மாறிலி override { திரும்பு true; }
    bool isDynamicallySized() மாறிலி override { திரும்பு true; }
};
class AstRefDType final : பொது Ast_முனை_DType {
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
    மாறிலி VIfaceCaptureTag* m_captureTagp = nullptr;

பொது:
    AstRefDType(File_வரி* fl, மாறிலி string& name)
        : ASTGEN_SUPER_RefDType(fl)
        , m_name{name} {}
    AstRefDType(File_வரி* fl, மாறிலி string& name, Ast_முனை_கோவை* classOrPackagep, AstPin* paramsp)
        : ASTGEN_SUPER_RefDType(fl)
        , m_name{name} {
        classOrPackageOpp(classOrPackagep);
        addParamsp(paramsp);
    }
    class Flag_Type_Of_கோவை {};  // type(expr) for parser only
    AstRefDType(File_வரி* fl, Flag_Type_Of_கோவை, Ast_முனை* typeofp)
        : ASTGEN_SUPER_RefDType(fl) {
        this->typeofp(typeofp);
        எனில் (Ast_முனை_DType* மாறிலி dtp = VN_CAST(typeofp, முனை_DType)) refDTypep(dtp);
    }
    ASTGEN_MEMBERS_AstRefDType;
    // METHODS
    bool same_முனை(மாறிலி Ast_முனை* samep) மாறிலி override {
        மாறிலி AstRefDType* மாறிலி asamep = VN_DBG_AS(samep, RefDType);
        திரும்பு (m_typedefp == asamep->m_typedefp && m_refDTypep == asamep->m_refDTypep
                && m_name == asamep->m_name && m_classOrPackagep == asamep->m_classOrPackagep);
    }
    bool similar_DType_முனை(மாறிலி Ast_முனை_DType* samep) மாறிலி override {
        திரும்பு subDTypep()->similarDType(samep->subDTypep());
    }
    மாறிலி எழுத்து* broken() மாறிலி override;
    வெறுமை dump(std::ostream& str = std::cout) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str = std::cout) மாறிலி override;
    வெறுமை dumpSmall(std::ostream& str) மாறிலி override;
    string name() மாறிலி override VL_MT_STABLE { திரும்பு m_name; }
    string prettyDTypeName(bool full) மாறிலி override {
        திரும்பு subDTypep() ? subDTypep()->prettyDTypeName(full) : prettyName();
    }
    AstBasicDType* basicp() மாறிலி override VL_MT_STABLE {
        திரும்பு subDTypep() ? subDTypep()->basicp() : nullptr;
    }
    Ast_முனை_DType* subDTypep() மாறிலி override VL_MT_STABLE;
    Ast_முனை_DType* getChildDTypep() மாறிலி override { திரும்பு VN_CAST(typeofp(), முனை_DType); }
    முழு widthAlignBytes() மாறிலி override { திரும்பு dtypeSkipRefp()->widthAlignBytes(); }
    முழு widthTotalBytes() மாறிலி override { திரும்பு dtypeSkipRefp()->widthTotalBytes(); }
    வெறுமை name(மாறிலி string& flag) override { m_name = flag; }
    Ast_முனை_DType* dtypeSkipRefp() மாறிலி { திரும்பு subDTypep()->skipRefp(); }
    AstTypedef* typedefp() மாறிலி VL_MT_SAFE { திரும்பு m_typedefp; }
    வெறுமை typedefp(AstTypedef* nodep) { m_typedefp = nodep; }
    Ast_முனை_DType* refDTypep() மாறிலி VL_MT_SAFE { திரும்பு m_refDTypep; }
    வெறுமை refDTypep(Ast_முனை_DType* nodep) { m_refDTypep = nodep; }
    Ast_முனை_DType* virtRefDTypep() மாறிலி override { திரும்பு refDTypep(); }
    வெறுமை virtRefDTypep(Ast_முனை_DType* nodep) override { refDTypep(nodep); }
    Ast_முனை_Module* classOrPackagep() மாறிலி { திரும்பு m_classOrPackagep; }
    வெறுமை classOrPackagep(Ast_முனை_Module* nodep) { m_classOrPackagep = nodep; }
    மாறிலி VIfaceCaptureTag* captureTagp() மாறிலி { திரும்பு m_captureTagp; }
    வெறுமை captureTagp(மாறிலி VIfaceCaptureTag* tagp) { m_captureTagp = tagp; }
    bool isCompound() மாறிலி override {
        v3fatalSrc("call isCompound on subdata type, not reference");
        திரும்பு false;
    }
};
class AstRequireDType final : பொது Ast_முனை_DType {
    // @astgen op1 := lhsp : Optional[AstNode<AstNodeExpr|AstNodeDType>]
    //
    // Require a generic node type (typically AstParseRef become a type.
பொது:
    AstRequireDType(File_வரி* fl, Ast_முனை* lhsp)
        : ASTGEN_SUPER_RequireDType(fl) {
        this->lhsp(lhsp);
    }
    ASTGEN_MEMBERS_AstRequireDType;
    // METHODS
    bool similar_DType_முனை(மாறிலி Ast_முனை_DType* samep) மாறிலி override {
        மாறிலி AstRequireDType* மாறிலி asamep = VN_DBG_AS(samep, RequireDType);
        திரும்பு subDTypep()->similarDType(asamep->subDTypep());
    }
    AstBasicDType* basicp() மாறிலி override VL_MT_STABLE { திரும்பு nullptr; }
    Ast_முனை_DType* subDTypep() மாறிலி override VL_MT_STABLE {
        // Used for recursive definition checking
        எனில் (Ast_முனை_DType* மாறிலி dtp = VN_CAST(lhsp(), முனை_DType)) திரும்பு dtp;

        திரும்பு nullptr;
    }
    முழு widthAlignBytes() மாறிலி override { V3ERROR_NA_RETURN(1); }
    முழு widthTotalBytes() மாறிலி override { V3ERROR_NA_RETURN(1); }
    bool isCompound() மாறிலி override { V3ERROR_NA_RETURN(false); }
};
class AstSampleQueueDType final : பொது Ast_முனை_DType {
    // @astgen op1 := childDTypep : Optional[AstNodeDType] // moved to refDTypep() in V3Width
    //
    // @astgen ptr := m_refDTypep : Optional[AstNodeDType]  // Elements of this type (post-width)
பொது:
    AstSampleQueueDType(File_வரி* fl, Ast_முனை_DType* dtp)
        : ASTGEN_SUPER_SampleQueueDType(fl) {
        refDTypep(dtp);
        dtypep(dtp);
    }
    ASTGEN_MEMBERS_AstSampleQueueDType;
    மாறிலி எழுத்து* broken() மாறிலி override {
        BROKEN_RTN(!((m_refDTypep && !childDTypep()) || (!m_refDTypep && childDTypep())));
        திரும்பு nullptr;
    }
    bool same_முனை(மாறிலி Ast_முனை* samep) மாறிலி override {
        மாறிலி AstSampleQueueDType* மாறிலி asamep = VN_DBG_AS(samep, SampleQueueDType);
        எனில் (!asamep->subDTypep()) திரும்பு false;
        திரும்பு (subDTypep() == asamep->subDTypep());
    }
    bool similar_DType_முனை(மாறிலி Ast_முனை_DType* samep) மாறிலி override {
        மாறிலி AstSampleQueueDType* மாறிலி asamep = VN_DBG_AS(samep, SampleQueueDType);
        திரும்பு asamep->subDTypep() && subDTypep()->similarDType(asamep->subDTypep());
    }
    வெறுமை dumpSmall(std::ostream& str) மாறிலி override;
    Ast_முனை_DType* getChildDTypep() மாறிலி override { திரும்பு childDTypep(); }
    // op1 = Range of variable
    Ast_முனை_DType* subDTypep() மாறிலி override VL_MT_STABLE {
        திரும்பு m_refDTypep ? m_refDTypep : childDTypep();
    }
    வெறுமை refDTypep(Ast_முனை_DType* nodep) { m_refDTypep = nodep; }
    Ast_முனை_DType* virtRefDTypep() மாறிலி override { திரும்பு m_refDTypep; }
    வெறுமை virtRefDTypep(Ast_முனை_DType* nodep) override { refDTypep(nodep); }
    // METHODS
    AstBasicDType* basicp() மாறிலி override VL_MT_STABLE { திரும்பு subDTypep()->basicp(); }
    முழு widthAlignBytes() மாறிலி override { திரும்பு அளவு(std::map<std::string, std::string>); }
    முழு widthTotalBytes() மாறிலி override { திரும்பு அளவு(std::map<std::string, std::string>); }
    bool isCompound() மாறிலி override { திரும்பு true; }
};
class AstStreamDType final : பொது Ast_முனை_DType {
    // Stream data type, used only as data type of stream operations
    // Should behave like AstPackArrayDType, but it doesn't have a size
பொது:
    explicit AstStreamDType(File_வரி* fl)
        : ASTGEN_SUPER_StreamDType(fl) {
        dtypep(this);
    }
    ASTGEN_MEMBERS_AstStreamDType;
    வெறுமை dumpSmall(std::ostream& str) மாறிலி override;
    bool hasDType() மாறிலி override VL_MT_SAFE { திரும்பு true; }
    bool maybePointedTo() மாறிலி override VL_MT_SAFE { திரும்பு true; }
    bool undead() மாறிலி override { திரும்பு true; }
    Ast_முனை_DType* subDTypep() மாறிலி override VL_MT_STABLE { திரும்பு nullptr; }
    Ast_முனை_DType* virtRefDTypep() மாறிலி override { திரும்பு nullptr; }
    வெறுமை virtRefDTypep(Ast_முனை_DType* nodep) override {}
    bool similar_DType_முனை(மாறிலி Ast_முனை_DType* samep) மாறிலி override { திரும்பு this == samep; }
    AstBasicDType* basicp() மாறிலி override VL_MT_STABLE { திரும்பு nullptr; }
    முழு widthAlignBytes() மாறிலி override { திரும்பு 1; }
    முழு widthTotalBytes() மாறிலி override { திரும்பு 1; }
    bool isCompound() மாறிலி override { திரும்பு false; }
};
class AstUnsizedArrayDType final : பொது Ast_முனை_DType {
    // Unsized/open-range Array data type, ie "some_dtype var_name []"
    // @astgen op1 := childDTypep : Optional[AstNodeDType] // moved to refDTypep() in V3Width
    //
    // @astgen ptr := m_refDTypep : Optional[AstNodeDType]  // Elements of this type (post-width)
பொது:
    AstUnsizedArrayDType(File_வரி* fl, VFlagChildDType, Ast_முனை_DType* dtp)
        : ASTGEN_SUPER_UnsizedArrayDType(fl) {
        childDTypep(dtp);  // Only for parser
        refDTypep(nullptr);
        dtypep(nullptr);  // V3Width will resolve
    }
    AstUnsizedArrayDType(File_வரி* fl, Ast_முனை_DType* dtp)
        : ASTGEN_SUPER_UnsizedArrayDType(fl) {
        refDTypep(dtp);
        dtypep(nullptr);  // V3Width will resolve
    }
    ASTGEN_MEMBERS_AstUnsizedArrayDType;
    மாறிலி எழுத்து* broken() மாறிலி override {
        BROKEN_RTN(!((m_refDTypep && !childDTypep()) || (!m_refDTypep && childDTypep())));
        திரும்பு nullptr;
    }
    bool same_முனை(மாறிலி Ast_முனை* samep) மாறிலி override;
    bool similar_DType_முனை(மாறிலி Ast_முனை_DType* samep) மாறிலி override;
    வெறுமை dumpSmall(std::ostream& str) மாறிலி override;
    Ast_முனை_DType* getChildDTypep() மாறிலி override { திரும்பு childDTypep(); }
    Ast_முனை_DType* subDTypep() மாறிலி override VL_MT_STABLE {
        திரும்பு m_refDTypep ? m_refDTypep : childDTypep();
    }
    வெறுமை refDTypep(Ast_முனை_DType* nodep) { m_refDTypep = nodep; }
    Ast_முனை_DType* virtRefDTypep() மாறிலி override { திரும்பு m_refDTypep; }
    வெறுமை virtRefDTypep(Ast_முனை_DType* nodep) override { refDTypep(nodep); }
    // METHODS
    AstBasicDType* basicp() மாறிலி override VL_MT_STABLE { திரும்பு subDTypep()->basicp(); }
    முழு widthAlignBytes() மாறிலி override { திரும்பு subDTypep()->widthAlignBytes(); }
    முழு widthTotalBytes() மாறிலி override { திரும்பு subDTypep()->widthTotalBytes(); }
    bool isCompound() மாறிலி override { திரும்பு true; }
};
class AstVoidDType final : பொது Ast_முனை_DType {
    // For e.g. a function returning void
பொது:
    explicit AstVoidDType(File_வரி* fl)
        : ASTGEN_SUPER_VoidDType(fl) {
        dtypep(this);
    }
    ASTGEN_MEMBERS_AstVoidDType;
    வெறுமை dumpSmall(std::ostream& str) மாறிலி override;
    bool hasDType() மாறிலி override VL_MT_SAFE { திரும்பு true; }
    bool maybePointedTo() மாறிலி override VL_MT_SAFE { திரும்பு true; }
    bool undead() மாறிலி override { திரும்பு true; }
    Ast_முனை_DType* subDTypep() மாறிலி override VL_MT_STABLE { திரும்பு nullptr; }
    Ast_முனை_DType* virtRefDTypep() மாறிலி override { திரும்பு nullptr; }
    வெறுமை virtRefDTypep(Ast_முனை_DType* nodep) override {}
    bool similar_DType_முனை(மாறிலி Ast_முனை_DType* samep) மாறிலி override { திரும்பு true; }
    AstBasicDType* basicp() மாறிலி override VL_MT_STABLE { திரும்பு nullptr; }
    முழு widthAlignBytes() மாறிலி override { திரும்பு 1; }
    முழு widthTotalBytes() மாறிலி override { திரும்பு 1; }
    bool isCompound() மாறிலி override { திரும்பு false; }
};
class AstWildcardArrayDType final : பொது Ast_முனை_DType {
    // Wildcard index type associative array data type, ie "some_dtype var_name [*]"
    // @astgen op1 := childDTypep : Optional[AstNodeDType] // moved to refDTypep() in V3Width
    //
    // @astgen ptr := m_refDTypep : Optional[AstNodeDType]  // Elements of this type (post-width)
பொது:
    AstWildcardArrayDType(File_வரி* fl, VFlagChildDType, Ast_முனை_DType* dtp)
        : ASTGEN_SUPER_WildcardArrayDType(fl) {
        childDTypep(dtp);  // Only for parser
        refDTypep(nullptr);
        dtypep(nullptr);  // V3Width will resolve
    }
    ASTGEN_MEMBERS_AstWildcardArrayDType;
    மாறிலி எழுத்து* broken() மாறிலி override {
        BROKEN_RTN(!((m_refDTypep && !childDTypep()) || (!m_refDTypep && childDTypep())));
        திரும்பு nullptr;
    }
    bool same_முனை(மாறிலி Ast_முனை* samep) மாறிலி override;
    bool similar_DType_முனை(மாறிலி Ast_முனை_DType* samep) மாறிலி override;
    வெறுமை dumpSmall(std::ostream& str) மாறிலி override;
    string prettyDTypeName(bool full) மாறிலி override;
    Ast_முனை_DType* getChildDTypep() மாறிலி override { திரும்பு childDTypep(); }
    Ast_முனை_DType* subDTypep() மாறிலி override VL_MT_STABLE {
        திரும்பு m_refDTypep ? m_refDTypep : childDTypep();
    }
    வெறுமை refDTypep(Ast_முனை_DType* nodep) { m_refDTypep = nodep; }
    Ast_முனை_DType* virtRefDTypep() மாறிலி override { திரும்பு m_refDTypep; }
    வெறுமை virtRefDTypep(Ast_முனை_DType* nodep) override { refDTypep(nodep); }
    // METHODS
    AstBasicDType* basicp() மாறிலி override VL_MT_STABLE { திரும்பு subDTypep()->basicp(); }
    முழு widthAlignBytes() மாறிலி override { திரும்பு அளவு(std::map<std::string, std::string>); }
    முழு widthTotalBytes() மாறிலி override { திரும்பு அளவு(std::map<std::string, std::string>); }
    bool isCompound() மாறிலி override { திரும்பு true; }
    bool isDynamicallySized() மாறிலி override { திரும்பு true; }
};

// === AstNodeArrayDType ===
class AstPackArrayDType final : பொது Ast_முனை_Array_DType {
    // Packed array data type, ie "some_dtype [2:0] var_name"
பொது:
    உள்வரி AstPackArrayDType(File_வரி* fl, VFlagChildDType, Ast_முனை_DType* dtp, AstRange* rangep);
    உள்வரி AstPackArrayDType(File_வரி* fl, Ast_முனை_DType* dtp, AstRange* rangep);
    ASTGEN_MEMBERS_AstPackArrayDType;
    string prettyDTypeName(bool full) மாறிலி override;
    bool isCompound() மாறிலி override { திரும்பு false; }
};
class AstUnpackArrayDType final : பொது Ast_முனை_Array_DType {
    // Array data type, ie "some_dtype var_name [2:0]"
    bool m_isCompound = false;  // Non-POD subDType, or parent requires compound
பொது:
    AstUnpackArrayDType(File_வரி* fl, VFlagChildDType, Ast_முனை_DType* dtp, AstRange* rangep)
        : ASTGEN_SUPER_UnpackArrayDType(fl) {
        childDTypep(dtp);  // Only for parser
        this->rangep(rangep);
        refDTypep(nullptr);
        dtypep(nullptr);  // V3Width will resolve
        // For backward compatibility AstNodeArrayDType and others inherit
        // width and signing from the subDType/base type
        widthFromSub(subDTypep());
    }
    AstUnpackArrayDType(File_வரி* fl, Ast_முனை_DType* dtp, AstRange* rangep)
        : ASTGEN_SUPER_UnpackArrayDType(fl) {
        this->rangep(rangep);
        refDTypep(dtp);
        dtypep(this);
        // For backward compatibility AstNodeArrayDType and others inherit
        // width and signing from the subDType/base type
        widthFromSub(subDTypep());
    }
    ASTGEN_MEMBERS_AstUnpackArrayDType;
    வெறுமை dump(std::ostream& str) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str) மாறிலி override;
    string prettyDTypeName(bool full) மாறிலி override;
    bool same_முனை(மாறிலி Ast_முனை* samep) மாறிலி override {
        மாறிலி AstUnpackArrayDType* மாறிலி sp = VN_DBG_AS(samep, UnpackArrayDType);
        திரும்பு m_isCompound == sp->m_isCompound;
    }
    bool similar_DType_முனை(மாறிலி Ast_முனை_DType* samep) மாறிலி override;
    bool isAggregateType() மாறிலி override { திரும்பு true; }
    // True iff 'otherp' is an array of the same shape, with
    // at least one dimension reversed ascending/descending.
    bool isReverseOf(மாறிலி Ast_முனை_DType* otherp) மாறிலி;
    // Outer dimension comes first. The first element is this node.
    std::vector<AstUnpackArrayDType*> unpackDimensions();
    வெறுமை isCompound(bool flag) { m_isCompound = flag; }
    bool isCompound() மாறிலி override VL_MT_SAFE { திரும்பு m_isCompound; }
    bool isIntegralOrPacked() மாறிலி override { திரும்பு false; }
};

// === AstNodeUOrStructDType ===
class AstStructDType final : பொது Ast_முனை_UOr_Struct_DType {
பொது:
    // VSigning below is mispurposed to indicate if packed or not
    AstStructDType(File_வரி* fl, VSigning numericUnpack)
        : ASTGEN_SUPER_StructDType(fl, numericUnpack) {}
    ASTGEN_MEMBERS_AstStructDType;
    string verilogKwd() மாறிலி override { திரும்பு "struct"; }
};
class AstUnionDType final : பொது Ast_முனை_UOr_Struct_DType {
    bool m_isSoft;  // Is a "union soft"
    bool m_isTagged;  // Is a "union tagged"

பொது:
    // VSigning below is mispurposed to indicate if packed or not
    // isSoft implies packed
    AstUnionDType(File_வரி* fl, bool isSoft, bool isTagged, VSigning numericUnpack)
        : ASTGEN_SUPER_UnionDType(fl, numericUnpack)
        , m_isSoft{isSoft}
        , m_isTagged{isTagged} {
        packed(packed() | m_isSoft);
    }
    ASTGEN_MEMBERS_AstUnionDType;
    string verilogKwd() மாறிலி override { திரும்பு "union"; }
    bool isSoft() மாறிலி { திரும்பு m_isSoft; }
    bool isTagged() மாறிலி { திரும்பு m_isTagged; }
    bool same_முனை(மாறிலி Ast_முனை* samep) மாறிலி override;
    வெறுமை dump(std::ostream& str) மாறிலி override;
    வெறுமை dumpJson(std::ostream& str) மாறிலி override;
};

#endif  // Guard
