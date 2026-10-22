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
#अन्तर्भाव "V3Ast.h"  // This helps code analysis tools pick up symbols in V3Ast.h
#define VL_NOT_FINAL  // This #define fixes broken code folding in the CLion IDE
#endif

// === Abstract base node types (AstNode*) =====================================

class Ast_गाँठो_DType VL_NOT_FINAL : सार्वजनिक Ast_गाँठो {
    // Ideally width() would migrate to BasicDType as that's where it makes sense,
    // but it's currently so prevalent in the code we leave it here.
    // Note the below members are included in AstTypeTable::Key lookups
    // dist-ast-dump-suppress  // Part of dumpSmall
    पूर्णाङ्क m_width = 0;  // (also in AstTypeTable::Key) Bit width of operation
    // (also in AstTypeTable::Key) If unsized, bitwidth of minimum implementation
    // dist-ast-dump-suppress  // Part of dumpSmall
    पूर्णाङ्क m_widthMin = 0;
    // dist-ast-dump-suppress  // Part of dumpSmall
    VSigning m_numeric;  // (also in AstTypeTable::Key) Node is signed
    // dist-ast-dump-suppress  // Part of dumpSmall
    bool m_generic = false;  // Simple globally referenced type, don't garbage collect
    // Unique number assigned to each dtype during creation for IEEE matching
    स्थिर पूर्णाङ्क s_unique_अङ्क;

रक्षित:
    // CONSTRUCTORS
    Ast_गाँठो_DType(VNType t, File_पङ्क्ति* fl)
        : Ast_गाँठो{t, fl} {}

गुप्त:
    // METHODS
    अचर Ast_गाँठो_DType* skipRefIterp(bool skipConst, bool skipEnum,
                                     bool assertOn = true) अचर VL_MT_STABLE;

रक्षित:
    // METHODS
    आभास bool similar_DType_गाँठो(अचर Ast_गाँठो_DType* samep) अचर = 0;

सार्वजनिक:
    ASTGEN_MEMBERS_Ast_गाँठो_DType;
    // ACCESSORS
    शून्य dump(std::ostream& str) अचर override;
    शून्य dumpJson(std::ostream& str) अचर override;
    आभास शून्य dumpSmall(std::ostream& str) अचर VL_MT_STABLE;
    bool hasDType() अचर override VL_MT_SAFE { फर्काऊ true; }
    /// Require VlUnpacked, instead of [] for POD elements.
    /// A non-POD object is always compound, but some POD elements
    /// are compound when methods calls operate on object, or when
    /// under another compound-requiring object e.g. class
    आभास bool isCompound() अचर = 0;
    // Integral or packed, allowed inside an unpacked union/struct
    आभास bool isIntegralOrPacked() अचर { फर्काऊ !isCompound(); }
    // (Slow) recurse down to find basic data type
    आभास AstBasicDType* basicp() अचर VL_MT_STABLE = 0;
    // (Slow) Recurse over MemberDType|ParamTypeDType|RefDType|ConstDType|EnumDType to other type
    अचर Ast_गाँठो_DType* skipRefp() अचर VL_MT_STABLE { फर्काऊ skipRefIterp(true, true); }
    Ast_गाँठो_DType* skipRefp() VL_MT_STABLE {
        फर्काऊ const_cast<Ast_गाँठो_DType*>(
            static_cast<अचर Ast_गाँठो_DType*>(this)->skipRefIterp(true, true));
    }
    // (Slow) Recurse over MemberDType|ParamTypeDType|RefDType|ConstDType|EnumDType to other type,
    // Returns null if not resolved
    अचर Ast_गाँठो_DType* skipRefOrNullp() अचर VL_MT_STABLE {
        फर्काऊ skipRefIterp(true, true, false);
    }
    Ast_गाँठो_DType* skipRefOrNullp() VL_MT_STABLE {
        फर्काऊ const_cast<Ast_गाँठो_DType*>(
            static_cast<अचर Ast_गाँठो_DType*>(this)->skipRefIterp(true, true, false));
    }
    // (Slow) Recurse over MemberDType|ParamTypeDType|RefDType|EnumDType to ConstDType
    अचर Ast_गाँठो_DType* skipRefToConstp() अचर { फर्काऊ skipRefIterp(false, true); }
    Ast_गाँठो_DType* skipRefToConstp() {
        फर्काऊ const_cast<Ast_गाँठो_DType*>(
            static_cast<अचर Ast_गाँठो_DType*>(this)->skipRefIterp(false, true));
    }
    // (Slow) Recurse over MemberDType|ParamTypeDType|RefDType|ConstDType to EnumDType
    अचर Ast_गाँठो_DType* skipRefToEnump() अचर { फर्काऊ skipRefIterp(true, false); }
    Ast_गाँठो_DType* skipRefToEnump() {
        फर्काऊ const_cast<Ast_गाँठो_DType*>(
            static_cast<अचर Ast_गाँठो_DType*>(this)->skipRefIterp(true, false));
    }
    // (Slow) Recurse over MemberDType|ParamTypeDType|RefDType|ConstDType to EnumDType,
    // Returns null if not resolved
    अचर Ast_गाँठो_DType* skipRefToEnumOrNullp() अचर { फर्काऊ skipRefIterp(true, false, false); }
    // (Slow) Recurse over MemberDType|ParamTypeDType|RefDType to other type
    अचर Ast_गाँठो_DType* skipRefToNonRefp() अचर { फर्काऊ skipRefIterp(false, false); }
    Ast_गाँठो_DType* skipRefToNonRefp() {
        फर्काऊ const_cast<Ast_गाँठो_DType*>(
            static_cast<अचर Ast_गाँठो_DType*>(this)->skipRefIterp(false, false));
    }
    // If array, returns element dtype, otherwise returns skipRef dtype
    // If skipRef is false, RefDTypes are not followed (safe before typedef linking)
    अचर Ast_गाँठो_DType* elemDTypep(bool skipRef = true) अचर VL_MT_STABLE;
    Ast_गाँठो_DType* elemDTypep(bool skipRef = true) VL_MT_STABLE {
        फर्काऊ const_cast<Ast_गाँठो_DType*>(
            static_cast<अचर Ast_गाँठो_DType*>(this)->elemDTypep(skipRef));
    }
    // (Slow) recurses - Structure alignment 1,2,4 or 8 bytes (arrays affect this)
    आभास पूर्णाङ्क widthAlignBytes() अचर = 0;
    // (Slow) recurses - Width in bytes rounding up 1,2,4,8,12,...
    आभास पूर्णाङ्क widthTotalBytes() अचर = 0;
    bool maybePointedTo() अचर override VL_MT_SAFE { फर्काऊ true; }
    // Iff has a non-null refDTypep(), as generic node function
    आभास Ast_गाँठो_DType* virtRefDTypep() अचर { फर्काऊ nullptr; }
    // Iff has refDTypep(), set as generic node function
    आभास शून्य virtRefDTypep(Ast_गाँठो_DType* nodep) {}
    // Iff has a non-null second dtypep, as generic node function
    आभास Ast_गाँठो_DType* virtRefDType2p() अचर { फर्काऊ nullptr; }
    // Iff has second dtype, set as generic node function
    आभास शून्य virtRefDType2p(Ast_गाँठो_DType* nodep) {}
    // Assignable equivalence.  Calls skipRefToNonRefp() during comparisons.
    bool similarDType(अचर Ast_गाँठो_DType* samep) अचर;
    // Iff has a non-null subDTypep(), as generic node function
    आभास Ast_गाँठो_DType* subDTypep() अचर VL_MT_STABLE { फर्काऊ nullptr; }
    आभास Ast_गाँठो_DType* subDType2p() अचर VL_MT_STABLE { फर्काऊ nullptr; }
    आभास bool isAggregateType() अचर { फर्काऊ false; }
    // True for unpacked, dynamic, queue, and associative arrays (not packed arrays)
    bool isNonPackedArray() अचर;
    आभास bool isFourstate() अचर;
    // Ideally an IEEE $typename
    आभास string prettyDTypeName(bool) अचर { फर्काऊ prettyTypeName(); }
    string prettyDTypeNameQ(bool full = false) अचर { फर्काऊ "'" + prettyDTypeName(full) + "'"; }
    string stateDTypeName() अचर { फर्काऊ this->isFourstate() ? "(4-state)" : "(2-state)"; }
    //
    // Changing the width may confuse the data type resolution, so must clear
    // TypeTable cache after use.
    शून्य widthForce(पूर्णाङ्क width, पूर्णाङ्क widthMin) {
        m_width = width;
        m_widthMin = widthMin;
    }
    // For backward compatibility inherit width and signing from the subDType/base type
    शून्य widthFromSub(अचर Ast_गाँठो_DType* nodep) {
        m_width = nodep->m_width;
        m_widthMin = nodep->m_widthMin;
        m_numeric = nodep->m_numeric;
    }
    //
    पूर्णाङ्क width() अचर VL_MT_STABLE { फर्काऊ m_width; }
    शून्य numeric(VSigning flag) { m_numeric = flag; }
    bool isSigned() अचर VL_MT_STABLE { फर्काऊ m_numeric.isSigned(); }
    bool isNosign() अचर VL_MT_SAFE { फर्काऊ m_numeric.isNosign(); }
    VSigning numeric() अचर VL_MT_STABLE { फर्काऊ m_numeric; }
    पूर्णाङ्क width_शब्दहरू() अचर VL_MT_STABLE { फर्काऊ VL_शब्दहरू_I(width()); }
    पूर्णाङ्क widthMin() अचर VL_MT_STABLE {  // If sized, the size,
                                         // if unsized the min digits to represent it
        फर्काऊ m_widthMin ? m_widthMin : m_width;
    }
    पूर्णाङ्क widthPow2() अचर;
    शून्य widthMinFromWidth() { m_widthMin = m_width; }
    bool widthSized() अचर VL_MT_SAFE { फर्काऊ !m_widthMin || m_widthMin == m_width; }
    bool generic() अचर VL_MT_SAFE { फर्काऊ m_generic; }
    शून्य generic(bool flag) { m_generic = flag; }
    std::pair<uint32_t, uint32_t> dimensions(bool includeBasic) अचर;
    uint32_t arrayUnpackedElements() अचर;  // 1, or total multiplication of all dimensions
    // Fixed aggregate streaming properties
    bool isStreamableFixedAggregate() अचर;
    // Whether is a string, or an unpacked array or structure containing one
    bool containsString() अचर;
    bool containsUnpackedStruct() अचर;
    पूर्णाङ्क widthStream() अचर;
    string vlEnumType() अचर;  // Return VerilatedVarType: VLVT_UINT32, etc
    string vlEnumPackedAgg() अचर;  // Return VLVF_PACKED_AGG etc flags, or empty
    स्थिर पूर्णाङ्क unique_अङ्क_Inc() { फर्काऊ ++s_unique_अङ्क; }
    अचर वर्ण* charIQWN() अचर {
        फर्काऊ (isString() ? "N" : isWide() ? "W" : isDouble() ? "D" : isQuad() ? "Q" : "I");
    }
    string cType(अचर string& name, bool forFunc, bool isRef, bool packed = false) अचर;
    // Represents a C++ LiteralType? (can be constexpr)
    bool isLiteralType() अचर VL_MT_STABLE;
    // C++ type (as emitted by cType) is trivially default constructible and destructible?
    bool isTriviallyConstructible() अचर VL_MT_STABLE;
    आभास bool isDynamicallySized() अचर { फर्काऊ false; }

गुप्त:
    class CTypeRecursed;
    CTypeRecursed cTypeRecurse(bool compound, bool packed) अचर;
};
class Ast_गाँठो_Array_DType VL_NOT_FINAL : सार्वजनिक Ast_गाँठो_DType {
    // Array data type, ie "some_dtype var_name [2:0]"
    // @astgen op1 := childDTypep : Optional[AstNodeDType] // moved to refDTypep() in V3Width
    // @astgen op2 := rangep : Optional[AstRange] // array bounds
    //
    // @astgen ptr := m_refDTypep : Optional[AstNodeDType]  // Elements of this type (post-width)
    Ast_गाँठो* rangenp() अचर { फर्काऊ reinterpret_cast<Ast_गाँठो*>(rangep()); }

रक्षित:
    Ast_गाँठो_Array_DType(VNType t, File_पङ्क्ति* fl)
        : Ast_गाँठो_DType{t, fl} {}

सार्वजनिक:
    ASTGEN_MEMBERS_Ast_गाँठो_Array_DType;
    शून्य dump(std::ostream& str) अचर override;
    शून्य dumpJson(std::ostream& str) अचर override;
    शून्य dumpSmall(std::ostream& str) अचर override;
    अचर वर्ण* broken() अचर override {
        BROKEN_RTN(!((m_refDTypep && !childDTypep()) || (!m_refDTypep && childDTypep())));
        फर्काऊ nullptr;
    }
    bool same_गाँठो(अचर Ast_गाँठो* samep) अचर override {
        अचर Ast_गाँठो_Array_DType* अचर asamep = VN_DBG_AS(samep, गाँठो_Array_DType);
        फर्काऊ hi() == asamep->hi() && rangenp()->same_रूख(asamep->rangenp())
               && subDTypep() == asamep->subDTypep();
    }  // HashedDT doesn't recurse, so need to check children
    bool similar_DType_गाँठो(अचर Ast_गाँठो_DType* samep) अचर override {
        अचर Ast_गाँठो_Array_DType* अचर asamep = VN_DBG_AS(samep, गाँठो_Array_DType);
        फर्काऊ elementsConst() == asamep->elementsConst()
               && subDTypep()->similarDType(asamep->subDTypep());
    }
    Ast_गाँठो_DType* getChildDTypep() अचर override { फर्काऊ childDTypep(); }
    Ast_गाँठो_DType* subDTypep() अचर override VL_MT_STABLE {
        फर्काऊ m_refDTypep ? m_refDTypep : childDTypep();
    }
    शून्य refDTypep(Ast_गाँठो_DType* nodep) { m_refDTypep = nodep; }
    Ast_गाँठो_DType* virtRefDTypep() अचर override { फर्काऊ m_refDTypep; }
    शून्य virtRefDTypep(Ast_गाँठो_DType* nodep) override { refDTypep(nodep); }
    // METHODS
    AstBasicDType* basicp() अचर override VL_MT_STABLE {
        फर्काऊ subDTypep()->basicp();
    }  // (Slow) recurse down to find basic data type
    पूर्णाङ्क widthAlignBytes() अचर override { फर्काऊ subDTypep()->widthAlignBytes(); }
    पूर्णाङ्क widthTotalBytes() अचर override {
        फर्काऊ elementsConst() * subDTypep()->widthTotalBytes();
    }
    अन्तःस्थ पूर्णाङ्क बायाँ() अचर VL_MT_STABLE;
    अन्तःस्थ पूर्णाङ्क दायाँ() अचर VL_MT_STABLE;
    अन्तःस्थ पूर्णाङ्क hi() अचर VL_MT_STABLE;
    अन्तःस्थ पूर्णाङ्क lo() अचर VL_MT_STABLE;
    अन्तःस्थ पूर्णाङ्क elementsConst() अचर VL_MT_STABLE;
    अन्तःस्थ VNumRange declRange() अचर VL_MT_STABLE;
};
class Ast_गाँठो_UOr_Struct_DType VL_NOT_FINAL : सार्वजनिक Ast_गाँठो_DType {
    // A struct or union; common handling
    // @astgen op1 := membersp : List[AstMemberDType]
    //
    // @astgen ptr := m_classOrPackagep : Optional[AstNodeModule]  // Package emitted with
    string m_name;  // Name from upper typedef, if any
    string m_typedefName;  // Typedef's dtypeName() once moved off the typedef declaring it
    अचर पूर्णाङ्क m_unique_अङ्क;  // Unique ID distinguishing this dtype instance, for hashing/naming
    bool m_packed;  // Packed struct/union, else unpacked
    bool m_isFourstate = false;  // V3Width computes; true if any member is 4-state
    bool m_constrainedRand = false;  // True if struct has constraint expression
    bool m_emitToString = false;  // Generate to_string() for this struct/union if set

रक्षित:
    Ast_गाँठो_UOr_Struct_DType(VNType t, File_पङ्क्ति* fl, VSigning numericUnpack)
        : Ast_गाँठो_DType{t, fl}
        , m_unique_अङ्क{unique_अङ्क_Inc()} {
        // VSigning::NOSIGN overloaded to indicate not packed
        m_packed = (numericUnpack != VSigning::NOSIGN);
        numeric(VSigning::fromBool(numericUnpack.isSigned()));
    }
    Ast_गाँठो_UOr_Struct_DType(अचर Ast_गाँठो_UOr_Struct_DType& other)
        : Ast_गाँठो_DType{other}
        , m_name{other.m_name}
        , m_typedefName{other.m_typedefName}
        , m_unique_अङ्क{unique_अङ्क_Inc()}
        , m_packed{other.m_packed}
        , m_isFourstate{other.m_isFourstate} {}

सार्वजनिक:
    ASTGEN_MEMBERS_Ast_गाँठो_UOr_Struct_DType;
    पूर्णाङ्क unique_अङ्क() अचर { फर्काऊ m_unique_अङ्क; }
    शून्य dump(std::ostream& str) अचर override;
    शून्य dumpJson(std::ostream& str) अचर override;
    string prettyDTypeName(bool) अचर override;
    bool isCompound() अचर override { फर्काऊ !packed(); }
    // For basicp() we reuse the size to indicate a "fake" basic type of same size
    AstBasicDType* basicp() अचर override VL_MT_STABLE {
        यदि (!m_packed) फर्काऊ nullptr;
        फर्काऊ (isFourstate()
                    ? VN_AS(findLogicRangeDType(VNumRange{width() - 1, 0}, width(), numeric()),
                            BasicDType)
                    : VN_AS(findBitRangeDType(VNumRange{width() - 1, 0}, width(), numeric()),
                            BasicDType));
    }
    // (Slow) recurses - Structure alignment 1,2,4 or 8 bytes (arrays affect this)
    पूर्णाङ्क widthAlignBytes() अचर override;
    // (Slow) recurses - Width in bytes rounding up 1,2,4,8,12,...
    पूर्णाङ्क widthTotalBytes() अचर override;
    bool similar_DType_गाँठो(अचर Ast_गाँठो_DType* samep) अचर override;
    string name() अचर override VL_MT_STABLE { फर्काऊ m_name; }
    शून्य name(अचर string& flag) override { m_name = flag; }
    // Keep the name the typedef declaring it gives, as it moves off it to the type table
    शून्य typedefName(अचर string& name) { m_typedefName = name; }
    // Name of the C structure for DPI, empty if no typedef names it, so declared where used
    string dpiName() अचर;
    // C typedef of the unpacked structure for DPI (IEEE 1800-2023 H.7.3)
    string dpiTypedef() अचर;
    bool packed() अचर VL_MT_SAFE { फर्काऊ m_packed; }
    शून्य packed(bool flag) { m_packed = flag; }
    // packed() but as don't support unpacked, presently all structs
    स्थिर bool packedUnsup() { फर्काऊ true; }
    शून्य isFourstate(bool flag) { m_isFourstate = flag; }
    bool isFourstate() अचर override VL_MT_SAFE { फर्काऊ m_isFourstate; }
    स्थिर पूर्णाङ्क lo() VL_MT_STABLE { फर्काऊ 0; }
    पूर्णाङ्क hi() अचर VL_MT_STABLE {
        फर्काऊ dtypep()->width() - 1;
    }  // Packed classes look like arrays
    VNumRange declRange() अचर VL_MT_STABLE { फर्काऊ VNumRange{hi(), lo()}; }
    Ast_गाँठो_Module* classOrPackagep() अचर { फर्काऊ m_classOrPackagep; }
    शून्य classOrPackagep(Ast_गाँठो_Module* classpackagep) { m_classOrPackagep = classpackagep; }
    bool isConstrainedRand() अचर { फर्काऊ m_constrainedRand; }
    शून्य markConstrainedRand(bool flag) { m_constrainedRand = flag; }
    bool emitToString() अचर { फर्काऊ m_emitToString; }
    शून्य setEmitToString() { m_emitToString = true; }
    bool isAggregateType() अचर override { फर्काऊ !packed(); }
};

