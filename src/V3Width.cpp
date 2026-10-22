// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Expression width calculations
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
// V3Width's Transformations:
//      Top down traversal:
//          Determine width of sub-expressions
//              width() = # bits upper expression wants, 0 for anything-goes
//              widthUnsized() = # bits for unsized constant, or 0 if it's sized
//              widthMin() = Alternative acceptable width for linting, or width() if sized
//              Determine this subop's width, can be either:
//                  Fixed width X
//                  Unsized, min width X   ('d5 is unsized, min 3 bits.)
//              Pass up:
//                  width() = # bits this expression generates
//                  widthSized() = true if all constants sized, else false
//          Compute size of this expression
//          Lint warn about mismatches
//              If expr size != subop fixed, bad
//              If expr size  < subop unsized minimum, bad
//              If expr size != subop, edit netlist
//                      For == and similar ops, if multibit underneath, add a REDOR
//                      If subop larger, add a EXTRACT
//                      If subop smaller, add a EXTEND
//          Pass size to sub-expressions if required (+/-* etc)
//              FINAL = true.
//              Subexpressions lint and extend as needed
//
//*************************************************************************
// Signedness depends on:
//      Decimal numbers are signed
//      Based numbers are unsigned unless 's' prefix
//      Comparison results are unsigned
//      Bit&Part selects are unsigned, even if whole
//      Concatenates are unsigned
//      Ignore signedness of self-determined:
//              shift rhs, ** rhs, x?: lhs, concat and replicate members
//      Else, if any operand unsigned, output unsigned
//
// Real number rules:
//      Real numbers are real (duh)
//      Reals convert to integers by rounding
//      Reals init to 0.0
//      Logicals convert compared to zero
//      If any operand is real, result is real
//*************************************************************************
// V3Width is the only visitor that uses vup.  We could switch to using userp,
// though note some iterators operate on next() and so would need to pass the
// same value on each nextp().
//*************************************************************************
// See notes in internal.txt about misuse of iterateAndNext and use of
// iterateSubtreeReturnEdits.
//*************************************************************************

#અન્તર્ભાવ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#અન્તર્ભાવ "V3Width.h"

#અન્તર્ભાવ "V3Assert.h"
#અન્તર્ભાવ "V3Ast.h"
#અન્તર્ભાવ "V3Begin.h"
#અન્તર્ભાવ "V3Const.h"
#અન્તર્ભાવ "V3ConstPool.h"
#અન્તર્ભાવ "V3Error.h"
#અન્તર્ભાવ "V3Global.h"
#અન્તર્ભાવ "V3LinkLValue.h"
#અન્તર્ભાવ "V3MemberMap.h"
#અન્તર્ભાવ "V3Number.h"
#અન્તર્ભાવ "V3Randomize.h"
#અન્તર્ભાવ "V3String.h"
#અન્તર્ભાવ "V3Task.h"
#અન્તર્ભાવ "V3UniqueNames.h"
#અન્તર્ભાવ "V3WidthCommit.h"

// More code; this file was getting too large; see actions there
#define VERILATOR_V3WIDTH_CPP_
#અન્તર્ભાવ "V3WidthRemove.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

વર્ગ ContainingClassFinder final {
    // The cache stores the class containing each node, excluding the node itself when it is a
    // class. This lets recursion through preceding siblings reuse the same ownership result.
    // Outside of V3Width, where the whole netlist is iterated, instead of this technique,
    // parents should be remembered during the top-down visit recursion into classes.
    std::unordered_map<અચલ Ast_ગાંઠ*, AstClass*> m_cache;

    AstClass* findCached(Ast_ગાંઠ* nodep) {
        જો (!nodep) પાછા nullptr;
        અચલ auto it = m_cache.find(nodep);
        જો (it != m_cache.end()) પાછા it->second;

        AstClass* classp = nullptr;
        જો (nodep->backp() && nodep->backp()->nextp() == nodep) {
            classp = findCached(nodep->backp());
        } નહીંતર જો (AstClass* અચલ parentp = VN_CAST(nodep->backp(), Class)) {
            classp = parentp;
        } નહીંતર જો (AstClassPackage* અચલ packagep = VN_CAST(nodep->backp(), ClassPackage)) {
            classp = packagep->classp();
        } નહીંતર {
            classp = findCached(nodep->backp());
        }
        m_cache.emplace(nodep, classp);
        પાછા classp;
    }

જાહેર:
    AstClass* find(Ast_ગાંઠ* nodep) {
        જો (AstClass* અચલ classp = VN_CAST(nodep, Class)) પાછા classp;
        જો (AstClassPackage* અચલ packagep = VN_CAST(nodep, ClassPackage)) {
            પાછા packagep->classp();
        }
        પાછા findCached(nodep);
    }
};

//######################################################################

ગણના Stage : uint8_t {
    PRELIM = 1,
    FINAL = 2,
    BOTH = 3
};  // Numbers are a bitmask <0>=prelim, <1>=final
std::ostream& operator<<(std::ostream& str, અચલ Stage& rhs) {
    પાછા str << ("-PFB"[static_cast<પૂર્ણાંક>(rhs)]);
}

ગણના Determ : uint8_t {
    SELF,  // Self-determined
    CONTEXT_DET,  // Context-determined
    ASSIGN  // Assignment-like where sign comes from RHS only
};
std::ostream& operator<<(std::ostream& str, અચલ Determ& rhs) {
    સ્થિર constexpr અચલ અક્ષર* અચલ s_det[] = {"SELF", "CNTX", "ASSN"};
    પાછા str << s_det[rhs];
}

ગણના StreamUse : uint8_t {
    STREAM_USE_NONE,  // Stream concatenation is not allowed in this context
    STREAM_USE_ASSIGN,  // Assignment-like context permits stream concatenation
    STREAM_USE_CAST,  // Explicit cast context permits stream concatenation
};

#define v3widthWarn(lhs, rhs, સંદેશ) \
    v3warnCode(((lhs) < (rhs)   ? V3ErrorCode::WIDTHTRUNC \
                : (lhs) > (rhs) ? V3ErrorCode::WIDTHEXPAND \
                                : V3ErrorCode::WIDTH), \
               સંદેશ)

//######################################################################
// Width state, as a visitor of each AstNode

વર્ગ WidthVP final {
    // Parameters to pass down hierarchy with visit functions.
    Ast_ગાંઠ_DType* અચલ m_dtypep;  // Parent's data type to resolve to
    અચલ Stage m_stage;  // If true, report errors
    અચલ StreamUse m_streamUse;  // Current expression may be a stream concat

જાહેર:
    WidthVP(Ast_ગાંઠ_DType* dtypep, Stage stage, StreamUse streamUse = STREAM_USE_NONE)
        : m_dtypep{dtypep}
        , m_stage{stage}
        , m_streamUse{streamUse} {
        // Prelim doesn't look at assignments, so shouldn't need a dtype,
        // however AstPattern uses them
    }
    WidthVP(Determ determ, Stage stage, StreamUse streamUse = STREAM_USE_NONE)
        : m_dtypep{nullptr}
        , m_stage{stage}
        , m_streamUse{streamUse} {
        જો (determ != SELF && stage != PRELIM)
            v3fatalSrc("Context-determined width request only allowed as prelim step");
    }
    WidthVP* p() { પાછા આ; }
    તાર્કિક selfDtm() અચલ { પાછા m_dtypep == nullptr; }
    StreamUse streamUse() અચલ { પાછા m_streamUse; }
    Ast_ગાંઠ_DType* dtypep() અચલ {
        // Detect where overrideDType is probably the intended call
        UASSERT(m_dtypep, "Width dtype request on self-determined or preliminary VUP");
        પાછા m_dtypep;
    }
    Ast_ગાંઠ_DType* dtypeNullp() અચલ { પાછા m_dtypep; }
    Ast_ગાંઠ_DType* dtypeNullSkipRefp() અચલ {
        Ast_ગાંઠ_DType* dtp = dtypeNullp();
        જો (dtp) dtp = dtp->skipRefp();
        પાછા dtp;
    }
    Ast_ગાંઠ_DType* dtypeOverridep(Ast_ગાંઠ_DType* અચલ defaultp) અચલ {
        UASSERT(m_stage != PRELIM, "Parent dtype should be a final-stage action");
        જો (!m_dtypep) પાછા defaultp;
        જો (m_dtypep->skipRefp()->isIntegralOrPacked()
            && defaultp->skipRefp()->isIntegralOrPacked()
            && m_dtypep->isFourstate() != defaultp->isFourstate()) {
            UASSERT_OBJ(m_dtypep->skipRefp()->basicp() && defaultp->skipRefp()->basicp(), m_dtypep,
                        "Unexpected type override from: " << defaultp << " to: " << m_dtypep);
            પાછા defaultp->findBitOrLogicDType(m_dtypep->width(), m_dtypep->widthMin(),
                                                 m_dtypep->numeric(), defaultp->isFourstate());
        }
        પાછા m_dtypep;
    }
    પૂર્ણાંક width() અચલ {
        UASSERT(m_dtypep, "Width request on self-determined or preliminary VUP");
        પાછા m_dtypep->width();
    }
    પૂર્ણાંક widthMin() અચલ {
        UASSERT(m_dtypep, "Width request on self-determined or preliminary VUP");
        પાછા m_dtypep->widthMin();
    }
    તાર્કિક prelim() અચલ { પાછા m_stage & PRELIM; }
    તાર્કિક final() અચલ { પાછા m_stage & FINAL; }
    શૂન્ય dump(std::ostream& str) અચલ {
        જો (!m_dtypep) {
            str << "  VUP(s=" << m_stage << ",self)";
        } નહીંતર {
            str << "  VUP(s=" << m_stage << ",dt=" << cvtToHex(dtypep());
            dtypep()->dumpSmall(str);
            str << ")";
        }
    }
};
std::ostream& operator<<(std::ostream& str, અચલ WidthVP* vup) {
    જો (vup) vup->dump(str);
    પાછા str;
}

//######################################################################

વર્ગ WidthClearVisitor final {
    // Rather than a VNVisitor, can just quickly touch every node
    શૂન્ય clearWidthRecurse(Ast_ગાંઠ* nodep) {
        માટે (; nodep; nodep = nodep->nextp()) {
            nodep->didWidth(ખોટું);
            જો (Ast_ગાંઠ* અચલ refp = nodep->op1p()) clearWidthRecurse(refp);
            જો (Ast_ગાંઠ* અચલ refp = nodep->op2p()) clearWidthRecurse(refp);
            જો (Ast_ગાંઠ* અચલ refp = nodep->op3p()) clearWidthRecurse(refp);
            જો (Ast_ગાંઠ* અચલ refp = nodep->op4p()) clearWidthRecurse(refp);
        }
    }

જાહેર:
    // CONSTRUCTORS
    explicit WidthClearVisitor(AstNetlist* nodep) { clearWidthRecurse(nodep); }
    આભાસી ~WidthClearVisitor() = મૂળભૂત;
};

//######################################################################

#define accept in_Width_Visitor_use_Ast_ગાંઠ_iterate_instead_of_Ast_ગાંઠ_accept

//######################################################################

વર્ગ WidthVisitor final : જાહેર VNVisitor {
    // TYPES
    using PatVecMap = std::map<પૂર્ણાંક, AstPatMember*>;
    using DTypeMap = std::map<અચલ std::string, AstPatMember*>;

    // STATE
    V3UniqueNames m_enumTempNames;  // Captured enum expressions
    V3UniqueNames m_insideTempNames;  // For generating unique temporary variable names for
                                      // `inside` expressions
    VMemberMap m_memberMap;  // Member names cached for fast lookup
    std::unordered_map<અચલ AstModport*, std::unordered_set<string>>
        m_modportNames;  // Names listed by each modport
    V3TaskConnectState m_taskConnectState;  // State to cache V3Task::taskConnects
    WidthVP* m_vup = nullptr;  // Current node state
    તાર્કિક m_underFork = ખોટું;  // Visiting under a fork
    તાર્કિક m_underSExpr = ખોટું;  // Visiting under a sequence expression
    તાર્કિક m_underPackedArray = ખોટું;  // Visiting under a AstPackArrayDType
    તાર્કિક m_underMemberSel = ખોટું;  // Viting under a MemberSel
    Ast_ગાંઠ* m_seqUnsupp = nullptr;  // Property has unsupported node
    તાર્કિક m_hasSExpr = ખોટું;  // Property has a sequence expression
    અચલ AstCell* m_cellp = nullptr;  // Current cell for arrayed instantiations
    std::vector<AstVar*> m_splitIfaceArrayVarps;  // Interface arrays split by V3Param
    અચલ AstEnumItem* m_enumItemp = nullptr;  // Current enum item
    Ast_ગાંઠ_FTask* m_ftaskp = nullptr;  // Current function/task
    Ast_ગાંઠ_Module* m_modep = nullptr;  // Current module
    અચલ AstConstraint* m_constraintp = nullptr;  // Current constraint
    Ast_ગાંઠ_Procedure* m_procedurep = nullptr;  // Current final/always
    અચલ AstWith* m_withp = nullptr;  // Current 'with' statement
    અચલ AstFunc* m_funcp = nullptr;  // Current function
    અચલ AstAttrOf* m_attrp = nullptr;  // Current attribute
    અચલ Ast_ગાંઠ_પદાવલિ* m_randomizeFromp = nullptr;  // Current randomize method call fromp
    અચલ તાર્કિક m_paramsOnly;  // Computing parameter value; limit operation
    અચલ તાર્કિક m_doGenerate;  // Do errors later inside generate statement
    તાર્કિક m_streamConcat = ખોટું;  // True if visiting arguments of stream concatenation
    તાર્કિક m_inParamOverride = ખોટું;  // True if visiting the value of a parameter override pin
    // Created dimension and enum tables, mapped to a reference reading the table (always cloned)
    std::map<std::pair<અચલ Ast_ગાંઠ_DType*, VAttrType>, AstVarRef*> m_tableMap;
    // Queues with given index type
    std::map<અચલ Ast_ગાંઠ_DType*, AstQueueDType*> m_queueDTypeIndexed;
    ContainingClassFinder m_containingClassFinder;
    std::unordered_set<AstVar*> m_aliasedVars;  // Variables referenced in alias
    std::unordered_set<અચલ AstVar*> m_curModVars;  // Variables declared in current module

    સ્થિર constexpr પૂર્ણાંક ENUM_LOOKUP_BITS = 16;  // Maximum # bits to make enum lookup table

    // ENUMS
    ગણના ExtendRule : uint8_t {
        EXTEND_EXP,  // Extend if expect sign and node signed, e.g. node=y in ADD(x,y), "x + y"
        EXTEND_ZERO,  // Extend with zeros. e.g. node=y in EQ(x,y), "x == y"
        EXTEND_LHS,  // Extend with sign if node signed. e.g. node=y in ASSIGN(y,x), "x = y"
        EXTEND_OFF  // No extension
    };
    ગણના વર્ગ ComparisonType : uint8_t {
        NORMAL_EQ,  // Conventional comparison equal/unequal operator: ==, !=
        NORMAL_GL,  // Conventional comparison grater/less than: <, >, <=, >=
        CASE,  // Four-state aware comparison: ===, !==
        WILD,  // Wildcard equality operators: ?== ?=!
    };

    સ્થિર StreamUse streamUseForDeterm(Determ determ) {
        પાછા determ == ASSIGN ? STREAM_USE_ASSIGN : STREAM_USE_NONE;
    }

    StreamUse currentStreamUse() અચલ { પાછા m_vup ? m_vup->streamUse() : STREAM_USE_NONE; }

    સ્થિર શૂન્ય packIfUnpacked(Ast_ગાંઠ_પદાવલિ* અચલ nodep) {
        જો (AstUnpackArrayDType* અચલ unpackDTypep = VN_CAST(nodep->dtypep(), UnpackArrayDType)) {
            અચલ પૂર્ણાંક elements_અંક = unpackDTypep->arrayUnpackedElements();
            અચલ પૂર્ણાંક unpackMinBits = elements_અંક * unpackDTypep->subDTypep()->widthMin();
            અચલ પૂર્ણાંક unpackBits = elements_અંક * unpackDTypep->subDTypep()->width();
            VNRelinker relinker;
            nodep->unlinkFrBack(&relinker);
            relinker.relink(નવું AstCvtArrayToPacked{
                nodep->fileline(), nodep,
                nodep->findBitOrLogicDType(unpackBits, unpackMinBits, VSigning::UNSIGNED,
                                           nodep->dtypep()->isFourstate())});
        }
    }
    // When fromp() is a DType (e.g. unlinked RefDType), resolve through
    // the ref chain; when it's an expression, dtypep() is already resolved.
    સ્થિર Ast_ગાંઠ_DType* fromDTypep(Ast_ગાંઠ* fromp) {
        જો (Ast_ગાંઠ_DType* અચલ dtypep = VN_CAST(fromp, ગાંઠ_DType))
            પાછા dtypep->skipRefOrNullp();
        પાછા fromp ? fromp->dtypep() : nullptr;
    }
    // VISITORS
    //   Naming:  width_O{outputtype}_L{lhstype}_R{rhstype}_W{widthing}_S{signing}
    //          Where type:
    //                  _O1=boolean (width 1 unsigned)
    //                  _Ou=unsigned
    //                  _Os=signed
    //                  _Ous=unsigned or signed
    //                  _Or=real
    //                  _Ox=anything

    // Widths: 1 bit out, lhs 1 bit; Real: converts via compare with 0
    શૂન્ય visit(AstLogNot* nodep) override { visit_log_not(nodep); }
    // Widths: 1 bit out, lhs 1 bit, rhs 1 bit; Real: converts via compare with 0
    શૂન્ય visit(AstLogAnd* nodep) override { visit_log_and_or(nodep); }
    શૂન્ય visit(AstLogOr* nodep) override { visit_log_and_or(nodep); }
    // Sequence and/or/intersect (IEEE 1800-2023 16.9.2), same width rules as LogAnd/LogOr
    શૂન્ય visit(AstSAnd* nodep) override { visit_log_and_or(nodep); }
    શૂન્ય visit(AstSIntersect* nodep) override { visit_log_and_or(nodep); }
    શૂન્ય visit(AstSOr* nodep) override { visit_log_and_or(nodep); }
    શૂન્ય visit(AstSWithin* nodep) override { visit_log_and_or(nodep); }
    શૂન્ય visit(AstLogEq* nodep) override {
        // Conversion from real not in IEEE, but a fallout
        visit_log_and_or(nodep);
    }
    શૂન્ય visit(AstLogIf* nodep) override {
        // Conversion from real not in IEEE, but a fallout
        visit_log_and_or(nodep);
    }

    // Widths: 1 bit out, Any width lhs
    શૂન્ય visit(AstRedAnd* nodep) override { visit_red_and_or(nodep); }
    શૂન્ય visit(AstRedOr* nodep) override { visit_red_and_or(nodep); }
    શૂન્ય visit(AstRedXor* nodep) override { visit_red_and_or(nodep); }
    શૂન્ય visit(AstOneHot* nodep) override { visit_red_and_or(nodep); }
    શૂન્ય visit(AstOneHot0* nodep) override { visit_red_and_or(nodep); }
    શૂન્ય visit(AstIsUnknown* nodep) override {
        visit_red_unknown(nodep);  // Allow real
    }

    // These have different node types, as they operate differently
    // Must add to case statement below,
    // Widths: 1 bit out, lhs width == rhs width.  real if lhs|rhs real
    શૂન્ય visit(AstEq* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::NORMAL_EQ); }
    શૂન્ય visit(AstNeq* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::NORMAL_EQ); }
    શૂન્ય visit(AstGt* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::NORMAL_GL); }
    શૂન્ય visit(AstGte* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::NORMAL_GL); }
    શૂન્ય visit(AstLt* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::NORMAL_GL); }
    શૂન્ય visit(AstLte* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::NORMAL_GL); }
    શૂન્ય visit(AstGtS* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::NORMAL_GL); }
    શૂન્ય visit(AstGteS* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::NORMAL_GL); }
    શૂન્ય visit(AstLtS* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::NORMAL_GL); }
    શૂન્ય visit(AstLteS* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::NORMAL_GL); }
    શૂન્ય visit(AstEqCase* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::CASE); }
    શૂન્ય visit(AstNeqCase* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::CASE); }
    // ...    These comparisons don't allow reals
    શૂન્ય visit(AstEqWild* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::WILD); }
    શૂન્ય visit(AstNeqWild* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::WILD); }
    // ...    Real compares
    શૂન્ય visit(AstEqD* nodep) override { visit_cmp_real(nodep); }
    શૂન્ય visit(AstNeqD* nodep) override { visit_cmp_real(nodep); }
    શૂન્ય visit(AstLtD* nodep) override { visit_cmp_real(nodep); }
    શૂન્ય visit(AstLteD* nodep) override { visit_cmp_real(nodep); }
    શૂન્ય visit(AstGtD* nodep) override { visit_cmp_real(nodep); }
    શૂન્ય visit(AstGteD* nodep) override { visit_cmp_real(nodep); }
    // ...    String compares
    શૂન્ય visit(AstEqN* nodep) override { visit_cmp_string(nodep); }
    શૂન્ય visit(AstNeqN* nodep) override { visit_cmp_string(nodep); }
    શૂન્ય visit(AstLtN* nodep) override { visit_cmp_string(nodep); }
    શૂન્ય visit(AstLteN* nodep) override { visit_cmp_string(nodep); }
    શૂન્ય visit(AstGtN* nodep) override { visit_cmp_string(nodep); }
    શૂન્ય visit(AstGteN* nodep) override { visit_cmp_string(nodep); }
    // ...    Data type compares
    શૂન્ય visit(AstEqT* nodep) override { visit_cmp_type(nodep); }
    શૂન્ય visit(AstNeqT* nodep) override { visit_cmp_type(nodep); }

    // Widths: out width = lhs width = rhs width
    // Signed: Output signed iff LHS & RHS signed.
    // Real: Not allowed
    શૂન્ય visit(AstAnd* nodep) override { visit_boolexpr_and_or(nodep); }
    શૂન્ય visit(AstOr* nodep) override { visit_boolexpr_and_or(nodep); }
    શૂન્ય visit(AstXor* nodep) override { visit_boolexpr_and_or(nodep); }
    શૂન્ય visit(AstBufIf1* nodep) override {
        visit_boolexpr_and_or(nodep);
    }  // Signed behavior changing in 3.814
    // Width: Max(Lhs,Rhs) sort of.
    // Real: If either side real
    // Signed: If both sides real
    શૂન્ય visit(AstAdd* nodep) override { visit_add_sub_replace(nodep, સાચું, ખોટું); }
    શૂન્ય visit(AstSub* nodep) override { visit_add_sub_replace(nodep, સાચું, ખોટું); }
    શૂન્ય visit(AstDiv* nodep) override { visit_add_sub_replace(nodep, સાચું, સાચું); }
    શૂન્ય visit(AstMul* nodep) override { visit_add_sub_replace(nodep, સાચું, ખોટું); }
    // These can't promote to real
    શૂન્ય visit(AstModDiv* nodep) override { visit_add_sub_replace(nodep, ખોટું, સાચું); }
    શૂન્ય visit(AstModDivS* nodep) override { visit_add_sub_replace(nodep, ખોટું, સાચું); }
    શૂન્ય visit(AstMulS* nodep) override { visit_add_sub_replace(nodep, ખોટું, ખોટું); }
    શૂન્ય visit(AstDivS* nodep) override { visit_add_sub_replace(nodep, ખોટું, સાચું); }
    // Widths: out width = lhs width, but upper matters
    // Signed: Output signed iff LHS signed; unary operator
    // Unary promote to real
    શૂન્ય visit(AstNegate* nodep) override { visit_negate_not(nodep, સાચું); }
    // Unary never real
    શૂન્ય visit(AstNot* nodep) override { visit_negate_not(nodep, ખોટું); }

    // Real: inputs and output real
    શૂન્ય visit(AstAddD* nodep) override { visit_real_add_sub(nodep); }
    શૂન્ય visit(AstSubD* nodep) override { visit_real_add_sub(nodep); }
    શૂન્ય visit(AstDivD* nodep) override { visit_real_add_sub(nodep); }
    શૂન્ય visit(AstMulD* nodep) override { visit_real_add_sub(nodep); }
    શૂન્ય visit(AstPowD* nodep) override { visit_real_add_sub(nodep); }
    શૂન્ય visit(Ast_ગાંઠ_System_Biop_D* nodep) override { visit_real_add_sub(nodep); }
    // Real: Output real
    શૂન્ય visit(AstNegateD* nodep) override { visit_real_neg_ceil(nodep); }
    શૂન્ય visit(Ast_ગાંઠ_System_Uniop_D* nodep) override { visit_real_neg_ceil(nodep); }

    // Widths: out signed/unsigned width = lhs width, input un|signed
    શૂન્ય visit(AstSigned* nodep) override { visit_signed_unsigned(nodep, VSigning::SIGNED); }
    શૂન્ય visit(AstUnsigned* nodep) override { visit_signed_unsigned(nodep, VSigning::UNSIGNED); }

    // Widths: Output width from lhs, rhs<33 bits
    // Signed: If lhs signed
    શૂન્ય visit(AstShiftL* nodep) override { visit_shift(nodep); }
    શૂન્ય visit(AstShiftR* nodep) override { visit_shift(nodep); }
    // ShiftRS converts to ShiftR, but not vice-versa
    શૂન્ય visit(AstShiftRS* nodep) override { visit_shift(nodep); }

    //========
    // Widths: Output real, input integer signed
    શૂન્ય visit(AstBitsToRealD* nodep) override { visit_Or_Lu64(nodep); }

    // Widths: Output integer signed, input real
    શૂન્ય visit(AstRToIS* nodep) override { visit_Os32_Lr(nodep); }
    શૂન્ય visit(AstRToIRoundS* nodep) override {
        // Only created here, size comes from upper expression
        assert_At_પદાવલિ(nodep);
        જો (m_vup->prelim()) {  // First stage evaluation
            iterateCheckReal(nodep, "LHS", nodep->lhsp(), BOTH);
        }
        UASSERT_OBJ(nodep->dtypep()->widthSized(), nodep, "RToIRoundS should be presized");
    }

    // Widths: Output integer unsigned, input real
    શૂન્ય visit(AstRealToBits* nodep) override { visit_Ou64_Lr(nodep); }

    // Output integer, input string
    શૂન્ય visit(AstLenN* nodep) override {
        // Widths: 32 bit out
        UASSERT_OBJ(nodep->lhsp(), nodep, "For unary ops only!");
        જો (m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            nodep->dtypeSetInt();
        }
    }
    શૂન્ય visit(AstPutcN* nodep) override {
        // CALLER: str.putc()
        UASSERT_OBJ(nodep->rhsp() && nodep->thsp(), nodep, "For ternary ops only!");
        જો (m_vup && m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckSigned32(nodep, "RHS", nodep->rhsp(), BOTH);
            iterateCheckSigned8(nodep, "THS", nodep->thsp(), BOTH);
            nodep->dtypeSetString();  // AstPutcN returns the new string to be assigned by
                                      // AstAssign
        }
    }
    શૂન્ય visit(AstGetcN* nodep) override {
        // CALLER: str.getc()
        UASSERT_OBJ(nodep->rhsp(), nodep, "For binary ops only!");
        જો (m_vup && m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckSigned32(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtypeSetBitSized(8, VSigning::UNSIGNED);
        }
    }
    શૂન્ય visit(AstGetcRefN* nodep) override {
        // CALLER: str.getc()
        UASSERT_OBJ(nodep->rhsp(), nodep, "For binary ops only!");
        જો (m_vup && m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckSigned32(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtypeSetBitSized(8, VSigning::UNSIGNED);
        }
    }
    શૂન્ય visit(AstSubstrN* nodep) override {
        // CALLER: str.substr()
        UASSERT_OBJ(nodep->rhsp() && nodep->thsp(), nodep, "For ternary ops only!");
        જો (m_vup && m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckSigned32(nodep, "RHS", nodep->rhsp(), BOTH);
            iterateCheckSigned32(nodep, "THS", nodep->thsp(), BOTH);
            nodep->dtypeSetString();
        }
    }
    શૂન્ય visit(AstCompareNN* nodep) override {
        // CALLER: str.compare(), str.icompare()
        // Widths: 32 bit out
        UASSERT_OBJ(nodep->rhsp(), nodep, "For binary ops only!");
        assert_At_પદાવલિ(nodep);
        જો (m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckString(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtypeSetInt();
        }
    }
    શૂન્ય visit(AstAtoN* nodep) override {
        // CALLER: str.atobin(), atoi(), atohex(), atooct(), atoreal()
        // Width: 64bit floating point for atoreal(), 32bit out for the others
        assert_At_પદાવલિ(nodep);
        જો (m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            જો (nodep->fmtType() == AstAtoN::ATOREAL) {
                nodep->dtypeSetDouble();
            } નહીંતર {
                nodep->dtypeSetInteger();
            }
        }
    }
    શૂન્ય visit(AstNToI* nodep) override {
        // Created here, should be already sized
        assert_At_પદાવલિ(nodep);
        જો (m_vup->prelim()) {
            UASSERT_OBJ(nodep->dtypep(), nodep, "NToI should be sized when created");
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
        }
    }

    // Widths: Constant, terminal
    શૂન્ય visit(AstTime* nodep) override { nodep->dtypeSetTime2State(); }
    શૂન્ય visit(AstTimeD* nodep) override { nodep->dtypeSetDouble(); }
    શૂન્ય visit(AstTimePrecision* nodep) override { nodep->dtypeSetInteger2State(); }
    શૂન્ય visit(AstGetInitialRandomSeed* nodep) override { nodep->dtypeSetInt(); }
    શૂન્ય visit(AstTimeUnit* nodep) override {
        nodep->replaceWith(
            નવું AstConst{nodep->fileline(), AstConst::Signed32{}, nodep->timeunit().powerOfTen()});
        VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
    }
    શૂન્ય visit(AstScopeName* nodep) override {
        nodep->dtypeSetUInt64();  // A pointer, but not that it matters
    }

    // Whether nodep, an override's value, is an untyped pattern, or a conditional of such
    સ્થિર તાર્કિક isOverridePattern(અચલ Ast_ગાંઠ* nodep) {
        જો (અચલ AstPattern* અચલ patternp = VN_CAST(nodep, Pattern)) {
            પાછા !patternp->childDTypep();
        }
        અચલ AstCond* અચલ condp = VN_CAST(nodep, Cond);
        પાછા condp && isOverridePattern(condp->thenp()) && isOverridePattern(condp->elsep());
    }
    શૂન્ય visit(AstCond* nodep) override {
        // op = cond ? expr1 : expr2
        // See IEEE-2012 11.4.11 and Table 11-21.
        //   LHS is self-determined
        //   Width: max(RHS, THS)
        //   Signed: Output signed iff RHS & THS signed  (presumed, not in IEEE)
        //   Real: Output real if either expression is real, non-real argument gets converted
        assert_At_પદાવલિ(nodep);
        જો (m_inParamOverride) {
            // Choosing between override patterns, so type and fold only the condition, then
            // keep the pattern it picks, which the specialized module types as a lone one
            અચલ પૂર્ણાંક errors = V3Error::error_ગણતરી();
            {
                VL_RESTORER(m_inParamOverride);
                m_inParamOverride = ખોટું;
                iterateCheckBool(nodep, "Conditional Test", nodep->condp(), BOTH);
                V3Const::constifyParamsEdit(nodep->condp());  // Reports a non-constant
            }
            અચલ AstConst* અચલ constp = VN_CAST(nodep->condp(), Const);
            અચલ તાર્કિક known = constp && !constp->અંક().isFourState();
            // Folding reports a non-constant, but leaves x or z, even inside a conditional
            જો (!known && V3Error::error_ગણતરી() == errors) {
                nodep->condp()->v3warn(E_UNSUPPORTED,
                                       "Unsupported: Parameter override '?:' with assignment"
                                       " patterns and a condition without a known value.");
            }
            // After an error, keep the first pattern, so that no conditional is left untyped
            Ast_ગાંઠ_પદાવલિ* અચલ keepp
                = (known && constp->isZero() ? nodep->elsep() : nodep->thenp())->unlinkFrBack();
            nodep->replaceWith(keepp);
            VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
            userIterate(keepp, WidthVP{SELF, BOTH}.p());
            પાછા;
        }
        જો (m_vup->prelim()) {  // First stage evaluation
            // Just once, do the conditional, expect one bit out.
            iterateCheckBool(nodep, "Conditional Test", nodep->condp(), BOTH);
            // Determine sub expression widths only relying on what's in the subops
            //  CONTEXT_DET determined, but need data type for pattern assignments
            userIterateAndNext(nodep->thenp(), WidthVP{m_vup->dtypeNullp(), PRELIM}.p());
            userIterateAndNext(nodep->elsep(), WidthVP{m_vup->dtypeNullp(), PRELIM}.p());
            // Calculate width of this expression.
            // First call (prelim()) m_vup->width() is probably zero, so we'll return
            //  the size of this subexpression only.
            // Second call (final()) m_vup->width() is probably the expression size, so
            //  the expression includes the size of the output too.
            Ast_ગાંઠ_DType* અચલ thenDTypep = nodep->thenp()->dtypep();
            Ast_ગાંઠ_DType* અચલ elseDTypep = nodep->elsep()->dtypep();
            Ast_ગાંઠ_DType* અચલ thenDTypeSkippedp = thenDTypep->skipRefp();
            Ast_ગાંઠ_DType* અચલ elseDTypeSkippedp = elseDTypep->skipRefp();
            જો (nodep->thenp()->isNull() && nodep->elsep()->isNull()) {
                nodep->dtypep(m_vup->dtypeNullp() ? m_vup->dtypeNullp() : nodep->findNullDType());
            } નહીંતર જો (nodep->thenp()->is_Class_Handle_કિંમત()
                       || nodep->elsep()->is_Class_Handle_કિંમત()) {
                Ast_ગાંઠ_DType* commonClassTypep = nullptr;
                જો (nodep->thenp()->is_Class_Handle_કિંમત() && nodep->elsep()->is_Class_Handle_કિંમત()) {
                    // Get the most-deriving class type that both arguments can be casted to.
                    commonClassTypep
                        = Ast_ગાંઠ::getCommonClassTypep(nodep->thenp(), nodep->elsep());
                }
                જો (commonClassTypep) {
                    nodep->dtypep(commonClassTypep);
                } નહીંતર {
                    nodep->v3error("Incompatible types of operands of condition operator: "
                                   << thenDTypep->prettyTypeName() << " and "
                                   << elseDTypep->prettyTypeName());
                    nodep->dtypeFrom(thenDTypep);
                }
            } નહીંતર જો (!((thenDTypeSkippedp->isIntegralOrPacked()
                          || VN_IS(thenDTypeSkippedp, BasicDType))
                         && (elseDTypeSkippedp->isIntegralOrPacked()
                             || VN_IS(elseDTypeSkippedp, BasicDType)))) {
                જો (Ast_ગાંઠ::computeCastable(elseDTypep, thenDTypep, nullptr).isAssignable()) {
                    nodep->dtypep(elseDTypep);
                } નહીંતર {
                    // Set any type even if not castable
                    nodep->dtypep(thenDTypep);
                    જો (!Ast_ગાંઠ::computeCastable(thenDTypep, elseDTypep, nullptr)
                             .isAssignable()) {
                        nodep->v3error("Neither " << thenDTypep->prettyDTypeNameQ() << " or "
                                                  << elseDTypep->prettyDTypeNameQ()
                                                  << " is castable to the other one");
                    }
                }
            } નહીંતર જો (thenDTypep->isDouble() || elseDTypep->isDouble()) {
                nodep->dtypeSetDouble();
            } નહીંતર જો (thenDTypep->isString() || elseDTypep->isString()) {
                // There is a specific case:
                // strings and how they should be handled when condp() is 'x
                // ('x ? "Option1" : "Option2") === "Option0"
                // ('x ? "Option1" : "Option2") ===
                // 56'b010011110111000001110100011010010110111101101110001100xx
                // Issue is that the type of this is still string but the value contains 'x - types
                // and domains are not the same thing in SystemVerilog but in Verilator they are
                nodep->dtypeSetString();
            } નહીંતર {
                અચલ તાર્કિક isAnyResultFourstate
                    = thenDTypeSkippedp->isFourstate() || elseDTypeSkippedp->isFourstate();
                તાર્કિક handled = ખોટું;
                અચલ તાર્કિક isCondpFourstate = nodep->condp()->isFourstateOrNoDtype();
                Ast_ગાંઠ_DType* અચલ thenEnumDTypep = thenDTypep->skipRefToEnump();
                જો (VN_IS(thenDTypeSkippedp, ગાંઠ_UOr_Struct_DType)
                    && thenDTypeSkippedp->same_વૃક્ષ(elseDTypeSkippedp)) {
                    nodep->dtypep(thenDTypep);
                    handled = સાચું;
                } નહીંતર જો (VN_IS(thenEnumDTypep, EnumDType)
                           && thenEnumDTypep->same_વૃક્ષ(elseDTypep->skipRefToEnump())) {
                    // if enum is two-state we have to check condp() - because if it is
                    // four-state the result must be four-state as well
                    જો (isAnyResultFourstate || !isCondpFourstate) {
                        nodep->dtypep(thenEnumDTypep);
                        handled = સાચું;
                    } નહીંતર {
                        // This is similar case to what happens with strings
                        // So, to avoid cases where:
                        // enum int {A, B} type_t;
                        // logic'(foo) ? A : B
                        // throws an ENUMVALUE error for this error is disabled for this expression
                        // This is a result of how Verilator's type system works - to avoid such
                        // situations Verilator would need to either treat everything as four-state
                        // or separate idea of type and values domain - since four-state values are
                        // casted to two-state only with explicit casts or with assignments
                        nodep->fileline(નવું File_લીટી{nodep->fileline()});
                        nodep->fileline()->warnOff(V3ErrorCode::ENUMVALUE, સાચું);
                    }
                }
                જો (!handled) {
                    અચલ પૂર્ણાંક width = std::max(thenDTypep->width(), elseDTypep->width());
                    અચલ પૂર્ણાંક widthMin = std::max(thenDTypep->widthMin(), elseDTypep->widthMin());
                    અચલ VSigning numeric
                        = VSigning::fromBool(thenDTypep->isSigned() && elseDTypep->isSigned());
                    અચલ તાર્કિક isFourstate = isAnyResultFourstate || isCondpFourstate;
                    // Try to preserve a dtype
                    જો ((width == elseDTypeSkippedp->width()
                         && widthMin == elseDTypeSkippedp->widthMin())
                        && elseDTypeSkippedp->numeric() == numeric
                        && isFourstate == elseDTypeSkippedp->isFourstate()) {
                        nodep->dtypep(elseDTypep);
                    } નહીંતર જો ((width == thenDTypeSkippedp->width()
                                && widthMin == thenDTypeSkippedp->widthMin())
                               && thenDTypeSkippedp->numeric() == numeric
                               && isFourstate == thenDTypeSkippedp->isFourstate()) {
                        nodep->dtypep(thenDTypep);
                    } નહીંતર {
                        // None of the children dtypes match generate one
                        nodep->dtypeSetBitOrLogicUnsized(width, widthMin, numeric, isFourstate);
                    }
                }
            }
        }
        જો (m_vup->final()) {
            Ast_ગાંઠ_DType* અચલ expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
            Ast_ગાંઠ_DType* અચલ subDTypep = expDTypep;
            nodep->dtypep(expDTypep);
            // Error report and change sizes for suboperands of this node.
            iterateCheck(nodep, "Conditional True", nodep->thenp(), CONTEXT_DET, FINAL, subDTypep,
                         EXTEND_EXP);
            iterateCheck(nodep, "Conditional False", nodep->elsep(), CONTEXT_DET, FINAL, subDTypep,
                         EXTEND_EXP);
        }
    }
    શૂન્ય visit(AstConcat* nodep) override {
        // Real: Not allowed (assumed)
        // Signed: unsigned output, input either (assumed)
        // IEEE-2012 Table 11-21, and 11.8.1:
        //   LHS, RHS is self-determined
        //   signed: Unsigned  (11.8.1)
        //   width: LHS + RHS
        Ast_ગાંઠ_DType* અચલ vdtypep = m_vup->dtypeNullSkipRefp();
        userIterate(vdtypep, WidthVP{SELF, BOTH}.p());
        // Conversions
        જો (Ast_ગાંઠ_પદાવલિ* અચલ newp = newConsDynArrayOrQueue(nodep->fileline(), vdtypep)) {
            // IEEE 1800-2023 10.10 requires looking at argument data type
            // to determine if value or push
            userIterateAndNext(nodep->lhsp(), WidthVP{vdtypep, PRELIM}.p());
            userIterateAndNext(nodep->rhsp(), WidthVP{vdtypep, PRELIM}.p());
            newp->dtypeFrom(vdtypep);
            addConsItem(newp, nodep->lhsp()->unlinkFrBack());
            addConsItem(newp, nodep->rhsp()->unlinkFrBack());
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
            પાછા;
        }
        જો (VN_IS(vdtypep, UnpackArrayDType)) {
            AstPattern* અચલ newp = નવું AstPattern{nodep->fileline(), nullptr};
            patConcatConvertRecurse(newp, nodep);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
            userIterate(newp, m_vup);
            પાછા;
        }

        // Concat handling
        જો (m_vup->prelim()) {
            જો (VN_IS(vdtypep, AssocArrayDType)  //
                || VN_IS(vdtypep, DynArrayDType)  //
                || VN_IS(vdtypep, QueueDType)) {
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: Concatenation to form "
                                                 << vdtypep->prettyDTypeNameQ() << " data type");
            }

            જો (vdtypep && vdtypep->isString()) {
                iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
                iterateCheckString(nodep, "RHS", nodep->rhsp(), BOTH);
                nodep->dtypeSetString();
            } નહીંતર {
                iterateCheckIntegralSelf(nodep, "LHS", nodep->lhsp(), SELF, BOTH);
                iterateCheckIntegralSelf(nodep, "RHS", nodep->rhsp(), SELF, BOTH);

                જો (m_streamConcat) {
                    packIfUnpacked(nodep->lhsp());
                    packIfUnpacked(nodep->rhsp());
                }
                nodep->dtypeSetBitOrLogicUnsized(
                    nodep->lhsp()->width() + nodep->rhsp()->width(),
                    nodep->lhsp()->widthMin() + nodep->rhsp()->widthMin(), VSigning::UNSIGNED,
                    Ast_ગાંઠ_પદાવલિ::isFourstateOrNoDtype(nodep->lhsp(), nodep->rhsp()));
            }
            // Cleanup zero width Verilog2001 {x,{0{foo}}} now,
            // otherwise having width(0) will cause later assertions to fire
            જો (અચલ AstReplicate* અચલ repp = VN_CAST(nodep->lhsp(), Replicate)) {
                જો (repp->width() == 0) {  // Keep rhs
                    nodep->replaceWith(nodep->rhsp()->unlinkFrBack());
                    VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
                    પાછા;
                }
            }
            જો (અચલ AstReplicate* અચલ repp = VN_CAST(nodep->rhsp(), Replicate)) {
                જો (repp->width() == 0) {  // Keep lhs
                    nodep->replaceWith(nodep->lhsp()->unlinkFrBack());
                    VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
                    પાછા;
                }
            }
        }
        જો (m_vup->final()) {
            જો (nodep->lhsp()->isString() || nodep->rhsp()->isString()) {
                Ast_ગાંઠ_પદાવલિ* lhsp = nodep->lhsp()->unlinkFrBack();
                Ast_ગાંઠ_પદાવલિ* rhsp = nodep->rhsp()->unlinkFrBack();
                જો (!lhsp->isString()) lhsp = નવું AstCvtPackString{lhsp->fileline(), lhsp};
                જો (!rhsp->isString()) rhsp = નવું AstCvtPackString{rhsp->fileline(), rhsp};
                Ast_ગાંઠ* અચલ newp = નવું AstConcatN{nodep->fileline(), lhsp, rhsp};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
                પાછા;
            }
            જો (!nodep->dtypep()->widthSized()) {
                // See also error in V3Number
                ગાંઠ_For_Unsized_Warning(nodep)->v3warn(
                    WIDTHCONCAT, "Unsized numbers/parameters not allowed in concatenations.");
            }
        }
    }
    શૂન્ય visit(AstConcatN* nodep) override {
        જો (nodep->didWidth()) પાછા;
        // String concatenate.
        // Already did AstConcat simplifications
        assert_At_પદાવલિ(nodep);
        જો (m_vup->prelim()) {
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckString(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtypeSetString();
        }
        જો (m_vup->final()) {
            nodep->didWidth(સાચું);
            જો (!nodep->dtypep()->widthSized()) {
                // See also error in V3Number
                ગાંઠ_For_Unsized_Warning(nodep)->v3warn(
                    WIDTHCONCAT, "Unsized numbers/parameters not allowed in concatenations.");
            }
        }
    }
    શૂન્ય visit(AstDefaultDisable* nodep) override {
        assertAtStatement(nodep);
        // it's like an if() condition.
        iterateCheckBool(nodep, "default disable iff condition", nodep->condp(), BOTH);
    }
    શૂન્ય visit(AstDelay* nodep) override {
        જો (nodep->isCycleDelay() && m_underSExpr) {
            userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
            જો (nodep->rhsp() && !nodep->isUnbounded()) {
                userIterateAndNext(nodep->rhsp(), WidthVP{SELF, BOTH}.p());
            }
            // A delay with a formal argument of a sequence or property is checked when that is
            // instanced, with the actual argument (IEEE 1800-2023 16.8)
            જો (V3AssertCommon::refsSvaFormal(nodep->lhsp())
                || (nodep->rhsp() && V3AssertCommon::refsSvaFormal(nodep->rhsp()))) {
                પાછા;
            }
            // Fold parameterized SVA cycle-delay bounds
            V3AssertCommon::checkSvaCycleDelay(nodep, nodep);
            પાછા;
        }
        જો (Ast_ગાંઠ_પદાવલિ* અચલ fallDelayp = nodep->fallDelay()) {
            iterateCheckDelay(nodep, "delay", nodep->lhsp(), BOTH);
            iterateCheckDelay(nodep, "delay", fallDelayp, BOTH);
            પાછા;
        }
        જો (VN_IS(m_procedurep, Final)) {
            nodep->v3error("Delays are not legal in final blocks (IEEE 1800-2023 9.2.3)");
            VL_DO_DANGLING(ધકેલો_Deletep(nodep->unlinkFrBack()), nodep);
            પાછા;
        }
        જો (!m_underFork && VN_IS(m_ftaskp, Func)) {
            nodep->v3error("Delays are not legal in functions. Suggest use a task "
                           "(IEEE 1800-2023 13.4.4)");
            VL_DO_DANGLING(ધકેલો_Deletep(nodep->unlinkFrBack()), nodep);
            પાછા;
        }
        જો (nodep->fileline()->timingOn()) {
            જો (v3Global.opt.timing().isSetTrue()) {
                iterateCheckDelay(nodep, "delay", nodep->lhsp(), BOTH);
                iterateAndNextNull(nodep->stmtsp());
                પાછા;
            } નહીંતર જો (v3Global.opt.timing().isSetFalse()) {
                nodep->v3warn(STMTDLY, "Ignoring delay on this statement due to --no-timing");
            } નહીંતર {
                nodep->v3warn(
                    E_NEEDTIMINGOPT,
                    "Use --timing or --no-timing to specify how delays should be handled");
            }
        }
        જો (nodep->stmtsp()) nodep->addNextHere(nodep->stmtsp()->unlinkFrBackWithNext());
        VL_DO_DANGLING(ધકેલો_Deletep(nodep->unlinkFrBack()), nodep);
    }
    શૂન્ય visit(AstFireEvent* nodep) override {
        assertAtStatement(nodep);
        iterateCheckSelf(nodep, "LHS", nodep->operandp(), SELF, BOTH);
    }
    શૂન્ય visit(AstFork* nodep) override {
        જો (!m_underFork && VN_IS(m_ftaskp, Func) && !nodep->joinType().joinNone()) {
            nodep->v3error("Only fork .. join_none is legal in functions. "
                           "(IEEE 1800-2023 13.4.4)");
            VL_DO_DANGLING(ધકેલો_Deletep(nodep->unlinkFrBack()), nodep);
            પાછા;
        }
        જો (!nodep->fileline()->timingOn()
            // With no statements, begin is identical
            || !nodep->forksp()
            || (!v3Global.opt.timing().isSetTrue()  // If no --timing
                && (v3Global.opt.bboxUnsup()
                    // With one statement and no timing, a begin block does as good as a
                    // fork/join or join_any
                    || (!nodep->forksp()->nextp() && !nodep->joinType().joinNone())))) {
            AstBegin* અચલ newp = નવું AstBegin{nodep->fileline(), nodep->name(), nullptr, ખોટું};
            nodep->replaceWith(newp);
            જો (nodep->declsp()) newp->addDeclsp(nodep->declsp()->unlinkFrBackWithNext());
            જો (nodep->stmtsp()) newp->addStmtsp(nodep->stmtsp()->unlinkFrBackWithNext());
            જો (nodep->forksp()) newp->addStmtsp(nodep->forksp()->unlinkFrBackWithNext());
            VL_DO_DANGLING(nodep->delete_વૃક્ષ(), nodep);
        } નહીંતર જો (v3Global.opt.timing().isSetTrue()) {
            VL_RESTORER(m_underFork);
            m_underFork = સાચું;
            iterateChildren(nodep);
        } નહીંતર જો (v3Global.opt.timing().isSetFalse()) {
            nodep->v3warn(E_NOTIMING, "Fork statements require --timing");
            VL_DO_DANGLING(ધકેલો_Deletep(nodep->unlinkFrBack()), nodep);
        } નહીંતર {
            nodep->v3warn(E_NEEDTIMINGOPT,
                          "Use --timing or --no-timing to specify how forks should be handled");
        }
    }
    શૂન્ય visitWaitOrDisableFork(Ast_ગાંઠ* nodep) {
        જો (nodep->fileline()->timingOn()) {
            જો (v3Global.opt.timing().isSetFalse()) {
                nodep->v3warn(E_NOTIMING, "Support for '" << nodep->verilogKwd()
                                                          << "' statement requires --timing");
                VL_DO_DANGLING(nodep->unlinkFrBack()->delete_વૃક્ષ(), nodep);
                પાછા;
            } નહીંતર જો (!v3Global.opt.timing().isSetTrue()) {
                nodep->v3warn(E_NEEDTIMINGOPT, "Use --timing or --no-timing to specify how "
                                                   << "disable fork should be handled");
            }
        }
        iterateChildren(nodep);
    }
    શૂન્ય visit(AstDisableFork* nodep) override { visitWaitOrDisableFork(nodep); }
    શૂન્ય visit(AstWaitFork* nodep) override { visitWaitOrDisableFork(nodep); }
    શૂન્ય visit(AstToLowerN* nodep) override {
        assert_At_પદાવલિ(nodep);
        જો (m_vup->prelim()) {
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            nodep->dtypeSetString();
        }
    }
    શૂન્ય visit(AstToUpperN* nodep) override {
        assert_At_પદાવલિ(nodep);
        જો (m_vup->prelim()) {
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            nodep->dtypeSetString();
        }
    }
    શૂન્ય visit(AstReplicate* nodep) override {
        // IEEE-2012 Table 11-21:
        //   LHS, RHS is self-determined
        //   width: value(LHS) * width(RHS)
        assert_At_પદાવલિ(nodep);
        જો (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "RHS", nodep->countp(), SELF, BOTH);
            V3Const::constifyParamsNoWarnEdit(nodep->countp());  // rhsp may change

            int32_t times = 1;  // IEEE replicate value is integral

            અચલ AstConst* અચલ constp = VN_CAST(nodep->countp(), Const);
            જો (constp) {
                જો (constp->અંક().isFourState()
                    || (constp->dtypep()->isSigned() && constp->અંક().isNegative())) {
                    nodep->v3error("Replication value of < 0 or X/Z not legal"
                                   " (IEEE 1800-2023 11.4.12.1): "
                                   << constp->prettyNameQ());
                } નહીંતર {
                    times = constp->toSInt();
                }
            }

            Ast_ગાંઠ_DType* અચલ vdtypep = m_vup->dtypeNullSkipRefp();
            જો (VN_IS(vdtypep, QueueDType) || VN_IS(vdtypep, DynArrayDType)
                || VN_IS(vdtypep, UnpackArrayDType)) {
                જો (times != 1) {
                    nodep->v3warn(E_UNSUPPORTED, "Unsupported: Non-1 replication to form "
                                                     << vdtypep->prettyDTypeNameQ()
                                                     << " data type");
                }
                જો (VN_IS(nodep->srcp(), Concat)) {
                    // Convert to concat directly, and visit(AstConst) will convert.
                    // Don't iterate lhsp as SELF, the potential Concat below needs
                    // the adtypep passed down to recognize the QueueDType
                    userIterateAndNext(nodep->srcp(), WidthVP{vdtypep, BOTH}.p());
                    nodep->replaceWith(nodep->srcp()->unlinkFrBack());
                    VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
                    પાછા;
                } નહીંતર {  // int a[] = {lhs} -> same as '{lhs}
                    AstPattern* અચલ newp = નવું AstPattern{
                        nodep->fileline(),
                        નવું AstPatMember{nodep->srcp()->fileline(), nodep->srcp()->unlinkFrBack(),
                                         nullptr, nullptr}};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
                    userIterate(newp, m_vup);
                    પાછા;
                }
            }
            જો (VN_IS(vdtypep, AssocArrayDType)) {
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: Replication to form "
                                                 << vdtypep->prettyDTypeNameQ() << " data type");
            }
            જો (vdtypep && vdtypep->isString()) {
                iterateCheckString(nodep, "LHS", nodep->srcp(), BOTH);
            } નહીંતર {
                iterateCheckIntegralSelf(nodep, "LHS", nodep->srcp(), SELF, BOTH);
            }

            જો ((vdtypep && vdtypep->isString()) || nodep->srcp()->isString()) {
                Ast_ગાંઠ* અચલ newp
                    = નવું AstReplicateN{nodep->fileline(), nodep->srcp()->unlinkFrBack(),
                                        nodep->countp()->unlinkFrBack()};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
                પાછા;
            } નહીંતર {
                જો (!constp) nodep->v3error("Replication value isn't a constant.");
                જો (times == 0
                    && !VN_IS(nodep->backp(), Concat)) {  // Concat Visitor will clean it up.
                    nodep->v3warn(ZEROREPL,
                                  "Replication value of 0 is only legal under a concatenation"
                                  " (IEEE 1800-2023 11.4.12.1)");
                    times = 1;  // Set to 1, so we can continue looking for errors
                }
                nodep->dtypeSetBitOrLogicUnsized(
                    (nodep->srcp()->width() * times), (nodep->srcp()->widthMin() * times),
                    VSigning::UNSIGNED, nodep->srcp()->isFourstateOrNoDtype());
            }
        }
        જો (m_vup->final()) {
            જો (!nodep->dtypep()->widthSized()) {
                // See also error in V3Number
                ગાંઠ_For_Unsized_Warning(nodep)->v3warn(
                    WIDTHCONCAT, "Unsized numbers/parameters not allowed in replications.");
            }
        }
    }
    શૂન્ય visit(AstReplicateN* nodep) override {
        // Replicate with string
        assert_At_પદાવલિ(nodep);
        જો (m_vup->prelim()) {
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckSizedSelf(nodep, "RHS", nodep->rhsp(), SELF, BOTH);
            V3Const::constifyParamsNoWarnEdit(nodep->rhsp());  // rhsp may change
            nodep->dtypeSetString();
        }
        જો (m_vup->final()) {
            જો (!nodep->dtypep()->widthSized()) {
                // See also error in V3Number
                ગાંઠ_For_Unsized_Warning(nodep)->v3warn(
                    WIDTHCONCAT, "Unsized numbers/parameters not allowed in replications.");
            }
        }
    }
    શૂન્ય visit(Ast_ગાંઠ_Dist_Biop* nodep) override {
        assert_At_પદાવલિ(nodep);
        જો (m_vup->prelim()) {  // First stage evaluation
            iterateCheckSigned32(nodep, "seed", nodep->lhsp(), BOTH);
            iterateCheckSigned32(nodep, "RHS", nodep->rhsp(), BOTH);
        }
    }
    શૂન્ય visit(Ast_ગાંઠ_Dist_Triop* nodep) override {
        જો (m_vup->prelim()) {  // First stage evaluation
            iterateCheckSigned32(nodep, "seed", nodep->lhsp(), BOTH);
            iterateCheckSigned32(nodep, "RHS", nodep->rhsp(), BOTH);
            iterateCheckSigned32(nodep, "THS", nodep->thsp(), BOTH);
        }
    }
    તાર્કિક streamImplicitUseAllowed(અચલ Ast_ગાંઠ_Stream* nodep) અચલ {
        જો (m_streamConcat) પાછા સાચું;
        જો (m_vup && m_vup->streamUse() != STREAM_USE_NONE) પાછા સાચું;

        અચલ Ast_ગાંઠ* અચલ backp = nodep->backp();
        જો (!backp) પાછા સાચું;  // Error elsewhere

        પાછા VN_IS(backp, ગાંઠ_Assign);
    }
    શૂન્ય visit(Ast_ગાંઠ_Stream* nodep) override {
        VL_RESTORER(m_streamConcat);
        // UINFOTREE(1, nodep, "stream-in vup" << m_vup, "stream-in ");
        assert_At_પદાવલિ(nodep);
        જો (m_vup->prelim()) {
            m_streamConcat = સાચું;
            iterateCheckSelf(nodep, "LHS", nodep->lhsp(), SELF, BOTH);
            m_streamConcat = ખોટું;
            iterateCheckSelf(nodep, "RHS", nodep->rhsp(), SELF, BOTH);
            V3Const::constifyParamsEdit(nodep->rhsp());  // rhsp may change
            જો (અચલ AstConst* અચલ constp = VN_CAST(nodep->rhsp(), Const)) {
                જો (constp->toUInt() == 0) nodep->v3error("Slice size cannot be zero.");
            } નહીંતર {
                nodep->v3error("Slice size isn't a constant or basic data type.");
            }
            અચલ Ast_ગાંઠ_DType* અચલ lhsDtypep = nodep->lhsp()->dtypep()->skipRefToEnump();
            જો (VN_IS(lhsDtypep, DynArrayDType) || VN_IS(lhsDtypep, QueueDType)
                || (VN_IS(lhsDtypep, UnpackArrayDType) && lhsDtypep->isStreamableFixedAggregate())
                || (VN_IS(lhsDtypep, ગાંઠ_UOr_Struct_DType)
                    && !VN_AS(lhsDtypep, ગાંઠ_UOr_Struct_DType)->packed()
                    && lhsDtypep->isStreamableFixedAggregate())) {
                nodep->dtypeSetStream();
            } નહીંતર જો (lhsDtypep->isCompound()) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: Stream operation on a variable of a type "
                                  << lhsDtypep->prettyDTypeNameQ());
                nodep->dtypeSetStream();
            } નહીંતર {
                nodep->dtypeSetBitOrLogicUnsized(
                    nodep->lhsp()->width(), nodep->lhsp()->widthMin(), VSigning::UNSIGNED,
                    Ast_ગાંઠ_પદાવલિ::isFourstateOrNoDtype(nodep->lhsp(), nodep->rhsp()));
            }
        }
        જો (m_vup->final()) {
            જો (!streamImplicitUseAllowed(nodep)) {
                nodep->v3error(
                    "Streaming concatenation cannot be used in an implicitly cast context "
                    "(IEEE 1800-2023 11.4.14)\n"
                    << nodep->warnMore() << "... Suggest use a cast");
            }
            જો (!nodep->dtypep()->widthSized()) {
                // See also error in V3Number
                ગાંઠ_For_Unsized_Warning(nodep)->v3warn(
                    WIDTHCONCAT, "Unsized numbers/parameters not allowed in streams.");
            }
        }
    }
    શૂન્ય visit(AstRange* nodep) override {
        // Real: Not allowed
        // Signed: unsigned output, input either
        // Convert all range values to constants
        UINFO(6, "RANGE " << nodep);
        V3Const::constifyParamsEdit(nodep->leftp());  // May relink pointed to node
        V3Const::constifyParamsEdit(nodep->rightp());  // May relink pointed to node
        checkConstantOrReplace(nodep->leftp(), સાચું,
                               "left side of bit range isn't a two-state constant"
                               " (IEEE 1800-2023 6.9.1)");
        checkConstantOrReplace(nodep->rightp(), સાચું,
                               "right side of bit range isn't a two-state constant"
                               " (IEEE 1800-2023 6.9.1)");
        જો (m_vup->prelim()) {
            // Don't need to iterate because V3Const already constified
            અચલ પૂર્ણાંક width = nodep->elementsConst();
            જો (nodep->fromBracket() && nodep->ડાબું_Const() > nodep->જમણું_Const()) {
                // From a C-Style '[size]' declaration, due to range handling a '[-1]'
                // will get transformed to look like a '[0:1]', instead report as error
                // now we know the datatype is used
                nodep->v3error("Size of range is '["
                               << (-width + 2)
                               << "]', must be positive integer (IEEE 1800-2023 7.4.2)");
            } નહીંતર જો (width > (1 << 28)) {
                nodep->v3error("Width of bit range is huge; vector of over 1 billion bits: 0x"
                               << std::hex << width << std::dec);
            }
            // Note width() not set on range; use elementsConst()
            અચલ તાર્કિક inDeadModule = m_modep && m_modep->dead();
            // Suppress ASCRANGE in parameterized template modules whose parameter-dependent
            // ranges haven't been resolved yet, or when the type has no owning module
            // (e.g. moved to TypeTable during DepGraph resolution).
            અચલ તાર્કિક inParameterizedTemplate
                = m_modep && (m_modep->dead() || m_modep->parameterizedTemplate());
            અચલ તાર્કિક inTypeTable = !m_modep;
            અચલ Ast_ગાંઠ* basep = nodep->backp();
            જ્યારે (VN_IS(basep, Range)) basep = basep->backp();
            જો (nodep->ascending() && !VN_IS(basep, UnpackArrayDType)
                && !VN_IS(basep, Cell)  // For cells we warn when connecting pins
                && !m_paramsOnly  // Skip during parameter evaluation
                && !inDeadModule && !inParameterizedTemplate && !inTypeTable) {
                nodep->v3warn(ASCRANGE, "Ascending bit range vector: left < right of bit range: ["
                                            << nodep->ડાબું_Const() << ":" << nodep->જમણું_Const()
                                            << "]");
            }
        }
    }

    શૂન્ય visit(AstSel* nodep) override {
        // Signed: always unsigned; Real: Not allowed
        // LSB is self-determined (IEEE 2012 11.5.1)
        // We also use SELs to shorten a signed constant etc, in this case they are signed.
        જો (nodep->didWidth()) પાછા;
        assert_At_પદાવલિ(nodep);
        જો (m_vup->prelim()) {
            UINFOTREE(9, nodep, "", "selWidth");
            userIterateAndNext(nodep->fromp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            userIterateAndNext(nodep->lsbp(), WidthVP{SELF, PRELIM}.p());
            checkCvtUS(nodep->fromp(), ખોટું);
            iterateCheckSizedSelf(nodep, "Select LHS", nodep->fromp(), SELF, BOTH);
            પૂર્ણાંક width = nodep->widthConst();
            જો (width <= 0) {
                nodep->v3error("Width of bit extract must be positive (IEEE 1800-2023 11.5.1)");
                nodep->dtypeSetBit();
                પાછા;
            }
            UASSERT_OBJ(nodep->dtypep(), nodep, "dtype wasn't set");  // by V3WidthSel

            અચલ તાર્કિક isWriteSelect = nodep->fromp()->isLValue();
            // Suppress SELRANGE in parameterized template modules where
            // parameter-dependent widths haven't been resolved yet.
            અચલ તાર્કિક inParameterizedTemplate
                = m_modep && (m_modep->dead() || m_modep->parameterizedTemplate());

            જો (VN_IS(nodep->lsbp(), Const) && nodep->msbConst() < nodep->lsbConst()) {
                // Likely impossible given above width check
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: left < right of bit extract: "  // LCOV_EXCL_LINE
                                  << nodep->msbConst() << "<" << nodep->lsbConst());
                width = (nodep->lsbConst() - nodep->msbConst() + 1);
                nodep->dtypeSetBitOrLogicSized(width, VSigning::UNSIGNED,
                                               nodep->fromp()->isFourstateOrNoDtype());
                nodep->widthConst(width);
                ધકેલો_Deletep(nodep->lsbp());
                nodep->lsbp()->replaceWith(નવું AstConst{nodep->lsbp()->fileline(), 0});
            }
            // We're extracting, so just make sure the expression is at least wide enough.
            જો (nodep->fromp()->width() < width && !inParameterizedTemplate) {
                // If it is not a lvalue, extend it
                જો (!isWriteSelect) {
                    nodep->v3warn(SELRANGE, "Extracting " << width << " bits from only "
                                                          << nodep->fromp()->width()
                                                          << " bit number");
                    Ast_ગાંઠ_DType* અચલ subDTypep = nodep->findBitOrLogicDType(
                        width, width, nodep->fromp()->dtypep()->numeric(),
                        nodep->fromp()->dtypep()->isFourstate());
                    widthCheckSized(nodep, "errorless...", nodep->fromp(), subDTypep, EXTEND_EXP,
                                    ખોટું /*noerror*/);
                } નહીંતર {
                    // Partial assignment
                    nodep->v3warn(SELRANGE, "Assigning " << width << " bits to only "
                                                         << nodep->fromp()->width()
                                                         << " bit number");
                }
            }
            // Check bit indexes.
            // What is the MSB?  We want the true MSB, not one starting at
            // 0, because a 4 bit index is required to look at a one-bit
            // variable[15:15] and 5 bits for [15:-2]
            પૂર્ણાંક frommsb = nodep->fromp()->width() - 1;
            પૂર્ણાંક fromlsb = 0;
            અચલ પૂર્ણાંક elw = nodep->declElWidth();  // Must adjust to tell user bit ranges
            જો (nodep->declRange().ranged()) {
                frommsb = nodep->declRange().hiMaxSelect() * elw
                          + (elw - 1);  // Corrected for negative lsb
                fromlsb = nodep->declRange().lo() * elw;
            } નહીંતર {
                // nodep->v3fatalSrc("Should have been declRanged in V3WidthSel");
            }
            અચલ પૂર્ણાંક selwidth = V3_સંખ્યા::log2b(frommsb + 1 - 1) + 1;  // Width to address a bit
            Ast_ગાંઠ_DType* અચલ selwidthDTypep = nodep->findBitOrLogicDType(
                selwidth, selwidth, nodep->lsbp()->dtypep()->numeric(),
                nodep->lsbp()->dtypep()->isFourstate());
            userIterateAndNext(nodep->fromp(), WidthVP{SELF, FINAL}.p());
            userIterateAndNext(nodep->lsbp(), WidthVP{SELF, FINAL}.p());
            જો (widthBad(nodep->lsbp(), selwidthDTypep) && nodep->lsbp()->width() != 32) {
                જો (!nodep->fileline()->warnIsOff(V3ErrorCode::WIDTH)) {
                    nodep->v3widthWarn(
                        (selwidth / elw), (nodep->lsbp()->width() / elw),
                        "Bit extraction of var["
                            << (frommsb / elw) << ":" << (fromlsb / elw) << "] requires "
                            << (selwidth / elw) << " bit index, not "
                            << (nodep->lsbp()->width() / elw)
                            << (nodep->lsbp()->width() != nodep->lsbp()->widthMin()
                                    ? " or " + cvtToStr(nodep->lsbp()->widthMin() / elw)
                                    : "")
                            << " bits.");
                    UINFO(1, "    Related node: " << nodep);
                }
            }
            જો (VN_IS(nodep->lsbp(), Const) && nodep->msbConst() > frommsb) {
                // See also warning in V3Const
                // We need to check here, because the widthCheckSized may silently
                // add another SEL which will lose the out-of-range check
                //
                // We don't want to trigger an error here if we are just
                // evaluating type sizes for a generate block condition. We
                // should only trigger the error if the out-of-range access is
                // actually generated.
                જો (m_doGenerate) {
                    UINFO(5, "Selection index out of range inside generate");
                } નહીંતર જો (!inParameterizedTemplate) {
                    જો (nodep->declRange().ranged()) {
                        nodep->v3warn(SELRANGE, "Selection index out of range: "
                                                    << nodep->msbConst() << ":"
                                                    << nodep->lsbConst() << " outside " << frommsb
                                                    << ":" << fromlsb);
                    } નહીંતર {
                        nodep->v3warn(SELRANGE,
                                      "Selection "
                                          << nodep->msbConst() << ":" << nodep->lsbConst()
                                          << " performed on an object without declared range");
                    }
                    UINFO(1, "    Related node: " << nodep);
                }
                // Extend unless it's a lvalue,
                // because extending lvalue would lose write access.
                જો (!isWriteSelect) {
                    // Extend it
                    અચલ પૂર્ણાંક extendTo = nodep->msbConst() + 1;
                    Ast_ગાંઠ_DType* અચલ subDTypep = nodep->findBitOrLogicDType(
                        extendTo, extendTo, nodep->fromp()->dtypep()->numeric(),
                        nodep->fromp()->dtypep()->isFourstate());
                    widthCheckSized(nodep, "errorless...", nodep->fromp(), subDTypep, EXTEND_EXP,
                                    ખોટું /*noerror*/);
                }
            }
            // iterate FINAL is two blocks above
            //
            // If we have a width problem with GENERATE etc, this will reduce
            // it down and mask it, so we have no chance of finding a real
            // error in the future. So don't do this for them.
            જો (!m_doGenerate) {
                // lsbp() must be self-determined, however for performance
                // we want the select to be truncated to fit within the
                // maximum select range, e.g. turn Xs outside of the select
                // into something fast which pulls from within the array.
                widthCheckSized(nodep, "Extract Range", nodep->lsbp(), selwidthDTypep, EXTEND_EXP,
                                ખોટું /*NOWARN*/);
            }
            // UINFOTREE(9, nodep, "", "seldone");
        }
    }

    શૂન્ય visit(AstArraySel* nodep) override {
        // Signed/Real: Output signed iff LHS signed/real; binary operator
        // Note by contrast, bit extract selects are unsigned
        // LSB is self-determined (IEEE 2012 11.5.1)
        assert_At_પદાવલિ(nodep);
        જો (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "Bit select", nodep->bitp(), SELF, BOTH);
            userIterateAndNext(nodep->fromp(), WidthVP{SELF, BOTH}.p());
            // Element of a whole array of interfaces
            જો (VN_IS(nodep->fromp(), ConsArray) && isIfaceArrayDType(nodep->fromp()->dtypep())) {
                V3Const::constifyParamsEdit(nodep->bitp());  // May relink pointed to node
                જો (અચલ AstConst* અચલ constp = VN_CAST(nodep->bitp(), Const)) {
                    અચલ પૂર્ણાંક bit = constp->toSInt();
                    Ast_ગાંઠ_પદાવલિ* newp = newIfaceArrayElement(nodep->fromp(), bit);
                    જો (!newp) {
                        // No such instance, so an error, unlike other out of range selects
                        અચલ AstUnpackArrayDType* અચલ arrp
                            = VN_AS(nodep->fromp()->dtypep()->skipRefp(), UnpackArrayDType);
                        nodep->v3error("Selection index out of range of interface array: "
                                       << bit + arrp->lo() << " outside " << arrp->hi() << ":"
                                       << arrp->lo() << " (IEEE 1800-2023 23.6)");
                        newp = newIfaceArrayElement(nodep->fromp(), 0);
                    }
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
                    પાછા;
                }
            }
            //
            પૂર્ણાંક frommsb;
            પૂર્ણાંક fromlsb;
            અચલ Ast_ગાંઠ_DType* અચલ fromDtp = nodep->fromp()->dtypep()->skipRefp();
            જો (અચલ AstUnpackArrayDType* અચલ adtypep = VN_CAST(fromDtp, UnpackArrayDType)) {
                frommsb = adtypep->hi();
                fromlsb = adtypep->lo();
                જો (fromlsb > frommsb) std::swap(frommsb, fromlsb);
                // However, if the lsb<0 we may go negative, so need more bits!
                જો (fromlsb < 0) frommsb += -fromlsb;
                nodep->dtypeFrom(adtypep->subDTypep());  // Need to strip off array reference
            } નહીંતર {
                // Note PackArrayDType doesn't use an ArraySel but a normal Sel.
                UINFO(1, "    Related dtype: " << fromDtp);
                nodep->v3fatalSrc("Array reference exceeds dimension of array");
                frommsb = fromlsb = 0;
            }
            અચલ પૂર્ણાંક selwidth = V3_સંખ્યા::log2b(frommsb + 1 - 1) + 1;  // Width to address a bit
            Ast_ગાંઠ_DType* અચલ selwidthDTypep = nodep->findBitOrLogicDType(
                selwidth, selwidth, nodep->bitp()->dtypep()->numeric(),
                nodep->bitp()->isFourstateOrNoDtype());
            જો (widthBad(nodep->bitp(), selwidthDTypep) && nodep->bitp()->width() != 32) {
                nodep->v3widthWarn(selwidth, nodep->bitp()->width(),
                                   "Bit extraction of array["
                                       << frommsb << ":" << fromlsb << "] requires " << selwidth
                                       << " bit index, not " << nodep->bitp()->width()
                                       << (nodep->bitp()->width() != nodep->bitp()->widthMin()
                                               ? " or " + cvtToStr(nodep->bitp()->widthMin())
                                               : "")
                                       << " bits.");
                જો (!nodep->fileline()->warnIsOff(V3ErrorCode::WIDTH)) {
                    UINFO(1, "    Related node: " << nodep);
                    UINFO(1, "    Related dtype: " << nodep->dtypep());
                }
            }
            જો (!m_doGenerate) {
                // Must check bounds before adding a select that truncates the bound
                // Note we've already subtracted off LSB
                જો (VN_IS(nodep->bitp(), Const)
                    && (VN_AS(nodep->bitp(), Const)->toSInt() > (frommsb - fromlsb)
                        || VN_AS(nodep->bitp(), Const)->toSInt() < 0)) {
                    // Suppress in dead/parameterized template modules
                    જો (!(m_modep && (m_modep->dead() || m_modep->parameterizedTemplate()))) {
                        nodep->v3warn(SELRANGE,
                                      "Selection index out of range: "
                                          << (VN_AS(nodep->bitp(), Const)->toSInt() + fromlsb)
                                          << " outside " << frommsb << ":" << fromlsb);
                    }
                }
                widthCheckSized(nodep, "Extract Range", nodep->bitp(), selwidthDTypep, EXTEND_EXP,
                                ખોટું /*NOWARN*/);
            }
        }
    }

    શૂન્ય visit(AstAssocSel* nodep) override {
        // Signed/Real: Output type based on array-declared type; binary operator
        assert_At_પદાવલિ(nodep);
        જો (m_vup->prelim()) {
            અચલ Ast_ગાંઠ_DType* અચલ fromDtp = nodep->fromp()->dtypep()->skipRefp();
            અચલ AstAssocArrayDType* અચલ adtypep = VN_CAST(fromDtp, AssocArrayDType);
            જો (!adtypep) {
                UINFO(1, "    Related dtype: " << fromDtp);
                nodep->v3fatalSrc("Associative array reference is not to associative array");
            }
            iterateCheckTyped(nodep, "Associative select", nodep->bitp(), adtypep->ચાવી_DTypep(),
                              BOTH);
            nodep->dtypeFrom(adtypep->subDTypep());
        }
    }

    શૂન્ય visit(AstAlias* nodep) override {
        જો (!nodep->didWidthAndSet()) {
            userIterateAndNext(nodep->itemsp(), WidthVP{SELF, BOTH}.p());
        }

        અચલ auto check_If_પદાવલિ_Ok = [આ](અચલ Ast_ગાંઠ_પદાવલિ* અચલ exprp) {
            જો (VN_IS(exprp, VarXRef)) {
                exprp->v3error("Hierarchical reference used for net alias (IEEE 1800-2023 10.11)");
                પાછા ખોટું;
            }

            અચલ AstVarRef* અચલ varRefp = VN_CAST(exprp, VarRef);
            જો (!varRefp) {
                exprp->v3warn(
                    E_UNSUPPORTED,
                    "Unsupported: Operand of alias statement is not a variable reference");
                પાછા ખોટું;
            }
            AstVar* અચલ varp = varRefp->varp();
            જો (!varp->isNet()) {
                exprp->v3error("Only nets are allowed in alias (IEEE 1800-2023 10.11): "
                               << varp->prettyNameQ());
                પાછા ખોટું;
            }
            જો (m_aliasedVars.find(varp) != m_aliasedVars.end()) {
                varRefp->v3error("Alias is specified more than once (IEEE 1800-2023 10.11): "
                                 << varp->prettyNameQ());
                પાછા ખોટું;
            } નહીંતર {
                m_aliasedVars.ઉમેરો(varp);
            }
            પાછા સાચું;
        };

        check_If_પદાવલિ_Ok(nodep->itemsp());
        Ast_ગાંઠ_DType* અચલ firstItemDtypep = nodep->itemsp()->dtypep();
        માટે (Ast_ગાંઠ* itemp = nodep->itemsp()->nextp(); itemp; itemp = itemp->nextp()) {
            check_If_પદાવલિ_Ok(VN_AS(itemp, ગાંઠ_પદાવલિ));
            જો (!firstItemDtypep->similarDType(itemp->dtypep())) {
                itemp->v3error("Incompatible data types of nets used for net alias. First operand "
                               "has the type "
                               << firstItemDtypep->prettyDTypeNameQ() << ", other has "
                               << itemp->dtypep()->prettyDTypeNameQ());
            }
        }
    }

    શૂન્ય visit(AstWildcardSel* nodep) override {
        // Signed/Real: Output type based on array-declared type; binary operator
        assert_At_પદાવલિ(nodep);
        જો (m_vup->prelim()) {
            અચલ Ast_ગાંઠ_DType* અચલ fromDtp = nodep->fromp()->dtypep()->skipRefp();
            અચલ AstWildcardArrayDType* અચલ adtypep = VN_CAST(fromDtp, WildcardArrayDType);
            જો (!adtypep) {
                UINFO(1, "    Related dtype: " << fromDtp);
                nodep->v3fatalSrc("Wildcard array reference is not to wildcard array");
            }
            અચલ AstBasicDType* અચલ basicp = nodep->bitp()->dtypep()->skipRefp()->basicp();
            જો (!basicp
                || (basicp->keyword() != VBasicDTypeKwd::STRING
                    && !basicp->keyword().isIntNumeric())) {
                nodep->v3error("Wildcard index must be integral (IEEE 1800-2023 7.8.1)");
            }
            iterateCheckTyped(nodep, "Wildcard associative select", nodep->bitp(),
                              adtypep->findStringDType(), BOTH);
            nodep->dtypeFrom(adtypep->subDTypep());
        }
    }

    શૂન્ય visit(AstSliceSel* nodep) override {
        // Always creates as output an unpacked array
        assert_At_પદાવલિ(nodep);
        જો (m_vup->prelim()) {
            userIterateAndNext(nodep->fromp(), WidthVP{SELF, BOTH}.p());
            //
            // Array indices are always constant
            અચલ Ast_ગાંઠ_DType* અચલ fromDtp = nodep->fromp()->dtypep()->skipRefp();
            અચલ AstUnpackArrayDType* અચલ adtypep = VN_CAST(fromDtp, UnpackArrayDType);
            જો (!adtypep) {
                UINFO(1, "    Related dtype: " << fromDtp);
                nodep->v3fatalSrc("Packed array reference exceeds dimension of array");
            }
            // Build new array Dtype based on the original's base type, but with new bounds
            Ast_ગાંઠ_DType* અચલ newDtp
                = નવું AstUnpackArrayDType{nodep->fileline(), adtypep->subDTypep(),
                                          નવું AstRange{nodep->fileline(), nodep->declRange()}};
            v3Global.rootp()->typeTablep()->addTypesp(newDtp);
            nodep->dtypeFrom(newDtp);

            જો (!m_doGenerate) {
                // Must check bounds before adding a select that truncates the bound
                // Note we've already subtracted off LSB
                અચલ પૂર્ણાંક subtracted = adtypep->declRange().lo();
                // Add subtracted value to get the original range
                અચલ VNumRange declRange{nodep->declRange().hi() + subtracted,
                                          nodep->declRange().lo() + subtracted,
                                          nodep->declRange().ascending()};
                જો ((declRange.hi() > adtypep->declRange().hi())
                    || declRange.lo() < adtypep->declRange().lo()) {
                    // Other simulators warn too
                    nodep->v3error("Slice selection index '" << declRange << "'"
                                                             << " outside data type's '"
                                                             << adtypep->declRange() << "'");
                    // Replaced by a placeholder value, so no element is out of range later
                    nodep->replaceWith(નવું AstConsArray{
                        nodep->fileline(), newDtp,
                        નવું AstConst{nodep->fileline(), AstConst::BitFalseErroring{}}});
                    VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
                    પાછા;
                } નહીંતર જો ((declRange.ascending() != adtypep->declRange().ascending())
                           && declRange.hi() != declRange.lo()) {
                    nodep->v3error("Slice selection '"
                                   << declRange << "'"
                                   << " has reversed range order versus data type's '"
                                   << adtypep->declRange() << "'");
                }
            }
            // A slice of a whole array of interfaces is the array of the selected elements
            જો (VN_IS(nodep->fromp(), ConsArray) && isIfaceArrayDType(nodep->fromp()->dtypep())) {
                AstConsArray* અચલ fromp = VN_AS(nodep->fromp(), ConsArray);
                AstConsArray* અચલ newp = નવું AstConsArray{nodep->fileline(), newDtp};
                અચલ VNumRange range = nodep->declRange();
                માટે (પૂર્ણાંક n = 0; n < range.elements(); ++n) {
                    newp->valuep(n, fromp->valuep(range.lo() + n)->clone_વૃક્ષ(ખોટું));
                }
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
            }
        }
    }

    શૂન્ય visit(AstSelBit* nodep) override {
        // Just a quick check as after V3Param these nodes instead are AstSel's
        userIterateAndNext(nodep->fromp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        userIterateAndNext(nodep->bitp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        userIterateAndNext(nodep->thsp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        userIterateAndNext(nodep->attrp(), WidthVP{SELF, BOTH}.p());
        Ast_ગાંઠ* અચલ selp = V3Width::widthSelNoIterEdit(nodep);
        જો (selp != nodep) {
            VL_DANGLING(nodep);
            userIterate(selp, m_vup);
            પાછા;
        }
        nodep->v3fatalSrc("AstSelBit should disappear after widthSel");
    }
    શૂન્ય visit(AstSelExtract* nodep) override {
        // Just a quick check as after V3Param these nodes instead are AstSel's
        userIterateAndNext(nodep->fromp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        userIterateAndNext(nodep->leftp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        userIterateAndNext(nodep->rightp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        userIterateAndNext(nodep->attrp(), WidthVP{SELF, BOTH}.p());
        Ast_ગાંઠ* અચલ selp = V3Width::widthSelNoIterEdit(nodep);
        જો (selp != nodep) {
            nodep = nullptr;
            userIterate(selp, m_vup);
            પાછા;
        }
        nodep->v3fatalSrc("AstSelExtract should disappear after widthSel");
    }
    શૂન્ય visit(AstSelPlus* nodep) override {
        userIterateAndNext(nodep->fromp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        userIterateAndNext(nodep->bitp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        userIterateAndNext(nodep->widthp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        userIterateAndNext(nodep->attrp(), WidthVP{SELF, BOTH}.p());
        Ast_ગાંઠ* અચલ selp = V3Width::widthSelNoIterEdit(nodep);
        જો (selp != nodep) {
            nodep = nullptr;
            userIterate(selp, m_vup);
            પાછા;
        }
        nodep->v3fatalSrc("AstSelPlus should disappear after widthSel");
    }
    શૂન્ય visit(AstSelMinus* nodep) override {
        userIterateAndNext(nodep->fromp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        userIterateAndNext(nodep->bitp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        userIterateAndNext(nodep->widthp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        userIterateAndNext(nodep->attrp(), WidthVP{SELF, BOTH}.p());
        Ast_ગાંઠ* અચલ selp = V3Width::widthSelNoIterEdit(nodep);
        જો (selp != nodep) {
            nodep = nullptr;
            userIterate(selp, m_vup);
            પાછા;
        }
        nodep->v3fatalSrc("AstSelMinus should disappear after widthSel");
    }

    શૂન્ય visit(AstExtend* nodep) override {
        // Typically created by this process, so we know width from here down is correct.
        // Exception is extend added by V3WidthSel - those need iteration
        જો (nodep->didWidthAndSet()) પાછા;
        userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
    }
    શૂન્ય visit(AstExtendS* nodep) override {
        // Typically created by this process, so we know width from here down is correct.
        // Exception is extend added by V3WidthSel - those need iteration
        જો (nodep->didWidthAndSet()) પાછા;
        userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
    }
    શૂન્ય visit(AstConst* nodep) override {
        // The node got setup with the signed/real state of the node.
        // However a later operation may have changed the node->signed w/o changing
        // the number's sign.  So we don't: nodep->dtypeChgSigned(nodep->num().isSigned());
        જો (nodep->didWidthAndSet()) પાછા;
        જો (m_vup && m_vup->prelim()) {
            જો (VN_IS(nodep->dtypep()->skipRefToEnump(), EnumDType)
                || VN_IS(nodep->dtypep()->skipRefp(), StructDType)
                || VN_IS(nodep->dtypep()->skipRefp(), PackArrayDType)) {
                // Assume this constant was properly casted earlier
                // (Otherwise it couldn't have an enum data type)
            } નહીંતર જો (nodep->અંક().isNull()) {
                nodep->dtypep(nodep->findNullDType());
            } નહીંતર જો (nodep->અંક().isString()) {
                nodep->dtypeSetString();
            } નહીંતર જો (nodep->અંક().isDouble()) {
                nodep->dtypeChgWidth(nodep->અંક().width(), nodep->અંક().sized()
                                                               ? nodep->અંક().width()
                                                               : nodep->અંક().widthToFit());
            } નહીંતર {
                nodep->dtypeSetBitOrLogicUnsized(
                    nodep->અંક().width(),
                    nodep->અંક().sized() ? nodep->અંક().width() : nodep->અંક().widthToFit(),
                    nodep->dtypep()->numeric(), nodep->અંક().isAnyXZ());
            }
        }
        // We don't size the constant until we commit the widths, as need parameters
        // to remain unsized, and numbers to remain unsized to avoid backp() warnings
    }
    શૂન્ય visit(AstEmptyQueue* nodep) override {
        nodep->dtypeSetEmptyQueue();
        જો (!VN_IS(nodep->backp(), Assign) && !VN_IS(nodep->backp(), Var)
            && !VN_IS(nodep->backp(), Arg)) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported/Illegal: empty queue ('{}') in this context");
        }
    }
    શૂન્ય visit(AstFell* nodep) override {
        assert_At_પદાવલિ(nodep);
        જો (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ સાચું);
            userIterate(nodep->sentreep(), nullptr);
            nodep->dtypeSetBit();
        }
    }
    શૂન્ય visit(AstFalling* nodep) override {
        assert_At_પદાવલિ(nodep);
        જો (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ સાચું);
            nodep->dtypeSetBit();
        }
    }
    શૂન્ય visit(AstFuture* nodep) override {
        assert_At_પદાવલિ(nodep);
        જો (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ સાચું);
            userIterate(nodep->sentreep(), nullptr);
            nodep->dtypeFrom(nodep->exprp());
        }
    }
    શૂન્ય visit(AstPast* nodep) override {
        assert_At_પદાવલિ(nodep);
        જો (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ સાચું);
            nodep->dtypeFrom(nodep->exprp());
            જો (nodep->ticksp()) {
                iterateCheckSizedSelf(nodep, "Ticks", nodep->ticksp(), SELF, BOTH,
                                      /* deferUntyped */ સાચું);
                V3Const::constifyParamsEdit(nodep->ticksp());  // ticksp may change
                અચલ AstConst* અચલ constp = VN_CAST(nodep->ticksp(), Const);
                જો (!constp) {
                    nodep->v3error("$past tick value must be constant (IEEE 1800-2023 16.9.3)");
                    nodep->ticksp()->unlinkFrBack()->delete_વૃક્ષ();
                } નહીંતર જો (constp->toSInt() < 1) {
                    constp->v3error("$past tick value must be >= 1 (IEEE 1800-2023 16.9.3)");
                    nodep->ticksp()->unlinkFrBack()->delete_વૃક્ષ();
                } નહીંતર જો (V3AssertCommon::check_Sva_કિંમત_Limit(nodep, constp, "$past tick value")) {
                    જો (constp->toSInt() > 10) {
                        constp->v3warn(TICKCOUNT, "$past tick value of "
                                                      << constp->toSInt()
                                                      << " may have a large performance cost");
                    }
                }
            }
            userIterate(nodep->sentreep(), nullptr);
        }
    }
    શૂન્ય visit(AstSConsRep* nodep) override {
        // IEEE 1800-2023 16.9.2 -- consecutive repetition [*N], [*N:M], [*N:$], [+], [*]
        assert_At_પદાવલિ(nodep);
        જો (m_vup->prelim()) {
            userIterateAndNext(nodep->exprp(), WidthVP{SELF, BOTH}.p());
            userIterateAndNext(nodep->countp(), WidthVP{SELF, BOTH}.p());
            V3Const::constifyParamsEdit(nodep->countp());
            અચલ AstConst* અચલ minConstp = VN_CAST(nodep->countp(), Const);
            જો (!minConstp) {
                nodep->v3error("Consecutive repetition count must be constant expression"
                               " (IEEE 1800-2023 16.9.2)");
            } નહીંતર જો (minConstp->toSInt() < 0) {
                nodep->v3error("Consecutive repetition count must be non-negative"
                               " (IEEE 1800-2023 16.9.2)");
            } નહીંતર જો (!nodep->unbounded() && !nodep->maxCountp() && minConstp->toSInt() < 1) {
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: [*0] consecutive repetition");
            }
            જો (nodep->maxCountp()) {
                userIterateAndNext(nodep->maxCountp(), WidthVP{SELF, BOTH}.p());
                V3Const::constifyParamsEdit(nodep->maxCountp());
                અચલ AstConst* અચલ maxConstp = VN_CAST(nodep->maxCountp(), Const);
                જો (!maxConstp) {
                    nodep->v3error("Consecutive repetition max count must be constant"
                                   " expression (IEEE 1800-2023 16.9.2)");
                } નહીંતર જો (minConstp && maxConstp->toSInt() < minConstp->toSInt()) {
                    nodep->v3error("Consecutive repetition max count must be >= min count"
                                   " (IEEE 1800-2023 16.9.2)");
                } નહીંતર જો (maxConstp->toSInt() < 1) {
                    nodep->v3warn(E_UNSUPPORTED, "Unsupported: [*N:0] consecutive repetition"
                                                 " with zero max count");
                }
            }
            nodep->dtypeSetBit();
        }
    }
    શૂન્ય visit(AstPropAlways* nodep) override {
        // IEEE 1800-2023 16.12.11
        assert_At_પદાવલિ(nodep);
        જો (m_vup->prelim()) {
            userIterateAndNext(nodep->propp(), WidthVP{SELF, BOTH}.p());
            Ast_ગાંઠ_પદાવલિ* propp = nodep->propp();
            જો (!VN_IS(nodep->loBoundp(), Unbounded)) {
                userIterateAndNext(nodep->loBoundp(), WidthVP{SELF, BOTH}.p());
                V3Const::constifyParamsEdit(nodep->loBoundp());
            }
            જો (!VN_IS(nodep->hiBoundp(), Unbounded)) {
                userIterateAndNext(nodep->hiBoundp(), WidthVP{SELF, BOTH}.p());
                V3Const::constifyParamsEdit(nodep->hiBoundp());
            }
            અચલ તાર્કિક loUnbounded = VN_IS(nodep->loBoundp(), Unbounded);
            અચલ તાર્કિક hiUnbounded = VN_IS(nodep->hiBoundp(), Unbounded);
            // Strong always must be bounded (IEEE 1800-2023 16.12.11: "the range
            // for a strong always shall be bounded"). Weak always [m:$] is legal:
            // an unbounded upper bound imposes no end-of-trace obligation.
            જો (nodep->isStrong() && (loUnbounded || hiUnbounded)) {
                Ast_ગાંઠ* અચલ boundp = loUnbounded ? nodep->loBoundp() : nodep->hiBoundp();
                boundp->v3error("s_always range must be bounded (IEEE 1800-2023 16.12.11)");
                nodep->dtypeSetBit();
                પાછા;
            }
            જો (loUnbounded) {
                // Only the high bound may be $ (cycle_delay_const_range_expression).
                nodep->loBoundp()->v3error("always range low bound must be a constant expression"
                                           " (IEEE 1800-2023 16.12.11)");
                nodep->dtypeSetBit();
                પાછા;
            }
            અચલ AstConst* અચલ loConstp = VN_CAST(nodep->loBoundp(), Const);
            અચલ AstConst* અચલ hiConstp = VN_CAST(nodep->hiBoundp(), Const);
            જો (!loConstp) {
                nodep->loBoundp()->v3error("always range low bound must be a constant expression"
                                           " (IEEE 1800-2023 16.12.11)");
            }
            જો (!hiUnbounded && !hiConstp) {
                nodep->hiBoundp()->v3error("always range high bound must be a constant expression"
                                           " (IEEE 1800-2023 16.12.11)");
            }
            જો (loConstp && loConstp->toSInt() < 0) {
                nodep->loBoundp()->v3error("always range low bound must be non-negative"
                                           " (IEEE 1800-2023 16.12.11)");
            } નહીંતર જો (loConstp) {
                V3AssertCommon::check_Sva_કિંમત_Limit(nodep->loBoundp(), loConstp,
                                                   "always range bound");
            }
            જો (!hiUnbounded && hiConstp) {
                V3AssertCommon::check_Sva_કિંમત_Limit(nodep->hiBoundp(), hiConstp,
                                                   "always range bound");
            }
            જો (!hiUnbounded && loConstp && hiConstp && hiConstp->toSInt() < loConstp->toSInt()) {
                nodep->hiBoundp()->v3error("always range high bound must be >= low bound"
                                           " (IEEE 1800-2023 16.12.11)");
            }
            તાર્કિક hasPropertyOp = propp->isMultiCycleSva();
            જો (!hasPropertyOp) {
                propp->foreach([&](અચલ Ast_ગાંઠ* np) {
                    જો (VN_IS(np, Implication) || VN_IS(np, Until) || VN_IS(np, PropSpec)) {
                        hasPropertyOp = સાચું;
                    }
                });
            }
            જો (hasPropertyOp) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: property/sequence operator inside bounded"
                              " always (IEEE 1800-2023 16.12.11)");
            }
            VL_DO_DANGLING(fixWidthReduce(propp), propp);
            nodep->dtypeSetBit();
        }
    }
    શૂન્ય visit(AstRising* nodep) override {
        assert_At_પદાવલિ(nodep);
        જો (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ સાચું);
            nodep->dtypeSetBit();
        }
    }
    શૂન્ય visit(AstRose* nodep) override {
        assert_At_પદાવલિ(nodep);
        જો (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ સાચું);
            userIterate(nodep->sentreep(), nullptr);
            nodep->dtypeSetBit();
        }
    }

    શૂન્ય visit(AstSampled* nodep) override {
        assert_At_પદાવલિ(nodep);
        જો (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ સાચું);
            nodep->dtypeFrom(nodep->exprp());
        }
    }

    શૂન્ય visit(AstSetuphold* nodep) override {
        File_લીટી* અચલ flp = nodep->fileline();
        AstAssignW* newp = nullptr;
        જો (nodep->delrefp()) {
            newp = convertSetupholdToAssign(flp, nodep->refevp(), nodep->delrefp());
        }
        જો (nodep->deldatap()) {
            જો (!newp) {
                newp = convertSetupholdToAssign(flp, nodep->dataevp(), nodep->deldatap());
            } નહીંતર {
                newp->addNextHere(
                    convertSetupholdToAssign(flp, nodep->dataevp(), nodep->deldatap()));
            }
        }
        જો (!newp) {
            VL_DO_DANGLING(ધકેલો_Deletep(nodep->unlinkFrBack()), nodep);
            પાછા;
        }
        nodep->replaceWith(નવું AstAlways{newp});
        VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
    }

    AstAssignW* convertSetupholdToAssign(File_લીટી* અચલ flp, Ast_ગાંઠ_પદાવલિ* અચલ evp,
                                         Ast_ગાંઠ_પદાવલિ* અચલ delp) {
        Ast_ગાંઠ_પદાવલિ* અચલ lhsp = delp->clone_વૃક્ષ_Pure(ખોટું);
        Ast_ગાંઠ_પદાવલિ* અચલ rhsp = evp->clone_વૃક્ષ_Pure(ખોટું);
        UASSERT_OBJ(VN_IS(lhsp, ગાંઠ_Var_Ref) || VN_IS(lhsp, ગાંઠ_Pre_Sel), lhsp,
                    "Incorrect reference in a timing check");
        જો (Ast_ગાંઠ_Var_Ref* varRefp = VN_CAST(lhsp, ગાંઠ_Var_Ref)) {
            જો (varRefp->varp()->direction() == VDirection::આગત) {
                VL_DO_DANGLING(ધકેલો_Deletep(lhsp), lhsp);
                VL_DO_DANGLING(ધકેલો_Deletep(rhsp), rhsp);
                પાછા nullptr;
            }
            varRefp->access(VAccess::WRITE);
        }
        જો (Ast_ગાંઠ_Pre_Sel* selp = VN_CAST(lhsp, ગાંઠ_Pre_Sel)) {
            જો (Ast_ગાંઠ_Var_Ref* varRefp = VN_CAST(selp->fromp(), ગાંઠ_Var_Ref)) {
                જો (varRefp->varp()->direction() == VDirection::આગત) {
                    VL_DO_DANGLING(ધકેલો_Deletep(lhsp), lhsp);
                    VL_DO_DANGLING(ધકેલો_Deletep(rhsp), rhsp);
                    પાછા nullptr;
                }
                varRefp->access(VAccess::WRITE);
            }
        }
        પાછા નવું AstAssignW{flp, lhsp, rhsp};
    }

    શૂન્ય visit(AstStable* nodep) override {
        assert_At_પદાવલિ(nodep);
        જો (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ સાચું);
            userIterate(nodep->sentreep(), nullptr);
            nodep->dtypeSetBit();
        }
    }
    શૂન્ય visit(AstSteady* nodep) override {
        assert_At_પદાવલિ(nodep);
        જો (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ સાચું);
            nodep->dtypeSetBit();
        }
    }

    શૂન્ય visit(Ast_Stmt_પદાવલિ* nodep) override {
        assertAtStatement(nodep);
        userIterateAndNext(nodep->exprp(), WidthVP{SELF, BOTH}.p());
    }

    શૂન્ય visit(AstImplication* nodep) override {
        m_seqUnsupp = nodep;
        assert_At_પદાવલિ(nodep);
        જો (m_vup->prelim()) {
            iterateCheckBool(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckBool(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtypeSetBit();
        }
    }

    શૂન્ય visit(AstUntil* nodep) override {
        assert_At_પદાવલિ(nodep);
        જો (m_vup->prelim()) {
            iterateCheckBool(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckBool(nodep, "RHS", nodep->rhsp(), BOTH);
            // Coerce unsized constant operands (e.g. literal 0/1) to actual
            // 1-bit width; iterateCheckBool keeps widthMin-fitting unsized
            // constants at their nominal 32-bit width, which trips the
            // downstream NFA lowering's Log* chains in V3AssertNfa.
            જો (nodep->lhsp()->width() != 1) fixWidthReduce(nodep->lhsp());
            જો (nodep->rhsp()->width() != 1) fixWidthReduce(nodep->rhsp());
            nodep->dtypeSetBit();
        }
    }
    શૂન્ય visit(AstAbortOn* nodep) override { visitAbortProp(nodep); }

    શૂન્ય visit(AstRand* nodep) override {
        assert_At_પદાવલિ(nodep);
        જો (m_vup->prelim()) {
            જો (nodep->urandom()) {
                nodep->dtypeSetUInt32();  // Says the spec
            } નહીંતર {
                nodep->dtypeSetInt();  // Says the spec
            }
            જો (nodep->seedp()) iterateCheckSigned32(nodep, "seed", nodep->seedp(), BOTH);
        }
    }
    શૂન્ય visit(AstSEventually* nodep) override {
        જો (v3Global.opt.timing().isSetFalse() || !v3Global.opt.timing().isSetTrue()) {
            nodep->v3warn(E_NOTIMING, "s_eventually requires --timing");
            nodep->replaceWith(નવું AstConst{nodep->fileline(), AstConst::Widthed_કિંમત{}, 1, 0});
            VL_DO_DANGLING(nodep->delete_વૃક્ષ(), nodep);
            પાછા;
        }
        જો (m_vup->prelim()) {
            iterateCheckBool(nodep, "exprp", nodep->exprp(), BOTH);
            nodep->dtypeSetBit();
        }
    }
    શૂન્ય visit(AstSGotoRep* nodep) override {
        assert_At_પદાવલિ(nodep);
        જો (m_vup->prelim()) {
            iterateCheckBool(nodep, "exprp", nodep->exprp(), BOTH);
            userIterateAndNext(nodep->countp(), WidthVP{SELF, BOTH}.p());
            જો (nodep->maxCountp()) {
                widthCheckGotoRepRange(nodep, "Goto");
            } નહીંતર {
                width_Check_Rep_ગણતરી(nodep);
            }
            nodep->dtypeSetBit();
        }
    }
    શૂન્ય visit(AstSNonConsRep* nodep) override {
        assert_At_પદાવલિ(nodep);
        જો (m_vup->prelim()) {
            iterateCheckBool(nodep, "exprp", nodep->exprp(), BOTH);
            userIterateAndNext(nodep->countp(), WidthVP{SELF, BOTH}.p());
            જો (nodep->maxCountp()) {
                widthCheckGotoRepRange(nodep, "Nonconsecutive");
            } નહીંતર {
                width_Check_Rep_ગણતરી(nodep);
            }
            nodep->dtypeSetBit();
        }
    }
    template <typename T_Rep>
    શૂન્ય width_Check_Rep_ગણતરી(T_Rep* nodep) {
        V3Const::constifyParamsNoWarnEdit(nodep->countp());
        અચલ AstConst* અચલ constp = VN_CAST(nodep->countp(), Const);
        જો (!constp) {
            nodep->v3error("Repetition count is not an elaboration-time constant"
                           " (IEEE 1800-2023 16.9.2)");
        } નહીંતર જો (constp->dtypep()->isSigned() && constp->અંક().isNegative()) {
            nodep->v3error("Repetition count must be non-negative"
                           " (IEEE 1800-2023 16.9.2)");
        } નહીંતર જો (constp->અંક().mostSetBitP1() > 31) {
            nodep->v3warn(
                E_UNSUPPORTED,
                "Unsupported: SVA repetition count exceeds implementation limit of 2147483647");
        } નહીંતર જો (constp->isZero()) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: zero repetition count"
                                         " (IEEE 1800-2023 16.9.2)");
        }
    }
    // IEEE 1800-2023 16.9.2 range-form bound validation for goto/nonconsec.
    // Parent accessors are re-fetched after constifyParamsEdit because that
    // call can replace the node in-tree (Lesson: AstSConsRep visitor pattern).
    // Templated to cover AstSGotoRep and AstSNonConsRep uniformly.
    template <typename T_Rep>
    શૂન્ય widthCheckGotoRepRange(T_Rep* nodep, અચલ અક્ષર* kind) {
        userIterateAndNext(nodep->maxCountp(), WidthVP{SELF, BOTH}.p());
        V3Const::constifyParamsEdit(nodep->countp());
        V3Const::constifyParamsEdit(nodep->maxCountp());
        અચલ AstConst* અચલ minConstp = VN_CAST(nodep->countp(), Const);
        અચલ AstConst* અચલ maxConstp = VN_CAST(nodep->maxCountp(), Const);
        જો (!minConstp || !maxConstp) {
            nodep->v3error(std::string{kind}
                           + " repetition range bounds must be constant expressions"
                             " (IEEE 1800-2023 16.9.2)");
            પાછા;
        }
        જો (minConstp->toSInt() < 0) {
            nodep->v3error(std::string{kind}
                           + " repetition range min count must be non-negative"
                             " (IEEE 1800-2023 16.9.2)");
            પાછા;
        }
        જો (maxConstp->toSInt() < minConstp->toSInt()) {
            nodep->v3error(std::string{kind}
                           + " repetition range max count must be >= min count"
                             " (IEEE 1800-2023 16.9.2)");
            પાછા;
        }
        જો (minConstp->isZero()) {
            nodep->v3warn(E_UNSUPPORTED, std::string{"Unsupported: zero min count in "} + kind
                                             + " repetition range (IEEE 1800-2023 16.9.2)");
        }
    }
    શૂન્ય visit(AstSThroughout* nodep) override {
        m_hasSExpr = સાચું;
        assert_At_પદાવલિ(nodep);
        જો (m_vup->prelim()) {
            // lhsp is a boolean expression, not a sequence -- clear m_underSExpr temporarily
            VL_RESTORER(m_underSExpr);
            m_underSExpr = ખોટું;
            iterateCheckBool(nodep, "lhsp", nodep->lhsp(), BOTH);
            m_underSExpr = સાચું;
            iterate(nodep->rhsp());
            nodep->dtypeSetBit();
        }
    }
    શૂન્ય visit(AstSExpr* nodep) override {
        VL_RESTORER(m_underSExpr);
        m_underSExpr = સાચું;
        m_hasSExpr = સાચું;
        assert_At_પદાવલિ(nodep);
        જો (m_vup->prelim()) {
            જો (nodep->preExprp()) {
                iterateCheckBool(nodep, "preExprp", nodep->preExprp(), BOTH);
            }
            iterate(nodep->delayp());
            iterateCheckBool(nodep, "exprp", nodep->exprp(), BOTH);
            nodep->dtypeSetBit();
        }
    }
    શૂન્ય visit(AstSClocked* nodep) override {
        VL_RESTORER(m_underSExpr);
        m_underSExpr = સાચું;
        m_hasSExpr = સાચું;
        assert_At_પદાવલિ(nodep);
        જો (m_vup->prelim()) {
            // Width each clock expression directly; the senitem chain is hoisted to
            // the assertion clock by V3AssertNfa, where it is fully processed.
            માટે (AstSenItem* senp = nodep->sensesp(); senp;
                 senp = VN_CAST(senp->nextp(), SenItem)) {
                userIterateAndNext(senp->sensp(), WidthVP{SELF, BOTH}.p());
            }
            iterateCheckBool(nodep, "exprp", nodep->exprp(), BOTH);
            nodep->dtypeSetBit();
        }
    }
    શૂન્ય visit(AstURandomRange* nodep) override {
        assert_At_પદાવલિ(nodep);
        જો (m_vup->prelim()) {
            nodep->dtypeSetUInt32();  // Says the spec
            Ast_ગાંઠ_DType* અચલ expDTypep = nodep->findUInt32DType();
            userIterateAndNext(nodep->lhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            userIterateAndNext(nodep->rhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            iterateCheck(nodep, "LHS", nodep->lhsp(), SELF, FINAL, expDTypep, EXTEND_EXP);
            iterateCheck(nodep, "RHS", nodep->rhsp(), SELF, FINAL, expDTypep, EXTEND_EXP);
        }
    }
    શૂન્ય visit(AstUnbounded* nodep) override {
        nodep->dtypeSetInt();  // Used in int context
        જો (VN_IS(nodep->backp(), IsUnbounded)) પાછા;  // Ok, leave
        જો (VN_IS(nodep->backp(), BracketArrayDType)) પાછા;  // Ok, leave
        જો (VN_IS(nodep->backp(), InsideRange)) પાછા;  // Ok, leave
        જો (અચલ auto* અચલ varp = VN_CAST(nodep->backp(), Var)) {
            જો (varp->isParam()) પાછા;  // Ok, leave
        }
        Ast_ગાંઠ* backp = nodep->backp();
        જો (VN_IS(backp, Sub)) backp = backp->backp();
        જો (અચલ auto* અચલ selp = VN_CAST(backp, SelExtract)) {
            જો (VN_IS(selp->fromp()->dtypep()->skipRefp(), QueueDType)) પાછા;
        }
        જો (અચલ auto* અચલ selp = VN_CAST(backp, SelBit)) {
            જો (VN_IS(selp->fromp()->dtypep()->skipRefp(), QueueDType)) પાછા;
        }
        // queue_slice[#:$] and queue_bitsel[$] etc handled in V3WidthSel
        nodep->v3warn(E_UNSUPPORTED,
                      "Unsupported: Unbounded ('$') outside of queue or string operations");
    }
    શૂન્ય visit(AstInferredDisable* nodep) override {
        assert_At_પદાવલિ(nodep);
        જો (m_vup->prelim()) nodep->dtypeSetBit();
    }
    શૂન્ય visit(AstIsUnbounded* nodep) override {
        assert_At_પદાવલિ(nodep);
        જો (m_vup->prelim()) {
            userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
            nodep->dtypeSetBit();
        }
    }
    શૂન્ય visit(AstCExpr* nodep) override {
        // Inserted by V3Width only so we know has been resolved
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
    }
    શૂન્ય visit(AstCExprUser* nodep) override {
        // Give it the size the user wants.
        જો (m_vup && m_vup->prelim()) {
            nodep->dtypeSetBitUnsized(32, 1, VSigning::UNSIGNED);  // We don't care
            // All arguments seek their natural sizes
            userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
        }
        જો (m_vup->final()) {
            Ast_ગાંઠ_DType* અચલ expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
            nodep->dtypep(expDTypep);  // Assume user knows the rules; go with the flow
            જો (nodep->width() > 64) {
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: $c can't generate wider than 64 bits");
            }
        }
    }
    શૂન્ય visit(AstCLog2* nodep) override {
        assert_At_પદાવલિ(nodep);
        જો (m_vup->prelim()) iterateCheckSizedSelf(nodep, "LHS", nodep->lhsp(), SELF, BOTH);
    }
    // Width a 'weight' coverage option as the signed 'int' member it sets (IEEE 1800-2023
    // 19.10); a weight is a non-negative integral value, constant for type_option (19.7)
    template <typename T_Option>
    શૂન્ય widthCoverWeight(T_Option* nodep) {
        iterateCheckSigned32(nodep, "weight", nodep->valuep(), BOTH);
        V3Const::constifyEdit(nodep->valuep());
        Ast_ગાંઠ_પદાવલિ* અચલ valuep = nodep->valuep();
        અચલ std::string optName
            = std::string{nodep->typeOption() ? "type_option" : "option"} + ".weight";
        જો (અચલ AstConst* અચલ constp = VN_CAST(valuep, Const)) {
            જો (!constp->અંક().isNegative()) પાછા;
            valuep->v3error("Coverage option '" << optName << "' is set to negative value '"
                                                << constp->toSInt()
                                                << "'; weights must be non-negative"
                                                   " (IEEE 1800-2023 19.7)");
        } નહીંતર જો (nodep->typeOption()) {
            valuep->v3error("Coverage option '" << optName
                                                << "' requires a constant expression"
                                                   " (IEEE 1800-2023 19.7.1)");
        } નહીંતર {
            પાછા;
        }
        // Continue with the default weight
        valuep->replaceWith(નવું AstConst{valuep->fileline(), AstConst::Signed32{}, 1});
        VL_DO_DANGLING(ધકેલો_Deletep(valuep), valuep);
    }
    // Width an integral coverage option V3Covergroup reads, folding a constant expression
    template <typename T_Option>
    શૂન્ય widthCoverInt(T_Option* nodep) {
        userIterateAndNext(nodep->valuep(), WidthVP{SELF, BOTH}.p());
        V3Const::constifyEdit(nodep->valuep());
    }
    શૂન્ય visit(AstCgOptionAssign* nodep) override {
        // Recursive function widthing can reach a covergroup constructor without first visiting
        // its class, so find the owning covergroup structurally instead of using visit context.
        AstClass* અચલ cgClassp = m_containingClassFinder.find(nodep);
        UASSERT_OBJ(cgClassp && cgClassp->isCovergroup(), nodep,
                    "Covergroup option is not under a covergroup class");

        // V3Covergroup stores the weight into the covergroup's option or type_option
        જો (nodep->optType() == VCoverOptionType::WEIGHT) {
            widthCoverWeight(nodep);
            પાછા;
        }
        // V3Covergroup takes a constant option.at_least as the default for coverpoints/crosses
        જો (nodep->optType() == VCoverOptionType::AT_LEAST) {
            widthCoverInt(nodep);
            પાછા;
        }

        // Extract covergroup option values and store in AstClass before deleting.
        જો (nodep->optType() == VCoverOptionType::AUTO_BIN_MAX) {
            // By V3Width time, V3Param has already folded any parameter references.
            // If the value is still not a constant, it is a runtime expression - emit error.
            જો (AstConst* constp = VN_CAST(nodep->valuep(), Const)) {
                cgClassp->cgAutoBinMax(constp->toSInt());
                UINFO(6, "  Covergroup " << cgClassp->name()
                                         << " option.auto_bin_max = " << constp->toSInt() << endl);
            } નહીંતર {
                nodep->valuep()->v3warn(COVERIGN, "Ignoring unsupported: non-constant "
                                                  "'option.auto_bin_max'; using default value");
            }
        } નહીંતર જો (nodep->optType() == VCoverOptionType::MERGE_INSTANCES) {
            // V3Covergroup stores it into type_option; as a type option it is constant
            // (IEEE 1800-2023 19.7.1)
            iterateCheckBool(nodep, "merge_instances", nodep->valuep(), BOTH);
            V3Const::constifyEdit(nodep->valuep());
            Ast_ગાંઠ_પદાવલિ* અચલ valuep = nodep->valuep();
            જો (!VN_IS(valuep, Const)) {
                valuep->v3error("Coverage option 'type_option.merge_instances' requires a"
                                " constant expression (IEEE 1800-2023 19.7.1)");
                valuep->replaceWith(નવું AstConst{valuep->fileline(), AstConst::BitFalse{}});
                VL_DO_DANGLING(ધકેલો_Deletep(valuep), valuep);
            }
            પાછા;
        } નહીંતર જો (nodep->optType() == VCoverOptionType::GET_INST_COVERAGE) {
            // V3Covergroup stores it into option, as the constructor evaluates it
            iterateCheckBool(nodep, "get_inst_coverage", nodep->valuep(), BOTH);
            પાછા;
        } નહીંતર જો (nodep->optType() == VCoverOptionType::DISTRIBUTE_FIRST) {
            // A bins 'with' filter applies before the values are distributed to the bins
            // (IEEE 1800-2023 19.5.1.1)
            અચલ AstConst* અચલ constp = VN_CAST(nodep->valuep(), Const);
            જો (!constp || !constp->અંક().isEqZero()) {
                nodep->v3warn(COVERIGN, "Ignoring unsupported: 'type_option.distribute_first';"
                                        " 'with' filters apply before values are distributed"
                                        " to bins");
            }
        }
        // Add more options here as needed (goal, per_instance, comment)

        // Delete the assignment node (we've extracted the value)
        VL_DO_DANGLING(ધકેલો_Deletep(nodep->unlinkFrBack()), nodep);
    }
    શૂન્ય visit(AstCoverOption* nodep) override {
        જો (nodep->optType() == VCoverOptionType::WEIGHT) {
            widthCoverWeight(nodep);
            પાછા;
        }
        જો (nodep->optType() == VCoverOptionType::AT_LEAST
            || nodep->optType() == VCoverOptionType::AUTO_BIN_MAX) {
            widthCoverInt(nodep);
            પાછા;
        }
        userIterateAndNext(nodep->valuep(), WidthVP{SELF, BOTH}.p());
    }
    શૂન્ય visit(AstCoverCross* nodep) override {
        userIterateAndNext(nodep->itemsp(), nullptr);
        જો (nodep->iffp()) iterateCheckBool(nodep, "iff condition", nodep->iffp(), BOTH);
        userIterateAndNext(nodep->optionsp(), nullptr);
        userIterateAndNext(nodep->binsp(), nullptr);
    }
    શૂન્ય visit(AstCoverCrossBin* nodep) override {
        userIterateAndNext(nodep->selectp(), nullptr);
        જો (nodep->iffp()) iterateCheckBool(nodep, "iff condition", nodep->iffp(), BOTH);
    }
    શૂન્ય visit(AstCoverpoint* nodep) override {
        // The coverpoint expression is self-determined (IEEE 1800-2023 19.5).  Width it
        // with a context so a bit/part-select (AstSel) is sized here; otherwise it would
        // reach assertAtExpr() with m_vup==null and fail as an internal error.
        userIterateAndNext(nodep->exprp(), WidthVP{SELF, BOTH}.p());
        // Bin values compare against the coverpoint expression
        userIterateAndNext(nodep->binsp(), WidthVP{nodep->exprp()->dtypep(), BOTH}.p());
        જો (nodep->iffp()) iterateCheckBool(nodep, "iff condition", nodep->iffp(), BOTH);
        userIterateAndNext(nodep->optionsp(), nullptr);
    }
    શૂન્ય widthCovergroupRanges(Ast_ગાંઠ* rangesp, પૂર્ણાંક fillWidth) {
        // Bin range/value entries are self-determined expressions (IEEE 1800-2023
        // 19.5).  Width each plain single-value entry self-determined so a referenced
        // parameter acquires a dtype, then constify so the reference folds to the AstConst
        // value that V3Covergroup requires.  AstInsideRange entries fold their own bounds in
        // visit(AstInsideRange).
        // '0/'1 entries then fill to fillWidth, the coverpoint width (IEEE 1800-2023 19.5.7).
        અચલ auto fill = [&](Ast_ગાંઠ* nodep) {
            જો (!fillWidth) પાછા;
            Ast_ગાંઠ_પદાવલિ* exprp = VN_AS(nodep, ગાંઠ_પદાવલિ);
            fixAutoExtend(exprp /*ref*/, fillWidth);
        };
        માટે (Ast_ગાંઠ *nextp, *itemp = rangesp; itemp; itemp = nextp) {
            nextp = itemp->nextp();
            જો (AstInsideRange* અચલ rangep = VN_CAST(itemp, InsideRange)) {
                userIterate(rangep, nullptr);
                fill(rangep->lhsp());
                fill(rangep->rhsp());
            } નહીંતર જો (VN_IS(itemp, CoverWith)) {
                userIterate(itemp, m_vup);
            } નહીંતર {
                itemp = userIterateSubtreeReturnEdits(itemp, WidthVP{SELF, BOTH}.p());
                fill(V3Const::constifyEdit(itemp));
            }
        }
    }
    શૂન્ય visit(AstCoverBinsof* nodep) override {
        userIterateAndNext(nodep->pointp(), nullptr);
        widthCovergroupRanges(nodep->rangesp(), 0);
    }
    શૂન્ય visit(AstCoverBin* nodep) override {
        // No m_vup for a bin directly in a covergroup body (unsupported, already warned)
        widthCovergroupRanges(nodep->rangesp(), m_vup ? m_vup->dtypep()->width() : 0);
        જો (nodep->iffp()) iterateCheckBool(nodep, "iff condition", nodep->iffp(), BOTH);
        જો (nodep->arraySizep()) {
            // The size of 'bins b[N]' or 'bins auto[N]' is a self-determined expression, which
            // V3Covergroup checks once folded
            userIterateAndNext(nodep->arraySizep(), WidthVP{SELF, BOTH}.p());
            V3Const::constifyEdit(nodep->arraySizep());  // arraySizep may change
        }
        userIterateAndNext(nodep->transp(), m_vup);
    }
    શૂન્ય visit(AstCoverWith* nodep) override {
        // The candidate value 'item' has the coverpoint's type (IEEE 1800-2023 19.5.1.1)
        userIterateAndNext(nodep->itemp(), nullptr);
        જો (!nodep->itemp()->dtypeSkipRefp()->isIntegralOrPacked()) {
            nodep->v3error("Bin 'with' filters are not allowed on a coverpoint of a "
                           "non-integral expression (IEEE 1800-2023 19.5.1.1)");
        }
        જો (!VN_IS(nodep->subp(), CoverpointRef)) {
            widthCovergroupRanges(nodep->subp(), m_vup->dtypep()->width());
        }
        widthCoverFilter(nodep->filterp(), "Bin");
    }
    શૂન્ય visit(AstCoverCrossWith* nodep) override {
        userIterateAndNext(nodep->selectp(), nullptr);
        // Each cross item has the type of its coverpoint (IEEE 1800-2023 19.6.1.3)
        userIterateAndNext(nodep->varsp(), nullptr);
        widthCoverFilter(nodep->filterp(), "Cross bin");
        જો (Ast_ગાંઠ_પદાવલિ* અચલ countp = nodep->countp()) {
            જો (VN_IS(countp, Unbounded)) {
                countp->dtypeSetInt();  // 'matches $'
            } નહીંતર {
                // The count of 'matches' is self-determined, which V3Covergroup checks once folded
                userIterateAndNext(countp, WidthVP{SELF, BOTH}.p());
                V3Const::constifyEdit(nodep->countp());  // countp may change
            }
        }
    }
    // A 'with' filter is true for a nonzero value (IEEE 1800-2023 12.4), of a type assignment
    // compatible with an integral type (19.5.1.1); 'whatp' names its bins
    શૂન્ય widthCoverFilter(Ast_ગાંઠ_પદાવલિ* filterp, અચલ અક્ષર* whatp) {
        filterp = VN_AS(userIterateSubtreeReturnEdits(filterp, WidthVP{SELF, BOTH}.p()), ગાંઠ_પદાવલિ);
        File_લીટી* અચલ fl = filterp->fileline();
        અચલ Ast_ગાંઠ_DType* અચલ resultp = filterp->dtypep()->skipRefp();
        જો (resultp->isDouble()) {
            VNRelinker relinker;
            filterp->unlinkFrBack(&relinker);
            relinker.relink(
                નવું AstNeqD{fl, filterp, નવું AstConst{fl, AstConst::RealDouble{}, 0.0}});
        } નહીંતર જો (!resultp->isIntegralOrPacked()) {
            filterp->v3error(whatp << " 'with' filter must be assignment compatible with an "
                                      "integral type, not "
                                   << resultp->prettyDTypeNameQ() << " (IEEE 1800-2023 19.5.1.1)");
            filterp->replaceWith(નવું AstConst{fl, AstConst::BitFalse{}});
            VL_DO_DANGLING(ધકેલો_Deletep(filterp), filterp);
        } નહીંતર {
            fixWidthReduce(filterp);
        }
    }
    શૂન્ય visit(AstCoverTransSet* nodep) override { userIterateAndNext(nodep->itemsp(), m_vup); }
    શૂન્ય visit(AstCoverTransItem* nodep) override {
        widthCovergroupRanges(nodep->valuesp(), m_vup ? m_vup->dtypep()->width() : 0);
    }
    શૂન્ય visit(AstPow* nodep) override {
        // Pow is special, output sign only depends on LHS sign, but
        // function result depends on both signs
        // RHS is self-determined (IEEE)
        // Real if either side is real (as with AstAdd)
        assert_At_પદાવલિ(nodep);
        જો (m_vup->prelim()) {
            userIterateAndNext(nodep->lhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            userIterateAndNext(nodep->rhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            જો (nodep->lhsp()->isDouble() || nodep->rhsp()->isDouble()) {
                spliceCvtD(nodep->lhsp());
                spliceCvtD(nodep->rhsp());
                VL_DO_DANGLING(replaceWithDVersion(nodep), nodep);
                પાછા;
            }
            checkCvtUS(nodep->lhsp(), ખોટું);
            iterateCheckSizedSelf(nodep, "RHS", nodep->rhsp(), SELF, BOTH);
            nodep->dtypeFrom(nodep->lhsp());
            જો (!nodep->isFourstateOrNoDtype()
                && (nodep->rhsp()->isFourstateOrNoDtype()
                    || (nodep->rhsp()->isSigned() && !nodep->lhsp()->isNeqZero()
                        && !(VN_IS(nodep->rhsp(), Const)
                             && !VN_AS(nodep->rhsp(), Const)->અંક().isNegative())))) {
                // Change dtype to four-state if it may be 0 ** negative
                nodep->dtypeSetLogicUnsized(nodep->width(), nodep->widthMin(),
                                            nodep->dtypep()->numeric());
            }
        }

        જો (m_vup->final()) {
            Ast_ગાંઠ_DType* expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
            Ast_ગાંઠ_DType* અચલ lhsDTypep = nodep->lhsp()->dtypep()->skipRefp();
            expDTypep
                = expDTypep->findBitOrLogicDType(expDTypep->width(), expDTypep->widthMin(),
                                                 lhsDTypep->numeric(), expDTypep->isFourstate());
            nodep->dtypep(expDTypep);
            iterateCheck(nodep, "LHS", nodep->lhsp(), CONTEXT_DET, FINAL, expDTypep, EXTEND_EXP);
            Ast_ગાંઠ* newp = nullptr;  // No change
            જો (nodep->lhsp()->isSigned() && nodep->rhsp()->isSigned()) {
                newp = નવું AstPowSS{nodep->fileline(), nodep->lhsp()->unlinkFrBack(),
                                    nodep->rhsp()->unlinkFrBack()};
            } નહીંતર જો (nodep->lhsp()->isSigned() && !nodep->rhsp()->isSigned()) {
                newp = નવું AstPowSU{nodep->fileline(), nodep->lhsp()->unlinkFrBack(),
                                    nodep->rhsp()->unlinkFrBack()};
            } નહીંતર જો (!nodep->lhsp()->isSigned() && nodep->rhsp()->isSigned()) {
                newp = નવું AstPowUS{nodep->fileline(), nodep->lhsp()->unlinkFrBack(),
                                    nodep->rhsp()->unlinkFrBack()};
            }
            જો (newp) {
                newp->dtypeFrom(nodep);
                UINFO(9, "powOld " << nodep);
                UINFO(9, "powNew " << newp);
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
            }
        }
    }
    શૂન્ય visit(AstPowSU* nodep) override {
        // POWSU/SS/US only created here, dtype already determined, so
        // nothing to do in this function
        userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
        userIterateAndNext(nodep->rhsp(), WidthVP{SELF, BOTH}.p());
    }
    શૂન્ય visit(AstPowSS* nodep) override {
        // POWSU/SS/US only created here, dtype already determined, so
        // nothing to do in this function
        userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
        userIterateAndNext(nodep->rhsp(), WidthVP{SELF, BOTH}.p());
    }
    શૂન્ય visit(AstPowUS* nodep) override {
        // POWSU/SS/US only created here, dtype already determined, so
        // nothing to do in this function
        userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
        userIterateAndNext(nodep->rhsp(), WidthVP{SELF, BOTH}.p());
    }
    શૂન્ય visit(Ast_ગણતરી_Bits* nodep) override {
        assert_At_પદાવલિ(nodep);
        જો (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->lhsp(), SELF, BOTH);
            iterateCheckSizedSelf(nodep, "RHS", nodep->rhsp(), SELF, BOTH);
            iterateCheckSizedSelf(nodep, "THS", nodep->thsp(), SELF, BOTH);
            iterateCheckSizedSelf(nodep, "FHS", nodep->fhsp(), SELF, BOTH);
            // For widthMin, if a 32 bit number, we need a 6 bit number as we need to return '32'.
            અચલ પૂર્ણાંક widthMin = V3_સંખ્યા::log2b(nodep->lhsp()->width()) + 1;
            nodep->dtypeSetBitUnsized(32, widthMin, VSigning::SIGNED);
        }
    }
    શૂન્ય visit(Ast_ગણતરી_Ones* nodep) override {
        જો (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->lhsp(), SELF, BOTH);
            // For widthMin, if a 32 bit number, we need a 6 bit number as we need to return '32'.
            અચલ પૂર્ણાંક widthMin = V3_સંખ્યા::log2b(nodep->lhsp()->width()) + 1;
            nodep->dtypeSetBitUnsized(32, widthMin, VSigning::SIGNED);
        }
    }
    શૂન્ય visit(AstCvtPackString* nodep) override {
        જો (nodep->didWidthAndSet()) પાછા;
        // Opaque returns, so arbitrary
        userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH, currentStreamUse()}.p());
        // Type set in constructor
        // A class handle is not a legal implicit conversion to string
        જો (Ast_ગાંઠ_DType* અચલ dt = nodep->lhsp()->dtypep()) {
            જો (VN_IS(dt->skipRefToEnump(), ClassRefDType)) {
                nodep->lhsp()->v3error(
                    "Cannot convert 'class' handle to a string:" << dt->prettyDTypeNameQ());
            }
        }
    }
    શૂન્ય visit(AstCvtPackedToArray* nodep) override {
        જો (nodep->didWidthAndSet()) પાછા;
        // Opaque returns, so arbitrary
        userIterateAndNext(nodep->fromp(), WidthVP{SELF, BOTH, currentStreamUse()}.p());
        // Type set in constructor
    }
    શૂન્ય visit(AstCvtArrayToArray* nodep) override {
        જો (nodep->didWidthAndSet()) પાછા;
        // Opaque returns, so arbitrary
        userIterateAndNext(nodep->fromp(), WidthVP{SELF, BOTH, currentStreamUse()}.p());
        // Type set in constructor
    }
    શૂન્ય visit(AstCvtArrayToPacked* nodep) override {
        જો (nodep->didWidthAndSet()) પાછા;
        // Opaque returns, so arbitrary
        userIterateAndNext(nodep->fromp(), WidthVP{SELF, BOTH, currentStreamUse()}.p());
        // Type set in constructor
    }
    શૂન્ય visit(AstCvtUnpackToQueue* nodep) override {
        જો (nodep->didWidthAndSet()) પાછા;
        // Opaque returns, so arbitrary
        userIterateAndNext(nodep->fromp(), WidthVP{SELF, BOTH}.p());
        // Type set in constructor
    }
    શૂન્ય visit(AstCvtUnpackToUnpack* nodep) override {
        // Type set when created, the value converted is of its own type
        જો (m_vup->final()) {
            userIterateAndNext(nodep->fromp(), WidthVP{nodep->fromp()->dtypep(), FINAL}.p());
        }
    }
    શૂન્ય visit(AstTimeImport* nodep) override {
        // LHS is a real number in seconds
        // Need to round to time units and precision
        userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
        અચલ AstConst* અચલ constp = VN_CAST(nodep->lhsp(), Const);
        UASSERT_OBJ(constp && constp->isDouble(), nodep, "Times should be doubles");
        UASSERT_OBJ(!nodep->timeunit().isNone(), nodep, "$time import no units");
        અચલ દ્વિગુણ timePrescale = constp->અંક().toDouble();
        UASSERT_OBJ(!v3Global.rootp()->timeprecision().isNone(), nodep, "Never set precision?");
        અચલ દ્વિગુણ time = timePrescale / nodep->timeunit().multiplier();
        // IEEE claims you should round to time precision here, but no simulator seems to do this
        AstConst* અચલ newp = નવું AstConst{nodep->fileline(), AstConst::RealDouble{}, time};
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_વૃક્ષ(), nodep);
    }
    શૂન્ય visit(AstEventControl* nodep) override {
        જો (!m_underFork && VN_IS(m_ftaskp, Func)) {
            nodep->v3error("Event controls are not legal in functions. Suggest use a task "
                           "(IEEE 1800-2023 13.4.4)");
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_વૃક્ષ(), nodep);
            પાછા;
        }
        જો (nodep->fileline()->timingOn()) {
            જો (v3Global.opt.timing().isSetTrue()) {
                iterateChildren(nodep);
                પાછા;
            } નહીંતર જો (v3Global.opt.timing().isSetFalse()) {
                nodep->v3warn(E_NOTIMING,
                              "Event control statement in this location requires --timing\n"
                                  << nodep->warnMore()
                                  << "... With --no-timing, suggest have one event control "
                                  << "statement per procedure, at the top of the procedure");
            } નહીંતર {
                nodep->v3warn(E_NEEDTIMINGOPT, "Use --timing or --no-timing to specify how "
                                               "event controls should be handled");
            }
        }
        જો (nodep->stmtsp()) nodep->addNextHere(nodep->stmtsp()->unlinkFrBack());
        VL_DO_DANGLING(nodep->unlinkFrBack()->delete_વૃક્ષ(), nodep);
    }
    // Name of the class a class extends, for $typename, as other simulators give it. Only the
    // class it directly extends, as $typename of that class names the classes that one extends.
    સ્થિર string typenameExtends(અચલ AstClass* classp) {
        // An interface class may extend several, so those are not named
        જો (!classp || classp->isInterfaceClass()) પાછા "";
        માટે (અચલ AstClassExtends* extendsp = classp->extendsp(); extendsp;
             extendsp = VN_AS(extendsp->nextp(), ClassExtends)) {
            જો (extendsp->isImplements()) ચાલુ;
            અચલ AstClass* અચલ basep = extendsp->classOrNullp();
            પાછા basep ? " extends class " + basep->dtypeName(સાચું) : "";
        }
        પાછા "";
    }
    શૂન્ય visit(AstAttrOf* nodep) override {
        VL_RESTORER(m_attrp);
        m_attrp = nodep;
        userIterateAndNext(nodep->fromp(), WidthVP{SELF, BOTH}.p());
        જો (nodep->dimp()) userIterateAndNext(nodep->dimp(), WidthVP{SELF, BOTH}.p());
        // Don't iterate children, don't want to lose VarRef.
        પસંદગી (nodep->attrType()) {
        સ્થિતિ VAttrType::DIM_DIMENSIONS:
        સ્થિતિ VAttrType::DIM_UNPK_DIMENSIONS: {
            Ast_ગાંઠ_DType* અચલ dtypep = fromDTypep(nodep->fromp());
            UASSERT_OBJ(dtypep, nodep, "Unsized expression");
            અચલ std::pair<uint32_t, uint32_t> dim = dtypep->dimensions(સાચું);
            અચલ પૂર્ણાંક val
                = (nodep->attrType() == VAttrType::DIM_UNPK_DIMENSIONS ? dim.second
                                                                       : (dim.first + dim.second));
            nodep->replaceWith(નવું AstConst(nodep->fileline(), AstConst::Signed32{}, val));
            VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
            તોડો;
        }
        સ્થિતિ VAttrType::DIM_BITS_OR_સંખ્યા: {
            // If dtype, compute DIM_BITS, else take expression as a number to use
            જો (VN_IS(nodep->fromp(), ગાંઠ_પદાવલિ)) {
                nodep->replaceWith(nodep->fromp()->unlinkFrBack());
                VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
            } નહીંતર {
                Ast_ગાંઠ* newp = નવું AstAttrOf{nodep->fileline(), VAttrType::DIM_BITS,
                                              nodep->fromp()->unlinkFrBack()};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
                userIterateAndNext(newp, WidthVP{SELF, BOTH}.p());  // Convert AttrOf
            }
            પાછા;
        }
        સ્થિતિ VAttrType::DIM_BITS:
        સ્થિતિ VAttrType::DIM_ઉપલું:
        સ્થિતિ VAttrType::DIM_INCREMENT:
        સ્થિતિ VAttrType::DIM_ડાબું:
        સ્થિતિ VAttrType::DIM_નીચલું:
        સ્થિતિ VAttrType::DIM_જમણું:
        સ્થિતિ VAttrType::DIM_SIZE: {
            Ast_ગાંઠ_DType* અચલ dtypep = fromDTypep(nodep->fromp());
            UASSERT_OBJ(dtypep, nodep, "Unsized expression");
            // Only worth asking while parameters are still being worked out.
            જો (m_paramsOnly) {
                // A module that is still being copied does not have its final sizes.
                અચલ Ast_ગાંઠ_Module* અચલ ownModp = v3Global.rootp()->containingModule(dtypep);
                જો (ownModp && ownModp->parameterizedTemplate() && !ownModp->dead()) {
                    UINFO(9, "size deferred, type still on template " << ownModp->name());
                    // These queries always give an int, so set that now and let the
                    // value be worked out once the copy exists.
                    nodep->dtypeSetInt();
                    પાછા;
                }
            }

            જો (VN_IS(dtypep, QueueDType) || VN_IS(dtypep, DynArrayDType)) {
                પસંદગી (nodep->attrType()) {
                સ્થિતિ VAttrType::DIM_SIZE: {
                    Ast_ગાંઠ_પદાવલિ* અચલ fromp = VN_AS(nodep->fromp()->unlinkFrBack(), ગાંઠ_પદાવલિ);
                    Ast_ગાંઠ* અચલ newp
                        = નવું AstCMethodHard{nodep->fileline(), fromp, VCMethod::DYN_SIZE};
                    newp->dtypeSetInt();
                    newp->didWidth(સાચું);
                    newp->protect(ખોટું);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(nodep->delete_વૃક્ષ(), nodep);
                    તોડો;
                }
                સ્થિતિ VAttrType::DIM_ડાબું:
                સ્થિતિ VAttrType::DIM_નીચલું: {
                    Ast_ગાંઠ* અચલ newp = નવું AstConst(nodep->fileline(), AstConst::Signed32{}, 0);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
                    તોડો;
                }
                સ્થિતિ VAttrType::DIM_જમણું:
                સ્થિતિ VAttrType::DIM_ઉપલું: {
                    Ast_ગાંઠ_પદાવલિ* અચલ fromp = VN_AS(nodep->fromp()->unlinkFrBack(), ગાંઠ_પદાવલિ);
                    Ast_ગાંઠ_પદાવલિ* અચલ sizep
                        = નવું AstCMethodHard{nodep->fileline(), fromp, VCMethod::DYN_SIZE};
                    sizep->dtypeSetInt();
                    sizep->didWidth(સાચું);
                    sizep->protect(ખોટું);
                    Ast_ગાંઠ* અચલ newp
                        = નવું AstSub{nodep->fileline(), sizep,
                                     નવું AstConst(nodep->fileline(), AstConst::Signed32{}, 1)};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(nodep->delete_વૃક્ષ(), nodep);
                    તોડો;
                }
                સ્થિતિ VAttrType::DIM_INCREMENT: {
                    Ast_ગાંઠ_પદાવલિ* અચલ newp
                        = નવું AstConst(nodep->fileline(), AstConst::Signed32{}, -1);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
                    તોડો;
                }
                સ્થિતિ VAttrType::DIM_BITS: {
                    જો (VN_IS(dtypep, DynArrayDType)) {
                        nodep->v3warn(E_UNSUPPORTED, "Unsupported: $bits for dynamic array");
                    } નહીંતર {
                        nodep->v3warn(E_UNSUPPORTED, "Unsupported: $bits for queue");
                    }
                    તોડો;
                }
                મૂળભૂત: nodep->v3fatalSrc("Unhandled attribute type");
                }
            } નહીંતર {
                અચલ std::pair<uint32_t, uint32_t> dimpair = dtypep->skipRefp()->dimensions(સાચું);
                અચલ uint32_t msbdim = dimpair.first + dimpair.second;
                જો (!nodep->dimp() || msbdim < 1) {
                    જો (VN_IS(dtypep, BasicDType) && dtypep->basicp()->isString()) {
                        // IEEE undocumented but $bits(string) must give length(string) * 8
                        Ast_ગાંઠ_પદાવલિ* અચલ fromp = VN_AS(nodep->fromp()->unlinkFrBack(), ગાંઠ_પદાવલિ);
                        Ast_ગાંઠ* અચલ newp = નવું AstShiftL{
                            nodep->fileline(), નવું AstLenN{nodep->fileline(), fromp},
                            નવું AstConst{nodep->fileline(), 3},  // * 8
                            32};
                        nodep->replaceWith(newp);
                        VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
                    } નહીંતર {
                        Ast_ગાંઠ_DType* અચલ baseDTypep = dtypep->skipRefp();
                        UASSERT_OBJ(baseDTypep, nodep, "Unsized expression");
                        અચલ પૂર્ણાંક dim = 1;
                        AstConst* અચલ newp = dimension_કિંમત(nodep->fileline(), baseDTypep,
                                                              nodep->attrType(), dim);
                        nodep->replaceWith(newp);
                        VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
                    }
                } નહીંતર જો (VN_IS(nodep->dimp(), Const)) {
                    અચલ પૂર્ણાંક dim = VN_AS(nodep->dimp(), Const)->toSInt();
                    AstConst* અચલ newp
                        = dimension_કિંમત(nodep->fileline(), dtypep, nodep->attrType(), dim);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
                } નહીંતર {  // Need a runtime lookup table.  Yuk.
                    UASSERT_OBJ(nodep->fromp() && dtypep, nodep, "Unsized expression");
                    AstVarRef* અચલ tabRefp = dimensionVarRefp(dtypep, nodep->attrType(), msbdim);
                    Ast_ગાંઠ_પદાવલિ* અચલ dimp = nodep->dimp()->unlinkFrBack();
                    Ast_ગાંઠ_પદાવલિ* અચલ newp = નવું AstArraySel{nodep->fileline(), tabRefp, dimp};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
                }
            }
            તોડો;
        }
        સ્થિતિ VAttrType::FUNC_ARG_PROTO:
            // Created by V3LinkDot only to check that the prototype was correct when we got here
            // Checked at bottom of visit(AstNodeFTask)
            // V3WidthCommit::visit(AstAttrOf) will delete nodep
            તોડો;
        સ્થિતિ VAttrType::FUNC_RETURN_PROTO: {
            // Created by V3LinkDot only to check that the prototype was correct when we got here
            UASSERT_OBJ(m_ftaskp, nodep, "FUNC attr not under function");
            Ast_ગાંઠ_DType* અચલ protoDtp = nodep->fromp()->dtypep();
            Ast_ગાંઠ_DType* અચલ declDtp = [&]() {
                જો (m_ftaskp->fvarp()) પાછા m_ftaskp->fvarp()->dtypep();
                Ast_ગાંઠ_DType* અચલ voidp = નવું AstVoidDType{m_ftaskp->fileline()};
                ધકેલો_Deletep(voidp);  // Note voidp used past here
                પાછા voidp;
            }();
            જો (!similarDTypeRecurse(protoDtp, declDtp)) {
                protoDtp->v3warn(
                    PROTOTYPEMIS,
                    "In prototype for "
                        << m_ftaskp->prettyNameQ()
                        << ", return data type does not match out-of-block"
                           " declaration data-type (IEEE 1800-2023 8.24)\n"
                        << protoDtp->warnMore()
                        << "... Prototype data type: " << protoDtp->prettyDTypeNameQ() << '\n'
                        << protoDtp->warnMore()
                        << "... Declaration data type: " << declDtp->prettyDTypeNameQ() << '\n'
                        << protoDtp->warnContextPrimary() << '\n'
                        << declDtp->warnOther() << "... Location of out-of-block declaration\n"
                        << declDtp->warnContextSecondary());
            }
            // V3WidthCommit::visit(AstAttrOf) will delete nodep
            તોડો;
        }
        સ્થિતિ VAttrType::TYPENAME: {
            UASSERT_OBJ(nodep->fromp(), nodep, "Unprovided expression");
            Ast_ગાંઠ_DType* અચલ dtypep = nodep->fromp()->dtypep();
            string result = dtypep->prettyDTypeName(સાચું);
            જો (અચલ AstClassRefDType* અચલ classRefp
                = VN_CAST(dtypep->skipRefOrNullp(), ClassRefDType)) {
                result += typenameExtends(classRefp->classp());
            }
            UINFO(9, "typename '" << result << "' from " << dtypep);
            Ast_ગાંઠ* અચલ newp = નવું AstConst{nodep->fileline(), AstConst::String{}, result};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_વૃક્ષ(), nodep);
            તોડો;
        }
        સ્થિતિ VAttrType::TYPEID:
            જો (Ast_ગાંઠ_DType* dtypep = VN_CAST(nodep->fromp(), ગાંઠ_DType)) {
                nodep->dtypep(dtypep);
            } નહીંતર {
                nodep->dtypep(VN_AS(nodep->fromp(), ગાંઠ_પદાવલિ)->dtypep());
            }
            તોડો;
        સ્થિતિ VAttrType::VAR_BASE:
            // Soon to be handled in V3LinkWidth SEL generation, under attrp() and newSubLsbOf
            તોડો;
        મૂળભૂત: {
            // Everything else resolved earlier
            nodep->dtypeSetBitUnsized(32, 1, VSigning::UNSIGNED);  // Approximation, unsized 32
            UINFO(1, "Missing ATTR type case node: " << nodep);
            nodep->v3fatalSrc("Missing ATTR type case");
            તોડો;
        }
        }
    }
    શૂન્ય visit(AstPull* nodep) override {
        // May have select underneath, let seek natural size
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
    }
    શૂન્ય visit(AstText* nodep) override {
        // Only used in CStmts which don't care....
    }

    શૂન્ય visit(AstCellArrayRef* nodep) override {
        જો (nodep->didWidthAndSet()) પાછા;
        userIterateAndNext(nodep->selp(), WidthVP{SELF, PRELIM}.p());
        userIterateAndNext(nodep->selp(), WidthVP{SELF, FINAL}.p());
        nodep->dtypeSetVoid();  // placeholder; this node shouldnt survive beyond linking
        nodep->didWidth(સાચું);
    }

    શૂન્ય visit(AstCellRef* nodep) override {
        જો (nodep->didWidthAndSet()) પાછા;

        જો (Ast_ગાંઠ_પદાવલિ* અચલ cellExprp = VN_CAST(nodep->cellp(), ગાંઠ_પદાવલિ)) {
            userIterateAndNext(cellExprp, WidthVP{SELF, PRELIM}.p());
            userIterateAndNext(cellExprp, WidthVP{SELF, FINAL}.p());
        }

        જો (Ast_ગાંઠ_પદાવલિ* અચલ exprp = VN_CAST(nodep->exprp(), ગાંઠ_પદાવલિ)) {
            userIterateAndNext(exprp, WidthVP{SELF, PRELIM}.p());
            nodep->dtypeFrom(exprp);
            userIterateAndNext(exprp, WidthVP{SELF, FINAL}.p());
        } નહીંતર {
            nodep->v3fatalSrc("CellRef exprp is not a NodeExpr: "  // LCOV_EXCL_LINE
                              << nodep->exprp()->prettyTypeName());
        }

        nodep->didWidth(સાચું);
    }

    // DTYPES
    શૂન્ય visit(Ast_ગાંઠ_Array_DType* nodep) override {
        જો (nodep->didWidthAndSet()) પાછા;  // This node is a dtype & not both PRELIMed+FINALed

        જો (nodep->subDTypep() == nodep->basicp()) {  // Innermost dimension
            AstBasicDType* અચલ basicp = nodep->basicp();
            // If basic dtype is LOGIC_IMPLICIT, it is actually 1 bit LOGIC
            જો (basicp->implicit()) {
                UASSERT_OBJ(basicp->width() <= 1, basicp,
                            "must be 1 bit but actually " << basicp->width() << " bits");
                AstBasicDType* અચલ newp = નવું AstBasicDType{
                    basicp->fileline(), VBasicDTypeKwd::LOGIC, basicp->numeric()};
                newp->widthForce(1, 1);
                basicp->replaceWith(newp);
                VL_DO_DANGLING(ધકેલો_Deletep(basicp), basicp);
            }
        }
        VL_RESTORER(m_underPackedArray);
        જો (VN_IS(nodep, PackArrayDType)) m_underPackedArray = સાચું;
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));

        // Cleanup array size
        userIterateAndNext(nodep->rangep(), WidthVP{SELF, BOTH}.p());
        nodep->dtypep(nodep);  // The array itself, not subDtype
        જો (AstUnpackArrayDType* અચલ adtypep = VN_CAST(nodep, UnpackArrayDType)) {
            // Historically array elements have width of the ref type not the full array
            nodep->widthFromSub(nodep->subDTypep());
            જો (nodep->subDTypep()->skipRefp()->isCompound()) adtypep->isCompound(સાચું);
        } નહીંતર {
            અચલ Ast_ગાંઠ_DType* અચલ elemDTypep = nodep->subDTypep()->skipRefp();
            જો (!elemDTypep->isIntegralOrPacked()) {
                nodep->v3error("Unpacked data type " << elemDTypep->prettyDTypeNameQ()
                                                     << " in packed array (IEEE 1800-2023 7.4.1)");
            }
            અચલ પૂર્ણાંક width = nodep->subDTypep()->width() * nodep->rangep()->elementsConst();
            nodep->widthForce(width, width);
            જો (!VL_RESTORER_PREV(m_underPackedArray)) {  // Outermost dimension
                // IEEE 1800-2023 7.4.1 "Packed arrays" says
                //   If a packed array is declared as signed,
                //   then the array viewed as a single vector shall be signed.
                અચલ Ast_ગાંઠ_DType* baseDTypep = nodep->subDTypep();
                જ્યારે (!VN_IS(baseDTypep, RefDType) && baseDTypep->subDTypep()) {
                    baseDTypep = baseDTypep->subDTypep();
                }
                અચલ AstBasicDType* અચલ basicp = nodep->basicp();
                જો (!VN_IS(baseDTypep, RefDType) && basicp && basicp->isSigned()) {
                    nodep->numeric(VSigning::fromBool(સાચું));
                }
            }
        }
        UINFO(4, "dtWidthed " << nodep);
    }
    શૂન્ય visit(AstAssocArrayDType* nodep) override {
        જો (nodep->didWidthAndSet()) પાછા;  // This node is a dtype & not both PRELIMed+FINALed
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        nodep->ચાવી_DTypep(iterateEditMoveDTypep(nodep, nodep->ચાવી_DTypep()));
        nodep->dtypep(nodep);  // The array itself, not subDtype
        UINFO(4, "dtWidthed " << nodep);
    }
    શૂન્ય visit(AstBracketArrayDType* nodep) override {
        // Type inserted only because parser didn't know elementsp() type
        // Resolve elementsp's type
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
        // We must edit when dtype still under normal nodes and before type table
        // See notes in iterateEditMoveDTypep
        Ast_ગાંઠ_DType* અચલ childp = nodep->childDTypep();
        childp->unlinkFrBack();
        Ast_ગાંઠ* અચલ elementsp = nodep->elementsp()->unlinkFrBack();
        Ast_ગાંઠ* newp;
        જો (VN_IS(elementsp, Unbounded)) {
            newp = નવું AstQueueDType{nodep->fileline(), VFlagChildDType{}, childp, nullptr};
            VL_DO_DANGLING(elementsp->delete_વૃક્ષ(), elementsp);
        } નહીંતર જો (Ast_ગાંઠ_DType* અચલ keyp = VN_CAST(elementsp, ગાંઠ_DType)) {
            newp = નવું AstAssocArrayDType{nodep->fileline(), VFlagChildDType{}, childp, keyp};
        } નહીંતર {
            // The subtract in the range may confuse users; as the array
            // size is self determined there's no reason to warn about widths
            File_લીટી* અચલ elementsNewFl = elementsp->fileline();
            elementsNewFl->warnOff(V3ErrorCode::WIDTHEXPAND, સાચું);
            // Must be expression that is constant, but we'll determine that later
            newp = નવું AstUnpackArrayDType{
                nodep->fileline(), VFlagChildDType{}, childp,
                નવું AstRange{nodep->fileline(), નવું AstConst(elementsp->fileline(), 0),
                             નવું AstSub{elementsNewFl, VN_AS(elementsp, ગાંઠ_પદાવલિ),
                                        નવું AstConst(elementsNewFl, 1)},
                             સાચું}};
        }
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_વૃક્ષ(), nodep);
        // Normally parent's iteration would cover this, but we might have entered by a specific
        // visit
        VL_DO_DANGLING(userIterate(newp, nullptr), newp);
    }
    શૂન્ય visit(AstConstraintRefDType* nodep) override {
        જો (nodep->didWidthAndSet()) પાછા;  // This node is a dtype & not both PRELIMed+FINALed
        nodep->dtypep(nodep);
    }
    શૂન્ય visit(AstDynArrayDType* nodep) override {
        જો (nodep->didWidthAndSet()) પાછા;  // This node is a dtype & not both PRELIMed+FINALed
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        nodep->dtypep(nodep);  // The array itself, not subDtype
        UINFO(4, "dtWidthed " << nodep);
    }
    શૂન્ય visit(AstQueueDType* nodep) override {
        જો (nodep->didWidthAndSet()) પાછા;  // This node is a dtype & not both PRELIMed+FINALed
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        nodep->dtypep(nodep);  // The array itself, not subDtype
        userIterateAndNext(nodep->boundp(), WidthVP{SELF, BOTH}.p());
        જો (VN_IS(nodep->boundp(), Unbounded)) {
            nodep->boundp()->unlinkFrBack()->delete_વૃક્ષ();  // nullptr will represent unbounded
        }
        UINFO(4, "dtWidthed " << nodep);
    }
    શૂન્ય visit(AstVoidDType* nodep) override {
        જો (nodep->didWidthAndSet()) પાછા;  // This node is a dtype & not both PRELIMed+FINALed
        nodep->dtypep(nodep);
        UINFO(4, "dtWidthed " << nodep);
    }
    શૂન્ય visit(AstUnsizedArrayDType* nodep) override {
        જો (nodep->didWidthAndSet()) પાછા;  // This node is a dtype & not both PRELIMed+FINALed
        જો (nodep->childDTypep() && !(m_ftaskp && m_ftaskp->dpiImport())) {
            nodep->v3error("Unsized packed dimension '[]' is only legal in a DPI import"
                           " (IEEE 1800-2023 A.2.2.1)");
        }
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        // Cleanup array size
        nodep->dtypep(nodep);  // The array itself, not subDtype
        UINFO(4, "dtWidthed " << nodep);
    }
    શૂન્ય visit(AstWildcardArrayDType* nodep) override {
        જો (nodep->didWidthAndSet()) પાછા;  // This node is a dtype & not both PRELIMed+FINALed
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        // Cleanup array size
        nodep->dtypep(nodep);  // The array itself, not subDtype
        UINFO(4, "dtWidthed " << nodep);
    }
    શૂન્ય visit(AstBasicDType* nodep) override {
        જો (nodep->didWidthAndSet()) પાછા;  // This node is a dtype & not both PRELIMed+FINALed
        જો (nodep->generic()) પાછા;  // Already perfect
        જો (nodep->rangep()) {
            userIterateAndNext(nodep->rangep(), WidthVP{SELF, BOTH}.p());
            // Because this DType has a unique child range, we know it's not
            // pointed at by other nodes unless they are referencing this type.
            // Furthermore the width() calculation would return identical
            // values.  Therefore we can directly replace the width
            nodep->widthForce(nodep->rangep()->elementsConst(), nodep->rangep()->elementsConst());
        } નહીંતર જો (nodep->isRanged()) {
            nodep->widthForce(nodep->nrange().elements(), nodep->nrange().elements());
        } નહીંતર જો (nodep->implicit()) {
            // Parameters may notice implicitness and change to different dtype
            nodep->widthForce(1, 1);
        }
        // else width in node is correct; it was set based on keyword().width()
        // at construction time.  Ditto signed, so "unsigned byte" etc works right.
        nodep->cvtRangeConst();
        // TODO: If BasicDType now looks like a generic type, we can convert to a real generic
        // dtype Instead for now doing this in V3WidthCommit
        UINFO(4, "dtWidthed " << nodep);
    }
    શૂન્ય visit(AstConstDType* nodep) override {
        જો (nodep->didWidthAndSet()) પાછા;  // This node is a dtype & not both PRELIMed+FINALed
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        userIterateChildren(nodep, nullptr);
        nodep->dtypep(nodep);  // Should already be set, but be clear it's not the subDType
        nodep->widthFromSub(nodep->subDTypep());
        UINFO(4, "dtWidthed " << nodep);
    }
    શૂન્ય visit(AstRefDType* nodep) override {
        જો (nodep->didWidthAndSet()) પાછા;  // This node is a dtype & not both PRELIMed+FINALed
        nodep->doingWidth(સાચું);
        જો (nodep->typeofp()) {  // type(typeofp_expression)
            જો (Ast_ગાંઠ_DType* typeofDtp = VN_CAST(nodep->typeofp(), ગાંઠ_DType)) {
                // It's directly a type, e.g. "type(int)"
                typeofDtp = iterateEditMoveDTypep(nodep, typeofDtp);  // Changes typeofp
                nodep->refDTypep(typeofDtp);
            } નહીંતર {
                // Type comes from expression's type, e.g. "type(variable)"
                userIterateAndNext(nodep->typeofp(), WidthVP{SELF, BOTH}.p());
                Ast_ગાંઠ* અચલ typeofp = nodep->typeofp();
                nodep->refDTypep(typeofp->dtypep());
                VL_DO_DANGLING(typeofp->unlinkFrBack()->delete_વૃક્ષ(), typeofp);
            }
            // We had to use AstRefDType for this construct as pointers to this type
            // in type table are still correct (which they wouldn't be if we replaced the node)
        }
        userIterateChildren(nodep, nullptr);
        જો (nodep->subDTypep()) {
            // Normally iterateEditMoveDTypep iterate would work, but the refs are under
            // the TypeDef which will upset iterateEditMoveDTypep as it can't find it under
            // this node's childDTypep
            userIterate(nodep->subDTypep(), nullptr);
            nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
            // Widths are resolved, but special iterate to check for recursion
            userIterate(nodep->subDTypep(), nullptr);
        }
        // Effectively nodep->dtypeFrom(nodep->dtypeSkipRefp());
        // But might be recursive, so instead manually recurse into the referenced type
        જો (!nodep->subDTypep()) {
            // Defer unlinked RefDTypes in parameterized template modules (or types
            // with no owning module, e.g. moved to TypeTable) until specialization
            // resolves them.
            અચલ તાર્કિક inTemplateModule = !m_modep || m_modep->parameterizedTemplate();
            જો (inTemplateModule) {
                nodep->doingWidth(ખોટું);
                પાછા;
            }
        }
        UASSERT_OBJ(nodep->subDTypep(), nodep, "Unlinked");
        nodep->dtypeFrom(nodep->subDTypep());
        nodep->widthFromSub(nodep->subDTypep());
        UINFO(4, "dtWidthed " << nodep);
        // No nodep->typedefp(nullptr) for now; V3WidthCommit needs to check accesses
        nodep->doingWidth(ખોટું);
    }
    // Width the parameters of the scopes declaring typedef 'nodep', as the name of the type it
    // declares holds their values, though one may follow it, in a module without a parameter
    // port list
    શૂન્ય widthDTypeNameParams(અચલ AstTypedef* nodep) {
        અચલ VDTypeNameScopes* અચલ scopesp = VDTypeNameScopes::currentp();
        જો (!scopesp) પાછા;  // Computing parameters, as by V3Param, so naming as found
        Ast_ગાંઠ* scopep = nullptr;
        માટે (અચલ Ast_ગાંઠ* innerp = nodep; scopesp->outerp(innerp, scopep) && scopep;
             innerp = scopep) {
            જો (અચલ std::vector<Ast_ગાંઠ*>* અચલ paramsp = scopesp->paramsp(scopep)) {
                માટે (Ast_ગાંઠ* અચલ paramp : *paramsp) {
                    જો (!paramp->didWidth()) userIterate(paramp, nullptr);
                }
            }
        }
    }
    શૂન્ય visit(AstTypedef* nodep) override {
        જો (nodep->didWidthAndSet()) પાછા;  // This node is a dtype & not both PRELIMed+FINALed
        જો (auto* અચલ refp = checkRefToTypedefRecurse(nodep, nodep)) {
            nodep->v3error("Typedef has self-reference: " << nodep->prettyNameQ() << '\n'
                                                          << nodep->warnContextPrimary() << '\n'
                                                          << refp->warnOther()
                                                          << "... Location of reference\n"
                                                          << refp->warnContextSecondary());
            // May cause internal error but avoids infinite loop on dump
            refp->typedefp(nullptr);
            VL_DO_DANGLING(ધકેલો_Deletep(nodep->unlinkFrBack()), nodep);
            પાછા;
        }
        // As it moves to the type table, a structure, union, or enumeration it declares keeps the
        // name it has from the typedef, with the scope of the typedef, so with the parameters of
        // the scope, widthed first, as one may follow the typedef
        અચલ તાર્કિક declares = nodep->childDTypep() != nullptr;
        nodep->dtypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        જો (declares) {
            Ast_ગાંઠ_DType* અચલ dtypep = nodep->dtypep();
            જો (Ast_ગાંઠ_UOr_Struct_DType* અચલ sdtypep = VN_CAST(dtypep, ગાંઠ_UOr_Struct_DType)) {
                widthDTypeNameParams(nodep);
                sdtypep->typedefName(nodep->dtypeName());
            } નહીંતર જો (AstEnumDType* અચલ edtypep = VN_CAST(dtypep, EnumDType)) {
                widthDTypeNameParams(nodep);
                edtypep->typedefName(nodep->dtypeName());
            }
        }
        userIterateChildren(nodep, nullptr);
    }
    શૂન્ય visit(AstParamTypeDType* nodep) override {
        જો (nodep->didWidthAndSet()) પાછા;  // This node is a dtype & not both PRELIMed+FINALed

        nodep->dtypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        userIterateChildren(nodep, nullptr);
        nodep->widthFromSub(nodep->subDTypep());
        // Clear childDTypep after dtypep is set to satisfy V3Broken invariant.
        // The child dtype has been moved to the type table by iterateEditMoveDTypep.
        જો (nodep->dtypep() && nodep->childDTypep()) { nodep->childDTypep(nullptr); }
    }
    શૂન્ય visit(AstRequireDType* nodep) override {
        userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
        જો (Ast_ગાંઠ_DType* અચલ dtp = VN_CAST(nodep->lhsp(), ગાંઠ_DType)) {
            nodep->replaceWith(dtp->unlinkFrBack());
        } નહીંતર {
            જો (nodep->lhsp())
                nodep->lhsp()->v3error("Expected data type, not a "
                                       << nodep->lhsp()->prettyTypeName());
            nodep->replaceWith(નવું AstVoidDType{nodep->fileline()});
        }
        VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
    }
    શૂન્ય visit(AstCastDynamic* nodep) override {
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
        Ast_ગાંઠ_DType* અચલ toDtp = nodep->ટોચ()->dtypep()->skipRefToEnump();
        Ast_ગાંઠ_DType* અચલ fromDtp = nodep->fromp()->dtypep()->skipRefToEnump();
        File_લીટી* અચલ fl = nodep->fileline();
        અચલ auto castable = Ast_ગાંઠ::computeCastable(toDtp, fromDtp, nodep->fromp());
        Ast_ગાંઠ* newp;
        જો (castable == VCastable::DYNAMIC_CLASS) {
            // Keep in place, will compute at runtime
            પાછા;
        } નહીંતર જો (castable == VCastable::ENUM_EXPLICIT || castable == VCastable::ENUM_IMPLICIT) {
            // TODO is from is a constant we could simplify, though normal constant
            // elimination should do much the same
            // Form: "( ((v > size) ? false : enum_valid[v[N:0]])
            //          ? ExprStmt(ExprAssign(out, Cast(v, type)), 1) : 0)"
            AstEnumDType* અચલ enumDtp = VN_AS(toDtp, EnumDType);
            UASSERT_OBJ(enumDtp, nodep, "$cast determined as enum, but not enum type");
            Ast_ગાંઠ_પદાવલિ* અચલ testp = enumTestValid(nodep->fromp(), enumDtp);
            File_લીટી* અચલ fl_novalue = નવું File_લીટી{fl};
            fl_novalue->warnOff(V3ErrorCode::ENUMVALUE, સાચું);
            newp = નવું AstCond{
                fl, testp,
                નવું Ast_પદાવલિ_Stmt{fl,
                                નવું AstAssign{fl_novalue, nodep->ટોચ()->unlinkFrBack(),
                                              nodep->fromp()->unlinkFrBack()},
                                નવું AstConst{fl, AstConst::Signed32{}, 1}},
                નવું AstConst{fl, AstConst::Signed32{}, 0}};
        } નહીંતર જો ((castable == VCastable::SAMEISH || castable == VCastable::COMPATIBLE)
                   // Unsupported: the static cast below has no lowering for unpacked arrays
                   && !VN_IS(toDtp, UnpackArrayDType)) {
            nodep->v3warn(CASTCONST, "$cast will always return one as "
                                         << toDtp->prettyDTypeNameQ()
                                         << " is always castable from "
                                         << fromDtp->prettyDTypeNameQ() << '\n'
                                         << nodep->warnMore() << "... Suggest static cast");
            newp = નવું Ast_પદાવલિ_Stmt{
                fl,
                નવું AstAssign{fl, nodep->ટોચ()->unlinkFrBack(),
                              નવું AstCast{fl, nodep->fromp()->unlinkFrBack(), toDtp}},
                નવું AstConst{fl, AstConst::Signed32{}, 1}};
        } નહીંતર જો (castable == VCastable::INCOMPATIBLE) {
            newp = નવું AstConst{fl, 0};
            nodep->v3warn(CASTCONST, "$cast will always return zero as "
                                         << toDtp->prettyDTypeNameQ() << " is not castable from "
                                         << fromDtp->prettyDTypeNameQ());
        } નહીંતર {
            newp = નવું AstConst{fl, 0};
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: $cast to "
                                             << toDtp->prettyDTypeNameQ() << " from "
                                             << fromDtp->prettyDTypeNameQ() << '\n'
                                             << nodep->warnMore()
                                             << "... Suggest try static cast");
        }
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
        userIterate(newp, m_vup);
    }
    શૂન્ય visit(AstCastParse* nodep) override {
        // nodep->dtp could be data type, or a primary_constant
        // Don't iterate lhsp, will deal with that once convert the type
        V3Const::constifyParamsEdit(nodep->dtp());  // itemp may change
        જો (AstConst* અચલ constp = VN_CAST(nodep->dtp(), Const)) {
            constp->unlinkFrBack();
            Ast_ગાંઠ* અચલ newp
                = નવું AstCastSize{nodep->fileline(), nodep->lhsp()->unlinkFrBack(), constp};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
            userIterate(newp, m_vup);
        } નહીંતર જો (Ast_ગાંઠ_DType* અચલ refp = VN_CAST(nodep->dtp(), ગાંઠ_DType)) {
            refp->unlinkFrBack();
            Ast_ગાંઠ* અચલ newp = નવું AstCast{nodep->fileline(), nodep->lhsp()->unlinkFrBack(),
                                              VFlagChildDType{}, refp};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
            userIterate(newp, m_vup);
        } નહીંતર {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Cast to " << nodep->dtp()->prettyTypeName());
            nodep->replaceWith(nodep->lhsp()->unlinkFrBack());
            VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
        }
    }
    શૂન્ય visit(AstCast* nodep) override {
        જો (nodep->didWidth()) પાછા;
        UINFO(9, "CAST " << nodep);
        nodep->dtypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        જો (m_vup->prelim()) {
            UINFOTREE(9, nodep, "", "CastPre");
            // UINFOTREE(1, nodep->backp(), "", "CastPreUpUp");
            જો (AstSigned* અચલ fromp = VN_CAST(nodep->fromp(), Signed)) {
                જો (VN_IS(fromp->lhsp(), ગાંઠ_Stream)) {
                    Ast_ગાંઠ* અચલ lhsp = fromp->lhsp()->unlinkFrBack();
                    fromp->replaceWith(lhsp);
                    VL_DO_DANGLING(fromp->delete_વૃક્ષ(), fromp);
                }
            }
            userIterateAndNext(nodep->fromp(), WidthVP{SELF, PRELIM, STREAM_USE_CAST}.p());
            UINFOTREE(9, nodep, "", "CastDit");
            Ast_ગાંઠ_DType* અચલ toDtp = nodep->dtypep()->skipRefToEnump();
            Ast_ગાંઠ_DType* અચલ fromDtp = nodep->fromp()->dtypep()->skipRefToEnump();
            અચલ auto castable = Ast_ગાંઠ::computeCastable(toDtp, fromDtp, nodep->fromp());
            તાર્કિક bad = ખોટું;
            જો (castable == VCastable::UNSUPPORTED) {
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: static cast to "
                                                 << toDtp->prettyDTypeNameQ() << " from "
                                                 << fromDtp->prettyDTypeNameQ());
                bad = સાચું;
            } નહીંતર જો (castable == VCastable::SAMEISH || castable == VCastable::COMPATIBLE
                       || castable == VCastable::ENUM_IMPLICIT
                       || castable == VCastable::ENUM_EXPLICIT) {
                ;  // Continue
            } નહીંતર જો (castable == VCastable::DYNAMIC_CLASS) {
                nodep->v3error("Dynamic, not static cast, required to cast "
                               << toDtp->prettyDTypeNameQ() << " from "
                               << fromDtp->prettyDTypeNameQ() << '\n'
                               << nodep->warnMore() << "... Suggest dynamic $cast");
                bad = સાચું;
            } નહીંતર જો (castable == VCastable::INCOMPATIBLE) {
                nodep->v3error("Incompatible types to static cast to "
                               << toDtp->prettyDTypeNameQ() << " from "
                               << fromDtp->prettyDTypeNameQ() << '\n');
                bad = સાચું;
            } નહીંતર {
                nodep->v3fatalSrc("bad casting case");
            }
            // For now, replace it ASAP, so widthing can propagate easily
            // The cast may change signing, but we don't know the sign yet.  Make it so.
            // Note we don't sign fromp() that would make the algorithm O(n^2) if lots of casting.
            Ast_ગાંઠ_પદાવલિ* newp = nullptr;
            જો (bad) {
            } નહીંતર જો (AstBasicDType* અચલ basicp = toDtp->basicp()) {
                જો (!basicp->isString() && fromDtp->isString()) {
                    newp = નવું AstNToI{nodep->fileline(), nodep->fromp()->unlinkFrBack(), toDtp};
                } નહીંતર જો (!basicp->isDouble() && !fromDtp->isDouble()) {
                    Ast_ગાંઠ_DType* અચલ origDTypep = nodep->dtypep();
                    જો (!VN_IS(fromDtp, StreamDType)) {
                        અચલ પૂર્ણાંક width = toDtp->width();
                        castSized(nodep, nodep->fromp(), width);
                    }
                    nodep->dtypeFrom(origDTypep);  // If was enum, need dtype to preserve as enum
                    // Note castSized might modify nodep->fromp()
                } નહીંતર {
                    iterateCheck(nodep, "value", nodep->fromp(), SELF, FINAL, fromDtp, EXTEND_EXP,
                                 ખોટું);
                }
                જો (newp) {
                } નહીંતર જો (basicp->isDouble() && !nodep->fromp()->isDouble()) {
                    જો (nodep->fromp()->isSigned()) {
                        newp = નવું AstISToRD{nodep->fileline(), nodep->fromp()->unlinkFrBack()};
                    } નહીંતર {
                        newp = નવું AstIToRD{nodep->fileline(), nodep->fromp()->unlinkFrBack()};
                    }
                } નહીંતર જો (!basicp->isDouble() && nodep->fromp()->isDouble()) {
                    newp = નવું AstRToIRoundS{nodep->fileline(), nodep->fromp()->unlinkFrBack()};
                    newp->dtypep(basicp);
                } નહીંતર જો (basicp->isSigned() && !nodep->fromp()->isSigned()) {
                    newp = નવું AstSigned{nodep->fileline(), nodep->fromp()->unlinkFrBack()};
                } નહીંતર જો (!basicp->isSigned() && nodep->fromp()->isSigned()) {
                    newp = નવું AstUnsigned{nodep->fileline(), nodep->fromp()->unlinkFrBack()};
                } નહીંતર {
                    // Can just remove cast, but need extend placeholder
                    // so we can avoid warning message
                }
            } નહીંતર જો (VN_IS(toDtp, QueueDType)) {
                જો (VN_IS(fromDtp, BasicDType)) {
                    newp = નવું AstCvtPackedToArray{nodep->fileline(),
                                                   nodep->fromp()->unlinkFrBack(), toDtp};
                } નહીંતર જો (VN_IS(fromDtp, QueueDType) || VN_IS(fromDtp, StreamDType)) {
                    પૂર્ણાંક srcElementBits = 1;
                    પૂર્ણાંક dstElementBits = 1;
                    જો (Ast_ગાંઠ_DType* અચલ elemDtp = fromDtp->subDTypep()) {
                        srcElementBits = elemDtp->width();
                    }
                    અચલ AstQueueDType* અચલ dstQueueDtp = VN_AS(toDtp, QueueDType);
                    જો (Ast_ગાંઠ_DType* અચલ elemDtp = dstQueueDtp->subDTypep()) {
                        dstElementBits = elemDtp->width();
                    }
                    newp = નવું AstCvtArrayToArray{nodep->fileline(),
                                                  nodep->fromp()->unlinkFrBack(),
                                                  toDtp,
                                                  ખોટું,
                                                  1,
                                                  dstElementBits,
                                                  srcElementBits};
                }
            } નહીંતર જો (VN_IS(toDtp, ClassRefDType)) {
                // Can just remove cast
            } નહીંતર {
                nodep->v3fatalSrc("Unimplemented: Casting non-simple data type "
                                  << toDtp->prettyDTypeNameQ());
            }
            જો (!newp) newp = nodep->fromp()->unlinkFrBack();
            nodep->fromp(newp);
            UINFOTREE(9, nodep, "", "CastOut");
            // UINFOTREE(1, nodep->backp(), "", "CastOutUpUp");
        }
        જો (m_vup->final()) {
            UINFOTREE(9, nodep, "", "CastFPit");
            iterateCheck(nodep, "value", nodep->fromp(), SELF, FINAL, nodep->fromp()->dtypep(),
                         EXTEND_EXP, ખોટું, STREAM_USE_CAST);
            UINFOTREE(9, nodep, "", "CastFin");
            Ast_ગાંઠ_પદાવલિ* અચલ underp = nodep->fromp()->unlinkFrBack();
            underp->dtypeFrom(nodep);
            underp->didWidth(સાચું);
            Ast_ગાંઠ_પદાવલિ* અચલ newp = નવું AstCastWrap{nodep->fileline(), underp};
            newp->didWidth(સાચું);
            UINFOTREE(9, newp, "", "CastRep");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
        }
    }
    શૂન્ય visit(AstCastSize* nodep) override {
        // IEEE: Signedness of result is same as self-determined signedness
        // However, the result is same as BITSEL, so we do not sign extend the LHS
        // UINFOTREE(1, nodep, "", "CastSizePre");
        assert_At_પદાવલિ(nodep);
        જો (m_vup->prelim()) {
            પૂર્ણાંક width = nodep->rhsp()->toSInt();
            જો (width < 1) {
                nodep->v3error("Size-changing cast to zero or negative size: " << width);
                width = 1;
            }
            userIterateAndNext(nodep->lhsp(), WidthVP{SELF, PRELIM, STREAM_USE_CAST}.p());
            castSized(nodep, nodep->lhsp(), width);  // lhsp may change
        }
        જો (m_vup->final()) {
            // CastSize not needed once sizes determined
            Ast_ગાંઠ* અચલ underp = nodep->lhsp()->unlinkFrBack();
            nodep->replaceWithKeepDType(underp);
            VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
        }
        // UINFOTREE(1, nodep, "", "CastSizeOut");
    }
    શૂન્ય visit(AstCastWrap* nodep) override {
        // Inserted by V3Width only so we know has been resolved
        userIterateAndNext(nodep->lhsp(), WidthVP{nodep->dtypep(), BOTH, STREAM_USE_CAST}.p());
    }
    શૂન્ય castSized(Ast_ગાંઠ* nodep, Ast_ગાંઠ* underp, પૂર્ણાંક width) {
        અચલ AstBasicDType* underDtp = VN_CAST(underp->dtypep(), BasicDType);
        જો (!underDtp) underDtp = underp->dtypep()->basicp();
        જો (!underDtp) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: Size-changing cast on non-basic data type");
            underDtp = VN_AS(nodep->findBitDType(), BasicDType);
        }
        UASSERT_OBJ(underp == nodep->op1p(), nodep, "Assuming op1 is cast value");
        // A cast propagates its size to the lower expression and is included in the maximum
        // width, so 23'(1'b1 + 1'b1) uses 23-bit math, but 1'(2'h2 * 2'h1) uses two-bit math.
        // However the output width is exactly that requested.
        // So two steps, first do the calculation's width (max of the two widths)
        {
            અચલ પૂર્ણાંક calcWidth = std::max(width, underDtp->width());
            Ast_ગાંઠ_DType* અચલ calcDtp = nodep->findBitOrLogicDType(
                calcWidth, calcWidth, underDtp->numeric(), underDtp->isFourstate());
            nodep->dtypep(calcDtp);
            // We ignore warnings as that is sort of the point of a cast
            iterateCheck(nodep, "Cast expr", underp, CONTEXT_DET, FINAL, calcDtp, EXTEND_EXP,
                         ખોટું, STREAM_USE_CAST);
            VL_DANGLING(underp);
            underp = nodep->op1p();  // Above asserts that op1 was underp pre-relink
        }
        // UINFOTREE(1, nodep, "", "CastSizeClc");
        // Next step, make the proper output width
        {
            Ast_ગાંઠ_DType* અચલ outDtp = nodep->findBitOrLogicDType(
                width, width, underDtp->numeric(), underDtp->isFourstate());
            nodep->dtypep(outDtp);
            // We ignore warnings as that is sort of the point of a cast
            widthCheckSized(nodep, "Cast expr", VN_AS(underp, ગાંઠ_પદાવલિ), outDtp, EXTEND_EXP,
                            ખોટું);
            VL_DANGLING(underp);
        }
    }
    શૂન્ય visit(AstConstraintBefore* nodep) override {
        userIterateAndNext(nodep->lhssp(), WidthVP{SELF, BOTH}.p());
        userIterateAndNext(nodep->rhssp(), WidthVP{SELF, BOTH}.p());
    }
    શૂન્ય visit(Ast_Constraint_પદાવલિ* nodep) override {
        userIterateAndNext(nodep->exprp(), WidthVP{SELF, BOTH}.p());
    }
    શૂન્ય visit(AstConstraintUnique* nodep) override {
        userIterateAndNext(nodep->rangesp(), WidthVP{SELF, BOTH}.p());
    }
    શૂન્ય visit(AstDistItem* nodep) override {
        userIterate(nodep->rangep(), m_vup);
        assert_At_પદાવલિ(nodep);
        જો (m_vup->prelim()) {  // First stage evaluation
            userIterate(nodep->weightp(), WidthVP{SELF, BOTH}.p());
        }
        nodep->dtypep(nodep->rangep()->dtypep());
    }
    શૂન્ય visit(AstVar* nodep) override {
        // UINFOTREE(1, nodep, "", "InitPre");
        // Must have deterministic constant width
        // We can't skip this step when width()!=0, as creating a AstVar
        // with non-constant range gets size 1, not size 0.  So use didWidth().
        જો (nodep->didWidth()) પાછા;
        જો (nodep->doingWidth()) {  // Early exit if have circular parameter definition
            UASSERT_OBJ(nodep->valuep(), nodep, "circular, but without value");
            nodep->v3error("Variable's initial value is circular: " << nodep->prettyNameQ());
            ધકેલો_Deletep(nodep->valuep()->unlinkFrBack());
            nodep->valuep(નવું AstConst{nodep->fileline(), AstConst::BitFalseErroring{}});
            nodep->dtypeFrom(nodep->valuep());
            nodep->didWidth(સાચું);
            પાછા;
        }
        nodep->doingWidth(સાચું);
        // Make sure dtype is sized
        nodep->dtypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        UASSERT_OBJ(nodep->dtypep(), nodep, "No dtype determined for var");
        // Removed at the end, see mainAcceptEdit
        જો (!m_paramsOnly && nodep->isIfaceArraySplit()) m_splitIfaceArrayVarps.ધકેલો_back(nodep);
        જો (nodep->attrsp()) {
            nodep->attrsp()->foreach([આ, nodep](AstAttrOf* attrp) {
                જો (attrp->attrType() == VAttrType::VAR_PORT_DTYPE) {
                    V3Const::constifyParamsEdit(attrp->fromp());  // fromp may change
                    જો (!similarDTypeRecurse(nodep->dtypep(), VN_AS(attrp->fromp(), ગાંઠ_DType))) {
                        nodep->dtypep()->v3error("Non-ANSI I/O declaration of signal "
                                                 "conflicts with type declaration: "
                                                 << nodep->prettyNameQ() << '\n'
                                                 << nodep->dtypep()->warnContextPrimary() << '\n'
                                                 << attrp->warnOther()
                                                 << "... Location of other declaration\n"
                                                 << attrp->warnContextSecondary());
                    }
                    VL_DO_DANGLING(ધકેલો_Deletep(attrp->unlinkFrBack()), attrp);
                }
            });
        }
        જો (m_ftaskp && m_ftaskp->dpiImport()) {
            Ast_ગાંઠ_DType* dtp = nodep->dtypep();
            Ast_ગાંઠ_DType* np = nullptr;
            જ્યારે (VN_IS(dtp->skipRefp(), DynArrayDType)
                   || VN_IS(dtp->skipRefp(), UnpackArrayDType)) {
                જો (અચલ AstDynArrayDType* અચલ unsizedp
                    = VN_CAST(dtp->skipRefp(), DynArrayDType)) {
                    જો (!np) {
                        UINFO(9, "Dynamic becomes unsized array (var itself) " << nodep);
                    } નહીંતર {
                        UINFO(9, "Dynamic becomes unsized array (subDType) " << dtp);
                    }
                    AstUnsizedArrayDType* અચલ newp
                        = નવું AstUnsizedArrayDType{unsizedp->fileline(), unsizedp->subDTypep()};
                    newp->dtypep(newp);
                    જો (!np) {  // for Var itself
                        nodep->dtypep(newp);
                    } નહીંતર {  // for subDType
                        np->virtRefDTypep(newp);
                    }
                    v3Global.rootp()->typeTablep()->addTypesp(newp);
                    np = newp;
                } નહીંતર {
                    np = dtp->skipRefp();
                }
                dtp = np->virtRefDTypep();
            }
        }
        જો (AstWildcardArrayDType* અચલ wildp
            = VN_CAST(nodep->dtypeSkipRefp(), WildcardArrayDType)) {
            nodep->dtypep(wildp);  // Skip RefDType like for other dynamic array types
        }
        જો (VN_IS(nodep->dtypep()->skipRefToConstp(), ConstDType)) nodep->isConst(સાચું);
        // Parameters if implicit untyped inherit from what they are assigned to
        અચલ AstBasicDType* અચલ bdtypep = VN_CAST(nodep->dtypep(), BasicDType);
        તાર્કિક didchk = ખોટું;
        અચલ તાર્કિક implicitParam = nodep->isParam() && bdtypep && bdtypep->implicit();
        જો (implicitParam) {
            જો (nodep->valuep()) {
                // Remove blanket deferral. We must attempt to visit the value to determine
                // type/deps. If it remains unresolved, specific node visitors (like AttrOf) should
                // handle deferral by setting a placeholder type to prevent "No dtype" errors
                // later.
                userIterateAndNext(nodep->valuep(), WidthVP{nodep->dtypep(), PRELIM}.p());
                UINFO(9, "implicitParamPRELIMIV " << nodep->valuep());
                // Although nodep will get a different width for parameters
                // just below, we want the init numbers to retain their
                // width/minwidth until parameters are replaced.
                // This prevents width warnings at the location the parameter is substituted in
                જો (nodep->valuep()->isDouble()) {
                    nodep->dtypeSetDouble();
                    VL_DANGLING(bdtypep);
                } નહીંતર જો (nodep->valuep()->isString()) {
                    nodep->dtypeSetString();
                    VL_DANGLING(bdtypep);
                } નહીંતર {
                    પૂર્ણાંક width = 0;
                    Ast_ગાંઠ_DType* અચલ કિંમત_DTypep = nodep->valuep()->dtypep();
                    UASSERT_OBJ(કિંમત_DTypep, nodep->valuep(),
                                "Null dtype on implicit param value");
                    અચલ AstBasicDType* અચલ કિંમત_Bdtypep = કિંમત_DTypep->basicp();
                    તાર્કિક issigned = ખોટું;
                    જો (bdtypep->isNosign()) {
                        જો (કિંમત_Bdtypep && કિંમત_Bdtypep->isSigned()) issigned = સાચું;
                    } નહીંતર {
                        issigned = bdtypep->isSigned();
                    }
                    જો (કિંમત_Bdtypep->isString()) {
                        // parameter X = "str", per IEEE is a number, not a string
                        જો (અચલ auto* અચલ constp = VN_CAST(nodep->valuep(), Const)) {
                            જો (constp->અંક().isString()) {
                                width = constp->અંક().toString().length() * 8;
                            }
                        }
                        જો (width < 8) width = 8;
                    } નહીંતર જો (nodep->valuep()->dtypep()->widthSized()) {
                        width = nodep->valuep()->width();
                    } નહીંતર {
                        જો (nodep->valuep()->width() > 32) {
                            nodep->valuep()->v3warn(
                                WIDTH,
                                "Assigning >32 bit to unranged parameter (defaults to 32 bits)");
                        }
                        width = 32;
                    }
                    // Can't just inherit valuep()->dtypep() as mwidth might not equal width
                    અચલ તાર્કિક is_કિંમત_Fourstate
                        = nodep->valuep()->dtypep()->skipRefp()->isFourstate();
                    જો (width == 1) {
                        // one bit parameter is same as "parameter [0] foo",
                        // not "parameter logic foo" as you can extract
                        // "foo[0]" from a parameter but not a wire
                        nodep->dtypep(nodep->findBitOrLogicRangeDType(
                            VNumRange{0, 0}, nodep->valuep()->widthMin(),
                            VSigning::fromBool(issigned), is_કિંમત_Fourstate));
                    } નહીંતર {
                        nodep->dtypeSetBitOrLogicUnsized(width, nodep->valuep()->widthMin(),
                                                         VSigning::fromBool(issigned),
                                                         is_કિંમત_Fourstate);
                    }
                    didchk = સાચું;
                }
                iterateCheckAssign(nodep, "Initial value", nodep->valuep(), FINAL,
                                   nodep->dtypep());
                UINFO(9, "implicitParamFromIV " << nodep->valuep());
                // UINFO below will print variable nodep
            } નહીંતર {
                // Or, if nothing assigned, they're integral
                nodep->dtypeSetInteger();
                VL_DANGLING(bdtypep);
            }
        } નહીંતર જો (bdtypep && bdtypep->implicit()) {  // Implicits get converted to size 1
            nodep->dtypeSetLogicSized(1, bdtypep->numeric());
            VL_DANGLING(bdtypep);
        }
        જો (nodep->isNet()) {
            Ast_ગાંઠ_DType* અચલ badDtp = dtypeNot4StateIntegralRecurse(nodep->dtypep());
            જો (badDtp)
                nodep->v3error(
                    "Net " << nodep->prettyNameQ()
                           << " data type must be 4-state integral or array/union/struct of such"
                           << " (IEEE 1800-2023 6.7.1)\n"
                           << nodep->warnContextPrimary() << '\n'
                           << badDtp->warnOther() << "... Location of failing data type "
                           << badDtp->prettyDTypeNameQ() << '\n'
                           << badDtp->warnContextSecondary());
        }
        જો (nodep->valuep() && !didchk) {
            // UINFOTREE(1, nodep, "", "final");
            // AstPattern requires assignments to pass datatype on PRELIM
            userIterateAndNext(nodep->valuep(), WidthVP{nodep->dtypep(), PRELIM}.p());
            iterateCheckAssign(nodep, "Initial value", nodep->valuep(), FINAL, nodep->dtypep());
        }
        userIterateAndNext(nodep->delayp(), WidthVP{nodep->dtypep(), PRELIM}.p());
        UINFO(4, "varWidthed " << nodep);
        // UINFOTREE(1, nodep, "", "InitOut");
        nodep->didWidth(સાચું);
        nodep->doingWidth(ખોટું);
    }
    શૂન્ય visit(Ast_ગાંઠ_Var_Ref* nodep) override {
        જો (nodep->didWidth()) પાછા;
        જો (!nodep->varp()) {
            જો (m_paramsOnly && VN_IS(nodep, VarXRef)) {
                checkConstantOrReplace(
                    nodep, ખોટું,
                    "Parameter-resolved constants must not use dotted references: "
                        + nodep->prettyNameQ());
                VL_DANGLING(nodep);
                પાછા;
            } નહીંતર {
                nodep->v3fatalSrc("Unlinked varref");
            }
        }
        જો (!nodep->varp()->didWidth()) {
            // Var hasn't been widthed, so make it so.
            userIterate(nodep->varp(), nullptr);
        }
        જો (!m_paramsOnly && nodep->varp()->isIfaceArraySplit()) {
            replaceSplitIfaceArrayRef(nodep);
            પાછા;
        }
        // UINFOTREE(9, nodep, "", "VRin");
        // UINFOTREE(9, nodep->varp(), "", "forvar");
        // Note genvar's are also entered as integers
        nodep->dtypeFrom(nodep->varp());
        જો (VN_IS(nodep->backp(), ગાંઠ_Assign) && nodep->access().isWriteOrRW()) {  // On LHS
            UASSERT_OBJ(nodep->dtypep(), nodep, "LHS var should be dtype completed");
        }
        // UINFOTREE(9, nodep, "", "VRout");
        જો (nodep->access().isWriteOrRW()) nodep->varp()->icoMaybeWritten(સાચું);
        જો (nodep->access().isWriteOrRW() && nodep->varp()->direction() == VDirection::CONSTREF) {
            nodep->v3error("Assigning to const ref variable: " << nodep->prettyNameQ());
        } નહીંતર જો (nodep->access().isWriteOrRW() && nodep->varp()->is_આગત()
                   && !nodep->varp()->isFuncLocal() && nodep->varp()->isReadOnly()
                   && (!m_ftaskp || !m_ftaskp->isConstructor())
                   && !VN_IS(m_procedurep, InitialAutomatic) && !VN_IS(m_procedurep, InitialStatic)
                   && !VN_IS(nodep->abovep(), AssignForce) && !VN_IS(nodep->abovep(), Release)) {
            // Skip ASSIGNIN for continuous assignments to net-type input ports
            // via hierarchical reference. Net ports allow multiple continuous
            // drivers (IEEE 1800-2023 23.3.3.3). Input ports default to net
            // when port kind is omitted (23.2.2.3, PORT type).
            અચલ તાર્કિક hierRef = !m_curModVars.ગણતરી(nodep->varp());
            અચલ તાર્કિક netPort
                = nodep->varp()->isNet() || nodep->varp()->varType() == VVarType::PORT;
            અચલ તાર્કિક contAssign = VN_IS(nodep->abovep(), AssignW);
            અચલ તાર્કિક pinConn = VN_IS(nodep->abovep(), Pin);
            જો (!(hierRef && netPort && (contAssign || pinConn))) {
                nodep->v3warn(ASSIGNIN,
                              "Assigning to input/const variable: " << nodep->prettyNameQ());
            }
        } નહીંતર જો (nodep->access().isWriteOrRW() && nodep->varp()->isConst() && !m_paramsOnly
                   && (!m_ftaskp || !m_ftaskp->isConstructor())
                   && !VN_IS(m_procedurep, InitialAutomatic) && !VN_IS(m_procedurep, InitialStatic)
                   && !m_underMemberSel) {
            // Too loose, but need to allow our generated first assignment
            // Move this to a property of the AstInitial block
            nodep->v3warn(E_CONSTWRITTEN, "Writing to 'const' data-typed variable "
                                              << nodep->prettyNameQ()
                                              << " (IEEE 1800-2023 6.20.6)");
        }
        જો (nodep->varp()->isClassMember() && !nodep->varp()->isFuncLocal()
            && !nodep->varp()->lifetime().isStatic() && m_ftaskp && m_ftaskp->isStatic()) {
            nodep->v3error("Cannot access non-static member variable "
                           << nodep->prettyNameQ() << " from a static method "
                           << m_ftaskp->prettyNameQ() << " without object (IEEE 1800-2023 8.10)");
        }
        nodep->didWidth(સાચું);
    }

    શૂન્ય visit(AstEnumDType* nodep) override {
        જો (nodep->didWidthAndSet()) પાછા;  // This node is a dtype & not both PRELIMed+FINALed
        UINFO(5, "  ENUMDTYPE " << nodep);
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        nodep->dtypep(nodep);
        Ast_ગાંઠ_DType* basicp = nodep->dtypep()->skipRefp()->basicp();
        Ast_ગાંઠ_DType* અચલ badDtp = dtypeNotIntAtomOrVecRecurse(nodep->subDTypep());
        જો (badDtp) {
            nodep->v3error(
                "Enum data type must be an integer atom or vector type (IEEE 1800-2023 6.19)\n"
                << nodep->warnContextPrimary() << '\n'
                << badDtp->warnOther() << "... Location of failing data type "
                << badDtp->prettyDTypeNameQ() << '\n'
                << badDtp->warnContextSecondary());
            basicp = nodep->findIntDType()->basicp();
            nodep->refDTypep(basicp);
        }
        nodep->widthFromSub(nodep->subDTypep());
        // Assign widths
        userIterateAndNext(nodep->itemsp(), WidthVP{nodep->dtypep(), BOTH}.p());
        // Assign missing values
        V3_સંખ્યા અંક(nodep, nodep->width(), 0);
        અચલ V3_સંખ્યા one{nodep, nodep->width(), 1};
        તાર્કિક wrapAround = ખોટું;
        std::map<અચલ V3_સંખ્યા, AstEnumItem*> inits;
        માટે (AstEnumItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), EnumItem)) {
            જો (itemp->valuep()) {
                જો (debug() >= 9) {
                    UINFO(0, "EnumInit " << itemp);
                    itemp->valuep()->dump_વૃક્ષ("-  EnumInit: ");
                }
                V3Const::constifyParamsEdit(itemp->valuep());  // itemp may change
                જો (!VN_IS(itemp->valuep(), Const)) {
                    itemp->valuep()->v3error("Enum value isn't a constant");
                    itemp->valuep()->unlinkFrBack()->delete_વૃક્ષ();
                    ચાલુ;
                }
                // TODO IEEE says assigning sized number that is not same size as enum is illegal
                wrapAround = ખોટું;  // Explicit value resets wrap-around tracking
            }
            જો (!itemp->valuep()) {
                જો (wrapAround) {
                    itemp->v3error("Enum value illegally wrapped around (IEEE 1800-2023 6.19)");
                }
                જો (અંક.isFourState()) {
                    itemp->v3error("Enum value that is unassigned cannot follow value with X/Zs "
                                   "(IEEE 1800-2023 6.19)");
                }
                itemp->valuep(નવું AstConst{itemp->fileline(), અંક});
            }

            અચલ AstConst* અચલ constp = VN_AS(itemp->valuep(), Const);
            જો (constp->અંક().isFourState() && basicp->basicp() && !basicp->isFourstate()) {
                itemp->v3error("Enum value with X/Zs cannot be assigned to non-fourstate type "
                               "(IEEE 1800-2023 6.19)");
            }
            અંક.opAssign(constp->અંક());
            // Look for duplicates
            અચલ auto pair = inits.emplace(અંક, itemp);
            જો (!pair.second) {  // IEEE says illegal
                અચલ Ast_ગાંઠ* અચલ otherp = pair.first->second;
                itemp->v3error("Overlapping enumeration value: "
                               << itemp->prettyNameQ() << '\n'
                               << itemp->warnContextPrimary() << '\n'
                               << otherp->warnOther() << "... Location of original declaration\n"
                               << otherp->warnContextSecondary());
            }
            // Detect wrap-around after increment for the next auto-valued item.
            // For signed types, overflow is positive-to-negative (e.g., INT_MAX+1).
            // For unsigned types, overflow wraps to zero (e.g., UINT_MAX+1).
            અચલ તાર્કિક prevNeg = constp->અંક().isNegative();
            અંક.opAdd(one, constp->અંક());
            જો (basicp->isSigned()) {
                wrapAround = !prevNeg && અંક.isNegative();
            } નહીંતર {
                wrapAround = અંક.isEqZero();
            }
        }
    }
    શૂન્ય visit(AstEnumItem* nodep) override {
        UINFO(5, "   ENUMITEM " << nodep);
        VL_RESTORER(m_enumItemp);
        m_enumItemp = nodep;
        Ast_ગાંઠ_DType* અચલ vdtypep = m_vup->dtypep();
        UASSERT_OBJ(vdtypep, nodep, "ENUMITEM not under ENUM");
        nodep->dtypep(vdtypep);
        જો (nodep->valuep()) {  // else the value will be assigned sequentially
            // Default type is int, but common to assign narrower values, so minwidth from value
            userIterateAndNext(nodep->valuep(), WidthVP{CONTEXT_DET, PRELIM}.p());
            તાર્કિક warnOn = સાચું;
            Ast_ગાંઠ_પદાવલિ* valuep = nodep->valuep();
            જો (અચલ AstAdd* અચલ anodep = VN_CAST(valuep, Add)) {
                // If constructed by V3LinkParse due to "enumitem[N_REPEATS] value"
                જો (anodep->fileline()->equal_First_લીટી_Col(*(nodep->fileline())))
                    valuep = anodep->lhsp();
            }
            જો (અચલ AstConst* અચલ constp = VN_CAST(valuep, Const)) {
                જો (static_cast<પૂર્ણાંક>(constp->અંક().mostSetBitP1()) > nodep->width()) {
                    constp->v3warn(ENUMITEMWIDTH,
                                   "Enum value exceeds width of enum type (IEEE 1800-2023 6.19)");
                    warnOn = ખોટું;  // Prevent normal WIDTHTRUNC
                }
            }
            // Minwidth does not come from value, as spec says set based on parent
            // and if we keep minwidth we'll consider it unsized which is incorrect
            iterateCheck(nodep, "Enum value", nodep->valuep(), CONTEXT_DET, FINAL, nodep->dtypep(),
                         EXTEND_EXP, warnOn);
            // Always create a cast, to avoid later ENUMVALUE warnings
            nodep->valuep(નવું AstCast{nodep->valuep()->fileline(), nodep->valuep()->unlinkFrBack(),
                                      nodep->dtypep()});
        }
    }
    શૂન્ય visit(AstEnumItemRef* nodep) override {
        UASSERT_OBJ(nodep->itemp(), nodep, "Unlinked");
        જો (!nodep->itemp()->didWidth()) {
            // We need to do the whole enum en masse
            Ast_ગાંઠ* enump = nodep->itemp();
            UASSERT_OBJ(enump, nodep, "EnumItemRef not linked");
            માટે (; enump; enump = enump->backp()) {
                જો (VN_IS(enump, EnumDType)) તોડો;
            }
            UASSERT_OBJ(enump, nodep, "EnumItemRef can't deref back to an Enum");
            VL_DO_DANGLING(userIterate(enump, m_vup), enump);  // Parent enump maybe relinked
        }
        nodep->dtypeFrom(nodep->itemp());
    }
    શૂન્ય visit(Ast_ગાંઠ_Cons* nodep) override {}  // Widthed when created
    શૂન્ય visit(Ast_ગાંઠ_Cons_Item* nodep) override {}  // Widthed when created
    શૂન્ય visit(AstDist* nodep) override {
        //  x dist {a :/ p, b :/ q} --> (p > 0 && x == a) || (q > 0 && x == b)
        // (only outside constraints; inside constraints V3Randomize handles weighted selection)
        userIterateAndNext(nodep->exprp(), WidthVP{CONTEXT_DET, PRELIM}.p());
        માટે (Ast_ગાંઠ *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            nextip = itemp->nextp();  // iterate may cause the node to get replaced
            VL_DO_DANGLING(userIterate(itemp, WidthVP{CONTEXT_DET, PRELIM}.p()), itemp);
        }

        AstBasicDType* dtype = VN_CAST(nodep->exprp()->dtypep(), BasicDType);
        Ast_ગાંઠ_DType* subDTypep = nullptr;
        nodep->dtypeSetBit();

        જો (dtype && dtype->isString()) {
            nodep->dtypeSetString();
            subDTypep = nodep->findStringDType();
        } નહીંતર જો (dtype && dtype->isDouble()) {
            nodep->dtypeSetDouble();
            subDTypep = nodep->findDoubleDType();
        } નહીંતર {
            // Take width as maximum across all items
            પૂર્ણાંક width = nodep->exprp()->width();
            પૂર્ણાંક mwidth = nodep->exprp()->widthMin();
            તાર્કિક isFourstate = nodep->exprp()->dtypep()->isFourstate();
            માટે (અચલ Ast_ગાંઠ* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
                width = std::max(width, itemp->width());
                mwidth = std::max(mwidth, itemp->widthMin());
                isFourstate |= itemp->dtypep()->isFourstate();
            }
            subDTypep = nodep->findBitOrLogicDType(
                width, mwidth, nodep->exprp()->dtypep()->numeric(), isFourstate);
        }

        iterateCheck(nodep, "Dist expression", nodep->exprp(), CONTEXT_DET, FINAL, subDTypep,
                     EXTEND_EXP);
        માટે (Ast_ગાંઠ *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            nextip = itemp->nextp();
            itemp = VN_AS(itemp, DistItem)->rangep();
            જો (VN_IS(itemp, InsideRange)) {
                userIterate(itemp, WidthVP{subDTypep, FINAL}.p());
            } નહીંતર {
                iterateCheck(nodep, "Dist Item", itemp, CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP);
            }
        }

        // Inside a constraint, V3Randomize handles dist lowering with proper weights,
        // but only for simple scalar/range items. Container-type items (queues, arrays)
        // must be lowered here via insideItem() which knows how to expand them.
        જો (m_constraintp) {
            તાર્કિક canLower = સાચું;
            માટે (AstDistItem* ditemp = nodep->itemsp(); ditemp;
                 ditemp = VN_AS(ditemp->nextp(), DistItem)) {
                જો (VN_IS(ditemp->rangep(), InsideRange)) ચાલુ;
                // An integral item is a plain equality on either path, so it may be
                // non-constant.  Testing for AstConst here instead would silently drop
                // the weights of a dist whose items are state variables.  Compound
                // items (containers, strings) still need insideItem(), and an impure
                // item must not be cloned into the bucket chain.
                અચલ Ast_ગાંઠ_DType* અચલ itemDtp = ditemp->rangep()->dtypep()->skipRefp();
                જો (!itemDtp->isIntegralOrPacked() || !ditemp->rangep()->isPure()) {
                    canLower = ખોટું;
                    તોડો;
                }
            }
            જો (canLower) પાછા;
        }

        // Outside constraint: lower to inside (ignores weights)
        Ast_ગાંઠ_પદાવલિ* newp = nullptr;
        માટે (AstDistItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), DistItem)) {
            Ast_ગાંઠ_પદાવલિ* inewp
                = insideItem(nodep, nodep->exprp()->clone_વૃક્ષ_Pure(ખોટું), itemp->rangep());
            જો (!inewp) ચાલુ;
            Ast_ગાંઠ_પદાવલિ* અચલ cmpp
                = નવું AstGt{itemp->fileline(), itemp->weightp()->unlinkFrBack(),
                            નવું AstConst{itemp->fileline(), 0}};
            cmpp->fileline()->modifyWarnOff(V3ErrorCode::UNSIGNED, સાચું);
            cmpp->fileline()->modifyWarnOff(V3ErrorCode::CMPCONST, સાચું);
            inewp = નવું AstLogAnd{itemp->fileline(), cmpp, inewp};
            newp = newp ? નવું AstLogOr{nodep->fileline(), newp, inewp} : inewp;
        }
        જો (!newp) newp = નવું AstConst{nodep->fileline(), AstConst::BitFalse{}};

        UINFOTREE(9, nodep, "", "dist-out");
        nodep->replaceWith(newp);
        VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
    }

    શૂન્ય visit(AstInside* nodep) override {
        userIterateAndNext(nodep->exprp(), WidthVP{CONTEXT_DET, PRELIM}.p());
        માટે (Ast_ગાંઠ *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            nextip = itemp->nextp();  // iterate may cause the node to get replaced
            VL_DO_DANGLING(userIterate(itemp, WidthVP{CONTEXT_DET, PRELIM}.p()), itemp);
        }

        AstBasicDType* dtype = VN_CAST(nodep->exprp()->dtypep(), BasicDType);
        Ast_ગાંઠ_DType* expDTypep = nullptr;

        જો (dtype && dtype->isString()) {
            nodep->dtypeSetString();
            expDTypep = nodep->findStringDType();
        } નહીંતર જો (dtype && dtype->isDouble()) {
            nodep->dtypeSetDouble();
            expDTypep = nodep->findDoubleDType();
        } નહીંતર {
            // Take width as maximum across all items
            પૂર્ણાંક width = nodep->exprp()->width();
            પૂર્ણાંક mwidth = nodep->exprp()->widthMin();
            તાર્કિક isFourstate = nodep->exprp()->dtypep()->isFourstate();
            માટે (અચલ Ast_ગાંઠ* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
                width = std::max(width, itemp->width());
                mwidth = std::max(mwidth, itemp->widthMin());
                isFourstate |= itemp->dtypep()->isFourstate();
            }
            nodep->dtypeSetBit();
            અચલ VSigning numeric = nodep->exprp()->dtypep()->numeric();
            expDTypep = nodep->findBitOrLogicDType(width, mwidth, numeric, isFourstate);
        }

        iterateCheck(nodep, "Inside expression", nodep->exprp(), CONTEXT_DET, FINAL, expDTypep,
                     EXTEND_EXP);
        માટે (Ast_ગાંઠ *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            nextip = itemp->nextp();  // iterate may cause the node to get replaced
            જો (VN_IS(itemp, InsideRange)) {
                userIterate(itemp, WidthVP{expDTypep, FINAL}.p());
            } નહીંતર જો (!itemp->dtypep()->skipRefp()->isNonPackedArray()) {
                iterateCheck(nodep, "Inside Item", itemp, CONTEXT_DET, FINAL, expDTypep,
                             EXTEND_EXP);
            }
        }

        Ast_ગાંઠ_પદાવલિ* exprp;
        Ast_પદાવલિ_Stmt* પદાવલિ_Stmtp = nullptr;
        // Skip constraints since those are declarative expressions and they are never actually
        // executed so, there is no need for purification since they cannot generate sideeffects.
        જો (!m_constraintp && !nodep->exprp()->isPure()) {
            File_લીટી* અચલ fl = nodep->exprp()->fileline();
            // Ensure sized dtype for temp variable
            Ast_ગાંઠ_DType* અચલ પદાવલિ_Dtp = nodep->exprp()->dtypep();
            અચલ પૂર્ણાંક w = પદાવલિ_Dtp->width();
            Ast_ગાંઠ_DType* અચલ tempDTypep
                = પદાવલિ_Dtp->widthSized() ? પદાવલિ_Dtp
                                        : nodep->findBitOrLogicDType(w, w, પદાવલિ_Dtp->numeric(),
                                                                     પદાવલિ_Dtp->isFourstate());
            AstVar* અચલ varp
                = નવું AstVar{fl, VVarType::XTEMP, m_insideTempNames.get(nodep), tempDTypep};
            exprp = નવું AstVarRef{fl, varp, VAccess::READ};
            પદાવલિ_Stmtp = નવું Ast_પદાવલિ_Stmt{fl,
                                        નવું AstAssign{fl, નવું AstVarRef{fl, varp, VAccess::WRITE},
                                                      nodep->exprp()->unlinkFrBack()},
                                        exprp};
            જો (m_ftaskp) {
                varp->funcLocal(સાચું);
                varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                m_ftaskp->prependStmtsp(varp);
            } નહીંતર {
                m_modep->prependStmtsp(varp);
            }
            iterate(varp);
            iterate(પદાવલિ_Stmtp);
        } નહીંતર {
            exprp = nodep->exprp();
        }
        UINFOTREE(9, nodep, "", "inside-in");
        // Now rip out the inside and replace with simple math
        Ast_ગાંઠ_પદાવલિ* newp = nullptr;
        માટે (Ast_ગાંઠ_પદાવલિ *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            nextip = VN_AS(itemp->nextp(), ગાંઠ_પદાવલિ);  // Will be unlinking
            Ast_ગાંઠ_પદાવલિ* inewp;
            જો (પદાવલિ_Stmtp) {
                inewp = insideItem(nodep, પદાવલિ_Stmtp, itemp);
                પદાવલિ_Stmtp = nullptr;
            } નહીંતર {
                inewp = insideItem(nodep, exprp->clone_વૃક્ષ_Pure(ખોટું), itemp);
            }
            જો (!inewp) ચાલુ;
            newp = newp ? નવું AstLogOr{nodep->fileline(), newp, inewp} : inewp;
        }
        જો (પદાવલિ_Stmtp) VL_DO_DANGLING(પદાવલિ_Stmtp->delete_વૃક્ષ(), પદાવલિ_Stmtp);
        જો (!newp) newp = નવું AstConst{nodep->fileline(), AstConst::BitFalse{}};
        UINFOTREE(9, newp, "", "inside-out");
        nodep->replaceWith(newp);
        VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
    }
    Ast_ગાંઠ_પદાવલિ* insideItem(Ast_ગાંઠ* nodep, Ast_ગાંઠ_પદાવલિ* exprp, Ast_ગાંઠ_પદાવલિ* itemp) {
        અચલ Ast_ગાંઠ_DType* અચલ itemDtp = itemp->dtypep()->skipRefp();
        જો (AstInsideRange* અચલ irangep = VN_CAST(itemp, InsideRange)) {
            // Similar logic in V3Case
            પાછા irangep->newAndFromInside(exprp, irangep->lhsp()->unlinkFrBack(),
                                             irangep->rhsp()->unlinkFrBack());
        } નહીંતર જો (VN_IS(itemDtp, UnpackArrayDType) || VN_IS(itemDtp, DynArrayDType)
                   || VN_IS(itemDtp, QueueDType)) {
            // Unsupported in parameters
            Ast_ગાંઠ_પદાવલિ* અચલ inewp = નવું AstCMethodHard{nodep->fileline(), itemp->unlinkFrBack(),
                                                          VCMethod::ARRAY_INSIDE, exprp};
            iterateCheckTyped(nodep, "inside value", exprp, itemDtp->subDTypep(), BOTH);
            inewp->dtypeSetBit();
            inewp->didWidth(સાચું);
            પાછા inewp;
        } નહીંતર જો (VN_IS(itemDtp, AssocArrayDType)) {
            nodep->v3error("Inside operator not specified on associative arrays "
                           "(IEEE 1800-2023 11.4.13)");
            VL_DO_DANGLING(exprp->delete_વૃક્ષ(), exprp);
            પાછા nullptr;
        }
        પાછા AstEqWild::newTyped(itemp->fileline(), exprp, itemp->unlinkFrBack());
    }
    શૂન્ય visit(AstInsideRange* nodep) override {
        // Just do each side; AstInside will rip these nodes out later.
        જો (!m_vup) {
            // This range appears outside a normal expression context (e.g. in a covergroup
            // bin declaration).  Width the bounds self-determined first so each bound (and any
            // referenced parameter) has a dtype, then constify so constant arithmetic is folded
            // (e.g. AstNegate(Const) -> Const, or a parameter reference -> Const).  Folding
            // preserves the now-present dtype, so no bound reaches V3WidthCommit without one.
            userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
            userIterateAndNext(nodep->rhsp(), WidthVP{SELF, BOTH}.p());
            V3Const::constifyEdit(nodep->lhsp());  // lhsp may change
            V3Const::constifyEdit(nodep->rhsp());  // rhsp may change
        } નહીંતર {
            જો (m_vup->prelim()) {
                userIterateAndNext(nodep->lhsp(), m_vup);
                userIterateAndNext(nodep->rhsp(), m_vup);
            }
            જો (m_vup->final()) {
                Ast_ગાંઠ_DType* અચલ expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
                // Warning waivers match visit_cmp_eq_gt on the lowered Gte/Lte
                અચલ પૂર્ણાંક expWidth = expDTypep->width();
                અચલ તાર્કિક waiveLhs = expWidth == 32
                                      && !(expDTypep->isSigned() && nodep->lhsp()->isSigned())
                                      && expDTypep->widthMin() >= nodep->lhsp()->width();
                અચલ તાર્કિક waiveRhs = expWidth == 32
                                      && !(expDTypep->isSigned() && nodep->rhsp()->isSigned())
                                      && expWidth >= nodep->rhsp()->widthMin();
                iterateCheck(nodep, "Range LHS", nodep->lhsp(), CONTEXT_DET, FINAL, expDTypep,
                             EXTEND_EXP, !waiveLhs);
                iterateCheck(nodep, "Range RHS", nodep->rhsp(), CONTEXT_DET, FINAL, expDTypep,
                             EXTEND_EXP, !waiveRhs);
            }
        }
        nodep->dtypeFrom(nodep->lhsp());
    }

    શૂન્ય visit(AstIfaceRefDType* nodep) override {
        જો (nodep->didWidthAndSet()) પાછા;  // This node is a dtype & not both PRELIMed+FINALed
        UINFO(5, "   IFACEREF " << nodep);
        userIterateChildren(nodep, m_vup);
        nodep->dtypep(nodep);
        જો (nodep->isVirtual()) nodep->ifaceViaCellp()->setHasVirtualRef();
        UINFO(4, "dtWidthed " << nodep);
    }
    શૂન્ય visit(Ast_ગાંઠ_UOr_Struct_DType* nodep) override {
        જો (nodep->doingWidth()) {  // Early exit if have circular parameter definition
            nodep->v3error("Struct's type is circular: " << nodep->prettyName());
            nodep->dtypeSetBit();
            nodep->doingWidth(ખોટું);
            પાછા;
        }
        જો (nodep->didWidthAndSet()) પાછા;  // This node is a dtype & not both PRELIMed+FINALed
        nodep->doingWidth(સાચું);
        UINFO(5, "   NODEUORS " << nodep);
        // Check for tagged unions
        જો (અચલ AstUnionDType* અચલ unionp = VN_CAST(nodep, UnionDType)) {
            જો (unionp->isTagged()) { nodep->v3warn(E_UNSUPPORTED, "Unsupported: tagged union"); }
        }
        // UINFOTREE(9, nodep, "", "class-in");
        userIterateChildren(nodep, nullptr);  // First size all members
        nodep->dtypep(nodep);
        nodep->isFourstate(ખોટું);
        // Error checks
        માટે (AstMemberDType* itemp = nodep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            Ast_ગાંઠ_DType* અચલ dtp = itemp->subDTypep()->skipRefp();
            જો (nodep->packed() && !dtp->isIntegralOrPacked())
                itemp->v3error("Unpacked data type "
                               << dtp->prettyDTypeNameQ()
                               << " in packed struct/union (IEEE 1800-2023 7.2.1)");
            જો ((VN_IS(nodep, UnionDType) || nodep->packed()) && itemp->valuep()) {
                itemp->v3error("Initial values not allowed in packed struct/union"
                               " (IEEE 1800-2023 7.2.2)");
                ધકેલો_Deletep(itemp->valuep()->unlinkFrBack());
            }
        }
        અચલ તાર્કિક isHardPackedUnion
            = nodep->packed() && VN_IS(nodep, UnionDType) && !VN_CAST(nodep, UnionDType)->isSoft();

        // Suppress union size errors in parameterized template modules where member
        // widths depend on unresolved parameters.  Also suppress when the type has no
        // owning module (e.g. moved to TypeTable during DepGraph resolution).
        // TODO: Revisit this gate if DepGraph becomes the sole flow and widthing
        // can assume all types are already specialized.
        અચલ તાર્કિક inTemplateModule = (m_modep && m_modep->parameterizedTemplate())
                                      || (VN_IS(nodep, UnionDType) && !m_modep);

        // Determine bit assignments and width
        જો (VN_IS(nodep, UnionDType) || nodep->packed()) {
            પૂર્ણાંક lsb = 0;
            પૂર્ણાંક width = 0;
            તાર્કિક first = સાચું;
            // Report errors on first member first
            AstMemberDType* itemp;
            // MSB is first, so loop backwards
            માટે (itemp = nodep->membersp(); itemp && itemp->nextp();
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {}
            માટે (; itemp; itemp = VN_CAST(itemp->backp(), MemberDType)) {
                જો (itemp->isFourstate()) nodep->isFourstate(સાચું);
                itemp->lsb(lsb);
                જો (VN_IS(nodep, UnionDType)) {
                    અચલ પૂર્ણાંક itemWidth = itemp->width();
                    // Skip union size check for template modules with unresolved parameters
                    જો (!first && isHardPackedUnion && itemWidth != width && !inTemplateModule) {
                        itemp->v3error("Hard packed union members must have equal size "
                                       "(IEEE 1800-2023 7.3.1)");
                    }
                    width = std::max(width, itemWidth);
                } નહીંતર {
                    lsb += itemp->width();
                    width += itemp->width();
                }
                first = ખોટું;
            }
            nodep->widthForce(width, width);  // Signing stays as-is, as parsed from declaration
        } નહીંતર {
            nodep->widthForce(1, 1);
        }
        nodep->doingWidth(ખોટું);
        // UINFOTREE(9, nodep, "", "class-out");
    }
    શૂન્ય visit(AstThisRef* nodep) override {
        જો (nodep->didWidthAndSet()) પાછા;
        nodep->dtypep(iterateEditMoveDTypep(nodep, nodep->childDTypep()));
        જો (m_ftaskp && m_ftaskp->isStatic()) {
            nodep->v3error("Cannot use 'this' in a static method "
                           << m_ftaskp->prettyNameQ() << " (IEEE 1800-2023 8.10-8.11)");
        }
    }
    શૂન્ય visit(AstClassRefDType* nodep) override {
        જો (nodep->didWidthAndSet()) પાછા;
        // TODO this maybe eventually required to properly resolve members,
        // though causes problems with t_class_forward.v, so for now avoided
        // userIterateChildren(nodep->classp(), nullptr);
    }
    શૂન્ય visit(AstClassOrPackageRef* nodep) override {
        જો (nodep->didWidthAndSet()) પાછા;
        userIterateChildren(nodep, nullptr);
    }
    શૂન્ય visit(AstDot* nodep) override {
        // We can only reach this from constify called during V3Param (so before linkDotParam)
        // ... #(Cls#(...)::...) ...
        //                ^^~~~ this is our DOT
        nodep->v3warn(E_UNSUPPORTED, "dotted expressions in parameters\n"
                                         << nodep->warnMore() << "... Suggest use a typedef");
        nodep->replaceWith(નવું AstConst{nodep->fileline(), AstConst::BitFalseErroring{}});
        VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
    }
    શૂન્ય visit(AstClassExtends* nodep) override {
        જો (nodep->didWidthAndSet()) પાછા;
        જો (VN_IS(nodep->childDTypep(), ClassRefDType)) {
            nodep->dtypep(iterateEditMoveDTypep(nodep, nodep->childDTypep()));
        }
    }
    શૂન્ય visit(AstMemberDType* nodep) override {
        જો (nodep->didWidthAndSet()) પાછા;  // This node is a dtype & not both PRELIMed+FINALed
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        nodep->widthFromSub(nodep->subDTypep());
        nodep->dtypeFrom(nodep->subDTypep());
        જો (nodep->valuep()) {
            userIterateAndNext(nodep->valuep(), WidthVP{nodep->dtypep(), PRELIM}.p());
            iterateCheckAssign(nodep, "Initial value", nodep->valuep(), FINAL, nodep->dtypep());
        }
    }
    શૂન્ય visit(AstStructSel* nodep) override {
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
    }
    શૂન્ય visit(AstMemberSel* nodep) override {
        UINFO(5, "   MEMBERSEL " << nodep);
        જો (nodep->didWidth()) પાછા;
        VL_RESTORER(m_underMemberSel);
        m_underMemberSel = સાચું;
        UINFOTREE(9, nodep, "", "mbs-in");
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
        UINFOTREE(9, nodep, "", "mbs-ic");
        // Find the fromp dtype - should be a class
        UASSERT_OBJ(nodep->fromp()->dtypep(), nodep->fromp(), "Unlinked data type");
        Ast_ગાંઠ_DType* અચલ fromDtp = nodep->fromp()->dtypep()->skipRefToEnump();
        UINFO(9, "     from dt " << fromDtp);
        અચલ AstMemberSel* અચલ fromSel = VN_CAST(nodep->fromp(), MemberSel);
        જો (AstClocking* અચલ clockingp = fromSel && fromSel->varp()
                                               ? VN_CAST(fromSel->varp()->firstAbovep(), Clocking)
                                               : nullptr) {
            // In:  MEMBERSEL{MEMBERSEL{vifaceref, "cb", cb_event}, "clockvar", null}
            // Out: MEMBERSEL{vifaceref, "clockvar", clockvar}
            UINFO(9, "     from clocking " << clockingp);
            જો (AstVar* અચલ varp = memberSelClocking(nodep, clockingp)) {
                જો (!varp->didWidth()) userIterate(varp, nullptr);
                AstMemberSel* fromp = VN_AS(nodep->fromp(), MemberSel);
                fromp->replaceWith(fromp->fromp()->unlinkFrBack());
                VL_DO_DANGLING(fromp->delete_વૃક્ષ(), fromp);
                nodep->dtypep(varp->dtypep());
                nodep->varp(varp);
                જો (nodep->access().isWriteOrRW()) {
                    V3LinkLValue::linkLValueSet(nodep, nodep->access());
                }
                જો (AstIfaceRefDType* અચલ adtypep
                    = VN_CAST(nodep->fromp()->dtypep()->skipRefp(), IfaceRefDType)) {
                    nodep->varp()->sensIfacep(adtypep->ifacep());
                }
                UINFO(9, "     done clocking msel " << nodep);
                nodep->didWidth(સાચું);  // Must not visit again: will confuse scopes
                પાછા;
            }
        } નહીંતર જો (Ast_ગાંઠ_UOr_Struct_DType* અચલ adtypep = VN_CAST(fromDtp, ગાંઠ_UOr_Struct_DType)) {
            જો (memberSelStruct(nodep, adtypep)) પાછા;
        } નહીંતર જો (AstClassRefDType* અચલ adtypep = VN_CAST(fromDtp, ClassRefDType)) {
            જો (memberSelClass(nodep, adtypep)) પાછા;
        } નહીંતર જો (AstIfaceRefDType* અચલ adtypep = VN_CAST(fromDtp, IfaceRefDType)) {
            જો (Ast_ગાંઠ* foundp = memberSelIface(nodep, adtypep)) {
                જો (AstClocking* અચલ clockingp = VN_CAST(foundp, Clocking))
                    foundp = clockingp->ensureEventp();
                જો (AstVar* અચલ varp = VN_CAST(foundp, Var)) {
                    જો (!varp->didWidth()) userIterate(varp, nullptr);
                    AstIface* અચલ ifacep = adtypep->ifaceViaCellp();
                    varp->sensIfacep(ifacep);
                    // Member of a non-virtual interface reference, e.g. an element selected
                    // from a whole array of interfaces: reference it through the interface,
                    // as V3LinkDot does for 'iface.member'
                    અચલ AstVarRef* અચલ refp = VN_CAST(nodep->fromp(), VarRef);
                    જો (refp && !adtypep->isVirtual() && refp->varp()->isIfaceRef()) {
                        AstVarXRef* અચલ newp = નવું AstVarXRef{nodep->fileline(), varp,
                                                                refp->name(), nodep->access()};
                        newp->didWidth(સાચું);
                        nodep->replaceWith(newp);
                        VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
                        પાછા;
                    }
                    nodep->dtypep(foundp->dtypep());
                    nodep->varp(varp);
                    nodep->didWidth(સાચું);
                    પાછા;
                }
                જો (AstModport* અચલ modportp = VN_CAST(foundp, Modport)) {
                    // Modport selection (e.g. vif.passive_mp) is compile-time
                    // type narrowing: replace MemberSel with fromp re-typed.
                    AstIfaceRefDType* અચલ newDtypep = નવું AstIfaceRefDType{
                        nodep->fileline(), nodep->fileline(), adtypep->cellName(),
                        adtypep->ifaceName(), modportp->name()};
                    newDtypep->ifacep(adtypep->ifacep());
                    newDtypep->cellp(adtypep->cellp());
                    newDtypep->modportp(modportp);
                    newDtypep->isVirtual(adtypep->isVirtual());
                    v3Global.rootp()->typeTablep()->addTypesp(newDtypep);
                    Ast_ગાંઠ_પદાવલિ* અચલ fromp = nodep->fromp()->unlinkFrBack();
                    fromp->dtypep(newDtypep);
                    nodep->replaceWith(fromp);
                    VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
                    પાછા;
                }
                જો (AstCell* અચલ cellp = VN_CAST(foundp, Cell)) {
                    // Sub-interface cell selection (e.g. vif.tx): resolve to the
                    // companion __Viftop var created by V3LinkCells for its dtype.
                    જો (VN_IS(cellp->modp(), Iface)) {
                        અચલ string viftopName = cellp->name() + "__Viftop";
                        Ast_ગાંઠ_Module* અચલ parentIfacep = adtypep->ifaceViaCellp();
                        AstVar* viftopVarp = nullptr;
                        માટે (Ast_ગાંઠ* itemp = parentIfacep->stmtsp(); itemp;
                             itemp = itemp->nextp()) {
                            જો (AstVar* અચલ vp = VN_CAST(itemp, Var)) {
                                જો (vp->name() == viftopName) {
                                    viftopVarp = vp;
                                    તોડો;
                                }
                            }
                        }
                        UASSERT_OBJ(viftopVarp, nodep,
                                    "No __Viftop variable for sub-interface cell");
                        જો (!viftopVarp->didWidth()) userIterate(viftopVarp, nullptr);
                        Ast_ગાંઠ_DType* subDtypep = viftopVarp->dtypep();
                        જો (adtypep->isVirtual()) {
                            // A sub-interface selected through a virtual interface
                            // handle is itself a virtual reference (a runtime
                            // instance pointer), not a static instance. Mark the
                            // dtype virtual so a method call on it dispatches per
                            // instance instead of inlining to one fixed scope.
                            જો (AstIfaceRefDType* અચલ subRefp
                                = VN_CAST(subDtypep->skipRefp(), IfaceRefDType)) {
                                AstIfaceRefDType* અચલ newDtypep = નવું AstIfaceRefDType{
                                    nodep->fileline(), subRefp->cellName(), subRefp->ifaceName()};
                                newDtypep->ifacep(subRefp->ifacep());
                                newDtypep->cellp(subRefp->cellp());
                                newDtypep->isVirtual(સાચું);
                                v3Global.rootp()->typeTablep()->addTypesp(newDtypep);
                                subDtypep = newDtypep;
                            }
                        }
                        nodep->dtypep(subDtypep);
                        nodep->varp(viftopVarp);
                        viftopVarp->sensIfacep(VN_AS(cellp->modp(), Iface));
                        nodep->didWidth(સાચું);
                        પાછા;
                    }
                }
                જો (Ast_ગાંઠ_FTask* અચલ ftaskp = VN_CAST(foundp, ગાંઠ_FTask)) {
                    AstMethodCall* અચલ newp = નવું AstMethodCall{
                        nodep->fileline(), nodep->fromp()->unlinkFrBack(), nodep->name()};
                    newp->taskp(ftaskp);
                    newp->dtypep(ftaskp->dtypep());
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
                    userIterate(newp, m_vup);
                    પાછા;
                }
                UINFO(1, "found object " << foundp);
                nodep->v3fatalSrc("MemberSel of non-variable\n"
                                  << nodep->warnContextPrimary() << '\n'
                                  << foundp->warnOther() << "... Location of found object\n"
                                  << foundp->warnContextSecondary());
            }
        } નહીંતર જો (VN_IS(fromDtp, EnumDType)  //
                   || VN_IS(fromDtp, AssocArrayDType)  //
                   || VN_IS(fromDtp, WildcardArrayDType)  //
                   || VN_IS(fromDtp, UnpackArrayDType)  //
                   || VN_IS(fromDtp, DynArrayDType)  //
                   || VN_IS(fromDtp, QueueDType)  //
                   || VN_IS(fromDtp, ConstraintRefDType)  //
                   || VN_IS(fromDtp, BasicDType)) {
            // Method call on enum without following parenthesis, e.g. "ENUM.next"
            // Convert this into a method call, and let that visitor figure out what to do next
            Ast_ગાંઠ* અચલ newp = નવું AstMethodCall{nodep->fileline(),
                                                    nodep->fromp()->unlinkFrBack(), nodep->name()};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
            userIterate(newp, m_vup);
            પાછા;
        } નહીંતર {
            nodep->v3error("Member selection of non-struct/union object '"
                           << nodep->fromp()->prettyTypeName() << "' which is a '"
                           << nodep->fromp()->dtypep()->prettyTypeName() << "'");
        }
        // Error handling
        nodep->replaceWith(નવું AstConst{nodep->fileline(), AstConst::BitFalse{}});
        VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
    }
    તાર્કિક memberSelClass(AstMemberSel* nodep, AstClassRefDType* adtypep) {
        જો (nodep->name() == "rand_mode" || nodep->name() == "randomize") {
            AstMethodCall* અચલ newp = નવું AstMethodCall{
                nodep->fileline(), nodep->fromp()->unlinkFrBack(), nodep->name()};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
            visit(newp);
            પાછા સાચું;
        }
        // Returns true if ok
        // No need to width-resolve the class, as it was done when we did the child
        AstClass* અચલ first_classp = adtypep->classp();
        UASSERT_OBJ(first_classp, nodep, "Unlinked");
        માટે (AstClass* classp = first_classp; classp;) {
            જો (Ast_ગાંઠ* અચલ foundp = m_memberMap.findMember(classp, nodep->name())) {
                જો (AstVar* અચલ varp = VN_CAST(foundp, Var)) {
                    જો (!varp->didWidth()) userIterate(varp, nullptr);
                    જો (varp->lifetime().isStatic() || varp->isParam()) {
                        // Static members are moved outside the class, so they shouldn't be
                        // accessed by member select on a class object
                        AstVarRef* અચલ varRefp
                            = નવું AstVarRef{nodep->fileline(), varp, nodep->access()};
                        varRefp->classOrPackagep(classp);
                        nodep->replaceWith(varRefp);
                        VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
                        પાછા સાચું;
                    }
                    જો (nodep->access().isWriteOrRW() && varp->isConst()) {
                        nodep->v3warn(E_CONSTWRITTEN, "Writing to 'const' data-typed variable "
                                                          << nodep->prettyNameQ()
                                                          << " (IEEE 1800-2023 6.20.6)");
                    }
                    nodep->dtypep(foundp->dtypep());
                    nodep->varp(varp);
                    nodep->didWidth(સાચું);
                    જો (nodep->fromp()->same_વૃક્ષ(m_randomizeFromp) && varp->isRand())  // null-safe
                        V3LinkLValue::linkLValueSet(nodep);
                    પાછા સાચું;
                }
                જો (AstConstraint* constrp = VN_CAST(foundp, Constraint)) {
                    nodep->replaceWith(નવું AstConstraintRef{
                        nodep->fileline(), nodep->fromp()->unlinkFrBack(), constrp});
                    VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
                    પાછા સાચું;
                }
                જો (AstEnumItemRef* અચલ adfoundp = VN_CAST(foundp, EnumItemRef)) {
                    nodep->replaceWith(adfoundp->clone_વૃક્ષ(ખોટું));
                    VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
                    પાછા સાચું;
                }
                જો (Ast_ગાંઠ_FTask* ftaskp = VN_CAST(foundp, ગાંઠ_FTask)) {
                    AstMethodCall* newp = નવું AstMethodCall{
                        nodep->fileline(), nodep->fromp()->unlinkFrBack(), nodep->name()};
                    newp->taskp(ftaskp);
                    newp->dtypep(ftaskp->dtypep());
                    newp->classOrPackagep(classp);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
                    પાછા સાચું;
                }
                જો (VN_IS(foundp, Constraint)) {
                    // We don't support constraints yet, so just keep as unlinked for now
                    // Presumably we'll next see a constraint_mode AstMethodCall
                    nodep->dtypep(nodep->findConstraintRefDType());
                    UINFO(9, "Unsupported constraint select " << nodep);
                    પાછા સાચું;
                }
                UINFO(1, "found object " << foundp);
                nodep->v3fatalSrc("MemberSel of non-variable\n"
                                  << nodep->warnContextPrimary() << '\n'
                                  << foundp->warnOther() << "... Location of found object\n"
                                  << foundp->warnContextSecondary());
            }
            classp = classp->extendsp() ? classp->extendsp()->classp() : nullptr;
        }

        VSpellCheck speller;
        માટે (AstClass* classp = first_classp; classp;) {
            માટે (Ast_ગાંઠ* itemp = classp->membersp(); itemp; itemp = itemp->nextp()) {
                જો (VN_IS(itemp, Constraint) || VN_IS(itemp, EnumItemRef) || VN_IS(itemp, Var)) {
                    speller.ધકેલો_Candidate(itemp->prettyName());
                }
            }
            classp = classp->extendsp() ? classp->extendsp()->classp() : nullptr;
        }
        અચલ string suggest = speller.best_Candidate_સંદેશ(nodep->prettyName());
        nodep->v3error(
            "Member " << nodep->prettyNameQ() << " not found in " << first_classp->verilogKwd()
                      << " " << first_classp->prettyNameQ() << "\n"
                      << (suggest.empty() ? "" : nodep->fileline()->warnMore() + suggest));
        પાછા ખોટું;  // Caller handles error
    }
    તાર્કિક modportAllowsItem(Ast_ગાંઠ* nodep, અચલ AstIfaceRefDType* adtypep, અચલ Ast_ગાંઠ* itemp) {
        // Variables, tasks/functions and clocking blocks of an interface are only visible
        // through a modport when the modport lists them
        અચલ AstModport* અચલ modportp = adtypep->modportp();
        જો (!modportp) પાછા સાચું;
        જો (અચલ AstVar* અચલ varp = VN_CAST(itemp, Var)) {
            જો (varp->isParam()) પાછા સાચું;
        } નહીંતર જો (!VN_IS(itemp, ગાંઠ_FTask) && !VN_IS(itemp, Clocking)) {
            પાછા સાચું;
        }
        auto pair = m_modportNames.emplace(modportp, std::unordered_set<string>{});
        જો (pair.second) {
            માટે (અચલ Ast_ગાંઠ* mpItemp = modportp->varsp(); mpItemp; mpItemp = mpItemp->nextp()) {
                pair.first->second.ઉમેરો(mpItemp->name());
            }
        }
        જો (pair.first->second.ગણતરી(itemp->name())) પાછા સાચું;
        nodep->v3error("Member " << itemp->prettyNameQ() << " found in interface "
                                 << adtypep->ifaceViaCellp()->prettyNameQ()
                                 << " but not in modport " << modportp->prettyNameQ()
                                 << " (IEEE 1800-2023 25.5)");
        પાછા ખોટું;
    }
    Ast_ગાંઠ* memberSelIface(AstMemberSel* nodep, AstIfaceRefDType* adtypep) {
        // Returns node if ok
        // No need to width-resolve the interface, as it was done when we did the child
        // ifaceViaCellp() handles dtypes with cellp-only (no ifacep), as produced
        // by sub-interface selection, enabling chained access (e.g. vif.tx.Tx).
        Ast_ગાંઠ_Module* અચલ ifacep = adtypep->ifaceViaCellp();
        UASSERT_OBJ(ifacep, nodep, "Unlinked");
        VSpellCheck speller;
        માટે (Ast_ગાંઠ* itemp = ifacep->stmtsp(); itemp; itemp = itemp->nextp()) {
            જો (itemp->name() == nodep->name()) {
                જો (!modportAllowsItem(nodep, adtypep, itemp)) {
                    પાછા nullptr;  // Caller handles error
                }
                પાછા itemp;
            }
            જો (VN_IS(itemp, Var) || VN_IS(itemp, Modport) || VN_IS(itemp, Cell)) {
                speller.ધકેલો_Candidate(itemp->prettyName());
            }
        }
        અચલ string suggest = speller.best_Candidate_સંદેશ(nodep->prettyName());
        nodep->v3error(
            "Member " << nodep->prettyNameQ() << " not found in interface "
                      << ifacep->prettyNameQ() << "\n"
                      << (suggest.empty() ? "" : nodep->fileline()->warnMore() + suggest));
        પાછા nullptr;  // Caller handles error
    }
    AstVar* memberSelClocking(AstMemberSel* nodep, AstClocking* clockingp) {
        // Returns node if ok
        VSpellCheck speller;

        માટે (Ast_ગાંઠ* itemp = clockingp->itemsp(); itemp; itemp = itemp->nextp()) {
            જો (AstClockingItem* citemp = VN_CAST(itemp, ClockingItem)) {
                જો (citemp->varp()->name() == nodep->name()) પાછા citemp->varp();
                speller.ધકેલો_Candidate(citemp->varp()->prettyName());
            }
        }
        અચલ string suggest = speller.best_Candidate_સંદેશ(nodep->prettyName());
        nodep->v3error(
            "Member " << nodep->prettyNameQ() << " not found in clocking block "
                      << clockingp->prettyNameQ() << "\n"
                      << (suggest.empty() ? "" : nodep->fileline()->warnMore() + suggest));
        પાછા nullptr;  // Caller handles error
    }
    તાર્કિક memberSelStruct(AstMemberSel* nodep, Ast_ગાંઠ_UOr_Struct_DType* adtypep) {
        // Returns true if ok
        જો (AstMemberDType* અચલ memberp
            = VN_CAST(m_memberMap.findMember(adtypep, nodep->name()), MemberDType)) {
            જો (m_attrp) {  // Looking for the base of the attribute
                nodep->dtypep(memberp);
                UINFO(9, "   MEMBERSEL(attr) -> " << nodep);
                UINFO(9, "           dt-> " << nodep->dtypep());
            } નહીંતર જો (adtypep->packed()) {
                AstSel* અચલ newp = નવું AstSel{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                                memberp->lsb(), memberp->width()};
                // Must skip over the member to find the union; as the member may disappear later
                newp->dtypep(memberp->subDTypep()->skipRefToEnump());
                newp->didWidth(સાચું);  // Don't replace dtype with basic type
                UINFO(9, "   MEMBERSEL -> " << newp);
                UINFO(9, "           dt-> " << newp->dtypep());
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
                // Should be able to treat it as a normal-ish nodesel - maybe.
                // The lhsp() will be strange until this stage; create the number here?
            } નહીંતર {
                AstStructSel* અચલ newp = નવું AstStructSel{
                    nodep->fileline(), nodep->fromp()->unlinkFrBack(), nodep->name()};
                // Must skip over the member to find the union; as the member may disappear later
                newp->dtypep(memberp->subDTypep()->skipRefToEnump());
                newp->didWidth(સાચું);  // Don't replace dtype with basic type
                UINFO(9, "   MEMBERSEL -> " << newp);
                UINFO(9, "           dt-> " << newp->dtypep());
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
                // Should be able to treat it as a normal-ish nodesel - maybe.
                // The lhsp() will be strange until this stage; create the number here?
            }
            પાછા સાચું;
        }
        nodep->v3error("Member " << nodep->prettyNameQ() << " not found in structure");
        પાછા ખોટું;
    }

    શૂન્ય visit(AstCMethodHard* nodep) override {
        // Never created before V3Width, so no need to redo it
        UASSERT_OBJ(nodep->dtypep(), nodep, "CMETHODCALLs should have already been sized");
    }

    શૂન્ય visit(AstMethodCall* nodep) override {
        UINFO(5, "   METHODCALL " << nodep);
        જો (nodep->didWidth()) પાછા;
        UINFOTREE(9, nodep, "", "mts-in");
        // Should check types the method requires, but at present we don't do much
        userIterate(nodep->fromp(), WidthVP{SELF, BOTH}.p());
        // Args are checked within each particular method's decode
        // Any AstWith is checked later when know types, in methodWithClause
        // Find the fromp dtype - should be a class
        UASSERT_OBJ(nodep->fromp() && nodep->fromp()->dtypep(), nodep, "Unsized expression");
        Ast_ગાંઠ_DType* અચલ fromDtp = nodep->fromp()->dtypep()->skipRefToEnump();
        AstBasicDType* અચલ basicp = fromDtp ? fromDtp->basicp() : nullptr;
        UINFO(9, "     from dt " << fromDtp);
        userIterate(fromDtp, WidthVP{SELF, BOTH}.p());
        જો (nodep->name() == "rand_mode") {
            methodCallRandMode(nodep);
        } નહીંતર જો (nodep->name() == "constraint_mode") {
            methodCallConstraint(nodep, nullptr);
        } નહીંતર જો (AstEnumDType* અચલ adtypep = VN_CAST(fromDtp, EnumDType)) {
            methodCallEnum(nodep, adtypep);
        } નહીંતર જો (AstAssocArrayDType* અચલ adtypep = VN_CAST(fromDtp, AssocArrayDType)) {
            methodCallAssoc(nodep, adtypep);
        } નહીંતર જો (AstWildcardArrayDType* અચલ adtypep = VN_CAST(fromDtp, WildcardArrayDType)) {
            methodCallWildcard(nodep, adtypep);
        } નહીંતર જો (AstDynArrayDType* અચલ adtypep = VN_CAST(fromDtp, DynArrayDType)) {
            methodCallDyn(nodep, adtypep);
        } નહીંતર જો (AstQueueDType* અચલ adtypep = VN_CAST(fromDtp, QueueDType)) {
            methodCallQueue(nodep, adtypep);
        } નહીંતર જો (AstClassRefDType* અચલ adtypep = VN_CAST(fromDtp, ClassRefDType)) {
            methodCallClass(nodep, adtypep);
        } નહીંતર જો (AstIfaceRefDType* અચલ adtypep = VN_CAST(fromDtp, IfaceRefDType)) {
            methodCallIfaceRef(nodep, adtypep);
        } નહીંતર જો (AstUnpackArrayDType* અચલ adtypep = VN_CAST(fromDtp, UnpackArrayDType)) {
            methodCallUnpack(nodep, adtypep);
        } નહીંતર જો (AstConstraintRefDType* અચલ adtypep = VN_CAST(fromDtp, ConstraintRefDType)) {
            methodCallConstraint(nodep, adtypep);
        } નહીંતર જો (basicp && basicp->isEvent()) {
            methodCallEvent(nodep, basicp);
        } નહીંતર જો (basicp && basicp->isString()) {
            methodCallString(nodep, basicp);
        } નહીંતર {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: Member call on object '"
                                             << nodep->fromp()->prettyTypeName()
                                             << "' which is a '"
                                             << nodep->fromp()->dtypep()->prettyTypeName() << "'");
            nodep->dtypeSetVoid();
        }
    }
    AstWith* methodWithClause(Ast_ગાંઠ_FTask_Ref* nodep, તાર્કિક required, તાર્કિક arbReturn,
                              Ast_ગાંઠ_DType* returnDtp, Ast_ગાંઠ_DType* indexDtp,
                              Ast_ગાંઠ_DType* કિંમત_Dtp) {
        UASSERT_OBJ(arbReturn || returnDtp, nodep, "Null return type");
        જો (AstWith* અચલ withp = nodep->withp()) {
            withp->indexArgRefp()->dtypep(indexDtp);
            withp->કિંમત_Arg_Refp()->dtypep(કિંમત_Dtp);
            userIterate(withp, WidthVP{returnDtp, BOTH}.p());
            withp->unlinkFrBack();
            પાછા withp;
        }
        જો (required) {
            nodep->v3error("'with' statement is required for ." << nodep->prettyName()
                                                                << " method");
        }
        પાછા nullptr;
    }
    શૂન્ય methodOkArguments(Ast_ગાંઠ_FTask_Ref* nodep, પૂર્ણાંક minArg, પૂર્ણાંક maxArg) {
        પૂર્ણાંક narg = 0;
        જો (AstWith* અચલ withp = nodep->withp()) {
            withp->v3error("'with' not legal on this method");
            VL_DO_DANGLING(ધકેલો_Deletep(withp->unlinkFrBack()), withp);
        }
        માટે (AstArg* argp = nodep->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) ++narg;
        અચલ તાર્કિક ok = (narg >= minArg) && (narg <= maxArg);
        જો (!ok) {
            nodep->v3error("The " << narg << " arguments passed to ." << nodep->prettyName()
                                  << " method does not match its requiring " << cvtToStr(minArg)
                                  << (minArg == maxArg ? "" : " to " + cvtToStr(maxArg))
                                  << " arguments");
            // Adjust to required argument counts, very bogus, but avoids core dump
            માટે (; narg < minArg; ++narg) {
                nodep->addArgsp(
                    નવું AstArg{nodep->fileline(), "",
                               નવું AstConst{nodep->fileline(), AstConst::BitFalseErroring{}}});
            }
            માટે (; narg > maxArg; --narg) {
                AstArg* argp = nodep->argsp();
                જ્યારે (argp->nextp()) argp = VN_AS(argp->nextp(), Arg);
                argp->unlinkFrBack();
                VL_DO_DANGLING(argp->delete_વૃક્ષ(), argp);
            }
        }
    }

    Ast_ગાંઠ_પદાવલિ* methodArg(AstMethodCall* nodep, પૂર્ણાંક arg) {
        AstArg* argp = nodep->argsp();
        માટે (પૂર્ણાંક narg = 0; narg < arg; ++narg) argp = VN_AS(argp->nextp(), Arg);
        UASSERT_OBJ(argp, nodep, "methodOkArguments() should have detected arg count error");
        પાછા argp->exprp();
    }

    શૂન્ય methodCallEnum(AstMethodCall* nodep, AstEnumDType* adtypep) {
        // Method call on enum without following parenthesis, e.g. "ENUM.next"
        // Convert this into a method call, and let that visitor figure out what to do next
        જો (nodep->name() == "num"  //
            || nodep->name() == "first"  //
            || nodep->name() == "last") {
            // Constant value
            AstConst* newp = nullptr;
            methodOkArguments(nodep, 0, 0);
            જો (nodep->name() == "num") {
                પૂર્ણાંક વસ્તુઓ = 0;
                માટે (Ast_ગાંઠ* itemp = adtypep->itemsp(); itemp; itemp = itemp->nextp()) ++વસ્તુઓ;
                newp = નવું AstConst(nodep->fileline(), AstConst::Signed32{}, વસ્તુઓ);
            } નહીંતર જો (nodep->name() == "first") {
                અચલ AstEnumItem* itemp = adtypep->itemsp();
                જો (!itemp) {
                    newp = નવું AstConst(nodep->fileline(), AstConst::Signed32{},
                                        0);  // Spec doesn't say what to do
                } નહીંતર {
                    newp = VN_AS(itemp->valuep()->clone_વૃક્ષ(ખોટું), Const);  // A const
                    newp->dtypeFrom(adtypep);  // To prevent a later ENUMVALUE
                }
            } નહીંતર જો (nodep->name() == "last") {
                અચલ AstEnumItem* itemp = adtypep->itemsp();
                જ્યારે (itemp && itemp->nextp()) itemp = VN_AS(itemp->nextp(), EnumItem);
                જો (!itemp) {
                    newp = નવું AstConst(nodep->fileline(), AstConst::Signed32{},
                                        0);  // Spec doesn't say what to do
                } નહીંતર {
                    newp = VN_AS(itemp->valuep()->clone_વૃક્ષ(ખોટું), Const);  // A const
                    newp->dtypeFrom(adtypep);  // To prevent a later ENUMVALUE
                }
            }
            UASSERT_OBJ(newp, nodep, "Enum method (perhaps enum item) not const");
            newp->fileline(nodep->fileline());  // Use method's filename/line number to be clearer;
                                                // may have warning disables
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
        } નહીંતર જો (nodep->name() == "name" || nodep->name() == "next" || nodep->name() == "prev") {
            VAttrType attrType;
            જો (nodep->name() == "name") {
                attrType = VAttrType::ENUM_NAME;
                methodOkArguments(nodep, 0, 0);
            } નહીંતર જો (nodep->name() == "next") {
                attrType = VAttrType::ENUM_NEXT;
                methodOkArguments(nodep, 0, 1);
            } નહીંતર જો (nodep->name() == "prev") {
                attrType = VAttrType::ENUM_PREV;
                methodOkArguments(nodep, 0, 1);
            } નહીંતર {
                nodep->v3fatalSrc("Bad case");
            }

            જો (nodep->name() != "name" && nodep->argsp()) {
                Ast_ગાંઠ_પદાવલિ* stepp = methodArg(nodep, 0);
                VL_DO_DANGLING(V3Const::constifyParamsNoWarnEdit(stepp), stepp);
                stepp = methodArg(nodep, 0);
                nodep->fileline()->modifyWarnOff(V3ErrorCode::WIDTHEXPAND, સાચું);
                iterateCheckUInt32(nodep, "argument", stepp, BOTH);
                જો (!VN_IS(stepp, Const)) {
                    stepp->v3warn(E_UNSUPPORTED,
                                  "Unsupported: enum next/prev with non-constant argument");
                    Ast_ગાંઠ_પદાવલિ* અચલ newp = નવું AstConst{stepp->fileline(), 1};
                    stepp->replaceWith(newp);
                    VL_DO_DANGLING(ધકેલો_Deletep(stepp), stepp);
                    stepp = newp;
                }
                અચલ uint32_t stepWidth = VN_AS(stepp, Const)->toUInt();
                જો (stepWidth == 0) {
                    // Step of 0 "legalizes" like $cast, use .next.prev
                    AstMethodCall* અચલ newp = નવું AstMethodCall{
                        nodep->fileline(),
                        નવું AstMethodCall{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                          "next"},
                        "prev"};
                    // No dtype assigned, we will recurse the new method and replace
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(nodep->delete_વૃક્ષ(), nodep);
                    પાછા;
                } નહીંતર જો (stepWidth != 1) {
                    // Unroll of enumVar.next(k) to enumVar.next(1).next(k - 1)
                    ધકેલો_Deletep(nodep->argsp()->unlinkFrBack());
                    AstMethodCall* અચલ clonep = nodep->clone_વૃક્ષ(ખોટું);
                    VN_AS(stepp, Const)->અંક().setLong(1);
                    AstConst* અચલ constp = નવું AstConst(nodep->fileline(), stepWidth - 1);
                    AstArg* અચલ argp = નવું AstArg{nodep->fileline(), "", constp};
                    AstMethodCall* અચલ newp
                        = નવું AstMethodCall{nodep->fileline(), clonep, nodep->name(), argp};
                    // No dtype assigned, we will recurse the new method and replace
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(nodep->delete_વૃક્ષ(), nodep);
                    પાછા;
                }
            }
            Ast_ગાંઠ_પદાવલિ* અચલ newp
                = enumSelect(nodep->fromp()->unlinkFrBack(), adtypep, attrType);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_વૃક્ષ(), nodep);
        } નહીંતર {
            nodep->v3error("Unknown built-in enum method " << nodep->prettyNameQ());
        }
    }
    શૂન્ય methodCallWildcard(AstMethodCall* nodep, AstWildcardArrayDType* adtypep) {
        AstCMethodHard* newp = nullptr;
        જો (nodep->name() == "num"  // function int num()
            || nodep->name() == "size") {
            methodOkArguments(nodep, 0, 0);
            newp = નવું AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::ASSOC_SIZE};  // So don't need num()
            newp->dtypeSetInt();
        } નહીંતર જો (nodep->name() == "first"  // function int first(ref index)
                   || nodep->name() == "last"  //
                   || nodep->name() == "next"  //
                   || nodep->name() == "prev"  //
                   || nodep->name() == "unique_index"  //
                   || nodep->name() == "find_index" || nodep->name() == "find_first_index"
                   || nodep->name() == "find_last_index") {
            nodep->v3error("Array method " << nodep->prettyNameQ()
                                           << " not legal on wildcard associative arrays");
        } નહીંતર જો (nodep->name() == "exists") {  // function int exists(input index)
            // IEEE really should have made this a "bit" return
            methodOkArguments(nodep, 1, 1);
            iterateCheckSelf(nodep, "argument", methodArg(nodep, 0), SELF, BOTH);
            Ast_ગાંઠ_પદાવલિ* અચલ index_exprp = method_Call_Wildcard_Index_પદાવલિ(nodep, adtypep);
            newp = નવું AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::ASSOC_EXISTS, index_exprp->unlinkFrBack()};
            newp->dtypeSetBitUnsized(32, 1, VSigning::SIGNED);
        } નહીંતર જો (nodep->name() == "delete") {  // function void delete([input integer index])
            methodOkArguments(nodep, 0, 1);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::WRITE);
            જો (!nodep->argsp()) {
                newp = નવું AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                          VCMethod::ASSOC_CLEAR};
                newp->dtypeSetVoid();
            } નહીંતર {
                iterateCheckSelf(nodep, "argument", methodArg(nodep, 0), SELF, BOTH);
                Ast_ગાંઠ_પદાવલિ* અચલ index_exprp = method_Call_Wildcard_Index_પદાવલિ(nodep, adtypep);
                newp = નવું AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                          VCMethod::ASSOC_ERASE, index_exprp->unlinkFrBack()};
                newp->dtypeSetVoid();
            }
        } નહીંતર જો (nodep->name() == "sort" || nodep->name() == "rsort"
                   || nodep->name() == "reverse" || nodep->name() == "shuffle") {
            nodep->v3error("Array method " << nodep->prettyNameQ()
                                           << " not legal on associative arrays");
        } નહીંતર જો (nodep->name() == "and" || nodep->name() == "or" || nodep->name() == "xor"
                   || nodep->name() == "sum" || nodep->name() == "product") {
            // All value return
            AstWith* અચલ withp
                = methodWithClause(nodep, ખોટું, ખોટું, adtypep->subDTypep(),
                                   adtypep->findStringDType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = નવું AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod("r_" + nodep->name())};
            newp->withp(withp);
            newp->dtypeFrom(withp ? withp->dtypep() : adtypep->subDTypep());
            જો (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } નહીંતર જો (nodep->name() == "min" || nodep->name() == "max" || nodep->name() == "unique") {
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = નવું AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->dtypep(queueDTypeIndexedBy(adtypep->subDTypep()));
            જો (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } નહીંતર જો (nodep->name() == "find" || nodep->name() == "find_first"
                   || nodep->name() == "find_last") {
            AstWith* અચલ withp
                = methodWithClause(nodep, સાચું, ખોટું, nodep->findBitDType(),
                                   adtypep->findStringDType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = નવું AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->withp(withp);
            newp->dtypep(queueDTypeIndexedBy(adtypep->subDTypep()));
            જો (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } નહીંતર જો (nodep->name() == "map") {
            AstWith* અચલ withp
                = methodWithClause(nodep, સાચું, ખોટું, adtypep->subDTypep(),
                                   adtypep->findStringDType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = નવું AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::ARRAY_MAP};
            newp->withp(withp);
            newp->dtypep(queueDTypeIndexedBy(withp ? withp->dtypep() : adtypep->subDTypep()));
            જો (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } નહીંતર {
            nodep->v3error("Unknown wildcard associative array method " << nodep->prettyNameQ());
            nodep->dtypeFrom(adtypep->subDTypep());  // Best guess
        }
        જો (newp) {
            newp->protect(ખોટું);
            newp->didWidth(સાચું);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_વૃક્ષ(), nodep);
        }
    }
    શૂન્ય methodCallAssoc(AstMethodCall* nodep, AstAssocArrayDType* adtypep) {
        AstCMethodHard* newp = nullptr;
        જો (nodep->withp() && adtypep->subDTypep()->isWide()) {
            nodep->v3warn(
                E_UNSUPPORTED,
                "Unsupported: `with` clause on assoc arrays with wide value types in method '"
                    << nodep->prettyName() << "'");
        }
        જો (nodep->name() == "num"  // function int num()
            || nodep->name() == "size") {
            methodOkArguments(nodep, 0, 0);
            newp = નવું AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::ASSOC_SIZE};  // So don't need num()
            newp->dtypeSetInt();
        } નહીંતર જો (nodep->name() == "first"  // function int first(ref index)
                   || nodep->name() == "last"  //
                   || nodep->name() == "next"  //
                   || nodep->name() == "prev") {
            methodOkArguments(nodep, 1, 1);
            iterateCheckTyped(nodep, "argument", methodArg(nodep, 0), adtypep->ચાવી_DTypep(), BOTH);
            Ast_ગાંઠ_પદાવલિ* અચલ index_exprp = method_Call_Assoc_Index_પદાવલિ(nodep, adtypep);
            methodCallLValueRecurse(nodep, index_exprp, VAccess::READWRITE);
            newp
                = નવું AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                     VCMethod::arrayMethod(nodep->name()),  // first/last/next/prev
                                     index_exprp->unlinkFrBack()};
            newp->dtypeSetInt();
            જો (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } નહીંતર જો (nodep->name() == "exists") {  // function int exists(input index)
            // IEEE really should have made this a "bit" return
            methodOkArguments(nodep, 1, 1);
            iterateCheckTyped(nodep, "argument", methodArg(nodep, 0), adtypep->ચાવી_DTypep(), BOTH);
            Ast_ગાંઠ_પદાવલિ* અચલ index_exprp = method_Call_Assoc_Index_પદાવલિ(nodep, adtypep);
            newp = નવું AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::ASSOC_EXISTS, index_exprp->unlinkFrBack()};
            newp->dtypeSetBitUnsized(32, 1, VSigning::SIGNED);
        } નહીંતર જો (nodep->name() == "delete") {  // function void delete([input integer index])
            methodOkArguments(nodep, 0, 1);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::WRITE);
            જો (!nodep->argsp()) {
                newp = નવું AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                          VCMethod::ASSOC_CLEAR};
                newp->dtypeSetVoid();
            } નહીંતર {
                iterateCheckTyped(nodep, "argument", methodArg(nodep, 0), adtypep->ચાવી_DTypep(),
                                  BOTH);
                Ast_ગાંઠ_પદાવલિ* અચલ index_exprp = method_Call_Assoc_Index_પદાવલિ(nodep, adtypep);
                newp = નવું AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                          VCMethod::ASSOC_ERASE, index_exprp->unlinkFrBack()};
                newp->dtypeSetVoid();
            }
        } નહીંતર જો (nodep->name() == "sort" || nodep->name() == "rsort"
                   || nodep->name() == "reverse" || nodep->name() == "shuffle") {
            nodep->v3error("Array method " << nodep->prettyNameQ()
                                           << " not legal on associative arrays");
        } નહીંતર જો (nodep->name() == "and" || nodep->name() == "or" || nodep->name() == "xor"
                   || nodep->name() == "sum" || nodep->name() == "product") {
            // All value return
            AstWith* અચલ withp = methodWithClause(nodep, ખોટું, ખોટું, adtypep->subDTypep(),
                                                    adtypep->ચાવી_DTypep(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = નવું AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod("r_" + nodep->name())};
            newp->withp(withp);
            newp->dtypeFrom(withp ? withp->dtypep() : adtypep->subDTypep());
            જો (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } નહીંતર જો (nodep->name() == "min" || nodep->name() == "max" || nodep->name() == "unique"
                   || nodep->name() == "unique_index") {
            AstWith* અચલ withp = methodWithClause(
                nodep, ખોટું, સાચું, nullptr, nodep->findUInt32DType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = નવું AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->withp(withp);
            જો (nodep->name() == "unique_index") {
                newp->dtypep(queueDTypeIndexedBy(adtypep->ચાવી_DTypep()));
            } નહીંતર {
                newp->dtypep(queueDTypeIndexedBy(adtypep->subDTypep()));
            }
            જો (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } નહીંતર જો (nodep->name() == "find" || nodep->name() == "find_first"
                   || nodep->name() == "find_last") {
            AstWith* અચલ withp = methodWithClause(nodep, સાચું, ખોટું, nodep->findBitDType(),
                                                    adtypep->ચાવી_DTypep(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = નવું AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->withp(withp);
            newp->dtypep(queueDTypeIndexedBy(adtypep->subDTypep()));
            જો (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } નહીંતર જો (nodep->name() == "find_index" || nodep->name() == "find_first_index"
                   || nodep->name() == "find_last_index") {
            AstWith* અચલ withp = methodWithClause(nodep, સાચું, ખોટું, nodep->findBitDType(),
                                                    adtypep->ચાવી_DTypep(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = નવું AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->withp(withp);
            newp->dtypep(queueDTypeIndexedBy(adtypep->ચાવી_DTypep()));
            જો (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } નહીંતર જો (nodep->name() == "map") {
            AstWith* અચલ withp = methodWithClause(nodep, સાચું, ખોટું, adtypep->subDTypep(),
                                                    adtypep->ચાવી_DTypep(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = નવું AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::ARRAY_MAP};
            newp->withp(withp);
            newp->dtypep(queueDTypeIndexedBy(withp ? withp->dtypep() : adtypep->subDTypep()));
            જો (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } નહીંતર {
            nodep->v3error("Unknown built-in associative array method " << nodep->prettyNameQ());
            nodep->dtypeFrom(adtypep->subDTypep());  // Best guess
        }
        જો (newp) {
            newp->protect(ખોટું);
            newp->didWidth(સાચું);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_વૃક્ષ(), nodep);
        }
    }
    Ast_ગાંઠ_પદાવલિ* method_Call_Assoc_Index_પદાવલિ(AstMethodCall* nodep, AstAssocArrayDType* adtypep) {
        Ast_ગાંઠ* અચલ index_exprp = nodep->argsp()->exprp();
        iterateCheck(nodep, "index", index_exprp, CONTEXT_DET, FINAL, adtypep->ચાવી_DTypep(),
                     EXTEND_EXP);
        VL_DANGLING(index_exprp);  // May have been edited
        પાછા nodep->argsp()->exprp();
    }
    Ast_ગાંઠ_પદાવલિ* method_Call_Wildcard_Index_પદાવલિ(AstMethodCall* nodep,
                                             AstWildcardArrayDType* adtypep) {
        Ast_ગાંઠ* અચલ index_exprp = nodep->argsp()->exprp();
        iterateCheck(nodep, "index", index_exprp, CONTEXT_DET, FINAL, adtypep->findStringDType(),
                     EXTEND_EXP);
        VL_DANGLING(index_exprp);  // May have been edited
        પાછા nodep->argsp()->exprp();
    }
    શૂન્ય methodCallLValueRecurse(AstMethodCall* nodep, Ast_ગાંઠ* childp, અચલ VAccess& access) {
        જો (AstCMethodHard* અચલ ichildp = VN_CAST(childp, CMethodHard)) {
            તાર્કિક methAt = ખોટું;
            તાર્કિક methAtBack = ખોટું;
            તાર્કિક methAtWrite = ખોટું;
            પસંદગી (ichildp->method()) {
            સ્થિતિ VCMethod::ARRAY_AT:  // FALLTHRU
                methAt = સાચું;
                તોડો;
            સ્થિતિ VCMethod::ARRAY_AT_BACK:  // FALLTHRU
                methAtBack = સાચું;
                તોડો;
            સ્થિતિ VCMethod::ARRAY_AT_WRITE:  // FALLTHRU
            સ્થિતિ VCMethod::DYN_AT_WRITE_APPEND:  // FALLTHRU
            સ્થિતિ VCMethod::DYN_AT_WRITE_APPEND_BACK:  //
                methAtWrite = સાચું;
                તોડો;
            મૂળભૂત: તોડો;
            }
            જો (methAt || methAtBack || methAtWrite) {
                અચલ Ast_ગાંઠ_DType* અચલ fromDtypep = ichildp->fromp()->dtypep()->skipRefp();
                જો (VN_IS(fromDtypep, QueueDType) || VN_IS(fromDtypep, DynArrayDType)) {
                    // Change access methods to writable ones
                    જો (methAt) {
                        ichildp->method(VCMethod::ARRAY_AT_WRITE);
                    } નહીંતર જો (methAtBack) {
                        ichildp->method(VCMethod::DYN_AT_WRITE_APPEND_BACK);
                    }
                }
                methodCallLValueRecurse(nodep, ichildp->fromp(), access);
                પાછા;
            }
        }
        જો (Ast_ગાંઠ_Var_Ref* અચલ varrefp = VN_CAST(childp, ગાંઠ_Var_Ref)) {
            varrefp->access(access);
        } નહીંતર જો (અચલ AstMemberSel* અચલ ichildp = VN_CAST(childp, MemberSel)) {
            methodCallLValueRecurse(nodep, ichildp->fromp(), access);
        } નહીંતર જો (અચલ AstStructSel* અચલ ichildp = VN_CAST(childp, StructSel)) {
            methodCallLValueRecurse(nodep, ichildp->fromp(), access);
        } નહીંતર જો (અચલ Ast_ગાંઠ_Sel* અચલ ichildp = VN_CAST(childp, ગાંઠ_Sel)) {
            methodCallLValueRecurse(nodep, ichildp->fromp(), access);
        } નહીંતર જો (VN_IS(childp, LambdaArgRef) || VN_IS(childp, FuncRef)) {
            // NOP
        } નહીંતર {
            UINFO(1, "    Related node: " << childp);
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: Non-variable on LHS of built-in method '"
                                             << nodep->prettyName() << "'");
        }
    }
    AstCMethodHard* methodCallArray(AstMethodCall* nodep, Ast_ગાંઠ_DType* adtypep) {
        AstCMethodHard* newp = nullptr;

        જો (nodep->name() == "reverse" || nodep->name() == "shuffle" || nodep->name() == "sort"
            || nodep->name() == "rsort") {
            AstWith* withp = nullptr;
            જો (nodep->name() == "sort" || nodep->name() == "rsort") {
                withp = methodWithClause(nodep, ખોટું, સાચું, nullptr, nodep->findUInt32DType(),
                                         adtypep->subDTypep());
            }
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::WRITE);
            newp = નવું AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->withp(withp);
            newp->dtypeSetVoid();
        } નહીંતર જો (nodep->name() == "min" || nodep->name() == "max" || nodep->name() == "unique"
                   || nodep->name() == "unique_index") {
            AstWith* અચલ withp = methodWithClause(
                nodep, ખોટું, સાચું, nullptr, nodep->findUInt32DType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = નવું AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->withp(withp);
            જો (nodep->name() == "unique_index") {
                newp->dtypep(newp->findQueueIndexDType());
            } નહીંતર {
                newp->dtypep(queueDTypeIndexedBy(adtypep->subDTypep()));
            }
            જો (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } નહીંતર જો (nodep->name() == "find" || nodep->name() == "find_first"
                   || nodep->name() == "find_last" || nodep->name() == "find_index") {
            AstWith* અચલ withp
                = methodWithClause(nodep, સાચું, ખોટું, nodep->findBitDType(),
                                   nodep->findUInt32DType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = નવું AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->withp(withp);
            newp->dtypep(queueDTypeIndexedBy(adtypep->subDTypep()));
            જો (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } નહીંતર જો (nodep->name() == "find_index" || nodep->name() == "find_first_index"
                   || nodep->name() == "find_last_index") {
            AstWith* અચલ withp
                = methodWithClause(nodep, સાચું, ખોટું, nodep->findBitDType(),
                                   nodep->findUInt32DType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = નવું AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->withp(withp);
            newp->dtypep(newp->findQueueIndexDType());
            જો (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } નહીંતર જો (nodep->name() == "map") {
            // map() - IEEE 1800-2023 7.12.5
            // Returns a queue with same element count, each element is the with expression result
            AstWith* અચલ withp
                = methodWithClause(nodep, સાચું, ખોટું, adtypep->subDTypep(),
                                   nodep->findUInt32DType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = નવું AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::ARRAY_MAP};
            newp->withp(withp);
            newp->dtypep(queueDTypeIndexedBy(withp ? withp->dtypep() : adtypep->subDTypep()));
            જો (!nodep->firstAbovep()) newp->dtypeSetVoid();
        }
        પાછા newp;
    }
    શૂન્ય methodCallDyn(AstMethodCall* nodep, AstDynArrayDType* adtypep) {
        AstCMethodHard* newp = nullptr;
        જો (nodep->name() == "at") {  // Created internally for []
            methodOkArguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
            newp = નવું AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::ARRAY_AT};
            newp->dtypeFrom(adtypep->subDTypep());
        } નહીંતર જો (nodep->name() == "atWrite") {  // Created internally for []
            methodOkArguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::WRITE);
            newp = નવું AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::ARRAY_AT_WRITE};
            newp->dtypeFrom(adtypep->subDTypep());
        } નહીંતર જો (nodep->name() == "size") {
            methodOkArguments(nodep, 0, 0);
            newp = નવું AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::DYN_SIZE};
            newp->dtypeSetInt();
        } નહીંતર જો (nodep->name() == "delete") {  // function void delete()
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::WRITE);
            newp = નવું AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::DYN_CLEAR};
            newp->dtypeSetVoid();
        } નહીંતર જો (nodep->name() == "and" || nodep->name() == "or" || nodep->name() == "xor"
                   || nodep->name() == "sum" || nodep->name() == "product") {
            // All value return
            AstWith* અચલ withp
                = methodWithClause(nodep, ખોટું, ખોટું, adtypep->subDTypep(),
                                   nodep->findUInt32DType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = નવું AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod("r_" + nodep->name())};
            newp->withp(withp);
            newp->dtypeFrom(withp ? withp->dtypep() : adtypep->subDTypep());
            જો (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } નહીંતર જો ((newp = methodCallArray(nodep, adtypep))) {
        } નહીંતર {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported/unknown built-in dynamic array method "
                                             << nodep->prettyNameQ());
            nodep->dtypeFrom(adtypep->subDTypep());  // Best guess
        }
        જો (newp) {
            newp->protect(ખોટું);
            newp->didWidth(સાચું);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_વૃક્ષ(), nodep);
        }
    }
    શૂન્ય methodCallQueue(AstMethodCall* nodep, AstQueueDType* adtypep) {
        AstCMethodHard* newp = nullptr;
        જો (nodep->name() == "at" || nodep->name() == "atBack") {  // Created internally for []
            methodOkArguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
            newp = નવું AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->dtypeFrom(adtypep->subDTypep());
        } નહીંતર જો (nodep->name() == "atWriteAppend"
                   || nodep->name() == "atWriteAppendBack") {  // Created internally for []
            methodOkArguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::WRITE);
            newp = નવું AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->dtypeFrom(adtypep->subDTypep());
        } નહીંતર જો (nodep->name() == "num"  // function int num()
                   || nodep->name() == "size") {
            methodOkArguments(nodep, 0, 0);
            newp = નવું AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::DYN_SIZE};
            newp->dtypeSetInt();
        } નહીંતર જો (nodep->name() == "delete") {  // function void delete([input integer index])
            methodOkArguments(nodep, 0, 1);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::WRITE);
            જો (!nodep->argsp()) {
                newp = નવું AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                          VCMethod::DYN_CLEAR};
                newp->dtypeSetVoid();
            } નહીંતર {
                iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
                Ast_ગાંઠ_પદાવલિ* અચલ index_exprp = method_Call_Queue_Index_પદાવલિ(nodep);
                જો (index_exprp->isZero()) {  // delete(0) is a pop_front
                    newp = નવું AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                              VCMethod::DYN_કાઢો_FRONT};
                    newp->dtypeFrom(adtypep->subDTypep());
                    newp->dtypeSetVoid();
                } નહીંતર {
                    newp = નવું AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                              VCMethod::DYN_ERASE, index_exprp->unlinkFrBack()};
                    newp->dtypeSetVoid();
                }
            }
        } નહીંતર જો (nodep->name() == "insert") {
            methodOkArguments(nodep, 2, 2);
            iterateCheckSigned32(nodep, "index", methodArg(nodep, 0), BOTH);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::WRITE);
            Ast_ગાંઠ_પદાવલિ* અચલ index_exprp = method_Call_Queue_Index_પદાવલિ(nodep);
            AstArg* અચલ argp = VN_AS(nodep->argsp()->nextp(), Arg);
            iterateCheckTyped(nodep, "insert value", argp->exprp(), adtypep->subDTypep(), BOTH);
            જો (index_exprp->isZero()) {  // insert(0, ...) is a push_front
                newp = નવું AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                          VCMethod::DYN_ધકેલો_FRONT, argp->exprp()->unlinkFrBack()};
                newp->dtypeSetVoid();
            } નહીંતર {
                newp = નવું AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                          VCMethod::DYN_ઉમેરો, index_exprp->unlinkFrBack()};
                newp->addPinsp(argp->exprp()->unlinkFrBack());
                newp->dtypeSetVoid();
            }
        } નહીંતર જો (nodep->name() == "pop_front" || nodep->name() == "pop_back") {
            methodOkArguments(nodep, 0, 0);
            // Returns element, so method both consumes (reads) and modifies the queue
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READWRITE);
            newp = નવું AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->dtypeFrom(adtypep->subDTypep());
        } નહીંતર જો (nodep->name() == "push_back" || nodep->name() == "push_front") {
            methodOkArguments(nodep, 1, 1);
            iterateCheckTyped(nodep, "argument", methodArg(nodep, 0), adtypep->subDTypep(), BOTH);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::WRITE);
            AstArg* અચલ argp = nodep->argsp();
            iterateCheckTyped(nodep, "push value", argp->exprp(), adtypep->subDTypep(), BOTH);
            newp = નવું AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name()),
                                      argp->exprp()->unlinkFrBack()};
            newp->dtypeSetVoid();
        } નહીંતર જો (nodep->name() == "and" || nodep->name() == "or" || nodep->name() == "xor"
                   || nodep->name() == "sum" || nodep->name() == "product") {
            AstWith* અચલ withp
                = methodWithClause(nodep, ખોટું, ખોટું, adtypep->subDTypep(),
                                   nodep->findUInt32DType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = નવું AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod("r_" + nodep->name())};
            newp->withp(withp);
            newp->dtypeFrom(withp ? withp->dtypep() : adtypep->subDTypep());
            જો (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } નહીંતર જો ((newp = methodCallArray(nodep, adtypep))) {
        } નહીંતર {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported/unknown built-in queue method " << nodep->prettyNameQ());
            nodep->dtypeFrom(adtypep->subDTypep());  // Best guess
        }
        જો (newp) {
            newp->protect(ખોટું);
            newp->didWidth(સાચું);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_વૃક્ષ(), nodep);
        }
    }
    Ast_ગાંઠ_પદાવલિ* method_Call_Queue_Index_પદાવલિ(AstMethodCall* nodep) {
        Ast_ગાંઠ* અચલ index_exprp = nodep->argsp()->exprp();
        iterateCheckSigned32(nodep, "index", index_exprp, BOTH);
        VL_DANGLING(index_exprp);  // May have been edited
        પાછા nodep->argsp()->exprp();
    }
    શૂન્ય methodCallWarnTiming(Ast_ગાંઠ_FTask_Ref* અચલ nodep, અચલ std::string& className) {
        જો (v3Global.opt.timing().isSetFalse()) {
            nodep->v3warn(E_NOTIMING,
                          className << "::" << nodep->prettyName() << "() requires --timing");
        } નહીંતર જો (!v3Global.opt.timing().isSetTrue()) {
            nodep->v3warn(E_NEEDTIMINGOPT, "Use --timing or --no-timing to specify how "
                                               << className << "::" << nodep->name()
                                               << "() should be handled");
        }
    }
    શૂન્ય methodCallIfaceRef(AstMethodCall* nodep, AstIfaceRefDType* adtypep) {
        // ifaceViaCellp() resolves the interface via cellp when ifacep is null,
        // as for a sub-interface selected through a virtual interface handle.
        AstIface* અચલ ifacep = adtypep->ifaceViaCellp();
        UINFO(5, __FUNCTION__ << ":" << nodep);
        જો (Ast_ગાંઠ_FTask* અચલ ftaskp
            = VN_CAST(m_memberMap.findMember(ifacep, nodep->name()), ગાંઠ_FTask)) {
            UINFO(5, __FUNCTION__ << "AstNodeFTask" << nodep);
            જો (!modportAllowsItem(nodep, adtypep, ftaskp)) {
                nodep->dtypeSetVoid();
                પાછા;
            }
            જો (adtypep->isVirtual()) {
                માટે (Ast_ગાંઠ* itemp = ifacep->stmtsp(); itemp; itemp = itemp->nextp()) {
                    જો (AstVar* અચલ mvarp = VN_CAST(itemp, Var)) { mvarp->sensIfacep(ifacep); }
                }
            }
            userIterate(ftaskp, nullptr);
            જો (ftaskp->isStatic()) {
                AstArg* અચલ argsp = nodep->argsp();
                જો (argsp) argsp->unlinkFrBackWithNext();
                Ast_ગાંઠ_FTask_Ref* newp = nullptr;
                જો (VN_IS(ftaskp, Task)) {
                    newp = નવું AstTaskRef{nodep->fileline(), VN_AS(ftaskp, Task), argsp};
                } નહીંતર {
                    newp = નવું AstFuncRef{nodep->fileline(), VN_AS(ftaskp, Func), argsp};
                }
                newp->classOrPackagep(ifacep);
                nodep->replaceWith(newp);
                VL_DO_DANGLING(nodep->delete_વૃક્ષ(), nodep);
            } નહીંતર {
                nodep->taskp(ftaskp);
                nodep->dtypeFrom(ftaskp);
                nodep->classOrPackagep(ifacep);
                જો (VN_IS(ftaskp, Task)) nodep->dtypeSetVoid();
                processFTaskRefArgs(nodep);
            }
            પાછા;
        }
        nodep->v3error("Member reference from interface to "
                       << nodep->prettyNameQ() << " is not referencing a valid task or function ");
    }
    શૂન્ય handleRandomizeArgs(Ast_ગાંઠ_FTask_Ref* અચલ nodep, AstClass* અચલ classp) {
        તાર્કિક hasNonNullArgs = ખોટું;
        AstConst* nullp = nullptr;
        માટે (AstArg *argp = nodep->argsp(), *nextp; argp; argp = nextp) {
            nextp = VN_AS(argp->nextp(), Arg);
            AstVar* randVarp = nullptr;
            Ast_ગાંઠ_પદાવલિ* exprp = argp->exprp();
            જો (AstConst* અચલ constp = VN_CAST(exprp, Const)) {
                જો (constp->અંક().isNull()) {
                    nullp = constp;
                    ચાલુ;
                }
            }
            hasNonNullArgs = સાચું;
            AstVar* fromVarp = nullptr;  // If it's a method call, the leftmost element
                                         // of the dot hierarchy
            જો (AstMethodCall* methodCallp = VN_CAST(nodep, MethodCall)) {
                Ast_ગાંઠ_પદાવલિ* fromp = methodCallp->fromp();
                જ્યારે (AstMemberSel* અચલ memberSelp = VN_CAST(fromp, MemberSel)) {
                    fromp = memberSelp->fromp();
                }
                AstVarRef* અચલ varrefp = VN_AS(fromp, VarRef);
                fromVarp = varrefp->varp();
            }
            જો (!VN_IS(exprp, VarRef) && !VN_IS(exprp, MemberSel)) {
                argp->v3error("'randomize()' argument must be a variable contained in "
                              << (fromVarp ? fromVarp->prettyNameQ() : classp->prettyNameQ()));
                VL_DO_DANGLING(argp->unlinkFrBack()->delete_વૃક્ષ(), argp);
                ચાલુ;
            }
            જ્યારે (exprp) {
                જો (AstMemberSel* અચલ memberSelp = VN_CAST(exprp, MemberSel)) {
                    randVarp = memberSelp->varp();
                    exprp = memberSelp->fromp();
                } નહીંતર {
                    જો (AstVarRef* અચલ varrefp = VN_CAST(exprp, VarRef)) {
                        randVarp = varrefp->varp();
                    } નહીંતર {
                        argp->v3warn(
                            E_UNSUPPORTED,
                            "Unsupported: Non-variable expression as 'randomize()' argument");
                        VL_DO_DANGLING(argp->unlinkFrBack()->delete_વૃક્ષ(), argp);
                    }
                    exprp = nullptr;
                }
                // All variables in the dot hierarchy must be randomizable
                જો (randVarp && !randVarp->isRand()) randVarp->rand(VRandAttr::RAND_INLINE);
            }
            // randVarp is now the leftmost element from the dot hierarchy in argp->exprp()
            જો (randVarp == fromVarp) {
                // The passed in variable is MemberSel'ected from the MethodCall target
            } નહીંતર જો (classp->existsMember([&](અચલ AstClass*, અચલ AstVar* memberVarp) {
                           પાછા memberVarp == randVarp;
                       })) {
                // The passed in variable is contained in the method call target
            } નહીંતર {
                // Passed in a constant or complex expression, or the above conditions are not
                // met
                argp->v3error("'randomize()' argument must be a variable contained in "
                              << (fromVarp ? fromVarp->prettyNameQ() : classp->prettyNameQ()));
                VL_DO_DANGLING(argp->unlinkFrBack()->delete_વૃક્ષ(), argp);
            }
        }
        જો (nullp && hasNonNullArgs) {
            nullp->v3error("Cannot pass more arguments to 'randomize(null)'");
        }
        // A solo 'null' arg is left in place; V3Randomize lowers it into a
        // check-only solve so constraints are validated against the current
        // values of every rand member (IEEE 1800-2023 18.11).
    }
    શૂન્ય methodCallClass(AstMethodCall* nodep, AstClassRefDType* adtypep) {
        // No need to width-resolve the class, as it was done when we did the child
        AstClass* અચલ first_classp = adtypep->classp();
        AstWith* withp = nullptr;
        જો (nodep->name() == "randomize") {
            VL_RESTORER(m_randomizeFromp);
            m_randomizeFromp = nodep->fromp();
            withp = methodWithClause(nodep, ખોટું, ખોટું, adtypep->findVoidDType(),
                                     adtypep->findBitDType(), adtypep);
            માટે (AstArg* argp = nodep->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) {
                જો (argp->exprp()) userIterate(argp->exprp(), WidthVP{SELF, BOTH}.p());
            }
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            V3Randomize::newRandomizeFunc(m_memberMap, first_classp);
            handleRandomizeArgs(nodep, first_classp);
        } નહીંતર જો (nodep->name() == "srandom") {
            methodOkArguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            V3Randomize::newSRandomFunc(m_memberMap, first_classp);
        }
        UASSERT_OBJ(first_classp, nodep, "Unlinked");
        માટે (AstClass* classp = first_classp; classp;) {
            જો (nodep->fileline()->timingOn()) {
                જો (classp->name() == "semaphore" || classp->name() == "process"
                    || VString::startsWith(classp->name(), "mailbox")) {
                    // Find the package the class is in
                    AstPackage* અચલ packagep = getItemPackage(classp);
                    // Check if it's std
                    જો (packagep && packagep->name() == "std") {
                        જો (classp->name() == "process") {
                            methodCallWarnTiming(nodep, "process");
                        } નહીંતર જો (classp->name() == "semaphore" && nodep->name() == "get") {
                            methodCallWarnTiming(nodep, "semaphore");
                        } નહીંતર જો (nodep->name() == "put" || nodep->name() == "get"
                                   || nodep->name() == "peek") {
                            methodCallWarnTiming(nodep, "mailbox");
                        }
                    }
                }
            }
            જો (Ast_ગાંઠ_FTask* અચલ ftaskp
                = VN_CAST(m_memberMap.findMember(classp, nodep->name()), ગાંઠ_FTask)) {
                userIterate(ftaskp, nullptr);
                જો (ftaskp->isStatic()) {
                    AstArg* અચલ argsp = nodep->argsp();
                    જો (argsp) argsp->unlinkFrBackWithNext();
                    Ast_ગાંઠ_FTask_Ref* newp = nullptr;
                    // We use m_vup to determine task or function, so that later error checks
                    // for funcref->task and taskref->func will pick up properly
                    જો (!m_vup) {  // Called as task
                        newp = નવું AstTaskRef{nodep->fileline(), nodep->name(), argsp};
                    } નહીંતર {
                        newp = નવું AstFuncRef{nodep->fileline(), nodep->name(), argsp};
                    }
                    newp->taskp(ftaskp);  // Not passed above as might be func vs task mismatch
                    newp->classOrPackagep(classp);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(nodep->delete_વૃક્ષ(), nodep);
                } નહીંતર {
                    nodep->taskp(ftaskp);
                    nodep->dtypeFrom(ftaskp);
                    nodep->classOrPackagep(classp);
                    જો (VN_IS(ftaskp, Task)) {
                        જો (m_vup && m_vup->prelim() && !VN_IS(nodep->backp(), Stmt_પદાવલિ)) {
                            nodep->v3error(
                                "Cannot call a task/void-function as a member function: "
                                << nodep->prettyNameQ());
                        }
                        nodep->dtypeSetVoid();
                    }
                    જો (withp) nodep->withp(withp);
                    processFTaskRefArgs(nodep);
                }
                પાછા;
            } નહીંતર જો (nodep->name() == "get_randstate") {
                methodOkArguments(nodep, 0, 0);
                first_classp->baseMostClassp()->needRNG(સાચું);
                v3Global.useRandomizeMethods(સાચું);
                AstCMethodHard* અચલ newp
                    = નવું AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                         VCMethod::RNG_GET_RANDSTATE, nullptr};
                newp->usePtr(સાચું);
                newp->dtypeSetString();
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
                પાછા;
            } નહીંતર જો (nodep->name() == "set_randstate") {
                methodOkArguments(nodep, 1, 1);
                Ast_ગાંઠ_પદાવલિ* અચલ expr1p = nodep->argsp()->exprp();  // May edit
                iterateCheckString(nodep, "LHS", expr1p, BOTH);
                Ast_ગાંઠ_પદાવલિ* અચલ exprp = nodep->argsp()->exprp();
                first_classp->baseMostClassp()->needRNG(સાચું);
                v3Global.useRandomizeMethods(સાચું);
                AstCMethodHard* અચલ newp
                    = નવું AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                         VCMethod::RNG_SET_RANDSTATE, exprp->unlinkFrBack()};
                newp->usePtr(સાચું);
                newp->dtypeSetVoid();
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
                પાછા;
            } નહીંતર જો (nodep->name() == "constraint_mode") {
                v3Global.useRandomizeMethods(સાચું);
                nodep->dtypep(nodep->findBasicDType(VBasicDTypeKwd::INT));
            }
            classp = classp->extendsp() ? classp->extendsp()->classp() : nullptr;
        }
        {
            VSpellCheck speller;
            માટે (AstClass* classp = first_classp; classp;) {
                માટે (Ast_ગાંઠ* itemp = classp->membersp(); itemp; itemp = itemp->nextp()) {
                    જો (VN_IS(itemp, ગાંઠ_FTask)) speller.ધકેલો_Candidate(itemp->prettyName());
                }
                classp = classp->extendsp() ? classp->extendsp()->classp() : nullptr;
            }
            અચલ string suggest = speller.best_Candidate_સંદેશ(nodep->prettyName());
            nodep->v3error("Class method "
                           << nodep->prettyNameQ() << " not found in "
                           << first_classp->verilogKwd() << " " << first_classp->prettyNameQ()
                           << "\n"
                           << (suggest.empty() ? "" : nodep->fileline()->warnMore() + suggest));
        }
        nodep->dtypeSetInt();  // Guess on error
    }
    શૂન્ય methodCallConstraint(AstMethodCall* nodep, AstConstraintRefDType*) {
        જો (nodep->name() == "constraint_mode") {
            // IEEE 1800-2023 18.9
            methodOkArguments(nodep, 0, 1);
            જો (nodep->argsp()) {
                iterateCheckBool(nodep, "argument", methodArg(nodep, 0), BOTH);
                nodep->dtypep(nodep->findBasicDType(VBasicDTypeKwd::INT));
            } નહીંતર {
                nodep->dtypeSetVoid();
            }
            v3Global.useRandomizeMethods(સાચું);
        } નહીંતર {
            nodep->v3error("No such constraint method " << nodep->prettyNameQ());
            nodep->replaceWith(નવું AstConst{nodep->fileline(), AstConst::BitFalseErroring{}});
            VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
        }
    }
    શૂન્ય methodCallRandMode(AstMethodCall* nodep) {
        methodOkArguments(nodep, 0, 1);
        // IEEE 1800-2023 18.8
        જો (nodep->argsp()) {
            iterateCheckBool(nodep, "argument", methodArg(nodep, 0), BOTH);
            nodep->dtypeSetVoid();
        } નહીંતર {
            nodep->dtypep(nodep->findBasicDType(VBasicDTypeKwd::INT));
        }
        v3Global.useRandomizeMethods(સાચું);
    }
    શૂન્ય methodCallUnpack(AstMethodCall* nodep, AstUnpackArrayDType* adtypep) {
        ગણના : uint8_t {
            UNKNOWN = 0,
            ARRAY_OR,
            ARRAY_AND,
            ARRAY_XOR,
            ARRAY_SUM,
            ARRAY_PRODUCT
        } methodId;

        methodId = UNKNOWN;
        જો (nodep->name() == "or") {
            methodId = ARRAY_OR;
        } નહીંતર જો (nodep->name() == "and") {
            methodId = ARRAY_AND;
        } નહીંતર જો (nodep->name() == "xor") {
            methodId = ARRAY_XOR;
        } નહીંતર જો (nodep->name() == "sum") {
            methodId = ARRAY_SUM;
        } નહીંતર જો (nodep->name() == "product") {
            methodId = ARRAY_PRODUCT;
        }

        જો (methodId) {
            AstWith* અચલ withp
                = methodWithClause(nodep, ખોટું, ખોટું, adtypep->subDTypep(),
                                   nodep->findUInt32DType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            જો (withp) {
                methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
                AstCMethodHard* અચલ newp
                    = નવું AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                         VCMethod::arrayMethod("r_" + nodep->name())};
                newp->withp(withp);
                newp->dtypeFrom(withp ? withp->dtypep() : adtypep->subDTypep());
                જો (!nodep->firstAbovep()) newp->dtypeSetVoid();
                newp->protect(ખોટું);
                newp->didWidth(સાચું);
                nodep->replaceWith(newp);
                VL_DO_DANGLING(nodep->delete_વૃક્ષ(), nodep);
                પાછા;
            } નહીંતર {
                // TODO use this code for small vectors, otherwise use runtime reduction.
                // Historically we expand all vectors.
                File_લીટી* અચલ fl = nodep->fileline();
                Ast_ગાંઠ_પદાવલિ* newp = nullptr;
                માટે (પૂર્ણાંક i = 0; i < adtypep->elementsConst(); ++i) {
                    Ast_ગાંઠ_પદાવલિ* અચલ arrayRef = nodep->fromp()->clone_વૃક્ષ_Pure(ખોટું);
                    Ast_ગાંઠ_પદાવલિ* અચલ selector = નવું AstArraySel{fl, arrayRef, i};
                    જો (!newp) {
                        newp = selector;
                    } નહીંતર {
                        પસંદગી (methodId) {
                        સ્થિતિ ARRAY_OR: newp = નવું AstOr{fl, newp, selector}; તોડો;
                        સ્થિતિ ARRAY_AND: newp = નવું AstAnd{fl, newp, selector}; તોડો;
                        સ્થિતિ ARRAY_XOR: newp = નવું AstXor{fl, newp, selector}; તોડો;
                        સ્થિતિ ARRAY_SUM: newp = નવું AstAdd{fl, newp, selector}; તોડો;
                        સ્થિતિ ARRAY_PRODUCT: newp = નવું AstMul{fl, newp, selector}; તોડો;
                        મૂળભૂત: nodep->v3fatalSrc("bad case");
                        }
                    }
                }
                nodep->replaceWith(newp);
                VL_DO_DANGLING(nodep->delete_વૃક્ષ(), nodep);
            }
        } નહીંતર જો (AstCMethodHard* newp = methodCallArray(nodep, adtypep)) {
            newp->protect(ખોટું);
            newp->didWidth(સાચું);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_વૃક્ષ(), nodep);
        } નહીંતર {
            nodep->v3error("Unknown built-in array method " << nodep->prettyNameQ());
            nodep->dtypeFrom(adtypep->subDTypep());  // Best guess
        }
    }
    શૂન્ય methodCallEvent(AstMethodCall* nodep, AstBasicDType*) {
        // Method call on event
        જો (nodep->name() == "triggered") {
            methodOkArguments(nodep, 0, 0);
            AstCMethodHard* અચલ callp = નવું AstCMethodHard{
                nodep->fileline(), nodep->fromp()->unlinkFrBack(), VCMethod::EVENT_IS_TRIGGERED};
            callp->dtypeSetBit();
            nodep->replaceWith(callp);
            VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
        } નહીંતર {
            nodep->v3error("Unknown built-in event method " << nodep->prettyNameQ());
        }
    }
    શૂન્ય methodCallString(AstMethodCall* nodep, AstBasicDType*) {
        // Method call on string
        જો (nodep->name() == "len") {
            // Constant value
            methodOkArguments(nodep, 0, 0);
            Ast_ગાંઠ* અચલ newp = નવું AstLenN{nodep->fileline(), nodep->fromp()->unlinkFrBack()};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
        } નહીંતર જો (nodep->name() == "itoa") {
            methodOkArguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
            VL_DO_DANGLING(replaceWithSFormat(nodep, "%0d"), nodep);
        } નહીંતર જો (nodep->name() == "hextoa") {
            methodOkArguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
            VL_DO_DANGLING(replaceWithSFormat(nodep, "%0x"), nodep);
        } નહીંતર જો (nodep->name() == "octtoa") {
            methodOkArguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
            VL_DO_DANGLING(replaceWithSFormat(nodep, "%0o"), nodep);
        } નહીંતર જો (nodep->name() == "bintoa") {
            methodOkArguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
            VL_DO_DANGLING(replaceWithSFormat(nodep, "%0b"), nodep);
        } નહીંતર જો (nodep->name() == "realtoa") {
            methodOkArguments(nodep, 1, 1);
            iterateCheckReal(nodep, "argument", methodArg(nodep, 0), BOTH);
            VL_DO_DANGLING(replaceWithSFormat(nodep, "%g"), nodep);
        } નહીંતર જો (nodep->name() == "tolower") {
            methodOkArguments(nodep, 0, 0);
            Ast_ગાંઠ* અચલ newp
                = નવું AstToLowerN{nodep->fileline(), nodep->fromp()->unlinkFrBack()};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_વૃક્ષ(), nodep);
        } નહીંતર જો (nodep->name() == "toupper") {
            methodOkArguments(nodep, 0, 0);
            Ast_ગાંઠ* અચલ newp
                = નવું AstToUpperN{nodep->fileline(), nodep->fromp()->unlinkFrBack()};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_વૃક્ષ(), nodep);
        } નહીંતર જો (nodep->name() == "compare" || nodep->name() == "icompare") {
            અચલ તાર્કિક ignoreCase = nodep->name()[0] == 'i';
            methodOkArguments(nodep, 1, 1);
            iterateCheckString(nodep, "argument", methodArg(nodep, 0), BOTH);
            AstArg* અચલ argp = nodep->argsp();
            Ast_ગાંઠ_પદાવલિ* અચલ lhs = nodep->fromp()->unlinkFrBack();
            Ast_ગાંઠ_પદાવલિ* અચલ rhs = argp->exprp()->unlinkFrBack();
            Ast_ગાંઠ* અચલ newp = નવું AstCompareNN{nodep->fileline(), lhs, rhs, ignoreCase};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_વૃક્ષ(), nodep);
        } નહીંતર જો (nodep->name() == "putc") {
            methodOkArguments(nodep, 2, 2);
            iterateCheckSigned32(nodep, "argument 0", methodArg(nodep, 0), BOTH);
            iterateCheckSigned8(nodep, "argument 1", methodArg(nodep, 1), BOTH);
            AstArg* અચલ arg0p = nodep->argsp();
            AstArg* અચલ arg1p = VN_AS(arg0p->nextp(), Arg);
            Ast_ગાંઠ_Var_Ref* અચલ fromp = VN_AS(nodep->fromp()->unlinkFrBack(), VarRef);
            Ast_ગાંઠ_પદાવલિ* અચલ rhsp = arg0p->exprp()->unlinkFrBack();
            Ast_ગાંઠ_પદાવલિ* અચલ thsp = arg1p->exprp()->unlinkFrBack();
            AstVarRef* અચલ varrefp
                = નવું AstVarRef{nodep->fileline(), fromp->varp(), VAccess::READ};
            Ast_ગાંઠ* અચલ newp = નવું AstAssign{
                nodep->fileline(), fromp, નવું AstPutcN{nodep->fileline(), varrefp, rhsp, thsp}};
            fromp->access(VAccess::WRITE);
            ધકેલો_Deletep(nodep->backp());
            VL_DO_DANGLING(nodep->backp()->replaceWith(newp), nodep);
        } નહીંતર જો (nodep->name() == "getc") {
            methodOkArguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
            AstArg* અચલ arg0p = nodep->argsp();
            Ast_ગાંઠ_પદાવલિ* અચલ lhsp = nodep->fromp()->unlinkFrBack();
            Ast_ગાંઠ_પદાવલિ* અચલ rhsp = arg0p->exprp()->unlinkFrBack();
            Ast_ગાંઠ_પદાવલિ* અચલ newp = નવું AstGetcN{nodep->fileline(), lhsp, rhsp};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_વૃક્ષ(), nodep);
        } નહીંતર જો (nodep->name() == "substr") {
            methodOkArguments(nodep, 2, 2);
            iterateCheckSigned32(nodep, "argument 0", methodArg(nodep, 0), BOTH);
            iterateCheckSigned32(nodep, "argument 1", methodArg(nodep, 1), BOTH);
            AstArg* અચલ arg0p = nodep->argsp();
            AstArg* અચલ arg1p = VN_AS(arg0p->nextp(), Arg);
            Ast_ગાંઠ_પદાવલિ* અચલ lhsp = nodep->fromp()->unlinkFrBack();
            Ast_ગાંઠ_પદાવલિ* અચલ rhsp = arg0p->exprp()->unlinkFrBack();
            Ast_ગાંઠ_પદાવલિ* અચલ thsp = arg1p->exprp()->unlinkFrBack();
            Ast_ગાંઠ_પદાવલિ* અચલ newp = નવું AstSubstrN{nodep->fileline(), lhsp, rhsp, thsp};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_વૃક્ષ(), nodep);
        } નહીંતર જો (nodep->name() == "atobin" || nodep->name() == "atohex"
                   || nodep->name() == "atoi" || nodep->name() == "atooct"
                   || nodep->name() == "atoreal") {
            AstAtoN::FmtType fmt;
            જો (nodep->name() == "atobin") {
                fmt = AstAtoN::ATOBIN;
            } નહીંતર જો (nodep->name() == "atohex") {
                fmt = AstAtoN::ATOHEX;
            } નહીંતર જો (nodep->name() == "atoi") {
                fmt = AstAtoN::ATOI;
            } નહીંતર જો (nodep->name() == "atooct") {
                fmt = AstAtoN::ATOOCT;
            } નહીંતર જો (nodep->name() == "atoreal") {
                fmt = AstAtoN::ATOREAL;
            } નહીંતર {
                nodep->v3fatalSrc("Bad case");
                fmt = AstAtoN::ATOI;
            }  // dummy assignment to suppress compiler warning
            methodOkArguments(nodep, 0, 0);
            Ast_ગાંઠ* અચલ newp
                = નવું AstAtoN{nodep->fileline(), nodep->fromp()->unlinkFrBack(), fmt};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_વૃક્ષ(), nodep);
        } નહીંતર {
            nodep->v3error("Unknown built-in string method " << nodep->prettyNameQ());
        }
    }
    AstQueueDType* queueDTypeIndexedBy(Ast_ગાંઠ_DType* indexDTypep) {
        // Return a Queue data type with the specified index, remembering so can use again if
        // needed
        જો (AstQueueDType* અચલ queuep = m_queueDTypeIndexed[indexDTypep]) {
            પાછા queuep;
        } નહીંતર {
            AstQueueDType* અચલ newp
                = નવું AstQueueDType{indexDTypep->fileline(), indexDTypep, nullptr};
            v3Global.rootp()->typeTablep()->addTypesp(newp);
            m_queueDTypeIndexed[indexDTypep] = newp;
            પાછા newp;
        }
    }

    શૂન્ય visit(AstNew* nodep) override {
        જો (nodep->didWidth()) પાછા;
        AstClass* classp = nullptr;
        તાર્કિક assign = ખોટું;
        જો (VN_IS(nodep->backp(), Assign)) {  // assignment case
            assign = સાચું;
            Ast_ગાંઠ* warnp = nullptr;
            AstClassRefDType* refp = nullptr;

            જો (nodep->isScoped()) {  // = ClassOrPackage::new
                UASSERT_OBJ(nodep->classOrPackagep(), nodep, "Unlinked classOrPackage");
                warnp = nodep->classOrPackagep();
                જો (AstClass* અચલ clsp = VN_CAST(warnp, Class)) {
                    AstClassRefDType* અચલ adtypep
                        = નવું AstClassRefDType{nodep->fileline(), clsp, nullptr};
                    v3Global.rootp()->typeTablep()->addTypesp(adtypep);
                    refp = adtypep;
                }
            } નહીંતર {  // = new
                warnp = m_vup->dtypeNullp();
                refp = m_vup ? VN_CAST(m_vup->dtypeNullSkipRefp(), ClassRefDType) : nullptr;
            }
            જો (!refp) {  // e.g. int a = new;
                nodep->v3error(
                    "new() assignment not legal to non-class "
                    + (VN_IS(warnp, ગાંઠ_DType)
                           ? ("data type "s + VN_AS(warnp, ગાંઠ_DType)->prettyDTypeNameQ())
                       : warnp ? warnp->prettyNameQ()
                               : ""));
                nodep->dtypep(m_vup->dtypep());
                પાછા;
            }
            nodep->dtypep(refp);

            classp = refp->classp();
            UASSERT_OBJ(classp, nodep, "Unlinked");
            જો (Ast_ગાંઠ_FTask* અચલ ftaskp
                = VN_CAST(m_memberMap.findMember(classp, "new"), Func)) {
                nodep->taskp(ftaskp);
                nodep->classOrPackagep(classp);
            } નહીંતર {
                // Either made explicitly or V3LinkDot made implicitly
                classp->v3fatalSrc("Can't find class's new");
            }
        } નહીંતર {  // super.new case
            // in this case class and taskp() should be properly linked in V3LinkDot.cpp during
            // "super" reference resolution
            classp = VN_CAST(nodep->classOrPackagep(), Class);
            UASSERT_OBJ(classp, nodep, "Unlinked classOrPackagep()");
            UASSERT_OBJ(nodep->taskp(), nodep, "Unlinked taskp()");
        }
        processFTaskRefArgs(nodep);
        જો (!assign) nodep->dtypeFrom(nodep->taskp());
    }
    શૂન્ય visit(AstNewCopy* nodep) override {
        જો (nodep->didWidthAndSet()) પાછા;
        AstClassRefDType* અચલ refp = VN_CAST(m_vup->dtypeNullSkipRefp(), ClassRefDType);
        જો (!refp) {  // e.g. int a = new;
            nodep->v3error("new() cannot copy from non-class data type "
                           + (m_vup->dtypeNullp() ? m_vup->dtypep()->prettyDTypeNameQ() : ""));
            nodep->dtypep(m_vup->dtypep());
            પાછા;
        }
        nodep->dtypep(refp);
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
        જો (!similarDTypeRecurse(nodep->dtypep(), nodep->rhsp()->dtypep())) {
            nodep->rhsp()->v3error("New-as-copier passed different data type '"
                                   << nodep->dtypep()->prettyTypeName() << "' than expected '"
                                   << nodep->rhsp()->dtypep()->prettyTypeName() << "'");
        }
    }
    શૂન્ય visit(AstNewDynamic* nodep) override {
        જો (nodep->didWidthAndSet()) પાછા;
        AstDynArrayDType* અચલ adtypep = VN_CAST(m_vup->dtypeNullSkipRefp(), DynArrayDType);
        જો (!adtypep) {  // e.g. int a = new;
            nodep->v3error(
                "dynamic new() not expected in this context (data type must be dynamic array)");
            પાછા;
        }
        // The AstNodeAssign visitor will be soon be replacing this node, make sure it gets it
        જો (!VN_IS(nodep->backp(), ગાંઠ_Assign)) {
            UINFO(1, "Got backp " << nodep->backp());
            nodep->v3error(
                "dynamic new() not expected in this context (expected under an assign)");
            પાછા;
        }
        nodep->dtypep(adtypep);
        જો (m_vup && m_vup->prelim()) {
            iterateCheckSigned32(nodep, "new() size", nodep->sizep(), BOTH);
        }
        જો (nodep->rhsp()) {
            iterateCheckTyped(nodep, "Dynamic array new RHS", nodep->rhsp(), adtypep, BOTH);
            // A keyed or defaulted initializer became a constructor, which takes the size
            AstConsDynArray* અચલ consp = VN_CAST(nodep->rhsp(), ConsDynArray);
            જો (consp && (consp->defaultp() || (consp->itemsp() && consp->itemsp()->keyp()))) {
                consp->sizep(nodep->sizep()->unlinkFrBack());
                nodep->replaceWith(consp->unlinkFrBack());
                VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
            }
        }
    }

    શૂન્ય visit(Ast_Tagged_પદાવલિ* nodep) override {
        // Tagged union expressions are currently unsupported
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: tagged union");
        // Set a placeholder type to allow further processing
        nodep->dtypeSetBit();
        userIterateChildren(nodep, m_vup);
    }
    શૂન્ય visit(AstTaggedPattern* nodep) override {
        // Tagged patterns are currently unsupported
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: tagged pattern");
        nodep->dtypeSetBit();
        userIterateChildren(nodep, m_vup);
    }
    શૂન્ય visit(AstPatternVar* nodep) override {
        // Pattern variable bindings are currently unsupported
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: pattern variable");
        nodep->dtypeSetBit();
    }
    શૂન્ય visit(AstPatternStar* nodep) override {
        // Pattern wildcards are currently unsupported
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: pattern wildcard");
        nodep->dtypeSetBit();
    }
    શૂન્ય visit(AstMatches* nodep) override {
        // Matches operator is currently unsupported
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: matches operator");
        // Matches returns a boolean
        nodep->dtypeSetBit();
        userIterateChildren(nodep, m_vup);
    }

    શૂન્ય visit(AstPattern* nodep) override {
        જો (m_inParamOverride && !nodep->childDTypep() && !m_vup->dtypeNullp()) {
            // The specialized module types an override pattern, so only type its members here
            VL_RESTORER(m_inParamOverride);
            માટે (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;
                 patp = VN_AS(patp->nextp(), PatMember)) {
                m_inParamOverride = ખોટું;
                userIterateAndNext(patp->repp(), WidthVP{SELF, BOTH}.p());
                // Struct member names and type keys need no typing
                જો (!VN_IS(patp->keyp(), Text) && !VN_IS(patp->keyp(), ગાંઠ_DType)) {
                    userIterateAndNext(patp->keyp(), WidthVP{SELF, BOTH}.p());
                }
                માટે (Ast_ગાંઠ *nextp, *valuep = patp->lhssp(); valuep; valuep = nextp) {
                    nextp = valuep->nextp();
                    // A nested pattern stays untyped, any other value is typed now
                    m_inParamOverride = VN_IS(valuep, Pattern);
                    userIterate(valuep, WidthVP{SELF, BOTH}.p());
                }
            }
            પાછા;
        }
        // Any other pattern is typed here, so its members aren't override patterns
        VL_RESTORER(m_inParamOverride);
        m_inParamOverride = ખોટું;
        જો (nodep->didWidthAndSet()) પાછા;
        UINFO(9, "PATTERN " << nodep);
        જો (nodep->childDTypep()) {  // data_type '{ pattern }
            nodep->dtypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        }
        જો (!nodep->dtypep() && m_vup->dtypeNullp()) {  // Get it from parent assignment/pin/etc
            nodep->dtypep(m_vup->dtypep());
        }
        Ast_ગાંઠ_DType* dtypep = nodep->dtypep();
        જો (!dtypep) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported/Illegal: Assignment pattern"
                                         " member not underneath a supported construct: "
                                             << nodep->backp()->prettyTypeName());

            જો (nodep->backp() && (VN_IS(nodep->backp(), Eq) || VN_IS(nodep->backp(), Neq)))
                પાછા;
            nodep->replaceWith(નવું AstConst{nodep->fileline(), AstConst::BitFalse{}});
            VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
            પાછા;
        }
        {
            dtypep = dtypep->skipRefp();
            nodep->dtypep(dtypep);
            UINFO(9, "  dtypep " << dtypep);
            nodep->dtypep(dtypep);
            જો (VN_IS(dtypep, UnionDType)) {
                nodep->v3error("Assignment pattern cannot be used for union"
                               " (IEEE 1800-2023 10.9)");
                પાછા;
            }
            // Determine replication count, and replicate initial value as
            // widths need to be individually determined
            માટે (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;
                 patp = VN_AS(patp->nextp(), PatMember)) {
                અચલ પૂર્ણાંક times = visitPatMemberRep(patp);
                માટે (પૂર્ણાંક i = 1; i < times; i++) {
                    AstPatMember* અચલ newp = patp->clone_વૃક્ષ(ખોટું);
                    patp->addNextHere(newp);
                    // This loop will see the new elements as part of nextp()
                }
            }
            // Convert any PatMember with multiple items to multiple PatMembers
            માટે (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;
                 patp = VN_AS(patp->nextp(), PatMember)) {
                જો (patp->lhssp()->nextp()) {
                    // Can't just addNext, as would add to end of all members.
                    // So detach, add next and reattach
                    VNRelinker relinkHandle;
                    patp->unlinkFrBack(&relinkHandle);
                    જ્યારે (Ast_ગાંઠ_પદાવલિ* અચલ movep = VN_AS(patp->lhssp()->nextp(), ગાંઠ_પદાવલિ)) {
                        movep->unlinkFrBack();  // Not unlinkFrBackWithNext, just one
                        Ast_ગાંઠ* newkeyp = nullptr;
                        જો (patp->keyp()) newkeyp = patp->keyp()->clone_વૃક્ષ(સાચું);
                        AstPatMember* અચલ newp
                            = નવું AstPatMember{patp->fileline(), movep, newkeyp, nullptr};
                        patp->addNext(newp);
                    }
                    relinkHandle.relink(patp);
                }
            }
            AstPatMember* defaultp = nullptr;
            માટે (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;) {
                AstPatMember* અચલ nextp = VN_AS(patp->nextp(), PatMember);
                જો (patp->isDefault()) {
                    જો (defaultp) {
                        VL_DO_DANGLING(ધકેલો_Deletep(defaultp), defaultp);
                        nodep->v3error("Multiple '{ default: } clauses");
                    }
                    defaultp = patp;
                    patp->unlinkFrBack();
                }
                patp = nextp;
            }
            જ્યારે (અચલ AstConstDType* અચલ vdtypep = VN_CAST(dtypep, ConstDType)) {
                dtypep = vdtypep->subDTypep()->skipRefp();
            }

            userIterate(dtypep, WidthVP{SELF, BOTH}.p());

            જો (auto* અચલ vdtypep = VN_CAST(dtypep, StructDType)) {
                patternStruct(nodep, vdtypep, defaultp);
            } નહીંતર જો (auto* અચલ vdtypep = VN_CAST(dtypep, ગાંઠ_Array_DType)) {
                patternArray(nodep, vdtypep, defaultp);
            } નહીંતર જો (VN_IS(dtypep, AssocArrayDType)) {
                AstConsAssoc* અચલ consp = નવું AstConsAssoc{nodep->fileline()};
                patternKeyed<AstConsAssocItem>(nodep, consp, dtypep, defaultp);
            } નહીંતર જો (VN_IS(dtypep, WildcardArrayDType)) {
                AstConsWildcard* અચલ consp = નવું AstConsWildcard{nodep->fileline()};
                patternKeyed<AstConsWildcardItem>(nodep, consp, dtypep, defaultp);
            } નહીંતર જો (VN_IS(dtypep, DynArrayDType) || VN_IS(dtypep, QueueDType)) {
                patternDynArrayOrQueue(nodep, dtypep, defaultp);
            } નહીંતર જો (VN_IS(dtypep, BasicDType) && VN_AS(dtypep, BasicDType)->isRanged()) {
                patternBasic(nodep, dtypep, defaultp);
            } નહીંતર {
                nodep->v3warn(
                    E_UNSUPPORTED,
                    "Unsupported: Assignment pattern applies against non-struct/union data type: "
                        << dtypep->prettyDTypeNameQ());
                nodep->replaceWith(નવું AstConst{nodep->fileline(), AstConst::BitFalse{}});
            }

            // Done with the Pattern
            VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
            જો (defaultp) VL_DO_DANGLING(ધકેલો_Deletep(defaultp), defaultp);
        }
    }
    શૂન્ય patternStruct(AstPattern* nodep, AstStructDType* vdtypep, AstPatMember* defaultp) {
        // Due to "default" and tagged patterns, we need to determine
        // which member each AstPatMember corresponds to before we can
        // determine the dtypep for that PatMember's value, and then
        // width the initial value appropriately.
        using PatMap = std::map<અચલ AstMemberDType*, AstPatMember*>;
        PatMap patmap;  // Store member: value
        DTypeMap dtypemap;  // Store data_type: default_value
        {
            અચલ AstMemberDType* memp = vdtypep->membersp();
            AstPatMember* patp = VN_CAST(nodep->itemsp(), PatMember);
            જ્યારે (patp) {
                કરો {
                    જો (patp->keyp()) {
                        // '{member:value} or '{data_type: default_value}
                        જો (અચલ AstText* textp = VN_CAST(patp->keyp(), Text)) {
                            // member: value
                            memp = VN_CAST(m_memberMap.findMember(vdtypep, textp->text()),
                                           MemberDType);
                            જો (!memp) {
                                patp->keyp()->v3error("Assignment pattern key '"
                                                      << textp->text() << "' not found as member");
                                તોડો;
                            } નહીંતર {
                                અચલ std::pair<PatMap::iterator, તાર્કિક> ret
                                    = patmap.emplace(memp, patp);
                                જો (!ret.second) {
                                    patp->v3error("Assignment pattern contains duplicate entry: "
                                                  << VN_AS(patp->keyp(), Text)->text());
                                }
                                memp = VN_AS(memp->nextp(), MemberDType);
                            }
                        } નહીંતર જો (Ast_ગાંઠ_DType* nodedtypep = VN_CAST(patp->keyp(), ગાંઠ_DType)) {
                            // data_type: default_value
                            userIterate(nodedtypep, WidthVP{SELF, BOTH}.p());
                            અચલ string dtype = nodedtypep->dtypep()->prettyDTypeName(સાચું);
                            // Override stored default_value
                            dtypemap[dtype] = patp;
                        } નહીંતર {
                            // Undefined pattern
                            patp->keyp()->v3error(
                                "Assignment pattern key not supported/understood: "
                                << patp->keyp()->prettyTypeName());
                        }
                    } નહીંતર જો (memp) {
                        // constant expr
                        અચલ std::pair<PatMap::iterator, તાર્કિક> ret = patmap.emplace(memp, patp);
                        જો (!ret.second) {
                            patp->v3error("Assignment pattern contains duplicate entry: "
                                          << VN_AS(patp->keyp(), Text)->text());
                        }
                        memp = VN_AS(memp->nextp(), MemberDType);
                    } નહીંતર {
                        patp->v3error(
                            "Assignment pattern contains more entries than structure members");
                    }
                } જ્યારે (ખોટું);

                // Next
                જો (patp) patp = VN_AS(patp->nextp(), PatMember);
            }
        }
        Ast_ગાંઠ_પદાવલિ* newp = nullptr;
        જો (vdtypep->packed()) {
            માટે (AstMemberDType* memp = vdtypep->membersp(); memp;
                 memp = VN_AS(memp->nextp(), MemberDType)) {
                અચલ auto it = patmap.find(memp);
                જો (it == patmap.end()) {  // default or default_type assignment
                    જો (AstStructDType* અચલ memp_nested_vdtypep
                        = VN_CAST(memp->virtRefDTypep(), StructDType)) {
                        newp = nestedvalueConcat_patternStruct(memp_nested_vdtypep, defaultp, newp,
                                                               nodep, dtypemap);
                    } નહીંતર {
                        AstPatMember* અચલ patp
                            = defaultPatp_patternStruct(nodep, memp, defaultp, dtypemap);
                        newp = કિંમત_Concat_pattern_Struct(patp, newp, memp, nodep);
                        જો (patp) VL_DO_DANGLING(ધકેલો_Deletep(patp), patp);
                    }
                } નહીંતર {  // member assignment
                    newp = કિંમત_Concat_pattern_Struct(it->second, newp, memp, nodep);
                }
            }
        } નહીંતર {  // Unpacked
            AstConsStruct* અચલ consp = નવું AstConsStruct{nodep->fileline(), vdtypep};
            માટે (AstMemberDType* memp = vdtypep->membersp(); memp;
                 memp = VN_AS(memp->nextp(), MemberDType)) {
                અચલ auto it = patmap.find(memp);
                AstPatMember* patp = nullptr;
                જો (it == patmap.end()) {  // Default or default_type assignment
                    patp = defaultPatp_patternStruct(nodep, memp, defaultp, dtypemap);
                    ધકેલો_Deletep(patp);  // patp used below
                    patp = defaultPatp_forDType(patp, memp->virtRefDTypep(), dtypemap);
                    ધકેલો_Deletep(patp);  // patp used below
                } નહીંતર {
                    patp = it->second;  // Member assignment
                }
                patp->dtypep(memp);
                Ast_ગાંઠ_પદાવલિ* અચલ valuep = pattern_Member_કિંમત_Iterate(patp);
                consp->addItemsp(નવું AstConsStructItem{patp->fileline(), memp, valuep});
            }
            newp = consp;
        }
        જો (newp) {
            nodep->replaceWith(newp);
        } નહીંતર {
            nodep->v3error("Assignment pattern with no members");
        }
    }

    Ast_ગાંઠ_પદાવલિ* nestedvalueConcat_patternStruct(AstStructDType* memp_vdtypep,
                                                 AstPatMember* defaultp, Ast_ગાંઠ_પદાવલિ* newp,
                                                 AstPattern* nodep, અચલ DTypeMap& dtypemap) {
        માટે (AstMemberDType* memp_nested = memp_vdtypep->membersp(); memp_nested;
             memp_nested = VN_AS(memp_nested->nextp(), MemberDType)) {
            જો (AstStructDType* અચલ memp_multinested_vdtypep
                = VN_CAST(memp_nested->virtRefDTypep(), StructDType)) {
                newp = nestedvalueConcat_patternStruct(memp_multinested_vdtypep, defaultp, newp,
                                                       nodep, dtypemap);
            } નહીંતર {
                AstPatMember* અચલ patp
                    = defaultPatp_patternStruct(nodep, memp_nested, defaultp, dtypemap);
                newp = કિંમત_Concat_pattern_Struct(patp, newp, memp_nested, nodep);
                જો (patp) VL_DO_DANGLING(ધકેલો_Deletep(patp), patp);
            }
        }
        પાછા newp;
    }

    AstPatMember* defaultPatp_patternStruct(AstPattern* nodep, AstMemberDType* memp,
                                            AstPatMember* defaultp, અચલ DTypeMap& dtypemap) {
        અચલ string memp_DType = memp->virtRefDTypep()->prettyDTypeName(સાચું);
        અચલ auto it = dtypemap.find(memp_DType);
        જો (it != dtypemap.end()) {
            // default_value for data_type
            પાછા it->second->clone_વૃક્ષ(ખોટું);
        }
        જો (defaultp) {
            // default_value for any unmatched member yet
            પાછા defaultp->clone_વૃક્ષ(ખોટું);
        }
        જો (memp->valuep()) {
            પાછા નવું AstPatMember{nodep->fileline(),
                                    VN_AS(memp->valuep()->clone_વૃક્ષ(ખોટું), ગાંઠ_પદાવલિ),
                                    નવું AstText{nodep->fileline(), memp->name()}, nullptr};
        }
        nodep->v3error("Assignment pattern missed initializing elements: "
                       << memp->virtRefDTypep()->prettyDTypeNameQ() << " " << memp->prettyNameQ());
        પાછા nullptr;
    }

    Ast_ગાંઠ_પદાવલિ* કિંમત_Concat_pattern_Struct(AstPatMember* patp, Ast_ગાંઠ_પદાવલિ* newp,
                                           AstMemberDType* memp, AstPattern* nodep) {
        જો (patp) {
            patp->dtypep(memp);
            Ast_ગાંઠ_પદાવલિ* અચલ valuep = pattern_Member_કિંમત_Iterate(patp);
            જો (!newp) {
                newp = valuep;
            } નહીંતર {
                AstConcat* અચલ concatp = નવું AstConcat{patp->fileline(), newp, valuep};
                // Have resolved widths, no reason to warn about widths on the concat itself
                newp->fileline()->modifyWarnOff(V3ErrorCode::WIDTHCONCAT, સાચું);
                valuep->fileline()->modifyWarnOff(V3ErrorCode::WIDTHCONCAT, સાચું);
                newp = concatp;
                newp->dtypeChgSigned(nodep->dtypep()->numeric());
            }
        }
        પાછા newp;
    }

    AstPatMember* defaultPatp_forDType(AstPatMember* defaultp, Ast_ગાંઠ_DType* elemDTypep,
                                       અચલ DTypeMap& dtypemap = DTypeMap{}) {
        Ast_ગાંઠ_પદાવલિ* અચલ valuep = defaultp->lhssp()->clone_વૃક્ષ(ખોટું);
        Ast_ગાંઠ_DType* અચલ elemDTypeSkipRefp = elemDTypep->skipRefp();
        અચલ AstStructDType* અચલ structp = VN_CAST(elemDTypeSkipRefp, StructDType);
        અચલ તાર્કિક unpackedStruct = structp && !structp->packed();

        જો (!VN_IS(elemDTypeSkipRefp, UnpackArrayDType) && !unpackedStruct) {
            VL_DO_DANGLING(ધકેલો_Deletep(valuep), valuep);
            પાછા defaultp->clone_વૃક્ષ(ખોટું);
        }
        જો (VN_IS(valuep, Pattern)) {
            VL_DO_DANGLING(ધકેલો_Deletep(valuep), valuep);
            પાછા defaultp->clone_વૃક્ષ(ખોટું);
        }
        જો (!valuep->dtypep()) userIterate(valuep, WidthVP{SELF, BOTH}.p());
        તાર્કિક wholeElement = unpackedStruct;
        જો (Ast_ગાંઠ_DType* અચલ કિંમત_DTypep = valuep->dtypep()) {
            // An array value only if of the matching type, else it applies to its elements
            // (IEEE 1800-2023 10.9.1)
            wholeElement = unpackedStruct ? !કિંમત_DTypep->skipRefp()->isIntegralOrPacked()
                                          : કિંમત_DTypep->similarDType(elemDTypeSkipRefp);
        }
        જો (wholeElement) {
            VL_DO_DANGLING(ધકેલો_Deletep(valuep), valuep);
            પાછા defaultp->clone_વૃક્ષ(ખોટું);
        }

        AstPatMember* અચલ nestedDefaultp
            = નવું AstPatMember{defaultp->fileline(), valuep, nullptr, nullptr};
        nestedDefaultp->isDefault(સાચું);
        // Propagate the outer 'data_type: value' entries into the nested aggregate
        માટે (અચલ auto& entry : dtypemap) nestedDefaultp->addNext(entry.second->clone_વૃક્ષ(ખોટું));
        AstPattern* અચલ recursivePatternp = નવું AstPattern{defaultp->fileline(), nestedDefaultp};
        પાછા નવું AstPatMember{defaultp->fileline(), recursivePatternp, nullptr, nullptr};
    }

    // Return the value of a pattern default for elements of the given type, or nullptr if no
    // default. For aggregate elements, it applies to their elements (IEEE 1800-2023 10.9.1).
    Ast_ગાંઠ_પદાવલિ* patternDefaultValuep(AstPatMember* defaultp, Ast_ગાંઠ_DType* elemDTypep) {
        જો (!defaultp) પાછા nullptr;
        AstPatMember* અચલ elemDefaultp = defaultPatp_forDType(defaultp, elemDTypep);
        elemDefaultp->dtypep(elemDTypep);
        Ast_ગાંઠ_પદાવલિ* અચલ valuep = pattern_Member_કિંમત_Iterate(elemDefaultp);
        VL_DO_DANGLING(ધકેલો_Deletep(elemDefaultp), elemDefaultp);
        પાછા valuep;
    }
    // An unpacked array concatenation, with each item an element, or an array of the elements
    // spliced in (IEEE 1800-2023 10.10)
    શૂન્ય patternConcatArray(AstPattern* nodep, AstUnpackArrayDType* arrayDtp) {
        અચલ VNumRange range = arrayDtp->declRange();
        અચલ પૂર્ણાંક elements = range.elements();
        AstConsArray* અચલ newp = નવું AstConsArray{nodep->fileline(), arrayDtp};
        પૂર્ણાંક entn = 0;  // Number of elements given so far, from the left
        તાર્કિક error = ખોટું;  // Reported an error, so the value is not complete
        માટે (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;
             patp = VN_AS(patp->nextp(), PatMember)) {
            patp->dtypep(arrayDtp->subDTypep());
            Ast_ગાંઠ_પદાવલિ* અચલ valuep = pattern_Member_કિંમત_Iterate(patp);
            અચલ uint64_t index = range.ascending() ? entn : elements - 1 - entn;
            અચલ AstUnpackArrayDType* અચલ spliceDtp
                = VN_CAST(valuep->dtypep()->skipRefp(), UnpackArrayDType);
            જો (spliceDtp && spliceDtp->subDTypep()->similarDType(arrayDtp->subDTypep())) {
                entn += spliceDtp->elementsConst();
                newp->splice_કિંમતો(index, valuep);
                ચાલુ;
            }
            અચલ VCastable castable = Ast_ગાંઠ::computeCastable(
                arrayDtp->subDTypep()->skipRefp(), valuep->dtypep()->skipRefp(), valuep);
            જો (castable != VCastable::SAMEISH && castable != VCastable::COMPATIBLE) {
                valuep->v3error("Unpacked array concatenation item is neither an element nor"
                                " an array of elements of its target (IEEE 1800-2023 10.10)"
                                << warnDTypes(valuep, arrayDtp->subDTypep(), valuep->dtypep(),
                                              "Element", "Item"));
                error = સાચું;
            }
            newp->valuep(index, valuep);
            ++entn;
        }
        જો (!error && entn != elements) {
            nodep->v3error("Unpacked array concatenation has "
                           << entn << " elements, but its target has " << elements
                           << " (IEEE 1800-2023 10.10)");
            error = સાચું;
        }
        જો (error) {
            VL_DO_DANGLING(ધકેલો_Deletep(newp), newp);
            nodep->replaceWith(નવું AstConst{nodep->fileline(), AstConst::BitFalseErroring{}});
        } નહીંતર {
            nodep->replaceWith(newp);
        }
    }
    શૂન્ય patternArray(AstPattern* nodep, Ast_ગાંઠ_Array_DType* arrayDtp, AstPatMember* defaultp) {
        જો (nodep->itemsp() && VN_AS(nodep->itemsp(), PatMember)->isConcat()) {
            patternConcatArray(nodep, VN_AS(arrayDtp, UnpackArrayDType));
            પાછા;
        }
        અચલ VNumRange range = arrayDtp->declRange();
        PatVecMap patmap = patVectorMap(nodep, range);
        UINFO(9, "ent " << range.ડાબું() << " to " << range.જમણું());
        Ast_ગાંઠ* newp = nullptr;
        તાર્કિક missed = ખોટું;  // Some element missed, so not a complete value
        માટે (પૂર્ણાંક entn = 0, ent = range.ડાબું(); entn < range.elements();
             ++entn, ent += range.ડાબું_To_જમણું_Inc()) {
            Ast_ગાંઠ_પદાવલિ* valuep = nullptr;
            અચલ auto it = patmap.find(ent);
            જો (it == patmap.end()) {
                જો (defaultp) {
                    valuep = patternDefaultValuep(defaultp, arrayDtp->subDTypep());
                } નહીંતર {
                    nodep->v3error("Assignment pattern missed initializing elements: " << ent);
                    missed = સાચું;
                }
            } નહીંતર {
                AstPatMember* અચલ patp = it->second;
                patmap.erase(it);
                patp->dtypep(arrayDtp->subDTypep());
                valuep = pattern_Member_કિંમત_Iterate(patp);
                // Only an unpacked array concatenation splices in arrays
                અચલ AstUnpackArrayDType* અચલ memberDtp
                    = VN_CAST(valuep->dtypep()->skipRefp(), UnpackArrayDType);
                જો (memberDtp && memberDtp->subDTypep()->similarDType(arrayDtp->subDTypep())) {
                    valuep->v3error("Assignment pattern member is an array of the elements, not"
                                    " an element (IEEE 1800-2023 10.9.1)"
                                    << warnDTypes(valuep, arrayDtp->subDTypep(), memberDtp,
                                                  "Element", "Pattern member")
                                    << "\n"
                                    << valuep->warnMore()
                                    << "... Suggest use an unpacked array concatenation {...} to "
                                       "splice arrays");
                    VL_DO_DANGLING(ધકેલો_Deletep(valuep), valuep);
                    valuep = નવું AstConst{patp->fileline(), AstConst::BitFalseErroring{}};
                }
            }

            જો (valuep) {
                જો (VN_IS(arrayDtp, UnpackArrayDType)) {
                    જો (!newp) newp = નવું AstConsArray{nodep->fileline(), arrayDtp};
                    VN_AS(newp, ConsArray)->valuep(ent - range.lo(), valuep);
                } નહીંતર {  // Packed. Convert to concat for now.
                    જો (!newp) {
                        newp = valuep;
                    } નહીંતર {
                        AstConcat* અચલ concatp
                            = નવું AstConcat{valuep->fileline(), VN_AS(newp, ગાંઠ_પદાવલિ), valuep};
                        newp = concatp;
                        newp->dtypeChgSigned(nodep->dtypep()->numeric());
                    }
                }
            }
        }
        જો (!patmap.empty()) nodep->v3error("Assignment pattern with too many elements");
        જો (missed && VN_IS(newp, ConsArray)) {
            VL_DO_DANGLING(ધકેલો_Deletep(newp), newp);
            newp = નવું AstConst{nodep->fileline(), AstConst::BitFalseErroring{}};
        }
        જો (newp) {
            nodep->replaceWith(newp);
        } નહીંતર {
            nodep->v3error("Assignment pattern with no members");
        }
        // UINFOTREE(9, newp, "", "apat-out");
    }
    // Return the key of an array pattern member, which must be a constant expression,
    // or nullptr after reporting an error
    AstConst* patternConstKeyp(AstPatMember* patp) {
        જો (VN_IS(patp->keyp(), ગાંઠ_DType)) {
            patp->keyp()->v3warn(E_UNSUPPORTED,
                                 "Unsupported: Data-type key in array assignment pattern");
            પાછા nullptr;
        }
        જો (patp->ચાવી_Exprp()) V3Const::constifyParamsNoWarnEdit(patp->ચાવી_Exprp());  // May change
        AstConst* અચલ constp = VN_CAST(patp->ચાવી_Exprp(), Const);
        જો (!constp) {
            patp->keyp()->v3error(
                "Assignment pattern key is not a constant expression (IEEE 1800-2023 10.9)");
        }
        પાછા constp;
    }
    // Return the key of a pattern member of a non-aggregate key type, converted to the key type,
    // unlinked, or nullptr after reporting an error
    AstConst* patternScalarKeyp(AstPatMember* patp, Ast_ગાંઠ_DType* ચાવી_DTypep) {
        જો (!patp->keyp()) {
            patp->v3error("Missing pattern key (need an expression then a ':')");
            પાછા nullptr;
        }
        // An integral key is evaluated in the context of the key type, as an index converted to
        // it (IEEE 1800-2023 7.8.4), but not truncated before checking it for X or Z
        જો (ચાવી_DTypep->skipRefp()->isIntegralOrPacked() && patp->ચાવી_Exprp()) {
            userIterate(patp->ચાવી_Exprp(), WidthVP{ચાવી_DTypep, PRELIM}.p());
            અચલ પૂર્ણાંક width = std::max(patp->ચાવી_Exprp()->width(), ચાવી_DTypep->width());
            Ast_ગાંઠ_DType* અચલ contextDTypep
                = ચાવી_DTypep->findBitOrLogicDType(width, width, ચાવી_DTypep->numeric(), સાચું);
            iterateCheck(patp, "key", patp->ચાવી_Exprp(), CONTEXT_DET, FINAL, contextDTypep,
                         EXTEND_EXP, ખોટું);
        }
        AstConst* અચલ constp = patternConstKeyp(patp);
        જો (!constp) પાછા nullptr;
        જો (constp->અંક().isFourState()) {
            patp->keyp()->v3error("Assignment pattern key cannot contain X or Z");
            પાછા nullptr;
        }
        // Convert to the key type. The conversion is inserted above 'constp', so take the key
        // from the member again rather than from what iterateCheck returns.
        iterateCheck(patp, "key", constp, CONTEXT_DET, FINAL, ચાવી_DTypep, EXTEND_EXP, ખોટું);
        Ast_ગાંઠ_પદાવલિ* અચલ convp = patp->ચાવી_Exprp();
        // Fold the conversion, so the key is a Const again
        Ast_ગાંઠ_પદાવલિ* અચલ keyp = V3Const::constifyEdit(convp);
        UASSERT_OBJ(VN_IS(keyp, Const), patp, "Key not constant after conversion");
        પાછા VN_AS(keyp->unlinkFrBack(), Const);
    }
    // Return the key of a keyed pattern member, for the kind of constructor, unlinked, or nullptr
    // after reporting an error. Only associative arrays can have aggregate keys.
    Ast_ગાંઠ_પદાવલિ* patternKeyp(AstConsAssoc*, AstPatMember* patp, Ast_ગાંઠ_DType* arrayDtp) {
        Ast_ગાંઠ_DType* અચલ ચાવી_DTypep = VN_AS(arrayDtp, AssocArrayDType)->ચાવી_DTypep();
        જો (!ચાવી_DTypep->skipRefp()->isAggregateType() || !patp->ચાવી_Exprp()) {
            પાછા patternScalarKeyp(patp, ચાવી_DTypep);
        }
        // An aggregate key is folded, and constant if made of constant values only
        iterateCheckTyped(patp, "key", patp->ચાવી_Exprp(), ચાવી_DTypep, BOTH);
        V3Const::constifyParamsNoWarnEdit(patp->ચાવી_Exprp());
        Ast_ગાંઠ_પદાવલિ* અચલ foldedp = patp->ચાવી_Exprp();
        જો (!foldedp->is_Constant_વૃક્ષ()) {
            foldedp->v3error(
                "Assignment pattern key is not a constant expression (IEEE 1800-2023 10.9)");
            પાછા nullptr;
        }
        જો (foldedp->exists([](અચલ AstConst* constp) { પાછા constp->અંક().isFourState(); })) {
            foldedp->v3error("Assignment pattern key cannot contain X or Z");
            પાછા nullptr;
        }
        પાછા foldedp->unlinkFrBack();
    }
    AstConst* patternKeyp(AstConsWildcard*, AstPatMember* patp, Ast_ગાંઠ_DType* arrayDtp) {
        પાછા patternScalarKeyp(patp, arrayDtp->findStringDType());
    }
    AstConst* patternKeyp(AstConsDynArray*, AstPatMember* patp, Ast_ગાંઠ_DType* arrayDtp) {
        // Converted to longint, not int, so a key wider than 32 bits is out of bounds rather than
        // truncated to a valid index
        પાછા patternScalarKeyp(patp, arrayDtp->findBasicDType(VBasicDTypeKwd::LONGINT));
    }
    // Sort constructor items by key, so equal values have equal trees, and report duplicate
    // keys (IEEE 1800-2023 10.9.1). Keys are compared after conversion to the key type, e.g.
    // the wildcard keys "CCC" and 23'h434343 are equal.
    template <typename T_Item, typename T_Cons>
    શૂન્ય sort_Cons_વસ્તુઓ(T_Cons* nodep) {
        std::vector<T_Item*> itemps;
        જ્યારે (T_Item* અચલ itemp = nodep->itemsp()) {
            itemp->unlinkFrBack();
            itemps.ધકેલો_back(itemp);
        }
        અચલ auto ચાવી_Of = [](અચલ T_Item* itemp) -> અચલ V3_સંખ્યા& {
            અચલ Ast_ગાંઠ* અચલ keyp = itemp->keyp();
            પાછા VN_AS(keyp, Const)->અંક();
        };
        std::stable_sort(itemps.begin(), itemps.end(), [&](અચલ T_Item* ap, અચલ T_Item* bp) {
            // Any total order will do, it need not match the runtime order of the keys
            પાછા ચાવી_Of(ap).isLtXZ(ચાવી_Of(bp));
        });
        અચલ T_Item* firstp = nullptr;  // First item with the current key
        માટે (T_Item* અચલ itemp : itemps) {
            જો (firstp && ચાવી_Of(firstp).isCaseEq(ચાવી_Of(itemp))) {
                // Sort is stable, so 'firstp' is the first one in the source
                itemp->keyp()->v3error("Assignment pattern key used multiple times"
                                       " (IEEE 1800-2023 10.9.1)\n"
                                       << itemp->keyp()->warnContextPrimary() << '\n'
                                       << firstp->keyp()->warnOther()
                                       << "... Location of first use\n"
                                       << firstp->keyp()->warnContextSecondary());
            } નહીંતર {
                firstp = itemp;
            }
            nodep->addItemsp(itemp);
        }
    }
    // Convert a keyed pattern into the constructor 'consp' of an associative, wildcard or
    // dynamic array
    template <typename T_Item, typename T_Cons>
    શૂન્ય patternKeyed(AstPattern* nodep, T_Cons* consp, Ast_ગાંઠ_DType* arrayDtp,
                      AstPatMember* defaultp) {
        consp->dtypeFrom(arrayDtp);
        consp->defaultp(patternDefaultValuep(defaultp, arrayDtp->subDTypep()));
        માટે (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;
             patp = VN_AS(patp->nextp(), PatMember)) {
            અચલ auto keyp = patternKeyp(consp, patp, arrayDtp);
            patp->dtypep(arrayDtp->subDTypep());
            Ast_ગાંઠ_પદાવલિ* અચલ valuep = pattern_Member_કિંમત_Iterate(patp);
            જો (keyp) {
                consp->addItemsp(નવું T_Item{patp->fileline(), keyp, valuep});
            } નહીંતર {
                VL_DO_DANGLING(ધકેલો_Deletep(valuep), valuep);
            }
        }
        // Aggregate keys are not sorted or checked for duplicates
        અચલ AstAssocArrayDType* અચલ assocDtp = VN_CAST(arrayDtp, AssocArrayDType);
        જો (!assocDtp || !assocDtp->ચાવી_DTypep()->skipRefp()->isAggregateType()) {
            sort_Cons_વસ્તુઓ<T_Item>(consp);
        }
        nodep->replaceWith(consp);
        // UINFOTREE(9, consp, "", "apat-out");
    }
    Ast_ગાંઠ_પદાવલિ* newConsDynArrayOrQueue(File_લીટી* fileline, અચલ Ast_ગાંઠ_DType* dtypep) {
        જો (VN_IS(dtypep, DynArrayDType)) પાછા નવું AstConsDynArray{fileline};
        જો (VN_IS(dtypep, QueueDType)) પાછા નવું AstConsQueue{fileline};
        પાછા nullptr;
    }
    // Add 'valuep' to a dynamic array or queue constructor: as one element, or as an array whose
    // elements are spliced in. A constructor being spliced (e.g. a concatenation's left operand)
    // has its items moved over, or added one by one to convert them if its kind or element type
    // differs.
    template <typename T_Item, typename T_Cons>
    શૂન્ય addConsItem(T_Cons* consp, Ast_ગાંઠ_પદાવલિ* valuep) {
        Ast_ગાંઠ_DType* અચલ arrayDtp = consp->dtypep()->skipRefp();
        // An element if assignable to the element type, else an array (IEEE 1800-2023 10.10)
        અચલ તાર્કિક isElement
            = Ast_ગાંઠ::computeCastable(arrayDtp->subDTypep(), valuep->dtypep(), nullptr)
                  .isAssignable();
        // A bounded queue to splice is constructed first, to truncate it
        અચલ AstQueueDType* અચલ subQueueDtp = VN_CAST(valuep->dtypep()->skipRefp(), QueueDType);
        જો (!isElement && !(subQueueDtp && subQueueDtp->boundp())) {
            T_Cons* અચલ subp = Ast_ગાંઠ::cast<T_Cons>(valuep);
            જો (subp && subp->dtypep()->skipRefp()->subDTypep() == arrayDtp->subDTypep()) {
                જો (subp->itemsp()) consp->addItemsp(subp->itemsp()->unlinkFrBackWithNext());
                VL_DO_DANGLING(ધકેલો_Deletep(subp), subp);
                પાછા;
            }
            જો (AstConsQueue* અચલ queuep = VN_CAST(valuep, ConsQueue)) {
                add_Cons_વસ્તુઓ(consp, queuep);
                પાછા;
            }
            જો (AstConsDynArray* અચલ dynp = VN_CAST(valuep, ConsDynArray)) {
                add_Cons_વસ્તુઓ(consp, dynp);
                પાછા;
            }
        }
        T_Item* અચલ itemp = નવું T_Item{valuep->fileline(), valuep};
        consp->addItemsp(itemp);
        // Width the value now, as constructors are widthed when created
        જો (isElement) {
            iterateCheckTyped(itemp, "value", valuep, arrayDtp->subDTypep(), FINAL);
        } નહીંતર {
            userIterateAndNext(valuep, WidthVP{arrayDtp, FINAL}.p());
        }
    }
    શૂન્ય addConsItem(Ast_ગાંઠ_પદાવલિ* consp, Ast_ગાંઠ_પદાવલિ* valuep) {
        જો (AstConsDynArray* અચલ dynp = VN_CAST(consp, ConsDynArray)) {
            addConsItem<AstConsDynArrayItem>(dynp, valuep);
        } નહીંતર {
            addConsItem<AstConsQueueItem>(VN_AS(consp, ConsQueue), valuep);
        }
    }
    template <typename T_Sub>
    શૂન્ય add_Cons_વસ્તુઓ(Ast_ગાંઠ_પદાવલિ* consp, T_Sub* subp) {
        જ્યારે (auto* અચલ itemp = subp->itemsp()) {
            Ast_ગાંઠ_પદાવલિ* અચલ valuep = itemp->valuep()->unlinkFrBack();
            VL_DO_DANGLING(ધકેલો_Deletep(itemp->unlinkFrBack()), itemp);
            addConsItem(consp, valuep);
        }
        VL_DO_DANGLING(ધકેલો_Deletep(subp), subp);
    }
    શૂન્ય patternDynArrayOrQueue(AstPattern* nodep, Ast_ગાંઠ_DType* arrayp, AstPatMember* defaultp) {
        Ast_ગાંઠ_પદાવલિ* અચલ newp = newConsDynArrayOrQueue(nodep->fileline(), arrayp);
        UASSERT_OBJ(newp, nodep, "Expected dynamic array or queue data type");
        newp->dtypeFrom(arrayp);
        // By the syntax, members are all keyed or all positional
        અચલ AstPatMember* અચલ firstp = VN_AS(nodep->itemsp(), PatMember);
        અચલ તાર્કિક keyed = defaultp || (firstp && firstp->keyp());
        જો (keyed && VN_IS(arrayp, DynArrayDType) && VN_IS(nodep->backp(), NewDynamic)) {
            // The initializer of new[], visit(AstNewDynamic) then adds the size
            patternKeyed<AstConsDynArrayItem>(nodep, VN_AS(newp, ConsDynArray), arrayp, defaultp);
            પાછા;
        }
        અચલ અક્ષર* અચલ kindp = VN_IS(arrayp, DynArrayDType) ? "dynamic array" : "queue";
        જો (defaultp) defaultp->v3error("Assignment pattern default cannot be used for " << kindp);
        માટે (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;
             patp = VN_AS(patp->nextp(), PatMember)) {
            જો (patp->keyp()) {
                patp->keyp()->v3error("Assignment pattern key cannot be used for " << kindp);
            }
            patp->dtypep(arrayp->subDTypep());
            addConsItem(newp, pattern_Member_કિંમત_Iterate(patp));
        }
        nodep->replaceWith(newp);
        // UINFOTREE(9, newp, "", "apat-out");
    }
    શૂન્ય patternBasic(AstPattern* nodep, Ast_ગાંઠ_DType* vdtypep, AstPatMember* defaultp) {
        અચલ AstBasicDType* bdtypep = VN_AS(vdtypep, BasicDType);
        અચલ VNumRange range = bdtypep->declRange();
        PatVecMap patmap = patVectorMap(nodep, range);
        UINFO(9, "ent " << range.hi() << " to " << range.lo());
        Ast_ગાંઠ_પદાવલિ* newp = nullptr;
        માટે (અચલ પૂર્ણાંક ent : range.seqHiToLo()) {
            Ast_ગાંઠ_પદાવલિ* valuep = nullptr;
            અચલ auto it = patmap.find(ent);
            જો (it == patmap.end()) {
                જો (defaultp) {
                    valuep = patternDefaultValuep(defaultp, nodep->findBitDType());
                } નહીંતર {
                    nodep->v3error("Assignment pattern missed initializing elements: " << ent);
                }
            } નહીંતર {
                AstPatMember* અચલ patp = it->second;
                patmap.erase(it);
                patp->dtypep(nodep->findBitDType());
                valuep = pattern_Member_કિંમત_Iterate(patp);
            }
            જો (valuep) {  // Packed. Convert to concat for now.
                જો (!newp) {
                    newp = valuep;
                } નહીંતર {
                    AstConcat* અચલ concatp = નવું AstConcat{valuep->fileline(), newp, valuep};
                    newp = concatp;
                    newp->dtypeChgSigned(nodep->dtypep()->numeric());
                }
            }
        }
        જો (!patmap.empty()) nodep->v3error("Assignment pattern with too many elements");
        જો (newp) {
            nodep->replaceWith(newp);
        } નહીંતર {
            nodep->v3error("Assignment pattern with no members");
        }
        // UINFOTREE(9, newp, "", "apat-out");
    }
    Ast_ગાંઠ_પદાવલિ* pattern_Member_કિંમત_Iterate(AstPatMember* patp) {
        // Determine values - might be another ConsArray
        userIterate(patp, WidthVP{patp->dtypep(), BOTH}.p());
        // Convert to ConsArray or constify immediately
        Ast_ગાંઠ_પદાવલિ* valuep = patp->lhssp()->unlinkFrBack();
        જો (VN_IS(valuep, Const)) {
            // Forming a AstConcat will cause problems with
            // unsized (uncommitted sized) constants
            જો (AstConst* અચલ newp = V3WidthCommit::newIfConstCommitSize(VN_AS(valuep, Const))) {
                VL_DO_DANGLING(ધકેલો_Deletep(valuep), valuep);
                valuep = newp;
            }
        }
        પાછા valuep;
    }

    સ્થિર શૂન્ય checkEventAssignment(અચલ Ast_ગાંઠ_Assign* અચલ asgnp) {
        string unsupEvtAsgn;
        જો (!usesDynamicScheduler(asgnp->lhsp())) unsupEvtAsgn = "to";
        જો (VN_IS(asgnp->rhsp(), CReset)) પાછા;
        જો (asgnp->rhsp()->dtypep()->isEvent() && !usesDynamicScheduler(asgnp->rhsp())) {
            unsupEvtAsgn += (unsupEvtAsgn.empty() ? "from" : " and from");
        }
        જો (!unsupEvtAsgn.empty()) {
            asgnp->v3warn(E_UNSUPPORTED, "Assignment "
                                             << unsupEvtAsgn
                                             << " event in statically scheduled context.\n"
                                             << asgnp->warnMore()
                                             << "Static event "
                                                "scheduling won't be able to handle this.\n"
                                             << asgnp->warnMore()
                                             << "... Suggest move the event into a "
                                                "completely dynamic context, eg. a class,  and "
                                                "reference it only from such context.");
        }
    }

    સ્થિર તાર્કિક usesDynamicScheduler(Ast_ગાંઠ* nodep) {
        UASSERT_OBJ(nodep->dtypep()->isEvent(), nodep, "Node does not have an event dtype");
        જ્યારે (સાચું) {
            AstVarRef* અચલ vrefp = VN_CAST(nodep, VarRef);
            જો (vrefp) પાછા usesDynamicScheduler(vrefp);
            જો (VN_IS(nodep, MemberSel)) {
                પાછા સાચું;
            } નહીંતર જો (Ast_ગાંઠ_Sel* selp = VN_CAST(nodep, ગાંઠ_Sel)) {
                nodep = selp->fromp();
            } નહીંતર {
                પાછા ખોટું;
            }
        }
    }

    સ્થિર તાર્કિક usesDynamicScheduler(AstVarRef* vrefp) {
        પાછા VN_IS(vrefp->classOrPackagep(), Class) || vrefp->varp()->isFuncLocal();
    }

    શૂન્ય visit(AstPatMember* nodep) override {
        Ast_ગાંઠ_DType* અચલ vdtypep = m_vup->dtypeNullp();
        UASSERT_OBJ(vdtypep, nodep, "Pattern member type not assigned by AstPattern visitor");
        nodep->dtypep(vdtypep);
        UINFO(9, "   PATMEMBER " << nodep);
        UASSERT_OBJ(!nodep->lhssp()->nextp(), nodep,
                    "PatMember value should be singular w/replicates removed");
        // Need to propagate assignment type downwards, even on prelim
        userIterateChildren(nodep, WidthVP{nodep->dtypep(), PRELIM}.p());
        iterateCheck(nodep, "Pattern value", nodep->lhssp(), ASSIGN, FINAL, vdtypep, EXTEND_LHS);
    }
    પૂર્ણાંક visitPatMemberRep(AstPatMember* nodep) {
        uint32_t times = 1;
        જો (nodep->repp()) {  // else repp()==nullptr shorthand for rep count 1
            iterateCheckSizedSelf(nodep, "LHS", nodep->repp(), SELF, BOTH);
            V3Const::constifyParamsEdit(nodep->repp());  // repp may change
            અચલ AstConst* અચલ constp = VN_CAST(nodep->repp(), Const);
            જો (!constp) {
                nodep->v3error("Replication value isn't a constant.");
                times = 0;
            } નહીંતર {
                times = constp->toUInt();
            }
            જો (times == 0) {
                nodep->v3error("Pattern replication value of 0 is not legal.");
                times = 1;
            }
            nodep->repp()
                ->unlinkFrBackWithNext()
                ->delete_વૃક્ષ();  // Done with replicate before cloning
        }
        પાછા times;
    }

    શૂન્ય visit(AstPropSpec* nodep) override {
        VL_RESTORER(m_seqUnsupp);
        VL_RESTORER(m_hasSExpr);
        assert_At_પદાવલિ(nodep);
        જો (m_vup->prelim()) {  // First stage evaluation
            // Only bare strong until uses V3AssertPre's timing-based lowering. Embedded forms
            // are lowered without timing by V3AssertNfa.
            Ast_ગાંઠ* propp = nodep->propp();
            જ્યારે (AstLogNot* અચલ notp = VN_CAST(propp, LogNot)) propp = notp->lhsp();
            AstUntil* અચલ untilp = VN_CAST(propp, Until);
            જો (untilp && VN_IS(nodep->backp(), ગાંઠ_Cover_Or_Assert) && untilp->isStrong()
                && (v3Global.opt.timing().isSetFalse() || !v3Global.opt.timing().isSetTrue())) {
                untilp->v3warn(E_NOTIMING, untilp->verilogKwd() << " requires --timing");
                untilp->replaceWith(નવું AstConst{untilp->fileline(), AstConst::BitFalse{}});
                VL_DO_DANGLING(ધકેલો_Deletep(untilp), untilp);
            }
            iterateCheckBool(nodep, "Property", nodep->propp(), BOTH);
            userIterateAndNext(nodep->sensesp(), nullptr);
            જો (nodep->disablep()) {
                iterateCheckBool(nodep, "Disable", nodep->disablep(),
                                 BOTH);  // it's like an if() condition.
            }
            nodep->dtypeSetBit();
        }
    }

    તાર્કિક firstNewStatementOkRecurse(Ast_ગાંઠ* nodep) {
        જો (AstVar* અચલ varp = VN_CAST(nodep, Var)) {
            જો (!varp->valuep() || VN_CAST(varp->valuep(), Const) || varp->isIO()) પાછા સાચું;
        }
        જો (AstAssign* અચલ aitemp = VN_CAST(nodep, Assign)) {
            જો (VN_IS(aitemp->rhsp(), Const) || VN_IS(aitemp->rhsp(), CReset)) પાછા સાચું;
        }
        જો (AstInitialStaticStmt* અચલ aitemp = VN_CAST(nodep, InitialStaticStmt)) {
            માટે (Ast_ગાંઠ* stmtp = aitemp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                જો (!firstNewStatementOkRecurse(stmtp)) પાછા ખોટું;
            }
            પાછા સાચું;
        }
        જો (AstInitialAutomaticStmt* અચલ aitemp = VN_CAST(nodep, InitialAutomaticStmt)) {
            માટે (Ast_ગાંઠ* stmtp = aitemp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                જો (!firstNewStatementOkRecurse(stmtp)) પાછા ખોટું;
            }
            પાછા સાચું;
        }
        પાછા ખોટું;
    }

    //--------------------
    // Top levels

    template <typename CaseItem>
    શૂન્ય handleCaseType(Ast_ગાંઠ* casep, Ast_ગાંઠ_પદાવલિ* exprp, CaseItem* itemsp) {
        AstAttrOf* અચલ exprap = VN_CAST(exprp, AttrOf);
        જો (!exprap) પાછા;
        જો (exprap->attrType() != VAttrType::TYPEID) પાછા;

        અચલ Ast_ગાંઠ_DType* અચલ પદાવલિ_Dtp = exprap->dtypep();
        UINFO(9, "case type exprDtp " << પદાવલિ_Dtp);
        // V3Param may have a pointer to this case statement, and we need
        // dotted references to remain properly named, so rather than
        // removing we convert it to a "normal" expression "case (1) ..."
        File_લીટી* અચલ newfl = casep->fileline();
        newfl->warnOff(V3ErrorCode::CASEINCOMPLETE, સાચું);  // Side effect of transform
        newfl->warnOff(V3ErrorCode::CASEOVERLAP, સાચું);  // Side effect of transform
        casep->fileline(newfl);
        માટે (CaseItem* itemp = itemsp; itemp; itemp = Ast_ગાંઠ::as<CaseItem>(itemp->nextp())) {
            જો (itemp->isDefault()) ચાલુ;
            તાર્કિક hit = ખોટું;
            માટે (Ast_ગાંઠ* condp = itemp->condsp(); condp; condp = condp->nextp()) {
                અચલ AstAttrOf* અચલ condAttrp = VN_CAST(condp, AttrOf);
                જો (!condAttrp) {
                    condp->v3error("Case(type) statement requires items that have type() items");
                } નહીંતર {
                    Ast_ગાંઠ_DType* અચલ condDtp = condAttrp->dtypep();
                    જો (Ast_ગાંઠ::computeCastable(પદાવલિ_Dtp, condDtp, casep) == VCastable::SAMEISH) {
                        hit = સાચું;
                        તોડો;
                    }
                }
            }
            ધકેલો_Deletep(itemp->condsp()->unlinkFrBackWithNext());
            // Item condition becomes constant 1 if hits else 0
            itemp->addCondsp(નવું AstConst{newfl, AstConst::BitTrue{}, hit});
        }
        exprap->replaceWith(નવું AstConst{newfl, AstConst::BitTrue{}});
        VL_DO_DANGLING(ધકેલો_Deletep(exprap), exprap);
    }

    template <typename CaseItem>
    શૂન્ય handleCase(Ast_ગાંઠ* casep, Ast_ગાંઠ_પદાવલિ* exprp, CaseItem* itemsp) {
        // IEEE-2012 12.5:
        //    Width: MAX(expr, all items)
        //    Signed: Only if expr, and all items signed
        // Take width as maximum across all items, if any is real whole thing is real
        Ast_ગાંઠ_DType* subDTypep = exprp->dtypep();
        માટે (CaseItem* itemp = itemsp; itemp; itemp = Ast_ગાંઠ::as<CaseItem>(itemp->nextp())) {
            માટે (Ast_ગાંઠ* condp = itemp->condsp(); condp; condp = condp->nextp()) {
                જો (condp->dtypep() == subDTypep) ચાલુ;

                જો (condp->dtypep()->isDouble() || subDTypep->isDouble()) {
                    subDTypep = casep->findDoubleDType();
                } નહીંતર જો (condp->dtypep()->isString() || subDTypep->isString()) {
                    subDTypep = casep->findStringDType();
                } નહીંતર {
                    અચલ પૂર્ણાંક width = std::max(subDTypep->width(), condp->width());
                    અચલ પૂર્ણાંક mwidth = std::max(subDTypep->widthMin(), condp->widthMin());
                    અચલ તાર્કિક issigned = subDTypep->isSigned() && condp->isSigned();
                    subDTypep = casep->findBitOrLogicDType(
                        width, mwidth, VSigning::fromBool(issigned),
                        subDTypep->isFourstate() || condp->dtypep()->isFourstate());
                }
            }
        }

        // Apply width
        iterateCheck(casep, "Case expression", exprp, CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP);
        માટે (CaseItem* itemp = itemsp; itemp; itemp = Ast_ગાંઠ::as<CaseItem>(itemp->nextp())) {
            માટે (Ast_ગાંઠ *nextcp, *condp = itemp->condsp(); condp; condp = nextcp) {
                nextcp = condp->nextp();  // Final may cause the node to get replaced
                iterateCheck(casep, "Case Item", condp, CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP);
            }
        }
    }

    શૂન્ય visit(AstGenCase* nodep) override {
        assertAtStatement(nodep);
        // Type check expression and case item conditions, but not bodies
        userIterateAndNext(nodep->exprp(), WidthVP{CONTEXT_DET, PRELIM}.p());
        માટે (AstGenCaseItem *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            // Prelim may cause the node to get replaced, pick up next up front
            nextip = VN_AS(itemp->nextp(), GenCaseItem);
            માટે (Ast_ગાંઠ *nextcp, *condp = itemp->condsp(); condp; condp = nextcp) {
                // Prelim may cause the node to get replaced, pick up next up front
                nextcp = condp->nextp();
                VL_DO_DANGLING(userIterate(condp, WidthVP{CONTEXT_DET, PRELIM}.p()), condp);
            }
        }
        // Deal with case(type(data_type))
        handleCaseType(nodep, nodep->exprp(), nodep->itemsp());
        // Type check
        handleCase(nodep, nodep->exprp(), nodep->itemsp());
    }
    શૂન્ય visit(AstGenFor* nodep) override {
        assertAtStatement(nodep);
        userIterateAndNext(nodep->initsp(), nullptr);
        iterateCheckBool(nodep, "For Test Condition", nodep->condp(), BOTH);
        userIterateAndNext(nodep->incsp(), nullptr);
    }
    શૂન્ય visit(AstGenIf* nodep) override {
        assertAtStatement(nodep);
        iterateCheckBool(nodep, "If", nodep->condp(), BOTH);
    }

    શૂન્ય visit(AstCase* nodep) override {
        assertAtStatement(nodep);
        // Type check expression case item conditions and bodies
        userIterateAndNext(nodep->exprp(), WidthVP{CONTEXT_DET, PRELIM}.p());
        માટે (AstCaseItem *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            nextip = VN_AS(itemp->nextp(), CaseItem);  // Prelim may cause the node to get replaced
            userIterateAndNext(itemp->stmtsp(), nullptr);
            માટે (Ast_ગાંઠ *nextcp, *condp = itemp->condsp(); condp; condp = nextcp) {
                nextcp = condp->nextp();  // Prelim may cause the node to get replaced
                VL_DO_DANGLING(userIterate(condp, WidthVP{CONTEXT_DET, PRELIM}.p()), condp);
            }
        }
        // Deal with case(type(data_type))
        handleCaseType(nodep, nodep->exprp(), nodep->itemsp());
        // Type check
        handleCase(nodep, nodep->exprp(), nodep->itemsp());
    }
    શૂન્ય visit(AstRandCase* nodep) override {
        // IEEE says each item is a int (32-bits), and sizes are based on natural sizing,
        // but we'll sum to a 64-bit number then math is faster.
        assertAtStatement(nodep);
        Ast_ગાંઠ_DType* અચલ itemDTypep = nodep->findUInt32DType();
        માટે (AstCaseItem *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            nextip = VN_AS(itemp->nextp(), CaseItem);  // Prelim may cause the node to get replaced
            userIterateAndNext(itemp->stmtsp(), nullptr);
            માટે (Ast_ગાંઠ *nextcp, *condp = itemp->condsp(); condp; condp = nextcp) {
                nextcp = condp->nextp();  // Prelim may cause the node to get replaced
                iterateCheckTyped(itemp, "Randcase Item", condp, itemDTypep, BOTH);
                VL_DANGLING(condp);  // Might have been replaced
            }
            VL_DANGLING(itemp);  // Might have been replaced
        }
    }

    શૂન્ય visit(AstLoop* nodep) override {
        UASSERT_OBJ(!nodep->contsp(), nodep, "'contsp' only used before LinkJump");
        assertAtStatement(nodep);
        userIterateAndNext(nodep->stmtsp(), nullptr);
    }
    શૂન્ય visit(AstLoopTest* nodep) override {
        assertAtStatement(nodep);
        iterateCheckBool(nodep, "Loop Condition", nodep->condp(), BOTH);
    }
    શૂન્ય visit(AstRepeat* nodep) override { nodep->v3fatalSrc("'repeat' missed in LinkJump"); }
    શૂન્ય visit(Ast_ગાંઠ_If* nodep) override {
        assertAtStatement(nodep);
        // UINFOTREE(1, nodep, "", "IfPre");
        userIterateAndNext(nodep->thensp(), nullptr);
        userIterateAndNext(nodep->elsesp(), nullptr);
        iterateCheckBool(nodep, "If", nodep->condp(), BOTH);  // it's like an if() condition.
        // UINFOTREE(1, nodep, "", "IfOut");
    }
    શૂન્ય visit(Ast_પદાવલિ_Stmt* nodep) override {
        userIterateAndNext(nodep->stmtsp(), nullptr);
        // expected result is same as parent's expected result
        userIterateAndNext(nodep->resultp(), m_vup);
        nodep->dtypeFrom(nodep->resultp());
    }
    શૂન્ય visit(Ast_ગાંઠ_Foreach* nodep) override {
        જો (nodep->didWidth()) પાછા;
        nodep->didWidth(સાચું);
        અચલ AstForeachHeader* અચલ headerp = nodep->headerp();
        // UINFOTREE(1, nodep, "", "foreach-old");
        userIterateAndNext(headerp->fromp(), WidthVP{SELF, BOTH}.p());
        Ast_ગાંઠ_પદાવલિ* અચલ fromp = headerp->fromp();
        UASSERT_OBJ(fromp->dtypep(), fromp, "Missing data type");
        Ast_ગાંઠ_DType* fromDtp = fromp->dtypep()->skipRefp();
        // Major dimension first
        માટે (Ast_ગાંઠ *argsp = headerp->elementsp(), *next_argsp; argsp; argsp = next_argsp) {
            next_argsp = argsp->nextp();
            અચલ તાર્કિક empty = VN_IS(argsp, Empty);
            AstVar* અચલ varp = VN_CAST(argsp, Var);
            UASSERT_OBJ(varp || empty, argsp, "Missing foreach loop variable");
            જો (varp) varp->usedLoopIdx(સાચું);
            જો (!fromDtp) {
                argsp->v3error("foreach loop variables exceed number of indices of array");
                VL_DO_DANGLING(nodep->unlinkFrBack()->delete_વૃક્ષ(), nodep);
                પાછા;
            }
            fromDtp = fromDtp->skipRefp();
            UINFO(9, "- foreachArg " << argsp);
            UINFO(9, "-   from on  " << fromp);
            UINFO(9, "-   from dtp " << fromDtp);

            જો (VN_IS(fromDtp, ગાંઠ_Array_DType) || VN_IS(fromDtp, DynArrayDType)
                || VN_IS(fromDtp, QueueDType)) {
                // Nothing special here
            } નહીંતર જો (AstBasicDType* અચલ adtypep = VN_CAST(fromDtp, BasicDType)) {
                જો (!adtypep->isString() && !adtypep->isRanged()) {
                    argsp->v3error("Illegal 'foreach' loop on " << fromDtp->prettyTypeName()
                                                                << " data type");
                    VL_DO_DANGLING(nodep->unlinkFrBack()->delete_વૃક્ષ(), nodep);
                    પાછા;
                }
            } નહીંતર જો (અચલ AstAssocArrayDType* અચલ adtypep
                       = VN_CAST(fromDtp, AssocArrayDType)) {
                varp->dtypeFrom(adtypep->ચાવી_DTypep());
            } નહીંતર {
                argsp->v3error("Illegal 'foreach' loop on " << fromDtp->prettyTypeName()
                                                            << " data type");
                VL_DO_DANGLING(nodep->unlinkFrBack()->delete_વૃક્ષ(), nodep);
                પાછા;
            }
            fromDtp = fromDtp->subDTypep();
        }
        // The parser validates we don't have "foreach (array[,,,])"
        Ast_ગાંઠ* અચલ bodyp = nodep->bodyp();
        userIterateAndNext(bodyp, nullptr);
        જો (AstForeach* અચલ loopp = VN_CAST(nodep, Foreach)) {
            VL_DO_DANGLING2(V3Begin::convertToWhile(loopp), loopp, nodep);
            પાછા;
        }
    }

    શૂન્ય visit(Ast_ગાંઠ_Assign* nodep) override {
        // IEEE-2012 10.7, 11.8.2, 11.8.3, 11.5:  (Careful of 11.8.1 which is
        //                  only one step; final dtype depends on assign LHS.)
        //    Determine RHS type width and signing
        //    Propagate type down to *non-self-determined* operators
        //       Real propagates only across one operator if one side is real -
        //       handled in each visitor.
        //    Then LHS sign-extends only if *RHS* is signed
        assertAtStatement(nodep);
        {
            // UINFOTREE(1, nodep, "", "assin:");
            userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
            UASSERT_OBJ(nodep->lhsp()->dtypep(), nodep, "How can LHS be untyped?");
            UASSERT_OBJ(nodep->lhsp()->dtypep()->widthSized(), nodep, "How can LHS be unsized?");
            // An interface instance, or an array or slice of them, is not a variable
            જો (અચલ AstIfaceRefDType* અચલ irefp
                = VN_CAST(nodep->lhsp()->dtypep()->elemDTypep(), IfaceRefDType)) {
                જો (!irefp->isVirtual()) {
                    nodep->v3error(
                        "Illegal assignment to an interface instance (IEEE 1800-2023 10.4)");
                    VL_DO_DANGLING(ધકેલો_Deletep(nodep->unlinkFrBack()), nodep);
                    પાછા;
                }
            }
            nodep->dtypeFrom(nodep->lhsp());
            //
            // AstPattern needs to know the proposed data type of the lhs, so pass on the prelim
            // UINFOTREE(1, nodep, "", "assrhs");
            userIterateAndNext(nodep->rhsp(), WidthVP{nodep->dtypep(), PRELIM}.p());
            //
            // UINFOTREE(1, nodep, "", "assign");
            streamAssignLower(nodep);
            Ast_ગાંઠ_DType* અચલ lhsDTypep = nodep->lhsp()->dtypep();

            // IEEE 1800-2023 7.6: For unpacked arrays to be assignment compatible,
            // the element types shall be equivalent (IEEE 1800-2023 6.22.2).
            // Note: Streaming operators and string literals have implicit conversion rules.
            જો (nodep->rhsp()->dtypep()) {  // May be null on earlier errors
                અચલ Ast_ગાંઠ_DType* અચલ lhsDtp = lhsDTypep->skipRefp();
                અચલ Ast_ગાંઠ_DType* અચલ rhsDtp = nodep->rhsp()->dtypep()->skipRefp();
                // Only check if number of states match for unpacked array to unpacked array
                // assignments
                અચલ તાર્કિક lhsIsUnpackArray = lhsDtp->isNonPackedArray();
                અચલ તાર્કિક rhsIsUnpackArray = rhsDtp->isNonPackedArray();
                જો (lhsIsUnpackArray && rhsIsUnpackArray) {
                    જો (lhsDtp->isFourstate() != rhsDtp->isFourstate()) {
                        nodep->v3error("Assignment between 2-state and 4-state types requires "
                                       "equivalent element types (IEEE 1800-2023 6.22.2, 7.6)"
                                       << warnDTypes(nodep, lhsDtp, rhsDtp));
                    }
                }
                checkUnpackedArrayAssignmentCompatible<Ast_ગાંઠ_Var_Ref, Ast_ગાંઠ_Var_Ref>(
                    nodep, VN_CAST(nodep->lhsp(), ગાંઠ_Var_Ref), VN_CAST(nodep->rhsp(), ગાંઠ_Var_Ref));
                // Variable references were checked above, this catches e.g. method call results
                જો (lhsDtp->isIntegralOrPacked() && rhsDtp->isAggregateType()
                    && !VN_IS(nodep->rhsp(), ગાંઠ_Var_Ref)) {
                    nodep->v3error("Illegal assignment betwen unpacked aggregate and "
                                   "integral type (IEEE 1800-2023 7.6)"
                                   << warnDTypes(nodep, lhsDtp, rhsDtp));
                }
            }

            iterateCheckAssign(nodep, "Assign RHS", nodep->rhsp(), FINAL, lhsDTypep);
            // UINFOTREE(1, nodep, "", "AssignOut");
        }
        જો (VN_IS(nodep, AssignForce)) checkForceReleaseLhs(nodep, nodep->lhsp());
        જો (Ast_ગાંઠ* અચલ controlp = nodep->timingControlp()) {
            જો (!m_underFork && VN_IS(m_ftaskp, Func)) {
                controlp->v3error("Timing controls are not legal in functions. Suggest use a task "
                                  "(IEEE 1800-2023 13.4.4)");
                VL_DO_DANGLING(controlp->unlinkFrBackWithNext()->delete_વૃક્ષ(), controlp);
            } નહીંતર જો (nodep->fileline()->timingOn() && v3Global.opt.timing().isSetTrue()) {
                iterateNull(controlp);
            } નહીંતર {
                જો (nodep->fileline()->timingOn()) {
                    જો (v3Global.opt.timing().isSetFalse()) {
                        controlp->v3warn(ASSIGNDLY, "Ignoring timing control on this "
                                                    "assignment/primitive due to --no-timing");
                    } નહીંતર {
                        controlp->v3warn(E_NEEDTIMINGOPT,
                                         "Use --timing or --no-timing to specify how "
                                         "timing controls should be handled");
                    }
                }
                VL_DO_DANGLING(controlp->unlinkFrBackWithNext()->delete_વૃક્ષ(), controlp);
            }
        }
        જો (VN_IS(nodep->rhsp(), EmptyQueue)) {
            UINFO(9, "= {} -> .delete(): " << nodep);
            અચલ Ast_ગાંઠ_DType* અચલ lhsDtp = nodep->lhsp()->dtypep()->skipRefp();
            જો (!VN_IS(lhsDtp, QueueDType) && !VN_IS(lhsDtp, DynArrayDType)) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported/Illegal: empty queue ('{}') in this assign context");
                VL_DO_DANGLING(ધકેલો_Deletep(nodep->unlinkFrBack()), nodep);
                પાછા;
            }
            AstMethodCall* અચલ newp
                = નવું AstMethodCall{nodep->fileline(), nodep->lhsp()->unlinkFrBack(), "delete"};
            newp->dtypeSetVoid();
            nodep->replaceWith(newp->makeStmt());
            VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
            // Need to now convert it
            visit(newp);
            પાછા;
        }
        જો (અચલ AstNewDynamic* અચલ dynp = VN_CAST(nodep->rhsp(), NewDynamic)) {
            UINFO(9, "= new[] -> .resize(): " << nodep);
            AstCMethodHard* newp;
            જો (!dynp->rhsp()) {
                newp = નવું AstCMethodHard{nodep->fileline(), nodep->lhsp()->unlinkFrBack(),
                                          VCMethod::DYN_RENEW, dynp->sizep()->unlinkFrBack()};
            } નહીંતર {
                newp = નવું AstCMethodHard{nodep->fileline(), nodep->lhsp()->unlinkFrBack(),
                                          VCMethod::DYN_RENEW_COPY, dynp->sizep()->unlinkFrBack()};
                newp->addPinsp(dynp->rhsp()->unlinkFrBack());
            }
            newp->didWidth(સાચું);
            newp->protect(ખોટું);
            newp->dtypeSetVoid();
            nodep->replaceWith(newp->makeStmt());
            VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
            પાછા;
        }

        // Queue slice on LHS: q[a:b] = rhs  ->  q.sliceAssign(a, b, rhs)
        // The LHS was lowered from AstSelExtract to CMethodHard(DYN_SLICE)
        // by V3WidthSel; that returns a temporary copy so the assignment is
        // silently discarded.  Transform into a mutating sliceAssign call.
        જો (AstCMethodHard* અચલ slicep = VN_CAST(nodep->lhsp(), CMethodHard)) {
            VCMethod assignMethod;
            જો (slicep->method() == VCMethod::DYN_SLICE) {
                assignMethod = VCMethod::DYN_SLICE_ASSIGN;
            } નહીંતર જો (slicep->method() == VCMethod::DYN_SLICE_BACK_BACK) {
                assignMethod = VCMethod::DYN_SLICE_ASSIGN_BACK_BACK;
            } નહીંતર જો (slicep->method() == VCMethod::DYN_SLICE_FRONT_BACK) {
                assignMethod = VCMethod::DYN_SLICE_ASSIGN_FRONT_BACK;
            } નહીંતર {
                assignMethod = VCMethod::_ENUM_MAX;  // not a slice
            }
            જો (assignMethod != VCMethod::_ENUM_MAX) {
                UINFO(9, "LHS queue slice -> sliceAssign: " << nodep);
                Ast_ગાંઠ_પદાવલિ* અચલ fromp = slicep->fromp()->unlinkFrBack();
                // Collect existing slice index pins (lsb, msb)
                Ast_ગાંઠ_પદાવલિ* અચલ lsbp = slicep->pinsp()->unlinkFrBack();
                Ast_ગાંઠ_પદાવલિ* અચલ msbp = slicep->pinsp()->unlinkFrBack();
                Ast_ગાંઠ_પદાવલિ* અચલ rhsp = nodep->rhsp()->unlinkFrBack();
                AstCMethodHard* અચલ newp
                    = નવું AstCMethodHard{nodep->fileline(), fromp, assignMethod};
                newp->addPinsp(lsbp);
                newp->addPinsp(msbp);
                newp->addPinsp(rhsp);
                newp->didWidth(સાચું);
                newp->protect(ખોટું);
                newp->dtypeSetVoid();
                nodep->replaceWith(newp->makeStmt());
                VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
                પાછા;
            }
        }

        જો (nodep->hasDType() && nodep->dtypep()->isEvent()) {
            checkEventAssignment(nodep);
            v3Global.setAssignsEvents();
        }
    }

    શૂન્ય visit(AstRSRule* nodep) override {
        જો (nodep->weightp()) iterateCheckUInt32(nodep, "weight", nodep->weightp(), BOTH);
        userIterateAndNext(nodep->prodlistsp(), nullptr);
        userIterateAndNext(nodep->weightStmtsp(), nullptr);
    }

    શૂન્ય visit(AstRelease* nodep) override {
        userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
        UASSERT_OBJ(nodep->lhsp()->dtypep(), nodep, "How can LValue be untyped?");
        UASSERT_OBJ(nodep->lhsp()->dtypep()->widthSized(), nodep, "How can LValue be unsized?");
        checkForceReleaseLhs(nodep, nodep->lhsp());
    }
    શૂન્ય visit(AstDeassign* nodep) override {
        userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
        UASSERT_OBJ(nodep->lhsp()->dtypep(), nodep, "L-value is untyped");
        UASSERT_OBJ(nodep->lhsp()->dtypep()->widthSized(), nodep, "L-value width is unsized");
        checkForceReleaseLhs(nodep, nodep->lhsp());
    }

    સ્થિર તાર્કિક isFormatNonNumericArg(અચલ Ast_ગાંઠ_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        પાછા dtypep->isString()  //
               || VN_IS(dtypep, AssocArrayDType)  //
               || VN_IS(dtypep, WildcardArrayDType)  //
               || VN_IS(dtypep, ClassRefDType)  //
               || VN_IS(dtypep, DynArrayDType)  //
               || VN_IS(dtypep, UnpackArrayDType)  //
               || VN_IS(dtypep, QueueDType)  //
               || (VN_IS(dtypep, ગાંઠ_UOr_Struct_DType)
                   && !VN_AS(dtypep, ગાંઠ_UOr_Struct_DType)->packed());
    }

    શૂન્ય checkFormatNumericArg(Ast_ગાંઠ* argp, અક્ષર ch) {
        // IEEE 1800-2023 21.2.1.1 suggests %d of a class is illegal.
        // Howver UVM tests require %0d print unique identifier of the class.
        જો (std::tolower(ch) == 'd') પાછા;
        // In contrast, %x of a class reference, causes errors on several simulators.
        જો (isFormatNonNumericArg(argp->dtypep()))
            argp->v3error(
                "$display-like format of '%"s + ch + "' illegal with non-numeric argument\n"
                << argp->warnMore() << "... Suggest use '%s'");
    }

    શૂન્ય visit(AstSFormatArg* nodep) override {}  // Done by AstSFormatF
    શૂન્ય visit(AstSFormat* nodep) override {
        assertAtStatement(nodep);
        userIterateAndNext(nodep->fmtp(), WidthVP{SELF, BOTH}.p());
        userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
    }
    શૂન્ય visit(AstToStringN* nodep) override {
        // Inserted by V3Width only so we know has been resolved
        nodep->dtypeSetString();
        userIterateAndNext(nodep->lhsp(), WidthVP{nodep->lhsp()->dtypep(), BOTH}.p());
    }
    શૂન્ય visit(AstSFormatF* nodep) override {
        // Excludes NodeDisplay, see below
        જો (m_vup && !m_vup->prelim()) પાછા;  // Can be called as statement or function
        જો (nodep->didWidthAndSet()) પાછા;
        //
        UINFO(9, "  Display in " << nodep->text());

        // Just let all arguments seek their natural sizes
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
        જો (!nodep->પદાવલિ_Format_2()) {  // Non-constant format, can't check format string types
            visit_sformatf_format(nodep);
        }

        Ast_ગાંઠ_પદાવલિ* oldExprsp = nodep->exprsp();
        જો (nodep->પદાવલિ_Format_2() && oldExprsp) {
            VL_DO_DANGLING(iterateCheckString(nodep, "format", nodep->exprsp(), BOTH), oldExprsp);
            oldExprsp = VN_AS(nodep->exprsp()->nextp(), ગાંઠ_પદાવલિ);
        }
        જો (oldExprsp) oldExprsp->unlinkFrBackWithNext();
        જ્યારે (Ast_ગાંઠ_પદાવલિ* argp = oldExprsp) {
            oldExprsp = VN_AS(oldExprsp->nextp(), ગાંઠ_પદાવલિ);
            જો (oldExprsp) oldExprsp->unlinkFrBackWithNext();
            જો (VN_IS(argp, SFormatArg)) {
                nodep->addExprsp(argp);
                ચાલુ;
            }
            // Need to record formatAttr's at elaboration time, as later optimizations
            // may change an argument's data type. Plus need them for runtime formats
            VFormatAttr formatAttr = VFormatAttr::UNSIGNED;
            Ast_ગાંઠ_DType* અચલ dtypep = argp->dtypep()->skipRefp();
            જો (dtypep->isDouble()) {
                formatAttr = VFormatAttr::DOUBLE;
            } નહીંતર જો (dtypep->isString()) {
                formatAttr = VFormatAttr::STRING;
            } નહીંતર જો (isFormatNonNumericArg(dtypep)) {
                જો (AstClassRefDType* અચલ classRefp = VN_CAST(dtypep, ClassRefDType)) {
                    જો (classRefp->classp()) {
                        classRefp->classp()->markPrintedFrom();
                        v3Global.hasPrintedObjects(સાચું);
                    }
                } નહીંતર {
                    // Class handles inside containers print type names, not their members.
                    Ast_ગાંઠ_DType* ગાંઠ_Dtypep = dtypep;
                    જ્યારે (ગાંઠ_Dtypep) {
                        ગાંઠ_Dtypep = ગાંઠ_Dtypep->skipRefp();
                        જો (Ast_ગાંઠ_UOr_Struct_DType* અચલ uOrStructDTypep
                            = VN_CAST(ગાંઠ_Dtypep, ગાંઠ_UOr_Struct_DType)) {
                            uOrStructDTypep->setEmitToString();
                            v3Global.hasPrintedObjects(સાચું);
                        }
                        ગાંઠ_Dtypep = ગાંઠ_Dtypep->subDTypep();
                    }
                }
                Ast_ગાંઠ_પદાવલિ* અચલ newp = નવું AstToStringN{argp->fileline(), argp};
                formatAttr = VFormatAttr::COMPLEX;
                argp = newp;
            } નહીંતર જો (nodep->પદાવલિ_Format_2()) {
                જો (AstEnumDType* અચલ enumDtp = formatEnumDType(argp)) {
                    nodep->addExprsp(newFormatEnumArg(argp, enumDtp));
                    ચાલુ;
                }
            }
            જો (formatAttr.isUnsigned() && dtypep->isSigned()) {
                formatAttr = VFormatAttr::SIGNED;
            }
            જો (formatAttr.isUnsigned()) {  // Save Ast space and imply the AstSFormatArg
                nodep->addExprsp(argp);
            } નહીંતર {
                nodep->addExprsp(નવું AstSFormatArg{argp->fileline(), formatAttr, argp});
            }
        }

        UINFO(9, "  Display out " << nodep->text());
    }
    શૂન્ય visit(AstCReset* nodep) override {
        assert_At_પદાવલિ(nodep);
        nodep->dtypeFrom(m_vup->dtypep());
    }
    શૂન્ય visit(AstCReturn* nodep) override { nodep->v3fatalSrc("Should not exist yet"); }
    શૂન્ય visit(AstConstraintRef* nodep) override { userIterateChildren(nodep, nullptr); }
    શૂન્ય visit(AstDisplay* nodep) override {
        assertAtStatement(nodep);
        જો (nodep->filep()) iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
        // Just let all arguments seek their natural sizes
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
    }
    શૂન્ય visit(AstElabDisplay* nodep) override {
        assertAtStatement(nodep);
        // Just let all arguments seek their natural sizes
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
        જો (!m_paramsOnly) {
            nodep->foreach([આ](AstScopeName* nodep) {  //
                nodep->replaceWith(
                    નવું AstConst{nodep->fileline(), AstConst::String{}, "<scope-unavailable>"});
                VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
            });
            V3Const::constifyParamsEdit(nodep->fmtp());  // fmtp may change
            string text = VString::dequotePercent(nodep->fmtp()->text());
            જો (text.empty()) text = "Elaboration system task message (IEEE 1800-2023 20.11)";
            પસંદગી (nodep->displayType()) {
            સ્થિતિ VDisplayType::DT_INFO: nodep->v3warn(USERINFO, text); તોડો;
            સ્થિતિ VDisplayType::DT_ERROR: nodep->v3warn(USERERROR, text); તોડો;
            સ્થિતિ VDisplayType::DT_WARNING: nodep->v3warn(USERWARN, text); તોડો;
            સ્થિતિ VDisplayType::DT_FATAL: nodep->v3warn(USERFATAL, text); તોડો;
            મૂળભૂત: UASSERT_OBJ(ખોટું, nodep, "Unexpected elaboration display type");
            }
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_વૃક્ષ(), nodep);
        }
    }
    શૂન્ય visit(AstDumpCtl* nodep) override {
        assertAtStatement(nodep);
        જો (nodep->exprp()) iterateCheckString(nodep, "LHS", nodep->exprp(), BOTH);
    }
    શૂન્ય visit(AstFOpen* nodep) override {
        // Although a system function in IEEE, here a statement which sets the file pointer (MCD)
        iterateCheckString(nodep, "filename", nodep->filenamep(), BOTH);
        iterateCheckString(nodep, "mode", nodep->modep(), BOTH);
    }
    શૂન્ય visit(AstFOpenMcd* nodep) override {
        iterateCheckString(nodep, "filename", nodep->filenamep(), BOTH);
    }
    શૂન્ય visit(AstFClose* nodep) override {
        assertAtStatement(nodep);
        iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
    }
    શૂન્ય visit(AstFError* nodep) override {
        assert_At_પદાવલિ(nodep);
        જો (m_vup->prelim()) {
            iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
            // Could be string or packed array
            userIterateAndNext(nodep->strp(), WidthVP{SELF, BOTH}.p());
        }
    }
    શૂન્ય visit(AstFEof* nodep) override {
        assert_At_પદાવલિ(nodep);
        જો (m_vup->prelim()) { iterateCheckFileDesc(nodep, nodep->filep(), BOTH); }
    }
    શૂન્ય visit(AstFFlush* nodep) override {
        assertAtStatement(nodep);
        જો (nodep->filep()) iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
    }
    શૂન્ય visit(AstFRewind* nodep) override { iterateCheckFileDesc(nodep, nodep->filep(), BOTH); }
    શૂન્ય visit(AstFTell* nodep) override { iterateCheckFileDesc(nodep, nodep->filep(), BOTH); }
    શૂન્ય visit(AstFSeek* nodep) override {
        iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
        iterateCheckSigned32(nodep, "$fseek offset", nodep->offset(), BOTH);
        iterateCheckSigned32(nodep, "$fseek operation", nodep->operation(), BOTH);
    }
    શૂન્ય visit(AstFGetC* nodep) override {
        assert_At_પદાવલિ(nodep);
        જો (m_vup->prelim()) iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
    }
    શૂન્ય visit(AstFGetS* nodep) override {
        assert_At_પદાવલિ(nodep);
        જો (m_vup->prelim()) {
            iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
            userIterateAndNext(nodep->strgp(), WidthVP{SELF, BOTH}.p());
        }
    }
    શૂન્ય visit(AstFUngetC* nodep) override {
        assert_At_પદાવલિ(nodep);
        જો (m_vup->prelim()) {
            iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
            iterateCheckSigned32(nodep, "$fungetc character", nodep->charp(), BOTH);
        }
    }
    શૂન્ય visit(AstFRead* nodep) override {
        assert_At_પદાવલિ(nodep);
        જો (m_vup->prelim()) {
            userIterateAndNext(nodep->memp(), WidthVP{SELF, BOTH}.p());
            iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
            જો (nodep->startp()) {
                iterateCheckSigned32(nodep, "$fread start", nodep->startp(), BOTH);
            }
            જો (nodep->countp()) {
                iterateCheckSigned32(nodep, "$fread count", nodep->countp(), BOTH);
            }
        }
    }
    શૂન્ય visit(AstFScanF* nodep) override {
        assert_At_પદાવલિ(nodep);
        જો (m_vup->prelim()) {
            iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
            userIterateAndNext(nodep->exprsp(), WidthVP{SELF, BOTH}.p());
        }
    }
    શૂન્ય visit(AstSScanF* nodep) override {
        assert_At_પદાવલિ(nodep);
        જો (m_vup->prelim()) {
            userIterateAndNext(nodep->fromp(), WidthVP{SELF, BOTH}.p());
            userIterateAndNext(nodep->exprsp(), WidthVP{SELF, BOTH}.p());
        }
    }
    શૂન્ય visit(Ast_ઢગલો_Trace_F* nodep) override { nodep->dtypeSetString(); }
    શૂન્ય visit(AstSysIgnore* nodep) override {
        userIterateAndNext(nodep->exprsp(), WidthVP{SELF, BOTH}.p());
    }
    શૂન્ય visit(AstSystemF* nodep) override {
        assert_At_પદાવલિ(nodep);
        જો (m_vup->prelim()) userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
    }
    શૂન્ય visit(AstSystemT* nodep) override {
        assertAtStatement(nodep);
        userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
    }
    શૂન્ય visit(Ast_ગાંઠ_Read_Write_Mem* nodep) override {
        assertAtStatement(nodep);
        iterateCheckString(nodep, "filename", nodep->filenamep(), BOTH);
        userIterateAndNext(nodep->memp(), WidthVP{SELF, BOTH}.p());
        અચલ Ast_ગાંઠ_DType* subp = nullptr;
        જો (અચલ AstAssocArrayDType* adtypep
            = VN_CAST(nodep->memp()->dtypep()->skipRefp(), AssocArrayDType)) {
            subp = adtypep->subDTypep();
            જો (!adtypep->ચાવી_DTypep()->skipRefp()->basicp()
                || !adtypep->ચાવી_DTypep()->skipRefp()->basicp()->keyword().isIntNumeric()) {
                nodep->memp()->v3error(nodep->verilogKwd()
                                       << " address/key must be integral (IEEE 1800-2023 21.4.1)");
            }
        } નહીંતર જો (અચલ AstUnpackArrayDType* અચલ adtypep
                   = VN_CAST(nodep->memp()->dtypep()->skipRefp(), UnpackArrayDType)) {
            subp = adtypep->subDTypep();
        } નહીંતર જો (અચલ AstDynArrayDType* અચલ adtypep
                   = VN_CAST(nodep->memp()->dtypep()->skipRefp(), DynArrayDType)) {
            subp = adtypep->subDTypep();
        } નહીંતર {
            nodep->memp()->v3warn(
                E_UNSUPPORTED,
                "Unsupported: " << nodep->verilogKwd()
                                << " into other than unpacked, dynamic, or associative array");
        }
        જો (subp
            && (!subp->skipRefp()->basicp()
                || !subp->skipRefp()->basicp()->keyword().isIntNumeric())) {
            nodep->memp()->v3warn(E_UNSUPPORTED,
                                  "Unsupported: " << nodep->verilogKwd()
                                                  << " array values must be integral");
        }
        userIterateAndNext(nodep->lsbp(), WidthVP{SELF, BOTH}.p());
        userIterateAndNext(nodep->msbp(), WidthVP{SELF, BOTH}.p());
    }
    શૂન્ય visit(AstTestPlusArgs* nodep) override {
        assert_At_પદાવલિ(nodep);
        જો (m_vup->prelim()) iterateCheckString(nodep, "LHS", nodep->searchp(), BOTH);
    }
    શૂન્ય visit(Ast_કિંમત_Plus_Args* nodep) override {
        assert_At_પદાવલિ(nodep);
        જો (m_vup->prelim()) {
            iterateCheckString(nodep, "LHS", nodep->searchp(), BOTH);
            userIterateAndNext(nodep->outp(), WidthVP{SELF, BOTH}.p());
        }
    }
    શૂન્ય visit(AstTimeFormat* nodep) override {
        assertAtStatement(nodep);
        જો (nodep->unitsp()) iterateCheckSigned32(nodep, "units", nodep->unitsp(), BOTH);
        જો (nodep->precisionp())
            iterateCheckSigned32(nodep, "precision", nodep->precisionp(), BOTH);
        જો (nodep->suffixp()) iterateCheckString(nodep, "suffix", nodep->suffixp(), BOTH);
        જો (nodep->widthp()) iterateCheckSigned32(nodep, "width", nodep->widthp(), BOTH);
    }
    શૂન્ય visit(AstCStmtUser* nodep) override {
        // Just let all arguments seek their natural sizes
        assertAtStatement(nodep);
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
    }
    શૂન્ય visit(AstAssert* nodep) override {
        assertAtStatement(nodep);
        iterateCheckBool(nodep, "Property", nodep->propp(), BOTH);  // it's like an if() condition.
        userIterateAndNext(nodep->passsp(), nullptr);
        userIterateAndNext(nodep->failsp(), nullptr);
    }
    શૂન્ય visit(AstAssertCtl* nodep) override {
        assertAtStatement(nodep);
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
    }
    શૂન્ય visit(AstAssertIntrinsic* nodep) override {
        assertAtStatement(nodep);
        iterateCheckBool(nodep, "Property", nodep->propp(), BOTH);  // it's like an if() condition.
        userIterateAndNext(nodep->passsp(), nullptr);
        userIterateAndNext(nodep->failsp(), nullptr);
    }
    શૂન્ય visit(AstCover* nodep) override {
        assertAtStatement(nodep);
        iterateCheckBool(nodep, "Property", nodep->propp(), BOTH);  // it's like an if() condition.
        userIterateAndNext(nodep->passsp(), nullptr);
    }
    શૂન્ય visit(AstRestrict* nodep) override {
        assertAtStatement(nodep);
        iterateCheckBool(nodep, "Property", nodep->propp(), BOTH);  // it's like an if() condition.
    }

    // Select the part of a pin connection that belongs to an element of an instance array,
    // one dimension at a time, starting with 'rangep'. 'idx' is the row-major position of the
    // element in the dimensions from 'rangep' inwards, each counted from the left. The
    // connection has had its PRELIM visit. Returns true if the pin needs nothing else (the
    // selected connection matches the port, or there was an error), false if the pin still
    // needs the usual checks.
    તાર્કિક pinInstArrayElement(AstPin* nodep, અચલ AstRange* rangep, પૂર્ણાંક idx) {
        Ast_ગાંઠ_DType* અચલ modDTypep = nodep->modVarp()->dtypep()->skipRefp();
        Ast_ગાંઠ_DType* અચલ conDTypep = nodep->exprp()->dtypep()->skipRefp();

        // If types match, then nothing else to do
        જો (conDTypep == modDTypep || similarDTypeRecurse(conDTypep, modDTypep)) પાછા સાચું;
        // Types don't match, but all dimensions done, still need the usual checks
        જો (!rangep) પાછા ખોટું;

        અચલ AstRange* અચલ subRangep = VN_AS(rangep->nextp(), Range);

        // Number of dimensions from 'rangep' inwards, and of elements in those after 'rangep'
        uint32_t nDims = 1;
        પૂર્ણાંક subElems = 1;
        માટે (અચલ AstRange* rp = subRangep; rp; rp = VN_AS(rp->nextp(), Range)) {
            ++nDims;
            subElems *= rp->elementsConst();
        }
        અચલ પૂર્ણાંક elements = rangep->elementsConst();
        અચલ પૂર્ણાંક from_ડાબું = idx / subElems;

        અચલ AstUnpackArrayDType* અચલ arrp = VN_CAST(conDTypep, UnpackArrayDType);
        અચલ uint32_t conDims = conDTypep->dimensions(ખોટું).second;
        અચલ uint32_t modDims = modDTypep->dimensions(ખોટું).second;

        // Unpacked array with other than the port's number of unpacked dimensions: its slowest
        // varying unpacked dimensions must match the instance array dimensions exactly in size,
        // and the rest be those of the port. Connect its element at the same position, counted
        // from the left. The inner dimensions then select from that element the same way.
        જો (arrp && conDims != modDims) {
            જો (conDims != nDims + modDims || arrp->elementsConst() != elements) {
                nodep->v3error(ucfirst(nodep->prettyOperatorName())
                               << " as part of a module instance array requires the connection's"
                                  " leading unpacked dimensions to match the instance array"
                                  " dimensions (IEEE 1800-2023 23.3.3.5)");
                userIterateAndNext(nodep->exprp(),
                                   WidthVP{conDTypep, FINAL, STREAM_USE_ASSIGN}.p());
                પાછા સાચું;
            }
            // ArraySel index is counted from lo
            અચલ પૂર્ણાંક bit = arrp->declRange().ascending() ? from_ડાબું : elements - 1 - from_ડાબું;
            // Connection is self-determined
            userIterateAndNext(nodep->exprp(), WidthVP{conDTypep, FINAL, STREAM_USE_ASSIGN}.p());
            Ast_ગાંઠ_પદાવલિ* અચલ fromp = VN_AS(nodep->exprp(), ગાંઠ_પદાવલિ)->unlinkFrBack();
            જો (Ast_ગાંઠ_પદાવલિ* અચલ elemp = newIfaceArrayElement(fromp, bit)) {
                nodep->exprp(elemp);
                VL_DO_DANGLING(ધકેલો_Deletep(fromp), fromp);
            } નહીંતર {
                AstArraySel* અચલ selp = નવું AstArraySel{fromp->fileline(), fromp, bit};
                selp->didWidth(સાચું);
                nodep->exprp(selp);
            }
            પાછા pinInstArrayElement(nodep, subRangep, idx % subElems);
        }

        // Otherwise if not packed (including an unpacked array with the port's number of unpacked
        // dimensions), connect it to every element, if it is compatible with the port
        જો (!conDTypep->isIntegralOrPacked() || !modDTypep->isIntegralOrPacked()) પાછા ખોટું;

        // Packed connection
        અચલ પૂર્ણાંક modwidth = modDTypep->width();
        અચલ પૂર્ણાંક conwidth = conDTypep->width();

        // Connection has the width of the port, connect it to every element
        જો (conwidth == modwidth) પાછા ખોટું;

        // Connection has the width of all the elements, connect a slice to each element,
        // leftmost element to the leftmost (most significant) slice
        જો (conwidth == modwidth * elements * subElems) {
            જો (rangep->ascending()) {
                nodep->exprp()->v3warn(ASCRANGE, "Ascending instance range connecting to "
                                                 "vector: left < right of instance range: ["
                                                     << rangep->ડાબું_Const() << ":"
                                                     << rangep->જમણું_Const() << "]");
            }
            અચલ પૂર્ણાંક selwidth = modwidth * subElems;
            // Connection is self-determined
            userIterateAndNext(nodep->exprp(), WidthVP{conDTypep, FINAL, STREAM_USE_ASSIGN}.p());
            Ast_ગાંઠ_પદાવલિ* અચલ fromp = VN_AS(nodep->exprp(), ગાંઠ_પદાવલિ)->unlinkFrBack();
            AstSel* અચલ selp = નવું AstSel{fromp->fileline(), fromp,
                                            selwidth * (elements - 1 - from_ડાબું), selwidth};
            selp->didWidth(સાચું);
            nodep->exprp(selp);
            પાછા pinInstArrayElement(nodep, subRangep, idx % subElems);
        }

        // Otherwise it is an error, as it must connect to one element or to all of them
        nodep->v3error(ucfirst(nodep->prettyOperatorName())
                       << " as part of a module instance array" << " requires " << modwidth
                       << " or " << modwidth * elements * subElems << " bits, but connection's "
                       << nodep->exprp()->prettyTypeName() << " generates " << conwidth
                       << " bits. (IEEE 1800-2023 23.3.3)");
        userIterateAndNext(nodep->exprp(), WidthVP{conDTypep, FINAL, STREAM_USE_ASSIGN}.p());
        પાછા સાચું;
    }

    શૂન્ય visit(AstPin* nodep) override {
        // UINFOTREE(1, nodep, "", "PinPre");
        // TOP LEVEL NODE
        જો (nodep->modVarp() && nodep->modVarp()->isGParam()) {
            // An unpacked array concatenation is a pattern of its items (IEEE 1800-2023 10.10)
            AstReplicate* અચલ concatp = VN_CAST(nodep->exprp(), Replicate);
            અચલ AstConst* અચલ countp = concatp ? VN_CAST(concatp->countp(), Const) : nullptr;
            અચલ Ast_ગાંઠ_DType* અચલ dtypep = nodep->modVarp()->subDTypep()->skipRefOrNullp();
            જો (countp && countp->toUInt() == 1
                && (VN_IS(dtypep, UnpackArrayDType) || VN_IS(dtypep, BracketArrayDType))) {
                AstPattern* અચલ patternp = નવું AstPattern{concatp->fileline(), nullptr};
                patConcatConvertRecurse(patternp, concatp->srcp());
                concatp->replaceWith(patternp);
                VL_DO_DANGLING(ધકેલો_Deletep(concatp), concatp);
            }
            // An override pattern is left for the specialized module to type
            VL_RESTORER(m_inParamOverride);
            m_inParamOverride = isOverridePattern(nodep->exprp());
            userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
        } નહીંતર જો (!m_paramsOnly) {
            જો (!nodep->modVarp()->didWidth()) {
                // Var hasn't been widthed, so make it so.
                userIterate(nodep->modVarp(), nullptr);
            }
            જો (!nodep->exprp()) {  // No-connect
                જો (nodep->modVarp()->isIfaceArraySplit()) expandIfaceArrayPin(nodep);
                પાછા;
            }
            // Very much like like an assignment, but which side is LH/RHS
            // depends on pin being a in/output/inout.
            અચલ VDirection pinDirection = nodep->modVarp()->direction();
            જો (VN_IS(nodep->exprp(), ગાંઠ_Stream) && pinDirection.isInoutOrRef()) {
                nodep->exprp()->v3error("Streaming concatenation cannot be connected to '"
                                        << pinDirection.prettyName()
                                        << "' port (IEEE 1800-2023 11.4.14)");
            }
            userIterateAndNext(nodep->exprp(), WidthVP{nodep->modVarp()->dtypep(), PRELIM}.p());
            Ast_ગાંઠ_DType* modDTypep = nodep->modVarp()->dtypep();
            Ast_ગાંઠ_DType* conDTypep = nodep->exprp()->dtypep();
            UASSERT_OBJ(modDTypep, nodep, "Unlinked pin data type");
            UASSERT_OBJ(conDTypep, nodep, "Unlinked pin data type");
            modDTypep = modDTypep->skipRefp();
            conDTypep = conDTypep->skipRefp();
            // An interface port, or an array of them, needs interface instances, not virtual
            // interfaces
            જો (અચલ AstIfaceRefDType* અચલ modIrefp
                = VN_CAST(modDTypep->elemDTypep(), IfaceRefDType)) {
                અચલ AstIfaceRefDType* અચલ conIrefp
                    = VN_CAST(conDTypep->elemDTypep(), IfaceRefDType);
                જો (!modIrefp->isVirtual() && conIrefp && conIrefp->isVirtual()) {
                    nodep->v3error("Illegal " << nodep->prettyOperatorName()
                                              << ", interface port connected to a virtual"
                                                 " interface (IEEE 1800-2023 23.3.3.4)");
                }
            }
            Ast_ગાંઠ_DType* subDTypep = modDTypep;
            અચલ પૂર્ણાંક modwidth = modDTypep->width();
            પૂર્ણાંક conwidth = conDTypep->width();
            જો (conDTypep == modDTypep  // If match, we're golden
                || similarDTypeRecurse(conDTypep, modDTypep)) {
                userIterateAndNext(nodep->exprp(),
                                   WidthVP{subDTypep, FINAL, STREAM_USE_ASSIGN}.p());
            } નહીંતર {
                જો (m_cellp->arrayIdx() >= 0) {
                    // Element of an instance array, select its part of the connection,
                    // then check it like any other pin, unless that is all done already
                    જો (pinInstArrayElement(nodep, m_cellp->rangep(), m_cellp->arrayIdx())) {
                        જો (nodep->modVarp()->isIfaceArraySplit()) expandIfaceArrayPin(nodep);
                        પાછા;
                    }
                    conDTypep = nodep->exprp()->dtypep()->skipRefp();
                    conwidth = conDTypep->width();
                }
                જો (nodep->modVarp()->direction() == VDirection::REF) {
                    nodep->v3error("Ref connection "
                                   << nodep->modVarp()->prettyNameQ()
                                   << " requires matching types;" << " ref requires "
                                   << modDTypep->prettyDTypeNameQ()
                                   << " data type but connection is "
                                   << conDTypep->prettyDTypeNameQ() << " data type.");
                } નહીંતર જો (nodep->modVarp()->isTristate()) {
                    જો (modwidth != conwidth) {
                        // Ideally should call pinReconnectSimple which would tolerate this
                        // then have a conversion warning
                        nodep->v3warn(E_UNSUPPORTED,
                                      "Unsupported: " << ucfirst(nodep->prettyOperatorName())
                                                      << " to inout signal requires " << modwidth
                                                      << " bits, but connection's "
                                                      << nodep->exprp()->prettyTypeName()
                                                      << " generates " << conwidth << " bits.");
                        // otherwise would need some mess to force both sides to proper size
                    }
                } નહીંતર જો (nodep->modVarp()->direction().isWritable()
                           && ((conDTypep->isDouble() && !modDTypep->isDouble())
                               || (!conDTypep->isDouble() && modDTypep->isDouble()))) {
                    nodep->v3warn(E_UNSUPPORTED,
                                  "Unsupported: " << ucfirst(nodep->prettyOperatorName())
                                                  << " connects real to non-real");
                }

                // Check if an interface is connected to a non-interface and vice versa
                જો ((VN_IS(modDTypep, IfaceRefDType) && !VN_IS(conDTypep, IfaceRefDType))
                    || (VN_IS(conDTypep, IfaceRefDType) && !VN_IS(modDTypep, IfaceRefDType))) {
                    nodep->v3error("Illegal " << nodep->prettyOperatorName() << ","
                                              << " mismatch between port which is"
                                              << (VN_CAST(modDTypep, IfaceRefDType) ? "" : " not")
                                              << " an interface," << " and expression which is"
                                              << (VN_CAST(conDTypep, IfaceRefDType) ? "" : " not")
                                              << " an interface.");
                }

                // TODO Simple dtype checking, should be a more general check
                અચલ Ast_ગાંઠ_Array_DType* અચલ પદાવલિ_Arrayp = VN_CAST(conDTypep, UnpackArrayDType);
                અચલ Ast_ગાંઠ_Array_DType* અચલ modArrayp = VN_CAST(modDTypep, UnpackArrayDType);
                જો (પદાવલિ_Arrayp && modArrayp && VN_IS(conDTypep->elemDTypep(), IfaceRefDType)
                    && unpackedArraySize(conDTypep) != unpackedArraySize(modDTypep)) {
                    nodep->v3error("Illegal "
                                   << nodep->prettyOperatorName() << ","
                                   << " mismatch between port which is an interface array of size "
                                   << unpackedArraySize(modDTypep) << ","
                                   << " and expression which is an interface array of size "
                                   << unpackedArraySize(conDTypep) << ".");
                    UINFO(1, "    Related lo: " << modDTypep);
                    UINFO(1, "    Related hi: " << conDTypep);
                } નહીંતર જો ((પદાવલિ_Arrayp && !modArrayp) || (!પદાવલિ_Arrayp && modArrayp)) {
                    nodep->v3error(
                        "Illegal "
                        << nodep->prettyOperatorName() << "," << " mismatch between port which is"
                        << (modArrayp ? "" : " not") << " an array," << " and expression which is"
                        << (પદાવલિ_Arrayp ? "" : " not") << " an array. (IEEE 1800-2023 7.6)");
                    UINFO(1, "    Related lo: " << modDTypep);
                    UINFO(1, "    Related hi: " << conDTypep);
                } નહીંતર {
                    checkUnpackedArrayAssignmentCompatible<AstVar, Ast_ગાંઠ_Var_Ref>(
                        nodep, nodep->modVarp(), VN_CAST(nodep->exprp(), ગાંઠ_Var_Ref));
                    UINFO(1, "    Related lo: " << modDTypep);
                    UINFO(1, "    Related hi: " << conDTypep);
                }
                iterateCheckAssign(nodep, "pin connection", nodep->exprp(), FINAL, subDTypep,
                                   nodep->modVarp()->direction().pinAccess());
            }
            // Interface array port: connect its elements
            જો (nodep->modVarp()->isIfaceArraySplit()) expandIfaceArrayPin(nodep);
        }
        // UINFOTREE(1, nodep, "", "PinOut");
    }
    શૂન્ય visit(AstCell* nodep) override {
        VL_RESTORER(m_cellp);
        m_cellp = nodep;
        જો (!m_paramsOnly) {
            જો (VN_IS(nodep->modp(), NotFoundModule)) {
                // We've resolved parameters and hit a module that we couldn't resolve.  It's
                // finally time to report it.
                // Note only here in V3Width as this is first visitor after V3Dead.
                nodep->modNameFileline()->v3warn(
                    MODMISSING, "Cannot find file containing module: '"
                                    << nodep->modName() << "'\n"
                                    << nodep->modNameFileline()->warnContextPrimary()
                                    << V3Error::warnAdditionalInfo()
                                    << v3Global.opt.file_Path_Looked_સંદેશ(nodep->modNameFileline(),
                                                                      nodep->modName()));
            }
            જો (nodep->rangep()) userIterateAndNext(nodep->rangep(), WidthVP{SELF, BOTH}.p());
            userIterateAndNext(nodep->pinsp(), nullptr);
            // If element of an instance array, pins are now connected, no longer need the ranges
            જો (nodep->arrayIdx() >= 0) {
                nodep->arrayIdx(-1);
                ધકેલો_Deletep(nodep->rangep()->unlinkFrBackWithNext());
            }
        }
        userIterateAndNext(nodep->paramsp(), nullptr);
    }
    શૂન્ય visit(AstGatePin* nodep) override {
        assert_At_પદાવલિ(nodep);
        જો (m_vup->prelim()) {
            userIterateAndNext(nodep->rangep(), WidthVP{SELF, BOTH}.p());
            userIterateAndNext(nodep->exprp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            nodep->dtypeFrom(nodep->rangep());
            // Very much like like an pin
            અચલ Ast_ગાંઠ_DType* અચલ pinDTypep = nodep->exprp()->dtypep();
            અચલ પૂર્ણાંક અંક_Insts = nodep->rangep()->elementsConst();
            અચલ પૂર્ણાંક modwidth = અંક_Insts;
            અચલ પૂર્ણાંક pinwidth = pinDTypep->width();
            જો (pinwidth == 1 && modwidth > 1) {  // Multiple connections
                Ast_ગાંઠ_DType* અચલ subDTypep = nodep->findLogicDType(1, 1, pinDTypep->numeric());
                userIterateAndNext(nodep->exprp(), WidthVP{subDTypep, FINAL}.p());
                Ast_ગાંઠ* અચલ newp
                    = નવું AstReplicate{nodep->fileline(), nodep->exprp()->unlinkFrBack(),
                                       static_cast<uint32_t>(અંક_Insts)};
                nodep->replaceWith(newp);
            } નહીંતર {
                જો (pinwidth != modwidth) {  // && is not generic interconnect (when supported)
                    nodep->exprp()->v3error("Gate primitive connection expects "
                                            << modwidth << " bits "
                                            << ((modwidth != 1) ? "or 1 bit "s : ""s)
                                            << "on the gate port, but the connection generates "
                                            << pinwidth << " bits (IEEE 1800-2023 28.3.6)");
                }
                // Eliminating so pass down all of vup
                userIterateAndNext(nodep->exprp(), m_vup);
                nodep->replaceWith(nodep->exprp()->unlinkFrBack());
            }
            VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
        }
    }
    શૂન્ય visit(Ast_ગાંઠ_FTask* nodep) override {
        // Grab width from the output variable (if it's a function)
        જો (nodep->didWidth()) પાછા;
        જો (nodep->doingWidth()) {
            UINFO(5, "Recursive function or task call: " << nodep);
            nodep->recursive(સાચું);
            nodep->didWidth(સાચું);
            પાછા;
        }
        જો (nodep->classMethod() && nodep->name() == "rand_mode") {
            nodep->v3error("The 'rand_mode' method is built-in and cannot be overridden"
                           " (IEEE 1800-2023 18.8)");
        } નહીંતર જો (nodep->classMethod() && nodep->name() == "constraint_mode") {
            nodep->v3error("The 'constraint_mode' method is built-in and cannot be overridden"
                           " (IEEE 1800-2023 18.9)");
        }
        જો (nodep->classMethod() && nodep->name() == "new") {
            જો (nodep->isVirtual())
                nodep->v3error("class 'new()' cannot be virual (IEEE 1800-2023 18.3)");
            જો (nodep->isStatic())
                nodep->v3error("class 'new()' cannot be static (IEEE 1800-2023 18.3)");
            Ast_ગાંઠ* firstp = nullptr;
            માટે (Ast_ગાંઠ* itemp = nodep->stmtsp(); itemp; itemp = itemp->nextp()) {
                જો (Ast_Stmt_પદાવલિ* અચલ sep = VN_CAST(itemp, Stmt_પદાવલિ)) {
                    જો (AstNew* અચલ newp = VN_CAST(sep->exprp(), New)) {
                        જો (firstp) {
                            UINFOTREE(1, firstp, "", "-earlier");
                            newp->v3warn(SUPERNFIRST,
                                         "'super.new' must be first statement in a 'function "
                                         "new' (IEEE 1800-2023 8.15)\n"
                                             << newp->warnContextPrimary() << '\n'
                                             << firstp->warnOther()
                                             << "... Location of earlier statement\n"
                                             << firstp->warnContextSecondary());
                            તોડો;
                        }
                    }
                    ચાલુ;
                }
                જો (firstNewStatementOkRecurse(itemp)) ચાલુ;
                firstp = itemp;
            }
        }
        // Function hasn't been widthed, so make it so.
        // Would use user1 etc, but V3Width called from too many places to spend a user
        nodep->doingWidth(સાચું);
        VL_RESTORER(m_funcp);
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        // First width the function variable, as if is a recursive function we need data type
        જો (nodep->fvarp()) userIterate(nodep->fvarp(), nullptr);
        જો (nodep->isConstructor()) {
            // Pretend it's void so less special casing needed when look at dtypes
            nodep->dtypeSetVoid();
        } નહીંતર જો (nodep->fvarp()) {
            m_funcp = VN_AS(nodep, Func);
            UASSERT_OBJ(m_funcp, nodep, "FTask with function variable, but isn't a function");
            nodep->dtypeFrom(nodep->fvarp());  // Which will get it from fvarp()->dtypep()
        } નહીંતર જો (VN_IS(nodep, Property)) {
            nodep->dtypeSetBit();
        }
        WidthVP* vup = nullptr;
        જો (VN_IS(nodep, Property)) vup = WidthVP{SELF, BOTH}.p();
        userIterateChildren(nodep, vup);

        nodep->didWidth(સાચું);
        nodep->doingWidth(ખોટું);
        જો (nodep->dpiImport() && !nodep->dpiOpenParent() && markHasOpenArray(nodep)) {
            nodep->dpiOpenParentInc();  // Mark so V3Task will wait for a child to build calling
                                        // func
        }
        જો (nodep->fvarp() && !nodep->dpiImport() && !nodep->pureVirtual()
            && (!nodep->stmtsp() || !nodep->stmtsp()->existsAndNext([&](અચલ AstVarRef* varrefp) {
                   પાછા varrefp->varp() == nodep->fvarp();
               }))) {
            nodep->v3warn(NORETURN,
                          "Non-void function " << nodep->prettyNameQ() << " has no return value");
        }

        std::vector<AstVar*> ports;
        std::vector<AstAttrOf*> protos;
        AstAttrOf* protop = nullptr;  // Base prototype
        માટે (Ast_ગાંઠ* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            જો (AstVar* અચલ portp = VN_CAST(stmtp, Var)) {
                જો (portp->isIO() && !portp->isFuncReturn()) { ports.emplace_back(portp); }
            } નહીંતર જો (AstAttrOf* અચલ attrp = VN_CAST(stmtp, AttrOf)) {
                જો (attrp->attrType() == VAttrType::FUNC_ARG_PROTO) { protos.emplace_back(attrp); }
                જો (attrp->attrType() == VAttrType::FUNC_RETURN_PROTO) { protop = attrp; }
            }
        }
        જો (protop) {
            માટે (size_t i = 0; i < std::max(ports.size(), protos.size()); ++i) {
                જો (i >= ports.size() || i >= protos.size()) {
                    protop->v3warn(
                        PROTOTYPEMIS,
                        "In prototype for "
                            << nodep->prettyNameQ()
                            << ", the argument counts do not match the out-of-block declaration"
                            << " (IEEE 1800-2023 8.24)\n"
                            << protop->warnContextPrimary() << '\n'
                            << nodep->warnOther() << "... Location of out-of-block declaration\n"
                            << nodep->warnContextSecondary());
                    તોડો;
                } નહીંતર {
                    AstVar* અચલ portp = ports[i];
                    AstAttrOf* અચલ rprotop = protos[i];
                    Ast_ગાંઠ_DType* અચલ declDtp = portp->dtypep();
                    Ast_ગાંઠ_DType* અચલ protoDtp = rprotop->fromp()->dtypep();
                    જો (portp->name() != rprotop->name()) {
                        protoDtp->v3warn(PROTOTYPEMIS,
                                         "In prototype for "
                                             << nodep->prettyNameQ() << ", argument " << (i + 1)
                                             << " named " << rprotop->prettyNameQ()
                                             << " mismatches out-of-block argument name "
                                             << portp->prettyNameQ() << "  (IEEE 1800-2023 8.24)\n"
                                             << protoDtp->warnContextPrimary() << '\n'
                                             << declDtp->warnOther()
                                             << "... Location of out-of-block declaration\n"
                                             << declDtp->warnContextSecondary());
                    } નહીંતર જો (!similarDTypeRecurse(protoDtp, declDtp)) {
                        protoDtp->v3warn(
                            PROTOTYPEMIS,
                            "In prototype for "
                                << nodep->prettyNameQ() << ", argument " << portp->prettyNameQ()
                                << " data-type does not match out-of-block"
                                   " declaration's data-type (IEEE 1800-2023 8.24)\n"
                                << protoDtp->warnMore() << "... Prototype data type: "
                                << protoDtp->prettyDTypeNameQ() << '\n'
                                << protoDtp->warnMore() << "... Declaration data type: "
                                << declDtp->prettyDTypeNameQ() << '\n'
                                << protoDtp->warnContextPrimary() << '\n'
                                << declDtp->warnOther()
                                << "... Location of out-of-block declaration\n"
                                << declDtp->warnContextSecondary());
                    }
                }
            }
            // V3WidthCommit::visit(AstAttrOf) will delete nodep
        }
    }
    શૂન્ય visit(AstConstraint* nodep) override {
        જો (nodep->didWidth()) પાછા;
        VL_RESTORER(m_constraintp);
        m_constraintp = nodep;
        userIterateChildren(nodep, nullptr);
        nodep->didWidth(સાચું);
    }
    શૂન્ય visit(AstProperty* nodep) override {
        જો (nodep->didWidth()) પાછા;
        જો (nodep->doingWidth()) {
            UINFO(5, "Recursive property call: " << nodep);
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Recursive property call: " << nodep->prettyNameQ());
            nodep->recursive(સાચું);
            nodep->didWidth(સાચું);
            પાછા;
        }
        nodep->doingWidth(સાચું);
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        // Property call will be replaced by property body in V3AssertPre. Property body has bit
        // dtype, so set it here too
        nodep->dtypeSetBit();
        માટે (Ast_ગાંઠ* propStmtp = nodep->stmtsp(); propStmtp; propStmtp = propStmtp->nextp()) {
            જો (VN_IS(propStmtp, Var)) {
                userIterate(propStmtp, nullptr);
            } નહીંતર જો (VN_IS(propStmtp, PropSpec)) {
                iterateCheckSelf(nodep, "PropSpec", propStmtp, SELF, BOTH);
            } નહીંતર જો (VN_IS(propStmtp, InitialStaticStmt)
                       || VN_IS(propStmtp, InitialAutomaticStmt)) {
                // Property-local variable initialization -- iterate this node only
                userIterate(propStmtp, nullptr);
            } નહીંતર {
                propStmtp->v3fatalSrc("Invalid statement under AstProperty");
            }
        }
        nodep->didWidth(સાચું);
        nodep->doingWidth(ખોટું);
    }
    શૂન્ય visit(AstReturn* nodep) override { nodep->v3fatalSrc("'return' missed in LinkJump"); }
    શૂન્ય visit(AstSequence* nodep) override {
        // IEEE 1800-2023 16.7 (sequence declarations), 16.8 (sequence instances)
        // Width-check the sequence body so expressions are typed for later inlining.
        // Referenced sequences in assertion context will be inlined by V3AssertPre.
        જો (nodep->didWidth()) પાછા;
        nodep->dtypeSetBit();
        માટે (Ast_ગાંઠ* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            જો (VN_IS(stmtp, Var)) {
                userIterate(stmtp, nullptr);
            } નહીંતર જો (VN_IS(stmtp, ગાંઠ_પદાવલિ)) {
                iterateCheckSelf(nodep, "SeqBody", stmtp, SELF, BOTH);
            } નહીંતર {
                stmtp->v3fatalSrc("Invalid statement under AstSequence");
            }
        }
        nodep->didWidth(સાચું);
        // Keep all sequences for now; cleanup happens after V3AssertPre inlining.
    }

    AstPackage* getItemPackage(Ast_ગાંઠ* pkgItemp) {
        જ્યારે (pkgItemp->backp() && pkgItemp->backp()->nextp() == pkgItemp) {
            pkgItemp = pkgItemp->backp();
        }
        પાછા VN_CAST(pkgItemp->backp(), Package);
    }
    શૂન્ય visit(AstFuncRef* nodep) override {
        visit(static_cast<Ast_ગાંઠ_FTask_Ref*>(nodep));
        જો (nodep->taskp() && VN_IS(nodep->taskp(), Task)) {
            UASSERT_OBJ(m_vup, nodep, "Function reference where widthed expression expectation");
            જો (m_vup->prelim() && !VN_IS(nodep->backp(), Stmt_પદાવલિ))
                nodep->v3error(
                    "Cannot call a task/void-function as a function: " << nodep->prettyNameQ());
            nodep->dtypeSetVoid();
        } નહીંતર {
            nodep->dtypeFrom(nodep->taskp());
        }
        જો (nodep->fileline()->timingOn()) {
            Ast_ગાંઠ_Module* અચલ classp = nodep->classOrPackagep();
            જો (nodep->name() == "self" && classp->name() == "process") {
                // Find if package the class is in is std::
                AstPackage* અચલ packagep = getItemPackage(classp);
                જો (packagep && packagep->name() == "std") {
                    methodCallWarnTiming(nodep, "process");
                }
            }
        }
        // UINFOTREE(1, nodep, "", "FuncOut");
    }
    // Returns true if dtypep0 and dtypep1 have same dimensions
    સ્થિર તાર્કિક areSameSize(AstUnpackArrayDType* dtypep0, AstUnpackArrayDType* dtypep1) {
        અચલ std::vector<AstUnpackArrayDType*> dims0 = dtypep0->unpackDimensions();
        અચલ std::vector<AstUnpackArrayDType*> dims1 = dtypep1->unpackDimensions();
        જો (dims0.size() != dims1.size()) પાછા ખોટું;
        માટે (size_t i = 0; i < dims0.size(); ++i) {
            જો (dims0[i]->elementsConst() != dims1[i]->elementsConst()) પાછા ખોટું;
        }
        પાછા સાચું;
    }
    // Makes sure that port and pin have same size and same datatype
    શૂન્ય checkUnpackedArrayArgs(AstVar* portp, Ast_ગાંઠ* pinp) {
        જો (AstUnpackArrayDType* અચલ portDtypep
            = VN_CAST(portp->dtypep()->skipRefp(), UnpackArrayDType)) {
            જો (AstUnpackArrayDType* અચલ pinDtypep
                = VN_CAST(pinp->dtypep()->skipRefp(), UnpackArrayDType)) {
                જો (!areSameSize(portDtypep, pinDtypep)) {
                    pinp->v3warn(E_UNSUPPORTED,
                                 "Shape of the argument does not match the shape of the parameter "
                                     << "(" << pinDtypep->prettyDTypeNameQ() << " v.s. "
                                     << portDtypep->prettyDTypeNameQ() << ")");
                }
                અચલ AstBasicDType* અચલ portBasicp = portDtypep->basicp();
                અચલ AstBasicDType* અચલ pinBasicp = pinDtypep->basicp();
                // Arrays of unpacked structures have no basic type
                અચલ તાર્કિક sameElemDType
                    = portBasicp && pinBasicp
                          ? portBasicp->width() == pinBasicp->width()
                                && portBasicp->keyword().isSameish(pinBasicp->keyword())
                          : pinDtypep->elemDTypep()->similarDType(portDtypep->elemDTypep());
                જો (!sameElemDType) {
                    pinp->v3warn(
                        E_UNSUPPORTED,
                        "Data type of the argument is incompatible with the parameter"
                            << warnDTypes(pinp, pinDtypep, portDtypep, "Argument", "Parameter"));
                }
            } નહીંતર {
                pinp->v3warn(E_UNSUPPORTED, "Argument is not an unpacked array while parameter "
                                                << portp->prettyNameQ() << " is");
            }
        }
    }
    શૂન્ય processFTaskRefArgs(Ast_ગાંઠ_FTask_Ref* nodep) {
        // For arguments, is assignment-like context; see IEEE rules in AstNodeAssign
        // Function hasn't been widthed, so make it so.
        UINFO(5, "  FTASKREF " << nodep);
        UASSERT_OBJ(nodep->taskp(), nodep, "Unlinked");
        જો (nodep->didWidth()) પાછા;
        userIterate(nodep->taskp(), nullptr);
        //
        // And do the arguments to the task/function too
        કરો {
        reloop:
            // taskConnects may create a new task, and change nodep->taskp()
            અચલ V3TaskConnects tconnects
                = V3Task::taskConnects(nodep, nodep->taskp()->stmtsp(), &m_taskConnectState);
            જો (m_taskConnectState.didWrap()) m_memberMap.clear();  // As added a member
            માટે (અચલ auto& tconnect : tconnects) {
                અચલ AstVar* અચલ portp = tconnect.first;
                AstArg* અચલ argp = tconnect.second;
                Ast_ગાંઠ_પદાવલિ* pinp = argp->exprp();
                જો (!pinp) ચાલુ;  // Argument error we'll find later
                // Prelim may cause the node to get replaced; we've lost our
                // pointer, so need to iterate separately later
                જો (portp->attrSFormat()
                    && (!VN_IS(pinp, SFormatF) || pinp->nextp())) {  // Not already done
                    UINFO(4, "   sformat via metacomment: " << nodep);
                    VNRelinker handle;
                    argp->unlinkFrBackWithNext(&handle);  // Format + additional args, if any
                    Ast_ગાંઠ_પદાવલિ* argsp = nullptr;
                    જ્યારે (AstArg* અચલ nextargp = VN_AS(argp->nextp(), Arg)) {
                        // Expression goes to SFormatF
                        argsp = argsp->addNext(nextargp->exprp()->unlinkFrBackWithNext());
                        nextargp->unlinkFrBack()->delete_વૃક્ષ();  // Remove the call's Arg wrapper
                    }
                    string format;
                    જો (VN_IS(pinp, Const)) {
                        format = VN_AS(pinp, Const)->અંક().toString();
                    } નહીંતર {
                        pinp->v3error(
                            "Format to $display-like function must have constant format string");
                    }
                    VL_DO_DANGLING(ધકેલો_Deletep(argp), argp);
                    AstSFormatF* અચલ newp
                        = નવું AstSFormatF{nodep->fileline(), format, ખોટું, argsp};
                    જો (!newp->scopeNamep() && newp->formatScopeTracking()) {
                        newp->scopeNamep(નવું AstScopeName{newp->fileline(), સાચું});
                    }
                    handle.relink(નવું AstArg{newp->fileline(), "", newp});
                    // Connection list is now incorrect (has extra args in it).
                    જાઓ reloop;  // so exit early; next loop will correct it
                }  //
                નહીંતર જો (portp->basicp() && portp->basicp()->keyword() == VBasicDTypeKwd::STRING
                         && !VN_IS(pinp, CvtPackString)
                         && !VN_IS(pinp, SFormatF)  // Already generates a string
                         && !VN_IS(portp->dtypep(), UnpackArrayDType)  // Unpacked array must match
                         && !(VN_IS(pinp, VarRef) && VN_AS(pinp, VarRef)->varp()->basicp()
                              && VN_AS(pinp, VarRef)->varp()->basicp()->keyword()
                                     == VBasicDTypeKwd::STRING)) {
                    UINFO(4, "   Add CvtPackString: " << pinp);
                    VNRelinker handle;
                    pinp->unlinkFrBack(&handle);  // No next, that's the next pin
                    Ast_ગાંઠ_પદાવલિ* અચલ newp = નવું AstCvtPackString{pinp->fileline(), pinp};
                    handle.relink(newp);
                    pinp = newp;
                }
                જો (portp->isWritable()) {
                    V3LinkLValue::linkLValueSet(pinp, portp->direction().pinAccess());
                }
                // AstPattern requires assignments to pass datatype on PRELIM
                VL_DO_DANGLING(userIterate(pinp, WidthVP{portp->dtypep(), PRELIM}.p()), pinp);
            }
        } જ્યારે (ખોટું);
        // Stage 2
        {
            અચલ V3TaskConnects tconnects = V3Task::taskConnects(nodep, nodep->taskp()->stmtsp());
            માટે (અચલ auto& tconnect : tconnects) {
                AstVar* અચલ portp = tconnect.first;
                અચલ AstArg* અચલ argp = tconnect.second;
                Ast_ગાંઠ_પદાવલિ* અચલ pinp = argp->exprp();
                જો (!pinp) ચાલુ;  // Argument error we'll find later
                // Change data types based on above accept completion
                જો (nodep->taskp()->dpiImport()) checkUnpackedArrayArgs(portp, pinp);
                જો (portp->isDouble()) VL_DO_DANGLING(spliceCvtD(pinp), pinp);
            }
        }
        // Stage 3
        {
            અચલ V3TaskConnects tconnects = V3Task::taskConnects(nodep, nodep->taskp()->stmtsp());
            માટે (અચલ auto& tconnect : tconnects) {
                અચલ AstVar* અચલ portp = tconnect.first;
                અચલ AstArg* અચલ argp = tconnect.second;
                Ast_ગાંઠ_પદાવલિ* અચલ pinp = argp->exprp();
                જો (!pinp) ચાલુ;  // Argument error we'll find later
                // Do PRELIM again, because above accept may have exited early
                // due to node replacement
                userIterate(pinp, WidthVP{portp->dtypep(), PRELIM}.p());
            }
        }
        // Cleanup any open arrays
        જો (markHasOpenArray(nodep->taskp())) makeOpenArrayShell(nodep);
        // Stage 4
        {
            અચલ V3TaskConnects tconnects = V3Task::taskConnects(nodep, nodep->taskp()->stmtsp());
            માટે (અચલ auto& tconnect : tconnects) {
                અચલ AstVar* અચલ portp = tconnect.first;
                અચલ AstArg* અચલ argp = tconnect.second;
                Ast_ગાંઠ_પદાવલિ* pinp = argp->exprp();
                જો (!pinp) ચાલુ;  // Argument error we'll find later
                Ast_ગાંઠ_DType* અચલ portDTypep = portp->dtypep()->skipRefToEnump();
                જો (VN_IS(pinp, EmptyQueue)) {
                    Ast_ગાંઠ_પદાવલિ* newp = newConsDynArrayOrQueue(pinp->fileline(), portDTypep);
                    જો (!newp) {
                        pinp->v3warn(E_UNSUPPORTED,
                                     "Unsupported/Illegal: empty queue ('{}') in this context");
                        newp = નવું AstConst{pinp->fileline(), AstConst::Unsized32{}, 0};
                    }
                    newp->dtypeFrom(portDTypep);
                    pinp->replaceWith(newp);
                    VL_DO_DANGLING(ધકેલો_Deletep(pinp), pinp);
                    pinp = newp;
                }
                અચલ Ast_ગાંઠ_DType* અચલ pinDTypep = pinp->dtypep()->skipRefToEnump();
                અચલ AstIfaceRefDType* અચલ portIfacep
                    = VN_CAST(portDTypep->elemDTypep(સાચું), IfaceRefDType);
                અચલ VCastable ifaceCastable
                    = portIfacep ? Ast_ગાંઠ::computeCastable(portDTypep, pinDTypep, pinp,
                                                            /* checkIfaceArgCompat */ સાચું)
                                 : VCastable{VCastable::UNSUPPORTED};
                અચલ તાર્કિક matchingRefDTypes = portIfacep
                                                   ? ifaceCastable == VCastable::SAMEISH
                                                   : similarDTypeRecurse(portDTypep, pinDTypep);
                અચલ તાર્કિક pinIsNull = VN_IS(pinp, Const) && VN_AS(pinp, Const)->અંક().isNull();
                જો ((portp->isRef() || portp->isConstRef()) && !matchingRefDTypes) {
                    જો (portIfacep) {
                        pinp->v3error("Ref virtual interface argument "
                                      << portp->prettyNameQ()
                                      << " requires the same interface type, parameters, and "
                                         "modport.");
                    } નહીંતર {
                        pinp->v3error("Ref argument requires matching types;"
                                      << " port " << portp->prettyNameQ() << " requires "
                                      << portDTypep->prettyDTypeNameQ() << " but connection is "
                                      << pinDTypep->prettyDTypeNameQ() << ".");
                    }
                } નહીંતર જો (portIfacep && portIfacep->isVirtual() && portp->is_આગત() && !pinIsNull
                           && !ifaceCastable.isAssignable()) {
                    pinp->v3error("Virtual interface argument "
                                  << portp->prettyNameQ()
                                  << " requires a compatible interface type, parameters, and "
                                     "modport.");
                } નહીંતર જો (portp->isWritable() && pinp->width() != portp->width()) {
                    pinp->v3widthWarn(portp->width(), pinp->width(),
                                      "Function output argument "
                                          << portp->prettyNameQ() << " requires " << portp->width()
                                          << " bits, but connection's " << pinp->prettyTypeName()
                                          << " generates " << pinp->width() << " bits.");
                    VNRelinker relinkHandle;
                    pinp->unlinkFrBack(&relinkHandle);
                    Ast_ગાંઠ_પદાવલિ* અચલ newp = નવું AstResizeLValue{pinp->fileline(), pinp};
                    relinkHandle.relink(newp);
                }
                જો (portp->direction() != VDirection::REF
                    && !(portp->basicp()
                         && portp->basicp()->untyped())  // for properties, handled in V3AssertPre
                    && ((VN_IS(portDTypep, BasicDType) && pinDTypep->isNonPackedArray())
                        || (VN_IS(pinDTypep, BasicDType) && portDTypep->isNonPackedArray()))) {
                    pinp->v3error("Function Argument expects " << portDTypep->prettyDTypeNameQ()
                                                               << ", got "
                                                               << pinDTypep->prettyDTypeNameQ());
                    ચાલુ;
                }
                // An untyped formal preserves the actual argument's self-determined type.
                જો (portp->basicp() && portp->basicp()->untyped()) {
                    iterateCheckSelf(nodep, "Function Argument", pinp, SELF, FINAL);
                } નહીંતર જો (!portp->basicp() || portp->basicp()->isOpaque()) {
                    // Output args: at return caller = callee, reverse direction.
                    checkClassAssign(nodep, "Function Argument", pinp, portDTypep,
                                     portp->direction() == VDirection::OUTPUT);
                    userIterate(pinp, WidthVP{portDTypep, FINAL, STREAM_USE_ASSIGN}.p());
                } નહીંતર {
                    iterateCheckAssign(nodep, "Function Argument", pinp, FINAL, portDTypep,
                                       portp->direction().pinAccess());
                }
            }
        }
    }

    શૂન્ય handleStdRandomizeArgs(Ast_ગાંઠ_FTask_Ref* અચલ nodep) {
        AstConst* nullp = nullptr;
        માટે (AstArg *argp = nodep->argsp(), *nextp; argp; argp = nextp) {
            nextp = VN_AS(argp->nextp(), Arg);
            Ast_ગાંઠ_પદાવલિ* અચલ exprp = argp->exprp();
            જો (AstConst* અચલ constp = VN_CAST(exprp, Const)) {
                જો (constp->અંક().isNull()) {
                    nullp = constp;
                    ચાલુ;
                }
            }
            જો (VN_IS(exprp, MemberSel)) {
                // Non-standard usage: std::randomize() with class-scoped member
                // IEEE 1800-2023 (18.12) limits args to current scope variables.
                // Verilator accepts this for compatibility with other simulators.
                ચાલુ;
            }
            જો (VN_IS(exprp, VarRef) || VN_IS(exprp, ArraySel) || VN_IS(exprp, StructSel)) {
                // Valid usage
                ચાલુ;
            }
            જો (અચલ AstCMethodHard* અચલ methodp = VN_CAST(exprp, CMethodHard)) {
                જો (methodp->method() == VCMethod::ARRAY_AT
                    || methodp->method() == VCMethod::ARRAY_AT_WRITE) {
                    ચાલુ;
                }
            }
            argp->v3error("Non-variable arguments for 'std::randomize()'.");
        }
        જો (nullp) nullp->v3error("'std::randomize()' does not accept 'null' as arguments.");
    }
    શૂન્ય visit(Ast_ગાંઠ_FTask_Ref* nodep) override {
        // For arguments, is assignment-like context; see IEEE rules in AstNodeAssign
        // Function hasn't been widthed, so make it so.
        જો (nodep->didWidth()) પાછા;
        UINFO(5, "  FTASKREF " << nodep);
        AstWith* withp = nullptr;
        જો (nodep->name() == "rand_mode" || nodep->name() == "constraint_mode") {
            v3Global.useRandomizeMethods(સાચું);
            nodep->dtypep(nodep->findBasicDType(VBasicDTypeKwd::INT));
            પાછા;  // Handled in V3Randomize
        } નહીંતર જો (nodep->name() == "randomize" || nodep->name() == "srandom"
                   || (!nodep->taskp()
                       && (nodep->name() == "get_randstate"
                           || nodep->name() == "set_randstate"))) {
            // TODO perhaps this should move to V3LinkDot
            AstClass* અચલ classp = VN_CAST(nodep->classOrPackagep(), Class);
            જો (nodep->classOrPackagep() && nodep->classOrPackagep()->name() == "std") {
                v3Global.useRandomizeMethods(સાચું);
                Ast_ગાંઠ_DType* અચલ adtypep = nodep->findBitDType();
                withp = methodWithClause(nodep, ખોટું, ખોટું, adtypep->findVoidDType(),
                                         adtypep->findBitDType(), adtypep);
                માટે (અચલ AstArg* argp = nodep->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) {
                    userIterateAndNext(argp->exprp(), WidthVP{SELF, BOTH}.p());
                }
                handleStdRandomizeArgs(nodep);  // Provided args should be in current scope
                processFTaskRefArgs(nodep);
                જો (withp) nodep->withp(withp);
                nodep->didWidth(સાચું);
                પાછા;
            }
            UASSERT_OBJ(classp, nodep, "Classless rand-thing should have failed in V3LinkDot");
            જો (nodep->name() == "randomize") {
                AstClassRefDType* અચલ adtypep
                    = નવું AstClassRefDType{nodep->fileline(), classp, nullptr};
                v3Global.rootp()->typeTablep()->addTypesp(adtypep);
                withp = methodWithClause(nodep, ખોટું, ખોટું, adtypep->findVoidDType(),
                                         adtypep->findBitDType(), adtypep);
                માટે (અચલ AstArg* argp = nodep->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) {
                    userIterateAndNext(argp->exprp(), WidthVP{SELF, BOTH}.p());
                }
                handleRandomizeArgs(nodep, classp);
            } નહીંતર જો (nodep->name() == "srandom") {
                nodep->taskp(V3Randomize::newSRandomFunc(m_memberMap, classp));
                m_memberMap.clear();
            } નહીંતર જો (nodep->name() == "get_randstate") {
                methodOkArguments(nodep, 0, 0);
                classp->baseMostClassp()->needRNG(સાચું);
                v3Global.useRandomizeMethods(સાચું);
                AstCExpr* અચલ newp
                    = નવું AstCExpr{nodep->fileline(), "__Vm_rng.get_randstate()", 1};
                newp->dtypeSetString();
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
                પાછા;
            } નહીંતર જો (nodep->name() == "set_randstate") {
                methodOkArguments(nodep, 1, 1);
                Ast_ગાંઠ_પદાવલિ* અચલ expr1p = nodep->argsp()->exprp();  // May edit
                iterateCheckString(nodep, "LHS", expr1p, BOTH);
                Ast_ગાંઠ_પદાવલિ* અચલ exprp = nodep->argsp()->exprp();
                classp->baseMostClassp()->needRNG(સાચું);
                v3Global.useRandomizeMethods(સાચું);
                AstCExpr* અચલ newp
                    = નવું AstCExpr{nodep->fileline(), "__Vm_rng.set_randstate(", 1};
                newp->add(exprp->unlinkFrBack());
                newp->add(")");
                newp->dtypeSetVoid();
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
                પાછા;
            } નહીંતર {
                UASSERT_OBJ(ખોટું, nodep, "Bad case");
            }
        }
        UASSERT_OBJ(nodep->taskp(), nodep, "Unlinked");
        જો (nodep->didWidth()) {
            જો (withp) nodep->withp(withp);
            પાછા;
        }
        જો ((nodep->taskp()->classMethod() && !nodep->taskp()->isStatic())
            && !VN_IS(m_procedurep, InitialAutomatic) && !m_constraintp) {
            તાર્કિક allow = ખોટું;
            જો (m_ftaskp && m_ftaskp->classMethod() && !m_ftaskp->isStatic()) {
                જો (અચલ AstFuncRef* અચલ funcRefp = VN_CAST(nodep, FuncRef)) {
                    allow = funcRefp->superReference();
                } નહીંતર જો (અચલ AstTaskRef* અચલ taskRefp = VN_CAST(nodep, TaskRef)) {
                    allow = taskRefp->superReference();
                }
                જો (!allow) {
                    અચલ AstClass* callerClassp = m_containingClassFinder.find(m_ftaskp);
                    જો (!callerClassp) {
                        callerClassp = m_containingClassFinder.find(m_ftaskp->classOrPackagep());
                    }
                    અચલ AstClass* calleeClassp = VN_CAST(nodep->classOrPackagep(), Class);
                    જો (!calleeClassp) {
                        calleeClassp = m_containingClassFinder.find(nodep->taskp());
                    }
                    allow = AstClass::isClassExtendedFrom(callerClassp, calleeClassp);
                }
            }
            જો (!allow) {
                // An embedded covergroup may call methods of its enclosing class
                // (IEEE 1800-2023 19.4); V3Covergroup routes the call through its handle.
                અચલ AstClass* અચલ cgClassp = m_containingClassFinder.find(nodep);
                જો (cgClassp && cgClassp->covergroupEnclosingClassp()) {
                    અચલ AstClass* calleeClassp = VN_CAST(nodep->classOrPackagep(), Class);
                    જો (!calleeClassp) {
                        calleeClassp = m_containingClassFinder.find(nodep->taskp());
                    }
                    allow = AstClass::isClassExtendedFrom(cgClassp->covergroupEnclosingClassp(),
                                                          calleeClassp);
                }
            }
            જો (!allow) {
                nodep->v3error("Cannot call non-static member function "
                               << nodep->prettyNameQ() << " without object (IEEE 1800-2023 8.10)");
            }
        }
        જો (nodep->taskp() && !nodep->scopeNamep()
            && (nodep->taskp()->dpiContext() || nodep->taskp()->dpiExport())) {
            nodep->scopeNamep(નવું AstScopeName{nodep->fileline(), ખોટું});
        }
        // And do the arguments to the task/function too
        processFTaskRefArgs(nodep);
        જો (withp) nodep->withp(withp);
        nodep->didWidth(સાચું);
        // See steps that follow in visit(AstFuncRef*)
    }
    શૂન્ય visit(Ast_ગાંઠ_Procedure* nodep) override {
        assertAtStatement(nodep);
        VL_RESTORER(m_procedurep);
        m_procedurep = nodep;
        userIterateChildren(nodep, nullptr);
    }
    શૂન્ય visit(AstSenItem* nodep) override {
        જો (nodep->isComboStar()) પાછા;
        UASSERT_OBJ(nodep->isClocked(), nodep, "Invalid edge");
        // Optimize concat/replicate senitems; this has to be done here at the latest, otherwise we
        // emit WIDTHCONCAT if there are unsized constants
        જો (VN_IS(nodep->sensp(), Concat) || VN_IS(nodep->sensp(), Replicate)) {
            auto* અચલ concatOrReplp = VN_CAST(nodep->sensp(), ગાંઠ_Biop);
            auto* અચલ rhsp = concatOrReplp->rhsp()->unlinkFrBack();
            જો (nodep->edgeType() == VEdgeType::ET_CHANGED) {
                // If it's ET_CHANGED, split concatenations into multiple senitems
                auto* અચલ lhsp = concatOrReplp->lhsp()->unlinkFrBack();
                nodep->addNextHere(નવું AstSenItem{lhsp->fileline(), nodep->edgeType(), lhsp});
            }  // Else only use the RHS
            nodep->replaceWith(નવું AstSenItem{rhsp->fileline(), nodep->edgeType(), rhsp});
            VL_DO_DANGLING(nodep->delete_વૃક્ષ(), nodep);
        } નહીંતર {
            userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
            જો (nodep->edgeType().anEdge()) {
                Ast_ગાંઠ_DType* અચલ sensDtp = nodep->sensp()->dtypep()->skipRefp();
                જો (sensDtp->isDouble()) {
                    nodep->sensp()->v3error(
                        "Edge event control not legal on real type (IEEE 1800-2023 6.12.1)");
                } નહીંતર જો (sensDtp->basicp() && !sensDtp->basicp()->keyword().isIntNumeric()) {
                    nodep->sensp()->v3error("Edge event control not legal on non-integral type "
                                            "(IEEE 1800-2023 9.4.2)");
                }
            }
        }
    }
    શૂન્ય visit(AstClockingItem* nodep) override {
        nodep->exprp()->foreach(
            [nodep](AstVarRef* અચલ refp) { refp->access(nodep->direction().pinAccess()); });
        userIterateChildren(nodep, WidthVP{SELF, PRELIM}.p());
    }
    શૂન્ય visit(AstWait* nodep) override {
        જો (!m_underFork && VN_IS(m_ftaskp, Func)) {
            nodep->v3error("Wait statements are not legal in functions. Suggest use a task "
                           "(IEEE 1800-2023 13.4.4)");
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_વૃક્ષ(), nodep);
            પાછા;
        }
        જો (nodep->fileline()->timingOn()) {
            જો (v3Global.opt.timing().isSetTrue()) {
                iterateCheckBool(nodep, "Wait", nodep->condp(),
                                 BOTH);  // it's like an if() condition.
                // TODO check also inside complex event expressions
                જો (Ast_ગાંઠ_Var_Ref* અચલ varrefp = VN_CAST(nodep->condp(), ગાંઠ_Var_Ref)) {
                    જો (varrefp->isEvent()) {
                        varrefp->v3error("Wait statement conditions do not take raw events"
                                         " (IEEE 1800-2023 15.5.3)\n"
                                         << varrefp->warnMore() << "... Suggest use '"
                                         << varrefp->prettyName() << ".triggered'");
                    }
                }
                iterateNull(nodep->stmtsp());
                પાછા;
            } નહીંતર જો (v3Global.opt.timing().isSetFalse()) {
                nodep->v3warn(E_NOTIMING, "Wait statements require --timing");
            } નહીંતર {
                nodep->v3warn(E_NEEDTIMINGOPT, "Use --timing or --no-timing to specify how wait "
                                               "statements should be handled");
            }
        }
        // If we ignore timing:
        // Statements we'll just execute immediately; equivalent to if they followed this
        જો (Ast_ગાંઠ* અચલ stmtsp = nodep->stmtsp()) {
            stmtsp->unlinkFrBackWithNext();
            nodep->replaceWith(stmtsp);
        } નહીંતર {
            nodep->unlinkFrBack();
        }
        VL_DO_DANGLING(nodep->delete_વૃક્ષ(), nodep);
    }
    શૂન્ય visit(AstWith* nodep) override {
        // Should otherwise be underneath a method call
        Ast_ગાંઠ_DType* અચલ vdtypep = m_vup->dtypeNullSkipRefp();
        VL_RESTORER(m_withp);
        m_withp = nodep;
        userIterateChildren(nodep->indexArgRefp(), nullptr);
        userIterateChildren(nodep->કિંમત_Arg_Refp(), nullptr);
        જો (!nodep->exprp()) {
            nodep->dtypeSetVoid();
        } નહીંતર {
            જો (!nodep->exprp()->hasDType()) {
                userIterateAndNext(nodep->exprp(), nullptr);
            } નહીંતર {
                જો (vdtypep) {
                    userIterateAndNext(nodep->exprp(), WidthVP{nodep->dtypep(), PRELIM}.p());
                } નહીંતર {  // 'sort with' allows arbitrary type
                    userIterateAndNext(nodep->exprp(), WidthVP{SELF, PRELIM}.p());
                }
            }

            જો (!nodep->exprp()->hasDType()) {
                nodep->dtypeSetVoid();
            } નહીંતર {
                nodep->dtypeFrom(nodep->exprp());
                iterateCheckAssign(nodep, "'with' return value", nodep->exprp(), FINAL,
                                   nodep->dtypep());
            }
        }
    }
    શૂન્ય visit(AstLambdaArgRef* nodep) override {
        UASSERT_OBJ(m_withp, nodep, "LambdaArgRef not underneath 'with' lambda");
        જો (nodep->index()) {
            nodep->dtypeFrom(m_withp->indexArgRefp());
        } નહીંતર {
            nodep->dtypeFrom(m_withp->કિંમત_Arg_Refp());
        }
    }
    શૂન્ય visit(AstNetlist* nodep) override {
        // Iterate modules backwards, in bottom-up order.  That's faster
        userIterateChildrenBackwardsConst(nodep, nullptr);
    }

    //--------------------
    // Default
    શૂન્ય visit(Ast_ગાંઠ_પદાવલિ* nodep) override {
        જો (!nodep->didWidth()) {
            nodep->v3fatalSrc(
                "Visit function missing? Widthed function missing for math node: " << nodep);
        }
        userIterateChildren(nodep, nullptr);
    }
    શૂન્ય visit(AstResizeLValue* nodep) override {
        // RESIZELVALUE adjusts width of lvalues for assignments/function calls
        // The parent context determines the required width
        UINFO(9, "visit AstResizeLValue " << nodep << endl);
        જો (nodep->didWidthAndSet()) પાછા;

        UASSERT_OBJ(nodep->lhsp(), nodep, "ResizeLValue missing lhs expression");

        // First, process child to know its natural width
        જો (m_vup->prelim()) {
            userIterateAndNext(nodep->lhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
        }

        // Get the required width from parent context
        જો (Ast_ગાંઠ_DType* અચલ vdtypep = m_vup->dtypeNullp()) {
            // Parent specified required width - use it
            nodep->dtypeFrom(vdtypep);
        } નહીંતર જો (!nodep->dtypep()) {
            // No parent context, use child's type
            nodep->dtypeFrom(nodep->lhsp());
        }

        // Verify we ended up with a valid datatype
        UASSERT_OBJ(nodep->dtypep(), nodep, "ResizeLValue still missing dtype after visiting");
        UASSERT_OBJ(nodep->width() > 0, nodep, "ResizeLValue has invalid width");

        // Log the transformation for debugging
        UINFO(9, "  ResizeLValue: " << nodep->lhsp()->width() << " bits -> " << nodep->width()
                                    << " bits" << endl);

        // Final processing
        userIterateAndNext(nodep->lhsp(), WidthVP{SELF, FINAL}.p());
        nodep->didWidth(સાચું);
    }
    શૂન્ય visitClass(AstClass* nodep) {
        જો (nodep->didWidthAndSet()) પાછા;

        // If the class is std::process
        જો (nodep->name() == "process") {
            AstPackage* અચલ packagep = getItemPackage(nodep);
            જો (packagep && packagep->name() == "std") {
                // Change type of m_process to VlProcessRef
                જો (AstVar* અચલ varp
                    = VN_CAST(m_memberMap.findMember(nodep, "m_process"), Var)) {
                    Ast_ગાંઠ_DType* અચલ dtypep = varp->getChildDTypep();
                    જો (!varp->dtypep()) {
                        VL_DO_DANGLING(ધકેલો_Deletep(dtypep->unlinkFrBack()), dtypep);
                    }
                    AstBasicDType* અચલ newdtypep = નવું AstBasicDType{
                        nodep->fileline(), VBasicDTypeKwd::PROCESS_REFERENCE, VSigning::UNSIGNED};
                    v3Global.rootp()->typeTablep()->addTypesp(newdtypep);
                    varp->dtypep(newdtypep);
                }
                // Mark that self requires process instance
                જો (Ast_ગાંઠ_FTask* અચલ ftaskp
                    = VN_CAST(m_memberMap.findMember(nodep, "self"), ગાંઠ_FTask)) {
                    ftaskp->setNeedProcess();
                    v3Global.setUsesTiming();
                }
            }
        }

        // Must do extends first, as we may in functions under this class
        // start following a tree of extends that takes us to other classes
        userIterateAndNext(nodep->extendsp(), nullptr);
        userIterateChildren(nodep, nullptr);  // First size all members
    }
    શૂન્ય visit(Ast_ગાંઠ_Module* nodep) override {
        assertAtStatement(nodep);
        VL_RESTORER_COPY(m_enumTempNames);
        VL_RESTORER_COPY(m_insideTempNames);
        જો (AstClass* અચલ classp = VN_CAST(nodep, Class)) {
            visitClass(classp);
        } નહીંતર {
            VL_RESTORER(m_modep);
            m_modep = nodep;
            // Collect local variables for ASSIGNIN hierarchical reference check
            m_curModVars.clear();
            nodep->foreach([આ](અચલ AstVar* varp) { m_curModVars.ઉમેરો(varp); });
            userIterateChildren(nodep, nullptr);
        }
    }
    શૂન્ય visit(Ast_ગાંઠ* nodep) override {
        // Default: Just iterate
        UASSERT_OBJ(!m_vup, nodep,
                    "Visit function missing? Widthed expectation for this node: " << nodep);
        userIterateChildren(nodep, nullptr);
    }

    //----------------------------------------------------------------------
    // WIDTH METHODs -- all iterate

    શૂન્ય visit_Or_Lu64(Ast_ગાંઠ_Uniop* nodep) {
        // CALLER: AstBitsToRealD
        // Real: Output real
        // LHS presumed self-determined, then coerced to real
        assert_At_પદાવલિ(nodep);
        જો (m_vup->prelim()) {  // First stage evaluation
            nodep->dtypeSetDouble();
            Ast_ગાંઠ_DType* અચલ subDTypep = nodep->findBitDType(64, 64, VSigning::UNSIGNED);
            // Self-determined operand
            userIterateAndNext(nodep->lhsp(), WidthVP{SELF, PRELIM}.p());
            iterateCheck(nodep, "LHS", nodep->lhsp(), SELF, FINAL, subDTypep, EXTEND_EXP);
        }
    }
    શૂન્ય visit(AstIToRD* nodep) override {
        // Real: Output real
        // LHS presumed self-determined, then coerced to real
        assert_At_પદાવલિ(nodep);
        જો (m_vup->prelim()) {  // First stage evaluation
            nodep->dtypeSetDouble();
            // Self-determined operand (TODO check if numeric type)
            userIterateAndNext(nodep->lhsp(), WidthVP{SELF, PRELIM}.p());
            જો (nodep->lhsp()->isSigned()) {
                nodep->replaceWith(
                    નવું AstISToRD{nodep->fileline(), nodep->lhsp()->unlinkFrBack()});
                VL_DO_DANGLING(nodep->delete_વૃક્ષ(), nodep);
            }
        }
    }
    શૂન્ય visit(AstISToRD* nodep) override {
        // Real: Output real
        // LHS presumed self-determined, then coerced to real
        assert_At_પદાવલિ(nodep);
        જો (m_vup->prelim()) {  // First stage evaluation
            nodep->dtypeSetDouble();
            // Self-determined operand (TODO check if numeric type)
            userIterateAndNext(nodep->lhsp(), WidthVP{SELF, PRELIM}.p());
        }
    }
    શૂન્ય visit_Os32_Lr(Ast_ગાંઠ_Uniop* nodep) {
        // CALLER: RToI
        // Real: LHS real
        // LHS presumed self-determined, then coerced to real
        assert_At_પદાવલિ(nodep);
        જો (m_vup->prelim()) {  // First stage evaluation
            iterateCheckReal(nodep, "LHS", nodep->lhsp(), BOTH);
            nodep->dtypeSetInteger2State();
        }
    }
    શૂન્ય visit_Ou64_Lr(Ast_ગાંઠ_Uniop* nodep) {
        // CALLER: RealToBits
        // Real: LHS real
        // LHS presumed self-determined, then coerced to real
        assert_At_પદાવલિ(nodep);
        જો (m_vup->prelim()) {  // First stage evaluation
            iterateCheckReal(nodep, "LHS", nodep->lhsp(), BOTH);
            nodep->dtypeSetUInt64();
        }
    }

    શૂન્ય visit_log_not(AstLogNot* nodep) {
        // CALLER: LogNot
        // Width-check: lhs 1 bit
        // Real: Allowed; implicitly compares with zero
        // We calculate the width of the UNDER expression.
        // We then check its width to see if it's legal, and edit if not
        // We finally set the width of our output
        // IEEE-2012: Table 11-21 and 11.8.1 (same as RedAnd):
        //   LHS is self-determined
        //   Width: 1 bit out
        //   Sign: unsigned out (11.8.1)
        UASSERT_OBJ(!nodep->op2p(), nodep, "For unary ops only!");
        assert_At_પદાવલિ(nodep);
        જો (m_vup->prelim()) {
            iterateCheckBool(nodep, "LHS", nodep->op1p(), BOTH);
            nodep->dtypeSetBitOrLogic(nodep->lhsp()->isFourstateOrNoDtype());
            // IEEE 1800-2023 16.12.3: property 'not' is not a sequence operator.
            // Boolean '!' is allowed in sequences (16.7 expression_or_dist).
            // The parser distinguishes the two via AstLogNot::fromProperty().
            જો (m_underSExpr && nodep->fromProperty()) {
                nodep->v3error("Unexpected 'not' in sequence expression context");
                AstConst* અચલ newp
                    = નવું AstConst{nodep->fileline(), AstConst::BitFalseErroring{}};
                newp->dtypeFrom(nodep);
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
            }
        }
    }
    શૂન્ય visit_log_and_or(Ast_ગાંઠ_Biop* nodep) {
        // CALLER: LogAnd, LogOr, LogEq, LogIf
        // Widths: 1 bit out, lhs 1 bit, rhs 1 bit
        // IEEE-2012 Table 11-21:
        //   LHS is self-determined
        //   RHS is self-determined
        assert_At_પદાવલિ(nodep);
        જો (m_vup->prelim()) {
            iterateCheckBool(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckBool(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtypeSetBitOrLogic(
                Ast_ગાંઠ_પદાવલિ::isFourstateOrNoDtype(nodep->lhsp(), nodep->rhsp()));
        }
    }
    શૂન્ય visitAbortProp(AstAbortOn* nodep) {
        // IEEE 1800-2023 16.12.14: abort condition is a 1-bit self-determined
        // Boolean; property subexpression carries its own type checking.
        // VAbortKind distinguishes accept/reject and sync/async, but the
        // type-check path is identical for all four.
        assert_At_પદાવલિ(nodep);
        જો (m_vup->prelim()) {
            iterateCheckBool(nodep, "cond", nodep->condp(), BOTH);
            iterateCheckBool(nodep, "prop", nodep->propp(), BOTH);
            nodep->dtypeSetBit();
        }
    }
    શૂન્ય visit_red_and_or(Ast_ગાંઠ_Uniop* nodep) {
        // CALLER: RedAnd, RedOr, ...
        // Signed: Output unsigned, Lhs/Rhs/etc non-real (presumed, not in IEEE)
        // IEEE-2012: Table 11-21 and 11.8.1:
        //   LHS is self-determined
        //   Width: 1 bit out
        //   Sign: unsigned out (11.8.1)
        assert_At_પદાવલિ(nodep);
        જો (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->lhsp(), SELF, BOTH);
            nodep->dtypeSetBitOrLogic(nodep->lhsp()->isFourstateOrNoDtype());
        }
    }
    શૂન્ય visit_red_unknown(Ast_ગાંઠ_Uniop* nodep) {
        // CALLER: IsUnknown
        // Signed: Output unsigned, Lhs/Rhs/etc non-real (presumed, not in IEEE)
        // IEEE-2012: Table 11-21 and 11.8.1:
        //   LHS is self-determined
        //   Width: 1 bit out
        //   Sign: unsigned out (11.8.1)
        assert_At_પદાવલિ(nodep);
        જો (m_vup->prelim()) {
            userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
            nodep->dtypeSetBit();
        }
    }

    // IEEE 1800-2023 6.22.2 Equivalent types
    તાર્કિક isEquivalentDType(અચલ Ast_ગાંઠ_DType* lhs, અચલ Ast_ગાંઠ_DType* rhs) {
        // a) If two types match, they are equivalent.
        જો (!lhs || !rhs) પાછા ખોટું;
        lhs = lhs->skipRefToEnump();
        rhs = rhs->skipRefToEnump();
        જો (lhs == rhs) પાછા સાચું;
        // If both are basic types, check if they are the same type
        જો (VN_IS(lhs, BasicDType) && VN_IS(rhs, BasicDType)) {
            અચલ auto* lb = VN_CAST(lhs, BasicDType);
            અચલ auto* rb = VN_CAST(rhs, BasicDType);
            જો (lb->isString() != rb->isString()) પાછા ખોટું;
        }

        // d) Unpacked fixed-size array types are equivalent if they have equivalent element types
        // and equal size; the actual range bounds may differ. Note that the element type of a
        // multidimensional array is itself an array type.
        અચલ તાર્કિક lhsIsUnpackArray = VN_IS(lhs, UnpackArrayDType);
        અચલ તાર્કિક rhsIsUnpackArray = VN_IS(rhs, UnpackArrayDType);
        જો (lhsIsUnpackArray || rhsIsUnpackArray) {
            જો (lhsIsUnpackArray && rhsIsUnpackArray) {
                અચલ AstUnpackArrayDType* અચલ lhsp = VN_CAST(lhs, UnpackArrayDType);
                અચલ AstUnpackArrayDType* અચલ rhsp = VN_CAST(rhs, UnpackArrayDType);
                અચલ પૂર્ણાંક lsz = lhsp->elementsConst();
                અચલ પૂર્ણાંક rsz = rhsp->elementsConst();
                જો (lsz >= 0 && rsz >= 0 && lsz != rsz) પાછા ખોટું;
                પાછા isEquivalentDType(lhsp->subDTypep(), rhsp->subDTypep());
            }
            પાછા ખોટું;
        }

        // e) Dynamic array, associative array, and queue types are equivalent if they are the same
        // kind of array (dynamic, associative, or queue), have equivalent index types (for
        // associative arrays), and have equivalent element types.
        અચલ તાર્કિક lhsIsDynArray = VN_IS(lhs, DynArrayDType);
        અચલ તાર્કિક rhsIsDynArray = VN_IS(rhs, DynArrayDType);
        અચલ તાર્કિક lhsIsQueue = VN_IS(lhs, QueueDType);
        અચલ તાર્કિક rhsIsQueue = VN_IS(rhs, QueueDType);
        અચલ તાર્કિક lhsIsAssocArray = VN_IS(lhs, AssocArrayDType);
        અચલ તાર્કિક rhsIsAssocArray = VN_IS(rhs, AssocArrayDType);

        જો (lhsIsDynArray || rhsIsDynArray || lhsIsQueue || rhsIsQueue || lhsIsAssocArray
            || rhsIsAssocArray) {
            જો (અચલ AstDynArrayDType* અચલ lhsp = VN_CAST(lhs, DynArrayDType)) {
                જો (અચલ AstDynArrayDType* અચલ rhsp = VN_CAST(rhs, DynArrayDType)) {
                    પાછા isEquivalentDType(lhsp->subDTypep(), rhsp->subDTypep());
                }
            }

            જો (અચલ AstQueueDType* અચલ lhsp = VN_CAST(lhs, QueueDType)) {
                જો (અચલ AstQueueDType* અચલ rhsp = VN_CAST(rhs, QueueDType)) {
                    પાછા isEquivalentDType(lhsp->subDTypep(), rhsp->subDTypep());
                }
            }

            જો (અચલ AstAssocArrayDType* અચલ lhsp = VN_CAST(lhs, AssocArrayDType)) {
                જો (અચલ AstAssocArrayDType* અચલ rhsp = VN_CAST(rhs, AssocArrayDType)) {
                    પાછા isEquivalentDType(lhsp->subDTypep(), rhsp->subDTypep())
                           && isEquivalentDType(lhsp->ચાવી_DTypep(), rhsp->ચાવી_DTypep());
                }
            }

            પાછા ખોટું;
        }

        // c) Packed arrays, packed structures, packed unions, and built-in integral
        // types are equivalent if they contain the same number of total bits, are either all
        // 2-state or all 4-state, and are either all signed or all unsigned.
        જો (lhs->isIntegralOrPacked() && rhs->isIntegralOrPacked()) {
            જો (lhs->width() != rhs->width()) પાછા ખોટું;
            જો (lhs->isFourstate() != rhs->isFourstate()) પાછા ખોટું;
            જો (lhs->isSigned() != rhs->isSigned()) પાછા ખોટું;
            પાછા સાચું;
        }

        // b) An anonymous enum, unpacked struct, or unpacked union type is equivalent to itself
        // among data objects declared within the same declaration statement and no other data
        // types.

        // DTypes compared above, if any remained then not equivalent.
        જો (VN_IS(lhs, EnumDType) || VN_IS(rhs, EnumDType)) પાછા ખોટું;
        પાછા !(VN_IS(lhs, ગાંઠ_UOr_Struct_DType) || VN_IS(rhs, ગાંઠ_UOr_Struct_DType));
    }

    સ્થિર તાર્કિક isAggregateType(અચલ Ast_ગાંઠ* nodep) {
        જો (!nodep) પાછા ખોટું;
        અચલ Ast_ગાંઠ_DType* dtypep = nodep->dtypep();
        જો (!dtypep) પાછા ખોટું;
        dtypep = dtypep->skipRefp();
        જો (!dtypep) પાછા ખોટું;
        પાછા dtypep->isAggregateType();
    }

    શૂન્ય visit_cmp_eq_gt(Ast_ગાંઠ_Biop* nodep, અચલ ComparisonType cmpType) {
        // CALLER: AstEq, AstGt, ..., AstLtS
        // Real allowed if and only if real_lhs set
        // See IEEE-2012 11.4.4, and 11.8.1:
        //   Widths: 1 bit out, width is max of LHS or RHS
        //   Sign:  signed compare (not output) if both signed, compare is signed,
        //             width mismatches sign extend
        //             else, compare is unsigned, **zero-extends**
        //   Real:  If either real, other side becomes real and real compare
        //   TODO: chandle/class handle/iface handle: WildEq/WildNeq same as Eq/Neq
        //   TODO: chandle/class handle/iface handle only allowed to self-compare or against null
        //   TODO: chandle/class handle/iface handle no relational compares
        UASSERT_OBJ(nodep->rhsp(), nodep, "For binary ops only!");
        assert_At_પદાવલિ(nodep);
        જો (m_vup->prelim()) {
            // Assignment patterns cannot self-determine their type.
            // Iterate the non-pattern operand first so its dtype is available,
            // then propagate that dtype to the pattern operand (analogous to
            // how assignments pass LHS dtype to RHS patterns).
            અચલ તાર્કિક lhsIsPat = VN_IS(nodep->lhsp(), Pattern);
            અચલ તાર્કિક rhsIsPat = VN_IS(nodep->rhsp(), Pattern);
            જો (lhsIsPat && !rhsIsPat) {
                userIterateAndNext(nodep->rhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
                userIterateAndNext(nodep->lhsp(), WidthVP{nodep->rhsp()->dtypep(), PRELIM}.p());
            } નહીંતર જો (rhsIsPat && !lhsIsPat) {
                userIterateAndNext(nodep->lhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
                userIterateAndNext(nodep->rhsp(), WidthVP{nodep->lhsp()->dtypep(), PRELIM}.p());
            } નહીંતર {
                userIterateAndNext(nodep->lhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
                userIterateAndNext(nodep->rhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            }

            અચલ તાર્કિક isAggrLhs = isAggregateType(nodep->lhsp());
            અચલ તાર્કિક isAggrRhs = isAggregateType(nodep->rhsp());

            જો ((isAggrLhs || isAggrRhs)
                && (cmpType == ComparisonType::NORMAL_EQ || cmpType == ComparisonType::CASE)) {
                અચલ Ast_ગાંઠ_DType* અચલ lhsDType = nodep->lhsp()->dtypep();
                અચલ Ast_ગાંઠ_DType* અચલ rhsDType = nodep->rhsp()->dtypep();

                જો (lhsDType && rhsDType && !isEquivalentDType(lhsDType, rhsDType)) {
                    nodep->v3error("Comparison requires matching data types"
                                   << warnDTypes(nodep, lhsDType, rhsDType));
                    Ast_ગાંઠ* અચલ newp
                        = નવું AstConst{nodep->fileline(), AstConst::BitFalseErroring{}};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
                    પાછા;
                }
                // Elements are compared by their left-to-right order, as when assigned (7.4.6)
                convertUnpackedOrder(nodep->rhsp(), nodep->lhsp()->dtypep());
            } નહીંતર જો (nodep->lhsp()->isDouble() || nodep->rhsp()->isDouble()) {
                જો (cmpType == ComparisonType::WILD) {
                    nodep->v3error("Real is illegal operand to ?== operator");
                    Ast_ગાંઠ* અચલ newp
                        = નવું AstConst{nodep->fileline(), AstConst::BitFalseErroring{}};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
                    પાછા;
                }
                જો (Ast_ગાંઠ_Biop* અચલ newp = replaceWithDVersion(nodep)) {
                    VL_DANGLING(nodep);
                    nodep = newp;  // Process new node instead
                    iterateCheckReal(nodep, "LHS", nodep->lhsp(), FINAL);
                    iterateCheckReal(nodep, "RHS", nodep->rhsp(), FINAL);
                }
            } નહીંતર જો (nodep->lhsp()->isString() || nodep->rhsp()->isString()) {
                જો (Ast_ગાંઠ_Biop* અચલ newp = replaceWithNVersion(nodep)) {
                    VL_DANGLING(nodep);
                    nodep = newp;  // Process new node instead
                    iterateCheckString(nodep, "LHS", nodep->lhsp(), FINAL);
                    iterateCheckString(nodep, "RHS", nodep->rhsp(), FINAL);
                }
            } નહીંતર {
                અચલ તાર્કિક signedFl = nodep->lhsp()->isSigned() && nodep->rhsp()->isSigned();
                જો (Ast_ગાંઠ_Biop* અચલ newp = replaceWithUOrSVersion(nodep, signedFl)) {
                    VL_DANGLING(nodep);
                    nodep = newp;  // Process new node instead
                }
                અચલ પૂર્ણાંક width = std::max(nodep->lhsp()->width(), nodep->rhsp()->width());
                અચલ પૂર્ણાંક ewidth = std::max(nodep->lhsp()->widthMin(), nodep->rhsp()->widthMin());
                તાર્કિક warnOn = સાચું;
                જો (!signedFl && width == 32) {
                    // Waive on unsigned < or <= if RHS is narrower, since can't give wrong answer
                    જો ((VN_IS(nodep, Lt) || VN_IS(nodep, Lte))
                        && (nodep->lhsp()->width() >= nodep->rhsp()->widthMin())) {
                        warnOn = ખોટું;
                    }
                    // Waive on unsigned > or >= if RHS is wider, since can't give wrong answer
                    જો ((VN_IS(nodep, Gt) || VN_IS(nodep, Gte))
                        && (nodep->lhsp()->widthMin() >= nodep->rhsp()->width())) {
                        warnOn = ખોટું;
                    }
                }
                iterateCheck(nodep, "LHS", nodep->lhsp(), CONTEXT_DET, FINAL,
                             nodep->findBitOrLogicDType(width, ewidth,
                                                        VSigning::fromBool(signedFl),
                                                        nodep->lhsp()->isFourstateOrNoDtype()),
                             (signedFl ? EXTEND_LHS : EXTEND_ZERO), warnOn);
                iterateCheck(nodep, "RHS", nodep->rhsp(), CONTEXT_DET, FINAL,
                             nodep->findBitOrLogicDType(width, ewidth,
                                                        VSigning::fromBool(signedFl),
                                                        nodep->rhsp()->isFourstateOrNoDtype()),
                             (signedFl ? EXTEND_LHS : EXTEND_ZERO), warnOn);
            }
            nodep->dtypeSetBitOrLogic(cmpType != ComparisonType::CASE
                                      && (nodep->lhsp()->isFourstateOrNoDtype()
                                          || ((cmpType == ComparisonType::NORMAL_EQ
                                               || cmpType == ComparisonType::NORMAL_GL)
                                              && nodep->rhsp()->isFourstateOrNoDtype())));
        }
    }
    શૂન્ય visit_cmp_real(Ast_ગાંઠ_Biop* nodep) {
        // CALLER: EqD, LtD
        // Widths: 1 bit out, lhs width == rhs width
        // Signed compare (not output) if both sides signed
        // Real if and only if real_allow set
        // IEEE, 11.4.4: relational compares (<,>,<=,>=,==,===,!=,!==) use
        // "zero padding" on unsigned
        UASSERT_OBJ(nodep->rhsp(), nodep, "For binary ops only!");
        assert_At_પદાવલિ(nodep);
        જો (m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckReal(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckReal(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtypeSetBit();
        }
    }
    શૂન્ય visit_cmp_string(Ast_ગાંઠ_Biop* nodep) {
        // CALLER: EqN, LtN
        // Widths: 1 bit out, lhs width == rhs width
        // String compare (not output)
        // Real if and only if real_allow set
        UASSERT_OBJ(nodep->rhsp(), nodep, "For binary ops only!");
        assert_At_પદાવલિ(nodep);
        જો (m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckString(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtypeSetBit();
        }
    }
    શૂન્ય visit_cmp_type(Ast_ગાંઠ_Biop* nodep) {
        // CALLER: EqT, LtT
        // Widths: 1 bit out
        // Data type compare (not output)
        assert_At_પદાવલિ(nodep);
        જો (m_vup->prelim()) {
            userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
            userIterateAndNext(nodep->rhsp(), WidthVP{SELF, BOTH}.p());
            અચલ AstAttrOf* અચલ lhsap = VN_AS(nodep->lhsp(), AttrOf);
            અચલ AstAttrOf* અચલ rhsap = VN_AS(nodep->rhsp(), AttrOf);
            UASSERT_OBJ(lhsap->attrType() == VAttrType::TYPEID, lhsap,
                        "Type compare expects type reference");
            UASSERT_OBJ(rhsap->attrType() == VAttrType::TYPEID, rhsap,
                        "Type compare expects type reference");
            અચલ Ast_ગાંઠ_DType* અચલ lhsDtp = lhsap->dtypep();
            અચલ Ast_ગાંઠ_DType* અચલ rhsDtp = rhsap->dtypep();
            UINFO(9, "==type lhsDtp " << lhsDtp);
            UINFO(9, "==type rhsDtp " << lhsDtp);
            અચલ તાર્કિક invert = VN_IS(nodep, NeqT);
            અચલ તાર્કિક identical
                = Ast_ગાંઠ::computeCastable(lhsDtp, rhsDtp, nodep) == VCastable::SAMEISH;
            UINFO(9, "== " << identical);
            અચલ તાર્કિક eq = invert ^ identical;
            Ast_ગાંઠ* અચલ newp = નવું AstConst{nodep->fileline(), AstConst::BitTrue{}, eq};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
        }
    }

    શૂન્ય visit_negate_not(Ast_ગાંઠ_Uniop* nodep, તાર્કિક real_ok) {
        // CALLER: (real_ok=false) Not
        // CALLER: (real_ok=true) Negate - allow real numbers
        // Signed: From lhs
        // IEEE-2012 Table 11-21:
        //    Widths: out width = lhs width
        UASSERT_OBJ(!nodep->op2p(), nodep, "For unary ops only!");
        assert_At_પદાવલિ(nodep);
        જો (m_vup->prelim()) {
            userIterateAndNext(nodep->lhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            જો (!real_ok) checkCvtUS(nodep->lhsp(), ખોટું);
        }
        જો (real_ok && nodep->lhsp()->isDouble()) {
            spliceCvtD(nodep->lhsp());
            જો (Ast_ગાંઠ_Uniop* અચલ newp = replaceWithDVersion(nodep)) {
                VL_DANGLING(nodep);
                nodep = newp;  // Process new node instead
                iterateCheckReal(nodep, "LHS", nodep->lhsp(), BOTH);
                nodep->dtypeSetDouble();
                પાછા;
            }
        } નહીંતર {
            // Note there aren't yet uniops that need version changes
            // So no need to call replaceWithUOrSVersion(nodep, nodep->isSigned())
        }
        જો (m_vup->prelim()) nodep->dtypeFrom(nodep->lhsp());
        જો (m_vup->final()) {
            Ast_ગાંઠ_DType* અચલ expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
            nodep->dtypep(expDTypep);  // Propagate expression type to negation
            Ast_ગાંઠ_DType* અચલ subDTypep = expDTypep;
            // Some warning suppressions
            તાર્કિક lhsWarn = સાચું;
            જો (VN_IS(nodep, Negate)) {
                // Warn if user wants extra bit from carry
                જો (subDTypep->widthMin() == (nodep->lhsp()->widthMin() + 1)) lhsWarn = ખોટું;
            }
            iterateCheck(nodep, "LHS", nodep->lhsp(), CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP,
                         lhsWarn);
        }
    }

    શૂન્ય visit_signed_unsigned(Ast_ગાંઠ_Uniop* nodep, VSigning rs_out) {
        // CALLER: Signed, Unsigned
        // Width: lhs is self determined width
        // See IEEE-2012 6.24.1:
        //   Width: Returns packed array, of size $bits(expression).
        //   Sign: Output sign is as specified by operation
        //   TODO: Type: Two-state if input is two-state, else four-state
        UASSERT_OBJ(!nodep->op2p(), nodep, "For unary ops only!");
        assert_At_પદાવલિ(nodep);
        જો (m_vup->prelim()) {
            userIterateAndNext(nodep->lhsp(), WidthVP{SELF, PRELIM}.p());
            checkCvtUS(nodep->lhsp(), સાચું);
            અચલ પૂર્ણાંક width = nodep->lhsp()->width();
            Ast_ગાંઠ_DType* અચલ expDTypep = nodep->findBitOrLogicDType(
                width, width, rs_out, nodep->lhsp()->isFourstateOrNoDtype());
            nodep->dtypep(expDTypep);
            Ast_ગાંઠ_DType* અચલ subDTypep = expDTypep;
            // The child's width is self determined
            iterateCheck(nodep, "LHS", nodep->lhsp(), SELF, FINAL, subDTypep, EXTEND_EXP);
        }
    }

    શૂન્ય visit_shift(Ast_ગાંઠ_Biop* nodep) {
        // CALLER: ShiftL, ShiftR, ShiftRS
        // Widths: Output width from lhs, rhs<33 bits
        // Signed: Output signed iff LHS signed; unary operator
        // See IEEE 2012 11.4.10:
        //   RHS is self-determined. RHS is always treated as unsigned, has no effect on result.
        iterate_shift_prelim(nodep);
        nodep->dtypeChgSigned(nodep->lhsp()->isSigned());
        અચલ Ast_ગાંઠ_Biop* અચલ newp = iterate_shift_final(nodep);
        VL_DANGLING(nodep);
        (શૂન્ય)newp;  // Ununused
    }
    શૂન્ય iterate_shift_prelim(Ast_ગાંઠ_Biop* nodep) {
        // Shifts
        // See IEEE-2012 11.4.10 and Table 11-21.
        //   RHS is self-determined. RHS is always treated as unsigned, has no effect on result.
        assert_At_પદાવલિ(nodep);
        જો (m_vup->prelim()) {
            userIterateAndNext(nodep->lhsp(), WidthVP{SELF, PRELIM}.p());
            checkCvtUS(nodep->lhsp(), ખોટું);
            iterateCheckSizedSelf(nodep, "RHS", nodep->rhsp(), SELF, BOTH);
            nodep->dtypeFrom(nodep->lhsp());
            જો (!nodep->isFourstateOrNoDtype() && nodep->rhsp()->isFourstateOrNoDtype()) {
                nodep->dtypeSetLogicUnsized(nodep->width(), nodep->widthMin(),
                                            nodep->dtypep()->numeric());
            }
        }
    }
    Ast_ગાંઠ_Biop* iterate_shift_final(Ast_ગાંઠ_Biop* nodep) {
        // Nodep maybe edited
        assert_At_પદાવલિ(nodep);
        જો (m_vup->final()) {
            Ast_ગાંઠ_DType* અચલ expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
            Ast_ગાંઠ_DType* અચલ subDTypep = expDTypep;
            nodep->dtypep(expDTypep);
            // ShiftRS converts to ShiftR, but not vice-versa
            જો (VN_IS(nodep, ShiftRS)) {
                જો (Ast_ગાંઠ_Biop* અચલ newp = replaceWithUOrSVersion(nodep, nodep->isSigned())) {
                    VL_DANGLING(nodep);
                    nodep = newp;  // Process new node instead
                }
            }
            તાર્કિક warnOn = સાચું;
            // No warning if "X = 1'b1<<N"; assume user is doing what they want
            જો (nodep->lhsp()->isOne() && VN_IS(nodep->backp(), ગાંઠ_Assign)) warnOn = ખોટું;
            // We don't currently suppress these, as it's the upper operator (e.g. assign)
            // that reports the WIDTHEXPAND.
            AstConst* અચલ shiftp = VN_CAST(nodep->rhsp(), Const);
            જો (shiftp && !shiftp->અંક().isFourState() && shiftp->width() <= 32) {
                અચલ int64_t shiftVal = shiftp->અંક().toSQuad();
                જો (VN_IS(nodep, ShiftL)) {
                    જો (shiftVal > 0 && nodep->width() == nodep->lhsp()->width() + shiftVal)
                        warnOn = ખોટું;
                }
            }
            iterateCheck(nodep, "LHS", nodep->lhsp(), CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP,
                         warnOn);
            જો (nodep->rhsp()->width() > 32) {
                જો (shiftp && shiftp->અંક().mostSetBitP1() <= 32) {
                    // If (number)<<96'h1, then make it into (number)<<32'h1
                    V3_સંખ્યા અંક{shiftp, 32, shiftp->અંક()};
                    Ast_ગાંઠ* અચલ shiftrhsp = nodep->rhsp();
                    nodep->rhsp()->replaceWith(નવું AstConst{shiftrhsp->fileline(), અંક});
                    VL_DO_DANGLING(shiftrhsp->delete_વૃક્ષ(), shiftrhsp);
                }
            }
        }
        પાછા nodep;  // May edit
    }

    શૂન્ય visit_boolexpr_and_or(Ast_ગાંઠ_Biop* nodep) {
        // CALLER: And, Or, Xor, ...
        // Lint widths: out width = lhs width = rhs width
        // Signed: if lhs & rhs signed
        // IEEE-2012 Table 11-21:
        //    Width: max(LHS, RHS)
        UASSERT_OBJ(nodep->rhsp(), nodep, "For binary ops only!");
        // If errors are off, we need to follow the spec; thus we really need to do the max()
        // because the rhs could be larger, and we need to have proper editing to get the widths
        // to be the same for our operations.
        assert_At_પદાવલિ(nodep);
        જો (m_vup->prelim()) {  // First stage evaluation
            // Determine expression widths only relying on what's in the subops
            userIterateAndNext(nodep->lhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            userIterateAndNext(nodep->rhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            checkCvtUS(nodep->lhsp(), ખોટું);
            checkCvtUS(nodep->rhsp(), ખોટું);
            અચલ પૂર્ણાંક width = std::max(nodep->lhsp()->width(), nodep->rhsp()->width());
            અચલ પૂર્ણાંક mwidth = std::max(nodep->lhsp()->widthMin(), nodep->rhsp()->widthMin());
            અચલ તાર્કિક expSigned = (nodep->lhsp()->isSigned() && nodep->rhsp()->isSigned());
            nodep->dtypeSetBitOrLogicUnsized(
                width, mwidth, VSigning::fromBool(expSigned),
                Ast_ગાંઠ_પદાવલિ::isFourstateOrNoDtype(nodep->lhsp(), nodep->rhsp()));
        }
        જો (m_vup->final()) {
            Ast_ગાંઠ_DType* અચલ expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
            Ast_ગાંઠ_DType* અચલ subDTypep = expDTypep;
            nodep->dtypep(expDTypep);
            // Error report and change sizes for suboperands of this node.
            iterateCheck(nodep, "LHS", nodep->lhsp(), CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP);
            iterateCheck(nodep, "RHS", nodep->rhsp(), CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP);
        }
    }

    શૂન્ય visit_add_sub_replace(Ast_ગાંઠ_Biop* nodep, તાર્કિક real_ok, અચલ તાર્કિક modOrDiv) {
        // CALLER: (real_ok=false) AddS, SubS, ...
        // CALLER: (real_ok=true) Add, Sub, ...
        // Widths: out width = lhs width = rhs width
        // Signed: Replace operator with signed operator, or signed to unsigned
        // Real: Replace operator with real operator
        // IEEE-2012 Table 11-21:
        //    Width: max(LHS, RHS)
        // If errors are off, we need to follow the spec; thus we really need to do the max()
        // because the rhs could be larger, and we need to have proper editing to get the widths
        // to be the same for our operations.
        //
        // UINFOTREE(9, nodep, "rus " << m_vup, "rusin");
        assert_At_પદાવલિ(nodep);
        જો (m_vup->prelim()) {  // First stage evaluation
            // Determine expression widths only relying on what's in the subops
            userIterateAndNext(nodep->lhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            userIterateAndNext(nodep->rhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            જો (!real_ok) {
                checkCvtUS(nodep->lhsp(), ખોટું);
                checkCvtUS(nodep->rhsp(), ખોટું);
            }
            જો (nodep->lhsp()->isDouble() || nodep->rhsp()->isDouble()) {
                spliceCvtD(nodep->lhsp());
                spliceCvtD(nodep->rhsp());
                જો (Ast_ગાંઠ_Biop* અચલ newp = replaceWithDVersion(nodep)) {
                    VL_DANGLING(nodep);
                    nodep = newp;  // Process new node instead
                }
                nodep->dtypeSetDouble();
                iterateCheckReal(nodep, "LHS", nodep->lhsp(), FINAL);
                iterateCheckReal(nodep, "RHS", nodep->rhsp(), FINAL);
                પાછા;
            } નહીંતર જો (nodep->lhsp()->isString() || nodep->rhsp()->isString()) {
                nodep->v3error(
                    "Operator "
                    << nodep->prettyTypeName()
                    << " is not legal on string data types (IEEE 1800-2023 6.16)\n"
                    << (VN_IS(nodep, Add)
                            ? (nodep->warnMore()
                               + "... Suggest to concatenate strings use '{LHS, RHS, ...}'")
                            : ""));
                nodep->replaceWith(નવું AstConst{nodep->fileline(), AstConst::String{}, ""});
                VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
                પાછા;
            } નહીંતર {
                અચલ પૂર્ણાંક width = std::max(nodep->lhsp()->width(), nodep->rhsp()->width());
                અચલ પૂર્ણાંક mwidth = std::max(nodep->lhsp()->widthMin(), nodep->rhsp()->widthMin());
                અચલ તાર્કિક expSigned = (nodep->lhsp()->isSigned() && nodep->rhsp()->isSigned());
                nodep->dtypeSetBitOrLogicUnsized(
                    width, mwidth, VSigning::fromBool(expSigned),
                    (modOrDiv && !nodep->rhsp()->isNeqZero())
                        || Ast_ગાંઠ_પદાવલિ::isFourstateOrNoDtype(nodep->lhsp(), nodep->rhsp()));
            }
        }
        જો (m_vup->final()) {
            // Parent's data type was computed using the max(upper, nodep->dtype)
            Ast_ગાંઠ_DType* અચલ expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
            Ast_ગાંઠ_DType* અચલ subDTypep = expDTypep;
            nodep->dtypep(expDTypep);
            // We don't use LHS && RHS -- unspecified language corner, see t_math_signed5 test
            // bool expSigned = (nodep->lhsp()->isSigned() && nodep->rhsp()->isSigned());
            જો (Ast_ગાંઠ_Biop* અચલ newp = replaceWithUOrSVersion(nodep, expDTypep->isSigned())) {
                VL_DANGLING(nodep);
                nodep = newp;  // Process new node instead
            }
            // Some warning suppressions
            તાર્કિક lhsWarn = સાચું;
            તાર્કિક rhsWarn = સાચું;
            જો (VN_IS(nodep, Add) || VN_IS(nodep, Sub)) {
                // Warn if user wants extra bit from carry
                જો (subDTypep->widthMin() == (nodep->lhsp()->widthMin() + 1)) lhsWarn = ખોટું;
                જો (subDTypep->widthMin() == (nodep->rhsp()->widthMin() + 1)) rhsWarn = ખોટું;
                જો (VN_IS(nodep, Add) && nodep->lhsp()->width() == 1
                    && nodep->rhsp()->width() != 1)
                    lhsWarn = ખોટું;  // do_increment + ...
                જો (nodep->rhsp()->width() == 1 && nodep->lhsp()->width() != 1)
                    rhsWarn = ખોટું;  // ... + do_increment
            } નહીંતર જો (VN_IS(nodep, Mul) || VN_IS(nodep, MulS)) {
                જો (subDTypep->widthMin() >= (nodep->lhsp()->widthMin())) lhsWarn = ખોટું;
                જો (subDTypep->widthMin() >= (nodep->rhsp()->widthMin())) rhsWarn = ખોટું;
            }
            // Final call, so make sure children check their sizes
            // Error report and change sizes for suboperands of this node.
            iterateCheck(nodep, "LHS", nodep->lhsp(), CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP,
                         lhsWarn);
            iterateCheck(nodep, "RHS", nodep->rhsp(), CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP,
                         rhsWarn);
        }
        // UINFOTREE(9, nodep, "", "rusou");
    }
    શૂન્ય visit_real_add_sub(Ast_ગાંઠ_Biop* nodep) {
        // CALLER: AddD, MulD, ...
        assert_At_પદાવલિ(nodep);
        જો (m_vup->prelim()) {  // First stage evaluation
            // Note similar steps in visit_add_sub_replace promotion to double
            iterateCheckReal(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckReal(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtypeSetDouble();
        }
    }
    શૂન્ય visit_real_neg_ceil(Ast_ગાંઠ_Uniop* nodep) {
        // CALLER: Negate, Ceil, Log, ...
        assert_At_પદાવલિ(nodep);
        જો (m_vup->prelim()) {  // First stage evaluation
            // See alsl visit_negate_not conversion
            iterateCheckReal(nodep, "LHS", nodep->lhsp(), BOTH);
            nodep->dtypeSetDouble();
        }
    }

    શૂન્ય visit_sformatf_format(AstSFormatF* nodep) {
        // For sformatf's with constant format, iterate/check arguments
        UASSERT_OBJ(!nodep->પદાવલિ_Format_2(), nodep, "Assumes constant format");
        તાર્કિક inPct = ખોટું;
        Ast_ગાંઠ_પદાવલિ* argp = nodep->exprsp();
        string newFormat;
        માટે (અક્ષર ch : nodep->text()) {
            જો (!inPct && ch == '%') {
                inPct = સાચું;
                newFormat += ch;
            } નહીંતર જો (inPct && (std::isdigit(ch) || ch == '.' || ch == '-')) {
                newFormat += ch;
            } નહીંતર જો (!inPct) {  // Normal text
                newFormat += ch;
            } નહીંતર {
                inPct = ખોટું;
                Ast_ગાંઠ_પદાવલિ* અચલ nextp = argp ? VN_AS(argp->nextp(), ગાંઠ_પદાવલિ) : nullptr;
                AstSFormatArg* અચલ fargp = VN_CAST(argp, SFormatArg);  // May not exist yet
                Ast_ગાંઠ_પદાવલિ* subargp = fargp ? fargp->exprp() : argp;
                અચલ Ast_ગાંઠ_DType* અચલ dtypep
                    = subargp ? subargp->dtypep()->skipRefp() : nullptr;
                ch = std::tolower(ch);
                પસંદગી (ch) {
                સ્થિતિ '?':  // Unspecified by user, guess
                    જો (dtypep->isDouble()) {
                        ch = 'g';
                    } નહીંતર જો (dtypep->isString()) {
                        ch = 's';
                    } નહીંતર જો (VN_IS(dtypep, BasicDType)) {
                        ch = nodep->missingArgChar();
                    } નહીંતર {
                        ch = 'p';
                    }
                    argp = nextp;
                    તોડો;
                સ્થિતિ '%': તોડો;  // %% - just output a %
                સ્થિતિ 'm': તોડો;  // %m - auto insert "name"
                સ્થિતિ 'l': તોડો;  // %m - auto insert "library"
                સ્થિતિ 'd':
                    જો (subargp) {
                        checkFormatNumericArg(subargp, ch);
                        જો (subargp->isDouble()) spliceCvtS(subargp, સાચું, 64);
                        argp = nextp;
                    }
                    તોડો;
                સ્થિતિ 'b':  // FALLTHRU
                સ્થિતિ 'o':  // FALLTHRU
                સ્થિતિ 'x':
                    જો (subargp) {
                        // V3Randomize may make a %x on a string, or may
                        // have a runtime format which gets past this error
                        checkFormatNumericArg(subargp, ch);
                        argp = nextp;
                    }
                    તોડો;
                સ્થિતિ 't':  // Decimal/float time
                    જો (subargp) {
                        checkFormatNumericArg(subargp, ch);
                        UASSERT_OBJ(!nodep->timeunit().isNone(), nodep,
                                    "display %t has no time units");
                        argp = nextp;
                    }
                    તોડો;
                સ્થિતિ 'e':  // FALLTHRU
                સ્થિતિ 'f':  // FALLTHRU
                સ્થિતિ 'g':
                    જો (subargp) {
                        checkFormatNumericArg(subargp, ch);
                        જો (!subargp->isDouble()) {
                            iterateCheckReal(nodep, "Display argument", subargp, BOTH);
                        }
                        argp = nextp;
                    }
                    તોડો;
                સ્થિતિ 'p':  // FALLTHRU
                સ્થિતિ 's':
                    // As with enum.name(): valid values print the mnemonic, else numeric
                    જો (subargp && !fargp) {
                        જો (AstEnumDType* અચલ enumDtp = formatEnumDType(subargp)) {
                            VNRelinker relinker;
                            subargp->unlinkFrBack(&relinker);
                            relinker.relink(newFormatEnumArg(subargp, enumDtp));
                        }
                    }
                    argp = nextp;
                    તોડો;
                મૂળભૂત:  // Most operators, just move to next argument
                    argp = nextp;
                    તોડો;
                }  // switch
                newFormat += ch;
            }
        }
        nodep->text(newFormat);
    }

    સ્થિર AstEnumDType* formatEnumDType(Ast_ગાંઠ_પદાવલિ* subargp) {
        AstEnumDType* enumDtp = VN_CAST(subargp->dtypep()->skipRefToEnump(), EnumDType);
        જો (!enumDtp) {
            જો (અચલ AstVarRef* અચલ varrefp = VN_CAST(subargp, VarRef)) {
                enumDtp = VN_CAST(varrefp->varp()->dtypep()->skipRefToEnump(), EnumDType);
            }
        }
        પાછા enumDtp;
    }

    Ast_પદાવલિ_Stmt* newEnumCapture(Ast_ગાંઠ_પદાવલિ* valuep, AstEnumDType* dtypep) {
        File_લીટી* અચલ flp = valuep->fileline();
        AstVar* અચલ varp = નવું AstVar{flp, VVarType::XTEMP, m_enumTempNames.get(valuep), dtypep};
        varp->noSubst(સાચું);
        જો (m_ftaskp) {
            varp->funcLocal(સાચું);
            varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            // Declare before use, including in non-inlined class methods.
            m_ftaskp->prependStmtsp(varp);
        } નહીંતર {
            UASSERT_OBJ(m_modep, valuep, "Enum expression without a containing module");
            m_modep->addStmtsp(varp);
        }
        iterate(varp);
        Ast_પદાવલિ_Stmt* અચલ exprp = નવું Ast_પદાવલિ_Stmt{
            flp, નવું AstAssign{flp, નવું AstVarRef{flp, varp, VAccess::WRITE}, valuep},
            નવું AstVarRef{flp, varp, VAccess::READ}};
        iterate(exprp);
        પાછા exprp;
    }

    AstSFormatArg* newFormatEnumArg(Ast_ગાંઠ_પદાવલિ* valuep, AstEnumDType* dtypep) {
        જો (valuep->isPure()) valuep = V3Const::constifyEdit(valuep);
        Ast_ગાંઠ_પદાવલિ* keyp;
        જો (VN_IS(valuep, Const) || VN_IS(valuep, VarRef) || (!m_modep && !m_ftaskp)) {
            // Standalone parameter expressions are checked for constness by V3Const.
            keyp = valuep->clone_વૃક્ષ(ખોટું);
        } નહીંતર {
            Ast_પદાવલિ_Stmt* અચલ capturep = newEnumCapture(valuep, dtypep);
            valuep = capturep;
            keyp = capturep->resultp()->clone_વૃક્ષ_Pure(ખોટું);
        }
        અચલ VFormatAttr attr = dtypep->isSigned() ? VFormatAttr::ENUM_SIGNED : VFormatAttr::ENUM;
        AstSFormatArg* અચલ argp = નવું AstSFormatArg{valuep->fileline(), attr, valuep};
        argp->dtypep(dtypep);
        argp->namep(enumSelect(keyp, dtypep, VAttrType::ENUM_NAME));
        પાછા argp;
    }

    //----------------------------------------------------------------------
    // LOWER LEVEL WIDTH METHODS  (none iterate)

    તાર્કિક widthBad(Ast_ગાંઠ* nodep, Ast_ગાંઠ_DType* expDTypep) {
        અચલ પૂર્ણાંક expWidth = expDTypep->width();
        પૂર્ણાંક expWidthMin = expDTypep->widthMin();
        UASSERT_OBJ(nodep->dtypep(), nodep,
                    "Under node " << nodep->prettyTypeName()
                                  << " has no dtype?? Missing Visitor func?");
        જો (expDTypep->basicp()->untyped() || nodep->dtypep()->basicp()->untyped()) પાછા ખોટું;
        // During DepGraph execution, expected width may be 0 if the type hasn't been
        UASSERT_OBJ(nodep->width() != 0, nodep,
                    "Under node " << nodep->prettyTypeName()
                                  << " has no expected width?? Missing Visitor func?");
        UASSERT_OBJ(expWidth != 0, nodep,
                    "Node " << nodep->prettyTypeName()
                            << " has no expected width?? Missing Visitor func?");
        જો (expWidthMin == 0) expWidthMin = expWidth;
        જો (nodep->dtypep()->width() == expWidth) પાછા ખોટું;
        જો (nodep->dtypep()->widthSized() && nodep->width() != expWidthMin) પાછા સાચું;
        જો (!nodep->dtypep()->widthSized() && nodep->widthMin() > expWidthMin) પાછા સાચું;
        પાછા ખોટું;
    }

    શૂન્ય fixWidthExtend(Ast_ગાંઠ_પદાવલિ* nodep, Ast_ગાંઠ_DType* expDTypep, ExtendRule extendRule) {
        // Fix the width mismatch by extending or truncating bits
        // *ONLY* call this from checkWidth()
        // Truncation is rarer, but can occur:  parameter [3:0] FOO = 64'h12312;
        // A(CONSTwide)+B becomes  A(CONSTwidened)+B
        // A(somewide)+B  becomes  A(TRUNC(somewide,width))+B
        //                    or   A(EXTRACT(somewide,width,0))+B
        // Sign extension depends on the type of the *present*
        // node, while the output dtype is the *expected* sign.
        // It is reasonable to have sign extension with unsigned output,
        // for example $unsigned(a)+$signed(b), the SIGNED(B) will be unsigned dtype out
        UINFO(4, "  widthExtend_(r=" << static_cast<પૂર્ણાંક>(extendRule) << ") old: " << nodep);
        જો (extendRule == EXTEND_OFF) પાછા;
        AstConst* અચલ constp = VN_CAST(nodep, Const);
        અચલ પૂર્ણાંક expWidth = expDTypep->width();
        જો (constp && !constp->અંક().isNegative()) {
            // Save later constant propagation work, just right-size it.
            V3_સંખ્યા અંક{nodep, expWidth, constp->અંક()};
            અંક.isSigned(ખોટું);
            Ast_ગાંઠ_પદાવલિ* અચલ newp = નવું AstConst{nodep->fileline(), અંક};
            constp->replaceWith(newp);
            VL_DO_DANGLING(ધકેલો_Deletep(constp), constp);
            VL_DANGLING(nodep);
            nodep = newp;
        } નહીંતર જો (expWidth < nodep->width()) {
            // Trunc - Extract
            VNRelinker linker;
            nodep->unlinkFrBack(&linker);
            Ast_ગાંઠ_પદાવલિ* અચલ newp = નવું AstSel{nodep->fileline(), nodep, 0, expWidth};
            newp->didWidth(સાચું);  // Don't replace dtype with unsigned
            linker.relink(newp);
            nodep = newp;
        } નહીંતર {
            // Extend
            VNRelinker linker;
            nodep->unlinkFrBack(&linker);
            તાર્કિક doSigned = ખોટું;
            પસંદગી (extendRule) {
            સ્થિતિ EXTEND_ZERO: doSigned = ખોટું; તોડો;
            સ્થિતિ EXTEND_EXP: doSigned = nodep->isSigned() && expDTypep->isSigned(); તોડો;
            સ્થિતિ EXTEND_LHS: doSigned = nodep->isSigned(); તોડો;
            મૂળભૂત: nodep->v3fatalSrc("bad case");
            }
            Ast_ગાંઠ_પદાવલિ* અચલ newp
                = (doSigned ? static_cast<Ast_ગાંઠ_પદાવલિ*>(નવું AstExtendS{nodep->fileline(), nodep})
                            : static_cast<Ast_ગાંઠ_પદાવલિ*>(નવું AstExtend{nodep->fileline(), nodep}));
            newp->didWidth(સાચું);
            linker.relink(newp);
            nodep = newp;
        }
        જો (expDTypep->isDouble() && !nodep->isDouble()) {
            // For AstVar init() among others
            // TODO do all to-real and to-integer conversions in this function
            // rather than in callers
            Ast_ગાંઠ_પદાવલિ* અચલ newp = spliceCvtD(nodep);
            nodep = newp;
        }
        જો (expDTypep->skipRefp()->isIntegralOrPacked()) {
            nodep->dtypeChgWidthSigned(expWidth, expDTypep->widthMin(), expDTypep->numeric());
        } નહીંતર {
            nodep->dtypeFrom(expDTypep);
        }
        UINFO(4, "             _new: " << nodep);
    }

    શૂન્ય fixWidthReduce(Ast_ગાંઠ_પદાવલિ* nodep) {
        // Fix the width mismatch by adding a reduction OR operator
        // IF (A(CONSTwide)) becomes IF (A(CONSTreduced))
        // IF (A(somewide)) on pointer becomes on pointer IF (A(NEQ(somewide, Null)))
        // IF (A(somewide)) elsewise becomes IF (A(REDOR(somewide, 0)))
        // Attempt to fix it quietly
        અચલ પૂર્ણાંક expWidth = 1;
        અચલ પૂર્ણાંક expSigned = ખોટું;
        જો (nodep->width() == expWidth) પાછા;
        UINFO(4, "  widthReduce_old: " << nodep);
        AstConst* અચલ constp = VN_CAST(nodep, Const);
        જો (constp) {
            V3_સંખ્યા અંક(nodep, expWidth);
            અંક.opRedOr(constp->અંક());
            અંક.isSigned(expSigned);
            Ast_ગાંઠ_પદાવલિ* અચલ newp = નવું AstConst{nodep->fileline(), અંક};
            constp->replaceWith(newp);
            VL_DO_DANGLING(constp->delete_વૃક્ષ(), constp);
            VL_DANGLING(nodep);
            nodep = newp;
        } નહીંતર {
            VNRelinker linker;
            nodep->unlinkFrBack(&linker);
            Ast_ગાંઠ_પદાવલિ* newp;
            Ast_ગાંઠ_DType* અચલ dtypeSkipp = nodep->dtypep()->skipRefp();
            જો (VN_IS(dtypeSkipp, ClassRefDType) || VN_IS(dtypeSkipp, IfaceRefDType)) {
                // Cannot use AstRedOr, as we may be redurcing a class handle
                newp = નવું AstNeq{nodep->fileline(),
                                  નવું AstConst{nodep->fileline(), AstConst::Null{}},

                                  nodep};
            } નહીંતર {
                // But, AstRedOr can be faster on wide
                newp = નવું AstRedOr{nodep->fileline(), nodep};
            }
            linker.relink(newp);
            nodep = newp;
        }
        nodep->dtypeChgWidthSigned(expWidth, expWidth, VSigning::fromBool(expSigned));
        UINFO(4, "             _new: " << nodep);
    }

    તાર્કિક fixAutoExtend(Ast_ગાંઠ_પદાવલિ*& nodepr, પૂર્ણાંક expWidth) {
        // For SystemVerilog '0,'1,'x,'z, autoextend and don't warn
        જો (AstConst* અચલ constp = VN_CAST(nodepr, Const)) {
            જો (constp->અંક().autoExtend() && !constp->અંક().sized() && constp->width() == 1) {
                // Make it the proper size.  Careful of proper extension of 0's/1's
                V3_સંખ્યા અંક(constp, expWidth);
                અંક.opRepl(constp->અંક(), expWidth);  // {width{'1}}
                Ast_ગાંઠ_પદાવલિ* અચલ newp = નવું AstConst{constp->fileline(), અંક};
                // Spec says always unsigned with proper width
                UINFOTREE(5, constp, "", "fixAutoExtend_old");
                UINFOTREE(5, newp, "", "_new");
                constp->replaceWith(newp);
                VL_DO_DANGLING(constp->delete_વૃક્ષ(), constp);
                // Tell caller the new constp, and that we changed it.
                nodepr = newp;
                પાછા સાચું;
            }
            // X/Z also upper bit extend.  In pre-SV only to 32-bits, SV forever.
            નહીંતર જો (!constp->અંક().sized()
                     // Make it the proper size.  Careful of proper extension of 0's/1's
                     && expWidth > 32 && constp->અંક().isMsbXZ()) {
                constp->v3warn(WIDTHXZEXPAND, "Unsized constant being X/Z extended to "
                                                  << expWidth
                                                  << " bits: " << constp->prettyName());
                V3_સંખ્યા અંક(constp, expWidth);
                અંક.opExtendXZ(constp->અંક(), constp->width());
                Ast_ગાંઠ_પદાવલિ* અચલ newp = નવું AstConst{constp->fileline(), અંક};
                // Spec says always unsigned with proper width
                UINFOTREE(5, constp, "", "fixUnszExtend_old");
                UINFOTREE(5, newp, "", "_new");
                constp->replaceWith(newp);
                VL_DO_DANGLING(constp->delete_વૃક્ષ(), constp);
                // Tell caller the new constp, and that we changed it.
                nodepr = newp;
                પાછા સાચું;
            }
        }
        પાછા ખોટું;  // No change
    }
    તાર્કિક isBaseClassRecurse(અચલ AstClass* અચલ cls1p, અચલ AstClass* અચલ cls2p) {
        // Returns true if cls1p and cls2p are equal or if cls1p is a base class of cls2p
        જો (cls1p == cls2p) પાછા સાચું;
        માટે (અચલ AstClassExtends* cextp = cls2p->extendsp(); cextp;
             cextp = VN_CAST(cextp->nextp(), ClassExtends)) {
            જો (isBaseClassRecurse(cls1p, cextp->classp())) પાછા સાચું;
        }
        પાછા ખોટું;
    }
    // Lines to end a message on nodep with the data types of its operands
    સ્થિર string warnDTypes(અચલ Ast_ગાંઠ* nodep, અચલ Ast_ગાંઠ_DType* lhsDtp,
                             અચલ Ast_ગાંઠ_DType* rhsDtp, અચલ string& lhsName = "Left-hand",
                             અચલ string& rhsName = "Right-hand")
        VL_REQUIRES(V3Error::s().m_mutex) {
        string lhsState = " " + lhsDtp->stateDTypeName();
        string rhsState = " " + rhsDtp->stateDTypeName();
        જો (lhsState == rhsState) lhsState = rhsState = "";
        પાછા "\n" + nodep->warnMore() + "... " + lhsName
               + " data type: " + lhsDtp->prettyDTypeNameQ() + lhsState + "\n" + nodep->warnMore()
               + "... " + rhsName + " data type: " + rhsDtp->prettyDTypeNameQ() + rhsState;
    }
    // Fixed-size unpacked arrays are assignment compatible only with the same number of
    // elements in each dimension (IEEE 1800-2023 7.6); dynamically sized ones always are
    સ્થિર તાર્કિક unpackedArraySizesMatch(અચલ Ast_ગાંઠ_DType* lhsDtp, અચલ Ast_ગાંઠ_DType* rhsDtp) {
        જ્યારે (lhsDtp->isNonPackedArray() && rhsDtp->isNonPackedArray()) {
            અચલ AstUnpackArrayDType* અચલ lhsArrayp = VN_CAST(lhsDtp, UnpackArrayDType);
            અચલ AstUnpackArrayDType* અચલ rhsArrayp = VN_CAST(rhsDtp, UnpackArrayDType);
            જો (lhsArrayp && rhsArrayp
                && lhsArrayp->elementsConst() != rhsArrayp->elementsConst()) {
                પાછા ખોટું;
            }
            lhsDtp = lhsDtp->subDTypep()->skipRefp();
            rhsDtp = rhsDtp->subDTypep()->skipRefp();
        }
        પાછા સાચું;
    }
    // Checks whether two types are assignment-compatible according to IEEE 1800-2023 7.6
    // Currently, this function only supports variables, which are of following types:
    // - Fixed-size unpacked array
    // - Dynamic unpacked array
    // - Associative array
    template <typename T, typename N>
    શૂન્ય checkUnpackedArrayAssignmentCompatible(અચલ Ast_ગાંઠ* nodep, અચલ T* અચલ lhsRefp,
                                                અચલ N* અચલ rhsRefp) {
        static_assert(
            (std::is_same<T, AstVar>::કિંમત || std::is_same<T, Ast_ગાંઠ_Var_Ref>::કિંમત)
                && (std::is_same<N, AstVar>::કિંમત || std::is_same<N, Ast_ગાંઠ_Var_Ref>::કિંમત),
            "Unsupported types provided.");
        જો (!lhsRefp || !rhsRefp) પાછા;
        std::string lhsName;
        std::string rhsName;
        જો (VN_IS(nodep, Pin)) {
            lhsName = std::string{"Pin"};
            rhsName = std::string{"Expression"};
        } નહીંતર {
            lhsName = std::string{"Left-hand"};
            rhsName = std::string{"Right-hand"};
        }

        અચલ Ast_ગાંઠ_DType* અચલ lhsDtp = lhsRefp->dtypep()->skipRefp();
        અચલ Ast_ગાંઠ_DType* અચલ rhsDtp = rhsRefp->dtypep()->skipRefp();
        અચલ તાર્કિક isLhsAggregate = lhsDtp->isAggregateType();
        અચલ તાર્કિક isRhsAggregate = rhsDtp->isAggregateType();
        જો (!isLhsAggregate && !isRhsAggregate) પાછા;
        જો (isLhsAggregate ^ isRhsAggregate) {
            nodep->v3error(
                "Illegal assignment: types are not assignment compatible (IEEE 1800-2023 7.6)"
                << warnDTypes(nodep, lhsDtp, rhsDtp, lhsName, rhsName));
            પાછા;
        } નહીંતર જો (VN_IS(lhsDtp, QueueDType) && VN_IS(rhsDtp, EmptyQueueDType)) {
            પાછા;
        }
        std::pair<uint32_t, uint32_t> lhsDim = lhsDtp->dimensions(ખોટું),
                                      rhsDim = rhsDtp->dimensions(ખોટું);
        // Check if unpacked array dimensions are matching
        જો (lhsDim.second != rhsDim.second) {
            nodep->v3error("Illegal assignment: Unmatched number of unpacked dimensions (IEEE "
                           "1800-2023 7.6)"
                           << warnDTypes(nodep, lhsDtp, rhsDtp, lhsName, rhsName));
            પાછા;
        }

        અચલ Ast_ગાંઠ_DType* lhsDtpIterp = lhsDtp;
        અચલ Ast_ગાંઠ_DType* rhsDtpIterp = rhsDtp;
        માટે (uint32_t dim = 0; dim < rhsDim.second; dim++) {
            // Associative arrays are compatible only with each other
            જો (VN_IS(lhsDtpIterp, AssocArrayDType) ^ VN_IS(rhsDtpIterp, AssocArrayDType)) {
                nodep->v3error("Illegal assignment: Associative arrays are assignment compatible "
                               "only with associative arrays (IEEE 1800-2023 7.6)"
                               << warnDTypes(nodep, lhsDtp, rhsDtp, lhsName, rhsName));
                પાછા;
            }
            lhsDtpIterp = lhsDtpIterp->subDTypep()->skipRefp();
            rhsDtpIterp = rhsDtpIterp->subDTypep()->skipRefp();
        }
        // Element types of source and target shall be equivalent
        જો (!isEquivalentDType(lhsDtpIterp, rhsDtpIterp)) {
            nodep->v3error("Illegal assignment: Array element types are not equivalent (IEEE "
                           "1800-2023 6.22.2)"
                           << warnDTypes(nodep, lhsDtp, rhsDtp, lhsName, rhsName));
        }
    }
    શૂન્ય checkClassAssign(અચલ Ast_ગાંઠ* nodep, અચલ અક્ષર* side, Ast_ગાંઠ* rhsp,
                          Ast_ગાંઠ_DType* અચલ lhsDTypep, તાર્કિક isOutputArg = ખોટું) {
        UASSERT_OBJ(rhsp->dtypep(), rhsp, "Node has no type");
        અચલ Ast_ગાંઠ_DType* અચલ lhsRawDTypep = lhsDTypep->skipRefp();
        અચલ Ast_ગાંઠ_DType* અચલ rhsRawDTypep = rhsp->dtypep()->skipRefp();
        જો (અચલ AstClassRefDType* અચલ lhsClassRefp = VN_CAST(lhsRawDTypep, ClassRefDType)) {
            જો (અચલ AstClassRefDType* અચલ rhsClassRefp
                = VN_CAST(rhsRawDTypep, ClassRefDType)) {
                // Output arg swaps sides: caller actual = callee formal at return.
                અચલ AstClass* અચલ dstp
                    = isOutputArg ? rhsClassRefp->classp() : lhsClassRefp->classp();
                અચલ AstClass* અચલ srcp
                    = isOutputArg ? lhsClassRefp->classp() : rhsClassRefp->classp();
                જો (isBaseClassRecurse(dstp, srcp)) પાછા;
            } નહીંતર જો (rhsp->isNull()) {
                પાછા;
            }
            nodep->v3error(side << " expects a " << lhsClassRefp->prettyTypeName() << ", got "
                                << rhsRawDTypep->prettyTypeName());
        } નહીંતર જો (VN_IS(rhsRawDTypep, ClassRefDType)) {
            nodep->v3error(side << " " << rhsRawDTypep->prettyDTypeNameQ()
                                << " cannot be assigned to non-class "
                                << lhsDTypep->prettyDTypeNameQ());
        }
        જો (VN_IS(lhsRawDTypep, DynArrayDType) && VN_IS(rhsRawDTypep, UnpackArrayDType)) {
            VNRelinker relinker;
            rhsp->unlinkFrBack(&relinker);
            relinker.relink(
                નવું AstCvtUnpackToQueue{rhsp->fileline(), VN_AS(rhsp, ગાંઠ_પદાવલિ), lhsDTypep});
        }
    }
    સ્થિર તાર્કિક similarDTypeRecurse(અચલ Ast_ગાંઠ_DType* અચલ node1p,
                                    અચલ Ast_ગાંઠ_DType* અચલ node2p) {
        પાછા node1p->skipRefp()->similarDType(node2p->skipRefp());
    }
    શૂન્ય iterateCheckFileDesc(Ast_ગાંઠ* parentp, Ast_ગાંઠ* underp, Stage stage) {
        UASSERT_OBJ(stage == BOTH, parentp, "Bad call");
        // underp may change as a result of replacement
        underp = userIterateSubtreeReturnEdits(underp, WidthVP{SELF, PRELIM}.p());
        Ast_ગાંઠ_DType* અચલ expDTypep = underp->findUInt32DType();
        underp
            = iterateCheck(parentp, "file_descriptor", underp, SELF, FINAL, expDTypep, EXTEND_EXP);
        (શૂન્ય)underp;  // cppcheck
    }
    શૂન્ય iterateCheckReal(Ast_ગાંઠ* parentp, અચલ અક્ષર* side, Ast_ગાંઠ* underp, Stage stage) {
        // Coerce child to real if not already. Child is self-determined
        // e.g. parentp=ADDD, underp=ADD in ADDD(ADD(a,b), real-CONST)
        // Don't need separate PRELIM and FINAL(double) calls;
        // as if resolves to double, the BOTH correctly resolved double,
        // otherwise self-determined was correct
        iterateCheckTypedSelfPrelim(parentp, side, underp, parentp->findDoubleDType(), stage);
    }
    શૂન્ય iterateCheckSigned8(Ast_ગાંઠ* parentp, અચલ અક્ષર* side, Ast_ગાંઠ* underp, Stage stage) {
        // Coerce child to signed8 if not already. Child is self-determined
        iterateCheckTypedSelfPrelim(parentp, side, underp, parentp->findSigned8DType(), stage);
    }
    શૂન્ય iterateCheckSigned32(Ast_ગાંઠ* parentp, અચલ અક્ષર* side, Ast_ગાંઠ* underp, Stage stage) {
        // Coerce child to signed32 if not already. Child is self-determined
        iterateCheckTypedSelfPrelim(parentp, side, underp, parentp->findIntDType(), stage);
    }
    શૂન્ય iterateCheckUInt32(Ast_ગાંઠ* parentp, અચલ અક્ષર* side, Ast_ગાંઠ* underp, Stage stage) {
        // Coerce child to unsigned32 if not already. Child is self-determined
        iterateCheckTypedSelfPrelim(parentp, side, underp, parentp->findUInt32DType(), stage);
    }
    શૂન્ય iterateCheckDelay(Ast_ગાંઠ* parentp, અચલ અક્ષર* side, Ast_ગાંઠ* underp, Stage stage) {
        // Coerce child to 64-bit delay if not already. Child is self-determined
        // underp may change as a result of replacement
        જો (stage & PRELIM) {
            underp = userIterateSubtreeReturnEdits(underp, WidthVP{SELF, PRELIM}.p());
        }
        જો (stage & FINAL) {
            Ast_ગાંઠ_DType* expDTypep;
            જો (underp->dtypep()->skipRefp()->isDouble()) {  // V3Timing will later convert double
                expDTypep = parentp->findDoubleDType();
            } નહીંતર {
                File_લીટી* અચલ newFl = નવું File_લીટી{underp->fileline()};
                newFl->warnOff(V3ErrorCode::WIDTHEXPAND, સાચું);
                underp->fileline(newFl);
                expDTypep = parentp->findBitDType(64, 64, VSigning::UNSIGNED);
            }
            underp
                = iterateCheck(parentp, side, underp, SELF, FINAL, expDTypep, EXTEND_EXP, ખોટું);
        }
        (શૂન્ય)underp;  // cppcheck
    }
    શૂન્ય iterateCheckString(Ast_ગાંઠ* parentp, અચલ અક્ષર* side, Ast_ગાંઠ* underp, Stage stage) {
        iterateCheckTyped(parentp, side, underp, parentp->findStringDType(), stage);
    }
    શૂન્ય iterateCheckTyped(Ast_ગાંઠ* parentp, અચલ અક્ષર* side, Ast_ગાંઠ* underp,
                           Ast_ગાંઠ_DType* expDTypep, Stage stage) {
        જો (stage & PRELIM) {
            underp = userIterateSubtreeReturnEdits(underp, WidthVP{expDTypep, PRELIM}.p());
        }
        જો (stage & FINAL) {
            underp = iterateCheck(parentp, side, underp, SELF, FINAL, expDTypep, EXTEND_EXP);
        }
        (શૂન્ય)underp;  // cppcheck
    }
    શૂન્ય iterateCheckTypedSelfPrelim(Ast_ગાંઠ* parentp, અચલ અક્ષર* side, Ast_ગાંઠ* underp,
                                     Ast_ગાંઠ_DType* expDTypep, Stage stage) {
        // underp may change as a result of replacement
        જો (stage & PRELIM) {
            underp = userIterateSubtreeReturnEdits(underp, WidthVP{SELF, PRELIM}.p());
        }
        જો (stage & FINAL) {
            underp = iterateCheck(parentp, side, underp, SELF, FINAL, expDTypep, EXTEND_EXP);
        }
        (શૂન્ય)underp;  // cppcheck
    }
    શૂન્ય iterateCheckIntegralSelf(Ast_ગાંઠ* parentp, અચલ અક્ષર* side, Ast_ગાંઠ* underp,
                                  Determ determ, Stage stage) {
        iterateCheckSelf(parentp, side, underp, determ, stage, સાચું);
    }
    શૂન્ય iterateCheckSelf(Ast_ગાંઠ* parentp, અચલ અક્ષર* side, Ast_ગાંઠ* underp, Determ determ,
                          Stage stage, તાર્કિક integralOnly = ખોટું) {
        // Coerce child to any data type; child is self-determined
        // i.e. isolated from expected type.
        // e.g. parentp=CONCAT, underp=lhs in CONCAT(lhs,rhs)
        UASSERT_OBJ(determ == SELF, parentp, "Bad call");
        UASSERT_OBJ(stage == FINAL || stage == BOTH, parentp, "Bad call");
        // underp may change as a result of replacement
        જો (stage & PRELIM) {
            underp = userIterateSubtreeReturnEdits(underp, WidthVP{SELF, PRELIM}.p());
        }
        underp
            = VN_IS(underp, ગાંઠ_પદાવલિ) ? checkCvtUS(VN_AS(underp, ગાંઠ_પદાવલિ), integralOnly) : underp;
        Ast_ગાંઠ_DType* અચલ expDTypep = underp->dtypep();
        underp = iterateCheck(parentp, side, underp, SELF, FINAL, expDTypep, EXTEND_EXP);
        (શૂન્ય)underp;  // cppcheck
    }
    શૂન્ય iterateCheckSizedSelf(Ast_ગાંઠ* parentp, અચલ અક્ષર* side, Ast_ગાંઠ* underp, Determ determ,
                               Stage stage, તાર્કિક deferUntyped = ખોટું) {
        // Coerce child to any sized-number data type; child is self-determined
        // i.e. isolated from expected type.
        // e.g. parentp=CONCAT, underp=lhs in CONCAT(lhs,rhs)
        UASSERT_OBJ(determ == SELF, parentp, "Bad call");
        UASSERT_OBJ(stage == FINAL || stage == BOTH, parentp, "Bad call");
        // underp may change as a result of replacement
        જો (stage & PRELIM) {
            underp = userIterateSubtreeReturnEdits(underp, WidthVP{SELF, PRELIM}.p());
        }
        underp = VN_IS(underp, ગાંઠ_પદાવલિ) ? checkCvtUS(VN_AS(underp, ગાંઠ_પદાવલિ), ખોટું) : underp;
        Ast_ગાંઠ_DType* અચલ expDTypep = underp->dtypep();
        underp = iterateCheck(parentp, side, underp, SELF, FINAL, expDTypep, EXTEND_EXP);
        Ast_ગાંઠ_DType* અચલ checkDtp = expDTypep->skipRefToEnump();
        // Sampling functions may still reference an untyped property formal here.
        // V3AssertPre checks the actual type after property argument substitution.
        જો (!checkDtp->isIntegralOrPacked()
            && !(deferUntyped && checkDtp->basicp() && checkDtp->basicp()->untyped())) {
            parentp->v3error(ucfirst(parentp->prettyOperatorName())
                             << " expects a numeric " << side << " operand, but it has data type "
                             << checkDtp->prettyDTypeNameQ() << " (IEEE 1800-2023 11.3)");
        }
        (શૂન્ય)underp;  // cppcheck
    }
    શૂન્ય iterateCheckAssign(Ast_ગાંઠ* parentp, અચલ અક્ષર* side, Ast_ગાંઠ* rhsp, Stage stage,
                            Ast_ગાંઠ_DType* lhsDTypep, VAccess rhsAccess = VAccess::READ) {
        // Check using assignment-like context rules
        // UINFOTREE(1, parentp, "", "checkass");
        UASSERT_OBJ(stage == FINAL, parentp, "Bad width call");
        // Create unpacked byte from string see IEEE 1800-2023 5.9
        જો (AstConst* constp = VN_CAST(rhsp, Const)) {
            જો (અચલ AstUnpackArrayDType* અચલ arrayp
                = VN_CAST(lhsDTypep->skipRefp(), UnpackArrayDType)) {
                જો (AstBasicDType* basicp = VN_CAST(arrayp->subDTypep()->skipRefp(), BasicDType)) {
                    જો (basicp->width() == 8 && constp->અંક().isFromString()) {
                        AstConsArray* newp = નવું AstConsArray{
                            constp->fileline(), lhsDTypep,
                            નવું AstConst{constp->fileline(), AstConst::Widthed_કિંમત{}, 8, 0}};
                        માટે (અચલ પૂર્ણાંક aindex : arrayp->declRange().seqLoToHi()) {
                            પૂર્ણાંક cindex = arrayp->declRange().ascending() ? (arrayp->hi() - aindex)
                                                                         : (aindex - arrayp->lo());
                            V3_સંખ્યા selected{constp, 8};
                            selected.opSel(constp->અંક(), cindex * 8 + 7, cindex * 8);
                            UINFO(0, "   aindex=" << aindex << "  cindex=" << cindex
                                                  << "  c=" << selected);
                            જો (!selected.isFourState()) {
                                જો (અચલ uint32_t c = selected.toUInt()) {
                                    newp->valuep(aindex,
                                                 નવું AstConst{constp->fileline(),
                                                              AstConst::Widthed_કિંમત{}, 8, c});
                                }
                            }
                        }
                        UINFO(6, "   unpackFromString: " << parentp);
                        rhsp->replaceWith(newp);
                        VL_DO_DANGLING(ધકેલો_Deletep(rhsp), rhsp);
                        rhsp = newp;
                    }
                }
            }
        }
        // We iterate and size the RHS based on the result of RHS evaluation
        checkClassAssign(parentp, side, rhsp, lhsDTypep);
        અચલ તાર્કિક lhsStream = (VN_IS(parentp, ગાંઠ_Assign)
                                && VN_IS(VN_AS(parentp, ગાંઠ_Assign)->lhsp(), ગાંઠ_Stream));
        rhsp = iterateCheck(parentp, side, rhsp, ASSIGN, FINAL, lhsDTypep,
                            lhsStream ? EXTEND_OFF : EXTEND_LHS, સાચું, STREAM_USE_NONE, rhsAccess);
        // UINFOTREE(1, parentp, "", "checkout");
        (શૂન્ય)rhsp;  // cppcheck
    }
    // Convert an unpacked array value to an equivalent type with some dimension reversed,
    // keeping elements in left-to-right order (IEEE 1800-2023 7.6)
    Ast_ગાંઠ_પદાવલિ* convertUnpackedOrder(Ast_ગાંઠ_પદાવલિ* exprp, Ast_ગાંઠ_DType* dtypep) {
        અચલ AstUnpackArrayDType* અચલ arrayp = VN_CAST(dtypep->skipRefp(), UnpackArrayDType);
        જો (!arrayp || !arrayp->isReverseOf(exprp->dtypep())) પાછા exprp;
        VNRelinker relinker;
        exprp->unlinkFrBack(&relinker);
        Ast_ગાંઠ_પદાવલિ* અચલ newp = નવું AstCvtUnpackToUnpack{exprp->fileline(), exprp, dtypep};
        relinker.relink(newp);
        પાછા newp;
    }

    સ્થિર તાર્કિક isLogicalOperator(અચલ Ast_ગાંઠ* nodep) {
        // IEEE 1800-2023 Table 11-1 logical operators, which take only integral or real operands
        પાછા VN_IS(nodep, LogAnd) || VN_IS(nodep, LogEq) || VN_IS(nodep, LogIf)
               || VN_IS(nodep, LogNot) || VN_IS(nodep, LogOr);
    }
    શૂન્ય iterateCheckBool(Ast_ગાંઠ* parentp, અચલ અક્ષર* side, Ast_ગાંઠ* underp, Stage stage) {
        UASSERT_OBJ(stage == BOTH, parentp,
                    "Bad call");  // Booleans always self-determined so do BOTH at once
        // Underp is used in a self-determined but boolean context, reduce a
        // multibit number to one bit
        // stage is always BOTH so not passed as argument
        // underp may change as a result of replacement
        UASSERT_OBJ(underp, parentp, "Node has no child");
        underp = userIterateSubtreeReturnEdits(underp, WidthVP{SELF, BOTH}.p());
        UASSERT_OBJ(underp, parentp, "Node has no child");
        UASSERT_OBJ(underp->dtypep(), underp,
                    "Node has no type");  // Perhaps forgot to do a prelim visit on it?
        //
        // For DOUBLE under a logical op, add implied test against zero, never a warning
        Ast_ગાંઠ_DType* અચલ underVDTypep = underp ? underp->dtypep()->skipRefp() : nullptr;
        જો (underp && underVDTypep->isDouble()) {
            UINFO(6, "   spliceCvtCmpD0: " << underp);
            VNRelinker linker;
            underp->unlinkFrBack(&linker);
            Ast_ગાંઠ* અચલ newp
                = નવું AstNeqD{parentp->fileline(), VN_AS(underp, ગાંઠ_પદાવલિ),
                              નવું AstConst{parentp->fileline(), AstConst::RealDouble{}, 0.0}};
            linker.relink(newp);
        } નહીંતર જો (underVDTypep->isString() && !isLogicalOperator(parentp)) {
            // For STRING as a condition, likewise test against the empty string, never a
            // warning; a string holds no NUL, so nonzero means non-empty (IEEE 1800-2023 6.16)
            UINFO(6, "   spliceCvtCmpN0: " << underp);
            VNRelinker linker;
            underp->unlinkFrBack(&linker);
            Ast_ગાંઠ* અચલ newp
                = નવું AstNeqN{parentp->fileline(), VN_AS(underp, ગાંઠ_પદાવલિ),
                              નવું AstConst{parentp->fileline(), AstConst::String{}, ""}};
            linker.relink(newp);
        } નહીંતર જો (VN_IS(underVDTypep, ClassRefDType) || VN_IS(underVDTypep, IfaceRefDType)
                   || (VN_IS(underVDTypep, BasicDType)
                       && VN_AS(underVDTypep, BasicDType)->isCHandle())) {
            // Allow warning-free "if (handle)"
            VL_DO_DANGLING(fixWidthReduce(VN_AS(underp, ગાંઠ_પદાવલિ)), underp);  // Changed
        } નહીંતર જો (!underVDTypep->basicp() || underVDTypep->isAggregateType()
                   || underVDTypep->isString()) {
            parentp->v3error(ucfirst(parentp->prettyOperatorName())
                             << " expects a numeric " << side << " operand, but it has data type "
                             << underVDTypep->prettyDTypeNameQ() << " (IEEE 1800-2023 11.3)");
            underp->replaceWith(નવું AstConst{parentp->fileline(), AstConst::BitFalseErroring{}});
            VL_DO_DANGLING(ધકેલો_Deletep(underp), underp);
        } નહીંતર {
            અચલ તાર્કિક bad = widthBad(underp, parentp->findBitDType());
            જો (bad) {
                {  // if (warnOn), but not needed here
                    UINFOTREE(5, parentp->backp(), "", "back");
                    parentp->v3widthWarn(1, underp->width(),
                                         "Logical operator "
                                             << parentp->prettyTypeName()
                                             << " expects 1 bit on the " << side << ", but "
                                             << side << "'s " << underp->prettyTypeName()
                                             << " generates " << underp->width()
                                             << (underp->width() != underp->widthMin()
                                                     ? " or " + cvtToStr(underp->widthMin())
                                                     : "")
                                             << " bits.");
                }
            }
            // Reduce to one bit even when the width is not reported, such as an unsized
            // (b & 1), so later passes see a 1-bit condition
            જો (Ast_પદાવલિ_Stmt* અચલ પદાવલિ_Stmtp = VN_CAST(underp, પદાવલિ_Stmt)) {
                // Reduce only the result, leaving the statements in place
                fixWidthReduce(પદાવલિ_Stmtp->resultp());
                પદાવલિ_Stmtp->dtypeFrom(પદાવલિ_Stmtp->resultp());
            } નહીંતર જો (underp->width() != 1) {
                VL_DO_DANGLING(fixWidthReduce(VN_AS(underp, ગાંઠ_પદાવલિ)), underp);  // Changed
            }
        }
    }

    સ્થિર શૂન્ય checkEnum(Ast_ગાંઠ* nodep, Ast_ગાંઠ* parentp, અચલ Ast_ગાંઠ_DType* targetDTypep,
                          અચલ Ast_ગાંઠ_DType* sourceDTypep, અચલ Ast_ગાંઠ_પદાવલિ* sourceExprp) {
        જો (અચલ AstEnumDType* અચલ targetEnump
            = VN_CAST(targetDTypep->skipRefToEnump(), EnumDType)) {
            અચલ VCastable castable
                = Ast_ગાંઠ::computeCastable(targetEnump, sourceDTypep, sourceExprp);
            જો (castable != VCastable::SAMEISH && castable != VCastable::COMPATIBLE
                && castable != VCastable::ENUM_IMPLICIT) {
                nodep->v3warn(ENUMVALUE, "Implicit conversion to enum "
                                             << targetDTypep->prettyDTypeNameQ() << " from "
                                             << sourceDTypep->prettyDTypeNameQ()
                                             << " (IEEE 1800-2023 6.19.3)\n"
                                             << parentp->warnMore()
                                             << "... Suggest use enum's mnemonic, or static cast");
            }
        }
    }

    Ast_ગાંઠ* iterateCheck(Ast_ગાંઠ* parentp, અચલ અક્ષર* side, Ast_ગાંઠ* underp, Determ determ,
                          Stage stage, Ast_ગાંઠ_DType* expDTypep, ExtendRule extendRule,
                          તાર્કિક warnOn = સાચું, StreamUse streamUse = STREAM_USE_NONE,
                          VAccess rhsAccess = VAccess::READ) {
        // Perform data type check on underp, which is underneath parentp used for error reporting
        // Returns the new underp
        // Conversion to/from doubles and integers are before iterating.
        UASSERT_OBJ(stage == FINAL, parentp, "Bad state to iterateCheck");
        UASSERT_OBJ(underp, parentp, "Node has no child");
        UASSERT_OBJ(underp->dtypep(), underp,
                    "Node has no type");  // Perhaps forgot to do a prelim visit on it?
        અચલ StreamUse childStreamUse
            = streamUse != STREAM_USE_NONE ? streamUse : streamUseForDeterm(determ);
        જો (rhsAccess.isReadOnly()) {
            જો (Ast_ગાંઠ_પદાવલિ* અચલ exprp = VN_CAST(underp, ગાંઠ_પદાવલિ)) {
                // A value only read is converted to a reversed expected type.
                underp = convertUnpackedOrder(exprp, expDTypep);
            }
        }
        જો (VN_IS(underp, ગાંઠ_DType)) {  // Note the node itself, not node's data type
            // Must be near top of these checks as underp->dtypep() will look normal
            underp->v3error(ucfirst(parentp->prettyOperatorName())
                            << " expected non-datatype " << side << " but "
                            << underp->prettyNameQ() << " is a datatype.");
        } નહીંતર જો (expDTypep == underp->dtypep()) {  // Perfect
            underp = userIterateSubtreeReturnEdits(underp,
                                                   WidthVP{expDTypep, FINAL, childStreamUse}.p());
        } નહીંતર જો (expDTypep->isDouble() && underp->isDouble()) {  // Also good
            underp = userIterateSubtreeReturnEdits(underp,
                                                   WidthVP{expDTypep, FINAL, childStreamUse}.p());
        } નહીંતર જો (expDTypep->isDouble() && !underp->isDouble()) {
            Ast_ગાંઠ* અચલ oldp
                = underp;  // Need FINAL on children; otherwise splice would block it
            spliceCvtD(VN_AS(underp, ગાંઠ_પદાવલિ));
            underp = userIterateSubtreeReturnEdits(oldp, WidthVP{SELF, FINAL, childStreamUse}.p());
        } નહીંતર જો (!expDTypep->isDouble() && underp->isDouble()) {
            Ast_ગાંઠ* અચલ oldp
                = underp;  // Need FINAL on children; otherwise splice would block it
            spliceCvtS(VN_AS(underp, ગાંઠ_પદાવલિ), સાચું, expDTypep->width());  // Round RHS
            underp = userIterateSubtreeReturnEdits(oldp, WidthVP{SELF, FINAL, childStreamUse}.p());
        } નહીંતર જો (expDTypep->isString() && !underp->dtypep()->isString()) {
            Ast_ગાંઠ* અચલ oldp
                = underp;  // Need FINAL on children; otherwise splice would block it
            spliceCvtString(VN_AS(underp, ગાંઠ_પદાવલિ));
            underp = userIterateSubtreeReturnEdits(oldp, WidthVP{SELF, FINAL, childStreamUse}.p());
        } નહીંતર {
            અચલ Ast_ગાંઠ_DType* અચલ underDtp = underp->dtypep()->skipRefp();
            જો (determ != ASSIGN && expDTypep->skipRefp()->isIntegralOrPacked()
                && underDtp->isAggregateType()) {
                parentp->v3error(ucfirst(parentp->prettyOperatorName())
                                 << " expects a numeric " << side
                                 << " operand, but it has data type "
                                 << underDtp->prettyDTypeNameQ() << " (IEEE 1800-2023 11.3)");
                Ast_ગાંઠ* અચલ newp
                    = નવું AstConst{underp->fileline(), AstConst::BitFalseErroring{}};
                underp->replaceWith(newp);
                VL_DO_DANGLING(ધકેલો_Deletep(underp), underp);
                underp = newp;
            }
            અચલ Ast_ગાંઠ_DType* અચલ expDtp = expDTypep->skipRefp();
            જો (determ == ASSIGN && VN_IS(expDtp, ગાંઠ_UOr_Struct_DType) && expDtp->isAggregateType()
                && !underDtp->isAggregateType()) {
                parentp->v3error("Illegal assignment: types are not assignment compatible (IEEE "
                                 "1800-2023 7.6)"
                                 << warnDTypes(parentp, expDtp, underDtp));
            }
            // Connections of interface arrays have their own check, with a better message
            જો (determ == ASSIGN && !isIfaceArrayDType(expDtp)
                && !unpackedArraySizesMatch(expDtp, underDtp)) {
                parentp->v3error("Illegal assignment: Unmatched array sizes (IEEE 1800-2023 7.6)"
                                 << warnDTypes(parentp, expDtp, underDtp));
            }
            અચલ AstBasicDType* અચલ expBasicp = expDTypep->basicp();
            અચલ AstBasicDType* અચલ underBasicp = underp->dtypep()->basicp();
            જો (expBasicp && underBasicp) {
                જો (!m_enumItemp && !parentp->fileline()->warnIsOff(V3ErrorCode::ENUMVALUE)
                    && warnOn) {
                    જો (rhsAccess.isReadOrRW()) {
                        checkEnum(underp, parentp, expDTypep, underp->dtypep(),
                                  VN_AS(underp, ગાંઠ_પદાવલિ));
                    }
                    જો (rhsAccess.isWriteOrRW()) {
                        checkEnum(underp, parentp, underp->dtypep(), expDTypep, nullptr);
                    }
                }
                Ast_ગાંઠ_DType* subDTypep = expDTypep;
                // We then iterate FINAL before width fixes, as if the under-operation
                // is e.g. an ADD, the ADD will auto-adjust to the proper data type
                // or if another operation e.g. ATOI will not.
                જો (determ == SELF) {
                    underp = userIterateSubtreeReturnEdits(
                        underp, WidthVP{SELF, FINAL, childStreamUse}.p());
                } નહીંતર જો (determ == ASSIGN) {
                    // IEEE: Signedness is solely determined by the RHS
                    // (underp), not by the LHS (expDTypep)
                    જો (underp->isSigned() != subDTypep->isSigned()
                        || underp->width() != subDTypep->width()) {
                        subDTypep = parentp->findBitOrLogicDType(
                            std::max(subDTypep->width(), underp->width()),
                            std::max(subDTypep->widthMin(), underp->widthMin()),
                            VSigning::fromBool(underp->isSigned()),
                            subDTypep->skipRefp()->isFourstate()
                                || underp->dtypep()->skipRefp()->isFourstate());
                        UINFO(9, "Assignment of opposite-signed RHS to LHS: " << parentp);
                    }
                    underp = userIterateSubtreeReturnEdits(
                        underp, WidthVP{subDTypep, FINAL, childStreamUse}.p());
                } નહીંતર {
                    underp = userIterateSubtreeReturnEdits(
                        underp, WidthVP{subDTypep, FINAL, childStreamUse}.p());
                }
                // Note the check uses the expected size, not the child's subDTypep as we want the
                // child node's width to end up correct for the assignment (etc)
                widthCheckSized(parentp, side, VN_AS(underp, ગાંઠ_પદાવલિ), expDTypep, extendRule,
                                warnOn);
            } નહીંતર જો (!VN_IS(parentp, Eq) && !VN_IS(parentp, Neq)
                       && !VN_IS(expDTypep->skipRefp(), IfaceRefDType)
                       && VN_IS(underp->dtypep()->skipRefp(), IfaceRefDType)) {
                underp->v3error(ucfirst(parentp->prettyOperatorName())
                                << " expected non-interface on " << side << " but "
                                << underp->prettyNameQ() << " is an interface.");
            } નહીંતર જો (અચલ AstIfaceRefDType* expIfaceRefp
                       = VN_CAST(expDTypep->skipRefp(), IfaceRefDType)) {
                અચલ AstIfaceRefDType* underIfaceRefp
                    = VN_CAST(underp->dtypep()->skipRefp(), IfaceRefDType);
                જો (VN_IS(underp, Const) && VN_AS(underp, Const)->અંક().isNull()) {
                    // '= null' is ok
                } નહીંતર જો (!underIfaceRefp) {
                    underp->v3error(ucfirst(parentp->prettyOperatorName())
                                    << " expected " << expIfaceRefp->ifaceViaCellp()->prettyNameQ()
                                    << " interface on " << side << " but " << underp->prettyNameQ()
                                    << " is not an interface.");
                } નહીંતર જો (expIfaceRefp->ifaceViaCellp() != underIfaceRefp->ifaceViaCellp()) {
                    underp->v3error(ucfirst(parentp->prettyOperatorName())
                                    << " expected " << expIfaceRefp->ifaceViaCellp()->prettyNameQ()
                                    << " interface on " << side << " but " << underp->prettyNameQ()
                                    << " is a different interface ("
                                    << underIfaceRefp->ifaceViaCellp()->prettyNameQ() << ").");
                } નહીંતર જો (underIfaceRefp->modportp()
                           && expIfaceRefp->modportp() != underIfaceRefp->modportp()) {
                    underp->v3error(ucfirst(parentp->prettyOperatorName())
                                    << " expected "
                                    << (expIfaceRefp->modportp()
                                            ? expIfaceRefp->modportp()->prettyNameQ()
                                            : "no")
                                    << " interface modport on " << side << " but got "
                                    << underIfaceRefp->modportp()->prettyNameQ() << " modport.");
                } નહીંતર {
                    underp = userIterateSubtreeReturnEdits(
                        underp, WidthVP{expDTypep, FINAL, childStreamUse}.p());
                }
            } નહીંતર {
                // Hope it just works out (perhaps a cast will deal with it)
                underp = userIterateSubtreeReturnEdits(
                    underp, WidthVP{expDTypep, FINAL, childStreamUse}.p());
            }
        }
        પાછા underp;
    }

    શૂન્ય
    widthCheckSized(Ast_ગાંઠ* parentp, અચલ અક્ષર* side,
                    Ast_ગાંઠ_પદાવલિ* underp,  // Node to be checked or have typecast added in front of
                    Ast_ગાંઠ_DType* expDTypep, ExtendRule extendRule, તાર્કિક warnOn = સાચું) {
        // Issue warnings on sized number width mismatches, then do appropriate size extension
        // Generally iterateCheck is what is wanted instead of this
        // UINFO(9,"wchk "<<side<<endl<<"  "<<parentp<<endl<<"  "<<underp<<endl<<"
        // e="<<expDTypep<<" i"<<warnOn<<endl);
        અચલ AstBasicDType* અચલ expBasicp = expDTypep->basicp();
        અચલ AstBasicDType* અચલ underBasicp = underp->dtypep()->basicp();
        જો (expDTypep == underp->dtypep()) {
            પાછા;  // Same type must match
        } નહીંતર જો (!expBasicp || expBasicp->isDouble() || !underBasicp
                   || underBasicp->isDouble()) {
            // This is perhaps a v3fatalSrc as we should have checked the types
            // before calling widthCheck, but we may have missed a non-sized
            // check in earlier code, so might as well assume it is the users'
            // fault.
            parentp->v3error(ucfirst(parentp->prettyOperatorName())
                             << " expected non-complex non-double " << side << " in width check");
#જો VL_DEBUG
            parentp->v3fatalSrc("widthCheckSized should not be called on doubles/complex types");
#endif
            પાછા;
        } નહીંતર {
            અચલ પૂર્ણાંક expWidth = expDTypep->width();
            પૂર્ણાંક expWidthMin = expDTypep->widthMin();
            જો (expWidthMin == 0) expWidthMin = expWidth;
            અચલ તાર્કિક bad = widthBad(underp, expDTypep);
            જો ((bad || underp->width() != expWidth) && fixAutoExtend(underp /*ref*/, expWidth)) {
                underp = nullptr;  // Changes underp
                પાછા;
            }
            // If user has a sizing cast, assume they know what they are doing
            // (for better or worse)
            જો (VN_IS(parentp->backp(), CastSize)) warnOn = ખોટું;
            જો (parentp->fileline()->erroringOn() || underp->fileline()->erroringOn())
                warnOn = ખોટું;
            જો (VN_IS(underp, Const) && VN_AS(underp, Const)->અંક().isFromString()
                && expWidth > underp->width()
                && (((expWidth - underp->width()) % 8) == 0)) {  // At least it's character sized
                // reg [31:0] == "foo" we'll consider probably fine.
                // Maybe this should be a special warning?  Not for now.
                warnOn = ખોટું;
            }
            જો ((VN_IS(parentp, Add) && underp->width() == 1 && underp->isOne())
                || (VN_IS(parentp, Sub) && underp->width() == 1 && underp->isOne()
                    && 0 == std::strcmp(side, "RHS"))) {
                // "foo + 1'b1", or "foo - 1'b1" are very common, people assume
                // they extend correctly
                warnOn = ખોટું;
            }
            જો (bad && warnOn) {
                UINFOTREE(5, parentp->backp(), "", "back");

                parentp->v3widthWarn(
                    expWidth, underp->width(),
                    ucfirst(parentp->prettyOperatorName())
                        << " expects " << expWidth
                        << (expWidth != expWidthMin ? " or " + cvtToStr(expWidthMin) : "")
                        << " bits on the " << side << ", but " << side << "'s "
                        << underp->prettyTypeName() << " generates " << underp->width()
                        << (underp->width() != underp->widthMin()
                                ? " or " + cvtToStr(underp->widthMin())
                                : "")
                        << " bits.");
            }
            જો (bad || underp->width() != expWidth) {
                // If we're in an NodeAssign, don't truncate the RHS if the LHS is
                // a NodeStream. The streaming operator changes the rules regarding
                // which bits to truncate.
                અચલ Ast_ગાંઠ_Assign* assignp = VN_CAST(parentp, ગાંઠ_Assign);
                અચલ AstPin* pinp = VN_CAST(parentp, Pin);
                જો (assignp && VN_IS(assignp->lhsp(), ગાંઠ_Stream)) {
                } નહીંતર જો (pinp && pinp->modVarp()->direction() != VDirection::આગત) {
                    // V3Inst::pinReconnectSimple must deal
                    UINFO(5, "pinInSizeMismatch: " << pinp);
                } નહીંતર જો (assignp && VN_IS(underp, ગાંઠ_Stream) && expWidth > underp->width()) {
                    // IEEE 1800-2023 11.4.14: When assigning a stream to a wider fixed-size
                    // target, widen by filling zero bits on the right. That is, left-justify
                    // the stream bits within the target width.
                    // Build: ShiftL(Extend(stream, expWidth), expWidth - streamWidth)
                    UINFO(5, "Widen NodeStream RHS with left-justify per 11.4.14");
                    VNRelinker linker;
                    અચલ પૂર્ણાંક shift = expWidth - underp->width();
                    underp->unlinkFrBack(&linker);
                    AstExtend* અચલ widenedp = નવું AstExtend{underp->fileline(), underp};
                    widenedp->didWidth(સાચું);
                    // Shift left so zeros fill on the right
                    Ast_ગાંઠ_પદાવલિ* અચલ shiftedp = નવું AstShiftL{
                        underp->fileline(), widenedp,
                        નવું AstConst{underp->fileline(), static_cast<uint32_t>(shift)}, expWidth};
                    // Final dtype should match expected
                    shiftedp->dtypep(expDTypep);
                    linker.relink(shiftedp);
                } નહીંતર {
                    VL_DO_DANGLING(fixWidthExtend(underp, expDTypep, extendRule), underp);
                }
            }
        }
    }

    //----------------------------------------------------------------------
    // SIGNED/DOUBLE METHODS

    Ast_ગાંઠ_પદાવલિ* checkCvtUS(Ast_ગાંઠ_પદાવલિ* nodep, તાર્કિક fatal) {
        જો (nodep && nodep->dtypep()->skipRefp()->isDouble()) {
            જો (fatal) {
                nodep->v3error("Expected integral input to " << nodep->backp()->prettyTypeName());
            } નહીંતર {
                nodep->v3warn(REALCVT,
                              "Implicit conversion of real to integer; expected integral input to "
                                  << nodep->backp()->prettyTypeName());
            }
            nodep = spliceCvtS(nodep, ખોટું, 32);
        }
        પાછા nodep;
    }

    Ast_ગાંઠ_પદાવલિ* spliceCvtD(Ast_ગાંઠ_પદાવલિ* nodep) {
        // For integer used in REAL context, convert to real
        // We don't warn here, "2.0 * 2" is common and reasonable
        જો (nodep && !nodep->dtypep()->skipRefp()->isDouble()) {
            UINFO(6, "   spliceCvtD: " << nodep);
            VNRelinker linker;
            nodep->unlinkFrBack(&linker);
            Ast_ગાંઠ_પદાવલિ* newp;
            જો (nodep->dtypep()->skipRefp()->isSigned()) {
                newp = નવું AstISToRD{nodep->fileline(), nodep};
            } નહીંતર {
                newp = નવું AstIToRD{nodep->fileline(), nodep};
            }
            linker.relink(newp);
            પાછા newp;
        } નહીંતર {
            પાછા nodep;
        }
    }
    Ast_ગાંઠ_પદાવલિ* spliceCvtS(Ast_ગાંઠ_પદાવલિ* nodep, તાર્કિક warnOn, પૂર્ણાંક width) {
        // IEEE-2012 11.8.1: Signed: Type coercion creates signed
        // 11.8.2: Argument to convert is self-determined
        જો (nodep && nodep->dtypep()->skipRefp()->isDouble()) {
            UINFO(6, "   spliceCvtS: " << nodep);
            VNRelinker linker;
            nodep->unlinkFrBack(&linker);
            જો (અચલ AstConst* અચલ constp = VN_CAST(nodep, Const)) {
                // We convert to/from int32_t rather than use floor() as want to make sure is
                // representable in integer's number of bits
                જો (constp->isDouble() && V3_સંખ્યા::epsilonIntegral(constp->અંક().toDouble())) {
                    warnOn = ખોટું;
                }
            }
            જો (warnOn) nodep->v3warn(REALCVT, "Implicit conversion of real to integer");
            Ast_ગાંઠ_પદાવલિ* અચલ newp = નવું AstRToIRoundS{nodep->fileline(), nodep};
            linker.relink(newp);
            newp->dtypeSetBitSized(width, VSigning::SIGNED);
            પાછા newp;
        } નહીંતર {
            પાછા nodep;
        }
    }
    Ast_ગાંઠ_પદાવલિ* spliceCvtString(Ast_ગાંઠ_પદાવલિ* nodep) {
        // IEEE-2012 11.8.1: Signed: Type coercion creates signed
        // 11.8.2: Argument to convert is self-determined
        જો (nodep && !(nodep->dtypep()->basicp() && nodep->dtypep()->basicp()->isString())) {
            UINFO(6, "   spliceCvtString: " << nodep);
            VNRelinker linker;
            nodep->unlinkFrBack(&linker);
            Ast_ગાંઠ_પદાવલિ* અચલ newp = નવું AstCvtPackString{nodep->fileline(), nodep};
            linker.relink(newp);
            પાછા newp;
        } નહીંતર {
            પાછા nodep;
        }
    }
    Ast_ગાંઠ_Biop* replaceWithUOrSVersion(Ast_ગાંઠ_Biop* nodep, તાર્કિક signedFlavorNeeded) {
        // Given a signed/unsigned node type, create the opposite type
        // Return new node or nullptr if nothing
        જો (signedFlavorNeeded == nodep->signedFlavor()) પાછા nullptr;
        જો (!nodep->dtypep()) nodep->dtypeFrom(nodep->lhsp());
        // To simplify callers, some node types don't need to change
        પસંદગી (nodep->type()) {
        સ્થિતિ VNType::Eq: nodep->dtypeChgSigned(signedFlavorNeeded); પાછા nullptr;
        સ્થિતિ VNType::Neq: nodep->dtypeChgSigned(signedFlavorNeeded); પાછા nullptr;
        સ્થિતિ VNType::EqCase: nodep->dtypeChgSigned(signedFlavorNeeded); પાછા nullptr;
        સ્થિતિ VNType::NeqCase: nodep->dtypeChgSigned(signedFlavorNeeded); પાછા nullptr;
        સ્થિતિ VNType::EqWild: nodep->dtypeChgSigned(signedFlavorNeeded); પાછા nullptr;
        સ્થિતિ VNType::NeqWild: nodep->dtypeChgSigned(signedFlavorNeeded); પાછા nullptr;
        સ્થિતિ VNType::Add: nodep->dtypeChgSigned(signedFlavorNeeded); પાછા nullptr;
        સ્થિતિ VNType::Sub: nodep->dtypeChgSigned(signedFlavorNeeded); પાછા nullptr;
        સ્થિતિ VNType::ShiftL: nodep->dtypeChgSigned(signedFlavorNeeded); પાછા nullptr;
        મૂળભૂત: તોડો;
        }
        File_લીટી* અચલ fl = nodep->fileline();
        Ast_ગાંઠ_પદાવલિ* અચલ lhsp = nodep->lhsp()->unlinkFrBack();
        Ast_ગાંઠ_પદાવલિ* અચલ rhsp = nodep->rhsp()->unlinkFrBack();
        Ast_ગાંઠ_Biop* newp = nullptr;
        પસંદગી (nodep->type()) {
        સ્થિતિ VNType::Gt: newp = નવું AstGtS{fl, lhsp, rhsp}; તોડો;
        સ્થિતિ VNType::GtS: newp = નવું AstGt{fl, lhsp, rhsp}; તોડો;
        સ્થિતિ VNType::Gte: newp = નવું AstGteS{fl, lhsp, rhsp}; તોડો;
        સ્થિતિ VNType::GteS: newp = નવું AstGte{fl, lhsp, rhsp}; તોડો;
        સ્થિતિ VNType::Lt: newp = નવું AstLtS{fl, lhsp, rhsp}; તોડો;
        સ્થિતિ VNType::LtS: newp = નવું AstLt{fl, lhsp, rhsp}; તોડો;
        સ્થિતિ VNType::Lte: newp = નવું AstLteS{fl, lhsp, rhsp}; તોડો;
        સ્થિતિ VNType::LteS: newp = નવું AstLte{fl, lhsp, rhsp}; તોડો;
        સ્થિતિ VNType::Div: newp = નવું AstDivS{fl, lhsp, rhsp}; તોડો;
        સ્થિતિ VNType::DivS: newp = નવું AstDiv{fl, lhsp, rhsp}; તોડો;
        સ્થિતિ VNType::ModDiv: newp = નવું AstModDivS{fl, lhsp, rhsp}; તોડો;
        સ્થિતિ VNType::ModDivS: newp = નવું AstModDiv{fl, lhsp, rhsp}; તોડો;
        સ્થિતિ VNType::Mul: newp = નવું AstMulS{fl, lhsp, rhsp}; તોડો;
        સ્થિતિ VNType::MulS: newp = નવું AstMul{fl, lhsp, rhsp}; તોડો;
        સ્થિતિ VNType::ShiftR: newp = નવું AstShiftRS{fl, lhsp, rhsp}; તોડો;
        સ્થિતિ VNType::ShiftRS: newp = નવું AstShiftR{fl, lhsp, rhsp}; તોડો;
        મૂળભૂત:  // LCOV_EXCL_LINE
            nodep->v3fatalSrc("Node needs sign change, but bad case: " << nodep);
            તોડો;
        }
        UINFO(6, "   ReplaceWithUOrSVersion: " << nodep << " w/ " << newp);
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
        પાછા newp;
    }
    Ast_ગાંઠ_Biop* replaceWithDVersion(Ast_ગાંઠ_Biop* nodep) {
        // Given a signed/unsigned node type, create the opposite type
        // Return new node or nullptr if nothing
        જો (nodep->doubleFlavor()) પાછા nullptr;
        File_લીટી* અચલ fl = nodep->fileline();
        Ast_ગાંઠ_પદાવલિ* અચલ lhsp = nodep->lhsp()->unlinkFrBack();
        Ast_ગાંઠ_પદાવલિ* અચલ rhsp = nodep->rhsp()->unlinkFrBack();
        Ast_ગાંઠ_Biop* newp = nullptr;
        // No width change on output;...                // All below have bool or double outputs
        પસંદગી (nodep->type()) {
        સ્થિતિ VNType::Add: newp = નવું AstAddD{fl, lhsp, rhsp}; તોડો;
        સ્થિતિ VNType::Sub: newp = નવું AstSubD{fl, lhsp, rhsp}; તોડો;
        સ્થિતિ VNType::Pow: newp = નવું AstPowD{fl, lhsp, rhsp}; તોડો;
        સ્થિતિ VNType::Eq:
        સ્થિતિ VNType::EqCase: newp = નવું AstEqD{fl, lhsp, rhsp}; તોડો;
        સ્થિતિ VNType::Neq:
        સ્થિતિ VNType::NeqCase: newp = નવું AstNeqD{fl, lhsp, rhsp}; તોડો;
        સ્થિતિ VNType::Gt:
        સ્થિતિ VNType::GtS: newp = નવું AstGtD{fl, lhsp, rhsp}; તોડો;
        સ્થિતિ VNType::Gte:
        સ્થિતિ VNType::GteS: newp = નવું AstGteD{fl, lhsp, rhsp}; તોડો;
        સ્થિતિ VNType::Lt:
        સ્થિતિ VNType::LtS: newp = નવું AstLtD{fl, lhsp, rhsp}; તોડો;
        સ્થિતિ VNType::Lte:
        સ્થિતિ VNType::LteS: newp = નવું AstLteD{fl, lhsp, rhsp}; તોડો;
        સ્થિતિ VNType::Div:
        સ્થિતિ VNType::DivS: newp = નવું AstDivD{fl, lhsp, rhsp}; તોડો;
        સ્થિતિ VNType::Mul:
        સ્થિતિ VNType::MulS: newp = નવું AstMulD{fl, lhsp, rhsp}; તોડો;
        મૂળભૂત:  // LCOV_EXCL_LINE
            nodep->v3fatalSrc("Node needs conversion to double, but bad case: " << nodep);
            તોડો;
        }
        UINFO(6, "   ReplaceWithDVersion: " << nodep << " w/ " << newp);
        nodep->replaceWith(newp);
        // No width change; the default created type (bool or double) is correct
        VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
        પાછા newp;
    }
    Ast_ગાંઠ_Biop* replaceWithNVersion(Ast_ગાંઠ_Biop* nodep) {
        // Given a signed/unsigned node type, replace with string version
        // Return new node or nullptr if nothing
        જો (nodep->stringFlavor()) પાછા nullptr;
        File_લીટી* અચલ fl = nodep->fileline();
        Ast_ગાંઠ_પદાવલિ* અચલ lhsp = nodep->lhsp()->unlinkFrBack();
        Ast_ગાંઠ_પદાવલિ* અચલ rhsp = nodep->rhsp()->unlinkFrBack();
        Ast_ગાંઠ_Biop* newp = nullptr;
        // No width change on output;...                // All below have bool or double outputs
        પસંદગી (nodep->type()) {
        સ્થિતિ VNType::Eq:
        સ્થિતિ VNType::EqCase:
        સ્થિતિ VNType::EqWild: newp = નવું AstEqN{fl, lhsp, rhsp}; તોડો;
        સ્થિતિ VNType::Neq:
        સ્થિતિ VNType::NeqCase:
        સ્થિતિ VNType::NeqWild: newp = નવું AstNeqN{fl, lhsp, rhsp}; તોડો;
        સ્થિતિ VNType::Gt:
        સ્થિતિ VNType::GtS: newp = નવું AstGtN{fl, lhsp, rhsp}; તોડો;
        સ્થિતિ VNType::Gte:
        સ્થિતિ VNType::GteS: newp = નવું AstGteN{fl, lhsp, rhsp}; તોડો;
        સ્થિતિ VNType::Lt:
        સ્થિતિ VNType::LtS: newp = નવું AstLtN{fl, lhsp, rhsp}; તોડો;
        સ્થિતિ VNType::Lte:
        સ્થિતિ VNType::LteS: newp = નવું AstLteN{fl, lhsp, rhsp}; તોડો;
        મૂળભૂત:  // LCOV_EXCL_LINE
            nodep->v3fatalSrc("Node needs conversion to string, but bad case: " << nodep);
            તોડો;
        }
        UINFO(6, "   ReplaceWithNVersion: " << nodep << " w/ " << newp);
        nodep->replaceWith(newp);
        // No width change; the default created type (bool or string) is correct
        VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
        પાછા newp;
    }
    Ast_ગાંઠ_Uniop* replaceWithDVersion(Ast_ગાંઠ_Uniop* nodep) {
        // Given a signed/unsigned node type, create the opposite type
        // Return new node or nullptr if nothing
        જો (nodep->doubleFlavor()) પાછા nullptr;
        File_લીટી* અચલ fl = nodep->fileline();
        Ast_ગાંઠ_પદાવલિ* અચલ lhsp = nodep->lhsp()->unlinkFrBack();
        Ast_ગાંઠ_Uniop* newp = nullptr;
        પસંદગી (nodep->type()) {
        સ્થિતિ VNType::Negate: newp = નવું AstNegateD{fl, lhsp}; તોડો;
        મૂળભૂત:  // LCOV_EXCL_LINE
            nodep->v3fatalSrc("Node needs conversion to double, but bad case: " << nodep);
            તોડો;
        }
        UINFO(6, "   ReplaceWithDVersion: " << nodep << " w/ " << newp);
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
        પાછા newp;
    }

    //----------------------------------------------------------------------
    // METHODS - strings

    શૂન્ય replaceWithSFormat(AstMethodCall* nodep, અચલ string& format) {
        // For string.itoa and similar, replace with SFormatF
        AstArg* અચલ argp = nodep->argsp();
        UASSERT_OBJ(argp, nodep,
                    "Argument needed for string method, call methodOkArguments before here");
        Ast_ગાંઠ_Var_Ref* અચલ fromp = VN_AS(nodep->fromp()->unlinkFrBack(), VarRef);
        Ast_ગાંઠ* અચલ newp = નવું AstAssign{
            nodep->fileline(), fromp,
            નવું AstSFormatF{nodep->fileline(), format, ખોટું, argp->exprp()->unlinkFrBack()}};
        fromp->access(VAccess::WRITE);
        ધકેલો_Deletep(nodep->backp());
        VL_DO_DANGLING(nodep->backp()->replaceWith(newp), newp);
    }

    //----------------------------------------------------------------------
    // METHODS - data types

    Ast_ગાંઠ_DType* iterateEditMoveDTypep(Ast_ગાંઠ* parentp, Ast_ગાંઠ_DType* dtnodep) {
        UASSERT_OBJ(dtnodep, parentp, "Caller should check for nullptr before computing dtype");
        // Iterate into a data type to resolve that type.
        // The data type may either:
        // 1. Be a child (typically getChildDTypep() returns it)
        //    DTypes at parse time get added as these to some node types
        //    such as AstVars.
        //    This function will move it to global scope (that is #2
        //    will now apply).
        // 2. Be under the Netlist and pointed to by an Ast member variable
        //    (typically refDTypep() or dtypep() returns it)
        //    so removing/changing a variable won't lose the dtype

        // Case #1 above applies?
        અચલ તાર્કિક child1 = (parentp->getChildDTypep() == dtnodep);
        અચલ તાર્કિક child2 = (parentp->getChild2DTypep() == dtnodep);
        જો (child1 || child2) {
            UINFO(9, "iterateEditMoveDTypep child iterating " << dtnodep);
            // Iterate, this might edit the dtypes which means dtnodep now lost
            VL_DO_DANGLING(userIterate(dtnodep, nullptr), dtnodep);
            // Figure out the new dtnodep, remained a child of parent so find it there
            dtnodep = child1 ? parentp->getChildDTypep() : parentp->getChild2DTypep();
            UASSERT_OBJ(dtnodep, parentp, "iterateEditMoveDTypep lost pointer to child");
            UASSERT_OBJ(dtnodep->didWidth(), parentp,
                        "iterateEditMoveDTypep didn't get width resolution of "
                            << dtnodep->prettyTypeName());

            // Move to under netlist
            UINFO(9, "iterateEditMoveDTypep child moving " << dtnodep);
            dtnodep->unlinkFrBack();
            v3Global.rootp()->typeTablep()->addTypesp(dtnodep);
        }
        જો (!dtnodep->didWidth()) {
            UINFO(9, "iterateEditMoveDTypep pointer iterating " << dtnodep);
            // See notes in visit(AstBracketArrayDType*)
            UASSERT_OBJ(!VN_IS(dtnodep, BracketArrayDType), parentp,
                        "Brackets should have been iterated as children");
            userIterate(dtnodep, nullptr);
            UASSERT_OBJ(dtnodep->didWidth(), parentp,
                        "iterateEditMoveDTypep didn't get width resolution");
        }
        પાછા dtnodep;
    }

    AstConst* dimension_કિંમત(File_લીટી* fileline, Ast_ગાંઠ_DType* nodep, VAttrType attrType,
                             પૂર્ણાંક dim) {
        // Return the dimension value for the specified attribute and constant dimension
        Ast_ગાંઠ_DType* dtypep = nodep->skipRefp();
        VNumRange declRange;  // ranged() set false
        માટે (પૂર્ણાંક i = 1; i <= dim; ++i) {
            // UINFO(9, "   dim at "<<dim<<"  "<<dtypep);
            declRange = VNumRange{};  // ranged() set false
            જો (અચલ Ast_ગાંઠ_Array_DType* અચલ adtypep = VN_CAST(dtypep, ગાંઠ_Array_DType)) {
                declRange = adtypep->declRange();
                જો (i < dim) dtypep = adtypep->subDTypep()->skipRefp();
                ચાલુ;
            } નહીંતર જો (અચલ Ast_ગાંઠ_UOr_Struct_DType* અચલ adtypep
                       = VN_CAST(dtypep, ગાંઠ_UOr_Struct_DType)) {
                declRange = adtypep->declRange();
                તોડો;  // Sub elements don't look like arrays and can't iterate into
            } નહીંતર જો (અચલ AstBasicDType* અચલ adtypep = VN_CAST(dtypep, BasicDType)) {
                જો (adtypep->isRanged()) declRange = adtypep->declRange();
                તોડો;
            }
            તોડો;  // LCOV_EXCL_LINE
        }
        AstConst* valp = nullptr;  // If nullptr, construct from val
        પૂર્ણાંક val = 0;
        પસંદગી (attrType) {
        સ્થિતિ VAttrType::DIM_BITS: {
            પૂર્ણાંક bits = 1;
            જ્યારે (dtypep) {
                // UINFO(9, "   bits at " << bits << "  " << dtypep);
                જો (અચલ Ast_ગાંઠ_Array_DType* અચલ adtypep = VN_CAST(dtypep, ગાંઠ_Array_DType)) {
                    bits *= adtypep->declRange().elements();
                    dtypep = adtypep->subDTypep()->skipRefp();
                    ચાલુ;
                } નહીંતર જો (અચલ Ast_ગાંઠ_UOr_Struct_DType* અચલ adtypep
                           = VN_CAST(dtypep, ગાંઠ_UOr_Struct_DType)) {
                    bits *= adtypep->isStreamableFixedAggregate() ? adtypep->widthStream()
                                                                  : adtypep->width();
                    તોડો;
                } નહીંતર જો (અચલ AstBasicDType* અચલ adtypep = VN_CAST(dtypep, BasicDType)) {
                    bits *= adtypep->width();
                    તોડો;
                }
                તોડો;
            }
            જો (dim == 0) {
                val = 0;
            } નહીંતર જો (dim == 1 && !declRange.ranged()
                       && bits == 1) {  // $bits should be sane for non-arrays
                val = nodep->width();
            } નહીંતર {
                val = bits;
            }
            તોડો;  // LCOV_EXCL_LINE
        }
        સ્થિતિ VAttrType::DIM_ઉપલું: val = !declRange.ranged() ? 0 : declRange.hi(); તોડો;
        સ્થિતિ VAttrType::DIM_ડાબું: val = !declRange.ranged() ? 0 : declRange.ડાબું(); તોડો;
        સ્થિતિ VAttrType::DIM_નીચલું: val = !declRange.ranged() ? 0 : declRange.lo(); તોડો;
        સ્થિતિ VAttrType::DIM_જમણું: val = !declRange.ranged() ? 0 : declRange.જમણું(); તોડો;
        સ્થિતિ VAttrType::DIM_INCREMENT:
            val = (declRange.ranged() && declRange.ascending()) ? -1 : 1;
            તોડો;
        સ્થિતિ VAttrType::DIM_SIZE: val = !declRange.ranged() ? 0 : declRange.elements(); તોડો;
        મૂળભૂત: nodep->v3fatalSrc("Missing DIM ATTR type case"); તોડો;
        }
        જો (!valp) valp = નવું AstConst{fileline, AstConst::Signed32{}, val};
        UINFO(9, " $dimension " << attrType.ascii() << "(" << cvtToHex(dtypep) << "," << dim
                                << ")=" << valp);
        પાછા valp;
    }
    AstVarRef* dimensionVarRefp(Ast_ગાંઠ_DType* nodep, VAttrType attrType, uint32_t msbdim) {
        // Return a reference to a constant table which has the specified dimension properties
        // for this data type. The reference is cached, so the table is built only once.
        AstVarRef*& cachedpr = m_tableMap[std::make_pair(nodep, attrType)];
        જો (cachedpr) પાછા cachedpr->clone_વૃક્ષ(ખોટું);

        Ast_ગાંઠ_Array_DType* અચલ vardtypep = નવું AstUnpackArrayDType{
            nodep->fileline(), nodep->findIntDType(), નવું AstRange(nodep->fileline(), msbdim, 0)};
        v3Global.rootp()->typeTablep()->addTypesp(vardtypep);
        userIterate(vardtypep, WidthVP{SELF, BOTH}.p());
        AstConsArray* અચલ consp = નવું AstConsArray{nodep->fileline(), vardtypep};
        માટે (unsigned i = 0; i < msbdim + 1; ++i) {
            consp->valuep(i, dimension_કિંમત(nodep->fileline(), nodep, attrType, i));
        }

        // Share identical tables via the constant pool
        cachedpr = V3ConstPool::find(consp);
        VL_DO_DANGLING(consp->delete_વૃક્ષ(), consp);  // V3ConstPool::find clones it
        cachedpr->varp()->didWidth(સાચું);
        // Keep the dtype of the value, as the entry might have a different, compatible one
        cachedpr->didWidth(સાચું);
        ધકેલો_Deletep(cachedpr);  // Deleted with the visitor - always cloned
        પાછા cachedpr->clone_વૃક્ષ(ખોટું);
    }
    સ્થિર uint64_t enum_Max_કિંમત(અચલ Ast_ગાંઠ* errNodep, અચલ AstEnumDType* adtypep) {
        // Most enums unless overridden are 32 bits, so we size array
        // based on max enum value used.
        // Ideally we would have a fast algorithm when a number is
        // of small width and complete and so can use an array, and
        // a map for when the value is many bits and sparse.
        uint64_t maxval = 0;
        માટે (અચલ AstEnumItem* itemp = adtypep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), EnumItem)) {
            અચલ AstConst* અચલ vconstp = VN_CAST(itemp->valuep(), Const);
            UASSERT_OBJ(vconstp, errNodep, "Enum item without constified value");
            જો (!vconstp->અંક().isAnyXZ() && vconstp->toUQuad() >= maxval)
                maxval = vconstp->toUQuad();
        }
        જો (adtypep->itemsp()->width() > 64) {
            errNodep->v3warn(E_UNSUPPORTED,
                             "Unsupported: enum next/prev method on enum with > 64 bits");
            પાછા 64;
        }
        પાછા maxval;
    }
    AstVarRef* enumVarRefp(AstEnumDType* અચલ nodep, VAttrType attrType, તાર્કિક assoc,
                           uint32_t msbdim) {
        // Return a reference to a constant table which has the specified attrType information
        // for this enum. The reference is cached, so the table is built only once.
        AstVarRef*& cachedpr = m_tableMap[std::make_pair(nodep, attrType)];
        જો (cachedpr) પાછા cachedpr->clone_વૃક્ષ(ખોટું);

        UINFO(9, "Construct Venumtab attr=" << attrType.ascii() << " assoc=" << assoc
                                            << " max=" << msbdim << " for " << nodep);
        File_લીટી* અચલ flp = nodep->fileline();
        Ast_ગાંઠ_DType* basep;
        જો (attrType == VAttrType::ENUM_NAME) {
            basep = nodep->findStringDType();
        } નહીંતર જો (attrType == VAttrType::ENUM_VALID) {
            // TODO in theory we could bit-pack the bits in the table, but
            // would require additional operations to extract, so only
            // would be worth it for larger tables which perhaps could be
            // better handled with equation generation?
            basep = nodep->findBitDType();
        } નહીંતર {
            basep = nodep->dtypep();
        }
        Ast_ગાંઠ_DType* vardtypep;
        જો (assoc) {
            // Key by the base type of the enum, not the enum itself, so identical maps of
            // different enum types can be shared in the constant pool
            vardtypep = નવું AstAssocArrayDType{flp, basep, nodep->subDTypep()};
        } નહીંતર {
            vardtypep = નવું AstUnpackArrayDType{flp, basep, નવું AstRange(flp, msbdim, 0)};
        }
        v3Global.rootp()->typeTablep()->addTypesp(vardtypep);

        // Default for all unspecified values
        AstConst* defaultp = nullptr;
        જો (attrType == VAttrType::ENUM_NAME) {
            defaultp = નવું AstConst{flp, AstConst::String{}, ""};
        } નહીંતર જો (attrType == VAttrType::ENUM_NEXT || attrType == VAttrType::ENUM_PREV) {
            defaultp = નવું AstConst{flp, V3_સંખ્યા{nodep, nodep->width(), 0}};
        } નહીંતર જો (attrType == VAttrType::ENUM_VALID) {
            defaultp = નવું AstConst{flp, AstConst::BitFalse{}};
        } નહીંતર {
            nodep->v3fatalSrc("Bad case");
        }

        // Find valid values and populate
        UASSERT_OBJ(nodep->itemsp(), nodep, "enum without items");
        std::map<uint64_t, Ast_ગાંઠ_પદાવલિ*> કિંમતો;
        {
            uint64_t ordinal = 0;
            AstEnumItem* અચલ firstp = nodep->itemsp();
            અચલ AstEnumItem* prevp = firstp;  // Prev must start with last item
            જ્યારે (prevp->nextp()) prevp = VN_AS(prevp->nextp(), EnumItem);
            માટે (AstEnumItem* itemp = firstp; itemp;) {
                AstEnumItem* અચલ nextp = VN_AS(itemp->nextp(), EnumItem);
                અચલ AstConst* અચલ vconstp = VN_AS(itemp->valuep(), Const);
                UASSERT_OBJ(vconstp, nodep, "Enum item without constified value");
                જો (!vconstp->અંક().isAnyXZ()) {  // Can 2-state runtime decode
                    અચલ uint64_t i = nodep->isWide() ? ++ordinal : vconstp->toUQuad();
                    જો (attrType == VAttrType::ENUM_NAME) {
                        કિંમતો[i] = નવું AstConst{flp, AstConst::String{},
                                                 V3_સંખ્યા::displayedEnumName(itemp)};
                    } નહીંતર જો (attrType == VAttrType::ENUM_NEXT) {
                        કિંમતો[i]
                            = (nextp ? nextp : firstp)->valuep()->clone_વૃક્ષ(ખોટું);  // A const
                    } નહીંતર જો (attrType == VAttrType::ENUM_PREV) {
                        કિંમતો[i] = prevp->valuep()->clone_વૃક્ષ(ખોટું);  // A const
                    } નહીંતર જો (attrType == VAttrType::ENUM_VALID) {
                        કિંમતો[i] = નવું AstConst{flp, AstConst::BitTrue{}};
                    } નહીંતર {
                        nodep->v3fatalSrc("Bad case");
                    }
                }
                prevp = itemp;
                itemp = nextp;
            }
        }
        // Add all specified values to table
        Ast_ગાંઠ_પદાવલિ* valuep = nullptr;
        જો (assoc) {
            // Keyed by the enum's base type
            AstConsAssoc* અચલ consp = નવું AstConsAssoc{flp, defaultp};
            consp->dtypep(vardtypep);
            માટે (અચલ auto& itr : કિંમતો) {
                V3_સંખ્યા ચાવી{nodep, nodep->width()};
                ચાવી.setQuad(itr.first);
                consp->addItemsp(નવું AstConsAssocItem{flp, નવું AstConst{flp, ચાવી}, itr.second});
            }
            valuep = consp;
        } નહીંતર {
            AstConsArray* અચલ consp = નવું AstConsArray{flp, vardtypep, defaultp};
            માટે (uint64_t i = 0; i < (msbdim + 1); ++i) {
                જો (કિંમતો[i]) consp->valuep(i, કિંમતો[i]);
            }
            valuep = consp;
        }

        // Share identical tables via the constant pool
        cachedpr = V3ConstPool::find(valuep);
        VL_DO_DANGLING(ધકેલો_Deletep(valuep), valuep);  // V3ConstPool::find clones it
        cachedpr->varp()->didWidth(સાચું);
        // Keep the dtype of the value, as the entry might have a different, compatible one
        cachedpr->didWidth(સાચું);
        ધકેલો_Deletep(cachedpr);  // Deleted with the visitor - always cloned
        પાછા cachedpr->clone_વૃક્ષ(ખોટું);
    }

    Ast_ગાંઠ_પદાવલિ* enumSelect(Ast_ગાંઠ_પદાવલિ* nodep, AstEnumDType* adtypep, VAttrType attrType) {
        // Return expression to get given attrType information from a enum's value (nodep)
        જો (attrType == VAttrType::ENUM_NAME) {
            જો (nodep->isPure()) nodep = V3Const::constifyEdit(nodep);
            જો (અચલ AstConst* અચલ constp = VN_CAST(nodep, Const)) {
                Ast_ગાંઠ_પદાવલિ* અચલ newp = નવું AstConst{nodep->fileline(), AstConst::String{},
                                                       constp->અંક().displayedEnumName(adtypep)};
                VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
                પાછા newp;
            }
        }
        જો (attrType == VAttrType::ENUM_NAME && !nodep->isPure() && (m_modep || m_ftaskp)) {
            Ast_પદાવલિ_Stmt* અચલ capturep = newEnumCapture(nodep, adtypep);
            Ast_ગાંઠ_પદાવલિ* અચલ valuep = capturep->resultp()->unlinkFrBack();
            capturep->resultp(enumSelect(valuep, adtypep, attrType));
            capturep->dtypeSetString();
            પાછા capturep;
        }
        જો (attrType == VAttrType::ENUM_NAME && adtypep->isWide()) {
            // Wide values select an ordinal in the shared name table.
            Ast_ગાંઠ_પદાવલિ* indexp = નવું AstConst{nodep->fileline(), 0};
            uint32_t ordinal = 0;
            માટે (અચલ AstEnumItem* itemp = adtypep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), EnumItem)) {
                અચલ AstConst* અચલ constp = VN_AS(itemp->valuep(), Const);
                જો (constp->અંક().isAnyXZ()) ચાલુ;
                indexp = નવું AstCond{nodep->fileline(),
                                     નવું AstEq{nodep->fileline(), nodep->clone_વૃક્ષ(ખોટું),
                                               itemp->valuep()->clone_વૃક્ષ(ખોટું)},
                                     નવું AstConst{nodep->fileline(), ++ordinal}, indexp};
            }
            AstVarRef* અચલ tabRefp = enumVarRefp(adtypep, attrType, ખોટું, ordinal);
            Ast_ગાંઠ_પદાવલિ* અચલ newp = નવું AstArraySel{nodep->fileline(), tabRefp, indexp};
            newp->dtypeSetString();
            VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
            પાછા newp;
        }
        અચલ uint64_t msbdim = enum_Max_કિંમત(nodep, adtypep);
        અચલ તાર્કિક assoc = msbdim > ENUM_LOOKUP_BITS;
        Ast_ગાંઠ_પદાવલિ* newp;
        જો (assoc) {
            AstVarRef* અચલ tabRefp = enumVarRefp(adtypep, attrType, સાચું, 0);
            newp = નવું AstAssocSel{nodep->fileline(), tabRefp, nodep};
        } નહીંતર {
            અચલ પૂર્ણાંક selwidth = V3_સંખ્યા::log2b(msbdim) + 1;  // Width to address a bit
            AstVarRef* અચલ tabRefp
                = enumVarRefp(adtypep, attrType, ખોટું, (1ULL << selwidth) - 1);
            Ast_ગાંઠ_પદાવલિ* અચલ boundp
                = attrType == VAttrType::ENUM_NAME
                          && (adtypep->width() > selwidth || msbdim != (1ULL << selwidth) - 1)
                      ? નવું AstGt{nodep->fileline(), nodep->clone_વૃક્ષ(ખોટું),
                                  નવું AstConst{nodep->fileline(),
                                               V3_સંખ્યા{nodep, adtypep->width(),
                                                        static_cast<uint32_t>(msbdim)}}}
                      : nullptr;
            newp = નવું AstArraySel{
                nodep->fileline(), tabRefp,
                // Select in case widths are off due to msblen!=width
                // We return "random" values if outside the range, which is fine
                // as next/previous on illegal values just need something good out
                નવું AstSel{nodep->fileline(), nodep, 0, selwidth}};
            જો (boundp) {
                newp = નવું AstCond{nodep->fileline(), boundp,
                                   નવું AstConst{nodep->fileline(), AstConst::String{}, ""}, newp};
            }
        }
        જો (attrType == VAttrType::ENUM_NAME) {
            newp->dtypeSetString();
        } નહીંતર જો (attrType == VAttrType::ENUM_VALID) {
            newp->dtypeSetBit();
        } નહીંતર {
            newp->dtypeFrom(adtypep);  // To prevent a later ENUMVALUE
        }
        પાછા newp;
    }
    Ast_ગાંઠ_પદાવલિ* enumTestValid(Ast_ગાંઠ_પદાવલિ* valp, AstEnumDType* enumDtp) {
        અચલ uint64_t maxval = enum_Max_કિંમત(valp, enumDtp);
        અચલ તાર્કિક assoc = maxval > ENUM_LOOKUP_BITS;
        Ast_ગાંઠ_પદાવલિ* testp = nullptr;
        File_લીટી* અચલ fl_novalue = નવું File_લીટી{valp->fileline()};
        fl_novalue->warnOff(V3ErrorCode::ENUMVALUE, સાચું);
        fl_novalue->warnOff(V3ErrorCode::CMPCONST, સાચું);
        જો (assoc) {
            AstVarRef* અચલ tabRefp = enumVarRefp(enumDtp, VAttrType::ENUM_VALID, સાચું, 0);
            testp = નવું AstAssocSel{fl_novalue, tabRefp, valp->clone_વૃક્ષ_Pure(ખોટું)};
        } નહીંતર {
            અચલ પૂર્ણાંક selwidth = V3_સંખ્યા::log2b(maxval) + 1;  // Width to address a bit
            AstVarRef* અચલ tabRefp
                = enumVarRefp(enumDtp, VAttrType::ENUM_VALID, ખોટું, (1ULL << selwidth) - 1);
            File_લીટી* અચલ fl_nowidth = નવું File_લીટી{fl_novalue};
            fl_nowidth->warnOff(V3ErrorCode::WIDTH, સાચું);
            testp = નવું AstCond{
                fl_novalue,
                નવું AstGt{fl_nowidth, valp->clone_વૃક્ષ_Pure(ખોટું),
                          નવું AstConst{fl_nowidth, AstConst::Unsized64{}, maxval}},
                નવું AstConst{fl_novalue, AstConst::BitFalse{}},
                નવું AstArraySel{fl_novalue, tabRefp,
                                નવું AstSel{fl_novalue, valp->clone_વૃક્ષ_Pure(ખોટું), 0, selwidth}}};
        }
        પાછા testp;
    }

    PatVecMap patVectorMap(AstPattern* nodep, અચલ VNumRange& range) {
        PatVecMap patmap;
        પૂર્ણાંક element = range.ડાબું();
        માટે (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;
             patp = VN_AS(patp->nextp(), PatMember)) {
            જો (patp->keyp()) {
                જો (અચલ AstConst* અચલ constp = patternConstKeyp(patp)) {
                    જો (constp->અંક().isFourState()) {
                        patp->keyp()->v3error("Assignment pattern key cannot contain X or Z");
                    } નહીંતર {
                        element = constp->toSInt();
                    }
                }
            }
            અચલ તાર્કિક newEntry = patmap.emplace(element, patp).second;
            જો (!newEntry) {
                patp->v3error("Assignment pattern key used multiple times: " << element);
            }
            element += range.ડાબું_To_જમણું_Inc();
        }
        પાછા patmap;
    }

    // Add to patternp the items of nodep, a concatenation or a single item
    શૂન્ય patConcatConvertRecurse(AstPattern* patternp, Ast_ગાંઠ_પદાવલિ* nodep) {
        જો (AstConcat* અચલ concatp = VN_CAST(nodep, Concat)) {
            patConcatConvertRecurse(patternp, concatp->lhsp());
            patConcatConvertRecurse(patternp, concatp->rhsp());
        } નહીંતર {
            AstPatMember* અચલ newp
                = નવું AstPatMember{nodep->fileline(), nodep->unlinkFrBack(), nullptr, nullptr};
            newp->isConcat(સાચું);
            patternp->addItemsp(newp);
        }
    }

    શૂન્ય makeOpenArrayShell(Ast_ગાંઠ_FTask_Ref* nodep) {
        UINFO(4, "Replicate openarray function " << nodep->taskp());
        Ast_ગાંઠ_FTask* અચલ oldTaskp = nodep->taskp();
        oldTaskp->dpiOpenParentInc();
        UASSERT_OBJ(!oldTaskp->dpiOpenChild(), oldTaskp,
                    "DPI task should be parent or child, not both");
        Ast_ગાંઠ_FTask* અચલ newTaskp = oldTaskp->clone_વૃક્ષ(ખોટું);
        newTaskp->dpiOpenChild(સાચું);
        newTaskp->dpiOpenParentClear();
        newTaskp->name(newTaskp->name() + "__Vdpioc" + cvtToStr(oldTaskp->dpiOpenParent()));
        oldTaskp->addNextHere(newTaskp);
        // Relink reference to new function
        nodep->taskp(newTaskp);
        nodep->name(nodep->taskp()->name());
        // Replace open array arguments with the callee's task
        અચલ V3TaskConnects tconnects = V3Task::taskConnects(nodep, nodep->taskp()->stmtsp());
        માટે (અચલ auto& tconnect : tconnects) {
            AstVar* અચલ portp = tconnect.first;
            અચલ AstArg* અચલ argp = tconnect.second;
            અચલ Ast_ગાંઠ* અચલ pinp = argp->exprp();
            જો (!pinp) ચાલુ;  // Argument error we'll find later
            જો (hasOpenArrayDTypeRecurse(portp->dtypep())) {
                Ast_ગાંઠ_DType* અચલ oldDTypep = portp->dtypep();
                portp->dtypep(pinp->dtypep());
                newTaskp->foreach([oldDTypep, portp](Ast_ગાંઠ* nodep) {
                    જો (nodep->dtypep() == oldDTypep) nodep->dtypep(portp->dtypep());
                });
            }
        }
    }

    તાર્કિક markHasOpenArray(Ast_ગાંઠ_FTask* nodep) {
        તાર્કિક hasOpen = ખોટું;
        માટે (Ast_ગાંઠ* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            જો (AstVar* અચલ portp = VN_CAST(stmtp, Var)) {
                જો (portp->isDpiOpenArray() || hasOpenArrayDTypeRecurse(portp->dtypep())) {
                    portp->isDpiOpenArray(સાચું);
                    hasOpen = સાચું;
                }
            }
        }
        પાછા hasOpen;
    }
    તાર્કિક hasOpenArrayDTypeRecurse(Ast_ગાંઠ_DType* nodep) {
        // Return true iff this datatype or child has an openarray
        જો (VN_IS(nodep, UnsizedArrayDType)) પાછા સાચું;
        જો (nodep->subDTypep()) પાછા hasOpenArrayDTypeRecurse(nodep->subDTypep()->skipRefp());
        પાછા ખોટું;
    }
    Ast_ગાંઠ_DType* dtypeNotIntAtomOrVecRecurse(Ast_ગાંઠ_DType* nodep, તાર્કિક ranged = ખોટું) {
        // If node is _not_ integer or atomic, return node that makes it fail
        nodep = nodep->skipRefToEnump();
        જો (AstBasicDType* અચલ dtp = VN_CAST(nodep, BasicDType)) {
            જો (ranged && (!dtp->isBitLogic() || dtp->isRanged()))
                પાછા dtp;  // Packed when already packed
            જો (dtp->keyword().isIntNumeric()) પાછા nullptr;
            પાછા dtp;
        } નહીંતર જો (AstPackArrayDType* અચલ dtp = VN_CAST(nodep, PackArrayDType)) {
            જો (ranged) પાછા dtp;  // Packed when already packed
            પાછા dtypeNotIntAtomOrVecRecurse(nodep->subDTypep(), સાચું);
        }
        પાછા nodep;
    }
    Ast_ગાંઠ_DType* dtypeNot4StateIntegralRecurse(Ast_ગાંઠ_DType* nodep) {
        // If node is _not_ inet valid data type, 4-state integral packed or union, return node
        // that makes it fail
        nodep = nodep->skipRefp();
        જો (AstBasicDType* અચલ dtp = VN_CAST(nodep, BasicDType)) {
            જો (!dtp->keyword().isFourstate()) પાછા dtp;
            પાછા nullptr;
        } નહીંતર જો (Ast_ગાંઠ_Array_DType* અચલ dtp = VN_CAST(nodep, ગાંઠ_Array_DType)) {
            પાછા dtypeNot4StateIntegralRecurse(dtp->subDTypep());
        } નહીંતર જો (Ast_ગાંઠ_UOr_Struct_DType* અચલ dtp = VN_CAST(nodep, ગાંઠ_UOr_Struct_DType)) {
            માટે (AstMemberDType* itemp = dtp->membersp(); itemp;
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {
                Ast_ગાંઠ_DType* અચલ badDtp = dtypeNot4StateIntegralRecurse(itemp->dtypep());
                જો (badDtp) પાછા badDtp;
            }
            પાછા nullptr;
        }
        પાછા nodep;
    }
    શૂન્ય checkForceReleaseLhs(Ast_ગાંઠ* nodep, Ast_ગાંઠ* lhsp) {
        // V3Force can't check as vector may have expanded, or propagated constant into index
        જો (Ast_ગાંઠ* અચલ selNodep = V3Width::selectNonConstantRecurse(lhsp))
            nodep->v3error((VN_IS(nodep, Release) ? "Release"s : "Force"s)
                               + " left-hand-side must not have variable bit/part select "
                                 "(IEEE 1800-2023 10.6.2)\n"
                           << nodep->warnContextPrimary() << '\n'
                           << selNodep->warnOther() << "... Location of non-constant index\n"
                           << selNodep->warnContextSecondary());
    }

    //----------------------------------------------------------------------
    // METHODS - special type detection

    શૂન્ય assert_At_પદાવલિ(Ast_ગાંઠ* nodep) {
        જો (VL_UNCOVERABLE(!m_vup)) {
            nodep->v3fatalSrc("Unexpected '" << nodep->prettyTypeName() << "' expression under '"
                                             << nodep->backp()->prettyTypeName() << "'");
        }
    }
    શૂન્ય assertAtStatement(Ast_ગાંઠ* nodep) {
        જો (VL_UNCOVERABLE(m_vup && !m_vup->selfDtm())) {
            UINFO(1, "-: " << m_vup);
            nodep->v3fatalSrc("No dtype expected at statement " << nodep->prettyTypeName());
        }
    }
    શૂન્ય checkConstantOrReplace(Ast_ગાંઠ* nodep, તાર્કિક noFourState, અચલ string& message) {
        // See also V3WidthSel::checkConstantOrReplace
        // Note can't call V3Const::constifyParam(nodep) here, as constify may change nodep on us!
        AstConst* અચલ constp = VN_CAST(nodep, Const);
        જો (!constp || (noFourState && constp->અંક().isFourState())) {
            nodep->v3error(message);
            nodep->replaceWith(નવું AstConst{nodep->fileline(), AstConst::Unsized32{}, 1});
            VL_DO_DANGLING2(ધકેલો_Deletep(nodep), nodep, constp);
            પાછા;
        }
    }
    Ast_ગાંઠ* ગાંઠ_For_Unsized_Warning(Ast_ગાંઠ* nodep) {
        // Return a nodep to use for unsized warnings, reporting on child if can
        જો (nodep->op1p() && nodep->op1p()->dtypep() && !nodep->op1p()->dtypep()->widthSized()) {
            પાછા nodep->op1p();
        } નહીંતર જો (nodep->op2p() && nodep->op2p()->dtypep()
                   && !nodep->op2p()->dtypep()->widthSized()) {
            પાછા nodep->op2p();
        }
        પાછા nodep;  // By default return this
    }
    AstRefDType* checkRefToTypedefRecurse(Ast_ગાંઠ* nodep, AstTypedef* typedefp) {
        // Recurse all children looking for self reference
        // This avoids iterateEditMoveDTypep going into a hard to resolve loop
        // Only call once for any given typedef, or will become O(n^2)
        જો (VL_LIKELY(!nodep)) પાછા nullptr;
        જો (auto* અચલ refp = VN_CAST(nodep, RefDType)) {
            જો (refp->typedefp() == typedefp) પાછા refp;
        }
        જો (auto* અચલ refp = checkRefToTypedefRecurse(nodep->op1p(), typedefp)) પાછા refp;
        જો (auto* અચલ refp = checkRefToTypedefRecurse(nodep->op2p(), typedefp)) પાછા refp;
        જો (auto* અચલ refp = checkRefToTypedefRecurse(nodep->op3p(), typedefp)) પાછા refp;
        જો (auto* અચલ refp = checkRefToTypedefRecurse(nodep->op4p(), typedefp)) પાછા refp;
        પાછા nullptr;
    }

    //----------------------------------------------------------------------
    // METHODS - interface arrays

    // True if an unpacked array of non-virtual interfaces
    સ્થિર તાર્કિક isIfaceArrayDType(અચલ Ast_ગાંઠ_DType* dtypep) {
        જો (!VN_IS(dtypep->skipRefp(), UnpackArrayDType)) પાછા ખોટું;
        અચલ AstIfaceRefDType* અચલ irefp = VN_CAST(dtypep->elemDTypep(), IfaceRefDType);
        પાછા irefp && !irefp->isVirtual();
    }

    // Number of elements of each unpacked dimension of 'dtypep', from the left, e.g. "2x3"
    સ્થિર std::string unpackedArraySize(અચલ Ast_ગાંઠ_DType* dtypep) {
        std::string size;
        dtypep = dtypep->skipRefp();
        જ્યારે (અચલ AstUnpackArrayDType* અચલ arrp = VN_CAST(dtypep, UnpackArrayDType)) {
            જો (!size.empty()) size += "x";
            size += std::to_string(arrp->elementsConst());
            dtypep = arrp->subDTypep()->skipRefp();
        }
        પાછા size;
    }

    // A whole array of interfaces is an AstConsArray of references to its elements, so select
    // its element 'bit' (counted from lo) directly. Returns nullptr if not such an array, or
    // 'bit' is out of range.
    સ્થિર Ast_ગાંઠ_પદાવલિ* newIfaceArrayElement(Ast_ગાંઠ_પદાવલિ* fromp, પૂર્ણાંક bit) {
        AstConsArray* અચલ consp = VN_CAST(fromp, ConsArray);
        જો (!consp || !isIfaceArrayDType(consp->dtypep())) પાછા nullptr;
        અચલ AstUnpackArrayDType* અચલ arrp
            = VN_AS(consp->dtypep()->skipRefp(), UnpackArrayDType);
        જો (bit < 0 || bit >= arrp->elementsConst()) પાછા nullptr;
        પાછા consp->valuep(bit)->clone_વૃક્ષ(ખોટું);
    }

    // Widthed references to the element variables of the whole interface array referenced by
    // 'refp', in the dimensions of 'dtypep' inwards. 'name' is the element name so far, the
    // element variables being named 'name' + 'suffix'. 'elemVarpr' is the element variable of
    // the leftmost element in the dimensions of 'dtypep', and is moved to the right past them.
    // E.g. 'ifc' of dimensions [0:1][3:2] -> '{'{ifc[0][3], ifc[0][2]}, '{ifc[1][3], ifc[1][2]}}
    Ast_ગાંઠ_પદાવલિ* new_Iface_Array_કિંમત(અચલ Ast_ગાંઠ_Var_Ref* refp, Ast_ગાંઠ_DType* dtypep,
                                    અચલ std::string& name, અચલ std::string& suffix,
                                    AstVar*& elemVarpr) {
        File_લીટી* અચલ flp = refp->fileline();
        AstUnpackArrayDType* અચલ arrp = VN_CAST(dtypep->skipRefp(), UnpackArrayDType);
        // Base case: reference the element variable when no dimensions left
        જો (!arrp) {
            AstVar* અચલ varp = elemVarpr;
            UASSERT_OBJ(varp && varp->name() == name + suffix, refp,
                        "Missing element variable of interface array " << name);
            elemVarpr = VN_CAST(varp->nextp(), Var);
            Ast_ગાંઠ_Var_Ref* newp;
            જો (અચલ AstVarXRef* અચલ xrefp = VN_CAST(refp, VarXRef)) {
                newp = નવું AstVarXRef{flp, varp, xrefp->dotted(), refp->access()};
            } નહીંતર {
                newp = નવું AstVarRef{flp, varp, refp->access()};
            }
            userIterate(newp, WidthVP{SELF, BOTH}.p());
            પાછા newp;
        }

        // Enumerate the current dimension given by 'arrp', from the left index to the right,
        // as the element variables are in that order, indexed from lo
        AstConsArray* અચલ consp = નવું AstConsArray{flp, arrp};
        અચલ VNumRange range = arrp->declRange();
        માટે (અચલ પૂર્ણાંક i : range.seq_ડાબું_To_જમણું()) {
            અચલ std::string s = name + "__BRA__" + Ast_ગાંઠ::encode_સંખ્યા(i) + "__KET__";
            consp->valuep(i - range.lo(),
                          new_Iface_Array_કિંમત(refp, arrp->subDTypep(), s, suffix, elemVarpr));
        }
        પાછા consp;
    }

    // Replace a reference to a whole interface array variable (a __Viftop companion, or an
    // interface array port) split by V3Param, with the AstConsArray of references to its
    // elements, which is how whole arrays of interfaces are handled from here on
    શૂન્ય replaceSplitIfaceArrayRef(Ast_ગાંઠ_Var_Ref* nodep) {
        AstVar* અચલ varp = nodep->varp();
        UASSERT_OBJ(varp->isIfaceArraySplit(), nodep, "Not a split interface array");
        std::string name = varp->name();
        std::string suffix;
        જો (varp->isIfaceParent()) {
            name = name.substr(0, name.rfind("__Viftop"));
            suffix = "__Viftop";
        }
        // The element variables follow the whole array variable in order (see V3Param)
        AstVar* elemVarp = VN_CAST(varp->nextp(), Var);
        nodep->replaceWith(new_Iface_Array_કિંમત(nodep, varp->dtypep(), name, suffix, elemVarp));
        VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
    }

    // Add a pin for each element port of interface array port of pin 'nodep', split by V3Param,
    // in the dimensions of 'dtypep' inwards, connected to the element at the same position,
    // counted from the left, of 'conp', which is the corresponding (sub)array of the pin's
    // connection, if any. 'suffix' is the name suffix of the dimensions outside 'dtypep'.
    // 'elemVarpr' is the element port variable of the rightmost element in the dimensions of
    // 'dtypep', and is moved to the left past them.
    શૂન્ય expandIfaceArrayPinDimensions(AstPin* nodep, અચલ Ast_ગાંઠ_DType* dtypep,
                                       Ast_ગાંઠ_પદાવલિ* conp, અચલ std::string& suffix,
                                       AstVar*& elemVarpr) {
        અચલ AstUnpackArrayDType* અચલ arrp = VN_CAST(dtypep->skipRefp(), UnpackArrayDType);
        // Base case: add the pin of the element port after 'nodep' when no dimensions left
        જો (!arrp) {
            UASSERT_OBJ(elemVarpr && elemVarpr->name() == nodep->modVarp()->name() + suffix, nodep,
                        "Missing element variable of interface array port " << suffix);
            AstPin* અચલ newp = nodep->clone_વૃક્ષ(ખોટું);
            newp->name(nodep->name() + suffix);
            newp->modVarp(elemVarpr);
            elemVarpr = VN_CAST(elemVarpr->backp(), Var);
            જો (conp) newp->exprp(conp->clone_વૃક્ષ(ખોટું));
            nodep->addNextHere(newp);
            પાછા;
        }

        // Enumerate the current dimension given by 'arrp'
        // Each element is added right after 'nodep', so go from right to left,
        // to end with an enumeration from the left index to the right index.
        અચલ VNumRange range = arrp->declRange();
        માટે (પૂર્ણાંક n = range.elements() - 1; n >= 0; --n) {
            અચલ પૂર્ણાંક i = range.ડાબું() + n * range.ડાબું_To_જમણું_Inc();
            અચલ std::string s = suffix + "__BRA__" + Ast_ગાંઠ::encode_સંખ્યા(i) + "__KET__";
            // Element of the connection, if any, which is only cloned at the leaves
            Ast_ગાંઠ_પદાવલિ* અચલ elemConnp = [&]() -> Ast_ગાંઠ_પદાવલિ* {
                જો (!conp) પાછા nullptr;
                અચલ AstUnpackArrayDType* અચલ conArrp
                    = VN_AS(conp->dtypep()->skipRefp(), UnpackArrayDType);
                અચલ પૂર્ણાંક elements = conArrp->elementsConst();
                // Counted from lo
                Ast_ગાંઠ_પદાવલિ* અચલ resultp
                    = VN_AS(conp, ConsArray)
                          ->valuep(conArrp->declRange().ascending() ? n : elements - 1 - n);
                UASSERT_OBJ(resultp, conp, "Interface array connection of mismatched shape");
                પાછા resultp;
            }();
            expandIfaceArrayPinDimensions(nodep, arrp->subDTypep(), elemConnp, s, elemVarpr);
        }
    }

    // Replace the given interface array port pin with a pin for each element port split by V3Param
    શૂન્ય expandIfaceArrayPin(AstPin* nodep) {
        AstVar* અચલ portp = nodep->modVarp();
        UASSERT_OBJ(portp->isIfaceArraySplit(), nodep, "Not a split interface array port");
        // The element variables follow the port in order (see V3Param). Pins are added from the
        // right, so start at the last.
        AstVar* elemVarp = portp;
        માટે (uint32_t n = portp->dtypep()->arrayUnpackedElements(); n; --n) {
            elemVarp = VN_CAST(elemVarp->nextp(), Var);
            UASSERT_OBJ(elemVarp, portp, "Missing element variables of interface array port");
        }
        // Remove the connection, so the element pins do not clone it all. Deletion is deferred,
        // so its elements can still be cloned from.
        Ast_ગાંઠ_પદાવલિ* અચલ exprp = VN_AS(nodep->exprp(), ગાંઠ_પદાવલિ);
        જો (exprp) ધકેલો_Deletep(exprp->unlinkFrBack());
        // Connect nothing if not a whole array of interfaces of the shape of the port, which is
        // reported elsewhere, so the element pins do not report it again
        અચલ તાર્કિક connect
            = exprp && isIfaceArrayDType(exprp->dtypep())
              && unpackedArraySize(exprp->dtypep()) == unpackedArraySize(portp->dtypep());
        expandIfaceArrayPinDimensions(nodep, portp->dtypep(), connect ? exprp : nullptr, "",
                                      elemVarp);
        UASSERT_OBJ(elemVarp == portp, portp, "Mismatched element port variable");
        VL_DO_DANGLING(ધકેલો_Deletep(nodep->unlinkFrBack()), nodep);
    }

    //----------------------------------------------------------------------
    // METHODS - special iterators
    // These functions save/restore the AstNUser information so it can pass to child nodes.

    Ast_ગાંઠ* userIterateSubtreeReturnEdits(Ast_ગાંઠ* nodep, WidthVP* vup) {
        જો (!nodep) પાછા nullptr;
        Ast_ગાંઠ* ret;
        {
            VL_RESTORER(m_vup);
            m_vup = vup;
            ret = iterateSubtreeReturnEdits(nodep);
        }
        પાછા ret;
    }
    શૂન્ય userIterate(Ast_ગાંઠ* nodep, WidthVP* vup) {
        જો (!nodep) પાછા;
        VL_RESTORER(m_vup);
        m_vup = vup;
        iterate(nodep);
    }
    શૂન્ય userIterateAndNext(Ast_ગાંઠ* nodep, WidthVP* vup) {
        જો (!nodep) પાછા;
        જો (nodep->didWidth()) પાછા;  // Avoid iterating list we have already iterated
        VL_RESTORER(m_vup);
        m_vup = vup;
        iterateAndNextNull(nodep);
    }
    શૂન્ય userIterateChildren(Ast_ગાંઠ* nodep, WidthVP* vup) {
        જો (!nodep) પાછા;
        VL_RESTORER(m_vup);
        m_vup = vup;
        iterateChildren(nodep);
    }
    શૂન્ય userIterateChildrenBackwardsConst(Ast_ગાંઠ* nodep, WidthVP* vup) {
        જો (!nodep) પાછા;
        VL_RESTORER(m_vup);
        m_vup = vup;
        iterateChildrenBackwardsConst(nodep);
    }

જાહેર:
    // CONSTRUCTORS
    WidthVisitor(તાર્કિક paramsOnly,  // [in] TRUE if we are considering parameters only.
                 તાર્કિક doGenerate)  // [in] TRUE if we are inside a generate statement and
        //                           // don't wish to trigger errors
        : m_enumTempNames{"__Venum"}
        , m_insideTempNames{"__VInside"}
        , m_paramsOnly{paramsOnly}
        , m_doGenerate{doGenerate} {}
    Ast_ગાંઠ* mainAcceptEdit(Ast_ગાંઠ* nodep) {
        nodep = userIterateSubtreeReturnEdits(nodep, WidthVP{SELF, BOTH}.p());
        // All references to the whole interface array variables split by V3Param are now replaced
        માટે (AstVar* અચલ varp : m_splitIfaceArrayVarps) {
            VL_DO_DANGLING(ધકેલો_Deletep(varp->unlinkFrBack()), varp);
        }
        m_splitIfaceArrayVarps.clear();
        પાછા nodep;
    }
    સ્થિર તાર્કિક lowerAsFixedAggregate(અચલ Ast_ગાંઠ_DType* અચલ dtypep) {
        પાછા dtypep->isStreamableFixedAggregate() && dtypep->containsUnpackedStruct();
    }
    શૂન્ય streamAssignLower(Ast_ગાંઠ_Assign* nodep) {
        Ast_ગાંઠ_DType* lhsDTypep = nodep->lhsp()->dtypep();
        // Check width of stream and wrap if needed
        જો (Ast_ગાંઠ_Stream* અચલ streamp = VN_CAST(nodep->rhsp(), ગાંઠ_Stream)) {
            Ast_ગાંઠ_DType* અચલ lhsDTypeSkippedRefp = lhsDTypep->skipRefp();
            અચલ પૂર્ણાંક lwidth = lhsDTypeSkippedRefp->widthStream();
            અચલ પૂર્ણાંક rwidth = streamp->lhsp()->dtypep()->skipRefp()->widthStream();
            જો (lwidth != 0 && lwidth < rwidth) {
                nodep->v3widthWarn(lwidth, rwidth,
                                   "Target fixed size variable ("
                                       << lwidth << " bits) is narrower than the stream ("
                                       << rwidth << " bits) (IEEE 1800-2023 11.4.14)");
            }
            જો (VN_IS(streamp->lhsp()->dtypep()->skipRefp(), QueueDType)
                && !VN_IS(nodep->lhsp()->dtypep()->skipRefp(), QueueDType)) {
                અચલ પૂર્ણાંક queueElementSize = streamp->lhsp()->dtypep()->subDTypep()->width();
                UASSERT_OBJ(queueElementSize <= lwidth, nodep, "LHS < RHS");
            }
            જો (VN_IS(lhsDTypeSkippedRefp, UnpackArrayDType)
                || lowerAsFixedAggregate(lhsDTypeSkippedRefp)) {
                streamp->unlinkFrBack();
                nodep->rhsp(
                    નવું AstCvtPackedToArray{streamp->fileline(), streamp, lhsDTypeSkippedRefp});
            }
        }
        જો (Ast_ગાંઠ_Stream* અચલ streamp = VN_CAST(nodep->lhsp(), ગાંઠ_Stream)) {
            અચલ Ast_ગાંઠ_DType* અચલ rhsDTypep = nodep->rhsp()->dtypep()->skipRefp();
            Ast_ગાંઠ_DType* અચલ lhsStreamDTypep = streamp->lhsp()->dtypep()->skipRefp();
            અચલ પૂર્ણાંક lwidth = lhsStreamDTypep->widthStream();
            અચલ પૂર્ણાંક rwidth = rhsDTypep->widthStream();
            જો (rwidth != 0 && rwidth < lwidth) {
                nodep->v3widthWarn(lwidth, rwidth,
                                   "Stream target requires "
                                       << lwidth << " bits, but source expression only provides "
                                       << rwidth << " bits (IEEE 1800-2023 11.4.14.3)");
            }
            જો (lowerAsFixedAggregate(lhsStreamDTypep)) {
                Ast_ગાંઠ_પદાવલિ* અચલ streamExprp = nodep->lhsp()->unlinkFrBack();
                Ast_ગાંઠ_પદાવલિ* અચલ dstp = streamp->lhsp()->unlinkFrBack();
                Ast_ગાંઠ_પદાવલિ* srcp = nodep->rhsp()->unlinkFrBack();
                જો (VN_IS(streamp, StreamL)) {
                    streamp->lhsp(srcp);
                    streamp->dtypeSetBitOrLogicUnsized(
                        srcp->width(), srcp->widthMin(), VSigning::UNSIGNED,
                        Ast_ગાંઠ_પદાવલિ::isFourstateOrNoDtype(streamExprp, srcp));
                    srcp = streamExprp;
                } નહીંતર {
                    જો (srcp->width() > lwidth) {
                        srcp = નવું AstSel{streamp->fileline(), srcp, srcp->width() - lwidth,
                                          lwidth};
                    }
                    VL_DO_DANGLING(ધકેલો_Deletep(streamExprp), streamExprp);
                }
                nodep->lhsp(dstp);
                nodep->rhsp(નવું AstCvtPackedToArray{srcp->fileline(), srcp, lhsStreamDTypep});
                nodep->dtypeFrom(dstp);
            } નહીંતર જો (VN_IS(rhsDTypep, UnpackArrayDType)) {
                Ast_ગાંઠ_પદાવલિ* અચલ rhsp = nodep->rhsp()->unlinkFrBack();
                nodep->rhsp(નવું AstCvtArrayToPacked{rhsp->fileline(), rhsp, streamp->dtypep()});
            }
        }
    }
    ~WidthVisitor() override = મૂળભૂત;
};

//######################################################################
// Width class functions

શૂન્ય V3Width::width(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        // We should do it in bottom-up module order, but it works in any order.
        અચલ WidthClearVisitor cvisitor{nodep};
        // Typedefs and classes are named by the scopes declaring them, so index those
        અચલ VDTypeNameScopes scopes{nodep};
        WidthVisitor visitor{ખોટું, ખોટું};
        (શૂન્ય)visitor.mainAcceptEdit(nodep);
        WidthRemoveVisitor rvisitor;
        (શૂન્ય)rvisitor.mainAcceptEdit(nodep);
    }  // Destruct before checking
    V3Global::dump_Check_Global_વૃક્ષ("width", 0, dump_વૃક્ષ_Either_Level() >= 3);
}
//! Single node parameter propagation
//! Smaller step... Only do a single node for parameter propagation
Ast_ગાંઠ* V3Width::widthParamsEdit(Ast_ગાંઠ* nodep) {
    UINFO(4, __FUNCTION__ << ": " << nodep);
    // We should do it in bottom-up module order, but it works in any order.
    WidthVisitor visitor{સાચું, ખોટું};
    nodep = visitor.mainAcceptEdit(nodep);
    // No WidthRemoveVisitor, as don't want to drop $signed etc inside gen blocks
    પાછા nodep;
}

શૂન્ય V3Width::streamAssignLowerEdit(Ast_ગાંઠ_Assign* nodep) {
    UINFO(4, __FUNCTION__ << ": " << nodep);
    WidthVisitor visitor{ખોટું, ખોટું};
    visitor.streamAssignLower(nodep);
}

//! Single node parameter propagation for generate blocks.
//! Smaller step... Only do a single node for parameter propagation
//! If we are inside a generated "if", "case" or "for", we don't want to
//! trigger warnings when we deal with the width. It is possible that
//! these are spurious, existing within sub-expressions that will not
//! actually be generated. Since such occurrences, must be constant, in
//! order to be something a generate block can depend on, we can wait until
//! later to do the width check.
//! @return  Pointer to the edited node.
Ast_ગાંઠ* V3Width::widthGenerateParamsEdit(
    Ast_ગાંઠ* nodep) {  //!< [in] AST whose parameters widths are to be analyzed.
    UINFO(4, __FUNCTION__ << ": " << nodep);
    // We should do it in bottom-up module order, but it works in any order.
    WidthVisitor visitor{સાચું, સાચું};
    nodep = visitor.mainAcceptEdit(nodep);
    // No WidthRemoveVisitor, as don't want to drop $signed etc inside gen blocks
    પાછા nodep;
}

Ast_ગાંઠ* V3Width::selectNonConstantRecurse(Ast_ગાંઠ* nodep, તાર્કિક inSel) {
    // If node has a non-constant select, return that select
    Ast_ગાંઠ* resultp = nullptr;
    જો (Ast_ગાંઠ_Sel* અચલ anodep = VN_CAST(nodep, ગાંઠ_Sel)) {
        resultp = selectNonConstantRecurse(anodep->fromp(), inSel);
        જો (resultp) પાછા resultp;
        resultp = selectNonConstantRecurse(anodep->bitp(), સાચું);
    } નહીંતર જો (AstSel* અચલ anodep = VN_CAST(nodep, Sel)) {
        resultp = selectNonConstantRecurse(anodep->fromp(), inSel);
        જો (resultp) પાછા resultp;
        resultp = selectNonConstantRecurse(anodep->lsbp(), સાચું);
    } નહીંતર જો (Ast_ગાંઠ_Var_Ref* અચલ anodep = VN_CAST(nodep, ગાંઠ_Var_Ref)) {
        જો (inSel && !anodep->varp()->isParam() && !anodep->varp()->isGenVar()) પાછા anodep;
    } નહીંતર {
        જો (Ast_ગાંઠ* અચલ refp = nodep->op1p()) resultp = selectNonConstantRecurse(refp, inSel);
        જો (resultp) પાછા resultp;
        જો (Ast_ગાંઠ* અચલ refp = nodep->op2p()) resultp = selectNonConstantRecurse(refp, inSel);
        જો (resultp) પાછા resultp;
        જો (Ast_ગાંઠ* અચલ refp = nodep->op3p()) resultp = selectNonConstantRecurse(refp, inSel);
        જો (resultp) પાછા resultp;
        જો (Ast_ગાંઠ* અચલ refp = nodep->op4p()) resultp = selectNonConstantRecurse(refp, inSel);
    }
    પાછા resultp;
}
