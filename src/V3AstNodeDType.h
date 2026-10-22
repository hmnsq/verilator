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

#నిర్వచితం_కాకపోతే VERILATOR_V3ASTNODEDTYPE_H_
#నిర్వచించు VERILATOR_V3ASTNODEDTYPE_H_

#నిర్వచితం_కాకపోతే VERILATOR_V3AST_H_
#దోషం "Use V3Ast.h as the include"
#అన్తర్భావ "V3Ast.h"  // This helps code analysis tools pick up symbols in V3Ast.h
#నిర్వచించు VL_NOT_FINAL  // This #define fixes broken code folding in the CLion IDE
#అయితే_ముగింపు

// === Abstract base node types (AstNode*) =====================================

class Ast_కణుపు_DType VL_NOT_FINAL : బహిరంగ Ast_కణుపు {
    // Ideally width() would migrate to BasicDType as that's where it makes sense,
    // but it's currently so prevalent in the code we leave it here.
    // Note the below members are included in AstTypeTable::Key lookups
    // dist-ast-dump-suppress  // Part of dumpSmall
    పూర్ణాంకం m_వెడల్పు = 0;  // (also in AstTypeTable::Key) Bit width of operation
    // (also in AstTypeTable::Key) If unsized, bitwidth of minimum implementation
    // dist-ast-dump-suppress  // Part of dumpSmall
    పూర్ణాంకం m_వెడల్పు_Min = 0;
    // dist-ast-dump-suppress  // Part of dumpSmall
    VSigning m_numeric;  // (also in AstTypeTable::Key) Node is signed
    // dist-ast-dump-suppress  // Part of dumpSmall
    bool m_generic = false;  // Simple globally referenced type, don't garbage collect
    // Unique number assigned to each dtype during creation for IEEE matching
    స్థిర పూర్ణాంకం s_unique_అంకె;

రక్షిత:
    // CONSTRUCTORS
    Ast_కణుపు_DType(VNType t, File_పంక్తి* fl)
        : Ast_కణుపు{t, fl} {}

అంతరంగ:
    // METHODS
    స్థిరరాశి Ast_కణుపు_DType* skipRefIterp(bool skipConst, bool skipEnum,
                                     bool assertOn = true) స్థిరరాశి VL_MT_STABLE;

రక్షిత:
    // METHODS
    మాయా bool similar_DType_కణుపు(స్థిరరాశి Ast_కణుపు_DType* samep) స్థిరరాశి = 0;

బహిరంగ:
    ASTGEN_MEMBERS_Ast_కణుపు_DType;
    // ACCESSORS
    శూన్య dump(std::ostream& str) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str) స్థిరరాశి override;
    మాయా శూన్య dumpSmall(std::ostream& str) స్థిరరాశి VL_MT_STABLE;
    bool hasDType() స్థిరరాశి override VL_MT_సురక్షిత { తిరిగి true; }
    /// Require VlUnpacked, instead of [] for POD elements.
    /// A non-POD object is always compound, but some POD elements
    /// are compound when methods calls operate on object, or when
    /// under another compound-requiring object e.g. class
    మాయా bool isCompound() స్థిరరాశి = 0;
    // Integral or packed, allowed inside an unpacked union/struct
    మాయా bool isIntegralOrPacked() స్థిరరాశి { తిరిగి !isCompound(); }
    // (Slow) recurse down to find basic data type
    మాయా AstBasicDType* basicp() స్థిరరాశి VL_MT_STABLE = 0;
    // (Slow) Recurse over MemberDType|ParamTypeDType|RefDType|ConstDType|EnumDType to other type
    స్థిరరాశి Ast_కణుపు_DType* skipRefp() స్థిరరాశి VL_MT_STABLE { తిరిగి skipRefIterp(true, true); }
    Ast_కణుపు_DType* skipRefp() VL_MT_STABLE {
        తిరిగి const_cast<Ast_కణుపు_DType*>(
            static_cast<స్థిరరాశి Ast_కణుపు_DType*>(this)->skipRefIterp(true, true));
    }
    // (Slow) Recurse over MemberDType|ParamTypeDType|RefDType|ConstDType|EnumDType to other type,
    // Returns null if not resolved
    స్థిరరాశి Ast_కణుపు_DType* skipRefOrNullp() స్థిరరాశి VL_MT_STABLE {
        తిరిగి skipRefIterp(true, true, false);
    }
    Ast_కణుపు_DType* skipRefOrNullp() VL_MT_STABLE {
        తిరిగి const_cast<Ast_కణుపు_DType*>(
            static_cast<స్థిరరాశి Ast_కణుపు_DType*>(this)->skipRefIterp(true, true, false));
    }
    // (Slow) Recurse over MemberDType|ParamTypeDType|RefDType|EnumDType to ConstDType
    స్థిరరాశి Ast_కణుపు_DType* skip_Ref_గమ్యం_Constp() స్థిరరాశి { తిరిగి skipRefIterp(false, true); }
    Ast_కణుపు_DType* skip_Ref_గమ్యం_Constp() {
        తిరిగి const_cast<Ast_కణుపు_DType*>(
            static_cast<స్థిరరాశి Ast_కణుపు_DType*>(this)->skipRefIterp(false, true));
    }
    // (Slow) Recurse over MemberDType|ParamTypeDType|RefDType|ConstDType to EnumDType
    స్థిరరాశి Ast_కణుపు_DType* skip_Ref_గమ్యం_Enump() స్థిరరాశి { తిరిగి skipRefIterp(true, false); }
    Ast_కణుపు_DType* skip_Ref_గమ్యం_Enump() {
        తిరిగి const_cast<Ast_కణుపు_DType*>(
            static_cast<స్థిరరాశి Ast_కణుపు_DType*>(this)->skipRefIterp(true, false));
    }
    // (Slow) Recurse over MemberDType|ParamTypeDType|RefDType|ConstDType to EnumDType,
    // Returns null if not resolved
    స్థిరరాశి Ast_కణుపు_DType* skip_Ref_గమ్యం_Enum_Or_Nullp() స్థిరరాశి { తిరిగి skipRefIterp(true, false, false); }
    // (Slow) Recurse over MemberDType|ParamTypeDType|RefDType to other type
    స్థిరరాశి Ast_కణుపు_DType* skip_Ref_గమ్యం_Non_Refp() స్థిరరాశి { తిరిగి skipRefIterp(false, false); }
    Ast_కణుపు_DType* skip_Ref_గమ్యం_Non_Refp() {
        తిరిగి const_cast<Ast_కణుపు_DType*>(
            static_cast<స్థిరరాశి Ast_కణుపు_DType*>(this)->skipRefIterp(false, false));
    }
    // If array, returns element dtype, otherwise returns skipRef dtype
    // If skipRef is false, RefDTypes are not followed (safe before typedef linking)
    స్థిరరాశి Ast_కణుపు_DType* elemDTypep(bool skipRef = true) స్థిరరాశి VL_MT_STABLE;
    Ast_కణుపు_DType* elemDTypep(bool skipRef = true) VL_MT_STABLE {
        తిరిగి const_cast<Ast_కణుపు_DType*>(
            static_cast<స్థిరరాశి Ast_కణుపు_DType*>(this)->elemDTypep(skipRef));
    }
    // (Slow) recurses - Structure alignment 1,2,4 or 8 bytes (arrays affect this)
    మాయా పూర్ణాంకం వెడల్పు_Align_బైట్లు() స్థిరరాశి = 0;
    // (Slow) recurses - Width in bytes rounding up 1,2,4,8,12,...
    మాయా పూర్ణాంకం వెడల్పు_మొత్తం_బైట్లు() స్థిరరాశి = 0;
    bool maybe_Pointed_గమ్యం() స్థిరరాశి override VL_MT_సురక్షిత { తిరిగి true; }
    // Iff has a non-null refDTypep(), as generic node function
    మాయా Ast_కణుపు_DType* virtRefDTypep() స్థిరరాశి { తిరిగి nullptr; }
    // Iff has refDTypep(), set as generic node function
    మాయా శూన్య virtRefDTypep(Ast_కణుపు_DType* nodep) {}
    // Iff has a non-null second dtypep, as generic node function
    మాయా Ast_కణుపు_DType* virtRefDType2p() స్థిరరాశి { తిరిగి nullptr; }
    // Iff has second dtype, set as generic node function
    మాయా శూన్య virtRefDType2p(Ast_కణుపు_DType* nodep) {}
    // Assignable equivalence.  Calls skipRefToNonRefp() during comparisons.
    bool similarDType(స్థిరరాశి Ast_కణుపు_DType* samep) స్థిరరాశి;
    // Iff has a non-null subDTypep(), as generic node function
    మాయా Ast_కణుపు_DType* subDTypep() స్థిరరాశి VL_MT_STABLE { తిరిగి nullptr; }
    మాయా Ast_కణుపు_DType* subDType2p() స్థిరరాశి VL_MT_STABLE { తిరిగి nullptr; }
    మాయా bool isAggregateType() స్థిరరాశి { తిరిగి false; }
    // True for unpacked, dynamic, queue, and associative arrays (not packed arrays)
    bool isNonPackedArray() స్థిరరాశి;
    మాయా bool isFourstate() స్థిరరాశి;
    // Ideally an IEEE $typename
    మాయా string prettyDTypeName(bool) స్థిరరాశి { తిరిగి prettyTypeName(); }
    string prettyDTypeNameQ(bool నిండిన_2 = false) స్థిరరాశి { తిరిగి "'" + prettyDTypeName(నిండిన_2) + "'"; }
    string stateDTypeName() స్థిరరాశి { తిరిగి this->isFourstate() ? "(4-state)" : "(2-state)"; }
    //
    // Changing the width may confuse the data type resolution, so must clear
    // TypeTable cache after use.
    శూన్య వెడల్పు_Force(పూర్ణాంకం వెడల్పు, పూర్ణాంకం వెడల్పు_Min) {
        m_వెడల్పు = వెడల్పు;
        m_వెడల్పు_Min = వెడల్పు_Min;
    }
    // For backward compatibility inherit width and signing from the subDType/base type
    శూన్య వెడల్పు_From_Sub(స్థిరరాశి Ast_కణుపు_DType* nodep) {
        m_వెడల్పు = nodep->m_వెడల్పు;
        m_వెడల్పు_Min = nodep->m_వెడల్పు_Min;
        m_numeric = nodep->m_numeric;
    }
    //
    పూర్ణాంకం వెడల్పు() స్థిరరాశి VL_MT_STABLE { తిరిగి m_వెడల్పు; }
    శూన్య numeric(VSigning flag) { m_numeric = flag; }
    bool isSigned() స్థిరరాశి VL_MT_STABLE { తిరిగి m_numeric.isSigned(); }
    bool isNosign() స్థిరరాశి VL_MT_సురక్షిత { తిరిగి m_numeric.isNosign(); }
    VSigning numeric() స్థిరరాశి VL_MT_STABLE { తిరిగి m_numeric; }
    పూర్ణాంకం వెడల్పు_పదాలు() స్థిరరాశి VL_MT_STABLE { తిరిగి VL_పదాలు_I(వెడల్పు()); }
    పూర్ణాంకం వెడల్పు_Min() స్థిరరాశి VL_MT_STABLE {  // If sized, the size,
                                         // if unsized the min digits to represent it
        తిరిగి m_వెడల్పు_Min ? m_వెడల్పు_Min : m_వెడల్పు;
    }
    పూర్ణాంకం వెడల్పు_Pow2() స్థిరరాశి;
    శూన్య వెడల్పు_Min_From_వెడల్పు() { m_వెడల్పు_Min = m_వెడల్పు; }
    bool వెడల్పు_Sized() స్థిరరాశి VL_MT_సురక్షిత { తిరిగి !m_వెడల్పు_Min || m_వెడల్పు_Min == m_వెడల్పు; }
    bool generic() స్థిరరాశి VL_MT_సురక్షిత { తిరిగి m_generic; }
    శూన్య generic(bool flag) { m_generic = flag; }
    std::pair<uint32_t, uint32_t> dimensions(bool includeBasic) స్థిరరాశి;
    uint32_t arrayUnpackedElements() స్థిరరాశి;  // 1, or total multiplication of all dimensions
    // Fixed aggregate streaming properties
    bool isStreamableFixedAggregate() స్థిరరాశి;
    // Whether is a string, or an unpacked array or structure containing one
    bool కలిగియుంది_String() స్థిరరాశి;
    bool కలిగియుంది_Unpacked_Struct() స్థిరరాశి;
    పూర్ణాంకం వెడల్పు_ప్రవాహం() స్థిరరాశి;
    string vlEnumType() స్థిరరాశి;  // Return VerilatedVarType: VLVT_UINT32, etc
    string vlEnumPackedAgg() స్థిరరాశి;  // Return VLVF_PACKED_AGG etc flags, or empty
    స్థిర పూర్ణాంకం unique_అంకె_Inc() { తిరిగి ++s_unique_అంకె; }
    స్థిరరాశి అక్షరం* charIQWN() స్థిరరాశి {
        తిరిగి (isString() ? "N" : isWide() ? "W" : isDouble() ? "D" : isQuad() ? "Q" : "I");
    }
    string cType(స్థిరరాశి string& name, bool forFunc, bool isRef, bool packed = false) స్థిరరాశి;
    // Represents a C++ LiteralType? (can be constexpr)
    bool isLiteralType() స్థిరరాశి VL_MT_STABLE;
    // C++ type (as emitted by cType) is trivially default constructible and destructible?
    bool isTriviallyConstructible() స్థిరరాశి VL_MT_STABLE;
    మాయా bool isDynamicallySized() స్థిరరాశి { తిరిగి false; }

