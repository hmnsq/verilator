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

#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ VERILATOR_V3ASTNODEDTYPE_H_
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VERILATOR_V3ASTNODEDTYPE_H_

#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ VERILATOR_V3AST_H_
#ਗਲਤੀ "Use V3Ast.h as the include"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Ast.h"  // This helps code analysis tools pick up symbols in V3Ast.h
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VL_NOT_FINAL  // This #define fixes broken code folding in the CLion IDE
#ਜੇ_ਅੰਤ

// === Abstract base node types (AstNode*) =====================================

class Ast_ਗੰਢ_DType VL_NOT_FINAL : ਜਨਤਕ Ast_ਗੰਢ {
    // Ideally width() would migrate to BasicDType as that's where it makes sense,
    // but it's currently so prevalent in the code we leave it here.
    // Note the below members are included in AstTypeTable::Key lookups
    // dist-ast-dump-suppress  // Part of dumpSmall
    ਪੂਰਨ m_ਚੌੜਾਈ = 0;  // (also in AstTypeTable::Key) Bit width of operation
    // (also in AstTypeTable::Key) If unsized, bitwidth of minimum implementation
    // dist-ast-dump-suppress  // Part of dumpSmall
    ਪੂਰਨ m_ਚੌੜਾਈ_Min = 0;
    // dist-ast-dump-suppress  // Part of dumpSmall
    VSigning m_numeric;  // (also in AstTypeTable::Key) Node is signed
    // dist-ast-dump-suppress  // Part of dumpSmall
    bool m_generic = false;  // Simple globally referenced type, don't garbage collect
    // Unique number assigned to each dtype during creation for IEEE matching
    ਸਥਿਰ ਪੂਰਨ s_unique_ਅੰਕ;

ਸੁਰੱਖਿਅਤ:
    // CONSTRUCTORS
    Ast_ਗੰਢ_DType(VNType t, File_ਸਤਰ* fl)
        : Ast_ਗੰਢ{t, fl} {}

ਨਿਜੀ:
    // METHODS
    ਅਚਲ Ast_ਗੰਢ_DType* skipRefIterp(bool skipConst, bool skipEnum,
                                     bool assertOn = true) ਅਚਲ VL_MT_STABLE;

ਸੁਰੱਖਿਅਤ:
    // METHODS
    ਆਭਾਸੀ bool similar_DType_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ_DType* samep) ਅਚਲ = 0;

ਜਨਤਕ:
    ASTGEN_MEMBERS_Ast_ਗੰਢ_DType;
    // ACCESSORS
    ਖਾਲੀ dump(std::ostream& str) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str) ਅਚਲ override;
    ਆਭਾਸੀ ਖਾਲੀ dumpSmall(std::ostream& str) ਅਚਲ VL_MT_STABLE;
    bool hasDType() ਅਚਲ override VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ true; }
    /// Require VlUnpacked, instead of [] for POD elements.
    /// A non-POD object is always compound, but some POD elements
    /// are compound when methods calls operate on object, or when
    /// under another compound-requiring object e.g. class
    ਆਭਾਸੀ bool isCompound() ਅਚਲ = 0;
    // Integral or packed, allowed inside an unpacked union/struct
    ਆਭਾਸੀ bool isIntegralOrPacked() ਅਚਲ { ਵਾਪਸ !isCompound(); }
    // (Slow) recurse down to find basic data type
    ਆਭਾਸੀ AstBasicDType* basicp() ਅਚਲ VL_MT_STABLE = 0;
    // (Slow) Recurse over MemberDType|ParamTypeDType|RefDType|ConstDType|EnumDType to other type
    ਅਚਲ Ast_ਗੰਢ_DType* skipRefp() ਅਚਲ VL_MT_STABLE { ਵਾਪਸ skipRefIterp(true, true); }
    Ast_ਗੰਢ_DType* skipRefp() VL_MT_STABLE {
        ਵਾਪਸ const_cast<Ast_ਗੰਢ_DType*>(
            static_cast<ਅਚਲ Ast_ਗੰਢ_DType*>(this)->skipRefIterp(true, true));
    }
    // (Slow) Recurse over MemberDType|ParamTypeDType|RefDType|ConstDType|EnumDType to other type,
    // Returns null if not resolved
    ਅਚਲ Ast_ਗੰਢ_DType* skipRefOrNullp() ਅਚਲ VL_MT_STABLE {
        ਵਾਪਸ skipRefIterp(true, true, false);
    }
    Ast_ਗੰਢ_DType* skipRefOrNullp() VL_MT_STABLE {
        ਵਾਪਸ const_cast<Ast_ਗੰਢ_DType*>(
            static_cast<ਅਚਲ Ast_ਗੰਢ_DType*>(this)->skipRefIterp(true, true, false));
    }
    // (Slow) Recurse over MemberDType|ParamTypeDType|RefDType|EnumDType to ConstDType
    ਅਚਲ Ast_ਗੰਢ_DType* skip_Ref_ਮੰਜ਼ਿਲ_Constp() ਅਚਲ { ਵਾਪਸ skipRefIterp(false, true); }
    Ast_ਗੰਢ_DType* skip_Ref_ਮੰਜ਼ਿਲ_Constp() {
        ਵਾਪਸ const_cast<Ast_ਗੰਢ_DType*>(
            static_cast<ਅਚਲ Ast_ਗੰਢ_DType*>(this)->skipRefIterp(false, true));
    }
    // (Slow) Recurse over MemberDType|ParamTypeDType|RefDType|ConstDType to EnumDType
    ਅਚਲ Ast_ਗੰਢ_DType* skip_Ref_ਮੰਜ਼ਿਲ_Enump() ਅਚਲ { ਵਾਪਸ skipRefIterp(true, false); }
    Ast_ਗੰਢ_DType* skip_Ref_ਮੰਜ਼ਿਲ_Enump() {
        ਵਾਪਸ const_cast<Ast_ਗੰਢ_DType*>(
            static_cast<ਅਚਲ Ast_ਗੰਢ_DType*>(this)->skipRefIterp(true, false));
    }
    // (Slow) Recurse over MemberDType|ParamTypeDType|RefDType|ConstDType to EnumDType,
    // Returns null if not resolved
    ਅਚਲ Ast_ਗੰਢ_DType* skip_Ref_ਮੰਜ਼ਿਲ_Enum_Or_Nullp() ਅਚਲ { ਵਾਪਸ skipRefIterp(true, false, false); }
    // (Slow) Recurse over MemberDType|ParamTypeDType|RefDType to other type
    ਅਚਲ Ast_ਗੰਢ_DType* skip_Ref_ਮੰਜ਼ਿਲ_Non_Refp() ਅਚਲ { ਵਾਪਸ skipRefIterp(false, false); }
    Ast_ਗੰਢ_DType* skip_Ref_ਮੰਜ਼ਿਲ_Non_Refp() {
        ਵਾਪਸ const_cast<Ast_ਗੰਢ_DType*>(
            static_cast<ਅਚਲ Ast_ਗੰਢ_DType*>(this)->skipRefIterp(false, false));
    }
    // If array, returns element dtype, otherwise returns skipRef dtype
    // If skipRef is false, RefDTypes are not followed (safe before typedef linking)
    ਅਚਲ Ast_ਗੰਢ_DType* elemDTypep(bool skipRef = true) ਅਚਲ VL_MT_STABLE;
    Ast_ਗੰਢ_DType* elemDTypep(bool skipRef = true) VL_MT_STABLE {
        ਵਾਪਸ const_cast<Ast_ਗੰਢ_DType*>(
            static_cast<ਅਚਲ Ast_ਗੰਢ_DType*>(this)->elemDTypep(skipRef));
    }
    // (Slow) recurses - Structure alignment 1,2,4 or 8 bytes (arrays affect this)
    ਆਭਾਸੀ ਪੂਰਨ ਚੌੜਾਈ_Align_ਬਾਈਟਾਂ() ਅਚਲ = 0;
    // (Slow) recurses - Width in bytes rounding up 1,2,4,8,12,...
    ਆਭਾਸੀ ਪੂਰਨ ਚੌੜਾਈ_ਕੁੱਲ_ਬਾਈਟਾਂ() ਅਚਲ = 0;
    bool maybe_Pointed_ਮੰਜ਼ਿਲ() ਅਚਲ override VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ true; }
    // Iff has a non-null refDTypep(), as generic node function
    ਆਭਾਸੀ Ast_ਗੰਢ_DType* virtRefDTypep() ਅਚਲ { ਵਾਪਸ nullptr; }
    // Iff has refDTypep(), set as generic node function
    ਆਭਾਸੀ ਖਾਲੀ virtRefDTypep(Ast_ਗੰਢ_DType* nodep) {}
    // Iff has a non-null second dtypep, as generic node function
    ਆਭਾਸੀ Ast_ਗੰਢ_DType* virtRefDType2p() ਅਚਲ { ਵਾਪਸ nullptr; }
    // Iff has second dtype, set as generic node function
    ਆਭਾਸੀ ਖਾਲੀ virtRefDType2p(Ast_ਗੰਢ_DType* nodep) {}
    // Assignable equivalence.  Calls skipRefToNonRefp() during comparisons.
    bool similarDType(ਅਚਲ Ast_ਗੰਢ_DType* samep) ਅਚਲ;
    // Iff has a non-null subDTypep(), as generic node function
    ਆਭਾਸੀ Ast_ਗੰਢ_DType* subDTypep() ਅਚਲ VL_MT_STABLE { ਵਾਪਸ nullptr; }
    ਆਭਾਸੀ Ast_ਗੰਢ_DType* subDType2p() ਅਚਲ VL_MT_STABLE { ਵਾਪਸ nullptr; }
    ਆਭਾਸੀ bool isAggregateType() ਅਚਲ { ਵਾਪਸ false; }
    // True for unpacked, dynamic, queue, and associative arrays (not packed arrays)
    bool isNonPackedArray() ਅਚਲ;
    ਆਭਾਸੀ bool isFourstate() ਅਚਲ;
    // Ideally an IEEE $typename
    ਆਭਾਸੀ string prettyDTypeName(bool) ਅਚਲ { ਵਾਪਸ prettyTypeName(); }
    string prettyDTypeNameQ(bool ਭਰਿਆ_2 = false) ਅਚਲ { ਵਾਪਸ "'" + prettyDTypeName(ਭਰਿਆ_2) + "'"; }
    string stateDTypeName() ਅਚਲ { ਵਾਪਸ this->isFourstate() ? "(4-state)" : "(2-state)"; }
    //
    // Changing the width may confuse the data type resolution, so must clear
    // TypeTable cache after use.
    ਖਾਲੀ ਚੌੜਾਈ_Force(ਪੂਰਨ ਚੌੜਾਈ, ਪੂਰਨ ਚੌੜਾਈ_Min) {
        m_ਚੌੜਾਈ = ਚੌੜਾਈ;
        m_ਚੌੜਾਈ_Min = ਚੌੜਾਈ_Min;
    }
    // For backward compatibility inherit width and signing from the subDType/base type
    ਖਾਲੀ ਚੌੜਾਈ_From_Sub(ਅਚਲ Ast_ਗੰਢ_DType* nodep) {
        m_ਚੌੜਾਈ = nodep->m_ਚੌੜਾਈ;
        m_ਚੌੜਾਈ_Min = nodep->m_ਚੌੜਾਈ_Min;
        m_numeric = nodep->m_numeric;
    }
    //
    ਪੂਰਨ ਚੌੜਾਈ() ਅਚਲ VL_MT_STABLE { ਵਾਪਸ m_ਚੌੜਾਈ; }
    ਖਾਲੀ numeric(VSigning flag) { m_numeric = flag; }
    bool isSigned() ਅਚਲ VL_MT_STABLE { ਵਾਪਸ m_numeric.isSigned(); }
    bool isNosign() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ m_numeric.isNosign(); }
    VSigning numeric() ਅਚਲ VL_MT_STABLE { ਵਾਪਸ m_numeric; }
    ਪੂਰਨ ਚੌੜਾਈ_ਸ਼ਬਦ() ਅਚਲ VL_MT_STABLE { ਵਾਪਸ VL_ਸ਼ਬਦ_I(ਚੌੜਾਈ()); }
    ਪੂਰਨ ਚੌੜਾਈ_Min() ਅਚਲ VL_MT_STABLE {  // If sized, the size,
                                         // if unsized the min digits to represent it
        ਵਾਪਸ m_ਚੌੜਾਈ_Min ? m_ਚੌੜਾਈ_Min : m_ਚੌੜਾਈ;
    }
    ਪੂਰਨ ਚੌੜਾਈ_Pow2() ਅਚਲ;
    ਖਾਲੀ ਚੌੜਾਈ_Min_From_ਚੌੜਾਈ() { m_ਚੌੜਾਈ_Min = m_ਚੌੜਾਈ; }
    bool ਚੌੜਾਈ_Sized() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ !m_ਚੌੜਾਈ_Min || m_ਚੌੜਾਈ_Min == m_ਚੌੜਾਈ; }
    bool generic() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ m_generic; }
    ਖਾਲੀ generic(bool flag) { m_generic = flag; }
    std::pair<uint32_t, uint32_t> dimensions(bool includeBasic) ਅਚਲ;
    uint32_t arrayUnpackedElements() ਅਚਲ;  // 1, or total multiplication of all dimensions
    // Fixed aggregate streaming properties
    bool isStreamableFixedAggregate() ਅਚਲ;
    // Whether is a string, or an unpacked array or structure containing one
    bool ਸ਼ਾਮਲ_ਹੈ_String() ਅਚਲ;
    bool ਸ਼ਾਮਲ_ਹੈ_Unpacked_Struct() ਅਚਲ;
    ਪੂਰਨ ਚੌੜਾਈ_ਧਾਰਾ() ਅਚਲ;
    string vlEnumType() ਅਚਲ;  // Return VerilatedVarType: VLVT_UINT32, etc
    string vlEnumPackedAgg() ਅਚਲ;  // Return VLVF_PACKED_AGG etc flags, or empty
    ਸਥਿਰ ਪੂਰਨ unique_ਅੰਕ_Inc() { ਵਾਪਸ ++s_unique_ਅੰਕ; }
    ਅਚਲ ਅੱਖਰ* charIQWN() ਅਚਲ {
        ਵਾਪਸ (isString() ? "N" : isWide() ? "W" : isDouble() ? "D" : isQuad() ? "Q" : "I");
    }
    string cType(ਅਚਲ string& name, bool forFunc, bool isRef, bool packed = false) ਅਚਲ;
    // Represents a C++ LiteralType? (can be constexpr)
    bool isLiteralType() ਅਚਲ VL_MT_STABLE;
    // C++ type (as emitted by cType) is trivially default constructible and destructible?
    bool isTriviallyConstructible() ਅਚਲ VL_MT_STABLE;
    ਆਭਾਸੀ bool isDynamicallySized() ਅਚਲ { ਵਾਪਸ false; }

