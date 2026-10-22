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

#شامل "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#شامل "V3Expand.h"

#شامل "V3Const.h"
#شامل "V3Stats.h"

#شامل <algorithm>
#شامل <array>

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

رده VExpandPattern final {
عمومی:
    شمارش en : uint8_t {
#define EXPAND_PATTERN_ID(id) id,
        FOR_EACH_EXPAND_PATTERN(EXPAND_PATTERN_ID)
#undef EXPAND_PATTERN_ID
            _ENUM_END
    };
    شمارش en m_e;
    ثابت نویسه* ascii() ثابت {
        ایستا ثابت نویسه* ثابت names[] = {
#define EXPAND_PATTERN_NAME(id) #id,
            FOR_EACH_EXPAND_PATTERN(EXPAND_PATTERN_NAME)
#undef EXPAND_PATTERN_NAME
                "_ENUM_END"  //
        };
        بازگشت names[m_e];
    }

    // cppcheck-suppress noExplicitConstructor
    VExpandPattern(en _e)
        : m_e{_e} {}
    operator en() ثابت { بازگشت m_e; }
};

//######################################################################
// Find nodes with side effects, to mark as non-expandable

رده ExpandOkVisitor final : عمومی VNVisitorConst {
    // NODE STATE
    //  AstNode::user2()        -> bool.  Is pure (along with all children)
    ثابت VNUser2InUse m_inuser2;

    // STATE - for current visit position (use VL_RESTORER)
    // Tracks similar to AstNode::isTreePureRecurse(), but avoid O(n^2)
    // False = pure, as nodes that ExpandVisitor inserts preserve pureness
    منطقی m_isImpure = درست;  // Currently pure

    تهی visit(Ast_گره* nodep) override {
        منطقی selfImpure = !nodep->isPure();
        {
            VL_RESTORER(m_isImpure);
            m_isImpure = نادرست;
            iterateChildrenConst(nodep);
            selfImpure |= m_isImpure;
            nodep->user2(selfImpure);
        }
        m_isImpure |= selfImpure;
    }

عمومی:
    // CONSTRUCTORS
    explicit ExpandOkVisitor(AstNetlist* nodep) { iterateConst(nodep); }
    ~ExpandOkVisitor() = پیشفرض;
};

//######################################################################
// Expand state, as a visitor of each AstNode

