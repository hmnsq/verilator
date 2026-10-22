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

#अगर_अघोषित VERILATOR_V3ASTNODEDTYPE_H_
#घोषणा VERILATOR_V3ASTNODEDTYPE_H_

#अगर_अघोषित VERILATOR_V3AST_H_
#त्रुटि "Use V3Ast.h as the include"
#समावेश "V3Ast.h"  // This helps code analysis tools pick up symbols in V3Ast.h
#घोषणा VL_NOT_FINAL  // This #define fixes broken code folding in the CLion IDE
#पूर्ण_अगर

// === Abstract base node types (AstNode*) =====================================

class Ast_गाँठ_DType VL_NOT_FINAL : खुला Ast_गाँठ {
    // Ideally width() would migrate to BasicDType as that's where it makes sense,
    // but it's currently so prevalent in the code we leave it here.
    // Note the below members are included in AstTypeTable::Key lookups
    // dist-ast-dump-suppress  // Part of dumpSmall
    पूर्णांक m_width = 0;  // (also in AstTypeTable::Key) Bit width of operation
    // (also in AstTypeTable::Key) If unsized, bitwidth of minimum implementation
    // dist-ast-dump-suppress  // Part of dumpSmall
    पूर्णांक m_widthMin = 0;
    // dist-ast-dump-suppress  // Part of dumpSmall
    VSigning m_numeric;  // (also in AstTypeTable::Key) Node is signed
    // dist-ast-dump-suppress  // Part of dumpSmall
    bool m_generic = false;  // Simple globally referenced type, don't garbage collect
    // Unique number assigned to each dtype during creation for IEEE matching
    अटल पूर्णांक s_unique_अंक;

रक्षित:
    // CONSTRUCTORS
    Ast_गाँठ_DType(VNType t, File_पंक्ति* fl)
        : Ast_गाँठ{t, fl} {}

निजी:
    // METHODS
    स्थिर Ast_गाँठ_DType* skipRefIterp(bool skipConst, bool skipEnum,
                                     bool assertOn = true) स्थिर VL_MT_STABLE;

रक्षित:
    // METHODS
    भव bool similar_DType_गाँठ(स्थिर Ast_गाँठ_DType* samep) स्थिर = 0;

खुला:
    ASTGEN_MEMBERS_Ast_गाँठ_DType;
    // ACCESSORS
    व्योम dump(std::ostream& str) स्थिर override;
    व्योम dumpJson(std::ostream& str) स्थिर override;
    भव व्योम dumpSmall(std::ostream& str) स्थिर VL_MT_STABLE;
    bool hasDType() स्थिर override VL_MT_SAFE { वापस true; }
    /// Require VlUnpacked, instead of [] for POD elements.
    /// A non-POD object is always compound, but some POD elements
    /// are compound when methods calls operate on object, or when
    /// under another compound-requiring object e.g. class
    भव bool isCompound() स्थिर = 0;
    // Integral or packed, allowed inside an unpacked union/struct
    भव bool isIntegralOrPacked() स्थिर { वापस !isCompound(); }
    // (Slow) recurse down to find basic data type
    भव AstBasicDType* basicp() स्थिर VL_MT_STABLE = 0;
    // (Slow) Recurse over MemberDType|ParamTypeDType|RefDType|ConstDType|EnumDType to other type
    स्थिर Ast_गाँठ_DType* skipRefp() स्थिर VL_MT_STABLE { वापस skipRefIterp(true, true); }
    Ast_गाँठ_DType* skipRefp() VL_MT_STABLE {
        वापस const_cast<Ast_गाँठ_DType*>(
            static_cast<स्थिर Ast_गाँठ_DType*>(this)->skipRefIterp(true, true));
    }
    // (Slow) Recurse over MemberDType|ParamTypeDType|RefDType|ConstDType|EnumDType to other type,
    // Returns null if not resolved
    स्थिर Ast_गाँठ_DType* skipRefOrNullp() स्थिर VL_MT_STABLE {
        वापस skipRefIterp(true, true, false);
    }
    Ast_गाँठ_DType* skipRefOrNullp() VL_MT_STABLE {
        वापस const_cast<Ast_गाँठ_DType*>(
            static_cast<स्थिर Ast_गाँठ_DType*>(this)->skipRefIterp(true, true, false));
    }
    // (Slow) Recurse over MemberDType|ParamTypeDType|RefDType|EnumDType to ConstDType
    स्थिर Ast_गाँठ_DType* skipRefToConstp() स्थिर { वापस skipRefIterp(false, true); }
    Ast_गाँठ_DType* skipRefToConstp() {
        वापस const_cast<Ast_गाँठ_DType*>(
            static_cast<स्थिर Ast_गाँठ_DType*>(this)->skipRefIterp(false, true));
    }
    // (Slow) Recurse over MemberDType|ParamTypeDType|RefDType|ConstDType to EnumDType
    स्थिर Ast_गाँठ_DType* skipRefToEnump() स्थिर { वापस skipRefIterp(true, false); }
    Ast_गाँठ_DType* skipRefToEnump() {
        वापस const_cast<Ast_गाँठ_DType*>(
            static_cast<स्थिर Ast_गाँठ_DType*>(this)->skipRefIterp(true, false));
    }
    // (Slow) Recurse over MemberDType|ParamTypeDType|RefDType|ConstDType to EnumDType,
    // Returns null if not resolved
    स्थिर Ast_गाँठ_DType* skipRefToEnumOrNullp() स्थिर { वापस skipRefIterp(true, false, false); }
    // (Slow) Recurse over MemberDType|ParamTypeDType|RefDType to other type
    स्थिर Ast_गाँठ_DType* skipRefToNonRefp() स्थिर { वापस skipRefIterp(false, false); }
    Ast_गाँठ_DType* skipRefToNonRefp() {
        वापस const_cast<Ast_गाँठ_DType*>(
            static_cast<स्थिर Ast_गाँठ_DType*>(this)->skipRefIterp(false, false));
    }
    // If array, returns element dtype, otherwise returns skipRef dtype
    // If skipRef is false, RefDTypes are not followed (safe before typedef linking)
    स्थिर Ast_गाँठ_DType* elemDTypep(bool skipRef = true) स्थिर VL_MT_STABLE;
    Ast_गाँठ_DType* elemDTypep(bool skipRef = true) VL_MT_STABLE {
        वापस const_cast<Ast_गाँठ_DType*>(
            static_cast<स्थिर Ast_गाँठ_DType*>(this)->elemDTypep(skipRef));
    }
    // (Slow) recurses - Structure alignment 1,2,4 or 8 bytes (arrays affect this)
    भव पूर्णांक widthAlignBytes() स्थिर = 0;
    // (Slow) recurses - Width in bytes rounding up 1,2,4,8,12,...
    भव पूर्णांक width_योग_Bytes() स्थिर = 0;
    bool maybePointedTo() स्थिर override VL_MT_SAFE { वापस true; }
    // Iff has a non-null refDTypep(), as generic node function
    भव Ast_गाँठ_DType* virtRefDTypep() स्थिर { वापस nullptr; }
    // Iff has refDTypep(), set as generic node function
    भव व्योम virtRefDTypep(Ast_गाँठ_DType* nodep) {}
    // Iff has a non-null second dtypep, as generic node function
    भव Ast_गाँठ_DType* virtRefDType2p() स्थिर { वापस nullptr; }
    // Iff has second dtype, set as generic node function
    भव व्योम virtRefDType2p(Ast_गाँठ_DType* nodep) {}
    // Assignable equivalence.  Calls skipRefToNonRefp() during comparisons.
    bool similarDType(स्थिर Ast_गाँठ_DType* samep) स्थिर;
    // Iff has a non-null subDTypep(), as generic node function
    भव Ast_गाँठ_DType* subDTypep() स्थिर VL_MT_STABLE { वापस nullptr; }
    भव Ast_गाँठ_DType* subDType2p() स्थिर VL_MT_STABLE { वापस nullptr; }
    भव bool isAggregateType() स्थिर { वापस false; }
    // True for unpacked, dynamic, queue, and associative arrays (not packed arrays)
    bool isNonPackedArray() स्थिर;
    भव bool isFourstate() स्थिर;
    // Ideally an IEEE $typename
    भव string prettyDTypeName(bool) स्थिर { वापस prettyTypeName(); }
    string prettyDTypeNameQ(bool full = false) स्थिर { वापस "'" + prettyDTypeName(full) + "'"; }
    string stateDTypeName() स्थिर { वापस this->isFourstate() ? "(4-state)" : "(2-state)"; }
    //
    // Changing the width may confuse the data type resolution, so must clear
    // TypeTable cache after use.
    व्योम widthForce(पूर्णांक width, पूर्णांक widthMin) {
        m_width = width;
        m_widthMin = widthMin;
    }
    // For backward compatibility inherit width and signing from the subDType/base type
    व्योम widthFromSub(स्थिर Ast_गाँठ_DType* nodep) {
        m_width = nodep->m_width;
        m_widthMin = nodep->m_widthMin;
        m_numeric = nodep->m_numeric;
    }
    //
    पूर्णांक width() स्थिर VL_MT_STABLE { वापस m_width; }
    व्योम numeric(VSigning flag) { m_numeric = flag; }
    bool isSigned() स्थिर VL_MT_STABLE { वापस m_numeric.isSigned(); }
    bool isNosign() स्थिर VL_MT_SAFE { वापस m_numeric.isNosign(); }
    VSigning numeric() स्थिर VL_MT_STABLE { वापस m_numeric; }
    पूर्णांक width_शब्दगण() स्थिर VL_MT_STABLE { वापस VL_शब्दगण_I(width()); }
    पूर्णांक widthMin() स्थिर VL_MT_STABLE {  // If sized, the size,
                                         // if unsized the min digits to represent it
        वापस m_widthMin ? m_widthMin : m_width;
    }
    पूर्णांक widthPow2() स्थिर;
    व्योम widthMinFromWidth() { m_widthMin = m_width; }
    bool widthSized() स्थिर VL_MT_SAFE { वापस !m_widthMin || m_widthMin == m_width; }
    bool generic() स्थिर VL_MT_SAFE { वापस m_generic; }
    व्योम generic(bool flag) { m_generic = flag; }
    std::pair<uint32_t, uint32_t> dimensions(bool includeBasic) स्थिर;
    uint32_t arrayUnpackedElements() स्थिर;  // 1, or total multiplication of all dimensions
    // Fixed aggregate streaming properties
    bool isStreamableFixedAggregate() स्थिर;
    // Whether is a string, or an unpacked array or structure containing one
    bool containsString() स्थिर;
    bool containsUnpackedStruct() स्थिर;
    पूर्णांक widthStream() स्थिर;
    string vlEnumType() स्थिर;  // Return VerilatedVarType: VLVT_UINT32, etc
    string vlEnumPackedAgg() स्थिर;  // Return VLVF_PACKED_AGG etc flags, or empty
    अटल पूर्णांक unique_अंक_Inc() { वापस ++s_unique_अंक; }
    स्थिर अक्षर* charIQWN() स्थिर {
        वापस (isString() ? "N" : isWide() ? "W" : isDouble() ? "D" : isQuad() ? "Q" : "I");
    }
    string cType(स्थिर string& name, bool forFunc, bool isRef, bool packed = false) स्थिर;
    // Represents a C++ LiteralType? (can be constexpr)
    bool isLiteralType() स्थिर VL_MT_STABLE;
    // C++ type (as emitted by cType) is trivially default constructible and destructible?
    bool isTriviallyConstructible() स्थिर VL_MT_STABLE;
    भव bool isDynamicallySized() स्थिर { वापस false; }

निजी:
    class CTypeRecursed;
    CTypeRecursed cTypeRecurse(bool compound, bool packed) स्थिर;
};
class Ast_गाँठ_Array_DType VL_NOT_FINAL : खुला Ast_गाँठ_DType {
    // Array data type, ie "some_dtype var_name [2:0]"
    // @astgen op1 := childDTypep : Optional[AstNodeDType] // moved to refDTypep() in V3Width
    // @astgen op2 := rangep : Optional[AstRange] // array bounds
    //
    // @astgen ptr := m_refDTypep : Optional[AstNodeDType]  // Elements of this type (post-width)
    Ast_गाँठ* rangenp() स्थिर { वापस reinterpret_cast<Ast_गाँठ*>(rangep()); }

रक्षित:
    Ast_गाँठ_Array_DType(VNType t, File_पंक्ति* fl)
        : Ast_गाँठ_DType{t, fl} {}

खुला:
    ASTGEN_MEMBERS_Ast_गाँठ_Array_DType;
    व्योम dump(std::ostream& str) स्थिर override;
    व्योम dumpJson(std::ostream& str) स्थिर override;
    व्योम dumpSmall(std::ostream& str) स्थिर override;
    स्थिर अक्षर* broken() स्थिर override {
        BROKEN_RTN(!((m_refDTypep && !childDTypep()) || (!m_refDTypep && childDTypep())));
        वापस nullptr;
    }
    bool same_गाँठ(स्थिर Ast_गाँठ* samep) स्थिर override {
        स्थिर Ast_गाँठ_Array_DType* स्थिर asamep = VN_DBG_AS(samep, गाँठ_Array_DType);
        वापस hi() == asamep->hi() && rangenp()->same_वृक्ष(asamep->rangenp())
               && subDTypep() == asamep->subDTypep();
    }  // HashedDT doesn't recurse, so need to check children
    bool similar_DType_गाँठ(स्थिर Ast_गाँठ_DType* samep) स्थिर override {
        स्थिर Ast_गाँठ_Array_DType* स्थिर asamep = VN_DBG_AS(samep, गाँठ_Array_DType);
        वापस elementsConst() == asamep->elementsConst()
               && subDTypep()->similarDType(asamep->subDTypep());
    }
    Ast_गाँठ_DType* getChildDTypep() स्थिर override { वापस childDTypep(); }
    Ast_गाँठ_DType* subDTypep() स्थिर override VL_MT_STABLE {
        वापस m_refDTypep ? m_refDTypep : childDTypep();
    }
    व्योम refDTypep(Ast_गाँठ_DType* nodep) { m_refDTypep = nodep; }
    Ast_गाँठ_DType* virtRefDTypep() स्थिर override { वापस m_refDTypep; }
    व्योम virtRefDTypep(Ast_गाँठ_DType* nodep) override { refDTypep(nodep); }
    // METHODS
    AstBasicDType* basicp() स्थिर override VL_MT_STABLE {
        वापस subDTypep()->basicp();
    }  // (Slow) recurse down to find basic data type
    पूर्णांक widthAlignBytes() स्थिर override { वापस subDTypep()->widthAlignBytes(); }
    पूर्णांक width_योग_Bytes() स्थिर override {
        वापस elementsConst() * subDTypep()->width_योग_Bytes();
    }
    अंतरभूत पूर्णांक बायाँ() स्थिर VL_MT_STABLE;
    अंतरभूत पूर्णांक दायाँ() स्थिर VL_MT_STABLE;
    अंतरभूत पूर्णांक hi() स्थिर VL_MT_STABLE;
    अंतरभूत पूर्णांक lo() स्थिर VL_MT_STABLE;
    अंतरभूत पूर्णांक elementsConst() स्थिर VL_MT_STABLE;
    अंतरभूत VNumRange declRange() स्थिर VL_MT_STABLE;
};
class Ast_गाँठ_UOr_Struct_DType VL_NOT_FINAL : खुला Ast_गाँठ_DType {
    // A struct or union; common handling
    // @astgen op1 := membersp : List[AstMemberDType]
    //
    // @astgen ptr := m_classOrPackagep : Optional[AstNodeModule]  // Package emitted with
    string m_name;  // Name from upper typedef, if any
    string m_typedefName;  // Typedef's dtypeName() once moved off the typedef declaring it
    स्थिर पूर्णांक m_unique_अंक;  // Unique ID distinguishing this dtype instance, for hashing/naming
    bool m_packed;  // Packed struct/union, else unpacked
    bool m_isFourstate = false;  // V3Width computes; true if any member is 4-state
    bool m_constrainedRand = false;  // True if struct has constraint expression
    bool m_emitToString = false;  // Generate to_string() for this struct/union if set

रक्षित:
    Ast_गाँठ_UOr_Struct_DType(VNType t, File_पंक्ति* fl, VSigning numericUnpack)
        : Ast_गाँठ_DType{t, fl}
        , m_unique_अंक{unique_अंक_Inc()} {
        // VSigning::NOSIGN overloaded to indicate not packed
        m_packed = (numericUnpack != VSigning::NOSIGN);
        numeric(VSigning::fromBool(numericUnpack.isSigned()));
    }
    Ast_गाँठ_UOr_Struct_DType(स्थिर Ast_गाँठ_UOr_Struct_DType& other)
        : Ast_गाँठ_DType{other}
        , m_name{other.m_name}
        , m_typedefName{other.m_typedefName}
        , m_unique_अंक{unique_अंक_Inc()}
        , m_packed{other.m_packed}
        , m_isFourstate{other.m_isFourstate} {}

खुला:
    ASTGEN_MEMBERS_Ast_गाँठ_UOr_Struct_DType;
    पूर्णांक unique_अंक() स्थिर { वापस m_unique_अंक; }
    व्योम dump(std::ostream& str) स्थिर override;
    व्योम dumpJson(std::ostream& str) स्थिर override;
    string prettyDTypeName(bool) स्थिर override;
    bool isCompound() स्थिर override { वापस !packed(); }
    // For basicp() we reuse the size to indicate a "fake" basic type of same size
    AstBasicDType* basicp() स्थिर override VL_MT_STABLE {
        अगर (!m_packed) वापस nullptr;
        वापस (isFourstate()
                    ? VN_AS(findLogicRangeDType(VNumRange{width() - 1, 0}, width(), numeric()),
                            BasicDType)
                    : VN_AS(findBitRangeDType(VNumRange{width() - 1, 0}, width(), numeric()),
                            BasicDType));
    }
    // (Slow) recurses - Structure alignment 1,2,4 or 8 bytes (arrays affect this)
    पूर्णांक widthAlignBytes() स्थिर override;
    // (Slow) recurses - Width in bytes rounding up 1,2,4,8,12,...
    पूर्णांक width_योग_Bytes() स्थिर override;
    bool similar_DType_गाँठ(स्थिर Ast_गाँठ_DType* samep) स्थिर override;
    string name() स्थिर override VL_MT_STABLE { वापस m_name; }
    व्योम name(स्थिर string& flag) override { m_name = flag; }
    // Keep the name the typedef declaring it gives, as it moves off it to the type table
    व्योम typedefName(स्थिर string& name) { m_typedefName = name; }
    // Name of the C structure for DPI, empty if no typedef names it, so declared where used
    string dpiName() स्थिर;
    // C typedef of the unpacked structure for DPI (IEEE 1800-2023 H.7.3)
    string dpiTypedef() स्थिर;
    bool packed() स्थिर VL_MT_SAFE { वापस m_packed; }
    व्योम packed(bool flag) { m_packed = flag; }
    // packed() but as don't support unpacked, presently all structs
    अटल bool packedUnsup() { वापस true; }
    व्योम isFourstate(bool flag) { m_isFourstate = flag; }
    bool isFourstate() स्थिर override VL_MT_SAFE { वापस m_isFourstate; }
    अटल पूर्णांक lo() VL_MT_STABLE { वापस 0; }
    पूर्णांक hi() स्थिर VL_MT_STABLE {
        वापस dtypep()->width() - 1;
    }  // Packed classes look like arrays
    VNumRange declRange() स्थिर VL_MT_STABLE { वापस VNumRange{hi(), lo()}; }
    Ast_गाँठ_Module* classOrPackagep() स्थिर { वापस m_classOrPackagep; }
    व्योम classOrPackagep(Ast_गाँठ_Module* classpackagep) { m_classOrPackagep = classpackagep; }
    bool isConstrainedRand() स्थिर { वापस m_constrainedRand; }
    व्योम markConstrainedRand(bool flag) { m_constrainedRand = flag; }
    bool emitToString() स्थिर { वापस m_emitToString; }
    व्योम setEmitToString() { m_emitToString = true; }
    bool isAggregateType() स्थिर override { वापस !packed(); }
};