// === Concrete node types =====================================================

// === AstNode ===
class AstEnumItem final : सार्वजनिक Ast_गाँठो {
    // @astgen op1 := rangep : Optional[AstRange] // Range for name appending
    // @astgen op2 := valuep : Optional[AstNodeExpr]
    string m_name;  // Name of item

सार्वजनिक:
    // Parents: ENUM
    AstEnumItem(File_पङ्क्ति* fl, अचर string& name, AstRange* rangep, Ast_गाँठो_अभिव्यक्ति* valuep)
        : ASTGEN_SUPER_EnumItem(fl)
        , m_name{name} {
        this->rangep(rangep);
        this->valuep(valuep);
    }
    ASTGEN_MEMBERS_AstEnumItem;
    string name() अचर override VL_MT_STABLE { फर्काऊ m_name; }
    bool maybePointedTo() अचर override VL_MT_SAFE { फर्काऊ true; }
    bool hasDType() अचर override VL_MT_SAFE { फर्काऊ true; }
    शून्य name(अचर string& flag) override { m_name = flag; }
};

// === AstNodeDType ===
class AstAssocArrayDType final : सार्वजनिक Ast_गाँठो_DType {
    // Associative array data type, ie "[some_dtype]"
    // @astgen op1 := childDTypep : Optional[AstNodeDType] // moved to refDTypep() in V3Width
    // @astgen op2 := keyChildDTypep : Optional[AstNodeDType]
    //
    // @astgen ptr := m_refDTypep : Optional[AstNodeDType]  // Elements of this type (post-width)
    // @astgen ptr := m_keyDTypep : Optional[AstNodeDType]  // Keys of this type (post-width)
सार्वजनिक:
    AstAssocArrayDType(File_पङ्क्ति* fl, VFlagChildDType, Ast_गाँठो_DType* dtp, Ast_गाँठो_DType* साँचो_Dtp)
        : ASTGEN_SUPER_AssocArrayDType(fl) {
        childDTypep(dtp);  // Only for parser
        साँचो_Child_DTypep(साँचो_Dtp);  // Only for parser
        refDTypep(nullptr);
        साँचो_DTypep(nullptr);
        dtypep(nullptr);  // V3Width will resolve
    }
    AstAssocArrayDType(File_पङ्क्ति* fl, Ast_गाँठो_DType* dtp, Ast_गाँठो_DType* साँचो_Dtp)
        : ASTGEN_SUPER_AssocArrayDType(fl) {
        refDTypep(dtp);
        साँचो_DTypep(साँचो_Dtp);
        dtypep(dtp);
    }
    ASTGEN_MEMBERS_AstAssocArrayDType;
    अचर वर्ण* broken() अचर override {
        BROKEN_RTN(!((m_refDTypep && !childDTypep()) || (!m_refDTypep && childDTypep())));
        BROKEN_RTN(!((m_साँचो_DTypep && !childDTypep()) || (!m_साँचो_DTypep && childDTypep())));
        फर्काऊ nullptr;
    }
    bool same_गाँठो(अचर Ast_गाँठो* samep) अचर override {
        अचर AstAssocArrayDType* अचर asamep = VN_DBG_AS(samep, AssocArrayDType);
        यदि (!asamep->subDTypep()) फर्काऊ false;
        यदि (!asamep->साँचो_DTypep()) फर्काऊ false;
        फर्काऊ (subDTypep() == asamep->subDTypep() && साँचो_DTypep() == asamep->साँचो_DTypep());
    }
    bool similar_DType_गाँठो(अचर Ast_गाँठो_DType* samep) अचर override {
        अचर AstAssocArrayDType* अचर asamep = VN_DBG_AS(samep, AssocArrayDType);
        फर्काऊ asamep->subDTypep() && subDTypep()->similarDType(asamep->subDTypep())
               && asamep->साँचो_DTypep() && साँचो_DTypep()->similarDType(asamep->साँचो_DTypep());
    }
    string prettyDTypeName(bool full) अचर override;
    शून्य dumpSmall(std::ostream& str) अचर override;
    Ast_गाँठो_DType* getChildDTypep() अचर override { फर्काऊ childDTypep(); }
    Ast_गाँठो_DType* getChild2DTypep() अचर override { फर्काऊ साँचो_Child_DTypep(); }
    Ast_गाँठो_DType* subDTypep() अचर override VL_MT_STABLE {
        फर्काऊ m_refDTypep ? m_refDTypep : childDTypep();
    }
    Ast_गाँठो_DType* subDType2p() अचर override VL_MT_STABLE {
        फर्काऊ m_साँचो_DTypep ? m_साँचो_DTypep : साँचो_Child_DTypep();
    }
    bool isAggregateType() अचर override { फर्काऊ true; }
    शून्य refDTypep(Ast_गाँठो_DType* nodep) { m_refDTypep = nodep; }
    Ast_गाँठो_DType* virtRefDTypep() अचर override { फर्काऊ m_refDTypep; }
    शून्य virtRefDTypep(Ast_गाँठो_DType* nodep) override { refDTypep(nodep); }
    Ast_गाँठो_DType* virtRefDType2p() अचर override { फर्काऊ m_साँचो_DTypep; }
    शून्य virtRefDType2p(Ast_गाँठो_DType* nodep) override { साँचो_DTypep(nodep); }
    //
    Ast_गाँठो_DType* साँचो_DTypep() अचर VL_MT_STABLE {
        फर्काऊ m_साँचो_DTypep ? m_साँचो_DTypep : साँचो_Child_DTypep();
    }
    शून्य साँचो_DTypep(Ast_गाँठो_DType* nodep) { m_साँचो_DTypep = nodep; }
    // METHODS
    AstBasicDType* basicp() अचर override VL_MT_STABLE { फर्काऊ nullptr; }
    पूर्णाङ्क widthAlignBytes() अचर override { फर्काऊ subDTypep()->widthAlignBytes(); }
    पूर्णाङ्क widthTotalBytes() अचर override { फर्काऊ subDTypep()->widthTotalBytes(); }
    bool isCompound() अचर override { फर्काऊ true; }
    bool isDynamicallySized() अचर override { फर्काऊ true; }
};
class AstBasicDType final : सार्वजनिक Ast_गाँठो_DType {
    // Builtin atomic/vectored data type
    // @astgen op1 := rangep : Optional[AstRange] // Range of variable
    संरचना Members final {
        VBasicDTypeKwd m_keyword;  // (also in VBasicTypeKey) What keyword created basic type
        // dist-ast-dump-suppress  // Part of dumpSmall
        VNumRange m_nrange;  // (also in VBasicTypeKey) Numeric msb/lsb (if non-opaque keyword)
        bool चालक==(अचर Members& rhs) अचर {
            फर्काऊ rhs.m_keyword == m_keyword && rhs.m_nrange == m_nrange;
        }
    } m;
    // See also in AstNodeDType: m_width, m_widthMin, m_numeric(issigned)
सार्वजनिक:
    AstBasicDType(File_पङ्क्ति* fl, VBasicDTypeKwd kwd, अचर VSigning& signst = VSigning::NOSIGN)
        : ASTGEN_SUPER_BasicDType(fl) {
        init(kwd, signst, 0, -1, nullptr);
    }
    AstBasicDType(File_पङ्क्ति* fl, VFlagLogicPacked, पूर्णाङ्क wantwidth)
        : ASTGEN_SUPER_BasicDType(fl) {
        init(VBasicDTypeKwd::LOGIC, VSigning::NOSIGN, wantwidth, -1, nullptr);
    }
    AstBasicDType(File_पङ्क्ति* fl, VFlagLogic2StatePacked, पूर्णाङ्क wantwidth)
        : ASTGEN_SUPER_BasicDType(fl) {
        init(VBasicDTypeKwd::LOGIC2STATE, VSigning::NOSIGN, wantwidth, -1, nullptr);
    }
    AstBasicDType(File_पङ्क्ति* fl, VFlagBitPacked, पूर्णाङ्क wantwidth)
        : ASTGEN_SUPER_BasicDType(fl) {
        init(VBasicDTypeKwd::BIT, VSigning::NOSIGN, wantwidth, -1, nullptr);
    }
    AstBasicDType(File_पङ्क्ति* fl, VBasicDTypeKwd kwd, VSigning numer, पूर्णाङ्क wantwidth, पूर्णाङ्क widthmin)
        : ASTGEN_SUPER_BasicDType(fl) {
        init(kwd, numer, wantwidth, widthmin, nullptr);
    }
    AstBasicDType(File_पङ्क्ति* fl, VBasicDTypeKwd kwd, VSigning numer, VNumRange range, पूर्णाङ्क widthmin)
        : ASTGEN_SUPER_BasicDType(fl) {
        init(kwd, numer, range.elements(), widthmin, nullptr);
        m.m_nrange = range;  // as init() presumes lsb==0, but range.lsb() might not be
    }
    // See also addRange in verilog.y
गुप्त:
    शून्य init(VBasicDTypeKwd kwd, VSigning numer, पूर्णाङ्क wantwidth, पूर्णाङ्क wantwidthmin,
              AstRange* rangep);

सार्वजनिक:
    ASTGEN_MEMBERS_AstBasicDType;
    शून्य dump(std::ostream& str) अचर override;
    शून्य dumpJson(std::ostream& str) अचर override;
    // width/widthMin/numeric compared elsewhere
    bool same_गाँठो(अचर Ast_गाँठो* samep) अचर override;
    bool similar_DType_गाँठो(अचर Ast_गाँठो_DType* samep) अचर override;
    string name() अचर override VL_MT_STABLE { फर्काऊ m.m_keyword.typeName(); }
    string prettyDTypeName(bool full) अचर override;
    अचर वर्ण* broken() अचर override {
        BROKEN_RTN(dtypep() != this);
        BROKEN_RTN(v3Global.widthMinUsage() == VWidthMinUsage::VERILOG_WIDTH
                   && widthMin() > width());
        फर्काऊ nullptr;
    }
    शून्य setSignedState(अचर VSigning& signst) {
        // Note NOSIGN does NOT change the state; this is required by the parser
        यदि (signst == VSigning::UNSIGNED) {
            numeric(signst);
        } अन्यथा यदि (signst == VSigning::SIGNED) {
            numeric(signst);
        }
    }
    // METHODS
    AstBasicDType* basicp() अचर override VL_MT_STABLE { फर्काऊ (AstBasicDType*)this; }
    // (Slow) recurses - Structure alignment 1,2,4 or 8 bytes (arrays affect this)
    पूर्णाङ्क widthAlignBytes() अचर override;
    // (Slow) recurses - Width in bytes rounding up 1,2,4,8,12,...
    पूर्णाङ्क widthTotalBytes() अचर override;
    bool isFourstate() अचर override { फर्काऊ keyword().isFourstate(); }
    VBasicDTypeKwd keyword() अचर VL_MT_SAFE {  // Avoid using - use isSomething accessors instead
        फर्काऊ m.m_keyword;
    }
    bool isBitLogic() अचर { फर्काऊ keyword().isBitLogic(); }
    bool isBit() अचर { फर्काऊ keyword().isBit(); }
    bool isCHandle() अचर VL_MT_STABLE { फर्काऊ keyword().isCHandle(); }
    bool isDouble() अचर VL_MT_STABLE { फर्काऊ keyword().isDouble(); }
    bool isEvent() अचर VL_MT_STABLE { फर्काऊ keyword() == VBasicDTypeKwd::EVENT; }
    bool isForkSync() अचर VL_MT_SAFE { फर्काऊ keyword() == VBasicDTypeKwd::FORK_SYNC; }
    bool isProcessRef() अचर VL_MT_SAFE { फर्काऊ keyword() == VBasicDTypeKwd::PROCESS_REFERENCE; }
    bool isDelayScheduler() अचर VL_MT_SAFE {
        फर्काऊ keyword() == VBasicDTypeKwd::DELAY_SCHEDULER;
    }
    bool isTriggerScheduler() अचर VL_MT_SAFE {
        फर्काऊ keyword() == VBasicDTypeKwd::TRIGGER_SCHEDULER;
    }
    bool isDynamicTriggerScheduler() अचर VL_MT_SAFE {
        फर्काऊ keyword() == VBasicDTypeKwd::DYNAMIC_TRIGGER_SCHEDULER;
    }
    bool isRandomGenerator() अचर VL_MT_SAFE {
        फर्काऊ keyword() == VBasicDTypeKwd::RANDOM_GENERATOR;
    }
    bool isStdRandomGenerator() अचर VL_MT_SAFE {
        फर्काऊ keyword() == VBasicDTypeKwd::RANDOM_STDGENERATOR;
    }
    bool isCovergroupInstHandle() अचर VL_MT_SAFE {
        फर्काऊ keyword() == VBasicDTypeKwd::COVERGROUP_INSTHANDLE;
    }
    bool isOpaque() अचर VL_MT_SAFE { फर्काऊ keyword().isOpaque(); }
    bool isString() अचर VL_MT_STABLE { फर्काऊ keyword().isString(); }
    bool isZeroInit() अचर { फर्काऊ keyword().isZeroInit(); }
    bool isRanged() अचर { फर्काऊ rangep() || m.m_nrange.ranged(); }
    bool isDpiBitVec() अचर {  // DPI uses svBitVecVal
        फर्काऊ keyword() == VBasicDTypeKwd::BIT && isRanged();
    }
    bool isDpiLogicVec() अचर {  // DPI uses svLogicVecVal
        फर्काऊ keyword().isFourstate() && !(keyword() == VBasicDTypeKwd::LOGIC && !isRanged());
    }
    bool isDpiCLayout() अचर {
        फर्काऊ keyword().isDpiCLayout()
               || (keyword().isSameish(VBasicDTypeKwd::LOGIC) && !isRanged());
    }
    bool isDpiPrimitive() अचर {  // DPI uses a primitive type
        फर्काऊ !isDpiBitVec() && !isDpiLogicVec();
    }
    // Generally the lo/hi/left/right funcs should be used instead of nrange()
    अचर VNumRange& nrange() अचर { फर्काऊ m.m_nrange; }
    अन्तःस्थ पूर्णाङ्क hi() अचर;
    अन्तःस्थ पूर्णाङ्क lo() अचर;
    अन्तःस्थ पूर्णाङ्क elements() अचर;
    पूर्णाङ्क बायाँ() अचर { फर्काऊ ascending() ? lo() : hi(); }  // How to show a declaration
    पूर्णाङ्क दायाँ() अचर { फर्काऊ ascending() ? hi() : lo(); }
    अन्तःस्थ bool ascending() अचर;
    bool implicit() अचर { फर्काऊ keyword() == VBasicDTypeKwd::LOGIC_IMPLICIT; }
    bool untyped() अचर { फर्काऊ keyword() == VBasicDTypeKwd::UNTYPED; }
    VNumRange declRange() अचर { फर्काऊ isRanged() ? VNumRange{बायाँ(), दायाँ()} : VNumRange{}; }
    शून्य cvtRangeConst();  // Convert to smaller representation
    bool isCompound() अचर override { फर्काऊ isString(); }
    bool isIntegralOrPacked() अचर override { फर्काऊ keyword().isIntNumeric(); }
};
class AstBracketArrayDType final : सार्वजनिक Ast_गाँठो_DType {
    // Associative/Queue/Normal array data type, ie "[dtype_or_expr]"
    // only for early parsing then becomes another data type
    // @astgen op1 := childDTypep : Optional[AstNodeDType] // moved to refDTypep() in V3Width
    // @astgen op2 := elementsp : AstNode  // Number of elements in array
सार्वजनिक:
    AstBracketArrayDType(File_पङ्क्ति* fl, VFlagChildDType, Ast_गाँठो_DType* childDTypep,
                         Ast_गाँठो* elementsp)
        : ASTGEN_SUPER_BracketArrayDType(fl) {
        this->childDTypep(childDTypep);
        this->elementsp(elementsp);
    }
    ASTGEN_MEMBERS_AstBracketArrayDType;
    bool similar_DType_गाँठो(अचर Ast_गाँठो_DType* samep) अचर override { फर्काऊ this == samep; }
    Ast_गाँठो_DType* subDTypep() अचर override VL_MT_STABLE { फर्काऊ childDTypep(); }
    // METHODS
    // Will be removed in V3Width, which relies on this
    // being a child not a dtype pointed node
    bool maybePointedTo() अचर override VL_MT_SAFE { फर्काऊ false; }
    AstBasicDType* basicp() अचर override VL_MT_STABLE { फर्काऊ nullptr; }
    पूर्णाङ्क widthAlignBytes() अचर override { V3ERROR_NA_RETURN(0); }
    पूर्णाङ्क widthTotalBytes() अचर override { V3ERROR_NA_RETURN(0); }
    bool isCompound() अचर override { फर्काऊ true; }
};
class AstCDType final : सार्वजनिक Ast_गाँठो_DType {
    // Raw "C" data type passed directly to output
    string m_name;  // Name of data type, printed when do V3EmitC
सार्वजनिक:
    AstCDType(File_पङ्क्ति* fl, अचर string& name)
        : ASTGEN_SUPER_CDType(fl)
        , m_name{name} {
        dtypep(this);
    }

