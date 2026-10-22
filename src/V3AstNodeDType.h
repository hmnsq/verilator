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

#اگر_غیر_تعریف_شدہ VERILATOR_V3ASTNODEDTYPE_H_
#تعریف_کرو VERILATOR_V3ASTNODEDTYPE_H_

#اگر_غیر_تعریف_شدہ VERILATOR_V3AST_H_
#غلطی "Use V3Ast.h as the include"
#شامل "V3Ast.h"  // This helps code analysis tools pick up symbols in V3Ast.h
#تعریف_کرو VL_NOT_FINAL  // This #define fixes broken code folding in the CLion IDE
#اگر_ختم

// === Abstract base node types (AstNode*) =====================================

class Ast_گرہ_DType VL_NOT_FINAL : عوامی Ast_گرہ {
    // Ideally width() would migrate to BasicDType as that's where it makes sense,
    // but it's currently so prevalent in the code we leave it here.
    // Note the below members are included in AstTypeTable::Key lookups
    // dist-ast-dump-suppress  // Part of dumpSmall
    صحیح m_چوڑائی = 0;  // (also in AstTypeTable::Key) Bit width of operation
    // (also in AstTypeTable::Key) If unsized, bitwidth of minimum implementation
    // dist-ast-dump-suppress  // Part of dumpSmall
    صحیح m_چوڑائی_Min = 0;
    // dist-ast-dump-suppress  // Part of dumpSmall
    VSigning m_numeric;  // (also in AstTypeTable::Key) Node is signed
    // dist-ast-dump-suppress  // Part of dumpSmall
    bool m_generic = false;  // Simple globally referenced type, don't garbage collect
    // Unique number assigned to each dtype during creation for IEEE matching
    ساکن صحیح s_unique_عدد;

محفوظ:
    // CONSTRUCTORS
    Ast_گرہ_DType(VNType t, File_سطر* fl)
        : Ast_گرہ{t, fl} {}

نجی:
    // METHODS
    ثابت Ast_گرہ_DType* skipRefIterp(bool skipConst, bool skipEnum,
                                     bool assertOn = true) ثابت VL_MT_STABLE;

محفوظ:
    // METHODS
    مجازی bool similar_DType_گرہ(ثابت Ast_گرہ_DType* samep) ثابت = 0;

عوامی:
    ASTGEN_MEMBERS_Ast_گرہ_DType;
    // ACCESSORS
    خالی dump(std::ostream& str) ثابت override;
    خالی dumpJson(std::ostream& str) ثابت override;
    مجازی خالی dumpSmall(std::ostream& str) ثابت VL_MT_STABLE;
    bool hasDType() ثابت override VL_MT_محفوظ { واپس true; }
    /// Require VlUnpacked, instead of [] for POD elements.
    /// A non-POD object is always compound, but some POD elements
    /// are compound when methods calls operate on object, or when
    /// under another compound-requiring object e.g. class
    مجازی bool isCompound() ثابت = 0;
    // Integral or packed, allowed inside an unpacked union/struct
    مجازی bool isIntegralOrPacked() ثابت { واپس !isCompound(); }
    // (Slow) recurse down to find basic data type
    مجازی AstBasicDType* basicp() ثابت VL_MT_STABLE = 0;
    // (Slow) Recurse over MemberDType|ParamTypeDType|RefDType|ConstDType|EnumDType to other type
    ثابت Ast_گرہ_DType* skipRefp() ثابت VL_MT_STABLE { واپس skipRefIterp(true, true); }
    Ast_گرہ_DType* skipRefp() VL_MT_STABLE {
        واپس const_cast<Ast_گرہ_DType*>(
            static_cast<ثابت Ast_گرہ_DType*>(this)->skipRefIterp(true, true));
    }
    // (Slow) Recurse over MemberDType|ParamTypeDType|RefDType|ConstDType|EnumDType to other type,
    // Returns null if not resolved
    ثابت Ast_گرہ_DType* skipRefOrNullp() ثابت VL_MT_STABLE {
        واپس skipRefIterp(true, true, false);
    }
    Ast_گرہ_DType* skipRefOrNullp() VL_MT_STABLE {
        واپس const_cast<Ast_گرہ_DType*>(
            static_cast<ثابت Ast_گرہ_DType*>(this)->skipRefIterp(true, true, false));
    }
    // (Slow) Recurse over MemberDType|ParamTypeDType|RefDType|EnumDType to ConstDType
    ثابت Ast_گرہ_DType* skip_Ref_مقصد_Constp() ثابت { واپس skipRefIterp(false, true); }
    Ast_گرہ_DType* skip_Ref_مقصد_Constp() {
        واپس const_cast<Ast_گرہ_DType*>(
            static_cast<ثابت Ast_گرہ_DType*>(this)->skipRefIterp(false, true));
    }
    // (Slow) Recurse over MemberDType|ParamTypeDType|RefDType|ConstDType to EnumDType
    ثابت Ast_گرہ_DType* skip_Ref_مقصد_Enump() ثابت { واپس skipRefIterp(true, false); }
    Ast_گرہ_DType* skip_Ref_مقصد_Enump() {
        واپس const_cast<Ast_گرہ_DType*>(
            static_cast<ثابت Ast_گرہ_DType*>(this)->skipRefIterp(true, false));
    }
    // (Slow) Recurse over MemberDType|ParamTypeDType|RefDType|ConstDType to EnumDType,
    // Returns null if not resolved
    ثابت Ast_گرہ_DType* skip_Ref_مقصد_Enum_Or_Nullp() ثابت { واپس skipRefIterp(true, false, false); }
    // (Slow) Recurse over MemberDType|ParamTypeDType|RefDType to other type
    ثابت Ast_گرہ_DType* skip_Ref_مقصد_Non_Refp() ثابت { واپس skipRefIterp(false, false); }
    Ast_گرہ_DType* skip_Ref_مقصد_Non_Refp() {
        واپس const_cast<Ast_گرہ_DType*>(
            static_cast<ثابت Ast_گرہ_DType*>(this)->skipRefIterp(false, false));
    }
    // If array, returns element dtype, otherwise returns skipRef dtype
    // If skipRef is false, RefDTypes are not followed (safe before typedef linking)
    ثابت Ast_گرہ_DType* elemDTypep(bool skipRef = true) ثابت VL_MT_STABLE;
    Ast_گرہ_DType* elemDTypep(bool skipRef = true) VL_MT_STABLE {
        واپس const_cast<Ast_گرہ_DType*>(
            static_cast<ثابت Ast_گرہ_DType*>(this)->elemDTypep(skipRef));
    }
    // (Slow) recurses - Structure alignment 1,2,4 or 8 bytes (arrays affect this)
    مجازی صحیح چوڑائی_Align_بائٹیں() ثابت = 0;
    // (Slow) recurses - Width in bytes rounding up 1,2,4,8,12,...
    مجازی صحیح چوڑائی_کل_بائٹیں() ثابت = 0;
    bool maybe_Pointed_مقصد() ثابت override VL_MT_محفوظ { واپس true; }
    // Iff has a non-null refDTypep(), as generic node function
    مجازی Ast_گرہ_DType* virtRefDTypep() ثابت { واپس nullptr; }
    // Iff has refDTypep(), set as generic node function
    مجازی خالی virtRefDTypep(Ast_گرہ_DType* nodep) {}
    // Iff has a non-null second dtypep, as generic node function
    مجازی Ast_گرہ_DType* virtRefDType2p() ثابت { واپس nullptr; }
    // Iff has second dtype, set as generic node function
    مجازی خالی virtRefDType2p(Ast_گرہ_DType* nodep) {}
    // Assignable equivalence.  Calls skipRefToNonRefp() during comparisons.
    bool similarDType(ثابت Ast_گرہ_DType* samep) ثابت;
    // Iff has a non-null subDTypep(), as generic node function
    مجازی Ast_گرہ_DType* subDTypep() ثابت VL_MT_STABLE { واپس nullptr; }
    مجازی Ast_گرہ_DType* subDType2p() ثابت VL_MT_STABLE { واپس nullptr; }
    مجازی bool isAggregateType() ثابت { واپس false; }
    // True for unpacked, dynamic, queue, and associative arrays (not packed arrays)
    bool isNonPackedArray() ثابت;
    مجازی bool isFourstate() ثابت;
    // Ideally an IEEE $typename
    مجازی string prettyDTypeName(bool) ثابت { واپس prettyTypeName(); }
    string prettyDTypeNameQ(bool بھرا_2 = false) ثابت { واپس "'" + prettyDTypeName(بھرا_2) + "'"; }
    string stateDTypeName() ثابت { واپس this->isFourstate() ? "(4-state)" : "(2-state)"; }
    //
    // Changing the width may confuse the data type resolution, so must clear
    // TypeTable cache after use.
    خالی چوڑائی_Force(صحیح چوڑائی, صحیح چوڑائی_Min) {
        m_چوڑائی = چوڑائی;
        m_چوڑائی_Min = چوڑائی_Min;
    }
    // For backward compatibility inherit width and signing from the subDType/base type
    خالی چوڑائی_From_Sub(ثابت Ast_گرہ_DType* nodep) {
        m_چوڑائی = nodep->m_چوڑائی;
        m_چوڑائی_Min = nodep->m_چوڑائی_Min;
        m_numeric = nodep->m_numeric;
    }
    //
    صحیح چوڑائی() ثابت VL_MT_STABLE { واپس m_چوڑائی; }
    خالی numeric(VSigning flag) { m_numeric = flag; }
    bool isSigned() ثابت VL_MT_STABLE { واپس m_numeric.isSigned(); }
    bool isNosign() ثابت VL_MT_محفوظ { واپس m_numeric.isNosign(); }
    VSigning numeric() ثابت VL_MT_STABLE { واپس m_numeric; }
    صحیح چوڑائی_الفاظ() ثابت VL_MT_STABLE { واپس VL_الفاظ_I(چوڑائی()); }
    صحیح چوڑائی_Min() ثابت VL_MT_STABLE {  // If sized, the size,
                                         // if unsized the min digits to represent it
        واپس m_چوڑائی_Min ? m_چوڑائی_Min : m_چوڑائی;
    }
    صحیح چوڑائی_Pow2() ثابت;
    خالی چوڑائی_Min_From_چوڑائی() { m_چوڑائی_Min = m_چوڑائی; }
    bool چوڑائی_Sized() ثابت VL_MT_محفوظ { واپس !m_چوڑائی_Min || m_چوڑائی_Min == m_چوڑائی; }
    bool generic() ثابت VL_MT_محفوظ { واپس m_generic; }
    خالی generic(bool flag) { m_generic = flag; }
    std::pair<uint32_t, uint32_t> dimensions(bool includeBasic) ثابت;
    uint32_t arrayUnpackedElements() ثابت;  // 1, or total multiplication of all dimensions
    // Fixed aggregate streaming properties
    bool isStreamableFixedAggregate() ثابت;
    // Whether is a string, or an unpacked array or structure containing one
    bool شامل_ہے_String() ثابت;
    bool شامل_ہے_Unpacked_Struct() ثابت;
    صحیح چوڑائی_دھارا() ثابت;
    string vlEnumType() ثابت;  // Return VerilatedVarType: VLVT_UINT32, etc
    string vlEnumPackedAgg() ثابت;  // Return VLVF_PACKED_AGG etc flags, or empty
    ساکن صحیح unique_عدد_Inc() { واپس ++s_unique_عدد; }
    ثابت حرف* charIQWN() ثابت {
        واپس (isString() ? "N" : isWide() ? "W" : isDouble() ? "D" : isQuad() ? "Q" : "I");
    }
    string cType(ثابت string& name, bool forFunc, bool isRef, bool packed = false) ثابت;
    // Represents a C++ LiteralType? (can be constexpr)
    bool isLiteralType() ثابت VL_MT_STABLE;
    // C++ type (as emitted by cType) is trivially default constructible and destructible?
    bool isTriviallyConstructible() ثابت VL_MT_STABLE;
    مجازی bool isDynamicallySized() ثابت { واپس false; }

نجی:
    class CTypeRecursed;
    CTypeRecursed cTypeRecurse(bool compound, bool packed) ثابت;
};
class Ast_گرہ_Array_DType VL_NOT_FINAL : عوامی Ast_گرہ_DType {
    // Array data type, ie "some_dtype var_name [2:0]"
    // @astgen op1 := childDTypep : Optional[AstNodeDType] // moved to refDTypep() in V3Width
    // @astgen op2 := rangep : Optional[AstRange] // array bounds
    //
    // @astgen ptr := m_refDTypep : Optional[AstNodeDType]  // Elements of this type (post-width)
    Ast_گرہ* rangenp() ثابت { واپس reinterpret_cast<Ast_گرہ*>(rangep()); }

محفوظ:
    Ast_گرہ_Array_DType(VNType t, File_سطر* fl)
        : Ast_گرہ_DType{t, fl} {}

عوامی:
    ASTGEN_MEMBERS_Ast_گرہ_Array_DType;
    خالی dump(std::ostream& str) ثابت override;
    خالی dumpJson(std::ostream& str) ثابت override;
    خالی dumpSmall(std::ostream& str) ثابت override;
    ثابت حرف* broken() ثابت override {
        BROKEN_RTN(!((m_refDTypep && !بچہ_DTypep()) || (!m_refDTypep && بچہ_DTypep())));
        واپس nullptr;
    }
    bool same_گرہ(ثابت Ast_گرہ* samep) ثابت override {
        ثابت Ast_گرہ_Array_DType* ثابت asamep = VN_DBG_AS(samep, گرہ_Array_DType);
        واپس hi() == asamep->hi() && rangenp()->same_درخت(asamep->rangenp())
               && subDTypep() == asamep->subDTypep();
    }  // HashedDT doesn't recurse, so need to check children
    bool similar_DType_گرہ(ثابت Ast_گرہ_DType* samep) ثابت override {
        ثابت Ast_گرہ_Array_DType* ثابت asamep = VN_DBG_AS(samep, گرہ_Array_DType);
        واپس elementsConst() == asamep->elementsConst()
               && subDTypep()->similarDType(asamep->subDTypep());
    }
    Ast_گرہ_DType* get_بچہ_DTypep() ثابت override { واپس بچہ_DTypep(); }
    Ast_گرہ_DType* subDTypep() ثابت override VL_MT_STABLE {
        واپس m_refDTypep ? m_refDTypep : بچہ_DTypep();
    }
    خالی refDTypep(Ast_گرہ_DType* nodep) { m_refDTypep = nodep; }
    Ast_گرہ_DType* virtRefDTypep() ثابت override { واپس m_refDTypep; }
    خالی virtRefDTypep(Ast_گرہ_DType* nodep) override { refDTypep(nodep); }
    // METHODS
    AstBasicDType* basicp() ثابت override VL_MT_STABLE {
        واپس subDTypep()->basicp();
    }  // (Slow) recurse down to find basic data type
    صحیح چوڑائی_Align_بائٹیں() ثابت override { واپس subDTypep()->چوڑائی_Align_بائٹیں(); }
    صحیح چوڑائی_کل_بائٹیں() ثابت override {
        واپس elementsConst() * subDTypep()->چوڑائی_کل_بائٹیں();
    }
    اندرونی صحیح بایاں() ثابت VL_MT_STABLE;
    اندرونی صحیح دایاں() ثابت VL_MT_STABLE;
    اندرونی صحیح hi() ثابت VL_MT_STABLE;
    اندرونی صحیح lo() ثابت VL_MT_STABLE;
    اندرونی صحیح elementsConst() ثابت VL_MT_STABLE;
    اندرونی VNumRange declRange() ثابت VL_MT_STABLE;
};
class Ast_گرہ_UOr_Struct_DType VL_NOT_FINAL : عوامی Ast_گرہ_DType {
    // A struct or union; common handling
    // @astgen op1 := membersp : List[AstMemberDType]
    //
    // @astgen ptr := m_classOrPackagep : Optional[AstNodeModule]  // Package emitted with
    string m_name;  // Name from upper typedef, if any
    string m_typedefName;  // Typedef's dtypeName() once moved off the typedef declaring it
    ثابت صحیح m_unique_عدد;  // Unique ID distinguishing this dtype instance, for hashing/naming
    bool m_packed;  // Packed struct/union, else unpacked
    bool m_isFourstate = false;  // V3Width computes; true if any member is 4-state
    bool m_constrainedRand = false;  // True if struct has constraint expression
    bool m_emit_مقصد_String = false;  // Generate to_string() for this struct/union if set

محفوظ:
    Ast_گرہ_UOr_Struct_DType(VNType t, File_سطر* fl, VSigning numericUnpack)
        : Ast_گرہ_DType{t, fl}
        , m_unique_عدد{unique_عدد_Inc()} {
        // VSigning::NOSIGN overloaded to indicate not packed
        m_packed = (numericUnpack != VSigning::NOSIGN);
        numeric(VSigning::fromBool(numericUnpack.isSigned()));
    }
    Ast_گرہ_UOr_Struct_DType(ثابت Ast_گرہ_UOr_Struct_DType& other)
        : Ast_گرہ_DType{other}
        , m_name{other.m_name}
        , m_typedefName{other.m_typedefName}
        , m_unique_عدد{unique_عدد_Inc()}
        , m_packed{other.m_packed}
        , m_isFourstate{other.m_isFourstate} {}

عوامی:
    ASTGEN_MEMBERS_Ast_گرہ_UOr_Struct_DType;
    صحیح unique_عدد() ثابت { واپس m_unique_عدد; }
    خالی dump(std::ostream& str) ثابت override;
    خالی dumpJson(std::ostream& str) ثابت override;
    string prettyDTypeName(bool) ثابت override;
    bool isCompound() ثابت override { واپس !packed(); }
    // For basicp() we reuse the size to indicate a "fake" basic type of same size
    AstBasicDType* basicp() ثابت override VL_MT_STABLE {
        اگر (!m_packed) واپس nullptr;
        واپس (isFourstate()
                    ? VN_AS(ڈھونڈو_Logic_Range_DType(VNumRange{چوڑائی() - 1, 0}, چوڑائی(), numeric()),
                            BasicDType)
                    : VN_AS(ڈھونڈو_ثنائی_ہندسہ_Range_DType(VNumRange{چوڑائی() - 1, 0}, چوڑائی(), numeric()),
                            BasicDType));
    }
    // (Slow) recurses - Structure alignment 1,2,4 or 8 bytes (arrays affect this)
    صحیح چوڑائی_Align_بائٹیں() ثابت override;
    // (Slow) recurses - Width in bytes rounding up 1,2,4,8,12,...
    صحیح چوڑائی_کل_بائٹیں() ثابت override;
    bool similar_DType_گرہ(ثابت Ast_گرہ_DType* samep) ثابت override;
    string name() ثابت override VL_MT_STABLE { واپس m_name; }
    خالی name(ثابت string& flag) override { m_name = flag; }
    // Keep the name the typedef declaring it gives, as it moves off it to the type table
    خالی typedefName(ثابت string& name) { m_typedefName = name; }
    // Name of the C structure for DPI, empty if no typedef names it, so declared where used
    string dpiName() ثابت;
    // C typedef of the unpacked structure for DPI (IEEE 1800-2023 H.7.3)
    string dpiTypedef() ثابت;
    bool packed() ثابت VL_MT_محفوظ { واپس m_packed; }
    خالی packed(bool flag) { m_packed = flag; }
    // packed() but as don't support unpacked, presently all structs
    ساکن bool packedUnsup() { واپس true; }
    خالی isFourstate(bool flag) { m_isFourstate = flag; }
    bool isFourstate() ثابت override VL_MT_محفوظ { واپس m_isFourstate; }
    ساکن صحیح lo() VL_MT_STABLE { واپس 0; }
    صحیح hi() ثابت VL_MT_STABLE {
        واپس dtypep()->چوڑائی() - 1;
    }  // Packed classes look like arrays
    VNumRange declRange() ثابت VL_MT_STABLE { واپس VNumRange{hi(), lo()}; }
    Ast_گرہ_Module* classOrPackagep() ثابت { واپس m_classOrPackagep; }
    خالی classOrPackagep(Ast_گرہ_Module* classpackagep) { m_classOrPackagep = classpackagep; }
    bool isConstrainedRand() ثابت { واپس m_constrainedRand; }
    خالی markConstrainedRand(bool flag) { m_constrainedRand = flag; }
    bool emit_مقصد_String() ثابت { واپس m_emit_مقصد_String; }
    خالی سیٹ_Emit_مقصد_String() { m_emit_مقصد_String = true; }
    bool isAggregateType() ثابت override { واپس !packed(); }
};

