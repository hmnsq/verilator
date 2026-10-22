// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Add temporaries, such as for expand nodes
//
// Code available from: https://verilator.org
//
//*************************************************************************
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of either the GNU Lesser General Public License Version 3
// or the Perl Artistic License Version 2.0.
// SPDX-FileCopyrightText: 2004-2026 Wilson Snyder
// SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0
//
//*************************************************************************
// V3Expand's Transformations:
//
// Each module:
//      Expand verilated.h macros into internal micro optimizations (RTL)
//      this will enable later optimizations.
//      Wide operands become assignments to each word of the vector, (WORDSELs)
//          Note in this case that the widthMin is not correct for the MSW of
//          the vector.  This must be accounted for if doing later constant
//          propagation across signals.
//
//*************************************************************************

#ଅନ୍ତର୍ଭାଵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ଅନ୍ତର୍ଭାଵ "V3Expand.h"

#ଅନ୍ତର୍ଭାଵ "V3Const.h"
#ଅନ୍ତର୍ଭାଵ "V3Stats.h"

#ଅନ୍ତର୍ଭାଵ <algorithm>
#ଅନ୍ତର୍ଭାଵ <array>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// The distinct expansion patterns, for statistics. One entry for each
// unique transformation ExpandVisitor applies. Keep sorted.

// clang-format off
#define FOR_EACH_EXPAND_PATTERN(macro) \
    macro(ASSIGN_AND) \
    macro(ASSIGN_ARRAYSEL) \
    macro(ASSIGN_CONCAT) \
    macro(ASSIGN_COND) \
    macro(ASSIGN_CONST) \
    macro(ASSIGN_EXTEND) \
    macro(ASSIGN_NOT) \
    macro(ASSIGN_OR) \
    macro(ASSIGN_REPLICATE) \
    macro(ASSIGN_SEL_ALIGNED) \
    macro(ASSIGN_SEL_UNALIGNED) \
    macro(ASSIGN_SHIFTL) \
    macro(ASSIGN_SHIFTR) \
    macro(ASSIGN_TO_SEL_CONST_NARROW) \
    macro(ASSIGN_TO_SEL_CONST_WIDE) \
    macro(ASSIGN_TO_SEL_VAR_NARROW) \
    macro(ASSIGN_TO_SEL_VAR_WIDE_ONE_BIT) \
    macro(ASSIGN_VARREF) \
    macro(ASSIGN_XOR) \
    macro(CONCAT) \
    macro(REDAND_TO_EQ) \
    macro(REDOR_TO_NEQ) \
    macro(REPLICATE) \
    macro(REPLICATE_ONE_BIT) \
    macro(SEL_FROM_NARROW) \
    macro(SEL_FROM_WIDE) \
    macro(WORDIZE_EQ) \
    macro(WORDIZE_NEQ) \
    macro(WORDIZE_REDAND) \
    macro(WORDIZE_REDOR) \
    macro(WORDIZE_REDXOR)
// clang-format on

ଶ୍ରେଣୀ VExpandPattern final {
ସର୍ବସାଧାରଣ:
    ଗଣନା en : uint8_t {
#define EXPAND_PATTERN_ID(id) id,
        FOR_EACH_EXPAND_PATTERN(EXPAND_PATTERN_ID)
#undef EXPAND_PATTERN_ID
            _ENUM_END
    };
    ଗଣନା en m_e;
    ଅଚଳ ଅକ୍ଷର* ascii() ଅଚଳ {
        ସ୍ଥିର ଅଚଳ ଅକ୍ଷର* ଅଚଳ names[] = {
#define EXPAND_PATTERN_NAME(id) #id,
            FOR_EACH_EXPAND_PATTERN(EXPAND_PATTERN_NAME)
#undef EXPAND_PATTERN_NAME
                "_ENUM_END"  //
        };
        ଫେରନ୍ତୁ names[m_e];
    }

    // cppcheck-suppress noExplicitConstructor
    VExpandPattern(en _e)
        : m_e{_e} {}
    operator en() ଅଚଳ { ଫେରନ୍ତୁ m_e; }
};

//######################################################################
// Find nodes with side effects, to mark as non-expandable

ଶ୍ରେଣୀ ExpandOkVisitor final : ସର୍ବସାଧାରଣ VNVisitorConst {
    // NODE STATE
    //  AstNode::user2()        -> bool.  Is pure (along with all children)
    ଅଚଳ VNUser2InUse m_inuser2;

    // STATE - for current visit position (use VL_RESTORER)
    // Tracks similar to AstNode::isTreePureRecurse(), but avoid O(n^2)
    // False = pure, as nodes that ExpandVisitor inserts preserve pureness
    ତାର୍କିକ m_isImpure = ସତ୍ୟ;  // Currently pure

    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override {
        ତାର୍କିକ selfImpure = !nodep->isPure();
        {
            VL_RESTORER(m_isImpure);
            m_isImpure = ମିଥ୍ୟା;
            iterateChildrenConst(nodep);
            selfImpure |= m_isImpure;
            nodep->user2(selfImpure);
        }
        m_isImpure |= selfImpure;
    }

ସର୍ବସାଧାରଣ:
    // CONSTRUCTORS
    explicit ExpandOkVisitor(AstNetlist* nodep) { iterateConst(nodep); }
    ~ExpandOkVisitor() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
};

//######################################################################
// Expand state, as a visitor of each AstNode