    ASTGEN_MEMBERS_AstCDType;
    bool same_गाँठो(अचर Ast_गाँठो* samep) अचर override {
        अचर AstCDType* अचर asamep = VN_DBG_AS(samep, CDType);
        फर्काऊ m_name == asamep->m_name;
    }
    bool similar_DType_गाँठो(अचर Ast_गाँठो_DType* samep) अचर override { फर्काऊ same_गाँठो(samep); }
    string name() अचर override VL_MT_STABLE { फर्काऊ m_name; }
    string prettyDTypeName(bool) अचर override { फर्काऊ m_name; }
    // METHODS
    AstBasicDType* basicp() अचर override VL_MT_STABLE { फर्काऊ nullptr; }
    पूर्णाङ्क widthAlignBytes() अचर override { फर्काऊ 8; }  // Assume
    पूर्णाङ्क widthTotalBytes() अचर override { फर्काऊ 8; }  // Assume
    bool isCompound() अचर override { फर्काऊ true; }
    // Return the global runtime type storing the requested number of bits.
    स्थिर string typeToHold(पूर्णाङ्क width) {
        यदि (width <= 8) फर्काऊ "::CData";
        यदि (width <= 16) फर्काऊ "::SData";
        यदि (width <= VL_IDATASIZE) फर्काऊ "::IData";
        यदि (width <= VL_QUADSIZE) फर्काऊ "::QData";

        फर्काऊ "::VlWide<" + std::to_string(VL_शब्दहरू_I(width)) + ">";
    }
};
class AstClassRefDType final : सार्वजनिक Ast_गाँठो_DType {
    // Reference to a class
    // @astgen op1 := paramsp: List[AstPin]
    //
    // @astgen ptr := m_classp : Optional[AstClass]  // data type pointed to, BELOW the AstTypedef
    // @astgen ptr := m_classOrPackagep : Optional[AstNodeModule]  // Package hierarchy
    bool m_rawPointer = false;  // Emit as a non-owning C++ pointer rather than VlClassRef
सार्वजनिक:
    AstClassRefDType(File_पङ्क्ति* fl, AstClass* classp, AstPin* paramsp)
        : ASTGEN_SUPER_ClassRefDType(fl)
        , m_classp{classp} {
        dtypep(this);
        addParamsp(paramsp);
    }
    ASTGEN_MEMBERS_AstClassRefDType;
    // METHODS
    bool same_गाँठो(अचर Ast_गाँठो* samep) अचर override {
        अचर AstClassRefDType* अचर asamep = VN_DBG_AS(samep, ClassRefDType);
        फर्काऊ (m_classp == asamep->m_classp && m_classOrPackagep == asamep->m_classOrPackagep
                && m_rawPointer == asamep->m_rawPointer);
    }
    bool similar_DType_गाँठो(अचर Ast_गाँठो_DType* samep) अचर override;
    शून्य dump(std::ostream& str = std::cout) अचर override;
    शून्य dumpJson(std::ostream& str = std::cout) अचर override;
    शून्य dumpSmall(std::ostream& str) अचर override;
    string prettyDTypeName(bool full) अचर override;
    string pretty_Name_सन्देश() अचर override;
    string name() अचर override VL_MT_STABLE;
    AstBasicDType* basicp() अचर override VL_MT_STABLE { फर्काऊ nullptr; }
    पूर्णाङ्क widthAlignBytes() अचर override { फर्काऊ 0; }
    पूर्णाङ्क widthTotalBytes() अचर override { फर्काऊ 0; }
    Ast_गाँठो_DType* virtRefDTypep() अचर override { फर्काऊ nullptr; }
    शून्य virtRefDTypep(Ast_गाँठो_DType* nodep) override {}
    Ast_गाँठो_DType* subDTypep() अचर override VL_MT_STABLE { फर्काऊ nullptr; }
    Ast_गाँठो_Module* classOrPackagep() अचर { फर्काऊ m_classOrPackagep; }
    शून्य classOrPackagep(Ast_गाँठो_Module* nodep) { m_classOrPackagep = nodep; }
    AstClass* classp() अचर VL_MT_STABLE { फर्काऊ m_classp; }
    शून्य classp(AstClass* nodep) { m_classp = nodep; }
    bool rawPointer() अचर { फर्काऊ m_rawPointer; }
    शून्य rawPointer(bool flag) { m_rawPointer = flag; }
    स्थिर शून्य selfTest();
    bool isCompound() अचर override { फर्काऊ true; }
};
class AstConstDType final : सार्वजनिक Ast_गाँठो_DType {
    // const data type, ie "const some_dtype var_name [2:0]"
    // ConstDType are removed in V3LinkLValue and become AstVar::isConst.
    // When more generic types are supported AstConstDType will be propagated further.
    // @astgen op1 := childDTypep : Optional[AstNodeDType]
    //
    // @astgen ptr := m_refDTypep : Optional[AstNodeDType]  // Inherit from this base data type
सार्वजनिक:
    AstConstDType(File_पङ्क्ति* fl, VFlagChildDType, Ast_गाँठो_DType* dtp)
        : ASTGEN_SUPER_ConstDType(fl) {
        childDTypep(dtp);  // Only for parser
        refDTypep(nullptr);  // V3Width will resolve
        dtypep(nullptr);  // V3Width will resolve
        widthFromSub(subDTypep());
    }
    ASTGEN_MEMBERS_AstConstDType;
    अचर वर्ण* broken() अचर override {
        BROKEN_RTN(!((m_refDTypep && !childDTypep()) || (!m_refDTypep && childDTypep())));
        फर्काऊ nullptr;
    }
    bool same_गाँठो(अचर Ast_गाँठो* samep) अचर override {
        अचर AstConstDType* अचर sp = VN_DBG_AS(samep, ConstDType);
        फर्काऊ (m_refDTypep == sp->m_refDTypep);
    }
    bool similar_DType_गाँठो(अचर Ast_गाँठो_DType* samep) अचर override {
        फर्काऊ skipRefp()->similarDType(samep->skipRefp());
    }
    Ast_गाँठो_DType* getChildDTypep() अचर override { फर्काऊ childDTypep(); }
    Ast_गाँठो_DType* subDTypep() अचर override VL_MT_STABLE {
        फर्काऊ m_refDTypep ? m_refDTypep : childDTypep();
    }
    शून्य refDTypep(Ast_गाँठो_DType* nodep) { m_refDTypep = nodep; }
    Ast_गाँठो_DType* virtRefDTypep() अचर override { फर्काऊ m_refDTypep; }
    शून्य virtRefDTypep(Ast_गाँठो_DType* nodep) override { refDTypep(nodep); }
    // METHODS
    AstBasicDType* basicp() अचर override VL_MT_STABLE { फर्काऊ subDTypep()->basicp(); }
    पूर्णाङ्क widthAlignBytes() अचर override { फर्काऊ subDTypep()->widthAlignBytes(); }
    पूर्णाङ्क widthTotalBytes() अचर override { फर्काऊ subDTypep()->widthTotalBytes(); }
    bool isCompound() अचर override {
        v3fatalSrc("call isCompound on subdata type, not reference");
        फर्काऊ false;
    }
};
class AstConstraintRefDType final : सार्वजनिक Ast_गाँठो_DType {
    // For e.g. a reference to constraint for constraint_mode
सार्वजनिक:
    explicit AstConstraintRefDType(File_पङ्क्ति* fl)
        : ASTGEN_SUPER_ConstraintRefDType(fl) {
        dtypep(this);
    }
    ASTGEN_MEMBERS_AstConstraintRefDType;
    bool hasDType() अचर override VL_MT_SAFE { फर्काऊ true; }
    bool maybePointedTo() अचर override VL_MT_SAFE { फर्काऊ true; }
    bool undead() अचर override { फर्काऊ true; }
    Ast_गाँठो_DType* subDTypep() अचर override VL_MT_STABLE { फर्काऊ nullptr; }
    Ast_गाँठो_DType* virtRefDTypep() अचर override { फर्काऊ nullptr; }
    शून्य virtRefDTypep(Ast_गाँठो_DType* nodep) override {}
    bool similar_DType_गाँठो(अचर Ast_गाँठो_DType* samep) अचर override { फर्काऊ this == samep; }
    AstBasicDType* basicp() अचर override VL_MT_STABLE { फर्काऊ nullptr; }
    पूर्णाङ्क widthAlignBytes() अचर override { फर्काऊ 1; }
    पूर्णाङ्क widthTotalBytes() अचर override { फर्काऊ 1; }
    bool isCompound() अचर override { फर्काऊ false; }
};
class AstCoverCrossDType final : सार्वजनिक Ast_गाँठो_DType {
    // Borrowed pointer to VlCoverCrossT<...> or construction-time VlCoverCrossDyn.
    अचर uint32_t m_dimensions;
    अचर uint32_t m_tuples;  // Fixed capacity number of cross tuples (VlCoverCrossT's Tuples)
    अचर uint32_t m_bins;  // Fixed capacity number of explicit bins (VlCoverCrossT's Bins)
    अचर uint32_t m_autoBins;  // Fixed capacity number of auto bins (VlCoverCrossT's AutoBins)
    अचर uint64_t m_bin_शब्दहरू;  // Fixed capacity selection words (VlCoverCrossT's BinWords)
    अचर bool m_dynamic;  // Bin layout is determined at covergroup construction

सार्वजनिक:
    AstCoverCrossDType(File_पङ्क्ति* fl, uint32_t dimensions, uint32_t tuples, uint32_t bins,
                       uint32_t autoBins, uint64_t bin_शब्दहरू, bool dynamic = false)
        : ASTGEN_SUPER_CoverCrossDType(fl)
        , m_dimensions{dimensions}
        , m_tuples{tuples}
        , m_bins{bins}
        , m_autoBins{autoBins}
        , m_bin_शब्दहरू{bin_शब्दहरू}
        , m_dynamic{dynamic} {
        dtypep(this);
    }
    ASTGEN_MEMBERS_AstCoverCrossDType;
    अचर वर्ण* broken() अचर override {
        BROKEN_RTN(m_dimensions == 0);
        फर्काऊ nullptr;
    }
    bool same_गाँठो(अचर Ast_गाँठो* samep) अचर override {  // LCOV_EXCL_START
        अचर AstCoverCrossDType* अचर sp = VN_DBG_AS(samep, CoverCrossDType);
        फर्काऊ dimensions() == sp->dimensions() && tuples() == sp->tuples() && bins() == sp->bins()
               && autoBins() == sp->autoBins() && bin_शब्दहरू() == sp->bin_शब्दहरू()
               && isDynamic() == sp->isDynamic();
    }  // LCOV_EXCL_STOP
    bool similar_DType_गाँठो(अचर Ast_गाँठो_DType* samep) अचर override { फर्काऊ this == samep; }
    शून्य dump(std::ostream& str) अचर override;
    शून्य dumpJson(std::ostream& str) अचर override;
    शून्य dumpSmall(std::ostream& str) अचर override;
    uint32_t dimensions() अचर { फर्काऊ m_dimensions; }
    uint32_t tuples() अचर { फर्काऊ m_tuples; }
    uint32_t bins() अचर { फर्काऊ m_bins; }
    uint32_t autoBins() अचर { फर्काऊ m_autoBins; }
    uint64_t bin_शब्दहरू() अचर { फर्काऊ m_bin_शब्दहरू; }
    bool isDynamic() अचर { फर्काऊ m_dynamic; }
    string cppTemplateArgs() अचर;
    AstBasicDType* basicp() अचर override VL_MT_STABLE { फर्काऊ nullptr; }
    पूर्णाङ्क widthAlignBytes() अचर override { फर्काऊ परिमाण(शून्य*); }
    पूर्णाङ्क widthTotalBytes() अचर override { फर्काऊ परिमाण(शून्य*); }
    bool isCompound() अचर override { फर्काऊ true; }
};
class AstCoverpointDType final : सार्वजनिक Ast_गाँठो_DType {
    // Borrowed pointer to a covergroup coverpoint runtime, 'VlCoverpointT<hitBound>*'.
    // Follows pattern of AstQueueDType in capturing the compile-time max bin overlap
    // template argument.
    uint32_t m_hitBound;  // VlCoverpointT<> template argument; hit list size, >= 1
सार्वजनिक:
    AstCoverpointDType(File_पङ्क्ति* fl, uint32_t hitBound)
        : ASTGEN_SUPER_CoverpointDType(fl)
        , m_hitBound{hitBound} {
        dtypep(this);
    }
    ASTGEN_MEMBERS_AstCoverpointDType;
    अचर वर्ण* broken() अचर override {
        BROKEN_RTN(m_hitBound < 1);
        फर्काऊ nullptr;
    }
    // V3Covergroup interns these one-per-hitBound into the type table, so identity is
    // equality; there is never a second node with the same bound to compare against.
    bool similar_DType_गाँठो(अचर Ast_गाँठो_DType* samep) अचर override { फर्काऊ this == samep; }
    शून्य dumpSmall(std::ostream& str) अचर override;
    // ACCESSORS
    uint32_t hitBound() अचर { फर्काऊ m_hitBound; }
    // METHODS
    AstBasicDType* basicp() अचर override VL_MT_STABLE { फर्काऊ nullptr; }
    पूर्णाङ्क widthAlignBytes() अचर override { फर्काऊ परिमाण(शून्य*); }
    पूर्णाङ्क widthTotalBytes() अचर override { फर्काऊ परिमाण(शून्य*); }
    bool isCompound() अचर override { फर्काऊ true; }
};
class AstDefImplicitDType final : सार्वजनिक Ast_गाँठो_DType {
    // For parsing enum/struct/unions that are declared with a variable rather than typedef
    // This allows "var enum {...} a,b" to share the enum definition for both variables
    // After link, these become typedefs
    // @astgen op1 := childDTypep : Optional[AstNodeDType]
    string m_name;  // Data type name
    अचर पूर्णाङ्क m_unique_अङ्क;  // Unique ID distinguishing this dtype instance, for hashing/naming

सार्वजनिक:
    AstDefImplicitDType(File_पङ्क्ति* fl, अचर string& name, VFlagChildDType, Ast_गाँठो_DType* dtp)
        : ASTGEN_SUPER_DefImplicitDType(fl)
        , m_name{name}
        , m_unique_अङ्क{unique_अङ्क_Inc()} {
        childDTypep(dtp);  // Only for parser
        dtypep(nullptr);  // V3Width will resolve
    }
    AstDefImplicitDType(अचर AstDefImplicitDType& other)
        : Ast_गाँठो_DType(other)
        , m_name{other.m_name}
        , m_unique_अङ्क{unique_अङ्क_Inc()} {}
    ASTGEN_MEMBERS_AstDefImplicitDType;
    पूर्णाङ्क unique_अङ्क() अचर { फर्काऊ m_unique_अङ्क; }
    bool same_गाँठो(अचर Ast_गाँठो* samep) अचर override {
        अचर AstDefImplicitDType* अचर sp = VN_DBG_AS(samep, DefImplicitDType);
        फर्काऊ unique_अङ्क() == sp->unique_अङ्क();
    }
    bool similar_DType_गाँठो(अचर Ast_गाँठो_DType* samep) अचर override { फर्काऊ same_गाँठो(samep); }
    Ast_गाँठो_DType* getChildDTypep() अचर override { फर्काऊ childDTypep(); }
    Ast_गाँठो_DType* subDTypep() अचर override VL_MT_STABLE {
        फर्काऊ dtypep() ? dtypep() : childDTypep();
    }
    // METHODS
    // op1 = Range of variable
    Ast_गाँठो_DType* dtypeSkipRefp() अचर { फर्काऊ dtypep()->skipRefp(); }
    AstBasicDType* basicp() अचर override VL_MT_STABLE { फर्काऊ subDTypep()->basicp(); }
    पूर्णाङ्क widthAlignBytes() अचर override { फर्काऊ dtypep()->widthAlignBytes(); }
    पूर्णाङ्क widthTotalBytes() अचर override { फर्काऊ dtypep()->widthTotalBytes(); }
    string name() अचर override VL_MT_STABLE { फर्काऊ m_name; }
    शून्य name(अचर string& flag) override { m_name = flag; }
    bool isCompound() अचर override { फर्काऊ false; }
};
class AstDynArrayDType final : सार्वजनिक Ast_गाँठो_DType {
    // Dynamic array data type, ie "[]"
    // @astgen op1 := childDTypep : Optional[AstNodeDType] // moved to refDTypep() in V3Width
    //
    // @astgen ptr := m_refDTypep : Optional[AstNodeDType]  // Elements of this type (post-width)
सार्वजनिक:
    AstDynArrayDType(File_पङ्क्ति* fl, VFlagChildDType, Ast_गाँठो_DType* dtp)
        : ASTGEN_SUPER_DynArrayDType(fl) {
        childDTypep(dtp);  // Only for parser
        refDTypep(nullptr);
        dtypep(nullptr);  // V3Width will resolve
    }
    AstDynArrayDType(File_पङ्क्ति* fl, Ast_गाँठो_DType* dtp)
        : ASTGEN_SUPER_DynArrayDType(fl) {
        refDTypep(dtp);
        dtypep(nullptr);  // V3Width will resolve
    }
    ASTGEN_MEMBERS_AstDynArrayDType;
    अचर वर्ण* broken() अचर override {
        BROKEN_RTN(!((m_refDTypep && !childDTypep()) || (!m_refDTypep && childDTypep())));
        फर्काऊ nullptr;
    }
    bool same_गाँठो(अचर Ast_गाँठो* samep) अचर override {
        अचर AstDynArrayDType* अचर asamep = VN_DBG_AS(samep, DynArrayDType);
        यदि (!asamep->subDTypep()) फर्काऊ false;
        फर्काऊ subDTypep() == asamep->subDTypep();
    }
    bool similar_DType_गाँठो(अचर Ast_गाँठो_DType* samep) अचर override {
        अचर AstDynArrayDType* अचर asamep = VN_DBG_AS(samep, DynArrayDType);
        फर्काऊ asamep->subDTypep() && subDTypep()->similarDType(asamep->subDTypep());
    }
    string prettyDTypeName(bool full) अचर override;
    शून्य dumpSmall(std::ostream& str) अचर override;
    Ast_गाँठो_DType* getChildDTypep() अचर override { फर्काऊ childDTypep(); }
    Ast_गाँठो_DType* subDTypep() अचर override VL_MT_STABLE {
        फर्काऊ m_refDTypep ? m_refDTypep : childDTypep();
    }
    bool isAggregateType() अचर override { फर्काऊ true; }
    शून्य refDTypep(Ast_गाँठो_DType* nodep) { m_refDTypep = nodep; }
    Ast_गाँठो_DType* virtRefDTypep() अचर override { फर्काऊ m_refDTypep; }
    शून्य virtRefDTypep(Ast_गाँठो_DType* nodep) override { refDTypep(nodep); }
    // METHODS
    AstBasicDType* basicp() अचर override VL_MT_STABLE { फर्काऊ nullptr; }
    पूर्णाङ्क widthAlignBytes() अचर override { फर्काऊ subDTypep()->widthAlignBytes(); }
    पूर्णाङ्क widthTotalBytes() अचर override { फर्काऊ subDTypep()->widthTotalBytes(); }
    bool isCompound() अचर override { फर्काऊ true; }
    bool isDynamicallySized() अचर override { फर्काऊ true; }
};
class AstEmptyQueueDType final : सार्वजनिक Ast_गाँठो_DType {
    // For EmptyQueue
सार्वजनिक:
    explicit AstEmptyQueueDType(File_पङ्क्ति* fl)
        : ASTGEN_SUPER_EmptyQueueDType(fl) {
        dtypep(this);
    }
    ASTGEN_MEMBERS_AstEmptyQueueDType;
    शून्य dumpSmall(std::ostream& str) अचर override;
    bool hasDType() अचर override VL_MT_SAFE { फर्काऊ true; }
    bool maybePointedTo() अचर override VL_MT_SAFE { फर्काऊ true; }
    bool undead() अचर override { फर्काऊ true; }
    Ast_गाँठो_DType* subDTypep() अचर override VL_MT_STABLE { फर्काऊ nullptr; }
    Ast_गाँठो_DType* virtRefDTypep() अचर override { फर्काऊ nullptr; }
    शून्य virtRefDTypep(Ast_गाँठो_DType* nodep) override {}
    bool similar_DType_गाँठो(अचर Ast_गाँठो_DType* samep) अचर override { फर्काऊ this == samep; }
    AstBasicDType* basicp() अचर override VL_MT_STABLE { फर्काऊ nullptr; }
    पूर्णाङ्क widthAlignBytes() अचर override { फर्काऊ 1; }
    पूर्णाङ्क widthTotalBytes() अचर override { फर्काऊ 1; }
    bool isCompound() अचर override { फर्काऊ false; }
};
class AstEnumDType final : सार्वजनिक Ast_गाँठो_DType {
    // Parents: TYPEDEF/MODULE
    // @astgen op1 := childDTypep : Optional[AstNodeDType]
    // @astgen op2 := itemsp : List[AstEnumItem]
    //
    // @astgen ptr := m_refDTypep : Optional[AstNodeDType]  // Elements of this type (post-width)
    string m_name;  // Name from upper typedef, if any
    string m_typedefName;  // Typedef's dtypeName() once moved off the typedef declaring it
    अचर पूर्णाङ्क m_unique_अङ्क;  // Unique ID distinguishing this dtype instance, for hashing/naming

सार्वजनिक:
    AstEnumDType(File_पङ्क्ति* fl, VFlagChildDType, Ast_गाँठो_DType* dtp, AstEnumItem* itemsp)
        : ASTGEN_SUPER_EnumDType(fl)
        , m_unique_अङ्क{unique_अङ्क_Inc()} {
        childDTypep(dtp);  // Only for parser
        refDTypep(nullptr);
        addItemsp(itemsp);
        dtypep(nullptr);  // V3Width will resolve
        widthFromSub(subDTypep());
    }
    AstEnumDType(अचर AstEnumDType& other)
        : Ast_गाँठो_DType{other}
        , m_name{other.m_name}
        , m_typedefName{other.m_typedefName}
        , m_unique_अङ्क{unique_अङ्क_Inc()} {}
    ASTGEN_MEMBERS_AstEnumDType;