అంతరంగ:
    class CTypeRecursed;
    CTypeRecursed cTypeRecurse(bool compound, bool packed) స్థిరరాశి;
};
class Ast_కణుపు_Array_DType VL_NOT_FINAL : బహిరంగ Ast_కణుపు_DType {
    // Array data type, ie "some_dtype var_name [2:0]"
    // @astgen op1 := childDTypep : Optional[AstNodeDType] // moved to refDTypep() in V3Width
    // @astgen op2 := rangep : Optional[AstRange] // array bounds
    //
    // @astgen ptr := m_refDTypep : Optional[AstNodeDType]  // Elements of this type (post-width)
    Ast_కణుపు* rangenp() స్థిరరాశి { తిరిగి reinterpret_cast<Ast_కణుపు*>(rangep()); }

రక్షిత:
    Ast_కణుపు_Array_DType(VNType t, File_పంక్తి* fl)
        : Ast_కణుపు_DType{t, fl} {}

బహిరంగ:
    ASTGEN_MEMBERS_Ast_కణుపు_Array_DType;
    శూన్య dump(std::ostream& str) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str) స్థిరరాశి override;
    శూన్య dumpSmall(std::ostream& str) స్థిరరాశి override;
    స్థిరరాశి అక్షరం* broken() స్థిరరాశి override {
        BROKEN_RTN(!((m_refDTypep && !బిడ్డ_DTypep()) || (!m_refDTypep && బిడ్డ_DTypep())));
        తిరిగి nullptr;
    }
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* samep) స్థిరరాశి override {
        స్థిరరాశి Ast_కణుపు_Array_DType* స్థిరరాశి asamep = VN_DBG_AS(samep, కణుపు_Array_DType);
        తిరిగి hi() == asamep->hi() && rangenp()->same_వృక్షం(asamep->rangenp())
               && subDTypep() == asamep->subDTypep();
    }  // HashedDT doesn't recurse, so need to check children
    bool similar_DType_కణుపు(స్థిరరాశి Ast_కణుపు_DType* samep) స్థిరరాశి override {
        స్థిరరాశి Ast_కణుపు_Array_DType* స్థిరరాశి asamep = VN_DBG_AS(samep, కణుపు_Array_DType);
        తిరిగి elementsConst() == asamep->elementsConst()
               && subDTypep()->similarDType(asamep->subDTypep());
    }
    Ast_కణుపు_DType* get_బిడ్డ_DTypep() స్థిరరాశి override { తిరిగి బిడ్డ_DTypep(); }
    Ast_కణుపు_DType* subDTypep() స్థిరరాశి override VL_MT_STABLE {
        తిరిగి m_refDTypep ? m_refDTypep : బిడ్డ_DTypep();
    }
    శూన్య refDTypep(Ast_కణుపు_DType* nodep) { m_refDTypep = nodep; }
    Ast_కణుపు_DType* virtRefDTypep() స్థిరరాశి override { తిరిగి m_refDTypep; }
    శూన్య virtRefDTypep(Ast_కణుపు_DType* nodep) override { refDTypep(nodep); }
    // METHODS
    AstBasicDType* basicp() స్థిరరాశి override VL_MT_STABLE {
        తిరిగి subDTypep()->basicp();
    }  // (Slow) recurse down to find basic data type
    పూర్ణాంకం వెడల్పు_Align_బైట్లు() స్థిరరాశి override { తిరిగి subDTypep()->వెడల్పు_Align_బైట్లు(); }
    పూర్ణాంకం వెడల్పు_మొత్తం_బైట్లు() స్థిరరాశి override {
        తిరిగి elementsConst() * subDTypep()->వెడల్పు_మొత్తం_బైట్లు();
    }
    అంతఃవరుస పూర్ణాంకం ఎడమ_2() స్థిరరాశి VL_MT_STABLE;
    అంతఃవరుస పూర్ణాంకం కుడి_2() స్థిరరాశి VL_MT_STABLE;
    అంతఃవరుస పూర్ణాంకం hi() స్థిరరాశి VL_MT_STABLE;
    అంతఃవరుస పూర్ణాంకం lo() స్థిరరాశి VL_MT_STABLE;
    అంతఃవరుస పూర్ణాంకం elementsConst() స్థిరరాశి VL_MT_STABLE;
    అంతఃవరుస VNumRange declRange() స్థిరరాశి VL_MT_STABLE;
};
class Ast_కణుపు_UOr_Struct_DType VL_NOT_FINAL : బహిరంగ Ast_కణుపు_DType {
    // A struct or union; common handling
    // @astgen op1 := membersp : List[AstMemberDType]
    //
    // @astgen ptr := m_classOrPackagep : Optional[AstNodeModule]  // Package emitted with
    string m_name;  // Name from upper typedef, if any
    string m_typedefName;  // Typedef's dtypeName() once moved off the typedef declaring it
    స్థిరరాశి పూర్ణాంకం m_unique_అంకె;  // Unique ID distinguishing this dtype instance, for hashing/naming
    bool m_packed;  // Packed struct/union, else unpacked
    bool m_isFourstate = false;  // V3Width computes; true if any member is 4-state
    bool m_constrainedRand = false;  // True if struct has constraint expression
    bool m_emit_గమ్యం_String = false;  // Generate to_string() for this struct/union if set

రక్షిత:
    Ast_కణుపు_UOr_Struct_DType(VNType t, File_పంక్తి* fl, VSigning numericUnpack)
        : Ast_కణుపు_DType{t, fl}
        , m_unique_అంకె{unique_అంకె_Inc()} {
        // VSigning::NOSIGN overloaded to indicate not packed
        m_packed = (numericUnpack != VSigning::NOSIGN);
        numeric(VSigning::fromBool(numericUnpack.isSigned()));
    }
    Ast_కణుపు_UOr_Struct_DType(స్థిరరాశి Ast_కణుపు_UOr_Struct_DType& other)
        : Ast_కణుపు_DType{other}
        , m_name{other.m_name}
        , m_typedefName{other.m_typedefName}
        , m_unique_అంకె{unique_అంకె_Inc()}
        , m_packed{other.m_packed}
        , m_isFourstate{other.m_isFourstate} {}

బహిరంగ:
    ASTGEN_MEMBERS_Ast_కణుపు_UOr_Struct_DType;
    పూర్ణాంకం unique_అంకె() స్థిరరాశి { తిరిగి m_unique_అంకె; }
    శూన్య dump(std::ostream& str) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str) స్థిరరాశి override;
    string prettyDTypeName(bool) స్థిరరాశి override;
    bool isCompound() స్థిరరాశి override { తిరిగి !packed(); }
    // For basicp() we reuse the size to indicate a "fake" basic type of same size
    AstBasicDType* basicp() స్థిరరాశి override VL_MT_STABLE {
        ఒకవేళ (!m_packed) తిరిగి nullptr;
        తిరిగి (isFourstate()
                    ? VN_AS(కనుగొను_Logic_Range_DType(VNumRange{వెడల్పు() - 1, 0}, వెడల్పు(), numeric()),
                            BasicDType)
                    : VN_AS(కనుగొను_బిట్టు_Range_DType(VNumRange{వెడల్పు() - 1, 0}, వెడల్పు(), numeric()),
                            BasicDType));
    }
    // (Slow) recurses - Structure alignment 1,2,4 or 8 bytes (arrays affect this)
    పూర్ణాంకం వెడల్పు_Align_బైట్లు() స్థిరరాశి override;
    // (Slow) recurses - Width in bytes rounding up 1,2,4,8,12,...
    పూర్ణాంకం వెడల్పు_మొత్తం_బైట్లు() స్థిరరాశి override;
    bool similar_DType_కణుపు(స్థిరరాశి Ast_కణుపు_DType* samep) స్థిరరాశి override;
    string name() స్థిరరాశి override VL_MT_STABLE { తిరిగి m_name; }
    శూన్య name(స్థిరరాశి string& flag) override { m_name = flag; }
    // Keep the name the typedef declaring it gives, as it moves off it to the type table
    శూన్య typedefName(స్థిరరాశి string& name) { m_typedefName = name; }
    // Name of the C structure for DPI, empty if no typedef names it, so declared where used
    string dpiName() స్థిరరాశి;
    // C typedef of the unpacked structure for DPI (IEEE 1800-2023 H.7.3)
    string dpiTypedef() స్థిరరాశి;
    bool packed() స్థిరరాశి VL_MT_సురక్షిత { తిరిగి m_packed; }
    శూన్య packed(bool flag) { m_packed = flag; }
    // packed() but as don't support unpacked, presently all structs
    స్థిర bool packedUnsup() { తిరిగి true; }
    శూన్య isFourstate(bool flag) { m_isFourstate = flag; }
    bool isFourstate() స్థిరరాశి override VL_MT_సురక్షిత { తిరిగి m_isFourstate; }
    స్థిర పూర్ణాంకం lo() VL_MT_STABLE { తిరిగి 0; }
    పూర్ణాంకం hi() స్థిరరాశి VL_MT_STABLE {
        తిరిగి dtypep()->వెడల్పు() - 1;
    }  // Packed classes look like arrays
    VNumRange declRange() స్థిరరాశి VL_MT_STABLE { తిరిగి VNumRange{hi(), lo()}; }
    Ast_కణుపు_Module* classOrPackagep() స్థిరరాశి { తిరిగి m_classOrPackagep; }
    శూన్య classOrPackagep(Ast_కణుపు_Module* classpackagep) { m_classOrPackagep = classpackagep; }
    bool isConstrainedRand() స్థిరరాశి { తిరిగి m_constrainedRand; }
    శూన్య markConstrainedRand(bool flag) { m_constrainedRand = flag; }
    bool emit_గమ్యం_String() స్థిరరాశి { తిరిగి m_emit_గమ్యం_String; }
    శూన్య సమితి_Emit_గమ్యం_String() { m_emit_గమ్యం_String = true; }
    bool isAggregateType() స్థిరరాశి override { తిరిగి !packed(); }
};

// === Concrete node types =====================================================

// === AstNode ===
class Ast_Enum_అంశం final : బహిరంగ Ast_కణుపు {
    // @astgen op1 := rangep : Optional[AstRange] // Range for name appending
    // @astgen op2 := valuep : Optional[AstNodeExpr]
    string m_name;  // Name of item

బహిరంగ:
    // Parents: ENUM
    Ast_Enum_అంశం(File_పంక్తి* fl, స్థిరరాశి string& name, AstRange* rangep, Ast_కణుపు_సమాసం* valuep)
        : ASTGEN_SUPER_Enum_అంశం(fl)
        , m_name{name} {
        this->rangep(rangep);
        this->valuep(valuep);
    }
    ASTGEN_MEMBERS_Ast_Enum_అంశం;
    string name() స్థిరరాశి override VL_MT_STABLE { తిరిగి m_name; }
    bool maybe_Pointed_గమ్యం() స్థిరరాశి override VL_MT_సురక్షిత { తిరిగి true; }
    bool hasDType() స్థిరరాశి override VL_MT_సురక్షిత { తిరిగి true; }
    శూన్య name(స్థిరరాశి string& flag) override { m_name = flag; }
};

