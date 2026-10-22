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

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "V3Expand.h"

#ضمن "V3Const.h"
#ضمن "V3Stats.h"

#ضمن <algorithm>
#ضمن <array>

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

صنف VExpandPattern final {
عام:
    تعداد en : uint8_t {
#define EXPAND_PATTERN_ID(id) id,
        FOR_EACH_EXPAND_PATTERN(EXPAND_PATTERN_ID)
#undef EXPAND_PATTERN_ID
            _ENUM_END
    };
    تعداد en m_e;
    ثابت حرف* ascii() ثابت {
        ساكن ثابت حرف* ثابت names[] = {
#define EXPAND_PATTERN_NAME(id) #id,
            FOR_EACH_EXPAND_PATTERN(EXPAND_PATTERN_NAME)
#undef EXPAND_PATTERN_NAME
                "_ENUM_END"  //
        };
        أرجع names[m_e];
    }

    // cppcheck-suppress noExplicitConstructor
    VExpandPattern(en _e)
        : m_e{_e} {}
    operator en() ثابت { أرجع m_e; }
};

//######################################################################
// Find nodes with side effects, to mark as non-expandable

صنف ExpandOkVisitor final : عام VNVisitorConst {
    // NODE STATE
    //  AstNode::user2()        -> bool.  Is pure (along with all children)
    ثابت VNUser2InUse m_inuser2;

    // STATE - for current visit position (use VL_RESTORER)
    // Tracks similar to AstNode::isTreePureRecurse(), but avoid O(n^2)
    // False = pure, as nodes that ExpandVisitor inserts preserve pureness
    منطقي m_isImpure = true;  // Currently pure

    فراغ visit(Ast_گرێ* nodep) override {
        منطقي selfImpure = !nodep->isPure();
        {
            VL_RESTORER(m_isImpure);
            m_isImpure = خطأ;
            iterateChildrenConst(nodep);
            selfImpure |= m_isImpure;
            nodep->user2(selfImpure);
        }
        m_isImpure |= selfImpure;
    }

عام:
    // CONSTRUCTORS
    explicit ExpandOkVisitor(AstNetlist* nodep) { iterateConst(nodep); }
    ~ExpandOkVisitor() = افتراضي;
};

//######################################################################
// Expand state, as a visitor of each AstNode