ଶ୍ରେଣୀ ExpandVisitor final : ସର୍ବସାଧାରଣ VNVisitor {
    // NODE STATE
    //  AstNode::user1()        -> bool.  Processed
    //  AstNode::user2()        -> See ExpandOkVisitor
    ଅଚଳ VNUser1InUse m_inuser1;

    // STATE - for current visit position (use VL_RESTORER)
    Ast_ଗଣ୍ଠି* m_stmtp = nullptr;  // Current statement

    // STATE - across all visitors
    AstCFunc* m_funcp = nullptr;  // Current function
    VDouble0 m_statImpure;  // Statistic tracking
    VDouble0 m_statWides;  // Statistic tracking
    VDouble0 m_stat_Wide_ଶବ୍ଦଗୁଡ଼ିକ;  // Statistic tracking
    VDouble0 m_statWideLimited;  // Statistic tracking
    VDouble0 m_statTemps;  // Statistic tracking
    // Number of applications of each expansion pattern for statistics
    std::array<VDouble0, VExpandPattern::_ENUM_END> m_statPatterns;

    // STATE - for current function
    size_t m_nTmps = 0;  // Sequence numbers for temopraries

    // METHODS
    // Note the application of the given expansion pattern to 'nodep'
    ଶୂନ୍ୟ didPattern(VExpandPattern pattern, Ast_ଗଣ୍ଠି* nodep) {
        UINFO(8, "    " << pattern.ascii() << " " << nodep);
        ++m_statPatterns[pattern];
    }

    // Use state that ExpandOkVisitor calculated
    ସ୍ଥିର ତାର୍କିକ isImpure(Ast_ଗଣ୍ଠି* nodep) {
        ଅଚଳ ତାର୍କିକ impure = nodep->user2();
        ଯଦି (impure) UINFO(9, "      impure " << nodep);
        ଫେରନ୍ତୁ impure;
    }

    ତାର୍କିକ doExpandWide(Ast_ଗଣ୍ଠି* nodep) {
        ଯଦି (isImpure(nodep)) {
            ++m_statImpure;
            ଫେରନ୍ତୁ ମିଥ୍ୟା;
        }
        ଯଦି (nodep->width_ଶବ୍ଦଗୁଡ଼ିକ() <= v3Global.opt.expandLimit()) {
            ++m_statWides;
            m_stat_Wide_ଶବ୍ଦଗୁଡ଼ିକ += nodep->width_ଶବ୍ଦଗୁଡ଼ିକ();
            ଫେରନ୍ତୁ ସତ୍ୟ;
        } ଅନ୍ୟଥା {
            ++m_statWideLimited;
            ଫେରନ୍ତୁ ମିଥ୍ୟା;
        }
    }

    ସ୍ଥିର ପୂର୍ଣ୍ଣସଂଖ୍ୟା longOrQuadWidth(Ast_ଗଣ୍ଠି* nodep) {
        ଫେରନ୍ତୁ (nodep->width() + (VL_EDATASIZE - 1)) & ~(VL_EDATASIZE - 1);
    }
    ସ୍ଥିର V3_ସଂଖ୍ୟା notWideMask(Ast_ଗଣ୍ଠି* nodep) {
        ଫେରନ୍ତୁ V3_ସଂଖ୍ୟା{nodep, VL_EDATASIZE, ~VL_MASK_E(nodep->widthMin())};
    }
    ସ୍ଥିର V3_ସଂଖ୍ୟା wordMask(Ast_ଗଣ୍ଠି* nodep) {
        ଯଦି (nodep->isWide()) {
            ଫେରନ୍ତୁ V3_ସଂଖ୍ୟା{nodep, VL_EDATASIZE, VL_MASK_E(nodep->widthMin())};
        } ଅନ୍ୟଥା {
            V3_ସଂଖ୍ୟା mask{nodep, longOrQuadWidth(nodep)};
            mask.setMask(nodep->widthMin());
            ଫେରନ୍ତୁ mask;
        }
    }

    ସ୍ଥିର ଶୂନ୍ୟ ଭର୍ତ୍ତି_Before(Ast_ଗଣ୍ଠି* placep, Ast_ଗଣ୍ଠି* newp) {
        newp->user1(1);  // Already processed, don't need to re-iterate
        placep->addHereThisAsNext(newp);
    }
    ସ୍ଥିର ଶୂନ୍ୟ replaceWithDelete(Ast_ଗଣ୍ଠି* nodep, Ast_ଗଣ୍ଠି* newp) {
        newp->user1(1);  // Already processed, don't need to re-iterate
        ଯଦି (newp->width() != nodep->width()) {
            UASSERT_OBJ(newp->widthMin() == nodep->widthMin(), nodep,
                        "Replacement width mismatch");
            newp->dtypeChgWidth(nodep->width(), nodep->widthMin());
        }
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_ବୃକ୍ଷ(), nodep);
    }
    ସ୍ଥିର Ast_ଗଣ୍ଠି* newWordAssign(Ast_ଗଣ୍ଠି_Assign* placep, ପୂର୍ଣ୍ଣସଂଖ୍ୟା word, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp,
                                  Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp) {
        File_ଧାଡ଼ି* ଅଚଳ fl = placep->fileline();
        ଫେରନ୍ତୁ ନୂତନ AstAssign{fl,
                             ନୂତନ AstWordSel{fl, lhsp->clone_ବୃକ୍ଷ_Pure(ସତ୍ୟ),
                                            ନୂତନ AstConst{fl, static_cast<uint32_t>(word)}},
                             rhsp};
    }
    ସ୍ଥିର ଶୂନ୍ୟ addWordAssign(Ast_ଗଣ୍ଠି_Assign* placep, ପୂର୍ଣ୍ଣସଂଖ୍ୟା word, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp,
                              Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp) {
        ଭର୍ତ୍ତି_Before(placep, newWordAssign(placep, word, lhsp, rhsp));
    }
    ସ୍ଥିର ଶୂନ୍ୟ addWordAssign(Ast_ଗଣ୍ଠି_Assign* placep, ପୂର୍ଣ୍ଣସଂଖ୍ୟା word, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp) {
        addWordAssign(placep, word, placep->lhsp(), rhsp);
    }

    AstVar* addLocalTmp(Ast_ଗଣ୍ଠି* placep, ଅଚଳ ଅକ୍ଷର* namep, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* valuep) {
        ଅଚଳ std::string name = "__V"s + namep + "_"s + std::to_string(m_nTmps);
        File_ଧାଡ଼ି* ଅଚଳ flp = placep->fileline();
        AstVar* ଅଚଳ tmpp = ନୂତନ AstVar{flp, VVarType::STMTTEMP, name, valuep->dtypep()};
        tmpp->funcLocal(ସତ୍ୟ);
        tmpp->isInternal(ସତ୍ୟ);
        tmpp->noReset(ସତ୍ୟ);
        tmpp->substConstOnly(ସତ୍ୟ);
        m_funcp->addVarsp(tmpp);
        ଭର୍ତ୍ତି_Before(placep, ନୂତନ AstAssign{flp, ନୂତନ AstVarRef{flp, tmpp, VAccess::WRITE}, valuep});
        ++m_statTemps;
        ଫେରନ୍ତୁ tmpp;
    }

    ସ୍ଥିର ଶୂନ୍ୟ fixCloneLvalue(Ast_ଗଣ୍ଠି* nodep) {
        // In AstSel transforms, we call clone() on VarRefs that were lvalues,
        // but are now being used on the RHS of the assignment
        ଯଦି (VN_IS(nodep, VarRef)) VN_AS(nodep, VarRef)->access(VAccess::READ);
        // Iterate
        ଯଦି (Ast_ଗଣ୍ଠି* ଅଚଳ refp = nodep->op1p()) fixCloneLvalue(refp);
        ଯଦି (Ast_ଗଣ୍ଠି* ଅଚଳ refp = nodep->op2p()) fixCloneLvalue(refp);
        ଯଦି (Ast_ଗଣ୍ଠି* ଅଚଳ refp = nodep->op3p()) fixCloneLvalue(refp);
        ଯଦି (Ast_ଗଣ୍ଠି* ଅଚଳ refp = nodep->op4p()) fixCloneLvalue(refp);
    }

    ସ୍ଥିର Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* newAstWordSelClone(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* nodep, ପୂର୍ଣ୍ଣସଂଖ୍ୟା word) {
        // Get the specified word number from a wide array
        // Or, if it's a long/quad, do appropriate conversion to wide
        // Concat may pass negative word numbers, that means it wants a zero
        File_ଧାଡ଼ି* ଅଚଳ fl = nodep->fileline();
        ଯଦି (nodep->isWide() && word >= 0 && word < nodep->width_ଶବ୍ଦଗୁଡ଼ିକ()) {
            ଫେରନ୍ତୁ ନୂତନ AstWordSel{fl, nodep->clone_ବୃକ୍ଷ_Pure(ସତ୍ୟ),
                                  ନୂତନ AstConst{fl, static_cast<uint32_t>(word)}};
        } ଅନ୍ୟଥା ଯଦି (nodep->isQuad() && word == 0) {
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ quadfromp = nodep->clone_ବୃକ୍ଷ_Pure(ସତ୍ୟ);
            quadfromp->dtypeSetBitUnsized(VL_QUADSIZE, quadfromp->widthMin(), VSigning::UNSIGNED);
            ଫେରନ୍ତୁ ନୂତନ AstCCast{fl, quadfromp, VL_EDATASIZE};
        } ଅନ୍ୟଥା ଯଦି (nodep->isQuad() && word == 1) {
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ quadfromp = nodep->clone_ବୃକ୍ଷ_Pure(ସତ୍ୟ);
            quadfromp->dtypeSetBitUnsized(VL_QUADSIZE, quadfromp->widthMin(), VSigning::UNSIGNED);
            ଫେରନ୍ତୁ ନୂତନ AstCCast{
                fl, ନୂତନ AstShiftR{fl, quadfromp, ନୂତନ AstConst{fl, VL_EDATASIZE}, VL_EDATASIZE},
                VL_EDATASIZE};
        } ଅନ୍ୟଥା ଯଦି (!nodep->isWide() && !nodep->isQuad() && word == 0) {
            ଫେରନ୍ତୁ nodep->clone_ବୃକ୍ଷ_Pure(ସତ୍ୟ);
        } ଅନ୍ୟଥା {  // Out of bounds
            ଫେରନ୍ତୁ ନୂତନ AstConst{fl, 0};
        }
    }

    ସ୍ଥିର Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* newWordGrabShift(File_ଧାଡ଼ି* fl, ପୂର୍ଣ୍ଣସଂଖ୍ୟା word, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, ପୂର୍ଣ୍ଣସଂଖ୍ୟା shift) {
        // Extract the expression to grab the value for the specified word, if it's the shift
        // of shift bits from lhsp
        // Negative word numbers requested for lhs when it's "before" what we want.
        // We get a 0 then.
        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା othword = word - shift / VL_EDATASIZE;
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ lop = newAstWordSelClone(lhsp, othword);
        ଅଚଳ uint32_t loShift = VL_BITBIT_E(shift);
        ଯଦି (!loShift) ଫେରନ୍ତୁ lop;
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ hip = newAstWordSelClone(lhsp, othword - 1);
        ଅଚଳ uint32_t hiShift = VL_EDATASIZE - loShift;  // Complement offset
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ newp
            = ନୂତନ AstOr{fl,  //
                        ନୂତନ AstShiftR{fl, hip, ନୂତନ AstConst{fl, hiShift}, VL_EDATASIZE},
                        ନୂତନ AstShiftL{fl, lop, ନୂତନ AstConst{fl, loShift}, VL_EDATASIZE}};
        ଫେରନ୍ତୁ V3Const::constifyEditCpp(newp);
    }

    // Return expression indexing the word that contains 'lsbp' + the given word offset
    ସ୍ଥିର Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* newWordIndex(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lsbp, uint32_t wordOffset = 0) {
        // This is indexing a WordSel, so a 32 bit constants are fine
        File_ଧାଡ଼ି* ଅଚଳ flp = lsbp->fileline();
        ଯଦି (ଅଚଳ AstConst* constp = VN_CAST(lsbp, Const)) {
            ଫେରନ୍ତୁ ନୂତନ AstConst{flp, wordOffset + VL_BITWORD_E(constp->toUInt())};
        }

        ଯଦି (lsbp->backp()) lsbp = lsbp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା);
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ wwl2p = ନୂତନ AstConst{flp, VL_EDATASIZE_LOG2};  // Word width log 2
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* indexp = ନୂତନ AstShiftR{flp, lsbp, wwl2p, VL_EDATASIZE};
        ଯଦି (wordOffset) indexp = ନୂତନ AstAdd{flp, ନୂତନ AstConst{flp, wordOffset}, indexp};
        ଫେରନ୍ତୁ indexp;
    }

    // Return word of fromp that contains word indexp + the given word offset.
    ସ୍ଥିର Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* newWordSelWord(File_ଧାଡ଼ି* flp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* fromp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* indexp,
                                       uint32_t wordOffset = 0) {
        UASSERT_OBJ(fromp->isWide(), fromp, "Only need AstWordSel on wide from's");

        ଯଦି (ଅଚଳ AstConst* ଅଚଳ constp = VN_CAST(indexp, Const)) {
            indexp = nullptr;
            wordOffset += constp->toUInt();
        }

        // e.g. "logic [95:0] var[0]; logic [0] sel; out = var[sel];"
        // Squash before C++ to avoid getting a C++ compiler warning
        // (even though code would be unreachable as presumably a
        // AstCond is protecting above this node.
        ଯଦି (wordOffset >= static_cast<uint32_t>(fromp->width_ଶବ୍ଦଗୁଡ଼ିକ())) {
            ଫେରନ୍ତୁ ନୂତନ AstConst{flp, AstConst::SizedEData{}, 0};
        }

        ଯଦି (fromp->backp()) fromp = fromp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା);

        // If indexp was constant, just use it
        ଯଦି (!indexp) ଫେରନ୍ତୁ ନୂତନ AstWordSel{flp, fromp, ନୂତନ AstConst{flp, wordOffset}};

        // Otherwise compute at runtime
        ଯଦି (indexp->backp()) indexp = indexp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା);
        ଯଦି (wordOffset) indexp = ନୂତନ AstAdd{flp, ନୂତନ AstConst{flp, wordOffset}, indexp};
        ଫେରନ୍ତୁ ନୂତନ AstWordSel{flp, fromp, indexp};
    }

    // Return word of fromp that contains bit lsbp + the given word offset.
    ସ୍ଥିର Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* newWordSelBit(File_ଧାଡ଼ି* flp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* fromp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lsbp,
                                      uint32_t wordOffset = 0) {
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ indexp = newWordIndex(lsbp, wordOffset);
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ wordSelp = newWordSelWord(flp, fromp, indexp);
        ଯଦି (!indexp->backp()) VL_DO_DANGLING(indexp->delete_ବୃକ୍ଷ(), indexp);
        ଫେରନ୍ତୁ wordSelp;
    }

    ସ୍ଥିର Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* newSelBitBit(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lsbp) {
        // Return equation to get the VL_BITBIT of a constant or non-constant
        File_ଧାଡ଼ି* ଅଚଳ fl = lsbp->fileline();
        ଯଦି (VN_IS(lsbp, Const)) {
            ଫେରନ୍ତୁ ନୂତନ AstConst{fl, VL_BITBIT_E(VN_AS(lsbp, Const)->toUInt())};
        } ଅନ୍ୟଥା {
            ଫେରନ୍ତୁ ନୂତନ AstAnd{fl, ନୂତନ AstConst{fl, VL_EDATASIZE - 1}, lsbp->clone_ବୃକ୍ଷ_Pure(ସତ୍ୟ)};
        }
    }

    //====================

    ତାର୍କିକ expandWide(Ast_ଗଣ୍ଠି_Assign* nodep, AstConst* rhsp) {
        ଯଦି (!doExpandWide(nodep)) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        didPattern(VExpandPattern::ASSIGN_CONST, nodep);
        // -> {for each_word{ ASSIGN(WORDSEL(wide,#),WORDSEL(CONST,#))}}
        ଯଦି (rhsp->ଅଙ୍କ().isFourState()) {
            rhsp->v3warn(E_UNSUPPORTED,  // LCOV_EXCL_LINE  // impossible?
                         "Unsupported: 4-state numbers in this context");
        }
        File_ଧାଡ଼ି* ଅଚଳ fl = nodep->fileline();
        ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା w = 0; w < nodep->width_ଶବ୍ଦଗୁଡ଼ିକ(); ++w) {
            addWordAssign(nodep, w,
                          ନୂତନ AstConst{fl, AstConst::SizedEData{}, rhsp->ଅଙ୍କ().edataWord(w)});
        }
        ଫେରନ୍ତୁ ସତ୍ୟ;
    }
    //-------- Uniops
    ତାର୍କିକ expandWide(Ast_ଗଣ୍ଠି_Assign* nodep, AstVarRef* rhsp) {
        // Special case: do not expand assignment of constant pool variables.
        // V3Subst undestands these directly.
        ଯଦି (rhsp->varp()->constPoolEntry()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଯଦି (!doExpandWide(nodep)) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        didPattern(VExpandPattern::ASSIGN_VARREF, nodep);
        ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା w = 0; w < nodep->width_ଶବ୍ଦଗୁଡ଼ିକ(); ++w) {
            addWordAssign(nodep, w, newAstWordSelClone(rhsp, w));
        }
        ଫେରନ୍ତୁ ସତ୍ୟ;
    }
    ତାର୍କିକ expandWide(Ast_ଗଣ୍ଠି_Assign* nodep, AstArraySel* rhsp) {
        UASSERT_OBJ(!VN_IS(nodep->dtypep()->skipRefp(), UnpackArrayDType), nodep,
                    "ArraySel with unpacked arrays should have been removed in V3Slice");
        ଯଦି (!doExpandWide(nodep)) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        didPattern(VExpandPattern::ASSIGN_ARRAYSEL, nodep);
        ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା w = 0; w < nodep->width_ଶବ୍ଦଗୁଡ଼ିକ(); ++w) {
            addWordAssign(nodep, w, newAstWordSelClone(rhsp, w));
        }
        ଫେରନ୍ତୁ ସତ୍ୟ;
    }
    ତାର୍କିକ expandWide(Ast_ଗଣ୍ଠି_Assign* nodep, AstNot* rhsp) {
        // -> {for each_word{ ASSIGN(WORDSEL(wide,#),NOT(WORDSEL(lhs,#))) }}
        ଯଦି (!doExpandWide(nodep)) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        didPattern(VExpandPattern::ASSIGN_NOT, nodep);
        File_ଧାଡ଼ି* ଅଚଳ fl = rhsp->fileline();
        ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା w = 0; w < nodep->width_ଶବ୍ଦଗୁଡ଼ିକ(); ++w) {
            addWordAssign(nodep, w, ନୂତନ AstNot{fl, newAstWordSelClone(rhsp->lhsp(), w)});
        }
        ଫେରନ୍ତୁ ସତ୍ୟ;
    }
    //-------- Biops
    ତାର୍କିକ expandWide(Ast_ଗଣ୍ଠି_Assign* nodep, AstAnd* rhsp) {
        ଯଦି (!doExpandWide(nodep)) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        didPattern(VExpandPattern::ASSIGN_AND, nodep);
        File_ଧାଡ଼ି* ଅଚଳ fl = nodep->fileline();
        ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା w = 0; w < nodep->width_ଶବ୍ଦଗୁଡ଼ିକ(); ++w) {
            addWordAssign(nodep, w,
                          ନୂତନ AstAnd{fl, newAstWordSelClone(rhsp->lhsp(), w),
                                     newAstWordSelClone(rhsp->rhsp(), w)});
        }
        ଫେରନ୍ତୁ ସତ୍ୟ;
    }
    ତାର୍କିକ expandWide(Ast_ଗଣ୍ଠି_Assign* nodep, AstOr* rhsp) {
        ଯଦି (!doExpandWide(nodep)) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        didPattern(VExpandPattern::ASSIGN_OR, nodep);
        File_ଧାଡ଼ି* ଅଚଳ fl = nodep->fileline();
        ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା w = 0; w < nodep->width_ଶବ୍ଦଗୁଡ଼ିକ(); ++w) {
            addWordAssign(nodep, w,
                          ନୂତନ AstOr{fl, newAstWordSelClone(rhsp->lhsp(), w),
                                    newAstWordSelClone(rhsp->rhsp(), w)});
        }
        ଫେରନ୍ତୁ ସତ୍ୟ;
    }
    ତାର୍କିକ expandWide(Ast_ଗଣ୍ଠି_Assign* nodep, AstXor* rhsp) {
        ଯଦି (!doExpandWide(nodep)) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        didPattern(VExpandPattern::ASSIGN_XOR, nodep);
        File_ଧାଡ଼ି* ଅଚଳ fl = nodep->fileline();
        ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା w = 0; w < nodep->width_ଶବ୍ଦଗୁଡ଼ିକ(); ++w) {
            addWordAssign(nodep, w,
                          ନୂତନ AstXor{fl, newAstWordSelClone(rhsp->lhsp(), w),
                                     newAstWordSelClone(rhsp->rhsp(), w)});
        }
        ଫେରନ୍ତୁ ସତ୍ୟ;
    }
    ତାର୍କିକ expandWideShift(Ast_ଗଣ୍ଠି_Assign* nodep, Ast_ଗଣ୍ଠି_Biop* rhsp, ତାର୍କିକ is_ବାମ_Shift) {
        ଯଦି (!doExpandWide(nodep)) ଫେରନ୍ତୁ ମିଥ୍ୟା;

        // Simplify the shift amount, in case it becomes a constant
        V3Const::constifyEditCpp(rhsp->rhsp());

        // If it's a constant shift by whole words, expand it so V3Subst can substitute it
        ଯଦି (ଅଚଳ AstConst* ଅଚଳ rhsConstp = VN_CAST(rhsp->rhsp(), Const)) {
            ଅଚଳ uint32_t shiftBits = rhsConstp->toUInt();
            ଯଦି (VL_BITBIT_E(shiftBits) == 0) {
                ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା width_ଶବ୍ଦଗୁଡ଼ିକ = nodep->width_ଶବ୍ଦଗୁଡ଼ିକ();
                ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା shift_ଶବ୍ଦଗୁଡ଼ିକ = std::min<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>(VL_BITWORD_E(shiftBits), width_ଶବ୍ଦଗୁଡ଼ିକ);
                File_ଧାଡ଼ି* ଅଚଳ flp = rhsp->fileline();
                ଯଦି (is_ବାମ_Shift) {
                    didPattern(VExpandPattern::ASSIGN_SHIFTL, nodep);
                    // Low words of the result are zero
                    ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା w = 0; w < shift_ଶବ୍ଦଗୁଡ଼ିକ; ++w) {
                        addWordAssign(nodep, w, ନୂତନ AstConst{flp, AstConst::SizedEData{}, 0});
                    }
                    // High words of the result are copied from higher words of the source
                    ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା w = shift_ଶବ୍ଦଗୁଡ଼ିକ; w < width_ଶବ୍ଦଗୁଡ଼ିକ; ++w) {
                        addWordAssign(nodep, w, newAstWordSelClone(rhsp->lhsp(), w - shift_ଶବ୍ଦଗୁଡ଼ିକ));
                    }
                } ଅନ୍ୟଥା {
                    didPattern(VExpandPattern::ASSIGN_SHIFTR, nodep);
                    // Low words of the result are copied from higher words of the source
                    ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା w = 0; w < width_ଶବ୍ଦଗୁଡ଼ିକ - shift_ଶବ୍ଦଗୁଡ଼ିକ; ++w) {
                        addWordAssign(nodep, w, newAstWordSelClone(rhsp->lhsp(), w + shift_ଶବ୍ଦଗୁଡ଼ିକ));
                    }
                    // High words of the result are zero
                    ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା w = width_ଶବ୍ଦଗୁଡ଼ିକ - shift_ଶବ୍ଦଗୁଡ଼ିକ; w < width_ଶବ୍ଦଗୁଡ଼ିକ; ++w) {
                        addWordAssign(nodep, w, ନୂତନ AstConst{flp, AstConst::SizedEData{}, 0});
                    }
                }
                ଫେରନ୍ତୁ ସତ୍ୟ;
            }
        }

        ଫେରନ୍ତୁ ମିଥ୍ୟା;
    }

    //-------- Triops
    ତାର୍କିକ expandWide(Ast_ଗଣ୍ଠି_Assign* nodep, AstCond* rhsp) {
        ଯଦି (!doExpandWide(nodep)) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        didPattern(VExpandPattern::ASSIGN_COND, nodep);
        File_ଧାଡ଼ି* ଅଚଳ fl = nodep->fileline();
        ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା w = 0; w < nodep->width_ଶବ୍ଦଗୁଡ଼ିକ(); ++w) {
            addWordAssign(nodep, w,
                          ନୂତନ AstCond{fl, rhsp->condp()->clone_ବୃକ୍ଷ_Pure(ସତ୍ୟ),
                                      newAstWordSelClone(rhsp->thenp(), w),
                                      newAstWordSelClone(rhsp->elsep(), w)});
        }
        ଫେରନ୍ତୁ ସତ୍ୟ;
    }

    // VISITORS
    ଶୂନ୍ୟ visit(AstNetlist* nodep) override { iterateAndNextNull(nodep->modulesp()); }

    ଶୂନ୍ୟ visit(AstCFunc* nodep) override {
        VL_RESTORER(m_funcp);
        VL_RESTORER(m_nTmps);
        m_funcp = nodep;
        m_nTmps = 0;
        ଅଚଳ VDouble0 statWidesBefore = m_statWides;
        iterateChildren(nodep);

        // Constant fold here if anything was expanded, as Ast size can likely be reduced
        ଯଦି (v3Global.opt.fConstEager() && m_statWides != statWidesBefore) {
            Ast_ଗଣ୍ଠି* ଅଚଳ editedp = V3Const::constifyEditCpp(nodep);
            UASSERT_OBJ(editedp == nodep, editedp, "Should not have replaced CFunc");
        }
    }

    ଶୂନ୍ୟ visit(AstExtend* nodep) override {
        ଯଦି (nodep->user1SetOnce()) ଫେରନ୍ତୁ;  // Process once
        iterateChildren(nodep);
        ଯଦି (nodep->isWide()) {
            // See under ASSIGN(EXTEND)
        } ଅନ୍ୟଥା {
            ଯଦି (isImpure(nodep)) ଫେରନ୍ତୁ;
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ lhsp = nodep->lhsp()->unlinkFrBack();
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* newp = lhsp;
            ଯଦି (nodep->isQuad()) {
                ଯଦି (lhsp->isQuad()) {
                    lhsp->dtypeFrom(nodep);  // Just mark it, else nop
                } ଅନ୍ୟଥା ଯଦି (lhsp->isWide()) {
                    nodep->v3fatalSrc("extending larger thing into smaller?");
                } ଅନ୍ୟଥା {
                    newp = ନୂତନ AstCCast{nodep->fileline(), lhsp, nodep};
                }
            } ଅନ୍ୟଥା {  // Long
                UASSERT_OBJ(!(lhsp->isQuad() || lhsp->isWide()), nodep,
                            "extending larger thing into smaller?");
                lhsp->dtypeFrom(nodep);  // Just mark it, else nop
            }
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        }
    }
    ତାର୍କିକ expandWide(Ast_ଗଣ୍ଠି_Assign* nodep, AstExtend* rhsp) {
        ଯଦି (!doExpandWide(nodep)) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        didPattern(VExpandPattern::ASSIGN_EXTEND, nodep);
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ rlhsp = rhsp->lhsp();
        ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା w = 0; w < rlhsp->width_ଶବ୍ଦଗୁଡ଼ିକ(); ++w) {
            addWordAssign(nodep, w, newAstWordSelClone(rlhsp, w));
        }
        ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା w = rlhsp->width_ଶବ୍ଦଗୁଡ଼ିକ(); w < nodep->width_ଶବ୍ଦଗୁଡ଼ିକ(); ++w) {
            addWordAssign(nodep, w, ନୂତନ AstConst{nodep->fileline(), AstConst::SizedEData{}, 0});
        }
        ଫେରନ୍ତୁ ସତ୍ୟ;
    }

    ଶୂନ୍ୟ visit(AstSel* nodep) override {
        ଯଦି (nodep->user1SetOnce()) ଫେରନ୍ତୁ;  // Process once
        iterateChildren(nodep);

        ଅଚଳ uint32_t width = static_cast<uint32_t>(nodep->widthConst());
        UASSERT_OBJ(nodep->widthMin() == static_cast<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>(width), nodep, "Width mismatch");

        // Skip if Sel is an LHS assignment select
        ଯଦି (Ast_ଗଣ୍ଠି_Assign* ଅଚଳ assignp = VN_CAST(nodep->backp(), ଗଣ୍ଠି_Assign)) {
            ଯଦି (nodep == assignp->lhsp()) ଫେରନ୍ତୁ;
        }

        // Skip if wide: See under ASSIGN(WIDE)
        ଯଦି (nodep->isWide()) ଫେରନ୍ତୁ;

        // Skip Sel from stream or queue
        ଯଦି (VN_IS(nodep->fromp()->dtypep(), StreamDType)) ଫେରନ୍ତୁ;
        ଯଦି (VN_IS(nodep->fromp()->dtypep(), QueueDType)) ଫେରନ୍ତୁ;

        // Result must be non-wide after skipping all the above
        UASSERT_OBJ(width <= 64, nodep, "Inconsistent result width");

        ଯଦି (nodep->fromp()->isWide()) {  // Long/Quad from Wide
            ଯଦି (isImpure(nodep)) ଫେରନ୍ତୁ;
            didPattern(VExpandPattern::SEL_FROM_WIDE, nodep);
            File_ଧାଡ଼ି* ଅଚଳ nfl = nodep->fileline();
            File_ଧାଡ଼ି* ଅଚଳ lfl = nodep->lsbp()->fileline();
            File_ଧାଡ଼ି* ଅଚଳ ffl = nodep->fromp()->fileline();
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ fromp = nodep->fromp();

            // To extract an up-to-64-bit value from a wide source, up to 3 words might be needed
            // from the source operand. Each word might need to be shifted by a different amount.
            // All word indices and shift amounts are derived from the LSB expression, so it might
            // be duplicated several times. The problem arises when the LSB expression itself is
            // a similar select, which results in an exponential size expansion.
            //
            // To avoid this, if the required expressions derived from the LSB expression are
            // large, and the LSB expression is required multiple times for the expansion, then
            // the LSB expression is evaluated into a temporary variable. This bounds the size
            // of the expansion on each Sel instance, hence the total expansion of nested Sels
            // is bounded linearly with the number of nested Sels (instead of exponentially).

            // Return simplified 'exprp' if it simplifies to a small tree, otherwise nullptr
            ଅଚଳ auto tryFold = [&](Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp) -> Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* {
                // Size limit for inlining expressions (any arbitrary value results in
                // a bounded expansion. Picked a value to allow common simple forms.)
                ସ୍ଥିର constexpr ପୂର୍ଣ୍ଣସଂଖ୍ୟା EXPAND_SEL_LSB_LIMIT = 8;
                // Simplify
                exprp = V3Const::constifyEditCpp(exprp);
                // Accept if not larger than the limit
                ଯଦି (!exprp->isLargerThan(EXPAND_SEL_LSB_LIMIT)) ଫେରନ୍ତୁ exprp;
                // Delete if rejected
                VL_DO_DANGLING(exprp->delete_ବୃକ୍ଷ(), exprp);
                ଫେରନ୍ତୁ nullptr;
            };

            // Return the index of the word holding the bit 'offset' bits above the LSB
            ଅଚଳ auto wordIdx = [&](uint32_t offset) -> Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* {
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ msbp = ନୂତନ AstAdd{lfl, ନୂତନ AstConst{lfl, offset},
                                                     nodep->lsbp()->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା)};
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ idxp = newWordIndex(msbp);
                ଯଦି (!msbp->backp()) VL_DO_DANGLING(msbp->delete_ବୃକ୍ଷ(), msbp);
                ଫେରନ୍ତୁ idxp;
            };

            // Bit index of the selected LSB within its word
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lBitp = tryFold(newSelBitBit(nodep->lsbp()));
            ଅଚଳ ତାର୍କିକ aligned = lBitp && lBitp->isZero();

            // Which of the other 2 words the expansion might need
            ଅଚଳ ତାର୍କିକ mNeeded = width > 1 && !aligned;
            ଅଚଳ ତାର୍କିକ hNeeded = width > VL_EDATASIZE;
            UASSERT_OBJ(!hNeeded || nodep->isQuad(), nodep, "Width mismatch");

            // Word indices
            ଅଚଳ uint32_t mMsbOffset = std::min<uint32_t>(width, VL_EDATASIZE) - 1;
            ଅଚଳ uint32_t hMsbOffset = width - 1;
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lIdxp = tryFold(newWordIndex(nodep->lsbp()));
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* mIdxp = mNeeded ? tryFold(wordIdx(mMsbOffset)) : nullptr;
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* hIdxp = hNeeded ? tryFold(wordIdx(hMsbOffset)) : nullptr;

            // If the LSB expression is needed multiple times, evaluate it into a temporary
            ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା nCopies = !lBitp + !lIdxp + (mNeeded && !mIdxp) + (hNeeded && !hIdxp);
            ଯଦି (nCopies > 1 && m_funcp && m_stmtp && !isImpure(m_stmtp)) {
                ++m_nTmps;  // Use fresh set of temporaries
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ lsbp = nodep->lsbp()->unlinkFrBack();
                AstVar* ଅଚଳ tmpp = addLocalTmp(m_stmtp, "ExpandSel_Lsb", lsbp);
                nodep->lsbp(ନୂତନ AstVarRef{lfl, tmpp, VAccess::READ});
            }

            // Rebuild remaining expressions derived from LSB if needed, now using the temporary
            ଯଦି (!lBitp) lBitp = newSelBitBit(nodep->lsbp());
            ଯଦି (!lIdxp) lIdxp = newWordIndex(nodep->lsbp());
            ଯଦି (!mIdxp) mIdxp = mNeeded ? wordIdx(mMsbOffset) : nullptr;
            ଯଦି (!hIdxp) hIdxp = hNeeded ? wordIdx(hMsbOffset) : nullptr;

            // Return word 'idxp' of 'fromp', without consuming 'idxp'
            ଅଚଳ auto wordSel = [&](Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* idxp) -> Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* {
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ clonep = idxp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା);
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ wordp = newWordSelWord(ffl, fromp, clonep);
                ଯଦି (!clonep->backp()) VL_DO_DANGLING(clonep->delete_ବୃକ୍ଷ(), clonep);
                ଫେରନ୍ତୁ wordp;
            };

            // Construct term containing the bits of the low word - always needed
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ lTermp = [&]() -> Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* {
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lWordp = wordSel(lIdxp);
                ଯଦି (nodep->isQuad()) lWordp = ନୂତନ AstCCast{nfl, lWordp, nodep};
                ଫେରନ୍ତୁ ନୂତନ AstShiftR{nfl, lWordp, lBitp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା), nodep->width()};
            }();

            // Construct term containing the bits of the middle word - if needed
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ mTermp = [&]() -> Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* {
                ଯଦି (!mNeeded) ଫେରନ୍ତୁ nullptr;

                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* mWordp = wordSel(mIdxp);
                ଯଦି (nodep->isQuad()) mWordp = ନୂତନ AstCCast{nfl, mWordp, nodep};
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ mShiftp = ନୂତନ AstSub{lfl, ନୂତନ AstConst{lfl, VL_EDATASIZE},
                                                        lBitp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା)};
                // If we're selecting bit zero, then all 32 bits in the mid word
                // get shifted << by 32 bits, so ignore them.
                ଅଚଳ V3_ସଂଖ୍ୟା zero{nodep, longOrQuadWidth(nodep)};
                ଫେରନ୍ତୁ ନୂତନ AstCond{
                    nfl,
                    // lsb % VL_EDATASIZE == 0 ?
                    ନୂତନ AstEq{nfl, ନୂତନ AstConst{nfl, 0}, lBitp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା)},
                    // 0 :
                    ନୂତନ AstConst{nfl, zero},
                    //  midword >> (VL_EDATASIZE - (lbs % VL_EDATASIZE))
                    ନୂତନ AstShiftL{nfl, mWordp, mShiftp, nodep->width()}};
            }();

            // Construct term containing the bits of the high word - if needed
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ hTermp = [&]() -> Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* {
                ଯଦି (!hNeeded) ଫେରନ୍ତୁ nullptr;

                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* hWordp = wordSel(hIdxp);
                hWordp = ନୂତନ AstCCast{nfl, hWordp, nodep};
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ hShiftp = ନୂତନ AstCond{
                    nfl,
                    // lsb % VL_EDATASIZE == 0 ?
                    ନୂତନ AstEq{nfl, ନୂତନ AstConst{nfl, 0}, lBitp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା)},
                    // VL_EDATASIZE :
                    ନୂତନ AstConst{lfl, VL_EDATASIZE},
                    // 64 - (lbs % VL_EDATASIZE)
                    ନୂତନ AstSub{lfl, ନୂତନ AstConst{lfl, 64}, lBitp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା)}};
                ଫେରନ୍ତୁ ନୂତନ AstShiftL{nfl, hWordp, hShiftp, nodep->width()};
            }();

            // Delete parts not captured during construction
            VL_DO_DANGLING(lBitp->delete_ବୃକ୍ଷ(), lBitp);
            VL_DO_DANGLING(lIdxp->delete_ବୃକ୍ଷ(), lIdxp);
            ଯଦି (mIdxp) VL_DO_DANGLING(mIdxp->delete_ବୃକ୍ଷ(), mIdxp);
            ଯଦି (hIdxp) VL_DO_DANGLING(hIdxp->delete_ବୃକ୍ଷ(), hIdxp);

            // Or reduce the terms
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* newp = lTermp;
            ଯଦି (mTermp) newp = ନୂତନ AstOr{nfl, mTermp, newp};
            ଯଦି (hTermp) newp = ନୂତନ AstOr{nfl, hTermp, newp};
            newp->dtypeFrom(nodep);
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        } ଅନ୍ୟଥା {  // Long/Quad from Long/Quad
            // No isImpure() check - can handle side effects in below
            didPattern(VExpandPattern::SEL_FROM_NARROW, nodep);
            File_ଧାଡ଼ି* ଅଚଳ fl = nodep->fileline();
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* fromp = nodep->fromp()->unlinkFrBack();
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ lsbp = nodep->lsbp()->unlinkFrBack();
            ଯଦି (nodep->isQuad() && !fromp->isQuad()) fromp = ନୂତନ AstCCast{fl, fromp, nodep};
            // {large}>>32 requires 64-bit shift operation; then cast
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* newp = ନୂତନ AstShiftR{fl, fromp, lsbp, fromp->width()};
            newp->dtypeFrom(fromp);
            ଯଦି (!nodep->isQuad() && fromp->isQuad()) newp = ନୂତନ AstCCast{fl, newp, nodep};
            newp->dtypeFrom(nodep);
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        }
    }

    ତାର୍କିକ expandWide(Ast_ଗଣ୍ଠି_Assign* nodep, AstSel* rhsp) {
        UASSERT_OBJ(nodep->widthMin() == rhsp->widthConst(), nodep, "Width mismatch");
        ଯଦି (!doExpandWide(nodep)) ଫେରନ୍ତୁ ମିଥ୍ୟା;

        // Simplify the index, in case it becomes a constant
        V3Const::constifyEditCpp(rhsp->lsbp());

        // If it's a constant select and aligned, we can just copy the words
        ଯଦି (ଅଚଳ AstConst* ଅଚଳ lsbConstp = VN_CAST(rhsp->lsbp(), Const)) {
            ଅଚଳ uint32_t lsb = lsbConstp->toUInt();
            ଯଦି (VL_BITBIT_E(lsb) == 0) {
                didPattern(VExpandPattern::ASSIGN_SEL_ALIGNED, nodep);
                ଅଚଳ uint32_t word = VL_BITWORD_E(lsb);
                ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା w = 0; w < nodep->width_ଶବ୍ଦଗୁଡ଼ିକ(); ++w) {
                    addWordAssign(nodep, w, newAstWordSelClone(rhsp->fromp(), w + word));
                }
                ଫେରନ୍ତୁ ସତ୍ୟ;
            }
        }

        didPattern(VExpandPattern::ASSIGN_SEL_UNALIGNED, nodep);
        File_ଧାଡ଼ି* ଅଚଳ flp = rhsp->fileline();

        // Use fresh set of temporaries
        ++m_nTmps;

        // Compute word index of LSB, store to temporary if not constant
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ wordLsbp = rhsp->lsbp()->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା);
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* wordIdxp = newWordIndex(wordLsbp);
        ଯଦି (!wordLsbp->backp()) VL_DO_DANGLING(wordLsbp->delete_ବୃକ୍ଷ(), wordLsbp);
        wordIdxp = V3Const::constifyEditCpp(wordIdxp);
        ଯଦି (!VN_IS(wordIdxp, Const)) {
            AstVar* ଅଚଳ tmpp = addLocalTmp(nodep, "ExpandSel_WordIdx", wordIdxp);
            wordIdxp = ନୂତନ AstVarRef{flp, tmpp, VAccess::READ};
        }

        // Compute shift amounts and mask, store to temporaries if not constants
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* loShftp = nullptr;
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* hiShftp = nullptr;
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* hiMaskp = nullptr;
        ଯଦି (ଅଚଳ AstConst* ଅଚଳ lsbConstp = VN_CAST(rhsp->lsbp(), Const)) {
            ଅଚଳ uint32_t bitOffset = VL_BITBIT_E(lsbConstp->toUInt());
            // Must be unaligned, otherwise we would have handled it above
            UASSERT_OBJ(bitOffset, nodep, "Missed aligned wide select");
            loShftp = ନୂତନ AstConst{flp, bitOffset};
            hiShftp = ନୂତନ AstConst{flp, VL_EDATASIZE - bitOffset};
            hiMaskp = nullptr;
        } ଅନ୍ୟଥା {
            // Compute Low word shift amount: bottom bits of lsb index
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ lsbp = rhsp->lsbp()->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା);
            loShftp = ନୂତନ AstAnd{flp, ନୂତନ AstConst{flp, VL_SIZEBITS_E}, lsbp};
            AstVar* ଅଚଳ loTmpp = addLocalTmp(nodep, "ExpandSel_LoShift", loShftp);
            loShftp = ନୂତନ AstVarRef{flp, loTmpp, VAccess::READ};

            // Compute if aligned: Low word shift amount is 0
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ zerop = ନୂତନ AstConst{flp, 0};
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* alignedp = ନୂତନ AstEq{flp, zerop, loShftp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା)};
            AstVar* ଅଚଳ alignedTmpp = addLocalTmp(nodep, "ExpandSel_Aligned", alignedp);
            alignedp = ନୂତନ AstVarRef{flp, alignedTmpp, VAccess::READ};

            // Computed High word shift amount: 0 if aligned else VL_EDATASIZE - Low word shift
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ edsp = ନୂତନ AstConst{flp, VL_EDATASIZE};
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ subp = ନୂତନ AstSub{flp, edsp, loShftp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା)};
            hiShftp = ନୂତନ AstCond{flp, alignedp, ନୂତନ AstConst{flp, 0}, subp};
            AstVar* ଅଚଳ hiTmpp = addLocalTmp(nodep, "ExpandSel_HiShift", hiShftp);
            hiShftp = ନୂତନ AstVarRef{flp, hiTmpp, VAccess::READ};

            // Compute the High word mask: 0 if aligned, ones otherwise
            hiMaskp = ନୂତନ AstCond{flp, alignedp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା),
                                  ନୂତନ AstConst{flp, AstConst::SizedEData{}, 0},
                                  ନୂତନ AstConst{flp, AstConst::SizedEData{}, -1ULL}};
            AstVar* ଅଚଳ maskTmpp = addLocalTmp(nodep, "ExpandSel_HiMask", hiMaskp);
            hiMaskp = ନୂତନ AstVarRef{flp, maskTmpp, VAccess::READ};
        }

        // Create each word of the selected result
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ fromp = rhsp->fromp();
        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା sel_ଶବ୍ଦଗୁଡ଼ିକ = nodep->width_ଶବ୍ଦଗୁଡ଼ିକ();
        ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା w = 0; w < sel_ଶବ୍ଦଗୁଡ଼ିକ; ++w) {
            // Grab bits from word 'VL_BITWORD_E(lsb) + w'
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ loWordp = newWordSelWord(fromp->fileline(), fromp, wordIdxp, w);
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ loShftClonep = loShftp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା);
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ lop = ନୂତନ AstShiftR{flp, loWordp, loShftClonep, VL_EDATASIZE};
            // Grab bits from word 'VL_BITWORD_E(lsb) + w + 1'
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* hiWordp = newWordSelWord(fromp->fileline(), fromp, wordIdxp, w + 1);
            // For the last word of the result, avoid an OOB access when the Sel is not OOB
            ଯଦି (w == sel_ଶବ୍ଦଗୁଡ଼ିକ - 1) {
                ଅଚଳ uint32_t max = fromp->width_ଶବ୍ଦଗୁଡ଼ିକ() - w - 1;
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ maxp = ନୂତନ AstConst{flp, max};
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ condp = ନୂତନ AstGte{flp, wordIdxp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା), maxp};
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ zerop = ନୂତନ AstConst{flp, AstConst::SizedEData{}, 0};
                hiWordp = ନୂତନ AstCond{flp, condp, zerop, hiWordp};
            }
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ hiShftClonep = hiShftp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା);
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ hiPartp = ନୂତନ AstShiftL{flp, hiWordp, hiShftClonep, VL_EDATASIZE};
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ hip
                = hiMaskp ? ନୂତନ AstAnd{flp, hiPartp, hiMaskp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା)} : hiPartp;
            // Combine them
            addWordAssign(nodep, w, ନୂତନ AstOr{nodep->fileline(), hip, lop});
        }

        // Delete parts not captured during construction
        ଯଦି (!wordIdxp->backp()) VL_DO_DANGLING(wordIdxp->delete_ବୃକ୍ଷ(), wordIdxp);
        ଯଦି (!loShftp->backp()) VL_DO_DANGLING(loShftp->delete_ବୃକ୍ଷ(), loShftp);
        ଯଦି (!hiShftp->backp()) VL_DO_DANGLING(hiShftp->delete_ବୃକ୍ଷ(), hiShftp);
        ଯଦି (hiMaskp && !hiMaskp->backp()) VL_DO_DANGLING(hiMaskp->delete_ବୃକ୍ଷ(), hiMaskp);

        ଫେରନ୍ତୁ ସତ୍ୟ;
    }

    ତାର୍କିକ expandLhs(Ast_ଗଣ୍ଠି_Assign* nodep, AstSel* lhsp) {
        // Possibilities
        //      destp: wide or narrow
        //      rhsp:  wide (destp must be wide), narrow, or 1 bit wide
        //      rhsp:  may be allones and can remove AND NOT gate
        //      lsbp:  constant or variable
        // Yuk.
        ଯଦି (isImpure(nodep)) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        File_ଧାଡ଼ି* ଅଚଳ nfl = nodep->fileline();
        File_ଧାଡ଼ି* ଅଚଳ lfl = lhsp->fileline();
        ଅଚଳ ତାର୍କିକ destwide = lhsp->fromp()->isWide();
        ଅଚଳ ତାର୍କିକ ones = nodep->rhsp()->isAllOnesV();
        ଯଦି (VN_IS(lhsp->lsbp(), Const)) {
            // The code should work without this constant test, but it won't
            // constify as nicely as we'd like.
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp = nodep->rhsp()->unlinkFrBack();
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ destp = lhsp->fromp()->unlinkFrBack();
            ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା lsb = lhsp->lsbConst();
            ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା msb = lhsp->msbConst();
            V3_ସଂଖ୍ୟା maskset{nodep, destp->widthMin()};
            maskset.setMask(msb + 1 - lsb, lsb);
            V3_ସଂଖ୍ୟା maskold{nodep, destp->widthMin()};
            maskold.opNot(maskset);
            ଯଦି (destwide) {
                didPattern(VExpandPattern::ASSIGN_TO_SEL_CONST_WIDE, nodep);
                ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା w = 0; w < destp->width_ଶବ୍ଦଗୁଡ଼ିକ(); ++w) {
                    ଯଦି (w >= VL_BITWORD_E(lsb) && w <= VL_BITWORD_E(msb)) {
                        // else we would just be setting it to the same exact value
                        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* oldvalp = newAstWordSelClone(destp, w);
                        fixCloneLvalue(oldvalp);
                        ଯଦି (!ones) {
                            oldvalp = ନୂତନ AstAnd{
                                lfl,
                                ନୂତନ AstConst{lfl, AstConst::SizedEData{}, maskold.edataWord(w)},
                                oldvalp};
                        }

                        // Appropriate word of new value to insert:
                        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* newp = newWordGrabShift(lfl, w, rhsp, lsb);

                        // Apply cleaning at the top word of the destination
                        // (no cleaning to do if dst's width is a whole number
                        // of words).
                        ଯଦି (w == destp->width_ଶବ୍ଦଗୁଡ଼ିକ() - 1 && VL_BITBIT_E(destp->widthMin()) != 0) {
                            V3_ସଂଖ୍ୟା cleanmask{nodep, VL_EDATASIZE};
                            cleanmask.setMask(VL_BITBIT_E(destp->widthMin()));
                            newp = ନୂତନ AstAnd{lfl, newp, ନୂତନ AstConst{lfl, cleanmask}};
                        }
                        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ orp
                            = V3Const::constifyEditCpp(ନୂତନ AstOr{lfl, oldvalp, newp});
                        addWordAssign(nodep, w, destp, orp);
                    }
                }
                VL_DO_DANGLING(rhsp->delete_ବୃକ୍ଷ(), rhsp);
                VL_DO_DANGLING(destp->delete_ବୃକ୍ଷ(), destp);
            } ଅନ୍ୟଥା {
                didPattern(VExpandPattern::ASSIGN_TO_SEL_CONST_NARROW, nodep);
                ଯଦି (destp->isQuad() && !rhsp->isQuad()) rhsp = ନୂତନ AstCCast{nfl, rhsp, nodep};
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* oldvalp = destp->clone_ବୃକ୍ଷ_Pure(ସତ୍ୟ);
                fixCloneLvalue(oldvalp);
                ଯଦି (!ones) oldvalp = ନୂତନ AstAnd{lfl, ନୂତନ AstConst{lfl, maskold}, oldvalp};

                // The bit-select can refer to bits outside the width of nodep
                // which we aren't allowed to assign to.  This is a mask of the
                // valid range of nodep which we apply to the new shifted RHS.
                V3_ସଂଖ୍ୟା cleanmask{nodep, destp->widthMin()};
                cleanmask.setMask(destp->widthMin());
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ shifted = ନୂତନ AstShiftL{
                    lfl, rhsp, ନୂତନ AstConst{lfl, static_cast<uint32_t>(lsb)}, destp->width()};
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ cleaned
                    = ନୂତନ AstAnd{lfl, shifted, ନୂତନ AstConst{lfl, cleanmask}};
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ orp
                    = V3Const::constifyEditCpp(ନୂତନ AstOr{lfl, oldvalp, cleaned});
                ଭର୍ତ୍ତି_Before(nodep, ନୂତନ AstAssign{nfl, destp, orp});
            }
            ଫେରନ୍ତୁ ସତ୍ୟ;
        } ଅନ୍ୟଥା {  // non-const select offset
            ଯଦି (destwide && lhsp->widthConst() == 1) {
                didPattern(VExpandPattern::ASSIGN_TO_SEL_VAR_WIDE_ONE_BIT, nodep);
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ rhsp = nodep->rhsp()->unlinkFrBack();
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ destp = lhsp->fromp()->unlinkFrBack();
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* oldvalp
                    = newWordSelBit(lfl, destp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା), lhsp->lsbp());
                fixCloneLvalue(oldvalp);
                ଯଦି (!ones) {
                    oldvalp = ନୂତନ AstAnd{
                        lfl,
                        ନୂତନ AstNot{
                            lfl, ନୂତନ AstShiftL{lfl, ନୂତନ AstConst{nfl, 1},
                                               // newSelBitBit may exceed the MSB of this variable.
                                               // That's ok as we'd just AND with a larger value,
                                               // but oldval would clip the upper bits to sanity
                                               newSelBitBit(lhsp->lsbp()), VL_EDATASIZE}},
                        oldvalp};
                }
                // Restrict the shift amount to 0-31, see bug804.
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ shiftp = ନୂତନ AstAnd{nfl, lhsp->lsbp()->clone_ବୃକ୍ଷ_Pure(ସତ୍ୟ),
                                                       ନୂତନ AstConst{nfl, VL_EDATASIZE - 1}};
                Ast_ଗଣ୍ଠି* ଅଚଳ newp = ନୂତନ AstAssign{
                    nfl, newWordSelBit(nfl, destp, lhsp->lsbp()),
                    ନୂତନ AstOr{lfl, oldvalp, ନୂତନ AstShiftL{lfl, rhsp, shiftp, VL_EDATASIZE}}};
                ଭର୍ତ୍ତି_Before(nodep, newp);
                ଫେରନ୍ତୁ ସତ୍ୟ;
            } ଅନ୍ୟଥା ଯଦି (destwide) {
                UINFO(8, "    ASSIGNSEL(varlsb,wide) -- NoOp -- " << nodep);
                //   For wide destp, we can either form a equation for every destination word,
                // with the appropriate long equation of if it's being written or not.
                //   Or, we can use a LHS variable arraysel with
                //   non-constant index to set the vector.
                // Doing the variable arraysel is better for globals and large arrays,
                // doing every word is better for temporaries and if we're setting most words
                // since it may result in better substitution optimizations later.
                //   This results in so much code, we're better off leaving a function call.
                // Reconsider if we get subexpression elimination.
                ଫେରନ୍ତୁ ମିଥ୍ୟା;
            } ଅନ୍ୟଥା {
                didPattern(VExpandPattern::ASSIGN_TO_SEL_VAR_NARROW, nodep);
                // nodep->dumpTree("-  old: ");
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp = nodep->rhsp()->unlinkFrBack();
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ destp = lhsp->fromp()->unlinkFrBack();
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* oldvalp = destp->clone_ବୃକ୍ଷ_Pure(ସତ୍ୟ);
                fixCloneLvalue(oldvalp);

                V3_ସଂଖ୍ୟା maskwidth{nodep, destp->widthMin()};
                maskwidth.setMask(lhsp->widthConst());

                ଯଦି (destp->isQuad() && !rhsp->isQuad()) rhsp = ନୂତନ AstCCast{nfl, rhsp, nodep};
                ଯଦି (!ones) {
                    oldvalp = ନୂତନ AstAnd{
                        lfl,
                        ନୂତନ AstNot{lfl, ନୂତନ AstShiftL{lfl, ନୂତନ AstConst{nfl, maskwidth},
                                                      lhsp->lsbp()->clone_ବୃକ୍ଷ_Pure(ସତ୍ୟ),
                                                      destp->width()}},
                        oldvalp};
                }
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* newp
                    = ନୂତନ AstShiftL{lfl, rhsp, lhsp->lsbp()->clone_ବୃକ୍ଷ_Pure(ସତ୍ୟ), destp->width()};
                // Apply cleaning to the new value being inserted.  Mask is
                // slightly wider than necessary to avoid an AND with all ones
                // being optimized out.  No need to clean if destp is
                // quad-sized as there are no extra bits to contaminate
                ଯଦି (destp->widthMin() != 64) {
                    V3_ସଂଖ୍ୟା cleanmask{nodep, destp->widthMin() + 1};
                    cleanmask.setMask(destp->widthMin());
                    newp = ନୂତନ AstAnd{lfl, newp, ନୂତନ AstConst{lfl, cleanmask}};
                }

                ଭର୍ତ୍ତି_Before(nodep, ନୂତନ AstAssign{nfl, destp, ନୂତନ AstOr{lfl, oldvalp, newp}});
                ଫେରନ୍ତୁ ସତ୍ୟ;
            }
        }
    }

    ଶୂନ୍ୟ visit(AstConcat* nodep) override {
        ଯଦି (nodep->user1SetOnce()) ଫେରନ୍ତୁ;  // Process once
        iterateChildren(nodep);
        ଯଦି (nodep->isWide()) {
            // See under ASSIGN(WIDE)
        } ଅନ୍ୟଥା {
            // No isImpure() check - can handle side effects in below
            didPattern(VExpandPattern::CONCAT, nodep);
            File_ଧାଡ଼ି* ଅଚଳ fl = nodep->fileline();
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp = nodep->lhsp()->unlinkFrBack();
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp = nodep->rhsp()->unlinkFrBack();
            ଅଚଳ uint32_t rhsshift = rhsp->widthMin();
            ଯଦି (nodep->isQuad() && !lhsp->isQuad()) lhsp = ନୂତନ AstCCast{fl, lhsp, nodep};
            ଯଦି (nodep->isQuad() && !rhsp->isQuad()) rhsp = ନୂତନ AstCCast{fl, rhsp, nodep};
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ newp = ନୂତନ AstOr{
                fl, ନୂତନ AstShiftL{fl, lhsp, ନୂତନ AstConst{fl, rhsshift}, nodep->width()}, rhsp};
            newp->dtypeFrom(nodep);  // Unsigned
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        }
    }
    ତାର୍କିକ expandWide(Ast_ଗଣ୍ଠି_Assign* nodep, AstConcat* rhsp) {
        ଯଦି (!doExpandWide(rhsp)) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        didPattern(VExpandPattern::ASSIGN_CONCAT, nodep);
        File_ଧାଡ଼ି* ଅଚଳ fl = rhsp->fileline();
        // Lhs or Rhs may be word, long, or quad.
        // newAstWordSelClone nicely abstracts the difference.
        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା rhsshift = rhsp->rhsp()->widthMin();
        // Sometimes doing the words backwards is preferable.
        // When we have x={x,foo} backwards is better, when x={foo,x} forward is better
        // However V3Subst tends to rip this up, so not worth optimizing now.
        ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା w = 0; w < rhsp->width_ଶବ୍ଦଗୁଡ଼ିକ(); ++w) {
            addWordAssign(nodep, w,
                          ନୂତନ AstOr{fl, newWordGrabShift(fl, w, rhsp->lhsp(), rhsshift),
                                    newAstWordSelClone(rhsp->rhsp(), w)});
        }
        ଫେରନ୍ତୁ ସତ୍ୟ;
    }

    ଶୂନ୍ୟ visit(AstReplicate* nodep) override {
        ଯଦି (nodep->user1SetOnce()) ଫେରନ୍ତୁ;  // Process once
        iterateChildren(nodep);
        ଯଦି (nodep->isWide()) {
            // See under ASSIGN(WIDE)
        } ଅନ୍ୟଥା {
            ଯଦି (isImpure(nodep)) ଫେରନ୍ତୁ;
            File_ଧାଡ଼ି* ଅଚଳ fl = nodep->fileline();
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp = nodep->srcp()->unlinkFrBack();
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* newp;
            ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା lhswidth = lhsp->widthMin();
            ଯଦି (lhswidth == 1) {
                didPattern(VExpandPattern::REPLICATE_ONE_BIT, nodep);
                newp = ନୂତନ AstNegate{fl, lhsp};
            } ଅନ୍ୟଥା {
                didPattern(VExpandPattern::REPLICATE, nodep);
                ଅଚଳ AstConst* ଅଚଳ constp = VN_AS(nodep->countp(), Const);
                UASSERT_OBJ(constp, nodep,
                            "Replication value isn't a constant.  Checked earlier!");
                ଅଚଳ uint32_t times = constp->toUInt();
                ଯଦି (nodep->isQuad() && !lhsp->isQuad()) lhsp = ନୂତନ AstCCast{fl, lhsp, nodep};
                newp = lhsp->clone_ବୃକ୍ଷ_Pure(ସତ୍ୟ);
                ପାଇଁ (unsigned repnum = 1; repnum < times; repnum++) {
                    ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା rhsshift = repnum * lhswidth;
                    newp = ନୂତନ AstOr{
                        fl,
                        ନୂତନ AstShiftL{fl, lhsp->clone_ବୃକ୍ଷ_Pure(ସତ୍ୟ),
                                      ନୂତନ AstConst{fl, static_cast<uint32_t>(rhsshift)},
                                      nodep->width()},
                        newp};
                    newp->dtypeFrom(nodep);  // Unsigned
                }
                VL_DO_DANGLING(lhsp->delete_ବୃକ୍ଷ(), lhsp);  // Never used
            }
            newp->dtypeFrom(nodep);  // Unsigned
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        }
    }
    ତାର୍କିକ expandWide(Ast_ଗଣ୍ଠି_Assign* nodep, AstReplicate* rhsp) {
        ଯଦି (!doExpandWide(rhsp)) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        didPattern(VExpandPattern::ASSIGN_REPLICATE, nodep);
        File_ଧାଡ଼ି* ଅଚଳ fl = nodep->fileline();
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ lhsp = rhsp->srcp();
        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା lhswidth = lhsp->widthMin();
        ଅଚଳ AstConst* ଅଚଳ constp = VN_AS(rhsp->countp(), Const);
        UASSERT_OBJ(constp, rhsp, "Replication value isn't a constant.  Checked earlier!");
        ଅଚଳ uint32_t times = constp->toUInt();
        ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା w = 0; w < rhsp->width_ଶବ୍ଦଗୁଡ଼ିକ(); ++w) {
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* newp;
            ଯଦି (lhswidth == 1) {
                newp = ନୂତନ AstNegate{fl, lhsp->clone_ବୃକ୍ଷ_Pure(ସତ୍ୟ)};
                // Replicate always unsigned
                newp->dtypeChgWidthSigned(VL_EDATASIZE, VL_EDATASIZE, VSigning::UNSIGNED);
            } ଅନ୍ୟଥା {
                newp = newAstWordSelClone(lhsp, w);
                File_ଧାଡ଼ି* ଅଚଳ rfl = rhsp->fileline();
                ପାଇଁ (unsigned repnum = 1; repnum < times; repnum++) {
                    newp = ନୂତନ AstOr{fl, newWordGrabShift(rfl, w, lhsp, lhswidth * repnum), newp};
                }
            }
            addWordAssign(nodep, w, newp);
        }
        ଫେରନ୍ତୁ ସତ୍ୟ;
    }

    ଶୂନ୍ୟ visitEqNeq(Ast_ଗଣ୍ଠି_Biop* nodep) {
        ଯଦି (nodep->user1SetOnce()) ଫେରନ୍ତୁ;  // Process once
        iterateChildren(nodep);
        ଯଦି (nodep->lhsp()->isWide()
            && !(VN_IS(nodep->lhsp()->dtypep()->skipRefp(), StreamDType)
                 || VN_IS(nodep->rhsp()->dtypep()->skipRefp(), StreamDType))) {
            ଯଦି (isImpure(nodep)) ଫେରନ୍ତୁ;
            ଯଦି (!doExpandWide(nodep->lhsp())) ଫେରନ୍ତୁ;
            ଯଦି (!doExpandWide(nodep->rhsp())) ଫେରନ୍ତୁ;
            didPattern(VN_IS(nodep, Neq) ? VExpandPattern::WORDIZE_NEQ
                                         : VExpandPattern::WORDIZE_EQ,
                       nodep);
            // -> (0=={or{for each_word{WORDSEL(lhs,#)^WORDSEL(rhs,#)}}}
            File_ଧାଡ଼ି* ଅଚଳ fl = nodep->fileline();
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* newp = nullptr;
            ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା w = 0; w < nodep->lhsp()->width_ଶବ୍ଦଗୁଡ଼ିକ(); ++w) {
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ eqp = ନୂତନ AstXor{fl, newAstWordSelClone(nodep->lhsp(), w),
                                                    newAstWordSelClone(nodep->rhsp(), w)};
                newp = newp ? ନୂତନ AstOr{fl, newp, eqp} : eqp;
            }
            ଯଦି (VN_IS(nodep, Neq)) {
                newp = ନୂତନ AstNeq{fl, ନୂତନ AstConst{fl, AstConst::SizedEData{}, 0}, newp};
            } ଅନ୍ୟଥା {
                newp = ନୂତନ AstEq{fl, ନୂତନ AstConst{fl, AstConst::SizedEData{}, 0}, newp};
            }
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        }
    }
    ଶୂନ୍ୟ visit(AstEq* nodep) override { visitEqNeq(nodep); }
    ଶୂନ୍ୟ visit(AstNeq* nodep) override { visitEqNeq(nodep); }

    ଶୂନ୍ୟ visit(AstRedOr* nodep) override {
        ଯଦି (nodep->user1SetOnce()) ଫେରନ୍ତୁ;  // Process once
        iterateChildren(nodep);
        File_ଧାଡ଼ି* ଅଚଳ fl = nodep->fileline();
        ଯଦି (nodep->lhsp()->isWide()) {
            ଯଦି (isImpure(nodep)) ଫେରନ୍ତୁ;
            ଯଦି (!doExpandWide(nodep->lhsp())) ଫେରନ୍ତୁ;
            didPattern(VExpandPattern::WORDIZE_REDOR, nodep);
            // -> (0!={or{for each_word{WORDSEL(lhs,#)}}}
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* newp = nullptr;
            ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା w = 0; w < nodep->lhsp()->width_ଶବ୍ଦଗୁଡ଼ିକ(); ++w) {
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ eqp = newAstWordSelClone(nodep->lhsp(), w);
                newp = newp ? ନୂତନ AstOr{fl, newp, eqp} : eqp;
            }
            newp = ନୂତନ AstNeq{fl, ନୂତନ AstConst{fl, AstConst::SizedEData{}, 0}, newp};
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        } ଅନ୍ୟଥା {
            // No isImpure() check - can handle side effects in below
            didPattern(VExpandPattern::REDOR_TO_NEQ, nodep);
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ lhsp = nodep->lhsp()->unlinkFrBack();
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ newp = ନୂତନ AstNeq{
                fl, ନୂତନ AstConst{fl, AstConst::Widthed_ମୂଲ୍ୟ{}, longOrQuadWidth(nodep), 0}, lhsp};
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        }
    }
    ଶୂନ୍ୟ visit(AstRedAnd* nodep) override {
        ଯଦି (nodep->user1SetOnce()) ଫେରନ୍ତୁ;  // Process once
        iterateChildren(nodep);
        File_ଧାଡ଼ି* ଅଚଳ fl = nodep->fileline();
        ଯଦି (nodep->lhsp()->isWide()) {
            ଯଦି (isImpure(nodep)) ଫେରନ୍ତୁ;
            ଯଦି (!doExpandWide(nodep->lhsp())) ଫେରନ୍ତୁ;
            didPattern(VExpandPattern::WORDIZE_REDAND, nodep);
            // -> (0!={and{for each_word{WORDSEL(lhs,#)}}}
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* newp = nullptr;
            ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା w = 0; w < nodep->lhsp()->width_ଶବ୍ଦଗୁଡ଼ିକ(); ++w) {
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* eqp = newAstWordSelClone(nodep->lhsp(), w);
                ଯଦି (w == nodep->lhsp()->width_ଶବ୍ଦଗୁଡ଼ିକ() - 1) {
                    // Rather than doing a (slowish) ==##, we OR in the
                    // bits that aren't part of the mask
                    eqp = ନୂତନ AstOr{fl, ନୂତନ AstConst{fl, notWideMask(nodep->lhsp())},
                                    // Bug in cppcheck
                                    // cppcheck-suppress memleak
                                    eqp};
                }
                newp = newp ? ନୂତନ AstAnd{fl, newp, eqp} : eqp;
            }
            newp = ନୂତନ AstEq{fl, ନୂତନ AstConst{fl, AstConst::SizedEData{}, VL_MASK_E(VL_EDATASIZE)},
                             newp};
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        } ଅନ୍ୟଥା {
            // No isImpure() check - can handle side effects in below
            didPattern(VExpandPattern::REDAND_TO_EQ, nodep);
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ lhsp = nodep->lhsp()->unlinkFrBack();
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ newp = ନୂତନ AstEq{fl, ନୂତନ AstConst{fl, wordMask(lhsp)}, lhsp};
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        }
    }
    ଶୂନ୍ୟ visit(AstRedXor* nodep) override {
        ଯଦି (nodep->user1SetOnce()) ଫେରନ୍ତୁ;  // Process once
        iterateChildren(nodep);
        ଯଦି (nodep->lhsp()->isWide()) {
            ଯଦି (isImpure(nodep)) ଫେରନ୍ତୁ;
            ଯଦି (!doExpandWide(nodep->lhsp())) ଫେରନ୍ତୁ;
            didPattern(VExpandPattern::WORDIZE_REDXOR, nodep);
            // -> (0!={redxor{for each_word{XOR(WORDSEL(lhs,#))}}}
            File_ଧାଡ଼ି* ଅଚଳ fl = nodep->fileline();
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* newp = nullptr;
            ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା w = 0; w < nodep->lhsp()->width_ଶବ୍ଦଗୁଡ଼ିକ(); ++w) {
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ eqp = newAstWordSelClone(nodep->lhsp(), w);
                newp = newp ? ନୂତନ AstXor{fl, newp, eqp} : eqp;
            }
            newp = ନୂତନ AstRedXor{fl, newp};
            UINFO(8, "    Wordize REDXORnew " << newp);
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        }
        // We don't reduce non-wide XORs, as its more efficient to use a temp register,
        // which the inlined function does nicely.
    }

    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Stmt* nodep) override {
        ଯଦି (nodep->user1SetOnce()) ଫେରନ୍ତୁ;  // Process once
        VL_RESTORER(m_stmtp);
        m_stmtp = nodep;
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Assign* nodep) override {
        ଯଦି (nodep->user1SetOnce()) ଫେରନ୍ତୁ;  // Process once
        ଯଦି (VN_IS(nodep->dtypep()->skipRefp(), UnpackArrayDType)) {
            ଫେରନ୍ତୁ;  // Skip for UnpackArrayDType
        }

        VL_RESTORER(m_stmtp);
        m_stmtp = nodep;
        iterateChildren(nodep);
        ତାର୍କିକ did = ମିଥ୍ୟା;
        ଯଦି (nodep->isWide()  //
            && ((VN_IS(nodep->lhsp(), VarRef) || VN_IS(nodep->lhsp(), ArraySel)))
            && !AstVar::scVarRecurse(nodep->lhsp())  // Need special function for SC
            && !AstVar::scVarRecurse(nodep->rhsp())) {
            ଯଦି (AstConst* ଅଚଳ rhsp = VN_CAST(nodep->rhsp(), Const)) {
                did = expandWide(nodep, rhsp);
            } ଅନ୍ୟଥା ଯଦି (AstVarRef* ଅଚଳ rhsp = VN_CAST(nodep->rhsp(), VarRef)) {
                did = expandWide(nodep, rhsp);
            } ଅନ୍ୟଥା ଯଦି (AstSel* ଅଚଳ rhsp = VN_CAST(nodep->rhsp(), Sel)) {
                did = expandWide(nodep, rhsp);
            } ଅନ୍ୟଥା ଯଦି (AstArraySel* ଅଚଳ rhsp = VN_CAST(nodep->rhsp(), ArraySel)) {
                did = expandWide(nodep, rhsp);
            } ଅନ୍ୟଥା ଯଦି (AstConcat* ଅଚଳ rhsp = VN_CAST(nodep->rhsp(), Concat)) {
                did = expandWide(nodep, rhsp);
            } ଅନ୍ୟଥା ଯଦି (AstExtend* ଅଚଳ rhsp = VN_CAST(nodep->rhsp(), Extend)) {
                did = expandWide(nodep, rhsp);
            } ଅନ୍ୟଥା ଯଦି (AstReplicate* ଅଚଳ rhsp = VN_CAST(nodep->rhsp(), Replicate)) {
                did = expandWide(nodep, rhsp);
            } ଅନ୍ୟଥା ଯଦି (AstAnd* ଅଚଳ rhsp = VN_CAST(nodep->rhsp(), And)) {
                did = expandWide(nodep, rhsp);
            } ଅନ୍ୟଥା ଯଦି (AstOr* ଅଚଳ rhsp = VN_CAST(nodep->rhsp(), Or)) {
                did = expandWide(nodep, rhsp);
            } ଅନ୍ୟଥା ଯଦି (AstNot* ଅଚଳ rhsp = VN_CAST(nodep->rhsp(), Not)) {
                did = expandWide(nodep, rhsp);
            } ଅନ୍ୟଥା ଯଦି (AstXor* ଅଚଳ rhsp = VN_CAST(nodep->rhsp(), Xor)) {
                did = expandWide(nodep, rhsp);
            } ଅନ୍ୟଥା ଯଦି (AstShiftL* ଅଚଳ rhsp = VN_CAST(nodep->rhsp(), ShiftL)) {
                did = expandWideShift(nodep, rhsp, /* isLeftShift: */ ସତ୍ୟ);
            } ଅନ୍ୟଥା ଯଦି (AstShiftR* ଅଚଳ rhsp = VN_CAST(nodep->rhsp(), ShiftR)) {
                did = expandWideShift(nodep, rhsp, /* isLeftShift: */ ମିଥ୍ୟା);
            } ଅନ୍ୟଥା ଯଦି (AstCond* ଅଚଳ rhsp = VN_CAST(nodep->rhsp(), Cond)) {
                did = expandWide(nodep, rhsp);
            }
        } ଅନ୍ୟଥା ଯଦି (AstSel* ଅଚଳ lhsp = VN_CAST(nodep->lhsp(), Sel)) {
            did = expandLhs(nodep, lhsp);
        }
        // Cleanup common code
        ଯଦି (did) VL_DO_DANGLING(nodep->unlinkFrBack()->delete_ବୃକ୍ଷ(), nodep);
    }

    //--------------------
    ଶୂନ୍ୟ visit(AstVar*) override {}  // Don't hit varrefs under vars
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override { iterateChildren(nodep); }