// === Concrete node types =====================================================

// === AstNode ===
class AstEnumItem final : खुला Ast_गाँठ {
    // @astgen op1 := rangep : Optional[AstRange] // Range for name appending
    // @astgen op2 := valuep : Optional[AstNodeExpr]
    string m_name;  // Name of item

खुला:
    // Parents: ENUM
    AstEnumItem(File_पंक्ति* fl, स्थिर string& name, AstRange* rangep, Ast_गाँठ_व्यंजक* valuep)
        : ASTGEN_SUPER_EnumItem(fl)
        , m_name{name} {
        this->rangep(rangep);
        this->valuep(valuep);
    }
    ASTGEN_MEMBERS_AstEnumItem;
    string name() स्थिर override VL_MT_STABLE { वापस m_name; }
    bool maybePointedTo() स्थिर override VL_MT_SAFE { वापस true; }
    bool hasDType() स्थिर override VL_MT_SAFE { वापस true; }
    व्योम name(स्थिर string& flag) override { m_name = flag; }
};

// === AstNodeDType ===
class AstAssocArrayDType final : खुला Ast_गाँठ_DType {
    // Associative array data type, ie "[some_dtype]"
    // @astgen op1 := childDTypep : Optional[AstNodeDType] // moved to refDTypep() in V3Width
    // @astgen op2 := keyChildDTypep : Optional[AstNodeDType]
    //
    // @astgen ptr := m_refDTypep : Optional[AstNodeDType]  // Elements of this type (post-width)
    // @astgen ptr := m_keyDTypep : Optional[AstNodeDType]  // Keys of this type (post-width)
खुला:
    AstAssocArrayDType(File_पंक्ति* fl, VFlagChildDType, Ast_गाँठ_DType* dtp, Ast_गाँठ_DType* कुंजी_Dtp)
        : ASTGEN_SUPER_AssocArrayDType(fl) {
        childDTypep(dtp);  // Only for parser
        कुंजी_Child_DTypep(कुंजी_Dtp);  // Only for parser
        refDTypep(nullptr);
        कुंजी_DTypep(nullptr);
        dtypep(nullptr);  // V3Width will resolve
    }
    AstAssocArrayDType(File_पंक्ति* fl, Ast_गाँठ_DType* dtp, Ast_गाँठ_DType* कुंजी_Dtp)
        : ASTGEN_SUPER_AssocArrayDType(fl) {
        refDTypep(dtp);
        कुंजी_DTypep(कुंजी_Dtp);
        dtypep(dtp);
    }
    ASTGEN_MEMBERS_AstAssocArrayDType;
    स्थिर अक्षर* broken() स्थिर override {
        BROKEN_RTN(!((m_refDTypep && !childDTypep()) || (!m_refDTypep && childDTypep())));
        BROKEN_RTN(!((m_कुंजी_DTypep && !childDTypep()) || (!m_कुंजी_DTypep && childDTypep())));
        वापस nullptr;
    }
    bool same_गाँठ(स्थिर Ast_गाँठ* samep) स्थिर override {
        स्थिर AstAssocArrayDType* स्थिर asamep = VN_DBG_AS(samep, AssocArrayDType);
        अगर (!asamep->subDTypep()) वापस false;
        अगर (!asamep->कुंजी_DTypep()) वापस false;
        वापस (subDTypep() == asamep->subDTypep() && कुंजी_DTypep() == asamep->कुंजी_DTypep());
    }
    bool similar_DType_गाँठ(स्थिर Ast_गाँठ_DType* samep) स्थिर override {
        स्थिर AstAssocArrayDType* स्थिर asamep = VN_DBG_AS(samep, AssocArrayDType);
        वापस asamep->subDTypep() && subDTypep()->similarDType(asamep->subDTypep())
               && asamep->कुंजी_DTypep() && कुंजी_DTypep()->similarDType(asamep->कुंजी_DTypep());
    }
    string prettyDTypeName(bool full) स्थिर override;
    व्योम dumpSmall(std::ostream& str) स्थिर override;
    Ast_गाँठ_DType* getChildDTypep() स्थिर override { वापस childDTypep(); }
    Ast_गाँठ_DType* getChild2DTypep() स्थिर override { वापस कुंजी_Child_DTypep(); }
    Ast_गाँठ_DType* subDTypep() स्थिर override VL_MT_STABLE {
        वापस m_refDTypep ? m_refDTypep : childDTypep();
    }
    Ast_गाँठ_DType* subDType2p() स्थिर override VL_MT_STABLE {
        वापस m_कुंजी_DTypep ? m_कुंजी_DTypep : कुंजी_Child_DTypep();
    }
    bool isAggregateType() स्थिर override { वापस true; }
    व्योम refDTypep(Ast_गाँठ_DType* nodep) { m_refDTypep = nodep; }
    Ast_गाँठ_DType* virtRefDTypep() स्थिर override { वापस m_refDTypep; }
    व्योम virtRefDTypep(Ast_गाँठ_DType* nodep) override { refDTypep(nodep); }
    Ast_गाँठ_DType* virtRefDType2p() स्थिर override { वापस m_कुंजी_DTypep; }
    व्योम virtRefDType2p(Ast_गाँठ_DType* nodep) override { कुंजी_DTypep(nodep); }
    //
    Ast_गाँठ_DType* कुंजी_DTypep() स्थिर VL_MT_STABLE {
        वापस m_कुंजी_DTypep ? m_कुंजी_DTypep : कुंजी_Child_DTypep();
    }
    व्योम कुंजी_DTypep(Ast_गाँठ_DType* nodep) { m_कुंजी_DTypep = nodep; }
    // METHODS
    AstBasicDType* basicp() स्थिर override VL_MT_STABLE { वापस nullptr; }
    पूर्णांक widthAlignBytes() स्थिर override { वापस subDTypep()->widthAlignBytes(); }
    पूर्णांक width_योग_Bytes() स्थिर override { वापस subDTypep()->width_योग_Bytes(); }
    bool isCompound() स्थिर override { वापस true; }
    bool isDynamicallySized() स्थिर override { वापस true; }
};
class AstBasicDType final : खुला Ast_गाँठ_DType {
    // Builtin atomic/vectored data type
    // @astgen op1 := rangep : Optional[AstRange] // Range of variable
    काष्ठा Members final {
        VBasicDTypeKwd m_keyword;  // (also in VBasicTypeKey) What keyword created basic type
        // dist-ast-dump-suppress  // Part of dumpSmall
        VNumRange m_nrange;  // (also in VBasicTypeKey) Numeric msb/lsb (if non-opaque keyword)
        bool चालक==(स्थिर Members& rhs) स्थिर {
            वापस rhs.m_keyword == m_keyword && rhs.m_nrange == m_nrange;
        }
    } m;
    // See also in AstNodeDType: m_width, m_widthMin, m_numeric(issigned)
खुला:
    AstBasicDType(File_पंक्ति* fl, VBasicDTypeKwd kwd, स्थिर VSigning& signst = VSigning::NOSIGN)
        : ASTGEN_SUPER_BasicDType(fl) {
        init(kwd, signst, 0, -1, nullptr);
    }
    AstBasicDType(File_पंक्ति* fl, VFlagLogicPacked, पूर्णांक wantwidth)
        : ASTGEN_SUPER_BasicDType(fl) {
        init(VBasicDTypeKwd::LOGIC, VSigning::NOSIGN, wantwidth, -1, nullptr);
    }
    AstBasicDType(File_पंक्ति* fl, VFlagLogic2StatePacked, पूर्णांक wantwidth)
        : ASTGEN_SUPER_BasicDType(fl) {
        init(VBasicDTypeKwd::LOGIC2STATE, VSigning::NOSIGN, wantwidth, -1, nullptr);
    }
    AstBasicDType(File_पंक्ति* fl, VFlagBitPacked, पूर्णांक wantwidth)
        : ASTGEN_SUPER_BasicDType(fl) {
        init(VBasicDTypeKwd::BIT, VSigning::NOSIGN, wantwidth, -1, nullptr);
    }
    AstBasicDType(File_पंक्ति* fl, VBasicDTypeKwd kwd, VSigning numer, पूर्णांक wantwidth, पूर्णांक widthmin)
        : ASTGEN_SUPER_BasicDType(fl) {
        init(kwd, numer, wantwidth, widthmin, nullptr);
    }
    AstBasicDType(File_पंक्ति* fl, VBasicDTypeKwd kwd, VSigning numer, VNumRange range, पूर्णांक widthmin)
        : ASTGEN_SUPER_BasicDType(fl) {
        init(kwd, numer, range.elements(), widthmin, nullptr);
        m.m_nrange = range;  // as init() presumes lsb==0, but range.lsb() might not be
    }
    // See also addRange in verilog.y
निजी:
    व्योम init(VBasicDTypeKwd kwd, VSigning numer, पूर्णांक wantwidth, पूर्णांक wantwidthmin,
              AstRange* rangep);

खुला:
    ASTGEN_MEMBERS_AstBasicDType;
    व्योम dump(std::ostream& str) स्थिर override;
    व्योम dumpJson(std::ostream& str) स्थिर override;
    // width/widthMin/numeric compared elsewhere
    bool same_गाँठ(स्थिर Ast_गाँठ* samep) स्थिर override;
    bool similar_DType_गाँठ(स्थिर Ast_गाँठ_DType* samep) स्थिर override;
    string name() स्थिर override VL_MT_STABLE { वापस m.m_keyword.typeName(); }
    string prettyDTypeName(bool full) स्थिर override;
    स्थिर अक्षर* broken() स्थिर override {
        BROKEN_RTN(dtypep() != this);
        BROKEN_RTN(v3Global.widthMinUsage() == VWidthMinUsage::VERILOG_WIDTH
                   && widthMin() > width());
        वापस nullptr;
    }
    व्योम setSignedState(स्थिर VSigning& signst) {
        // Note NOSIGN does NOT change the state; this is required by the parser
        अगर (signst == VSigning::UNSIGNED) {
            numeric(signst);
        } अन्यथा अगर (signst == VSigning::SIGNED) {
            numeric(signst);
        }
    }
    // METHODS
    AstBasicDType* basicp() स्थिर override VL_MT_STABLE { वापस (AstBasicDType*)this; }
    // (Slow) recurses - Structure alignment 1,2,4 or 8 bytes (arrays affect this)
    पूर्णांक widthAlignBytes() स्थिर override;
    // (Slow) recurses - Width in bytes rounding up 1,2,4,8,12,...
    पूर्णांक width_योग_Bytes() स्थिर override;
    bool isFourstate() स्थिर override { वापस keyword().isFourstate(); }
    VBasicDTypeKwd keyword() स्थिर VL_MT_SAFE {  // Avoid using - use isSomething accessors instead
        वापस m.m_keyword;
    }
    bool isBitLogic() स्थिर { वापस keyword().isBitLogic(); }
    bool isBit() स्थिर { वापस keyword().isBit(); }
    bool isCHandle() स्थिर VL_MT_STABLE { वापस keyword().isCHandle(); }
    bool isDouble() स्थिर VL_MT_STABLE { वापस keyword().isDouble(); }
    bool isEvent() स्थिर VL_MT_STABLE { वापस keyword() == VBasicDTypeKwd::EVENT; }
    bool isForkSync() स्थिर VL_MT_SAFE { वापस keyword() == VBasicDTypeKwd::FORK_SYNC; }
    bool isProcessRef() स्थिर VL_MT_SAFE { वापस keyword() == VBasicDTypeKwd::PROCESS_REFERENCE; }
    bool isDelayScheduler() स्थिर VL_MT_SAFE {
        वापस keyword() == VBasicDTypeKwd::DELAY_SCHEDULER;
    }
    bool isTriggerScheduler() स्थिर VL_MT_SAFE {
        वापस keyword() == VBasicDTypeKwd::TRIGGER_SCHEDULER;
    }
    bool isDynamicTriggerScheduler() स्थिर VL_MT_SAFE {
        वापस keyword() == VBasicDTypeKwd::DYNAMIC_TRIGGER_SCHEDULER;
    }
    bool isRandomGenerator() स्थिर VL_MT_SAFE {
        वापस keyword() == VBasicDTypeKwd::RANDOM_GENERATOR;
    }
    bool isStdRandomGenerator() स्थिर VL_MT_SAFE {
        वापस keyword() == VBasicDTypeKwd::RANDOM_STDGENERATOR;
    }
    bool isCovergroupInstHandle() स्थिर VL_MT_SAFE {
        वापस keyword() == VBasicDTypeKwd::COVERGROUP_INSTHANDLE;
    }
    bool isOpaque() स्थिर VL_MT_SAFE { वापस keyword().isOpaque(); }
    bool isString() स्थिर VL_MT_STABLE { वापस keyword().isString(); }
    bool isZeroInit() स्थिर { वापस keyword().isZeroInit(); }
    bool isRanged() स्थिर { वापस rangep() || m.m_nrange.ranged(); }
    bool isDpiBitVec() स्थिर {  // DPI uses svBitVecVal
        वापस keyword() == VBasicDTypeKwd::BIT && isRanged();
    }
    bool isDpiLogicVec() स्थिर {  // DPI uses svLogicVecVal
        वापस keyword().isFourstate() && !(keyword() == VBasicDTypeKwd::LOGIC && !isRanged());
    }
    bool isDpiCLayout() स्थिर {
        वापस keyword().isDpiCLayout()
               || (keyword().isSameish(VBasicDTypeKwd::LOGIC) && !isRanged());
    }
    bool isDpiPrimitive() स्थिर {  // DPI uses a primitive type
        वापस !isDpiBitVec() && !isDpiLogicVec();
    }
    // Generally the lo/hi/left/right funcs should be used instead of nrange()
    स्थिर VNumRange& nrange() स्थिर { वापस m.m_nrange; }
    अंतरभूत पूर्णांक hi() स्थिर;
    अंतरभूत पूर्णांक lo() स्थिर;
    अंतरभूत पूर्णांक elements() स्थिर;
    पूर्णांक बायाँ() स्थिर { वापस ascending() ? lo() : hi(); }  // How to show a declaration
    पूर्णांक दायाँ() स्थिर { वापस ascending() ? hi() : lo(); }
    अंतरभूत bool ascending() स्थिर;
    bool implicit() स्थिर { वापस keyword() == VBasicDTypeKwd::LOGIC_IMPLICIT; }
    bool untyped() स्थिर { वापस keyword() == VBasicDTypeKwd::UNTYPED; }
    VNumRange declRange() स्थिर { वापस isRanged() ? VNumRange{बायाँ(), दायाँ()} : VNumRange{}; }
    व्योम cvtRangeConst();  // Convert to smaller representation
    bool isCompound() स्थिर override { वापस isString(); }
    bool isIntegralOrPacked() स्थिर override { वापस keyword().isIntNumeric(); }
};
class AstBracketArrayDType final : खुला Ast_गाँठ_DType {
    // Associative/Queue/Normal array data type, ie "[dtype_or_expr]"
    // only for early parsing then becomes another data type
    // @astgen op1 := childDTypep : Optional[AstNodeDType] // moved to refDTypep() in V3Width
    // @astgen op2 := elementsp : AstNode  // Number of elements in array
खुला:
    AstBracketArrayDType(File_पंक्ति* fl, VFlagChildDType, Ast_गाँठ_DType* childDTypep,
                         Ast_गाँठ* elementsp)
        : ASTGEN_SUPER_BracketArrayDType(fl) {
        this->childDTypep(childDTypep);
        this->elementsp(elementsp);
    }
    ASTGEN_MEMBERS_AstBracketArrayDType;
    bool similar_DType_गाँठ(स्थिर Ast_गाँठ_DType* samep) स्थिर override { वापस this == samep; }
    Ast_गाँठ_DType* subDTypep() स्थिर override VL_MT_STABLE { वापस childDTypep(); }
    // METHODS
    // Will be removed in V3Width, which relies on this
    // being a child not a dtype pointed node
    bool maybePointedTo() स्थिर override VL_MT_SAFE { वापस false; }
    AstBasicDType* basicp() स्थिर override VL_MT_STABLE { वापस nullptr; }
    पूर्णांक widthAlignBytes() स्थिर override { V3ERROR_NA_RETURN(0); }
    पूर्णांक width_योग_Bytes() स्थिर override { V3ERROR_NA_RETURN(0); }
    bool isCompound() स्थिर override { वापस true; }
};
class AstCDType final : खुला Ast_गाँठ_DType {
    // Raw "C" data type passed directly to output
    string m_name;  // Name of data type, printed when do V3EmitC
खुला:
    AstCDType(File_पंक्ति* fl, स्थिर string& name)
        : ASTGEN_SUPER_CDType(fl)
        , m_name{name} {
        dtypep(this);
    }