// === Concrete node types =====================================================

// === AstNode ===
class Ast_Enum_شے final : عوامی Ast_گرہ {
    // @astgen op1 := rangep : Optional[AstRange] // Range for name appending
    // @astgen op2 := valuep : Optional[AstNodeExpr]
    string m_name;  // Name of item

عوامی:
    // Parents: ENUM
    Ast_Enum_شے(File_سطر* fl, ثابت string& name, AstRange* rangep, Ast_گرہ_اظہاریہ* valuep)
        : ASTGEN_SUPER_Enum_شے(fl)
        , m_name{name} {
        this->rangep(rangep);
        this->valuep(valuep);
    }
    ASTGEN_MEMBERS_Ast_Enum_شے;
    string name() ثابت override VL_MT_STABLE { واپس m_name; }
    bool maybe_Pointed_مقصد() ثابت override VL_MT_محفوظ { واپس true; }
    bool hasDType() ثابت override VL_MT_محفوظ { واپس true; }
    خالی name(ثابت string& flag) override { m_name = flag; }
};

// === AstNodeDType ===
class AstAssocArrayDType final : عوامی Ast_گرہ_DType {
    // Associative array data type, ie "[some_dtype]"
    // @astgen op1 := childDTypep : Optional[AstNodeDType] // moved to refDTypep() in V3Width
    // @astgen op2 := keyChildDTypep : Optional[AstNodeDType]
    //
    // @astgen ptr := m_refDTypep : Optional[AstNodeDType]  // Elements of this type (post-width)
    // @astgen ptr := m_keyDTypep : Optional[AstNodeDType]  // Keys of this type (post-width)
عوامی:
    AstAssocArrayDType(File_سطر* fl, VFlag_بچہ_DType, Ast_گرہ_DType* dtp, Ast_گرہ_DType* کلید_Dtp)
        : ASTGEN_SUPER_AssocArrayDType(fl) {
        بچہ_DTypep(dtp);  // Only for parser
        کلید_بچہ_DTypep(کلید_Dtp);  // Only for parser
        refDTypep(nullptr);
        کلید_DTypep(nullptr);
        dtypep(nullptr);  // V3Width will resolve
    }
    AstAssocArrayDType(File_سطر* fl, Ast_گرہ_DType* dtp, Ast_گرہ_DType* کلید_Dtp)
        : ASTGEN_SUPER_AssocArrayDType(fl) {
        refDTypep(dtp);
        کلید_DTypep(کلید_Dtp);
        dtypep(dtp);
    }
    ASTGEN_MEMBERS_AstAssocArrayDType;
    ثابت حرف* broken() ثابت override {
        BROKEN_RTN(!((m_refDTypep && !بچہ_DTypep()) || (!m_refDTypep && بچہ_DTypep())));
        BROKEN_RTN(!((m_کلید_DTypep && !بچہ_DTypep()) || (!m_کلید_DTypep && بچہ_DTypep())));
        واپس nullptr;
    }
    bool same_گرہ(ثابت Ast_گرہ* samep) ثابت override {
        ثابت AstAssocArrayDType* ثابت asamep = VN_DBG_AS(samep, AssocArrayDType);
        اگر (!asamep->subDTypep()) واپس false;
        اگر (!asamep->کلید_DTypep()) واپس false;
        واپس (subDTypep() == asamep->subDTypep() && کلید_DTypep() == asamep->کلید_DTypep());
    }
    bool similar_DType_گرہ(ثابت Ast_گرہ_DType* samep) ثابت override {
        ثابت AstAssocArrayDType* ثابت asamep = VN_DBG_AS(samep, AssocArrayDType);
        واپس asamep->subDTypep() && subDTypep()->similarDType(asamep->subDTypep())
               && asamep->کلید_DTypep() && کلید_DTypep()->similarDType(asamep->کلید_DTypep());
    }
    string prettyDTypeName(bool بھرا_2) ثابت override;
    خالی dumpSmall(std::ostream& str) ثابت override;
    Ast_گرہ_DType* get_بچہ_DTypep() ثابت override { واپس بچہ_DTypep(); }
    Ast_گرہ_DType* getChild2DTypep() ثابت override { واپس کلید_بچہ_DTypep(); }
    Ast_گرہ_DType* subDTypep() ثابت override VL_MT_STABLE {
        واپس m_refDTypep ? m_refDTypep : بچہ_DTypep();
    }
    Ast_گرہ_DType* subDType2p() ثابت override VL_MT_STABLE {
        واپس m_کلید_DTypep ? m_کلید_DTypep : کلید_بچہ_DTypep();
    }
    bool isAggregateType() ثابت override { واپس true; }
    خالی refDTypep(Ast_گرہ_DType* nodep) { m_refDTypep = nodep; }
    Ast_گرہ_DType* virtRefDTypep() ثابت override { واپس m_refDTypep; }
    خالی virtRefDTypep(Ast_گرہ_DType* nodep) override { refDTypep(nodep); }
    Ast_گرہ_DType* virtRefDType2p() ثابت override { واپس m_کلید_DTypep; }
    خالی virtRefDType2p(Ast_گرہ_DType* nodep) override { کلید_DTypep(nodep); }
    //
    Ast_گرہ_DType* کلید_DTypep() ثابت VL_MT_STABLE {
        واپس m_کلید_DTypep ? m_کلید_DTypep : کلید_بچہ_DTypep();
    }
    خالی کلید_DTypep(Ast_گرہ_DType* nodep) { m_کلید_DTypep = nodep; }
    // METHODS
    AstBasicDType* basicp() ثابت override VL_MT_STABLE { واپس nullptr; }
    صحیح چوڑائی_Align_بائٹیں() ثابت override { واپس subDTypep()->چوڑائی_Align_بائٹیں(); }
    صحیح چوڑائی_کل_بائٹیں() ثابت override { واپس subDTypep()->چوڑائی_کل_بائٹیں(); }
    bool isCompound() ثابت override { واپس true; }
    bool isDynamicallySized() ثابت override { واپس true; }
};
class AstBasicDType final : عوامی Ast_گرہ_DType {
    // Builtin atomic/vectored data type
    // @astgen op1 := rangep : Optional[AstRange] // Range of variable
    ساخت Members final {
        VBasicDTypeKwd m_keyword;  // (also in VBasicTypeKey) What keyword created basic type
        // dist-ast-dump-suppress  // Part of dumpSmall
        VNumRange m_nrange;  // (also in VBasicTypeKey) Numeric msb/lsb (if non-opaque keyword)
        bool عامل==(ثابت Members& rhs) ثابت {
            واپس rhs.m_keyword == m_keyword && rhs.m_nrange == m_nrange;
        }
    } m;
    // See also in AstNodeDType: m_width, m_widthMin, m_numeric(issigned)
عوامی:
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
    AstBasicDType(File_سطر* fl, VFlag_ثنائی_ہندسہ_Packed, صحیح wantwidth)
        : ASTGEN_SUPER_BasicDType(fl) {
        init(VBasicDTypeKwd::ثنائی_ہندسہ, VSigning::NOSIGN, wantwidth, -1, nullptr);
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
نجی:
    خالی init(VBasicDTypeKwd kwd, VSigning numer, صحیح wantwidth, صحیح wantwidthmin,
              AstRange* rangep);

عوامی:
    ASTGEN_MEMBERS_AstBasicDType;
    خالی dump(std::ostream& str) ثابت override;
    خالی dumpJson(std::ostream& str) ثابت override;
    // width/widthMin/numeric compared elsewhere
    bool same_گرہ(ثابت Ast_گرہ* samep) ثابت override;
    bool similar_DType_گرہ(ثابت Ast_گرہ_DType* samep) ثابت override;
    string name() ثابت override VL_MT_STABLE { واپس m.m_keyword.typeName(); }
    string prettyDTypeName(bool بھرا_2) ثابت override;
    ثابت حرف* broken() ثابت override {
        BROKEN_RTN(dtypep() != this);
        BROKEN_RTN(v3Global.چوڑائی_Min_Usage() == VWidthMinUsage::VERILOG_چوڑائی
                   && چوڑائی_Min() > چوڑائی());
        واپس nullptr;
    }
    خالی سیٹ_Signed_State(ثابت VSigning& signst) {
        // Note NOSIGN does NOT change the state; this is required by the parser
        اگر (signst == VSigning::UNSIGNED) {
            numeric(signst);
        } ورنہ اگر (signst == VSigning::SIGNED) {
            numeric(signst);
        }
    }
    // METHODS
    AstBasicDType* basicp() ثابت override VL_MT_STABLE { واپس (AstBasicDType*)this; }
    // (Slow) recurses - Structure alignment 1,2,4 or 8 bytes (arrays affect this)
    صحیح چوڑائی_Align_بائٹیں() ثابت override;
    // (Slow) recurses - Width in bytes rounding up 1,2,4,8,12,...
    صحیح چوڑائی_کل_بائٹیں() ثابت override;
    bool isFourstate() ثابت override { واپس keyword().isFourstate(); }
    VBasicDTypeKwd keyword() ثابت VL_MT_محفوظ {  // Avoid using - use isSomething accessors instead
        واپس m.m_keyword;
    }
    bool is_ثنائی_ہندسہ_Logic() ثابت { واپس keyword().is_ثنائی_ہندسہ_Logic(); }
    bool is_ثنائی_ہندسہ() ثابت { واپس keyword().is_ثنائی_ہندسہ(); }
    bool isCHandle() ثابت VL_MT_STABLE { واپس keyword().isCHandle(); }
    bool isDouble() ثابت VL_MT_STABLE { واپس keyword().isDouble(); }
    bool isEvent() ثابت VL_MT_STABLE { واپس keyword() == VBasicDTypeKwd::EVENT; }
    bool isForkSync() ثابت VL_MT_محفوظ { واپس keyword() == VBasicDTypeKwd::FORK_SYNC; }
    bool isProcessRef() ثابت VL_MT_محفوظ { واپس keyword() == VBasicDTypeKwd::PROCESS_REFERENCE; }
    bool isDelayScheduler() ثابت VL_MT_محفوظ {
        واپس keyword() == VBasicDTypeKwd::DELAY_SCHEDULER;
    }
    bool isTriggerScheduler() ثابت VL_MT_محفوظ {
        واپس keyword() == VBasicDTypeKwd::TRIGGER_SCHEDULER;
    }
    bool isDynamicTriggerScheduler() ثابت VL_MT_محفوظ {
        واپس keyword() == VBasicDTypeKwd::DYNAMIC_TRIGGER_SCHEDULER;
    }
    bool isRandomGenerator() ثابت VL_MT_محفوظ {
        واپس keyword() == VBasicDTypeKwd::RANDOM_GENERATOR;
    }
    bool isStdRandomGenerator() ثابت VL_MT_محفوظ {
        واپس keyword() == VBasicDTypeKwd::RANDOM_STDGENERATOR;
    }
    bool isCovergroupInstHandle() ثابت VL_MT_محفوظ {
        واپس keyword() == VBasicDTypeKwd::COVERGROUP_INSTHANDLE;
    }
    bool isOpaque() ثابت VL_MT_محفوظ { واپس keyword().isOpaque(); }
    bool isString() ثابت VL_MT_STABLE { واپس keyword().isString(); }
    bool isZeroInit() ثابت { واپس keyword().isZeroInit(); }
    bool isRanged() ثابت { واپس rangep() || m.m_nrange.ranged(); }
    bool is_Dpi_ثنائی_ہندسہ_Vec() ثابت {  // DPI uses svBitVecVal
        واپس keyword() == VBasicDTypeKwd::ثنائی_ہندسہ && isRanged();
    }
    bool isDpiLogicVec() ثابت {  // DPI uses svLogicVecVal
        واپس keyword().isFourstate() && !(keyword() == VBasicDTypeKwd::LOGIC && !isRanged());
    }
    bool isDpiCLayout() ثابت {
        واپس keyword().isDpiCLayout()
               || (keyword().isSameish(VBasicDTypeKwd::LOGIC) && !isRanged());
    }
    bool isDpiPrimitive() ثابت {  // DPI uses a primitive type
        واپس !is_Dpi_ثنائی_ہندسہ_Vec() && !isDpiLogicVec();
    }
    // Generally the lo/hi/left/right funcs should be used instead of nrange()
    ثابت VNumRange& nrange() ثابت { واپس m.m_nrange; }
    اندرونی صحیح hi() ثابت;
    اندرونی صحیح lo() ثابت;
    اندرونی صحیح elements() ثابت;
    صحیح بایاں() ثابت { واپس ascending() ? lo() : hi(); }  // How to show a declaration
    صحیح دایاں() ثابت { واپس ascending() ? hi() : lo(); }
    اندرونی bool ascending() ثابت;
    bool implicit() ثابت { واپس keyword() == VBasicDTypeKwd::LOGIC_IMPLICIT; }
    bool untyped() ثابت { واپس keyword() == VBasicDTypeKwd::UNTYPED; }
    VNumRange declRange() ثابت { واپس isRanged() ? VNumRange{بایاں(), دایاں()} : VNumRange{}; }
    خالی cvtRangeConst();  // Convert to smaller representation
    bool isCompound() ثابت override { واپس isString(); }
    bool isIntegralOrPacked() ثابت override { واپس keyword().isIntNumeric(); }
};
class AstBracketArrayDType final : عوامی Ast_گرہ_DType {
    // Associative/Queue/Normal array data type, ie "[dtype_or_expr]"
    // only for early parsing then becomes another data type
    // @astgen op1 := childDTypep : Optional[AstNodeDType] // moved to refDTypep() in V3Width
    // @astgen op2 := elementsp : AstNode  // Number of elements in array
عوامی:
    AstBracketArrayDType(File_سطر* fl, VFlag_بچہ_DType, Ast_گرہ_DType* بچہ_DTypep,
                         Ast_گرہ* elementsp)
        : ASTGEN_SUPER_BracketArrayDType(fl) {
        this->بچہ_DTypep(بچہ_DTypep);
        this->elementsp(elementsp);
    }
    ASTGEN_MEMBERS_AstBracketArrayDType;
    bool similar_DType_گرہ(ثابت Ast_گرہ_DType* samep) ثابت override { واپس this == samep; }
    Ast_گرہ_DType* subDTypep() ثابت override VL_MT_STABLE { واپس بچہ_DTypep(); }
    // METHODS
    // Will be removed in V3Width, which relies on this
    // being a child not a dtype pointed node
    bool maybe_Pointed_مقصد() ثابت override VL_MT_محفوظ { واپس false; }
    AstBasicDType* basicp() ثابت override VL_MT_STABLE { واپس nullptr; }
    صحیح چوڑائی_Align_بائٹیں() ثابت override { V3ERROR_NA_RETURN(0); }
    صحیح چوڑائی_کل_بائٹیں() ثابت override { V3ERROR_NA_RETURN(0); }
    bool isCompound() ثابت override { واپس true; }
};
class AstCDType final : عوامی Ast_گرہ_DType {
    // Raw "C" data type passed directly to output
    string m_name;  // Name of data type, printed when do V3EmitC
عوامی:
    AstCDType(File_سطر* fl, ثابت string& name)
        : ASTGEN_SUPER_CDType(fl)
        , m_name{name} {
        dtypep(this);
    }

