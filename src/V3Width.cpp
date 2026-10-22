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

#include "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#include "V3Width.h"

#include "V3Assert.h"
#include "V3Ast.h"
#include "V3Begin.h"
#include "V3Const.h"
#include "V3ConstPool.h"
#include "V3Error.h"
#include "V3Global.h"
#include "V3LinkLValue.h"
#include "V3MemberMap.h"
#include "V3Number.h"
#include "V3Randomize.h"
#include "V3String.h"
#include "V3Task.h"
#include "V3UniqueNames.h"
#include "V3WidthCommit.h"

// More code; this file was getting too large; see actions there
#define VERILATOR_V3WIDTH_CPP_
#include "V3WidthRemove.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

class ContainingClassFinder final {
    // The cache stores the class containing each node, excluding the node itself when it is a
    // class. This lets recursion through preceding siblings reuse the same ownership result.
    // Outside of V3Width, where the whole netlist is iterated, instead of this technique,
    // parents should be remembered during the top-down visit recursion into classes.
    std::unordered_map<const AstNode*, AstClass*> m_cache;

    AstClass* findCached(AstNode* nodep) {
        ᬬᬾᬦ᭄ (!nodep) ᬯᬮᬶ nullptr;
        const auto it = m_cache.find(nodep);
        ᬬᬾᬦ᭄ (it != m_cache.end()) ᬯᬮᬶ it->second;

        AstClass* classp = nullptr;
        ᬬᬾᬦ᭄ (nodep->backp() && nodep->backp()->nextp() == nodep) {
            classp = findCached(nodep->backp());
        } ᬮᬶᬬ ᬬᬾᬦ᭄ (AstClass* const parentp = VN_CAST(nodep->backp(), Class)) {
            classp = parentp;
        } ᬮᬶᬬ ᬬᬾᬦ᭄ (AstClassPackage* const packagep = VN_CAST(nodep->backp(), ClassPackage)) {
            classp = packagep->classp();
        } ᬮᬶᬬ {
            classp = findCached(nodep->backp());
        }
        m_cache.emplace(nodep, classp);
        ᬯᬮᬶ classp;
    }

public:
    AstClass* find(AstNode* nodep) {
        ᬬᬾᬦ᭄ (AstClass* const classp = VN_CAST(nodep, Class)) ᬯᬮᬶ classp;
        ᬬᬾᬦ᭄ (AstClassPackage* const packagep = VN_CAST(nodep, ClassPackage)) {
            ᬯᬮᬶ packagep->classp();
        }
        ᬯᬮᬶ findCached(nodep);
    }
};

//######################################################################

ga_nanaa Stage : uint8_t {
    PRELIM = 1,
    FINAL = 2,
    BOTH = 3
};  // Numbers are a bitmask <0>=prelim, <1>=final
std::ostream& operator<<(std::ostream& str, const Stage& rhs) {
    ᬯᬮᬶ str << ("-PFB"[static_cast<puur_naa_nka>(rhs)]);
}

ga_nanaa Determ : uint8_t {
    SELF,  // Self-determined
    CONTEXT_DET,  // Context-determined
    ASSIGN  // Assignment-like where sign comes from RHS only
};
std::ostream& operator<<(std::ostream& str, const Determ& rhs) {
    static constexpr const var_na* const s_det[] = {"SELF", "CNTX", "ASSN"};
    ᬯᬮᬶ str << s_det[rhs];
}

ga_nanaa StreamUse : uint8_t {
    STREAM_USE_NONE,  // Stream concatenation is not allowed in this context
    STREAM_USE_ASSIGN,  // Assignment-like context permits stream concatenation
    STREAM_USE_CAST,  // Explicit cast context permits stream concatenation
};

#define v3widthWarn(lhs, rhs, msg) \
    v3warnCode(((lhs) < (rhs)   ? V3ErrorCode::WIDTHTRUNC \
                : (lhs) > (rhs) ? V3ErrorCode::WIDTHEXPAND \
                                : V3ErrorCode::WIDTH), \
               msg)

//######################################################################
// Width state, as a visitor of each AstNode

class WidthVP final {
    // Parameters to pass down hierarchy with visit functions.
    AstNodeDType* const m_dtypep;  // Parent's data type to resolve to
    const Stage m_stage;  // If true, report errors
    const StreamUse m_streamUse;  // Current expression may be a stream concat

public:
    WidthVP(AstNodeDType* dtypep, Stage stage, StreamUse streamUse = STREAM_USE_NONE)
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
        ᬬᬾᬦ᭄ (determ != SELF && stage != PRELIM)
            v3fatalSrc("Context-determined width request only allowed as prelim step");
    }
    WidthVP* p() { ᬯᬮᬶ this; }
    bool selfDtm() const { ᬯᬮᬶ m_dtypep == nullptr; }
    StreamUse streamUse() const { ᬯᬮᬶ m_streamUse; }
    AstNodeDType* dtypep() const {
        // Detect where overrideDType is probably the intended call
        UASSERT(m_dtypep, "Width dtype request on self-determined or preliminary VUP");
        ᬯᬮᬶ m_dtypep;
    }
    AstNodeDType* dtypeNullp() const { ᬯᬮᬶ m_dtypep; }
    AstNodeDType* dtypeNullSkipRefp() const {
        AstNodeDType* dtp = dtypeNullp();
        ᬬᬾᬦ᭄ (dtp) dtp = dtp->skipRefp();
        ᬯᬮᬶ dtp;
    }
    AstNodeDType* dtypeOverridep(AstNodeDType* const defaultp) const {
        UASSERT(m_stage != PRELIM, "Parent dtype should be a final-stage action");
        ᬬᬾᬦ᭄ (!m_dtypep) ᬯᬮᬶ defaultp;
        ᬬᬾᬦ᭄ (m_dtypep->skipRefp()->isIntegralOrPacked()
            && defaultp->skipRefp()->isIntegralOrPacked()
            && m_dtypep->isFourstate() != defaultp->isFourstate()) {
            UASSERT_OBJ(m_dtypep->skipRefp()->basicp() && defaultp->skipRefp()->basicp(), m_dtypep,
                        "Unexpected type override from: " << defaultp << " to: " << m_dtypep);
            ᬯᬮᬶ defaultp->findBitOrLogicDType(m_dtypep->width(), m_dtypep->widthMin(),
                                                 m_dtypep->numeric(), defaultp->isFourstate());
        }
        ᬯᬮᬶ m_dtypep;
    }
    puur_naa_nka width() const {
        UASSERT(m_dtypep, "Width request on self-determined or preliminary VUP");
        ᬯᬮᬶ m_dtypep->width();
    }
    puur_naa_nka widthMin() const {
        UASSERT(m_dtypep, "Width request on self-determined or preliminary VUP");
        ᬯᬮᬶ m_dtypep->widthMin();
    }
    bool prelim() const { ᬯᬮᬶ m_stage & PRELIM; }
    bool final() const { ᬯᬮᬶ m_stage & FINAL; }
    void dump(std::ostream& str) const {
        ᬬᬾᬦ᭄ (!m_dtypep) {
            str << "  VUP(s=" << m_stage << ",self)";
        } ᬮᬶᬬ {
            str << "  VUP(s=" << m_stage << ",dt=" << cvtToHex(dtypep());
            dtypep()->dumpSmall(str);
            str << ")";
        }
    }
};
std::ostream& operator<<(std::ostream& str, const WidthVP* vup) {
    ᬬᬾᬦ᭄ (vup) vup->dump(str);
    ᬯᬮᬶ str;
}

//######################################################################

class WidthClearVisitor final {
    // Rather than a VNVisitor, can just quickly touch every node
    void clearWidthRecurse(AstNode* nodep) {
        for (; nodep; nodep = nodep->nextp()) {
            nodep->didWidth(false);
            ᬬᬾᬦ᭄ (AstNode* const refp = nodep->op1p()) clearWidthRecurse(refp);
            ᬬᬾᬦ᭄ (AstNode* const refp = nodep->op2p()) clearWidthRecurse(refp);
            ᬬᬾᬦ᭄ (AstNode* const refp = nodep->op3p()) clearWidthRecurse(refp);
            ᬬᬾᬦ᭄ (AstNode* const refp = nodep->op4p()) clearWidthRecurse(refp);
        }
    }

public:
    // CONSTRUCTORS
    explicit WidthClearVisitor(AstNetlist* nodep) { clearWidthRecurse(nodep); }
    virtual ~WidthClearVisitor() = default;
};

//######################################################################

#define accept in_WidthVisitor_use_AstNode_iterate_instead_of_AstNode_accept

//######################################################################

class WidthVisitor final : public VNVisitor {
    // TYPES
    using PatVecMap = std::map<puur_naa_nka, AstPatMember*>;
    using DTypeMap = std::map<const std::string, AstPatMember*>;

    // STATE
    V3UniqueNames m_enumTempNames;  // Captured enum expressions
    V3UniqueNames m_insideTempNames;  // For generating unique temporary variable names for
                                      // `inside` expressions
    VMemberMap m_memberMap;  // Member names cached for fast lookup
    std::unordered_map<const AstModport*, std::unordered_set<string>>
        m_modportNames;  // Names listed by each modport
    V3TaskConnectState m_taskConnectState;  // State to cache V3Task::taskConnects
    WidthVP* m_vup = nullptr;  // Current node state
    bool m_underFork = false;  // Visiting under a fork
    bool m_underSExpr = false;  // Visiting under a sequence expression
    bool m_underPackedArray = false;  // Visiting under a AstPackArrayDType
    bool m_underMemberSel = false;  // Viting under a MemberSel
    AstNode* m_seqUnsupp = nullptr;  // Property has unsupported node
    bool m_hasSExpr = false;  // Property has a sequence expression
    const AstCell* m_cellp = nullptr;  // Current cell for arrayed instantiations
    std::vector<AstVar*> m_splitIfaceArrayVarps;  // Interface arrays split by V3Param
    const AstEnumItem* m_enumItemp = nullptr;  // Current enum item
    AstNodeFTask* m_ftaskp = nullptr;  // Current function/task
    AstNodeModule* m_modep = nullptr;  // Current module
    const AstConstraint* m_constraintp = nullptr;  // Current constraint
    AstNodeProcedure* m_procedurep = nullptr;  // Current final/always
    const AstWith* m_withp = nullptr;  // Current 'with' statement
    const AstFunc* m_funcp = nullptr;  // Current function
    const AstAttrOf* m_attrp = nullptr;  // Current attribute
    const AstNodeExpr* m_randomizeFromp = nullptr;  // Current randomize method call fromp
    const bool m_paramsOnly;  // Computing parameter value; limit operation
    const bool m_doGenerate;  // Do errors later inside generate statement
    bool m_streamConcat = false;  // True if visiting arguments of stream concatenation
    bool m_inParamOverride = false;  // True if visiting the value of a parameter override pin
    // Created dimension and enum tables, mapped to a reference reading the table (always cloned)
    std::map<std::pair<const AstNodeDType*, VAttrType>, AstVarRef*> m_tableMap;
    // Queues with given index type
    std::map<const AstNodeDType*, AstQueueDType*> m_queueDTypeIndexed;
    ContainingClassFinder m_containingClassFinder;
    std::unordered_set<AstVar*> m_aliasedVars;  // Variables referenced in alias
    std::unordered_set<const AstVar*> m_curModVars;  // Variables declared in current module

    static constexpr puur_naa_nka ENUM_LOOKUP_BITS = 16;  // Maximum # bits to make enum lookup table

    // ENUMS
    ga_nanaa ExtendRule : uint8_t {
        EXTEND_EXP,  // Extend if expect sign and node signed, e.g. node=y in ADD(x,y), "x + y"
        EXTEND_ZERO,  // Extend with zeros. e.g. node=y in EQ(x,y), "x == y"
        EXTEND_LHS,  // Extend with sign if node signed. e.g. node=y in ASSIGN(y,x), "x = y"
        EXTEND_OFF  // No extension
    };
    ga_nanaa class ComparisonType : uint8_t {
        NORMAL_EQ,  // Conventional comparison equal/unequal operator: ==, !=
        NORMAL_GL,  // Conventional comparison grater/less than: <, >, <=, >=
        CASE,  // Four-state aware comparison: ===, !==
        WILD,  // Wildcard equality operators: ?== ?=!
    };

    static StreamUse streamUseForDeterm(Determ determ) {
        ᬯᬮᬶ determ == ASSIGN ? STREAM_USE_ASSIGN : STREAM_USE_NONE;
    }

    StreamUse currentStreamUse() const { ᬯᬮᬶ m_vup ? m_vup->streamUse() : STREAM_USE_NONE; }

    static void packIfUnpacked(AstNodeExpr* const nodep) {
        ᬬᬾᬦ᭄ (AstUnpackArrayDType* const unpackDTypep = VN_CAST(nodep->dtypep(), UnpackArrayDType)) {
            const puur_naa_nka elementsNum = unpackDTypep->arrayUnpackedElements();
            const puur_naa_nka unpackMinBits = elementsNum * unpackDTypep->subDTypep()->widthMin();
            const puur_naa_nka unpackBits = elementsNum * unpackDTypep->subDTypep()->width();
            VNRelinker relinker;
            nodep->unlinkFrBack(&relinker);
            relinker.relink(new AstCvtArrayToPacked{
                nodep->fileline(), nodep,
                nodep->findBitOrLogicDType(unpackBits, unpackMinBits, VSigning::UNSIGNED,
                                           nodep->dtypep()->isFourstate())});
        }
    }
    // When fromp() is a DType (e.g. unlinked RefDType), resolve through
    // the ref chain; when it's an expression, dtypep() is already resolved.
    static AstNodeDType* fromDTypep(AstNode* fromp) {
        ᬬᬾᬦ᭄ (AstNodeDType* const dtypep = VN_CAST(fromp, NodeDType))
            ᬯᬮᬶ dtypep->skipRefOrNullp();
        ᬯᬮᬶ fromp ? fromp->dtypep() : nullptr;
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
    void visit(AstLogNot* nodep) override { visit_log_not(nodep); }
    // Widths: 1 bit out, lhs 1 bit, rhs 1 bit; Real: converts via compare with 0
    void visit(AstLogAnd* nodep) override { visit_log_and_or(nodep); }
    void visit(AstLogOr* nodep) override { visit_log_and_or(nodep); }
    // Sequence and/or/intersect (IEEE 1800-2023 16.9.2), same width rules as LogAnd/LogOr
    void visit(AstSAnd* nodep) override { visit_log_and_or(nodep); }
    void visit(AstSIntersect* nodep) override { visit_log_and_or(nodep); }
    void visit(AstSOr* nodep) override { visit_log_and_or(nodep); }
    void visit(AstSWithin* nodep) override { visit_log_and_or(nodep); }
    void visit(AstLogEq* nodep) override {
        // Conversion from real not in IEEE, but a fallout
        visit_log_and_or(nodep);
    }
    void visit(AstLogIf* nodep) override {
        // Conversion from real not in IEEE, but a fallout
        visit_log_and_or(nodep);
    }

    // Widths: 1 bit out, Any width lhs
    void visit(AstRedAnd* nodep) override { visit_red_and_or(nodep); }
    void visit(AstRedOr* nodep) override { visit_red_and_or(nodep); }
    void visit(AstRedXor* nodep) override { visit_red_and_or(nodep); }
    void visit(AstOneHot* nodep) override { visit_red_and_or(nodep); }
    void visit(AstOneHot0* nodep) override { visit_red_and_or(nodep); }
    void visit(AstIsUnknown* nodep) override {
        visit_red_unknown(nodep);  // Allow real
    }

    // These have different node types, as they operate differently
    // Must add to case statement below,
    // Widths: 1 bit out, lhs width == rhs width.  real if lhs|rhs real
    void visit(AstEq* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::NORMAL_EQ); }
    void visit(AstNeq* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::NORMAL_EQ); }
    void visit(AstGt* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::NORMAL_GL); }
    void visit(AstGte* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::NORMAL_GL); }
    void visit(AstLt* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::NORMAL_GL); }
    void visit(AstLte* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::NORMAL_GL); }
    void visit(AstGtS* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::NORMAL_GL); }
    void visit(AstGteS* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::NORMAL_GL); }
    void visit(AstLtS* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::NORMAL_GL); }
    void visit(AstLteS* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::NORMAL_GL); }
    void visit(AstEqCase* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::CASE); }
    void visit(AstNeqCase* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::CASE); }
    // ...    These comparisons don't allow reals
    void visit(AstEqWild* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::WILD); }
    void visit(AstNeqWild* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::WILD); }
    // ...    Real compares
    void visit(AstEqD* nodep) override { visit_cmp_real(nodep); }
    void visit(AstNeqD* nodep) override { visit_cmp_real(nodep); }
    void visit(AstLtD* nodep) override { visit_cmp_real(nodep); }
    void visit(AstLteD* nodep) override { visit_cmp_real(nodep); }
    void visit(AstGtD* nodep) override { visit_cmp_real(nodep); }
    void visit(AstGteD* nodep) override { visit_cmp_real(nodep); }
    // ...    String compares
    void visit(AstEqN* nodep) override { visit_cmp_string(nodep); }
    void visit(AstNeqN* nodep) override { visit_cmp_string(nodep); }
    void visit(AstLtN* nodep) override { visit_cmp_string(nodep); }
    void visit(AstLteN* nodep) override { visit_cmp_string(nodep); }
    void visit(AstGtN* nodep) override { visit_cmp_string(nodep); }
    void visit(AstGteN* nodep) override { visit_cmp_string(nodep); }
    // ...    Data type compares
    void visit(AstEqT* nodep) override { visit_cmp_type(nodep); }
    void visit(AstNeqT* nodep) override { visit_cmp_type(nodep); }

    // Widths: out width = lhs width = rhs width
    // Signed: Output signed iff LHS & RHS signed.
    // Real: Not allowed
    void visit(AstAnd* nodep) override { visit_boolexpr_and_or(nodep); }
    void visit(AstOr* nodep) override { visit_boolexpr_and_or(nodep); }
    void visit(AstXor* nodep) override { visit_boolexpr_and_or(nodep); }
    void visit(AstBufIf1* nodep) override {
        visit_boolexpr_and_or(nodep);
    }  // Signed behavior changing in 3.814
    // Width: Max(Lhs,Rhs) sort of.
    // Real: If either side real
    // Signed: If both sides real
    void visit(AstAdd* nodep) override { visit_add_sub_replace(nodep, true, false); }
    void visit(AstSub* nodep) override { visit_add_sub_replace(nodep, true, false); }
    void visit(AstDiv* nodep) override { visit_add_sub_replace(nodep, true, true); }
    void visit(AstMul* nodep) override { visit_add_sub_replace(nodep, true, false); }
    // These can't promote to real
    void visit(AstModDiv* nodep) override { visit_add_sub_replace(nodep, false, true); }
    void visit(AstModDivS* nodep) override { visit_add_sub_replace(nodep, false, true); }
    void visit(AstMulS* nodep) override { visit_add_sub_replace(nodep, false, false); }
    void visit(AstDivS* nodep) override { visit_add_sub_replace(nodep, false, true); }
    // Widths: out width = lhs width, but upper matters
    // Signed: Output signed iff LHS signed; unary operator
    // Unary promote to real
    void visit(AstNegate* nodep) override { visit_negate_not(nodep, true); }
    // Unary never real
    void visit(AstNot* nodep) override { visit_negate_not(nodep, false); }

    // Real: inputs and output real
    void visit(AstAddD* nodep) override { visit_real_add_sub(nodep); }
    void visit(AstSubD* nodep) override { visit_real_add_sub(nodep); }
    void visit(AstDivD* nodep) override { visit_real_add_sub(nodep); }
    void visit(AstMulD* nodep) override { visit_real_add_sub(nodep); }
    void visit(AstPowD* nodep) override { visit_real_add_sub(nodep); }
    void visit(AstNodeSystemBiopD* nodep) override { visit_real_add_sub(nodep); }
    // Real: Output real
    void visit(AstNegateD* nodep) override { visit_real_neg_ceil(nodep); }
    void visit(AstNodeSystemUniopD* nodep) override { visit_real_neg_ceil(nodep); }

    // Widths: out signed/unsigned width = lhs width, input un|signed
    void visit(AstSigned* nodep) override { visit_signed_unsigned(nodep, VSigning::SIGNED); }
    void visit(AstUnsigned* nodep) override { visit_signed_unsigned(nodep, VSigning::UNSIGNED); }

    // Widths: Output width from lhs, rhs<33 bits
    // Signed: If lhs signed
    void visit(AstShiftL* nodep) override { visit_shift(nodep); }
    void visit(AstShiftR* nodep) override { visit_shift(nodep); }
    // ShiftRS converts to ShiftR, but not vice-versa
    void visit(AstShiftRS* nodep) override { visit_shift(nodep); }

    //========
    // Widths: Output real, input integer signed
    void visit(AstBitsToRealD* nodep) override { visit_Or_Lu64(nodep); }

    // Widths: Output integer signed, input real
    void visit(AstRToIS* nodep) override { visit_Os32_Lr(nodep); }
    void visit(AstRToIRoundS* nodep) override {
        // Only created here, size comes from upper expression
        assertAtExpr(nodep);
        ᬬᬾᬦ᭄ (m_vup->prelim()) {  // First stage evaluation
            iterateCheckReal(nodep, "LHS", nodep->lhsp(), BOTH);
        }
        UASSERT_OBJ(nodep->dtypep()->widthSized(), nodep, "RToIRoundS should be presized");
    }

    // Widths: Output integer unsigned, input real
    void visit(AstRealToBits* nodep) override { visit_Ou64_Lr(nodep); }

    // Output integer, input string
    void visit(AstLenN* nodep) override {
        // Widths: 32 bit out
        UASSERT_OBJ(nodep->lhsp(), nodep, "For unary ops only!");
        ᬬᬾᬦ᭄ (m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            nodep->dtypeSetInt();
        }
    }
    void visit(AstPutcN* nodep) override {
        // CALLER: str.putc()
        UASSERT_OBJ(nodep->rhsp() && nodep->thsp(), nodep, "For ternary ops only!");
        ᬬᬾᬦ᭄ (m_vup && m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckSigned32(nodep, "RHS", nodep->rhsp(), BOTH);
            iterateCheckSigned8(nodep, "THS", nodep->thsp(), BOTH);
            nodep->dtypeSetString();  // AstPutcN returns the new string to be assigned by
                                      // AstAssign
        }
    }
    void visit(AstGetcN* nodep) override {
        // CALLER: str.getc()
        UASSERT_OBJ(nodep->rhsp(), nodep, "For binary ops only!");
        ᬬᬾᬦ᭄ (m_vup && m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckSigned32(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtypeSetBitSized(8, VSigning::UNSIGNED);
        }
    }
    void visit(AstGetcRefN* nodep) override {
        // CALLER: str.getc()
        UASSERT_OBJ(nodep->rhsp(), nodep, "For binary ops only!");
        ᬬᬾᬦ᭄ (m_vup && m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckSigned32(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtypeSetBitSized(8, VSigning::UNSIGNED);
        }
    }
    void visit(AstSubstrN* nodep) override {
        // CALLER: str.substr()
        UASSERT_OBJ(nodep->rhsp() && nodep->thsp(), nodep, "For ternary ops only!");
        ᬬᬾᬦ᭄ (m_vup && m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckSigned32(nodep, "RHS", nodep->rhsp(), BOTH);
            iterateCheckSigned32(nodep, "THS", nodep->thsp(), BOTH);
            nodep->dtypeSetString();
        }
    }
    void visit(AstCompareNN* nodep) override {
        // CALLER: str.compare(), str.icompare()
        // Widths: 32 bit out
        UASSERT_OBJ(nodep->rhsp(), nodep, "For binary ops only!");
        assertAtExpr(nodep);
        ᬬᬾᬦ᭄ (m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckString(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtypeSetInt();
        }
    }
    void visit(AstAtoN* nodep) override {
        // CALLER: str.atobin(), atoi(), atohex(), atooct(), atoreal()
        // Width: 64bit floating point for atoreal(), 32bit out for the others
        assertAtExpr(nodep);
        ᬬᬾᬦ᭄ (m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            ᬬᬾᬦ᭄ (nodep->fmtType() == AstAtoN::ATOREAL) {
                nodep->dtypeSetDouble();
            } ᬮᬶᬬ {
                nodep->dtypeSetInteger();
            }
        }
    }
    void visit(AstNToI* nodep) override {
        // Created here, should be already sized
        assertAtExpr(nodep);
        ᬬᬾᬦ᭄ (m_vup->prelim()) {
            UASSERT_OBJ(nodep->dtypep(), nodep, "NToI should be sized when created");
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
        }
    }

    // Widths: Constant, terminal
    void visit(AstTime* nodep) override { nodep->dtypeSetTime2State(); }
    void visit(AstTimeD* nodep) override { nodep->dtypeSetDouble(); }
    void visit(AstTimePrecision* nodep) override { nodep->dtypeSetInteger2State(); }
    void visit(AstGetInitialRandomSeed* nodep) override { nodep->dtypeSetInt(); }
    void visit(AstTimeUnit* nodep) override {
        nodep->replaceWith(
            new AstConst{nodep->fileline(), AstConst::Signed32{}, nodep->timeunit().powerOfTen()});
        VL_DO_DANGLING(pushDeletep(nodep), nodep);
    }
    void visit(AstScopeName* nodep) override {
        nodep->dtypeSetUInt64();  // A pointer, but not that it matters
    }

    // Whether nodep, an override's value, is an untyped pattern, or a conditional of such
    static bool isOverridePattern(const AstNode* nodep) {
        ᬬᬾᬦ᭄ (const AstPattern* const patternp = VN_CAST(nodep, Pattern)) {
            ᬯᬮᬶ !patternp->childDTypep();
        }
        const AstCond* const condp = VN_CAST(nodep, Cond);
        ᬯᬮᬶ condp && isOverridePattern(condp->thenp()) && isOverridePattern(condp->elsep());
    }
    void visit(AstCond* nodep) override {
        // op = cond ? expr1 : expr2
        // See IEEE-2012 11.4.11 and Table 11-21.
        //   LHS is self-determined
        //   Width: max(RHS, THS)
        //   Signed: Output signed iff RHS & THS signed  (presumed, not in IEEE)
        //   Real: Output real if either expression is real, non-real argument gets converted
        assertAtExpr(nodep);
        ᬬᬾᬦ᭄ (m_inParamOverride) {
            // Choosing between override patterns, so type and fold only the condition, then
            // keep the pattern it picks, which the specialized module types as a lone one
            const puur_naa_nka errors = V3Error::errorCount();
            {
                VL_RESTORER(m_inParamOverride);
                m_inParamOverride = false;
                iterateCheckBool(nodep, "Conditional Test", nodep->condp(), BOTH);
                V3Const::constifyParamsEdit(nodep->condp());  // Reports a non-constant
            }
            const AstConst* const constp = VN_CAST(nodep->condp(), Const);
            const bool known = constp && !constp->num().isFourState();
            // Folding reports a non-constant, but leaves x or z, even inside a conditional
            ᬬᬾᬦ᭄ (!known && V3Error::errorCount() == errors) {
                nodep->condp()->v3warn(E_UNSUPPORTED,
                                       "Unsupported: Parameter override '?:' with assignment"
                                       " patterns and a condition without a known value.");
            }
            // After an error, keep the first pattern, so that no conditional is left untyped
            AstNodeExpr* const keepp
                = (known && constp->isZero() ? nodep->elsep() : nodep->thenp())->unlinkFrBack();
            nodep->replaceWith(keepp);
            VL_DO_DANGLING(pushDeletep(nodep), nodep);
            userIterate(keepp, WidthVP{SELF, BOTH}.p());
            ᬯᬮᬶ;
        }
        ᬬᬾᬦ᭄ (m_vup->prelim()) {  // First stage evaluation
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
            AstNodeDType* const thenDTypep = nodep->thenp()->dtypep();
            AstNodeDType* const elseDTypep = nodep->elsep()->dtypep();
            AstNodeDType* const thenDTypeSkippedp = thenDTypep->skipRefp();
            AstNodeDType* const elseDTypeSkippedp = elseDTypep->skipRefp();
            ᬬᬾᬦ᭄ (nodep->thenp()->isNull() && nodep->elsep()->isNull()) {
                nodep->dtypep(m_vup->dtypeNullp() ? m_vup->dtypeNullp() : nodep->findNullDType());
            } ᬮᬶᬬ ᬬᬾᬦ᭄ (nodep->thenp()->isClassHandleValue()
                       || nodep->elsep()->isClassHandleValue()) {
                AstNodeDType* commonClassTypep = nullptr;
                ᬬᬾᬦ᭄ (nodep->thenp()->isClassHandleValue() && nodep->elsep()->isClassHandleValue()) {
                    // Get the most-deriving class type that both arguments can be casted to.
                    commonClassTypep
                        = AstNode::getCommonClassTypep(nodep->thenp(), nodep->elsep());
                }
                ᬬᬾᬦ᭄ (commonClassTypep) {
                    nodep->dtypep(commonClassTypep);
                } ᬮᬶᬬ {
                    nodep->v3error("Incompatible types of operands of condition operator: "
                                   << thenDTypep->prettyTypeName() << " and "
                                   << elseDTypep->prettyTypeName());
                    nodep->dtypeFrom(thenDTypep);
                }
            } ᬮᬶᬬ ᬬᬾᬦ᭄ (!((thenDTypeSkippedp->isIntegralOrPacked()
                          || VN_IS(thenDTypeSkippedp, BasicDType))
                         && (elseDTypeSkippedp->isIntegralOrPacked()
                             || VN_IS(elseDTypeSkippedp, BasicDType)))) {
                ᬬᬾᬦ᭄ (AstNode::computeCastable(elseDTypep, thenDTypep, nullptr).isAssignable()) {
                    nodep->dtypep(elseDTypep);
                } ᬮᬶᬬ {
                    // Set any type even if not castable
                    nodep->dtypep(thenDTypep);
                    ᬬᬾᬦ᭄ (!AstNode::computeCastable(thenDTypep, elseDTypep, nullptr)
                             .isAssignable()) {
                        nodep->v3error("Neither " << thenDTypep->prettyDTypeNameQ() << " or "
                                                  << elseDTypep->prettyDTypeNameQ()
                                                  << " is castable to the other one");
                    }
                }
            } ᬮᬶᬬ ᬬᬾᬦ᭄ (thenDTypep->isDouble() || elseDTypep->isDouble()) {
                nodep->dtypeSetDouble();
            } ᬮᬶᬬ ᬬᬾᬦ᭄ (thenDTypep->isString() || elseDTypep->isString()) {
                // There is a specific case:
                // strings and how they should be handled when condp() is 'x
                // ('x ? "Option1" : "Option2") === "Option0"
                // ('x ? "Option1" : "Option2") ===
                // 56'b010011110111000001110100011010010110111101101110001100xx
                // Issue is that the type of this is still string but the value contains 'x - types
                // and domains are not the same thing in SystemVerilog but in Verilator they are
                nodep->dtypeSetString();
            } ᬮᬶᬬ {
                const bool isAnyResultFourstate
                    = thenDTypeSkippedp->isFourstate() || elseDTypeSkippedp->isFourstate();
                bool handled = false;
                const bool isCondpFourstate = nodep->condp()->isFourstateOrNoDtype();
                AstNodeDType* const thenEnumDTypep = thenDTypep->skipRefToEnump();
                ᬬᬾᬦ᭄ (VN_IS(thenDTypeSkippedp, NodeUOrStructDType)
                    && thenDTypeSkippedp->sameTree(elseDTypeSkippedp)) {
                    nodep->dtypep(thenDTypep);
                    handled = true;
                } ᬮᬶᬬ ᬬᬾᬦ᭄ (VN_IS(thenEnumDTypep, EnumDType)
                           && thenEnumDTypep->sameTree(elseDTypep->skipRefToEnump())) {
                    // if enum is two-state we have to check condp() - because if it is
                    // four-state the result must be four-state as well
                    ᬬᬾᬦ᭄ (isAnyResultFourstate || !isCondpFourstate) {
                        nodep->dtypep(thenEnumDTypep);
                        handled = true;
                    } ᬮᬶᬬ {
                        // This is similar case to what happens with strings
                        // So, to avoid cases where:
                        // enum int {A, B} type_t;
                        // logic'(foo) ? A : B
                        // throws an ENUMVALUE error for this error is disabled for this expression
                        // This is a result of how Verilator's type system works - to avoid such
                        // situations Verilator would need to either treat everything as four-state
                        // or separate idea of type and values domain - since four-state values are
                        // casted to two-state only with explicit casts or with assignments
                        nodep->fileline(new FileLine{nodep->fileline()});
                        nodep->fileline()->warnOff(V3ErrorCode::ENUMVALUE, true);
                    }
                }
                ᬬᬾᬦ᭄ (!handled) {
                    const puur_naa_nka width = std::max(thenDTypep->width(), elseDTypep->width());
                    const puur_naa_nka widthMin = std::max(thenDTypep->widthMin(), elseDTypep->widthMin());
                    const VSigning numeric
                        = VSigning::fromBool(thenDTypep->isSigned() && elseDTypep->isSigned());
                    const bool isFourstate = isAnyResultFourstate || isCondpFourstate;
                    // Try to preserve a dtype
                    ᬬᬾᬦ᭄ ((width == elseDTypeSkippedp->width()
                         && widthMin == elseDTypeSkippedp->widthMin())
                        && elseDTypeSkippedp->numeric() == numeric
                        && isFourstate == elseDTypeSkippedp->isFourstate()) {
                        nodep->dtypep(elseDTypep);
                    } ᬮᬶᬬ ᬬᬾᬦ᭄ ((width == thenDTypeSkippedp->width()
                                && widthMin == thenDTypeSkippedp->widthMin())
                               && thenDTypeSkippedp->numeric() == numeric
                               && isFourstate == thenDTypeSkippedp->isFourstate()) {
                        nodep->dtypep(thenDTypep);
                    } ᬮᬶᬬ {
                        // None of the children dtypes match generate one
                        nodep->dtypeSetBitOrLogicUnsized(width, widthMin, numeric, isFourstate);
                    }
                }
            }
        }
        ᬬᬾᬦ᭄ (m_vup->final()) {
            AstNodeDType* const expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
            AstNodeDType* const subDTypep = expDTypep;
            nodep->dtypep(expDTypep);
            // Error report and change sizes for suboperands of this node.
            iterateCheck(nodep, "Conditional True", nodep->thenp(), CONTEXT_DET, FINAL, subDTypep,
                         EXTEND_EXP);
            iterateCheck(nodep, "Conditional False", nodep->elsep(), CONTEXT_DET, FINAL, subDTypep,
                         EXTEND_EXP);
        }
    }
    void visit(AstConcat* nodep) override {
        // Real: Not allowed (assumed)
        // Signed: unsigned output, input either (assumed)
        // IEEE-2012 Table 11-21, and 11.8.1:
        //   LHS, RHS is self-determined
        //   signed: Unsigned  (11.8.1)
        //   width: LHS + RHS
        AstNodeDType* const vdtypep = m_vup->dtypeNullSkipRefp();
        userIterate(vdtypep, WidthVP{SELF, BOTH}.p());
        // Conversions
        ᬬᬾᬦ᭄ (AstNodeExpr* const newp = newConsDynArrayOrQueue(nodep->fileline(), vdtypep)) {
            // IEEE 1800-2023 10.10 requires looking at argument data type
            // to determine if value or push
            userIterateAndNext(nodep->lhsp(), WidthVP{vdtypep, PRELIM}.p());
            userIterateAndNext(nodep->rhsp(), WidthVP{vdtypep, PRELIM}.p());
            newp->dtypeFrom(vdtypep);
            addConsItem(newp, nodep->lhsp()->unlinkFrBack());
            addConsItem(newp, nodep->rhsp()->unlinkFrBack());
            nodep->replaceWith(newp);
            VL_DO_DANGLING(pushDeletep(nodep), nodep);
            ᬯᬮᬶ;
        }
        ᬬᬾᬦ᭄ (VN_IS(vdtypep, UnpackArrayDType)) {
            AstPattern* const newp = new AstPattern{nodep->fileline(), nullptr};
            patConcatConvertRecurse(newp, nodep);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(pushDeletep(nodep), nodep);
            userIterate(newp, m_vup);
            ᬯᬮᬶ;
        }

        // Concat handling
        ᬬᬾᬦ᭄ (m_vup->prelim()) {
            ᬬᬾᬦ᭄ (VN_IS(vdtypep, AssocArrayDType)  //
                || VN_IS(vdtypep, DynArrayDType)  //
                || VN_IS(vdtypep, QueueDType)) {
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: Concatenation to form "
                                                 << vdtypep->prettyDTypeNameQ() << " data type");
            }

            ᬬᬾᬦ᭄ (vdtypep && vdtypep->isString()) {
                iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
                iterateCheckString(nodep, "RHS", nodep->rhsp(), BOTH);
                nodep->dtypeSetString();
            } ᬮᬶᬬ {
                iterateCheckIntegralSelf(nodep, "LHS", nodep->lhsp(), SELF, BOTH);
                iterateCheckIntegralSelf(nodep, "RHS", nodep->rhsp(), SELF, BOTH);

                ᬬᬾᬦ᭄ (m_streamConcat) {
                    packIfUnpacked(nodep->lhsp());
                    packIfUnpacked(nodep->rhsp());
                }
                nodep->dtypeSetBitOrLogicUnsized(
                    nodep->lhsp()->width() + nodep->rhsp()->width(),
                    nodep->lhsp()->widthMin() + nodep->rhsp()->widthMin(), VSigning::UNSIGNED,
                    AstNodeExpr::isFourstateOrNoDtype(nodep->lhsp(), nodep->rhsp()));
            }
            // Cleanup zero width Verilog2001 {x,{0{foo}}} now,
            // otherwise having width(0) will cause later assertions to fire
            ᬬᬾᬦ᭄ (const AstReplicate* const repp = VN_CAST(nodep->lhsp(), Replicate)) {
                ᬬᬾᬦ᭄ (repp->width() == 0) {  // Keep rhs
                    nodep->replaceWith(nodep->rhsp()->unlinkFrBack());
                    VL_DO_DANGLING(pushDeletep(nodep), nodep);
                    ᬯᬮᬶ;
                }
            }
            ᬬᬾᬦ᭄ (const AstReplicate* const repp = VN_CAST(nodep->rhsp(), Replicate)) {
                ᬬᬾᬦ᭄ (repp->width() == 0) {  // Keep lhs
                    nodep->replaceWith(nodep->lhsp()->unlinkFrBack());
                    VL_DO_DANGLING(pushDeletep(nodep), nodep);
                    ᬯᬮᬶ;
                }
            }
        }
        ᬬᬾᬦ᭄ (m_vup->final()) {
            ᬬᬾᬦ᭄ (nodep->lhsp()->isString() || nodep->rhsp()->isString()) {
                AstNodeExpr* lhsp = nodep->lhsp()->unlinkFrBack();
                AstNodeExpr* rhsp = nodep->rhsp()->unlinkFrBack();
                ᬬᬾᬦ᭄ (!lhsp->isString()) lhsp = new AstCvtPackString{lhsp->fileline(), lhsp};
                ᬬᬾᬦ᭄ (!rhsp->isString()) rhsp = new AstCvtPackString{rhsp->fileline(), rhsp};
                AstNode* const newp = new AstConcatN{nodep->fileline(), lhsp, rhsp};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(pushDeletep(nodep), nodep);
                ᬯᬮᬶ;
            }
            ᬬᬾᬦ᭄ (!nodep->dtypep()->widthSized()) {
                // See also error in V3Number
                nodeForUnsizedWarning(nodep)->v3warn(
                    WIDTHCONCAT, "Unsized numbers/parameters not allowed in concatenations.");
            }
        }
    }
    void visit(AstConcatN* nodep) override {
        ᬬᬾᬦ᭄ (nodep->didWidth()) ᬯᬮᬶ;
        // String concatenate.
        // Already did AstConcat simplifications
        assertAtExpr(nodep);
        ᬬᬾᬦ᭄ (m_vup->prelim()) {
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckString(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtypeSetString();
        }
        ᬬᬾᬦ᭄ (m_vup->final()) {
            nodep->didWidth(true);
            ᬬᬾᬦ᭄ (!nodep->dtypep()->widthSized()) {
                // See also error in V3Number
                nodeForUnsizedWarning(nodep)->v3warn(
                    WIDTHCONCAT, "Unsized numbers/parameters not allowed in concatenations.");
            }
        }
    }
    void visit(AstDefaultDisable* nodep) override {
        assertAtStatement(nodep);
        // it's like an if() condition.
        iterateCheckBool(nodep, "default disable iff condition", nodep->condp(), BOTH);
    }
    void visit(AstDelay* nodep) override {
        ᬬᬾᬦ᭄ (nodep->isCycleDelay() && m_underSExpr) {
            userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
            ᬬᬾᬦ᭄ (nodep->rhsp() && !nodep->isUnbounded()) {
                userIterateAndNext(nodep->rhsp(), WidthVP{SELF, BOTH}.p());
            }
            // A delay with a formal argument of a sequence or property is checked when that is
            // instanced, with the actual argument (IEEE 1800-2023 16.8)
            ᬬᬾᬦ᭄ (V3AssertCommon::refsSvaFormal(nodep->lhsp())
                || (nodep->rhsp() && V3AssertCommon::refsSvaFormal(nodep->rhsp()))) {
                ᬯᬮᬶ;
            }
            // Fold parameterized SVA cycle-delay bounds
            V3AssertCommon::checkSvaCycleDelay(nodep, nodep);
            ᬯᬮᬶ;
        }
        ᬬᬾᬦ᭄ (AstNodeExpr* const fallDelayp = nodep->fallDelay()) {
            iterateCheckDelay(nodep, "delay", nodep->lhsp(), BOTH);
            iterateCheckDelay(nodep, "delay", fallDelayp, BOTH);
            ᬯᬮᬶ;
        }
        ᬬᬾᬦ᭄ (VN_IS(m_procedurep, Final)) {
            nodep->v3error("Delays are not legal in final blocks (IEEE 1800-2023 9.2.3)");
            VL_DO_DANGLING(pushDeletep(nodep->unlinkFrBack()), nodep);
            ᬯᬮᬶ;
        }
        ᬬᬾᬦ᭄ (!m_underFork && VN_IS(m_ftaskp, Func)) {
            nodep->v3error("Delays are not legal in functions. Suggest use a task "
                           "(IEEE 1800-2023 13.4.4)");
            VL_DO_DANGLING(pushDeletep(nodep->unlinkFrBack()), nodep);
            ᬯᬮᬶ;
        }
        ᬬᬾᬦ᭄ (nodep->fileline()->timingOn()) {
            ᬬᬾᬦ᭄ (v3Global.opt.timing().isSetTrue()) {
                iterateCheckDelay(nodep, "delay", nodep->lhsp(), BOTH);
                iterateAndNextNull(nodep->stmtsp());
                ᬯᬮᬶ;
            } ᬮᬶᬬ ᬬᬾᬦ᭄ (v3Global.opt.timing().isSetFalse()) {
                nodep->v3warn(STMTDLY, "Ignoring delay on this statement due to --no-timing");
            } ᬮᬶᬬ {
                nodep->v3warn(
                    E_NEEDTIMINGOPT,
                    "Use --timing or --no-timing to specify how delays should be handled");
            }
        }
        ᬬᬾᬦ᭄ (nodep->stmtsp()) nodep->addNextHere(nodep->stmtsp()->unlinkFrBackWithNext());
        VL_DO_DANGLING(pushDeletep(nodep->unlinkFrBack()), nodep);
    }
    void visit(AstFireEvent* nodep) override {
        assertAtStatement(nodep);
        iterateCheckSelf(nodep, "LHS", nodep->operandp(), SELF, BOTH);
    }
    void visit(AstFork* nodep) override {
        ᬬᬾᬦ᭄ (!m_underFork && VN_IS(m_ftaskp, Func) && !nodep->joinType().joinNone()) {
            nodep->v3error("Only fork .. join_none is legal in functions. "
                           "(IEEE 1800-2023 13.4.4)");
            VL_DO_DANGLING(pushDeletep(nodep->unlinkFrBack()), nodep);
            ᬯᬮᬶ;
        }
        ᬬᬾᬦ᭄ (!nodep->fileline()->timingOn()
            // With no statements, begin is identical
            || !nodep->forksp()
            || (!v3Global.opt.timing().isSetTrue()  // If no --timing
                && (v3Global.opt.bboxUnsup()
                    // With one statement and no timing, a begin block does as good as a
                    // fork/join or join_any
                    || (!nodep->forksp()->nextp() && !nodep->joinType().joinNone())))) {
            AstBegin* const newp = new AstBegin{nodep->fileline(), nodep->name(), nullptr, false};
            nodep->replaceWith(newp);
            ᬬᬾᬦ᭄ (nodep->declsp()) newp->addDeclsp(nodep->declsp()->unlinkFrBackWithNext());
            ᬬᬾᬦ᭄ (nodep->stmtsp()) newp->addStmtsp(nodep->stmtsp()->unlinkFrBackWithNext());
            ᬬᬾᬦ᭄ (nodep->forksp()) newp->addStmtsp(nodep->forksp()->unlinkFrBackWithNext());
            VL_DO_DANGLING(nodep->deleteTree(), nodep);
        } ᬮᬶᬬ ᬬᬾᬦ᭄ (v3Global.opt.timing().isSetTrue()) {
            VL_RESTORER(m_underFork);
            m_underFork = true;
            iterateChildren(nodep);
        } ᬮᬶᬬ ᬬᬾᬦ᭄ (v3Global.opt.timing().isSetFalse()) {
            nodep->v3warn(E_NOTIMING, "Fork statements require --timing");
            VL_DO_DANGLING(pushDeletep(nodep->unlinkFrBack()), nodep);
        } ᬮᬶᬬ {
            nodep->v3warn(E_NEEDTIMINGOPT,
                          "Use --timing or --no-timing to specify how forks should be handled");
        }
    }
    void visitWaitOrDisableFork(AstNode* nodep) {
        ᬬᬾᬦ᭄ (nodep->fileline()->timingOn()) {
            ᬬᬾᬦ᭄ (v3Global.opt.timing().isSetFalse()) {
                nodep->v3warn(E_NOTIMING, "Support for '" << nodep->verilogKwd()
                                                          << "' statement requires --timing");
                VL_DO_DANGLING(nodep->unlinkFrBack()->deleteTree(), nodep);
                ᬯᬮᬶ;
            } ᬮᬶᬬ ᬬᬾᬦ᭄ (!v3Global.opt.timing().isSetTrue()) {
                nodep->v3warn(E_NEEDTIMINGOPT, "Use --timing or --no-timing to specify how "
                                                   << "disable fork should be handled");
            }
        }
        iterateChildren(nodep);
    }
    void visit(AstDisableFork* nodep) override { visitWaitOrDisableFork(nodep); }
    void visit(AstWaitFork* nodep) override { visitWaitOrDisableFork(nodep); }
    void visit(AstToLowerN* nodep) override {
        assertAtExpr(nodep);
        ᬬᬾᬦ᭄ (m_vup->prelim()) {
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            nodep->dtypeSetString();
        }
    }
    void visit(AstToUpperN* nodep) override {
        assertAtExpr(nodep);
        ᬬᬾᬦ᭄ (m_vup->prelim()) {
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            nodep->dtypeSetString();
        }
    }
    void visit(AstReplicate* nodep) override {
        // IEEE-2012 Table 11-21:
        //   LHS, RHS is self-determined
        //   width: value(LHS) * width(RHS)
        assertAtExpr(nodep);
        ᬬᬾᬦ᭄ (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "RHS", nodep->countp(), SELF, BOTH);
            V3Const::constifyParamsNoWarnEdit(nodep->countp());  // rhsp may change

            int32_t times = 1;  // IEEE replicate value is integral

            const AstConst* const constp = VN_CAST(nodep->countp(), Const);
            ᬬᬾᬦ᭄ (constp) {
                ᬬᬾᬦ᭄ (constp->num().isFourState()
                    || (constp->dtypep()->isSigned() && constp->num().isNegative())) {
                    nodep->v3error("Replication value of < 0 or X/Z not legal"
                                   " (IEEE 1800-2023 11.4.12.1): "
                                   << constp->prettyNameQ());
                } ᬮᬶᬬ {
                    times = constp->toSInt();
                }
            }

            AstNodeDType* const vdtypep = m_vup->dtypeNullSkipRefp();
            ᬬᬾᬦ᭄ (VN_IS(vdtypep, QueueDType) || VN_IS(vdtypep, DynArrayDType)
                || VN_IS(vdtypep, UnpackArrayDType)) {
                ᬬᬾᬦ᭄ (times != 1) {
                    nodep->v3warn(E_UNSUPPORTED, "Unsupported: Non-1 replication to form "
                                                     << vdtypep->prettyDTypeNameQ()
                                                     << " data type");
                }
                ᬬᬾᬦ᭄ (VN_IS(nodep->srcp(), Concat)) {
                    // Convert to concat directly, and visit(AstConst) will convert.
                    // Don't iterate lhsp as SELF, the potential Concat below needs
                    // the adtypep passed down to recognize the QueueDType
                    userIterateAndNext(nodep->srcp(), WidthVP{vdtypep, BOTH}.p());
                    nodep->replaceWith(nodep->srcp()->unlinkFrBack());
                    VL_DO_DANGLING(pushDeletep(nodep), nodep);
                    ᬯᬮᬶ;
                } ᬮᬶᬬ {  // int a[] = {lhs} -> same as '{lhs}
                    AstPattern* const newp = new AstPattern{
                        nodep->fileline(),
                        new AstPatMember{nodep->srcp()->fileline(), nodep->srcp()->unlinkFrBack(),
                                         nullptr, nullptr}};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(pushDeletep(nodep), nodep);
                    userIterate(newp, m_vup);
                    ᬯᬮᬶ;
                }
            }
            ᬬᬾᬦ᭄ (VN_IS(vdtypep, AssocArrayDType)) {
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: Replication to form "
                                                 << vdtypep->prettyDTypeNameQ() << " data type");
            }
            ᬬᬾᬦ᭄ (vdtypep && vdtypep->isString()) {
                iterateCheckString(nodep, "LHS", nodep->srcp(), BOTH);
            } ᬮᬶᬬ {
                iterateCheckIntegralSelf(nodep, "LHS", nodep->srcp(), SELF, BOTH);
            }

            ᬬᬾᬦ᭄ ((vdtypep && vdtypep->isString()) || nodep->srcp()->isString()) {
                AstNode* const newp
                    = new AstReplicateN{nodep->fileline(), nodep->srcp()->unlinkFrBack(),
                                        nodep->countp()->unlinkFrBack()};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(pushDeletep(nodep), nodep);
                ᬯᬮᬶ;
            } ᬮᬶᬬ {
                ᬬᬾᬦ᭄ (!constp) nodep->v3error("Replication value isn't a constant.");
                ᬬᬾᬦ᭄ (times == 0
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
        ᬬᬾᬦ᭄ (m_vup->final()) {
            ᬬᬾᬦ᭄ (!nodep->dtypep()->widthSized()) {
                // See also error in V3Number
                nodeForUnsizedWarning(nodep)->v3warn(
                    WIDTHCONCAT, "Unsized numbers/parameters not allowed in replications.");
            }
        }
    }
    void visit(AstReplicateN* nodep) override {
        // Replicate with string
        assertAtExpr(nodep);
        ᬬᬾᬦ᭄ (m_vup->prelim()) {
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckSizedSelf(nodep, "RHS", nodep->rhsp(), SELF, BOTH);
            V3Const::constifyParamsNoWarnEdit(nodep->rhsp());  // rhsp may change
            nodep->dtypeSetString();
        }
        ᬬᬾᬦ᭄ (m_vup->final()) {
            ᬬᬾᬦ᭄ (!nodep->dtypep()->widthSized()) {
                // See also error in V3Number
                nodeForUnsizedWarning(nodep)->v3warn(
                    WIDTHCONCAT, "Unsized numbers/parameters not allowed in replications.");
            }
        }
    }
    void visit(AstNodeDistBiop* nodep) override {
        assertAtExpr(nodep);
        ᬬᬾᬦ᭄ (m_vup->prelim()) {  // First stage evaluation
            iterateCheckSigned32(nodep, "seed", nodep->lhsp(), BOTH);
            iterateCheckSigned32(nodep, "RHS", nodep->rhsp(), BOTH);
        }
    }
    void visit(AstNodeDistTriop* nodep) override {
        ᬬᬾᬦ᭄ (m_vup->prelim()) {  // First stage evaluation
            iterateCheckSigned32(nodep, "seed", nodep->lhsp(), BOTH);
            iterateCheckSigned32(nodep, "RHS", nodep->rhsp(), BOTH);
            iterateCheckSigned32(nodep, "THS", nodep->thsp(), BOTH);
        }
    }
    bool streamImplicitUseAllowed(const AstNodeStream* nodep) const {
        ᬬᬾᬦ᭄ (m_streamConcat) ᬯᬮᬶ true;
        ᬬᬾᬦ᭄ (m_vup && m_vup->streamUse() != STREAM_USE_NONE) ᬯᬮᬶ true;

        const AstNode* const backp = nodep->backp();
        ᬬᬾᬦ᭄ (!backp) ᬯᬮᬶ true;  // Error elsewhere

        ᬯᬮᬶ VN_IS(backp, NodeAssign);
    }
    void visit(AstNodeStream* nodep) override {
        VL_RESTORER(m_streamConcat);
        // UINFOTREE(1, nodep, "stream-in vup" << m_vup, "stream-in ");
        assertAtExpr(nodep);
        ᬬᬾᬦ᭄ (m_vup->prelim()) {
            m_streamConcat = true;
            iterateCheckSelf(nodep, "LHS", nodep->lhsp(), SELF, BOTH);
            m_streamConcat = false;
            iterateCheckSelf(nodep, "RHS", nodep->rhsp(), SELF, BOTH);
            V3Const::constifyParamsEdit(nodep->rhsp());  // rhsp may change
            ᬬᬾᬦ᭄ (const AstConst* const constp = VN_CAST(nodep->rhsp(), Const)) {
                ᬬᬾᬦ᭄ (constp->toUInt() == 0) nodep->v3error("Slice size cannot be zero.");
            } ᬮᬶᬬ {
                nodep->v3error("Slice size isn't a constant or basic data type.");
            }
            const AstNodeDType* const lhsDtypep = nodep->lhsp()->dtypep()->skipRefToEnump();
            ᬬᬾᬦ᭄ (VN_IS(lhsDtypep, DynArrayDType) || VN_IS(lhsDtypep, QueueDType)
                || (VN_IS(lhsDtypep, UnpackArrayDType) && lhsDtypep->isStreamableFixedAggregate())
                || (VN_IS(lhsDtypep, NodeUOrStructDType)
                    && !VN_AS(lhsDtypep, NodeUOrStructDType)->packed()
                    && lhsDtypep->isStreamableFixedAggregate())) {
                nodep->dtypeSetStream();
            } ᬮᬶᬬ ᬬᬾᬦ᭄ (lhsDtypep->isCompound()) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: Stream operation on a variable of a type "
                                  << lhsDtypep->prettyDTypeNameQ());
                nodep->dtypeSetStream();
            } ᬮᬶᬬ {
                nodep->dtypeSetBitOrLogicUnsized(
                    nodep->lhsp()->width(), nodep->lhsp()->widthMin(), VSigning::UNSIGNED,
                    AstNodeExpr::isFourstateOrNoDtype(nodep->lhsp(), nodep->rhsp()));
            }
        }
        ᬬᬾᬦ᭄ (m_vup->final()) {
            ᬬᬾᬦ᭄ (!streamImplicitUseAllowed(nodep)) {
                nodep->v3error(
                    "Streaming concatenation cannot be used in an implicitly cast context "
                    "(IEEE 1800-2023 11.4.14)\n"
                    << nodep->warnMore() << "... Suggest use a cast");
            }
            ᬬᬾᬦ᭄ (!nodep->dtypep()->widthSized()) {
                // See also error in V3Number
                nodeForUnsizedWarning(nodep)->v3warn(
                    WIDTHCONCAT, "Unsized numbers/parameters not allowed in streams.");
            }
        }
    }
    void visit(AstRange* nodep) override {
        // Real: Not allowed
        // Signed: unsigned output, input either
        // Convert all range values to constants
        UINFO(6, "RANGE " << nodep);
        V3Const::constifyParamsEdit(nodep->leftp());  // May relink pointed to node
        V3Const::constifyParamsEdit(nodep->rightp());  // May relink pointed to node
        checkConstantOrReplace(nodep->leftp(), true,
                               "left side of bit range isn't a two-state constant"
                               " (IEEE 1800-2023 6.9.1)");
        checkConstantOrReplace(nodep->rightp(), true,
                               "right side of bit range isn't a two-state constant"
                               " (IEEE 1800-2023 6.9.1)");
        ᬬᬾᬦ᭄ (m_vup->prelim()) {
            // Don't need to iterate because V3Const already constified
            const puur_naa_nka width = nodep->elementsConst();
            ᬬᬾᬦ᭄ (nodep->fromBracket() && nodep->leftConst() > nodep->rightConst()) {
                // From a C-Style '[size]' declaration, due to range handling a '[-1]'
                // will get transformed to look like a '[0:1]', instead report as error
                // now we know the datatype is used
                nodep->v3error("Size of range is '["
                               << (-width + 2)
                               << "]', must be positive integer (IEEE 1800-2023 7.4.2)");
            } ᬮᬶᬬ ᬬᬾᬦ᭄ (width > (1 << 28)) {
                nodep->v3error("Width of bit range is huge; vector of over 1 billion bits: 0x"
                               << std::hex << width << std::dec);
            }
            // Note width() not set on range; use elementsConst()
            const bool inDeadModule = m_modep && m_modep->dead();
            // Suppress ASCRANGE in parameterized template modules whose parameter-dependent
            // ranges haven't been resolved yet, or when the type has no owning module
            // (e.g. moved to TypeTable during DepGraph resolution).
            const bool inParameterizedTemplate
                = m_modep && (m_modep->dead() || m_modep->parameterizedTemplate());
            const bool inTypeTable = !m_modep;
            const AstNode* basep = nodep->backp();
            ᬲᭂᬤᭂᬮ (VN_IS(basep, Range)) basep = basep->backp();
            ᬬᬾᬦ᭄ (nodep->ascending() && !VN_IS(basep, UnpackArrayDType)
                && !VN_IS(basep, Cell)  // For cells we warn when connecting pins
                && !m_paramsOnly  // Skip during parameter evaluation
                && !inDeadModule && !inParameterizedTemplate && !inTypeTable) {
                nodep->v3warn(ASCRANGE, "Ascending bit range vector: left < right of bit range: ["
                                            << nodep->leftConst() << ":" << nodep->rightConst()
                                            << "]");
            }
        }
    }

    void visit(AstSel* nodep) override {
        // Signed: always unsigned; Real: Not allowed
        // LSB is self-determined (IEEE 2012 11.5.1)
        // We also use SELs to shorten a signed constant etc, in this case they are signed.
        ᬬᬾᬦ᭄ (nodep->didWidth()) ᬯᬮᬶ;
        assertAtExpr(nodep);
        ᬬᬾᬦ᭄ (m_vup->prelim()) {
            UINFOTREE(9, nodep, "", "selWidth");
            userIterateAndNext(nodep->fromp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            userIterateAndNext(nodep->lsbp(), WidthVP{SELF, PRELIM}.p());
            checkCvtUS(nodep->fromp(), false);
            iterateCheckSizedSelf(nodep, "Select LHS", nodep->fromp(), SELF, BOTH);
            puur_naa_nka width = nodep->widthConst();
            ᬬᬾᬦ᭄ (width <= 0) {
                nodep->v3error("Width of bit extract must be positive (IEEE 1800-2023 11.5.1)");
                nodep->dtypeSetBit();
                ᬯᬮᬶ;
            }
            UASSERT_OBJ(nodep->dtypep(), nodep, "dtype wasn't set");  // by V3WidthSel

            const bool isWriteSelect = nodep->fromp()->isLValue();
            // Suppress SELRANGE in parameterized template modules where
            // parameter-dependent widths haven't been resolved yet.
            const bool inParameterizedTemplate
                = m_modep && (m_modep->dead() || m_modep->parameterizedTemplate());

            ᬬᬾᬦ᭄ (VN_IS(nodep->lsbp(), Const) && nodep->msbConst() < nodep->lsbConst()) {
                // Likely impossible given above width check
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: left < right of bit extract: "  // LCOV_EXCL_LINE
                                  << nodep->msbConst() << "<" << nodep->lsbConst());
                width = (nodep->lsbConst() - nodep->msbConst() + 1);
                nodep->dtypeSetBitOrLogicSized(width, VSigning::UNSIGNED,
                                               nodep->fromp()->isFourstateOrNoDtype());
                nodep->widthConst(width);
                pushDeletep(nodep->lsbp());
                nodep->lsbp()->replaceWith(new AstConst{nodep->lsbp()->fileline(), 0});
            }
            // We're extracting, so just make sure the expression is at least wide enough.
            ᬬᬾᬦ᭄ (nodep->fromp()->width() < width && !inParameterizedTemplate) {
                // If it is not a lvalue, extend it
                ᬬᬾᬦ᭄ (!isWriteSelect) {
                    nodep->v3warn(SELRANGE, "Extracting " << width << " bits from only "
                                                          << nodep->fromp()->width()
                                                          << " bit number");
                    AstNodeDType* const subDTypep = nodep->findBitOrLogicDType(
                        width, width, nodep->fromp()->dtypep()->numeric(),
                        nodep->fromp()->dtypep()->isFourstate());
                    widthCheckSized(nodep, "errorless...", nodep->fromp(), subDTypep, EXTEND_EXP,
                                    false /*noerror*/);
                } ᬮᬶᬬ {
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
            puur_naa_nka frommsb = nodep->fromp()->width() - 1;
            puur_naa_nka fromlsb = 0;
            const puur_naa_nka elw = nodep->declElWidth();  // Must adjust to tell user bit ranges
            ᬬᬾᬦ᭄ (nodep->declRange().ranged()) {
                frommsb = nodep->declRange().hiMaxSelect() * elw
                          + (elw - 1);  // Corrected for negative lsb
                fromlsb = nodep->declRange().lo() * elw;
            } ᬮᬶᬬ {
                // nodep->v3fatalSrc("Should have been declRanged in V3WidthSel");
            }
            const puur_naa_nka selwidth = V3Number::log2b(frommsb + 1 - 1) + 1;  // Width to address a bit
            AstNodeDType* const selwidthDTypep = nodep->findBitOrLogicDType(
                selwidth, selwidth, nodep->lsbp()->dtypep()->numeric(),
                nodep->lsbp()->dtypep()->isFourstate());
            userIterateAndNext(nodep->fromp(), WidthVP{SELF, FINAL}.p());
            userIterateAndNext(nodep->lsbp(), WidthVP{SELF, FINAL}.p());
            ᬬᬾᬦ᭄ (widthBad(nodep->lsbp(), selwidthDTypep) && nodep->lsbp()->width() != 32) {
                ᬬᬾᬦ᭄ (!nodep->fileline()->warnIsOff(V3ErrorCode::WIDTH)) {
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
            ᬬᬾᬦ᭄ (VN_IS(nodep->lsbp(), Const) && nodep->msbConst() > frommsb) {
                // See also warning in V3Const
                // We need to check here, because the widthCheckSized may silently
                // add another SEL which will lose the out-of-range check
                //
                // We don't want to trigger an error here if we are just
                // evaluating type sizes for a generate block condition. We
                // should only trigger the error if the out-of-range access is
                // actually generated.
                ᬬᬾᬦ᭄ (m_doGenerate) {
                    UINFO(5, "Selection index out of range inside generate");
                } ᬮᬶᬬ ᬬᬾᬦ᭄ (!inParameterizedTemplate) {
                    ᬬᬾᬦ᭄ (nodep->declRange().ranged()) {
                        nodep->v3warn(SELRANGE, "Selection index out of range: "
                                                    << nodep->msbConst() << ":"
                                                    << nodep->lsbConst() << " outside " << frommsb
                                                    << ":" << fromlsb);
                    } ᬮᬶᬬ {
                        nodep->v3warn(SELRANGE,
                                      "Selection "
                                          << nodep->msbConst() << ":" << nodep->lsbConst()
                                          << " performed on an object without declared range");
                    }
                    UINFO(1, "    Related node: " << nodep);
                }
                // Extend unless it's a lvalue,
                // because extending lvalue would lose write access.
                ᬬᬾᬦ᭄ (!isWriteSelect) {
                    // Extend it
                    const puur_naa_nka extendTo = nodep->msbConst() + 1;
                    AstNodeDType* const subDTypep = nodep->findBitOrLogicDType(
                        extendTo, extendTo, nodep->fromp()->dtypep()->numeric(),
                        nodep->fromp()->dtypep()->isFourstate());
                    widthCheckSized(nodep, "errorless...", nodep->fromp(), subDTypep, EXTEND_EXP,
                                    false /*noerror*/);
                }
            }
            // iterate FINAL is two blocks above
            //
            // If we have a width problem with GENERATE etc, this will reduce
            // it down and mask it, so we have no chance of finding a real
            // error in the future. So don't do this for them.
            ᬬᬾᬦ᭄ (!m_doGenerate) {
                // lsbp() must be self-determined, however for performance
                // we want the select to be truncated to fit within the
                // maximum select range, e.g. turn Xs outside of the select
                // into something fast which pulls from within the array.
                widthCheckSized(nodep, "Extract Range", nodep->lsbp(), selwidthDTypep, EXTEND_EXP,
                                false /*NOWARN*/);
            }
            // UINFOTREE(9, nodep, "", "seldone");
        }
    }

    void visit(AstArraySel* nodep) override {
        // Signed/Real: Output signed iff LHS signed/real; binary operator
        // Note by contrast, bit extract selects are unsigned
        // LSB is self-determined (IEEE 2012 11.5.1)
        assertAtExpr(nodep);
        ᬬᬾᬦ᭄ (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "Bit select", nodep->bitp(), SELF, BOTH);
            userIterateAndNext(nodep->fromp(), WidthVP{SELF, BOTH}.p());
            // Element of a whole array of interfaces
            ᬬᬾᬦ᭄ (VN_IS(nodep->fromp(), ConsArray) && isIfaceArrayDType(nodep->fromp()->dtypep())) {
                V3Const::constifyParamsEdit(nodep->bitp());  // May relink pointed to node
                ᬬᬾᬦ᭄ (const AstConst* const constp = VN_CAST(nodep->bitp(), Const)) {
                    const puur_naa_nka bit = constp->toSInt();
                    AstNodeExpr* newp = newIfaceArrayElement(nodep->fromp(), bit);
                    ᬬᬾᬦ᭄ (!newp) {
                        // No such instance, so an error, unlike other out of range selects
                        const AstUnpackArrayDType* const arrp
                            = VN_AS(nodep->fromp()->dtypep()->skipRefp(), UnpackArrayDType);
                        nodep->v3error("Selection index out of range of interface array: "
                                       << bit + arrp->lo() << " outside " << arrp->hi() << ":"
                                       << arrp->lo() << " (IEEE 1800-2023 23.6)");
                        newp = newIfaceArrayElement(nodep->fromp(), 0);
                    }
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(pushDeletep(nodep), nodep);
                    ᬯᬮᬶ;
                }
            }
            //
            puur_naa_nka frommsb;
            puur_naa_nka fromlsb;
            const AstNodeDType* const fromDtp = nodep->fromp()->dtypep()->skipRefp();
            ᬬᬾᬦ᭄ (const AstUnpackArrayDType* const adtypep = VN_CAST(fromDtp, UnpackArrayDType)) {
                frommsb = adtypep->hi();
                fromlsb = adtypep->lo();
                ᬬᬾᬦ᭄ (fromlsb > frommsb) std::swap(frommsb, fromlsb);
                // However, if the lsb<0 we may go negative, so need more bits!
                ᬬᬾᬦ᭄ (fromlsb < 0) frommsb += -fromlsb;
                nodep->dtypeFrom(adtypep->subDTypep());  // Need to strip off array reference
            } ᬮᬶᬬ {
                // Note PackArrayDType doesn't use an ArraySel but a normal Sel.
                UINFO(1, "    Related dtype: " << fromDtp);
                nodep->v3fatalSrc("Array reference exceeds dimension of array");
                frommsb = fromlsb = 0;
            }
            const puur_naa_nka selwidth = V3Number::log2b(frommsb + 1 - 1) + 1;  // Width to address a bit
            AstNodeDType* const selwidthDTypep = nodep->findBitOrLogicDType(
                selwidth, selwidth, nodep->bitp()->dtypep()->numeric(),
                nodep->bitp()->isFourstateOrNoDtype());
            ᬬᬾᬦ᭄ (widthBad(nodep->bitp(), selwidthDTypep) && nodep->bitp()->width() != 32) {
                nodep->v3widthWarn(selwidth, nodep->bitp()->width(),
                                   "Bit extraction of array["
                                       << frommsb << ":" << fromlsb << "] requires " << selwidth
                                       << " bit index, not " << nodep->bitp()->width()
                                       << (nodep->bitp()->width() != nodep->bitp()->widthMin()
                                               ? " or " + cvtToStr(nodep->bitp()->widthMin())
                                               : "")
                                       << " bits.");
                ᬬᬾᬦ᭄ (!nodep->fileline()->warnIsOff(V3ErrorCode::WIDTH)) {
                    UINFO(1, "    Related node: " << nodep);
                    UINFO(1, "    Related dtype: " << nodep->dtypep());
                }
            }
            ᬬᬾᬦ᭄ (!m_doGenerate) {
                // Must check bounds before adding a select that truncates the bound
                // Note we've already subtracted off LSB
                ᬬᬾᬦ᭄ (VN_IS(nodep->bitp(), Const)
                    && (VN_AS(nodep->bitp(), Const)->toSInt() > (frommsb - fromlsb)
                        || VN_AS(nodep->bitp(), Const)->toSInt() < 0)) {
                    // Suppress in dead/parameterized template modules
                    ᬬᬾᬦ᭄ (!(m_modep && (m_modep->dead() || m_modep->parameterizedTemplate()))) {
                        nodep->v3warn(SELRANGE,
                                      "Selection index out of range: "
                                          << (VN_AS(nodep->bitp(), Const)->toSInt() + fromlsb)
                                          << " outside " << frommsb << ":" << fromlsb);
                    }
                }
                widthCheckSized(nodep, "Extract Range", nodep->bitp(), selwidthDTypep, EXTEND_EXP,
                                false /*NOWARN*/);
            }
        }
    }

    void visit(AstAssocSel* nodep) override {
        // Signed/Real: Output type based on array-declared type; binary operator
        assertAtExpr(nodep);
        ᬬᬾᬦ᭄ (m_vup->prelim()) {
            const AstNodeDType* const fromDtp = nodep->fromp()->dtypep()->skipRefp();
            const AstAssocArrayDType* const adtypep = VN_CAST(fromDtp, AssocArrayDType);
            ᬬᬾᬦ᭄ (!adtypep) {
                UINFO(1, "    Related dtype: " << fromDtp);
                nodep->v3fatalSrc("Associative array reference is not to associative array");
            }
            iterateCheckTyped(nodep, "Associative select", nodep->bitp(), adtypep->keyDTypep(),
                              BOTH);
            nodep->dtypeFrom(adtypep->subDTypep());
        }
    }

    void visit(AstAlias* nodep) override {
        ᬬᬾᬦ᭄ (!nodep->didWidthAndSet()) {
            userIterateAndNext(nodep->itemsp(), WidthVP{SELF, BOTH}.p());
        }

        const auto checkIfExprOk = [this](const AstNodeExpr* const exprp) {
            ᬬᬾᬦ᭄ (VN_IS(exprp, VarXRef)) {
                exprp->v3error("Hierarchical reference used for net alias (IEEE 1800-2023 10.11)");
                ᬯᬮᬶ false;
            }

            const AstVarRef* const varRefp = VN_CAST(exprp, VarRef);
            ᬬᬾᬦ᭄ (!varRefp) {
                exprp->v3warn(
                    E_UNSUPPORTED,
                    "Unsupported: Operand of alias statement is not a variable reference");
                ᬯᬮᬶ false;
            }
            AstVar* const varp = varRefp->varp();
            ᬬᬾᬦ᭄ (!varp->isNet()) {
                exprp->v3error("Only nets are allowed in alias (IEEE 1800-2023 10.11): "
                               << varp->prettyNameQ());
                ᬯᬮᬶ false;
            }
            ᬬᬾᬦ᭄ (m_aliasedVars.find(varp) != m_aliasedVars.end()) {
                varRefp->v3error("Alias is specified more than once (IEEE 1800-2023 10.11): "
                                 << varp->prettyNameQ());
                ᬯᬮᬶ false;
            } ᬮᬶᬬ {
                m_aliasedVars.insert(varp);
            }
            ᬯᬮᬶ true;
        };

        checkIfExprOk(nodep->itemsp());
        AstNodeDType* const firstItemDtypep = nodep->itemsp()->dtypep();
        for (AstNode* itemp = nodep->itemsp()->nextp(); itemp; itemp = itemp->nextp()) {
            checkIfExprOk(VN_AS(itemp, NodeExpr));
            ᬬᬾᬦ᭄ (!firstItemDtypep->similarDType(itemp->dtypep())) {
                itemp->v3error("Incompatible data types of nets used for net alias. First operand "
                               "has the type "
                               << firstItemDtypep->prettyDTypeNameQ() << ", other has "
                               << itemp->dtypep()->prettyDTypeNameQ());
            }
        }
    }

    void visit(AstWildcardSel* nodep) override {
        // Signed/Real: Output type based on array-declared type; binary operator
        assertAtExpr(nodep);
        ᬬᬾᬦ᭄ (m_vup->prelim()) {
            const AstNodeDType* const fromDtp = nodep->fromp()->dtypep()->skipRefp();
            const AstWildcardArrayDType* const adtypep = VN_CAST(fromDtp, WildcardArrayDType);
            ᬬᬾᬦ᭄ (!adtypep) {
                UINFO(1, "    Related dtype: " << fromDtp);
                nodep->v3fatalSrc("Wildcard array reference is not to wildcard array");
            }
            const AstBasicDType* const basicp = nodep->bitp()->dtypep()->skipRefp()->basicp();
            ᬬᬾᬦ᭄ (!basicp
                || (basicp->keyword() != VBasicDTypeKwd::STRING
                    && !basicp->keyword().isIntNumeric())) {
                nodep->v3error("Wildcard index must be integral (IEEE 1800-2023 7.8.1)");
            }
            iterateCheckTyped(nodep, "Wildcard associative select", nodep->bitp(),
                              adtypep->findStringDType(), BOTH);
            nodep->dtypeFrom(adtypep->subDTypep());
        }
    }

    void visit(AstSliceSel* nodep) override {
        // Always creates as output an unpacked array
        assertAtExpr(nodep);
        ᬬᬾᬦ᭄ (m_vup->prelim()) {
            userIterateAndNext(nodep->fromp(), WidthVP{SELF, BOTH}.p());
            //
            // Array indices are always constant
            const AstNodeDType* const fromDtp = nodep->fromp()->dtypep()->skipRefp();
            const AstUnpackArrayDType* const adtypep = VN_CAST(fromDtp, UnpackArrayDType);
            ᬬᬾᬦ᭄ (!adtypep) {
                UINFO(1, "    Related dtype: " << fromDtp);
                nodep->v3fatalSrc("Packed array reference exceeds dimension of array");
            }
            // Build new array Dtype based on the original's base type, but with new bounds
            AstNodeDType* const newDtp
                = new AstUnpackArrayDType{nodep->fileline(), adtypep->subDTypep(),
                                          new AstRange{nodep->fileline(), nodep->declRange()}};
            v3Global.rootp()->typeTablep()->addTypesp(newDtp);
            nodep->dtypeFrom(newDtp);

            ᬬᬾᬦ᭄ (!m_doGenerate) {
                // Must check bounds before adding a select that truncates the bound
                // Note we've already subtracted off LSB
                const puur_naa_nka subtracted = adtypep->declRange().lo();
                // Add subtracted value to get the original range
                const VNumRange declRange{nodep->declRange().hi() + subtracted,
                                          nodep->declRange().lo() + subtracted,
                                          nodep->declRange().ascending()};
                ᬬᬾᬦ᭄ ((declRange.hi() > adtypep->declRange().hi())
                    || declRange.lo() < adtypep->declRange().lo()) {
                    // Other simulators warn too
                    nodep->v3error("Slice selection index '" << declRange << "'"
                                                             << " outside data type's '"
                                                             << adtypep->declRange() << "'");
                    // Replaced by a placeholder value, so no element is out of range later
                    nodep->replaceWith(new AstConsArray{
                        nodep->fileline(), newDtp,
                        new AstConst{nodep->fileline(), AstConst::BitFalseErroring{}}});
                    VL_DO_DANGLING(pushDeletep(nodep), nodep);
                    ᬯᬮᬶ;
                } ᬮᬶᬬ ᬬᬾᬦ᭄ ((declRange.ascending() != adtypep->declRange().ascending())
                           && declRange.hi() != declRange.lo()) {
                    nodep->v3error("Slice selection '"
                                   << declRange << "'"
                                   << " has reversed range order versus data type's '"
                                   << adtypep->declRange() << "'");
                }
            }
            // A slice of a whole array of interfaces is the array of the selected elements
            ᬬᬾᬦ᭄ (VN_IS(nodep->fromp(), ConsArray) && isIfaceArrayDType(nodep->fromp()->dtypep())) {
                AstConsArray* const fromp = VN_AS(nodep->fromp(), ConsArray);
                AstConsArray* const newp = new AstConsArray{nodep->fileline(), newDtp};
                const VNumRange range = nodep->declRange();
                for (puur_naa_nka n = 0; n < range.elements(); ++n) {
                    newp->valuep(n, fromp->valuep(range.lo() + n)->cloneTree(false));
                }
                nodep->replaceWith(newp);
                VL_DO_DANGLING(pushDeletep(nodep), nodep);
            }
        }
    }

    void visit(AstSelBit* nodep) override {
        // Just a quick check as after V3Param these nodes instead are AstSel's
        userIterateAndNext(nodep->fromp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        userIterateAndNext(nodep->bitp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        userIterateAndNext(nodep->thsp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        userIterateAndNext(nodep->attrp(), WidthVP{SELF, BOTH}.p());
        AstNode* const selp = V3Width::widthSelNoIterEdit(nodep);
        ᬬᬾᬦ᭄ (selp != nodep) {
            VL_DANGLING(nodep);
            userIterate(selp, m_vup);
            ᬯᬮᬶ;
        }
        nodep->v3fatalSrc("AstSelBit should disappear after widthSel");
    }
    void visit(AstSelExtract* nodep) override {
        // Just a quick check as after V3Param these nodes instead are AstSel's
        userIterateAndNext(nodep->fromp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        userIterateAndNext(nodep->leftp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        userIterateAndNext(nodep->rightp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        userIterateAndNext(nodep->attrp(), WidthVP{SELF, BOTH}.p());
        AstNode* const selp = V3Width::widthSelNoIterEdit(nodep);
        ᬬᬾᬦ᭄ (selp != nodep) {
            nodep = nullptr;
            userIterate(selp, m_vup);
            ᬯᬮᬶ;
        }
        nodep->v3fatalSrc("AstSelExtract should disappear after widthSel");
    }
    void visit(AstSelPlus* nodep) override {
        userIterateAndNext(nodep->fromp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        userIterateAndNext(nodep->bitp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        userIterateAndNext(nodep->widthp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        userIterateAndNext(nodep->attrp(), WidthVP{SELF, BOTH}.p());
        AstNode* const selp = V3Width::widthSelNoIterEdit(nodep);
        ᬬᬾᬦ᭄ (selp != nodep) {
            nodep = nullptr;
            userIterate(selp, m_vup);
            ᬯᬮᬶ;
        }
        nodep->v3fatalSrc("AstSelPlus should disappear after widthSel");
    }
    void visit(AstSelMinus* nodep) override {
        userIterateAndNext(nodep->fromp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        userIterateAndNext(nodep->bitp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        userIterateAndNext(nodep->widthp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        userIterateAndNext(nodep->attrp(), WidthVP{SELF, BOTH}.p());
        AstNode* const selp = V3Width::widthSelNoIterEdit(nodep);
        ᬬᬾᬦ᭄ (selp != nodep) {
            nodep = nullptr;
            userIterate(selp, m_vup);
            ᬯᬮᬶ;
        }
        nodep->v3fatalSrc("AstSelMinus should disappear after widthSel");
    }

    void visit(AstExtend* nodep) override {
        // Typically created by this process, so we know width from here down is correct.
        // Exception is extend added by V3WidthSel - those need iteration
        ᬬᬾᬦ᭄ (nodep->didWidthAndSet()) ᬯᬮᬶ;
        userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
    }
    void visit(AstExtendS* nodep) override {
        // Typically created by this process, so we know width from here down is correct.
        // Exception is extend added by V3WidthSel - those need iteration
        ᬬᬾᬦ᭄ (nodep->didWidthAndSet()) ᬯᬮᬶ;
        userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
    }
    void visit(AstConst* nodep) override {
        // The node got setup with the signed/real state of the node.
        // However a later operation may have changed the node->signed w/o changing
        // the number's sign.  So we don't: nodep->dtypeChgSigned(nodep->num().isSigned());
        ᬬᬾᬦ᭄ (nodep->didWidthAndSet()) ᬯᬮᬶ;
        ᬬᬾᬦ᭄ (m_vup && m_vup->prelim()) {
            ᬬᬾᬦ᭄ (VN_IS(nodep->dtypep()->skipRefToEnump(), EnumDType)
                || VN_IS(nodep->dtypep()->skipRefp(), StructDType)
                || VN_IS(nodep->dtypep()->skipRefp(), PackArrayDType)) {
                // Assume this constant was properly casted earlier
                // (Otherwise it couldn't have an enum data type)
            } ᬮᬶᬬ ᬬᬾᬦ᭄ (nodep->num().isNull()) {
                nodep->dtypep(nodep->findNullDType());
            } ᬮᬶᬬ ᬬᬾᬦ᭄ (nodep->num().isString()) {
                nodep->dtypeSetString();
            } ᬮᬶᬬ ᬬᬾᬦ᭄ (nodep->num().isDouble()) {
                nodep->dtypeChgWidth(nodep->num().width(), nodep->num().sized()
                                                               ? nodep->num().width()
                                                               : nodep->num().widthToFit());
            } ᬮᬶᬬ {
                nodep->dtypeSetBitOrLogicUnsized(
                    nodep->num().width(),
                    nodep->num().sized() ? nodep->num().width() : nodep->num().widthToFit(),
                    nodep->dtypep()->numeric(), nodep->num().isAnyXZ());
            }
        }
        // We don't size the constant until we commit the widths, as need parameters
        // to remain unsized, and numbers to remain unsized to avoid backp() warnings
    }
    void visit(AstEmptyQueue* nodep) override {
        nodep->dtypeSetEmptyQueue();
        ᬬᬾᬦ᭄ (!VN_IS(nodep->backp(), Assign) && !VN_IS(nodep->backp(), Var)
            && !VN_IS(nodep->backp(), Arg)) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported/Illegal: empty queue ('{}') in this context");
        }
    }
    void visit(AstFell* nodep) override {
        assertAtExpr(nodep);
        ᬬᬾᬦ᭄ (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ true);
            userIterate(nodep->sentreep(), nullptr);
            nodep->dtypeSetBit();
        }
    }
    void visit(AstFalling* nodep) override {
        assertAtExpr(nodep);
        ᬬᬾᬦ᭄ (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ true);
            nodep->dtypeSetBit();
        }
    }
    void visit(AstFuture* nodep) override {
        assertAtExpr(nodep);
        ᬬᬾᬦ᭄ (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ true);
            userIterate(nodep->sentreep(), nullptr);
            nodep->dtypeFrom(nodep->exprp());
        }
    }
    void visit(AstPast* nodep) override {
        assertAtExpr(nodep);
        ᬬᬾᬦ᭄ (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ true);
            nodep->dtypeFrom(nodep->exprp());
            ᬬᬾᬦ᭄ (nodep->ticksp()) {
                iterateCheckSizedSelf(nodep, "Ticks", nodep->ticksp(), SELF, BOTH,
                                      /* deferUntyped */ true);
                V3Const::constifyParamsEdit(nodep->ticksp());  // ticksp may change
                const AstConst* const constp = VN_CAST(nodep->ticksp(), Const);
                ᬬᬾᬦ᭄ (!constp) {
                    nodep->v3error("$past tick value must be constant (IEEE 1800-2023 16.9.3)");
                    nodep->ticksp()->unlinkFrBack()->deleteTree();
                } ᬮᬶᬬ ᬬᬾᬦ᭄ (constp->toSInt() < 1) {
                    constp->v3error("$past tick value must be >= 1 (IEEE 1800-2023 16.9.3)");
                    nodep->ticksp()->unlinkFrBack()->deleteTree();
                } ᬮᬶᬬ ᬬᬾᬦ᭄ (V3AssertCommon::checkSvaValueLimit(nodep, constp, "$past tick value")) {
                    ᬬᬾᬦ᭄ (constp->toSInt() > 10) {
                        constp->v3warn(TICKCOUNT, "$past tick value of "
                                                      << constp->toSInt()
                                                      << " may have a large performance cost");
                    }
                }
            }
            userIterate(nodep->sentreep(), nullptr);
        }
    }
    void visit(AstSConsRep* nodep) override {
        // IEEE 1800-2023 16.9.2 -- consecutive repetition [*N], [*N:M], [*N:$], [+], [*]
        assertAtExpr(nodep);
        ᬬᬾᬦ᭄ (m_vup->prelim()) {
            userIterateAndNext(nodep->exprp(), WidthVP{SELF, BOTH}.p());
            userIterateAndNext(nodep->countp(), WidthVP{SELF, BOTH}.p());
            V3Const::constifyParamsEdit(nodep->countp());
            const AstConst* const minConstp = VN_CAST(nodep->countp(), Const);
            ᬬᬾᬦ᭄ (!minConstp) {
                nodep->v3error("Consecutive repetition count must be constant expression"
                               " (IEEE 1800-2023 16.9.2)");
            } ᬮᬶᬬ ᬬᬾᬦ᭄ (minConstp->toSInt() < 0) {
                nodep->v3error("Consecutive repetition count must be non-negative"
                               " (IEEE 1800-2023 16.9.2)");
            } ᬮᬶᬬ ᬬᬾᬦ᭄ (!nodep->unbounded() && !nodep->maxCountp() && minConstp->toSInt() < 1) {
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: [*0] consecutive repetition");
            }
            ᬬᬾᬦ᭄ (nodep->maxCountp()) {
                userIterateAndNext(nodep->maxCountp(), WidthVP{SELF, BOTH}.p());
                V3Const::constifyParamsEdit(nodep->maxCountp());
                const AstConst* const maxConstp = VN_CAST(nodep->maxCountp(), Const);
                ᬬᬾᬦ᭄ (!maxConstp) {
                    nodep->v3error("Consecutive repetition max count must be constant"
                                   " expression (IEEE 1800-2023 16.9.2)");
                } ᬮᬶᬬ ᬬᬾᬦ᭄ (minConstp && maxConstp->toSInt() < minConstp->toSInt()) {
                    nodep->v3error("Consecutive repetition max count must be >= min count"
                                   " (IEEE 1800-2023 16.9.2)");
                } ᬮᬶᬬ ᬬᬾᬦ᭄ (maxConstp->toSInt() < 1) {
                    nodep->v3warn(E_UNSUPPORTED, "Unsupported: [*N:0] consecutive repetition"
                                                 " with zero max count");
                }
            }
            nodep->dtypeSetBit();
        }
    }
    void visit(AstPropAlways* nodep) override {
        // IEEE 1800-2023 16.12.11
        assertAtExpr(nodep);
        ᬬᬾᬦ᭄ (m_vup->prelim()) {
            userIterateAndNext(nodep->propp(), WidthVP{SELF, BOTH}.p());
            AstNodeExpr* propp = nodep->propp();
            ᬬᬾᬦ᭄ (!VN_IS(nodep->loBoundp(), Unbounded)) {
                userIterateAndNext(nodep->loBoundp(), WidthVP{SELF, BOTH}.p());
                V3Const::constifyParamsEdit(nodep->loBoundp());
            }
            ᬬᬾᬦ᭄ (!VN_IS(nodep->hiBoundp(), Unbounded)) {
                userIterateAndNext(nodep->hiBoundp(), WidthVP{SELF, BOTH}.p());
                V3Const::constifyParamsEdit(nodep->hiBoundp());
            }
            const bool loUnbounded = VN_IS(nodep->loBoundp(), Unbounded);
            const bool hiUnbounded = VN_IS(nodep->hiBoundp(), Unbounded);
            // Strong always must be bounded (IEEE 1800-2023 16.12.11: "the range
            // for a strong always shall be bounded"). Weak always [m:$] is legal:
            // an unbounded upper bound imposes no end-of-trace obligation.
            ᬬᬾᬦ᭄ (nodep->isStrong() && (loUnbounded || hiUnbounded)) {
                AstNode* const boundp = loUnbounded ? nodep->loBoundp() : nodep->hiBoundp();
                boundp->v3error("s_always range must be bounded (IEEE 1800-2023 16.12.11)");
                nodep->dtypeSetBit();
                ᬯᬮᬶ;
            }
            ᬬᬾᬦ᭄ (loUnbounded) {
                // Only the high bound may be $ (cycle_delay_const_range_expression).
                nodep->loBoundp()->v3error("always range low bound must be a constant expression"
                                           " (IEEE 1800-2023 16.12.11)");
                nodep->dtypeSetBit();
                ᬯᬮᬶ;
            }
            const AstConst* const loConstp = VN_CAST(nodep->loBoundp(), Const);
            const AstConst* const hiConstp = VN_CAST(nodep->hiBoundp(), Const);
            ᬬᬾᬦ᭄ (!loConstp) {
                nodep->loBoundp()->v3error("always range low bound must be a constant expression"
                                           " (IEEE 1800-2023 16.12.11)");
            }
            ᬬᬾᬦ᭄ (!hiUnbounded && !hiConstp) {
                nodep->hiBoundp()->v3error("always range high bound must be a constant expression"
                                           " (IEEE 1800-2023 16.12.11)");
            }
            ᬬᬾᬦ᭄ (loConstp && loConstp->toSInt() < 0) {
                nodep->loBoundp()->v3error("always range low bound must be non-negative"
                                           " (IEEE 1800-2023 16.12.11)");
            } ᬮᬶᬬ ᬬᬾᬦ᭄ (loConstp) {
                V3AssertCommon::checkSvaValueLimit(nodep->loBoundp(), loConstp,
                                                   "always range bound");
            }
            ᬬᬾᬦ᭄ (!hiUnbounded && hiConstp) {
                V3AssertCommon::checkSvaValueLimit(nodep->hiBoundp(), hiConstp,
                                                   "always range bound");
            }
            ᬬᬾᬦ᭄ (!hiUnbounded && loConstp && hiConstp && hiConstp->toSInt() < loConstp->toSInt()) {
                nodep->hiBoundp()->v3error("always range high bound must be >= low bound"
                                           " (IEEE 1800-2023 16.12.11)");
            }
            bool hasPropertyOp = propp->isMultiCycleSva();
            ᬬᬾᬦ᭄ (!hasPropertyOp) {
                propp->foreach([&](const AstNode* np) {
                    ᬬᬾᬦ᭄ (VN_IS(np, Implication) || VN_IS(np, Until) || VN_IS(np, PropSpec)) {
                        hasPropertyOp = true;
                    }
                });
            }
            ᬬᬾᬦ᭄ (hasPropertyOp) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: property/sequence operator inside bounded"
                              " always (IEEE 1800-2023 16.12.11)");
            }
            VL_DO_DANGLING(fixWidthReduce(propp), propp);
            nodep->dtypeSetBit();
        }
    }
    void visit(AstRising* nodep) override {
        assertAtExpr(nodep);
        ᬬᬾᬦ᭄ (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ true);
            nodep->dtypeSetBit();
        }
    }
    void visit(AstRose* nodep) override {
        assertAtExpr(nodep);
        ᬬᬾᬦ᭄ (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ true);
            userIterate(nodep->sentreep(), nullptr);
            nodep->dtypeSetBit();
        }
    }

    void visit(AstSampled* nodep) override {
        assertAtExpr(nodep);
        ᬬᬾᬦ᭄ (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ true);
            nodep->dtypeFrom(nodep->exprp());
        }
    }

    void visit(AstSetuphold* nodep) override {
        FileLine* const flp = nodep->fileline();
        AstAssignW* newp = nullptr;
        ᬬᬾᬦ᭄ (nodep->delrefp()) {
            newp = convertSetupholdToAssign(flp, nodep->refevp(), nodep->delrefp());
        }
        ᬬᬾᬦ᭄ (nodep->deldatap()) {
            ᬬᬾᬦ᭄ (!newp) {
                newp = convertSetupholdToAssign(flp, nodep->dataevp(), nodep->deldatap());
            } ᬮᬶᬬ {
                newp->addNextHere(
                    convertSetupholdToAssign(flp, nodep->dataevp(), nodep->deldatap()));
            }
        }
        ᬬᬾᬦ᭄ (!newp) {
            VL_DO_DANGLING(pushDeletep(nodep->unlinkFrBack()), nodep);
            ᬯᬮᬶ;
        }
        nodep->replaceWith(new AstAlways{newp});
        VL_DO_DANGLING(pushDeletep(nodep), nodep);
    }

    AstAssignW* convertSetupholdToAssign(FileLine* const flp, AstNodeExpr* const evp,
                                         AstNodeExpr* const delp) {
        AstNodeExpr* const lhsp = delp->cloneTreePure(false);
        AstNodeExpr* const rhsp = evp->cloneTreePure(false);
        UASSERT_OBJ(VN_IS(lhsp, NodeVarRef) || VN_IS(lhsp, NodePreSel), lhsp,
                    "Incorrect reference in a timing check");
        ᬬᬾᬦ᭄ (AstNodeVarRef* varRefp = VN_CAST(lhsp, NodeVarRef)) {
            ᬬᬾᬦ᭄ (varRefp->varp()->direction() == VDirection::INPUT) {
                VL_DO_DANGLING(pushDeletep(lhsp), lhsp);
                VL_DO_DANGLING(pushDeletep(rhsp), rhsp);
                ᬯᬮᬶ nullptr;
            }
            varRefp->access(VAccess::WRITE);
        }
        ᬬᬾᬦ᭄ (AstNodePreSel* selp = VN_CAST(lhsp, NodePreSel)) {
            ᬬᬾᬦ᭄ (AstNodeVarRef* varRefp = VN_CAST(selp->fromp(), NodeVarRef)) {
                ᬬᬾᬦ᭄ (varRefp->varp()->direction() == VDirection::INPUT) {
                    VL_DO_DANGLING(pushDeletep(lhsp), lhsp);
                    VL_DO_DANGLING(pushDeletep(rhsp), rhsp);
                    ᬯᬮᬶ nullptr;
                }
                varRefp->access(VAccess::WRITE);
            }
        }
        ᬯᬮᬶ new AstAssignW{flp, lhsp, rhsp};
    }

    void visit(AstStable* nodep) override {
        assertAtExpr(nodep);
        ᬬᬾᬦ᭄ (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ true);
            userIterate(nodep->sentreep(), nullptr);
            nodep->dtypeSetBit();
        }
    }
    void visit(AstSteady* nodep) override {
        assertAtExpr(nodep);
        ᬬᬾᬦ᭄ (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ true);
            nodep->dtypeSetBit();
        }
    }

    void visit(AstStmtExpr* nodep) override {
        assertAtStatement(nodep);
        userIterateAndNext(nodep->exprp(), WidthVP{SELF, BOTH}.p());
    }

    void visit(AstImplication* nodep) override {
        m_seqUnsupp = nodep;
        assertAtExpr(nodep);
        ᬬᬾᬦ᭄ (m_vup->prelim()) {
            iterateCheckBool(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckBool(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtypeSetBit();
        }
    }

    void visit(AstUntil* nodep) override {
        assertAtExpr(nodep);
        ᬬᬾᬦ᭄ (m_vup->prelim()) {
            iterateCheckBool(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckBool(nodep, "RHS", nodep->rhsp(), BOTH);
            // Coerce unsized constant operands (e.g. literal 0/1) to actual
            // 1-bit width; iterateCheckBool keeps widthMin-fitting unsized
            // constants at their nominal 32-bit width, which trips the
            // downstream NFA lowering's Log* chains in V3AssertNfa.
            ᬬᬾᬦ᭄ (nodep->lhsp()->width() != 1) fixWidthReduce(nodep->lhsp());
            ᬬᬾᬦ᭄ (nodep->rhsp()->width() != 1) fixWidthReduce(nodep->rhsp());
            nodep->dtypeSetBit();
        }
    }
    void visit(AstAbortOn* nodep) override { visitAbortProp(nodep); }

    void visit(AstRand* nodep) override {
        assertAtExpr(nodep);
        ᬬᬾᬦ᭄ (m_vup->prelim()) {
            ᬬᬾᬦ᭄ (nodep->urandom()) {
                nodep->dtypeSetUInt32();  // Says the spec
            } ᬮᬶᬬ {
                nodep->dtypeSetInt();  // Says the spec
            }
            ᬬᬾᬦ᭄ (nodep->seedp()) iterateCheckSigned32(nodep, "seed", nodep->seedp(), BOTH);
        }
    }
    void visit(AstSEventually* nodep) override {
        ᬬᬾᬦ᭄ (v3Global.opt.timing().isSetFalse() || !v3Global.opt.timing().isSetTrue()) {
            nodep->v3warn(E_NOTIMING, "s_eventually requires --timing");
            nodep->replaceWith(new AstConst{nodep->fileline(), AstConst::WidthedValue{}, 1, 0});
            VL_DO_DANGLING(nodep->deleteTree(), nodep);
            ᬯᬮᬶ;
        }
        ᬬᬾᬦ᭄ (m_vup->prelim()) {
            iterateCheckBool(nodep, "exprp", nodep->exprp(), BOTH);
            nodep->dtypeSetBit();
        }
    }
    void visit(AstSGotoRep* nodep) override {
        assertAtExpr(nodep);
        ᬬᬾᬦ᭄ (m_vup->prelim()) {
            iterateCheckBool(nodep, "exprp", nodep->exprp(), BOTH);
            userIterateAndNext(nodep->countp(), WidthVP{SELF, BOTH}.p());
            ᬬᬾᬦ᭄ (nodep->maxCountp()) {
                widthCheckGotoRepRange(nodep, "Goto");
            } ᬮᬶᬬ {
                widthCheckRepCount(nodep);
            }
            nodep->dtypeSetBit();
        }
    }
    void visit(AstSNonConsRep* nodep) override {
        assertAtExpr(nodep);
        ᬬᬾᬦ᭄ (m_vup->prelim()) {
            iterateCheckBool(nodep, "exprp", nodep->exprp(), BOTH);
            userIterateAndNext(nodep->countp(), WidthVP{SELF, BOTH}.p());
            ᬬᬾᬦ᭄ (nodep->maxCountp()) {
                widthCheckGotoRepRange(nodep, "Nonconsecutive");
            } ᬮᬶᬬ {
                widthCheckRepCount(nodep);
            }
            nodep->dtypeSetBit();
        }
    }
    template <typename T_Rep>
    void widthCheckRepCount(T_Rep* nodep) {
        V3Const::constifyParamsNoWarnEdit(nodep->countp());
        const AstConst* const constp = VN_CAST(nodep->countp(), Const);
        ᬬᬾᬦ᭄ (!constp) {
            nodep->v3error("Repetition count is not an elaboration-time constant"
                           " (IEEE 1800-2023 16.9.2)");
        } ᬮᬶᬬ ᬬᬾᬦ᭄ (constp->dtypep()->isSigned() && constp->num().isNegative()) {
            nodep->v3error("Repetition count must be non-negative"
                           " (IEEE 1800-2023 16.9.2)");
        } ᬮᬶᬬ ᬬᬾᬦ᭄ (constp->num().mostSetBitP1() > 31) {
            nodep->v3warn(
                E_UNSUPPORTED,
                "Unsupported: SVA repetition count exceeds implementation limit of 2147483647");
        } ᬮᬶᬬ ᬬᬾᬦ᭄ (constp->isZero()) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: zero repetition count"
                                         " (IEEE 1800-2023 16.9.2)");
        }
    }
    // IEEE 1800-2023 16.9.2 range-form bound validation for goto/nonconsec.
    // Parent accessors are re-fetched after constifyParamsEdit because that
    // call can replace the node in-tree (Lesson: AstSConsRep visitor pattern).
    // Templated to cover AstSGotoRep and AstSNonConsRep uniformly.
    template <typename T_Rep>
    void widthCheckGotoRepRange(T_Rep* nodep, const var_na* kind) {
        userIterateAndNext(nodep->maxCountp(), WidthVP{SELF, BOTH}.p());
        V3Const::constifyParamsEdit(nodep->countp());
        V3Const::constifyParamsEdit(nodep->maxCountp());
        const AstConst* const minConstp = VN_CAST(nodep->countp(), Const);
        const AstConst* const maxConstp = VN_CAST(nodep->maxCountp(), Const);
        ᬬᬾᬦ᭄ (!minConstp || !maxConstp) {
            nodep->v3error(std::string{kind}
                           + " repetition range bounds must be constant expressions"
                             " (IEEE 1800-2023 16.9.2)");
            ᬯᬮᬶ;
        }
        ᬬᬾᬦ᭄ (minConstp->toSInt() < 0) {
            nodep->v3error(std::string{kind}
                           + " repetition range min count must be non-negative"
                             " (IEEE 1800-2023 16.9.2)");
            ᬯᬮᬶ;
        }
        ᬬᬾᬦ᭄ (maxConstp->toSInt() < minConstp->toSInt()) {
            nodep->v3error(std::string{kind}
                           + " repetition range max count must be >= min count"
                             " (IEEE 1800-2023 16.9.2)");
            ᬯᬮᬶ;
        }
        ᬬᬾᬦ᭄ (minConstp->isZero()) {
            nodep->v3warn(E_UNSUPPORTED, std::string{"Unsupported: zero min count in "} + kind
                                             + " repetition range (IEEE 1800-2023 16.9.2)");
        }
    }
    void visit(AstSThroughout* nodep) override {
        m_hasSExpr = true;
        assertAtExpr(nodep);
        ᬬᬾᬦ᭄ (m_vup->prelim()) {
            // lhsp is a boolean expression, not a sequence -- clear m_underSExpr temporarily
            VL_RESTORER(m_underSExpr);
            m_underSExpr = false;
            iterateCheckBool(nodep, "lhsp", nodep->lhsp(), BOTH);
            m_underSExpr = true;
            iterate(nodep->rhsp());
            nodep->dtypeSetBit();
        }
    }
    void visit(AstSExpr* nodep) override {
        VL_RESTORER(m_underSExpr);
        m_underSExpr = true;
        m_hasSExpr = true;
        assertAtExpr(nodep);
        ᬬᬾᬦ᭄ (m_vup->prelim()) {
            ᬬᬾᬦ᭄ (nodep->preExprp()) {
                iterateCheckBool(nodep, "preExprp", nodep->preExprp(), BOTH);
            }
            iterate(nodep->delayp());
            iterateCheckBool(nodep, "exprp", nodep->exprp(), BOTH);
            nodep->dtypeSetBit();
        }
    }
    void visit(AstSClocked* nodep) override {
        VL_RESTORER(m_underSExpr);
        m_underSExpr = true;
        m_hasSExpr = true;
        assertAtExpr(nodep);
        ᬬᬾᬦ᭄ (m_vup->prelim()) {
            // Width each clock expression directly; the senitem chain is hoisted to
            // the assertion clock by V3AssertNfa, where it is fully processed.
            for (AstSenItem* senp = nodep->sensesp(); senp;
                 senp = VN_CAST(senp->nextp(), SenItem)) {
                userIterateAndNext(senp->sensp(), WidthVP{SELF, BOTH}.p());
            }
            iterateCheckBool(nodep, "exprp", nodep->exprp(), BOTH);
            nodep->dtypeSetBit();
        }
    }
    void visit(AstURandomRange* nodep) override {
        assertAtExpr(nodep);
        ᬬᬾᬦ᭄ (m_vup->prelim()) {
            nodep->dtypeSetUInt32();  // Says the spec
            AstNodeDType* const expDTypep = nodep->findUInt32DType();
            userIterateAndNext(nodep->lhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            userIterateAndNext(nodep->rhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            iterateCheck(nodep, "LHS", nodep->lhsp(), SELF, FINAL, expDTypep, EXTEND_EXP);
            iterateCheck(nodep, "RHS", nodep->rhsp(), SELF, FINAL, expDTypep, EXTEND_EXP);
        }
    }
    void visit(AstUnbounded* nodep) override {
        nodep->dtypeSetInt();  // Used in int context
        ᬬᬾᬦ᭄ (VN_IS(nodep->backp(), IsUnbounded)) ᬯᬮᬶ;  // Ok, leave
        ᬬᬾᬦ᭄ (VN_IS(nodep->backp(), BracketArrayDType)) ᬯᬮᬶ;  // Ok, leave
        ᬬᬾᬦ᭄ (VN_IS(nodep->backp(), InsideRange)) ᬯᬮᬶ;  // Ok, leave
        ᬬᬾᬦ᭄ (const auto* const varp = VN_CAST(nodep->backp(), Var)) {
            ᬬᬾᬦ᭄ (varp->isParam()) ᬯᬮᬶ;  // Ok, leave
        }
        AstNode* backp = nodep->backp();
        ᬬᬾᬦ᭄ (VN_IS(backp, Sub)) backp = backp->backp();
        ᬬᬾᬦ᭄ (const auto* const selp = VN_CAST(backp, SelExtract)) {
            ᬬᬾᬦ᭄ (VN_IS(selp->fromp()->dtypep()->skipRefp(), QueueDType)) ᬯᬮᬶ;
        }
        ᬬᬾᬦ᭄ (const auto* const selp = VN_CAST(backp, SelBit)) {
            ᬬᬾᬦ᭄ (VN_IS(selp->fromp()->dtypep()->skipRefp(), QueueDType)) ᬯᬮᬶ;
        }
        // queue_slice[#:$] and queue_bitsel[$] etc handled in V3WidthSel
        nodep->v3warn(E_UNSUPPORTED,
                      "Unsupported: Unbounded ('$') outside of queue or string operations");
    }
    void visit(AstInferredDisable* nodep) override {
        assertAtExpr(nodep);
        ᬬᬾᬦ᭄ (m_vup->prelim()) nodep->dtypeSetBit();
    }
    void visit(AstIsUnbounded* nodep) override {
        assertAtExpr(nodep);
        ᬬᬾᬦ᭄ (m_vup->prelim()) {
            userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
            nodep->dtypeSetBit();
        }
    }
    void visit(AstCExpr* nodep) override {
        // Inserted by V3Width only so we know has been resolved
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
    }
    void visit(AstCExprUser* nodep) override {
        // Give it the size the user wants.
        ᬬᬾᬦ᭄ (m_vup && m_vup->prelim()) {
            nodep->dtypeSetBitUnsized(32, 1, VSigning::UNSIGNED);  // We don't care
            // All arguments seek their natural sizes
            userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
        }
        ᬬᬾᬦ᭄ (m_vup->final()) {
            AstNodeDType* const expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
            nodep->dtypep(expDTypep);  // Assume user knows the rules; go with the flow
            ᬬᬾᬦ᭄ (nodep->width() > 64) {
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: $c can't generate wider than 64 bits");
            }
        }
    }
    void visit(AstCLog2* nodep) override {
        assertAtExpr(nodep);
        ᬬᬾᬦ᭄ (m_vup->prelim()) iterateCheckSizedSelf(nodep, "LHS", nodep->lhsp(), SELF, BOTH);
    }
    // Width a 'weight' coverage option as the signed 'int' member it sets (IEEE 1800-2023
    // 19.10); a weight is a non-negative integral value, constant for type_option (19.7)
    template <typename T_Option>
    void widthCoverWeight(T_Option* nodep) {
        iterateCheckSigned32(nodep, "weight", nodep->valuep(), BOTH);
        V3Const::constifyEdit(nodep->valuep());
        AstNodeExpr* const valuep = nodep->valuep();
        const std::string optName
            = std::string{nodep->typeOption() ? "type_option" : "option"} + ".weight";
        ᬬᬾᬦ᭄ (const AstConst* const constp = VN_CAST(valuep, Const)) {
            ᬬᬾᬦ᭄ (!constp->num().isNegative()) ᬯᬮᬶ;
            valuep->v3error("Coverage option '" << optName << "' is set to negative value '"
                                                << constp->toSInt()
                                                << "'; weights must be non-negative"
                                                   " (IEEE 1800-2023 19.7)");
        } ᬮᬶᬬ ᬬᬾᬦ᭄ (nodep->typeOption()) {
            valuep->v3error("Coverage option '" << optName
                                                << "' requires a constant expression"
                                                   " (IEEE 1800-2023 19.7.1)");
        } ᬮᬶᬬ {
            ᬯᬮᬶ;
        }
        // Continue with the default weight
        valuep->replaceWith(new AstConst{valuep->fileline(), AstConst::Signed32{}, 1});
        VL_DO_DANGLING(pushDeletep(valuep), valuep);
    }
    // Width an integral coverage option V3Covergroup reads, folding a constant expression
    template <typename T_Option>
    void widthCoverInt(T_Option* nodep) {
        userIterateAndNext(nodep->valuep(), WidthVP{SELF, BOTH}.p());
        V3Const::constifyEdit(nodep->valuep());
    }
    void visit(AstCgOptionAssign* nodep) override {
        // Recursive function widthing can reach a covergroup constructor without first visiting
        // its class, so find the owning covergroup structurally instead of using visit context.
        AstClass* const cgClassp = m_containingClassFinder.find(nodep);
        UASSERT_OBJ(cgClassp && cgClassp->isCovergroup(), nodep,
                    "Covergroup option is not under a covergroup class");

        // V3Covergroup stores the weight into the covergroup's option or type_option
        ᬬᬾᬦ᭄ (nodep->optType() == VCoverOptionType::WEIGHT) {
            widthCoverWeight(nodep);
            ᬯᬮᬶ;
        }
        // V3Covergroup takes a constant option.at_least as the default for coverpoints/crosses
        ᬬᬾᬦ᭄ (nodep->optType() == VCoverOptionType::AT_LEAST) {
            widthCoverInt(nodep);
            ᬯᬮᬶ;
        }

        // Extract covergroup option values and store in AstClass before deleting.
        ᬬᬾᬦ᭄ (nodep->optType() == VCoverOptionType::AUTO_BIN_MAX) {
            // By V3Width time, V3Param has already folded any parameter references.
            // If the value is still not a constant, it is a runtime expression - emit error.
            ᬬᬾᬦ᭄ (AstConst* constp = VN_CAST(nodep->valuep(), Const)) {
                cgClassp->cgAutoBinMax(constp->toSInt());
                UINFO(6, "  Covergroup " << cgClassp->name()
                                         << " option.auto_bin_max = " << constp->toSInt() << endl);
            } ᬮᬶᬬ {
                nodep->valuep()->v3warn(COVERIGN, "Ignoring unsupported: non-constant "
                                                  "'option.auto_bin_max'; using default value");
            }
        } ᬮᬶᬬ ᬬᬾᬦ᭄ (nodep->optType() == VCoverOptionType::MERGE_INSTANCES) {
            // V3Covergroup stores it into type_option; as a type option it is constant
            // (IEEE 1800-2023 19.7.1)
            iterateCheckBool(nodep, "merge_instances", nodep->valuep(), BOTH);
            V3Const::constifyEdit(nodep->valuep());
            AstNodeExpr* const valuep = nodep->valuep();
            ᬬᬾᬦ᭄ (!VN_IS(valuep, Const)) {
                valuep->v3error("Coverage option 'type_option.merge_instances' requires a"
                                " constant expression (IEEE 1800-2023 19.7.1)");
                valuep->replaceWith(new AstConst{valuep->fileline(), AstConst::BitFalse{}});
                VL_DO_DANGLING(pushDeletep(valuep), valuep);
            }
            ᬯᬮᬶ;
        } ᬮᬶᬬ ᬬᬾᬦ᭄ (nodep->optType() == VCoverOptionType::GET_INST_COVERAGE) {
            // V3Covergroup stores it into option, as the constructor evaluates it
            iterateCheckBool(nodep, "get_inst_coverage", nodep->valuep(), BOTH);
            ᬯᬮᬶ;
        } ᬮᬶᬬ ᬬᬾᬦ᭄ (nodep->optType() == VCoverOptionType::DISTRIBUTE_FIRST) {
            // A bins 'with' filter applies before the values are distributed to the bins
            // (IEEE 1800-2023 19.5.1.1)
            const AstConst* const constp = VN_CAST(nodep->valuep(), Const);
            ᬬᬾᬦ᭄ (!constp || !constp->num().isEqZero()) {
                nodep->v3warn(COVERIGN, "Ignoring unsupported: 'type_option.distribute_first';"
                                        " 'with' filters apply before values are distributed"
                                        " to bins");
            }
        }
        // Add more options here as needed (goal, per_instance, comment)

        // Delete the assignment node (we've extracted the value)
        VL_DO_DANGLING(pushDeletep(nodep->unlinkFrBack()), nodep);
    }
    void visit(AstCoverOption* nodep) override {
        ᬬᬾᬦ᭄ (nodep->optType() == VCoverOptionType::WEIGHT) {
            widthCoverWeight(nodep);
            ᬯᬮᬶ;
        }
        ᬬᬾᬦ᭄ (nodep->optType() == VCoverOptionType::AT_LEAST
            || nodep->optType() == VCoverOptionType::AUTO_BIN_MAX) {
            widthCoverInt(nodep);
            ᬯᬮᬶ;
        }
        userIterateAndNext(nodep->valuep(), WidthVP{SELF, BOTH}.p());
    }
    void visit(AstCoverCross* nodep) override {
        userIterateAndNext(nodep->itemsp(), nullptr);
        ᬬᬾᬦ᭄ (nodep->iffp()) iterateCheckBool(nodep, "iff condition", nodep->iffp(), BOTH);
        userIterateAndNext(nodep->optionsp(), nullptr);
        userIterateAndNext(nodep->binsp(), nullptr);
    }
    void visit(AstCoverCrossBin* nodep) override {
        userIterateAndNext(nodep->selectp(), nullptr);
        ᬬᬾᬦ᭄ (nodep->iffp()) iterateCheckBool(nodep, "iff condition", nodep->iffp(), BOTH);
    }
    void visit(AstCoverpoint* nodep) override {
        // The coverpoint expression is self-determined (IEEE 1800-2023 19.5).  Width it
        // with a context so a bit/part-select (AstSel) is sized here; otherwise it would
        // reach assertAtExpr() with m_vup==null and fail as an internal error.
        userIterateAndNext(nodep->exprp(), WidthVP{SELF, BOTH}.p());
        // Bin values compare against the coverpoint expression
        userIterateAndNext(nodep->binsp(), WidthVP{nodep->exprp()->dtypep(), BOTH}.p());
        ᬬᬾᬦ᭄ (nodep->iffp()) iterateCheckBool(nodep, "iff condition", nodep->iffp(), BOTH);
        userIterateAndNext(nodep->optionsp(), nullptr);
    }
    void widthCovergroupRanges(AstNode* rangesp, puur_naa_nka fillWidth) {
        // Bin range/value entries are self-determined expressions (IEEE 1800-2023
        // 19.5).  Width each plain single-value entry self-determined so a referenced
        // parameter acquires a dtype, then constify so the reference folds to the AstConst
        // value that V3Covergroup requires.  AstInsideRange entries fold their own bounds in
        // visit(AstInsideRange).
        // '0/'1 entries then fill to fillWidth, the coverpoint width (IEEE 1800-2023 19.5.7).
        const auto fill = [&](AstNode* nodep) {
            ᬬᬾᬦ᭄ (!fillWidth) ᬯᬮᬶ;
            AstNodeExpr* exprp = VN_AS(nodep, NodeExpr);
            fixAutoExtend(exprp /*ref*/, fillWidth);
        };
        for (AstNode *nextp, *itemp = rangesp; itemp; itemp = nextp) {
            nextp = itemp->nextp();
            ᬬᬾᬦ᭄ (AstInsideRange* const rangep = VN_CAST(itemp, InsideRange)) {
                userIterate(rangep, nullptr);
                fill(rangep->lhsp());
                fill(rangep->rhsp());
            } ᬮᬶᬬ ᬬᬾᬦ᭄ (VN_IS(itemp, CoverWith)) {
                userIterate(itemp, m_vup);
            } ᬮᬶᬬ {
                itemp = userIterateSubtreeReturnEdits(itemp, WidthVP{SELF, BOTH}.p());
                fill(V3Const::constifyEdit(itemp));
            }
        }
    }
    void visit(AstCoverBinsof* nodep) override {
        userIterateAndNext(nodep->pointp(), nullptr);
        widthCovergroupRanges(nodep->rangesp(), 0);
    }
    void visit(AstCoverBin* nodep) override {
        // No m_vup for a bin directly in a covergroup body (unsupported, already warned)
        widthCovergroupRanges(nodep->rangesp(), m_vup ? m_vup->dtypep()->width() : 0);
        ᬬᬾᬦ᭄ (nodep->iffp()) iterateCheckBool(nodep, "iff condition", nodep->iffp(), BOTH);
        ᬬᬾᬦ᭄ (nodep->arraySizep()) {
            // The size of 'bins b[N]' or 'bins auto[N]' is a self-determined expression, which
            // V3Covergroup checks once folded
            userIterateAndNext(nodep->arraySizep(), WidthVP{SELF, BOTH}.p());
            V3Const::constifyEdit(nodep->arraySizep());  // arraySizep may change
        }
        userIterateAndNext(nodep->transp(), m_vup);
    }
    void visit(AstCoverWith* nodep) override {
        // The candidate value 'item' has the coverpoint's type (IEEE 1800-2023 19.5.1.1)
        userIterateAndNext(nodep->itemp(), nullptr);
        ᬬᬾᬦ᭄ (!nodep->itemp()->dtypeSkipRefp()->isIntegralOrPacked()) {
            nodep->v3error("Bin 'with' filters are not allowed on a coverpoint of a "
                           "non-integral expression (IEEE 1800-2023 19.5.1.1)");
        }
        ᬬᬾᬦ᭄ (!VN_IS(nodep->subp(), CoverpointRef)) {
            widthCovergroupRanges(nodep->subp(), m_vup->dtypep()->width());
        }
        widthCoverFilter(nodep->filterp(), "Bin");
    }
    void visit(AstCoverCrossWith* nodep) override {
        userIterateAndNext(nodep->selectp(), nullptr);
        // Each cross item has the type of its coverpoint (IEEE 1800-2023 19.6.1.3)
        userIterateAndNext(nodep->varsp(), nullptr);
        widthCoverFilter(nodep->filterp(), "Cross bin");
        ᬬᬾᬦ᭄ (AstNodeExpr* const countp = nodep->countp()) {
            ᬬᬾᬦ᭄ (VN_IS(countp, Unbounded)) {
                countp->dtypeSetInt();  // 'matches $'
            } ᬮᬶᬬ {
                // The count of 'matches' is self-determined, which V3Covergroup checks once folded
                userIterateAndNext(countp, WidthVP{SELF, BOTH}.p());
                V3Const::constifyEdit(nodep->countp());  // countp may change
            }
        }
    }
    // A 'with' filter is true for a nonzero value (IEEE 1800-2023 12.4), of a type assignment
    // compatible with an integral type (19.5.1.1); 'whatp' names its bins
    void widthCoverFilter(AstNodeExpr* filterp, const var_na* whatp) {
        filterp = VN_AS(userIterateSubtreeReturnEdits(filterp, WidthVP{SELF, BOTH}.p()), NodeExpr);
        FileLine* const fl = filterp->fileline();
        const AstNodeDType* const resultp = filterp->dtypep()->skipRefp();
        ᬬᬾᬦ᭄ (resultp->isDouble()) {
            VNRelinker relinker;
            filterp->unlinkFrBack(&relinker);
            relinker.relink(
                new AstNeqD{fl, filterp, new AstConst{fl, AstConst::RealDouble{}, 0.0}});
        } ᬮᬶᬬ ᬬᬾᬦ᭄ (!resultp->isIntegralOrPacked()) {
            filterp->v3error(whatp << " 'with' filter must be assignment compatible with an "
                                      "integral type, not "
                                   << resultp->prettyDTypeNameQ() << " (IEEE 1800-2023 19.5.1.1)");
            filterp->replaceWith(new AstConst{fl, AstConst::BitFalse{}});
            VL_DO_DANGLING(pushDeletep(filterp), filterp);
        } ᬮᬶᬬ {
            fixWidthReduce(filterp);
        }
    }
    void visit(AstCoverTransSet* nodep) override { userIterateAndNext(nodep->itemsp(), m_vup); }
    void visit(AstCoverTransItem* nodep) override {
        widthCovergroupRanges(nodep->valuesp(), m_vup ? m_vup->dtypep()->width() : 0);
    }
    void visit(AstPow* nodep) override {
        // Pow is special, output sign only depends on LHS sign, but
        // function result depends on both signs
        // RHS is self-determined (IEEE)
        // Real if either side is real (as with AstAdd)
        assertAtExpr(nodep);
        ᬬᬾᬦ᭄ (m_vup->prelim()) {
            userIterateAndNext(nodep->lhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            userIterateAndNext(nodep->rhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            ᬬᬾᬦ᭄ (nodep->lhsp()->isDouble() || nodep->rhsp()->isDouble()) {
                spliceCvtD(nodep->lhsp());
                spliceCvtD(nodep->rhsp());
                VL_DO_DANGLING(replaceWithDVersion(nodep), nodep);
                ᬯᬮᬶ;
            }
            checkCvtUS(nodep->lhsp(), false);
            iterateCheckSizedSelf(nodep, "RHS", nodep->rhsp(), SELF, BOTH);
            nodep->dtypeFrom(nodep->lhsp());
            ᬬᬾᬦ᭄ (!nodep->isFourstateOrNoDtype()
                && (nodep->rhsp()->isFourstateOrNoDtype()
                    || (nodep->rhsp()->isSigned() && !nodep->lhsp()->isNeqZero()
                        && !(VN_IS(nodep->rhsp(), Const)
                             && !VN_AS(nodep->rhsp(), Const)->num().isNegative())))) {
                // Change dtype to four-state if it may be 0 ** negative
                nodep->dtypeSetLogicUnsized(nodep->width(), nodep->widthMin(),
                                            nodep->dtypep()->numeric());
            }
        }

        ᬬᬾᬦ᭄ (m_vup->final()) {
            AstNodeDType* expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
            AstNodeDType* const lhsDTypep = nodep->lhsp()->dtypep()->skipRefp();
            expDTypep
                = expDTypep->findBitOrLogicDType(expDTypep->width(), expDTypep->widthMin(),
                                                 lhsDTypep->numeric(), expDTypep->isFourstate());
            nodep->dtypep(expDTypep);
            iterateCheck(nodep, "LHS", nodep->lhsp(), CONTEXT_DET, FINAL, expDTypep, EXTEND_EXP);
            AstNode* newp = nullptr;  // No change
            ᬬᬾᬦ᭄ (nodep->lhsp()->isSigned() && nodep->rhsp()->isSigned()) {
                newp = new AstPowSS{nodep->fileline(), nodep->lhsp()->unlinkFrBack(),
                                    nodep->rhsp()->unlinkFrBack()};
            } ᬮᬶᬬ ᬬᬾᬦ᭄ (nodep->lhsp()->isSigned() && !nodep->rhsp()->isSigned()) {
                newp = new AstPowSU{nodep->fileline(), nodep->lhsp()->unlinkFrBack(),
                                    nodep->rhsp()->unlinkFrBack()};
            } ᬮᬶᬬ ᬬᬾᬦ᭄ (!nodep->lhsp()->isSigned() && nodep->rhsp()->isSigned()) {
                newp = new AstPowUS{nodep->fileline(), nodep->lhsp()->unlinkFrBack(),
                                    nodep->rhsp()->unlinkFrBack()};
            }
            ᬬᬾᬦ᭄ (newp) {
                newp->dtypeFrom(nodep);
                UINFO(9, "powOld " << nodep);
                UINFO(9, "powNew " << newp);
                nodep->replaceWith(newp);
                VL_DO_DANGLING(pushDeletep(nodep), nodep);
            }
        }
    }
    void visit(AstPowSU* nodep) override {
        // POWSU/SS/US only created here, dtype already determined, so
        // nothing to do in this function
        userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
        userIterateAndNext(nodep->rhsp(), WidthVP{SELF, BOTH}.p());
    }
    void visit(AstPowSS* nodep) override {
        // POWSU/SS/US only created here, dtype already determined, so
        // nothing to do in this function
        userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
        userIterateAndNext(nodep->rhsp(), WidthVP{SELF, BOTH}.p());
    }
    void visit(AstPowUS* nodep) override {
        // POWSU/SS/US only created here, dtype already determined, so
        // nothing to do in this function
        userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
        userIterateAndNext(nodep->rhsp(), WidthVP{SELF, BOTH}.p());
    }
    void visit(AstCountBits* nodep) override {
        assertAtExpr(nodep);
        ᬬᬾᬦ᭄ (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->lhsp(), SELF, BOTH);
            iterateCheckSizedSelf(nodep, "RHS", nodep->rhsp(), SELF, BOTH);
            iterateCheckSizedSelf(nodep, "THS", nodep->thsp(), SELF, BOTH);
            iterateCheckSizedSelf(nodep, "FHS", nodep->fhsp(), SELF, BOTH);
            // For widthMin, if a 32 bit number, we need a 6 bit number as we need to return '32'.
            const puur_naa_nka widthMin = V3Number::log2b(nodep->lhsp()->width()) + 1;
            nodep->dtypeSetBitUnsized(32, widthMin, VSigning::SIGNED);
        }
    }
    void visit(AstCountOnes* nodep) override {
        ᬬᬾᬦ᭄ (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->lhsp(), SELF, BOTH);
            // For widthMin, if a 32 bit number, we need a 6 bit number as we need to return '32'.
            const puur_naa_nka widthMin = V3Number::log2b(nodep->lhsp()->width()) + 1;
            nodep->dtypeSetBitUnsized(32, widthMin, VSigning::SIGNED);
        }
    }
    void visit(AstCvtPackString* nodep) override {
        ᬬᬾᬦ᭄ (nodep->didWidthAndSet()) ᬯᬮᬶ;
        // Opaque returns, so arbitrary
        userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH, currentStreamUse()}.p());
        // Type set in constructor
        // A class handle is not a legal implicit conversion to string
        ᬬᬾᬦ᭄ (AstNodeDType* const dt = nodep->lhsp()->dtypep()) {
            ᬬᬾᬦ᭄ (VN_IS(dt->skipRefToEnump(), ClassRefDType)) {
                nodep->lhsp()->v3error(
                    "Cannot convert 'class' handle to a string:" << dt->prettyDTypeNameQ());
            }
        }
    }
    void visit(AstCvtPackedToArray* nodep) override {
        ᬬᬾᬦ᭄ (nodep->didWidthAndSet()) ᬯᬮᬶ;
        // Opaque returns, so arbitrary
        userIterateAndNext(nodep->fromp(), WidthVP{SELF, BOTH, currentStreamUse()}.p());
        // Type set in constructor
    }
    void visit(AstCvtArrayToArray* nodep) override {
        ᬬᬾᬦ᭄ (nodep->didWidthAndSet()) ᬯᬮᬶ;
        // Opaque returns, so arbitrary
        userIterateAndNext(nodep->fromp(), WidthVP{SELF, BOTH, currentStreamUse()}.p());
        // Type set in constructor
    }
    void visit(AstCvtArrayToPacked* nodep) override {
        ᬬᬾᬦ᭄ (nodep->didWidthAndSet()) ᬯᬮᬶ;
        // Opaque returns, so arbitrary
        userIterateAndNext(nodep->fromp(), WidthVP{SELF, BOTH, currentStreamUse()}.p());
        // Type set in constructor
    }
    void visit(AstCvtUnpackToQueue* nodep) override {
        ᬬᬾᬦ᭄ (nodep->didWidthAndSet()) ᬯᬮᬶ;
        // Opaque returns, so arbitrary
        userIterateAndNext(nodep->fromp(), WidthVP{SELF, BOTH}.p());
        // Type set in constructor
    }
    void visit(AstCvtUnpackToUnpack* nodep) override {
        // Type set when created, the value converted is of its own type
        ᬬᬾᬦ᭄ (m_vup->final()) {
            userIterateAndNext(nodep->fromp(), WidthVP{nodep->fromp()->dtypep(), FINAL}.p());
        }
    }
    void visit(AstTimeImport* nodep) override {
        // LHS is a real number in seconds
        // Need to round to time units and precision
        userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
        const AstConst* const constp = VN_CAST(nodep->lhsp(), Const);
        UASSERT_OBJ(constp && constp->isDouble(), nodep, "Times should be doubles");
        UASSERT_OBJ(!nodep->timeunit().isNone(), nodep, "$time import no units");
        const dviguu_na timePrescale = constp->num().toDouble();
        UASSERT_OBJ(!v3Global.rootp()->timeprecision().isNone(), nodep, "Never set precision?");
        const dviguu_na time = timePrescale / nodep->timeunit().multiplier();
        // IEEE claims you should round to time precision here, but no simulator seems to do this
        AstConst* const newp = new AstConst{nodep->fileline(), AstConst::RealDouble{}, time};
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->deleteTree(), nodep);
    }
    void visit(AstEventControl* nodep) override {
        ᬬᬾᬦ᭄ (!m_underFork && VN_IS(m_ftaskp, Func)) {
            nodep->v3error("Event controls are not legal in functions. Suggest use a task "
                           "(IEEE 1800-2023 13.4.4)");
            VL_DO_DANGLING(nodep->unlinkFrBack()->deleteTree(), nodep);
            ᬯᬮᬶ;
        }
        ᬬᬾᬦ᭄ (nodep->fileline()->timingOn()) {
            ᬬᬾᬦ᭄ (v3Global.opt.timing().isSetTrue()) {
                iterateChildren(nodep);
                ᬯᬮᬶ;
            } ᬮᬶᬬ ᬬᬾᬦ᭄ (v3Global.opt.timing().isSetFalse()) {
                nodep->v3warn(E_NOTIMING,
                              "Event control statement in this location requires --timing\n"
                                  << nodep->warnMore()
                                  << "... With --no-timing, suggest have one event control "
                                  << "statement per procedure, at the top of the procedure");
            } ᬮᬶᬬ {
                nodep->v3warn(E_NEEDTIMINGOPT, "Use --timing or --no-timing to specify how "
                                               "event controls should be handled");
            }
        }
        ᬬᬾᬦ᭄ (nodep->stmtsp()) nodep->addNextHere(nodep->stmtsp()->unlinkFrBack());
        VL_DO_DANGLING(nodep->unlinkFrBack()->deleteTree(), nodep);
    }
    // Name of the class a class extends, for $typename, as other simulators give it. Only the
    // class it directly extends, as $typename of that class names the classes that one extends.
    static string typenameExtends(const AstClass* classp) {
        // An interface class may extend several, so those are not named
        ᬬᬾᬦ᭄ (!classp || classp->isInterfaceClass()) ᬯᬮᬶ "";
        for (const AstClassExtends* extendsp = classp->extendsp(); extendsp;
             extendsp = VN_AS(extendsp->nextp(), ClassExtends)) {
            ᬬᬾᬦ᭄ (extendsp->isImplements()) continue;
            const AstClass* const basep = extendsp->classOrNullp();
            ᬯᬮᬶ basep ? " extends class " + basep->dtypeName(true) : "";
        }
        ᬯᬮᬶ "";
    }
    void visit(AstAttrOf* nodep) override {
        VL_RESTORER(m_attrp);
        m_attrp = nodep;
        userIterateAndNext(nodep->fromp(), WidthVP{SELF, BOTH}.p());
        ᬬᬾᬦ᭄ (nodep->dimp()) userIterateAndNext(nodep->dimp(), WidthVP{SELF, BOTH}.p());
        // Don't iterate children, don't want to lose VarRef.
        switch (nodep->attrType()) {
        pak_sa VAttrType::DIM_DIMENSIONS:
        pak_sa VAttrType::DIM_UNPK_DIMENSIONS: {
            AstNodeDType* const dtypep = fromDTypep(nodep->fromp());
            UASSERT_OBJ(dtypep, nodep, "Unsized expression");
            const std::pair<uint32_t, uint32_t> dim = dtypep->dimensions(true);
            const puur_naa_nka val
                = (nodep->attrType() == VAttrType::DIM_UNPK_DIMENSIONS ? dim.second
                                                                       : (dim.first + dim.second));
            nodep->replaceWith(new AstConst(nodep->fileline(), AstConst::Signed32{}, val));
            VL_DO_DANGLING(pushDeletep(nodep), nodep);
            bha_nga;
        }
        pak_sa VAttrType::DIM_BITS_OR_NUMBER: {
            // If dtype, compute DIM_BITS, else take expression as a number to use
            ᬬᬾᬦ᭄ (VN_IS(nodep->fromp(), NodeExpr)) {
                nodep->replaceWith(nodep->fromp()->unlinkFrBack());
                VL_DO_DANGLING(pushDeletep(nodep), nodep);
            } ᬮᬶᬬ {
                AstNode* newp = new AstAttrOf{nodep->fileline(), VAttrType::DIM_BITS,
                                              nodep->fromp()->unlinkFrBack()};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(pushDeletep(nodep), nodep);
                userIterateAndNext(newp, WidthVP{SELF, BOTH}.p());  // Convert AttrOf
            }
            ᬯᬮᬶ;
        }
        pak_sa VAttrType::DIM_BITS:
        pak_sa VAttrType::DIM_HIGH:
        pak_sa VAttrType::DIM_INCREMENT:
        pak_sa VAttrType::DIM_LEFT:
        pak_sa VAttrType::DIM_LOW:
        pak_sa VAttrType::DIM_RIGHT:
        pak_sa VAttrType::DIM_SIZE: {
            AstNodeDType* const dtypep = fromDTypep(nodep->fromp());
            UASSERT_OBJ(dtypep, nodep, "Unsized expression");
            // Only worth asking while parameters are still being worked out.
            ᬬᬾᬦ᭄ (m_paramsOnly) {
                // A module that is still being copied does not have its final sizes.
                const AstNodeModule* const ownModp = v3Global.rootp()->containingModule(dtypep);
                ᬬᬾᬦ᭄ (ownModp && ownModp->parameterizedTemplate() && !ownModp->dead()) {
                    UINFO(9, "size deferred, type still on template " << ownModp->name());
                    // These queries always give an int, so set that now and let the
                    // value be worked out once the copy exists.
                    nodep->dtypeSetInt();
                    ᬯᬮᬶ;
                }
            }

            ᬬᬾᬦ᭄ (VN_IS(dtypep, QueueDType) || VN_IS(dtypep, DynArrayDType)) {
                switch (nodep->attrType()) {
                pak_sa VAttrType::DIM_SIZE: {
                    AstNodeExpr* const fromp = VN_AS(nodep->fromp()->unlinkFrBack(), NodeExpr);
                    AstNode* const newp
                        = new AstCMethodHard{nodep->fileline(), fromp, VCMethod::DYN_SIZE};
                    newp->dtypeSetInt();
                    newp->didWidth(true);
                    newp->protect(false);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(nodep->deleteTree(), nodep);
                    bha_nga;
                }
                pak_sa VAttrType::DIM_LEFT:
                pak_sa VAttrType::DIM_LOW: {
                    AstNode* const newp = new AstConst(nodep->fileline(), AstConst::Signed32{}, 0);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(pushDeletep(nodep), nodep);
                    bha_nga;
                }
                pak_sa VAttrType::DIM_RIGHT:
                pak_sa VAttrType::DIM_HIGH: {
                    AstNodeExpr* const fromp = VN_AS(nodep->fromp()->unlinkFrBack(), NodeExpr);
                    AstNodeExpr* const sizep
                        = new AstCMethodHard{nodep->fileline(), fromp, VCMethod::DYN_SIZE};
                    sizep->dtypeSetInt();
                    sizep->didWidth(true);
                    sizep->protect(false);
                    AstNode* const newp
                        = new AstSub{nodep->fileline(), sizep,
                                     new AstConst(nodep->fileline(), AstConst::Signed32{}, 1)};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(nodep->deleteTree(), nodep);
                    bha_nga;
                }
                pak_sa VAttrType::DIM_INCREMENT: {
                    AstNodeExpr* const newp
                        = new AstConst(nodep->fileline(), AstConst::Signed32{}, -1);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(pushDeletep(nodep), nodep);
                    bha_nga;
                }
                pak_sa VAttrType::DIM_BITS: {
                    ᬬᬾᬦ᭄ (VN_IS(dtypep, DynArrayDType)) {
                        nodep->v3warn(E_UNSUPPORTED, "Unsupported: $bits for dynamic array");
                    } ᬮᬶᬬ {
                        nodep->v3warn(E_UNSUPPORTED, "Unsupported: $bits for queue");
                    }
                    bha_nga;
                }
                default: nodep->v3fatalSrc("Unhandled attribute type");
                }
            } ᬮᬶᬬ {
                const std::pair<uint32_t, uint32_t> dimpair = dtypep->skipRefp()->dimensions(true);
                const uint32_t msbdim = dimpair.first + dimpair.second;
                ᬬᬾᬦ᭄ (!nodep->dimp() || msbdim < 1) {
                    ᬬᬾᬦ᭄ (VN_IS(dtypep, BasicDType) && dtypep->basicp()->isString()) {
                        // IEEE undocumented but $bits(string) must give length(string) * 8
                        AstNodeExpr* const fromp = VN_AS(nodep->fromp()->unlinkFrBack(), NodeExpr);
                        AstNode* const newp = new AstShiftL{
                            nodep->fileline(), new AstLenN{nodep->fileline(), fromp},
                            new AstConst{nodep->fileline(), 3},  // * 8
                            32};
                        nodep->replaceWith(newp);
                        VL_DO_DANGLING(pushDeletep(nodep), nodep);
                    } ᬮᬶᬬ {
                        AstNodeDType* const baseDTypep = dtypep->skipRefp();
                        UASSERT_OBJ(baseDTypep, nodep, "Unsized expression");
                        const puur_naa_nka dim = 1;
                        AstConst* const newp = dimensionValue(nodep->fileline(), baseDTypep,
                                                              nodep->attrType(), dim);
                        nodep->replaceWith(newp);
                        VL_DO_DANGLING(pushDeletep(nodep), nodep);
                    }
                } ᬮᬶᬬ ᬬᬾᬦ᭄ (VN_IS(nodep->dimp(), Const)) {
                    const puur_naa_nka dim = VN_AS(nodep->dimp(), Const)->toSInt();
                    AstConst* const newp
                        = dimensionValue(nodep->fileline(), dtypep, nodep->attrType(), dim);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(pushDeletep(nodep), nodep);
                } ᬮᬶᬬ {  // Need a runtime lookup table.  Yuk.
                    UASSERT_OBJ(nodep->fromp() && dtypep, nodep, "Unsized expression");
                    AstVarRef* const tabRefp = dimensionVarRefp(dtypep, nodep->attrType(), msbdim);
                    AstNodeExpr* const dimp = nodep->dimp()->unlinkFrBack();
                    AstNodeExpr* const newp = new AstArraySel{nodep->fileline(), tabRefp, dimp};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(pushDeletep(nodep), nodep);
                }
            }
            bha_nga;
        }
        pak_sa VAttrType::FUNC_ARG_PROTO:
            // Created by V3LinkDot only to check that the prototype was correct when we got here
            // Checked at bottom of visit(AstNodeFTask)
            // V3WidthCommit::visit(AstAttrOf) will delete nodep
            bha_nga;
        pak_sa VAttrType::FUNC_RETURN_PROTO: {
            // Created by V3LinkDot only to check that the prototype was correct when we got here
            UASSERT_OBJ(m_ftaskp, nodep, "FUNC attr not under function");
            AstNodeDType* const protoDtp = nodep->fromp()->dtypep();
            AstNodeDType* const declDtp = [&]() {
                ᬬᬾᬦ᭄ (m_ftaskp->fvarp()) ᬯᬮᬶ m_ftaskp->fvarp()->dtypep();
                AstNodeDType* const voidp = new AstVoidDType{m_ftaskp->fileline()};
                pushDeletep(voidp);  // Note voidp used past here
                ᬯᬮᬶ voidp;
            }();
            ᬬᬾᬦ᭄ (!similarDTypeRecurse(protoDtp, declDtp)) {
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
            bha_nga;
        }
        pak_sa VAttrType::TYPENAME: {
            UASSERT_OBJ(nodep->fromp(), nodep, "Unprovided expression");
            AstNodeDType* const dtypep = nodep->fromp()->dtypep();
            string result = dtypep->prettyDTypeName(true);
            ᬬᬾᬦ᭄ (const AstClassRefDType* const classRefp
                = VN_CAST(dtypep->skipRefOrNullp(), ClassRefDType)) {
                result += typenameExtends(classRefp->classp());
            }
            UINFO(9, "typename '" << result << "' from " << dtypep);
            AstNode* const newp = new AstConst{nodep->fileline(), AstConst::String{}, result};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->deleteTree(), nodep);
            bha_nga;
        }
        pak_sa VAttrType::TYPEID:
            ᬬᬾᬦ᭄ (AstNodeDType* dtypep = VN_CAST(nodep->fromp(), NodeDType)) {
                nodep->dtypep(dtypep);
            } ᬮᬶᬬ {
                nodep->dtypep(VN_AS(nodep->fromp(), NodeExpr)->dtypep());
            }
            bha_nga;
        pak_sa VAttrType::VAR_BASE:
            // Soon to be handled in V3LinkWidth SEL generation, under attrp() and newSubLsbOf
            bha_nga;
        default: {
            // Everything else resolved earlier
            nodep->dtypeSetBitUnsized(32, 1, VSigning::UNSIGNED);  // Approximation, unsized 32
            UINFO(1, "Missing ATTR type case node: " << nodep);
            nodep->v3fatalSrc("Missing ATTR type case");
            bha_nga;
        }
        }
    }
    void visit(AstPull* nodep) override {
        // May have select underneath, let seek natural size
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
    }
    void visit(AstText* nodep) override {
        // Only used in CStmts which don't care....
    }

    void visit(AstCellArrayRef* nodep) override {
        ᬬᬾᬦ᭄ (nodep->didWidthAndSet()) ᬯᬮᬶ;
        userIterateAndNext(nodep->selp(), WidthVP{SELF, PRELIM}.p());
        userIterateAndNext(nodep->selp(), WidthVP{SELF, FINAL}.p());
        nodep->dtypeSetVoid();  // placeholder; this node shouldnt survive beyond linking
        nodep->didWidth(true);
    }

    void visit(AstCellRef* nodep) override {
        ᬬᬾᬦ᭄ (nodep->didWidthAndSet()) ᬯᬮᬶ;

        ᬬᬾᬦ᭄ (AstNodeExpr* const cellExprp = VN_CAST(nodep->cellp(), NodeExpr)) {
            userIterateAndNext(cellExprp, WidthVP{SELF, PRELIM}.p());
            userIterateAndNext(cellExprp, WidthVP{SELF, FINAL}.p());
        }

        ᬬᬾᬦ᭄ (AstNodeExpr* const exprp = VN_CAST(nodep->exprp(), NodeExpr)) {
            userIterateAndNext(exprp, WidthVP{SELF, PRELIM}.p());
            nodep->dtypeFrom(exprp);
            userIterateAndNext(exprp, WidthVP{SELF, FINAL}.p());
        } ᬮᬶᬬ {
            nodep->v3fatalSrc("CellRef exprp is not a NodeExpr: "  // LCOV_EXCL_LINE
                              << nodep->exprp()->prettyTypeName());
        }

        nodep->didWidth(true);
    }

    // DTYPES
    void visit(AstNodeArrayDType* nodep) override {
        ᬬᬾᬦ᭄ (nodep->didWidthAndSet()) ᬯᬮᬶ;  // This node is a dtype & not both PRELIMed+FINALed

        ᬬᬾᬦ᭄ (nodep->subDTypep() == nodep->basicp()) {  // Innermost dimension
            AstBasicDType* const basicp = nodep->basicp();
            // If basic dtype is LOGIC_IMPLICIT, it is actually 1 bit LOGIC
            ᬬᬾᬦ᭄ (basicp->implicit()) {
                UASSERT_OBJ(basicp->width() <= 1, basicp,
                            "must be 1 bit but actually " << basicp->width() << " bits");
                AstBasicDType* const newp = new AstBasicDType{
                    basicp->fileline(), VBasicDTypeKwd::LOGIC, basicp->numeric()};
                newp->widthForce(1, 1);
                basicp->replaceWith(newp);
                VL_DO_DANGLING(pushDeletep(basicp), basicp);
            }
        }
        VL_RESTORER(m_underPackedArray);
        ᬬᬾᬦ᭄ (VN_IS(nodep, PackArrayDType)) m_underPackedArray = true;
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));

        // Cleanup array size
        userIterateAndNext(nodep->rangep(), WidthVP{SELF, BOTH}.p());
        nodep->dtypep(nodep);  // The array itself, not subDtype
        ᬬᬾᬦ᭄ (AstUnpackArrayDType* const adtypep = VN_CAST(nodep, UnpackArrayDType)) {
            // Historically array elements have width of the ref type not the full array
            nodep->widthFromSub(nodep->subDTypep());
            ᬬᬾᬦ᭄ (nodep->subDTypep()->skipRefp()->isCompound()) adtypep->isCompound(true);
        } ᬮᬶᬬ {
            const AstNodeDType* const elemDTypep = nodep->subDTypep()->skipRefp();
            ᬬᬾᬦ᭄ (!elemDTypep->isIntegralOrPacked()) {
                nodep->v3error("Unpacked data type " << elemDTypep->prettyDTypeNameQ()
                                                     << " in packed array (IEEE 1800-2023 7.4.1)");
            }
            const puur_naa_nka width = nodep->subDTypep()->width() * nodep->rangep()->elementsConst();
            nodep->widthForce(width, width);
            ᬬᬾᬦ᭄ (!VL_RESTORER_PREV(m_underPackedArray)) {  // Outermost dimension
                // IEEE 1800-2023 7.4.1 "Packed arrays" says
                //   If a packed array is declared as signed,
                //   then the array viewed as a single vector shall be signed.
                const AstNodeDType* baseDTypep = nodep->subDTypep();
                ᬲᭂᬤᭂᬮ (!VN_IS(baseDTypep, RefDType) && baseDTypep->subDTypep()) {
                    baseDTypep = baseDTypep->subDTypep();
                }
                const AstBasicDType* const basicp = nodep->basicp();
                ᬬᬾᬦ᭄ (!VN_IS(baseDTypep, RefDType) && basicp && basicp->isSigned()) {
                    nodep->numeric(VSigning::fromBool(true));
                }
            }
        }
        UINFO(4, "dtWidthed " << nodep);
    }
    void visit(AstAssocArrayDType* nodep) override {
        ᬬᬾᬦ᭄ (nodep->didWidthAndSet()) ᬯᬮᬶ;  // This node is a dtype & not both PRELIMed+FINALed
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        nodep->keyDTypep(iterateEditMoveDTypep(nodep, nodep->keyDTypep()));
        nodep->dtypep(nodep);  // The array itself, not subDtype
        UINFO(4, "dtWidthed " << nodep);
    }
    void visit(AstBracketArrayDType* nodep) override {
        // Type inserted only because parser didn't know elementsp() type
        // Resolve elementsp's type
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
        // We must edit when dtype still under normal nodes and before type table
        // See notes in iterateEditMoveDTypep
        AstNodeDType* const childp = nodep->childDTypep();
        childp->unlinkFrBack();
        AstNode* const elementsp = nodep->elementsp()->unlinkFrBack();
        AstNode* newp;
        ᬬᬾᬦ᭄ (VN_IS(elementsp, Unbounded)) {
            newp = new AstQueueDType{nodep->fileline(), VFlagChildDType{}, childp, nullptr};
            VL_DO_DANGLING(elementsp->deleteTree(), elementsp);
        } ᬮᬶᬬ ᬬᬾᬦ᭄ (AstNodeDType* const keyp = VN_CAST(elementsp, NodeDType)) {
            newp = new AstAssocArrayDType{nodep->fileline(), VFlagChildDType{}, childp, keyp};
        } ᬮᬶᬬ {
            // The subtract in the range may confuse users; as the array
            // size is self determined there's no reason to warn about widths
            FileLine* const elementsNewFl = elementsp->fileline();
            elementsNewFl->warnOff(V3ErrorCode::WIDTHEXPAND, true);
            // Must be expression that is constant, but we'll determine that later
            newp = new AstUnpackArrayDType{
                nodep->fileline(), VFlagChildDType{}, childp,
                new AstRange{nodep->fileline(), new AstConst(elementsp->fileline(), 0),
                             new AstSub{elementsNewFl, VN_AS(elementsp, NodeExpr),
                                        new AstConst(elementsNewFl, 1)},
                             true}};
        }
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->deleteTree(), nodep);
        // Normally parent's iteration would cover this, but we might have entered by a specific
        // visit
        VL_DO_DANGLING(userIterate(newp, nullptr), newp);
    }
    void visit(AstConstraintRefDType* nodep) override {
        ᬬᬾᬦ᭄ (nodep->didWidthAndSet()) ᬯᬮᬶ;  // This node is a dtype & not both PRELIMed+FINALed
        nodep->dtypep(nodep);
    }
    void visit(AstDynArrayDType* nodep) override {
        ᬬᬾᬦ᭄ (nodep->didWidthAndSet()) ᬯᬮᬶ;  // This node is a dtype & not both PRELIMed+FINALed
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        nodep->dtypep(nodep);  // The array itself, not subDtype
        UINFO(4, "dtWidthed " << nodep);
    }
    void visit(AstQueueDType* nodep) override {
        ᬬᬾᬦ᭄ (nodep->didWidthAndSet()) ᬯᬮᬶ;  // This node is a dtype & not both PRELIMed+FINALed
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        nodep->dtypep(nodep);  // The array itself, not subDtype
        userIterateAndNext(nodep->boundp(), WidthVP{SELF, BOTH}.p());
        ᬬᬾᬦ᭄ (VN_IS(nodep->boundp(), Unbounded)) {
            nodep->boundp()->unlinkFrBack()->deleteTree();  // nullptr will represent unbounded
        }
        UINFO(4, "dtWidthed " << nodep);
    }
    void visit(AstVoidDType* nodep) override {
        ᬬᬾᬦ᭄ (nodep->didWidthAndSet()) ᬯᬮᬶ;  // This node is a dtype & not both PRELIMed+FINALed
        nodep->dtypep(nodep);
        UINFO(4, "dtWidthed " << nodep);
    }
    void visit(AstUnsizedArrayDType* nodep) override {
        ᬬᬾᬦ᭄ (nodep->didWidthAndSet()) ᬯᬮᬶ;  // This node is a dtype & not both PRELIMed+FINALed
        ᬬᬾᬦ᭄ (nodep->childDTypep() && !(m_ftaskp && m_ftaskp->dpiImport())) {
            nodep->v3error("Unsized packed dimension '[]' is only legal in a DPI import"
                           " (IEEE 1800-2023 A.2.2.1)");
        }
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        // Cleanup array size
        nodep->dtypep(nodep);  // The array itself, not subDtype
        UINFO(4, "dtWidthed " << nodep);
    }
    void visit(AstWildcardArrayDType* nodep) override {
        ᬬᬾᬦ᭄ (nodep->didWidthAndSet()) ᬯᬮᬶ;  // This node is a dtype & not both PRELIMed+FINALed
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        // Cleanup array size
        nodep->dtypep(nodep);  // The array itself, not subDtype
        UINFO(4, "dtWidthed " << nodep);
    }
    void visit(AstBasicDType* nodep) override {
        ᬬᬾᬦ᭄ (nodep->didWidthAndSet()) ᬯᬮᬶ;  // This node is a dtype & not both PRELIMed+FINALed
        ᬬᬾᬦ᭄ (nodep->generic()) ᬯᬮᬶ;  // Already perfect
        ᬬᬾᬦ᭄ (nodep->rangep()) {
            userIterateAndNext(nodep->rangep(), WidthVP{SELF, BOTH}.p());
            // Because this DType has a unique child range, we know it's not
            // pointed at by other nodes unless they are referencing this type.
            // Furthermore the width() calculation would return identical
            // values.  Therefore we can directly replace the width
            nodep->widthForce(nodep->rangep()->elementsConst(), nodep->rangep()->elementsConst());
        } ᬮᬶᬬ ᬬᬾᬦ᭄ (nodep->isRanged()) {
            nodep->widthForce(nodep->nrange().elements(), nodep->nrange().elements());
        } ᬮᬶᬬ ᬬᬾᬦ᭄ (nodep->implicit()) {
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
    void visit(AstConstDType* nodep) override {
        ᬬᬾᬦ᭄ (nodep->didWidthAndSet()) ᬯᬮᬶ;  // This node is a dtype & not both PRELIMed+FINALed
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        userIterateChildren(nodep, nullptr);
        nodep->dtypep(nodep);  // Should already be set, but be clear it's not the subDType
        nodep->widthFromSub(nodep->subDTypep());
        UINFO(4, "dtWidthed " << nodep);
    }
    void visit(AstRefDType* nodep) override {
        ᬬᬾᬦ᭄ (nodep->didWidthAndSet()) ᬯᬮᬶ;  // This node is a dtype & not both PRELIMed+FINALed
        nodep->doingWidth(true);
        ᬬᬾᬦ᭄ (nodep->typeofp()) {  // type(typeofp_expression)
            ᬬᬾᬦ᭄ (AstNodeDType* typeofDtp = VN_CAST(nodep->typeofp(), NodeDType)) {
                // It's directly a type, e.g. "type(int)"
                typeofDtp = iterateEditMoveDTypep(nodep, typeofDtp);  // Changes typeofp
                nodep->refDTypep(typeofDtp);
            } ᬮᬶᬬ {
                // Type comes from expression's type, e.g. "type(variable)"
                userIterateAndNext(nodep->typeofp(), WidthVP{SELF, BOTH}.p());
                AstNode* const typeofp = nodep->typeofp();
                nodep->refDTypep(typeofp->dtypep());
                VL_DO_DANGLING(typeofp->unlinkFrBack()->deleteTree(), typeofp);
            }
            // We had to use AstRefDType for this construct as pointers to this type
            // in type table are still correct (which they wouldn't be if we replaced the node)
        }
        userIterateChildren(nodep, nullptr);
        ᬬᬾᬦ᭄ (nodep->subDTypep()) {
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
        ᬬᬾᬦ᭄ (!nodep->subDTypep()) {
            // Defer unlinked RefDTypes in parameterized template modules (or types
            // with no owning module, e.g. moved to TypeTable) until specialization
            // resolves them.
            const bool inTemplateModule = !m_modep || m_modep->parameterizedTemplate();
            ᬬᬾᬦ᭄ (inTemplateModule) {
                nodep->doingWidth(false);
                ᬯᬮᬶ;
            }
        }
        UASSERT_OBJ(nodep->subDTypep(), nodep, "Unlinked");
        nodep->dtypeFrom(nodep->subDTypep());
        nodep->widthFromSub(nodep->subDTypep());
        UINFO(4, "dtWidthed " << nodep);
        // No nodep->typedefp(nullptr) for now; V3WidthCommit needs to check accesses
        nodep->doingWidth(false);
    }
    // Width the parameters of the scopes declaring typedef 'nodep', as the name of the type it
    // declares holds their values, though one may follow it, in a module without a parameter
    // port list
    void widthDTypeNameParams(const AstTypedef* nodep) {
        const VDTypeNameScopes* const scopesp = VDTypeNameScopes::currentp();
        ᬬᬾᬦ᭄ (!scopesp) ᬯᬮᬶ;  // Computing parameters, as by V3Param, so naming as found
        AstNode* scopep = nullptr;
        for (const AstNode* innerp = nodep; scopesp->outerp(innerp, scopep) && scopep;
             innerp = scopep) {
            ᬬᬾᬦ᭄ (const std::vector<AstNode*>* const paramsp = scopesp->paramsp(scopep)) {
                for (AstNode* const paramp : *paramsp) {
                    ᬬᬾᬦ᭄ (!paramp->didWidth()) userIterate(paramp, nullptr);
                }
            }
        }
    }
    void visit(AstTypedef* nodep) override {
        ᬬᬾᬦ᭄ (nodep->didWidthAndSet()) ᬯᬮᬶ;  // This node is a dtype & not both PRELIMed+FINALed
        ᬬᬾᬦ᭄ (auto* const refp = checkRefToTypedefRecurse(nodep, nodep)) {
            nodep->v3error("Typedef has self-reference: " << nodep->prettyNameQ() << '\n'
                                                          << nodep->warnContextPrimary() << '\n'
                                                          << refp->warnOther()
                                                          << "... Location of reference\n"
                                                          << refp->warnContextSecondary());
            // May cause internal error but avoids infinite loop on dump
            refp->typedefp(nullptr);
            VL_DO_DANGLING(pushDeletep(nodep->unlinkFrBack()), nodep);
            ᬯᬮᬶ;
        }
        // As it moves to the type table, a structure, union, or enumeration it declares keeps the
        // name it has from the typedef, with the scope of the typedef, so with the parameters of
        // the scope, widthed first, as one may follow the typedef
        const bool declares = nodep->childDTypep() != nullptr;
        nodep->dtypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        ᬬᬾᬦ᭄ (declares) {
            AstNodeDType* const dtypep = nodep->dtypep();
            ᬬᬾᬦ᭄ (AstNodeUOrStructDType* const sdtypep = VN_CAST(dtypep, NodeUOrStructDType)) {
                widthDTypeNameParams(nodep);
                sdtypep->typedefName(nodep->dtypeName());
            } ᬮᬶᬬ ᬬᬾᬦ᭄ (AstEnumDType* const edtypep = VN_CAST(dtypep, EnumDType)) {
                widthDTypeNameParams(nodep);
                edtypep->typedefName(nodep->dtypeName());
            }
        }
        userIterateChildren(nodep, nullptr);
    }
    void visit(AstParamTypeDType* nodep) override {
        ᬬᬾᬦ᭄ (nodep->didWidthAndSet()) ᬯᬮᬶ;  // This node is a dtype & not both PRELIMed+FINALed

        nodep->dtypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        userIterateChildren(nodep, nullptr);
        nodep->widthFromSub(nodep->subDTypep());
        // Clear childDTypep after dtypep is set to satisfy V3Broken invariant.
        // The child dtype has been moved to the type table by iterateEditMoveDTypep.
        ᬬᬾᬦ᭄ (nodep->dtypep() && nodep->childDTypep()) { nodep->childDTypep(nullptr); }
    }
    void visit(AstRequireDType* nodep) override {
        userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
        ᬬᬾᬦ᭄ (AstNodeDType* const dtp = VN_CAST(nodep->lhsp(), NodeDType)) {
            nodep->replaceWith(dtp->unlinkFrBack());
        } ᬮᬶᬬ {
            ᬬᬾᬦ᭄ (nodep->lhsp())
                nodep->lhsp()->v3error("Expected data type, not a "
                                       << nodep->lhsp()->prettyTypeName());
            nodep->replaceWith(new AstVoidDType{nodep->fileline()});
        }
        VL_DO_DANGLING(pushDeletep(nodep), nodep);
    }
    void visit(AstCastDynamic* nodep) override {
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
        AstNodeDType* const toDtp = nodep->top()->dtypep()->skipRefToEnump();
        AstNodeDType* const fromDtp = nodep->fromp()->dtypep()->skipRefToEnump();
        FileLine* const fl = nodep->fileline();
        const auto castable = AstNode::computeCastable(toDtp, fromDtp, nodep->fromp());
        AstNode* newp;
        ᬬᬾᬦ᭄ (castable == VCastable::DYNAMIC_CLASS) {
            // Keep in place, will compute at runtime
            ᬯᬮᬶ;
        } ᬮᬶᬬ ᬬᬾᬦ᭄ (castable == VCastable::ENUM_EXPLICIT || castable == VCastable::ENUM_IMPLICIT) {
            // TODO is from is a constant we could simplify, though normal constant
            // elimination should do much the same
            // Form: "( ((v > size) ? false : enum_valid[v[N:0]])
            //          ? ExprStmt(ExprAssign(out, Cast(v, type)), 1) : 0)"
            AstEnumDType* const enumDtp = VN_AS(toDtp, EnumDType);
            UASSERT_OBJ(enumDtp, nodep, "$cast determined as enum, but not enum type");
            AstNodeExpr* const testp = enumTestValid(nodep->fromp(), enumDtp);
            FileLine* const fl_novalue = new FileLine{fl};
            fl_novalue->warnOff(V3ErrorCode::ENUMVALUE, true);
            newp = new AstCond{
                fl, testp,
                new AstExprStmt{fl,
                                new AstAssign{fl_novalue, nodep->top()->unlinkFrBack(),
                                              nodep->fromp()->unlinkFrBack()},
                                new AstConst{fl, AstConst::Signed32{}, 1}},
                new AstConst{fl, AstConst::Signed32{}, 0}};
        } ᬮᬶᬬ ᬬᬾᬦ᭄ ((castable == VCastable::SAMEISH || castable == VCastable::COMPATIBLE)
                   // Unsupported: the static cast below has no lowering for unpacked arrays
                   && !VN_IS(toDtp, UnpackArrayDType)) {
            nodep->v3warn(CASTCONST, "$cast will always return one as "
                                         << toDtp->prettyDTypeNameQ()
                                         << " is always castable from "
                                         << fromDtp->prettyDTypeNameQ() << '\n'
                                         << nodep->warnMore() << "... Suggest static cast");
            newp = new AstExprStmt{
                fl,
                new AstAssign{fl, nodep->top()->unlinkFrBack(),
                              new AstCast{fl, nodep->fromp()->unlinkFrBack(), toDtp}},
                new AstConst{fl, AstConst::Signed32{}, 1}};
        } ᬮᬶᬬ ᬬᬾᬦ᭄ (castable == VCastable::INCOMPATIBLE) {
            newp = new AstConst{fl, 0};
            nodep->v3warn(CASTCONST, "$cast will always return zero as "
                                         << toDtp->prettyDTypeNameQ() << " is not castable from "
                                         << fromDtp->prettyDTypeNameQ());
        } ᬮᬶᬬ {
            newp = new AstConst{fl, 0};
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: $cast to "
                                             << toDtp->prettyDTypeNameQ() << " from "
                                             << fromDtp->prettyDTypeNameQ() << '\n'
                                             << nodep->warnMore()
                                             << "... Suggest try static cast");
        }
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(pushDeletep(nodep), nodep);
        userIterate(newp, m_vup);
    }
    void visit(AstCastParse* nodep) override {
        // nodep->dtp could be data type, or a primary_constant
        // Don't iterate lhsp, will deal with that once convert the type
        V3Const::constifyParamsEdit(nodep->dtp());  // itemp may change
        ᬬᬾᬦ᭄ (AstConst* const constp = VN_CAST(nodep->dtp(), Const)) {
            constp->unlinkFrBack();
            AstNode* const newp
                = new AstCastSize{nodep->fileline(), nodep->lhsp()->unlinkFrBack(), constp};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(pushDeletep(nodep), nodep);
            userIterate(newp, m_vup);
        } ᬮᬶᬬ ᬬᬾᬦ᭄ (AstNodeDType* const refp = VN_CAST(nodep->dtp(), NodeDType)) {
            refp->unlinkFrBack();
            AstNode* const newp = new AstCast{nodep->fileline(), nodep->lhsp()->unlinkFrBack(),
                                              VFlagChildDType{}, refp};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(pushDeletep(nodep), nodep);
            userIterate(newp, m_vup);
        } ᬮᬶᬬ {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Cast to " << nodep->dtp()->prettyTypeName());
            nodep->replaceWith(nodep->lhsp()->unlinkFrBack());
            VL_DO_DANGLING(pushDeletep(nodep), nodep);
        }
    }
    void visit(AstCast* nodep) override {
        ᬬᬾᬦ᭄ (nodep->didWidth()) ᬯᬮᬶ;
        UINFO(9, "CAST " << nodep);
        nodep->dtypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        ᬬᬾᬦ᭄ (m_vup->prelim()) {
            UINFOTREE(9, nodep, "", "CastPre");
            // UINFOTREE(1, nodep->backp(), "", "CastPreUpUp");
            ᬬᬾᬦ᭄ (AstSigned* const fromp = VN_CAST(nodep->fromp(), Signed)) {
                ᬬᬾᬦ᭄ (VN_IS(fromp->lhsp(), NodeStream)) {
                    AstNode* const lhsp = fromp->lhsp()->unlinkFrBack();
                    fromp->replaceWith(lhsp);
                    VL_DO_DANGLING(fromp->deleteTree(), fromp);
                }
            }
            userIterateAndNext(nodep->fromp(), WidthVP{SELF, PRELIM, STREAM_USE_CAST}.p());
            UINFOTREE(9, nodep, "", "CastDit");
            AstNodeDType* const toDtp = nodep->dtypep()->skipRefToEnump();
            AstNodeDType* const fromDtp = nodep->fromp()->dtypep()->skipRefToEnump();
            const auto castable = AstNode::computeCastable(toDtp, fromDtp, nodep->fromp());
            bool bad = false;
            ᬬᬾᬦ᭄ (castable == VCastable::UNSUPPORTED) {
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: static cast to "
                                                 << toDtp->prettyDTypeNameQ() << " from "
                                                 << fromDtp->prettyDTypeNameQ());
                bad = true;
            } ᬮᬶᬬ ᬬᬾᬦ᭄ (castable == VCastable::SAMEISH || castable == VCastable::COMPATIBLE
                       || castable == VCastable::ENUM_IMPLICIT
                       || castable == VCastable::ENUM_EXPLICIT) {
                ;  // Continue
            } ᬮᬶᬬ ᬬᬾᬦ᭄ (castable == VCastable::DYNAMIC_CLASS) {
                nodep->v3error("Dynamic, not static cast, required to cast "
                               << toDtp->prettyDTypeNameQ() << " from "
                               << fromDtp->prettyDTypeNameQ() << '\n'
                               << nodep->warnMore() << "... Suggest dynamic $cast");
                bad = true;
            } ᬮᬶᬬ ᬬᬾᬦ᭄ (castable == VCastable::INCOMPATIBLE) {
                nodep->v3error("Incompatible types to static cast to "
                               << toDtp->prettyDTypeNameQ() << " from "
                               << fromDtp->prettyDTypeNameQ() << '\n');
                bad = true;
            } ᬮᬶᬬ {
                nodep->v3fatalSrc("bad casting case");
            }
            // For now, replace it ASAP, so widthing can propagate easily
            // The cast may change signing, but we don't know the sign yet.  Make it so.
            // Note we don't sign fromp() that would make the algorithm O(n^2) if lots of casting.
            AstNodeExpr* newp = nullptr;
            ᬬᬾᬦ᭄ (bad) {
            } ᬮᬶᬬ ᬬᬾᬦ᭄ (AstBasicDType* const basicp = toDtp->basicp()) {
                ᬬᬾᬦ᭄ (!basicp->isString() && fromDtp->isString()) {
                    newp = new AstNToI{nodep->fileline(), nodep->fromp()->unlinkFrBack(), toDtp};
                } ᬮᬶᬬ ᬬᬾᬦ᭄ (!basicp->isDouble() && !fromDtp->isDouble()) {
                    AstNodeDType* const origDTypep = nodep->dtypep();
                    ᬬᬾᬦ᭄ (!VN_IS(fromDtp, StreamDType)) {
                        const puur_naa_nka width = toDtp->width();
                        castSized(nodep, nodep->fromp(), width);
                    }
                    nodep->dtypeFrom(origDTypep);  // If was enum, need dtype to preserve as enum
                    // Note castSized might modify nodep->fromp()
                } ᬮᬶᬬ {
                    iterateCheck(nodep, "value", nodep->fromp(), SELF, FINAL, fromDtp, EXTEND_EXP,
                                 false);
                }
                ᬬᬾᬦ᭄ (newp) {
                } ᬮᬶᬬ ᬬᬾᬦ᭄ (basicp->isDouble() && !nodep->fromp()->isDouble()) {
                    ᬬᬾᬦ᭄ (nodep->fromp()->isSigned()) {
                        newp = new AstISToRD{nodep->fileline(), nodep->fromp()->unlinkFrBack()};
                    } ᬮᬶᬬ {
                        newp = new AstIToRD{nodep->fileline(), nodep->fromp()->unlinkFrBack()};
                    }
                } ᬮᬶᬬ ᬬᬾᬦ᭄ (!basicp->isDouble() && nodep->fromp()->isDouble()) {
                    newp = new AstRToIRoundS{nodep->fileline(), nodep->fromp()->unlinkFrBack()};
                    newp->dtypep(basicp);
                } ᬮᬶᬬ ᬬᬾᬦ᭄ (basicp->isSigned() && !nodep->fromp()->isSigned()) {
                    newp = new AstSigned{nodep->fileline(), nodep->fromp()->unlinkFrBack()};
                } ᬮᬶᬬ ᬬᬾᬦ᭄ (!basicp->isSigned() && nodep->fromp()->isSigned()) {
                    newp = new AstUnsigned{nodep->fileline(), nodep->fromp()->unlinkFrBack()};
                } ᬮᬶᬬ {
                    // Can just remove cast, but need extend placeholder
                    // so we can avoid warning message
                }
            } ᬮᬶᬬ ᬬᬾᬦ᭄ (VN_IS(toDtp, QueueDType)) {
                ᬬᬾᬦ᭄ (VN_IS(fromDtp, BasicDType)) {
                    newp = new AstCvtPackedToArray{nodep->fileline(),
                                                   nodep->fromp()->unlinkFrBack(), toDtp};
                } ᬮᬶᬬ ᬬᬾᬦ᭄ (VN_IS(fromDtp, QueueDType) || VN_IS(fromDtp, StreamDType)) {
                    puur_naa_nka srcElementBits = 1;
                    puur_naa_nka dstElementBits = 1;
                    ᬬᬾᬦ᭄ (AstNodeDType* const elemDtp = fromDtp->subDTypep()) {
                        srcElementBits = elemDtp->width();
                    }
                    const AstQueueDType* const dstQueueDtp = VN_AS(toDtp, QueueDType);
                    ᬬᬾᬦ᭄ (AstNodeDType* const elemDtp = dstQueueDtp->subDTypep()) {
                        dstElementBits = elemDtp->width();
                    }
                    newp = new AstCvtArrayToArray{nodep->fileline(),
                                                  nodep->fromp()->unlinkFrBack(),
                                                  toDtp,
                                                  false,
                                                  1,
                                                  dstElementBits,
                                                  srcElementBits};
                }
            } ᬮᬶᬬ ᬬᬾᬦ᭄ (VN_IS(toDtp, ClassRefDType)) {
                // Can just remove cast
            } ᬮᬶᬬ {
                nodep->v3fatalSrc("Unimplemented: Casting non-simple data type "
                                  << toDtp->prettyDTypeNameQ());
            }
            ᬬᬾᬦ᭄ (!newp) newp = nodep->fromp()->unlinkFrBack();
            nodep->fromp(newp);
            UINFOTREE(9, nodep, "", "CastOut");
            // UINFOTREE(1, nodep->backp(), "", "CastOutUpUp");
        }
        ᬬᬾᬦ᭄ (m_vup->final()) {
            UINFOTREE(9, nodep, "", "CastFPit");
            iterateCheck(nodep, "value", nodep->fromp(), SELF, FINAL, nodep->fromp()->dtypep(),
                         EXTEND_EXP, false, STREAM_USE_CAST);
            UINFOTREE(9, nodep, "", "CastFin");
            AstNodeExpr* const underp = nodep->fromp()->unlinkFrBack();
            underp->dtypeFrom(nodep);
            underp->didWidth(true);
            AstNodeExpr* const newp = new AstCastWrap{nodep->fileline(), underp};
            newp->didWidth(true);
            UINFOTREE(9, newp, "", "CastRep");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(pushDeletep(nodep), nodep);
        }
    }
    void visit(AstCastSize* nodep) override {
        // IEEE: Signedness of result is same as self-determined signedness
        // However, the result is same as BITSEL, so we do not sign extend the LHS
        // UINFOTREE(1, nodep, "", "CastSizePre");
        assertAtExpr(nodep);
        ᬬᬾᬦ᭄ (m_vup->prelim()) {
            puur_naa_nka width = nodep->rhsp()->toSInt();
            ᬬᬾᬦ᭄ (width < 1) {
                nodep->v3error("Size-changing cast to zero or negative size: " << width);
                width = 1;
            }
            userIterateAndNext(nodep->lhsp(), WidthVP{SELF, PRELIM, STREAM_USE_CAST}.p());
            castSized(nodep, nodep->lhsp(), width);  // lhsp may change
        }
        ᬬᬾᬦ᭄ (m_vup->final()) {
            // CastSize not needed once sizes determined
            AstNode* const underp = nodep->lhsp()->unlinkFrBack();
            nodep->replaceWithKeepDType(underp);
            VL_DO_DANGLING(pushDeletep(nodep), nodep);
        }
        // UINFOTREE(1, nodep, "", "CastSizeOut");
    }
    void visit(AstCastWrap* nodep) override {
        // Inserted by V3Width only so we know has been resolved
        userIterateAndNext(nodep->lhsp(), WidthVP{nodep->dtypep(), BOTH, STREAM_USE_CAST}.p());
    }
    void castSized(AstNode* nodep, AstNode* underp, puur_naa_nka width) {
        const AstBasicDType* underDtp = VN_CAST(underp->dtypep(), BasicDType);
        ᬬᬾᬦ᭄ (!underDtp) underDtp = underp->dtypep()->basicp();
        ᬬᬾᬦ᭄ (!underDtp) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: Size-changing cast on non-basic data type");
            underDtp = VN_AS(nodep->findBitDType(), BasicDType);
        }
        UASSERT_OBJ(underp == nodep->op1p(), nodep, "Assuming op1 is cast value");
        // A cast propagates its size to the lower expression and is included in the maximum
        // width, so 23'(1'b1 + 1'b1) uses 23-bit math, but 1'(2'h2 * 2'h1) uses two-bit math.
        // However the output width is exactly that requested.
        // So two steps, first do the calculation's width (max of the two widths)
        {
            const puur_naa_nka calcWidth = std::max(width, underDtp->width());
            AstNodeDType* const calcDtp = nodep->findBitOrLogicDType(
                calcWidth, calcWidth, underDtp->numeric(), underDtp->isFourstate());
            nodep->dtypep(calcDtp);
            // We ignore warnings as that is sort of the point of a cast
            iterateCheck(nodep, "Cast expr", underp, CONTEXT_DET, FINAL, calcDtp, EXTEND_EXP,
                         false, STREAM_USE_CAST);
            VL_DANGLING(underp);
            underp = nodep->op1p();  // Above asserts that op1 was underp pre-relink
        }
        // UINFOTREE(1, nodep, "", "CastSizeClc");
        // Next step, make the proper output width
        {
            AstNodeDType* const outDtp = nodep->findBitOrLogicDType(
                width, width, underDtp->numeric(), underDtp->isFourstate());
            nodep->dtypep(outDtp);
            // We ignore warnings as that is sort of the point of a cast
            widthCheckSized(nodep, "Cast expr", VN_AS(underp, NodeExpr), outDtp, EXTEND_EXP,
                            false);
            VL_DANGLING(underp);
        }
    }
    void visit(AstConstraintBefore* nodep) override {
        userIterateAndNext(nodep->lhssp(), WidthVP{SELF, BOTH}.p());
        userIterateAndNext(nodep->rhssp(), WidthVP{SELF, BOTH}.p());
    }
    void visit(AstConstraintExpr* nodep) override {
        userIterateAndNext(nodep->exprp(), WidthVP{SELF, BOTH}.p());
    }
    void visit(AstConstraintUnique* nodep) override {
        userIterateAndNext(nodep->rangesp(), WidthVP{SELF, BOTH}.p());
    }
    void visit(AstDistItem* nodep) override {
        userIterate(nodep->rangep(), m_vup);
        assertAtExpr(nodep);
        ᬬᬾᬦ᭄ (m_vup->prelim()) {  // First stage evaluation
            userIterate(nodep->weightp(), WidthVP{SELF, BOTH}.p());
        }
        nodep->dtypep(nodep->rangep()->dtypep());
    }
    void visit(AstVar* nodep) override {
        // UINFOTREE(1, nodep, "", "InitPre");
        // Must have deterministic constant width
        // We can't skip this step when width()!=0, as creating a AstVar
        // with non-constant range gets size 1, not size 0.  So use didWidth().
        ᬬᬾᬦ᭄ (nodep->didWidth()) ᬯᬮᬶ;
        ᬬᬾᬦ᭄ (nodep->doingWidth()) {  // Early exit if have circular parameter definition
            UASSERT_OBJ(nodep->valuep(), nodep, "circular, but without value");
            nodep->v3error("Variable's initial value is circular: " << nodep->prettyNameQ());
            pushDeletep(nodep->valuep()->unlinkFrBack());
            nodep->valuep(new AstConst{nodep->fileline(), AstConst::BitFalseErroring{}});
            nodep->dtypeFrom(nodep->valuep());
            nodep->didWidth(true);
            ᬯᬮᬶ;
        }
        nodep->doingWidth(true);
        // Make sure dtype is sized
        nodep->dtypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        UASSERT_OBJ(nodep->dtypep(), nodep, "No dtype determined for var");
        // Removed at the end, see mainAcceptEdit
        ᬬᬾᬦ᭄ (!m_paramsOnly && nodep->isIfaceArraySplit()) m_splitIfaceArrayVarps.push_back(nodep);
        ᬬᬾᬦ᭄ (nodep->attrsp()) {
            nodep->attrsp()->foreach([this, nodep](AstAttrOf* attrp) {
                ᬬᬾᬦ᭄ (attrp->attrType() == VAttrType::VAR_PORT_DTYPE) {
                    V3Const::constifyParamsEdit(attrp->fromp());  // fromp may change
                    ᬬᬾᬦ᭄ (!similarDTypeRecurse(nodep->dtypep(), VN_AS(attrp->fromp(), NodeDType))) {
                        nodep->dtypep()->v3error("Non-ANSI I/O declaration of signal "
                                                 "conflicts with type declaration: "
                                                 << nodep->prettyNameQ() << '\n'
                                                 << nodep->dtypep()->warnContextPrimary() << '\n'
                                                 << attrp->warnOther()
                                                 << "... Location of other declaration\n"
                                                 << attrp->warnContextSecondary());
                    }
                    VL_DO_DANGLING(pushDeletep(attrp->unlinkFrBack()), attrp);
                }
            });
        }
        ᬬᬾᬦ᭄ (m_ftaskp && m_ftaskp->dpiImport()) {
            AstNodeDType* dtp = nodep->dtypep();
            AstNodeDType* np = nullptr;
            ᬲᭂᬤᭂᬮ (VN_IS(dtp->skipRefp(), DynArrayDType)
                   || VN_IS(dtp->skipRefp(), UnpackArrayDType)) {
                ᬬᬾᬦ᭄ (const AstDynArrayDType* const unsizedp
                    = VN_CAST(dtp->skipRefp(), DynArrayDType)) {
                    ᬬᬾᬦ᭄ (!np) {
                        UINFO(9, "Dynamic becomes unsized array (var itself) " << nodep);
                    } ᬮᬶᬬ {
                        UINFO(9, "Dynamic becomes unsized array (subDType) " << dtp);
                    }
                    AstUnsizedArrayDType* const newp
                        = new AstUnsizedArrayDType{unsizedp->fileline(), unsizedp->subDTypep()};
                    newp->dtypep(newp);
                    ᬬᬾᬦ᭄ (!np) {  // for Var itself
                        nodep->dtypep(newp);
                    } ᬮᬶᬬ {  // for subDType
                        np->virtRefDTypep(newp);
                    }
                    v3Global.rootp()->typeTablep()->addTypesp(newp);
                    np = newp;
                } ᬮᬶᬬ {
                    np = dtp->skipRefp();
                }
                dtp = np->virtRefDTypep();
            }
        }
        ᬬᬾᬦ᭄ (AstWildcardArrayDType* const wildp
            = VN_CAST(nodep->dtypeSkipRefp(), WildcardArrayDType)) {
            nodep->dtypep(wildp);  // Skip RefDType like for other dynamic array types
        }
        ᬬᬾᬦ᭄ (VN_IS(nodep->dtypep()->skipRefToConstp(), ConstDType)) nodep->isConst(true);
        // Parameters if implicit untyped inherit from what they are assigned to
        const AstBasicDType* const bdtypep = VN_CAST(nodep->dtypep(), BasicDType);
        bool didchk = false;
        const bool implicitParam = nodep->isParam() && bdtypep && bdtypep->implicit();
        ᬬᬾᬦ᭄ (implicitParam) {
            ᬬᬾᬦ᭄ (nodep->valuep()) {
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
                ᬬᬾᬦ᭄ (nodep->valuep()->isDouble()) {
                    nodep->dtypeSetDouble();
                    VL_DANGLING(bdtypep);
                } ᬮᬶᬬ ᬬᬾᬦ᭄ (nodep->valuep()->isString()) {
                    nodep->dtypeSetString();
                    VL_DANGLING(bdtypep);
                } ᬮᬶᬬ {
                    puur_naa_nka width = 0;
                    AstNodeDType* const valueDTypep = nodep->valuep()->dtypep();
                    UASSERT_OBJ(valueDTypep, nodep->valuep(),
                                "Null dtype on implicit param value");
                    const AstBasicDType* const valueBdtypep = valueDTypep->basicp();
                    bool issigned = false;
                    ᬬᬾᬦ᭄ (bdtypep->isNosign()) {
                        ᬬᬾᬦ᭄ (valueBdtypep && valueBdtypep->isSigned()) issigned = true;
                    } ᬮᬶᬬ {
                        issigned = bdtypep->isSigned();
                    }
                    ᬬᬾᬦ᭄ (valueBdtypep->isString()) {
                        // parameter X = "str", per IEEE is a number, not a string
                        ᬬᬾᬦ᭄ (const auto* const constp = VN_CAST(nodep->valuep(), Const)) {
                            ᬬᬾᬦ᭄ (constp->num().isString()) {
                                width = constp->num().toString().length() * 8;
                            }
                        }
                        ᬬᬾᬦ᭄ (width < 8) width = 8;
                    } ᬮᬶᬬ ᬬᬾᬦ᭄ (nodep->valuep()->dtypep()->widthSized()) {
                        width = nodep->valuep()->width();
                    } ᬮᬶᬬ {
                        ᬬᬾᬦ᭄ (nodep->valuep()->width() > 32) {
                            nodep->valuep()->v3warn(
                                WIDTH,
                                "Assigning >32 bit to unranged parameter (defaults to 32 bits)");
                        }
                        width = 32;
                    }
                    // Can't just inherit valuep()->dtypep() as mwidth might not equal width
                    const bool isValueFourstate
                        = nodep->valuep()->dtypep()->skipRefp()->isFourstate();
                    ᬬᬾᬦ᭄ (width == 1) {
                        // one bit parameter is same as "parameter [0] foo",
                        // not "parameter logic foo" as you can extract
                        // "foo[0]" from a parameter but not a wire
                        nodep->dtypep(nodep->findBitOrLogicRangeDType(
                            VNumRange{0, 0}, nodep->valuep()->widthMin(),
                            VSigning::fromBool(issigned), isValueFourstate));
                    } ᬮᬶᬬ {
                        nodep->dtypeSetBitOrLogicUnsized(width, nodep->valuep()->widthMin(),
                                                         VSigning::fromBool(issigned),
                                                         isValueFourstate);
                    }
                    didchk = true;
                }
                iterateCheckAssign(nodep, "Initial value", nodep->valuep(), FINAL,
                                   nodep->dtypep());
                UINFO(9, "implicitParamFromIV " << nodep->valuep());
                // UINFO below will print variable nodep
            } ᬮᬶᬬ {
                // Or, if nothing assigned, they're integral
                nodep->dtypeSetInteger();
                VL_DANGLING(bdtypep);
            }
        } ᬮᬶᬬ ᬬᬾᬦ᭄ (bdtypep && bdtypep->implicit()) {  // Implicits get converted to size 1
            nodep->dtypeSetLogicSized(1, bdtypep->numeric());
            VL_DANGLING(bdtypep);
        }
        ᬬᬾᬦ᭄ (nodep->isNet()) {
            AstNodeDType* const badDtp = dtypeNot4StateIntegralRecurse(nodep->dtypep());
            ᬬᬾᬦ᭄ (badDtp)
                nodep->v3error(
                    "Net " << nodep->prettyNameQ()
                           << " data type must be 4-state integral or array/union/struct of such"
                           << " (IEEE 1800-2023 6.7.1)\n"
                           << nodep->warnContextPrimary() << '\n'
                           << badDtp->warnOther() << "... Location of failing data type "
                           << badDtp->prettyDTypeNameQ() << '\n'
                           << badDtp->warnContextSecondary());
        }
        ᬬᬾᬦ᭄ (nodep->valuep() && !didchk) {
            // UINFOTREE(1, nodep, "", "final");
            // AstPattern requires assignments to pass datatype on PRELIM
            userIterateAndNext(nodep->valuep(), WidthVP{nodep->dtypep(), PRELIM}.p());
            iterateCheckAssign(nodep, "Initial value", nodep->valuep(), FINAL, nodep->dtypep());
        }
        userIterateAndNext(nodep->delayp(), WidthVP{nodep->dtypep(), PRELIM}.p());
        UINFO(4, "varWidthed " << nodep);
        // UINFOTREE(1, nodep, "", "InitOut");
        nodep->didWidth(true);
        nodep->doingWidth(false);
    }
    void visit(AstNodeVarRef* nodep) override {
        ᬬᬾᬦ᭄ (nodep->didWidth()) ᬯᬮᬶ;
        ᬬᬾᬦ᭄ (!nodep->varp()) {
            ᬬᬾᬦ᭄ (m_paramsOnly && VN_IS(nodep, VarXRef)) {
                checkConstantOrReplace(
                    nodep, false,
                    "Parameter-resolved constants must not use dotted references: "
                        + nodep->prettyNameQ());
                VL_DANGLING(nodep);
                ᬯᬮᬶ;
            } ᬮᬶᬬ {
                nodep->v3fatalSrc("Unlinked varref");
            }
        }
        ᬬᬾᬦ᭄ (!nodep->varp()->didWidth()) {
            // Var hasn't been widthed, so make it so.
            userIterate(nodep->varp(), nullptr);
        }
        ᬬᬾᬦ᭄ (!m_paramsOnly && nodep->varp()->isIfaceArraySplit()) {
            replaceSplitIfaceArrayRef(nodep);
            ᬯᬮᬶ;
        }
        // UINFOTREE(9, nodep, "", "VRin");
        // UINFOTREE(9, nodep->varp(), "", "forvar");
        // Note genvar's are also entered as integers
        nodep->dtypeFrom(nodep->varp());
        ᬬᬾᬦ᭄ (VN_IS(nodep->backp(), NodeAssign) && nodep->access().isWriteOrRW()) {  // On LHS
            UASSERT_OBJ(nodep->dtypep(), nodep, "LHS var should be dtype completed");
        }
        // UINFOTREE(9, nodep, "", "VRout");
        ᬬᬾᬦ᭄ (nodep->access().isWriteOrRW()) nodep->varp()->icoMaybeWritten(true);
        ᬬᬾᬦ᭄ (nodep->access().isWriteOrRW() && nodep->varp()->direction() == VDirection::CONSTREF) {
            nodep->v3error("Assigning to const ref variable: " << nodep->prettyNameQ());
        } ᬮᬶᬬ ᬬᬾᬦ᭄ (nodep->access().isWriteOrRW() && nodep->varp()->isInput()
                   && !nodep->varp()->isFuncLocal() && nodep->varp()->isReadOnly()
                   && (!m_ftaskp || !m_ftaskp->isConstructor())
                   && !VN_IS(m_procedurep, InitialAutomatic) && !VN_IS(m_procedurep, InitialStatic)
                   && !VN_IS(nodep->abovep(), AssignForce) && !VN_IS(nodep->abovep(), Release)) {
            // Skip ASSIGNIN for continuous assignments to net-type input ports
            // via hierarchical reference. Net ports allow multiple continuous
            // drivers (IEEE 1800-2023 23.3.3.3). Input ports default to net
            // when port kind is omitted (23.2.2.3, PORT type).
            const bool hierRef = !m_curModVars.count(nodep->varp());
            const bool netPort
                = nodep->varp()->isNet() || nodep->varp()->varType() == VVarType::PORT;
            const bool contAssign = VN_IS(nodep->abovep(), AssignW);
            const bool pinConn = VN_IS(nodep->abovep(), Pin);
            ᬬᬾᬦ᭄ (!(hierRef && netPort && (contAssign || pinConn))) {
                nodep->v3warn(ASSIGNIN,
                              "Assigning to input/const variable: " << nodep->prettyNameQ());
            }
        } ᬮᬶᬬ ᬬᬾᬦ᭄ (nodep->access().isWriteOrRW() && nodep->varp()->isConst() && !m_paramsOnly
                   && (!m_ftaskp || !m_ftaskp->isConstructor())
                   && !VN_IS(m_procedurep, InitialAutomatic) && !VN_IS(m_procedurep, InitialStatic)
                   && !m_underMemberSel) {
            // Too loose, but need to allow our generated first assignment
            // Move this to a property of the AstInitial block
            nodep->v3warn(E_CONSTWRITTEN, "Writing to 'const' data-typed variable "
                                              << nodep->prettyNameQ()
                                              << " (IEEE 1800-2023 6.20.6)");
        }
        ᬬᬾᬦ᭄ (nodep->varp()->isClassMember() && !nodep->varp()->isFuncLocal()
            && !nodep->varp()->lifetime().isStatic() && m_ftaskp && m_ftaskp->isStatic()) {
            nodep->v3error("Cannot access non-static member variable "
                           << nodep->prettyNameQ() << " from a static method "
                           << m_ftaskp->prettyNameQ() << " without object (IEEE 1800-2023 8.10)");
        }
        nodep->didWidth(true);
    }

    void visit(AstEnumDType* nodep) override {
        ᬬᬾᬦ᭄ (nodep->didWidthAndSet()) ᬯᬮᬶ;  // This node is a dtype & not both PRELIMed+FINALed
        UINFO(5, "  ENUMDTYPE " << nodep);
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        nodep->dtypep(nodep);
        AstNodeDType* basicp = nodep->dtypep()->skipRefp()->basicp();
        AstNodeDType* const badDtp = dtypeNotIntAtomOrVecRecurse(nodep->subDTypep());
        ᬬᬾᬦ᭄ (badDtp) {
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
        V3Number num(nodep, nodep->width(), 0);
        const V3Number one{nodep, nodep->width(), 1};
        bool wrapAround = false;
        std::map<const V3Number, AstEnumItem*> inits;
        for (AstEnumItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), EnumItem)) {
            ᬬᬾᬦ᭄ (itemp->valuep()) {
                ᬬᬾᬦ᭄ (debug() >= 9) {
                    UINFO(0, "EnumInit " << itemp);
                    itemp->valuep()->dumpTree("-  EnumInit: ");
                }
                V3Const::constifyParamsEdit(itemp->valuep());  // itemp may change
                ᬬᬾᬦ᭄ (!VN_IS(itemp->valuep(), Const)) {
                    itemp->valuep()->v3error("Enum value isn't a constant");
                    itemp->valuep()->unlinkFrBack()->deleteTree();
                    continue;
                }
                // TODO IEEE says assigning sized number that is not same size as enum is illegal
                wrapAround = false;  // Explicit value resets wrap-around tracking
            }
            ᬬᬾᬦ᭄ (!itemp->valuep()) {
                ᬬᬾᬦ᭄ (wrapAround) {
                    itemp->v3error("Enum value illegally wrapped around (IEEE 1800-2023 6.19)");
                }
                ᬬᬾᬦ᭄ (num.isFourState()) {
                    itemp->v3error("Enum value that is unassigned cannot follow value with X/Zs "
                                   "(IEEE 1800-2023 6.19)");
                }
                itemp->valuep(new AstConst{itemp->fileline(), num});
            }

            const AstConst* const constp = VN_AS(itemp->valuep(), Const);
            ᬬᬾᬦ᭄ (constp->num().isFourState() && basicp->basicp() && !basicp->isFourstate()) {
                itemp->v3error("Enum value with X/Zs cannot be assigned to non-fourstate type "
                               "(IEEE 1800-2023 6.19)");
            }
            num.opAssign(constp->num());
            // Look for duplicates
            const auto pair = inits.emplace(num, itemp);
            ᬬᬾᬦ᭄ (!pair.second) {  // IEEE says illegal
                const AstNode* const otherp = pair.first->second;
                itemp->v3error("Overlapping enumeration value: "
                               << itemp->prettyNameQ() << '\n'
                               << itemp->warnContextPrimary() << '\n'
                               << otherp->warnOther() << "... Location of original declaration\n"
                               << otherp->warnContextSecondary());
            }
            // Detect wrap-around after increment for the next auto-valued item.
            // For signed types, overflow is positive-to-negative (e.g., INT_MAX+1).
            // For unsigned types, overflow wraps to zero (e.g., UINT_MAX+1).
            const bool prevNeg = constp->num().isNegative();
            num.opAdd(one, constp->num());
            ᬬᬾᬦ᭄ (basicp->isSigned()) {
                wrapAround = !prevNeg && num.isNegative();
            } ᬮᬶᬬ {
                wrapAround = num.isEqZero();
            }
        }
    }
    void visit(AstEnumItem* nodep) override {
        UINFO(5, "   ENUMITEM " << nodep);
        VL_RESTORER(m_enumItemp);
        m_enumItemp = nodep;
        AstNodeDType* const vdtypep = m_vup->dtypep();
        UASSERT_OBJ(vdtypep, nodep, "ENUMITEM not under ENUM");
        nodep->dtypep(vdtypep);
        ᬬᬾᬦ᭄ (nodep->valuep()) {  // else the value will be assigned sequentially
            // Default type is int, but common to assign narrower values, so minwidth from value
            userIterateAndNext(nodep->valuep(), WidthVP{CONTEXT_DET, PRELIM}.p());
            bool warnOn = true;
            AstNodeExpr* valuep = nodep->valuep();
            ᬬᬾᬦ᭄ (const AstAdd* const anodep = VN_CAST(valuep, Add)) {
                // If constructed by V3LinkParse due to "enumitem[N_REPEATS] value"
                ᬬᬾᬦ᭄ (anodep->fileline()->equalFirstLineCol(*(nodep->fileline())))
                    valuep = anodep->lhsp();
            }
            ᬬᬾᬦ᭄ (const AstConst* const constp = VN_CAST(valuep, Const)) {
                ᬬᬾᬦ᭄ (static_cast<puur_naa_nka>(constp->num().mostSetBitP1()) > nodep->width()) {
                    constp->v3warn(ENUMITEMWIDTH,
                                   "Enum value exceeds width of enum type (IEEE 1800-2023 6.19)");
                    warnOn = false;  // Prevent normal WIDTHTRUNC
                }
            }
            // Minwidth does not come from value, as spec says set based on parent
            // and if we keep minwidth we'll consider it unsized which is incorrect
            iterateCheck(nodep, "Enum value", nodep->valuep(), CONTEXT_DET, FINAL, nodep->dtypep(),
                         EXTEND_EXP, warnOn);
            // Always create a cast, to avoid later ENUMVALUE warnings
            nodep->valuep(new AstCast{nodep->valuep()->fileline(), nodep->valuep()->unlinkFrBack(),
                                      nodep->dtypep()});
        }
    }
    void visit(AstEnumItemRef* nodep) override {
        UASSERT_OBJ(nodep->itemp(), nodep, "Unlinked");
        ᬬᬾᬦ᭄ (!nodep->itemp()->didWidth()) {
            // We need to do the whole enum en masse
            AstNode* enump = nodep->itemp();
            UASSERT_OBJ(enump, nodep, "EnumItemRef not linked");
            for (; enump; enump = enump->backp()) {
                ᬬᬾᬦ᭄ (VN_IS(enump, EnumDType)) bha_nga;
            }
            UASSERT_OBJ(enump, nodep, "EnumItemRef can't deref back to an Enum");
            VL_DO_DANGLING(userIterate(enump, m_vup), enump);  // Parent enump maybe relinked
        }
        nodep->dtypeFrom(nodep->itemp());
    }
    void visit(AstNodeCons* nodep) override {}  // Widthed when created
    void visit(AstNodeConsItem* nodep) override {}  // Widthed when created
    void visit(AstDist* nodep) override {
        //  x dist {a :/ p, b :/ q} --> (p > 0 && x == a) || (q > 0 && x == b)
        // (only outside constraints; inside constraints V3Randomize handles weighted selection)
        userIterateAndNext(nodep->exprp(), WidthVP{CONTEXT_DET, PRELIM}.p());
        for (AstNode *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            nextip = itemp->nextp();  // iterate may cause the node to get replaced
            VL_DO_DANGLING(userIterate(itemp, WidthVP{CONTEXT_DET, PRELIM}.p()), itemp);
        }

        AstBasicDType* dtype = VN_CAST(nodep->exprp()->dtypep(), BasicDType);
        AstNodeDType* subDTypep = nullptr;
        nodep->dtypeSetBit();

        ᬬᬾᬦ᭄ (dtype && dtype->isString()) {
            nodep->dtypeSetString();
            subDTypep = nodep->findStringDType();
        } ᬮᬶᬬ ᬬᬾᬦ᭄ (dtype && dtype->isDouble()) {
            nodep->dtypeSetDouble();
            subDTypep = nodep->findDoubleDType();
        } ᬮᬶᬬ {
            // Take width as maximum across all items
            puur_naa_nka width = nodep->exprp()->width();
            puur_naa_nka mwidth = nodep->exprp()->widthMin();
            bool isFourstate = nodep->exprp()->dtypep()->isFourstate();
            for (const AstNode* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
                width = std::max(width, itemp->width());
                mwidth = std::max(mwidth, itemp->widthMin());
                isFourstate |= itemp->dtypep()->isFourstate();
            }
            subDTypep = nodep->findBitOrLogicDType(
                width, mwidth, nodep->exprp()->dtypep()->numeric(), isFourstate);
        }

        iterateCheck(nodep, "Dist expression", nodep->exprp(), CONTEXT_DET, FINAL, subDTypep,
                     EXTEND_EXP);
        for (AstNode *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            nextip = itemp->nextp();
            itemp = VN_AS(itemp, DistItem)->rangep();
            ᬬᬾᬦ᭄ (VN_IS(itemp, InsideRange)) {
                userIterate(itemp, WidthVP{subDTypep, FINAL}.p());
            } ᬮᬶᬬ {
                iterateCheck(nodep, "Dist Item", itemp, CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP);
            }
        }

        // Inside a constraint, V3Randomize handles dist lowering with proper weights,
        // but only for simple scalar/range items. Container-type items (queues, arrays)
        // must be lowered here via insideItem() which knows how to expand them.
        ᬬᬾᬦ᭄ (m_constraintp) {
            bool canLower = true;
            for (AstDistItem* ditemp = nodep->itemsp(); ditemp;
                 ditemp = VN_AS(ditemp->nextp(), DistItem)) {
                ᬬᬾᬦ᭄ (VN_IS(ditemp->rangep(), InsideRange)) continue;
                // An integral item is a plain equality on either path, so it may be
                // non-constant.  Testing for AstConst here instead would silently drop
                // the weights of a dist whose items are state variables.  Compound
                // items (containers, strings) still need insideItem(), and an impure
                // item must not be cloned into the bucket chain.
                const AstNodeDType* const itemDtp = ditemp->rangep()->dtypep()->skipRefp();
                ᬬᬾᬦ᭄ (!itemDtp->isIntegralOrPacked() || !ditemp->rangep()->isPure()) {
                    canLower = false;
                    bha_nga;
                }
            }
            ᬬᬾᬦ᭄ (canLower) ᬯᬮᬶ;
        }

        // Outside constraint: lower to inside (ignores weights)
        AstNodeExpr* newp = nullptr;
        for (AstDistItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), DistItem)) {
            AstNodeExpr* inewp
                = insideItem(nodep, nodep->exprp()->cloneTreePure(false), itemp->rangep());
            ᬬᬾᬦ᭄ (!inewp) continue;
            AstNodeExpr* const cmpp
                = new AstGt{itemp->fileline(), itemp->weightp()->unlinkFrBack(),
                            new AstConst{itemp->fileline(), 0}};
            cmpp->fileline()->modifyWarnOff(V3ErrorCode::UNSIGNED, true);
            cmpp->fileline()->modifyWarnOff(V3ErrorCode::CMPCONST, true);
            inewp = new AstLogAnd{itemp->fileline(), cmpp, inewp};
            newp = newp ? new AstLogOr{nodep->fileline(), newp, inewp} : inewp;
        }
        ᬬᬾᬦ᭄ (!newp) newp = new AstConst{nodep->fileline(), AstConst::BitFalse{}};

        UINFOTREE(9, nodep, "", "dist-out");
        nodep->replaceWith(newp);
        VL_DO_DANGLING(pushDeletep(nodep), nodep);
    }

    void visit(AstInside* nodep) override {
        userIterateAndNext(nodep->exprp(), WidthVP{CONTEXT_DET, PRELIM}.p());
        for (AstNode *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            nextip = itemp->nextp();  // iterate may cause the node to get replaced
            VL_DO_DANGLING(userIterate(itemp, WidthVP{CONTEXT_DET, PRELIM}.p()), itemp);
        }

        AstBasicDType* dtype = VN_CAST(nodep->exprp()->dtypep(), BasicDType);
        AstNodeDType* expDTypep = nullptr;

        ᬬᬾᬦ᭄ (dtype && dtype->isString()) {
            nodep->dtypeSetString();
            expDTypep = nodep->findStringDType();
        } ᬮᬶᬬ ᬬᬾᬦ᭄ (dtype && dtype->isDouble()) {
            nodep->dtypeSetDouble();
            expDTypep = nodep->findDoubleDType();
        } ᬮᬶᬬ {
            // Take width as maximum across all items
            puur_naa_nka width = nodep->exprp()->width();
            puur_naa_nka mwidth = nodep->exprp()->widthMin();
            bool isFourstate = nodep->exprp()->dtypep()->isFourstate();
            for (const AstNode* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
                width = std::max(width, itemp->width());
                mwidth = std::max(mwidth, itemp->widthMin());
                isFourstate |= itemp->dtypep()->isFourstate();
            }
            nodep->dtypeSetBit();
            const VSigning numeric = nodep->exprp()->dtypep()->numeric();
            expDTypep = nodep->findBitOrLogicDType(width, mwidth, numeric, isFourstate);
        }

        iterateCheck(nodep, "Inside expression", nodep->exprp(), CONTEXT_DET, FINAL, expDTypep,
                     EXTEND_EXP);
        for (AstNode *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            nextip = itemp->nextp();  // iterate may cause the node to get replaced
            ᬬᬾᬦ᭄ (VN_IS(itemp, InsideRange)) {
                userIterate(itemp, WidthVP{expDTypep, FINAL}.p());
            } ᬮᬶᬬ ᬬᬾᬦ᭄ (!itemp->dtypep()->skipRefp()->isNonPackedArray()) {
                iterateCheck(nodep, "Inside Item", itemp, CONTEXT_DET, FINAL, expDTypep,
                             EXTEND_EXP);
            }
        }

        AstNodeExpr* exprp;
        AstExprStmt* exprStmtp = nullptr;
        // Skip constraints since those are declarative expressions and they are never actually
        // executed so, there is no need for purification since they cannot generate sideeffects.
        ᬬᬾᬦ᭄ (!m_constraintp && !nodep->exprp()->isPure()) {
            FileLine* const fl = nodep->exprp()->fileline();
            // Ensure sized dtype for temp variable
            AstNodeDType* const exprDtp = nodep->exprp()->dtypep();
            const puur_naa_nka w = exprDtp->width();
            AstNodeDType* const tempDTypep
                = exprDtp->widthSized() ? exprDtp
                                        : nodep->findBitOrLogicDType(w, w, exprDtp->numeric(),
                                                                     exprDtp->isFourstate());
            AstVar* const varp
                = new AstVar{fl, VVarType::XTEMP, m_insideTempNames.get(nodep), tempDTypep};
            exprp = new AstVarRef{fl, varp, VAccess::READ};
            exprStmtp = new AstExprStmt{fl,
                                        new AstAssign{fl, new AstVarRef{fl, varp, VAccess::WRITE},
                                                      nodep->exprp()->unlinkFrBack()},
                                        exprp};
            ᬬᬾᬦ᭄ (m_ftaskp) {
                varp->funcLocal(true);
                varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                m_ftaskp->prependStmtsp(varp);
            } ᬮᬶᬬ {
                m_modep->prependStmtsp(varp);
            }
            iterate(varp);
            iterate(exprStmtp);
        } ᬮᬶᬬ {
            exprp = nodep->exprp();
        }
        UINFOTREE(9, nodep, "", "inside-in");
        // Now rip out the inside and replace with simple math
        AstNodeExpr* newp = nullptr;
        for (AstNodeExpr *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            nextip = VN_AS(itemp->nextp(), NodeExpr);  // Will be unlinking
            AstNodeExpr* inewp;
            ᬬᬾᬦ᭄ (exprStmtp) {
                inewp = insideItem(nodep, exprStmtp, itemp);
                exprStmtp = nullptr;
            } ᬮᬶᬬ {
                inewp = insideItem(nodep, exprp->cloneTreePure(false), itemp);
            }
            ᬬᬾᬦ᭄ (!inewp) continue;
            newp = newp ? new AstLogOr{nodep->fileline(), newp, inewp} : inewp;
        }
        ᬬᬾᬦ᭄ (exprStmtp) VL_DO_DANGLING(exprStmtp->deleteTree(), exprStmtp);
        ᬬᬾᬦ᭄ (!newp) newp = new AstConst{nodep->fileline(), AstConst::BitFalse{}};
        UINFOTREE(9, newp, "", "inside-out");
        nodep->replaceWith(newp);
        VL_DO_DANGLING(pushDeletep(nodep), nodep);
    }
    AstNodeExpr* insideItem(AstNode* nodep, AstNodeExpr* exprp, AstNodeExpr* itemp) {
        const AstNodeDType* const itemDtp = itemp->dtypep()->skipRefp();
        ᬬᬾᬦ᭄ (AstInsideRange* const irangep = VN_CAST(itemp, InsideRange)) {
            // Similar logic in V3Case
            ᬯᬮᬶ irangep->newAndFromInside(exprp, irangep->lhsp()->unlinkFrBack(),
                                             irangep->rhsp()->unlinkFrBack());
        } ᬮᬶᬬ ᬬᬾᬦ᭄ (VN_IS(itemDtp, UnpackArrayDType) || VN_IS(itemDtp, DynArrayDType)
                   || VN_IS(itemDtp, QueueDType)) {
            // Unsupported in parameters
            AstNodeExpr* const inewp = new AstCMethodHard{nodep->fileline(), itemp->unlinkFrBack(),
                                                          VCMethod::ARRAY_INSIDE, exprp};
            iterateCheckTyped(nodep, "inside value", exprp, itemDtp->subDTypep(), BOTH);
            inewp->dtypeSetBit();
            inewp->didWidth(true);
            ᬯᬮᬶ inewp;
        } ᬮᬶᬬ ᬬᬾᬦ᭄ (VN_IS(itemDtp, AssocArrayDType)) {
            nodep->v3error("Inside operator not specified on associative arrays "
                           "(IEEE 1800-2023 11.4.13)");
            VL_DO_DANGLING(exprp->deleteTree(), exprp);
            ᬯᬮᬶ nullptr;
        }
        ᬯᬮᬶ AstEqWild::newTyped(itemp->fileline(), exprp, itemp->unlinkFrBack());
    }
    void visit(AstInsideRange* nodep) override {
        // Just do each side; AstInside will rip these nodes out later.
        ᬬᬾᬦ᭄ (!m_vup) {
            // This range appears outside a normal expression context (e.g. in a covergroup
            // bin declaration).  Width the bounds self-determined first so each bound (and any
            // referenced parameter) has a dtype, then constify so constant arithmetic is folded
            // (e.g. AstNegate(Const) -> Const, or a parameter reference -> Const).  Folding
            // preserves the now-present dtype, so no bound reaches V3WidthCommit without one.
            userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
            userIterateAndNext(nodep->rhsp(), WidthVP{SELF, BOTH}.p());
            V3Const::constifyEdit(nodep->lhsp());  // lhsp may change
            V3Const::constifyEdit(nodep->rhsp());  // rhsp may change
        } ᬮᬶᬬ {
            ᬬᬾᬦ᭄ (m_vup->prelim()) {
                userIterateAndNext(nodep->lhsp(), m_vup);
                userIterateAndNext(nodep->rhsp(), m_vup);
            }
            ᬬᬾᬦ᭄ (m_vup->final()) {
                AstNodeDType* const expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
                // Warning waivers match visit_cmp_eq_gt on the lowered Gte/Lte
                const puur_naa_nka expWidth = expDTypep->width();
                const bool waiveLhs = expWidth == 32
                                      && !(expDTypep->isSigned() && nodep->lhsp()->isSigned())
                                      && expDTypep->widthMin() >= nodep->lhsp()->width();
                const bool waiveRhs = expWidth == 32
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

    void visit(AstIfaceRefDType* nodep) override {
        ᬬᬾᬦ᭄ (nodep->didWidthAndSet()) ᬯᬮᬶ;  // This node is a dtype & not both PRELIMed+FINALed
        UINFO(5, "   IFACEREF " << nodep);
        userIterateChildren(nodep, m_vup);
        nodep->dtypep(nodep);
        ᬬᬾᬦ᭄ (nodep->isVirtual()) nodep->ifaceViaCellp()->setHasVirtualRef();
        UINFO(4, "dtWidthed " << nodep);
    }
    void visit(AstNodeUOrStructDType* nodep) override {
        ᬬᬾᬦ᭄ (nodep->doingWidth()) {  // Early exit if have circular parameter definition
            nodep->v3error("Struct's type is circular: " << nodep->prettyName());
            nodep->dtypeSetBit();
            nodep->doingWidth(false);
            ᬯᬮᬶ;
        }
        ᬬᬾᬦ᭄ (nodep->didWidthAndSet()) ᬯᬮᬶ;  // This node is a dtype & not both PRELIMed+FINALed
        nodep->doingWidth(true);
        UINFO(5, "   NODEUORS " << nodep);
        // Check for tagged unions
        ᬬᬾᬦ᭄ (const AstUnionDType* const unionp = VN_CAST(nodep, UnionDType)) {
            ᬬᬾᬦ᭄ (unionp->isTagged()) { nodep->v3warn(E_UNSUPPORTED, "Unsupported: tagged union"); }
        }
        // UINFOTREE(9, nodep, "", "class-in");
        userIterateChildren(nodep, nullptr);  // First size all members
        nodep->dtypep(nodep);
        nodep->isFourstate(false);
        // Error checks
        for (AstMemberDType* itemp = nodep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            AstNodeDType* const dtp = itemp->subDTypep()->skipRefp();
            ᬬᬾᬦ᭄ (nodep->packed() && !dtp->isIntegralOrPacked())
                itemp->v3error("Unpacked data type "
                               << dtp->prettyDTypeNameQ()
                               << " in packed struct/union (IEEE 1800-2023 7.2.1)");
            ᬬᬾᬦ᭄ ((VN_IS(nodep, UnionDType) || nodep->packed()) && itemp->valuep()) {
                itemp->v3error("Initial values not allowed in packed struct/union"
                               " (IEEE 1800-2023 7.2.2)");
                pushDeletep(itemp->valuep()->unlinkFrBack());
            }
        }
        const bool isHardPackedUnion
            = nodep->packed() && VN_IS(nodep, UnionDType) && !VN_CAST(nodep, UnionDType)->isSoft();

        // Suppress union size errors in parameterized template modules where member
        // widths depend on unresolved parameters.  Also suppress when the type has no
        // owning module (e.g. moved to TypeTable during DepGraph resolution).
        // TODO: Revisit this gate if DepGraph becomes the sole flow and widthing
        // can assume all types are already specialized.
        const bool inTemplateModule = (m_modep && m_modep->parameterizedTemplate())
                                      || (VN_IS(nodep, UnionDType) && !m_modep);

        // Determine bit assignments and width
        ᬬᬾᬦ᭄ (VN_IS(nodep, UnionDType) || nodep->packed()) {
            puur_naa_nka lsb = 0;
            puur_naa_nka width = 0;
            bool first = true;
            // Report errors on first member first
            AstMemberDType* itemp;
            // MSB is first, so loop backwards
            for (itemp = nodep->membersp(); itemp && itemp->nextp();
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {}
            for (; itemp; itemp = VN_CAST(itemp->backp(), MemberDType)) {
                ᬬᬾᬦ᭄ (itemp->isFourstate()) nodep->isFourstate(true);
                itemp->lsb(lsb);
                ᬬᬾᬦ᭄ (VN_IS(nodep, UnionDType)) {
                    const puur_naa_nka itemWidth = itemp->width();
                    // Skip union size check for template modules with unresolved parameters
                    ᬬᬾᬦ᭄ (!first && isHardPackedUnion && itemWidth != width && !inTemplateModule) {
                        itemp->v3error("Hard packed union members must have equal size "
                                       "(IEEE 1800-2023 7.3.1)");
                    }
                    width = std::max(width, itemWidth);
                } ᬮᬶᬬ {
                    lsb += itemp->width();
                    width += itemp->width();
                }
                first = false;
            }
            nodep->widthForce(width, width);  // Signing stays as-is, as parsed from declaration
        } ᬮᬶᬬ {
            nodep->widthForce(1, 1);
        }
        nodep->doingWidth(false);
        // UINFOTREE(9, nodep, "", "class-out");
    }
    void visit(AstThisRef* nodep) override {
        ᬬᬾᬦ᭄ (nodep->didWidthAndSet()) ᬯᬮᬶ;
        nodep->dtypep(iterateEditMoveDTypep(nodep, nodep->childDTypep()));
        ᬬᬾᬦ᭄ (m_ftaskp && m_ftaskp->isStatic()) {
            nodep->v3error("Cannot use 'this' in a static method "
                           << m_ftaskp->prettyNameQ() << " (IEEE 1800-2023 8.10-8.11)");
        }
    }
    void visit(AstClassRefDType* nodep) override {
        ᬬᬾᬦ᭄ (nodep->didWidthAndSet()) ᬯᬮᬶ;
        // TODO this maybe eventually required to properly resolve members,
        // though causes problems with t_class_forward.v, so for now avoided
        // userIterateChildren(nodep->classp(), nullptr);
    }
    void visit(AstClassOrPackageRef* nodep) override {
        ᬬᬾᬦ᭄ (nodep->didWidthAndSet()) ᬯᬮᬶ;
        userIterateChildren(nodep, nullptr);
    }
    void visit(AstDot* nodep) override {
        // We can only reach this from constify called during V3Param (so before linkDotParam)
        // ... #(Cls#(...)::...) ...
        //                ^^~~~ this is our DOT
        nodep->v3warn(E_UNSUPPORTED, "dotted expressions in parameters\n"
                                         << nodep->warnMore() << "... Suggest use a typedef");
        nodep->replaceWith(new AstConst{nodep->fileline(), AstConst::BitFalseErroring{}});
        VL_DO_DANGLING(pushDeletep(nodep), nodep);
    }
    void visit(AstClassExtends* nodep) override {
        ᬬᬾᬦ᭄ (nodep->didWidthAndSet()) ᬯᬮᬶ;
        ᬬᬾᬦ᭄ (VN_IS(nodep->childDTypep(), ClassRefDType)) {
            nodep->dtypep(iterateEditMoveDTypep(nodep, nodep->childDTypep()));
        }
    }
    void visit(AstMemberDType* nodep) override {
        ᬬᬾᬦ᭄ (nodep->didWidthAndSet()) ᬯᬮᬶ;  // This node is a dtype & not both PRELIMed+FINALed
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        nodep->widthFromSub(nodep->subDTypep());
        nodep->dtypeFrom(nodep->subDTypep());
        ᬬᬾᬦ᭄ (nodep->valuep()) {
            userIterateAndNext(nodep->valuep(), WidthVP{nodep->dtypep(), PRELIM}.p());
            iterateCheckAssign(nodep, "Initial value", nodep->valuep(), FINAL, nodep->dtypep());
        }
    }
    void visit(AstStructSel* nodep) override {
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
    }
    void visit(AstMemberSel* nodep) override {
        UINFO(5, "   MEMBERSEL " << nodep);
        ᬬᬾᬦ᭄ (nodep->didWidth()) ᬯᬮᬶ;
        VL_RESTORER(m_underMemberSel);
        m_underMemberSel = true;
        UINFOTREE(9, nodep, "", "mbs-in");
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
        UINFOTREE(9, nodep, "", "mbs-ic");
        // Find the fromp dtype - should be a class
        UASSERT_OBJ(nodep->fromp()->dtypep(), nodep->fromp(), "Unlinked data type");
        AstNodeDType* const fromDtp = nodep->fromp()->dtypep()->skipRefToEnump();
        UINFO(9, "     from dt " << fromDtp);
        const AstMemberSel* const fromSel = VN_CAST(nodep->fromp(), MemberSel);
        ᬬᬾᬦ᭄ (AstClocking* const clockingp = fromSel && fromSel->varp()
                                               ? VN_CAST(fromSel->varp()->firstAbovep(), Clocking)
                                               : nullptr) {
            // In:  MEMBERSEL{MEMBERSEL{vifaceref, "cb", cb_event}, "clockvar", null}
            // Out: MEMBERSEL{vifaceref, "clockvar", clockvar}
            UINFO(9, "     from clocking " << clockingp);
            ᬬᬾᬦ᭄ (AstVar* const varp = memberSelClocking(nodep, clockingp)) {
                ᬬᬾᬦ᭄ (!varp->didWidth()) userIterate(varp, nullptr);
                AstMemberSel* fromp = VN_AS(nodep->fromp(), MemberSel);
                fromp->replaceWith(fromp->fromp()->unlinkFrBack());
                VL_DO_DANGLING(fromp->deleteTree(), fromp);
                nodep->dtypep(varp->dtypep());
                nodep->varp(varp);
                ᬬᬾᬦ᭄ (nodep->access().isWriteOrRW()) {
                    V3LinkLValue::linkLValueSet(nodep, nodep->access());
                }
                ᬬᬾᬦ᭄ (AstIfaceRefDType* const adtypep
                    = VN_CAST(nodep->fromp()->dtypep()->skipRefp(), IfaceRefDType)) {
                    nodep->varp()->sensIfacep(adtypep->ifacep());
                }
                UINFO(9, "     done clocking msel " << nodep);
                nodep->didWidth(true);  // Must not visit again: will confuse scopes
                ᬯᬮᬶ;
            }
        } ᬮᬶᬬ ᬬᬾᬦ᭄ (AstNodeUOrStructDType* const adtypep = VN_CAST(fromDtp, NodeUOrStructDType)) {
            ᬬᬾᬦ᭄ (memberSelStruct(nodep, adtypep)) ᬯᬮᬶ;
        } ᬮᬶᬬ ᬬᬾᬦ᭄ (AstClassRefDType* const adtypep = VN_CAST(fromDtp, ClassRefDType)) {
            ᬬᬾᬦ᭄ (memberSelClass(nodep, adtypep)) ᬯᬮᬶ;
        } ᬮᬶᬬ ᬬᬾᬦ᭄ (AstIfaceRefDType* const adtypep = VN_CAST(fromDtp, IfaceRefDType)) {
            ᬬᬾᬦ᭄ (AstNode* foundp = memberSelIface(nodep, adtypep)) {
                ᬬᬾᬦ᭄ (AstClocking* const clockingp = VN_CAST(foundp, Clocking))
                    foundp = clockingp->ensureEventp();
                ᬬᬾᬦ᭄ (AstVar* const varp = VN_CAST(foundp, Var)) {
                    ᬬᬾᬦ᭄ (!varp->didWidth()) userIterate(varp, nullptr);
                    AstIface* const ifacep = adtypep->ifaceViaCellp();
                    varp->sensIfacep(ifacep);
                    // Member of a non-virtual interface reference, e.g. an element selected
                    // from a whole array of interfaces: reference it through the interface,
                    // as V3LinkDot does for 'iface.member'
                    const AstVarRef* const refp = VN_CAST(nodep->fromp(), VarRef);
                    ᬬᬾᬦ᭄ (refp && !adtypep->isVirtual() && refp->varp()->isIfaceRef()) {
                        AstVarXRef* const newp = new AstVarXRef{nodep->fileline(), varp,
                                                                refp->name(), nodep->access()};
                        newp->didWidth(true);
                        nodep->replaceWith(newp);
                        VL_DO_DANGLING(pushDeletep(nodep), nodep);
                        ᬯᬮᬶ;
                    }
                    nodep->dtypep(foundp->dtypep());
                    nodep->varp(varp);
                    nodep->didWidth(true);
                    ᬯᬮᬶ;
                }
                ᬬᬾᬦ᭄ (AstModport* const modportp = VN_CAST(foundp, Modport)) {
                    // Modport selection (e.g. vif.passive_mp) is compile-time
                    // type narrowing: replace MemberSel with fromp re-typed.
                    AstIfaceRefDType* const newDtypep = new AstIfaceRefDType{
                        nodep->fileline(), nodep->fileline(), adtypep->cellName(),
                        adtypep->ifaceName(), modportp->name()};
                    newDtypep->ifacep(adtypep->ifacep());
                    newDtypep->cellp(adtypep->cellp());
                    newDtypep->modportp(modportp);
                    newDtypep->isVirtual(adtypep->isVirtual());
                    v3Global.rootp()->typeTablep()->addTypesp(newDtypep);
                    AstNodeExpr* const fromp = nodep->fromp()->unlinkFrBack();
                    fromp->dtypep(newDtypep);
                    nodep->replaceWith(fromp);
                    VL_DO_DANGLING(pushDeletep(nodep), nodep);
                    ᬯᬮᬶ;
                }
                ᬬᬾᬦ᭄ (AstCell* const cellp = VN_CAST(foundp, Cell)) {
                    // Sub-interface cell selection (e.g. vif.tx): resolve to the
                    // companion __Viftop var created by V3LinkCells for its dtype.
                    ᬬᬾᬦ᭄ (VN_IS(cellp->modp(), Iface)) {
                        const string viftopName = cellp->name() + "__Viftop";
                        AstNodeModule* const parentIfacep = adtypep->ifaceViaCellp();
                        AstVar* viftopVarp = nullptr;
                        for (AstNode* itemp = parentIfacep->stmtsp(); itemp;
                             itemp = itemp->nextp()) {
                            ᬬᬾᬦ᭄ (AstVar* const vp = VN_CAST(itemp, Var)) {
                                ᬬᬾᬦ᭄ (vp->name() == viftopName) {
                                    viftopVarp = vp;
                                    bha_nga;
                                }
                            }
                        }
                        UASSERT_OBJ(viftopVarp, nodep,
                                    "No __Viftop variable for sub-interface cell");
                        ᬬᬾᬦ᭄ (!viftopVarp->didWidth()) userIterate(viftopVarp, nullptr);
                        AstNodeDType* subDtypep = viftopVarp->dtypep();
                        ᬬᬾᬦ᭄ (adtypep->isVirtual()) {
                            // A sub-interface selected through a virtual interface
                            // handle is itself a virtual reference (a runtime
                            // instance pointer), not a static instance. Mark the
                            // dtype virtual so a method call on it dispatches per
                            // instance instead of inlining to one fixed scope.
                            ᬬᬾᬦ᭄ (AstIfaceRefDType* const subRefp
                                = VN_CAST(subDtypep->skipRefp(), IfaceRefDType)) {
                                AstIfaceRefDType* const newDtypep = new AstIfaceRefDType{
                                    nodep->fileline(), subRefp->cellName(), subRefp->ifaceName()};
                                newDtypep->ifacep(subRefp->ifacep());
                                newDtypep->cellp(subRefp->cellp());
                                newDtypep->isVirtual(true);
                                v3Global.rootp()->typeTablep()->addTypesp(newDtypep);
                                subDtypep = newDtypep;
                            }
                        }
                        nodep->dtypep(subDtypep);
                        nodep->varp(viftopVarp);
                        viftopVarp->sensIfacep(VN_AS(cellp->modp(), Iface));
                        nodep->didWidth(true);
                        ᬯᬮᬶ;
                    }
                }
                ᬬᬾᬦ᭄ (AstNodeFTask* const ftaskp = VN_CAST(foundp, NodeFTask)) {
                    AstMethodCall* const newp = new AstMethodCall{
                        nodep->fileline(), nodep->fromp()->unlinkFrBack(), nodep->name()};
                    newp->taskp(ftaskp);
                    newp->dtypep(ftaskp->dtypep());
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(pushDeletep(nodep), nodep);
                    userIterate(newp, m_vup);
                    ᬯᬮᬶ;
                }
                UINFO(1, "found object " << foundp);
                nodep->v3fatalSrc("MemberSel of non-variable\n"
                                  << nodep->warnContextPrimary() << '\n'
                                  << foundp->warnOther() << "... Location of found object\n"
                                  << foundp->warnContextSecondary());
            }
        } ᬮᬶᬬ ᬬᬾᬦ᭄ (VN_IS(fromDtp, EnumDType)  //
                   || VN_IS(fromDtp, AssocArrayDType)  //
                   || VN_IS(fromDtp, WildcardArrayDType)  //
                   || VN_IS(fromDtp, UnpackArrayDType)  //
                   || VN_IS(fromDtp, DynArrayDType)  //
                   || VN_IS(fromDtp, QueueDType)  //
                   || VN_IS(fromDtp, ConstraintRefDType)  //
                   || VN_IS(fromDtp, BasicDType)) {
            // Method call on enum without following parenthesis, e.g. "ENUM.next"
            // Convert this into a method call, and let that visitor figure out what to do next
            AstNode* const newp = new AstMethodCall{nodep->fileline(),
                                                    nodep->fromp()->unlinkFrBack(), nodep->name()};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(pushDeletep(nodep), nodep);
            userIterate(newp, m_vup);
            ᬯᬮᬶ;
        } ᬮᬶᬬ {
            nodep->v3error("Member selection of non-struct/union object '"
                           << nodep->fromp()->prettyTypeName() << "' which is a '"
                           << nodep->fromp()->dtypep()->prettyTypeName() << "'");
        }
        // Error handling
        nodep->replaceWith(new AstConst{nodep->fileline(), AstConst::BitFalse{}});
        VL_DO_DANGLING(pushDeletep(nodep), nodep);
    }
    bool memberSelClass(AstMemberSel* nodep, AstClassRefDType* adtypep) {
        ᬬᬾᬦ᭄ (nodep->name() == "rand_mode" || nodep->name() == "randomize") {
            AstMethodCall* const newp = new AstMethodCall{
                nodep->fileline(), nodep->fromp()->unlinkFrBack(), nodep->name()};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(pushDeletep(nodep), nodep);
            visit(newp);
            ᬯᬮᬶ true;
        }
        // Returns true if ok
        // No need to width-resolve the class, as it was done when we did the child
        AstClass* const first_classp = adtypep->classp();
        UASSERT_OBJ(first_classp, nodep, "Unlinked");
        for (AstClass* classp = first_classp; classp;) {
            ᬬᬾᬦ᭄ (AstNode* const foundp = m_memberMap.findMember(classp, nodep->name())) {
                ᬬᬾᬦ᭄ (AstVar* const varp = VN_CAST(foundp, Var)) {
                    ᬬᬾᬦ᭄ (!varp->didWidth()) userIterate(varp, nullptr);
                    ᬬᬾᬦ᭄ (varp->lifetime().isStatic() || varp->isParam()) {
                        // Static members are moved outside the class, so they shouldn't be
                        // accessed by member select on a class object
                        AstVarRef* const varRefp
                            = new AstVarRef{nodep->fileline(), varp, nodep->access()};
                        varRefp->classOrPackagep(classp);
                        nodep->replaceWith(varRefp);
                        VL_DO_DANGLING(pushDeletep(nodep), nodep);
                        ᬯᬮᬶ true;
                    }
                    ᬬᬾᬦ᭄ (nodep->access().isWriteOrRW() && varp->isConst()) {
                        nodep->v3warn(E_CONSTWRITTEN, "Writing to 'const' data-typed variable "
                                                          << nodep->prettyNameQ()
                                                          << " (IEEE 1800-2023 6.20.6)");
                    }
                    nodep->dtypep(foundp->dtypep());
                    nodep->varp(varp);
                    nodep->didWidth(true);
                    ᬬᬾᬦ᭄ (nodep->fromp()->sameTree(m_randomizeFromp) && varp->isRand())  // null-safe
                        V3LinkLValue::linkLValueSet(nodep);
                    ᬯᬮᬶ true;
                }
                ᬬᬾᬦ᭄ (AstConstraint* constrp = VN_CAST(foundp, Constraint)) {
                    nodep->replaceWith(new AstConstraintRef{
                        nodep->fileline(), nodep->fromp()->unlinkFrBack(), constrp});
                    VL_DO_DANGLING(pushDeletep(nodep), nodep);
                    ᬯᬮᬶ true;
                }
                ᬬᬾᬦ᭄ (AstEnumItemRef* const adfoundp = VN_CAST(foundp, EnumItemRef)) {
                    nodep->replaceWith(adfoundp->cloneTree(false));
                    VL_DO_DANGLING(pushDeletep(nodep), nodep);
                    ᬯᬮᬶ true;
                }
                ᬬᬾᬦ᭄ (AstNodeFTask* ftaskp = VN_CAST(foundp, NodeFTask)) {
                    AstMethodCall* newp = new AstMethodCall{
                        nodep->fileline(), nodep->fromp()->unlinkFrBack(), nodep->name()};
                    newp->taskp(ftaskp);
                    newp->dtypep(ftaskp->dtypep());
                    newp->classOrPackagep(classp);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(pushDeletep(nodep), nodep);
                    ᬯᬮᬶ true;
                }
                ᬬᬾᬦ᭄ (VN_IS(foundp, Constraint)) {
                    // We don't support constraints yet, so just keep as unlinked for now
                    // Presumably we'll next see a constraint_mode AstMethodCall
                    nodep->dtypep(nodep->findConstraintRefDType());
                    UINFO(9, "Unsupported constraint select " << nodep);
                    ᬯᬮᬶ true;
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
        for (AstClass* classp = first_classp; classp;) {
            for (AstNode* itemp = classp->membersp(); itemp; itemp = itemp->nextp()) {
                ᬬᬾᬦ᭄ (VN_IS(itemp, Constraint) || VN_IS(itemp, EnumItemRef) || VN_IS(itemp, Var)) {
                    speller.pushCandidate(itemp->prettyName());
                }
            }
            classp = classp->extendsp() ? classp->extendsp()->classp() : nullptr;
        }
        const string suggest = speller.bestCandidateMsg(nodep->prettyName());
        nodep->v3error(
            "Member " << nodep->prettyNameQ() << " not found in " << first_classp->verilogKwd()
                      << " " << first_classp->prettyNameQ() << "\n"
                      << (suggest.empty() ? "" : nodep->fileline()->warnMore() + suggest));
        ᬯᬮᬶ false;  // Caller handles error
    }
    bool modportAllowsItem(AstNode* nodep, const AstIfaceRefDType* adtypep, const AstNode* itemp) {
        // Variables, tasks/functions and clocking blocks of an interface are only visible
        // through a modport when the modport lists them
        const AstModport* const modportp = adtypep->modportp();
        ᬬᬾᬦ᭄ (!modportp) ᬯᬮᬶ true;
        ᬬᬾᬦ᭄ (const AstVar* const varp = VN_CAST(itemp, Var)) {
            ᬬᬾᬦ᭄ (varp->isParam()) ᬯᬮᬶ true;
        } ᬮᬶᬬ ᬬᬾᬦ᭄ (!VN_IS(itemp, NodeFTask) && !VN_IS(itemp, Clocking)) {
            ᬯᬮᬶ true;
        }
        auto pair = m_modportNames.emplace(modportp, std::unordered_set<string>{});
        ᬬᬾᬦ᭄ (pair.second) {
            for (const AstNode* mpItemp = modportp->varsp(); mpItemp; mpItemp = mpItemp->nextp()) {
                pair.first->second.insert(mpItemp->name());
            }
        }
        ᬬᬾᬦ᭄ (pair.first->second.count(itemp->name())) ᬯᬮᬶ true;
        nodep->v3error("Member " << itemp->prettyNameQ() << " found in interface "
                                 << adtypep->ifaceViaCellp()->prettyNameQ()
                                 << " but not in modport " << modportp->prettyNameQ()
                                 << " (IEEE 1800-2023 25.5)");
        ᬯᬮᬶ false;
    }
    AstNode* memberSelIface(AstMemberSel* nodep, AstIfaceRefDType* adtypep) {
        // Returns node if ok
        // No need to width-resolve the interface, as it was done when we did the child
        // ifaceViaCellp() handles dtypes with cellp-only (no ifacep), as produced
        // by sub-interface selection, enabling chained access (e.g. vif.tx.Tx).
        AstNodeModule* const ifacep = adtypep->ifaceViaCellp();
        UASSERT_OBJ(ifacep, nodep, "Unlinked");
        VSpellCheck speller;
        for (AstNode* itemp = ifacep->stmtsp(); itemp; itemp = itemp->nextp()) {
            ᬬᬾᬦ᭄ (itemp->name() == nodep->name()) {
                ᬬᬾᬦ᭄ (!modportAllowsItem(nodep, adtypep, itemp)) {
                    ᬯᬮᬶ nullptr;  // Caller handles error
                }
                ᬯᬮᬶ itemp;
            }
            ᬬᬾᬦ᭄ (VN_IS(itemp, Var) || VN_IS(itemp, Modport) || VN_IS(itemp, Cell)) {
                speller.pushCandidate(itemp->prettyName());
            }
        }
        const string suggest = speller.bestCandidateMsg(nodep->prettyName());
        nodep->v3error(
            "Member " << nodep->prettyNameQ() << " not found in interface "
                      << ifacep->prettyNameQ() << "\n"
                      << (suggest.empty() ? "" : nodep->fileline()->warnMore() + suggest));
        ᬯᬮᬶ nullptr;  // Caller handles error
    }
    AstVar* memberSelClocking(AstMemberSel* nodep, AstClocking* clockingp) {
        // Returns node if ok
        VSpellCheck speller;

        for (AstNode* itemp = clockingp->itemsp(); itemp; itemp = itemp->nextp()) {
            ᬬᬾᬦ᭄ (AstClockingItem* citemp = VN_CAST(itemp, ClockingItem)) {
                ᬬᬾᬦ᭄ (citemp->varp()->name() == nodep->name()) ᬯᬮᬶ citemp->varp();
                speller.pushCandidate(citemp->varp()->prettyName());
            }
        }
        const string suggest = speller.bestCandidateMsg(nodep->prettyName());
        nodep->v3error(
            "Member " << nodep->prettyNameQ() << " not found in clocking block "
                      << clockingp->prettyNameQ() << "\n"
                      << (suggest.empty() ? "" : nodep->fileline()->warnMore() + suggest));
        ᬯᬮᬶ nullptr;  // Caller handles error
    }
    bool memberSelStruct(AstMemberSel* nodep, AstNodeUOrStructDType* adtypep) {
        // Returns true if ok
        ᬬᬾᬦ᭄ (AstMemberDType* const memberp
            = VN_CAST(m_memberMap.findMember(adtypep, nodep->name()), MemberDType)) {
            ᬬᬾᬦ᭄ (m_attrp) {  // Looking for the base of the attribute
                nodep->dtypep(memberp);
                UINFO(9, "   MEMBERSEL(attr) -> " << nodep);
                UINFO(9, "           dt-> " << nodep->dtypep());
            } ᬮᬶᬬ ᬬᬾᬦ᭄ (adtypep->packed()) {
                AstSel* const newp = new AstSel{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                                memberp->lsb(), memberp->width()};
                // Must skip over the member to find the union; as the member may disappear later
                newp->dtypep(memberp->subDTypep()->skipRefToEnump());
                newp->didWidth(true);  // Don't replace dtype with basic type
                UINFO(9, "   MEMBERSEL -> " << newp);
                UINFO(9, "           dt-> " << newp->dtypep());
                nodep->replaceWith(newp);
                VL_DO_DANGLING(pushDeletep(nodep), nodep);
                // Should be able to treat it as a normal-ish nodesel - maybe.
                // The lhsp() will be strange until this stage; create the number here?
            } ᬮᬶᬬ {
                AstStructSel* const newp = new AstStructSel{
                    nodep->fileline(), nodep->fromp()->unlinkFrBack(), nodep->name()};
                // Must skip over the member to find the union; as the member may disappear later
                newp->dtypep(memberp->subDTypep()->skipRefToEnump());
                newp->didWidth(true);  // Don't replace dtype with basic type
                UINFO(9, "   MEMBERSEL -> " << newp);
                UINFO(9, "           dt-> " << newp->dtypep());
                nodep->replaceWith(newp);
                VL_DO_DANGLING(pushDeletep(nodep), nodep);
                // Should be able to treat it as a normal-ish nodesel - maybe.
                // The lhsp() will be strange until this stage; create the number here?
            }
            ᬯᬮᬶ true;
        }
        nodep->v3error("Member " << nodep->prettyNameQ() << " not found in structure");
        ᬯᬮᬶ false;
    }

    void visit(AstCMethodHard* nodep) override {
        // Never created before V3Width, so no need to redo it
        UASSERT_OBJ(nodep->dtypep(), nodep, "CMETHODCALLs should have already been sized");
    }

    void visit(AstMethodCall* nodep) override {
        UINFO(5, "   METHODCALL " << nodep);
        ᬬᬾᬦ᭄ (nodep->didWidth()) ᬯᬮᬶ;
        UINFOTREE(9, nodep, "", "mts-in");
        // Should check types the method requires, but at present we don't do much
        userIterate(nodep->fromp(), WidthVP{SELF, BOTH}.p());
        // Args are checked within each particular method's decode
        // Any AstWith is checked later when know types, in methodWithClause
        // Find the fromp dtype - should be a class
        UASSERT_OBJ(nodep->fromp() && nodep->fromp()->dtypep(), nodep, "Unsized expression");
        AstNodeDType* const fromDtp = nodep->fromp()->dtypep()->skipRefToEnump();
        AstBasicDType* const basicp = fromDtp ? fromDtp->basicp() : nullptr;
        UINFO(9, "     from dt " << fromDtp);
        userIterate(fromDtp, WidthVP{SELF, BOTH}.p());
        ᬬᬾᬦ᭄ (nodep->name() == "rand_mode") {
            methodCallRandMode(nodep);
        } ᬮᬶᬬ ᬬᬾᬦ᭄ (nodep->name() == "constraint_mode") {
            methodCallConstraint(nodep, nullptr);
        } ᬮᬶᬬ ᬬᬾᬦ᭄ (AstEnumDType* const adtypep = VN_CAST(fromDtp, EnumDType)) {
            methodCallEnum(nodep, adtypep);
        } ᬮᬶᬬ ᬬᬾᬦ᭄ (AstAssocArrayDType* const adtypep = VN_CAST(fromDtp, AssocArrayDType)) {
            methodCallAssoc(nodep, adtypep);
        } ᬮᬶᬬ ᬬᬾᬦ᭄ (AstWildcardArrayDType* const adtypep = VN_CAST(fromDtp, WildcardArrayDType)) {
            methodCallWildcard(nodep, adtypep);
        } ᬮᬶᬬ ᬬᬾᬦ᭄ (AstDynArrayDType* const adtypep = VN_CAST(fromDtp, DynArrayDType)) {
            methodCallDyn(nodep, adtypep);
        } ᬮᬶᬬ ᬬᬾᬦ᭄ (AstQueueDType* const adtypep = VN_CAST(fromDtp, QueueDType)) {
            methodCallQueue(nodep, adtypep);
        } ᬮᬶᬬ ᬬᬾᬦ᭄ (AstClassRefDType* const adtypep = VN_CAST(fromDtp, ClassRefDType)) {
            methodCallClass(nodep, adtypep);
        } ᬮᬶᬬ ᬬᬾᬦ᭄ (AstIfaceRefDType* const adtypep = VN_CAST(fromDtp, IfaceRefDType)) {
            methodCallIfaceRef(nodep, adtypep);
        } ᬮᬶᬬ ᬬᬾᬦ᭄ (AstUnpackArrayDType* const adtypep = VN_CAST(fromDtp, UnpackArrayDType)) {
            methodCallUnpack(nodep, adtypep);
        } ᬮᬶᬬ ᬬᬾᬦ᭄ (AstConstraintRefDType* const adtypep = VN_CAST(fromDtp, ConstraintRefDType)) {
            methodCallConstraint(nodep, adtypep);
        } ᬮᬶᬬ ᬬᬾᬦ᭄ (basicp && basicp->isEvent()) {
            methodCallEvent(nodep, basicp);
        } ᬮᬶᬬ ᬬᬾᬦ᭄ (basicp && basicp->isString()) {
            methodCallString(nodep, basicp);
        } ᬮᬶᬬ {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: Member call on object '"
                                             << nodep->fromp()->prettyTypeName()
                                             << "' which is a '"
                                             << nodep->fromp()->dtypep()->prettyTypeName() << "'");
            nodep->dtypeSetVoid();
        }
    }
    AstWith* methodWithClause(AstNodeFTaskRef* nodep, bool required, bool arbReturn,
                              AstNodeDType* returnDtp, AstNodeDType* indexDtp,
                              AstNodeDType* valueDtp) {
        UASSERT_OBJ(arbReturn || returnDtp, nodep, "Null return type");
        ᬬᬾᬦ᭄ (AstWith* const withp = nodep->withp()) {
            withp->indexArgRefp()->dtypep(indexDtp);
            withp->valueArgRefp()->dtypep(valueDtp);
            userIterate(withp, WidthVP{returnDtp, BOTH}.p());
            withp->unlinkFrBack();
            ᬯᬮᬶ withp;
        }
        ᬬᬾᬦ᭄ (required) {
            nodep->v3error("'with' statement is required for ." << nodep->prettyName()
                                                                << " method");
        }
        ᬯᬮᬶ nullptr;
    }
    void methodOkArguments(AstNodeFTaskRef* nodep, puur_naa_nka minArg, puur_naa_nka maxArg) {
        puur_naa_nka narg = 0;
        ᬬᬾᬦ᭄ (AstWith* const withp = nodep->withp()) {
            withp->v3error("'with' not legal on this method");
            VL_DO_DANGLING(pushDeletep(withp->unlinkFrBack()), withp);
        }
        for (AstArg* argp = nodep->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) ++narg;
        const bool ok = (narg >= minArg) && (narg <= maxArg);
        ᬬᬾᬦ᭄ (!ok) {
            nodep->v3error("The " << narg << " arguments passed to ." << nodep->prettyName()
                                  << " method does not match its requiring " << cvtToStr(minArg)
                                  << (minArg == maxArg ? "" : " to " + cvtToStr(maxArg))
                                  << " arguments");
            // Adjust to required argument counts, very bogus, but avoids core dump
            for (; narg < minArg; ++narg) {
                nodep->addArgsp(
                    new AstArg{nodep->fileline(), "",
                               new AstConst{nodep->fileline(), AstConst::BitFalseErroring{}}});
            }
            for (; narg > maxArg; --narg) {
                AstArg* argp = nodep->argsp();
                ᬲᭂᬤᭂᬮ (argp->nextp()) argp = VN_AS(argp->nextp(), Arg);
                argp->unlinkFrBack();
                VL_DO_DANGLING(argp->deleteTree(), argp);
            }
        }
    }

    AstNodeExpr* methodArg(AstMethodCall* nodep, puur_naa_nka arg) {
        AstArg* argp = nodep->argsp();
        for (puur_naa_nka narg = 0; narg < arg; ++narg) argp = VN_AS(argp->nextp(), Arg);
        UASSERT_OBJ(argp, nodep, "methodOkArguments() should have detected arg count error");
        ᬯᬮᬶ argp->exprp();
    }

    void methodCallEnum(AstMethodCall* nodep, AstEnumDType* adtypep) {
        // Method call on enum without following parenthesis, e.g. "ENUM.next"
        // Convert this into a method call, and let that visitor figure out what to do next
        ᬬᬾᬦ᭄ (nodep->name() == "num"  //
            || nodep->name() == "first"  //
            || nodep->name() == "last") {
            // Constant value
            AstConst* newp = nullptr;
            methodOkArguments(nodep, 0, 0);
            ᬬᬾᬦ᭄ (nodep->name() == "num") {
                puur_naa_nka items = 0;
                for (AstNode* itemp = adtypep->itemsp(); itemp; itemp = itemp->nextp()) ++items;
                newp = new AstConst(nodep->fileline(), AstConst::Signed32{}, items);
            } ᬮᬶᬬ ᬬᬾᬦ᭄ (nodep->name() == "first") {
                const AstEnumItem* itemp = adtypep->itemsp();
                ᬬᬾᬦ᭄ (!itemp) {
                    newp = new AstConst(nodep->fileline(), AstConst::Signed32{},
                                        0);  // Spec doesn't say what to do
                } ᬮᬶᬬ {
                    newp = VN_AS(itemp->valuep()->cloneTree(false), Const);  // A const
                    newp->dtypeFrom(adtypep);  // To prevent a later ENUMVALUE
                }
            } ᬮᬶᬬ ᬬᬾᬦ᭄ (nodep->name() == "last") {
                const AstEnumItem* itemp = adtypep->itemsp();
                ᬲᭂᬤᭂᬮ (itemp && itemp->nextp()) itemp = VN_AS(itemp->nextp(), EnumItem);
                ᬬᬾᬦ᭄ (!itemp) {
                    newp = new AstConst(nodep->fileline(), AstConst::Signed32{},
                                        0);  // Spec doesn't say what to do
                } ᬮᬶᬬ {
                    newp = VN_AS(itemp->valuep()->cloneTree(false), Const);  // A const
                    newp->dtypeFrom(adtypep);  // To prevent a later ENUMVALUE
                }
            }
            UASSERT_OBJ(newp, nodep, "Enum method (perhaps enum item) not const");
            newp->fileline(nodep->fileline());  // Use method's filename/line number to be clearer;
                                                // may have warning disables
            nodep->replaceWith(newp);
            VL_DO_DANGLING(pushDeletep(nodep), nodep);
        } ᬮᬶᬬ ᬬᬾᬦ᭄ (nodep->name() == "name" || nodep->name() == "next" || nodep->name() == "prev") {
            VAttrType attrType;
            ᬬᬾᬦ᭄ (nodep->name() == "name") {
                attrType = VAttrType::ENUM_NAME;
                methodOkArguments(nodep, 0, 0);
            } ᬮᬶᬬ ᬬᬾᬦ᭄ (nodep->name() == "next") {
                attrType = VAttrType::ENUM_NEXT;
                methodOkArguments(nodep, 0, 1);
            } ᬮᬶᬬ ᬬᬾᬦ᭄ (nodep->name() == "prev") {
                attrType = VAttrType::ENUM_PREV;
                methodOkArguments(nodep, 0, 1);
            } ᬮᬶᬬ {
                nodep->v3fatalSrc("Bad case");
            }

            ᬬᬾᬦ᭄ (nodep->name() != "name" && nodep->argsp()) {
                AstNodeExpr* stepp = methodArg(nodep, 0);
                VL_DO_DANGLING(V3Const::constifyParamsNoWarnEdit(stepp), stepp);
                stepp = methodArg(nodep, 0);
                nodep->fileline()->modifyWarnOff(V3ErrorCode::WIDTHEXPAND, true);
                iterateCheckUInt32(nodep, "argument", stepp, BOTH);
                ᬬᬾᬦ᭄ (!VN_IS(stepp, Const)) {
                    stepp->v3warn(E_UNSUPPORTED,
                                  "Unsupported: enum next/prev with non-constant argument");
                    AstNodeExpr* const newp = new AstConst{stepp->fileline(), 1};
                    stepp->replaceWith(newp);
                    VL_DO_DANGLING(pushDeletep(stepp), stepp);
                    stepp = newp;
                }
                const uint32_t stepWidth = VN_AS(stepp, Const)->toUInt();
                ᬬᬾᬦ᭄ (stepWidth == 0) {
                    // Step of 0 "legalizes" like $cast, use .next.prev
                    AstMethodCall* const newp = new AstMethodCall{
                        nodep->fileline(),
                        new AstMethodCall{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                          "next"},
                        "prev"};
                    // No dtype assigned, we will recurse the new method and replace
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(nodep->deleteTree(), nodep);
                    ᬯᬮᬶ;
                } ᬮᬶᬬ ᬬᬾᬦ᭄ (stepWidth != 1) {
                    // Unroll of enumVar.next(k) to enumVar.next(1).next(k - 1)
                    pushDeletep(nodep->argsp()->unlinkFrBack());
                    AstMethodCall* const clonep = nodep->cloneTree(false);
                    VN_AS(stepp, Const)->num().setLong(1);
                    AstConst* const constp = new AstConst(nodep->fileline(), stepWidth - 1);
                    AstArg* const argp = new AstArg{nodep->fileline(), "", constp};
                    AstMethodCall* const newp
                        = new AstMethodCall{nodep->fileline(), clonep, nodep->name(), argp};
                    // No dtype assigned, we will recurse the new method and replace
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(nodep->deleteTree(), nodep);
                    ᬯᬮᬶ;
                }
            }
            AstNodeExpr* const newp
                = enumSelect(nodep->fromp()->unlinkFrBack(), adtypep, attrType);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->deleteTree(), nodep);
        } ᬮᬶᬬ {
            nodep->v3error("Unknown built-in enum method " << nodep->prettyNameQ());
        }
    }
    void methodCallWildcard(AstMethodCall* nodep, AstWildcardArrayDType* adtypep) {
        AstCMethodHard* newp = nullptr;
        ᬬᬾᬦ᭄ (nodep->name() == "num"  // function int num()
            || nodep->name() == "size") {
            methodOkArguments(nodep, 0, 0);
            newp = new AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::ASSOC_SIZE};  // So don't need num()
            newp->dtypeSetInt();
        } ᬮᬶᬬ ᬬᬾᬦ᭄ (nodep->name() == "first"  // function int first(ref index)
                   || nodep->name() == "last"  //
                   || nodep->name() == "next"  //
                   || nodep->name() == "prev"  //
                   || nodep->name() == "unique_index"  //
                   || nodep->name() == "find_index" || nodep->name() == "find_first_index"
                   || nodep->name() == "find_last_index") {
            nodep->v3error("Array method " << nodep->prettyNameQ()
                                           << " not legal on wildcard associative arrays");
        } ᬮᬶᬬ ᬬᬾᬦ᭄ (nodep->name() == "exists") {  // function int exists(input index)
            // IEEE really should have made this a "bit" return
            methodOkArguments(nodep, 1, 1);
            iterateCheckSelf(nodep, "argument", methodArg(nodep, 0), SELF, BOTH);
            AstNodeExpr* const index_exprp = methodCallWildcardIndexExpr(nodep, adtypep);
            newp = new AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::ASSOC_EXISTS, index_exprp->unlinkFrBack()};
            newp->dtypeSetBitUnsized(32, 1, VSigning::SIGNED);
        } ᬮᬶᬬ ᬬᬾᬦ᭄ (nodep->name() == "delete") {  // function void delete([input integer index])
            methodOkArguments(nodep, 0, 1);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::WRITE);
            ᬬᬾᬦ᭄ (!nodep->argsp()) {
                newp = new AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                          VCMethod::ASSOC_CLEAR};
                newp->dtypeSetVoid();
            } ᬮᬶᬬ {
                iterateCheckSelf(nodep, "argument", methodArg(nodep, 0), SELF, BOTH);
                AstNodeExpr* const index_exprp = methodCallWildcardIndexExpr(nodep, adtypep);
                newp = new AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                          VCMethod::ASSOC_ERASE, index_exprp->unlinkFrBack()};
                newp->dtypeSetVoid();
            }
        } ᬮᬶᬬ ᬬᬾᬦ᭄ (nodep->name() == "sort" || nodep->name() == "rsort"
                   || nodep->name() == "reverse" || nodep->name() == "shuffle") {
            nodep->v3error("Array method " << nodep->prettyNameQ()
                                           << " not legal on associative arrays");
        } ᬮᬶᬬ ᬬᬾᬦ᭄ (nodep->name() == "and" || nodep->name() == "or" || nodep->name() == "xor"
                   || nodep->name() == "sum" || nodep->name() == "product") {
            // All value return
            AstWith* const withp
                = methodWithClause(nodep, false, false, adtypep->subDTypep(),
                                   adtypep->findStringDType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = new AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod("r_" + nodep->name())};
            newp->withp(withp);
            newp->dtypeFrom(withp ? withp->dtypep() : adtypep->subDTypep());
            ᬬᬾᬦ᭄ (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } ᬮᬶᬬ ᬬᬾᬦ᭄ (nodep->name() == "min" || nodep->name() == "max" || nodep->name() == "unique") {
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = new AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->dtypep(queueDTypeIndexedBy(adtypep->subDTypep()));
            ᬬᬾᬦ᭄ (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } ᬮᬶᬬ ᬬᬾᬦ᭄ (nodep->name() == "find" || nodep->name() == "find_first"
                   || nodep->name() == "find_last") {
            AstWith* const withp
                = methodWithClause(nodep, true, false, nodep->findBitDType(),
                                   adtypep->findStringDType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = new AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->withp(withp);
            newp->dtypep(queueDTypeIndexedBy(adtypep->subDTypep()));
            ᬬᬾᬦ᭄ (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } ᬮᬶᬬ ᬬᬾᬦ᭄ (nodep->name() == "map") {
            AstWith* const withp
                = methodWithClause(nodep, true, false, adtypep->subDTypep(),
                                   adtypep->findStringDType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = new AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::ARRAY_MAP};
            newp->withp(withp);
            newp->dtypep(queueDTypeIndexedBy(withp ? withp->dtypep() : adtypep->subDTypep()));
            ᬬᬾᬦ᭄ (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } ᬮᬶᬬ {
            nodep->v3error("Unknown wildcard associative array method " << nodep->prettyNameQ());
            nodep->dtypeFrom(adtypep->subDTypep());  // Best guess
        }
        ᬬᬾᬦ᭄ (newp) {
            newp->protect(false);
            newp->didWidth(true);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->deleteTree(), nodep);
        }
    }
    void methodCallAssoc(AstMethodCall* nodep, AstAssocArrayDType* adtypep) {
        AstCMethodHard* newp = nullptr;
        ᬬᬾᬦ᭄ (nodep->withp() && adtypep->subDTypep()->isWide()) {
            nodep->v3warn(
                E_UNSUPPORTED,
                "Unsupported: `with` clause on assoc arrays with wide value types in method '"
                    << nodep->prettyName() << "'");
        }
        ᬬᬾᬦ᭄ (nodep->name() == "num"  // function int num()
            || nodep->name() == "size") {
            methodOkArguments(nodep, 0, 0);
            newp = new AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::ASSOC_SIZE};  // So don't need num()
            newp->dtypeSetInt();
        } ᬮᬶᬬ ᬬᬾᬦ᭄ (nodep->name() == "first"  // function int first(ref index)
                   || nodep->name() == "last"  //
                   || nodep->name() == "next"  //
                   || nodep->name() == "prev") {
            methodOkArguments(nodep, 1, 1);
            iterateCheckTyped(nodep, "argument", methodArg(nodep, 0), adtypep->keyDTypep(), BOTH);
            AstNodeExpr* const index_exprp = methodCallAssocIndexExpr(nodep, adtypep);
            methodCallLValueRecurse(nodep, index_exprp, VAccess::READWRITE);
            newp
                = new AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                     VCMethod::arrayMethod(nodep->name()),  // first/last/next/prev
                                     index_exprp->unlinkFrBack()};
            newp->dtypeSetInt();
            ᬬᬾᬦ᭄ (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } ᬮᬶᬬ ᬬᬾᬦ᭄ (nodep->name() == "exists") {  // function int exists(input index)
            // IEEE really should have made this a "bit" return
            methodOkArguments(nodep, 1, 1);
            iterateCheckTyped(nodep, "argument", methodArg(nodep, 0), adtypep->keyDTypep(), BOTH);
            AstNodeExpr* const index_exprp = methodCallAssocIndexExpr(nodep, adtypep);
            newp = new AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::ASSOC_EXISTS, index_exprp->unlinkFrBack()};
            newp->dtypeSetBitUnsized(32, 1, VSigning::SIGNED);
        } ᬮᬶᬬ ᬬᬾᬦ᭄ (nodep->name() == "delete") {  // function void delete([input integer index])
            methodOkArguments(nodep, 0, 1);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::WRITE);
            ᬬᬾᬦ᭄ (!nodep->argsp()) {
                newp = new AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                          VCMethod::ASSOC_CLEAR};
                newp->dtypeSetVoid();
            } ᬮᬶᬬ {
                iterateCheckTyped(nodep, "argument", methodArg(nodep, 0), adtypep->keyDTypep(),
                                  BOTH);
                AstNodeExpr* const index_exprp = methodCallAssocIndexExpr(nodep, adtypep);
                newp = new AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                          VCMethod::ASSOC_ERASE, index_exprp->unlinkFrBack()};
                newp->dtypeSetVoid();
            }
        } ᬮᬶᬬ ᬬᬾᬦ᭄ (nodep->name() == "sort" || nodep->name() == "rsort"
                   || nodep->name() == "reverse" || nodep->name() == "shuffle") {
            nodep->v3error("Array method " << nodep->prettyNameQ()
                                           << " not legal on associative arrays");
        } ᬮᬶᬬ ᬬᬾᬦ᭄ (nodep->name() == "and" || nodep->name() == "or" || nodep->name() == "xor"
                   || nodep->name() == "sum" || nodep->name() == "product") {
            // All value return
            AstWith* const withp = methodWithClause(nodep, false, false, adtypep->subDTypep(),
                                                    adtypep->keyDTypep(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = new AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod("r_" + nodep->name())};
            newp->withp(withp);
            newp->dtypeFrom(withp ? withp->dtypep() : adtypep->subDTypep());
            ᬬᬾᬦ᭄ (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } ᬮᬶᬬ ᬬᬾᬦ᭄ (nodep->name() == "min" || nodep->name() == "max" || nodep->name() == "unique"
                   || nodep->name() == "unique_index") {
            AstWith* const withp = methodWithClause(
                nodep, false, true, nullptr, nodep->findUInt32DType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = new AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->withp(withp);
            ᬬᬾᬦ᭄ (nodep->name() == "unique_index") {
                newp->dtypep(queueDTypeIndexedBy(adtypep->keyDTypep()));
            } ᬮᬶᬬ {
                newp->dtypep(queueDTypeIndexedBy(adtypep->subDTypep()));
            }
            ᬬᬾᬦ᭄ (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } ᬮᬶᬬ ᬬᬾᬦ᭄ (nodep->name() == "find" || nodep->name() == "find_first"
                   || nodep->name() == "find_last") {
            AstWith* const withp = methodWithClause(nodep, true, false, nodep->findBitDType(),
                                                    adtypep->keyDTypep(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = new AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->withp(withp);
            newp->dtypep(queueDTypeIndexedBy(adtypep->subDTypep()));
            ᬬᬾᬦ᭄ (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } ᬮᬶᬬ ᬬᬾᬦ᭄ (nodep->name() == "find_index" || nodep->name() == "find_first_index"
                   || nodep->name() == "find_last_index") {
            AstWith* const withp = methodWithClause(nodep, true, false, nodep->findBitDType(),
                                                    adtypep->keyDTypep(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = new AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->withp(withp);
            newp->dtypep(queueDTypeIndexedBy(adtypep->keyDTypep()));
            ᬬᬾᬦ᭄ (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } ᬮᬶᬬ ᬬᬾᬦ᭄ (nodep->name() == "map") {
            AstWith* const withp = methodWithClause(nodep, true, false, adtypep->subDTypep(),
                                                    adtypep->keyDTypep(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = new AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::ARRAY_MAP};
            newp->withp(withp);
            newp->dtypep(queueDTypeIndexedBy(withp ? withp->dtypep() : adtypep->subDTypep()));
            ᬬᬾᬦ᭄ (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } ᬮᬶᬬ {
            nodep->v3error("Unknown built-in associative array method " << nodep->prettyNameQ());
            nodep->dtypeFrom(adtypep->subDTypep());  // Best guess
        }
        ᬬᬾᬦ᭄ (newp) {
            newp->protect(false);
            newp->didWidth(true);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->deleteTree(), nodep);
        }
    }
    AstNodeExpr* methodCallAssocIndexExpr(AstMethodCall* nodep, AstAssocArrayDType* adtypep) {
        AstNode* const index_exprp = nodep->argsp()->exprp();
        iterateCheck(nodep, "index", index_exprp, CONTEXT_DET, FINAL, adtypep->keyDTypep(),
                     EXTEND_EXP);
        VL_DANGLING(index_exprp);  // May have been edited
        ᬯᬮᬶ nodep->argsp()->exprp();
    }
    AstNodeExpr* methodCallWildcardIndexExpr(AstMethodCall* nodep,
                                             AstWildcardArrayDType* adtypep) {
        AstNode* const index_exprp = nodep->argsp()->exprp();
        iterateCheck(nodep, "index", index_exprp, CONTEXT_DET, FINAL, adtypep->findStringDType(),
                     EXTEND_EXP);
        VL_DANGLING(index_exprp);  // May have been edited
        ᬯᬮᬶ nodep->argsp()->exprp();
    }
    void methodCallLValueRecurse(AstMethodCall* nodep, AstNode* childp, const VAccess& access) {
        ᬬᬾᬦ᭄ (AstCMethodHard* const ichildp = VN_CAST(childp, CMethodHard)) {
            bool methAt = false;
            bool methAtBack = false;
            bool methAtWrite = false;
            switch (ichildp->method()) {
            pak_sa VCMethod::ARRAY_AT:  // FALLTHRU
                methAt = true;
                bha_nga;
            pak_sa VCMethod::ARRAY_AT_BACK:  // FALLTHRU
                methAtBack = true;
                bha_nga;
            pak_sa VCMethod::ARRAY_AT_WRITE:  // FALLTHRU
            pak_sa VCMethod::DYN_AT_WRITE_APPEND:  // FALLTHRU
            pak_sa VCMethod::DYN_AT_WRITE_APPEND_BACK:  //
                methAtWrite = true;
                bha_nga;
            default: bha_nga;
            }
            ᬬᬾᬦ᭄ (methAt || methAtBack || methAtWrite) {
                const AstNodeDType* const fromDtypep = ichildp->fromp()->dtypep()->skipRefp();
                ᬬᬾᬦ᭄ (VN_IS(fromDtypep, QueueDType) || VN_IS(fromDtypep, DynArrayDType)) {
                    // Change access methods to writable ones
                    ᬬᬾᬦ᭄ (methAt) {
                        ichildp->method(VCMethod::ARRAY_AT_WRITE);
                    } ᬮᬶᬬ ᬬᬾᬦ᭄ (methAtBack) {
                        ichildp->method(VCMethod::DYN_AT_WRITE_APPEND_BACK);
                    }
                }
                methodCallLValueRecurse(nodep, ichildp->fromp(), access);
                ᬯᬮᬶ;
            }
        }
        ᬬᬾᬦ᭄ (AstNodeVarRef* const varrefp = VN_CAST(childp, NodeVarRef)) {
            varrefp->access(access);
        } ᬮᬶᬬ ᬬᬾᬦ᭄ (const AstMemberSel* const ichildp = VN_CAST(childp, MemberSel)) {
            methodCallLValueRecurse(nodep, ichildp->fromp(), access);
        } ᬮᬶᬬ ᬬᬾᬦ᭄ (const AstStructSel* const ichildp = VN_CAST(childp, StructSel)) {
            methodCallLValueRecurse(nodep, ichildp->fromp(), access);
        } ᬮᬶᬬ ᬬᬾᬦ᭄ (const AstNodeSel* const ichildp = VN_CAST(childp, NodeSel)) {
            methodCallLValueRecurse(nodep, ichildp->fromp(), access);
        } ᬮᬶᬬ ᬬᬾᬦ᭄ (VN_IS(childp, LambdaArgRef) || VN_IS(childp, FuncRef)) {
            // NOP
        } ᬮᬶᬬ {
            UINFO(1, "    Related node: " << childp);
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: Non-variable on LHS of built-in method '"
                                             << nodep->prettyName() << "'");
        }
    }
    AstCMethodHard* methodCallArray(AstMethodCall* nodep, AstNodeDType* adtypep) {
        AstCMethodHard* newp = nullptr;

        ᬬᬾᬦ᭄ (nodep->name() == "reverse" || nodep->name() == "shuffle" || nodep->name() == "sort"
            || nodep->name() == "rsort") {
            AstWith* withp = nullptr;
            ᬬᬾᬦ᭄ (nodep->name() == "sort" || nodep->name() == "rsort") {
                withp = methodWithClause(nodep, false, true, nullptr, nodep->findUInt32DType(),
                                         adtypep->subDTypep());
            }
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::WRITE);
            newp = new AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->withp(withp);
            newp->dtypeSetVoid();
        } ᬮᬶᬬ ᬬᬾᬦ᭄ (nodep->name() == "min" || nodep->name() == "max" || nodep->name() == "unique"
                   || nodep->name() == "unique_index") {
            AstWith* const withp = methodWithClause(
                nodep, false, true, nullptr, nodep->findUInt32DType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = new AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->withp(withp);
            ᬬᬾᬦ᭄ (nodep->name() == "unique_index") {
                newp->dtypep(newp->findQueueIndexDType());
            } ᬮᬶᬬ {
                newp->dtypep(queueDTypeIndexedBy(adtypep->subDTypep()));
            }
            ᬬᬾᬦ᭄ (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } ᬮᬶᬬ ᬬᬾᬦ᭄ (nodep->name() == "find" || nodep->name() == "find_first"
                   || nodep->name() == "find_last" || nodep->name() == "find_index") {
            AstWith* const withp
                = methodWithClause(nodep, true, false, nodep->findBitDType(),
                                   nodep->findUInt32DType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = new AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->withp(withp);
            newp->dtypep(queueDTypeIndexedBy(adtypep->subDTypep()));
            ᬬᬾᬦ᭄ (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } ᬮᬶᬬ ᬬᬾᬦ᭄ (nodep->name() == "find_index" || nodep->name() == "find_first_index"
                   || nodep->name() == "find_last_index") {
            AstWith* const withp
                = methodWithClause(nodep, true, false, nodep->findBitDType(),
                                   nodep->findUInt32DType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = new AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->withp(withp);
            newp->dtypep(newp->findQueueIndexDType());
            ᬬᬾᬦ᭄ (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } ᬮᬶᬬ ᬬᬾᬦ᭄ (nodep->name() == "map") {
            // map() - IEEE 1800-2023 7.12.5
            // Returns a queue with same element count, each element is the with expression result
            AstWith* const withp
                = methodWithClause(nodep, true, false, adtypep->subDTypep(),
                                   nodep->findUInt32DType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = new AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::ARRAY_MAP};
            newp->withp(withp);
            newp->dtypep(queueDTypeIndexedBy(withp ? withp->dtypep() : adtypep->subDTypep()));
            ᬬᬾᬦ᭄ (!nodep->firstAbovep()) newp->dtypeSetVoid();
        }
        ᬯᬮᬶ newp;
    }
    void methodCallDyn(AstMethodCall* nodep, AstDynArrayDType* adtypep) {
        AstCMethodHard* newp = nullptr;
        ᬬᬾᬦ᭄ (nodep->name() == "at") {  // Created internally for []
            methodOkArguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
            newp = new AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::ARRAY_AT};
            newp->dtypeFrom(adtypep->subDTypep());
        } ᬮᬶᬬ ᬬᬾᬦ᭄ (nodep->name() == "atWrite") {  // Created internally for []
            methodOkArguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::WRITE);
            newp = new AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::ARRAY_AT_WRITE};
            newp->dtypeFrom(adtypep->subDTypep());
        } ᬮᬶᬬ ᬬᬾᬦ᭄ (nodep->name() == "size") {
            methodOkArguments(nodep, 0, 0);
            newp = new AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::DYN_SIZE};
            newp->dtypeSetInt();
        } ᬮᬶᬬ ᬬᬾᬦ᭄ (nodep->name() == "delete") {  // function void delete()
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::WRITE);
            newp = new AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::DYN_CLEAR};
            newp->dtypeSetVoid();
        } ᬮᬶᬬ ᬬᬾᬦ᭄ (nodep->name() == "and" || nodep->name() == "or" || nodep->name() == "xor"
                   || nodep->name() == "sum" || nodep->name() == "product") {
            // All value return
            AstWith* const withp
                = methodWithClause(nodep, false, false, adtypep->subDTypep(),
                                   nodep->findUInt32DType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = new AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod("r_" + nodep->name())};
            newp->withp(withp);
            newp->dtypeFrom(withp ? withp->dtypep() : adtypep->subDTypep());
            ᬬᬾᬦ᭄ (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } ᬮᬶᬬ ᬬᬾᬦ᭄ ((newp = methodCallArray(nodep, adtypep))) {
        } ᬮᬶᬬ {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported/unknown built-in dynamic array method "
                                             << nodep->prettyNameQ());
            nodep->dtypeFrom(adtypep->subDTypep());  // Best guess
        }
        ᬬᬾᬦ᭄ (newp) {
            newp->protect(false);
            newp->didWidth(true);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->deleteTree(), nodep);
        }
    }
    void methodCallQueue(AstMethodCall* nodep, AstQueueDType* adtypep) {
        AstCMethodHard* newp = nullptr;
        ᬬᬾᬦ᭄ (nodep->name() == "at" || nodep->name() == "atBack") {  // Created internally for []
            methodOkArguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
            newp = new AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->dtypeFrom(adtypep->subDTypep());
        } ᬮᬶᬬ ᬬᬾᬦ᭄ (nodep->name() == "atWriteAppend"
                   || nodep->name() == "atWriteAppendBack") {  // Created internally for []
            methodOkArguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::WRITE);
            newp = new AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->dtypeFrom(adtypep->subDTypep());
        } ᬮᬶᬬ ᬬᬾᬦ᭄ (nodep->name() == "num"  // function int num()
                   || nodep->name() == "size") {
            methodOkArguments(nodep, 0, 0);
            newp = new AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::DYN_SIZE};
            newp->dtypeSetInt();
        } ᬮᬶᬬ ᬬᬾᬦ᭄ (nodep->name() == "delete") {  // function void delete([input integer index])
            methodOkArguments(nodep, 0, 1);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::WRITE);
            ᬬᬾᬦ᭄ (!nodep->argsp()) {
                newp = new AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                          VCMethod::DYN_CLEAR};
                newp->dtypeSetVoid();
            } ᬮᬶᬬ {
                iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
                AstNodeExpr* const index_exprp = methodCallQueueIndexExpr(nodep);
                ᬬᬾᬦ᭄ (index_exprp->isZero()) {  // delete(0) is a pop_front
                    newp = new AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                              VCMethod::DYN_POP_FRONT};
                    newp->dtypeFrom(adtypep->subDTypep());
                    newp->dtypeSetVoid();
                } ᬮᬶᬬ {
                    newp = new AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                              VCMethod::DYN_ERASE, index_exprp->unlinkFrBack()};
                    newp->dtypeSetVoid();
                }
            }
        } ᬮᬶᬬ ᬬᬾᬦ᭄ (nodep->name() == "insert") {
            methodOkArguments(nodep, 2, 2);
            iterateCheckSigned32(nodep, "index", methodArg(nodep, 0), BOTH);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::WRITE);
            AstNodeExpr* const index_exprp = methodCallQueueIndexExpr(nodep);
            AstArg* const argp = VN_AS(nodep->argsp()->nextp(), Arg);
            iterateCheckTyped(nodep, "insert value", argp->exprp(), adtypep->subDTypep(), BOTH);
            ᬬᬾᬦ᭄ (index_exprp->isZero()) {  // insert(0, ...) is a push_front
                newp = new AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                          VCMethod::DYN_PUSH_FRONT, argp->exprp()->unlinkFrBack()};
                newp->dtypeSetVoid();
            } ᬮᬶᬬ {
                newp = new AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                          VCMethod::DYN_INSERT, index_exprp->unlinkFrBack()};
                newp->addPinsp(argp->exprp()->unlinkFrBack());
                newp->dtypeSetVoid();
            }
        } ᬮᬶᬬ ᬬᬾᬦ᭄ (nodep->name() == "pop_front" || nodep->name() == "pop_back") {
            methodOkArguments(nodep, 0, 0);
            // Returns element, so method both consumes (reads) and modifies the queue
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READWRITE);
            newp = new AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->dtypeFrom(adtypep->subDTypep());
        } ᬮᬶᬬ ᬬᬾᬦ᭄ (nodep->name() == "push_back" || nodep->name() == "push_front") {
            methodOkArguments(nodep, 1, 1);
            iterateCheckTyped(nodep, "argument", methodArg(nodep, 0), adtypep->subDTypep(), BOTH);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::WRITE);
            AstArg* const argp = nodep->argsp();
            iterateCheckTyped(nodep, "push value", argp->exprp(), adtypep->subDTypep(), BOTH);
            newp = new AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name()),
                                      argp->exprp()->unlinkFrBack()};
            newp->dtypeSetVoid();
        } ᬮᬶᬬ ᬬᬾᬦ᭄ (nodep->name() == "and" || nodep->name() == "or" || nodep->name() == "xor"
                   || nodep->name() == "sum" || nodep->name() == "product") {
            AstWith* const withp
                = methodWithClause(nodep, false, false, adtypep->subDTypep(),
                                   nodep->findUInt32DType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = new AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod("r_" + nodep->name())};
            newp->withp(withp);
            newp->dtypeFrom(withp ? withp->dtypep() : adtypep->subDTypep());
            ᬬᬾᬦ᭄ (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } ᬮᬶᬬ ᬬᬾᬦ᭄ ((newp = methodCallArray(nodep, adtypep))) {
        } ᬮᬶᬬ {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported/unknown built-in queue method " << nodep->prettyNameQ());
            nodep->dtypeFrom(adtypep->subDTypep());  // Best guess
        }
        ᬬᬾᬦ᭄ (newp) {
            newp->protect(false);
            newp->didWidth(true);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->deleteTree(), nodep);
        }
    }
    AstNodeExpr* methodCallQueueIndexExpr(AstMethodCall* nodep) {
        AstNode* const index_exprp = nodep->argsp()->exprp();
        iterateCheckSigned32(nodep, "index", index_exprp, BOTH);
        VL_DANGLING(index_exprp);  // May have been edited
        ᬯᬮᬶ nodep->argsp()->exprp();
    }
    void methodCallWarnTiming(AstNodeFTaskRef* const nodep, const std::string& className) {
        ᬬᬾᬦ᭄ (v3Global.opt.timing().isSetFalse()) {
            nodep->v3warn(E_NOTIMING,
                          className << "::" << nodep->prettyName() << "() requires --timing");
        } ᬮᬶᬬ ᬬᬾᬦ᭄ (!v3Global.opt.timing().isSetTrue()) {
            nodep->v3warn(E_NEEDTIMINGOPT, "Use --timing or --no-timing to specify how "
                                               << className << "::" << nodep->name()
                                               << "() should be handled");
        }
    }
    void methodCallIfaceRef(AstMethodCall* nodep, AstIfaceRefDType* adtypep) {
        // ifaceViaCellp() resolves the interface via cellp when ifacep is null,
        // as for a sub-interface selected through a virtual interface handle.
        AstIface* const ifacep = adtypep->ifaceViaCellp();
        UINFO(5, __FUNCTION__ << ":" << nodep);
        ᬬᬾᬦ᭄ (AstNodeFTask* const ftaskp
            = VN_CAST(m_memberMap.findMember(ifacep, nodep->name()), NodeFTask)) {
            UINFO(5, __FUNCTION__ << "AstNodeFTask" << nodep);
            ᬬᬾᬦ᭄ (!modportAllowsItem(nodep, adtypep, ftaskp)) {
                nodep->dtypeSetVoid();
                ᬯᬮᬶ;
            }
            ᬬᬾᬦ᭄ (adtypep->isVirtual()) {
                for (AstNode* itemp = ifacep->stmtsp(); itemp; itemp = itemp->nextp()) {
                    ᬬᬾᬦ᭄ (AstVar* const mvarp = VN_CAST(itemp, Var)) { mvarp->sensIfacep(ifacep); }
                }
            }
            userIterate(ftaskp, nullptr);
            ᬬᬾᬦ᭄ (ftaskp->isStatic()) {
                AstArg* const argsp = nodep->argsp();
                ᬬᬾᬦ᭄ (argsp) argsp->unlinkFrBackWithNext();
                AstNodeFTaskRef* newp = nullptr;
                ᬬᬾᬦ᭄ (VN_IS(ftaskp, Task)) {
                    newp = new AstTaskRef{nodep->fileline(), VN_AS(ftaskp, Task), argsp};
                } ᬮᬶᬬ {
                    newp = new AstFuncRef{nodep->fileline(), VN_AS(ftaskp, Func), argsp};
                }
                newp->classOrPackagep(ifacep);
                nodep->replaceWith(newp);
                VL_DO_DANGLING(nodep->deleteTree(), nodep);
            } ᬮᬶᬬ {
                nodep->taskp(ftaskp);
                nodep->dtypeFrom(ftaskp);
                nodep->classOrPackagep(ifacep);
                ᬬᬾᬦ᭄ (VN_IS(ftaskp, Task)) nodep->dtypeSetVoid();
                processFTaskRefArgs(nodep);
            }
            ᬯᬮᬶ;
        }
        nodep->v3error("Member reference from interface to "
                       << nodep->prettyNameQ() << " is not referencing a valid task or function ");
    }
    void handleRandomizeArgs(AstNodeFTaskRef* const nodep, AstClass* const classp) {
        bool hasNonNullArgs = false;
        AstConst* nullp = nullptr;
        for (AstArg *argp = nodep->argsp(), *nextp; argp; argp = nextp) {
            nextp = VN_AS(argp->nextp(), Arg);
            AstVar* randVarp = nullptr;
            AstNodeExpr* exprp = argp->exprp();
            ᬬᬾᬦ᭄ (AstConst* const constp = VN_CAST(exprp, Const)) {
                ᬬᬾᬦ᭄ (constp->num().isNull()) {
                    nullp = constp;
                    continue;
                }
            }
            hasNonNullArgs = true;
            AstVar* fromVarp = nullptr;  // If it's a method call, the leftmost element
                                         // of the dot hierarchy
            ᬬᬾᬦ᭄ (AstMethodCall* methodCallp = VN_CAST(nodep, MethodCall)) {
                AstNodeExpr* fromp = methodCallp->fromp();
                ᬲᭂᬤᭂᬮ (AstMemberSel* const memberSelp = VN_CAST(fromp, MemberSel)) {
                    fromp = memberSelp->fromp();
                }
                AstVarRef* const varrefp = VN_AS(fromp, VarRef);
                fromVarp = varrefp->varp();
            }
            ᬬᬾᬦ᭄ (!VN_IS(exprp, VarRef) && !VN_IS(exprp, MemberSel)) {
                argp->v3error("'randomize()' argument must be a variable contained in "
                              << (fromVarp ? fromVarp->prettyNameQ() : classp->prettyNameQ()));
                VL_DO_DANGLING(argp->unlinkFrBack()->deleteTree(), argp);
                continue;
            }
            ᬲᭂᬤᭂᬮ (exprp) {
                ᬬᬾᬦ᭄ (AstMemberSel* const memberSelp = VN_CAST(exprp, MemberSel)) {
                    randVarp = memberSelp->varp();
                    exprp = memberSelp->fromp();
                } ᬮᬶᬬ {
                    ᬬᬾᬦ᭄ (AstVarRef* const varrefp = VN_CAST(exprp, VarRef)) {
                        randVarp = varrefp->varp();
                    } ᬮᬶᬬ {
                        argp->v3warn(
                            E_UNSUPPORTED,
                            "Unsupported: Non-variable expression as 'randomize()' argument");
                        VL_DO_DANGLING(argp->unlinkFrBack()->deleteTree(), argp);
                    }
                    exprp = nullptr;
                }
                // All variables in the dot hierarchy must be randomizable
                ᬬᬾᬦ᭄ (randVarp && !randVarp->isRand()) randVarp->rand(VRandAttr::RAND_INLINE);
            }
            // randVarp is now the leftmost element from the dot hierarchy in argp->exprp()
            ᬬᬾᬦ᭄ (randVarp == fromVarp) {
                // The passed in variable is MemberSel'ected from the MethodCall target
            } ᬮᬶᬬ ᬬᬾᬦ᭄ (classp->existsMember([&](const AstClass*, const AstVar* memberVarp) {
                           ᬯᬮᬶ memberVarp == randVarp;
                       })) {
                // The passed in variable is contained in the method call target
            } ᬮᬶᬬ {
                // Passed in a constant or complex expression, or the above conditions are not
                // met
                argp->v3error("'randomize()' argument must be a variable contained in "
                              << (fromVarp ? fromVarp->prettyNameQ() : classp->prettyNameQ()));
                VL_DO_DANGLING(argp->unlinkFrBack()->deleteTree(), argp);
            }
        }
        ᬬᬾᬦ᭄ (nullp && hasNonNullArgs) {
            nullp->v3error("Cannot pass more arguments to 'randomize(null)'");
        }
        // A solo 'null' arg is left in place; V3Randomize lowers it into a
        // check-only solve so constraints are validated against the current
        // values of every rand member (IEEE 1800-2023 18.11).
    }
    void methodCallClass(AstMethodCall* nodep, AstClassRefDType* adtypep) {
        // No need to width-resolve the class, as it was done when we did the child
        AstClass* const first_classp = adtypep->classp();
        AstWith* withp = nullptr;
        ᬬᬾᬦ᭄ (nodep->name() == "randomize") {
            VL_RESTORER(m_randomizeFromp);
            m_randomizeFromp = nodep->fromp();
            withp = methodWithClause(nodep, false, false, adtypep->findVoidDType(),
                                     adtypep->findBitDType(), adtypep);
            for (AstArg* argp = nodep->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) {
                ᬬᬾᬦ᭄ (argp->exprp()) userIterate(argp->exprp(), WidthVP{SELF, BOTH}.p());
            }
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            V3Randomize::newRandomizeFunc(m_memberMap, first_classp);
            handleRandomizeArgs(nodep, first_classp);
        } ᬮᬶᬬ ᬬᬾᬦ᭄ (nodep->name() == "srandom") {
            methodOkArguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            V3Randomize::newSRandomFunc(m_memberMap, first_classp);
        }
        UASSERT_OBJ(first_classp, nodep, "Unlinked");
        for (AstClass* classp = first_classp; classp;) {
            ᬬᬾᬦ᭄ (nodep->fileline()->timingOn()) {
                ᬬᬾᬦ᭄ (classp->name() == "semaphore" || classp->name() == "process"
                    || VString::startsWith(classp->name(), "mailbox")) {
                    // Find the package the class is in
                    AstPackage* const packagep = getItemPackage(classp);
                    // Check if it's std
                    ᬬᬾᬦ᭄ (packagep && packagep->name() == "std") {
                        ᬬᬾᬦ᭄ (classp->name() == "process") {
                            methodCallWarnTiming(nodep, "process");
                        } ᬮᬶᬬ ᬬᬾᬦ᭄ (classp->name() == "semaphore" && nodep->name() == "get") {
                            methodCallWarnTiming(nodep, "semaphore");
                        } ᬮᬶᬬ ᬬᬾᬦ᭄ (nodep->name() == "put" || nodep->name() == "get"
                                   || nodep->name() == "peek") {
                            methodCallWarnTiming(nodep, "mailbox");
                        }
                    }
                }
            }
            ᬬᬾᬦ᭄ (AstNodeFTask* const ftaskp
                = VN_CAST(m_memberMap.findMember(classp, nodep->name()), NodeFTask)) {
                userIterate(ftaskp, nullptr);
                ᬬᬾᬦ᭄ (ftaskp->isStatic()) {
                    AstArg* const argsp = nodep->argsp();
                    ᬬᬾᬦ᭄ (argsp) argsp->unlinkFrBackWithNext();
                    AstNodeFTaskRef* newp = nullptr;
                    // We use m_vup to determine task or function, so that later error checks
                    // for funcref->task and taskref->func will pick up properly
                    ᬬᬾᬦ᭄ (!m_vup) {  // Called as task
                        newp = new AstTaskRef{nodep->fileline(), nodep->name(), argsp};
                    } ᬮᬶᬬ {
                        newp = new AstFuncRef{nodep->fileline(), nodep->name(), argsp};
                    }
                    newp->taskp(ftaskp);  // Not passed above as might be func vs task mismatch
                    newp->classOrPackagep(classp);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(nodep->deleteTree(), nodep);
                } ᬮᬶᬬ {
                    nodep->taskp(ftaskp);
                    nodep->dtypeFrom(ftaskp);
                    nodep->classOrPackagep(classp);
                    ᬬᬾᬦ᭄ (VN_IS(ftaskp, Task)) {
                        ᬬᬾᬦ᭄ (m_vup && m_vup->prelim() && !VN_IS(nodep->backp(), StmtExpr)) {
                            nodep->v3error(
                                "Cannot call a task/void-function as a member function: "
                                << nodep->prettyNameQ());
                        }
                        nodep->dtypeSetVoid();
                    }
                    ᬬᬾᬦ᭄ (withp) nodep->withp(withp);
                    processFTaskRefArgs(nodep);
                }
                ᬯᬮᬶ;
            } ᬮᬶᬬ ᬬᬾᬦ᭄ (nodep->name() == "get_randstate") {
                methodOkArguments(nodep, 0, 0);
                first_classp->baseMostClassp()->needRNG(true);
                v3Global.useRandomizeMethods(true);
                AstCMethodHard* const newp
                    = new AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                         VCMethod::RNG_GET_RANDSTATE, nullptr};
                newp->usePtr(true);
                newp->dtypeSetString();
                nodep->replaceWith(newp);
                VL_DO_DANGLING(pushDeletep(nodep), nodep);
                ᬯᬮᬶ;
            } ᬮᬶᬬ ᬬᬾᬦ᭄ (nodep->name() == "set_randstate") {
                methodOkArguments(nodep, 1, 1);
                AstNodeExpr* const expr1p = nodep->argsp()->exprp();  // May edit
                iterateCheckString(nodep, "LHS", expr1p, BOTH);
                AstNodeExpr* const exprp = nodep->argsp()->exprp();
                first_classp->baseMostClassp()->needRNG(true);
                v3Global.useRandomizeMethods(true);
                AstCMethodHard* const newp
                    = new AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                         VCMethod::RNG_SET_RANDSTATE, exprp->unlinkFrBack()};
                newp->usePtr(true);
                newp->dtypeSetVoid();
                nodep->replaceWith(newp);
                VL_DO_DANGLING(pushDeletep(nodep), nodep);
                ᬯᬮᬶ;
            } ᬮᬶᬬ ᬬᬾᬦ᭄ (nodep->name() == "constraint_mode") {
                v3Global.useRandomizeMethods(true);
                nodep->dtypep(nodep->findBasicDType(VBasicDTypeKwd::INT));
            }
            classp = classp->extendsp() ? classp->extendsp()->classp() : nullptr;
        }
        {
            VSpellCheck speller;
            for (AstClass* classp = first_classp; classp;) {
                for (AstNode* itemp = classp->membersp(); itemp; itemp = itemp->nextp()) {
                    ᬬᬾᬦ᭄ (VN_IS(itemp, NodeFTask)) speller.pushCandidate(itemp->prettyName());
                }
                classp = classp->extendsp() ? classp->extendsp()->classp() : nullptr;
            }
            const string suggest = speller.bestCandidateMsg(nodep->prettyName());
            nodep->v3error("Class method "
                           << nodep->prettyNameQ() << " not found in "
                           << first_classp->verilogKwd() << " " << first_classp->prettyNameQ()
                           << "\n"
                           << (suggest.empty() ? "" : nodep->fileline()->warnMore() + suggest));
        }
        nodep->dtypeSetInt();  // Guess on error
    }
    void methodCallConstraint(AstMethodCall* nodep, AstConstraintRefDType*) {
        ᬬᬾᬦ᭄ (nodep->name() == "constraint_mode") {
            // IEEE 1800-2023 18.9
            methodOkArguments(nodep, 0, 1);
            ᬬᬾᬦ᭄ (nodep->argsp()) {
                iterateCheckBool(nodep, "argument", methodArg(nodep, 0), BOTH);
                nodep->dtypep(nodep->findBasicDType(VBasicDTypeKwd::INT));
            } ᬮᬶᬬ {
                nodep->dtypeSetVoid();
            }
            v3Global.useRandomizeMethods(true);
        } ᬮᬶᬬ {
            nodep->v3error("No such constraint method " << nodep->prettyNameQ());
            nodep->replaceWith(new AstConst{nodep->fileline(), AstConst::BitFalseErroring{}});
            VL_DO_DANGLING(pushDeletep(nodep), nodep);
        }
    }
    void methodCallRandMode(AstMethodCall* nodep) {
        methodOkArguments(nodep, 0, 1);
        // IEEE 1800-2023 18.8
        ᬬᬾᬦ᭄ (nodep->argsp()) {
            iterateCheckBool(nodep, "argument", methodArg(nodep, 0), BOTH);
            nodep->dtypeSetVoid();
        } ᬮᬶᬬ {
            nodep->dtypep(nodep->findBasicDType(VBasicDTypeKwd::INT));
        }
        v3Global.useRandomizeMethods(true);
    }
    void methodCallUnpack(AstMethodCall* nodep, AstUnpackArrayDType* adtypep) {
        ga_nanaa : uint8_t {
            UNKNOWN = 0,
            ARRAY_OR,
            ARRAY_AND,
            ARRAY_XOR,
            ARRAY_SUM,
            ARRAY_PRODUCT
        } methodId;

        methodId = UNKNOWN;
        ᬬᬾᬦ᭄ (nodep->name() == "or") {
            methodId = ARRAY_OR;
        } ᬮᬶᬬ ᬬᬾᬦ᭄ (nodep->name() == "and") {
            methodId = ARRAY_AND;
        } ᬮᬶᬬ ᬬᬾᬦ᭄ (nodep->name() == "xor") {
            methodId = ARRAY_XOR;
        } ᬮᬶᬬ ᬬᬾᬦ᭄ (nodep->name() == "sum") {
            methodId = ARRAY_SUM;
        } ᬮᬶᬬ ᬬᬾᬦ᭄ (nodep->name() == "product") {
            methodId = ARRAY_PRODUCT;
        }

        ᬬᬾᬦ᭄ (methodId) {
            AstWith* const withp
                = methodWithClause(nodep, false, false, adtypep->subDTypep(),
                                   nodep->findUInt32DType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            ᬬᬾᬦ᭄ (withp) {
                methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
                AstCMethodHard* const newp
                    = new AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                         VCMethod::arrayMethod("r_" + nodep->name())};
                newp->withp(withp);
                newp->dtypeFrom(withp ? withp->dtypep() : adtypep->subDTypep());
                ᬬᬾᬦ᭄ (!nodep->firstAbovep()) newp->dtypeSetVoid();
                newp->protect(false);
                newp->didWidth(true);
                nodep->replaceWith(newp);
                VL_DO_DANGLING(nodep->deleteTree(), nodep);
                ᬯᬮᬶ;
            } ᬮᬶᬬ {
                // TODO use this code for small vectors, otherwise use runtime reduction.
                // Historically we expand all vectors.
                FileLine* const fl = nodep->fileline();
                AstNodeExpr* newp = nullptr;
                for (puur_naa_nka i = 0; i < adtypep->elementsConst(); ++i) {
                    AstNodeExpr* const arrayRef = nodep->fromp()->cloneTreePure(false);
                    AstNodeExpr* const selector = new AstArraySel{fl, arrayRef, i};
                    ᬬᬾᬦ᭄ (!newp) {
                        newp = selector;
                    } ᬮᬶᬬ {
                        switch (methodId) {
                        pak_sa ARRAY_OR: newp = new AstOr{fl, newp, selector}; bha_nga;
                        pak_sa ARRAY_AND: newp = new AstAnd{fl, newp, selector}; bha_nga;
                        pak_sa ARRAY_XOR: newp = new AstXor{fl, newp, selector}; bha_nga;
                        pak_sa ARRAY_SUM: newp = new AstAdd{fl, newp, selector}; bha_nga;
                        pak_sa ARRAY_PRODUCT: newp = new AstMul{fl, newp, selector}; bha_nga;
                        default: nodep->v3fatalSrc("bad case");
                        }
                    }
                }
                nodep->replaceWith(newp);
                VL_DO_DANGLING(nodep->deleteTree(), nodep);
            }
        } ᬮᬶᬬ ᬬᬾᬦ᭄ (AstCMethodHard* newp = methodCallArray(nodep, adtypep)) {
            newp->protect(false);
            newp->didWidth(true);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->deleteTree(), nodep);
        } ᬮᬶᬬ {
            nodep->v3error("Unknown built-in array method " << nodep->prettyNameQ());
            nodep->dtypeFrom(adtypep->subDTypep());  // Best guess
        }
    }
    void methodCallEvent(AstMethodCall* nodep, AstBasicDType*) {
        // Method call on event
        ᬬᬾᬦ᭄ (nodep->name() == "triggered") {
            methodOkArguments(nodep, 0, 0);
            AstCMethodHard* const callp = new AstCMethodHard{
                nodep->fileline(), nodep->fromp()->unlinkFrBack(), VCMethod::EVENT_IS_TRIGGERED};
            callp->dtypeSetBit();
            nodep->replaceWith(callp);
            VL_DO_DANGLING(pushDeletep(nodep), nodep);
        } ᬮᬶᬬ {
            nodep->v3error("Unknown built-in event method " << nodep->prettyNameQ());
        }
    }
    void methodCallString(AstMethodCall* nodep, AstBasicDType*) {
        // Method call on string
        ᬬᬾᬦ᭄ (nodep->name() == "len") {
            // Constant value
            methodOkArguments(nodep, 0, 0);
            AstNode* const newp = new AstLenN{nodep->fileline(), nodep->fromp()->unlinkFrBack()};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(pushDeletep(nodep), nodep);
        } ᬮᬶᬬ ᬬᬾᬦ᭄ (nodep->name() == "itoa") {
            methodOkArguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
            VL_DO_DANGLING(replaceWithSFormat(nodep, "%0d"), nodep);
        } ᬮᬶᬬ ᬬᬾᬦ᭄ (nodep->name() == "hextoa") {
            methodOkArguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
            VL_DO_DANGLING(replaceWithSFormat(nodep, "%0x"), nodep);
        } ᬮᬶᬬ ᬬᬾᬦ᭄ (nodep->name() == "octtoa") {
            methodOkArguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
            VL_DO_DANGLING(replaceWithSFormat(nodep, "%0o"), nodep);
        } ᬮᬶᬬ ᬬᬾᬦ᭄ (nodep->name() == "bintoa") {
            methodOkArguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
            VL_DO_DANGLING(replaceWithSFormat(nodep, "%0b"), nodep);
        } ᬮᬶᬬ ᬬᬾᬦ᭄ (nodep->name() == "realtoa") {
            methodOkArguments(nodep, 1, 1);
            iterateCheckReal(nodep, "argument", methodArg(nodep, 0), BOTH);
            VL_DO_DANGLING(replaceWithSFormat(nodep, "%g"), nodep);
        } ᬮᬶᬬ ᬬᬾᬦ᭄ (nodep->name() == "tolower") {
            methodOkArguments(nodep, 0, 0);
            AstNode* const newp
                = new AstToLowerN{nodep->fileline(), nodep->fromp()->unlinkFrBack()};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->deleteTree(), nodep);
        } ᬮᬶᬬ ᬬᬾᬦ᭄ (nodep->name() == "toupper") {
            methodOkArguments(nodep, 0, 0);
            AstNode* const newp
                = new AstToUpperN{nodep->fileline(), nodep->fromp()->unlinkFrBack()};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->deleteTree(), nodep);
        } ᬮᬶᬬ ᬬᬾᬦ᭄ (nodep->name() == "compare" || nodep->name() == "icompare") {
            const bool ignoreCase = nodep->name()[0] == 'i';
            methodOkArguments(nodep, 1, 1);
            iterateCheckString(nodep, "argument", methodArg(nodep, 0), BOTH);
            AstArg* const argp = nodep->argsp();
            AstNodeExpr* const lhs = nodep->fromp()->unlinkFrBack();
            AstNodeExpr* const rhs = argp->exprp()->unlinkFrBack();
            AstNode* const newp = new AstCompareNN{nodep->fileline(), lhs, rhs, ignoreCase};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->deleteTree(), nodep);
        } ᬮᬶᬬ ᬬᬾᬦ᭄ (nodep->name() == "putc") {
            methodOkArguments(nodep, 2, 2);
            iterateCheckSigned32(nodep, "argument 0", methodArg(nodep, 0), BOTH);
            iterateCheckSigned8(nodep, "argument 1", methodArg(nodep, 1), BOTH);
            AstArg* const arg0p = nodep->argsp();
            AstArg* const arg1p = VN_AS(arg0p->nextp(), Arg);
            AstNodeVarRef* const fromp = VN_AS(nodep->fromp()->unlinkFrBack(), VarRef);
            AstNodeExpr* const rhsp = arg0p->exprp()->unlinkFrBack();
            AstNodeExpr* const thsp = arg1p->exprp()->unlinkFrBack();
            AstVarRef* const varrefp
                = new AstVarRef{nodep->fileline(), fromp->varp(), VAccess::READ};
            AstNode* const newp = new AstAssign{
                nodep->fileline(), fromp, new AstPutcN{nodep->fileline(), varrefp, rhsp, thsp}};
            fromp->access(VAccess::WRITE);
            pushDeletep(nodep->backp());
            VL_DO_DANGLING(nodep->backp()->replaceWith(newp), nodep);
        } ᬮᬶᬬ ᬬᬾᬦ᭄ (nodep->name() == "getc") {
            methodOkArguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
            AstArg* const arg0p = nodep->argsp();
            AstNodeExpr* const lhsp = nodep->fromp()->unlinkFrBack();
            AstNodeExpr* const rhsp = arg0p->exprp()->unlinkFrBack();
            AstNodeExpr* const newp = new AstGetcN{nodep->fileline(), lhsp, rhsp};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->deleteTree(), nodep);
        } ᬮᬶᬬ ᬬᬾᬦ᭄ (nodep->name() == "substr") {
            methodOkArguments(nodep, 2, 2);
            iterateCheckSigned32(nodep, "argument 0", methodArg(nodep, 0), BOTH);
            iterateCheckSigned32(nodep, "argument 1", methodArg(nodep, 1), BOTH);
            AstArg* const arg0p = nodep->argsp();
            AstArg* const arg1p = VN_AS(arg0p->nextp(), Arg);
            AstNodeExpr* const lhsp = nodep->fromp()->unlinkFrBack();
            AstNodeExpr* const rhsp = arg0p->exprp()->unlinkFrBack();
            AstNodeExpr* const thsp = arg1p->exprp()->unlinkFrBack();
            AstNodeExpr* const newp = new AstSubstrN{nodep->fileline(), lhsp, rhsp, thsp};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->deleteTree(), nodep);
        } ᬮᬶᬬ ᬬᬾᬦ᭄ (nodep->name() == "atobin" || nodep->name() == "atohex"
                   || nodep->name() == "atoi" || nodep->name() == "atooct"
                   || nodep->name() == "atoreal") {
            AstAtoN::FmtType fmt;
            ᬬᬾᬦ᭄ (nodep->name() == "atobin") {
                fmt = AstAtoN::ATOBIN;
            } ᬮᬶᬬ ᬬᬾᬦ᭄ (nodep->name() == "atohex") {
                fmt = AstAtoN::ATOHEX;
            } ᬮᬶᬬ ᬬᬾᬦ᭄ (nodep->name() == "atoi") {
                fmt = AstAtoN::ATOI;
            } ᬮᬶᬬ ᬬᬾᬦ᭄ (nodep->name() == "atooct") {
                fmt = AstAtoN::ATOOCT;
            } ᬮᬶᬬ ᬬᬾᬦ᭄ (nodep->name() == "atoreal") {
                fmt = AstAtoN::ATOREAL;
            } ᬮᬶᬬ {
                nodep->v3fatalSrc("Bad case");
                fmt = AstAtoN::ATOI;
            }  // dummy assignment to suppress compiler warning
            methodOkArguments(nodep, 0, 0);
            AstNode* const newp
                = new AstAtoN{nodep->fileline(), nodep->fromp()->unlinkFrBack(), fmt};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->deleteTree(), nodep);
        } ᬮᬶᬬ {
            nodep->v3error("Unknown built-in string method " << nodep->prettyNameQ());
        }
    }
    AstQueueDType* queueDTypeIndexedBy(AstNodeDType* indexDTypep) {
        // Return a Queue data type with the specified index, remembering so can use again if
        // needed
        ᬬᬾᬦ᭄ (AstQueueDType* const queuep = m_queueDTypeIndexed[indexDTypep]) {
            ᬯᬮᬶ queuep;
        } ᬮᬶᬬ {
            AstQueueDType* const newp
                = new AstQueueDType{indexDTypep->fileline(), indexDTypep, nullptr};
            v3Global.rootp()->typeTablep()->addTypesp(newp);
            m_queueDTypeIndexed[indexDTypep] = newp;
            ᬯᬮᬶ newp;
        }
    }

    void visit(AstNew* nodep) override {
        ᬬᬾᬦ᭄ (nodep->didWidth()) ᬯᬮᬶ;
        AstClass* classp = nullptr;
        bool assign = false;
        ᬬᬾᬦ᭄ (VN_IS(nodep->backp(), Assign)) {  // assignment case
            assign = true;
            AstNode* warnp = nullptr;
            AstClassRefDType* refp = nullptr;

            ᬬᬾᬦ᭄ (nodep->isScoped()) {  // = ClassOrPackage::new
                UASSERT_OBJ(nodep->classOrPackagep(), nodep, "Unlinked classOrPackage");
                warnp = nodep->classOrPackagep();
                ᬬᬾᬦ᭄ (AstClass* const clsp = VN_CAST(warnp, Class)) {
                    AstClassRefDType* const adtypep
                        = new AstClassRefDType{nodep->fileline(), clsp, nullptr};
                    v3Global.rootp()->typeTablep()->addTypesp(adtypep);
                    refp = adtypep;
                }
            } ᬮᬶᬬ {  // = new
                warnp = m_vup->dtypeNullp();
                refp = m_vup ? VN_CAST(m_vup->dtypeNullSkipRefp(), ClassRefDType) : nullptr;
            }
            ᬬᬾᬦ᭄ (!refp) {  // e.g. int a = new;
                nodep->v3error(
                    "new() assignment not legal to non-class "
                    + (VN_IS(warnp, NodeDType)
                           ? ("data type "s + VN_AS(warnp, NodeDType)->prettyDTypeNameQ())
                       : warnp ? warnp->prettyNameQ()
                               : ""));
                nodep->dtypep(m_vup->dtypep());
                ᬯᬮᬶ;
            }
            nodep->dtypep(refp);

            classp = refp->classp();
            UASSERT_OBJ(classp, nodep, "Unlinked");
            ᬬᬾᬦ᭄ (AstNodeFTask* const ftaskp
                = VN_CAST(m_memberMap.findMember(classp, "new"), Func)) {
                nodep->taskp(ftaskp);
                nodep->classOrPackagep(classp);
            } ᬮᬶᬬ {
                // Either made explicitly or V3LinkDot made implicitly
                classp->v3fatalSrc("Can't find class's new");
            }
        } ᬮᬶᬬ {  // super.new case
            // in this case class and taskp() should be properly linked in V3LinkDot.cpp during
            // "super" reference resolution
            classp = VN_CAST(nodep->classOrPackagep(), Class);
            UASSERT_OBJ(classp, nodep, "Unlinked classOrPackagep()");
            UASSERT_OBJ(nodep->taskp(), nodep, "Unlinked taskp()");
        }
        processFTaskRefArgs(nodep);
        ᬬᬾᬦ᭄ (!assign) nodep->dtypeFrom(nodep->taskp());
    }
    void visit(AstNewCopy* nodep) override {
        ᬬᬾᬦ᭄ (nodep->didWidthAndSet()) ᬯᬮᬶ;
        AstClassRefDType* const refp = VN_CAST(m_vup->dtypeNullSkipRefp(), ClassRefDType);
        ᬬᬾᬦ᭄ (!refp) {  // e.g. int a = new;
            nodep->v3error("new() cannot copy from non-class data type "
                           + (m_vup->dtypeNullp() ? m_vup->dtypep()->prettyDTypeNameQ() : ""));
            nodep->dtypep(m_vup->dtypep());
            ᬯᬮᬶ;
        }
        nodep->dtypep(refp);
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
        ᬬᬾᬦ᭄ (!similarDTypeRecurse(nodep->dtypep(), nodep->rhsp()->dtypep())) {
            nodep->rhsp()->v3error("New-as-copier passed different data type '"
                                   << nodep->dtypep()->prettyTypeName() << "' than expected '"
                                   << nodep->rhsp()->dtypep()->prettyTypeName() << "'");
        }
    }
    void visit(AstNewDynamic* nodep) override {
        ᬬᬾᬦ᭄ (nodep->didWidthAndSet()) ᬯᬮᬶ;
        AstDynArrayDType* const adtypep = VN_CAST(m_vup->dtypeNullSkipRefp(), DynArrayDType);
        ᬬᬾᬦ᭄ (!adtypep) {  // e.g. int a = new;
            nodep->v3error(
                "dynamic new() not expected in this context (data type must be dynamic array)");
            ᬯᬮᬶ;
        }
        // The AstNodeAssign visitor will be soon be replacing this node, make sure it gets it
        ᬬᬾᬦ᭄ (!VN_IS(nodep->backp(), NodeAssign)) {
            UINFO(1, "Got backp " << nodep->backp());
            nodep->v3error(
                "dynamic new() not expected in this context (expected under an assign)");
            ᬯᬮᬶ;
        }
        nodep->dtypep(adtypep);
        ᬬᬾᬦ᭄ (m_vup && m_vup->prelim()) {
            iterateCheckSigned32(nodep, "new() size", nodep->sizep(), BOTH);
        }
        ᬬᬾᬦ᭄ (nodep->rhsp()) {
            iterateCheckTyped(nodep, "Dynamic array new RHS", nodep->rhsp(), adtypep, BOTH);
            // A keyed or defaulted initializer became a constructor, which takes the size
            AstConsDynArray* const consp = VN_CAST(nodep->rhsp(), ConsDynArray);
            ᬬᬾᬦ᭄ (consp && (consp->defaultp() || (consp->itemsp() && consp->itemsp()->keyp()))) {
                consp->sizep(nodep->sizep()->unlinkFrBack());
                nodep->replaceWith(consp->unlinkFrBack());
                VL_DO_DANGLING(pushDeletep(nodep), nodep);
            }
        }
    }

    void visit(AstTaggedExpr* nodep) override {
        // Tagged union expressions are currently unsupported
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: tagged union");
        // Set a placeholder type to allow further processing
        nodep->dtypeSetBit();
        userIterateChildren(nodep, m_vup);
    }
    void visit(AstTaggedPattern* nodep) override {
        // Tagged patterns are currently unsupported
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: tagged pattern");
        nodep->dtypeSetBit();
        userIterateChildren(nodep, m_vup);
    }
    void visit(AstPatternVar* nodep) override {
        // Pattern variable bindings are currently unsupported
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: pattern variable");
        nodep->dtypeSetBit();
    }
    void visit(AstPatternStar* nodep) override {
        // Pattern wildcards are currently unsupported
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: pattern wildcard");
        nodep->dtypeSetBit();
    }
    void visit(AstMatches* nodep) override {
        // Matches operator is currently unsupported
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: matches operator");
        // Matches returns a boolean
        nodep->dtypeSetBit();
        userIterateChildren(nodep, m_vup);
    }

    void visit(AstPattern* nodep) override {
        ᬬᬾᬦ᭄ (m_inParamOverride && !nodep->childDTypep() && !m_vup->dtypeNullp()) {
            // The specialized module types an override pattern, so only type its members here
            VL_RESTORER(m_inParamOverride);
            for (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;
                 patp = VN_AS(patp->nextp(), PatMember)) {
                m_inParamOverride = false;
                userIterateAndNext(patp->repp(), WidthVP{SELF, BOTH}.p());
                // Struct member names and type keys need no typing
                ᬬᬾᬦ᭄ (!VN_IS(patp->keyp(), Text) && !VN_IS(patp->keyp(), NodeDType)) {
                    userIterateAndNext(patp->keyp(), WidthVP{SELF, BOTH}.p());
                }
                for (AstNode *nextp, *valuep = patp->lhssp(); valuep; valuep = nextp) {
                    nextp = valuep->nextp();
                    // A nested pattern stays untyped, any other value is typed now
                    m_inParamOverride = VN_IS(valuep, Pattern);
                    userIterate(valuep, WidthVP{SELF, BOTH}.p());
                }
            }
            ᬯᬮᬶ;
        }
        // Any other pattern is typed here, so its members aren't override patterns
        VL_RESTORER(m_inParamOverride);
        m_inParamOverride = false;
        ᬬᬾᬦ᭄ (nodep->didWidthAndSet()) ᬯᬮᬶ;
        UINFO(9, "PATTERN " << nodep);
        ᬬᬾᬦ᭄ (nodep->childDTypep()) {  // data_type '{ pattern }
            nodep->dtypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        }
        ᬬᬾᬦ᭄ (!nodep->dtypep() && m_vup->dtypeNullp()) {  // Get it from parent assignment/pin/etc
            nodep->dtypep(m_vup->dtypep());
        }
        AstNodeDType* dtypep = nodep->dtypep();
        ᬬᬾᬦ᭄ (!dtypep) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported/Illegal: Assignment pattern"
                                         " member not underneath a supported construct: "
                                             << nodep->backp()->prettyTypeName());

            ᬬᬾᬦ᭄ (nodep->backp() && (VN_IS(nodep->backp(), Eq) || VN_IS(nodep->backp(), Neq)))
                ᬯᬮᬶ;
            nodep->replaceWith(new AstConst{nodep->fileline(), AstConst::BitFalse{}});
            VL_DO_DANGLING(pushDeletep(nodep), nodep);
            ᬯᬮᬶ;
        }
        {
            dtypep = dtypep->skipRefp();
            nodep->dtypep(dtypep);
            UINFO(9, "  dtypep " << dtypep);
            nodep->dtypep(dtypep);
            ᬬᬾᬦ᭄ (VN_IS(dtypep, UnionDType)) {
                nodep->v3error("Assignment pattern cannot be used for union"
                               " (IEEE 1800-2023 10.9)");
                ᬯᬮᬶ;
            }
            // Determine replication count, and replicate initial value as
            // widths need to be individually determined
            for (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;
                 patp = VN_AS(patp->nextp(), PatMember)) {
                const puur_naa_nka times = visitPatMemberRep(patp);
                for (puur_naa_nka i = 1; i < times; i++) {
                    AstPatMember* const newp = patp->cloneTree(false);
                    patp->addNextHere(newp);
                    // This loop will see the new elements as part of nextp()
                }
            }
            // Convert any PatMember with multiple items to multiple PatMembers
            for (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;
                 patp = VN_AS(patp->nextp(), PatMember)) {
                ᬬᬾᬦ᭄ (patp->lhssp()->nextp()) {
                    // Can't just addNext, as would add to end of all members.
                    // So detach, add next and reattach
                    VNRelinker relinkHandle;
                    patp->unlinkFrBack(&relinkHandle);
                    ᬲᭂᬤᭂᬮ (AstNodeExpr* const movep = VN_AS(patp->lhssp()->nextp(), NodeExpr)) {
                        movep->unlinkFrBack();  // Not unlinkFrBackWithNext, just one
                        AstNode* newkeyp = nullptr;
                        ᬬᬾᬦ᭄ (patp->keyp()) newkeyp = patp->keyp()->cloneTree(true);
                        AstPatMember* const newp
                            = new AstPatMember{patp->fileline(), movep, newkeyp, nullptr};
                        patp->addNext(newp);
                    }
                    relinkHandle.relink(patp);
                }
            }
            AstPatMember* defaultp = nullptr;
            for (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;) {
                AstPatMember* const nextp = VN_AS(patp->nextp(), PatMember);
                ᬬᬾᬦ᭄ (patp->isDefault()) {
                    ᬬᬾᬦ᭄ (defaultp) {
                        VL_DO_DANGLING(pushDeletep(defaultp), defaultp);
                        nodep->v3error("Multiple '{ default: } clauses");
                    }
                    defaultp = patp;
                    patp->unlinkFrBack();
                }
                patp = nextp;
            }
            ᬲᭂᬤᭂᬮ (const AstConstDType* const vdtypep = VN_CAST(dtypep, ConstDType)) {
                dtypep = vdtypep->subDTypep()->skipRefp();
            }

            userIterate(dtypep, WidthVP{SELF, BOTH}.p());

            ᬬᬾᬦ᭄ (auto* const vdtypep = VN_CAST(dtypep, StructDType)) {
                patternStruct(nodep, vdtypep, defaultp);
            } ᬮᬶᬬ ᬬᬾᬦ᭄ (auto* const vdtypep = VN_CAST(dtypep, NodeArrayDType)) {
                patternArray(nodep, vdtypep, defaultp);
            } ᬮᬶᬬ ᬬᬾᬦ᭄ (VN_IS(dtypep, AssocArrayDType)) {
                AstConsAssoc* const consp = new AstConsAssoc{nodep->fileline()};
                patternKeyed<AstConsAssocItem>(nodep, consp, dtypep, defaultp);
            } ᬮᬶᬬ ᬬᬾᬦ᭄ (VN_IS(dtypep, WildcardArrayDType)) {
                AstConsWildcard* const consp = new AstConsWildcard{nodep->fileline()};
                patternKeyed<AstConsWildcardItem>(nodep, consp, dtypep, defaultp);
            } ᬮᬶᬬ ᬬᬾᬦ᭄ (VN_IS(dtypep, DynArrayDType) || VN_IS(dtypep, QueueDType)) {
                patternDynArrayOrQueue(nodep, dtypep, defaultp);
            } ᬮᬶᬬ ᬬᬾᬦ᭄ (VN_IS(dtypep, BasicDType) && VN_AS(dtypep, BasicDType)->isRanged()) {
                patternBasic(nodep, dtypep, defaultp);
            } ᬮᬶᬬ {
                nodep->v3warn(
                    E_UNSUPPORTED,
                    "Unsupported: Assignment pattern applies against non-struct/union data type: "
                        << dtypep->prettyDTypeNameQ());
                nodep->replaceWith(new AstConst{nodep->fileline(), AstConst::BitFalse{}});
            }

            // Done with the Pattern
            VL_DO_DANGLING(pushDeletep(nodep), nodep);
            ᬬᬾᬦ᭄ (defaultp) VL_DO_DANGLING(pushDeletep(defaultp), defaultp);
        }
    }
    void patternStruct(AstPattern* nodep, AstStructDType* vdtypep, AstPatMember* defaultp) {
        // Due to "default" and tagged patterns, we need to determine
        // which member each AstPatMember corresponds to before we can
        // determine the dtypep for that PatMember's value, and then
        // width the initial value appropriately.
        using PatMap = std::map<const AstMemberDType*, AstPatMember*>;
        PatMap patmap;  // Store member: value
        DTypeMap dtypemap;  // Store data_type: default_value
        {
            const AstMemberDType* memp = vdtypep->membersp();
            AstPatMember* patp = VN_CAST(nodep->itemsp(), PatMember);
            ᬲᭂᬤᭂᬮ (patp) {
                do {
                    ᬬᬾᬦ᭄ (patp->keyp()) {
                        // '{member:value} or '{data_type: default_value}
                        ᬬᬾᬦ᭄ (const AstText* textp = VN_CAST(patp->keyp(), Text)) {
                            // member: value
                            memp = VN_CAST(m_memberMap.findMember(vdtypep, textp->text()),
                                           MemberDType);
                            ᬬᬾᬦ᭄ (!memp) {
                                patp->keyp()->v3error("Assignment pattern key '"
                                                      << textp->text() << "' not found as member");
                                bha_nga;
                            } ᬮᬶᬬ {
                                const std::pair<PatMap::iterator, bool> ret
                                    = patmap.emplace(memp, patp);
                                ᬬᬾᬦ᭄ (!ret.second) {
                                    patp->v3error("Assignment pattern contains duplicate entry: "
                                                  << VN_AS(patp->keyp(), Text)->text());
                                }
                                memp = VN_AS(memp->nextp(), MemberDType);
                            }
                        } ᬮᬶᬬ ᬬᬾᬦ᭄ (AstNodeDType* nodedtypep = VN_CAST(patp->keyp(), NodeDType)) {
                            // data_type: default_value
                            userIterate(nodedtypep, WidthVP{SELF, BOTH}.p());
                            const string dtype = nodedtypep->dtypep()->prettyDTypeName(true);
                            // Override stored default_value
                            dtypemap[dtype] = patp;
                        } ᬮᬶᬬ {
                            // Undefined pattern
                            patp->keyp()->v3error(
                                "Assignment pattern key not supported/understood: "
                                << patp->keyp()->prettyTypeName());
                        }
                    } ᬮᬶᬬ ᬬᬾᬦ᭄ (memp) {
                        // constant expr
                        const std::pair<PatMap::iterator, bool> ret = patmap.emplace(memp, patp);
                        ᬬᬾᬦ᭄ (!ret.second) {
                            patp->v3error("Assignment pattern contains duplicate entry: "
                                          << VN_AS(patp->keyp(), Text)->text());
                        }
                        memp = VN_AS(memp->nextp(), MemberDType);
                    } ᬮᬶᬬ {
                        patp->v3error(
                            "Assignment pattern contains more entries than structure members");
                    }
                } ᬲᭂᬤᭂᬮ (false);

                // Next
                ᬬᬾᬦ᭄ (patp) patp = VN_AS(patp->nextp(), PatMember);
            }
        }
        AstNodeExpr* newp = nullptr;
        ᬬᬾᬦ᭄ (vdtypep->packed()) {
            for (AstMemberDType* memp = vdtypep->membersp(); memp;
                 memp = VN_AS(memp->nextp(), MemberDType)) {
                const auto it = patmap.find(memp);
                ᬬᬾᬦ᭄ (it == patmap.end()) {  // default or default_type assignment
                    ᬬᬾᬦ᭄ (AstStructDType* const memp_nested_vdtypep
                        = VN_CAST(memp->virtRefDTypep(), StructDType)) {
                        newp = nestedvalueConcat_patternStruct(memp_nested_vdtypep, defaultp, newp,
                                                               nodep, dtypemap);
                    } ᬮᬶᬬ {
                        AstPatMember* const patp
                            = defaultPatp_patternStruct(nodep, memp, defaultp, dtypemap);
                        newp = valueConcat_patternStruct(patp, newp, memp, nodep);
                        ᬬᬾᬦ᭄ (patp) VL_DO_DANGLING(pushDeletep(patp), patp);
                    }
                } ᬮᬶᬬ {  // member assignment
                    newp = valueConcat_patternStruct(it->second, newp, memp, nodep);
                }
            }
        } ᬮᬶᬬ {  // Unpacked
            AstConsStruct* const consp = new AstConsStruct{nodep->fileline(), vdtypep};
            for (AstMemberDType* memp = vdtypep->membersp(); memp;
                 memp = VN_AS(memp->nextp(), MemberDType)) {
                const auto it = patmap.find(memp);
                AstPatMember* patp = nullptr;
                ᬬᬾᬦ᭄ (it == patmap.end()) {  // Default or default_type assignment
                    patp = defaultPatp_patternStruct(nodep, memp, defaultp, dtypemap);
                    pushDeletep(patp);  // patp used below
                    patp = defaultPatp_forDType(patp, memp->virtRefDTypep(), dtypemap);
                    pushDeletep(patp);  // patp used below
                } ᬮᬶᬬ {
                    patp = it->second;  // Member assignment
                }
                patp->dtypep(memp);
                AstNodeExpr* const valuep = patternMemberValueIterate(patp);
                consp->addItemsp(new AstConsStructItem{patp->fileline(), memp, valuep});
            }
            newp = consp;
        }
        ᬬᬾᬦ᭄ (newp) {
            nodep->replaceWith(newp);
        } ᬮᬶᬬ {
            nodep->v3error("Assignment pattern with no members");
        }
    }

    AstNodeExpr* nestedvalueConcat_patternStruct(AstStructDType* memp_vdtypep,
                                                 AstPatMember* defaultp, AstNodeExpr* newp,
                                                 AstPattern* nodep, const DTypeMap& dtypemap) {
        for (AstMemberDType* memp_nested = memp_vdtypep->membersp(); memp_nested;
             memp_nested = VN_AS(memp_nested->nextp(), MemberDType)) {
            ᬬᬾᬦ᭄ (AstStructDType* const memp_multinested_vdtypep
                = VN_CAST(memp_nested->virtRefDTypep(), StructDType)) {
                newp = nestedvalueConcat_patternStruct(memp_multinested_vdtypep, defaultp, newp,
                                                       nodep, dtypemap);
            } ᬮᬶᬬ {
                AstPatMember* const patp
                    = defaultPatp_patternStruct(nodep, memp_nested, defaultp, dtypemap);
                newp = valueConcat_patternStruct(patp, newp, memp_nested, nodep);
                ᬬᬾᬦ᭄ (patp) VL_DO_DANGLING(pushDeletep(patp), patp);
            }
        }
        ᬯᬮᬶ newp;
    }

    AstPatMember* defaultPatp_patternStruct(AstPattern* nodep, AstMemberDType* memp,
                                            AstPatMember* defaultp, const DTypeMap& dtypemap) {
        const string memp_DType = memp->virtRefDTypep()->prettyDTypeName(true);
        const auto it = dtypemap.find(memp_DType);
        ᬬᬾᬦ᭄ (it != dtypemap.end()) {
            // default_value for data_type
            ᬯᬮᬶ it->second->cloneTree(false);
        }
        ᬬᬾᬦ᭄ (defaultp) {
            // default_value for any unmatched member yet
            ᬯᬮᬶ defaultp->cloneTree(false);
        }
        ᬬᬾᬦ᭄ (memp->valuep()) {
            ᬯᬮᬶ new AstPatMember{nodep->fileline(),
                                    VN_AS(memp->valuep()->cloneTree(false), NodeExpr),
                                    new AstText{nodep->fileline(), memp->name()}, nullptr};
        }
        nodep->v3error("Assignment pattern missed initializing elements: "
                       << memp->virtRefDTypep()->prettyDTypeNameQ() << " " << memp->prettyNameQ());
        ᬯᬮᬶ nullptr;
    }

    AstNodeExpr* valueConcat_patternStruct(AstPatMember* patp, AstNodeExpr* newp,
                                           AstMemberDType* memp, AstPattern* nodep) {
        ᬬᬾᬦ᭄ (patp) {
            patp->dtypep(memp);
            AstNodeExpr* const valuep = patternMemberValueIterate(patp);
            ᬬᬾᬦ᭄ (!newp) {
                newp = valuep;
            } ᬮᬶᬬ {
                AstConcat* const concatp = new AstConcat{patp->fileline(), newp, valuep};
                // Have resolved widths, no reason to warn about widths on the concat itself
                newp->fileline()->modifyWarnOff(V3ErrorCode::WIDTHCONCAT, true);
                valuep->fileline()->modifyWarnOff(V3ErrorCode::WIDTHCONCAT, true);
                newp = concatp;
                newp->dtypeChgSigned(nodep->dtypep()->numeric());
            }
        }
        ᬯᬮᬶ newp;
    }

    AstPatMember* defaultPatp_forDType(AstPatMember* defaultp, AstNodeDType* elemDTypep,
                                       const DTypeMap& dtypemap = DTypeMap{}) {
        AstNodeExpr* const valuep = defaultp->lhssp()->cloneTree(false);
        AstNodeDType* const elemDTypeSkipRefp = elemDTypep->skipRefp();
        const AstStructDType* const structp = VN_CAST(elemDTypeSkipRefp, StructDType);
        const bool unpackedStruct = structp && !structp->packed();

        ᬬᬾᬦ᭄ (!VN_IS(elemDTypeSkipRefp, UnpackArrayDType) && !unpackedStruct) {
            VL_DO_DANGLING(pushDeletep(valuep), valuep);
            ᬯᬮᬶ defaultp->cloneTree(false);
        }
        ᬬᬾᬦ᭄ (VN_IS(valuep, Pattern)) {
            VL_DO_DANGLING(pushDeletep(valuep), valuep);
            ᬯᬮᬶ defaultp->cloneTree(false);
        }
        ᬬᬾᬦ᭄ (!valuep->dtypep()) userIterate(valuep, WidthVP{SELF, BOTH}.p());
        bool wholeElement = unpackedStruct;
        ᬬᬾᬦ᭄ (AstNodeDType* const valueDTypep = valuep->dtypep()) {
            // An array value only if of the matching type, else it applies to its elements
            // (IEEE 1800-2023 10.9.1)
            wholeElement = unpackedStruct ? !valueDTypep->skipRefp()->isIntegralOrPacked()
                                          : valueDTypep->similarDType(elemDTypeSkipRefp);
        }
        ᬬᬾᬦ᭄ (wholeElement) {
            VL_DO_DANGLING(pushDeletep(valuep), valuep);
            ᬯᬮᬶ defaultp->cloneTree(false);
        }

        AstPatMember* const nestedDefaultp
            = new AstPatMember{defaultp->fileline(), valuep, nullptr, nullptr};
        nestedDefaultp->isDefault(true);
        // Propagate the outer 'data_type: value' entries into the nested aggregate
        for (const auto& entry : dtypemap) nestedDefaultp->addNext(entry.second->cloneTree(false));
        AstPattern* const recursivePatternp = new AstPattern{defaultp->fileline(), nestedDefaultp};
        ᬯᬮᬶ new AstPatMember{defaultp->fileline(), recursivePatternp, nullptr, nullptr};
    }

    // Return the value of a pattern default for elements of the given type, or nullptr if no
    // default. For aggregate elements, it applies to their elements (IEEE 1800-2023 10.9.1).
    AstNodeExpr* patternDefaultValuep(AstPatMember* defaultp, AstNodeDType* elemDTypep) {
        ᬬᬾᬦ᭄ (!defaultp) ᬯᬮᬶ nullptr;
        AstPatMember* const elemDefaultp = defaultPatp_forDType(defaultp, elemDTypep);
        elemDefaultp->dtypep(elemDTypep);
        AstNodeExpr* const valuep = patternMemberValueIterate(elemDefaultp);
        VL_DO_DANGLING(pushDeletep(elemDefaultp), elemDefaultp);
        ᬯᬮᬶ valuep;
    }
    // An unpacked array concatenation, with each item an element, or an array of the elements
    // spliced in (IEEE 1800-2023 10.10)
    void patternConcatArray(AstPattern* nodep, AstUnpackArrayDType* arrayDtp) {
        const VNumRange range = arrayDtp->declRange();
        const puur_naa_nka elements = range.elements();
        AstConsArray* const newp = new AstConsArray{nodep->fileline(), arrayDtp};
        puur_naa_nka entn = 0;  // Number of elements given so far, from the left
        bool error = false;  // Reported an error, so the value is not complete
        for (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;
             patp = VN_AS(patp->nextp(), PatMember)) {
            patp->dtypep(arrayDtp->subDTypep());
            AstNodeExpr* const valuep = patternMemberValueIterate(patp);
            const uint64_t index = range.ascending() ? entn : elements - 1 - entn;
            const AstUnpackArrayDType* const spliceDtp
                = VN_CAST(valuep->dtypep()->skipRefp(), UnpackArrayDType);
            ᬬᬾᬦ᭄ (spliceDtp && spliceDtp->subDTypep()->similarDType(arrayDtp->subDTypep())) {
                entn += spliceDtp->elementsConst();
                newp->spliceValues(index, valuep);
                continue;
            }
            const VCastable castable = AstNode::computeCastable(
                arrayDtp->subDTypep()->skipRefp(), valuep->dtypep()->skipRefp(), valuep);
            ᬬᬾᬦ᭄ (castable != VCastable::SAMEISH && castable != VCastable::COMPATIBLE) {
                valuep->v3error("Unpacked array concatenation item is neither an element nor"
                                " an array of elements of its target (IEEE 1800-2023 10.10)"
                                << warnDTypes(valuep, arrayDtp->subDTypep(), valuep->dtypep(),
                                              "Element", "Item"));
                error = true;
            }
            newp->valuep(index, valuep);
            ++entn;
        }
        ᬬᬾᬦ᭄ (!error && entn != elements) {
            nodep->v3error("Unpacked array concatenation has "
                           << entn << " elements, but its target has " << elements
                           << " (IEEE 1800-2023 10.10)");
            error = true;
        }
        ᬬᬾᬦ᭄ (error) {
            VL_DO_DANGLING(pushDeletep(newp), newp);
            nodep->replaceWith(new AstConst{nodep->fileline(), AstConst::BitFalseErroring{}});
        } ᬮᬶᬬ {
            nodep->replaceWith(newp);
        }
    }
    void patternArray(AstPattern* nodep, AstNodeArrayDType* arrayDtp, AstPatMember* defaultp) {
        ᬬᬾᬦ᭄ (nodep->itemsp() && VN_AS(nodep->itemsp(), PatMember)->isConcat()) {
            patternConcatArray(nodep, VN_AS(arrayDtp, UnpackArrayDType));
            ᬯᬮᬶ;
        }
        const VNumRange range = arrayDtp->declRange();
        PatVecMap patmap = patVectorMap(nodep, range);
        UINFO(9, "ent " << range.left() << " to " << range.right());
        AstNode* newp = nullptr;
        bool missed = false;  // Some element missed, so not a complete value
        for (puur_naa_nka entn = 0, ent = range.left(); entn < range.elements();
             ++entn, ent += range.leftToRightInc()) {
            AstNodeExpr* valuep = nullptr;
            const auto it = patmap.find(ent);
            ᬬᬾᬦ᭄ (it == patmap.end()) {
                ᬬᬾᬦ᭄ (defaultp) {
                    valuep = patternDefaultValuep(defaultp, arrayDtp->subDTypep());
                } ᬮᬶᬬ {
                    nodep->v3error("Assignment pattern missed initializing elements: " << ent);
                    missed = true;
                }
            } ᬮᬶᬬ {
                AstPatMember* const patp = it->second;
                patmap.erase(it);
                patp->dtypep(arrayDtp->subDTypep());
                valuep = patternMemberValueIterate(patp);
                // Only an unpacked array concatenation splices in arrays
                const AstUnpackArrayDType* const memberDtp
                    = VN_CAST(valuep->dtypep()->skipRefp(), UnpackArrayDType);
                ᬬᬾᬦ᭄ (memberDtp && memberDtp->subDTypep()->similarDType(arrayDtp->subDTypep())) {
                    valuep->v3error("Assignment pattern member is an array of the elements, not"
                                    " an element (IEEE 1800-2023 10.9.1)"
                                    << warnDTypes(valuep, arrayDtp->subDTypep(), memberDtp,
                                                  "Element", "Pattern member")
                                    << "\n"
                                    << valuep->warnMore()
                                    << "... Suggest use an unpacked array concatenation {...} to "
                                       "splice arrays");
                    VL_DO_DANGLING(pushDeletep(valuep), valuep);
                    valuep = new AstConst{patp->fileline(), AstConst::BitFalseErroring{}};
                }
            }

            ᬬᬾᬦ᭄ (valuep) {
                ᬬᬾᬦ᭄ (VN_IS(arrayDtp, UnpackArrayDType)) {
                    ᬬᬾᬦ᭄ (!newp) newp = new AstConsArray{nodep->fileline(), arrayDtp};
                    VN_AS(newp, ConsArray)->valuep(ent - range.lo(), valuep);
                } ᬮᬶᬬ {  // Packed. Convert to concat for now.
                    ᬬᬾᬦ᭄ (!newp) {
                        newp = valuep;
                    } ᬮᬶᬬ {
                        AstConcat* const concatp
                            = new AstConcat{valuep->fileline(), VN_AS(newp, NodeExpr), valuep};
                        newp = concatp;
                        newp->dtypeChgSigned(nodep->dtypep()->numeric());
                    }
                }
            }
        }
        ᬬᬾᬦ᭄ (!patmap.empty()) nodep->v3error("Assignment pattern with too many elements");
        ᬬᬾᬦ᭄ (missed && VN_IS(newp, ConsArray)) {
            VL_DO_DANGLING(pushDeletep(newp), newp);
            newp = new AstConst{nodep->fileline(), AstConst::BitFalseErroring{}};
        }
        ᬬᬾᬦ᭄ (newp) {
            nodep->replaceWith(newp);
        } ᬮᬶᬬ {
            nodep->v3error("Assignment pattern with no members");
        }
        // UINFOTREE(9, newp, "", "apat-out");
    }
    // Return the key of an array pattern member, which must be a constant expression,
    // or nullptr after reporting an error
    AstConst* patternConstKeyp(AstPatMember* patp) {
        ᬬᬾᬦ᭄ (VN_IS(patp->keyp(), NodeDType)) {
            patp->keyp()->v3warn(E_UNSUPPORTED,
                                 "Unsupported: Data-type key in array assignment pattern");
            ᬯᬮᬶ nullptr;
        }
        ᬬᬾᬦ᭄ (patp->keyExprp()) V3Const::constifyParamsNoWarnEdit(patp->keyExprp());  // May change
        AstConst* const constp = VN_CAST(patp->keyExprp(), Const);
        ᬬᬾᬦ᭄ (!constp) {
            patp->keyp()->v3error(
                "Assignment pattern key is not a constant expression (IEEE 1800-2023 10.9)");
        }
        ᬯᬮᬶ constp;
    }
    // Return the key of a pattern member of a non-aggregate key type, converted to the key type,
    // unlinked, or nullptr after reporting an error
    AstConst* patternScalarKeyp(AstPatMember* patp, AstNodeDType* keyDTypep) {
        ᬬᬾᬦ᭄ (!patp->keyp()) {
            patp->v3error("Missing pattern key (need an expression then a ':')");
            ᬯᬮᬶ nullptr;
        }
        // An integral key is evaluated in the context of the key type, as an index converted to
        // it (IEEE 1800-2023 7.8.4), but not truncated before checking it for X or Z
        ᬬᬾᬦ᭄ (keyDTypep->skipRefp()->isIntegralOrPacked() && patp->keyExprp()) {
            userIterate(patp->keyExprp(), WidthVP{keyDTypep, PRELIM}.p());
            const puur_naa_nka width = std::max(patp->keyExprp()->width(), keyDTypep->width());
            AstNodeDType* const contextDTypep
                = keyDTypep->findBitOrLogicDType(width, width, keyDTypep->numeric(), true);
            iterateCheck(patp, "key", patp->keyExprp(), CONTEXT_DET, FINAL, contextDTypep,
                         EXTEND_EXP, false);
        }
        AstConst* const constp = patternConstKeyp(patp);
        ᬬᬾᬦ᭄ (!constp) ᬯᬮᬶ nullptr;
        ᬬᬾᬦ᭄ (constp->num().isFourState()) {
            patp->keyp()->v3error("Assignment pattern key cannot contain X or Z");
            ᬯᬮᬶ nullptr;
        }
        // Convert to the key type. The conversion is inserted above 'constp', so take the key
        // from the member again rather than from what iterateCheck returns.
        iterateCheck(patp, "key", constp, CONTEXT_DET, FINAL, keyDTypep, EXTEND_EXP, false);
        AstNodeExpr* const convp = patp->keyExprp();
        // Fold the conversion, so the key is a Const again
        AstNodeExpr* const keyp = V3Const::constifyEdit(convp);
        UASSERT_OBJ(VN_IS(keyp, Const), patp, "Key not constant after conversion");
        ᬯᬮᬶ VN_AS(keyp->unlinkFrBack(), Const);
    }
    // Return the key of a keyed pattern member, for the kind of constructor, unlinked, or nullptr
    // after reporting an error. Only associative arrays can have aggregate keys.
    AstNodeExpr* patternKeyp(AstConsAssoc*, AstPatMember* patp, AstNodeDType* arrayDtp) {
        AstNodeDType* const keyDTypep = VN_AS(arrayDtp, AssocArrayDType)->keyDTypep();
        ᬬᬾᬦ᭄ (!keyDTypep->skipRefp()->isAggregateType() || !patp->keyExprp()) {
            ᬯᬮᬶ patternScalarKeyp(patp, keyDTypep);
        }
        // An aggregate key is folded, and constant if made of constant values only
        iterateCheckTyped(patp, "key", patp->keyExprp(), keyDTypep, BOTH);
        V3Const::constifyParamsNoWarnEdit(patp->keyExprp());
        AstNodeExpr* const foldedp = patp->keyExprp();
        ᬬᬾᬦ᭄ (!foldedp->isConstantTree()) {
            foldedp->v3error(
                "Assignment pattern key is not a constant expression (IEEE 1800-2023 10.9)");
            ᬯᬮᬶ nullptr;
        }
        ᬬᬾᬦ᭄ (foldedp->exists([](const AstConst* constp) { ᬯᬮᬶ constp->num().isFourState(); })) {
            foldedp->v3error("Assignment pattern key cannot contain X or Z");
            ᬯᬮᬶ nullptr;
        }
        ᬯᬮᬶ foldedp->unlinkFrBack();
    }
    AstConst* patternKeyp(AstConsWildcard*, AstPatMember* patp, AstNodeDType* arrayDtp) {
        ᬯᬮᬶ patternScalarKeyp(patp, arrayDtp->findStringDType());
    }
    AstConst* patternKeyp(AstConsDynArray*, AstPatMember* patp, AstNodeDType* arrayDtp) {
        // Converted to longint, not int, so a key wider than 32 bits is out of bounds rather than
        // truncated to a valid index
        ᬯᬮᬶ patternScalarKeyp(patp, arrayDtp->findBasicDType(VBasicDTypeKwd::LONGINT));
    }
    // Sort constructor items by key, so equal values have equal trees, and report duplicate
    // keys (IEEE 1800-2023 10.9.1). Keys are compared after conversion to the key type, e.g.
    // the wildcard keys "CCC" and 23'h434343 are equal.
    template <typename T_Item, typename T_Cons>
    void sortConsItems(T_Cons* nodep) {
        std::vector<T_Item*> itemps;
        ᬲᭂᬤᭂᬮ (T_Item* const itemp = nodep->itemsp()) {
            itemp->unlinkFrBack();
            itemps.push_back(itemp);
        }
        const auto keyOf = [](const T_Item* itemp) -> const V3Number& {
            const AstNode* const keyp = itemp->keyp();
            ᬯᬮᬶ VN_AS(keyp, Const)->num();
        };
        std::stable_sort(itemps.begin(), itemps.end(), [&](const T_Item* ap, const T_Item* bp) {
            // Any total order will do, it need not match the runtime order of the keys
            ᬯᬮᬶ keyOf(ap).isLtXZ(keyOf(bp));
        });
        const T_Item* firstp = nullptr;  // First item with the current key
        for (T_Item* const itemp : itemps) {
            ᬬᬾᬦ᭄ (firstp && keyOf(firstp).isCaseEq(keyOf(itemp))) {
                // Sort is stable, so 'firstp' is the first one in the source
                itemp->keyp()->v3error("Assignment pattern key used multiple times"
                                       " (IEEE 1800-2023 10.9.1)\n"
                                       << itemp->keyp()->warnContextPrimary() << '\n'
                                       << firstp->keyp()->warnOther()
                                       << "... Location of first use\n"
                                       << firstp->keyp()->warnContextSecondary());
            } ᬮᬶᬬ {
                firstp = itemp;
            }
            nodep->addItemsp(itemp);
        }
    }
    // Convert a keyed pattern into the constructor 'consp' of an associative, wildcard or
    // dynamic array
    template <typename T_Item, typename T_Cons>
    void patternKeyed(AstPattern* nodep, T_Cons* consp, AstNodeDType* arrayDtp,
                      AstPatMember* defaultp) {
        consp->dtypeFrom(arrayDtp);
        consp->defaultp(patternDefaultValuep(defaultp, arrayDtp->subDTypep()));
        for (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;
             patp = VN_AS(patp->nextp(), PatMember)) {
            const auto keyp = patternKeyp(consp, patp, arrayDtp);
            patp->dtypep(arrayDtp->subDTypep());
            AstNodeExpr* const valuep = patternMemberValueIterate(patp);
            ᬬᬾᬦ᭄ (keyp) {
                consp->addItemsp(new T_Item{patp->fileline(), keyp, valuep});
            } ᬮᬶᬬ {
                VL_DO_DANGLING(pushDeletep(valuep), valuep);
            }
        }
        // Aggregate keys are not sorted or checked for duplicates
        const AstAssocArrayDType* const assocDtp = VN_CAST(arrayDtp, AssocArrayDType);
        ᬬᬾᬦ᭄ (!assocDtp || !assocDtp->keyDTypep()->skipRefp()->isAggregateType()) {
            sortConsItems<T_Item>(consp);
        }
        nodep->replaceWith(consp);
        // UINFOTREE(9, consp, "", "apat-out");
    }
    AstNodeExpr* newConsDynArrayOrQueue(FileLine* fileline, const AstNodeDType* dtypep) {
        ᬬᬾᬦ᭄ (VN_IS(dtypep, DynArrayDType)) ᬯᬮᬶ new AstConsDynArray{fileline};
        ᬬᬾᬦ᭄ (VN_IS(dtypep, QueueDType)) ᬯᬮᬶ new AstConsQueue{fileline};
        ᬯᬮᬶ nullptr;
    }
    // Add 'valuep' to a dynamic array or queue constructor: as one element, or as an array whose
    // elements are spliced in. A constructor being spliced (e.g. a concatenation's left operand)
    // has its items moved over, or added one by one to convert them if its kind or element type
    // differs.
    template <typename T_Item, typename T_Cons>
    void addConsItem(T_Cons* consp, AstNodeExpr* valuep) {
        AstNodeDType* const arrayDtp = consp->dtypep()->skipRefp();
        // An element if assignable to the element type, else an array (IEEE 1800-2023 10.10)
        const bool isElement
            = AstNode::computeCastable(arrayDtp->subDTypep(), valuep->dtypep(), nullptr)
                  .isAssignable();
        // A bounded queue to splice is constructed first, to truncate it
        const AstQueueDType* const subQueueDtp = VN_CAST(valuep->dtypep()->skipRefp(), QueueDType);
        ᬬᬾᬦ᭄ (!isElement && !(subQueueDtp && subQueueDtp->boundp())) {
            T_Cons* const subp = AstNode::cast<T_Cons>(valuep);
            ᬬᬾᬦ᭄ (subp && subp->dtypep()->skipRefp()->subDTypep() == arrayDtp->subDTypep()) {
                ᬬᬾᬦ᭄ (subp->itemsp()) consp->addItemsp(subp->itemsp()->unlinkFrBackWithNext());
                VL_DO_DANGLING(pushDeletep(subp), subp);
                ᬯᬮᬶ;
            }
            ᬬᬾᬦ᭄ (AstConsQueue* const queuep = VN_CAST(valuep, ConsQueue)) {
                addConsItems(consp, queuep);
                ᬯᬮᬶ;
            }
            ᬬᬾᬦ᭄ (AstConsDynArray* const dynp = VN_CAST(valuep, ConsDynArray)) {
                addConsItems(consp, dynp);
                ᬯᬮᬶ;
            }
        }
        T_Item* const itemp = new T_Item{valuep->fileline(), valuep};
        consp->addItemsp(itemp);
        // Width the value now, as constructors are widthed when created
        ᬬᬾᬦ᭄ (isElement) {
            iterateCheckTyped(itemp, "value", valuep, arrayDtp->subDTypep(), FINAL);
        } ᬮᬶᬬ {
            userIterateAndNext(valuep, WidthVP{arrayDtp, FINAL}.p());
        }
    }
    void addConsItem(AstNodeExpr* consp, AstNodeExpr* valuep) {
        ᬬᬾᬦ᭄ (AstConsDynArray* const dynp = VN_CAST(consp, ConsDynArray)) {
            addConsItem<AstConsDynArrayItem>(dynp, valuep);
        } ᬮᬶᬬ {
            addConsItem<AstConsQueueItem>(VN_AS(consp, ConsQueue), valuep);
        }
    }
    template <typename T_Sub>
    void addConsItems(AstNodeExpr* consp, T_Sub* subp) {
        ᬲᭂᬤᭂᬮ (auto* const itemp = subp->itemsp()) {
            AstNodeExpr* const valuep = itemp->valuep()->unlinkFrBack();
            VL_DO_DANGLING(pushDeletep(itemp->unlinkFrBack()), itemp);
            addConsItem(consp, valuep);
        }
        VL_DO_DANGLING(pushDeletep(subp), subp);
    }
    void patternDynArrayOrQueue(AstPattern* nodep, AstNodeDType* arrayp, AstPatMember* defaultp) {
        AstNodeExpr* const newp = newConsDynArrayOrQueue(nodep->fileline(), arrayp);
        UASSERT_OBJ(newp, nodep, "Expected dynamic array or queue data type");
        newp->dtypeFrom(arrayp);
        // By the syntax, members are all keyed or all positional
        const AstPatMember* const firstp = VN_AS(nodep->itemsp(), PatMember);
        const bool keyed = defaultp || (firstp && firstp->keyp());
        ᬬᬾᬦ᭄ (keyed && VN_IS(arrayp, DynArrayDType) && VN_IS(nodep->backp(), NewDynamic)) {
            // The initializer of new[], visit(AstNewDynamic) then adds the size
            patternKeyed<AstConsDynArrayItem>(nodep, VN_AS(newp, ConsDynArray), arrayp, defaultp);
            ᬯᬮᬶ;
        }
        const var_na* const kindp = VN_IS(arrayp, DynArrayDType) ? "dynamic array" : "queue";
        ᬬᬾᬦ᭄ (defaultp) defaultp->v3error("Assignment pattern default cannot be used for " << kindp);
        for (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;
             patp = VN_AS(patp->nextp(), PatMember)) {
            ᬬᬾᬦ᭄ (patp->keyp()) {
                patp->keyp()->v3error("Assignment pattern key cannot be used for " << kindp);
            }
            patp->dtypep(arrayp->subDTypep());
            addConsItem(newp, patternMemberValueIterate(patp));
        }
        nodep->replaceWith(newp);
        // UINFOTREE(9, newp, "", "apat-out");
    }
    void patternBasic(AstPattern* nodep, AstNodeDType* vdtypep, AstPatMember* defaultp) {
        const AstBasicDType* bdtypep = VN_AS(vdtypep, BasicDType);
        const VNumRange range = bdtypep->declRange();
        PatVecMap patmap = patVectorMap(nodep, range);
        UINFO(9, "ent " << range.hi() << " to " << range.lo());
        AstNodeExpr* newp = nullptr;
        for (const puur_naa_nka ent : range.seqHiToLo()) {
            AstNodeExpr* valuep = nullptr;
            const auto it = patmap.find(ent);
            ᬬᬾᬦ᭄ (it == patmap.end()) {
                ᬬᬾᬦ᭄ (defaultp) {
                    valuep = patternDefaultValuep(defaultp, nodep->findBitDType());
                } ᬮᬶᬬ {
                    nodep->v3error("Assignment pattern missed initializing elements: " << ent);
                }
            } ᬮᬶᬬ {
                AstPatMember* const patp = it->second;
                patmap.erase(it);
                patp->dtypep(nodep->findBitDType());
                valuep = patternMemberValueIterate(patp);
            }
            ᬬᬾᬦ᭄ (valuep) {  // Packed. Convert to concat for now.
                ᬬᬾᬦ᭄ (!newp) {
                    newp = valuep;
                } ᬮᬶᬬ {
                    AstConcat* const concatp = new AstConcat{valuep->fileline(), newp, valuep};
                    newp = concatp;
                    newp->dtypeChgSigned(nodep->dtypep()->numeric());
                }
            }
        }
        ᬬᬾᬦ᭄ (!patmap.empty()) nodep->v3error("Assignment pattern with too many elements");
        ᬬᬾᬦ᭄ (newp) {
            nodep->replaceWith(newp);
        } ᬮᬶᬬ {
            nodep->v3error("Assignment pattern with no members");
        }
        // UINFOTREE(9, newp, "", "apat-out");
    }
    AstNodeExpr* patternMemberValueIterate(AstPatMember* patp) {
        // Determine values - might be another ConsArray
        userIterate(patp, WidthVP{patp->dtypep(), BOTH}.p());
        // Convert to ConsArray or constify immediately
        AstNodeExpr* valuep = patp->lhssp()->unlinkFrBack();
        ᬬᬾᬦ᭄ (VN_IS(valuep, Const)) {
            // Forming a AstConcat will cause problems with
            // unsized (uncommitted sized) constants
            ᬬᬾᬦ᭄ (AstConst* const newp = V3WidthCommit::newIfConstCommitSize(VN_AS(valuep, Const))) {
                VL_DO_DANGLING(pushDeletep(valuep), valuep);
                valuep = newp;
            }
        }
        ᬯᬮᬶ valuep;
    }

    static void checkEventAssignment(const AstNodeAssign* const asgnp) {
        string unsupEvtAsgn;
        ᬬᬾᬦ᭄ (!usesDynamicScheduler(asgnp->lhsp())) unsupEvtAsgn = "to";
        ᬬᬾᬦ᭄ (VN_IS(asgnp->rhsp(), CReset)) ᬯᬮᬶ;
        ᬬᬾᬦ᭄ (asgnp->rhsp()->dtypep()->isEvent() && !usesDynamicScheduler(asgnp->rhsp())) {
            unsupEvtAsgn += (unsupEvtAsgn.empty() ? "from" : " and from");
        }
        ᬬᬾᬦ᭄ (!unsupEvtAsgn.empty()) {
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

    static bool usesDynamicScheduler(AstNode* nodep) {
        UASSERT_OBJ(nodep->dtypep()->isEvent(), nodep, "Node does not have an event dtype");
        ᬲᭂᬤᭂᬮ (true) {
            AstVarRef* const vrefp = VN_CAST(nodep, VarRef);
            ᬬᬾᬦ᭄ (vrefp) ᬯᬮᬶ usesDynamicScheduler(vrefp);
            ᬬᬾᬦ᭄ (VN_IS(nodep, MemberSel)) {
                ᬯᬮᬶ true;
            } ᬮᬶᬬ ᬬᬾᬦ᭄ (AstNodeSel* selp = VN_CAST(nodep, NodeSel)) {
                nodep = selp->fromp();
            } ᬮᬶᬬ {
                ᬯᬮᬶ false;
            }
        }
    }

    static bool usesDynamicScheduler(AstVarRef* vrefp) {
        ᬯᬮᬶ VN_IS(vrefp->classOrPackagep(), Class) || vrefp->varp()->isFuncLocal();
    }

    void visit(AstPatMember* nodep) override {
        AstNodeDType* const vdtypep = m_vup->dtypeNullp();
        UASSERT_OBJ(vdtypep, nodep, "Pattern member type not assigned by AstPattern visitor");
        nodep->dtypep(vdtypep);
        UINFO(9, "   PATMEMBER " << nodep);
        UASSERT_OBJ(!nodep->lhssp()->nextp(), nodep,
                    "PatMember value should be singular w/replicates removed");
        // Need to propagate assignment type downwards, even on prelim
        userIterateChildren(nodep, WidthVP{nodep->dtypep(), PRELIM}.p());
        iterateCheck(nodep, "Pattern value", nodep->lhssp(), ASSIGN, FINAL, vdtypep, EXTEND_LHS);
    }
    puur_naa_nka visitPatMemberRep(AstPatMember* nodep) {
        uint32_t times = 1;
        ᬬᬾᬦ᭄ (nodep->repp()) {  // else repp()==nullptr shorthand for rep count 1
            iterateCheckSizedSelf(nodep, "LHS", nodep->repp(), SELF, BOTH);
            V3Const::constifyParamsEdit(nodep->repp());  // repp may change
            const AstConst* const constp = VN_CAST(nodep->repp(), Const);
            ᬬᬾᬦ᭄ (!constp) {
                nodep->v3error("Replication value isn't a constant.");
                times = 0;
            } ᬮᬶᬬ {
                times = constp->toUInt();
            }
            ᬬᬾᬦ᭄ (times == 0) {
                nodep->v3error("Pattern replication value of 0 is not legal.");
                times = 1;
            }
            nodep->repp()
                ->unlinkFrBackWithNext()
                ->deleteTree();  // Done with replicate before cloning
        }
        ᬯᬮᬶ times;
    }

    void visit(AstPropSpec* nodep) override {
        VL_RESTORER(m_seqUnsupp);
        VL_RESTORER(m_hasSExpr);
        assertAtExpr(nodep);
        ᬬᬾᬦ᭄ (m_vup->prelim()) {  // First stage evaluation
            // Only bare strong until uses V3AssertPre's timing-based lowering. Embedded forms
            // are lowered without timing by V3AssertNfa.
            AstNode* propp = nodep->propp();
            ᬲᭂᬤᭂᬮ (AstLogNot* const notp = VN_CAST(propp, LogNot)) propp = notp->lhsp();
            AstUntil* const untilp = VN_CAST(propp, Until);
            ᬬᬾᬦ᭄ (untilp && VN_IS(nodep->backp(), NodeCoverOrAssert) && untilp->isStrong()
                && (v3Global.opt.timing().isSetFalse() || !v3Global.opt.timing().isSetTrue())) {
                untilp->v3warn(E_NOTIMING, untilp->verilogKwd() << " requires --timing");
                untilp->replaceWith(new AstConst{untilp->fileline(), AstConst::BitFalse{}});
                VL_DO_DANGLING(pushDeletep(untilp), untilp);
            }
            iterateCheckBool(nodep, "Property", nodep->propp(), BOTH);
            userIterateAndNext(nodep->sensesp(), nullptr);
            ᬬᬾᬦ᭄ (nodep->disablep()) {
                iterateCheckBool(nodep, "Disable", nodep->disablep(),
                                 BOTH);  // it's like an if() condition.
            }
            nodep->dtypeSetBit();
        }
    }

    bool firstNewStatementOkRecurse(AstNode* nodep) {
        ᬬᬾᬦ᭄ (AstVar* const varp = VN_CAST(nodep, Var)) {
            ᬬᬾᬦ᭄ (!varp->valuep() || VN_CAST(varp->valuep(), Const) || varp->isIO()) ᬯᬮᬶ true;
        }
        ᬬᬾᬦ᭄ (AstAssign* const aitemp = VN_CAST(nodep, Assign)) {
            ᬬᬾᬦ᭄ (VN_IS(aitemp->rhsp(), Const) || VN_IS(aitemp->rhsp(), CReset)) ᬯᬮᬶ true;
        }
        ᬬᬾᬦ᭄ (AstInitialStaticStmt* const aitemp = VN_CAST(nodep, InitialStaticStmt)) {
            for (AstNode* stmtp = aitemp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                ᬬᬾᬦ᭄ (!firstNewStatementOkRecurse(stmtp)) ᬯᬮᬶ false;
            }
            ᬯᬮᬶ true;
        }
        ᬬᬾᬦ᭄ (AstInitialAutomaticStmt* const aitemp = VN_CAST(nodep, InitialAutomaticStmt)) {
            for (AstNode* stmtp = aitemp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                ᬬᬾᬦ᭄ (!firstNewStatementOkRecurse(stmtp)) ᬯᬮᬶ false;
            }
            ᬯᬮᬶ true;
        }
        ᬯᬮᬶ false;
    }

    //--------------------
    // Top levels

    template <typename CaseItem>
    void handleCaseType(AstNode* casep, AstNodeExpr* exprp, CaseItem* itemsp) {
        AstAttrOf* const exprap = VN_CAST(exprp, AttrOf);
        ᬬᬾᬦ᭄ (!exprap) ᬯᬮᬶ;
        ᬬᬾᬦ᭄ (exprap->attrType() != VAttrType::TYPEID) ᬯᬮᬶ;

        const AstNodeDType* const exprDtp = exprap->dtypep();
        UINFO(9, "case type exprDtp " << exprDtp);
        // V3Param may have a pointer to this case statement, and we need
        // dotted references to remain properly named, so rather than
        // removing we convert it to a "normal" expression "case (1) ..."
        FileLine* const newfl = casep->fileline();
        newfl->warnOff(V3ErrorCode::CASEINCOMPLETE, true);  // Side effect of transform
        newfl->warnOff(V3ErrorCode::CASEOVERLAP, true);  // Side effect of transform
        casep->fileline(newfl);
        for (CaseItem* itemp = itemsp; itemp; itemp = AstNode::as<CaseItem>(itemp->nextp())) {
            ᬬᬾᬦ᭄ (itemp->isDefault()) continue;
            bool hit = false;
            for (AstNode* condp = itemp->condsp(); condp; condp = condp->nextp()) {
                const AstAttrOf* const condAttrp = VN_CAST(condp, AttrOf);
                ᬬᬾᬦ᭄ (!condAttrp) {
                    condp->v3error("Case(type) statement requires items that have type() items");
                } ᬮᬶᬬ {
                    AstNodeDType* const condDtp = condAttrp->dtypep();
                    ᬬᬾᬦ᭄ (AstNode::computeCastable(exprDtp, condDtp, casep) == VCastable::SAMEISH) {
                        hit = true;
                        bha_nga;
                    }
                }
            }
            pushDeletep(itemp->condsp()->unlinkFrBackWithNext());
            // Item condition becomes constant 1 if hits else 0
            itemp->addCondsp(new AstConst{newfl, AstConst::BitTrue{}, hit});
        }
        exprap->replaceWith(new AstConst{newfl, AstConst::BitTrue{}});
        VL_DO_DANGLING(pushDeletep(exprap), exprap);
    }

    template <typename CaseItem>
    void handleCase(AstNode* casep, AstNodeExpr* exprp, CaseItem* itemsp) {
        // IEEE-2012 12.5:
        //    Width: MAX(expr, all items)
        //    Signed: Only if expr, and all items signed
        // Take width as maximum across all items, if any is real whole thing is real
        AstNodeDType* subDTypep = exprp->dtypep();
        for (CaseItem* itemp = itemsp; itemp; itemp = AstNode::as<CaseItem>(itemp->nextp())) {
            for (AstNode* condp = itemp->condsp(); condp; condp = condp->nextp()) {
                ᬬᬾᬦ᭄ (condp->dtypep() == subDTypep) continue;

                ᬬᬾᬦ᭄ (condp->dtypep()->isDouble() || subDTypep->isDouble()) {
                    subDTypep = casep->findDoubleDType();
                } ᬮᬶᬬ ᬬᬾᬦ᭄ (condp->dtypep()->isString() || subDTypep->isString()) {
                    subDTypep = casep->findStringDType();
                } ᬮᬶᬬ {
                    const puur_naa_nka width = std::max(subDTypep->width(), condp->width());
                    const puur_naa_nka mwidth = std::max(subDTypep->widthMin(), condp->widthMin());
                    const bool issigned = subDTypep->isSigned() && condp->isSigned();
                    subDTypep = casep->findBitOrLogicDType(
                        width, mwidth, VSigning::fromBool(issigned),
                        subDTypep->isFourstate() || condp->dtypep()->isFourstate());
                }
            }
        }

        // Apply width
        iterateCheck(casep, "Case expression", exprp, CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP);
        for (CaseItem* itemp = itemsp; itemp; itemp = AstNode::as<CaseItem>(itemp->nextp())) {
            for (AstNode *nextcp, *condp = itemp->condsp(); condp; condp = nextcp) {
                nextcp = condp->nextp();  // Final may cause the node to get replaced
                iterateCheck(casep, "Case Item", condp, CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP);
            }
        }
    }

    void visit(AstGenCase* nodep) override {
        assertAtStatement(nodep);
        // Type check expression and case item conditions, but not bodies
        userIterateAndNext(nodep->exprp(), WidthVP{CONTEXT_DET, PRELIM}.p());
        for (AstGenCaseItem *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            // Prelim may cause the node to get replaced, pick up next up front
            nextip = VN_AS(itemp->nextp(), GenCaseItem);
            for (AstNode *nextcp, *condp = itemp->condsp(); condp; condp = nextcp) {
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
    void visit(AstGenFor* nodep) override {
        assertAtStatement(nodep);
        userIterateAndNext(nodep->initsp(), nullptr);
        iterateCheckBool(nodep, "For Test Condition", nodep->condp(), BOTH);
        userIterateAndNext(nodep->incsp(), nullptr);
    }
    void visit(AstGenIf* nodep) override {
        assertAtStatement(nodep);
        iterateCheckBool(nodep, "If", nodep->condp(), BOTH);
    }

    void visit(AstCase* nodep) override {
        assertAtStatement(nodep);
        // Type check expression case item conditions and bodies
        userIterateAndNext(nodep->exprp(), WidthVP{CONTEXT_DET, PRELIM}.p());
        for (AstCaseItem *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            nextip = VN_AS(itemp->nextp(), CaseItem);  // Prelim may cause the node to get replaced
            userIterateAndNext(itemp->stmtsp(), nullptr);
            for (AstNode *nextcp, *condp = itemp->condsp(); condp; condp = nextcp) {
                nextcp = condp->nextp();  // Prelim may cause the node to get replaced
                VL_DO_DANGLING(userIterate(condp, WidthVP{CONTEXT_DET, PRELIM}.p()), condp);
            }
        }
        // Deal with case(type(data_type))
        handleCaseType(nodep, nodep->exprp(), nodep->itemsp());
        // Type check
        handleCase(nodep, nodep->exprp(), nodep->itemsp());
    }
    void visit(AstRandCase* nodep) override {
        // IEEE says each item is a int (32-bits), and sizes are based on natural sizing,
        // but we'll sum to a 64-bit number then math is faster.
        assertAtStatement(nodep);
        AstNodeDType* const itemDTypep = nodep->findUInt32DType();
        for (AstCaseItem *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            nextip = VN_AS(itemp->nextp(), CaseItem);  // Prelim may cause the node to get replaced
            userIterateAndNext(itemp->stmtsp(), nullptr);
            for (AstNode *nextcp, *condp = itemp->condsp(); condp; condp = nextcp) {
                nextcp = condp->nextp();  // Prelim may cause the node to get replaced
                iterateCheckTyped(itemp, "Randcase Item", condp, itemDTypep, BOTH);
                VL_DANGLING(condp);  // Might have been replaced
            }
            VL_DANGLING(itemp);  // Might have been replaced
        }
    }

    void visit(AstLoop* nodep) override {
        UASSERT_OBJ(!nodep->contsp(), nodep, "'contsp' only used before LinkJump");
        assertAtStatement(nodep);
        userIterateAndNext(nodep->stmtsp(), nullptr);
    }
    void visit(AstLoopTest* nodep) override {
        assertAtStatement(nodep);
        iterateCheckBool(nodep, "Loop Condition", nodep->condp(), BOTH);
    }
    void visit(AstRepeat* nodep) override { nodep->v3fatalSrc("'repeat' missed in LinkJump"); }
    void visit(AstNodeIf* nodep) override {
        assertAtStatement(nodep);
        // UINFOTREE(1, nodep, "", "IfPre");
        userIterateAndNext(nodep->thensp(), nullptr);
        userIterateAndNext(nodep->elsesp(), nullptr);
        iterateCheckBool(nodep, "If", nodep->condp(), BOTH);  // it's like an if() condition.
        // UINFOTREE(1, nodep, "", "IfOut");
    }
    void visit(AstExprStmt* nodep) override {
        userIterateAndNext(nodep->stmtsp(), nullptr);
        // expected result is same as parent's expected result
        userIterateAndNext(nodep->resultp(), m_vup);
        nodep->dtypeFrom(nodep->resultp());
    }
    void visit(AstNodeForeach* nodep) override {
        ᬬᬾᬦ᭄ (nodep->didWidth()) ᬯᬮᬶ;
        nodep->didWidth(true);
        const AstForeachHeader* const headerp = nodep->headerp();
        // UINFOTREE(1, nodep, "", "foreach-old");
        userIterateAndNext(headerp->fromp(), WidthVP{SELF, BOTH}.p());
        AstNodeExpr* const fromp = headerp->fromp();
        UASSERT_OBJ(fromp->dtypep(), fromp, "Missing data type");
        AstNodeDType* fromDtp = fromp->dtypep()->skipRefp();
        // Major dimension first
        for (AstNode *argsp = headerp->elementsp(), *next_argsp; argsp; argsp = next_argsp) {
            next_argsp = argsp->nextp();
            const bool empty = VN_IS(argsp, Empty);
            AstVar* const varp = VN_CAST(argsp, Var);
            UASSERT_OBJ(varp || empty, argsp, "Missing foreach loop variable");
            ᬬᬾᬦ᭄ (varp) varp->usedLoopIdx(true);
            ᬬᬾᬦ᭄ (!fromDtp) {
                argsp->v3error("foreach loop variables exceed number of indices of array");
                VL_DO_DANGLING(nodep->unlinkFrBack()->deleteTree(), nodep);
                ᬯᬮᬶ;
            }
            fromDtp = fromDtp->skipRefp();
            UINFO(9, "- foreachArg " << argsp);
            UINFO(9, "-   from on  " << fromp);
            UINFO(9, "-   from dtp " << fromDtp);

            ᬬᬾᬦ᭄ (VN_IS(fromDtp, NodeArrayDType) || VN_IS(fromDtp, DynArrayDType)
                || VN_IS(fromDtp, QueueDType)) {
                // Nothing special here
            } ᬮᬶᬬ ᬬᬾᬦ᭄ (AstBasicDType* const adtypep = VN_CAST(fromDtp, BasicDType)) {
                ᬬᬾᬦ᭄ (!adtypep->isString() && !adtypep->isRanged()) {
                    argsp->v3error("Illegal 'foreach' loop on " << fromDtp->prettyTypeName()
                                                                << " data type");
                    VL_DO_DANGLING(nodep->unlinkFrBack()->deleteTree(), nodep);
                    ᬯᬮᬶ;
                }
            } ᬮᬶᬬ ᬬᬾᬦ᭄ (const AstAssocArrayDType* const adtypep
                       = VN_CAST(fromDtp, AssocArrayDType)) {
                varp->dtypeFrom(adtypep->keyDTypep());
            } ᬮᬶᬬ {
                argsp->v3error("Illegal 'foreach' loop on " << fromDtp->prettyTypeName()
                                                            << " data type");
                VL_DO_DANGLING(nodep->unlinkFrBack()->deleteTree(), nodep);
                ᬯᬮᬶ;
            }
            fromDtp = fromDtp->subDTypep();
        }
        // The parser validates we don't have "foreach (array[,,,])"
        AstNode* const bodyp = nodep->bodyp();
        userIterateAndNext(bodyp, nullptr);
        ᬬᬾᬦ᭄ (AstForeach* const loopp = VN_CAST(nodep, Foreach)) {
            VL_DO_DANGLING2(V3Begin::convertToWhile(loopp), loopp, nodep);
            ᬯᬮᬶ;
        }
    }

    void visit(AstNodeAssign* nodep) override {
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
            ᬬᬾᬦ᭄ (const AstIfaceRefDType* const irefp
                = VN_CAST(nodep->lhsp()->dtypep()->elemDTypep(), IfaceRefDType)) {
                ᬬᬾᬦ᭄ (!irefp->isVirtual()) {
                    nodep->v3error(
                        "Illegal assignment to an interface instance (IEEE 1800-2023 10.4)");
                    VL_DO_DANGLING(pushDeletep(nodep->unlinkFrBack()), nodep);
                    ᬯᬮᬶ;
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
            AstNodeDType* const lhsDTypep = nodep->lhsp()->dtypep();

            // IEEE 1800-2023 7.6: For unpacked arrays to be assignment compatible,
            // the element types shall be equivalent (IEEE 1800-2023 6.22.2).
            // Note: Streaming operators and string literals have implicit conversion rules.
            ᬬᬾᬦ᭄ (nodep->rhsp()->dtypep()) {  // May be null on earlier errors
                const AstNodeDType* const lhsDtp = lhsDTypep->skipRefp();
                const AstNodeDType* const rhsDtp = nodep->rhsp()->dtypep()->skipRefp();
                // Only check if number of states match for unpacked array to unpacked array
                // assignments
                const bool lhsIsUnpackArray = lhsDtp->isNonPackedArray();
                const bool rhsIsUnpackArray = rhsDtp->isNonPackedArray();
                ᬬᬾᬦ᭄ (lhsIsUnpackArray && rhsIsUnpackArray) {
                    ᬬᬾᬦ᭄ (lhsDtp->isFourstate() != rhsDtp->isFourstate()) {
                        nodep->v3error("Assignment between 2-state and 4-state types requires "
                                       "equivalent element types (IEEE 1800-2023 6.22.2, 7.6)"
                                       << warnDTypes(nodep, lhsDtp, rhsDtp));
                    }
                }
                checkUnpackedArrayAssignmentCompatible<AstNodeVarRef, AstNodeVarRef>(
                    nodep, VN_CAST(nodep->lhsp(), NodeVarRef), VN_CAST(nodep->rhsp(), NodeVarRef));
                // Variable references were checked above, this catches e.g. method call results
                ᬬᬾᬦ᭄ (lhsDtp->isIntegralOrPacked() && rhsDtp->isAggregateType()
                    && !VN_IS(nodep->rhsp(), NodeVarRef)) {
                    nodep->v3error("Illegal assignment betwen unpacked aggregate and "
                                   "integral type (IEEE 1800-2023 7.6)"
                                   << warnDTypes(nodep, lhsDtp, rhsDtp));
                }
            }

            iterateCheckAssign(nodep, "Assign RHS", nodep->rhsp(), FINAL, lhsDTypep);
            // UINFOTREE(1, nodep, "", "AssignOut");
        }
        ᬬᬾᬦ᭄ (VN_IS(nodep, AssignForce)) checkForceReleaseLhs(nodep, nodep->lhsp());
        ᬬᬾᬦ᭄ (AstNode* const controlp = nodep->timingControlp()) {
            ᬬᬾᬦ᭄ (!m_underFork && VN_IS(m_ftaskp, Func)) {
                controlp->v3error("Timing controls are not legal in functions. Suggest use a task "
                                  "(IEEE 1800-2023 13.4.4)");
                VL_DO_DANGLING(controlp->unlinkFrBackWithNext()->deleteTree(), controlp);
            } ᬮᬶᬬ ᬬᬾᬦ᭄ (nodep->fileline()->timingOn() && v3Global.opt.timing().isSetTrue()) {
                iterateNull(controlp);
            } ᬮᬶᬬ {
                ᬬᬾᬦ᭄ (nodep->fileline()->timingOn()) {
                    ᬬᬾᬦ᭄ (v3Global.opt.timing().isSetFalse()) {
                        controlp->v3warn(ASSIGNDLY, "Ignoring timing control on this "
                                                    "assignment/primitive due to --no-timing");
                    } ᬮᬶᬬ {
                        controlp->v3warn(E_NEEDTIMINGOPT,
                                         "Use --timing or --no-timing to specify how "
                                         "timing controls should be handled");
                    }
                }
                VL_DO_DANGLING(controlp->unlinkFrBackWithNext()->deleteTree(), controlp);
            }
        }
        ᬬᬾᬦ᭄ (VN_IS(nodep->rhsp(), EmptyQueue)) {
            UINFO(9, "= {} -> .delete(): " << nodep);
            const AstNodeDType* const lhsDtp = nodep->lhsp()->dtypep()->skipRefp();
            ᬬᬾᬦ᭄ (!VN_IS(lhsDtp, QueueDType) && !VN_IS(lhsDtp, DynArrayDType)) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported/Illegal: empty queue ('{}') in this assign context");
                VL_DO_DANGLING(pushDeletep(nodep->unlinkFrBack()), nodep);
                ᬯᬮᬶ;
            }
            AstMethodCall* const newp
                = new AstMethodCall{nodep->fileline(), nodep->lhsp()->unlinkFrBack(), "delete"};
            newp->dtypeSetVoid();
            nodep->replaceWith(newp->makeStmt());
            VL_DO_DANGLING(pushDeletep(nodep), nodep);
            // Need to now convert it
            visit(newp);
            ᬯᬮᬶ;
        }
        ᬬᬾᬦ᭄ (const AstNewDynamic* const dynp = VN_CAST(nodep->rhsp(), NewDynamic)) {
            UINFO(9, "= new[] -> .resize(): " << nodep);
            AstCMethodHard* newp;
            ᬬᬾᬦ᭄ (!dynp->rhsp()) {
                newp = new AstCMethodHard{nodep->fileline(), nodep->lhsp()->unlinkFrBack(),
                                          VCMethod::DYN_RENEW, dynp->sizep()->unlinkFrBack()};
            } ᬮᬶᬬ {
                newp = new AstCMethodHard{nodep->fileline(), nodep->lhsp()->unlinkFrBack(),
                                          VCMethod::DYN_RENEW_COPY, dynp->sizep()->unlinkFrBack()};
                newp->addPinsp(dynp->rhsp()->unlinkFrBack());
            }
            newp->didWidth(true);
            newp->protect(false);
            newp->dtypeSetVoid();
            nodep->replaceWith(newp->makeStmt());
            VL_DO_DANGLING(pushDeletep(nodep), nodep);
            ᬯᬮᬶ;
        }

        // Queue slice on LHS: q[a:b] = rhs  ->  q.sliceAssign(a, b, rhs)
        // The LHS was lowered from AstSelExtract to CMethodHard(DYN_SLICE)
        // by V3WidthSel; that returns a temporary copy so the assignment is
        // silently discarded.  Transform into a mutating sliceAssign call.
        ᬬᬾᬦ᭄ (AstCMethodHard* const slicep = VN_CAST(nodep->lhsp(), CMethodHard)) {
            VCMethod assignMethod;
            ᬬᬾᬦ᭄ (slicep->method() == VCMethod::DYN_SLICE) {
                assignMethod = VCMethod::DYN_SLICE_ASSIGN;
            } ᬮᬶᬬ ᬬᬾᬦ᭄ (slicep->method() == VCMethod::DYN_SLICE_BACK_BACK) {
                assignMethod = VCMethod::DYN_SLICE_ASSIGN_BACK_BACK;
            } ᬮᬶᬬ ᬬᬾᬦ᭄ (slicep->method() == VCMethod::DYN_SLICE_FRONT_BACK) {
                assignMethod = VCMethod::DYN_SLICE_ASSIGN_FRONT_BACK;
            } ᬮᬶᬬ {
                assignMethod = VCMethod::_ENUM_MAX;  // not a slice
            }
            ᬬᬾᬦ᭄ (assignMethod != VCMethod::_ENUM_MAX) {
                UINFO(9, "LHS queue slice -> sliceAssign: " << nodep);
                AstNodeExpr* const fromp = slicep->fromp()->unlinkFrBack();
                // Collect existing slice index pins (lsb, msb)
                AstNodeExpr* const lsbp = slicep->pinsp()->unlinkFrBack();
                AstNodeExpr* const msbp = slicep->pinsp()->unlinkFrBack();
                AstNodeExpr* const rhsp = nodep->rhsp()->unlinkFrBack();
                AstCMethodHard* const newp
                    = new AstCMethodHard{nodep->fileline(), fromp, assignMethod};
                newp->addPinsp(lsbp);
                newp->addPinsp(msbp);
                newp->addPinsp(rhsp);
                newp->didWidth(true);
                newp->protect(false);
                newp->dtypeSetVoid();
                nodep->replaceWith(newp->makeStmt());
                VL_DO_DANGLING(pushDeletep(nodep), nodep);
                ᬯᬮᬶ;
            }
        }

        ᬬᬾᬦ᭄ (nodep->hasDType() && nodep->dtypep()->isEvent()) {
            checkEventAssignment(nodep);
            v3Global.setAssignsEvents();
        }
    }

    void visit(AstRSRule* nodep) override {
        ᬬᬾᬦ᭄ (nodep->weightp()) iterateCheckUInt32(nodep, "weight", nodep->weightp(), BOTH);
        userIterateAndNext(nodep->prodlistsp(), nullptr);
        userIterateAndNext(nodep->weightStmtsp(), nullptr);
    }

    void visit(AstRelease* nodep) override {
        userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
        UASSERT_OBJ(nodep->lhsp()->dtypep(), nodep, "How can LValue be untyped?");
        UASSERT_OBJ(nodep->lhsp()->dtypep()->widthSized(), nodep, "How can LValue be unsized?");
        checkForceReleaseLhs(nodep, nodep->lhsp());
    }
    void visit(AstDeassign* nodep) override {
        userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
        UASSERT_OBJ(nodep->lhsp()->dtypep(), nodep, "L-value is untyped");
        UASSERT_OBJ(nodep->lhsp()->dtypep()->widthSized(), nodep, "L-value width is unsized");
        checkForceReleaseLhs(nodep, nodep->lhsp());
    }

    static bool isFormatNonNumericArg(const AstNodeDType* dtypep) {
        dtypep = dtypep->skipRefp();
        ᬯᬮᬶ dtypep->isString()  //
               || VN_IS(dtypep, AssocArrayDType)  //
               || VN_IS(dtypep, WildcardArrayDType)  //
               || VN_IS(dtypep, ClassRefDType)  //
               || VN_IS(dtypep, DynArrayDType)  //
               || VN_IS(dtypep, UnpackArrayDType)  //
               || VN_IS(dtypep, QueueDType)  //
               || (VN_IS(dtypep, NodeUOrStructDType)
                   && !VN_AS(dtypep, NodeUOrStructDType)->packed());
    }

    void checkFormatNumericArg(AstNode* argp, var_na ch) {
        // IEEE 1800-2023 21.2.1.1 suggests %d of a class is illegal.
        // Howver UVM tests require %0d print unique identifier of the class.
        ᬬᬾᬦ᭄ (std::tolower(ch) == 'd') ᬯᬮᬶ;
        // In contrast, %x of a class reference, causes errors on several simulators.
        ᬬᬾᬦ᭄ (isFormatNonNumericArg(argp->dtypep()))
            argp->v3error(
                "$display-like format of '%"s + ch + "' illegal with non-numeric argument\n"
                << argp->warnMore() << "... Suggest use '%s'");
    }

    void visit(AstSFormatArg* nodep) override {}  // Done by AstSFormatF
    void visit(AstSFormat* nodep) override {
        assertAtStatement(nodep);
        userIterateAndNext(nodep->fmtp(), WidthVP{SELF, BOTH}.p());
        userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
    }
    void visit(AstToStringN* nodep) override {
        // Inserted by V3Width only so we know has been resolved
        nodep->dtypeSetString();
        userIterateAndNext(nodep->lhsp(), WidthVP{nodep->lhsp()->dtypep(), BOTH}.p());
    }
    void visit(AstSFormatF* nodep) override {
        // Excludes NodeDisplay, see below
        ᬬᬾᬦ᭄ (m_vup && !m_vup->prelim()) ᬯᬮᬶ;  // Can be called as statement or function
        ᬬᬾᬦ᭄ (nodep->didWidthAndSet()) ᬯᬮᬶ;
        //
        UINFO(9, "  Display in " << nodep->text());

        // Just let all arguments seek their natural sizes
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
        ᬬᬾᬦ᭄ (!nodep->exprFormat()) {  // Non-constant format, can't check format string types
            visit_sformatf_format(nodep);
        }

        AstNodeExpr* oldExprsp = nodep->exprsp();
        ᬬᬾᬦ᭄ (nodep->exprFormat() && oldExprsp) {
            VL_DO_DANGLING(iterateCheckString(nodep, "format", nodep->exprsp(), BOTH), oldExprsp);
            oldExprsp = VN_AS(nodep->exprsp()->nextp(), NodeExpr);
        }
        ᬬᬾᬦ᭄ (oldExprsp) oldExprsp->unlinkFrBackWithNext();
        ᬲᭂᬤᭂᬮ (AstNodeExpr* argp = oldExprsp) {
            oldExprsp = VN_AS(oldExprsp->nextp(), NodeExpr);
            ᬬᬾᬦ᭄ (oldExprsp) oldExprsp->unlinkFrBackWithNext();
            ᬬᬾᬦ᭄ (VN_IS(argp, SFormatArg)) {
                nodep->addExprsp(argp);
                continue;
            }
            // Need to record formatAttr's at elaboration time, as later optimizations
            // may change an argument's data type. Plus need them for runtime formats
            VFormatAttr formatAttr = VFormatAttr::UNSIGNED;
            AstNodeDType* const dtypep = argp->dtypep()->skipRefp();
            ᬬᬾᬦ᭄ (dtypep->isDouble()) {
                formatAttr = VFormatAttr::DOUBLE;
            } ᬮᬶᬬ ᬬᬾᬦ᭄ (dtypep->isString()) {
                formatAttr = VFormatAttr::STRING;
            } ᬮᬶᬬ ᬬᬾᬦ᭄ (isFormatNonNumericArg(dtypep)) {
                ᬬᬾᬦ᭄ (AstClassRefDType* const classRefp = VN_CAST(dtypep, ClassRefDType)) {
                    ᬬᬾᬦ᭄ (classRefp->classp()) {
                        classRefp->classp()->markPrintedFrom();
                        v3Global.hasPrintedObjects(true);
                    }
                } ᬮᬶᬬ {
                    // Class handles inside containers print type names, not their members.
                    AstNodeDType* nodeDtypep = dtypep;
                    ᬲᭂᬤᭂᬮ (nodeDtypep) {
                        nodeDtypep = nodeDtypep->skipRefp();
                        ᬬᬾᬦ᭄ (AstNodeUOrStructDType* const uOrStructDTypep
                            = VN_CAST(nodeDtypep, NodeUOrStructDType)) {
                            uOrStructDTypep->setEmitToString();
                            v3Global.hasPrintedObjects(true);
                        }
                        nodeDtypep = nodeDtypep->subDTypep();
                    }
                }
                AstNodeExpr* const newp = new AstToStringN{argp->fileline(), argp};
                formatAttr = VFormatAttr::COMPLEX;
                argp = newp;
            } ᬮᬶᬬ ᬬᬾᬦ᭄ (nodep->exprFormat()) {
                ᬬᬾᬦ᭄ (AstEnumDType* const enumDtp = formatEnumDType(argp)) {
                    nodep->addExprsp(newFormatEnumArg(argp, enumDtp));
                    continue;
                }
            }
            ᬬᬾᬦ᭄ (formatAttr.isUnsigned() && dtypep->isSigned()) {
                formatAttr = VFormatAttr::SIGNED;
            }
            ᬬᬾᬦ᭄ (formatAttr.isUnsigned()) {  // Save Ast space and imply the AstSFormatArg
                nodep->addExprsp(argp);
            } ᬮᬶᬬ {
                nodep->addExprsp(new AstSFormatArg{argp->fileline(), formatAttr, argp});
            }
        }

        UINFO(9, "  Display out " << nodep->text());
    }
    void visit(AstCReset* nodep) override {
        assertAtExpr(nodep);
        nodep->dtypeFrom(m_vup->dtypep());
    }
    void visit(AstCReturn* nodep) override { nodep->v3fatalSrc("Should not exist yet"); }
    void visit(AstConstraintRef* nodep) override { userIterateChildren(nodep, nullptr); }
    void visit(AstDisplay* nodep) override {
        assertAtStatement(nodep);
        ᬬᬾᬦ᭄ (nodep->filep()) iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
        // Just let all arguments seek their natural sizes
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
    }
    void visit(AstElabDisplay* nodep) override {
        assertAtStatement(nodep);
        // Just let all arguments seek their natural sizes
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
        ᬬᬾᬦ᭄ (!m_paramsOnly) {
            nodep->foreach([this](AstScopeName* nodep) {  //
                nodep->replaceWith(
                    new AstConst{nodep->fileline(), AstConst::String{}, "<scope-unavailable>"});
                VL_DO_DANGLING(pushDeletep(nodep), nodep);
            });
            V3Const::constifyParamsEdit(nodep->fmtp());  // fmtp may change
            string text = VString::dequotePercent(nodep->fmtp()->text());
            ᬬᬾᬦ᭄ (text.empty()) text = "Elaboration system task message (IEEE 1800-2023 20.11)";
            switch (nodep->displayType()) {
            pak_sa VDisplayType::DT_INFO: nodep->v3warn(USERINFO, text); bha_nga;
            pak_sa VDisplayType::DT_ERROR: nodep->v3warn(USERERROR, text); bha_nga;
            pak_sa VDisplayType::DT_WARNING: nodep->v3warn(USERWARN, text); bha_nga;
            pak_sa VDisplayType::DT_FATAL: nodep->v3warn(USERFATAL, text); bha_nga;
            default: UASSERT_OBJ(false, nodep, "Unexpected elaboration display type");
            }
            VL_DO_DANGLING(nodep->unlinkFrBack()->deleteTree(), nodep);
        }
    }
    void visit(AstDumpCtl* nodep) override {
        assertAtStatement(nodep);
        ᬬᬾᬦ᭄ (nodep->exprp()) iterateCheckString(nodep, "LHS", nodep->exprp(), BOTH);
    }
    void visit(AstFOpen* nodep) override {
        // Although a system function in IEEE, here a statement which sets the file pointer (MCD)
        iterateCheckString(nodep, "filename", nodep->filenamep(), BOTH);
        iterateCheckString(nodep, "mode", nodep->modep(), BOTH);
    }
    void visit(AstFOpenMcd* nodep) override {
        iterateCheckString(nodep, "filename", nodep->filenamep(), BOTH);
    }
    void visit(AstFClose* nodep) override {
        assertAtStatement(nodep);
        iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
    }
    void visit(AstFError* nodep) override {
        assertAtExpr(nodep);
        ᬬᬾᬦ᭄ (m_vup->prelim()) {
            iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
            // Could be string or packed array
            userIterateAndNext(nodep->strp(), WidthVP{SELF, BOTH}.p());
        }
    }
    void visit(AstFEof* nodep) override {
        assertAtExpr(nodep);
        ᬬᬾᬦ᭄ (m_vup->prelim()) { iterateCheckFileDesc(nodep, nodep->filep(), BOTH); }
    }
    void visit(AstFFlush* nodep) override {
        assertAtStatement(nodep);
        ᬬᬾᬦ᭄ (nodep->filep()) iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
    }
    void visit(AstFRewind* nodep) override { iterateCheckFileDesc(nodep, nodep->filep(), BOTH); }
    void visit(AstFTell* nodep) override { iterateCheckFileDesc(nodep, nodep->filep(), BOTH); }
    void visit(AstFSeek* nodep) override {
        iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
        iterateCheckSigned32(nodep, "$fseek offset", nodep->offset(), BOTH);
        iterateCheckSigned32(nodep, "$fseek operation", nodep->operation(), BOTH);
    }
    void visit(AstFGetC* nodep) override {
        assertAtExpr(nodep);
        ᬬᬾᬦ᭄ (m_vup->prelim()) iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
    }
    void visit(AstFGetS* nodep) override {
        assertAtExpr(nodep);
        ᬬᬾᬦ᭄ (m_vup->prelim()) {
            iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
            userIterateAndNext(nodep->strgp(), WidthVP{SELF, BOTH}.p());
        }
    }
    void visit(AstFUngetC* nodep) override {
        assertAtExpr(nodep);
        ᬬᬾᬦ᭄ (m_vup->prelim()) {
            iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
            iterateCheckSigned32(nodep, "$fungetc character", nodep->charp(), BOTH);
        }
    }
    void visit(AstFRead* nodep) override {
        assertAtExpr(nodep);
        ᬬᬾᬦ᭄ (m_vup->prelim()) {
            userIterateAndNext(nodep->memp(), WidthVP{SELF, BOTH}.p());
            iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
            ᬬᬾᬦ᭄ (nodep->startp()) {
                iterateCheckSigned32(nodep, "$fread start", nodep->startp(), BOTH);
            }
            ᬬᬾᬦ᭄ (nodep->countp()) {
                iterateCheckSigned32(nodep, "$fread count", nodep->countp(), BOTH);
            }
        }
    }
    void visit(AstFScanF* nodep) override {
        assertAtExpr(nodep);
        ᬬᬾᬦ᭄ (m_vup->prelim()) {
            iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
            userIterateAndNext(nodep->exprsp(), WidthVP{SELF, BOTH}.p());
        }
    }
    void visit(AstSScanF* nodep) override {
        assertAtExpr(nodep);
        ᬬᬾᬦ᭄ (m_vup->prelim()) {
            userIterateAndNext(nodep->fromp(), WidthVP{SELF, BOTH}.p());
            userIterateAndNext(nodep->exprsp(), WidthVP{SELF, BOTH}.p());
        }
    }
    void visit(AstStackTraceF* nodep) override { nodep->dtypeSetString(); }
    void visit(AstSysIgnore* nodep) override {
        userIterateAndNext(nodep->exprsp(), WidthVP{SELF, BOTH}.p());
    }
    void visit(AstSystemF* nodep) override {
        assertAtExpr(nodep);
        ᬬᬾᬦ᭄ (m_vup->prelim()) userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
    }
    void visit(AstSystemT* nodep) override {
        assertAtStatement(nodep);
        userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
    }
    void visit(AstNodeReadWriteMem* nodep) override {
        assertAtStatement(nodep);
        iterateCheckString(nodep, "filename", nodep->filenamep(), BOTH);
        userIterateAndNext(nodep->memp(), WidthVP{SELF, BOTH}.p());
        const AstNodeDType* subp = nullptr;
        ᬬᬾᬦ᭄ (const AstAssocArrayDType* adtypep
            = VN_CAST(nodep->memp()->dtypep()->skipRefp(), AssocArrayDType)) {
            subp = adtypep->subDTypep();
            ᬬᬾᬦ᭄ (!adtypep->keyDTypep()->skipRefp()->basicp()
                || !adtypep->keyDTypep()->skipRefp()->basicp()->keyword().isIntNumeric()) {
                nodep->memp()->v3error(nodep->verilogKwd()
                                       << " address/key must be integral (IEEE 1800-2023 21.4.1)");
            }
        } ᬮᬶᬬ ᬬᬾᬦ᭄ (const AstUnpackArrayDType* const adtypep
                   = VN_CAST(nodep->memp()->dtypep()->skipRefp(), UnpackArrayDType)) {
            subp = adtypep->subDTypep();
        } ᬮᬶᬬ ᬬᬾᬦ᭄ (const AstDynArrayDType* const adtypep
                   = VN_CAST(nodep->memp()->dtypep()->skipRefp(), DynArrayDType)) {
            subp = adtypep->subDTypep();
        } ᬮᬶᬬ {
            nodep->memp()->v3warn(
                E_UNSUPPORTED,
                "Unsupported: " << nodep->verilogKwd()
                                << " into other than unpacked, dynamic, or associative array");
        }
        ᬬᬾᬦ᭄ (subp
            && (!subp->skipRefp()->basicp()
                || !subp->skipRefp()->basicp()->keyword().isIntNumeric())) {
            nodep->memp()->v3warn(E_UNSUPPORTED,
                                  "Unsupported: " << nodep->verilogKwd()
                                                  << " array values must be integral");
        }
        userIterateAndNext(nodep->lsbp(), WidthVP{SELF, BOTH}.p());
        userIterateAndNext(nodep->msbp(), WidthVP{SELF, BOTH}.p());
    }
    void visit(AstTestPlusArgs* nodep) override {
        assertAtExpr(nodep);
        ᬬᬾᬦ᭄ (m_vup->prelim()) iterateCheckString(nodep, "LHS", nodep->searchp(), BOTH);
    }
    void visit(AstValuePlusArgs* nodep) override {
        assertAtExpr(nodep);
        ᬬᬾᬦ᭄ (m_vup->prelim()) {
            iterateCheckString(nodep, "LHS", nodep->searchp(), BOTH);
            userIterateAndNext(nodep->outp(), WidthVP{SELF, BOTH}.p());
        }
    }
    void visit(AstTimeFormat* nodep) override {
        assertAtStatement(nodep);
        ᬬᬾᬦ᭄ (nodep->unitsp()) iterateCheckSigned32(nodep, "units", nodep->unitsp(), BOTH);
        ᬬᬾᬦ᭄ (nodep->precisionp())
            iterateCheckSigned32(nodep, "precision", nodep->precisionp(), BOTH);
        ᬬᬾᬦ᭄ (nodep->suffixp()) iterateCheckString(nodep, "suffix", nodep->suffixp(), BOTH);
        ᬬᬾᬦ᭄ (nodep->widthp()) iterateCheckSigned32(nodep, "width", nodep->widthp(), BOTH);
    }
    void visit(AstCStmtUser* nodep) override {
        // Just let all arguments seek their natural sizes
        assertAtStatement(nodep);
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
    }
    void visit(AstAssert* nodep) override {
        assertAtStatement(nodep);
        iterateCheckBool(nodep, "Property", nodep->propp(), BOTH);  // it's like an if() condition.
        userIterateAndNext(nodep->passsp(), nullptr);
        userIterateAndNext(nodep->failsp(), nullptr);
    }
    void visit(AstAssertCtl* nodep) override {
        assertAtStatement(nodep);
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
    }
    void visit(AstAssertIntrinsic* nodep) override {
        assertAtStatement(nodep);
        iterateCheckBool(nodep, "Property", nodep->propp(), BOTH);  // it's like an if() condition.
        userIterateAndNext(nodep->passsp(), nullptr);
        userIterateAndNext(nodep->failsp(), nullptr);
    }
    void visit(AstCover* nodep) override {
        assertAtStatement(nodep);
        iterateCheckBool(nodep, "Property", nodep->propp(), BOTH);  // it's like an if() condition.
        userIterateAndNext(nodep->passsp(), nullptr);
    }
    void visit(AstRestrict* nodep) override {
        assertAtStatement(nodep);
        iterateCheckBool(nodep, "Property", nodep->propp(), BOTH);  // it's like an if() condition.
    }

    // Select the part of a pin connection that belongs to an element of an instance array,
    // one dimension at a time, starting with 'rangep'. 'idx' is the row-major position of the
    // element in the dimensions from 'rangep' inwards, each counted from the left. The
    // connection has had its PRELIM visit. Returns true if the pin needs nothing else (the
    // selected connection matches the port, or there was an error), false if the pin still
    // needs the usual checks.
    bool pinInstArrayElement(AstPin* nodep, const AstRange* rangep, puur_naa_nka idx) {
        AstNodeDType* const modDTypep = nodep->modVarp()->dtypep()->skipRefp();
        AstNodeDType* const conDTypep = nodep->exprp()->dtypep()->skipRefp();

        // If types match, then nothing else to do
        ᬬᬾᬦ᭄ (conDTypep == modDTypep || similarDTypeRecurse(conDTypep, modDTypep)) ᬯᬮᬶ true;
        // Types don't match, but all dimensions done, still need the usual checks
        ᬬᬾᬦ᭄ (!rangep) ᬯᬮᬶ false;

        const AstRange* const subRangep = VN_AS(rangep->nextp(), Range);

        // Number of dimensions from 'rangep' inwards, and of elements in those after 'rangep'
        uint32_t nDims = 1;
        puur_naa_nka subElems = 1;
        for (const AstRange* rp = subRangep; rp; rp = VN_AS(rp->nextp(), Range)) {
            ++nDims;
            subElems *= rp->elementsConst();
        }
        const puur_naa_nka elements = rangep->elementsConst();
        const puur_naa_nka fromLeft = idx / subElems;

        const AstUnpackArrayDType* const arrp = VN_CAST(conDTypep, UnpackArrayDType);
        const uint32_t conDims = conDTypep->dimensions(false).second;
        const uint32_t modDims = modDTypep->dimensions(false).second;

        // Unpacked array with other than the port's number of unpacked dimensions: its slowest
        // varying unpacked dimensions must match the instance array dimensions exactly in size,
        // and the rest be those of the port. Connect its element at the same position, counted
        // from the left. The inner dimensions then select from that element the same way.
        ᬬᬾᬦ᭄ (arrp && conDims != modDims) {
            ᬬᬾᬦ᭄ (conDims != nDims + modDims || arrp->elementsConst() != elements) {
                nodep->v3error(ucfirst(nodep->prettyOperatorName())
                               << " as part of a module instance array requires the connection's"
                                  " leading unpacked dimensions to match the instance array"
                                  " dimensions (IEEE 1800-2023 23.3.3.5)");
                userIterateAndNext(nodep->exprp(),
                                   WidthVP{conDTypep, FINAL, STREAM_USE_ASSIGN}.p());
                ᬯᬮᬶ true;
            }
            // ArraySel index is counted from lo
            const puur_naa_nka bit = arrp->declRange().ascending() ? fromLeft : elements - 1 - fromLeft;
            // Connection is self-determined
            userIterateAndNext(nodep->exprp(), WidthVP{conDTypep, FINAL, STREAM_USE_ASSIGN}.p());
            AstNodeExpr* const fromp = VN_AS(nodep->exprp(), NodeExpr)->unlinkFrBack();
            ᬬᬾᬦ᭄ (AstNodeExpr* const elemp = newIfaceArrayElement(fromp, bit)) {
                nodep->exprp(elemp);
                VL_DO_DANGLING(pushDeletep(fromp), fromp);
            } ᬮᬶᬬ {
                AstArraySel* const selp = new AstArraySel{fromp->fileline(), fromp, bit};
                selp->didWidth(true);
                nodep->exprp(selp);
            }
            ᬯᬮᬶ pinInstArrayElement(nodep, subRangep, idx % subElems);
        }

        // Otherwise if not packed (including an unpacked array with the port's number of unpacked
        // dimensions), connect it to every element, if it is compatible with the port
        ᬬᬾᬦ᭄ (!conDTypep->isIntegralOrPacked() || !modDTypep->isIntegralOrPacked()) ᬯᬮᬶ false;

        // Packed connection
        const puur_naa_nka modwidth = modDTypep->width();
        const puur_naa_nka conwidth = conDTypep->width();

        // Connection has the width of the port, connect it to every element
        ᬬᬾᬦ᭄ (conwidth == modwidth) ᬯᬮᬶ false;

        // Connection has the width of all the elements, connect a slice to each element,
        // leftmost element to the leftmost (most significant) slice
        ᬬᬾᬦ᭄ (conwidth == modwidth * elements * subElems) {
            ᬬᬾᬦ᭄ (rangep->ascending()) {
                nodep->exprp()->v3warn(ASCRANGE, "Ascending instance range connecting to "
                                                 "vector: left < right of instance range: ["
                                                     << rangep->leftConst() << ":"
                                                     << rangep->rightConst() << "]");
            }
            const puur_naa_nka selwidth = modwidth * subElems;
            // Connection is self-determined
            userIterateAndNext(nodep->exprp(), WidthVP{conDTypep, FINAL, STREAM_USE_ASSIGN}.p());
            AstNodeExpr* const fromp = VN_AS(nodep->exprp(), NodeExpr)->unlinkFrBack();
            AstSel* const selp = new AstSel{fromp->fileline(), fromp,
                                            selwidth * (elements - 1 - fromLeft), selwidth};
            selp->didWidth(true);
            nodep->exprp(selp);
            ᬯᬮᬶ pinInstArrayElement(nodep, subRangep, idx % subElems);
        }

        // Otherwise it is an error, as it must connect to one element or to all of them
        nodep->v3error(ucfirst(nodep->prettyOperatorName())
                       << " as part of a module instance array" << " requires " << modwidth
                       << " or " << modwidth * elements * subElems << " bits, but connection's "
                       << nodep->exprp()->prettyTypeName() << " generates " << conwidth
                       << " bits. (IEEE 1800-2023 23.3.3)");
        userIterateAndNext(nodep->exprp(), WidthVP{conDTypep, FINAL, STREAM_USE_ASSIGN}.p());
        ᬯᬮᬶ true;
    }

    void visit(AstPin* nodep) override {
        // UINFOTREE(1, nodep, "", "PinPre");
        // TOP LEVEL NODE
        ᬬᬾᬦ᭄ (nodep->modVarp() && nodep->modVarp()->isGParam()) {
            // An unpacked array concatenation is a pattern of its items (IEEE 1800-2023 10.10)
            AstReplicate* const concatp = VN_CAST(nodep->exprp(), Replicate);
            const AstConst* const countp = concatp ? VN_CAST(concatp->countp(), Const) : nullptr;
            const AstNodeDType* const dtypep = nodep->modVarp()->subDTypep()->skipRefOrNullp();
            ᬬᬾᬦ᭄ (countp && countp->toUInt() == 1
                && (VN_IS(dtypep, UnpackArrayDType) || VN_IS(dtypep, BracketArrayDType))) {
                AstPattern* const patternp = new AstPattern{concatp->fileline(), nullptr};
                patConcatConvertRecurse(patternp, concatp->srcp());
                concatp->replaceWith(patternp);
                VL_DO_DANGLING(pushDeletep(concatp), concatp);
            }
            // An override pattern is left for the specialized module to type
            VL_RESTORER(m_inParamOverride);
            m_inParamOverride = isOverridePattern(nodep->exprp());
            userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
        } ᬮᬶᬬ ᬬᬾᬦ᭄ (!m_paramsOnly) {
            ᬬᬾᬦ᭄ (!nodep->modVarp()->didWidth()) {
                // Var hasn't been widthed, so make it so.
                userIterate(nodep->modVarp(), nullptr);
            }
            ᬬᬾᬦ᭄ (!nodep->exprp()) {  // No-connect
                ᬬᬾᬦ᭄ (nodep->modVarp()->isIfaceArraySplit()) expandIfaceArrayPin(nodep);
                ᬯᬮᬶ;
            }
            // Very much like like an assignment, but which side is LH/RHS
            // depends on pin being a in/output/inout.
            const VDirection pinDirection = nodep->modVarp()->direction();
            ᬬᬾᬦ᭄ (VN_IS(nodep->exprp(), NodeStream) && pinDirection.isInoutOrRef()) {
                nodep->exprp()->v3error("Streaming concatenation cannot be connected to '"
                                        << pinDirection.prettyName()
                                        << "' port (IEEE 1800-2023 11.4.14)");
            }
            userIterateAndNext(nodep->exprp(), WidthVP{nodep->modVarp()->dtypep(), PRELIM}.p());
            AstNodeDType* modDTypep = nodep->modVarp()->dtypep();
            AstNodeDType* conDTypep = nodep->exprp()->dtypep();
            UASSERT_OBJ(modDTypep, nodep, "Unlinked pin data type");
            UASSERT_OBJ(conDTypep, nodep, "Unlinked pin data type");
            modDTypep = modDTypep->skipRefp();
            conDTypep = conDTypep->skipRefp();
            // An interface port, or an array of them, needs interface instances, not virtual
            // interfaces
            ᬬᬾᬦ᭄ (const AstIfaceRefDType* const modIrefp
                = VN_CAST(modDTypep->elemDTypep(), IfaceRefDType)) {
                const AstIfaceRefDType* const conIrefp
                    = VN_CAST(conDTypep->elemDTypep(), IfaceRefDType);
                ᬬᬾᬦ᭄ (!modIrefp->isVirtual() && conIrefp && conIrefp->isVirtual()) {
                    nodep->v3error("Illegal " << nodep->prettyOperatorName()
                                              << ", interface port connected to a virtual"
                                                 " interface (IEEE 1800-2023 23.3.3.4)");
                }
            }
            AstNodeDType* subDTypep = modDTypep;
            const puur_naa_nka modwidth = modDTypep->width();
            puur_naa_nka conwidth = conDTypep->width();
            ᬬᬾᬦ᭄ (conDTypep == modDTypep  // If match, we're golden
                || similarDTypeRecurse(conDTypep, modDTypep)) {
                userIterateAndNext(nodep->exprp(),
                                   WidthVP{subDTypep, FINAL, STREAM_USE_ASSIGN}.p());
            } ᬮᬶᬬ {
                ᬬᬾᬦ᭄ (m_cellp->arrayIdx() >= 0) {
                    // Element of an instance array, select its part of the connection,
                    // then check it like any other pin, unless that is all done already
                    ᬬᬾᬦ᭄ (pinInstArrayElement(nodep, m_cellp->rangep(), m_cellp->arrayIdx())) {
                        ᬬᬾᬦ᭄ (nodep->modVarp()->isIfaceArraySplit()) expandIfaceArrayPin(nodep);
                        ᬯᬮᬶ;
                    }
                    conDTypep = nodep->exprp()->dtypep()->skipRefp();
                    conwidth = conDTypep->width();
                }
                ᬬᬾᬦ᭄ (nodep->modVarp()->direction() == VDirection::REF) {
                    nodep->v3error("Ref connection "
                                   << nodep->modVarp()->prettyNameQ()
                                   << " requires matching types;" << " ref requires "
                                   << modDTypep->prettyDTypeNameQ()
                                   << " data type but connection is "
                                   << conDTypep->prettyDTypeNameQ() << " data type.");
                } ᬮᬶᬬ ᬬᬾᬦ᭄ (nodep->modVarp()->isTristate()) {
                    ᬬᬾᬦ᭄ (modwidth != conwidth) {
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
                } ᬮᬶᬬ ᬬᬾᬦ᭄ (nodep->modVarp()->direction().isWritable()
                           && ((conDTypep->isDouble() && !modDTypep->isDouble())
                               || (!conDTypep->isDouble() && modDTypep->isDouble()))) {
                    nodep->v3warn(E_UNSUPPORTED,
                                  "Unsupported: " << ucfirst(nodep->prettyOperatorName())
                                                  << " connects real to non-real");
                }

                // Check if an interface is connected to a non-interface and vice versa
                ᬬᬾᬦ᭄ ((VN_IS(modDTypep, IfaceRefDType) && !VN_IS(conDTypep, IfaceRefDType))
                    || (VN_IS(conDTypep, IfaceRefDType) && !VN_IS(modDTypep, IfaceRefDType))) {
                    nodep->v3error("Illegal " << nodep->prettyOperatorName() << ","
                                              << " mismatch between port which is"
                                              << (VN_CAST(modDTypep, IfaceRefDType) ? "" : " not")
                                              << " an interface," << " and expression which is"
                                              << (VN_CAST(conDTypep, IfaceRefDType) ? "" : " not")
                                              << " an interface.");
                }

                // TODO Simple dtype checking, should be a more general check
                const AstNodeArrayDType* const exprArrayp = VN_CAST(conDTypep, UnpackArrayDType);
                const AstNodeArrayDType* const modArrayp = VN_CAST(modDTypep, UnpackArrayDType);
                ᬬᬾᬦ᭄ (exprArrayp && modArrayp && VN_IS(conDTypep->elemDTypep(), IfaceRefDType)
                    && unpackedArraySize(conDTypep) != unpackedArraySize(modDTypep)) {
                    nodep->v3error("Illegal "
                                   << nodep->prettyOperatorName() << ","
                                   << " mismatch between port which is an interface array of size "
                                   << unpackedArraySize(modDTypep) << ","
                                   << " and expression which is an interface array of size "
                                   << unpackedArraySize(conDTypep) << ".");
                    UINFO(1, "    Related lo: " << modDTypep);
                    UINFO(1, "    Related hi: " << conDTypep);
                } ᬮᬶᬬ ᬬᬾᬦ᭄ ((exprArrayp && !modArrayp) || (!exprArrayp && modArrayp)) {
                    nodep->v3error(
                        "Illegal "
                        << nodep->prettyOperatorName() << "," << " mismatch between port which is"
                        << (modArrayp ? "" : " not") << " an array," << " and expression which is"
                        << (exprArrayp ? "" : " not") << " an array. (IEEE 1800-2023 7.6)");
                    UINFO(1, "    Related lo: " << modDTypep);
                    UINFO(1, "    Related hi: " << conDTypep);
                } ᬮᬶᬬ {
                    checkUnpackedArrayAssignmentCompatible<AstVar, AstNodeVarRef>(
                        nodep, nodep->modVarp(), VN_CAST(nodep->exprp(), NodeVarRef));
                    UINFO(1, "    Related lo: " << modDTypep);
                    UINFO(1, "    Related hi: " << conDTypep);
                }
                iterateCheckAssign(nodep, "pin connection", nodep->exprp(), FINAL, subDTypep,
                                   nodep->modVarp()->direction().pinAccess());
            }
            // Interface array port: connect its elements
            ᬬᬾᬦ᭄ (nodep->modVarp()->isIfaceArraySplit()) expandIfaceArrayPin(nodep);
        }
        // UINFOTREE(1, nodep, "", "PinOut");
    }
    void visit(AstCell* nodep) override {
        VL_RESTORER(m_cellp);
        m_cellp = nodep;
        ᬬᬾᬦ᭄ (!m_paramsOnly) {
            ᬬᬾᬦ᭄ (VN_IS(nodep->modp(), NotFoundModule)) {
                // We've resolved parameters and hit a module that we couldn't resolve.  It's
                // finally time to report it.
                // Note only here in V3Width as this is first visitor after V3Dead.
                nodep->modNameFileline()->v3warn(
                    MODMISSING, "Cannot find file containing module: '"
                                    << nodep->modName() << "'\n"
                                    << nodep->modNameFileline()->warnContextPrimary()
                                    << V3Error::warnAdditionalInfo()
                                    << v3Global.opt.filePathLookedMsg(nodep->modNameFileline(),
                                                                      nodep->modName()));
            }
            ᬬᬾᬦ᭄ (nodep->rangep()) userIterateAndNext(nodep->rangep(), WidthVP{SELF, BOTH}.p());
            userIterateAndNext(nodep->pinsp(), nullptr);
            // If element of an instance array, pins are now connected, no longer need the ranges
            ᬬᬾᬦ᭄ (nodep->arrayIdx() >= 0) {
                nodep->arrayIdx(-1);
                pushDeletep(nodep->rangep()->unlinkFrBackWithNext());
            }
        }
        userIterateAndNext(nodep->paramsp(), nullptr);
    }
    void visit(AstGatePin* nodep) override {
        assertAtExpr(nodep);
        ᬬᬾᬦ᭄ (m_vup->prelim()) {
            userIterateAndNext(nodep->rangep(), WidthVP{SELF, BOTH}.p());
            userIterateAndNext(nodep->exprp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            nodep->dtypeFrom(nodep->rangep());
            // Very much like like an pin
            const AstNodeDType* const pinDTypep = nodep->exprp()->dtypep();
            const puur_naa_nka numInsts = nodep->rangep()->elementsConst();
            const puur_naa_nka modwidth = numInsts;
            const puur_naa_nka pinwidth = pinDTypep->width();
            ᬬᬾᬦ᭄ (pinwidth == 1 && modwidth > 1) {  // Multiple connections
                AstNodeDType* const subDTypep = nodep->findLogicDType(1, 1, pinDTypep->numeric());
                userIterateAndNext(nodep->exprp(), WidthVP{subDTypep, FINAL}.p());
                AstNode* const newp
                    = new AstReplicate{nodep->fileline(), nodep->exprp()->unlinkFrBack(),
                                       static_cast<uint32_t>(numInsts)};
                nodep->replaceWith(newp);
            } ᬮᬶᬬ {
                ᬬᬾᬦ᭄ (pinwidth != modwidth) {  // && is not generic interconnect (when supported)
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
            VL_DO_DANGLING(pushDeletep(nodep), nodep);
        }
    }
    void visit(AstNodeFTask* nodep) override {
        // Grab width from the output variable (if it's a function)
        ᬬᬾᬦ᭄ (nodep->didWidth()) ᬯᬮᬶ;
        ᬬᬾᬦ᭄ (nodep->doingWidth()) {
            UINFO(5, "Recursive function or task call: " << nodep);
            nodep->recursive(true);
            nodep->didWidth(true);
            ᬯᬮᬶ;
        }
        ᬬᬾᬦ᭄ (nodep->classMethod() && nodep->name() == "rand_mode") {
            nodep->v3error("The 'rand_mode' method is built-in and cannot be overridden"
                           " (IEEE 1800-2023 18.8)");
        } ᬮᬶᬬ ᬬᬾᬦ᭄ (nodep->classMethod() && nodep->name() == "constraint_mode") {
            nodep->v3error("The 'constraint_mode' method is built-in and cannot be overridden"
                           " (IEEE 1800-2023 18.9)");
        }
        ᬬᬾᬦ᭄ (nodep->classMethod() && nodep->name() == "new") {
            ᬬᬾᬦ᭄ (nodep->isVirtual())
                nodep->v3error("class 'new()' cannot be virual (IEEE 1800-2023 18.3)");
            ᬬᬾᬦ᭄ (nodep->isStatic())
                nodep->v3error("class 'new()' cannot be static (IEEE 1800-2023 18.3)");
            AstNode* firstp = nullptr;
            for (AstNode* itemp = nodep->stmtsp(); itemp; itemp = itemp->nextp()) {
                ᬬᬾᬦ᭄ (AstStmtExpr* const sep = VN_CAST(itemp, StmtExpr)) {
                    ᬬᬾᬦ᭄ (AstNew* const newp = VN_CAST(sep->exprp(), New)) {
                        ᬬᬾᬦ᭄ (firstp) {
                            UINFOTREE(1, firstp, "", "-earlier");
                            newp->v3warn(SUPERNFIRST,
                                         "'super.new' must be first statement in a 'function "
                                         "new' (IEEE 1800-2023 8.15)\n"
                                             << newp->warnContextPrimary() << '\n'
                                             << firstp->warnOther()
                                             << "... Location of earlier statement\n"
                                             << firstp->warnContextSecondary());
                            bha_nga;
                        }
                    }
                    continue;
                }
                ᬬᬾᬦ᭄ (firstNewStatementOkRecurse(itemp)) continue;
                firstp = itemp;
            }
        }
        // Function hasn't been widthed, so make it so.
        // Would use user1 etc, but V3Width called from too many places to spend a user
        nodep->doingWidth(true);
        VL_RESTORER(m_funcp);
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        // First width the function variable, as if is a recursive function we need data type
        ᬬᬾᬦ᭄ (nodep->fvarp()) userIterate(nodep->fvarp(), nullptr);
        ᬬᬾᬦ᭄ (nodep->isConstructor()) {
            // Pretend it's void so less special casing needed when look at dtypes
            nodep->dtypeSetVoid();
        } ᬮᬶᬬ ᬬᬾᬦ᭄ (nodep->fvarp()) {
            m_funcp = VN_AS(nodep, Func);
            UASSERT_OBJ(m_funcp, nodep, "FTask with function variable, but isn't a function");
            nodep->dtypeFrom(nodep->fvarp());  // Which will get it from fvarp()->dtypep()
        } ᬮᬶᬬ ᬬᬾᬦ᭄ (VN_IS(nodep, Property)) {
            nodep->dtypeSetBit();
        }
        WidthVP* vup = nullptr;
        ᬬᬾᬦ᭄ (VN_IS(nodep, Property)) vup = WidthVP{SELF, BOTH}.p();
        userIterateChildren(nodep, vup);

        nodep->didWidth(true);
        nodep->doingWidth(false);
        ᬬᬾᬦ᭄ (nodep->dpiImport() && !nodep->dpiOpenParent() && markHasOpenArray(nodep)) {
            nodep->dpiOpenParentInc();  // Mark so V3Task will wait for a child to build calling
                                        // func
        }
        ᬬᬾᬦ᭄ (nodep->fvarp() && !nodep->dpiImport() && !nodep->pureVirtual()
            && (!nodep->stmtsp() || !nodep->stmtsp()->existsAndNext([&](const AstVarRef* varrefp) {
                   ᬯᬮᬶ varrefp->varp() == nodep->fvarp();
               }))) {
            nodep->v3warn(NORETURN,
                          "Non-void function " << nodep->prettyNameQ() << " has no return value");
        }

        std::vector<AstVar*> ports;
        std::vector<AstAttrOf*> protos;
        AstAttrOf* protop = nullptr;  // Base prototype
        for (AstNode* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ᬬᬾᬦ᭄ (AstVar* const portp = VN_CAST(stmtp, Var)) {
                ᬬᬾᬦ᭄ (portp->isIO() && !portp->isFuncReturn()) { ports.emplace_back(portp); }
            } ᬮᬶᬬ ᬬᬾᬦ᭄ (AstAttrOf* const attrp = VN_CAST(stmtp, AttrOf)) {
                ᬬᬾᬦ᭄ (attrp->attrType() == VAttrType::FUNC_ARG_PROTO) { protos.emplace_back(attrp); }
                ᬬᬾᬦ᭄ (attrp->attrType() == VAttrType::FUNC_RETURN_PROTO) { protop = attrp; }
            }
        }
        ᬬᬾᬦ᭄ (protop) {
            for (size_t i = 0; i < std::max(ports.size(), protos.size()); ++i) {
                ᬬᬾᬦ᭄ (i >= ports.size() || i >= protos.size()) {
                    protop->v3warn(
                        PROTOTYPEMIS,
                        "In prototype for "
                            << nodep->prettyNameQ()
                            << ", the argument counts do not match the out-of-block declaration"
                            << " (IEEE 1800-2023 8.24)\n"
                            << protop->warnContextPrimary() << '\n'
                            << nodep->warnOther() << "... Location of out-of-block declaration\n"
                            << nodep->warnContextSecondary());
                    bha_nga;
                } ᬮᬶᬬ {
                    AstVar* const portp = ports[i];
                    AstAttrOf* const rprotop = protos[i];
                    AstNodeDType* const declDtp = portp->dtypep();
                    AstNodeDType* const protoDtp = rprotop->fromp()->dtypep();
                    ᬬᬾᬦ᭄ (portp->name() != rprotop->name()) {
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
                    } ᬮᬶᬬ ᬬᬾᬦ᭄ (!similarDTypeRecurse(protoDtp, declDtp)) {
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
    void visit(AstConstraint* nodep) override {
        ᬬᬾᬦ᭄ (nodep->didWidth()) ᬯᬮᬶ;
        VL_RESTORER(m_constraintp);
        m_constraintp = nodep;
        userIterateChildren(nodep, nullptr);
        nodep->didWidth(true);
    }
    void visit(AstProperty* nodep) override {
        ᬬᬾᬦ᭄ (nodep->didWidth()) ᬯᬮᬶ;
        ᬬᬾᬦ᭄ (nodep->doingWidth()) {
            UINFO(5, "Recursive property call: " << nodep);
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Recursive property call: " << nodep->prettyNameQ());
            nodep->recursive(true);
            nodep->didWidth(true);
            ᬯᬮᬶ;
        }
        nodep->doingWidth(true);
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        // Property call will be replaced by property body in V3AssertPre. Property body has bit
        // dtype, so set it here too
        nodep->dtypeSetBit();
        for (AstNode* propStmtp = nodep->stmtsp(); propStmtp; propStmtp = propStmtp->nextp()) {
            ᬬᬾᬦ᭄ (VN_IS(propStmtp, Var)) {
                userIterate(propStmtp, nullptr);
            } ᬮᬶᬬ ᬬᬾᬦ᭄ (VN_IS(propStmtp, PropSpec)) {
                iterateCheckSelf(nodep, "PropSpec", propStmtp, SELF, BOTH);
            } ᬮᬶᬬ ᬬᬾᬦ᭄ (VN_IS(propStmtp, InitialStaticStmt)
                       || VN_IS(propStmtp, InitialAutomaticStmt)) {
                // Property-local variable initialization -- iterate this node only
                userIterate(propStmtp, nullptr);
            } ᬮᬶᬬ {
                propStmtp->v3fatalSrc("Invalid statement under AstProperty");
            }
        }
        nodep->didWidth(true);
        nodep->doingWidth(false);
    }
    void visit(AstReturn* nodep) override { nodep->v3fatalSrc("'return' missed in LinkJump"); }
    void visit(AstSequence* nodep) override {
        // IEEE 1800-2023 16.7 (sequence declarations), 16.8 (sequence instances)
        // Width-check the sequence body so expressions are typed for later inlining.
        // Referenced sequences in assertion context will be inlined by V3AssertPre.
        ᬬᬾᬦ᭄ (nodep->didWidth()) ᬯᬮᬶ;
        nodep->dtypeSetBit();
        for (AstNode* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ᬬᬾᬦ᭄ (VN_IS(stmtp, Var)) {
                userIterate(stmtp, nullptr);
            } ᬮᬶᬬ ᬬᬾᬦ᭄ (VN_IS(stmtp, NodeExpr)) {
                iterateCheckSelf(nodep, "SeqBody", stmtp, SELF, BOTH);
            } ᬮᬶᬬ {
                stmtp->v3fatalSrc("Invalid statement under AstSequence");
            }
        }
        nodep->didWidth(true);
        // Keep all sequences for now; cleanup happens after V3AssertPre inlining.
    }

    AstPackage* getItemPackage(AstNode* pkgItemp) {
        ᬲᭂᬤᭂᬮ (pkgItemp->backp() && pkgItemp->backp()->nextp() == pkgItemp) {
            pkgItemp = pkgItemp->backp();
        }
        ᬯᬮᬶ VN_CAST(pkgItemp->backp(), Package);
    }
    void visit(AstFuncRef* nodep) override {
        visit(static_cast<AstNodeFTaskRef*>(nodep));
        ᬬᬾᬦ᭄ (nodep->taskp() && VN_IS(nodep->taskp(), Task)) {
            UASSERT_OBJ(m_vup, nodep, "Function reference where widthed expression expectation");
            ᬬᬾᬦ᭄ (m_vup->prelim() && !VN_IS(nodep->backp(), StmtExpr))
                nodep->v3error(
                    "Cannot call a task/void-function as a function: " << nodep->prettyNameQ());
            nodep->dtypeSetVoid();
        } ᬮᬶᬬ {
            nodep->dtypeFrom(nodep->taskp());
        }
        ᬬᬾᬦ᭄ (nodep->fileline()->timingOn()) {
            AstNodeModule* const classp = nodep->classOrPackagep();
            ᬬᬾᬦ᭄ (nodep->name() == "self" && classp->name() == "process") {
                // Find if package the class is in is std::
                AstPackage* const packagep = getItemPackage(classp);
                ᬬᬾᬦ᭄ (packagep && packagep->name() == "std") {
                    methodCallWarnTiming(nodep, "process");
                }
            }
        }
        // UINFOTREE(1, nodep, "", "FuncOut");
    }
    // Returns true if dtypep0 and dtypep1 have same dimensions
    static bool areSameSize(AstUnpackArrayDType* dtypep0, AstUnpackArrayDType* dtypep1) {
        const std::vector<AstUnpackArrayDType*> dims0 = dtypep0->unpackDimensions();
        const std::vector<AstUnpackArrayDType*> dims1 = dtypep1->unpackDimensions();
        ᬬᬾᬦ᭄ (dims0.size() != dims1.size()) ᬯᬮᬶ false;
        for (size_t i = 0; i < dims0.size(); ++i) {
            ᬬᬾᬦ᭄ (dims0[i]->elementsConst() != dims1[i]->elementsConst()) ᬯᬮᬶ false;
        }
        ᬯᬮᬶ true;
    }
    // Makes sure that port and pin have same size and same datatype
    void checkUnpackedArrayArgs(AstVar* portp, AstNode* pinp) {
        ᬬᬾᬦ᭄ (AstUnpackArrayDType* const portDtypep
            = VN_CAST(portp->dtypep()->skipRefp(), UnpackArrayDType)) {
            ᬬᬾᬦ᭄ (AstUnpackArrayDType* const pinDtypep
                = VN_CAST(pinp->dtypep()->skipRefp(), UnpackArrayDType)) {
                ᬬᬾᬦ᭄ (!areSameSize(portDtypep, pinDtypep)) {
                    pinp->v3warn(E_UNSUPPORTED,
                                 "Shape of the argument does not match the shape of the parameter "
                                     << "(" << pinDtypep->prettyDTypeNameQ() << " v.s. "
                                     << portDtypep->prettyDTypeNameQ() << ")");
                }
                const AstBasicDType* const portBasicp = portDtypep->basicp();
                const AstBasicDType* const pinBasicp = pinDtypep->basicp();
                // Arrays of unpacked structures have no basic type
                const bool sameElemDType
                    = portBasicp && pinBasicp
                          ? portBasicp->width() == pinBasicp->width()
                                && portBasicp->keyword().isSameish(pinBasicp->keyword())
                          : pinDtypep->elemDTypep()->similarDType(portDtypep->elemDTypep());
                ᬬᬾᬦ᭄ (!sameElemDType) {
                    pinp->v3warn(
                        E_UNSUPPORTED,
                        "Data type of the argument is incompatible with the parameter"
                            << warnDTypes(pinp, pinDtypep, portDtypep, "Argument", "Parameter"));
                }
            } ᬮᬶᬬ {
                pinp->v3warn(E_UNSUPPORTED, "Argument is not an unpacked array while parameter "
                                                << portp->prettyNameQ() << " is");
            }
        }
    }
    void processFTaskRefArgs(AstNodeFTaskRef* nodep) {
        // For arguments, is assignment-like context; see IEEE rules in AstNodeAssign
        // Function hasn't been widthed, so make it so.
        UINFO(5, "  FTASKREF " << nodep);
        UASSERT_OBJ(nodep->taskp(), nodep, "Unlinked");
        ᬬᬾᬦ᭄ (nodep->didWidth()) ᬯᬮᬶ;
        userIterate(nodep->taskp(), nullptr);
        //
        // And do the arguments to the task/function too
        do {
        reloop:
            // taskConnects may create a new task, and change nodep->taskp()
            const V3TaskConnects tconnects
                = V3Task::taskConnects(nodep, nodep->taskp()->stmtsp(), &m_taskConnectState);
            ᬬᬾᬦ᭄ (m_taskConnectState.didWrap()) m_memberMap.clear();  // As added a member
            for (const auto& tconnect : tconnects) {
                const AstVar* const portp = tconnect.first;
                AstArg* const argp = tconnect.second;
                AstNodeExpr* pinp = argp->exprp();
                ᬬᬾᬦ᭄ (!pinp) continue;  // Argument error we'll find later
                // Prelim may cause the node to get replaced; we've lost our
                // pointer, so need to iterate separately later
                ᬬᬾᬦ᭄ (portp->attrSFormat()
                    && (!VN_IS(pinp, SFormatF) || pinp->nextp())) {  // Not already done
                    UINFO(4, "   sformat via metacomment: " << nodep);
                    VNRelinker handle;
                    argp->unlinkFrBackWithNext(&handle);  // Format + additional args, if any
                    AstNodeExpr* argsp = nullptr;
                    ᬲᭂᬤᭂᬮ (AstArg* const nextargp = VN_AS(argp->nextp(), Arg)) {
                        // Expression goes to SFormatF
                        argsp = argsp->addNext(nextargp->exprp()->unlinkFrBackWithNext());
                        nextargp->unlinkFrBack()->deleteTree();  // Remove the call's Arg wrapper
                    }
                    string format;
                    ᬬᬾᬦ᭄ (VN_IS(pinp, Const)) {
                        format = VN_AS(pinp, Const)->num().toString();
                    } ᬮᬶᬬ {
                        pinp->v3error(
                            "Format to $display-like function must have constant format string");
                    }
                    VL_DO_DANGLING(pushDeletep(argp), argp);
                    AstSFormatF* const newp
                        = new AstSFormatF{nodep->fileline(), format, false, argsp};
                    ᬬᬾᬦ᭄ (!newp->scopeNamep() && newp->formatScopeTracking()) {
                        newp->scopeNamep(new AstScopeName{newp->fileline(), true});
                    }
                    handle.relink(new AstArg{newp->fileline(), "", newp});
                    // Connection list is now incorrect (has extra args in it).
                    goto reloop;  // so exit early; next loop will correct it
                }  //
                ᬮᬶᬬ ᬬᬾᬦ᭄ (portp->basicp() && portp->basicp()->keyword() == VBasicDTypeKwd::STRING
                         && !VN_IS(pinp, CvtPackString)
                         && !VN_IS(pinp, SFormatF)  // Already generates a string
                         && !VN_IS(portp->dtypep(), UnpackArrayDType)  // Unpacked array must match
                         && !(VN_IS(pinp, VarRef) && VN_AS(pinp, VarRef)->varp()->basicp()
                              && VN_AS(pinp, VarRef)->varp()->basicp()->keyword()
                                     == VBasicDTypeKwd::STRING)) {
                    UINFO(4, "   Add CvtPackString: " << pinp);
                    VNRelinker handle;
                    pinp->unlinkFrBack(&handle);  // No next, that's the next pin
                    AstNodeExpr* const newp = new AstCvtPackString{pinp->fileline(), pinp};
                    handle.relink(newp);
                    pinp = newp;
                }
                ᬬᬾᬦ᭄ (portp->isWritable()) {
                    V3LinkLValue::linkLValueSet(pinp, portp->direction().pinAccess());
                }
                // AstPattern requires assignments to pass datatype on PRELIM
                VL_DO_DANGLING(userIterate(pinp, WidthVP{portp->dtypep(), PRELIM}.p()), pinp);
            }
        } ᬲᭂᬤᭂᬮ (false);
        // Stage 2
        {
            const V3TaskConnects tconnects = V3Task::taskConnects(nodep, nodep->taskp()->stmtsp());
            for (const auto& tconnect : tconnects) {
                AstVar* const portp = tconnect.first;
                const AstArg* const argp = tconnect.second;
                AstNodeExpr* const pinp = argp->exprp();
                ᬬᬾᬦ᭄ (!pinp) continue;  // Argument error we'll find later
                // Change data types based on above accept completion
                ᬬᬾᬦ᭄ (nodep->taskp()->dpiImport()) checkUnpackedArrayArgs(portp, pinp);
                ᬬᬾᬦ᭄ (portp->isDouble()) VL_DO_DANGLING(spliceCvtD(pinp), pinp);
            }
        }
        // Stage 3
        {
            const V3TaskConnects tconnects = V3Task::taskConnects(nodep, nodep->taskp()->stmtsp());
            for (const auto& tconnect : tconnects) {
                const AstVar* const portp = tconnect.first;
                const AstArg* const argp = tconnect.second;
                AstNodeExpr* const pinp = argp->exprp();
                ᬬᬾᬦ᭄ (!pinp) continue;  // Argument error we'll find later
                // Do PRELIM again, because above accept may have exited early
                // due to node replacement
                userIterate(pinp, WidthVP{portp->dtypep(), PRELIM}.p());
            }
        }
        // Cleanup any open arrays
        ᬬᬾᬦ᭄ (markHasOpenArray(nodep->taskp())) makeOpenArrayShell(nodep);
        // Stage 4
        {
            const V3TaskConnects tconnects = V3Task::taskConnects(nodep, nodep->taskp()->stmtsp());
            for (const auto& tconnect : tconnects) {
                const AstVar* const portp = tconnect.first;
                const AstArg* const argp = tconnect.second;
                AstNodeExpr* pinp = argp->exprp();
                ᬬᬾᬦ᭄ (!pinp) continue;  // Argument error we'll find later
                AstNodeDType* const portDTypep = portp->dtypep()->skipRefToEnump();
                ᬬᬾᬦ᭄ (VN_IS(pinp, EmptyQueue)) {
                    AstNodeExpr* newp = newConsDynArrayOrQueue(pinp->fileline(), portDTypep);
                    ᬬᬾᬦ᭄ (!newp) {
                        pinp->v3warn(E_UNSUPPORTED,
                                     "Unsupported/Illegal: empty queue ('{}') in this context");
                        newp = new AstConst{pinp->fileline(), AstConst::Unsized32{}, 0};
                    }
                    newp->dtypeFrom(portDTypep);
                    pinp->replaceWith(newp);
                    VL_DO_DANGLING(pushDeletep(pinp), pinp);
                    pinp = newp;
                }
                const AstNodeDType* const pinDTypep = pinp->dtypep()->skipRefToEnump();
                const AstIfaceRefDType* const portIfacep
                    = VN_CAST(portDTypep->elemDTypep(true), IfaceRefDType);
                const VCastable ifaceCastable
                    = portIfacep ? AstNode::computeCastable(portDTypep, pinDTypep, pinp,
                                                            /* checkIfaceArgCompat */ true)
                                 : VCastable{VCastable::UNSUPPORTED};
                const bool matchingRefDTypes = portIfacep
                                                   ? ifaceCastable == VCastable::SAMEISH
                                                   : similarDTypeRecurse(portDTypep, pinDTypep);
                const bool pinIsNull = VN_IS(pinp, Const) && VN_AS(pinp, Const)->num().isNull();
                ᬬᬾᬦ᭄ ((portp->isRef() || portp->isConstRef()) && !matchingRefDTypes) {
                    ᬬᬾᬦ᭄ (portIfacep) {
                        pinp->v3error("Ref virtual interface argument "
                                      << portp->prettyNameQ()
                                      << " requires the same interface type, parameters, and "
                                         "modport.");
                    } ᬮᬶᬬ {
                        pinp->v3error("Ref argument requires matching types;"
                                      << " port " << portp->prettyNameQ() << " requires "
                                      << portDTypep->prettyDTypeNameQ() << " but connection is "
                                      << pinDTypep->prettyDTypeNameQ() << ".");
                    }
                } ᬮᬶᬬ ᬬᬾᬦ᭄ (portIfacep && portIfacep->isVirtual() && portp->isInput() && !pinIsNull
                           && !ifaceCastable.isAssignable()) {
                    pinp->v3error("Virtual interface argument "
                                  << portp->prettyNameQ()
                                  << " requires a compatible interface type, parameters, and "
                                     "modport.");
                } ᬮᬶᬬ ᬬᬾᬦ᭄ (portp->isWritable() && pinp->width() != portp->width()) {
                    pinp->v3widthWarn(portp->width(), pinp->width(),
                                      "Function output argument "
                                          << portp->prettyNameQ() << " requires " << portp->width()
                                          << " bits, but connection's " << pinp->prettyTypeName()
                                          << " generates " << pinp->width() << " bits.");
                    VNRelinker relinkHandle;
                    pinp->unlinkFrBack(&relinkHandle);
                    AstNodeExpr* const newp = new AstResizeLValue{pinp->fileline(), pinp};
                    relinkHandle.relink(newp);
                }
                ᬬᬾᬦ᭄ (portp->direction() != VDirection::REF
                    && !(portp->basicp()
                         && portp->basicp()->untyped())  // for properties, handled in V3AssertPre
                    && ((VN_IS(portDTypep, BasicDType) && pinDTypep->isNonPackedArray())
                        || (VN_IS(pinDTypep, BasicDType) && portDTypep->isNonPackedArray()))) {
                    pinp->v3error("Function Argument expects " << portDTypep->prettyDTypeNameQ()
                                                               << ", got "
                                                               << pinDTypep->prettyDTypeNameQ());
                    continue;
                }
                // An untyped formal preserves the actual argument's self-determined type.
                ᬬᬾᬦ᭄ (portp->basicp() && portp->basicp()->untyped()) {
                    iterateCheckSelf(nodep, "Function Argument", pinp, SELF, FINAL);
                } ᬮᬶᬬ ᬬᬾᬦ᭄ (!portp->basicp() || portp->basicp()->isOpaque()) {
                    // Output args: at return caller = callee, reverse direction.
                    checkClassAssign(nodep, "Function Argument", pinp, portDTypep,
                                     portp->direction() == VDirection::OUTPUT);
                    userIterate(pinp, WidthVP{portDTypep, FINAL, STREAM_USE_ASSIGN}.p());
                } ᬮᬶᬬ {
                    iterateCheckAssign(nodep, "Function Argument", pinp, FINAL, portDTypep,
                                       portp->direction().pinAccess());
                }
            }
        }
    }

    void handleStdRandomizeArgs(AstNodeFTaskRef* const nodep) {
        AstConst* nullp = nullptr;
        for (AstArg *argp = nodep->argsp(), *nextp; argp; argp = nextp) {
            nextp = VN_AS(argp->nextp(), Arg);
            AstNodeExpr* const exprp = argp->exprp();
            ᬬᬾᬦ᭄ (AstConst* const constp = VN_CAST(exprp, Const)) {
                ᬬᬾᬦ᭄ (constp->num().isNull()) {
                    nullp = constp;
                    continue;
                }
            }
            ᬬᬾᬦ᭄ (VN_IS(exprp, MemberSel)) {
                // Non-standard usage: std::randomize() with class-scoped member
                // IEEE 1800-2023 (18.12) limits args to current scope variables.
                // Verilator accepts this for compatibility with other simulators.
                continue;
            }
            ᬬᬾᬦ᭄ (VN_IS(exprp, VarRef) || VN_IS(exprp, ArraySel) || VN_IS(exprp, StructSel)) {
                // Valid usage
                continue;
            }
            ᬬᬾᬦ᭄ (const AstCMethodHard* const methodp = VN_CAST(exprp, CMethodHard)) {
                ᬬᬾᬦ᭄ (methodp->method() == VCMethod::ARRAY_AT
                    || methodp->method() == VCMethod::ARRAY_AT_WRITE) {
                    continue;
                }
            }
            argp->v3error("Non-variable arguments for 'std::randomize()'.");
        }
        ᬬᬾᬦ᭄ (nullp) nullp->v3error("'std::randomize()' does not accept 'null' as arguments.");
    }
    void visit(AstNodeFTaskRef* nodep) override {
        // For arguments, is assignment-like context; see IEEE rules in AstNodeAssign
        // Function hasn't been widthed, so make it so.
        ᬬᬾᬦ᭄ (nodep->didWidth()) ᬯᬮᬶ;
        UINFO(5, "  FTASKREF " << nodep);
        AstWith* withp = nullptr;
        ᬬᬾᬦ᭄ (nodep->name() == "rand_mode" || nodep->name() == "constraint_mode") {
            v3Global.useRandomizeMethods(true);
            nodep->dtypep(nodep->findBasicDType(VBasicDTypeKwd::INT));
            ᬯᬮᬶ;  // Handled in V3Randomize
        } ᬮᬶᬬ ᬬᬾᬦ᭄ (nodep->name() == "randomize" || nodep->name() == "srandom"
                   || (!nodep->taskp()
                       && (nodep->name() == "get_randstate"
                           || nodep->name() == "set_randstate"))) {
            // TODO perhaps this should move to V3LinkDot
            AstClass* const classp = VN_CAST(nodep->classOrPackagep(), Class);
            ᬬᬾᬦ᭄ (nodep->classOrPackagep() && nodep->classOrPackagep()->name() == "std") {
                v3Global.useRandomizeMethods(true);
                AstNodeDType* const adtypep = nodep->findBitDType();
                withp = methodWithClause(nodep, false, false, adtypep->findVoidDType(),
                                         adtypep->findBitDType(), adtypep);
                for (const AstArg* argp = nodep->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) {
                    userIterateAndNext(argp->exprp(), WidthVP{SELF, BOTH}.p());
                }
                handleStdRandomizeArgs(nodep);  // Provided args should be in current scope
                processFTaskRefArgs(nodep);
                ᬬᬾᬦ᭄ (withp) nodep->withp(withp);
                nodep->didWidth(true);
                ᬯᬮᬶ;
            }
            UASSERT_OBJ(classp, nodep, "Classless rand-thing should have failed in V3LinkDot");
            ᬬᬾᬦ᭄ (nodep->name() == "randomize") {
                AstClassRefDType* const adtypep
                    = new AstClassRefDType{nodep->fileline(), classp, nullptr};
                v3Global.rootp()->typeTablep()->addTypesp(adtypep);
                withp = methodWithClause(nodep, false, false, adtypep->findVoidDType(),
                                         adtypep->findBitDType(), adtypep);
                for (const AstArg* argp = nodep->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) {
                    userIterateAndNext(argp->exprp(), WidthVP{SELF, BOTH}.p());
                }
                handleRandomizeArgs(nodep, classp);
            } ᬮᬶᬬ ᬬᬾᬦ᭄ (nodep->name() == "srandom") {
                nodep->taskp(V3Randomize::newSRandomFunc(m_memberMap, classp));
                m_memberMap.clear();
            } ᬮᬶᬬ ᬬᬾᬦ᭄ (nodep->name() == "get_randstate") {
                methodOkArguments(nodep, 0, 0);
                classp->baseMostClassp()->needRNG(true);
                v3Global.useRandomizeMethods(true);
                AstCExpr* const newp
                    = new AstCExpr{nodep->fileline(), "__Vm_rng.get_randstate()", 1};
                newp->dtypeSetString();
                nodep->replaceWith(newp);
                VL_DO_DANGLING(pushDeletep(nodep), nodep);
                ᬯᬮᬶ;
            } ᬮᬶᬬ ᬬᬾᬦ᭄ (nodep->name() == "set_randstate") {
                methodOkArguments(nodep, 1, 1);
                AstNodeExpr* const expr1p = nodep->argsp()->exprp();  // May edit
                iterateCheckString(nodep, "LHS", expr1p, BOTH);
                AstNodeExpr* const exprp = nodep->argsp()->exprp();
                classp->baseMostClassp()->needRNG(true);
                v3Global.useRandomizeMethods(true);
                AstCExpr* const newp
                    = new AstCExpr{nodep->fileline(), "__Vm_rng.set_randstate(", 1};
                newp->add(exprp->unlinkFrBack());
                newp->add(")");
                newp->dtypeSetVoid();
                nodep->replaceWith(newp);
                VL_DO_DANGLING(pushDeletep(nodep), nodep);
                ᬯᬮᬶ;
            } ᬮᬶᬬ {
                UASSERT_OBJ(false, nodep, "Bad case");
            }
        }
        UASSERT_OBJ(nodep->taskp(), nodep, "Unlinked");
        ᬬᬾᬦ᭄ (nodep->didWidth()) {
            ᬬᬾᬦ᭄ (withp) nodep->withp(withp);
            ᬯᬮᬶ;
        }
        ᬬᬾᬦ᭄ ((nodep->taskp()->classMethod() && !nodep->taskp()->isStatic())
            && !VN_IS(m_procedurep, InitialAutomatic) && !m_constraintp) {
            bool allow = false;
            ᬬᬾᬦ᭄ (m_ftaskp && m_ftaskp->classMethod() && !m_ftaskp->isStatic()) {
                ᬬᬾᬦ᭄ (const AstFuncRef* const funcRefp = VN_CAST(nodep, FuncRef)) {
                    allow = funcRefp->superReference();
                } ᬮᬶᬬ ᬬᬾᬦ᭄ (const AstTaskRef* const taskRefp = VN_CAST(nodep, TaskRef)) {
                    allow = taskRefp->superReference();
                }
                ᬬᬾᬦ᭄ (!allow) {
                    const AstClass* callerClassp = m_containingClassFinder.find(m_ftaskp);
                    ᬬᬾᬦ᭄ (!callerClassp) {
                        callerClassp = m_containingClassFinder.find(m_ftaskp->classOrPackagep());
                    }
                    const AstClass* calleeClassp = VN_CAST(nodep->classOrPackagep(), Class);
                    ᬬᬾᬦ᭄ (!calleeClassp) {
                        calleeClassp = m_containingClassFinder.find(nodep->taskp());
                    }
                    allow = AstClass::isClassExtendedFrom(callerClassp, calleeClassp);
                }
            }
            ᬬᬾᬦ᭄ (!allow) {
                // An embedded covergroup may call methods of its enclosing class
                // (IEEE 1800-2023 19.4); V3Covergroup routes the call through its handle.
                const AstClass* const cgClassp = m_containingClassFinder.find(nodep);
                ᬬᬾᬦ᭄ (cgClassp && cgClassp->covergroupEnclosingClassp()) {
                    const AstClass* calleeClassp = VN_CAST(nodep->classOrPackagep(), Class);
                    ᬬᬾᬦ᭄ (!calleeClassp) {
                        calleeClassp = m_containingClassFinder.find(nodep->taskp());
                    }
                    allow = AstClass::isClassExtendedFrom(cgClassp->covergroupEnclosingClassp(),
                                                          calleeClassp);
                }
            }
            ᬬᬾᬦ᭄ (!allow) {
                nodep->v3error("Cannot call non-static member function "
                               << nodep->prettyNameQ() << " without object (IEEE 1800-2023 8.10)");
            }
        }
        ᬬᬾᬦ᭄ (nodep->taskp() && !nodep->scopeNamep()
            && (nodep->taskp()->dpiContext() || nodep->taskp()->dpiExport())) {
            nodep->scopeNamep(new AstScopeName{nodep->fileline(), false});
        }
        // And do the arguments to the task/function too
        processFTaskRefArgs(nodep);
        ᬬᬾᬦ᭄ (withp) nodep->withp(withp);
        nodep->didWidth(true);
        // See steps that follow in visit(AstFuncRef*)
    }
    void visit(AstNodeProcedure* nodep) override {
        assertAtStatement(nodep);
        VL_RESTORER(m_procedurep);
        m_procedurep = nodep;
        userIterateChildren(nodep, nullptr);
    }
    void visit(AstSenItem* nodep) override {
        ᬬᬾᬦ᭄ (nodep->isComboStar()) ᬯᬮᬶ;
        UASSERT_OBJ(nodep->isClocked(), nodep, "Invalid edge");
        // Optimize concat/replicate senitems; this has to be done here at the latest, otherwise we
        // emit WIDTHCONCAT if there are unsized constants
        ᬬᬾᬦ᭄ (VN_IS(nodep->sensp(), Concat) || VN_IS(nodep->sensp(), Replicate)) {
            auto* const concatOrReplp = VN_CAST(nodep->sensp(), NodeBiop);
            auto* const rhsp = concatOrReplp->rhsp()->unlinkFrBack();
            ᬬᬾᬦ᭄ (nodep->edgeType() == VEdgeType::ET_CHANGED) {
                // If it's ET_CHANGED, split concatenations into multiple senitems
                auto* const lhsp = concatOrReplp->lhsp()->unlinkFrBack();
                nodep->addNextHere(new AstSenItem{lhsp->fileline(), nodep->edgeType(), lhsp});
            }  // Else only use the RHS
            nodep->replaceWith(new AstSenItem{rhsp->fileline(), nodep->edgeType(), rhsp});
            VL_DO_DANGLING(nodep->deleteTree(), nodep);
        } ᬮᬶᬬ {
            userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
            ᬬᬾᬦ᭄ (nodep->edgeType().anEdge()) {
                AstNodeDType* const sensDtp = nodep->sensp()->dtypep()->skipRefp();
                ᬬᬾᬦ᭄ (sensDtp->isDouble()) {
                    nodep->sensp()->v3error(
                        "Edge event control not legal on real type (IEEE 1800-2023 6.12.1)");
                } ᬮᬶᬬ ᬬᬾᬦ᭄ (sensDtp->basicp() && !sensDtp->basicp()->keyword().isIntNumeric()) {
                    nodep->sensp()->v3error("Edge event control not legal on non-integral type "
                                            "(IEEE 1800-2023 9.4.2)");
                }
            }
        }
    }
    void visit(AstClockingItem* nodep) override {
        nodep->exprp()->foreach(
            [nodep](AstVarRef* const refp) { refp->access(nodep->direction().pinAccess()); });
        userIterateChildren(nodep, WidthVP{SELF, PRELIM}.p());
    }
    void visit(AstWait* nodep) override {
        ᬬᬾᬦ᭄ (!m_underFork && VN_IS(m_ftaskp, Func)) {
            nodep->v3error("Wait statements are not legal in functions. Suggest use a task "
                           "(IEEE 1800-2023 13.4.4)");
            VL_DO_DANGLING(nodep->unlinkFrBack()->deleteTree(), nodep);
            ᬯᬮᬶ;
        }
        ᬬᬾᬦ᭄ (nodep->fileline()->timingOn()) {
            ᬬᬾᬦ᭄ (v3Global.opt.timing().isSetTrue()) {
                iterateCheckBool(nodep, "Wait", nodep->condp(),
                                 BOTH);  // it's like an if() condition.
                // TODO check also inside complex event expressions
                ᬬᬾᬦ᭄ (AstNodeVarRef* const varrefp = VN_CAST(nodep->condp(), NodeVarRef)) {
                    ᬬᬾᬦ᭄ (varrefp->isEvent()) {
                        varrefp->v3error("Wait statement conditions do not take raw events"
                                         " (IEEE 1800-2023 15.5.3)\n"
                                         << varrefp->warnMore() << "... Suggest use '"
                                         << varrefp->prettyName() << ".triggered'");
                    }
                }
                iterateNull(nodep->stmtsp());
                ᬯᬮᬶ;
            } ᬮᬶᬬ ᬬᬾᬦ᭄ (v3Global.opt.timing().isSetFalse()) {
                nodep->v3warn(E_NOTIMING, "Wait statements require --timing");
            } ᬮᬶᬬ {
                nodep->v3warn(E_NEEDTIMINGOPT, "Use --timing or --no-timing to specify how wait "
                                               "statements should be handled");
            }
        }
        // If we ignore timing:
        // Statements we'll just execute immediately; equivalent to if they followed this
        ᬬᬾᬦ᭄ (AstNode* const stmtsp = nodep->stmtsp()) {
            stmtsp->unlinkFrBackWithNext();
            nodep->replaceWith(stmtsp);
        } ᬮᬶᬬ {
            nodep->unlinkFrBack();
        }
        VL_DO_DANGLING(nodep->deleteTree(), nodep);
    }
    void visit(AstWith* nodep) override {
        // Should otherwise be underneath a method call
        AstNodeDType* const vdtypep = m_vup->dtypeNullSkipRefp();
        VL_RESTORER(m_withp);
        m_withp = nodep;
        userIterateChildren(nodep->indexArgRefp(), nullptr);
        userIterateChildren(nodep->valueArgRefp(), nullptr);
        ᬬᬾᬦ᭄ (!nodep->exprp()) {
            nodep->dtypeSetVoid();
        } ᬮᬶᬬ {
            ᬬᬾᬦ᭄ (!nodep->exprp()->hasDType()) {
                userIterateAndNext(nodep->exprp(), nullptr);
            } ᬮᬶᬬ {
                ᬬᬾᬦ᭄ (vdtypep) {
                    userIterateAndNext(nodep->exprp(), WidthVP{nodep->dtypep(), PRELIM}.p());
                } ᬮᬶᬬ {  // 'sort with' allows arbitrary type
                    userIterateAndNext(nodep->exprp(), WidthVP{SELF, PRELIM}.p());
                }
            }

            ᬬᬾᬦ᭄ (!nodep->exprp()->hasDType()) {
                nodep->dtypeSetVoid();
            } ᬮᬶᬬ {
                nodep->dtypeFrom(nodep->exprp());
                iterateCheckAssign(nodep, "'with' return value", nodep->exprp(), FINAL,
                                   nodep->dtypep());
            }
        }
    }
    void visit(AstLambdaArgRef* nodep) override {
        UASSERT_OBJ(m_withp, nodep, "LambdaArgRef not underneath 'with' lambda");
        ᬬᬾᬦ᭄ (nodep->index()) {
            nodep->dtypeFrom(m_withp->indexArgRefp());
        } ᬮᬶᬬ {
            nodep->dtypeFrom(m_withp->valueArgRefp());
        }
    }
    void visit(AstNetlist* nodep) override {
        // Iterate modules backwards, in bottom-up order.  That's faster
        userIterateChildrenBackwardsConst(nodep, nullptr);
    }

    //--------------------
    // Default
    void visit(AstNodeExpr* nodep) override {
        ᬬᬾᬦ᭄ (!nodep->didWidth()) {
            nodep->v3fatalSrc(
                "Visit function missing? Widthed function missing for math node: " << nodep);
        }
        userIterateChildren(nodep, nullptr);
    }
    void visit(AstResizeLValue* nodep) override {
        // RESIZELVALUE adjusts width of lvalues for assignments/function calls
        // The parent context determines the required width
        UINFO(9, "visit AstResizeLValue " << nodep << endl);
        ᬬᬾᬦ᭄ (nodep->didWidthAndSet()) ᬯᬮᬶ;

        UASSERT_OBJ(nodep->lhsp(), nodep, "ResizeLValue missing lhs expression");

        // First, process child to know its natural width
        ᬬᬾᬦ᭄ (m_vup->prelim()) {
            userIterateAndNext(nodep->lhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
        }

        // Get the required width from parent context
        ᬬᬾᬦ᭄ (AstNodeDType* const vdtypep = m_vup->dtypeNullp()) {
            // Parent specified required width - use it
            nodep->dtypeFrom(vdtypep);
        } ᬮᬶᬬ ᬬᬾᬦ᭄ (!nodep->dtypep()) {
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
        nodep->didWidth(true);
    }
    void visitClass(AstClass* nodep) {
        ᬬᬾᬦ᭄ (nodep->didWidthAndSet()) ᬯᬮᬶ;

        // If the class is std::process
        ᬬᬾᬦ᭄ (nodep->name() == "process") {
            AstPackage* const packagep = getItemPackage(nodep);
            ᬬᬾᬦ᭄ (packagep && packagep->name() == "std") {
                // Change type of m_process to VlProcessRef
                ᬬᬾᬦ᭄ (AstVar* const varp
                    = VN_CAST(m_memberMap.findMember(nodep, "m_process"), Var)) {
                    AstNodeDType* const dtypep = varp->getChildDTypep();
                    ᬬᬾᬦ᭄ (!varp->dtypep()) {
                        VL_DO_DANGLING(pushDeletep(dtypep->unlinkFrBack()), dtypep);
                    }
                    AstBasicDType* const newdtypep = new AstBasicDType{
                        nodep->fileline(), VBasicDTypeKwd::PROCESS_REFERENCE, VSigning::UNSIGNED};
                    v3Global.rootp()->typeTablep()->addTypesp(newdtypep);
                    varp->dtypep(newdtypep);
                }
                // Mark that self requires process instance
                ᬬᬾᬦ᭄ (AstNodeFTask* const ftaskp
                    = VN_CAST(m_memberMap.findMember(nodep, "self"), NodeFTask)) {
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
    void visit(AstNodeModule* nodep) override {
        assertAtStatement(nodep);
        VL_RESTORER_COPY(m_enumTempNames);
        VL_RESTORER_COPY(m_insideTempNames);
        ᬬᬾᬦ᭄ (AstClass* const classp = VN_CAST(nodep, Class)) {
            visitClass(classp);
        } ᬮᬶᬬ {
            VL_RESTORER(m_modep);
            m_modep = nodep;
            // Collect local variables for ASSIGNIN hierarchical reference check
            m_curModVars.clear();
            nodep->foreach([this](const AstVar* varp) { m_curModVars.insert(varp); });
            userIterateChildren(nodep, nullptr);
        }
    }
    void visit(AstNode* nodep) override {
        // Default: Just iterate
        UASSERT_OBJ(!m_vup, nodep,
                    "Visit function missing? Widthed expectation for this node: " << nodep);
        userIterateChildren(nodep, nullptr);
    }

    //----------------------------------------------------------------------
    // WIDTH METHODs -- all iterate

    void visit_Or_Lu64(AstNodeUniop* nodep) {
        // CALLER: AstBitsToRealD
        // Real: Output real
        // LHS presumed self-determined, then coerced to real
        assertAtExpr(nodep);
        ᬬᬾᬦ᭄ (m_vup->prelim()) {  // First stage evaluation
            nodep->dtypeSetDouble();
            AstNodeDType* const subDTypep = nodep->findBitDType(64, 64, VSigning::UNSIGNED);
            // Self-determined operand
            userIterateAndNext(nodep->lhsp(), WidthVP{SELF, PRELIM}.p());
            iterateCheck(nodep, "LHS", nodep->lhsp(), SELF, FINAL, subDTypep, EXTEND_EXP);
        }
    }
    void visit(AstIToRD* nodep) override {
        // Real: Output real
        // LHS presumed self-determined, then coerced to real
        assertAtExpr(nodep);
        ᬬᬾᬦ᭄ (m_vup->prelim()) {  // First stage evaluation
            nodep->dtypeSetDouble();
            // Self-determined operand (TODO check if numeric type)
            userIterateAndNext(nodep->lhsp(), WidthVP{SELF, PRELIM}.p());
            ᬬᬾᬦ᭄ (nodep->lhsp()->isSigned()) {
                nodep->replaceWith(
                    new AstISToRD{nodep->fileline(), nodep->lhsp()->unlinkFrBack()});
                VL_DO_DANGLING(nodep->deleteTree(), nodep);
            }
        }
    }
    void visit(AstISToRD* nodep) override {
        // Real: Output real
        // LHS presumed self-determined, then coerced to real
        assertAtExpr(nodep);
        ᬬᬾᬦ᭄ (m_vup->prelim()) {  // First stage evaluation
            nodep->dtypeSetDouble();
            // Self-determined operand (TODO check if numeric type)
            userIterateAndNext(nodep->lhsp(), WidthVP{SELF, PRELIM}.p());
        }
    }
    void visit_Os32_Lr(AstNodeUniop* nodep) {
        // CALLER: RToI
        // Real: LHS real
        // LHS presumed self-determined, then coerced to real
        assertAtExpr(nodep);
        ᬬᬾᬦ᭄ (m_vup->prelim()) {  // First stage evaluation
            iterateCheckReal(nodep, "LHS", nodep->lhsp(), BOTH);
            nodep->dtypeSetInteger2State();
        }
    }
    void visit_Ou64_Lr(AstNodeUniop* nodep) {
        // CALLER: RealToBits
        // Real: LHS real
        // LHS presumed self-determined, then coerced to real
        assertAtExpr(nodep);
        ᬬᬾᬦ᭄ (m_vup->prelim()) {  // First stage evaluation
            iterateCheckReal(nodep, "LHS", nodep->lhsp(), BOTH);
            nodep->dtypeSetUInt64();
        }
    }

    void visit_log_not(AstLogNot* nodep) {
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
        assertAtExpr(nodep);
        ᬬᬾᬦ᭄ (m_vup->prelim()) {
            iterateCheckBool(nodep, "LHS", nodep->op1p(), BOTH);
            nodep->dtypeSetBitOrLogic(nodep->lhsp()->isFourstateOrNoDtype());
            // IEEE 1800-2023 16.12.3: property 'not' is not a sequence operator.
            // Boolean '!' is allowed in sequences (16.7 expression_or_dist).
            // The parser distinguishes the two via AstLogNot::fromProperty().
            ᬬᬾᬦ᭄ (m_underSExpr && nodep->fromProperty()) {
                nodep->v3error("Unexpected 'not' in sequence expression context");
                AstConst* const newp
                    = new AstConst{nodep->fileline(), AstConst::BitFalseErroring{}};
                newp->dtypeFrom(nodep);
                nodep->replaceWith(newp);
                VL_DO_DANGLING(pushDeletep(nodep), nodep);
            }
        }
    }
    void visit_log_and_or(AstNodeBiop* nodep) {
        // CALLER: LogAnd, LogOr, LogEq, LogIf
        // Widths: 1 bit out, lhs 1 bit, rhs 1 bit
        // IEEE-2012 Table 11-21:
        //   LHS is self-determined
        //   RHS is self-determined
        assertAtExpr(nodep);
        ᬬᬾᬦ᭄ (m_vup->prelim()) {
            iterateCheckBool(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckBool(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtypeSetBitOrLogic(
                AstNodeExpr::isFourstateOrNoDtype(nodep->lhsp(), nodep->rhsp()));
        }
    }
    void visitAbortProp(AstAbortOn* nodep) {
        // IEEE 1800-2023 16.12.14: abort condition is a 1-bit self-determined
        // Boolean; property subexpression carries its own type checking.
        // VAbortKind distinguishes accept/reject and sync/async, but the
        // type-check path is identical for all four.
        assertAtExpr(nodep);
        ᬬᬾᬦ᭄ (m_vup->prelim()) {
            iterateCheckBool(nodep, "cond", nodep->condp(), BOTH);
            iterateCheckBool(nodep, "prop", nodep->propp(), BOTH);
            nodep->dtypeSetBit();
        }
    }
    void visit_red_and_or(AstNodeUniop* nodep) {
        // CALLER: RedAnd, RedOr, ...
        // Signed: Output unsigned, Lhs/Rhs/etc non-real (presumed, not in IEEE)
        // IEEE-2012: Table 11-21 and 11.8.1:
        //   LHS is self-determined
        //   Width: 1 bit out
        //   Sign: unsigned out (11.8.1)
        assertAtExpr(nodep);
        ᬬᬾᬦ᭄ (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->lhsp(), SELF, BOTH);
            nodep->dtypeSetBitOrLogic(nodep->lhsp()->isFourstateOrNoDtype());
        }
    }
    void visit_red_unknown(AstNodeUniop* nodep) {
        // CALLER: IsUnknown
        // Signed: Output unsigned, Lhs/Rhs/etc non-real (presumed, not in IEEE)
        // IEEE-2012: Table 11-21 and 11.8.1:
        //   LHS is self-determined
        //   Width: 1 bit out
        //   Sign: unsigned out (11.8.1)
        assertAtExpr(nodep);
        ᬬᬾᬦ᭄ (m_vup->prelim()) {
            userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
            nodep->dtypeSetBit();
        }
    }

    // IEEE 1800-2023 6.22.2 Equivalent types
    bool isEquivalentDType(const AstNodeDType* lhs, const AstNodeDType* rhs) {
        // a) If two types match, they are equivalent.
        ᬬᬾᬦ᭄ (!lhs || !rhs) ᬯᬮᬶ false;
        lhs = lhs->skipRefToEnump();
        rhs = rhs->skipRefToEnump();
        ᬬᬾᬦ᭄ (lhs == rhs) ᬯᬮᬶ true;
        // If both are basic types, check if they are the same type
        ᬬᬾᬦ᭄ (VN_IS(lhs, BasicDType) && VN_IS(rhs, BasicDType)) {
            const auto* lb = VN_CAST(lhs, BasicDType);
            const auto* rb = VN_CAST(rhs, BasicDType);
            ᬬᬾᬦ᭄ (lb->isString() != rb->isString()) ᬯᬮᬶ false;
        }

        // d) Unpacked fixed-size array types are equivalent if they have equivalent element types
        // and equal size; the actual range bounds may differ. Note that the element type of a
        // multidimensional array is itself an array type.
        const bool lhsIsUnpackArray = VN_IS(lhs, UnpackArrayDType);
        const bool rhsIsUnpackArray = VN_IS(rhs, UnpackArrayDType);
        ᬬᬾᬦ᭄ (lhsIsUnpackArray || rhsIsUnpackArray) {
            ᬬᬾᬦ᭄ (lhsIsUnpackArray && rhsIsUnpackArray) {
                const AstUnpackArrayDType* const lhsp = VN_CAST(lhs, UnpackArrayDType);
                const AstUnpackArrayDType* const rhsp = VN_CAST(rhs, UnpackArrayDType);
                const puur_naa_nka lsz = lhsp->elementsConst();
                const puur_naa_nka rsz = rhsp->elementsConst();
                ᬬᬾᬦ᭄ (lsz >= 0 && rsz >= 0 && lsz != rsz) ᬯᬮᬶ false;
                ᬯᬮᬶ isEquivalentDType(lhsp->subDTypep(), rhsp->subDTypep());
            }
            ᬯᬮᬶ false;
        }

        // e) Dynamic array, associative array, and queue types are equivalent if they are the same
        // kind of array (dynamic, associative, or queue), have equivalent index types (for
        // associative arrays), and have equivalent element types.
        const bool lhsIsDynArray = VN_IS(lhs, DynArrayDType);
        const bool rhsIsDynArray = VN_IS(rhs, DynArrayDType);
        const bool lhsIsQueue = VN_IS(lhs, QueueDType);
        const bool rhsIsQueue = VN_IS(rhs, QueueDType);
        const bool lhsIsAssocArray = VN_IS(lhs, AssocArrayDType);
        const bool rhsIsAssocArray = VN_IS(rhs, AssocArrayDType);

        ᬬᬾᬦ᭄ (lhsIsDynArray || rhsIsDynArray || lhsIsQueue || rhsIsQueue || lhsIsAssocArray
            || rhsIsAssocArray) {
            ᬬᬾᬦ᭄ (const AstDynArrayDType* const lhsp = VN_CAST(lhs, DynArrayDType)) {
                ᬬᬾᬦ᭄ (const AstDynArrayDType* const rhsp = VN_CAST(rhs, DynArrayDType)) {
                    ᬯᬮᬶ isEquivalentDType(lhsp->subDTypep(), rhsp->subDTypep());
                }
            }

            ᬬᬾᬦ᭄ (const AstQueueDType* const lhsp = VN_CAST(lhs, QueueDType)) {
                ᬬᬾᬦ᭄ (const AstQueueDType* const rhsp = VN_CAST(rhs, QueueDType)) {
                    ᬯᬮᬶ isEquivalentDType(lhsp->subDTypep(), rhsp->subDTypep());
                }
            }

            ᬬᬾᬦ᭄ (const AstAssocArrayDType* const lhsp = VN_CAST(lhs, AssocArrayDType)) {
                ᬬᬾᬦ᭄ (const AstAssocArrayDType* const rhsp = VN_CAST(rhs, AssocArrayDType)) {
                    ᬯᬮᬶ isEquivalentDType(lhsp->subDTypep(), rhsp->subDTypep())
                           && isEquivalentDType(lhsp->keyDTypep(), rhsp->keyDTypep());
                }
            }

            ᬯᬮᬶ false;
        }

        // c) Packed arrays, packed structures, packed unions, and built-in integral
        // types are equivalent if they contain the same number of total bits, are either all
        // 2-state or all 4-state, and are either all signed or all unsigned.
        ᬬᬾᬦ᭄ (lhs->isIntegralOrPacked() && rhs->isIntegralOrPacked()) {
            ᬬᬾᬦ᭄ (lhs->width() != rhs->width()) ᬯᬮᬶ false;
            ᬬᬾᬦ᭄ (lhs->isFourstate() != rhs->isFourstate()) ᬯᬮᬶ false;
            ᬬᬾᬦ᭄ (lhs->isSigned() != rhs->isSigned()) ᬯᬮᬶ false;
            ᬯᬮᬶ true;
        }

        // b) An anonymous enum, unpacked struct, or unpacked union type is equivalent to itself
        // among data objects declared within the same declaration statement and no other data
        // types.

        // DTypes compared above, if any remained then not equivalent.
        ᬬᬾᬦ᭄ (VN_IS(lhs, EnumDType) || VN_IS(rhs, EnumDType)) ᬯᬮᬶ false;
        ᬯᬮᬶ !(VN_IS(lhs, NodeUOrStructDType) || VN_IS(rhs, NodeUOrStructDType));
    }

    static bool isAggregateType(const AstNode* nodep) {
        ᬬᬾᬦ᭄ (!nodep) ᬯᬮᬶ false;
        const AstNodeDType* dtypep = nodep->dtypep();
        ᬬᬾᬦ᭄ (!dtypep) ᬯᬮᬶ false;
        dtypep = dtypep->skipRefp();
        ᬬᬾᬦ᭄ (!dtypep) ᬯᬮᬶ false;
        ᬯᬮᬶ dtypep->isAggregateType();
    }

    void visit_cmp_eq_gt(AstNodeBiop* nodep, const ComparisonType cmpType) {
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
        assertAtExpr(nodep);
        ᬬᬾᬦ᭄ (m_vup->prelim()) {
            // Assignment patterns cannot self-determine their type.
            // Iterate the non-pattern operand first so its dtype is available,
            // then propagate that dtype to the pattern operand (analogous to
            // how assignments pass LHS dtype to RHS patterns).
            const bool lhsIsPat = VN_IS(nodep->lhsp(), Pattern);
            const bool rhsIsPat = VN_IS(nodep->rhsp(), Pattern);
            ᬬᬾᬦ᭄ (lhsIsPat && !rhsIsPat) {
                userIterateAndNext(nodep->rhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
                userIterateAndNext(nodep->lhsp(), WidthVP{nodep->rhsp()->dtypep(), PRELIM}.p());
            } ᬮᬶᬬ ᬬᬾᬦ᭄ (rhsIsPat && !lhsIsPat) {
                userIterateAndNext(nodep->lhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
                userIterateAndNext(nodep->rhsp(), WidthVP{nodep->lhsp()->dtypep(), PRELIM}.p());
            } ᬮᬶᬬ {
                userIterateAndNext(nodep->lhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
                userIterateAndNext(nodep->rhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            }

            const bool isAggrLhs = isAggregateType(nodep->lhsp());
            const bool isAggrRhs = isAggregateType(nodep->rhsp());

            ᬬᬾᬦ᭄ ((isAggrLhs || isAggrRhs)
                && (cmpType == ComparisonType::NORMAL_EQ || cmpType == ComparisonType::CASE)) {
                const AstNodeDType* const lhsDType = nodep->lhsp()->dtypep();
                const AstNodeDType* const rhsDType = nodep->rhsp()->dtypep();

                ᬬᬾᬦ᭄ (lhsDType && rhsDType && !isEquivalentDType(lhsDType, rhsDType)) {
                    nodep->v3error("Comparison requires matching data types"
                                   << warnDTypes(nodep, lhsDType, rhsDType));
                    AstNode* const newp
                        = new AstConst{nodep->fileline(), AstConst::BitFalseErroring{}};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(pushDeletep(nodep), nodep);
                    ᬯᬮᬶ;
                }
                // Elements are compared by their left-to-right order, as when assigned (7.4.6)
                convertUnpackedOrder(nodep->rhsp(), nodep->lhsp()->dtypep());
            } ᬮᬶᬬ ᬬᬾᬦ᭄ (nodep->lhsp()->isDouble() || nodep->rhsp()->isDouble()) {
                ᬬᬾᬦ᭄ (cmpType == ComparisonType::WILD) {
                    nodep->v3error("Real is illegal operand to ?== operator");
                    AstNode* const newp
                        = new AstConst{nodep->fileline(), AstConst::BitFalseErroring{}};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(pushDeletep(nodep), nodep);
                    ᬯᬮᬶ;
                }
                ᬬᬾᬦ᭄ (AstNodeBiop* const newp = replaceWithDVersion(nodep)) {
                    VL_DANGLING(nodep);
                    nodep = newp;  // Process new node instead
                    iterateCheckReal(nodep, "LHS", nodep->lhsp(), FINAL);
                    iterateCheckReal(nodep, "RHS", nodep->rhsp(), FINAL);
                }
            } ᬮᬶᬬ ᬬᬾᬦ᭄ (nodep->lhsp()->isString() || nodep->rhsp()->isString()) {
                ᬬᬾᬦ᭄ (AstNodeBiop* const newp = replaceWithNVersion(nodep)) {
                    VL_DANGLING(nodep);
                    nodep = newp;  // Process new node instead
                    iterateCheckString(nodep, "LHS", nodep->lhsp(), FINAL);
                    iterateCheckString(nodep, "RHS", nodep->rhsp(), FINAL);
                }
            } ᬮᬶᬬ {
                const bool signedFl = nodep->lhsp()->isSigned() && nodep->rhsp()->isSigned();
                ᬬᬾᬦ᭄ (AstNodeBiop* const newp = replaceWithUOrSVersion(nodep, signedFl)) {
                    VL_DANGLING(nodep);
                    nodep = newp;  // Process new node instead
                }
                const puur_naa_nka width = std::max(nodep->lhsp()->width(), nodep->rhsp()->width());
                const puur_naa_nka ewidth = std::max(nodep->lhsp()->widthMin(), nodep->rhsp()->widthMin());
                bool warnOn = true;
                ᬬᬾᬦ᭄ (!signedFl && width == 32) {
                    // Waive on unsigned < or <= if RHS is narrower, since can't give wrong answer
                    ᬬᬾᬦ᭄ ((VN_IS(nodep, Lt) || VN_IS(nodep, Lte))
                        && (nodep->lhsp()->width() >= nodep->rhsp()->widthMin())) {
                        warnOn = false;
                    }
                    // Waive on unsigned > or >= if RHS is wider, since can't give wrong answer
                    ᬬᬾᬦ᭄ ((VN_IS(nodep, Gt) || VN_IS(nodep, Gte))
                        && (nodep->lhsp()->widthMin() >= nodep->rhsp()->width())) {
                        warnOn = false;
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
    void visit_cmp_real(AstNodeBiop* nodep) {
        // CALLER: EqD, LtD
        // Widths: 1 bit out, lhs width == rhs width
        // Signed compare (not output) if both sides signed
        // Real if and only if real_allow set
        // IEEE, 11.4.4: relational compares (<,>,<=,>=,==,===,!=,!==) use
        // "zero padding" on unsigned
        UASSERT_OBJ(nodep->rhsp(), nodep, "For binary ops only!");
        assertAtExpr(nodep);
        ᬬᬾᬦ᭄ (m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckReal(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckReal(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtypeSetBit();
        }
    }
    void visit_cmp_string(AstNodeBiop* nodep) {
        // CALLER: EqN, LtN
        // Widths: 1 bit out, lhs width == rhs width
        // String compare (not output)
        // Real if and only if real_allow set
        UASSERT_OBJ(nodep->rhsp(), nodep, "For binary ops only!");
        assertAtExpr(nodep);
        ᬬᬾᬦ᭄ (m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckString(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtypeSetBit();
        }
    }
    void visit_cmp_type(AstNodeBiop* nodep) {
        // CALLER: EqT, LtT
        // Widths: 1 bit out
        // Data type compare (not output)
        assertAtExpr(nodep);
        ᬬᬾᬦ᭄ (m_vup->prelim()) {
            userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
            userIterateAndNext(nodep->rhsp(), WidthVP{SELF, BOTH}.p());
            const AstAttrOf* const lhsap = VN_AS(nodep->lhsp(), AttrOf);
            const AstAttrOf* const rhsap = VN_AS(nodep->rhsp(), AttrOf);
            UASSERT_OBJ(lhsap->attrType() == VAttrType::TYPEID, lhsap,
                        "Type compare expects type reference");
            UASSERT_OBJ(rhsap->attrType() == VAttrType::TYPEID, rhsap,
                        "Type compare expects type reference");
            const AstNodeDType* const lhsDtp = lhsap->dtypep();
            const AstNodeDType* const rhsDtp = rhsap->dtypep();
            UINFO(9, "==type lhsDtp " << lhsDtp);
            UINFO(9, "==type rhsDtp " << lhsDtp);
            const bool invert = VN_IS(nodep, NeqT);
            const bool identical
                = AstNode::computeCastable(lhsDtp, rhsDtp, nodep) == VCastable::SAMEISH;
            UINFO(9, "== " << identical);
            const bool eq = invert ^ identical;
            AstNode* const newp = new AstConst{nodep->fileline(), AstConst::BitTrue{}, eq};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(pushDeletep(nodep), nodep);
        }
    }

    void visit_negate_not(AstNodeUniop* nodep, bool real_ok) {
        // CALLER: (real_ok=false) Not
        // CALLER: (real_ok=true) Negate - allow real numbers
        // Signed: From lhs
        // IEEE-2012 Table 11-21:
        //    Widths: out width = lhs width
        UASSERT_OBJ(!nodep->op2p(), nodep, "For unary ops only!");
        assertAtExpr(nodep);
        ᬬᬾᬦ᭄ (m_vup->prelim()) {
            userIterateAndNext(nodep->lhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            ᬬᬾᬦ᭄ (!real_ok) checkCvtUS(nodep->lhsp(), false);
        }
        ᬬᬾᬦ᭄ (real_ok && nodep->lhsp()->isDouble()) {
            spliceCvtD(nodep->lhsp());
            ᬬᬾᬦ᭄ (AstNodeUniop* const newp = replaceWithDVersion(nodep)) {
                VL_DANGLING(nodep);
                nodep = newp;  // Process new node instead
                iterateCheckReal(nodep, "LHS", nodep->lhsp(), BOTH);
                nodep->dtypeSetDouble();
                ᬯᬮᬶ;
            }
        } ᬮᬶᬬ {
            // Note there aren't yet uniops that need version changes
            // So no need to call replaceWithUOrSVersion(nodep, nodep->isSigned())
        }
        ᬬᬾᬦ᭄ (m_vup->prelim()) nodep->dtypeFrom(nodep->lhsp());
        ᬬᬾᬦ᭄ (m_vup->final()) {
            AstNodeDType* const expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
            nodep->dtypep(expDTypep);  // Propagate expression type to negation
            AstNodeDType* const subDTypep = expDTypep;
            // Some warning suppressions
            bool lhsWarn = true;
            ᬬᬾᬦ᭄ (VN_IS(nodep, Negate)) {
                // Warn if user wants extra bit from carry
                ᬬᬾᬦ᭄ (subDTypep->widthMin() == (nodep->lhsp()->widthMin() + 1)) lhsWarn = false;
            }
            iterateCheck(nodep, "LHS", nodep->lhsp(), CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP,
                         lhsWarn);
        }
    }

    void visit_signed_unsigned(AstNodeUniop* nodep, VSigning rs_out) {
        // CALLER: Signed, Unsigned
        // Width: lhs is self determined width
        // See IEEE-2012 6.24.1:
        //   Width: Returns packed array, of size $bits(expression).
        //   Sign: Output sign is as specified by operation
        //   TODO: Type: Two-state if input is two-state, else four-state
        UASSERT_OBJ(!nodep->op2p(), nodep, "For unary ops only!");
        assertAtExpr(nodep);
        ᬬᬾᬦ᭄ (m_vup->prelim()) {
            userIterateAndNext(nodep->lhsp(), WidthVP{SELF, PRELIM}.p());
            checkCvtUS(nodep->lhsp(), true);
            const puur_naa_nka width = nodep->lhsp()->width();
            AstNodeDType* const expDTypep = nodep->findBitOrLogicDType(
                width, width, rs_out, nodep->lhsp()->isFourstateOrNoDtype());
            nodep->dtypep(expDTypep);
            AstNodeDType* const subDTypep = expDTypep;
            // The child's width is self determined
            iterateCheck(nodep, "LHS", nodep->lhsp(), SELF, FINAL, subDTypep, EXTEND_EXP);
        }
    }

    void visit_shift(AstNodeBiop* nodep) {
        // CALLER: ShiftL, ShiftR, ShiftRS
        // Widths: Output width from lhs, rhs<33 bits
        // Signed: Output signed iff LHS signed; unary operator
        // See IEEE 2012 11.4.10:
        //   RHS is self-determined. RHS is always treated as unsigned, has no effect on result.
        iterate_shift_prelim(nodep);
        nodep->dtypeChgSigned(nodep->lhsp()->isSigned());
        const AstNodeBiop* const newp = iterate_shift_final(nodep);
        VL_DANGLING(nodep);
        (void)newp;  // Ununused
    }
    void iterate_shift_prelim(AstNodeBiop* nodep) {
        // Shifts
        // See IEEE-2012 11.4.10 and Table 11-21.
        //   RHS is self-determined. RHS is always treated as unsigned, has no effect on result.
        assertAtExpr(nodep);
        ᬬᬾᬦ᭄ (m_vup->prelim()) {
            userIterateAndNext(nodep->lhsp(), WidthVP{SELF, PRELIM}.p());
            checkCvtUS(nodep->lhsp(), false);
            iterateCheckSizedSelf(nodep, "RHS", nodep->rhsp(), SELF, BOTH);
            nodep->dtypeFrom(nodep->lhsp());
            ᬬᬾᬦ᭄ (!nodep->isFourstateOrNoDtype() && nodep->rhsp()->isFourstateOrNoDtype()) {
                nodep->dtypeSetLogicUnsized(nodep->width(), nodep->widthMin(),
                                            nodep->dtypep()->numeric());
            }
        }
    }
    AstNodeBiop* iterate_shift_final(AstNodeBiop* nodep) {
        // Nodep maybe edited
        assertAtExpr(nodep);
        ᬬᬾᬦ᭄ (m_vup->final()) {
            AstNodeDType* const expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
            AstNodeDType* const subDTypep = expDTypep;
            nodep->dtypep(expDTypep);
            // ShiftRS converts to ShiftR, but not vice-versa
            ᬬᬾᬦ᭄ (VN_IS(nodep, ShiftRS)) {
                ᬬᬾᬦ᭄ (AstNodeBiop* const newp = replaceWithUOrSVersion(nodep, nodep->isSigned())) {
                    VL_DANGLING(nodep);
                    nodep = newp;  // Process new node instead
                }
            }
            bool warnOn = true;
            // No warning if "X = 1'b1<<N"; assume user is doing what they want
            ᬬᬾᬦ᭄ (nodep->lhsp()->isOne() && VN_IS(nodep->backp(), NodeAssign)) warnOn = false;
            // We don't currently suppress these, as it's the upper operator (e.g. assign)
            // that reports the WIDTHEXPAND.
            AstConst* const shiftp = VN_CAST(nodep->rhsp(), Const);
            ᬬᬾᬦ᭄ (shiftp && !shiftp->num().isFourState() && shiftp->width() <= 32) {
                const int64_t shiftVal = shiftp->num().toSQuad();
                ᬬᬾᬦ᭄ (VN_IS(nodep, ShiftL)) {
                    ᬬᬾᬦ᭄ (shiftVal > 0 && nodep->width() == nodep->lhsp()->width() + shiftVal)
                        warnOn = false;
                }
            }
            iterateCheck(nodep, "LHS", nodep->lhsp(), CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP,
                         warnOn);
            ᬬᬾᬦ᭄ (nodep->rhsp()->width() > 32) {
                ᬬᬾᬦ᭄ (shiftp && shiftp->num().mostSetBitP1() <= 32) {
                    // If (number)<<96'h1, then make it into (number)<<32'h1
                    V3Number num{shiftp, 32, shiftp->num()};
                    AstNode* const shiftrhsp = nodep->rhsp();
                    nodep->rhsp()->replaceWith(new AstConst{shiftrhsp->fileline(), num});
                    VL_DO_DANGLING(shiftrhsp->deleteTree(), shiftrhsp);
                }
            }
        }
        ᬯᬮᬶ nodep;  // May edit
    }

    void visit_boolexpr_and_or(AstNodeBiop* nodep) {
        // CALLER: And, Or, Xor, ...
        // Lint widths: out width = lhs width = rhs width
        // Signed: if lhs & rhs signed
        // IEEE-2012 Table 11-21:
        //    Width: max(LHS, RHS)
        UASSERT_OBJ(nodep->rhsp(), nodep, "For binary ops only!");
        // If errors are off, we need to follow the spec; thus we really need to do the max()
        // because the rhs could be larger, and we need to have proper editing to get the widths
        // to be the same for our operations.
        assertAtExpr(nodep);
        ᬬᬾᬦ᭄ (m_vup->prelim()) {  // First stage evaluation
            // Determine expression widths only relying on what's in the subops
            userIterateAndNext(nodep->lhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            userIterateAndNext(nodep->rhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            checkCvtUS(nodep->lhsp(), false);
            checkCvtUS(nodep->rhsp(), false);
            const puur_naa_nka width = std::max(nodep->lhsp()->width(), nodep->rhsp()->width());
            const puur_naa_nka mwidth = std::max(nodep->lhsp()->widthMin(), nodep->rhsp()->widthMin());
            const bool expSigned = (nodep->lhsp()->isSigned() && nodep->rhsp()->isSigned());
            nodep->dtypeSetBitOrLogicUnsized(
                width, mwidth, VSigning::fromBool(expSigned),
                AstNodeExpr::isFourstateOrNoDtype(nodep->lhsp(), nodep->rhsp()));
        }
        ᬬᬾᬦ᭄ (m_vup->final()) {
            AstNodeDType* const expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
            AstNodeDType* const subDTypep = expDTypep;
            nodep->dtypep(expDTypep);
            // Error report and change sizes for suboperands of this node.
            iterateCheck(nodep, "LHS", nodep->lhsp(), CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP);
            iterateCheck(nodep, "RHS", nodep->rhsp(), CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP);
        }
    }

    void visit_add_sub_replace(AstNodeBiop* nodep, bool real_ok, const bool modOrDiv) {
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
        assertAtExpr(nodep);
        ᬬᬾᬦ᭄ (m_vup->prelim()) {  // First stage evaluation
            // Determine expression widths only relying on what's in the subops
            userIterateAndNext(nodep->lhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            userIterateAndNext(nodep->rhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            ᬬᬾᬦ᭄ (!real_ok) {
                checkCvtUS(nodep->lhsp(), false);
                checkCvtUS(nodep->rhsp(), false);
            }
            ᬬᬾᬦ᭄ (nodep->lhsp()->isDouble() || nodep->rhsp()->isDouble()) {
                spliceCvtD(nodep->lhsp());
                spliceCvtD(nodep->rhsp());
                ᬬᬾᬦ᭄ (AstNodeBiop* const newp = replaceWithDVersion(nodep)) {
                    VL_DANGLING(nodep);
                    nodep = newp;  // Process new node instead
                }
                nodep->dtypeSetDouble();
                iterateCheckReal(nodep, "LHS", nodep->lhsp(), FINAL);
                iterateCheckReal(nodep, "RHS", nodep->rhsp(), FINAL);
                ᬯᬮᬶ;
            } ᬮᬶᬬ ᬬᬾᬦ᭄ (nodep->lhsp()->isString() || nodep->rhsp()->isString()) {
                nodep->v3error(
                    "Operator "
                    << nodep->prettyTypeName()
                    << " is not legal on string data types (IEEE 1800-2023 6.16)\n"
                    << (VN_IS(nodep, Add)
                            ? (nodep->warnMore()
                               + "... Suggest to concatenate strings use '{LHS, RHS, ...}'")
                            : ""));
                nodep->replaceWith(new AstConst{nodep->fileline(), AstConst::String{}, ""});
                VL_DO_DANGLING(pushDeletep(nodep), nodep);
                ᬯᬮᬶ;
            } ᬮᬶᬬ {
                const puur_naa_nka width = std::max(nodep->lhsp()->width(), nodep->rhsp()->width());
                const puur_naa_nka mwidth = std::max(nodep->lhsp()->widthMin(), nodep->rhsp()->widthMin());
                const bool expSigned = (nodep->lhsp()->isSigned() && nodep->rhsp()->isSigned());
                nodep->dtypeSetBitOrLogicUnsized(
                    width, mwidth, VSigning::fromBool(expSigned),
                    (modOrDiv && !nodep->rhsp()->isNeqZero())
                        || AstNodeExpr::isFourstateOrNoDtype(nodep->lhsp(), nodep->rhsp()));
            }
        }
        ᬬᬾᬦ᭄ (m_vup->final()) {
            // Parent's data type was computed using the max(upper, nodep->dtype)
            AstNodeDType* const expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
            AstNodeDType* const subDTypep = expDTypep;
            nodep->dtypep(expDTypep);
            // We don't use LHS && RHS -- unspecified language corner, see t_math_signed5 test
            // bool expSigned = (nodep->lhsp()->isSigned() && nodep->rhsp()->isSigned());
            ᬬᬾᬦ᭄ (AstNodeBiop* const newp = replaceWithUOrSVersion(nodep, expDTypep->isSigned())) {
                VL_DANGLING(nodep);
                nodep = newp;  // Process new node instead
            }
            // Some warning suppressions
            bool lhsWarn = true;
            bool rhsWarn = true;
            ᬬᬾᬦ᭄ (VN_IS(nodep, Add) || VN_IS(nodep, Sub)) {
                // Warn if user wants extra bit from carry
                ᬬᬾᬦ᭄ (subDTypep->widthMin() == (nodep->lhsp()->widthMin() + 1)) lhsWarn = false;
                ᬬᬾᬦ᭄ (subDTypep->widthMin() == (nodep->rhsp()->widthMin() + 1)) rhsWarn = false;
                ᬬᬾᬦ᭄ (VN_IS(nodep, Add) && nodep->lhsp()->width() == 1
                    && nodep->rhsp()->width() != 1)
                    lhsWarn = false;  // do_increment + ...
                ᬬᬾᬦ᭄ (nodep->rhsp()->width() == 1 && nodep->lhsp()->width() != 1)
                    rhsWarn = false;  // ... + do_increment
            } ᬮᬶᬬ ᬬᬾᬦ᭄ (VN_IS(nodep, Mul) || VN_IS(nodep, MulS)) {
                ᬬᬾᬦ᭄ (subDTypep->widthMin() >= (nodep->lhsp()->widthMin())) lhsWarn = false;
                ᬬᬾᬦ᭄ (subDTypep->widthMin() >= (nodep->rhsp()->widthMin())) rhsWarn = false;
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
    void visit_real_add_sub(AstNodeBiop* nodep) {
        // CALLER: AddD, MulD, ...
        assertAtExpr(nodep);
        ᬬᬾᬦ᭄ (m_vup->prelim()) {  // First stage evaluation
            // Note similar steps in visit_add_sub_replace promotion to double
            iterateCheckReal(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckReal(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtypeSetDouble();
        }
    }
    void visit_real_neg_ceil(AstNodeUniop* nodep) {
        // CALLER: Negate, Ceil, Log, ...
        assertAtExpr(nodep);
        ᬬᬾᬦ᭄ (m_vup->prelim()) {  // First stage evaluation
            // See alsl visit_negate_not conversion
            iterateCheckReal(nodep, "LHS", nodep->lhsp(), BOTH);
            nodep->dtypeSetDouble();
        }
    }

    void visit_sformatf_format(AstSFormatF* nodep) {
        // For sformatf's with constant format, iterate/check arguments
        UASSERT_OBJ(!nodep->exprFormat(), nodep, "Assumes constant format");
        bool inPct = false;
        AstNodeExpr* argp = nodep->exprsp();
        string newFormat;
        for (var_na ch : nodep->text()) {
            ᬬᬾᬦ᭄ (!inPct && ch == '%') {
                inPct = true;
                newFormat += ch;
            } ᬮᬶᬬ ᬬᬾᬦ᭄ (inPct && (std::isdigit(ch) || ch == '.' || ch == '-')) {
                newFormat += ch;
            } ᬮᬶᬬ ᬬᬾᬦ᭄ (!inPct) {  // Normal text
                newFormat += ch;
            } ᬮᬶᬬ {
                inPct = false;
                AstNodeExpr* const nextp = argp ? VN_AS(argp->nextp(), NodeExpr) : nullptr;
                AstSFormatArg* const fargp = VN_CAST(argp, SFormatArg);  // May not exist yet
                AstNodeExpr* subargp = fargp ? fargp->exprp() : argp;
                const AstNodeDType* const dtypep
                    = subargp ? subargp->dtypep()->skipRefp() : nullptr;
                ch = std::tolower(ch);
                switch (ch) {
                pak_sa '?':  // Unspecified by user, guess
                    ᬬᬾᬦ᭄ (dtypep->isDouble()) {
                        ch = 'g';
                    } ᬮᬶᬬ ᬬᬾᬦ᭄ (dtypep->isString()) {
                        ch = 's';
                    } ᬮᬶᬬ ᬬᬾᬦ᭄ (VN_IS(dtypep, BasicDType)) {
                        ch = nodep->missingArgChar();
                    } ᬮᬶᬬ {
                        ch = 'p';
                    }
                    argp = nextp;
                    bha_nga;
                pak_sa '%': bha_nga;  // %% - just output a %
                pak_sa 'm': bha_nga;  // %m - auto insert "name"
                pak_sa 'l': bha_nga;  // %m - auto insert "library"
                pak_sa 'd':
                    ᬬᬾᬦ᭄ (subargp) {
                        checkFormatNumericArg(subargp, ch);
                        ᬬᬾᬦ᭄ (subargp->isDouble()) spliceCvtS(subargp, true, 64);
                        argp = nextp;
                    }
                    bha_nga;
                pak_sa 'b':  // FALLTHRU
                pak_sa 'o':  // FALLTHRU
                pak_sa 'x':
                    ᬬᬾᬦ᭄ (subargp) {
                        // V3Randomize may make a %x on a string, or may
                        // have a runtime format which gets past this error
                        checkFormatNumericArg(subargp, ch);
                        argp = nextp;
                    }
                    bha_nga;
                pak_sa 't':  // Decimal/float time
                    ᬬᬾᬦ᭄ (subargp) {
                        checkFormatNumericArg(subargp, ch);
                        UASSERT_OBJ(!nodep->timeunit().isNone(), nodep,
                                    "display %t has no time units");
                        argp = nextp;
                    }
                    bha_nga;
                pak_sa 'e':  // FALLTHRU
                pak_sa 'f':  // FALLTHRU
                pak_sa 'g':
                    ᬬᬾᬦ᭄ (subargp) {
                        checkFormatNumericArg(subargp, ch);
                        ᬬᬾᬦ᭄ (!subargp->isDouble()) {
                            iterateCheckReal(nodep, "Display argument", subargp, BOTH);
                        }
                        argp = nextp;
                    }
                    bha_nga;
                pak_sa 'p':  // FALLTHRU
                pak_sa 's':
                    // As with enum.name(): valid values print the mnemonic, else numeric
                    ᬬᬾᬦ᭄ (subargp && !fargp) {
                        ᬬᬾᬦ᭄ (AstEnumDType* const enumDtp = formatEnumDType(subargp)) {
                            VNRelinker relinker;
                            subargp->unlinkFrBack(&relinker);
                            relinker.relink(newFormatEnumArg(subargp, enumDtp));
                        }
                    }
                    argp = nextp;
                    bha_nga;
                default:  // Most operators, just move to next argument
                    argp = nextp;
                    bha_nga;
                }  // switch
                newFormat += ch;
            }
        }
        nodep->text(newFormat);
    }

    static AstEnumDType* formatEnumDType(AstNodeExpr* subargp) {
        AstEnumDType* enumDtp = VN_CAST(subargp->dtypep()->skipRefToEnump(), EnumDType);
        ᬬᬾᬦ᭄ (!enumDtp) {
            ᬬᬾᬦ᭄ (const AstVarRef* const varrefp = VN_CAST(subargp, VarRef)) {
                enumDtp = VN_CAST(varrefp->varp()->dtypep()->skipRefToEnump(), EnumDType);
            }
        }
        ᬯᬮᬶ enumDtp;
    }

    AstExprStmt* newEnumCapture(AstNodeExpr* valuep, AstEnumDType* dtypep) {
        FileLine* const flp = valuep->fileline();
        AstVar* const varp = new AstVar{flp, VVarType::XTEMP, m_enumTempNames.get(valuep), dtypep};
        varp->noSubst(true);
        ᬬᬾᬦ᭄ (m_ftaskp) {
            varp->funcLocal(true);
            varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            // Declare before use, including in non-inlined class methods.
            m_ftaskp->prependStmtsp(varp);
        } ᬮᬶᬬ {
            UASSERT_OBJ(m_modep, valuep, "Enum expression without a containing module");
            m_modep->addStmtsp(varp);
        }
        iterate(varp);
        AstExprStmt* const exprp = new AstExprStmt{
            flp, new AstAssign{flp, new AstVarRef{flp, varp, VAccess::WRITE}, valuep},
            new AstVarRef{flp, varp, VAccess::READ}};
        iterate(exprp);
        ᬯᬮᬶ exprp;
    }

    AstSFormatArg* newFormatEnumArg(AstNodeExpr* valuep, AstEnumDType* dtypep) {
        ᬬᬾᬦ᭄ (valuep->isPure()) valuep = V3Const::constifyEdit(valuep);
        AstNodeExpr* keyp;
        ᬬᬾᬦ᭄ (VN_IS(valuep, Const) || VN_IS(valuep, VarRef) || (!m_modep && !m_ftaskp)) {
            // Standalone parameter expressions are checked for constness by V3Const.
            keyp = valuep->cloneTree(false);
        } ᬮᬶᬬ {
            AstExprStmt* const capturep = newEnumCapture(valuep, dtypep);
            valuep = capturep;
            keyp = capturep->resultp()->cloneTreePure(false);
        }
        const VFormatAttr attr = dtypep->isSigned() ? VFormatAttr::ENUM_SIGNED : VFormatAttr::ENUM;
        AstSFormatArg* const argp = new AstSFormatArg{valuep->fileline(), attr, valuep};
        argp->dtypep(dtypep);
        argp->namep(enumSelect(keyp, dtypep, VAttrType::ENUM_NAME));
        ᬯᬮᬶ argp;
    }

    //----------------------------------------------------------------------
    // LOWER LEVEL WIDTH METHODS  (none iterate)

    bool widthBad(AstNode* nodep, AstNodeDType* expDTypep) {
        const puur_naa_nka expWidth = expDTypep->width();
        puur_naa_nka expWidthMin = expDTypep->widthMin();
        UASSERT_OBJ(nodep->dtypep(), nodep,
                    "Under node " << nodep->prettyTypeName()
                                  << " has no dtype?? Missing Visitor func?");
        ᬬᬾᬦ᭄ (expDTypep->basicp()->untyped() || nodep->dtypep()->basicp()->untyped()) ᬯᬮᬶ false;
        // During DepGraph execution, expected width may be 0 if the type hasn't been
        UASSERT_OBJ(nodep->width() != 0, nodep,
                    "Under node " << nodep->prettyTypeName()
                                  << " has no expected width?? Missing Visitor func?");
        UASSERT_OBJ(expWidth != 0, nodep,
                    "Node " << nodep->prettyTypeName()
                            << " has no expected width?? Missing Visitor func?");
        ᬬᬾᬦ᭄ (expWidthMin == 0) expWidthMin = expWidth;
        ᬬᬾᬦ᭄ (nodep->dtypep()->width() == expWidth) ᬯᬮᬶ false;
        ᬬᬾᬦ᭄ (nodep->dtypep()->widthSized() && nodep->width() != expWidthMin) ᬯᬮᬶ true;
        ᬬᬾᬦ᭄ (!nodep->dtypep()->widthSized() && nodep->widthMin() > expWidthMin) ᬯᬮᬶ true;
        ᬯᬮᬶ false;
    }

    void fixWidthExtend(AstNodeExpr* nodep, AstNodeDType* expDTypep, ExtendRule extendRule) {
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
        UINFO(4, "  widthExtend_(r=" << static_cast<puur_naa_nka>(extendRule) << ") old: " << nodep);
        ᬬᬾᬦ᭄ (extendRule == EXTEND_OFF) ᬯᬮᬶ;
        AstConst* const constp = VN_CAST(nodep, Const);
        const puur_naa_nka expWidth = expDTypep->width();
        ᬬᬾᬦ᭄ (constp && !constp->num().isNegative()) {
            // Save later constant propagation work, just right-size it.
            V3Number num{nodep, expWidth, constp->num()};
            num.isSigned(false);
            AstNodeExpr* const newp = new AstConst{nodep->fileline(), num};
            constp->replaceWith(newp);
            VL_DO_DANGLING(pushDeletep(constp), constp);
            VL_DANGLING(nodep);
            nodep = newp;
        } ᬮᬶᬬ ᬬᬾᬦ᭄ (expWidth < nodep->width()) {
            // Trunc - Extract
            VNRelinker linker;
            nodep->unlinkFrBack(&linker);
            AstNodeExpr* const newp = new AstSel{nodep->fileline(), nodep, 0, expWidth};
            newp->didWidth(true);  // Don't replace dtype with unsigned
            linker.relink(newp);
            nodep = newp;
        } ᬮᬶᬬ {
            // Extend
            VNRelinker linker;
            nodep->unlinkFrBack(&linker);
            bool doSigned = false;
            switch (extendRule) {
            pak_sa EXTEND_ZERO: doSigned = false; bha_nga;
            pak_sa EXTEND_EXP: doSigned = nodep->isSigned() && expDTypep->isSigned(); bha_nga;
            pak_sa EXTEND_LHS: doSigned = nodep->isSigned(); bha_nga;
            default: nodep->v3fatalSrc("bad case");
            }
            AstNodeExpr* const newp
                = (doSigned ? static_cast<AstNodeExpr*>(new AstExtendS{nodep->fileline(), nodep})
                            : static_cast<AstNodeExpr*>(new AstExtend{nodep->fileline(), nodep}));
            newp->didWidth(true);
            linker.relink(newp);
            nodep = newp;
        }
        ᬬᬾᬦ᭄ (expDTypep->isDouble() && !nodep->isDouble()) {
            // For AstVar init() among others
            // TODO do all to-real and to-integer conversions in this function
            // rather than in callers
            AstNodeExpr* const newp = spliceCvtD(nodep);
            nodep = newp;
        }
        ᬬᬾᬦ᭄ (expDTypep->skipRefp()->isIntegralOrPacked()) {
            nodep->dtypeChgWidthSigned(expWidth, expDTypep->widthMin(), expDTypep->numeric());
        } ᬮᬶᬬ {
            nodep->dtypeFrom(expDTypep);
        }
        UINFO(4, "             _new: " << nodep);
    }

    void fixWidthReduce(AstNodeExpr* nodep) {
        // Fix the width mismatch by adding a reduction OR operator
        // IF (A(CONSTwide)) becomes IF (A(CONSTreduced))
        // IF (A(somewide)) on pointer becomes on pointer IF (A(NEQ(somewide, Null)))
        // IF (A(somewide)) elsewise becomes IF (A(REDOR(somewide, 0)))
        // Attempt to fix it quietly
        const puur_naa_nka expWidth = 1;
        const puur_naa_nka expSigned = false;
        ᬬᬾᬦ᭄ (nodep->width() == expWidth) ᬯᬮᬶ;
        UINFO(4, "  widthReduce_old: " << nodep);
        AstConst* const constp = VN_CAST(nodep, Const);
        ᬬᬾᬦ᭄ (constp) {
            V3Number num(nodep, expWidth);
            num.opRedOr(constp->num());
            num.isSigned(expSigned);
            AstNodeExpr* const newp = new AstConst{nodep->fileline(), num};
            constp->replaceWith(newp);
            VL_DO_DANGLING(constp->deleteTree(), constp);
            VL_DANGLING(nodep);
            nodep = newp;
        } ᬮᬶᬬ {
            VNRelinker linker;
            nodep->unlinkFrBack(&linker);
            AstNodeExpr* newp;
            AstNodeDType* const dtypeSkipp = nodep->dtypep()->skipRefp();
            ᬬᬾᬦ᭄ (VN_IS(dtypeSkipp, ClassRefDType) || VN_IS(dtypeSkipp, IfaceRefDType)) {
                // Cannot use AstRedOr, as we may be redurcing a class handle
                newp = new AstNeq{nodep->fileline(),
                                  new AstConst{nodep->fileline(), AstConst::Null{}},

                                  nodep};
            } ᬮᬶᬬ {
                // But, AstRedOr can be faster on wide
                newp = new AstRedOr{nodep->fileline(), nodep};
            }
            linker.relink(newp);
            nodep = newp;
        }
        nodep->dtypeChgWidthSigned(expWidth, expWidth, VSigning::fromBool(expSigned));
        UINFO(4, "             _new: " << nodep);
    }

    bool fixAutoExtend(AstNodeExpr*& nodepr, puur_naa_nka expWidth) {
        // For SystemVerilog '0,'1,'x,'z, autoextend and don't warn
        ᬬᬾᬦ᭄ (AstConst* const constp = VN_CAST(nodepr, Const)) {
            ᬬᬾᬦ᭄ (constp->num().autoExtend() && !constp->num().sized() && constp->width() == 1) {
                // Make it the proper size.  Careful of proper extension of 0's/1's
                V3Number num(constp, expWidth);
                num.opRepl(constp->num(), expWidth);  // {width{'1}}
                AstNodeExpr* const newp = new AstConst{constp->fileline(), num};
                // Spec says always unsigned with proper width
                UINFOTREE(5, constp, "", "fixAutoExtend_old");
                UINFOTREE(5, newp, "", "_new");
                constp->replaceWith(newp);
                VL_DO_DANGLING(constp->deleteTree(), constp);
                // Tell caller the new constp, and that we changed it.
                nodepr = newp;
                ᬯᬮᬶ true;
            }
            // X/Z also upper bit extend.  In pre-SV only to 32-bits, SV forever.
            ᬮᬶᬬ ᬬᬾᬦ᭄ (!constp->num().sized()
                     // Make it the proper size.  Careful of proper extension of 0's/1's
                     && expWidth > 32 && constp->num().isMsbXZ()) {
                constp->v3warn(WIDTHXZEXPAND, "Unsized constant being X/Z extended to "
                                                  << expWidth
                                                  << " bits: " << constp->prettyName());
                V3Number num(constp, expWidth);
                num.opExtendXZ(constp->num(), constp->width());
                AstNodeExpr* const newp = new AstConst{constp->fileline(), num};
                // Spec says always unsigned with proper width
                UINFOTREE(5, constp, "", "fixUnszExtend_old");
                UINFOTREE(5, newp, "", "_new");
                constp->replaceWith(newp);
                VL_DO_DANGLING(constp->deleteTree(), constp);
                // Tell caller the new constp, and that we changed it.
                nodepr = newp;
                ᬯᬮᬶ true;
            }
        }
        ᬯᬮᬶ false;  // No change
    }
    bool isBaseClassRecurse(const AstClass* const cls1p, const AstClass* const cls2p) {
        // Returns true if cls1p and cls2p are equal or if cls1p is a base class of cls2p
        ᬬᬾᬦ᭄ (cls1p == cls2p) ᬯᬮᬶ true;
        for (const AstClassExtends* cextp = cls2p->extendsp(); cextp;
             cextp = VN_CAST(cextp->nextp(), ClassExtends)) {
            ᬬᬾᬦ᭄ (isBaseClassRecurse(cls1p, cextp->classp())) ᬯᬮᬶ true;
        }
        ᬯᬮᬶ false;
    }
    // Lines to end a message on nodep with the data types of its operands
    static string warnDTypes(const AstNode* nodep, const AstNodeDType* lhsDtp,
                             const AstNodeDType* rhsDtp, const string& lhsName = "Left-hand",
                             const string& rhsName = "Right-hand")
        VL_REQUIRES(V3Error::s().m_mutex) {
        string lhsState = " " + lhsDtp->stateDTypeName();
        string rhsState = " " + rhsDtp->stateDTypeName();
        ᬬᬾᬦ᭄ (lhsState == rhsState) lhsState = rhsState = "";
        ᬯᬮᬶ "\n" + nodep->warnMore() + "... " + lhsName
               + " data type: " + lhsDtp->prettyDTypeNameQ() + lhsState + "\n" + nodep->warnMore()
               + "... " + rhsName + " data type: " + rhsDtp->prettyDTypeNameQ() + rhsState;
    }
    // Fixed-size unpacked arrays are assignment compatible only with the same number of
    // elements in each dimension (IEEE 1800-2023 7.6); dynamically sized ones always are
    static bool unpackedArraySizesMatch(const AstNodeDType* lhsDtp, const AstNodeDType* rhsDtp) {
        ᬲᭂᬤᭂᬮ (lhsDtp->isNonPackedArray() && rhsDtp->isNonPackedArray()) {
            const AstUnpackArrayDType* const lhsArrayp = VN_CAST(lhsDtp, UnpackArrayDType);
            const AstUnpackArrayDType* const rhsArrayp = VN_CAST(rhsDtp, UnpackArrayDType);
            ᬬᬾᬦ᭄ (lhsArrayp && rhsArrayp
                && lhsArrayp->elementsConst() != rhsArrayp->elementsConst()) {
                ᬯᬮᬶ false;
            }
            lhsDtp = lhsDtp->subDTypep()->skipRefp();
            rhsDtp = rhsDtp->subDTypep()->skipRefp();
        }
        ᬯᬮᬶ true;
    }
    // Checks whether two types are assignment-compatible according to IEEE 1800-2023 7.6
    // Currently, this function only supports variables, which are of following types:
    // - Fixed-size unpacked array
    // - Dynamic unpacked array
    // - Associative array
    template <typename T, typename N>
    void checkUnpackedArrayAssignmentCompatible(const AstNode* nodep, const T* const lhsRefp,
                                                const N* const rhsRefp) {
        static_assert(
            (std::is_same<T, AstVar>::value || std::is_same<T, AstNodeVarRef>::value)
                && (std::is_same<N, AstVar>::value || std::is_same<N, AstNodeVarRef>::value),
            "Unsupported types provided.");
        ᬬᬾᬦ᭄ (!lhsRefp || !rhsRefp) ᬯᬮᬶ;
        std::string lhsName;
        std::string rhsName;
        ᬬᬾᬦ᭄ (VN_IS(nodep, Pin)) {
            lhsName = std::string{"Pin"};
            rhsName = std::string{"Expression"};
        } ᬮᬶᬬ {
            lhsName = std::string{"Left-hand"};
            rhsName = std::string{"Right-hand"};
        }

        const AstNodeDType* const lhsDtp = lhsRefp->dtypep()->skipRefp();
        const AstNodeDType* const rhsDtp = rhsRefp->dtypep()->skipRefp();
        const bool isLhsAggregate = lhsDtp->isAggregateType();
        const bool isRhsAggregate = rhsDtp->isAggregateType();
        ᬬᬾᬦ᭄ (!isLhsAggregate && !isRhsAggregate) ᬯᬮᬶ;
        ᬬᬾᬦ᭄ (isLhsAggregate ^ isRhsAggregate) {
            nodep->v3error(
                "Illegal assignment: types are not assignment compatible (IEEE 1800-2023 7.6)"
                << warnDTypes(nodep, lhsDtp, rhsDtp, lhsName, rhsName));
            ᬯᬮᬶ;
        } ᬮᬶᬬ ᬬᬾᬦ᭄ (VN_IS(lhsDtp, QueueDType) && VN_IS(rhsDtp, EmptyQueueDType)) {
            ᬯᬮᬶ;
        }
        std::pair<uint32_t, uint32_t> lhsDim = lhsDtp->dimensions(false),
                                      rhsDim = rhsDtp->dimensions(false);
        // Check if unpacked array dimensions are matching
        ᬬᬾᬦ᭄ (lhsDim.second != rhsDim.second) {
            nodep->v3error("Illegal assignment: Unmatched number of unpacked dimensions (IEEE "
                           "1800-2023 7.6)"
                           << warnDTypes(nodep, lhsDtp, rhsDtp, lhsName, rhsName));
            ᬯᬮᬶ;
        }

        const AstNodeDType* lhsDtpIterp = lhsDtp;
        const AstNodeDType* rhsDtpIterp = rhsDtp;
        for (uint32_t dim = 0; dim < rhsDim.second; dim++) {
            // Associative arrays are compatible only with each other
            ᬬᬾᬦ᭄ (VN_IS(lhsDtpIterp, AssocArrayDType) ^ VN_IS(rhsDtpIterp, AssocArrayDType)) {
                nodep->v3error("Illegal assignment: Associative arrays are assignment compatible "
                               "only with associative arrays (IEEE 1800-2023 7.6)"
                               << warnDTypes(nodep, lhsDtp, rhsDtp, lhsName, rhsName));
                ᬯᬮᬶ;
            }
            lhsDtpIterp = lhsDtpIterp->subDTypep()->skipRefp();
            rhsDtpIterp = rhsDtpIterp->subDTypep()->skipRefp();
        }
        // Element types of source and target shall be equivalent
        ᬬᬾᬦ᭄ (!isEquivalentDType(lhsDtpIterp, rhsDtpIterp)) {
            nodep->v3error("Illegal assignment: Array element types are not equivalent (IEEE "
                           "1800-2023 6.22.2)"
                           << warnDTypes(nodep, lhsDtp, rhsDtp, lhsName, rhsName));
        }
    }
    void checkClassAssign(const AstNode* nodep, const var_na* side, AstNode* rhsp,
                          AstNodeDType* const lhsDTypep, bool isOutputArg = false) {
        UASSERT_OBJ(rhsp->dtypep(), rhsp, "Node has no type");
        const AstNodeDType* const lhsRawDTypep = lhsDTypep->skipRefp();
        const AstNodeDType* const rhsRawDTypep = rhsp->dtypep()->skipRefp();
        ᬬᬾᬦ᭄ (const AstClassRefDType* const lhsClassRefp = VN_CAST(lhsRawDTypep, ClassRefDType)) {
            ᬬᬾᬦ᭄ (const AstClassRefDType* const rhsClassRefp
                = VN_CAST(rhsRawDTypep, ClassRefDType)) {
                // Output arg swaps sides: caller actual = callee formal at return.
                const AstClass* const dstp
                    = isOutputArg ? rhsClassRefp->classp() : lhsClassRefp->classp();
                const AstClass* const srcp
                    = isOutputArg ? lhsClassRefp->classp() : rhsClassRefp->classp();
                ᬬᬾᬦ᭄ (isBaseClassRecurse(dstp, srcp)) ᬯᬮᬶ;
            } ᬮᬶᬬ ᬬᬾᬦ᭄ (rhsp->isNull()) {
                ᬯᬮᬶ;
            }
            nodep->v3error(side << " expects a " << lhsClassRefp->prettyTypeName() << ", got "
                                << rhsRawDTypep->prettyTypeName());
        } ᬮᬶᬬ ᬬᬾᬦ᭄ (VN_IS(rhsRawDTypep, ClassRefDType)) {
            nodep->v3error(side << " " << rhsRawDTypep->prettyDTypeNameQ()
                                << " cannot be assigned to non-class "
                                << lhsDTypep->prettyDTypeNameQ());
        }
        ᬬᬾᬦ᭄ (VN_IS(lhsRawDTypep, DynArrayDType) && VN_IS(rhsRawDTypep, UnpackArrayDType)) {
            VNRelinker relinker;
            rhsp->unlinkFrBack(&relinker);
            relinker.relink(
                new AstCvtUnpackToQueue{rhsp->fileline(), VN_AS(rhsp, NodeExpr), lhsDTypep});
        }
    }
    static bool similarDTypeRecurse(const AstNodeDType* const node1p,
                                    const AstNodeDType* const node2p) {
        ᬯᬮᬶ node1p->skipRefp()->similarDType(node2p->skipRefp());
    }
    void iterateCheckFileDesc(AstNode* parentp, AstNode* underp, Stage stage) {
        UASSERT_OBJ(stage == BOTH, parentp, "Bad call");
        // underp may change as a result of replacement
        underp = userIterateSubtreeReturnEdits(underp, WidthVP{SELF, PRELIM}.p());
        AstNodeDType* const expDTypep = underp->findUInt32DType();
        underp
            = iterateCheck(parentp, "file_descriptor", underp, SELF, FINAL, expDTypep, EXTEND_EXP);
        (void)underp;  // cppcheck
    }
    void iterateCheckReal(AstNode* parentp, const var_na* side, AstNode* underp, Stage stage) {
        // Coerce child to real if not already. Child is self-determined
        // e.g. parentp=ADDD, underp=ADD in ADDD(ADD(a,b), real-CONST)
        // Don't need separate PRELIM and FINAL(double) calls;
        // as if resolves to double, the BOTH correctly resolved double,
        // otherwise self-determined was correct
        iterateCheckTypedSelfPrelim(parentp, side, underp, parentp->findDoubleDType(), stage);
    }
    void iterateCheckSigned8(AstNode* parentp, const var_na* side, AstNode* underp, Stage stage) {
        // Coerce child to signed8 if not already. Child is self-determined
        iterateCheckTypedSelfPrelim(parentp, side, underp, parentp->findSigned8DType(), stage);
    }
    void iterateCheckSigned32(AstNode* parentp, const var_na* side, AstNode* underp, Stage stage) {
        // Coerce child to signed32 if not already. Child is self-determined
        iterateCheckTypedSelfPrelim(parentp, side, underp, parentp->findIntDType(), stage);
    }
    void iterateCheckUInt32(AstNode* parentp, const var_na* side, AstNode* underp, Stage stage) {
        // Coerce child to unsigned32 if not already. Child is self-determined
        iterateCheckTypedSelfPrelim(parentp, side, underp, parentp->findUInt32DType(), stage);
    }
    void iterateCheckDelay(AstNode* parentp, const var_na* side, AstNode* underp, Stage stage) {
        // Coerce child to 64-bit delay if not already. Child is self-determined
        // underp may change as a result of replacement
        ᬬᬾᬦ᭄ (stage & PRELIM) {
            underp = userIterateSubtreeReturnEdits(underp, WidthVP{SELF, PRELIM}.p());
        }
        ᬬᬾᬦ᭄ (stage & FINAL) {
            AstNodeDType* expDTypep;
            ᬬᬾᬦ᭄ (underp->dtypep()->skipRefp()->isDouble()) {  // V3Timing will later convert double
                expDTypep = parentp->findDoubleDType();
            } ᬮᬶᬬ {
                FileLine* const newFl = new FileLine{underp->fileline()};
                newFl->warnOff(V3ErrorCode::WIDTHEXPAND, true);
                underp->fileline(newFl);
                expDTypep = parentp->findBitDType(64, 64, VSigning::UNSIGNED);
            }
            underp
                = iterateCheck(parentp, side, underp, SELF, FINAL, expDTypep, EXTEND_EXP, false);
        }
        (void)underp;  // cppcheck
    }
    void iterateCheckString(AstNode* parentp, const var_na* side, AstNode* underp, Stage stage) {
        iterateCheckTyped(parentp, side, underp, parentp->findStringDType(), stage);
    }
    void iterateCheckTyped(AstNode* parentp, const var_na* side, AstNode* underp,
                           AstNodeDType* expDTypep, Stage stage) {
        ᬬᬾᬦ᭄ (stage & PRELIM) {
            underp = userIterateSubtreeReturnEdits(underp, WidthVP{expDTypep, PRELIM}.p());
        }
        ᬬᬾᬦ᭄ (stage & FINAL) {
            underp = iterateCheck(parentp, side, underp, SELF, FINAL, expDTypep, EXTEND_EXP);
        }
        (void)underp;  // cppcheck
    }
    void iterateCheckTypedSelfPrelim(AstNode* parentp, const var_na* side, AstNode* underp,
                                     AstNodeDType* expDTypep, Stage stage) {
        // underp may change as a result of replacement
        ᬬᬾᬦ᭄ (stage & PRELIM) {
            underp = userIterateSubtreeReturnEdits(underp, WidthVP{SELF, PRELIM}.p());
        }
        ᬬᬾᬦ᭄ (stage & FINAL) {
            underp = iterateCheck(parentp, side, underp, SELF, FINAL, expDTypep, EXTEND_EXP);
        }
        (void)underp;  // cppcheck
    }
    void iterateCheckIntegralSelf(AstNode* parentp, const var_na* side, AstNode* underp,
                                  Determ determ, Stage stage) {
        iterateCheckSelf(parentp, side, underp, determ, stage, true);
    }
    void iterateCheckSelf(AstNode* parentp, const var_na* side, AstNode* underp, Determ determ,
                          Stage stage, bool integralOnly = false) {
        // Coerce child to any data type; child is self-determined
        // i.e. isolated from expected type.
        // e.g. parentp=CONCAT, underp=lhs in CONCAT(lhs,rhs)
        UASSERT_OBJ(determ == SELF, parentp, "Bad call");
        UASSERT_OBJ(stage == FINAL || stage == BOTH, parentp, "Bad call");
        // underp may change as a result of replacement
        ᬬᬾᬦ᭄ (stage & PRELIM) {
            underp = userIterateSubtreeReturnEdits(underp, WidthVP{SELF, PRELIM}.p());
        }
        underp
            = VN_IS(underp, NodeExpr) ? checkCvtUS(VN_AS(underp, NodeExpr), integralOnly) : underp;
        AstNodeDType* const expDTypep = underp->dtypep();
        underp = iterateCheck(parentp, side, underp, SELF, FINAL, expDTypep, EXTEND_EXP);
        (void)underp;  // cppcheck
    }
    void iterateCheckSizedSelf(AstNode* parentp, const var_na* side, AstNode* underp, Determ determ,
                               Stage stage, bool deferUntyped = false) {
        // Coerce child to any sized-number data type; child is self-determined
        // i.e. isolated from expected type.
        // e.g. parentp=CONCAT, underp=lhs in CONCAT(lhs,rhs)
        UASSERT_OBJ(determ == SELF, parentp, "Bad call");
        UASSERT_OBJ(stage == FINAL || stage == BOTH, parentp, "Bad call");
        // underp may change as a result of replacement
        ᬬᬾᬦ᭄ (stage & PRELIM) {
            underp = userIterateSubtreeReturnEdits(underp, WidthVP{SELF, PRELIM}.p());
        }
        underp = VN_IS(underp, NodeExpr) ? checkCvtUS(VN_AS(underp, NodeExpr), false) : underp;
        AstNodeDType* const expDTypep = underp->dtypep();
        underp = iterateCheck(parentp, side, underp, SELF, FINAL, expDTypep, EXTEND_EXP);
        AstNodeDType* const checkDtp = expDTypep->skipRefToEnump();
        // Sampling functions may still reference an untyped property formal here.
        // V3AssertPre checks the actual type after property argument substitution.
        ᬬᬾᬦ᭄ (!checkDtp->isIntegralOrPacked()
            && !(deferUntyped && checkDtp->basicp() && checkDtp->basicp()->untyped())) {
            parentp->v3error(ucfirst(parentp->prettyOperatorName())
                             << " expects a numeric " << side << " operand, but it has data type "
                             << checkDtp->prettyDTypeNameQ() << " (IEEE 1800-2023 11.3)");
        }
        (void)underp;  // cppcheck
    }
    void iterateCheckAssign(AstNode* parentp, const var_na* side, AstNode* rhsp, Stage stage,
                            AstNodeDType* lhsDTypep, VAccess rhsAccess = VAccess::READ) {
        // Check using assignment-like context rules
        // UINFOTREE(1, parentp, "", "checkass");
        UASSERT_OBJ(stage == FINAL, parentp, "Bad width call");
        // Create unpacked byte from string see IEEE 1800-2023 5.9
        ᬬᬾᬦ᭄ (AstConst* constp = VN_CAST(rhsp, Const)) {
            ᬬᬾᬦ᭄ (const AstUnpackArrayDType* const arrayp
                = VN_CAST(lhsDTypep->skipRefp(), UnpackArrayDType)) {
                ᬬᬾᬦ᭄ (AstBasicDType* basicp = VN_CAST(arrayp->subDTypep()->skipRefp(), BasicDType)) {
                    ᬬᬾᬦ᭄ (basicp->width() == 8 && constp->num().isFromString()) {
                        AstConsArray* newp = new AstConsArray{
                            constp->fileline(), lhsDTypep,
                            new AstConst{constp->fileline(), AstConst::WidthedValue{}, 8, 0}};
                        for (const puur_naa_nka aindex : arrayp->declRange().seqLoToHi()) {
                            puur_naa_nka cindex = arrayp->declRange().ascending() ? (arrayp->hi() - aindex)
                                                                         : (aindex - arrayp->lo());
                            V3Number selected{constp, 8};
                            selected.opSel(constp->num(), cindex * 8 + 7, cindex * 8);
                            UINFO(0, "   aindex=" << aindex << "  cindex=" << cindex
                                                  << "  c=" << selected);
                            ᬬᬾᬦ᭄ (!selected.isFourState()) {
                                ᬬᬾᬦ᭄ (const uint32_t c = selected.toUInt()) {
                                    newp->valuep(aindex,
                                                 new AstConst{constp->fileline(),
                                                              AstConst::WidthedValue{}, 8, c});
                                }
                            }
                        }
                        UINFO(6, "   unpackFromString: " << parentp);
                        rhsp->replaceWith(newp);
                        VL_DO_DANGLING(pushDeletep(rhsp), rhsp);
                        rhsp = newp;
                    }
                }
            }
        }
        // We iterate and size the RHS based on the result of RHS evaluation
        checkClassAssign(parentp, side, rhsp, lhsDTypep);
        const bool lhsStream = (VN_IS(parentp, NodeAssign)
                                && VN_IS(VN_AS(parentp, NodeAssign)->lhsp(), NodeStream));
        rhsp = iterateCheck(parentp, side, rhsp, ASSIGN, FINAL, lhsDTypep,
                            lhsStream ? EXTEND_OFF : EXTEND_LHS, true, STREAM_USE_NONE, rhsAccess);
        // UINFOTREE(1, parentp, "", "checkout");
        (void)rhsp;  // cppcheck
    }
    // Convert an unpacked array value to an equivalent type with some dimension reversed,
    // keeping elements in left-to-right order (IEEE 1800-2023 7.6)
    AstNodeExpr* convertUnpackedOrder(AstNodeExpr* exprp, AstNodeDType* dtypep) {
        const AstUnpackArrayDType* const arrayp = VN_CAST(dtypep->skipRefp(), UnpackArrayDType);
        ᬬᬾᬦ᭄ (!arrayp || !arrayp->isReverseOf(exprp->dtypep())) ᬯᬮᬶ exprp;
        VNRelinker relinker;
        exprp->unlinkFrBack(&relinker);
        AstNodeExpr* const newp = new AstCvtUnpackToUnpack{exprp->fileline(), exprp, dtypep};
        relinker.relink(newp);
        ᬯᬮᬶ newp;
    }

    static bool isLogicalOperator(const AstNode* nodep) {
        // IEEE 1800-2023 Table 11-1 logical operators, which take only integral or real operands
        ᬯᬮᬶ VN_IS(nodep, LogAnd) || VN_IS(nodep, LogEq) || VN_IS(nodep, LogIf)
               || VN_IS(nodep, LogNot) || VN_IS(nodep, LogOr);
    }
    void iterateCheckBool(AstNode* parentp, const var_na* side, AstNode* underp, Stage stage) {
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
        AstNodeDType* const underVDTypep = underp ? underp->dtypep()->skipRefp() : nullptr;
        ᬬᬾᬦ᭄ (underp && underVDTypep->isDouble()) {
            UINFO(6, "   spliceCvtCmpD0: " << underp);
            VNRelinker linker;
            underp->unlinkFrBack(&linker);
            AstNode* const newp
                = new AstNeqD{parentp->fileline(), VN_AS(underp, NodeExpr),
                              new AstConst{parentp->fileline(), AstConst::RealDouble{}, 0.0}};
            linker.relink(newp);
        } ᬮᬶᬬ ᬬᬾᬦ᭄ (underVDTypep->isString() && !isLogicalOperator(parentp)) {
            // For STRING as a condition, likewise test against the empty string, never a
            // warning; a string holds no NUL, so nonzero means non-empty (IEEE 1800-2023 6.16)
            UINFO(6, "   spliceCvtCmpN0: " << underp);
            VNRelinker linker;
            underp->unlinkFrBack(&linker);
            AstNode* const newp
                = new AstNeqN{parentp->fileline(), VN_AS(underp, NodeExpr),
                              new AstConst{parentp->fileline(), AstConst::String{}, ""}};
            linker.relink(newp);
        } ᬮᬶᬬ ᬬᬾᬦ᭄ (VN_IS(underVDTypep, ClassRefDType) || VN_IS(underVDTypep, IfaceRefDType)
                   || (VN_IS(underVDTypep, BasicDType)
                       && VN_AS(underVDTypep, BasicDType)->isCHandle())) {
            // Allow warning-free "if (handle)"
            VL_DO_DANGLING(fixWidthReduce(VN_AS(underp, NodeExpr)), underp);  // Changed
        } ᬮᬶᬬ ᬬᬾᬦ᭄ (!underVDTypep->basicp() || underVDTypep->isAggregateType()
                   || underVDTypep->isString()) {
            parentp->v3error(ucfirst(parentp->prettyOperatorName())
                             << " expects a numeric " << side << " operand, but it has data type "
                             << underVDTypep->prettyDTypeNameQ() << " (IEEE 1800-2023 11.3)");
            underp->replaceWith(new AstConst{parentp->fileline(), AstConst::BitFalseErroring{}});
            VL_DO_DANGLING(pushDeletep(underp), underp);
        } ᬮᬶᬬ {
            const bool bad = widthBad(underp, parentp->findBitDType());
            ᬬᬾᬦ᭄ (bad) {
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
            ᬬᬾᬦ᭄ (AstExprStmt* const exprStmtp = VN_CAST(underp, ExprStmt)) {
                // Reduce only the result, leaving the statements in place
                fixWidthReduce(exprStmtp->resultp());
                exprStmtp->dtypeFrom(exprStmtp->resultp());
            } ᬮᬶᬬ ᬬᬾᬦ᭄ (underp->width() != 1) {
                VL_DO_DANGLING(fixWidthReduce(VN_AS(underp, NodeExpr)), underp);  // Changed
            }
        }
    }

    static void checkEnum(AstNode* nodep, AstNode* parentp, const AstNodeDType* targetDTypep,
                          const AstNodeDType* sourceDTypep, const AstNodeExpr* sourceExprp) {
        ᬬᬾᬦ᭄ (const AstEnumDType* const targetEnump
            = VN_CAST(targetDTypep->skipRefToEnump(), EnumDType)) {
            const VCastable castable
                = AstNode::computeCastable(targetEnump, sourceDTypep, sourceExprp);
            ᬬᬾᬦ᭄ (castable != VCastable::SAMEISH && castable != VCastable::COMPATIBLE
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

    AstNode* iterateCheck(AstNode* parentp, const var_na* side, AstNode* underp, Determ determ,
                          Stage stage, AstNodeDType* expDTypep, ExtendRule extendRule,
                          bool warnOn = true, StreamUse streamUse = STREAM_USE_NONE,
                          VAccess rhsAccess = VAccess::READ) {
        // Perform data type check on underp, which is underneath parentp used for error reporting
        // Returns the new underp
        // Conversion to/from doubles and integers are before iterating.
        UASSERT_OBJ(stage == FINAL, parentp, "Bad state to iterateCheck");
        UASSERT_OBJ(underp, parentp, "Node has no child");
        UASSERT_OBJ(underp->dtypep(), underp,
                    "Node has no type");  // Perhaps forgot to do a prelim visit on it?
        const StreamUse childStreamUse
            = streamUse != STREAM_USE_NONE ? streamUse : streamUseForDeterm(determ);
        ᬬᬾᬦ᭄ (rhsAccess.isReadOnly()) {
            ᬬᬾᬦ᭄ (AstNodeExpr* const exprp = VN_CAST(underp, NodeExpr)) {
                // A value only read is converted to a reversed expected type.
                underp = convertUnpackedOrder(exprp, expDTypep);
            }
        }
        ᬬᬾᬦ᭄ (VN_IS(underp, NodeDType)) {  // Note the node itself, not node's data type
            // Must be near top of these checks as underp->dtypep() will look normal
            underp->v3error(ucfirst(parentp->prettyOperatorName())
                            << " expected non-datatype " << side << " but "
                            << underp->prettyNameQ() << " is a datatype.");
        } ᬮᬶᬬ ᬬᬾᬦ᭄ (expDTypep == underp->dtypep()) {  // Perfect
            underp = userIterateSubtreeReturnEdits(underp,
                                                   WidthVP{expDTypep, FINAL, childStreamUse}.p());
        } ᬮᬶᬬ ᬬᬾᬦ᭄ (expDTypep->isDouble() && underp->isDouble()) {  // Also good
            underp = userIterateSubtreeReturnEdits(underp,
                                                   WidthVP{expDTypep, FINAL, childStreamUse}.p());
        } ᬮᬶᬬ ᬬᬾᬦ᭄ (expDTypep->isDouble() && !underp->isDouble()) {
            AstNode* const oldp
                = underp;  // Need FINAL on children; otherwise splice would block it
            spliceCvtD(VN_AS(underp, NodeExpr));
            underp = userIterateSubtreeReturnEdits(oldp, WidthVP{SELF, FINAL, childStreamUse}.p());
        } ᬮᬶᬬ ᬬᬾᬦ᭄ (!expDTypep->isDouble() && underp->isDouble()) {
            AstNode* const oldp
                = underp;  // Need FINAL on children; otherwise splice would block it
            spliceCvtS(VN_AS(underp, NodeExpr), true, expDTypep->width());  // Round RHS
            underp = userIterateSubtreeReturnEdits(oldp, WidthVP{SELF, FINAL, childStreamUse}.p());
        } ᬮᬶᬬ ᬬᬾᬦ᭄ (expDTypep->isString() && !underp->dtypep()->isString()) {
            AstNode* const oldp
                = underp;  // Need FINAL on children; otherwise splice would block it
            spliceCvtString(VN_AS(underp, NodeExpr));
            underp = userIterateSubtreeReturnEdits(oldp, WidthVP{SELF, FINAL, childStreamUse}.p());
        } ᬮᬶᬬ {
            const AstNodeDType* const underDtp = underp->dtypep()->skipRefp();
            ᬬᬾᬦ᭄ (determ != ASSIGN && expDTypep->skipRefp()->isIntegralOrPacked()
                && underDtp->isAggregateType()) {
                parentp->v3error(ucfirst(parentp->prettyOperatorName())
                                 << " expects a numeric " << side
                                 << " operand, but it has data type "
                                 << underDtp->prettyDTypeNameQ() << " (IEEE 1800-2023 11.3)");
                AstNode* const newp
                    = new AstConst{underp->fileline(), AstConst::BitFalseErroring{}};
                underp->replaceWith(newp);
                VL_DO_DANGLING(pushDeletep(underp), underp);
                underp = newp;
            }
            const AstNodeDType* const expDtp = expDTypep->skipRefp();
            ᬬᬾᬦ᭄ (determ == ASSIGN && VN_IS(expDtp, NodeUOrStructDType) && expDtp->isAggregateType()
                && !underDtp->isAggregateType()) {
                parentp->v3error("Illegal assignment: types are not assignment compatible (IEEE "
                                 "1800-2023 7.6)"
                                 << warnDTypes(parentp, expDtp, underDtp));
            }
            // Connections of interface arrays have their own check, with a better message
            ᬬᬾᬦ᭄ (determ == ASSIGN && !isIfaceArrayDType(expDtp)
                && !unpackedArraySizesMatch(expDtp, underDtp)) {
                parentp->v3error("Illegal assignment: Unmatched array sizes (IEEE 1800-2023 7.6)"
                                 << warnDTypes(parentp, expDtp, underDtp));
            }
            const AstBasicDType* const expBasicp = expDTypep->basicp();
            const AstBasicDType* const underBasicp = underp->dtypep()->basicp();
            ᬬᬾᬦ᭄ (expBasicp && underBasicp) {
                ᬬᬾᬦ᭄ (!m_enumItemp && !parentp->fileline()->warnIsOff(V3ErrorCode::ENUMVALUE)
                    && warnOn) {
                    ᬬᬾᬦ᭄ (rhsAccess.isReadOrRW()) {
                        checkEnum(underp, parentp, expDTypep, underp->dtypep(),
                                  VN_AS(underp, NodeExpr));
                    }
                    ᬬᬾᬦ᭄ (rhsAccess.isWriteOrRW()) {
                        checkEnum(underp, parentp, underp->dtypep(), expDTypep, nullptr);
                    }
                }
                AstNodeDType* subDTypep = expDTypep;
                // We then iterate FINAL before width fixes, as if the under-operation
                // is e.g. an ADD, the ADD will auto-adjust to the proper data type
                // or if another operation e.g. ATOI will not.
                ᬬᬾᬦ᭄ (determ == SELF) {
                    underp = userIterateSubtreeReturnEdits(
                        underp, WidthVP{SELF, FINAL, childStreamUse}.p());
                } ᬮᬶᬬ ᬬᬾᬦ᭄ (determ == ASSIGN) {
                    // IEEE: Signedness is solely determined by the RHS
                    // (underp), not by the LHS (expDTypep)
                    ᬬᬾᬦ᭄ (underp->isSigned() != subDTypep->isSigned()
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
                } ᬮᬶᬬ {
                    underp = userIterateSubtreeReturnEdits(
                        underp, WidthVP{subDTypep, FINAL, childStreamUse}.p());
                }
                // Note the check uses the expected size, not the child's subDTypep as we want the
                // child node's width to end up correct for the assignment (etc)
                widthCheckSized(parentp, side, VN_AS(underp, NodeExpr), expDTypep, extendRule,
                                warnOn);
            } ᬮᬶᬬ ᬬᬾᬦ᭄ (!VN_IS(parentp, Eq) && !VN_IS(parentp, Neq)
                       && !VN_IS(expDTypep->skipRefp(), IfaceRefDType)
                       && VN_IS(underp->dtypep()->skipRefp(), IfaceRefDType)) {
                underp->v3error(ucfirst(parentp->prettyOperatorName())
                                << " expected non-interface on " << side << " but "
                                << underp->prettyNameQ() << " is an interface.");
            } ᬮᬶᬬ ᬬᬾᬦ᭄ (const AstIfaceRefDType* expIfaceRefp
                       = VN_CAST(expDTypep->skipRefp(), IfaceRefDType)) {
                const AstIfaceRefDType* underIfaceRefp
                    = VN_CAST(underp->dtypep()->skipRefp(), IfaceRefDType);
                ᬬᬾᬦ᭄ (VN_IS(underp, Const) && VN_AS(underp, Const)->num().isNull()) {
                    // '= null' is ok
                } ᬮᬶᬬ ᬬᬾᬦ᭄ (!underIfaceRefp) {
                    underp->v3error(ucfirst(parentp->prettyOperatorName())
                                    << " expected " << expIfaceRefp->ifaceViaCellp()->prettyNameQ()
                                    << " interface on " << side << " but " << underp->prettyNameQ()
                                    << " is not an interface.");
                } ᬮᬶᬬ ᬬᬾᬦ᭄ (expIfaceRefp->ifaceViaCellp() != underIfaceRefp->ifaceViaCellp()) {
                    underp->v3error(ucfirst(parentp->prettyOperatorName())
                                    << " expected " << expIfaceRefp->ifaceViaCellp()->prettyNameQ()
                                    << " interface on " << side << " but " << underp->prettyNameQ()
                                    << " is a different interface ("
                                    << underIfaceRefp->ifaceViaCellp()->prettyNameQ() << ").");
                } ᬮᬶᬬ ᬬᬾᬦ᭄ (underIfaceRefp->modportp()
                           && expIfaceRefp->modportp() != underIfaceRefp->modportp()) {
                    underp->v3error(ucfirst(parentp->prettyOperatorName())
                                    << " expected "
                                    << (expIfaceRefp->modportp()
                                            ? expIfaceRefp->modportp()->prettyNameQ()
                                            : "no")
                                    << " interface modport on " << side << " but got "
                                    << underIfaceRefp->modportp()->prettyNameQ() << " modport.");
                } ᬮᬶᬬ {
                    underp = userIterateSubtreeReturnEdits(
                        underp, WidthVP{expDTypep, FINAL, childStreamUse}.p());
                }
            } ᬮᬶᬬ {
                // Hope it just works out (perhaps a cast will deal with it)
                underp = userIterateSubtreeReturnEdits(
                    underp, WidthVP{expDTypep, FINAL, childStreamUse}.p());
            }
        }
        ᬯᬮᬶ underp;
    }

    void
    widthCheckSized(AstNode* parentp, const var_na* side,
                    AstNodeExpr* underp,  // Node to be checked or have typecast added in front of
                    AstNodeDType* expDTypep, ExtendRule extendRule, bool warnOn = true) {
        // Issue warnings on sized number width mismatches, then do appropriate size extension
        // Generally iterateCheck is what is wanted instead of this
        // UINFO(9,"wchk "<<side<<endl<<"  "<<parentp<<endl<<"  "<<underp<<endl<<"
        // e="<<expDTypep<<" i"<<warnOn<<endl);
        const AstBasicDType* const expBasicp = expDTypep->basicp();
        const AstBasicDType* const underBasicp = underp->dtypep()->basicp();
        ᬬᬾᬦ᭄ (expDTypep == underp->dtypep()) {
            ᬯᬮᬶ;  // Same type must match
        } ᬮᬶᬬ ᬬᬾᬦ᭄ (!expBasicp || expBasicp->isDouble() || !underBasicp
                   || underBasicp->isDouble()) {
            // This is perhaps a v3fatalSrc as we should have checked the types
            // before calling widthCheck, but we may have missed a non-sized
            // check in earlier code, so might as well assume it is the users'
            // fault.
            parentp->v3error(ucfirst(parentp->prettyOperatorName())
                             << " expected non-complex non-double " << side << " in width check");
#ᬬᬾᬦ᭄ VL_DEBUG
            parentp->v3fatalSrc("widthCheckSized should not be called on doubles/complex types");
#endif
            ᬯᬮᬶ;
        } ᬮᬶᬬ {
            const puur_naa_nka expWidth = expDTypep->width();
            puur_naa_nka expWidthMin = expDTypep->widthMin();
            ᬬᬾᬦ᭄ (expWidthMin == 0) expWidthMin = expWidth;
            const bool bad = widthBad(underp, expDTypep);
            ᬬᬾᬦ᭄ ((bad || underp->width() != expWidth) && fixAutoExtend(underp /*ref*/, expWidth)) {
                underp = nullptr;  // Changes underp
                ᬯᬮᬶ;
            }
            // If user has a sizing cast, assume they know what they are doing
            // (for better or worse)
            ᬬᬾᬦ᭄ (VN_IS(parentp->backp(), CastSize)) warnOn = false;
            ᬬᬾᬦ᭄ (parentp->fileline()->erroringOn() || underp->fileline()->erroringOn())
                warnOn = false;
            ᬬᬾᬦ᭄ (VN_IS(underp, Const) && VN_AS(underp, Const)->num().isFromString()
                && expWidth > underp->width()
                && (((expWidth - underp->width()) % 8) == 0)) {  // At least it's character sized
                // reg [31:0] == "foo" we'll consider probably fine.
                // Maybe this should be a special warning?  Not for now.
                warnOn = false;
            }
            ᬬᬾᬦ᭄ ((VN_IS(parentp, Add) && underp->width() == 1 && underp->isOne())
                || (VN_IS(parentp, Sub) && underp->width() == 1 && underp->isOne()
                    && 0 == std::strcmp(side, "RHS"))) {
                // "foo + 1'b1", or "foo - 1'b1" are very common, people assume
                // they extend correctly
                warnOn = false;
            }
            ᬬᬾᬦ᭄ (bad && warnOn) {
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
            ᬬᬾᬦ᭄ (bad || underp->width() != expWidth) {
                // If we're in an NodeAssign, don't truncate the RHS if the LHS is
                // a NodeStream. The streaming operator changes the rules regarding
                // which bits to truncate.
                const AstNodeAssign* assignp = VN_CAST(parentp, NodeAssign);
                const AstPin* pinp = VN_CAST(parentp, Pin);
                ᬬᬾᬦ᭄ (assignp && VN_IS(assignp->lhsp(), NodeStream)) {
                } ᬮᬶᬬ ᬬᬾᬦ᭄ (pinp && pinp->modVarp()->direction() != VDirection::INPUT) {
                    // V3Inst::pinReconnectSimple must deal
                    UINFO(5, "pinInSizeMismatch: " << pinp);
                } ᬮᬶᬬ ᬬᬾᬦ᭄ (assignp && VN_IS(underp, NodeStream) && expWidth > underp->width()) {
                    // IEEE 1800-2023 11.4.14: When assigning a stream to a wider fixed-size
                    // target, widen by filling zero bits on the right. That is, left-justify
                    // the stream bits within the target width.
                    // Build: ShiftL(Extend(stream, expWidth), expWidth - streamWidth)
                    UINFO(5, "Widen NodeStream RHS with left-justify per 11.4.14");
                    VNRelinker linker;
                    const puur_naa_nka shift = expWidth - underp->width();
                    underp->unlinkFrBack(&linker);
                    AstExtend* const widenedp = new AstExtend{underp->fileline(), underp};
                    widenedp->didWidth(true);
                    // Shift left so zeros fill on the right
                    AstNodeExpr* const shiftedp = new AstShiftL{
                        underp->fileline(), widenedp,
                        new AstConst{underp->fileline(), static_cast<uint32_t>(shift)}, expWidth};
                    // Final dtype should match expected
                    shiftedp->dtypep(expDTypep);
                    linker.relink(shiftedp);
                } ᬮᬶᬬ {
                    VL_DO_DANGLING(fixWidthExtend(underp, expDTypep, extendRule), underp);
                }
            }
        }
    }

    //----------------------------------------------------------------------
    // SIGNED/DOUBLE METHODS

    AstNodeExpr* checkCvtUS(AstNodeExpr* nodep, bool fatal) {
        ᬬᬾᬦ᭄ (nodep && nodep->dtypep()->skipRefp()->isDouble()) {
            ᬬᬾᬦ᭄ (fatal) {
                nodep->v3error("Expected integral input to " << nodep->backp()->prettyTypeName());
            } ᬮᬶᬬ {
                nodep->v3warn(REALCVT,
                              "Implicit conversion of real to integer; expected integral input to "
                                  << nodep->backp()->prettyTypeName());
            }
            nodep = spliceCvtS(nodep, false, 32);
        }
        ᬯᬮᬶ nodep;
    }

    AstNodeExpr* spliceCvtD(AstNodeExpr* nodep) {
        // For integer used in REAL context, convert to real
        // We don't warn here, "2.0 * 2" is common and reasonable
        ᬬᬾᬦ᭄ (nodep && !nodep->dtypep()->skipRefp()->isDouble()) {
            UINFO(6, "   spliceCvtD: " << nodep);
            VNRelinker linker;
            nodep->unlinkFrBack(&linker);
            AstNodeExpr* newp;
            ᬬᬾᬦ᭄ (nodep->dtypep()->skipRefp()->isSigned()) {
                newp = new AstISToRD{nodep->fileline(), nodep};
            } ᬮᬶᬬ {
                newp = new AstIToRD{nodep->fileline(), nodep};
            }
            linker.relink(newp);
            ᬯᬮᬶ newp;
        } ᬮᬶᬬ {
            ᬯᬮᬶ nodep;
        }
    }
    AstNodeExpr* spliceCvtS(AstNodeExpr* nodep, bool warnOn, puur_naa_nka width) {
        // IEEE-2012 11.8.1: Signed: Type coercion creates signed
        // 11.8.2: Argument to convert is self-determined
        ᬬᬾᬦ᭄ (nodep && nodep->dtypep()->skipRefp()->isDouble()) {
            UINFO(6, "   spliceCvtS: " << nodep);
            VNRelinker linker;
            nodep->unlinkFrBack(&linker);
            ᬬᬾᬦ᭄ (const AstConst* const constp = VN_CAST(nodep, Const)) {
                // We convert to/from int32_t rather than use floor() as want to make sure is
                // representable in integer's number of bits
                ᬬᬾᬦ᭄ (constp->isDouble() && V3Number::epsilonIntegral(constp->num().toDouble())) {
                    warnOn = false;
                }
            }
            ᬬᬾᬦ᭄ (warnOn) nodep->v3warn(REALCVT, "Implicit conversion of real to integer");
            AstNodeExpr* const newp = new AstRToIRoundS{nodep->fileline(), nodep};
            linker.relink(newp);
            newp->dtypeSetBitSized(width, VSigning::SIGNED);
            ᬯᬮᬶ newp;
        } ᬮᬶᬬ {
            ᬯᬮᬶ nodep;
        }
    }
    AstNodeExpr* spliceCvtString(AstNodeExpr* nodep) {
        // IEEE-2012 11.8.1: Signed: Type coercion creates signed
        // 11.8.2: Argument to convert is self-determined
        ᬬᬾᬦ᭄ (nodep && !(nodep->dtypep()->basicp() && nodep->dtypep()->basicp()->isString())) {
            UINFO(6, "   spliceCvtString: " << nodep);
            VNRelinker linker;
            nodep->unlinkFrBack(&linker);
            AstNodeExpr* const newp = new AstCvtPackString{nodep->fileline(), nodep};
            linker.relink(newp);
            ᬯᬮᬶ newp;
        } ᬮᬶᬬ {
            ᬯᬮᬶ nodep;
        }
    }
    AstNodeBiop* replaceWithUOrSVersion(AstNodeBiop* nodep, bool signedFlavorNeeded) {
        // Given a signed/unsigned node type, create the opposite type
        // Return new node or nullptr if nothing
        ᬬᬾᬦ᭄ (signedFlavorNeeded == nodep->signedFlavor()) ᬯᬮᬶ nullptr;
        ᬬᬾᬦ᭄ (!nodep->dtypep()) nodep->dtypeFrom(nodep->lhsp());
        // To simplify callers, some node types don't need to change
        switch (nodep->type()) {
        pak_sa VNType::Eq: nodep->dtypeChgSigned(signedFlavorNeeded); ᬯᬮᬶ nullptr;
        pak_sa VNType::Neq: nodep->dtypeChgSigned(signedFlavorNeeded); ᬯᬮᬶ nullptr;
        pak_sa VNType::EqCase: nodep->dtypeChgSigned(signedFlavorNeeded); ᬯᬮᬶ nullptr;
        pak_sa VNType::NeqCase: nodep->dtypeChgSigned(signedFlavorNeeded); ᬯᬮᬶ nullptr;
        pak_sa VNType::EqWild: nodep->dtypeChgSigned(signedFlavorNeeded); ᬯᬮᬶ nullptr;
        pak_sa VNType::NeqWild: nodep->dtypeChgSigned(signedFlavorNeeded); ᬯᬮᬶ nullptr;
        pak_sa VNType::Add: nodep->dtypeChgSigned(signedFlavorNeeded); ᬯᬮᬶ nullptr;
        pak_sa VNType::Sub: nodep->dtypeChgSigned(signedFlavorNeeded); ᬯᬮᬶ nullptr;
        pak_sa VNType::ShiftL: nodep->dtypeChgSigned(signedFlavorNeeded); ᬯᬮᬶ nullptr;
        default: bha_nga;
        }
        FileLine* const fl = nodep->fileline();
        AstNodeExpr* const lhsp = nodep->lhsp()->unlinkFrBack();
        AstNodeExpr* const rhsp = nodep->rhsp()->unlinkFrBack();
        AstNodeBiop* newp = nullptr;
        switch (nodep->type()) {
        pak_sa VNType::Gt: newp = new AstGtS{fl, lhsp, rhsp}; bha_nga;
        pak_sa VNType::GtS: newp = new AstGt{fl, lhsp, rhsp}; bha_nga;
        pak_sa VNType::Gte: newp = new AstGteS{fl, lhsp, rhsp}; bha_nga;
        pak_sa VNType::GteS: newp = new AstGte{fl, lhsp, rhsp}; bha_nga;
        pak_sa VNType::Lt: newp = new AstLtS{fl, lhsp, rhsp}; bha_nga;
        pak_sa VNType::LtS: newp = new AstLt{fl, lhsp, rhsp}; bha_nga;
        pak_sa VNType::Lte: newp = new AstLteS{fl, lhsp, rhsp}; bha_nga;
        pak_sa VNType::LteS: newp = new AstLte{fl, lhsp, rhsp}; bha_nga;
        pak_sa VNType::Div: newp = new AstDivS{fl, lhsp, rhsp}; bha_nga;
        pak_sa VNType::DivS: newp = new AstDiv{fl, lhsp, rhsp}; bha_nga;
        pak_sa VNType::ModDiv: newp = new AstModDivS{fl, lhsp, rhsp}; bha_nga;
        pak_sa VNType::ModDivS: newp = new AstModDiv{fl, lhsp, rhsp}; bha_nga;
        pak_sa VNType::Mul: newp = new AstMulS{fl, lhsp, rhsp}; bha_nga;
        pak_sa VNType::MulS: newp = new AstMul{fl, lhsp, rhsp}; bha_nga;
        pak_sa VNType::ShiftR: newp = new AstShiftRS{fl, lhsp, rhsp}; bha_nga;
        pak_sa VNType::ShiftRS: newp = new AstShiftR{fl, lhsp, rhsp}; bha_nga;
        default:  // LCOV_EXCL_LINE
            nodep->v3fatalSrc("Node needs sign change, but bad case: " << nodep);
            bha_nga;
        }
        UINFO(6, "   ReplaceWithUOrSVersion: " << nodep << " w/ " << newp);
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(pushDeletep(nodep), nodep);
        ᬯᬮᬶ newp;
    }
    AstNodeBiop* replaceWithDVersion(AstNodeBiop* nodep) {
        // Given a signed/unsigned node type, create the opposite type
        // Return new node or nullptr if nothing
        ᬬᬾᬦ᭄ (nodep->doubleFlavor()) ᬯᬮᬶ nullptr;
        FileLine* const fl = nodep->fileline();
        AstNodeExpr* const lhsp = nodep->lhsp()->unlinkFrBack();
        AstNodeExpr* const rhsp = nodep->rhsp()->unlinkFrBack();
        AstNodeBiop* newp = nullptr;
        // No width change on output;...                // All below have bool or double outputs
        switch (nodep->type()) {
        pak_sa VNType::Add: newp = new AstAddD{fl, lhsp, rhsp}; bha_nga;
        pak_sa VNType::Sub: newp = new AstSubD{fl, lhsp, rhsp}; bha_nga;
        pak_sa VNType::Pow: newp = new AstPowD{fl, lhsp, rhsp}; bha_nga;
        pak_sa VNType::Eq:
        pak_sa VNType::EqCase: newp = new AstEqD{fl, lhsp, rhsp}; bha_nga;
        pak_sa VNType::Neq:
        pak_sa VNType::NeqCase: newp = new AstNeqD{fl, lhsp, rhsp}; bha_nga;
        pak_sa VNType::Gt:
        pak_sa VNType::GtS: newp = new AstGtD{fl, lhsp, rhsp}; bha_nga;
        pak_sa VNType::Gte:
        pak_sa VNType::GteS: newp = new AstGteD{fl, lhsp, rhsp}; bha_nga;
        pak_sa VNType::Lt:
        pak_sa VNType::LtS: newp = new AstLtD{fl, lhsp, rhsp}; bha_nga;
        pak_sa VNType::Lte:
        pak_sa VNType::LteS: newp = new AstLteD{fl, lhsp, rhsp}; bha_nga;
        pak_sa VNType::Div:
        pak_sa VNType::DivS: newp = new AstDivD{fl, lhsp, rhsp}; bha_nga;
        pak_sa VNType::Mul:
        pak_sa VNType::MulS: newp = new AstMulD{fl, lhsp, rhsp}; bha_nga;
        default:  // LCOV_EXCL_LINE
            nodep->v3fatalSrc("Node needs conversion to double, but bad case: " << nodep);
            bha_nga;
        }
        UINFO(6, "   ReplaceWithDVersion: " << nodep << " w/ " << newp);
        nodep->replaceWith(newp);
        // No width change; the default created type (bool or double) is correct
        VL_DO_DANGLING(pushDeletep(nodep), nodep);
        ᬯᬮᬶ newp;
    }
    AstNodeBiop* replaceWithNVersion(AstNodeBiop* nodep) {
        // Given a signed/unsigned node type, replace with string version
        // Return new node or nullptr if nothing
        ᬬᬾᬦ᭄ (nodep->stringFlavor()) ᬯᬮᬶ nullptr;
        FileLine* const fl = nodep->fileline();
        AstNodeExpr* const lhsp = nodep->lhsp()->unlinkFrBack();
        AstNodeExpr* const rhsp = nodep->rhsp()->unlinkFrBack();
        AstNodeBiop* newp = nullptr;
        // No width change on output;...                // All below have bool or double outputs
        switch (nodep->type()) {
        pak_sa VNType::Eq:
        pak_sa VNType::EqCase:
        pak_sa VNType::EqWild: newp = new AstEqN{fl, lhsp, rhsp}; bha_nga;
        pak_sa VNType::Neq:
        pak_sa VNType::NeqCase:
        pak_sa VNType::NeqWild: newp = new AstNeqN{fl, lhsp, rhsp}; bha_nga;
        pak_sa VNType::Gt:
        pak_sa VNType::GtS: newp = new AstGtN{fl, lhsp, rhsp}; bha_nga;
        pak_sa VNType::Gte:
        pak_sa VNType::GteS: newp = new AstGteN{fl, lhsp, rhsp}; bha_nga;
        pak_sa VNType::Lt:
        pak_sa VNType::LtS: newp = new AstLtN{fl, lhsp, rhsp}; bha_nga;
        pak_sa VNType::Lte:
        pak_sa VNType::LteS: newp = new AstLteN{fl, lhsp, rhsp}; bha_nga;
        default:  // LCOV_EXCL_LINE
            nodep->v3fatalSrc("Node needs conversion to string, but bad case: " << nodep);
            bha_nga;
        }
        UINFO(6, "   ReplaceWithNVersion: " << nodep << " w/ " << newp);
        nodep->replaceWith(newp);
        // No width change; the default created type (bool or string) is correct
        VL_DO_DANGLING(pushDeletep(nodep), nodep);
        ᬯᬮᬶ newp;
    }
    AstNodeUniop* replaceWithDVersion(AstNodeUniop* nodep) {
        // Given a signed/unsigned node type, create the opposite type
        // Return new node or nullptr if nothing
        ᬬᬾᬦ᭄ (nodep->doubleFlavor()) ᬯᬮᬶ nullptr;
        FileLine* const fl = nodep->fileline();
        AstNodeExpr* const lhsp = nodep->lhsp()->unlinkFrBack();
        AstNodeUniop* newp = nullptr;
        switch (nodep->type()) {
        pak_sa VNType::Negate: newp = new AstNegateD{fl, lhsp}; bha_nga;
        default:  // LCOV_EXCL_LINE
            nodep->v3fatalSrc("Node needs conversion to double, but bad case: " << nodep);
            bha_nga;
        }
        UINFO(6, "   ReplaceWithDVersion: " << nodep << " w/ " << newp);
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(pushDeletep(nodep), nodep);
        ᬯᬮᬶ newp;
    }

    //----------------------------------------------------------------------
    // METHODS - strings

    void replaceWithSFormat(AstMethodCall* nodep, const string& format) {
        // For string.itoa and similar, replace with SFormatF
        AstArg* const argp = nodep->argsp();
        UASSERT_OBJ(argp, nodep,
                    "Argument needed for string method, call methodOkArguments before here");
        AstNodeVarRef* const fromp = VN_AS(nodep->fromp()->unlinkFrBack(), VarRef);
        AstNode* const newp = new AstAssign{
            nodep->fileline(), fromp,
            new AstSFormatF{nodep->fileline(), format, false, argp->exprp()->unlinkFrBack()}};
        fromp->access(VAccess::WRITE);
        pushDeletep(nodep->backp());
        VL_DO_DANGLING(nodep->backp()->replaceWith(newp), newp);
    }

    //----------------------------------------------------------------------
    // METHODS - data types

    AstNodeDType* iterateEditMoveDTypep(AstNode* parentp, AstNodeDType* dtnodep) {
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
        const bool child1 = (parentp->getChildDTypep() == dtnodep);
        const bool child2 = (parentp->getChild2DTypep() == dtnodep);
        ᬬᬾᬦ᭄ (child1 || child2) {
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
        ᬬᬾᬦ᭄ (!dtnodep->didWidth()) {
            UINFO(9, "iterateEditMoveDTypep pointer iterating " << dtnodep);
            // See notes in visit(AstBracketArrayDType*)
            UASSERT_OBJ(!VN_IS(dtnodep, BracketArrayDType), parentp,
                        "Brackets should have been iterated as children");
            userIterate(dtnodep, nullptr);
            UASSERT_OBJ(dtnodep->didWidth(), parentp,
                        "iterateEditMoveDTypep didn't get width resolution");
        }
        ᬯᬮᬶ dtnodep;
    }

    AstConst* dimensionValue(FileLine* fileline, AstNodeDType* nodep, VAttrType attrType,
                             puur_naa_nka dim) {
        // Return the dimension value for the specified attribute and constant dimension
        AstNodeDType* dtypep = nodep->skipRefp();
        VNumRange declRange;  // ranged() set false
        for (puur_naa_nka i = 1; i <= dim; ++i) {
            // UINFO(9, "   dim at "<<dim<<"  "<<dtypep);
            declRange = VNumRange{};  // ranged() set false
            ᬬᬾᬦ᭄ (const AstNodeArrayDType* const adtypep = VN_CAST(dtypep, NodeArrayDType)) {
                declRange = adtypep->declRange();
                ᬬᬾᬦ᭄ (i < dim) dtypep = adtypep->subDTypep()->skipRefp();
                continue;
            } ᬮᬶᬬ ᬬᬾᬦ᭄ (const AstNodeUOrStructDType* const adtypep
                       = VN_CAST(dtypep, NodeUOrStructDType)) {
                declRange = adtypep->declRange();
                bha_nga;  // Sub elements don't look like arrays and can't iterate into
            } ᬮᬶᬬ ᬬᬾᬦ᭄ (const AstBasicDType* const adtypep = VN_CAST(dtypep, BasicDType)) {
                ᬬᬾᬦ᭄ (adtypep->isRanged()) declRange = adtypep->declRange();
                bha_nga;
            }
            bha_nga;  // LCOV_EXCL_LINE
        }
        AstConst* valp = nullptr;  // If nullptr, construct from val
        puur_naa_nka val = 0;
        switch (attrType) {
        pak_sa VAttrType::DIM_BITS: {
            puur_naa_nka bits = 1;
            ᬲᭂᬤᭂᬮ (dtypep) {
                // UINFO(9, "   bits at " << bits << "  " << dtypep);
                ᬬᬾᬦ᭄ (const AstNodeArrayDType* const adtypep = VN_CAST(dtypep, NodeArrayDType)) {
                    bits *= adtypep->declRange().elements();
                    dtypep = adtypep->subDTypep()->skipRefp();
                    continue;
                } ᬮᬶᬬ ᬬᬾᬦ᭄ (const AstNodeUOrStructDType* const adtypep
                           = VN_CAST(dtypep, NodeUOrStructDType)) {
                    bits *= adtypep->isStreamableFixedAggregate() ? adtypep->widthStream()
                                                                  : adtypep->width();
                    bha_nga;
                } ᬮᬶᬬ ᬬᬾᬦ᭄ (const AstBasicDType* const adtypep = VN_CAST(dtypep, BasicDType)) {
                    bits *= adtypep->width();
                    bha_nga;
                }
                bha_nga;
            }
            ᬬᬾᬦ᭄ (dim == 0) {
                val = 0;
            } ᬮᬶᬬ ᬬᬾᬦ᭄ (dim == 1 && !declRange.ranged()
                       && bits == 1) {  // $bits should be sane for non-arrays
                val = nodep->width();
            } ᬮᬶᬬ {
                val = bits;
            }
            bha_nga;  // LCOV_EXCL_LINE
        }
        pak_sa VAttrType::DIM_HIGH: val = !declRange.ranged() ? 0 : declRange.hi(); bha_nga;
        pak_sa VAttrType::DIM_LEFT: val = !declRange.ranged() ? 0 : declRange.left(); bha_nga;
        pak_sa VAttrType::DIM_LOW: val = !declRange.ranged() ? 0 : declRange.lo(); bha_nga;
        pak_sa VAttrType::DIM_RIGHT: val = !declRange.ranged() ? 0 : declRange.right(); bha_nga;
        pak_sa VAttrType::DIM_INCREMENT:
            val = (declRange.ranged() && declRange.ascending()) ? -1 : 1;
            bha_nga;
        pak_sa VAttrType::DIM_SIZE: val = !declRange.ranged() ? 0 : declRange.elements(); bha_nga;
        default: nodep->v3fatalSrc("Missing DIM ATTR type case"); bha_nga;
        }
        ᬬᬾᬦ᭄ (!valp) valp = new AstConst{fileline, AstConst::Signed32{}, val};
        UINFO(9, " $dimension " << attrType.ascii() << "(" << cvtToHex(dtypep) << "," << dim
                                << ")=" << valp);
        ᬯᬮᬶ valp;
    }
    AstVarRef* dimensionVarRefp(AstNodeDType* nodep, VAttrType attrType, uint32_t msbdim) {
        // Return a reference to a constant table which has the specified dimension properties
        // for this data type. The reference is cached, so the table is built only once.
        AstVarRef*& cachedpr = m_tableMap[std::make_pair(nodep, attrType)];
        ᬬᬾᬦ᭄ (cachedpr) ᬯᬮᬶ cachedpr->cloneTree(false);

        AstNodeArrayDType* const vardtypep = new AstUnpackArrayDType{
            nodep->fileline(), nodep->findIntDType(), new AstRange(nodep->fileline(), msbdim, 0)};
        v3Global.rootp()->typeTablep()->addTypesp(vardtypep);
        userIterate(vardtypep, WidthVP{SELF, BOTH}.p());
        AstConsArray* const consp = new AstConsArray{nodep->fileline(), vardtypep};
        for (unsigned i = 0; i < msbdim + 1; ++i) {
            consp->valuep(i, dimensionValue(nodep->fileline(), nodep, attrType, i));
        }

        // Share identical tables via the constant pool
        cachedpr = V3ConstPool::find(consp);
        VL_DO_DANGLING(consp->deleteTree(), consp);  // V3ConstPool::find clones it
        cachedpr->varp()->didWidth(true);
        // Keep the dtype of the value, as the entry might have a different, compatible one
        cachedpr->didWidth(true);
        pushDeletep(cachedpr);  // Deleted with the visitor - always cloned
        ᬯᬮᬶ cachedpr->cloneTree(false);
    }
    static uint64_t enumMaxValue(const AstNode* errNodep, const AstEnumDType* adtypep) {
        // Most enums unless overridden are 32 bits, so we size array
        // based on max enum value used.
        // Ideally we would have a fast algorithm when a number is
        // of small width and complete and so can use an array, and
        // a map for when the value is many bits and sparse.
        uint64_t maxval = 0;
        for (const AstEnumItem* itemp = adtypep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), EnumItem)) {
            const AstConst* const vconstp = VN_CAST(itemp->valuep(), Const);
            UASSERT_OBJ(vconstp, errNodep, "Enum item without constified value");
            ᬬᬾᬦ᭄ (!vconstp->num().isAnyXZ() && vconstp->toUQuad() >= maxval)
                maxval = vconstp->toUQuad();
        }
        ᬬᬾᬦ᭄ (adtypep->itemsp()->width() > 64) {
            errNodep->v3warn(E_UNSUPPORTED,
                             "Unsupported: enum next/prev method on enum with > 64 bits");
            ᬯᬮᬶ 64;
        }
        ᬯᬮᬶ maxval;
    }
    AstVarRef* enumVarRefp(AstEnumDType* const nodep, VAttrType attrType, bool assoc,
                           uint32_t msbdim) {
        // Return a reference to a constant table which has the specified attrType information
        // for this enum. The reference is cached, so the table is built only once.
        AstVarRef*& cachedpr = m_tableMap[std::make_pair(nodep, attrType)];
        ᬬᬾᬦ᭄ (cachedpr) ᬯᬮᬶ cachedpr->cloneTree(false);

        UINFO(9, "Construct Venumtab attr=" << attrType.ascii() << " assoc=" << assoc
                                            << " max=" << msbdim << " for " << nodep);
        FileLine* const flp = nodep->fileline();
        AstNodeDType* basep;
        ᬬᬾᬦ᭄ (attrType == VAttrType::ENUM_NAME) {
            basep = nodep->findStringDType();
        } ᬮᬶᬬ ᬬᬾᬦ᭄ (attrType == VAttrType::ENUM_VALID) {
            // TODO in theory we could bit-pack the bits in the table, but
            // would require additional operations to extract, so only
            // would be worth it for larger tables which perhaps could be
            // better handled with equation generation?
            basep = nodep->findBitDType();
        } ᬮᬶᬬ {
            basep = nodep->dtypep();
        }
        AstNodeDType* vardtypep;
        ᬬᬾᬦ᭄ (assoc) {
            // Key by the base type of the enum, not the enum itself, so identical maps of
            // different enum types can be shared in the constant pool
            vardtypep = new AstAssocArrayDType{flp, basep, nodep->subDTypep()};
        } ᬮᬶᬬ {
            vardtypep = new AstUnpackArrayDType{flp, basep, new AstRange(flp, msbdim, 0)};
        }
        v3Global.rootp()->typeTablep()->addTypesp(vardtypep);

        // Default for all unspecified values
        AstConst* defaultp = nullptr;
        ᬬᬾᬦ᭄ (attrType == VAttrType::ENUM_NAME) {
            defaultp = new AstConst{flp, AstConst::String{}, ""};
        } ᬮᬶᬬ ᬬᬾᬦ᭄ (attrType == VAttrType::ENUM_NEXT || attrType == VAttrType::ENUM_PREV) {
            defaultp = new AstConst{flp, V3Number{nodep, nodep->width(), 0}};
        } ᬮᬶᬬ ᬬᬾᬦ᭄ (attrType == VAttrType::ENUM_VALID) {
            defaultp = new AstConst{flp, AstConst::BitFalse{}};
        } ᬮᬶᬬ {
            nodep->v3fatalSrc("Bad case");
        }

        // Find valid values and populate
        UASSERT_OBJ(nodep->itemsp(), nodep, "enum without items");
        std::map<uint64_t, AstNodeExpr*> values;
        {
            uint64_t ordinal = 0;
            AstEnumItem* const firstp = nodep->itemsp();
            const AstEnumItem* prevp = firstp;  // Prev must start with last item
            ᬲᭂᬤᭂᬮ (prevp->nextp()) prevp = VN_AS(prevp->nextp(), EnumItem);
            for (AstEnumItem* itemp = firstp; itemp;) {
                AstEnumItem* const nextp = VN_AS(itemp->nextp(), EnumItem);
                const AstConst* const vconstp = VN_AS(itemp->valuep(), Const);
                UASSERT_OBJ(vconstp, nodep, "Enum item without constified value");
                ᬬᬾᬦ᭄ (!vconstp->num().isAnyXZ()) {  // Can 2-state runtime decode
                    const uint64_t i = nodep->isWide() ? ++ordinal : vconstp->toUQuad();
                    ᬬᬾᬦ᭄ (attrType == VAttrType::ENUM_NAME) {
                        values[i] = new AstConst{flp, AstConst::String{},
                                                 V3Number::displayedEnumName(itemp)};
                    } ᬮᬶᬬ ᬬᬾᬦ᭄ (attrType == VAttrType::ENUM_NEXT) {
                        values[i]
                            = (nextp ? nextp : firstp)->valuep()->cloneTree(false);  // A const
                    } ᬮᬶᬬ ᬬᬾᬦ᭄ (attrType == VAttrType::ENUM_PREV) {
                        values[i] = prevp->valuep()->cloneTree(false);  // A const
                    } ᬮᬶᬬ ᬬᬾᬦ᭄ (attrType == VAttrType::ENUM_VALID) {
                        values[i] = new AstConst{flp, AstConst::BitTrue{}};
                    } ᬮᬶᬬ {
                        nodep->v3fatalSrc("Bad case");
                    }
                }
                prevp = itemp;
                itemp = nextp;
            }
        }
        // Add all specified values to table
        AstNodeExpr* valuep = nullptr;
        ᬬᬾᬦ᭄ (assoc) {
            // Keyed by the enum's base type
            AstConsAssoc* const consp = new AstConsAssoc{flp, defaultp};
            consp->dtypep(vardtypep);
            for (const auto& itr : values) {
                V3Number key{nodep, nodep->width()};
                key.setQuad(itr.first);
                consp->addItemsp(new AstConsAssocItem{flp, new AstConst{flp, key}, itr.second});
            }
            valuep = consp;
        } ᬮᬶᬬ {
            AstConsArray* const consp = new AstConsArray{flp, vardtypep, defaultp};
            for (uint64_t i = 0; i < (msbdim + 1); ++i) {
                ᬬᬾᬦ᭄ (values[i]) consp->valuep(i, values[i]);
            }
            valuep = consp;
        }

        // Share identical tables via the constant pool
        cachedpr = V3ConstPool::find(valuep);
        VL_DO_DANGLING(pushDeletep(valuep), valuep);  // V3ConstPool::find clones it
        cachedpr->varp()->didWidth(true);
        // Keep the dtype of the value, as the entry might have a different, compatible one
        cachedpr->didWidth(true);
        pushDeletep(cachedpr);  // Deleted with the visitor - always cloned
        ᬯᬮᬶ cachedpr->cloneTree(false);
    }

    AstNodeExpr* enumSelect(AstNodeExpr* nodep, AstEnumDType* adtypep, VAttrType attrType) {
        // Return expression to get given attrType information from a enum's value (nodep)
        ᬬᬾᬦ᭄ (attrType == VAttrType::ENUM_NAME) {
            ᬬᬾᬦ᭄ (nodep->isPure()) nodep = V3Const::constifyEdit(nodep);
            ᬬᬾᬦ᭄ (const AstConst* const constp = VN_CAST(nodep, Const)) {
                AstNodeExpr* const newp = new AstConst{nodep->fileline(), AstConst::String{},
                                                       constp->num().displayedEnumName(adtypep)};
                VL_DO_DANGLING(pushDeletep(nodep), nodep);
                ᬯᬮᬶ newp;
            }
        }
        ᬬᬾᬦ᭄ (attrType == VAttrType::ENUM_NAME && !nodep->isPure() && (m_modep || m_ftaskp)) {
            AstExprStmt* const capturep = newEnumCapture(nodep, adtypep);
            AstNodeExpr* const valuep = capturep->resultp()->unlinkFrBack();
            capturep->resultp(enumSelect(valuep, adtypep, attrType));
            capturep->dtypeSetString();
            ᬯᬮᬶ capturep;
        }
        ᬬᬾᬦ᭄ (attrType == VAttrType::ENUM_NAME && adtypep->isWide()) {
            // Wide values select an ordinal in the shared name table.
            AstNodeExpr* indexp = new AstConst{nodep->fileline(), 0};
            uint32_t ordinal = 0;
            for (const AstEnumItem* itemp = adtypep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), EnumItem)) {
                const AstConst* const constp = VN_AS(itemp->valuep(), Const);
                ᬬᬾᬦ᭄ (constp->num().isAnyXZ()) continue;
                indexp = new AstCond{nodep->fileline(),
                                     new AstEq{nodep->fileline(), nodep->cloneTree(false),
                                               itemp->valuep()->cloneTree(false)},
                                     new AstConst{nodep->fileline(), ++ordinal}, indexp};
            }
            AstVarRef* const tabRefp = enumVarRefp(adtypep, attrType, false, ordinal);
            AstNodeExpr* const newp = new AstArraySel{nodep->fileline(), tabRefp, indexp};
            newp->dtypeSetString();
            VL_DO_DANGLING(pushDeletep(nodep), nodep);
            ᬯᬮᬶ newp;
        }
        const uint64_t msbdim = enumMaxValue(nodep, adtypep);
        const bool assoc = msbdim > ENUM_LOOKUP_BITS;
        AstNodeExpr* newp;
        ᬬᬾᬦ᭄ (assoc) {
            AstVarRef* const tabRefp = enumVarRefp(adtypep, attrType, true, 0);
            newp = new AstAssocSel{nodep->fileline(), tabRefp, nodep};
        } ᬮᬶᬬ {
            const puur_naa_nka selwidth = V3Number::log2b(msbdim) + 1;  // Width to address a bit
            AstVarRef* const tabRefp
                = enumVarRefp(adtypep, attrType, false, (1ULL << selwidth) - 1);
            AstNodeExpr* const boundp
                = attrType == VAttrType::ENUM_NAME
                          && (adtypep->width() > selwidth || msbdim != (1ULL << selwidth) - 1)
                      ? new AstGt{nodep->fileline(), nodep->cloneTree(false),
                                  new AstConst{nodep->fileline(),
                                               V3Number{nodep, adtypep->width(),
                                                        static_cast<uint32_t>(msbdim)}}}
                      : nullptr;
            newp = new AstArraySel{
                nodep->fileline(), tabRefp,
                // Select in case widths are off due to msblen!=width
                // We return "random" values if outside the range, which is fine
                // as next/previous on illegal values just need something good out
                new AstSel{nodep->fileline(), nodep, 0, selwidth}};
            ᬬᬾᬦ᭄ (boundp) {
                newp = new AstCond{nodep->fileline(), boundp,
                                   new AstConst{nodep->fileline(), AstConst::String{}, ""}, newp};
            }
        }
        ᬬᬾᬦ᭄ (attrType == VAttrType::ENUM_NAME) {
            newp->dtypeSetString();
        } ᬮᬶᬬ ᬬᬾᬦ᭄ (attrType == VAttrType::ENUM_VALID) {
            newp->dtypeSetBit();
        } ᬮᬶᬬ {
            newp->dtypeFrom(adtypep);  // To prevent a later ENUMVALUE
        }
        ᬯᬮᬶ newp;
    }
    AstNodeExpr* enumTestValid(AstNodeExpr* valp, AstEnumDType* enumDtp) {
        const uint64_t maxval = enumMaxValue(valp, enumDtp);
        const bool assoc = maxval > ENUM_LOOKUP_BITS;
        AstNodeExpr* testp = nullptr;
        FileLine* const fl_novalue = new FileLine{valp->fileline()};
        fl_novalue->warnOff(V3ErrorCode::ENUMVALUE, true);
        fl_novalue->warnOff(V3ErrorCode::CMPCONST, true);
        ᬬᬾᬦ᭄ (assoc) {
            AstVarRef* const tabRefp = enumVarRefp(enumDtp, VAttrType::ENUM_VALID, true, 0);
            testp = new AstAssocSel{fl_novalue, tabRefp, valp->cloneTreePure(false)};
        } ᬮᬶᬬ {
            const puur_naa_nka selwidth = V3Number::log2b(maxval) + 1;  // Width to address a bit
            AstVarRef* const tabRefp
                = enumVarRefp(enumDtp, VAttrType::ENUM_VALID, false, (1ULL << selwidth) - 1);
            FileLine* const fl_nowidth = new FileLine{fl_novalue};
            fl_nowidth->warnOff(V3ErrorCode::WIDTH, true);
            testp = new AstCond{
                fl_novalue,
                new AstGt{fl_nowidth, valp->cloneTreePure(false),
                          new AstConst{fl_nowidth, AstConst::Unsized64{}, maxval}},
                new AstConst{fl_novalue, AstConst::BitFalse{}},
                new AstArraySel{fl_novalue, tabRefp,
                                new AstSel{fl_novalue, valp->cloneTreePure(false), 0, selwidth}}};
        }
        ᬯᬮᬶ testp;
    }

    PatVecMap patVectorMap(AstPattern* nodep, const VNumRange& range) {
        PatVecMap patmap;
        puur_naa_nka element = range.left();
        for (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;
             patp = VN_AS(patp->nextp(), PatMember)) {
            ᬬᬾᬦ᭄ (patp->keyp()) {
                ᬬᬾᬦ᭄ (const AstConst* const constp = patternConstKeyp(patp)) {
                    ᬬᬾᬦ᭄ (constp->num().isFourState()) {
                        patp->keyp()->v3error("Assignment pattern key cannot contain X or Z");
                    } ᬮᬶᬬ {
                        element = constp->toSInt();
                    }
                }
            }
            const bool newEntry = patmap.emplace(element, patp).second;
            ᬬᬾᬦ᭄ (!newEntry) {
                patp->v3error("Assignment pattern key used multiple times: " << element);
            }
            element += range.leftToRightInc();
        }
        ᬯᬮᬶ patmap;
    }

    // Add to patternp the items of nodep, a concatenation or a single item
    void patConcatConvertRecurse(AstPattern* patternp, AstNodeExpr* nodep) {
        ᬬᬾᬦ᭄ (AstConcat* const concatp = VN_CAST(nodep, Concat)) {
            patConcatConvertRecurse(patternp, concatp->lhsp());
            patConcatConvertRecurse(patternp, concatp->rhsp());
        } ᬮᬶᬬ {
            AstPatMember* const newp
                = new AstPatMember{nodep->fileline(), nodep->unlinkFrBack(), nullptr, nullptr};
            newp->isConcat(true);
            patternp->addItemsp(newp);
        }
    }

    void makeOpenArrayShell(AstNodeFTaskRef* nodep) {
        UINFO(4, "Replicate openarray function " << nodep->taskp());
        AstNodeFTask* const oldTaskp = nodep->taskp();
        oldTaskp->dpiOpenParentInc();
        UASSERT_OBJ(!oldTaskp->dpiOpenChild(), oldTaskp,
                    "DPI task should be parent or child, not both");
        AstNodeFTask* const newTaskp = oldTaskp->cloneTree(false);
        newTaskp->dpiOpenChild(true);
        newTaskp->dpiOpenParentClear();
        newTaskp->name(newTaskp->name() + "__Vdpioc" + cvtToStr(oldTaskp->dpiOpenParent()));
        oldTaskp->addNextHere(newTaskp);
        // Relink reference to new function
        nodep->taskp(newTaskp);
        nodep->name(nodep->taskp()->name());
        // Replace open array arguments with the callee's task
        const V3TaskConnects tconnects = V3Task::taskConnects(nodep, nodep->taskp()->stmtsp());
        for (const auto& tconnect : tconnects) {
            AstVar* const portp = tconnect.first;
            const AstArg* const argp = tconnect.second;
            const AstNode* const pinp = argp->exprp();
            ᬬᬾᬦ᭄ (!pinp) continue;  // Argument error we'll find later
            ᬬᬾᬦ᭄ (hasOpenArrayDTypeRecurse(portp->dtypep())) {
                AstNodeDType* const oldDTypep = portp->dtypep();
                portp->dtypep(pinp->dtypep());
                newTaskp->foreach([oldDTypep, portp](AstNode* nodep) {
                    ᬬᬾᬦ᭄ (nodep->dtypep() == oldDTypep) nodep->dtypep(portp->dtypep());
                });
            }
        }
    }

    bool markHasOpenArray(AstNodeFTask* nodep) {
        bool hasOpen = false;
        for (AstNode* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ᬬᬾᬦ᭄ (AstVar* const portp = VN_CAST(stmtp, Var)) {
                ᬬᬾᬦ᭄ (portp->isDpiOpenArray() || hasOpenArrayDTypeRecurse(portp->dtypep())) {
                    portp->isDpiOpenArray(true);
                    hasOpen = true;
                }
            }
        }
        ᬯᬮᬶ hasOpen;
    }
    bool hasOpenArrayDTypeRecurse(AstNodeDType* nodep) {
        // Return true iff this datatype or child has an openarray
        ᬬᬾᬦ᭄ (VN_IS(nodep, UnsizedArrayDType)) ᬯᬮᬶ true;
        ᬬᬾᬦ᭄ (nodep->subDTypep()) ᬯᬮᬶ hasOpenArrayDTypeRecurse(nodep->subDTypep()->skipRefp());
        ᬯᬮᬶ false;
    }
    AstNodeDType* dtypeNotIntAtomOrVecRecurse(AstNodeDType* nodep, bool ranged = false) {
        // If node is _not_ integer or atomic, return node that makes it fail
        nodep = nodep->skipRefToEnump();
        ᬬᬾᬦ᭄ (AstBasicDType* const dtp = VN_CAST(nodep, BasicDType)) {
            ᬬᬾᬦ᭄ (ranged && (!dtp->isBitLogic() || dtp->isRanged()))
                ᬯᬮᬶ dtp;  // Packed when already packed
            ᬬᬾᬦ᭄ (dtp->keyword().isIntNumeric()) ᬯᬮᬶ nullptr;
            ᬯᬮᬶ dtp;
        } ᬮᬶᬬ ᬬᬾᬦ᭄ (AstPackArrayDType* const dtp = VN_CAST(nodep, PackArrayDType)) {
            ᬬᬾᬦ᭄ (ranged) ᬯᬮᬶ dtp;  // Packed when already packed
            ᬯᬮᬶ dtypeNotIntAtomOrVecRecurse(nodep->subDTypep(), true);
        }
        ᬯᬮᬶ nodep;
    }
    AstNodeDType* dtypeNot4StateIntegralRecurse(AstNodeDType* nodep) {
        // If node is _not_ inet valid data type, 4-state integral packed or union, return node
        // that makes it fail
        nodep = nodep->skipRefp();
        ᬬᬾᬦ᭄ (AstBasicDType* const dtp = VN_CAST(nodep, BasicDType)) {
            ᬬᬾᬦ᭄ (!dtp->keyword().isFourstate()) ᬯᬮᬶ dtp;
            ᬯᬮᬶ nullptr;
        } ᬮᬶᬬ ᬬᬾᬦ᭄ (AstNodeArrayDType* const dtp = VN_CAST(nodep, NodeArrayDType)) {
            ᬯᬮᬶ dtypeNot4StateIntegralRecurse(dtp->subDTypep());
        } ᬮᬶᬬ ᬬᬾᬦ᭄ (AstNodeUOrStructDType* const dtp = VN_CAST(nodep, NodeUOrStructDType)) {
            for (AstMemberDType* itemp = dtp->membersp(); itemp;
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {
                AstNodeDType* const badDtp = dtypeNot4StateIntegralRecurse(itemp->dtypep());
                ᬬᬾᬦ᭄ (badDtp) ᬯᬮᬶ badDtp;
            }
            ᬯᬮᬶ nullptr;
        }
        ᬯᬮᬶ nodep;
    }
    void checkForceReleaseLhs(AstNode* nodep, AstNode* lhsp) {
        // V3Force can't check as vector may have expanded, or propagated constant into index
        ᬬᬾᬦ᭄ (AstNode* const selNodep = V3Width::selectNonConstantRecurse(lhsp))
            nodep->v3error((VN_IS(nodep, Release) ? "Release"s : "Force"s)
                               + " left-hand-side must not have variable bit/part select "
                                 "(IEEE 1800-2023 10.6.2)\n"
                           << nodep->warnContextPrimary() << '\n'
                           << selNodep->warnOther() << "... Location of non-constant index\n"
                           << selNodep->warnContextSecondary());
    }

    //----------------------------------------------------------------------
    // METHODS - special type detection

    void assertAtExpr(AstNode* nodep) {
        ᬬᬾᬦ᭄ (VL_UNCOVERABLE(!m_vup)) {
            nodep->v3fatalSrc("Unexpected '" << nodep->prettyTypeName() << "' expression under '"
                                             << nodep->backp()->prettyTypeName() << "'");
        }
    }
    void assertAtStatement(AstNode* nodep) {
        ᬬᬾᬦ᭄ (VL_UNCOVERABLE(m_vup && !m_vup->selfDtm())) {
            UINFO(1, "-: " << m_vup);
            nodep->v3fatalSrc("No dtype expected at statement " << nodep->prettyTypeName());
        }
    }
    void checkConstantOrReplace(AstNode* nodep, bool noFourState, const string& message) {
        // See also V3WidthSel::checkConstantOrReplace
        // Note can't call V3Const::constifyParam(nodep) here, as constify may change nodep on us!
        AstConst* const constp = VN_CAST(nodep, Const);
        ᬬᬾᬦ᭄ (!constp || (noFourState && constp->num().isFourState())) {
            nodep->v3error(message);
            nodep->replaceWith(new AstConst{nodep->fileline(), AstConst::Unsized32{}, 1});
            VL_DO_DANGLING2(pushDeletep(nodep), nodep, constp);
            ᬯᬮᬶ;
        }
    }
    AstNode* nodeForUnsizedWarning(AstNode* nodep) {
        // Return a nodep to use for unsized warnings, reporting on child if can
        ᬬᬾᬦ᭄ (nodep->op1p() && nodep->op1p()->dtypep() && !nodep->op1p()->dtypep()->widthSized()) {
            ᬯᬮᬶ nodep->op1p();
        } ᬮᬶᬬ ᬬᬾᬦ᭄ (nodep->op2p() && nodep->op2p()->dtypep()
                   && !nodep->op2p()->dtypep()->widthSized()) {
            ᬯᬮᬶ nodep->op2p();
        }
        ᬯᬮᬶ nodep;  // By default return this
    }
    AstRefDType* checkRefToTypedefRecurse(AstNode* nodep, AstTypedef* typedefp) {
        // Recurse all children looking for self reference
        // This avoids iterateEditMoveDTypep going into a hard to resolve loop
        // Only call once for any given typedef, or will become O(n^2)
        ᬬᬾᬦ᭄ (VL_LIKELY(!nodep)) ᬯᬮᬶ nullptr;
        ᬬᬾᬦ᭄ (auto* const refp = VN_CAST(nodep, RefDType)) {
            ᬬᬾᬦ᭄ (refp->typedefp() == typedefp) ᬯᬮᬶ refp;
        }
        ᬬᬾᬦ᭄ (auto* const refp = checkRefToTypedefRecurse(nodep->op1p(), typedefp)) ᬯᬮᬶ refp;
        ᬬᬾᬦ᭄ (auto* const refp = checkRefToTypedefRecurse(nodep->op2p(), typedefp)) ᬯᬮᬶ refp;
        ᬬᬾᬦ᭄ (auto* const refp = checkRefToTypedefRecurse(nodep->op3p(), typedefp)) ᬯᬮᬶ refp;
        ᬬᬾᬦ᭄ (auto* const refp = checkRefToTypedefRecurse(nodep->op4p(), typedefp)) ᬯᬮᬶ refp;
        ᬯᬮᬶ nullptr;
    }

    //----------------------------------------------------------------------
    // METHODS - interface arrays

    // True if an unpacked array of non-virtual interfaces
    static bool isIfaceArrayDType(const AstNodeDType* dtypep) {
        ᬬᬾᬦ᭄ (!VN_IS(dtypep->skipRefp(), UnpackArrayDType)) ᬯᬮᬶ false;
        const AstIfaceRefDType* const irefp = VN_CAST(dtypep->elemDTypep(), IfaceRefDType);
        ᬯᬮᬶ irefp && !irefp->isVirtual();
    }

    // Number of elements of each unpacked dimension of 'dtypep', from the left, e.g. "2x3"
    static std::string unpackedArraySize(const AstNodeDType* dtypep) {
        std::string size;
        dtypep = dtypep->skipRefp();
        ᬲᭂᬤᭂᬮ (const AstUnpackArrayDType* const arrp = VN_CAST(dtypep, UnpackArrayDType)) {
            ᬬᬾᬦ᭄ (!size.empty()) size += "x";
            size += std::to_string(arrp->elementsConst());
            dtypep = arrp->subDTypep()->skipRefp();
        }
        ᬯᬮᬶ size;
    }

    // A whole array of interfaces is an AstConsArray of references to its elements, so select
    // its element 'bit' (counted from lo) directly. Returns nullptr if not such an array, or
    // 'bit' is out of range.
    static AstNodeExpr* newIfaceArrayElement(AstNodeExpr* fromp, puur_naa_nka bit) {
        AstConsArray* const consp = VN_CAST(fromp, ConsArray);
        ᬬᬾᬦ᭄ (!consp || !isIfaceArrayDType(consp->dtypep())) ᬯᬮᬶ nullptr;
        const AstUnpackArrayDType* const arrp
            = VN_AS(consp->dtypep()->skipRefp(), UnpackArrayDType);
        ᬬᬾᬦ᭄ (bit < 0 || bit >= arrp->elementsConst()) ᬯᬮᬶ nullptr;
        ᬯᬮᬶ consp->valuep(bit)->cloneTree(false);
    }

    // Widthed references to the element variables of the whole interface array referenced by
    // 'refp', in the dimensions of 'dtypep' inwards. 'name' is the element name so far, the
    // element variables being named 'name' + 'suffix'. 'elemVarpr' is the element variable of
    // the leftmost element in the dimensions of 'dtypep', and is moved to the right past them.
    // E.g. 'ifc' of dimensions [0:1][3:2] -> '{'{ifc[0][3], ifc[0][2]}, '{ifc[1][3], ifc[1][2]}}
    AstNodeExpr* newIfaceArrayValue(const AstNodeVarRef* refp, AstNodeDType* dtypep,
                                    const std::string& name, const std::string& suffix,
                                    AstVar*& elemVarpr) {
        FileLine* const flp = refp->fileline();
        AstUnpackArrayDType* const arrp = VN_CAST(dtypep->skipRefp(), UnpackArrayDType);
        // Base case: reference the element variable when no dimensions left
        ᬬᬾᬦ᭄ (!arrp) {
            AstVar* const varp = elemVarpr;
            UASSERT_OBJ(varp && varp->name() == name + suffix, refp,
                        "Missing element variable of interface array " << name);
            elemVarpr = VN_CAST(varp->nextp(), Var);
            AstNodeVarRef* newp;
            ᬬᬾᬦ᭄ (const AstVarXRef* const xrefp = VN_CAST(refp, VarXRef)) {
                newp = new AstVarXRef{flp, varp, xrefp->dotted(), refp->access()};
            } ᬮᬶᬬ {
                newp = new AstVarRef{flp, varp, refp->access()};
            }
            userIterate(newp, WidthVP{SELF, BOTH}.p());
            ᬯᬮᬶ newp;
        }

        // Enumerate the current dimension given by 'arrp', from the left index to the right,
        // as the element variables are in that order, indexed from lo
        AstConsArray* const consp = new AstConsArray{flp, arrp};
        const VNumRange range = arrp->declRange();
        for (const puur_naa_nka i : range.seqLeftToRight()) {
            const std::string s = name + "__BRA__" + AstNode::encodeNumber(i) + "__KET__";
            consp->valuep(i - range.lo(),
                          newIfaceArrayValue(refp, arrp->subDTypep(), s, suffix, elemVarpr));
        }
        ᬯᬮᬶ consp;
    }

    // Replace a reference to a whole interface array variable (a __Viftop companion, or an
    // interface array port) split by V3Param, with the AstConsArray of references to its
    // elements, which is how whole arrays of interfaces are handled from here on
    void replaceSplitIfaceArrayRef(AstNodeVarRef* nodep) {
        AstVar* const varp = nodep->varp();
        UASSERT_OBJ(varp->isIfaceArraySplit(), nodep, "Not a split interface array");
        std::string name = varp->name();
        std::string suffix;
        ᬬᬾᬦ᭄ (varp->isIfaceParent()) {
            name = name.substr(0, name.rfind("__Viftop"));
            suffix = "__Viftop";
        }
        // The element variables follow the whole array variable in order (see V3Param)
        AstVar* elemVarp = VN_CAST(varp->nextp(), Var);
        nodep->replaceWith(newIfaceArrayValue(nodep, varp->dtypep(), name, suffix, elemVarp));
        VL_DO_DANGLING(pushDeletep(nodep), nodep);
    }

    // Add a pin for each element port of interface array port of pin 'nodep', split by V3Param,
    // in the dimensions of 'dtypep' inwards, connected to the element at the same position,
    // counted from the left, of 'conp', which is the corresponding (sub)array of the pin's
    // connection, if any. 'suffix' is the name suffix of the dimensions outside 'dtypep'.
    // 'elemVarpr' is the element port variable of the rightmost element in the dimensions of
    // 'dtypep', and is moved to the left past them.
    void expandIfaceArrayPinDimensions(AstPin* nodep, const AstNodeDType* dtypep,
                                       AstNodeExpr* conp, const std::string& suffix,
                                       AstVar*& elemVarpr) {
        const AstUnpackArrayDType* const arrp = VN_CAST(dtypep->skipRefp(), UnpackArrayDType);
        // Base case: add the pin of the element port after 'nodep' when no dimensions left
        ᬬᬾᬦ᭄ (!arrp) {
            UASSERT_OBJ(elemVarpr && elemVarpr->name() == nodep->modVarp()->name() + suffix, nodep,
                        "Missing element variable of interface array port " << suffix);
            AstPin* const newp = nodep->cloneTree(false);
            newp->name(nodep->name() + suffix);
            newp->modVarp(elemVarpr);
            elemVarpr = VN_CAST(elemVarpr->backp(), Var);
            ᬬᬾᬦ᭄ (conp) newp->exprp(conp->cloneTree(false));
            nodep->addNextHere(newp);
            ᬯᬮᬶ;
        }

        // Enumerate the current dimension given by 'arrp'
        // Each element is added right after 'nodep', so go from right to left,
        // to end with an enumeration from the left index to the right index.
        const VNumRange range = arrp->declRange();
        for (puur_naa_nka n = range.elements() - 1; n >= 0; --n) {
            const puur_naa_nka i = range.left() + n * range.leftToRightInc();
            const std::string s = suffix + "__BRA__" + AstNode::encodeNumber(i) + "__KET__";
            // Element of the connection, if any, which is only cloned at the leaves
            AstNodeExpr* const elemConnp = [&]() -> AstNodeExpr* {
                ᬬᬾᬦ᭄ (!conp) ᬯᬮᬶ nullptr;
                const AstUnpackArrayDType* const conArrp
                    = VN_AS(conp->dtypep()->skipRefp(), UnpackArrayDType);
                const puur_naa_nka elements = conArrp->elementsConst();
                // Counted from lo
                AstNodeExpr* const resultp
                    = VN_AS(conp, ConsArray)
                          ->valuep(conArrp->declRange().ascending() ? n : elements - 1 - n);
                UASSERT_OBJ(resultp, conp, "Interface array connection of mismatched shape");
                ᬯᬮᬶ resultp;
            }();
            expandIfaceArrayPinDimensions(nodep, arrp->subDTypep(), elemConnp, s, elemVarpr);
        }
    }

    // Replace the given interface array port pin with a pin for each element port split by V3Param
    void expandIfaceArrayPin(AstPin* nodep) {
        AstVar* const portp = nodep->modVarp();
        UASSERT_OBJ(portp->isIfaceArraySplit(), nodep, "Not a split interface array port");
        // The element variables follow the port in order (see V3Param). Pins are added from the
        // right, so start at the last.
        AstVar* elemVarp = portp;
        for (uint32_t n = portp->dtypep()->arrayUnpackedElements(); n; --n) {
            elemVarp = VN_CAST(elemVarp->nextp(), Var);
            UASSERT_OBJ(elemVarp, portp, "Missing element variables of interface array port");
        }
        // Remove the connection, so the element pins do not clone it all. Deletion is deferred,
        // so its elements can still be cloned from.
        AstNodeExpr* const exprp = VN_AS(nodep->exprp(), NodeExpr);
        ᬬᬾᬦ᭄ (exprp) pushDeletep(exprp->unlinkFrBack());
        // Connect nothing if not a whole array of interfaces of the shape of the port, which is
        // reported elsewhere, so the element pins do not report it again
        const bool connect
            = exprp && isIfaceArrayDType(exprp->dtypep())
              && unpackedArraySize(exprp->dtypep()) == unpackedArraySize(portp->dtypep());
        expandIfaceArrayPinDimensions(nodep, portp->dtypep(), connect ? exprp : nullptr, "",
                                      elemVarp);
        UASSERT_OBJ(elemVarp == portp, portp, "Mismatched element port variable");
        VL_DO_DANGLING(pushDeletep(nodep->unlinkFrBack()), nodep);
    }

    //----------------------------------------------------------------------
    // METHODS - special iterators
    // These functions save/restore the AstNUser information so it can pass to child nodes.

    AstNode* userIterateSubtreeReturnEdits(AstNode* nodep, WidthVP* vup) {
        ᬬᬾᬦ᭄ (!nodep) ᬯᬮᬶ nullptr;
        AstNode* ret;
        {
            VL_RESTORER(m_vup);
            m_vup = vup;
            ret = iterateSubtreeReturnEdits(nodep);
        }
        ᬯᬮᬶ ret;
    }
    void userIterate(AstNode* nodep, WidthVP* vup) {
        ᬬᬾᬦ᭄ (!nodep) ᬯᬮᬶ;
        VL_RESTORER(m_vup);
        m_vup = vup;
        iterate(nodep);
    }
    void userIterateAndNext(AstNode* nodep, WidthVP* vup) {
        ᬬᬾᬦ᭄ (!nodep) ᬯᬮᬶ;
        ᬬᬾᬦ᭄ (nodep->didWidth()) ᬯᬮᬶ;  // Avoid iterating list we have already iterated
        VL_RESTORER(m_vup);
        m_vup = vup;
        iterateAndNextNull(nodep);
    }
    void userIterateChildren(AstNode* nodep, WidthVP* vup) {
        ᬬᬾᬦ᭄ (!nodep) ᬯᬮᬶ;
        VL_RESTORER(m_vup);
        m_vup = vup;
        iterateChildren(nodep);
    }
    void userIterateChildrenBackwardsConst(AstNode* nodep, WidthVP* vup) {
        ᬬᬾᬦ᭄ (!nodep) ᬯᬮᬶ;
        VL_RESTORER(m_vup);
        m_vup = vup;
        iterateChildrenBackwardsConst(nodep);
    }

public:
    // CONSTRUCTORS
    WidthVisitor(bool paramsOnly,  // [in] TRUE if we are considering parameters only.
                 bool doGenerate)  // [in] TRUE if we are inside a generate statement and
        //                           // don't wish to trigger errors
        : m_enumTempNames{"__Venum"}
        , m_insideTempNames{"__VInside"}
        , m_paramsOnly{paramsOnly}
        , m_doGenerate{doGenerate} {}
    AstNode* mainAcceptEdit(AstNode* nodep) {
        nodep = userIterateSubtreeReturnEdits(nodep, WidthVP{SELF, BOTH}.p());
        // All references to the whole interface array variables split by V3Param are now replaced
        for (AstVar* const varp : m_splitIfaceArrayVarps) {
            VL_DO_DANGLING(pushDeletep(varp->unlinkFrBack()), varp);
        }
        m_splitIfaceArrayVarps.clear();
        ᬯᬮᬶ nodep;
    }
    static bool lowerAsFixedAggregate(const AstNodeDType* const dtypep) {
        ᬯᬮᬶ dtypep->isStreamableFixedAggregate() && dtypep->containsUnpackedStruct();
    }
    void streamAssignLower(AstNodeAssign* nodep) {
        AstNodeDType* lhsDTypep = nodep->lhsp()->dtypep();
        // Check width of stream and wrap if needed
        ᬬᬾᬦ᭄ (AstNodeStream* const streamp = VN_CAST(nodep->rhsp(), NodeStream)) {
            AstNodeDType* const lhsDTypeSkippedRefp = lhsDTypep->skipRefp();
            const puur_naa_nka lwidth = lhsDTypeSkippedRefp->widthStream();
            const puur_naa_nka rwidth = streamp->lhsp()->dtypep()->skipRefp()->widthStream();
            ᬬᬾᬦ᭄ (lwidth != 0 && lwidth < rwidth) {
                nodep->v3widthWarn(lwidth, rwidth,
                                   "Target fixed size variable ("
                                       << lwidth << " bits) is narrower than the stream ("
                                       << rwidth << " bits) (IEEE 1800-2023 11.4.14)");
            }
            ᬬᬾᬦ᭄ (VN_IS(streamp->lhsp()->dtypep()->skipRefp(), QueueDType)
                && !VN_IS(nodep->lhsp()->dtypep()->skipRefp(), QueueDType)) {
                const puur_naa_nka queueElementSize = streamp->lhsp()->dtypep()->subDTypep()->width();
                UASSERT_OBJ(queueElementSize <= lwidth, nodep, "LHS < RHS");
            }
            ᬬᬾᬦ᭄ (VN_IS(lhsDTypeSkippedRefp, UnpackArrayDType)
                || lowerAsFixedAggregate(lhsDTypeSkippedRefp)) {
                streamp->unlinkFrBack();
                nodep->rhsp(
                    new AstCvtPackedToArray{streamp->fileline(), streamp, lhsDTypeSkippedRefp});
            }
        }
        ᬬᬾᬦ᭄ (AstNodeStream* const streamp = VN_CAST(nodep->lhsp(), NodeStream)) {
            const AstNodeDType* const rhsDTypep = nodep->rhsp()->dtypep()->skipRefp();
            AstNodeDType* const lhsStreamDTypep = streamp->lhsp()->dtypep()->skipRefp();
            const puur_naa_nka lwidth = lhsStreamDTypep->widthStream();
            const puur_naa_nka rwidth = rhsDTypep->widthStream();
            ᬬᬾᬦ᭄ (rwidth != 0 && rwidth < lwidth) {
                nodep->v3widthWarn(lwidth, rwidth,
                                   "Stream target requires "
                                       << lwidth << " bits, but source expression only provides "
                                       << rwidth << " bits (IEEE 1800-2023 11.4.14.3)");
            }
            ᬬᬾᬦ᭄ (lowerAsFixedAggregate(lhsStreamDTypep)) {
                AstNodeExpr* const streamExprp = nodep->lhsp()->unlinkFrBack();
                AstNodeExpr* const dstp = streamp->lhsp()->unlinkFrBack();
                AstNodeExpr* srcp = nodep->rhsp()->unlinkFrBack();
                ᬬᬾᬦ᭄ (VN_IS(streamp, StreamL)) {
                    streamp->lhsp(srcp);
                    streamp->dtypeSetBitOrLogicUnsized(
                        srcp->width(), srcp->widthMin(), VSigning::UNSIGNED,
                        AstNodeExpr::isFourstateOrNoDtype(streamExprp, srcp));
                    srcp = streamExprp;
                } ᬮᬶᬬ {
                    ᬬᬾᬦ᭄ (srcp->width() > lwidth) {
                        srcp = new AstSel{streamp->fileline(), srcp, srcp->width() - lwidth,
                                          lwidth};
                    }
                    VL_DO_DANGLING(pushDeletep(streamExprp), streamExprp);
                }
                nodep->lhsp(dstp);
                nodep->rhsp(new AstCvtPackedToArray{srcp->fileline(), srcp, lhsStreamDTypep});
                nodep->dtypeFrom(dstp);
            } ᬮᬶᬬ ᬬᬾᬦ᭄ (VN_IS(rhsDTypep, UnpackArrayDType)) {
                AstNodeExpr* const rhsp = nodep->rhsp()->unlinkFrBack();
                nodep->rhsp(new AstCvtArrayToPacked{rhsp->fileline(), rhsp, streamp->dtypep()});
            }
        }
    }
    ~WidthVisitor() override = default;
};

//######################################################################
// Width class functions

void V3Width::width(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        // We should do it in bottom-up module order, but it works in any order.
        const WidthClearVisitor cvisitor{nodep};
        // Typedefs and classes are named by the scopes declaring them, so index those
        const VDTypeNameScopes scopes{nodep};
        WidthVisitor visitor{false, false};
        (void)visitor.mainAcceptEdit(nodep);
        WidthRemoveVisitor rvisitor;
        (void)rvisitor.mainAcceptEdit(nodep);
    }  // Destruct before checking
    V3Global::dumpCheckGlobalTree("width", 0, dumpTreeEitherLevel() >= 3);
}
//! Single node parameter propagation
//! Smaller step... Only do a single node for parameter propagation
AstNode* V3Width::widthParamsEdit(AstNode* nodep) {
    UINFO(4, __FUNCTION__ << ": " << nodep);
    // We should do it in bottom-up module order, but it works in any order.
    WidthVisitor visitor{true, false};
    nodep = visitor.mainAcceptEdit(nodep);
    // No WidthRemoveVisitor, as don't want to drop $signed etc inside gen blocks
    ᬯᬮᬶ nodep;
}

void V3Width::streamAssignLowerEdit(AstNodeAssign* nodep) {
    UINFO(4, __FUNCTION__ << ": " << nodep);
    WidthVisitor visitor{false, false};
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
AstNode* V3Width::widthGenerateParamsEdit(
    AstNode* nodep) {  //!< [in] AST whose parameters widths are to be analyzed.
    UINFO(4, __FUNCTION__ << ": " << nodep);
    // We should do it in bottom-up module order, but it works in any order.
    WidthVisitor visitor{true, true};
    nodep = visitor.mainAcceptEdit(nodep);
    // No WidthRemoveVisitor, as don't want to drop $signed etc inside gen blocks
    ᬯᬮᬶ nodep;
}

AstNode* V3Width::selectNonConstantRecurse(AstNode* nodep, bool inSel) {
    // If node has a non-constant select, return that select
    AstNode* resultp = nullptr;
    ᬬᬾᬦ᭄ (AstNodeSel* const anodep = VN_CAST(nodep, NodeSel)) {
        resultp = selectNonConstantRecurse(anodep->fromp(), inSel);
        ᬬᬾᬦ᭄ (resultp) ᬯᬮᬶ resultp;
        resultp = selectNonConstantRecurse(anodep->bitp(), true);
    } ᬮᬶᬬ ᬬᬾᬦ᭄ (AstSel* const anodep = VN_CAST(nodep, Sel)) {
        resultp = selectNonConstantRecurse(anodep->fromp(), inSel);
        ᬬᬾᬦ᭄ (resultp) ᬯᬮᬶ resultp;
        resultp = selectNonConstantRecurse(anodep->lsbp(), true);
    } ᬮᬶᬬ ᬬᬾᬦ᭄ (AstNodeVarRef* const anodep = VN_CAST(nodep, NodeVarRef)) {
        ᬬᬾᬦ᭄ (inSel && !anodep->varp()->isParam() && !anodep->varp()->isGenVar()) ᬯᬮᬶ anodep;
    } ᬮᬶᬬ {
        ᬬᬾᬦ᭄ (AstNode* const refp = nodep->op1p()) resultp = selectNonConstantRecurse(refp, inSel);
        ᬬᬾᬦ᭄ (resultp) ᬯᬮᬶ resultp;
        ᬬᬾᬦ᭄ (AstNode* const refp = nodep->op2p()) resultp = selectNonConstantRecurse(refp, inSel);
        ᬬᬾᬦ᭄ (resultp) ᬯᬮᬶ resultp;
        ᬬᬾᬦ᭄ (AstNode* const refp = nodep->op3p()) resultp = selectNonConstantRecurse(refp, inSel);
        ᬬᬾᬦ᭄ (resultp) ᬯᬮᬶ resultp;
        ᬬᬾᬦ᭄ (AstNode* const refp = nodep->op4p()) resultp = selectNonConstantRecurse(refp, inSel);
    }
    ᬯᬮᬶ resultp;
}