    ASTGEN_MEMBERS_AstCDType;
    bool same_गाँठ(स्थिर Ast_गाँठ* samep) स्थिर override {
        स्थिर AstCDType* स्थिर asamep = VN_DBG_AS(samep, CDType);
        वापस m_name == asamep->m_name;
    }
    bool similar_DType_गाँठ(स्थिर Ast_गाँठ_DType* samep) स्थिर override { वापस same_गाँठ(samep); }
    string name() स्थिर override VL_MT_STABLE { वापस m_name; }
    string prettyDTypeName(bool) स्थिर override { वापस m_name; }
    // METHODS
    AstBasicDType* basicp() स्थिर override VL_MT_STABLE { वापस nullptr; }
    पूर्णांक widthAlignBytes() स्थिर override { वापस 8; }  // Assume
    पूर्णांक width_योग_Bytes() स्थिर override { वापस 8; }  // Assume
    bool isCompound() स्थिर override { वापस true; }
    // Return the global runtime type storing the requested number of bits.
    अटल string typeToHold(पूर्णांक width) {
        अगर (width <= 8) वापस "::CData";
        अगर (width <= 16) वापस "::SData";
        अगर (width <= VL_IDATASIZE) वापस "::IData";
        अगर (width <= VL_QUADSIZE) वापस "::QData";

        वापस "::VlWide<" + std::to_string(VL_शब्दगण_I(width)) + ">";
    }
};
class AstClassRefDType final : खुला Ast_गाँठ_DType {
    // Reference to a class
    // @astgen op1 := paramsp: List[AstPin]
    //
    // @astgen ptr := m_classp : Optional[AstClass]  // data type pointed to, BELOW the AstTypedef
    // @astgen ptr := m_classOrPackagep : Optional[AstNodeModule]  // Package hierarchy
    bool m_rawPointer = false;  // Emit as a non-owning C++ pointer rather than VlClassRef
खुला:
    AstClassRefDType(File_पंक्ति* fl, AstClass* classp, AstPin* paramsp)
        : ASTGEN_SUPER_ClassRefDType(fl)
        , m_classp{classp} {
        dtypep(this);
        addParamsp(paramsp);
    }
    ASTGEN_MEMBERS_AstClassRefDType;
    // METHODS
    bool same_गाँठ(स्थिर Ast_गाँठ* samep) स्थिर override {
        स्थिर AstClassRefDType* स्थिर asamep = VN_DBG_AS(samep, ClassRefDType);
        वापस (m_classp == asamep->m_classp && m_classOrPackagep == asamep->m_classOrPackagep
                && m_rawPointer == asamep->m_rawPointer);
    }
    bool similar_DType_गाँठ(स्थिर Ast_गाँठ_DType* samep) स्थिर override;
    व्योम dump(std::ostream& str = std::cout) स्थिर override;
    व्योम dumpJson(std::ostream& str = std::cout) स्थिर override;
    व्योम dumpSmall(std::ostream& str) स्थिर override;
    string prettyDTypeName(bool full) स्थिर override;
    string pretty_Name_संदेश() स्थिर override;
    string name() स्थिर override VL_MT_STABLE;
    AstBasicDType* basicp() स्थिर override VL_MT_STABLE { वापस nullptr; }
    पूर्णांक widthAlignBytes() स्थिर override { वापस 0; }
    पूर्णांक width_योग_Bytes() स्थिर override { वापस 0; }
    Ast_गाँठ_DType* virtRefDTypep() स्थिर override { वापस nullptr; }
    व्योम virtRefDTypep(Ast_गाँठ_DType* nodep) override {}
    Ast_गाँठ_DType* subDTypep() स्थिर override VL_MT_STABLE { वापस nullptr; }
    Ast_गाँठ_Module* classOrPackagep() स्थिर { वापस m_classOrPackagep; }
    व्योम classOrPackagep(Ast_गाँठ_Module* nodep) { m_classOrPackagep = nodep; }
    AstClass* classp() स्थिर VL_MT_STABLE { वापस m_classp; }
    व्योम classp(AstClass* nodep) { m_classp = nodep; }
    bool rawPointer() स्थिर { वापस m_rawPointer; }
    व्योम rawPointer(bool flag) { m_rawPointer = flag; }
    अटल व्योम selfTest();
    bool isCompound() स्थिर override { वापस true; }
};
class AstConstDType final : खुला Ast_गाँठ_DType {
    // const data type, ie "const some_dtype var_name [2:0]"
    // ConstDType are removed in V3LinkLValue and become AstVar::isConst.
    // When more generic types are supported AstConstDType will be propagated further.
    // @astgen op1 := childDTypep : Optional[AstNodeDType]
    //
    // @astgen ptr := m_refDTypep : Optional[AstNodeDType]  // Inherit from this base data type
खुला:
    AstConstDType(File_पंक्ति* fl, VFlagChildDType, Ast_गाँठ_DType* dtp)
        : ASTGEN_SUPER_ConstDType(fl) {
        childDTypep(dtp);  // Only for parser
        refDTypep(nullptr);  // V3Width will resolve
        dtypep(nullptr);  // V3Width will resolve
        widthFromSub(subDTypep());
    }
    ASTGEN_MEMBERS_AstConstDType;
    स्थिर अक्षर* broken() स्थिर override {
        BROKEN_RTN(!((m_refDTypep && !childDTypep()) || (!m_refDTypep && childDTypep())));
        वापस nullptr;
    }
    bool same_गाँठ(स्थिर Ast_गाँठ* samep) स्थिर override {
        स्थिर AstConstDType* स्थिर sp = VN_DBG_AS(samep, ConstDType);
        वापस (m_refDTypep == sp->m_refDTypep);
    }
    bool similar_DType_गाँठ(स्थिर Ast_गाँठ_DType* samep) स्थिर override {
        वापस skipRefp()->similarDType(samep->skipRefp());
    }
    Ast_गाँठ_DType* getChildDTypep() स्थिर override { वापस childDTypep(); }
    Ast_गाँठ_DType* subDTypep() स्थिर override VL_MT_STABLE {
        वापस m_refDTypep ? m_refDTypep : childDTypep();
    }
    व्योम refDTypep(Ast_गाँठ_DType* nodep) { m_refDTypep = nodep; }
    Ast_गाँठ_DType* virtRefDTypep() स्थिर override { वापस m_refDTypep; }
    व्योम virtRefDTypep(Ast_गाँठ_DType* nodep) override { refDTypep(nodep); }
    // METHODS
    AstBasicDType* basicp() स्थिर override VL_MT_STABLE { वापस subDTypep()->basicp(); }
    पूर्णांक widthAlignBytes() स्थिर override { वापस subDTypep()->widthAlignBytes(); }
    पूर्णांक width_योग_Bytes() स्थिर override { वापस subDTypep()->width_योग_Bytes(); }
    bool isCompound() स्थिर override {
        v3fatalSrc("call isCompound on subdata type, not reference");
        वापस false;
    }
};
class AstConstraintRefDType final : खुला Ast_गाँठ_DType {
    // For e.g. a reference to constraint for constraint_mode
खुला:
    explicit AstConstraintRefDType(File_पंक्ति* fl)
        : ASTGEN_SUPER_ConstraintRefDType(fl) {
        dtypep(this);
    }
    ASTGEN_MEMBERS_AstConstraintRefDType;
    bool hasDType() स्थिर override VL_MT_SAFE { वापस true; }
    bool maybePointedTo() स्थिर override VL_MT_SAFE { वापस true; }
    bool undead() स्थिर override { वापस true; }
    Ast_गाँठ_DType* subDTypep() स्थिर override VL_MT_STABLE { वापस nullptr; }
    Ast_गाँठ_DType* virtRefDTypep() स्थिर override { वापस nullptr; }
    व्योम virtRefDTypep(Ast_गाँठ_DType* nodep) override {}
    bool similar_DType_गाँठ(स्थिर Ast_गाँठ_DType* samep) स्थिर override { वापस this == samep; }
    AstBasicDType* basicp() स्थिर override VL_MT_STABLE { वापस nullptr; }
    पूर्णांक widthAlignBytes() स्थिर override { वापस 1; }
    पूर्णांक width_योग_Bytes() स्थिर override { वापस 1; }
    bool isCompound() स्थिर override { वापस false; }
};
class AstCoverCrossDType final : खुला Ast_गाँठ_DType {
    // Borrowed pointer to VlCoverCrossT<...> or construction-time VlCoverCrossDyn.
    स्थिर uint32_t m_dimensions;
    स्थिर uint32_t m_tuples;  // Fixed capacity number of cross tuples (VlCoverCrossT's Tuples)
    स्थिर uint32_t m_bins;  // Fixed capacity number of explicit bins (VlCoverCrossT's Bins)
    स्थिर uint32_t m_autoBins;  // Fixed capacity number of auto bins (VlCoverCrossT's AutoBins)
    स्थिर uint64_t m_bin_शब्दगण;  // Fixed capacity selection words (VlCoverCrossT's BinWords)
    स्थिर bool m_dynamic;  // Bin layout is determined at covergroup construction

खुला:
    AstCoverCrossDType(File_पंक्ति* fl, uint32_t dimensions, uint32_t tuples, uint32_t bins,
                       uint32_t autoBins, uint64_t bin_शब्दगण, bool dynamic = false)
        : ASTGEN_SUPER_CoverCrossDType(fl)
        , m_dimensions{dimensions}
        , m_tuples{tuples}
        , m_bins{bins}
        , m_autoBins{autoBins}
        , m_bin_शब्दगण{bin_शब्दगण}
        , m_dynamic{dynamic} {
        dtypep(this);
    }
    ASTGEN_MEMBERS_AstCoverCrossDType;
    स्थिर अक्षर* broken() स्थिर override {
        BROKEN_RTN(m_dimensions == 0);
        वापस nullptr;
    }
    bool same_गाँठ(स्थिर Ast_गाँठ* samep) स्थिर override {  // LCOV_EXCL_START
        स्थिर AstCoverCrossDType* स्थिर sp = VN_DBG_AS(samep, CoverCrossDType);
        वापस dimensions() == sp->dimensions() && tuples() == sp->tuples() && bins() == sp->bins()
               && autoBins() == sp->autoBins() && bin_शब्दगण() == sp->bin_शब्दगण()
               && isDynamic() == sp->isDynamic();
    }  // LCOV_EXCL_STOP
    bool similar_DType_गाँठ(स्थिर Ast_गाँठ_DType* samep) स्थिर override { वापस this == samep; }
    व्योम dump(std::ostream& str) स्थिर override;
    व्योम dumpJson(std::ostream& str) स्थिर override;
    व्योम dumpSmall(std::ostream& str) स्थिर override;
    uint32_t dimensions() स्थिर { वापस m_dimensions; }
    uint32_t tuples() स्थिर { वापस m_tuples; }
    uint32_t bins() स्थिर { वापस m_bins; }
    uint32_t autoBins() स्थिर { वापस m_autoBins; }
    uint64_t bin_शब्दगण() स्थिर { वापस m_bin_शब्दगण; }
    bool isDynamic() स्थिर { वापस m_dynamic; }
    string cppTemplateArgs() स्थिर;
    AstBasicDType* basicp() स्थिर override VL_MT_STABLE { वापस nullptr; }
    पूर्णांक widthAlignBytes() स्थिर override { वापस माप(व्योम*); }
    पूर्णांक width_योग_Bytes() स्थिर override { वापस माप(व्योम*); }
    bool isCompound() स्थिर override { वापस true; }
};
class AstCoverpointDType final : खुला Ast_गाँठ_DType {
    // Borrowed pointer to a covergroup coverpoint runtime, 'VlCoverpointT<hitBound>*'.
    // Follows pattern of AstQueueDType in capturing the compile-time max bin overlap
    // template argument.
    uint32_t m_hitBound;  // VlCoverpointT<> template argument; hit list size, >= 1
खुला:
    AstCoverpointDType(File_पंक्ति* fl, uint32_t hitBound)
        : ASTGEN_SUPER_CoverpointDType(fl)
        , m_hitBound{hitBound} {
        dtypep(this);
    }
    ASTGEN_MEMBERS_AstCoverpointDType;
    स्थिर अक्षर* broken() स्थिर override {
        BROKEN_RTN(m_hitBound < 1);
        वापस nullptr;
    }
    // V3Covergroup interns these one-per-hitBound into the type table, so identity is
    // equality; there is never a second node with the same bound to compare against.
    bool similar_DType_गाँठ(स्थिर Ast_गाँठ_DType* samep) स्थिर override { वापस this == samep; }
    व्योम dumpSmall(std::ostream& str) स्थिर override;
    // ACCESSORS
    uint32_t hitBound() स्थिर { वापस m_hitBound; }
    // METHODS
    AstBasicDType* basicp() स्थिर override VL_MT_STABLE { वापस nullptr; }
    पूर्णांक widthAlignBytes() स्थिर override { वापस माप(व्योम*); }
    पूर्णांक width_योग_Bytes() स्थिर override { वापस माप(व्योम*); }
    bool isCompound() स्थिर override { वापस true; }
};
class AstDefImplicitDType final : खुला Ast_गाँठ_DType {
    // For parsing enum/struct/unions that are declared with a variable rather than typedef
    // This allows "var enum {...} a,b" to share the enum definition for both variables
    // After link, these become typedefs
    // @astgen op1 := childDTypep : Optional[AstNodeDType]
    string m_name;  // Data type name
    स्थिर पूर्णांक m_unique_अंक;  // Unique ID distinguishing this dtype instance, for hashing/naming

खुला:
    AstDefImplicitDType(File_पंक्ति* fl, स्थिर string& name, VFlagChildDType, Ast_गाँठ_DType* dtp)
        : ASTGEN_SUPER_DefImplicitDType(fl)
        , m_name{name}
        , m_unique_अंक{unique_अंक_Inc()} {
        childDTypep(dtp);  // Only for parser
        dtypep(nullptr);  // V3Width will resolve
    }
    AstDefImplicitDType(स्थिर AstDefImplicitDType& other)
        : Ast_गाँठ_DType(other)
        , m_name{other.m_name}
        , m_unique_अंक{unique_अंक_Inc()} {}
    ASTGEN_MEMBERS_AstDefImplicitDType;
    पूर्णांक unique_अंक() स्थिर { वापस m_unique_अंक; }
    bool same_गाँठ(स्थिर Ast_गाँठ* samep) स्थिर override {
        स्थिर AstDefImplicitDType* स्थिर sp = VN_DBG_AS(samep, DefImplicitDType);
        वापस unique_अंक() == sp->unique_अंक();
    }
    bool similar_DType_गाँठ(स्थिर Ast_गाँठ_DType* samep) स्थिर override { वापस same_गाँठ(samep); }
    Ast_गाँठ_DType* getChildDTypep() स्थिर override { वापस childDTypep(); }
    Ast_गाँठ_DType* subDTypep() स्थिर override VL_MT_STABLE {
        वापस dtypep() ? dtypep() : childDTypep();
    }
    // METHODS
    // op1 = Range of variable
    Ast_गाँठ_DType* dtypeSkipRefp() स्थिर { वापस dtypep()->skipRefp(); }
    AstBasicDType* basicp() स्थिर override VL_MT_STABLE { वापस subDTypep()->basicp(); }
    पूर्णांक widthAlignBytes() स्थिर override { वापस dtypep()->widthAlignBytes(); }
    पूर्णांक width_योग_Bytes() स्थिर override { वापस dtypep()->width_योग_Bytes(); }
    string name() स्थिर override VL_MT_STABLE { वापस m_name; }
    व्योम name(स्थिर string& flag) override { m_name = flag; }
    bool isCompound() स्थिर override { वापस false; }
};
class AstDynArrayDType final : खुला Ast_गाँठ_DType {
    // Dynamic array data type, ie "[]"
    // @astgen op1 := childDTypep : Optional[AstNodeDType] // moved to refDTypep() in V3Width
    //
    // @astgen ptr := m_refDTypep : Optional[AstNodeDType]  // Elements of this type (post-width)
खुला:
    AstDynArrayDType(File_पंक्ति* fl, VFlagChildDType, Ast_गाँठ_DType* dtp)
        : ASTGEN_SUPER_DynArrayDType(fl) {
        childDTypep(dtp);  // Only for parser
        refDTypep(nullptr);
        dtypep(nullptr);  // V3Width will resolve
    }
    AstDynArrayDType(File_पंक्ति* fl, Ast_गाँठ_DType* dtp)
        : ASTGEN_SUPER_DynArrayDType(fl) {
        refDTypep(dtp);
        dtypep(nullptr);  // V3Width will resolve
    }
    ASTGEN_MEMBERS_AstDynArrayDType;
    स्थिर अक्षर* broken() स्थिर override {
        BROKEN_RTN(!((m_refDTypep && !childDTypep()) || (!m_refDTypep && childDTypep())));
        वापस nullptr;
    }
    bool same_गाँठ(स्थिर Ast_गाँठ* samep) स्थिर override {
        स्थिर AstDynArrayDType* स्थिर asamep = VN_DBG_AS(samep, DynArrayDType);
        अगर (!asamep->subDTypep()) वापस false;
        वापस subDTypep() == asamep->subDTypep();
    }
    bool similar_DType_गाँठ(स्थिर Ast_गाँठ_DType* samep) स्थिर override {
        स्थिर AstDynArrayDType* स्थिर asamep = VN_DBG_AS(samep, DynArrayDType);
        वापस asamep->subDTypep() && subDTypep()->similarDType(asamep->subDTypep());
    }
    string prettyDTypeName(bool full) स्थिर override;
    व्योम dumpSmall(std::ostream& str) स्थिर override;
    Ast_गाँठ_DType* getChildDTypep() स्थिर override { वापस childDTypep(); }
    Ast_गाँठ_DType* subDTypep() स्थिर override VL_MT_STABLE {
        वापस m_refDTypep ? m_refDTypep : childDTypep();
    }
    bool isAggregateType() स्थिर override { वापस true; }
    व्योम refDTypep(Ast_गाँठ_DType* nodep) { m_refDTypep = nodep; }
    Ast_गाँठ_DType* virtRefDTypep() स्थिर override { वापस m_refDTypep; }
    व्योम virtRefDTypep(Ast_गाँठ_DType* nodep) override { refDTypep(nodep); }
    // METHODS
    AstBasicDType* basicp() स्थिर override VL_MT_STABLE { वापस nullptr; }
    पूर्णांक widthAlignBytes() स्थिर override { वापस subDTypep()->widthAlignBytes(); }
    पूर्णांक width_योग_Bytes() स्थिर override { वापस subDTypep()->width_योग_Bytes(); }
    bool isCompound() स्थिर override { वापस true; }
    bool isDynamicallySized() स्थिर override { वापस true; }
};
class AstEmptyQueueDType final : खुला Ast_गाँठ_DType {
    // For EmptyQueue
खुला:
    explicit AstEmptyQueueDType(File_पंक्ति* fl)
        : ASTGEN_SUPER_EmptyQueueDType(fl) {
        dtypep(this);
    }
    ASTGEN_MEMBERS_AstEmptyQueueDType;
    व्योम dumpSmall(std::ostream& str) स्थिर override;
    bool hasDType() स्थिर override VL_MT_SAFE { वापस true; }
    bool maybePointedTo() स्थिर override VL_MT_SAFE { वापस true; }
    bool undead() स्थिर override { वापस true; }
    Ast_गाँठ_DType* subDTypep() स्थिर override VL_MT_STABLE { वापस nullptr; }
    Ast_गाँठ_DType* virtRefDTypep() स्थिर override { वापस nullptr; }
    व्योम virtRefDTypep(Ast_गाँठ_DType* nodep) override {}
    bool similar_DType_गाँठ(स्थिर Ast_गाँठ_DType* samep) स्थिर override { वापस this == samep; }
    AstBasicDType* basicp() स्थिर override VL_MT_STABLE { वापस nullptr; }
    पूर्णांक widthAlignBytes() स्थिर override { वापस 1; }
    पूर्णांक width_योग_Bytes() स्थिर override { वापस 1; }
    bool isCompound() स्थिर override { वापस false; }
};
class AstEnumDType final : खुला Ast_गाँठ_DType {
    // Parents: TYPEDEF/MODULE
    // @astgen op1 := childDTypep : Optional[AstNodeDType]
    // @astgen op2 := itemsp : List[AstEnumItem]
    //
    // @astgen ptr := m_refDTypep : Optional[AstNodeDType]  // Elements of this type (post-width)
    string m_name;  // Name from upper typedef, if any
    string m_typedefName;  // Typedef's dtypeName() once moved off the typedef declaring it
    स्थिर पूर्णांक m_unique_अंक;  // Unique ID distinguishing this dtype instance, for hashing/naming

खुला:
    AstEnumDType(File_पंक्ति* fl, VFlagChildDType, Ast_गाँठ_DType* dtp, AstEnumItem* itemsp)
        : ASTGEN_SUPER_EnumDType(fl)
        , m_unique_अंक{unique_अंक_Inc()} {
        childDTypep(dtp);  // Only for parser
        refDTypep(nullptr);
        addItemsp(itemsp);
        dtypep(nullptr);  // V3Width will resolve
        widthFromSub(subDTypep());
    }
    AstEnumDType(स्थिर AstEnumDType& other)
        : Ast_गाँठ_DType{other}
        , m_name{other.m_name}
        , m_typedefName{other.m_typedefName}
        , m_unique_अंक{unique_अंक_Inc()} {}
    ASTGEN_MEMBERS_AstEnumDType;