    ASTGEN_MEMBERS_AstCDType;
    bool same_گرہ(ثابت Ast_گرہ* samep) ثابت override {
        ثابت AstCDType* ثابت asamep = VN_DBG_AS(samep, CDType);
        واپس m_name == asamep->m_name;
    }
    bool similar_DType_گرہ(ثابت Ast_گرہ_DType* samep) ثابت override { واپس same_گرہ(samep); }
    string name() ثابت override VL_MT_STABLE { واپس m_name; }
    string prettyDTypeName(bool) ثابت override { واپس m_name; }
    // METHODS
    AstBasicDType* basicp() ثابت override VL_MT_STABLE { واپس nullptr; }
    صحیح چوڑائی_Align_بائٹیں() ثابت override { واپس 8; }  // Assume
    صحیح چوڑائی_کل_بائٹیں() ثابت override { واپس 8; }  // Assume
    bool isCompound() ثابت override { واپس true; }
    // Return the global runtime type storing the requested number of bits.
    ساکن string type_مقصد_Hold(صحیح چوڑائی) {
        اگر (چوڑائی <= 8) واپس "::CData";
        اگر (چوڑائی <= 16) واپس "::SData";
        اگر (چوڑائی <= VL_IDATASIZE) واپس "::IData";
        اگر (چوڑائی <= VL_QUADSIZE) واپس "::QData";

        واپس "::VlWide<" + std::مقصد_string(VL_الفاظ_I(چوڑائی)) + ">";
    }
};
class AstClassRefDType final : عوامی Ast_گرہ_DType {
    // Reference to a class
    // @astgen op1 := paramsp: List[AstPin]
    //
    // @astgen ptr := m_classp : Optional[AstClass]  // data type pointed to, BELOW the AstTypedef
    // @astgen ptr := m_classOrPackagep : Optional[AstNodeModule]  // Package hierarchy
    bool m_raw_اشارہ_گر = false;  // Emit as a non-owning C++ pointer rather than VlClassRef
عوامی:
    AstClassRefDType(File_سطر* fl, AstClass* classp, AstPin* paramsp)
        : ASTGEN_SUPER_ClassRefDType(fl)
        , m_classp{classp} {
        dtypep(this);
        جوڑو_Paramsp(paramsp);
    }
    ASTGEN_MEMBERS_AstClassRefDType;
    // METHODS
    bool same_گرہ(ثابت Ast_گرہ* samep) ثابت override {
        ثابت AstClassRefDType* ثابت asamep = VN_DBG_AS(samep, ClassRefDType);
        واپس (m_classp == asamep->m_classp && m_classOrPackagep == asamep->m_classOrPackagep
                && m_raw_اشارہ_گر == asamep->m_raw_اشارہ_گر);
    }
    bool similar_DType_گرہ(ثابت Ast_گرہ_DType* samep) ثابت override;
    خالی dump(std::ostream& str = std::cout) ثابت override;
    خالی dumpJson(std::ostream& str = std::cout) ثابت override;
    خالی dumpSmall(std::ostream& str) ثابت override;
    string prettyDTypeName(bool بھرا_2) ثابت override;
    string pretty_Name_پیغام() ثابت override;
    string name() ثابت override VL_MT_STABLE;
    AstBasicDType* basicp() ثابت override VL_MT_STABLE { واپس nullptr; }
    صحیح چوڑائی_Align_بائٹیں() ثابت override { واپس 0; }
    صحیح چوڑائی_کل_بائٹیں() ثابت override { واپس 0; }
    Ast_گرہ_DType* virtRefDTypep() ثابت override { واپس nullptr; }
    خالی virtRefDTypep(Ast_گرہ_DType* nodep) override {}
    Ast_گرہ_DType* subDTypep() ثابت override VL_MT_STABLE { واپس nullptr; }
    Ast_گرہ_Module* classOrPackagep() ثابت { واپس m_classOrPackagep; }
    خالی classOrPackagep(Ast_گرہ_Module* nodep) { m_classOrPackagep = nodep; }
    AstClass* classp() ثابت VL_MT_STABLE { واپس m_classp; }
    خالی classp(AstClass* nodep) { m_classp = nodep; }
    bool raw_اشارہ_گر() ثابت { واپس m_raw_اشارہ_گر; }
    خالی raw_اشارہ_گر(bool flag) { m_raw_اشارہ_گر = flag; }
    ساکن خالی selfTest();
    bool isCompound() ثابت override { واپس true; }
};
class AstConstDType final : عوامی Ast_گرہ_DType {
    // const data type, ie "const some_dtype var_name [2:0]"
    // ConstDType are removed in V3LinkLValue and become AstVar::isConst.
    // When more generic types are supported AstConstDType will be propagated further.
    // @astgen op1 := childDTypep : Optional[AstNodeDType]
    //
    // @astgen ptr := m_refDTypep : Optional[AstNodeDType]  // Inherit from this base data type
عوامی:
    AstConstDType(File_سطر* fl, VFlag_بچہ_DType, Ast_گرہ_DType* dtp)
        : ASTGEN_SUPER_ConstDType(fl) {
        بچہ_DTypep(dtp);  // Only for parser
        refDTypep(nullptr);  // V3Width will resolve
        dtypep(nullptr);  // V3Width will resolve
        چوڑائی_From_Sub(subDTypep());
    }
    ASTGEN_MEMBERS_AstConstDType;
    ثابت حرف* broken() ثابت override {
        BROKEN_RTN(!((m_refDTypep && !بچہ_DTypep()) || (!m_refDTypep && بچہ_DTypep())));
        واپس nullptr;
    }
    bool same_گرہ(ثابت Ast_گرہ* samep) ثابت override {
        ثابت AstConstDType* ثابت sp = VN_DBG_AS(samep, ConstDType);
        واپس (m_refDTypep == sp->m_refDTypep);
    }
    bool similar_DType_گرہ(ثابت Ast_گرہ_DType* samep) ثابت override {
        واپس skipRefp()->similarDType(samep->skipRefp());
    }
    Ast_گرہ_DType* get_بچہ_DTypep() ثابت override { واپس بچہ_DTypep(); }
    Ast_گرہ_DType* subDTypep() ثابت override VL_MT_STABLE {
        واپس m_refDTypep ? m_refDTypep : بچہ_DTypep();
    }
    خالی refDTypep(Ast_گرہ_DType* nodep) { m_refDTypep = nodep; }
    Ast_گرہ_DType* virtRefDTypep() ثابت override { واپس m_refDTypep; }
    خالی virtRefDTypep(Ast_گرہ_DType* nodep) override { refDTypep(nodep); }
    // METHODS
    AstBasicDType* basicp() ثابت override VL_MT_STABLE { واپس subDTypep()->basicp(); }
    صحیح چوڑائی_Align_بائٹیں() ثابت override { واپس subDTypep()->چوڑائی_Align_بائٹیں(); }
    صحیح چوڑائی_کل_بائٹیں() ثابت override { واپس subDTypep()->چوڑائی_کل_بائٹیں(); }
    bool isCompound() ثابت override {
        v3fatalSrc("call isCompound on subdata type, not reference");
        واپس false;
    }
};
class AstConstraintRefDType final : عوامی Ast_گرہ_DType {
    // For e.g. a reference to constraint for constraint_mode
عوامی:
    explicit AstConstraintRefDType(File_سطر* fl)
        : ASTGEN_SUPER_ConstraintRefDType(fl) {
        dtypep(this);
    }
    ASTGEN_MEMBERS_AstConstraintRefDType;
    bool hasDType() ثابت override VL_MT_محفوظ { واپس true; }
    bool maybe_Pointed_مقصد() ثابت override VL_MT_محفوظ { واپس true; }
    bool undead() ثابت override { واپس true; }
    Ast_گرہ_DType* subDTypep() ثابت override VL_MT_STABLE { واپس nullptr; }
    Ast_گرہ_DType* virtRefDTypep() ثابت override { واپس nullptr; }
    خالی virtRefDTypep(Ast_گرہ_DType* nodep) override {}
    bool similar_DType_گرہ(ثابت Ast_گرہ_DType* samep) ثابت override { واپس this == samep; }
    AstBasicDType* basicp() ثابت override VL_MT_STABLE { واپس nullptr; }
    صحیح چوڑائی_Align_بائٹیں() ثابت override { واپس 1; }
    صحیح چوڑائی_کل_بائٹیں() ثابت override { واپس 1; }
    bool isCompound() ثابت override { واپس false; }
};
class AstCoverCrossDType final : عوامی Ast_گرہ_DType {
    // Borrowed pointer to VlCoverCrossT<...> or construction-time VlCoverCrossDyn.
    ثابت uint32_t m_dimensions;
    ثابت uint32_t m_tuples;  // Fixed capacity number of cross tuples (VlCoverCrossT's Tuples)
    ثابت uint32_t m_bins;  // Fixed capacity number of explicit bins (VlCoverCrossT's Bins)
    ثابت uint32_t m_autoBins;  // Fixed capacity number of auto bins (VlCoverCrossT's AutoBins)
    ثابت uint64_t m_bin_الفاظ;  // Fixed capacity selection words (VlCoverCrossT's BinWords)
    ثابت bool m_dynamic;  // Bin layout is determined at covergroup construction

عوامی:
    AstCoverCrossDType(File_سطر* fl, uint32_t dimensions, uint32_t tuples, uint32_t bins,
                       uint32_t autoBins, uint64_t bin_الفاظ, bool dynamic = false)
        : ASTGEN_SUPER_CoverCrossDType(fl)
        , m_dimensions{dimensions}
        , m_tuples{tuples}
        , m_bins{bins}
        , m_autoBins{autoBins}
        , m_bin_الفاظ{bin_الفاظ}
        , m_dynamic{dynamic} {
        dtypep(this);
    }
    ASTGEN_MEMBERS_AstCoverCrossDType;
    ثابت حرف* broken() ثابت override {
        BROKEN_RTN(m_dimensions == 0);
        واپس nullptr;
    }
    bool same_گرہ(ثابت Ast_گرہ* samep) ثابت override {  // LCOV_EXCL_START
        ثابت AstCoverCrossDType* ثابت sp = VN_DBG_AS(samep, CoverCrossDType);
        واپس dimensions() == sp->dimensions() && tuples() == sp->tuples() && bins() == sp->bins()
               && autoBins() == sp->autoBins() && bin_الفاظ() == sp->bin_الفاظ()
               && isDynamic() == sp->isDynamic();
    }  // LCOV_EXCL_STOP
    bool similar_DType_گرہ(ثابت Ast_گرہ_DType* samep) ثابت override { واپس this == samep; }
    خالی dump(std::ostream& str) ثابت override;
    خالی dumpJson(std::ostream& str) ثابت override;
    خالی dumpSmall(std::ostream& str) ثابت override;
    uint32_t dimensions() ثابت { واپس m_dimensions; }
    uint32_t tuples() ثابت { واپس m_tuples; }
    uint32_t bins() ثابت { واپس m_bins; }
    uint32_t autoBins() ثابت { واپس m_autoBins; }
    uint64_t bin_الفاظ() ثابت { واپس m_bin_الفاظ; }
    bool isDynamic() ثابت { واپس m_dynamic; }
    string cpp_Template_دلائل() ثابت;
    AstBasicDType* basicp() ثابت override VL_MT_STABLE { واپس nullptr; }
    صحیح چوڑائی_Align_بائٹیں() ثابت override { واپس حجم(خالی*); }
    صحیح چوڑائی_کل_بائٹیں() ثابت override { واپس حجم(خالی*); }
    bool isCompound() ثابت override { واپس true; }
};
class AstCoverpointDType final : عوامی Ast_گرہ_DType {
    // Borrowed pointer to a covergroup coverpoint runtime, 'VlCoverpointT<hitBound>*'.
    // Follows pattern of AstQueueDType in capturing the compile-time max bin overlap
    // template argument.
    uint32_t m_کامیابی_Bound;  // VlCoverpointT<> template argument; hit list size, >= 1
عوامی:
    AstCoverpointDType(File_سطر* fl, uint32_t کامیابی_Bound)
        : ASTGEN_SUPER_CoverpointDType(fl)
        , m_کامیابی_Bound{کامیابی_Bound} {
        dtypep(this);
    }
    ASTGEN_MEMBERS_AstCoverpointDType;
    ثابت حرف* broken() ثابت override {
        BROKEN_RTN(m_کامیابی_Bound < 1);
        واپس nullptr;
    }
    // V3Covergroup interns these one-per-hitBound into the type table, so identity is
    // equality; there is never a second node with the same bound to compare against.
    bool similar_DType_گرہ(ثابت Ast_گرہ_DType* samep) ثابت override { واپس this == samep; }
    خالی dumpSmall(std::ostream& str) ثابت override;
    // ACCESSORS
    uint32_t کامیابی_Bound() ثابت { واپس m_کامیابی_Bound; }
    // METHODS
    AstBasicDType* basicp() ثابت override VL_MT_STABLE { واپس nullptr; }
    صحیح چوڑائی_Align_بائٹیں() ثابت override { واپس حجم(خالی*); }
    صحیح چوڑائی_کل_بائٹیں() ثابت override { واپس حجم(خالی*); }
    bool isCompound() ثابت override { واپس true; }
};
class AstDefImplicitDType final : عوامی Ast_گرہ_DType {
    // For parsing enum/struct/unions that are declared with a variable rather than typedef
    // This allows "var enum {...} a,b" to share the enum definition for both variables
    // After link, these become typedefs
    // @astgen op1 := childDTypep : Optional[AstNodeDType]
    string m_name;  // Data type name
    ثابت صحیح m_unique_عدد;  // Unique ID distinguishing this dtype instance, for hashing/naming

عوامی:
    AstDefImplicitDType(File_سطر* fl, ثابت string& name, VFlag_بچہ_DType, Ast_گرہ_DType* dtp)
        : ASTGEN_SUPER_DefImplicitDType(fl)
        , m_name{name}
        , m_unique_عدد{unique_عدد_Inc()} {
        بچہ_DTypep(dtp);  // Only for parser
        dtypep(nullptr);  // V3Width will resolve
    }
    AstDefImplicitDType(ثابت AstDefImplicitDType& other)
        : Ast_گرہ_DType(other)
        , m_name{other.m_name}
        , m_unique_عدد{unique_عدد_Inc()} {}
    ASTGEN_MEMBERS_AstDefImplicitDType;
    صحیح unique_عدد() ثابت { واپس m_unique_عدد; }
    bool same_گرہ(ثابت Ast_گرہ* samep) ثابت override {
        ثابت AstDefImplicitDType* ثابت sp = VN_DBG_AS(samep, DefImplicitDType);
        واپس unique_عدد() == sp->unique_عدد();
    }
    bool similar_DType_گرہ(ثابت Ast_گرہ_DType* samep) ثابت override { واپس same_گرہ(samep); }
    Ast_گرہ_DType* get_بچہ_DTypep() ثابت override { واپس بچہ_DTypep(); }
    Ast_گرہ_DType* subDTypep() ثابت override VL_MT_STABLE {
        واپس dtypep() ? dtypep() : بچہ_DTypep();
    }
    // METHODS
    // op1 = Range of variable
    Ast_گرہ_DType* dtypeSkipRefp() ثابت { واپس dtypep()->skipRefp(); }
    AstBasicDType* basicp() ثابت override VL_MT_STABLE { واپس subDTypep()->basicp(); }
    صحیح چوڑائی_Align_بائٹیں() ثابت override { واپس dtypep()->چوڑائی_Align_بائٹیں(); }
    صحیح چوڑائی_کل_بائٹیں() ثابت override { واپس dtypep()->چوڑائی_کل_بائٹیں(); }
    string name() ثابت override VL_MT_STABLE { واپس m_name; }
    خالی name(ثابت string& flag) override { m_name = flag; }
    bool isCompound() ثابت override { واپس false; }
};
class AstDynArrayDType final : عوامی Ast_گرہ_DType {
    // Dynamic array data type, ie "[]"
    // @astgen op1 := childDTypep : Optional[AstNodeDType] // moved to refDTypep() in V3Width
    //
    // @astgen ptr := m_refDTypep : Optional[AstNodeDType]  // Elements of this type (post-width)
عوامی:
    AstDynArrayDType(File_سطر* fl, VFlag_بچہ_DType, Ast_گرہ_DType* dtp)
        : ASTGEN_SUPER_DynArrayDType(fl) {
        بچہ_DTypep(dtp);  // Only for parser
        refDTypep(nullptr);
        dtypep(nullptr);  // V3Width will resolve
    }
    AstDynArrayDType(File_سطر* fl, Ast_گرہ_DType* dtp)
        : ASTGEN_SUPER_DynArrayDType(fl) {
        refDTypep(dtp);
        dtypep(nullptr);  // V3Width will resolve
    }
    ASTGEN_MEMBERS_AstDynArrayDType;
    ثابت حرف* broken() ثابت override {
        BROKEN_RTN(!((m_refDTypep && !بچہ_DTypep()) || (!m_refDTypep && بچہ_DTypep())));
        واپس nullptr;
    }
    bool same_گرہ(ثابت Ast_گرہ* samep) ثابت override {
        ثابت AstDynArrayDType* ثابت asamep = VN_DBG_AS(samep, DynArrayDType);
        اگر (!asamep->subDTypep()) واپس false;
        واپس subDTypep() == asamep->subDTypep();
    }
    bool similar_DType_گرہ(ثابت Ast_گرہ_DType* samep) ثابت override {
        ثابت AstDynArrayDType* ثابت asamep = VN_DBG_AS(samep, DynArrayDType);
        واپس asamep->subDTypep() && subDTypep()->similarDType(asamep->subDTypep());
    }
    string prettyDTypeName(bool بھرا_2) ثابت override;
    خالی dumpSmall(std::ostream& str) ثابت override;
    Ast_گرہ_DType* get_بچہ_DTypep() ثابت override { واپس بچہ_DTypep(); }
    Ast_گرہ_DType* subDTypep() ثابت override VL_MT_STABLE {
        واپس m_refDTypep ? m_refDTypep : بچہ_DTypep();
    }
    bool isAggregateType() ثابت override { واپس true; }
    خالی refDTypep(Ast_گرہ_DType* nodep) { m_refDTypep = nodep; }
    Ast_گرہ_DType* virtRefDTypep() ثابت override { واپس m_refDTypep; }
    خالی virtRefDTypep(Ast_گرہ_DType* nodep) override { refDTypep(nodep); }
    // METHODS
    AstBasicDType* basicp() ثابت override VL_MT_STABLE { واپس nullptr; }
    صحیح چوڑائی_Align_بائٹیں() ثابت override { واپس subDTypep()->چوڑائی_Align_بائٹیں(); }
    صحیح چوڑائی_کل_بائٹیں() ثابت override { واپس subDTypep()->چوڑائی_کل_بائٹیں(); }
    bool isCompound() ثابت override { واپس true; }
    bool isDynamicallySized() ثابت override { واپس true; }
};
class Ast_خالی_خانے_قطار_DType final : عوامی Ast_گرہ_DType {
    // For EmptyQueue
عوامی:
    explicit Ast_خالی_خانے_قطار_DType(File_سطر* fl)
        : ASTGEN_SUPER_خالی_خانے_قطار_DType(fl) {
        dtypep(this);
    }
    ASTGEN_MEMBERS_Ast_خالی_خانے_قطار_DType;
    خالی dumpSmall(std::ostream& str) ثابت override;
    bool hasDType() ثابت override VL_MT_محفوظ { واپس true; }
    bool maybe_Pointed_مقصد() ثابت override VL_MT_محفوظ { واپس true; }
    bool undead() ثابت override { واپس true; }
    Ast_گرہ_DType* subDTypep() ثابت override VL_MT_STABLE { واپس nullptr; }
    Ast_گرہ_DType* virtRefDTypep() ثابت override { واپس nullptr; }
    خالی virtRefDTypep(Ast_گرہ_DType* nodep) override {}
    bool similar_DType_گرہ(ثابت Ast_گرہ_DType* samep) ثابت override { واپس this == samep; }
    AstBasicDType* basicp() ثابت override VL_MT_STABLE { واپس nullptr; }
    صحیح چوڑائی_Align_بائٹیں() ثابت override { واپس 1; }
    صحیح چوڑائی_کل_بائٹیں() ثابت override { واپس 1; }
    bool isCompound() ثابت override { واپس false; }
};
class AstEnumDType final : عوامی Ast_گرہ_DType {
    // Parents: TYPEDEF/MODULE
    // @astgen op1 := childDTypep : Optional[AstNodeDType]
    // @astgen op2 := itemsp : List[AstEnumItem]
    //
    // @astgen ptr := m_refDTypep : Optional[AstNodeDType]  // Elements of this type (post-width)
    string m_name;  // Name from upper typedef, if any
    string m_typedefName;  // Typedef's dtypeName() once moved off the typedef declaring it
    ثابت صحیح m_unique_عدد;  // Unique ID distinguishing this dtype instance, for hashing/naming

عوامی:
    AstEnumDType(File_سطر* fl, VFlag_بچہ_DType, Ast_گرہ_DType* dtp, Ast_Enum_شے* itemsp)
        : ASTGEN_SUPER_EnumDType(fl)
        , m_unique_عدد{unique_عدد_Inc()} {
        بچہ_DTypep(dtp);  // Only for parser
        refDTypep(nullptr);
        جوڑو_Itemsp(itemsp);
        dtypep(nullptr);  // V3Width will resolve
        چوڑائی_From_Sub(subDTypep());
    }
    AstEnumDType(ثابت AstEnumDType& other)
        : Ast_گرہ_DType{other}
        , m_name{other.m_name}
        , m_typedefName{other.m_typedefName}
        , m_unique_عدد{unique_عدد_Inc()} {}
    ASTGEN_MEMBERS_AstEnumDType;