رده ExpandVisitor final : عمومی VNVisitor {
    // NODE STATE
    //  AstNode::user1()        -> bool.  Processed
    //  AstNode::user2()        -> See ExpandOkVisitor
    ثابت VNUser1InUse m_inuser1;

    // STATE - for current visit position (use VL_RESTORER)
    Ast_گره* m_stmtp = nullptr;  // Current statement

    // STATE - across all visitors
    AstCFunc* m_funcp = nullptr;  // Current function
    VDouble0 m_statImpure;  // Statistic tracking
    VDouble0 m_statWides;  // Statistic tracking
    VDouble0 m_stat_Wide_واژگان;  // Statistic tracking
    VDouble0 m_statWideLimited;  // Statistic tracking
    VDouble0 m_statTemps;  // Statistic tracking
    // Number of applications of each expansion pattern for statistics
    std::array<VDouble0, VExpandPattern::_ENUM_END> m_statPatterns;

    // STATE - for current function
    size_t m_nTmps = 0;  // Sequence numbers for temopraries

    // METHODS
    // Note the application of the given expansion pattern to 'nodep'
    تهی didPattern(VExpandPattern pattern, Ast_گره* nodep) {
        UINFO(8, "    " << pattern.ascii() << " " << nodep);
        ++m_statPatterns[pattern];
    }

    // Use state that ExpandOkVisitor calculated
    ایستا منطقی isImpure(Ast_گره* nodep) {
        ثابت منطقی impure = nodep->user2();
        اگر (impure) UINFO(9, "      impure " << nodep);
        بازگشت impure;
    }

    منطقی doExpandWide(Ast_گره* nodep) {
        اگر (isImpure(nodep)) {
            ++m_statImpure;
            بازگشت نادرست;
        }
        اگر (nodep->width_واژگان() <= v3Global.opt.expandLimit()) {
            ++m_statWides;
            m_stat_Wide_واژگان += nodep->width_واژگان();
            بازگشت درست;
        } وگرنه {
            ++m_statWideLimited;
            بازگشت نادرست;
        }
    }

    ایستا صحیح longOrQuadWidth(Ast_گره* nodep) {
        بازگشت (nodep->width() + (VL_EDATASIZE - 1)) & ~(VL_EDATASIZE - 1);
    }
    ایستا V3_عدد notWideMask(Ast_گره* nodep) {
        بازگشت V3_عدد{nodep, VL_EDATASIZE, ~VL_MASK_E(nodep->widthMin())};
    }
    ایستا V3_عدد wordMask(Ast_گره* nodep) {
        اگر (nodep->isWide()) {
            بازگشت V3_عدد{nodep, VL_EDATASIZE, VL_MASK_E(nodep->widthMin())};
        } وگرنه {
            V3_عدد mask{nodep, longOrQuadWidth(nodep)};
            mask.setMask(nodep->widthMin());
            بازگشت mask;
        }
    }

    ایستا تهی درج_Before(Ast_گره* placep, Ast_گره* newp) {
        newp->user1(1);  // Already processed, don't need to re-iterate
        placep->addHereThisAsNext(newp);
    }
    ایستا تهی replaceWithDelete(Ast_گره* nodep, Ast_گره* newp) {
        newp->user1(1);  // Already processed, don't need to re-iterate
        اگر (newp->width() != nodep->width()) {
            UASSERT_OBJ(newp->widthMin() == nodep->widthMin(), nodep,
                        "Replacement width mismatch");
            newp->dtypeChgWidth(nodep->width(), nodep->widthMin());
        }
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_درخت(), nodep);
    }
    ایستا Ast_گره* newWordAssign(Ast_گره_Assign* placep, صحیح word, Ast_گره_عبارت* lhsp,
                                  Ast_گره_عبارت* rhsp) {
        File_سطر* ثابت fl = placep->fileline();
        بازگشت جدید AstAssign{fl,
                             جدید AstWordSel{fl, lhsp->clone_درخت_Pure(درست),
                                            جدید AstConst{fl, static_cast<uint32_t>(word)}},
                             rhsp};
    }
    ایستا تهی addWordAssign(Ast_گره_Assign* placep, صحیح word, Ast_گره_عبارت* lhsp,
                              Ast_گره_عبارت* rhsp) {
        درج_Before(placep, newWordAssign(placep, word, lhsp, rhsp));
    }
    ایستا تهی addWordAssign(Ast_گره_Assign* placep, صحیح word, Ast_گره_عبارت* rhsp) {
        addWordAssign(placep, word, placep->lhsp(), rhsp);
    }

    AstVar* addLocalTmp(Ast_گره* placep, ثابت نویسه* namep, Ast_گره_عبارت* valuep) {
        ثابت std::string name = "__V"s + namep + "_"s + std::to_string(m_nTmps);
        File_سطر* ثابت flp = placep->fileline();
        AstVar* ثابت tmpp = جدید AstVar{flp, VVarType::STMTTEMP, name, valuep->dtypep()};
        tmpp->funcLocal(درست);
        tmpp->isInternal(درست);
        tmpp->noReset(درست);
        tmpp->substConstOnly(درست);
        m_funcp->addVarsp(tmpp);
        درج_Before(placep, جدید AstAssign{flp, جدید AstVarRef{flp, tmpp, VAccess::WRITE}, valuep});
        ++m_statTemps;
        بازگشت tmpp;
    }

    ایستا تهی fixCloneLvalue(Ast_گره* nodep) {
        // In AstSel transforms, we call clone() on VarRefs that were lvalues,
        // but are now being used on the RHS of the assignment
        اگر (VN_IS(nodep, VarRef)) VN_AS(nodep, VarRef)->access(VAccess::READ);
        // Iterate
        اگر (Ast_گره* ثابت refp = nodep->op1p()) fixCloneLvalue(refp);
        اگر (Ast_گره* ثابت refp = nodep->op2p()) fixCloneLvalue(refp);
        اگر (Ast_گره* ثابت refp = nodep->op3p()) fixCloneLvalue(refp);
        اگر (Ast_گره* ثابت refp = nodep->op4p()) fixCloneLvalue(refp);
    }

    ایستا Ast_گره_عبارت* newAstWordSelClone(Ast_گره_عبارت* nodep, صحیح word) {
        // Get the specified word number from a wide array
        // Or, if it's a long/quad, do appropriate conversion to wide
        // Concat may pass negative word numbers, that means it wants a zero
        File_سطر* ثابت fl = nodep->fileline();
        اگر (nodep->isWide() && word >= 0 && word < nodep->width_واژگان()) {
            بازگشت جدید AstWordSel{fl, nodep->clone_درخت_Pure(درست),
                                  جدید AstConst{fl, static_cast<uint32_t>(word)}};
        } وگرنه اگر (nodep->isQuad() && word == 0) {
            Ast_گره_عبارت* ثابت quadfromp = nodep->clone_درخت_Pure(درست);
            quadfromp->dtypeSetBitUnsized(VL_QUADSIZE, quadfromp->widthMin(), VSigning::UNSIGNED);
            بازگشت جدید AstCCast{fl, quadfromp, VL_EDATASIZE};
        } وگرنه اگر (nodep->isQuad() && word == 1) {
            Ast_گره_عبارت* ثابت quadfromp = nodep->clone_درخت_Pure(درست);
            quadfromp->dtypeSetBitUnsized(VL_QUADSIZE, quadfromp->widthMin(), VSigning::UNSIGNED);
            بازگشت جدید AstCCast{
                fl, جدید AstShiftR{fl, quadfromp, جدید AstConst{fl, VL_EDATASIZE}, VL_EDATASIZE},
                VL_EDATASIZE};
        } وگرنه اگر (!nodep->isWide() && !nodep->isQuad() && word == 0) {
            بازگشت nodep->clone_درخت_Pure(درست);
        } وگرنه {  // Out of bounds
            بازگشت جدید AstConst{fl, 0};
        }
    }

    ایستا Ast_گره_عبارت* newWordGrabShift(File_سطر* fl, صحیح word, Ast_گره_عبارت* lhsp, صحیح shift) {
        // Extract the expression to grab the value for the specified word, if it's the shift
        // of shift bits from lhsp
        // Negative word numbers requested for lhs when it's "before" what we want.
        // We get a 0 then.
        ثابت صحیح othword = word - shift / VL_EDATASIZE;
        Ast_گره_عبارت* ثابت lop = newAstWordSelClone(lhsp, othword);
        ثابت uint32_t loShift = VL_BITBIT_E(shift);
        اگر (!loShift) بازگشت lop;
        Ast_گره_عبارت* ثابت hip = newAstWordSelClone(lhsp, othword - 1);
        ثابت uint32_t hiShift = VL_EDATASIZE - loShift;  // Complement offset
        Ast_گره_عبارت* ثابت newp
            = جدید AstOr{fl,  //
                        جدید AstShiftR{fl, hip, جدید AstConst{fl, hiShift}, VL_EDATASIZE},
                        جدید AstShiftL{fl, lop, جدید AstConst{fl, loShift}, VL_EDATASIZE}};
        بازگشت V3Const::constifyEditCpp(newp);
    }

    // Return expression indexing the word that contains 'lsbp' + the given word offset
    ایستا Ast_گره_عبارت* newWordIndex(Ast_گره_عبارت* lsbp, uint32_t wordOffset = 0) {
        // This is indexing a WordSel, so a 32 bit constants are fine
        File_سطر* ثابت flp = lsbp->fileline();
        اگر (ثابت AstConst* constp = VN_CAST(lsbp, Const)) {
            بازگشت جدید AstConst{flp, wordOffset + VL_BITWORD_E(constp->toUInt())};
        }

        اگر (lsbp->backp()) lsbp = lsbp->clone_درخت_Pure(نادرست);
        Ast_گره_عبارت* ثابت wwl2p = جدید AstConst{flp, VL_EDATASIZE_LOG2};  // Word width log 2
        Ast_گره_عبارت* indexp = جدید AstShiftR{flp, lsbp, wwl2p, VL_EDATASIZE};
        اگر (wordOffset) indexp = جدید AstAdd{flp, جدید AstConst{flp, wordOffset}, indexp};
        بازگشت indexp;
    }

    // Return word of fromp that contains word indexp + the given word offset.
    ایستا Ast_گره_عبارت* newWordSelWord(File_سطر* flp, Ast_گره_عبارت* fromp, Ast_گره_عبارت* indexp,
                                       uint32_t wordOffset = 0) {
        UASSERT_OBJ(fromp->isWide(), fromp, "Only need AstWordSel on wide from's");

        اگر (ثابت AstConst* ثابت constp = VN_CAST(indexp, Const)) {
            indexp = nullptr;
            wordOffset += constp->toUInt();
        }

        // e.g. "logic [95:0] var[0]; logic [0] sel; out = var[sel];"
        // Squash before C++ to avoid getting a C++ compiler warning
        // (even though code would be unreachable as presumably a
        // AstCond is protecting above this node.
        اگر (wordOffset >= static_cast<uint32_t>(fromp->width_واژگان())) {
            بازگشت جدید AstConst{flp, AstConst::SizedEData{}, 0};
        }

        اگر (fromp->backp()) fromp = fromp->clone_درخت_Pure(نادرست);

        // If indexp was constant, just use it
        اگر (!indexp) بازگشت جدید AstWordSel{flp, fromp, جدید AstConst{flp, wordOffset}};

        // Otherwise compute at runtime
        اگر (indexp->backp()) indexp = indexp->clone_درخت_Pure(نادرست);
        اگر (wordOffset) indexp = جدید AstAdd{flp, جدید AstConst{flp, wordOffset}, indexp};
        بازگشت جدید AstWordSel{flp, fromp, indexp};
    }

    // Return word of fromp that contains bit lsbp + the given word offset.
    ایستا Ast_گره_عبارت* newWordSelBit(File_سطر* flp, Ast_گره_عبارت* fromp, Ast_گره_عبارت* lsbp,
                                      uint32_t wordOffset = 0) {
        Ast_گره_عبارت* ثابت indexp = newWordIndex(lsbp, wordOffset);
        Ast_گره_عبارت* ثابت wordSelp = newWordSelWord(flp, fromp, indexp);
        اگر (!indexp->backp()) VL_DO_DANGLING(indexp->delete_درخت(), indexp);
        بازگشت wordSelp;
    }

    ایستا Ast_گره_عبارت* newSelBitBit(Ast_گره_عبارت* lsbp) {
        // Return equation to get the VL_BITBIT of a constant or non-constant
        File_سطر* ثابت fl = lsbp->fileline();
        اگر (VN_IS(lsbp, Const)) {
            بازگشت جدید AstConst{fl, VL_BITBIT_E(VN_AS(lsbp, Const)->toUInt())};
        } وگرنه {
            بازگشت جدید AstAnd{fl, جدید AstConst{fl, VL_EDATASIZE - 1}, lsbp->clone_درخت_Pure(درست)};
        }
    }

    //====================

    منطقی expandWide(Ast_گره_Assign* nodep, AstConst* rhsp) {
        اگر (!doExpandWide(nodep)) بازگشت نادرست;
        didPattern(VExpandPattern::ASSIGN_CONST, nodep);
        // -> {for each_word{ ASSIGN(WORDSEL(wide,#),WORDSEL(CONST,#))}}
        اگر (rhsp->رقم().isFourState()) {
            rhsp->v3warn(E_UNSUPPORTED,  // LCOV_EXCL_LINE  // impossible?
                         "Unsupported: 4-state numbers in this context");
        }
        File_سطر* ثابت fl = nodep->fileline();
        برای (صحیح w = 0; w < nodep->width_واژگان(); ++w) {
            addWordAssign(nodep, w,
                          جدید AstConst{fl, AstConst::SizedEData{}, rhsp->رقم().edataWord(w)});
        }
        بازگشت درست;
    }
    //-------- Uniops
    منطقی expandWide(Ast_گره_Assign* nodep, AstVarRef* rhsp) {
        // Special case: do not expand assignment of constant pool variables.
        // V3Subst undestands these directly.
        اگر (rhsp->varp()->constPoolEntry()) بازگشت نادرست;
        اگر (!doExpandWide(nodep)) بازگشت نادرست;
        didPattern(VExpandPattern::ASSIGN_VARREF, nodep);
        برای (صحیح w = 0; w < nodep->width_واژگان(); ++w) {
            addWordAssign(nodep, w, newAstWordSelClone(rhsp, w));
        }
        بازگشت درست;
    }
    منطقی expandWide(Ast_گره_Assign* nodep, AstArraySel* rhsp) {
        UASSERT_OBJ(!VN_IS(nodep->dtypep()->skipRefp(), UnpackArrayDType), nodep,
                    "ArraySel with unpacked arrays should have been removed in V3Slice");
        اگر (!doExpandWide(nodep)) بازگشت نادرست;
        didPattern(VExpandPattern::ASSIGN_ARRAYSEL, nodep);
        برای (صحیح w = 0; w < nodep->width_واژگان(); ++w) {
            addWordAssign(nodep, w, newAstWordSelClone(rhsp, w));
        }
        بازگشت درست;
    }
    منطقی expandWide(Ast_گره_Assign* nodep, AstNot* rhsp) {
        // -> {for each_word{ ASSIGN(WORDSEL(wide,#),NOT(WORDSEL(lhs,#))) }}
        اگر (!doExpandWide(nodep)) بازگشت نادرست;
        didPattern(VExpandPattern::ASSIGN_NOT, nodep);
        File_سطر* ثابت fl = rhsp->fileline();
        برای (صحیح w = 0; w < nodep->width_واژگان(); ++w) {
            addWordAssign(nodep, w, جدید AstNot{fl, newAstWordSelClone(rhsp->lhsp(), w)});
        }
        بازگشت درست;
    }
    //-------- Biops
    منطقی expandWide(Ast_گره_Assign* nodep, AstAnd* rhsp) {
        اگر (!doExpandWide(nodep)) بازگشت نادرست;
        didPattern(VExpandPattern::ASSIGN_AND, nodep);
        File_سطر* ثابت fl = nodep->fileline();
        برای (صحیح w = 0; w < nodep->width_واژگان(); ++w) {
            addWordAssign(nodep, w,
                          جدید AstAnd{fl, newAstWordSelClone(rhsp->lhsp(), w),
                                     newAstWordSelClone(rhsp->rhsp(), w)});
        }
        بازگشت درست;
    }
    منطقی expandWide(Ast_گره_Assign* nodep, AstOr* rhsp) {
        اگر (!doExpandWide(nodep)) بازگشت نادرست;
        didPattern(VExpandPattern::ASSIGN_OR, nodep);
        File_سطر* ثابت fl = nodep->fileline();
        برای (صحیح w = 0; w < nodep->width_واژگان(); ++w) {
            addWordAssign(nodep, w,
                          جدید AstOr{fl, newAstWordSelClone(rhsp->lhsp(), w),
                                    newAstWordSelClone(rhsp->rhsp(), w)});
        }
        بازگشت درست;
    }
    منطقی expandWide(Ast_گره_Assign* nodep, AstXor* rhsp) {
        اگر (!doExpandWide(nodep)) بازگشت نادرست;
        didPattern(VExpandPattern::ASSIGN_XOR, nodep);
        File_سطر* ثابت fl = nodep->fileline();
        برای (صحیح w = 0; w < nodep->width_واژگان(); ++w) {
            addWordAssign(nodep, w,
                          جدید AstXor{fl, newAstWordSelClone(rhsp->lhsp(), w),
                                     newAstWordSelClone(rhsp->rhsp(), w)});
        }
        بازگشت درست;
    }
    منطقی expandWideShift(Ast_گره_Assign* nodep, Ast_گره_Biop* rhsp, منطقی is_چپ_Shift) {
        اگر (!doExpandWide(nodep)) بازگشت نادرست;

        // Simplify the shift amount, in case it becomes a constant
        V3Const::constifyEditCpp(rhsp->rhsp());

        // If it's a constant shift by whole words, expand it so V3Subst can substitute it
        اگر (ثابت AstConst* ثابت rhsConstp = VN_CAST(rhsp->rhsp(), Const)) {
            ثابت uint32_t shiftBits = rhsConstp->toUInt();
            اگر (VL_BITBIT_E(shiftBits) == 0) {
                ثابت صحیح width_واژگان = nodep->width_واژگان();
                ثابت صحیح shift_واژگان = std::min<صحیح>(VL_BITWORD_E(shiftBits), width_واژگان);
                File_سطر* ثابت flp = rhsp->fileline();
                اگر (is_چپ_Shift) {
                    didPattern(VExpandPattern::ASSIGN_SHIFTL, nodep);
                    // Low words of the result are zero
                    برای (صحیح w = 0; w < shift_واژگان; ++w) {
                        addWordAssign(nodep, w, جدید AstConst{flp, AstConst::SizedEData{}, 0});
                    }
                    // High words of the result are copied from higher words of the source
                    برای (صحیح w = shift_واژگان; w < width_واژگان; ++w) {
                        addWordAssign(nodep, w, newAstWordSelClone(rhsp->lhsp(), w - shift_واژگان));
                    }
                } وگرنه {
                    didPattern(VExpandPattern::ASSIGN_SHIFTR, nodep);
                    // Low words of the result are copied from higher words of the source
                    برای (صحیح w = 0; w < width_واژگان - shift_واژگان; ++w) {
                        addWordAssign(nodep, w, newAstWordSelClone(rhsp->lhsp(), w + shift_واژگان));
                    }
                    // High words of the result are zero
                    برای (صحیح w = width_واژگان - shift_واژگان; w < width_واژگان; ++w) {
                        addWordAssign(nodep, w, جدید AstConst{flp, AstConst::SizedEData{}, 0});
                    }
                }
                بازگشت درست;
            }
        }

        بازگشت نادرست;
    }

    //-------- Triops
    منطقی expandWide(Ast_گره_Assign* nodep, AstCond* rhsp) {
        اگر (!doExpandWide(nodep)) بازگشت نادرست;
        didPattern(VExpandPattern::ASSIGN_COND, nodep);
        File_سطر* ثابت fl = nodep->fileline();
        برای (صحیح w = 0; w < nodep->width_واژگان(); ++w) {
            addWordAssign(nodep, w,
                          جدید AstCond{fl, rhsp->condp()->clone_درخت_Pure(درست),
                                      newAstWordSelClone(rhsp->thenp(), w),
                                      newAstWordSelClone(rhsp->elsep(), w)});
        }
        بازگشت درست;
    }

    // VISITORS
    تهی visit(AstNetlist* nodep) override { iterateAndNextNull(nodep->modulesp()); }

    تهی visit(AstCFunc* nodep) override {
        VL_RESTORER(m_funcp);
        VL_RESTORER(m_nTmps);
        m_funcp = nodep;
        m_nTmps = 0;
        ثابت VDouble0 statWidesBefore = m_statWides;
        iterateChildren(nodep);

        // Constant fold here if anything was expanded, as Ast size can likely be reduced
        اگر (v3Global.opt.fConstEager() && m_statWides != statWidesBefore) {
            Ast_گره* ثابت editedp = V3Const::constifyEditCpp(nodep);
            UASSERT_OBJ(editedp == nodep, editedp, "Should not have replaced CFunc");
        }
    }

    تهی visit(AstExtend* nodep) override {
        اگر (nodep->user1SetOnce()) بازگشت;  // Process once
        iterateChildren(nodep);
        اگر (nodep->isWide()) {
            // See under ASSIGN(EXTEND)
        } وگرنه {
            اگر (isImpure(nodep)) بازگشت;
            Ast_گره_عبارت* ثابت lhsp = nodep->lhsp()->unlinkFrBack();
            Ast_گره_عبارت* newp = lhsp;
            اگر (nodep->isQuad()) {
                اگر (lhsp->isQuad()) {
                    lhsp->dtypeFrom(nodep);  // Just mark it, else nop
                } وگرنه اگر (lhsp->isWide()) {
                    nodep->v3fatalSrc("extending larger thing into smaller?");
                } وگرنه {
                    newp = جدید AstCCast{nodep->fileline(), lhsp, nodep};
                }
            } وگرنه {  // Long
                UASSERT_OBJ(!(lhsp->isQuad() || lhsp->isWide()), nodep,
                            "extending larger thing into smaller?");
                lhsp->dtypeFrom(nodep);  // Just mark it, else nop
            }
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        }
    }
    منطقی expandWide(Ast_گره_Assign* nodep, AstExtend* rhsp) {
        اگر (!doExpandWide(nodep)) بازگشت نادرست;
        didPattern(VExpandPattern::ASSIGN_EXTEND, nodep);
        Ast_گره_عبارت* ثابت rlhsp = rhsp->lhsp();
        برای (صحیح w = 0; w < rlhsp->width_واژگان(); ++w) {
            addWordAssign(nodep, w, newAstWordSelClone(rlhsp, w));
        }
        برای (صحیح w = rlhsp->width_واژگان(); w < nodep->width_واژگان(); ++w) {
            addWordAssign(nodep, w, جدید AstConst{nodep->fileline(), AstConst::SizedEData{}, 0});
        }
        بازگشت درست;
    }

    تهی visit(AstSel* nodep) override {
        اگر (nodep->user1SetOnce()) بازگشت;  // Process once
        iterateChildren(nodep);

        ثابت uint32_t width = static_cast<uint32_t>(nodep->widthConst());
        UASSERT_OBJ(nodep->widthMin() == static_cast<صحیح>(width), nodep, "Width mismatch");

        // Skip if Sel is an LHS assignment select
        اگر (Ast_گره_Assign* ثابت assignp = VN_CAST(nodep->backp(), گره_Assign)) {
            اگر (nodep == assignp->lhsp()) بازگشت;
        }

        // Skip if wide: See under ASSIGN(WIDE)
        اگر (nodep->isWide()) بازگشت;

        // Skip Sel from stream or queue
        اگر (VN_IS(nodep->fromp()->dtypep(), StreamDType)) بازگشت;
        اگر (VN_IS(nodep->fromp()->dtypep(), QueueDType)) بازگشت;

        // Result must be non-wide after skipping all the above
        UASSERT_OBJ(width <= 64, nodep, "Inconsistent result width");

        اگر (nodep->fromp()->isWide()) {  // Long/Quad from Wide
            اگر (isImpure(nodep)) بازگشت;
            didPattern(VExpandPattern::SEL_FROM_WIDE, nodep);
            File_سطر* ثابت nfl = nodep->fileline();
            File_سطر* ثابت lfl = nodep->lsbp()->fileline();
            File_سطر* ثابت ffl = nodep->fromp()->fileline();
            Ast_گره_عبارت* ثابت fromp = nodep->fromp();

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
            ثابت auto tryFold = [&](Ast_گره_عبارت* exprp) -> Ast_گره_عبارت* {
                // Size limit for inlining expressions (any arbitrary value results in
                // a bounded expansion. Picked a value to allow common simple forms.)
                ایستا constexpr صحیح EXPAND_SEL_LSB_LIMIT = 8;
                // Simplify
                exprp = V3Const::constifyEditCpp(exprp);
                // Accept if not larger than the limit
                اگر (!exprp->isLargerThan(EXPAND_SEL_LSB_LIMIT)) بازگشت exprp;
                // Delete if rejected
                VL_DO_DANGLING(exprp->delete_درخت(), exprp);
                بازگشت nullptr;
            };

            // Return the index of the word holding the bit 'offset' bits above the LSB
            ثابت auto wordIdx = [&](uint32_t offset) -> Ast_گره_عبارت* {
                Ast_گره_عبارت* ثابت msbp = جدید AstAdd{lfl, جدید AstConst{lfl, offset},
                                                     nodep->lsbp()->clone_درخت_Pure(نادرست)};
                Ast_گره_عبارت* ثابت idxp = newWordIndex(msbp);
                اگر (!msbp->backp()) VL_DO_DANGLING(msbp->delete_درخت(), msbp);
                بازگشت idxp;
            };

            // Bit index of the selected LSB within its word
            Ast_گره_عبارت* lBitp = tryFold(newSelBitBit(nodep->lsbp()));
            ثابت منطقی aligned = lBitp && lBitp->isZero();

            // Which of the other 2 words the expansion might need
            ثابت منطقی mNeeded = width > 1 && !aligned;
            ثابت منطقی hNeeded = width > VL_EDATASIZE;
            UASSERT_OBJ(!hNeeded || nodep->isQuad(), nodep, "Width mismatch");

            // Word indices
            ثابت uint32_t mMsbOffset = std::min<uint32_t>(width, VL_EDATASIZE) - 1;
            ثابت uint32_t hMsbOffset = width - 1;
            Ast_گره_عبارت* lIdxp = tryFold(newWordIndex(nodep->lsbp()));
            Ast_گره_عبارت* mIdxp = mNeeded ? tryFold(wordIdx(mMsbOffset)) : nullptr;
            Ast_گره_عبارت* hIdxp = hNeeded ? tryFold(wordIdx(hMsbOffset)) : nullptr;

            // If the LSB expression is needed multiple times, evaluate it into a temporary
            ثابت صحیح nCopies = !lBitp + !lIdxp + (mNeeded && !mIdxp) + (hNeeded && !hIdxp);
            اگر (nCopies > 1 && m_funcp && m_stmtp && !isImpure(m_stmtp)) {
                ++m_nTmps;  // Use fresh set of temporaries
                Ast_گره_عبارت* ثابت lsbp = nodep->lsbp()->unlinkFrBack();
                AstVar* ثابت tmpp = addLocalTmp(m_stmtp, "ExpandSel_Lsb", lsbp);
                nodep->lsbp(جدید AstVarRef{lfl, tmpp, VAccess::READ});
            }

            // Rebuild remaining expressions derived from LSB if needed, now using the temporary
            اگر (!lBitp) lBitp = newSelBitBit(nodep->lsbp());
            اگر (!lIdxp) lIdxp = newWordIndex(nodep->lsbp());
            اگر (!mIdxp) mIdxp = mNeeded ? wordIdx(mMsbOffset) : nullptr;
            اگر (!hIdxp) hIdxp = hNeeded ? wordIdx(hMsbOffset) : nullptr;

            // Return word 'idxp' of 'fromp', without consuming 'idxp'
            ثابت auto wordSel = [&](Ast_گره_عبارت* idxp) -> Ast_گره_عبارت* {
                Ast_گره_عبارت* ثابت clonep = idxp->clone_درخت_Pure(نادرست);
                Ast_گره_عبارت* ثابت wordp = newWordSelWord(ffl, fromp, clonep);
                اگر (!clonep->backp()) VL_DO_DANGLING(clonep->delete_درخت(), clonep);
                بازگشت wordp;
            };

            // Construct term containing the bits of the low word - always needed
            Ast_گره_عبارت* ثابت lTermp = [&]() -> Ast_گره_عبارت* {
                Ast_گره_عبارت* lWordp = wordSel(lIdxp);
                اگر (nodep->isQuad()) lWordp = جدید AstCCast{nfl, lWordp, nodep};
                بازگشت جدید AstShiftR{nfl, lWordp, lBitp->clone_درخت_Pure(نادرست), nodep->width()};
            }();

            // Construct term containing the bits of the middle word - if needed
            Ast_گره_عبارت* ثابت mTermp = [&]() -> Ast_گره_عبارت* {
                اگر (!mNeeded) بازگشت nullptr;

                Ast_گره_عبارت* mWordp = wordSel(mIdxp);
                اگر (nodep->isQuad()) mWordp = جدید AstCCast{nfl, mWordp, nodep};
                Ast_گره_عبارت* ثابت mShiftp = جدید AstSub{lfl, جدید AstConst{lfl, VL_EDATASIZE},
                                                        lBitp->clone_درخت_Pure(نادرست)};
                // If we're selecting bit zero, then all 32 bits in the mid word
                // get shifted << by 32 bits, so ignore them.
                ثابت V3_عدد zero{nodep, longOrQuadWidth(nodep)};
                بازگشت جدید AstCond{
                    nfl,
                    // lsb % VL_EDATASIZE == 0 ?
                    جدید AstEq{nfl, جدید AstConst{nfl, 0}, lBitp->clone_درخت_Pure(نادرست)},
                    // 0 :
                    جدید AstConst{nfl, zero},
                    //  midword >> (VL_EDATASIZE - (lbs % VL_EDATASIZE))
                    جدید AstShiftL{nfl, mWordp, mShiftp, nodep->width()}};
            }();

            // Construct term containing the bits of the high word - if needed
            Ast_گره_عبارت* ثابت hTermp = [&]() -> Ast_گره_عبارت* {
                اگر (!hNeeded) بازگشت nullptr;

                Ast_گره_عبارت* hWordp = wordSel(hIdxp);
                hWordp = جدید AstCCast{nfl, hWordp, nodep};
                Ast_گره_عبارت* ثابت hShiftp = جدید AstCond{
                    nfl,
                    // lsb % VL_EDATASIZE == 0 ?
                    جدید AstEq{nfl, جدید AstConst{nfl, 0}, lBitp->clone_درخت_Pure(نادرست)},
                    // VL_EDATASIZE :
                    جدید AstConst{lfl, VL_EDATASIZE},
                    // 64 - (lbs % VL_EDATASIZE)
                    جدید AstSub{lfl, جدید AstConst{lfl, 64}, lBitp->clone_درخت_Pure(نادرست)}};
                بازگشت جدید AstShiftL{nfl, hWordp, hShiftp, nodep->width()};
            }();

            // Delete parts not captured during construction
            VL_DO_DANGLING(lBitp->delete_درخت(), lBitp);
            VL_DO_DANGLING(lIdxp->delete_درخت(), lIdxp);
            اگر (mIdxp) VL_DO_DANGLING(mIdxp->delete_درخت(), mIdxp);
            اگر (hIdxp) VL_DO_DANGLING(hIdxp->delete_درخت(), hIdxp);

            // Or reduce the terms
            Ast_گره_عبارت* newp = lTermp;
            اگر (mTermp) newp = جدید AstOr{nfl, mTermp, newp};
            اگر (hTermp) newp = جدید AstOr{nfl, hTermp, newp};
            newp->dtypeFrom(nodep);
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        } وگرنه {  // Long/Quad from Long/Quad
            // No isImpure() check - can handle side effects in below
            didPattern(VExpandPattern::SEL_FROM_NARROW, nodep);
            File_سطر* ثابت fl = nodep->fileline();
            Ast_گره_عبارت* fromp = nodep->fromp()->unlinkFrBack();
            Ast_گره_عبارت* ثابت lsbp = nodep->lsbp()->unlinkFrBack();
            اگر (nodep->isQuad() && !fromp->isQuad()) fromp = جدید AstCCast{fl, fromp, nodep};
            // {large}>>32 requires 64-bit shift operation; then cast
            Ast_گره_عبارت* newp = جدید AstShiftR{fl, fromp, lsbp, fromp->width()};
            newp->dtypeFrom(fromp);
            اگر (!nodep->isQuad() && fromp->isQuad()) newp = جدید AstCCast{fl, newp, nodep};
            newp->dtypeFrom(nodep);
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        }
    }

    منطقی expandWide(Ast_گره_Assign* nodep, AstSel* rhsp) {
        UASSERT_OBJ(nodep->widthMin() == rhsp->widthConst(), nodep, "Width mismatch");
        اگر (!doExpandWide(nodep)) بازگشت نادرست;

        // Simplify the index, in case it becomes a constant
        V3Const::constifyEditCpp(rhsp->lsbp());

        // If it's a constant select and aligned, we can just copy the words
        اگر (ثابت AstConst* ثابت lsbConstp = VN_CAST(rhsp->lsbp(), Const)) {
            ثابت uint32_t lsb = lsbConstp->toUInt();
            اگر (VL_BITBIT_E(lsb) == 0) {
                didPattern(VExpandPattern::ASSIGN_SEL_ALIGNED, nodep);
                ثابت uint32_t word = VL_BITWORD_E(lsb);
                برای (صحیح w = 0; w < nodep->width_واژگان(); ++w) {
                    addWordAssign(nodep, w, newAstWordSelClone(rhsp->fromp(), w + word));
                }
                بازگشت درست;
            }
        }

        didPattern(VExpandPattern::ASSIGN_SEL_UNALIGNED, nodep);
        File_سطر* ثابت flp = rhsp->fileline();

        // Use fresh set of temporaries
        ++m_nTmps;

        // Compute word index of LSB, store to temporary if not constant
        Ast_گره_عبارت* ثابت wordLsbp = rhsp->lsbp()->clone_درخت_Pure(نادرست);
        Ast_گره_عبارت* wordIdxp = newWordIndex(wordLsbp);
        اگر (!wordLsbp->backp()) VL_DO_DANGLING(wordLsbp->delete_درخت(), wordLsbp);
        wordIdxp = V3Const::constifyEditCpp(wordIdxp);
        اگر (!VN_IS(wordIdxp, Const)) {
            AstVar* ثابت tmpp = addLocalTmp(nodep, "ExpandSel_WordIdx", wordIdxp);
            wordIdxp = جدید AstVarRef{flp, tmpp, VAccess::READ};
        }

        // Compute shift amounts and mask, store to temporaries if not constants
        Ast_گره_عبارت* loShftp = nullptr;
        Ast_گره_عبارت* hiShftp = nullptr;
        Ast_گره_عبارت* hiMaskp = nullptr;
        اگر (ثابت AstConst* ثابت lsbConstp = VN_CAST(rhsp->lsbp(), Const)) {
            ثابت uint32_t bitOffset = VL_BITBIT_E(lsbConstp->toUInt());
            // Must be unaligned, otherwise we would have handled it above
            UASSERT_OBJ(bitOffset, nodep, "Missed aligned wide select");
            loShftp = جدید AstConst{flp, bitOffset};
            hiShftp = جدید AstConst{flp, VL_EDATASIZE - bitOffset};
            hiMaskp = nullptr;
        } وگرنه {
            // Compute Low word shift amount: bottom bits of lsb index
            Ast_گره_عبارت* ثابت lsbp = rhsp->lsbp()->clone_درخت_Pure(نادرست);
            loShftp = جدید AstAnd{flp, جدید AstConst{flp, VL_SIZEBITS_E}, lsbp};
            AstVar* ثابت loTmpp = addLocalTmp(nodep, "ExpandSel_LoShift", loShftp);
            loShftp = جدید AstVarRef{flp, loTmpp, VAccess::READ};

            // Compute if aligned: Low word shift amount is 0
            Ast_گره_عبارت* ثابت zerop = جدید AstConst{flp, 0};
            Ast_گره_عبارت* alignedp = جدید AstEq{flp, zerop, loShftp->clone_درخت_Pure(نادرست)};
            AstVar* ثابت alignedTmpp = addLocalTmp(nodep, "ExpandSel_Aligned", alignedp);
            alignedp = جدید AstVarRef{flp, alignedTmpp, VAccess::READ};

            // Computed High word shift amount: 0 if aligned else VL_EDATASIZE - Low word shift
            Ast_گره_عبارت* ثابت edsp = جدید AstConst{flp, VL_EDATASIZE};
            Ast_گره_عبارت* ثابت subp = جدید AstSub{flp, edsp, loShftp->clone_درخت_Pure(نادرست)};
            hiShftp = جدید AstCond{flp, alignedp, جدید AstConst{flp, 0}, subp};
            AstVar* ثابت hiTmpp = addLocalTmp(nodep, "ExpandSel_HiShift", hiShftp);
            hiShftp = جدید AstVarRef{flp, hiTmpp, VAccess::READ};

            // Compute the High word mask: 0 if aligned, ones otherwise
            hiMaskp = جدید AstCond{flp, alignedp->clone_درخت_Pure(نادرست),
                                  جدید AstConst{flp, AstConst::SizedEData{}, 0},
                                  جدید AstConst{flp, AstConst::SizedEData{}, -1ULL}};
            AstVar* ثابت maskTmpp = addLocalTmp(nodep, "ExpandSel_HiMask", hiMaskp);
            hiMaskp = جدید AstVarRef{flp, maskTmpp, VAccess::READ};
        }

        // Create each word of the selected result
        Ast_گره_عبارت* ثابت fromp = rhsp->fromp();
        ثابت صحیح sel_واژگان = nodep->width_واژگان();
        برای (صحیح w = 0; w < sel_واژگان; ++w) {
            // Grab bits from word 'VL_BITWORD_E(lsb) + w'
            Ast_گره_عبارت* ثابت loWordp = newWordSelWord(fromp->fileline(), fromp, wordIdxp, w);
            Ast_گره_عبارت* ثابت loShftClonep = loShftp->clone_درخت_Pure(نادرست);
            Ast_گره_عبارت* ثابت lop = جدید AstShiftR{flp, loWordp, loShftClonep, VL_EDATASIZE};
            // Grab bits from word 'VL_BITWORD_E(lsb) + w + 1'
            Ast_گره_عبارت* hiWordp = newWordSelWord(fromp->fileline(), fromp, wordIdxp, w + 1);
            // For the last word of the result, avoid an OOB access when the Sel is not OOB
            اگر (w == sel_واژگان - 1) {
                ثابت uint32_t max = fromp->width_واژگان() - w - 1;
                Ast_گره_عبارت* ثابت maxp = جدید AstConst{flp, max};
                Ast_گره_عبارت* ثابت condp = جدید AstGte{flp, wordIdxp->clone_درخت_Pure(نادرست), maxp};
                Ast_گره_عبارت* ثابت zerop = جدید AstConst{flp, AstConst::SizedEData{}, 0};
                hiWordp = جدید AstCond{flp, condp, zerop, hiWordp};
            }
            Ast_گره_عبارت* ثابت hiShftClonep = hiShftp->clone_درخت_Pure(نادرست);
            Ast_گره_عبارت* ثابت hiPartp = جدید AstShiftL{flp, hiWordp, hiShftClonep, VL_EDATASIZE};
            Ast_گره_عبارت* ثابت hip
                = hiMaskp ? جدید AstAnd{flp, hiPartp, hiMaskp->clone_درخت_Pure(نادرست)} : hiPartp;
            // Combine them
            addWordAssign(nodep, w, جدید AstOr{nodep->fileline(), hip, lop});
        }

        // Delete parts not captured during construction
        اگر (!wordIdxp->backp()) VL_DO_DANGLING(wordIdxp->delete_درخت(), wordIdxp);
        اگر (!loShftp->backp()) VL_DO_DANGLING(loShftp->delete_درخت(), loShftp);
        اگر (!hiShftp->backp()) VL_DO_DANGLING(hiShftp->delete_درخت(), hiShftp);
        اگر (hiMaskp && !hiMaskp->backp()) VL_DO_DANGLING(hiMaskp->delete_درخت(), hiMaskp);

        بازگشت درست;
    }

    منطقی expandLhs(Ast_گره_Assign* nodep, AstSel* lhsp) {
        // Possibilities
        //      destp: wide or narrow
        //      rhsp:  wide (destp must be wide), narrow, or 1 bit wide
        //      rhsp:  may be allones and can remove AND NOT gate
        //      lsbp:  constant or variable
        // Yuk.
        اگر (isImpure(nodep)) بازگشت نادرست;
        File_سطر* ثابت nfl = nodep->fileline();
        File_سطر* ثابت lfl = lhsp->fileline();
        ثابت منطقی destwide = lhsp->fromp()->isWide();
        ثابت منطقی ones = nodep->rhsp()->isAllOnesV();
        اگر (VN_IS(lhsp->lsbp(), Const)) {
            // The code should work without this constant test, but it won't
            // constify as nicely as we'd like.
            Ast_گره_عبارت* rhsp = nodep->rhsp()->unlinkFrBack();
            Ast_گره_عبارت* ثابت destp = lhsp->fromp()->unlinkFrBack();
            ثابت صحیح lsb = lhsp->lsbConst();
            ثابت صحیح msb = lhsp->msbConst();
            V3_عدد maskset{nodep, destp->widthMin()};
            maskset.setMask(msb + 1 - lsb, lsb);
            V3_عدد maskold{nodep, destp->widthMin()};
            maskold.opNot(maskset);
            اگر (destwide) {
                didPattern(VExpandPattern::ASSIGN_TO_SEL_CONST_WIDE, nodep);
                برای (صحیح w = 0; w < destp->width_واژگان(); ++w) {
                    اگر (w >= VL_BITWORD_E(lsb) && w <= VL_BITWORD_E(msb)) {
                        // else we would just be setting it to the same exact value
                        Ast_گره_عبارت* oldvalp = newAstWordSelClone(destp, w);
                        fixCloneLvalue(oldvalp);
                        اگر (!ones) {
                            oldvalp = جدید AstAnd{
                                lfl,
                                جدید AstConst{lfl, AstConst::SizedEData{}, maskold.edataWord(w)},
                                oldvalp};
                        }

                        // Appropriate word of new value to insert:
                        Ast_گره_عبارت* newp = newWordGrabShift(lfl, w, rhsp, lsb);

                        // Apply cleaning at the top word of the destination
                        // (no cleaning to do if dst's width is a whole number
                        // of words).
                        اگر (w == destp->width_واژگان() - 1 && VL_BITBIT_E(destp->widthMin()) != 0) {
                            V3_عدد cleanmask{nodep, VL_EDATASIZE};
                            cleanmask.setMask(VL_BITBIT_E(destp->widthMin()));
                            newp = جدید AstAnd{lfl, newp, جدید AstConst{lfl, cleanmask}};
                        }
                        Ast_گره_عبارت* ثابت orp
                            = V3Const::constifyEditCpp(جدید AstOr{lfl, oldvalp, newp});
                        addWordAssign(nodep, w, destp, orp);
                    }
                }
                VL_DO_DANGLING(rhsp->delete_درخت(), rhsp);
                VL_DO_DANGLING(destp->delete_درخت(), destp);
            } وگرنه {
                didPattern(VExpandPattern::ASSIGN_TO_SEL_CONST_NARROW, nodep);
                اگر (destp->isQuad() && !rhsp->isQuad()) rhsp = جدید AstCCast{nfl, rhsp, nodep};
                Ast_گره_عبارت* oldvalp = destp->clone_درخت_Pure(درست);
                fixCloneLvalue(oldvalp);
                اگر (!ones) oldvalp = جدید AstAnd{lfl, جدید AstConst{lfl, maskold}, oldvalp};

                // The bit-select can refer to bits outside the width of nodep
                // which we aren't allowed to assign to.  This is a mask of the
                // valid range of nodep which we apply to the new shifted RHS.
                V3_عدد cleanmask{nodep, destp->widthMin()};
                cleanmask.setMask(destp->widthMin());
                Ast_گره_عبارت* ثابت shifted = جدید AstShiftL{
                    lfl, rhsp, جدید AstConst{lfl, static_cast<uint32_t>(lsb)}, destp->width()};
                Ast_گره_عبارت* ثابت cleaned
                    = جدید AstAnd{lfl, shifted, جدید AstConst{lfl, cleanmask}};
                Ast_گره_عبارت* ثابت orp
                    = V3Const::constifyEditCpp(جدید AstOr{lfl, oldvalp, cleaned});
                درج_Before(nodep, جدید AstAssign{nfl, destp, orp});
            }
            بازگشت درست;
        } وگرنه {  // non-const select offset
            اگر (destwide && lhsp->widthConst() == 1) {
                didPattern(VExpandPattern::ASSIGN_TO_SEL_VAR_WIDE_ONE_BIT, nodep);
                Ast_گره_عبارت* ثابت rhsp = nodep->rhsp()->unlinkFrBack();
                Ast_گره_عبارت* ثابت destp = lhsp->fromp()->unlinkFrBack();
                Ast_گره_عبارت* oldvalp
                    = newWordSelBit(lfl, destp->clone_درخت_Pure(نادرست), lhsp->lsbp());
                fixCloneLvalue(oldvalp);
                اگر (!ones) {
                    oldvalp = جدید AstAnd{
                        lfl,
                        جدید AstNot{
                            lfl, جدید AstShiftL{lfl, جدید AstConst{nfl, 1},
                                               // newSelBitBit may exceed the MSB of this variable.
                                               // That's ok as we'd just AND with a larger value,
                                               // but oldval would clip the upper bits to sanity
                                               newSelBitBit(lhsp->lsbp()), VL_EDATASIZE}},
                        oldvalp};
                }
                // Restrict the shift amount to 0-31, see bug804.
                Ast_گره_عبارت* ثابت shiftp = جدید AstAnd{nfl, lhsp->lsbp()->clone_درخت_Pure(درست),
                                                       جدید AstConst{nfl, VL_EDATASIZE - 1}};
                Ast_گره* ثابت newp = جدید AstAssign{
                    nfl, newWordSelBit(nfl, destp, lhsp->lsbp()),
                    جدید AstOr{lfl, oldvalp, جدید AstShiftL{lfl, rhsp, shiftp, VL_EDATASIZE}}};
                درج_Before(nodep, newp);
                بازگشت درست;
            } وگرنه اگر (destwide) {
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
                بازگشت نادرست;
            } وگرنه {
                didPattern(VExpandPattern::ASSIGN_TO_SEL_VAR_NARROW, nodep);
                // nodep->dumpTree("-  old: ");
                Ast_گره_عبارت* rhsp = nodep->rhsp()->unlinkFrBack();
                Ast_گره_عبارت* ثابت destp = lhsp->fromp()->unlinkFrBack();
                Ast_گره_عبارت* oldvalp = destp->clone_درخت_Pure(درست);
                fixCloneLvalue(oldvalp);

                V3_عدد maskwidth{nodep, destp->widthMin()};
                maskwidth.setMask(lhsp->widthConst());

                اگر (destp->isQuad() && !rhsp->isQuad()) rhsp = جدید AstCCast{nfl, rhsp, nodep};
                اگر (!ones) {
                    oldvalp = جدید AstAnd{
                        lfl,
                        جدید AstNot{lfl, جدید AstShiftL{lfl, جدید AstConst{nfl, maskwidth},
                                                      lhsp->lsbp()->clone_درخت_Pure(درست),
                                                      destp->width()}},
                        oldvalp};
                }
                Ast_گره_عبارت* newp
                    = جدید AstShiftL{lfl, rhsp, lhsp->lsbp()->clone_درخت_Pure(درست), destp->width()};
                // Apply cleaning to the new value being inserted.  Mask is
                // slightly wider than necessary to avoid an AND with all ones
                // being optimized out.  No need to clean if destp is
                // quad-sized as there are no extra bits to contaminate
                اگر (destp->widthMin() != 64) {
                    V3_عدد cleanmask{nodep, destp->widthMin() + 1};
                    cleanmask.setMask(destp->widthMin());
                    newp = جدید AstAnd{lfl, newp, جدید AstConst{lfl, cleanmask}};
                }

                درج_Before(nodep, جدید AstAssign{nfl, destp, جدید AstOr{lfl, oldvalp, newp}});
                بازگشت درست;
            }
        }
    }

    تهی visit(AstConcat* nodep) override {
        اگر (nodep->user1SetOnce()) بازگشت;  // Process once
        iterateChildren(nodep);
        اگر (nodep->isWide()) {
            // See under ASSIGN(WIDE)
        } وگرنه {
            // No isImpure() check - can handle side effects in below
            didPattern(VExpandPattern::CONCAT, nodep);
            File_سطر* ثابت fl = nodep->fileline();
            Ast_گره_عبارت* lhsp = nodep->lhsp()->unlinkFrBack();
            Ast_گره_عبارت* rhsp = nodep->rhsp()->unlinkFrBack();
            ثابت uint32_t rhsshift = rhsp->widthMin();
            اگر (nodep->isQuad() && !lhsp->isQuad()) lhsp = جدید AstCCast{fl, lhsp, nodep};
            اگر (nodep->isQuad() && !rhsp->isQuad()) rhsp = جدید AstCCast{fl, rhsp, nodep};
            Ast_گره_عبارت* ثابت newp = جدید AstOr{
                fl, جدید AstShiftL{fl, lhsp, جدید AstConst{fl, rhsshift}, nodep->width()}, rhsp};
            newp->dtypeFrom(nodep);  // Unsigned
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        }
    }
    منطقی expandWide(Ast_گره_Assign* nodep, AstConcat* rhsp) {
        اگر (!doExpandWide(rhsp)) بازگشت نادرست;
        didPattern(VExpandPattern::ASSIGN_CONCAT, nodep);
        File_سطر* ثابت fl = rhsp->fileline();
        // Lhs or Rhs may be word, long, or quad.
        // newAstWordSelClone nicely abstracts the difference.
        ثابت صحیح rhsshift = rhsp->rhsp()->widthMin();
        // Sometimes doing the words backwards is preferable.
        // When we have x={x,foo} backwards is better, when x={foo,x} forward is better
        // However V3Subst tends to rip this up, so not worth optimizing now.
        برای (صحیح w = 0; w < rhsp->width_واژگان(); ++w) {
            addWordAssign(nodep, w,
                          جدید AstOr{fl, newWordGrabShift(fl, w, rhsp->lhsp(), rhsshift),
                                    newAstWordSelClone(rhsp->rhsp(), w)});
        }
        بازگشت درست;
    }

    تهی visit(AstReplicate* nodep) override {
        اگر (nodep->user1SetOnce()) بازگشت;  // Process once
        iterateChildren(nodep);
        اگر (nodep->isWide()) {
            // See under ASSIGN(WIDE)
        } وگرنه {
            اگر (isImpure(nodep)) بازگشت;
            File_سطر* ثابت fl = nodep->fileline();
            Ast_گره_عبارت* lhsp = nodep->srcp()->unlinkFrBack();
            Ast_گره_عبارت* newp;
            ثابت صحیح lhswidth = lhsp->widthMin();
            اگر (lhswidth == 1) {
                didPattern(VExpandPattern::REPLICATE_ONE_BIT, nodep);
                newp = جدید AstNegate{fl, lhsp};
            } وگرنه {
                didPattern(VExpandPattern::REPLICATE, nodep);
                ثابت AstConst* ثابت constp = VN_AS(nodep->countp(), Const);
                UASSERT_OBJ(constp, nodep,
                            "Replication value isn't a constant.  Checked earlier!");
                ثابت uint32_t times = constp->toUInt();
                اگر (nodep->isQuad() && !lhsp->isQuad()) lhsp = جدید AstCCast{fl, lhsp, nodep};
                newp = lhsp->clone_درخت_Pure(درست);
                برای (unsigned repnum = 1; repnum < times; repnum++) {
                    ثابت صحیح rhsshift = repnum * lhswidth;
                    newp = جدید AstOr{
                        fl,
                        جدید AstShiftL{fl, lhsp->clone_درخت_Pure(درست),
                                      جدید AstConst{fl, static_cast<uint32_t>(rhsshift)},
                                      nodep->width()},
                        newp};
                    newp->dtypeFrom(nodep);  // Unsigned
                }
                VL_DO_DANGLING(lhsp->delete_درخت(), lhsp);  // Never used
            }
            newp->dtypeFrom(nodep);  // Unsigned
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        }
    }
    منطقی expandWide(Ast_گره_Assign* nodep, AstReplicate* rhsp) {
        اگر (!doExpandWide(rhsp)) بازگشت نادرست;
        didPattern(VExpandPattern::ASSIGN_REPLICATE, nodep);
        File_سطر* ثابت fl = nodep->fileline();
        Ast_گره_عبارت* ثابت lhsp = rhsp->srcp();
        ثابت صحیح lhswidth = lhsp->widthMin();
        ثابت AstConst* ثابت constp = VN_AS(rhsp->countp(), Const);
        UASSERT_OBJ(constp, rhsp, "Replication value isn't a constant.  Checked earlier!");
        ثابت uint32_t times = constp->toUInt();
        برای (صحیح w = 0; w < rhsp->width_واژگان(); ++w) {
            Ast_گره_عبارت* newp;
            اگر (lhswidth == 1) {
                newp = جدید AstNegate{fl, lhsp->clone_درخت_Pure(درست)};
                // Replicate always unsigned
                newp->dtypeChgWidthSigned(VL_EDATASIZE, VL_EDATASIZE, VSigning::UNSIGNED);
            } وگرنه {
                newp = newAstWordSelClone(lhsp, w);
                File_سطر* ثابت rfl = rhsp->fileline();
                برای (unsigned repnum = 1; repnum < times; repnum++) {
                    newp = جدید AstOr{fl, newWordGrabShift(rfl, w, lhsp, lhswidth * repnum), newp};
                }
            }
            addWordAssign(nodep, w, newp);
        }
        بازگشت درست;
    }

    تهی visitEqNeq(Ast_گره_Biop* nodep) {
        اگر (nodep->user1SetOnce()) بازگشت;  // Process once
        iterateChildren(nodep);
        اگر (nodep->lhsp()->isWide()
            && !(VN_IS(nodep->lhsp()->dtypep()->skipRefp(), StreamDType)
                 || VN_IS(nodep->rhsp()->dtypep()->skipRefp(), StreamDType))) {
            اگر (isImpure(nodep)) بازگشت;
            اگر (!doExpandWide(nodep->lhsp())) بازگشت;
            اگر (!doExpandWide(nodep->rhsp())) بازگشت;
            didPattern(VN_IS(nodep, Neq) ? VExpandPattern::WORDIZE_NEQ
                                         : VExpandPattern::WORDIZE_EQ,
                       nodep);
            // -> (0=={or{for each_word{WORDSEL(lhs,#)^WORDSEL(rhs,#)}}}
            File_سطر* ثابت fl = nodep->fileline();
            Ast_گره_عبارت* newp = nullptr;
            برای (صحیح w = 0; w < nodep->lhsp()->width_واژگان(); ++w) {
                Ast_گره_عبارت* ثابت eqp = جدید AstXor{fl, newAstWordSelClone(nodep->lhsp(), w),
                                                    newAstWordSelClone(nodep->rhsp(), w)};
                newp = newp ? جدید AstOr{fl, newp, eqp} : eqp;
            }
            اگر (VN_IS(nodep, Neq)) {
                newp = جدید AstNeq{fl, جدید AstConst{fl, AstConst::SizedEData{}, 0}, newp};
            } وگرنه {
                newp = جدید AstEq{fl, جدید AstConst{fl, AstConst::SizedEData{}, 0}, newp};
            }
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        }
    }
    تهی visit(AstEq* nodep) override { visitEqNeq(nodep); }
    تهی visit(AstNeq* nodep) override { visitEqNeq(nodep); }

    تهی visit(AstRedOr* nodep) override {
        اگر (nodep->user1SetOnce()) بازگشت;  // Process once
        iterateChildren(nodep);
        File_سطر* ثابت fl = nodep->fileline();
        اگر (nodep->lhsp()->isWide()) {
            اگر (isImpure(nodep)) بازگشت;
            اگر (!doExpandWide(nodep->lhsp())) بازگشت;
            didPattern(VExpandPattern::WORDIZE_REDOR, nodep);
            // -> (0!={or{for each_word{WORDSEL(lhs,#)}}}
            Ast_گره_عبارت* newp = nullptr;
            برای (صحیح w = 0; w < nodep->lhsp()->width_واژگان(); ++w) {
                Ast_گره_عبارت* ثابت eqp = newAstWordSelClone(nodep->lhsp(), w);
                newp = newp ? جدید AstOr{fl, newp, eqp} : eqp;
            }
            newp = جدید AstNeq{fl, جدید AstConst{fl, AstConst::SizedEData{}, 0}, newp};
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        } وگرنه {
            // No isImpure() check - can handle side effects in below
            didPattern(VExpandPattern::REDOR_TO_NEQ, nodep);
            Ast_گره_عبارت* ثابت lhsp = nodep->lhsp()->unlinkFrBack();
            Ast_گره_عبارت* ثابت newp = جدید AstNeq{
                fl, جدید AstConst{fl, AstConst::Widthed_مقدار{}, longOrQuadWidth(nodep), 0}, lhsp};
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        }
    }
    تهی visit(AstRedAnd* nodep) override {
        اگر (nodep->user1SetOnce()) بازگشت;  // Process once
        iterateChildren(nodep);
        File_سطر* ثابت fl = nodep->fileline();
        اگر (nodep->lhsp()->isWide()) {
            اگر (isImpure(nodep)) بازگشت;
            اگر (!doExpandWide(nodep->lhsp())) بازگشت;
            didPattern(VExpandPattern::WORDIZE_REDAND, nodep);
            // -> (0!={and{for each_word{WORDSEL(lhs,#)}}}
            Ast_گره_عبارت* newp = nullptr;
            برای (صحیح w = 0; w < nodep->lhsp()->width_واژگان(); ++w) {
                Ast_گره_عبارت* eqp = newAstWordSelClone(nodep->lhsp(), w);
                اگر (w == nodep->lhsp()->width_واژگان() - 1) {
                    // Rather than doing a (slowish) ==##, we OR in the
                    // bits that aren't part of the mask
                    eqp = جدید AstOr{fl, جدید AstConst{fl, notWideMask(nodep->lhsp())},
                                    // Bug in cppcheck
                                    // cppcheck-suppress memleak
                                    eqp};
                }
                newp = newp ? جدید AstAnd{fl, newp, eqp} : eqp;
            }
            newp = جدید AstEq{fl, جدید AstConst{fl, AstConst::SizedEData{}, VL_MASK_E(VL_EDATASIZE)},
                             newp};
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        } وگرنه {
            // No isImpure() check - can handle side effects in below
            didPattern(VExpandPattern::REDAND_TO_EQ, nodep);
            Ast_گره_عبارت* ثابت lhsp = nodep->lhsp()->unlinkFrBack();
            Ast_گره_عبارت* ثابت newp = جدید AstEq{fl, جدید AstConst{fl, wordMask(lhsp)}, lhsp};
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        }
    }
    تهی visit(AstRedXor* nodep) override {
        اگر (nodep->user1SetOnce()) بازگشت;  // Process once
        iterateChildren(nodep);
        اگر (nodep->lhsp()->isWide()) {
            اگر (isImpure(nodep)) بازگشت;
            اگر (!doExpandWide(nodep->lhsp())) بازگشت;
            didPattern(VExpandPattern::WORDIZE_REDXOR, nodep);
            // -> (0!={redxor{for each_word{XOR(WORDSEL(lhs,#))}}}
            File_سطر* ثابت fl = nodep->fileline();
            Ast_گره_عبارت* newp = nullptr;
            برای (صحیح w = 0; w < nodep->lhsp()->width_واژگان(); ++w) {
                Ast_گره_عبارت* ثابت eqp = newAstWordSelClone(nodep->lhsp(), w);
                newp = newp ? جدید AstXor{fl, newp, eqp} : eqp;
            }
            newp = جدید AstRedXor{fl, newp};
            UINFO(8, "    Wordize REDXORnew " << newp);
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        }
        // We don't reduce non-wide XORs, as its more efficient to use a temp register,
        // which the inlined function does nicely.
    }

    تهی visit(Ast_گره_Stmt* nodep) override {
        اگر (nodep->user1SetOnce()) بازگشت;  // Process once
        VL_RESTORER(m_stmtp);
        m_stmtp = nodep;
        iterateChildren(nodep);
    }
    تهی visit(Ast_گره_Assign* nodep) override {
        اگر (nodep->user1SetOnce()) بازگشت;  // Process once
        اگر (VN_IS(nodep->dtypep()->skipRefp(), UnpackArrayDType)) {
            بازگشت;  // Skip for UnpackArrayDType
        }

        VL_RESTORER(m_stmtp);
        m_stmtp = nodep;
        iterateChildren(nodep);
        منطقی did = نادرست;
        اگر (nodep->isWide()  //
            && ((VN_IS(nodep->lhsp(), VarRef) || VN_IS(nodep->lhsp(), ArraySel)))
            && !AstVar::scVarRecurse(nodep->lhsp())  // Need special function for SC
            && !AstVar::scVarRecurse(nodep->rhsp())) {
            اگر (AstConst* ثابت rhsp = VN_CAST(nodep->rhsp(), Const)) {
                did = expandWide(nodep, rhsp);
            } وگرنه اگر (AstVarRef* ثابت rhsp = VN_CAST(nodep->rhsp(), VarRef)) {
                did = expandWide(nodep, rhsp);
            } وگرنه اگر (AstSel* ثابت rhsp = VN_CAST(nodep->rhsp(), Sel)) {
                did = expandWide(nodep, rhsp);
            } وگرنه اگر (AstArraySel* ثابت rhsp = VN_CAST(nodep->rhsp(), ArraySel)) {
                did = expandWide(nodep, rhsp);
            } وگرنه اگر (AstConcat* ثابت rhsp = VN_CAST(nodep->rhsp(), Concat)) {
                did = expandWide(nodep, rhsp);
            } وگرنه اگر (AstExtend* ثابت rhsp = VN_CAST(nodep->rhsp(), Extend)) {
                did = expandWide(nodep, rhsp);
            } وگرنه اگر (AstReplicate* ثابت rhsp = VN_CAST(nodep->rhsp(), Replicate)) {
                did = expandWide(nodep, rhsp);
            } وگرنه اگر (AstAnd* ثابت rhsp = VN_CAST(nodep->rhsp(), And)) {
                did = expandWide(nodep, rhsp);
            } وگرنه اگر (AstOr* ثابت rhsp = VN_CAST(nodep->rhsp(), Or)) {
                did = expandWide(nodep, rhsp);
            } وگرنه اگر (AstNot* ثابت rhsp = VN_CAST(nodep->rhsp(), Not)) {
                did = expandWide(nodep, rhsp);
            } وگرنه اگر (AstXor* ثابت rhsp = VN_CAST(nodep->rhsp(), Xor)) {
                did = expandWide(nodep, rhsp);
            } وگرنه اگر (AstShiftL* ثابت rhsp = VN_CAST(nodep->rhsp(), ShiftL)) {
                did = expandWideShift(nodep, rhsp, /* isLeftShift: */ درست);
            } وگرنه اگر (AstShiftR* ثابت rhsp = VN_CAST(nodep->rhsp(), ShiftR)) {
                did = expandWideShift(nodep, rhsp, /* isLeftShift: */ نادرست);
            } وگرنه اگر (AstCond* ثابت rhsp = VN_CAST(nodep->rhsp(), Cond)) {
                did = expandWide(nodep, rhsp);
            }
        } وگرنه اگر (AstSel* ثابت lhsp = VN_CAST(nodep->lhsp(), Sel)) {
            did = expandLhs(nodep, lhsp);
        }
        // Cleanup common code
        اگر (did) VL_DO_DANGLING(nodep->unlinkFrBack()->delete_درخت(), nodep);
    }

    //--------------------
    تهی visit(AstVar*) override {}  // Don't hit varrefs under vars
    تهی visit(Ast_گره* nodep) override { iterateChildren(nodep); }

عمومی:
    // CONSTRUCTORS
    explicit ExpandVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~ExpandVisitor() override {
        V3Stats::addStat("Optimizations, Expand, expanded wides", m_statWides);
        V3Stats::addStat("Optimizations, Expand, expanded wide words", m_stat_Wide_واژگان);
        V3Stats::addStat("Optimizations, Expand, not expanded impure", m_statImpure);
        V3Stats::addStat("Optimizations, Expand, not expanded limit", m_statWideLimited);
        V3Stats::addStat("Optimizations, Expand, temporaries created", m_statTemps);
        ثابت auto emitStat = [این](VExpandPattern pattern) {
            std::string str{pattern.ascii()};
            std::transform(str.begin(), str.end(), str.begin(), [](unsigned نویسه c) {  //
                بازگشت c == '_' ? ' ' : std::tolower(c);
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

تهی V3Expand::expandAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        ثابت ExpandOkVisitor okVisitor{nodep};
        ExpandVisitor{nodep};
    }  // Destruct before checking
    V3Global::dump_Check_Global_درخت("expand", 0, dump_درخت_Either_Level() >= 3);
}