ସର୍ବସାଧାରଣ:
    // CONSTRUCTORS
    explicit ExpandVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~ExpandVisitor() override {
        V3Stats::addStat("Optimizations, Expand, expanded wides", m_statWides);
        V3Stats::addStat("Optimizations, Expand, expanded wide words", m_stat_Wide_ଶବ୍ଦଗୁଡ଼ିକ);
        V3Stats::addStat("Optimizations, Expand, not expanded impure", m_statImpure);
        V3Stats::addStat("Optimizations, Expand, not expanded limit", m_statWideLimited);
        V3Stats::addStat("Optimizations, Expand, temporaries created", m_statTemps);
        ଅଚଳ auto emitStat = [ଏହି](VExpandPattern pattern) {
            std::string str{pattern.ascii()};
            std::transform(str.begin(), str.end(), str.begin(), [](unsigned ଅକ୍ଷର c) {  //
                ଫେରନ୍ତୁ c == '_' ? ' ' : std::tolower(c);
            });
            V3Stats::addStat("Optimizations, Expand, pattern " + str, m_statPatterns[pattern]);
        };
#define EXPAND_PATTERN_EMIT_STAT(id) emitStat(VExpandPattern::id);
        FOR_EACH_EXPAND_PATTERN(EXPAND_PATTERN_EMIT_STAT)
#undef EXPAND_PATTERN_EMIT_STAT
    }
};

//######################################################################
// Expand class functions

ଶୂନ୍ୟ V3Expand::expandAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        ଅଚଳ ExpandOkVisitor okVisitor{nodep};
        ExpandVisitor{nodep};
    }  // Destruct before checking
    V3Global::dump_Check_Global_ବୃକ୍ଷ("expand", 0, dump_ବୃକ୍ଷ_Either_Level() >= 3);
}