    स्थिर अक्षर* broken() स्थिर override;
    पूर्णांक unique_अंक() स्थिर { वापस m_unique_अंक; }
    bool same_गाँठ(स्थिर Ast_गाँठ* samep) स्थिर override {
        स्थिर AstEnumDType* स्थिर sp = VN_DBG_AS(samep, EnumDType);
        वापस unique_अंक() == sp->unique_अंक();
    }
    bool similar_DType_गाँठ(स्थिर Ast_गाँठ_DType* samep) स्थिर override { वापस same_गाँठ(samep); }
    Ast_गाँठ_DType* getChildDTypep() स्थिर override { वापस childDTypep(); }
    Ast_गाँठ_DType* subDTypep() स्थिर override VL_MT_STABLE {
        वापस m_refDTypep ? m_refDTypep : childDTypep();
    }
    व्योम refDTypep(Ast_गाँठ_DType* nodep) { m_refDTypep = nodep; }
    Ast_गाँठ_DType* virtRefDTypep() स्थिर override { वापस m_refDTypep; }
    व्योम virtRefDTypep(Ast_गाँठ_DType* nodep) override { refDTypep(nodep); }
    string name() स्थिर override VL_MT_STABLE { वापस m_name; }
    व्योम name(स्थिर string& flag) override { m_name = flag; }
    // Keep the name the typedef declaring it gives, as it moves off it to the type table
    व्योम typedefName(स्थिर string& name) { m_typedefName = name; }
    व्योम dump(std::ostream& str = std::cout) स्थिर override;
    व्योम dumpJson(std::ostream& str = std::cout) स्थिर override;
    व्योम dumpSmall(std::ostream& str) स्थिर override;
    string prettyDTypeName(bool full) स्थिर override;
    // METHODS
    AstBasicDType* basicp() स्थिर override VL_MT_STABLE { वापस subDTypep()->basicp(); }
    पूर्णांक widthAlignBytes() स्थिर override { वापस subDTypep()->widthAlignBytes(); }
    पूर्णांक width_योग_Bytes() स्थिर override { वापस subDTypep()->width_योग_Bytes(); }
    माप_प्रकार item_गिनती() स्थिर {
        माप_प्रकार गिनती = 0;
        क्रम (Ast_गाँठ* itemp = itemsp(); itemp; itemp = itemp->nextp()) ++गिनती;
        वापस गिनती;
    }
    bool isCompound() स्थिर override { वापस false; }
};