// === AstNodeDType ===
class AstAssocArrayDType final : బహిరంగ Ast_కణుపు_DType {
    // Associative array data type, ie "[some_dtype]"
    // @astgen op1 := childDTypep : Optional[AstNodeDType] // moved to refDTypep() in V3Width
    // @astgen op2 := keyChildDTypep : Optional[AstNodeDType]
    //
    // @astgen ptr := m_refDTypep : Optional[AstNodeDType]  // Elements of this type (post-width)
    // @astgen ptr := m_keyDTypep : Optional[AstNodeDType]  // Keys of this type (post-width)
బహిరంగ:
    AstAssocArrayDType(File_పంక్తి* fl, VFlag_బిడ్డ_DType, Ast_కణుపు_DType* dtp, Ast_కణుపు_DType* కీలకం_Dtp)
        : ASTGEN_SUPER_AssocArrayDType(fl) {
        బిడ్డ_DTypep(dtp);  // Only for parser
        కీలకం_బిడ్డ_DTypep(కీలకం_Dtp);  // Only for parser
        refDTypep(nullptr);
        కీలకం_DTypep(nullptr);
        dtypep(nullptr);  // V3Width will resolve
    }
    AstAssocArrayDType(File_పంక్తి* fl, Ast_కణుపు_DType* dtp, Ast_కణుపు_DType* కీలకం_Dtp)
        : ASTGEN_SUPER_AssocArrayDType(fl) {
        refDTypep(dtp);
        కీలకం_DTypep(కీలకం_Dtp);
        dtypep(dtp);
    }
    ASTGEN_MEMBERS_AstAssocArrayDType;
    స్థిరరాశి అక్షరం* broken() స్థిరరాశి override {
        BROKEN_RTN(!((m_refDTypep && !బిడ్డ_DTypep()) || (!m_refDTypep && బిడ్డ_DTypep())));
        BROKEN_RTN(!((m_కీలకం_DTypep && !బిడ్డ_DTypep()) || (!m_కీలకం_DTypep && బిడ్డ_DTypep())));
        తిరిగి nullptr;
    }
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* samep) స్థిరరాశి override {
        స్థిరరాశి AstAssocArrayDType* స్థిరరాశి asamep = VN_DBG_AS(samep, AssocArrayDType);
        ఒకవేళ (!asamep->subDTypep()) తిరిగి false;
        ఒకవేళ (!asamep->కీలకం_DTypep()) తిరిగి false;
        తిరిగి (subDTypep() == asamep->subDTypep() && కీలకం_DTypep() == asamep->కీలకం_DTypep());
    }
    bool similar_DType_కణుపు(స్థిరరాశి Ast_కణుపు_DType* samep) స్థిరరాశి override {
        స్థిరరాశి AstAssocArrayDType* స్థిరరాశి asamep = VN_DBG_AS(samep, AssocArrayDType);
        తిరిగి asamep->subDTypep() && subDTypep()->similarDType(asamep->subDTypep())
               && asamep->కీలకం_DTypep() && కీలకం_DTypep()->similarDType(asamep->కీలకం_DTypep());
    }
    string prettyDTypeName(bool నిండిన_2) స్థిరరాశి override;
    శూన్య dumpSmall(std::ostream& str) స్థిరరాశి override;
    Ast_కణుపు_DType* get_బిడ్డ_DTypep() స్థిరరాశి override { తిరిగి బిడ్డ_DTypep(); }
    Ast_కణుపు_DType* getChild2DTypep() స్థిరరాశి override { తిరిగి కీలకం_బిడ్డ_DTypep(); }
    Ast_కణుపు_DType* subDTypep() స్థిరరాశి override VL_MT_STABLE {
        తిరిగి m_refDTypep ? m_refDTypep : బిడ్డ_DTypep();
    }
    Ast_కణుపు_DType* subDType2p() స్థిరరాశి override VL_MT_STABLE {
        తిరిగి m_కీలకం_DTypep ? m_కీలకం_DTypep : కీలకం_బిడ్డ_DTypep();
    }
    bool isAggregateType() స్థిరరాశి override { తిరిగి true; }
    శూన్య refDTypep(Ast_కణుపు_DType* nodep) { m_refDTypep = nodep; }
    Ast_కణుపు_DType* virtRefDTypep() స్థిరరాశి override { తిరిగి m_refDTypep; }
    శూన్య virtRefDTypep(Ast_కణుపు_DType* nodep) override { refDTypep(nodep); }
    Ast_కణుపు_DType* virtRefDType2p() స్థిరరాశి override { తిరిగి m_కీలకం_DTypep; }
    శూన్య virtRefDType2p(Ast_కణుపు_DType* nodep) override { కీలకం_DTypep(nodep); }
    //
    Ast_కణుపు_DType* కీలకం_DTypep() స్థిరరాశి VL_MT_STABLE {
        తిరిగి m_కీలకం_DTypep ? m_కీలకం_DTypep : కీలకం_బిడ్డ_DTypep();
    }
    శూన్య కీలకం_DTypep(Ast_కణుపు_DType* nodep) { m_కీలకం_DTypep = nodep; }
    // METHODS
    AstBasicDType* basicp() స్థిరరాశి override VL_MT_STABLE { తిరిగి nullptr; }
    పూర్ణాంకం వెడల్పు_Align_బైట్లు() స్థిరరాశి override { తిరిగి subDTypep()->వెడల్పు_Align_బైట్లు(); }
    పూర్ణాంకం వెడల్పు_మొత్తం_బైట్లు() స్థిరరాశి override { తిరిగి subDTypep()->వెడల్పు_మొత్తం_బైట్లు(); }
    bool isCompound() స్థిరరాశి override { తిరిగి true; }
    bool isDynamicallySized() స్థిరరాశి override { తిరిగి true; }
};
class AstBasicDType final : బహిరంగ Ast_కణుపు_DType {
    // Builtin atomic/vectored data type
    // @astgen op1 := rangep : Optional[AstRange] // Range of variable
    నిర్మాణం Members final {
        VBasicDTypeKwd m_keyword;  // (also in VBasicTypeKey) What keyword created basic type
        // dist-ast-dump-suppress  // Part of dumpSmall
        VNumRange m_nrange;  // (also in VBasicTypeKey) Numeric msb/lsb (if non-opaque keyword)
        bool క్రియాకారకం==(స్థిరరాశి Members& rhs) స్థిరరాశి {
            తిరిగి rhs.m_keyword == m_keyword && rhs.m_nrange == m_nrange;
        }
    } m;
    // See also in AstNodeDType: m_width, m_widthMin, m_numeric(issigned)
బహిరంగ:
    AstBasicDType(File_పంక్తి* fl, VBasicDTypeKwd kwd, స్థిరరాశి VSigning& signst = VSigning::NOSIGN)
        : ASTGEN_SUPER_BasicDType(fl) {
        init(kwd, signst, 0, -1, nullptr);
    }
    AstBasicDType(File_పంక్తి* fl, VFlagLogicPacked, పూర్ణాంకం wantwidth)
        : ASTGEN_SUPER_BasicDType(fl) {
        init(VBasicDTypeKwd::LOGIC, VSigning::NOSIGN, wantwidth, -1, nullptr);
    }
    AstBasicDType(File_పంక్తి* fl, VFlagLogic2StatePacked, పూర్ణాంకం wantwidth)
        : ASTGEN_SUPER_BasicDType(fl) {
        init(VBasicDTypeKwd::LOGIC2STATE, VSigning::NOSIGN, wantwidth, -1, nullptr);
    }
    AstBasicDType(File_పంక్తి* fl, VFlag_బిట్టు_Packed, పూర్ణాంకం wantwidth)
        : ASTGEN_SUPER_BasicDType(fl) {
        init(VBasicDTypeKwd::బిట్టు, VSigning::NOSIGN, wantwidth, -1, nullptr);
    }
    AstBasicDType(File_పంక్తి* fl, VBasicDTypeKwd kwd, VSigning numer, పూర్ణాంకం wantwidth, పూర్ణాంకం widthmin)
        : ASTGEN_SUPER_BasicDType(fl) {
        init(kwd, numer, wantwidth, widthmin, nullptr);
    }
    AstBasicDType(File_పంక్తి* fl, VBasicDTypeKwd kwd, VSigning numer, VNumRange range, పూర్ణాంకం widthmin)
        : ASTGEN_SUPER_BasicDType(fl) {
        init(kwd, numer, range.elements(), widthmin, nullptr);
        m.m_nrange = range;  // as init() presumes lsb==0, but range.lsb() might not be
    }
    // See also addRange in verilog.y
అంతరంగ:
    శూన్య init(VBasicDTypeKwd kwd, VSigning numer, పూర్ణాంకం wantwidth, పూర్ణాంకం wantwidthmin,
              AstRange* rangep);

బహిరంగ:
    ASTGEN_MEMBERS_AstBasicDType;
    శూన్య dump(std::ostream& str) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str) స్థిరరాశి override;
    // width/widthMin/numeric compared elsewhere
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* samep) స్థిరరాశి override;
    bool similar_DType_కణుపు(స్థిరరాశి Ast_కణుపు_DType* samep) స్థిరరాశి override;
    string name() స్థిరరాశి override VL_MT_STABLE { తిరిగి m.m_keyword.typeName(); }
    string prettyDTypeName(bool నిండిన_2) స్థిరరాశి override;
    స్థిరరాశి అక్షరం* broken() స్థిరరాశి override {
        BROKEN_RTN(dtypep() != this);
        BROKEN_RTN(v3Global.వెడల్పు_Min_Usage() == VWidthMinUsage::VERILOG_వెడల్పు
                   && వెడల్పు_Min() > వెడల్పు());
        తిరిగి nullptr;
    }
    శూన్య సమితి_Signed_State(స్థిరరాశి VSigning& signst) {
        // Note NOSIGN does NOT change the state; this is required by the parser
        ఒకవేళ (signst == VSigning::UNSIGNED) {
            numeric(signst);
        } లేకపోతే ఒకవేళ (signst == VSigning::SIGNED) {
            numeric(signst);
        }
    }
    // METHODS
    AstBasicDType* basicp() స్థిరరాశి override VL_MT_STABLE { తిరిగి (AstBasicDType*)this; }
    // (Slow) recurses - Structure alignment 1,2,4 or 8 bytes (arrays affect this)
    పూర్ణాంకం వెడల్పు_Align_బైట్లు() స్థిరరాశి override;
    // (Slow) recurses - Width in bytes rounding up 1,2,4,8,12,...
    పూర్ణాంకం వెడల్పు_మొత్తం_బైట్లు() స్థిరరాశి override;
    bool isFourstate() స్థిరరాశి override { తిరిగి keyword().isFourstate(); }
    VBasicDTypeKwd keyword() స్థిరరాశి VL_MT_సురక్షిత {  // Avoid using - use isSomething accessors instead
        తిరిగి m.m_keyword;
    }
    bool is_బిట్టు_Logic() స్థిరరాశి { తిరిగి keyword().is_బిట్టు_Logic(); }
    bool is_బిట్టు() స్థిరరాశి { తిరిగి keyword().is_బిట్టు(); }
    bool isCHandle() స్థిరరాశి VL_MT_STABLE { తిరిగి keyword().isCHandle(); }
    bool isDouble() స్థిరరాశి VL_MT_STABLE { తిరిగి keyword().isDouble(); }
    bool isEvent() స్థిరరాశి VL_MT_STABLE { తిరిగి keyword() == VBasicDTypeKwd::EVENT; }
    bool isForkSync() స్థిరరాశి VL_MT_సురక్షిత { తిరిగి keyword() == VBasicDTypeKwd::FORK_SYNC; }
    bool isProcessRef() స్థిరరాశి VL_MT_సురక్షిత { తిరిగి keyword() == VBasicDTypeKwd::PROCESS_REFERENCE; }
    bool isDelayScheduler() స్థిరరాశి VL_MT_సురక్షిత {
        తిరిగి keyword() == VBasicDTypeKwd::DELAY_SCHEDULER;
    }
    bool isTriggerScheduler() స్థిరరాశి VL_MT_సురక్షిత {
        తిరిగి keyword() == VBasicDTypeKwd::TRIGGER_SCHEDULER;
    }
    bool isDynamicTriggerScheduler() స్థిరరాశి VL_MT_సురక్షిత {
        తిరిగి keyword() == VBasicDTypeKwd::DYNAMIC_TRIGGER_SCHEDULER;
    }
    bool isRandomGenerator() స్థిరరాశి VL_MT_సురక్షిత {
        తిరిగి keyword() == VBasicDTypeKwd::RANDOM_GENERATOR;
    }
    bool isStdRandomGenerator() స్థిరరాశి VL_MT_సురక్షిత {
        తిరిగి keyword() == VBasicDTypeKwd::RANDOM_STDGENERATOR;
    }
    bool isCovergroupInstHandle() స్థిరరాశి VL_MT_సురక్షిత {
        తిరిగి keyword() == VBasicDTypeKwd::COVERGROUP_INSTHANDLE;
    }
    bool isOpaque() స్థిరరాశి VL_MT_సురక్షిత { తిరిగి keyword().isOpaque(); }
    bool isString() స్థిరరాశి VL_MT_STABLE { తిరిగి keyword().isString(); }
    bool isZeroInit() స్థిరరాశి { తిరిగి keyword().isZeroInit(); }
    bool isRanged() స్థిరరాశి { తిరిగి rangep() || m.m_nrange.ranged(); }
    bool is_Dpi_బిట్టు_Vec() స్థిరరాశి {  // DPI uses svBitVecVal
        తిరిగి keyword() == VBasicDTypeKwd::బిట్టు && isRanged();
    }
    bool isDpiLogicVec() స్థిరరాశి {  // DPI uses svLogicVecVal
        తిరిగి keyword().isFourstate() && !(keyword() == VBasicDTypeKwd::LOGIC && !isRanged());
    }
    bool isDpiCLayout() స్థిరరాశి {
        తిరిగి keyword().isDpiCLayout()
               || (keyword().isSameish(VBasicDTypeKwd::LOGIC) && !isRanged());
    }
    bool isDpiPrimitive() స్థిరరాశి {  // DPI uses a primitive type
        తిరిగి !is_Dpi_బిట్టు_Vec() && !isDpiLogicVec();
    }
    // Generally the lo/hi/left/right funcs should be used instead of nrange()
    స్థిరరాశి VNumRange& nrange() స్థిరరాశి { తిరిగి m.m_nrange; }
    అంతఃవరుస పూర్ణాంకం hi() స్థిరరాశి;
    అంతఃవరుస పూర్ణాంకం lo() స్థిరరాశి;
    అంతఃవరుస పూర్ణాంకం elements() స్థిరరాశి;
    పూర్ణాంకం ఎడమ_2() స్థిరరాశి { తిరిగి ascending() ? lo() : hi(); }  // How to show a declaration
    పూర్ణాంకం కుడి_2() స్థిరరాశి { తిరిగి ascending() ? hi() : lo(); }
    అంతఃవరుస bool ascending() స్థిరరాశి;
    bool implicit() స్థిరరాశి { తిరిగి keyword() == VBasicDTypeKwd::LOGIC_IMPLICIT; }
    bool untyped() స్థిరరాశి { తిరిగి keyword() == VBasicDTypeKwd::UNTYPED; }
    VNumRange declRange() స్థిరరాశి { తిరిగి isRanged() ? VNumRange{ఎడమ_2(), కుడి_2()} : VNumRange{}; }
    శూన్య cvtRangeConst();  // Convert to smaller representation
    bool isCompound() స్థిరరాశి override { తిరిగి isString(); }
    bool isIntegralOrPacked() స్థిరరాశి override { తిరిగి keyword().isIntNumeric(); }
};
class AstBracketArrayDType final : బహిరంగ Ast_కణుపు_DType {
    // Associative/Queue/Normal array data type, ie "[dtype_or_expr]"
    // only for early parsing then becomes another data type
    // @astgen op1 := childDTypep : Optional[AstNodeDType] // moved to refDTypep() in V3Width
    // @astgen op2 := elementsp : AstNode  // Number of elements in array
బహిరంగ:
    AstBracketArrayDType(File_పంక్తి* fl, VFlag_బిడ్డ_DType, Ast_కణుపు_DType* బిడ్డ_DTypep,
                         Ast_కణుపు* elementsp)
        : ASTGEN_SUPER_BracketArrayDType(fl) {
        this->బిడ్డ_DTypep(బిడ్డ_DTypep);
        this->elementsp(elementsp);
    }
    ASTGEN_MEMBERS_AstBracketArrayDType;
    bool similar_DType_కణుపు(స్థిరరాశి Ast_కణుపు_DType* samep) స్థిరరాశి override { తిరిగి this == samep; }
    Ast_కణుపు_DType* subDTypep() స్థిరరాశి override VL_MT_STABLE { తిరిగి బిడ్డ_DTypep(); }
    // METHODS
    // Will be removed in V3Width, which relies on this
    // being a child not a dtype pointed node
    bool maybe_Pointed_గమ్యం() స్థిరరాశి override VL_MT_సురక్షిత { తిరిగి false; }
    AstBasicDType* basicp() స్థిరరాశి override VL_MT_STABLE { తిరిగి nullptr; }
    పూర్ణాంకం వెడల్పు_Align_బైట్లు() స్థిరరాశి override { V3ERROR_NA_RETURN(0); }
    పూర్ణాంకం వెడల్పు_మొత్తం_బైట్లు() స్థిరరాశి override { V3ERROR_NA_RETURN(0); }
    bool isCompound() స్థిరరాశి override { తిరిగి true; }
};
class AstCDType final : బహిరంగ Ast_కణుపు_DType {
    // Raw "C" data type passed directly to output
    string m_name;  // Name of data type, printed when do V3EmitC
బహిరంగ:
    AstCDType(File_పంక్తి* fl, స్థిరరాశి string& name)
        : ASTGEN_SUPER_CDType(fl)
        , m_name{name} {
        dtypep(this);
    }

