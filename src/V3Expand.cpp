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

#અન્તર્ભાવ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#અન્તર્ભાવ "V3Expand.h"

#અન્તર્ભાવ "V3Const.h"
#અન્તર્ભાવ "V3Stats.h"

#અન્તર્ભાવ <algorithm>
#અન્તર્ભાવ <array>

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

વર્ગ VExpandPattern final {
જાહેર:
    ગણના en : uint8_t {
#define EXPAND_PATTERN_ID(id) id,
        FOR_EACH_EXPAND_PATTERN(EXPAND_PATTERN_ID)
#undef EXPAND_PATTERN_ID
            _ENUM_END
    };
    ગણના en m_e;
    અચલ અક્ષર* ascii() અચલ {
        સ્થિર અચલ અક્ષર* અચલ names[] = {
#define EXPAND_PATTERN_NAME(id) #id,
            FOR_EACH_EXPAND_PATTERN(EXPAND_PATTERN_NAME)
#undef EXPAND_PATTERN_NAME
                "_ENUM_END"  //
        };
        પાછા names[m_e];
    }

    // cppcheck-suppress noExplicitConstructor
    VExpandPattern(en _e)
        : m_e{_e} {}
    operator en() અચલ { પાછા m_e; }
};

//######################################################################
// Find nodes with side effects, to mark as non-expandable

વર્ગ ExpandOkVisitor final : જાહેર VNVisitorConst {
    // NODE STATE
    //  AstNode::user2()        -> bool.  Is pure (along with all children)
    અચલ VNUser2InUse m_inuser2;

    // STATE - for current visit position (use VL_RESTORER)
    // Tracks similar to AstNode::isTreePureRecurse(), but avoid O(n^2)
    // False = pure, as nodes that ExpandVisitor inserts preserve pureness
    તાર્કિક m_isImpure = સાચું;  // Currently pure

    શૂન્ય visit(Ast_ગાંઠ* nodep) override {
        તાર્કિક selfImpure = !nodep->isPure();
        {
            VL_RESTORER(m_isImpure);
            m_isImpure = ખોટું;
            iterateChildrenConst(nodep);
            selfImpure |= m_isImpure;
            nodep->user2(selfImpure);
        }
        m_isImpure |= selfImpure;
    }

જાહેર:
    // CONSTRUCTORS
    explicit ExpandOkVisitor(AstNetlist* nodep) { iterateConst(nodep); }
    ~ExpandOkVisitor() = મૂળભૂત;
};

//######################################################################
// Expand state, as a visitor of each AstNode