ਨਿਜੀ:
    class CTypeRecursed;
    CTypeRecursed cTypeRecurse(bool compound, bool packed) ਅਚਲ;
};
class Ast_ਗੰਢ_Array_DType VL_NOT_FINAL : ਜਨਤਕ Ast_ਗੰਢ_DType {
    // Array data type, ie "some_dtype var_name [2:0]"
    // @astgen op1 := childDTypep : Optional[AstNodeDType] // moved to refDTypep() in V3Width
    // @astgen op2 := rangep : Optional[AstRange] // array bounds
    //
    // @astgen ptr := m_refDTypep : Optional[AstNodeDType]  // Elements of this type (post-width)
    Ast_ਗੰਢ* rangenp() ਅਚਲ { ਵਾਪਸ reinterpret_cast<Ast_ਗੰਢ*>(rangep()); }

ਸੁਰੱਖਿਅਤ:
    Ast_ਗੰਢ_Array_DType(VNType t, File_ਸਤਰ* fl)
        : Ast_ਗੰਢ_DType{t, fl} {}

ਜਨਤਕ:
    ASTGEN_MEMBERS_Ast_ਗੰਢ_Array_DType;
    ਖਾਲੀ dump(std::ostream& str) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str) ਅਚਲ override;
    ਖਾਲੀ dumpSmall(std::ostream& str) ਅਚਲ override;
    ਅਚਲ ਅੱਖਰ* broken() ਅਚਲ override {
        BROKEN_RTN(!((m_refDTypep && !ਬੱਚਾ_DTypep()) || (!m_refDTypep && ਬੱਚਾ_DTypep())));
        ਵਾਪਸ nullptr;
    }
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* samep) ਅਚਲ override {
        ਅਚਲ Ast_ਗੰਢ_Array_DType* ਅਚਲ asamep = VN_DBG_AS(samep, ਗੰਢ_Array_DType);
        ਵਾਪਸ hi() == asamep->hi() && rangenp()->same_ਰੁੱਖ(asamep->rangenp())
               && subDTypep() == asamep->subDTypep();
    }  // HashedDT doesn't recurse, so need to check children
    bool similar_DType_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ_DType* samep) ਅਚਲ override {
        ਅਚਲ Ast_ਗੰਢ_Array_DType* ਅਚਲ asamep = VN_DBG_AS(samep, ਗੰਢ_Array_DType);
        ਵਾਪਸ elementsConst() == asamep->elementsConst()
               && subDTypep()->similarDType(asamep->subDTypep());
    }
    Ast_ਗੰਢ_DType* get_ਬੱਚਾ_DTypep() ਅਚਲ override { ਵਾਪਸ ਬੱਚਾ_DTypep(); }
    Ast_ਗੰਢ_DType* subDTypep() ਅਚਲ override VL_MT_STABLE {
        ਵਾਪਸ m_refDTypep ? m_refDTypep : ਬੱਚਾ_DTypep();
    }
    ਖਾਲੀ refDTypep(Ast_ਗੰਢ_DType* nodep) { m_refDTypep = nodep; }
    Ast_ਗੰਢ_DType* virtRefDTypep() ਅਚਲ override { ਵਾਪਸ m_refDTypep; }
    ਖਾਲੀ virtRefDTypep(Ast_ਗੰਢ_DType* nodep) override { refDTypep(nodep); }
    // METHODS
    AstBasicDType* basicp() ਅਚਲ override VL_MT_STABLE {
        ਵਾਪਸ subDTypep()->basicp();
    }  // (Slow) recurse down to find basic data type
    ਪੂਰਨ ਚੌੜਾਈ_Align_ਬਾਈਟਾਂ() ਅਚਲ override { ਵਾਪਸ subDTypep()->ਚੌੜਾਈ_Align_ਬਾਈਟਾਂ(); }
    ਪੂਰਨ ਚੌੜਾਈ_ਕੁੱਲ_ਬਾਈਟਾਂ() ਅਚਲ override {
        ਵਾਪਸ elementsConst() * subDTypep()->ਚੌੜਾਈ_ਕੁੱਲ_ਬਾਈਟਾਂ();
    }
    ਸਤਰਅੰਦਰ ਪੂਰਨ ਖੱਬਾ() ਅਚਲ VL_MT_STABLE;
    ਸਤਰਅੰਦਰ ਪੂਰਨ ਸੱਜਾ() ਅਚਲ VL_MT_STABLE;
    ਸਤਰਅੰਦਰ ਪੂਰਨ hi() ਅਚਲ VL_MT_STABLE;
    ਸਤਰਅੰਦਰ ਪੂਰਨ lo() ਅਚਲ VL_MT_STABLE;
    ਸਤਰਅੰਦਰ ਪੂਰਨ elementsConst() ਅਚਲ VL_MT_STABLE;
    ਸਤਰਅੰਦਰ VNumRange declRange() ਅਚਲ VL_MT_STABLE;
};
class Ast_ਗੰਢ_UOr_Struct_DType VL_NOT_FINAL : ਜਨਤਕ Ast_ਗੰਢ_DType {
    // A struct or union; common handling
    // @astgen op1 := membersp : List[AstMemberDType]
    //
    // @astgen ptr := m_classOrPackagep : Optional[AstNodeModule]  // Package emitted with
    string m_name;  // Name from upper typedef, if any
    string m_typedefName;  // Typedef's dtypeName() once moved off the typedef declaring it
    ਅਚਲ ਪੂਰਨ m_unique_ਅੰਕ;  // Unique ID distinguishing this dtype instance, for hashing/naming
    bool m_packed;  // Packed struct/union, else unpacked
    bool m_isFourstate = false;  // V3Width computes; true if any member is 4-state
    bool m_constrainedRand = false;  // True if struct has constraint expression
    bool m_emit_ਮੰਜ਼ਿਲ_String = false;  // Generate to_string() for this struct/union if set

ਸੁਰੱਖਿਅਤ:
    Ast_ਗੰਢ_UOr_Struct_DType(VNType t, File_ਸਤਰ* fl, VSigning numericUnpack)
        : Ast_ਗੰਢ_DType{t, fl}
        , m_unique_ਅੰਕ{unique_ਅੰਕ_Inc()} {
        // VSigning::NOSIGN overloaded to indicate not packed
        m_packed = (numericUnpack != VSigning::NOSIGN);
        numeric(VSigning::fromBool(numericUnpack.isSigned()));
    }
    Ast_ਗੰਢ_UOr_Struct_DType(ਅਚਲ Ast_ਗੰਢ_UOr_Struct_DType& other)
        : Ast_ਗੰਢ_DType{other}
        , m_name{other.m_name}
        , m_typedefName{other.m_typedefName}
        , m_unique_ਅੰਕ{unique_ਅੰਕ_Inc()}
        , m_packed{other.m_packed}
        , m_isFourstate{other.m_isFourstate} {}

ਜਨਤਕ:
    ASTGEN_MEMBERS_Ast_ਗੰਢ_UOr_Struct_DType;
    ਪੂਰਨ unique_ਅੰਕ() ਅਚਲ { ਵਾਪਸ m_unique_ਅੰਕ; }
    ਖਾਲੀ dump(std::ostream& str) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str) ਅਚਲ override;
    string prettyDTypeName(bool) ਅਚਲ override;
    bool isCompound() ਅਚਲ override { ਵਾਪਸ !packed(); }
    // For basicp() we reuse the size to indicate a "fake" basic type of same size
    AstBasicDType* basicp() ਅਚਲ override VL_MT_STABLE {
        ਜੇ (!m_packed) ਵਾਪਸ nullptr;
        ਵਾਪਸ (isFourstate()
                    ? VN_AS(ਲੱਭੋ_Logic_Range_DType(VNumRange{ਚੌੜਾਈ() - 1, 0}, ਚੌੜਾਈ(), numeric()),
                            BasicDType)
                    : VN_AS(ਲੱਭੋ_ਦੋਆਧਾਰੀ_ਅੰਕ_Range_DType(VNumRange{ਚੌੜਾਈ() - 1, 0}, ਚੌੜਾਈ(), numeric()),
                            BasicDType));
    }
    // (Slow) recurses - Structure alignment 1,2,4 or 8 bytes (arrays affect this)
    ਪੂਰਨ ਚੌੜਾਈ_Align_ਬਾਈਟਾਂ() ਅਚਲ override;
    // (Slow) recurses - Width in bytes rounding up 1,2,4,8,12,...
    ਪੂਰਨ ਚੌੜਾਈ_ਕੁੱਲ_ਬਾਈਟਾਂ() ਅਚਲ override;
    bool similar_DType_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ_DType* samep) ਅਚਲ override;
    string name() ਅਚਲ override VL_MT_STABLE { ਵਾਪਸ m_name; }
    ਖਾਲੀ name(ਅਚਲ string& flag) override { m_name = flag; }
    // Keep the name the typedef declaring it gives, as it moves off it to the type table
    ਖਾਲੀ typedefName(ਅਚਲ string& name) { m_typedefName = name; }
    // Name of the C structure for DPI, empty if no typedef names it, so declared where used
    string dpiName() ਅਚਲ;
    // C typedef of the unpacked structure for DPI (IEEE 1800-2023 H.7.3)
    string dpiTypedef() ਅਚਲ;
    bool packed() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ m_packed; }
    ਖਾਲੀ packed(bool flag) { m_packed = flag; }
    // packed() but as don't support unpacked, presently all structs
    ਸਥਿਰ bool packedUnsup() { ਵਾਪਸ true; }
    ਖਾਲੀ isFourstate(bool flag) { m_isFourstate = flag; }
    bool isFourstate() ਅਚਲ override VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ m_isFourstate; }
    ਸਥਿਰ ਪੂਰਨ lo() VL_MT_STABLE { ਵਾਪਸ 0; }
    ਪੂਰਨ hi() ਅਚਲ VL_MT_STABLE {
        ਵਾਪਸ dtypep()->ਚੌੜਾਈ() - 1;
    }  // Packed classes look like arrays
    VNumRange declRange() ਅਚਲ VL_MT_STABLE { ਵਾਪਸ VNumRange{hi(), lo()}; }
    Ast_ਗੰਢ_Module* classOrPackagep() ਅਚਲ { ਵਾਪਸ m_classOrPackagep; }
    ਖਾਲੀ classOrPackagep(Ast_ਗੰਢ_Module* classpackagep) { m_classOrPackagep = classpackagep; }
    bool isConstrainedRand() ਅਚਲ { ਵਾਪਸ m_constrainedRand; }
    ਖਾਲੀ markConstrainedRand(bool flag) { m_constrainedRand = flag; }
    bool emit_ਮੰਜ਼ਿਲ_String() ਅਚਲ { ਵਾਪਸ m_emit_ਮੰਜ਼ਿਲ_String; }
    ਖਾਲੀ ਸੈੱਟ_Emit_ਮੰਜ਼ਿਲ_String() { m_emit_ਮੰਜ਼ਿਲ_String = true; }
    bool isAggregateType() ਅਚਲ override { ਵਾਪਸ !packed(); }
};

// === Concrete node types =====================================================

// === AstNode ===
class Ast_Enum_ਵਸਤੂ final : ਜਨਤਕ Ast_ਗੰਢ {
    // @astgen op1 := rangep : Optional[AstRange] // Range for name appending
    // @astgen op2 := valuep : Optional[AstNodeExpr]
    string m_name;  // Name of item

ਜਨਤਕ:
    // Parents: ENUM
    Ast_Enum_ਵਸਤੂ(File_ਸਤਰ* fl, ਅਚਲ string& name, AstRange* rangep, Ast_ਗੰਢ_ਵਿਅੰਜਕ* valuep)
        : ASTGEN_SUPER_Enum_ਵਸਤੂ(fl)
        , m_name{name} {
        this->rangep(rangep);
        this->valuep(valuep);
    }
    ASTGEN_MEMBERS_Ast_Enum_ਵਸਤੂ;
    string name() ਅਚਲ override VL_MT_STABLE { ਵਾਪਸ m_name; }
    bool maybe_Pointed_ਮੰਜ਼ਿਲ() ਅਚਲ override VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ true; }
    bool hasDType() ਅਚਲ override VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ true; }
    ਖਾਲੀ name(ਅਚਲ string& flag) override { m_name = flag; }
};