    ASTGEN_MEMBERS_AstCDType;
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* samep) స్థిరరాశి override {
        స్థిరరాశి AstCDType* స్థిరరాశి asamep = VN_DBG_AS(samep, CDType);
        తిరిగి m_name == asamep->m_name;
    }
    bool similar_DType_కణుపు(స్థిరరాశి Ast_కణుపు_DType* samep) స్థిరరాశి override { తిరిగి same_కణుపు(samep); }
    string name() స్థిరరాశి override VL_MT_STABLE { తిరిగి m_name; }
    string prettyDTypeName(bool) స్థిరరాశి override { తిరిగి m_name; }
    // METHODS
    AstBasicDType* basicp() స్థిరరాశి override VL_MT_STABLE { తిరిగి nullptr; }
    పూర్ణాంకం వెడల్పు_Align_బైట్లు() స్థిరరాశి override { తిరిగి 8; }  // Assume
    పూర్ణాంకం వెడల్పు_మొత్తం_బైట్లు() స్థిరరాశి override { తిరిగి 8; }  // Assume
    bool isCompound() స్థిరరాశి override { తిరిగి true; }
    // Return the global runtime type storing the requested number of bits.
    స్థిర string type_గమ్యం_Hold(పూర్ణాంకం వెడల్పు) {
        ఒకవేళ (వెడల్పు <= 8) తిరిగి "::CData";
        ఒకవేళ (వెడల్పు <= 16) తిరిగి "::SData";
        ఒకవేళ (వెడల్పు <= VL_IDATASIZE) తిరిగి "::IData";
        ఒకవేళ (వెడల్పు <= VL_QUADSIZE) తిరిగి "::QData";

        తిరిగి "::VlWide<" + std::గమ్యం_string(VL_పదాలు_I(వెడల్పు)) + ">";
    }
};
class AstClassRefDType final : బహిరంగ Ast_కణుపు_DType {
    // Reference to a class
    // @astgen op1 := paramsp: List[AstPin]
    //
    // @astgen ptr := m_classp : Optional[AstClass]  // data type pointed to, BELOW the AstTypedef
    // @astgen ptr := m_classOrPackagep : Optional[AstNodeModule]  // Package hierarchy
    bool m_raw_సూచిక = false;  // Emit as a non-owning C++ pointer rather than VlClassRef
బహిరంగ:
    AstClassRefDType(File_పంక్తి* fl, AstClass* classp, AstPin* paramsp)
        : ASTGEN_SUPER_ClassRefDType(fl)
        , m_classp{classp} {
        dtypep(this);
        కూడు_Paramsp(paramsp);
    }
    ASTGEN_MEMBERS_AstClassRefDType;
    // METHODS
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* samep) స్థిరరాశి override {
        స్థిరరాశి AstClassRefDType* స్థిరరాశి asamep = VN_DBG_AS(samep, ClassRefDType);
        తిరిగి (m_classp == asamep->m_classp && m_classOrPackagep == asamep->m_classOrPackagep
                && m_raw_సూచిక == asamep->m_raw_సూచిక);
    }
    bool similar_DType_కణుపు(స్థిరరాశి Ast_కణుపు_DType* samep) స్థిరరాశి override;
    శూన్య dump(std::ostream& str = std::cout) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str = std::cout) స్థిరరాశి override;
    శూన్య dumpSmall(std::ostream& str) స్థిరరాశి override;
    string prettyDTypeName(bool నిండిన_2) స్థిరరాశి override;
    string pretty_Name_సందేశం() స్థిరరాశి override;
    string name() స్థిరరాశి override VL_MT_STABLE;
    AstBasicDType* basicp() స్థిరరాశి override VL_MT_STABLE { తిరిగి nullptr; }
    పూర్ణాంకం వెడల్పు_Align_బైట్లు() స్థిరరాశి override { తిరిగి 0; }
    పూర్ణాంకం వెడల్పు_మొత్తం_బైట్లు() స్థిరరాశి override { తిరిగి 0; }
    Ast_కణుపు_DType* virtRefDTypep() స్థిరరాశి override { తిరిగి nullptr; }
    శూన్య virtRefDTypep(Ast_కణుపు_DType* nodep) override {}
    Ast_కణుపు_DType* subDTypep() స్థిరరాశి override VL_MT_STABLE { తిరిగి nullptr; }
    Ast_కణుపు_Module* classOrPackagep() స్థిరరాశి { తిరిగి m_classOrPackagep; }
    శూన్య classOrPackagep(Ast_కణుపు_Module* nodep) { m_classOrPackagep = nodep; }
    AstClass* classp() స్థిరరాశి VL_MT_STABLE { తిరిగి m_classp; }
    శూన్య classp(AstClass* nodep) { m_classp = nodep; }
    bool raw_సూచిక() స్థిరరాశి { తిరిగి m_raw_సూచిక; }
    శూన్య raw_సూచిక(bool flag) { m_raw_సూచిక = flag; }
    స్థిర శూన్య selfTest();
    bool isCompound() స్థిరరాశి override { తిరిగి true; }
};
class AstConstDType final : బహిరంగ Ast_కణుపు_DType {
    // const data type, ie "const some_dtype var_name [2:0]"
    // ConstDType are removed in V3LinkLValue and become AstVar::isConst.
    // When more generic types are supported AstConstDType will be propagated further.
    // @astgen op1 := childDTypep : Optional[AstNodeDType]
    //
    // @astgen ptr := m_refDTypep : Optional[AstNodeDType]  // Inherit from this base data type
బహిరంగ:
    AstConstDType(File_పంక్తి* fl, VFlag_బిడ్డ_DType, Ast_కణుపు_DType* dtp)
        : ASTGEN_SUPER_ConstDType(fl) {
        బిడ్డ_DTypep(dtp);  // Only for parser
        refDTypep(nullptr);  // V3Width will resolve
        dtypep(nullptr);  // V3Width will resolve
        వెడల్పు_From_Sub(subDTypep());
    }
    ASTGEN_MEMBERS_AstConstDType;
    స్థిరరాశి అక్షరం* broken() స్థిరరాశి override {
        BROKEN_RTN(!((m_refDTypep && !బిడ్డ_DTypep()) || (!m_refDTypep && బిడ్డ_DTypep())));
        తిరిగి nullptr;
    }
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* samep) స్థిరరాశి override {
        స్థిరరాశి AstConstDType* స్థిరరాశి sp = VN_DBG_AS(samep, ConstDType);
        తిరిగి (m_refDTypep == sp->m_refDTypep);
    }
    bool similar_DType_కణుపు(స్థిరరాశి Ast_కణుపు_DType* samep) స్థిరరాశి override {
        తిరిగి skipRefp()->similarDType(samep->skipRefp());
    }
    Ast_కణుపు_DType* get_బిడ్డ_DTypep() స్థిరరాశి override { తిరిగి బిడ్డ_DTypep(); }
    Ast_కణుపు_DType* subDTypep() స్థిరరాశి override VL_MT_STABLE {
        తిరిగి m_refDTypep ? m_refDTypep : బిడ్డ_DTypep();
    }
    శూన్య refDTypep(Ast_కణుపు_DType* nodep) { m_refDTypep = nodep; }
    Ast_కణుపు_DType* virtRefDTypep() స్థిరరాశి override { తిరిగి m_refDTypep; }
    శూన్య virtRefDTypep(Ast_కణుపు_DType* nodep) override { refDTypep(nodep); }
    // METHODS
    AstBasicDType* basicp() స్థిరరాశి override VL_MT_STABLE { తిరిగి subDTypep()->basicp(); }
    పూర్ణాంకం వెడల్పు_Align_బైట్లు() స్థిరరాశి override { తిరిగి subDTypep()->వెడల్పు_Align_బైట్లు(); }
    పూర్ణాంకం వెడల్పు_మొత్తం_బైట్లు() స్థిరరాశి override { తిరిగి subDTypep()->వెడల్పు_మొత్తం_బైట్లు(); }
    bool isCompound() స్థిరరాశి override {
        v3fatalSrc("call isCompound on subdata type, not reference");
        తిరిగి false;
    }
};
class AstConstraintRefDType final : బహిరంగ Ast_కణుపు_DType {
    // For e.g. a reference to constraint for constraint_mode
బహిరంగ:
    explicit AstConstraintRefDType(File_పంక్తి* fl)
        : ASTGEN_SUPER_ConstraintRefDType(fl) {
        dtypep(this);
    }
    ASTGEN_MEMBERS_AstConstraintRefDType;
    bool hasDType() స్థిరరాశి override VL_MT_సురక్షిత { తిరిగి true; }
    bool maybe_Pointed_గమ్యం() స్థిరరాశి override VL_MT_సురక్షిత { తిరిగి true; }
    bool undead() స్థిరరాశి override { తిరిగి true; }
    Ast_కణుపు_DType* subDTypep() స్థిరరాశి override VL_MT_STABLE { తిరిగి nullptr; }
    Ast_కణుపు_DType* virtRefDTypep() స్థిరరాశి override { తిరిగి nullptr; }
    శూన్య virtRefDTypep(Ast_కణుపు_DType* nodep) override {}
    bool similar_DType_కణుపు(స్థిరరాశి Ast_కణుపు_DType* samep) స్థిరరాశి override { తిరిగి this == samep; }
    AstBasicDType* basicp() స్థిరరాశి override VL_MT_STABLE { తిరిగి nullptr; }
    పూర్ణాంకం వెడల్పు_Align_బైట్లు() స్థిరరాశి override { తిరిగి 1; }
    పూర్ణాంకం వెడల్పు_మొత్తం_బైట్లు() స్థిరరాశి override { తిరిగి 1; }
    bool isCompound() స్థిరరాశి override { తిరిగి false; }
};
class AstCoverCrossDType final : బహిరంగ Ast_కణుపు_DType {
    // Borrowed pointer to VlCoverCrossT<...> or construction-time VlCoverCrossDyn.
    స్థిరరాశి uint32_t m_dimensions;
    స్థిరరాశి uint32_t m_tuples;  // Fixed capacity number of cross tuples (VlCoverCrossT's Tuples)
    స్థిరరాశి uint32_t m_bins;  // Fixed capacity number of explicit bins (VlCoverCrossT's Bins)
    స్థిరరాశి uint32_t m_autoBins;  // Fixed capacity number of auto bins (VlCoverCrossT's AutoBins)
    స్థిరరాశి uint64_t m_bin_పదాలు;  // Fixed capacity selection words (VlCoverCrossT's BinWords)
    స్థిరరాశి bool m_dynamic;  // Bin layout is determined at covergroup construction

బహిరంగ:
    AstCoverCrossDType(File_పంక్తి* fl, uint32_t dimensions, uint32_t tuples, uint32_t bins,
                       uint32_t autoBins, uint64_t bin_పదాలు, bool dynamic = false)
        : ASTGEN_SUPER_CoverCrossDType(fl)
        , m_dimensions{dimensions}
        , m_tuples{tuples}
        , m_bins{bins}
        , m_autoBins{autoBins}
        , m_bin_పదాలు{bin_పదాలు}
        , m_dynamic{dynamic} {
        dtypep(this);
    }
    ASTGEN_MEMBERS_AstCoverCrossDType;
    స్థిరరాశి అక్షరం* broken() స్థిరరాశి override {
        BROKEN_RTN(m_dimensions == 0);
        తిరిగి nullptr;
    }
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* samep) స్థిరరాశి override {  // LCOV_EXCL_START
        స్థిరరాశి AstCoverCrossDType* స్థిరరాశి sp = VN_DBG_AS(samep, CoverCrossDType);
        తిరిగి dimensions() == sp->dimensions() && tuples() == sp->tuples() && bins() == sp->bins()
               && autoBins() == sp->autoBins() && bin_పదాలు() == sp->bin_పదాలు()
               && isDynamic() == sp->isDynamic();
    }  // LCOV_EXCL_STOP
    bool similar_DType_కణుపు(స్థిరరాశి Ast_కణుపు_DType* samep) స్థిరరాశి override { తిరిగి this == samep; }
    శూన్య dump(std::ostream& str) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str) స్థిరరాశి override;
    శూన్య dumpSmall(std::ostream& str) స్థిరరాశి override;
    uint32_t dimensions() స్థిరరాశి { తిరిగి m_dimensions; }
    uint32_t tuples() స్థిరరాశి { తిరిగి m_tuples; }
    uint32_t bins() స్థిరరాశి { తిరిగి m_bins; }
    uint32_t autoBins() స్థిరరాశి { తిరిగి m_autoBins; }
    uint64_t bin_పదాలు() స్థిరరాశి { తిరిగి m_bin_పదాలు; }
    bool isDynamic() స్థిరరాశి { తిరిగి m_dynamic; }
    string cpp_Template_వాదనలు() స్థిరరాశి;
    AstBasicDType* basicp() స్థిరరాశి override VL_MT_STABLE { తిరిగి nullptr; }
    పూర్ణాంకం వెడల్పు_Align_బైట్లు() స్థిరరాశి override { తిరిగి పరిమాణం(శూన్య*); }
    పూర్ణాంకం వెడల్పు_మొత్తం_బైట్లు() స్థిరరాశి override { తిరిగి పరిమాణం(శూన్య*); }
    bool isCompound() స్థిరరాశి override { తిరిగి true; }
};
class AstCoverpointDType final : బహిరంగ Ast_కణుపు_DType {
    // Borrowed pointer to a covergroup coverpoint runtime, 'VlCoverpointT<hitBound>*'.
    // Follows pattern of AstQueueDType in capturing the compile-time max bin overlap
    // template argument.
    uint32_t m_తగిలింది_Bound;  // VlCoverpointT<> template argument; hit list size, >= 1
బహిరంగ:
    AstCoverpointDType(File_పంక్తి* fl, uint32_t తగిలింది_Bound)
        : ASTGEN_SUPER_CoverpointDType(fl)
        , m_తగిలింది_Bound{తగిలింది_Bound} {
        dtypep(this);
    }
    ASTGEN_MEMBERS_AstCoverpointDType;
    స్థిరరాశి అక్షరం* broken() స్థిరరాశి override {
        BROKEN_RTN(m_తగిలింది_Bound < 1);
        తిరిగి nullptr;
    }
    // V3Covergroup interns these one-per-hitBound into the type table, so identity is
    // equality; there is never a second node with the same bound to compare against.
    bool similar_DType_కణుపు(స్థిరరాశి Ast_కణుపు_DType* samep) స్థిరరాశి override { తిరిగి this == samep; }
    శూన్య dumpSmall(std::ostream& str) స్థిరరాశి override;
    // ACCESSORS
    uint32_t తగిలింది_Bound() స్థిరరాశి { తిరిగి m_తగిలింది_Bound; }
    // METHODS
    AstBasicDType* basicp() స్థిరరాశి override VL_MT_STABLE { తిరిగి nullptr; }
    పూర్ణాంకం వెడల్పు_Align_బైట్లు() స్థిరరాశి override { తిరిగి పరిమాణం(శూన్య*); }
    పూర్ణాంకం వెడల్పు_మొత్తం_బైట్లు() స్థిరరాశి override { తిరిగి పరిమాణం(శూన్య*); }
    bool isCompound() స్థిరరాశి override { తిరిగి true; }
};
class AstDefImplicitDType final : బహిరంగ Ast_కణుపు_DType {
    // For parsing enum/struct/unions that are declared with a variable rather than typedef
    // This allows "var enum {...} a,b" to share the enum definition for both variables
    // After link, these become typedefs
    // @astgen op1 := childDTypep : Optional[AstNodeDType]
    string m_name;  // Data type name
    స్థిరరాశి పూర్ణాంకం m_unique_అంకె;  // Unique ID distinguishing this dtype instance, for hashing/naming

బహిరంగ:
    AstDefImplicitDType(File_పంక్తి* fl, స్థిరరాశి string& name, VFlag_బిడ్డ_DType, Ast_కణుపు_DType* dtp)
        : ASTGEN_SUPER_DefImplicitDType(fl)
        , m_name{name}
        , m_unique_అంకె{unique_అంకె_Inc()} {
        బిడ్డ_DTypep(dtp);  // Only for parser
        dtypep(nullptr);  // V3Width will resolve
    }
    AstDefImplicitDType(స్థిరరాశి AstDefImplicitDType& other)
        : Ast_కణుపు_DType(other)
        , m_name{other.m_name}
        , m_unique_అంకె{unique_అంకె_Inc()} {}
    ASTGEN_MEMBERS_AstDefImplicitDType;
    పూర్ణాంకం unique_అంకె() స్థిరరాశి { తిరిగి m_unique_అంకె; }
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* samep) స్థిరరాశి override {
        స్థిరరాశి AstDefImplicitDType* స్థిరరాశి sp = VN_DBG_AS(samep, DefImplicitDType);
        తిరిగి unique_అంకె() == sp->unique_అంకె();
    }
    bool similar_DType_కణుపు(స్థిరరాశి Ast_కణుపు_DType* samep) స్థిరరాశి override { తిరిగి same_కణుపు(samep); }
    Ast_కణుపు_DType* get_బిడ్డ_DTypep() స్థిరరాశి override { తిరిగి బిడ్డ_DTypep(); }
    Ast_కణుపు_DType* subDTypep() స్థిరరాశి override VL_MT_STABLE {
        తిరిగి dtypep() ? dtypep() : బిడ్డ_DTypep();
    }
    // METHODS
    // op1 = Range of variable
    Ast_కణుపు_DType* dtypeSkipRefp() స్థిరరాశి { తిరిగి dtypep()->skipRefp(); }
    AstBasicDType* basicp() స్థిరరాశి override VL_MT_STABLE { తిరిగి subDTypep()->basicp(); }
    పూర్ణాంకం వెడల్పు_Align_బైట్లు() స్థిరరాశి override { తిరిగి dtypep()->వెడల్పు_Align_బైట్లు(); }
    పూర్ణాంకం వెడల్పు_మొత్తం_బైట్లు() స్థిరరాశి override { తిరిగి dtypep()->వెడల్పు_మొత్తం_బైట్లు(); }
    string name() స్థిరరాశి override VL_MT_STABLE { తిరిగి m_name; }
    శూన్య name(స్థిరరాశి string& flag) override { m_name = flag; }
    bool isCompound() స్థిరరాశి override { తిరిగి false; }
};
class AstDynArrayDType final : బహిరంగ Ast_కణుపు_DType {
    // Dynamic array data type, ie "[]"
    // @astgen op1 := childDTypep : Optional[AstNodeDType] // moved to refDTypep() in V3Width
    //
    // @astgen ptr := m_refDTypep : Optional[AstNodeDType]  // Elements of this type (post-width)
బహిరంగ:
    AstDynArrayDType(File_పంక్తి* fl, VFlag_బిడ్డ_DType, Ast_కణుపు_DType* dtp)
        : ASTGEN_SUPER_DynArrayDType(fl) {
        బిడ్డ_DTypep(dtp);  // Only for parser
        refDTypep(nullptr);
        dtypep(nullptr);  // V3Width will resolve
    }
    AstDynArrayDType(File_పంక్తి* fl, Ast_కణుపు_DType* dtp)
        : ASTGEN_SUPER_DynArrayDType(fl) {
        refDTypep(dtp);
        dtypep(nullptr);  // V3Width will resolve
    }
    ASTGEN_MEMBERS_AstDynArrayDType;
    స్థిరరాశి అక్షరం* broken() స్థిరరాశి override {
        BROKEN_RTN(!((m_refDTypep && !బిడ్డ_DTypep()) || (!m_refDTypep && బిడ్డ_DTypep())));
        తిరిగి nullptr;
    }
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* samep) స్థిరరాశి override {
        స్థిరరాశి AstDynArrayDType* స్థిరరాశి asamep = VN_DBG_AS(samep, DynArrayDType);
        ఒకవేళ (!asamep->subDTypep()) తిరిగి false;
        తిరిగి subDTypep() == asamep->subDTypep();
    }
    bool similar_DType_కణుపు(స్థిరరాశి Ast_కణుపు_DType* samep) స్థిరరాశి override {
        స్థిరరాశి AstDynArrayDType* స్థిరరాశి asamep = VN_DBG_AS(samep, DynArrayDType);
        తిరిగి asamep->subDTypep() && subDTypep()->similarDType(asamep->subDTypep());
    }
    string prettyDTypeName(bool నిండిన_2) స్థిరరాశి override;
    శూన్య dumpSmall(std::ostream& str) స్థిరరాశి override;
    Ast_కణుపు_DType* get_బిడ్డ_DTypep() స్థిరరాశి override { తిరిగి బిడ్డ_DTypep(); }
    Ast_కణుపు_DType* subDTypep() స్థిరరాశి override VL_MT_STABLE {
        తిరిగి m_refDTypep ? m_refDTypep : బిడ్డ_DTypep();
    }
    bool isAggregateType() స్థిరరాశి override { తిరిగి true; }
    శూన్య refDTypep(Ast_కణుపు_DType* nodep) { m_refDTypep = nodep; }
    Ast_కణుపు_DType* virtRefDTypep() స్థిరరాశి override { తిరిగి m_refDTypep; }
    శూన్య virtRefDTypep(Ast_కణుపు_DType* nodep) override { refDTypep(nodep); }
    // METHODS
    AstBasicDType* basicp() స్థిరరాశి override VL_MT_STABLE { తిరిగి nullptr; }
    పూర్ణాంకం వెడల్పు_Align_బైట్లు() స్థిరరాశి override { తిరిగి subDTypep()->వెడల్పు_Align_బైట్లు(); }
    పూర్ణాంకం వెడల్పు_మొత్తం_బైట్లు() స్థిరరాశి override { తిరిగి subDTypep()->వెడల్పు_మొత్తం_బైట్లు(); }
    bool isCompound() స్థిరరాశి override { తిరిగి true; }
    bool isDynamicallySized() స్థిరరాశి override { తిరిగి true; }
};
class Ast_ఖాళీలు_వరుస_DType final : బహిరంగ Ast_కణుపు_DType {
    // For EmptyQueue
బహిరంగ:
    explicit Ast_ఖాళీలు_వరుస_DType(File_పంక్తి* fl)
        : ASTGEN_SUPER_ఖాళీలు_వరుస_DType(fl) {
        dtypep(this);
    }
    ASTGEN_MEMBERS_Ast_ఖాళీలు_వరుస_DType;
    శూన్య dumpSmall(std::ostream& str) స్థిరరాశి override;
    bool hasDType() స్థిరరాశి override VL_MT_సురక్షిత { తిరిగి true; }
    bool maybe_Pointed_గమ్యం() స్థిరరాశి override VL_MT_సురక్షిత { తిరిగి true; }
    bool undead() స్థిరరాశి override { తిరిగి true; }
    Ast_కణుపు_DType* subDTypep() స్థిరరాశి override VL_MT_STABLE { తిరిగి nullptr; }
    Ast_కణుపు_DType* virtRefDTypep() స్థిరరాశి override { తిరిగి nullptr; }
    శూన్య virtRefDTypep(Ast_కణుపు_DType* nodep) override {}
    bool similar_DType_కణుపు(స్థిరరాశి Ast_కణుపు_DType* samep) స్థిరరాశి override { తిరిగి this == samep; }
    AstBasicDType* basicp() స్థిరరాశి override VL_MT_STABLE { తిరిగి nullptr; }
    పూర్ణాంకం వెడల్పు_Align_బైట్లు() స్థిరరాశి override { తిరిగి 1; }
    పూర్ణాంకం వెడల్పు_మొత్తం_బైట్లు() స్థిరరాశి override { తిరిగి 1; }
    bool isCompound() స్థిరరాశి override { తిరిగి false; }
};
class AstEnumDType final : బహిరంగ Ast_కణుపు_DType {
    // Parents: TYPEDEF/MODULE
    // @astgen op1 := childDTypep : Optional[AstNodeDType]
    // @astgen op2 := itemsp : List[AstEnumItem]
    //
    // @astgen ptr := m_refDTypep : Optional[AstNodeDType]  // Elements of this type (post-width)
    string m_name;  // Name from upper typedef, if any
    string m_typedefName;  // Typedef's dtypeName() once moved off the typedef declaring it
    స్థిరరాశి పూర్ణాంకం m_unique_అంకె;  // Unique ID distinguishing this dtype instance, for hashing/naming

బహిరంగ:
    AstEnumDType(File_పంక్తి* fl, VFlag_బిడ్డ_DType, Ast_కణుపు_DType* dtp, Ast_Enum_అంశం* itemsp)
        : ASTGEN_SUPER_EnumDType(fl)
        , m_unique_అంకె{unique_అంకె_Inc()} {
        బిడ్డ_DTypep(dtp);  // Only for parser
        refDTypep(nullptr);
        కూడు_Itemsp(itemsp);
        dtypep(nullptr);  // V3Width will resolve
        వెడల్పు_From_Sub(subDTypep());
    }
    AstEnumDType(స్థిరరాశి AstEnumDType& other)
        : Ast_కణుపు_DType{other}
        , m_name{other.m_name}
        , m_typedefName{other.m_typedefName}
        , m_unique_అంకె{unique_అంకె_Inc()} {}
    ASTGEN_MEMBERS_AstEnumDType;