    ثابت حرف* broken() ثابت override;
    صحیح unique_عدد() ثابت { واپس m_unique_عدد; }
    bool same_گرہ(ثابت Ast_گرہ* samep) ثابت override {
        ثابت AstEnumDType* ثابت sp = VN_DBG_AS(samep, EnumDType);
        واپس unique_عدد() == sp->unique_عدد();
    }
    bool similar_DType_گرہ(ثابت Ast_گرہ_DType* samep) ثابت override { واپس same_گرہ(samep); }
    Ast_گرہ_DType* get_بچہ_DTypep() ثابت override { واپس بچہ_DTypep(); }
    Ast_گرہ_DType* subDTypep() ثابت override VL_MT_STABLE {
        واپس m_refDTypep ? m_refDTypep : بچہ_DTypep();
    }
    خالی refDTypep(Ast_گرہ_DType* nodep) { m_refDTypep = nodep; }
    Ast_گرہ_DType* virtRefDTypep() ثابت override { واپس m_refDTypep; }
    خالی virtRefDTypep(Ast_گرہ_DType* nodep) override { refDTypep(nodep); }
    string name() ثابت override VL_MT_STABLE { واپس m_name; }
    خالی name(ثابت string& flag) override { m_name = flag; }
    // Keep the name the typedef declaring it gives, as it moves off it to the type table
    خالی typedefName(ثابت string& name) { m_typedefName = name; }
    خالی dump(std::ostream& str = std::cout) ثابت override;
    خالی dumpJson(std::ostream& str = std::cout) ثابت override;
    خالی dumpSmall(std::ostream& str) ثابت override;
    string prettyDTypeName(bool بھرا_2) ثابت override;
    // METHODS
    AstBasicDType* basicp() ثابت override VL_MT_STABLE { واپس subDTypep()->basicp(); }
    صحیح چوڑائی_Align_بائٹیں() ثابت override { واپس subDTypep()->چوڑائی_Align_بائٹیں(); }
    صحیح چوڑائی_کل_بائٹیں() ثابت override { واپس subDTypep()->چوڑائی_کل_بائٹیں(); }
    سائز_t شے_گنتی() ثابت {
        سائز_t گنتی = 0;
        برائے (Ast_گرہ* itemp = itemsp(); itemp; itemp = itemp->nextp()) ++گنتی;
        واپس گنتی;
    }
    bool isCompound() ثابت override { واپس false; }
};

class AstIfaceGenericDType final : عوامی Ast_گرہ_DType {
    // Generic interface that will be replaced with AstIfaceRefDType
    File_سطر* m_modportFileline;  // Where modport token was
    // dist-ast-dump-suppress  // Part of name()
    string m_modportName;  // "" = no modport
عوامی:
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
    خالی dumpSmall(std::ostream& str) ثابت override;
    bool hasDType() ثابت override VL_MT_محفوظ { واپس true; }
    bool maybe_Pointed_مقصد() ثابت override VL_MT_محفوظ { واپس true; }
    bool undead() ثابت override { واپس true; }
    Ast_گرہ_DType* subDTypep() ثابت override VL_MT_STABLE { واپس nullptr; }
    Ast_گرہ_DType* virtRefDTypep() ثابت override { واپس nullptr; }
    خالی virtRefDTypep(Ast_گرہ_DType* nodep) override {}
    bool similar_DType_گرہ(ثابت Ast_گرہ_DType* samep) ثابت override { واپس this == samep; }
    AstBasicDType* basicp() ثابت override VL_MT_STABLE { واپس nullptr; }
    صحیح چوڑائی_Align_بائٹیں() ثابت override { واپس 1; }
    صحیح چوڑائی_کل_بائٹیں() ثابت override { واپس 1; }
    string modportName() ثابت { واپس m_modportName; }
    bool isModport() { واپس !m_modportName.خالی_خانے(); }
    bool isCompound() ثابت override { واپس true; }
    File_سطر* modportFileline() ثابت { واپس m_modportFileline; }
    string name() ثابت override { واپس m_modportName; }
};

class AstIfaceRefDType final : عوامی Ast_گرہ_DType {
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
عوامی:
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
        جوڑو_Paramsp(paramsp);
    }
    ASTGEN_MEMBERS_AstIfaceRefDType;

