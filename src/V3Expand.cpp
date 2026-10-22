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

#অন্তর্ভাৱ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#অন্তর্ভাৱ "V3Expand.h"

#অন্তর্ভাৱ "V3Const.h"
#অন্তর্ভাৱ "V3Stats.h"

#অন্তর্ভাৱ <algorithm>
#অন্তর্ভাৱ <array>

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

শ্ৰেণী VExpandPattern final {
সার্ৱজনিক:
    গণনা en : uint8_t {
#define EXPAND_PATTERN_ID(id) id,
        FOR_EACH_EXPAND_PATTERN(EXPAND_PATTERN_ID)
#undef EXPAND_PATTERN_ID
            _ENUM_END
    };
    গণনা en m_e;
    ধ্রুৱ ৱর্ণ* ascii() ধ্রুৱ {
        স্থির ধ্রুৱ ৱর্ণ* ধ্রুৱ names[] = {
#define EXPAND_PATTERN_NAME(id) #id,
            FOR_EACH_EXPAND_PATTERN(EXPAND_PATTERN_NAME)
#undef EXPAND_PATTERN_NAME
                "_ENUM_END"  //
        };
        ঘূৰাই names[m_e];
    }

    // cppcheck-suppress noExplicitConstructor
    VExpandPattern(en _e)
        : m_e{_e} {}
    operator en() ধ্রুৱ { ঘূৰাই m_e; }
};

//######################################################################
// Find nodes with side effects, to mark as non-expandable

শ্ৰেণী ExpandOkVisitor final : সার্ৱজনিক VNVisitorConst {
    // NODE STATE
    //  AstNode::user2()        -> bool.  Is pure (along with all children)
    ধ্রুৱ VNUser2InUse m_inuser2;

    // STATE - for current visit position (use VL_RESTORER)
    // Tracks similar to AstNode::isTreePureRecurse(), but avoid O(n^2)
    // False = pure, as nodes that ExpandVisitor inserts preserve pureness
    তার্কিক m_isImpure = সঁচা;  // Currently pure

    শূন্য visit(Ast_গ্ৰন্থি* nodep) override {
        তার্কিক selfImpure = !nodep->isPure();
        {
            VL_RESTORER(m_isImpure);
            m_isImpure = মিছা;
            iterateChildrenConst(nodep);
            selfImpure |= m_isImpure;
            nodep->user2(selfImpure);
        }
        m_isImpure |= selfImpure;
    }

সার্ৱজনিক:
    // CONSTRUCTORS
    explicit ExpandOkVisitor(AstNetlist* nodep) { iterateConst(nodep); }
    ~ExpandOkVisitor() = স্ৱভাৱ;
};

//######################################################################
// Expand state, as a visitor of each AstNode

