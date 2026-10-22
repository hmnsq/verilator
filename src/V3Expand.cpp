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

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ಅನ್ತರ್ಭಾವ "V3Expand.h"

#ಅನ್ತರ್ಭಾವ "V3Const.h"
#ಅನ್ತರ್ಭಾವ "V3Stats.h"

#ಅನ್ತರ್ಭಾವ <algorithm>
#ಅನ್ತರ್ಭಾವ <array>

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

ವರ್ಗ VExpandPattern final {
ಸಾರ್ವಜನಿಕ:
    ಗಣನಾ en : uint8_t {
#define EXPAND_PATTERN_ID(id) id,
        FOR_EACH_EXPAND_PATTERN(EXPAND_PATTERN_ID)
#undef EXPAND_PATTERN_ID
            _ENUM_END
    };
    ಗಣನಾ en m_e;
    ಧ್ರುವ ವರ್ಣ* ascii() ಧ್ರುವ {
        ಸ್ಥಿರ ಧ್ರುವ ವರ್ಣ* ಧ್ರುವ names[] = {
#define EXPAND_PATTERN_NAME(id) #id,
            FOR_EACH_EXPAND_PATTERN(EXPAND_PATTERN_NAME)
#undef EXPAND_PATTERN_NAME
                "_ENUM_END"  //
        };
        ಪ್ರತ್ಯಾಗಮ names[m_e];
    }

    // cppcheck-suppress noExplicitConstructor
    VExpandPattern(en _e)
        : m_e{_e} {}
    operator en() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_e; }
};

//######################################################################
// Find nodes with side effects, to mark as non-expandable

ವರ್ಗ ExpandOkVisitor final : ಸಾರ್ವಜನಿಕ VNVisitorConst {
    // NODE STATE
    //  AstNode::user2()        -> bool.  Is pure (along with all children)
    ಧ್ರುವ VNUser2InUse m_inuser2;

    // STATE - for current visit position (use VL_RESTORER)
    // Tracks similar to AstNode::isTreePureRecurse(), but avoid O(n^2)
    // False = pure, as nodes that ExpandVisitor inserts preserve pureness
    ತಾರ್ಕಿಕ m_isImpure = ಸತ್ಯ;  // Currently pure

    ಶೂನ್ಯ visit(Ast_ಗಂಟು* nodep) override {
        ತಾರ್ಕಿಕ selfImpure = !nodep->isPure();
        {
            VL_RESTORER(m_isImpure);
            m_isImpure = ಅಸತ್ಯ;
            iterateChildrenConst(nodep);
            selfImpure |= m_isImpure;
            nodep->user2(selfImpure);
        }
        m_isImpure |= selfImpure;
    }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    explicit ExpandOkVisitor(AstNetlist* nodep) { iterateConst(nodep); }
    ~ExpandOkVisitor() = ಸ್ವಭಾವ;
};

//######################################################################
// Expand state, as a visitor of each AstNode