    // METHODS
    خالی dump(std::ostream& str = std::cout) ثابت override;
    خالی dumpJson(std::ostream& str = std::cout) ثابت override;
    خالی dumpSmall(std::ostream& str) ثابت override;
    string prettyDTypeName(bool بھرا_2) ثابت override;
    AstBasicDType* basicp() ثابت override VL_MT_STABLE { واپس nullptr; }
    bool similar_DType_گرہ(ثابت Ast_گرہ_DType* samep) ثابت override {
        // Each occurrence of a virtual interface type parses to its own node,
        // so pointer identity is insufficient; compare the referenced
        // interface instead, same equivalence as AstNode::computeCastable uses.
        ثابت AstIfaceRefDType* ثابت asamep = VN_DBG_AS(samep, IfaceRefDType);
        واپس iface_بذریعہ_Cellp() && iface_بذریعہ_Cellp() == asamep->iface_بذریعہ_Cellp()
               && modportName() == asamep->modportName() && isVirtual() == asamep->isVirtual();
    }
    صحیح چوڑائی_Align_بائٹیں() ثابت override { واپس 0; }
    صحیح چوڑائی_کل_بائٹیں() ثابت override { واپس 0; }
    bool isVirtual() ثابت { واپس m_virtual; }
    خالی isVirtual(bool flag) {
        m_virtual = flag;
        اگر (flag) v3Global.سیٹ_Has_Virt_Ifaces();
    }
    File_سطر* modportFileline() ثابت { واپس m_modportFileline; }
    خالی modportFileline(File_سطر* ثابت modportFileline) { m_modportFileline = modportFileline; }
    string cellName() ثابت { واپس m_cellName; }
    خالی cellName(ثابت string& name) { m_cellName = name; }
    string ifaceName() ثابت { واپس m_ifaceName; }
    string ifaceNameQ() ثابت { واپس "'" + prettyName(ifaceName()) + "'"; }
    خالی ifaceName(ثابت string& name) { m_ifaceName = name; }
    خالی modportName(ثابت string& modportName) { m_modportName = modportName; }
    string modportName() ثابت { واپس m_modportName; }
    AstIface* iface_بذریعہ_Cellp() ثابت;  // Use cellp or ifacep
    AstIface* ifacep() ثابت { واپس m_ifacep; }
    خالی ifacep(AstIface* nodep) { m_ifacep = nodep; }
    AstCell* cellp() ثابت { واپس m_cellp; }
    خالی cellp(AstCell* nodep) { m_cellp = nodep; }
    AstModport* modportp() ثابت { واپس m_modportp; }
    خالی modportp(AstModport* modportp) { m_modportp = modportp; }
    bool isModport() { واپس !m_modportName.خالی_خانے(); }
    bool isCompound() ثابت override { واپس true; }  // But not relevant
};
class AstMemberDType final : عوامی Ast_گرہ_DType {
    // A member of a struct/union
    // PARENT: AstNodeUOrStructDType
    // @astgen op1 := childDTypep : Optional[AstNodeDType]
    // @astgen op3 := valuep : Optional[AstNode]
    //
    // @astgen ptr := m_refDTypep : Optional[AstNodeDType]  // Elements of this type (post-width)
    string m_name;  // Name of variable
    string m_لیبل;  // Holds the string of the verilator tag -- used in JSON output.
    صحیح m_lsb = -1;  // Within this level's packed struct, the LSB of the first bit of the member
    bool m_constrainedRand = false;  // Member has a constraint expression
    VRandAttr m_rand;  // Randomizability of this member (rand, randc, etc)
عوامی:
    AstMemberDType(File_سطر* fl, ثابت string& name, VFlag_بچہ_DType, Ast_گرہ_DType* dtp,
                   Ast_گرہ* valuep)
        : ASTGEN_SUPER_MemberDType(fl)
        , m_name{name} {
        بچہ_DTypep(dtp);  // Only for parser
        this->valuep(valuep);
        dtypep(nullptr);  // V3Width will resolve
        refDTypep(nullptr);
    }
    AstMemberDType(File_سطر* fl, ثابت string& name, Ast_گرہ_DType* dtp)
        : ASTGEN_SUPER_MemberDType(fl)
        , m_name{name} {
        UASSERT(dtp, "AstMember created with no dtype");
        refDTypep(dtp);
        dtypep(this);
        چوڑائی_From_Sub(subDTypep());
    }
    ASTGEN_MEMBERS_AstMemberDType;
    خالی dump(std::ostream& str = std::cout) ثابت override;
    خالی dumpJson(std::ostream& str = std::cout) ثابت override;
    خالی dumpSmall(std::ostream& str) ثابت override;
    string prettyDTypeName(bool بھرا_2) ثابت override;
    string name() ثابت override VL_MT_STABLE { واپس m_name; }  // * = Var name
    bool hasDType() ثابت override VL_MT_محفوظ { واپس true; }
    bool maybe_Pointed_مقصد() ثابت override VL_MT_محفوظ { واپس true; }
    Ast_گرہ_DType* get_بچہ_DTypep() ثابت override { واپس بچہ_DTypep(); }
    Ast_گرہ_UOr_Struct_DType* get_بچہ_Structp();
    Ast_گرہ_DType* subDTypep() ثابت override VL_MT_STABLE {
        واپس m_refDTypep ? m_refDTypep : بچہ_DTypep();
    }
    خالی refDTypep(Ast_گرہ_DType* nodep) { m_refDTypep = nodep; }
    Ast_گرہ_DType* virtRefDTypep() ثابت override { واپس m_refDTypep; }
    خالی virtRefDTypep(Ast_گرہ_DType* nodep) override { refDTypep(nodep); }
    bool similar_DType_گرہ(ثابت Ast_گرہ_DType* samep) ثابت override { واپس this == samep; }
    //
    // (Slow) recurse down to find basic data type (Note don't need virtual -
    // AstVar isn't a NodeDType)
    AstBasicDType* basicp() ثابت override VL_MT_STABLE { واپس subDTypep()->basicp(); }
    // op1 = Range of variable (Note don't need virtual - AstVar isn't a NodeDType)
    Ast_گرہ_DType* dtypeSkipRefp() ثابت { واپس subDTypep()->skipRefp(); }
    // (Slow) recurses - Structure alignment 1,2,4 or 8 bytes (arrays affect this)
    صحیح چوڑائی_Align_بائٹیں() ثابت override { واپس subDTypep()->چوڑائی_Align_بائٹیں(); }
    // (Slow) recurses - Width in bytes rounding up 1,2,4,8,12,...
    صحیح چوڑائی_کل_بائٹیں() ثابت override { واپس subDTypep()->چوڑائی_کل_بائٹیں(); }
    // METHODS
    خالی name(ثابت string& name) override { m_name = name; }
    string لیبل() ثابت override { واپس m_لیبل; }
    خالی لیبل(ثابت string& متن) override { m_لیبل = متن; }
    صحیح lsb() ثابت { واپس m_lsb; }
    خالی lsb(صحیح lsb) { m_lsb = lsb; }
    bool isCompound() ثابت override {
        v3fatalSrc("call isCompound on subdata type, not reference");
        واپس false;
    }
    bool isConstrainedRand() ثابت { واپس m_constrainedRand; }
    خالی markConstrainedRand(bool flag) { m_constrainedRand = flag; }
    VRandAttr rand() ثابت { واپس m_rand; }
    خالی rand(ثابت VRandAttr flag) { m_rand = flag; }
};
class Ast_NBACommit_قطار_DType final : عوامی Ast_گرہ_DType {
    // @astgen ptr := m_subDTypep : AstNodeDType  // Type of the corresponding variable
    ثابت bool m_partial;  // Partial element update required

عوامی:
    Ast_NBACommit_قطار_DType(File_سطر* fl, Ast_گرہ_DType* subDTypep, bool partial)
        : ASTGEN_SUPER_NBACommit_قطار_DType(fl)
        , m_partial{partial}
        , m_subDTypep{subDTypep} {
        dtypep(this);
    }
    ASTGEN_MEMBERS_Ast_NBACommit_قطار_DType;
    خالی dump(std::ostream& str) ثابت override;
    خالی dumpJson(std::ostream& str) ثابت override;
    Ast_گرہ_DType* subDTypep() ثابت override VL_MT_STABLE { واپس m_subDTypep; }
    bool partial() ثابت { واپس m_partial; }
    bool same_گرہ(ثابت Ast_گرہ* samep) ثابت override {
        ثابت Ast_NBACommit_قطار_DType* ثابت asamep = VN_DBG_AS(samep, NBACommit_قطار_DType);
        واپس m_partial == asamep->m_partial;
    }
    bool similar_DType_گرہ(ثابت Ast_گرہ_DType* samep) ثابت override { واپس this == samep; }
    AstBasicDType* basicp() ثابت override VL_MT_STABLE { واپس nullptr; }
    صحیح چوڑائی_Align_بائٹیں() ثابت override { واپس 1; }
    صحیح چوڑائی_کل_بائٹیں() ثابت override { واپس 24; }
    bool isCompound() ثابت override { واپس true; }
};
class AstParamTypeDType final : عوامی Ast_گرہ_DType {
    // Parents: MODULE
    // A parameter type statement; much like a var or typedef
    // @astgen op1 := childDTypep : Optional[AstNodeDType]
    ثابت VVarType m_varType;  // Type of variable (for localparam vs. param)
    ثابت VFwdType m_fwdType;  // Forward type for lint check
    string m_name;  // Name of variable
عوامی:
    AstParamTypeDType(File_سطر* fl, VVarType type, VFwdType fwdType, ثابت string& name,
                      VFlag_بچہ_DType, Ast_گرہ_DType* dtp)
        : ASTGEN_SUPER_ParamTypeDType(fl)
        , m_varType{type}
        , m_fwdType{fwdType}
        , m_name{name} {
        بچہ_DTypep(dtp);  // Only for parser
        dtypep(nullptr);  // V3Width will resolve
    }
    ASTGEN_MEMBERS_AstParamTypeDType;
    خالی dump(std::ostream& str = std::cout) ثابت override;
    خالی dumpJson(std::ostream& str = std::cout) ثابت override;
    string prettyDTypeName(bool بھرا_2) ثابت override;
    Ast_گرہ_DType* get_بچہ_DTypep() ثابت override { واپس بچہ_DTypep(); }
    Ast_گرہ_DType* subDTypep() ثابت override VL_MT_STABLE {
        واپس dtypep() ? dtypep() : بچہ_DTypep();
    }
    AstBasicDType* basicp() ثابت override VL_MT_STABLE { واپس subDTypep()->basicp(); }
    bool similar_DType_گرہ(ثابت Ast_گرہ_DType* samep) ثابت override {
        ثابت AstParamTypeDType* ثابت sp = VN_DBG_AS(samep, ParamTypeDType);
        واپس this->subDTypep()->similarDType(sp->subDTypep());
    }
    صحیح چوڑائی_Align_بائٹیں() ثابت override { واپس dtypep()->چوڑائی_Align_بائٹیں(); }
    صحیح چوڑائی_کل_بائٹیں() ثابت override { واپس dtypep()->چوڑائی_کل_بائٹیں(); }
    // METHODS
    string name() ثابت override VL_MT_STABLE { واپس m_name; }
    bool maybe_Pointed_مقصد() ثابت override VL_MT_محفوظ { واپس true; }
    bool hasDType() ثابت override VL_MT_محفوظ { واپس true; }
    خالی name(ثابت string& flag) override { m_name = flag; }
    VVarType varType() ثابت { واپس m_varType; }  // * = Type of variable
    VFwdType fwdType() ثابت { واپس m_fwdType; }
    bool isParam() ثابت { واپس true; }
    bool isGParam() ثابت { واپس (varType() == VVarType::GPARAM); }
    bool isCompound() ثابت override {
        v3fatalSrc("call isCompound on subdata type, not reference");
        واپس false;
    }
};
class AstParseTypeDType final : عوامی Ast_گرہ_DType {
    // Parents: VAR
    // During parsing, this indicates the type of a parameter is a "parameter type"
    // e.g. the data type is a container of any data type
    ثابت VFwdType m_fwdType;  // Forward type for lint check
عوامی:
    explicit AstParseTypeDType(File_سطر* fl, VFwdType fwdType = VFwdType::NONE)
        : ASTGEN_SUPER_ParseTypeDType(fl)
        , m_fwdType{fwdType} {}
    ASTGEN_MEMBERS_AstParseTypeDType;
    Ast_گرہ_DType* dtypep() ثابت VL_MT_STABLE { واپس nullptr; }
    // METHODS
    خالی dump(std::ostream& str = std::cout) ثابت override;
    خالی dumpJson(std::ostream& str = std::cout) ثابت override;
    bool similar_DType_گرہ(ثابت Ast_گرہ_DType* samep) ثابت override { واپس this == samep; }
    AstBasicDType* basicp() ثابت override VL_MT_STABLE { واپس nullptr; }
    صحیح چوڑائی_Align_بائٹیں() ثابت override { واپس 0; }
    صحیح چوڑائی_کل_بائٹیں() ثابت override { واپس 0; }
    bool isCompound() ثابت override {
        v3fatalSrc("call isCompound on subdata type, not reference");
        واپس false;
    }
    VFwdType fwdType() ثابت { واپس m_fwdType; }
};
class Ast_قطار_DType final : عوامی Ast_گرہ_DType {
    // Queue array data type, ie "[ $ ]"
    // @astgen op1 := childDTypep : Optional[AstNodeDType] // moved to refDTypep() in V3Width
    // @astgen op2 := boundp : Optional[AstNodeExpr]
    //
    // @astgen ptr := m_refDTypep : Optional[AstNodeDType]  // Elements of this type (post-width)
عوامی:
    Ast_قطار_DType(File_سطر* fl, VFlag_بچہ_DType, Ast_گرہ_DType* dtp, Ast_گرہ_اظہاریہ* boundp)
        : ASTGEN_SUPER_قطار_DType(fl) {
        بچہ_DTypep(dtp);
        this->boundp(boundp);
        refDTypep(nullptr);
        dtypep(nullptr);  // V3Width will resolve
    }
    Ast_قطار_DType(File_سطر* fl, Ast_گرہ_DType* dtp, Ast_گرہ_اظہاریہ* boundp)
        : ASTGEN_SUPER_قطار_DType(fl) {
        this->boundp(boundp);
        refDTypep(dtp);
        dtypep(dtp);
    }
    ASTGEN_MEMBERS_Ast_قطار_DType;
    ثابت حرف* broken() ثابت override {
        BROKEN_RTN(!((m_refDTypep && !بچہ_DTypep()) || (!m_refDTypep && بچہ_DTypep())));
        واپس nullptr;
    }
    bool same_گرہ(ثابت Ast_گرہ* samep) ثابت override {
        ثابت Ast_قطار_DType* ثابت asamep = VN_DBG_AS(samep, قطار_DType);
        اگر (!asamep->subDTypep()) واپس false;
        واپس (subDTypep() == asamep->subDTypep());
    }
    bool similar_DType_گرہ(ثابت Ast_گرہ_DType* samep) ثابت override {
        ثابت Ast_قطار_DType* ثابت asamep = VN_DBG_AS(samep, قطار_DType);
        واپس asamep->subDTypep() && subDTypep()->similarDType(asamep->subDTypep());
    }
    خالی dumpSmall(std::ostream& str) ثابت override;
    string prettyDTypeName(bool بھرا_2) ثابت override;
    Ast_گرہ_DType* get_بچہ_DTypep() ثابت override { واپس بچہ_DTypep(); }
    Ast_گرہ_DType* subDTypep() ثابت override VL_MT_STABLE {
        واپس m_refDTypep ? m_refDTypep : بچہ_DTypep();
    }
    bool isAggregateType() ثابت override { واپس true; }
    خالی refDTypep(Ast_گرہ_DType* nodep) { m_refDTypep = nodep; }
    اندرونی صحیح boundConst() ثابت VL_MT_STABLE;
    Ast_گرہ_DType* virtRefDTypep() ثابت override { واپس m_refDTypep; }
    خالی virtRefDTypep(Ast_گرہ_DType* nodep) override { refDTypep(nodep); }
    // METHODS
    AstBasicDType* basicp() ثابت override VL_MT_STABLE { واپس nullptr; }
    صحیح چوڑائی_Align_بائٹیں() ثابت override { واپس subDTypep()->چوڑائی_Align_بائٹیں(); }
    صحیح چوڑائی_کل_بائٹیں() ثابت override { واپس subDTypep()->چوڑائی_کل_بائٹیں(); }
    bool isCompound() ثابت override { واپس true; }
    bool isDynamicallySized() ثابت override { واپس true; }
};
class AstRefDType final : عوامی Ast_گرہ_DType {
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
    ثابت VIface_Capture_لیبل* m_captureTagp = nullptr;

عوامی:
    AstRefDType(File_سطر* fl, ثابت string& name)
        : ASTGEN_SUPER_RefDType(fl)
        , m_name{name} {}
    AstRefDType(File_سطر* fl, ثابت string& name, Ast_گرہ_اظہاریہ* classOrPackagep, AstPin* paramsp)
        : ASTGEN_SUPER_RefDType(fl)
        , m_name{name} {
        classOrPackageOpp(classOrPackagep);
        جوڑو_Paramsp(paramsp);
    }
    class Flag_Type_کا_اظہاریہ {};  // type(expr) for parser only
    AstRefDType(File_سطر* fl, Flag_Type_کا_اظہاریہ, Ast_گرہ* typeofp)
        : ASTGEN_SUPER_RefDType(fl) {
        this->typeofp(typeofp);
        اگر (Ast_گرہ_DType* ثابت dtp = VN_CAST(typeofp, گرہ_DType)) refDTypep(dtp);
    }
    ASTGEN_MEMBERS_AstRefDType;
    // METHODS
    bool same_گرہ(ثابت Ast_گرہ* samep) ثابت override {
        ثابت AstRefDType* ثابت asamep = VN_DBG_AS(samep, RefDType);
        واپس (m_typedefp == asamep->m_typedefp && m_refDTypep == asamep->m_refDTypep
                && m_name == asamep->m_name && m_classOrPackagep == asamep->m_classOrPackagep);
    }
    bool similar_DType_گرہ(ثابت Ast_گرہ_DType* samep) ثابت override {
        واپس subDTypep()->similarDType(samep->subDTypep());
    }
    ثابت حرف* broken() ثابت override;
    خالی dump(std::ostream& str = std::cout) ثابت override;
    خالی dumpJson(std::ostream& str = std::cout) ثابت override;
    خالی dumpSmall(std::ostream& str) ثابت override;
    string name() ثابت override VL_MT_STABLE { واپس m_name; }
    string prettyDTypeName(bool بھرا_2) ثابت override {
        واپس subDTypep() ? subDTypep()->prettyDTypeName(بھرا_2) : prettyName();
    }
    AstBasicDType* basicp() ثابت override VL_MT_STABLE {
        واپس subDTypep() ? subDTypep()->basicp() : nullptr;
    }
    Ast_گرہ_DType* subDTypep() ثابت override VL_MT_STABLE;
    Ast_گرہ_DType* get_بچہ_DTypep() ثابت override { واپس VN_CAST(typeofp(), گرہ_DType); }
    صحیح چوڑائی_Align_بائٹیں() ثابت override { واپس dtypeSkipRefp()->چوڑائی_Align_بائٹیں(); }
    صحیح چوڑائی_کل_بائٹیں() ثابت override { واپس dtypeSkipRefp()->چوڑائی_کل_بائٹیں(); }
    خالی name(ثابت string& flag) override { m_name = flag; }
    Ast_گرہ_DType* dtypeSkipRefp() ثابت { واپس subDTypep()->skipRefp(); }
    AstTypedef* typedefp() ثابت VL_MT_محفوظ { واپس m_typedefp; }
    خالی typedefp(AstTypedef* nodep) { m_typedefp = nodep; }
    Ast_گرہ_DType* refDTypep() ثابت VL_MT_محفوظ { واپس m_refDTypep; }
    خالی refDTypep(Ast_گرہ_DType* nodep) { m_refDTypep = nodep; }
    Ast_گرہ_DType* virtRefDTypep() ثابت override { واپس refDTypep(); }
    خالی virtRefDTypep(Ast_گرہ_DType* nodep) override { refDTypep(nodep); }
    Ast_گرہ_Module* classOrPackagep() ثابت { واپس m_classOrPackagep; }
    خالی classOrPackagep(Ast_گرہ_Module* nodep) { m_classOrPackagep = nodep; }
    ثابت VIface_Capture_لیبل* captureTagp() ثابت { واپس m_captureTagp; }
    خالی captureTagp(ثابت VIface_Capture_لیبل* tagp) { m_captureTagp = tagp; }
    bool isCompound() ثابت override {
        v3fatalSrc("call isCompound on subdata type, not reference");
        واپس false;
    }
};
class AstRequireDType final : عوامی Ast_گرہ_DType {
    // @astgen op1 := lhsp : Optional[AstNode<AstNodeExpr|AstNodeDType>]
    //
    // Require a generic node type (typically AstParseRef become a type.
عوامی:
    AstRequireDType(File_سطر* fl, Ast_گرہ* lhsp)
        : ASTGEN_SUPER_RequireDType(fl) {
        this->lhsp(lhsp);
    }
    ASTGEN_MEMBERS_AstRequireDType;
    // METHODS
    bool similar_DType_گرہ(ثابت Ast_گرہ_DType* samep) ثابت override {
        ثابت AstRequireDType* ثابت asamep = VN_DBG_AS(samep, RequireDType);
        واپس subDTypep()->similarDType(asamep->subDTypep());
    }
    AstBasicDType* basicp() ثابت override VL_MT_STABLE { واپس nullptr; }
    Ast_گرہ_DType* subDTypep() ثابت override VL_MT_STABLE {
        // Used for recursive definition checking
        اگر (Ast_گرہ_DType* ثابت dtp = VN_CAST(lhsp(), گرہ_DType)) واپس dtp;

        واپس nullptr;
    }
    صحیح چوڑائی_Align_بائٹیں() ثابت override { V3ERROR_NA_RETURN(1); }
    صحیح چوڑائی_کل_بائٹیں() ثابت override { V3ERROR_NA_RETURN(1); }
    bool isCompound() ثابت override { V3ERROR_NA_RETURN(false); }
};
class Ast_Sample_قطار_DType final : عوامی Ast_گرہ_DType {
    // @astgen op1 := childDTypep : Optional[AstNodeDType] // moved to refDTypep() in V3Width
    //
    // @astgen ptr := m_refDTypep : Optional[AstNodeDType]  // Elements of this type (post-width)
عوامی:
    Ast_Sample_قطار_DType(File_سطر* fl, Ast_گرہ_DType* dtp)
        : ASTGEN_SUPER_Sample_قطار_DType(fl) {
        refDTypep(dtp);
        dtypep(dtp);
    }
    ASTGEN_MEMBERS_Ast_Sample_قطار_DType;
    ثابت حرف* broken() ثابت override {
        BROKEN_RTN(!((m_refDTypep && !بچہ_DTypep()) || (!m_refDTypep && بچہ_DTypep())));
        واپس nullptr;
    }
    bool same_گرہ(ثابت Ast_گرہ* samep) ثابت override {
        ثابت Ast_Sample_قطار_DType* ثابت asamep = VN_DBG_AS(samep, Sample_قطار_DType);
        اگر (!asamep->subDTypep()) واپس false;
        واپس (subDTypep() == asamep->subDTypep());
    }
    bool similar_DType_گرہ(ثابت Ast_گرہ_DType* samep) ثابت override {
        ثابت Ast_Sample_قطار_DType* ثابت asamep = VN_DBG_AS(samep, Sample_قطار_DType);
        واپس asamep->subDTypep() && subDTypep()->similarDType(asamep->subDTypep());
    }
    خالی dumpSmall(std::ostream& str) ثابت override;
    Ast_گرہ_DType* get_بچہ_DTypep() ثابت override { واپس بچہ_DTypep(); }
    // op1 = Range of variable
    Ast_گرہ_DType* subDTypep() ثابت override VL_MT_STABLE {
        واپس m_refDTypep ? m_refDTypep : بچہ_DTypep();
    }
    خالی refDTypep(Ast_گرہ_DType* nodep) { m_refDTypep = nodep; }
    Ast_گرہ_DType* virtRefDTypep() ثابت override { واپس m_refDTypep; }
    خالی virtRefDTypep(Ast_گرہ_DType* nodep) override { refDTypep(nodep); }
    // METHODS
    AstBasicDType* basicp() ثابت override VL_MT_STABLE { واپس subDTypep()->basicp(); }
    صحیح چوڑائی_Align_بائٹیں() ثابت override { واپس حجم(std::map<std::string, std::string>); }
    صحیح چوڑائی_کل_بائٹیں() ثابت override { واپس حجم(std::map<std::string, std::string>); }
    bool isCompound() ثابت override { واپس true; }
};
class Ast_دھارا_DType final : عوامی Ast_گرہ_DType {
    // Stream data type, used only as data type of stream operations
    // Should behave like AstPackArrayDType, but it doesn't have a size
عوامی:
    explicit Ast_دھارا_DType(File_سطر* fl)
        : ASTGEN_SUPER_دھارا_DType(fl) {
        dtypep(this);
    }
    ASTGEN_MEMBERS_Ast_دھارا_DType;
    خالی dumpSmall(std::ostream& str) ثابت override;
    bool hasDType() ثابت override VL_MT_محفوظ { واپس true; }
    bool maybe_Pointed_مقصد() ثابت override VL_MT_محفوظ { واپس true; }
    bool undead() ثابت override { واپس true; }
    Ast_گرہ_DType* subDTypep() ثابت override VL_MT_STABLE { واپس nullptr; }
    Ast_گرہ_DType* virtRefDTypep() ثابت override { واپس nullptr; }
    خالی virtRefDTypep(Ast_گرہ_DType* nodep) override {}
    bool similar_DType_گرہ(ثابت Ast_گرہ_DType* samep) ثابت override { واپس this == samep; }
    AstBasicDType* basicp() ثابت override VL_MT_STABLE { واپس nullptr; }
    صحیح چوڑائی_Align_بائٹیں() ثابت override { واپس 1; }
    صحیح چوڑائی_کل_بائٹیں() ثابت override { واپس 1; }
    bool isCompound() ثابت override { واپس false; }
};
class AstUnsizedArrayDType final : عوامی Ast_گرہ_DType {
    // Unsized/open-range Array data type, ie "some_dtype var_name []"
    // @astgen op1 := childDTypep : Optional[AstNodeDType] // moved to refDTypep() in V3Width
    //
    // @astgen ptr := m_refDTypep : Optional[AstNodeDType]  // Elements of this type (post-width)
عوامی:
    AstUnsizedArrayDType(File_سطر* fl, VFlag_بچہ_DType, Ast_گرہ_DType* dtp)
        : ASTGEN_SUPER_UnsizedArrayDType(fl) {
        بچہ_DTypep(dtp);  // Only for parser
        refDTypep(nullptr);
        dtypep(nullptr);  // V3Width will resolve
    }
    AstUnsizedArrayDType(File_سطر* fl, Ast_گرہ_DType* dtp)
        : ASTGEN_SUPER_UnsizedArrayDType(fl) {
        refDTypep(dtp);
        dtypep(nullptr);  // V3Width will resolve
    }
    ASTGEN_MEMBERS_AstUnsizedArrayDType;
    ثابت حرف* broken() ثابت override {
        BROKEN_RTN(!((m_refDTypep && !بچہ_DTypep()) || (!m_refDTypep && بچہ_DTypep())));
        واپس nullptr;
    }
    bool same_گرہ(ثابت Ast_گرہ* samep) ثابت override;
    bool similar_DType_گرہ(ثابت Ast_گرہ_DType* samep) ثابت override;
    خالی dumpSmall(std::ostream& str) ثابت override;
    Ast_گرہ_DType* get_بچہ_DTypep() ثابت override { واپس بچہ_DTypep(); }
    Ast_گرہ_DType* subDTypep() ثابت override VL_MT_STABLE {
        واپس m_refDTypep ? m_refDTypep : بچہ_DTypep();
    }
    خالی refDTypep(Ast_گرہ_DType* nodep) { m_refDTypep = nodep; }
    Ast_گرہ_DType* virtRefDTypep() ثابت override { واپس m_refDTypep; }
    خالی virtRefDTypep(Ast_گرہ_DType* nodep) override { refDTypep(nodep); }
    // METHODS
    AstBasicDType* basicp() ثابت override VL_MT_STABLE { واپس subDTypep()->basicp(); }
    صحیح چوڑائی_Align_بائٹیں() ثابت override { واپس subDTypep()->چوڑائی_Align_بائٹیں(); }
    صحیح چوڑائی_کل_بائٹیں() ثابت override { واپس subDTypep()->چوڑائی_کل_بائٹیں(); }
    bool isCompound() ثابت override { واپس true; }
};
class AstVoidDType final : عوامی Ast_گرہ_DType {
    // For e.g. a function returning void
عوامی:
    explicit AstVoidDType(File_سطر* fl)
        : ASTGEN_SUPER_VoidDType(fl) {
        dtypep(this);
    }
    ASTGEN_MEMBERS_AstVoidDType;
    خالی dumpSmall(std::ostream& str) ثابت override;
    bool hasDType() ثابت override VL_MT_محفوظ { واپس true; }
    bool maybe_Pointed_مقصد() ثابت override VL_MT_محفوظ { واپس true; }
    bool undead() ثابت override { واپس true; }
    Ast_گرہ_DType* subDTypep() ثابت override VL_MT_STABLE { واپس nullptr; }
    Ast_گرہ_DType* virtRefDTypep() ثابت override { واپس nullptr; }
    خالی virtRefDTypep(Ast_گرہ_DType* nodep) override {}
    bool similar_DType_گرہ(ثابت Ast_گرہ_DType* samep) ثابت override { واپس true; }
    AstBasicDType* basicp() ثابت override VL_MT_STABLE { واپس nullptr; }
    صحیح چوڑائی_Align_بائٹیں() ثابت override { واپس 1; }
    صحیح چوڑائی_کل_بائٹیں() ثابت override { واپس 1; }
    bool isCompound() ثابت override { واپس false; }
};
class AstWildcardArrayDType final : عوامی Ast_گرہ_DType {
    // Wildcard index type associative array data type, ie "some_dtype var_name [*]"
    // @astgen op1 := childDTypep : Optional[AstNodeDType] // moved to refDTypep() in V3Width
    //
    // @astgen ptr := m_refDTypep : Optional[AstNodeDType]  // Elements of this type (post-width)
عوامی:
    AstWildcardArrayDType(File_سطر* fl, VFlag_بچہ_DType, Ast_گرہ_DType* dtp)
        : ASTGEN_SUPER_WildcardArrayDType(fl) {
        بچہ_DTypep(dtp);  // Only for parser
        refDTypep(nullptr);
        dtypep(nullptr);  // V3Width will resolve
    }
    ASTGEN_MEMBERS_AstWildcardArrayDType;
    ثابت حرف* broken() ثابت override {
        BROKEN_RTN(!((m_refDTypep && !بچہ_DTypep()) || (!m_refDTypep && بچہ_DTypep())));
        واپس nullptr;
    }
    bool same_گرہ(ثابت Ast_گرہ* samep) ثابت override;
    bool similar_DType_گرہ(ثابت Ast_گرہ_DType* samep) ثابت override;
    خالی dumpSmall(std::ostream& str) ثابت override;
    string prettyDTypeName(bool بھرا_2) ثابت override;
    Ast_گرہ_DType* get_بچہ_DTypep() ثابت override { واپس بچہ_DTypep(); }
    Ast_گرہ_DType* subDTypep() ثابت override VL_MT_STABLE {
        واپس m_refDTypep ? m_refDTypep : بچہ_DTypep();
    }
    خالی refDTypep(Ast_گرہ_DType* nodep) { m_refDTypep = nodep; }
    Ast_گرہ_DType* virtRefDTypep() ثابت override { واپس m_refDTypep; }
    خالی virtRefDTypep(Ast_گرہ_DType* nodep) override { refDTypep(nodep); }
    // METHODS
    AstBasicDType* basicp() ثابت override VL_MT_STABLE { واپس subDTypep()->basicp(); }
    صحیح چوڑائی_Align_بائٹیں() ثابت override { واپس حجم(std::map<std::string, std::string>); }
    صحیح چوڑائی_کل_بائٹیں() ثابت override { واپس حجم(std::map<std::string, std::string>); }
    bool isCompound() ثابت override { واپس true; }
    bool isDynamicallySized() ثابت override { واپس true; }
};