শ্ৰেণী ExpandVisitor final : সার্ৱজনিক VNVisitor {
    // NODE STATE
    //  AstNode::user1()        -> bool.  Processed
    //  AstNode::user2()        -> See ExpandOkVisitor
    ধ্রুৱ VNUser1InUse m_inuser1;

    // STATE - for current visit position (use VL_RESTORER)
    Ast_গ্ৰন্থি* m_stmtp = nullptr;  // Current statement

    // STATE - across all visitors
    AstCFunc* m_funcp = nullptr;  // Current function
    VDouble0 m_statImpure;  // Statistic tracking
    VDouble0 m_statWides;  // Statistic tracking
    VDouble0 m_stat_Wide_শব্দগুলি;  // Statistic tracking
    VDouble0 m_statWideLimited;  // Statistic tracking
    VDouble0 m_statTemps;  // Statistic tracking
    // Number of applications of each expansion pattern for statistics
    std::array<VDouble0, VExpandPattern::_ENUM_END> m_statPatterns;

    // STATE - for current function
    size_t m_nTmps = 0;  // Sequence numbers for temopraries

    // METHODS
    // Note the application of the given expansion pattern to 'nodep'
    শূন্য didPattern(VExpandPattern pattern, Ast_গ্ৰন্থি* nodep) {
        UINFO(8, "    " << pattern.ascii() << " " << nodep);
        ++m_statPatterns[pattern];
    }

    // Use state that ExpandOkVisitor calculated
    স্থির তার্কিক isImpure(Ast_গ্ৰন্থি* nodep) {
        ধ্রুৱ তার্কিক impure = nodep->user2();
        যদি (impure) UINFO(9, "      impure " << nodep);
        ঘূৰাই impure;
    }

    তার্কিক doExpandWide(Ast_গ্ৰন্থি* nodep) {
        যদি (isImpure(nodep)) {
            ++m_statImpure;
            ঘূৰাই মিছা;
        }
        যদি (nodep->width_শব্দগুলি() <= v3Global.opt.expandLimit()) {
            ++m_statWides;
            m_stat_Wide_শব্দগুলি += nodep->width_শব্দগুলি();
            ঘূৰাই সঁচা;
        } অন্যথা {
            ++m_statWideLimited;
            ঘূৰাই মিছা;
        }
    }

    স্থির পূর্ণাঙ্ক longOrQuadWidth(Ast_গ্ৰন্থি* nodep) {
        ঘূৰাই (nodep->width() + (VL_EDATASIZE - 1)) & ~(VL_EDATASIZE - 1);
    }
    স্থির V3_সংখ্যা notWideMask(Ast_গ্ৰন্থি* nodep) {
        ঘূৰাই V3_সংখ্যা{nodep, VL_EDATASIZE, ~VL_MASK_E(nodep->widthMin())};
    }
    স্থির V3_সংখ্যা wordMask(Ast_গ্ৰন্থি* nodep) {
        যদি (nodep->isWide()) {
            ঘূৰাই V3_সংখ্যা{nodep, VL_EDATASIZE, VL_MASK_E(nodep->widthMin())};
        } অন্যথা {
            V3_সংখ্যা mask{nodep, longOrQuadWidth(nodep)};
            mask.setMask(nodep->widthMin());
            ঘূৰাই mask;
        }
    }

    স্থির শূন্য ঢোকাও_Before(Ast_গ্ৰন্থি* placep, Ast_গ্ৰন্থি* newp) {
        newp->user1(1);  // Already processed, don't need to re-iterate
        placep->addHereThisAsNext(newp);
    }
    স্থির শূন্য replaceWithDelete(Ast_গ্ৰন্থি* nodep, Ast_গ্ৰন্থি* newp) {
        newp->user1(1);  // Already processed, don't need to re-iterate
        যদি (newp->width() != nodep->width()) {
            UASSERT_OBJ(newp->widthMin() == nodep->widthMin(), nodep,
                        "Replacement width mismatch");
            newp->dtypeChgWidth(nodep->width(), nodep->widthMin());
        }
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
    }
    স্থির Ast_গ্ৰন্থি* newWordAssign(Ast_গ্ৰন্থি_Assign* placep, পূর্ণাঙ্ক word, Ast_গ্ৰন্থি_ৰাশিমালা* lhsp,
                                  Ast_গ্ৰন্থি_ৰাশিমালা* rhsp) {
        File_সাৰি* ধ্রুৱ fl = placep->fileline();
        ঘূৰাই নূতন AstAssign{fl,
                             নূতন AstWordSel{fl, lhsp->clone_বৃক্ষ_Pure(সঁচা),
                                            নূতন AstConst{fl, static_cast<uint32_t>(word)}},
                             rhsp};
    }
    স্থির শূন্য addWordAssign(Ast_গ্ৰন্থি_Assign* placep, পূর্ণাঙ্ক word, Ast_গ্ৰন্থি_ৰাশিমালা* lhsp,
                              Ast_গ্ৰন্থি_ৰাশিমালা* rhsp) {
        ঢোকাও_Before(placep, newWordAssign(placep, word, lhsp, rhsp));
    }
    স্থির শূন্য addWordAssign(Ast_গ্ৰন্থি_Assign* placep, পূর্ণাঙ্ক word, Ast_গ্ৰন্থি_ৰাশিমালা* rhsp) {
        addWordAssign(placep, word, placep->lhsp(), rhsp);
    }

    AstVar* addLocalTmp(Ast_গ্ৰন্থি* placep, ধ্রুৱ ৱর্ণ* namep, Ast_গ্ৰন্থি_ৰাশিমালা* valuep) {
        ধ্রুৱ std::string name = "__V"s + namep + "_"s + std::to_string(m_nTmps);
        File_সাৰি* ধ্রুৱ flp = placep->fileline();
        AstVar* ধ্রুৱ tmpp = নূতন AstVar{flp, VVarType::STMTTEMP, name, valuep->dtypep()};
        tmpp->funcLocal(সঁচা);
        tmpp->isInternal(সঁচা);
        tmpp->noReset(সঁচা);
        tmpp->substConstOnly(সঁচা);
        m_funcp->addVarsp(tmpp);
        ঢোকাও_Before(placep, নূতন AstAssign{flp, নূতন AstVarRef{flp, tmpp, VAccess::WRITE}, valuep});
        ++m_statTemps;
        ঘূৰাই tmpp;
    }

    স্থির শূন্য fixCloneLvalue(Ast_গ্ৰন্থি* nodep) {
        // In AstSel transforms, we call clone() on VarRefs that were lvalues,
        // but are now being used on the RHS of the assignment
        যদি (VN_IS(nodep, VarRef)) VN_AS(nodep, VarRef)->access(VAccess::READ);
        // Iterate
        যদি (Ast_গ্ৰন্থি* ধ্রুৱ refp = nodep->op1p()) fixCloneLvalue(refp);
        যদি (Ast_গ্ৰন্থি* ধ্রুৱ refp = nodep->op2p()) fixCloneLvalue(refp);
        যদি (Ast_গ্ৰন্থি* ধ্রুৱ refp = nodep->op3p()) fixCloneLvalue(refp);
        যদি (Ast_গ্ৰন্থি* ধ্রুৱ refp = nodep->op4p()) fixCloneLvalue(refp);
    }

    স্থির Ast_গ্ৰন্থি_ৰাশিমালা* newAstWordSelClone(Ast_গ্ৰন্থি_ৰাশিমালা* nodep, পূর্ণাঙ্ক word) {
        // Get the specified word number from a wide array
        // Or, if it's a long/quad, do appropriate conversion to wide
        // Concat may pass negative word numbers, that means it wants a zero
        File_সাৰি* ধ্রুৱ fl = nodep->fileline();
        যদি (nodep->isWide() && word >= 0 && word < nodep->width_শব্দগুলি()) {
            ঘূৰাই নূতন AstWordSel{fl, nodep->clone_বৃক্ষ_Pure(সঁচা),
                                  নূতন AstConst{fl, static_cast<uint32_t>(word)}};
        } অন্যথা যদি (nodep->isQuad() && word == 0) {
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ quadfromp = nodep->clone_বৃক্ষ_Pure(সঁচা);
            quadfromp->dtypeSetBitUnsized(VL_QUADSIZE, quadfromp->widthMin(), VSigning::UNSIGNED);
            ঘূৰাই নূতন AstCCast{fl, quadfromp, VL_EDATASIZE};
        } অন্যথা যদি (nodep->isQuad() && word == 1) {
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ quadfromp = nodep->clone_বৃক্ষ_Pure(সঁচা);
            quadfromp->dtypeSetBitUnsized(VL_QUADSIZE, quadfromp->widthMin(), VSigning::UNSIGNED);
            ঘূৰাই নূতন AstCCast{
                fl, নূতন AstShiftR{fl, quadfromp, নূতন AstConst{fl, VL_EDATASIZE}, VL_EDATASIZE},
                VL_EDATASIZE};
        } অন্যথা যদি (!nodep->isWide() && !nodep->isQuad() && word == 0) {
            ঘূৰাই nodep->clone_বৃক্ষ_Pure(সঁচা);
        } অন্যথা {  // Out of bounds
            ঘূৰাই নূতন AstConst{fl, 0};
        }
    }

    স্থির Ast_গ্ৰন্থি_ৰাশিমালা* newWordGrabShift(File_সাৰি* fl, পূর্ণাঙ্ক word, Ast_গ্ৰন্থি_ৰাশিমালা* lhsp, পূর্ণাঙ্ক shift) {
        // Extract the expression to grab the value for the specified word, if it's the shift
        // of shift bits from lhsp
        // Negative word numbers requested for lhs when it's "before" what we want.
        // We get a 0 then.
        ধ্রুৱ পূর্ণাঙ্ক othword = word - shift / VL_EDATASIZE;
        Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ lop = newAstWordSelClone(lhsp, othword);
        ধ্রুৱ uint32_t loShift = VL_BITBIT_E(shift);
        যদি (!loShift) ঘূৰাই lop;
        Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ hip = newAstWordSelClone(lhsp, othword - 1);
        ধ্রুৱ uint32_t hiShift = VL_EDATASIZE - loShift;  // Complement offset
        Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ newp
            = নূতন AstOr{fl,  //
                        নূতন AstShiftR{fl, hip, নূতন AstConst{fl, hiShift}, VL_EDATASIZE},
                        নূতন AstShiftL{fl, lop, নূতন AstConst{fl, loShift}, VL_EDATASIZE}};
        ঘূৰাই V3Const::constifyEditCpp(newp);
    }

    // Return expression indexing the word that contains 'lsbp' + the given word offset
    স্থির Ast_গ্ৰন্থি_ৰাশিমালা* newWordIndex(Ast_গ্ৰন্থি_ৰাশিমালা* lsbp, uint32_t wordOffset = 0) {
        // This is indexing a WordSel, so a 32 bit constants are fine
        File_সাৰি* ধ্রুৱ flp = lsbp->fileline();
        যদি (ধ্রুৱ AstConst* constp = VN_CAST(lsbp, Const)) {
            ঘূৰাই নূতন AstConst{flp, wordOffset + VL_BITWORD_E(constp->toUInt())};
        }

        যদি (lsbp->backp()) lsbp = lsbp->clone_বৃক্ষ_Pure(মিছা);
        Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ wwl2p = নূতন AstConst{flp, VL_EDATASIZE_LOG2};  // Word width log 2
        Ast_গ্ৰন্থি_ৰাশিমালা* indexp = নূতন AstShiftR{flp, lsbp, wwl2p, VL_EDATASIZE};
        যদি (wordOffset) indexp = নূতন AstAdd{flp, নূতন AstConst{flp, wordOffset}, indexp};
        ঘূৰাই indexp;
    }

    // Return word of fromp that contains word indexp + the given word offset.
    স্থির Ast_গ্ৰন্থি_ৰাশিমালা* newWordSelWord(File_সাৰি* flp, Ast_গ্ৰন্থি_ৰাশিমালা* fromp, Ast_গ্ৰন্থি_ৰাশিমালা* indexp,
                                       uint32_t wordOffset = 0) {
        UASSERT_OBJ(fromp->isWide(), fromp, "Only need AstWordSel on wide from's");

        যদি (ধ্রুৱ AstConst* ধ্রুৱ constp = VN_CAST(indexp, Const)) {
            indexp = nullptr;
            wordOffset += constp->toUInt();
        }

        // e.g. "logic [95:0] var[0]; logic [0] sel; out = var[sel];"
        // Squash before C++ to avoid getting a C++ compiler warning
        // (even though code would be unreachable as presumably a
        // AstCond is protecting above this node.
        যদি (wordOffset >= static_cast<uint32_t>(fromp->width_শব্দগুলি())) {
            ঘূৰাই নূতন AstConst{flp, AstConst::SizedEData{}, 0};
        }

        যদি (fromp->backp()) fromp = fromp->clone_বৃক্ষ_Pure(মিছা);

        // If indexp was constant, just use it
        যদি (!indexp) ঘূৰাই নূতন AstWordSel{flp, fromp, নূতন AstConst{flp, wordOffset}};

        // Otherwise compute at runtime
        যদি (indexp->backp()) indexp = indexp->clone_বৃক্ষ_Pure(মিছা);
        যদি (wordOffset) indexp = নূতন AstAdd{flp, নূতন AstConst{flp, wordOffset}, indexp};
        ঘূৰাই নূতন AstWordSel{flp, fromp, indexp};
    }

    // Return word of fromp that contains bit lsbp + the given word offset.
    স্থির Ast_গ্ৰন্থি_ৰাশিমালা* newWordSelBit(File_সাৰি* flp, Ast_গ্ৰন্থি_ৰাশিমালা* fromp, Ast_গ্ৰন্থি_ৰাশিমালা* lsbp,
                                      uint32_t wordOffset = 0) {
        Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ indexp = newWordIndex(lsbp, wordOffset);
        Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ wordSelp = newWordSelWord(flp, fromp, indexp);
        যদি (!indexp->backp()) VL_DO_DANGLING(indexp->delete_বৃক্ষ(), indexp);
        ঘূৰাই wordSelp;
    }

    স্থির Ast_গ্ৰন্থি_ৰাশিমালা* newSelBitBit(Ast_গ্ৰন্থি_ৰাশিমালা* lsbp) {
        // Return equation to get the VL_BITBIT of a constant or non-constant
        File_সাৰি* ধ্রুৱ fl = lsbp->fileline();
        যদি (VN_IS(lsbp, Const)) {
            ঘূৰাই নূতন AstConst{fl, VL_BITBIT_E(VN_AS(lsbp, Const)->toUInt())};
        } অন্যথা {
            ঘূৰাই নূতন AstAnd{fl, নূতন AstConst{fl, VL_EDATASIZE - 1}, lsbp->clone_বৃক্ষ_Pure(সঁচা)};
        }
    }

    //====================

    তার্কিক expandWide(Ast_গ্ৰন্থি_Assign* nodep, AstConst* rhsp) {
        যদি (!doExpandWide(nodep)) ঘূৰাই মিছা;
        didPattern(VExpandPattern::ASSIGN_CONST, nodep);
        // -> {for each_word{ ASSIGN(WORDSEL(wide,#),WORDSEL(CONST,#))}}
        যদি (rhsp->অঙ্ক().isFourState()) {
            rhsp->v3warn(E_UNSUPPORTED,  // LCOV_EXCL_LINE  // impossible?
                         "Unsupported: 4-state numbers in this context");
        }
        File_সাৰি* ধ্রুৱ fl = nodep->fileline();
        বাবে (পূর্ণাঙ্ক w = 0; w < nodep->width_শব্দগুলি(); ++w) {
            addWordAssign(nodep, w,
                          নূতন AstConst{fl, AstConst::SizedEData{}, rhsp->অঙ্ক().edataWord(w)});
        }
        ঘূৰাই সঁচা;
    }
    //-------- Uniops
    তার্কিক expandWide(Ast_গ্ৰন্থি_Assign* nodep, AstVarRef* rhsp) {
        // Special case: do not expand assignment of constant pool variables.
        // V3Subst undestands these directly.
        যদি (rhsp->varp()->constPoolEntry()) ঘূৰাই মিছা;
        যদি (!doExpandWide(nodep)) ঘূৰাই মিছা;
        didPattern(VExpandPattern::ASSIGN_VARREF, nodep);
        বাবে (পূর্ণাঙ্ক w = 0; w < nodep->width_শব্দগুলি(); ++w) {
            addWordAssign(nodep, w, newAstWordSelClone(rhsp, w));
        }
        ঘূৰাই সঁচা;
    }
    তার্কিক expandWide(Ast_গ্ৰন্থি_Assign* nodep, AstArraySel* rhsp) {
        UASSERT_OBJ(!VN_IS(nodep->dtypep()->skipRefp(), UnpackArrayDType), nodep,
                    "ArraySel with unpacked arrays should have been removed in V3Slice");
        যদি (!doExpandWide(nodep)) ঘূৰাই মিছা;
        didPattern(VExpandPattern::ASSIGN_ARRAYSEL, nodep);
        বাবে (পূর্ণাঙ্ক w = 0; w < nodep->width_শব্দগুলি(); ++w) {
            addWordAssign(nodep, w, newAstWordSelClone(rhsp, w));
        }
        ঘূৰাই সঁচা;
    }
    তার্কিক expandWide(Ast_গ্ৰন্থি_Assign* nodep, AstNot* rhsp) {
        // -> {for each_word{ ASSIGN(WORDSEL(wide,#),NOT(WORDSEL(lhs,#))) }}
        যদি (!doExpandWide(nodep)) ঘূৰাই মিছা;
        didPattern(VExpandPattern::ASSIGN_NOT, nodep);
        File_সাৰি* ধ্রুৱ fl = rhsp->fileline();
        বাবে (পূর্ণাঙ্ক w = 0; w < nodep->width_শব্দগুলি(); ++w) {
            addWordAssign(nodep, w, নূতন AstNot{fl, newAstWordSelClone(rhsp->lhsp(), w)});
        }
        ঘূৰাই সঁচা;
    }
    //-------- Biops
    তার্কিক expandWide(Ast_গ্ৰন্থি_Assign* nodep, AstAnd* rhsp) {
        যদি (!doExpandWide(nodep)) ঘূৰাই মিছা;
        didPattern(VExpandPattern::ASSIGN_AND, nodep);
        File_সাৰি* ধ্রুৱ fl = nodep->fileline();
        বাবে (পূর্ণাঙ্ক w = 0; w < nodep->width_শব্দগুলি(); ++w) {
            addWordAssign(nodep, w,
                          নূতন AstAnd{fl, newAstWordSelClone(rhsp->lhsp(), w),
                                     newAstWordSelClone(rhsp->rhsp(), w)});
        }
        ঘূৰাই সঁচা;
    }
    তার্কিক expandWide(Ast_গ্ৰন্থি_Assign* nodep, AstOr* rhsp) {
        যদি (!doExpandWide(nodep)) ঘূৰাই মিছা;
        didPattern(VExpandPattern::ASSIGN_OR, nodep);
        File_সাৰি* ধ্রুৱ fl = nodep->fileline();
        বাবে (পূর্ণাঙ্ক w = 0; w < nodep->width_শব্দগুলি(); ++w) {
            addWordAssign(nodep, w,
                          নূতন AstOr{fl, newAstWordSelClone(rhsp->lhsp(), w),
                                    newAstWordSelClone(rhsp->rhsp(), w)});
        }
        ঘূৰাই সঁচা;
    }
    তার্কিক expandWide(Ast_গ্ৰন্থি_Assign* nodep, AstXor* rhsp) {
        যদি (!doExpandWide(nodep)) ঘূৰাই মিছা;
        didPattern(VExpandPattern::ASSIGN_XOR, nodep);
        File_সাৰি* ধ্রুৱ fl = nodep->fileline();
        বাবে (পূর্ণাঙ্ক w = 0; w < nodep->width_শব্দগুলি(); ++w) {
            addWordAssign(nodep, w,
                          নূতন AstXor{fl, newAstWordSelClone(rhsp->lhsp(), w),
                                     newAstWordSelClone(rhsp->rhsp(), w)});
        }
        ঘূৰাই সঁচা;
    }
    তার্কিক expandWideShift(Ast_গ্ৰন্থি_Assign* nodep, Ast_গ্ৰন্থি_Biop* rhsp, তার্কিক is_বাম_Shift) {
        যদি (!doExpandWide(nodep)) ঘূৰাই মিছা;

        // Simplify the shift amount, in case it becomes a constant
        V3Const::constifyEditCpp(rhsp->rhsp());

        // If it's a constant shift by whole words, expand it so V3Subst can substitute it
        যদি (ধ্রুৱ AstConst* ধ্রুৱ rhsConstp = VN_CAST(rhsp->rhsp(), Const)) {
            ধ্রুৱ uint32_t shiftBits = rhsConstp->toUInt();
            যদি (VL_BITBIT_E(shiftBits) == 0) {
                ধ্রুৱ পূর্ণাঙ্ক width_শব্দগুলি = nodep->width_শব্দগুলি();
                ধ্রুৱ পূর্ণাঙ্ক shift_শব্দগুলি = std::min<পূর্ণাঙ্ক>(VL_BITWORD_E(shiftBits), width_শব্দগুলি);
                File_সাৰি* ধ্রুৱ flp = rhsp->fileline();
                যদি (is_বাম_Shift) {
                    didPattern(VExpandPattern::ASSIGN_SHIFTL, nodep);
                    // Low words of the result are zero
                    বাবে (পূর্ণাঙ্ক w = 0; w < shift_শব্দগুলি; ++w) {
                        addWordAssign(nodep, w, নূতন AstConst{flp, AstConst::SizedEData{}, 0});
                    }
                    // High words of the result are copied from higher words of the source
                    বাবে (পূর্ণাঙ্ক w = shift_শব্দগুলি; w < width_শব্দগুলি; ++w) {
                        addWordAssign(nodep, w, newAstWordSelClone(rhsp->lhsp(), w - shift_শব্দগুলি));
                    }
                } অন্যথা {
                    didPattern(VExpandPattern::ASSIGN_SHIFTR, nodep);
                    // Low words of the result are copied from higher words of the source
                    বাবে (পূর্ণাঙ্ক w = 0; w < width_শব্দগুলি - shift_শব্দগুলি; ++w) {
                        addWordAssign(nodep, w, newAstWordSelClone(rhsp->lhsp(), w + shift_শব্দগুলি));
                    }
                    // High words of the result are zero
                    বাবে (পূর্ণাঙ্ক w = width_শব্দগুলি - shift_শব্দগুলি; w < width_শব্দগুলি; ++w) {
                        addWordAssign(nodep, w, নূতন AstConst{flp, AstConst::SizedEData{}, 0});
                    }
                }
                ঘূৰাই সঁচা;
            }
        }

        ঘূৰাই মিছা;
    }

    //-------- Triops
    তার্কিক expandWide(Ast_গ্ৰন্থি_Assign* nodep, AstCond* rhsp) {
        যদি (!doExpandWide(nodep)) ঘূৰাই মিছা;
        didPattern(VExpandPattern::ASSIGN_COND, nodep);
        File_সাৰি* ধ্রুৱ fl = nodep->fileline();
        বাবে (পূর্ণাঙ্ক w = 0; w < nodep->width_শব্দগুলি(); ++w) {
            addWordAssign(nodep, w,
                          নূতন AstCond{fl, rhsp->condp()->clone_বৃক্ষ_Pure(সঁচা),
                                      newAstWordSelClone(rhsp->thenp(), w),
                                      newAstWordSelClone(rhsp->elsep(), w)});
        }
        ঘূৰাই সঁচা;
    }

    // VISITORS
    শূন্য visit(AstNetlist* nodep) override { iterateAndNextNull(nodep->modulesp()); }

    শূন্য visit(AstCFunc* nodep) override {
        VL_RESTORER(m_funcp);
        VL_RESTORER(m_nTmps);
        m_funcp = nodep;
        m_nTmps = 0;
        ধ্রুৱ VDouble0 statWidesBefore = m_statWides;
        iterateChildren(nodep);

        // Constant fold here if anything was expanded, as Ast size can likely be reduced
        যদি (v3Global.opt.fConstEager() && m_statWides != statWidesBefore) {
            Ast_গ্ৰন্থি* ধ্রুৱ editedp = V3Const::constifyEditCpp(nodep);
            UASSERT_OBJ(editedp == nodep, editedp, "Should not have replaced CFunc");
        }
    }

    শূন্য visit(AstExtend* nodep) override {
        যদি (nodep->user1SetOnce()) ঘূৰাই;  // Process once
        iterateChildren(nodep);
        যদি (nodep->isWide()) {
            // See under ASSIGN(EXTEND)
        } অন্যথা {
            যদি (isImpure(nodep)) ঘূৰাই;
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ lhsp = nodep->lhsp()->unlinkFrBack();
            Ast_গ্ৰন্থি_ৰাশিমালা* newp = lhsp;
            যদি (nodep->isQuad()) {
                যদি (lhsp->isQuad()) {
                    lhsp->dtypeFrom(nodep);  // Just mark it, else nop
                } অন্যথা যদি (lhsp->isWide()) {
                    nodep->v3fatalSrc("extending larger thing into smaller?");
                } অন্যথা {
                    newp = নূতন AstCCast{nodep->fileline(), lhsp, nodep};
                }
            } অন্যথা {  // Long
                UASSERT_OBJ(!(lhsp->isQuad() || lhsp->isWide()), nodep,
                            "extending larger thing into smaller?");
                lhsp->dtypeFrom(nodep);  // Just mark it, else nop
            }
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        }
    }
    তার্কিক expandWide(Ast_গ্ৰন্থি_Assign* nodep, AstExtend* rhsp) {
        যদি (!doExpandWide(nodep)) ঘূৰাই মিছা;
        didPattern(VExpandPattern::ASSIGN_EXTEND, nodep);
        Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ rlhsp = rhsp->lhsp();
        বাবে (পূর্ণাঙ্ক w = 0; w < rlhsp->width_শব্দগুলি(); ++w) {
            addWordAssign(nodep, w, newAstWordSelClone(rlhsp, w));
        }
        বাবে (পূর্ণাঙ্ক w = rlhsp->width_শব্দগুলি(); w < nodep->width_শব্দগুলি(); ++w) {
            addWordAssign(nodep, w, নূতন AstConst{nodep->fileline(), AstConst::SizedEData{}, 0});
        }
        ঘূৰাই সঁচা;
    }

    শূন্য visit(AstSel* nodep) override {
        যদি (nodep->user1SetOnce()) ঘূৰাই;  // Process once
        iterateChildren(nodep);

        ধ্রুৱ uint32_t width = static_cast<uint32_t>(nodep->widthConst());
        UASSERT_OBJ(nodep->widthMin() == static_cast<পূর্ণাঙ্ক>(width), nodep, "Width mismatch");

        // Skip if Sel is an LHS assignment select
        যদি (Ast_গ্ৰন্থি_Assign* ধ্রুৱ assignp = VN_CAST(nodep->backp(), গ্ৰন্থি_Assign)) {
            যদি (nodep == assignp->lhsp()) ঘূৰাই;
        }

        // Skip if wide: See under ASSIGN(WIDE)
        যদি (nodep->isWide()) ঘূৰাই;

        // Skip Sel from stream or queue
        যদি (VN_IS(nodep->fromp()->dtypep(), StreamDType)) ঘূৰাই;
        যদি (VN_IS(nodep->fromp()->dtypep(), QueueDType)) ঘূৰাই;

        // Result must be non-wide after skipping all the above
        UASSERT_OBJ(width <= 64, nodep, "Inconsistent result width");

        যদি (nodep->fromp()->isWide()) {  // Long/Quad from Wide
            যদি (isImpure(nodep)) ঘূৰাই;
            didPattern(VExpandPattern::SEL_FROM_WIDE, nodep);
            File_সাৰি* ধ্রুৱ nfl = nodep->fileline();
            File_সাৰি* ধ্রুৱ lfl = nodep->lsbp()->fileline();
            File_সাৰি* ধ্রুৱ ffl = nodep->fromp()->fileline();
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ fromp = nodep->fromp();

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
            ধ্রুৱ auto tryFold = [&](Ast_গ্ৰন্থি_ৰাশিমালা* exprp) -> Ast_গ্ৰন্থি_ৰাশিমালা* {
                // Size limit for inlining expressions (any arbitrary value results in
                // a bounded expansion. Picked a value to allow common simple forms.)
                স্থির constexpr পূর্ণাঙ্ক EXPAND_SEL_LSB_LIMIT = 8;
                // Simplify
                exprp = V3Const::constifyEditCpp(exprp);
                // Accept if not larger than the limit
                যদি (!exprp->isLargerThan(EXPAND_SEL_LSB_LIMIT)) ঘূৰাই exprp;
                // Delete if rejected
                VL_DO_DANGLING(exprp->delete_বৃক্ষ(), exprp);
                ঘূৰাই nullptr;
            };

            // Return the index of the word holding the bit 'offset' bits above the LSB
            ধ্রুৱ auto wordIdx = [&](uint32_t offset) -> Ast_গ্ৰন্থি_ৰাশিমালা* {
                Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ msbp = নূতন AstAdd{lfl, নূতন AstConst{lfl, offset},
                                                     nodep->lsbp()->clone_বৃক্ষ_Pure(মিছা)};
                Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ idxp = newWordIndex(msbp);
                যদি (!msbp->backp()) VL_DO_DANGLING(msbp->delete_বৃক্ষ(), msbp);
                ঘূৰাই idxp;
            };

            // Bit index of the selected LSB within its word
            Ast_গ্ৰন্থি_ৰাশিমালা* lBitp = tryFold(newSelBitBit(nodep->lsbp()));
            ধ্রুৱ তার্কিক aligned = lBitp && lBitp->isZero();

            // Which of the other 2 words the expansion might need
            ধ্রুৱ তার্কিক mNeeded = width > 1 && !aligned;
            ধ্রুৱ তার্কিক hNeeded = width > VL_EDATASIZE;
            UASSERT_OBJ(!hNeeded || nodep->isQuad(), nodep, "Width mismatch");

            // Word indices
            ধ্রুৱ uint32_t mMsbOffset = std::min<uint32_t>(width, VL_EDATASIZE) - 1;
            ধ্রুৱ uint32_t hMsbOffset = width - 1;
            Ast_গ্ৰন্থি_ৰাশিমালা* lIdxp = tryFold(newWordIndex(nodep->lsbp()));
            Ast_গ্ৰন্থি_ৰাশিমালা* mIdxp = mNeeded ? tryFold(wordIdx(mMsbOffset)) : nullptr;
            Ast_গ্ৰন্থি_ৰাশিমালা* hIdxp = hNeeded ? tryFold(wordIdx(hMsbOffset)) : nullptr;

            // If the LSB expression is needed multiple times, evaluate it into a temporary
            ধ্রুৱ পূর্ণাঙ্ক nCopies = !lBitp + !lIdxp + (mNeeded && !mIdxp) + (hNeeded && !hIdxp);
            যদি (nCopies > 1 && m_funcp && m_stmtp && !isImpure(m_stmtp)) {
                ++m_nTmps;  // Use fresh set of temporaries
                Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ lsbp = nodep->lsbp()->unlinkFrBack();
                AstVar* ধ্রুৱ tmpp = addLocalTmp(m_stmtp, "ExpandSel_Lsb", lsbp);
                nodep->lsbp(নূতন AstVarRef{lfl, tmpp, VAccess::READ});
            }

            // Rebuild remaining expressions derived from LSB if needed, now using the temporary
            যদি (!lBitp) lBitp = newSelBitBit(nodep->lsbp());
            যদি (!lIdxp) lIdxp = newWordIndex(nodep->lsbp());
            যদি (!mIdxp) mIdxp = mNeeded ? wordIdx(mMsbOffset) : nullptr;
            যদি (!hIdxp) hIdxp = hNeeded ? wordIdx(hMsbOffset) : nullptr;

            // Return word 'idxp' of 'fromp', without consuming 'idxp'
            ধ্রুৱ auto wordSel = [&](Ast_গ্ৰন্থি_ৰাশিমালা* idxp) -> Ast_গ্ৰন্থি_ৰাশিমালা* {
                Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ clonep = idxp->clone_বৃক্ষ_Pure(মিছা);
                Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ wordp = newWordSelWord(ffl, fromp, clonep);
                যদি (!clonep->backp()) VL_DO_DANGLING(clonep->delete_বৃক্ষ(), clonep);
                ঘূৰাই wordp;
            };

            // Construct term containing the bits of the low word - always needed
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ lTermp = [&]() -> Ast_গ্ৰন্থি_ৰাশিমালা* {
                Ast_গ্ৰন্থি_ৰাশিমালা* lWordp = wordSel(lIdxp);
                যদি (nodep->isQuad()) lWordp = নূতন AstCCast{nfl, lWordp, nodep};
                ঘূৰাই নূতন AstShiftR{nfl, lWordp, lBitp->clone_বৃক্ষ_Pure(মিছা), nodep->width()};
            }();

            // Construct term containing the bits of the middle word - if needed
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ mTermp = [&]() -> Ast_গ্ৰন্থি_ৰাশিমালা* {
                যদি (!mNeeded) ঘূৰাই nullptr;

                Ast_গ্ৰন্থি_ৰাশিমালা* mWordp = wordSel(mIdxp);
                যদি (nodep->isQuad()) mWordp = নূতন AstCCast{nfl, mWordp, nodep};
                Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ mShiftp = নূতন AstSub{lfl, নূতন AstConst{lfl, VL_EDATASIZE},
                                                        lBitp->clone_বৃক্ষ_Pure(মিছা)};
                // If we're selecting bit zero, then all 32 bits in the mid word
                // get shifted << by 32 bits, so ignore them.
                ধ্রুৱ V3_সংখ্যা zero{nodep, longOrQuadWidth(nodep)};
                ঘূৰাই নূতন AstCond{
                    nfl,
                    // lsb % VL_EDATASIZE == 0 ?
                    নূতন AstEq{nfl, নূতন AstConst{nfl, 0}, lBitp->clone_বৃক্ষ_Pure(মিছা)},
                    // 0 :
                    নূতন AstConst{nfl, zero},
                    //  midword >> (VL_EDATASIZE - (lbs % VL_EDATASIZE))
                    নূতন AstShiftL{nfl, mWordp, mShiftp, nodep->width()}};
            }();

            // Construct term containing the bits of the high word - if needed
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ hTermp = [&]() -> Ast_গ্ৰন্থি_ৰাশিমালা* {
                যদি (!hNeeded) ঘূৰাই nullptr;

                Ast_গ্ৰন্থি_ৰাশিমালা* hWordp = wordSel(hIdxp);
                hWordp = নূতন AstCCast{nfl, hWordp, nodep};
                Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ hShiftp = নূতন AstCond{
                    nfl,
                    // lsb % VL_EDATASIZE == 0 ?
                    নূতন AstEq{nfl, নূতন AstConst{nfl, 0}, lBitp->clone_বৃক্ষ_Pure(মিছা)},
                    // VL_EDATASIZE :
                    নূতন AstConst{lfl, VL_EDATASIZE},
                    // 64 - (lbs % VL_EDATASIZE)
                    নূতন AstSub{lfl, নূতন AstConst{lfl, 64}, lBitp->clone_বৃক্ষ_Pure(মিছা)}};
                ঘূৰাই নূতন AstShiftL{nfl, hWordp, hShiftp, nodep->width()};
            }();

            // Delete parts not captured during construction
            VL_DO_DANGLING(lBitp->delete_বৃক্ষ(), lBitp);
            VL_DO_DANGLING(lIdxp->delete_বৃক্ষ(), lIdxp);
            যদি (mIdxp) VL_DO_DANGLING(mIdxp->delete_বৃক্ষ(), mIdxp);
            যদি (hIdxp) VL_DO_DANGLING(hIdxp->delete_বৃক্ষ(), hIdxp);

            // Or reduce the terms
            Ast_গ্ৰন্থি_ৰাশিমালা* newp = lTermp;
            যদি (mTermp) newp = নূতন AstOr{nfl, mTermp, newp};
            যদি (hTermp) newp = নূতন AstOr{nfl, hTermp, newp};
            newp->dtypeFrom(nodep);
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        } অন্যথা {  // Long/Quad from Long/Quad
            // No isImpure() check - can handle side effects in below
            didPattern(VExpandPattern::SEL_FROM_NARROW, nodep);
            File_সাৰি* ধ্রুৱ fl = nodep->fileline();
            Ast_গ্ৰন্থি_ৰাশিমালা* fromp = nodep->fromp()->unlinkFrBack();
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ lsbp = nodep->lsbp()->unlinkFrBack();
            যদি (nodep->isQuad() && !fromp->isQuad()) fromp = নূতন AstCCast{fl, fromp, nodep};
            // {large}>>32 requires 64-bit shift operation; then cast
            Ast_গ্ৰন্থি_ৰাশিমালা* newp = নূতন AstShiftR{fl, fromp, lsbp, fromp->width()};
            newp->dtypeFrom(fromp);
            যদি (!nodep->isQuad() && fromp->isQuad()) newp = নূতন AstCCast{fl, newp, nodep};
            newp->dtypeFrom(nodep);
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        }
    }

    তার্কিক expandWide(Ast_গ্ৰন্থি_Assign* nodep, AstSel* rhsp) {
        UASSERT_OBJ(nodep->widthMin() == rhsp->widthConst(), nodep, "Width mismatch");
        যদি (!doExpandWide(nodep)) ঘূৰাই মিছা;

        // Simplify the index, in case it becomes a constant
        V3Const::constifyEditCpp(rhsp->lsbp());

        // If it's a constant select and aligned, we can just copy the words
        যদি (ধ্রুৱ AstConst* ধ্রুৱ lsbConstp = VN_CAST(rhsp->lsbp(), Const)) {
            ধ্রুৱ uint32_t lsb = lsbConstp->toUInt();
            যদি (VL_BITBIT_E(lsb) == 0) {
                didPattern(VExpandPattern::ASSIGN_SEL_ALIGNED, nodep);
                ধ্রুৱ uint32_t word = VL_BITWORD_E(lsb);
                বাবে (পূর্ণাঙ্ক w = 0; w < nodep->width_শব্দগুলি(); ++w) {
                    addWordAssign(nodep, w, newAstWordSelClone(rhsp->fromp(), w + word));
                }
                ঘূৰাই সঁচা;
            }
        }

        didPattern(VExpandPattern::ASSIGN_SEL_UNALIGNED, nodep);
        File_সাৰি* ধ্রুৱ flp = rhsp->fileline();

        // Use fresh set of temporaries
        ++m_nTmps;

        // Compute word index of LSB, store to temporary if not constant
        Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ wordLsbp = rhsp->lsbp()->clone_বৃক্ষ_Pure(মিছা);
        Ast_গ্ৰন্থি_ৰাশিমালা* wordIdxp = newWordIndex(wordLsbp);
        যদি (!wordLsbp->backp()) VL_DO_DANGLING(wordLsbp->delete_বৃক্ষ(), wordLsbp);
        wordIdxp = V3Const::constifyEditCpp(wordIdxp);
        যদি (!VN_IS(wordIdxp, Const)) {
            AstVar* ধ্রুৱ tmpp = addLocalTmp(nodep, "ExpandSel_WordIdx", wordIdxp);
            wordIdxp = নূতন AstVarRef{flp, tmpp, VAccess::READ};
        }

        // Compute shift amounts and mask, store to temporaries if not constants
        Ast_গ্ৰন্থি_ৰাশিমালা* loShftp = nullptr;
        Ast_গ্ৰন্থি_ৰাশিমালা* hiShftp = nullptr;
        Ast_গ্ৰন্থি_ৰাশিমালা* hiMaskp = nullptr;
        যদি (ধ্রুৱ AstConst* ধ্রুৱ lsbConstp = VN_CAST(rhsp->lsbp(), Const)) {
            ধ্রুৱ uint32_t bitOffset = VL_BITBIT_E(lsbConstp->toUInt());
            // Must be unaligned, otherwise we would have handled it above
            UASSERT_OBJ(bitOffset, nodep, "Missed aligned wide select");
            loShftp = নূতন AstConst{flp, bitOffset};
            hiShftp = নূতন AstConst{flp, VL_EDATASIZE - bitOffset};
            hiMaskp = nullptr;
        } অন্যথা {
            // Compute Low word shift amount: bottom bits of lsb index
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ lsbp = rhsp->lsbp()->clone_বৃক্ষ_Pure(মিছা);
            loShftp = নূতন AstAnd{flp, নূতন AstConst{flp, VL_SIZEBITS_E}, lsbp};
            AstVar* ধ্রুৱ loTmpp = addLocalTmp(nodep, "ExpandSel_LoShift", loShftp);
            loShftp = নূতন AstVarRef{flp, loTmpp, VAccess::READ};

            // Compute if aligned: Low word shift amount is 0
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ zerop = নূতন AstConst{flp, 0};
            Ast_গ্ৰন্থি_ৰাশিমালা* alignedp = নূতন AstEq{flp, zerop, loShftp->clone_বৃক্ষ_Pure(মিছা)};
            AstVar* ধ্রুৱ alignedTmpp = addLocalTmp(nodep, "ExpandSel_Aligned", alignedp);
            alignedp = নূতন AstVarRef{flp, alignedTmpp, VAccess::READ};

            // Computed High word shift amount: 0 if aligned else VL_EDATASIZE - Low word shift
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ edsp = নূতন AstConst{flp, VL_EDATASIZE};
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ subp = নূতন AstSub{flp, edsp, loShftp->clone_বৃক্ষ_Pure(মিছা)};
            hiShftp = নূতন AstCond{flp, alignedp, নূতন AstConst{flp, 0}, subp};
            AstVar* ধ্রুৱ hiTmpp = addLocalTmp(nodep, "ExpandSel_HiShift", hiShftp);
            hiShftp = নূতন AstVarRef{flp, hiTmpp, VAccess::READ};

            // Compute the High word mask: 0 if aligned, ones otherwise
            hiMaskp = নূতন AstCond{flp, alignedp->clone_বৃক্ষ_Pure(মিছা),
                                  নূতন AstConst{flp, AstConst::SizedEData{}, 0},
                                  নূতন AstConst{flp, AstConst::SizedEData{}, -1ULL}};
            AstVar* ধ্রুৱ maskTmpp = addLocalTmp(nodep, "ExpandSel_HiMask", hiMaskp);
            hiMaskp = নূতন AstVarRef{flp, maskTmpp, VAccess::READ};
        }

        // Create each word of the selected result
        Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ fromp = rhsp->fromp();
        ধ্রুৱ পূর্ণাঙ্ক sel_শব্দগুলি = nodep->width_শব্দগুলি();
        বাবে (পূর্ণাঙ্ক w = 0; w < sel_শব্দগুলি; ++w) {
            // Grab bits from word 'VL_BITWORD_E(lsb) + w'
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ loWordp = newWordSelWord(fromp->fileline(), fromp, wordIdxp, w);
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ loShftClonep = loShftp->clone_বৃক্ষ_Pure(মিছা);
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ lop = নূতন AstShiftR{flp, loWordp, loShftClonep, VL_EDATASIZE};
            // Grab bits from word 'VL_BITWORD_E(lsb) + w + 1'
            Ast_গ্ৰন্থি_ৰাশিমালা* hiWordp = newWordSelWord(fromp->fileline(), fromp, wordIdxp, w + 1);
            // For the last word of the result, avoid an OOB access when the Sel is not OOB
            যদি (w == sel_শব্দগুলি - 1) {
                ধ্রুৱ uint32_t max = fromp->width_শব্দগুলি() - w - 1;
                Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ maxp = নূতন AstConst{flp, max};
                Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ condp = নূতন AstGte{flp, wordIdxp->clone_বৃক্ষ_Pure(মিছা), maxp};
                Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ zerop = নূতন AstConst{flp, AstConst::SizedEData{}, 0};
                hiWordp = নূতন AstCond{flp, condp, zerop, hiWordp};
            }
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ hiShftClonep = hiShftp->clone_বৃক্ষ_Pure(মিছা);
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ hiPartp = নূতন AstShiftL{flp, hiWordp, hiShftClonep, VL_EDATASIZE};
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ hip
                = hiMaskp ? নূতন AstAnd{flp, hiPartp, hiMaskp->clone_বৃক্ষ_Pure(মিছা)} : hiPartp;
            // Combine them
            addWordAssign(nodep, w, নূতন AstOr{nodep->fileline(), hip, lop});
        }

        // Delete parts not captured during construction
        যদি (!wordIdxp->backp()) VL_DO_DANGLING(wordIdxp->delete_বৃক্ষ(), wordIdxp);
        যদি (!loShftp->backp()) VL_DO_DANGLING(loShftp->delete_বৃক্ষ(), loShftp);
        যদি (!hiShftp->backp()) VL_DO_DANGLING(hiShftp->delete_বৃক্ষ(), hiShftp);
        যদি (hiMaskp && !hiMaskp->backp()) VL_DO_DANGLING(hiMaskp->delete_বৃক্ষ(), hiMaskp);

        ঘূৰাই সঁচা;
    }

    তার্কিক expandLhs(Ast_গ্ৰন্থি_Assign* nodep, AstSel* lhsp) {
        // Possibilities
        //      destp: wide or narrow
        //      rhsp:  wide (destp must be wide), narrow, or 1 bit wide
        //      rhsp:  may be allones and can remove AND NOT gate
        //      lsbp:  constant or variable
        // Yuk.
        যদি (isImpure(nodep)) ঘূৰাই মিছা;
        File_সাৰি* ধ্রুৱ nfl = nodep->fileline();
        File_সাৰি* ধ্রুৱ lfl = lhsp->fileline();
        ধ্রুৱ তার্কিক destwide = lhsp->fromp()->isWide();
        ধ্রুৱ তার্কিক ones = nodep->rhsp()->isAllOnesV();
        যদি (VN_IS(lhsp->lsbp(), Const)) {
            // The code should work without this constant test, but it won't
            // constify as nicely as we'd like.
            Ast_গ্ৰন্থি_ৰাশিমালা* rhsp = nodep->rhsp()->unlinkFrBack();
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ destp = lhsp->fromp()->unlinkFrBack();
            ধ্রুৱ পূর্ণাঙ্ক lsb = lhsp->lsbConst();
            ধ্রুৱ পূর্ণাঙ্ক msb = lhsp->msbConst();
            V3_সংখ্যা maskset{nodep, destp->widthMin()};
            maskset.setMask(msb + 1 - lsb, lsb);
            V3_সংখ্যা maskold{nodep, destp->widthMin()};
            maskold.opNot(maskset);
            যদি (destwide) {
                didPattern(VExpandPattern::ASSIGN_TO_SEL_CONST_WIDE, nodep);
                বাবে (পূর্ণাঙ্ক w = 0; w < destp->width_শব্দগুলি(); ++w) {
                    যদি (w >= VL_BITWORD_E(lsb) && w <= VL_BITWORD_E(msb)) {
                        // else we would just be setting it to the same exact value
                        Ast_গ্ৰন্থি_ৰাশিমালা* oldvalp = newAstWordSelClone(destp, w);
                        fixCloneLvalue(oldvalp);
                        যদি (!ones) {
                            oldvalp = নূতন AstAnd{
                                lfl,
                                নূতন AstConst{lfl, AstConst::SizedEData{}, maskold.edataWord(w)},
                                oldvalp};
                        }

                        // Appropriate word of new value to insert:
                        Ast_গ্ৰন্থি_ৰাশিমালা* newp = newWordGrabShift(lfl, w, rhsp, lsb);

                        // Apply cleaning at the top word of the destination
                        // (no cleaning to do if dst's width is a whole number
                        // of words).
                        যদি (w == destp->width_শব্দগুলি() - 1 && VL_BITBIT_E(destp->widthMin()) != 0) {
                            V3_সংখ্যা cleanmask{nodep, VL_EDATASIZE};
                            cleanmask.setMask(VL_BITBIT_E(destp->widthMin()));
                            newp = নূতন AstAnd{lfl, newp, নূতন AstConst{lfl, cleanmask}};
                        }
                        Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ orp
                            = V3Const::constifyEditCpp(নূতন AstOr{lfl, oldvalp, newp});
                        addWordAssign(nodep, w, destp, orp);
                    }
                }
                VL_DO_DANGLING(rhsp->delete_বৃক্ষ(), rhsp);
                VL_DO_DANGLING(destp->delete_বৃক্ষ(), destp);
            } অন্যথা {
                didPattern(VExpandPattern::ASSIGN_TO_SEL_CONST_NARROW, nodep);
                যদি (destp->isQuad() && !rhsp->isQuad()) rhsp = নূতন AstCCast{nfl, rhsp, nodep};
                Ast_গ্ৰন্থি_ৰাশিমালা* oldvalp = destp->clone_বৃক্ষ_Pure(সঁচা);
                fixCloneLvalue(oldvalp);
                যদি (!ones) oldvalp = নূতন AstAnd{lfl, নূতন AstConst{lfl, maskold}, oldvalp};

                // The bit-select can refer to bits outside the width of nodep
                // which we aren't allowed to assign to.  This is a mask of the
                // valid range of nodep which we apply to the new shifted RHS.
                V3_সংখ্যা cleanmask{nodep, destp->widthMin()};
                cleanmask.setMask(destp->widthMin());
                Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ shifted = নূতন AstShiftL{
                    lfl, rhsp, নূতন AstConst{lfl, static_cast<uint32_t>(lsb)}, destp->width()};
                Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ cleaned
                    = নূতন AstAnd{lfl, shifted, নূতন AstConst{lfl, cleanmask}};
                Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ orp
                    = V3Const::constifyEditCpp(নূতন AstOr{lfl, oldvalp, cleaned});
                ঢোকাও_Before(nodep, নূতন AstAssign{nfl, destp, orp});
            }
            ঘূৰাই সঁচা;
        } অন্যথা {  // non-const select offset
            যদি (destwide && lhsp->widthConst() == 1) {
                didPattern(VExpandPattern::ASSIGN_TO_SEL_VAR_WIDE_ONE_BIT, nodep);
                Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ rhsp = nodep->rhsp()->unlinkFrBack();
                Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ destp = lhsp->fromp()->unlinkFrBack();
                Ast_গ্ৰন্থি_ৰাশিমালা* oldvalp
                    = newWordSelBit(lfl, destp->clone_বৃক্ষ_Pure(মিছা), lhsp->lsbp());
                fixCloneLvalue(oldvalp);
                যদি (!ones) {
                    oldvalp = নূতন AstAnd{
                        lfl,
                        নূতন AstNot{
                            lfl, নূতন AstShiftL{lfl, নূতন AstConst{nfl, 1},
                                               // newSelBitBit may exceed the MSB of this variable.
                                               // That's ok as we'd just AND with a larger value,
                                               // but oldval would clip the upper bits to sanity
                                               newSelBitBit(lhsp->lsbp()), VL_EDATASIZE}},
                        oldvalp};
                }
                // Restrict the shift amount to 0-31, see bug804.
                Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ shiftp = নূতন AstAnd{nfl, lhsp->lsbp()->clone_বৃক্ষ_Pure(সঁচা),
                                                       নূতন AstConst{nfl, VL_EDATASIZE - 1}};
                Ast_গ্ৰন্থি* ধ্রুৱ newp = নূতন AstAssign{
                    nfl, newWordSelBit(nfl, destp, lhsp->lsbp()),
                    নূতন AstOr{lfl, oldvalp, নূতন AstShiftL{lfl, rhsp, shiftp, VL_EDATASIZE}}};
                ঢোকাও_Before(nodep, newp);
                ঘূৰাই সঁচা;
            } অন্যথা যদি (destwide) {
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
                ঘূৰাই মিছা;
            } অন্যথা {
                didPattern(VExpandPattern::ASSIGN_TO_SEL_VAR_NARROW, nodep);
                // nodep->dumpTree("-  old: ");
                Ast_গ্ৰন্থি_ৰাশিমালা* rhsp = nodep->rhsp()->unlinkFrBack();
                Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ destp = lhsp->fromp()->unlinkFrBack();
                Ast_গ্ৰন্থি_ৰাশিমালা* oldvalp = destp->clone_বৃক্ষ_Pure(সঁচা);
                fixCloneLvalue(oldvalp);

                V3_সংখ্যা maskwidth{nodep, destp->widthMin()};
                maskwidth.setMask(lhsp->widthConst());

                যদি (destp->isQuad() && !rhsp->isQuad()) rhsp = নূতন AstCCast{nfl, rhsp, nodep};
                যদি (!ones) {
                    oldvalp = নূতন AstAnd{
                        lfl,
                        নূতন AstNot{lfl, নূতন AstShiftL{lfl, নূতন AstConst{nfl, maskwidth},
                                                      lhsp->lsbp()->clone_বৃক্ষ_Pure(সঁচা),
                                                      destp->width()}},
                        oldvalp};
                }
                Ast_গ্ৰন্থি_ৰাশিমালা* newp
                    = নূতন AstShiftL{lfl, rhsp, lhsp->lsbp()->clone_বৃক্ষ_Pure(সঁচা), destp->width()};
                // Apply cleaning to the new value being inserted.  Mask is
                // slightly wider than necessary to avoid an AND with all ones
                // being optimized out.  No need to clean if destp is
                // quad-sized as there are no extra bits to contaminate
                যদি (destp->widthMin() != 64) {
                    V3_সংখ্যা cleanmask{nodep, destp->widthMin() + 1};
                    cleanmask.setMask(destp->widthMin());
                    newp = নূতন AstAnd{lfl, newp, নূতন AstConst{lfl, cleanmask}};
                }

                ঢোকাও_Before(nodep, নূতন AstAssign{nfl, destp, নূতন AstOr{lfl, oldvalp, newp}});
                ঘূৰাই সঁচা;
            }
        }
    }

    শূন্য visit(AstConcat* nodep) override {
        যদি (nodep->user1SetOnce()) ঘূৰাই;  // Process once
        iterateChildren(nodep);
        যদি (nodep->isWide()) {
            // See under ASSIGN(WIDE)
        } অন্যথা {
            // No isImpure() check - can handle side effects in below
            didPattern(VExpandPattern::CONCAT, nodep);
            File_সাৰি* ধ্রুৱ fl = nodep->fileline();
            Ast_গ্ৰন্থি_ৰাশিমালা* lhsp = nodep->lhsp()->unlinkFrBack();
            Ast_গ্ৰন্থি_ৰাশিমালা* rhsp = nodep->rhsp()->unlinkFrBack();
            ধ্রুৱ uint32_t rhsshift = rhsp->widthMin();
            যদি (nodep->isQuad() && !lhsp->isQuad()) lhsp = নূতন AstCCast{fl, lhsp, nodep};
            যদি (nodep->isQuad() && !rhsp->isQuad()) rhsp = নূতন AstCCast{fl, rhsp, nodep};
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ newp = নূতন AstOr{
                fl, নূতন AstShiftL{fl, lhsp, নূতন AstConst{fl, rhsshift}, nodep->width()}, rhsp};
            newp->dtypeFrom(nodep);  // Unsigned
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        }
    }
    তার্কিক expandWide(Ast_গ্ৰন্থি_Assign* nodep, AstConcat* rhsp) {
        যদি (!doExpandWide(rhsp)) ঘূৰাই মিছা;
        didPattern(VExpandPattern::ASSIGN_CONCAT, nodep);
        File_সাৰি* ধ্রুৱ fl = rhsp->fileline();
        // Lhs or Rhs may be word, long, or quad.
        // newAstWordSelClone nicely abstracts the difference.
        ধ্রুৱ পূর্ণাঙ্ক rhsshift = rhsp->rhsp()->widthMin();
        // Sometimes doing the words backwards is preferable.
        // When we have x={x,foo} backwards is better, when x={foo,x} forward is better
        // However V3Subst tends to rip this up, so not worth optimizing now.
        বাবে (পূর্ণাঙ্ক w = 0; w < rhsp->width_শব্দগুলি(); ++w) {
            addWordAssign(nodep, w,
                          নূতন AstOr{fl, newWordGrabShift(fl, w, rhsp->lhsp(), rhsshift),
                                    newAstWordSelClone(rhsp->rhsp(), w)});
        }
        ঘূৰাই সঁচা;
    }

    শূন্য visit(AstReplicate* nodep) override {
        যদি (nodep->user1SetOnce()) ঘূৰাই;  // Process once
        iterateChildren(nodep);
        যদি (nodep->isWide()) {
            // See under ASSIGN(WIDE)
        } অন্যথা {
            যদি (isImpure(nodep)) ঘূৰাই;
            File_সাৰি* ধ্রুৱ fl = nodep->fileline();
            Ast_গ্ৰন্থি_ৰাশিমালা* lhsp = nodep->srcp()->unlinkFrBack();
            Ast_গ্ৰন্থি_ৰাশিমালা* newp;
            ধ্রুৱ পূর্ণাঙ্ক lhswidth = lhsp->widthMin();
            যদি (lhswidth == 1) {
                didPattern(VExpandPattern::REPLICATE_ONE_BIT, nodep);
                newp = নূতন AstNegate{fl, lhsp};
            } অন্যথা {
                didPattern(VExpandPattern::REPLICATE, nodep);
                ধ্রুৱ AstConst* ধ্রুৱ constp = VN_AS(nodep->countp(), Const);
                UASSERT_OBJ(constp, nodep,
                            "Replication value isn't a constant.  Checked earlier!");
                ধ্রুৱ uint32_t times = constp->toUInt();
                যদি (nodep->isQuad() && !lhsp->isQuad()) lhsp = নূতন AstCCast{fl, lhsp, nodep};
                newp = lhsp->clone_বৃক্ষ_Pure(সঁচা);
                বাবে (unsigned repnum = 1; repnum < times; repnum++) {
                    ধ্রুৱ পূর্ণাঙ্ক rhsshift = repnum * lhswidth;
                    newp = নূতন AstOr{
                        fl,
                        নূতন AstShiftL{fl, lhsp->clone_বৃক্ষ_Pure(সঁচা),
                                      নূতন AstConst{fl, static_cast<uint32_t>(rhsshift)},
                                      nodep->width()},
                        newp};
                    newp->dtypeFrom(nodep);  // Unsigned
                }
                VL_DO_DANGLING(lhsp->delete_বৃক্ষ(), lhsp);  // Never used
            }
            newp->dtypeFrom(nodep);  // Unsigned
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        }
    }
    তার্কিক expandWide(Ast_গ্ৰন্থি_Assign* nodep, AstReplicate* rhsp) {
        যদি (!doExpandWide(rhsp)) ঘূৰাই মিছা;
        didPattern(VExpandPattern::ASSIGN_REPLICATE, nodep);
        File_সাৰি* ধ্রুৱ fl = nodep->fileline();
        Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ lhsp = rhsp->srcp();
        ধ্রুৱ পূর্ণাঙ্ক lhswidth = lhsp->widthMin();
        ধ্রুৱ AstConst* ধ্রুৱ constp = VN_AS(rhsp->countp(), Const);
        UASSERT_OBJ(constp, rhsp, "Replication value isn't a constant.  Checked earlier!");
        ধ্রুৱ uint32_t times = constp->toUInt();
        বাবে (পূর্ণাঙ্ক w = 0; w < rhsp->width_শব্দগুলি(); ++w) {
            Ast_গ্ৰন্থি_ৰাশিমালা* newp;
            যদি (lhswidth == 1) {
                newp = নূতন AstNegate{fl, lhsp->clone_বৃক্ষ_Pure(সঁচা)};
                // Replicate always unsigned
                newp->dtypeChgWidthSigned(VL_EDATASIZE, VL_EDATASIZE, VSigning::UNSIGNED);
            } অন্যথা {
                newp = newAstWordSelClone(lhsp, w);
                File_সাৰি* ধ্রুৱ rfl = rhsp->fileline();
                বাবে (unsigned repnum = 1; repnum < times; repnum++) {
                    newp = নূতন AstOr{fl, newWordGrabShift(rfl, w, lhsp, lhswidth * repnum), newp};
                }
            }
            addWordAssign(nodep, w, newp);
        }
        ঘূৰাই সঁচা;
    }

    শূন্য visitEqNeq(Ast_গ্ৰন্থি_Biop* nodep) {
        যদি (nodep->user1SetOnce()) ঘূৰাই;  // Process once
        iterateChildren(nodep);
        যদি (nodep->lhsp()->isWide()
            && !(VN_IS(nodep->lhsp()->dtypep()->skipRefp(), StreamDType)
                 || VN_IS(nodep->rhsp()->dtypep()->skipRefp(), StreamDType))) {
            যদি (isImpure(nodep)) ঘূৰাই;
            যদি (!doExpandWide(nodep->lhsp())) ঘূৰাই;
            যদি (!doExpandWide(nodep->rhsp())) ঘূৰাই;
            didPattern(VN_IS(nodep, Neq) ? VExpandPattern::WORDIZE_NEQ
                                         : VExpandPattern::WORDIZE_EQ,
                       nodep);
            // -> (0=={or{for each_word{WORDSEL(lhs,#)^WORDSEL(rhs,#)}}}
            File_সাৰি* ধ্রুৱ fl = nodep->fileline();
            Ast_গ্ৰন্থি_ৰাশিমালা* newp = nullptr;
            বাবে (পূর্ণাঙ্ক w = 0; w < nodep->lhsp()->width_শব্দগুলি(); ++w) {
                Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ eqp = নূতন AstXor{fl, newAstWordSelClone(nodep->lhsp(), w),
                                                    newAstWordSelClone(nodep->rhsp(), w)};
                newp = newp ? নূতন AstOr{fl, newp, eqp} : eqp;
            }
            যদি (VN_IS(nodep, Neq)) {
                newp = নূতন AstNeq{fl, নূতন AstConst{fl, AstConst::SizedEData{}, 0}, newp};
            } অন্যথা {
                newp = নূতন AstEq{fl, নূতন AstConst{fl, AstConst::SizedEData{}, 0}, newp};
            }
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        }
    }
    শূন্য visit(AstEq* nodep) override { visitEqNeq(nodep); }
    শূন্য visit(AstNeq* nodep) override { visitEqNeq(nodep); }

    শূন্য visit(AstRedOr* nodep) override {
        যদি (nodep->user1SetOnce()) ঘূৰাই;  // Process once
        iterateChildren(nodep);
        File_সাৰি* ধ্রুৱ fl = nodep->fileline();
        যদি (nodep->lhsp()->isWide()) {
            যদি (isImpure(nodep)) ঘূৰাই;
            যদি (!doExpandWide(nodep->lhsp())) ঘূৰাই;
            didPattern(VExpandPattern::WORDIZE_REDOR, nodep);
            // -> (0!={or{for each_word{WORDSEL(lhs,#)}}}
            Ast_গ্ৰন্থি_ৰাশিমালা* newp = nullptr;
            বাবে (পূর্ণাঙ্ক w = 0; w < nodep->lhsp()->width_শব্দগুলি(); ++w) {
                Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ eqp = newAstWordSelClone(nodep->lhsp(), w);
                newp = newp ? নূতন AstOr{fl, newp, eqp} : eqp;
            }
            newp = নূতন AstNeq{fl, নূতন AstConst{fl, AstConst::SizedEData{}, 0}, newp};
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        } অন্যথা {
            // No isImpure() check - can handle side effects in below
            didPattern(VExpandPattern::REDOR_TO_NEQ, nodep);
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ lhsp = nodep->lhsp()->unlinkFrBack();
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ newp = নূতন AstNeq{
                fl, নূতন AstConst{fl, AstConst::Widthed_মান{}, longOrQuadWidth(nodep), 0}, lhsp};
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        }
    }
    শূন্য visit(AstRedAnd* nodep) override {
        যদি (nodep->user1SetOnce()) ঘূৰাই;  // Process once
        iterateChildren(nodep);
        File_সাৰি* ধ্রুৱ fl = nodep->fileline();
        যদি (nodep->lhsp()->isWide()) {
            যদি (isImpure(nodep)) ঘূৰাই;
            যদি (!doExpandWide(nodep->lhsp())) ঘূৰাই;
            didPattern(VExpandPattern::WORDIZE_REDAND, nodep);
            // -> (0!={and{for each_word{WORDSEL(lhs,#)}}}
            Ast_গ্ৰন্থি_ৰাশিমালা* newp = nullptr;
            বাবে (পূর্ণাঙ্ক w = 0; w < nodep->lhsp()->width_শব্দগুলি(); ++w) {
                Ast_গ্ৰন্থি_ৰাশিমালা* eqp = newAstWordSelClone(nodep->lhsp(), w);
                যদি (w == nodep->lhsp()->width_শব্দগুলি() - 1) {
                    // Rather than doing a (slowish) ==##, we OR in the
                    // bits that aren't part of the mask
                    eqp = নূতন AstOr{fl, নূতন AstConst{fl, notWideMask(nodep->lhsp())},
                                    // Bug in cppcheck
                                    // cppcheck-suppress memleak
                                    eqp};
                }
                newp = newp ? নূতন AstAnd{fl, newp, eqp} : eqp;
            }
            newp = নূতন AstEq{fl, নূতন AstConst{fl, AstConst::SizedEData{}, VL_MASK_E(VL_EDATASIZE)},
                             newp};
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        } অন্যথা {
            // No isImpure() check - can handle side effects in below
            didPattern(VExpandPattern::REDAND_TO_EQ, nodep);
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ lhsp = nodep->lhsp()->unlinkFrBack();
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ newp = নূতন AstEq{fl, নূতন AstConst{fl, wordMask(lhsp)}, lhsp};
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        }
    }
    শূন্য visit(AstRedXor* nodep) override {
        যদি (nodep->user1SetOnce()) ঘূৰাই;  // Process once
        iterateChildren(nodep);
        যদি (nodep->lhsp()->isWide()) {
            যদি (isImpure(nodep)) ঘূৰাই;
            যদি (!doExpandWide(nodep->lhsp())) ঘূৰাই;
            didPattern(VExpandPattern::WORDIZE_REDXOR, nodep);
            // -> (0!={redxor{for each_word{XOR(WORDSEL(lhs,#))}}}
            File_সাৰি* ধ্রুৱ fl = nodep->fileline();
            Ast_গ্ৰন্থি_ৰাশিমালা* newp = nullptr;
            বাবে (পূর্ণাঙ্ক w = 0; w < nodep->lhsp()->width_শব্দগুলি(); ++w) {
                Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ eqp = newAstWordSelClone(nodep->lhsp(), w);
                newp = newp ? নূতন AstXor{fl, newp, eqp} : eqp;
            }
            newp = নূতন AstRedXor{fl, newp};
            UINFO(8, "    Wordize REDXORnew " << newp);
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        }
        // We don't reduce non-wide XORs, as its more efficient to use a temp register,
        // which the inlined function does nicely.
    }

    শূন্য visit(Ast_গ্ৰন্থি_Stmt* nodep) override {
        যদি (nodep->user1SetOnce()) ঘূৰাই;  // Process once
        VL_RESTORER(m_stmtp);
        m_stmtp = nodep;
        iterateChildren(nodep);
    }
    শূন্য visit(Ast_গ্ৰন্থি_Assign* nodep) override {
        যদি (nodep->user1SetOnce()) ঘূৰাই;  // Process once
        যদি (VN_IS(nodep->dtypep()->skipRefp(), UnpackArrayDType)) {
            ঘূৰাই;  // Skip for UnpackArrayDType
        }

        VL_RESTORER(m_stmtp);
        m_stmtp = nodep;
        iterateChildren(nodep);
        তার্কিক did = মিছা;
        যদি (nodep->isWide()  //
            && ((VN_IS(nodep->lhsp(), VarRef) || VN_IS(nodep->lhsp(), ArraySel)))
            && !AstVar::scVarRecurse(nodep->lhsp())  // Need special function for SC
            && !AstVar::scVarRecurse(nodep->rhsp())) {
            যদি (AstConst* ধ্রুৱ rhsp = VN_CAST(nodep->rhsp(), Const)) {
                did = expandWide(nodep, rhsp);
            } অন্যথা যদি (AstVarRef* ধ্রুৱ rhsp = VN_CAST(nodep->rhsp(), VarRef)) {
                did = expandWide(nodep, rhsp);
            } অন্যথা যদি (AstSel* ধ্রুৱ rhsp = VN_CAST(nodep->rhsp(), Sel)) {
                did = expandWide(nodep, rhsp);
            } অন্যথা যদি (AstArraySel* ধ্রুৱ rhsp = VN_CAST(nodep->rhsp(), ArraySel)) {
                did = expandWide(nodep, rhsp);
            } অন্যথা যদি (AstConcat* ধ্রুৱ rhsp = VN_CAST(nodep->rhsp(), Concat)) {
                did = expandWide(nodep, rhsp);
            } অন্যথা যদি (AstExtend* ধ্রুৱ rhsp = VN_CAST(nodep->rhsp(), Extend)) {
                did = expandWide(nodep, rhsp);
            } অন্যথা যদি (AstReplicate* ধ্রুৱ rhsp = VN_CAST(nodep->rhsp(), Replicate)) {
                did = expandWide(nodep, rhsp);
            } অন্যথা যদি (AstAnd* ধ্রুৱ rhsp = VN_CAST(nodep->rhsp(), And)) {
                did = expandWide(nodep, rhsp);
            } অন্যথা যদি (AstOr* ধ্রুৱ rhsp = VN_CAST(nodep->rhsp(), Or)) {
                did = expandWide(nodep, rhsp);
            } অন্যথা যদি (AstNot* ধ্রুৱ rhsp = VN_CAST(nodep->rhsp(), Not)) {
                did = expandWide(nodep, rhsp);
            } অন্যথা যদি (AstXor* ধ্রুৱ rhsp = VN_CAST(nodep->rhsp(), Xor)) {
                did = expandWide(nodep, rhsp);
            } অন্যথা যদি (AstShiftL* ধ্রুৱ rhsp = VN_CAST(nodep->rhsp(), ShiftL)) {
                did = expandWideShift(nodep, rhsp, /* isLeftShift: */ সঁচা);
            } অন্যথা যদি (AstShiftR* ধ্রুৱ rhsp = VN_CAST(nodep->rhsp(), ShiftR)) {
                did = expandWideShift(nodep, rhsp, /* isLeftShift: */ মিছা);
            } অন্যথা যদি (AstCond* ধ্রুৱ rhsp = VN_CAST(nodep->rhsp(), Cond)) {
                did = expandWide(nodep, rhsp);
            }
        } অন্যথা যদি (AstSel* ধ্রুৱ lhsp = VN_CAST(nodep->lhsp(), Sel)) {
            did = expandLhs(nodep, lhsp);
        }
        // Cleanup common code
        যদি (did) VL_DO_DANGLING(nodep->unlinkFrBack()->delete_বৃক্ষ(), nodep);
    }

    //--------------------
    শূন্য visit(AstVar*) override {}  // Don't hit varrefs under vars
    শূন্য visit(Ast_গ্ৰন্থি* nodep) override { iterateChildren(nodep); }

সার্ৱজনিক:
    // CONSTRUCTORS
    explicit ExpandVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~ExpandVisitor() override {
        V3Stats::addStat("Optimizations, Expand, expanded wides", m_statWides);
        V3Stats::addStat("Optimizations, Expand, expanded wide words", m_stat_Wide_শব্দগুলি);
        V3Stats::addStat("Optimizations, Expand, not expanded impure", m_statImpure);
        V3Stats::addStat("Optimizations, Expand, not expanded limit", m_statWideLimited);
        V3Stats::addStat("Optimizations, Expand, temporaries created", m_statTemps);
        ধ্রুৱ auto emitStat = [এতত্](VExpandPattern pattern) {
            std::string str{pattern.ascii()};
            std::transform(str.begin(), str.end(), str.begin(), [](unsigned ৱর্ণ c) {  //
                ঘূৰাই c == '_' ? ' ' : std::tolower(c);
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

শূন্য V3Expand::expandAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        ধ্রুৱ ExpandOkVisitor okVisitor{nodep};
        ExpandVisitor{nodep};
    }  // Destruct before checking
    V3Global::dump_Check_Global_বৃক্ষ("expand", 0, dump_বৃক্ষ_Either_Level() >= 3);
}