    अचर वर्ण* broken() अचर override;
    पूर्णाङ्क unique_अङ्क() अचर { फर्काऊ m_unique_अङ्क; }
    bool same_गाँठो(अचर Ast_गाँठो* samep) अचर override {
        अचर AstEnumDType* अचर sp = VN_DBG_AS(samep, EnumDType);
        फर्काऊ unique_अङ्क() == sp->unique_अङ्क();
    }
    bool similar_DType_गाँठो(अचर Ast_गाँठो_DType* samep) अचर override { फर्काऊ same_गाँठो(samep); }
    Ast_गाँठो_DType* getChildDTypep() अचर override { फर्काऊ childDTypep(); }
    Ast_गाँठो_DType* subDTypep() अचर override VL_MT_STABLE {
        फर्काऊ m_refDTypep ? m_refDTypep : childDTypep();
    }
    शून्य refDTypep(Ast_गाँठो_DType* nodep) { m_refDTypep = nodep; }
    Ast_गाँठो_DType* virtRefDTypep() अचर override { फर्काऊ m_refDTypep; }
    शून्य virtRefDTypep(Ast_गाँठो_DType* nodep) override { refDTypep(nodep); }
    string name() अचर override VL_MT_STABLE { फर्काऊ m_name; }
    शून्य name(अचर string& flag) override { m_name = flag; }
    // Keep the name the typedef declaring it gives, as it moves off it to the type table
    शून्य typedefName(अचर string& name) { m_typedefName = name; }
    शून्य dump(std::ostream& str = std::cout) अचर override;
    शून्य dumpJson(std::ostream& str = std::cout) अचर override;
    शून्य dumpSmall(std::ostream& str) अचर override;
    string prettyDTypeName(bool full) अचर override;
    // METHODS
    AstBasicDType* basicp() अचर override VL_MT_STABLE { फर्काऊ subDTypep()->basicp(); }
    पूर्णाङ्क widthAlignBytes() अचर override { फर्काऊ subDTypep()->widthAlignBytes(); }
    पूर्णाङ्क widthTotalBytes() अचर override { फर्काऊ subDTypep()->widthTotalBytes(); }
    माप_प्रकार item_गन्ती() अचर {
        माप_प्रकार गन्ती = 0;
        लागि (Ast_गाँठो* itemp = itemsp(); itemp; itemp = itemp->nextp()) ++गन्ती;
        फर्काऊ गन्ती;
    }
    bool isCompound() अचर override { फर्काऊ false; }
};