// === AstNodeArrayDType ===
class AstPackArrayDType final : عوامی Ast_گرہ_Array_DType {
    // Packed array data type, ie "some_dtype [2:0] var_name"
عوامی:
    اندرونی AstPackArrayDType(File_سطر* fl, VFlag_بچہ_DType, Ast_گرہ_DType* dtp, AstRange* rangep);
    اندرونی AstPackArrayDType(File_سطر* fl, Ast_گرہ_DType* dtp, AstRange* rangep);
    ASTGEN_MEMBERS_AstPackArrayDType;
    string prettyDTypeName(bool بھرا_2) ثابت override;
    bool isCompound() ثابت override { واپس false; }
};
class AstUnpackArrayDType final : عوامی Ast_گرہ_Array_DType {
    // Array data type, ie "some_dtype var_name [2:0]"
    bool m_isCompound = false;  // Non-POD subDType, or parent requires compound
عوامی:
    AstUnpackArrayDType(File_سطر* fl, VFlag_بچہ_DType, Ast_گرہ_DType* dtp, AstRange* rangep)
        : ASTGEN_SUPER_UnpackArrayDType(fl) {
        بچہ_DTypep(dtp);  // Only for parser
        this->rangep(rangep);
        refDTypep(nullptr);
        dtypep(nullptr);  // V3Width will resolve
        // For backward compatibility AstNodeArrayDType and others inherit
        // width and signing from the subDType/base type
        چوڑائی_From_Sub(subDTypep());
    }
    AstUnpackArrayDType(File_سطر* fl, Ast_گرہ_DType* dtp, AstRange* rangep)
        : ASTGEN_SUPER_UnpackArrayDType(fl) {
        this->rangep(rangep);
        refDTypep(dtp);
        dtypep(this);
        // For backward compatibility AstNodeArrayDType and others inherit
        // width and signing from the subDType/base type
        چوڑائی_From_Sub(subDTypep());
    }
    ASTGEN_MEMBERS_AstUnpackArrayDType;
    خالی dump(std::ostream& str) ثابت override;
    خالی dumpJson(std::ostream& str) ثابت override;
    string prettyDTypeName(bool بھرا_2) ثابت override;
    bool same_گرہ(ثابت Ast_گرہ* samep) ثابت override {
        ثابت AstUnpackArrayDType* ثابت sp = VN_DBG_AS(samep, UnpackArrayDType);
        واپس m_isCompound == sp->m_isCompound;
    }
    bool similar_DType_گرہ(ثابت Ast_گرہ_DType* samep) ثابت override;
    bool isAggregateType() ثابت override { واپس true; }
    // True iff 'otherp' is an array of the same shape, with
    // at least one dimension reversed ascending/descending.
    bool is_الٹ_کا(ثابت Ast_گرہ_DType* otherp) ثابت;
    // Outer dimension comes first. The first element is this node.
    std::سمتیہ<AstUnpackArrayDType*> unpackDimensions();
    خالی isCompound(bool flag) { m_isCompound = flag; }
    bool isCompound() ثابت override VL_MT_محفوظ { واپس m_isCompound; }
    bool isIntegralOrPacked() ثابت override { واپس false; }
};