// === AstNodeDType ===
class AstAssocArrayDType final : ਜਨਤਕ Ast_ਗੰਢ_DType {
    // Associative array data type, ie "[some_dtype]"
    // @astgen op1 := childDTypep : Optional[AstNodeDType] // moved to refDTypep() in V3Width
    // @astgen op2 := keyChildDTypep : Optional[AstNodeDType]
    //
    // @astgen ptr := m_refDTypep : Optional[AstNodeDType]  // Elements of this type (post-width)
    // @astgen ptr := m_keyDTypep : Optional[AstNodeDType]  // Keys of this type (post-width)
ਜਨਤਕ:
    AstAssocArrayDType(File_ਸਤਰ* fl, VFlag_ਬੱਚਾ_DType, Ast_ਗੰਢ_DType* dtp, Ast_ਗੰਢ_DType* ਕੁੰਜੀ_Dtp)
        : ASTGEN_SUPER_AssocArrayDType(fl) {
        ਬੱਚਾ_DTypep(dtp);  // Only for parser
        ਕੁੰਜੀ_ਬੱਚਾ_DTypep(ਕੁੰਜੀ_Dtp);  // Only for parser
        refDTypep(nullptr);
        ਕੁੰਜੀ_DTypep(nullptr);
        dtypep(nullptr);  // V3Width will resolve
    }
    AstAssocArrayDType(File_ਸਤਰ* fl, Ast_ਗੰਢ_DType* dtp, Ast_ਗੰਢ_DType* ਕੁੰਜੀ_Dtp)
        : ASTGEN_SUPER_AssocArrayDType(fl) {
        refDTypep(dtp);
        ਕੁੰਜੀ_DTypep(ਕੁੰਜੀ_Dtp);
        dtypep(dtp);
    }
    ASTGEN_MEMBERS_AstAssocArrayDType;
    ਅਚਲ ਅੱਖਰ* broken() ਅਚਲ override {
        BROKEN_RTN(!((m_refDTypep && !ਬੱਚਾ_DTypep()) || (!m_refDTypep && ਬੱਚਾ_DTypep())));
        BROKEN_RTN(!((m_ਕੁੰਜੀ_DTypep && !ਬੱਚਾ_DTypep()) || (!m_ਕੁੰਜੀ_DTypep && ਬੱਚਾ_DTypep())));
        ਵਾਪਸ nullptr;
    }
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* samep) ਅਚਲ override {
        ਅਚਲ AstAssocArrayDType* ਅਚਲ asamep = VN_DBG_AS(samep, AssocArrayDType);
        ਜੇ (!asamep->subDTypep()) ਵਾਪਸ false;
        ਜੇ (!asamep->ਕੁੰਜੀ_DTypep()) ਵਾਪਸ false;
        ਵਾਪਸ (subDTypep() == asamep->subDTypep() && ਕੁੰਜੀ_DTypep() == asamep->ਕੁੰਜੀ_DTypep());
    }
    bool similar_DType_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ_DType* samep) ਅਚਲ override {
        ਅਚਲ AstAssocArrayDType* ਅਚਲ asamep = VN_DBG_AS(samep, AssocArrayDType);
        ਵਾਪਸ asamep->subDTypep() && subDTypep()->similarDType(asamep->subDTypep())
               && asamep->ਕੁੰਜੀ_DTypep() && ਕੁੰਜੀ_DTypep()->similarDType(asamep->ਕੁੰਜੀ_DTypep());
    }
    string prettyDTypeName(bool ਭਰਿਆ_2) ਅਚਲ override;
    ਖਾਲੀ dumpSmall(std::ostream& str) ਅਚਲ override;
    Ast_ਗੰਢ_DType* get_ਬੱਚਾ_DTypep() ਅਚਲ override { ਵਾਪਸ ਬੱਚਾ_DTypep(); }
    Ast_ਗੰਢ_DType* getChild2DTypep() ਅਚਲ override { ਵਾਪਸ ਕੁੰਜੀ_ਬੱਚਾ_DTypep(); }
    Ast_ਗੰਢ_DType* subDTypep() ਅਚਲ override VL_MT_STABLE {
        ਵਾਪਸ m_refDTypep ? m_refDTypep : ਬੱਚਾ_DTypep();
    }
    Ast_ਗੰਢ_DType* subDType2p() ਅਚਲ override VL_MT_STABLE {
        ਵਾਪਸ m_ਕੁੰਜੀ_DTypep ? m_ਕੁੰਜੀ_DTypep : ਕੁੰਜੀ_ਬੱਚਾ_DTypep();
    }
    bool isAggregateType() ਅਚਲ override { ਵਾਪਸ true; }
    ਖਾਲੀ refDTypep(Ast_ਗੰਢ_DType* nodep) { m_refDTypep = nodep; }
    Ast_ਗੰਢ_DType* virtRefDTypep() ਅਚਲ override { ਵਾਪਸ m_refDTypep; }
    ਖਾਲੀ virtRefDTypep(Ast_ਗੰਢ_DType* nodep) override { refDTypep(nodep); }
    Ast_ਗੰਢ_DType* virtRefDType2p() ਅਚਲ override { ਵਾਪਸ m_ਕੁੰਜੀ_DTypep; }
    ਖਾਲੀ virtRefDType2p(Ast_ਗੰਢ_DType* nodep) override { ਕੁੰਜੀ_DTypep(nodep); }
    //
    Ast_ਗੰਢ_DType* ਕੁੰਜੀ_DTypep() ਅਚਲ VL_MT_STABLE {
        ਵਾਪਸ m_ਕੁੰਜੀ_DTypep ? m_ਕੁੰਜੀ_DTypep : ਕੁੰਜੀ_ਬੱਚਾ_DTypep();
    }
    ਖਾਲੀ ਕੁੰਜੀ_DTypep(Ast_ਗੰਢ_DType* nodep) { m_ਕੁੰਜੀ_DTypep = nodep; }
    // METHODS
    AstBasicDType* basicp() ਅਚਲ override VL_MT_STABLE { ਵਾਪਸ nullptr; }
    ਪੂਰਨ ਚੌੜਾਈ_Align_ਬਾਈਟਾਂ() ਅਚਲ override { ਵਾਪਸ subDTypep()->ਚੌੜਾਈ_Align_ਬਾਈਟਾਂ(); }
    ਪੂਰਨ ਚੌੜਾਈ_ਕੁੱਲ_ਬਾਈਟਾਂ() ਅਚਲ override { ਵਾਪਸ subDTypep()->ਚੌੜਾਈ_ਕੁੱਲ_ਬਾਈਟਾਂ(); }
    bool isCompound() ਅਚਲ override { ਵਾਪਸ true; }
    bool isDynamicallySized() ਅਚਲ override { ਵਾਪਸ true; }
};
class AstBasicDType final : ਜਨਤਕ Ast_ਗੰਢ_DType {
    // Builtin atomic/vectored data type
    // @astgen op1 := rangep : Optional[AstRange] // Range of variable
    ਬਣਤਰ Members final {
        VBasicDTypeKwd m_keyword;  // (also in VBasicTypeKey) What keyword created basic type
        // dist-ast-dump-suppress  // Part of dumpSmall
        VNumRange m_nrange;  // (also in VBasicTypeKey) Numeric msb/lsb (if non-opaque keyword)
        bool ਸੰਚਾਲਕ==(ਅਚਲ Members& rhs) ਅਚਲ {
            ਵਾਪਸ rhs.m_keyword == m_keyword && rhs.m_nrange == m_nrange;
        }
    } m;
    // See also in AstNodeDType: m_width, m_widthMin, m_numeric(issigned)
ਜਨਤਕ:
    AstBasicDType(File_ਸਤਰ* fl, VBasicDTypeKwd kwd, ਅਚਲ VSigning& signst = VSigning::NOSIGN)
        : ASTGEN_SUPER_BasicDType(fl) {
        init(kwd, signst, 0, -1, nullptr);
    }
    AstBasicDType(File_ਸਤਰ* fl, VFlagLogicPacked, ਪੂਰਨ wantwidth)
        : ASTGEN_SUPER_BasicDType(fl) {
        init(VBasicDTypeKwd::LOGIC, VSigning::NOSIGN, wantwidth, -1, nullptr);
    }
    AstBasicDType(File_ਸਤਰ* fl, VFlagLogic2StatePacked, ਪੂਰਨ wantwidth)
        : ASTGEN_SUPER_BasicDType(fl) {
        init(VBasicDTypeKwd::LOGIC2STATE, VSigning::NOSIGN, wantwidth, -1, nullptr);
    }
    AstBasicDType(File_ਸਤਰ* fl, VFlag_ਦੋਆਧਾਰੀ_ਅੰਕ_Packed, ਪੂਰਨ wantwidth)
        : ASTGEN_SUPER_BasicDType(fl) {
        init(VBasicDTypeKwd::ਦੋਆਧਾਰੀ_ਅੰਕ, VSigning::NOSIGN, wantwidth, -1, nullptr);
    }
    AstBasicDType(File_ਸਤਰ* fl, VBasicDTypeKwd kwd, VSigning numer, ਪੂਰਨ wantwidth, ਪੂਰਨ widthmin)
        : ASTGEN_SUPER_BasicDType(fl) {
        init(kwd, numer, wantwidth, widthmin, nullptr);
    }
    AstBasicDType(File_ਸਤਰ* fl, VBasicDTypeKwd kwd, VSigning numer, VNumRange range, ਪੂਰਨ widthmin)
        : ASTGEN_SUPER_BasicDType(fl) {
        init(kwd, numer, range.elements(), widthmin, nullptr);
        m.m_nrange = range;  // as init() presumes lsb==0, but range.lsb() might not be
    }
    // See also addRange in verilog.y
ਨਿਜੀ:
    ਖਾਲੀ init(VBasicDTypeKwd kwd, VSigning numer, ਪੂਰਨ wantwidth, ਪੂਰਨ wantwidthmin,
              AstRange* rangep);

ਜਨਤਕ:
    ASTGEN_MEMBERS_AstBasicDType;
    ਖਾਲੀ dump(std::ostream& str) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str) ਅਚਲ override;
    // width/widthMin/numeric compared elsewhere
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* samep) ਅਚਲ override;
    bool similar_DType_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ_DType* samep) ਅਚਲ override;
    string name() ਅਚਲ override VL_MT_STABLE { ਵਾਪਸ m.m_keyword.typeName(); }
    string prettyDTypeName(bool ਭਰਿਆ_2) ਅਚਲ override;
    ਅਚਲ ਅੱਖਰ* broken() ਅਚਲ override {
        BROKEN_RTN(dtypep() != this);
        BROKEN_RTN(v3Global.ਚੌੜਾਈ_Min_Usage() == VWidthMinUsage::VERILOG_ਚੌੜਾਈ
                   && ਚੌੜਾਈ_Min() > ਚੌੜਾਈ());
        ਵਾਪਸ nullptr;
    }
    ਖਾਲੀ ਸੈੱਟ_Signed_State(ਅਚਲ VSigning& signst) {
        // Note NOSIGN does NOT change the state; this is required by the parser
        ਜੇ (signst == VSigning::UNSIGNED) {
            numeric(signst);
        } ਨਹੀਂਤਾਂ ਜੇ (signst == VSigning::SIGNED) {
            numeric(signst);
        }
    }
    // METHODS
    AstBasicDType* basicp() ਅਚਲ override VL_MT_STABLE { ਵਾਪਸ (AstBasicDType*)this; }
    // (Slow) recurses - Structure alignment 1,2,4 or 8 bytes (arrays affect this)
    ਪੂਰਨ ਚੌੜਾਈ_Align_ਬਾਈਟਾਂ() ਅਚਲ override;
    // (Slow) recurses - Width in bytes rounding up 1,2,4,8,12,...
    ਪੂਰਨ ਚੌੜਾਈ_ਕੁੱਲ_ਬਾਈਟਾਂ() ਅਚਲ override;
    bool isFourstate() ਅਚਲ override { ਵਾਪਸ keyword().isFourstate(); }
    VBasicDTypeKwd keyword() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ {  // Avoid using - use isSomething accessors instead
        ਵਾਪਸ m.m_keyword;
    }
    bool is_ਦੋਆਧਾਰੀ_ਅੰਕ_Logic() ਅਚਲ { ਵਾਪਸ keyword().is_ਦੋਆਧਾਰੀ_ਅੰਕ_Logic(); }
    bool is_ਦੋਆਧਾਰੀ_ਅੰਕ() ਅਚਲ { ਵਾਪਸ keyword().is_ਦੋਆਧਾਰੀ_ਅੰਕ(); }
    bool isCHandle() ਅਚਲ VL_MT_STABLE { ਵਾਪਸ keyword().isCHandle(); }
    bool isDouble() ਅਚਲ VL_MT_STABLE { ਵਾਪਸ keyword().isDouble(); }
    bool isEvent() ਅਚਲ VL_MT_STABLE { ਵਾਪਸ keyword() == VBasicDTypeKwd::EVENT; }
    bool isForkSync() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ keyword() == VBasicDTypeKwd::FORK_SYNC; }
    bool isProcessRef() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ keyword() == VBasicDTypeKwd::PROCESS_REFERENCE; }
    bool isDelayScheduler() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ {
        ਵਾਪਸ keyword() == VBasicDTypeKwd::DELAY_SCHEDULER;
    }
    bool isTriggerScheduler() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ {
        ਵਾਪਸ keyword() == VBasicDTypeKwd::TRIGGER_SCHEDULER;
    }
    bool isDynamicTriggerScheduler() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ {
        ਵਾਪਸ keyword() == VBasicDTypeKwd::DYNAMIC_TRIGGER_SCHEDULER;
    }
    bool isRandomGenerator() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ {
        ਵਾਪਸ keyword() == VBasicDTypeKwd::RANDOM_GENERATOR;
    }
    bool isStdRandomGenerator() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ {
        ਵਾਪਸ keyword() == VBasicDTypeKwd::RANDOM_STDGENERATOR;
    }
    bool isCovergroupInstHandle() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ {
        ਵਾਪਸ keyword() == VBasicDTypeKwd::COVERGROUP_INSTHANDLE;
    }
    bool isOpaque() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ keyword().isOpaque(); }
    bool isString() ਅਚਲ VL_MT_STABLE { ਵਾਪਸ keyword().isString(); }
    bool isZeroInit() ਅਚਲ { ਵਾਪਸ keyword().isZeroInit(); }
    bool isRanged() ਅਚਲ { ਵਾਪਸ rangep() || m.m_nrange.ranged(); }
    bool is_Dpi_ਦੋਆਧਾਰੀ_ਅੰਕ_Vec() ਅਚਲ {  // DPI uses svBitVecVal
        ਵਾਪਸ keyword() == VBasicDTypeKwd::ਦੋਆਧਾਰੀ_ਅੰਕ && isRanged();
    }
    bool isDpiLogicVec() ਅਚਲ {  // DPI uses svLogicVecVal
        ਵਾਪਸ keyword().isFourstate() && !(keyword() == VBasicDTypeKwd::LOGIC && !isRanged());
    }
    bool isDpiCLayout() ਅਚਲ {
        ਵਾਪਸ keyword().isDpiCLayout()
               || (keyword().isSameish(VBasicDTypeKwd::LOGIC) && !isRanged());
    }
    bool isDpiPrimitive() ਅਚਲ {  // DPI uses a primitive type
        ਵਾਪਸ !is_Dpi_ਦੋਆਧਾਰੀ_ਅੰਕ_Vec() && !isDpiLogicVec();
    }
    // Generally the lo/hi/left/right funcs should be used instead of nrange()
    ਅਚਲ VNumRange& nrange() ਅਚਲ { ਵਾਪਸ m.m_nrange; }
    ਸਤਰਅੰਦਰ ਪੂਰਨ hi() ਅਚਲ;
    ਸਤਰਅੰਦਰ ਪੂਰਨ lo() ਅਚਲ;
    ਸਤਰਅੰਦਰ ਪੂਰਨ elements() ਅਚਲ;
    ਪੂਰਨ ਖੱਬਾ() ਅਚਲ { ਵਾਪਸ ascending() ? lo() : hi(); }  // How to show a declaration
    ਪੂਰਨ ਸੱਜਾ() ਅਚਲ { ਵਾਪਸ ascending() ? hi() : lo(); }
    ਸਤਰਅੰਦਰ bool ascending() ਅਚਲ;
    bool implicit() ਅਚਲ { ਵਾਪਸ keyword() == VBasicDTypeKwd::LOGIC_IMPLICIT; }
    bool untyped() ਅਚਲ { ਵਾਪਸ keyword() == VBasicDTypeKwd::UNTYPED; }
    VNumRange declRange() ਅਚਲ { ਵਾਪਸ isRanged() ? VNumRange{ਖੱਬਾ(), ਸੱਜਾ()} : VNumRange{}; }
    ਖਾਲੀ cvtRangeConst();  // Convert to smaller representation
    bool isCompound() ਅਚਲ override { ਵਾਪਸ isString(); }
    bool isIntegralOrPacked() ਅਚਲ override { ਵਾਪਸ keyword().isIntNumeric(); }
};
class AstBracketArrayDType final : ਜਨਤਕ Ast_ਗੰਢ_DType {
    // Associative/Queue/Normal array data type, ie "[dtype_or_expr]"
    // only for early parsing then becomes another data type
    // @astgen op1 := childDTypep : Optional[AstNodeDType] // moved to refDTypep() in V3Width
    // @astgen op2 := elementsp : AstNode  // Number of elements in array
ਜਨਤਕ:
    AstBracketArrayDType(File_ਸਤਰ* fl, VFlag_ਬੱਚਾ_DType, Ast_ਗੰਢ_DType* ਬੱਚਾ_DTypep,
                         Ast_ਗੰਢ* elementsp)
        : ASTGEN_SUPER_BracketArrayDType(fl) {
        this->ਬੱਚਾ_DTypep(ਬੱਚਾ_DTypep);
        this->elementsp(elementsp);
    }
    ASTGEN_MEMBERS_AstBracketArrayDType;
    bool similar_DType_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ_DType* samep) ਅਚਲ override { ਵਾਪਸ this == samep; }
    Ast_ਗੰਢ_DType* subDTypep() ਅਚਲ override VL_MT_STABLE { ਵਾਪਸ ਬੱਚਾ_DTypep(); }
    // METHODS
    // Will be removed in V3Width, which relies on this
    // being a child not a dtype pointed node
    bool maybe_Pointed_ਮੰਜ਼ਿਲ() ਅਚਲ override VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ false; }
    AstBasicDType* basicp() ਅਚਲ override VL_MT_STABLE { ਵਾਪਸ nullptr; }
    ਪੂਰਨ ਚੌੜਾਈ_Align_ਬਾਈਟਾਂ() ਅਚਲ override { V3ERROR_NA_RETURN(0); }
    ਪੂਰਨ ਚੌੜਾਈ_ਕੁੱਲ_ਬਾਈਟਾਂ() ਅਚਲ override { V3ERROR_NA_RETURN(0); }
    bool isCompound() ਅਚਲ override { ਵਾਪਸ true; }
};
class AstCDType final : ਜਨਤਕ Ast_ਗੰਢ_DType {
    // Raw "C" data type passed directly to output
    string m_name;  // Name of data type, printed when do V3EmitC
ਜਨਤਕ:
    AstCDType(File_ਸਤਰ* fl, ਅਚਲ string& name)
        : ASTGEN_SUPER_CDType(fl)
        , m_name{name} {
        dtypep(this);
    }

