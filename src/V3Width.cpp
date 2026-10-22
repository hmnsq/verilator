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

#অন্তর্ভাৱ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#অন্তর্ভাৱ "V3Width.h"

#অন্তর্ভাৱ "V3Assert.h"
#অন্তর্ভাৱ "V3Ast.h"
#অন্তর্ভাৱ "V3Begin.h"
#অন্তর্ভাৱ "V3Const.h"
#অন্তর্ভাৱ "V3ConstPool.h"
#অন্তর্ভাৱ "V3Error.h"
#অন্তর্ভাৱ "V3Global.h"
#অন্তর্ভাৱ "V3LinkLValue.h"
#অন্তর্ভাৱ "V3MemberMap.h"
#অন্তর্ভাৱ "V3Number.h"
#অন্তর্ভাৱ "V3Randomize.h"
#অন্তর্ভাৱ "V3String.h"
#অন্তর্ভাৱ "V3Task.h"
#অন্তর্ভাৱ "V3UniqueNames.h"
#অন্তর্ভাৱ "V3WidthCommit.h"

// More code; this file was getting too large; see actions there
#define VERILATOR_V3WIDTH_CPP_
#অন্তর্ভাৱ "V3WidthRemove.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

শ্ৰেণী ContainingClassFinder final {
    // The cache stores the class containing each node, excluding the node itself when it is a
    // class. This lets recursion through preceding siblings reuse the same ownership result.
    // Outside of V3Width, where the whole netlist is iterated, instead of this technique,
    // parents should be remembered during the top-down visit recursion into classes.
    std::unordered_map<ধ্রুৱ Ast_গ্ৰন্থি*, AstClass*> m_cache;

    AstClass* findCached(Ast_গ্ৰন্থি* nodep) {
        যদি (!nodep) ঘূৰাই nullptr;
        ধ্রুৱ auto it = m_cache.find(nodep);
        যদি (it != m_cache.end()) ঘূৰাই it->second;

        AstClass* classp = nullptr;
        যদি (nodep->backp() && nodep->backp()->nextp() == nodep) {
            classp = findCached(nodep->backp());
        } অন্যথা যদি (AstClass* ধ্রুৱ parentp = VN_CAST(nodep->backp(), Class)) {
            classp = parentp;
        } অন্যথা যদি (AstClassPackage* ধ্রুৱ packagep = VN_CAST(nodep->backp(), ClassPackage)) {
            classp = packagep->classp();
        } অন্যথা {
            classp = findCached(nodep->backp());
        }
        m_cache.emplace(nodep, classp);
        ঘূৰাই classp;
    }

সার্ৱজনিক:
    AstClass* find(Ast_গ্ৰন্থি* nodep) {
        যদি (AstClass* ধ্রুৱ classp = VN_CAST(nodep, Class)) ঘূৰাই classp;
        যদি (AstClassPackage* ধ্রুৱ packagep = VN_CAST(nodep, ClassPackage)) {
            ঘূৰাই packagep->classp();
        }
        ঘূৰাই findCached(nodep);
    }
};

//######################################################################

গণনা Stage : uint8_t {
    PRELIM = 1,
    FINAL = 2,
    BOTH = 3
};  // Numbers are a bitmask <0>=prelim, <1>=final
std::ostream& operator<<(std::ostream& str, ধ্রুৱ Stage& rhs) {
    ঘূৰাই str << ("-PFB"[static_cast<পূর্ণাঙ্ক>(rhs)]);
}

গণনা Determ : uint8_t {
    SELF,  // Self-determined
    CONTEXT_DET,  // Context-determined
    ASSIGN  // Assignment-like where sign comes from RHS only
};
std::ostream& operator<<(std::ostream& str, ধ্রুৱ Determ& rhs) {
    স্থির constexpr ধ্রুৱ ৱর্ণ* ধ্রুৱ s_det[] = {"SELF", "CNTX", "ASSN"};
    ঘূৰাই str << s_det[rhs];
}

গণনা StreamUse : uint8_t {
    STREAM_USE_NONE,  // Stream concatenation is not allowed in this context
    STREAM_USE_ASSIGN,  // Assignment-like context permits stream concatenation
    STREAM_USE_CAST,  // Explicit cast context permits stream concatenation
};

#define v3widthWarn(lhs, rhs, বাৰ্তা) \
    v3warnCode(((lhs) < (rhs)   ? V3ErrorCode::WIDTHTRUNC \
                : (lhs) > (rhs) ? V3ErrorCode::WIDTHEXPAND \
                                : V3ErrorCode::WIDTH), \
               বাৰ্তা)

//######################################################################
// Width state, as a visitor of each AstNode

শ্ৰেণী WidthVP final {
    // Parameters to pass down hierarchy with visit functions.
    Ast_গ্ৰন্থি_DType* ধ্রুৱ m_dtypep;  // Parent's data type to resolve to
    ধ্রুৱ Stage m_stage;  // If true, report errors
    ধ্রুৱ StreamUse m_streamUse;  // Current expression may be a stream concat

সার্ৱজনিক:
    WidthVP(Ast_গ্ৰন্থি_DType* dtypep, Stage stage, StreamUse streamUse = STREAM_USE_NONE)
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
        যদি (determ != SELF && stage != PRELIM)
            v3fatalSrc("Context-determined width request only allowed as prelim step");
    }
    WidthVP* p() { ঘূৰাই এতত্; }
    তার্কিক selfDtm() ধ্রুৱ { ঘূৰাই m_dtypep == nullptr; }
    StreamUse streamUse() ধ্রুৱ { ঘূৰাই m_streamUse; }
    Ast_গ্ৰন্থি_DType* dtypep() ধ্রুৱ {
        // Detect where overrideDType is probably the intended call
        UASSERT(m_dtypep, "Width dtype request on self-determined or preliminary VUP");
        ঘূৰাই m_dtypep;
    }
    Ast_গ্ৰন্থি_DType* dtypeNullp() ধ্রুৱ { ঘূৰাই m_dtypep; }
    Ast_গ্ৰন্থি_DType* dtypeNullSkipRefp() ধ্রুৱ {
        Ast_গ্ৰন্থি_DType* dtp = dtypeNullp();
        যদি (dtp) dtp = dtp->skipRefp();
        ঘূৰাই dtp;
    }
    Ast_গ্ৰন্থি_DType* dtypeOverridep(Ast_গ্ৰন্থি_DType* ধ্রুৱ defaultp) ধ্রুৱ {
        UASSERT(m_stage != PRELIM, "Parent dtype should be a final-stage action");
        যদি (!m_dtypep) ঘূৰাই defaultp;
        যদি (m_dtypep->skipRefp()->isIntegralOrPacked()
            && defaultp->skipRefp()->isIntegralOrPacked()
            && m_dtypep->isFourstate() != defaultp->isFourstate()) {
            UASSERT_OBJ(m_dtypep->skipRefp()->basicp() && defaultp->skipRefp()->basicp(), m_dtypep,
                        "Unexpected type override from: " << defaultp << " to: " << m_dtypep);
            ঘূৰাই defaultp->findBitOrLogicDType(m_dtypep->width(), m_dtypep->widthMin(),
                                                 m_dtypep->numeric(), defaultp->isFourstate());
        }
        ঘূৰাই m_dtypep;
    }
    পূর্ণাঙ্ক width() ধ্রুৱ {
        UASSERT(m_dtypep, "Width request on self-determined or preliminary VUP");
        ঘূৰাই m_dtypep->width();
    }
    পূর্ণাঙ্ক widthMin() ধ্রুৱ {
        UASSERT(m_dtypep, "Width request on self-determined or preliminary VUP");
        ঘূৰাই m_dtypep->widthMin();
    }
    তার্কিক prelim() ধ্রুৱ { ঘূৰাই m_stage & PRELIM; }
    তার্কিক final() ধ্রুৱ { ঘূৰাই m_stage & FINAL; }
    শূন্য dump(std::ostream& str) ধ্রুৱ {
        যদি (!m_dtypep) {
            str << "  VUP(s=" << m_stage << ",self)";
        } অন্যথা {
            str << "  VUP(s=" << m_stage << ",dt=" << cvtToHex(dtypep());
            dtypep()->dumpSmall(str);
            str << ")";
        }
    }
};
std::ostream& operator<<(std::ostream& str, ধ্রুৱ WidthVP* vup) {
    যদি (vup) vup->dump(str);
    ঘূৰাই str;
}

//######################################################################

শ্ৰেণী WidthClearVisitor final {
    // Rather than a VNVisitor, can just quickly touch every node
    শূন্য clearWidthRecurse(Ast_গ্ৰন্থি* nodep) {
        বাবে (; nodep; nodep = nodep->nextp()) {
            nodep->didWidth(মিছা);
            যদি (Ast_গ্ৰন্থি* ধ্রুৱ refp = nodep->op1p()) clearWidthRecurse(refp);
            যদি (Ast_গ্ৰন্থি* ধ্রুৱ refp = nodep->op2p()) clearWidthRecurse(refp);
            যদি (Ast_গ্ৰন্থি* ধ্রুৱ refp = nodep->op3p()) clearWidthRecurse(refp);
            যদি (Ast_গ্ৰন্থি* ধ্রুৱ refp = nodep->op4p()) clearWidthRecurse(refp);
        }
    }

সার্ৱজনিক:
    // CONSTRUCTORS
    explicit WidthClearVisitor(AstNetlist* nodep) { clearWidthRecurse(nodep); }
    আভাস ~WidthClearVisitor() = স্ৱভাৱ;
};

//######################################################################

#define accept in_Width_Visitor_use_Ast_গ্ৰন্থি_iterate_instead_of_Ast_গ্ৰন্থি_accept

//######################################################################

শ্ৰেণী WidthVisitor final : সার্ৱজনিক VNVisitor {
    // TYPES
    using PatVecMap = std::map<পূর্ণাঙ্ক, AstPatMember*>;
    using DTypeMap = std::map<ধ্রুৱ std::string, AstPatMember*>;

    // STATE
    V3UniqueNames m_enumTempNames;  // Captured enum expressions
    V3UniqueNames m_insideTempNames;  // For generating unique temporary variable names for
                                      // `inside` expressions
    VMemberMap m_memberMap;  // Member names cached for fast lookup
    std::unordered_map<ধ্রুৱ AstModport*, std::unordered_set<string>>
        m_modportNames;  // Names listed by each modport
    V3TaskConnectState m_taskConnectState;  // State to cache V3Task::taskConnects
    WidthVP* m_vup = nullptr;  // Current node state
    তার্কিক m_underFork = মিছা;  // Visiting under a fork
    তার্কিক m_underSExpr = মিছা;  // Visiting under a sequence expression
    তার্কিক m_underPackedArray = মিছা;  // Visiting under a AstPackArrayDType
    তার্কিক m_underMemberSel = মিছা;  // Viting under a MemberSel
    Ast_গ্ৰন্থি* m_seqUnsupp = nullptr;  // Property has unsupported node
    তার্কিক m_hasSExpr = মিছা;  // Property has a sequence expression
    ধ্রুৱ AstCell* m_cellp = nullptr;  // Current cell for arrayed instantiations
    std::vector<AstVar*> m_splitIfaceArrayVarps;  // Interface arrays split by V3Param
    ধ্রুৱ AstEnumItem* m_enumItemp = nullptr;  // Current enum item
    Ast_গ্ৰন্থি_FTask* m_ftaskp = nullptr;  // Current function/task
    Ast_গ্ৰন্থি_Module* m_modep = nullptr;  // Current module
    ধ্রুৱ AstConstraint* m_constraintp = nullptr;  // Current constraint
    Ast_গ্ৰন্থি_Procedure* m_procedurep = nullptr;  // Current final/always
    ধ্রুৱ AstWith* m_withp = nullptr;  // Current 'with' statement
    ধ্রুৱ AstFunc* m_funcp = nullptr;  // Current function
    ধ্রুৱ AstAttrOf* m_attrp = nullptr;  // Current attribute
    ধ্রুৱ Ast_গ্ৰন্থি_ৰাশিমালা* m_randomizeFromp = nullptr;  // Current randomize method call fromp
    ধ্রুৱ তার্কিক m_paramsOnly;  // Computing parameter value; limit operation
    ধ্রুৱ তার্কিক m_doGenerate;  // Do errors later inside generate statement
    তার্কিক m_streamConcat = মিছা;  // True if visiting arguments of stream concatenation
    তার্কিক m_inParamOverride = মিছা;  // True if visiting the value of a parameter override pin
    // Created dimension and enum tables, mapped to a reference reading the table (always cloned)
    std::map<std::pair<ধ্রুৱ Ast_গ্ৰন্থি_DType*, VAttrType>, AstVarRef*> m_tableMap;
    // Queues with given index type
    std::map<ধ্রুৱ Ast_গ্ৰন্থি_DType*, AstQueueDType*> m_queueDTypeIndexed;
    ContainingClassFinder m_containingClassFinder;
    std::unordered_set<AstVar*> m_aliasedVars;  // Variables referenced in alias
    std::unordered_set<ধ্রুৱ AstVar*> m_curModVars;  // Variables declared in current module

    স্থির constexpr পূর্ণাঙ্ক ENUM_LOOKUP_BITS = 16;  // Maximum # bits to make enum lookup table

    // ENUMS
    গণনা ExtendRule : uint8_t {
        EXTEND_EXP,  // Extend if expect sign and node signed, e.g. node=y in ADD(x,y), "x + y"
        EXTEND_ZERO,  // Extend with zeros. e.g. node=y in EQ(x,y), "x == y"
        EXTEND_LHS,  // Extend with sign if node signed. e.g. node=y in ASSIGN(y,x), "x = y"
        EXTEND_OFF  // No extension
    };
    গণনা শ্ৰেণী ComparisonType : uint8_t {
        NORMAL_EQ,  // Conventional comparison equal/unequal operator: ==, !=
        NORMAL_GL,  // Conventional comparison grater/less than: <, >, <=, >=
        CASE,  // Four-state aware comparison: ===, !==
        WILD,  // Wildcard equality operators: ?== ?=!
    };

    স্থির StreamUse streamUseForDeterm(Determ determ) {
        ঘূৰাই determ == ASSIGN ? STREAM_USE_ASSIGN : STREAM_USE_NONE;
    }

    StreamUse currentStreamUse() ধ্রুৱ { ঘূৰাই m_vup ? m_vup->streamUse() : STREAM_USE_NONE; }

    স্থির শূন্য packIfUnpacked(Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ nodep) {
        যদি (AstUnpackArrayDType* ধ্রুৱ unpackDTypep = VN_CAST(nodep->dtypep(), UnpackArrayDType)) {
            ধ্রুৱ পূর্ণাঙ্ক elements_অঙ্ক = unpackDTypep->arrayUnpackedElements();
            ধ্রুৱ পূর্ণাঙ্ক unpackMinBits = elements_অঙ্ক * unpackDTypep->subDTypep()->widthMin();
            ধ্রুৱ পূর্ণাঙ্ক unpackBits = elements_অঙ্ক * unpackDTypep->subDTypep()->width();
            VNRelinker relinker;
            nodep->unlinkFrBack(&relinker);
            relinker.relink(নূতন AstCvtArrayToPacked{
                nodep->fileline(), nodep,
                nodep->findBitOrLogicDType(unpackBits, unpackMinBits, VSigning::UNSIGNED,
                                           nodep->dtypep()->isFourstate())});
        }
    }
    // When fromp() is a DType (e.g. unlinked RefDType), resolve through
    // the ref chain; when it's an expression, dtypep() is already resolved.
    স্থির Ast_গ্ৰন্থি_DType* fromDTypep(Ast_গ্ৰন্থি* fromp) {
        যদি (Ast_গ্ৰন্থি_DType* ধ্রুৱ dtypep = VN_CAST(fromp, গ্ৰন্থি_DType))
            ঘূৰাই dtypep->skipRefOrNullp();
        ঘূৰাই fromp ? fromp->dtypep() : nullptr;
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
    শূন্য visit(AstLogNot* nodep) override { visit_log_not(nodep); }
    // Widths: 1 bit out, lhs 1 bit, rhs 1 bit; Real: converts via compare with 0
    শূন্য visit(AstLogAnd* nodep) override { visit_log_and_or(nodep); }
    শূন্য visit(AstLogOr* nodep) override { visit_log_and_or(nodep); }
    // Sequence and/or/intersect (IEEE 1800-2023 16.9.2), same width rules as LogAnd/LogOr
    শূন্য visit(AstSAnd* nodep) override { visit_log_and_or(nodep); }
    শূন্য visit(AstSIntersect* nodep) override { visit_log_and_or(nodep); }
    শূন্য visit(AstSOr* nodep) override { visit_log_and_or(nodep); }
    শূন্য visit(AstSWithin* nodep) override { visit_log_and_or(nodep); }
    শূন্য visit(AstLogEq* nodep) override {
        // Conversion from real not in IEEE, but a fallout
        visit_log_and_or(nodep);
    }
    শূন্য visit(AstLogIf* nodep) override {
        // Conversion from real not in IEEE, but a fallout
        visit_log_and_or(nodep);
    }

    // Widths: 1 bit out, Any width lhs
    শূন্য visit(AstRedAnd* nodep) override { visit_red_and_or(nodep); }
    শূন্য visit(AstRedOr* nodep) override { visit_red_and_or(nodep); }
    শূন্য visit(AstRedXor* nodep) override { visit_red_and_or(nodep); }
    শূন্য visit(AstOneHot* nodep) override { visit_red_and_or(nodep); }
    শূন্য visit(AstOneHot0* nodep) override { visit_red_and_or(nodep); }
    শূন্য visit(AstIsUnknown* nodep) override {
        visit_red_unknown(nodep);  // Allow real
    }

    // These have different node types, as they operate differently
    // Must add to case statement below,
    // Widths: 1 bit out, lhs width == rhs width.  real if lhs|rhs real
    শূন্য visit(AstEq* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::NORMAL_EQ); }
    শূন্য visit(AstNeq* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::NORMAL_EQ); }
    শূন্য visit(AstGt* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::NORMAL_GL); }
    শূন্য visit(AstGte* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::NORMAL_GL); }
    শূন্য visit(AstLt* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::NORMAL_GL); }
    শূন্য visit(AstLte* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::NORMAL_GL); }
    শূন্য visit(AstGtS* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::NORMAL_GL); }
    শূন্য visit(AstGteS* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::NORMAL_GL); }
    শূন্য visit(AstLtS* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::NORMAL_GL); }
    শূন্য visit(AstLteS* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::NORMAL_GL); }
    শূন্য visit(AstEqCase* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::CASE); }
    শূন্য visit(AstNeqCase* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::CASE); }
    // ...    These comparisons don't allow reals
    শূন্য visit(AstEqWild* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::WILD); }
    শূন্য visit(AstNeqWild* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::WILD); }
    // ...    Real compares
    শূন্য visit(AstEqD* nodep) override { visit_cmp_real(nodep); }
    শূন্য visit(AstNeqD* nodep) override { visit_cmp_real(nodep); }
    শূন্য visit(AstLtD* nodep) override { visit_cmp_real(nodep); }
    শূন্য visit(AstLteD* nodep) override { visit_cmp_real(nodep); }
    শূন্য visit(AstGtD* nodep) override { visit_cmp_real(nodep); }
    শূন্য visit(AstGteD* nodep) override { visit_cmp_real(nodep); }
    // ...    String compares
    শূন্য visit(AstEqN* nodep) override { visit_cmp_string(nodep); }
    শূন্য visit(AstNeqN* nodep) override { visit_cmp_string(nodep); }
    শূন্য visit(AstLtN* nodep) override { visit_cmp_string(nodep); }
    শূন্য visit(AstLteN* nodep) override { visit_cmp_string(nodep); }
    শূন্য visit(AstGtN* nodep) override { visit_cmp_string(nodep); }
    শূন্য visit(AstGteN* nodep) override { visit_cmp_string(nodep); }
    // ...    Data type compares
    শূন্য visit(AstEqT* nodep) override { visit_cmp_type(nodep); }
    শূন্য visit(AstNeqT* nodep) override { visit_cmp_type(nodep); }

    // Widths: out width = lhs width = rhs width
    // Signed: Output signed iff LHS & RHS signed.
    // Real: Not allowed
    শূন্য visit(AstAnd* nodep) override { visit_boolexpr_and_or(nodep); }
    শূন্য visit(AstOr* nodep) override { visit_boolexpr_and_or(nodep); }
    শূন্য visit(AstXor* nodep) override { visit_boolexpr_and_or(nodep); }
    শূন্য visit(AstBufIf1* nodep) override {
        visit_boolexpr_and_or(nodep);
    }  // Signed behavior changing in 3.814
    // Width: Max(Lhs,Rhs) sort of.
    // Real: If either side real
    // Signed: If both sides real
    শূন্য visit(AstAdd* nodep) override { visit_add_sub_replace(nodep, সঁচা, মিছা); }
    শূন্য visit(AstSub* nodep) override { visit_add_sub_replace(nodep, সঁচা, মিছা); }
    শূন্য visit(AstDiv* nodep) override { visit_add_sub_replace(nodep, সঁচা, সঁচা); }
    শূন্য visit(AstMul* nodep) override { visit_add_sub_replace(nodep, সঁচা, মিছা); }
    // These can't promote to real
    শূন্য visit(AstModDiv* nodep) override { visit_add_sub_replace(nodep, মিছা, সঁচা); }
    শূন্য visit(AstModDivS* nodep) override { visit_add_sub_replace(nodep, মিছা, সঁচা); }
    শূন্য visit(AstMulS* nodep) override { visit_add_sub_replace(nodep, মিছা, মিছা); }
    শূন্য visit(AstDivS* nodep) override { visit_add_sub_replace(nodep, মিছা, সঁচা); }
    // Widths: out width = lhs width, but upper matters
    // Signed: Output signed iff LHS signed; unary operator
    // Unary promote to real
    শূন্য visit(AstNegate* nodep) override { visit_negate_not(nodep, সঁচা); }
    // Unary never real
    শূন্য visit(AstNot* nodep) override { visit_negate_not(nodep, মিছা); }

    // Real: inputs and output real
    শূন্য visit(AstAddD* nodep) override { visit_real_add_sub(nodep); }
    শূন্য visit(AstSubD* nodep) override { visit_real_add_sub(nodep); }
    শূন্য visit(AstDivD* nodep) override { visit_real_add_sub(nodep); }
    শূন্য visit(AstMulD* nodep) override { visit_real_add_sub(nodep); }
    শূন্য visit(AstPowD* nodep) override { visit_real_add_sub(nodep); }
    শূন্য visit(Ast_গ্ৰন্থি_System_Biop_D* nodep) override { visit_real_add_sub(nodep); }
    // Real: Output real
    শূন্য visit(AstNegateD* nodep) override { visit_real_neg_ceil(nodep); }
    শূন্য visit(Ast_গ্ৰন্থি_System_Uniop_D* nodep) override { visit_real_neg_ceil(nodep); }

    // Widths: out signed/unsigned width = lhs width, input un|signed
    শূন্য visit(AstSigned* nodep) override { visit_signed_unsigned(nodep, VSigning::SIGNED); }
    শূন্য visit(AstUnsigned* nodep) override { visit_signed_unsigned(nodep, VSigning::UNSIGNED); }

    // Widths: Output width from lhs, rhs<33 bits
    // Signed: If lhs signed
    শূন্য visit(AstShiftL* nodep) override { visit_shift(nodep); }
    শূন্য visit(AstShiftR* nodep) override { visit_shift(nodep); }
    // ShiftRS converts to ShiftR, but not vice-versa
    শূন্য visit(AstShiftRS* nodep) override { visit_shift(nodep); }

    //========
    // Widths: Output real, input integer signed
    শূন্য visit(AstBitsToRealD* nodep) override { visit_Or_Lu64(nodep); }

    // Widths: Output integer signed, input real
    শূন্য visit(AstRToIS* nodep) override { visit_Os32_Lr(nodep); }
    শূন্য visit(AstRToIRoundS* nodep) override {
        // Only created here, size comes from upper expression
        assert_At_ৰাশিমালা(nodep);
        যদি (m_vup->prelim()) {  // First stage evaluation
            iterateCheckReal(nodep, "LHS", nodep->lhsp(), BOTH);
        }
        UASSERT_OBJ(nodep->dtypep()->widthSized(), nodep, "RToIRoundS should be presized");
    }

    // Widths: Output integer unsigned, input real
    শূন্য visit(AstRealToBits* nodep) override { visit_Ou64_Lr(nodep); }

    // Output integer, input string
    শূন্য visit(AstLenN* nodep) override {
        // Widths: 32 bit out
        UASSERT_OBJ(nodep->lhsp(), nodep, "For unary ops only!");
        যদি (m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            nodep->dtypeSetInt();
        }
    }
    শূন্য visit(AstPutcN* nodep) override {
        // CALLER: str.putc()
        UASSERT_OBJ(nodep->rhsp() && nodep->thsp(), nodep, "For ternary ops only!");
        যদি (m_vup && m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckSigned32(nodep, "RHS", nodep->rhsp(), BOTH);
            iterateCheckSigned8(nodep, "THS", nodep->thsp(), BOTH);
            nodep->dtypeSetString();  // AstPutcN returns the new string to be assigned by
                                      // AstAssign
        }
    }
    শূন্য visit(AstGetcN* nodep) override {
        // CALLER: str.getc()
        UASSERT_OBJ(nodep->rhsp(), nodep, "For binary ops only!");
        যদি (m_vup && m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckSigned32(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtypeSetBitSized(8, VSigning::UNSIGNED);
        }
    }
    শূন্য visit(AstGetcRefN* nodep) override {
        // CALLER: str.getc()
        UASSERT_OBJ(nodep->rhsp(), nodep, "For binary ops only!");
        যদি (m_vup && m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckSigned32(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtypeSetBitSized(8, VSigning::UNSIGNED);
        }
    }
    শূন্য visit(AstSubstrN* nodep) override {
        // CALLER: str.substr()
        UASSERT_OBJ(nodep->rhsp() && nodep->thsp(), nodep, "For ternary ops only!");
        যদি (m_vup && m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckSigned32(nodep, "RHS", nodep->rhsp(), BOTH);
            iterateCheckSigned32(nodep, "THS", nodep->thsp(), BOTH);
            nodep->dtypeSetString();
        }
    }
    শূন্য visit(AstCompareNN* nodep) override {
        // CALLER: str.compare(), str.icompare()
        // Widths: 32 bit out
        UASSERT_OBJ(nodep->rhsp(), nodep, "For binary ops only!");
        assert_At_ৰাশিমালা(nodep);
        যদি (m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckString(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtypeSetInt();
        }
    }
    শূন্য visit(AstAtoN* nodep) override {
        // CALLER: str.atobin(), atoi(), atohex(), atooct(), atoreal()
        // Width: 64bit floating point for atoreal(), 32bit out for the others
        assert_At_ৰাশিমালা(nodep);
        যদি (m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            যদি (nodep->fmtType() == AstAtoN::ATOREAL) {
                nodep->dtypeSetDouble();
            } অন্যথা {
                nodep->dtypeSetInteger();
            }
        }
    }
    শূন্য visit(AstNToI* nodep) override {
        // Created here, should be already sized
        assert_At_ৰাশিমালা(nodep);
        যদি (m_vup->prelim()) {
            UASSERT_OBJ(nodep->dtypep(), nodep, "NToI should be sized when created");
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
        }
    }

    // Widths: Constant, terminal
    শূন্য visit(AstTime* nodep) override { nodep->dtypeSetTime2State(); }
    শূন্য visit(AstTimeD* nodep) override { nodep->dtypeSetDouble(); }
    শূন্য visit(AstTimePrecision* nodep) override { nodep->dtypeSetInteger2State(); }
    শূন্য visit(AstGetInitialRandomSeed* nodep) override { nodep->dtypeSetInt(); }
    শূন্য visit(AstTimeUnit* nodep) override {
        nodep->replaceWith(
            নূতন AstConst{nodep->fileline(), AstConst::Signed32{}, nodep->timeunit().powerOfTen()});
        VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
    }
    শূন্য visit(AstScopeName* nodep) override {
        nodep->dtypeSetUInt64();  // A pointer, but not that it matters
    }

    // Whether nodep, an override's value, is an untyped pattern, or a conditional of such
    স্থির তার্কিক isOverridePattern(ধ্রুৱ Ast_গ্ৰন্থি* nodep) {
        যদি (ধ্রুৱ AstPattern* ধ্রুৱ patternp = VN_CAST(nodep, Pattern)) {
            ঘূৰাই !patternp->childDTypep();
        }
        ধ্রুৱ AstCond* ধ্রুৱ condp = VN_CAST(nodep, Cond);
        ঘূৰাই condp && isOverridePattern(condp->thenp()) && isOverridePattern(condp->elsep());
    }
    শূন্য visit(AstCond* nodep) override {
        // op = cond ? expr1 : expr2
        // See IEEE-2012 11.4.11 and Table 11-21.
        //   LHS is self-determined
        //   Width: max(RHS, THS)
        //   Signed: Output signed iff RHS & THS signed  (presumed, not in IEEE)
        //   Real: Output real if either expression is real, non-real argument gets converted
        assert_At_ৰাশিমালা(nodep);
        যদি (m_inParamOverride) {
            // Choosing between override patterns, so type and fold only the condition, then
            // keep the pattern it picks, which the specialized module types as a lone one
            ধ্রুৱ পূর্ণাঙ্ক errors = V3Error::error_গুনতি();
            {
                VL_RESTORER(m_inParamOverride);
                m_inParamOverride = মিছা;
                iterateCheckBool(nodep, "Conditional Test", nodep->condp(), BOTH);
                V3Const::constifyParamsEdit(nodep->condp());  // Reports a non-constant
            }
            ধ্রুৱ AstConst* ধ্রুৱ constp = VN_CAST(nodep->condp(), Const);
            ধ্রুৱ তার্কিক known = constp && !constp->অঙ্ক().isFourState();
            // Folding reports a non-constant, but leaves x or z, even inside a conditional
            যদি (!known && V3Error::error_গুনতি() == errors) {
                nodep->condp()->v3warn(E_UNSUPPORTED,
                                       "Unsupported: Parameter override '?:' with assignment"
                                       " patterns and a condition without a known value.");
            }
            // After an error, keep the first pattern, so that no conditional is left untyped
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ keepp
                = (known && constp->isZero() ? nodep->elsep() : nodep->thenp())->unlinkFrBack();
            nodep->replaceWith(keepp);
            VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
            userIterate(keepp, WidthVP{SELF, BOTH}.p());
            ঘূৰাই;
        }
        যদি (m_vup->prelim()) {  // First stage evaluation
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
            Ast_গ্ৰন্থি_DType* ধ্রুৱ thenDTypep = nodep->thenp()->dtypep();
            Ast_গ্ৰন্থি_DType* ধ্রুৱ elseDTypep = nodep->elsep()->dtypep();
            Ast_গ্ৰন্থি_DType* ধ্রুৱ thenDTypeSkippedp = thenDTypep->skipRefp();
            Ast_গ্ৰন্থি_DType* ধ্রুৱ elseDTypeSkippedp = elseDTypep->skipRefp();
            যদি (nodep->thenp()->isNull() && nodep->elsep()->isNull()) {
                nodep->dtypep(m_vup->dtypeNullp() ? m_vup->dtypeNullp() : nodep->findNullDType());
            } অন্যথা যদি (nodep->thenp()->is_Class_Handle_মান()
                       || nodep->elsep()->is_Class_Handle_মান()) {
                Ast_গ্ৰন্থি_DType* commonClassTypep = nullptr;
                যদি (nodep->thenp()->is_Class_Handle_মান() && nodep->elsep()->is_Class_Handle_মান()) {
                    // Get the most-deriving class type that both arguments can be casted to.
                    commonClassTypep
                        = Ast_গ্ৰন্থি::getCommonClassTypep(nodep->thenp(), nodep->elsep());
                }
                যদি (commonClassTypep) {
                    nodep->dtypep(commonClassTypep);
                } অন্যথা {
                    nodep->v3error("Incompatible types of operands of condition operator: "
                                   << thenDTypep->prettyTypeName() << " and "
                                   << elseDTypep->prettyTypeName());
                    nodep->dtypeFrom(thenDTypep);
                }
            } অন্যথা যদি (!((thenDTypeSkippedp->isIntegralOrPacked()
                          || VN_IS(thenDTypeSkippedp, BasicDType))
                         && (elseDTypeSkippedp->isIntegralOrPacked()
                             || VN_IS(elseDTypeSkippedp, BasicDType)))) {
                যদি (Ast_গ্ৰন্থি::computeCastable(elseDTypep, thenDTypep, nullptr).isAssignable()) {
                    nodep->dtypep(elseDTypep);
                } অন্যথা {
                    // Set any type even if not castable
                    nodep->dtypep(thenDTypep);
                    যদি (!Ast_গ্ৰন্থি::computeCastable(thenDTypep, elseDTypep, nullptr)
                             .isAssignable()) {
                        nodep->v3error("Neither " << thenDTypep->prettyDTypeNameQ() << " or "
                                                  << elseDTypep->prettyDTypeNameQ()
                                                  << " is castable to the other one");
                    }
                }
            } অন্যথা যদি (thenDTypep->isDouble() || elseDTypep->isDouble()) {
                nodep->dtypeSetDouble();
            } অন্যথা যদি (thenDTypep->isString() || elseDTypep->isString()) {
                // There is a specific case:
                // strings and how they should be handled when condp() is 'x
                // ('x ? "Option1" : "Option2") === "Option0"
                // ('x ? "Option1" : "Option2") ===
                // 56'b010011110111000001110100011010010110111101101110001100xx
                // Issue is that the type of this is still string but the value contains 'x - types
                // and domains are not the same thing in SystemVerilog but in Verilator they are
                nodep->dtypeSetString();
            } অন্যথা {
                ধ্রুৱ তার্কিক isAnyResultFourstate
                    = thenDTypeSkippedp->isFourstate() || elseDTypeSkippedp->isFourstate();
                তার্কিক handled = মিছা;
                ধ্রুৱ তার্কিক isCondpFourstate = nodep->condp()->isFourstateOrNoDtype();
                Ast_গ্ৰন্থি_DType* ধ্রুৱ thenEnumDTypep = thenDTypep->skipRefToEnump();
                যদি (VN_IS(thenDTypeSkippedp, গ্ৰন্থি_UOr_Struct_DType)
                    && thenDTypeSkippedp->same_বৃক্ষ(elseDTypeSkippedp)) {
                    nodep->dtypep(thenDTypep);
                    handled = সঁচা;
                } অন্যথা যদি (VN_IS(thenEnumDTypep, EnumDType)
                           && thenEnumDTypep->same_বৃক্ষ(elseDTypep->skipRefToEnump())) {
                    // if enum is two-state we have to check condp() - because if it is
                    // four-state the result must be four-state as well
                    যদি (isAnyResultFourstate || !isCondpFourstate) {
                        nodep->dtypep(thenEnumDTypep);
                        handled = সঁচা;
                    } অন্যথা {
                        // This is similar case to what happens with strings
                        // So, to avoid cases where:
                        // enum int {A, B} type_t;
                        // logic'(foo) ? A : B
                        // throws an ENUMVALUE error for this error is disabled for this expression
                        // This is a result of how Verilator's type system works - to avoid such
                        // situations Verilator would need to either treat everything as four-state
                        // or separate idea of type and values domain - since four-state values are
                        // casted to two-state only with explicit casts or with assignments
                        nodep->fileline(নূতন File_সাৰি{nodep->fileline()});
                        nodep->fileline()->warnOff(V3ErrorCode::ENUMVALUE, সঁচা);
                    }
                }
                যদি (!handled) {
                    ধ্রুৱ পূর্ণাঙ্ক width = std::max(thenDTypep->width(), elseDTypep->width());
                    ধ্রুৱ পূর্ণাঙ্ক widthMin = std::max(thenDTypep->widthMin(), elseDTypep->widthMin());
                    ধ্রুৱ VSigning numeric
                        = VSigning::fromBool(thenDTypep->isSigned() && elseDTypep->isSigned());
                    ধ্রুৱ তার্কিক isFourstate = isAnyResultFourstate || isCondpFourstate;
                    // Try to preserve a dtype
                    যদি ((width == elseDTypeSkippedp->width()
                         && widthMin == elseDTypeSkippedp->widthMin())
                        && elseDTypeSkippedp->numeric() == numeric
                        && isFourstate == elseDTypeSkippedp->isFourstate()) {
                        nodep->dtypep(elseDTypep);
                    } অন্যথা যদি ((width == thenDTypeSkippedp->width()
                                && widthMin == thenDTypeSkippedp->widthMin())
                               && thenDTypeSkippedp->numeric() == numeric
                               && isFourstate == thenDTypeSkippedp->isFourstate()) {
                        nodep->dtypep(thenDTypep);
                    } অন্যথা {
                        // None of the children dtypes match generate one
                        nodep->dtypeSetBitOrLogicUnsized(width, widthMin, numeric, isFourstate);
                    }
                }
            }
        }
        যদি (m_vup->final()) {
            Ast_গ্ৰন্থি_DType* ধ্রুৱ expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
            Ast_গ্ৰন্থি_DType* ধ্রুৱ subDTypep = expDTypep;
            nodep->dtypep(expDTypep);
            // Error report and change sizes for suboperands of this node.
            iterateCheck(nodep, "Conditional True", nodep->thenp(), CONTEXT_DET, FINAL, subDTypep,
                         EXTEND_EXP);
            iterateCheck(nodep, "Conditional False", nodep->elsep(), CONTEXT_DET, FINAL, subDTypep,
                         EXTEND_EXP);
        }
    }
    শূন্য visit(AstConcat* nodep) override {
        // Real: Not allowed (assumed)
        // Signed: unsigned output, input either (assumed)
        // IEEE-2012 Table 11-21, and 11.8.1:
        //   LHS, RHS is self-determined
        //   signed: Unsigned  (11.8.1)
        //   width: LHS + RHS
        Ast_গ্ৰন্থি_DType* ধ্রুৱ vdtypep = m_vup->dtypeNullSkipRefp();
        userIterate(vdtypep, WidthVP{SELF, BOTH}.p());
        // Conversions
        যদি (Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ newp = newConsDynArrayOrQueue(nodep->fileline(), vdtypep)) {
            // IEEE 1800-2023 10.10 requires looking at argument data type
            // to determine if value or push
            userIterateAndNext(nodep->lhsp(), WidthVP{vdtypep, PRELIM}.p());
            userIterateAndNext(nodep->rhsp(), WidthVP{vdtypep, PRELIM}.p());
            newp->dtypeFrom(vdtypep);
            addConsItem(newp, nodep->lhsp()->unlinkFrBack());
            addConsItem(newp, nodep->rhsp()->unlinkFrBack());
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
            ঘূৰাই;
        }
        যদি (VN_IS(vdtypep, UnpackArrayDType)) {
            AstPattern* ধ্রুৱ newp = নূতন AstPattern{nodep->fileline(), nullptr};
            patConcatConvertRecurse(newp, nodep);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
            userIterate(newp, m_vup);
            ঘূৰাই;
        }

        // Concat handling
        যদি (m_vup->prelim()) {
            যদি (VN_IS(vdtypep, AssocArrayDType)  //
                || VN_IS(vdtypep, DynArrayDType)  //
                || VN_IS(vdtypep, QueueDType)) {
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: Concatenation to form "
                                                 << vdtypep->prettyDTypeNameQ() << " data type");
            }

            যদি (vdtypep && vdtypep->isString()) {
                iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
                iterateCheckString(nodep, "RHS", nodep->rhsp(), BOTH);
                nodep->dtypeSetString();
            } অন্যথা {
                iterateCheckIntegralSelf(nodep, "LHS", nodep->lhsp(), SELF, BOTH);
                iterateCheckIntegralSelf(nodep, "RHS", nodep->rhsp(), SELF, BOTH);

                যদি (m_streamConcat) {
                    packIfUnpacked(nodep->lhsp());
                    packIfUnpacked(nodep->rhsp());
                }
                nodep->dtypeSetBitOrLogicUnsized(
                    nodep->lhsp()->width() + nodep->rhsp()->width(),
                    nodep->lhsp()->widthMin() + nodep->rhsp()->widthMin(), VSigning::UNSIGNED,
                    Ast_গ্ৰন্থি_ৰাশিমালা::isFourstateOrNoDtype(nodep->lhsp(), nodep->rhsp()));
            }
            // Cleanup zero width Verilog2001 {x,{0{foo}}} now,
            // otherwise having width(0) will cause later assertions to fire
            যদি (ধ্রুৱ AstReplicate* ধ্রুৱ repp = VN_CAST(nodep->lhsp(), Replicate)) {
                যদি (repp->width() == 0) {  // Keep rhs
                    nodep->replaceWith(nodep->rhsp()->unlinkFrBack());
                    VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
                    ঘূৰাই;
                }
            }
            যদি (ধ্রুৱ AstReplicate* ধ্রুৱ repp = VN_CAST(nodep->rhsp(), Replicate)) {
                যদি (repp->width() == 0) {  // Keep lhs
                    nodep->replaceWith(nodep->lhsp()->unlinkFrBack());
                    VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
                    ঘূৰাই;
                }
            }
        }
        যদি (m_vup->final()) {
            যদি (nodep->lhsp()->isString() || nodep->rhsp()->isString()) {
                Ast_গ্ৰন্থি_ৰাশিমালা* lhsp = nodep->lhsp()->unlinkFrBack();
                Ast_গ্ৰন্থি_ৰাশিমালা* rhsp = nodep->rhsp()->unlinkFrBack();
                যদি (!lhsp->isString()) lhsp = নূতন AstCvtPackString{lhsp->fileline(), lhsp};
                যদি (!rhsp->isString()) rhsp = নূতন AstCvtPackString{rhsp->fileline(), rhsp};
                Ast_গ্ৰন্থি* ধ্রুৱ newp = নূতন AstConcatN{nodep->fileline(), lhsp, rhsp};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
                ঘূৰাই;
            }
            যদি (!nodep->dtypep()->widthSized()) {
                // See also error in V3Number
                গ্ৰন্থি_For_Unsized_Warning(nodep)->v3warn(
                    WIDTHCONCAT, "Unsized numbers/parameters not allowed in concatenations.");
            }
        }
    }
    শূন্য visit(AstConcatN* nodep) override {
        যদি (nodep->didWidth()) ঘূৰাই;
        // String concatenate.
        // Already did AstConcat simplifications
        assert_At_ৰাশিমালা(nodep);
        যদি (m_vup->prelim()) {
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckString(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtypeSetString();
        }
        যদি (m_vup->final()) {
            nodep->didWidth(সঁচা);
            যদি (!nodep->dtypep()->widthSized()) {
                // See also error in V3Number
                গ্ৰন্থি_For_Unsized_Warning(nodep)->v3warn(
                    WIDTHCONCAT, "Unsized numbers/parameters not allowed in concatenations.");
            }
        }
    }
    শূন্য visit(AstDefaultDisable* nodep) override {
        assertAtStatement(nodep);
        // it's like an if() condition.
        iterateCheckBool(nodep, "default disable iff condition", nodep->condp(), BOTH);
    }
    শূন্য visit(AstDelay* nodep) override {
        যদি (nodep->isCycleDelay() && m_underSExpr) {
            userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
            যদি (nodep->rhsp() && !nodep->isUnbounded()) {
                userIterateAndNext(nodep->rhsp(), WidthVP{SELF, BOTH}.p());
            }
            // A delay with a formal argument of a sequence or property is checked when that is
            // instanced, with the actual argument (IEEE 1800-2023 16.8)
            যদি (V3AssertCommon::refsSvaFormal(nodep->lhsp())
                || (nodep->rhsp() && V3AssertCommon::refsSvaFormal(nodep->rhsp()))) {
                ঘূৰাই;
            }
            // Fold parameterized SVA cycle-delay bounds
            V3AssertCommon::checkSvaCycleDelay(nodep, nodep);
            ঘূৰাই;
        }
        যদি (Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ fallDelayp = nodep->fallDelay()) {
            iterateCheckDelay(nodep, "delay", nodep->lhsp(), BOTH);
            iterateCheckDelay(nodep, "delay", fallDelayp, BOTH);
            ঘূৰাই;
        }
        যদি (VN_IS(m_procedurep, Final)) {
            nodep->v3error("Delays are not legal in final blocks (IEEE 1800-2023 9.2.3)");
            VL_DO_DANGLING(ঠেলো_Deletep(nodep->unlinkFrBack()), nodep);
            ঘূৰাই;
        }
        যদি (!m_underFork && VN_IS(m_ftaskp, Func)) {
            nodep->v3error("Delays are not legal in functions. Suggest use a task "
                           "(IEEE 1800-2023 13.4.4)");
            VL_DO_DANGLING(ঠেলো_Deletep(nodep->unlinkFrBack()), nodep);
            ঘূৰাই;
        }
        যদি (nodep->fileline()->timingOn()) {
            যদি (v3Global.opt.timing().isSetTrue()) {
                iterateCheckDelay(nodep, "delay", nodep->lhsp(), BOTH);
                iterateAndNextNull(nodep->stmtsp());
                ঘূৰাই;
            } অন্যথা যদি (v3Global.opt.timing().isSetFalse()) {
                nodep->v3warn(STMTDLY, "Ignoring delay on this statement due to --no-timing");
            } অন্যথা {
                nodep->v3warn(
                    E_NEEDTIMINGOPT,
                    "Use --timing or --no-timing to specify how delays should be handled");
            }
        }
        যদি (nodep->stmtsp()) nodep->addNextHere(nodep->stmtsp()->unlinkFrBackWithNext());
        VL_DO_DANGLING(ঠেলো_Deletep(nodep->unlinkFrBack()), nodep);
    }
    শূন্য visit(AstFireEvent* nodep) override {
        assertAtStatement(nodep);
        iterateCheckSelf(nodep, "LHS", nodep->operandp(), SELF, BOTH);
    }
    শূন্য visit(AstFork* nodep) override {
        যদি (!m_underFork && VN_IS(m_ftaskp, Func) && !nodep->joinType().joinNone()) {
            nodep->v3error("Only fork .. join_none is legal in functions. "
                           "(IEEE 1800-2023 13.4.4)");
            VL_DO_DANGLING(ঠেলো_Deletep(nodep->unlinkFrBack()), nodep);
            ঘূৰাই;
        }
        যদি (!nodep->fileline()->timingOn()
            // With no statements, begin is identical
            || !nodep->forksp()
            || (!v3Global.opt.timing().isSetTrue()  // If no --timing
                && (v3Global.opt.bboxUnsup()
                    // With one statement and no timing, a begin block does as good as a
                    // fork/join or join_any
                    || (!nodep->forksp()->nextp() && !nodep->joinType().joinNone())))) {
            AstBegin* ধ্রুৱ newp = নূতন AstBegin{nodep->fileline(), nodep->name(), nullptr, মিছা};
            nodep->replaceWith(newp);
            যদি (nodep->declsp()) newp->addDeclsp(nodep->declsp()->unlinkFrBackWithNext());
            যদি (nodep->stmtsp()) newp->addStmtsp(nodep->stmtsp()->unlinkFrBackWithNext());
            যদি (nodep->forksp()) newp->addStmtsp(nodep->forksp()->unlinkFrBackWithNext());
            VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
        } অন্যথা যদি (v3Global.opt.timing().isSetTrue()) {
            VL_RESTORER(m_underFork);
            m_underFork = সঁচা;
            iterateChildren(nodep);
        } অন্যথা যদি (v3Global.opt.timing().isSetFalse()) {
            nodep->v3warn(E_NOTIMING, "Fork statements require --timing");
            VL_DO_DANGLING(ঠেলো_Deletep(nodep->unlinkFrBack()), nodep);
        } অন্যথা {
            nodep->v3warn(E_NEEDTIMINGOPT,
                          "Use --timing or --no-timing to specify how forks should be handled");
        }
    }
    শূন্য visitWaitOrDisableFork(Ast_গ্ৰন্থি* nodep) {
        যদি (nodep->fileline()->timingOn()) {
            যদি (v3Global.opt.timing().isSetFalse()) {
                nodep->v3warn(E_NOTIMING, "Support for '" << nodep->verilogKwd()
                                                          << "' statement requires --timing");
                VL_DO_DANGLING(nodep->unlinkFrBack()->delete_বৃক্ষ(), nodep);
                ঘূৰাই;
            } অন্যথা যদি (!v3Global.opt.timing().isSetTrue()) {
                nodep->v3warn(E_NEEDTIMINGOPT, "Use --timing or --no-timing to specify how "
                                                   << "disable fork should be handled");
            }
        }
        iterateChildren(nodep);
    }
    শূন্য visit(AstDisableFork* nodep) override { visitWaitOrDisableFork(nodep); }
    শূন্য visit(AstWaitFork* nodep) override { visitWaitOrDisableFork(nodep); }
    শূন্য visit(AstToLowerN* nodep) override {
        assert_At_ৰাশিমালা(nodep);
        যদি (m_vup->prelim()) {
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            nodep->dtypeSetString();
        }
    }
    শূন্য visit(AstToUpperN* nodep) override {
        assert_At_ৰাশিমালা(nodep);
        যদি (m_vup->prelim()) {
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            nodep->dtypeSetString();
        }
    }
    শূন্য visit(AstReplicate* nodep) override {
        // IEEE-2012 Table 11-21:
        //   LHS, RHS is self-determined
        //   width: value(LHS) * width(RHS)
        assert_At_ৰাশিমালা(nodep);
        যদি (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "RHS", nodep->countp(), SELF, BOTH);
            V3Const::constifyParamsNoWarnEdit(nodep->countp());  // rhsp may change

            int32_t times = 1;  // IEEE replicate value is integral

            ধ্রুৱ AstConst* ধ্রুৱ constp = VN_CAST(nodep->countp(), Const);
            যদি (constp) {
                যদি (constp->অঙ্ক().isFourState()
                    || (constp->dtypep()->isSigned() && constp->অঙ্ক().isNegative())) {
                    nodep->v3error("Replication value of < 0 or X/Z not legal"
                                   " (IEEE 1800-2023 11.4.12.1): "
                                   << constp->prettyNameQ());
                } অন্যথা {
                    times = constp->toSInt();
                }
            }

            Ast_গ্ৰন্থি_DType* ধ্রুৱ vdtypep = m_vup->dtypeNullSkipRefp();
            যদি (VN_IS(vdtypep, QueueDType) || VN_IS(vdtypep, DynArrayDType)
                || VN_IS(vdtypep, UnpackArrayDType)) {
                যদি (times != 1) {
                    nodep->v3warn(E_UNSUPPORTED, "Unsupported: Non-1 replication to form "
                                                     << vdtypep->prettyDTypeNameQ()
                                                     << " data type");
                }
                যদি (VN_IS(nodep->srcp(), Concat)) {
                    // Convert to concat directly, and visit(AstConst) will convert.
                    // Don't iterate lhsp as SELF, the potential Concat below needs
                    // the adtypep passed down to recognize the QueueDType
                    userIterateAndNext(nodep->srcp(), WidthVP{vdtypep, BOTH}.p());
                    nodep->replaceWith(nodep->srcp()->unlinkFrBack());
                    VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
                    ঘূৰাই;
                } অন্যথা {  // int a[] = {lhs} -> same as '{lhs}
                    AstPattern* ধ্রুৱ newp = নূতন AstPattern{
                        nodep->fileline(),
                        নূতন AstPatMember{nodep->srcp()->fileline(), nodep->srcp()->unlinkFrBack(),
                                         nullptr, nullptr}};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
                    userIterate(newp, m_vup);
                    ঘূৰাই;
                }
            }
            যদি (VN_IS(vdtypep, AssocArrayDType)) {
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: Replication to form "
                                                 << vdtypep->prettyDTypeNameQ() << " data type");
            }
            যদি (vdtypep && vdtypep->isString()) {
                iterateCheckString(nodep, "LHS", nodep->srcp(), BOTH);
            } অন্যথা {
                iterateCheckIntegralSelf(nodep, "LHS", nodep->srcp(), SELF, BOTH);
            }

            যদি ((vdtypep && vdtypep->isString()) || nodep->srcp()->isString()) {
                Ast_গ্ৰন্থি* ধ্রুৱ newp
                    = নূতন AstReplicateN{nodep->fileline(), nodep->srcp()->unlinkFrBack(),
                                        nodep->countp()->unlinkFrBack()};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
                ঘূৰাই;
            } অন্যথা {
                যদি (!constp) nodep->v3error("Replication value isn't a constant.");
                যদি (times == 0
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
        যদি (m_vup->final()) {
            যদি (!nodep->dtypep()->widthSized()) {
                // See also error in V3Number
                গ্ৰন্থি_For_Unsized_Warning(nodep)->v3warn(
                    WIDTHCONCAT, "Unsized numbers/parameters not allowed in replications.");
            }
        }
    }
    শূন্য visit(AstReplicateN* nodep) override {
        // Replicate with string
        assert_At_ৰাশিমালা(nodep);
        যদি (m_vup->prelim()) {
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckSizedSelf(nodep, "RHS", nodep->rhsp(), SELF, BOTH);
            V3Const::constifyParamsNoWarnEdit(nodep->rhsp());  // rhsp may change
            nodep->dtypeSetString();
        }
        যদি (m_vup->final()) {
            যদি (!nodep->dtypep()->widthSized()) {
                // See also error in V3Number
                গ্ৰন্থি_For_Unsized_Warning(nodep)->v3warn(
                    WIDTHCONCAT, "Unsized numbers/parameters not allowed in replications.");
            }
        }
    }
    শূন্য visit(Ast_গ্ৰন্থি_Dist_Biop* nodep) override {
        assert_At_ৰাশিমালা(nodep);
        যদি (m_vup->prelim()) {  // First stage evaluation
            iterateCheckSigned32(nodep, "seed", nodep->lhsp(), BOTH);
            iterateCheckSigned32(nodep, "RHS", nodep->rhsp(), BOTH);
        }
    }
    শূন্য visit(Ast_গ্ৰন্থি_Dist_Triop* nodep) override {
        যদি (m_vup->prelim()) {  // First stage evaluation
            iterateCheckSigned32(nodep, "seed", nodep->lhsp(), BOTH);
            iterateCheckSigned32(nodep, "RHS", nodep->rhsp(), BOTH);
            iterateCheckSigned32(nodep, "THS", nodep->thsp(), BOTH);
        }
    }
    তার্কিক streamImplicitUseAllowed(ধ্রুৱ Ast_গ্ৰন্থি_Stream* nodep) ধ্রুৱ {
        যদি (m_streamConcat) ঘূৰাই সঁচা;
        যদি (m_vup && m_vup->streamUse() != STREAM_USE_NONE) ঘূৰাই সঁচা;

        ধ্রুৱ Ast_গ্ৰন্থি* ধ্রুৱ backp = nodep->backp();
        যদি (!backp) ঘূৰাই সঁচা;  // Error elsewhere

        ঘূৰাই VN_IS(backp, গ্ৰন্থি_Assign);
    }
    শূন্য visit(Ast_গ্ৰন্থি_Stream* nodep) override {
        VL_RESTORER(m_streamConcat);
        // UINFOTREE(1, nodep, "stream-in vup" << m_vup, "stream-in ");
        assert_At_ৰাশিমালা(nodep);
        যদি (m_vup->prelim()) {
            m_streamConcat = সঁচা;
            iterateCheckSelf(nodep, "LHS", nodep->lhsp(), SELF, BOTH);
            m_streamConcat = মিছা;
            iterateCheckSelf(nodep, "RHS", nodep->rhsp(), SELF, BOTH);
            V3Const::constifyParamsEdit(nodep->rhsp());  // rhsp may change
            যদি (ধ্রুৱ AstConst* ধ্রুৱ constp = VN_CAST(nodep->rhsp(), Const)) {
                যদি (constp->toUInt() == 0) nodep->v3error("Slice size cannot be zero.");
            } অন্যথা {
                nodep->v3error("Slice size isn't a constant or basic data type.");
            }
            ধ্রুৱ Ast_গ্ৰন্থি_DType* ধ্রুৱ lhsDtypep = nodep->lhsp()->dtypep()->skipRefToEnump();
            যদি (VN_IS(lhsDtypep, DynArrayDType) || VN_IS(lhsDtypep, QueueDType)
                || (VN_IS(lhsDtypep, UnpackArrayDType) && lhsDtypep->isStreamableFixedAggregate())
                || (VN_IS(lhsDtypep, গ্ৰন্থি_UOr_Struct_DType)
                    && !VN_AS(lhsDtypep, গ্ৰন্থি_UOr_Struct_DType)->packed()
                    && lhsDtypep->isStreamableFixedAggregate())) {
                nodep->dtypeSetStream();
            } অন্যথা যদি (lhsDtypep->isCompound()) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: Stream operation on a variable of a type "
                                  << lhsDtypep->prettyDTypeNameQ());
                nodep->dtypeSetStream();
            } অন্যথা {
                nodep->dtypeSetBitOrLogicUnsized(
                    nodep->lhsp()->width(), nodep->lhsp()->widthMin(), VSigning::UNSIGNED,
                    Ast_গ্ৰন্থি_ৰাশিমালা::isFourstateOrNoDtype(nodep->lhsp(), nodep->rhsp()));
            }
        }
        যদি (m_vup->final()) {
            যদি (!streamImplicitUseAllowed(nodep)) {
                nodep->v3error(
                    "Streaming concatenation cannot be used in an implicitly cast context "
                    "(IEEE 1800-2023 11.4.14)\n"
                    << nodep->warnMore() << "... Suggest use a cast");
            }
            যদি (!nodep->dtypep()->widthSized()) {
                // See also error in V3Number
                গ্ৰন্থি_For_Unsized_Warning(nodep)->v3warn(
                    WIDTHCONCAT, "Unsized numbers/parameters not allowed in streams.");
            }
        }
    }
    শূন্য visit(AstRange* nodep) override {
        // Real: Not allowed
        // Signed: unsigned output, input either
        // Convert all range values to constants
        UINFO(6, "RANGE " << nodep);
        V3Const::constifyParamsEdit(nodep->leftp());  // May relink pointed to node
        V3Const::constifyParamsEdit(nodep->rightp());  // May relink pointed to node
        checkConstantOrReplace(nodep->leftp(), সঁচা,
                               "left side of bit range isn't a two-state constant"
                               " (IEEE 1800-2023 6.9.1)");
        checkConstantOrReplace(nodep->rightp(), সঁচা,
                               "right side of bit range isn't a two-state constant"
                               " (IEEE 1800-2023 6.9.1)");
        যদি (m_vup->prelim()) {
            // Don't need to iterate because V3Const already constified
            ধ্রুৱ পূর্ণাঙ্ক width = nodep->elementsConst();
            যদি (nodep->fromBracket() && nodep->বাম_Const() > nodep->ডান_Const()) {
                // From a C-Style '[size]' declaration, due to range handling a '[-1]'
                // will get transformed to look like a '[0:1]', instead report as error
                // now we know the datatype is used
                nodep->v3error("Size of range is '["
                               << (-width + 2)
                               << "]', must be positive integer (IEEE 1800-2023 7.4.2)");
            } অন্যথা যদি (width > (1 << 28)) {
                nodep->v3error("Width of bit range is huge; vector of over 1 billion bits: 0x"
                               << std::hex << width << std::dec);
            }
            // Note width() not set on range; use elementsConst()
            ধ্রুৱ তার্কিক inDeadModule = m_modep && m_modep->dead();
            // Suppress ASCRANGE in parameterized template modules whose parameter-dependent
            // ranges haven't been resolved yet, or when the type has no owning module
            // (e.g. moved to TypeTable during DepGraph resolution).
            ধ্রুৱ তার্কিক inParameterizedTemplate
                = m_modep && (m_modep->dead() || m_modep->parameterizedTemplate());
            ধ্রুৱ তার্কিক inTypeTable = !m_modep;
            ধ্রুৱ Ast_গ্ৰন্থি* basep = nodep->backp();
            যেতিয়ালৈ (VN_IS(basep, Range)) basep = basep->backp();
            যদি (nodep->ascending() && !VN_IS(basep, UnpackArrayDType)
                && !VN_IS(basep, Cell)  // For cells we warn when connecting pins
                && !m_paramsOnly  // Skip during parameter evaluation
                && !inDeadModule && !inParameterizedTemplate && !inTypeTable) {
                nodep->v3warn(ASCRANGE, "Ascending bit range vector: left < right of bit range: ["
                                            << nodep->বাম_Const() << ":" << nodep->ডান_Const()
                                            << "]");
            }
        }
    }

    শূন্য visit(AstSel* nodep) override {
        // Signed: always unsigned; Real: Not allowed
        // LSB is self-determined (IEEE 2012 11.5.1)
        // We also use SELs to shorten a signed constant etc, in this case they are signed.
        যদি (nodep->didWidth()) ঘূৰাই;
        assert_At_ৰাশিমালা(nodep);
        যদি (m_vup->prelim()) {
            UINFOTREE(9, nodep, "", "selWidth");
            userIterateAndNext(nodep->fromp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            userIterateAndNext(nodep->lsbp(), WidthVP{SELF, PRELIM}.p());
            checkCvtUS(nodep->fromp(), মিছা);
            iterateCheckSizedSelf(nodep, "Select LHS", nodep->fromp(), SELF, BOTH);
            পূর্ণাঙ্ক width = nodep->widthConst();
            যদি (width <= 0) {
                nodep->v3error("Width of bit extract must be positive (IEEE 1800-2023 11.5.1)");
                nodep->dtypeSetBit();
                ঘূৰাই;
            }
            UASSERT_OBJ(nodep->dtypep(), nodep, "dtype wasn't set");  // by V3WidthSel

            ধ্রুৱ তার্কিক isWriteSelect = nodep->fromp()->isLValue();
            // Suppress SELRANGE in parameterized template modules where
            // parameter-dependent widths haven't been resolved yet.
            ধ্রুৱ তার্কিক inParameterizedTemplate
                = m_modep && (m_modep->dead() || m_modep->parameterizedTemplate());

            যদি (VN_IS(nodep->lsbp(), Const) && nodep->msbConst() < nodep->lsbConst()) {
                // Likely impossible given above width check
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: left < right of bit extract: "  // LCOV_EXCL_LINE
                                  << nodep->msbConst() << "<" << nodep->lsbConst());
                width = (nodep->lsbConst() - nodep->msbConst() + 1);
                nodep->dtypeSetBitOrLogicSized(width, VSigning::UNSIGNED,
                                               nodep->fromp()->isFourstateOrNoDtype());
                nodep->widthConst(width);
                ঠেলো_Deletep(nodep->lsbp());
                nodep->lsbp()->replaceWith(নূতন AstConst{nodep->lsbp()->fileline(), 0});
            }
            // We're extracting, so just make sure the expression is at least wide enough.
            যদি (nodep->fromp()->width() < width && !inParameterizedTemplate) {
                // If it is not a lvalue, extend it
                যদি (!isWriteSelect) {
                    nodep->v3warn(SELRANGE, "Extracting " << width << " bits from only "
                                                          << nodep->fromp()->width()
                                                          << " bit number");
                    Ast_গ্ৰন্থি_DType* ধ্রুৱ subDTypep = nodep->findBitOrLogicDType(
                        width, width, nodep->fromp()->dtypep()->numeric(),
                        nodep->fromp()->dtypep()->isFourstate());
                    widthCheckSized(nodep, "errorless...", nodep->fromp(), subDTypep, EXTEND_EXP,
                                    মিছা /*noerror*/);
                } অন্যথা {
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
            পূর্ণাঙ্ক frommsb = nodep->fromp()->width() - 1;
            পূর্ণাঙ্ক fromlsb = 0;
            ধ্রুৱ পূর্ণাঙ্ক elw = nodep->declElWidth();  // Must adjust to tell user bit ranges
            যদি (nodep->declRange().ranged()) {
                frommsb = nodep->declRange().hiMaxSelect() * elw
                          + (elw - 1);  // Corrected for negative lsb
                fromlsb = nodep->declRange().lo() * elw;
            } অন্যথা {
                // nodep->v3fatalSrc("Should have been declRanged in V3WidthSel");
            }
            ধ্রুৱ পূর্ণাঙ্ক selwidth = V3_সংখ্যা::log2b(frommsb + 1 - 1) + 1;  // Width to address a bit
            Ast_গ্ৰন্থি_DType* ধ্রুৱ selwidthDTypep = nodep->findBitOrLogicDType(
                selwidth, selwidth, nodep->lsbp()->dtypep()->numeric(),
                nodep->lsbp()->dtypep()->isFourstate());
            userIterateAndNext(nodep->fromp(), WidthVP{SELF, FINAL}.p());
            userIterateAndNext(nodep->lsbp(), WidthVP{SELF, FINAL}.p());
            যদি (widthBad(nodep->lsbp(), selwidthDTypep) && nodep->lsbp()->width() != 32) {
                যদি (!nodep->fileline()->warnIsOff(V3ErrorCode::WIDTH)) {
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
            যদি (VN_IS(nodep->lsbp(), Const) && nodep->msbConst() > frommsb) {
                // See also warning in V3Const
                // We need to check here, because the widthCheckSized may silently
                // add another SEL which will lose the out-of-range check
                //
                // We don't want to trigger an error here if we are just
                // evaluating type sizes for a generate block condition. We
                // should only trigger the error if the out-of-range access is
                // actually generated.
                যদি (m_doGenerate) {
                    UINFO(5, "Selection index out of range inside generate");
                } অন্যথা যদি (!inParameterizedTemplate) {
                    যদি (nodep->declRange().ranged()) {
                        nodep->v3warn(SELRANGE, "Selection index out of range: "
                                                    << nodep->msbConst() << ":"
                                                    << nodep->lsbConst() << " outside " << frommsb
                                                    << ":" << fromlsb);
                    } অন্যথা {
                        nodep->v3warn(SELRANGE,
                                      "Selection "
                                          << nodep->msbConst() << ":" << nodep->lsbConst()
                                          << " performed on an object without declared range");
                    }
                    UINFO(1, "    Related node: " << nodep);
                }
                // Extend unless it's a lvalue,
                // because extending lvalue would lose write access.
                যদি (!isWriteSelect) {
                    // Extend it
                    ধ্রুৱ পূর্ণাঙ্ক extendTo = nodep->msbConst() + 1;
                    Ast_গ্ৰন্থি_DType* ধ্রুৱ subDTypep = nodep->findBitOrLogicDType(
                        extendTo, extendTo, nodep->fromp()->dtypep()->numeric(),
                        nodep->fromp()->dtypep()->isFourstate());
                    widthCheckSized(nodep, "errorless...", nodep->fromp(), subDTypep, EXTEND_EXP,
                                    মিছা /*noerror*/);
                }
            }
            // iterate FINAL is two blocks above
            //
            // If we have a width problem with GENERATE etc, this will reduce
            // it down and mask it, so we have no chance of finding a real
            // error in the future. So don't do this for them.
            যদি (!m_doGenerate) {
                // lsbp() must be self-determined, however for performance
                // we want the select to be truncated to fit within the
                // maximum select range, e.g. turn Xs outside of the select
                // into something fast which pulls from within the array.
                widthCheckSized(nodep, "Extract Range", nodep->lsbp(), selwidthDTypep, EXTEND_EXP,
                                মিছা /*NOWARN*/);
            }
            // UINFOTREE(9, nodep, "", "seldone");
        }
    }

    শূন্য visit(AstArraySel* nodep) override {
        // Signed/Real: Output signed iff LHS signed/real; binary operator
        // Note by contrast, bit extract selects are unsigned
        // LSB is self-determined (IEEE 2012 11.5.1)
        assert_At_ৰাশিমালা(nodep);
        যদি (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "Bit select", nodep->bitp(), SELF, BOTH);
            userIterateAndNext(nodep->fromp(), WidthVP{SELF, BOTH}.p());
            // Element of a whole array of interfaces
            যদি (VN_IS(nodep->fromp(), ConsArray) && isIfaceArrayDType(nodep->fromp()->dtypep())) {
                V3Const::constifyParamsEdit(nodep->bitp());  // May relink pointed to node
                যদি (ধ্রুৱ AstConst* ধ্রুৱ constp = VN_CAST(nodep->bitp(), Const)) {
                    ধ্রুৱ পূর্ণাঙ্ক bit = constp->toSInt();
                    Ast_গ্ৰন্থি_ৰাশিমালা* newp = newIfaceArrayElement(nodep->fromp(), bit);
                    যদি (!newp) {
                        // No such instance, so an error, unlike other out of range selects
                        ধ্রুৱ AstUnpackArrayDType* ধ্রুৱ arrp
                            = VN_AS(nodep->fromp()->dtypep()->skipRefp(), UnpackArrayDType);
                        nodep->v3error("Selection index out of range of interface array: "
                                       << bit + arrp->lo() << " outside " << arrp->hi() << ":"
                                       << arrp->lo() << " (IEEE 1800-2023 23.6)");
                        newp = newIfaceArrayElement(nodep->fromp(), 0);
                    }
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
                    ঘূৰাই;
                }
            }
            //
            পূর্ণাঙ্ক frommsb;
            পূর্ণাঙ্ক fromlsb;
            ধ্রুৱ Ast_গ্ৰন্থি_DType* ধ্রুৱ fromDtp = nodep->fromp()->dtypep()->skipRefp();
            যদি (ধ্রুৱ AstUnpackArrayDType* ধ্রুৱ adtypep = VN_CAST(fromDtp, UnpackArrayDType)) {
                frommsb = adtypep->hi();
                fromlsb = adtypep->lo();
                যদি (fromlsb > frommsb) std::swap(frommsb, fromlsb);
                // However, if the lsb<0 we may go negative, so need more bits!
                যদি (fromlsb < 0) frommsb += -fromlsb;
                nodep->dtypeFrom(adtypep->subDTypep());  // Need to strip off array reference
            } অন্যথা {
                // Note PackArrayDType doesn't use an ArraySel but a normal Sel.
                UINFO(1, "    Related dtype: " << fromDtp);
                nodep->v3fatalSrc("Array reference exceeds dimension of array");
                frommsb = fromlsb = 0;
            }
            ধ্রুৱ পূর্ণাঙ্ক selwidth = V3_সংখ্যা::log2b(frommsb + 1 - 1) + 1;  // Width to address a bit
            Ast_গ্ৰন্থি_DType* ধ্রুৱ selwidthDTypep = nodep->findBitOrLogicDType(
                selwidth, selwidth, nodep->bitp()->dtypep()->numeric(),
                nodep->bitp()->isFourstateOrNoDtype());
            যদি (widthBad(nodep->bitp(), selwidthDTypep) && nodep->bitp()->width() != 32) {
                nodep->v3widthWarn(selwidth, nodep->bitp()->width(),
                                   "Bit extraction of array["
                                       << frommsb << ":" << fromlsb << "] requires " << selwidth
                                       << " bit index, not " << nodep->bitp()->width()
                                       << (nodep->bitp()->width() != nodep->bitp()->widthMin()
                                               ? " or " + cvtToStr(nodep->bitp()->widthMin())
                                               : "")
                                       << " bits.");
                যদি (!nodep->fileline()->warnIsOff(V3ErrorCode::WIDTH)) {
                    UINFO(1, "    Related node: " << nodep);
                    UINFO(1, "    Related dtype: " << nodep->dtypep());
                }
            }
            যদি (!m_doGenerate) {
                // Must check bounds before adding a select that truncates the bound
                // Note we've already subtracted off LSB
                যদি (VN_IS(nodep->bitp(), Const)
                    && (VN_AS(nodep->bitp(), Const)->toSInt() > (frommsb - fromlsb)
                        || VN_AS(nodep->bitp(), Const)->toSInt() < 0)) {
                    // Suppress in dead/parameterized template modules
                    যদি (!(m_modep && (m_modep->dead() || m_modep->parameterizedTemplate()))) {
                        nodep->v3warn(SELRANGE,
                                      "Selection index out of range: "
                                          << (VN_AS(nodep->bitp(), Const)->toSInt() + fromlsb)
                                          << " outside " << frommsb << ":" << fromlsb);
                    }
                }
                widthCheckSized(nodep, "Extract Range", nodep->bitp(), selwidthDTypep, EXTEND_EXP,
                                মিছা /*NOWARN*/);
            }
        }
    }

    শূন্য visit(AstAssocSel* nodep) override {
        // Signed/Real: Output type based on array-declared type; binary operator
        assert_At_ৰাশিমালা(nodep);
        যদি (m_vup->prelim()) {
            ধ্রুৱ Ast_গ্ৰন্থি_DType* ধ্রুৱ fromDtp = nodep->fromp()->dtypep()->skipRefp();
            ধ্রুৱ AstAssocArrayDType* ধ্রুৱ adtypep = VN_CAST(fromDtp, AssocArrayDType);
            যদি (!adtypep) {
                UINFO(1, "    Related dtype: " << fromDtp);
                nodep->v3fatalSrc("Associative array reference is not to associative array");
            }
            iterateCheckTyped(nodep, "Associative select", nodep->bitp(), adtypep->চাবি_DTypep(),
                              BOTH);
            nodep->dtypeFrom(adtypep->subDTypep());
        }
    }

    শূন্য visit(AstAlias* nodep) override {
        যদি (!nodep->didWidthAndSet()) {
            userIterateAndNext(nodep->itemsp(), WidthVP{SELF, BOTH}.p());
        }

        ধ্রুৱ auto check_If_ৰাশিমালা_Ok = [এতত্](ধ্রুৱ Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ exprp) {
            যদি (VN_IS(exprp, VarXRef)) {
                exprp->v3error("Hierarchical reference used for net alias (IEEE 1800-2023 10.11)");
                ঘূৰাই মিছা;
            }

            ধ্রুৱ AstVarRef* ধ্রুৱ varRefp = VN_CAST(exprp, VarRef);
            যদি (!varRefp) {
                exprp->v3warn(
                    E_UNSUPPORTED,
                    "Unsupported: Operand of alias statement is not a variable reference");
                ঘূৰাই মিছা;
            }
            AstVar* ধ্রুৱ varp = varRefp->varp();
            যদি (!varp->isNet()) {
                exprp->v3error("Only nets are allowed in alias (IEEE 1800-2023 10.11): "
                               << varp->prettyNameQ());
                ঘূৰাই মিছা;
            }
            যদি (m_aliasedVars.find(varp) != m_aliasedVars.end()) {
                varRefp->v3error("Alias is specified more than once (IEEE 1800-2023 10.11): "
                                 << varp->prettyNameQ());
                ঘূৰাই মিছা;
            } অন্যথা {
                m_aliasedVars.ঢোকাও(varp);
            }
            ঘূৰাই সঁচা;
        };

        check_If_ৰাশিমালা_Ok(nodep->itemsp());
        Ast_গ্ৰন্থি_DType* ধ্রুৱ firstItemDtypep = nodep->itemsp()->dtypep();
        বাবে (Ast_গ্ৰন্থি* itemp = nodep->itemsp()->nextp(); itemp; itemp = itemp->nextp()) {
            check_If_ৰাশিমালা_Ok(VN_AS(itemp, গ্ৰন্থি_ৰাশিমালা));
            যদি (!firstItemDtypep->similarDType(itemp->dtypep())) {
                itemp->v3error("Incompatible data types of nets used for net alias. First operand "
                               "has the type "
                               << firstItemDtypep->prettyDTypeNameQ() << ", other has "
                               << itemp->dtypep()->prettyDTypeNameQ());
            }
        }
    }

    শূন্য visit(AstWildcardSel* nodep) override {
        // Signed/Real: Output type based on array-declared type; binary operator
        assert_At_ৰাশিমালা(nodep);
        যদি (m_vup->prelim()) {
            ধ্রুৱ Ast_গ্ৰন্থি_DType* ধ্রুৱ fromDtp = nodep->fromp()->dtypep()->skipRefp();
            ধ্রুৱ AstWildcardArrayDType* ধ্রুৱ adtypep = VN_CAST(fromDtp, WildcardArrayDType);
            যদি (!adtypep) {
                UINFO(1, "    Related dtype: " << fromDtp);
                nodep->v3fatalSrc("Wildcard array reference is not to wildcard array");
            }
            ধ্রুৱ AstBasicDType* ধ্রুৱ basicp = nodep->bitp()->dtypep()->skipRefp()->basicp();
            যদি (!basicp
                || (basicp->keyword() != VBasicDTypeKwd::STRING
                    && !basicp->keyword().isIntNumeric())) {
                nodep->v3error("Wildcard index must be integral (IEEE 1800-2023 7.8.1)");
            }
            iterateCheckTyped(nodep, "Wildcard associative select", nodep->bitp(),
                              adtypep->findStringDType(), BOTH);
            nodep->dtypeFrom(adtypep->subDTypep());
        }
    }

    শূন্য visit(AstSliceSel* nodep) override {
        // Always creates as output an unpacked array
        assert_At_ৰাশিমালা(nodep);
        যদি (m_vup->prelim()) {
            userIterateAndNext(nodep->fromp(), WidthVP{SELF, BOTH}.p());
            //
            // Array indices are always constant
            ধ্রুৱ Ast_গ্ৰন্থি_DType* ধ্রুৱ fromDtp = nodep->fromp()->dtypep()->skipRefp();
            ধ্রুৱ AstUnpackArrayDType* ধ্রুৱ adtypep = VN_CAST(fromDtp, UnpackArrayDType);
            যদি (!adtypep) {
                UINFO(1, "    Related dtype: " << fromDtp);
                nodep->v3fatalSrc("Packed array reference exceeds dimension of array");
            }
            // Build new array Dtype based on the original's base type, but with new bounds
            Ast_গ্ৰন্থি_DType* ধ্রুৱ newDtp
                = নূতন AstUnpackArrayDType{nodep->fileline(), adtypep->subDTypep(),
                                          নূতন AstRange{nodep->fileline(), nodep->declRange()}};
            v3Global.rootp()->typeTablep()->addTypesp(newDtp);
            nodep->dtypeFrom(newDtp);

            যদি (!m_doGenerate) {
                // Must check bounds before adding a select that truncates the bound
                // Note we've already subtracted off LSB
                ধ্রুৱ পূর্ণাঙ্ক subtracted = adtypep->declRange().lo();
                // Add subtracted value to get the original range
                ধ্রুৱ VNumRange declRange{nodep->declRange().hi() + subtracted,
                                          nodep->declRange().lo() + subtracted,
                                          nodep->declRange().ascending()};
                যদি ((declRange.hi() > adtypep->declRange().hi())
                    || declRange.lo() < adtypep->declRange().lo()) {
                    // Other simulators warn too
                    nodep->v3error("Slice selection index '" << declRange << "'"
                                                             << " outside data type's '"
                                                             << adtypep->declRange() << "'");
                    // Replaced by a placeholder value, so no element is out of range later
                    nodep->replaceWith(নূতন AstConsArray{
                        nodep->fileline(), newDtp,
                        নূতন AstConst{nodep->fileline(), AstConst::BitFalseErroring{}}});
                    VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
                    ঘূৰাই;
                } অন্যথা যদি ((declRange.ascending() != adtypep->declRange().ascending())
                           && declRange.hi() != declRange.lo()) {
                    nodep->v3error("Slice selection '"
                                   << declRange << "'"
                                   << " has reversed range order versus data type's '"
                                   << adtypep->declRange() << "'");
                }
            }
            // A slice of a whole array of interfaces is the array of the selected elements
            যদি (VN_IS(nodep->fromp(), ConsArray) && isIfaceArrayDType(nodep->fromp()->dtypep())) {
                AstConsArray* ধ্রুৱ fromp = VN_AS(nodep->fromp(), ConsArray);
                AstConsArray* ধ্রুৱ newp = নূতন AstConsArray{nodep->fileline(), newDtp};
                ধ্রুৱ VNumRange range = nodep->declRange();
                বাবে (পূর্ণাঙ্ক n = 0; n < range.elements(); ++n) {
                    newp->valuep(n, fromp->valuep(range.lo() + n)->clone_বৃক্ষ(মিছা));
                }
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
            }
        }
    }

    শূন্য visit(AstSelBit* nodep) override {
        // Just a quick check as after V3Param these nodes instead are AstSel's
        userIterateAndNext(nodep->fromp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        userIterateAndNext(nodep->bitp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        userIterateAndNext(nodep->thsp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        userIterateAndNext(nodep->attrp(), WidthVP{SELF, BOTH}.p());
        Ast_গ্ৰন্থি* ধ্রুৱ selp = V3Width::widthSelNoIterEdit(nodep);
        যদি (selp != nodep) {
            VL_DANGLING(nodep);
            userIterate(selp, m_vup);
            ঘূৰাই;
        }
        nodep->v3fatalSrc("AstSelBit should disappear after widthSel");
    }
    শূন্য visit(AstSelExtract* nodep) override {
        // Just a quick check as after V3Param these nodes instead are AstSel's
        userIterateAndNext(nodep->fromp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        userIterateAndNext(nodep->leftp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        userIterateAndNext(nodep->rightp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        userIterateAndNext(nodep->attrp(), WidthVP{SELF, BOTH}.p());
        Ast_গ্ৰন্থি* ধ্রুৱ selp = V3Width::widthSelNoIterEdit(nodep);
        যদি (selp != nodep) {
            nodep = nullptr;
            userIterate(selp, m_vup);
            ঘূৰাই;
        }
        nodep->v3fatalSrc("AstSelExtract should disappear after widthSel");
    }
    শূন্য visit(AstSelPlus* nodep) override {
        userIterateAndNext(nodep->fromp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        userIterateAndNext(nodep->bitp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        userIterateAndNext(nodep->widthp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        userIterateAndNext(nodep->attrp(), WidthVP{SELF, BOTH}.p());
        Ast_গ্ৰন্থি* ধ্রুৱ selp = V3Width::widthSelNoIterEdit(nodep);
        যদি (selp != nodep) {
            nodep = nullptr;
            userIterate(selp, m_vup);
            ঘূৰাই;
        }
        nodep->v3fatalSrc("AstSelPlus should disappear after widthSel");
    }
    শূন্য visit(AstSelMinus* nodep) override {
        userIterateAndNext(nodep->fromp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        userIterateAndNext(nodep->bitp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        userIterateAndNext(nodep->widthp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        userIterateAndNext(nodep->attrp(), WidthVP{SELF, BOTH}.p());
        Ast_গ্ৰন্থি* ধ্রুৱ selp = V3Width::widthSelNoIterEdit(nodep);
        যদি (selp != nodep) {
            nodep = nullptr;
            userIterate(selp, m_vup);
            ঘূৰাই;
        }
        nodep->v3fatalSrc("AstSelMinus should disappear after widthSel");
    }

    শূন্য visit(AstExtend* nodep) override {
        // Typically created by this process, so we know width from here down is correct.
        // Exception is extend added by V3WidthSel - those need iteration
        যদি (nodep->didWidthAndSet()) ঘূৰাই;
        userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
    }
    শূন্য visit(AstExtendS* nodep) override {
        // Typically created by this process, so we know width from here down is correct.
        // Exception is extend added by V3WidthSel - those need iteration
        যদি (nodep->didWidthAndSet()) ঘূৰাই;
        userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
    }
    শূন্য visit(AstConst* nodep) override {
        // The node got setup with the signed/real state of the node.
        // However a later operation may have changed the node->signed w/o changing
        // the number's sign.  So we don't: nodep->dtypeChgSigned(nodep->num().isSigned());
        যদি (nodep->didWidthAndSet()) ঘূৰাই;
        যদি (m_vup && m_vup->prelim()) {
            যদি (VN_IS(nodep->dtypep()->skipRefToEnump(), EnumDType)
                || VN_IS(nodep->dtypep()->skipRefp(), StructDType)
                || VN_IS(nodep->dtypep()->skipRefp(), PackArrayDType)) {
                // Assume this constant was properly casted earlier
                // (Otherwise it couldn't have an enum data type)
            } অন্যথা যদি (nodep->অঙ্ক().isNull()) {
                nodep->dtypep(nodep->findNullDType());
            } অন্যথা যদি (nodep->অঙ্ক().isString()) {
                nodep->dtypeSetString();
            } অন্যথা যদি (nodep->অঙ্ক().isDouble()) {
                nodep->dtypeChgWidth(nodep->অঙ্ক().width(), nodep->অঙ্ক().sized()
                                                               ? nodep->অঙ্ক().width()
                                                               : nodep->অঙ্ক().widthToFit());
            } অন্যথা {
                nodep->dtypeSetBitOrLogicUnsized(
                    nodep->অঙ্ক().width(),
                    nodep->অঙ্ক().sized() ? nodep->অঙ্ক().width() : nodep->অঙ্ক().widthToFit(),
                    nodep->dtypep()->numeric(), nodep->অঙ্ক().isAnyXZ());
            }
        }
        // We don't size the constant until we commit the widths, as need parameters
        // to remain unsized, and numbers to remain unsized to avoid backp() warnings
    }
    শূন্য visit(AstEmptyQueue* nodep) override {
        nodep->dtypeSetEmptyQueue();
        যদি (!VN_IS(nodep->backp(), Assign) && !VN_IS(nodep->backp(), Var)
            && !VN_IS(nodep->backp(), Arg)) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported/Illegal: empty queue ('{}') in this context");
        }
    }
    শূন্য visit(AstFell* nodep) override {
        assert_At_ৰাশিমালা(nodep);
        যদি (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ সঁচা);
            userIterate(nodep->sentreep(), nullptr);
            nodep->dtypeSetBit();
        }
    }
    শূন্য visit(AstFalling* nodep) override {
        assert_At_ৰাশিমালা(nodep);
        যদি (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ সঁচা);
            nodep->dtypeSetBit();
        }
    }
    শূন্য visit(AstFuture* nodep) override {
        assert_At_ৰাশিমালা(nodep);
        যদি (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ সঁচা);
            userIterate(nodep->sentreep(), nullptr);
            nodep->dtypeFrom(nodep->exprp());
        }
    }
    শূন্য visit(AstPast* nodep) override {
        assert_At_ৰাশিমালা(nodep);
        যদি (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ সঁচা);
            nodep->dtypeFrom(nodep->exprp());
            যদি (nodep->ticksp()) {
                iterateCheckSizedSelf(nodep, "Ticks", nodep->ticksp(), SELF, BOTH,
                                      /* deferUntyped */ সঁচা);
                V3Const::constifyParamsEdit(nodep->ticksp());  // ticksp may change
                ধ্রুৱ AstConst* ধ্রুৱ constp = VN_CAST(nodep->ticksp(), Const);
                যদি (!constp) {
                    nodep->v3error("$past tick value must be constant (IEEE 1800-2023 16.9.3)");
                    nodep->ticksp()->unlinkFrBack()->delete_বৃক্ষ();
                } অন্যথা যদি (constp->toSInt() < 1) {
                    constp->v3error("$past tick value must be >= 1 (IEEE 1800-2023 16.9.3)");
                    nodep->ticksp()->unlinkFrBack()->delete_বৃক্ষ();
                } অন্যথা যদি (V3AssertCommon::check_Sva_মান_Limit(nodep, constp, "$past tick value")) {
                    যদি (constp->toSInt() > 10) {
                        constp->v3warn(TICKCOUNT, "$past tick value of "
                                                      << constp->toSInt()
                                                      << " may have a large performance cost");
                    }
                }
            }
            userIterate(nodep->sentreep(), nullptr);
        }
    }
    শূন্য visit(AstSConsRep* nodep) override {
        // IEEE 1800-2023 16.9.2 -- consecutive repetition [*N], [*N:M], [*N:$], [+], [*]
        assert_At_ৰাশিমালা(nodep);
        যদি (m_vup->prelim()) {
            userIterateAndNext(nodep->exprp(), WidthVP{SELF, BOTH}.p());
            userIterateAndNext(nodep->countp(), WidthVP{SELF, BOTH}.p());
            V3Const::constifyParamsEdit(nodep->countp());
            ধ্রুৱ AstConst* ধ্রুৱ minConstp = VN_CAST(nodep->countp(), Const);
            যদি (!minConstp) {
                nodep->v3error("Consecutive repetition count must be constant expression"
                               " (IEEE 1800-2023 16.9.2)");
            } অন্যথা যদি (minConstp->toSInt() < 0) {
                nodep->v3error("Consecutive repetition count must be non-negative"
                               " (IEEE 1800-2023 16.9.2)");
            } অন্যথা যদি (!nodep->unbounded() && !nodep->maxCountp() && minConstp->toSInt() < 1) {
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: [*0] consecutive repetition");
            }
            যদি (nodep->maxCountp()) {
                userIterateAndNext(nodep->maxCountp(), WidthVP{SELF, BOTH}.p());
                V3Const::constifyParamsEdit(nodep->maxCountp());
                ধ্রুৱ AstConst* ধ্রুৱ maxConstp = VN_CAST(nodep->maxCountp(), Const);
                যদি (!maxConstp) {
                    nodep->v3error("Consecutive repetition max count must be constant"
                                   " expression (IEEE 1800-2023 16.9.2)");
                } অন্যথা যদি (minConstp && maxConstp->toSInt() < minConstp->toSInt()) {
                    nodep->v3error("Consecutive repetition max count must be >= min count"
                                   " (IEEE 1800-2023 16.9.2)");
                } অন্যথা যদি (maxConstp->toSInt() < 1) {
                    nodep->v3warn(E_UNSUPPORTED, "Unsupported: [*N:0] consecutive repetition"
                                                 " with zero max count");
                }
            }
            nodep->dtypeSetBit();
        }
    }
    শূন্য visit(AstPropAlways* nodep) override {
        // IEEE 1800-2023 16.12.11
        assert_At_ৰাশিমালা(nodep);
        যদি (m_vup->prelim()) {
            userIterateAndNext(nodep->propp(), WidthVP{SELF, BOTH}.p());
            Ast_গ্ৰন্থি_ৰাশিমালা* propp = nodep->propp();
            যদি (!VN_IS(nodep->loBoundp(), Unbounded)) {
                userIterateAndNext(nodep->loBoundp(), WidthVP{SELF, BOTH}.p());
                V3Const::constifyParamsEdit(nodep->loBoundp());
            }
            যদি (!VN_IS(nodep->hiBoundp(), Unbounded)) {
                userIterateAndNext(nodep->hiBoundp(), WidthVP{SELF, BOTH}.p());
                V3Const::constifyParamsEdit(nodep->hiBoundp());
            }
            ধ্রুৱ তার্কিক loUnbounded = VN_IS(nodep->loBoundp(), Unbounded);
            ধ্রুৱ তার্কিক hiUnbounded = VN_IS(nodep->hiBoundp(), Unbounded);
            // Strong always must be bounded (IEEE 1800-2023 16.12.11: "the range
            // for a strong always shall be bounded"). Weak always [m:$] is legal:
            // an unbounded upper bound imposes no end-of-trace obligation.
            যদি (nodep->isStrong() && (loUnbounded || hiUnbounded)) {
                Ast_গ্ৰন্থি* ধ্রুৱ boundp = loUnbounded ? nodep->loBoundp() : nodep->hiBoundp();
                boundp->v3error("s_always range must be bounded (IEEE 1800-2023 16.12.11)");
                nodep->dtypeSetBit();
                ঘূৰাই;
            }
            যদি (loUnbounded) {
                // Only the high bound may be $ (cycle_delay_const_range_expression).
                nodep->loBoundp()->v3error("always range low bound must be a constant expression"
                                           " (IEEE 1800-2023 16.12.11)");
                nodep->dtypeSetBit();
                ঘূৰাই;
            }
            ধ্রুৱ AstConst* ধ্রুৱ loConstp = VN_CAST(nodep->loBoundp(), Const);
            ধ্রুৱ AstConst* ধ্রুৱ hiConstp = VN_CAST(nodep->hiBoundp(), Const);
            যদি (!loConstp) {
                nodep->loBoundp()->v3error("always range low bound must be a constant expression"
                                           " (IEEE 1800-2023 16.12.11)");
            }
            যদি (!hiUnbounded && !hiConstp) {
                nodep->hiBoundp()->v3error("always range high bound must be a constant expression"
                                           " (IEEE 1800-2023 16.12.11)");
            }
            যদি (loConstp && loConstp->toSInt() < 0) {
                nodep->loBoundp()->v3error("always range low bound must be non-negative"
                                           " (IEEE 1800-2023 16.12.11)");
            } অন্যথা যদি (loConstp) {
                V3AssertCommon::check_Sva_মান_Limit(nodep->loBoundp(), loConstp,
                                                   "always range bound");
            }
            যদি (!hiUnbounded && hiConstp) {
                V3AssertCommon::check_Sva_মান_Limit(nodep->hiBoundp(), hiConstp,
                                                   "always range bound");
            }
            যদি (!hiUnbounded && loConstp && hiConstp && hiConstp->toSInt() < loConstp->toSInt()) {
                nodep->hiBoundp()->v3error("always range high bound must be >= low bound"
                                           " (IEEE 1800-2023 16.12.11)");
            }
            তার্কিক hasPropertyOp = propp->isMultiCycleSva();
            যদি (!hasPropertyOp) {
                propp->foreach([&](ধ্রুৱ Ast_গ্ৰন্থি* np) {
                    যদি (VN_IS(np, Implication) || VN_IS(np, Until) || VN_IS(np, PropSpec)) {
                        hasPropertyOp = সঁচা;
                    }
                });
            }
            যদি (hasPropertyOp) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: property/sequence operator inside bounded"
                              " always (IEEE 1800-2023 16.12.11)");
            }
            VL_DO_DANGLING(fixWidthReduce(propp), propp);
            nodep->dtypeSetBit();
        }
    }
    শূন্য visit(AstRising* nodep) override {
        assert_At_ৰাশিমালা(nodep);
        যদি (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ সঁচা);
            nodep->dtypeSetBit();
        }
    }
    শূন্য visit(AstRose* nodep) override {
        assert_At_ৰাশিমালা(nodep);
        যদি (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ সঁচা);
            userIterate(nodep->sentreep(), nullptr);
            nodep->dtypeSetBit();
        }
    }

    শূন্য visit(AstSampled* nodep) override {
        assert_At_ৰাশিমালা(nodep);
        যদি (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ সঁচা);
            nodep->dtypeFrom(nodep->exprp());
        }
    }

    শূন্য visit(AstSetuphold* nodep) override {
        File_সাৰি* ধ্রুৱ flp = nodep->fileline();
        AstAssignW* newp = nullptr;
        যদি (nodep->delrefp()) {
            newp = convertSetupholdToAssign(flp, nodep->refevp(), nodep->delrefp());
        }
        যদি (nodep->deldatap()) {
            যদি (!newp) {
                newp = convertSetupholdToAssign(flp, nodep->dataevp(), nodep->deldatap());
            } অন্যথা {
                newp->addNextHere(
                    convertSetupholdToAssign(flp, nodep->dataevp(), nodep->deldatap()));
            }
        }
        যদি (!newp) {
            VL_DO_DANGLING(ঠেলো_Deletep(nodep->unlinkFrBack()), nodep);
            ঘূৰাই;
        }
        nodep->replaceWith(নূতন AstAlways{newp});
        VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
    }

    AstAssignW* convertSetupholdToAssign(File_সাৰি* ধ্রুৱ flp, Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ evp,
                                         Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ delp) {
        Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ lhsp = delp->clone_বৃক্ষ_Pure(মিছা);
        Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ rhsp = evp->clone_বৃক্ষ_Pure(মিছা);
        UASSERT_OBJ(VN_IS(lhsp, গ্ৰন্থি_Var_Ref) || VN_IS(lhsp, গ্ৰন্থি_Pre_Sel), lhsp,
                    "Incorrect reference in a timing check");
        যদি (Ast_গ্ৰন্থি_Var_Ref* varRefp = VN_CAST(lhsp, গ্ৰন্থি_Var_Ref)) {
            যদি (varRefp->varp()->direction() == VDirection::প্ৰবেশ) {
                VL_DO_DANGLING(ঠেলো_Deletep(lhsp), lhsp);
                VL_DO_DANGLING(ঠেলো_Deletep(rhsp), rhsp);
                ঘূৰাই nullptr;
            }
            varRefp->access(VAccess::WRITE);
        }
        যদি (Ast_গ্ৰন্থি_Pre_Sel* selp = VN_CAST(lhsp, গ্ৰন্থি_Pre_Sel)) {
            যদি (Ast_গ্ৰন্থি_Var_Ref* varRefp = VN_CAST(selp->fromp(), গ্ৰন্থি_Var_Ref)) {
                যদি (varRefp->varp()->direction() == VDirection::প্ৰবেশ) {
                    VL_DO_DANGLING(ঠেলো_Deletep(lhsp), lhsp);
                    VL_DO_DANGLING(ঠেলো_Deletep(rhsp), rhsp);
                    ঘূৰাই nullptr;
                }
                varRefp->access(VAccess::WRITE);
            }
        }
        ঘূৰাই নূতন AstAssignW{flp, lhsp, rhsp};
    }

    শূন্য visit(AstStable* nodep) override {
        assert_At_ৰাশিমালা(nodep);
        যদি (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ সঁচা);
            userIterate(nodep->sentreep(), nullptr);
            nodep->dtypeSetBit();
        }
    }
    শূন্য visit(AstSteady* nodep) override {
        assert_At_ৰাশিমালা(nodep);
        যদি (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ সঁচা);
            nodep->dtypeSetBit();
        }
    }

    শূন্য visit(Ast_Stmt_ৰাশিমালা* nodep) override {
        assertAtStatement(nodep);
        userIterateAndNext(nodep->exprp(), WidthVP{SELF, BOTH}.p());
    }

    শূন্য visit(AstImplication* nodep) override {
        m_seqUnsupp = nodep;
        assert_At_ৰাশিমালা(nodep);
        যদি (m_vup->prelim()) {
            iterateCheckBool(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckBool(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtypeSetBit();
        }
    }

    শূন্য visit(AstUntil* nodep) override {
        assert_At_ৰাশিমালা(nodep);
        যদি (m_vup->prelim()) {
            iterateCheckBool(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckBool(nodep, "RHS", nodep->rhsp(), BOTH);
            // Coerce unsized constant operands (e.g. literal 0/1) to actual
            // 1-bit width; iterateCheckBool keeps widthMin-fitting unsized
            // constants at their nominal 32-bit width, which trips the
            // downstream NFA lowering's Log* chains in V3AssertNfa.
            যদি (nodep->lhsp()->width() != 1) fixWidthReduce(nodep->lhsp());
            যদি (nodep->rhsp()->width() != 1) fixWidthReduce(nodep->rhsp());
            nodep->dtypeSetBit();
        }
    }
    শূন্য visit(AstAbortOn* nodep) override { visitAbortProp(nodep); }

    শূন্য visit(AstRand* nodep) override {
        assert_At_ৰাশিমালা(nodep);
        যদি (m_vup->prelim()) {
            যদি (nodep->urandom()) {
                nodep->dtypeSetUInt32();  // Says the spec
            } অন্যথা {
                nodep->dtypeSetInt();  // Says the spec
            }
            যদি (nodep->seedp()) iterateCheckSigned32(nodep, "seed", nodep->seedp(), BOTH);
        }
    }
    শূন্য visit(AstSEventually* nodep) override {
        যদি (v3Global.opt.timing().isSetFalse() || !v3Global.opt.timing().isSetTrue()) {
            nodep->v3warn(E_NOTIMING, "s_eventually requires --timing");
            nodep->replaceWith(নূতন AstConst{nodep->fileline(), AstConst::Widthed_মান{}, 1, 0});
            VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
            ঘূৰাই;
        }
        যদি (m_vup->prelim()) {
            iterateCheckBool(nodep, "exprp", nodep->exprp(), BOTH);
            nodep->dtypeSetBit();
        }
    }
    শূন্য visit(AstSGotoRep* nodep) override {
        assert_At_ৰাশিমালা(nodep);
        যদি (m_vup->prelim()) {
            iterateCheckBool(nodep, "exprp", nodep->exprp(), BOTH);
            userIterateAndNext(nodep->countp(), WidthVP{SELF, BOTH}.p());
            যদি (nodep->maxCountp()) {
                widthCheckGotoRepRange(nodep, "Goto");
            } অন্যথা {
                width_Check_Rep_গুনতি(nodep);
            }
            nodep->dtypeSetBit();
        }
    }
    শূন্য visit(AstSNonConsRep* nodep) override {
        assert_At_ৰাশিমালা(nodep);
        যদি (m_vup->prelim()) {
            iterateCheckBool(nodep, "exprp", nodep->exprp(), BOTH);
            userIterateAndNext(nodep->countp(), WidthVP{SELF, BOTH}.p());
            যদি (nodep->maxCountp()) {
                widthCheckGotoRepRange(nodep, "Nonconsecutive");
            } অন্যথা {
                width_Check_Rep_গুনতি(nodep);
            }
            nodep->dtypeSetBit();
        }
    }
    template <typename T_Rep>
    শূন্য width_Check_Rep_গুনতি(T_Rep* nodep) {
        V3Const::constifyParamsNoWarnEdit(nodep->countp());
        ধ্রুৱ AstConst* ধ্রুৱ constp = VN_CAST(nodep->countp(), Const);
        যদি (!constp) {
            nodep->v3error("Repetition count is not an elaboration-time constant"
                           " (IEEE 1800-2023 16.9.2)");
        } অন্যথা যদি (constp->dtypep()->isSigned() && constp->অঙ্ক().isNegative()) {
            nodep->v3error("Repetition count must be non-negative"
                           " (IEEE 1800-2023 16.9.2)");
        } অন্যথা যদি (constp->অঙ্ক().mostSetBitP1() > 31) {
            nodep->v3warn(
                E_UNSUPPORTED,
                "Unsupported: SVA repetition count exceeds implementation limit of 2147483647");
        } অন্যথা যদি (constp->isZero()) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: zero repetition count"
                                         " (IEEE 1800-2023 16.9.2)");
        }
    }
    // IEEE 1800-2023 16.9.2 range-form bound validation for goto/nonconsec.
    // Parent accessors are re-fetched after constifyParamsEdit because that
    // call can replace the node in-tree (Lesson: AstSConsRep visitor pattern).
    // Templated to cover AstSGotoRep and AstSNonConsRep uniformly.
    template <typename T_Rep>
    শূন্য widthCheckGotoRepRange(T_Rep* nodep, ধ্রুৱ ৱর্ণ* kind) {
        userIterateAndNext(nodep->maxCountp(), WidthVP{SELF, BOTH}.p());
        V3Const::constifyParamsEdit(nodep->countp());
        V3Const::constifyParamsEdit(nodep->maxCountp());
        ধ্রুৱ AstConst* ধ্রুৱ minConstp = VN_CAST(nodep->countp(), Const);
        ধ্রুৱ AstConst* ধ্রুৱ maxConstp = VN_CAST(nodep->maxCountp(), Const);
        যদি (!minConstp || !maxConstp) {
            nodep->v3error(std::string{kind}
                           + " repetition range bounds must be constant expressions"
                             " (IEEE 1800-2023 16.9.2)");
            ঘূৰাই;
        }
        যদি (minConstp->toSInt() < 0) {
            nodep->v3error(std::string{kind}
                           + " repetition range min count must be non-negative"
                             " (IEEE 1800-2023 16.9.2)");
            ঘূৰাই;
        }
        যদি (maxConstp->toSInt() < minConstp->toSInt()) {
            nodep->v3error(std::string{kind}
                           + " repetition range max count must be >= min count"
                             " (IEEE 1800-2023 16.9.2)");
            ঘূৰাই;
        }
        যদি (minConstp->isZero()) {
            nodep->v3warn(E_UNSUPPORTED, std::string{"Unsupported: zero min count in "} + kind
                                             + " repetition range (IEEE 1800-2023 16.9.2)");
        }
    }
    শূন্য visit(AstSThroughout* nodep) override {
        m_hasSExpr = সঁচা;
        assert_At_ৰাশিমালা(nodep);
        যদি (m_vup->prelim()) {
            // lhsp is a boolean expression, not a sequence -- clear m_underSExpr temporarily
            VL_RESTORER(m_underSExpr);
            m_underSExpr = মিছা;
            iterateCheckBool(nodep, "lhsp", nodep->lhsp(), BOTH);
            m_underSExpr = সঁচা;
            iterate(nodep->rhsp());
            nodep->dtypeSetBit();
        }
    }
    শূন্য visit(AstSExpr* nodep) override {
        VL_RESTORER(m_underSExpr);
        m_underSExpr = সঁচা;
        m_hasSExpr = সঁচা;
        assert_At_ৰাশিমালা(nodep);
        যদি (m_vup->prelim()) {
            যদি (nodep->preExprp()) {
                iterateCheckBool(nodep, "preExprp", nodep->preExprp(), BOTH);
            }
            iterate(nodep->delayp());
            iterateCheckBool(nodep, "exprp", nodep->exprp(), BOTH);
            nodep->dtypeSetBit();
        }
    }
    শূন্য visit(AstSClocked* nodep) override {
        VL_RESTORER(m_underSExpr);
        m_underSExpr = সঁচা;
        m_hasSExpr = সঁচা;
        assert_At_ৰাশিমালা(nodep);
        যদি (m_vup->prelim()) {
            // Width each clock expression directly; the senitem chain is hoisted to
            // the assertion clock by V3AssertNfa, where it is fully processed.
            বাবে (AstSenItem* senp = nodep->sensesp(); senp;
                 senp = VN_CAST(senp->nextp(), SenItem)) {
                userIterateAndNext(senp->sensp(), WidthVP{SELF, BOTH}.p());
            }
            iterateCheckBool(nodep, "exprp", nodep->exprp(), BOTH);
            nodep->dtypeSetBit();
        }
    }
    শূন্য visit(AstURandomRange* nodep) override {
        assert_At_ৰাশিমালা(nodep);
        যদি (m_vup->prelim()) {
            nodep->dtypeSetUInt32();  // Says the spec
            Ast_গ্ৰন্থি_DType* ধ্রুৱ expDTypep = nodep->findUInt32DType();
            userIterateAndNext(nodep->lhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            userIterateAndNext(nodep->rhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            iterateCheck(nodep, "LHS", nodep->lhsp(), SELF, FINAL, expDTypep, EXTEND_EXP);
            iterateCheck(nodep, "RHS", nodep->rhsp(), SELF, FINAL, expDTypep, EXTEND_EXP);
        }
    }
    শূন্য visit(AstUnbounded* nodep) override {
        nodep->dtypeSetInt();  // Used in int context
        যদি (VN_IS(nodep->backp(), IsUnbounded)) ঘূৰাই;  // Ok, leave
        যদি (VN_IS(nodep->backp(), BracketArrayDType)) ঘূৰাই;  // Ok, leave
        যদি (VN_IS(nodep->backp(), InsideRange)) ঘূৰাই;  // Ok, leave
        যদি (ধ্রুৱ auto* ধ্রুৱ varp = VN_CAST(nodep->backp(), Var)) {
            যদি (varp->isParam()) ঘূৰাই;  // Ok, leave
        }
        Ast_গ্ৰন্থি* backp = nodep->backp();
        যদি (VN_IS(backp, Sub)) backp = backp->backp();
        যদি (ধ্রুৱ auto* ধ্রুৱ selp = VN_CAST(backp, SelExtract)) {
            যদি (VN_IS(selp->fromp()->dtypep()->skipRefp(), QueueDType)) ঘূৰাই;
        }
        যদি (ধ্রুৱ auto* ধ্রুৱ selp = VN_CAST(backp, SelBit)) {
            যদি (VN_IS(selp->fromp()->dtypep()->skipRefp(), QueueDType)) ঘূৰাই;
        }
        // queue_slice[#:$] and queue_bitsel[$] etc handled in V3WidthSel
        nodep->v3warn(E_UNSUPPORTED,
                      "Unsupported: Unbounded ('$') outside of queue or string operations");
    }
    শূন্য visit(AstInferredDisable* nodep) override {
        assert_At_ৰাশিমালা(nodep);
        যদি (m_vup->prelim()) nodep->dtypeSetBit();
    }
    শূন্য visit(AstIsUnbounded* nodep) override {
        assert_At_ৰাশিমালা(nodep);
        যদি (m_vup->prelim()) {
            userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
            nodep->dtypeSetBit();
        }
    }
    শূন্য visit(AstCExpr* nodep) override {
        // Inserted by V3Width only so we know has been resolved
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
    }
    শূন্য visit(AstCExprUser* nodep) override {
        // Give it the size the user wants.
        যদি (m_vup && m_vup->prelim()) {
            nodep->dtypeSetBitUnsized(32, 1, VSigning::UNSIGNED);  // We don't care
            // All arguments seek their natural sizes
            userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
        }
        যদি (m_vup->final()) {
            Ast_গ্ৰন্থি_DType* ধ্রুৱ expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
            nodep->dtypep(expDTypep);  // Assume user knows the rules; go with the flow
            যদি (nodep->width() > 64) {
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: $c can't generate wider than 64 bits");
            }
        }
    }
    শূন্য visit(AstCLog2* nodep) override {
        assert_At_ৰাশিমালা(nodep);
        যদি (m_vup->prelim()) iterateCheckSizedSelf(nodep, "LHS", nodep->lhsp(), SELF, BOTH);
    }
    // Width a 'weight' coverage option as the signed 'int' member it sets (IEEE 1800-2023
    // 19.10); a weight is a non-negative integral value, constant for type_option (19.7)
    template <typename T_Option>
    শূন্য widthCoverWeight(T_Option* nodep) {
        iterateCheckSigned32(nodep, "weight", nodep->valuep(), BOTH);
        V3Const::constifyEdit(nodep->valuep());
        Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ valuep = nodep->valuep();
        ধ্রুৱ std::string optName
            = std::string{nodep->typeOption() ? "type_option" : "option"} + ".weight";
        যদি (ধ্রুৱ AstConst* ধ্রুৱ constp = VN_CAST(valuep, Const)) {
            যদি (!constp->অঙ্ক().isNegative()) ঘূৰাই;
            valuep->v3error("Coverage option '" << optName << "' is set to negative value '"
                                                << constp->toSInt()
                                                << "'; weights must be non-negative"
                                                   " (IEEE 1800-2023 19.7)");
        } অন্যথা যদি (nodep->typeOption()) {
            valuep->v3error("Coverage option '" << optName
                                                << "' requires a constant expression"
                                                   " (IEEE 1800-2023 19.7.1)");
        } অন্যথা {
            ঘূৰাই;
        }
        // Continue with the default weight
        valuep->replaceWith(নূতন AstConst{valuep->fileline(), AstConst::Signed32{}, 1});
        VL_DO_DANGLING(ঠেলো_Deletep(valuep), valuep);
    }
    // Width an integral coverage option V3Covergroup reads, folding a constant expression
    template <typename T_Option>
    শূন্য widthCoverInt(T_Option* nodep) {
        userIterateAndNext(nodep->valuep(), WidthVP{SELF, BOTH}.p());
        V3Const::constifyEdit(nodep->valuep());
    }
    শূন্য visit(AstCgOptionAssign* nodep) override {
        // Recursive function widthing can reach a covergroup constructor without first visiting
        // its class, so find the owning covergroup structurally instead of using visit context.
        AstClass* ধ্রুৱ cgClassp = m_containingClassFinder.find(nodep);
        UASSERT_OBJ(cgClassp && cgClassp->isCovergroup(), nodep,
                    "Covergroup option is not under a covergroup class");

        // V3Covergroup stores the weight into the covergroup's option or type_option
        যদি (nodep->optType() == VCoverOptionType::WEIGHT) {
            widthCoverWeight(nodep);
            ঘূৰাই;
        }
        // V3Covergroup takes a constant option.at_least as the default for coverpoints/crosses
        যদি (nodep->optType() == VCoverOptionType::AT_LEAST) {
            widthCoverInt(nodep);
            ঘূৰাই;
        }

        // Extract covergroup option values and store in AstClass before deleting.
        যদি (nodep->optType() == VCoverOptionType::AUTO_BIN_MAX) {
            // By V3Width time, V3Param has already folded any parameter references.
            // If the value is still not a constant, it is a runtime expression - emit error.
            যদি (AstConst* constp = VN_CAST(nodep->valuep(), Const)) {
                cgClassp->cgAutoBinMax(constp->toSInt());
                UINFO(6, "  Covergroup " << cgClassp->name()
                                         << " option.auto_bin_max = " << constp->toSInt() << endl);
            } অন্যথা {
                nodep->valuep()->v3warn(COVERIGN, "Ignoring unsupported: non-constant "
                                                  "'option.auto_bin_max'; using default value");
            }
        } অন্যথা যদি (nodep->optType() == VCoverOptionType::MERGE_INSTANCES) {
            // V3Covergroup stores it into type_option; as a type option it is constant
            // (IEEE 1800-2023 19.7.1)
            iterateCheckBool(nodep, "merge_instances", nodep->valuep(), BOTH);
            V3Const::constifyEdit(nodep->valuep());
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ valuep = nodep->valuep();
            যদি (!VN_IS(valuep, Const)) {
                valuep->v3error("Coverage option 'type_option.merge_instances' requires a"
                                " constant expression (IEEE 1800-2023 19.7.1)");
                valuep->replaceWith(নূতন AstConst{valuep->fileline(), AstConst::BitFalse{}});
                VL_DO_DANGLING(ঠেলো_Deletep(valuep), valuep);
            }
            ঘূৰাই;
        } অন্যথা যদি (nodep->optType() == VCoverOptionType::GET_INST_COVERAGE) {
            // V3Covergroup stores it into option, as the constructor evaluates it
            iterateCheckBool(nodep, "get_inst_coverage", nodep->valuep(), BOTH);
            ঘূৰাই;
        } অন্যথা যদি (nodep->optType() == VCoverOptionType::DISTRIBUTE_FIRST) {
            // A bins 'with' filter applies before the values are distributed to the bins
            // (IEEE 1800-2023 19.5.1.1)
            ধ্রুৱ AstConst* ধ্রুৱ constp = VN_CAST(nodep->valuep(), Const);
            যদি (!constp || !constp->অঙ্ক().isEqZero()) {
                nodep->v3warn(COVERIGN, "Ignoring unsupported: 'type_option.distribute_first';"
                                        " 'with' filters apply before values are distributed"
                                        " to bins");
            }
        }
        // Add more options here as needed (goal, per_instance, comment)

        // Delete the assignment node (we've extracted the value)
        VL_DO_DANGLING(ঠেলো_Deletep(nodep->unlinkFrBack()), nodep);
    }
    শূন্য visit(AstCoverOption* nodep) override {
        যদি (nodep->optType() == VCoverOptionType::WEIGHT) {
            widthCoverWeight(nodep);
            ঘূৰাই;
        }
        যদি (nodep->optType() == VCoverOptionType::AT_LEAST
            || nodep->optType() == VCoverOptionType::AUTO_BIN_MAX) {
            widthCoverInt(nodep);
            ঘূৰাই;
        }
        userIterateAndNext(nodep->valuep(), WidthVP{SELF, BOTH}.p());
    }
    শূন্য visit(AstCoverCross* nodep) override {
        userIterateAndNext(nodep->itemsp(), nullptr);
        যদি (nodep->iffp()) iterateCheckBool(nodep, "iff condition", nodep->iffp(), BOTH);
        userIterateAndNext(nodep->optionsp(), nullptr);
        userIterateAndNext(nodep->binsp(), nullptr);
    }
    শূন্য visit(AstCoverCrossBin* nodep) override {
        userIterateAndNext(nodep->selectp(), nullptr);
        যদি (nodep->iffp()) iterateCheckBool(nodep, "iff condition", nodep->iffp(), BOTH);
    }
    শূন্য visit(AstCoverpoint* nodep) override {
        // The coverpoint expression is self-determined (IEEE 1800-2023 19.5).  Width it
        // with a context so a bit/part-select (AstSel) is sized here; otherwise it would
        // reach assertAtExpr() with m_vup==null and fail as an internal error.
        userIterateAndNext(nodep->exprp(), WidthVP{SELF, BOTH}.p());
        // Bin values compare against the coverpoint expression
        userIterateAndNext(nodep->binsp(), WidthVP{nodep->exprp()->dtypep(), BOTH}.p());
        যদি (nodep->iffp()) iterateCheckBool(nodep, "iff condition", nodep->iffp(), BOTH);
        userIterateAndNext(nodep->optionsp(), nullptr);
    }
    শূন্য widthCovergroupRanges(Ast_গ্ৰন্থি* rangesp, পূর্ণাঙ্ক fillWidth) {
        // Bin range/value entries are self-determined expressions (IEEE 1800-2023
        // 19.5).  Width each plain single-value entry self-determined so a referenced
        // parameter acquires a dtype, then constify so the reference folds to the AstConst
        // value that V3Covergroup requires.  AstInsideRange entries fold their own bounds in
        // visit(AstInsideRange).
        // '0/'1 entries then fill to fillWidth, the coverpoint width (IEEE 1800-2023 19.5.7).
        ধ্রুৱ auto fill = [&](Ast_গ্ৰন্থি* nodep) {
            যদি (!fillWidth) ঘূৰাই;
            Ast_গ্ৰন্থি_ৰাশিমালা* exprp = VN_AS(nodep, গ্ৰন্থি_ৰাশিমালা);
            fixAutoExtend(exprp /*ref*/, fillWidth);
        };
        বাবে (Ast_গ্ৰন্থি *nextp, *itemp = rangesp; itemp; itemp = nextp) {
            nextp = itemp->nextp();
            যদি (AstInsideRange* ধ্রুৱ rangep = VN_CAST(itemp, InsideRange)) {
                userIterate(rangep, nullptr);
                fill(rangep->lhsp());
                fill(rangep->rhsp());
            } অন্যথা যদি (VN_IS(itemp, CoverWith)) {
                userIterate(itemp, m_vup);
            } অন্যথা {
                itemp = userIterateSubtreeReturnEdits(itemp, WidthVP{SELF, BOTH}.p());
                fill(V3Const::constifyEdit(itemp));
            }
        }
    }
    শূন্য visit(AstCoverBinsof* nodep) override {
        userIterateAndNext(nodep->pointp(), nullptr);
        widthCovergroupRanges(nodep->rangesp(), 0);
    }
    শূন্য visit(AstCoverBin* nodep) override {
        // No m_vup for a bin directly in a covergroup body (unsupported, already warned)
        widthCovergroupRanges(nodep->rangesp(), m_vup ? m_vup->dtypep()->width() : 0);
        যদি (nodep->iffp()) iterateCheckBool(nodep, "iff condition", nodep->iffp(), BOTH);
        যদি (nodep->arraySizep()) {
            // The size of 'bins b[N]' or 'bins auto[N]' is a self-determined expression, which
            // V3Covergroup checks once folded
            userIterateAndNext(nodep->arraySizep(), WidthVP{SELF, BOTH}.p());
            V3Const::constifyEdit(nodep->arraySizep());  // arraySizep may change
        }
        userIterateAndNext(nodep->transp(), m_vup);
    }
    শূন্য visit(AstCoverWith* nodep) override {
        // The candidate value 'item' has the coverpoint's type (IEEE 1800-2023 19.5.1.1)
        userIterateAndNext(nodep->itemp(), nullptr);
        যদি (!nodep->itemp()->dtypeSkipRefp()->isIntegralOrPacked()) {
            nodep->v3error("Bin 'with' filters are not allowed on a coverpoint of a "
                           "non-integral expression (IEEE 1800-2023 19.5.1.1)");
        }
        যদি (!VN_IS(nodep->subp(), CoverpointRef)) {
            widthCovergroupRanges(nodep->subp(), m_vup->dtypep()->width());
        }
        widthCoverFilter(nodep->filterp(), "Bin");
    }
    শূন্য visit(AstCoverCrossWith* nodep) override {
        userIterateAndNext(nodep->selectp(), nullptr);
        // Each cross item has the type of its coverpoint (IEEE 1800-2023 19.6.1.3)
        userIterateAndNext(nodep->varsp(), nullptr);
        widthCoverFilter(nodep->filterp(), "Cross bin");
        যদি (Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ countp = nodep->countp()) {
            যদি (VN_IS(countp, Unbounded)) {
                countp->dtypeSetInt();  // 'matches $'
            } অন্যথা {
                // The count of 'matches' is self-determined, which V3Covergroup checks once folded
                userIterateAndNext(countp, WidthVP{SELF, BOTH}.p());
                V3Const::constifyEdit(nodep->countp());  // countp may change
            }
        }
    }
    // A 'with' filter is true for a nonzero value (IEEE 1800-2023 12.4), of a type assignment
    // compatible with an integral type (19.5.1.1); 'whatp' names its bins
    শূন্য widthCoverFilter(Ast_গ্ৰন্থি_ৰাশিমালা* filterp, ধ্রুৱ ৱর্ণ* whatp) {
        filterp = VN_AS(userIterateSubtreeReturnEdits(filterp, WidthVP{SELF, BOTH}.p()), গ্ৰন্থি_ৰাশিমালা);
        File_সাৰি* ধ্রুৱ fl = filterp->fileline();
        ধ্রুৱ Ast_গ্ৰন্থি_DType* ধ্রুৱ resultp = filterp->dtypep()->skipRefp();
        যদি (resultp->isDouble()) {
            VNRelinker relinker;
            filterp->unlinkFrBack(&relinker);
            relinker.relink(
                নূতন AstNeqD{fl, filterp, নূতন AstConst{fl, AstConst::RealDouble{}, 0.0}});
        } অন্যথা যদি (!resultp->isIntegralOrPacked()) {
            filterp->v3error(whatp << " 'with' filter must be assignment compatible with an "
                                      "integral type, not "
                                   << resultp->prettyDTypeNameQ() << " (IEEE 1800-2023 19.5.1.1)");
            filterp->replaceWith(নূতন AstConst{fl, AstConst::BitFalse{}});
            VL_DO_DANGLING(ঠেলো_Deletep(filterp), filterp);
        } অন্যথা {
            fixWidthReduce(filterp);
        }
    }
    শূন্য visit(AstCoverTransSet* nodep) override { userIterateAndNext(nodep->itemsp(), m_vup); }
    শূন্য visit(AstCoverTransItem* nodep) override {
        widthCovergroupRanges(nodep->valuesp(), m_vup ? m_vup->dtypep()->width() : 0);
    }
    শূন্য visit(AstPow* nodep) override {
        // Pow is special, output sign only depends on LHS sign, but
        // function result depends on both signs
        // RHS is self-determined (IEEE)
        // Real if either side is real (as with AstAdd)
        assert_At_ৰাশিমালা(nodep);
        যদি (m_vup->prelim()) {
            userIterateAndNext(nodep->lhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            userIterateAndNext(nodep->rhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            যদি (nodep->lhsp()->isDouble() || nodep->rhsp()->isDouble()) {
                spliceCvtD(nodep->lhsp());
                spliceCvtD(nodep->rhsp());
                VL_DO_DANGLING(replaceWithDVersion(nodep), nodep);
                ঘূৰাই;
            }
            checkCvtUS(nodep->lhsp(), মিছা);
            iterateCheckSizedSelf(nodep, "RHS", nodep->rhsp(), SELF, BOTH);
            nodep->dtypeFrom(nodep->lhsp());
            যদি (!nodep->isFourstateOrNoDtype()
                && (nodep->rhsp()->isFourstateOrNoDtype()
                    || (nodep->rhsp()->isSigned() && !nodep->lhsp()->isNeqZero()
                        && !(VN_IS(nodep->rhsp(), Const)
                             && !VN_AS(nodep->rhsp(), Const)->অঙ্ক().isNegative())))) {
                // Change dtype to four-state if it may be 0 ** negative
                nodep->dtypeSetLogicUnsized(nodep->width(), nodep->widthMin(),
                                            nodep->dtypep()->numeric());
            }
        }

        যদি (m_vup->final()) {
            Ast_গ্ৰন্থি_DType* expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
            Ast_গ্ৰন্থি_DType* ধ্রুৱ lhsDTypep = nodep->lhsp()->dtypep()->skipRefp();
            expDTypep
                = expDTypep->findBitOrLogicDType(expDTypep->width(), expDTypep->widthMin(),
                                                 lhsDTypep->numeric(), expDTypep->isFourstate());
            nodep->dtypep(expDTypep);
            iterateCheck(nodep, "LHS", nodep->lhsp(), CONTEXT_DET, FINAL, expDTypep, EXTEND_EXP);
            Ast_গ্ৰন্থি* newp = nullptr;  // No change
            যদি (nodep->lhsp()->isSigned() && nodep->rhsp()->isSigned()) {
                newp = নূতন AstPowSS{nodep->fileline(), nodep->lhsp()->unlinkFrBack(),
                                    nodep->rhsp()->unlinkFrBack()};
            } অন্যথা যদি (nodep->lhsp()->isSigned() && !nodep->rhsp()->isSigned()) {
                newp = নূতন AstPowSU{nodep->fileline(), nodep->lhsp()->unlinkFrBack(),
                                    nodep->rhsp()->unlinkFrBack()};
            } অন্যথা যদি (!nodep->lhsp()->isSigned() && nodep->rhsp()->isSigned()) {
                newp = নূতন AstPowUS{nodep->fileline(), nodep->lhsp()->unlinkFrBack(),
                                    nodep->rhsp()->unlinkFrBack()};
            }
            যদি (newp) {
                newp->dtypeFrom(nodep);
                UINFO(9, "powOld " << nodep);
                UINFO(9, "powNew " << newp);
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
            }
        }
    }
    শূন্য visit(AstPowSU* nodep) override {
        // POWSU/SS/US only created here, dtype already determined, so
        // nothing to do in this function
        userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
        userIterateAndNext(nodep->rhsp(), WidthVP{SELF, BOTH}.p());
    }
    শূন্য visit(AstPowSS* nodep) override {
        // POWSU/SS/US only created here, dtype already determined, so
        // nothing to do in this function
        userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
        userIterateAndNext(nodep->rhsp(), WidthVP{SELF, BOTH}.p());
    }
    শূন্য visit(AstPowUS* nodep) override {
        // POWSU/SS/US only created here, dtype already determined, so
        // nothing to do in this function
        userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
        userIterateAndNext(nodep->rhsp(), WidthVP{SELF, BOTH}.p());
    }
    শূন্য visit(Ast_গুনতি_Bits* nodep) override {
        assert_At_ৰাশিমালা(nodep);
        যদি (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->lhsp(), SELF, BOTH);
            iterateCheckSizedSelf(nodep, "RHS", nodep->rhsp(), SELF, BOTH);
            iterateCheckSizedSelf(nodep, "THS", nodep->thsp(), SELF, BOTH);
            iterateCheckSizedSelf(nodep, "FHS", nodep->fhsp(), SELF, BOTH);
            // For widthMin, if a 32 bit number, we need a 6 bit number as we need to return '32'.
            ধ্রুৱ পূর্ণাঙ্ক widthMin = V3_সংখ্যা::log2b(nodep->lhsp()->width()) + 1;
            nodep->dtypeSetBitUnsized(32, widthMin, VSigning::SIGNED);
        }
    }
    শূন্য visit(Ast_গুনতি_Ones* nodep) override {
        যদি (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->lhsp(), SELF, BOTH);
            // For widthMin, if a 32 bit number, we need a 6 bit number as we need to return '32'.
            ধ্রুৱ পূর্ণাঙ্ক widthMin = V3_সংখ্যা::log2b(nodep->lhsp()->width()) + 1;
            nodep->dtypeSetBitUnsized(32, widthMin, VSigning::SIGNED);
        }
    }
    শূন্য visit(AstCvtPackString* nodep) override {
        যদি (nodep->didWidthAndSet()) ঘূৰাই;
        // Opaque returns, so arbitrary
        userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH, currentStreamUse()}.p());
        // Type set in constructor
        // A class handle is not a legal implicit conversion to string
        যদি (Ast_গ্ৰন্থি_DType* ধ্রুৱ dt = nodep->lhsp()->dtypep()) {
            যদি (VN_IS(dt->skipRefToEnump(), ClassRefDType)) {
                nodep->lhsp()->v3error(
                    "Cannot convert 'class' handle to a string:" << dt->prettyDTypeNameQ());
            }
        }
    }
    শূন্য visit(AstCvtPackedToArray* nodep) override {
        যদি (nodep->didWidthAndSet()) ঘূৰাই;
        // Opaque returns, so arbitrary
        userIterateAndNext(nodep->fromp(), WidthVP{SELF, BOTH, currentStreamUse()}.p());
        // Type set in constructor
    }
    শূন্য visit(AstCvtArrayToArray* nodep) override {
        যদি (nodep->didWidthAndSet()) ঘূৰাই;
        // Opaque returns, so arbitrary
        userIterateAndNext(nodep->fromp(), WidthVP{SELF, BOTH, currentStreamUse()}.p());
        // Type set in constructor
    }
    শূন্য visit(AstCvtArrayToPacked* nodep) override {
        যদি (nodep->didWidthAndSet()) ঘূৰাই;
        // Opaque returns, so arbitrary
        userIterateAndNext(nodep->fromp(), WidthVP{SELF, BOTH, currentStreamUse()}.p());
        // Type set in constructor
    }
    শূন্য visit(AstCvtUnpackToQueue* nodep) override {
        যদি (nodep->didWidthAndSet()) ঘূৰাই;
        // Opaque returns, so arbitrary
        userIterateAndNext(nodep->fromp(), WidthVP{SELF, BOTH}.p());
        // Type set in constructor
    }
    শূন্য visit(AstCvtUnpackToUnpack* nodep) override {
        // Type set when created, the value converted is of its own type
        যদি (m_vup->final()) {
            userIterateAndNext(nodep->fromp(), WidthVP{nodep->fromp()->dtypep(), FINAL}.p());
        }
    }
    শূন্য visit(AstTimeImport* nodep) override {
        // LHS is a real number in seconds
        // Need to round to time units and precision
        userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
        ধ্রুৱ AstConst* ধ্রুৱ constp = VN_CAST(nodep->lhsp(), Const);
        UASSERT_OBJ(constp && constp->isDouble(), nodep, "Times should be doubles");
        UASSERT_OBJ(!nodep->timeunit().isNone(), nodep, "$time import no units");
        ধ্রুৱ দ্ৱিগুণ timePrescale = constp->অঙ্ক().toDouble();
        UASSERT_OBJ(!v3Global.rootp()->timeprecision().isNone(), nodep, "Never set precision?");
        ধ্রুৱ দ্ৱিগুণ time = timePrescale / nodep->timeunit().multiplier();
        // IEEE claims you should round to time precision here, but no simulator seems to do this
        AstConst* ধ্রুৱ newp = নূতন AstConst{nodep->fileline(), AstConst::RealDouble{}, time};
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
    }
    শূন্য visit(AstEventControl* nodep) override {
        যদি (!m_underFork && VN_IS(m_ftaskp, Func)) {
            nodep->v3error("Event controls are not legal in functions. Suggest use a task "
                           "(IEEE 1800-2023 13.4.4)");
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_বৃক্ষ(), nodep);
            ঘূৰাই;
        }
        যদি (nodep->fileline()->timingOn()) {
            যদি (v3Global.opt.timing().isSetTrue()) {
                iterateChildren(nodep);
                ঘূৰাই;
            } অন্যথা যদি (v3Global.opt.timing().isSetFalse()) {
                nodep->v3warn(E_NOTIMING,
                              "Event control statement in this location requires --timing\n"
                                  << nodep->warnMore()
                                  << "... With --no-timing, suggest have one event control "
                                  << "statement per procedure, at the top of the procedure");
            } অন্যথা {
                nodep->v3warn(E_NEEDTIMINGOPT, "Use --timing or --no-timing to specify how "
                                               "event controls should be handled");
            }
        }
        যদি (nodep->stmtsp()) nodep->addNextHere(nodep->stmtsp()->unlinkFrBack());
        VL_DO_DANGLING(nodep->unlinkFrBack()->delete_বৃক্ষ(), nodep);
    }
    // Name of the class a class extends, for $typename, as other simulators give it. Only the
    // class it directly extends, as $typename of that class names the classes that one extends.
    স্থির string typenameExtends(ধ্রুৱ AstClass* classp) {
        // An interface class may extend several, so those are not named
        যদি (!classp || classp->isInterfaceClass()) ঘূৰাই "";
        বাবে (ধ্রুৱ AstClassExtends* extendsp = classp->extendsp(); extendsp;
             extendsp = VN_AS(extendsp->nextp(), ClassExtends)) {
            যদি (extendsp->isImplements()) অনুৱর্ত;
            ধ্রুৱ AstClass* ধ্রুৱ basep = extendsp->classOrNullp();
            ঘূৰাই basep ? " extends class " + basep->dtypeName(সঁচা) : "";
        }
        ঘূৰাই "";
    }
    শূন্য visit(AstAttrOf* nodep) override {
        VL_RESTORER(m_attrp);
        m_attrp = nodep;
        userIterateAndNext(nodep->fromp(), WidthVP{SELF, BOTH}.p());
        যদি (nodep->dimp()) userIterateAndNext(nodep->dimp(), WidthVP{SELF, BOTH}.p());
        // Don't iterate children, don't want to lose VarRef.
        ৱিকল্প (nodep->attrType()) {
        পক্ষ VAttrType::DIM_DIMENSIONS:
        পক্ষ VAttrType::DIM_UNPK_DIMENSIONS: {
            Ast_গ্ৰন্থি_DType* ধ্রুৱ dtypep = fromDTypep(nodep->fromp());
            UASSERT_OBJ(dtypep, nodep, "Unsized expression");
            ধ্রুৱ std::pair<uint32_t, uint32_t> dim = dtypep->dimensions(সঁচা);
            ধ্রুৱ পূর্ণাঙ্ক val
                = (nodep->attrType() == VAttrType::DIM_UNPK_DIMENSIONS ? dim.second
                                                                       : (dim.first + dim.second));
            nodep->replaceWith(নূতন AstConst(nodep->fileline(), AstConst::Signed32{}, val));
            VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
            ভঙ্গ;
        }
        পক্ষ VAttrType::DIM_BITS_OR_সংখ্যা: {
            // If dtype, compute DIM_BITS, else take expression as a number to use
            যদি (VN_IS(nodep->fromp(), গ্ৰন্থি_ৰাশিমালা)) {
                nodep->replaceWith(nodep->fromp()->unlinkFrBack());
                VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
            } অন্যথা {
                Ast_গ্ৰন্থি* newp = নূতন AstAttrOf{nodep->fileline(), VAttrType::DIM_BITS,
                                              nodep->fromp()->unlinkFrBack()};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
                userIterateAndNext(newp, WidthVP{SELF, BOTH}.p());  // Convert AttrOf
            }
            ঘূৰাই;
        }
        পক্ষ VAttrType::DIM_BITS:
        পক্ষ VAttrType::DIM_উপৰেৰ:
        পক্ষ VAttrType::DIM_INCREMENT:
        পক্ষ VAttrType::DIM_বাম:
        পক্ষ VAttrType::DIM_নিচেৰ:
        পক্ষ VAttrType::DIM_ডান:
        পক্ষ VAttrType::DIM_SIZE: {
            Ast_গ্ৰন্থি_DType* ধ্রুৱ dtypep = fromDTypep(nodep->fromp());
            UASSERT_OBJ(dtypep, nodep, "Unsized expression");
            // Only worth asking while parameters are still being worked out.
            যদি (m_paramsOnly) {
                // A module that is still being copied does not have its final sizes.
                ধ্রুৱ Ast_গ্ৰন্থি_Module* ধ্রুৱ ownModp = v3Global.rootp()->containingModule(dtypep);
                যদি (ownModp && ownModp->parameterizedTemplate() && !ownModp->dead()) {
                    UINFO(9, "size deferred, type still on template " << ownModp->name());
                    // These queries always give an int, so set that now and let the
                    // value be worked out once the copy exists.
                    nodep->dtypeSetInt();
                    ঘূৰাই;
                }
            }

            যদি (VN_IS(dtypep, QueueDType) || VN_IS(dtypep, DynArrayDType)) {
                ৱিকল্প (nodep->attrType()) {
                পক্ষ VAttrType::DIM_SIZE: {
                    Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ fromp = VN_AS(nodep->fromp()->unlinkFrBack(), গ্ৰন্থি_ৰাশিমালা);
                    Ast_গ্ৰন্থি* ধ্রুৱ newp
                        = নূতন AstCMethodHard{nodep->fileline(), fromp, VCMethod::DYN_SIZE};
                    newp->dtypeSetInt();
                    newp->didWidth(সঁচা);
                    newp->protect(মিছা);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
                    ভঙ্গ;
                }
                পক্ষ VAttrType::DIM_বাম:
                পক্ষ VAttrType::DIM_নিচেৰ: {
                    Ast_গ্ৰন্থি* ধ্রুৱ newp = নূতন AstConst(nodep->fileline(), AstConst::Signed32{}, 0);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
                    ভঙ্গ;
                }
                পক্ষ VAttrType::DIM_ডান:
                পক্ষ VAttrType::DIM_উপৰেৰ: {
                    Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ fromp = VN_AS(nodep->fromp()->unlinkFrBack(), গ্ৰন্থি_ৰাশিমালা);
                    Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ sizep
                        = নূতন AstCMethodHard{nodep->fileline(), fromp, VCMethod::DYN_SIZE};
                    sizep->dtypeSetInt();
                    sizep->didWidth(সঁচা);
                    sizep->protect(মিছা);
                    Ast_গ্ৰন্থি* ধ্রুৱ newp
                        = নূতন AstSub{nodep->fileline(), sizep,
                                     নূতন AstConst(nodep->fileline(), AstConst::Signed32{}, 1)};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
                    ভঙ্গ;
                }
                পক্ষ VAttrType::DIM_INCREMENT: {
                    Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ newp
                        = নূতন AstConst(nodep->fileline(), AstConst::Signed32{}, -1);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
                    ভঙ্গ;
                }
                পক্ষ VAttrType::DIM_BITS: {
                    যদি (VN_IS(dtypep, DynArrayDType)) {
                        nodep->v3warn(E_UNSUPPORTED, "Unsupported: $bits for dynamic array");
                    } অন্যথা {
                        nodep->v3warn(E_UNSUPPORTED, "Unsupported: $bits for queue");
                    }
                    ভঙ্গ;
                }
                স্ৱভাৱ: nodep->v3fatalSrc("Unhandled attribute type");
                }
            } অন্যথা {
                ধ্রুৱ std::pair<uint32_t, uint32_t> dimpair = dtypep->skipRefp()->dimensions(সঁচা);
                ধ্রুৱ uint32_t msbdim = dimpair.first + dimpair.second;
                যদি (!nodep->dimp() || msbdim < 1) {
                    যদি (VN_IS(dtypep, BasicDType) && dtypep->basicp()->isString()) {
                        // IEEE undocumented but $bits(string) must give length(string) * 8
                        Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ fromp = VN_AS(nodep->fromp()->unlinkFrBack(), গ্ৰন্থি_ৰাশিমালা);
                        Ast_গ্ৰন্থি* ধ্রুৱ newp = নূতন AstShiftL{
                            nodep->fileline(), নূতন AstLenN{nodep->fileline(), fromp},
                            নূতন AstConst{nodep->fileline(), 3},  // * 8
                            32};
                        nodep->replaceWith(newp);
                        VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
                    } অন্যথা {
                        Ast_গ্ৰন্থি_DType* ধ্রুৱ baseDTypep = dtypep->skipRefp();
                        UASSERT_OBJ(baseDTypep, nodep, "Unsized expression");
                        ধ্রুৱ পূর্ণাঙ্ক dim = 1;
                        AstConst* ধ্রুৱ newp = dimension_মান(nodep->fileline(), baseDTypep,
                                                              nodep->attrType(), dim);
                        nodep->replaceWith(newp);
                        VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
                    }
                } অন্যথা যদি (VN_IS(nodep->dimp(), Const)) {
                    ধ্রুৱ পূর্ণাঙ্ক dim = VN_AS(nodep->dimp(), Const)->toSInt();
                    AstConst* ধ্রুৱ newp
                        = dimension_মান(nodep->fileline(), dtypep, nodep->attrType(), dim);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
                } অন্যথা {  // Need a runtime lookup table.  Yuk.
                    UASSERT_OBJ(nodep->fromp() && dtypep, nodep, "Unsized expression");
                    AstVarRef* ধ্রুৱ tabRefp = dimensionVarRefp(dtypep, nodep->attrType(), msbdim);
                    Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ dimp = nodep->dimp()->unlinkFrBack();
                    Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ newp = নূতন AstArraySel{nodep->fileline(), tabRefp, dimp};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
                }
            }
            ভঙ্গ;
        }
        পক্ষ VAttrType::FUNC_ARG_PROTO:
            // Created by V3LinkDot only to check that the prototype was correct when we got here
            // Checked at bottom of visit(AstNodeFTask)
            // V3WidthCommit::visit(AstAttrOf) will delete nodep
            ভঙ্গ;
        পক্ষ VAttrType::FUNC_RETURN_PROTO: {
            // Created by V3LinkDot only to check that the prototype was correct when we got here
            UASSERT_OBJ(m_ftaskp, nodep, "FUNC attr not under function");
            Ast_গ্ৰন্থি_DType* ধ্রুৱ protoDtp = nodep->fromp()->dtypep();
            Ast_গ্ৰন্থি_DType* ধ্রুৱ declDtp = [&]() {
                যদি (m_ftaskp->fvarp()) ঘূৰাই m_ftaskp->fvarp()->dtypep();
                Ast_গ্ৰন্থি_DType* ধ্রুৱ voidp = নূতন AstVoidDType{m_ftaskp->fileline()};
                ঠেলো_Deletep(voidp);  // Note voidp used past here
                ঘূৰাই voidp;
            }();
            যদি (!similarDTypeRecurse(protoDtp, declDtp)) {
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
            ভঙ্গ;
        }
        পক্ষ VAttrType::TYPENAME: {
            UASSERT_OBJ(nodep->fromp(), nodep, "Unprovided expression");
            Ast_গ্ৰন্থি_DType* ধ্রুৱ dtypep = nodep->fromp()->dtypep();
            string result = dtypep->prettyDTypeName(সঁচা);
            যদি (ধ্রুৱ AstClassRefDType* ধ্রুৱ classRefp
                = VN_CAST(dtypep->skipRefOrNullp(), ClassRefDType)) {
                result += typenameExtends(classRefp->classp());
            }
            UINFO(9, "typename '" << result << "' from " << dtypep);
            Ast_গ্ৰন্থি* ধ্রুৱ newp = নূতন AstConst{nodep->fileline(), AstConst::String{}, result};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
            ভঙ্গ;
        }
        পক্ষ VAttrType::TYPEID:
            যদি (Ast_গ্ৰন্থি_DType* dtypep = VN_CAST(nodep->fromp(), গ্ৰন্থি_DType)) {
                nodep->dtypep(dtypep);
            } অন্যথা {
                nodep->dtypep(VN_AS(nodep->fromp(), গ্ৰন্থি_ৰাশিমালা)->dtypep());
            }
            ভঙ্গ;
        পক্ষ VAttrType::VAR_BASE:
            // Soon to be handled in V3LinkWidth SEL generation, under attrp() and newSubLsbOf
            ভঙ্গ;
        স্ৱভাৱ: {
            // Everything else resolved earlier
            nodep->dtypeSetBitUnsized(32, 1, VSigning::UNSIGNED);  // Approximation, unsized 32
            UINFO(1, "Missing ATTR type case node: " << nodep);
            nodep->v3fatalSrc("Missing ATTR type case");
            ভঙ্গ;
        }
        }
    }
    শূন্য visit(AstPull* nodep) override {
        // May have select underneath, let seek natural size
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
    }
    শূন্য visit(AstText* nodep) override {
        // Only used in CStmts which don't care....
    }

    শূন্য visit(AstCellArrayRef* nodep) override {
        যদি (nodep->didWidthAndSet()) ঘূৰাই;
        userIterateAndNext(nodep->selp(), WidthVP{SELF, PRELIM}.p());
        userIterateAndNext(nodep->selp(), WidthVP{SELF, FINAL}.p());
        nodep->dtypeSetVoid();  // placeholder; this node shouldnt survive beyond linking
        nodep->didWidth(সঁচা);
    }

    শূন্য visit(AstCellRef* nodep) override {
        যদি (nodep->didWidthAndSet()) ঘূৰাই;

        যদি (Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ cellExprp = VN_CAST(nodep->cellp(), গ্ৰন্থি_ৰাশিমালা)) {
            userIterateAndNext(cellExprp, WidthVP{SELF, PRELIM}.p());
            userIterateAndNext(cellExprp, WidthVP{SELF, FINAL}.p());
        }

        যদি (Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ exprp = VN_CAST(nodep->exprp(), গ্ৰন্থি_ৰাশিমালা)) {
            userIterateAndNext(exprp, WidthVP{SELF, PRELIM}.p());
            nodep->dtypeFrom(exprp);
            userIterateAndNext(exprp, WidthVP{SELF, FINAL}.p());
        } অন্যথা {
            nodep->v3fatalSrc("CellRef exprp is not a NodeExpr: "  // LCOV_EXCL_LINE
                              << nodep->exprp()->prettyTypeName());
        }

        nodep->didWidth(সঁচা);
    }

    // DTYPES
    শূন্য visit(Ast_গ্ৰন্থি_Array_DType* nodep) override {
        যদি (nodep->didWidthAndSet()) ঘূৰাই;  // This node is a dtype & not both PRELIMed+FINALed

        যদি (nodep->subDTypep() == nodep->basicp()) {  // Innermost dimension
            AstBasicDType* ধ্রুৱ basicp = nodep->basicp();
            // If basic dtype is LOGIC_IMPLICIT, it is actually 1 bit LOGIC
            যদি (basicp->implicit()) {
                UASSERT_OBJ(basicp->width() <= 1, basicp,
                            "must be 1 bit but actually " << basicp->width() << " bits");
                AstBasicDType* ধ্রুৱ newp = নূতন AstBasicDType{
                    basicp->fileline(), VBasicDTypeKwd::LOGIC, basicp->numeric()};
                newp->widthForce(1, 1);
                basicp->replaceWith(newp);
                VL_DO_DANGLING(ঠেলো_Deletep(basicp), basicp);
            }
        }
        VL_RESTORER(m_underPackedArray);
        যদি (VN_IS(nodep, PackArrayDType)) m_underPackedArray = সঁচা;
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));

        // Cleanup array size
        userIterateAndNext(nodep->rangep(), WidthVP{SELF, BOTH}.p());
        nodep->dtypep(nodep);  // The array itself, not subDtype
        যদি (AstUnpackArrayDType* ধ্রুৱ adtypep = VN_CAST(nodep, UnpackArrayDType)) {
            // Historically array elements have width of the ref type not the full array
            nodep->widthFromSub(nodep->subDTypep());
            যদি (nodep->subDTypep()->skipRefp()->isCompound()) adtypep->isCompound(সঁচা);
        } অন্যথা {
            ধ্রুৱ Ast_গ্ৰন্থি_DType* ধ্রুৱ elemDTypep = nodep->subDTypep()->skipRefp();
            যদি (!elemDTypep->isIntegralOrPacked()) {
                nodep->v3error("Unpacked data type " << elemDTypep->prettyDTypeNameQ()
                                                     << " in packed array (IEEE 1800-2023 7.4.1)");
            }
            ধ্রুৱ পূর্ণাঙ্ক width = nodep->subDTypep()->width() * nodep->rangep()->elementsConst();
            nodep->widthForce(width, width);
            যদি (!VL_RESTORER_PREV(m_underPackedArray)) {  // Outermost dimension
                // IEEE 1800-2023 7.4.1 "Packed arrays" says
                //   If a packed array is declared as signed,
                //   then the array viewed as a single vector shall be signed.
                ধ্রুৱ Ast_গ্ৰন্থি_DType* baseDTypep = nodep->subDTypep();
                যেতিয়ালৈ (!VN_IS(baseDTypep, RefDType) && baseDTypep->subDTypep()) {
                    baseDTypep = baseDTypep->subDTypep();
                }
                ধ্রুৱ AstBasicDType* ধ্রুৱ basicp = nodep->basicp();
                যদি (!VN_IS(baseDTypep, RefDType) && basicp && basicp->isSigned()) {
                    nodep->numeric(VSigning::fromBool(সঁচা));
                }
            }
        }
        UINFO(4, "dtWidthed " << nodep);
    }
    শূন্য visit(AstAssocArrayDType* nodep) override {
        যদি (nodep->didWidthAndSet()) ঘূৰাই;  // This node is a dtype & not both PRELIMed+FINALed
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        nodep->চাবি_DTypep(iterateEditMoveDTypep(nodep, nodep->চাবি_DTypep()));
        nodep->dtypep(nodep);  // The array itself, not subDtype
        UINFO(4, "dtWidthed " << nodep);
    }
    শূন্য visit(AstBracketArrayDType* nodep) override {
        // Type inserted only because parser didn't know elementsp() type
        // Resolve elementsp's type
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
        // We must edit when dtype still under normal nodes and before type table
        // See notes in iterateEditMoveDTypep
        Ast_গ্ৰন্থি_DType* ধ্রুৱ childp = nodep->childDTypep();
        childp->unlinkFrBack();
        Ast_গ্ৰন্থি* ধ্রুৱ elementsp = nodep->elementsp()->unlinkFrBack();
        Ast_গ্ৰন্থি* newp;
        যদি (VN_IS(elementsp, Unbounded)) {
            newp = নূতন AstQueueDType{nodep->fileline(), VFlagChildDType{}, childp, nullptr};
            VL_DO_DANGLING(elementsp->delete_বৃক্ষ(), elementsp);
        } অন্যথা যদি (Ast_গ্ৰন্থি_DType* ধ্রুৱ keyp = VN_CAST(elementsp, গ্ৰন্থি_DType)) {
            newp = নূতন AstAssocArrayDType{nodep->fileline(), VFlagChildDType{}, childp, keyp};
        } অন্যথা {
            // The subtract in the range may confuse users; as the array
            // size is self determined there's no reason to warn about widths
            File_সাৰি* ধ্রুৱ elementsNewFl = elementsp->fileline();
            elementsNewFl->warnOff(V3ErrorCode::WIDTHEXPAND, সঁচা);
            // Must be expression that is constant, but we'll determine that later
            newp = নূতন AstUnpackArrayDType{
                nodep->fileline(), VFlagChildDType{}, childp,
                নূতন AstRange{nodep->fileline(), নূতন AstConst(elementsp->fileline(), 0),
                             নূতন AstSub{elementsNewFl, VN_AS(elementsp, গ্ৰন্থি_ৰাশিমালা),
                                        নূতন AstConst(elementsNewFl, 1)},
                             সঁচা}};
        }
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
        // Normally parent's iteration would cover this, but we might have entered by a specific
        // visit
        VL_DO_DANGLING(userIterate(newp, nullptr), newp);
    }
    শূন্য visit(AstConstraintRefDType* nodep) override {
        যদি (nodep->didWidthAndSet()) ঘূৰাই;  // This node is a dtype & not both PRELIMed+FINALed
        nodep->dtypep(nodep);
    }
    শূন্য visit(AstDynArrayDType* nodep) override {
        যদি (nodep->didWidthAndSet()) ঘূৰাই;  // This node is a dtype & not both PRELIMed+FINALed
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        nodep->dtypep(nodep);  // The array itself, not subDtype
        UINFO(4, "dtWidthed " << nodep);
    }
    শূন্য visit(AstQueueDType* nodep) override {
        যদি (nodep->didWidthAndSet()) ঘূৰাই;  // This node is a dtype & not both PRELIMed+FINALed
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        nodep->dtypep(nodep);  // The array itself, not subDtype
        userIterateAndNext(nodep->boundp(), WidthVP{SELF, BOTH}.p());
        যদি (VN_IS(nodep->boundp(), Unbounded)) {
            nodep->boundp()->unlinkFrBack()->delete_বৃক্ষ();  // nullptr will represent unbounded
        }
        UINFO(4, "dtWidthed " << nodep);
    }
    শূন্য visit(AstVoidDType* nodep) override {
        যদি (nodep->didWidthAndSet()) ঘূৰাই;  // This node is a dtype & not both PRELIMed+FINALed
        nodep->dtypep(nodep);
        UINFO(4, "dtWidthed " << nodep);
    }
    শূন্য visit(AstUnsizedArrayDType* nodep) override {
        যদি (nodep->didWidthAndSet()) ঘূৰাই;  // This node is a dtype & not both PRELIMed+FINALed
        যদি (nodep->childDTypep() && !(m_ftaskp && m_ftaskp->dpiImport())) {
            nodep->v3error("Unsized packed dimension '[]' is only legal in a DPI import"
                           " (IEEE 1800-2023 A.2.2.1)");
        }
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        // Cleanup array size
        nodep->dtypep(nodep);  // The array itself, not subDtype
        UINFO(4, "dtWidthed " << nodep);
    }
    শূন্য visit(AstWildcardArrayDType* nodep) override {
        যদি (nodep->didWidthAndSet()) ঘূৰাই;  // This node is a dtype & not both PRELIMed+FINALed
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        // Cleanup array size
        nodep->dtypep(nodep);  // The array itself, not subDtype
        UINFO(4, "dtWidthed " << nodep);
    }
    শূন্য visit(AstBasicDType* nodep) override {
        যদি (nodep->didWidthAndSet()) ঘূৰাই;  // This node is a dtype & not both PRELIMed+FINALed
        যদি (nodep->generic()) ঘূৰাই;  // Already perfect
        যদি (nodep->rangep()) {
            userIterateAndNext(nodep->rangep(), WidthVP{SELF, BOTH}.p());
            // Because this DType has a unique child range, we know it's not
            // pointed at by other nodes unless they are referencing this type.
            // Furthermore the width() calculation would return identical
            // values.  Therefore we can directly replace the width
            nodep->widthForce(nodep->rangep()->elementsConst(), nodep->rangep()->elementsConst());
        } অন্যথা যদি (nodep->isRanged()) {
            nodep->widthForce(nodep->nrange().elements(), nodep->nrange().elements());
        } অন্যথা যদি (nodep->implicit()) {
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
    শূন্য visit(AstConstDType* nodep) override {
        যদি (nodep->didWidthAndSet()) ঘূৰাই;  // This node is a dtype & not both PRELIMed+FINALed
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        userIterateChildren(nodep, nullptr);
        nodep->dtypep(nodep);  // Should already be set, but be clear it's not the subDType
        nodep->widthFromSub(nodep->subDTypep());
        UINFO(4, "dtWidthed " << nodep);
    }
    শূন্য visit(AstRefDType* nodep) override {
        যদি (nodep->didWidthAndSet()) ঘূৰাই;  // This node is a dtype & not both PRELIMed+FINALed
        nodep->doingWidth(সঁচা);
        যদি (nodep->typeofp()) {  // type(typeofp_expression)
            যদি (Ast_গ্ৰন্থি_DType* typeofDtp = VN_CAST(nodep->typeofp(), গ্ৰন্থি_DType)) {
                // It's directly a type, e.g. "type(int)"
                typeofDtp = iterateEditMoveDTypep(nodep, typeofDtp);  // Changes typeofp
                nodep->refDTypep(typeofDtp);
            } অন্যথা {
                // Type comes from expression's type, e.g. "type(variable)"
                userIterateAndNext(nodep->typeofp(), WidthVP{SELF, BOTH}.p());
                Ast_গ্ৰন্থি* ধ্রুৱ typeofp = nodep->typeofp();
                nodep->refDTypep(typeofp->dtypep());
                VL_DO_DANGLING(typeofp->unlinkFrBack()->delete_বৃক্ষ(), typeofp);
            }
            // We had to use AstRefDType for this construct as pointers to this type
            // in type table are still correct (which they wouldn't be if we replaced the node)
        }
        userIterateChildren(nodep, nullptr);
        যদি (nodep->subDTypep()) {
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
        যদি (!nodep->subDTypep()) {
            // Defer unlinked RefDTypes in parameterized template modules (or types
            // with no owning module, e.g. moved to TypeTable) until specialization
            // resolves them.
            ধ্রুৱ তার্কিক inTemplateModule = !m_modep || m_modep->parameterizedTemplate();
            যদি (inTemplateModule) {
                nodep->doingWidth(মিছা);
                ঘূৰাই;
            }
        }
        UASSERT_OBJ(nodep->subDTypep(), nodep, "Unlinked");
        nodep->dtypeFrom(nodep->subDTypep());
        nodep->widthFromSub(nodep->subDTypep());
        UINFO(4, "dtWidthed " << nodep);
        // No nodep->typedefp(nullptr) for now; V3WidthCommit needs to check accesses
        nodep->doingWidth(মিছা);
    }
    // Width the parameters of the scopes declaring typedef 'nodep', as the name of the type it
    // declares holds their values, though one may follow it, in a module without a parameter
    // port list
    শূন্য widthDTypeNameParams(ধ্রুৱ AstTypedef* nodep) {
        ধ্রুৱ VDTypeNameScopes* ধ্রুৱ scopesp = VDTypeNameScopes::currentp();
        যদি (!scopesp) ঘূৰাই;  // Computing parameters, as by V3Param, so naming as found
        Ast_গ্ৰন্থি* scopep = nullptr;
        বাবে (ধ্রুৱ Ast_গ্ৰন্থি* innerp = nodep; scopesp->outerp(innerp, scopep) && scopep;
             innerp = scopep) {
            যদি (ধ্রুৱ std::vector<Ast_গ্ৰন্থি*>* ধ্রুৱ paramsp = scopesp->paramsp(scopep)) {
                বাবে (Ast_গ্ৰন্থি* ধ্রুৱ paramp : *paramsp) {
                    যদি (!paramp->didWidth()) userIterate(paramp, nullptr);
                }
            }
        }
    }
    শূন্য visit(AstTypedef* nodep) override {
        যদি (nodep->didWidthAndSet()) ঘূৰাই;  // This node is a dtype & not both PRELIMed+FINALed
        যদি (auto* ধ্রুৱ refp = checkRefToTypedefRecurse(nodep, nodep)) {
            nodep->v3error("Typedef has self-reference: " << nodep->prettyNameQ() << '\n'
                                                          << nodep->warnContextPrimary() << '\n'
                                                          << refp->warnOther()
                                                          << "... Location of reference\n"
                                                          << refp->warnContextSecondary());
            // May cause internal error but avoids infinite loop on dump
            refp->typedefp(nullptr);
            VL_DO_DANGLING(ঠেলো_Deletep(nodep->unlinkFrBack()), nodep);
            ঘূৰাই;
        }
        // As it moves to the type table, a structure, union, or enumeration it declares keeps the
        // name it has from the typedef, with the scope of the typedef, so with the parameters of
        // the scope, widthed first, as one may follow the typedef
        ধ্রুৱ তার্কিক declares = nodep->childDTypep() != nullptr;
        nodep->dtypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        যদি (declares) {
            Ast_গ্ৰন্থি_DType* ধ্রুৱ dtypep = nodep->dtypep();
            যদি (Ast_গ্ৰন্থি_UOr_Struct_DType* ধ্রুৱ sdtypep = VN_CAST(dtypep, গ্ৰন্থি_UOr_Struct_DType)) {
                widthDTypeNameParams(nodep);
                sdtypep->typedefName(nodep->dtypeName());
            } অন্যথা যদি (AstEnumDType* ধ্রুৱ edtypep = VN_CAST(dtypep, EnumDType)) {
                widthDTypeNameParams(nodep);
                edtypep->typedefName(nodep->dtypeName());
            }
        }
        userIterateChildren(nodep, nullptr);
    }
    শূন্য visit(AstParamTypeDType* nodep) override {
        যদি (nodep->didWidthAndSet()) ঘূৰাই;  // This node is a dtype & not both PRELIMed+FINALed

        nodep->dtypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        userIterateChildren(nodep, nullptr);
        nodep->widthFromSub(nodep->subDTypep());
        // Clear childDTypep after dtypep is set to satisfy V3Broken invariant.
        // The child dtype has been moved to the type table by iterateEditMoveDTypep.
        যদি (nodep->dtypep() && nodep->childDTypep()) { nodep->childDTypep(nullptr); }
    }
    শূন্য visit(AstRequireDType* nodep) override {
        userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
        যদি (Ast_গ্ৰন্থি_DType* ধ্রুৱ dtp = VN_CAST(nodep->lhsp(), গ্ৰন্থি_DType)) {
            nodep->replaceWith(dtp->unlinkFrBack());
        } অন্যথা {
            যদি (nodep->lhsp())
                nodep->lhsp()->v3error("Expected data type, not a "
                                       << nodep->lhsp()->prettyTypeName());
            nodep->replaceWith(নূতন AstVoidDType{nodep->fileline()});
        }
        VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
    }
    শূন্য visit(AstCastDynamic* nodep) override {
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
        Ast_গ্ৰন্থি_DType* ধ্রুৱ toDtp = nodep->শীৰ্ষ()->dtypep()->skipRefToEnump();
        Ast_গ্ৰন্থি_DType* ধ্রুৱ fromDtp = nodep->fromp()->dtypep()->skipRefToEnump();
        File_সাৰি* ধ্রুৱ fl = nodep->fileline();
        ধ্রুৱ auto castable = Ast_গ্ৰন্থি::computeCastable(toDtp, fromDtp, nodep->fromp());
        Ast_গ্ৰন্থি* newp;
        যদি (castable == VCastable::DYNAMIC_CLASS) {
            // Keep in place, will compute at runtime
            ঘূৰাই;
        } অন্যথা যদি (castable == VCastable::ENUM_EXPLICIT || castable == VCastable::ENUM_IMPLICIT) {
            // TODO is from is a constant we could simplify, though normal constant
            // elimination should do much the same
            // Form: "( ((v > size) ? false : enum_valid[v[N:0]])
            //          ? ExprStmt(ExprAssign(out, Cast(v, type)), 1) : 0)"
            AstEnumDType* ধ্রুৱ enumDtp = VN_AS(toDtp, EnumDType);
            UASSERT_OBJ(enumDtp, nodep, "$cast determined as enum, but not enum type");
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ testp = enumTestValid(nodep->fromp(), enumDtp);
            File_সাৰি* ধ্রুৱ fl_novalue = নূতন File_সাৰি{fl};
            fl_novalue->warnOff(V3ErrorCode::ENUMVALUE, সঁচা);
            newp = নূতন AstCond{
                fl, testp,
                নূতন Ast_ৰাশিমালা_Stmt{fl,
                                নূতন AstAssign{fl_novalue, nodep->শীৰ্ষ()->unlinkFrBack(),
                                              nodep->fromp()->unlinkFrBack()},
                                নূতন AstConst{fl, AstConst::Signed32{}, 1}},
                নূতন AstConst{fl, AstConst::Signed32{}, 0}};
        } অন্যথা যদি ((castable == VCastable::SAMEISH || castable == VCastable::COMPATIBLE)
                   // Unsupported: the static cast below has no lowering for unpacked arrays
                   && !VN_IS(toDtp, UnpackArrayDType)) {
            nodep->v3warn(CASTCONST, "$cast will always return one as "
                                         << toDtp->prettyDTypeNameQ()
                                         << " is always castable from "
                                         << fromDtp->prettyDTypeNameQ() << '\n'
                                         << nodep->warnMore() << "... Suggest static cast");
            newp = নূতন Ast_ৰাশিমালা_Stmt{
                fl,
                নূতন AstAssign{fl, nodep->শীৰ্ষ()->unlinkFrBack(),
                              নূতন AstCast{fl, nodep->fromp()->unlinkFrBack(), toDtp}},
                নূতন AstConst{fl, AstConst::Signed32{}, 1}};
        } অন্যথা যদি (castable == VCastable::INCOMPATIBLE) {
            newp = নূতন AstConst{fl, 0};
            nodep->v3warn(CASTCONST, "$cast will always return zero as "
                                         << toDtp->prettyDTypeNameQ() << " is not castable from "
                                         << fromDtp->prettyDTypeNameQ());
        } অন্যথা {
            newp = নূতন AstConst{fl, 0};
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: $cast to "
                                             << toDtp->prettyDTypeNameQ() << " from "
                                             << fromDtp->prettyDTypeNameQ() << '\n'
                                             << nodep->warnMore()
                                             << "... Suggest try static cast");
        }
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
        userIterate(newp, m_vup);
    }
    শূন্য visit(AstCastParse* nodep) override {
        // nodep->dtp could be data type, or a primary_constant
        // Don't iterate lhsp, will deal with that once convert the type
        V3Const::constifyParamsEdit(nodep->dtp());  // itemp may change
        যদি (AstConst* ধ্রুৱ constp = VN_CAST(nodep->dtp(), Const)) {
            constp->unlinkFrBack();
            Ast_গ্ৰন্থি* ধ্রুৱ newp
                = নূতন AstCastSize{nodep->fileline(), nodep->lhsp()->unlinkFrBack(), constp};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
            userIterate(newp, m_vup);
        } অন্যথা যদি (Ast_গ্ৰন্থি_DType* ধ্রুৱ refp = VN_CAST(nodep->dtp(), গ্ৰন্থি_DType)) {
            refp->unlinkFrBack();
            Ast_গ্ৰন্থি* ধ্রুৱ newp = নূতন AstCast{nodep->fileline(), nodep->lhsp()->unlinkFrBack(),
                                              VFlagChildDType{}, refp};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
            userIterate(newp, m_vup);
        } অন্যথা {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Cast to " << nodep->dtp()->prettyTypeName());
            nodep->replaceWith(nodep->lhsp()->unlinkFrBack());
            VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
        }
    }
    শূন্য visit(AstCast* nodep) override {
        যদি (nodep->didWidth()) ঘূৰাই;
        UINFO(9, "CAST " << nodep);
        nodep->dtypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        যদি (m_vup->prelim()) {
            UINFOTREE(9, nodep, "", "CastPre");
            // UINFOTREE(1, nodep->backp(), "", "CastPreUpUp");
            যদি (AstSigned* ধ্রুৱ fromp = VN_CAST(nodep->fromp(), Signed)) {
                যদি (VN_IS(fromp->lhsp(), গ্ৰন্থি_Stream)) {
                    Ast_গ্ৰন্থি* ধ্রুৱ lhsp = fromp->lhsp()->unlinkFrBack();
                    fromp->replaceWith(lhsp);
                    VL_DO_DANGLING(fromp->delete_বৃক্ষ(), fromp);
                }
            }
            userIterateAndNext(nodep->fromp(), WidthVP{SELF, PRELIM, STREAM_USE_CAST}.p());
            UINFOTREE(9, nodep, "", "CastDit");
            Ast_গ্ৰন্থি_DType* ধ্রুৱ toDtp = nodep->dtypep()->skipRefToEnump();
            Ast_গ্ৰন্থি_DType* ধ্রুৱ fromDtp = nodep->fromp()->dtypep()->skipRefToEnump();
            ধ্রুৱ auto castable = Ast_গ্ৰন্থি::computeCastable(toDtp, fromDtp, nodep->fromp());
            তার্কিক bad = মিছা;
            যদি (castable == VCastable::UNSUPPORTED) {
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: static cast to "
                                                 << toDtp->prettyDTypeNameQ() << " from "
                                                 << fromDtp->prettyDTypeNameQ());
                bad = সঁচা;
            } অন্যথা যদি (castable == VCastable::SAMEISH || castable == VCastable::COMPATIBLE
                       || castable == VCastable::ENUM_IMPLICIT
                       || castable == VCastable::ENUM_EXPLICIT) {
                ;  // Continue
            } অন্যথা যদি (castable == VCastable::DYNAMIC_CLASS) {
                nodep->v3error("Dynamic, not static cast, required to cast "
                               << toDtp->prettyDTypeNameQ() << " from "
                               << fromDtp->prettyDTypeNameQ() << '\n'
                               << nodep->warnMore() << "... Suggest dynamic $cast");
                bad = সঁচা;
            } অন্যথা যদি (castable == VCastable::INCOMPATIBLE) {
                nodep->v3error("Incompatible types to static cast to "
                               << toDtp->prettyDTypeNameQ() << " from "
                               << fromDtp->prettyDTypeNameQ() << '\n');
                bad = সঁচা;
            } অন্যথা {
                nodep->v3fatalSrc("bad casting case");
            }
            // For now, replace it ASAP, so widthing can propagate easily
            // The cast may change signing, but we don't know the sign yet.  Make it so.
            // Note we don't sign fromp() that would make the algorithm O(n^2) if lots of casting.
            Ast_গ্ৰন্থি_ৰাশিমালা* newp = nullptr;
            যদি (bad) {
            } অন্যথা যদি (AstBasicDType* ধ্রুৱ basicp = toDtp->basicp()) {
                যদি (!basicp->isString() && fromDtp->isString()) {
                    newp = নূতন AstNToI{nodep->fileline(), nodep->fromp()->unlinkFrBack(), toDtp};
                } অন্যথা যদি (!basicp->isDouble() && !fromDtp->isDouble()) {
                    Ast_গ্ৰন্থি_DType* ধ্রুৱ origDTypep = nodep->dtypep();
                    যদি (!VN_IS(fromDtp, StreamDType)) {
                        ধ্রুৱ পূর্ণাঙ্ক width = toDtp->width();
                        castSized(nodep, nodep->fromp(), width);
                    }
                    nodep->dtypeFrom(origDTypep);  // If was enum, need dtype to preserve as enum
                    // Note castSized might modify nodep->fromp()
                } অন্যথা {
                    iterateCheck(nodep, "value", nodep->fromp(), SELF, FINAL, fromDtp, EXTEND_EXP,
                                 মিছা);
                }
                যদি (newp) {
                } অন্যথা যদি (basicp->isDouble() && !nodep->fromp()->isDouble()) {
                    যদি (nodep->fromp()->isSigned()) {
                        newp = নূতন AstISToRD{nodep->fileline(), nodep->fromp()->unlinkFrBack()};
                    } অন্যথা {
                        newp = নূতন AstIToRD{nodep->fileline(), nodep->fromp()->unlinkFrBack()};
                    }
                } অন্যথা যদি (!basicp->isDouble() && nodep->fromp()->isDouble()) {
                    newp = নূতন AstRToIRoundS{nodep->fileline(), nodep->fromp()->unlinkFrBack()};
                    newp->dtypep(basicp);
                } অন্যথা যদি (basicp->isSigned() && !nodep->fromp()->isSigned()) {
                    newp = নূতন AstSigned{nodep->fileline(), nodep->fromp()->unlinkFrBack()};
                } অন্যথা যদি (!basicp->isSigned() && nodep->fromp()->isSigned()) {
                    newp = নূতন AstUnsigned{nodep->fileline(), nodep->fromp()->unlinkFrBack()};
                } অন্যথা {
                    // Can just remove cast, but need extend placeholder
                    // so we can avoid warning message
                }
            } অন্যথা যদি (VN_IS(toDtp, QueueDType)) {
                যদি (VN_IS(fromDtp, BasicDType)) {
                    newp = নূতন AstCvtPackedToArray{nodep->fileline(),
                                                   nodep->fromp()->unlinkFrBack(), toDtp};
                } অন্যথা যদি (VN_IS(fromDtp, QueueDType) || VN_IS(fromDtp, StreamDType)) {
                    পূর্ণাঙ্ক srcElementBits = 1;
                    পূর্ণাঙ্ক dstElementBits = 1;
                    যদি (Ast_গ্ৰন্থি_DType* ধ্রুৱ elemDtp = fromDtp->subDTypep()) {
                        srcElementBits = elemDtp->width();
                    }
                    ধ্রুৱ AstQueueDType* ধ্রুৱ dstQueueDtp = VN_AS(toDtp, QueueDType);
                    যদি (Ast_গ্ৰন্থি_DType* ধ্রুৱ elemDtp = dstQueueDtp->subDTypep()) {
                        dstElementBits = elemDtp->width();
                    }
                    newp = নূতন AstCvtArrayToArray{nodep->fileline(),
                                                  nodep->fromp()->unlinkFrBack(),
                                                  toDtp,
                                                  মিছা,
                                                  1,
                                                  dstElementBits,
                                                  srcElementBits};
                }
            } অন্যথা যদি (VN_IS(toDtp, ClassRefDType)) {
                // Can just remove cast
            } অন্যথা {
                nodep->v3fatalSrc("Unimplemented: Casting non-simple data type "
                                  << toDtp->prettyDTypeNameQ());
            }
            যদি (!newp) newp = nodep->fromp()->unlinkFrBack();
            nodep->fromp(newp);
            UINFOTREE(9, nodep, "", "CastOut");
            // UINFOTREE(1, nodep->backp(), "", "CastOutUpUp");
        }
        যদি (m_vup->final()) {
            UINFOTREE(9, nodep, "", "CastFPit");
            iterateCheck(nodep, "value", nodep->fromp(), SELF, FINAL, nodep->fromp()->dtypep(),
                         EXTEND_EXP, মিছা, STREAM_USE_CAST);
            UINFOTREE(9, nodep, "", "CastFin");
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ underp = nodep->fromp()->unlinkFrBack();
            underp->dtypeFrom(nodep);
            underp->didWidth(সঁচা);
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ newp = নূতন AstCastWrap{nodep->fileline(), underp};
            newp->didWidth(সঁচা);
            UINFOTREE(9, newp, "", "CastRep");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
        }
    }
    শূন্য visit(AstCastSize* nodep) override {
        // IEEE: Signedness of result is same as self-determined signedness
        // However, the result is same as BITSEL, so we do not sign extend the LHS
        // UINFOTREE(1, nodep, "", "CastSizePre");
        assert_At_ৰাশিমালা(nodep);
        যদি (m_vup->prelim()) {
            পূর্ণাঙ্ক width = nodep->rhsp()->toSInt();
            যদি (width < 1) {
                nodep->v3error("Size-changing cast to zero or negative size: " << width);
                width = 1;
            }
            userIterateAndNext(nodep->lhsp(), WidthVP{SELF, PRELIM, STREAM_USE_CAST}.p());
            castSized(nodep, nodep->lhsp(), width);  // lhsp may change
        }
        যদি (m_vup->final()) {
            // CastSize not needed once sizes determined
            Ast_গ্ৰন্থি* ধ্রুৱ underp = nodep->lhsp()->unlinkFrBack();
            nodep->replaceWithKeepDType(underp);
            VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
        }
        // UINFOTREE(1, nodep, "", "CastSizeOut");
    }
    শূন্য visit(AstCastWrap* nodep) override {
        // Inserted by V3Width only so we know has been resolved
        userIterateAndNext(nodep->lhsp(), WidthVP{nodep->dtypep(), BOTH, STREAM_USE_CAST}.p());
    }
    শূন্য castSized(Ast_গ্ৰন্থি* nodep, Ast_গ্ৰন্থি* underp, পূর্ণাঙ্ক width) {
        ধ্রুৱ AstBasicDType* underDtp = VN_CAST(underp->dtypep(), BasicDType);
        যদি (!underDtp) underDtp = underp->dtypep()->basicp();
        যদি (!underDtp) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: Size-changing cast on non-basic data type");
            underDtp = VN_AS(nodep->findBitDType(), BasicDType);
        }
        UASSERT_OBJ(underp == nodep->op1p(), nodep, "Assuming op1 is cast value");
        // A cast propagates its size to the lower expression and is included in the maximum
        // width, so 23'(1'b1 + 1'b1) uses 23-bit math, but 1'(2'h2 * 2'h1) uses two-bit math.
        // However the output width is exactly that requested.
        // So two steps, first do the calculation's width (max of the two widths)
        {
            ধ্রুৱ পূর্ণাঙ্ক calcWidth = std::max(width, underDtp->width());
            Ast_গ্ৰন্থি_DType* ধ্রুৱ calcDtp = nodep->findBitOrLogicDType(
                calcWidth, calcWidth, underDtp->numeric(), underDtp->isFourstate());
            nodep->dtypep(calcDtp);
            // We ignore warnings as that is sort of the point of a cast
            iterateCheck(nodep, "Cast expr", underp, CONTEXT_DET, FINAL, calcDtp, EXTEND_EXP,
                         মিছা, STREAM_USE_CAST);
            VL_DANGLING(underp);
            underp = nodep->op1p();  // Above asserts that op1 was underp pre-relink
        }
        // UINFOTREE(1, nodep, "", "CastSizeClc");
        // Next step, make the proper output width
        {
            Ast_গ্ৰন্থি_DType* ধ্রুৱ outDtp = nodep->findBitOrLogicDType(
                width, width, underDtp->numeric(), underDtp->isFourstate());
            nodep->dtypep(outDtp);
            // We ignore warnings as that is sort of the point of a cast
            widthCheckSized(nodep, "Cast expr", VN_AS(underp, গ্ৰন্থি_ৰাশিমালা), outDtp, EXTEND_EXP,
                            মিছা);
            VL_DANGLING(underp);
        }
    }
    শূন্য visit(AstConstraintBefore* nodep) override {
        userIterateAndNext(nodep->lhssp(), WidthVP{SELF, BOTH}.p());
        userIterateAndNext(nodep->rhssp(), WidthVP{SELF, BOTH}.p());
    }
    শূন্য visit(Ast_Constraint_ৰাশিমালা* nodep) override {
        userIterateAndNext(nodep->exprp(), WidthVP{SELF, BOTH}.p());
    }
    শূন্য visit(AstConstraintUnique* nodep) override {
        userIterateAndNext(nodep->rangesp(), WidthVP{SELF, BOTH}.p());
    }
    শূন্য visit(AstDistItem* nodep) override {
        userIterate(nodep->rangep(), m_vup);
        assert_At_ৰাশিমালা(nodep);
        যদি (m_vup->prelim()) {  // First stage evaluation
            userIterate(nodep->weightp(), WidthVP{SELF, BOTH}.p());
        }
        nodep->dtypep(nodep->rangep()->dtypep());
    }
    শূন্য visit(AstVar* nodep) override {
        // UINFOTREE(1, nodep, "", "InitPre");
        // Must have deterministic constant width
        // We can't skip this step when width()!=0, as creating a AstVar
        // with non-constant range gets size 1, not size 0.  So use didWidth().
        যদি (nodep->didWidth()) ঘূৰাই;
        যদি (nodep->doingWidth()) {  // Early exit if have circular parameter definition
            UASSERT_OBJ(nodep->valuep(), nodep, "circular, but without value");
            nodep->v3error("Variable's initial value is circular: " << nodep->prettyNameQ());
            ঠেলো_Deletep(nodep->valuep()->unlinkFrBack());
            nodep->valuep(নূতন AstConst{nodep->fileline(), AstConst::BitFalseErroring{}});
            nodep->dtypeFrom(nodep->valuep());
            nodep->didWidth(সঁচা);
            ঘূৰাই;
        }
        nodep->doingWidth(সঁচা);
        // Make sure dtype is sized
        nodep->dtypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        UASSERT_OBJ(nodep->dtypep(), nodep, "No dtype determined for var");
        // Removed at the end, see mainAcceptEdit
        যদি (!m_paramsOnly && nodep->isIfaceArraySplit()) m_splitIfaceArrayVarps.ঠেলো_back(nodep);
        যদি (nodep->attrsp()) {
            nodep->attrsp()->foreach([এতত্, nodep](AstAttrOf* attrp) {
                যদি (attrp->attrType() == VAttrType::VAR_PORT_DTYPE) {
                    V3Const::constifyParamsEdit(attrp->fromp());  // fromp may change
                    যদি (!similarDTypeRecurse(nodep->dtypep(), VN_AS(attrp->fromp(), গ্ৰন্থি_DType))) {
                        nodep->dtypep()->v3error("Non-ANSI I/O declaration of signal "
                                                 "conflicts with type declaration: "
                                                 << nodep->prettyNameQ() << '\n'
                                                 << nodep->dtypep()->warnContextPrimary() << '\n'
                                                 << attrp->warnOther()
                                                 << "... Location of other declaration\n"
                                                 << attrp->warnContextSecondary());
                    }
                    VL_DO_DANGLING(ঠেলো_Deletep(attrp->unlinkFrBack()), attrp);
                }
            });
        }
        যদি (m_ftaskp && m_ftaskp->dpiImport()) {
            Ast_গ্ৰন্থি_DType* dtp = nodep->dtypep();
            Ast_গ্ৰন্থি_DType* np = nullptr;
            যেতিয়ালৈ (VN_IS(dtp->skipRefp(), DynArrayDType)
                   || VN_IS(dtp->skipRefp(), UnpackArrayDType)) {
                যদি (ধ্রুৱ AstDynArrayDType* ধ্রুৱ unsizedp
                    = VN_CAST(dtp->skipRefp(), DynArrayDType)) {
                    যদি (!np) {
                        UINFO(9, "Dynamic becomes unsized array (var itself) " << nodep);
                    } অন্যথা {
                        UINFO(9, "Dynamic becomes unsized array (subDType) " << dtp);
                    }
                    AstUnsizedArrayDType* ধ্রুৱ newp
                        = নূতন AstUnsizedArrayDType{unsizedp->fileline(), unsizedp->subDTypep()};
                    newp->dtypep(newp);
                    যদি (!np) {  // for Var itself
                        nodep->dtypep(newp);
                    } অন্যথা {  // for subDType
                        np->virtRefDTypep(newp);
                    }
                    v3Global.rootp()->typeTablep()->addTypesp(newp);
                    np = newp;
                } অন্যথা {
                    np = dtp->skipRefp();
                }
                dtp = np->virtRefDTypep();
            }
        }
        যদি (AstWildcardArrayDType* ধ্রুৱ wildp
            = VN_CAST(nodep->dtypeSkipRefp(), WildcardArrayDType)) {
            nodep->dtypep(wildp);  // Skip RefDType like for other dynamic array types
        }
        যদি (VN_IS(nodep->dtypep()->skipRefToConstp(), ConstDType)) nodep->isConst(সঁচা);
        // Parameters if implicit untyped inherit from what they are assigned to
        ধ্রুৱ AstBasicDType* ধ্রুৱ bdtypep = VN_CAST(nodep->dtypep(), BasicDType);
        তার্কিক didchk = মিছা;
        ধ্রুৱ তার্কিক implicitParam = nodep->isParam() && bdtypep && bdtypep->implicit();
        যদি (implicitParam) {
            যদি (nodep->valuep()) {
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
                যদি (nodep->valuep()->isDouble()) {
                    nodep->dtypeSetDouble();
                    VL_DANGLING(bdtypep);
                } অন্যথা যদি (nodep->valuep()->isString()) {
                    nodep->dtypeSetString();
                    VL_DANGLING(bdtypep);
                } অন্যথা {
                    পূর্ণাঙ্ক width = 0;
                    Ast_গ্ৰন্থি_DType* ধ্রুৱ মান_DTypep = nodep->valuep()->dtypep();
                    UASSERT_OBJ(মান_DTypep, nodep->valuep(),
                                "Null dtype on implicit param value");
                    ধ্রুৱ AstBasicDType* ধ্রুৱ মান_Bdtypep = মান_DTypep->basicp();
                    তার্কিক issigned = মিছা;
                    যদি (bdtypep->isNosign()) {
                        যদি (মান_Bdtypep && মান_Bdtypep->isSigned()) issigned = সঁচা;
                    } অন্যথা {
                        issigned = bdtypep->isSigned();
                    }
                    যদি (মান_Bdtypep->isString()) {
                        // parameter X = "str", per IEEE is a number, not a string
                        যদি (ধ্রুৱ auto* ধ্রুৱ constp = VN_CAST(nodep->valuep(), Const)) {
                            যদি (constp->অঙ্ক().isString()) {
                                width = constp->অঙ্ক().toString().length() * 8;
                            }
                        }
                        যদি (width < 8) width = 8;
                    } অন্যথা যদি (nodep->valuep()->dtypep()->widthSized()) {
                        width = nodep->valuep()->width();
                    } অন্যথা {
                        যদি (nodep->valuep()->width() > 32) {
                            nodep->valuep()->v3warn(
                                WIDTH,
                                "Assigning >32 bit to unranged parameter (defaults to 32 bits)");
                        }
                        width = 32;
                    }
                    // Can't just inherit valuep()->dtypep() as mwidth might not equal width
                    ধ্রুৱ তার্কিক is_মান_Fourstate
                        = nodep->valuep()->dtypep()->skipRefp()->isFourstate();
                    যদি (width == 1) {
                        // one bit parameter is same as "parameter [0] foo",
                        // not "parameter logic foo" as you can extract
                        // "foo[0]" from a parameter but not a wire
                        nodep->dtypep(nodep->findBitOrLogicRangeDType(
                            VNumRange{0, 0}, nodep->valuep()->widthMin(),
                            VSigning::fromBool(issigned), is_মান_Fourstate));
                    } অন্যথা {
                        nodep->dtypeSetBitOrLogicUnsized(width, nodep->valuep()->widthMin(),
                                                         VSigning::fromBool(issigned),
                                                         is_মান_Fourstate);
                    }
                    didchk = সঁচা;
                }
                iterateCheckAssign(nodep, "Initial value", nodep->valuep(), FINAL,
                                   nodep->dtypep());
                UINFO(9, "implicitParamFromIV " << nodep->valuep());
                // UINFO below will print variable nodep
            } অন্যথা {
                // Or, if nothing assigned, they're integral
                nodep->dtypeSetInteger();
                VL_DANGLING(bdtypep);
            }
        } অন্যথা যদি (bdtypep && bdtypep->implicit()) {  // Implicits get converted to size 1
            nodep->dtypeSetLogicSized(1, bdtypep->numeric());
            VL_DANGLING(bdtypep);
        }
        যদি (nodep->isNet()) {
            Ast_গ্ৰন্থি_DType* ধ্রুৱ badDtp = dtypeNot4StateIntegralRecurse(nodep->dtypep());
            যদি (badDtp)
                nodep->v3error(
                    "Net " << nodep->prettyNameQ()
                           << " data type must be 4-state integral or array/union/struct of such"
                           << " (IEEE 1800-2023 6.7.1)\n"
                           << nodep->warnContextPrimary() << '\n'
                           << badDtp->warnOther() << "... Location of failing data type "
                           << badDtp->prettyDTypeNameQ() << '\n'
                           << badDtp->warnContextSecondary());
        }
        যদি (nodep->valuep() && !didchk) {
            // UINFOTREE(1, nodep, "", "final");
            // AstPattern requires assignments to pass datatype on PRELIM
            userIterateAndNext(nodep->valuep(), WidthVP{nodep->dtypep(), PRELIM}.p());
            iterateCheckAssign(nodep, "Initial value", nodep->valuep(), FINAL, nodep->dtypep());
        }
        userIterateAndNext(nodep->delayp(), WidthVP{nodep->dtypep(), PRELIM}.p());
        UINFO(4, "varWidthed " << nodep);
        // UINFOTREE(1, nodep, "", "InitOut");
        nodep->didWidth(সঁচা);
        nodep->doingWidth(মিছা);
    }
    শূন্য visit(Ast_গ্ৰন্থি_Var_Ref* nodep) override {
        যদি (nodep->didWidth()) ঘূৰাই;
        যদি (!nodep->varp()) {
            যদি (m_paramsOnly && VN_IS(nodep, VarXRef)) {
                checkConstantOrReplace(
                    nodep, মিছা,
                    "Parameter-resolved constants must not use dotted references: "
                        + nodep->prettyNameQ());
                VL_DANGLING(nodep);
                ঘূৰাই;
            } অন্যথা {
                nodep->v3fatalSrc("Unlinked varref");
            }
        }
        যদি (!nodep->varp()->didWidth()) {
            // Var hasn't been widthed, so make it so.
            userIterate(nodep->varp(), nullptr);
        }
        যদি (!m_paramsOnly && nodep->varp()->isIfaceArraySplit()) {
            replaceSplitIfaceArrayRef(nodep);
            ঘূৰাই;
        }
        // UINFOTREE(9, nodep, "", "VRin");
        // UINFOTREE(9, nodep->varp(), "", "forvar");
        // Note genvar's are also entered as integers
        nodep->dtypeFrom(nodep->varp());
        যদি (VN_IS(nodep->backp(), গ্ৰন্থি_Assign) && nodep->access().isWriteOrRW()) {  // On LHS
            UASSERT_OBJ(nodep->dtypep(), nodep, "LHS var should be dtype completed");
        }
        // UINFOTREE(9, nodep, "", "VRout");
        যদি (nodep->access().isWriteOrRW()) nodep->varp()->icoMaybeWritten(সঁচা);
        যদি (nodep->access().isWriteOrRW() && nodep->varp()->direction() == VDirection::CONSTREF) {
            nodep->v3error("Assigning to const ref variable: " << nodep->prettyNameQ());
        } অন্যথা যদি (nodep->access().isWriteOrRW() && nodep->varp()->is_প্ৰবেশ()
                   && !nodep->varp()->isFuncLocal() && nodep->varp()->isReadOnly()
                   && (!m_ftaskp || !m_ftaskp->isConstructor())
                   && !VN_IS(m_procedurep, InitialAutomatic) && !VN_IS(m_procedurep, InitialStatic)
                   && !VN_IS(nodep->abovep(), AssignForce) && !VN_IS(nodep->abovep(), Release)) {
            // Skip ASSIGNIN for continuous assignments to net-type input ports
            // via hierarchical reference. Net ports allow multiple continuous
            // drivers (IEEE 1800-2023 23.3.3.3). Input ports default to net
            // when port kind is omitted (23.2.2.3, PORT type).
            ধ্রুৱ তার্কিক hierRef = !m_curModVars.গুনতি(nodep->varp());
            ধ্রুৱ তার্কিক netPort
                = nodep->varp()->isNet() || nodep->varp()->varType() == VVarType::PORT;
            ধ্রুৱ তার্কিক contAssign = VN_IS(nodep->abovep(), AssignW);
            ধ্রুৱ তার্কিক pinConn = VN_IS(nodep->abovep(), Pin);
            যদি (!(hierRef && netPort && (contAssign || pinConn))) {
                nodep->v3warn(ASSIGNIN,
                              "Assigning to input/const variable: " << nodep->prettyNameQ());
            }
        } অন্যথা যদি (nodep->access().isWriteOrRW() && nodep->varp()->isConst() && !m_paramsOnly
                   && (!m_ftaskp || !m_ftaskp->isConstructor())
                   && !VN_IS(m_procedurep, InitialAutomatic) && !VN_IS(m_procedurep, InitialStatic)
                   && !m_underMemberSel) {
            // Too loose, but need to allow our generated first assignment
            // Move this to a property of the AstInitial block
            nodep->v3warn(E_CONSTWRITTEN, "Writing to 'const' data-typed variable "
                                              << nodep->prettyNameQ()
                                              << " (IEEE 1800-2023 6.20.6)");
        }
        যদি (nodep->varp()->isClassMember() && !nodep->varp()->isFuncLocal()
            && !nodep->varp()->lifetime().isStatic() && m_ftaskp && m_ftaskp->isStatic()) {
            nodep->v3error("Cannot access non-static member variable "
                           << nodep->prettyNameQ() << " from a static method "
                           << m_ftaskp->prettyNameQ() << " without object (IEEE 1800-2023 8.10)");
        }
        nodep->didWidth(সঁচা);
    }

    শূন্য visit(AstEnumDType* nodep) override {
        যদি (nodep->didWidthAndSet()) ঘূৰাই;  // This node is a dtype & not both PRELIMed+FINALed
        UINFO(5, "  ENUMDTYPE " << nodep);
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        nodep->dtypep(nodep);
        Ast_গ্ৰন্থি_DType* basicp = nodep->dtypep()->skipRefp()->basicp();
        Ast_গ্ৰন্থি_DType* ধ্রুৱ badDtp = dtypeNotIntAtomOrVecRecurse(nodep->subDTypep());
        যদি (badDtp) {
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
        V3_সংখ্যা অঙ্ক(nodep, nodep->width(), 0);
        ধ্রুৱ V3_সংখ্যা one{nodep, nodep->width(), 1};
        তার্কিক wrapAround = মিছা;
        std::map<ধ্রুৱ V3_সংখ্যা, AstEnumItem*> inits;
        বাবে (AstEnumItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), EnumItem)) {
            যদি (itemp->valuep()) {
                যদি (debug() >= 9) {
                    UINFO(0, "EnumInit " << itemp);
                    itemp->valuep()->dump_বৃক্ষ("-  EnumInit: ");
                }
                V3Const::constifyParamsEdit(itemp->valuep());  // itemp may change
                যদি (!VN_IS(itemp->valuep(), Const)) {
                    itemp->valuep()->v3error("Enum value isn't a constant");
                    itemp->valuep()->unlinkFrBack()->delete_বৃক্ষ();
                    অনুৱর্ত;
                }
                // TODO IEEE says assigning sized number that is not same size as enum is illegal
                wrapAround = মিছা;  // Explicit value resets wrap-around tracking
            }
            যদি (!itemp->valuep()) {
                যদি (wrapAround) {
                    itemp->v3error("Enum value illegally wrapped around (IEEE 1800-2023 6.19)");
                }
                যদি (অঙ্ক.isFourState()) {
                    itemp->v3error("Enum value that is unassigned cannot follow value with X/Zs "
                                   "(IEEE 1800-2023 6.19)");
                }
                itemp->valuep(নূতন AstConst{itemp->fileline(), অঙ্ক});
            }

            ধ্রুৱ AstConst* ধ্রুৱ constp = VN_AS(itemp->valuep(), Const);
            যদি (constp->অঙ্ক().isFourState() && basicp->basicp() && !basicp->isFourstate()) {
                itemp->v3error("Enum value with X/Zs cannot be assigned to non-fourstate type "
                               "(IEEE 1800-2023 6.19)");
            }
            অঙ্ক.opAssign(constp->অঙ্ক());
            // Look for duplicates
            ধ্রুৱ auto pair = inits.emplace(অঙ্ক, itemp);
            যদি (!pair.second) {  // IEEE says illegal
                ধ্রুৱ Ast_গ্ৰন্থি* ধ্রুৱ otherp = pair.first->second;
                itemp->v3error("Overlapping enumeration value: "
                               << itemp->prettyNameQ() << '\n'
                               << itemp->warnContextPrimary() << '\n'
                               << otherp->warnOther() << "... Location of original declaration\n"
                               << otherp->warnContextSecondary());
            }
            // Detect wrap-around after increment for the next auto-valued item.
            // For signed types, overflow is positive-to-negative (e.g., INT_MAX+1).
            // For unsigned types, overflow wraps to zero (e.g., UINT_MAX+1).
            ধ্রুৱ তার্কিক prevNeg = constp->অঙ্ক().isNegative();
            অঙ্ক.opAdd(one, constp->অঙ্ক());
            যদি (basicp->isSigned()) {
                wrapAround = !prevNeg && অঙ্ক.isNegative();
            } অন্যথা {
                wrapAround = অঙ্ক.isEqZero();
            }
        }
    }
    শূন্য visit(AstEnumItem* nodep) override {
        UINFO(5, "   ENUMITEM " << nodep);
        VL_RESTORER(m_enumItemp);
        m_enumItemp = nodep;
        Ast_গ্ৰন্থি_DType* ধ্রুৱ vdtypep = m_vup->dtypep();
        UASSERT_OBJ(vdtypep, nodep, "ENUMITEM not under ENUM");
        nodep->dtypep(vdtypep);
        যদি (nodep->valuep()) {  // else the value will be assigned sequentially
            // Default type is int, but common to assign narrower values, so minwidth from value
            userIterateAndNext(nodep->valuep(), WidthVP{CONTEXT_DET, PRELIM}.p());
            তার্কিক warnOn = সঁচা;
            Ast_গ্ৰন্থি_ৰাশিমালা* valuep = nodep->valuep();
            যদি (ধ্রুৱ AstAdd* ধ্রুৱ anodep = VN_CAST(valuep, Add)) {
                // If constructed by V3LinkParse due to "enumitem[N_REPEATS] value"
                যদি (anodep->fileline()->equal_First_সাৰি_Col(*(nodep->fileline())))
                    valuep = anodep->lhsp();
            }
            যদি (ধ্রুৱ AstConst* ধ্রুৱ constp = VN_CAST(valuep, Const)) {
                যদি (static_cast<পূর্ণাঙ্ক>(constp->অঙ্ক().mostSetBitP1()) > nodep->width()) {
                    constp->v3warn(ENUMITEMWIDTH,
                                   "Enum value exceeds width of enum type (IEEE 1800-2023 6.19)");
                    warnOn = মিছা;  // Prevent normal WIDTHTRUNC
                }
            }
            // Minwidth does not come from value, as spec says set based on parent
            // and if we keep minwidth we'll consider it unsized which is incorrect
            iterateCheck(nodep, "Enum value", nodep->valuep(), CONTEXT_DET, FINAL, nodep->dtypep(),
                         EXTEND_EXP, warnOn);
            // Always create a cast, to avoid later ENUMVALUE warnings
            nodep->valuep(নূতন AstCast{nodep->valuep()->fileline(), nodep->valuep()->unlinkFrBack(),
                                      nodep->dtypep()});
        }
    }
    শূন্য visit(AstEnumItemRef* nodep) override {
        UASSERT_OBJ(nodep->itemp(), nodep, "Unlinked");
        যদি (!nodep->itemp()->didWidth()) {
            // We need to do the whole enum en masse
            Ast_গ্ৰন্থি* enump = nodep->itemp();
            UASSERT_OBJ(enump, nodep, "EnumItemRef not linked");
            বাবে (; enump; enump = enump->backp()) {
                যদি (VN_IS(enump, EnumDType)) ভঙ্গ;
            }
            UASSERT_OBJ(enump, nodep, "EnumItemRef can't deref back to an Enum");
            VL_DO_DANGLING(userIterate(enump, m_vup), enump);  // Parent enump maybe relinked
        }
        nodep->dtypeFrom(nodep->itemp());
    }
    শূন্য visit(Ast_গ্ৰন্থি_Cons* nodep) override {}  // Widthed when created
    শূন্য visit(Ast_গ্ৰন্থি_Cons_Item* nodep) override {}  // Widthed when created
    শূন্য visit(AstDist* nodep) override {
        //  x dist {a :/ p, b :/ q} --> (p > 0 && x == a) || (q > 0 && x == b)
        // (only outside constraints; inside constraints V3Randomize handles weighted selection)
        userIterateAndNext(nodep->exprp(), WidthVP{CONTEXT_DET, PRELIM}.p());
        বাবে (Ast_গ্ৰন্থি *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            nextip = itemp->nextp();  // iterate may cause the node to get replaced
            VL_DO_DANGLING(userIterate(itemp, WidthVP{CONTEXT_DET, PRELIM}.p()), itemp);
        }

        AstBasicDType* dtype = VN_CAST(nodep->exprp()->dtypep(), BasicDType);
        Ast_গ্ৰন্থি_DType* subDTypep = nullptr;
        nodep->dtypeSetBit();

        যদি (dtype && dtype->isString()) {
            nodep->dtypeSetString();
            subDTypep = nodep->findStringDType();
        } অন্যথা যদি (dtype && dtype->isDouble()) {
            nodep->dtypeSetDouble();
            subDTypep = nodep->findDoubleDType();
        } অন্যথা {
            // Take width as maximum across all items
            পূর্ণাঙ্ক width = nodep->exprp()->width();
            পূর্ণাঙ্ক mwidth = nodep->exprp()->widthMin();
            তার্কিক isFourstate = nodep->exprp()->dtypep()->isFourstate();
            বাবে (ধ্রুৱ Ast_গ্ৰন্থি* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
                width = std::max(width, itemp->width());
                mwidth = std::max(mwidth, itemp->widthMin());
                isFourstate |= itemp->dtypep()->isFourstate();
            }
            subDTypep = nodep->findBitOrLogicDType(
                width, mwidth, nodep->exprp()->dtypep()->numeric(), isFourstate);
        }

        iterateCheck(nodep, "Dist expression", nodep->exprp(), CONTEXT_DET, FINAL, subDTypep,
                     EXTEND_EXP);
        বাবে (Ast_গ্ৰন্থি *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            nextip = itemp->nextp();
            itemp = VN_AS(itemp, DistItem)->rangep();
            যদি (VN_IS(itemp, InsideRange)) {
                userIterate(itemp, WidthVP{subDTypep, FINAL}.p());
            } অন্যথা {
                iterateCheck(nodep, "Dist Item", itemp, CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP);
            }
        }

        // Inside a constraint, V3Randomize handles dist lowering with proper weights,
        // but only for simple scalar/range items. Container-type items (queues, arrays)
        // must be lowered here via insideItem() which knows how to expand them.
        যদি (m_constraintp) {
            তার্কিক canLower = সঁচা;
            বাবে (AstDistItem* ditemp = nodep->itemsp(); ditemp;
                 ditemp = VN_AS(ditemp->nextp(), DistItem)) {
                যদি (VN_IS(ditemp->rangep(), InsideRange)) অনুৱর্ত;
                // An integral item is a plain equality on either path, so it may be
                // non-constant.  Testing for AstConst here instead would silently drop
                // the weights of a dist whose items are state variables.  Compound
                // items (containers, strings) still need insideItem(), and an impure
                // item must not be cloned into the bucket chain.
                ধ্রুৱ Ast_গ্ৰন্থি_DType* ধ্রুৱ itemDtp = ditemp->rangep()->dtypep()->skipRefp();
                যদি (!itemDtp->isIntegralOrPacked() || !ditemp->rangep()->isPure()) {
                    canLower = মিছা;
                    ভঙ্গ;
                }
            }
            যদি (canLower) ঘূৰাই;
        }

        // Outside constraint: lower to inside (ignores weights)
        Ast_গ্ৰন্থি_ৰাশিমালা* newp = nullptr;
        বাবে (AstDistItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), DistItem)) {
            Ast_গ্ৰন্থি_ৰাশিমালা* inewp
                = insideItem(nodep, nodep->exprp()->clone_বৃক্ষ_Pure(মিছা), itemp->rangep());
            যদি (!inewp) অনুৱর্ত;
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ cmpp
                = নূতন AstGt{itemp->fileline(), itemp->weightp()->unlinkFrBack(),
                            নূতন AstConst{itemp->fileline(), 0}};
            cmpp->fileline()->modifyWarnOff(V3ErrorCode::UNSIGNED, সঁচা);
            cmpp->fileline()->modifyWarnOff(V3ErrorCode::CMPCONST, সঁচা);
            inewp = নূতন AstLogAnd{itemp->fileline(), cmpp, inewp};
            newp = newp ? নূতন AstLogOr{nodep->fileline(), newp, inewp} : inewp;
        }
        যদি (!newp) newp = নূতন AstConst{nodep->fileline(), AstConst::BitFalse{}};

        UINFOTREE(9, nodep, "", "dist-out");
        nodep->replaceWith(newp);
        VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
    }

    শূন্য visit(AstInside* nodep) override {
        userIterateAndNext(nodep->exprp(), WidthVP{CONTEXT_DET, PRELIM}.p());
        বাবে (Ast_গ্ৰন্থি *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            nextip = itemp->nextp();  // iterate may cause the node to get replaced
            VL_DO_DANGLING(userIterate(itemp, WidthVP{CONTEXT_DET, PRELIM}.p()), itemp);
        }

        AstBasicDType* dtype = VN_CAST(nodep->exprp()->dtypep(), BasicDType);
        Ast_গ্ৰন্থি_DType* expDTypep = nullptr;

        যদি (dtype && dtype->isString()) {
            nodep->dtypeSetString();
            expDTypep = nodep->findStringDType();
        } অন্যথা যদি (dtype && dtype->isDouble()) {
            nodep->dtypeSetDouble();
            expDTypep = nodep->findDoubleDType();
        } অন্যথা {
            // Take width as maximum across all items
            পূর্ণাঙ্ক width = nodep->exprp()->width();
            পূর্ণাঙ্ক mwidth = nodep->exprp()->widthMin();
            তার্কিক isFourstate = nodep->exprp()->dtypep()->isFourstate();
            বাবে (ধ্রুৱ Ast_গ্ৰন্থি* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
                width = std::max(width, itemp->width());
                mwidth = std::max(mwidth, itemp->widthMin());
                isFourstate |= itemp->dtypep()->isFourstate();
            }
            nodep->dtypeSetBit();
            ধ্রুৱ VSigning numeric = nodep->exprp()->dtypep()->numeric();
            expDTypep = nodep->findBitOrLogicDType(width, mwidth, numeric, isFourstate);
        }

        iterateCheck(nodep, "Inside expression", nodep->exprp(), CONTEXT_DET, FINAL, expDTypep,
                     EXTEND_EXP);
        বাবে (Ast_গ্ৰন্থি *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            nextip = itemp->nextp();  // iterate may cause the node to get replaced
            যদি (VN_IS(itemp, InsideRange)) {
                userIterate(itemp, WidthVP{expDTypep, FINAL}.p());
            } অন্যথা যদি (!itemp->dtypep()->skipRefp()->isNonPackedArray()) {
                iterateCheck(nodep, "Inside Item", itemp, CONTEXT_DET, FINAL, expDTypep,
                             EXTEND_EXP);
            }
        }

        Ast_গ্ৰন্থি_ৰাশিমালা* exprp;
        Ast_ৰাশিমালা_Stmt* ৰাশিমালা_Stmtp = nullptr;
        // Skip constraints since those are declarative expressions and they are never actually
        // executed so, there is no need for purification since they cannot generate sideeffects.
        যদি (!m_constraintp && !nodep->exprp()->isPure()) {
            File_সাৰি* ধ্রুৱ fl = nodep->exprp()->fileline();
            // Ensure sized dtype for temp variable
            Ast_গ্ৰন্থি_DType* ধ্রুৱ ৰাশিমালা_Dtp = nodep->exprp()->dtypep();
            ধ্রুৱ পূর্ণাঙ্ক w = ৰাশিমালা_Dtp->width();
            Ast_গ্ৰন্থি_DType* ধ্রুৱ tempDTypep
                = ৰাশিমালা_Dtp->widthSized() ? ৰাশিমালা_Dtp
                                        : nodep->findBitOrLogicDType(w, w, ৰাশিমালা_Dtp->numeric(),
                                                                     ৰাশিমালা_Dtp->isFourstate());
            AstVar* ধ্রুৱ varp
                = নূতন AstVar{fl, VVarType::XTEMP, m_insideTempNames.get(nodep), tempDTypep};
            exprp = নূতন AstVarRef{fl, varp, VAccess::READ};
            ৰাশিমালা_Stmtp = নূতন Ast_ৰাশিমালা_Stmt{fl,
                                        নূতন AstAssign{fl, নূতন AstVarRef{fl, varp, VAccess::WRITE},
                                                      nodep->exprp()->unlinkFrBack()},
                                        exprp};
            যদি (m_ftaskp) {
                varp->funcLocal(সঁচা);
                varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                m_ftaskp->prependStmtsp(varp);
            } অন্যথা {
                m_modep->prependStmtsp(varp);
            }
            iterate(varp);
            iterate(ৰাশিমালা_Stmtp);
        } অন্যথা {
            exprp = nodep->exprp();
        }
        UINFOTREE(9, nodep, "", "inside-in");
        // Now rip out the inside and replace with simple math
        Ast_গ্ৰন্থি_ৰাশিমালা* newp = nullptr;
        বাবে (Ast_গ্ৰন্থি_ৰাশিমালা *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            nextip = VN_AS(itemp->nextp(), গ্ৰন্থি_ৰাশিমালা);  // Will be unlinking
            Ast_গ্ৰন্থি_ৰাশিমালা* inewp;
            যদি (ৰাশিমালা_Stmtp) {
                inewp = insideItem(nodep, ৰাশিমালা_Stmtp, itemp);
                ৰাশিমালা_Stmtp = nullptr;
            } অন্যথা {
                inewp = insideItem(nodep, exprp->clone_বৃক্ষ_Pure(মিছা), itemp);
            }
            যদি (!inewp) অনুৱর্ত;
            newp = newp ? নূতন AstLogOr{nodep->fileline(), newp, inewp} : inewp;
        }
        যদি (ৰাশিমালা_Stmtp) VL_DO_DANGLING(ৰাশিমালা_Stmtp->delete_বৃক্ষ(), ৰাশিমালা_Stmtp);
        যদি (!newp) newp = নূতন AstConst{nodep->fileline(), AstConst::BitFalse{}};
        UINFOTREE(9, newp, "", "inside-out");
        nodep->replaceWith(newp);
        VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
    }
    Ast_গ্ৰন্থি_ৰাশিমালা* insideItem(Ast_গ্ৰন্থি* nodep, Ast_গ্ৰন্থি_ৰাশিমালা* exprp, Ast_গ্ৰন্থি_ৰাশিমালা* itemp) {
        ধ্রুৱ Ast_গ্ৰন্থি_DType* ধ্রুৱ itemDtp = itemp->dtypep()->skipRefp();
        যদি (AstInsideRange* ধ্রুৱ irangep = VN_CAST(itemp, InsideRange)) {
            // Similar logic in V3Case
            ঘূৰাই irangep->newAndFromInside(exprp, irangep->lhsp()->unlinkFrBack(),
                                             irangep->rhsp()->unlinkFrBack());
        } অন্যথা যদি (VN_IS(itemDtp, UnpackArrayDType) || VN_IS(itemDtp, DynArrayDType)
                   || VN_IS(itemDtp, QueueDType)) {
            // Unsupported in parameters
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ inewp = নূতন AstCMethodHard{nodep->fileline(), itemp->unlinkFrBack(),
                                                          VCMethod::ARRAY_INSIDE, exprp};
            iterateCheckTyped(nodep, "inside value", exprp, itemDtp->subDTypep(), BOTH);
            inewp->dtypeSetBit();
            inewp->didWidth(সঁচা);
            ঘূৰাই inewp;
        } অন্যথা যদি (VN_IS(itemDtp, AssocArrayDType)) {
            nodep->v3error("Inside operator not specified on associative arrays "
                           "(IEEE 1800-2023 11.4.13)");
            VL_DO_DANGLING(exprp->delete_বৃক্ষ(), exprp);
            ঘূৰাই nullptr;
        }
        ঘূৰাই AstEqWild::newTyped(itemp->fileline(), exprp, itemp->unlinkFrBack());
    }
    শূন্য visit(AstInsideRange* nodep) override {
        // Just do each side; AstInside will rip these nodes out later.
        যদি (!m_vup) {
            // This range appears outside a normal expression context (e.g. in a covergroup
            // bin declaration).  Width the bounds self-determined first so each bound (and any
            // referenced parameter) has a dtype, then constify so constant arithmetic is folded
            // (e.g. AstNegate(Const) -> Const, or a parameter reference -> Const).  Folding
            // preserves the now-present dtype, so no bound reaches V3WidthCommit without one.
            userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
            userIterateAndNext(nodep->rhsp(), WidthVP{SELF, BOTH}.p());
            V3Const::constifyEdit(nodep->lhsp());  // lhsp may change
            V3Const::constifyEdit(nodep->rhsp());  // rhsp may change
        } অন্যথা {
            যদি (m_vup->prelim()) {
                userIterateAndNext(nodep->lhsp(), m_vup);
                userIterateAndNext(nodep->rhsp(), m_vup);
            }
            যদি (m_vup->final()) {
                Ast_গ্ৰন্থি_DType* ধ্রুৱ expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
                // Warning waivers match visit_cmp_eq_gt on the lowered Gte/Lte
                ধ্রুৱ পূর্ণাঙ্ক expWidth = expDTypep->width();
                ধ্রুৱ তার্কিক waiveLhs = expWidth == 32
                                      && !(expDTypep->isSigned() && nodep->lhsp()->isSigned())
                                      && expDTypep->widthMin() >= nodep->lhsp()->width();
                ধ্রুৱ তার্কিক waiveRhs = expWidth == 32
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

    শূন্য visit(AstIfaceRefDType* nodep) override {
        যদি (nodep->didWidthAndSet()) ঘূৰাই;  // This node is a dtype & not both PRELIMed+FINALed
        UINFO(5, "   IFACEREF " << nodep);
        userIterateChildren(nodep, m_vup);
        nodep->dtypep(nodep);
        যদি (nodep->isVirtual()) nodep->ifaceViaCellp()->setHasVirtualRef();
        UINFO(4, "dtWidthed " << nodep);
    }
    শূন্য visit(Ast_গ্ৰন্থি_UOr_Struct_DType* nodep) override {
        যদি (nodep->doingWidth()) {  // Early exit if have circular parameter definition
            nodep->v3error("Struct's type is circular: " << nodep->prettyName());
            nodep->dtypeSetBit();
            nodep->doingWidth(মিছা);
            ঘূৰাই;
        }
        যদি (nodep->didWidthAndSet()) ঘূৰাই;  // This node is a dtype & not both PRELIMed+FINALed
        nodep->doingWidth(সঁচা);
        UINFO(5, "   NODEUORS " << nodep);
        // Check for tagged unions
        যদি (ধ্রুৱ AstUnionDType* ধ্রুৱ unionp = VN_CAST(nodep, UnionDType)) {
            যদি (unionp->isTagged()) { nodep->v3warn(E_UNSUPPORTED, "Unsupported: tagged union"); }
        }
        // UINFOTREE(9, nodep, "", "class-in");
        userIterateChildren(nodep, nullptr);  // First size all members
        nodep->dtypep(nodep);
        nodep->isFourstate(মিছা);
        // Error checks
        বাবে (AstMemberDType* itemp = nodep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            Ast_গ্ৰন্থি_DType* ধ্রুৱ dtp = itemp->subDTypep()->skipRefp();
            যদি (nodep->packed() && !dtp->isIntegralOrPacked())
                itemp->v3error("Unpacked data type "
                               << dtp->prettyDTypeNameQ()
                               << " in packed struct/union (IEEE 1800-2023 7.2.1)");
            যদি ((VN_IS(nodep, UnionDType) || nodep->packed()) && itemp->valuep()) {
                itemp->v3error("Initial values not allowed in packed struct/union"
                               " (IEEE 1800-2023 7.2.2)");
                ঠেলো_Deletep(itemp->valuep()->unlinkFrBack());
            }
        }
        ধ্রুৱ তার্কিক isHardPackedUnion
            = nodep->packed() && VN_IS(nodep, UnionDType) && !VN_CAST(nodep, UnionDType)->isSoft();

        // Suppress union size errors in parameterized template modules where member
        // widths depend on unresolved parameters.  Also suppress when the type has no
        // owning module (e.g. moved to TypeTable during DepGraph resolution).
        // TODO: Revisit this gate if DepGraph becomes the sole flow and widthing
        // can assume all types are already specialized.
        ধ্রুৱ তার্কিক inTemplateModule = (m_modep && m_modep->parameterizedTemplate())
                                      || (VN_IS(nodep, UnionDType) && !m_modep);

        // Determine bit assignments and width
        যদি (VN_IS(nodep, UnionDType) || nodep->packed()) {
            পূর্ণাঙ্ক lsb = 0;
            পূর্ণাঙ্ক width = 0;
            তার্কিক first = সঁচা;
            // Report errors on first member first
            AstMemberDType* itemp;
            // MSB is first, so loop backwards
            বাবে (itemp = nodep->membersp(); itemp && itemp->nextp();
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {}
            বাবে (; itemp; itemp = VN_CAST(itemp->backp(), MemberDType)) {
                যদি (itemp->isFourstate()) nodep->isFourstate(সঁচা);
                itemp->lsb(lsb);
                যদি (VN_IS(nodep, UnionDType)) {
                    ধ্রুৱ পূর্ণাঙ্ক itemWidth = itemp->width();
                    // Skip union size check for template modules with unresolved parameters
                    যদি (!first && isHardPackedUnion && itemWidth != width && !inTemplateModule) {
                        itemp->v3error("Hard packed union members must have equal size "
                                       "(IEEE 1800-2023 7.3.1)");
                    }
                    width = std::max(width, itemWidth);
                } অন্যথা {
                    lsb += itemp->width();
                    width += itemp->width();
                }
                first = মিছা;
            }
            nodep->widthForce(width, width);  // Signing stays as-is, as parsed from declaration
        } অন্যথা {
            nodep->widthForce(1, 1);
        }
        nodep->doingWidth(মিছা);
        // UINFOTREE(9, nodep, "", "class-out");
    }
    শূন্য visit(AstThisRef* nodep) override {
        যদি (nodep->didWidthAndSet()) ঘূৰাই;
        nodep->dtypep(iterateEditMoveDTypep(nodep, nodep->childDTypep()));
        যদি (m_ftaskp && m_ftaskp->isStatic()) {
            nodep->v3error("Cannot use 'this' in a static method "
                           << m_ftaskp->prettyNameQ() << " (IEEE 1800-2023 8.10-8.11)");
        }
    }
    শূন্য visit(AstClassRefDType* nodep) override {
        যদি (nodep->didWidthAndSet()) ঘূৰাই;
        // TODO this maybe eventually required to properly resolve members,
        // though causes problems with t_class_forward.v, so for now avoided
        // userIterateChildren(nodep->classp(), nullptr);
    }
    শূন্য visit(AstClassOrPackageRef* nodep) override {
        যদি (nodep->didWidthAndSet()) ঘূৰাই;
        userIterateChildren(nodep, nullptr);
    }
    শূন্য visit(AstDot* nodep) override {
        // We can only reach this from constify called during V3Param (so before linkDotParam)
        // ... #(Cls#(...)::...) ...
        //                ^^~~~ this is our DOT
        nodep->v3warn(E_UNSUPPORTED, "dotted expressions in parameters\n"
                                         << nodep->warnMore() << "... Suggest use a typedef");
        nodep->replaceWith(নূতন AstConst{nodep->fileline(), AstConst::BitFalseErroring{}});
        VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
    }
    শূন্য visit(AstClassExtends* nodep) override {
        যদি (nodep->didWidthAndSet()) ঘূৰাই;
        যদি (VN_IS(nodep->childDTypep(), ClassRefDType)) {
            nodep->dtypep(iterateEditMoveDTypep(nodep, nodep->childDTypep()));
        }
    }
    শূন্য visit(AstMemberDType* nodep) override {
        যদি (nodep->didWidthAndSet()) ঘূৰাই;  // This node is a dtype & not both PRELIMed+FINALed
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        nodep->widthFromSub(nodep->subDTypep());
        nodep->dtypeFrom(nodep->subDTypep());
        যদি (nodep->valuep()) {
            userIterateAndNext(nodep->valuep(), WidthVP{nodep->dtypep(), PRELIM}.p());
            iterateCheckAssign(nodep, "Initial value", nodep->valuep(), FINAL, nodep->dtypep());
        }
    }
    শূন্য visit(AstStructSel* nodep) override {
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
    }
    শূন্য visit(AstMemberSel* nodep) override {
        UINFO(5, "   MEMBERSEL " << nodep);
        যদি (nodep->didWidth()) ঘূৰাই;
        VL_RESTORER(m_underMemberSel);
        m_underMemberSel = সঁচা;
        UINFOTREE(9, nodep, "", "mbs-in");
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
        UINFOTREE(9, nodep, "", "mbs-ic");
        // Find the fromp dtype - should be a class
        UASSERT_OBJ(nodep->fromp()->dtypep(), nodep->fromp(), "Unlinked data type");
        Ast_গ্ৰন্থি_DType* ধ্রুৱ fromDtp = nodep->fromp()->dtypep()->skipRefToEnump();
        UINFO(9, "     from dt " << fromDtp);
        ধ্রুৱ AstMemberSel* ধ্রুৱ fromSel = VN_CAST(nodep->fromp(), MemberSel);
        যদি (AstClocking* ধ্রুৱ clockingp = fromSel && fromSel->varp()
                                               ? VN_CAST(fromSel->varp()->firstAbovep(), Clocking)
                                               : nullptr) {
            // In:  MEMBERSEL{MEMBERSEL{vifaceref, "cb", cb_event}, "clockvar", null}
            // Out: MEMBERSEL{vifaceref, "clockvar", clockvar}
            UINFO(9, "     from clocking " << clockingp);
            যদি (AstVar* ধ্রুৱ varp = memberSelClocking(nodep, clockingp)) {
                যদি (!varp->didWidth()) userIterate(varp, nullptr);
                AstMemberSel* fromp = VN_AS(nodep->fromp(), MemberSel);
                fromp->replaceWith(fromp->fromp()->unlinkFrBack());
                VL_DO_DANGLING(fromp->delete_বৃক্ষ(), fromp);
                nodep->dtypep(varp->dtypep());
                nodep->varp(varp);
                যদি (nodep->access().isWriteOrRW()) {
                    V3LinkLValue::linkLValueSet(nodep, nodep->access());
                }
                যদি (AstIfaceRefDType* ধ্রুৱ adtypep
                    = VN_CAST(nodep->fromp()->dtypep()->skipRefp(), IfaceRefDType)) {
                    nodep->varp()->sensIfacep(adtypep->ifacep());
                }
                UINFO(9, "     done clocking msel " << nodep);
                nodep->didWidth(সঁচা);  // Must not visit again: will confuse scopes
                ঘূৰাই;
            }
        } অন্যথা যদি (Ast_গ্ৰন্থি_UOr_Struct_DType* ধ্রুৱ adtypep = VN_CAST(fromDtp, গ্ৰন্থি_UOr_Struct_DType)) {
            যদি (memberSelStruct(nodep, adtypep)) ঘূৰাই;
        } অন্যথা যদি (AstClassRefDType* ধ্রুৱ adtypep = VN_CAST(fromDtp, ClassRefDType)) {
            যদি (memberSelClass(nodep, adtypep)) ঘূৰাই;
        } অন্যথা যদি (AstIfaceRefDType* ধ্রুৱ adtypep = VN_CAST(fromDtp, IfaceRefDType)) {
            যদি (Ast_গ্ৰন্থি* foundp = memberSelIface(nodep, adtypep)) {
                যদি (AstClocking* ধ্রুৱ clockingp = VN_CAST(foundp, Clocking))
                    foundp = clockingp->ensureEventp();
                যদি (AstVar* ধ্রুৱ varp = VN_CAST(foundp, Var)) {
                    যদি (!varp->didWidth()) userIterate(varp, nullptr);
                    AstIface* ধ্রুৱ ifacep = adtypep->ifaceViaCellp();
                    varp->sensIfacep(ifacep);
                    // Member of a non-virtual interface reference, e.g. an element selected
                    // from a whole array of interfaces: reference it through the interface,
                    // as V3LinkDot does for 'iface.member'
                    ধ্রুৱ AstVarRef* ধ্রুৱ refp = VN_CAST(nodep->fromp(), VarRef);
                    যদি (refp && !adtypep->isVirtual() && refp->varp()->isIfaceRef()) {
                        AstVarXRef* ধ্রুৱ newp = নূতন AstVarXRef{nodep->fileline(), varp,
                                                                refp->name(), nodep->access()};
                        newp->didWidth(সঁচা);
                        nodep->replaceWith(newp);
                        VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
                        ঘূৰাই;
                    }
                    nodep->dtypep(foundp->dtypep());
                    nodep->varp(varp);
                    nodep->didWidth(সঁচা);
                    ঘূৰাই;
                }
                যদি (AstModport* ধ্রুৱ modportp = VN_CAST(foundp, Modport)) {
                    // Modport selection (e.g. vif.passive_mp) is compile-time
                    // type narrowing: replace MemberSel with fromp re-typed.
                    AstIfaceRefDType* ধ্রুৱ newDtypep = নূতন AstIfaceRefDType{
                        nodep->fileline(), nodep->fileline(), adtypep->cellName(),
                        adtypep->ifaceName(), modportp->name()};
                    newDtypep->ifacep(adtypep->ifacep());
                    newDtypep->cellp(adtypep->cellp());
                    newDtypep->modportp(modportp);
                    newDtypep->isVirtual(adtypep->isVirtual());
                    v3Global.rootp()->typeTablep()->addTypesp(newDtypep);
                    Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ fromp = nodep->fromp()->unlinkFrBack();
                    fromp->dtypep(newDtypep);
                    nodep->replaceWith(fromp);
                    VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
                    ঘূৰাই;
                }
                যদি (AstCell* ধ্রুৱ cellp = VN_CAST(foundp, Cell)) {
                    // Sub-interface cell selection (e.g. vif.tx): resolve to the
                    // companion __Viftop var created by V3LinkCells for its dtype.
                    যদি (VN_IS(cellp->modp(), Iface)) {
                        ধ্রুৱ string viftopName = cellp->name() + "__Viftop";
                        Ast_গ্ৰন্থি_Module* ধ্রুৱ parentIfacep = adtypep->ifaceViaCellp();
                        AstVar* viftopVarp = nullptr;
                        বাবে (Ast_গ্ৰন্থি* itemp = parentIfacep->stmtsp(); itemp;
                             itemp = itemp->nextp()) {
                            যদি (AstVar* ধ্রুৱ vp = VN_CAST(itemp, Var)) {
                                যদি (vp->name() == viftopName) {
                                    viftopVarp = vp;
                                    ভঙ্গ;
                                }
                            }
                        }
                        UASSERT_OBJ(viftopVarp, nodep,
                                    "No __Viftop variable for sub-interface cell");
                        যদি (!viftopVarp->didWidth()) userIterate(viftopVarp, nullptr);
                        Ast_গ্ৰন্থি_DType* subDtypep = viftopVarp->dtypep();
                        যদি (adtypep->isVirtual()) {
                            // A sub-interface selected through a virtual interface
                            // handle is itself a virtual reference (a runtime
                            // instance pointer), not a static instance. Mark the
                            // dtype virtual so a method call on it dispatches per
                            // instance instead of inlining to one fixed scope.
                            যদি (AstIfaceRefDType* ধ্রুৱ subRefp
                                = VN_CAST(subDtypep->skipRefp(), IfaceRefDType)) {
                                AstIfaceRefDType* ধ্রুৱ newDtypep = নূতন AstIfaceRefDType{
                                    nodep->fileline(), subRefp->cellName(), subRefp->ifaceName()};
                                newDtypep->ifacep(subRefp->ifacep());
                                newDtypep->cellp(subRefp->cellp());
                                newDtypep->isVirtual(সঁচা);
                                v3Global.rootp()->typeTablep()->addTypesp(newDtypep);
                                subDtypep = newDtypep;
                            }
                        }
                        nodep->dtypep(subDtypep);
                        nodep->varp(viftopVarp);
                        viftopVarp->sensIfacep(VN_AS(cellp->modp(), Iface));
                        nodep->didWidth(সঁচা);
                        ঘূৰাই;
                    }
                }
                যদি (Ast_গ্ৰন্থি_FTask* ধ্রুৱ ftaskp = VN_CAST(foundp, গ্ৰন্থি_FTask)) {
                    AstMethodCall* ধ্রুৱ newp = নূতন AstMethodCall{
                        nodep->fileline(), nodep->fromp()->unlinkFrBack(), nodep->name()};
                    newp->taskp(ftaskp);
                    newp->dtypep(ftaskp->dtypep());
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
                    userIterate(newp, m_vup);
                    ঘূৰাই;
                }
                UINFO(1, "found object " << foundp);
                nodep->v3fatalSrc("MemberSel of non-variable\n"
                                  << nodep->warnContextPrimary() << '\n'
                                  << foundp->warnOther() << "... Location of found object\n"
                                  << foundp->warnContextSecondary());
            }
        } অন্যথা যদি (VN_IS(fromDtp, EnumDType)  //
                   || VN_IS(fromDtp, AssocArrayDType)  //
                   || VN_IS(fromDtp, WildcardArrayDType)  //
                   || VN_IS(fromDtp, UnpackArrayDType)  //
                   || VN_IS(fromDtp, DynArrayDType)  //
                   || VN_IS(fromDtp, QueueDType)  //
                   || VN_IS(fromDtp, ConstraintRefDType)  //
                   || VN_IS(fromDtp, BasicDType)) {
            // Method call on enum without following parenthesis, e.g. "ENUM.next"
            // Convert this into a method call, and let that visitor figure out what to do next
            Ast_গ্ৰন্থি* ধ্রুৱ newp = নূতন AstMethodCall{nodep->fileline(),
                                                    nodep->fromp()->unlinkFrBack(), nodep->name()};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
            userIterate(newp, m_vup);
            ঘূৰাই;
        } অন্যথা {
            nodep->v3error("Member selection of non-struct/union object '"
                           << nodep->fromp()->prettyTypeName() << "' which is a '"
                           << nodep->fromp()->dtypep()->prettyTypeName() << "'");
        }
        // Error handling
        nodep->replaceWith(নূতন AstConst{nodep->fileline(), AstConst::BitFalse{}});
        VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
    }
    তার্কিক memberSelClass(AstMemberSel* nodep, AstClassRefDType* adtypep) {
        যদি (nodep->name() == "rand_mode" || nodep->name() == "randomize") {
            AstMethodCall* ধ্রুৱ newp = নূতন AstMethodCall{
                nodep->fileline(), nodep->fromp()->unlinkFrBack(), nodep->name()};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
            visit(newp);
            ঘূৰাই সঁচা;
        }
        // Returns true if ok
        // No need to width-resolve the class, as it was done when we did the child
        AstClass* ধ্রুৱ first_classp = adtypep->classp();
        UASSERT_OBJ(first_classp, nodep, "Unlinked");
        বাবে (AstClass* classp = first_classp; classp;) {
            যদি (Ast_গ্ৰন্থি* ধ্রুৱ foundp = m_memberMap.findMember(classp, nodep->name())) {
                যদি (AstVar* ধ্রুৱ varp = VN_CAST(foundp, Var)) {
                    যদি (!varp->didWidth()) userIterate(varp, nullptr);
                    যদি (varp->lifetime().isStatic() || varp->isParam()) {
                        // Static members are moved outside the class, so they shouldn't be
                        // accessed by member select on a class object
                        AstVarRef* ধ্রুৱ varRefp
                            = নূতন AstVarRef{nodep->fileline(), varp, nodep->access()};
                        varRefp->classOrPackagep(classp);
                        nodep->replaceWith(varRefp);
                        VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
                        ঘূৰাই সঁচা;
                    }
                    যদি (nodep->access().isWriteOrRW() && varp->isConst()) {
                        nodep->v3warn(E_CONSTWRITTEN, "Writing to 'const' data-typed variable "
                                                          << nodep->prettyNameQ()
                                                          << " (IEEE 1800-2023 6.20.6)");
                    }
                    nodep->dtypep(foundp->dtypep());
                    nodep->varp(varp);
                    nodep->didWidth(সঁচা);
                    যদি (nodep->fromp()->same_বৃক্ষ(m_randomizeFromp) && varp->isRand())  // null-safe
                        V3LinkLValue::linkLValueSet(nodep);
                    ঘূৰাই সঁচা;
                }
                যদি (AstConstraint* constrp = VN_CAST(foundp, Constraint)) {
                    nodep->replaceWith(নূতন AstConstraintRef{
                        nodep->fileline(), nodep->fromp()->unlinkFrBack(), constrp});
                    VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
                    ঘূৰাই সঁচা;
                }
                যদি (AstEnumItemRef* ধ্রুৱ adfoundp = VN_CAST(foundp, EnumItemRef)) {
                    nodep->replaceWith(adfoundp->clone_বৃক্ষ(মিছা));
                    VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
                    ঘূৰাই সঁচা;
                }
                যদি (Ast_গ্ৰন্থি_FTask* ftaskp = VN_CAST(foundp, গ্ৰন্থি_FTask)) {
                    AstMethodCall* newp = নূতন AstMethodCall{
                        nodep->fileline(), nodep->fromp()->unlinkFrBack(), nodep->name()};
                    newp->taskp(ftaskp);
                    newp->dtypep(ftaskp->dtypep());
                    newp->classOrPackagep(classp);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
                    ঘূৰাই সঁচা;
                }
                যদি (VN_IS(foundp, Constraint)) {
                    // We don't support constraints yet, so just keep as unlinked for now
                    // Presumably we'll next see a constraint_mode AstMethodCall
                    nodep->dtypep(nodep->findConstraintRefDType());
                    UINFO(9, "Unsupported constraint select " << nodep);
                    ঘূৰাই সঁচা;
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
        বাবে (AstClass* classp = first_classp; classp;) {
            বাবে (Ast_গ্ৰন্থি* itemp = classp->membersp(); itemp; itemp = itemp->nextp()) {
                যদি (VN_IS(itemp, Constraint) || VN_IS(itemp, EnumItemRef) || VN_IS(itemp, Var)) {
                    speller.ঠেলো_Candidate(itemp->prettyName());
                }
            }
            classp = classp->extendsp() ? classp->extendsp()->classp() : nullptr;
        }
        ধ্রুৱ string suggest = speller.best_Candidate_বাৰ্তা(nodep->prettyName());
        nodep->v3error(
            "Member " << nodep->prettyNameQ() << " not found in " << first_classp->verilogKwd()
                      << " " << first_classp->prettyNameQ() << "\n"
                      << (suggest.empty() ? "" : nodep->fileline()->warnMore() + suggest));
        ঘূৰাই মিছা;  // Caller handles error
    }
    তার্কিক modportAllowsItem(Ast_গ্ৰন্থি* nodep, ধ্রুৱ AstIfaceRefDType* adtypep, ধ্রুৱ Ast_গ্ৰন্থি* itemp) {
        // Variables, tasks/functions and clocking blocks of an interface are only visible
        // through a modport when the modport lists them
        ধ্রুৱ AstModport* ধ্রুৱ modportp = adtypep->modportp();
        যদি (!modportp) ঘূৰাই সঁচা;
        যদি (ধ্রুৱ AstVar* ধ্রুৱ varp = VN_CAST(itemp, Var)) {
            যদি (varp->isParam()) ঘূৰাই সঁচা;
        } অন্যথা যদি (!VN_IS(itemp, গ্ৰন্থি_FTask) && !VN_IS(itemp, Clocking)) {
            ঘূৰাই সঁচা;
        }
        auto pair = m_modportNames.emplace(modportp, std::unordered_set<string>{});
        যদি (pair.second) {
            বাবে (ধ্রুৱ Ast_গ্ৰন্থি* mpItemp = modportp->varsp(); mpItemp; mpItemp = mpItemp->nextp()) {
                pair.first->second.ঢোকাও(mpItemp->name());
            }
        }
        যদি (pair.first->second.গুনতি(itemp->name())) ঘূৰাই সঁচা;
        nodep->v3error("Member " << itemp->prettyNameQ() << " found in interface "
                                 << adtypep->ifaceViaCellp()->prettyNameQ()
                                 << " but not in modport " << modportp->prettyNameQ()
                                 << " (IEEE 1800-2023 25.5)");
        ঘূৰাই মিছা;
    }
    Ast_গ্ৰন্থি* memberSelIface(AstMemberSel* nodep, AstIfaceRefDType* adtypep) {
        // Returns node if ok
        // No need to width-resolve the interface, as it was done when we did the child
        // ifaceViaCellp() handles dtypes with cellp-only (no ifacep), as produced
        // by sub-interface selection, enabling chained access (e.g. vif.tx.Tx).
        Ast_গ্ৰন্থি_Module* ধ্রুৱ ifacep = adtypep->ifaceViaCellp();
        UASSERT_OBJ(ifacep, nodep, "Unlinked");
        VSpellCheck speller;
        বাবে (Ast_গ্ৰন্থি* itemp = ifacep->stmtsp(); itemp; itemp = itemp->nextp()) {
            যদি (itemp->name() == nodep->name()) {
                যদি (!modportAllowsItem(nodep, adtypep, itemp)) {
                    ঘূৰাই nullptr;  // Caller handles error
                }
                ঘূৰাই itemp;
            }
            যদি (VN_IS(itemp, Var) || VN_IS(itemp, Modport) || VN_IS(itemp, Cell)) {
                speller.ঠেলো_Candidate(itemp->prettyName());
            }
        }
        ধ্রুৱ string suggest = speller.best_Candidate_বাৰ্তা(nodep->prettyName());
        nodep->v3error(
            "Member " << nodep->prettyNameQ() << " not found in interface "
                      << ifacep->prettyNameQ() << "\n"
                      << (suggest.empty() ? "" : nodep->fileline()->warnMore() + suggest));
        ঘূৰাই nullptr;  // Caller handles error
    }
    AstVar* memberSelClocking(AstMemberSel* nodep, AstClocking* clockingp) {
        // Returns node if ok
        VSpellCheck speller;

        বাবে (Ast_গ্ৰন্থি* itemp = clockingp->itemsp(); itemp; itemp = itemp->nextp()) {
            যদি (AstClockingItem* citemp = VN_CAST(itemp, ClockingItem)) {
                যদি (citemp->varp()->name() == nodep->name()) ঘূৰাই citemp->varp();
                speller.ঠেলো_Candidate(citemp->varp()->prettyName());
            }
        }
        ধ্রুৱ string suggest = speller.best_Candidate_বাৰ্তা(nodep->prettyName());
        nodep->v3error(
            "Member " << nodep->prettyNameQ() << " not found in clocking block "
                      << clockingp->prettyNameQ() << "\n"
                      << (suggest.empty() ? "" : nodep->fileline()->warnMore() + suggest));
        ঘূৰাই nullptr;  // Caller handles error
    }
    তার্কিক memberSelStruct(AstMemberSel* nodep, Ast_গ্ৰন্থি_UOr_Struct_DType* adtypep) {
        // Returns true if ok
        যদি (AstMemberDType* ধ্রুৱ memberp
            = VN_CAST(m_memberMap.findMember(adtypep, nodep->name()), MemberDType)) {
            যদি (m_attrp) {  // Looking for the base of the attribute
                nodep->dtypep(memberp);
                UINFO(9, "   MEMBERSEL(attr) -> " << nodep);
                UINFO(9, "           dt-> " << nodep->dtypep());
            } অন্যথা যদি (adtypep->packed()) {
                AstSel* ধ্রুৱ newp = নূতন AstSel{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                                memberp->lsb(), memberp->width()};
                // Must skip over the member to find the union; as the member may disappear later
                newp->dtypep(memberp->subDTypep()->skipRefToEnump());
                newp->didWidth(সঁচা);  // Don't replace dtype with basic type
                UINFO(9, "   MEMBERSEL -> " << newp);
                UINFO(9, "           dt-> " << newp->dtypep());
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
                // Should be able to treat it as a normal-ish nodesel - maybe.
                // The lhsp() will be strange until this stage; create the number here?
            } অন্যথা {
                AstStructSel* ধ্রুৱ newp = নূতন AstStructSel{
                    nodep->fileline(), nodep->fromp()->unlinkFrBack(), nodep->name()};
                // Must skip over the member to find the union; as the member may disappear later
                newp->dtypep(memberp->subDTypep()->skipRefToEnump());
                newp->didWidth(সঁচা);  // Don't replace dtype with basic type
                UINFO(9, "   MEMBERSEL -> " << newp);
                UINFO(9, "           dt-> " << newp->dtypep());
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
                // Should be able to treat it as a normal-ish nodesel - maybe.
                // The lhsp() will be strange until this stage; create the number here?
            }
            ঘূৰাই সঁচা;
        }
        nodep->v3error("Member " << nodep->prettyNameQ() << " not found in structure");
        ঘূৰাই মিছা;
    }

    শূন্য visit(AstCMethodHard* nodep) override {
        // Never created before V3Width, so no need to redo it
        UASSERT_OBJ(nodep->dtypep(), nodep, "CMETHODCALLs should have already been sized");
    }

    শূন্য visit(AstMethodCall* nodep) override {
        UINFO(5, "   METHODCALL " << nodep);
        যদি (nodep->didWidth()) ঘূৰাই;
        UINFOTREE(9, nodep, "", "mts-in");
        // Should check types the method requires, but at present we don't do much
        userIterate(nodep->fromp(), WidthVP{SELF, BOTH}.p());
        // Args are checked within each particular method's decode
        // Any AstWith is checked later when know types, in methodWithClause
        // Find the fromp dtype - should be a class
        UASSERT_OBJ(nodep->fromp() && nodep->fromp()->dtypep(), nodep, "Unsized expression");
        Ast_গ্ৰন্থি_DType* ধ্রুৱ fromDtp = nodep->fromp()->dtypep()->skipRefToEnump();
        AstBasicDType* ধ্রুৱ basicp = fromDtp ? fromDtp->basicp() : nullptr;
        UINFO(9, "     from dt " << fromDtp);
        userIterate(fromDtp, WidthVP{SELF, BOTH}.p());
        যদি (nodep->name() == "rand_mode") {
            methodCallRandMode(nodep);
        } অন্যথা যদি (nodep->name() == "constraint_mode") {
            methodCallConstraint(nodep, nullptr);
        } অন্যথা যদি (AstEnumDType* ধ্রুৱ adtypep = VN_CAST(fromDtp, EnumDType)) {
            methodCallEnum(nodep, adtypep);
        } অন্যথা যদি (AstAssocArrayDType* ধ্রুৱ adtypep = VN_CAST(fromDtp, AssocArrayDType)) {
            methodCallAssoc(nodep, adtypep);
        } অন্যথা যদি (AstWildcardArrayDType* ধ্রুৱ adtypep = VN_CAST(fromDtp, WildcardArrayDType)) {
            methodCallWildcard(nodep, adtypep);
        } অন্যথা যদি (AstDynArrayDType* ধ্রুৱ adtypep = VN_CAST(fromDtp, DynArrayDType)) {
            methodCallDyn(nodep, adtypep);
        } অন্যথা যদি (AstQueueDType* ধ্রুৱ adtypep = VN_CAST(fromDtp, QueueDType)) {
            methodCallQueue(nodep, adtypep);
        } অন্যথা যদি (AstClassRefDType* ধ্রুৱ adtypep = VN_CAST(fromDtp, ClassRefDType)) {
            methodCallClass(nodep, adtypep);
        } অন্যথা যদি (AstIfaceRefDType* ধ্রুৱ adtypep = VN_CAST(fromDtp, IfaceRefDType)) {
            methodCallIfaceRef(nodep, adtypep);
        } অন্যথা যদি (AstUnpackArrayDType* ধ্রুৱ adtypep = VN_CAST(fromDtp, UnpackArrayDType)) {
            methodCallUnpack(nodep, adtypep);
        } অন্যথা যদি (AstConstraintRefDType* ধ্রুৱ adtypep = VN_CAST(fromDtp, ConstraintRefDType)) {
            methodCallConstraint(nodep, adtypep);
        } অন্যথা যদি (basicp && basicp->isEvent()) {
            methodCallEvent(nodep, basicp);
        } অন্যথা যদি (basicp && basicp->isString()) {
            methodCallString(nodep, basicp);
        } অন্যথা {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: Member call on object '"
                                             << nodep->fromp()->prettyTypeName()
                                             << "' which is a '"
                                             << nodep->fromp()->dtypep()->prettyTypeName() << "'");
            nodep->dtypeSetVoid();
        }
    }
    AstWith* methodWithClause(Ast_গ্ৰন্থি_FTask_Ref* nodep, তার্কিক required, তার্কিক arbReturn,
                              Ast_গ্ৰন্থি_DType* returnDtp, Ast_গ্ৰন্থি_DType* indexDtp,
                              Ast_গ্ৰন্থি_DType* মান_Dtp) {
        UASSERT_OBJ(arbReturn || returnDtp, nodep, "Null return type");
        যদি (AstWith* ধ্রুৱ withp = nodep->withp()) {
            withp->indexArgRefp()->dtypep(indexDtp);
            withp->মান_Arg_Refp()->dtypep(মান_Dtp);
            userIterate(withp, WidthVP{returnDtp, BOTH}.p());
            withp->unlinkFrBack();
            ঘূৰাই withp;
        }
        যদি (required) {
            nodep->v3error("'with' statement is required for ." << nodep->prettyName()
                                                                << " method");
        }
        ঘূৰাই nullptr;
    }
    শূন্য methodOkArguments(Ast_গ্ৰন্থি_FTask_Ref* nodep, পূর্ণাঙ্ক minArg, পূর্ণাঙ্ক maxArg) {
        পূর্ণাঙ্ক narg = 0;
        যদি (AstWith* ধ্রুৱ withp = nodep->withp()) {
            withp->v3error("'with' not legal on this method");
            VL_DO_DANGLING(ঠেলো_Deletep(withp->unlinkFrBack()), withp);
        }
        বাবে (AstArg* argp = nodep->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) ++narg;
        ধ্রুৱ তার্কিক ok = (narg >= minArg) && (narg <= maxArg);
        যদি (!ok) {
            nodep->v3error("The " << narg << " arguments passed to ." << nodep->prettyName()
                                  << " method does not match its requiring " << cvtToStr(minArg)
                                  << (minArg == maxArg ? "" : " to " + cvtToStr(maxArg))
                                  << " arguments");
            // Adjust to required argument counts, very bogus, but avoids core dump
            বাবে (; narg < minArg; ++narg) {
                nodep->addArgsp(
                    নূতন AstArg{nodep->fileline(), "",
                               নূতন AstConst{nodep->fileline(), AstConst::BitFalseErroring{}}});
            }
            বাবে (; narg > maxArg; --narg) {
                AstArg* argp = nodep->argsp();
                যেতিয়ালৈ (argp->nextp()) argp = VN_AS(argp->nextp(), Arg);
                argp->unlinkFrBack();
                VL_DO_DANGLING(argp->delete_বৃক্ষ(), argp);
            }
        }
    }

    Ast_গ্ৰন্থি_ৰাশিমালা* methodArg(AstMethodCall* nodep, পূর্ণাঙ্ক arg) {
        AstArg* argp = nodep->argsp();
        বাবে (পূর্ণাঙ্ক narg = 0; narg < arg; ++narg) argp = VN_AS(argp->nextp(), Arg);
        UASSERT_OBJ(argp, nodep, "methodOkArguments() should have detected arg count error");
        ঘূৰাই argp->exprp();
    }

    শূন্য methodCallEnum(AstMethodCall* nodep, AstEnumDType* adtypep) {
        // Method call on enum without following parenthesis, e.g. "ENUM.next"
        // Convert this into a method call, and let that visitor figure out what to do next
        যদি (nodep->name() == "num"  //
            || nodep->name() == "first"  //
            || nodep->name() == "last") {
            // Constant value
            AstConst* newp = nullptr;
            methodOkArguments(nodep, 0, 0);
            যদি (nodep->name() == "num") {
                পূর্ণাঙ্ক বস্তুসমূহ = 0;
                বাবে (Ast_গ্ৰন্থি* itemp = adtypep->itemsp(); itemp; itemp = itemp->nextp()) ++বস্তুসমূহ;
                newp = নূতন AstConst(nodep->fileline(), AstConst::Signed32{}, বস্তুসমূহ);
            } অন্যথা যদি (nodep->name() == "first") {
                ধ্রুৱ AstEnumItem* itemp = adtypep->itemsp();
                যদি (!itemp) {
                    newp = নূতন AstConst(nodep->fileline(), AstConst::Signed32{},
                                        0);  // Spec doesn't say what to do
                } অন্যথা {
                    newp = VN_AS(itemp->valuep()->clone_বৃক্ষ(মিছা), Const);  // A const
                    newp->dtypeFrom(adtypep);  // To prevent a later ENUMVALUE
                }
            } অন্যথা যদি (nodep->name() == "last") {
                ধ্রুৱ AstEnumItem* itemp = adtypep->itemsp();
                যেতিয়ালৈ (itemp && itemp->nextp()) itemp = VN_AS(itemp->nextp(), EnumItem);
                যদি (!itemp) {
                    newp = নূতন AstConst(nodep->fileline(), AstConst::Signed32{},
                                        0);  // Spec doesn't say what to do
                } অন্যথা {
                    newp = VN_AS(itemp->valuep()->clone_বৃক্ষ(মিছা), Const);  // A const
                    newp->dtypeFrom(adtypep);  // To prevent a later ENUMVALUE
                }
            }
            UASSERT_OBJ(newp, nodep, "Enum method (perhaps enum item) not const");
            newp->fileline(nodep->fileline());  // Use method's filename/line number to be clearer;
                                                // may have warning disables
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
        } অন্যথা যদি (nodep->name() == "name" || nodep->name() == "next" || nodep->name() == "prev") {
            VAttrType attrType;
            যদি (nodep->name() == "name") {
                attrType = VAttrType::ENUM_NAME;
                methodOkArguments(nodep, 0, 0);
            } অন্যথা যদি (nodep->name() == "next") {
                attrType = VAttrType::ENUM_NEXT;
                methodOkArguments(nodep, 0, 1);
            } অন্যথা যদি (nodep->name() == "prev") {
                attrType = VAttrType::ENUM_PREV;
                methodOkArguments(nodep, 0, 1);
            } অন্যথা {
                nodep->v3fatalSrc("Bad case");
            }

            যদি (nodep->name() != "name" && nodep->argsp()) {
                Ast_গ্ৰন্থি_ৰাশিমালা* stepp = methodArg(nodep, 0);
                VL_DO_DANGLING(V3Const::constifyParamsNoWarnEdit(stepp), stepp);
                stepp = methodArg(nodep, 0);
                nodep->fileline()->modifyWarnOff(V3ErrorCode::WIDTHEXPAND, সঁচা);
                iterateCheckUInt32(nodep, "argument", stepp, BOTH);
                যদি (!VN_IS(stepp, Const)) {
                    stepp->v3warn(E_UNSUPPORTED,
                                  "Unsupported: enum next/prev with non-constant argument");
                    Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ newp = নূতন AstConst{stepp->fileline(), 1};
                    stepp->replaceWith(newp);
                    VL_DO_DANGLING(ঠেলো_Deletep(stepp), stepp);
                    stepp = newp;
                }
                ধ্রুৱ uint32_t stepWidth = VN_AS(stepp, Const)->toUInt();
                যদি (stepWidth == 0) {
                    // Step of 0 "legalizes" like $cast, use .next.prev
                    AstMethodCall* ধ্রুৱ newp = নূতন AstMethodCall{
                        nodep->fileline(),
                        নূতন AstMethodCall{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                          "next"},
                        "prev"};
                    // No dtype assigned, we will recurse the new method and replace
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
                    ঘূৰাই;
                } অন্যথা যদি (stepWidth != 1) {
                    // Unroll of enumVar.next(k) to enumVar.next(1).next(k - 1)
                    ঠেলো_Deletep(nodep->argsp()->unlinkFrBack());
                    AstMethodCall* ধ্রুৱ clonep = nodep->clone_বৃক্ষ(মিছা);
                    VN_AS(stepp, Const)->অঙ্ক().setLong(1);
                    AstConst* ধ্রুৱ constp = নূতন AstConst(nodep->fileline(), stepWidth - 1);
                    AstArg* ধ্রুৱ argp = নূতন AstArg{nodep->fileline(), "", constp};
                    AstMethodCall* ধ্রুৱ newp
                        = নূতন AstMethodCall{nodep->fileline(), clonep, nodep->name(), argp};
                    // No dtype assigned, we will recurse the new method and replace
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
                    ঘূৰাই;
                }
            }
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ newp
                = enumSelect(nodep->fromp()->unlinkFrBack(), adtypep, attrType);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
        } অন্যথা {
            nodep->v3error("Unknown built-in enum method " << nodep->prettyNameQ());
        }
    }
    শূন্য methodCallWildcard(AstMethodCall* nodep, AstWildcardArrayDType* adtypep) {
        AstCMethodHard* newp = nullptr;
        যদি (nodep->name() == "num"  // function int num()
            || nodep->name() == "size") {
            methodOkArguments(nodep, 0, 0);
            newp = নূতন AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::ASSOC_SIZE};  // So don't need num()
            newp->dtypeSetInt();
        } অন্যথা যদি (nodep->name() == "first"  // function int first(ref index)
                   || nodep->name() == "last"  //
                   || nodep->name() == "next"  //
                   || nodep->name() == "prev"  //
                   || nodep->name() == "unique_index"  //
                   || nodep->name() == "find_index" || nodep->name() == "find_first_index"
                   || nodep->name() == "find_last_index") {
            nodep->v3error("Array method " << nodep->prettyNameQ()
                                           << " not legal on wildcard associative arrays");
        } অন্যথা যদি (nodep->name() == "exists") {  // function int exists(input index)
            // IEEE really should have made this a "bit" return
            methodOkArguments(nodep, 1, 1);
            iterateCheckSelf(nodep, "argument", methodArg(nodep, 0), SELF, BOTH);
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ index_exprp = method_Call_Wildcard_Index_ৰাশিমালা(nodep, adtypep);
            newp = নূতন AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::ASSOC_EXISTS, index_exprp->unlinkFrBack()};
            newp->dtypeSetBitUnsized(32, 1, VSigning::SIGNED);
        } অন্যথা যদি (nodep->name() == "delete") {  // function void delete([input integer index])
            methodOkArguments(nodep, 0, 1);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::WRITE);
            যদি (!nodep->argsp()) {
                newp = নূতন AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                          VCMethod::ASSOC_CLEAR};
                newp->dtypeSetVoid();
            } অন্যথা {
                iterateCheckSelf(nodep, "argument", methodArg(nodep, 0), SELF, BOTH);
                Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ index_exprp = method_Call_Wildcard_Index_ৰাশিমালা(nodep, adtypep);
                newp = নূতন AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                          VCMethod::ASSOC_ERASE, index_exprp->unlinkFrBack()};
                newp->dtypeSetVoid();
            }
        } অন্যথা যদি (nodep->name() == "sort" || nodep->name() == "rsort"
                   || nodep->name() == "reverse" || nodep->name() == "shuffle") {
            nodep->v3error("Array method " << nodep->prettyNameQ()
                                           << " not legal on associative arrays");
        } অন্যথা যদি (nodep->name() == "and" || nodep->name() == "or" || nodep->name() == "xor"
                   || nodep->name() == "sum" || nodep->name() == "product") {
            // All value return
            AstWith* ধ্রুৱ withp
                = methodWithClause(nodep, মিছা, মিছা, adtypep->subDTypep(),
                                   adtypep->findStringDType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = নূতন AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod("r_" + nodep->name())};
            newp->withp(withp);
            newp->dtypeFrom(withp ? withp->dtypep() : adtypep->subDTypep());
            যদি (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } অন্যথা যদি (nodep->name() == "min" || nodep->name() == "max" || nodep->name() == "unique") {
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = নূতন AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->dtypep(queueDTypeIndexedBy(adtypep->subDTypep()));
            যদি (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } অন্যথা যদি (nodep->name() == "find" || nodep->name() == "find_first"
                   || nodep->name() == "find_last") {
            AstWith* ধ্রুৱ withp
                = methodWithClause(nodep, সঁচা, মিছা, nodep->findBitDType(),
                                   adtypep->findStringDType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = নূতন AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->withp(withp);
            newp->dtypep(queueDTypeIndexedBy(adtypep->subDTypep()));
            যদি (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } অন্যথা যদি (nodep->name() == "map") {
            AstWith* ধ্রুৱ withp
                = methodWithClause(nodep, সঁচা, মিছা, adtypep->subDTypep(),
                                   adtypep->findStringDType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = নূতন AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::ARRAY_MAP};
            newp->withp(withp);
            newp->dtypep(queueDTypeIndexedBy(withp ? withp->dtypep() : adtypep->subDTypep()));
            যদি (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } অন্যথা {
            nodep->v3error("Unknown wildcard associative array method " << nodep->prettyNameQ());
            nodep->dtypeFrom(adtypep->subDTypep());  // Best guess
        }
        যদি (newp) {
            newp->protect(মিছা);
            newp->didWidth(সঁচা);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
        }
    }
    শূন্য methodCallAssoc(AstMethodCall* nodep, AstAssocArrayDType* adtypep) {
        AstCMethodHard* newp = nullptr;
        যদি (nodep->withp() && adtypep->subDTypep()->isWide()) {
            nodep->v3warn(
                E_UNSUPPORTED,
                "Unsupported: `with` clause on assoc arrays with wide value types in method '"
                    << nodep->prettyName() << "'");
        }
        যদি (nodep->name() == "num"  // function int num()
            || nodep->name() == "size") {
            methodOkArguments(nodep, 0, 0);
            newp = নূতন AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::ASSOC_SIZE};  // So don't need num()
            newp->dtypeSetInt();
        } অন্যথা যদি (nodep->name() == "first"  // function int first(ref index)
                   || nodep->name() == "last"  //
                   || nodep->name() == "next"  //
                   || nodep->name() == "prev") {
            methodOkArguments(nodep, 1, 1);
            iterateCheckTyped(nodep, "argument", methodArg(nodep, 0), adtypep->চাবি_DTypep(), BOTH);
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ index_exprp = method_Call_Assoc_Index_ৰাশিমালা(nodep, adtypep);
            methodCallLValueRecurse(nodep, index_exprp, VAccess::READWRITE);
            newp
                = নূতন AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                     VCMethod::arrayMethod(nodep->name()),  // first/last/next/prev
                                     index_exprp->unlinkFrBack()};
            newp->dtypeSetInt();
            যদি (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } অন্যথা যদি (nodep->name() == "exists") {  // function int exists(input index)
            // IEEE really should have made this a "bit" return
            methodOkArguments(nodep, 1, 1);
            iterateCheckTyped(nodep, "argument", methodArg(nodep, 0), adtypep->চাবি_DTypep(), BOTH);
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ index_exprp = method_Call_Assoc_Index_ৰাশিমালা(nodep, adtypep);
            newp = নূতন AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::ASSOC_EXISTS, index_exprp->unlinkFrBack()};
            newp->dtypeSetBitUnsized(32, 1, VSigning::SIGNED);
        } অন্যথা যদি (nodep->name() == "delete") {  // function void delete([input integer index])
            methodOkArguments(nodep, 0, 1);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::WRITE);
            যদি (!nodep->argsp()) {
                newp = নূতন AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                          VCMethod::ASSOC_CLEAR};
                newp->dtypeSetVoid();
            } অন্যথা {
                iterateCheckTyped(nodep, "argument", methodArg(nodep, 0), adtypep->চাবি_DTypep(),
                                  BOTH);
                Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ index_exprp = method_Call_Assoc_Index_ৰাশিমালা(nodep, adtypep);
                newp = নূতন AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                          VCMethod::ASSOC_ERASE, index_exprp->unlinkFrBack()};
                newp->dtypeSetVoid();
            }
        } অন্যথা যদি (nodep->name() == "sort" || nodep->name() == "rsort"
                   || nodep->name() == "reverse" || nodep->name() == "shuffle") {
            nodep->v3error("Array method " << nodep->prettyNameQ()
                                           << " not legal on associative arrays");
        } অন্যথা যদি (nodep->name() == "and" || nodep->name() == "or" || nodep->name() == "xor"
                   || nodep->name() == "sum" || nodep->name() == "product") {
            // All value return
            AstWith* ধ্রুৱ withp = methodWithClause(nodep, মিছা, মিছা, adtypep->subDTypep(),
                                                    adtypep->চাবি_DTypep(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = নূতন AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod("r_" + nodep->name())};
            newp->withp(withp);
            newp->dtypeFrom(withp ? withp->dtypep() : adtypep->subDTypep());
            যদি (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } অন্যথা যদি (nodep->name() == "min" || nodep->name() == "max" || nodep->name() == "unique"
                   || nodep->name() == "unique_index") {
            AstWith* ধ্রুৱ withp = methodWithClause(
                nodep, মিছা, সঁচা, nullptr, nodep->findUInt32DType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = নূতন AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->withp(withp);
            যদি (nodep->name() == "unique_index") {
                newp->dtypep(queueDTypeIndexedBy(adtypep->চাবি_DTypep()));
            } অন্যথা {
                newp->dtypep(queueDTypeIndexedBy(adtypep->subDTypep()));
            }
            যদি (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } অন্যথা যদি (nodep->name() == "find" || nodep->name() == "find_first"
                   || nodep->name() == "find_last") {
            AstWith* ধ্রুৱ withp = methodWithClause(nodep, সঁচা, মিছা, nodep->findBitDType(),
                                                    adtypep->চাবি_DTypep(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = নূতন AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->withp(withp);
            newp->dtypep(queueDTypeIndexedBy(adtypep->subDTypep()));
            যদি (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } অন্যথা যদি (nodep->name() == "find_index" || nodep->name() == "find_first_index"
                   || nodep->name() == "find_last_index") {
            AstWith* ধ্রুৱ withp = methodWithClause(nodep, সঁচা, মিছা, nodep->findBitDType(),
                                                    adtypep->চাবি_DTypep(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = নূতন AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->withp(withp);
            newp->dtypep(queueDTypeIndexedBy(adtypep->চাবি_DTypep()));
            যদি (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } অন্যথা যদি (nodep->name() == "map") {
            AstWith* ধ্রুৱ withp = methodWithClause(nodep, সঁচা, মিছা, adtypep->subDTypep(),
                                                    adtypep->চাবি_DTypep(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = নূতন AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::ARRAY_MAP};
            newp->withp(withp);
            newp->dtypep(queueDTypeIndexedBy(withp ? withp->dtypep() : adtypep->subDTypep()));
            যদি (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } অন্যথা {
            nodep->v3error("Unknown built-in associative array method " << nodep->prettyNameQ());
            nodep->dtypeFrom(adtypep->subDTypep());  // Best guess
        }
        যদি (newp) {
            newp->protect(মিছা);
            newp->didWidth(সঁচা);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
        }
    }
    Ast_গ্ৰন্থি_ৰাশিমালা* method_Call_Assoc_Index_ৰাশিমালা(AstMethodCall* nodep, AstAssocArrayDType* adtypep) {
        Ast_গ্ৰন্থি* ধ্রুৱ index_exprp = nodep->argsp()->exprp();
        iterateCheck(nodep, "index", index_exprp, CONTEXT_DET, FINAL, adtypep->চাবি_DTypep(),
                     EXTEND_EXP);
        VL_DANGLING(index_exprp);  // May have been edited
        ঘূৰাই nodep->argsp()->exprp();
    }
    Ast_গ্ৰন্থি_ৰাশিমালা* method_Call_Wildcard_Index_ৰাশিমালা(AstMethodCall* nodep,
                                             AstWildcardArrayDType* adtypep) {
        Ast_গ্ৰন্থি* ধ্রুৱ index_exprp = nodep->argsp()->exprp();
        iterateCheck(nodep, "index", index_exprp, CONTEXT_DET, FINAL, adtypep->findStringDType(),
                     EXTEND_EXP);
        VL_DANGLING(index_exprp);  // May have been edited
        ঘূৰাই nodep->argsp()->exprp();
    }
    শূন্য methodCallLValueRecurse(AstMethodCall* nodep, Ast_গ্ৰন্থি* childp, ধ্রুৱ VAccess& access) {
        যদি (AstCMethodHard* ধ্রুৱ ichildp = VN_CAST(childp, CMethodHard)) {
            তার্কিক methAt = মিছা;
            তার্কিক methAtBack = মিছা;
            তার্কিক methAtWrite = মিছা;
            ৱিকল্প (ichildp->method()) {
            পক্ষ VCMethod::ARRAY_AT:  // FALLTHRU
                methAt = সঁচা;
                ভঙ্গ;
            পক্ষ VCMethod::ARRAY_AT_BACK:  // FALLTHRU
                methAtBack = সঁচা;
                ভঙ্গ;
            পক্ষ VCMethod::ARRAY_AT_WRITE:  // FALLTHRU
            পক্ষ VCMethod::DYN_AT_WRITE_APPEND:  // FALLTHRU
            পক্ষ VCMethod::DYN_AT_WRITE_APPEND_BACK:  //
                methAtWrite = সঁচা;
                ভঙ্গ;
            স্ৱভাৱ: ভঙ্গ;
            }
            যদি (methAt || methAtBack || methAtWrite) {
                ধ্রুৱ Ast_গ্ৰন্থি_DType* ধ্রুৱ fromDtypep = ichildp->fromp()->dtypep()->skipRefp();
                যদি (VN_IS(fromDtypep, QueueDType) || VN_IS(fromDtypep, DynArrayDType)) {
                    // Change access methods to writable ones
                    যদি (methAt) {
                        ichildp->method(VCMethod::ARRAY_AT_WRITE);
                    } অন্যথা যদি (methAtBack) {
                        ichildp->method(VCMethod::DYN_AT_WRITE_APPEND_BACK);
                    }
                }
                methodCallLValueRecurse(nodep, ichildp->fromp(), access);
                ঘূৰাই;
            }
        }
        যদি (Ast_গ্ৰন্থি_Var_Ref* ধ্রুৱ varrefp = VN_CAST(childp, গ্ৰন্থি_Var_Ref)) {
            varrefp->access(access);
        } অন্যথা যদি (ধ্রুৱ AstMemberSel* ধ্রুৱ ichildp = VN_CAST(childp, MemberSel)) {
            methodCallLValueRecurse(nodep, ichildp->fromp(), access);
        } অন্যথা যদি (ধ্রুৱ AstStructSel* ধ্রুৱ ichildp = VN_CAST(childp, StructSel)) {
            methodCallLValueRecurse(nodep, ichildp->fromp(), access);
        } অন্যথা যদি (ধ্রুৱ Ast_গ্ৰন্থি_Sel* ধ্রুৱ ichildp = VN_CAST(childp, গ্ৰন্থি_Sel)) {
            methodCallLValueRecurse(nodep, ichildp->fromp(), access);
        } অন্যথা যদি (VN_IS(childp, LambdaArgRef) || VN_IS(childp, FuncRef)) {
            // NOP
        } অন্যথা {
            UINFO(1, "    Related node: " << childp);
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: Non-variable on LHS of built-in method '"
                                             << nodep->prettyName() << "'");
        }
    }
    AstCMethodHard* methodCallArray(AstMethodCall* nodep, Ast_গ্ৰন্থি_DType* adtypep) {
        AstCMethodHard* newp = nullptr;

        যদি (nodep->name() == "reverse" || nodep->name() == "shuffle" || nodep->name() == "sort"
            || nodep->name() == "rsort") {
            AstWith* withp = nullptr;
            যদি (nodep->name() == "sort" || nodep->name() == "rsort") {
                withp = methodWithClause(nodep, মিছা, সঁচা, nullptr, nodep->findUInt32DType(),
                                         adtypep->subDTypep());
            }
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::WRITE);
            newp = নূতন AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->withp(withp);
            newp->dtypeSetVoid();
        } অন্যথা যদি (nodep->name() == "min" || nodep->name() == "max" || nodep->name() == "unique"
                   || nodep->name() == "unique_index") {
            AstWith* ধ্রুৱ withp = methodWithClause(
                nodep, মিছা, সঁচা, nullptr, nodep->findUInt32DType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = নূতন AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->withp(withp);
            যদি (nodep->name() == "unique_index") {
                newp->dtypep(newp->findQueueIndexDType());
            } অন্যথা {
                newp->dtypep(queueDTypeIndexedBy(adtypep->subDTypep()));
            }
            যদি (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } অন্যথা যদি (nodep->name() == "find" || nodep->name() == "find_first"
                   || nodep->name() == "find_last" || nodep->name() == "find_index") {
            AstWith* ধ্রুৱ withp
                = methodWithClause(nodep, সঁচা, মিছা, nodep->findBitDType(),
                                   nodep->findUInt32DType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = নূতন AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->withp(withp);
            newp->dtypep(queueDTypeIndexedBy(adtypep->subDTypep()));
            যদি (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } অন্যথা যদি (nodep->name() == "find_index" || nodep->name() == "find_first_index"
                   || nodep->name() == "find_last_index") {
            AstWith* ধ্রুৱ withp
                = methodWithClause(nodep, সঁচা, মিছা, nodep->findBitDType(),
                                   nodep->findUInt32DType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = নূতন AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->withp(withp);
            newp->dtypep(newp->findQueueIndexDType());
            যদি (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } অন্যথা যদি (nodep->name() == "map") {
            // map() - IEEE 1800-2023 7.12.5
            // Returns a queue with same element count, each element is the with expression result
            AstWith* ধ্রুৱ withp
                = methodWithClause(nodep, সঁচা, মিছা, adtypep->subDTypep(),
                                   nodep->findUInt32DType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = নূতন AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::ARRAY_MAP};
            newp->withp(withp);
            newp->dtypep(queueDTypeIndexedBy(withp ? withp->dtypep() : adtypep->subDTypep()));
            যদি (!nodep->firstAbovep()) newp->dtypeSetVoid();
        }
        ঘূৰাই newp;
    }
    শূন্য methodCallDyn(AstMethodCall* nodep, AstDynArrayDType* adtypep) {
        AstCMethodHard* newp = nullptr;
        যদি (nodep->name() == "at") {  // Created internally for []
            methodOkArguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
            newp = নূতন AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::ARRAY_AT};
            newp->dtypeFrom(adtypep->subDTypep());
        } অন্যথা যদি (nodep->name() == "atWrite") {  // Created internally for []
            methodOkArguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::WRITE);
            newp = নূতন AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::ARRAY_AT_WRITE};
            newp->dtypeFrom(adtypep->subDTypep());
        } অন্যথা যদি (nodep->name() == "size") {
            methodOkArguments(nodep, 0, 0);
            newp = নূতন AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::DYN_SIZE};
            newp->dtypeSetInt();
        } অন্যথা যদি (nodep->name() == "delete") {  // function void delete()
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::WRITE);
            newp = নূতন AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::DYN_CLEAR};
            newp->dtypeSetVoid();
        } অন্যথা যদি (nodep->name() == "and" || nodep->name() == "or" || nodep->name() == "xor"
                   || nodep->name() == "sum" || nodep->name() == "product") {
            // All value return
            AstWith* ধ্রুৱ withp
                = methodWithClause(nodep, মিছা, মিছা, adtypep->subDTypep(),
                                   nodep->findUInt32DType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = নূতন AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod("r_" + nodep->name())};
            newp->withp(withp);
            newp->dtypeFrom(withp ? withp->dtypep() : adtypep->subDTypep());
            যদি (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } অন্যথা যদি ((newp = methodCallArray(nodep, adtypep))) {
        } অন্যথা {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported/unknown built-in dynamic array method "
                                             << nodep->prettyNameQ());
            nodep->dtypeFrom(adtypep->subDTypep());  // Best guess
        }
        যদি (newp) {
            newp->protect(মিছা);
            newp->didWidth(সঁচা);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
        }
    }
    শূন্য methodCallQueue(AstMethodCall* nodep, AstQueueDType* adtypep) {
        AstCMethodHard* newp = nullptr;
        যদি (nodep->name() == "at" || nodep->name() == "atBack") {  // Created internally for []
            methodOkArguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
            newp = নূতন AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->dtypeFrom(adtypep->subDTypep());
        } অন্যথা যদি (nodep->name() == "atWriteAppend"
                   || nodep->name() == "atWriteAppendBack") {  // Created internally for []
            methodOkArguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::WRITE);
            newp = নূতন AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->dtypeFrom(adtypep->subDTypep());
        } অন্যথা যদি (nodep->name() == "num"  // function int num()
                   || nodep->name() == "size") {
            methodOkArguments(nodep, 0, 0);
            newp = নূতন AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::DYN_SIZE};
            newp->dtypeSetInt();
        } অন্যথা যদি (nodep->name() == "delete") {  // function void delete([input integer index])
            methodOkArguments(nodep, 0, 1);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::WRITE);
            যদি (!nodep->argsp()) {
                newp = নূতন AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                          VCMethod::DYN_CLEAR};
                newp->dtypeSetVoid();
            } অন্যথা {
                iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
                Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ index_exprp = method_Call_Queue_Index_ৰাশিমালা(nodep);
                যদি (index_exprp->isZero()) {  // delete(0) is a pop_front
                    newp = নূতন AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                              VCMethod::DYN_বেৰ_কৰো_FRONT};
                    newp->dtypeFrom(adtypep->subDTypep());
                    newp->dtypeSetVoid();
                } অন্যথা {
                    newp = নূতন AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                              VCMethod::DYN_ERASE, index_exprp->unlinkFrBack()};
                    newp->dtypeSetVoid();
                }
            }
        } অন্যথা যদি (nodep->name() == "insert") {
            methodOkArguments(nodep, 2, 2);
            iterateCheckSigned32(nodep, "index", methodArg(nodep, 0), BOTH);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::WRITE);
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ index_exprp = method_Call_Queue_Index_ৰাশিমালা(nodep);
            AstArg* ধ্রুৱ argp = VN_AS(nodep->argsp()->nextp(), Arg);
            iterateCheckTyped(nodep, "insert value", argp->exprp(), adtypep->subDTypep(), BOTH);
            যদি (index_exprp->isZero()) {  // insert(0, ...) is a push_front
                newp = নূতন AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                          VCMethod::DYN_ঠেলো_FRONT, argp->exprp()->unlinkFrBack()};
                newp->dtypeSetVoid();
            } অন্যথা {
                newp = নূতন AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                          VCMethod::DYN_ঢোকাও, index_exprp->unlinkFrBack()};
                newp->addPinsp(argp->exprp()->unlinkFrBack());
                newp->dtypeSetVoid();
            }
        } অন্যথা যদি (nodep->name() == "pop_front" || nodep->name() == "pop_back") {
            methodOkArguments(nodep, 0, 0);
            // Returns element, so method both consumes (reads) and modifies the queue
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READWRITE);
            newp = নূতন AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->dtypeFrom(adtypep->subDTypep());
        } অন্যথা যদি (nodep->name() == "push_back" || nodep->name() == "push_front") {
            methodOkArguments(nodep, 1, 1);
            iterateCheckTyped(nodep, "argument", methodArg(nodep, 0), adtypep->subDTypep(), BOTH);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::WRITE);
            AstArg* ধ্রুৱ argp = nodep->argsp();
            iterateCheckTyped(nodep, "push value", argp->exprp(), adtypep->subDTypep(), BOTH);
            newp = নূতন AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name()),
                                      argp->exprp()->unlinkFrBack()};
            newp->dtypeSetVoid();
        } অন্যথা যদি (nodep->name() == "and" || nodep->name() == "or" || nodep->name() == "xor"
                   || nodep->name() == "sum" || nodep->name() == "product") {
            AstWith* ধ্রুৱ withp
                = methodWithClause(nodep, মিছা, মিছা, adtypep->subDTypep(),
                                   nodep->findUInt32DType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = নূতন AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod("r_" + nodep->name())};
            newp->withp(withp);
            newp->dtypeFrom(withp ? withp->dtypep() : adtypep->subDTypep());
            যদি (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } অন্যথা যদি ((newp = methodCallArray(nodep, adtypep))) {
        } অন্যথা {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported/unknown built-in queue method " << nodep->prettyNameQ());
            nodep->dtypeFrom(adtypep->subDTypep());  // Best guess
        }
        যদি (newp) {
            newp->protect(মিছা);
            newp->didWidth(সঁচা);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
        }
    }
    Ast_গ্ৰন্থি_ৰাশিমালা* method_Call_Queue_Index_ৰাশিমালা(AstMethodCall* nodep) {
        Ast_গ্ৰন্থি* ধ্রুৱ index_exprp = nodep->argsp()->exprp();
        iterateCheckSigned32(nodep, "index", index_exprp, BOTH);
        VL_DANGLING(index_exprp);  // May have been edited
        ঘূৰাই nodep->argsp()->exprp();
    }
    শূন্য methodCallWarnTiming(Ast_গ্ৰন্থি_FTask_Ref* ধ্রুৱ nodep, ধ্রুৱ std::string& className) {
        যদি (v3Global.opt.timing().isSetFalse()) {
            nodep->v3warn(E_NOTIMING,
                          className << "::" << nodep->prettyName() << "() requires --timing");
        } অন্যথা যদি (!v3Global.opt.timing().isSetTrue()) {
            nodep->v3warn(E_NEEDTIMINGOPT, "Use --timing or --no-timing to specify how "
                                               << className << "::" << nodep->name()
                                               << "() should be handled");
        }
    }
    শূন্য methodCallIfaceRef(AstMethodCall* nodep, AstIfaceRefDType* adtypep) {
        // ifaceViaCellp() resolves the interface via cellp when ifacep is null,
        // as for a sub-interface selected through a virtual interface handle.
        AstIface* ধ্রুৱ ifacep = adtypep->ifaceViaCellp();
        UINFO(5, __FUNCTION__ << ":" << nodep);
        যদি (Ast_গ্ৰন্থি_FTask* ধ্রুৱ ftaskp
            = VN_CAST(m_memberMap.findMember(ifacep, nodep->name()), গ্ৰন্থি_FTask)) {
            UINFO(5, __FUNCTION__ << "AstNodeFTask" << nodep);
            যদি (!modportAllowsItem(nodep, adtypep, ftaskp)) {
                nodep->dtypeSetVoid();
                ঘূৰাই;
            }
            যদি (adtypep->isVirtual()) {
                বাবে (Ast_গ্ৰন্থি* itemp = ifacep->stmtsp(); itemp; itemp = itemp->nextp()) {
                    যদি (AstVar* ধ্রুৱ mvarp = VN_CAST(itemp, Var)) { mvarp->sensIfacep(ifacep); }
                }
            }
            userIterate(ftaskp, nullptr);
            যদি (ftaskp->isStatic()) {
                AstArg* ধ্রুৱ argsp = nodep->argsp();
                যদি (argsp) argsp->unlinkFrBackWithNext();
                Ast_গ্ৰন্থি_FTask_Ref* newp = nullptr;
                যদি (VN_IS(ftaskp, Task)) {
                    newp = নূতন AstTaskRef{nodep->fileline(), VN_AS(ftaskp, Task), argsp};
                } অন্যথা {
                    newp = নূতন AstFuncRef{nodep->fileline(), VN_AS(ftaskp, Func), argsp};
                }
                newp->classOrPackagep(ifacep);
                nodep->replaceWith(newp);
                VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
            } অন্যথা {
                nodep->taskp(ftaskp);
                nodep->dtypeFrom(ftaskp);
                nodep->classOrPackagep(ifacep);
                যদি (VN_IS(ftaskp, Task)) nodep->dtypeSetVoid();
                processFTaskRefArgs(nodep);
            }
            ঘূৰাই;
        }
        nodep->v3error("Member reference from interface to "
                       << nodep->prettyNameQ() << " is not referencing a valid task or function ");
    }
    শূন্য handleRandomizeArgs(Ast_গ্ৰন্থি_FTask_Ref* ধ্রুৱ nodep, AstClass* ধ্রুৱ classp) {
        তার্কিক hasNonNullArgs = মিছা;
        AstConst* nullp = nullptr;
        বাবে (AstArg *argp = nodep->argsp(), *nextp; argp; argp = nextp) {
            nextp = VN_AS(argp->nextp(), Arg);
            AstVar* randVarp = nullptr;
            Ast_গ্ৰন্থি_ৰাশিমালা* exprp = argp->exprp();
            যদি (AstConst* ধ্রুৱ constp = VN_CAST(exprp, Const)) {
                যদি (constp->অঙ্ক().isNull()) {
                    nullp = constp;
                    অনুৱর্ত;
                }
            }
            hasNonNullArgs = সঁচা;
            AstVar* fromVarp = nullptr;  // If it's a method call, the leftmost element
                                         // of the dot hierarchy
            যদি (AstMethodCall* methodCallp = VN_CAST(nodep, MethodCall)) {
                Ast_গ্ৰন্থি_ৰাশিমালা* fromp = methodCallp->fromp();
                যেতিয়ালৈ (AstMemberSel* ধ্রুৱ memberSelp = VN_CAST(fromp, MemberSel)) {
                    fromp = memberSelp->fromp();
                }
                AstVarRef* ধ্রুৱ varrefp = VN_AS(fromp, VarRef);
                fromVarp = varrefp->varp();
            }
            যদি (!VN_IS(exprp, VarRef) && !VN_IS(exprp, MemberSel)) {
                argp->v3error("'randomize()' argument must be a variable contained in "
                              << (fromVarp ? fromVarp->prettyNameQ() : classp->prettyNameQ()));
                VL_DO_DANGLING(argp->unlinkFrBack()->delete_বৃক্ষ(), argp);
                অনুৱর্ত;
            }
            যেতিয়ালৈ (exprp) {
                যদি (AstMemberSel* ধ্রুৱ memberSelp = VN_CAST(exprp, MemberSel)) {
                    randVarp = memberSelp->varp();
                    exprp = memberSelp->fromp();
                } অন্যথা {
                    যদি (AstVarRef* ধ্রুৱ varrefp = VN_CAST(exprp, VarRef)) {
                        randVarp = varrefp->varp();
                    } অন্যথা {
                        argp->v3warn(
                            E_UNSUPPORTED,
                            "Unsupported: Non-variable expression as 'randomize()' argument");
                        VL_DO_DANGLING(argp->unlinkFrBack()->delete_বৃক্ষ(), argp);
                    }
                    exprp = nullptr;
                }
                // All variables in the dot hierarchy must be randomizable
                যদি (randVarp && !randVarp->isRand()) randVarp->rand(VRandAttr::RAND_INLINE);
            }
            // randVarp is now the leftmost element from the dot hierarchy in argp->exprp()
            যদি (randVarp == fromVarp) {
                // The passed in variable is MemberSel'ected from the MethodCall target
            } অন্যথা যদি (classp->existsMember([&](ধ্রুৱ AstClass*, ধ্রুৱ AstVar* memberVarp) {
                           ঘূৰাই memberVarp == randVarp;
                       })) {
                // The passed in variable is contained in the method call target
            } অন্যথা {
                // Passed in a constant or complex expression, or the above conditions are not
                // met
                argp->v3error("'randomize()' argument must be a variable contained in "
                              << (fromVarp ? fromVarp->prettyNameQ() : classp->prettyNameQ()));
                VL_DO_DANGLING(argp->unlinkFrBack()->delete_বৃক্ষ(), argp);
            }
        }
        যদি (nullp && hasNonNullArgs) {
            nullp->v3error("Cannot pass more arguments to 'randomize(null)'");
        }
        // A solo 'null' arg is left in place; V3Randomize lowers it into a
        // check-only solve so constraints are validated against the current
        // values of every rand member (IEEE 1800-2023 18.11).
    }
    শূন্য methodCallClass(AstMethodCall* nodep, AstClassRefDType* adtypep) {
        // No need to width-resolve the class, as it was done when we did the child
        AstClass* ধ্রুৱ first_classp = adtypep->classp();
        AstWith* withp = nullptr;
        যদি (nodep->name() == "randomize") {
            VL_RESTORER(m_randomizeFromp);
            m_randomizeFromp = nodep->fromp();
            withp = methodWithClause(nodep, মিছা, মিছা, adtypep->findVoidDType(),
                                     adtypep->findBitDType(), adtypep);
            বাবে (AstArg* argp = nodep->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) {
                যদি (argp->exprp()) userIterate(argp->exprp(), WidthVP{SELF, BOTH}.p());
            }
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            V3Randomize::newRandomizeFunc(m_memberMap, first_classp);
            handleRandomizeArgs(nodep, first_classp);
        } অন্যথা যদি (nodep->name() == "srandom") {
            methodOkArguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            V3Randomize::newSRandomFunc(m_memberMap, first_classp);
        }
        UASSERT_OBJ(first_classp, nodep, "Unlinked");
        বাবে (AstClass* classp = first_classp; classp;) {
            যদি (nodep->fileline()->timingOn()) {
                যদি (classp->name() == "semaphore" || classp->name() == "process"
                    || VString::startsWith(classp->name(), "mailbox")) {
                    // Find the package the class is in
                    AstPackage* ধ্রুৱ packagep = getItemPackage(classp);
                    // Check if it's std
                    যদি (packagep && packagep->name() == "std") {
                        যদি (classp->name() == "process") {
                            methodCallWarnTiming(nodep, "process");
                        } অন্যথা যদি (classp->name() == "semaphore" && nodep->name() == "get") {
                            methodCallWarnTiming(nodep, "semaphore");
                        } অন্যথা যদি (nodep->name() == "put" || nodep->name() == "get"
                                   || nodep->name() == "peek") {
                            methodCallWarnTiming(nodep, "mailbox");
                        }
                    }
                }
            }
            যদি (Ast_গ্ৰন্থি_FTask* ধ্রুৱ ftaskp
                = VN_CAST(m_memberMap.findMember(classp, nodep->name()), গ্ৰন্থি_FTask)) {
                userIterate(ftaskp, nullptr);
                যদি (ftaskp->isStatic()) {
                    AstArg* ধ্রুৱ argsp = nodep->argsp();
                    যদি (argsp) argsp->unlinkFrBackWithNext();
                    Ast_গ্ৰন্থি_FTask_Ref* newp = nullptr;
                    // We use m_vup to determine task or function, so that later error checks
                    // for funcref->task and taskref->func will pick up properly
                    যদি (!m_vup) {  // Called as task
                        newp = নূতন AstTaskRef{nodep->fileline(), nodep->name(), argsp};
                    } অন্যথা {
                        newp = নূতন AstFuncRef{nodep->fileline(), nodep->name(), argsp};
                    }
                    newp->taskp(ftaskp);  // Not passed above as might be func vs task mismatch
                    newp->classOrPackagep(classp);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
                } অন্যথা {
                    nodep->taskp(ftaskp);
                    nodep->dtypeFrom(ftaskp);
                    nodep->classOrPackagep(classp);
                    যদি (VN_IS(ftaskp, Task)) {
                        যদি (m_vup && m_vup->prelim() && !VN_IS(nodep->backp(), Stmt_ৰাশিমালা)) {
                            nodep->v3error(
                                "Cannot call a task/void-function as a member function: "
                                << nodep->prettyNameQ());
                        }
                        nodep->dtypeSetVoid();
                    }
                    যদি (withp) nodep->withp(withp);
                    processFTaskRefArgs(nodep);
                }
                ঘূৰাই;
            } অন্যথা যদি (nodep->name() == "get_randstate") {
                methodOkArguments(nodep, 0, 0);
                first_classp->baseMostClassp()->needRNG(সঁচা);
                v3Global.useRandomizeMethods(সঁচা);
                AstCMethodHard* ধ্রুৱ newp
                    = নূতন AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                         VCMethod::RNG_GET_RANDSTATE, nullptr};
                newp->usePtr(সঁচা);
                newp->dtypeSetString();
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
                ঘূৰাই;
            } অন্যথা যদি (nodep->name() == "set_randstate") {
                methodOkArguments(nodep, 1, 1);
                Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ expr1p = nodep->argsp()->exprp();  // May edit
                iterateCheckString(nodep, "LHS", expr1p, BOTH);
                Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ exprp = nodep->argsp()->exprp();
                first_classp->baseMostClassp()->needRNG(সঁচা);
                v3Global.useRandomizeMethods(সঁচা);
                AstCMethodHard* ধ্রুৱ newp
                    = নূতন AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                         VCMethod::RNG_SET_RANDSTATE, exprp->unlinkFrBack()};
                newp->usePtr(সঁচা);
                newp->dtypeSetVoid();
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
                ঘূৰাই;
            } অন্যথা যদি (nodep->name() == "constraint_mode") {
                v3Global.useRandomizeMethods(সঁচা);
                nodep->dtypep(nodep->findBasicDType(VBasicDTypeKwd::INT));
            }
            classp = classp->extendsp() ? classp->extendsp()->classp() : nullptr;
        }
        {
            VSpellCheck speller;
            বাবে (AstClass* classp = first_classp; classp;) {
                বাবে (Ast_গ্ৰন্থি* itemp = classp->membersp(); itemp; itemp = itemp->nextp()) {
                    যদি (VN_IS(itemp, গ্ৰন্থি_FTask)) speller.ঠেলো_Candidate(itemp->prettyName());
                }
                classp = classp->extendsp() ? classp->extendsp()->classp() : nullptr;
            }
            ধ্রুৱ string suggest = speller.best_Candidate_বাৰ্তা(nodep->prettyName());
            nodep->v3error("Class method "
                           << nodep->prettyNameQ() << " not found in "
                           << first_classp->verilogKwd() << " " << first_classp->prettyNameQ()
                           << "\n"
                           << (suggest.empty() ? "" : nodep->fileline()->warnMore() + suggest));
        }
        nodep->dtypeSetInt();  // Guess on error
    }
    শূন্য methodCallConstraint(AstMethodCall* nodep, AstConstraintRefDType*) {
        যদি (nodep->name() == "constraint_mode") {
            // IEEE 1800-2023 18.9
            methodOkArguments(nodep, 0, 1);
            যদি (nodep->argsp()) {
                iterateCheckBool(nodep, "argument", methodArg(nodep, 0), BOTH);
                nodep->dtypep(nodep->findBasicDType(VBasicDTypeKwd::INT));
            } অন্যথা {
                nodep->dtypeSetVoid();
            }
            v3Global.useRandomizeMethods(সঁচা);
        } অন্যথা {
            nodep->v3error("No such constraint method " << nodep->prettyNameQ());
            nodep->replaceWith(নূতন AstConst{nodep->fileline(), AstConst::BitFalseErroring{}});
            VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
        }
    }
    শূন্য methodCallRandMode(AstMethodCall* nodep) {
        methodOkArguments(nodep, 0, 1);
        // IEEE 1800-2023 18.8
        যদি (nodep->argsp()) {
            iterateCheckBool(nodep, "argument", methodArg(nodep, 0), BOTH);
            nodep->dtypeSetVoid();
        } অন্যথা {
            nodep->dtypep(nodep->findBasicDType(VBasicDTypeKwd::INT));
        }
        v3Global.useRandomizeMethods(সঁচা);
    }
    শূন্য methodCallUnpack(AstMethodCall* nodep, AstUnpackArrayDType* adtypep) {
        গণনা : uint8_t {
            UNKNOWN = 0,
            ARRAY_OR,
            ARRAY_AND,
            ARRAY_XOR,
            ARRAY_SUM,
            ARRAY_PRODUCT
        } methodId;

        methodId = UNKNOWN;
        যদি (nodep->name() == "or") {
            methodId = ARRAY_OR;
        } অন্যথা যদি (nodep->name() == "and") {
            methodId = ARRAY_AND;
        } অন্যথা যদি (nodep->name() == "xor") {
            methodId = ARRAY_XOR;
        } অন্যথা যদি (nodep->name() == "sum") {
            methodId = ARRAY_SUM;
        } অন্যথা যদি (nodep->name() == "product") {
            methodId = ARRAY_PRODUCT;
        }

        যদি (methodId) {
            AstWith* ধ্রুৱ withp
                = methodWithClause(nodep, মিছা, মিছা, adtypep->subDTypep(),
                                   nodep->findUInt32DType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            যদি (withp) {
                methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
                AstCMethodHard* ধ্রুৱ newp
                    = নূতন AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                         VCMethod::arrayMethod("r_" + nodep->name())};
                newp->withp(withp);
                newp->dtypeFrom(withp ? withp->dtypep() : adtypep->subDTypep());
                যদি (!nodep->firstAbovep()) newp->dtypeSetVoid();
                newp->protect(মিছা);
                newp->didWidth(সঁচা);
                nodep->replaceWith(newp);
                VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
                ঘূৰাই;
            } অন্যথা {
                // TODO use this code for small vectors, otherwise use runtime reduction.
                // Historically we expand all vectors.
                File_সাৰি* ধ্রুৱ fl = nodep->fileline();
                Ast_গ্ৰন্থি_ৰাশিমালা* newp = nullptr;
                বাবে (পূর্ণাঙ্ক i = 0; i < adtypep->elementsConst(); ++i) {
                    Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ arrayRef = nodep->fromp()->clone_বৃক্ষ_Pure(মিছা);
                    Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ selector = নূতন AstArraySel{fl, arrayRef, i};
                    যদি (!newp) {
                        newp = selector;
                    } অন্যথা {
                        ৱিকল্প (methodId) {
                        পক্ষ ARRAY_OR: newp = নূতন AstOr{fl, newp, selector}; ভঙ্গ;
                        পক্ষ ARRAY_AND: newp = নূতন AstAnd{fl, newp, selector}; ভঙ্গ;
                        পক্ষ ARRAY_XOR: newp = নূতন AstXor{fl, newp, selector}; ভঙ্গ;
                        পক্ষ ARRAY_SUM: newp = নূতন AstAdd{fl, newp, selector}; ভঙ্গ;
                        পক্ষ ARRAY_PRODUCT: newp = নূতন AstMul{fl, newp, selector}; ভঙ্গ;
                        স্ৱভাৱ: nodep->v3fatalSrc("bad case");
                        }
                    }
                }
                nodep->replaceWith(newp);
                VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
            }
        } অন্যথা যদি (AstCMethodHard* newp = methodCallArray(nodep, adtypep)) {
            newp->protect(মিছা);
            newp->didWidth(সঁচা);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
        } অন্যথা {
            nodep->v3error("Unknown built-in array method " << nodep->prettyNameQ());
            nodep->dtypeFrom(adtypep->subDTypep());  // Best guess
        }
    }
    শূন্য methodCallEvent(AstMethodCall* nodep, AstBasicDType*) {
        // Method call on event
        যদি (nodep->name() == "triggered") {
            methodOkArguments(nodep, 0, 0);
            AstCMethodHard* ধ্রুৱ callp = নূতন AstCMethodHard{
                nodep->fileline(), nodep->fromp()->unlinkFrBack(), VCMethod::EVENT_IS_TRIGGERED};
            callp->dtypeSetBit();
            nodep->replaceWith(callp);
            VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
        } অন্যথা {
            nodep->v3error("Unknown built-in event method " << nodep->prettyNameQ());
        }
    }
    শূন্য methodCallString(AstMethodCall* nodep, AstBasicDType*) {
        // Method call on string
        যদি (nodep->name() == "len") {
            // Constant value
            methodOkArguments(nodep, 0, 0);
            Ast_গ্ৰন্থি* ধ্রুৱ newp = নূতন AstLenN{nodep->fileline(), nodep->fromp()->unlinkFrBack()};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
        } অন্যথা যদি (nodep->name() == "itoa") {
            methodOkArguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
            VL_DO_DANGLING(replaceWithSFormat(nodep, "%0d"), nodep);
        } অন্যথা যদি (nodep->name() == "hextoa") {
            methodOkArguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
            VL_DO_DANGLING(replaceWithSFormat(nodep, "%0x"), nodep);
        } অন্যথা যদি (nodep->name() == "octtoa") {
            methodOkArguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
            VL_DO_DANGLING(replaceWithSFormat(nodep, "%0o"), nodep);
        } অন্যথা যদি (nodep->name() == "bintoa") {
            methodOkArguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
            VL_DO_DANGLING(replaceWithSFormat(nodep, "%0b"), nodep);
        } অন্যথা যদি (nodep->name() == "realtoa") {
            methodOkArguments(nodep, 1, 1);
            iterateCheckReal(nodep, "argument", methodArg(nodep, 0), BOTH);
            VL_DO_DANGLING(replaceWithSFormat(nodep, "%g"), nodep);
        } অন্যথা যদি (nodep->name() == "tolower") {
            methodOkArguments(nodep, 0, 0);
            Ast_গ্ৰন্থি* ধ্রুৱ newp
                = নূতন AstToLowerN{nodep->fileline(), nodep->fromp()->unlinkFrBack()};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
        } অন্যথা যদি (nodep->name() == "toupper") {
            methodOkArguments(nodep, 0, 0);
            Ast_গ্ৰন্থি* ধ্রুৱ newp
                = নূতন AstToUpperN{nodep->fileline(), nodep->fromp()->unlinkFrBack()};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
        } অন্যথা যদি (nodep->name() == "compare" || nodep->name() == "icompare") {
            ধ্রুৱ তার্কিক ignoreCase = nodep->name()[0] == 'i';
            methodOkArguments(nodep, 1, 1);
            iterateCheckString(nodep, "argument", methodArg(nodep, 0), BOTH);
            AstArg* ধ্রুৱ argp = nodep->argsp();
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ lhs = nodep->fromp()->unlinkFrBack();
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ rhs = argp->exprp()->unlinkFrBack();
            Ast_গ্ৰন্থি* ধ্রুৱ newp = নূতন AstCompareNN{nodep->fileline(), lhs, rhs, ignoreCase};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
        } অন্যথা যদি (nodep->name() == "putc") {
            methodOkArguments(nodep, 2, 2);
            iterateCheckSigned32(nodep, "argument 0", methodArg(nodep, 0), BOTH);
            iterateCheckSigned8(nodep, "argument 1", methodArg(nodep, 1), BOTH);
            AstArg* ধ্রুৱ arg0p = nodep->argsp();
            AstArg* ধ্রুৱ arg1p = VN_AS(arg0p->nextp(), Arg);
            Ast_গ্ৰন্থি_Var_Ref* ধ্রুৱ fromp = VN_AS(nodep->fromp()->unlinkFrBack(), VarRef);
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ rhsp = arg0p->exprp()->unlinkFrBack();
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ thsp = arg1p->exprp()->unlinkFrBack();
            AstVarRef* ধ্রুৱ varrefp
                = নূতন AstVarRef{nodep->fileline(), fromp->varp(), VAccess::READ};
            Ast_গ্ৰন্থি* ধ্রুৱ newp = নূতন AstAssign{
                nodep->fileline(), fromp, নূতন AstPutcN{nodep->fileline(), varrefp, rhsp, thsp}};
            fromp->access(VAccess::WRITE);
            ঠেলো_Deletep(nodep->backp());
            VL_DO_DANGLING(nodep->backp()->replaceWith(newp), nodep);
        } অন্যথা যদি (nodep->name() == "getc") {
            methodOkArguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
            AstArg* ধ্রুৱ arg0p = nodep->argsp();
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ lhsp = nodep->fromp()->unlinkFrBack();
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ rhsp = arg0p->exprp()->unlinkFrBack();
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ newp = নূতন AstGetcN{nodep->fileline(), lhsp, rhsp};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
        } অন্যথা যদি (nodep->name() == "substr") {
            methodOkArguments(nodep, 2, 2);
            iterateCheckSigned32(nodep, "argument 0", methodArg(nodep, 0), BOTH);
            iterateCheckSigned32(nodep, "argument 1", methodArg(nodep, 1), BOTH);
            AstArg* ধ্রুৱ arg0p = nodep->argsp();
            AstArg* ধ্রুৱ arg1p = VN_AS(arg0p->nextp(), Arg);
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ lhsp = nodep->fromp()->unlinkFrBack();
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ rhsp = arg0p->exprp()->unlinkFrBack();
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ thsp = arg1p->exprp()->unlinkFrBack();
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ newp = নূতন AstSubstrN{nodep->fileline(), lhsp, rhsp, thsp};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
        } অন্যথা যদি (nodep->name() == "atobin" || nodep->name() == "atohex"
                   || nodep->name() == "atoi" || nodep->name() == "atooct"
                   || nodep->name() == "atoreal") {
            AstAtoN::FmtType fmt;
            যদি (nodep->name() == "atobin") {
                fmt = AstAtoN::ATOBIN;
            } অন্যথা যদি (nodep->name() == "atohex") {
                fmt = AstAtoN::ATOHEX;
            } অন্যথা যদি (nodep->name() == "atoi") {
                fmt = AstAtoN::ATOI;
            } অন্যথা যদি (nodep->name() == "atooct") {
                fmt = AstAtoN::ATOOCT;
            } অন্যথা যদি (nodep->name() == "atoreal") {
                fmt = AstAtoN::ATOREAL;
            } অন্যথা {
                nodep->v3fatalSrc("Bad case");
                fmt = AstAtoN::ATOI;
            }  // dummy assignment to suppress compiler warning
            methodOkArguments(nodep, 0, 0);
            Ast_গ্ৰন্থি* ধ্রুৱ newp
                = নূতন AstAtoN{nodep->fileline(), nodep->fromp()->unlinkFrBack(), fmt};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
        } অন্যথা {
            nodep->v3error("Unknown built-in string method " << nodep->prettyNameQ());
        }
    }
    AstQueueDType* queueDTypeIndexedBy(Ast_গ্ৰন্থি_DType* indexDTypep) {
        // Return a Queue data type with the specified index, remembering so can use again if
        // needed
        যদি (AstQueueDType* ধ্রুৱ queuep = m_queueDTypeIndexed[indexDTypep]) {
            ঘূৰাই queuep;
        } অন্যথা {
            AstQueueDType* ধ্রুৱ newp
                = নূতন AstQueueDType{indexDTypep->fileline(), indexDTypep, nullptr};
            v3Global.rootp()->typeTablep()->addTypesp(newp);
            m_queueDTypeIndexed[indexDTypep] = newp;
            ঘূৰাই newp;
        }
    }

    শূন্য visit(AstNew* nodep) override {
        যদি (nodep->didWidth()) ঘূৰাই;
        AstClass* classp = nullptr;
        তার্কিক assign = মিছা;
        যদি (VN_IS(nodep->backp(), Assign)) {  // assignment case
            assign = সঁচা;
            Ast_গ্ৰন্থি* warnp = nullptr;
            AstClassRefDType* refp = nullptr;

            যদি (nodep->isScoped()) {  // = ClassOrPackage::new
                UASSERT_OBJ(nodep->classOrPackagep(), nodep, "Unlinked classOrPackage");
                warnp = nodep->classOrPackagep();
                যদি (AstClass* ধ্রুৱ clsp = VN_CAST(warnp, Class)) {
                    AstClassRefDType* ধ্রুৱ adtypep
                        = নূতন AstClassRefDType{nodep->fileline(), clsp, nullptr};
                    v3Global.rootp()->typeTablep()->addTypesp(adtypep);
                    refp = adtypep;
                }
            } অন্যথা {  // = new
                warnp = m_vup->dtypeNullp();
                refp = m_vup ? VN_CAST(m_vup->dtypeNullSkipRefp(), ClassRefDType) : nullptr;
            }
            যদি (!refp) {  // e.g. int a = new;
                nodep->v3error(
                    "new() assignment not legal to non-class "
                    + (VN_IS(warnp, গ্ৰন্থি_DType)
                           ? ("data type "s + VN_AS(warnp, গ্ৰন্থি_DType)->prettyDTypeNameQ())
                       : warnp ? warnp->prettyNameQ()
                               : ""));
                nodep->dtypep(m_vup->dtypep());
                ঘূৰাই;
            }
            nodep->dtypep(refp);

            classp = refp->classp();
            UASSERT_OBJ(classp, nodep, "Unlinked");
            যদি (Ast_গ্ৰন্থি_FTask* ধ্রুৱ ftaskp
                = VN_CAST(m_memberMap.findMember(classp, "new"), Func)) {
                nodep->taskp(ftaskp);
                nodep->classOrPackagep(classp);
            } অন্যথা {
                // Either made explicitly or V3LinkDot made implicitly
                classp->v3fatalSrc("Can't find class's new");
            }
        } অন্যথা {  // super.new case
            // in this case class and taskp() should be properly linked in V3LinkDot.cpp during
            // "super" reference resolution
            classp = VN_CAST(nodep->classOrPackagep(), Class);
            UASSERT_OBJ(classp, nodep, "Unlinked classOrPackagep()");
            UASSERT_OBJ(nodep->taskp(), nodep, "Unlinked taskp()");
        }
        processFTaskRefArgs(nodep);
        যদি (!assign) nodep->dtypeFrom(nodep->taskp());
    }
    শূন্য visit(AstNewCopy* nodep) override {
        যদি (nodep->didWidthAndSet()) ঘূৰাই;
        AstClassRefDType* ধ্রুৱ refp = VN_CAST(m_vup->dtypeNullSkipRefp(), ClassRefDType);
        যদি (!refp) {  // e.g. int a = new;
            nodep->v3error("new() cannot copy from non-class data type "
                           + (m_vup->dtypeNullp() ? m_vup->dtypep()->prettyDTypeNameQ() : ""));
            nodep->dtypep(m_vup->dtypep());
            ঘূৰাই;
        }
        nodep->dtypep(refp);
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
        যদি (!similarDTypeRecurse(nodep->dtypep(), nodep->rhsp()->dtypep())) {
            nodep->rhsp()->v3error("New-as-copier passed different data type '"
                                   << nodep->dtypep()->prettyTypeName() << "' than expected '"
                                   << nodep->rhsp()->dtypep()->prettyTypeName() << "'");
        }
    }
    শূন্য visit(AstNewDynamic* nodep) override {
        যদি (nodep->didWidthAndSet()) ঘূৰাই;
        AstDynArrayDType* ধ্রুৱ adtypep = VN_CAST(m_vup->dtypeNullSkipRefp(), DynArrayDType);
        যদি (!adtypep) {  // e.g. int a = new;
            nodep->v3error(
                "dynamic new() not expected in this context (data type must be dynamic array)");
            ঘূৰাই;
        }
        // The AstNodeAssign visitor will be soon be replacing this node, make sure it gets it
        যদি (!VN_IS(nodep->backp(), গ্ৰন্থি_Assign)) {
            UINFO(1, "Got backp " << nodep->backp());
            nodep->v3error(
                "dynamic new() not expected in this context (expected under an assign)");
            ঘূৰাই;
        }
        nodep->dtypep(adtypep);
        যদি (m_vup && m_vup->prelim()) {
            iterateCheckSigned32(nodep, "new() size", nodep->sizep(), BOTH);
        }
        যদি (nodep->rhsp()) {
            iterateCheckTyped(nodep, "Dynamic array new RHS", nodep->rhsp(), adtypep, BOTH);
            // A keyed or defaulted initializer became a constructor, which takes the size
            AstConsDynArray* ধ্রুৱ consp = VN_CAST(nodep->rhsp(), ConsDynArray);
            যদি (consp && (consp->defaultp() || (consp->itemsp() && consp->itemsp()->keyp()))) {
                consp->sizep(nodep->sizep()->unlinkFrBack());
                nodep->replaceWith(consp->unlinkFrBack());
                VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
            }
        }
    }

    শূন্য visit(Ast_Tagged_ৰাশিমালা* nodep) override {
        // Tagged union expressions are currently unsupported
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: tagged union");
        // Set a placeholder type to allow further processing
        nodep->dtypeSetBit();
        userIterateChildren(nodep, m_vup);
    }
    শূন্য visit(AstTaggedPattern* nodep) override {
        // Tagged patterns are currently unsupported
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: tagged pattern");
        nodep->dtypeSetBit();
        userIterateChildren(nodep, m_vup);
    }
    শূন্য visit(AstPatternVar* nodep) override {
        // Pattern variable bindings are currently unsupported
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: pattern variable");
        nodep->dtypeSetBit();
    }
    শূন্য visit(AstPatternStar* nodep) override {
        // Pattern wildcards are currently unsupported
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: pattern wildcard");
        nodep->dtypeSetBit();
    }
    শূন্য visit(AstMatches* nodep) override {
        // Matches operator is currently unsupported
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: matches operator");
        // Matches returns a boolean
        nodep->dtypeSetBit();
        userIterateChildren(nodep, m_vup);
    }

    শূন্য visit(AstPattern* nodep) override {
        যদি (m_inParamOverride && !nodep->childDTypep() && !m_vup->dtypeNullp()) {
            // The specialized module types an override pattern, so only type its members here
            VL_RESTORER(m_inParamOverride);
            বাবে (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;
                 patp = VN_AS(patp->nextp(), PatMember)) {
                m_inParamOverride = মিছা;
                userIterateAndNext(patp->repp(), WidthVP{SELF, BOTH}.p());
                // Struct member names and type keys need no typing
                যদি (!VN_IS(patp->keyp(), Text) && !VN_IS(patp->keyp(), গ্ৰন্থি_DType)) {
                    userIterateAndNext(patp->keyp(), WidthVP{SELF, BOTH}.p());
                }
                বাবে (Ast_গ্ৰন্থি *nextp, *valuep = patp->lhssp(); valuep; valuep = nextp) {
                    nextp = valuep->nextp();
                    // A nested pattern stays untyped, any other value is typed now
                    m_inParamOverride = VN_IS(valuep, Pattern);
                    userIterate(valuep, WidthVP{SELF, BOTH}.p());
                }
            }
            ঘূৰাই;
        }
        // Any other pattern is typed here, so its members aren't override patterns
        VL_RESTORER(m_inParamOverride);
        m_inParamOverride = মিছা;
        যদি (nodep->didWidthAndSet()) ঘূৰাই;
        UINFO(9, "PATTERN " << nodep);
        যদি (nodep->childDTypep()) {  // data_type '{ pattern }
            nodep->dtypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        }
        যদি (!nodep->dtypep() && m_vup->dtypeNullp()) {  // Get it from parent assignment/pin/etc
            nodep->dtypep(m_vup->dtypep());
        }
        Ast_গ্ৰন্থি_DType* dtypep = nodep->dtypep();
        যদি (!dtypep) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported/Illegal: Assignment pattern"
                                         " member not underneath a supported construct: "
                                             << nodep->backp()->prettyTypeName());

            যদি (nodep->backp() && (VN_IS(nodep->backp(), Eq) || VN_IS(nodep->backp(), Neq)))
                ঘূৰাই;
            nodep->replaceWith(নূতন AstConst{nodep->fileline(), AstConst::BitFalse{}});
            VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
            ঘূৰাই;
        }
        {
            dtypep = dtypep->skipRefp();
            nodep->dtypep(dtypep);
            UINFO(9, "  dtypep " << dtypep);
            nodep->dtypep(dtypep);
            যদি (VN_IS(dtypep, UnionDType)) {
                nodep->v3error("Assignment pattern cannot be used for union"
                               " (IEEE 1800-2023 10.9)");
                ঘূৰাই;
            }
            // Determine replication count, and replicate initial value as
            // widths need to be individually determined
            বাবে (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;
                 patp = VN_AS(patp->nextp(), PatMember)) {
                ধ্রুৱ পূর্ণাঙ্ক times = visitPatMemberRep(patp);
                বাবে (পূর্ণাঙ্ক i = 1; i < times; i++) {
                    AstPatMember* ধ্রুৱ newp = patp->clone_বৃক্ষ(মিছা);
                    patp->addNextHere(newp);
                    // This loop will see the new elements as part of nextp()
                }
            }
            // Convert any PatMember with multiple items to multiple PatMembers
            বাবে (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;
                 patp = VN_AS(patp->nextp(), PatMember)) {
                যদি (patp->lhssp()->nextp()) {
                    // Can't just addNext, as would add to end of all members.
                    // So detach, add next and reattach
                    VNRelinker relinkHandle;
                    patp->unlinkFrBack(&relinkHandle);
                    যেতিয়ালৈ (Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ movep = VN_AS(patp->lhssp()->nextp(), গ্ৰন্থি_ৰাশিমালা)) {
                        movep->unlinkFrBack();  // Not unlinkFrBackWithNext, just one
                        Ast_গ্ৰন্থি* newkeyp = nullptr;
                        যদি (patp->keyp()) newkeyp = patp->keyp()->clone_বৃক্ষ(সঁচা);
                        AstPatMember* ধ্রুৱ newp
                            = নূতন AstPatMember{patp->fileline(), movep, newkeyp, nullptr};
                        patp->addNext(newp);
                    }
                    relinkHandle.relink(patp);
                }
            }
            AstPatMember* defaultp = nullptr;
            বাবে (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;) {
                AstPatMember* ধ্রুৱ nextp = VN_AS(patp->nextp(), PatMember);
                যদি (patp->isDefault()) {
                    যদি (defaultp) {
                        VL_DO_DANGLING(ঠেলো_Deletep(defaultp), defaultp);
                        nodep->v3error("Multiple '{ default: } clauses");
                    }
                    defaultp = patp;
                    patp->unlinkFrBack();
                }
                patp = nextp;
            }
            যেতিয়ালৈ (ধ্রুৱ AstConstDType* ধ্রুৱ vdtypep = VN_CAST(dtypep, ConstDType)) {
                dtypep = vdtypep->subDTypep()->skipRefp();
            }

            userIterate(dtypep, WidthVP{SELF, BOTH}.p());

            যদি (auto* ধ্রুৱ vdtypep = VN_CAST(dtypep, StructDType)) {
                patternStruct(nodep, vdtypep, defaultp);
            } অন্যথা যদি (auto* ধ্রুৱ vdtypep = VN_CAST(dtypep, গ্ৰন্থি_Array_DType)) {
                patternArray(nodep, vdtypep, defaultp);
            } অন্যথা যদি (VN_IS(dtypep, AssocArrayDType)) {
                AstConsAssoc* ধ্রুৱ consp = নূতন AstConsAssoc{nodep->fileline()};
                patternKeyed<AstConsAssocItem>(nodep, consp, dtypep, defaultp);
            } অন্যথা যদি (VN_IS(dtypep, WildcardArrayDType)) {
                AstConsWildcard* ধ্রুৱ consp = নূতন AstConsWildcard{nodep->fileline()};
                patternKeyed<AstConsWildcardItem>(nodep, consp, dtypep, defaultp);
            } অন্যথা যদি (VN_IS(dtypep, DynArrayDType) || VN_IS(dtypep, QueueDType)) {
                patternDynArrayOrQueue(nodep, dtypep, defaultp);
            } অন্যথা যদি (VN_IS(dtypep, BasicDType) && VN_AS(dtypep, BasicDType)->isRanged()) {
                patternBasic(nodep, dtypep, defaultp);
            } অন্যথা {
                nodep->v3warn(
                    E_UNSUPPORTED,
                    "Unsupported: Assignment pattern applies against non-struct/union data type: "
                        << dtypep->prettyDTypeNameQ());
                nodep->replaceWith(নূতন AstConst{nodep->fileline(), AstConst::BitFalse{}});
            }

            // Done with the Pattern
            VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
            যদি (defaultp) VL_DO_DANGLING(ঠেলো_Deletep(defaultp), defaultp);
        }
    }
    শূন্য patternStruct(AstPattern* nodep, AstStructDType* vdtypep, AstPatMember* defaultp) {
        // Due to "default" and tagged patterns, we need to determine
        // which member each AstPatMember corresponds to before we can
        // determine the dtypep for that PatMember's value, and then
        // width the initial value appropriately.
        using PatMap = std::map<ধ্রুৱ AstMemberDType*, AstPatMember*>;
        PatMap patmap;  // Store member: value
        DTypeMap dtypemap;  // Store data_type: default_value
        {
            ধ্রুৱ AstMemberDType* memp = vdtypep->membersp();
            AstPatMember* patp = VN_CAST(nodep->itemsp(), PatMember);
            যেতিয়ালৈ (patp) {
                কৰা {
                    যদি (patp->keyp()) {
                        // '{member:value} or '{data_type: default_value}
                        যদি (ধ্রুৱ AstText* textp = VN_CAST(patp->keyp(), Text)) {
                            // member: value
                            memp = VN_CAST(m_memberMap.findMember(vdtypep, textp->text()),
                                           MemberDType);
                            যদি (!memp) {
                                patp->keyp()->v3error("Assignment pattern key '"
                                                      << textp->text() << "' not found as member");
                                ভঙ্গ;
                            } অন্যথা {
                                ধ্রুৱ std::pair<PatMap::iterator, তার্কিক> ret
                                    = patmap.emplace(memp, patp);
                                যদি (!ret.second) {
                                    patp->v3error("Assignment pattern contains duplicate entry: "
                                                  << VN_AS(patp->keyp(), Text)->text());
                                }
                                memp = VN_AS(memp->nextp(), MemberDType);
                            }
                        } অন্যথা যদি (Ast_গ্ৰন্থি_DType* nodedtypep = VN_CAST(patp->keyp(), গ্ৰন্থি_DType)) {
                            // data_type: default_value
                            userIterate(nodedtypep, WidthVP{SELF, BOTH}.p());
                            ধ্রুৱ string dtype = nodedtypep->dtypep()->prettyDTypeName(সঁচা);
                            // Override stored default_value
                            dtypemap[dtype] = patp;
                        } অন্যথা {
                            // Undefined pattern
                            patp->keyp()->v3error(
                                "Assignment pattern key not supported/understood: "
                                << patp->keyp()->prettyTypeName());
                        }
                    } অন্যথা যদি (memp) {
                        // constant expr
                        ধ্রুৱ std::pair<PatMap::iterator, তার্কিক> ret = patmap.emplace(memp, patp);
                        যদি (!ret.second) {
                            patp->v3error("Assignment pattern contains duplicate entry: "
                                          << VN_AS(patp->keyp(), Text)->text());
                        }
                        memp = VN_AS(memp->nextp(), MemberDType);
                    } অন্যথা {
                        patp->v3error(
                            "Assignment pattern contains more entries than structure members");
                    }
                } যেতিয়ালৈ (মিছা);

                // Next
                যদি (patp) patp = VN_AS(patp->nextp(), PatMember);
            }
        }
        Ast_গ্ৰন্থি_ৰাশিমালা* newp = nullptr;
        যদি (vdtypep->packed()) {
            বাবে (AstMemberDType* memp = vdtypep->membersp(); memp;
                 memp = VN_AS(memp->nextp(), MemberDType)) {
                ধ্রুৱ auto it = patmap.find(memp);
                যদি (it == patmap.end()) {  // default or default_type assignment
                    যদি (AstStructDType* ধ্রুৱ memp_nested_vdtypep
                        = VN_CAST(memp->virtRefDTypep(), StructDType)) {
                        newp = nestedvalueConcat_patternStruct(memp_nested_vdtypep, defaultp, newp,
                                                               nodep, dtypemap);
                    } অন্যথা {
                        AstPatMember* ধ্রুৱ patp
                            = defaultPatp_patternStruct(nodep, memp, defaultp, dtypemap);
                        newp = মান_Concat_pattern_Struct(patp, newp, memp, nodep);
                        যদি (patp) VL_DO_DANGLING(ঠেলো_Deletep(patp), patp);
                    }
                } অন্যথা {  // member assignment
                    newp = মান_Concat_pattern_Struct(it->second, newp, memp, nodep);
                }
            }
        } অন্যথা {  // Unpacked
            AstConsStruct* ধ্রুৱ consp = নূতন AstConsStruct{nodep->fileline(), vdtypep};
            বাবে (AstMemberDType* memp = vdtypep->membersp(); memp;
                 memp = VN_AS(memp->nextp(), MemberDType)) {
                ধ্রুৱ auto it = patmap.find(memp);
                AstPatMember* patp = nullptr;
                যদি (it == patmap.end()) {  // Default or default_type assignment
                    patp = defaultPatp_patternStruct(nodep, memp, defaultp, dtypemap);
                    ঠেলো_Deletep(patp);  // patp used below
                    patp = defaultPatp_forDType(patp, memp->virtRefDTypep(), dtypemap);
                    ঠেলো_Deletep(patp);  // patp used below
                } অন্যথা {
                    patp = it->second;  // Member assignment
                }
                patp->dtypep(memp);
                Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ valuep = pattern_Member_মান_Iterate(patp);
                consp->addItemsp(নূতন AstConsStructItem{patp->fileline(), memp, valuep});
            }
            newp = consp;
        }
        যদি (newp) {
            nodep->replaceWith(newp);
        } অন্যথা {
            nodep->v3error("Assignment pattern with no members");
        }
    }

    Ast_গ্ৰন্থি_ৰাশিমালা* nestedvalueConcat_patternStruct(AstStructDType* memp_vdtypep,
                                                 AstPatMember* defaultp, Ast_গ্ৰন্থি_ৰাশিমালা* newp,
                                                 AstPattern* nodep, ধ্রুৱ DTypeMap& dtypemap) {
        বাবে (AstMemberDType* memp_nested = memp_vdtypep->membersp(); memp_nested;
             memp_nested = VN_AS(memp_nested->nextp(), MemberDType)) {
            যদি (AstStructDType* ধ্রুৱ memp_multinested_vdtypep
                = VN_CAST(memp_nested->virtRefDTypep(), StructDType)) {
                newp = nestedvalueConcat_patternStruct(memp_multinested_vdtypep, defaultp, newp,
                                                       nodep, dtypemap);
            } অন্যথা {
                AstPatMember* ধ্রুৱ patp
                    = defaultPatp_patternStruct(nodep, memp_nested, defaultp, dtypemap);
                newp = মান_Concat_pattern_Struct(patp, newp, memp_nested, nodep);
                যদি (patp) VL_DO_DANGLING(ঠেলো_Deletep(patp), patp);
            }
        }
        ঘূৰাই newp;
    }

    AstPatMember* defaultPatp_patternStruct(AstPattern* nodep, AstMemberDType* memp,
                                            AstPatMember* defaultp, ধ্রুৱ DTypeMap& dtypemap) {
        ধ্রুৱ string memp_DType = memp->virtRefDTypep()->prettyDTypeName(সঁচা);
        ধ্রুৱ auto it = dtypemap.find(memp_DType);
        যদি (it != dtypemap.end()) {
            // default_value for data_type
            ঘূৰাই it->second->clone_বৃক্ষ(মিছা);
        }
        যদি (defaultp) {
            // default_value for any unmatched member yet
            ঘূৰাই defaultp->clone_বৃক্ষ(মিছা);
        }
        যদি (memp->valuep()) {
            ঘূৰাই নূতন AstPatMember{nodep->fileline(),
                                    VN_AS(memp->valuep()->clone_বৃক্ষ(মিছা), গ্ৰন্থি_ৰাশিমালা),
                                    নূতন AstText{nodep->fileline(), memp->name()}, nullptr};
        }
        nodep->v3error("Assignment pattern missed initializing elements: "
                       << memp->virtRefDTypep()->prettyDTypeNameQ() << " " << memp->prettyNameQ());
        ঘূৰাই nullptr;
    }

    Ast_গ্ৰন্থি_ৰাশিমালা* মান_Concat_pattern_Struct(AstPatMember* patp, Ast_গ্ৰন্থি_ৰাশিমালা* newp,
                                           AstMemberDType* memp, AstPattern* nodep) {
        যদি (patp) {
            patp->dtypep(memp);
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ valuep = pattern_Member_মান_Iterate(patp);
            যদি (!newp) {
                newp = valuep;
            } অন্যথা {
                AstConcat* ধ্রুৱ concatp = নূতন AstConcat{patp->fileline(), newp, valuep};
                // Have resolved widths, no reason to warn about widths on the concat itself
                newp->fileline()->modifyWarnOff(V3ErrorCode::WIDTHCONCAT, সঁচা);
                valuep->fileline()->modifyWarnOff(V3ErrorCode::WIDTHCONCAT, সঁচা);
                newp = concatp;
                newp->dtypeChgSigned(nodep->dtypep()->numeric());
            }
        }
        ঘূৰাই newp;
    }

    AstPatMember* defaultPatp_forDType(AstPatMember* defaultp, Ast_গ্ৰন্থি_DType* elemDTypep,
                                       ধ্রুৱ DTypeMap& dtypemap = DTypeMap{}) {
        Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ valuep = defaultp->lhssp()->clone_বৃক্ষ(মিছা);
        Ast_গ্ৰন্থি_DType* ধ্রুৱ elemDTypeSkipRefp = elemDTypep->skipRefp();
        ধ্রুৱ AstStructDType* ধ্রুৱ structp = VN_CAST(elemDTypeSkipRefp, StructDType);
        ধ্রুৱ তার্কিক unpackedStruct = structp && !structp->packed();

        যদি (!VN_IS(elemDTypeSkipRefp, UnpackArrayDType) && !unpackedStruct) {
            VL_DO_DANGLING(ঠেলো_Deletep(valuep), valuep);
            ঘূৰাই defaultp->clone_বৃক্ষ(মিছা);
        }
        যদি (VN_IS(valuep, Pattern)) {
            VL_DO_DANGLING(ঠেলো_Deletep(valuep), valuep);
            ঘূৰাই defaultp->clone_বৃক্ষ(মিছা);
        }
        যদি (!valuep->dtypep()) userIterate(valuep, WidthVP{SELF, BOTH}.p());
        তার্কিক wholeElement = unpackedStruct;
        যদি (Ast_গ্ৰন্থি_DType* ধ্রুৱ মান_DTypep = valuep->dtypep()) {
            // An array value only if of the matching type, else it applies to its elements
            // (IEEE 1800-2023 10.9.1)
            wholeElement = unpackedStruct ? !মান_DTypep->skipRefp()->isIntegralOrPacked()
                                          : মান_DTypep->similarDType(elemDTypeSkipRefp);
        }
        যদি (wholeElement) {
            VL_DO_DANGLING(ঠেলো_Deletep(valuep), valuep);
            ঘূৰাই defaultp->clone_বৃক্ষ(মিছা);
        }

        AstPatMember* ধ্রুৱ nestedDefaultp
            = নূতন AstPatMember{defaultp->fileline(), valuep, nullptr, nullptr};
        nestedDefaultp->isDefault(সঁচা);
        // Propagate the outer 'data_type: value' entries into the nested aggregate
        বাবে (ধ্রুৱ auto& entry : dtypemap) nestedDefaultp->addNext(entry.second->clone_বৃক্ষ(মিছা));
        AstPattern* ধ্রুৱ recursivePatternp = নূতন AstPattern{defaultp->fileline(), nestedDefaultp};
        ঘূৰাই নূতন AstPatMember{defaultp->fileline(), recursivePatternp, nullptr, nullptr};
    }

    // Return the value of a pattern default for elements of the given type, or nullptr if no
    // default. For aggregate elements, it applies to their elements (IEEE 1800-2023 10.9.1).
    Ast_গ্ৰন্থি_ৰাশিমালা* patternDefaultValuep(AstPatMember* defaultp, Ast_গ্ৰন্থি_DType* elemDTypep) {
        যদি (!defaultp) ঘূৰাই nullptr;
        AstPatMember* ধ্রুৱ elemDefaultp = defaultPatp_forDType(defaultp, elemDTypep);
        elemDefaultp->dtypep(elemDTypep);
        Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ valuep = pattern_Member_মান_Iterate(elemDefaultp);
        VL_DO_DANGLING(ঠেলো_Deletep(elemDefaultp), elemDefaultp);
        ঘূৰাই valuep;
    }
    // An unpacked array concatenation, with each item an element, or an array of the elements
    // spliced in (IEEE 1800-2023 10.10)
    শূন্য patternConcatArray(AstPattern* nodep, AstUnpackArrayDType* arrayDtp) {
        ধ্রুৱ VNumRange range = arrayDtp->declRange();
        ধ্রুৱ পূর্ণাঙ্ক elements = range.elements();
        AstConsArray* ধ্রুৱ newp = নূতন AstConsArray{nodep->fileline(), arrayDtp};
        পূর্ণাঙ্ক entn = 0;  // Number of elements given so far, from the left
        তার্কিক error = মিছা;  // Reported an error, so the value is not complete
        বাবে (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;
             patp = VN_AS(patp->nextp(), PatMember)) {
            patp->dtypep(arrayDtp->subDTypep());
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ valuep = pattern_Member_মান_Iterate(patp);
            ধ্রুৱ uint64_t index = range.ascending() ? entn : elements - 1 - entn;
            ধ্রুৱ AstUnpackArrayDType* ধ্রুৱ spliceDtp
                = VN_CAST(valuep->dtypep()->skipRefp(), UnpackArrayDType);
            যদি (spliceDtp && spliceDtp->subDTypep()->similarDType(arrayDtp->subDTypep())) {
                entn += spliceDtp->elementsConst();
                newp->splice_মানসমূহ(index, valuep);
                অনুৱর্ত;
            }
            ধ্রুৱ VCastable castable = Ast_গ্ৰন্থি::computeCastable(
                arrayDtp->subDTypep()->skipRefp(), valuep->dtypep()->skipRefp(), valuep);
            যদি (castable != VCastable::SAMEISH && castable != VCastable::COMPATIBLE) {
                valuep->v3error("Unpacked array concatenation item is neither an element nor"
                                " an array of elements of its target (IEEE 1800-2023 10.10)"
                                << warnDTypes(valuep, arrayDtp->subDTypep(), valuep->dtypep(),
                                              "Element", "Item"));
                error = সঁচা;
            }
            newp->valuep(index, valuep);
            ++entn;
        }
        যদি (!error && entn != elements) {
            nodep->v3error("Unpacked array concatenation has "
                           << entn << " elements, but its target has " << elements
                           << " (IEEE 1800-2023 10.10)");
            error = সঁচা;
        }
        যদি (error) {
            VL_DO_DANGLING(ঠেলো_Deletep(newp), newp);
            nodep->replaceWith(নূতন AstConst{nodep->fileline(), AstConst::BitFalseErroring{}});
        } অন্যথা {
            nodep->replaceWith(newp);
        }
    }
    শূন্য patternArray(AstPattern* nodep, Ast_গ্ৰন্থি_Array_DType* arrayDtp, AstPatMember* defaultp) {
        যদি (nodep->itemsp() && VN_AS(nodep->itemsp(), PatMember)->isConcat()) {
            patternConcatArray(nodep, VN_AS(arrayDtp, UnpackArrayDType));
            ঘূৰাই;
        }
        ধ্রুৱ VNumRange range = arrayDtp->declRange();
        PatVecMap patmap = patVectorMap(nodep, range);
        UINFO(9, "ent " << range.বাম() << " to " << range.ডান());
        Ast_গ্ৰন্থি* newp = nullptr;
        তার্কিক missed = মিছা;  // Some element missed, so not a complete value
        বাবে (পূর্ণাঙ্ক entn = 0, ent = range.বাম(); entn < range.elements();
             ++entn, ent += range.বাম_To_ডান_Inc()) {
            Ast_গ্ৰন্থি_ৰাশিমালা* valuep = nullptr;
            ধ্রুৱ auto it = patmap.find(ent);
            যদি (it == patmap.end()) {
                যদি (defaultp) {
                    valuep = patternDefaultValuep(defaultp, arrayDtp->subDTypep());
                } অন্যথা {
                    nodep->v3error("Assignment pattern missed initializing elements: " << ent);
                    missed = সঁচা;
                }
            } অন্যথা {
                AstPatMember* ধ্রুৱ patp = it->second;
                patmap.erase(it);
                patp->dtypep(arrayDtp->subDTypep());
                valuep = pattern_Member_মান_Iterate(patp);
                // Only an unpacked array concatenation splices in arrays
                ধ্রুৱ AstUnpackArrayDType* ধ্রুৱ memberDtp
                    = VN_CAST(valuep->dtypep()->skipRefp(), UnpackArrayDType);
                যদি (memberDtp && memberDtp->subDTypep()->similarDType(arrayDtp->subDTypep())) {
                    valuep->v3error("Assignment pattern member is an array of the elements, not"
                                    " an element (IEEE 1800-2023 10.9.1)"
                                    << warnDTypes(valuep, arrayDtp->subDTypep(), memberDtp,
                                                  "Element", "Pattern member")
                                    << "\n"
                                    << valuep->warnMore()
                                    << "... Suggest use an unpacked array concatenation {...} to "
                                       "splice arrays");
                    VL_DO_DANGLING(ঠেলো_Deletep(valuep), valuep);
                    valuep = নূতন AstConst{patp->fileline(), AstConst::BitFalseErroring{}};
                }
            }

            যদি (valuep) {
                যদি (VN_IS(arrayDtp, UnpackArrayDType)) {
                    যদি (!newp) newp = নূতন AstConsArray{nodep->fileline(), arrayDtp};
                    VN_AS(newp, ConsArray)->valuep(ent - range.lo(), valuep);
                } অন্যথা {  // Packed. Convert to concat for now.
                    যদি (!newp) {
                        newp = valuep;
                    } অন্যথা {
                        AstConcat* ধ্রুৱ concatp
                            = নূতন AstConcat{valuep->fileline(), VN_AS(newp, গ্ৰন্থি_ৰাশিমালা), valuep};
                        newp = concatp;
                        newp->dtypeChgSigned(nodep->dtypep()->numeric());
                    }
                }
            }
        }
        যদি (!patmap.empty()) nodep->v3error("Assignment pattern with too many elements");
        যদি (missed && VN_IS(newp, ConsArray)) {
            VL_DO_DANGLING(ঠেলো_Deletep(newp), newp);
            newp = নূতন AstConst{nodep->fileline(), AstConst::BitFalseErroring{}};
        }
        যদি (newp) {
            nodep->replaceWith(newp);
        } অন্যথা {
            nodep->v3error("Assignment pattern with no members");
        }
        // UINFOTREE(9, newp, "", "apat-out");
    }
    // Return the key of an array pattern member, which must be a constant expression,
    // or nullptr after reporting an error
    AstConst* patternConstKeyp(AstPatMember* patp) {
        যদি (VN_IS(patp->keyp(), গ্ৰন্থি_DType)) {
            patp->keyp()->v3warn(E_UNSUPPORTED,
                                 "Unsupported: Data-type key in array assignment pattern");
            ঘূৰাই nullptr;
        }
        যদি (patp->চাবি_Exprp()) V3Const::constifyParamsNoWarnEdit(patp->চাবি_Exprp());  // May change
        AstConst* ধ্রুৱ constp = VN_CAST(patp->চাবি_Exprp(), Const);
        যদি (!constp) {
            patp->keyp()->v3error(
                "Assignment pattern key is not a constant expression (IEEE 1800-2023 10.9)");
        }
        ঘূৰাই constp;
    }
    // Return the key of a pattern member of a non-aggregate key type, converted to the key type,
    // unlinked, or nullptr after reporting an error
    AstConst* patternScalarKeyp(AstPatMember* patp, Ast_গ্ৰন্থি_DType* চাবি_DTypep) {
        যদি (!patp->keyp()) {
            patp->v3error("Missing pattern key (need an expression then a ':')");
            ঘূৰাই nullptr;
        }
        // An integral key is evaluated in the context of the key type, as an index converted to
        // it (IEEE 1800-2023 7.8.4), but not truncated before checking it for X or Z
        যদি (চাবি_DTypep->skipRefp()->isIntegralOrPacked() && patp->চাবি_Exprp()) {
            userIterate(patp->চাবি_Exprp(), WidthVP{চাবি_DTypep, PRELIM}.p());
            ধ্রুৱ পূর্ণাঙ্ক width = std::max(patp->চাবি_Exprp()->width(), চাবি_DTypep->width());
            Ast_গ্ৰন্থি_DType* ধ্রুৱ contextDTypep
                = চাবি_DTypep->findBitOrLogicDType(width, width, চাবি_DTypep->numeric(), সঁচা);
            iterateCheck(patp, "key", patp->চাবি_Exprp(), CONTEXT_DET, FINAL, contextDTypep,
                         EXTEND_EXP, মিছা);
        }
        AstConst* ধ্রুৱ constp = patternConstKeyp(patp);
        যদি (!constp) ঘূৰাই nullptr;
        যদি (constp->অঙ্ক().isFourState()) {
            patp->keyp()->v3error("Assignment pattern key cannot contain X or Z");
            ঘূৰাই nullptr;
        }
        // Convert to the key type. The conversion is inserted above 'constp', so take the key
        // from the member again rather than from what iterateCheck returns.
        iterateCheck(patp, "key", constp, CONTEXT_DET, FINAL, চাবি_DTypep, EXTEND_EXP, মিছা);
        Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ convp = patp->চাবি_Exprp();
        // Fold the conversion, so the key is a Const again
        Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ keyp = V3Const::constifyEdit(convp);
        UASSERT_OBJ(VN_IS(keyp, Const), patp, "Key not constant after conversion");
        ঘূৰাই VN_AS(keyp->unlinkFrBack(), Const);
    }
    // Return the key of a keyed pattern member, for the kind of constructor, unlinked, or nullptr
    // after reporting an error. Only associative arrays can have aggregate keys.
    Ast_গ্ৰন্থি_ৰাশিমালা* patternKeyp(AstConsAssoc*, AstPatMember* patp, Ast_গ্ৰন্থি_DType* arrayDtp) {
        Ast_গ্ৰন্থি_DType* ধ্রুৱ চাবি_DTypep = VN_AS(arrayDtp, AssocArrayDType)->চাবি_DTypep();
        যদি (!চাবি_DTypep->skipRefp()->isAggregateType() || !patp->চাবি_Exprp()) {
            ঘূৰাই patternScalarKeyp(patp, চাবি_DTypep);
        }
        // An aggregate key is folded, and constant if made of constant values only
        iterateCheckTyped(patp, "key", patp->চাবি_Exprp(), চাবি_DTypep, BOTH);
        V3Const::constifyParamsNoWarnEdit(patp->চাবি_Exprp());
        Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ foldedp = patp->চাবি_Exprp();
        যদি (!foldedp->is_Constant_বৃক্ষ()) {
            foldedp->v3error(
                "Assignment pattern key is not a constant expression (IEEE 1800-2023 10.9)");
            ঘূৰাই nullptr;
        }
        যদি (foldedp->exists([](ধ্রুৱ AstConst* constp) { ঘূৰাই constp->অঙ্ক().isFourState(); })) {
            foldedp->v3error("Assignment pattern key cannot contain X or Z");
            ঘূৰাই nullptr;
        }
        ঘূৰাই foldedp->unlinkFrBack();
    }
    AstConst* patternKeyp(AstConsWildcard*, AstPatMember* patp, Ast_গ্ৰন্থি_DType* arrayDtp) {
        ঘূৰাই patternScalarKeyp(patp, arrayDtp->findStringDType());
    }
    AstConst* patternKeyp(AstConsDynArray*, AstPatMember* patp, Ast_গ্ৰন্থি_DType* arrayDtp) {
        // Converted to longint, not int, so a key wider than 32 bits is out of bounds rather than
        // truncated to a valid index
        ঘূৰাই patternScalarKeyp(patp, arrayDtp->findBasicDType(VBasicDTypeKwd::LONGINT));
    }
    // Sort constructor items by key, so equal values have equal trees, and report duplicate
    // keys (IEEE 1800-2023 10.9.1). Keys are compared after conversion to the key type, e.g.
    // the wildcard keys "CCC" and 23'h434343 are equal.
    template <typename T_Item, typename T_Cons>
    শূন্য sort_Cons_বস্তুসমূহ(T_Cons* nodep) {
        std::vector<T_Item*> itemps;
        যেতিয়ালৈ (T_Item* ধ্রুৱ itemp = nodep->itemsp()) {
            itemp->unlinkFrBack();
            itemps.ঠেলো_back(itemp);
        }
        ধ্রুৱ auto চাবি_Of = [](ধ্রুৱ T_Item* itemp) -> ধ্রুৱ V3_সংখ্যা& {
            ধ্রুৱ Ast_গ্ৰন্থি* ধ্রুৱ keyp = itemp->keyp();
            ঘূৰাই VN_AS(keyp, Const)->অঙ্ক();
        };
        std::stable_sort(itemps.begin(), itemps.end(), [&](ধ্রুৱ T_Item* ap, ধ্রুৱ T_Item* bp) {
            // Any total order will do, it need not match the runtime order of the keys
            ঘূৰাই চাবি_Of(ap).isLtXZ(চাবি_Of(bp));
        });
        ধ্রুৱ T_Item* firstp = nullptr;  // First item with the current key
        বাবে (T_Item* ধ্রুৱ itemp : itemps) {
            যদি (firstp && চাবি_Of(firstp).isCaseEq(চাবি_Of(itemp))) {
                // Sort is stable, so 'firstp' is the first one in the source
                itemp->keyp()->v3error("Assignment pattern key used multiple times"
                                       " (IEEE 1800-2023 10.9.1)\n"
                                       << itemp->keyp()->warnContextPrimary() << '\n'
                                       << firstp->keyp()->warnOther()
                                       << "... Location of first use\n"
                                       << firstp->keyp()->warnContextSecondary());
            } অন্যথা {
                firstp = itemp;
            }
            nodep->addItemsp(itemp);
        }
    }
    // Convert a keyed pattern into the constructor 'consp' of an associative, wildcard or
    // dynamic array
    template <typename T_Item, typename T_Cons>
    শূন্য patternKeyed(AstPattern* nodep, T_Cons* consp, Ast_গ্ৰন্থি_DType* arrayDtp,
                      AstPatMember* defaultp) {
        consp->dtypeFrom(arrayDtp);
        consp->defaultp(patternDefaultValuep(defaultp, arrayDtp->subDTypep()));
        বাবে (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;
             patp = VN_AS(patp->nextp(), PatMember)) {
            ধ্রুৱ auto keyp = patternKeyp(consp, patp, arrayDtp);
            patp->dtypep(arrayDtp->subDTypep());
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ valuep = pattern_Member_মান_Iterate(patp);
            যদি (keyp) {
                consp->addItemsp(নূতন T_Item{patp->fileline(), keyp, valuep});
            } অন্যথা {
                VL_DO_DANGLING(ঠেলো_Deletep(valuep), valuep);
            }
        }
        // Aggregate keys are not sorted or checked for duplicates
        ধ্রুৱ AstAssocArrayDType* ধ্রুৱ assocDtp = VN_CAST(arrayDtp, AssocArrayDType);
        যদি (!assocDtp || !assocDtp->চাবি_DTypep()->skipRefp()->isAggregateType()) {
            sort_Cons_বস্তুসমূহ<T_Item>(consp);
        }
        nodep->replaceWith(consp);
        // UINFOTREE(9, consp, "", "apat-out");
    }
    Ast_গ্ৰন্থি_ৰাশিমালা* newConsDynArrayOrQueue(File_সাৰি* fileline, ধ্রুৱ Ast_গ্ৰন্থি_DType* dtypep) {
        যদি (VN_IS(dtypep, DynArrayDType)) ঘূৰাই নূতন AstConsDynArray{fileline};
        যদি (VN_IS(dtypep, QueueDType)) ঘূৰাই নূতন AstConsQueue{fileline};
        ঘূৰাই nullptr;
    }
    // Add 'valuep' to a dynamic array or queue constructor: as one element, or as an array whose
    // elements are spliced in. A constructor being spliced (e.g. a concatenation's left operand)
    // has its items moved over, or added one by one to convert them if its kind or element type
    // differs.
    template <typename T_Item, typename T_Cons>
    শূন্য addConsItem(T_Cons* consp, Ast_গ্ৰন্থি_ৰাশিমালা* valuep) {
        Ast_গ্ৰন্থি_DType* ধ্রুৱ arrayDtp = consp->dtypep()->skipRefp();
        // An element if assignable to the element type, else an array (IEEE 1800-2023 10.10)
        ধ্রুৱ তার্কিক isElement
            = Ast_গ্ৰন্থি::computeCastable(arrayDtp->subDTypep(), valuep->dtypep(), nullptr)
                  .isAssignable();
        // A bounded queue to splice is constructed first, to truncate it
        ধ্রুৱ AstQueueDType* ধ্রুৱ subQueueDtp = VN_CAST(valuep->dtypep()->skipRefp(), QueueDType);
        যদি (!isElement && !(subQueueDtp && subQueueDtp->boundp())) {
            T_Cons* ধ্রুৱ subp = Ast_গ্ৰন্থি::cast<T_Cons>(valuep);
            যদি (subp && subp->dtypep()->skipRefp()->subDTypep() == arrayDtp->subDTypep()) {
                যদি (subp->itemsp()) consp->addItemsp(subp->itemsp()->unlinkFrBackWithNext());
                VL_DO_DANGLING(ঠেলো_Deletep(subp), subp);
                ঘূৰাই;
            }
            যদি (AstConsQueue* ধ্রুৱ queuep = VN_CAST(valuep, ConsQueue)) {
                add_Cons_বস্তুসমূহ(consp, queuep);
                ঘূৰাই;
            }
            যদি (AstConsDynArray* ধ্রুৱ dynp = VN_CAST(valuep, ConsDynArray)) {
                add_Cons_বস্তুসমূহ(consp, dynp);
                ঘূৰাই;
            }
        }
        T_Item* ধ্রুৱ itemp = নূতন T_Item{valuep->fileline(), valuep};
        consp->addItemsp(itemp);
        // Width the value now, as constructors are widthed when created
        যদি (isElement) {
            iterateCheckTyped(itemp, "value", valuep, arrayDtp->subDTypep(), FINAL);
        } অন্যথা {
            userIterateAndNext(valuep, WidthVP{arrayDtp, FINAL}.p());
        }
    }
    শূন্য addConsItem(Ast_গ্ৰন্থি_ৰাশিমালা* consp, Ast_গ্ৰন্থি_ৰাশিমালা* valuep) {
        যদি (AstConsDynArray* ধ্রুৱ dynp = VN_CAST(consp, ConsDynArray)) {
            addConsItem<AstConsDynArrayItem>(dynp, valuep);
        } অন্যথা {
            addConsItem<AstConsQueueItem>(VN_AS(consp, ConsQueue), valuep);
        }
    }
    template <typename T_Sub>
    শূন্য add_Cons_বস্তুসমূহ(Ast_গ্ৰন্থি_ৰাশিমালা* consp, T_Sub* subp) {
        যেতিয়ালৈ (auto* ধ্রুৱ itemp = subp->itemsp()) {
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ valuep = itemp->valuep()->unlinkFrBack();
            VL_DO_DANGLING(ঠেলো_Deletep(itemp->unlinkFrBack()), itemp);
            addConsItem(consp, valuep);
        }
        VL_DO_DANGLING(ঠেলো_Deletep(subp), subp);
    }
    শূন্য patternDynArrayOrQueue(AstPattern* nodep, Ast_গ্ৰন্থি_DType* arrayp, AstPatMember* defaultp) {
        Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ newp = newConsDynArrayOrQueue(nodep->fileline(), arrayp);
        UASSERT_OBJ(newp, nodep, "Expected dynamic array or queue data type");
        newp->dtypeFrom(arrayp);
        // By the syntax, members are all keyed or all positional
        ধ্রুৱ AstPatMember* ধ্রুৱ firstp = VN_AS(nodep->itemsp(), PatMember);
        ধ্রুৱ তার্কিক keyed = defaultp || (firstp && firstp->keyp());
        যদি (keyed && VN_IS(arrayp, DynArrayDType) && VN_IS(nodep->backp(), NewDynamic)) {
            // The initializer of new[], visit(AstNewDynamic) then adds the size
            patternKeyed<AstConsDynArrayItem>(nodep, VN_AS(newp, ConsDynArray), arrayp, defaultp);
            ঘূৰাই;
        }
        ধ্রুৱ ৱর্ণ* ধ্রুৱ kindp = VN_IS(arrayp, DynArrayDType) ? "dynamic array" : "queue";
        যদি (defaultp) defaultp->v3error("Assignment pattern default cannot be used for " << kindp);
        বাবে (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;
             patp = VN_AS(patp->nextp(), PatMember)) {
            যদি (patp->keyp()) {
                patp->keyp()->v3error("Assignment pattern key cannot be used for " << kindp);
            }
            patp->dtypep(arrayp->subDTypep());
            addConsItem(newp, pattern_Member_মান_Iterate(patp));
        }
        nodep->replaceWith(newp);
        // UINFOTREE(9, newp, "", "apat-out");
    }
    শূন্য patternBasic(AstPattern* nodep, Ast_গ্ৰন্থি_DType* vdtypep, AstPatMember* defaultp) {
        ধ্রুৱ AstBasicDType* bdtypep = VN_AS(vdtypep, BasicDType);
        ধ্রুৱ VNumRange range = bdtypep->declRange();
        PatVecMap patmap = patVectorMap(nodep, range);
        UINFO(9, "ent " << range.hi() << " to " << range.lo());
        Ast_গ্ৰন্থি_ৰাশিমালা* newp = nullptr;
        বাবে (ধ্রুৱ পূর্ণাঙ্ক ent : range.seqHiToLo()) {
            Ast_গ্ৰন্থি_ৰাশিমালা* valuep = nullptr;
            ধ্রুৱ auto it = patmap.find(ent);
            যদি (it == patmap.end()) {
                যদি (defaultp) {
                    valuep = patternDefaultValuep(defaultp, nodep->findBitDType());
                } অন্যথা {
                    nodep->v3error("Assignment pattern missed initializing elements: " << ent);
                }
            } অন্যথা {
                AstPatMember* ধ্রুৱ patp = it->second;
                patmap.erase(it);
                patp->dtypep(nodep->findBitDType());
                valuep = pattern_Member_মান_Iterate(patp);
            }
            যদি (valuep) {  // Packed. Convert to concat for now.
                যদি (!newp) {
                    newp = valuep;
                } অন্যথা {
                    AstConcat* ধ্রুৱ concatp = নূতন AstConcat{valuep->fileline(), newp, valuep};
                    newp = concatp;
                    newp->dtypeChgSigned(nodep->dtypep()->numeric());
                }
            }
        }
        যদি (!patmap.empty()) nodep->v3error("Assignment pattern with too many elements");
        যদি (newp) {
            nodep->replaceWith(newp);
        } অন্যথা {
            nodep->v3error("Assignment pattern with no members");
        }
        // UINFOTREE(9, newp, "", "apat-out");
    }
    Ast_গ্ৰন্থি_ৰাশিমালা* pattern_Member_মান_Iterate(AstPatMember* patp) {
        // Determine values - might be another ConsArray
        userIterate(patp, WidthVP{patp->dtypep(), BOTH}.p());
        // Convert to ConsArray or constify immediately
        Ast_গ্ৰন্থি_ৰাশিমালা* valuep = patp->lhssp()->unlinkFrBack();
        যদি (VN_IS(valuep, Const)) {
            // Forming a AstConcat will cause problems with
            // unsized (uncommitted sized) constants
            যদি (AstConst* ধ্রুৱ newp = V3WidthCommit::newIfConstCommitSize(VN_AS(valuep, Const))) {
                VL_DO_DANGLING(ঠেলো_Deletep(valuep), valuep);
                valuep = newp;
            }
        }
        ঘূৰাই valuep;
    }

    স্থির শূন্য checkEventAssignment(ধ্রুৱ Ast_গ্ৰন্থি_Assign* ধ্রুৱ asgnp) {
        string unsupEvtAsgn;
        যদি (!usesDynamicScheduler(asgnp->lhsp())) unsupEvtAsgn = "to";
        যদি (VN_IS(asgnp->rhsp(), CReset)) ঘূৰাই;
        যদি (asgnp->rhsp()->dtypep()->isEvent() && !usesDynamicScheduler(asgnp->rhsp())) {
            unsupEvtAsgn += (unsupEvtAsgn.empty() ? "from" : " and from");
        }
        যদি (!unsupEvtAsgn.empty()) {
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

    স্থির তার্কিক usesDynamicScheduler(Ast_গ্ৰন্থি* nodep) {
        UASSERT_OBJ(nodep->dtypep()->isEvent(), nodep, "Node does not have an event dtype");
        যেতিয়ালৈ (সঁচা) {
            AstVarRef* ধ্রুৱ vrefp = VN_CAST(nodep, VarRef);
            যদি (vrefp) ঘূৰাই usesDynamicScheduler(vrefp);
            যদি (VN_IS(nodep, MemberSel)) {
                ঘূৰাই সঁচা;
            } অন্যথা যদি (Ast_গ্ৰন্থি_Sel* selp = VN_CAST(nodep, গ্ৰন্থি_Sel)) {
                nodep = selp->fromp();
            } অন্যথা {
                ঘূৰাই মিছা;
            }
        }
    }

    স্থির তার্কিক usesDynamicScheduler(AstVarRef* vrefp) {
        ঘূৰাই VN_IS(vrefp->classOrPackagep(), Class) || vrefp->varp()->isFuncLocal();
    }

    শূন্য visit(AstPatMember* nodep) override {
        Ast_গ্ৰন্থি_DType* ধ্রুৱ vdtypep = m_vup->dtypeNullp();
        UASSERT_OBJ(vdtypep, nodep, "Pattern member type not assigned by AstPattern visitor");
        nodep->dtypep(vdtypep);
        UINFO(9, "   PATMEMBER " << nodep);
        UASSERT_OBJ(!nodep->lhssp()->nextp(), nodep,
                    "PatMember value should be singular w/replicates removed");
        // Need to propagate assignment type downwards, even on prelim
        userIterateChildren(nodep, WidthVP{nodep->dtypep(), PRELIM}.p());
        iterateCheck(nodep, "Pattern value", nodep->lhssp(), ASSIGN, FINAL, vdtypep, EXTEND_LHS);
    }
    পূর্ণাঙ্ক visitPatMemberRep(AstPatMember* nodep) {
        uint32_t times = 1;
        যদি (nodep->repp()) {  // else repp()==nullptr shorthand for rep count 1
            iterateCheckSizedSelf(nodep, "LHS", nodep->repp(), SELF, BOTH);
            V3Const::constifyParamsEdit(nodep->repp());  // repp may change
            ধ্রুৱ AstConst* ধ্রুৱ constp = VN_CAST(nodep->repp(), Const);
            যদি (!constp) {
                nodep->v3error("Replication value isn't a constant.");
                times = 0;
            } অন্যথা {
                times = constp->toUInt();
            }
            যদি (times == 0) {
                nodep->v3error("Pattern replication value of 0 is not legal.");
                times = 1;
            }
            nodep->repp()
                ->unlinkFrBackWithNext()
                ->delete_বৃক্ষ();  // Done with replicate before cloning
        }
        ঘূৰাই times;
    }

    শূন্য visit(AstPropSpec* nodep) override {
        VL_RESTORER(m_seqUnsupp);
        VL_RESTORER(m_hasSExpr);
        assert_At_ৰাশিমালা(nodep);
        যদি (m_vup->prelim()) {  // First stage evaluation
            // Only bare strong until uses V3AssertPre's timing-based lowering. Embedded forms
            // are lowered without timing by V3AssertNfa.
            Ast_গ্ৰন্থি* propp = nodep->propp();
            যেতিয়ালৈ (AstLogNot* ধ্রুৱ notp = VN_CAST(propp, LogNot)) propp = notp->lhsp();
            AstUntil* ধ্রুৱ untilp = VN_CAST(propp, Until);
            যদি (untilp && VN_IS(nodep->backp(), গ্ৰন্থি_Cover_Or_Assert) && untilp->isStrong()
                && (v3Global.opt.timing().isSetFalse() || !v3Global.opt.timing().isSetTrue())) {
                untilp->v3warn(E_NOTIMING, untilp->verilogKwd() << " requires --timing");
                untilp->replaceWith(নূতন AstConst{untilp->fileline(), AstConst::BitFalse{}});
                VL_DO_DANGLING(ঠেলো_Deletep(untilp), untilp);
            }
            iterateCheckBool(nodep, "Property", nodep->propp(), BOTH);
            userIterateAndNext(nodep->sensesp(), nullptr);
            যদি (nodep->disablep()) {
                iterateCheckBool(nodep, "Disable", nodep->disablep(),
                                 BOTH);  // it's like an if() condition.
            }
            nodep->dtypeSetBit();
        }
    }

    তার্কিক firstNewStatementOkRecurse(Ast_গ্ৰন্থি* nodep) {
        যদি (AstVar* ধ্রুৱ varp = VN_CAST(nodep, Var)) {
            যদি (!varp->valuep() || VN_CAST(varp->valuep(), Const) || varp->isIO()) ঘূৰাই সঁচা;
        }
        যদি (AstAssign* ধ্রুৱ aitemp = VN_CAST(nodep, Assign)) {
            যদি (VN_IS(aitemp->rhsp(), Const) || VN_IS(aitemp->rhsp(), CReset)) ঘূৰাই সঁচা;
        }
        যদি (AstInitialStaticStmt* ধ্রুৱ aitemp = VN_CAST(nodep, InitialStaticStmt)) {
            বাবে (Ast_গ্ৰন্থি* stmtp = aitemp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                যদি (!firstNewStatementOkRecurse(stmtp)) ঘূৰাই মিছা;
            }
            ঘূৰাই সঁচা;
        }
        যদি (AstInitialAutomaticStmt* ধ্রুৱ aitemp = VN_CAST(nodep, InitialAutomaticStmt)) {
            বাবে (Ast_গ্ৰন্থি* stmtp = aitemp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                যদি (!firstNewStatementOkRecurse(stmtp)) ঘূৰাই মিছা;
            }
            ঘূৰাই সঁচা;
        }
        ঘূৰাই মিছা;
    }

    //--------------------
    // Top levels

    template <typename CaseItem>
    শূন্য handleCaseType(Ast_গ্ৰন্থি* casep, Ast_গ্ৰন্থি_ৰাশিমালা* exprp, CaseItem* itemsp) {
        AstAttrOf* ধ্রুৱ exprap = VN_CAST(exprp, AttrOf);
        যদি (!exprap) ঘূৰাই;
        যদি (exprap->attrType() != VAttrType::TYPEID) ঘূৰাই;

        ধ্রুৱ Ast_গ্ৰন্থি_DType* ধ্রুৱ ৰাশিমালা_Dtp = exprap->dtypep();
        UINFO(9, "case type exprDtp " << ৰাশিমালা_Dtp);
        // V3Param may have a pointer to this case statement, and we need
        // dotted references to remain properly named, so rather than
        // removing we convert it to a "normal" expression "case (1) ..."
        File_সাৰি* ধ্রুৱ newfl = casep->fileline();
        newfl->warnOff(V3ErrorCode::CASEINCOMPLETE, সঁচা);  // Side effect of transform
        newfl->warnOff(V3ErrorCode::CASEOVERLAP, সঁচা);  // Side effect of transform
        casep->fileline(newfl);
        বাবে (CaseItem* itemp = itemsp; itemp; itemp = Ast_গ্ৰন্থি::as<CaseItem>(itemp->nextp())) {
            যদি (itemp->isDefault()) অনুৱর্ত;
            তার্কিক hit = মিছা;
            বাবে (Ast_গ্ৰন্থি* condp = itemp->condsp(); condp; condp = condp->nextp()) {
                ধ্রুৱ AstAttrOf* ধ্রুৱ condAttrp = VN_CAST(condp, AttrOf);
                যদি (!condAttrp) {
                    condp->v3error("Case(type) statement requires items that have type() items");
                } অন্যথা {
                    Ast_গ্ৰন্থি_DType* ধ্রুৱ condDtp = condAttrp->dtypep();
                    যদি (Ast_গ্ৰন্থি::computeCastable(ৰাশিমালা_Dtp, condDtp, casep) == VCastable::SAMEISH) {
                        hit = সঁচা;
                        ভঙ্গ;
                    }
                }
            }
            ঠেলো_Deletep(itemp->condsp()->unlinkFrBackWithNext());
            // Item condition becomes constant 1 if hits else 0
            itemp->addCondsp(নূতন AstConst{newfl, AstConst::BitTrue{}, hit});
        }
        exprap->replaceWith(নূতন AstConst{newfl, AstConst::BitTrue{}});
        VL_DO_DANGLING(ঠেলো_Deletep(exprap), exprap);
    }

    template <typename CaseItem>
    শূন্য handleCase(Ast_গ্ৰন্থি* casep, Ast_গ্ৰন্থি_ৰাশিমালা* exprp, CaseItem* itemsp) {
        // IEEE-2012 12.5:
        //    Width: MAX(expr, all items)
        //    Signed: Only if expr, and all items signed
        // Take width as maximum across all items, if any is real whole thing is real
        Ast_গ্ৰন্থি_DType* subDTypep = exprp->dtypep();
        বাবে (CaseItem* itemp = itemsp; itemp; itemp = Ast_গ্ৰন্থি::as<CaseItem>(itemp->nextp())) {
            বাবে (Ast_গ্ৰন্থি* condp = itemp->condsp(); condp; condp = condp->nextp()) {
                যদি (condp->dtypep() == subDTypep) অনুৱর্ত;

                যদি (condp->dtypep()->isDouble() || subDTypep->isDouble()) {
                    subDTypep = casep->findDoubleDType();
                } অন্যথা যদি (condp->dtypep()->isString() || subDTypep->isString()) {
                    subDTypep = casep->findStringDType();
                } অন্যথা {
                    ধ্রুৱ পূর্ণাঙ্ক width = std::max(subDTypep->width(), condp->width());
                    ধ্রুৱ পূর্ণাঙ্ক mwidth = std::max(subDTypep->widthMin(), condp->widthMin());
                    ধ্রুৱ তার্কিক issigned = subDTypep->isSigned() && condp->isSigned();
                    subDTypep = casep->findBitOrLogicDType(
                        width, mwidth, VSigning::fromBool(issigned),
                        subDTypep->isFourstate() || condp->dtypep()->isFourstate());
                }
            }
        }

        // Apply width
        iterateCheck(casep, "Case expression", exprp, CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP);
        বাবে (CaseItem* itemp = itemsp; itemp; itemp = Ast_গ্ৰন্থি::as<CaseItem>(itemp->nextp())) {
            বাবে (Ast_গ্ৰন্থি *nextcp, *condp = itemp->condsp(); condp; condp = nextcp) {
                nextcp = condp->nextp();  // Final may cause the node to get replaced
                iterateCheck(casep, "Case Item", condp, CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP);
            }
        }
    }

    শূন্য visit(AstGenCase* nodep) override {
        assertAtStatement(nodep);
        // Type check expression and case item conditions, but not bodies
        userIterateAndNext(nodep->exprp(), WidthVP{CONTEXT_DET, PRELIM}.p());
        বাবে (AstGenCaseItem *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            // Prelim may cause the node to get replaced, pick up next up front
            nextip = VN_AS(itemp->nextp(), GenCaseItem);
            বাবে (Ast_গ্ৰন্থি *nextcp, *condp = itemp->condsp(); condp; condp = nextcp) {
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
    শূন্য visit(AstGenFor* nodep) override {
        assertAtStatement(nodep);
        userIterateAndNext(nodep->initsp(), nullptr);
        iterateCheckBool(nodep, "For Test Condition", nodep->condp(), BOTH);
        userIterateAndNext(nodep->incsp(), nullptr);
    }
    শূন্য visit(AstGenIf* nodep) override {
        assertAtStatement(nodep);
        iterateCheckBool(nodep, "If", nodep->condp(), BOTH);
    }

    শূন্য visit(AstCase* nodep) override {
        assertAtStatement(nodep);
        // Type check expression case item conditions and bodies
        userIterateAndNext(nodep->exprp(), WidthVP{CONTEXT_DET, PRELIM}.p());
        বাবে (AstCaseItem *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            nextip = VN_AS(itemp->nextp(), CaseItem);  // Prelim may cause the node to get replaced
            userIterateAndNext(itemp->stmtsp(), nullptr);
            বাবে (Ast_গ্ৰন্থি *nextcp, *condp = itemp->condsp(); condp; condp = nextcp) {
                nextcp = condp->nextp();  // Prelim may cause the node to get replaced
                VL_DO_DANGLING(userIterate(condp, WidthVP{CONTEXT_DET, PRELIM}.p()), condp);
            }
        }
        // Deal with case(type(data_type))
        handleCaseType(nodep, nodep->exprp(), nodep->itemsp());
        // Type check
        handleCase(nodep, nodep->exprp(), nodep->itemsp());
    }
    শূন্য visit(AstRandCase* nodep) override {
        // IEEE says each item is a int (32-bits), and sizes are based on natural sizing,
        // but we'll sum to a 64-bit number then math is faster.
        assertAtStatement(nodep);
        Ast_গ্ৰন্থি_DType* ধ্রুৱ itemDTypep = nodep->findUInt32DType();
        বাবে (AstCaseItem *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            nextip = VN_AS(itemp->nextp(), CaseItem);  // Prelim may cause the node to get replaced
            userIterateAndNext(itemp->stmtsp(), nullptr);
            বাবে (Ast_গ্ৰন্থি *nextcp, *condp = itemp->condsp(); condp; condp = nextcp) {
                nextcp = condp->nextp();  // Prelim may cause the node to get replaced
                iterateCheckTyped(itemp, "Randcase Item", condp, itemDTypep, BOTH);
                VL_DANGLING(condp);  // Might have been replaced
            }
            VL_DANGLING(itemp);  // Might have been replaced
        }
    }

    শূন্য visit(AstLoop* nodep) override {
        UASSERT_OBJ(!nodep->contsp(), nodep, "'contsp' only used before LinkJump");
        assertAtStatement(nodep);
        userIterateAndNext(nodep->stmtsp(), nullptr);
    }
    শূন্য visit(AstLoopTest* nodep) override {
        assertAtStatement(nodep);
        iterateCheckBool(nodep, "Loop Condition", nodep->condp(), BOTH);
    }
    শূন্য visit(AstRepeat* nodep) override { nodep->v3fatalSrc("'repeat' missed in LinkJump"); }
    শূন্য visit(Ast_গ্ৰন্থি_If* nodep) override {
        assertAtStatement(nodep);
        // UINFOTREE(1, nodep, "", "IfPre");
        userIterateAndNext(nodep->thensp(), nullptr);
        userIterateAndNext(nodep->elsesp(), nullptr);
        iterateCheckBool(nodep, "If", nodep->condp(), BOTH);  // it's like an if() condition.
        // UINFOTREE(1, nodep, "", "IfOut");
    }
    শূন্য visit(Ast_ৰাশিমালা_Stmt* nodep) override {
        userIterateAndNext(nodep->stmtsp(), nullptr);
        // expected result is same as parent's expected result
        userIterateAndNext(nodep->resultp(), m_vup);
        nodep->dtypeFrom(nodep->resultp());
    }
    শূন্য visit(Ast_গ্ৰন্থি_Foreach* nodep) override {
        যদি (nodep->didWidth()) ঘূৰাই;
        nodep->didWidth(সঁচা);
        ধ্রুৱ AstForeachHeader* ধ্রুৱ headerp = nodep->headerp();
        // UINFOTREE(1, nodep, "", "foreach-old");
        userIterateAndNext(headerp->fromp(), WidthVP{SELF, BOTH}.p());
        Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ fromp = headerp->fromp();
        UASSERT_OBJ(fromp->dtypep(), fromp, "Missing data type");
        Ast_গ্ৰন্থি_DType* fromDtp = fromp->dtypep()->skipRefp();
        // Major dimension first
        বাবে (Ast_গ্ৰন্থি *argsp = headerp->elementsp(), *next_argsp; argsp; argsp = next_argsp) {
            next_argsp = argsp->nextp();
            ধ্রুৱ তার্কিক empty = VN_IS(argsp, Empty);
            AstVar* ধ্রুৱ varp = VN_CAST(argsp, Var);
            UASSERT_OBJ(varp || empty, argsp, "Missing foreach loop variable");
            যদি (varp) varp->usedLoopIdx(সঁচা);
            যদি (!fromDtp) {
                argsp->v3error("foreach loop variables exceed number of indices of array");
                VL_DO_DANGLING(nodep->unlinkFrBack()->delete_বৃক্ষ(), nodep);
                ঘূৰাই;
            }
            fromDtp = fromDtp->skipRefp();
            UINFO(9, "- foreachArg " << argsp);
            UINFO(9, "-   from on  " << fromp);
            UINFO(9, "-   from dtp " << fromDtp);

            যদি (VN_IS(fromDtp, গ্ৰন্থি_Array_DType) || VN_IS(fromDtp, DynArrayDType)
                || VN_IS(fromDtp, QueueDType)) {
                // Nothing special here
            } অন্যথা যদি (AstBasicDType* ধ্রুৱ adtypep = VN_CAST(fromDtp, BasicDType)) {
                যদি (!adtypep->isString() && !adtypep->isRanged()) {
                    argsp->v3error("Illegal 'foreach' loop on " << fromDtp->prettyTypeName()
                                                                << " data type");
                    VL_DO_DANGLING(nodep->unlinkFrBack()->delete_বৃক্ষ(), nodep);
                    ঘূৰাই;
                }
            } অন্যথা যদি (ধ্রুৱ AstAssocArrayDType* ধ্রুৱ adtypep
                       = VN_CAST(fromDtp, AssocArrayDType)) {
                varp->dtypeFrom(adtypep->চাবি_DTypep());
            } অন্যথা {
                argsp->v3error("Illegal 'foreach' loop on " << fromDtp->prettyTypeName()
                                                            << " data type");
                VL_DO_DANGLING(nodep->unlinkFrBack()->delete_বৃক্ষ(), nodep);
                ঘূৰাই;
            }
            fromDtp = fromDtp->subDTypep();
        }
        // The parser validates we don't have "foreach (array[,,,])"
        Ast_গ্ৰন্থি* ধ্রুৱ bodyp = nodep->bodyp();
        userIterateAndNext(bodyp, nullptr);
        যদি (AstForeach* ধ্রুৱ loopp = VN_CAST(nodep, Foreach)) {
            VL_DO_DANGLING2(V3Begin::convertToWhile(loopp), loopp, nodep);
            ঘূৰাই;
        }
    }

    শূন্য visit(Ast_গ্ৰন্থি_Assign* nodep) override {
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
            যদি (ধ্রুৱ AstIfaceRefDType* ধ্রুৱ irefp
                = VN_CAST(nodep->lhsp()->dtypep()->elemDTypep(), IfaceRefDType)) {
                যদি (!irefp->isVirtual()) {
                    nodep->v3error(
                        "Illegal assignment to an interface instance (IEEE 1800-2023 10.4)");
                    VL_DO_DANGLING(ঠেলো_Deletep(nodep->unlinkFrBack()), nodep);
                    ঘূৰাই;
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
            Ast_গ্ৰন্থি_DType* ধ্রুৱ lhsDTypep = nodep->lhsp()->dtypep();

            // IEEE 1800-2023 7.6: For unpacked arrays to be assignment compatible,
            // the element types shall be equivalent (IEEE 1800-2023 6.22.2).
            // Note: Streaming operators and string literals have implicit conversion rules.
            যদি (nodep->rhsp()->dtypep()) {  // May be null on earlier errors
                ধ্রুৱ Ast_গ্ৰন্থি_DType* ধ্রুৱ lhsDtp = lhsDTypep->skipRefp();
                ধ্রুৱ Ast_গ্ৰন্থি_DType* ধ্রুৱ rhsDtp = nodep->rhsp()->dtypep()->skipRefp();
                // Only check if number of states match for unpacked array to unpacked array
                // assignments
                ধ্রুৱ তার্কিক lhsIsUnpackArray = lhsDtp->isNonPackedArray();
                ধ্রুৱ তার্কিক rhsIsUnpackArray = rhsDtp->isNonPackedArray();
                যদি (lhsIsUnpackArray && rhsIsUnpackArray) {
                    যদি (lhsDtp->isFourstate() != rhsDtp->isFourstate()) {
                        nodep->v3error("Assignment between 2-state and 4-state types requires "
                                       "equivalent element types (IEEE 1800-2023 6.22.2, 7.6)"
                                       << warnDTypes(nodep, lhsDtp, rhsDtp));
                    }
                }
                checkUnpackedArrayAssignmentCompatible<Ast_গ্ৰন্থি_Var_Ref, Ast_গ্ৰন্থি_Var_Ref>(
                    nodep, VN_CAST(nodep->lhsp(), গ্ৰন্থি_Var_Ref), VN_CAST(nodep->rhsp(), গ্ৰন্থি_Var_Ref));
                // Variable references were checked above, this catches e.g. method call results
                যদি (lhsDtp->isIntegralOrPacked() && rhsDtp->isAggregateType()
                    && !VN_IS(nodep->rhsp(), গ্ৰন্থি_Var_Ref)) {
                    nodep->v3error("Illegal assignment betwen unpacked aggregate and "
                                   "integral type (IEEE 1800-2023 7.6)"
                                   << warnDTypes(nodep, lhsDtp, rhsDtp));
                }
            }

            iterateCheckAssign(nodep, "Assign RHS", nodep->rhsp(), FINAL, lhsDTypep);
            // UINFOTREE(1, nodep, "", "AssignOut");
        }
        যদি (VN_IS(nodep, AssignForce)) checkForceReleaseLhs(nodep, nodep->lhsp());
        যদি (Ast_গ্ৰন্থি* ধ্রুৱ controlp = nodep->timingControlp()) {
            যদি (!m_underFork && VN_IS(m_ftaskp, Func)) {
                controlp->v3error("Timing controls are not legal in functions. Suggest use a task "
                                  "(IEEE 1800-2023 13.4.4)");
                VL_DO_DANGLING(controlp->unlinkFrBackWithNext()->delete_বৃক্ষ(), controlp);
            } অন্যথা যদি (nodep->fileline()->timingOn() && v3Global.opt.timing().isSetTrue()) {
                iterateNull(controlp);
            } অন্যথা {
                যদি (nodep->fileline()->timingOn()) {
                    যদি (v3Global.opt.timing().isSetFalse()) {
                        controlp->v3warn(ASSIGNDLY, "Ignoring timing control on this "
                                                    "assignment/primitive due to --no-timing");
                    } অন্যথা {
                        controlp->v3warn(E_NEEDTIMINGOPT,
                                         "Use --timing or --no-timing to specify how "
                                         "timing controls should be handled");
                    }
                }
                VL_DO_DANGLING(controlp->unlinkFrBackWithNext()->delete_বৃক্ষ(), controlp);
            }
        }
        যদি (VN_IS(nodep->rhsp(), EmptyQueue)) {
            UINFO(9, "= {} -> .delete(): " << nodep);
            ধ্রুৱ Ast_গ্ৰন্থি_DType* ধ্রুৱ lhsDtp = nodep->lhsp()->dtypep()->skipRefp();
            যদি (!VN_IS(lhsDtp, QueueDType) && !VN_IS(lhsDtp, DynArrayDType)) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported/Illegal: empty queue ('{}') in this assign context");
                VL_DO_DANGLING(ঠেলো_Deletep(nodep->unlinkFrBack()), nodep);
                ঘূৰাই;
            }
            AstMethodCall* ধ্রুৱ newp
                = নূতন AstMethodCall{nodep->fileline(), nodep->lhsp()->unlinkFrBack(), "delete"};
            newp->dtypeSetVoid();
            nodep->replaceWith(newp->makeStmt());
            VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
            // Need to now convert it
            visit(newp);
            ঘূৰাই;
        }
        যদি (ধ্রুৱ AstNewDynamic* ধ্রুৱ dynp = VN_CAST(nodep->rhsp(), NewDynamic)) {
            UINFO(9, "= new[] -> .resize(): " << nodep);
            AstCMethodHard* newp;
            যদি (!dynp->rhsp()) {
                newp = নূতন AstCMethodHard{nodep->fileline(), nodep->lhsp()->unlinkFrBack(),
                                          VCMethod::DYN_RENEW, dynp->sizep()->unlinkFrBack()};
            } অন্যথা {
                newp = নূতন AstCMethodHard{nodep->fileline(), nodep->lhsp()->unlinkFrBack(),
                                          VCMethod::DYN_RENEW_COPY, dynp->sizep()->unlinkFrBack()};
                newp->addPinsp(dynp->rhsp()->unlinkFrBack());
            }
            newp->didWidth(সঁচা);
            newp->protect(মিছা);
            newp->dtypeSetVoid();
            nodep->replaceWith(newp->makeStmt());
            VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
            ঘূৰাই;
        }

        // Queue slice on LHS: q[a:b] = rhs  ->  q.sliceAssign(a, b, rhs)
        // The LHS was lowered from AstSelExtract to CMethodHard(DYN_SLICE)
        // by V3WidthSel; that returns a temporary copy so the assignment is
        // silently discarded.  Transform into a mutating sliceAssign call.
        যদি (AstCMethodHard* ধ্রুৱ slicep = VN_CAST(nodep->lhsp(), CMethodHard)) {
            VCMethod assignMethod;
            যদি (slicep->method() == VCMethod::DYN_SLICE) {
                assignMethod = VCMethod::DYN_SLICE_ASSIGN;
            } অন্যথা যদি (slicep->method() == VCMethod::DYN_SLICE_BACK_BACK) {
                assignMethod = VCMethod::DYN_SLICE_ASSIGN_BACK_BACK;
            } অন্যথা যদি (slicep->method() == VCMethod::DYN_SLICE_FRONT_BACK) {
                assignMethod = VCMethod::DYN_SLICE_ASSIGN_FRONT_BACK;
            } অন্যথা {
                assignMethod = VCMethod::_ENUM_MAX;  // not a slice
            }
            যদি (assignMethod != VCMethod::_ENUM_MAX) {
                UINFO(9, "LHS queue slice -> sliceAssign: " << nodep);
                Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ fromp = slicep->fromp()->unlinkFrBack();
                // Collect existing slice index pins (lsb, msb)
                Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ lsbp = slicep->pinsp()->unlinkFrBack();
                Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ msbp = slicep->pinsp()->unlinkFrBack();
                Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ rhsp = nodep->rhsp()->unlinkFrBack();
                AstCMethodHard* ধ্রুৱ newp
                    = নূতন AstCMethodHard{nodep->fileline(), fromp, assignMethod};
                newp->addPinsp(lsbp);
                newp->addPinsp(msbp);
                newp->addPinsp(rhsp);
                newp->didWidth(সঁচা);
                newp->protect(মিছা);
                newp->dtypeSetVoid();
                nodep->replaceWith(newp->makeStmt());
                VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
                ঘূৰাই;
            }
        }

        যদি (nodep->hasDType() && nodep->dtypep()->isEvent()) {
            checkEventAssignment(nodep);
            v3Global.setAssignsEvents();
        }
    }

    শূন্য visit(AstRSRule* nodep) override {
        যদি (nodep->weightp()) iterateCheckUInt32(nodep, "weight", nodep->weightp(), BOTH);
        userIterateAndNext(nodep->prodlistsp(), nullptr);
        userIterateAndNext(nodep->weightStmtsp(), nullptr);
    }

    শূন্য visit(AstRelease* nodep) override {
        userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
        UASSERT_OBJ(nodep->lhsp()->dtypep(), nodep, "How can LValue be untyped?");
        UASSERT_OBJ(nodep->lhsp()->dtypep()->widthSized(), nodep, "How can LValue be unsized?");
        checkForceReleaseLhs(nodep, nodep->lhsp());
    }
    শূন্য visit(AstDeassign* nodep) override {
        userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
        UASSERT_OBJ(nodep->lhsp()->dtypep(), nodep, "L-value is untyped");
        UASSERT_OBJ(nodep->lhsp()->dtypep()->widthSized(), nodep, "L-value width is unsized");
        checkForceReleaseLhs(nodep, nodep->lhsp());
    }

    স্থির তার্কিক isFormatNonNumericArg(ধ্রুৱ Ast_গ্ৰন্থি_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        ঘূৰাই dtypep->isString()  //
               || VN_IS(dtypep, AssocArrayDType)  //
               || VN_IS(dtypep, WildcardArrayDType)  //
               || VN_IS(dtypep, ClassRefDType)  //
               || VN_IS(dtypep, DynArrayDType)  //
               || VN_IS(dtypep, UnpackArrayDType)  //
               || VN_IS(dtypep, QueueDType)  //
               || (VN_IS(dtypep, গ্ৰন্থি_UOr_Struct_DType)
                   && !VN_AS(dtypep, গ্ৰন্থি_UOr_Struct_DType)->packed());
    }

    শূন্য checkFormatNumericArg(Ast_গ্ৰন্থি* argp, ৱর্ণ ch) {
        // IEEE 1800-2023 21.2.1.1 suggests %d of a class is illegal.
        // Howver UVM tests require %0d print unique identifier of the class.
        যদি (std::tolower(ch) == 'd') ঘূৰাই;
        // In contrast, %x of a class reference, causes errors on several simulators.
        যদি (isFormatNonNumericArg(argp->dtypep()))
            argp->v3error(
                "$display-like format of '%"s + ch + "' illegal with non-numeric argument\n"
                << argp->warnMore() << "... Suggest use '%s'");
    }

    শূন্য visit(AstSFormatArg* nodep) override {}  // Done by AstSFormatF
    শূন্য visit(AstSFormat* nodep) override {
        assertAtStatement(nodep);
        userIterateAndNext(nodep->fmtp(), WidthVP{SELF, BOTH}.p());
        userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
    }
    শূন্য visit(AstToStringN* nodep) override {
        // Inserted by V3Width only so we know has been resolved
        nodep->dtypeSetString();
        userIterateAndNext(nodep->lhsp(), WidthVP{nodep->lhsp()->dtypep(), BOTH}.p());
    }
    শূন্য visit(AstSFormatF* nodep) override {
        // Excludes NodeDisplay, see below
        যদি (m_vup && !m_vup->prelim()) ঘূৰাই;  // Can be called as statement or function
        যদি (nodep->didWidthAndSet()) ঘূৰাই;
        //
        UINFO(9, "  Display in " << nodep->text());

        // Just let all arguments seek their natural sizes
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
        যদি (!nodep->ৰাশিমালা_Format_2()) {  // Non-constant format, can't check format string types
            visit_sformatf_format(nodep);
        }

        Ast_গ্ৰন্থি_ৰাশিমালা* oldExprsp = nodep->exprsp();
        যদি (nodep->ৰাশিমালা_Format_2() && oldExprsp) {
            VL_DO_DANGLING(iterateCheckString(nodep, "format", nodep->exprsp(), BOTH), oldExprsp);
            oldExprsp = VN_AS(nodep->exprsp()->nextp(), গ্ৰন্থি_ৰাশিমালা);
        }
        যদি (oldExprsp) oldExprsp->unlinkFrBackWithNext();
        যেতিয়ালৈ (Ast_গ্ৰন্থি_ৰাশিমালা* argp = oldExprsp) {
            oldExprsp = VN_AS(oldExprsp->nextp(), গ্ৰন্থি_ৰাশিমালা);
            যদি (oldExprsp) oldExprsp->unlinkFrBackWithNext();
            যদি (VN_IS(argp, SFormatArg)) {
                nodep->addExprsp(argp);
                অনুৱর্ত;
            }
            // Need to record formatAttr's at elaboration time, as later optimizations
            // may change an argument's data type. Plus need them for runtime formats
            VFormatAttr formatAttr = VFormatAttr::UNSIGNED;
            Ast_গ্ৰন্থি_DType* ধ্রুৱ dtypep = argp->dtypep()->skipRefp();
            যদি (dtypep->isDouble()) {
                formatAttr = VFormatAttr::DOUBLE;
            } অন্যথা যদি (dtypep->isString()) {
                formatAttr = VFormatAttr::STRING;
            } অন্যথা যদি (isFormatNonNumericArg(dtypep)) {
                যদি (AstClassRefDType* ধ্রুৱ classRefp = VN_CAST(dtypep, ClassRefDType)) {
                    যদি (classRefp->classp()) {
                        classRefp->classp()->markPrintedFrom();
                        v3Global.hasPrintedObjects(সঁচা);
                    }
                } অন্যথা {
                    // Class handles inside containers print type names, not their members.
                    Ast_গ্ৰন্থি_DType* গ্ৰন্থি_Dtypep = dtypep;
                    যেতিয়ালৈ (গ্ৰন্থি_Dtypep) {
                        গ্ৰন্থি_Dtypep = গ্ৰন্থি_Dtypep->skipRefp();
                        যদি (Ast_গ্ৰন্থি_UOr_Struct_DType* ধ্রুৱ uOrStructDTypep
                            = VN_CAST(গ্ৰন্থি_Dtypep, গ্ৰন্থি_UOr_Struct_DType)) {
                            uOrStructDTypep->setEmitToString();
                            v3Global.hasPrintedObjects(সঁচা);
                        }
                        গ্ৰন্থি_Dtypep = গ্ৰন্থি_Dtypep->subDTypep();
                    }
                }
                Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ newp = নূতন AstToStringN{argp->fileline(), argp};
                formatAttr = VFormatAttr::COMPLEX;
                argp = newp;
            } অন্যথা যদি (nodep->ৰাশিমালা_Format_2()) {
                যদি (AstEnumDType* ধ্রুৱ enumDtp = formatEnumDType(argp)) {
                    nodep->addExprsp(newFormatEnumArg(argp, enumDtp));
                    অনুৱর্ত;
                }
            }
            যদি (formatAttr.isUnsigned() && dtypep->isSigned()) {
                formatAttr = VFormatAttr::SIGNED;
            }
            যদি (formatAttr.isUnsigned()) {  // Save Ast space and imply the AstSFormatArg
                nodep->addExprsp(argp);
            } অন্যথা {
                nodep->addExprsp(নূতন AstSFormatArg{argp->fileline(), formatAttr, argp});
            }
        }

        UINFO(9, "  Display out " << nodep->text());
    }
    শূন্য visit(AstCReset* nodep) override {
        assert_At_ৰাশিমালা(nodep);
        nodep->dtypeFrom(m_vup->dtypep());
    }
    শূন্য visit(AstCReturn* nodep) override { nodep->v3fatalSrc("Should not exist yet"); }
    শূন্য visit(AstConstraintRef* nodep) override { userIterateChildren(nodep, nullptr); }
    শূন্য visit(AstDisplay* nodep) override {
        assertAtStatement(nodep);
        যদি (nodep->filep()) iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
        // Just let all arguments seek their natural sizes
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
    }
    শূন্য visit(AstElabDisplay* nodep) override {
        assertAtStatement(nodep);
        // Just let all arguments seek their natural sizes
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
        যদি (!m_paramsOnly) {
            nodep->foreach([এতত্](AstScopeName* nodep) {  //
                nodep->replaceWith(
                    নূতন AstConst{nodep->fileline(), AstConst::String{}, "<scope-unavailable>"});
                VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
            });
            V3Const::constifyParamsEdit(nodep->fmtp());  // fmtp may change
            string text = VString::dequotePercent(nodep->fmtp()->text());
            যদি (text.empty()) text = "Elaboration system task message (IEEE 1800-2023 20.11)";
            ৱিকল্প (nodep->displayType()) {
            পক্ষ VDisplayType::DT_INFO: nodep->v3warn(USERINFO, text); ভঙ্গ;
            পক্ষ VDisplayType::DT_ERROR: nodep->v3warn(USERERROR, text); ভঙ্গ;
            পক্ষ VDisplayType::DT_WARNING: nodep->v3warn(USERWARN, text); ভঙ্গ;
            পক্ষ VDisplayType::DT_FATAL: nodep->v3warn(USERFATAL, text); ভঙ্গ;
            স্ৱভাৱ: UASSERT_OBJ(মিছা, nodep, "Unexpected elaboration display type");
            }
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_বৃক্ষ(), nodep);
        }
    }
    শূন্য visit(AstDumpCtl* nodep) override {
        assertAtStatement(nodep);
        যদি (nodep->exprp()) iterateCheckString(nodep, "LHS", nodep->exprp(), BOTH);
    }
    শূন্য visit(AstFOpen* nodep) override {
        // Although a system function in IEEE, here a statement which sets the file pointer (MCD)
        iterateCheckString(nodep, "filename", nodep->filenamep(), BOTH);
        iterateCheckString(nodep, "mode", nodep->modep(), BOTH);
    }
    শূন্য visit(AstFOpenMcd* nodep) override {
        iterateCheckString(nodep, "filename", nodep->filenamep(), BOTH);
    }
    শূন্য visit(AstFClose* nodep) override {
        assertAtStatement(nodep);
        iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
    }
    শূন্য visit(AstFError* nodep) override {
        assert_At_ৰাশিমালা(nodep);
        যদি (m_vup->prelim()) {
            iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
            // Could be string or packed array
            userIterateAndNext(nodep->strp(), WidthVP{SELF, BOTH}.p());
        }
    }
    শূন্য visit(AstFEof* nodep) override {
        assert_At_ৰাশিমালা(nodep);
        যদি (m_vup->prelim()) { iterateCheckFileDesc(nodep, nodep->filep(), BOTH); }
    }
    শূন্য visit(AstFFlush* nodep) override {
        assertAtStatement(nodep);
        যদি (nodep->filep()) iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
    }
    শূন্য visit(AstFRewind* nodep) override { iterateCheckFileDesc(nodep, nodep->filep(), BOTH); }
    শূন্য visit(AstFTell* nodep) override { iterateCheckFileDesc(nodep, nodep->filep(), BOTH); }
    শূন্য visit(AstFSeek* nodep) override {
        iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
        iterateCheckSigned32(nodep, "$fseek offset", nodep->offset(), BOTH);
        iterateCheckSigned32(nodep, "$fseek operation", nodep->operation(), BOTH);
    }
    শূন্য visit(AstFGetC* nodep) override {
        assert_At_ৰাশিমালা(nodep);
        যদি (m_vup->prelim()) iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
    }
    শূন্য visit(AstFGetS* nodep) override {
        assert_At_ৰাশিমালা(nodep);
        যদি (m_vup->prelim()) {
            iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
            userIterateAndNext(nodep->strgp(), WidthVP{SELF, BOTH}.p());
        }
    }
    শূন্য visit(AstFUngetC* nodep) override {
        assert_At_ৰাশিমালা(nodep);
        যদি (m_vup->prelim()) {
            iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
            iterateCheckSigned32(nodep, "$fungetc character", nodep->charp(), BOTH);
        }
    }
    শূন্য visit(AstFRead* nodep) override {
        assert_At_ৰাশিমালা(nodep);
        যদি (m_vup->prelim()) {
            userIterateAndNext(nodep->memp(), WidthVP{SELF, BOTH}.p());
            iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
            যদি (nodep->startp()) {
                iterateCheckSigned32(nodep, "$fread start", nodep->startp(), BOTH);
            }
            যদি (nodep->countp()) {
                iterateCheckSigned32(nodep, "$fread count", nodep->countp(), BOTH);
            }
        }
    }
    শূন্য visit(AstFScanF* nodep) override {
        assert_At_ৰাশিমালা(nodep);
        যদি (m_vup->prelim()) {
            iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
            userIterateAndNext(nodep->exprsp(), WidthVP{SELF, BOTH}.p());
        }
    }
    শূন্য visit(AstSScanF* nodep) override {
        assert_At_ৰাশিমালা(nodep);
        যদি (m_vup->prelim()) {
            userIterateAndNext(nodep->fromp(), WidthVP{SELF, BOTH}.p());
            userIterateAndNext(nodep->exprsp(), WidthVP{SELF, BOTH}.p());
        }
    }
    শূন্য visit(Ast_স্তূপ_Trace_F* nodep) override { nodep->dtypeSetString(); }
    শূন্য visit(AstSysIgnore* nodep) override {
        userIterateAndNext(nodep->exprsp(), WidthVP{SELF, BOTH}.p());
    }
    শূন্য visit(AstSystemF* nodep) override {
        assert_At_ৰাশিমালা(nodep);
        যদি (m_vup->prelim()) userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
    }
    শূন্য visit(AstSystemT* nodep) override {
        assertAtStatement(nodep);
        userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
    }
    শূন্য visit(Ast_গ্ৰন্থি_Read_Write_Mem* nodep) override {
        assertAtStatement(nodep);
        iterateCheckString(nodep, "filename", nodep->filenamep(), BOTH);
        userIterateAndNext(nodep->memp(), WidthVP{SELF, BOTH}.p());
        ধ্রুৱ Ast_গ্ৰন্থি_DType* subp = nullptr;
        যদি (ধ্রুৱ AstAssocArrayDType* adtypep
            = VN_CAST(nodep->memp()->dtypep()->skipRefp(), AssocArrayDType)) {
            subp = adtypep->subDTypep();
            যদি (!adtypep->চাবি_DTypep()->skipRefp()->basicp()
                || !adtypep->চাবি_DTypep()->skipRefp()->basicp()->keyword().isIntNumeric()) {
                nodep->memp()->v3error(nodep->verilogKwd()
                                       << " address/key must be integral (IEEE 1800-2023 21.4.1)");
            }
        } অন্যথা যদি (ধ্রুৱ AstUnpackArrayDType* ধ্রুৱ adtypep
                   = VN_CAST(nodep->memp()->dtypep()->skipRefp(), UnpackArrayDType)) {
            subp = adtypep->subDTypep();
        } অন্যথা যদি (ধ্রুৱ AstDynArrayDType* ধ্রুৱ adtypep
                   = VN_CAST(nodep->memp()->dtypep()->skipRefp(), DynArrayDType)) {
            subp = adtypep->subDTypep();
        } অন্যথা {
            nodep->memp()->v3warn(
                E_UNSUPPORTED,
                "Unsupported: " << nodep->verilogKwd()
                                << " into other than unpacked, dynamic, or associative array");
        }
        যদি (subp
            && (!subp->skipRefp()->basicp()
                || !subp->skipRefp()->basicp()->keyword().isIntNumeric())) {
            nodep->memp()->v3warn(E_UNSUPPORTED,
                                  "Unsupported: " << nodep->verilogKwd()
                                                  << " array values must be integral");
        }
        userIterateAndNext(nodep->lsbp(), WidthVP{SELF, BOTH}.p());
        userIterateAndNext(nodep->msbp(), WidthVP{SELF, BOTH}.p());
    }
    শূন্য visit(AstTestPlusArgs* nodep) override {
        assert_At_ৰাশিমালা(nodep);
        যদি (m_vup->prelim()) iterateCheckString(nodep, "LHS", nodep->searchp(), BOTH);
    }
    শূন্য visit(Ast_মান_Plus_Args* nodep) override {
        assert_At_ৰাশিমালা(nodep);
        যদি (m_vup->prelim()) {
            iterateCheckString(nodep, "LHS", nodep->searchp(), BOTH);
            userIterateAndNext(nodep->outp(), WidthVP{SELF, BOTH}.p());
        }
    }
    শূন্য visit(AstTimeFormat* nodep) override {
        assertAtStatement(nodep);
        যদি (nodep->unitsp()) iterateCheckSigned32(nodep, "units", nodep->unitsp(), BOTH);
        যদি (nodep->precisionp())
            iterateCheckSigned32(nodep, "precision", nodep->precisionp(), BOTH);
        যদি (nodep->suffixp()) iterateCheckString(nodep, "suffix", nodep->suffixp(), BOTH);
        যদি (nodep->widthp()) iterateCheckSigned32(nodep, "width", nodep->widthp(), BOTH);
    }
    শূন্য visit(AstCStmtUser* nodep) override {
        // Just let all arguments seek their natural sizes
        assertAtStatement(nodep);
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
    }
    শূন্য visit(AstAssert* nodep) override {
        assertAtStatement(nodep);
        iterateCheckBool(nodep, "Property", nodep->propp(), BOTH);  // it's like an if() condition.
        userIterateAndNext(nodep->passsp(), nullptr);
        userIterateAndNext(nodep->failsp(), nullptr);
    }
    শূন্য visit(AstAssertCtl* nodep) override {
        assertAtStatement(nodep);
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
    }
    শূন্য visit(AstAssertIntrinsic* nodep) override {
        assertAtStatement(nodep);
        iterateCheckBool(nodep, "Property", nodep->propp(), BOTH);  // it's like an if() condition.
        userIterateAndNext(nodep->passsp(), nullptr);
        userIterateAndNext(nodep->failsp(), nullptr);
    }
    শূন্য visit(AstCover* nodep) override {
        assertAtStatement(nodep);
        iterateCheckBool(nodep, "Property", nodep->propp(), BOTH);  // it's like an if() condition.
        userIterateAndNext(nodep->passsp(), nullptr);
    }
    শূন্য visit(AstRestrict* nodep) override {
        assertAtStatement(nodep);
        iterateCheckBool(nodep, "Property", nodep->propp(), BOTH);  // it's like an if() condition.
    }

    // Select the part of a pin connection that belongs to an element of an instance array,
    // one dimension at a time, starting with 'rangep'. 'idx' is the row-major position of the
    // element in the dimensions from 'rangep' inwards, each counted from the left. The
    // connection has had its PRELIM visit. Returns true if the pin needs nothing else (the
    // selected connection matches the port, or there was an error), false if the pin still
    // needs the usual checks.
    তার্কিক pinInstArrayElement(AstPin* nodep, ধ্রুৱ AstRange* rangep, পূর্ণাঙ্ক idx) {
        Ast_গ্ৰন্থি_DType* ধ্রুৱ modDTypep = nodep->modVarp()->dtypep()->skipRefp();
        Ast_গ্ৰন্থি_DType* ধ্রুৱ conDTypep = nodep->exprp()->dtypep()->skipRefp();

        // If types match, then nothing else to do
        যদি (conDTypep == modDTypep || similarDTypeRecurse(conDTypep, modDTypep)) ঘূৰাই সঁচা;
        // Types don't match, but all dimensions done, still need the usual checks
        যদি (!rangep) ঘূৰাই মিছা;

        ধ্রুৱ AstRange* ধ্রুৱ subRangep = VN_AS(rangep->nextp(), Range);

        // Number of dimensions from 'rangep' inwards, and of elements in those after 'rangep'
        uint32_t nDims = 1;
        পূর্ণাঙ্ক subElems = 1;
        বাবে (ধ্রুৱ AstRange* rp = subRangep; rp; rp = VN_AS(rp->nextp(), Range)) {
            ++nDims;
            subElems *= rp->elementsConst();
        }
        ধ্রুৱ পূর্ণাঙ্ক elements = rangep->elementsConst();
        ধ্রুৱ পূর্ণাঙ্ক from_বাম = idx / subElems;

        ধ্রুৱ AstUnpackArrayDType* ধ্রুৱ arrp = VN_CAST(conDTypep, UnpackArrayDType);
        ধ্রুৱ uint32_t conDims = conDTypep->dimensions(মিছা).second;
        ধ্রুৱ uint32_t modDims = modDTypep->dimensions(মিছা).second;

        // Unpacked array with other than the port's number of unpacked dimensions: its slowest
        // varying unpacked dimensions must match the instance array dimensions exactly in size,
        // and the rest be those of the port. Connect its element at the same position, counted
        // from the left. The inner dimensions then select from that element the same way.
        যদি (arrp && conDims != modDims) {
            যদি (conDims != nDims + modDims || arrp->elementsConst() != elements) {
                nodep->v3error(ucfirst(nodep->prettyOperatorName())
                               << " as part of a module instance array requires the connection's"
                                  " leading unpacked dimensions to match the instance array"
                                  " dimensions (IEEE 1800-2023 23.3.3.5)");
                userIterateAndNext(nodep->exprp(),
                                   WidthVP{conDTypep, FINAL, STREAM_USE_ASSIGN}.p());
                ঘূৰাই সঁচা;
            }
            // ArraySel index is counted from lo
            ধ্রুৱ পূর্ণাঙ্ক bit = arrp->declRange().ascending() ? from_বাম : elements - 1 - from_বাম;
            // Connection is self-determined
            userIterateAndNext(nodep->exprp(), WidthVP{conDTypep, FINAL, STREAM_USE_ASSIGN}.p());
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ fromp = VN_AS(nodep->exprp(), গ্ৰন্থি_ৰাশিমালা)->unlinkFrBack();
            যদি (Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ elemp = newIfaceArrayElement(fromp, bit)) {
                nodep->exprp(elemp);
                VL_DO_DANGLING(ঠেলো_Deletep(fromp), fromp);
            } অন্যথা {
                AstArraySel* ধ্রুৱ selp = নূতন AstArraySel{fromp->fileline(), fromp, bit};
                selp->didWidth(সঁচা);
                nodep->exprp(selp);
            }
            ঘূৰাই pinInstArrayElement(nodep, subRangep, idx % subElems);
        }

        // Otherwise if not packed (including an unpacked array with the port's number of unpacked
        // dimensions), connect it to every element, if it is compatible with the port
        যদি (!conDTypep->isIntegralOrPacked() || !modDTypep->isIntegralOrPacked()) ঘূৰাই মিছা;

        // Packed connection
        ধ্রুৱ পূর্ণাঙ্ক modwidth = modDTypep->width();
        ধ্রুৱ পূর্ণাঙ্ক conwidth = conDTypep->width();

        // Connection has the width of the port, connect it to every element
        যদি (conwidth == modwidth) ঘূৰাই মিছা;

        // Connection has the width of all the elements, connect a slice to each element,
        // leftmost element to the leftmost (most significant) slice
        যদি (conwidth == modwidth * elements * subElems) {
            যদি (rangep->ascending()) {
                nodep->exprp()->v3warn(ASCRANGE, "Ascending instance range connecting to "
                                                 "vector: left < right of instance range: ["
                                                     << rangep->বাম_Const() << ":"
                                                     << rangep->ডান_Const() << "]");
            }
            ধ্রুৱ পূর্ণাঙ্ক selwidth = modwidth * subElems;
            // Connection is self-determined
            userIterateAndNext(nodep->exprp(), WidthVP{conDTypep, FINAL, STREAM_USE_ASSIGN}.p());
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ fromp = VN_AS(nodep->exprp(), গ্ৰন্থি_ৰাশিমালা)->unlinkFrBack();
            AstSel* ধ্রুৱ selp = নূতন AstSel{fromp->fileline(), fromp,
                                            selwidth * (elements - 1 - from_বাম), selwidth};
            selp->didWidth(সঁচা);
            nodep->exprp(selp);
            ঘূৰাই pinInstArrayElement(nodep, subRangep, idx % subElems);
        }

        // Otherwise it is an error, as it must connect to one element or to all of them
        nodep->v3error(ucfirst(nodep->prettyOperatorName())
                       << " as part of a module instance array" << " requires " << modwidth
                       << " or " << modwidth * elements * subElems << " bits, but connection's "
                       << nodep->exprp()->prettyTypeName() << " generates " << conwidth
                       << " bits. (IEEE 1800-2023 23.3.3)");
        userIterateAndNext(nodep->exprp(), WidthVP{conDTypep, FINAL, STREAM_USE_ASSIGN}.p());
        ঘূৰাই সঁচা;
    }

    শূন্য visit(AstPin* nodep) override {
        // UINFOTREE(1, nodep, "", "PinPre");
        // TOP LEVEL NODE
        যদি (nodep->modVarp() && nodep->modVarp()->isGParam()) {
            // An unpacked array concatenation is a pattern of its items (IEEE 1800-2023 10.10)
            AstReplicate* ধ্রুৱ concatp = VN_CAST(nodep->exprp(), Replicate);
            ধ্রুৱ AstConst* ধ্রুৱ countp = concatp ? VN_CAST(concatp->countp(), Const) : nullptr;
            ধ্রুৱ Ast_গ্ৰন্থি_DType* ধ্রুৱ dtypep = nodep->modVarp()->subDTypep()->skipRefOrNullp();
            যদি (countp && countp->toUInt() == 1
                && (VN_IS(dtypep, UnpackArrayDType) || VN_IS(dtypep, BracketArrayDType))) {
                AstPattern* ধ্রুৱ patternp = নূতন AstPattern{concatp->fileline(), nullptr};
                patConcatConvertRecurse(patternp, concatp->srcp());
                concatp->replaceWith(patternp);
                VL_DO_DANGLING(ঠেলো_Deletep(concatp), concatp);
            }
            // An override pattern is left for the specialized module to type
            VL_RESTORER(m_inParamOverride);
            m_inParamOverride = isOverridePattern(nodep->exprp());
            userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
        } অন্যথা যদি (!m_paramsOnly) {
            যদি (!nodep->modVarp()->didWidth()) {
                // Var hasn't been widthed, so make it so.
                userIterate(nodep->modVarp(), nullptr);
            }
            যদি (!nodep->exprp()) {  // No-connect
                যদি (nodep->modVarp()->isIfaceArraySplit()) expandIfaceArrayPin(nodep);
                ঘূৰাই;
            }
            // Very much like like an assignment, but which side is LH/RHS
            // depends on pin being a in/output/inout.
            ধ্রুৱ VDirection pinDirection = nodep->modVarp()->direction();
            যদি (VN_IS(nodep->exprp(), গ্ৰন্থি_Stream) && pinDirection.isInoutOrRef()) {
                nodep->exprp()->v3error("Streaming concatenation cannot be connected to '"
                                        << pinDirection.prettyName()
                                        << "' port (IEEE 1800-2023 11.4.14)");
            }
            userIterateAndNext(nodep->exprp(), WidthVP{nodep->modVarp()->dtypep(), PRELIM}.p());
            Ast_গ্ৰন্থি_DType* modDTypep = nodep->modVarp()->dtypep();
            Ast_গ্ৰন্থি_DType* conDTypep = nodep->exprp()->dtypep();
            UASSERT_OBJ(modDTypep, nodep, "Unlinked pin data type");
            UASSERT_OBJ(conDTypep, nodep, "Unlinked pin data type");
            modDTypep = modDTypep->skipRefp();
            conDTypep = conDTypep->skipRefp();
            // An interface port, or an array of them, needs interface instances, not virtual
            // interfaces
            যদি (ধ্রুৱ AstIfaceRefDType* ধ্রুৱ modIrefp
                = VN_CAST(modDTypep->elemDTypep(), IfaceRefDType)) {
                ধ্রুৱ AstIfaceRefDType* ধ্রুৱ conIrefp
                    = VN_CAST(conDTypep->elemDTypep(), IfaceRefDType);
                যদি (!modIrefp->isVirtual() && conIrefp && conIrefp->isVirtual()) {
                    nodep->v3error("Illegal " << nodep->prettyOperatorName()
                                              << ", interface port connected to a virtual"
                                                 " interface (IEEE 1800-2023 23.3.3.4)");
                }
            }
            Ast_গ্ৰন্থি_DType* subDTypep = modDTypep;
            ধ্রুৱ পূর্ণাঙ্ক modwidth = modDTypep->width();
            পূর্ণাঙ্ক conwidth = conDTypep->width();
            যদি (conDTypep == modDTypep  // If match, we're golden
                || similarDTypeRecurse(conDTypep, modDTypep)) {
                userIterateAndNext(nodep->exprp(),
                                   WidthVP{subDTypep, FINAL, STREAM_USE_ASSIGN}.p());
            } অন্যথা {
                যদি (m_cellp->arrayIdx() >= 0) {
                    // Element of an instance array, select its part of the connection,
                    // then check it like any other pin, unless that is all done already
                    যদি (pinInstArrayElement(nodep, m_cellp->rangep(), m_cellp->arrayIdx())) {
                        যদি (nodep->modVarp()->isIfaceArraySplit()) expandIfaceArrayPin(nodep);
                        ঘূৰাই;
                    }
                    conDTypep = nodep->exprp()->dtypep()->skipRefp();
                    conwidth = conDTypep->width();
                }
                যদি (nodep->modVarp()->direction() == VDirection::REF) {
                    nodep->v3error("Ref connection "
                                   << nodep->modVarp()->prettyNameQ()
                                   << " requires matching types;" << " ref requires "
                                   << modDTypep->prettyDTypeNameQ()
                                   << " data type but connection is "
                                   << conDTypep->prettyDTypeNameQ() << " data type.");
                } অন্যথা যদি (nodep->modVarp()->isTristate()) {
                    যদি (modwidth != conwidth) {
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
                } অন্যথা যদি (nodep->modVarp()->direction().isWritable()
                           && ((conDTypep->isDouble() && !modDTypep->isDouble())
                               || (!conDTypep->isDouble() && modDTypep->isDouble()))) {
                    nodep->v3warn(E_UNSUPPORTED,
                                  "Unsupported: " << ucfirst(nodep->prettyOperatorName())
                                                  << " connects real to non-real");
                }

                // Check if an interface is connected to a non-interface and vice versa
                যদি ((VN_IS(modDTypep, IfaceRefDType) && !VN_IS(conDTypep, IfaceRefDType))
                    || (VN_IS(conDTypep, IfaceRefDType) && !VN_IS(modDTypep, IfaceRefDType))) {
                    nodep->v3error("Illegal " << nodep->prettyOperatorName() << ","
                                              << " mismatch between port which is"
                                              << (VN_CAST(modDTypep, IfaceRefDType) ? "" : " not")
                                              << " an interface," << " and expression which is"
                                              << (VN_CAST(conDTypep, IfaceRefDType) ? "" : " not")
                                              << " an interface.");
                }

                // TODO Simple dtype checking, should be a more general check
                ধ্রুৱ Ast_গ্ৰন্থি_Array_DType* ধ্রুৱ ৰাশিমালা_Arrayp = VN_CAST(conDTypep, UnpackArrayDType);
                ধ্রুৱ Ast_গ্ৰন্থি_Array_DType* ধ্রুৱ modArrayp = VN_CAST(modDTypep, UnpackArrayDType);
                যদি (ৰাশিমালা_Arrayp && modArrayp && VN_IS(conDTypep->elemDTypep(), IfaceRefDType)
                    && unpackedArraySize(conDTypep) != unpackedArraySize(modDTypep)) {
                    nodep->v3error("Illegal "
                                   << nodep->prettyOperatorName() << ","
                                   << " mismatch between port which is an interface array of size "
                                   << unpackedArraySize(modDTypep) << ","
                                   << " and expression which is an interface array of size "
                                   << unpackedArraySize(conDTypep) << ".");
                    UINFO(1, "    Related lo: " << modDTypep);
                    UINFO(1, "    Related hi: " << conDTypep);
                } অন্যথা যদি ((ৰাশিমালা_Arrayp && !modArrayp) || (!ৰাশিমালা_Arrayp && modArrayp)) {
                    nodep->v3error(
                        "Illegal "
                        << nodep->prettyOperatorName() << "," << " mismatch between port which is"
                        << (modArrayp ? "" : " not") << " an array," << " and expression which is"
                        << (ৰাশিমালা_Arrayp ? "" : " not") << " an array. (IEEE 1800-2023 7.6)");
                    UINFO(1, "    Related lo: " << modDTypep);
                    UINFO(1, "    Related hi: " << conDTypep);
                } অন্যথা {
                    checkUnpackedArrayAssignmentCompatible<AstVar, Ast_গ্ৰন্থি_Var_Ref>(
                        nodep, nodep->modVarp(), VN_CAST(nodep->exprp(), গ্ৰন্থি_Var_Ref));
                    UINFO(1, "    Related lo: " << modDTypep);
                    UINFO(1, "    Related hi: " << conDTypep);
                }
                iterateCheckAssign(nodep, "pin connection", nodep->exprp(), FINAL, subDTypep,
                                   nodep->modVarp()->direction().pinAccess());
            }
            // Interface array port: connect its elements
            যদি (nodep->modVarp()->isIfaceArraySplit()) expandIfaceArrayPin(nodep);
        }
        // UINFOTREE(1, nodep, "", "PinOut");
    }
    শূন্য visit(AstCell* nodep) override {
        VL_RESTORER(m_cellp);
        m_cellp = nodep;
        যদি (!m_paramsOnly) {
            যদি (VN_IS(nodep->modp(), NotFoundModule)) {
                // We've resolved parameters and hit a module that we couldn't resolve.  It's
                // finally time to report it.
                // Note only here in V3Width as this is first visitor after V3Dead.
                nodep->modNameFileline()->v3warn(
                    MODMISSING, "Cannot find file containing module: '"
                                    << nodep->modName() << "'\n"
                                    << nodep->modNameFileline()->warnContextPrimary()
                                    << V3Error::warnAdditionalInfo()
                                    << v3Global.opt.file_Path_Looked_বাৰ্তা(nodep->modNameFileline(),
                                                                      nodep->modName()));
            }
            যদি (nodep->rangep()) userIterateAndNext(nodep->rangep(), WidthVP{SELF, BOTH}.p());
            userIterateAndNext(nodep->pinsp(), nullptr);
            // If element of an instance array, pins are now connected, no longer need the ranges
            যদি (nodep->arrayIdx() >= 0) {
                nodep->arrayIdx(-1);
                ঠেলো_Deletep(nodep->rangep()->unlinkFrBackWithNext());
            }
        }
        userIterateAndNext(nodep->paramsp(), nullptr);
    }
    শূন্য visit(AstGatePin* nodep) override {
        assert_At_ৰাশিমালা(nodep);
        যদি (m_vup->prelim()) {
            userIterateAndNext(nodep->rangep(), WidthVP{SELF, BOTH}.p());
            userIterateAndNext(nodep->exprp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            nodep->dtypeFrom(nodep->rangep());
            // Very much like like an pin
            ধ্রুৱ Ast_গ্ৰন্থি_DType* ধ্রুৱ pinDTypep = nodep->exprp()->dtypep();
            ধ্রুৱ পূর্ণাঙ্ক অঙ্ক_Insts = nodep->rangep()->elementsConst();
            ধ্রুৱ পূর্ণাঙ্ক modwidth = অঙ্ক_Insts;
            ধ্রুৱ পূর্ণাঙ্ক pinwidth = pinDTypep->width();
            যদি (pinwidth == 1 && modwidth > 1) {  // Multiple connections
                Ast_গ্ৰন্থি_DType* ধ্রুৱ subDTypep = nodep->findLogicDType(1, 1, pinDTypep->numeric());
                userIterateAndNext(nodep->exprp(), WidthVP{subDTypep, FINAL}.p());
                Ast_গ্ৰন্থি* ধ্রুৱ newp
                    = নূতন AstReplicate{nodep->fileline(), nodep->exprp()->unlinkFrBack(),
                                       static_cast<uint32_t>(অঙ্ক_Insts)};
                nodep->replaceWith(newp);
            } অন্যথা {
                যদি (pinwidth != modwidth) {  // && is not generic interconnect (when supported)
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
            VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
        }
    }
    শূন্য visit(Ast_গ্ৰন্থি_FTask* nodep) override {
        // Grab width from the output variable (if it's a function)
        যদি (nodep->didWidth()) ঘূৰাই;
        যদি (nodep->doingWidth()) {
            UINFO(5, "Recursive function or task call: " << nodep);
            nodep->recursive(সঁচা);
            nodep->didWidth(সঁচা);
            ঘূৰাই;
        }
        যদি (nodep->classMethod() && nodep->name() == "rand_mode") {
            nodep->v3error("The 'rand_mode' method is built-in and cannot be overridden"
                           " (IEEE 1800-2023 18.8)");
        } অন্যথা যদি (nodep->classMethod() && nodep->name() == "constraint_mode") {
            nodep->v3error("The 'constraint_mode' method is built-in and cannot be overridden"
                           " (IEEE 1800-2023 18.9)");
        }
        যদি (nodep->classMethod() && nodep->name() == "new") {
            যদি (nodep->isVirtual())
                nodep->v3error("class 'new()' cannot be virual (IEEE 1800-2023 18.3)");
            যদি (nodep->isStatic())
                nodep->v3error("class 'new()' cannot be static (IEEE 1800-2023 18.3)");
            Ast_গ্ৰন্থি* firstp = nullptr;
            বাবে (Ast_গ্ৰন্থি* itemp = nodep->stmtsp(); itemp; itemp = itemp->nextp()) {
                যদি (Ast_Stmt_ৰাশিমালা* ধ্রুৱ sep = VN_CAST(itemp, Stmt_ৰাশিমালা)) {
                    যদি (AstNew* ধ্রুৱ newp = VN_CAST(sep->exprp(), New)) {
                        যদি (firstp) {
                            UINFOTREE(1, firstp, "", "-earlier");
                            newp->v3warn(SUPERNFIRST,
                                         "'super.new' must be first statement in a 'function "
                                         "new' (IEEE 1800-2023 8.15)\n"
                                             << newp->warnContextPrimary() << '\n'
                                             << firstp->warnOther()
                                             << "... Location of earlier statement\n"
                                             << firstp->warnContextSecondary());
                            ভঙ্গ;
                        }
                    }
                    অনুৱর্ত;
                }
                যদি (firstNewStatementOkRecurse(itemp)) অনুৱর্ত;
                firstp = itemp;
            }
        }
        // Function hasn't been widthed, so make it so.
        // Would use user1 etc, but V3Width called from too many places to spend a user
        nodep->doingWidth(সঁচা);
        VL_RESTORER(m_funcp);
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        // First width the function variable, as if is a recursive function we need data type
        যদি (nodep->fvarp()) userIterate(nodep->fvarp(), nullptr);
        যদি (nodep->isConstructor()) {
            // Pretend it's void so less special casing needed when look at dtypes
            nodep->dtypeSetVoid();
        } অন্যথা যদি (nodep->fvarp()) {
            m_funcp = VN_AS(nodep, Func);
            UASSERT_OBJ(m_funcp, nodep, "FTask with function variable, but isn't a function");
            nodep->dtypeFrom(nodep->fvarp());  // Which will get it from fvarp()->dtypep()
        } অন্যথা যদি (VN_IS(nodep, Property)) {
            nodep->dtypeSetBit();
        }
        WidthVP* vup = nullptr;
        যদি (VN_IS(nodep, Property)) vup = WidthVP{SELF, BOTH}.p();
        userIterateChildren(nodep, vup);

        nodep->didWidth(সঁচা);
        nodep->doingWidth(মিছা);
        যদি (nodep->dpiImport() && !nodep->dpiOpenParent() && markHasOpenArray(nodep)) {
            nodep->dpiOpenParentInc();  // Mark so V3Task will wait for a child to build calling
                                        // func
        }
        যদি (nodep->fvarp() && !nodep->dpiImport() && !nodep->pureVirtual()
            && (!nodep->stmtsp() || !nodep->stmtsp()->existsAndNext([&](ধ্রুৱ AstVarRef* varrefp) {
                   ঘূৰাই varrefp->varp() == nodep->fvarp();
               }))) {
            nodep->v3warn(NORETURN,
                          "Non-void function " << nodep->prettyNameQ() << " has no return value");
        }

        std::vector<AstVar*> ports;
        std::vector<AstAttrOf*> protos;
        AstAttrOf* protop = nullptr;  // Base prototype
        বাবে (Ast_গ্ৰন্থি* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            যদি (AstVar* ধ্রুৱ portp = VN_CAST(stmtp, Var)) {
                যদি (portp->isIO() && !portp->isFuncReturn()) { ports.emplace_back(portp); }
            } অন্যথা যদি (AstAttrOf* ধ্রুৱ attrp = VN_CAST(stmtp, AttrOf)) {
                যদি (attrp->attrType() == VAttrType::FUNC_ARG_PROTO) { protos.emplace_back(attrp); }
                যদি (attrp->attrType() == VAttrType::FUNC_RETURN_PROTO) { protop = attrp; }
            }
        }
        যদি (protop) {
            বাবে (size_t i = 0; i < std::max(ports.size(), protos.size()); ++i) {
                যদি (i >= ports.size() || i >= protos.size()) {
                    protop->v3warn(
                        PROTOTYPEMIS,
                        "In prototype for "
                            << nodep->prettyNameQ()
                            << ", the argument counts do not match the out-of-block declaration"
                            << " (IEEE 1800-2023 8.24)\n"
                            << protop->warnContextPrimary() << '\n'
                            << nodep->warnOther() << "... Location of out-of-block declaration\n"
                            << nodep->warnContextSecondary());
                    ভঙ্গ;
                } অন্যথা {
                    AstVar* ধ্রুৱ portp = ports[i];
                    AstAttrOf* ধ্রুৱ rprotop = protos[i];
                    Ast_গ্ৰন্থি_DType* ধ্রুৱ declDtp = portp->dtypep();
                    Ast_গ্ৰন্থি_DType* ধ্রুৱ protoDtp = rprotop->fromp()->dtypep();
                    যদি (portp->name() != rprotop->name()) {
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
                    } অন্যথা যদি (!similarDTypeRecurse(protoDtp, declDtp)) {
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
    শূন্য visit(AstConstraint* nodep) override {
        যদি (nodep->didWidth()) ঘূৰাই;
        VL_RESTORER(m_constraintp);
        m_constraintp = nodep;
        userIterateChildren(nodep, nullptr);
        nodep->didWidth(সঁচা);
    }
    শূন্য visit(AstProperty* nodep) override {
        যদি (nodep->didWidth()) ঘূৰাই;
        যদি (nodep->doingWidth()) {
            UINFO(5, "Recursive property call: " << nodep);
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Recursive property call: " << nodep->prettyNameQ());
            nodep->recursive(সঁচা);
            nodep->didWidth(সঁচা);
            ঘূৰাই;
        }
        nodep->doingWidth(সঁচা);
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        // Property call will be replaced by property body in V3AssertPre. Property body has bit
        // dtype, so set it here too
        nodep->dtypeSetBit();
        বাবে (Ast_গ্ৰন্থি* propStmtp = nodep->stmtsp(); propStmtp; propStmtp = propStmtp->nextp()) {
            যদি (VN_IS(propStmtp, Var)) {
                userIterate(propStmtp, nullptr);
            } অন্যথা যদি (VN_IS(propStmtp, PropSpec)) {
                iterateCheckSelf(nodep, "PropSpec", propStmtp, SELF, BOTH);
            } অন্যথা যদি (VN_IS(propStmtp, InitialStaticStmt)
                       || VN_IS(propStmtp, InitialAutomaticStmt)) {
                // Property-local variable initialization -- iterate this node only
                userIterate(propStmtp, nullptr);
            } অন্যথা {
                propStmtp->v3fatalSrc("Invalid statement under AstProperty");
            }
        }
        nodep->didWidth(সঁচা);
        nodep->doingWidth(মিছা);
    }
    শূন্য visit(AstReturn* nodep) override { nodep->v3fatalSrc("'return' missed in LinkJump"); }
    শূন্য visit(AstSequence* nodep) override {
        // IEEE 1800-2023 16.7 (sequence declarations), 16.8 (sequence instances)
        // Width-check the sequence body so expressions are typed for later inlining.
        // Referenced sequences in assertion context will be inlined by V3AssertPre.
        যদি (nodep->didWidth()) ঘূৰাই;
        nodep->dtypeSetBit();
        বাবে (Ast_গ্ৰন্থি* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            যদি (VN_IS(stmtp, Var)) {
                userIterate(stmtp, nullptr);
            } অন্যথা যদি (VN_IS(stmtp, গ্ৰন্থি_ৰাশিমালা)) {
                iterateCheckSelf(nodep, "SeqBody", stmtp, SELF, BOTH);
            } অন্যথা {
                stmtp->v3fatalSrc("Invalid statement under AstSequence");
            }
        }
        nodep->didWidth(সঁচা);
        // Keep all sequences for now; cleanup happens after V3AssertPre inlining.
    }

    AstPackage* getItemPackage(Ast_গ্ৰন্থি* pkgItemp) {
        যেতিয়ালৈ (pkgItemp->backp() && pkgItemp->backp()->nextp() == pkgItemp) {
            pkgItemp = pkgItemp->backp();
        }
        ঘূৰাই VN_CAST(pkgItemp->backp(), Package);
    }
    শূন্য visit(AstFuncRef* nodep) override {
        visit(static_cast<Ast_গ্ৰন্থি_FTask_Ref*>(nodep));
        যদি (nodep->taskp() && VN_IS(nodep->taskp(), Task)) {
            UASSERT_OBJ(m_vup, nodep, "Function reference where widthed expression expectation");
            যদি (m_vup->prelim() && !VN_IS(nodep->backp(), Stmt_ৰাশিমালা))
                nodep->v3error(
                    "Cannot call a task/void-function as a function: " << nodep->prettyNameQ());
            nodep->dtypeSetVoid();
        } অন্যথা {
            nodep->dtypeFrom(nodep->taskp());
        }
        যদি (nodep->fileline()->timingOn()) {
            Ast_গ্ৰন্থি_Module* ধ্রুৱ classp = nodep->classOrPackagep();
            যদি (nodep->name() == "self" && classp->name() == "process") {
                // Find if package the class is in is std::
                AstPackage* ধ্রুৱ packagep = getItemPackage(classp);
                যদি (packagep && packagep->name() == "std") {
                    methodCallWarnTiming(nodep, "process");
                }
            }
        }
        // UINFOTREE(1, nodep, "", "FuncOut");
    }
    // Returns true if dtypep0 and dtypep1 have same dimensions
    স্থির তার্কিক areSameSize(AstUnpackArrayDType* dtypep0, AstUnpackArrayDType* dtypep1) {
        ধ্রুৱ std::vector<AstUnpackArrayDType*> dims0 = dtypep0->unpackDimensions();
        ধ্রুৱ std::vector<AstUnpackArrayDType*> dims1 = dtypep1->unpackDimensions();
        যদি (dims0.size() != dims1.size()) ঘূৰাই মিছা;
        বাবে (size_t i = 0; i < dims0.size(); ++i) {
            যদি (dims0[i]->elementsConst() != dims1[i]->elementsConst()) ঘূৰাই মিছা;
        }
        ঘূৰাই সঁচা;
    }
    // Makes sure that port and pin have same size and same datatype
    শূন্য checkUnpackedArrayArgs(AstVar* portp, Ast_গ্ৰন্থি* pinp) {
        যদি (AstUnpackArrayDType* ধ্রুৱ portDtypep
            = VN_CAST(portp->dtypep()->skipRefp(), UnpackArrayDType)) {
            যদি (AstUnpackArrayDType* ধ্রুৱ pinDtypep
                = VN_CAST(pinp->dtypep()->skipRefp(), UnpackArrayDType)) {
                যদি (!areSameSize(portDtypep, pinDtypep)) {
                    pinp->v3warn(E_UNSUPPORTED,
                                 "Shape of the argument does not match the shape of the parameter "
                                     << "(" << pinDtypep->prettyDTypeNameQ() << " v.s. "
                                     << portDtypep->prettyDTypeNameQ() << ")");
                }
                ধ্রুৱ AstBasicDType* ধ্রুৱ portBasicp = portDtypep->basicp();
                ধ্রুৱ AstBasicDType* ধ্রুৱ pinBasicp = pinDtypep->basicp();
                // Arrays of unpacked structures have no basic type
                ধ্রুৱ তার্কিক sameElemDType
                    = portBasicp && pinBasicp
                          ? portBasicp->width() == pinBasicp->width()
                                && portBasicp->keyword().isSameish(pinBasicp->keyword())
                          : pinDtypep->elemDTypep()->similarDType(portDtypep->elemDTypep());
                যদি (!sameElemDType) {
                    pinp->v3warn(
                        E_UNSUPPORTED,
                        "Data type of the argument is incompatible with the parameter"
                            << warnDTypes(pinp, pinDtypep, portDtypep, "Argument", "Parameter"));
                }
            } অন্যথা {
                pinp->v3warn(E_UNSUPPORTED, "Argument is not an unpacked array while parameter "
                                                << portp->prettyNameQ() << " is");
            }
        }
    }
    শূন্য processFTaskRefArgs(Ast_গ্ৰন্থি_FTask_Ref* nodep) {
        // For arguments, is assignment-like context; see IEEE rules in AstNodeAssign
        // Function hasn't been widthed, so make it so.
        UINFO(5, "  FTASKREF " << nodep);
        UASSERT_OBJ(nodep->taskp(), nodep, "Unlinked");
        যদি (nodep->didWidth()) ঘূৰাই;
        userIterate(nodep->taskp(), nullptr);
        //
        // And do the arguments to the task/function too
        কৰা {
        reloop:
            // taskConnects may create a new task, and change nodep->taskp()
            ধ্রুৱ V3TaskConnects tconnects
                = V3Task::taskConnects(nodep, nodep->taskp()->stmtsp(), &m_taskConnectState);
            যদি (m_taskConnectState.didWrap()) m_memberMap.clear();  // As added a member
            বাবে (ধ্রুৱ auto& tconnect : tconnects) {
                ধ্রুৱ AstVar* ধ্রুৱ portp = tconnect.first;
                AstArg* ধ্রুৱ argp = tconnect.second;
                Ast_গ্ৰন্থি_ৰাশিমালা* pinp = argp->exprp();
                যদি (!pinp) অনুৱর্ত;  // Argument error we'll find later
                // Prelim may cause the node to get replaced; we've lost our
                // pointer, so need to iterate separately later
                যদি (portp->attrSFormat()
                    && (!VN_IS(pinp, SFormatF) || pinp->nextp())) {  // Not already done
                    UINFO(4, "   sformat via metacomment: " << nodep);
                    VNRelinker handle;
                    argp->unlinkFrBackWithNext(&handle);  // Format + additional args, if any
                    Ast_গ্ৰন্থি_ৰাশিমালা* argsp = nullptr;
                    যেতিয়ালৈ (AstArg* ধ্রুৱ nextargp = VN_AS(argp->nextp(), Arg)) {
                        // Expression goes to SFormatF
                        argsp = argsp->addNext(nextargp->exprp()->unlinkFrBackWithNext());
                        nextargp->unlinkFrBack()->delete_বৃক্ষ();  // Remove the call's Arg wrapper
                    }
                    string format;
                    যদি (VN_IS(pinp, Const)) {
                        format = VN_AS(pinp, Const)->অঙ্ক().toString();
                    } অন্যথা {
                        pinp->v3error(
                            "Format to $display-like function must have constant format string");
                    }
                    VL_DO_DANGLING(ঠেলো_Deletep(argp), argp);
                    AstSFormatF* ধ্রুৱ newp
                        = নূতন AstSFormatF{nodep->fileline(), format, মিছা, argsp};
                    যদি (!newp->scopeNamep() && newp->formatScopeTracking()) {
                        newp->scopeNamep(নূতন AstScopeName{newp->fileline(), সঁচা});
                    }
                    handle.relink(নূতন AstArg{newp->fileline(), "", newp});
                    // Connection list is now incorrect (has extra args in it).
                    গচ্ছ reloop;  // so exit early; next loop will correct it
                }  //
                অন্যথা যদি (portp->basicp() && portp->basicp()->keyword() == VBasicDTypeKwd::STRING
                         && !VN_IS(pinp, CvtPackString)
                         && !VN_IS(pinp, SFormatF)  // Already generates a string
                         && !VN_IS(portp->dtypep(), UnpackArrayDType)  // Unpacked array must match
                         && !(VN_IS(pinp, VarRef) && VN_AS(pinp, VarRef)->varp()->basicp()
                              && VN_AS(pinp, VarRef)->varp()->basicp()->keyword()
                                     == VBasicDTypeKwd::STRING)) {
                    UINFO(4, "   Add CvtPackString: " << pinp);
                    VNRelinker handle;
                    pinp->unlinkFrBack(&handle);  // No next, that's the next pin
                    Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ newp = নূতন AstCvtPackString{pinp->fileline(), pinp};
                    handle.relink(newp);
                    pinp = newp;
                }
                যদি (portp->isWritable()) {
                    V3LinkLValue::linkLValueSet(pinp, portp->direction().pinAccess());
                }
                // AstPattern requires assignments to pass datatype on PRELIM
                VL_DO_DANGLING(userIterate(pinp, WidthVP{portp->dtypep(), PRELIM}.p()), pinp);
            }
        } যেতিয়ালৈ (মিছা);
        // Stage 2
        {
            ধ্রুৱ V3TaskConnects tconnects = V3Task::taskConnects(nodep, nodep->taskp()->stmtsp());
            বাবে (ধ্রুৱ auto& tconnect : tconnects) {
                AstVar* ধ্রুৱ portp = tconnect.first;
                ধ্রুৱ AstArg* ধ্রুৱ argp = tconnect.second;
                Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ pinp = argp->exprp();
                যদি (!pinp) অনুৱর্ত;  // Argument error we'll find later
                // Change data types based on above accept completion
                যদি (nodep->taskp()->dpiImport()) checkUnpackedArrayArgs(portp, pinp);
                যদি (portp->isDouble()) VL_DO_DANGLING(spliceCvtD(pinp), pinp);
            }
        }
        // Stage 3
        {
            ধ্রুৱ V3TaskConnects tconnects = V3Task::taskConnects(nodep, nodep->taskp()->stmtsp());
            বাবে (ধ্রুৱ auto& tconnect : tconnects) {
                ধ্রুৱ AstVar* ধ্রুৱ portp = tconnect.first;
                ধ্রুৱ AstArg* ধ্রুৱ argp = tconnect.second;
                Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ pinp = argp->exprp();
                যদি (!pinp) অনুৱর্ত;  // Argument error we'll find later
                // Do PRELIM again, because above accept may have exited early
                // due to node replacement
                userIterate(pinp, WidthVP{portp->dtypep(), PRELIM}.p());
            }
        }
        // Cleanup any open arrays
        যদি (markHasOpenArray(nodep->taskp())) makeOpenArrayShell(nodep);
        // Stage 4
        {
            ধ্রুৱ V3TaskConnects tconnects = V3Task::taskConnects(nodep, nodep->taskp()->stmtsp());
            বাবে (ধ্রুৱ auto& tconnect : tconnects) {
                ধ্রুৱ AstVar* ধ্রুৱ portp = tconnect.first;
                ধ্রুৱ AstArg* ধ্রুৱ argp = tconnect.second;
                Ast_গ্ৰন্থি_ৰাশিমালা* pinp = argp->exprp();
                যদি (!pinp) অনুৱর্ত;  // Argument error we'll find later
                Ast_গ্ৰন্থি_DType* ধ্রুৱ portDTypep = portp->dtypep()->skipRefToEnump();
                যদি (VN_IS(pinp, EmptyQueue)) {
                    Ast_গ্ৰন্থি_ৰাশিমালা* newp = newConsDynArrayOrQueue(pinp->fileline(), portDTypep);
                    যদি (!newp) {
                        pinp->v3warn(E_UNSUPPORTED,
                                     "Unsupported/Illegal: empty queue ('{}') in this context");
                        newp = নূতন AstConst{pinp->fileline(), AstConst::Unsized32{}, 0};
                    }
                    newp->dtypeFrom(portDTypep);
                    pinp->replaceWith(newp);
                    VL_DO_DANGLING(ঠেলো_Deletep(pinp), pinp);
                    pinp = newp;
                }
                ধ্রুৱ Ast_গ্ৰন্থি_DType* ধ্রুৱ pinDTypep = pinp->dtypep()->skipRefToEnump();
                ধ্রুৱ AstIfaceRefDType* ধ্রুৱ portIfacep
                    = VN_CAST(portDTypep->elemDTypep(সঁচা), IfaceRefDType);
                ধ্রুৱ VCastable ifaceCastable
                    = portIfacep ? Ast_গ্ৰন্থি::computeCastable(portDTypep, pinDTypep, pinp,
                                                            /* checkIfaceArgCompat */ সঁচা)
                                 : VCastable{VCastable::UNSUPPORTED};
                ধ্রুৱ তার্কিক matchingRefDTypes = portIfacep
                                                   ? ifaceCastable == VCastable::SAMEISH
                                                   : similarDTypeRecurse(portDTypep, pinDTypep);
                ধ্রুৱ তার্কিক pinIsNull = VN_IS(pinp, Const) && VN_AS(pinp, Const)->অঙ্ক().isNull();
                যদি ((portp->isRef() || portp->isConstRef()) && !matchingRefDTypes) {
                    যদি (portIfacep) {
                        pinp->v3error("Ref virtual interface argument "
                                      << portp->prettyNameQ()
                                      << " requires the same interface type, parameters, and "
                                         "modport.");
                    } অন্যথা {
                        pinp->v3error("Ref argument requires matching types;"
                                      << " port " << portp->prettyNameQ() << " requires "
                                      << portDTypep->prettyDTypeNameQ() << " but connection is "
                                      << pinDTypep->prettyDTypeNameQ() << ".");
                    }
                } অন্যথা যদি (portIfacep && portIfacep->isVirtual() && portp->is_প্ৰবেশ() && !pinIsNull
                           && !ifaceCastable.isAssignable()) {
                    pinp->v3error("Virtual interface argument "
                                  << portp->prettyNameQ()
                                  << " requires a compatible interface type, parameters, and "
                                     "modport.");
                } অন্যথা যদি (portp->isWritable() && pinp->width() != portp->width()) {
                    pinp->v3widthWarn(portp->width(), pinp->width(),
                                      "Function output argument "
                                          << portp->prettyNameQ() << " requires " << portp->width()
                                          << " bits, but connection's " << pinp->prettyTypeName()
                                          << " generates " << pinp->width() << " bits.");
                    VNRelinker relinkHandle;
                    pinp->unlinkFrBack(&relinkHandle);
                    Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ newp = নূতন AstResizeLValue{pinp->fileline(), pinp};
                    relinkHandle.relink(newp);
                }
                যদি (portp->direction() != VDirection::REF
                    && !(portp->basicp()
                         && portp->basicp()->untyped())  // for properties, handled in V3AssertPre
                    && ((VN_IS(portDTypep, BasicDType) && pinDTypep->isNonPackedArray())
                        || (VN_IS(pinDTypep, BasicDType) && portDTypep->isNonPackedArray()))) {
                    pinp->v3error("Function Argument expects " << portDTypep->prettyDTypeNameQ()
                                                               << ", got "
                                                               << pinDTypep->prettyDTypeNameQ());
                    অনুৱর্ত;
                }
                // An untyped formal preserves the actual argument's self-determined type.
                যদি (portp->basicp() && portp->basicp()->untyped()) {
                    iterateCheckSelf(nodep, "Function Argument", pinp, SELF, FINAL);
                } অন্যথা যদি (!portp->basicp() || portp->basicp()->isOpaque()) {
                    // Output args: at return caller = callee, reverse direction.
                    checkClassAssign(nodep, "Function Argument", pinp, portDTypep,
                                     portp->direction() == VDirection::OUTPUT);
                    userIterate(pinp, WidthVP{portDTypep, FINAL, STREAM_USE_ASSIGN}.p());
                } অন্যথা {
                    iterateCheckAssign(nodep, "Function Argument", pinp, FINAL, portDTypep,
                                       portp->direction().pinAccess());
                }
            }
        }
    }

    শূন্য handleStdRandomizeArgs(Ast_গ্ৰন্থি_FTask_Ref* ধ্রুৱ nodep) {
        AstConst* nullp = nullptr;
        বাবে (AstArg *argp = nodep->argsp(), *nextp; argp; argp = nextp) {
            nextp = VN_AS(argp->nextp(), Arg);
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ exprp = argp->exprp();
            যদি (AstConst* ধ্রুৱ constp = VN_CAST(exprp, Const)) {
                যদি (constp->অঙ্ক().isNull()) {
                    nullp = constp;
                    অনুৱর্ত;
                }
            }
            যদি (VN_IS(exprp, MemberSel)) {
                // Non-standard usage: std::randomize() with class-scoped member
                // IEEE 1800-2023 (18.12) limits args to current scope variables.
                // Verilator accepts this for compatibility with other simulators.
                অনুৱর্ত;
            }
            যদি (VN_IS(exprp, VarRef) || VN_IS(exprp, ArraySel) || VN_IS(exprp, StructSel)) {
                // Valid usage
                অনুৱর্ত;
            }
            যদি (ধ্রুৱ AstCMethodHard* ধ্রুৱ methodp = VN_CAST(exprp, CMethodHard)) {
                যদি (methodp->method() == VCMethod::ARRAY_AT
                    || methodp->method() == VCMethod::ARRAY_AT_WRITE) {
                    অনুৱর্ত;
                }
            }
            argp->v3error("Non-variable arguments for 'std::randomize()'.");
        }
        যদি (nullp) nullp->v3error("'std::randomize()' does not accept 'null' as arguments.");
    }
    শূন্য visit(Ast_গ্ৰন্থি_FTask_Ref* nodep) override {
        // For arguments, is assignment-like context; see IEEE rules in AstNodeAssign
        // Function hasn't been widthed, so make it so.
        যদি (nodep->didWidth()) ঘূৰাই;
        UINFO(5, "  FTASKREF " << nodep);
        AstWith* withp = nullptr;
        যদি (nodep->name() == "rand_mode" || nodep->name() == "constraint_mode") {
            v3Global.useRandomizeMethods(সঁচা);
            nodep->dtypep(nodep->findBasicDType(VBasicDTypeKwd::INT));
            ঘূৰাই;  // Handled in V3Randomize
        } অন্যথা যদি (nodep->name() == "randomize" || nodep->name() == "srandom"
                   || (!nodep->taskp()
                       && (nodep->name() == "get_randstate"
                           || nodep->name() == "set_randstate"))) {
            // TODO perhaps this should move to V3LinkDot
            AstClass* ধ্রুৱ classp = VN_CAST(nodep->classOrPackagep(), Class);
            যদি (nodep->classOrPackagep() && nodep->classOrPackagep()->name() == "std") {
                v3Global.useRandomizeMethods(সঁচা);
                Ast_গ্ৰন্থি_DType* ধ্রুৱ adtypep = nodep->findBitDType();
                withp = methodWithClause(nodep, মিছা, মিছা, adtypep->findVoidDType(),
                                         adtypep->findBitDType(), adtypep);
                বাবে (ধ্রুৱ AstArg* argp = nodep->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) {
                    userIterateAndNext(argp->exprp(), WidthVP{SELF, BOTH}.p());
                }
                handleStdRandomizeArgs(nodep);  // Provided args should be in current scope
                processFTaskRefArgs(nodep);
                যদি (withp) nodep->withp(withp);
                nodep->didWidth(সঁচা);
                ঘূৰাই;
            }
            UASSERT_OBJ(classp, nodep, "Classless rand-thing should have failed in V3LinkDot");
            যদি (nodep->name() == "randomize") {
                AstClassRefDType* ধ্রুৱ adtypep
                    = নূতন AstClassRefDType{nodep->fileline(), classp, nullptr};
                v3Global.rootp()->typeTablep()->addTypesp(adtypep);
                withp = methodWithClause(nodep, মিছা, মিছা, adtypep->findVoidDType(),
                                         adtypep->findBitDType(), adtypep);
                বাবে (ধ্রুৱ AstArg* argp = nodep->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) {
                    userIterateAndNext(argp->exprp(), WidthVP{SELF, BOTH}.p());
                }
                handleRandomizeArgs(nodep, classp);
            } অন্যথা যদি (nodep->name() == "srandom") {
                nodep->taskp(V3Randomize::newSRandomFunc(m_memberMap, classp));
                m_memberMap.clear();
            } অন্যথা যদি (nodep->name() == "get_randstate") {
                methodOkArguments(nodep, 0, 0);
                classp->baseMostClassp()->needRNG(সঁচা);
                v3Global.useRandomizeMethods(সঁচা);
                AstCExpr* ধ্রুৱ newp
                    = নূতন AstCExpr{nodep->fileline(), "__Vm_rng.get_randstate()", 1};
                newp->dtypeSetString();
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
                ঘূৰাই;
            } অন্যথা যদি (nodep->name() == "set_randstate") {
                methodOkArguments(nodep, 1, 1);
                Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ expr1p = nodep->argsp()->exprp();  // May edit
                iterateCheckString(nodep, "LHS", expr1p, BOTH);
                Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ exprp = nodep->argsp()->exprp();
                classp->baseMostClassp()->needRNG(সঁচা);
                v3Global.useRandomizeMethods(সঁচা);
                AstCExpr* ধ্রুৱ newp
                    = নূতন AstCExpr{nodep->fileline(), "__Vm_rng.set_randstate(", 1};
                newp->add(exprp->unlinkFrBack());
                newp->add(")");
                newp->dtypeSetVoid();
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
                ঘূৰাই;
            } অন্যথা {
                UASSERT_OBJ(মিছা, nodep, "Bad case");
            }
        }
        UASSERT_OBJ(nodep->taskp(), nodep, "Unlinked");
        যদি (nodep->didWidth()) {
            যদি (withp) nodep->withp(withp);
            ঘূৰাই;
        }
        যদি ((nodep->taskp()->classMethod() && !nodep->taskp()->isStatic())
            && !VN_IS(m_procedurep, InitialAutomatic) && !m_constraintp) {
            তার্কিক allow = মিছা;
            যদি (m_ftaskp && m_ftaskp->classMethod() && !m_ftaskp->isStatic()) {
                যদি (ধ্রুৱ AstFuncRef* ধ্রুৱ funcRefp = VN_CAST(nodep, FuncRef)) {
                    allow = funcRefp->superReference();
                } অন্যথা যদি (ধ্রুৱ AstTaskRef* ধ্রুৱ taskRefp = VN_CAST(nodep, TaskRef)) {
                    allow = taskRefp->superReference();
                }
                যদি (!allow) {
                    ধ্রুৱ AstClass* callerClassp = m_containingClassFinder.find(m_ftaskp);
                    যদি (!callerClassp) {
                        callerClassp = m_containingClassFinder.find(m_ftaskp->classOrPackagep());
                    }
                    ধ্রুৱ AstClass* calleeClassp = VN_CAST(nodep->classOrPackagep(), Class);
                    যদি (!calleeClassp) {
                        calleeClassp = m_containingClassFinder.find(nodep->taskp());
                    }
                    allow = AstClass::isClassExtendedFrom(callerClassp, calleeClassp);
                }
            }
            যদি (!allow) {
                // An embedded covergroup may call methods of its enclosing class
                // (IEEE 1800-2023 19.4); V3Covergroup routes the call through its handle.
                ধ্রুৱ AstClass* ধ্রুৱ cgClassp = m_containingClassFinder.find(nodep);
                যদি (cgClassp && cgClassp->covergroupEnclosingClassp()) {
                    ধ্রুৱ AstClass* calleeClassp = VN_CAST(nodep->classOrPackagep(), Class);
                    যদি (!calleeClassp) {
                        calleeClassp = m_containingClassFinder.find(nodep->taskp());
                    }
                    allow = AstClass::isClassExtendedFrom(cgClassp->covergroupEnclosingClassp(),
                                                          calleeClassp);
                }
            }
            যদি (!allow) {
                nodep->v3error("Cannot call non-static member function "
                               << nodep->prettyNameQ() << " without object (IEEE 1800-2023 8.10)");
            }
        }
        যদি (nodep->taskp() && !nodep->scopeNamep()
            && (nodep->taskp()->dpiContext() || nodep->taskp()->dpiExport())) {
            nodep->scopeNamep(নূতন AstScopeName{nodep->fileline(), মিছা});
        }
        // And do the arguments to the task/function too
        processFTaskRefArgs(nodep);
        যদি (withp) nodep->withp(withp);
        nodep->didWidth(সঁচা);
        // See steps that follow in visit(AstFuncRef*)
    }
    শূন্য visit(Ast_গ্ৰন্থি_Procedure* nodep) override {
        assertAtStatement(nodep);
        VL_RESTORER(m_procedurep);
        m_procedurep = nodep;
        userIterateChildren(nodep, nullptr);
    }
    শূন্য visit(AstSenItem* nodep) override {
        যদি (nodep->isComboStar()) ঘূৰাই;
        UASSERT_OBJ(nodep->isClocked(), nodep, "Invalid edge");
        // Optimize concat/replicate senitems; this has to be done here at the latest, otherwise we
        // emit WIDTHCONCAT if there are unsized constants
        যদি (VN_IS(nodep->sensp(), Concat) || VN_IS(nodep->sensp(), Replicate)) {
            auto* ধ্রুৱ concatOrReplp = VN_CAST(nodep->sensp(), গ্ৰন্থি_Biop);
            auto* ধ্রুৱ rhsp = concatOrReplp->rhsp()->unlinkFrBack();
            যদি (nodep->edgeType() == VEdgeType::ET_CHANGED) {
                // If it's ET_CHANGED, split concatenations into multiple senitems
                auto* ধ্রুৱ lhsp = concatOrReplp->lhsp()->unlinkFrBack();
                nodep->addNextHere(নূতন AstSenItem{lhsp->fileline(), nodep->edgeType(), lhsp});
            }  // Else only use the RHS
            nodep->replaceWith(নূতন AstSenItem{rhsp->fileline(), nodep->edgeType(), rhsp});
            VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
        } অন্যথা {
            userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
            যদি (nodep->edgeType().anEdge()) {
                Ast_গ্ৰন্থি_DType* ধ্রুৱ sensDtp = nodep->sensp()->dtypep()->skipRefp();
                যদি (sensDtp->isDouble()) {
                    nodep->sensp()->v3error(
                        "Edge event control not legal on real type (IEEE 1800-2023 6.12.1)");
                } অন্যথা যদি (sensDtp->basicp() && !sensDtp->basicp()->keyword().isIntNumeric()) {
                    nodep->sensp()->v3error("Edge event control not legal on non-integral type "
                                            "(IEEE 1800-2023 9.4.2)");
                }
            }
        }
    }
    শূন্য visit(AstClockingItem* nodep) override {
        nodep->exprp()->foreach(
            [nodep](AstVarRef* ধ্রুৱ refp) { refp->access(nodep->direction().pinAccess()); });
        userIterateChildren(nodep, WidthVP{SELF, PRELIM}.p());
    }
    শূন্য visit(AstWait* nodep) override {
        যদি (!m_underFork && VN_IS(m_ftaskp, Func)) {
            nodep->v3error("Wait statements are not legal in functions. Suggest use a task "
                           "(IEEE 1800-2023 13.4.4)");
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_বৃক্ষ(), nodep);
            ঘূৰাই;
        }
        যদি (nodep->fileline()->timingOn()) {
            যদি (v3Global.opt.timing().isSetTrue()) {
                iterateCheckBool(nodep, "Wait", nodep->condp(),
                                 BOTH);  // it's like an if() condition.
                // TODO check also inside complex event expressions
                যদি (Ast_গ্ৰন্থি_Var_Ref* ধ্রুৱ varrefp = VN_CAST(nodep->condp(), গ্ৰন্থি_Var_Ref)) {
                    যদি (varrefp->isEvent()) {
                        varrefp->v3error("Wait statement conditions do not take raw events"
                                         " (IEEE 1800-2023 15.5.3)\n"
                                         << varrefp->warnMore() << "... Suggest use '"
                                         << varrefp->prettyName() << ".triggered'");
                    }
                }
                iterateNull(nodep->stmtsp());
                ঘূৰাই;
            } অন্যথা যদি (v3Global.opt.timing().isSetFalse()) {
                nodep->v3warn(E_NOTIMING, "Wait statements require --timing");
            } অন্যথা {
                nodep->v3warn(E_NEEDTIMINGOPT, "Use --timing or --no-timing to specify how wait "
                                               "statements should be handled");
            }
        }
        // If we ignore timing:
        // Statements we'll just execute immediately; equivalent to if they followed this
        যদি (Ast_গ্ৰন্থি* ধ্রুৱ stmtsp = nodep->stmtsp()) {
            stmtsp->unlinkFrBackWithNext();
            nodep->replaceWith(stmtsp);
        } অন্যথা {
            nodep->unlinkFrBack();
        }
        VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
    }
    শূন্য visit(AstWith* nodep) override {
        // Should otherwise be underneath a method call
        Ast_গ্ৰন্থি_DType* ধ্রুৱ vdtypep = m_vup->dtypeNullSkipRefp();
        VL_RESTORER(m_withp);
        m_withp = nodep;
        userIterateChildren(nodep->indexArgRefp(), nullptr);
        userIterateChildren(nodep->মান_Arg_Refp(), nullptr);
        যদি (!nodep->exprp()) {
            nodep->dtypeSetVoid();
        } অন্যথা {
            যদি (!nodep->exprp()->hasDType()) {
                userIterateAndNext(nodep->exprp(), nullptr);
            } অন্যথা {
                যদি (vdtypep) {
                    userIterateAndNext(nodep->exprp(), WidthVP{nodep->dtypep(), PRELIM}.p());
                } অন্যথা {  // 'sort with' allows arbitrary type
                    userIterateAndNext(nodep->exprp(), WidthVP{SELF, PRELIM}.p());
                }
            }

            যদি (!nodep->exprp()->hasDType()) {
                nodep->dtypeSetVoid();
            } অন্যথা {
                nodep->dtypeFrom(nodep->exprp());
                iterateCheckAssign(nodep, "'with' return value", nodep->exprp(), FINAL,
                                   nodep->dtypep());
            }
        }
    }
    শূন্য visit(AstLambdaArgRef* nodep) override {
        UASSERT_OBJ(m_withp, nodep, "LambdaArgRef not underneath 'with' lambda");
        যদি (nodep->index()) {
            nodep->dtypeFrom(m_withp->indexArgRefp());
        } অন্যথা {
            nodep->dtypeFrom(m_withp->মান_Arg_Refp());
        }
    }
    শূন্য visit(AstNetlist* nodep) override {
        // Iterate modules backwards, in bottom-up order.  That's faster
        userIterateChildrenBackwardsConst(nodep, nullptr);
    }

    //--------------------
    // Default
    শূন্য visit(Ast_গ্ৰন্থি_ৰাশিমালা* nodep) override {
        যদি (!nodep->didWidth()) {
            nodep->v3fatalSrc(
                "Visit function missing? Widthed function missing for math node: " << nodep);
        }
        userIterateChildren(nodep, nullptr);
    }
    শূন্য visit(AstResizeLValue* nodep) override {
        // RESIZELVALUE adjusts width of lvalues for assignments/function calls
        // The parent context determines the required width
        UINFO(9, "visit AstResizeLValue " << nodep << endl);
        যদি (nodep->didWidthAndSet()) ঘূৰাই;

        UASSERT_OBJ(nodep->lhsp(), nodep, "ResizeLValue missing lhs expression");

        // First, process child to know its natural width
        যদি (m_vup->prelim()) {
            userIterateAndNext(nodep->lhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
        }

        // Get the required width from parent context
        যদি (Ast_গ্ৰন্থি_DType* ধ্রুৱ vdtypep = m_vup->dtypeNullp()) {
            // Parent specified required width - use it
            nodep->dtypeFrom(vdtypep);
        } অন্যথা যদি (!nodep->dtypep()) {
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
        nodep->didWidth(সঁচা);
    }
    শূন্য visitClass(AstClass* nodep) {
        যদি (nodep->didWidthAndSet()) ঘূৰাই;

        // If the class is std::process
        যদি (nodep->name() == "process") {
            AstPackage* ধ্রুৱ packagep = getItemPackage(nodep);
            যদি (packagep && packagep->name() == "std") {
                // Change type of m_process to VlProcessRef
                যদি (AstVar* ধ্রুৱ varp
                    = VN_CAST(m_memberMap.findMember(nodep, "m_process"), Var)) {
                    Ast_গ্ৰন্থি_DType* ধ্রুৱ dtypep = varp->getChildDTypep();
                    যদি (!varp->dtypep()) {
                        VL_DO_DANGLING(ঠেলো_Deletep(dtypep->unlinkFrBack()), dtypep);
                    }
                    AstBasicDType* ধ্রুৱ newdtypep = নূতন AstBasicDType{
                        nodep->fileline(), VBasicDTypeKwd::PROCESS_REFERENCE, VSigning::UNSIGNED};
                    v3Global.rootp()->typeTablep()->addTypesp(newdtypep);
                    varp->dtypep(newdtypep);
                }
                // Mark that self requires process instance
                যদি (Ast_গ্ৰন্থি_FTask* ধ্রুৱ ftaskp
                    = VN_CAST(m_memberMap.findMember(nodep, "self"), গ্ৰন্থি_FTask)) {
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
    শূন্য visit(Ast_গ্ৰন্থি_Module* nodep) override {
        assertAtStatement(nodep);
        VL_RESTORER_COPY(m_enumTempNames);
        VL_RESTORER_COPY(m_insideTempNames);
        যদি (AstClass* ধ্রুৱ classp = VN_CAST(nodep, Class)) {
            visitClass(classp);
        } অন্যথা {
            VL_RESTORER(m_modep);
            m_modep = nodep;
            // Collect local variables for ASSIGNIN hierarchical reference check
            m_curModVars.clear();
            nodep->foreach([এতত্](ধ্রুৱ AstVar* varp) { m_curModVars.ঢোকাও(varp); });
            userIterateChildren(nodep, nullptr);
        }
    }
    শূন্য visit(Ast_গ্ৰন্থি* nodep) override {
        // Default: Just iterate
        UASSERT_OBJ(!m_vup, nodep,
                    "Visit function missing? Widthed expectation for this node: " << nodep);
        userIterateChildren(nodep, nullptr);
    }

    //----------------------------------------------------------------------
    // WIDTH METHODs -- all iterate

    শূন্য visit_Or_Lu64(Ast_গ্ৰন্থি_Uniop* nodep) {
        // CALLER: AstBitsToRealD
        // Real: Output real
        // LHS presumed self-determined, then coerced to real
        assert_At_ৰাশিমালা(nodep);
        যদি (m_vup->prelim()) {  // First stage evaluation
            nodep->dtypeSetDouble();
            Ast_গ্ৰন্থি_DType* ধ্রুৱ subDTypep = nodep->findBitDType(64, 64, VSigning::UNSIGNED);
            // Self-determined operand
            userIterateAndNext(nodep->lhsp(), WidthVP{SELF, PRELIM}.p());
            iterateCheck(nodep, "LHS", nodep->lhsp(), SELF, FINAL, subDTypep, EXTEND_EXP);
        }
    }
    শূন্য visit(AstIToRD* nodep) override {
        // Real: Output real
        // LHS presumed self-determined, then coerced to real
        assert_At_ৰাশিমালা(nodep);
        যদি (m_vup->prelim()) {  // First stage evaluation
            nodep->dtypeSetDouble();
            // Self-determined operand (TODO check if numeric type)
            userIterateAndNext(nodep->lhsp(), WidthVP{SELF, PRELIM}.p());
            যদি (nodep->lhsp()->isSigned()) {
                nodep->replaceWith(
                    নূতন AstISToRD{nodep->fileline(), nodep->lhsp()->unlinkFrBack()});
                VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
            }
        }
    }
    শূন্য visit(AstISToRD* nodep) override {
        // Real: Output real
        // LHS presumed self-determined, then coerced to real
        assert_At_ৰাশিমালা(nodep);
        যদি (m_vup->prelim()) {  // First stage evaluation
            nodep->dtypeSetDouble();
            // Self-determined operand (TODO check if numeric type)
            userIterateAndNext(nodep->lhsp(), WidthVP{SELF, PRELIM}.p());
        }
    }
    শূন্য visit_Os32_Lr(Ast_গ্ৰন্থি_Uniop* nodep) {
        // CALLER: RToI
        // Real: LHS real
        // LHS presumed self-determined, then coerced to real
        assert_At_ৰাশিমালা(nodep);
        যদি (m_vup->prelim()) {  // First stage evaluation
            iterateCheckReal(nodep, "LHS", nodep->lhsp(), BOTH);
            nodep->dtypeSetInteger2State();
        }
    }
    শূন্য visit_Ou64_Lr(Ast_গ্ৰন্থি_Uniop* nodep) {
        // CALLER: RealToBits
        // Real: LHS real
        // LHS presumed self-determined, then coerced to real
        assert_At_ৰাশিমালা(nodep);
        যদি (m_vup->prelim()) {  // First stage evaluation
            iterateCheckReal(nodep, "LHS", nodep->lhsp(), BOTH);
            nodep->dtypeSetUInt64();
        }
    }

    শূন্য visit_log_not(AstLogNot* nodep) {
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
        assert_At_ৰাশিমালা(nodep);
        যদি (m_vup->prelim()) {
            iterateCheckBool(nodep, "LHS", nodep->op1p(), BOTH);
            nodep->dtypeSetBitOrLogic(nodep->lhsp()->isFourstateOrNoDtype());
            // IEEE 1800-2023 16.12.3: property 'not' is not a sequence operator.
            // Boolean '!' is allowed in sequences (16.7 expression_or_dist).
            // The parser distinguishes the two via AstLogNot::fromProperty().
            যদি (m_underSExpr && nodep->fromProperty()) {
                nodep->v3error("Unexpected 'not' in sequence expression context");
                AstConst* ধ্রুৱ newp
                    = নূতন AstConst{nodep->fileline(), AstConst::BitFalseErroring{}};
                newp->dtypeFrom(nodep);
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
            }
        }
    }
    শূন্য visit_log_and_or(Ast_গ্ৰন্থি_Biop* nodep) {
        // CALLER: LogAnd, LogOr, LogEq, LogIf
        // Widths: 1 bit out, lhs 1 bit, rhs 1 bit
        // IEEE-2012 Table 11-21:
        //   LHS is self-determined
        //   RHS is self-determined
        assert_At_ৰাশিমালা(nodep);
        যদি (m_vup->prelim()) {
            iterateCheckBool(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckBool(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtypeSetBitOrLogic(
                Ast_গ্ৰন্থি_ৰাশিমালা::isFourstateOrNoDtype(nodep->lhsp(), nodep->rhsp()));
        }
    }
    শূন্য visitAbortProp(AstAbortOn* nodep) {
        // IEEE 1800-2023 16.12.14: abort condition is a 1-bit self-determined
        // Boolean; property subexpression carries its own type checking.
        // VAbortKind distinguishes accept/reject and sync/async, but the
        // type-check path is identical for all four.
        assert_At_ৰাশিমালা(nodep);
        যদি (m_vup->prelim()) {
            iterateCheckBool(nodep, "cond", nodep->condp(), BOTH);
            iterateCheckBool(nodep, "prop", nodep->propp(), BOTH);
            nodep->dtypeSetBit();
        }
    }
    শূন্য visit_red_and_or(Ast_গ্ৰন্থি_Uniop* nodep) {
        // CALLER: RedAnd, RedOr, ...
        // Signed: Output unsigned, Lhs/Rhs/etc non-real (presumed, not in IEEE)
        // IEEE-2012: Table 11-21 and 11.8.1:
        //   LHS is self-determined
        //   Width: 1 bit out
        //   Sign: unsigned out (11.8.1)
        assert_At_ৰাশিমালা(nodep);
        যদি (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->lhsp(), SELF, BOTH);
            nodep->dtypeSetBitOrLogic(nodep->lhsp()->isFourstateOrNoDtype());
        }
    }
    শূন্য visit_red_unknown(Ast_গ্ৰন্থি_Uniop* nodep) {
        // CALLER: IsUnknown
        // Signed: Output unsigned, Lhs/Rhs/etc non-real (presumed, not in IEEE)
        // IEEE-2012: Table 11-21 and 11.8.1:
        //   LHS is self-determined
        //   Width: 1 bit out
        //   Sign: unsigned out (11.8.1)
        assert_At_ৰাশিমালা(nodep);
        যদি (m_vup->prelim()) {
            userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
            nodep->dtypeSetBit();
        }
    }

    // IEEE 1800-2023 6.22.2 Equivalent types
    তার্কিক isEquivalentDType(ধ্রুৱ Ast_গ্ৰন্থি_DType* lhs, ধ্রুৱ Ast_গ্ৰন্থি_DType* rhs) {
        // a) If two types match, they are equivalent.
        যদি (!lhs || !rhs) ঘূৰাই মিছা;
        lhs = lhs->skipRefToEnump();
        rhs = rhs->skipRefToEnump();
        যদি (lhs == rhs) ঘূৰাই সঁচা;
        // If both are basic types, check if they are the same type
        যদি (VN_IS(lhs, BasicDType) && VN_IS(rhs, BasicDType)) {
            ধ্রুৱ auto* lb = VN_CAST(lhs, BasicDType);
            ধ্রুৱ auto* rb = VN_CAST(rhs, BasicDType);
            যদি (lb->isString() != rb->isString()) ঘূৰাই মিছা;
        }

        // d) Unpacked fixed-size array types are equivalent if they have equivalent element types
        // and equal size; the actual range bounds may differ. Note that the element type of a
        // multidimensional array is itself an array type.
        ধ্রুৱ তার্কিক lhsIsUnpackArray = VN_IS(lhs, UnpackArrayDType);
        ধ্রুৱ তার্কিক rhsIsUnpackArray = VN_IS(rhs, UnpackArrayDType);
        যদি (lhsIsUnpackArray || rhsIsUnpackArray) {
            যদি (lhsIsUnpackArray && rhsIsUnpackArray) {
                ধ্রুৱ AstUnpackArrayDType* ধ্রুৱ lhsp = VN_CAST(lhs, UnpackArrayDType);
                ধ্রুৱ AstUnpackArrayDType* ধ্রুৱ rhsp = VN_CAST(rhs, UnpackArrayDType);
                ধ্রুৱ পূর্ণাঙ্ক lsz = lhsp->elementsConst();
                ধ্রুৱ পূর্ণাঙ্ক rsz = rhsp->elementsConst();
                যদি (lsz >= 0 && rsz >= 0 && lsz != rsz) ঘূৰাই মিছা;
                ঘূৰাই isEquivalentDType(lhsp->subDTypep(), rhsp->subDTypep());
            }
            ঘূৰাই মিছা;
        }

        // e) Dynamic array, associative array, and queue types are equivalent if they are the same
        // kind of array (dynamic, associative, or queue), have equivalent index types (for
        // associative arrays), and have equivalent element types.
        ধ্রুৱ তার্কিক lhsIsDynArray = VN_IS(lhs, DynArrayDType);
        ধ্রুৱ তার্কিক rhsIsDynArray = VN_IS(rhs, DynArrayDType);
        ধ্রুৱ তার্কিক lhsIsQueue = VN_IS(lhs, QueueDType);
        ধ্রুৱ তার্কিক rhsIsQueue = VN_IS(rhs, QueueDType);
        ধ্রুৱ তার্কিক lhsIsAssocArray = VN_IS(lhs, AssocArrayDType);
        ধ্রুৱ তার্কিক rhsIsAssocArray = VN_IS(rhs, AssocArrayDType);

        যদি (lhsIsDynArray || rhsIsDynArray || lhsIsQueue || rhsIsQueue || lhsIsAssocArray
            || rhsIsAssocArray) {
            যদি (ধ্রুৱ AstDynArrayDType* ধ্রুৱ lhsp = VN_CAST(lhs, DynArrayDType)) {
                যদি (ধ্রুৱ AstDynArrayDType* ধ্রুৱ rhsp = VN_CAST(rhs, DynArrayDType)) {
                    ঘূৰাই isEquivalentDType(lhsp->subDTypep(), rhsp->subDTypep());
                }
            }

            যদি (ধ্রুৱ AstQueueDType* ধ্রুৱ lhsp = VN_CAST(lhs, QueueDType)) {
                যদি (ধ্রুৱ AstQueueDType* ধ্রুৱ rhsp = VN_CAST(rhs, QueueDType)) {
                    ঘূৰাই isEquivalentDType(lhsp->subDTypep(), rhsp->subDTypep());
                }
            }

            যদি (ধ্রুৱ AstAssocArrayDType* ধ্রুৱ lhsp = VN_CAST(lhs, AssocArrayDType)) {
                যদি (ধ্রুৱ AstAssocArrayDType* ধ্রুৱ rhsp = VN_CAST(rhs, AssocArrayDType)) {
                    ঘূৰাই isEquivalentDType(lhsp->subDTypep(), rhsp->subDTypep())
                           && isEquivalentDType(lhsp->চাবি_DTypep(), rhsp->চাবি_DTypep());
                }
            }

            ঘূৰাই মিছা;
        }

        // c) Packed arrays, packed structures, packed unions, and built-in integral
        // types are equivalent if they contain the same number of total bits, are either all
        // 2-state or all 4-state, and are either all signed or all unsigned.
        যদি (lhs->isIntegralOrPacked() && rhs->isIntegralOrPacked()) {
            যদি (lhs->width() != rhs->width()) ঘূৰাই মিছা;
            যদি (lhs->isFourstate() != rhs->isFourstate()) ঘূৰাই মিছা;
            যদি (lhs->isSigned() != rhs->isSigned()) ঘূৰাই মিছা;
            ঘূৰাই সঁচা;
        }

        // b) An anonymous enum, unpacked struct, or unpacked union type is equivalent to itself
        // among data objects declared within the same declaration statement and no other data
        // types.

        // DTypes compared above, if any remained then not equivalent.
        যদি (VN_IS(lhs, EnumDType) || VN_IS(rhs, EnumDType)) ঘূৰাই মিছা;
        ঘূৰাই !(VN_IS(lhs, গ্ৰন্থি_UOr_Struct_DType) || VN_IS(rhs, গ্ৰন্থি_UOr_Struct_DType));
    }

    স্থির তার্কিক isAggregateType(ধ্রুৱ Ast_গ্ৰন্থি* nodep) {
        যদি (!nodep) ঘূৰাই মিছা;
        ধ্রুৱ Ast_গ্ৰন্থি_DType* dtypep = nodep->dtypep();
        যদি (!dtypep) ঘূৰাই মিছা;
        dtypep = dtypep->skipRefp();
        যদি (!dtypep) ঘূৰাই মিছা;
        ঘূৰাই dtypep->isAggregateType();
    }

    শূন্য visit_cmp_eq_gt(Ast_গ্ৰন্থি_Biop* nodep, ধ্রুৱ ComparisonType cmpType) {
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
        assert_At_ৰাশিমালা(nodep);
        যদি (m_vup->prelim()) {
            // Assignment patterns cannot self-determine their type.
            // Iterate the non-pattern operand first so its dtype is available,
            // then propagate that dtype to the pattern operand (analogous to
            // how assignments pass LHS dtype to RHS patterns).
            ধ্রুৱ তার্কিক lhsIsPat = VN_IS(nodep->lhsp(), Pattern);
            ধ্রুৱ তার্কিক rhsIsPat = VN_IS(nodep->rhsp(), Pattern);
            যদি (lhsIsPat && !rhsIsPat) {
                userIterateAndNext(nodep->rhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
                userIterateAndNext(nodep->lhsp(), WidthVP{nodep->rhsp()->dtypep(), PRELIM}.p());
            } অন্যথা যদি (rhsIsPat && !lhsIsPat) {
                userIterateAndNext(nodep->lhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
                userIterateAndNext(nodep->rhsp(), WidthVP{nodep->lhsp()->dtypep(), PRELIM}.p());
            } অন্যথা {
                userIterateAndNext(nodep->lhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
                userIterateAndNext(nodep->rhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            }

            ধ্রুৱ তার্কিক isAggrLhs = isAggregateType(nodep->lhsp());
            ধ্রুৱ তার্কিক isAggrRhs = isAggregateType(nodep->rhsp());

            যদি ((isAggrLhs || isAggrRhs)
                && (cmpType == ComparisonType::NORMAL_EQ || cmpType == ComparisonType::CASE)) {
                ধ্রুৱ Ast_গ্ৰন্থি_DType* ধ্রুৱ lhsDType = nodep->lhsp()->dtypep();
                ধ্রুৱ Ast_গ্ৰন্থি_DType* ধ্রুৱ rhsDType = nodep->rhsp()->dtypep();

                যদি (lhsDType && rhsDType && !isEquivalentDType(lhsDType, rhsDType)) {
                    nodep->v3error("Comparison requires matching data types"
                                   << warnDTypes(nodep, lhsDType, rhsDType));
                    Ast_গ্ৰন্থি* ধ্রুৱ newp
                        = নূতন AstConst{nodep->fileline(), AstConst::BitFalseErroring{}};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
                    ঘূৰাই;
                }
                // Elements are compared by their left-to-right order, as when assigned (7.4.6)
                convertUnpackedOrder(nodep->rhsp(), nodep->lhsp()->dtypep());
            } অন্যথা যদি (nodep->lhsp()->isDouble() || nodep->rhsp()->isDouble()) {
                যদি (cmpType == ComparisonType::WILD) {
                    nodep->v3error("Real is illegal operand to ?== operator");
                    Ast_গ্ৰন্থি* ধ্রুৱ newp
                        = নূতন AstConst{nodep->fileline(), AstConst::BitFalseErroring{}};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
                    ঘূৰাই;
                }
                যদি (Ast_গ্ৰন্থি_Biop* ধ্রুৱ newp = replaceWithDVersion(nodep)) {
                    VL_DANGLING(nodep);
                    nodep = newp;  // Process new node instead
                    iterateCheckReal(nodep, "LHS", nodep->lhsp(), FINAL);
                    iterateCheckReal(nodep, "RHS", nodep->rhsp(), FINAL);
                }
            } অন্যথা যদি (nodep->lhsp()->isString() || nodep->rhsp()->isString()) {
                যদি (Ast_গ্ৰন্থি_Biop* ধ্রুৱ newp = replaceWithNVersion(nodep)) {
                    VL_DANGLING(nodep);
                    nodep = newp;  // Process new node instead
                    iterateCheckString(nodep, "LHS", nodep->lhsp(), FINAL);
                    iterateCheckString(nodep, "RHS", nodep->rhsp(), FINAL);
                }
            } অন্যথা {
                ধ্রুৱ তার্কিক signedFl = nodep->lhsp()->isSigned() && nodep->rhsp()->isSigned();
                যদি (Ast_গ্ৰন্থি_Biop* ধ্রুৱ newp = replaceWithUOrSVersion(nodep, signedFl)) {
                    VL_DANGLING(nodep);
                    nodep = newp;  // Process new node instead
                }
                ধ্রুৱ পূর্ণাঙ্ক width = std::max(nodep->lhsp()->width(), nodep->rhsp()->width());
                ধ্রুৱ পূর্ণাঙ্ক ewidth = std::max(nodep->lhsp()->widthMin(), nodep->rhsp()->widthMin());
                তার্কিক warnOn = সঁচা;
                যদি (!signedFl && width == 32) {
                    // Waive on unsigned < or <= if RHS is narrower, since can't give wrong answer
                    যদি ((VN_IS(nodep, Lt) || VN_IS(nodep, Lte))
                        && (nodep->lhsp()->width() >= nodep->rhsp()->widthMin())) {
                        warnOn = মিছা;
                    }
                    // Waive on unsigned > or >= if RHS is wider, since can't give wrong answer
                    যদি ((VN_IS(nodep, Gt) || VN_IS(nodep, Gte))
                        && (nodep->lhsp()->widthMin() >= nodep->rhsp()->width())) {
                        warnOn = মিছা;
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
    শূন্য visit_cmp_real(Ast_গ্ৰন্থি_Biop* nodep) {
        // CALLER: EqD, LtD
        // Widths: 1 bit out, lhs width == rhs width
        // Signed compare (not output) if both sides signed
        // Real if and only if real_allow set
        // IEEE, 11.4.4: relational compares (<,>,<=,>=,==,===,!=,!==) use
        // "zero padding" on unsigned
        UASSERT_OBJ(nodep->rhsp(), nodep, "For binary ops only!");
        assert_At_ৰাশিমালা(nodep);
        যদি (m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckReal(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckReal(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtypeSetBit();
        }
    }
    শূন্য visit_cmp_string(Ast_গ্ৰন্থি_Biop* nodep) {
        // CALLER: EqN, LtN
        // Widths: 1 bit out, lhs width == rhs width
        // String compare (not output)
        // Real if and only if real_allow set
        UASSERT_OBJ(nodep->rhsp(), nodep, "For binary ops only!");
        assert_At_ৰাশিমালা(nodep);
        যদি (m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckString(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtypeSetBit();
        }
    }
    শূন্য visit_cmp_type(Ast_গ্ৰন্থি_Biop* nodep) {
        // CALLER: EqT, LtT
        // Widths: 1 bit out
        // Data type compare (not output)
        assert_At_ৰাশিমালা(nodep);
        যদি (m_vup->prelim()) {
            userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
            userIterateAndNext(nodep->rhsp(), WidthVP{SELF, BOTH}.p());
            ধ্রুৱ AstAttrOf* ধ্রুৱ lhsap = VN_AS(nodep->lhsp(), AttrOf);
            ধ্রুৱ AstAttrOf* ধ্রুৱ rhsap = VN_AS(nodep->rhsp(), AttrOf);
            UASSERT_OBJ(lhsap->attrType() == VAttrType::TYPEID, lhsap,
                        "Type compare expects type reference");
            UASSERT_OBJ(rhsap->attrType() == VAttrType::TYPEID, rhsap,
                        "Type compare expects type reference");
            ধ্রুৱ Ast_গ্ৰন্থি_DType* ধ্রুৱ lhsDtp = lhsap->dtypep();
            ধ্রুৱ Ast_গ্ৰন্থি_DType* ধ্রুৱ rhsDtp = rhsap->dtypep();
            UINFO(9, "==type lhsDtp " << lhsDtp);
            UINFO(9, "==type rhsDtp " << lhsDtp);
            ধ্রুৱ তার্কিক invert = VN_IS(nodep, NeqT);
            ধ্রুৱ তার্কিক identical
                = Ast_গ্ৰন্থি::computeCastable(lhsDtp, rhsDtp, nodep) == VCastable::SAMEISH;
            UINFO(9, "== " << identical);
            ধ্রুৱ তার্কিক eq = invert ^ identical;
            Ast_গ্ৰন্থি* ধ্রুৱ newp = নূতন AstConst{nodep->fileline(), AstConst::BitTrue{}, eq};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
        }
    }

    শূন্য visit_negate_not(Ast_গ্ৰন্থি_Uniop* nodep, তার্কিক real_ok) {
        // CALLER: (real_ok=false) Not
        // CALLER: (real_ok=true) Negate - allow real numbers
        // Signed: From lhs
        // IEEE-2012 Table 11-21:
        //    Widths: out width = lhs width
        UASSERT_OBJ(!nodep->op2p(), nodep, "For unary ops only!");
        assert_At_ৰাশিমালা(nodep);
        যদি (m_vup->prelim()) {
            userIterateAndNext(nodep->lhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            যদি (!real_ok) checkCvtUS(nodep->lhsp(), মিছা);
        }
        যদি (real_ok && nodep->lhsp()->isDouble()) {
            spliceCvtD(nodep->lhsp());
            যদি (Ast_গ্ৰন্থি_Uniop* ধ্রুৱ newp = replaceWithDVersion(nodep)) {
                VL_DANGLING(nodep);
                nodep = newp;  // Process new node instead
                iterateCheckReal(nodep, "LHS", nodep->lhsp(), BOTH);
                nodep->dtypeSetDouble();
                ঘূৰাই;
            }
        } অন্যথা {
            // Note there aren't yet uniops that need version changes
            // So no need to call replaceWithUOrSVersion(nodep, nodep->isSigned())
        }
        যদি (m_vup->prelim()) nodep->dtypeFrom(nodep->lhsp());
        যদি (m_vup->final()) {
            Ast_গ্ৰন্থি_DType* ধ্রুৱ expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
            nodep->dtypep(expDTypep);  // Propagate expression type to negation
            Ast_গ্ৰন্থি_DType* ধ্রুৱ subDTypep = expDTypep;
            // Some warning suppressions
            তার্কিক lhsWarn = সঁচা;
            যদি (VN_IS(nodep, Negate)) {
                // Warn if user wants extra bit from carry
                যদি (subDTypep->widthMin() == (nodep->lhsp()->widthMin() + 1)) lhsWarn = মিছা;
            }
            iterateCheck(nodep, "LHS", nodep->lhsp(), CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP,
                         lhsWarn);
        }
    }

    শূন্য visit_signed_unsigned(Ast_গ্ৰন্থি_Uniop* nodep, VSigning rs_out) {
        // CALLER: Signed, Unsigned
        // Width: lhs is self determined width
        // See IEEE-2012 6.24.1:
        //   Width: Returns packed array, of size $bits(expression).
        //   Sign: Output sign is as specified by operation
        //   TODO: Type: Two-state if input is two-state, else four-state
        UASSERT_OBJ(!nodep->op2p(), nodep, "For unary ops only!");
        assert_At_ৰাশিমালা(nodep);
        যদি (m_vup->prelim()) {
            userIterateAndNext(nodep->lhsp(), WidthVP{SELF, PRELIM}.p());
            checkCvtUS(nodep->lhsp(), সঁচা);
            ধ্রুৱ পূর্ণাঙ্ক width = nodep->lhsp()->width();
            Ast_গ্ৰন্থি_DType* ধ্রুৱ expDTypep = nodep->findBitOrLogicDType(
                width, width, rs_out, nodep->lhsp()->isFourstateOrNoDtype());
            nodep->dtypep(expDTypep);
            Ast_গ্ৰন্থি_DType* ধ্রুৱ subDTypep = expDTypep;
            // The child's width is self determined
            iterateCheck(nodep, "LHS", nodep->lhsp(), SELF, FINAL, subDTypep, EXTEND_EXP);
        }
    }

    শূন্য visit_shift(Ast_গ্ৰন্থি_Biop* nodep) {
        // CALLER: ShiftL, ShiftR, ShiftRS
        // Widths: Output width from lhs, rhs<33 bits
        // Signed: Output signed iff LHS signed; unary operator
        // See IEEE 2012 11.4.10:
        //   RHS is self-determined. RHS is always treated as unsigned, has no effect on result.
        iterate_shift_prelim(nodep);
        nodep->dtypeChgSigned(nodep->lhsp()->isSigned());
        ধ্রুৱ Ast_গ্ৰন্থি_Biop* ধ্রুৱ newp = iterate_shift_final(nodep);
        VL_DANGLING(nodep);
        (শূন্য)newp;  // Ununused
    }
    শূন্য iterate_shift_prelim(Ast_গ্ৰন্থি_Biop* nodep) {
        // Shifts
        // See IEEE-2012 11.4.10 and Table 11-21.
        //   RHS is self-determined. RHS is always treated as unsigned, has no effect on result.
        assert_At_ৰাশিমালা(nodep);
        যদি (m_vup->prelim()) {
            userIterateAndNext(nodep->lhsp(), WidthVP{SELF, PRELIM}.p());
            checkCvtUS(nodep->lhsp(), মিছা);
            iterateCheckSizedSelf(nodep, "RHS", nodep->rhsp(), SELF, BOTH);
            nodep->dtypeFrom(nodep->lhsp());
            যদি (!nodep->isFourstateOrNoDtype() && nodep->rhsp()->isFourstateOrNoDtype()) {
                nodep->dtypeSetLogicUnsized(nodep->width(), nodep->widthMin(),
                                            nodep->dtypep()->numeric());
            }
        }
    }
    Ast_গ্ৰন্থি_Biop* iterate_shift_final(Ast_গ্ৰন্থি_Biop* nodep) {
        // Nodep maybe edited
        assert_At_ৰাশিমালা(nodep);
        যদি (m_vup->final()) {
            Ast_গ্ৰন্থি_DType* ধ্রুৱ expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
            Ast_গ্ৰন্থি_DType* ধ্রুৱ subDTypep = expDTypep;
            nodep->dtypep(expDTypep);
            // ShiftRS converts to ShiftR, but not vice-versa
            যদি (VN_IS(nodep, ShiftRS)) {
                যদি (Ast_গ্ৰন্থি_Biop* ধ্রুৱ newp = replaceWithUOrSVersion(nodep, nodep->isSigned())) {
                    VL_DANGLING(nodep);
                    nodep = newp;  // Process new node instead
                }
            }
            তার্কিক warnOn = সঁচা;
            // No warning if "X = 1'b1<<N"; assume user is doing what they want
            যদি (nodep->lhsp()->isOne() && VN_IS(nodep->backp(), গ্ৰন্থি_Assign)) warnOn = মিছা;
            // We don't currently suppress these, as it's the upper operator (e.g. assign)
            // that reports the WIDTHEXPAND.
            AstConst* ধ্রুৱ shiftp = VN_CAST(nodep->rhsp(), Const);
            যদি (shiftp && !shiftp->অঙ্ক().isFourState() && shiftp->width() <= 32) {
                ধ্রুৱ int64_t shiftVal = shiftp->অঙ্ক().toSQuad();
                যদি (VN_IS(nodep, ShiftL)) {
                    যদি (shiftVal > 0 && nodep->width() == nodep->lhsp()->width() + shiftVal)
                        warnOn = মিছা;
                }
            }
            iterateCheck(nodep, "LHS", nodep->lhsp(), CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP,
                         warnOn);
            যদি (nodep->rhsp()->width() > 32) {
                যদি (shiftp && shiftp->অঙ্ক().mostSetBitP1() <= 32) {
                    // If (number)<<96'h1, then make it into (number)<<32'h1
                    V3_সংখ্যা অঙ্ক{shiftp, 32, shiftp->অঙ্ক()};
                    Ast_গ্ৰন্থি* ধ্রুৱ shiftrhsp = nodep->rhsp();
                    nodep->rhsp()->replaceWith(নূতন AstConst{shiftrhsp->fileline(), অঙ্ক});
                    VL_DO_DANGLING(shiftrhsp->delete_বৃক্ষ(), shiftrhsp);
                }
            }
        }
        ঘূৰাই nodep;  // May edit
    }

    শূন্য visit_boolexpr_and_or(Ast_গ্ৰন্থি_Biop* nodep) {
        // CALLER: And, Or, Xor, ...
        // Lint widths: out width = lhs width = rhs width
        // Signed: if lhs & rhs signed
        // IEEE-2012 Table 11-21:
        //    Width: max(LHS, RHS)
        UASSERT_OBJ(nodep->rhsp(), nodep, "For binary ops only!");
        // If errors are off, we need to follow the spec; thus we really need to do the max()
        // because the rhs could be larger, and we need to have proper editing to get the widths
        // to be the same for our operations.
        assert_At_ৰাশিমালা(nodep);
        যদি (m_vup->prelim()) {  // First stage evaluation
            // Determine expression widths only relying on what's in the subops
            userIterateAndNext(nodep->lhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            userIterateAndNext(nodep->rhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            checkCvtUS(nodep->lhsp(), মিছা);
            checkCvtUS(nodep->rhsp(), মিছা);
            ধ্রুৱ পূর্ণাঙ্ক width = std::max(nodep->lhsp()->width(), nodep->rhsp()->width());
            ধ্রুৱ পূর্ণাঙ্ক mwidth = std::max(nodep->lhsp()->widthMin(), nodep->rhsp()->widthMin());
            ধ্রুৱ তার্কিক expSigned = (nodep->lhsp()->isSigned() && nodep->rhsp()->isSigned());
            nodep->dtypeSetBitOrLogicUnsized(
                width, mwidth, VSigning::fromBool(expSigned),
                Ast_গ্ৰন্থি_ৰাশিমালা::isFourstateOrNoDtype(nodep->lhsp(), nodep->rhsp()));
        }
        যদি (m_vup->final()) {
            Ast_গ্ৰন্থি_DType* ধ্রুৱ expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
            Ast_গ্ৰন্থি_DType* ধ্রুৱ subDTypep = expDTypep;
            nodep->dtypep(expDTypep);
            // Error report and change sizes for suboperands of this node.
            iterateCheck(nodep, "LHS", nodep->lhsp(), CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP);
            iterateCheck(nodep, "RHS", nodep->rhsp(), CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP);
        }
    }

    শূন্য visit_add_sub_replace(Ast_গ্ৰন্থি_Biop* nodep, তার্কিক real_ok, ধ্রুৱ তার্কিক modOrDiv) {
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
        assert_At_ৰাশিমালা(nodep);
        যদি (m_vup->prelim()) {  // First stage evaluation
            // Determine expression widths only relying on what's in the subops
            userIterateAndNext(nodep->lhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            userIterateAndNext(nodep->rhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            যদি (!real_ok) {
                checkCvtUS(nodep->lhsp(), মিছা);
                checkCvtUS(nodep->rhsp(), মিছা);
            }
            যদি (nodep->lhsp()->isDouble() || nodep->rhsp()->isDouble()) {
                spliceCvtD(nodep->lhsp());
                spliceCvtD(nodep->rhsp());
                যদি (Ast_গ্ৰন্থি_Biop* ধ্রুৱ newp = replaceWithDVersion(nodep)) {
                    VL_DANGLING(nodep);
                    nodep = newp;  // Process new node instead
                }
                nodep->dtypeSetDouble();
                iterateCheckReal(nodep, "LHS", nodep->lhsp(), FINAL);
                iterateCheckReal(nodep, "RHS", nodep->rhsp(), FINAL);
                ঘূৰাই;
            } অন্যথা যদি (nodep->lhsp()->isString() || nodep->rhsp()->isString()) {
                nodep->v3error(
                    "Operator "
                    << nodep->prettyTypeName()
                    << " is not legal on string data types (IEEE 1800-2023 6.16)\n"
                    << (VN_IS(nodep, Add)
                            ? (nodep->warnMore()
                               + "... Suggest to concatenate strings use '{LHS, RHS, ...}'")
                            : ""));
                nodep->replaceWith(নূতন AstConst{nodep->fileline(), AstConst::String{}, ""});
                VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
                ঘূৰাই;
            } অন্যথা {
                ধ্রুৱ পূর্ণাঙ্ক width = std::max(nodep->lhsp()->width(), nodep->rhsp()->width());
                ধ্রুৱ পূর্ণাঙ্ক mwidth = std::max(nodep->lhsp()->widthMin(), nodep->rhsp()->widthMin());
                ধ্রুৱ তার্কিক expSigned = (nodep->lhsp()->isSigned() && nodep->rhsp()->isSigned());
                nodep->dtypeSetBitOrLogicUnsized(
                    width, mwidth, VSigning::fromBool(expSigned),
                    (modOrDiv && !nodep->rhsp()->isNeqZero())
                        || Ast_গ্ৰন্থি_ৰাশিমালা::isFourstateOrNoDtype(nodep->lhsp(), nodep->rhsp()));
            }
        }
        যদি (m_vup->final()) {
            // Parent's data type was computed using the max(upper, nodep->dtype)
            Ast_গ্ৰন্থি_DType* ধ্রুৱ expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
            Ast_গ্ৰন্থি_DType* ধ্রুৱ subDTypep = expDTypep;
            nodep->dtypep(expDTypep);
            // We don't use LHS && RHS -- unspecified language corner, see t_math_signed5 test
            // bool expSigned = (nodep->lhsp()->isSigned() && nodep->rhsp()->isSigned());
            যদি (Ast_গ্ৰন্থি_Biop* ধ্রুৱ newp = replaceWithUOrSVersion(nodep, expDTypep->isSigned())) {
                VL_DANGLING(nodep);
                nodep = newp;  // Process new node instead
            }
            // Some warning suppressions
            তার্কিক lhsWarn = সঁচা;
            তার্কিক rhsWarn = সঁচা;
            যদি (VN_IS(nodep, Add) || VN_IS(nodep, Sub)) {
                // Warn if user wants extra bit from carry
                যদি (subDTypep->widthMin() == (nodep->lhsp()->widthMin() + 1)) lhsWarn = মিছা;
                যদি (subDTypep->widthMin() == (nodep->rhsp()->widthMin() + 1)) rhsWarn = মিছা;
                যদি (VN_IS(nodep, Add) && nodep->lhsp()->width() == 1
                    && nodep->rhsp()->width() != 1)
                    lhsWarn = মিছা;  // do_increment + ...
                যদি (nodep->rhsp()->width() == 1 && nodep->lhsp()->width() != 1)
                    rhsWarn = মিছা;  // ... + do_increment
            } অন্যথা যদি (VN_IS(nodep, Mul) || VN_IS(nodep, MulS)) {
                যদি (subDTypep->widthMin() >= (nodep->lhsp()->widthMin())) lhsWarn = মিছা;
                যদি (subDTypep->widthMin() >= (nodep->rhsp()->widthMin())) rhsWarn = মিছা;
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
    শূন্য visit_real_add_sub(Ast_গ্ৰন্থি_Biop* nodep) {
        // CALLER: AddD, MulD, ...
        assert_At_ৰাশিমালা(nodep);
        যদি (m_vup->prelim()) {  // First stage evaluation
            // Note similar steps in visit_add_sub_replace promotion to double
            iterateCheckReal(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckReal(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtypeSetDouble();
        }
    }
    শূন্য visit_real_neg_ceil(Ast_গ্ৰন্থি_Uniop* nodep) {
        // CALLER: Negate, Ceil, Log, ...
        assert_At_ৰাশিমালা(nodep);
        যদি (m_vup->prelim()) {  // First stage evaluation
            // See alsl visit_negate_not conversion
            iterateCheckReal(nodep, "LHS", nodep->lhsp(), BOTH);
            nodep->dtypeSetDouble();
        }
    }

    শূন্য visit_sformatf_format(AstSFormatF* nodep) {
        // For sformatf's with constant format, iterate/check arguments
        UASSERT_OBJ(!nodep->ৰাশিমালা_Format_2(), nodep, "Assumes constant format");
        তার্কিক inPct = মিছা;
        Ast_গ্ৰন্থি_ৰাশিমালা* argp = nodep->exprsp();
        string newFormat;
        বাবে (ৱর্ণ ch : nodep->text()) {
            যদি (!inPct && ch == '%') {
                inPct = সঁচা;
                newFormat += ch;
            } অন্যথা যদি (inPct && (std::isdigit(ch) || ch == '.' || ch == '-')) {
                newFormat += ch;
            } অন্যথা যদি (!inPct) {  // Normal text
                newFormat += ch;
            } অন্যথা {
                inPct = মিছা;
                Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ nextp = argp ? VN_AS(argp->nextp(), গ্ৰন্থি_ৰাশিমালা) : nullptr;
                AstSFormatArg* ধ্রুৱ fargp = VN_CAST(argp, SFormatArg);  // May not exist yet
                Ast_গ্ৰন্থি_ৰাশিমালা* subargp = fargp ? fargp->exprp() : argp;
                ধ্রুৱ Ast_গ্ৰন্থি_DType* ধ্রুৱ dtypep
                    = subargp ? subargp->dtypep()->skipRefp() : nullptr;
                ch = std::tolower(ch);
                ৱিকল্প (ch) {
                পক্ষ '?':  // Unspecified by user, guess
                    যদি (dtypep->isDouble()) {
                        ch = 'g';
                    } অন্যথা যদি (dtypep->isString()) {
                        ch = 's';
                    } অন্যথা যদি (VN_IS(dtypep, BasicDType)) {
                        ch = nodep->missingArgChar();
                    } অন্যথা {
                        ch = 'p';
                    }
                    argp = nextp;
                    ভঙ্গ;
                পক্ষ '%': ভঙ্গ;  // %% - just output a %
                পক্ষ 'm': ভঙ্গ;  // %m - auto insert "name"
                পক্ষ 'l': ভঙ্গ;  // %m - auto insert "library"
                পক্ষ 'd':
                    যদি (subargp) {
                        checkFormatNumericArg(subargp, ch);
                        যদি (subargp->isDouble()) spliceCvtS(subargp, সঁচা, 64);
                        argp = nextp;
                    }
                    ভঙ্গ;
                পক্ষ 'b':  // FALLTHRU
                পক্ষ 'o':  // FALLTHRU
                পক্ষ 'x':
                    যদি (subargp) {
                        // V3Randomize may make a %x on a string, or may
                        // have a runtime format which gets past this error
                        checkFormatNumericArg(subargp, ch);
                        argp = nextp;
                    }
                    ভঙ্গ;
                পক্ষ 't':  // Decimal/float time
                    যদি (subargp) {
                        checkFormatNumericArg(subargp, ch);
                        UASSERT_OBJ(!nodep->timeunit().isNone(), nodep,
                                    "display %t has no time units");
                        argp = nextp;
                    }
                    ভঙ্গ;
                পক্ষ 'e':  // FALLTHRU
                পক্ষ 'f':  // FALLTHRU
                পক্ষ 'g':
                    যদি (subargp) {
                        checkFormatNumericArg(subargp, ch);
                        যদি (!subargp->isDouble()) {
                            iterateCheckReal(nodep, "Display argument", subargp, BOTH);
                        }
                        argp = nextp;
                    }
                    ভঙ্গ;
                পক্ষ 'p':  // FALLTHRU
                পক্ষ 's':
                    // As with enum.name(): valid values print the mnemonic, else numeric
                    যদি (subargp && !fargp) {
                        যদি (AstEnumDType* ধ্রুৱ enumDtp = formatEnumDType(subargp)) {
                            VNRelinker relinker;
                            subargp->unlinkFrBack(&relinker);
                            relinker.relink(newFormatEnumArg(subargp, enumDtp));
                        }
                    }
                    argp = nextp;
                    ভঙ্গ;
                স্ৱভাৱ:  // Most operators, just move to next argument
                    argp = nextp;
                    ভঙ্গ;
                }  // switch
                newFormat += ch;
            }
        }
        nodep->text(newFormat);
    }

    স্থির AstEnumDType* formatEnumDType(Ast_গ্ৰন্থি_ৰাশিমালা* subargp) {
        AstEnumDType* enumDtp = VN_CAST(subargp->dtypep()->skipRefToEnump(), EnumDType);
        যদি (!enumDtp) {
            যদি (ধ্রুৱ AstVarRef* ধ্রুৱ varrefp = VN_CAST(subargp, VarRef)) {
                enumDtp = VN_CAST(varrefp->varp()->dtypep()->skipRefToEnump(), EnumDType);
            }
        }
        ঘূৰাই enumDtp;
    }

    Ast_ৰাশিমালা_Stmt* newEnumCapture(Ast_গ্ৰন্থি_ৰাশিমালা* valuep, AstEnumDType* dtypep) {
        File_সাৰি* ধ্রুৱ flp = valuep->fileline();
        AstVar* ধ্রুৱ varp = নূতন AstVar{flp, VVarType::XTEMP, m_enumTempNames.get(valuep), dtypep};
        varp->noSubst(সঁচা);
        যদি (m_ftaskp) {
            varp->funcLocal(সঁচা);
            varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            // Declare before use, including in non-inlined class methods.
            m_ftaskp->prependStmtsp(varp);
        } অন্যথা {
            UASSERT_OBJ(m_modep, valuep, "Enum expression without a containing module");
            m_modep->addStmtsp(varp);
        }
        iterate(varp);
        Ast_ৰাশিমালা_Stmt* ধ্রুৱ exprp = নূতন Ast_ৰাশিমালা_Stmt{
            flp, নূতন AstAssign{flp, নূতন AstVarRef{flp, varp, VAccess::WRITE}, valuep},
            নূতন AstVarRef{flp, varp, VAccess::READ}};
        iterate(exprp);
        ঘূৰাই exprp;
    }

    AstSFormatArg* newFormatEnumArg(Ast_গ্ৰন্থি_ৰাশিমালা* valuep, AstEnumDType* dtypep) {
        যদি (valuep->isPure()) valuep = V3Const::constifyEdit(valuep);
        Ast_গ্ৰন্থি_ৰাশিমালা* keyp;
        যদি (VN_IS(valuep, Const) || VN_IS(valuep, VarRef) || (!m_modep && !m_ftaskp)) {
            // Standalone parameter expressions are checked for constness by V3Const.
            keyp = valuep->clone_বৃক্ষ(মিছা);
        } অন্যথা {
            Ast_ৰাশিমালা_Stmt* ধ্রুৱ capturep = newEnumCapture(valuep, dtypep);
            valuep = capturep;
            keyp = capturep->resultp()->clone_বৃক্ষ_Pure(মিছা);
        }
        ধ্রুৱ VFormatAttr attr = dtypep->isSigned() ? VFormatAttr::ENUM_SIGNED : VFormatAttr::ENUM;
        AstSFormatArg* ধ্রুৱ argp = নূতন AstSFormatArg{valuep->fileline(), attr, valuep};
        argp->dtypep(dtypep);
        argp->namep(enumSelect(keyp, dtypep, VAttrType::ENUM_NAME));
        ঘূৰাই argp;
    }

    //----------------------------------------------------------------------
    // LOWER LEVEL WIDTH METHODS  (none iterate)

    তার্কিক widthBad(Ast_গ্ৰন্থি* nodep, Ast_গ্ৰন্থি_DType* expDTypep) {
        ধ্রুৱ পূর্ণাঙ্ক expWidth = expDTypep->width();
        পূর্ণাঙ্ক expWidthMin = expDTypep->widthMin();
        UASSERT_OBJ(nodep->dtypep(), nodep,
                    "Under node " << nodep->prettyTypeName()
                                  << " has no dtype?? Missing Visitor func?");
        যদি (expDTypep->basicp()->untyped() || nodep->dtypep()->basicp()->untyped()) ঘূৰাই মিছা;
        // During DepGraph execution, expected width may be 0 if the type hasn't been
        UASSERT_OBJ(nodep->width() != 0, nodep,
                    "Under node " << nodep->prettyTypeName()
                                  << " has no expected width?? Missing Visitor func?");
        UASSERT_OBJ(expWidth != 0, nodep,
                    "Node " << nodep->prettyTypeName()
                            << " has no expected width?? Missing Visitor func?");
        যদি (expWidthMin == 0) expWidthMin = expWidth;
        যদি (nodep->dtypep()->width() == expWidth) ঘূৰাই মিছা;
        যদি (nodep->dtypep()->widthSized() && nodep->width() != expWidthMin) ঘূৰাই সঁচা;
        যদি (!nodep->dtypep()->widthSized() && nodep->widthMin() > expWidthMin) ঘূৰাই সঁচা;
        ঘূৰাই মিছা;
    }

    শূন্য fixWidthExtend(Ast_গ্ৰন্থি_ৰাশিমালা* nodep, Ast_গ্ৰন্থি_DType* expDTypep, ExtendRule extendRule) {
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
        UINFO(4, "  widthExtend_(r=" << static_cast<পূর্ণাঙ্ক>(extendRule) << ") old: " << nodep);
        যদি (extendRule == EXTEND_OFF) ঘূৰাই;
        AstConst* ধ্রুৱ constp = VN_CAST(nodep, Const);
        ধ্রুৱ পূর্ণাঙ্ক expWidth = expDTypep->width();
        যদি (constp && !constp->অঙ্ক().isNegative()) {
            // Save later constant propagation work, just right-size it.
            V3_সংখ্যা অঙ্ক{nodep, expWidth, constp->অঙ্ক()};
            অঙ্ক.isSigned(মিছা);
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ newp = নূতন AstConst{nodep->fileline(), অঙ্ক};
            constp->replaceWith(newp);
            VL_DO_DANGLING(ঠেলো_Deletep(constp), constp);
            VL_DANGLING(nodep);
            nodep = newp;
        } অন্যথা যদি (expWidth < nodep->width()) {
            // Trunc - Extract
            VNRelinker linker;
            nodep->unlinkFrBack(&linker);
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ newp = নূতন AstSel{nodep->fileline(), nodep, 0, expWidth};
            newp->didWidth(সঁচা);  // Don't replace dtype with unsigned
            linker.relink(newp);
            nodep = newp;
        } অন্যথা {
            // Extend
            VNRelinker linker;
            nodep->unlinkFrBack(&linker);
            তার্কিক doSigned = মিছা;
            ৱিকল্প (extendRule) {
            পক্ষ EXTEND_ZERO: doSigned = মিছা; ভঙ্গ;
            পক্ষ EXTEND_EXP: doSigned = nodep->isSigned() && expDTypep->isSigned(); ভঙ্গ;
            পক্ষ EXTEND_LHS: doSigned = nodep->isSigned(); ভঙ্গ;
            স্ৱভাৱ: nodep->v3fatalSrc("bad case");
            }
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ newp
                = (doSigned ? static_cast<Ast_গ্ৰন্থি_ৰাশিমালা*>(নূতন AstExtendS{nodep->fileline(), nodep})
                            : static_cast<Ast_গ্ৰন্থি_ৰাশিমালা*>(নূতন AstExtend{nodep->fileline(), nodep}));
            newp->didWidth(সঁচা);
            linker.relink(newp);
            nodep = newp;
        }
        যদি (expDTypep->isDouble() && !nodep->isDouble()) {
            // For AstVar init() among others
            // TODO do all to-real and to-integer conversions in this function
            // rather than in callers
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ newp = spliceCvtD(nodep);
            nodep = newp;
        }
        যদি (expDTypep->skipRefp()->isIntegralOrPacked()) {
            nodep->dtypeChgWidthSigned(expWidth, expDTypep->widthMin(), expDTypep->numeric());
        } অন্যথা {
            nodep->dtypeFrom(expDTypep);
        }
        UINFO(4, "             _new: " << nodep);
    }

    শূন্য fixWidthReduce(Ast_গ্ৰন্থি_ৰাশিমালা* nodep) {
        // Fix the width mismatch by adding a reduction OR operator
        // IF (A(CONSTwide)) becomes IF (A(CONSTreduced))
        // IF (A(somewide)) on pointer becomes on pointer IF (A(NEQ(somewide, Null)))
        // IF (A(somewide)) elsewise becomes IF (A(REDOR(somewide, 0)))
        // Attempt to fix it quietly
        ধ্রুৱ পূর্ণাঙ্ক expWidth = 1;
        ধ্রুৱ পূর্ণাঙ্ক expSigned = মিছা;
        যদি (nodep->width() == expWidth) ঘূৰাই;
        UINFO(4, "  widthReduce_old: " << nodep);
        AstConst* ধ্রুৱ constp = VN_CAST(nodep, Const);
        যদি (constp) {
            V3_সংখ্যা অঙ্ক(nodep, expWidth);
            অঙ্ক.opRedOr(constp->অঙ্ক());
            অঙ্ক.isSigned(expSigned);
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ newp = নূতন AstConst{nodep->fileline(), অঙ্ক};
            constp->replaceWith(newp);
            VL_DO_DANGLING(constp->delete_বৃক্ষ(), constp);
            VL_DANGLING(nodep);
            nodep = newp;
        } অন্যথা {
            VNRelinker linker;
            nodep->unlinkFrBack(&linker);
            Ast_গ্ৰন্থি_ৰাশিমালা* newp;
            Ast_গ্ৰন্থি_DType* ধ্রুৱ dtypeSkipp = nodep->dtypep()->skipRefp();
            যদি (VN_IS(dtypeSkipp, ClassRefDType) || VN_IS(dtypeSkipp, IfaceRefDType)) {
                // Cannot use AstRedOr, as we may be redurcing a class handle
                newp = নূতন AstNeq{nodep->fileline(),
                                  নূতন AstConst{nodep->fileline(), AstConst::Null{}},

                                  nodep};
            } অন্যথা {
                // But, AstRedOr can be faster on wide
                newp = নূতন AstRedOr{nodep->fileline(), nodep};
            }
            linker.relink(newp);
            nodep = newp;
        }
        nodep->dtypeChgWidthSigned(expWidth, expWidth, VSigning::fromBool(expSigned));
        UINFO(4, "             _new: " << nodep);
    }

    তার্কিক fixAutoExtend(Ast_গ্ৰন্থি_ৰাশিমালা*& nodepr, পূর্ণাঙ্ক expWidth) {
        // For SystemVerilog '0,'1,'x,'z, autoextend and don't warn
        যদি (AstConst* ধ্রুৱ constp = VN_CAST(nodepr, Const)) {
            যদি (constp->অঙ্ক().autoExtend() && !constp->অঙ্ক().sized() && constp->width() == 1) {
                // Make it the proper size.  Careful of proper extension of 0's/1's
                V3_সংখ্যা অঙ্ক(constp, expWidth);
                অঙ্ক.opRepl(constp->অঙ্ক(), expWidth);  // {width{'1}}
                Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ newp = নূতন AstConst{constp->fileline(), অঙ্ক};
                // Spec says always unsigned with proper width
                UINFOTREE(5, constp, "", "fixAutoExtend_old");
                UINFOTREE(5, newp, "", "_new");
                constp->replaceWith(newp);
                VL_DO_DANGLING(constp->delete_বৃক্ষ(), constp);
                // Tell caller the new constp, and that we changed it.
                nodepr = newp;
                ঘূৰাই সঁচা;
            }
            // X/Z also upper bit extend.  In pre-SV only to 32-bits, SV forever.
            অন্যথা যদি (!constp->অঙ্ক().sized()
                     // Make it the proper size.  Careful of proper extension of 0's/1's
                     && expWidth > 32 && constp->অঙ্ক().isMsbXZ()) {
                constp->v3warn(WIDTHXZEXPAND, "Unsized constant being X/Z extended to "
                                                  << expWidth
                                                  << " bits: " << constp->prettyName());
                V3_সংখ্যা অঙ্ক(constp, expWidth);
                অঙ্ক.opExtendXZ(constp->অঙ্ক(), constp->width());
                Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ newp = নূতন AstConst{constp->fileline(), অঙ্ক};
                // Spec says always unsigned with proper width
                UINFOTREE(5, constp, "", "fixUnszExtend_old");
                UINFOTREE(5, newp, "", "_new");
                constp->replaceWith(newp);
                VL_DO_DANGLING(constp->delete_বৃক্ষ(), constp);
                // Tell caller the new constp, and that we changed it.
                nodepr = newp;
                ঘূৰাই সঁচা;
            }
        }
        ঘূৰাই মিছা;  // No change
    }
    তার্কিক isBaseClassRecurse(ধ্রুৱ AstClass* ধ্রুৱ cls1p, ধ্রুৱ AstClass* ধ্রুৱ cls2p) {
        // Returns true if cls1p and cls2p are equal or if cls1p is a base class of cls2p
        যদি (cls1p == cls2p) ঘূৰাই সঁচা;
        বাবে (ধ্রুৱ AstClassExtends* cextp = cls2p->extendsp(); cextp;
             cextp = VN_CAST(cextp->nextp(), ClassExtends)) {
            যদি (isBaseClassRecurse(cls1p, cextp->classp())) ঘূৰাই সঁচা;
        }
        ঘূৰাই মিছা;
    }
    // Lines to end a message on nodep with the data types of its operands
    স্থির string warnDTypes(ধ্রুৱ Ast_গ্ৰন্থি* nodep, ধ্রুৱ Ast_গ্ৰন্থি_DType* lhsDtp,
                             ধ্রুৱ Ast_গ্ৰন্থি_DType* rhsDtp, ধ্রুৱ string& lhsName = "Left-hand",
                             ধ্রুৱ string& rhsName = "Right-hand")
        VL_REQUIRES(V3Error::s().m_mutex) {
        string lhsState = " " + lhsDtp->stateDTypeName();
        string rhsState = " " + rhsDtp->stateDTypeName();
        যদি (lhsState == rhsState) lhsState = rhsState = "";
        ঘূৰাই "\n" + nodep->warnMore() + "... " + lhsName
               + " data type: " + lhsDtp->prettyDTypeNameQ() + lhsState + "\n" + nodep->warnMore()
               + "... " + rhsName + " data type: " + rhsDtp->prettyDTypeNameQ() + rhsState;
    }
    // Fixed-size unpacked arrays are assignment compatible only with the same number of
    // elements in each dimension (IEEE 1800-2023 7.6); dynamically sized ones always are
    স্থির তার্কিক unpackedArraySizesMatch(ধ্রুৱ Ast_গ্ৰন্থি_DType* lhsDtp, ধ্রুৱ Ast_গ্ৰন্থি_DType* rhsDtp) {
        যেতিয়ালৈ (lhsDtp->isNonPackedArray() && rhsDtp->isNonPackedArray()) {
            ধ্রুৱ AstUnpackArrayDType* ধ্রুৱ lhsArrayp = VN_CAST(lhsDtp, UnpackArrayDType);
            ধ্রুৱ AstUnpackArrayDType* ধ্রুৱ rhsArrayp = VN_CAST(rhsDtp, UnpackArrayDType);
            যদি (lhsArrayp && rhsArrayp
                && lhsArrayp->elementsConst() != rhsArrayp->elementsConst()) {
                ঘূৰাই মিছা;
            }
            lhsDtp = lhsDtp->subDTypep()->skipRefp();
            rhsDtp = rhsDtp->subDTypep()->skipRefp();
        }
        ঘূৰাই সঁচা;
    }
    // Checks whether two types are assignment-compatible according to IEEE 1800-2023 7.6
    // Currently, this function only supports variables, which are of following types:
    // - Fixed-size unpacked array
    // - Dynamic unpacked array
    // - Associative array
    template <typename T, typename N>
    শূন্য checkUnpackedArrayAssignmentCompatible(ধ্রুৱ Ast_গ্ৰন্থি* nodep, ধ্রুৱ T* ধ্রুৱ lhsRefp,
                                                ধ্রুৱ N* ধ্রুৱ rhsRefp) {
        static_assert(
            (std::is_same<T, AstVar>::মান || std::is_same<T, Ast_গ্ৰন্থি_Var_Ref>::মান)
                && (std::is_same<N, AstVar>::মান || std::is_same<N, Ast_গ্ৰন্থি_Var_Ref>::মান),
            "Unsupported types provided.");
        যদি (!lhsRefp || !rhsRefp) ঘূৰাই;
        std::string lhsName;
        std::string rhsName;
        যদি (VN_IS(nodep, Pin)) {
            lhsName = std::string{"Pin"};
            rhsName = std::string{"Expression"};
        } অন্যথা {
            lhsName = std::string{"Left-hand"};
            rhsName = std::string{"Right-hand"};
        }

        ধ্রুৱ Ast_গ্ৰন্থি_DType* ধ্রুৱ lhsDtp = lhsRefp->dtypep()->skipRefp();
        ধ্রুৱ Ast_গ্ৰন্থি_DType* ধ্রুৱ rhsDtp = rhsRefp->dtypep()->skipRefp();
        ধ্রুৱ তার্কিক isLhsAggregate = lhsDtp->isAggregateType();
        ধ্রুৱ তার্কিক isRhsAggregate = rhsDtp->isAggregateType();
        যদি (!isLhsAggregate && !isRhsAggregate) ঘূৰাই;
        যদি (isLhsAggregate ^ isRhsAggregate) {
            nodep->v3error(
                "Illegal assignment: types are not assignment compatible (IEEE 1800-2023 7.6)"
                << warnDTypes(nodep, lhsDtp, rhsDtp, lhsName, rhsName));
            ঘূৰাই;
        } অন্যথা যদি (VN_IS(lhsDtp, QueueDType) && VN_IS(rhsDtp, EmptyQueueDType)) {
            ঘূৰাই;
        }
        std::pair<uint32_t, uint32_t> lhsDim = lhsDtp->dimensions(মিছা),
                                      rhsDim = rhsDtp->dimensions(মিছা);
        // Check if unpacked array dimensions are matching
        যদি (lhsDim.second != rhsDim.second) {
            nodep->v3error("Illegal assignment: Unmatched number of unpacked dimensions (IEEE "
                           "1800-2023 7.6)"
                           << warnDTypes(nodep, lhsDtp, rhsDtp, lhsName, rhsName));
            ঘূৰাই;
        }

        ধ্রুৱ Ast_গ্ৰন্থি_DType* lhsDtpIterp = lhsDtp;
        ধ্রুৱ Ast_গ্ৰন্থি_DType* rhsDtpIterp = rhsDtp;
        বাবে (uint32_t dim = 0; dim < rhsDim.second; dim++) {
            // Associative arrays are compatible only with each other
            যদি (VN_IS(lhsDtpIterp, AssocArrayDType) ^ VN_IS(rhsDtpIterp, AssocArrayDType)) {
                nodep->v3error("Illegal assignment: Associative arrays are assignment compatible "
                               "only with associative arrays (IEEE 1800-2023 7.6)"
                               << warnDTypes(nodep, lhsDtp, rhsDtp, lhsName, rhsName));
                ঘূৰাই;
            }
            lhsDtpIterp = lhsDtpIterp->subDTypep()->skipRefp();
            rhsDtpIterp = rhsDtpIterp->subDTypep()->skipRefp();
        }
        // Element types of source and target shall be equivalent
        যদি (!isEquivalentDType(lhsDtpIterp, rhsDtpIterp)) {
            nodep->v3error("Illegal assignment: Array element types are not equivalent (IEEE "
                           "1800-2023 6.22.2)"
                           << warnDTypes(nodep, lhsDtp, rhsDtp, lhsName, rhsName));
        }
    }
    শূন্য checkClassAssign(ধ্রুৱ Ast_গ্ৰন্থি* nodep, ধ্রুৱ ৱর্ণ* side, Ast_গ্ৰন্থি* rhsp,
                          Ast_গ্ৰন্থি_DType* ধ্রুৱ lhsDTypep, তার্কিক isOutputArg = মিছা) {
        UASSERT_OBJ(rhsp->dtypep(), rhsp, "Node has no type");
        ধ্রুৱ Ast_গ্ৰন্থি_DType* ধ্রুৱ lhsRawDTypep = lhsDTypep->skipRefp();
        ধ্রুৱ Ast_গ্ৰন্থি_DType* ধ্রুৱ rhsRawDTypep = rhsp->dtypep()->skipRefp();
        যদি (ধ্রুৱ AstClassRefDType* ধ্রুৱ lhsClassRefp = VN_CAST(lhsRawDTypep, ClassRefDType)) {
            যদি (ধ্রুৱ AstClassRefDType* ধ্রুৱ rhsClassRefp
                = VN_CAST(rhsRawDTypep, ClassRefDType)) {
                // Output arg swaps sides: caller actual = callee formal at return.
                ধ্রুৱ AstClass* ধ্রুৱ dstp
                    = isOutputArg ? rhsClassRefp->classp() : lhsClassRefp->classp();
                ধ্রুৱ AstClass* ধ্রুৱ srcp
                    = isOutputArg ? lhsClassRefp->classp() : rhsClassRefp->classp();
                যদি (isBaseClassRecurse(dstp, srcp)) ঘূৰাই;
            } অন্যথা যদি (rhsp->isNull()) {
                ঘূৰাই;
            }
            nodep->v3error(side << " expects a " << lhsClassRefp->prettyTypeName() << ", got "
                                << rhsRawDTypep->prettyTypeName());
        } অন্যথা যদি (VN_IS(rhsRawDTypep, ClassRefDType)) {
            nodep->v3error(side << " " << rhsRawDTypep->prettyDTypeNameQ()
                                << " cannot be assigned to non-class "
                                << lhsDTypep->prettyDTypeNameQ());
        }
        যদি (VN_IS(lhsRawDTypep, DynArrayDType) && VN_IS(rhsRawDTypep, UnpackArrayDType)) {
            VNRelinker relinker;
            rhsp->unlinkFrBack(&relinker);
            relinker.relink(
                নূতন AstCvtUnpackToQueue{rhsp->fileline(), VN_AS(rhsp, গ্ৰন্থি_ৰাশিমালা), lhsDTypep});
        }
    }
    স্থির তার্কিক similarDTypeRecurse(ধ্রুৱ Ast_গ্ৰন্থি_DType* ধ্রুৱ node1p,
                                    ধ্রুৱ Ast_গ্ৰন্থি_DType* ধ্রুৱ node2p) {
        ঘূৰাই node1p->skipRefp()->similarDType(node2p->skipRefp());
    }
    শূন্য iterateCheckFileDesc(Ast_গ্ৰন্থি* parentp, Ast_গ্ৰন্থি* underp, Stage stage) {
        UASSERT_OBJ(stage == BOTH, parentp, "Bad call");
        // underp may change as a result of replacement
        underp = userIterateSubtreeReturnEdits(underp, WidthVP{SELF, PRELIM}.p());
        Ast_গ্ৰন্থি_DType* ধ্রুৱ expDTypep = underp->findUInt32DType();
        underp
            = iterateCheck(parentp, "file_descriptor", underp, SELF, FINAL, expDTypep, EXTEND_EXP);
        (শূন্য)underp;  // cppcheck
    }
    শূন্য iterateCheckReal(Ast_গ্ৰন্থি* parentp, ধ্রুৱ ৱর্ণ* side, Ast_গ্ৰন্থি* underp, Stage stage) {
        // Coerce child to real if not already. Child is self-determined
        // e.g. parentp=ADDD, underp=ADD in ADDD(ADD(a,b), real-CONST)
        // Don't need separate PRELIM and FINAL(double) calls;
        // as if resolves to double, the BOTH correctly resolved double,
        // otherwise self-determined was correct
        iterateCheckTypedSelfPrelim(parentp, side, underp, parentp->findDoubleDType(), stage);
    }
    শূন্য iterateCheckSigned8(Ast_গ্ৰন্থি* parentp, ধ্রুৱ ৱর্ণ* side, Ast_গ্ৰন্থি* underp, Stage stage) {
        // Coerce child to signed8 if not already. Child is self-determined
        iterateCheckTypedSelfPrelim(parentp, side, underp, parentp->findSigned8DType(), stage);
    }
    শূন্য iterateCheckSigned32(Ast_গ্ৰন্থি* parentp, ধ্রুৱ ৱর্ণ* side, Ast_গ্ৰন্থি* underp, Stage stage) {
        // Coerce child to signed32 if not already. Child is self-determined
        iterateCheckTypedSelfPrelim(parentp, side, underp, parentp->findIntDType(), stage);
    }
    শূন্য iterateCheckUInt32(Ast_গ্ৰন্থি* parentp, ধ্রুৱ ৱর্ণ* side, Ast_গ্ৰন্থি* underp, Stage stage) {
        // Coerce child to unsigned32 if not already. Child is self-determined
        iterateCheckTypedSelfPrelim(parentp, side, underp, parentp->findUInt32DType(), stage);
    }
    শূন্য iterateCheckDelay(Ast_গ্ৰন্থি* parentp, ধ্রুৱ ৱর্ণ* side, Ast_গ্ৰন্থি* underp, Stage stage) {
        // Coerce child to 64-bit delay if not already. Child is self-determined
        // underp may change as a result of replacement
        যদি (stage & PRELIM) {
            underp = userIterateSubtreeReturnEdits(underp, WidthVP{SELF, PRELIM}.p());
        }
        যদি (stage & FINAL) {
            Ast_গ্ৰন্থি_DType* expDTypep;
            যদি (underp->dtypep()->skipRefp()->isDouble()) {  // V3Timing will later convert double
                expDTypep = parentp->findDoubleDType();
            } অন্যথা {
                File_সাৰি* ধ্রুৱ newFl = নূতন File_সাৰি{underp->fileline()};
                newFl->warnOff(V3ErrorCode::WIDTHEXPAND, সঁচা);
                underp->fileline(newFl);
                expDTypep = parentp->findBitDType(64, 64, VSigning::UNSIGNED);
            }
            underp
                = iterateCheck(parentp, side, underp, SELF, FINAL, expDTypep, EXTEND_EXP, মিছা);
        }
        (শূন্য)underp;  // cppcheck
    }
    শূন্য iterateCheckString(Ast_গ্ৰন্থি* parentp, ধ্রুৱ ৱর্ণ* side, Ast_গ্ৰন্থি* underp, Stage stage) {
        iterateCheckTyped(parentp, side, underp, parentp->findStringDType(), stage);
    }
    শূন্য iterateCheckTyped(Ast_গ্ৰন্থি* parentp, ধ্রুৱ ৱর্ণ* side, Ast_গ্ৰন্থি* underp,
                           Ast_গ্ৰন্থি_DType* expDTypep, Stage stage) {
        যদি (stage & PRELIM) {
            underp = userIterateSubtreeReturnEdits(underp, WidthVP{expDTypep, PRELIM}.p());
        }
        যদি (stage & FINAL) {
            underp = iterateCheck(parentp, side, underp, SELF, FINAL, expDTypep, EXTEND_EXP);
        }
        (শূন্য)underp;  // cppcheck
    }
    শূন্য iterateCheckTypedSelfPrelim(Ast_গ্ৰন্থি* parentp, ধ্রুৱ ৱর্ণ* side, Ast_গ্ৰন্থি* underp,
                                     Ast_গ্ৰন্থি_DType* expDTypep, Stage stage) {
        // underp may change as a result of replacement
        যদি (stage & PRELIM) {
            underp = userIterateSubtreeReturnEdits(underp, WidthVP{SELF, PRELIM}.p());
        }
        যদি (stage & FINAL) {
            underp = iterateCheck(parentp, side, underp, SELF, FINAL, expDTypep, EXTEND_EXP);
        }
        (শূন্য)underp;  // cppcheck
    }
    শূন্য iterateCheckIntegralSelf(Ast_গ্ৰন্থি* parentp, ধ্রুৱ ৱর্ণ* side, Ast_গ্ৰন্থি* underp,
                                  Determ determ, Stage stage) {
        iterateCheckSelf(parentp, side, underp, determ, stage, সঁচা);
    }
    শূন্য iterateCheckSelf(Ast_গ্ৰন্থি* parentp, ধ্রুৱ ৱর্ণ* side, Ast_গ্ৰন্থি* underp, Determ determ,
                          Stage stage, তার্কিক integralOnly = মিছা) {
        // Coerce child to any data type; child is self-determined
        // i.e. isolated from expected type.
        // e.g. parentp=CONCAT, underp=lhs in CONCAT(lhs,rhs)
        UASSERT_OBJ(determ == SELF, parentp, "Bad call");
        UASSERT_OBJ(stage == FINAL || stage == BOTH, parentp, "Bad call");
        // underp may change as a result of replacement
        যদি (stage & PRELIM) {
            underp = userIterateSubtreeReturnEdits(underp, WidthVP{SELF, PRELIM}.p());
        }
        underp
            = VN_IS(underp, গ্ৰন্থি_ৰাশিমালা) ? checkCvtUS(VN_AS(underp, গ্ৰন্থি_ৰাশিমালা), integralOnly) : underp;
        Ast_গ্ৰন্থি_DType* ধ্রুৱ expDTypep = underp->dtypep();
        underp = iterateCheck(parentp, side, underp, SELF, FINAL, expDTypep, EXTEND_EXP);
        (শূন্য)underp;  // cppcheck
    }
    শূন্য iterateCheckSizedSelf(Ast_গ্ৰন্থি* parentp, ধ্রুৱ ৱর্ণ* side, Ast_গ্ৰন্থি* underp, Determ determ,
                               Stage stage, তার্কিক deferUntyped = মিছা) {
        // Coerce child to any sized-number data type; child is self-determined
        // i.e. isolated from expected type.
        // e.g. parentp=CONCAT, underp=lhs in CONCAT(lhs,rhs)
        UASSERT_OBJ(determ == SELF, parentp, "Bad call");
        UASSERT_OBJ(stage == FINAL || stage == BOTH, parentp, "Bad call");
        // underp may change as a result of replacement
        যদি (stage & PRELIM) {
            underp = userIterateSubtreeReturnEdits(underp, WidthVP{SELF, PRELIM}.p());
        }
        underp = VN_IS(underp, গ্ৰন্থি_ৰাশিমালা) ? checkCvtUS(VN_AS(underp, গ্ৰন্থি_ৰাশিমালা), মিছা) : underp;
        Ast_গ্ৰন্থি_DType* ধ্রুৱ expDTypep = underp->dtypep();
        underp = iterateCheck(parentp, side, underp, SELF, FINAL, expDTypep, EXTEND_EXP);
        Ast_গ্ৰন্থি_DType* ধ্রুৱ checkDtp = expDTypep->skipRefToEnump();
        // Sampling functions may still reference an untyped property formal here.
        // V3AssertPre checks the actual type after property argument substitution.
        যদি (!checkDtp->isIntegralOrPacked()
            && !(deferUntyped && checkDtp->basicp() && checkDtp->basicp()->untyped())) {
            parentp->v3error(ucfirst(parentp->prettyOperatorName())
                             << " expects a numeric " << side << " operand, but it has data type "
                             << checkDtp->prettyDTypeNameQ() << " (IEEE 1800-2023 11.3)");
        }
        (শূন্য)underp;  // cppcheck
    }
    শূন্য iterateCheckAssign(Ast_গ্ৰন্থি* parentp, ধ্রুৱ ৱর্ণ* side, Ast_গ্ৰন্থি* rhsp, Stage stage,
                            Ast_গ্ৰন্থি_DType* lhsDTypep, VAccess rhsAccess = VAccess::READ) {
        // Check using assignment-like context rules
        // UINFOTREE(1, parentp, "", "checkass");
        UASSERT_OBJ(stage == FINAL, parentp, "Bad width call");
        // Create unpacked byte from string see IEEE 1800-2023 5.9
        যদি (AstConst* constp = VN_CAST(rhsp, Const)) {
            যদি (ধ্রুৱ AstUnpackArrayDType* ধ্রুৱ arrayp
                = VN_CAST(lhsDTypep->skipRefp(), UnpackArrayDType)) {
                যদি (AstBasicDType* basicp = VN_CAST(arrayp->subDTypep()->skipRefp(), BasicDType)) {
                    যদি (basicp->width() == 8 && constp->অঙ্ক().isFromString()) {
                        AstConsArray* newp = নূতন AstConsArray{
                            constp->fileline(), lhsDTypep,
                            নূতন AstConst{constp->fileline(), AstConst::Widthed_মান{}, 8, 0}};
                        বাবে (ধ্রুৱ পূর্ণাঙ্ক aindex : arrayp->declRange().seqLoToHi()) {
                            পূর্ণাঙ্ক cindex = arrayp->declRange().ascending() ? (arrayp->hi() - aindex)
                                                                         : (aindex - arrayp->lo());
                            V3_সংখ্যা selected{constp, 8};
                            selected.opSel(constp->অঙ্ক(), cindex * 8 + 7, cindex * 8);
                            UINFO(0, "   aindex=" << aindex << "  cindex=" << cindex
                                                  << "  c=" << selected);
                            যদি (!selected.isFourState()) {
                                যদি (ধ্রুৱ uint32_t c = selected.toUInt()) {
                                    newp->valuep(aindex,
                                                 নূতন AstConst{constp->fileline(),
                                                              AstConst::Widthed_মান{}, 8, c});
                                }
                            }
                        }
                        UINFO(6, "   unpackFromString: " << parentp);
                        rhsp->replaceWith(newp);
                        VL_DO_DANGLING(ঠেলো_Deletep(rhsp), rhsp);
                        rhsp = newp;
                    }
                }
            }
        }
        // We iterate and size the RHS based on the result of RHS evaluation
        checkClassAssign(parentp, side, rhsp, lhsDTypep);
        ধ্রুৱ তার্কিক lhsStream = (VN_IS(parentp, গ্ৰন্থি_Assign)
                                && VN_IS(VN_AS(parentp, গ্ৰন্থি_Assign)->lhsp(), গ্ৰন্থি_Stream));
        rhsp = iterateCheck(parentp, side, rhsp, ASSIGN, FINAL, lhsDTypep,
                            lhsStream ? EXTEND_OFF : EXTEND_LHS, সঁচা, STREAM_USE_NONE, rhsAccess);
        // UINFOTREE(1, parentp, "", "checkout");
        (শূন্য)rhsp;  // cppcheck
    }
    // Convert an unpacked array value to an equivalent type with some dimension reversed,
    // keeping elements in left-to-right order (IEEE 1800-2023 7.6)
    Ast_গ্ৰন্থি_ৰাশিমালা* convertUnpackedOrder(Ast_গ্ৰন্থি_ৰাশিমালা* exprp, Ast_গ্ৰন্থি_DType* dtypep) {
        ধ্রুৱ AstUnpackArrayDType* ধ্রুৱ arrayp = VN_CAST(dtypep->skipRefp(), UnpackArrayDType);
        যদি (!arrayp || !arrayp->isReverseOf(exprp->dtypep())) ঘূৰাই exprp;
        VNRelinker relinker;
        exprp->unlinkFrBack(&relinker);
        Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ newp = নূতন AstCvtUnpackToUnpack{exprp->fileline(), exprp, dtypep};
        relinker.relink(newp);
        ঘূৰাই newp;
    }

    স্থির তার্কিক isLogicalOperator(ধ্রুৱ Ast_গ্ৰন্থি* nodep) {
        // IEEE 1800-2023 Table 11-1 logical operators, which take only integral or real operands
        ঘূৰাই VN_IS(nodep, LogAnd) || VN_IS(nodep, LogEq) || VN_IS(nodep, LogIf)
               || VN_IS(nodep, LogNot) || VN_IS(nodep, LogOr);
    }
    শূন্য iterateCheckBool(Ast_গ্ৰন্থি* parentp, ধ্রুৱ ৱর্ণ* side, Ast_গ্ৰন্থি* underp, Stage stage) {
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
        Ast_গ্ৰন্থি_DType* ধ্রুৱ underVDTypep = underp ? underp->dtypep()->skipRefp() : nullptr;
        যদি (underp && underVDTypep->isDouble()) {
            UINFO(6, "   spliceCvtCmpD0: " << underp);
            VNRelinker linker;
            underp->unlinkFrBack(&linker);
            Ast_গ্ৰন্থি* ধ্রুৱ newp
                = নূতন AstNeqD{parentp->fileline(), VN_AS(underp, গ্ৰন্থি_ৰাশিমালা),
                              নূতন AstConst{parentp->fileline(), AstConst::RealDouble{}, 0.0}};
            linker.relink(newp);
        } অন্যথা যদি (underVDTypep->isString() && !isLogicalOperator(parentp)) {
            // For STRING as a condition, likewise test against the empty string, never a
            // warning; a string holds no NUL, so nonzero means non-empty (IEEE 1800-2023 6.16)
            UINFO(6, "   spliceCvtCmpN0: " << underp);
            VNRelinker linker;
            underp->unlinkFrBack(&linker);
            Ast_গ্ৰন্থি* ধ্রুৱ newp
                = নূতন AstNeqN{parentp->fileline(), VN_AS(underp, গ্ৰন্থি_ৰাশিমালা),
                              নূতন AstConst{parentp->fileline(), AstConst::String{}, ""}};
            linker.relink(newp);
        } অন্যথা যদি (VN_IS(underVDTypep, ClassRefDType) || VN_IS(underVDTypep, IfaceRefDType)
                   || (VN_IS(underVDTypep, BasicDType)
                       && VN_AS(underVDTypep, BasicDType)->isCHandle())) {
            // Allow warning-free "if (handle)"
            VL_DO_DANGLING(fixWidthReduce(VN_AS(underp, গ্ৰন্থি_ৰাশিমালা)), underp);  // Changed
        } অন্যথা যদি (!underVDTypep->basicp() || underVDTypep->isAggregateType()
                   || underVDTypep->isString()) {
            parentp->v3error(ucfirst(parentp->prettyOperatorName())
                             << " expects a numeric " << side << " operand, but it has data type "
                             << underVDTypep->prettyDTypeNameQ() << " (IEEE 1800-2023 11.3)");
            underp->replaceWith(নূতন AstConst{parentp->fileline(), AstConst::BitFalseErroring{}});
            VL_DO_DANGLING(ঠেলো_Deletep(underp), underp);
        } অন্যথা {
            ধ্রুৱ তার্কিক bad = widthBad(underp, parentp->findBitDType());
            যদি (bad) {
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
            যদি (Ast_ৰাশিমালা_Stmt* ধ্রুৱ ৰাশিমালা_Stmtp = VN_CAST(underp, ৰাশিমালা_Stmt)) {
                // Reduce only the result, leaving the statements in place
                fixWidthReduce(ৰাশিমালা_Stmtp->resultp());
                ৰাশিমালা_Stmtp->dtypeFrom(ৰাশিমালা_Stmtp->resultp());
            } অন্যথা যদি (underp->width() != 1) {
                VL_DO_DANGLING(fixWidthReduce(VN_AS(underp, গ্ৰন্থি_ৰাশিমালা)), underp);  // Changed
            }
        }
    }

    স্থির শূন্য checkEnum(Ast_গ্ৰন্থি* nodep, Ast_গ্ৰন্থি* parentp, ধ্রুৱ Ast_গ্ৰন্থি_DType* targetDTypep,
                          ধ্রুৱ Ast_গ্ৰন্থি_DType* sourceDTypep, ধ্রুৱ Ast_গ্ৰন্থি_ৰাশিমালা* sourceExprp) {
        যদি (ধ্রুৱ AstEnumDType* ধ্রুৱ targetEnump
            = VN_CAST(targetDTypep->skipRefToEnump(), EnumDType)) {
            ধ্রুৱ VCastable castable
                = Ast_গ্ৰন্থি::computeCastable(targetEnump, sourceDTypep, sourceExprp);
            যদি (castable != VCastable::SAMEISH && castable != VCastable::COMPATIBLE
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

    Ast_গ্ৰন্থি* iterateCheck(Ast_গ্ৰন্থি* parentp, ধ্রুৱ ৱর্ণ* side, Ast_গ্ৰন্থি* underp, Determ determ,
                          Stage stage, Ast_গ্ৰন্থি_DType* expDTypep, ExtendRule extendRule,
                          তার্কিক warnOn = সঁচা, StreamUse streamUse = STREAM_USE_NONE,
                          VAccess rhsAccess = VAccess::READ) {
        // Perform data type check on underp, which is underneath parentp used for error reporting
        // Returns the new underp
        // Conversion to/from doubles and integers are before iterating.
        UASSERT_OBJ(stage == FINAL, parentp, "Bad state to iterateCheck");
        UASSERT_OBJ(underp, parentp, "Node has no child");
        UASSERT_OBJ(underp->dtypep(), underp,
                    "Node has no type");  // Perhaps forgot to do a prelim visit on it?
        ধ্রুৱ StreamUse childStreamUse
            = streamUse != STREAM_USE_NONE ? streamUse : streamUseForDeterm(determ);
        যদি (rhsAccess.isReadOnly()) {
            যদি (Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ exprp = VN_CAST(underp, গ্ৰন্থি_ৰাশিমালা)) {
                // A value only read is converted to a reversed expected type.
                underp = convertUnpackedOrder(exprp, expDTypep);
            }
        }
        যদি (VN_IS(underp, গ্ৰন্থি_DType)) {  // Note the node itself, not node's data type
            // Must be near top of these checks as underp->dtypep() will look normal
            underp->v3error(ucfirst(parentp->prettyOperatorName())
                            << " expected non-datatype " << side << " but "
                            << underp->prettyNameQ() << " is a datatype.");
        } অন্যথা যদি (expDTypep == underp->dtypep()) {  // Perfect
            underp = userIterateSubtreeReturnEdits(underp,
                                                   WidthVP{expDTypep, FINAL, childStreamUse}.p());
        } অন্যথা যদি (expDTypep->isDouble() && underp->isDouble()) {  // Also good
            underp = userIterateSubtreeReturnEdits(underp,
                                                   WidthVP{expDTypep, FINAL, childStreamUse}.p());
        } অন্যথা যদি (expDTypep->isDouble() && !underp->isDouble()) {
            Ast_গ্ৰন্থি* ধ্রুৱ oldp
                = underp;  // Need FINAL on children; otherwise splice would block it
            spliceCvtD(VN_AS(underp, গ্ৰন্থি_ৰাশিমালা));
            underp = userIterateSubtreeReturnEdits(oldp, WidthVP{SELF, FINAL, childStreamUse}.p());
        } অন্যথা যদি (!expDTypep->isDouble() && underp->isDouble()) {
            Ast_গ্ৰন্থি* ধ্রুৱ oldp
                = underp;  // Need FINAL on children; otherwise splice would block it
            spliceCvtS(VN_AS(underp, গ্ৰন্থি_ৰাশিমালা), সঁচা, expDTypep->width());  // Round RHS
            underp = userIterateSubtreeReturnEdits(oldp, WidthVP{SELF, FINAL, childStreamUse}.p());
        } অন্যথা যদি (expDTypep->isString() && !underp->dtypep()->isString()) {
            Ast_গ্ৰন্থি* ধ্রুৱ oldp
                = underp;  // Need FINAL on children; otherwise splice would block it
            spliceCvtString(VN_AS(underp, গ্ৰন্থি_ৰাশিমালা));
            underp = userIterateSubtreeReturnEdits(oldp, WidthVP{SELF, FINAL, childStreamUse}.p());
        } অন্যথা {
            ধ্রুৱ Ast_গ্ৰন্থি_DType* ধ্রুৱ underDtp = underp->dtypep()->skipRefp();
            যদি (determ != ASSIGN && expDTypep->skipRefp()->isIntegralOrPacked()
                && underDtp->isAggregateType()) {
                parentp->v3error(ucfirst(parentp->prettyOperatorName())
                                 << " expects a numeric " << side
                                 << " operand, but it has data type "
                                 << underDtp->prettyDTypeNameQ() << " (IEEE 1800-2023 11.3)");
                Ast_গ্ৰন্থি* ধ্রুৱ newp
                    = নূতন AstConst{underp->fileline(), AstConst::BitFalseErroring{}};
                underp->replaceWith(newp);
                VL_DO_DANGLING(ঠেলো_Deletep(underp), underp);
                underp = newp;
            }
            ধ্রুৱ Ast_গ্ৰন্থি_DType* ধ্রুৱ expDtp = expDTypep->skipRefp();
            যদি (determ == ASSIGN && VN_IS(expDtp, গ্ৰন্থি_UOr_Struct_DType) && expDtp->isAggregateType()
                && !underDtp->isAggregateType()) {
                parentp->v3error("Illegal assignment: types are not assignment compatible (IEEE "
                                 "1800-2023 7.6)"
                                 << warnDTypes(parentp, expDtp, underDtp));
            }
            // Connections of interface arrays have their own check, with a better message
            যদি (determ == ASSIGN && !isIfaceArrayDType(expDtp)
                && !unpackedArraySizesMatch(expDtp, underDtp)) {
                parentp->v3error("Illegal assignment: Unmatched array sizes (IEEE 1800-2023 7.6)"
                                 << warnDTypes(parentp, expDtp, underDtp));
            }
            ধ্রুৱ AstBasicDType* ধ্রুৱ expBasicp = expDTypep->basicp();
            ধ্রুৱ AstBasicDType* ধ্রুৱ underBasicp = underp->dtypep()->basicp();
            যদি (expBasicp && underBasicp) {
                যদি (!m_enumItemp && !parentp->fileline()->warnIsOff(V3ErrorCode::ENUMVALUE)
                    && warnOn) {
                    যদি (rhsAccess.isReadOrRW()) {
                        checkEnum(underp, parentp, expDTypep, underp->dtypep(),
                                  VN_AS(underp, গ্ৰন্থি_ৰাশিমালা));
                    }
                    যদি (rhsAccess.isWriteOrRW()) {
                        checkEnum(underp, parentp, underp->dtypep(), expDTypep, nullptr);
                    }
                }
                Ast_গ্ৰন্থি_DType* subDTypep = expDTypep;
                // We then iterate FINAL before width fixes, as if the under-operation
                // is e.g. an ADD, the ADD will auto-adjust to the proper data type
                // or if another operation e.g. ATOI will not.
                যদি (determ == SELF) {
                    underp = userIterateSubtreeReturnEdits(
                        underp, WidthVP{SELF, FINAL, childStreamUse}.p());
                } অন্যথা যদি (determ == ASSIGN) {
                    // IEEE: Signedness is solely determined by the RHS
                    // (underp), not by the LHS (expDTypep)
                    যদি (underp->isSigned() != subDTypep->isSigned()
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
                } অন্যথা {
                    underp = userIterateSubtreeReturnEdits(
                        underp, WidthVP{subDTypep, FINAL, childStreamUse}.p());
                }
                // Note the check uses the expected size, not the child's subDTypep as we want the
                // child node's width to end up correct for the assignment (etc)
                widthCheckSized(parentp, side, VN_AS(underp, গ্ৰন্থি_ৰাশিমালা), expDTypep, extendRule,
                                warnOn);
            } অন্যথা যদি (!VN_IS(parentp, Eq) && !VN_IS(parentp, Neq)
                       && !VN_IS(expDTypep->skipRefp(), IfaceRefDType)
                       && VN_IS(underp->dtypep()->skipRefp(), IfaceRefDType)) {
                underp->v3error(ucfirst(parentp->prettyOperatorName())
                                << " expected non-interface on " << side << " but "
                                << underp->prettyNameQ() << " is an interface.");
            } অন্যথা যদি (ধ্রুৱ AstIfaceRefDType* expIfaceRefp
                       = VN_CAST(expDTypep->skipRefp(), IfaceRefDType)) {
                ধ্রুৱ AstIfaceRefDType* underIfaceRefp
                    = VN_CAST(underp->dtypep()->skipRefp(), IfaceRefDType);
                যদি (VN_IS(underp, Const) && VN_AS(underp, Const)->অঙ্ক().isNull()) {
                    // '= null' is ok
                } অন্যথা যদি (!underIfaceRefp) {
                    underp->v3error(ucfirst(parentp->prettyOperatorName())
                                    << " expected " << expIfaceRefp->ifaceViaCellp()->prettyNameQ()
                                    << " interface on " << side << " but " << underp->prettyNameQ()
                                    << " is not an interface.");
                } অন্যথা যদি (expIfaceRefp->ifaceViaCellp() != underIfaceRefp->ifaceViaCellp()) {
                    underp->v3error(ucfirst(parentp->prettyOperatorName())
                                    << " expected " << expIfaceRefp->ifaceViaCellp()->prettyNameQ()
                                    << " interface on " << side << " but " << underp->prettyNameQ()
                                    << " is a different interface ("
                                    << underIfaceRefp->ifaceViaCellp()->prettyNameQ() << ").");
                } অন্যথা যদি (underIfaceRefp->modportp()
                           && expIfaceRefp->modportp() != underIfaceRefp->modportp()) {
                    underp->v3error(ucfirst(parentp->prettyOperatorName())
                                    << " expected "
                                    << (expIfaceRefp->modportp()
                                            ? expIfaceRefp->modportp()->prettyNameQ()
                                            : "no")
                                    << " interface modport on " << side << " but got "
                                    << underIfaceRefp->modportp()->prettyNameQ() << " modport.");
                } অন্যথা {
                    underp = userIterateSubtreeReturnEdits(
                        underp, WidthVP{expDTypep, FINAL, childStreamUse}.p());
                }
            } অন্যথা {
                // Hope it just works out (perhaps a cast will deal with it)
                underp = userIterateSubtreeReturnEdits(
                    underp, WidthVP{expDTypep, FINAL, childStreamUse}.p());
            }
        }
        ঘূৰাই underp;
    }

    শূন্য
    widthCheckSized(Ast_গ্ৰন্থি* parentp, ধ্রুৱ ৱর্ণ* side,
                    Ast_গ্ৰন্থি_ৰাশিমালা* underp,  // Node to be checked or have typecast added in front of
                    Ast_গ্ৰন্থি_DType* expDTypep, ExtendRule extendRule, তার্কিক warnOn = সঁচা) {
        // Issue warnings on sized number width mismatches, then do appropriate size extension
        // Generally iterateCheck is what is wanted instead of this
        // UINFO(9,"wchk "<<side<<endl<<"  "<<parentp<<endl<<"  "<<underp<<endl<<"
        // e="<<expDTypep<<" i"<<warnOn<<endl);
        ধ্রুৱ AstBasicDType* ধ্রুৱ expBasicp = expDTypep->basicp();
        ধ্রুৱ AstBasicDType* ধ্রুৱ underBasicp = underp->dtypep()->basicp();
        যদি (expDTypep == underp->dtypep()) {
            ঘূৰাই;  // Same type must match
        } অন্যথা যদি (!expBasicp || expBasicp->isDouble() || !underBasicp
                   || underBasicp->isDouble()) {
            // This is perhaps a v3fatalSrc as we should have checked the types
            // before calling widthCheck, but we may have missed a non-sized
            // check in earlier code, so might as well assume it is the users'
            // fault.
            parentp->v3error(ucfirst(parentp->prettyOperatorName())
                             << " expected non-complex non-double " << side << " in width check");
#যদি VL_DEBUG
            parentp->v3fatalSrc("widthCheckSized should not be called on doubles/complex types");
#endif
            ঘূৰাই;
        } অন্যথা {
            ধ্রুৱ পূর্ণাঙ্ক expWidth = expDTypep->width();
            পূর্ণাঙ্ক expWidthMin = expDTypep->widthMin();
            যদি (expWidthMin == 0) expWidthMin = expWidth;
            ধ্রুৱ তার্কিক bad = widthBad(underp, expDTypep);
            যদি ((bad || underp->width() != expWidth) && fixAutoExtend(underp /*ref*/, expWidth)) {
                underp = nullptr;  // Changes underp
                ঘূৰাই;
            }
            // If user has a sizing cast, assume they know what they are doing
            // (for better or worse)
            যদি (VN_IS(parentp->backp(), CastSize)) warnOn = মিছা;
            যদি (parentp->fileline()->erroringOn() || underp->fileline()->erroringOn())
                warnOn = মিছা;
            যদি (VN_IS(underp, Const) && VN_AS(underp, Const)->অঙ্ক().isFromString()
                && expWidth > underp->width()
                && (((expWidth - underp->width()) % 8) == 0)) {  // At least it's character sized
                // reg [31:0] == "foo" we'll consider probably fine.
                // Maybe this should be a special warning?  Not for now.
                warnOn = মিছা;
            }
            যদি ((VN_IS(parentp, Add) && underp->width() == 1 && underp->isOne())
                || (VN_IS(parentp, Sub) && underp->width() == 1 && underp->isOne()
                    && 0 == std::strcmp(side, "RHS"))) {
                // "foo + 1'b1", or "foo - 1'b1" are very common, people assume
                // they extend correctly
                warnOn = মিছা;
            }
            যদি (bad && warnOn) {
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
            যদি (bad || underp->width() != expWidth) {
                // If we're in an NodeAssign, don't truncate the RHS if the LHS is
                // a NodeStream. The streaming operator changes the rules regarding
                // which bits to truncate.
                ধ্রুৱ Ast_গ্ৰন্থি_Assign* assignp = VN_CAST(parentp, গ্ৰন্থি_Assign);
                ধ্রুৱ AstPin* pinp = VN_CAST(parentp, Pin);
                যদি (assignp && VN_IS(assignp->lhsp(), গ্ৰন্থি_Stream)) {
                } অন্যথা যদি (pinp && pinp->modVarp()->direction() != VDirection::প্ৰবেশ) {
                    // V3Inst::pinReconnectSimple must deal
                    UINFO(5, "pinInSizeMismatch: " << pinp);
                } অন্যথা যদি (assignp && VN_IS(underp, গ্ৰন্থি_Stream) && expWidth > underp->width()) {
                    // IEEE 1800-2023 11.4.14: When assigning a stream to a wider fixed-size
                    // target, widen by filling zero bits on the right. That is, left-justify
                    // the stream bits within the target width.
                    // Build: ShiftL(Extend(stream, expWidth), expWidth - streamWidth)
                    UINFO(5, "Widen NodeStream RHS with left-justify per 11.4.14");
                    VNRelinker linker;
                    ধ্রুৱ পূর্ণাঙ্ক shift = expWidth - underp->width();
                    underp->unlinkFrBack(&linker);
                    AstExtend* ধ্রুৱ widenedp = নূতন AstExtend{underp->fileline(), underp};
                    widenedp->didWidth(সঁচা);
                    // Shift left so zeros fill on the right
                    Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ shiftedp = নূতন AstShiftL{
                        underp->fileline(), widenedp,
                        নূতন AstConst{underp->fileline(), static_cast<uint32_t>(shift)}, expWidth};
                    // Final dtype should match expected
                    shiftedp->dtypep(expDTypep);
                    linker.relink(shiftedp);
                } অন্যথা {
                    VL_DO_DANGLING(fixWidthExtend(underp, expDTypep, extendRule), underp);
                }
            }
        }
    }

    //----------------------------------------------------------------------
    // SIGNED/DOUBLE METHODS

    Ast_গ্ৰন্থি_ৰাশিমালা* checkCvtUS(Ast_গ্ৰন্থি_ৰাশিমালা* nodep, তার্কিক fatal) {
        যদি (nodep && nodep->dtypep()->skipRefp()->isDouble()) {
            যদি (fatal) {
                nodep->v3error("Expected integral input to " << nodep->backp()->prettyTypeName());
            } অন্যথা {
                nodep->v3warn(REALCVT,
                              "Implicit conversion of real to integer; expected integral input to "
                                  << nodep->backp()->prettyTypeName());
            }
            nodep = spliceCvtS(nodep, মিছা, 32);
        }
        ঘূৰাই nodep;
    }

    Ast_গ্ৰন্থি_ৰাশিমালা* spliceCvtD(Ast_গ্ৰন্থি_ৰাশিমালা* nodep) {
        // For integer used in REAL context, convert to real
        // We don't warn here, "2.0 * 2" is common and reasonable
        যদি (nodep && !nodep->dtypep()->skipRefp()->isDouble()) {
            UINFO(6, "   spliceCvtD: " << nodep);
            VNRelinker linker;
            nodep->unlinkFrBack(&linker);
            Ast_গ্ৰন্থি_ৰাশিমালা* newp;
            যদি (nodep->dtypep()->skipRefp()->isSigned()) {
                newp = নূতন AstISToRD{nodep->fileline(), nodep};
            } অন্যথা {
                newp = নূতন AstIToRD{nodep->fileline(), nodep};
            }
            linker.relink(newp);
            ঘূৰাই newp;
        } অন্যথা {
            ঘূৰাই nodep;
        }
    }
    Ast_গ্ৰন্থি_ৰাশিমালা* spliceCvtS(Ast_গ্ৰন্থি_ৰাশিমালা* nodep, তার্কিক warnOn, পূর্ণাঙ্ক width) {
        // IEEE-2012 11.8.1: Signed: Type coercion creates signed
        // 11.8.2: Argument to convert is self-determined
        যদি (nodep && nodep->dtypep()->skipRefp()->isDouble()) {
            UINFO(6, "   spliceCvtS: " << nodep);
            VNRelinker linker;
            nodep->unlinkFrBack(&linker);
            যদি (ধ্রুৱ AstConst* ধ্রুৱ constp = VN_CAST(nodep, Const)) {
                // We convert to/from int32_t rather than use floor() as want to make sure is
                // representable in integer's number of bits
                যদি (constp->isDouble() && V3_সংখ্যা::epsilonIntegral(constp->অঙ্ক().toDouble())) {
                    warnOn = মিছা;
                }
            }
            যদি (warnOn) nodep->v3warn(REALCVT, "Implicit conversion of real to integer");
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ newp = নূতন AstRToIRoundS{nodep->fileline(), nodep};
            linker.relink(newp);
            newp->dtypeSetBitSized(width, VSigning::SIGNED);
            ঘূৰাই newp;
        } অন্যথা {
            ঘূৰাই nodep;
        }
    }
    Ast_গ্ৰন্থি_ৰাশিমালা* spliceCvtString(Ast_গ্ৰন্থি_ৰাশিমালা* nodep) {
        // IEEE-2012 11.8.1: Signed: Type coercion creates signed
        // 11.8.2: Argument to convert is self-determined
        যদি (nodep && !(nodep->dtypep()->basicp() && nodep->dtypep()->basicp()->isString())) {
            UINFO(6, "   spliceCvtString: " << nodep);
            VNRelinker linker;
            nodep->unlinkFrBack(&linker);
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ newp = নূতন AstCvtPackString{nodep->fileline(), nodep};
            linker.relink(newp);
            ঘূৰাই newp;
        } অন্যথা {
            ঘূৰাই nodep;
        }
    }
    Ast_গ্ৰন্থি_Biop* replaceWithUOrSVersion(Ast_গ্ৰন্থি_Biop* nodep, তার্কিক signedFlavorNeeded) {
        // Given a signed/unsigned node type, create the opposite type
        // Return new node or nullptr if nothing
        যদি (signedFlavorNeeded == nodep->signedFlavor()) ঘূৰাই nullptr;
        যদি (!nodep->dtypep()) nodep->dtypeFrom(nodep->lhsp());
        // To simplify callers, some node types don't need to change
        ৱিকল্প (nodep->type()) {
        পক্ষ VNType::Eq: nodep->dtypeChgSigned(signedFlavorNeeded); ঘূৰাই nullptr;
        পক্ষ VNType::Neq: nodep->dtypeChgSigned(signedFlavorNeeded); ঘূৰাই nullptr;
        পক্ষ VNType::EqCase: nodep->dtypeChgSigned(signedFlavorNeeded); ঘূৰাই nullptr;
        পক্ষ VNType::NeqCase: nodep->dtypeChgSigned(signedFlavorNeeded); ঘূৰাই nullptr;
        পক্ষ VNType::EqWild: nodep->dtypeChgSigned(signedFlavorNeeded); ঘূৰাই nullptr;
        পক্ষ VNType::NeqWild: nodep->dtypeChgSigned(signedFlavorNeeded); ঘূৰাই nullptr;
        পক্ষ VNType::Add: nodep->dtypeChgSigned(signedFlavorNeeded); ঘূৰাই nullptr;
        পক্ষ VNType::Sub: nodep->dtypeChgSigned(signedFlavorNeeded); ঘূৰাই nullptr;
        পক্ষ VNType::ShiftL: nodep->dtypeChgSigned(signedFlavorNeeded); ঘূৰাই nullptr;
        স্ৱভাৱ: ভঙ্গ;
        }
        File_সাৰি* ধ্রুৱ fl = nodep->fileline();
        Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ lhsp = nodep->lhsp()->unlinkFrBack();
        Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ rhsp = nodep->rhsp()->unlinkFrBack();
        Ast_গ্ৰন্থি_Biop* newp = nullptr;
        ৱিকল্প (nodep->type()) {
        পক্ষ VNType::Gt: newp = নূতন AstGtS{fl, lhsp, rhsp}; ভঙ্গ;
        পক্ষ VNType::GtS: newp = নূতন AstGt{fl, lhsp, rhsp}; ভঙ্গ;
        পক্ষ VNType::Gte: newp = নূতন AstGteS{fl, lhsp, rhsp}; ভঙ্গ;
        পক্ষ VNType::GteS: newp = নূতন AstGte{fl, lhsp, rhsp}; ভঙ্গ;
        পক্ষ VNType::Lt: newp = নূতন AstLtS{fl, lhsp, rhsp}; ভঙ্গ;
        পক্ষ VNType::LtS: newp = নূতন AstLt{fl, lhsp, rhsp}; ভঙ্গ;
        পক্ষ VNType::Lte: newp = নূতন AstLteS{fl, lhsp, rhsp}; ভঙ্গ;
        পক্ষ VNType::LteS: newp = নূতন AstLte{fl, lhsp, rhsp}; ভঙ্গ;
        পক্ষ VNType::Div: newp = নূতন AstDivS{fl, lhsp, rhsp}; ভঙ্গ;
        পক্ষ VNType::DivS: newp = নূতন AstDiv{fl, lhsp, rhsp}; ভঙ্গ;
        পক্ষ VNType::ModDiv: newp = নূতন AstModDivS{fl, lhsp, rhsp}; ভঙ্গ;
        পক্ষ VNType::ModDivS: newp = নূতন AstModDiv{fl, lhsp, rhsp}; ভঙ্গ;
        পক্ষ VNType::Mul: newp = নূতন AstMulS{fl, lhsp, rhsp}; ভঙ্গ;
        পক্ষ VNType::MulS: newp = নূতন AstMul{fl, lhsp, rhsp}; ভঙ্গ;
        পক্ষ VNType::ShiftR: newp = নূতন AstShiftRS{fl, lhsp, rhsp}; ভঙ্গ;
        পক্ষ VNType::ShiftRS: newp = নূতন AstShiftR{fl, lhsp, rhsp}; ভঙ্গ;
        স্ৱভাৱ:  // LCOV_EXCL_LINE
            nodep->v3fatalSrc("Node needs sign change, but bad case: " << nodep);
            ভঙ্গ;
        }
        UINFO(6, "   ReplaceWithUOrSVersion: " << nodep << " w/ " << newp);
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
        ঘূৰাই newp;
    }
    Ast_গ্ৰন্থি_Biop* replaceWithDVersion(Ast_গ্ৰন্থি_Biop* nodep) {
        // Given a signed/unsigned node type, create the opposite type
        // Return new node or nullptr if nothing
        যদি (nodep->doubleFlavor()) ঘূৰাই nullptr;
        File_সাৰি* ধ্রুৱ fl = nodep->fileline();
        Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ lhsp = nodep->lhsp()->unlinkFrBack();
        Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ rhsp = nodep->rhsp()->unlinkFrBack();
        Ast_গ্ৰন্থি_Biop* newp = nullptr;
        // No width change on output;...                // All below have bool or double outputs
        ৱিকল্প (nodep->type()) {
        পক্ষ VNType::Add: newp = নূতন AstAddD{fl, lhsp, rhsp}; ভঙ্গ;
        পক্ষ VNType::Sub: newp = নূতন AstSubD{fl, lhsp, rhsp}; ভঙ্গ;
        পক্ষ VNType::Pow: newp = নূতন AstPowD{fl, lhsp, rhsp}; ভঙ্গ;
        পক্ষ VNType::Eq:
        পক্ষ VNType::EqCase: newp = নূতন AstEqD{fl, lhsp, rhsp}; ভঙ্গ;
        পক্ষ VNType::Neq:
        পক্ষ VNType::NeqCase: newp = নূতন AstNeqD{fl, lhsp, rhsp}; ভঙ্গ;
        পক্ষ VNType::Gt:
        পক্ষ VNType::GtS: newp = নূতন AstGtD{fl, lhsp, rhsp}; ভঙ্গ;
        পক্ষ VNType::Gte:
        পক্ষ VNType::GteS: newp = নূতন AstGteD{fl, lhsp, rhsp}; ভঙ্গ;
        পক্ষ VNType::Lt:
        পক্ষ VNType::LtS: newp = নূতন AstLtD{fl, lhsp, rhsp}; ভঙ্গ;
        পক্ষ VNType::Lte:
        পক্ষ VNType::LteS: newp = নূতন AstLteD{fl, lhsp, rhsp}; ভঙ্গ;
        পক্ষ VNType::Div:
        পক্ষ VNType::DivS: newp = নূতন AstDivD{fl, lhsp, rhsp}; ভঙ্গ;
        পক্ষ VNType::Mul:
        পক্ষ VNType::MulS: newp = নূতন AstMulD{fl, lhsp, rhsp}; ভঙ্গ;
        স্ৱভাৱ:  // LCOV_EXCL_LINE
            nodep->v3fatalSrc("Node needs conversion to double, but bad case: " << nodep);
            ভঙ্গ;
        }
        UINFO(6, "   ReplaceWithDVersion: " << nodep << " w/ " << newp);
        nodep->replaceWith(newp);
        // No width change; the default created type (bool or double) is correct
        VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
        ঘূৰাই newp;
    }
    Ast_গ্ৰন্থি_Biop* replaceWithNVersion(Ast_গ্ৰন্থি_Biop* nodep) {
        // Given a signed/unsigned node type, replace with string version
        // Return new node or nullptr if nothing
        যদি (nodep->stringFlavor()) ঘূৰাই nullptr;
        File_সাৰি* ধ্রুৱ fl = nodep->fileline();
        Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ lhsp = nodep->lhsp()->unlinkFrBack();
        Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ rhsp = nodep->rhsp()->unlinkFrBack();
        Ast_গ্ৰন্থি_Biop* newp = nullptr;
        // No width change on output;...                // All below have bool or double outputs
        ৱিকল্প (nodep->type()) {
        পক্ষ VNType::Eq:
        পক্ষ VNType::EqCase:
        পক্ষ VNType::EqWild: newp = নূতন AstEqN{fl, lhsp, rhsp}; ভঙ্গ;
        পক্ষ VNType::Neq:
        পক্ষ VNType::NeqCase:
        পক্ষ VNType::NeqWild: newp = নূতন AstNeqN{fl, lhsp, rhsp}; ভঙ্গ;
        পক্ষ VNType::Gt:
        পক্ষ VNType::GtS: newp = নূতন AstGtN{fl, lhsp, rhsp}; ভঙ্গ;
        পক্ষ VNType::Gte:
        পক্ষ VNType::GteS: newp = নূতন AstGteN{fl, lhsp, rhsp}; ভঙ্গ;
        পক্ষ VNType::Lt:
        পক্ষ VNType::LtS: newp = নূতন AstLtN{fl, lhsp, rhsp}; ভঙ্গ;
        পক্ষ VNType::Lte:
        পক্ষ VNType::LteS: newp = নূতন AstLteN{fl, lhsp, rhsp}; ভঙ্গ;
        স্ৱভাৱ:  // LCOV_EXCL_LINE
            nodep->v3fatalSrc("Node needs conversion to string, but bad case: " << nodep);
            ভঙ্গ;
        }
        UINFO(6, "   ReplaceWithNVersion: " << nodep << " w/ " << newp);
        nodep->replaceWith(newp);
        // No width change; the default created type (bool or string) is correct
        VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
        ঘূৰাই newp;
    }
    Ast_গ্ৰন্থি_Uniop* replaceWithDVersion(Ast_গ্ৰন্থি_Uniop* nodep) {
        // Given a signed/unsigned node type, create the opposite type
        // Return new node or nullptr if nothing
        যদি (nodep->doubleFlavor()) ঘূৰাই nullptr;
        File_সাৰি* ধ্রুৱ fl = nodep->fileline();
        Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ lhsp = nodep->lhsp()->unlinkFrBack();
        Ast_গ্ৰন্থি_Uniop* newp = nullptr;
        ৱিকল্প (nodep->type()) {
        পক্ষ VNType::Negate: newp = নূতন AstNegateD{fl, lhsp}; ভঙ্গ;
        স্ৱভাৱ:  // LCOV_EXCL_LINE
            nodep->v3fatalSrc("Node needs conversion to double, but bad case: " << nodep);
            ভঙ্গ;
        }
        UINFO(6, "   ReplaceWithDVersion: " << nodep << " w/ " << newp);
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
        ঘূৰাই newp;
    }

    //----------------------------------------------------------------------
    // METHODS - strings

    শূন্য replaceWithSFormat(AstMethodCall* nodep, ধ্রুৱ string& format) {
        // For string.itoa and similar, replace with SFormatF
        AstArg* ধ্রুৱ argp = nodep->argsp();
        UASSERT_OBJ(argp, nodep,
                    "Argument needed for string method, call methodOkArguments before here");
        Ast_গ্ৰন্থি_Var_Ref* ধ্রুৱ fromp = VN_AS(nodep->fromp()->unlinkFrBack(), VarRef);
        Ast_গ্ৰন্থি* ধ্রুৱ newp = নূতন AstAssign{
            nodep->fileline(), fromp,
            নূতন AstSFormatF{nodep->fileline(), format, মিছা, argp->exprp()->unlinkFrBack()}};
        fromp->access(VAccess::WRITE);
        ঠেলো_Deletep(nodep->backp());
        VL_DO_DANGLING(nodep->backp()->replaceWith(newp), newp);
    }

    //----------------------------------------------------------------------
    // METHODS - data types

    Ast_গ্ৰন্থি_DType* iterateEditMoveDTypep(Ast_গ্ৰন্থি* parentp, Ast_গ্ৰন্থি_DType* dtnodep) {
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
        ধ্রুৱ তার্কিক child1 = (parentp->getChildDTypep() == dtnodep);
        ধ্রুৱ তার্কিক child2 = (parentp->getChild2DTypep() == dtnodep);
        যদি (child1 || child2) {
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
        যদি (!dtnodep->didWidth()) {
            UINFO(9, "iterateEditMoveDTypep pointer iterating " << dtnodep);
            // See notes in visit(AstBracketArrayDType*)
            UASSERT_OBJ(!VN_IS(dtnodep, BracketArrayDType), parentp,
                        "Brackets should have been iterated as children");
            userIterate(dtnodep, nullptr);
            UASSERT_OBJ(dtnodep->didWidth(), parentp,
                        "iterateEditMoveDTypep didn't get width resolution");
        }
        ঘূৰাই dtnodep;
    }

    AstConst* dimension_মান(File_সাৰি* fileline, Ast_গ্ৰন্থি_DType* nodep, VAttrType attrType,
                             পূর্ণাঙ্ক dim) {
        // Return the dimension value for the specified attribute and constant dimension
        Ast_গ্ৰন্থি_DType* dtypep = nodep->skipRefp();
        VNumRange declRange;  // ranged() set false
        বাবে (পূর্ণাঙ্ক i = 1; i <= dim; ++i) {
            // UINFO(9, "   dim at "<<dim<<"  "<<dtypep);
            declRange = VNumRange{};  // ranged() set false
            যদি (ধ্রুৱ Ast_গ্ৰন্থি_Array_DType* ধ্রুৱ adtypep = VN_CAST(dtypep, গ্ৰন্থি_Array_DType)) {
                declRange = adtypep->declRange();
                যদি (i < dim) dtypep = adtypep->subDTypep()->skipRefp();
                অনুৱর্ত;
            } অন্যথা যদি (ধ্রুৱ Ast_গ্ৰন্থি_UOr_Struct_DType* ধ্রুৱ adtypep
                       = VN_CAST(dtypep, গ্ৰন্থি_UOr_Struct_DType)) {
                declRange = adtypep->declRange();
                ভঙ্গ;  // Sub elements don't look like arrays and can't iterate into
            } অন্যথা যদি (ধ্রুৱ AstBasicDType* ধ্রুৱ adtypep = VN_CAST(dtypep, BasicDType)) {
                যদি (adtypep->isRanged()) declRange = adtypep->declRange();
                ভঙ্গ;
            }
            ভঙ্গ;  // LCOV_EXCL_LINE
        }
        AstConst* valp = nullptr;  // If nullptr, construct from val
        পূর্ণাঙ্ক val = 0;
        ৱিকল্প (attrType) {
        পক্ষ VAttrType::DIM_BITS: {
            পূর্ণাঙ্ক bits = 1;
            যেতিয়ালৈ (dtypep) {
                // UINFO(9, "   bits at " << bits << "  " << dtypep);
                যদি (ধ্রুৱ Ast_গ্ৰন্থি_Array_DType* ধ্রুৱ adtypep = VN_CAST(dtypep, গ্ৰন্থি_Array_DType)) {
                    bits *= adtypep->declRange().elements();
                    dtypep = adtypep->subDTypep()->skipRefp();
                    অনুৱর্ত;
                } অন্যথা যদি (ধ্রুৱ Ast_গ্ৰন্থি_UOr_Struct_DType* ধ্রুৱ adtypep
                           = VN_CAST(dtypep, গ্ৰন্থি_UOr_Struct_DType)) {
                    bits *= adtypep->isStreamableFixedAggregate() ? adtypep->widthStream()
                                                                  : adtypep->width();
                    ভঙ্গ;
                } অন্যথা যদি (ধ্রুৱ AstBasicDType* ধ্রুৱ adtypep = VN_CAST(dtypep, BasicDType)) {
                    bits *= adtypep->width();
                    ভঙ্গ;
                }
                ভঙ্গ;
            }
            যদি (dim == 0) {
                val = 0;
            } অন্যথা যদি (dim == 1 && !declRange.ranged()
                       && bits == 1) {  // $bits should be sane for non-arrays
                val = nodep->width();
            } অন্যথা {
                val = bits;
            }
            ভঙ্গ;  // LCOV_EXCL_LINE
        }
        পক্ষ VAttrType::DIM_উপৰেৰ: val = !declRange.ranged() ? 0 : declRange.hi(); ভঙ্গ;
        পক্ষ VAttrType::DIM_বাম: val = !declRange.ranged() ? 0 : declRange.বাম(); ভঙ্গ;
        পক্ষ VAttrType::DIM_নিচেৰ: val = !declRange.ranged() ? 0 : declRange.lo(); ভঙ্গ;
        পক্ষ VAttrType::DIM_ডান: val = !declRange.ranged() ? 0 : declRange.ডান(); ভঙ্গ;
        পক্ষ VAttrType::DIM_INCREMENT:
            val = (declRange.ranged() && declRange.ascending()) ? -1 : 1;
            ভঙ্গ;
        পক্ষ VAttrType::DIM_SIZE: val = !declRange.ranged() ? 0 : declRange.elements(); ভঙ্গ;
        স্ৱভাৱ: nodep->v3fatalSrc("Missing DIM ATTR type case"); ভঙ্গ;
        }
        যদি (!valp) valp = নূতন AstConst{fileline, AstConst::Signed32{}, val};
        UINFO(9, " $dimension " << attrType.ascii() << "(" << cvtToHex(dtypep) << "," << dim
                                << ")=" << valp);
        ঘূৰাই valp;
    }
    AstVarRef* dimensionVarRefp(Ast_গ্ৰন্থি_DType* nodep, VAttrType attrType, uint32_t msbdim) {
        // Return a reference to a constant table which has the specified dimension properties
        // for this data type. The reference is cached, so the table is built only once.
        AstVarRef*& cachedpr = m_tableMap[std::make_pair(nodep, attrType)];
        যদি (cachedpr) ঘূৰাই cachedpr->clone_বৃক্ষ(মিছা);

        Ast_গ্ৰন্থি_Array_DType* ধ্রুৱ vardtypep = নূতন AstUnpackArrayDType{
            nodep->fileline(), nodep->findIntDType(), নূতন AstRange(nodep->fileline(), msbdim, 0)};
        v3Global.rootp()->typeTablep()->addTypesp(vardtypep);
        userIterate(vardtypep, WidthVP{SELF, BOTH}.p());
        AstConsArray* ধ্রুৱ consp = নূতন AstConsArray{nodep->fileline(), vardtypep};
        বাবে (unsigned i = 0; i < msbdim + 1; ++i) {
            consp->valuep(i, dimension_মান(nodep->fileline(), nodep, attrType, i));
        }

        // Share identical tables via the constant pool
        cachedpr = V3ConstPool::find(consp);
        VL_DO_DANGLING(consp->delete_বৃক্ষ(), consp);  // V3ConstPool::find clones it
        cachedpr->varp()->didWidth(সঁচা);
        // Keep the dtype of the value, as the entry might have a different, compatible one
        cachedpr->didWidth(সঁচা);
        ঠেলো_Deletep(cachedpr);  // Deleted with the visitor - always cloned
        ঘূৰাই cachedpr->clone_বৃক্ষ(মিছা);
    }
    স্থির uint64_t enum_Max_মান(ধ্রুৱ Ast_গ্ৰন্থি* errNodep, ধ্রুৱ AstEnumDType* adtypep) {
        // Most enums unless overridden are 32 bits, so we size array
        // based on max enum value used.
        // Ideally we would have a fast algorithm when a number is
        // of small width and complete and so can use an array, and
        // a map for when the value is many bits and sparse.
        uint64_t maxval = 0;
        বাবে (ধ্রুৱ AstEnumItem* itemp = adtypep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), EnumItem)) {
            ধ্রুৱ AstConst* ধ্রুৱ vconstp = VN_CAST(itemp->valuep(), Const);
            UASSERT_OBJ(vconstp, errNodep, "Enum item without constified value");
            যদি (!vconstp->অঙ্ক().isAnyXZ() && vconstp->toUQuad() >= maxval)
                maxval = vconstp->toUQuad();
        }
        যদি (adtypep->itemsp()->width() > 64) {
            errNodep->v3warn(E_UNSUPPORTED,
                             "Unsupported: enum next/prev method on enum with > 64 bits");
            ঘূৰাই 64;
        }
        ঘূৰাই maxval;
    }
    AstVarRef* enumVarRefp(AstEnumDType* ধ্রুৱ nodep, VAttrType attrType, তার্কিক assoc,
                           uint32_t msbdim) {
        // Return a reference to a constant table which has the specified attrType information
        // for this enum. The reference is cached, so the table is built only once.
        AstVarRef*& cachedpr = m_tableMap[std::make_pair(nodep, attrType)];
        যদি (cachedpr) ঘূৰাই cachedpr->clone_বৃক্ষ(মিছা);

        UINFO(9, "Construct Venumtab attr=" << attrType.ascii() << " assoc=" << assoc
                                            << " max=" << msbdim << " for " << nodep);
        File_সাৰি* ধ্রুৱ flp = nodep->fileline();
        Ast_গ্ৰন্থি_DType* basep;
        যদি (attrType == VAttrType::ENUM_NAME) {
            basep = nodep->findStringDType();
        } অন্যথা যদি (attrType == VAttrType::ENUM_VALID) {
            // TODO in theory we could bit-pack the bits in the table, but
            // would require additional operations to extract, so only
            // would be worth it for larger tables which perhaps could be
            // better handled with equation generation?
            basep = nodep->findBitDType();
        } অন্যথা {
            basep = nodep->dtypep();
        }
        Ast_গ্ৰন্থি_DType* vardtypep;
        যদি (assoc) {
            // Key by the base type of the enum, not the enum itself, so identical maps of
            // different enum types can be shared in the constant pool
            vardtypep = নূতন AstAssocArrayDType{flp, basep, nodep->subDTypep()};
        } অন্যথা {
            vardtypep = নূতন AstUnpackArrayDType{flp, basep, নূতন AstRange(flp, msbdim, 0)};
        }
        v3Global.rootp()->typeTablep()->addTypesp(vardtypep);

        // Default for all unspecified values
        AstConst* defaultp = nullptr;
        যদি (attrType == VAttrType::ENUM_NAME) {
            defaultp = নূতন AstConst{flp, AstConst::String{}, ""};
        } অন্যথা যদি (attrType == VAttrType::ENUM_NEXT || attrType == VAttrType::ENUM_PREV) {
            defaultp = নূতন AstConst{flp, V3_সংখ্যা{nodep, nodep->width(), 0}};
        } অন্যথা যদি (attrType == VAttrType::ENUM_VALID) {
            defaultp = নূতন AstConst{flp, AstConst::BitFalse{}};
        } অন্যথা {
            nodep->v3fatalSrc("Bad case");
        }

        // Find valid values and populate
        UASSERT_OBJ(nodep->itemsp(), nodep, "enum without items");
        std::map<uint64_t, Ast_গ্ৰন্থি_ৰাশিমালা*> মানসমূহ;
        {
            uint64_t ordinal = 0;
            AstEnumItem* ধ্রুৱ firstp = nodep->itemsp();
            ধ্রুৱ AstEnumItem* prevp = firstp;  // Prev must start with last item
            যেতিয়ালৈ (prevp->nextp()) prevp = VN_AS(prevp->nextp(), EnumItem);
            বাবে (AstEnumItem* itemp = firstp; itemp;) {
                AstEnumItem* ধ্রুৱ nextp = VN_AS(itemp->nextp(), EnumItem);
                ধ্রুৱ AstConst* ধ্রুৱ vconstp = VN_AS(itemp->valuep(), Const);
                UASSERT_OBJ(vconstp, nodep, "Enum item without constified value");
                যদি (!vconstp->অঙ্ক().isAnyXZ()) {  // Can 2-state runtime decode
                    ধ্রুৱ uint64_t i = nodep->isWide() ? ++ordinal : vconstp->toUQuad();
                    যদি (attrType == VAttrType::ENUM_NAME) {
                        মানসমূহ[i] = নূতন AstConst{flp, AstConst::String{},
                                                 V3_সংখ্যা::displayedEnumName(itemp)};
                    } অন্যথা যদি (attrType == VAttrType::ENUM_NEXT) {
                        মানসমূহ[i]
                            = (nextp ? nextp : firstp)->valuep()->clone_বৃক্ষ(মিছা);  // A const
                    } অন্যথা যদি (attrType == VAttrType::ENUM_PREV) {
                        মানসমূহ[i] = prevp->valuep()->clone_বৃক্ষ(মিছা);  // A const
                    } অন্যথা যদি (attrType == VAttrType::ENUM_VALID) {
                        মানসমূহ[i] = নূতন AstConst{flp, AstConst::BitTrue{}};
                    } অন্যথা {
                        nodep->v3fatalSrc("Bad case");
                    }
                }
                prevp = itemp;
                itemp = nextp;
            }
        }
        // Add all specified values to table
        Ast_গ্ৰন্থি_ৰাশিমালা* valuep = nullptr;
        যদি (assoc) {
            // Keyed by the enum's base type
            AstConsAssoc* ধ্রুৱ consp = নূতন AstConsAssoc{flp, defaultp};
            consp->dtypep(vardtypep);
            বাবে (ধ্রুৱ auto& itr : মানসমূহ) {
                V3_সংখ্যা চাবি{nodep, nodep->width()};
                চাবি.setQuad(itr.first);
                consp->addItemsp(নূতন AstConsAssocItem{flp, নূতন AstConst{flp, চাবি}, itr.second});
            }
            valuep = consp;
        } অন্যথা {
            AstConsArray* ধ্রুৱ consp = নূতন AstConsArray{flp, vardtypep, defaultp};
            বাবে (uint64_t i = 0; i < (msbdim + 1); ++i) {
                যদি (মানসমূহ[i]) consp->valuep(i, মানসমূহ[i]);
            }
            valuep = consp;
        }

        // Share identical tables via the constant pool
        cachedpr = V3ConstPool::find(valuep);
        VL_DO_DANGLING(ঠেলো_Deletep(valuep), valuep);  // V3ConstPool::find clones it
        cachedpr->varp()->didWidth(সঁচা);
        // Keep the dtype of the value, as the entry might have a different, compatible one
        cachedpr->didWidth(সঁচা);
        ঠেলো_Deletep(cachedpr);  // Deleted with the visitor - always cloned
        ঘূৰাই cachedpr->clone_বৃক্ষ(মিছা);
    }

    Ast_গ্ৰন্থি_ৰাশিমালা* enumSelect(Ast_গ্ৰন্থি_ৰাশিমালা* nodep, AstEnumDType* adtypep, VAttrType attrType) {
        // Return expression to get given attrType information from a enum's value (nodep)
        যদি (attrType == VAttrType::ENUM_NAME) {
            যদি (nodep->isPure()) nodep = V3Const::constifyEdit(nodep);
            যদি (ধ্রুৱ AstConst* ধ্রুৱ constp = VN_CAST(nodep, Const)) {
                Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ newp = নূতন AstConst{nodep->fileline(), AstConst::String{},
                                                       constp->অঙ্ক().displayedEnumName(adtypep)};
                VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
                ঘূৰাই newp;
            }
        }
        যদি (attrType == VAttrType::ENUM_NAME && !nodep->isPure() && (m_modep || m_ftaskp)) {
            Ast_ৰাশিমালা_Stmt* ধ্রুৱ capturep = newEnumCapture(nodep, adtypep);
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ valuep = capturep->resultp()->unlinkFrBack();
            capturep->resultp(enumSelect(valuep, adtypep, attrType));
            capturep->dtypeSetString();
            ঘূৰাই capturep;
        }
        যদি (attrType == VAttrType::ENUM_NAME && adtypep->isWide()) {
            // Wide values select an ordinal in the shared name table.
            Ast_গ্ৰন্থি_ৰাশিমালা* indexp = নূতন AstConst{nodep->fileline(), 0};
            uint32_t ordinal = 0;
            বাবে (ধ্রুৱ AstEnumItem* itemp = adtypep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), EnumItem)) {
                ধ্রুৱ AstConst* ধ্রুৱ constp = VN_AS(itemp->valuep(), Const);
                যদি (constp->অঙ্ক().isAnyXZ()) অনুৱর্ত;
                indexp = নূতন AstCond{nodep->fileline(),
                                     নূতন AstEq{nodep->fileline(), nodep->clone_বৃক্ষ(মিছা),
                                               itemp->valuep()->clone_বৃক্ষ(মিছা)},
                                     নূতন AstConst{nodep->fileline(), ++ordinal}, indexp};
            }
            AstVarRef* ধ্রুৱ tabRefp = enumVarRefp(adtypep, attrType, মিছা, ordinal);
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ newp = নূতন AstArraySel{nodep->fileline(), tabRefp, indexp};
            newp->dtypeSetString();
            VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
            ঘূৰাই newp;
        }
        ধ্রুৱ uint64_t msbdim = enum_Max_মান(nodep, adtypep);
        ধ্রুৱ তার্কিক assoc = msbdim > ENUM_LOOKUP_BITS;
        Ast_গ্ৰন্থি_ৰাশিমালা* newp;
        যদি (assoc) {
            AstVarRef* ধ্রুৱ tabRefp = enumVarRefp(adtypep, attrType, সঁচা, 0);
            newp = নূতন AstAssocSel{nodep->fileline(), tabRefp, nodep};
        } অন্যথা {
            ধ্রুৱ পূর্ণাঙ্ক selwidth = V3_সংখ্যা::log2b(msbdim) + 1;  // Width to address a bit
            AstVarRef* ধ্রুৱ tabRefp
                = enumVarRefp(adtypep, attrType, মিছা, (1ULL << selwidth) - 1);
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ boundp
                = attrType == VAttrType::ENUM_NAME
                          && (adtypep->width() > selwidth || msbdim != (1ULL << selwidth) - 1)
                      ? নূতন AstGt{nodep->fileline(), nodep->clone_বৃক্ষ(মিছা),
                                  নূতন AstConst{nodep->fileline(),
                                               V3_সংখ্যা{nodep, adtypep->width(),
                                                        static_cast<uint32_t>(msbdim)}}}
                      : nullptr;
            newp = নূতন AstArraySel{
                nodep->fileline(), tabRefp,
                // Select in case widths are off due to msblen!=width
                // We return "random" values if outside the range, which is fine
                // as next/previous on illegal values just need something good out
                নূতন AstSel{nodep->fileline(), nodep, 0, selwidth}};
            যদি (boundp) {
                newp = নূতন AstCond{nodep->fileline(), boundp,
                                   নূতন AstConst{nodep->fileline(), AstConst::String{}, ""}, newp};
            }
        }
        যদি (attrType == VAttrType::ENUM_NAME) {
            newp->dtypeSetString();
        } অন্যথা যদি (attrType == VAttrType::ENUM_VALID) {
            newp->dtypeSetBit();
        } অন্যথা {
            newp->dtypeFrom(adtypep);  // To prevent a later ENUMVALUE
        }
        ঘূৰাই newp;
    }
    Ast_গ্ৰন্থি_ৰাশিমালা* enumTestValid(Ast_গ্ৰন্থি_ৰাশিমালা* valp, AstEnumDType* enumDtp) {
        ধ্রুৱ uint64_t maxval = enum_Max_মান(valp, enumDtp);
        ধ্রুৱ তার্কিক assoc = maxval > ENUM_LOOKUP_BITS;
        Ast_গ্ৰন্থি_ৰাশিমালা* testp = nullptr;
        File_সাৰি* ধ্রুৱ fl_novalue = নূতন File_সাৰি{valp->fileline()};
        fl_novalue->warnOff(V3ErrorCode::ENUMVALUE, সঁচা);
        fl_novalue->warnOff(V3ErrorCode::CMPCONST, সঁচা);
        যদি (assoc) {
            AstVarRef* ধ্রুৱ tabRefp = enumVarRefp(enumDtp, VAttrType::ENUM_VALID, সঁচা, 0);
            testp = নূতন AstAssocSel{fl_novalue, tabRefp, valp->clone_বৃক্ষ_Pure(মিছা)};
        } অন্যথা {
            ধ্রুৱ পূর্ণাঙ্ক selwidth = V3_সংখ্যা::log2b(maxval) + 1;  // Width to address a bit
            AstVarRef* ধ্রুৱ tabRefp
                = enumVarRefp(enumDtp, VAttrType::ENUM_VALID, মিছা, (1ULL << selwidth) - 1);
            File_সাৰি* ধ্রুৱ fl_nowidth = নূতন File_সাৰি{fl_novalue};
            fl_nowidth->warnOff(V3ErrorCode::WIDTH, সঁচা);
            testp = নূতন AstCond{
                fl_novalue,
                নূতন AstGt{fl_nowidth, valp->clone_বৃক্ষ_Pure(মিছা),
                          নূতন AstConst{fl_nowidth, AstConst::Unsized64{}, maxval}},
                নূতন AstConst{fl_novalue, AstConst::BitFalse{}},
                নূতন AstArraySel{fl_novalue, tabRefp,
                                নূতন AstSel{fl_novalue, valp->clone_বৃক্ষ_Pure(মিছা), 0, selwidth}}};
        }
        ঘূৰাই testp;
    }

    PatVecMap patVectorMap(AstPattern* nodep, ধ্রুৱ VNumRange& range) {
        PatVecMap patmap;
        পূর্ণাঙ্ক element = range.বাম();
        বাবে (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;
             patp = VN_AS(patp->nextp(), PatMember)) {
            যদি (patp->keyp()) {
                যদি (ধ্রুৱ AstConst* ধ্রুৱ constp = patternConstKeyp(patp)) {
                    যদি (constp->অঙ্ক().isFourState()) {
                        patp->keyp()->v3error("Assignment pattern key cannot contain X or Z");
                    } অন্যথা {
                        element = constp->toSInt();
                    }
                }
            }
            ধ্রুৱ তার্কিক newEntry = patmap.emplace(element, patp).second;
            যদি (!newEntry) {
                patp->v3error("Assignment pattern key used multiple times: " << element);
            }
            element += range.বাম_To_ডান_Inc();
        }
        ঘূৰাই patmap;
    }

    // Add to patternp the items of nodep, a concatenation or a single item
    শূন্য patConcatConvertRecurse(AstPattern* patternp, Ast_গ্ৰন্থি_ৰাশিমালা* nodep) {
        যদি (AstConcat* ধ্রুৱ concatp = VN_CAST(nodep, Concat)) {
            patConcatConvertRecurse(patternp, concatp->lhsp());
            patConcatConvertRecurse(patternp, concatp->rhsp());
        } অন্যথা {
            AstPatMember* ধ্রুৱ newp
                = নূতন AstPatMember{nodep->fileline(), nodep->unlinkFrBack(), nullptr, nullptr};
            newp->isConcat(সঁচা);
            patternp->addItemsp(newp);
        }
    }

    শূন্য makeOpenArrayShell(Ast_গ্ৰন্থি_FTask_Ref* nodep) {
        UINFO(4, "Replicate openarray function " << nodep->taskp());
        Ast_গ্ৰন্থি_FTask* ধ্রুৱ oldTaskp = nodep->taskp();
        oldTaskp->dpiOpenParentInc();
        UASSERT_OBJ(!oldTaskp->dpiOpenChild(), oldTaskp,
                    "DPI task should be parent or child, not both");
        Ast_গ্ৰন্থি_FTask* ধ্রুৱ newTaskp = oldTaskp->clone_বৃক্ষ(মিছা);
        newTaskp->dpiOpenChild(সঁচা);
        newTaskp->dpiOpenParentClear();
        newTaskp->name(newTaskp->name() + "__Vdpioc" + cvtToStr(oldTaskp->dpiOpenParent()));
        oldTaskp->addNextHere(newTaskp);
        // Relink reference to new function
        nodep->taskp(newTaskp);
        nodep->name(nodep->taskp()->name());
        // Replace open array arguments with the callee's task
        ধ্রুৱ V3TaskConnects tconnects = V3Task::taskConnects(nodep, nodep->taskp()->stmtsp());
        বাবে (ধ্রুৱ auto& tconnect : tconnects) {
            AstVar* ধ্রুৱ portp = tconnect.first;
            ধ্রুৱ AstArg* ধ্রুৱ argp = tconnect.second;
            ধ্রুৱ Ast_গ্ৰন্থি* ধ্রুৱ pinp = argp->exprp();
            যদি (!pinp) অনুৱর্ত;  // Argument error we'll find later
            যদি (hasOpenArrayDTypeRecurse(portp->dtypep())) {
                Ast_গ্ৰন্থি_DType* ধ্রুৱ oldDTypep = portp->dtypep();
                portp->dtypep(pinp->dtypep());
                newTaskp->foreach([oldDTypep, portp](Ast_গ্ৰন্থি* nodep) {
                    যদি (nodep->dtypep() == oldDTypep) nodep->dtypep(portp->dtypep());
                });
            }
        }
    }

    তার্কিক markHasOpenArray(Ast_গ্ৰন্থি_FTask* nodep) {
        তার্কিক hasOpen = মিছা;
        বাবে (Ast_গ্ৰন্থি* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            যদি (AstVar* ধ্রুৱ portp = VN_CAST(stmtp, Var)) {
                যদি (portp->isDpiOpenArray() || hasOpenArrayDTypeRecurse(portp->dtypep())) {
                    portp->isDpiOpenArray(সঁচা);
                    hasOpen = সঁচা;
                }
            }
        }
        ঘূৰাই hasOpen;
    }
    তার্কিক hasOpenArrayDTypeRecurse(Ast_গ্ৰন্থি_DType* nodep) {
        // Return true iff this datatype or child has an openarray
        যদি (VN_IS(nodep, UnsizedArrayDType)) ঘূৰাই সঁচা;
        যদি (nodep->subDTypep()) ঘূৰাই hasOpenArrayDTypeRecurse(nodep->subDTypep()->skipRefp());
        ঘূৰাই মিছা;
    }
    Ast_গ্ৰন্থি_DType* dtypeNotIntAtomOrVecRecurse(Ast_গ্ৰন্থি_DType* nodep, তার্কিক ranged = মিছা) {
        // If node is _not_ integer or atomic, return node that makes it fail
        nodep = nodep->skipRefToEnump();
        যদি (AstBasicDType* ধ্রুৱ dtp = VN_CAST(nodep, BasicDType)) {
            যদি (ranged && (!dtp->isBitLogic() || dtp->isRanged()))
                ঘূৰাই dtp;  // Packed when already packed
            যদি (dtp->keyword().isIntNumeric()) ঘূৰাই nullptr;
            ঘূৰাই dtp;
        } অন্যথা যদি (AstPackArrayDType* ধ্রুৱ dtp = VN_CAST(nodep, PackArrayDType)) {
            যদি (ranged) ঘূৰাই dtp;  // Packed when already packed
            ঘূৰাই dtypeNotIntAtomOrVecRecurse(nodep->subDTypep(), সঁচা);
        }
        ঘূৰাই nodep;
    }
    Ast_গ্ৰন্থি_DType* dtypeNot4StateIntegralRecurse(Ast_গ্ৰন্থি_DType* nodep) {
        // If node is _not_ inet valid data type, 4-state integral packed or union, return node
        // that makes it fail
        nodep = nodep->skipRefp();
        যদি (AstBasicDType* ধ্রুৱ dtp = VN_CAST(nodep, BasicDType)) {
            যদি (!dtp->keyword().isFourstate()) ঘূৰাই dtp;
            ঘূৰাই nullptr;
        } অন্যথা যদি (Ast_গ্ৰন্থি_Array_DType* ধ্রুৱ dtp = VN_CAST(nodep, গ্ৰন্থি_Array_DType)) {
            ঘূৰাই dtypeNot4StateIntegralRecurse(dtp->subDTypep());
        } অন্যথা যদি (Ast_গ্ৰন্থি_UOr_Struct_DType* ধ্রুৱ dtp = VN_CAST(nodep, গ্ৰন্থি_UOr_Struct_DType)) {
            বাবে (AstMemberDType* itemp = dtp->membersp(); itemp;
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {
                Ast_গ্ৰন্থি_DType* ধ্রুৱ badDtp = dtypeNot4StateIntegralRecurse(itemp->dtypep());
                যদি (badDtp) ঘূৰাই badDtp;
            }
            ঘূৰাই nullptr;
        }
        ঘূৰাই nodep;
    }
    শূন্য checkForceReleaseLhs(Ast_গ্ৰন্থি* nodep, Ast_গ্ৰন্থি* lhsp) {
        // V3Force can't check as vector may have expanded, or propagated constant into index
        যদি (Ast_গ্ৰন্থি* ধ্রুৱ selNodep = V3Width::selectNonConstantRecurse(lhsp))
            nodep->v3error((VN_IS(nodep, Release) ? "Release"s : "Force"s)
                               + " left-hand-side must not have variable bit/part select "
                                 "(IEEE 1800-2023 10.6.2)\n"
                           << nodep->warnContextPrimary() << '\n'
                           << selNodep->warnOther() << "... Location of non-constant index\n"
                           << selNodep->warnContextSecondary());
    }

    //----------------------------------------------------------------------
    // METHODS - special type detection

    শূন্য assert_At_ৰাশিমালা(Ast_গ্ৰন্থি* nodep) {
        যদি (VL_UNCOVERABLE(!m_vup)) {
            nodep->v3fatalSrc("Unexpected '" << nodep->prettyTypeName() << "' expression under '"
                                             << nodep->backp()->prettyTypeName() << "'");
        }
    }
    শূন্য assertAtStatement(Ast_গ্ৰন্থি* nodep) {
        যদি (VL_UNCOVERABLE(m_vup && !m_vup->selfDtm())) {
            UINFO(1, "-: " << m_vup);
            nodep->v3fatalSrc("No dtype expected at statement " << nodep->prettyTypeName());
        }
    }
    শূন্য checkConstantOrReplace(Ast_গ্ৰন্থি* nodep, তার্কিক noFourState, ধ্রুৱ string& message) {
        // See also V3WidthSel::checkConstantOrReplace
        // Note can't call V3Const::constifyParam(nodep) here, as constify may change nodep on us!
        AstConst* ধ্রুৱ constp = VN_CAST(nodep, Const);
        যদি (!constp || (noFourState && constp->অঙ্ক().isFourState())) {
            nodep->v3error(message);
            nodep->replaceWith(নূতন AstConst{nodep->fileline(), AstConst::Unsized32{}, 1});
            VL_DO_DANGLING2(ঠেলো_Deletep(nodep), nodep, constp);
            ঘূৰাই;
        }
    }
    Ast_গ্ৰন্থি* গ্ৰন্থি_For_Unsized_Warning(Ast_গ্ৰন্থি* nodep) {
        // Return a nodep to use for unsized warnings, reporting on child if can
        যদি (nodep->op1p() && nodep->op1p()->dtypep() && !nodep->op1p()->dtypep()->widthSized()) {
            ঘূৰাই nodep->op1p();
        } অন্যথা যদি (nodep->op2p() && nodep->op2p()->dtypep()
                   && !nodep->op2p()->dtypep()->widthSized()) {
            ঘূৰাই nodep->op2p();
        }
        ঘূৰাই nodep;  // By default return this
    }
    AstRefDType* checkRefToTypedefRecurse(Ast_গ্ৰন্থি* nodep, AstTypedef* typedefp) {
        // Recurse all children looking for self reference
        // This avoids iterateEditMoveDTypep going into a hard to resolve loop
        // Only call once for any given typedef, or will become O(n^2)
        যদি (VL_LIKELY(!nodep)) ঘূৰাই nullptr;
        যদি (auto* ধ্রুৱ refp = VN_CAST(nodep, RefDType)) {
            যদি (refp->typedefp() == typedefp) ঘূৰাই refp;
        }
        যদি (auto* ধ্রুৱ refp = checkRefToTypedefRecurse(nodep->op1p(), typedefp)) ঘূৰাই refp;
        যদি (auto* ধ্রুৱ refp = checkRefToTypedefRecurse(nodep->op2p(), typedefp)) ঘূৰাই refp;
        যদি (auto* ধ্রুৱ refp = checkRefToTypedefRecurse(nodep->op3p(), typedefp)) ঘূৰাই refp;
        যদি (auto* ধ্রুৱ refp = checkRefToTypedefRecurse(nodep->op4p(), typedefp)) ঘূৰাই refp;
        ঘূৰাই nullptr;
    }

    //----------------------------------------------------------------------
    // METHODS - interface arrays

    // True if an unpacked array of non-virtual interfaces
    স্থির তার্কিক isIfaceArrayDType(ধ্রুৱ Ast_গ্ৰন্থি_DType* dtypep) {
        যদি (!VN_IS(dtypep->skipRefp(), UnpackArrayDType)) ঘূৰাই মিছা;
        ধ্রুৱ AstIfaceRefDType* ধ্রুৱ irefp = VN_CAST(dtypep->elemDTypep(), IfaceRefDType);
        ঘূৰাই irefp && !irefp->isVirtual();
    }

    // Number of elements of each unpacked dimension of 'dtypep', from the left, e.g. "2x3"
    স্থির std::string unpackedArraySize(ধ্রুৱ Ast_গ্ৰন্থি_DType* dtypep) {
        std::string size;
        dtypep = dtypep->skipRefp();
        যেতিয়ালৈ (ধ্রুৱ AstUnpackArrayDType* ধ্রুৱ arrp = VN_CAST(dtypep, UnpackArrayDType)) {
            যদি (!size.empty()) size += "x";
            size += std::to_string(arrp->elementsConst());
            dtypep = arrp->subDTypep()->skipRefp();
        }
        ঘূৰাই size;
    }

    // A whole array of interfaces is an AstConsArray of references to its elements, so select
    // its element 'bit' (counted from lo) directly. Returns nullptr if not such an array, or
    // 'bit' is out of range.
    স্থির Ast_গ্ৰন্থি_ৰাশিমালা* newIfaceArrayElement(Ast_গ্ৰন্থি_ৰাশিমালা* fromp, পূর্ণাঙ্ক bit) {
        AstConsArray* ধ্রুৱ consp = VN_CAST(fromp, ConsArray);
        যদি (!consp || !isIfaceArrayDType(consp->dtypep())) ঘূৰাই nullptr;
        ধ্রুৱ AstUnpackArrayDType* ধ্রুৱ arrp
            = VN_AS(consp->dtypep()->skipRefp(), UnpackArrayDType);
        যদি (bit < 0 || bit >= arrp->elementsConst()) ঘূৰাই nullptr;
        ঘূৰাই consp->valuep(bit)->clone_বৃক্ষ(মিছা);
    }

    // Widthed references to the element variables of the whole interface array referenced by
    // 'refp', in the dimensions of 'dtypep' inwards. 'name' is the element name so far, the
    // element variables being named 'name' + 'suffix'. 'elemVarpr' is the element variable of
    // the leftmost element in the dimensions of 'dtypep', and is moved to the right past them.
    // E.g. 'ifc' of dimensions [0:1][3:2] -> '{'{ifc[0][3], ifc[0][2]}, '{ifc[1][3], ifc[1][2]}}
    Ast_গ্ৰন্থি_ৰাশিমালা* new_Iface_Array_মান(ধ্রুৱ Ast_গ্ৰন্থি_Var_Ref* refp, Ast_গ্ৰন্থি_DType* dtypep,
                                    ধ্রুৱ std::string& name, ধ্রুৱ std::string& suffix,
                                    AstVar*& elemVarpr) {
        File_সাৰি* ধ্রুৱ flp = refp->fileline();
        AstUnpackArrayDType* ধ্রুৱ arrp = VN_CAST(dtypep->skipRefp(), UnpackArrayDType);
        // Base case: reference the element variable when no dimensions left
        যদি (!arrp) {
            AstVar* ধ্রুৱ varp = elemVarpr;
            UASSERT_OBJ(varp && varp->name() == name + suffix, refp,
                        "Missing element variable of interface array " << name);
            elemVarpr = VN_CAST(varp->nextp(), Var);
            Ast_গ্ৰন্থি_Var_Ref* newp;
            যদি (ধ্রুৱ AstVarXRef* ধ্রুৱ xrefp = VN_CAST(refp, VarXRef)) {
                newp = নূতন AstVarXRef{flp, varp, xrefp->dotted(), refp->access()};
            } অন্যথা {
                newp = নূতন AstVarRef{flp, varp, refp->access()};
            }
            userIterate(newp, WidthVP{SELF, BOTH}.p());
            ঘূৰাই newp;
        }

        // Enumerate the current dimension given by 'arrp', from the left index to the right,
        // as the element variables are in that order, indexed from lo
        AstConsArray* ধ্রুৱ consp = নূতন AstConsArray{flp, arrp};
        ধ্রুৱ VNumRange range = arrp->declRange();
        বাবে (ধ্রুৱ পূর্ণাঙ্ক i : range.seq_বাম_To_ডান()) {
            ধ্রুৱ std::string s = name + "__BRA__" + Ast_গ্ৰন্থি::encode_সংখ্যা(i) + "__KET__";
            consp->valuep(i - range.lo(),
                          new_Iface_Array_মান(refp, arrp->subDTypep(), s, suffix, elemVarpr));
        }
        ঘূৰাই consp;
    }

    // Replace a reference to a whole interface array variable (a __Viftop companion, or an
    // interface array port) split by V3Param, with the AstConsArray of references to its
    // elements, which is how whole arrays of interfaces are handled from here on
    শূন্য replaceSplitIfaceArrayRef(Ast_গ্ৰন্থি_Var_Ref* nodep) {
        AstVar* ধ্রুৱ varp = nodep->varp();
        UASSERT_OBJ(varp->isIfaceArraySplit(), nodep, "Not a split interface array");
        std::string name = varp->name();
        std::string suffix;
        যদি (varp->isIfaceParent()) {
            name = name.substr(0, name.rfind("__Viftop"));
            suffix = "__Viftop";
        }
        // The element variables follow the whole array variable in order (see V3Param)
        AstVar* elemVarp = VN_CAST(varp->nextp(), Var);
        nodep->replaceWith(new_Iface_Array_মান(nodep, varp->dtypep(), name, suffix, elemVarp));
        VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
    }

    // Add a pin for each element port of interface array port of pin 'nodep', split by V3Param,
    // in the dimensions of 'dtypep' inwards, connected to the element at the same position,
    // counted from the left, of 'conp', which is the corresponding (sub)array of the pin's
    // connection, if any. 'suffix' is the name suffix of the dimensions outside 'dtypep'.
    // 'elemVarpr' is the element port variable of the rightmost element in the dimensions of
    // 'dtypep', and is moved to the left past them.
    শূন্য expandIfaceArrayPinDimensions(AstPin* nodep, ধ্রুৱ Ast_গ্ৰন্থি_DType* dtypep,
                                       Ast_গ্ৰন্থি_ৰাশিমালা* conp, ধ্রুৱ std::string& suffix,
                                       AstVar*& elemVarpr) {
        ধ্রুৱ AstUnpackArrayDType* ধ্রুৱ arrp = VN_CAST(dtypep->skipRefp(), UnpackArrayDType);
        // Base case: add the pin of the element port after 'nodep' when no dimensions left
        যদি (!arrp) {
            UASSERT_OBJ(elemVarpr && elemVarpr->name() == nodep->modVarp()->name() + suffix, nodep,
                        "Missing element variable of interface array port " << suffix);
            AstPin* ধ্রুৱ newp = nodep->clone_বৃক্ষ(মিছা);
            newp->name(nodep->name() + suffix);
            newp->modVarp(elemVarpr);
            elemVarpr = VN_CAST(elemVarpr->backp(), Var);
            যদি (conp) newp->exprp(conp->clone_বৃক্ষ(মিছা));
            nodep->addNextHere(newp);
            ঘূৰাই;
        }

        // Enumerate the current dimension given by 'arrp'
        // Each element is added right after 'nodep', so go from right to left,
        // to end with an enumeration from the left index to the right index.
        ধ্রুৱ VNumRange range = arrp->declRange();
        বাবে (পূর্ণাঙ্ক n = range.elements() - 1; n >= 0; --n) {
            ধ্রুৱ পূর্ণাঙ্ক i = range.বাম() + n * range.বাম_To_ডান_Inc();
            ধ্রুৱ std::string s = suffix + "__BRA__" + Ast_গ্ৰন্থি::encode_সংখ্যা(i) + "__KET__";
            // Element of the connection, if any, which is only cloned at the leaves
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ elemConnp = [&]() -> Ast_গ্ৰন্থি_ৰাশিমালা* {
                যদি (!conp) ঘূৰাই nullptr;
                ধ্রুৱ AstUnpackArrayDType* ধ্রুৱ conArrp
                    = VN_AS(conp->dtypep()->skipRefp(), UnpackArrayDType);
                ধ্রুৱ পূর্ণাঙ্ক elements = conArrp->elementsConst();
                // Counted from lo
                Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ resultp
                    = VN_AS(conp, ConsArray)
                          ->valuep(conArrp->declRange().ascending() ? n : elements - 1 - n);
                UASSERT_OBJ(resultp, conp, "Interface array connection of mismatched shape");
                ঘূৰাই resultp;
            }();
            expandIfaceArrayPinDimensions(nodep, arrp->subDTypep(), elemConnp, s, elemVarpr);
        }
    }

    // Replace the given interface array port pin with a pin for each element port split by V3Param
    শূন্য expandIfaceArrayPin(AstPin* nodep) {
        AstVar* ধ্রুৱ portp = nodep->modVarp();
        UASSERT_OBJ(portp->isIfaceArraySplit(), nodep, "Not a split interface array port");
        // The element variables follow the port in order (see V3Param). Pins are added from the
        // right, so start at the last.
        AstVar* elemVarp = portp;
        বাবে (uint32_t n = portp->dtypep()->arrayUnpackedElements(); n; --n) {
            elemVarp = VN_CAST(elemVarp->nextp(), Var);
            UASSERT_OBJ(elemVarp, portp, "Missing element variables of interface array port");
        }
        // Remove the connection, so the element pins do not clone it all. Deletion is deferred,
        // so its elements can still be cloned from.
        Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ exprp = VN_AS(nodep->exprp(), গ্ৰন্থি_ৰাশিমালা);
        যদি (exprp) ঠেলো_Deletep(exprp->unlinkFrBack());
        // Connect nothing if not a whole array of interfaces of the shape of the port, which is
        // reported elsewhere, so the element pins do not report it again
        ধ্রুৱ তার্কিক connect
            = exprp && isIfaceArrayDType(exprp->dtypep())
              && unpackedArraySize(exprp->dtypep()) == unpackedArraySize(portp->dtypep());
        expandIfaceArrayPinDimensions(nodep, portp->dtypep(), connect ? exprp : nullptr, "",
                                      elemVarp);
        UASSERT_OBJ(elemVarp == portp, portp, "Mismatched element port variable");
        VL_DO_DANGLING(ঠেলো_Deletep(nodep->unlinkFrBack()), nodep);
    }

    //----------------------------------------------------------------------
    // METHODS - special iterators
    // These functions save/restore the AstNUser information so it can pass to child nodes.

    Ast_গ্ৰন্থি* userIterateSubtreeReturnEdits(Ast_গ্ৰন্থি* nodep, WidthVP* vup) {
        যদি (!nodep) ঘূৰাই nullptr;
        Ast_গ্ৰন্থি* ret;
        {
            VL_RESTORER(m_vup);
            m_vup = vup;
            ret = iterateSubtreeReturnEdits(nodep);
        }
        ঘূৰাই ret;
    }
    শূন্য userIterate(Ast_গ্ৰন্থি* nodep, WidthVP* vup) {
        যদি (!nodep) ঘূৰাই;
        VL_RESTORER(m_vup);
        m_vup = vup;
        iterate(nodep);
    }
    শূন্য userIterateAndNext(Ast_গ্ৰন্থি* nodep, WidthVP* vup) {
        যদি (!nodep) ঘূৰাই;
        যদি (nodep->didWidth()) ঘূৰাই;  // Avoid iterating list we have already iterated
        VL_RESTORER(m_vup);
        m_vup = vup;
        iterateAndNextNull(nodep);
    }
    শূন্য userIterateChildren(Ast_গ্ৰন্থি* nodep, WidthVP* vup) {
        যদি (!nodep) ঘূৰাই;
        VL_RESTORER(m_vup);
        m_vup = vup;
        iterateChildren(nodep);
    }
    শূন্য userIterateChildrenBackwardsConst(Ast_গ্ৰন্থি* nodep, WidthVP* vup) {
        যদি (!nodep) ঘূৰাই;
        VL_RESTORER(m_vup);
        m_vup = vup;
        iterateChildrenBackwardsConst(nodep);
    }

সার্ৱজনিক:
    // CONSTRUCTORS
    WidthVisitor(তার্কিক paramsOnly,  // [in] TRUE if we are considering parameters only.
                 তার্কিক doGenerate)  // [in] TRUE if we are inside a generate statement and
        //                           // don't wish to trigger errors
        : m_enumTempNames{"__Venum"}
        , m_insideTempNames{"__VInside"}
        , m_paramsOnly{paramsOnly}
        , m_doGenerate{doGenerate} {}
    Ast_গ্ৰন্থি* mainAcceptEdit(Ast_গ্ৰন্থি* nodep) {
        nodep = userIterateSubtreeReturnEdits(nodep, WidthVP{SELF, BOTH}.p());
        // All references to the whole interface array variables split by V3Param are now replaced
        বাবে (AstVar* ধ্রুৱ varp : m_splitIfaceArrayVarps) {
            VL_DO_DANGLING(ঠেলো_Deletep(varp->unlinkFrBack()), varp);
        }
        m_splitIfaceArrayVarps.clear();
        ঘূৰাই nodep;
    }
    স্থির তার্কিক lowerAsFixedAggregate(ধ্রুৱ Ast_গ্ৰন্থি_DType* ধ্রুৱ dtypep) {
        ঘূৰাই dtypep->isStreamableFixedAggregate() && dtypep->containsUnpackedStruct();
    }
    শূন্য streamAssignLower(Ast_গ্ৰন্থি_Assign* nodep) {
        Ast_গ্ৰন্থি_DType* lhsDTypep = nodep->lhsp()->dtypep();
        // Check width of stream and wrap if needed
        যদি (Ast_গ্ৰন্থি_Stream* ধ্রুৱ streamp = VN_CAST(nodep->rhsp(), গ্ৰন্থি_Stream)) {
            Ast_গ্ৰন্থি_DType* ধ্রুৱ lhsDTypeSkippedRefp = lhsDTypep->skipRefp();
            ধ্রুৱ পূর্ণাঙ্ক lwidth = lhsDTypeSkippedRefp->widthStream();
            ধ্রুৱ পূর্ণাঙ্ক rwidth = streamp->lhsp()->dtypep()->skipRefp()->widthStream();
            যদি (lwidth != 0 && lwidth < rwidth) {
                nodep->v3widthWarn(lwidth, rwidth,
                                   "Target fixed size variable ("
                                       << lwidth << " bits) is narrower than the stream ("
                                       << rwidth << " bits) (IEEE 1800-2023 11.4.14)");
            }
            যদি (VN_IS(streamp->lhsp()->dtypep()->skipRefp(), QueueDType)
                && !VN_IS(nodep->lhsp()->dtypep()->skipRefp(), QueueDType)) {
                ধ্রুৱ পূর্ণাঙ্ক queueElementSize = streamp->lhsp()->dtypep()->subDTypep()->width();
                UASSERT_OBJ(queueElementSize <= lwidth, nodep, "LHS < RHS");
            }
            যদি (VN_IS(lhsDTypeSkippedRefp, UnpackArrayDType)
                || lowerAsFixedAggregate(lhsDTypeSkippedRefp)) {
                streamp->unlinkFrBack();
                nodep->rhsp(
                    নূতন AstCvtPackedToArray{streamp->fileline(), streamp, lhsDTypeSkippedRefp});
            }
        }
        যদি (Ast_গ্ৰন্থি_Stream* ধ্রুৱ streamp = VN_CAST(nodep->lhsp(), গ্ৰন্থি_Stream)) {
            ধ্রুৱ Ast_গ্ৰন্থি_DType* ধ্রুৱ rhsDTypep = nodep->rhsp()->dtypep()->skipRefp();
            Ast_গ্ৰন্থি_DType* ধ্রুৱ lhsStreamDTypep = streamp->lhsp()->dtypep()->skipRefp();
            ধ্রুৱ পূর্ণাঙ্ক lwidth = lhsStreamDTypep->widthStream();
            ধ্রুৱ পূর্ণাঙ্ক rwidth = rhsDTypep->widthStream();
            যদি (rwidth != 0 && rwidth < lwidth) {
                nodep->v3widthWarn(lwidth, rwidth,
                                   "Stream target requires "
                                       << lwidth << " bits, but source expression only provides "
                                       << rwidth << " bits (IEEE 1800-2023 11.4.14.3)");
            }
            যদি (lowerAsFixedAggregate(lhsStreamDTypep)) {
                Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ streamExprp = nodep->lhsp()->unlinkFrBack();
                Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ dstp = streamp->lhsp()->unlinkFrBack();
                Ast_গ্ৰন্থি_ৰাশিমালা* srcp = nodep->rhsp()->unlinkFrBack();
                যদি (VN_IS(streamp, StreamL)) {
                    streamp->lhsp(srcp);
                    streamp->dtypeSetBitOrLogicUnsized(
                        srcp->width(), srcp->widthMin(), VSigning::UNSIGNED,
                        Ast_গ্ৰন্থি_ৰাশিমালা::isFourstateOrNoDtype(streamExprp, srcp));
                    srcp = streamExprp;
                } অন্যথা {
                    যদি (srcp->width() > lwidth) {
                        srcp = নূতন AstSel{streamp->fileline(), srcp, srcp->width() - lwidth,
                                          lwidth};
                    }
                    VL_DO_DANGLING(ঠেলো_Deletep(streamExprp), streamExprp);
                }
                nodep->lhsp(dstp);
                nodep->rhsp(নূতন AstCvtPackedToArray{srcp->fileline(), srcp, lhsStreamDTypep});
                nodep->dtypeFrom(dstp);
            } অন্যথা যদি (VN_IS(rhsDTypep, UnpackArrayDType)) {
                Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ rhsp = nodep->rhsp()->unlinkFrBack();
                nodep->rhsp(নূতন AstCvtArrayToPacked{rhsp->fileline(), rhsp, streamp->dtypep()});
            }
        }
    }
    ~WidthVisitor() override = স্ৱভাৱ;
};

//######################################################################
// Width class functions

শূন্য V3Width::width(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        // We should do it in bottom-up module order, but it works in any order.
        ধ্রুৱ WidthClearVisitor cvisitor{nodep};
        // Typedefs and classes are named by the scopes declaring them, so index those
        ধ্রুৱ VDTypeNameScopes scopes{nodep};
        WidthVisitor visitor{মিছা, মিছা};
        (শূন্য)visitor.mainAcceptEdit(nodep);
        WidthRemoveVisitor rvisitor;
        (শূন্য)rvisitor.mainAcceptEdit(nodep);
    }  // Destruct before checking
    V3Global::dump_Check_Global_বৃক্ষ("width", 0, dump_বৃক্ষ_Either_Level() >= 3);
}
//! Single node parameter propagation
//! Smaller step... Only do a single node for parameter propagation
Ast_গ্ৰন্থি* V3Width::widthParamsEdit(Ast_গ্ৰন্থি* nodep) {
    UINFO(4, __FUNCTION__ << ": " << nodep);
    // We should do it in bottom-up module order, but it works in any order.
    WidthVisitor visitor{সঁচা, মিছা};
    nodep = visitor.mainAcceptEdit(nodep);
    // No WidthRemoveVisitor, as don't want to drop $signed etc inside gen blocks
    ঘূৰাই nodep;
}

শূন্য V3Width::streamAssignLowerEdit(Ast_গ্ৰন্থি_Assign* nodep) {
    UINFO(4, __FUNCTION__ << ": " << nodep);
    WidthVisitor visitor{মিছা, মিছা};
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
Ast_গ্ৰন্থি* V3Width::widthGenerateParamsEdit(
    Ast_গ্ৰন্থি* nodep) {  //!< [in] AST whose parameters widths are to be analyzed.
    UINFO(4, __FUNCTION__ << ": " << nodep);
    // We should do it in bottom-up module order, but it works in any order.
    WidthVisitor visitor{সঁচা, সঁচা};
    nodep = visitor.mainAcceptEdit(nodep);
    // No WidthRemoveVisitor, as don't want to drop $signed etc inside gen blocks
    ঘূৰাই nodep;
}

Ast_গ্ৰন্থি* V3Width::selectNonConstantRecurse(Ast_গ্ৰন্থি* nodep, তার্কিক inSel) {
    // If node has a non-constant select, return that select
    Ast_গ্ৰন্থি* resultp = nullptr;
    যদি (Ast_গ্ৰন্থি_Sel* ধ্রুৱ anodep = VN_CAST(nodep, গ্ৰন্থি_Sel)) {
        resultp = selectNonConstantRecurse(anodep->fromp(), inSel);
        যদি (resultp) ঘূৰাই resultp;
        resultp = selectNonConstantRecurse(anodep->bitp(), সঁচা);
    } অন্যথা যদি (AstSel* ধ্রুৱ anodep = VN_CAST(nodep, Sel)) {
        resultp = selectNonConstantRecurse(anodep->fromp(), inSel);
        যদি (resultp) ঘূৰাই resultp;
        resultp = selectNonConstantRecurse(anodep->lsbp(), সঁচা);
    } অন্যথা যদি (Ast_গ্ৰন্থি_Var_Ref* ধ্রুৱ anodep = VN_CAST(nodep, গ্ৰন্থি_Var_Ref)) {
        যদি (inSel && !anodep->varp()->isParam() && !anodep->varp()->isGenVar()) ঘূৰাই anodep;
    } অন্যথা {
        যদি (Ast_গ্ৰন্থি* ধ্রুৱ refp = nodep->op1p()) resultp = selectNonConstantRecurse(refp, inSel);
        যদি (resultp) ঘূৰাই resultp;
        যদি (Ast_গ্ৰন্থি* ধ্রুৱ refp = nodep->op2p()) resultp = selectNonConstantRecurse(refp, inSel);
        যদি (resultp) ঘূৰাই resultp;
        যদি (Ast_গ্ৰন্থি* ধ্রুৱ refp = nodep->op3p()) resultp = selectNonConstantRecurse(refp, inSel);
        যদি (resultp) ঘূৰাই resultp;
        যদি (Ast_গ্ৰন্থি* ধ্রুৱ refp = nodep->op4p()) resultp = selectNonConstantRecurse(refp, inSel);
    }
    ঘূৰাই resultp;
}