class AstIfaceGenericDType final : सार्वजनिक Ast_गाँठो_DType {
    // Generic interface that will be replaced with AstIfaceRefDType
    File_पङ्क्ति* m_modportFileline;  // Where modport token was
    // dist-ast-dump-suppress  // Part of name()
    string m_modportName;  // "" = no modport
सार्वजनिक:
    explicit AstIfaceGenericDType(File_पङ्क्ति* fl)
        : ASTGEN_SUPER_IfaceGenericDType(fl) {
        dtypep(this);
    }
    AstIfaceGenericDType(File_पङ्क्ति* fl, File_पङ्क्ति* modportFl, अचर string& modport)
        : ASTGEN_SUPER_IfaceGenericDType(fl)
        , m_modportFileline{modportFl}
        , m_modportName{modport} {
        dtypep(this);
    }
    ASTGEN_MEMBERS_AstIfaceGenericDType;
    शून्य dumpSmall(std::ostream& str) अचर override;
    bool hasDType() अचर override VL_MT_SAFE { फर्काऊ true; }
    bool maybePointedTo() अचर override VL_MT_SAFE { फर्काऊ true; }
    bool undead() अचर override { फर्काऊ true; }
    Ast_गाँठो_DType* subDTypep() अचर override VL_MT_STABLE { फर्काऊ nullptr; }
    Ast_गाँठो_DType* virtRefDTypep() अचर override { फर्काऊ nullptr; }
    शून्य virtRefDTypep(Ast_गाँठो_DType* nodep) override {}
    bool similar_DType_गाँठो(अचर Ast_गाँठो_DType* samep) अचर override { फर्काऊ this == samep; }
    AstBasicDType* basicp() अचर override VL_MT_STABLE { फर्काऊ nullptr; }
    पूर्णाङ्क widthAlignBytes() अचर override { फर्काऊ 1; }
    पूर्णाङ्क widthTotalBytes() अचर override { फर्काऊ 1; }
    string modportName() अचर { फर्काऊ m_modportName; }
    bool isModport() { फर्काऊ !m_modportName.empty(); }
    bool isCompound() अचर override { फर्काऊ true; }
    File_पङ्क्ति* modportFileline() अचर { फर्काऊ m_modportFileline; }
    string name() अचर override { फर्काऊ m_modportName; }
};