વર્ગ ExpandVisitor final : જાહેર VNVisitor {
    // NODE STATE
    //  AstNode::user1()        -> bool.  Processed
    //  AstNode::user2()        -> See ExpandOkVisitor
    અચલ VNUser1InUse m_inuser1;

    // STATE - for current visit position (use VL_RESTORER)
    Ast_ગાંઠ* m_stmtp = nullptr;  // Current statement

    // STATE - across all visitors
    AstCFunc* m_funcp = nullptr;  // Current function
    VDouble0 m_statImpure;  // Statistic tracking
    VDouble0 m_statWides;  // Statistic tracking
    VDouble0 m_stat_Wide_શબ્દો;  // Statistic tracking
    VDouble0 m_statWideLimited;  // Statistic tracking
    VDouble0 m_statTemps;  // Statistic tracking
    // Number of applications of each expansion pattern for statistics
    std::array<VDouble0, VExpandPattern::_ENUM_END> m_statPatterns;

    // STATE - for current function
    size_t m_nTmps = 0;  // Sequence numbers for temopraries

    // METHODS
    // Note the application of the given expansion pattern to 'nodep'
    શૂન્ય didPattern(VExpandPattern pattern, Ast_ગાંઠ* nodep) {
        UINFO(8, "    " << pattern.ascii() << " " << nodep);
        ++m_statPatterns[pattern];
    }

    // Use state that ExpandOkVisitor calculated
    સ્થિર તાર્કિક isImpure(Ast_ગાંઠ* nodep) {
        અચલ તાર્કિક impure = nodep->user2();
        જો (impure) UINFO(9, "      impure " << nodep);
        પાછા impure;
    }

    તાર્કિક doExpandWide(Ast_ગાંઠ* nodep) {
        જો (isImpure(nodep)) {
            ++m_statImpure;
            પાછા ખોટું;
        }
        જો (nodep->width_શબ્દો() <= v3Global.opt.expandLimit()) {
            ++m_statWides;
            m_stat_Wide_શબ્દો += nodep->width_શબ્દો();
            પાછા સાચું;
        } નહીંતર {
            ++m_statWideLimited;
            પાછા ખોટું;
        }
    }

    સ્થિર પૂર્ણાંક longOrQuadWidth(Ast_ગાંઠ* nodep) {
        પાછા (nodep->width() + (VL_EDATASIZE - 1)) & ~(VL_EDATASIZE - 1);
    }
    સ્થિર V3_સંખ્યા notWideMask(Ast_ગાંઠ* nodep) {
        પાછા V3_સંખ્યા{nodep, VL_EDATASIZE, ~VL_MASK_E(nodep->widthMin())};
    }
    સ્થિર V3_સંખ્યા wordMask(Ast_ગાંઠ* nodep) {
        જો (nodep->isWide()) {
            પાછા V3_સંખ્યા{nodep, VL_EDATASIZE, VL_MASK_E(nodep->widthMin())};
        } નહીંતર {
            V3_સંખ્યા mask{nodep, longOrQuadWidth(nodep)};
            mask.setMask(nodep->widthMin());
            પાછા mask;
        }
    }

    સ્થિર શૂન્ય ઉમેરો_Before(Ast_ગાંઠ* placep, Ast_ગાંઠ* newp) {
        newp->user1(1);  // Already processed, don't need to re-iterate
        placep->addHereThisAsNext(newp);
    }
    સ્થિર શૂન્ય replaceWithDelete(Ast_ગાંઠ* nodep, Ast_ગાંઠ* newp) {
        newp->user1(1);  // Already processed, don't need to re-iterate
        જો (newp->width() != nodep->width()) {
            UASSERT_OBJ(newp->widthMin() == nodep->widthMin(), nodep,
                        "Replacement width mismatch");
            newp->dtypeChgWidth(nodep->width(), nodep->widthMin());
        }
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_વૃક્ષ(), nodep);
    }
    સ્થિર Ast_ગાંઠ* newWordAssign(Ast_ગાંઠ_Assign* placep, પૂર્ણાંક word, Ast_ગાંઠ_પદાવલિ* lhsp,
                                  Ast_ગાંઠ_પદાવલિ* rhsp) {
        File_લીટી* અચલ fl = placep->fileline();
        પાછા નવું AstAssign{fl,
                             નવું AstWordSel{fl, lhsp->clone_વૃક્ષ_Pure(સાચું),
                                            નવું AstConst{fl, static_cast<uint32_t>(word)}},
                             rhsp};
    }
    સ્થિર શૂન્ય addWordAssign(Ast_ગાંઠ_Assign* placep, પૂર્ણાંક word, Ast_ગાંઠ_પદાવલિ* lhsp,
                              Ast_ગાંઠ_પદાવલિ* rhsp) {
        ઉમેરો_Before(placep, newWordAssign(placep, word, lhsp, rhsp));
    }
    સ્થિર શૂન્ય addWordAssign(Ast_ગાંઠ_Assign* placep, પૂર્ણાંક word, Ast_ગાંઠ_પદાવલિ* rhsp) {
        addWordAssign(placep, word, placep->lhsp(), rhsp);
    }

    AstVar* addLocalTmp(Ast_ગાંઠ* placep, અચલ અક્ષર* namep, Ast_ગાંઠ_પદાવલિ* valuep) {
        અચલ std::string name = "__V"s + namep + "_"s + std::to_string(m_nTmps);
        File_લીટી* અચલ flp = placep->fileline();
        AstVar* અચલ tmpp = નવું AstVar{flp, VVarType::STMTTEMP, name, valuep->dtypep()};
        tmpp->funcLocal(સાચું);
        tmpp->isInternal(સાચું);
        tmpp->noReset(સાચું);
        tmpp->substConstOnly(સાચું);
        m_funcp->addVarsp(tmpp);
        ઉમેરો_Before(placep, નવું AstAssign{flp, નવું AstVarRef{flp, tmpp, VAccess::WRITE}, valuep});
        ++m_statTemps;
        પાછા tmpp;
    }

    સ્થિર શૂન્ય fixCloneLvalue(Ast_ગાંઠ* nodep) {
        // In AstSel transforms, we call clone() on VarRefs that were lvalues,
        // but are now being used on the RHS of the assignment
        જો (VN_IS(nodep, VarRef)) VN_AS(nodep, VarRef)->access(VAccess::READ);
        // Iterate
        જો (Ast_ગાંઠ* અચલ refp = nodep->op1p()) fixCloneLvalue(refp);
        જો (Ast_ગાંઠ* અચલ refp = nodep->op2p()) fixCloneLvalue(refp);
        જો (Ast_ગાંઠ* અચલ refp = nodep->op3p()) fixCloneLvalue(refp);
        જો (Ast_ગાંઠ* અચલ refp = nodep->op4p()) fixCloneLvalue(refp);
    }

    સ્થિર Ast_ગાંઠ_પદાવલિ* newAstWordSelClone(Ast_ગાંઠ_પદાવલિ* nodep, પૂર્ણાંક word) {
        // Get the specified word number from a wide array
        // Or, if it's a long/quad, do appropriate conversion to wide
        // Concat may pass negative word numbers, that means it wants a zero
        File_લીટી* અચલ fl = nodep->fileline();
        જો (nodep->isWide() && word >= 0 && word < nodep->width_શબ્દો()) {
            પાછા નવું AstWordSel{fl, nodep->clone_વૃક્ષ_Pure(સાચું),
                                  નવું AstConst{fl, static_cast<uint32_t>(word)}};
        } નહીંતર જો (nodep->isQuad() && word == 0) {
            Ast_ગાંઠ_પદાવલિ* અચલ quadfromp = nodep->clone_વૃક્ષ_Pure(સાચું);
            quadfromp->dtypeSetBitUnsized(VL_QUADSIZE, quadfromp->widthMin(), VSigning::UNSIGNED);
            પાછા નવું AstCCast{fl, quadfromp, VL_EDATASIZE};
        } નહીંતર જો (nodep->isQuad() && word == 1) {
            Ast_ગાંઠ_પદાવલિ* અચલ quadfromp = nodep->clone_વૃક્ષ_Pure(સાચું);
            quadfromp->dtypeSetBitUnsized(VL_QUADSIZE, quadfromp->widthMin(), VSigning::UNSIGNED);
            પાછા નવું AstCCast{
                fl, નવું AstShiftR{fl, quadfromp, નવું AstConst{fl, VL_EDATASIZE}, VL_EDATASIZE},
                VL_EDATASIZE};
        } નહીંતર જો (!nodep->isWide() && !nodep->isQuad() && word == 0) {
            પાછા nodep->clone_વૃક્ષ_Pure(સાચું);
        } નહીંતર {  // Out of bounds
            પાછા નવું AstConst{fl, 0};
        }
    }

    સ્થિર Ast_ગાંઠ_પદાવલિ* newWordGrabShift(File_લીટી* fl, પૂર્ણાંક word, Ast_ગાંઠ_પદાવલિ* lhsp, પૂર્ણાંક shift) {
        // Extract the expression to grab the value for the specified word, if it's the shift
        // of shift bits from lhsp
        // Negative word numbers requested for lhs when it's "before" what we want.
        // We get a 0 then.
        અચલ પૂર્ણાંક othword = word - shift / VL_EDATASIZE;
        Ast_ગાંઠ_પદાવલિ* અચલ lop = newAstWordSelClone(lhsp, othword);
        અચલ uint32_t loShift = VL_BITBIT_E(shift);
        જો (!loShift) પાછા lop;
        Ast_ગાંઠ_પદાવલિ* અચલ hip = newAstWordSelClone(lhsp, othword - 1);
        અચલ uint32_t hiShift = VL_EDATASIZE - loShift;  // Complement offset
        Ast_ગાંઠ_પદાવલિ* અચલ newp
            = નવું AstOr{fl,  //
                        નવું AstShiftR{fl, hip, નવું AstConst{fl, hiShift}, VL_EDATASIZE},
                        નવું AstShiftL{fl, lop, નવું AstConst{fl, loShift}, VL_EDATASIZE}};
        પાછા V3Const::constifyEditCpp(newp);
    }

    // Return expression indexing the word that contains 'lsbp' + the given word offset
    સ્થિર Ast_ગાંઠ_પદાવલિ* newWordIndex(Ast_ગાંઠ_પદાવલિ* lsbp, uint32_t wordOffset = 0) {
        // This is indexing a WordSel, so a 32 bit constants are fine
        File_લીટી* અચલ flp = lsbp->fileline();
        જો (અચલ AstConst* constp = VN_CAST(lsbp, Const)) {
            પાછા નવું AstConst{flp, wordOffset + VL_BITWORD_E(constp->toUInt())};
        }

        જો (lsbp->backp()) lsbp = lsbp->clone_વૃક્ષ_Pure(ખોટું);
        Ast_ગાંઠ_પદાવલિ* અચલ wwl2p = નવું AstConst{flp, VL_EDATASIZE_LOG2};  // Word width log 2
        Ast_ગાંઠ_પદાવલિ* indexp = નવું AstShiftR{flp, lsbp, wwl2p, VL_EDATASIZE};
        જો (wordOffset) indexp = નવું AstAdd{flp, નવું AstConst{flp, wordOffset}, indexp};
        પાછા indexp;
    }

    // Return word of fromp that contains word indexp + the given word offset.
    સ્થિર Ast_ગાંઠ_પદાવલિ* newWordSelWord(File_લીટી* flp, Ast_ગાંઠ_પદાવલિ* fromp, Ast_ગાંઠ_પદાવલિ* indexp,
                                       uint32_t wordOffset = 0) {
        UASSERT_OBJ(fromp->isWide(), fromp, "Only need AstWordSel on wide from's");

        જો (અચલ AstConst* અચલ constp = VN_CAST(indexp, Const)) {
            indexp = nullptr;
            wordOffset += constp->toUInt();
        }

        // e.g. "logic [95:0] var[0]; logic [0] sel; out = var[sel];"
        // Squash before C++ to avoid getting a C++ compiler warning
        // (even though code would be unreachable as presumably a
        // AstCond is protecting above this node.
        જો (wordOffset >= static_cast<uint32_t>(fromp->width_શબ્દો())) {
            પાછા નવું AstConst{flp, AstConst::SizedEData{}, 0};
        }

        જો (fromp->backp()) fromp = fromp->clone_વૃક્ષ_Pure(ખોટું);

        // If indexp was constant, just use it
        જો (!indexp) પાછા નવું AstWordSel{flp, fromp, નવું AstConst{flp, wordOffset}};

        // Otherwise compute at runtime
        જો (indexp->backp()) indexp = indexp->clone_વૃક્ષ_Pure(ખોટું);
        જો (wordOffset) indexp = નવું AstAdd{flp, નવું AstConst{flp, wordOffset}, indexp};
        પાછા નવું AstWordSel{flp, fromp, indexp};
    }

    // Return word of fromp that contains bit lsbp + the given word offset.
    સ્થિર Ast_ગાંઠ_પદાવલિ* newWordSelBit(File_લીટી* flp, Ast_ગાંઠ_પદાવલિ* fromp, Ast_ગાંઠ_પદાવલિ* lsbp,
                                      uint32_t wordOffset = 0) {
        Ast_ગાંઠ_પદાવલિ* અચલ indexp = newWordIndex(lsbp, wordOffset);
        Ast_ગાંઠ_પદાવલિ* અચલ wordSelp = newWordSelWord(flp, fromp, indexp);
        જો (!indexp->backp()) VL_DO_DANGLING(indexp->delete_વૃક્ષ(), indexp);
        પાછા wordSelp;
    }

    સ્થિર Ast_ગાંઠ_પદાવલિ* newSelBitBit(Ast_ગાંઠ_પદાવલિ* lsbp) {
        // Return equation to get the VL_BITBIT of a constant or non-constant
        File_લીટી* અચલ fl = lsbp->fileline();
        જો (VN_IS(lsbp, Const)) {
            પાછા નવું AstConst{fl, VL_BITBIT_E(VN_AS(lsbp, Const)->toUInt())};
        } નહીંતર {
            પાછા નવું AstAnd{fl, નવું AstConst{fl, VL_EDATASIZE - 1}, lsbp->clone_વૃક્ષ_Pure(સાચું)};
        }
    }

    //====================

    તાર્કિક expandWide(Ast_ગાંઠ_Assign* nodep, AstConst* rhsp) {
        જો (!doExpandWide(nodep)) પાછા ખોટું;
        didPattern(VExpandPattern::ASSIGN_CONST, nodep);
        // -> {for each_word{ ASSIGN(WORDSEL(wide,#),WORDSEL(CONST,#))}}
        જો (rhsp->અંક().isFourState()) {
            rhsp->v3warn(E_UNSUPPORTED,  // LCOV_EXCL_LINE  // impossible?
                         "Unsupported: 4-state numbers in this context");
        }
        File_લીટી* અચલ fl = nodep->fileline();
        માટે (પૂર્ણાંક w = 0; w < nodep->width_શબ્દો(); ++w) {
            addWordAssign(nodep, w,
                          નવું AstConst{fl, AstConst::SizedEData{}, rhsp->અંક().edataWord(w)});
        }
        પાછા સાચું;
    }
    //-------- Uniops
    તાર્કિક expandWide(Ast_ગાંઠ_Assign* nodep, AstVarRef* rhsp) {
        // Special case: do not expand assignment of constant pool variables.
        // V3Subst undestands these directly.
        જો (rhsp->varp()->constPoolEntry()) પાછા ખોટું;
        જો (!doExpandWide(nodep)) પાછા ખોટું;
        didPattern(VExpandPattern::ASSIGN_VARREF, nodep);
        માટે (પૂર્ણાંક w = 0; w < nodep->width_શબ્દો(); ++w) {
            addWordAssign(nodep, w, newAstWordSelClone(rhsp, w));
        }
        પાછા સાચું;
    }
    તાર્કિક expandWide(Ast_ગાંઠ_Assign* nodep, AstArraySel* rhsp) {
        UASSERT_OBJ(!VN_IS(nodep->dtypep()->skipRefp(), UnpackArrayDType), nodep,
                    "ArraySel with unpacked arrays should have been removed in V3Slice");
        જો (!doExpandWide(nodep)) પાછા ખોટું;
        didPattern(VExpandPattern::ASSIGN_ARRAYSEL, nodep);
        માટે (પૂર્ણાંક w = 0; w < nodep->width_શબ્દો(); ++w) {
            addWordAssign(nodep, w, newAstWordSelClone(rhsp, w));
        }
        પાછા સાચું;
    }
    તાર્કિક expandWide(Ast_ગાંઠ_Assign* nodep, AstNot* rhsp) {
        // -> {for each_word{ ASSIGN(WORDSEL(wide,#),NOT(WORDSEL(lhs,#))) }}
        જો (!doExpandWide(nodep)) પાછા ખોટું;
        didPattern(VExpandPattern::ASSIGN_NOT, nodep);
        File_લીટી* અચલ fl = rhsp->fileline();
        માટે (પૂર્ણાંક w = 0; w < nodep->width_શબ્દો(); ++w) {
            addWordAssign(nodep, w, નવું AstNot{fl, newAstWordSelClone(rhsp->lhsp(), w)});
        }
        પાછા સાચું;
    }
    //-------- Biops
    તાર્કિક expandWide(Ast_ગાંઠ_Assign* nodep, AstAnd* rhsp) {
        જો (!doExpandWide(nodep)) પાછા ખોટું;
        didPattern(VExpandPattern::ASSIGN_AND, nodep);
        File_લીટી* અચલ fl = nodep->fileline();
        માટે (પૂર્ણાંક w = 0; w < nodep->width_શબ્દો(); ++w) {
            addWordAssign(nodep, w,
                          નવું AstAnd{fl, newAstWordSelClone(rhsp->lhsp(), w),
                                     newAstWordSelClone(rhsp->rhsp(), w)});
        }
        પાછા સાચું;
    }
    તાર્કિક expandWide(Ast_ગાંઠ_Assign* nodep, AstOr* rhsp) {
        જો (!doExpandWide(nodep)) પાછા ખોટું;
        didPattern(VExpandPattern::ASSIGN_OR, nodep);
        File_લીટી* અચલ fl = nodep->fileline();
        માટે (પૂર્ણાંક w = 0; w < nodep->width_શબ્દો(); ++w) {
            addWordAssign(nodep, w,
                          નવું AstOr{fl, newAstWordSelClone(rhsp->lhsp(), w),
                                    newAstWordSelClone(rhsp->rhsp(), w)});
        }
        પાછા સાચું;
    }
    તાર્કિક expandWide(Ast_ગાંઠ_Assign* nodep, AstXor* rhsp) {
        જો (!doExpandWide(nodep)) પાછા ખોટું;
        didPattern(VExpandPattern::ASSIGN_XOR, nodep);
        File_લીટી* અચલ fl = nodep->fileline();
        માટે (પૂર્ણાંક w = 0; w < nodep->width_શબ્દો(); ++w) {
            addWordAssign(nodep, w,
                          નવું AstXor{fl, newAstWordSelClone(rhsp->lhsp(), w),
                                     newAstWordSelClone(rhsp->rhsp(), w)});
        }
        પાછા સાચું;
    }
    તાર્કિક expandWideShift(Ast_ગાંઠ_Assign* nodep, Ast_ગાંઠ_Biop* rhsp, તાર્કિક is_ડાબું_Shift) {
        જો (!doExpandWide(nodep)) પાછા ખોટું;

        // Simplify the shift amount, in case it becomes a constant
        V3Const::constifyEditCpp(rhsp->rhsp());

        // If it's a constant shift by whole words, expand it so V3Subst can substitute it
        જો (અચલ AstConst* અચલ rhsConstp = VN_CAST(rhsp->rhsp(), Const)) {
            અચલ uint32_t shiftBits = rhsConstp->toUInt();
            જો (VL_BITBIT_E(shiftBits) == 0) {
                અચલ પૂર્ણાંક width_શબ્દો = nodep->width_શબ્દો();
                અચલ પૂર્ણાંક shift_શબ્દો = std::min<પૂર્ણાંક>(VL_BITWORD_E(shiftBits), width_શબ્દો);
                File_લીટી* અચલ flp = rhsp->fileline();
                જો (is_ડાબું_Shift) {
                    didPattern(VExpandPattern::ASSIGN_SHIFTL, nodep);
                    // Low words of the result are zero
                    માટે (પૂર્ણાંક w = 0; w < shift_શબ્દો; ++w) {
                        addWordAssign(nodep, w, નવું AstConst{flp, AstConst::SizedEData{}, 0});
                    }
                    // High words of the result are copied from higher words of the source
                    માટે (પૂર્ણાંક w = shift_શબ્દો; w < width_શબ્દો; ++w) {
                        addWordAssign(nodep, w, newAstWordSelClone(rhsp->lhsp(), w - shift_શબ્દો));
                    }
                } નહીંતર {
                    didPattern(VExpandPattern::ASSIGN_SHIFTR, nodep);
                    // Low words of the result are copied from higher words of the source
                    માટે (પૂર્ણાંક w = 0; w < width_શબ્દો - shift_શબ્દો; ++w) {
                        addWordAssign(nodep, w, newAstWordSelClone(rhsp->lhsp(), w + shift_શબ્દો));
                    }
                    // High words of the result are zero
                    માટે (પૂર્ણાંક w = width_શબ્દો - shift_શબ્દો; w < width_શબ્દો; ++w) {
                        addWordAssign(nodep, w, નવું AstConst{flp, AstConst::SizedEData{}, 0});
                    }
                }
                પાછા સાચું;
            }
        }

        પાછા ખોટું;
    }

    //-------- Triops
    તાર્કિક expandWide(Ast_ગાંઠ_Assign* nodep, AstCond* rhsp) {
        જો (!doExpandWide(nodep)) પાછા ખોટું;
        didPattern(VExpandPattern::ASSIGN_COND, nodep);
        File_લીટી* અચલ fl = nodep->fileline();
        માટે (પૂર્ણાંક w = 0; w < nodep->width_શબ્દો(); ++w) {
            addWordAssign(nodep, w,
                          નવું AstCond{fl, rhsp->condp()->clone_વૃક્ષ_Pure(સાચું),
                                      newAstWordSelClone(rhsp->thenp(), w),
                                      newAstWordSelClone(rhsp->elsep(), w)});
        }
        પાછા સાચું;
    }

    // VISITORS
    શૂન્ય visit(AstNetlist* nodep) override { iterateAndNextNull(nodep->modulesp()); }

    શૂન્ય visit(AstCFunc* nodep) override {
        VL_RESTORER(m_funcp);
        VL_RESTORER(m_nTmps);
        m_funcp = nodep;
        m_nTmps = 0;
        અચલ VDouble0 statWidesBefore = m_statWides;
        iterateChildren(nodep);

        // Constant fold here if anything was expanded, as Ast size can likely be reduced
        જો (v3Global.opt.fConstEager() && m_statWides != statWidesBefore) {
            Ast_ગાંઠ* અચલ editedp = V3Const::constifyEditCpp(nodep);
            UASSERT_OBJ(editedp == nodep, editedp, "Should not have replaced CFunc");
        }
    }

    શૂન્ય visit(AstExtend* nodep) override {
        જો (nodep->user1SetOnce()) પાછા;  // Process once
        iterateChildren(nodep);
        જો (nodep->isWide()) {
            // See under ASSIGN(EXTEND)
        } નહીંતર {
            જો (isImpure(nodep)) પાછા;
            Ast_ગાંઠ_પદાવલિ* અચલ lhsp = nodep->lhsp()->unlinkFrBack();
            Ast_ગાંઠ_પદાવલિ* newp = lhsp;
            જો (nodep->isQuad()) {
                જો (lhsp->isQuad()) {
                    lhsp->dtypeFrom(nodep);  // Just mark it, else nop
                } નહીંતર જો (lhsp->isWide()) {
                    nodep->v3fatalSrc("extending larger thing into smaller?");
                } નહીંતર {
                    newp = નવું AstCCast{nodep->fileline(), lhsp, nodep};
                }
            } નહીંતર {  // Long
                UASSERT_OBJ(!(lhsp->isQuad() || lhsp->isWide()), nodep,
                            "extending larger thing into smaller?");
                lhsp->dtypeFrom(nodep);  // Just mark it, else nop
            }
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        }
    }
    તાર્કિક expandWide(Ast_ગાંઠ_Assign* nodep, AstExtend* rhsp) {
        જો (!doExpandWide(nodep)) પાછા ખોટું;
        didPattern(VExpandPattern::ASSIGN_EXTEND, nodep);
        Ast_ગાંઠ_પદાવલિ* અચલ rlhsp = rhsp->lhsp();
        માટે (પૂર્ણાંક w = 0; w < rlhsp->width_શબ્દો(); ++w) {
            addWordAssign(nodep, w, newAstWordSelClone(rlhsp, w));
        }
        માટે (પૂર્ણાંક w = rlhsp->width_શબ્દો(); w < nodep->width_શબ્દો(); ++w) {
            addWordAssign(nodep, w, નવું AstConst{nodep->fileline(), AstConst::SizedEData{}, 0});
        }
        પાછા સાચું;
    }

    શૂન્ય visit(AstSel* nodep) override {
        જો (nodep->user1SetOnce()) પાછા;  // Process once
        iterateChildren(nodep);

        અચલ uint32_t width = static_cast<uint32_t>(nodep->widthConst());
        UASSERT_OBJ(nodep->widthMin() == static_cast<પૂર્ણાંક>(width), nodep, "Width mismatch");

        // Skip if Sel is an LHS assignment select
        જો (Ast_ગાંઠ_Assign* અચલ assignp = VN_CAST(nodep->backp(), ગાંઠ_Assign)) {
            જો (nodep == assignp->lhsp()) પાછા;
        }

        // Skip if wide: See under ASSIGN(WIDE)
        જો (nodep->isWide()) પાછા;

        // Skip Sel from stream or queue
        જો (VN_IS(nodep->fromp()->dtypep(), StreamDType)) પાછા;
        જો (VN_IS(nodep->fromp()->dtypep(), QueueDType)) પાછા;

        // Result must be non-wide after skipping all the above
        UASSERT_OBJ(width <= 64, nodep, "Inconsistent result width");

        જો (nodep->fromp()->isWide()) {  // Long/Quad from Wide
            જો (isImpure(nodep)) પાછા;
            didPattern(VExpandPattern::SEL_FROM_WIDE, nodep);
            File_લીટી* અચલ nfl = nodep->fileline();
            File_લીટી* અચલ lfl = nodep->lsbp()->fileline();
            File_લીટી* અચલ ffl = nodep->fromp()->fileline();
            Ast_ગાંઠ_પદાવલિ* અચલ fromp = nodep->fromp();

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
            અચલ auto tryFold = [&](Ast_ગાંઠ_પદાવલિ* exprp) -> Ast_ગાંઠ_પદાવલિ* {
                // Size limit for inlining expressions (any arbitrary value results in
                // a bounded expansion. Picked a value to allow common simple forms.)
                સ્થિર constexpr પૂર્ણાંક EXPAND_SEL_LSB_LIMIT = 8;
                // Simplify
                exprp = V3Const::constifyEditCpp(exprp);
                // Accept if not larger than the limit
                જો (!exprp->isLargerThan(EXPAND_SEL_LSB_LIMIT)) પાછા exprp;
                // Delete if rejected
                VL_DO_DANGLING(exprp->delete_વૃક્ષ(), exprp);
                પાછા nullptr;
            };

            // Return the index of the word holding the bit 'offset' bits above the LSB
            અચલ auto wordIdx = [&](uint32_t offset) -> Ast_ગાંઠ_પદાવલિ* {
                Ast_ગાંઠ_પદાવલિ* અચલ msbp = નવું AstAdd{lfl, નવું AstConst{lfl, offset},
                                                     nodep->lsbp()->clone_વૃક્ષ_Pure(ખોટું)};
                Ast_ગાંઠ_પદાવલિ* અચલ idxp = newWordIndex(msbp);
                જો (!msbp->backp()) VL_DO_DANGLING(msbp->delete_વૃક્ષ(), msbp);
                પાછા idxp;
            };

            // Bit index of the selected LSB within its word
            Ast_ગાંઠ_પદાવલિ* lBitp = tryFold(newSelBitBit(nodep->lsbp()));
            અચલ તાર્કિક aligned = lBitp && lBitp->isZero();

            // Which of the other 2 words the expansion might need
            અચલ તાર્કિક mNeeded = width > 1 && !aligned;
            અચલ તાર્કિક hNeeded = width > VL_EDATASIZE;
            UASSERT_OBJ(!hNeeded || nodep->isQuad(), nodep, "Width mismatch");

            // Word indices
            અચલ uint32_t mMsbOffset = std::min<uint32_t>(width, VL_EDATASIZE) - 1;
            અચલ uint32_t hMsbOffset = width - 1;
            Ast_ગાંઠ_પદાવલિ* lIdxp = tryFold(newWordIndex(nodep->lsbp()));
            Ast_ગાંઠ_પદાવલિ* mIdxp = mNeeded ? tryFold(wordIdx(mMsbOffset)) : nullptr;
            Ast_ગાંઠ_પદાવલિ* hIdxp = hNeeded ? tryFold(wordIdx(hMsbOffset)) : nullptr;

            // If the LSB expression is needed multiple times, evaluate it into a temporary
            અચલ પૂર્ણાંક nCopies = !lBitp + !lIdxp + (mNeeded && !mIdxp) + (hNeeded && !hIdxp);
            જો (nCopies > 1 && m_funcp && m_stmtp && !isImpure(m_stmtp)) {
                ++m_nTmps;  // Use fresh set of temporaries
                Ast_ગાંઠ_પદાવલિ* અચલ lsbp = nodep->lsbp()->unlinkFrBack();
                AstVar* અચલ tmpp = addLocalTmp(m_stmtp, "ExpandSel_Lsb", lsbp);
                nodep->lsbp(નવું AstVarRef{lfl, tmpp, VAccess::READ});
            }

            // Rebuild remaining expressions derived from LSB if needed, now using the temporary
            જો (!lBitp) lBitp = newSelBitBit(nodep->lsbp());
            જો (!lIdxp) lIdxp = newWordIndex(nodep->lsbp());
            જો (!mIdxp) mIdxp = mNeeded ? wordIdx(mMsbOffset) : nullptr;
            જો (!hIdxp) hIdxp = hNeeded ? wordIdx(hMsbOffset) : nullptr;

            // Return word 'idxp' of 'fromp', without consuming 'idxp'
            અચલ auto wordSel = [&](Ast_ગાંઠ_પદાવલિ* idxp) -> Ast_ગાંઠ_પદાવલિ* {
                Ast_ગાંઠ_પદાવલિ* અચલ clonep = idxp->clone_વૃક્ષ_Pure(ખોટું);
                Ast_ગાંઠ_પદાવલિ* અચલ wordp = newWordSelWord(ffl, fromp, clonep);
                જો (!clonep->backp()) VL_DO_DANGLING(clonep->delete_વૃક્ષ(), clonep);
                પાછા wordp;
            };

            // Construct term containing the bits of the low word - always needed
            Ast_ગાંઠ_પદાવલિ* અચલ lTermp = [&]() -> Ast_ગાંઠ_પદાવલિ* {
                Ast_ગાંઠ_પદાવલિ* lWordp = wordSel(lIdxp);
                જો (nodep->isQuad()) lWordp = નવું AstCCast{nfl, lWordp, nodep};
                પાછા નવું AstShiftR{nfl, lWordp, lBitp->clone_વૃક્ષ_Pure(ખોટું), nodep->width()};
            }();

            // Construct term containing the bits of the middle word - if needed
            Ast_ગાંઠ_પદાવલિ* અચલ mTermp = [&]() -> Ast_ગાંઠ_પદાવલિ* {
                જો (!mNeeded) પાછા nullptr;

                Ast_ગાંઠ_પદાવલિ* mWordp = wordSel(mIdxp);
                જો (nodep->isQuad()) mWordp = નવું AstCCast{nfl, mWordp, nodep};
                Ast_ગાંઠ_પદાવલિ* અચલ mShiftp = નવું AstSub{lfl, નવું AstConst{lfl, VL_EDATASIZE},
                                                        lBitp->clone_વૃક્ષ_Pure(ખોટું)};
                // If we're selecting bit zero, then all 32 bits in the mid word
                // get shifted << by 32 bits, so ignore them.
                અચલ V3_સંખ્યા zero{nodep, longOrQuadWidth(nodep)};
                પાછા નવું AstCond{
                    nfl,
                    // lsb % VL_EDATASIZE == 0 ?
                    નવું AstEq{nfl, નવું AstConst{nfl, 0}, lBitp->clone_વૃક્ષ_Pure(ખોટું)},
                    // 0 :
                    નવું AstConst{nfl, zero},
                    //  midword >> (VL_EDATASIZE - (lbs % VL_EDATASIZE))
                    નવું AstShiftL{nfl, mWordp, mShiftp, nodep->width()}};
            }();

            // Construct term containing the bits of the high word - if needed
            Ast_ગાંઠ_પદાવલિ* અચલ hTermp = [&]() -> Ast_ગાંઠ_પદાવલિ* {
                જો (!hNeeded) પાછા nullptr;

                Ast_ગાંઠ_પદાવલિ* hWordp = wordSel(hIdxp);
                hWordp = નવું AstCCast{nfl, hWordp, nodep};
                Ast_ગાંઠ_પદાવલિ* અચલ hShiftp = નવું AstCond{
                    nfl,
                    // lsb % VL_EDATASIZE == 0 ?
                    નવું AstEq{nfl, નવું AstConst{nfl, 0}, lBitp->clone_વૃક્ષ_Pure(ખોટું)},
                    // VL_EDATASIZE :
                    નવું AstConst{lfl, VL_EDATASIZE},
                    // 64 - (lbs % VL_EDATASIZE)
                    નવું AstSub{lfl, નવું AstConst{lfl, 64}, lBitp->clone_વૃક્ષ_Pure(ખોટું)}};
                પાછા નવું AstShiftL{nfl, hWordp, hShiftp, nodep->width()};
            }();

            // Delete parts not captured during construction
            VL_DO_DANGLING(lBitp->delete_વૃક્ષ(), lBitp);
            VL_DO_DANGLING(lIdxp->delete_વૃક્ષ(), lIdxp);
            જો (mIdxp) VL_DO_DANGLING(mIdxp->delete_વૃક્ષ(), mIdxp);
            જો (hIdxp) VL_DO_DANGLING(hIdxp->delete_વૃક્ષ(), hIdxp);

            // Or reduce the terms
            Ast_ગાંઠ_પદાવલિ* newp = lTermp;
            જો (mTermp) newp = નવું AstOr{nfl, mTermp, newp};
            જો (hTermp) newp = નવું AstOr{nfl, hTermp, newp};
            newp->dtypeFrom(nodep);
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        } નહીંતર {  // Long/Quad from Long/Quad
            // No isImpure() check - can handle side effects in below
            didPattern(VExpandPattern::SEL_FROM_NARROW, nodep);
            File_લીટી* અચલ fl = nodep->fileline();
            Ast_ગાંઠ_પદાવલિ* fromp = nodep->fromp()->unlinkFrBack();
            Ast_ગાંઠ_પદાવલિ* અચલ lsbp = nodep->lsbp()->unlinkFrBack();
            જો (nodep->isQuad() && !fromp->isQuad()) fromp = નવું AstCCast{fl, fromp, nodep};
            // {large}>>32 requires 64-bit shift operation; then cast
            Ast_ગાંઠ_પદાવલિ* newp = નવું AstShiftR{fl, fromp, lsbp, fromp->width()};
            newp->dtypeFrom(fromp);
            જો (!nodep->isQuad() && fromp->isQuad()) newp = નવું AstCCast{fl, newp, nodep};
            newp->dtypeFrom(nodep);
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        }
    }

    તાર્કિક expandWide(Ast_ગાંઠ_Assign* nodep, AstSel* rhsp) {
        UASSERT_OBJ(nodep->widthMin() == rhsp->widthConst(), nodep, "Width mismatch");
        જો (!doExpandWide(nodep)) પાછા ખોટું;

        // Simplify the index, in case it becomes a constant
        V3Const::constifyEditCpp(rhsp->lsbp());

        // If it's a constant select and aligned, we can just copy the words
        જો (અચલ AstConst* અચલ lsbConstp = VN_CAST(rhsp->lsbp(), Const)) {
            અચલ uint32_t lsb = lsbConstp->toUInt();
            જો (VL_BITBIT_E(lsb) == 0) {
                didPattern(VExpandPattern::ASSIGN_SEL_ALIGNED, nodep);
                અચલ uint32_t word = VL_BITWORD_E(lsb);
                માટે (પૂર્ણાંક w = 0; w < nodep->width_શબ્દો(); ++w) {
                    addWordAssign(nodep, w, newAstWordSelClone(rhsp->fromp(), w + word));
                }
                પાછા સાચું;
            }
        }

        didPattern(VExpandPattern::ASSIGN_SEL_UNALIGNED, nodep);
        File_લીટી* અચલ flp = rhsp->fileline();

        // Use fresh set of temporaries
        ++m_nTmps;

        // Compute word index of LSB, store to temporary if not constant
        Ast_ગાંઠ_પદાવલિ* અચલ wordLsbp = rhsp->lsbp()->clone_વૃક્ષ_Pure(ખોટું);
        Ast_ગાંઠ_પદાવલિ* wordIdxp = newWordIndex(wordLsbp);
        જો (!wordLsbp->backp()) VL_DO_DANGLING(wordLsbp->delete_વૃક્ષ(), wordLsbp);
        wordIdxp = V3Const::constifyEditCpp(wordIdxp);
        જો (!VN_IS(wordIdxp, Const)) {
            AstVar* અચલ tmpp = addLocalTmp(nodep, "ExpandSel_WordIdx", wordIdxp);
            wordIdxp = નવું AstVarRef{flp, tmpp, VAccess::READ};
        }

        // Compute shift amounts and mask, store to temporaries if not constants
        Ast_ગાંઠ_પદાવલિ* loShftp = nullptr;
        Ast_ગાંઠ_પદાવલિ* hiShftp = nullptr;
        Ast_ગાંઠ_પદાવલિ* hiMaskp = nullptr;
        જો (અચલ AstConst* અચલ lsbConstp = VN_CAST(rhsp->lsbp(), Const)) {
            અચલ uint32_t bitOffset = VL_BITBIT_E(lsbConstp->toUInt());
            // Must be unaligned, otherwise we would have handled it above
            UASSERT_OBJ(bitOffset, nodep, "Missed aligned wide select");
            loShftp = નવું AstConst{flp, bitOffset};
            hiShftp = નવું AstConst{flp, VL_EDATASIZE - bitOffset};
            hiMaskp = nullptr;
        } નહીંતર {
            // Compute Low word shift amount: bottom bits of lsb index
            Ast_ગાંઠ_પદાવલિ* અચલ lsbp = rhsp->lsbp()->clone_વૃક્ષ_Pure(ખોટું);
            loShftp = નવું AstAnd{flp, નવું AstConst{flp, VL_SIZEBITS_E}, lsbp};
            AstVar* અચલ loTmpp = addLocalTmp(nodep, "ExpandSel_LoShift", loShftp);
            loShftp = નવું AstVarRef{flp, loTmpp, VAccess::READ};

            // Compute if aligned: Low word shift amount is 0
            Ast_ગાંઠ_પદાવલિ* અચલ zerop = નવું AstConst{flp, 0};
            Ast_ગાંઠ_પદાવલિ* alignedp = નવું AstEq{flp, zerop, loShftp->clone_વૃક્ષ_Pure(ખોટું)};
            AstVar* અચલ alignedTmpp = addLocalTmp(nodep, "ExpandSel_Aligned", alignedp);
            alignedp = નવું AstVarRef{flp, alignedTmpp, VAccess::READ};

            // Computed High word shift amount: 0 if aligned else VL_EDATASIZE - Low word shift
            Ast_ગાંઠ_પદાવલિ* અચલ edsp = નવું AstConst{flp, VL_EDATASIZE};
            Ast_ગાંઠ_પદાવલિ* અચલ subp = નવું AstSub{flp, edsp, loShftp->clone_વૃક્ષ_Pure(ખોટું)};
            hiShftp = નવું AstCond{flp, alignedp, નવું AstConst{flp, 0}, subp};
            AstVar* અચલ hiTmpp = addLocalTmp(nodep, "ExpandSel_HiShift", hiShftp);
            hiShftp = નવું AstVarRef{flp, hiTmpp, VAccess::READ};

            // Compute the High word mask: 0 if aligned, ones otherwise
            hiMaskp = નવું AstCond{flp, alignedp->clone_વૃક્ષ_Pure(ખોટું),
                                  નવું AstConst{flp, AstConst::SizedEData{}, 0},
                                  નવું AstConst{flp, AstConst::SizedEData{}, -1ULL}};
            AstVar* અચલ maskTmpp = addLocalTmp(nodep, "ExpandSel_HiMask", hiMaskp);
            hiMaskp = નવું AstVarRef{flp, maskTmpp, VAccess::READ};
        }

        // Create each word of the selected result
        Ast_ગાંઠ_પદાવલિ* અચલ fromp = rhsp->fromp();
        અચલ પૂર્ણાંક sel_શબ્દો = nodep->width_શબ્દો();
        માટે (પૂર્ણાંક w = 0; w < sel_શબ્દો; ++w) {
            // Grab bits from word 'VL_BITWORD_E(lsb) + w'
            Ast_ગાંઠ_પદાવલિ* અચલ loWordp = newWordSelWord(fromp->fileline(), fromp, wordIdxp, w);
            Ast_ગાંઠ_પદાવલિ* અચલ loShftClonep = loShftp->clone_વૃક્ષ_Pure(ખોટું);
            Ast_ગાંઠ_પદાવલિ* અચલ lop = નવું AstShiftR{flp, loWordp, loShftClonep, VL_EDATASIZE};
            // Grab bits from word 'VL_BITWORD_E(lsb) + w + 1'
            Ast_ગાંઠ_પદાવલિ* hiWordp = newWordSelWord(fromp->fileline(), fromp, wordIdxp, w + 1);
            // For the last word of the result, avoid an OOB access when the Sel is not OOB
            જો (w == sel_શબ્દો - 1) {
                અચલ uint32_t max = fromp->width_શબ્દો() - w - 1;
                Ast_ગાંઠ_પદાવલિ* અચલ maxp = નવું AstConst{flp, max};
                Ast_ગાંઠ_પદાવલિ* અચલ condp = નવું AstGte{flp, wordIdxp->clone_વૃક્ષ_Pure(ખોટું), maxp};
                Ast_ગાંઠ_પદાવલિ* અચલ zerop = નવું AstConst{flp, AstConst::SizedEData{}, 0};
                hiWordp = નવું AstCond{flp, condp, zerop, hiWordp};
            }
            Ast_ગાંઠ_પદાવલિ* અચલ hiShftClonep = hiShftp->clone_વૃક્ષ_Pure(ખોટું);
            Ast_ગાંઠ_પદાવલિ* અચલ hiPartp = નવું AstShiftL{flp, hiWordp, hiShftClonep, VL_EDATASIZE};
            Ast_ગાંઠ_પદાવલિ* અચલ hip
                = hiMaskp ? નવું AstAnd{flp, hiPartp, hiMaskp->clone_વૃક્ષ_Pure(ખોટું)} : hiPartp;
            // Combine them
            addWordAssign(nodep, w, નવું AstOr{nodep->fileline(), hip, lop});
        }

        // Delete parts not captured during construction
        જો (!wordIdxp->backp()) VL_DO_DANGLING(wordIdxp->delete_વૃક્ષ(), wordIdxp);
        જો (!loShftp->backp()) VL_DO_DANGLING(loShftp->delete_વૃક્ષ(), loShftp);
        જો (!hiShftp->backp()) VL_DO_DANGLING(hiShftp->delete_વૃક્ષ(), hiShftp);
        જો (hiMaskp && !hiMaskp->backp()) VL_DO_DANGLING(hiMaskp->delete_વૃક્ષ(), hiMaskp);

        પાછા સાચું;
    }

    તાર્કિક expandLhs(Ast_ગાંઠ_Assign* nodep, AstSel* lhsp) {
        // Possibilities
        //      destp: wide or narrow
        //      rhsp:  wide (destp must be wide), narrow, or 1 bit wide
        //      rhsp:  may be allones and can remove AND NOT gate
        //      lsbp:  constant or variable
        // Yuk.
        જો (isImpure(nodep)) પાછા ખોટું;
        File_લીટી* અચલ nfl = nodep->fileline();
        File_લીટી* અચલ lfl = lhsp->fileline();
        અચલ તાર્કિક destwide = lhsp->fromp()->isWide();
        અચલ તાર્કિક ones = nodep->rhsp()->isAllOnesV();
        જો (VN_IS(lhsp->lsbp(), Const)) {
            // The code should work without this constant test, but it won't
            // constify as nicely as we'd like.
            Ast_ગાંઠ_પદાવલિ* rhsp = nodep->rhsp()->unlinkFrBack();
            Ast_ગાંઠ_પદાવલિ* અચલ destp = lhsp->fromp()->unlinkFrBack();
            અચલ પૂર્ણાંક lsb = lhsp->lsbConst();
            અચલ પૂર્ણાંક msb = lhsp->msbConst();
            V3_સંખ્યા maskset{nodep, destp->widthMin()};
            maskset.setMask(msb + 1 - lsb, lsb);
            V3_સંખ્યા maskold{nodep, destp->widthMin()};
            maskold.opNot(maskset);
            જો (destwide) {
                didPattern(VExpandPattern::ASSIGN_TO_SEL_CONST_WIDE, nodep);
                માટે (પૂર્ણાંક w = 0; w < destp->width_શબ્દો(); ++w) {
                    જો (w >= VL_BITWORD_E(lsb) && w <= VL_BITWORD_E(msb)) {
                        // else we would just be setting it to the same exact value
                        Ast_ગાંઠ_પદાવલિ* oldvalp = newAstWordSelClone(destp, w);
                        fixCloneLvalue(oldvalp);
                        જો (!ones) {
                            oldvalp = નવું AstAnd{
                                lfl,
                                નવું AstConst{lfl, AstConst::SizedEData{}, maskold.edataWord(w)},
                                oldvalp};
                        }

                        // Appropriate word of new value to insert:
                        Ast_ગાંઠ_પદાવલિ* newp = newWordGrabShift(lfl, w, rhsp, lsb);

                        // Apply cleaning at the top word of the destination
                        // (no cleaning to do if dst's width is a whole number
                        // of words).
                        જો (w == destp->width_શબ્દો() - 1 && VL_BITBIT_E(destp->widthMin()) != 0) {
                            V3_સંખ્યા cleanmask{nodep, VL_EDATASIZE};
                            cleanmask.setMask(VL_BITBIT_E(destp->widthMin()));
                            newp = નવું AstAnd{lfl, newp, નવું AstConst{lfl, cleanmask}};
                        }
                        Ast_ગાંઠ_પદાવલિ* અચલ orp
                            = V3Const::constifyEditCpp(નવું AstOr{lfl, oldvalp, newp});
                        addWordAssign(nodep, w, destp, orp);
                    }
                }
                VL_DO_DANGLING(rhsp->delete_વૃક્ષ(), rhsp);
                VL_DO_DANGLING(destp->delete_વૃક્ષ(), destp);
            } નહીંતર {
                didPattern(VExpandPattern::ASSIGN_TO_SEL_CONST_NARROW, nodep);
                જો (destp->isQuad() && !rhsp->isQuad()) rhsp = નવું AstCCast{nfl, rhsp, nodep};
                Ast_ગાંઠ_પદાવલિ* oldvalp = destp->clone_વૃક્ષ_Pure(સાચું);
                fixCloneLvalue(oldvalp);
                જો (!ones) oldvalp = નવું AstAnd{lfl, નવું AstConst{lfl, maskold}, oldvalp};

                // The bit-select can refer to bits outside the width of nodep
                // which we aren't allowed to assign to.  This is a mask of the
                // valid range of nodep which we apply to the new shifted RHS.
                V3_સંખ્યા cleanmask{nodep, destp->widthMin()};
                cleanmask.setMask(destp->widthMin());
                Ast_ગાંઠ_પદાવલિ* અચલ shifted = નવું AstShiftL{
                    lfl, rhsp, નવું AstConst{lfl, static_cast<uint32_t>(lsb)}, destp->width()};
                Ast_ગાંઠ_પદાવલિ* અચલ cleaned
                    = નવું AstAnd{lfl, shifted, નવું AstConst{lfl, cleanmask}};
                Ast_ગાંઠ_પદાવલિ* અચલ orp
                    = V3Const::constifyEditCpp(નવું AstOr{lfl, oldvalp, cleaned});
                ઉમેરો_Before(nodep, નવું AstAssign{nfl, destp, orp});
            }
            પાછા સાચું;
        } નહીંતર {  // non-const select offset
            જો (destwide && lhsp->widthConst() == 1) {
                didPattern(VExpandPattern::ASSIGN_TO_SEL_VAR_WIDE_ONE_BIT, nodep);
                Ast_ગાંઠ_પદાવલિ* અચલ rhsp = nodep->rhsp()->unlinkFrBack();
                Ast_ગાંઠ_પદાવલિ* અચલ destp = lhsp->fromp()->unlinkFrBack();
                Ast_ગાંઠ_પદાવલિ* oldvalp
                    = newWordSelBit(lfl, destp->clone_વૃક્ષ_Pure(ખોટું), lhsp->lsbp());
                fixCloneLvalue(oldvalp);
                જો (!ones) {
                    oldvalp = નવું AstAnd{
                        lfl,
                        નવું AstNot{
                            lfl, નવું AstShiftL{lfl, નવું AstConst{nfl, 1},
                                               // newSelBitBit may exceed the MSB of this variable.
                                               // That's ok as we'd just AND with a larger value,
                                               // but oldval would clip the upper bits to sanity
                                               newSelBitBit(lhsp->lsbp()), VL_EDATASIZE}},
                        oldvalp};
                }
                // Restrict the shift amount to 0-31, see bug804.
                Ast_ગાંઠ_પદાવલિ* અચલ shiftp = નવું AstAnd{nfl, lhsp->lsbp()->clone_વૃક્ષ_Pure(સાચું),
                                                       નવું AstConst{nfl, VL_EDATASIZE - 1}};
                Ast_ગાંઠ* અચલ newp = નવું AstAssign{
                    nfl, newWordSelBit(nfl, destp, lhsp->lsbp()),
                    નવું AstOr{lfl, oldvalp, નવું AstShiftL{lfl, rhsp, shiftp, VL_EDATASIZE}}};
                ઉમેરો_Before(nodep, newp);
                પાછા સાચું;
            } નહીંતર જો (destwide) {
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
                પાછા ખોટું;
            } નહીંતર {
                didPattern(VExpandPattern::ASSIGN_TO_SEL_VAR_NARROW, nodep);
                // nodep->dumpTree("-  old: ");
                Ast_ગાંઠ_પદાવલિ* rhsp = nodep->rhsp()->unlinkFrBack();
                Ast_ગાંઠ_પદાવલિ* અચલ destp = lhsp->fromp()->unlinkFrBack();
                Ast_ગાંઠ_પદાવલિ* oldvalp = destp->clone_વૃક્ષ_Pure(સાચું);
                fixCloneLvalue(oldvalp);

                V3_સંખ્યા maskwidth{nodep, destp->widthMin()};
                maskwidth.setMask(lhsp->widthConst());

                જો (destp->isQuad() && !rhsp->isQuad()) rhsp = નવું AstCCast{nfl, rhsp, nodep};
                જો (!ones) {
                    oldvalp = નવું AstAnd{
                        lfl,
                        નવું AstNot{lfl, નવું AstShiftL{lfl, નવું AstConst{nfl, maskwidth},
                                                      lhsp->lsbp()->clone_વૃક્ષ_Pure(સાચું),
                                                      destp->width()}},
                        oldvalp};
                }
                Ast_ગાંઠ_પદાવલિ* newp
                    = નવું AstShiftL{lfl, rhsp, lhsp->lsbp()->clone_વૃક્ષ_Pure(સાચું), destp->width()};
                // Apply cleaning to the new value being inserted.  Mask is
                // slightly wider than necessary to avoid an AND with all ones
                // being optimized out.  No need to clean if destp is
                // quad-sized as there are no extra bits to contaminate
                જો (destp->widthMin() != 64) {
                    V3_સંખ્યા cleanmask{nodep, destp->widthMin() + 1};
                    cleanmask.setMask(destp->widthMin());
                    newp = નવું AstAnd{lfl, newp, નવું AstConst{lfl, cleanmask}};
                }

                ઉમેરો_Before(nodep, નવું AstAssign{nfl, destp, નવું AstOr{lfl, oldvalp, newp}});
                પાછા સાચું;
            }
        }
    }

    શૂન્ય visit(AstConcat* nodep) override {
        જો (nodep->user1SetOnce()) પાછા;  // Process once
        iterateChildren(nodep);
        જો (nodep->isWide()) {
            // See under ASSIGN(WIDE)
        } નહીંતર {
            // No isImpure() check - can handle side effects in below
            didPattern(VExpandPattern::CONCAT, nodep);
            File_લીટી* અચલ fl = nodep->fileline();
            Ast_ગાંઠ_પદાવલિ* lhsp = nodep->lhsp()->unlinkFrBack();
            Ast_ગાંઠ_પદાવલિ* rhsp = nodep->rhsp()->unlinkFrBack();
            અચલ uint32_t rhsshift = rhsp->widthMin();
            જો (nodep->isQuad() && !lhsp->isQuad()) lhsp = નવું AstCCast{fl, lhsp, nodep};
            જો (nodep->isQuad() && !rhsp->isQuad()) rhsp = નવું AstCCast{fl, rhsp, nodep};
            Ast_ગાંઠ_પદાવલિ* અચલ newp = નવું AstOr{
                fl, નવું AstShiftL{fl, lhsp, નવું AstConst{fl, rhsshift}, nodep->width()}, rhsp};
            newp->dtypeFrom(nodep);  // Unsigned
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        }
    }
    તાર્કિક expandWide(Ast_ગાંઠ_Assign* nodep, AstConcat* rhsp) {
        જો (!doExpandWide(rhsp)) પાછા ખોટું;
        didPattern(VExpandPattern::ASSIGN_CONCAT, nodep);
        File_લીટી* અચલ fl = rhsp->fileline();
        // Lhs or Rhs may be word, long, or quad.
        // newAstWordSelClone nicely abstracts the difference.
        અચલ પૂર્ણાંક rhsshift = rhsp->rhsp()->widthMin();
        // Sometimes doing the words backwards is preferable.
        // When we have x={x,foo} backwards is better, when x={foo,x} forward is better
        // However V3Subst tends to rip this up, so not worth optimizing now.
        માટે (પૂર્ણાંક w = 0; w < rhsp->width_શબ્દો(); ++w) {
            addWordAssign(nodep, w,
                          નવું AstOr{fl, newWordGrabShift(fl, w, rhsp->lhsp(), rhsshift),
                                    newAstWordSelClone(rhsp->rhsp(), w)});
        }
        પાછા સાચું;
    }

    શૂન્ય visit(AstReplicate* nodep) override {
        જો (nodep->user1SetOnce()) પાછા;  // Process once
        iterateChildren(nodep);
        જો (nodep->isWide()) {
            // See under ASSIGN(WIDE)
        } નહીંતર {
            જો (isImpure(nodep)) પાછા;
            File_લીટી* અચલ fl = nodep->fileline();
            Ast_ગાંઠ_પદાવલિ* lhsp = nodep->srcp()->unlinkFrBack();
            Ast_ગાંઠ_પદાવલિ* newp;
            અચલ પૂર્ણાંક lhswidth = lhsp->widthMin();
            જો (lhswidth == 1) {
                didPattern(VExpandPattern::REPLICATE_ONE_BIT, nodep);
                newp = નવું AstNegate{fl, lhsp};
            } નહીંતર {
                didPattern(VExpandPattern::REPLICATE, nodep);
                અચલ AstConst* અચલ constp = VN_AS(nodep->countp(), Const);
                UASSERT_OBJ(constp, nodep,
                            "Replication value isn't a constant.  Checked earlier!");
                અચલ uint32_t times = constp->toUInt();
                જો (nodep->isQuad() && !lhsp->isQuad()) lhsp = નવું AstCCast{fl, lhsp, nodep};
                newp = lhsp->clone_વૃક્ષ_Pure(સાચું);
                માટે (unsigned repnum = 1; repnum < times; repnum++) {
                    અચલ પૂર્ણાંક rhsshift = repnum * lhswidth;
                    newp = નવું AstOr{
                        fl,
                        નવું AstShiftL{fl, lhsp->clone_વૃક્ષ_Pure(સાચું),
                                      નવું AstConst{fl, static_cast<uint32_t>(rhsshift)},
                                      nodep->width()},
                        newp};
                    newp->dtypeFrom(nodep);  // Unsigned
                }
                VL_DO_DANGLING(lhsp->delete_વૃક્ષ(), lhsp);  // Never used
            }
            newp->dtypeFrom(nodep);  // Unsigned
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        }
    }
    તાર્કિક expandWide(Ast_ગાંઠ_Assign* nodep, AstReplicate* rhsp) {
        જો (!doExpandWide(rhsp)) પાછા ખોટું;
        didPattern(VExpandPattern::ASSIGN_REPLICATE, nodep);
        File_લીટી* અચલ fl = nodep->fileline();
        Ast_ગાંઠ_પદાવલિ* અચલ lhsp = rhsp->srcp();
        અચલ પૂર્ણાંક lhswidth = lhsp->widthMin();
        અચલ AstConst* અચલ constp = VN_AS(rhsp->countp(), Const);
        UASSERT_OBJ(constp, rhsp, "Replication value isn't a constant.  Checked earlier!");
        અચલ uint32_t times = constp->toUInt();
        માટે (પૂર્ણાંક w = 0; w < rhsp->width_શબ્દો(); ++w) {
            Ast_ગાંઠ_પદાવલિ* newp;
            જો (lhswidth == 1) {
                newp = નવું AstNegate{fl, lhsp->clone_વૃક્ષ_Pure(સાચું)};
                // Replicate always unsigned
                newp->dtypeChgWidthSigned(VL_EDATASIZE, VL_EDATASIZE, VSigning::UNSIGNED);
            } નહીંતર {
                newp = newAstWordSelClone(lhsp, w);
                File_લીટી* અચલ rfl = rhsp->fileline();
                માટે (unsigned repnum = 1; repnum < times; repnum++) {
                    newp = નવું AstOr{fl, newWordGrabShift(rfl, w, lhsp, lhswidth * repnum), newp};
                }
            }
            addWordAssign(nodep, w, newp);
        }
        પાછા સાચું;
    }

    શૂન્ય visitEqNeq(Ast_ગાંઠ_Biop* nodep) {
        જો (nodep->user1SetOnce()) પાછા;  // Process once
        iterateChildren(nodep);
        જો (nodep->lhsp()->isWide()
            && !(VN_IS(nodep->lhsp()->dtypep()->skipRefp(), StreamDType)
                 || VN_IS(nodep->rhsp()->dtypep()->skipRefp(), StreamDType))) {
            જો (isImpure(nodep)) પાછા;
            જો (!doExpandWide(nodep->lhsp())) પાછા;
            જો (!doExpandWide(nodep->rhsp())) પાછા;
            didPattern(VN_IS(nodep, Neq) ? VExpandPattern::WORDIZE_NEQ
                                         : VExpandPattern::WORDIZE_EQ,
                       nodep);
            // -> (0=={or{for each_word{WORDSEL(lhs,#)^WORDSEL(rhs,#)}}}
            File_લીટી* અચલ fl = nodep->fileline();
            Ast_ગાંઠ_પદાવલિ* newp = nullptr;
            માટે (પૂર્ણાંક w = 0; w < nodep->lhsp()->width_શબ્દો(); ++w) {
                Ast_ગાંઠ_પદાવલિ* અચલ eqp = નવું AstXor{fl, newAstWordSelClone(nodep->lhsp(), w),
                                                    newAstWordSelClone(nodep->rhsp(), w)};
                newp = newp ? નવું AstOr{fl, newp, eqp} : eqp;
            }
            જો (VN_IS(nodep, Neq)) {
                newp = નવું AstNeq{fl, નવું AstConst{fl, AstConst::SizedEData{}, 0}, newp};
            } નહીંતર {
                newp = નવું AstEq{fl, નવું AstConst{fl, AstConst::SizedEData{}, 0}, newp};
            }
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        }
    }
    શૂન્ય visit(AstEq* nodep) override { visitEqNeq(nodep); }
    શૂન્ય visit(AstNeq* nodep) override { visitEqNeq(nodep); }

    શૂન્ય visit(AstRedOr* nodep) override {
        જો (nodep->user1SetOnce()) પાછા;  // Process once
        iterateChildren(nodep);
        File_લીટી* અચલ fl = nodep->fileline();
        જો (nodep->lhsp()->isWide()) {
            જો (isImpure(nodep)) પાછા;
            જો (!doExpandWide(nodep->lhsp())) પાછા;
            didPattern(VExpandPattern::WORDIZE_REDOR, nodep);
            // -> (0!={or{for each_word{WORDSEL(lhs,#)}}}
            Ast_ગાંઠ_પદાવલિ* newp = nullptr;
            માટે (પૂર્ણાંક w = 0; w < nodep->lhsp()->width_શબ્દો(); ++w) {
                Ast_ગાંઠ_પદાવલિ* અચલ eqp = newAstWordSelClone(nodep->lhsp(), w);
                newp = newp ? નવું AstOr{fl, newp, eqp} : eqp;
            }
            newp = નવું AstNeq{fl, નવું AstConst{fl, AstConst::SizedEData{}, 0}, newp};
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        } નહીંતર {
            // No isImpure() check - can handle side effects in below
            didPattern(VExpandPattern::REDOR_TO_NEQ, nodep);
            Ast_ગાંઠ_પદાવલિ* અચલ lhsp = nodep->lhsp()->unlinkFrBack();
            Ast_ગાંઠ_પદાવલિ* અચલ newp = નવું AstNeq{
                fl, નવું AstConst{fl, AstConst::Widthed_કિંમત{}, longOrQuadWidth(nodep), 0}, lhsp};
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        }
    }
    શૂન્ય visit(AstRedAnd* nodep) override {
        જો (nodep->user1SetOnce()) પાછા;  // Process once
        iterateChildren(nodep);
        File_લીટી* અચલ fl = nodep->fileline();
        જો (nodep->lhsp()->isWide()) {
            જો (isImpure(nodep)) પાછા;
            જો (!doExpandWide(nodep->lhsp())) પાછા;
            didPattern(VExpandPattern::WORDIZE_REDAND, nodep);
            // -> (0!={and{for each_word{WORDSEL(lhs,#)}}}
            Ast_ગાંઠ_પદાવલિ* newp = nullptr;
            માટે (પૂર્ણાંક w = 0; w < nodep->lhsp()->width_શબ્દો(); ++w) {
                Ast_ગાંઠ_પદાવલિ* eqp = newAstWordSelClone(nodep->lhsp(), w);
                જો (w == nodep->lhsp()->width_શબ્દો() - 1) {
                    // Rather than doing a (slowish) ==##, we OR in the
                    // bits that aren't part of the mask
                    eqp = નવું AstOr{fl, નવું AstConst{fl, notWideMask(nodep->lhsp())},
                                    // Bug in cppcheck
                                    // cppcheck-suppress memleak
                                    eqp};
                }
                newp = newp ? નવું AstAnd{fl, newp, eqp} : eqp;
            }
            newp = નવું AstEq{fl, નવું AstConst{fl, AstConst::SizedEData{}, VL_MASK_E(VL_EDATASIZE)},
                             newp};
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        } નહીંતર {
            // No isImpure() check - can handle side effects in below
            didPattern(VExpandPattern::REDAND_TO_EQ, nodep);
            Ast_ગાંઠ_પદાવલિ* અચલ lhsp = nodep->lhsp()->unlinkFrBack();
            Ast_ગાંઠ_પદાવલિ* અચલ newp = નવું AstEq{fl, નવું AstConst{fl, wordMask(lhsp)}, lhsp};
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        }
    }
    શૂન્ય visit(AstRedXor* nodep) override {
        જો (nodep->user1SetOnce()) પાછા;  // Process once
        iterateChildren(nodep);
        જો (nodep->lhsp()->isWide()) {
            જો (isImpure(nodep)) પાછા;
            જો (!doExpandWide(nodep->lhsp())) પાછા;
            didPattern(VExpandPattern::WORDIZE_REDXOR, nodep);
            // -> (0!={redxor{for each_word{XOR(WORDSEL(lhs,#))}}}
            File_લીટી* અચલ fl = nodep->fileline();
            Ast_ગાંઠ_પદાવલિ* newp = nullptr;
            માટે (પૂર્ણાંક w = 0; w < nodep->lhsp()->width_શબ્દો(); ++w) {
                Ast_ગાંઠ_પદાવલિ* અચલ eqp = newAstWordSelClone(nodep->lhsp(), w);
                newp = newp ? નવું AstXor{fl, newp, eqp} : eqp;
            }
            newp = નવું AstRedXor{fl, newp};
            UINFO(8, "    Wordize REDXORnew " << newp);
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        }
        // We don't reduce non-wide XORs, as its more efficient to use a temp register,
        // which the inlined function does nicely.
    }

    શૂન્ય visit(Ast_ગાંઠ_Stmt* nodep) override {
        જો (nodep->user1SetOnce()) પાછા;  // Process once
        VL_RESTORER(m_stmtp);
        m_stmtp = nodep;
        iterateChildren(nodep);
    }
    શૂન્ય visit(Ast_ગાંઠ_Assign* nodep) override {
        જો (nodep->user1SetOnce()) પાછા;  // Process once
        જો (VN_IS(nodep->dtypep()->skipRefp(), UnpackArrayDType)) {
            પાછા;  // Skip for UnpackArrayDType
        }

        VL_RESTORER(m_stmtp);
        m_stmtp = nodep;
        iterateChildren(nodep);
        તાર્કિક did = ખોટું;
        જો (nodep->isWide()  //
            && ((VN_IS(nodep->lhsp(), VarRef) || VN_IS(nodep->lhsp(), ArraySel)))
            && !AstVar::scVarRecurse(nodep->lhsp())  // Need special function for SC
            && !AstVar::scVarRecurse(nodep->rhsp())) {
            જો (AstConst* અચલ rhsp = VN_CAST(nodep->rhsp(), Const)) {
                did = expandWide(nodep, rhsp);
            } નહીંતર જો (AstVarRef* અચલ rhsp = VN_CAST(nodep->rhsp(), VarRef)) {
                did = expandWide(nodep, rhsp);
            } નહીંતર જો (AstSel* અચલ rhsp = VN_CAST(nodep->rhsp(), Sel)) {
                did = expandWide(nodep, rhsp);
            } નહીંતર જો (AstArraySel* અચલ rhsp = VN_CAST(nodep->rhsp(), ArraySel)) {
                did = expandWide(nodep, rhsp);
            } નહીંતર જો (AstConcat* અચલ rhsp = VN_CAST(nodep->rhsp(), Concat)) {
                did = expandWide(nodep, rhsp);
            } નહીંતર જો (AstExtend* અચલ rhsp = VN_CAST(nodep->rhsp(), Extend)) {
                did = expandWide(nodep, rhsp);
            } નહીંતર જો (AstReplicate* અચલ rhsp = VN_CAST(nodep->rhsp(), Replicate)) {
                did = expandWide(nodep, rhsp);
            } નહીંતર જો (AstAnd* અચલ rhsp = VN_CAST(nodep->rhsp(), And)) {
                did = expandWide(nodep, rhsp);
            } નહીંતર જો (AstOr* અચલ rhsp = VN_CAST(nodep->rhsp(), Or)) {
                did = expandWide(nodep, rhsp);
            } નહીંતર જો (AstNot* અચલ rhsp = VN_CAST(nodep->rhsp(), Not)) {
                did = expandWide(nodep, rhsp);
            } નહીંતર જો (AstXor* અચલ rhsp = VN_CAST(nodep->rhsp(), Xor)) {
                did = expandWide(nodep, rhsp);
            } નહીંતર જો (AstShiftL* અચલ rhsp = VN_CAST(nodep->rhsp(), ShiftL)) {
                did = expandWideShift(nodep, rhsp, /* isLeftShift: */ સાચું);
            } નહીંતર જો (AstShiftR* અચલ rhsp = VN_CAST(nodep->rhsp(), ShiftR)) {
                did = expandWideShift(nodep, rhsp, /* isLeftShift: */ ખોટું);
            } નહીંતર જો (AstCond* અચલ rhsp = VN_CAST(nodep->rhsp(), Cond)) {
                did = expandWide(nodep, rhsp);
            }
        } નહીંતર જો (AstSel* અચલ lhsp = VN_CAST(nodep->lhsp(), Sel)) {
            did = expandLhs(nodep, lhsp);
        }
        // Cleanup common code
        જો (did) VL_DO_DANGLING(nodep->unlinkFrBack()->delete_વૃક્ષ(), nodep);
    }

    //--------------------
    શૂન્ય visit(AstVar*) override {}  // Don't hit varrefs under vars
    શૂન્ય visit(Ast_ગાંઠ* nodep) override { iterateChildren(nodep); }

જાહેર:
    // CONSTRUCTORS
    explicit ExpandVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~ExpandVisitor() override {
        V3Stats::addStat("Optimizations, Expand, expanded wides", m_statWides);
        V3Stats::addStat("Optimizations, Expand, expanded wide words", m_stat_Wide_શબ્દો);
        V3Stats::addStat("Optimizations, Expand, not expanded impure", m_statImpure);
        V3Stats::addStat("Optimizations, Expand, not expanded limit", m_statWideLimited);
        V3Stats::addStat("Optimizations, Expand, temporaries created", m_statTemps);
        અચલ auto emitStat = [આ](VExpandPattern pattern) {
            std::string str{pattern.ascii()};
            std::transform(str.begin(), str.end(), str.begin(), [](unsigned અક્ષર c) {  //
                પાછા c == '_' ? ' ' : std::tolower(c);
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

શૂન્ય V3Expand::expandAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        અચલ ExpandOkVisitor okVisitor{nodep};
        ExpandVisitor{nodep};
    }  // Destruct before checking
    V3Global::dump_Check_Global_વૃક્ષ("expand", 0, dump_વૃક્ષ_Either_Level() >= 3);
}