    స్థిరరాశి అక్షరం* broken() స్థిరరాశి override;
    పూర్ణాంకం unique_అంకె() స్థిరరాశి { తిరిగి m_unique_అంకె; }
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* samep) స్థిరరాశి override {
        స్థిరరాశి AstEnumDType* స్థిరరాశి sp = VN_DBG_AS(samep, EnumDType);
        తిరిగి unique_అంకె() == sp->unique_అంకె();
    }
    bool similar_DType_కణుపు(స్థిరరాశి Ast_కణుపు_DType* samep) స్థిరరాశి override { తిరిగి same_కణుపు(samep); }
    Ast_కణుపు_DType* get_బిడ్డ_DTypep() స్థిరరాశి override { తిరిగి బిడ్డ_DTypep(); }
    Ast_కణుపు_DType* subDTypep() స్థిరరాశి override VL_MT_STABLE {
        తిరిగి m_refDTypep ? m_refDTypep : బిడ్డ_DTypep();
    }
    శూన్య refDTypep(Ast_కణుపు_DType* nodep) { m_refDTypep = nodep; }
    Ast_కణుపు_DType* virtRefDTypep() స్థిరరాశి override { తిరిగి m_refDTypep; }
    శూన్య virtRefDTypep(Ast_కణుపు_DType* nodep) override { refDTypep(nodep); }
    string name() స్థిరరాశి override VL_MT_STABLE { తిరిగి m_name; }
    శూన్య name(స్థిరరాశి string& flag) override { m_name = flag; }
    // Keep the name the typedef declaring it gives, as it moves off it to the type table
    శూన్య typedefName(స్థిరరాశి string& name) { m_typedefName = name; }
    శూన్య dump(std::ostream& str = std::cout) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str = std::cout) స్థిరరాశి override;
    శూన్య dumpSmall(std::ostream& str) స్థిరరాశి override;
    string prettyDTypeName(bool నిండిన_2) స్థిరరాశి override;
    // METHODS
    AstBasicDType* basicp() స్థిరరాశి override VL_MT_STABLE { తిరిగి subDTypep()->basicp(); }
    పూర్ణాంకం వెడల్పు_Align_బైట్లు() స్థిరరాశి override { తిరిగి subDTypep()->వెడల్పు_Align_బైట్లు(); }
    పూర్ణాంకం వెడల్పు_మొత్తం_బైట్లు() స్థిరరాశి override { తిరిగి subDTypep()->వెడల్పు_మొత్తం_బైట్లు(); }
    కొలత_t అంశం_లెక్క() స్థిరరాశి {
        కొలత_t లెక్క = 0;
        కొరకు (Ast_కణుపు* itemp = itemsp(); itemp; itemp = itemp->nextp()) ++లెక్క;
        తిరిగి లెక్క;
    }
    bool isCompound() స్థిరరాశి override { తిరిగి false; }
};

class AstIfaceGenericDType final : బహిరంగ Ast_కణుపు_DType {
    // Generic interface that will be replaced with AstIfaceRefDType
    File_పంక్తి* m_modportFileline;  // Where modport token was
    // dist-ast-dump-suppress  // Part of name()
    string m_modportName;  // "" = no modport
బహిరంగ:
    explicit AstIfaceGenericDType(File_పంక్తి* fl)
        : ASTGEN_SUPER_IfaceGenericDType(fl) {
        dtypep(this);
    }
    AstIfaceGenericDType(File_పంక్తి* fl, File_పంక్తి* modportFl, స్థిరరాశి string& modport)
        : ASTGEN_SUPER_IfaceGenericDType(fl)
        , m_modportFileline{modportFl}
        , m_modportName{modport} {
        dtypep(this);
    }
    ASTGEN_MEMBERS_AstIfaceGenericDType;
    శూన్య dumpSmall(std::ostream& str) స్థిరరాశి override;
    bool hasDType() స్థిరరాశి override VL_MT_సురక్షిత { తిరిగి true; }
    bool maybe_Pointed_గమ్యం() స్థిరరాశి override VL_MT_సురక్షిత { తిరిగి true; }
    bool undead() స్థిరరాశి override { తిరిగి true; }
    Ast_కణుపు_DType* subDTypep() స్థిరరాశి override VL_MT_STABLE { తిరిగి nullptr; }
    Ast_కణుపు_DType* virtRefDTypep() స్థిరరాశి override { తిరిగి nullptr; }
    శూన్య virtRefDTypep(Ast_కణుపు_DType* nodep) override {}
    bool similar_DType_కణుపు(స్థిరరాశి Ast_కణుపు_DType* samep) స్థిరరాశి override { తిరిగి this == samep; }
    AstBasicDType* basicp() స్థిరరాశి override VL_MT_STABLE { తిరిగి nullptr; }
    పూర్ణాంకం వెడల్పు_Align_బైట్లు() స్థిరరాశి override { తిరిగి 1; }
    పూర్ణాంకం వెడల్పు_మొత్తం_బైట్లు() స్థిరరాశి override { తిరిగి 1; }
    string modportName() స్థిరరాశి { తిరిగి m_modportName; }
    bool isModport() { తిరిగి !m_modportName.ఖాళీలు(); }
    bool isCompound() స్థిరరాశి override { తిరిగి true; }
    File_పంక్తి* modportFileline() స్థిరరాశి { తిరిగి m_modportFileline; }
    string name() స్థిరరాశి override { తిరిగి m_modportName; }
};