// === AstNodeUOrStructDType ===
class AstStructDType final : عوامی Ast_گرہ_UOr_Struct_DType {
عوامی:
    // VSigning below is mispurposed to indicate if packed or not
    AstStructDType(File_سطر* fl, VSigning numericUnpack)
        : ASTGEN_SUPER_StructDType(fl, numericUnpack) {}
    ASTGEN_MEMBERS_AstStructDType;
    string verilogKwd() ثابت override { واپس "struct"; }
};
class AstUnionDType final : عوامی Ast_گرہ_UOr_Struct_DType {
    bool m_isSoft;  // Is a "union soft"
    bool m_isTagged;  // Is a "union tagged"

عوامی:
    // VSigning below is mispurposed to indicate if packed or not
    // isSoft implies packed
    AstUnionDType(File_سطر* fl, bool isSoft, bool isTagged, VSigning numericUnpack)
        : ASTGEN_SUPER_UnionDType(fl, numericUnpack)
        , m_isSoft{isSoft}
        , m_isTagged{isTagged} {
        packed(packed() | m_isSoft);
    }
    ASTGEN_MEMBERS_AstUnionDType;
    string verilogKwd() ثابت override { واپس "union"; }
    bool isSoft() ثابت { واپس m_isSoft; }
    bool isTagged() ثابت { واپس m_isTagged; }
    bool same_گرہ(ثابت Ast_گرہ* samep) ثابت override;
    خالی dump(std::ostream& str) ثابت override;
    خالی dumpJson(std::ostream& str) ثابت override;
};

#اگر_ختم  // Guard