class AstIfaceGenericDType final : खुला Ast_गाँठ_DType {
    // Generic interface that will be replaced with AstIfaceRefDType
    File_पंक्ति* m_modportFileline;  // Where modport token was
    // dist-ast-dump-suppress  // Part of name()
    string m_modportName;  // "" = no modport
खुला:
    explicit AstIfaceGenericDType(File_पंक्ति* fl)
        : ASTGEN_SUPER_IfaceGenericDType(fl) {
        dtypep(this);
    }
    AstIfaceGenericDType(File_पंक्ति* fl, File_पंक्ति* modportFl, स्थिर string& modport)
        : ASTGEN_SUPER_IfaceGenericDType(fl)
        , m_modportFileline{modportFl}
        , m_modportName{modport} {
        dtypep(this);
    }
    ASTGEN_MEMBERS_AstIfaceGenericDType;
    व्योम dumpSmall(std::ostream& str) स्थिर override;
    bool hasDType() स्थिर override VL_MT_SAFE { वापस true; }
    bool maybePointedTo() स्थिर override VL_MT_SAFE { वापस true; }
    bool undead() स्थिर override { वापस true; }
    Ast_गाँठ_DType* subDTypep() स्थिर override VL_MT_STABLE { वापस nullptr; }
    Ast_गाँठ_DType* virtRefDTypep() स्थिर override { वापस nullptr; }
    व्योम virtRefDTypep(Ast_गाँठ_DType* nodep) override {}
    bool similar_DType_गाँठ(स्थिर Ast_गाँठ_DType* samep) स्थिर override { वापस this == samep; }
    AstBasicDType* basicp() स्थिर override VL_MT_STABLE { वापस nullptr; }
    पूर्णांक widthAlignBytes() स्थिर override { वापस 1; }
    पूर्णांक width_योग_Bytes() स्थिर override { वापस 1; }
    string modportName() स्थिर { वापस m_modportName; }
    bool isModport() { वापस !m_modportName.empty(); }
    bool isCompound() स्थिर override { वापस true; }
    File_पंक्ति* modportFileline() स्थिर { वापस m_modportFileline; }
    string name() स्थिर override { वापस m_modportName; }
};