    ASTGEN_MEMBERS_AstCDType;
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* samep) ਅਚਲ override {
        ਅਚਲ AstCDType* ਅਚਲ asamep = VN_DBG_AS(samep, CDType);
        ਵਾਪਸ m_name == asamep->m_name;
    }
    bool similar_DType_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ_DType* samep) ਅਚਲ override { ਵਾਪਸ same_ਗੰਢ(samep); }
    string name() ਅਚਲ override VL_MT_STABLE { ਵਾਪਸ m_name; }
    string prettyDTypeName(bool) ਅਚਲ override { ਵਾਪਸ m_name; }
    // METHODS
    AstBasicDType* basicp() ਅਚਲ override VL_MT_STABLE { ਵਾਪਸ nullptr; }
    ਪੂਰਨ ਚੌੜਾਈ_Align_ਬਾਈਟਾਂ() ਅਚਲ override { ਵਾਪਸ 8; }  // Assume
    ਪੂਰਨ ਚੌੜਾਈ_ਕੁੱਲ_ਬਾਈਟਾਂ() ਅਚਲ override { ਵਾਪਸ 8; }  // Assume
    bool isCompound() ਅਚਲ override { ਵਾਪਸ true; }
    // Return the global runtime type storing the requested number of bits.
    ਸਥਿਰ string type_ਮੰਜ਼ਿਲ_Hold(ਪੂਰਨ ਚੌੜਾਈ) {
        ਜੇ (ਚੌੜਾਈ <= 8) ਵਾਪਸ "::CData";
        ਜੇ (ਚੌੜਾਈ <= 16) ਵਾਪਸ "::SData";
        ਜੇ (ਚੌੜਾਈ <= VL_IDATASIZE) ਵਾਪਸ "::IData";
        ਜੇ (ਚੌੜਾਈ <= VL_QUADSIZE) ਵਾਪਸ "::QData";

        ਵਾਪਸ "::VlWide<" + std::ਮੰਜ਼ਿਲ_string(VL_ਸ਼ਬਦ_I(ਚੌੜਾਈ)) + ">";
    }
};
class AstClassRefDType final : ਜਨਤਕ Ast_ਗੰਢ_DType {
    // Reference to a class
    // @astgen op1 := paramsp: List[AstPin]
    //
    // @astgen ptr := m_classp : Optional[AstClass]  // data type pointed to, BELOW the AstTypedef
    // @astgen ptr := m_classOrPackagep : Optional[AstNodeModule]  // Package hierarchy
    bool m_raw_ਸੂਚਕ = false;  // Emit as a non-owning C++ pointer rather than VlClassRef
ਜਨਤਕ:
    AstClassRefDType(File_ਸਤਰ* fl, AstClass* classp, AstPin* paramsp)
        : ASTGEN_SUPER_ClassRefDType(fl)
        , m_classp{classp} {
        dtypep(this);
        ਜੋੜੋ_Paramsp(paramsp);
    }
    ASTGEN_MEMBERS_AstClassRefDType;
    // METHODS
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* samep) ਅਚਲ override {
        ਅਚਲ AstClassRefDType* ਅਚਲ asamep = VN_DBG_AS(samep, ClassRefDType);
        ਵਾਪਸ (m_classp == asamep->m_classp && m_classOrPackagep == asamep->m_classOrPackagep
                && m_raw_ਸੂਚਕ == asamep->m_raw_ਸੂਚਕ);
    }
    bool similar_DType_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ_DType* samep) ਅਚਲ override;
    ਖਾਲੀ dump(std::ostream& str = std::cout) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str = std::cout) ਅਚਲ override;
    ਖਾਲੀ dumpSmall(std::ostream& str) ਅਚਲ override;
    string prettyDTypeName(bool ਭਰਿਆ_2) ਅਚਲ override;
    string pretty_Name_ਸੁਨੇਹਾ() ਅਚਲ override;
    string name() ਅਚਲ override VL_MT_STABLE;
    AstBasicDType* basicp() ਅਚਲ override VL_MT_STABLE { ਵਾਪਸ nullptr; }
    ਪੂਰਨ ਚੌੜਾਈ_Align_ਬਾਈਟਾਂ() ਅਚਲ override { ਵਾਪਸ 0; }
    ਪੂਰਨ ਚੌੜਾਈ_ਕੁੱਲ_ਬਾਈਟਾਂ() ਅਚਲ override { ਵਾਪਸ 0; }
    Ast_ਗੰਢ_DType* virtRefDTypep() ਅਚਲ override { ਵਾਪਸ nullptr; }
    ਖਾਲੀ virtRefDTypep(Ast_ਗੰਢ_DType* nodep) override {}
    Ast_ਗੰਢ_DType* subDTypep() ਅਚਲ override VL_MT_STABLE { ਵਾਪਸ nullptr; }
    Ast_ਗੰਢ_Module* classOrPackagep() ਅਚਲ { ਵਾਪਸ m_classOrPackagep; }
    ਖਾਲੀ classOrPackagep(Ast_ਗੰਢ_Module* nodep) { m_classOrPackagep = nodep; }
    AstClass* classp() ਅਚਲ VL_MT_STABLE { ਵਾਪਸ m_classp; }
    ਖਾਲੀ classp(AstClass* nodep) { m_classp = nodep; }
    bool raw_ਸੂਚਕ() ਅਚਲ { ਵਾਪਸ m_raw_ਸੂਚਕ; }
    ਖਾਲੀ raw_ਸੂਚਕ(bool flag) { m_raw_ਸੂਚਕ = flag; }
    ਸਥਿਰ ਖਾਲੀ selfTest();
    bool isCompound() ਅਚਲ override { ਵਾਪਸ true; }
};
class AstConstDType final : ਜਨਤਕ Ast_ਗੰਢ_DType {
    // const data type, ie "const some_dtype var_name [2:0]"
    // ConstDType are removed in V3LinkLValue and become AstVar::isConst.
    // When more generic types are supported AstConstDType will be propagated further.
    // @astgen op1 := childDTypep : Optional[AstNodeDType]
    //
    // @astgen ptr := m_refDTypep : Optional[AstNodeDType]  // Inherit from this base data type
ਜਨਤਕ:
    AstConstDType(File_ਸਤਰ* fl, VFlag_ਬੱਚਾ_DType, Ast_ਗੰਢ_DType* dtp)
        : ASTGEN_SUPER_ConstDType(fl) {
        ਬੱਚਾ_DTypep(dtp);  // Only for parser
        refDTypep(nullptr);  // V3Width will resolve
        dtypep(nullptr);  // V3Width will resolve
        ਚੌੜਾਈ_From_Sub(subDTypep());
    }
    ASTGEN_MEMBERS_AstConstDType;
    ਅਚਲ ਅੱਖਰ* broken() ਅਚਲ override {
        BROKEN_RTN(!((m_refDTypep && !ਬੱਚਾ_DTypep()) || (!m_refDTypep && ਬੱਚਾ_DTypep())));
        ਵਾਪਸ nullptr;
    }
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* samep) ਅਚਲ override {
        ਅਚਲ AstConstDType* ਅਚਲ sp = VN_DBG_AS(samep, ConstDType);
        ਵਾਪਸ (m_refDTypep == sp->m_refDTypep);
    }
    bool similar_DType_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ_DType* samep) ਅਚਲ override {
        ਵਾਪਸ skipRefp()->similarDType(samep->skipRefp());
    }
    Ast_ਗੰਢ_DType* get_ਬੱਚਾ_DTypep() ਅਚਲ override { ਵਾਪਸ ਬੱਚਾ_DTypep(); }
    Ast_ਗੰਢ_DType* subDTypep() ਅਚਲ override VL_MT_STABLE {
        ਵਾਪਸ m_refDTypep ? m_refDTypep : ਬੱਚਾ_DTypep();
    }
    ਖਾਲੀ refDTypep(Ast_ਗੰਢ_DType* nodep) { m_refDTypep = nodep; }
    Ast_ਗੰਢ_DType* virtRefDTypep() ਅਚਲ override { ਵਾਪਸ m_refDTypep; }
    ਖਾਲੀ virtRefDTypep(Ast_ਗੰਢ_DType* nodep) override { refDTypep(nodep); }
    // METHODS
    AstBasicDType* basicp() ਅਚਲ override VL_MT_STABLE { ਵਾਪਸ subDTypep()->basicp(); }
    ਪੂਰਨ ਚੌੜਾਈ_Align_ਬਾਈਟਾਂ() ਅਚਲ override { ਵਾਪਸ subDTypep()->ਚੌੜਾਈ_Align_ਬਾਈਟਾਂ(); }
    ਪੂਰਨ ਚੌੜਾਈ_ਕੁੱਲ_ਬਾਈਟਾਂ() ਅਚਲ override { ਵਾਪਸ subDTypep()->ਚੌੜਾਈ_ਕੁੱਲ_ਬਾਈਟਾਂ(); }
    bool isCompound() ਅਚਲ override {
        v3fatalSrc("call isCompound on subdata type, not reference");
        ਵਾਪਸ false;
    }
};
class AstConstraintRefDType final : ਜਨਤਕ Ast_ਗੰਢ_DType {
    // For e.g. a reference to constraint for constraint_mode
ਜਨਤਕ:
    explicit AstConstraintRefDType(File_ਸਤਰ* fl)
        : ASTGEN_SUPER_ConstraintRefDType(fl) {
        dtypep(this);
    }
    ASTGEN_MEMBERS_AstConstraintRefDType;
    bool hasDType() ਅਚਲ override VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ true; }
    bool maybe_Pointed_ਮੰਜ਼ਿਲ() ਅਚਲ override VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ true; }
    bool undead() ਅਚਲ override { ਵਾਪਸ true; }
    Ast_ਗੰਢ_DType* subDTypep() ਅਚਲ override VL_MT_STABLE { ਵਾਪਸ nullptr; }
    Ast_ਗੰਢ_DType* virtRefDTypep() ਅਚਲ override { ਵਾਪਸ nullptr; }
    ਖਾਲੀ virtRefDTypep(Ast_ਗੰਢ_DType* nodep) override {}
    bool similar_DType_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ_DType* samep) ਅਚਲ override { ਵਾਪਸ this == samep; }
    AstBasicDType* basicp() ਅਚਲ override VL_MT_STABLE { ਵਾਪਸ nullptr; }
    ਪੂਰਨ ਚੌੜਾਈ_Align_ਬਾਈਟਾਂ() ਅਚਲ override { ਵਾਪਸ 1; }
    ਪੂਰਨ ਚੌੜਾਈ_ਕੁੱਲ_ਬਾਈਟਾਂ() ਅਚਲ override { ਵਾਪਸ 1; }
    bool isCompound() ਅਚਲ override { ਵਾਪਸ false; }
};
class AstCoverCrossDType final : ਜਨਤਕ Ast_ਗੰਢ_DType {
    // Borrowed pointer to VlCoverCrossT<...> or construction-time VlCoverCrossDyn.
    ਅਚਲ uint32_t m_dimensions;
    ਅਚਲ uint32_t m_tuples;  // Fixed capacity number of cross tuples (VlCoverCrossT's Tuples)
    ਅਚਲ uint32_t m_bins;  // Fixed capacity number of explicit bins (VlCoverCrossT's Bins)
    ਅਚਲ uint32_t m_autoBins;  // Fixed capacity number of auto bins (VlCoverCrossT's AutoBins)
    ਅਚਲ uint64_t m_bin_ਸ਼ਬਦ;  // Fixed capacity selection words (VlCoverCrossT's BinWords)
    ਅਚਲ bool m_dynamic;  // Bin layout is determined at covergroup construction

ਜਨਤਕ:
    AstCoverCrossDType(File_ਸਤਰ* fl, uint32_t dimensions, uint32_t tuples, uint32_t bins,
                       uint32_t autoBins, uint64_t bin_ਸ਼ਬਦ, bool dynamic = false)
        : ASTGEN_SUPER_CoverCrossDType(fl)
        , m_dimensions{dimensions}
        , m_tuples{tuples}
        , m_bins{bins}
        , m_autoBins{autoBins}
        , m_bin_ਸ਼ਬਦ{bin_ਸ਼ਬਦ}
        , m_dynamic{dynamic} {
        dtypep(this);
    }
    ASTGEN_MEMBERS_AstCoverCrossDType;
    ਅਚਲ ਅੱਖਰ* broken() ਅਚਲ override {
        BROKEN_RTN(m_dimensions == 0);
        ਵਾਪਸ nullptr;
    }
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* samep) ਅਚਲ override {  // LCOV_EXCL_START
        ਅਚਲ AstCoverCrossDType* ਅਚਲ sp = VN_DBG_AS(samep, CoverCrossDType);
        ਵਾਪਸ dimensions() == sp->dimensions() && tuples() == sp->tuples() && bins() == sp->bins()
               && autoBins() == sp->autoBins() && bin_ਸ਼ਬਦ() == sp->bin_ਸ਼ਬਦ()
               && isDynamic() == sp->isDynamic();
    }  // LCOV_EXCL_STOP
    bool similar_DType_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ_DType* samep) ਅਚਲ override { ਵਾਪਸ this == samep; }
    ਖਾਲੀ dump(std::ostream& str) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str) ਅਚਲ override;
    ਖਾਲੀ dumpSmall(std::ostream& str) ਅਚਲ override;
    uint32_t dimensions() ਅਚਲ { ਵਾਪਸ m_dimensions; }
    uint32_t tuples() ਅਚਲ { ਵਾਪਸ m_tuples; }
    uint32_t bins() ਅਚਲ { ਵਾਪਸ m_bins; }
    uint32_t autoBins() ਅਚਲ { ਵਾਪਸ m_autoBins; }
    uint64_t bin_ਸ਼ਬਦ() ਅਚਲ { ਵਾਪਸ m_bin_ਸ਼ਬਦ; }
    bool isDynamic() ਅਚਲ { ਵਾਪਸ m_dynamic; }
    string cpp_Template_ਦਲੀਲਾਂ() ਅਚਲ;
    AstBasicDType* basicp() ਅਚਲ override VL_MT_STABLE { ਵਾਪਸ nullptr; }
    ਪੂਰਨ ਚੌੜਾਈ_Align_ਬਾਈਟਾਂ() ਅਚਲ override { ਵਾਪਸ ਆਕਾਰ(ਖਾਲੀ*); }
    ਪੂਰਨ ਚੌੜਾਈ_ਕੁੱਲ_ਬਾਈਟਾਂ() ਅਚਲ override { ਵਾਪਸ ਆਕਾਰ(ਖਾਲੀ*); }
    bool isCompound() ਅਚਲ override { ਵਾਪਸ true; }
};
class AstCoverpointDType final : ਜਨਤਕ Ast_ਗੰਢ_DType {
    // Borrowed pointer to a covergroup coverpoint runtime, 'VlCoverpointT<hitBound>*'.
    // Follows pattern of AstQueueDType in capturing the compile-time max bin overlap
    // template argument.
    uint32_t m_ਪ੍ਰਾਪਤੀ_Bound;  // VlCoverpointT<> template argument; hit list size, >= 1
ਜਨਤਕ:
    AstCoverpointDType(File_ਸਤਰ* fl, uint32_t ਪ੍ਰਾਪਤੀ_Bound)
        : ASTGEN_SUPER_CoverpointDType(fl)
        , m_ਪ੍ਰਾਪਤੀ_Bound{ਪ੍ਰਾਪਤੀ_Bound} {
        dtypep(this);
    }
    ASTGEN_MEMBERS_AstCoverpointDType;
    ਅਚਲ ਅੱਖਰ* broken() ਅਚਲ override {
        BROKEN_RTN(m_ਪ੍ਰਾਪਤੀ_Bound < 1);
        ਵਾਪਸ nullptr;
    }
    // V3Covergroup interns these one-per-hitBound into the type table, so identity is
    // equality; there is never a second node with the same bound to compare against.
    bool similar_DType_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ_DType* samep) ਅਚਲ override { ਵਾਪਸ this == samep; }
    ਖਾਲੀ dumpSmall(std::ostream& str) ਅਚਲ override;
    // ACCESSORS
    uint32_t ਪ੍ਰਾਪਤੀ_Bound() ਅਚਲ { ਵਾਪਸ m_ਪ੍ਰਾਪਤੀ_Bound; }
    // METHODS
    AstBasicDType* basicp() ਅਚਲ override VL_MT_STABLE { ਵਾਪਸ nullptr; }
    ਪੂਰਨ ਚੌੜਾਈ_Align_ਬਾਈਟਾਂ() ਅਚਲ override { ਵਾਪਸ ਆਕਾਰ(ਖਾਲੀ*); }
    ਪੂਰਨ ਚੌੜਾਈ_ਕੁੱਲ_ਬਾਈਟਾਂ() ਅਚਲ override { ਵਾਪਸ ਆਕਾਰ(ਖਾਲੀ*); }
    bool isCompound() ਅਚਲ override { ਵਾਪਸ true; }
};
class AstDefImplicitDType final : ਜਨਤਕ Ast_ਗੰਢ_DType {
    // For parsing enum/struct/unions that are declared with a variable rather than typedef
    // This allows "var enum {...} a,b" to share the enum definition for both variables
    // After link, these become typedefs
    // @astgen op1 := childDTypep : Optional[AstNodeDType]
    string m_name;  // Data type name
    ਅਚਲ ਪੂਰਨ m_unique_ਅੰਕ;  // Unique ID distinguishing this dtype instance, for hashing/naming

ਜਨਤਕ:
    AstDefImplicitDType(File_ਸਤਰ* fl, ਅਚਲ string& name, VFlag_ਬੱਚਾ_DType, Ast_ਗੰਢ_DType* dtp)
        : ASTGEN_SUPER_DefImplicitDType(fl)
        , m_name{name}
        , m_unique_ਅੰਕ{unique_ਅੰਕ_Inc()} {
        ਬੱਚਾ_DTypep(dtp);  // Only for parser
        dtypep(nullptr);  // V3Width will resolve
    }
    AstDefImplicitDType(ਅਚਲ AstDefImplicitDType& other)
        : Ast_ਗੰਢ_DType(other)
        , m_name{other.m_name}
        , m_unique_ਅੰਕ{unique_ਅੰਕ_Inc()} {}
    ASTGEN_MEMBERS_AstDefImplicitDType;
    ਪੂਰਨ unique_ਅੰਕ() ਅਚਲ { ਵਾਪਸ m_unique_ਅੰਕ; }
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* samep) ਅਚਲ override {
        ਅਚਲ AstDefImplicitDType* ਅਚਲ sp = VN_DBG_AS(samep, DefImplicitDType);
        ਵਾਪਸ unique_ਅੰਕ() == sp->unique_ਅੰਕ();
    }
    bool similar_DType_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ_DType* samep) ਅਚਲ override { ਵਾਪਸ same_ਗੰਢ(samep); }
    Ast_ਗੰਢ_DType* get_ਬੱਚਾ_DTypep() ਅਚਲ override { ਵਾਪਸ ਬੱਚਾ_DTypep(); }
    Ast_ਗੰਢ_DType* subDTypep() ਅਚਲ override VL_MT_STABLE {
        ਵਾਪਸ dtypep() ? dtypep() : ਬੱਚਾ_DTypep();
    }
    // METHODS
    // op1 = Range of variable
    Ast_ਗੰਢ_DType* dtypeSkipRefp() ਅਚਲ { ਵਾਪਸ dtypep()->skipRefp(); }
    AstBasicDType* basicp() ਅਚਲ override VL_MT_STABLE { ਵਾਪਸ subDTypep()->basicp(); }
    ਪੂਰਨ ਚੌੜਾਈ_Align_ਬਾਈਟਾਂ() ਅਚਲ override { ਵਾਪਸ dtypep()->ਚੌੜਾਈ_Align_ਬਾਈਟਾਂ(); }
    ਪੂਰਨ ਚੌੜਾਈ_ਕੁੱਲ_ਬਾਈਟਾਂ() ਅਚਲ override { ਵਾਪਸ dtypep()->ਚੌੜਾਈ_ਕੁੱਲ_ਬਾਈਟਾਂ(); }
    string name() ਅਚਲ override VL_MT_STABLE { ਵਾਪਸ m_name; }
    ਖਾਲੀ name(ਅਚਲ string& flag) override { m_name = flag; }
    bool isCompound() ਅਚਲ override { ਵਾਪਸ false; }
};
class AstDynArrayDType final : ਜਨਤਕ Ast_ਗੰਢ_DType {
    // Dynamic array data type, ie "[]"
    // @astgen op1 := childDTypep : Optional[AstNodeDType] // moved to refDTypep() in V3Width
    //
    // @astgen ptr := m_refDTypep : Optional[AstNodeDType]  // Elements of this type (post-width)
ਜਨਤਕ:
    AstDynArrayDType(File_ਸਤਰ* fl, VFlag_ਬੱਚਾ_DType, Ast_ਗੰਢ_DType* dtp)
        : ASTGEN_SUPER_DynArrayDType(fl) {
        ਬੱਚਾ_DTypep(dtp);  // Only for parser
        refDTypep(nullptr);
        dtypep(nullptr);  // V3Width will resolve
    }
    AstDynArrayDType(File_ਸਤਰ* fl, Ast_ਗੰਢ_DType* dtp)
        : ASTGEN_SUPER_DynArrayDType(fl) {
        refDTypep(dtp);
        dtypep(nullptr);  // V3Width will resolve
    }
    ASTGEN_MEMBERS_AstDynArrayDType;
    ਅਚਲ ਅੱਖਰ* broken() ਅਚਲ override {
        BROKEN_RTN(!((m_refDTypep && !ਬੱਚਾ_DTypep()) || (!m_refDTypep && ਬੱਚਾ_DTypep())));
        ਵਾਪਸ nullptr;
    }
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* samep) ਅਚਲ override {
        ਅਚਲ AstDynArrayDType* ਅਚਲ asamep = VN_DBG_AS(samep, DynArrayDType);
        ਜੇ (!asamep->subDTypep()) ਵਾਪਸ false;
        ਵਾਪਸ subDTypep() == asamep->subDTypep();
    }
    bool similar_DType_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ_DType* samep) ਅਚਲ override {
        ਅਚਲ AstDynArrayDType* ਅਚਲ asamep = VN_DBG_AS(samep, DynArrayDType);
        ਵਾਪਸ asamep->subDTypep() && subDTypep()->similarDType(asamep->subDTypep());
    }
    string prettyDTypeName(bool ਭਰਿਆ_2) ਅਚਲ override;
    ਖਾਲੀ dumpSmall(std::ostream& str) ਅਚਲ override;
    Ast_ਗੰਢ_DType* get_ਬੱਚਾ_DTypep() ਅਚਲ override { ਵਾਪਸ ਬੱਚਾ_DTypep(); }
    Ast_ਗੰਢ_DType* subDTypep() ਅਚਲ override VL_MT_STABLE {
        ਵਾਪਸ m_refDTypep ? m_refDTypep : ਬੱਚਾ_DTypep();
    }
    bool isAggregateType() ਅਚਲ override { ਵਾਪਸ true; }
    ਖਾਲੀ refDTypep(Ast_ਗੰਢ_DType* nodep) { m_refDTypep = nodep; }
    Ast_ਗੰਢ_DType* virtRefDTypep() ਅਚਲ override { ਵਾਪਸ m_refDTypep; }
    ਖਾਲੀ virtRefDTypep(Ast_ਗੰਢ_DType* nodep) override { refDTypep(nodep); }
    // METHODS
    AstBasicDType* basicp() ਅਚਲ override VL_MT_STABLE { ਵਾਪਸ nullptr; }
    ਪੂਰਨ ਚੌੜਾਈ_Align_ਬਾਈਟਾਂ() ਅਚਲ override { ਵਾਪਸ subDTypep()->ਚੌੜਾਈ_Align_ਬਾਈਟਾਂ(); }
    ਪੂਰਨ ਚੌੜਾਈ_ਕੁੱਲ_ਬਾਈਟਾਂ() ਅਚਲ override { ਵਾਪਸ subDTypep()->ਚੌੜਾਈ_ਕੁੱਲ_ਬਾਈਟਾਂ(); }
    bool isCompound() ਅਚਲ override { ਵਾਪਸ true; }
    bool isDynamicallySized() ਅਚਲ override { ਵਾਪਸ true; }
};
class Ast_ਖਾਲੀ_ਥਾਵਾਂ_ਕਤਾਰ_DType final : ਜਨਤਕ Ast_ਗੰਢ_DType {
    // For EmptyQueue
ਜਨਤਕ:
    explicit Ast_ਖਾਲੀ_ਥਾਵਾਂ_ਕਤਾਰ_DType(File_ਸਤਰ* fl)
        : ASTGEN_SUPER_ਖਾਲੀ_ਥਾਵਾਂ_ਕਤਾਰ_DType(fl) {
        dtypep(this);
    }
    ASTGEN_MEMBERS_Ast_ਖਾਲੀ_ਥਾਵਾਂ_ਕਤਾਰ_DType;
    ਖਾਲੀ dumpSmall(std::ostream& str) ਅਚਲ override;
    bool hasDType() ਅਚਲ override VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ true; }
    bool maybe_Pointed_ਮੰਜ਼ਿਲ() ਅਚਲ override VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ true; }
    bool undead() ਅਚਲ override { ਵਾਪਸ true; }
    Ast_ਗੰਢ_DType* subDTypep() ਅਚਲ override VL_MT_STABLE { ਵਾਪਸ nullptr; }
    Ast_ਗੰਢ_DType* virtRefDTypep() ਅਚਲ override { ਵਾਪਸ nullptr; }
    ਖਾਲੀ virtRefDTypep(Ast_ਗੰਢ_DType* nodep) override {}
    bool similar_DType_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ_DType* samep) ਅਚਲ override { ਵਾਪਸ this == samep; }
    AstBasicDType* basicp() ਅਚਲ override VL_MT_STABLE { ਵਾਪਸ nullptr; }
    ਪੂਰਨ ਚੌੜਾਈ_Align_ਬਾਈਟਾਂ() ਅਚਲ override { ਵਾਪਸ 1; }
    ਪੂਰਨ ਚੌੜਾਈ_ਕੁੱਲ_ਬਾਈਟਾਂ() ਅਚਲ override { ਵਾਪਸ 1; }
    bool isCompound() ਅਚਲ override { ਵਾਪਸ false; }
};
class AstEnumDType final : ਜਨਤਕ Ast_ਗੰਢ_DType {
    // Parents: TYPEDEF/MODULE
    // @astgen op1 := childDTypep : Optional[AstNodeDType]
    // @astgen op2 := itemsp : List[AstEnumItem]
    //
    // @astgen ptr := m_refDTypep : Optional[AstNodeDType]  // Elements of this type (post-width)
    string m_name;  // Name from upper typedef, if any
    string m_typedefName;  // Typedef's dtypeName() once moved off the typedef declaring it
    ਅਚਲ ਪੂਰਨ m_unique_ਅੰਕ;  // Unique ID distinguishing this dtype instance, for hashing/naming

ਜਨਤਕ:
    AstEnumDType(File_ਸਤਰ* fl, VFlag_ਬੱਚਾ_DType, Ast_ਗੰਢ_DType* dtp, Ast_Enum_ਵਸਤੂ* itemsp)
        : ASTGEN_SUPER_EnumDType(fl)
        , m_unique_ਅੰਕ{unique_ਅੰਕ_Inc()} {
        ਬੱਚਾ_DTypep(dtp);  // Only for parser
        refDTypep(nullptr);
        ਜੋੜੋ_Itemsp(itemsp);
        dtypep(nullptr);  // V3Width will resolve
        ਚੌੜਾਈ_From_Sub(subDTypep());
    }
    AstEnumDType(ਅਚਲ AstEnumDType& other)
        : Ast_ਗੰਢ_DType{other}
        , m_name{other.m_name}
        , m_typedefName{other.m_typedefName}
        , m_unique_ਅੰਕ{unique_ਅੰਕ_Inc()} {}
    ASTGEN_MEMBERS_AstEnumDType;

