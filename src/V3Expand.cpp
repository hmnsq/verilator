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

#অন্তর্ভাব "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#অন্তর্ভাব "V3Expand.h"

#অন্তর্ভাব "V3Const.h"
#অন্তর্ভাব "V3Stats.h"

#অন্তর্ভাব <algorithm>
#অন্তর্ভাব <array>

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

শ্রেণী VExpandPattern final {
সর্বজনীন:
    গণনা en : uint8_t {
#define EXPAND_PATTERN_ID(id) id,
        FOR_EACH_EXPAND_PATTERN(EXPAND_PATTERN_ID)
#undef EXPAND_PATTERN_ID
            _ENUM_END
    };
    গণনা en m_e;
    ধ্রুবক অক্ষর* ascii() ধ্রুবক {
        স্থির ধ্রুবক অক্ষর* ধ্রুবক names[] = {
#define EXPAND_PATTERN_NAME(id) #id,
            FOR_EACH_EXPAND_PATTERN(EXPAND_PATTERN_NAME)
#undef EXPAND_PATTERN_NAME
                "_ENUM_END"  //
        };
        ফেরত names[m_e];
    }

    // cppcheck-suppress noExplicitConstructor
    VExpandPattern(en _e)
        : m_e{_e} {}
    operator en() ধ্রুবক { ফেরত m_e; }
};

//######################################################################
// Find nodes with side effects, to mark as non-expandable

শ্রেণী ExpandOkVisitor final : সর্বজনীন VNVisitorConst {
    // NODE STATE
    //  AstNode::user2()        -> bool.  Is pure (along with all children)
    ধ্রুবক VNUser2InUse m_inuser2;

    // STATE - for current visit position (use VL_RESTORER)
    // Tracks similar to AstNode::isTreePureRecurse(), but avoid O(n^2)
    // False = pure, as nodes that ExpandVisitor inserts preserve pureness
    যৌক্তিক m_isImpure = সত্য;  // Currently pure

    শূন্য visit(Ast_গ্রন্থি* nodep) override {
        যৌক্তিক selfImpure = !nodep->isPure();
        {
            VL_RESTORER(m_isImpure);
            m_isImpure = মিথ্যা;
            iterateChildrenConst(nodep);
            selfImpure |= m_isImpure;
            nodep->user2(selfImpure);
        }
        m_isImpure |= selfImpure;
    }

সর্বজনীন:
    // CONSTRUCTORS
    explicit ExpandOkVisitor(AstNetlist* nodep) { iterateConst(nodep); }
    ~ExpandOkVisitor() = পূর্বনির্ধারিত;
};

//######################################################################
// Expand state, as a visitor of each AstNode