class AstIfaceRefDType final : सार्वजनिक Ast_गाँठो_DType {
    // Reference to an interface, either for a port, or inside parent cell
    // @astgen op1 := paramsp : List[AstPin]
    //
    // @astgen ptr := m_ifacep : Optional[AstIface]  // Interface; cellp() should override
    // @astgen ptr := m_cellp : Optional[AstCell]  // When exact parent cell known; not a guess
    // @astgen ptr := m_modportp : Optional[AstModport]  // nullptr = unlinked or no modport
    File_पङ्क्ति* m_modportFileline;  // Where modport token was
    string m_cellName;  // "" = no cell, such as when connects to 'input' iface
    string m_ifaceName;  // Interface name
    string m_modportName;  // "" = no modport
    bool m_virtual = false;  // True if virtual interface
सार्वजनिक:
    AstIfaceRefDType(File_पङ्क्ति* fl, अचर string& cellName, अचर string& ifaceName)
        : ASTGEN_SUPER_IfaceRefDType(fl)
        , m_modportFileline{nullptr}
        , m_cellName{cellName}
        , m_ifaceName{ifaceName}
        , m_modportName{""} {}
    AstIfaceRefDType(File_पङ्क्ति* fl, File_पङ्क्ति* modportFl, अचर string& cellName,
                     अचर string& ifaceName, अचर string& modport)
        : ASTGEN_SUPER_IfaceRefDType(fl)
        , m_modportFileline{modportFl}
        , m_cellName{cellName}
        , m_ifaceName{ifaceName}
        , m_modportName{modport} {}
    AstIfaceRefDType(File_पङ्क्ति* fl, File_पङ्क्ति* modportFl, अचर string& cellName,
                     अचर string& ifaceName, अचर string& modport, AstPin* paramsp)
        : ASTGEN_SUPER_IfaceRefDType(fl)
        , m_modportFileline{modportFl}
        , m_cellName{cellName}
        , m_ifaceName{ifaceName}
        , m_modportName{modport} {
        addParamsp(paramsp);
    }
    ASTGEN_MEMBERS_AstIfaceRefDType;