    ਅਚਲ ਅੱਖਰ* broken() ਅਚਲ override;
    ਪੂਰਨ unique_ਅੰਕ() ਅਚਲ { ਵਾਪਸ m_unique_ਅੰਕ; }
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* samep) ਅਚਲ override {
        ਅਚਲ AstEnumDType* ਅਚਲ sp = VN_DBG_AS(samep, EnumDType);
        ਵਾਪਸ unique_ਅੰਕ() == sp->unique_ਅੰਕ();
    }
    bool similar_DType_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ_DType* samep) ਅਚਲ override { ਵਾਪਸ same_ਗੰਢ(samep); }
    Ast_ਗੰਢ_DType* get_ਬੱਚਾ_DTypep() ਅਚਲ override { ਵਾਪਸ ਬੱਚਾ_DTypep(); }
    Ast_ਗੰਢ_DType* subDTypep() ਅਚਲ override VL_MT_STABLE {
        ਵਾਪਸ m_refDTypep ? m_refDTypep : ਬੱਚਾ_DTypep();
    }
    ਖਾਲੀ refDTypep(Ast_ਗੰਢ_DType* nodep) { m_refDTypep = nodep; }
    Ast_ਗੰਢ_DType* virtRefDTypep() ਅਚਲ override { ਵਾਪਸ m_refDTypep; }
    ਖਾਲੀ virtRefDTypep(Ast_ਗੰਢ_DType* nodep) override { refDTypep(nodep); }
    string name() ਅਚਲ override VL_MT_STABLE { ਵਾਪਸ m_name; }
    ਖਾਲੀ name(ਅਚਲ string& flag) override { m_name = flag; }
    // Keep the name the typedef declaring it gives, as it moves off it to the type table
    ਖਾਲੀ typedefName(ਅਚਲ string& name) { m_typedefName = name; }
    ਖਾਲੀ dump(std::ostream& str = std::cout) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str = std::cout) ਅਚਲ override;
    ਖਾਲੀ dumpSmall(std::ostream& str) ਅਚਲ override;
    string prettyDTypeName(bool ਭਰਿਆ_2) ਅਚਲ override;
    // METHODS
    AstBasicDType* basicp() ਅਚਲ override VL_MT_STABLE { ਵਾਪਸ subDTypep()->basicp(); }
    ਪੂਰਨ ਚੌੜਾਈ_Align_ਬਾਈਟਾਂ() ਅਚਲ override { ਵਾਪਸ subDTypep()->ਚੌੜਾਈ_Align_ਬਾਈਟਾਂ(); }
    ਪੂਰਨ ਚੌੜਾਈ_ਕੁੱਲ_ਬਾਈਟਾਂ() ਅਚਲ override { ਵਾਪਸ subDTypep()->ਚੌੜਾਈ_ਕੁੱਲ_ਬਾਈਟਾਂ(); }
    ਮਾਪ_t ਵਸਤੂ_ਗਿਣਤੀ() ਅਚਲ {
        ਮਾਪ_t ਗਿਣਤੀ_2 = 0;
        ਲਈ (Ast_ਗੰਢ* itemp = itemsp(); itemp; itemp = itemp->nextp()) ++ਗਿਣਤੀ_2;
        ਵਾਪਸ ਗਿਣਤੀ_2;
    }
    bool isCompound() ਅਚਲ override { ਵਾਪਸ false; }
};

class AstIfaceGenericDType final : ਜਨਤਕ Ast_ਗੰਢ_DType {
    // Generic interface that will be replaced with AstIfaceRefDType
    File_ਸਤਰ* m_modportFileline;  // Where modport token was
    // dist-ast-dump-suppress  // Part of name()
    string m_modportName;  // "" = no modport
ਜਨਤਕ:
    explicit AstIfaceGenericDType(File_ਸਤਰ* fl)
        : ASTGEN_SUPER_IfaceGenericDType(fl) {
        dtypep(this);
    }
    AstIfaceGenericDType(File_ਸਤਰ* fl, File_ਸਤਰ* modportFl, ਅਚਲ string& modport)
        : ASTGEN_SUPER_IfaceGenericDType(fl)
        , m_modportFileline{modportFl}
        , m_modportName{modport} {
        dtypep(this);
    }
    ASTGEN_MEMBERS_AstIfaceGenericDType;
    ਖਾਲੀ dumpSmall(std::ostream& str) ਅਚਲ override;
    bool hasDType() ਅਚਲ override VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ true; }
    bool maybe_Pointed_ਮੰਜ਼ਿਲ() ਅਚਲ override VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ true; }
    bool undead() ਅਚਲ override { ਵਾਪਸ true; }
    Ast_ਗੰਢ_DType* subDTypep() ਅਚਲ override VL_MT_STABLE { ਵਾਪਸ nullptr; }
    Ast_ਗੰਢ_DType* virtRefDTypep() ਅਚਲ override { ਵਾਪਸ nullptr; }
    ਖਾਲੀ virtRefDTypep(Ast_ਗੰਢ_DType* nodep) override {}
    bool similar_DType_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ_DType* samep) ਅਚਲ override { ਵਾਪਸ this == samep; }
    AstBasicDType* basicp() ਅਚਲ override VL_MT_STABLE { ਵਾਪਸ nullptr; }
    ਪੂਰਨ ਚੌੜਾਈ_Align_ਬਾਈਟਾਂ() ਅਚਲ override { ਵਾਪਸ 1; }
    ਪੂਰਨ ਚੌੜਾਈ_ਕੁੱਲ_ਬਾਈਟਾਂ() ਅਚਲ override { ਵਾਪਸ 1; }
    string modportName() ਅਚਲ { ਵਾਪਸ m_modportName; }
    bool isModport() { ਵਾਪਸ !m_modportName.ਖਾਲੀ_ਥਾਵਾਂ(); }
    bool isCompound() ਅਚਲ override { ਵਾਪਸ true; }
    File_ਸਤਰ* modportFileline() ਅਚਲ { ਵਾਪਸ m_modportFileline; }
    string name() ਅਚਲ override { ਵਾਪਸ m_modportName; }
};