শ্রেণী ExpandVisitor final : সর্বজনীন VNVisitor {
    // NODE STATE
    //  AstNode::user1()        -> bool.  Processed
    //  AstNode::user2()        -> See ExpandOkVisitor
    ধ্রুবক VNUser1InUse m_inuser1;

    // STATE - for current visit position (use VL_RESTORER)
    Ast_গ্রন্থি* m_stmtp = nullptr;  // Current statement

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
    আকার_t m_nTmps = 0;  // Sequence numbers for temopraries

    // METHODS
    // Note the application of the given expansion pattern to 'nodep'
    শূন্য didPattern(VExpandPattern pattern, Ast_গ্রন্থি* nodep) {
        UINFO(8, "    " << pattern.ascii() << " " << nodep);
        ++m_statPatterns[pattern];
    }

    // Use state that ExpandOkVisitor calculated
    স্থির যৌক্তিক isImpure(Ast_গ্রন্থি* nodep) {
        ধ্রুবক যৌক্তিক impure = nodep->user2();
        যদি (impure) UINFO(9, "      impure " << nodep);
        ফেরত impure;
    }

    যৌক্তিক doExpandWide(Ast_গ্রন্থি* nodep) {
        যদি (isImpure(nodep)) {
            ++m_statImpure;
            ফেরত মিথ্যা;
        }
        যদি (nodep->width_শব্দগুলি() <= v3Global.opt.expandLimit()) {
            ++m_statWides;
            m_stat_Wide_শব্দগুলি += nodep->width_শব্দগুলি();
            ফেরত সত্য;
        } অন্যথা {
            ++m_statWideLimited;
            ফেরত মিথ্যা;
        }
    }

    স্থির পূর্ণসংখ্যা longOrQuadWidth(Ast_গ্রন্থি* nodep) {
        ফেরত (nodep->width() + (VL_EDATASIZE - 1)) & ~(VL_EDATASIZE - 1);
    }
    স্থির V3_সংখ্যা notWideMask(Ast_গ্রন্থি* nodep) {
        ফেরত V3_সংখ্যা{nodep, VL_EDATASIZE, ~VL_MASK_E(nodep->widthMin())};
    }
    স্থির V3_সংখ্যা wordMask(Ast_গ্রন্থি* nodep) {
        যদি (nodep->isWide()) {
            ফেরত V3_সংখ্যা{nodep, VL_EDATASIZE, VL_MASK_E(nodep->widthMin())};
        } অন্যথা {
            V3_সংখ্যা mask{nodep, longOrQuadWidth(nodep)};
            mask.setMask(nodep->widthMin());
            ফেরত mask;
        }
    }

    স্থির শূন্য ঢোকাও_Before(Ast_গ্রন্থি* placep, Ast_গ্রন্থি* newp) {
        newp->user1(1);  // Already processed, don't need to re-iterate
        placep->add_Here_This_As_পরবর্তী(newp);
    }
    স্থির শূন্য replaceWithDelete(Ast_গ্রন্থি* nodep, Ast_গ্রন্থি* newp) {
        newp->user1(1);  // Already processed, don't need to re-iterate
        যদি (newp->width() != nodep->width()) {
            UASSERT_OBJ(newp->widthMin() == nodep->widthMin(), nodep,
                        "Replacement width mismatch");
            newp->dtypeChgWidth(nodep->width(), nodep->widthMin());
        }
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
    }
    স্থির Ast_গ্রন্থি* newWordAssign(Ast_গ্রন্থি_Assign* placep, পূর্ণসংখ্যা word, Ast_গ্রন্থি_রাশিমালা* lhsp,
                                  Ast_গ্রন্থি_রাশিমালা* rhsp) {
        File_সারি* ধ্রুবক fl = placep->fileline();
        ফেরত নতুন AstAssign{fl,
                             নতুন AstWordSel{fl, lhsp->clone_বৃক্ষ_Pure(সত্য),
                                            নতুন AstConst{fl, static_cast<uint32_t>(word)}},
                             rhsp};
    }
    স্থির শূন্য addWordAssign(Ast_গ্রন্থি_Assign* placep, পূর্ণসংখ্যা word, Ast_গ্রন্থি_রাশিমালা* lhsp,
                              Ast_গ্রন্থি_রাশিমালা* rhsp) {
        ঢোকাও_Before(placep, newWordAssign(placep, word, lhsp, rhsp));
    }
    স্থির শূন্য addWordAssign(Ast_গ্রন্থি_Assign* placep, পূর্ণসংখ্যা word, Ast_গ্রন্থি_রাশিমালা* rhsp) {
        addWordAssign(placep, word, placep->lhsp(), rhsp);
    }

    AstVar* addLocalTmp(Ast_গ্রন্থি* placep, ধ্রুবক অক্ষর* namep, Ast_গ্রন্থি_রাশিমালা* valuep) {
        ধ্রুবক std::string name = "__V"s + namep + "_"s + std::to_string(m_nTmps);
        File_সারি* ধ্রুবক flp = placep->fileline();
        AstVar* ধ্রুবক tmpp = নতুন AstVar{flp, VVarType::STMTTEMP, name, valuep->dtypep()};
        tmpp->funcLocal(সত্য);
        tmpp->isInternal(সত্য);
        tmpp->noReset(সত্য);
        tmpp->substConstOnly(সত্য);
        m_funcp->addVarsp(tmpp);
        ঢোকাও_Before(placep, নতুন AstAssign{flp, নতুন AstVarRef{flp, tmpp, VAccess::WRITE}, valuep});
        ++m_statTemps;
        ফেরত tmpp;
    }

    স্থির শূন্য fixCloneLvalue(Ast_গ্রন্থি* nodep) {
        // In AstSel transforms, we call clone() on VarRefs that were lvalues,
        // but are now being used on the RHS of the assignment
        যদি (VN_IS(nodep, VarRef)) VN_AS(nodep, VarRef)->access(VAccess::READ);
        // Iterate
        যদি (Ast_গ্রন্থি* ধ্রুবক refp = nodep->op1p()) fixCloneLvalue(refp);
        যদি (Ast_গ্রন্থি* ধ্রুবক refp = nodep->op2p()) fixCloneLvalue(refp);
        যদি (Ast_গ্রন্থি* ধ্রুবক refp = nodep->op3p()) fixCloneLvalue(refp);
        যদি (Ast_গ্রন্থি* ধ্রুবক refp = nodep->op4p()) fixCloneLvalue(refp);
    }

    স্থির Ast_গ্রন্থি_রাশিমালা* newAstWordSelClone(Ast_গ্রন্থি_রাশিমালা* nodep, পূর্ণসংখ্যা word) {
        // Get the specified word number from a wide array
        // Or, if it's a long/quad, do appropriate conversion to wide
        // Concat may pass negative word numbers, that means it wants a zero
        File_সারি* ধ্রুবক fl = nodep->fileline();
        যদি (nodep->isWide() && word >= 0 && word < nodep->width_শব্দগুলি()) {
            ফেরত নতুন AstWordSel{fl, nodep->clone_বৃক্ষ_Pure(সত্য),
                                  নতুন AstConst{fl, static_cast<uint32_t>(word)}};
        } অন্যথা যদি (nodep->isQuad() && word == 0) {
            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক quadfromp = nodep->clone_বৃক্ষ_Pure(সত্য);
            quadfromp->dtypeSetBitUnsized(VL_QUADSIZE, quadfromp->widthMin(), VSigning::UNSIGNED);
            ফেরত নতুন AstCCast{fl, quadfromp, VL_EDATASIZE};
        } অন্যথা যদি (nodep->isQuad() && word == 1) {
            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক quadfromp = nodep->clone_বৃক্ষ_Pure(সত্য);
            quadfromp->dtypeSetBitUnsized(VL_QUADSIZE, quadfromp->widthMin(), VSigning::UNSIGNED);
            ফেরত নতুন AstCCast{
                fl, নতুন AstShiftR{fl, quadfromp, নতুন AstConst{fl, VL_EDATASIZE}, VL_EDATASIZE},
                VL_EDATASIZE};
        } অন্যথা যদি (!nodep->isWide() && !nodep->isQuad() && word == 0) {
            ফেরত nodep->clone_বৃক্ষ_Pure(সত্য);
        } অন্যথা {  // Out of bounds
            ফেরত নতুন AstConst{fl, 0};
        }
    }

    স্থির Ast_গ্রন্থি_রাশিমালা* newWordGrabShift(File_সারি* fl, পূর্ণসংখ্যা word, Ast_গ্রন্থি_রাশিমালা* lhsp, পূর্ণসংখ্যা shift) {
        // Extract the expression to grab the value for the specified word, if it's the shift
        // of shift bits from lhsp
        // Negative word numbers requested for lhs when it's "before" what we want.
        // We get a 0 then.
        ধ্রুবক পূর্ণসংখ্যা othword = word - shift / VL_EDATASIZE;
        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক lop = newAstWordSelClone(lhsp, othword);
        ধ্রুবক uint32_t loShift = VL_BITBIT_E(shift);
        যদি (!loShift) ফেরত lop;
        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক hip = newAstWordSelClone(lhsp, othword - 1);
        ধ্রুবক uint32_t hiShift = VL_EDATASIZE - loShift;  // Complement offset
        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক newp
            = নতুন AstOr{fl,  //
                        নতুন AstShiftR{fl, hip, নতুন AstConst{fl, hiShift}, VL_EDATASIZE},
                        নতুন AstShiftL{fl, lop, নতুন AstConst{fl, loShift}, VL_EDATASIZE}};
        ফেরত V3Const::constifyEditCpp(newp);
    }

    // Return expression indexing the word that contains 'lsbp' + the given word offset
    স্থির Ast_গ্রন্থি_রাশিমালা* new_Word_সূচক(Ast_গ্রন্থি_রাশিমালা* lsbp, uint32_t wordOffset = 0) {
        // This is indexing a WordSel, so a 32 bit constants are fine
        File_সারি* ধ্রুবক flp = lsbp->fileline();
        যদি (ধ্রুবক AstConst* constp = VN_CAST(lsbp, Const)) {
            ফেরত নতুন AstConst{flp, wordOffset + VL_BITWORD_E(constp->toUInt())};
        }

        যদি (lsbp->backp()) lsbp = lsbp->clone_বৃক্ষ_Pure(মিথ্যা);
        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক wwl2p = নতুন AstConst{flp, VL_EDATASIZE_LOG2};  // Word width log 2
        Ast_গ্রন্থি_রাশিমালা* indexp = নতুন AstShiftR{flp, lsbp, wwl2p, VL_EDATASIZE};
        যদি (wordOffset) indexp = নতুন AstAdd{flp, নতুন AstConst{flp, wordOffset}, indexp};
        ফেরত indexp;
    }

    // Return word of fromp that contains word indexp + the given word offset.
    স্থির Ast_গ্রন্থি_রাশিমালা* newWordSelWord(File_সারি* flp, Ast_গ্রন্থি_রাশিমালা* fromp, Ast_গ্রন্থি_রাশিমালা* indexp,
                                       uint32_t wordOffset = 0) {
        UASSERT_OBJ(fromp->isWide(), fromp, "Only need AstWordSel on wide from's");

        যদি (ধ্রুবক AstConst* ধ্রুবক constp = VN_CAST(indexp, Const)) {
            indexp = nullptr;
            wordOffset += constp->toUInt();
        }

        // e.g. "logic [95:0] var[0]; logic [0] sel; out = var[sel];"
        // Squash before C++ to avoid getting a C++ compiler warning
        // (even though code would be unreachable as presumably a
        // AstCond is protecting above this node.
        যদি (wordOffset >= static_cast<uint32_t>(fromp->width_শব্দগুলি())) {
            ফেরত নতুন AstConst{flp, AstConst::SizedEData{}, 0};
        }

        যদি (fromp->backp()) fromp = fromp->clone_বৃক্ষ_Pure(মিথ্যা);

        // If indexp was constant, just use it
        যদি (!indexp) ফেরত নতুন AstWordSel{flp, fromp, নতুন AstConst{flp, wordOffset}};

        // Otherwise compute at runtime
        যদি (indexp->backp()) indexp = indexp->clone_বৃক্ষ_Pure(মিথ্যা);
        যদি (wordOffset) indexp = নতুন AstAdd{flp, নতুন AstConst{flp, wordOffset}, indexp};
        ফেরত নতুন AstWordSel{flp, fromp, indexp};
    }

    // Return word of fromp that contains bit lsbp + the given word offset.
    স্থির Ast_গ্রন্থি_রাশিমালা* newWordSelBit(File_সারি* flp, Ast_গ্রন্থি_রাশিমালা* fromp, Ast_গ্রন্থি_রাশিমালা* lsbp,
                                      uint32_t wordOffset = 0) {
        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক indexp = new_Word_সূচক(lsbp, wordOffset);
        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক wordSelp = newWordSelWord(flp, fromp, indexp);
        যদি (!indexp->backp()) VL_DO_DANGLING(indexp->delete_বৃক্ষ(), indexp);
        ফেরত wordSelp;
    }

    স্থির Ast_গ্রন্থি_রাশিমালা* newSelBitBit(Ast_গ্রন্থি_রাশিমালা* lsbp) {
        // Return equation to get the VL_BITBIT of a constant or non-constant
        File_সারি* ধ্রুবক fl = lsbp->fileline();
        যদি (VN_IS(lsbp, Const)) {
            ফেরত নতুন AstConst{fl, VL_BITBIT_E(VN_AS(lsbp, Const)->toUInt())};
        } অন্যথা {
            ফেরত নতুন AstAnd{fl, নতুন AstConst{fl, VL_EDATASIZE - 1}, lsbp->clone_বৃক্ষ_Pure(সত্য)};
        }
    }

    //====================

    যৌক্তিক expandWide(Ast_গ্রন্থি_Assign* nodep, AstConst* rhsp) {
        যদি (!doExpandWide(nodep)) ফেরত মিথ্যা;
        didPattern(VExpandPattern::ASSIGN_CONST, nodep);
        // -> {for each_word{ ASSIGN(WORDSEL(wide,#),WORDSEL(CONST,#))}}
        যদি (rhsp->অঙ্ক().isFourState()) {
            rhsp->v3warn(E_UNSUPPORTED,  // LCOV_EXCL_LINE  // impossible?
                         "Unsupported: 4-state numbers in this context");
        }
        File_সারি* ধ্রুবক fl = nodep->fileline();
        জন্য (পূর্ণসংখ্যা w = 0; w < nodep->width_শব্দগুলি(); ++w) {
            addWordAssign(nodep, w,
                          নতুন AstConst{fl, AstConst::SizedEData{}, rhsp->অঙ্ক().edataWord(w)});
        }
        ফেরত সত্য;
    }
    //-------- Uniops
    যৌক্তিক expandWide(Ast_গ্রন্থি_Assign* nodep, AstVarRef* rhsp) {
        // Special case: do not expand assignment of constant pool variables.
        // V3Subst undestands these directly.
        যদি (rhsp->varp()->constPoolEntry()) ফেরত মিথ্যা;
        যদি (!doExpandWide(nodep)) ফেরত মিথ্যা;
        didPattern(VExpandPattern::ASSIGN_VARREF, nodep);
        জন্য (পূর্ণসংখ্যা w = 0; w < nodep->width_শব্দগুলি(); ++w) {
            addWordAssign(nodep, w, newAstWordSelClone(rhsp, w));
        }
        ফেরত সত্য;
    }
    যৌক্তিক expandWide(Ast_গ্রন্থি_Assign* nodep, AstArraySel* rhsp) {
        UASSERT_OBJ(!VN_IS(nodep->dtypep()->skipRefp(), UnpackArrayDType), nodep,
                    "ArraySel with unpacked arrays should have been removed in V3Slice");
        যদি (!doExpandWide(nodep)) ফেরত মিথ্যা;
        didPattern(VExpandPattern::ASSIGN_ARRAYSEL, nodep);
        জন্য (পূর্ণসংখ্যা w = 0; w < nodep->width_শব্দগুলি(); ++w) {
            addWordAssign(nodep, w, newAstWordSelClone(rhsp, w));
        }
        ফেরত সত্য;
    }
    যৌক্তিক expandWide(Ast_গ্রন্থি_Assign* nodep, AstNot* rhsp) {
        // -> {for each_word{ ASSIGN(WORDSEL(wide,#),NOT(WORDSEL(lhs,#))) }}
        যদি (!doExpandWide(nodep)) ফেরত মিথ্যা;
        didPattern(VExpandPattern::ASSIGN_NOT, nodep);
        File_সারি* ধ্রুবক fl = rhsp->fileline();
        জন্য (পূর্ণসংখ্যা w = 0; w < nodep->width_শব্দগুলি(); ++w) {
            addWordAssign(nodep, w, নতুন AstNot{fl, newAstWordSelClone(rhsp->lhsp(), w)});
        }
        ফেরত সত্য;
    }
    //-------- Biops
    যৌক্তিক expandWide(Ast_গ্রন্থি_Assign* nodep, AstAnd* rhsp) {
        যদি (!doExpandWide(nodep)) ফেরত মিথ্যা;
        didPattern(VExpandPattern::ASSIGN_AND, nodep);
        File_সারি* ধ্রুবক fl = nodep->fileline();
        জন্য (পূর্ণসংখ্যা w = 0; w < nodep->width_শব্দগুলি(); ++w) {
            addWordAssign(nodep, w,
                          নতুন AstAnd{fl, newAstWordSelClone(rhsp->lhsp(), w),
                                     newAstWordSelClone(rhsp->rhsp(), w)});
        }
        ফেরত সত্য;
    }
    যৌক্তিক expandWide(Ast_গ্রন্থি_Assign* nodep, AstOr* rhsp) {
        যদি (!doExpandWide(nodep)) ফেরত মিথ্যা;
        didPattern(VExpandPattern::ASSIGN_OR, nodep);
        File_সারি* ধ্রুবক fl = nodep->fileline();
        জন্য (পূর্ণসংখ্যা w = 0; w < nodep->width_শব্দগুলি(); ++w) {
            addWordAssign(nodep, w,
                          নতুন AstOr{fl, newAstWordSelClone(rhsp->lhsp(), w),
                                    newAstWordSelClone(rhsp->rhsp(), w)});
        }
        ফেরত সত্য;
    }
    যৌক্তিক expandWide(Ast_গ্রন্থি_Assign* nodep, AstXor* rhsp) {
        যদি (!doExpandWide(nodep)) ফেরত মিথ্যা;
        didPattern(VExpandPattern::ASSIGN_XOR, nodep);
        File_সারি* ধ্রুবক fl = nodep->fileline();
        জন্য (পূর্ণসংখ্যা w = 0; w < nodep->width_শব্দগুলি(); ++w) {
            addWordAssign(nodep, w,
                          নতুন AstXor{fl, newAstWordSelClone(rhsp->lhsp(), w),
                                     newAstWordSelClone(rhsp->rhsp(), w)});
        }
        ফেরত সত্য;
    }
    যৌক্তিক expandWideShift(Ast_গ্রন্থি_Assign* nodep, Ast_গ্রন্থি_Biop* rhsp, যৌক্তিক is_বাম_Shift) {
        যদি (!doExpandWide(nodep)) ফেরত মিথ্যা;

        // Simplify the shift amount, in case it becomes a constant
        V3Const::constifyEditCpp(rhsp->rhsp());

        // If it's a constant shift by whole words, expand it so V3Subst can substitute it
        যদি (ধ্রুবক AstConst* ধ্রুবক rhsConstp = VN_CAST(rhsp->rhsp(), Const)) {
            ধ্রুবক uint32_t shiftBits = rhsConstp->toUInt();
            যদি (VL_BITBIT_E(shiftBits) == 0) {
                ধ্রুবক পূর্ণসংখ্যা width_শব্দগুলি = nodep->width_শব্দগুলি();
                ধ্রুবক পূর্ণসংখ্যা shift_শব্দগুলি = std::min<পূর্ণসংখ্যা>(VL_BITWORD_E(shiftBits), width_শব্দগুলি);
                File_সারি* ধ্রুবক flp = rhsp->fileline();
                যদি (is_বাম_Shift) {
                    didPattern(VExpandPattern::ASSIGN_SHIFTL, nodep);
                    // Low words of the result are zero
                    জন্য (পূর্ণসংখ্যা w = 0; w < shift_শব্দগুলি; ++w) {
                        addWordAssign(nodep, w, নতুন AstConst{flp, AstConst::SizedEData{}, 0});
                    }
                    // High words of the result are copied from higher words of the source
                    জন্য (পূর্ণসংখ্যা w = shift_শব্দগুলি; w < width_শব্দগুলি; ++w) {
                        addWordAssign(nodep, w, newAstWordSelClone(rhsp->lhsp(), w - shift_শব্দগুলি));
                    }
                } অন্যথা {
                    didPattern(VExpandPattern::ASSIGN_SHIFTR, nodep);
                    // Low words of the result are copied from higher words of the source
                    জন্য (পূর্ণসংখ্যা w = 0; w < width_শব্দগুলি - shift_শব্দগুলি; ++w) {
                        addWordAssign(nodep, w, newAstWordSelClone(rhsp->lhsp(), w + shift_শব্দগুলি));
                    }
                    // High words of the result are zero
                    জন্য (পূর্ণসংখ্যা w = width_শব্দগুলি - shift_শব্দগুলি; w < width_শব্দগুলি; ++w) {
                        addWordAssign(nodep, w, নতুন AstConst{flp, AstConst::SizedEData{}, 0});
                    }
                }
                ফেরত সত্য;
            }
        }

        ফেরত মিথ্যা;
    }

    //-------- Triops
    যৌক্তিক expandWide(Ast_গ্রন্থি_Assign* nodep, AstCond* rhsp) {
        যদি (!doExpandWide(nodep)) ফেরত মিথ্যা;
        didPattern(VExpandPattern::ASSIGN_COND, nodep);
        File_সারি* ধ্রুবক fl = nodep->fileline();
        জন্য (পূর্ণসংখ্যা w = 0; w < nodep->width_শব্দগুলি(); ++w) {
            addWordAssign(nodep, w,
                          নতুন AstCond{fl, rhsp->condp()->clone_বৃক্ষ_Pure(সত্য),
                                      newAstWordSelClone(rhsp->thenp(), w),
                                      newAstWordSelClone(rhsp->elsep(), w)});
        }
        ফেরত সত্য;
    }

    // VISITORS
    শূন্য visit(AstNetlist* nodep) override { iterate_And_পরবর্তী_Null(nodep->modulesp()); }

    শূন্য visit(AstCFunc* nodep) override {
        VL_RESTORER(m_funcp);
        VL_RESTORER(m_nTmps);
        m_funcp = nodep;
        m_nTmps = 0;
        ধ্রুবক VDouble0 statWidesBefore = m_statWides;
        iterateChildren(nodep);

        // Constant fold here if anything was expanded, as Ast size can likely be reduced
        যদি (v3Global.opt.fConstEager() && m_statWides != statWidesBefore) {
            Ast_গ্রন্থি* ধ্রুবক editedp = V3Const::constifyEditCpp(nodep);
            UASSERT_OBJ(editedp == nodep, editedp, "Should not have replaced CFunc");
        }
    }

    শূন্য visit(AstExtend* nodep) override {
        যদি (nodep->user1SetOnce()) ফেরত;  // Process once
        iterateChildren(nodep);
        যদি (nodep->isWide()) {
            // See under ASSIGN(EXTEND)
        } অন্যথা {
            যদি (isImpure(nodep)) ফেরত;
            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক lhsp = nodep->lhsp()->unlinkFrBack();
            Ast_গ্রন্থি_রাশিমালা* newp = lhsp;
            যদি (nodep->isQuad()) {
                যদি (lhsp->isQuad()) {
                    lhsp->dtypeFrom(nodep);  // Just mark it, else nop
                } অন্যথা যদি (lhsp->isWide()) {
                    nodep->v3fatalSrc("extending larger thing into smaller?");
                } অন্যথা {
                    newp = নতুন AstCCast{nodep->fileline(), lhsp, nodep};
                }
            } অন্যথা {  // Long
                UASSERT_OBJ(!(lhsp->isQuad() || lhsp->isWide()), nodep,
                            "extending larger thing into smaller?");
                lhsp->dtypeFrom(nodep);  // Just mark it, else nop
            }
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        }
    }
    যৌক্তিক expandWide(Ast_গ্রন্থি_Assign* nodep, AstExtend* rhsp) {
        যদি (!doExpandWide(nodep)) ফেরত মিথ্যা;
        didPattern(VExpandPattern::ASSIGN_EXTEND, nodep);
        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক rlhsp = rhsp->lhsp();
        জন্য (পূর্ণসংখ্যা w = 0; w < rlhsp->width_শব্দগুলি(); ++w) {
            addWordAssign(nodep, w, newAstWordSelClone(rlhsp, w));
        }
        জন্য (পূর্ণসংখ্যা w = rlhsp->width_শব্দগুলি(); w < nodep->width_শব্দগুলি(); ++w) {
            addWordAssign(nodep, w, নতুন AstConst{nodep->fileline(), AstConst::SizedEData{}, 0});
        }
        ফেরত সত্য;
    }

    শূন্য visit(AstSel* nodep) override {
        যদি (nodep->user1SetOnce()) ফেরত;  // Process once
        iterateChildren(nodep);

        ধ্রুবক uint32_t width = static_cast<uint32_t>(nodep->widthConst());
        UASSERT_OBJ(nodep->widthMin() == static_cast<পূর্ণসংখ্যা>(width), nodep, "Width mismatch");

        // Skip if Sel is an LHS assignment select
        যদি (Ast_গ্রন্থি_Assign* ধ্রুবক assignp = VN_CAST(nodep->backp(), গ্রন্থি_Assign)) {
            যদি (nodep == assignp->lhsp()) ফেরত;
        }

        // Skip if wide: See under ASSIGN(WIDE)
        যদি (nodep->isWide()) ফেরত;

        // Skip Sel from stream or queue
        যদি (VN_IS(nodep->fromp()->dtypep(), StreamDType)) ফেরত;
        যদি (VN_IS(nodep->fromp()->dtypep(), QueueDType)) ফেরত;

        // Result must be non-wide after skipping all the above
        UASSERT_OBJ(width <= 64, nodep, "Inconsistent result width");

        যদি (nodep->fromp()->isWide()) {  // Long/Quad from Wide
            যদি (isImpure(nodep)) ফেরত;
            didPattern(VExpandPattern::SEL_FROM_WIDE, nodep);
            File_সারি* ধ্রুবক nfl = nodep->fileline();
            File_সারি* ধ্রুবক lfl = nodep->lsbp()->fileline();
            File_সারি* ধ্রুবক ffl = nodep->fromp()->fileline();
            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক fromp = nodep->fromp();

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
            ধ্রুবক auto tryFold = [&](Ast_গ্রন্থি_রাশিমালা* exprp) -> Ast_গ্রন্থি_রাশিমালা* {
                // Size limit for inlining expressions (any arbitrary value results in
                // a bounded expansion. Picked a value to allow common simple forms.)
                স্থির constexpr পূর্ণসংখ্যা EXPAND_SEL_LSB_LIMIT = 8;
                // Simplify
                exprp = V3Const::constifyEditCpp(exprp);
                // Accept if not larger than the limit
                যদি (!exprp->isLargerThan(EXPAND_SEL_LSB_LIMIT)) ফেরত exprp;
                // Delete if rejected
                VL_DO_DANGLING(exprp->delete_বৃক্ষ(), exprp);
                ফেরত nullptr;
            };

            // Return the index of the word holding the bit 'offset' bits above the LSB
            ধ্রুবক auto wordIdx = [&](uint32_t offset) -> Ast_গ্রন্থি_রাশিমালা* {
                Ast_গ্রন্থি_রাশিমালা* ধ্রুবক msbp = নতুন AstAdd{lfl, নতুন AstConst{lfl, offset},
                                                     nodep->lsbp()->clone_বৃক্ষ_Pure(মিথ্যা)};
                Ast_গ্রন্থি_রাশিমালা* ধ্রুবক idxp = new_Word_সূচক(msbp);
                যদি (!msbp->backp()) VL_DO_DANGLING(msbp->delete_বৃক্ষ(), msbp);
                ফেরত idxp;
            };

            // Bit index of the selected LSB within its word
            Ast_গ্রন্থি_রাশিমালা* lBitp = tryFold(newSelBitBit(nodep->lsbp()));
            ধ্রুবক যৌক্তিক aligned = lBitp && lBitp->isZero();

            // Which of the other 2 words the expansion might need
            ধ্রুবক যৌক্তিক mNeeded = width > 1 && !aligned;
            ধ্রুবক যৌক্তিক hNeeded = width > VL_EDATASIZE;
            UASSERT_OBJ(!hNeeded || nodep->isQuad(), nodep, "Width mismatch");

            // Word indices
            ধ্রুবক uint32_t mMsbOffset = std::min<uint32_t>(width, VL_EDATASIZE) - 1;
            ধ্রুবক uint32_t hMsbOffset = width - 1;
            Ast_গ্রন্থি_রাশিমালা* lIdxp = tryFold(new_Word_সূচক(nodep->lsbp()));
            Ast_গ্রন্থি_রাশিমালা* mIdxp = mNeeded ? tryFold(wordIdx(mMsbOffset)) : nullptr;
            Ast_গ্রন্থি_রাশিমালা* hIdxp = hNeeded ? tryFold(wordIdx(hMsbOffset)) : nullptr;

            // If the LSB expression is needed multiple times, evaluate it into a temporary
            ধ্রুবক পূর্ণসংখ্যা nCopies = !lBitp + !lIdxp + (mNeeded && !mIdxp) + (hNeeded && !hIdxp);
            যদি (nCopies > 1 && m_funcp && m_stmtp && !isImpure(m_stmtp)) {
                ++m_nTmps;  // Use fresh set of temporaries
                Ast_গ্রন্থি_রাশিমালা* ধ্রুবক lsbp = nodep->lsbp()->unlinkFrBack();
                AstVar* ধ্রুবক tmpp = addLocalTmp(m_stmtp, "ExpandSel_Lsb", lsbp);
                nodep->lsbp(নতুন AstVarRef{lfl, tmpp, VAccess::READ});
            }

            // Rebuild remaining expressions derived from LSB if needed, now using the temporary
            যদি (!lBitp) lBitp = newSelBitBit(nodep->lsbp());
            যদি (!lIdxp) lIdxp = new_Word_সূচক(nodep->lsbp());
            যদি (!mIdxp) mIdxp = mNeeded ? wordIdx(mMsbOffset) : nullptr;
            যদি (!hIdxp) hIdxp = hNeeded ? wordIdx(hMsbOffset) : nullptr;

            // Return word 'idxp' of 'fromp', without consuming 'idxp'
            ধ্রুবক auto wordSel = [&](Ast_গ্রন্থি_রাশিমালা* idxp) -> Ast_গ্রন্থি_রাশিমালা* {
                Ast_গ্রন্থি_রাশিমালা* ধ্রুবক clonep = idxp->clone_বৃক্ষ_Pure(মিথ্যা);
                Ast_গ্রন্থি_রাশিমালা* ধ্রুবক wordp = newWordSelWord(ffl, fromp, clonep);
                যদি (!clonep->backp()) VL_DO_DANGLING(clonep->delete_বৃক্ষ(), clonep);
                ফেরত wordp;
            };

            // Construct term containing the bits of the low word - always needed
            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক lTermp = [&]() -> Ast_গ্রন্থি_রাশিমালা* {
                Ast_গ্রন্থি_রাশিমালা* lWordp = wordSel(lIdxp);
                যদি (nodep->isQuad()) lWordp = নতুন AstCCast{nfl, lWordp, nodep};
                ফেরত নতুন AstShiftR{nfl, lWordp, lBitp->clone_বৃক্ষ_Pure(মিথ্যা), nodep->width()};
            }();

            // Construct term containing the bits of the middle word - if needed
            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক mTermp = [&]() -> Ast_গ্রন্থি_রাশিমালা* {
                যদি (!mNeeded) ফেরত nullptr;

                Ast_গ্রন্থি_রাশিমালা* mWordp = wordSel(mIdxp);
                যদি (nodep->isQuad()) mWordp = নতুন AstCCast{nfl, mWordp, nodep};
                Ast_গ্রন্থি_রাশিমালা* ধ্রুবক mShiftp = নতুন AstSub{lfl, নতুন AstConst{lfl, VL_EDATASIZE},
                                                        lBitp->clone_বৃক্ষ_Pure(মিথ্যা)};
                // If we're selecting bit zero, then all 32 bits in the mid word
                // get shifted << by 32 bits, so ignore them.
                ধ্রুবক V3_সংখ্যা zero{nodep, longOrQuadWidth(nodep)};
                ফেরত নতুন AstCond{
                    nfl,
                    // lsb % VL_EDATASIZE == 0 ?
                    নতুন AstEq{nfl, নতুন AstConst{nfl, 0}, lBitp->clone_বৃক্ষ_Pure(মিথ্যা)},
                    // 0 :
                    নতুন AstConst{nfl, zero},
                    //  midword >> (VL_EDATASIZE - (lbs % VL_EDATASIZE))
                    নতুন AstShiftL{nfl, mWordp, mShiftp, nodep->width()}};
            }();

            // Construct term containing the bits of the high word - if needed
            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক hTermp = [&]() -> Ast_গ্রন্থি_রাশিমালা* {
                যদি (!hNeeded) ফেরত nullptr;

                Ast_গ্রন্থি_রাশিমালা* hWordp = wordSel(hIdxp);
                hWordp = নতুন AstCCast{nfl, hWordp, nodep};
                Ast_গ্রন্থি_রাশিমালা* ধ্রুবক hShiftp = নতুন AstCond{
                    nfl,
                    // lsb % VL_EDATASIZE == 0 ?
                    নতুন AstEq{nfl, নতুন AstConst{nfl, 0}, lBitp->clone_বৃক্ষ_Pure(মিথ্যা)},
                    // VL_EDATASIZE :
                    নতুন AstConst{lfl, VL_EDATASIZE},
                    // 64 - (lbs % VL_EDATASIZE)
                    নতুন AstSub{lfl, নতুন AstConst{lfl, 64}, lBitp->clone_বৃক্ষ_Pure(মিথ্যা)}};
                ফেরত নতুন AstShiftL{nfl, hWordp, hShiftp, nodep->width()};
            }();

            // Delete parts not captured during construction
            VL_DO_DANGLING(lBitp->delete_বৃক্ষ(), lBitp);
            VL_DO_DANGLING(lIdxp->delete_বৃক্ষ(), lIdxp);
            যদি (mIdxp) VL_DO_DANGLING(mIdxp->delete_বৃক্ষ(), mIdxp);
            যদি (hIdxp) VL_DO_DANGLING(hIdxp->delete_বৃক্ষ(), hIdxp);

            // Or reduce the terms
            Ast_গ্রন্থি_রাশিমালা* newp = lTermp;
            যদি (mTermp) newp = নতুন AstOr{nfl, mTermp, newp};
            যদি (hTermp) newp = নতুন AstOr{nfl, hTermp, newp};
            newp->dtypeFrom(nodep);
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        } অন্যথা {  // Long/Quad from Long/Quad
            // No isImpure() check - can handle side effects in below
            didPattern(VExpandPattern::SEL_FROM_NARROW, nodep);
            File_সারি* ধ্রুবক fl = nodep->fileline();
            Ast_গ্রন্থি_রাশিমালা* fromp = nodep->fromp()->unlinkFrBack();
            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক lsbp = nodep->lsbp()->unlinkFrBack();
            যদি (nodep->isQuad() && !fromp->isQuad()) fromp = নতুন AstCCast{fl, fromp, nodep};
            // {large}>>32 requires 64-bit shift operation; then cast
            Ast_গ্রন্থি_রাশিমালা* newp = নতুন AstShiftR{fl, fromp, lsbp, fromp->width()};
            newp->dtypeFrom(fromp);
            যদি (!nodep->isQuad() && fromp->isQuad()) newp = নতুন AstCCast{fl, newp, nodep};
            newp->dtypeFrom(nodep);
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        }
    }

    যৌক্তিক expandWide(Ast_গ্রন্থি_Assign* nodep, AstSel* rhsp) {
        UASSERT_OBJ(nodep->widthMin() == rhsp->widthConst(), nodep, "Width mismatch");
        যদি (!doExpandWide(nodep)) ফেরত মিথ্যা;

        // Simplify the index, in case it becomes a constant
        V3Const::constifyEditCpp(rhsp->lsbp());

        // If it's a constant select and aligned, we can just copy the words
        যদি (ধ্রুবক AstConst* ধ্রুবক lsbConstp = VN_CAST(rhsp->lsbp(), Const)) {
            ধ্রুবক uint32_t lsb = lsbConstp->toUInt();
            যদি (VL_BITBIT_E(lsb) == 0) {
                didPattern(VExpandPattern::ASSIGN_SEL_ALIGNED, nodep);
                ধ্রুবক uint32_t word = VL_BITWORD_E(lsb);
                জন্য (পূর্ণসংখ্যা w = 0; w < nodep->width_শব্দগুলি(); ++w) {
                    addWordAssign(nodep, w, newAstWordSelClone(rhsp->fromp(), w + word));
                }
                ফেরত সত্য;
            }
        }

        didPattern(VExpandPattern::ASSIGN_SEL_UNALIGNED, nodep);
        File_সারি* ধ্রুবক flp = rhsp->fileline();

        // Use fresh set of temporaries
        ++m_nTmps;

        // Compute word index of LSB, store to temporary if not constant
        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক wordLsbp = rhsp->lsbp()->clone_বৃক্ষ_Pure(মিথ্যা);
        Ast_গ্রন্থি_রাশিমালা* wordIdxp = new_Word_সূচক(wordLsbp);
        যদি (!wordLsbp->backp()) VL_DO_DANGLING(wordLsbp->delete_বৃক্ষ(), wordLsbp);
        wordIdxp = V3Const::constifyEditCpp(wordIdxp);
        যদি (!VN_IS(wordIdxp, Const)) {
            AstVar* ধ্রুবক tmpp = addLocalTmp(nodep, "ExpandSel_WordIdx", wordIdxp);
            wordIdxp = নতুন AstVarRef{flp, tmpp, VAccess::READ};
        }

        // Compute shift amounts and mask, store to temporaries if not constants
        Ast_গ্রন্থি_রাশিমালা* loShftp = nullptr;
        Ast_গ্রন্থি_রাশিমালা* hiShftp = nullptr;
        Ast_গ্রন্থি_রাশিমালা* hiMaskp = nullptr;
        যদি (ধ্রুবক AstConst* ধ্রুবক lsbConstp = VN_CAST(rhsp->lsbp(), Const)) {
            ধ্রুবক uint32_t bitOffset = VL_BITBIT_E(lsbConstp->toUInt());
            // Must be unaligned, otherwise we would have handled it above
            UASSERT_OBJ(bitOffset, nodep, "Missed aligned wide select");
            loShftp = নতুন AstConst{flp, bitOffset};
            hiShftp = নতুন AstConst{flp, VL_EDATASIZE - bitOffset};
            hiMaskp = nullptr;
        } অন্যথা {
            // Compute Low word shift amount: bottom bits of lsb index
            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক lsbp = rhsp->lsbp()->clone_বৃক্ষ_Pure(মিথ্যা);
            loShftp = নতুন AstAnd{flp, নতুন AstConst{flp, VL_SIZEBITS_E}, lsbp};
            AstVar* ধ্রুবক loTmpp = addLocalTmp(nodep, "ExpandSel_LoShift", loShftp);
            loShftp = নতুন AstVarRef{flp, loTmpp, VAccess::READ};

            // Compute if aligned: Low word shift amount is 0
            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক zerop = নতুন AstConst{flp, 0};
            Ast_গ্রন্থি_রাশিমালা* alignedp = নতুন AstEq{flp, zerop, loShftp->clone_বৃক্ষ_Pure(মিথ্যা)};
            AstVar* ধ্রুবক alignedTmpp = addLocalTmp(nodep, "ExpandSel_Aligned", alignedp);
            alignedp = নতুন AstVarRef{flp, alignedTmpp, VAccess::READ};

            // Computed High word shift amount: 0 if aligned else VL_EDATASIZE - Low word shift
            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক edsp = নতুন AstConst{flp, VL_EDATASIZE};
            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক subp = নতুন AstSub{flp, edsp, loShftp->clone_বৃক্ষ_Pure(মিথ্যা)};
            hiShftp = নতুন AstCond{flp, alignedp, নতুন AstConst{flp, 0}, subp};
            AstVar* ধ্রুবক hiTmpp = addLocalTmp(nodep, "ExpandSel_HiShift", hiShftp);
            hiShftp = নতুন AstVarRef{flp, hiTmpp, VAccess::READ};

            // Compute the High word mask: 0 if aligned, ones otherwise
            hiMaskp = নতুন AstCond{flp, alignedp->clone_বৃক্ষ_Pure(মিথ্যা),
                                  নতুন AstConst{flp, AstConst::SizedEData{}, 0},
                                  নতুন AstConst{flp, AstConst::SizedEData{}, -1ULL}};
            AstVar* ধ্রুবক maskTmpp = addLocalTmp(nodep, "ExpandSel_HiMask", hiMaskp);
            hiMaskp = নতুন AstVarRef{flp, maskTmpp, VAccess::READ};
        }

        // Create each word of the selected result
        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক fromp = rhsp->fromp();
        ধ্রুবক পূর্ণসংখ্যা sel_শব্দগুলি = nodep->width_শব্দগুলি();
        জন্য (পূর্ণসংখ্যা w = 0; w < sel_শব্দগুলি; ++w) {
            // Grab bits from word 'VL_BITWORD_E(lsb) + w'
            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক loWordp = newWordSelWord(fromp->fileline(), fromp, wordIdxp, w);
            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক loShftClonep = loShftp->clone_বৃক্ষ_Pure(মিথ্যা);
            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক lop = নতুন AstShiftR{flp, loWordp, loShftClonep, VL_EDATASIZE};
            // Grab bits from word 'VL_BITWORD_E(lsb) + w + 1'
            Ast_গ্রন্থি_রাশিমালা* hiWordp = newWordSelWord(fromp->fileline(), fromp, wordIdxp, w + 1);
            // For the last word of the result, avoid an OOB access when the Sel is not OOB
            যদি (w == sel_শব্দগুলি - 1) {
                ধ্রুবক uint32_t max = fromp->width_শব্দগুলি() - w - 1;
                Ast_গ্রন্থি_রাশিমালা* ধ্রুবক maxp = নতুন AstConst{flp, max};
                Ast_গ্রন্থি_রাশিমালা* ধ্রুবক condp = নতুন AstGte{flp, wordIdxp->clone_বৃক্ষ_Pure(মিথ্যা), maxp};
                Ast_গ্রন্থি_রাশিমালা* ধ্রুবক zerop = নতুন AstConst{flp, AstConst::SizedEData{}, 0};
                hiWordp = নতুন AstCond{flp, condp, zerop, hiWordp};
            }
            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক hiShftClonep = hiShftp->clone_বৃক্ষ_Pure(মিথ্যা);
            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক hiPartp = নতুন AstShiftL{flp, hiWordp, hiShftClonep, VL_EDATASIZE};
            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক hip
                = hiMaskp ? নতুন AstAnd{flp, hiPartp, hiMaskp->clone_বৃক্ষ_Pure(মিথ্যা)} : hiPartp;
            // Combine them
            addWordAssign(nodep, w, নতুন AstOr{nodep->fileline(), hip, lop});
        }

        // Delete parts not captured during construction
        যদি (!wordIdxp->backp()) VL_DO_DANGLING(wordIdxp->delete_বৃক্ষ(), wordIdxp);
        যদি (!loShftp->backp()) VL_DO_DANGLING(loShftp->delete_বৃক্ষ(), loShftp);
        যদি (!hiShftp->backp()) VL_DO_DANGLING(hiShftp->delete_বৃক্ষ(), hiShftp);
        যদি (hiMaskp && !hiMaskp->backp()) VL_DO_DANGLING(hiMaskp->delete_বৃক্ষ(), hiMaskp);

        ফেরত সত্য;
    }

    যৌক্তিক expandLhs(Ast_গ্রন্থি_Assign* nodep, AstSel* lhsp) {
        // Possibilities
        //      destp: wide or narrow
        //      rhsp:  wide (destp must be wide), narrow, or 1 bit wide
        //      rhsp:  may be allones and can remove AND NOT gate
        //      lsbp:  constant or variable
        // Yuk.
        যদি (isImpure(nodep)) ফেরত মিথ্যা;
        File_সারি* ধ্রুবক nfl = nodep->fileline();
        File_সারি* ধ্রুবক lfl = lhsp->fileline();
        ধ্রুবক যৌক্তিক destwide = lhsp->fromp()->isWide();
        ধ্রুবক যৌক্তিক ones = nodep->rhsp()->isAllOnesV();
        যদি (VN_IS(lhsp->lsbp(), Const)) {
            // The code should work without this constant test, but it won't
            // constify as nicely as we'd like.
            Ast_গ্রন্থি_রাশিমালা* rhsp = nodep->rhsp()->unlinkFrBack();
            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক destp = lhsp->fromp()->unlinkFrBack();
            ধ্রুবক পূর্ণসংখ্যা lsb = lhsp->lsbConst();
            ধ্রুবক পূর্ণসংখ্যা msb = lhsp->msbConst();
            V3_সংখ্যা maskset{nodep, destp->widthMin()};
            maskset.setMask(msb + 1 - lsb, lsb);
            V3_সংখ্যা maskold{nodep, destp->widthMin()};
            maskold.opNot(maskset);
            যদি (destwide) {
                didPattern(VExpandPattern::ASSIGN_TO_SEL_CONST_WIDE, nodep);
                জন্য (পূর্ণসংখ্যা w = 0; w < destp->width_শব্দগুলি(); ++w) {
                    যদি (w >= VL_BITWORD_E(lsb) && w <= VL_BITWORD_E(msb)) {
                        // else we would just be setting it to the same exact value
                        Ast_গ্রন্থি_রাশিমালা* oldvalp = newAstWordSelClone(destp, w);
                        fixCloneLvalue(oldvalp);
                        যদি (!ones) {
                            oldvalp = নতুন AstAnd{
                                lfl,
                                নতুন AstConst{lfl, AstConst::SizedEData{}, maskold.edataWord(w)},
                                oldvalp};
                        }

                        // Appropriate word of new value to insert:
                        Ast_গ্রন্থি_রাশিমালা* newp = newWordGrabShift(lfl, w, rhsp, lsb);

                        // Apply cleaning at the top word of the destination
                        // (no cleaning to do if dst's width is a whole number
                        // of words).
                        যদি (w == destp->width_শব্দগুলি() - 1 && VL_BITBIT_E(destp->widthMin()) != 0) {
                            V3_সংখ্যা cleanmask{nodep, VL_EDATASIZE};
                            cleanmask.setMask(VL_BITBIT_E(destp->widthMin()));
                            newp = নতুন AstAnd{lfl, newp, নতুন AstConst{lfl, cleanmask}};
                        }
                        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক orp
                            = V3Const::constifyEditCpp(নতুন AstOr{lfl, oldvalp, newp});
                        addWordAssign(nodep, w, destp, orp);
                    }
                }
                VL_DO_DANGLING(rhsp->delete_বৃক্ষ(), rhsp);
                VL_DO_DANGLING(destp->delete_বৃক্ষ(), destp);
            } অন্যথা {
                didPattern(VExpandPattern::ASSIGN_TO_SEL_CONST_NARROW, nodep);
                যদি (destp->isQuad() && !rhsp->isQuad()) rhsp = নতুন AstCCast{nfl, rhsp, nodep};
                Ast_গ্রন্থি_রাশিমালা* oldvalp = destp->clone_বৃক্ষ_Pure(সত্য);
                fixCloneLvalue(oldvalp);
                যদি (!ones) oldvalp = নতুন AstAnd{lfl, নতুন AstConst{lfl, maskold}, oldvalp};

                // The bit-select can refer to bits outside the width of nodep
                // which we aren't allowed to assign to.  This is a mask of the
                // valid range of nodep which we apply to the new shifted RHS.
                V3_সংখ্যা cleanmask{nodep, destp->widthMin()};
                cleanmask.setMask(destp->widthMin());
                Ast_গ্রন্থি_রাশিমালা* ধ্রুবক shifted = নতুন AstShiftL{
                    lfl, rhsp, নতুন AstConst{lfl, static_cast<uint32_t>(lsb)}, destp->width()};
                Ast_গ্রন্থি_রাশিমালা* ধ্রুবক cleaned
                    = নতুন AstAnd{lfl, shifted, নতুন AstConst{lfl, cleanmask}};
                Ast_গ্রন্থি_রাশিমালা* ধ্রুবক orp
                    = V3Const::constifyEditCpp(নতুন AstOr{lfl, oldvalp, cleaned});
                ঢোকাও_Before(nodep, নতুন AstAssign{nfl, destp, orp});
            }
            ফেরত সত্য;
        } অন্যথা {  // non-const select offset
            যদি (destwide && lhsp->widthConst() == 1) {
                didPattern(VExpandPattern::ASSIGN_TO_SEL_VAR_WIDE_ONE_BIT, nodep);
                Ast_গ্রন্থি_রাশিমালা* ধ্রুবক rhsp = nodep->rhsp()->unlinkFrBack();
                Ast_গ্রন্থি_রাশিমালা* ধ্রুবক destp = lhsp->fromp()->unlinkFrBack();
                Ast_গ্রন্থি_রাশিমালা* oldvalp
                    = newWordSelBit(lfl, destp->clone_বৃক্ষ_Pure(মিথ্যা), lhsp->lsbp());
                fixCloneLvalue(oldvalp);
                যদি (!ones) {
                    oldvalp = নতুন AstAnd{
                        lfl,
                        নতুন AstNot{
                            lfl, নতুন AstShiftL{lfl, নতুন AstConst{nfl, 1},
                                               // newSelBitBit may exceed the MSB of this variable.
                                               // That's ok as we'd just AND with a larger value,
                                               // but oldval would clip the upper bits to sanity
                                               newSelBitBit(lhsp->lsbp()), VL_EDATASIZE}},
                        oldvalp};
                }
                // Restrict the shift amount to 0-31, see bug804.
                Ast_গ্রন্থি_রাশিমালা* ধ্রুবক shiftp = নতুন AstAnd{nfl, lhsp->lsbp()->clone_বৃক্ষ_Pure(সত্য),
                                                       নতুন AstConst{nfl, VL_EDATASIZE - 1}};
                Ast_গ্রন্থি* ধ্রুবক newp = নতুন AstAssign{
                    nfl, newWordSelBit(nfl, destp, lhsp->lsbp()),
                    নতুন AstOr{lfl, oldvalp, নতুন AstShiftL{lfl, rhsp, shiftp, VL_EDATASIZE}}};
                ঢোকাও_Before(nodep, newp);
                ফেরত সত্য;
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
                ফেরত মিথ্যা;
            } অন্যথা {
                didPattern(VExpandPattern::ASSIGN_TO_SEL_VAR_NARROW, nodep);
                // nodep->dumpTree("-  old: ");
                Ast_গ্রন্থি_রাশিমালা* rhsp = nodep->rhsp()->unlinkFrBack();
                Ast_গ্রন্থি_রাশিমালা* ধ্রুবক destp = lhsp->fromp()->unlinkFrBack();
                Ast_গ্রন্থি_রাশিমালা* oldvalp = destp->clone_বৃক্ষ_Pure(সত্য);
                fixCloneLvalue(oldvalp);

                V3_সংখ্যা maskwidth{nodep, destp->widthMin()};
                maskwidth.setMask(lhsp->widthConst());

                যদি (destp->isQuad() && !rhsp->isQuad()) rhsp = নতুন AstCCast{nfl, rhsp, nodep};
                যদি (!ones) {
                    oldvalp = নতুন AstAnd{
                        lfl,
                        নতুন AstNot{lfl, নতুন AstShiftL{lfl, নতুন AstConst{nfl, maskwidth},
                                                      lhsp->lsbp()->clone_বৃক্ষ_Pure(সত্য),
                                                      destp->width()}},
                        oldvalp};
                }
                Ast_গ্রন্থি_রাশিমালা* newp
                    = নতুন AstShiftL{lfl, rhsp, lhsp->lsbp()->clone_বৃক্ষ_Pure(সত্য), destp->width()};
                // Apply cleaning to the new value being inserted.  Mask is
                // slightly wider than necessary to avoid an AND with all ones
                // being optimized out.  No need to clean if destp is
                // quad-sized as there are no extra bits to contaminate
                যদি (destp->widthMin() != 64) {
                    V3_সংখ্যা cleanmask{nodep, destp->widthMin() + 1};
                    cleanmask.setMask(destp->widthMin());
                    newp = নতুন AstAnd{lfl, newp, নতুন AstConst{lfl, cleanmask}};
                }

                ঢোকাও_Before(nodep, নতুন AstAssign{nfl, destp, নতুন AstOr{lfl, oldvalp, newp}});
                ফেরত সত্য;
            }
        }
    }

    শূন্য visit(AstConcat* nodep) override {
        যদি (nodep->user1SetOnce()) ফেরত;  // Process once
        iterateChildren(nodep);
        যদি (nodep->isWide()) {
            // See under ASSIGN(WIDE)
        } অন্যথা {
            // No isImpure() check - can handle side effects in below
            didPattern(VExpandPattern::CONCAT, nodep);
            File_সারি* ধ্রুবক fl = nodep->fileline();
            Ast_গ্রন্থি_রাশিমালা* lhsp = nodep->lhsp()->unlinkFrBack();
            Ast_গ্রন্থি_রাশিমালা* rhsp = nodep->rhsp()->unlinkFrBack();
            ধ্রুবক uint32_t rhsshift = rhsp->widthMin();
            যদি (nodep->isQuad() && !lhsp->isQuad()) lhsp = নতুন AstCCast{fl, lhsp, nodep};
            যদি (nodep->isQuad() && !rhsp->isQuad()) rhsp = নতুন AstCCast{fl, rhsp, nodep};
            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক newp = নতুন AstOr{
                fl, নতুন AstShiftL{fl, lhsp, নতুন AstConst{fl, rhsshift}, nodep->width()}, rhsp};
            newp->dtypeFrom(nodep);  // Unsigned
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        }
    }
    যৌক্তিক expandWide(Ast_গ্রন্থি_Assign* nodep, AstConcat* rhsp) {
        যদি (!doExpandWide(rhsp)) ফেরত মিথ্যা;
        didPattern(VExpandPattern::ASSIGN_CONCAT, nodep);
        File_সারি* ধ্রুবক fl = rhsp->fileline();
        // Lhs or Rhs may be word, long, or quad.
        // newAstWordSelClone nicely abstracts the difference.
        ধ্রুবক পূর্ণসংখ্যা rhsshift = rhsp->rhsp()->widthMin();
        // Sometimes doing the words backwards is preferable.
        // When we have x={x,foo} backwards is better, when x={foo,x} forward is better
        // However V3Subst tends to rip this up, so not worth optimizing now.
        জন্য (পূর্ণসংখ্যা w = 0; w < rhsp->width_শব্দগুলি(); ++w) {
            addWordAssign(nodep, w,
                          নতুন AstOr{fl, newWordGrabShift(fl, w, rhsp->lhsp(), rhsshift),
                                    newAstWordSelClone(rhsp->rhsp(), w)});
        }
        ফেরত সত্য;
    }

    শূন্য visit(AstReplicate* nodep) override {
        যদি (nodep->user1SetOnce()) ফেরত;  // Process once
        iterateChildren(nodep);
        যদি (nodep->isWide()) {
            // See under ASSIGN(WIDE)
        } অন্যথা {
            যদি (isImpure(nodep)) ফেরত;
            File_সারি* ধ্রুবক fl = nodep->fileline();
            Ast_গ্রন্থি_রাশিমালা* lhsp = nodep->srcp()->unlinkFrBack();
            Ast_গ্রন্থি_রাশিমালা* newp;
            ধ্রুবক পূর্ণসংখ্যা lhswidth = lhsp->widthMin();
            যদি (lhswidth == 1) {
                didPattern(VExpandPattern::REPLICATE_ONE_BIT, nodep);
                newp = নতুন AstNegate{fl, lhsp};
            } অন্যথা {
                didPattern(VExpandPattern::REPLICATE, nodep);
                ধ্রুবক AstConst* ধ্রুবক constp = VN_AS(nodep->countp(), Const);
                UASSERT_OBJ(constp, nodep,
                            "Replication value isn't a constant.  Checked earlier!");
                ধ্রুবক uint32_t times = constp->toUInt();
                যদি (nodep->isQuad() && !lhsp->isQuad()) lhsp = নতুন AstCCast{fl, lhsp, nodep};
                newp = lhsp->clone_বৃক্ষ_Pure(সত্য);
                জন্য (unsigned repnum = 1; repnum < times; repnum++) {
                    ধ্রুবক পূর্ণসংখ্যা rhsshift = repnum * lhswidth;
                    newp = নতুন AstOr{
                        fl,
                        নতুন AstShiftL{fl, lhsp->clone_বৃক্ষ_Pure(সত্য),
                                      নতুন AstConst{fl, static_cast<uint32_t>(rhsshift)},
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
    যৌক্তিক expandWide(Ast_গ্রন্থি_Assign* nodep, AstReplicate* rhsp) {
        যদি (!doExpandWide(rhsp)) ফেরত মিথ্যা;
        didPattern(VExpandPattern::ASSIGN_REPLICATE, nodep);
        File_সারি* ধ্রুবক fl = nodep->fileline();
        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক lhsp = rhsp->srcp();
        ধ্রুবক পূর্ণসংখ্যা lhswidth = lhsp->widthMin();
        ধ্রুবক AstConst* ধ্রুবক constp = VN_AS(rhsp->countp(), Const);
        UASSERT_OBJ(constp, rhsp, "Replication value isn't a constant.  Checked earlier!");
        ধ্রুবক uint32_t times = constp->toUInt();
        জন্য (পূর্ণসংখ্যা w = 0; w < rhsp->width_শব্দগুলি(); ++w) {
            Ast_গ্রন্থি_রাশিমালা* newp;
            যদি (lhswidth == 1) {
                newp = নতুন AstNegate{fl, lhsp->clone_বৃক্ষ_Pure(সত্য)};
                // Replicate always unsigned
                newp->dtypeChgWidthSigned(VL_EDATASIZE, VL_EDATASIZE, VSigning::UNSIGNED);
            } অন্যথা {
                newp = newAstWordSelClone(lhsp, w);
                File_সারি* ধ্রুবক rfl = rhsp->fileline();
                জন্য (unsigned repnum = 1; repnum < times; repnum++) {
                    newp = নতুন AstOr{fl, newWordGrabShift(rfl, w, lhsp, lhswidth * repnum), newp};
                }
            }
            addWordAssign(nodep, w, newp);
        }
        ফেরত সত্য;
    }

    শূন্য visitEqNeq(Ast_গ্রন্থি_Biop* nodep) {
        যদি (nodep->user1SetOnce()) ফেরত;  // Process once
        iterateChildren(nodep);
        যদি (nodep->lhsp()->isWide()
            && !(VN_IS(nodep->lhsp()->dtypep()->skipRefp(), StreamDType)
                 || VN_IS(nodep->rhsp()->dtypep()->skipRefp(), StreamDType))) {
            যদি (isImpure(nodep)) ফেরত;
            যদি (!doExpandWide(nodep->lhsp())) ফেরত;
            যদি (!doExpandWide(nodep->rhsp())) ফেরত;
            didPattern(VN_IS(nodep, Neq) ? VExpandPattern::WORDIZE_NEQ
                                         : VExpandPattern::WORDIZE_EQ,
                       nodep);
            // -> (0=={or{for each_word{WORDSEL(lhs,#)^WORDSEL(rhs,#)}}}
            File_সারি* ধ্রুবক fl = nodep->fileline();
            Ast_গ্রন্থি_রাশিমালা* newp = nullptr;
            জন্য (পূর্ণসংখ্যা w = 0; w < nodep->lhsp()->width_শব্দগুলি(); ++w) {
                Ast_গ্রন্থি_রাশিমালা* ধ্রুবক eqp = নতুন AstXor{fl, newAstWordSelClone(nodep->lhsp(), w),
                                                    newAstWordSelClone(nodep->rhsp(), w)};
                newp = newp ? নতুন AstOr{fl, newp, eqp} : eqp;
            }
            যদি (VN_IS(nodep, Neq)) {
                newp = নতুন AstNeq{fl, নতুন AstConst{fl, AstConst::SizedEData{}, 0}, newp};
            } অন্যথা {
                newp = নতুন AstEq{fl, নতুন AstConst{fl, AstConst::SizedEData{}, 0}, newp};
            }
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        }
    }
    শূন্য visit(AstEq* nodep) override { visitEqNeq(nodep); }
    শূন্য visit(AstNeq* nodep) override { visitEqNeq(nodep); }

    শূন্য visit(AstRedOr* nodep) override {
        যদি (nodep->user1SetOnce()) ফেরত;  // Process once
        iterateChildren(nodep);
        File_সারি* ধ্রুবক fl = nodep->fileline();
        যদি (nodep->lhsp()->isWide()) {
            যদি (isImpure(nodep)) ফেরত;
            যদি (!doExpandWide(nodep->lhsp())) ফেরত;
            didPattern(VExpandPattern::WORDIZE_REDOR, nodep);
            // -> (0!={or{for each_word{WORDSEL(lhs,#)}}}
            Ast_গ্রন্থি_রাশিমালা* newp = nullptr;
            জন্য (পূর্ণসংখ্যা w = 0; w < nodep->lhsp()->width_শব্দগুলি(); ++w) {
                Ast_গ্রন্থি_রাশিমালা* ধ্রুবক eqp = newAstWordSelClone(nodep->lhsp(), w);
                newp = newp ? নতুন AstOr{fl, newp, eqp} : eqp;
            }
            newp = নতুন AstNeq{fl, নতুন AstConst{fl, AstConst::SizedEData{}, 0}, newp};
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        } অন্যথা {
            // No isImpure() check - can handle side effects in below
            didPattern(VExpandPattern::REDOR_TO_NEQ, nodep);
            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক lhsp = nodep->lhsp()->unlinkFrBack();
            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক newp = নতুন AstNeq{
                fl, নতুন AstConst{fl, AstConst::Widthed_মান{}, longOrQuadWidth(nodep), 0}, lhsp};
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        }
    }
    শূন্য visit(AstRedAnd* nodep) override {
        যদি (nodep->user1SetOnce()) ফেরত;  // Process once
        iterateChildren(nodep);
        File_সারি* ধ্রুবক fl = nodep->fileline();
        যদি (nodep->lhsp()->isWide()) {
            যদি (isImpure(nodep)) ফেরত;
            যদি (!doExpandWide(nodep->lhsp())) ফেরত;
            didPattern(VExpandPattern::WORDIZE_REDAND, nodep);
            // -> (0!={and{for each_word{WORDSEL(lhs,#)}}}
            Ast_গ্রন্থি_রাশিমালা* newp = nullptr;
            জন্য (পূর্ণসংখ্যা w = 0; w < nodep->lhsp()->width_শব্দগুলি(); ++w) {
                Ast_গ্রন্থি_রাশিমালা* eqp = newAstWordSelClone(nodep->lhsp(), w);
                যদি (w == nodep->lhsp()->width_শব্দগুলি() - 1) {
                    // Rather than doing a (slowish) ==##, we OR in the
                    // bits that aren't part of the mask
                    eqp = নতুন AstOr{fl, নতুন AstConst{fl, notWideMask(nodep->lhsp())},
                                    // Bug in cppcheck
                                    // cppcheck-suppress memleak
                                    eqp};
                }
                newp = newp ? নতুন AstAnd{fl, newp, eqp} : eqp;
            }
            newp = নতুন AstEq{fl, নতুন AstConst{fl, AstConst::SizedEData{}, VL_MASK_E(VL_EDATASIZE)},
                             newp};
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        } অন্যথা {
            // No isImpure() check - can handle side effects in below
            didPattern(VExpandPattern::REDAND_TO_EQ, nodep);
            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক lhsp = nodep->lhsp()->unlinkFrBack();
            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক newp = নতুন AstEq{fl, নতুন AstConst{fl, wordMask(lhsp)}, lhsp};
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        }
    }
    শূন্য visit(AstRedXor* nodep) override {
        যদি (nodep->user1SetOnce()) ফেরত;  // Process once
        iterateChildren(nodep);
        যদি (nodep->lhsp()->isWide()) {
            যদি (isImpure(nodep)) ফেরত;
            যদি (!doExpandWide(nodep->lhsp())) ফেরত;
            didPattern(VExpandPattern::WORDIZE_REDXOR, nodep);
            // -> (0!={redxor{for each_word{XOR(WORDSEL(lhs,#))}}}
            File_সারি* ধ্রুবক fl = nodep->fileline();
            Ast_গ্রন্থি_রাশিমালা* newp = nullptr;
            জন্য (পূর্ণসংখ্যা w = 0; w < nodep->lhsp()->width_শব্দগুলি(); ++w) {
                Ast_গ্রন্থি_রাশিমালা* ধ্রুবক eqp = newAstWordSelClone(nodep->lhsp(), w);
                newp = newp ? নতুন AstXor{fl, newp, eqp} : eqp;
            }
            newp = নতুন AstRedXor{fl, newp};
            UINFO(8, "    Wordize REDXORnew " << newp);
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        }
        // We don't reduce non-wide XORs, as its more efficient to use a temp register,
        // which the inlined function does nicely.
    }

    শূন্য visit(Ast_গ্রন্থি_Stmt* nodep) override {
        যদি (nodep->user1SetOnce()) ফেরত;  // Process once
        VL_RESTORER(m_stmtp);
        m_stmtp = nodep;
        iterateChildren(nodep);
    }
    শূন্য visit(Ast_গ্রন্থি_Assign* nodep) override {
        যদি (nodep->user1SetOnce()) ফেরত;  // Process once
        যদি (VN_IS(nodep->dtypep()->skipRefp(), UnpackArrayDType)) {
            ফেরত;  // Skip for UnpackArrayDType
        }

        VL_RESTORER(m_stmtp);
        m_stmtp = nodep;
        iterateChildren(nodep);
        যৌক্তিক did = মিথ্যা;
        যদি (nodep->isWide()  //
            && ((VN_IS(nodep->lhsp(), VarRef) || VN_IS(nodep->lhsp(), ArraySel)))
            && !AstVar::scVarRecurse(nodep->lhsp())  // Need special function for SC
            && !AstVar::scVarRecurse(nodep->rhsp())) {
            যদি (AstConst* ধ্রুবক rhsp = VN_CAST(nodep->rhsp(), Const)) {
                did = expandWide(nodep, rhsp);
            } অন্যথা যদি (AstVarRef* ধ্রুবক rhsp = VN_CAST(nodep->rhsp(), VarRef)) {
                did = expandWide(nodep, rhsp);
            } অন্যথা যদি (AstSel* ধ্রুবক rhsp = VN_CAST(nodep->rhsp(), Sel)) {
                did = expandWide(nodep, rhsp);
            } অন্যথা যদি (AstArraySel* ধ্রুবক rhsp = VN_CAST(nodep->rhsp(), ArraySel)) {
                did = expandWide(nodep, rhsp);
            } অন্যথা যদি (AstConcat* ধ্রুবক rhsp = VN_CAST(nodep->rhsp(), Concat)) {
                did = expandWide(nodep, rhsp);
            } অন্যথা যদি (AstExtend* ধ্রুবক rhsp = VN_CAST(nodep->rhsp(), Extend)) {
                did = expandWide(nodep, rhsp);
            } অন্যথা যদি (AstReplicate* ধ্রুবক rhsp = VN_CAST(nodep->rhsp(), Replicate)) {
                did = expandWide(nodep, rhsp);
            } অন্যথা যদি (AstAnd* ধ্রুবক rhsp = VN_CAST(nodep->rhsp(), And)) {
                did = expandWide(nodep, rhsp);
            } অন্যথা যদি (AstOr* ধ্রুবক rhsp = VN_CAST(nodep->rhsp(), Or)) {
                did = expandWide(nodep, rhsp);
            } অন্যথা যদি (AstNot* ধ্রুবক rhsp = VN_CAST(nodep->rhsp(), Not)) {
                did = expandWide(nodep, rhsp);
            } অন্যথা যদি (AstXor* ধ্রুবক rhsp = VN_CAST(nodep->rhsp(), Xor)) {
                did = expandWide(nodep, rhsp);
            } অন্যথা যদি (AstShiftL* ধ্রুবক rhsp = VN_CAST(nodep->rhsp(), ShiftL)) {
                did = expandWideShift(nodep, rhsp, /* isLeftShift: */ সত্য);
            } অন্যথা যদি (AstShiftR* ধ্রুবক rhsp = VN_CAST(nodep->rhsp(), ShiftR)) {
                did = expandWideShift(nodep, rhsp, /* isLeftShift: */ মিথ্যা);
            } অন্যথা যদি (AstCond* ধ্রুবক rhsp = VN_CAST(nodep->rhsp(), Cond)) {
                did = expandWide(nodep, rhsp);
            }
        } অন্যথা যদি (AstSel* ধ্রুবক lhsp = VN_CAST(nodep->lhsp(), Sel)) {
            did = expandLhs(nodep, lhsp);
        }
        // Cleanup common code
        যদি (did) VL_DO_DANGLING(nodep->unlinkFrBack()->delete_বৃক্ষ(), nodep);
    }

    //--------------------
    শূন্য visit(AstVar*) override {}  // Don't hit varrefs under vars
    শূন্য visit(Ast_গ্রন্থি* nodep) override { iterateChildren(nodep); }

সর্বজনীন:
    // CONSTRUCTORS
    explicit ExpandVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~ExpandVisitor() override {
        V3Stats::addStat("Optimizations, Expand, expanded wides", m_statWides);
        V3Stats::addStat("Optimizations, Expand, expanded wide words", m_stat_Wide_শব্দগুলি);
        V3Stats::addStat("Optimizations, Expand, not expanded impure", m_statImpure);
        V3Stats::addStat("Optimizations, Expand, not expanded limit", m_statWideLimited);
        V3Stats::addStat("Optimizations, Expand, temporaries created", m_statTemps);
        ধ্রুবক auto emitStat = [এই](VExpandPattern pattern) {
            std::string str{pattern.ascii()};
            std::transform(str.begin(), str.end(), str.begin(), [](unsigned অক্ষর c) {  //
                ফেরত c == '_' ? ' ' : std::tolower(c);
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
        ধ্রুবক ExpandOkVisitor okVisitor{nodep};
        ExpandVisitor{nodep};
    }  // Destruct before checking
    V3Global::dump_Check_Global_বৃক্ষ("expand", 0, dump_বৃক্ষ_Either_Level() >= 3);
}