    // METHODS
    शून्य dump(std::ostream& str = std::cout) अचर override;
    शून्य dumpJson(std::ostream& str = std::cout) अचर override;
    शून्य dumpSmall(std::ostream& str) अचर override;
    string prettyDTypeName(bool full) अचर override;
    AstBasicDType* basicp() अचर override VL_MT_STABLE { फर्काऊ nullptr; }
    bool similar_DType_गाँठो(अचर Ast_गाँठो_DType* samep) अचर override {
        // Each occurrence of a virtual interface type parses to its own node,
        // so pointer identity is insufficient; compare the referenced
        // interface instead, same equivalence as AstNode::computeCastable uses.
        अचर AstIfaceRefDType* अचर asamep = VN_DBG_AS(samep, IfaceRefDType);
        फर्काऊ ifaceViaCellp() && ifaceViaCellp() == asamep->ifaceViaCellp()
               && modportName() == asamep->modportName() && isVirtual() == asamep->isVirtual();
    }
    पूर्णाङ्क widthAlignBytes() अचर override { फर्काऊ 0; }
    पूर्णाङ्क widthTotalBytes() अचर override { फर्काऊ 0; }
    bool isVirtual() अचर { फर्काऊ m_virtual; }
    शून्य isVirtual(bool flag) {
        m_virtual = flag;
        यदि (flag) v3Global.setHasVirtIfaces();
    }
    File_पङ्क्ति* modportFileline() अचर { फर्काऊ m_modportFileline; }
    शून्य modportFileline(File_पङ्क्ति* अचर modportFileline) { m_modportFileline = modportFileline; }
    string cellName() अचर { फर्काऊ m_cellName; }
    शून्य cellName(अचर string& name) { m_cellName = name; }
    string ifaceName() अचर { फर्काऊ m_ifaceName; }
    string ifaceNameQ() अचर { फर्काऊ "'" + prettyName(ifaceName()) + "'"; }
    शून्य ifaceName(अचर string& name) { m_ifaceName = name; }
    शून्य modportName(अचर string& modportName) { m_modportName = modportName; }
    string modportName() अचर { फर्काऊ m_modportName; }
    AstIface* ifaceViaCellp() अचर;  // Use cellp or ifacep
    AstIface* ifacep() अचर { फर्काऊ m_ifacep; }
    शून्य ifacep(AstIface* nodep) { m_ifacep = nodep; }
    AstCell* cellp() अचर { फर्काऊ m_cellp; }
    शून्य cellp(AstCell* nodep) { m_cellp = nodep; }
    AstModport* modportp() अचर { फर्काऊ m_modportp; }
    शून्य modportp(AstModport* modportp) { m_modportp = modportp; }
    bool isModport() { फर्काऊ !m_modportName.empty(); }
    bool isCompound() अचर override { फर्काऊ true; }  // But not relevant
};
class AstMemberDType final : सार्वजनिक Ast_गाँठो_DType {
    // A member of a struct/union
    // PARENT: AstNodeUOrStructDType
    // @astgen op1 := childDTypep : Optional[AstNodeDType]
    // @astgen op3 := valuep : Optional[AstNode]
    //
    // @astgen ptr := m_refDTypep : Optional[AstNodeDType]  // Elements of this type (post-width)
    string m_name;  // Name of variable
    string m_tag;  // Holds the string of the verilator tag -- used in JSON output.
    पूर्णाङ्क m_lsb = -1;  // Within this level's packed struct, the LSB of the first bit of the member
    bool m_constrainedRand = false;  // Member has a constraint expression
    VRandAttr m_rand;  // Randomizability of this member (rand, randc, etc)
सार्वजनिक:
    AstMemberDType(File_पङ्क्ति* fl, अचर string& name, VFlagChildDType, Ast_गाँठो_DType* dtp,
                   Ast_गाँठो* valuep)
        : ASTGEN_SUPER_MemberDType(fl)
        , m_name{name} {
        childDTypep(dtp);  // Only for parser
        this->valuep(valuep);
        dtypep(nullptr);  // V3Width will resolve
        refDTypep(nullptr);
    }
    AstMemberDType(File_पङ्क्ति* fl, अचर string& name, Ast_गाँठो_DType* dtp)
        : ASTGEN_SUPER_MemberDType(fl)
        , m_name{name} {
        UASSERT(dtp, "AstMember created with no dtype");
        refDTypep(dtp);
        dtypep(this);
        widthFromSub(subDTypep());
    }
    ASTGEN_MEMBERS_AstMemberDType;
    शून्य dump(std::ostream& str = std::cout) अचर override;
    शून्य dumpJson(std::ostream& str = std::cout) अचर override;
    शून्य dumpSmall(std::ostream& str) अचर override;
    string prettyDTypeName(bool full) अचर override;
    string name() अचर override VL_MT_STABLE { फर्काऊ m_name; }  // * = Var name
    bool hasDType() अचर override VL_MT_SAFE { फर्काऊ true; }
    bool maybePointedTo() अचर override VL_MT_SAFE { फर्काऊ true; }
    Ast_गाँठो_DType* getChildDTypep() अचर override { फर्काऊ childDTypep(); }
    Ast_गाँठो_UOr_Struct_DType* getChildStructp();
    Ast_गाँठो_DType* subDTypep() अचर override VL_MT_STABLE {
        फर्काऊ m_refDTypep ? m_refDTypep : childDTypep();
    }
    शून्य refDTypep(Ast_गाँठो_DType* nodep) { m_refDTypep = nodep; }
    Ast_गाँठो_DType* virtRefDTypep() अचर override { फर्काऊ m_refDTypep; }
    शून्य virtRefDTypep(Ast_गाँठो_DType* nodep) override { refDTypep(nodep); }
    bool similar_DType_गाँठो(अचर Ast_गाँठो_DType* samep) अचर override { फर्काऊ this == samep; }
    //
    // (Slow) recurse down to find basic data type (Note don't need virtual -
    // AstVar isn't a NodeDType)
    AstBasicDType* basicp() अचर override VL_MT_STABLE { फर्काऊ subDTypep()->basicp(); }
    // op1 = Range of variable (Note don't need virtual - AstVar isn't a NodeDType)
    Ast_गाँठो_DType* dtypeSkipRefp() अचर { फर्काऊ subDTypep()->skipRefp(); }
    // (Slow) recurses - Structure alignment 1,2,4 or 8 bytes (arrays affect this)
    पूर्णाङ्क widthAlignBytes() अचर override { फर्काऊ subDTypep()->widthAlignBytes(); }
    // (Slow) recurses - Width in bytes rounding up 1,2,4,8,12,...
    पूर्णाङ्क widthTotalBytes() अचर override { फर्काऊ subDTypep()->widthTotalBytes(); }
    // METHODS
    शून्य name(अचर string& name) override { m_name = name; }
    string tag() अचर override { फर्काऊ m_tag; }
    शून्य tag(अचर string& text) override { m_tag = text; }
    पूर्णाङ्क lsb() अचर { फर्काऊ m_lsb; }
    शून्य lsb(पूर्णाङ्क lsb) { m_lsb = lsb; }
    bool isCompound() अचर override {
        v3fatalSrc("call isCompound on subdata type, not reference");
        फर्काऊ false;
    }
    bool isConstrainedRand() अचर { फर्काऊ m_constrainedRand; }
    शून्य markConstrainedRand(bool flag) { m_constrainedRand = flag; }
    VRandAttr अक्रम() अचर { फर्काऊ m_rand; }
    शून्य अक्रम(अचर VRandAttr flag) { m_rand = flag; }
};
class AstNBACommitQueueDType final : सार्वजनिक Ast_गाँठो_DType {
    // @astgen ptr := m_subDTypep : AstNodeDType  // Type of the corresponding variable
    अचर bool m_partial;  // Partial element update required

सार्वजनिक:
    AstNBACommitQueueDType(File_पङ्क्ति* fl, Ast_गाँठो_DType* subDTypep, bool partial)
        : ASTGEN_SUPER_NBACommitQueueDType(fl)
        , m_partial{partial}
        , m_subDTypep{subDTypep} {
        dtypep(this);
    }
    ASTGEN_MEMBERS_AstNBACommitQueueDType;
    शून्य dump(std::ostream& str) अचर override;
    शून्य dumpJson(std::ostream& str) अचर override;
    Ast_गाँठो_DType* subDTypep() अचर override VL_MT_STABLE { फर्काऊ m_subDTypep; }
    bool partial() अचर { फर्काऊ m_partial; }
    bool same_गाँठो(अचर Ast_गाँठो* samep) अचर override {
        अचर AstNBACommitQueueDType* अचर asamep = VN_DBG_AS(samep, NBACommitQueueDType);
        फर्काऊ m_partial == asamep->m_partial;
    }
    bool similar_DType_गाँठो(अचर Ast_गाँठो_DType* samep) अचर override { फर्काऊ this == samep; }
    AstBasicDType* basicp() अचर override VL_MT_STABLE { फर्काऊ nullptr; }
    पूर्णाङ्क widthAlignBytes() अचर override { फर्काऊ 1; }
    पूर्णाङ्क widthTotalBytes() अचर override { फर्काऊ 24; }
    bool isCompound() अचर override { फर्काऊ true; }
};
class AstParamTypeDType final : सार्वजनिक Ast_गाँठो_DType {
    // Parents: MODULE
    // A parameter type statement; much like a var or typedef
    // @astgen op1 := childDTypep : Optional[AstNodeDType]
    अचर VVarType m_varType;  // Type of variable (for localparam vs. param)
    अचर VFwdType m_fwdType;  // Forward type for lint check
    string m_name;  // Name of variable
सार्वजनिक:
    AstParamTypeDType(File_पङ्क्ति* fl, VVarType type, VFwdType fwdType, अचर string& name,
                      VFlagChildDType, Ast_गाँठो_DType* dtp)
        : ASTGEN_SUPER_ParamTypeDType(fl)
        , m_varType{type}
        , m_fwdType{fwdType}
        , m_name{name} {
        childDTypep(dtp);  // Only for parser
        dtypep(nullptr);  // V3Width will resolve
    }
    ASTGEN_MEMBERS_AstParamTypeDType;
    शून्य dump(std::ostream& str = std::cout) अचर override;
    शून्य dumpJson(std::ostream& str = std::cout) अचर override;
    string prettyDTypeName(bool full) अचर override;
    Ast_गाँठो_DType* getChildDTypep() अचर override { फर्काऊ childDTypep(); }
    Ast_गाँठो_DType* subDTypep() अचर override VL_MT_STABLE {
        फर्काऊ dtypep() ? dtypep() : childDTypep();
    }
    AstBasicDType* basicp() अचर override VL_MT_STABLE { फर्काऊ subDTypep()->basicp(); }
    bool similar_DType_गाँठो(अचर Ast_गाँठो_DType* samep) अचर override {
        अचर AstParamTypeDType* अचर sp = VN_DBG_AS(samep, ParamTypeDType);
        फर्काऊ this->subDTypep()->similarDType(sp->subDTypep());
    }
    पूर्णाङ्क widthAlignBytes() अचर override { फर्काऊ dtypep()->widthAlignBytes(); }
    पूर्णाङ्क widthTotalBytes() अचर override { फर्काऊ dtypep()->widthTotalBytes(); }
    // METHODS
    string name() अचर override VL_MT_STABLE { फर्काऊ m_name; }
    bool maybePointedTo() अचर override VL_MT_SAFE { फर्काऊ true; }
    bool hasDType() अचर override VL_MT_SAFE { फर्काऊ true; }
    शून्य name(अचर string& flag) override { m_name = flag; }
    VVarType varType() अचर { फर्काऊ m_varType; }  // * = Type of variable
    VFwdType fwdType() अचर { फर्काऊ m_fwdType; }
    bool isParam() अचर { फर्काऊ true; }
    bool isGParam() अचर { फर्काऊ (varType() == VVarType::GPARAM); }
    bool isCompound() अचर override {
        v3fatalSrc("call isCompound on subdata type, not reference");
        फर्काऊ false;
    }
};
class AstParseTypeDType final : सार्वजनिक Ast_गाँठो_DType {
    // Parents: VAR
    // During parsing, this indicates the type of a parameter is a "parameter type"
    // e.g. the data type is a container of any data type
    अचर VFwdType m_fwdType;  // Forward type for lint check
सार्वजनिक:
    explicit AstParseTypeDType(File_पङ्क्ति* fl, VFwdType fwdType = VFwdType::NONE)
        : ASTGEN_SUPER_ParseTypeDType(fl)
        , m_fwdType{fwdType} {}
    ASTGEN_MEMBERS_AstParseTypeDType;
    Ast_गाँठो_DType* dtypep() अचर VL_MT_STABLE { फर्काऊ nullptr; }
    // METHODS
    शून्य dump(std::ostream& str = std::cout) अचर override;
    शून्य dumpJson(std::ostream& str = std::cout) अचर override;
    bool similar_DType_गाँठो(अचर Ast_गाँठो_DType* samep) अचर override { फर्काऊ this == samep; }
    AstBasicDType* basicp() अचर override VL_MT_STABLE { फर्काऊ nullptr; }
    पूर्णाङ्क widthAlignBytes() अचर override { फर्काऊ 0; }
    पूर्णाङ्क widthTotalBytes() अचर override { फर्काऊ 0; }
    bool isCompound() अचर override {
        v3fatalSrc("call isCompound on subdata type, not reference");
        फर्काऊ false;
    }
    VFwdType fwdType() अचर { फर्काऊ m_fwdType; }
};
class AstQueueDType final : सार्वजनिक Ast_गाँठो_DType {
    // Queue array data type, ie "[ $ ]"
    // @astgen op1 := childDTypep : Optional[AstNodeDType] // moved to refDTypep() in V3Width
    // @astgen op2 := boundp : Optional[AstNodeExpr]
    //
    // @astgen ptr := m_refDTypep : Optional[AstNodeDType]  // Elements of this type (post-width)
सार्वजनिक:
    AstQueueDType(File_पङ्क्ति* fl, VFlagChildDType, Ast_गाँठो_DType* dtp, Ast_गाँठो_अभिव्यक्ति* boundp)
        : ASTGEN_SUPER_QueueDType(fl) {
        childDTypep(dtp);
        this->boundp(boundp);
        refDTypep(nullptr);
        dtypep(nullptr);  // V3Width will resolve
    }
    AstQueueDType(File_पङ्क्ति* fl, Ast_गाँठो_DType* dtp, Ast_गाँठो_अभिव्यक्ति* boundp)
        : ASTGEN_SUPER_QueueDType(fl) {
        this->boundp(boundp);
        refDTypep(dtp);
        dtypep(dtp);
    }
    ASTGEN_MEMBERS_AstQueueDType;
    अचर वर्ण* broken() अचर override {
        BROKEN_RTN(!((m_refDTypep && !childDTypep()) || (!m_refDTypep && childDTypep())));
        फर्काऊ nullptr;
    }
    bool same_गाँठो(अचर Ast_गाँठो* samep) अचर override {
        अचर AstQueueDType* अचर asamep = VN_DBG_AS(samep, QueueDType);
        यदि (!asamep->subDTypep()) फर्काऊ false;
        फर्काऊ (subDTypep() == asamep->subDTypep());
    }
    bool similar_DType_गाँठो(अचर Ast_गाँठो_DType* samep) अचर override {
        अचर AstQueueDType* अचर asamep = VN_DBG_AS(samep, QueueDType);
        फर्काऊ asamep->subDTypep() && subDTypep()->similarDType(asamep->subDTypep());
    }
    शून्य dumpSmall(std::ostream& str) अचर override;
    string prettyDTypeName(bool full) अचर override;
    Ast_गाँठो_DType* getChildDTypep() अचर override { फर्काऊ childDTypep(); }
    Ast_गाँठो_DType* subDTypep() अचर override VL_MT_STABLE {
        फर्काऊ m_refDTypep ? m_refDTypep : childDTypep();
    }
    bool isAggregateType() अचर override { फर्काऊ true; }
    शून्य refDTypep(Ast_गाँठो_DType* nodep) { m_refDTypep = nodep; }
    अन्तःस्थ पूर्णाङ्क boundConst() अचर VL_MT_STABLE;
    Ast_गाँठो_DType* virtRefDTypep() अचर override { फर्काऊ m_refDTypep; }
    शून्य virtRefDTypep(Ast_गाँठो_DType* nodep) override { refDTypep(nodep); }
    // METHODS
    AstBasicDType* basicp() अचर override VL_MT_STABLE { फर्काऊ nullptr; }
    पूर्णाङ्क widthAlignBytes() अचर override { फर्काऊ subDTypep()->widthAlignBytes(); }
    पूर्णाङ्क widthTotalBytes() अचर override { फर्काऊ subDTypep()->widthTotalBytes(); }
    bool isCompound() अचर override { फर्काऊ true; }
    bool isDynamicallySized() अचर override { फर्काऊ true; }
};
class AstRefDType final : सार्वजनिक Ast_गाँठो_DType {
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
    अचर VIfaceCaptureTag* m_captureTagp = nullptr;

सार्वजनिक:
    AstRefDType(File_पङ्क्ति* fl, अचर string& name)
        : ASTGEN_SUPER_RefDType(fl)
        , m_name{name} {}
    AstRefDType(File_पङ्क्ति* fl, अचर string& name, Ast_गाँठो_अभिव्यक्ति* classOrPackagep, AstPin* paramsp)
        : ASTGEN_SUPER_RefDType(fl)
        , m_name{name} {
        classOrPackageOpp(classOrPackagep);
        addParamsp(paramsp);
    }
    class Flag_Type_Of_अभिव्यक्ति {};  // type(expr) for parser only
    AstRefDType(File_पङ्क्ति* fl, Flag_Type_Of_अभिव्यक्ति, Ast_गाँठो* typeofp)
        : ASTGEN_SUPER_RefDType(fl) {
        this->typeofp(typeofp);
        यदि (Ast_गाँठो_DType* अचर dtp = VN_CAST(typeofp, गाँठो_DType)) refDTypep(dtp);
    }
    ASTGEN_MEMBERS_AstRefDType;
    // METHODS
    bool same_गाँठो(अचर Ast_गाँठो* samep) अचर override {
        अचर AstRefDType* अचर asamep = VN_DBG_AS(samep, RefDType);
        फर्काऊ (m_typedefp == asamep->m_typedefp && m_refDTypep == asamep->m_refDTypep
                && m_name == asamep->m_name && m_classOrPackagep == asamep->m_classOrPackagep);
    }
    bool similar_DType_गाँठो(अचर Ast_गाँठो_DType* samep) अचर override {
        फर्काऊ subDTypep()->similarDType(samep->subDTypep());
    }
    अचर वर्ण* broken() अचर override;
    शून्य dump(std::ostream& str = std::cout) अचर override;
    शून्य dumpJson(std::ostream& str = std::cout) अचर override;
    शून्य dumpSmall(std::ostream& str) अचर override;
    string name() अचर override VL_MT_STABLE { फर्काऊ m_name; }
    string prettyDTypeName(bool full) अचर override {
        फर्काऊ subDTypep() ? subDTypep()->prettyDTypeName(full) : prettyName();
    }
    AstBasicDType* basicp() अचर override VL_MT_STABLE {
        फर्काऊ subDTypep() ? subDTypep()->basicp() : nullptr;
    }
    Ast_गाँठो_DType* subDTypep() अचर override VL_MT_STABLE;
    Ast_गाँठो_DType* getChildDTypep() अचर override { फर्काऊ VN_CAST(typeofp(), गाँठो_DType); }
    पूर्णाङ्क widthAlignBytes() अचर override { फर्काऊ dtypeSkipRefp()->widthAlignBytes(); }
    पूर्णाङ्क widthTotalBytes() अचर override { फर्काऊ dtypeSkipRefp()->widthTotalBytes(); }
    शून्य name(अचर string& flag) override { m_name = flag; }
    Ast_गाँठो_DType* dtypeSkipRefp() अचर { फर्काऊ subDTypep()->skipRefp(); }
    AstTypedef* typedefp() अचर VL_MT_SAFE { फर्काऊ m_typedefp; }
    शून्य typedefp(AstTypedef* nodep) { m_typedefp = nodep; }
    Ast_गाँठो_DType* refDTypep() अचर VL_MT_SAFE { फर्काऊ m_refDTypep; }
    शून्य refDTypep(Ast_गाँठो_DType* nodep) { m_refDTypep = nodep; }
    Ast_गाँठो_DType* virtRefDTypep() अचर override { फर्काऊ refDTypep(); }
    शून्य virtRefDTypep(Ast_गाँठो_DType* nodep) override { refDTypep(nodep); }
    Ast_गाँठो_Module* classOrPackagep() अचर { फर्काऊ m_classOrPackagep; }
    शून्य classOrPackagep(Ast_गाँठो_Module* nodep) { m_classOrPackagep = nodep; }
    अचर VIfaceCaptureTag* captureTagp() अचर { फर्काऊ m_captureTagp; }
    शून्य captureTagp(अचर VIfaceCaptureTag* tagp) { m_captureTagp = tagp; }
    bool isCompound() अचर override {
        v3fatalSrc("call isCompound on subdata type, not reference");
        फर्काऊ false;
    }
};
class AstRequireDType final : सार्वजनिक Ast_गाँठो_DType {
    // @astgen op1 := lhsp : Optional[AstNode<AstNodeExpr|AstNodeDType>]
    //
    // Require a generic node type (typically AstParseRef become a type.
सार्वजनिक:
    AstRequireDType(File_पङ्क्ति* fl, Ast_गाँठो* lhsp)
        : ASTGEN_SUPER_RequireDType(fl) {
        this->lhsp(lhsp);
    }
    ASTGEN_MEMBERS_AstRequireDType;
    // METHODS
    bool similar_DType_गाँठो(अचर Ast_गाँठो_DType* samep) अचर override {
        अचर AstRequireDType* अचर asamep = VN_DBG_AS(samep, RequireDType);
        फर्काऊ subDTypep()->similarDType(asamep->subDTypep());
    }
    AstBasicDType* basicp() अचर override VL_MT_STABLE { फर्काऊ nullptr; }
    Ast_गाँठो_DType* subDTypep() अचर override VL_MT_STABLE {
        // Used for recursive definition checking
        यदि (Ast_गाँठो_DType* अचर dtp = VN_CAST(lhsp(), गाँठो_DType)) फर्काऊ dtp;

        फर्काऊ nullptr;
    }
    पूर्णाङ्क widthAlignBytes() अचर override { V3ERROR_NA_RETURN(1); }
    पूर्णाङ्क widthTotalBytes() अचर override { V3ERROR_NA_RETURN(1); }
    bool isCompound() अचर override { V3ERROR_NA_RETURN(false); }
};
class AstSampleQueueDType final : सार्वजनिक Ast_गाँठो_DType {
    // @astgen op1 := childDTypep : Optional[AstNodeDType] // moved to refDTypep() in V3Width
    //
    // @astgen ptr := m_refDTypep : Optional[AstNodeDType]  // Elements of this type (post-width)
सार्वजनिक:
    AstSampleQueueDType(File_पङ्क्ति* fl, Ast_गाँठो_DType* dtp)
        : ASTGEN_SUPER_SampleQueueDType(fl) {
        refDTypep(dtp);
        dtypep(dtp);
    }
    ASTGEN_MEMBERS_AstSampleQueueDType;
    अचर वर्ण* broken() अचर override {
        BROKEN_RTN(!((m_refDTypep && !childDTypep()) || (!m_refDTypep && childDTypep())));
        फर्काऊ nullptr;
    }
    bool same_गाँठो(अचर Ast_गाँठो* samep) अचर override {
        अचर AstSampleQueueDType* अचर asamep = VN_DBG_AS(samep, SampleQueueDType);
        यदि (!asamep->subDTypep()) फर्काऊ false;
        फर्काऊ (subDTypep() == asamep->subDTypep());
    }
    bool similar_DType_गाँठो(अचर Ast_गाँठो_DType* samep) अचर override {
        अचर AstSampleQueueDType* अचर asamep = VN_DBG_AS(samep, SampleQueueDType);
        फर्काऊ asamep->subDTypep() && subDTypep()->similarDType(asamep->subDTypep());
    }
    शून्य dumpSmall(std::ostream& str) अचर override;
    Ast_गाँठो_DType* getChildDTypep() अचर override { फर्काऊ childDTypep(); }
    // op1 = Range of variable
    Ast_गाँठो_DType* subDTypep() अचर override VL_MT_STABLE {
        फर्काऊ m_refDTypep ? m_refDTypep : childDTypep();
    }
    शून्य refDTypep(Ast_गाँठो_DType* nodep) { m_refDTypep = nodep; }
    Ast_गाँठो_DType* virtRefDTypep() अचर override { फर्काऊ m_refDTypep; }
    शून्य virtRefDTypep(Ast_गाँठो_DType* nodep) override { refDTypep(nodep); }
    // METHODS
    AstBasicDType* basicp() अचर override VL_MT_STABLE { फर्काऊ subDTypep()->basicp(); }
    पूर्णाङ्क widthAlignBytes() अचर override { फर्काऊ परिमाण(std::map<std::string, std::string>); }
    पूर्णाङ्क widthTotalBytes() अचर override { फर्काऊ परिमाण(std::map<std::string, std::string>); }
    bool isCompound() अचर override { फर्काऊ true; }
};
class AstStreamDType final : सार्वजनिक Ast_गाँठो_DType {
    // Stream data type, used only as data type of stream operations
    // Should behave like AstPackArrayDType, but it doesn't have a size
सार्वजनिक:
    explicit AstStreamDType(File_पङ्क्ति* fl)
        : ASTGEN_SUPER_StreamDType(fl) {
        dtypep(this);
    }
    ASTGEN_MEMBERS_AstStreamDType;
    शून्य dumpSmall(std::ostream& str) अचर override;
    bool hasDType() अचर override VL_MT_SAFE { फर्काऊ true; }
    bool maybePointedTo() अचर override VL_MT_SAFE { फर्काऊ true; }
    bool undead() अचर override { फर्काऊ true; }
    Ast_गाँठो_DType* subDTypep() अचर override VL_MT_STABLE { फर्काऊ nullptr; }
    Ast_गाँठो_DType* virtRefDTypep() अचर override { फर्काऊ nullptr; }
    शून्य virtRefDTypep(Ast_गाँठो_DType* nodep) override {}
    bool similar_DType_गाँठो(अचर Ast_गाँठो_DType* samep) अचर override { फर्काऊ this == samep; }
    AstBasicDType* basicp() अचर override VL_MT_STABLE { फर्काऊ nullptr; }
    पूर्णाङ्क widthAlignBytes() अचर override { फर्काऊ 1; }
    पूर्णाङ्क widthTotalBytes() अचर override { फर्काऊ 1; }
    bool isCompound() अचर override { फर्काऊ false; }
};
class AstUnsizedArrayDType final : सार्वजनिक Ast_गाँठो_DType {
    // Unsized/open-range Array data type, ie "some_dtype var_name []"
    // @astgen op1 := childDTypep : Optional[AstNodeDType] // moved to refDTypep() in V3Width
    //
    // @astgen ptr := m_refDTypep : Optional[AstNodeDType]  // Elements of this type (post-width)
सार्वजनिक:
    AstUnsizedArrayDType(File_पङ्क्ति* fl, VFlagChildDType, Ast_गाँठो_DType* dtp)
        : ASTGEN_SUPER_UnsizedArrayDType(fl) {
        childDTypep(dtp);  // Only for parser
        refDTypep(nullptr);
        dtypep(nullptr);  // V3Width will resolve
    }
    AstUnsizedArrayDType(File_पङ्क्ति* fl, Ast_गाँठो_DType* dtp)
        : ASTGEN_SUPER_UnsizedArrayDType(fl) {
        refDTypep(dtp);
        dtypep(nullptr);  // V3Width will resolve
    }
    ASTGEN_MEMBERS_AstUnsizedArrayDType;
    अचर वर्ण* broken() अचर override {
        BROKEN_RTN(!((m_refDTypep && !childDTypep()) || (!m_refDTypep && childDTypep())));
        फर्काऊ nullptr;
    }
    bool same_गाँठो(अचर Ast_गाँठो* samep) अचर override;
    bool similar_DType_गाँठो(अचर Ast_गाँठो_DType* samep) अचर override;
    शून्य dumpSmall(std::ostream& str) अचर override;
    Ast_गाँठो_DType* getChildDTypep() अचर override { फर्काऊ childDTypep(); }
    Ast_गाँठो_DType* subDTypep() अचर override VL_MT_STABLE {
        फर्काऊ m_refDTypep ? m_refDTypep : childDTypep();
    }
    शून्य refDTypep(Ast_गाँठो_DType* nodep) { m_refDTypep = nodep; }
    Ast_गाँठो_DType* virtRefDTypep() अचर override { फर्काऊ m_refDTypep; }
    शून्य virtRefDTypep(Ast_गाँठो_DType* nodep) override { refDTypep(nodep); }
    // METHODS
    AstBasicDType* basicp() अचर override VL_MT_STABLE { फर्काऊ subDTypep()->basicp(); }
    पूर्णाङ्क widthAlignBytes() अचर override { फर्काऊ subDTypep()->widthAlignBytes(); }
    पूर्णाङ्क widthTotalBytes() अचर override { फर्काऊ subDTypep()->widthTotalBytes(); }
    bool isCompound() अचर override { फर्काऊ true; }
};
class AstVoidDType final : सार्वजनिक Ast_गाँठो_DType {
    // For e.g. a function returning void
सार्वजनिक:
    explicit AstVoidDType(File_पङ्क्ति* fl)
        : ASTGEN_SUPER_VoidDType(fl) {
        dtypep(this);
    }
    ASTGEN_MEMBERS_AstVoidDType;
    शून्य dumpSmall(std::ostream& str) अचर override;
    bool hasDType() अचर override VL_MT_SAFE { फर्काऊ true; }
    bool maybePointedTo() अचर override VL_MT_SAFE { फर्काऊ true; }
    bool undead() अचर override { फर्काऊ true; }
    Ast_गाँठो_DType* subDTypep() अचर override VL_MT_STABLE { फर्काऊ nullptr; }
    Ast_गाँठो_DType* virtRefDTypep() अचर override { फर्काऊ nullptr; }
    शून्य virtRefDTypep(Ast_गाँठो_DType* nodep) override {}
    bool similar_DType_गाँठो(अचर Ast_गाँठो_DType* samep) अचर override { फर्काऊ true; }
    AstBasicDType* basicp() अचर override VL_MT_STABLE { फर्काऊ nullptr; }
    पूर्णाङ्क widthAlignBytes() अचर override { फर्काऊ 1; }
    पूर्णाङ्क widthTotalBytes() अचर override { फर्काऊ 1; }
    bool isCompound() अचर override { फर्काऊ false; }
};
class AstWildcardArrayDType final : सार्वजनिक Ast_गाँठो_DType {
    // Wildcard index type associative array data type, ie "some_dtype var_name [*]"
    // @astgen op1 := childDTypep : Optional[AstNodeDType] // moved to refDTypep() in V3Width
    //
    // @astgen ptr := m_refDTypep : Optional[AstNodeDType]  // Elements of this type (post-width)
सार्वजनिक:
    AstWildcardArrayDType(File_पङ्क्ति* fl, VFlagChildDType, Ast_गाँठो_DType* dtp)
        : ASTGEN_SUPER_WildcardArrayDType(fl) {
        childDTypep(dtp);  // Only for parser
        refDTypep(nullptr);
        dtypep(nullptr);  // V3Width will resolve
    }
    ASTGEN_MEMBERS_AstWildcardArrayDType;
    अचर वर्ण* broken() अचर override {
        BROKEN_RTN(!((m_refDTypep && !childDTypep()) || (!m_refDTypep && childDTypep())));
        फर्काऊ nullptr;
    }
    bool same_गाँठो(अचर Ast_गाँठो* samep) अचर override;
    bool similar_DType_गाँठो(अचर Ast_गाँठो_DType* samep) अचर override;
    शून्य dumpSmall(std::ostream& str) अचर override;
    string prettyDTypeName(bool full) अचर override;
    Ast_गाँठो_DType* getChildDTypep() अचर override { फर्काऊ childDTypep(); }
    Ast_गाँठो_DType* subDTypep() अचर override VL_MT_STABLE {
        फर्काऊ m_refDTypep ? m_refDTypep : childDTypep();
    }
    शून्य refDTypep(Ast_गाँठो_DType* nodep) { m_refDTypep = nodep; }
    Ast_गाँठो_DType* virtRefDTypep() अचर override { फर्काऊ m_refDTypep; }
    शून्य virtRefDTypep(Ast_गाँठो_DType* nodep) override { refDTypep(nodep); }
    // METHODS
    AstBasicDType* basicp() अचर override VL_MT_STABLE { फर्काऊ subDTypep()->basicp(); }
    पूर्णाङ्क widthAlignBytes() अचर override { फर्काऊ परिमाण(std::map<std::string, std::string>); }
    पूर्णाङ्क widthTotalBytes() अचर override { फर्काऊ परिमाण(std::map<std::string, std::string>); }
    bool isCompound() अचर override { फर्काऊ true; }
    bool isDynamicallySized() अचर override { फर्काऊ true; }
};