class AstIfaceRefDType final : ਜਨਤਕ Ast_ਗੰਢ_DType {
    // Reference to an interface, either for a port, or inside parent cell
    // @astgen op1 := paramsp : List[AstPin]
    //
    // @astgen ptr := m_ifacep : Optional[AstIface]  // Interface; cellp() should override
    // @astgen ptr := m_cellp : Optional[AstCell]  // When exact parent cell known; not a guess
    // @astgen ptr := m_modportp : Optional[AstModport]  // nullptr = unlinked or no modport
    File_ਸਤਰ* m_modportFileline;  // Where modport token was
    string m_cellName;  // "" = no cell, such as when connects to 'input' iface
    string m_ifaceName;  // Interface name
    string m_modportName;  // "" = no modport
    bool m_virtual = false;  // True if virtual interface
ਜਨਤਕ:
    AstIfaceRefDType(File_ਸਤਰ* fl, ਅਚਲ string& cellName, ਅਚਲ string& ifaceName)
        : ASTGEN_SUPER_IfaceRefDType(fl)
        , m_modportFileline{nullptr}
        , m_cellName{cellName}
        , m_ifaceName{ifaceName}
        , m_modportName{""} {}
    AstIfaceRefDType(File_ਸਤਰ* fl, File_ਸਤਰ* modportFl, ਅਚਲ string& cellName,
                     ਅਚਲ string& ifaceName, ਅਚਲ string& modport)
        : ASTGEN_SUPER_IfaceRefDType(fl)
        , m_modportFileline{modportFl}
        , m_cellName{cellName}
        , m_ifaceName{ifaceName}
        , m_modportName{modport} {}
    AstIfaceRefDType(File_ਸਤਰ* fl, File_ਸਤਰ* modportFl, ਅਚਲ string& cellName,
                     ਅਚਲ string& ifaceName, ਅਚਲ string& modport, AstPin* paramsp)
        : ASTGEN_SUPER_IfaceRefDType(fl)
        , m_modportFileline{modportFl}
        , m_cellName{cellName}
        , m_ifaceName{ifaceName}
        , m_modportName{modport} {
        ਜੋੜੋ_Paramsp(paramsp);
    }
    ASTGEN_MEMBERS_AstIfaceRefDType;