class AstIfaceRefDType final : బహిరంగ Ast_కణుపు_DType {
    // Reference to an interface, either for a port, or inside parent cell
    // @astgen op1 := paramsp : List[AstPin]
    //
    // @astgen ptr := m_ifacep : Optional[AstIface]  // Interface; cellp() should override
    // @astgen ptr := m_cellp : Optional[AstCell]  // When exact parent cell known; not a guess
    // @astgen ptr := m_modportp : Optional[AstModport]  // nullptr = unlinked or no modport
    File_పంక్తి* m_modportFileline;  // Where modport token was
    string m_cellName;  // "" = no cell, such as when connects to 'input' iface
    string m_ifaceName;  // Interface name
    string m_modportName;  // "" = no modport
    bool m_virtual = false;  // True if virtual interface
బహిరంగ:
    AstIfaceRefDType(File_పంక్తి* fl, స్థిరరాశి string& cellName, స్థిరరాశి string& ifaceName)
        : ASTGEN_SUPER_IfaceRefDType(fl)
        , m_modportFileline{nullptr}
        , m_cellName{cellName}
        , m_ifaceName{ifaceName}
        , m_modportName{""} {}
    AstIfaceRefDType(File_పంక్తి* fl, File_పంక్తి* modportFl, స్థిరరాశి string& cellName,
                     స్థిరరాశి string& ifaceName, స్థిరరాశి string& modport)
        : ASTGEN_SUPER_IfaceRefDType(fl)
        , m_modportFileline{modportFl}
        , m_cellName{cellName}
        , m_ifaceName{ifaceName}
        , m_modportName{modport} {}
    AstIfaceRefDType(File_పంక్తి* fl, File_పంక్తి* modportFl, స్థిరరాశి string& cellName,
                     స్థిరరాశి string& ifaceName, స్థిరరాశి string& modport, AstPin* paramsp)
        : ASTGEN_SUPER_IfaceRefDType(fl)
        , m_modportFileline{modportFl}
        , m_cellName{cellName}
        , m_ifaceName{ifaceName}
        , m_modportName{modport} {
        కూడు_Paramsp(paramsp);
    }
    ASTGEN_MEMBERS_AstIfaceRefDType;