// === AstNodeArrayDType ===
class AstPackArrayDType final : सार्वजनिक Ast_गाँठो_Array_DType {
    // Packed array data type, ie "some_dtype [2:0] var_name"
सार्वजनिक:
    अन्तःस्थ AstPackArrayDType(File_पङ्क्ति* fl, VFlagChildDType, Ast_गाँठो_DType* dtp, AstRange* rangep);
    अन्तःस्थ AstPackArrayDType(File_पङ्क्ति* fl, Ast_गाँठो_DType* dtp, AstRange* rangep);
    ASTGEN_MEMBERS_AstPackArrayDType;
    string prettyDTypeName(bool full) अचर override;
    bool isCompound() अचर override { फर्काऊ false; }
};
class AstUnpackArrayDType final : सार्वजनिक Ast_गाँठो_Array_DType {
    // Array data type, ie "some_dtype var_name [2:0]"
    bool m_isCompound = false;  // Non-POD subDType, or parent requires compound
सार्वजनिक:
    AstUnpackArrayDType(File_पङ्क्ति* fl, VFlagChildDType, Ast_गाँठो_DType* dtp, AstRange* rangep)
        : ASTGEN_SUPER_UnpackArrayDType(fl) {
        childDTypep(dtp);  // Only for parser
        this->rangep(rangep);
        refDTypep(nullptr);
        dtypep(nullptr);  // V3Width will resolve
        // For backward compatibility AstNodeArrayDType and others inherit
        // width and signing from the subDType/base type
        widthFromSub(subDTypep());
    }
    AstUnpackArrayDType(File_पङ्क्ति* fl, Ast_गाँठो_DType* dtp, AstRange* rangep)
        : ASTGEN_SUPER_UnpackArrayDType(fl) {
        this->rangep(rangep);
        refDTypep(dtp);
        dtypep(this);
        // For backward compatibility AstNodeArrayDType and others inherit
        // width and signing from the subDType/base type
        widthFromSub(subDTypep());
    }
    ASTGEN_MEMBERS_AstUnpackArrayDType;
    शून्य dump(std::ostream& str) अचर override;
    शून्य dumpJson(std::ostream& str) अचर override;
    string prettyDTypeName(bool full) अचर override;
    bool same_गाँठो(अचर Ast_गाँठो* samep) अचर override {
        अचर AstUnpackArrayDType* अचर sp = VN_DBG_AS(samep, UnpackArrayDType);
        फर्काऊ m_isCompound == sp->m_isCompound;
    }
    bool similar_DType_गाँठो(अचर Ast_गाँठो_DType* samep) अचर override;
    bool isAggregateType() अचर override { फर्काऊ true; }
    // True iff 'otherp' is an array of the same shape, with
    // at least one dimension reversed ascending/descending.
    bool isReverseOf(अचर Ast_गाँठो_DType* otherp) अचर;
    // Outer dimension comes first. The first element is this node.
    std::vector<AstUnpackArrayDType*> unpackDimensions();
    शून्य isCompound(bool flag) { m_isCompound = flag; }
    bool isCompound() अचर override VL_MT_SAFE { फर्काऊ m_isCompound; }
    bool isIntegralOrPacked() अचर override { फर्काऊ false; }
};

// === AstNodeUOrStructDType ===
class AstStructDType final : सार्वजनिक Ast_गाँठो_UOr_Struct_DType {
सार्वजनिक:
    // VSigning below is mispurposed to indicate if packed or not
    AstStructDType(File_पङ्क्ति* fl, VSigning numericUnpack)
        : ASTGEN_SUPER_StructDType(fl, numericUnpack) {}
    ASTGEN_MEMBERS_AstStructDType;
    string verilogKwd() अचर override { फर्काऊ "struct"; }
};
class AstUnionDType final : सार्वजनिक Ast_गाँठो_UOr_Struct_DType {
    bool m_isSoft;  // Is a "union soft"
    bool m_isTagged;  // Is a "union tagged"

सार्वजनिक:
    // VSigning below is mispurposed to indicate if packed or not
    // isSoft implies packed
    AstUnionDType(File_पङ्क्ति* fl, bool isSoft, bool isTagged, VSigning numericUnpack)
        : ASTGEN_SUPER_UnionDType(fl, numericUnpack)
        , m_isSoft{isSoft}
        , m_isTagged{isTagged} {
        packed(packed() | m_isSoft);
    }
    ASTGEN_MEMBERS_AstUnionDType;
    string verilogKwd() अचर override { फर्काऊ "union"; }
    bool isSoft() अचर { फर्काऊ m_isSoft; }
    bool isTagged() अचर { फर्काऊ m_isTagged; }
    bool same_गाँठो(अचर Ast_गाँठो* samep) अचर override;
    शून्य dump(std::ostream& str) अचर override;
    शून्य dumpJson(std::ostream& str) अचर override;
};

#endif  // Guard