    // METHODS
    ਖਾਲੀ dump(std::ostream& str = std::cout) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str = std::cout) ਅਚਲ override;
    ਖਾਲੀ dumpSmall(std::ostream& str) ਅਚਲ override;
    string prettyDTypeName(bool ਭਰਿਆ_2) ਅਚਲ override;
    AstBasicDType* basicp() ਅਚਲ override VL_MT_STABLE { ਵਾਪਸ nullptr; }
    bool similar_DType_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ_DType* samep) ਅਚਲ override {
        // Each occurrence of a virtual interface type parses to its own node,
        // so pointer identity is insufficient; compare the referenced
        // interface instead, same equivalence as AstNode::computeCastable uses.
        ਅਚਲ AstIfaceRefDType* ਅਚਲ asamep = VN_DBG_AS(samep, IfaceRefDType);
        ਵਾਪਸ iface_ਰਾਹੀਂ_Cellp() && iface_ਰਾਹੀਂ_Cellp() == asamep->iface_ਰਾਹੀਂ_Cellp()
               && modportName() == asamep->modportName() && isVirtual() == asamep->isVirtual();
    }
    ਪੂਰਨ ਚੌੜਾਈ_Align_ਬਾਈਟਾਂ() ਅਚਲ override { ਵਾਪਸ 0; }
    ਪੂਰਨ ਚੌੜਾਈ_ਕੁੱਲ_ਬਾਈਟਾਂ() ਅਚਲ override { ਵਾਪਸ 0; }
    bool isVirtual() ਅਚਲ { ਵਾਪਸ m_virtual; }
    ਖਾਲੀ isVirtual(bool flag) {
        m_virtual = flag;
        ਜੇ (flag) v3Global.ਸੈੱਟ_Has_Virt_Ifaces();
    }
    File_ਸਤਰ* modportFileline() ਅਚਲ { ਵਾਪਸ m_modportFileline; }
    ਖਾਲੀ modportFileline(File_ਸਤਰ* ਅਚਲ modportFileline) { m_modportFileline = modportFileline; }
    string cellName() ਅਚਲ { ਵਾਪਸ m_cellName; }
    ਖਾਲੀ cellName(ਅਚਲ string& name) { m_cellName = name; }
    string ifaceName() ਅਚਲ { ਵਾਪਸ m_ifaceName; }
    string ifaceNameQ() ਅਚਲ { ਵਾਪਸ "'" + prettyName(ifaceName()) + "'"; }
    ਖਾਲੀ ifaceName(ਅਚਲ string& name) { m_ifaceName = name; }
    ਖਾਲੀ modportName(ਅਚਲ string& modportName) { m_modportName = modportName; }
    string modportName() ਅਚਲ { ਵਾਪਸ m_modportName; }
    AstIface* iface_ਰਾਹੀਂ_Cellp() ਅਚਲ;  // Use cellp or ifacep
    AstIface* ifacep() ਅਚਲ { ਵਾਪਸ m_ifacep; }
    ਖਾਲੀ ifacep(AstIface* nodep) { m_ifacep = nodep; }
    AstCell* cellp() ਅਚਲ { ਵਾਪਸ m_cellp; }
    ਖਾਲੀ cellp(AstCell* nodep) { m_cellp = nodep; }
    AstModport* modportp() ਅਚਲ { ਵਾਪਸ m_modportp; }
    ਖਾਲੀ modportp(AstModport* modportp) { m_modportp = modportp; }
    bool isModport() { ਵਾਪਸ !m_modportName.ਖਾਲੀ_ਥਾਵਾਂ(); }
    bool isCompound() ਅਚਲ override { ਵਾਪਸ true; }  // But not relevant
};
class AstMemberDType final : ਜਨਤਕ Ast_ਗੰਢ_DType {
    // A member of a struct/union
    // PARENT: AstNodeUOrStructDType
    // @astgen op1 := childDTypep : Optional[AstNodeDType]
    // @astgen op3 := valuep : Optional[AstNode]
    //
    // @astgen ptr := m_refDTypep : Optional[AstNodeDType]  // Elements of this type (post-width)
    string m_name;  // Name of variable
    string m_ਟੈਗ;  // Holds the string of the verilator tag -- used in JSON output.
    ਪੂਰਨ m_lsb = -1;  // Within this level's packed struct, the LSB of the first bit of the member
    bool m_constrainedRand = false;  // Member has a constraint expression
    VRandAttr m_rand;  // Randomizability of this member (rand, randc, etc)
ਜਨਤਕ:
    AstMemberDType(File_ਸਤਰ* fl, ਅਚਲ string& name, VFlag_ਬੱਚਾ_DType, Ast_ਗੰਢ_DType* dtp,
                   Ast_ਗੰਢ* valuep)
        : ASTGEN_SUPER_MemberDType(fl)
        , m_name{name} {
        ਬੱਚਾ_DTypep(dtp);  // Only for parser
        this->valuep(valuep);
        dtypep(nullptr);  // V3Width will resolve
        refDTypep(nullptr);
    }
    AstMemberDType(File_ਸਤਰ* fl, ਅਚਲ string& name, Ast_ਗੰਢ_DType* dtp)
        : ASTGEN_SUPER_MemberDType(fl)
        , m_name{name} {
        UASSERT(dtp, "AstMember created with no dtype");
        refDTypep(dtp);
        dtypep(this);
        ਚੌੜਾਈ_From_Sub(subDTypep());
    }
    ASTGEN_MEMBERS_AstMemberDType;
    ਖਾਲੀ dump(std::ostream& str = std::cout) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str = std::cout) ਅਚਲ override;
    ਖਾਲੀ dumpSmall(std::ostream& str) ਅਚਲ override;
    string prettyDTypeName(bool ਭਰਿਆ_2) ਅਚਲ override;
    string name() ਅਚਲ override VL_MT_STABLE { ਵਾਪਸ m_name; }  // * = Var name
    bool hasDType() ਅਚਲ override VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ true; }
    bool maybe_Pointed_ਮੰਜ਼ਿਲ() ਅਚਲ override VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ true; }
    Ast_ਗੰਢ_DType* get_ਬੱਚਾ_DTypep() ਅਚਲ override { ਵਾਪਸ ਬੱਚਾ_DTypep(); }
    Ast_ਗੰਢ_UOr_Struct_DType* get_ਬੱਚਾ_Structp();
    Ast_ਗੰਢ_DType* subDTypep() ਅਚਲ override VL_MT_STABLE {
        ਵਾਪਸ m_refDTypep ? m_refDTypep : ਬੱਚਾ_DTypep();
    }
    ਖਾਲੀ refDTypep(Ast_ਗੰਢ_DType* nodep) { m_refDTypep = nodep; }
    Ast_ਗੰਢ_DType* virtRefDTypep() ਅਚਲ override { ਵਾਪਸ m_refDTypep; }
    ਖਾਲੀ virtRefDTypep(Ast_ਗੰਢ_DType* nodep) override { refDTypep(nodep); }
    bool similar_DType_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ_DType* samep) ਅਚਲ override { ਵਾਪਸ this == samep; }
    //
    // (Slow) recurse down to find basic data type (Note don't need virtual -
    // AstVar isn't a NodeDType)
    AstBasicDType* basicp() ਅਚਲ override VL_MT_STABLE { ਵਾਪਸ subDTypep()->basicp(); }
    // op1 = Range of variable (Note don't need virtual - AstVar isn't a NodeDType)
    Ast_ਗੰਢ_DType* dtypeSkipRefp() ਅਚਲ { ਵਾਪਸ subDTypep()->skipRefp(); }
    // (Slow) recurses - Structure alignment 1,2,4 or 8 bytes (arrays affect this)
    ਪੂਰਨ ਚੌੜਾਈ_Align_ਬਾਈਟਾਂ() ਅਚਲ override { ਵਾਪਸ subDTypep()->ਚੌੜਾਈ_Align_ਬਾਈਟਾਂ(); }
    // (Slow) recurses - Width in bytes rounding up 1,2,4,8,12,...
    ਪੂਰਨ ਚੌੜਾਈ_ਕੁੱਲ_ਬਾਈਟਾਂ() ਅਚਲ override { ਵਾਪਸ subDTypep()->ਚੌੜਾਈ_ਕੁੱਲ_ਬਾਈਟਾਂ(); }
    // METHODS
    ਖਾਲੀ name(ਅਚਲ string& name) override { m_name = name; }
    string ਟੈਗ() ਅਚਲ override { ਵਾਪਸ m_ਟੈਗ; }
    ਖਾਲੀ ਟੈਗ(ਅਚਲ string& ਪਾਠ) override { m_ਟੈਗ = ਪਾਠ; }
    ਪੂਰਨ lsb() ਅਚਲ { ਵਾਪਸ m_lsb; }
    ਖਾਲੀ lsb(ਪੂਰਨ lsb) { m_lsb = lsb; }
    bool isCompound() ਅਚਲ override {
        v3fatalSrc("call isCompound on subdata type, not reference");
        ਵਾਪਸ false;
    }
    bool isConstrainedRand() ਅਚਲ { ਵਾਪਸ m_constrainedRand; }
    ਖਾਲੀ markConstrainedRand(bool flag) { m_constrainedRand = flag; }
    VRandAttr rand() ਅਚਲ { ਵਾਪਸ m_rand; }
    ਖਾਲੀ rand(ਅਚਲ VRandAttr flag) { m_rand = flag; }
};
class Ast_NBACommit_ਕਤਾਰ_DType final : ਜਨਤਕ Ast_ਗੰਢ_DType {
    // @astgen ptr := m_subDTypep : AstNodeDType  // Type of the corresponding variable
    ਅਚਲ bool m_partial;  // Partial element update required

ਜਨਤਕ:
    Ast_NBACommit_ਕਤਾਰ_DType(File_ਸਤਰ* fl, Ast_ਗੰਢ_DType* subDTypep, bool partial)
        : ASTGEN_SUPER_NBACommit_ਕਤਾਰ_DType(fl)
        , m_partial{partial}
        , m_subDTypep{subDTypep} {
        dtypep(this);
    }
    ASTGEN_MEMBERS_Ast_NBACommit_ਕਤਾਰ_DType;
    ਖਾਲੀ dump(std::ostream& str) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str) ਅਚਲ override;
    Ast_ਗੰਢ_DType* subDTypep() ਅਚਲ override VL_MT_STABLE { ਵਾਪਸ m_subDTypep; }
    bool partial() ਅਚਲ { ਵਾਪਸ m_partial; }
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* samep) ਅਚਲ override {
        ਅਚਲ Ast_NBACommit_ਕਤਾਰ_DType* ਅਚਲ asamep = VN_DBG_AS(samep, NBACommit_ਕਤਾਰ_DType);
        ਵਾਪਸ m_partial == asamep->m_partial;
    }
    bool similar_DType_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ_DType* samep) ਅਚਲ override { ਵਾਪਸ this == samep; }
    AstBasicDType* basicp() ਅਚਲ override VL_MT_STABLE { ਵਾਪਸ nullptr; }
    ਪੂਰਨ ਚੌੜਾਈ_Align_ਬਾਈਟਾਂ() ਅਚਲ override { ਵਾਪਸ 1; }
    ਪੂਰਨ ਚੌੜਾਈ_ਕੁੱਲ_ਬਾਈਟਾਂ() ਅਚਲ override { ਵਾਪਸ 24; }
    bool isCompound() ਅਚਲ override { ਵਾਪਸ true; }
};
class AstParamTypeDType final : ਜਨਤਕ Ast_ਗੰਢ_DType {
    // Parents: MODULE
    // A parameter type statement; much like a var or typedef
    // @astgen op1 := childDTypep : Optional[AstNodeDType]
    ਅਚਲ VVarType m_varType;  // Type of variable (for localparam vs. param)
    ਅਚਲ VFwdType m_fwdType;  // Forward type for lint check
    string m_name;  // Name of variable
ਜਨਤਕ:
    AstParamTypeDType(File_ਸਤਰ* fl, VVarType type, VFwdType fwdType, ਅਚਲ string& name,
                      VFlag_ਬੱਚਾ_DType, Ast_ਗੰਢ_DType* dtp)
        : ASTGEN_SUPER_ParamTypeDType(fl)
        , m_varType{type}
        , m_fwdType{fwdType}
        , m_name{name} {
        ਬੱਚਾ_DTypep(dtp);  // Only for parser
        dtypep(nullptr);  // V3Width will resolve
    }
    ASTGEN_MEMBERS_AstParamTypeDType;
    ਖਾਲੀ dump(std::ostream& str = std::cout) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str = std::cout) ਅਚਲ override;
    string prettyDTypeName(bool ਭਰਿਆ_2) ਅਚਲ override;
    Ast_ਗੰਢ_DType* get_ਬੱਚਾ_DTypep() ਅਚਲ override { ਵਾਪਸ ਬੱਚਾ_DTypep(); }
    Ast_ਗੰਢ_DType* subDTypep() ਅਚਲ override VL_MT_STABLE {
        ਵਾਪਸ dtypep() ? dtypep() : ਬੱਚਾ_DTypep();
    }
    AstBasicDType* basicp() ਅਚਲ override VL_MT_STABLE { ਵਾਪਸ subDTypep()->basicp(); }
    bool similar_DType_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ_DType* samep) ਅਚਲ override {
        ਅਚਲ AstParamTypeDType* ਅਚਲ sp = VN_DBG_AS(samep, ParamTypeDType);
        ਵਾਪਸ this->subDTypep()->similarDType(sp->subDTypep());
    }
    ਪੂਰਨ ਚੌੜਾਈ_Align_ਬਾਈਟਾਂ() ਅਚਲ override { ਵਾਪਸ dtypep()->ਚੌੜਾਈ_Align_ਬਾਈਟਾਂ(); }
    ਪੂਰਨ ਚੌੜਾਈ_ਕੁੱਲ_ਬਾਈਟਾਂ() ਅਚਲ override { ਵਾਪਸ dtypep()->ਚੌੜਾਈ_ਕੁੱਲ_ਬਾਈਟਾਂ(); }
    // METHODS
    string name() ਅਚਲ override VL_MT_STABLE { ਵਾਪਸ m_name; }
    bool maybe_Pointed_ਮੰਜ਼ਿਲ() ਅਚਲ override VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ true; }
    bool hasDType() ਅਚਲ override VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ true; }
    ਖਾਲੀ name(ਅਚਲ string& flag) override { m_name = flag; }
    VVarType varType() ਅਚਲ { ਵਾਪਸ m_varType; }  // * = Type of variable
    VFwdType fwdType() ਅਚਲ { ਵਾਪਸ m_fwdType; }
    bool isParam() ਅਚਲ { ਵਾਪਸ true; }
    bool isGParam() ਅਚਲ { ਵਾਪਸ (varType() == VVarType::GPARAM); }
    bool isCompound() ਅਚਲ override {
        v3fatalSrc("call isCompound on subdata type, not reference");
        ਵਾਪਸ false;
    }
};
class AstParseTypeDType final : ਜਨਤਕ Ast_ਗੰਢ_DType {
    // Parents: VAR
    // During parsing, this indicates the type of a parameter is a "parameter type"
    // e.g. the data type is a container of any data type
    ਅਚਲ VFwdType m_fwdType;  // Forward type for lint check
ਜਨਤਕ:
    explicit AstParseTypeDType(File_ਸਤਰ* fl, VFwdType fwdType = VFwdType::NONE)
        : ASTGEN_SUPER_ParseTypeDType(fl)
        , m_fwdType{fwdType} {}
    ASTGEN_MEMBERS_AstParseTypeDType;
    Ast_ਗੰਢ_DType* dtypep() ਅਚਲ VL_MT_STABLE { ਵਾਪਸ nullptr; }
    // METHODS
    ਖਾਲੀ dump(std::ostream& str = std::cout) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str = std::cout) ਅਚਲ override;
    bool similar_DType_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ_DType* samep) ਅਚਲ override { ਵਾਪਸ this == samep; }
    AstBasicDType* basicp() ਅਚਲ override VL_MT_STABLE { ਵਾਪਸ nullptr; }
    ਪੂਰਨ ਚੌੜਾਈ_Align_ਬਾਈਟਾਂ() ਅਚਲ override { ਵਾਪਸ 0; }
    ਪੂਰਨ ਚੌੜਾਈ_ਕੁੱਲ_ਬਾਈਟਾਂ() ਅਚਲ override { ਵਾਪਸ 0; }
    bool isCompound() ਅਚਲ override {
        v3fatalSrc("call isCompound on subdata type, not reference");
        ਵਾਪਸ false;
    }
    VFwdType fwdType() ਅਚਲ { ਵਾਪਸ m_fwdType; }
};
class Ast_ਕਤਾਰ_DType final : ਜਨਤਕ Ast_ਗੰਢ_DType {
    // Queue array data type, ie "[ $ ]"
    // @astgen op1 := childDTypep : Optional[AstNodeDType] // moved to refDTypep() in V3Width
    // @astgen op2 := boundp : Optional[AstNodeExpr]
    //
    // @astgen ptr := m_refDTypep : Optional[AstNodeDType]  // Elements of this type (post-width)
ਜਨਤਕ:
    Ast_ਕਤਾਰ_DType(File_ਸਤਰ* fl, VFlag_ਬੱਚਾ_DType, Ast_ਗੰਢ_DType* dtp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* boundp)
        : ASTGEN_SUPER_ਕਤਾਰ_DType(fl) {
        ਬੱਚਾ_DTypep(dtp);
        this->boundp(boundp);
        refDTypep(nullptr);
        dtypep(nullptr);  // V3Width will resolve
    }
    Ast_ਕਤਾਰ_DType(File_ਸਤਰ* fl, Ast_ਗੰਢ_DType* dtp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* boundp)
        : ASTGEN_SUPER_ਕਤਾਰ_DType(fl) {
        this->boundp(boundp);
        refDTypep(dtp);
        dtypep(dtp);
    }
    ASTGEN_MEMBERS_Ast_ਕਤਾਰ_DType;
    ਅਚਲ ਅੱਖਰ* broken() ਅਚਲ override {
        BROKEN_RTN(!((m_refDTypep && !ਬੱਚਾ_DTypep()) || (!m_refDTypep && ਬੱਚਾ_DTypep())));
        ਵਾਪਸ nullptr;
    }
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* samep) ਅਚਲ override {
        ਅਚਲ Ast_ਕਤਾਰ_DType* ਅਚਲ asamep = VN_DBG_AS(samep, ਕਤਾਰ_DType);
        ਜੇ (!asamep->subDTypep()) ਵਾਪਸ false;
        ਵਾਪਸ (subDTypep() == asamep->subDTypep());
    }
    bool similar_DType_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ_DType* samep) ਅਚਲ override {
        ਅਚਲ Ast_ਕਤਾਰ_DType* ਅਚਲ asamep = VN_DBG_AS(samep, ਕਤਾਰ_DType);
        ਵਾਪਸ asamep->subDTypep() && subDTypep()->similarDType(asamep->subDTypep());
    }
    ਖਾਲੀ dumpSmall(std::ostream& str) ਅਚਲ override;
    string prettyDTypeName(bool ਭਰਿਆ_2) ਅਚਲ override;
    Ast_ਗੰਢ_DType* get_ਬੱਚਾ_DTypep() ਅਚਲ override { ਵਾਪਸ ਬੱਚਾ_DTypep(); }
    Ast_ਗੰਢ_DType* subDTypep() ਅਚਲ override VL_MT_STABLE {
        ਵਾਪਸ m_refDTypep ? m_refDTypep : ਬੱਚਾ_DTypep();
    }
    bool isAggregateType() ਅਚਲ override { ਵਾਪਸ true; }
    ਖਾਲੀ refDTypep(Ast_ਗੰਢ_DType* nodep) { m_refDTypep = nodep; }
    ਸਤਰਅੰਦਰ ਪੂਰਨ boundConst() ਅਚਲ VL_MT_STABLE;
    Ast_ਗੰਢ_DType* virtRefDTypep() ਅਚਲ override { ਵਾਪਸ m_refDTypep; }
    ਖਾਲੀ virtRefDTypep(Ast_ਗੰਢ_DType* nodep) override { refDTypep(nodep); }
    // METHODS
    AstBasicDType* basicp() ਅਚਲ override VL_MT_STABLE { ਵਾਪਸ nullptr; }
    ਪੂਰਨ ਚੌੜਾਈ_Align_ਬਾਈਟਾਂ() ਅਚਲ override { ਵਾਪਸ subDTypep()->ਚੌੜਾਈ_Align_ਬਾਈਟਾਂ(); }
    ਪੂਰਨ ਚੌੜਾਈ_ਕੁੱਲ_ਬਾਈਟਾਂ() ਅਚਲ override { ਵਾਪਸ subDTypep()->ਚੌੜਾਈ_ਕੁੱਲ_ਬਾਈਟਾਂ(); }
    bool isCompound() ਅਚਲ override { ਵਾਪਸ true; }
    bool isDynamicallySized() ਅਚਲ override { ਵਾਪਸ true; }
};
class AstRefDType final : ਜਨਤਕ Ast_ਗੰਢ_DType {
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
    ਅਚਲ VIface_Capture_ਟੈਗ* m_captureTagp = nullptr;

ਜਨਤਕ:
    AstRefDType(File_ਸਤਰ* fl, ਅਚਲ string& name)
        : ASTGEN_SUPER_RefDType(fl)
        , m_name{name} {}
    AstRefDType(File_ਸਤਰ* fl, ਅਚਲ string& name, Ast_ਗੰਢ_ਵਿਅੰਜਕ* classOrPackagep, AstPin* paramsp)
        : ASTGEN_SUPER_RefDType(fl)
        , m_name{name} {
        classOrPackageOpp(classOrPackagep);
        ਜੋੜੋ_Paramsp(paramsp);
    }
    class Flag_Type_ਦਾ_ਵਿਅੰਜਕ {};  // type(expr) for parser only
    AstRefDType(File_ਸਤਰ* fl, Flag_Type_ਦਾ_ਵਿਅੰਜਕ, Ast_ਗੰਢ* typeofp)
        : ASTGEN_SUPER_RefDType(fl) {
        this->typeofp(typeofp);
        ਜੇ (Ast_ਗੰਢ_DType* ਅਚਲ dtp = VN_CAST(typeofp, ਗੰਢ_DType)) refDTypep(dtp);
    }
    ASTGEN_MEMBERS_AstRefDType;
    // METHODS
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* samep) ਅਚਲ override {
        ਅਚਲ AstRefDType* ਅਚਲ asamep = VN_DBG_AS(samep, RefDType);
        ਵਾਪਸ (m_typedefp == asamep->m_typedefp && m_refDTypep == asamep->m_refDTypep
                && m_name == asamep->m_name && m_classOrPackagep == asamep->m_classOrPackagep);
    }
    bool similar_DType_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ_DType* samep) ਅਚਲ override {
        ਵਾਪਸ subDTypep()->similarDType(samep->subDTypep());
    }
    ਅਚਲ ਅੱਖਰ* broken() ਅਚਲ override;
    ਖਾਲੀ dump(std::ostream& str = std::cout) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str = std::cout) ਅਚਲ override;
    ਖਾਲੀ dumpSmall(std::ostream& str) ਅਚਲ override;
    string name() ਅਚਲ override VL_MT_STABLE { ਵਾਪਸ m_name; }
    string prettyDTypeName(bool ਭਰਿਆ_2) ਅਚਲ override {
        ਵਾਪਸ subDTypep() ? subDTypep()->prettyDTypeName(ਭਰਿਆ_2) : prettyName();
    }
    AstBasicDType* basicp() ਅਚਲ override VL_MT_STABLE {
        ਵਾਪਸ subDTypep() ? subDTypep()->basicp() : nullptr;
    }
    Ast_ਗੰਢ_DType* subDTypep() ਅਚਲ override VL_MT_STABLE;
    Ast_ਗੰਢ_DType* get_ਬੱਚਾ_DTypep() ਅਚਲ override { ਵਾਪਸ VN_CAST(typeofp(), ਗੰਢ_DType); }
    ਪੂਰਨ ਚੌੜਾਈ_Align_ਬਾਈਟਾਂ() ਅਚਲ override { ਵਾਪਸ dtypeSkipRefp()->ਚੌੜਾਈ_Align_ਬਾਈਟਾਂ(); }
    ਪੂਰਨ ਚੌੜਾਈ_ਕੁੱਲ_ਬਾਈਟਾਂ() ਅਚਲ override { ਵਾਪਸ dtypeSkipRefp()->ਚੌੜਾਈ_ਕੁੱਲ_ਬਾਈਟਾਂ(); }
    ਖਾਲੀ name(ਅਚਲ string& flag) override { m_name = flag; }
    Ast_ਗੰਢ_DType* dtypeSkipRefp() ਅਚਲ { ਵਾਪਸ subDTypep()->skipRefp(); }
    AstTypedef* typedefp() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ m_typedefp; }
    ਖਾਲੀ typedefp(AstTypedef* nodep) { m_typedefp = nodep; }
    Ast_ਗੰਢ_DType* refDTypep() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ m_refDTypep; }
    ਖਾਲੀ refDTypep(Ast_ਗੰਢ_DType* nodep) { m_refDTypep = nodep; }
    Ast_ਗੰਢ_DType* virtRefDTypep() ਅਚਲ override { ਵਾਪਸ refDTypep(); }
    ਖਾਲੀ virtRefDTypep(Ast_ਗੰਢ_DType* nodep) override { refDTypep(nodep); }
    Ast_ਗੰਢ_Module* classOrPackagep() ਅਚਲ { ਵਾਪਸ m_classOrPackagep; }
    ਖਾਲੀ classOrPackagep(Ast_ਗੰਢ_Module* nodep) { m_classOrPackagep = nodep; }
    ਅਚਲ VIface_Capture_ਟੈਗ* captureTagp() ਅਚਲ { ਵਾਪਸ m_captureTagp; }
    ਖਾਲੀ captureTagp(ਅਚਲ VIface_Capture_ਟੈਗ* tagp) { m_captureTagp = tagp; }
    bool isCompound() ਅਚਲ override {
        v3fatalSrc("call isCompound on subdata type, not reference");
        ਵਾਪਸ false;
    }
};
class AstRequireDType final : ਜਨਤਕ Ast_ਗੰਢ_DType {
    // @astgen op1 := lhsp : Optional[AstNode<AstNodeExpr|AstNodeDType>]
    //
    // Require a generic node type (typically AstParseRef become a type.
ਜਨਤਕ:
    AstRequireDType(File_ਸਤਰ* fl, Ast_ਗੰਢ* lhsp)
        : ASTGEN_SUPER_RequireDType(fl) {
        this->lhsp(lhsp);
    }
    ASTGEN_MEMBERS_AstRequireDType;
    // METHODS
    bool similar_DType_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ_DType* samep) ਅਚਲ override {
        ਅਚਲ AstRequireDType* ਅਚਲ asamep = VN_DBG_AS(samep, RequireDType);
        ਵਾਪਸ subDTypep()->similarDType(asamep->subDTypep());
    }
    AstBasicDType* basicp() ਅਚਲ override VL_MT_STABLE { ਵਾਪਸ nullptr; }
    Ast_ਗੰਢ_DType* subDTypep() ਅਚਲ override VL_MT_STABLE {
        // Used for recursive definition checking
        ਜੇ (Ast_ਗੰਢ_DType* ਅਚਲ dtp = VN_CAST(lhsp(), ਗੰਢ_DType)) ਵਾਪਸ dtp;

        ਵਾਪਸ nullptr;
    }
    ਪੂਰਨ ਚੌੜਾਈ_Align_ਬਾਈਟਾਂ() ਅਚਲ override { V3ERROR_NA_RETURN(1); }
    ਪੂਰਨ ਚੌੜਾਈ_ਕੁੱਲ_ਬਾਈਟਾਂ() ਅਚਲ override { V3ERROR_NA_RETURN(1); }
    bool isCompound() ਅਚਲ override { V3ERROR_NA_RETURN(false); }
};
class Ast_Sample_ਕਤਾਰ_DType final : ਜਨਤਕ Ast_ਗੰਢ_DType {
    // @astgen op1 := childDTypep : Optional[AstNodeDType] // moved to refDTypep() in V3Width
    //
    // @astgen ptr := m_refDTypep : Optional[AstNodeDType]  // Elements of this type (post-width)
ਜਨਤਕ:
    Ast_Sample_ਕਤਾਰ_DType(File_ਸਤਰ* fl, Ast_ਗੰਢ_DType* dtp)
        : ASTGEN_SUPER_Sample_ਕਤਾਰ_DType(fl) {
        refDTypep(dtp);
        dtypep(dtp);
    }
    ASTGEN_MEMBERS_Ast_Sample_ਕਤਾਰ_DType;
    ਅਚਲ ਅੱਖਰ* broken() ਅਚਲ override {
        BROKEN_RTN(!((m_refDTypep && !ਬੱਚਾ_DTypep()) || (!m_refDTypep && ਬੱਚਾ_DTypep())));
        ਵਾਪਸ nullptr;
    }
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* samep) ਅਚਲ override {
        ਅਚਲ Ast_Sample_ਕਤਾਰ_DType* ਅਚਲ asamep = VN_DBG_AS(samep, Sample_ਕਤਾਰ_DType);
        ਜੇ (!asamep->subDTypep()) ਵਾਪਸ false;
        ਵਾਪਸ (subDTypep() == asamep->subDTypep());
    }
    bool similar_DType_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ_DType* samep) ਅਚਲ override {
        ਅਚਲ Ast_Sample_ਕਤਾਰ_DType* ਅਚਲ asamep = VN_DBG_AS(samep, Sample_ਕਤਾਰ_DType);
        ਵਾਪਸ asamep->subDTypep() && subDTypep()->similarDType(asamep->subDTypep());
    }
    ਖਾਲੀ dumpSmall(std::ostream& str) ਅਚਲ override;
    Ast_ਗੰਢ_DType* get_ਬੱਚਾ_DTypep() ਅਚਲ override { ਵਾਪਸ ਬੱਚਾ_DTypep(); }
    // op1 = Range of variable
    Ast_ਗੰਢ_DType* subDTypep() ਅਚਲ override VL_MT_STABLE {
        ਵਾਪਸ m_refDTypep ? m_refDTypep : ਬੱਚਾ_DTypep();
    }
    ਖਾਲੀ refDTypep(Ast_ਗੰਢ_DType* nodep) { m_refDTypep = nodep; }
    Ast_ਗੰਢ_DType* virtRefDTypep() ਅਚਲ override { ਵਾਪਸ m_refDTypep; }
    ਖਾਲੀ virtRefDTypep(Ast_ਗੰਢ_DType* nodep) override { refDTypep(nodep); }
    // METHODS
    AstBasicDType* basicp() ਅਚਲ override VL_MT_STABLE { ਵਾਪਸ subDTypep()->basicp(); }
    ਪੂਰਨ ਚੌੜਾਈ_Align_ਬਾਈਟਾਂ() ਅਚਲ override { ਵਾਪਸ ਆਕਾਰ(std::map<std::string, std::string>); }
    ਪੂਰਨ ਚੌੜਾਈ_ਕੁੱਲ_ਬਾਈਟਾਂ() ਅਚਲ override { ਵਾਪਸ ਆਕਾਰ(std::map<std::string, std::string>); }
    bool isCompound() ਅਚਲ override { ਵਾਪਸ true; }
};
class Ast_ਧਾਰਾ_DType final : ਜਨਤਕ Ast_ਗੰਢ_DType {
    // Stream data type, used only as data type of stream operations
    // Should behave like AstPackArrayDType, but it doesn't have a size
ਜਨਤਕ:
    explicit Ast_ਧਾਰਾ_DType(File_ਸਤਰ* fl)
        : ASTGEN_SUPER_ਧਾਰਾ_DType(fl) {
        dtypep(this);
    }
    ASTGEN_MEMBERS_Ast_ਧਾਰਾ_DType;
    ਖਾਲੀ dumpSmall(std::ostream& str) ਅਚਲ override;
    bool hasDType() ਅਚਲ override VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ true; }
    bool maybe_Pointed_ਮੰਜ਼ਿਲ() ਅਚਲ override VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ true; }
    bool undead() ਅਚਲ override { ਵਾਪਸ true; }
    Ast_ਗੰਢ_DType* subDTypep() ਅਚਲ override VL_MT_STABLE { ਵਾਪਸ nullptr; }
    Ast_ਗੰਢ_DType* virtRefDTypep() ਅਚਲ override { ਵਾਪਸ nullptr; }
    ਖਾਲੀ virtRefDTypep(Ast_ਗੰਢ_DType* nodep) override {}
    bool similar_DType_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ_DType* samep) ਅਚਲ override { ਵਾਪਸ this == samep; }
    AstBasicDType* basicp() ਅਚਲ override VL_MT_STABLE { ਵਾਪਸ nullptr; }
    ਪੂਰਨ ਚੌੜਾਈ_Align_ਬਾਈਟਾਂ() ਅਚਲ override { ਵਾਪਸ 1; }
    ਪੂਰਨ ਚੌੜਾਈ_ਕੁੱਲ_ਬਾਈਟਾਂ() ਅਚਲ override { ਵਾਪਸ 1; }
    bool isCompound() ਅਚਲ override { ਵਾਪਸ false; }
};
class AstUnsizedArrayDType final : ਜਨਤਕ Ast_ਗੰਢ_DType {
    // Unsized/open-range Array data type, ie "some_dtype var_name []"
    // @astgen op1 := childDTypep : Optional[AstNodeDType] // moved to refDTypep() in V3Width
    //
    // @astgen ptr := m_refDTypep : Optional[AstNodeDType]  // Elements of this type (post-width)
ਜਨਤਕ:
    AstUnsizedArrayDType(File_ਸਤਰ* fl, VFlag_ਬੱਚਾ_DType, Ast_ਗੰਢ_DType* dtp)
        : ASTGEN_SUPER_UnsizedArrayDType(fl) {
        ਬੱਚਾ_DTypep(dtp);  // Only for parser
        refDTypep(nullptr);
        dtypep(nullptr);  // V3Width will resolve
    }
    AstUnsizedArrayDType(File_ਸਤਰ* fl, Ast_ਗੰਢ_DType* dtp)
        : ASTGEN_SUPER_UnsizedArrayDType(fl) {
        refDTypep(dtp);
        dtypep(nullptr);  // V3Width will resolve
    }
    ASTGEN_MEMBERS_AstUnsizedArrayDType;
    ਅਚਲ ਅੱਖਰ* broken() ਅਚਲ override {
        BROKEN_RTN(!((m_refDTypep && !ਬੱਚਾ_DTypep()) || (!m_refDTypep && ਬੱਚਾ_DTypep())));
        ਵਾਪਸ nullptr;
    }
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* samep) ਅਚਲ override;
    bool similar_DType_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ_DType* samep) ਅਚਲ override;
    ਖਾਲੀ dumpSmall(std::ostream& str) ਅਚਲ override;
    Ast_ਗੰਢ_DType* get_ਬੱਚਾ_DTypep() ਅਚਲ override { ਵਾਪਸ ਬੱਚਾ_DTypep(); }
    Ast_ਗੰਢ_DType* subDTypep() ਅਚਲ override VL_MT_STABLE {
        ਵਾਪਸ m_refDTypep ? m_refDTypep : ਬੱਚਾ_DTypep();
    }
    ਖਾਲੀ refDTypep(Ast_ਗੰਢ_DType* nodep) { m_refDTypep = nodep; }
    Ast_ਗੰਢ_DType* virtRefDTypep() ਅਚਲ override { ਵਾਪਸ m_refDTypep; }
    ਖਾਲੀ virtRefDTypep(Ast_ਗੰਢ_DType* nodep) override { refDTypep(nodep); }
    // METHODS
    AstBasicDType* basicp() ਅਚਲ override VL_MT_STABLE { ਵਾਪਸ subDTypep()->basicp(); }
    ਪੂਰਨ ਚੌੜਾਈ_Align_ਬਾਈਟਾਂ() ਅਚਲ override { ਵਾਪਸ subDTypep()->ਚੌੜਾਈ_Align_ਬਾਈਟਾਂ(); }
    ਪੂਰਨ ਚੌੜਾਈ_ਕੁੱਲ_ਬਾਈਟਾਂ() ਅਚਲ override { ਵਾਪਸ subDTypep()->ਚੌੜਾਈ_ਕੁੱਲ_ਬਾਈਟਾਂ(); }
    bool isCompound() ਅਚਲ override { ਵਾਪਸ true; }
};
class AstVoidDType final : ਜਨਤਕ Ast_ਗੰਢ_DType {
    // For e.g. a function returning void
ਜਨਤਕ:
    explicit AstVoidDType(File_ਸਤਰ* fl)
        : ASTGEN_SUPER_VoidDType(fl) {
        dtypep(this);
    }
    ASTGEN_MEMBERS_AstVoidDType;
    ਖਾਲੀ dumpSmall(std::ostream& str) ਅਚਲ override;
    bool hasDType() ਅਚਲ override VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ true; }
    bool maybe_Pointed_ਮੰਜ਼ਿਲ() ਅਚਲ override VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ true; }
    bool undead() ਅਚਲ override { ਵਾਪਸ true; }
    Ast_ਗੰਢ_DType* subDTypep() ਅਚਲ override VL_MT_STABLE { ਵਾਪਸ nullptr; }
    Ast_ਗੰਢ_DType* virtRefDTypep() ਅਚਲ override { ਵਾਪਸ nullptr; }
    ਖਾਲੀ virtRefDTypep(Ast_ਗੰਢ_DType* nodep) override {}
    bool similar_DType_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ_DType* samep) ਅਚਲ override { ਵਾਪਸ true; }
    AstBasicDType* basicp() ਅਚਲ override VL_MT_STABLE { ਵਾਪਸ nullptr; }
    ਪੂਰਨ ਚੌੜਾਈ_Align_ਬਾਈਟਾਂ() ਅਚਲ override { ਵਾਪਸ 1; }
    ਪੂਰਨ ਚੌੜਾਈ_ਕੁੱਲ_ਬਾਈਟਾਂ() ਅਚਲ override { ਵਾਪਸ 1; }
    bool isCompound() ਅਚਲ override { ਵਾਪਸ false; }
};
class AstWildcardArrayDType final : ਜਨਤਕ Ast_ਗੰਢ_DType {
    // Wildcard index type associative array data type, ie "some_dtype var_name [*]"
    // @astgen op1 := childDTypep : Optional[AstNodeDType] // moved to refDTypep() in V3Width
    //
    // @astgen ptr := m_refDTypep : Optional[AstNodeDType]  // Elements of this type (post-width)
ਜਨਤਕ:
    AstWildcardArrayDType(File_ਸਤਰ* fl, VFlag_ਬੱਚਾ_DType, Ast_ਗੰਢ_DType* dtp)
        : ASTGEN_SUPER_WildcardArrayDType(fl) {
        ਬੱਚਾ_DTypep(dtp);  // Only for parser
        refDTypep(nullptr);
        dtypep(nullptr);  // V3Width will resolve
    }
    ASTGEN_MEMBERS_AstWildcardArrayDType;
    ਅਚਲ ਅੱਖਰ* broken() ਅਚਲ override {
        BROKEN_RTN(!((m_refDTypep && !ਬੱਚਾ_DTypep()) || (!m_refDTypep && ਬੱਚਾ_DTypep())));
        ਵਾਪਸ nullptr;
    }
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* samep) ਅਚਲ override;
    bool similar_DType_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ_DType* samep) ਅਚਲ override;
    ਖਾਲੀ dumpSmall(std::ostream& str) ਅਚਲ override;
    string prettyDTypeName(bool ਭਰਿਆ_2) ਅਚਲ override;
    Ast_ਗੰਢ_DType* get_ਬੱਚਾ_DTypep() ਅਚਲ override { ਵਾਪਸ ਬੱਚਾ_DTypep(); }
    Ast_ਗੰਢ_DType* subDTypep() ਅਚਲ override VL_MT_STABLE {
        ਵਾਪਸ m_refDTypep ? m_refDTypep : ਬੱਚਾ_DTypep();
    }
    ਖਾਲੀ refDTypep(Ast_ਗੰਢ_DType* nodep) { m_refDTypep = nodep; }
    Ast_ਗੰਢ_DType* virtRefDTypep() ਅਚਲ override { ਵਾਪਸ m_refDTypep; }
    ਖਾਲੀ virtRefDTypep(Ast_ਗੰਢ_DType* nodep) override { refDTypep(nodep); }
    // METHODS
    AstBasicDType* basicp() ਅਚਲ override VL_MT_STABLE { ਵਾਪਸ subDTypep()->basicp(); }
    ਪੂਰਨ ਚੌੜਾਈ_Align_ਬਾਈਟਾਂ() ਅਚਲ override { ਵਾਪਸ ਆਕਾਰ(std::map<std::string, std::string>); }
    ਪੂਰਨ ਚੌੜਾਈ_ਕੁੱਲ_ਬਾਈਟਾਂ() ਅਚਲ override { ਵਾਪਸ ਆਕਾਰ(std::map<std::string, std::string>); }
    bool isCompound() ਅਚਲ override { ਵਾਪਸ true; }
    bool isDynamicallySized() ਅਚਲ override { ਵਾਪਸ true; }
};