class AstIfaceRefDType final : खुला Ast_गाँठ_DType {
    // Reference to an interface, either for a port, or inside parent cell
    // @astgen op1 := paramsp : List[AstPin]
    //
    // @astgen ptr := m_ifacep : Optional[AstIface]  // Interface; cellp() should override
    // @astgen ptr := m_cellp : Optional[AstCell]  // When exact parent cell known; not a guess
    // @astgen ptr := m_modportp : Optional[AstModport]  // nullptr = unlinked or no modport
    File_पंक्ति* m_modportFileline;  // Where modport token was
    string m_cellName;  // "" = no cell, such as when connects to 'input' iface
    string m_ifaceName;  // Interface name
    string m_modportName;  // "" = no modport
    bool m_virtual = false;  // True if virtual interface
खुला:
    AstIfaceRefDType(File_पंक्ति* fl, स्थिर string& cellName, स्थिर string& ifaceName)
        : ASTGEN_SUPER_IfaceRefDType(fl)
        , m_modportFileline{nullptr}
        , m_cellName{cellName}
        , m_ifaceName{ifaceName}
        , m_modportName{""} {}
    AstIfaceRefDType(File_पंक्ति* fl, File_पंक्ति* modportFl, स्थिर string& cellName,
                     स्थिर string& ifaceName, स्थिर string& modport)
        : ASTGEN_SUPER_IfaceRefDType(fl)
        , m_modportFileline{modportFl}
        , m_cellName{cellName}
        , m_ifaceName{ifaceName}
        , m_modportName{modport} {}
    AstIfaceRefDType(File_पंक्ति* fl, File_पंक्ति* modportFl, स्थिर string& cellName,
                     स्थिर string& ifaceName, स्थिर string& modport, AstPin* paramsp)
        : ASTGEN_SUPER_IfaceRefDType(fl)
        , m_modportFileline{modportFl}
        , m_cellName{cellName}
        , m_ifaceName{ifaceName}
        , m_modportName{modport} {
        addParamsp(paramsp);
    }
    ASTGEN_MEMBERS_AstIfaceRefDType;