    // METHODS
    శూన్య dump(std::ostream& str = std::cout) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str = std::cout) స్థిరరాశి override;
    శూన్య dumpSmall(std::ostream& str) స్థిరరాశి override;
    string prettyDTypeName(bool నిండిన_2) స్థిరరాశి override;
    AstBasicDType* basicp() స్థిరరాశి override VL_MT_STABLE { తిరిగి nullptr; }
    bool similar_DType_కణుపు(స్థిరరాశి Ast_కణుపు_DType* samep) స్థిరరాశి override {
        // Each occurrence of a virtual interface type parses to its own node,
        // so pointer identity is insufficient; compare the referenced
        // interface instead, same equivalence as AstNode::computeCastable uses.
        స్థిరరాశి AstIfaceRefDType* స్థిరరాశి asamep = VN_DBG_AS(samep, IfaceRefDType);
        తిరిగి iface_ద్వారా_Cellp() && iface_ద్వారా_Cellp() == asamep->iface_ద్వారా_Cellp()
               && modportName() == asamep->modportName() && isVirtual() == asamep->isVirtual();
    }
    పూర్ణాంకం వెడల్పు_Align_బైట్లు() స్థిరరాశి override { తిరిగి 0; }
    పూర్ణాంకం వెడల్పు_మొత్తం_బైట్లు() స్థిరరాశి override { తిరిగి 0; }
    bool isVirtual() స్థిరరాశి { తిరిగి m_virtual; }
    శూన్య isVirtual(bool flag) {
        m_virtual = flag;
        ఒకవేళ (flag) v3Global.సమితి_Has_Virt_Ifaces();
    }
    File_పంక్తి* modportFileline() స్థిరరాశి { తిరిగి m_modportFileline; }
    శూన్య modportFileline(File_పంక్తి* స్థిరరాశి modportFileline) { m_modportFileline = modportFileline; }
    string cellName() స్థిరరాశి { తిరిగి m_cellName; }
    శూన్య cellName(స్థిరరాశి string& name) { m_cellName = name; }
    string ifaceName() స్థిరరాశి { తిరిగి m_ifaceName; }
    string ifaceNameQ() స్థిరరాశి { తిరిగి "'" + prettyName(ifaceName()) + "'"; }
    శూన్య ifaceName(స్థిరరాశి string& name) { m_ifaceName = name; }
    శూన్య modportName(స్థిరరాశి string& modportName) { m_modportName = modportName; }
    string modportName() స్థిరరాశి { తిరిగి m_modportName; }
    AstIface* iface_ద్వారా_Cellp() స్థిరరాశి;  // Use cellp or ifacep
    AstIface* ifacep() స్థిరరాశి { తిరిగి m_ifacep; }
    శూన్య ifacep(AstIface* nodep) { m_ifacep = nodep; }
    AstCell* cellp() స్థిరరాశి { తిరిగి m_cellp; }
    శూన్య cellp(AstCell* nodep) { m_cellp = nodep; }
    AstModport* modportp() స్థిరరాశి { తిరిగి m_modportp; }
    శూన్య modportp(AstModport* modportp) { m_modportp = modportp; }
    bool isModport() { తిరిగి !m_modportName.ఖాళీలు(); }
    bool isCompound() స్థిరరాశి override { తిరిగి true; }  // But not relevant
};
class AstMemberDType final : బహిరంగ Ast_కణుపు_DType {
    // A member of a struct/union
    // PARENT: AstNodeUOrStructDType
    // @astgen op1 := childDTypep : Optional[AstNodeDType]
    // @astgen op3 := valuep : Optional[AstNode]
    //
    // @astgen ptr := m_refDTypep : Optional[AstNodeDType]  // Elements of this type (post-width)
    string m_name;  // Name of variable
    string m_ట్యాగ్;  // Holds the string of the verilator tag -- used in JSON output.
    పూర్ణాంకం m_lsb = -1;  // Within this level's packed struct, the LSB of the first bit of the member
    bool m_constrainedRand = false;  // Member has a constraint expression
    VRandAttr m_rand;  // Randomizability of this member (rand, randc, etc)
బహిరంగ:
    AstMemberDType(File_పంక్తి* fl, స్థిరరాశి string& name, VFlag_బిడ్డ_DType, Ast_కణుపు_DType* dtp,
                   Ast_కణుపు* valuep)
        : ASTGEN_SUPER_MemberDType(fl)
        , m_name{name} {
        బిడ్డ_DTypep(dtp);  // Only for parser
        this->valuep(valuep);
        dtypep(nullptr);  // V3Width will resolve
        refDTypep(nullptr);
    }
    AstMemberDType(File_పంక్తి* fl, స్థిరరాశి string& name, Ast_కణుపు_DType* dtp)
        : ASTGEN_SUPER_MemberDType(fl)
        , m_name{name} {
        UASSERT(dtp, "AstMember created with no dtype");
        refDTypep(dtp);
        dtypep(this);
        వెడల్పు_From_Sub(subDTypep());
    }
    ASTGEN_MEMBERS_AstMemberDType;
    శూన్య dump(std::ostream& str = std::cout) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str = std::cout) స్థిరరాశి override;
    శూన్య dumpSmall(std::ostream& str) స్థిరరాశి override;
    string prettyDTypeName(bool నిండిన_2) స్థిరరాశి override;
    string name() స్థిరరాశి override VL_MT_STABLE { తిరిగి m_name; }  // * = Var name
    bool hasDType() స్థిరరాశి override VL_MT_సురక్షిత { తిరిగి true; }
    bool maybe_Pointed_గమ్యం() స్థిరరాశి override VL_MT_సురక్షిత { తిరిగి true; }
    Ast_కణుపు_DType* get_బిడ్డ_DTypep() స్థిరరాశి override { తిరిగి బిడ్డ_DTypep(); }
    Ast_కణుపు_UOr_Struct_DType* get_బిడ్డ_Structp();
    Ast_కణుపు_DType* subDTypep() స్థిరరాశి override VL_MT_STABLE {
        తిరిగి m_refDTypep ? m_refDTypep : బిడ్డ_DTypep();
    }
    శూన్య refDTypep(Ast_కణుపు_DType* nodep) { m_refDTypep = nodep; }
    Ast_కణుపు_DType* virtRefDTypep() స్థిరరాశి override { తిరిగి m_refDTypep; }
    శూన్య virtRefDTypep(Ast_కణుపు_DType* nodep) override { refDTypep(nodep); }
    bool similar_DType_కణుపు(స్థిరరాశి Ast_కణుపు_DType* samep) స్థిరరాశి override { తిరిగి this == samep; }
    //
    // (Slow) recurse down to find basic data type (Note don't need virtual -
    // AstVar isn't a NodeDType)
    AstBasicDType* basicp() స్థిరరాశి override VL_MT_STABLE { తిరిగి subDTypep()->basicp(); }
    // op1 = Range of variable (Note don't need virtual - AstVar isn't a NodeDType)
    Ast_కణుపు_DType* dtypeSkipRefp() స్థిరరాశి { తిరిగి subDTypep()->skipRefp(); }
    // (Slow) recurses - Structure alignment 1,2,4 or 8 bytes (arrays affect this)
    పూర్ణాంకం వెడల్పు_Align_బైట్లు() స్థిరరాశి override { తిరిగి subDTypep()->వెడల్పు_Align_బైట్లు(); }
    // (Slow) recurses - Width in bytes rounding up 1,2,4,8,12,...
    పూర్ణాంకం వెడల్పు_మొత్తం_బైట్లు() స్థిరరాశి override { తిరిగి subDTypep()->వెడల్పు_మొత్తం_బైట్లు(); }
    // METHODS
    శూన్య name(స్థిరరాశి string& name) override { m_name = name; }
    string ట్యాగ్() స్థిరరాశి override { తిరిగి m_ట్యాగ్; }
    శూన్య ట్యాగ్(స్థిరరాశి string& పాఠ్యం) override { m_ట్యాగ్ = పాఠ్యం; }
    పూర్ణాంకం lsb() స్థిరరాశి { తిరిగి m_lsb; }
    శూన్య lsb(పూర్ణాంకం lsb) { m_lsb = lsb; }
    bool isCompound() స్థిరరాశి override {
        v3fatalSrc("call isCompound on subdata type, not reference");
        తిరిగి false;
    }
    bool isConstrainedRand() స్థిరరాశి { తిరిగి m_constrainedRand; }
    శూన్య markConstrainedRand(bool flag) { m_constrainedRand = flag; }
    VRandAttr rand() స్థిరరాశి { తిరిగి m_rand; }
    శూన్య rand(స్థిరరాశి VRandAttr flag) { m_rand = flag; }
};
class Ast_NBACommit_వరుస_DType final : బహిరంగ Ast_కణుపు_DType {
    // @astgen ptr := m_subDTypep : AstNodeDType  // Type of the corresponding variable
    స్థిరరాశి bool m_partial;  // Partial element update required

బహిరంగ:
    Ast_NBACommit_వరుస_DType(File_పంక్తి* fl, Ast_కణుపు_DType* subDTypep, bool partial)
        : ASTGEN_SUPER_NBACommit_వరుస_DType(fl)
        , m_partial{partial}
        , m_subDTypep{subDTypep} {
        dtypep(this);
    }
    ASTGEN_MEMBERS_Ast_NBACommit_వరుస_DType;
    శూన్య dump(std::ostream& str) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str) స్థిరరాశి override;
    Ast_కణుపు_DType* subDTypep() స్థిరరాశి override VL_MT_STABLE { తిరిగి m_subDTypep; }
    bool partial() స్థిరరాశి { తిరిగి m_partial; }
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* samep) స్థిరరాశి override {
        స్థిరరాశి Ast_NBACommit_వరుస_DType* స్థిరరాశి asamep = VN_DBG_AS(samep, NBACommit_వరుస_DType);
        తిరిగి m_partial == asamep->m_partial;
    }
    bool similar_DType_కణుపు(స్థిరరాశి Ast_కణుపు_DType* samep) స్థిరరాశి override { తిరిగి this == samep; }
    AstBasicDType* basicp() స్థిరరాశి override VL_MT_STABLE { తిరిగి nullptr; }
    పూర్ణాంకం వెడల్పు_Align_బైట్లు() స్థిరరాశి override { తిరిగి 1; }
    పూర్ణాంకం వెడల్పు_మొత్తం_బైట్లు() స్థిరరాశి override { తిరిగి 24; }
    bool isCompound() స్థిరరాశి override { తిరిగి true; }
};
class AstParamTypeDType final : బహిరంగ Ast_కణుపు_DType {
    // Parents: MODULE
    // A parameter type statement; much like a var or typedef
    // @astgen op1 := childDTypep : Optional[AstNodeDType]
    స్థిరరాశి VVarType m_varType;  // Type of variable (for localparam vs. param)
    స్థిరరాశి VFwdType m_fwdType;  // Forward type for lint check
    string m_name;  // Name of variable
బహిరంగ:
    AstParamTypeDType(File_పంక్తి* fl, VVarType type, VFwdType fwdType, స్థిరరాశి string& name,
                      VFlag_బిడ్డ_DType, Ast_కణుపు_DType* dtp)
        : ASTGEN_SUPER_ParamTypeDType(fl)
        , m_varType{type}
        , m_fwdType{fwdType}
        , m_name{name} {
        బిడ్డ_DTypep(dtp);  // Only for parser
        dtypep(nullptr);  // V3Width will resolve
    }
    ASTGEN_MEMBERS_AstParamTypeDType;
    శూన్య dump(std::ostream& str = std::cout) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str = std::cout) స్థిరరాశి override;
    string prettyDTypeName(bool నిండిన_2) స్థిరరాశి override;
    Ast_కణుపు_DType* get_బిడ్డ_DTypep() స్థిరరాశి override { తిరిగి బిడ్డ_DTypep(); }
    Ast_కణుపు_DType* subDTypep() స్థిరరాశి override VL_MT_STABLE {
        తిరిగి dtypep() ? dtypep() : బిడ్డ_DTypep();
    }
    AstBasicDType* basicp() స్థిరరాశి override VL_MT_STABLE { తిరిగి subDTypep()->basicp(); }
    bool similar_DType_కణుపు(స్థిరరాశి Ast_కణుపు_DType* samep) స్థిరరాశి override {
        స్థిరరాశి AstParamTypeDType* స్థిరరాశి sp = VN_DBG_AS(samep, ParamTypeDType);
        తిరిగి this->subDTypep()->similarDType(sp->subDTypep());
    }
    పూర్ణాంకం వెడల్పు_Align_బైట్లు() స్థిరరాశి override { తిరిగి dtypep()->వెడల్పు_Align_బైట్లు(); }
    పూర్ణాంకం వెడల్పు_మొత్తం_బైట్లు() స్థిరరాశి override { తిరిగి dtypep()->వెడల్పు_మొత్తం_బైట్లు(); }
    // METHODS
    string name() స్థిరరాశి override VL_MT_STABLE { తిరిగి m_name; }
    bool maybe_Pointed_గమ్యం() స్థిరరాశి override VL_MT_సురక్షిత { తిరిగి true; }
    bool hasDType() స్థిరరాశి override VL_MT_సురక్షిత { తిరిగి true; }
    శూన్య name(స్థిరరాశి string& flag) override { m_name = flag; }
    VVarType varType() స్థిరరాశి { తిరిగి m_varType; }  // * = Type of variable
    VFwdType fwdType() స్థిరరాశి { తిరిగి m_fwdType; }
    bool isParam() స్థిరరాశి { తిరిగి true; }
    bool isGParam() స్థిరరాశి { తిరిగి (varType() == VVarType::GPARAM); }
    bool isCompound() స్థిరరాశి override {
        v3fatalSrc("call isCompound on subdata type, not reference");
        తిరిగి false;
    }
};
class AstParseTypeDType final : బహిరంగ Ast_కణుపు_DType {
    // Parents: VAR
    // During parsing, this indicates the type of a parameter is a "parameter type"
    // e.g. the data type is a container of any data type
    స్థిరరాశి VFwdType m_fwdType;  // Forward type for lint check
బహిరంగ:
    explicit AstParseTypeDType(File_పంక్తి* fl, VFwdType fwdType = VFwdType::NONE)
        : ASTGEN_SUPER_ParseTypeDType(fl)
        , m_fwdType{fwdType} {}
    ASTGEN_MEMBERS_AstParseTypeDType;
    Ast_కణుపు_DType* dtypep() స్థిరరాశి VL_MT_STABLE { తిరిగి nullptr; }
    // METHODS
    శూన్య dump(std::ostream& str = std::cout) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str = std::cout) స్థిరరాశి override;
    bool similar_DType_కణుపు(స్థిరరాశి Ast_కణుపు_DType* samep) స్థిరరాశి override { తిరిగి this == samep; }
    AstBasicDType* basicp() స్థిరరాశి override VL_MT_STABLE { తిరిగి nullptr; }
    పూర్ణాంకం వెడల్పు_Align_బైట్లు() స్థిరరాశి override { తిరిగి 0; }
    పూర్ణాంకం వెడల్పు_మొత్తం_బైట్లు() స్థిరరాశి override { తిరిగి 0; }
    bool isCompound() స్థిరరాశి override {
        v3fatalSrc("call isCompound on subdata type, not reference");
        తిరిగి false;
    }
    VFwdType fwdType() స్థిరరాశి { తిరిగి m_fwdType; }
};
class Ast_వరుస_DType final : బహిరంగ Ast_కణుపు_DType {
    // Queue array data type, ie "[ $ ]"
    // @astgen op1 := childDTypep : Optional[AstNodeDType] // moved to refDTypep() in V3Width
    // @astgen op2 := boundp : Optional[AstNodeExpr]
    //
    // @astgen ptr := m_refDTypep : Optional[AstNodeDType]  // Elements of this type (post-width)
బహిరంగ:
    Ast_వరుస_DType(File_పంక్తి* fl, VFlag_బిడ్డ_DType, Ast_కణుపు_DType* dtp, Ast_కణుపు_సమాసం* boundp)
        : ASTGEN_SUPER_వరుస_DType(fl) {
        బిడ్డ_DTypep(dtp);
        this->boundp(boundp);
        refDTypep(nullptr);
        dtypep(nullptr);  // V3Width will resolve
    }
    Ast_వరుస_DType(File_పంక్తి* fl, Ast_కణుపు_DType* dtp, Ast_కణుపు_సమాసం* boundp)
        : ASTGEN_SUPER_వరుస_DType(fl) {
        this->boundp(boundp);
        refDTypep(dtp);
        dtypep(dtp);
    }
    ASTGEN_MEMBERS_Ast_వరుస_DType;
    స్థిరరాశి అక్షరం* broken() స్థిరరాశి override {
        BROKEN_RTN(!((m_refDTypep && !బిడ్డ_DTypep()) || (!m_refDTypep && బిడ్డ_DTypep())));
        తిరిగి nullptr;
    }
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* samep) స్థిరరాశి override {
        స్థిరరాశి Ast_వరుస_DType* స్థిరరాశి asamep = VN_DBG_AS(samep, వరుస_DType);
        ఒకవేళ (!asamep->subDTypep()) తిరిగి false;
        తిరిగి (subDTypep() == asamep->subDTypep());
    }
    bool similar_DType_కణుపు(స్థిరరాశి Ast_కణుపు_DType* samep) స్థిరరాశి override {
        స్థిరరాశి Ast_వరుస_DType* స్థిరరాశి asamep = VN_DBG_AS(samep, వరుస_DType);
        తిరిగి asamep->subDTypep() && subDTypep()->similarDType(asamep->subDTypep());
    }
    శూన్య dumpSmall(std::ostream& str) స్థిరరాశి override;
    string prettyDTypeName(bool నిండిన_2) స్థిరరాశి override;
    Ast_కణుపు_DType* get_బిడ్డ_DTypep() స్థిరరాశి override { తిరిగి బిడ్డ_DTypep(); }
    Ast_కణుపు_DType* subDTypep() స్థిరరాశి override VL_MT_STABLE {
        తిరిగి m_refDTypep ? m_refDTypep : బిడ్డ_DTypep();
    }
    bool isAggregateType() స్థిరరాశి override { తిరిగి true; }
    శూన్య refDTypep(Ast_కణుపు_DType* nodep) { m_refDTypep = nodep; }
    అంతఃవరుస పూర్ణాంకం boundConst() స్థిరరాశి VL_MT_STABLE;
    Ast_కణుపు_DType* virtRefDTypep() స్థిరరాశి override { తిరిగి m_refDTypep; }
    శూన్య virtRefDTypep(Ast_కణుపు_DType* nodep) override { refDTypep(nodep); }
    // METHODS
    AstBasicDType* basicp() స్థిరరాశి override VL_MT_STABLE { తిరిగి nullptr; }
    పూర్ణాంకం వెడల్పు_Align_బైట్లు() స్థిరరాశి override { తిరిగి subDTypep()->వెడల్పు_Align_బైట్లు(); }
    పూర్ణాంకం వెడల్పు_మొత్తం_బైట్లు() స్థిరరాశి override { తిరిగి subDTypep()->వెడల్పు_మొత్తం_బైట్లు(); }
    bool isCompound() స్థిరరాశి override { తిరిగి true; }
    bool isDynamicallySized() స్థిరరాశి override { తిరిగి true; }
};
class AstRefDType final : బహిరంగ Ast_కణుపు_DType {
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
    స్థిరరాశి VIface_Capture_ట్యాగ్* m_captureTagp = nullptr;

బహిరంగ:
    AstRefDType(File_పంక్తి* fl, స్థిరరాశి string& name)
        : ASTGEN_SUPER_RefDType(fl)
        , m_name{name} {}
    AstRefDType(File_పంక్తి* fl, స్థిరరాశి string& name, Ast_కణుపు_సమాసం* classOrPackagep, AstPin* paramsp)
        : ASTGEN_SUPER_RefDType(fl)
        , m_name{name} {
        classOrPackageOpp(classOrPackagep);
        కూడు_Paramsp(paramsp);
    }
    class Flag_Type_యొక్క_సమాసం {};  // type(expr) for parser only
    AstRefDType(File_పంక్తి* fl, Flag_Type_యొక్క_సమాసం, Ast_కణుపు* typeofp)
        : ASTGEN_SUPER_RefDType(fl) {
        this->typeofp(typeofp);
        ఒకవేళ (Ast_కణుపు_DType* స్థిరరాశి dtp = VN_CAST(typeofp, కణుపు_DType)) refDTypep(dtp);
    }
    ASTGEN_MEMBERS_AstRefDType;
    // METHODS
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* samep) స్థిరరాశి override {
        స్థిరరాశి AstRefDType* స్థిరరాశి asamep = VN_DBG_AS(samep, RefDType);
        తిరిగి (m_typedefp == asamep->m_typedefp && m_refDTypep == asamep->m_refDTypep
                && m_name == asamep->m_name && m_classOrPackagep == asamep->m_classOrPackagep);
    }
    bool similar_DType_కణుపు(స్థిరరాశి Ast_కణుపు_DType* samep) స్థిరరాశి override {
        తిరిగి subDTypep()->similarDType(samep->subDTypep());
    }
    స్థిరరాశి అక్షరం* broken() స్థిరరాశి override;
    శూన్య dump(std::ostream& str = std::cout) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str = std::cout) స్థిరరాశి override;
    శూన్య dumpSmall(std::ostream& str) స్థిరరాశి override;
    string name() స్థిరరాశి override VL_MT_STABLE { తిరిగి m_name; }
    string prettyDTypeName(bool నిండిన_2) స్థిరరాశి override {
        తిరిగి subDTypep() ? subDTypep()->prettyDTypeName(నిండిన_2) : prettyName();
    }
    AstBasicDType* basicp() స్థిరరాశి override VL_MT_STABLE {
        తిరిగి subDTypep() ? subDTypep()->basicp() : nullptr;
    }
    Ast_కణుపు_DType* subDTypep() స్థిరరాశి override VL_MT_STABLE;
    Ast_కణుపు_DType* get_బిడ్డ_DTypep() స్థిరరాశి override { తిరిగి VN_CAST(typeofp(), కణుపు_DType); }
    పూర్ణాంకం వెడల్పు_Align_బైట్లు() స్థిరరాశి override { తిరిగి dtypeSkipRefp()->వెడల్పు_Align_బైట్లు(); }
    పూర్ణాంకం వెడల్పు_మొత్తం_బైట్లు() స్థిరరాశి override { తిరిగి dtypeSkipRefp()->వెడల్పు_మొత్తం_బైట్లు(); }
    శూన్య name(స్థిరరాశి string& flag) override { m_name = flag; }
    Ast_కణుపు_DType* dtypeSkipRefp() స్థిరరాశి { తిరిగి subDTypep()->skipRefp(); }
    AstTypedef* typedefp() స్థిరరాశి VL_MT_సురక్షిత { తిరిగి m_typedefp; }
    శూన్య typedefp(AstTypedef* nodep) { m_typedefp = nodep; }
    Ast_కణుపు_DType* refDTypep() స్థిరరాశి VL_MT_సురక్షిత { తిరిగి m_refDTypep; }
    శూన్య refDTypep(Ast_కణుపు_DType* nodep) { m_refDTypep = nodep; }
    Ast_కణుపు_DType* virtRefDTypep() స్థిరరాశి override { తిరిగి refDTypep(); }
    శూన్య virtRefDTypep(Ast_కణుపు_DType* nodep) override { refDTypep(nodep); }
    Ast_కణుపు_Module* classOrPackagep() స్థిరరాశి { తిరిగి m_classOrPackagep; }
    శూన్య classOrPackagep(Ast_కణుపు_Module* nodep) { m_classOrPackagep = nodep; }
    స్థిరరాశి VIface_Capture_ట్యాగ్* captureTagp() స్థిరరాశి { తిరిగి m_captureTagp; }
    శూన్య captureTagp(స్థిరరాశి VIface_Capture_ట్యాగ్* tagp) { m_captureTagp = tagp; }
    bool isCompound() స్థిరరాశి override {
        v3fatalSrc("call isCompound on subdata type, not reference");
        తిరిగి false;
    }
};
class AstRequireDType final : బహిరంగ Ast_కణుపు_DType {
    // @astgen op1 := lhsp : Optional[AstNode<AstNodeExpr|AstNodeDType>]
    //
    // Require a generic node type (typically AstParseRef become a type.
బహిరంగ:
    AstRequireDType(File_పంక్తి* fl, Ast_కణుపు* lhsp)
        : ASTGEN_SUPER_RequireDType(fl) {
        this->lhsp(lhsp);
    }
    ASTGEN_MEMBERS_AstRequireDType;
    // METHODS
    bool similar_DType_కణుపు(స్థిరరాశి Ast_కణుపు_DType* samep) స్థిరరాశి override {
        స్థిరరాశి AstRequireDType* స్థిరరాశి asamep = VN_DBG_AS(samep, RequireDType);
        తిరిగి subDTypep()->similarDType(asamep->subDTypep());
    }
    AstBasicDType* basicp() స్థిరరాశి override VL_MT_STABLE { తిరిగి nullptr; }
    Ast_కణుపు_DType* subDTypep() స్థిరరాశి override VL_MT_STABLE {
        // Used for recursive definition checking
        ఒకవేళ (Ast_కణుపు_DType* స్థిరరాశి dtp = VN_CAST(lhsp(), కణుపు_DType)) తిరిగి dtp;

        తిరిగి nullptr;
    }
    పూర్ణాంకం వెడల్పు_Align_బైట్లు() స్థిరరాశి override { V3ERROR_NA_RETURN(1); }
    పూర్ణాంకం వెడల్పు_మొత్తం_బైట్లు() స్థిరరాశి override { V3ERROR_NA_RETURN(1); }
    bool isCompound() స్థిరరాశి override { V3ERROR_NA_RETURN(false); }
};
class Ast_Sample_వరుస_DType final : బహిరంగ Ast_కణుపు_DType {
    // @astgen op1 := childDTypep : Optional[AstNodeDType] // moved to refDTypep() in V3Width
    //
    // @astgen ptr := m_refDTypep : Optional[AstNodeDType]  // Elements of this type (post-width)
బహిరంగ:
    Ast_Sample_వరుస_DType(File_పంక్తి* fl, Ast_కణుపు_DType* dtp)
        : ASTGEN_SUPER_Sample_వరుస_DType(fl) {
        refDTypep(dtp);
        dtypep(dtp);
    }
    ASTGEN_MEMBERS_Ast_Sample_వరుస_DType;
    స్థిరరాశి అక్షరం* broken() స్థిరరాశి override {
        BROKEN_RTN(!((m_refDTypep && !బిడ్డ_DTypep()) || (!m_refDTypep && బిడ్డ_DTypep())));
        తిరిగి nullptr;
    }
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* samep) స్థిరరాశి override {
        స్థిరరాశి Ast_Sample_వరుస_DType* స్థిరరాశి asamep = VN_DBG_AS(samep, Sample_వరుస_DType);
        ఒకవేళ (!asamep->subDTypep()) తిరిగి false;
        తిరిగి (subDTypep() == asamep->subDTypep());
    }
    bool similar_DType_కణుపు(స్థిరరాశి Ast_కణుపు_DType* samep) స్థిరరాశి override {
        స్థిరరాశి Ast_Sample_వరుస_DType* స్థిరరాశి asamep = VN_DBG_AS(samep, Sample_వరుస_DType);
        తిరిగి asamep->subDTypep() && subDTypep()->similarDType(asamep->subDTypep());
    }
    శూన్య dumpSmall(std::ostream& str) స్థిరరాశి override;
    Ast_కణుపు_DType* get_బిడ్డ_DTypep() స్థిరరాశి override { తిరిగి బిడ్డ_DTypep(); }
    // op1 = Range of variable
    Ast_కణుపు_DType* subDTypep() స్థిరరాశి override VL_MT_STABLE {
        తిరిగి m_refDTypep ? m_refDTypep : బిడ్డ_DTypep();
    }
    శూన్య refDTypep(Ast_కణుపు_DType* nodep) { m_refDTypep = nodep; }
    Ast_కణుపు_DType* virtRefDTypep() స్థిరరాశి override { తిరిగి m_refDTypep; }
    శూన్య virtRefDTypep(Ast_కణుపు_DType* nodep) override { refDTypep(nodep); }
    // METHODS
    AstBasicDType* basicp() స్థిరరాశి override VL_MT_STABLE { తిరిగి subDTypep()->basicp(); }
    పూర్ణాంకం వెడల్పు_Align_బైట్లు() స్థిరరాశి override { తిరిగి పరిమాణం(std::map<std::string, std::string>); }
    పూర్ణాంకం వెడల్పు_మొత్తం_బైట్లు() స్థిరరాశి override { తిరిగి పరిమాణం(std::map<std::string, std::string>); }
    bool isCompound() స్థిరరాశి override { తిరిగి true; }
};
class Ast_ప్రవాహం_DType final : బహిరంగ Ast_కణుపు_DType {
    // Stream data type, used only as data type of stream operations
    // Should behave like AstPackArrayDType, but it doesn't have a size
బహిరంగ:
    explicit Ast_ప్రవాహం_DType(File_పంక్తి* fl)
        : ASTGEN_SUPER_ప్రవాహం_DType(fl) {
        dtypep(this);
    }
    ASTGEN_MEMBERS_Ast_ప్రవాహం_DType;
    శూన్య dumpSmall(std::ostream& str) స్థిరరాశి override;
    bool hasDType() స్థిరరాశి override VL_MT_సురక్షిత { తిరిగి true; }
    bool maybe_Pointed_గమ్యం() స్థిరరాశి override VL_MT_సురక్షిత { తిరిగి true; }
    bool undead() స్థిరరాశి override { తిరిగి true; }
    Ast_కణుపు_DType* subDTypep() స్థిరరాశి override VL_MT_STABLE { తిరిగి nullptr; }
    Ast_కణుపు_DType* virtRefDTypep() స్థిరరాశి override { తిరిగి nullptr; }
    శూన్య virtRefDTypep(Ast_కణుపు_DType* nodep) override {}
    bool similar_DType_కణుపు(స్థిరరాశి Ast_కణుపు_DType* samep) స్థిరరాశి override { తిరిగి this == samep; }
    AstBasicDType* basicp() స్థిరరాశి override VL_MT_STABLE { తిరిగి nullptr; }
    పూర్ణాంకం వెడల్పు_Align_బైట్లు() స్థిరరాశి override { తిరిగి 1; }
    పూర్ణాంకం వెడల్పు_మొత్తం_బైట్లు() స్థిరరాశి override { తిరిగి 1; }
    bool isCompound() స్థిరరాశి override { తిరిగి false; }
};
class AstUnsizedArrayDType final : బహిరంగ Ast_కణుపు_DType {
    // Unsized/open-range Array data type, ie "some_dtype var_name []"
    // @astgen op1 := childDTypep : Optional[AstNodeDType] // moved to refDTypep() in V3Width
    //
    // @astgen ptr := m_refDTypep : Optional[AstNodeDType]  // Elements of this type (post-width)
బహిరంగ:
    AstUnsizedArrayDType(File_పంక్తి* fl, VFlag_బిడ్డ_DType, Ast_కణుపు_DType* dtp)
        : ASTGEN_SUPER_UnsizedArrayDType(fl) {
        బిడ్డ_DTypep(dtp);  // Only for parser
        refDTypep(nullptr);
        dtypep(nullptr);  // V3Width will resolve
    }
    AstUnsizedArrayDType(File_పంక్తి* fl, Ast_కణుపు_DType* dtp)
        : ASTGEN_SUPER_UnsizedArrayDType(fl) {
        refDTypep(dtp);
        dtypep(nullptr);  // V3Width will resolve
    }
    ASTGEN_MEMBERS_AstUnsizedArrayDType;
    స్థిరరాశి అక్షరం* broken() స్థిరరాశి override {
        BROKEN_RTN(!((m_refDTypep && !బిడ్డ_DTypep()) || (!m_refDTypep && బిడ్డ_DTypep())));
        తిరిగి nullptr;
    }
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* samep) స్థిరరాశి override;
    bool similar_DType_కణుపు(స్థిరరాశి Ast_కణుపు_DType* samep) స్థిరరాశి override;
    శూన్య dumpSmall(std::ostream& str) స్థిరరాశి override;
    Ast_కణుపు_DType* get_బిడ్డ_DTypep() స్థిరరాశి override { తిరిగి బిడ్డ_DTypep(); }
    Ast_కణుపు_DType* subDTypep() స్థిరరాశి override VL_MT_STABLE {
        తిరిగి m_refDTypep ? m_refDTypep : బిడ్డ_DTypep();
    }
    శూన్య refDTypep(Ast_కణుపు_DType* nodep) { m_refDTypep = nodep; }
    Ast_కణుపు_DType* virtRefDTypep() స్థిరరాశి override { తిరిగి m_refDTypep; }
    శూన్య virtRefDTypep(Ast_కణుపు_DType* nodep) override { refDTypep(nodep); }
    // METHODS
    AstBasicDType* basicp() స్థిరరాశి override VL_MT_STABLE { తిరిగి subDTypep()->basicp(); }
    పూర్ణాంకం వెడల్పు_Align_బైట్లు() స్థిరరాశి override { తిరిగి subDTypep()->వెడల్పు_Align_బైట్లు(); }
    పూర్ణాంకం వెడల్పు_మొత్తం_బైట్లు() స్థిరరాశి override { తిరిగి subDTypep()->వెడల్పు_మొత్తం_బైట్లు(); }
    bool isCompound() స్థిరరాశి override { తిరిగి true; }
};
class AstVoidDType final : బహిరంగ Ast_కణుపు_DType {
    // For e.g. a function returning void
బహిరంగ:
    explicit AstVoidDType(File_పంక్తి* fl)
        : ASTGEN_SUPER_VoidDType(fl) {
        dtypep(this);
    }
    ASTGEN_MEMBERS_AstVoidDType;
    శూన్య dumpSmall(std::ostream& str) స్థిరరాశి override;
    bool hasDType() స్థిరరాశి override VL_MT_సురక్షిత { తిరిగి true; }
    bool maybe_Pointed_గమ్యం() స్థిరరాశి override VL_MT_సురక్షిత { తిరిగి true; }
    bool undead() స్థిరరాశి override { తిరిగి true; }
    Ast_కణుపు_DType* subDTypep() స్థిరరాశి override VL_MT_STABLE { తిరిగి nullptr; }
    Ast_కణుపు_DType* virtRefDTypep() స్థిరరాశి override { తిరిగి nullptr; }
    శూన్య virtRefDTypep(Ast_కణుపు_DType* nodep) override {}
    bool similar_DType_కణుపు(స్థిరరాశి Ast_కణుపు_DType* samep) స్థిరరాశి override { తిరిగి true; }
    AstBasicDType* basicp() స్థిరరాశి override VL_MT_STABLE { తిరిగి nullptr; }
    పూర్ణాంకం వెడల్పు_Align_బైట్లు() స్థిరరాశి override { తిరిగి 1; }
    పూర్ణాంకం వెడల్పు_మొత్తం_బైట్లు() స్థిరరాశి override { తిరిగి 1; }
    bool isCompound() స్థిరరాశి override { తిరిగి false; }
};
class AstWildcardArrayDType final : బహిరంగ Ast_కణుపు_DType {
    // Wildcard index type associative array data type, ie "some_dtype var_name [*]"
    // @astgen op1 := childDTypep : Optional[AstNodeDType] // moved to refDTypep() in V3Width
    //
    // @astgen ptr := m_refDTypep : Optional[AstNodeDType]  // Elements of this type (post-width)
బహిరంగ:
    AstWildcardArrayDType(File_పంక్తి* fl, VFlag_బిడ్డ_DType, Ast_కణుపు_DType* dtp)
        : ASTGEN_SUPER_WildcardArrayDType(fl) {
        బిడ్డ_DTypep(dtp);  // Only for parser
        refDTypep(nullptr);
        dtypep(nullptr);  // V3Width will resolve
    }
    ASTGEN_MEMBERS_AstWildcardArrayDType;
    స్థిరరాశి అక్షరం* broken() స్థిరరాశి override {
        BROKEN_RTN(!((m_refDTypep && !బిడ్డ_DTypep()) || (!m_refDTypep && బిడ్డ_DTypep())));
        తిరిగి nullptr;
    }
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* samep) స్థిరరాశి override;
    bool similar_DType_కణుపు(స్థిరరాశి Ast_కణుపు_DType* samep) స్థిరరాశి override;
    శూన్య dumpSmall(std::ostream& str) స్థిరరాశి override;
    string prettyDTypeName(bool నిండిన_2) స్థిరరాశి override;
    Ast_కణుపు_DType* get_బిడ్డ_DTypep() స్థిరరాశి override { తిరిగి బిడ్డ_DTypep(); }
    Ast_కణుపు_DType* subDTypep() స్థిరరాశి override VL_MT_STABLE {
        తిరిగి m_refDTypep ? m_refDTypep : బిడ్డ_DTypep();
    }
    శూన్య refDTypep(Ast_కణుపు_DType* nodep) { m_refDTypep = nodep; }
    Ast_కణుపు_DType* virtRefDTypep() స్థిరరాశి override { తిరిగి m_refDTypep; }
    శూన్య virtRefDTypep(Ast_కణుపు_DType* nodep) override { refDTypep(nodep); }
    // METHODS
    AstBasicDType* basicp() స్థిరరాశి override VL_MT_STABLE { తిరిగి subDTypep()->basicp(); }
    పూర్ణాంకం వెడల్పు_Align_బైట్లు() స్థిరరాశి override { తిరిగి పరిమాణం(std::map<std::string, std::string>); }
    పూర్ణాంకం వెడల్పు_మొత్తం_బైట్లు() స్థిరరాశి override { తిరిగి పరిమాణం(std::map<std::string, std::string>); }
    bool isCompound() స్థిరరాశి override { తిరిగి true; }
    bool isDynamicallySized() స్థిరరాశి override { తిరిగి true; }
};