ವರ್ಗ ExpandVisitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
    // NODE STATE
    //  AstNode::user1()        -> bool.  Processed
    //  AstNode::user2()        -> See ExpandOkVisitor
    ಧ್ರುವ VNUser1InUse m_inuser1;

    // STATE - for current visit position (use VL_RESTORER)
    Ast_ಗಂಟು* m_stmtp = nullptr;  // Current statement

    // STATE - across all visitors
    AstCFunc* m_funcp = nullptr;  // Current function
    VDouble0 m_statImpure;  // Statistic tracking
    VDouble0 m_statWides;  // Statistic tracking
    VDouble0 m_stat_Wide_ಪದಗಳು;  // Statistic tracking
    VDouble0 m_statWideLimited;  // Statistic tracking
    VDouble0 m_statTemps;  // Statistic tracking
    // Number of applications of each expansion pattern for statistics
    std::array<VDouble0, VExpandPattern::_ENUM_END> m_statPatterns;

    // STATE - for current function
    size_t m_nTmps = 0;  // Sequence numbers for temopraries

    // METHODS
    // Note the application of the given expansion pattern to 'nodep'
    ಶೂನ್ಯ didPattern(VExpandPattern pattern, Ast_ಗಂಟು* nodep) {
        UINFO(8, "    " << pattern.ascii() << " " << nodep);
        ++m_statPatterns[pattern];
    }

    // Use state that ExpandOkVisitor calculated
    ಸ್ಥಿರ ತಾರ್ಕಿಕ isImpure(Ast_ಗಂಟು* nodep) {
        ಧ್ರುವ ತಾರ್ಕಿಕ impure = nodep->user2();
        ಯದಿ (impure) UINFO(9, "      impure " << nodep);
        ಪ್ರತ್ಯಾಗಮ impure;
    }

    ತಾರ್ಕಿಕ doExpandWide(Ast_ಗಂಟು* nodep) {
        ಯದಿ (isImpure(nodep)) {
            ++m_statImpure;
            ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        }
        ಯದಿ (nodep->width_ಪದಗಳು() <= v3Global.opt.expandLimit()) {
            ++m_statWides;
            m_stat_Wide_ಪದಗಳು += nodep->width_ಪದಗಳು();
            ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        } ಅನ್ಯಥಾ {
            ++m_statWideLimited;
            ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        }
    }

    ಸ್ಥಿರ ಪೂರ್ಣಾಙ್ಕ longOrQuadWidth(Ast_ಗಂಟು* nodep) {
        ಪ್ರತ್ಯಾಗಮ (nodep->width() + (VL_EDATASIZE - 1)) & ~(VL_EDATASIZE - 1);
    }
    ಸ್ಥಿರ V3_ಸಂಖ್ಯೆ notWideMask(Ast_ಗಂಟು* nodep) {
        ಪ್ರತ್ಯಾಗಮ V3_ಸಂಖ್ಯೆ{nodep, VL_EDATASIZE, ~VL_MASK_E(nodep->widthMin())};
    }
    ಸ್ಥಿರ V3_ಸಂಖ್ಯೆ wordMask(Ast_ಗಂಟು* nodep) {
        ಯದಿ (nodep->isWide()) {
            ಪ್ರತ್ಯಾಗಮ V3_ಸಂಖ್ಯೆ{nodep, VL_EDATASIZE, VL_MASK_E(nodep->widthMin())};
        } ಅನ್ಯಥಾ {
            V3_ಸಂಖ್ಯೆ mask{nodep, longOrQuadWidth(nodep)};
            mask.setMask(nodep->widthMin());
            ಪ್ರತ್ಯಾಗಮ mask;
        }
    }

    ಸ್ಥಿರ ಶೂನ್ಯ ಸೇರಿಸು_Before(Ast_ಗಂಟು* placep, Ast_ಗಂಟು* newp) {
        newp->user1(1);  // Already processed, don't need to re-iterate
        placep->addHereThisAsNext(newp);
    }
    ಸ್ಥಿರ ಶೂನ್ಯ replaceWithDelete(Ast_ಗಂಟು* nodep, Ast_ಗಂಟು* newp) {
        newp->user1(1);  // Already processed, don't need to re-iterate
        ಯದಿ (newp->width() != nodep->width()) {
            UASSERT_OBJ(newp->widthMin() == nodep->widthMin(), nodep,
                        "Replacement width mismatch");
            newp->dtypeChgWidth(nodep->width(), nodep->widthMin());
        }
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
    }
    ಸ್ಥಿರ Ast_ಗಂಟು* newWordAssign(Ast_ಗಂಟು_Assign* placep, ಪೂರ್ಣಾಙ್ಕ word, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp,
                                  Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp) {
        File_ಸಾಲು* ಧ್ರುವ fl = placep->fileline();
        ಪ್ರತ್ಯಾಗಮ ನೂತನ AstAssign{fl,
                             ನೂತನ AstWordSel{fl, lhsp->clone_ವೃಕ್ಷ_Pure(ಸತ್ಯ),
                                            ನೂತನ AstConst{fl, static_cast<uint32_t>(word)}},
                             rhsp};
    }
    ಸ್ಥಿರ ಶೂನ್ಯ addWordAssign(Ast_ಗಂಟು_Assign* placep, ಪೂರ್ಣಾಙ್ಕ word, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp,
                              Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp) {
        ಸೇರಿಸು_Before(placep, newWordAssign(placep, word, lhsp, rhsp));
    }
    ಸ್ಥಿರ ಶೂನ್ಯ addWordAssign(Ast_ಗಂಟು_Assign* placep, ಪೂರ್ಣಾಙ್ಕ word, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp) {
        addWordAssign(placep, word, placep->lhsp(), rhsp);
    }

    AstVar* addLocalTmp(Ast_ಗಂಟು* placep, ಧ್ರುವ ವರ್ಣ* namep, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* valuep) {
        ಧ್ರುವ std::string name = "__V"s + namep + "_"s + std::to_string(m_nTmps);
        File_ಸಾಲು* ಧ್ರುವ flp = placep->fileline();
        AstVar* ಧ್ರುವ tmpp = ನೂತನ AstVar{flp, VVarType::STMTTEMP, name, valuep->dtypep()};
        tmpp->funcLocal(ಸತ್ಯ);
        tmpp->isInternal(ಸತ್ಯ);
        tmpp->noReset(ಸತ್ಯ);
        tmpp->substConstOnly(ಸತ್ಯ);
        m_funcp->addVarsp(tmpp);
        ಸೇರಿಸು_Before(placep, ನೂತನ AstAssign{flp, ನೂತನ AstVarRef{flp, tmpp, VAccess::WRITE}, valuep});
        ++m_statTemps;
        ಪ್ರತ್ಯಾಗಮ tmpp;
    }

    ಸ್ಥಿರ ಶೂನ್ಯ fixCloneLvalue(Ast_ಗಂಟು* nodep) {
        // In AstSel transforms, we call clone() on VarRefs that were lvalues,
        // but are now being used on the RHS of the assignment
        ಯದಿ (VN_IS(nodep, VarRef)) VN_AS(nodep, VarRef)->access(VAccess::READ);
        // Iterate
        ಯದಿ (Ast_ಗಂಟು* ಧ್ರುವ refp = nodep->op1p()) fixCloneLvalue(refp);
        ಯದಿ (Ast_ಗಂಟು* ಧ್ರುವ refp = nodep->op2p()) fixCloneLvalue(refp);
        ಯದಿ (Ast_ಗಂಟು* ಧ್ರುವ refp = nodep->op3p()) fixCloneLvalue(refp);
        ಯದಿ (Ast_ಗಂಟು* ಧ್ರುವ refp = nodep->op4p()) fixCloneLvalue(refp);
    }

    ಸ್ಥಿರ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newAstWordSelClone(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep, ಪೂರ್ಣಾಙ್ಕ word) {
        // Get the specified word number from a wide array
        // Or, if it's a long/quad, do appropriate conversion to wide
        // Concat may pass negative word numbers, that means it wants a zero
        File_ಸಾಲು* ಧ್ರುವ fl = nodep->fileline();
        ಯದಿ (nodep->isWide() && word >= 0 && word < nodep->width_ಪದಗಳು()) {
            ಪ್ರತ್ಯಾಗಮ ನೂತನ AstWordSel{fl, nodep->clone_ವೃಕ್ಷ_Pure(ಸತ್ಯ),
                                  ನೂತನ AstConst{fl, static_cast<uint32_t>(word)}};
        } ಅನ್ಯಥಾ ಯದಿ (nodep->isQuad() && word == 0) {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ quadfromp = nodep->clone_ವೃಕ್ಷ_Pure(ಸತ್ಯ);
            quadfromp->dtypeSetBitUnsized(VL_QUADSIZE, quadfromp->widthMin(), VSigning::UNSIGNED);
            ಪ್ರತ್ಯಾಗಮ ನೂತನ AstCCast{fl, quadfromp, VL_EDATASIZE};
        } ಅನ್ಯಥಾ ಯದಿ (nodep->isQuad() && word == 1) {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ quadfromp = nodep->clone_ವೃಕ್ಷ_Pure(ಸತ್ಯ);
            quadfromp->dtypeSetBitUnsized(VL_QUADSIZE, quadfromp->widthMin(), VSigning::UNSIGNED);
            ಪ್ರತ್ಯಾಗಮ ನೂತನ AstCCast{
                fl, ನೂತನ AstShiftR{fl, quadfromp, ನೂತನ AstConst{fl, VL_EDATASIZE}, VL_EDATASIZE},
                VL_EDATASIZE};
        } ಅನ್ಯಥಾ ಯದಿ (!nodep->isWide() && !nodep->isQuad() && word == 0) {
            ಪ್ರತ್ಯಾಗಮ nodep->clone_ವೃಕ್ಷ_Pure(ಸತ್ಯ);
        } ಅನ್ಯಥಾ {  // Out of bounds
            ಪ್ರತ್ಯಾಗಮ ನೂತನ AstConst{fl, 0};
        }
    }

    ಸ್ಥಿರ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newWordGrabShift(File_ಸಾಲು* fl, ಪೂರ್ಣಾಙ್ಕ word, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, ಪೂರ್ಣಾಙ್ಕ shift) {
        // Extract the expression to grab the value for the specified word, if it's the shift
        // of shift bits from lhsp
        // Negative word numbers requested for lhs when it's "before" what we want.
        // We get a 0 then.
        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ othword = word - shift / VL_EDATASIZE;
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ lop = newAstWordSelClone(lhsp, othword);
        ಧ್ರುವ uint32_t loShift = VL_BITBIT_E(shift);
        ಯದಿ (!loShift) ಪ್ರತ್ಯಾಗಮ lop;
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ hip = newAstWordSelClone(lhsp, othword - 1);
        ಧ್ರುವ uint32_t hiShift = VL_EDATASIZE - loShift;  // Complement offset
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ newp
            = ನೂತನ AstOr{fl,  //
                        ನೂತನ AstShiftR{fl, hip, ನೂತನ AstConst{fl, hiShift}, VL_EDATASIZE},
                        ನೂತನ AstShiftL{fl, lop, ನೂತನ AstConst{fl, loShift}, VL_EDATASIZE}};
        ಪ್ರತ್ಯಾಗಮ V3Const::constifyEditCpp(newp);
    }

    // Return expression indexing the word that contains 'lsbp' + the given word offset
    ಸ್ಥಿರ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newWordIndex(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lsbp, uint32_t wordOffset = 0) {
        // This is indexing a WordSel, so a 32 bit constants are fine
        File_ಸಾಲು* ಧ್ರುವ flp = lsbp->fileline();
        ಯದಿ (ಧ್ರುವ AstConst* constp = VN_CAST(lsbp, Const)) {
            ಪ್ರತ್ಯಾಗಮ ನೂತನ AstConst{flp, wordOffset + VL_BITWORD_E(constp->toUInt())};
        }

        ಯದಿ (lsbp->backp()) lsbp = lsbp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ);
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ wwl2p = ನೂತನ AstConst{flp, VL_EDATASIZE_LOG2};  // Word width log 2
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* indexp = ನೂತನ AstShiftR{flp, lsbp, wwl2p, VL_EDATASIZE};
        ಯದಿ (wordOffset) indexp = ನೂತನ AstAdd{flp, ನೂತನ AstConst{flp, wordOffset}, indexp};
        ಪ್ರತ್ಯಾಗಮ indexp;
    }

    // Return word of fromp that contains word indexp + the given word offset.
    ಸ್ಥಿರ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newWordSelWord(File_ಸಾಲು* flp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* fromp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* indexp,
                                       uint32_t wordOffset = 0) {
        UASSERT_OBJ(fromp->isWide(), fromp, "Only need AstWordSel on wide from's");

        ಯದಿ (ಧ್ರುವ AstConst* ಧ್ರುವ constp = VN_CAST(indexp, Const)) {
            indexp = nullptr;
            wordOffset += constp->toUInt();
        }

        // e.g. "logic [95:0] var[0]; logic [0] sel; out = var[sel];"
        // Squash before C++ to avoid getting a C++ compiler warning
        // (even though code would be unreachable as presumably a
        // AstCond is protecting above this node.
        ಯದಿ (wordOffset >= static_cast<uint32_t>(fromp->width_ಪದಗಳು())) {
            ಪ್ರತ್ಯಾಗಮ ನೂತನ AstConst{flp, AstConst::SizedEData{}, 0};
        }

        ಯದಿ (fromp->backp()) fromp = fromp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ);

        // If indexp was constant, just use it
        ಯದಿ (!indexp) ಪ್ರತ್ಯಾಗಮ ನೂತನ AstWordSel{flp, fromp, ನೂತನ AstConst{flp, wordOffset}};

        // Otherwise compute at runtime
        ಯದಿ (indexp->backp()) indexp = indexp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ);
        ಯದಿ (wordOffset) indexp = ನೂತನ AstAdd{flp, ನೂತನ AstConst{flp, wordOffset}, indexp};
        ಪ್ರತ್ಯಾಗಮ ನೂತನ AstWordSel{flp, fromp, indexp};
    }

    // Return word of fromp that contains bit lsbp + the given word offset.
    ಸ್ಥಿರ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newWordSelBit(File_ಸಾಲು* flp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* fromp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lsbp,
                                      uint32_t wordOffset = 0) {
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ indexp = newWordIndex(lsbp, wordOffset);
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ wordSelp = newWordSelWord(flp, fromp, indexp);
        ಯದಿ (!indexp->backp()) VL_DO_DANGLING(indexp->delete_ವೃಕ್ಷ(), indexp);
        ಪ್ರತ್ಯಾಗಮ wordSelp;
    }

    ಸ್ಥಿರ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newSelBitBit(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lsbp) {
        // Return equation to get the VL_BITBIT of a constant or non-constant
        File_ಸಾಲು* ಧ್ರುವ fl = lsbp->fileline();
        ಯದಿ (VN_IS(lsbp, Const)) {
            ಪ್ರತ್ಯಾಗಮ ನೂತನ AstConst{fl, VL_BITBIT_E(VN_AS(lsbp, Const)->toUInt())};
        } ಅನ್ಯಥಾ {
            ಪ್ರತ್ಯಾಗಮ ನೂತನ AstAnd{fl, ನೂತನ AstConst{fl, VL_EDATASIZE - 1}, lsbp->clone_ವೃಕ್ಷ_Pure(ಸತ್ಯ)};
        }
    }

    //====================

    ತಾರ್ಕಿಕ expandWide(Ast_ಗಂಟು_Assign* nodep, AstConst* rhsp) {
        ಯದಿ (!doExpandWide(nodep)) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        didPattern(VExpandPattern::ASSIGN_CONST, nodep);
        // -> {for each_word{ ASSIGN(WORDSEL(wide,#),WORDSEL(CONST,#))}}
        ಯದಿ (rhsp->ಅಂಕೆ().isFourState()) {
            rhsp->v3warn(E_UNSUPPORTED,  // LCOV_EXCL_LINE  // impossible?
                         "Unsupported: 4-state numbers in this context");
        }
        File_ಸಾಲು* ಧ್ರುವ fl = nodep->fileline();
        ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ w = 0; w < nodep->width_ಪದಗಳು(); ++w) {
            addWordAssign(nodep, w,
                          ನೂತನ AstConst{fl, AstConst::SizedEData{}, rhsp->ಅಂಕೆ().edataWord(w)});
        }
        ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
    }
    //-------- Uniops
    ತಾರ್ಕಿಕ expandWide(Ast_ಗಂಟು_Assign* nodep, AstVarRef* rhsp) {
        // Special case: do not expand assignment of constant pool variables.
        // V3Subst undestands these directly.
        ಯದಿ (rhsp->varp()->constPoolEntry()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಯದಿ (!doExpandWide(nodep)) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        didPattern(VExpandPattern::ASSIGN_VARREF, nodep);
        ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ w = 0; w < nodep->width_ಪದಗಳು(); ++w) {
            addWordAssign(nodep, w, newAstWordSelClone(rhsp, w));
        }
        ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
    }
    ತಾರ್ಕಿಕ expandWide(Ast_ಗಂಟು_Assign* nodep, AstArraySel* rhsp) {
        UASSERT_OBJ(!VN_IS(nodep->dtypep()->skipRefp(), UnpackArrayDType), nodep,
                    "ArraySel with unpacked arrays should have been removed in V3Slice");
        ಯದಿ (!doExpandWide(nodep)) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        didPattern(VExpandPattern::ASSIGN_ARRAYSEL, nodep);
        ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ w = 0; w < nodep->width_ಪದಗಳು(); ++w) {
            addWordAssign(nodep, w, newAstWordSelClone(rhsp, w));
        }
        ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
    }
    ತಾರ್ಕಿಕ expandWide(Ast_ಗಂಟು_Assign* nodep, AstNot* rhsp) {
        // -> {for each_word{ ASSIGN(WORDSEL(wide,#),NOT(WORDSEL(lhs,#))) }}
        ಯದಿ (!doExpandWide(nodep)) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        didPattern(VExpandPattern::ASSIGN_NOT, nodep);
        File_ಸಾಲು* ಧ್ರುವ fl = rhsp->fileline();
        ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ w = 0; w < nodep->width_ಪದಗಳು(); ++w) {
            addWordAssign(nodep, w, ನೂತನ AstNot{fl, newAstWordSelClone(rhsp->lhsp(), w)});
        }
        ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
    }
    //-------- Biops
    ತಾರ್ಕಿಕ expandWide(Ast_ಗಂಟು_Assign* nodep, AstAnd* rhsp) {
        ಯದಿ (!doExpandWide(nodep)) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        didPattern(VExpandPattern::ASSIGN_AND, nodep);
        File_ಸಾಲು* ಧ್ರುವ fl = nodep->fileline();
        ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ w = 0; w < nodep->width_ಪದಗಳು(); ++w) {
            addWordAssign(nodep, w,
                          ನೂತನ AstAnd{fl, newAstWordSelClone(rhsp->lhsp(), w),
                                     newAstWordSelClone(rhsp->rhsp(), w)});
        }
        ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
    }
    ತಾರ್ಕಿಕ expandWide(Ast_ಗಂಟು_Assign* nodep, AstOr* rhsp) {
        ಯದಿ (!doExpandWide(nodep)) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        didPattern(VExpandPattern::ASSIGN_OR, nodep);
        File_ಸಾಲು* ಧ್ರುವ fl = nodep->fileline();
        ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ w = 0; w < nodep->width_ಪದಗಳು(); ++w) {
            addWordAssign(nodep, w,
                          ನೂತನ AstOr{fl, newAstWordSelClone(rhsp->lhsp(), w),
                                    newAstWordSelClone(rhsp->rhsp(), w)});
        }
        ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
    }
    ತಾರ್ಕಿಕ expandWide(Ast_ಗಂಟು_Assign* nodep, AstXor* rhsp) {
        ಯದಿ (!doExpandWide(nodep)) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        didPattern(VExpandPattern::ASSIGN_XOR, nodep);
        File_ಸಾಲು* ಧ್ರುವ fl = nodep->fileline();
        ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ w = 0; w < nodep->width_ಪದಗಳು(); ++w) {
            addWordAssign(nodep, w,
                          ನೂತನ AstXor{fl, newAstWordSelClone(rhsp->lhsp(), w),
                                     newAstWordSelClone(rhsp->rhsp(), w)});
        }
        ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
    }
    ತಾರ್ಕಿಕ expandWideShift(Ast_ಗಂಟು_Assign* nodep, Ast_ಗಂಟು_Biop* rhsp, ತಾರ್ಕಿಕ is_ಎಡ_Shift) {
        ಯದಿ (!doExpandWide(nodep)) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;

        // Simplify the shift amount, in case it becomes a constant
        V3Const::constifyEditCpp(rhsp->rhsp());

        // If it's a constant shift by whole words, expand it so V3Subst can substitute it
        ಯದಿ (ಧ್ರುವ AstConst* ಧ್ರುವ rhsConstp = VN_CAST(rhsp->rhsp(), Const)) {
            ಧ್ರುವ uint32_t shiftBits = rhsConstp->toUInt();
            ಯದಿ (VL_BITBIT_E(shiftBits) == 0) {
                ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ width_ಪದಗಳು = nodep->width_ಪದಗಳು();
                ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ shift_ಪದಗಳು = std::min<ಪೂರ್ಣಾಙ್ಕ>(VL_BITWORD_E(shiftBits), width_ಪದಗಳು);
                File_ಸಾಲು* ಧ್ರುವ flp = rhsp->fileline();
                ಯದಿ (is_ಎಡ_Shift) {
                    didPattern(VExpandPattern::ASSIGN_SHIFTL, nodep);
                    // Low words of the result are zero
                    ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ w = 0; w < shift_ಪದಗಳು; ++w) {
                        addWordAssign(nodep, w, ನೂತನ AstConst{flp, AstConst::SizedEData{}, 0});
                    }
                    // High words of the result are copied from higher words of the source
                    ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ w = shift_ಪದಗಳು; w < width_ಪದಗಳು; ++w) {
                        addWordAssign(nodep, w, newAstWordSelClone(rhsp->lhsp(), w - shift_ಪದಗಳು));
                    }
                } ಅನ್ಯಥಾ {
                    didPattern(VExpandPattern::ASSIGN_SHIFTR, nodep);
                    // Low words of the result are copied from higher words of the source
                    ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ w = 0; w < width_ಪದಗಳು - shift_ಪದಗಳು; ++w) {
                        addWordAssign(nodep, w, newAstWordSelClone(rhsp->lhsp(), w + shift_ಪದಗಳು));
                    }
                    // High words of the result are zero
                    ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ w = width_ಪದಗಳು - shift_ಪದಗಳು; w < width_ಪದಗಳು; ++w) {
                        addWordAssign(nodep, w, ನೂತನ AstConst{flp, AstConst::SizedEData{}, 0});
                    }
                }
                ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
            }
        }

        ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    }

    //-------- Triops
    ತಾರ್ಕಿಕ expandWide(Ast_ಗಂಟು_Assign* nodep, AstCond* rhsp) {
        ಯದಿ (!doExpandWide(nodep)) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        didPattern(VExpandPattern::ASSIGN_COND, nodep);
        File_ಸಾಲು* ಧ್ರುವ fl = nodep->fileline();
        ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ w = 0; w < nodep->width_ಪದಗಳು(); ++w) {
            addWordAssign(nodep, w,
                          ನೂತನ AstCond{fl, rhsp->condp()->clone_ವೃಕ್ಷ_Pure(ಸತ್ಯ),
                                      newAstWordSelClone(rhsp->thenp(), w),
                                      newAstWordSelClone(rhsp->elsep(), w)});
        }
        ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
    }

    // VISITORS
    ಶೂನ್ಯ visit(AstNetlist* nodep) override { iterateAndNextNull(nodep->modulesp()); }

    ಶೂನ್ಯ visit(AstCFunc* nodep) override {
        VL_RESTORER(m_funcp);
        VL_RESTORER(m_nTmps);
        m_funcp = nodep;
        m_nTmps = 0;
        ಧ್ರುವ VDouble0 statWidesBefore = m_statWides;
        iterateChildren(nodep);

        // Constant fold here if anything was expanded, as Ast size can likely be reduced
        ಯದಿ (v3Global.opt.fConstEager() && m_statWides != statWidesBefore) {
            Ast_ಗಂಟು* ಧ್ರುವ editedp = V3Const::constifyEditCpp(nodep);
            UASSERT_OBJ(editedp == nodep, editedp, "Should not have replaced CFunc");
        }
    }

    ಶೂನ್ಯ visit(AstExtend* nodep) override {
        ಯದಿ (nodep->user1SetOnce()) ಪ್ರತ್ಯಾಗಮ;  // Process once
        iterateChildren(nodep);
        ಯದಿ (nodep->isWide()) {
            // See under ASSIGN(EXTEND)
        } ಅನ್ಯಥಾ {
            ಯದಿ (isImpure(nodep)) ಪ್ರತ್ಯಾಗಮ;
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ lhsp = nodep->lhsp()->unlinkFrBack();
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newp = lhsp;
            ಯದಿ (nodep->isQuad()) {
                ಯದಿ (lhsp->isQuad()) {
                    lhsp->dtypeFrom(nodep);  // Just mark it, else nop
                } ಅನ್ಯಥಾ ಯದಿ (lhsp->isWide()) {
                    nodep->v3fatalSrc("extending larger thing into smaller?");
                } ಅನ್ಯಥಾ {
                    newp = ನೂತನ AstCCast{nodep->fileline(), lhsp, nodep};
                }
            } ಅನ್ಯಥಾ {  // Long
                UASSERT_OBJ(!(lhsp->isQuad() || lhsp->isWide()), nodep,
                            "extending larger thing into smaller?");
                lhsp->dtypeFrom(nodep);  // Just mark it, else nop
            }
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        }
    }
    ತಾರ್ಕಿಕ expandWide(Ast_ಗಂಟು_Assign* nodep, AstExtend* rhsp) {
        ಯದಿ (!doExpandWide(nodep)) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        didPattern(VExpandPattern::ASSIGN_EXTEND, nodep);
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ rlhsp = rhsp->lhsp();
        ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ w = 0; w < rlhsp->width_ಪದಗಳು(); ++w) {
            addWordAssign(nodep, w, newAstWordSelClone(rlhsp, w));
        }
        ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ w = rlhsp->width_ಪದಗಳು(); w < nodep->width_ಪದಗಳು(); ++w) {
            addWordAssign(nodep, w, ನೂತನ AstConst{nodep->fileline(), AstConst::SizedEData{}, 0});
        }
        ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
    }

    ಶೂನ್ಯ visit(AstSel* nodep) override {
        ಯದಿ (nodep->user1SetOnce()) ಪ್ರತ್ಯಾಗಮ;  // Process once
        iterateChildren(nodep);

        ಧ್ರುವ uint32_t width = static_cast<uint32_t>(nodep->widthConst());
        UASSERT_OBJ(nodep->widthMin() == static_cast<ಪೂರ್ಣಾಙ್ಕ>(width), nodep, "Width mismatch");

        // Skip if Sel is an LHS assignment select
        ಯದಿ (Ast_ಗಂಟು_Assign* ಧ್ರುವ assignp = VN_CAST(nodep->backp(), ಗಂಟು_Assign)) {
            ಯದಿ (nodep == assignp->lhsp()) ಪ್ರತ್ಯಾಗಮ;
        }

        // Skip if wide: See under ASSIGN(WIDE)
        ಯದಿ (nodep->isWide()) ಪ್ರತ್ಯಾಗಮ;

        // Skip Sel from stream or queue
        ಯದಿ (VN_IS(nodep->fromp()->dtypep(), StreamDType)) ಪ್ರತ್ಯಾಗಮ;
        ಯದಿ (VN_IS(nodep->fromp()->dtypep(), QueueDType)) ಪ್ರತ್ಯಾಗಮ;

        // Result must be non-wide after skipping all the above
        UASSERT_OBJ(width <= 64, nodep, "Inconsistent result width");

        ಯದಿ (nodep->fromp()->isWide()) {  // Long/Quad from Wide
            ಯದಿ (isImpure(nodep)) ಪ್ರತ್ಯಾಗಮ;
            didPattern(VExpandPattern::SEL_FROM_WIDE, nodep);
            File_ಸಾಲು* ಧ್ರುವ nfl = nodep->fileline();
            File_ಸಾಲು* ಧ್ರುವ lfl = nodep->lsbp()->fileline();
            File_ಸಾಲು* ಧ್ರುವ ffl = nodep->fromp()->fileline();
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ fromp = nodep->fromp();

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
            ಧ್ರುವ auto tryFold = [&](Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp) -> Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* {
                // Size limit for inlining expressions (any arbitrary value results in
                // a bounded expansion. Picked a value to allow common simple forms.)
                ಸ್ಥಿರ constexpr ಪೂರ್ಣಾಙ್ಕ EXPAND_SEL_LSB_LIMIT = 8;
                // Simplify
                exprp = V3Const::constifyEditCpp(exprp);
                // Accept if not larger than the limit
                ಯದಿ (!exprp->isLargerThan(EXPAND_SEL_LSB_LIMIT)) ಪ್ರತ್ಯಾಗಮ exprp;
                // Delete if rejected
                VL_DO_DANGLING(exprp->delete_ವೃಕ್ಷ(), exprp);
                ಪ್ರತ್ಯಾಗಮ nullptr;
            };

            // Return the index of the word holding the bit 'offset' bits above the LSB
            ಧ್ರುವ auto wordIdx = [&](uint32_t offset) -> Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* {
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ msbp = ನೂತನ AstAdd{lfl, ನೂತನ AstConst{lfl, offset},
                                                     nodep->lsbp()->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ)};
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ idxp = newWordIndex(msbp);
                ಯದಿ (!msbp->backp()) VL_DO_DANGLING(msbp->delete_ವೃಕ್ಷ(), msbp);
                ಪ್ರತ್ಯಾಗಮ idxp;
            };

            // Bit index of the selected LSB within its word
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lBitp = tryFold(newSelBitBit(nodep->lsbp()));
            ಧ್ರುವ ತಾರ್ಕಿಕ aligned = lBitp && lBitp->isZero();

            // Which of the other 2 words the expansion might need
            ಧ್ರುವ ತಾರ್ಕಿಕ mNeeded = width > 1 && !aligned;
            ಧ್ರುವ ತಾರ್ಕಿಕ hNeeded = width > VL_EDATASIZE;
            UASSERT_OBJ(!hNeeded || nodep->isQuad(), nodep, "Width mismatch");

            // Word indices
            ಧ್ರುವ uint32_t mMsbOffset = std::min<uint32_t>(width, VL_EDATASIZE) - 1;
            ಧ್ರುವ uint32_t hMsbOffset = width - 1;
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lIdxp = tryFold(newWordIndex(nodep->lsbp()));
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* mIdxp = mNeeded ? tryFold(wordIdx(mMsbOffset)) : nullptr;
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* hIdxp = hNeeded ? tryFold(wordIdx(hMsbOffset)) : nullptr;

            // If the LSB expression is needed multiple times, evaluate it into a temporary
            ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ nCopies = !lBitp + !lIdxp + (mNeeded && !mIdxp) + (hNeeded && !hIdxp);
            ಯದಿ (nCopies > 1 && m_funcp && m_stmtp && !isImpure(m_stmtp)) {
                ++m_nTmps;  // Use fresh set of temporaries
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ lsbp = nodep->lsbp()->unlinkFrBack();
                AstVar* ಧ್ರುವ tmpp = addLocalTmp(m_stmtp, "ExpandSel_Lsb", lsbp);
                nodep->lsbp(ನೂತನ AstVarRef{lfl, tmpp, VAccess::READ});
            }

            // Rebuild remaining expressions derived from LSB if needed, now using the temporary
            ಯದಿ (!lBitp) lBitp = newSelBitBit(nodep->lsbp());
            ಯದಿ (!lIdxp) lIdxp = newWordIndex(nodep->lsbp());
            ಯದಿ (!mIdxp) mIdxp = mNeeded ? wordIdx(mMsbOffset) : nullptr;
            ಯದಿ (!hIdxp) hIdxp = hNeeded ? wordIdx(hMsbOffset) : nullptr;

            // Return word 'idxp' of 'fromp', without consuming 'idxp'
            ಧ್ರುವ auto wordSel = [&](Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* idxp) -> Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* {
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ clonep = idxp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ);
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ wordp = newWordSelWord(ffl, fromp, clonep);
                ಯದಿ (!clonep->backp()) VL_DO_DANGLING(clonep->delete_ವೃಕ್ಷ(), clonep);
                ಪ್ರತ್ಯಾಗಮ wordp;
            };

            // Construct term containing the bits of the low word - always needed
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ lTermp = [&]() -> Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* {
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lWordp = wordSel(lIdxp);
                ಯದಿ (nodep->isQuad()) lWordp = ನೂತನ AstCCast{nfl, lWordp, nodep};
                ಪ್ರತ್ಯಾಗಮ ನೂತನ AstShiftR{nfl, lWordp, lBitp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ), nodep->width()};
            }();

            // Construct term containing the bits of the middle word - if needed
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ mTermp = [&]() -> Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* {
                ಯದಿ (!mNeeded) ಪ್ರತ್ಯಾಗಮ nullptr;

                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* mWordp = wordSel(mIdxp);
                ಯದಿ (nodep->isQuad()) mWordp = ನೂತನ AstCCast{nfl, mWordp, nodep};
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ mShiftp = ನೂತನ AstSub{lfl, ನೂತನ AstConst{lfl, VL_EDATASIZE},
                                                        lBitp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ)};
                // If we're selecting bit zero, then all 32 bits in the mid word
                // get shifted << by 32 bits, so ignore them.
                ಧ್ರುವ V3_ಸಂಖ್ಯೆ zero{nodep, longOrQuadWidth(nodep)};
                ಪ್ರತ್ಯಾಗಮ ನೂತನ AstCond{
                    nfl,
                    // lsb % VL_EDATASIZE == 0 ?
                    ನೂತನ AstEq{nfl, ನೂತನ AstConst{nfl, 0}, lBitp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ)},
                    // 0 :
                    ನೂತನ AstConst{nfl, zero},
                    //  midword >> (VL_EDATASIZE - (lbs % VL_EDATASIZE))
                    ನೂತನ AstShiftL{nfl, mWordp, mShiftp, nodep->width()}};
            }();

            // Construct term containing the bits of the high word - if needed
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ hTermp = [&]() -> Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* {
                ಯದಿ (!hNeeded) ಪ್ರತ್ಯಾಗಮ nullptr;

                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* hWordp = wordSel(hIdxp);
                hWordp = ನೂತನ AstCCast{nfl, hWordp, nodep};
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ hShiftp = ನೂತನ AstCond{
                    nfl,
                    // lsb % VL_EDATASIZE == 0 ?
                    ನೂತನ AstEq{nfl, ನೂತನ AstConst{nfl, 0}, lBitp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ)},
                    // VL_EDATASIZE :
                    ನೂತನ AstConst{lfl, VL_EDATASIZE},
                    // 64 - (lbs % VL_EDATASIZE)
                    ನೂತನ AstSub{lfl, ನೂತನ AstConst{lfl, 64}, lBitp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ)}};
                ಪ್ರತ್ಯಾಗಮ ನೂತನ AstShiftL{nfl, hWordp, hShiftp, nodep->width()};
            }();

            // Delete parts not captured during construction
            VL_DO_DANGLING(lBitp->delete_ವೃಕ್ಷ(), lBitp);
            VL_DO_DANGLING(lIdxp->delete_ವೃಕ್ಷ(), lIdxp);
            ಯದಿ (mIdxp) VL_DO_DANGLING(mIdxp->delete_ವೃಕ್ಷ(), mIdxp);
            ಯದಿ (hIdxp) VL_DO_DANGLING(hIdxp->delete_ವೃಕ್ಷ(), hIdxp);

            // Or reduce the terms
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newp = lTermp;
            ಯದಿ (mTermp) newp = ನೂತನ AstOr{nfl, mTermp, newp};
            ಯದಿ (hTermp) newp = ನೂತನ AstOr{nfl, hTermp, newp};
            newp->dtypeFrom(nodep);
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        } ಅನ್ಯಥಾ {  // Long/Quad from Long/Quad
            // No isImpure() check - can handle side effects in below
            didPattern(VExpandPattern::SEL_FROM_NARROW, nodep);
            File_ಸಾಲು* ಧ್ರುವ fl = nodep->fileline();
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* fromp = nodep->fromp()->unlinkFrBack();
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ lsbp = nodep->lsbp()->unlinkFrBack();
            ಯದಿ (nodep->isQuad() && !fromp->isQuad()) fromp = ನೂತನ AstCCast{fl, fromp, nodep};
            // {large}>>32 requires 64-bit shift operation; then cast
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newp = ನೂತನ AstShiftR{fl, fromp, lsbp, fromp->width()};
            newp->dtypeFrom(fromp);
            ಯದಿ (!nodep->isQuad() && fromp->isQuad()) newp = ನೂತನ AstCCast{fl, newp, nodep};
            newp->dtypeFrom(nodep);
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        }
    }

    ತಾರ್ಕಿಕ expandWide(Ast_ಗಂಟು_Assign* nodep, AstSel* rhsp) {
        UASSERT_OBJ(nodep->widthMin() == rhsp->widthConst(), nodep, "Width mismatch");
        ಯದಿ (!doExpandWide(nodep)) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;

        // Simplify the index, in case it becomes a constant
        V3Const::constifyEditCpp(rhsp->lsbp());

        // If it's a constant select and aligned, we can just copy the words
        ಯದಿ (ಧ್ರುವ AstConst* ಧ್ರುವ lsbConstp = VN_CAST(rhsp->lsbp(), Const)) {
            ಧ್ರುವ uint32_t lsb = lsbConstp->toUInt();
            ಯದಿ (VL_BITBIT_E(lsb) == 0) {
                didPattern(VExpandPattern::ASSIGN_SEL_ALIGNED, nodep);
                ಧ್ರುವ uint32_t word = VL_BITWORD_E(lsb);
                ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ w = 0; w < nodep->width_ಪದಗಳು(); ++w) {
                    addWordAssign(nodep, w, newAstWordSelClone(rhsp->fromp(), w + word));
                }
                ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
            }
        }

        didPattern(VExpandPattern::ASSIGN_SEL_UNALIGNED, nodep);
        File_ಸಾಲು* ಧ್ರುವ flp = rhsp->fileline();

        // Use fresh set of temporaries
        ++m_nTmps;

        // Compute word index of LSB, store to temporary if not constant
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ wordLsbp = rhsp->lsbp()->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ);
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* wordIdxp = newWordIndex(wordLsbp);
        ಯದಿ (!wordLsbp->backp()) VL_DO_DANGLING(wordLsbp->delete_ವೃಕ್ಷ(), wordLsbp);
        wordIdxp = V3Const::constifyEditCpp(wordIdxp);
        ಯದಿ (!VN_IS(wordIdxp, Const)) {
            AstVar* ಧ್ರುವ tmpp = addLocalTmp(nodep, "ExpandSel_WordIdx", wordIdxp);
            wordIdxp = ನೂತನ AstVarRef{flp, tmpp, VAccess::READ};
        }

        // Compute shift amounts and mask, store to temporaries if not constants
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* loShftp = nullptr;
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* hiShftp = nullptr;
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* hiMaskp = nullptr;
        ಯದಿ (ಧ್ರುವ AstConst* ಧ್ರುವ lsbConstp = VN_CAST(rhsp->lsbp(), Const)) {
            ಧ್ರುವ uint32_t bitOffset = VL_BITBIT_E(lsbConstp->toUInt());
            // Must be unaligned, otherwise we would have handled it above
            UASSERT_OBJ(bitOffset, nodep, "Missed aligned wide select");
            loShftp = ನೂತನ AstConst{flp, bitOffset};
            hiShftp = ನೂತನ AstConst{flp, VL_EDATASIZE - bitOffset};
            hiMaskp = nullptr;
        } ಅನ್ಯಥಾ {
            // Compute Low word shift amount: bottom bits of lsb index
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ lsbp = rhsp->lsbp()->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ);
            loShftp = ನೂತನ AstAnd{flp, ನೂತನ AstConst{flp, VL_SIZEBITS_E}, lsbp};
            AstVar* ಧ್ರುವ loTmpp = addLocalTmp(nodep, "ExpandSel_LoShift", loShftp);
            loShftp = ನೂತನ AstVarRef{flp, loTmpp, VAccess::READ};

            // Compute if aligned: Low word shift amount is 0
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ zerop = ನೂತನ AstConst{flp, 0};
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* alignedp = ನೂತನ AstEq{flp, zerop, loShftp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ)};
            AstVar* ಧ್ರುವ alignedTmpp = addLocalTmp(nodep, "ExpandSel_Aligned", alignedp);
            alignedp = ನೂತನ AstVarRef{flp, alignedTmpp, VAccess::READ};

            // Computed High word shift amount: 0 if aligned else VL_EDATASIZE - Low word shift
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ edsp = ನೂತನ AstConst{flp, VL_EDATASIZE};
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ subp = ನೂತನ AstSub{flp, edsp, loShftp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ)};
            hiShftp = ನೂತನ AstCond{flp, alignedp, ನೂತನ AstConst{flp, 0}, subp};
            AstVar* ಧ್ರುವ hiTmpp = addLocalTmp(nodep, "ExpandSel_HiShift", hiShftp);
            hiShftp = ನೂತನ AstVarRef{flp, hiTmpp, VAccess::READ};

            // Compute the High word mask: 0 if aligned, ones otherwise
            hiMaskp = ನೂತನ AstCond{flp, alignedp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ),
                                  ನೂತನ AstConst{flp, AstConst::SizedEData{}, 0},
                                  ನೂತನ AstConst{flp, AstConst::SizedEData{}, -1ULL}};
            AstVar* ಧ್ರುವ maskTmpp = addLocalTmp(nodep, "ExpandSel_HiMask", hiMaskp);
            hiMaskp = ನೂತನ AstVarRef{flp, maskTmpp, VAccess::READ};
        }

        // Create each word of the selected result
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ fromp = rhsp->fromp();
        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ sel_ಪದಗಳು = nodep->width_ಪದಗಳು();
        ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ w = 0; w < sel_ಪದಗಳು; ++w) {
            // Grab bits from word 'VL_BITWORD_E(lsb) + w'
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ loWordp = newWordSelWord(fromp->fileline(), fromp, wordIdxp, w);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ loShftClonep = loShftp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ lop = ನೂತನ AstShiftR{flp, loWordp, loShftClonep, VL_EDATASIZE};
            // Grab bits from word 'VL_BITWORD_E(lsb) + w + 1'
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* hiWordp = newWordSelWord(fromp->fileline(), fromp, wordIdxp, w + 1);
            // For the last word of the result, avoid an OOB access when the Sel is not OOB
            ಯದಿ (w == sel_ಪದಗಳು - 1) {
                ಧ್ರುವ uint32_t max = fromp->width_ಪದಗಳು() - w - 1;
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ maxp = ನೂತನ AstConst{flp, max};
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ condp = ನೂತನ AstGte{flp, wordIdxp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ), maxp};
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ zerop = ನೂತನ AstConst{flp, AstConst::SizedEData{}, 0};
                hiWordp = ನೂತನ AstCond{flp, condp, zerop, hiWordp};
            }
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ hiShftClonep = hiShftp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ hiPartp = ನೂತನ AstShiftL{flp, hiWordp, hiShftClonep, VL_EDATASIZE};
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ hip
                = hiMaskp ? ನೂತನ AstAnd{flp, hiPartp, hiMaskp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ)} : hiPartp;
            // Combine them
            addWordAssign(nodep, w, ನೂತನ AstOr{nodep->fileline(), hip, lop});
        }

        // Delete parts not captured during construction
        ಯದಿ (!wordIdxp->backp()) VL_DO_DANGLING(wordIdxp->delete_ವೃಕ್ಷ(), wordIdxp);
        ಯದಿ (!loShftp->backp()) VL_DO_DANGLING(loShftp->delete_ವೃಕ್ಷ(), loShftp);
        ಯದಿ (!hiShftp->backp()) VL_DO_DANGLING(hiShftp->delete_ವೃಕ್ಷ(), hiShftp);
        ಯದಿ (hiMaskp && !hiMaskp->backp()) VL_DO_DANGLING(hiMaskp->delete_ವೃಕ್ಷ(), hiMaskp);

        ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
    }

    ತಾರ್ಕಿಕ expandLhs(Ast_ಗಂಟು_Assign* nodep, AstSel* lhsp) {
        // Possibilities
        //      destp: wide or narrow
        //      rhsp:  wide (destp must be wide), narrow, or 1 bit wide
        //      rhsp:  may be allones and can remove AND NOT gate
        //      lsbp:  constant or variable
        // Yuk.
        ಯದಿ (isImpure(nodep)) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        File_ಸಾಲು* ಧ್ರುವ nfl = nodep->fileline();
        File_ಸಾಲು* ಧ್ರುವ lfl = lhsp->fileline();
        ಧ್ರುವ ತಾರ್ಕಿಕ destwide = lhsp->fromp()->isWide();
        ಧ್ರುವ ತಾರ್ಕಿಕ ones = nodep->rhsp()->isAllOnesV();
        ಯದಿ (VN_IS(lhsp->lsbp(), Const)) {
            // The code should work without this constant test, but it won't
            // constify as nicely as we'd like.
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp = nodep->rhsp()->unlinkFrBack();
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ destp = lhsp->fromp()->unlinkFrBack();
            ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ lsb = lhsp->lsbConst();
            ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ msb = lhsp->msbConst();
            V3_ಸಂಖ್ಯೆ maskset{nodep, destp->widthMin()};
            maskset.setMask(msb + 1 - lsb, lsb);
            V3_ಸಂಖ್ಯೆ maskold{nodep, destp->widthMin()};
            maskold.opNot(maskset);
            ಯದಿ (destwide) {
                didPattern(VExpandPattern::ASSIGN_TO_SEL_CONST_WIDE, nodep);
                ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ w = 0; w < destp->width_ಪದಗಳು(); ++w) {
                    ಯದಿ (w >= VL_BITWORD_E(lsb) && w <= VL_BITWORD_E(msb)) {
                        // else we would just be setting it to the same exact value
                        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* oldvalp = newAstWordSelClone(destp, w);
                        fixCloneLvalue(oldvalp);
                        ಯದಿ (!ones) {
                            oldvalp = ನೂತನ AstAnd{
                                lfl,
                                ನೂತನ AstConst{lfl, AstConst::SizedEData{}, maskold.edataWord(w)},
                                oldvalp};
                        }

                        // Appropriate word of new value to insert:
                        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newp = newWordGrabShift(lfl, w, rhsp, lsb);

                        // Apply cleaning at the top word of the destination
                        // (no cleaning to do if dst's width is a whole number
                        // of words).
                        ಯದಿ (w == destp->width_ಪದಗಳು() - 1 && VL_BITBIT_E(destp->widthMin()) != 0) {
                            V3_ಸಂಖ್ಯೆ cleanmask{nodep, VL_EDATASIZE};
                            cleanmask.setMask(VL_BITBIT_E(destp->widthMin()));
                            newp = ನೂತನ AstAnd{lfl, newp, ನೂತನ AstConst{lfl, cleanmask}};
                        }
                        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ orp
                            = V3Const::constifyEditCpp(ನೂತನ AstOr{lfl, oldvalp, newp});
                        addWordAssign(nodep, w, destp, orp);
                    }
                }
                VL_DO_DANGLING(rhsp->delete_ವೃಕ್ಷ(), rhsp);
                VL_DO_DANGLING(destp->delete_ವೃಕ್ಷ(), destp);
            } ಅನ್ಯಥಾ {
                didPattern(VExpandPattern::ASSIGN_TO_SEL_CONST_NARROW, nodep);
                ಯದಿ (destp->isQuad() && !rhsp->isQuad()) rhsp = ನೂತನ AstCCast{nfl, rhsp, nodep};
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* oldvalp = destp->clone_ವೃಕ್ಷ_Pure(ಸತ್ಯ);
                fixCloneLvalue(oldvalp);
                ಯದಿ (!ones) oldvalp = ನೂತನ AstAnd{lfl, ನೂತನ AstConst{lfl, maskold}, oldvalp};

                // The bit-select can refer to bits outside the width of nodep
                // which we aren't allowed to assign to.  This is a mask of the
                // valid range of nodep which we apply to the new shifted RHS.
                V3_ಸಂಖ್ಯೆ cleanmask{nodep, destp->widthMin()};
                cleanmask.setMask(destp->widthMin());
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ shifted = ನೂತನ AstShiftL{
                    lfl, rhsp, ನೂತನ AstConst{lfl, static_cast<uint32_t>(lsb)}, destp->width()};
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ cleaned
                    = ನೂತನ AstAnd{lfl, shifted, ನೂತನ AstConst{lfl, cleanmask}};
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ orp
                    = V3Const::constifyEditCpp(ನೂತನ AstOr{lfl, oldvalp, cleaned});
                ಸೇರಿಸು_Before(nodep, ನೂತನ AstAssign{nfl, destp, orp});
            }
            ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        } ಅನ್ಯಥಾ {  // non-const select offset
            ಯದಿ (destwide && lhsp->widthConst() == 1) {
                didPattern(VExpandPattern::ASSIGN_TO_SEL_VAR_WIDE_ONE_BIT, nodep);
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ rhsp = nodep->rhsp()->unlinkFrBack();
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ destp = lhsp->fromp()->unlinkFrBack();
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* oldvalp
                    = newWordSelBit(lfl, destp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ), lhsp->lsbp());
                fixCloneLvalue(oldvalp);
                ಯದಿ (!ones) {
                    oldvalp = ನೂತನ AstAnd{
                        lfl,
                        ನೂತನ AstNot{
                            lfl, ನೂತನ AstShiftL{lfl, ನೂತನ AstConst{nfl, 1},
                                               // newSelBitBit may exceed the MSB of this variable.
                                               // That's ok as we'd just AND with a larger value,
                                               // but oldval would clip the upper bits to sanity
                                               newSelBitBit(lhsp->lsbp()), VL_EDATASIZE}},
                        oldvalp};
                }
                // Restrict the shift amount to 0-31, see bug804.
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ shiftp = ನೂತನ AstAnd{nfl, lhsp->lsbp()->clone_ವೃಕ್ಷ_Pure(ಸತ್ಯ),
                                                       ನೂತನ AstConst{nfl, VL_EDATASIZE - 1}};
                Ast_ಗಂಟು* ಧ್ರುವ newp = ನೂತನ AstAssign{
                    nfl, newWordSelBit(nfl, destp, lhsp->lsbp()),
                    ನೂತನ AstOr{lfl, oldvalp, ನೂತನ AstShiftL{lfl, rhsp, shiftp, VL_EDATASIZE}}};
                ಸೇರಿಸು_Before(nodep, newp);
                ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
            } ಅನ್ಯಥಾ ಯದಿ (destwide) {
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
                ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            } ಅನ್ಯಥಾ {
                didPattern(VExpandPattern::ASSIGN_TO_SEL_VAR_NARROW, nodep);
                // nodep->dumpTree("-  old: ");
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp = nodep->rhsp()->unlinkFrBack();
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ destp = lhsp->fromp()->unlinkFrBack();
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* oldvalp = destp->clone_ವೃಕ್ಷ_Pure(ಸತ್ಯ);
                fixCloneLvalue(oldvalp);

                V3_ಸಂಖ್ಯೆ maskwidth{nodep, destp->widthMin()};
                maskwidth.setMask(lhsp->widthConst());

                ಯದಿ (destp->isQuad() && !rhsp->isQuad()) rhsp = ನೂತನ AstCCast{nfl, rhsp, nodep};
                ಯದಿ (!ones) {
                    oldvalp = ನೂತನ AstAnd{
                        lfl,
                        ನೂತನ AstNot{lfl, ನೂತನ AstShiftL{lfl, ನೂತನ AstConst{nfl, maskwidth},
                                                      lhsp->lsbp()->clone_ವೃಕ್ಷ_Pure(ಸತ್ಯ),
                                                      destp->width()}},
                        oldvalp};
                }
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newp
                    = ನೂತನ AstShiftL{lfl, rhsp, lhsp->lsbp()->clone_ವೃಕ್ಷ_Pure(ಸತ್ಯ), destp->width()};
                // Apply cleaning to the new value being inserted.  Mask is
                // slightly wider than necessary to avoid an AND with all ones
                // being optimized out.  No need to clean if destp is
                // quad-sized as there are no extra bits to contaminate
                ಯದಿ (destp->widthMin() != 64) {
                    V3_ಸಂಖ್ಯೆ cleanmask{nodep, destp->widthMin() + 1};
                    cleanmask.setMask(destp->widthMin());
                    newp = ನೂತನ AstAnd{lfl, newp, ನೂತನ AstConst{lfl, cleanmask}};
                }

                ಸೇರಿಸು_Before(nodep, ನೂತನ AstAssign{nfl, destp, ನೂತನ AstOr{lfl, oldvalp, newp}});
                ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
            }
        }
    }

    ಶೂನ್ಯ visit(AstConcat* nodep) override {
        ಯದಿ (nodep->user1SetOnce()) ಪ್ರತ್ಯಾಗಮ;  // Process once
        iterateChildren(nodep);
        ಯದಿ (nodep->isWide()) {
            // See under ASSIGN(WIDE)
        } ಅನ್ಯಥಾ {
            // No isImpure() check - can handle side effects in below
            didPattern(VExpandPattern::CONCAT, nodep);
            File_ಸಾಲು* ಧ್ರುವ fl = nodep->fileline();
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp = nodep->lhsp()->unlinkFrBack();
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp = nodep->rhsp()->unlinkFrBack();
            ಧ್ರುವ uint32_t rhsshift = rhsp->widthMin();
            ಯದಿ (nodep->isQuad() && !lhsp->isQuad()) lhsp = ನೂತನ AstCCast{fl, lhsp, nodep};
            ಯದಿ (nodep->isQuad() && !rhsp->isQuad()) rhsp = ನೂತನ AstCCast{fl, rhsp, nodep};
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ newp = ನೂತನ AstOr{
                fl, ನೂತನ AstShiftL{fl, lhsp, ನೂತನ AstConst{fl, rhsshift}, nodep->width()}, rhsp};
            newp->dtypeFrom(nodep);  // Unsigned
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        }
    }
    ತಾರ್ಕಿಕ expandWide(Ast_ಗಂಟು_Assign* nodep, AstConcat* rhsp) {
        ಯದಿ (!doExpandWide(rhsp)) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        didPattern(VExpandPattern::ASSIGN_CONCAT, nodep);
        File_ಸಾಲು* ಧ್ರುವ fl = rhsp->fileline();
        // Lhs or Rhs may be word, long, or quad.
        // newAstWordSelClone nicely abstracts the difference.
        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ rhsshift = rhsp->rhsp()->widthMin();
        // Sometimes doing the words backwards is preferable.
        // When we have x={x,foo} backwards is better, when x={foo,x} forward is better
        // However V3Subst tends to rip this up, so not worth optimizing now.
        ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ w = 0; w < rhsp->width_ಪದಗಳು(); ++w) {
            addWordAssign(nodep, w,
                          ನೂತನ AstOr{fl, newWordGrabShift(fl, w, rhsp->lhsp(), rhsshift),
                                    newAstWordSelClone(rhsp->rhsp(), w)});
        }
        ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
    }

    ಶೂನ್ಯ visit(AstReplicate* nodep) override {
        ಯದಿ (nodep->user1SetOnce()) ಪ್ರತ್ಯಾಗಮ;  // Process once
        iterateChildren(nodep);
        ಯದಿ (nodep->isWide()) {
            // See under ASSIGN(WIDE)
        } ಅನ್ಯಥಾ {
            ಯದಿ (isImpure(nodep)) ಪ್ರತ್ಯಾಗಮ;
            File_ಸಾಲು* ಧ್ರುವ fl = nodep->fileline();
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp = nodep->srcp()->unlinkFrBack();
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newp;
            ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ lhswidth = lhsp->widthMin();
            ಯದಿ (lhswidth == 1) {
                didPattern(VExpandPattern::REPLICATE_ONE_BIT, nodep);
                newp = ನೂತನ AstNegate{fl, lhsp};
            } ಅನ್ಯಥಾ {
                didPattern(VExpandPattern::REPLICATE, nodep);
                ಧ್ರುವ AstConst* ಧ್ರುವ constp = VN_AS(nodep->countp(), Const);
                UASSERT_OBJ(constp, nodep,
                            "Replication value isn't a constant.  Checked earlier!");
                ಧ್ರುವ uint32_t times = constp->toUInt();
                ಯದಿ (nodep->isQuad() && !lhsp->isQuad()) lhsp = ನೂತನ AstCCast{fl, lhsp, nodep};
                newp = lhsp->clone_ವೃಕ್ಷ_Pure(ಸತ್ಯ);
                ಪ್ರತಿ (unsigned repnum = 1; repnum < times; repnum++) {
                    ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ rhsshift = repnum * lhswidth;
                    newp = ನೂತನ AstOr{
                        fl,
                        ನೂತನ AstShiftL{fl, lhsp->clone_ವೃಕ್ಷ_Pure(ಸತ್ಯ),
                                      ನೂತನ AstConst{fl, static_cast<uint32_t>(rhsshift)},
                                      nodep->width()},
                        newp};
                    newp->dtypeFrom(nodep);  // Unsigned
                }
                VL_DO_DANGLING(lhsp->delete_ವೃಕ್ಷ(), lhsp);  // Never used
            }
            newp->dtypeFrom(nodep);  // Unsigned
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        }
    }
    ತಾರ್ಕಿಕ expandWide(Ast_ಗಂಟು_Assign* nodep, AstReplicate* rhsp) {
        ಯದಿ (!doExpandWide(rhsp)) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        didPattern(VExpandPattern::ASSIGN_REPLICATE, nodep);
        File_ಸಾಲು* ಧ್ರುವ fl = nodep->fileline();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ lhsp = rhsp->srcp();
        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ lhswidth = lhsp->widthMin();
        ಧ್ರುವ AstConst* ಧ್ರುವ constp = VN_AS(rhsp->countp(), Const);
        UASSERT_OBJ(constp, rhsp, "Replication value isn't a constant.  Checked earlier!");
        ಧ್ರುವ uint32_t times = constp->toUInt();
        ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ w = 0; w < rhsp->width_ಪದಗಳು(); ++w) {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newp;
            ಯದಿ (lhswidth == 1) {
                newp = ನೂತನ AstNegate{fl, lhsp->clone_ವೃಕ್ಷ_Pure(ಸತ್ಯ)};
                // Replicate always unsigned
                newp->dtypeChgWidthSigned(VL_EDATASIZE, VL_EDATASIZE, VSigning::UNSIGNED);
            } ಅನ್ಯಥಾ {
                newp = newAstWordSelClone(lhsp, w);
                File_ಸಾಲು* ಧ್ರುವ rfl = rhsp->fileline();
                ಪ್ರತಿ (unsigned repnum = 1; repnum < times; repnum++) {
                    newp = ನೂತನ AstOr{fl, newWordGrabShift(rfl, w, lhsp, lhswidth * repnum), newp};
                }
            }
            addWordAssign(nodep, w, newp);
        }
        ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
    }

    ಶೂನ್ಯ visitEqNeq(Ast_ಗಂಟು_Biop* nodep) {
        ಯದಿ (nodep->user1SetOnce()) ಪ್ರತ್ಯಾಗಮ;  // Process once
        iterateChildren(nodep);
        ಯದಿ (nodep->lhsp()->isWide()
            && !(VN_IS(nodep->lhsp()->dtypep()->skipRefp(), StreamDType)
                 || VN_IS(nodep->rhsp()->dtypep()->skipRefp(), StreamDType))) {
            ಯದಿ (isImpure(nodep)) ಪ್ರತ್ಯಾಗಮ;
            ಯದಿ (!doExpandWide(nodep->lhsp())) ಪ್ರತ್ಯಾಗಮ;
            ಯದಿ (!doExpandWide(nodep->rhsp())) ಪ್ರತ್ಯಾಗಮ;
            didPattern(VN_IS(nodep, Neq) ? VExpandPattern::WORDIZE_NEQ
                                         : VExpandPattern::WORDIZE_EQ,
                       nodep);
            // -> (0=={or{for each_word{WORDSEL(lhs,#)^WORDSEL(rhs,#)}}}
            File_ಸಾಲು* ಧ್ರುವ fl = nodep->fileline();
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newp = nullptr;
            ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ w = 0; w < nodep->lhsp()->width_ಪದಗಳು(); ++w) {
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ eqp = ನೂತನ AstXor{fl, newAstWordSelClone(nodep->lhsp(), w),
                                                    newAstWordSelClone(nodep->rhsp(), w)};
                newp = newp ? ನೂತನ AstOr{fl, newp, eqp} : eqp;
            }
            ಯದಿ (VN_IS(nodep, Neq)) {
                newp = ನೂತನ AstNeq{fl, ನೂತನ AstConst{fl, AstConst::SizedEData{}, 0}, newp};
            } ಅನ್ಯಥಾ {
                newp = ನೂತನ AstEq{fl, ನೂತನ AstConst{fl, AstConst::SizedEData{}, 0}, newp};
            }
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        }
    }
    ಶೂನ್ಯ visit(AstEq* nodep) override { visitEqNeq(nodep); }
    ಶೂನ್ಯ visit(AstNeq* nodep) override { visitEqNeq(nodep); }

    ಶೂನ್ಯ visit(AstRedOr* nodep) override {
        ಯದಿ (nodep->user1SetOnce()) ಪ್ರತ್ಯಾಗಮ;  // Process once
        iterateChildren(nodep);
        File_ಸಾಲು* ಧ್ರುವ fl = nodep->fileline();
        ಯದಿ (nodep->lhsp()->isWide()) {
            ಯದಿ (isImpure(nodep)) ಪ್ರತ್ಯಾಗಮ;
            ಯದಿ (!doExpandWide(nodep->lhsp())) ಪ್ರತ್ಯಾಗಮ;
            didPattern(VExpandPattern::WORDIZE_REDOR, nodep);
            // -> (0!={or{for each_word{WORDSEL(lhs,#)}}}
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newp = nullptr;
            ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ w = 0; w < nodep->lhsp()->width_ಪದಗಳು(); ++w) {
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ eqp = newAstWordSelClone(nodep->lhsp(), w);
                newp = newp ? ನೂತನ AstOr{fl, newp, eqp} : eqp;
            }
            newp = ನೂತನ AstNeq{fl, ನೂತನ AstConst{fl, AstConst::SizedEData{}, 0}, newp};
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        } ಅನ್ಯಥಾ {
            // No isImpure() check - can handle side effects in below
            didPattern(VExpandPattern::REDOR_TO_NEQ, nodep);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ lhsp = nodep->lhsp()->unlinkFrBack();
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ newp = ನೂತನ AstNeq{
                fl, ನೂತನ AstConst{fl, AstConst::Widthed_ಮೌಲ್ಯ{}, longOrQuadWidth(nodep), 0}, lhsp};
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        }
    }
    ಶೂನ್ಯ visit(AstRedAnd* nodep) override {
        ಯದಿ (nodep->user1SetOnce()) ಪ್ರತ್ಯಾಗಮ;  // Process once
        iterateChildren(nodep);
        File_ಸಾಲು* ಧ್ರುವ fl = nodep->fileline();
        ಯದಿ (nodep->lhsp()->isWide()) {
            ಯದಿ (isImpure(nodep)) ಪ್ರತ್ಯಾಗಮ;
            ಯದಿ (!doExpandWide(nodep->lhsp())) ಪ್ರತ್ಯಾಗಮ;
            didPattern(VExpandPattern::WORDIZE_REDAND, nodep);
            // -> (0!={and{for each_word{WORDSEL(lhs,#)}}}
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newp = nullptr;
            ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ w = 0; w < nodep->lhsp()->width_ಪದಗಳು(); ++w) {
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* eqp = newAstWordSelClone(nodep->lhsp(), w);
                ಯದಿ (w == nodep->lhsp()->width_ಪದಗಳು() - 1) {
                    // Rather than doing a (slowish) ==##, we OR in the
                    // bits that aren't part of the mask
                    eqp = ನೂತನ AstOr{fl, ನೂತನ AstConst{fl, notWideMask(nodep->lhsp())},
                                    // Bug in cppcheck
                                    // cppcheck-suppress memleak
                                    eqp};
                }
                newp = newp ? ನೂತನ AstAnd{fl, newp, eqp} : eqp;
            }
            newp = ನೂತನ AstEq{fl, ನೂತನ AstConst{fl, AstConst::SizedEData{}, VL_MASK_E(VL_EDATASIZE)},
                             newp};
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        } ಅನ್ಯಥಾ {
            // No isImpure() check - can handle side effects in below
            didPattern(VExpandPattern::REDAND_TO_EQ, nodep);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ lhsp = nodep->lhsp()->unlinkFrBack();
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ newp = ನೂತನ AstEq{fl, ನೂತನ AstConst{fl, wordMask(lhsp)}, lhsp};
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        }
    }
    ಶೂನ್ಯ visit(AstRedXor* nodep) override {
        ಯದಿ (nodep->user1SetOnce()) ಪ್ರತ್ಯಾಗಮ;  // Process once
        iterateChildren(nodep);
        ಯದಿ (nodep->lhsp()->isWide()) {
            ಯದಿ (isImpure(nodep)) ಪ್ರತ್ಯಾಗಮ;
            ಯದಿ (!doExpandWide(nodep->lhsp())) ಪ್ರತ್ಯಾಗಮ;
            didPattern(VExpandPattern::WORDIZE_REDXOR, nodep);
            // -> (0!={redxor{for each_word{XOR(WORDSEL(lhs,#))}}}
            File_ಸಾಲು* ಧ್ರುವ fl = nodep->fileline();
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newp = nullptr;
            ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ w = 0; w < nodep->lhsp()->width_ಪದಗಳು(); ++w) {
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ eqp = newAstWordSelClone(nodep->lhsp(), w);
                newp = newp ? ನೂತನ AstXor{fl, newp, eqp} : eqp;
            }
            newp = ನೂತನ AstRedXor{fl, newp};
            UINFO(8, "    Wordize REDXORnew " << newp);
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        }
        // We don't reduce non-wide XORs, as its more efficient to use a temp register,
        // which the inlined function does nicely.
    }

    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Stmt* nodep) override {
        ಯದಿ (nodep->user1SetOnce()) ಪ್ರತ್ಯಾಗಮ;  // Process once
        VL_RESTORER(m_stmtp);
        m_stmtp = nodep;
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Assign* nodep) override {
        ಯದಿ (nodep->user1SetOnce()) ಪ್ರತ್ಯಾಗಮ;  // Process once
        ಯದಿ (VN_IS(nodep->dtypep()->skipRefp(), UnpackArrayDType)) {
            ಪ್ರತ್ಯಾಗಮ;  // Skip for UnpackArrayDType
        }

        VL_RESTORER(m_stmtp);
        m_stmtp = nodep;
        iterateChildren(nodep);
        ತಾರ್ಕಿಕ did = ಅಸತ್ಯ;
        ಯದಿ (nodep->isWide()  //
            && ((VN_IS(nodep->lhsp(), VarRef) || VN_IS(nodep->lhsp(), ArraySel)))
            && !AstVar::scVarRecurse(nodep->lhsp())  // Need special function for SC
            && !AstVar::scVarRecurse(nodep->rhsp())) {
            ಯದಿ (AstConst* ಧ್ರುವ rhsp = VN_CAST(nodep->rhsp(), Const)) {
                did = expandWide(nodep, rhsp);
            } ಅನ್ಯಥಾ ಯದಿ (AstVarRef* ಧ್ರುವ rhsp = VN_CAST(nodep->rhsp(), VarRef)) {
                did = expandWide(nodep, rhsp);
            } ಅನ್ಯಥಾ ಯದಿ (AstSel* ಧ್ರುವ rhsp = VN_CAST(nodep->rhsp(), Sel)) {
                did = expandWide(nodep, rhsp);
            } ಅನ್ಯಥಾ ಯದಿ (AstArraySel* ಧ್ರುವ rhsp = VN_CAST(nodep->rhsp(), ArraySel)) {
                did = expandWide(nodep, rhsp);
            } ಅನ್ಯಥಾ ಯದಿ (AstConcat* ಧ್ರುವ rhsp = VN_CAST(nodep->rhsp(), Concat)) {
                did = expandWide(nodep, rhsp);
            } ಅನ್ಯಥಾ ಯದಿ (AstExtend* ಧ್ರುವ rhsp = VN_CAST(nodep->rhsp(), Extend)) {
                did = expandWide(nodep, rhsp);
            } ಅನ್ಯಥಾ ಯದಿ (AstReplicate* ಧ್ರುವ rhsp = VN_CAST(nodep->rhsp(), Replicate)) {
                did = expandWide(nodep, rhsp);
            } ಅನ್ಯಥಾ ಯದಿ (AstAnd* ಧ್ರುವ rhsp = VN_CAST(nodep->rhsp(), And)) {
                did = expandWide(nodep, rhsp);
            } ಅನ್ಯಥಾ ಯದಿ (AstOr* ಧ್ರುವ rhsp = VN_CAST(nodep->rhsp(), Or)) {
                did = expandWide(nodep, rhsp);
            } ಅನ್ಯಥಾ ಯದಿ (AstNot* ಧ್ರುವ rhsp = VN_CAST(nodep->rhsp(), Not)) {
                did = expandWide(nodep, rhsp);
            } ಅನ್ಯಥಾ ಯದಿ (AstXor* ಧ್ರುವ rhsp = VN_CAST(nodep->rhsp(), Xor)) {
                did = expandWide(nodep, rhsp);
            } ಅನ್ಯಥಾ ಯದಿ (AstShiftL* ಧ್ರುವ rhsp = VN_CAST(nodep->rhsp(), ShiftL)) {
                did = expandWideShift(nodep, rhsp, /* isLeftShift: */ ಸತ್ಯ);
            } ಅನ್ಯಥಾ ಯದಿ (AstShiftR* ಧ್ರುವ rhsp = VN_CAST(nodep->rhsp(), ShiftR)) {
                did = expandWideShift(nodep, rhsp, /* isLeftShift: */ ಅಸತ್ಯ);
            } ಅನ್ಯಥಾ ಯದಿ (AstCond* ಧ್ರುವ rhsp = VN_CAST(nodep->rhsp(), Cond)) {
                did = expandWide(nodep, rhsp);
            }
        } ಅನ್ಯಥಾ ಯದಿ (AstSel* ಧ್ರುವ lhsp = VN_CAST(nodep->lhsp(), Sel)) {
            did = expandLhs(nodep, lhsp);
        }
        // Cleanup common code
        ಯದಿ (did) VL_DO_DANGLING(nodep->unlinkFrBack()->delete_ವೃಕ್ಷ(), nodep);
    }

    //--------------------
    ಶೂನ್ಯ visit(AstVar*) override {}  // Don't hit varrefs under vars
    ಶೂನ್ಯ visit(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    explicit ExpandVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~ExpandVisitor() override {
        V3Stats::addStat("Optimizations, Expand, expanded wides", m_statWides);
        V3Stats::addStat("Optimizations, Expand, expanded wide words", m_stat_Wide_ಪದಗಳು);
        V3Stats::addStat("Optimizations, Expand, not expanded impure", m_statImpure);
        V3Stats::addStat("Optimizations, Expand, not expanded limit", m_statWideLimited);
        V3Stats::addStat("Optimizations, Expand, temporaries created", m_statTemps);
        ಧ್ರುವ auto emitStat = [ಏತತ್](VExpandPattern pattern) {
            std::string str{pattern.ascii()};
            std::transform(str.begin(), str.end(), str.begin(), [](unsigned ವರ್ಣ c) {  //
                ಪ್ರತ್ಯಾಗಮ c == '_' ? ' ' : std::tolower(c);
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

ಶೂನ್ಯ V3Expand::expandAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        ಧ್ರುವ ExpandOkVisitor okVisitor{nodep};
        ExpandVisitor{nodep};
    }  // Destruct before checking
    V3Global::dump_Check_Global_ವೃಕ್ಷ("expand", 0, dump_ವೃಕ್ಷ_Either_Level() >= 3);
}