    // METHODS
    व्योम dump(std::ostream& str = std::cout) स्थिर override;
    व्योम dumpJson(std::ostream& str = std::cout) स्थिर override;
    व्योम dumpSmall(std::ostream& str) स्थिर override;
    string prettyDTypeName(bool full) स्थिर override;
    AstBasicDType* basicp() स्थिर override VL_MT_STABLE { वापस nullptr; }
    bool similar_DType_गाँठ(स्थिर Ast_गाँठ_DType* samep) स्थिर override {
        // Each occurrence of a virtual interface type parses to its own node,
        // so pointer identity is insufficient; compare the referenced
        // interface instead, same equivalence as AstNode::computeCastable uses.
        स्थिर AstIfaceRefDType* स्थिर asamep = VN_DBG_AS(samep, IfaceRefDType);
        वापस ifaceViaCellp() && ifaceViaCellp() == asamep->ifaceViaCellp()
               && modportName() == asamep->modportName() && isVirtual() == asamep->isVirtual();
    }
    पूर्णांक widthAlignBytes() स्थिर override { वापस 0; }
    पूर्णांक width_योग_Bytes() स्थिर override { वापस 0; }
    bool isVirtual() स्थिर { वापस m_virtual; }
    व्योम isVirtual(bool flag) {
        m_virtual = flag;
        अगर (flag) v3Global.setHasVirtIfaces();
    }
    File_पंक्ति* modportFileline() स्थिर { वापस m_modportFileline; }
    व्योम modportFileline(File_पंक्ति* स्थिर modportFileline) { m_modportFileline = modportFileline; }
    string cellName() स्थिर { वापस m_cellName; }
    व्योम cellName(स्थिर string& name) { m_cellName = name; }
    string ifaceName() स्थिर { वापस m_ifaceName; }
    string ifaceNameQ() स्थिर { वापस "'" + prettyName(ifaceName()) + "'"; }
    व्योम ifaceName(स्थिर string& name) { m_ifaceName = name; }
    व्योम modportName(स्थिर string& modportName) { m_modportName = modportName; }
    string modportName() स्थिर { वापस m_modportName; }
    AstIface* ifaceViaCellp() स्थिर;  // Use cellp or ifacep
    AstIface* ifacep() स्थिर { वापस m_ifacep; }
    व्योम ifacep(AstIface* nodep) { m_ifacep = nodep; }
    AstCell* cellp() स्थिर { वापस m_cellp; }
    व्योम cellp(AstCell* nodep) { m_cellp = nodep; }
    AstModport* modportp() स्थिर { वापस m_modportp; }
    व्योम modportp(AstModport* modportp) { m_modportp = modportp; }
    bool isModport() { वापस !m_modportName.empty(); }
    bool isCompound() स्थिर override { वापस true; }  // But not relevant
};
class AstMemberDType final : खुला Ast_गाँठ_DType {
    // A member of a struct/union
    // PARENT: AstNodeUOrStructDType
    // @astgen op1 := childDTypep : Optional[AstNodeDType]
    // @astgen op3 := valuep : Optional[AstNode]
    //
    // @astgen ptr := m_refDTypep : Optional[AstNodeDType]  // Elements of this type (post-width)
    string m_name;  // Name of variable
    string m_tag;  // Holds the string of the verilator tag -- used in JSON output.
    पूर्णांक m_lsb = -1;  // Within this level's packed struct, the LSB of the first bit of the member
    bool m_constrainedRand = false;  // Member has a constraint expression
    VRandAttr m_rand;  // Randomizability of this member (rand, randc, etc)
खुला:
    AstMemberDType(File_पंक्ति* fl, स्थिर string& name, VFlagChildDType, Ast_गाँठ_DType* dtp,
                   Ast_गाँठ* valuep)
        : ASTGEN_SUPER_MemberDType(fl)
        , m_name{name} {
        childDTypep(dtp);  // Only for parser
        this->valuep(valuep);
        dtypep(nullptr);  // V3Width will resolve
        refDTypep(nullptr);
    }
    AstMemberDType(File_पंक्ति* fl, स्थिर string& name, Ast_गाँठ_DType* dtp)
        : ASTGEN_SUPER_MemberDType(fl)
        , m_name{name} {
        UASSERT(dtp, "AstMember created with no dtype");
        refDTypep(dtp);
        dtypep(this);
        widthFromSub(subDTypep());
    }
    ASTGEN_MEMBERS_AstMemberDType;
    व्योम dump(std::ostream& str = std::cout) स्थिर override;
    व्योम dumpJson(std::ostream& str = std::cout) स्थिर override;
    व्योम dumpSmall(std::ostream& str) स्थिर override;
    string prettyDTypeName(bool full) स्थिर override;
    string name() स्थिर override VL_MT_STABLE { वापस m_name; }  // * = Var name
    bool hasDType() स्थिर override VL_MT_SAFE { वापस true; }
    bool maybePointedTo() स्थिर override VL_MT_SAFE { वापस true; }
    Ast_गाँठ_DType* getChildDTypep() स्थिर override { वापस childDTypep(); }
    Ast_गाँठ_UOr_Struct_DType* getChildStructp();
    Ast_गाँठ_DType* subDTypep() स्थिर override VL_MT_STABLE {
        वापस m_refDTypep ? m_refDTypep : childDTypep();
    }
    व्योम refDTypep(Ast_गाँठ_DType* nodep) { m_refDTypep = nodep; }
    Ast_गाँठ_DType* virtRefDTypep() स्थिर override { वापस m_refDTypep; }
    व्योम virtRefDTypep(Ast_गाँठ_DType* nodep) override { refDTypep(nodep); }
    bool similar_DType_गाँठ(स्थिर Ast_गाँठ_DType* samep) स्थिर override { वापस this == samep; }
    //
    // (Slow) recurse down to find basic data type (Note don't need virtual -
    // AstVar isn't a NodeDType)
    AstBasicDType* basicp() स्थिर override VL_MT_STABLE { वापस subDTypep()->basicp(); }
    // op1 = Range of variable (Note don't need virtual - AstVar isn't a NodeDType)
    Ast_गाँठ_DType* dtypeSkipRefp() स्थिर { वापस subDTypep()->skipRefp(); }
    // (Slow) recurses - Structure alignment 1,2,4 or 8 bytes (arrays affect this)
    पूर्णांक widthAlignBytes() स्थिर override { वापस subDTypep()->widthAlignBytes(); }
    // (Slow) recurses - Width in bytes rounding up 1,2,4,8,12,...
    पूर्णांक width_योग_Bytes() स्थिर override { वापस subDTypep()->width_योग_Bytes(); }
    // METHODS
    व्योम name(स्थिर string& name) override { m_name = name; }
    string tag() स्थिर override { वापस m_tag; }
    व्योम tag(स्थिर string& text) override { m_tag = text; }
    पूर्णांक lsb() स्थिर { वापस m_lsb; }
    व्योम lsb(पूर्णांक lsb) { m_lsb = lsb; }
    bool isCompound() स्थिर override {
        v3fatalSrc("call isCompound on subdata type, not reference");
        वापस false;
    }
    bool isConstrainedRand() स्थिर { वापस m_constrainedRand; }
    व्योम markConstrainedRand(bool flag) { m_constrainedRand = flag; }
    VRandAttr अक्रम() स्थिर { वापस m_rand; }
    व्योम अक्रम(स्थिर VRandAttr flag) { m_rand = flag; }
};
class AstNBACommitQueueDType final : खुला Ast_गाँठ_DType {
    // @astgen ptr := m_subDTypep : AstNodeDType  // Type of the corresponding variable
    स्थिर bool m_partial;  // Partial element update required

खुला:
    AstNBACommitQueueDType(File_पंक्ति* fl, Ast_गाँठ_DType* subDTypep, bool partial)
        : ASTGEN_SUPER_NBACommitQueueDType(fl)
        , m_partial{partial}
        , m_subDTypep{subDTypep} {
        dtypep(this);
    }
    ASTGEN_MEMBERS_AstNBACommitQueueDType;
    व्योम dump(std::ostream& str) स्थिर override;
    व्योम dumpJson(std::ostream& str) स्थिर override;
    Ast_गाँठ_DType* subDTypep() स्थिर override VL_MT_STABLE { वापस m_subDTypep; }
    bool partial() स्थिर { वापस m_partial; }
    bool same_गाँठ(स्थिर Ast_गाँठ* samep) स्थिर override {
        स्थिर AstNBACommitQueueDType* स्थिर asamep = VN_DBG_AS(samep, NBACommitQueueDType);
        वापस m_partial == asamep->m_partial;
    }
    bool similar_DType_गाँठ(स्थिर Ast_गाँठ_DType* samep) स्थिर override { वापस this == samep; }
    AstBasicDType* basicp() स्थिर override VL_MT_STABLE { वापस nullptr; }
    पूर्णांक widthAlignBytes() स्थिर override { वापस 1; }
    पूर्णांक width_योग_Bytes() स्थिर override { वापस 24; }
    bool isCompound() स्थिर override { वापस true; }
};
class AstParamTypeDType final : खुला Ast_गाँठ_DType {
    // Parents: MODULE
    // A parameter type statement; much like a var or typedef
    // @astgen op1 := childDTypep : Optional[AstNodeDType]
    स्थिर VVarType m_varType;  // Type of variable (for localparam vs. param)
    स्थिर VFwdType m_fwdType;  // Forward type for lint check
    string m_name;  // Name of variable
खुला:
    AstParamTypeDType(File_पंक्ति* fl, VVarType type, VFwdType fwdType, स्थिर string& name,
                      VFlagChildDType, Ast_गाँठ_DType* dtp)
        : ASTGEN_SUPER_ParamTypeDType(fl)
        , m_varType{type}
        , m_fwdType{fwdType}
        , m_name{name} {
        childDTypep(dtp);  // Only for parser
        dtypep(nullptr);  // V3Width will resolve
    }
    ASTGEN_MEMBERS_AstParamTypeDType;
    व्योम dump(std::ostream& str = std::cout) स्थिर override;
    व्योम dumpJson(std::ostream& str = std::cout) स्थिर override;
    string prettyDTypeName(bool full) स्थिर override;
    Ast_गाँठ_DType* getChildDTypep() स्थिर override { वापस childDTypep(); }
    Ast_गाँठ_DType* subDTypep() स्थिर override VL_MT_STABLE {
        वापस dtypep() ? dtypep() : childDTypep();
    }
    AstBasicDType* basicp() स्थिर override VL_MT_STABLE { वापस subDTypep()->basicp(); }
    bool similar_DType_गाँठ(स्थिर Ast_गाँठ_DType* samep) स्थिर override {
        स्थिर AstParamTypeDType* स्थिर sp = VN_DBG_AS(samep, ParamTypeDType);
        वापस this->subDTypep()->similarDType(sp->subDTypep());
    }
    पूर्णांक widthAlignBytes() स्थिर override { वापस dtypep()->widthAlignBytes(); }
    पूर्णांक width_योग_Bytes() स्थिर override { वापस dtypep()->width_योग_Bytes(); }
    // METHODS
    string name() स्थिर override VL_MT_STABLE { वापस m_name; }
    bool maybePointedTo() स्थिर override VL_MT_SAFE { वापस true; }
    bool hasDType() स्थिर override VL_MT_SAFE { वापस true; }
    व्योम name(स्थिर string& flag) override { m_name = flag; }
    VVarType varType() स्थिर { वापस m_varType; }  // * = Type of variable
    VFwdType fwdType() स्थिर { वापस m_fwdType; }
    bool isParam() स्थिर { वापस true; }
    bool isGParam() स्थिर { वापस (varType() == VVarType::GPARAM); }
    bool isCompound() स्थिर override {
        v3fatalSrc("call isCompound on subdata type, not reference");
        वापस false;
    }
};
class AstParseTypeDType final : खुला Ast_गाँठ_DType {
    // Parents: VAR
    // During parsing, this indicates the type of a parameter is a "parameter type"
    // e.g. the data type is a container of any data type
    स्थिर VFwdType m_fwdType;  // Forward type for lint check
खुला:
    explicit AstParseTypeDType(File_पंक्ति* fl, VFwdType fwdType = VFwdType::NONE)
        : ASTGEN_SUPER_ParseTypeDType(fl)
        , m_fwdType{fwdType} {}
    ASTGEN_MEMBERS_AstParseTypeDType;
    Ast_गाँठ_DType* dtypep() स्थिर VL_MT_STABLE { वापस nullptr; }
    // METHODS
    व्योम dump(std::ostream& str = std::cout) स्थिर override;
    व्योम dumpJson(std::ostream& str = std::cout) स्थिर override;
    bool similar_DType_गाँठ(स्थिर Ast_गाँठ_DType* samep) स्थिर override { वापस this == samep; }
    AstBasicDType* basicp() स्थिर override VL_MT_STABLE { वापस nullptr; }
    पूर्णांक widthAlignBytes() स्थिर override { वापस 0; }
    पूर्णांक width_योग_Bytes() स्थिर override { वापस 0; }
    bool isCompound() स्थिर override {
        v3fatalSrc("call isCompound on subdata type, not reference");
        वापस false;
    }
    VFwdType fwdType() स्थिर { वापस m_fwdType; }
};
class AstQueueDType final : खुला Ast_गाँठ_DType {
    // Queue array data type, ie "[ $ ]"
    // @astgen op1 := childDTypep : Optional[AstNodeDType] // moved to refDTypep() in V3Width
    // @astgen op2 := boundp : Optional[AstNodeExpr]
    //
    // @astgen ptr := m_refDTypep : Optional[AstNodeDType]  // Elements of this type (post-width)
खुला:
    AstQueueDType(File_पंक्ति* fl, VFlagChildDType, Ast_गाँठ_DType* dtp, Ast_गाँठ_व्यंजक* boundp)
        : ASTGEN_SUPER_QueueDType(fl) {
        childDTypep(dtp);
        this->boundp(boundp);
        refDTypep(nullptr);
        dtypep(nullptr);  // V3Width will resolve
    }
    AstQueueDType(File_पंक्ति* fl, Ast_गाँठ_DType* dtp, Ast_गाँठ_व्यंजक* boundp)
        : ASTGEN_SUPER_QueueDType(fl) {
        this->boundp(boundp);
        refDTypep(dtp);
        dtypep(dtp);
    }
    ASTGEN_MEMBERS_AstQueueDType;
    स्थिर अक्षर* broken() स्थिर override {
        BROKEN_RTN(!((m_refDTypep && !childDTypep()) || (!m_refDTypep && childDTypep())));
        वापस nullptr;
    }
    bool same_गाँठ(स्थिर Ast_गाँठ* samep) स्थिर override {
        स्थिर AstQueueDType* स्थिर asamep = VN_DBG_AS(samep, QueueDType);
        अगर (!asamep->subDTypep()) वापस false;
        वापस (subDTypep() == asamep->subDTypep());
    }
    bool similar_DType_गाँठ(स्थिर Ast_गाँठ_DType* samep) स्थिर override {
        स्थिर AstQueueDType* स्थिर asamep = VN_DBG_AS(samep, QueueDType);
        वापस asamep->subDTypep() && subDTypep()->similarDType(asamep->subDTypep());
    }
    व्योम dumpSmall(std::ostream& str) स्थिर override;
    string prettyDTypeName(bool full) स्थिर override;
    Ast_गाँठ_DType* getChildDTypep() स्थिर override { वापस childDTypep(); }
    Ast_गाँठ_DType* subDTypep() स्थिर override VL_MT_STABLE {
        वापस m_refDTypep ? m_refDTypep : childDTypep();
    }
    bool isAggregateType() स्थिर override { वापस true; }
    व्योम refDTypep(Ast_गाँठ_DType* nodep) { m_refDTypep = nodep; }
    अंतरभूत पूर्णांक boundConst() स्थिर VL_MT_STABLE;
    Ast_गाँठ_DType* virtRefDTypep() स्थिर override { वापस m_refDTypep; }
    व्योम virtRefDTypep(Ast_गाँठ_DType* nodep) override { refDTypep(nodep); }
    // METHODS
    AstBasicDType* basicp() स्थिर override VL_MT_STABLE { वापस nullptr; }
    पूर्णांक widthAlignBytes() स्थिर override { वापस subDTypep()->widthAlignBytes(); }
    पूर्णांक width_योग_Bytes() स्थिर override { वापस subDTypep()->width_योग_Bytes(); }
    bool isCompound() स्थिर override { वापस true; }
    bool isDynamicallySized() स्थिर override { वापस true; }
};
class AstRefDType final : खुला Ast_गाँठ_DType {
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
    स्थिर VIfaceCaptureTag* m_captureTagp = nullptr;

खुला:
    AstRefDType(File_पंक्ति* fl, स्थिर string& name)
        : ASTGEN_SUPER_RefDType(fl)
        , m_name{name} {}
    AstRefDType(File_पंक्ति* fl, स्थिर string& name, Ast_गाँठ_व्यंजक* classOrPackagep, AstPin* paramsp)
        : ASTGEN_SUPER_RefDType(fl)
        , m_name{name} {
        classOrPackageOpp(classOrPackagep);
        addParamsp(paramsp);
    }
    class Flag_Type_Of_व्यंजक {};  // type(expr) for parser only
    AstRefDType(File_पंक्ति* fl, Flag_Type_Of_व्यंजक, Ast_गाँठ* typeofp)
        : ASTGEN_SUPER_RefDType(fl) {
        this->typeofp(typeofp);
        अगर (Ast_गाँठ_DType* स्थिर dtp = VN_CAST(typeofp, गाँठ_DType)) refDTypep(dtp);
    }
    ASTGEN_MEMBERS_AstRefDType;
    // METHODS
    bool same_गाँठ(स्थिर Ast_गाँठ* samep) स्थिर override {
        स्थिर AstRefDType* स्थिर asamep = VN_DBG_AS(samep, RefDType);
        वापस (m_typedefp == asamep->m_typedefp && m_refDTypep == asamep->m_refDTypep
                && m_name == asamep->m_name && m_classOrPackagep == asamep->m_classOrPackagep);
    }
    bool similar_DType_गाँठ(स्थिर Ast_गाँठ_DType* samep) स्थिर override {
        वापस subDTypep()->similarDType(samep->subDTypep());
    }
    स्थिर अक्षर* broken() स्थिर override;
    व्योम dump(std::ostream& str = std::cout) स्थिर override;
    व्योम dumpJson(std::ostream& str = std::cout) स्थिर override;
    व्योम dumpSmall(std::ostream& str) स्थिर override;
    string name() स्थिर override VL_MT_STABLE { वापस m_name; }
    string prettyDTypeName(bool full) स्थिर override {
        वापस subDTypep() ? subDTypep()->prettyDTypeName(full) : prettyName();
    }
    AstBasicDType* basicp() स्थिर override VL_MT_STABLE {
        वापस subDTypep() ? subDTypep()->basicp() : nullptr;
    }
    Ast_गाँठ_DType* subDTypep() स्थिर override VL_MT_STABLE;
    Ast_गाँठ_DType* getChildDTypep() स्थिर override { वापस VN_CAST(typeofp(), गाँठ_DType); }
    पूर्णांक widthAlignBytes() स्थिर override { वापस dtypeSkipRefp()->widthAlignBytes(); }
    पूर्णांक width_योग_Bytes() स्थिर override { वापस dtypeSkipRefp()->width_योग_Bytes(); }
    व्योम name(स्थिर string& flag) override { m_name = flag; }
    Ast_गाँठ_DType* dtypeSkipRefp() स्थिर { वापस subDTypep()->skipRefp(); }
    AstTypedef* typedefp() स्थिर VL_MT_SAFE { वापस m_typedefp; }
    व्योम typedefp(AstTypedef* nodep) { m_typedefp = nodep; }
    Ast_गाँठ_DType* refDTypep() स्थिर VL_MT_SAFE { वापस m_refDTypep; }
    व्योम refDTypep(Ast_गाँठ_DType* nodep) { m_refDTypep = nodep; }
    Ast_गाँठ_DType* virtRefDTypep() स्थिर override { वापस refDTypep(); }
    व्योम virtRefDTypep(Ast_गाँठ_DType* nodep) override { refDTypep(nodep); }
    Ast_गाँठ_Module* classOrPackagep() स्थिर { वापस m_classOrPackagep; }
    व्योम classOrPackagep(Ast_गाँठ_Module* nodep) { m_classOrPackagep = nodep; }
    स्थिर VIfaceCaptureTag* captureTagp() स्थिर { वापस m_captureTagp; }
    व्योम captureTagp(स्थिर VIfaceCaptureTag* tagp) { m_captureTagp = tagp; }
    bool isCompound() स्थिर override {
        v3fatalSrc("call isCompound on subdata type, not reference");
        वापस false;
    }
};
class AstRequireDType final : खुला Ast_गाँठ_DType {
    // @astgen op1 := lhsp : Optional[AstNode<AstNodeExpr|AstNodeDType>]
    //
    // Require a generic node type (typically AstParseRef become a type.
खुला:
    AstRequireDType(File_पंक्ति* fl, Ast_गाँठ* lhsp)
        : ASTGEN_SUPER_RequireDType(fl) {
        this->lhsp(lhsp);
    }
    ASTGEN_MEMBERS_AstRequireDType;
    // METHODS
    bool similar_DType_गाँठ(स्थिर Ast_गाँठ_DType* samep) स्थिर override {
        स्थिर AstRequireDType* स्थिर asamep = VN_DBG_AS(samep, RequireDType);
        वापस subDTypep()->similarDType(asamep->subDTypep());
    }
    AstBasicDType* basicp() स्थिर override VL_MT_STABLE { वापस nullptr; }
    Ast_गाँठ_DType* subDTypep() स्थिर override VL_MT_STABLE {
        // Used for recursive definition checking
        अगर (Ast_गाँठ_DType* स्थिर dtp = VN_CAST(lhsp(), गाँठ_DType)) वापस dtp;

        वापस nullptr;
    }
    पूर्णांक widthAlignBytes() स्थिर override { V3ERROR_NA_RETURN(1); }
    पूर्णांक width_योग_Bytes() स्थिर override { V3ERROR_NA_RETURN(1); }
    bool isCompound() स्थिर override { V3ERROR_NA_RETURN(false); }
};
class AstSampleQueueDType final : खुला Ast_गाँठ_DType {
    // @astgen op1 := childDTypep : Optional[AstNodeDType] // moved to refDTypep() in V3Width
    //
    // @astgen ptr := m_refDTypep : Optional[AstNodeDType]  // Elements of this type (post-width)
खुला:
    AstSampleQueueDType(File_पंक्ति* fl, Ast_गाँठ_DType* dtp)
        : ASTGEN_SUPER_SampleQueueDType(fl) {
        refDTypep(dtp);
        dtypep(dtp);
    }
    ASTGEN_MEMBERS_AstSampleQueueDType;
    स्थिर अक्षर* broken() स्थिर override {
        BROKEN_RTN(!((m_refDTypep && !childDTypep()) || (!m_refDTypep && childDTypep())));
        वापस nullptr;
    }
    bool same_गाँठ(स्थिर Ast_गाँठ* samep) स्थिर override {
        स्थिर AstSampleQueueDType* स्थिर asamep = VN_DBG_AS(samep, SampleQueueDType);
        अगर (!asamep->subDTypep()) वापस false;
        वापस (subDTypep() == asamep->subDTypep());
    }
    bool similar_DType_गाँठ(स्थिर Ast_गाँठ_DType* samep) स्थिर override {
        स्थिर AstSampleQueueDType* स्थिर asamep = VN_DBG_AS(samep, SampleQueueDType);
        वापस asamep->subDTypep() && subDTypep()->similarDType(asamep->subDTypep());
    }
    व्योम dumpSmall(std::ostream& str) स्थिर override;
    Ast_गाँठ_DType* getChildDTypep() स्थिर override { वापस childDTypep(); }
    // op1 = Range of variable
    Ast_गाँठ_DType* subDTypep() स्थिर override VL_MT_STABLE {
        वापस m_refDTypep ? m_refDTypep : childDTypep();
    }
    व्योम refDTypep(Ast_गाँठ_DType* nodep) { m_refDTypep = nodep; }
    Ast_गाँठ_DType* virtRefDTypep() स्थिर override { वापस m_refDTypep; }
    व्योम virtRefDTypep(Ast_गाँठ_DType* nodep) override { refDTypep(nodep); }
    // METHODS
    AstBasicDType* basicp() स्थिर override VL_MT_STABLE { वापस subDTypep()->basicp(); }
    पूर्णांक widthAlignBytes() स्थिर override { वापस माप(std::map<std::string, std::string>); }
    पूर्णांक width_योग_Bytes() स्थिर override { वापस माप(std::map<std::string, std::string>); }
    bool isCompound() स्थिर override { वापस true; }
};
class AstStreamDType final : खुला Ast_गाँठ_DType {
    // Stream data type, used only as data type of stream operations
    // Should behave like AstPackArrayDType, but it doesn't have a size
खुला:
    explicit AstStreamDType(File_पंक्ति* fl)
        : ASTGEN_SUPER_StreamDType(fl) {
        dtypep(this);
    }
    ASTGEN_MEMBERS_AstStreamDType;
    व्योम dumpSmall(std::ostream& str) स्थिर override;
    bool hasDType() स्थिर override VL_MT_SAFE { वापस true; }
    bool maybePointedTo() स्थिर override VL_MT_SAFE { वापस true; }
    bool undead() स्थिर override { वापस true; }
    Ast_गाँठ_DType* subDTypep() स्थिर override VL_MT_STABLE { वापस nullptr; }
    Ast_गाँठ_DType* virtRefDTypep() स्थिर override { वापस nullptr; }
    व्योम virtRefDTypep(Ast_गाँठ_DType* nodep) override {}
    bool similar_DType_गाँठ(स्थिर Ast_गाँठ_DType* samep) स्थिर override { वापस this == samep; }
    AstBasicDType* basicp() स्थिर override VL_MT_STABLE { वापस nullptr; }
    पूर्णांक widthAlignBytes() स्थिर override { वापस 1; }
    पूर्णांक width_योग_Bytes() स्थिर override { वापस 1; }
    bool isCompound() स्थिर override { वापस false; }
};
class AstUnsizedArrayDType final : खुला Ast_गाँठ_DType {
    // Unsized/open-range Array data type, ie "some_dtype var_name []"
    // @astgen op1 := childDTypep : Optional[AstNodeDType] // moved to refDTypep() in V3Width
    //
    // @astgen ptr := m_refDTypep : Optional[AstNodeDType]  // Elements of this type (post-width)
खुला:
    AstUnsizedArrayDType(File_पंक्ति* fl, VFlagChildDType, Ast_गाँठ_DType* dtp)
        : ASTGEN_SUPER_UnsizedArrayDType(fl) {
        childDTypep(dtp);  // Only for parser
        refDTypep(nullptr);
        dtypep(nullptr);  // V3Width will resolve
    }
    AstUnsizedArrayDType(File_पंक्ति* fl, Ast_गाँठ_DType* dtp)
        : ASTGEN_SUPER_UnsizedArrayDType(fl) {
        refDTypep(dtp);
        dtypep(nullptr);  // V3Width will resolve
    }
    ASTGEN_MEMBERS_AstUnsizedArrayDType;
    स्थिर अक्षर* broken() स्थिर override {
        BROKEN_RTN(!((m_refDTypep && !childDTypep()) || (!m_refDTypep && childDTypep())));
        वापस nullptr;
    }
    bool same_गाँठ(स्थिर Ast_गाँठ* samep) स्थिर override;
    bool similar_DType_गाँठ(स्थिर Ast_गाँठ_DType* samep) स्थिर override;
    व्योम dumpSmall(std::ostream& str) स्थिर override;
    Ast_गाँठ_DType* getChildDTypep() स्थिर override { वापस childDTypep(); }
    Ast_गाँठ_DType* subDTypep() स्थिर override VL_MT_STABLE {
        वापस m_refDTypep ? m_refDTypep : childDTypep();
    }
    व्योम refDTypep(Ast_गाँठ_DType* nodep) { m_refDTypep = nodep; }
    Ast_गाँठ_DType* virtRefDTypep() स्थिर override { वापस m_refDTypep; }
    व्योम virtRefDTypep(Ast_गाँठ_DType* nodep) override { refDTypep(nodep); }
    // METHODS
    AstBasicDType* basicp() स्थिर override VL_MT_STABLE { वापस subDTypep()->basicp(); }
    पूर्णांक widthAlignBytes() स्थिर override { वापस subDTypep()->widthAlignBytes(); }
    पूर्णांक width_योग_Bytes() स्थिर override { वापस subDTypep()->width_योग_Bytes(); }
    bool isCompound() स्थिर override { वापस true; }
};
class AstVoidDType final : खुला Ast_गाँठ_DType {
    // For e.g. a function returning void
खुला:
    explicit AstVoidDType(File_पंक्ति* fl)
        : ASTGEN_SUPER_VoidDType(fl) {
        dtypep(this);
    }
    ASTGEN_MEMBERS_AstVoidDType;
    व्योम dumpSmall(std::ostream& str) स्थिर override;
    bool hasDType() स्थिर override VL_MT_SAFE { वापस true; }
    bool maybePointedTo() स्थिर override VL_MT_SAFE { वापस true; }
    bool undead() स्थिर override { वापस true; }
    Ast_गाँठ_DType* subDTypep() स्थिर override VL_MT_STABLE { वापस nullptr; }
    Ast_गाँठ_DType* virtRefDTypep() स्थिर override { वापस nullptr; }
    व्योम virtRefDTypep(Ast_गाँठ_DType* nodep) override {}
    bool similar_DType_गाँठ(स्थिर Ast_गाँठ_DType* samep) स्थिर override { वापस true; }
    AstBasicDType* basicp() स्थिर override VL_MT_STABLE { वापस nullptr; }
    पूर्णांक widthAlignBytes() स्थिर override { वापस 1; }
    पूर्णांक width_योग_Bytes() स्थिर override { वापस 1; }
    bool isCompound() स्थिर override { वापस false; }
};
class AstWildcardArrayDType final : खुला Ast_गाँठ_DType {
    // Wildcard index type associative array data type, ie "some_dtype var_name [*]"
    // @astgen op1 := childDTypep : Optional[AstNodeDType] // moved to refDTypep() in V3Width
    //
    // @astgen ptr := m_refDTypep : Optional[AstNodeDType]  // Elements of this type (post-width)
खुला:
    AstWildcardArrayDType(File_पंक्ति* fl, VFlagChildDType, Ast_गाँठ_DType* dtp)
        : ASTGEN_SUPER_WildcardArrayDType(fl) {
        childDTypep(dtp);  // Only for parser
        refDTypep(nullptr);
        dtypep(nullptr);  // V3Width will resolve
    }
    ASTGEN_MEMBERS_AstWildcardArrayDType;
    स्थिर अक्षर* broken() स्थिर override {
        BROKEN_RTN(!((m_refDTypep && !childDTypep()) || (!m_refDTypep && childDTypep())));
        वापस nullptr;
    }
    bool same_गाँठ(स्थिर Ast_गाँठ* samep) स्थिर override;
    bool similar_DType_गाँठ(स्थिर Ast_गाँठ_DType* samep) स्थिर override;
    व्योम dumpSmall(std::ostream& str) स्थिर override;
    string prettyDTypeName(bool full) स्थिर override;
    Ast_गाँठ_DType* getChildDTypep() स्थिर override { वापस childDTypep(); }
    Ast_गाँठ_DType* subDTypep() स्थिर override VL_MT_STABLE {
        वापस m_refDTypep ? m_refDTypep : childDTypep();
    }
    व्योम refDTypep(Ast_गाँठ_DType* nodep) { m_refDTypep = nodep; }
    Ast_गाँठ_DType* virtRefDTypep() स्थिर override { वापस m_refDTypep; }
    व्योम virtRefDTypep(Ast_गाँठ_DType* nodep) override { refDTypep(nodep); }
    // METHODS
    AstBasicDType* basicp() स्थिर override VL_MT_STABLE { वापस subDTypep()->basicp(); }
    पूर्णांक widthAlignBytes() स्थिर override { वापस माप(std::map<std::string, std::string>); }
    पूर्णांक width_योग_Bytes() स्थिर override { वापस माप(std::map<std::string, std::string>); }
    bool isCompound() स्थिर override { वापस true; }
    bool isDynamicallySized() स्थिर override { वापस true; }
};