صنف ExpandVisitor final : عام VNVisitor {
    // NODE STATE
    //  AstNode::user1()        -> bool.  Processed
    //  AstNode::user2()        -> See ExpandOkVisitor
    ثابت VNUser1InUse m_inuser1;

    // STATE - for current visit position (use VL_RESTORER)
    Ast_گرێ* m_stmtp = nullptr;  // Current statement

    // STATE - across all visitors
    AstCFunc* m_funcp = nullptr;  // Current function
    VDouble0 m_statImpure;  // Statistic tracking
    VDouble0 m_statWides;  // Statistic tracking
    VDouble0 m_stat_Wide_وشەکان;  // Statistic tracking
    VDouble0 m_statWideLimited;  // Statistic tracking
    VDouble0 m_statTemps;  // Statistic tracking
    // Number of applications of each expansion pattern for statistics
    std::array<VDouble0, VExpandPattern::_ENUM_END> m_statPatterns;

    // STATE - for current function
    size_t m_nTmps = 0;  // Sequence numbers for temopraries

    // METHODS
    // Note the application of the given expansion pattern to 'nodep'
    فراغ didPattern(VExpandPattern pattern, Ast_گرێ* nodep) {
        UINFO(8, "    " << pattern.ascii() << " " << nodep);
        ++m_statPatterns[pattern];
    }

    // Use state that ExpandOkVisitor calculated
    ساكن منطقي isImpure(Ast_گرێ* nodep) {
        ثابت منطقي impure = nodep->user2();
        إذا (impure) UINFO(9, "      impure " << nodep);
        أرجع impure;
    }

    منطقي doExpandWide(Ast_گرێ* nodep) {
        إذا (isImpure(nodep)) {
            ++m_statImpure;
            أرجع خطأ;
        }
        إذا (nodep->width_وشەکان() <= v3Global.opt.expandLimit()) {
            ++m_statWides;
            m_stat_Wide_وشەکان += nodep->width_وشەکان();
            أرجع true;
        } وإلا {
            ++m_statWideLimited;
            أرجع خطأ;
        }
    }

    ساكن صحيح longOrQuadWidth(Ast_گرێ* nodep) {
        أرجع (nodep->width() + (VL_EDATASIZE - 1)) & ~(VL_EDATASIZE - 1);
    }
    ساكن V3_ژمار notWideMask(Ast_گرێ* nodep) {
        أرجع V3_ژمار{nodep, VL_EDATASIZE, ~VL_MASK_E(nodep->widthMin())};
    }
    ساكن V3_ژمار wordMask(Ast_گرێ* nodep) {
        إذا (nodep->isWide()) {
            أرجع V3_ژمار{nodep, VL_EDATASIZE, VL_MASK_E(nodep->widthMin())};
        } وإلا {
            V3_ژمار mask{nodep, longOrQuadWidth(nodep)};
            mask.setMask(nodep->widthMin());
            أرجع mask;
        }
    }

    ساكن فراغ زیادکردن_Before(Ast_گرێ* placep, Ast_گرێ* newp) {
        newp->user1(1);  // Already processed, don't need to re-iterate
        placep->addHereThisAsNext(newp);
    }
    ساكن فراغ replaceWithDelete(Ast_گرێ* nodep, Ast_گرێ* newp) {
        newp->user1(1);  // Already processed, don't need to re-iterate
        إذا (newp->width() != nodep->width()) {
            UASSERT_OBJ(newp->widthMin() == nodep->widthMin(), nodep,
                        "Replacement width mismatch");
            newp->dtypeChgWidth(nodep->width(), nodep->widthMin());
        }
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_درەخت(), nodep);
    }
    ساكن Ast_گرێ* newWordAssign(Ast_گرێ_Assign* placep, صحيح word, Ast_گرێ_دەربڕین* lhsp,
                                  Ast_گرێ_دەربڕین* rhsp) {
        File_هێڵ* ثابت fl = placep->fileline();
        أرجع جديد AstAssign{fl,
                             جديد AstWordSel{fl, lhsp->clone_درەخت_Pure(true),
                                            جديد AstConst{fl, static_cast<uint32_t>(word)}},
                             rhsp};
    }
    ساكن فراغ addWordAssign(Ast_گرێ_Assign* placep, صحيح word, Ast_گرێ_دەربڕین* lhsp,
                              Ast_گرێ_دەربڕین* rhsp) {
        زیادکردن_Before(placep, newWordAssign(placep, word, lhsp, rhsp));
    }
    ساكن فراغ addWordAssign(Ast_گرێ_Assign* placep, صحيح word, Ast_گرێ_دەربڕین* rhsp) {
        addWordAssign(placep, word, placep->lhsp(), rhsp);
    }

    AstVar* addLocalTmp(Ast_گرێ* placep, ثابت حرف* namep, Ast_گرێ_دەربڕین* valuep) {
        ثابت std::string name = "__V"s + namep + "_"s + std::to_string(m_nTmps);
        File_هێڵ* ثابت flp = placep->fileline();
        AstVar* ثابت tmpp = جديد AstVar{flp, VVarType::STMTTEMP, name, valuep->dtypep()};
        tmpp->funcLocal(true);
        tmpp->isInternal(true);
        tmpp->noReset(true);
        tmpp->substConstOnly(true);
        m_funcp->addVarsp(tmpp);
        زیادکردن_Before(placep, جديد AstAssign{flp, جديد AstVarRef{flp, tmpp, VAccess::WRITE}, valuep});
        ++m_statTemps;
        أرجع tmpp;
    }

    ساكن فراغ fixCloneLvalue(Ast_گرێ* nodep) {
        // In AstSel transforms, we call clone() on VarRefs that were lvalues,
        // but are now being used on the RHS of the assignment
        إذا (VN_IS(nodep, VarRef)) VN_AS(nodep, VarRef)->access(VAccess::READ);
        // Iterate
        إذا (Ast_گرێ* ثابت refp = nodep->op1p()) fixCloneLvalue(refp);
        إذا (Ast_گرێ* ثابت refp = nodep->op2p()) fixCloneLvalue(refp);
        إذا (Ast_گرێ* ثابت refp = nodep->op3p()) fixCloneLvalue(refp);
        إذا (Ast_گرێ* ثابت refp = nodep->op4p()) fixCloneLvalue(refp);
    }

    ساكن Ast_گرێ_دەربڕین* newAstWordSelClone(Ast_گرێ_دەربڕین* nodep, صحيح word) {
        // Get the specified word number from a wide array
        // Or, if it's a long/quad, do appropriate conversion to wide
        // Concat may pass negative word numbers, that means it wants a zero
        File_هێڵ* ثابت fl = nodep->fileline();
        إذا (nodep->isWide() && word >= 0 && word < nodep->width_وشەکان()) {
            أرجع جديد AstWordSel{fl, nodep->clone_درەخت_Pure(true),
                                  جديد AstConst{fl, static_cast<uint32_t>(word)}};
        } وإلا إذا (nodep->isQuad() && word == 0) {
            Ast_گرێ_دەربڕین* ثابت quadfromp = nodep->clone_درەخت_Pure(true);
            quadfromp->dtypeSetBitUnsized(VL_QUADSIZE, quadfromp->widthMin(), VSigning::UNSIGNED);
            أرجع جديد AstCCast{fl, quadfromp, VL_EDATASIZE};
        } وإلا إذا (nodep->isQuad() && word == 1) {
            Ast_گرێ_دەربڕین* ثابت quadfromp = nodep->clone_درەخت_Pure(true);
            quadfromp->dtypeSetBitUnsized(VL_QUADSIZE, quadfromp->widthMin(), VSigning::UNSIGNED);
            أرجع جديد AstCCast{
                fl, جديد AstShiftR{fl, quadfromp, جديد AstConst{fl, VL_EDATASIZE}, VL_EDATASIZE},
                VL_EDATASIZE};
        } وإلا إذا (!nodep->isWide() && !nodep->isQuad() && word == 0) {
            أرجع nodep->clone_درەخت_Pure(true);
        } وإلا {  // Out of bounds
            أرجع جديد AstConst{fl, 0};
        }
    }

    ساكن Ast_گرێ_دەربڕین* newWordGrabShift(File_هێڵ* fl, صحيح word, Ast_گرێ_دەربڕین* lhsp, صحيح shift) {
        // Extract the expression to grab the value for the specified word, if it's the shift
        // of shift bits from lhsp
        // Negative word numbers requested for lhs when it's "before" what we want.
        // We get a 0 then.
        ثابت صحيح othword = word - shift / VL_EDATASIZE;
        Ast_گرێ_دەربڕین* ثابت lop = newAstWordSelClone(lhsp, othword);
        ثابت uint32_t loShift = VL_BITBIT_E(shift);
        إذا (!loShift) أرجع lop;
        Ast_گرێ_دەربڕین* ثابت hip = newAstWordSelClone(lhsp, othword - 1);
        ثابت uint32_t hiShift = VL_EDATASIZE - loShift;  // Complement offset
        Ast_گرێ_دەربڕین* ثابت newp
            = جديد AstOr{fl,  //
                        جديد AstShiftR{fl, hip, جديد AstConst{fl, hiShift}, VL_EDATASIZE},
                        جديد AstShiftL{fl, lop, جديد AstConst{fl, loShift}, VL_EDATASIZE}};
        أرجع V3Const::constifyEditCpp(newp);
    }

    // Return expression indexing the word that contains 'lsbp' + the given word offset
    ساكن Ast_گرێ_دەربڕین* newWordIndex(Ast_گرێ_دەربڕین* lsbp, uint32_t wordOffset = 0) {
        // This is indexing a WordSel, so a 32 bit constants are fine
        File_هێڵ* ثابت flp = lsbp->fileline();
        إذا (ثابت AstConst* constp = VN_CAST(lsbp, Const)) {
            أرجع جديد AstConst{flp, wordOffset + VL_BITWORD_E(constp->toUInt())};
        }

        إذا (lsbp->backp()) lsbp = lsbp->clone_درەخت_Pure(خطأ);
        Ast_گرێ_دەربڕین* ثابت wwl2p = جديد AstConst{flp, VL_EDATASIZE_LOG2};  // Word width log 2
        Ast_گرێ_دەربڕین* indexp = جديد AstShiftR{flp, lsbp, wwl2p, VL_EDATASIZE};
        إذا (wordOffset) indexp = جديد AstAdd{flp, جديد AstConst{flp, wordOffset}, indexp};
        أرجع indexp;
    }

    // Return word of fromp that contains word indexp + the given word offset.
    ساكن Ast_گرێ_دەربڕین* newWordSelWord(File_هێڵ* flp, Ast_گرێ_دەربڕین* fromp, Ast_گرێ_دەربڕین* indexp,
                                       uint32_t wordOffset = 0) {
        UASSERT_OBJ(fromp->isWide(), fromp, "Only need AstWordSel on wide from's");

        إذا (ثابت AstConst* ثابت constp = VN_CAST(indexp, Const)) {
            indexp = nullptr;
            wordOffset += constp->toUInt();
        }

        // e.g. "logic [95:0] var[0]; logic [0] sel; out = var[sel];"
        // Squash before C++ to avoid getting a C++ compiler warning
        // (even though code would be unreachable as presumably a
        // AstCond is protecting above this node.
        إذا (wordOffset >= static_cast<uint32_t>(fromp->width_وشەکان())) {
            أرجع جديد AstConst{flp, AstConst::SizedEData{}, 0};
        }

        إذا (fromp->backp()) fromp = fromp->clone_درەخت_Pure(خطأ);

        // If indexp was constant, just use it
        إذا (!indexp) أرجع جديد AstWordSel{flp, fromp, جديد AstConst{flp, wordOffset}};

        // Otherwise compute at runtime
        إذا (indexp->backp()) indexp = indexp->clone_درەخت_Pure(خطأ);
        إذا (wordOffset) indexp = جديد AstAdd{flp, جديد AstConst{flp, wordOffset}, indexp};
        أرجع جديد AstWordSel{flp, fromp, indexp};
    }

    // Return word of fromp that contains bit lsbp + the given word offset.
    ساكن Ast_گرێ_دەربڕین* newWordSelBit(File_هێڵ* flp, Ast_گرێ_دەربڕین* fromp, Ast_گرێ_دەربڕین* lsbp,
                                      uint32_t wordOffset = 0) {
        Ast_گرێ_دەربڕین* ثابت indexp = newWordIndex(lsbp, wordOffset);
        Ast_گرێ_دەربڕین* ثابت wordSelp = newWordSelWord(flp, fromp, indexp);
        إذا (!indexp->backp()) VL_DO_DANGLING(indexp->delete_درەخت(), indexp);
        أرجع wordSelp;
    }

    ساكن Ast_گرێ_دەربڕین* newSelBitBit(Ast_گرێ_دەربڕین* lsbp) {
        // Return equation to get the VL_BITBIT of a constant or non-constant
        File_هێڵ* ثابت fl = lsbp->fileline();
        إذا (VN_IS(lsbp, Const)) {
            أرجع جديد AstConst{fl, VL_BITBIT_E(VN_AS(lsbp, Const)->toUInt())};
        } وإلا {
            أرجع جديد AstAnd{fl, جديد AstConst{fl, VL_EDATASIZE - 1}, lsbp->clone_درەخت_Pure(true)};
        }
    }

    //====================

    منطقي expandWide(Ast_گرێ_Assign* nodep, AstConst* rhsp) {
        إذا (!doExpandWide(nodep)) أرجع خطأ;
        didPattern(VExpandPattern::ASSIGN_CONST, nodep);
        // -> {for each_word{ ASSIGN(WORDSEL(wide,#),WORDSEL(CONST,#))}}
        إذا (rhsp->ژمارە().isFourState()) {
            rhsp->v3warn(E_UNSUPPORTED,  // LCOV_EXCL_LINE  // impossible?
                         "Unsupported: 4-state numbers in this context");
        }
        File_هێڵ* ثابت fl = nodep->fileline();
        لكل (صحيح w = 0; w < nodep->width_وشەکان(); ++w) {
            addWordAssign(nodep, w,
                          جديد AstConst{fl, AstConst::SizedEData{}, rhsp->ژمارە().edataWord(w)});
        }
        أرجع true;
    }
    //-------- Uniops
    منطقي expandWide(Ast_گرێ_Assign* nodep, AstVarRef* rhsp) {
        // Special case: do not expand assignment of constant pool variables.
        // V3Subst undestands these directly.
        إذا (rhsp->varp()->constPoolEntry()) أرجع خطأ;
        إذا (!doExpandWide(nodep)) أرجع خطأ;
        didPattern(VExpandPattern::ASSIGN_VARREF, nodep);
        لكل (صحيح w = 0; w < nodep->width_وشەکان(); ++w) {
            addWordAssign(nodep, w, newAstWordSelClone(rhsp, w));
        }
        أرجع true;
    }
    منطقي expandWide(Ast_گرێ_Assign* nodep, AstArraySel* rhsp) {
        UASSERT_OBJ(!VN_IS(nodep->dtypep()->skipRefp(), UnpackArrayDType), nodep,
                    "ArraySel with unpacked arrays should have been removed in V3Slice");
        إذا (!doExpandWide(nodep)) أرجع خطأ;
        didPattern(VExpandPattern::ASSIGN_ARRAYSEL, nodep);
        لكل (صحيح w = 0; w < nodep->width_وشەکان(); ++w) {
            addWordAssign(nodep, w, newAstWordSelClone(rhsp, w));
        }
        أرجع true;
    }
    منطقي expandWide(Ast_گرێ_Assign* nodep, AstNot* rhsp) {
        // -> {for each_word{ ASSIGN(WORDSEL(wide,#),NOT(WORDSEL(lhs,#))) }}
        إذا (!doExpandWide(nodep)) أرجع خطأ;
        didPattern(VExpandPattern::ASSIGN_NOT, nodep);
        File_هێڵ* ثابت fl = rhsp->fileline();
        لكل (صحيح w = 0; w < nodep->width_وشەکان(); ++w) {
            addWordAssign(nodep, w, جديد AstNot{fl, newAstWordSelClone(rhsp->lhsp(), w)});
        }
        أرجع true;
    }
    //-------- Biops
    منطقي expandWide(Ast_گرێ_Assign* nodep, AstAnd* rhsp) {
        إذا (!doExpandWide(nodep)) أرجع خطأ;
        didPattern(VExpandPattern::ASSIGN_AND, nodep);
        File_هێڵ* ثابت fl = nodep->fileline();
        لكل (صحيح w = 0; w < nodep->width_وشەکان(); ++w) {
            addWordAssign(nodep, w,
                          جديد AstAnd{fl, newAstWordSelClone(rhsp->lhsp(), w),
                                     newAstWordSelClone(rhsp->rhsp(), w)});
        }
        أرجع true;
    }
    منطقي expandWide(Ast_گرێ_Assign* nodep, AstOr* rhsp) {
        إذا (!doExpandWide(nodep)) أرجع خطأ;
        didPattern(VExpandPattern::ASSIGN_OR, nodep);
        File_هێڵ* ثابت fl = nodep->fileline();
        لكل (صحيح w = 0; w < nodep->width_وشەکان(); ++w) {
            addWordAssign(nodep, w,
                          جديد AstOr{fl, newAstWordSelClone(rhsp->lhsp(), w),
                                    newAstWordSelClone(rhsp->rhsp(), w)});
        }
        أرجع true;
    }
    منطقي expandWide(Ast_گرێ_Assign* nodep, AstXor* rhsp) {
        إذا (!doExpandWide(nodep)) أرجع خطأ;
        didPattern(VExpandPattern::ASSIGN_XOR, nodep);
        File_هێڵ* ثابت fl = nodep->fileline();
        لكل (صحيح w = 0; w < nodep->width_وشەکان(); ++w) {
            addWordAssign(nodep, w,
                          جديد AstXor{fl, newAstWordSelClone(rhsp->lhsp(), w),
                                     newAstWordSelClone(rhsp->rhsp(), w)});
        }
        أرجع true;
    }
    منطقي expandWideShift(Ast_گرێ_Assign* nodep, Ast_گرێ_Biop* rhsp, منطقي is_چەپ_Shift) {
        إذا (!doExpandWide(nodep)) أرجع خطأ;

        // Simplify the shift amount, in case it becomes a constant
        V3Const::constifyEditCpp(rhsp->rhsp());

        // If it's a constant shift by whole words, expand it so V3Subst can substitute it
        إذا (ثابت AstConst* ثابت rhsConstp = VN_CAST(rhsp->rhsp(), Const)) {
            ثابت uint32_t shiftBits = rhsConstp->toUInt();
            إذا (VL_BITBIT_E(shiftBits) == 0) {
                ثابت صحيح width_وشەکان = nodep->width_وشەکان();
                ثابت صحيح shift_وشەکان = std::min<صحيح>(VL_BITWORD_E(shiftBits), width_وشەکان);
                File_هێڵ* ثابت flp = rhsp->fileline();
                إذا (is_چەپ_Shift) {
                    didPattern(VExpandPattern::ASSIGN_SHIFTL, nodep);
                    // Low words of the result are zero
                    لكل (صحيح w = 0; w < shift_وشەکان; ++w) {
                        addWordAssign(nodep, w, جديد AstConst{flp, AstConst::SizedEData{}, 0});
                    }
                    // High words of the result are copied from higher words of the source
                    لكل (صحيح w = shift_وشەکان; w < width_وشەکان; ++w) {
                        addWordAssign(nodep, w, newAstWordSelClone(rhsp->lhsp(), w - shift_وشەکان));
                    }
                } وإلا {
                    didPattern(VExpandPattern::ASSIGN_SHIFTR, nodep);
                    // Low words of the result are copied from higher words of the source
                    لكل (صحيح w = 0; w < width_وشەکان - shift_وشەکان; ++w) {
                        addWordAssign(nodep, w, newAstWordSelClone(rhsp->lhsp(), w + shift_وشەکان));
                    }
                    // High words of the result are zero
                    لكل (صحيح w = width_وشەکان - shift_وشەکان; w < width_وشەکان; ++w) {
                        addWordAssign(nodep, w, جديد AstConst{flp, AstConst::SizedEData{}, 0});
                    }
                }
                أرجع true;
            }
        }

        أرجع خطأ;
    }

    //-------- Triops
    منطقي expandWide(Ast_گرێ_Assign* nodep, AstCond* rhsp) {
        إذا (!doExpandWide(nodep)) أرجع خطأ;
        didPattern(VExpandPattern::ASSIGN_COND, nodep);
        File_هێڵ* ثابت fl = nodep->fileline();
        لكل (صحيح w = 0; w < nodep->width_وشەکان(); ++w) {
            addWordAssign(nodep, w,
                          جديد AstCond{fl, rhsp->condp()->clone_درەخت_Pure(true),
                                      newAstWordSelClone(rhsp->thenp(), w),
                                      newAstWordSelClone(rhsp->elsep(), w)});
        }
        أرجع true;
    }

    // VISITORS
    فراغ visit(AstNetlist* nodep) override { iterateAndNextNull(nodep->modulesp()); }

    فراغ visit(AstCFunc* nodep) override {
        VL_RESTORER(m_funcp);
        VL_RESTORER(m_nTmps);
        m_funcp = nodep;
        m_nTmps = 0;
        ثابت VDouble0 statWidesBefore = m_statWides;
        iterateChildren(nodep);

        // Constant fold here if anything was expanded, as Ast size can likely be reduced
        إذا (v3Global.opt.fConstEager() && m_statWides != statWidesBefore) {
            Ast_گرێ* ثابت editedp = V3Const::constifyEditCpp(nodep);
            UASSERT_OBJ(editedp == nodep, editedp, "Should not have replaced CFunc");
        }
    }

    فراغ visit(AstExtend* nodep) override {
        إذا (nodep->user1SetOnce()) أرجع;  // Process once
        iterateChildren(nodep);
        إذا (nodep->isWide()) {
            // See under ASSIGN(EXTEND)
        } وإلا {
            إذا (isImpure(nodep)) أرجع;
            Ast_گرێ_دەربڕین* ثابت lhsp = nodep->lhsp()->unlinkFrBack();
            Ast_گرێ_دەربڕین* newp = lhsp;
            إذا (nodep->isQuad()) {
                إذا (lhsp->isQuad()) {
                    lhsp->dtypeFrom(nodep);  // Just mark it, else nop
                } وإلا إذا (lhsp->isWide()) {
                    nodep->v3fatalSrc("extending larger thing into smaller?");
                } وإلا {
                    newp = جديد AstCCast{nodep->fileline(), lhsp, nodep};
                }
            } وإلا {  // Long
                UASSERT_OBJ(!(lhsp->isQuad() || lhsp->isWide()), nodep,
                            "extending larger thing into smaller?");
                lhsp->dtypeFrom(nodep);  // Just mark it, else nop
            }
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        }
    }
    منطقي expandWide(Ast_گرێ_Assign* nodep, AstExtend* rhsp) {
        إذا (!doExpandWide(nodep)) أرجع خطأ;
        didPattern(VExpandPattern::ASSIGN_EXTEND, nodep);
        Ast_گرێ_دەربڕین* ثابت rlhsp = rhsp->lhsp();
        لكل (صحيح w = 0; w < rlhsp->width_وشەکان(); ++w) {
            addWordAssign(nodep, w, newAstWordSelClone(rlhsp, w));
        }
        لكل (صحيح w = rlhsp->width_وشەکان(); w < nodep->width_وشەکان(); ++w) {
            addWordAssign(nodep, w, جديد AstConst{nodep->fileline(), AstConst::SizedEData{}, 0});
        }
        أرجع true;
    }

    فراغ visit(AstSel* nodep) override {
        إذا (nodep->user1SetOnce()) أرجع;  // Process once
        iterateChildren(nodep);

        ثابت uint32_t width = static_cast<uint32_t>(nodep->widthConst());
        UASSERT_OBJ(nodep->widthMin() == static_cast<صحيح>(width), nodep, "Width mismatch");

        // Skip if Sel is an LHS assignment select
        إذا (Ast_گرێ_Assign* ثابت assignp = VN_CAST(nodep->backp(), گرێ_Assign)) {
            إذا (nodep == assignp->lhsp()) أرجع;
        }

        // Skip if wide: See under ASSIGN(WIDE)
        إذا (nodep->isWide()) أرجع;

        // Skip Sel from stream or queue
        إذا (VN_IS(nodep->fromp()->dtypep(), StreamDType)) أرجع;
        إذا (VN_IS(nodep->fromp()->dtypep(), QueueDType)) أرجع;

        // Result must be non-wide after skipping all the above
        UASSERT_OBJ(width <= 64, nodep, "Inconsistent result width");

        إذا (nodep->fromp()->isWide()) {  // Long/Quad from Wide
            إذا (isImpure(nodep)) أرجع;
            didPattern(VExpandPattern::SEL_FROM_WIDE, nodep);
            File_هێڵ* ثابت nfl = nodep->fileline();
            File_هێڵ* ثابت lfl = nodep->lsbp()->fileline();
            File_هێڵ* ثابت ffl = nodep->fromp()->fileline();
            Ast_گرێ_دەربڕین* ثابت fromp = nodep->fromp();

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
            ثابت auto tryFold = [&](Ast_گرێ_دەربڕین* exprp) -> Ast_گرێ_دەربڕین* {
                // Size limit for inlining expressions (any arbitrary value results in
                // a bounded expansion. Picked a value to allow common simple forms.)
                ساكن constexpr صحيح EXPAND_SEL_LSB_LIMIT = 8;
                // Simplify
                exprp = V3Const::constifyEditCpp(exprp);
                // Accept if not larger than the limit
                إذا (!exprp->isLargerThan(EXPAND_SEL_LSB_LIMIT)) أرجع exprp;
                // Delete if rejected
                VL_DO_DANGLING(exprp->delete_درەخت(), exprp);
                أرجع nullptr;
            };

            // Return the index of the word holding the bit 'offset' bits above the LSB
            ثابت auto wordIdx = [&](uint32_t offset) -> Ast_گرێ_دەربڕین* {
                Ast_گرێ_دەربڕین* ثابت msbp = جديد AstAdd{lfl, جديد AstConst{lfl, offset},
                                                     nodep->lsbp()->clone_درەخت_Pure(خطأ)};
                Ast_گرێ_دەربڕین* ثابت idxp = newWordIndex(msbp);
                إذا (!msbp->backp()) VL_DO_DANGLING(msbp->delete_درەخت(), msbp);
                أرجع idxp;
            };

            // Bit index of the selected LSB within its word
            Ast_گرێ_دەربڕین* lBitp = tryFold(newSelBitBit(nodep->lsbp()));
            ثابت منطقي aligned = lBitp && lBitp->isZero();

            // Which of the other 2 words the expansion might need
            ثابت منطقي mNeeded = width > 1 && !aligned;
            ثابت منطقي hNeeded = width > VL_EDATASIZE;
            UASSERT_OBJ(!hNeeded || nodep->isQuad(), nodep, "Width mismatch");

            // Word indices
            ثابت uint32_t mMsbOffset = std::min<uint32_t>(width, VL_EDATASIZE) - 1;
            ثابت uint32_t hMsbOffset = width - 1;
            Ast_گرێ_دەربڕین* lIdxp = tryFold(newWordIndex(nodep->lsbp()));
            Ast_گرێ_دەربڕین* mIdxp = mNeeded ? tryFold(wordIdx(mMsbOffset)) : nullptr;
            Ast_گرێ_دەربڕین* hIdxp = hNeeded ? tryFold(wordIdx(hMsbOffset)) : nullptr;

            // If the LSB expression is needed multiple times, evaluate it into a temporary
            ثابت صحيح nCopies = !lBitp + !lIdxp + (mNeeded && !mIdxp) + (hNeeded && !hIdxp);
            إذا (nCopies > 1 && m_funcp && m_stmtp && !isImpure(m_stmtp)) {
                ++m_nTmps;  // Use fresh set of temporaries
                Ast_گرێ_دەربڕین* ثابت lsbp = nodep->lsbp()->unlinkFrBack();
                AstVar* ثابت tmpp = addLocalTmp(m_stmtp, "ExpandSel_Lsb", lsbp);
                nodep->lsbp(جديد AstVarRef{lfl, tmpp, VAccess::READ});
            }

            // Rebuild remaining expressions derived from LSB if needed, now using the temporary
            إذا (!lBitp) lBitp = newSelBitBit(nodep->lsbp());
            إذا (!lIdxp) lIdxp = newWordIndex(nodep->lsbp());
            إذا (!mIdxp) mIdxp = mNeeded ? wordIdx(mMsbOffset) : nullptr;
            إذا (!hIdxp) hIdxp = hNeeded ? wordIdx(hMsbOffset) : nullptr;

            // Return word 'idxp' of 'fromp', without consuming 'idxp'
            ثابت auto wordSel = [&](Ast_گرێ_دەربڕین* idxp) -> Ast_گرێ_دەربڕین* {
                Ast_گرێ_دەربڕین* ثابت clonep = idxp->clone_درەخت_Pure(خطأ);
                Ast_گرێ_دەربڕین* ثابت wordp = newWordSelWord(ffl, fromp, clonep);
                إذا (!clonep->backp()) VL_DO_DANGLING(clonep->delete_درەخت(), clonep);
                أرجع wordp;
            };

            // Construct term containing the bits of the low word - always needed
            Ast_گرێ_دەربڕین* ثابت lTermp = [&]() -> Ast_گرێ_دەربڕین* {
                Ast_گرێ_دەربڕین* lWordp = wordSel(lIdxp);
                إذا (nodep->isQuad()) lWordp = جديد AstCCast{nfl, lWordp, nodep};
                أرجع جديد AstShiftR{nfl, lWordp, lBitp->clone_درەخت_Pure(خطأ), nodep->width()};
            }();

            // Construct term containing the bits of the middle word - if needed
            Ast_گرێ_دەربڕین* ثابت mTermp = [&]() -> Ast_گرێ_دەربڕین* {
                إذا (!mNeeded) أرجع nullptr;

                Ast_گرێ_دەربڕین* mWordp = wordSel(mIdxp);
                إذا (nodep->isQuad()) mWordp = جديد AstCCast{nfl, mWordp, nodep};
                Ast_گرێ_دەربڕین* ثابت mShiftp = جديد AstSub{lfl, جديد AstConst{lfl, VL_EDATASIZE},
                                                        lBitp->clone_درەخت_Pure(خطأ)};
                // If we're selecting bit zero, then all 32 bits in the mid word
                // get shifted << by 32 bits, so ignore them.
                ثابت V3_ژمار zero{nodep, longOrQuadWidth(nodep)};
                أرجع جديد AstCond{
                    nfl,
                    // lsb % VL_EDATASIZE == 0 ?
                    جديد AstEq{nfl, جديد AstConst{nfl, 0}, lBitp->clone_درەخت_Pure(خطأ)},
                    // 0 :
                    جديد AstConst{nfl, zero},
                    //  midword >> (VL_EDATASIZE - (lbs % VL_EDATASIZE))
                    جديد AstShiftL{nfl, mWordp, mShiftp, nodep->width()}};
            }();

            // Construct term containing the bits of the high word - if needed
            Ast_گرێ_دەربڕین* ثابت hTermp = [&]() -> Ast_گرێ_دەربڕین* {
                إذا (!hNeeded) أرجع nullptr;

                Ast_گرێ_دەربڕین* hWordp = wordSel(hIdxp);
                hWordp = جديد AstCCast{nfl, hWordp, nodep};
                Ast_گرێ_دەربڕین* ثابت hShiftp = جديد AstCond{
                    nfl,
                    // lsb % VL_EDATASIZE == 0 ?
                    جديد AstEq{nfl, جديد AstConst{nfl, 0}, lBitp->clone_درەخت_Pure(خطأ)},
                    // VL_EDATASIZE :
                    جديد AstConst{lfl, VL_EDATASIZE},
                    // 64 - (lbs % VL_EDATASIZE)
                    جديد AstSub{lfl, جديد AstConst{lfl, 64}, lBitp->clone_درەخت_Pure(خطأ)}};
                أرجع جديد AstShiftL{nfl, hWordp, hShiftp, nodep->width()};
            }();

            // Delete parts not captured during construction
            VL_DO_DANGLING(lBitp->delete_درەخت(), lBitp);
            VL_DO_DANGLING(lIdxp->delete_درەخت(), lIdxp);
            إذا (mIdxp) VL_DO_DANGLING(mIdxp->delete_درەخت(), mIdxp);
            إذا (hIdxp) VL_DO_DANGLING(hIdxp->delete_درەخت(), hIdxp);

            // Or reduce the terms
            Ast_گرێ_دەربڕین* newp = lTermp;
            إذا (mTermp) newp = جديد AstOr{nfl, mTermp, newp};
            إذا (hTermp) newp = جديد AstOr{nfl, hTermp, newp};
            newp->dtypeFrom(nodep);
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        } وإلا {  // Long/Quad from Long/Quad
            // No isImpure() check - can handle side effects in below
            didPattern(VExpandPattern::SEL_FROM_NARROW, nodep);
            File_هێڵ* ثابت fl = nodep->fileline();
            Ast_گرێ_دەربڕین* fromp = nodep->fromp()->unlinkFrBack();
            Ast_گرێ_دەربڕین* ثابت lsbp = nodep->lsbp()->unlinkFrBack();
            إذا (nodep->isQuad() && !fromp->isQuad()) fromp = جديد AstCCast{fl, fromp, nodep};
            // {large}>>32 requires 64-bit shift operation; then cast
            Ast_گرێ_دەربڕین* newp = جديد AstShiftR{fl, fromp, lsbp, fromp->width()};
            newp->dtypeFrom(fromp);
            إذا (!nodep->isQuad() && fromp->isQuad()) newp = جديد AstCCast{fl, newp, nodep};
            newp->dtypeFrom(nodep);
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        }
    }

    منطقي expandWide(Ast_گرێ_Assign* nodep, AstSel* rhsp) {
        UASSERT_OBJ(nodep->widthMin() == rhsp->widthConst(), nodep, "Width mismatch");
        إذا (!doExpandWide(nodep)) أرجع خطأ;

        // Simplify the index, in case it becomes a constant
        V3Const::constifyEditCpp(rhsp->lsbp());

        // If it's a constant select and aligned, we can just copy the words
        إذا (ثابت AstConst* ثابت lsbConstp = VN_CAST(rhsp->lsbp(), Const)) {
            ثابت uint32_t lsb = lsbConstp->toUInt();
            إذا (VL_BITBIT_E(lsb) == 0) {
                didPattern(VExpandPattern::ASSIGN_SEL_ALIGNED, nodep);
                ثابت uint32_t word = VL_BITWORD_E(lsb);
                لكل (صحيح w = 0; w < nodep->width_وشەکان(); ++w) {
                    addWordAssign(nodep, w, newAstWordSelClone(rhsp->fromp(), w + word));
                }
                أرجع true;
            }
        }

        didPattern(VExpandPattern::ASSIGN_SEL_UNALIGNED, nodep);
        File_هێڵ* ثابت flp = rhsp->fileline();

        // Use fresh set of temporaries
        ++m_nTmps;

        // Compute word index of LSB, store to temporary if not constant
        Ast_گرێ_دەربڕین* ثابت wordLsbp = rhsp->lsbp()->clone_درەخت_Pure(خطأ);
        Ast_گرێ_دەربڕین* wordIdxp = newWordIndex(wordLsbp);
        إذا (!wordLsbp->backp()) VL_DO_DANGLING(wordLsbp->delete_درەخت(), wordLsbp);
        wordIdxp = V3Const::constifyEditCpp(wordIdxp);
        إذا (!VN_IS(wordIdxp, Const)) {
            AstVar* ثابت tmpp = addLocalTmp(nodep, "ExpandSel_WordIdx", wordIdxp);
            wordIdxp = جديد AstVarRef{flp, tmpp, VAccess::READ};
        }

        // Compute shift amounts and mask, store to temporaries if not constants
        Ast_گرێ_دەربڕین* loShftp = nullptr;
        Ast_گرێ_دەربڕین* hiShftp = nullptr;
        Ast_گرێ_دەربڕین* hiMaskp = nullptr;
        إذا (ثابت AstConst* ثابت lsbConstp = VN_CAST(rhsp->lsbp(), Const)) {
            ثابت uint32_t bitOffset = VL_BITBIT_E(lsbConstp->toUInt());
            // Must be unaligned, otherwise we would have handled it above
            UASSERT_OBJ(bitOffset, nodep, "Missed aligned wide select");
            loShftp = جديد AstConst{flp, bitOffset};
            hiShftp = جديد AstConst{flp, VL_EDATASIZE - bitOffset};
            hiMaskp = nullptr;
        } وإلا {
            // Compute Low word shift amount: bottom bits of lsb index
            Ast_گرێ_دەربڕین* ثابت lsbp = rhsp->lsbp()->clone_درەخت_Pure(خطأ);
            loShftp = جديد AstAnd{flp, جديد AstConst{flp, VL_SIZEBITS_E}, lsbp};
            AstVar* ثابت loTmpp = addLocalTmp(nodep, "ExpandSel_LoShift", loShftp);
            loShftp = جديد AstVarRef{flp, loTmpp, VAccess::READ};

            // Compute if aligned: Low word shift amount is 0
            Ast_گرێ_دەربڕین* ثابت zerop = جديد AstConst{flp, 0};
            Ast_گرێ_دەربڕین* alignedp = جديد AstEq{flp, zerop, loShftp->clone_درەخت_Pure(خطأ)};
            AstVar* ثابت alignedTmpp = addLocalTmp(nodep, "ExpandSel_Aligned", alignedp);
            alignedp = جديد AstVarRef{flp, alignedTmpp, VAccess::READ};

            // Computed High word shift amount: 0 if aligned else VL_EDATASIZE - Low word shift
            Ast_گرێ_دەربڕین* ثابت edsp = جديد AstConst{flp, VL_EDATASIZE};
            Ast_گرێ_دەربڕین* ثابت subp = جديد AstSub{flp, edsp, loShftp->clone_درەخت_Pure(خطأ)};
            hiShftp = جديد AstCond{flp, alignedp, جديد AstConst{flp, 0}, subp};
            AstVar* ثابت hiTmpp = addLocalTmp(nodep, "ExpandSel_HiShift", hiShftp);
            hiShftp = جديد AstVarRef{flp, hiTmpp, VAccess::READ};

            // Compute the High word mask: 0 if aligned, ones otherwise
            hiMaskp = جديد AstCond{flp, alignedp->clone_درەخت_Pure(خطأ),
                                  جديد AstConst{flp, AstConst::SizedEData{}, 0},
                                  جديد AstConst{flp, AstConst::SizedEData{}, -1ULL}};
            AstVar* ثابت maskTmpp = addLocalTmp(nodep, "ExpandSel_HiMask", hiMaskp);
            hiMaskp = جديد AstVarRef{flp, maskTmpp, VAccess::READ};
        }

        // Create each word of the selected result
        Ast_گرێ_دەربڕین* ثابت fromp = rhsp->fromp();
        ثابت صحيح sel_وشەکان = nodep->width_وشەکان();
        لكل (صحيح w = 0; w < sel_وشەکان; ++w) {
            // Grab bits from word 'VL_BITWORD_E(lsb) + w'
            Ast_گرێ_دەربڕین* ثابت loWordp = newWordSelWord(fromp->fileline(), fromp, wordIdxp, w);
            Ast_گرێ_دەربڕین* ثابت loShftClonep = loShftp->clone_درەخت_Pure(خطأ);
            Ast_گرێ_دەربڕین* ثابت lop = جديد AstShiftR{flp, loWordp, loShftClonep, VL_EDATASIZE};
            // Grab bits from word 'VL_BITWORD_E(lsb) + w + 1'
            Ast_گرێ_دەربڕین* hiWordp = newWordSelWord(fromp->fileline(), fromp, wordIdxp, w + 1);
            // For the last word of the result, avoid an OOB access when the Sel is not OOB
            إذا (w == sel_وشەکان - 1) {
                ثابت uint32_t max = fromp->width_وشەکان() - w - 1;
                Ast_گرێ_دەربڕین* ثابت maxp = جديد AstConst{flp, max};
                Ast_گرێ_دەربڕین* ثابت condp = جديد AstGte{flp, wordIdxp->clone_درەخت_Pure(خطأ), maxp};
                Ast_گرێ_دەربڕین* ثابت zerop = جديد AstConst{flp, AstConst::SizedEData{}, 0};
                hiWordp = جديد AstCond{flp, condp, zerop, hiWordp};
            }
            Ast_گرێ_دەربڕین* ثابت hiShftClonep = hiShftp->clone_درەخت_Pure(خطأ);
            Ast_گرێ_دەربڕین* ثابت hiPartp = جديد AstShiftL{flp, hiWordp, hiShftClonep, VL_EDATASIZE};
            Ast_گرێ_دەربڕین* ثابت hip
                = hiMaskp ? جديد AstAnd{flp, hiPartp, hiMaskp->clone_درەخت_Pure(خطأ)} : hiPartp;
            // Combine them
            addWordAssign(nodep, w, جديد AstOr{nodep->fileline(), hip, lop});
        }

        // Delete parts not captured during construction
        إذا (!wordIdxp->backp()) VL_DO_DANGLING(wordIdxp->delete_درەخت(), wordIdxp);
        إذا (!loShftp->backp()) VL_DO_DANGLING(loShftp->delete_درەخت(), loShftp);
        إذا (!hiShftp->backp()) VL_DO_DANGLING(hiShftp->delete_درەخت(), hiShftp);
        إذا (hiMaskp && !hiMaskp->backp()) VL_DO_DANGLING(hiMaskp->delete_درەخت(), hiMaskp);

        أرجع true;
    }

    منطقي expandLhs(Ast_گرێ_Assign* nodep, AstSel* lhsp) {
        // Possibilities
        //      destp: wide or narrow
        //      rhsp:  wide (destp must be wide), narrow, or 1 bit wide
        //      rhsp:  may be allones and can remove AND NOT gate
        //      lsbp:  constant or variable
        // Yuk.
        إذا (isImpure(nodep)) أرجع خطأ;
        File_هێڵ* ثابت nfl = nodep->fileline();
        File_هێڵ* ثابت lfl = lhsp->fileline();
        ثابت منطقي destwide = lhsp->fromp()->isWide();
        ثابت منطقي ones = nodep->rhsp()->isAllOnesV();
        إذا (VN_IS(lhsp->lsbp(), Const)) {
            // The code should work without this constant test, but it won't
            // constify as nicely as we'd like.
            Ast_گرێ_دەربڕین* rhsp = nodep->rhsp()->unlinkFrBack();
            Ast_گرێ_دەربڕین* ثابت destp = lhsp->fromp()->unlinkFrBack();
            ثابت صحيح lsb = lhsp->lsbConst();
            ثابت صحيح msb = lhsp->msbConst();
            V3_ژمار maskset{nodep, destp->widthMin()};
            maskset.setMask(msb + 1 - lsb, lsb);
            V3_ژمار maskold{nodep, destp->widthMin()};
            maskold.opNot(maskset);
            إذا (destwide) {
                didPattern(VExpandPattern::ASSIGN_TO_SEL_CONST_WIDE, nodep);
                لكل (صحيح w = 0; w < destp->width_وشەکان(); ++w) {
                    إذا (w >= VL_BITWORD_E(lsb) && w <= VL_BITWORD_E(msb)) {
                        // else we would just be setting it to the same exact value
                        Ast_گرێ_دەربڕین* oldvalp = newAstWordSelClone(destp, w);
                        fixCloneLvalue(oldvalp);
                        إذا (!ones) {
                            oldvalp = جديد AstAnd{
                                lfl,
                                جديد AstConst{lfl, AstConst::SizedEData{}, maskold.edataWord(w)},
                                oldvalp};
                        }

                        // Appropriate word of new value to insert:
                        Ast_گرێ_دەربڕین* newp = newWordGrabShift(lfl, w, rhsp, lsb);

                        // Apply cleaning at the top word of the destination
                        // (no cleaning to do if dst's width is a whole number
                        // of words).
                        إذا (w == destp->width_وشەکان() - 1 && VL_BITBIT_E(destp->widthMin()) != 0) {
                            V3_ژمار cleanmask{nodep, VL_EDATASIZE};
                            cleanmask.setMask(VL_BITBIT_E(destp->widthMin()));
                            newp = جديد AstAnd{lfl, newp, جديد AstConst{lfl, cleanmask}};
                        }
                        Ast_گرێ_دەربڕین* ثابت orp
                            = V3Const::constifyEditCpp(جديد AstOr{lfl, oldvalp, newp});
                        addWordAssign(nodep, w, destp, orp);
                    }
                }
                VL_DO_DANGLING(rhsp->delete_درەخت(), rhsp);
                VL_DO_DANGLING(destp->delete_درەخت(), destp);
            } وإلا {
                didPattern(VExpandPattern::ASSIGN_TO_SEL_CONST_NARROW, nodep);
                إذا (destp->isQuad() && !rhsp->isQuad()) rhsp = جديد AstCCast{nfl, rhsp, nodep};
                Ast_گرێ_دەربڕین* oldvalp = destp->clone_درەخت_Pure(true);
                fixCloneLvalue(oldvalp);
                إذا (!ones) oldvalp = جديد AstAnd{lfl, جديد AstConst{lfl, maskold}, oldvalp};

                // The bit-select can refer to bits outside the width of nodep
                // which we aren't allowed to assign to.  This is a mask of the
                // valid range of nodep which we apply to the new shifted RHS.
                V3_ژمار cleanmask{nodep, destp->widthMin()};
                cleanmask.setMask(destp->widthMin());
                Ast_گرێ_دەربڕین* ثابت shifted = جديد AstShiftL{
                    lfl, rhsp, جديد AstConst{lfl, static_cast<uint32_t>(lsb)}, destp->width()};
                Ast_گرێ_دەربڕین* ثابت cleaned
                    = جديد AstAnd{lfl, shifted, جديد AstConst{lfl, cleanmask}};
                Ast_گرێ_دەربڕین* ثابت orp
                    = V3Const::constifyEditCpp(جديد AstOr{lfl, oldvalp, cleaned});
                زیادکردن_Before(nodep, جديد AstAssign{nfl, destp, orp});
            }
            أرجع true;
        } وإلا {  // non-const select offset
            إذا (destwide && lhsp->widthConst() == 1) {
                didPattern(VExpandPattern::ASSIGN_TO_SEL_VAR_WIDE_ONE_BIT, nodep);
                Ast_گرێ_دەربڕین* ثابت rhsp = nodep->rhsp()->unlinkFrBack();
                Ast_گرێ_دەربڕین* ثابت destp = lhsp->fromp()->unlinkFrBack();
                Ast_گرێ_دەربڕین* oldvalp
                    = newWordSelBit(lfl, destp->clone_درەخت_Pure(خطأ), lhsp->lsbp());
                fixCloneLvalue(oldvalp);
                إذا (!ones) {
                    oldvalp = جديد AstAnd{
                        lfl,
                        جديد AstNot{
                            lfl, جديد AstShiftL{lfl, جديد AstConst{nfl, 1},
                                               // newSelBitBit may exceed the MSB of this variable.
                                               // That's ok as we'd just AND with a larger value,
                                               // but oldval would clip the upper bits to sanity
                                               newSelBitBit(lhsp->lsbp()), VL_EDATASIZE}},
                        oldvalp};
                }
                // Restrict the shift amount to 0-31, see bug804.
                Ast_گرێ_دەربڕین* ثابت shiftp = جديد AstAnd{nfl, lhsp->lsbp()->clone_درەخت_Pure(true),
                                                       جديد AstConst{nfl, VL_EDATASIZE - 1}};
                Ast_گرێ* ثابت newp = جديد AstAssign{
                    nfl, newWordSelBit(nfl, destp, lhsp->lsbp()),
                    جديد AstOr{lfl, oldvalp, جديد AstShiftL{lfl, rhsp, shiftp, VL_EDATASIZE}}};
                زیادکردن_Before(nodep, newp);
                أرجع true;
            } وإلا إذا (destwide) {
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
                أرجع خطأ;
            } وإلا {
                didPattern(VExpandPattern::ASSIGN_TO_SEL_VAR_NARROW, nodep);
                // nodep->dumpTree("-  old: ");
                Ast_گرێ_دەربڕین* rhsp = nodep->rhsp()->unlinkFrBack();
                Ast_گرێ_دەربڕین* ثابت destp = lhsp->fromp()->unlinkFrBack();
                Ast_گرێ_دەربڕین* oldvalp = destp->clone_درەخت_Pure(true);
                fixCloneLvalue(oldvalp);

                V3_ژمار maskwidth{nodep, destp->widthMin()};
                maskwidth.setMask(lhsp->widthConst());

                إذا (destp->isQuad() && !rhsp->isQuad()) rhsp = جديد AstCCast{nfl, rhsp, nodep};
                إذا (!ones) {
                    oldvalp = جديد AstAnd{
                        lfl,
                        جديد AstNot{lfl, جديد AstShiftL{lfl, جديد AstConst{nfl, maskwidth},
                                                      lhsp->lsbp()->clone_درەخت_Pure(true),
                                                      destp->width()}},
                        oldvalp};
                }
                Ast_گرێ_دەربڕین* newp
                    = جديد AstShiftL{lfl, rhsp, lhsp->lsbp()->clone_درەخت_Pure(true), destp->width()};
                // Apply cleaning to the new value being inserted.  Mask is
                // slightly wider than necessary to avoid an AND with all ones
                // being optimized out.  No need to clean if destp is
                // quad-sized as there are no extra bits to contaminate
                إذا (destp->widthMin() != 64) {
                    V3_ژمار cleanmask{nodep, destp->widthMin() + 1};
                    cleanmask.setMask(destp->widthMin());
                    newp = جديد AstAnd{lfl, newp, جديد AstConst{lfl, cleanmask}};
                }

                زیادکردن_Before(nodep, جديد AstAssign{nfl, destp, جديد AstOr{lfl, oldvalp, newp}});
                أرجع true;
            }
        }
    }

    فراغ visit(AstConcat* nodep) override {
        إذا (nodep->user1SetOnce()) أرجع;  // Process once
        iterateChildren(nodep);
        إذا (nodep->isWide()) {
            // See under ASSIGN(WIDE)
        } وإلا {
            // No isImpure() check - can handle side effects in below
            didPattern(VExpandPattern::CONCAT, nodep);
            File_هێڵ* ثابت fl = nodep->fileline();
            Ast_گرێ_دەربڕین* lhsp = nodep->lhsp()->unlinkFrBack();
            Ast_گرێ_دەربڕین* rhsp = nodep->rhsp()->unlinkFrBack();
            ثابت uint32_t rhsshift = rhsp->widthMin();
            إذا (nodep->isQuad() && !lhsp->isQuad()) lhsp = جديد AstCCast{fl, lhsp, nodep};
            إذا (nodep->isQuad() && !rhsp->isQuad()) rhsp = جديد AstCCast{fl, rhsp, nodep};
            Ast_گرێ_دەربڕین* ثابت newp = جديد AstOr{
                fl, جديد AstShiftL{fl, lhsp, جديد AstConst{fl, rhsshift}, nodep->width()}, rhsp};
            newp->dtypeFrom(nodep);  // Unsigned
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        }
    }
    منطقي expandWide(Ast_گرێ_Assign* nodep, AstConcat* rhsp) {
        إذا (!doExpandWide(rhsp)) أرجع خطأ;
        didPattern(VExpandPattern::ASSIGN_CONCAT, nodep);
        File_هێڵ* ثابت fl = rhsp->fileline();
        // Lhs or Rhs may be word, long, or quad.
        // newAstWordSelClone nicely abstracts the difference.
        ثابت صحيح rhsshift = rhsp->rhsp()->widthMin();
        // Sometimes doing the words backwards is preferable.
        // When we have x={x,foo} backwards is better, when x={foo,x} forward is better
        // However V3Subst tends to rip this up, so not worth optimizing now.
        لكل (صحيح w = 0; w < rhsp->width_وشەکان(); ++w) {
            addWordAssign(nodep, w,
                          جديد AstOr{fl, newWordGrabShift(fl, w, rhsp->lhsp(), rhsshift),
                                    newAstWordSelClone(rhsp->rhsp(), w)});
        }
        أرجع true;
    }

    فراغ visit(AstReplicate* nodep) override {
        إذا (nodep->user1SetOnce()) أرجع;  // Process once
        iterateChildren(nodep);
        إذا (nodep->isWide()) {
            // See under ASSIGN(WIDE)
        } وإلا {
            إذا (isImpure(nodep)) أرجع;
            File_هێڵ* ثابت fl = nodep->fileline();
            Ast_گرێ_دەربڕین* lhsp = nodep->srcp()->unlinkFrBack();
            Ast_گرێ_دەربڕین* newp;
            ثابت صحيح lhswidth = lhsp->widthMin();
            إذا (lhswidth == 1) {
                didPattern(VExpandPattern::REPLICATE_ONE_BIT, nodep);
                newp = جديد AstNegate{fl, lhsp};
            } وإلا {
                didPattern(VExpandPattern::REPLICATE, nodep);
                ثابت AstConst* ثابت constp = VN_AS(nodep->countp(), Const);
                UASSERT_OBJ(constp, nodep,
                            "Replication value isn't a constant.  Checked earlier!");
                ثابت uint32_t times = constp->toUInt();
                إذا (nodep->isQuad() && !lhsp->isQuad()) lhsp = جديد AstCCast{fl, lhsp, nodep};
                newp = lhsp->clone_درەخت_Pure(true);
                لكل (unsigned repnum = 1; repnum < times; repnum++) {
                    ثابت صحيح rhsshift = repnum * lhswidth;
                    newp = جديد AstOr{
                        fl,
                        جديد AstShiftL{fl, lhsp->clone_درەخت_Pure(true),
                                      جديد AstConst{fl, static_cast<uint32_t>(rhsshift)},
                                      nodep->width()},
                        newp};
                    newp->dtypeFrom(nodep);  // Unsigned
                }
                VL_DO_DANGLING(lhsp->delete_درەخت(), lhsp);  // Never used
            }
            newp->dtypeFrom(nodep);  // Unsigned
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        }
    }
    منطقي expandWide(Ast_گرێ_Assign* nodep, AstReplicate* rhsp) {
        إذا (!doExpandWide(rhsp)) أرجع خطأ;
        didPattern(VExpandPattern::ASSIGN_REPLICATE, nodep);
        File_هێڵ* ثابت fl = nodep->fileline();
        Ast_گرێ_دەربڕین* ثابت lhsp = rhsp->srcp();
        ثابت صحيح lhswidth = lhsp->widthMin();
        ثابت AstConst* ثابت constp = VN_AS(rhsp->countp(), Const);
        UASSERT_OBJ(constp, rhsp, "Replication value isn't a constant.  Checked earlier!");
        ثابت uint32_t times = constp->toUInt();
        لكل (صحيح w = 0; w < rhsp->width_وشەکان(); ++w) {
            Ast_گرێ_دەربڕین* newp;
            إذا (lhswidth == 1) {
                newp = جديد AstNegate{fl, lhsp->clone_درەخت_Pure(true)};
                // Replicate always unsigned
                newp->dtypeChgWidthSigned(VL_EDATASIZE, VL_EDATASIZE, VSigning::UNSIGNED);
            } وإلا {
                newp = newAstWordSelClone(lhsp, w);
                File_هێڵ* ثابت rfl = rhsp->fileline();
                لكل (unsigned repnum = 1; repnum < times; repnum++) {
                    newp = جديد AstOr{fl, newWordGrabShift(rfl, w, lhsp, lhswidth * repnum), newp};
                }
            }
            addWordAssign(nodep, w, newp);
        }
        أرجع true;
    }

    فراغ visitEqNeq(Ast_گرێ_Biop* nodep) {
        إذا (nodep->user1SetOnce()) أرجع;  // Process once
        iterateChildren(nodep);
        إذا (nodep->lhsp()->isWide()
            && !(VN_IS(nodep->lhsp()->dtypep()->skipRefp(), StreamDType)
                 || VN_IS(nodep->rhsp()->dtypep()->skipRefp(), StreamDType))) {
            إذا (isImpure(nodep)) أرجع;
            إذا (!doExpandWide(nodep->lhsp())) أرجع;
            إذا (!doExpandWide(nodep->rhsp())) أرجع;
            didPattern(VN_IS(nodep, Neq) ? VExpandPattern::WORDIZE_NEQ
                                         : VExpandPattern::WORDIZE_EQ,
                       nodep);
            // -> (0=={or{for each_word{WORDSEL(lhs,#)^WORDSEL(rhs,#)}}}
            File_هێڵ* ثابت fl = nodep->fileline();
            Ast_گرێ_دەربڕین* newp = nullptr;
            لكل (صحيح w = 0; w < nodep->lhsp()->width_وشەکان(); ++w) {
                Ast_گرێ_دەربڕین* ثابت eqp = جديد AstXor{fl, newAstWordSelClone(nodep->lhsp(), w),
                                                    newAstWordSelClone(nodep->rhsp(), w)};
                newp = newp ? جديد AstOr{fl, newp, eqp} : eqp;
            }
            إذا (VN_IS(nodep, Neq)) {
                newp = جديد AstNeq{fl, جديد AstConst{fl, AstConst::SizedEData{}, 0}, newp};
            } وإلا {
                newp = جديد AstEq{fl, جديد AstConst{fl, AstConst::SizedEData{}, 0}, newp};
            }
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        }
    }
    فراغ visit(AstEq* nodep) override { visitEqNeq(nodep); }
    فراغ visit(AstNeq* nodep) override { visitEqNeq(nodep); }

    فراغ visit(AstRedOr* nodep) override {
        إذا (nodep->user1SetOnce()) أرجع;  // Process once
        iterateChildren(nodep);
        File_هێڵ* ثابت fl = nodep->fileline();
        إذا (nodep->lhsp()->isWide()) {
            إذا (isImpure(nodep)) أرجع;
            إذا (!doExpandWide(nodep->lhsp())) أرجع;
            didPattern(VExpandPattern::WORDIZE_REDOR, nodep);
            // -> (0!={or{for each_word{WORDSEL(lhs,#)}}}
            Ast_گرێ_دەربڕین* newp = nullptr;
            لكل (صحيح w = 0; w < nodep->lhsp()->width_وشەکان(); ++w) {
                Ast_گرێ_دەربڕین* ثابت eqp = newAstWordSelClone(nodep->lhsp(), w);
                newp = newp ? جديد AstOr{fl, newp, eqp} : eqp;
            }
            newp = جديد AstNeq{fl, جديد AstConst{fl, AstConst::SizedEData{}, 0}, newp};
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        } وإلا {
            // No isImpure() check - can handle side effects in below
            didPattern(VExpandPattern::REDOR_TO_NEQ, nodep);
            Ast_گرێ_دەربڕین* ثابت lhsp = nodep->lhsp()->unlinkFrBack();
            Ast_گرێ_دەربڕین* ثابت newp = جديد AstNeq{
                fl, جديد AstConst{fl, AstConst::Widthed_بەها{}, longOrQuadWidth(nodep), 0}, lhsp};
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        }
    }
    فراغ visit(AstRedAnd* nodep) override {
        إذا (nodep->user1SetOnce()) أرجع;  // Process once
        iterateChildren(nodep);
        File_هێڵ* ثابت fl = nodep->fileline();
        إذا (nodep->lhsp()->isWide()) {
            إذا (isImpure(nodep)) أرجع;
            إذا (!doExpandWide(nodep->lhsp())) أرجع;
            didPattern(VExpandPattern::WORDIZE_REDAND, nodep);
            // -> (0!={and{for each_word{WORDSEL(lhs,#)}}}
            Ast_گرێ_دەربڕین* newp = nullptr;
            لكل (صحيح w = 0; w < nodep->lhsp()->width_وشەکان(); ++w) {
                Ast_گرێ_دەربڕین* eqp = newAstWordSelClone(nodep->lhsp(), w);
                إذا (w == nodep->lhsp()->width_وشەکان() - 1) {
                    // Rather than doing a (slowish) ==##, we OR in the
                    // bits that aren't part of the mask
                    eqp = جديد AstOr{fl, جديد AstConst{fl, notWideMask(nodep->lhsp())},
                                    // Bug in cppcheck
                                    // cppcheck-suppress memleak
                                    eqp};
                }
                newp = newp ? جديد AstAnd{fl, newp, eqp} : eqp;
            }
            newp = جديد AstEq{fl, جديد AstConst{fl, AstConst::SizedEData{}, VL_MASK_E(VL_EDATASIZE)},
                             newp};
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        } وإلا {
            // No isImpure() check - can handle side effects in below
            didPattern(VExpandPattern::REDAND_TO_EQ, nodep);
            Ast_گرێ_دەربڕین* ثابت lhsp = nodep->lhsp()->unlinkFrBack();
            Ast_گرێ_دەربڕین* ثابت newp = جديد AstEq{fl, جديد AstConst{fl, wordMask(lhsp)}, lhsp};
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        }
    }
    فراغ visit(AstRedXor* nodep) override {
        إذا (nodep->user1SetOnce()) أرجع;  // Process once
        iterateChildren(nodep);
        إذا (nodep->lhsp()->isWide()) {
            إذا (isImpure(nodep)) أرجع;
            إذا (!doExpandWide(nodep->lhsp())) أرجع;
            didPattern(VExpandPattern::WORDIZE_REDXOR, nodep);
            // -> (0!={redxor{for each_word{XOR(WORDSEL(lhs,#))}}}
            File_هێڵ* ثابت fl = nodep->fileline();
            Ast_گرێ_دەربڕین* newp = nullptr;
            لكل (صحيح w = 0; w < nodep->lhsp()->width_وشەکان(); ++w) {
                Ast_گرێ_دەربڕین* ثابت eqp = newAstWordSelClone(nodep->lhsp(), w);
                newp = newp ? جديد AstXor{fl, newp, eqp} : eqp;
            }
            newp = جديد AstRedXor{fl, newp};
            UINFO(8, "    Wordize REDXORnew " << newp);
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        }
        // We don't reduce non-wide XORs, as its more efficient to use a temp register,
        // which the inlined function does nicely.
    }

    فراغ visit(Ast_گرێ_Stmt* nodep) override {
        إذا (nodep->user1SetOnce()) أرجع;  // Process once
        VL_RESTORER(m_stmtp);
        m_stmtp = nodep;
        iterateChildren(nodep);
    }
    فراغ visit(Ast_گرێ_Assign* nodep) override {
        إذا (nodep->user1SetOnce()) أرجع;  // Process once
        إذا (VN_IS(nodep->dtypep()->skipRefp(), UnpackArrayDType)) {
            أرجع;  // Skip for UnpackArrayDType
        }

        VL_RESTORER(m_stmtp);
        m_stmtp = nodep;
        iterateChildren(nodep);
        منطقي did = خطأ;
        إذا (nodep->isWide()  //
            && ((VN_IS(nodep->lhsp(), VarRef) || VN_IS(nodep->lhsp(), ArraySel)))
            && !AstVar::scVarRecurse(nodep->lhsp())  // Need special function for SC
            && !AstVar::scVarRecurse(nodep->rhsp())) {
            إذا (AstConst* ثابت rhsp = VN_CAST(nodep->rhsp(), Const)) {
                did = expandWide(nodep, rhsp);
            } وإلا إذا (AstVarRef* ثابت rhsp = VN_CAST(nodep->rhsp(), VarRef)) {
                did = expandWide(nodep, rhsp);
            } وإلا إذا (AstSel* ثابت rhsp = VN_CAST(nodep->rhsp(), Sel)) {
                did = expandWide(nodep, rhsp);
            } وإلا إذا (AstArraySel* ثابت rhsp = VN_CAST(nodep->rhsp(), ArraySel)) {
                did = expandWide(nodep, rhsp);
            } وإلا إذا (AstConcat* ثابت rhsp = VN_CAST(nodep->rhsp(), Concat)) {
                did = expandWide(nodep, rhsp);
            } وإلا إذا (AstExtend* ثابت rhsp = VN_CAST(nodep->rhsp(), Extend)) {
                did = expandWide(nodep, rhsp);
            } وإلا إذا (AstReplicate* ثابت rhsp = VN_CAST(nodep->rhsp(), Replicate)) {
                did = expandWide(nodep, rhsp);
            } وإلا إذا (AstAnd* ثابت rhsp = VN_CAST(nodep->rhsp(), And)) {
                did = expandWide(nodep, rhsp);
            } وإلا إذا (AstOr* ثابت rhsp = VN_CAST(nodep->rhsp(), Or)) {
                did = expandWide(nodep, rhsp);
            } وإلا إذا (AstNot* ثابت rhsp = VN_CAST(nodep->rhsp(), Not)) {
                did = expandWide(nodep, rhsp);
            } وإلا إذا (AstXor* ثابت rhsp = VN_CAST(nodep->rhsp(), Xor)) {
                did = expandWide(nodep, rhsp);
            } وإلا إذا (AstShiftL* ثابت rhsp = VN_CAST(nodep->rhsp(), ShiftL)) {
                did = expandWideShift(nodep, rhsp, /* isLeftShift: */ true);
            } وإلا إذا (AstShiftR* ثابت rhsp = VN_CAST(nodep->rhsp(), ShiftR)) {
                did = expandWideShift(nodep, rhsp, /* isLeftShift: */ خطأ);
            } وإلا إذا (AstCond* ثابت rhsp = VN_CAST(nodep->rhsp(), Cond)) {
                did = expandWide(nodep, rhsp);
            }
        } وإلا إذا (AstSel* ثابت lhsp = VN_CAST(nodep->lhsp(), Sel)) {
            did = expandLhs(nodep, lhsp);
        }
        // Cleanup common code
        إذا (did) VL_DO_DANGLING(nodep->unlinkFrBack()->delete_درەخت(), nodep);
    }

    //--------------------
    فراغ visit(AstVar*) override {}  // Don't hit varrefs under vars
    فراغ visit(Ast_گرێ* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    explicit ExpandVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~ExpandVisitor() override {
        V3Stats::addStat("Optimizations, Expand, expanded wides", m_statWides);
        V3Stats::addStat("Optimizations, Expand, expanded wide words", m_stat_Wide_وشەکان);
        V3Stats::addStat("Optimizations, Expand, not expanded impure", m_statImpure);
        V3Stats::addStat("Optimizations, Expand, not expanded limit", m_statWideLimited);
        V3Stats::addStat("Optimizations, Expand, temporaries created", m_statTemps);
        ثابت auto emitStat = [هذا](VExpandPattern pattern) {
            std::string str{pattern.ascii()};
            std::transform(str.begin(), str.end(), str.begin(), [](unsigned حرف c) {  //
                أرجع c == '_' ? ' ' : std::tolower(c);
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

فراغ V3Expand::expandAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        ثابت ExpandOkVisitor okVisitor{nodep};
        ExpandVisitor{nodep};
    }  // Destruct before checking
    V3Global::dump_Check_Global_درەخت("expand", 0, dump_درەخت_Either_Level() >= 3);
}