// === AstNodeArrayDType ===
class AstPackArrayDType final : ਜਨਤਕ Ast_ਗੰਢ_Array_DType {
    // Packed array data type, ie "some_dtype [2:0] var_name"
ਜਨਤਕ:
    ਸਤਰਅੰਦਰ AstPackArrayDType(File_ਸਤਰ* fl, VFlag_ਬੱਚਾ_DType, Ast_ਗੰਢ_DType* dtp, AstRange* rangep);
    ਸਤਰਅੰਦਰ AstPackArrayDType(File_ਸਤਰ* fl, Ast_ਗੰਢ_DType* dtp, AstRange* rangep);
    ASTGEN_MEMBERS_AstPackArrayDType;
    string prettyDTypeName(bool ਭਰਿਆ_2) ਅਚਲ override;
    bool isCompound() ਅਚਲ override { ਵਾਪਸ false; }
};
class AstUnpackArrayDType final : ਜਨਤਕ Ast_ਗੰਢ_Array_DType {
    // Array data type, ie "some_dtype var_name [2:0]"
    bool m_isCompound = false;  // Non-POD subDType, or parent requires compound
ਜਨਤਕ:
    AstUnpackArrayDType(File_ਸਤਰ* fl, VFlag_ਬੱਚਾ_DType, Ast_ਗੰਢ_DType* dtp, AstRange* rangep)
        : ASTGEN_SUPER_UnpackArrayDType(fl) {
        ਬੱਚਾ_DTypep(dtp);  // Only for parser
        this->rangep(rangep);
        refDTypep(nullptr);
        dtypep(nullptr);  // V3Width will resolve
        // For backward compatibility AstNodeArrayDType and others inherit
        // width and signing from the subDType/base type
        ਚੌੜਾਈ_From_Sub(subDTypep());
    }
    AstUnpackArrayDType(File_ਸਤਰ* fl, Ast_ਗੰਢ_DType* dtp, AstRange* rangep)
        : ASTGEN_SUPER_UnpackArrayDType(fl) {
        this->rangep(rangep);
        refDTypep(dtp);
        dtypep(this);
        // For backward compatibility AstNodeArrayDType and others inherit
        // width and signing from the subDType/base type
        ਚੌੜਾਈ_From_Sub(subDTypep());
    }
    ASTGEN_MEMBERS_AstUnpackArrayDType;
    ਖਾਲੀ dump(std::ostream& str) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str) ਅਚਲ override;
    string prettyDTypeName(bool ਭਰਿਆ_2) ਅਚਲ override;
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* samep) ਅਚਲ override {
        ਅਚਲ AstUnpackArrayDType* ਅਚਲ sp = VN_DBG_AS(samep, UnpackArrayDType);
        ਵਾਪਸ m_isCompound == sp->m_isCompound;
    }
    bool similar_DType_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ_DType* samep) ਅਚਲ override;
    bool isAggregateType() ਅਚਲ override { ਵਾਪਸ true; }
    // True iff 'otherp' is an array of the same shape, with
    // at least one dimension reversed ascending/descending.
    bool is_ਉਲਟ_ਦਾ(ਅਚਲ Ast_ਗੰਢ_DType* otherp) ਅਚਲ;
    // Outer dimension comes first. The first element is this node.
    std::ਸਦਿਸ਼<AstUnpackArrayDType*> unpackDimensions();
    ਖਾਲੀ isCompound(bool flag) { m_isCompound = flag; }
    bool isCompound() ਅਚਲ override VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ m_isCompound; }
    bool isIntegralOrPacked() ਅਚਲ override { ਵਾਪਸ false; }
};

// === AstNodeUOrStructDType ===
class AstStructDType final : ਜਨਤਕ Ast_ਗੰਢ_UOr_Struct_DType {
ਜਨਤਕ:
    // VSigning below is mispurposed to indicate if packed or not
    AstStructDType(File_ਸਤਰ* fl, VSigning numericUnpack)
        : ASTGEN_SUPER_StructDType(fl, numericUnpack) {}
    ASTGEN_MEMBERS_AstStructDType;
    string verilogKwd() ਅਚਲ override { ਵਾਪਸ "struct"; }
};
class AstUnionDType final : ਜਨਤਕ Ast_ਗੰਢ_UOr_Struct_DType {
    bool m_isSoft;  // Is a "union soft"
    bool m_isTagged;  // Is a "union tagged"

ਜਨਤਕ:
    // VSigning below is mispurposed to indicate if packed or not
    // isSoft implies packed
    AstUnionDType(File_ਸਤਰ* fl, bool isSoft, bool isTagged, VSigning numericUnpack)
        : ASTGEN_SUPER_UnionDType(fl, numericUnpack)
        , m_isSoft{isSoft}
        , m_isTagged{isTagged} {
        packed(packed() | m_isSoft);
    }
    ASTGEN_MEMBERS_AstUnionDType;
    string verilogKwd() ਅਚਲ override { ਵਾਪਸ "union"; }
    bool isSoft() ਅਚਲ { ਵਾਪਸ m_isSoft; }
    bool isTagged() ਅਚਲ { ਵਾਪਸ m_isTagged; }
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* samep) ਅਚਲ override;
    ਖਾਲੀ dump(std::ostream& str) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str) ਅਚਲ override;
};

#ਜੇ_ਅੰਤ  // Guard