// === AstNodeArrayDType ===
class AstPackArrayDType final : खुला Ast_गाँठ_Array_DType {
    // Packed array data type, ie "some_dtype [2:0] var_name"
खुला:
    अंतरभूत AstPackArrayDType(File_पंक्ति* fl, VFlagChildDType, Ast_गाँठ_DType* dtp, AstRange* rangep);
    अंतरभूत AstPackArrayDType(File_पंक्ति* fl, Ast_गाँठ_DType* dtp, AstRange* rangep);
    ASTGEN_MEMBERS_AstPackArrayDType;
    string prettyDTypeName(bool full) स्थिर override;
    bool isCompound() स्थिर override { वापस false; }
};
class AstUnpackArrayDType final : खुला Ast_गाँठ_Array_DType {
    // Array data type, ie "some_dtype var_name [2:0]"
    bool m_isCompound = false;  // Non-POD subDType, or parent requires compound
खुला:
    AstUnpackArrayDType(File_पंक्ति* fl, VFlagChildDType, Ast_गाँठ_DType* dtp, AstRange* rangep)
        : ASTGEN_SUPER_UnpackArrayDType(fl) {
        childDTypep(dtp);  // Only for parser
        this->rangep(rangep);
        refDTypep(nullptr);
        dtypep(nullptr);  // V3Width will resolve
        // For backward compatibility AstNodeArrayDType and others inherit
        // width and signing from the subDType/base type
        widthFromSub(subDTypep());
    }
    AstUnpackArrayDType(File_पंक्ति* fl, Ast_गाँठ_DType* dtp, AstRange* rangep)
        : ASTGEN_SUPER_UnpackArrayDType(fl) {
        this->rangep(rangep);
        refDTypep(dtp);
        dtypep(this);
        // For backward compatibility AstNodeArrayDType and others inherit
        // width and signing from the subDType/base type
        widthFromSub(subDTypep());
    }
    ASTGEN_MEMBERS_AstUnpackArrayDType;
    व्योम dump(std::ostream& str) स्थिर override;
    व्योम dumpJson(std::ostream& str) स्थिर override;
    string prettyDTypeName(bool full) स्थिर override;
    bool same_गाँठ(स्थिर Ast_गाँठ* samep) स्थिर override {
        स्थिर AstUnpackArrayDType* स्थिर sp = VN_DBG_AS(samep, UnpackArrayDType);
        वापस m_isCompound == sp->m_isCompound;
    }
    bool similar_DType_गाँठ(स्थिर Ast_गाँठ_DType* samep) स्थिर override;
    bool isAggregateType() स्थिर override { वापस true; }
    // True iff 'otherp' is an array of the same shape, with
    // at least one dimension reversed ascending/descending.
    bool isReverseOf(स्थिर Ast_गाँठ_DType* otherp) स्थिर;
    // Outer dimension comes first. The first element is this node.
    std::vector<AstUnpackArrayDType*> unpackDimensions();
    व्योम isCompound(bool flag) { m_isCompound = flag; }
    bool isCompound() स्थिर override VL_MT_SAFE { वापस m_isCompound; }
    bool isIntegralOrPacked() स्थिर override { वापस false; }
};

// === AstNodeUOrStructDType ===
class AstStructDType final : खुला Ast_गाँठ_UOr_Struct_DType {
खुला:
    // VSigning below is mispurposed to indicate if packed or not
    AstStructDType(File_पंक्ति* fl, VSigning numericUnpack)
        : ASTGEN_SUPER_StructDType(fl, numericUnpack) {}
    ASTGEN_MEMBERS_AstStructDType;
    string verilogKwd() स्थिर override { वापस "struct"; }
};
class AstUnionDType final : खुला Ast_गाँठ_UOr_Struct_DType {
    bool m_isSoft;  // Is a "union soft"
    bool m_isTagged;  // Is a "union tagged"

खुला:
    // VSigning below is mispurposed to indicate if packed or not
    // isSoft implies packed
    AstUnionDType(File_पंक्ति* fl, bool isSoft, bool isTagged, VSigning numericUnpack)
        : ASTGEN_SUPER_UnionDType(fl, numericUnpack)
        , m_isSoft{isSoft}
        , m_isTagged{isTagged} {
        packed(packed() | m_isSoft);
    }
    ASTGEN_MEMBERS_AstUnionDType;
    string verilogKwd() स्थिर override { वापस "union"; }
    bool isSoft() स्थिर { वापस m_isSoft; }
    bool isTagged() स्थिर { वापस m_isTagged; }
    bool same_गाँठ(स्थिर Ast_गाँठ* samep) स्थिर override;
    व्योम dump(std::ostream& str) स्थिर override;
    व्योम dumpJson(std::ostream& str) स्थिर override;
};

#पूर्ण_अगर  // Guard