// === AstNodeArrayDType ===
class AstPackArrayDType final : బహిరంగ Ast_కణుపు_Array_DType {
    // Packed array data type, ie "some_dtype [2:0] var_name"
బహిరంగ:
    అంతఃవరుస AstPackArrayDType(File_పంక్తి* fl, VFlag_బిడ్డ_DType, Ast_కణుపు_DType* dtp, AstRange* rangep);
    అంతఃవరుస AstPackArrayDType(File_పంక్తి* fl, Ast_కణుపు_DType* dtp, AstRange* rangep);
    ASTGEN_MEMBERS_AstPackArrayDType;
    string prettyDTypeName(bool నిండిన_2) స్థిరరాశి override;
    bool isCompound() స్థిరరాశి override { తిరిగి false; }
};
class AstUnpackArrayDType final : బహిరంగ Ast_కణుపు_Array_DType {
    // Array data type, ie "some_dtype var_name [2:0]"
    bool m_isCompound = false;  // Non-POD subDType, or parent requires compound
బహిరంగ:
    AstUnpackArrayDType(File_పంక్తి* fl, VFlag_బిడ్డ_DType, Ast_కణుపు_DType* dtp, AstRange* rangep)
        : ASTGEN_SUPER_UnpackArrayDType(fl) {
        బిడ్డ_DTypep(dtp);  // Only for parser
        this->rangep(rangep);
        refDTypep(nullptr);
        dtypep(nullptr);  // V3Width will resolve
        // For backward compatibility AstNodeArrayDType and others inherit
        // width and signing from the subDType/base type
        వెడల్పు_From_Sub(subDTypep());
    }
    AstUnpackArrayDType(File_పంక్తి* fl, Ast_కణుపు_DType* dtp, AstRange* rangep)
        : ASTGEN_SUPER_UnpackArrayDType(fl) {
        this->rangep(rangep);
        refDTypep(dtp);
        dtypep(this);
        // For backward compatibility AstNodeArrayDType and others inherit
        // width and signing from the subDType/base type
        వెడల్పు_From_Sub(subDTypep());
    }
    ASTGEN_MEMBERS_AstUnpackArrayDType;
    శూన్య dump(std::ostream& str) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str) స్థిరరాశి override;
    string prettyDTypeName(bool నిండిన_2) స్థిరరాశి override;
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* samep) స్థిరరాశి override {
        స్థిరరాశి AstUnpackArrayDType* స్థిరరాశి sp = VN_DBG_AS(samep, UnpackArrayDType);
        తిరిగి m_isCompound == sp->m_isCompound;
    }
    bool similar_DType_కణుపు(స్థిరరాశి Ast_కణుపు_DType* samep) స్థిరరాశి override;
    bool isAggregateType() స్థిరరాశి override { తిరిగి true; }
    // True iff 'otherp' is an array of the same shape, with
    // at least one dimension reversed ascending/descending.
    bool is_తిరగేయి_యొక్క(స్థిరరాశి Ast_కణుపు_DType* otherp) స్థిరరాశి;
    // Outer dimension comes first. The first element is this node.
    std::సదిశ<AstUnpackArrayDType*> unpackDimensions();
    శూన్య isCompound(bool flag) { m_isCompound = flag; }
    bool isCompound() స్థిరరాశి override VL_MT_సురక్షిత { తిరిగి m_isCompound; }
    bool isIntegralOrPacked() స్థిరరాశి override { తిరిగి false; }
};

// === AstNodeUOrStructDType ===
class AstStructDType final : బహిరంగ Ast_కణుపు_UOr_Struct_DType {
బహిరంగ:
    // VSigning below is mispurposed to indicate if packed or not
    AstStructDType(File_పంక్తి* fl, VSigning numericUnpack)
        : ASTGEN_SUPER_StructDType(fl, numericUnpack) {}
    ASTGEN_MEMBERS_AstStructDType;
    string verilogKwd() స్థిరరాశి override { తిరిగి "struct"; }
};
class AstUnionDType final : బహిరంగ Ast_కణుపు_UOr_Struct_DType {
    bool m_isSoft;  // Is a "union soft"
    bool m_isTagged;  // Is a "union tagged"

బహిరంగ:
    // VSigning below is mispurposed to indicate if packed or not
    // isSoft implies packed
    AstUnionDType(File_పంక్తి* fl, bool isSoft, bool isTagged, VSigning numericUnpack)
        : ASTGEN_SUPER_UnionDType(fl, numericUnpack)
        , m_isSoft{isSoft}
        , m_isTagged{isTagged} {
        packed(packed() | m_isSoft);
    }
    ASTGEN_MEMBERS_AstUnionDType;
    string verilogKwd() స్థిరరాశి override { తిరిగి "union"; }
    bool isSoft() స్థిరరాశి { తిరిగి m_isSoft; }
    bool isTagged() స్థిరరాశి { తిరిగి m_isTagged; }
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* samep) స్థిరరాశి override;
    శూన్య dump(std::ostream& str) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str) స్థిరరాశి override;
};

#అయితే_ముగింపు  // Guard
