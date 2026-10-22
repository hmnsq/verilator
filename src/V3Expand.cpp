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

#அந்தர்பாவ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#அந்தர்பாவ "V3Expand.h"

#அந்தர்பாவ "V3Const.h"
#அந்தர்பாவ "V3Stats.h"

#அந்தர்பாவ <algorithm>
#அந்தர்பாவ <array>

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

வகுப்பு VExpandPattern final {
பொது:
    எண்ணிக்கை en : uint8_t {
#define EXPAND_PATTERN_ID(id) id,
        FOR_EACH_EXPAND_PATTERN(EXPAND_PATTERN_ID)
#undef EXPAND_PATTERN_ID
            _ENUM_END
    };
    எண்ணிக்கை en m_e;
    மாறிலி எழுத்து* ascii() மாறிலி {
        நிலையான மாறிலி எழுத்து* மாறிலி names[] = {
#define EXPAND_PATTERN_NAME(id) #id,
            FOR_EACH_EXPAND_PATTERN(EXPAND_PATTERN_NAME)
#undef EXPAND_PATTERN_NAME
                "_ENUM_END"  //
        };
        திரும்பு names[m_e];
    }

    // cppcheck-suppress noExplicitConstructor
    VExpandPattern(en _e)
        : m_e{_e} {}
    operator en() மாறிலி { திரும்பு m_e; }
};

//######################################################################
// Find nodes with side effects, to mark as non-expandable

வகுப்பு ExpandOkVisitor final : பொது VNVisitorConst {
    // NODE STATE
    //  AstNode::user2()        -> bool.  Is pure (along with all children)
    மாறிலி VNUser2InUse m_inuser2;

    // STATE - for current visit position (use VL_RESTORER)
    // Tracks similar to AstNode::isTreePureRecurse(), but avoid O(n^2)
    // False = pure, as nodes that ExpandVisitor inserts preserve pureness
    தருக்க m_isImpure = உண்மை;  // Currently pure

    வெறுமை visit(Ast_முனை* nodep) override {
        தருக்க selfImpure = !nodep->isPure();
        {
            VL_RESTORER(m_isImpure);
            m_isImpure = பொய்;
            iterateChildrenConst(nodep);
            selfImpure |= m_isImpure;
            nodep->user2(selfImpure);
        }
        m_isImpure |= selfImpure;
    }

பொது:
    // CONSTRUCTORS
    explicit ExpandOkVisitor(AstNetlist* nodep) { iterateConst(nodep); }
    ~ExpandOkVisitor() = இயல்பு;
};

//######################################################################
// Expand state, as a visitor of each AstNode

வகுப்பு ExpandVisitor final : பொது VNVisitor {
    // NODE STATE
    //  AstNode::user1()        -> bool.  Processed
    //  AstNode::user2()        -> See ExpandOkVisitor
    மாறிலி VNUser1InUse m_inuser1;

    // STATE - for current visit position (use VL_RESTORER)
    Ast_முனை* m_stmtp = nullptr;  // Current statement

    // STATE - across all visitors
    AstCFunc* m_funcp = nullptr;  // Current function
    VDouble0 m_statImpure;  // Statistic tracking
    VDouble0 m_statWides;  // Statistic tracking
    VDouble0 m_stat_Wide_சொற்கள்;  // Statistic tracking
    VDouble0 m_statWideLimited;  // Statistic tracking
    VDouble0 m_statTemps;  // Statistic tracking
    // Number of applications of each expansion pattern for statistics
    std::array<VDouble0, VExpandPattern::_ENUM_END> m_statPatterns;

    // STATE - for current function
    size_t m_nTmps = 0;  // Sequence numbers for temopraries

    // METHODS
    // Note the application of the given expansion pattern to 'nodep'
    வெறுமை didPattern(VExpandPattern pattern, Ast_முனை* nodep) {
        UINFO(8, "    " << pattern.ascii() << " " << nodep);
        ++m_statPatterns[pattern];
    }

    // Use state that ExpandOkVisitor calculated
    நிலையான தருக்க isImpure(Ast_முனை* nodep) {
        மாறிலி தருக்க impure = nodep->user2();
        எனில் (impure) UINFO(9, "      impure " << nodep);
        திரும்பு impure;
    }

    தருக்க doExpandWide(Ast_முனை* nodep) {
        எனில் (isImpure(nodep)) {
            ++m_statImpure;
            திரும்பு பொய்;
        }
        எனில் (nodep->width_சொற்கள்() <= v3Global.opt.expandLimit()) {
            ++m_statWides;
            m_stat_Wide_சொற்கள் += nodep->width_சொற்கள்();
            திரும்பு உண்மை;
        } இல்லையெனில் {
            ++m_statWideLimited;
            திரும்பு பொய்;
        }
    }

    நிலையான முழு longOrQuadWidth(Ast_முனை* nodep) {
        திரும்பு (nodep->width() + (VL_EDATASIZE - 1)) & ~(VL_EDATASIZE - 1);
    }
    நிலையான V3_எண் notWideMask(Ast_முனை* nodep) {
        திரும்பு V3_எண்{nodep, VL_EDATASIZE, ~VL_MASK_E(nodep->widthMin())};
    }
    நிலையான V3_எண் wordMask(Ast_முனை* nodep) {
        எனில் (nodep->isWide()) {
            திரும்பு V3_எண்{nodep, VL_EDATASIZE, VL_MASK_E(nodep->widthMin())};
        } இல்லையெனில் {
            V3_எண் mask{nodep, longOrQuadWidth(nodep)};
            mask.setMask(nodep->widthMin());
            திரும்பு mask;
        }
    }

    நிலையான வெறுமை செருகு_Before(Ast_முனை* placep, Ast_முனை* newp) {
        newp->user1(1);  // Already processed, don't need to re-iterate
        placep->addHereThisAsNext(newp);
    }
    நிலையான வெறுமை replaceWithDelete(Ast_முனை* nodep, Ast_முனை* newp) {
        newp->user1(1);  // Already processed, don't need to re-iterate
        எனில் (newp->width() != nodep->width()) {
            UASSERT_OBJ(newp->widthMin() == nodep->widthMin(), nodep,
                        "Replacement width mismatch");
            newp->dtypeChgWidth(nodep->width(), nodep->widthMin());
        }
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_மரம்(), nodep);
    }
    நிலையான Ast_முனை* newWordAssign(Ast_முனை_Assign* placep, முழு word, Ast_முனை_கோவை* lhsp,
                                  Ast_முனை_கோவை* rhsp) {
        File_வரி* மாறிலி fl = placep->fileline();
        திரும்பு புதிய AstAssign{fl,
                             புதிய AstWordSel{fl, lhsp->clone_மரம்_Pure(உண்மை),
                                            புதிய AstConst{fl, static_cast<uint32_t>(word)}},
                             rhsp};
    }
    நிலையான வெறுமை addWordAssign(Ast_முனை_Assign* placep, முழு word, Ast_முனை_கோவை* lhsp,
                              Ast_முனை_கோவை* rhsp) {
        செருகு_Before(placep, newWordAssign(placep, word, lhsp, rhsp));
    }
    நிலையான வெறுமை addWordAssign(Ast_முனை_Assign* placep, முழு word, Ast_முனை_கோவை* rhsp) {
        addWordAssign(placep, word, placep->lhsp(), rhsp);
    }

    AstVar* addLocalTmp(Ast_முனை* placep, மாறிலி எழுத்து* namep, Ast_முனை_கோவை* valuep) {
        மாறிலி std::string name = "__V"s + namep + "_"s + std::to_string(m_nTmps);
        File_வரி* மாறிலி flp = placep->fileline();
        AstVar* மாறிலி tmpp = புதிய AstVar{flp, VVarType::STMTTEMP, name, valuep->dtypep()};
        tmpp->funcLocal(உண்மை);
        tmpp->isInternal(உண்மை);
        tmpp->noReset(உண்மை);
        tmpp->substConstOnly(உண்மை);
        m_funcp->addVarsp(tmpp);
        செருகு_Before(placep, புதிய AstAssign{flp, புதிய AstVarRef{flp, tmpp, VAccess::WRITE}, valuep});
        ++m_statTemps;
        திரும்பு tmpp;
    }

    நிலையான வெறுமை fixCloneLvalue(Ast_முனை* nodep) {
        // In AstSel transforms, we call clone() on VarRefs that were lvalues,
        // but are now being used on the RHS of the assignment
        எனில் (VN_IS(nodep, VarRef)) VN_AS(nodep, VarRef)->access(VAccess::READ);
        // Iterate
        எனில் (Ast_முனை* மாறிலி refp = nodep->op1p()) fixCloneLvalue(refp);
        எனில் (Ast_முனை* மாறிலி refp = nodep->op2p()) fixCloneLvalue(refp);
        எனில் (Ast_முனை* மாறிலி refp = nodep->op3p()) fixCloneLvalue(refp);
        எனில் (Ast_முனை* மாறிலி refp = nodep->op4p()) fixCloneLvalue(refp);
    }

    நிலையான Ast_முனை_கோவை* newAstWordSelClone(Ast_முனை_கோவை* nodep, முழு word) {
        // Get the specified word number from a wide array
        // Or, if it's a long/quad, do appropriate conversion to wide
        // Concat may pass negative word numbers, that means it wants a zero
        File_வரி* மாறிலி fl = nodep->fileline();
        எனில் (nodep->isWide() && word >= 0 && word < nodep->width_சொற்கள்()) {
            திரும்பு புதிய AstWordSel{fl, nodep->clone_மரம்_Pure(உண்மை),
                                  புதிய AstConst{fl, static_cast<uint32_t>(word)}};
        } இல்லையெனில் எனில் (nodep->isQuad() && word == 0) {
            Ast_முனை_கோவை* மாறிலி quadfromp = nodep->clone_மரம்_Pure(உண்மை);
            quadfromp->dtypeSetBitUnsized(VL_QUADSIZE, quadfromp->widthMin(), VSigning::UNSIGNED);
            திரும்பு புதிய AstCCast{fl, quadfromp, VL_EDATASIZE};
        } இல்லையெனில் எனில் (nodep->isQuad() && word == 1) {
            Ast_முனை_கோவை* மாறிலி quadfromp = nodep->clone_மரம்_Pure(உண்மை);
            quadfromp->dtypeSetBitUnsized(VL_QUADSIZE, quadfromp->widthMin(), VSigning::UNSIGNED);
            திரும்பு புதிய AstCCast{
                fl, புதிய AstShiftR{fl, quadfromp, புதிய AstConst{fl, VL_EDATASIZE}, VL_EDATASIZE},
                VL_EDATASIZE};
        } இல்லையெனில் எனில் (!nodep->isWide() && !nodep->isQuad() && word == 0) {
            திரும்பு nodep->clone_மரம்_Pure(உண்மை);
        } இல்லையெனில் {  // Out of bounds
            திரும்பு புதிய AstConst{fl, 0};
        }
    }

    நிலையான Ast_முனை_கோவை* newWordGrabShift(File_வரி* fl, முழு word, Ast_முனை_கோவை* lhsp, முழு shift) {
        // Extract the expression to grab the value for the specified word, if it's the shift
        // of shift bits from lhsp
        // Negative word numbers requested for lhs when it's "before" what we want.
        // We get a 0 then.
        மாறிலி முழு othword = word - shift / VL_EDATASIZE;
        Ast_முனை_கோவை* மாறிலி lop = newAstWordSelClone(lhsp, othword);
        மாறிலி uint32_t loShift = VL_BITBIT_E(shift);
        எனில் (!loShift) திரும்பு lop;
        Ast_முனை_கோவை* மாறிலி hip = newAstWordSelClone(lhsp, othword - 1);
        மாறிலி uint32_t hiShift = VL_EDATASIZE - loShift;  // Complement offset
        Ast_முனை_கோவை* மாறிலி newp
            = புதிய AstOr{fl,  //
                        புதிய AstShiftR{fl, hip, புதிய AstConst{fl, hiShift}, VL_EDATASIZE},
                        புதிய AstShiftL{fl, lop, புதிய AstConst{fl, loShift}, VL_EDATASIZE}};
        திரும்பு V3Const::constifyEditCpp(newp);
    }

    // Return expression indexing the word that contains 'lsbp' + the given word offset
    நிலையான Ast_முனை_கோவை* newWordIndex(Ast_முனை_கோவை* lsbp, uint32_t wordOffset = 0) {
        // This is indexing a WordSel, so a 32 bit constants are fine
        File_வரி* மாறிலி flp = lsbp->fileline();
        எனில் (மாறிலி AstConst* constp = VN_CAST(lsbp, Const)) {
            திரும்பு புதிய AstConst{flp, wordOffset + VL_BITWORD_E(constp->toUInt())};
        }

        எனில் (lsbp->backp()) lsbp = lsbp->clone_மரம்_Pure(பொய்);
        Ast_முனை_கோவை* மாறிலி wwl2p = புதிய AstConst{flp, VL_EDATASIZE_LOG2};  // Word width log 2
        Ast_முனை_கோவை* indexp = புதிய AstShiftR{flp, lsbp, wwl2p, VL_EDATASIZE};
        எனில் (wordOffset) indexp = புதிய AstAdd{flp, புதிய AstConst{flp, wordOffset}, indexp};
        திரும்பு indexp;
    }

    // Return word of fromp that contains word indexp + the given word offset.
    நிலையான Ast_முனை_கோவை* newWordSelWord(File_வரி* flp, Ast_முனை_கோவை* fromp, Ast_முனை_கோவை* indexp,
                                       uint32_t wordOffset = 0) {
        UASSERT_OBJ(fromp->isWide(), fromp, "Only need AstWordSel on wide from's");

        எனில் (மாறிலி AstConst* மாறிலி constp = VN_CAST(indexp, Const)) {
            indexp = nullptr;
            wordOffset += constp->toUInt();
        }

        // e.g. "logic [95:0] var[0]; logic [0] sel; out = var[sel];"
        // Squash before C++ to avoid getting a C++ compiler warning
        // (even though code would be unreachable as presumably a
        // AstCond is protecting above this node.
        எனில் (wordOffset >= static_cast<uint32_t>(fromp->width_சொற்கள்())) {
            திரும்பு புதிய AstConst{flp, AstConst::SizedEData{}, 0};
        }

        எனில் (fromp->backp()) fromp = fromp->clone_மரம்_Pure(பொய்);

        // If indexp was constant, just use it
        எனில் (!indexp) திரும்பு புதிய AstWordSel{flp, fromp, புதிய AstConst{flp, wordOffset}};

        // Otherwise compute at runtime
        எனில் (indexp->backp()) indexp = indexp->clone_மரம்_Pure(பொய்);
        எனில் (wordOffset) indexp = புதிய AstAdd{flp, புதிய AstConst{flp, wordOffset}, indexp};
        திரும்பு புதிய AstWordSel{flp, fromp, indexp};
    }

    // Return word of fromp that contains bit lsbp + the given word offset.
    நிலையான Ast_முனை_கோவை* newWordSelBit(File_வரி* flp, Ast_முனை_கோவை* fromp, Ast_முனை_கோவை* lsbp,
                                      uint32_t wordOffset = 0) {
        Ast_முனை_கோவை* மாறிலி indexp = newWordIndex(lsbp, wordOffset);
        Ast_முனை_கோவை* மாறிலி wordSelp = newWordSelWord(flp, fromp, indexp);
        எனில் (!indexp->backp()) VL_DO_DANGLING(indexp->delete_மரம்(), indexp);
        திரும்பு wordSelp;
    }

    நிலையான Ast_முனை_கோவை* newSelBitBit(Ast_முனை_கோவை* lsbp) {
        // Return equation to get the VL_BITBIT of a constant or non-constant
        File_வரி* மாறிலி fl = lsbp->fileline();
        எனில் (VN_IS(lsbp, Const)) {
            திரும்பு புதிய AstConst{fl, VL_BITBIT_E(VN_AS(lsbp, Const)->toUInt())};
        } இல்லையெனில் {
            திரும்பு புதிய AstAnd{fl, புதிய AstConst{fl, VL_EDATASIZE - 1}, lsbp->clone_மரம்_Pure(உண்மை)};
        }
    }

    //====================

    தருக்க expandWide(Ast_முனை_Assign* nodep, AstConst* rhsp) {
        எனில் (!doExpandWide(nodep)) திரும்பு பொய்;
        didPattern(VExpandPattern::ASSIGN_CONST, nodep);
        // -> {for each_word{ ASSIGN(WORDSEL(wide,#),WORDSEL(CONST,#))}}
        எனில் (rhsp->இலக்கம்().isFourState()) {
            rhsp->v3warn(E_UNSUPPORTED,  // LCOV_EXCL_LINE  // impossible?
                         "Unsupported: 4-state numbers in this context");
        }
        File_வரி* மாறிலி fl = nodep->fileline();
        க்கு (முழு w = 0; w < nodep->width_சொற்கள்(); ++w) {
            addWordAssign(nodep, w,
                          புதிய AstConst{fl, AstConst::SizedEData{}, rhsp->இலக்கம்().edataWord(w)});
        }
        திரும்பு உண்மை;
    }
    //-------- Uniops
    தருக்க expandWide(Ast_முனை_Assign* nodep, AstVarRef* rhsp) {
        // Special case: do not expand assignment of constant pool variables.
        // V3Subst undestands these directly.
        எனில் (rhsp->varp()->constPoolEntry()) திரும்பு பொய்;
        எனில் (!doExpandWide(nodep)) திரும்பு பொய்;
        didPattern(VExpandPattern::ASSIGN_VARREF, nodep);
        க்கு (முழு w = 0; w < nodep->width_சொற்கள்(); ++w) {
            addWordAssign(nodep, w, newAstWordSelClone(rhsp, w));
        }
        திரும்பு உண்மை;
    }
    தருக்க expandWide(Ast_முனை_Assign* nodep, AstArraySel* rhsp) {
        UASSERT_OBJ(!VN_IS(nodep->dtypep()->skipRefp(), UnpackArrayDType), nodep,
                    "ArraySel with unpacked arrays should have been removed in V3Slice");
        எனில் (!doExpandWide(nodep)) திரும்பு பொய்;
        didPattern(VExpandPattern::ASSIGN_ARRAYSEL, nodep);
        க்கு (முழு w = 0; w < nodep->width_சொற்கள்(); ++w) {
            addWordAssign(nodep, w, newAstWordSelClone(rhsp, w));
        }
        திரும்பு உண்மை;
    }
    தருக்க expandWide(Ast_முனை_Assign* nodep, AstNot* rhsp) {
        // -> {for each_word{ ASSIGN(WORDSEL(wide,#),NOT(WORDSEL(lhs,#))) }}
        எனில் (!doExpandWide(nodep)) திரும்பு பொய்;
        didPattern(VExpandPattern::ASSIGN_NOT, nodep);
        File_வரி* மாறிலி fl = rhsp->fileline();
        க்கு (முழு w = 0; w < nodep->width_சொற்கள்(); ++w) {
            addWordAssign(nodep, w, புதிய AstNot{fl, newAstWordSelClone(rhsp->lhsp(), w)});
        }
        திரும்பு உண்மை;
    }
    //-------- Biops
    தருக்க expandWide(Ast_முனை_Assign* nodep, AstAnd* rhsp) {
        எனில் (!doExpandWide(nodep)) திரும்பு பொய்;
        didPattern(VExpandPattern::ASSIGN_AND, nodep);
        File_வரி* மாறிலி fl = nodep->fileline();
        க்கு (முழு w = 0; w < nodep->width_சொற்கள்(); ++w) {
            addWordAssign(nodep, w,
                          புதிய AstAnd{fl, newAstWordSelClone(rhsp->lhsp(), w),
                                     newAstWordSelClone(rhsp->rhsp(), w)});
        }
        திரும்பு உண்மை;
    }
    தருக்க expandWide(Ast_முனை_Assign* nodep, AstOr* rhsp) {
        எனில் (!doExpandWide(nodep)) திரும்பு பொய்;
        didPattern(VExpandPattern::ASSIGN_OR, nodep);
        File_வரி* மாறிலி fl = nodep->fileline();
        க்கு (முழு w = 0; w < nodep->width_சொற்கள்(); ++w) {
            addWordAssign(nodep, w,
                          புதிய AstOr{fl, newAstWordSelClone(rhsp->lhsp(), w),
                                    newAstWordSelClone(rhsp->rhsp(), w)});
        }
        திரும்பு உண்மை;
    }
    தருக்க expandWide(Ast_முனை_Assign* nodep, AstXor* rhsp) {
        எனில் (!doExpandWide(nodep)) திரும்பு பொய்;
        didPattern(VExpandPattern::ASSIGN_XOR, nodep);
        File_வரி* மாறிலி fl = nodep->fileline();
        க்கு (முழு w = 0; w < nodep->width_சொற்கள்(); ++w) {
            addWordAssign(nodep, w,
                          புதிய AstXor{fl, newAstWordSelClone(rhsp->lhsp(), w),
                                     newAstWordSelClone(rhsp->rhsp(), w)});
        }
        திரும்பு உண்மை;
    }
    தருக்க expandWideShift(Ast_முனை_Assign* nodep, Ast_முனை_Biop* rhsp, தருக்க is_இடது_Shift) {
        எனில் (!doExpandWide(nodep)) திரும்பு பொய்;

        // Simplify the shift amount, in case it becomes a constant
        V3Const::constifyEditCpp(rhsp->rhsp());

        // If it's a constant shift by whole words, expand it so V3Subst can substitute it
        எனில் (மாறிலி AstConst* மாறிலி rhsConstp = VN_CAST(rhsp->rhsp(), Const)) {
            மாறிலி uint32_t shiftBits = rhsConstp->toUInt();
            எனில் (VL_BITBIT_E(shiftBits) == 0) {
                மாறிலி முழு width_சொற்கள் = nodep->width_சொற்கள்();
                மாறிலி முழு shift_சொற்கள் = std::min<முழு>(VL_BITWORD_E(shiftBits), width_சொற்கள்);
                File_வரி* மாறிலி flp = rhsp->fileline();
                எனில் (is_இடது_Shift) {
                    didPattern(VExpandPattern::ASSIGN_SHIFTL, nodep);
                    // Low words of the result are zero
                    க்கு (முழு w = 0; w < shift_சொற்கள்; ++w) {
                        addWordAssign(nodep, w, புதிய AstConst{flp, AstConst::SizedEData{}, 0});
                    }
                    // High words of the result are copied from higher words of the source
                    க்கு (முழு w = shift_சொற்கள்; w < width_சொற்கள்; ++w) {
                        addWordAssign(nodep, w, newAstWordSelClone(rhsp->lhsp(), w - shift_சொற்கள்));
                    }
                } இல்லையெனில் {
                    didPattern(VExpandPattern::ASSIGN_SHIFTR, nodep);
                    // Low words of the result are copied from higher words of the source
                    க்கு (முழு w = 0; w < width_சொற்கள் - shift_சொற்கள்; ++w) {
                        addWordAssign(nodep, w, newAstWordSelClone(rhsp->lhsp(), w + shift_சொற்கள்));
                    }
                    // High words of the result are zero
                    க்கு (முழு w = width_சொற்கள் - shift_சொற்கள்; w < width_சொற்கள்; ++w) {
                        addWordAssign(nodep, w, புதிய AstConst{flp, AstConst::SizedEData{}, 0});
                    }
                }
                திரும்பு உண்மை;
            }
        }

        திரும்பு பொய்;
    }

    //-------- Triops
    தருக்க expandWide(Ast_முனை_Assign* nodep, AstCond* rhsp) {
        எனில் (!doExpandWide(nodep)) திரும்பு பொய்;
        didPattern(VExpandPattern::ASSIGN_COND, nodep);
        File_வரி* மாறிலி fl = nodep->fileline();
        க்கு (முழு w = 0; w < nodep->width_சொற்கள்(); ++w) {
            addWordAssign(nodep, w,
                          புதிய AstCond{fl, rhsp->condp()->clone_மரம்_Pure(உண்மை),
                                      newAstWordSelClone(rhsp->thenp(), w),
                                      newAstWordSelClone(rhsp->elsep(), w)});
        }
        திரும்பு உண்மை;
    }

    // VISITORS
    வெறுமை visit(AstNetlist* nodep) override { iterateAndNextNull(nodep->modulesp()); }

    வெறுமை visit(AstCFunc* nodep) override {
        VL_RESTORER(m_funcp);
        VL_RESTORER(m_nTmps);
        m_funcp = nodep;
        m_nTmps = 0;
        மாறிலி VDouble0 statWidesBefore = m_statWides;
        iterateChildren(nodep);

        // Constant fold here if anything was expanded, as Ast size can likely be reduced
        எனில் (v3Global.opt.fConstEager() && m_statWides != statWidesBefore) {
            Ast_முனை* மாறிலி editedp = V3Const::constifyEditCpp(nodep);
            UASSERT_OBJ(editedp == nodep, editedp, "Should not have replaced CFunc");
        }
    }

    வெறுமை visit(AstExtend* nodep) override {
        எனில் (nodep->user1SetOnce()) திரும்பு;  // Process once
        iterateChildren(nodep);
        எனில் (nodep->isWide()) {
            // See under ASSIGN(EXTEND)
        } இல்லையெனில் {
            எனில் (isImpure(nodep)) திரும்பு;
            Ast_முனை_கோவை* மாறிலி lhsp = nodep->lhsp()->unlinkFrBack();
            Ast_முனை_கோவை* newp = lhsp;
            எனில் (nodep->isQuad()) {
                எனில் (lhsp->isQuad()) {
                    lhsp->dtypeFrom(nodep);  // Just mark it, else nop
                } இல்லையெனில் எனில் (lhsp->isWide()) {
                    nodep->v3fatalSrc("extending larger thing into smaller?");
                } இல்லையெனில் {
                    newp = புதிய AstCCast{nodep->fileline(), lhsp, nodep};
                }
            } இல்லையெனில் {  // Long
                UASSERT_OBJ(!(lhsp->isQuad() || lhsp->isWide()), nodep,
                            "extending larger thing into smaller?");
                lhsp->dtypeFrom(nodep);  // Just mark it, else nop
            }
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        }
    }
    தருக்க expandWide(Ast_முனை_Assign* nodep, AstExtend* rhsp) {
        எனில் (!doExpandWide(nodep)) திரும்பு பொய்;
        didPattern(VExpandPattern::ASSIGN_EXTEND, nodep);
        Ast_முனை_கோவை* மாறிலி rlhsp = rhsp->lhsp();
        க்கு (முழு w = 0; w < rlhsp->width_சொற்கள்(); ++w) {
            addWordAssign(nodep, w, newAstWordSelClone(rlhsp, w));
        }
        க்கு (முழு w = rlhsp->width_சொற்கள்(); w < nodep->width_சொற்கள்(); ++w) {
            addWordAssign(nodep, w, புதிய AstConst{nodep->fileline(), AstConst::SizedEData{}, 0});
        }
        திரும்பு உண்மை;
    }

    வெறுமை visit(AstSel* nodep) override {
        எனில் (nodep->user1SetOnce()) திரும்பு;  // Process once
        iterateChildren(nodep);

        மாறிலி uint32_t width = static_cast<uint32_t>(nodep->widthConst());
        UASSERT_OBJ(nodep->widthMin() == static_cast<முழு>(width), nodep, "Width mismatch");

        // Skip if Sel is an LHS assignment select
        எனில் (Ast_முனை_Assign* மாறிலி assignp = VN_CAST(nodep->backp(), முனை_Assign)) {
            எனில் (nodep == assignp->lhsp()) திரும்பு;
        }

        // Skip if wide: See under ASSIGN(WIDE)
        எனில் (nodep->isWide()) திரும்பு;

        // Skip Sel from stream or queue
        எனில் (VN_IS(nodep->fromp()->dtypep(), StreamDType)) திரும்பு;
        எனில் (VN_IS(nodep->fromp()->dtypep(), QueueDType)) திரும்பு;

        // Result must be non-wide after skipping all the above
        UASSERT_OBJ(width <= 64, nodep, "Inconsistent result width");

        எனில் (nodep->fromp()->isWide()) {  // Long/Quad from Wide
            எனில் (isImpure(nodep)) திரும்பு;
            didPattern(VExpandPattern::SEL_FROM_WIDE, nodep);
            File_வரி* மாறிலி nfl = nodep->fileline();
            File_வரி* மாறிலி lfl = nodep->lsbp()->fileline();
            File_வரி* மாறிலி ffl = nodep->fromp()->fileline();
            Ast_முனை_கோவை* மாறிலி fromp = nodep->fromp();

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
            மாறிலி auto tryFold = [&](Ast_முனை_கோவை* exprp) -> Ast_முனை_கோவை* {
                // Size limit for inlining expressions (any arbitrary value results in
                // a bounded expansion. Picked a value to allow common simple forms.)
                நிலையான constexpr முழு EXPAND_SEL_LSB_LIMIT = 8;
                // Simplify
                exprp = V3Const::constifyEditCpp(exprp);
                // Accept if not larger than the limit
                எனில் (!exprp->isLargerThan(EXPAND_SEL_LSB_LIMIT)) திரும்பு exprp;
                // Delete if rejected
                VL_DO_DANGLING(exprp->delete_மரம்(), exprp);
                திரும்பு nullptr;
            };

            // Return the index of the word holding the bit 'offset' bits above the LSB
            மாறிலி auto wordIdx = [&](uint32_t offset) -> Ast_முனை_கோவை* {
                Ast_முனை_கோவை* மாறிலி msbp = புதிய AstAdd{lfl, புதிய AstConst{lfl, offset},
                                                     nodep->lsbp()->clone_மரம்_Pure(பொய்)};
                Ast_முனை_கோவை* மாறிலி idxp = newWordIndex(msbp);
                எனில் (!msbp->backp()) VL_DO_DANGLING(msbp->delete_மரம்(), msbp);
                திரும்பு idxp;
            };

            // Bit index of the selected LSB within its word
            Ast_முனை_கோவை* lBitp = tryFold(newSelBitBit(nodep->lsbp()));
            மாறிலி தருக்க aligned = lBitp && lBitp->isZero();

            // Which of the other 2 words the expansion might need
            மாறிலி தருக்க mNeeded = width > 1 && !aligned;
            மாறிலி தருக்க hNeeded = width > VL_EDATASIZE;
            UASSERT_OBJ(!hNeeded || nodep->isQuad(), nodep, "Width mismatch");

            // Word indices
            மாறிலி uint32_t mMsbOffset = std::min<uint32_t>(width, VL_EDATASIZE) - 1;
            மாறிலி uint32_t hMsbOffset = width - 1;
            Ast_முனை_கோவை* lIdxp = tryFold(newWordIndex(nodep->lsbp()));
            Ast_முனை_கோவை* mIdxp = mNeeded ? tryFold(wordIdx(mMsbOffset)) : nullptr;
            Ast_முனை_கோவை* hIdxp = hNeeded ? tryFold(wordIdx(hMsbOffset)) : nullptr;

            // If the LSB expression is needed multiple times, evaluate it into a temporary
            மாறிலி முழு nCopies = !lBitp + !lIdxp + (mNeeded && !mIdxp) + (hNeeded && !hIdxp);
            எனில் (nCopies > 1 && m_funcp && m_stmtp && !isImpure(m_stmtp)) {
                ++m_nTmps;  // Use fresh set of temporaries
                Ast_முனை_கோவை* மாறிலி lsbp = nodep->lsbp()->unlinkFrBack();
                AstVar* மாறிலி tmpp = addLocalTmp(m_stmtp, "ExpandSel_Lsb", lsbp);
                nodep->lsbp(புதிய AstVarRef{lfl, tmpp, VAccess::READ});
            }

            // Rebuild remaining expressions derived from LSB if needed, now using the temporary
            எனில் (!lBitp) lBitp = newSelBitBit(nodep->lsbp());
            எனில் (!lIdxp) lIdxp = newWordIndex(nodep->lsbp());
            எனில் (!mIdxp) mIdxp = mNeeded ? wordIdx(mMsbOffset) : nullptr;
            எனில் (!hIdxp) hIdxp = hNeeded ? wordIdx(hMsbOffset) : nullptr;

            // Return word 'idxp' of 'fromp', without consuming 'idxp'
            மாறிலி auto wordSel = [&](Ast_முனை_கோவை* idxp) -> Ast_முனை_கோவை* {
                Ast_முனை_கோவை* மாறிலி clonep = idxp->clone_மரம்_Pure(பொய்);
                Ast_முனை_கோவை* மாறிலி wordp = newWordSelWord(ffl, fromp, clonep);
                எனில் (!clonep->backp()) VL_DO_DANGLING(clonep->delete_மரம்(), clonep);
                திரும்பு wordp;
            };

            // Construct term containing the bits of the low word - always needed
            Ast_முனை_கோவை* மாறிலி lTermp = [&]() -> Ast_முனை_கோவை* {
                Ast_முனை_கோவை* lWordp = wordSel(lIdxp);
                எனில் (nodep->isQuad()) lWordp = புதிய AstCCast{nfl, lWordp, nodep};
                திரும்பு புதிய AstShiftR{nfl, lWordp, lBitp->clone_மரம்_Pure(பொய்), nodep->width()};
            }();

            // Construct term containing the bits of the middle word - if needed
            Ast_முனை_கோவை* மாறிலி mTermp = [&]() -> Ast_முனை_கோவை* {
                எனில் (!mNeeded) திரும்பு nullptr;

                Ast_முனை_கோவை* mWordp = wordSel(mIdxp);
                எனில் (nodep->isQuad()) mWordp = புதிய AstCCast{nfl, mWordp, nodep};
                Ast_முனை_கோவை* மாறிலி mShiftp = புதிய AstSub{lfl, புதிய AstConst{lfl, VL_EDATASIZE},
                                                        lBitp->clone_மரம்_Pure(பொய்)};
                // If we're selecting bit zero, then all 32 bits in the mid word
                // get shifted << by 32 bits, so ignore them.
                மாறிலி V3_எண் zero{nodep, longOrQuadWidth(nodep)};
                திரும்பு புதிய AstCond{
                    nfl,
                    // lsb % VL_EDATASIZE == 0 ?
                    புதிய AstEq{nfl, புதிய AstConst{nfl, 0}, lBitp->clone_மரம்_Pure(பொய்)},
                    // 0 :
                    புதிய AstConst{nfl, zero},
                    //  midword >> (VL_EDATASIZE - (lbs % VL_EDATASIZE))
                    புதிய AstShiftL{nfl, mWordp, mShiftp, nodep->width()}};
            }();

            // Construct term containing the bits of the high word - if needed
            Ast_முனை_கோவை* மாறிலி hTermp = [&]() -> Ast_முனை_கோவை* {
                எனில் (!hNeeded) திரும்பு nullptr;

                Ast_முனை_கோவை* hWordp = wordSel(hIdxp);
                hWordp = புதிய AstCCast{nfl, hWordp, nodep};
                Ast_முனை_கோவை* மாறிலி hShiftp = புதிய AstCond{
                    nfl,
                    // lsb % VL_EDATASIZE == 0 ?
                    புதிய AstEq{nfl, புதிய AstConst{nfl, 0}, lBitp->clone_மரம்_Pure(பொய்)},
                    // VL_EDATASIZE :
                    புதிய AstConst{lfl, VL_EDATASIZE},
                    // 64 - (lbs % VL_EDATASIZE)
                    புதிய AstSub{lfl, புதிய AstConst{lfl, 64}, lBitp->clone_மரம்_Pure(பொய்)}};
                திரும்பு புதிய AstShiftL{nfl, hWordp, hShiftp, nodep->width()};
            }();

            // Delete parts not captured during construction
            VL_DO_DANGLING(lBitp->delete_மரம்(), lBitp);
            VL_DO_DANGLING(lIdxp->delete_மரம்(), lIdxp);
            எனில் (mIdxp) VL_DO_DANGLING(mIdxp->delete_மரம்(), mIdxp);
            எனில் (hIdxp) VL_DO_DANGLING(hIdxp->delete_மரம்(), hIdxp);

            // Or reduce the terms
            Ast_முனை_கோவை* newp = lTermp;
            எனில் (mTermp) newp = புதிய AstOr{nfl, mTermp, newp};
            எனில் (hTermp) newp = புதிய AstOr{nfl, hTermp, newp};
            newp->dtypeFrom(nodep);
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        } இல்லையெனில் {  // Long/Quad from Long/Quad
            // No isImpure() check - can handle side effects in below
            didPattern(VExpandPattern::SEL_FROM_NARROW, nodep);
            File_வரி* மாறிலி fl = nodep->fileline();
            Ast_முனை_கோவை* fromp = nodep->fromp()->unlinkFrBack();
            Ast_முனை_கோவை* மாறிலி lsbp = nodep->lsbp()->unlinkFrBack();
            எனில் (nodep->isQuad() && !fromp->isQuad()) fromp = புதிய AstCCast{fl, fromp, nodep};
            // {large}>>32 requires 64-bit shift operation; then cast
            Ast_முனை_கோவை* newp = புதிய AstShiftR{fl, fromp, lsbp, fromp->width()};
            newp->dtypeFrom(fromp);
            எனில் (!nodep->isQuad() && fromp->isQuad()) newp = புதிய AstCCast{fl, newp, nodep};
            newp->dtypeFrom(nodep);
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        }
    }

    தருக்க expandWide(Ast_முனை_Assign* nodep, AstSel* rhsp) {
        UASSERT_OBJ(nodep->widthMin() == rhsp->widthConst(), nodep, "Width mismatch");
        எனில் (!doExpandWide(nodep)) திரும்பு பொய்;

        // Simplify the index, in case it becomes a constant
        V3Const::constifyEditCpp(rhsp->lsbp());

        // If it's a constant select and aligned, we can just copy the words
        எனில் (மாறிலி AstConst* மாறிலி lsbConstp = VN_CAST(rhsp->lsbp(), Const)) {
            மாறிலி uint32_t lsb = lsbConstp->toUInt();
            எனில் (VL_BITBIT_E(lsb) == 0) {
                didPattern(VExpandPattern::ASSIGN_SEL_ALIGNED, nodep);
                மாறிலி uint32_t word = VL_BITWORD_E(lsb);
                க்கு (முழு w = 0; w < nodep->width_சொற்கள்(); ++w) {
                    addWordAssign(nodep, w, newAstWordSelClone(rhsp->fromp(), w + word));
                }
                திரும்பு உண்மை;
            }
        }

        didPattern(VExpandPattern::ASSIGN_SEL_UNALIGNED, nodep);
        File_வரி* மாறிலி flp = rhsp->fileline();

        // Use fresh set of temporaries
        ++m_nTmps;

        // Compute word index of LSB, store to temporary if not constant
        Ast_முனை_கோவை* மாறிலி wordLsbp = rhsp->lsbp()->clone_மரம்_Pure(பொய்);
        Ast_முனை_கோவை* wordIdxp = newWordIndex(wordLsbp);
        எனில் (!wordLsbp->backp()) VL_DO_DANGLING(wordLsbp->delete_மரம்(), wordLsbp);
        wordIdxp = V3Const::constifyEditCpp(wordIdxp);
        எனில் (!VN_IS(wordIdxp, Const)) {
            AstVar* மாறிலி tmpp = addLocalTmp(nodep, "ExpandSel_WordIdx", wordIdxp);
            wordIdxp = புதிய AstVarRef{flp, tmpp, VAccess::READ};
        }

        // Compute shift amounts and mask, store to temporaries if not constants
        Ast_முனை_கோவை* loShftp = nullptr;
        Ast_முனை_கோவை* hiShftp = nullptr;
        Ast_முனை_கோவை* hiMaskp = nullptr;
        எனில் (மாறிலி AstConst* மாறிலி lsbConstp = VN_CAST(rhsp->lsbp(), Const)) {
            மாறிலி uint32_t bitOffset = VL_BITBIT_E(lsbConstp->toUInt());
            // Must be unaligned, otherwise we would have handled it above
            UASSERT_OBJ(bitOffset, nodep, "Missed aligned wide select");
            loShftp = புதிய AstConst{flp, bitOffset};
            hiShftp = புதிய AstConst{flp, VL_EDATASIZE - bitOffset};
            hiMaskp = nullptr;
        } இல்லையெனில் {
            // Compute Low word shift amount: bottom bits of lsb index
            Ast_முனை_கோவை* மாறிலி lsbp = rhsp->lsbp()->clone_மரம்_Pure(பொய்);
            loShftp = புதிய AstAnd{flp, புதிய AstConst{flp, VL_SIZEBITS_E}, lsbp};
            AstVar* மாறிலி loTmpp = addLocalTmp(nodep, "ExpandSel_LoShift", loShftp);
            loShftp = புதிய AstVarRef{flp, loTmpp, VAccess::READ};

            // Compute if aligned: Low word shift amount is 0
            Ast_முனை_கோவை* மாறிலி zerop = புதிய AstConst{flp, 0};
            Ast_முனை_கோவை* alignedp = புதிய AstEq{flp, zerop, loShftp->clone_மரம்_Pure(பொய்)};
            AstVar* மாறிலி alignedTmpp = addLocalTmp(nodep, "ExpandSel_Aligned", alignedp);
            alignedp = புதிய AstVarRef{flp, alignedTmpp, VAccess::READ};

            // Computed High word shift amount: 0 if aligned else VL_EDATASIZE - Low word shift
            Ast_முனை_கோவை* மாறிலி edsp = புதிய AstConst{flp, VL_EDATASIZE};
            Ast_முனை_கோவை* மாறிலி subp = புதிய AstSub{flp, edsp, loShftp->clone_மரம்_Pure(பொய்)};
            hiShftp = புதிய AstCond{flp, alignedp, புதிய AstConst{flp, 0}, subp};
            AstVar* மாறிலி hiTmpp = addLocalTmp(nodep, "ExpandSel_HiShift", hiShftp);
            hiShftp = புதிய AstVarRef{flp, hiTmpp, VAccess::READ};

            // Compute the High word mask: 0 if aligned, ones otherwise
            hiMaskp = புதிய AstCond{flp, alignedp->clone_மரம்_Pure(பொய்),
                                  புதிய AstConst{flp, AstConst::SizedEData{}, 0},
                                  புதிய AstConst{flp, AstConst::SizedEData{}, -1ULL}};
            AstVar* மாறிலி maskTmpp = addLocalTmp(nodep, "ExpandSel_HiMask", hiMaskp);
            hiMaskp = புதிய AstVarRef{flp, maskTmpp, VAccess::READ};
        }

        // Create each word of the selected result
        Ast_முனை_கோவை* மாறிலி fromp = rhsp->fromp();
        மாறிலி முழு sel_சொற்கள் = nodep->width_சொற்கள்();
        க்கு (முழு w = 0; w < sel_சொற்கள்; ++w) {
            // Grab bits from word 'VL_BITWORD_E(lsb) + w'
            Ast_முனை_கோவை* மாறிலி loWordp = newWordSelWord(fromp->fileline(), fromp, wordIdxp, w);
            Ast_முனை_கோவை* மாறிலி loShftClonep = loShftp->clone_மரம்_Pure(பொய்);
            Ast_முனை_கோவை* மாறிலி lop = புதிய AstShiftR{flp, loWordp, loShftClonep, VL_EDATASIZE};
            // Grab bits from word 'VL_BITWORD_E(lsb) + w + 1'
            Ast_முனை_கோவை* hiWordp = newWordSelWord(fromp->fileline(), fromp, wordIdxp, w + 1);
            // For the last word of the result, avoid an OOB access when the Sel is not OOB
            எனில் (w == sel_சொற்கள் - 1) {
                மாறிலி uint32_t max = fromp->width_சொற்கள்() - w - 1;
                Ast_முனை_கோவை* மாறிலி maxp = புதிய AstConst{flp, max};
                Ast_முனை_கோவை* மாறிலி condp = புதிய AstGte{flp, wordIdxp->clone_மரம்_Pure(பொய்), maxp};
                Ast_முனை_கோவை* மாறிலி zerop = புதிய AstConst{flp, AstConst::SizedEData{}, 0};
                hiWordp = புதிய AstCond{flp, condp, zerop, hiWordp};
            }
            Ast_முனை_கோவை* மாறிலி hiShftClonep = hiShftp->clone_மரம்_Pure(பொய்);
            Ast_முனை_கோவை* மாறிலி hiPartp = புதிய AstShiftL{flp, hiWordp, hiShftClonep, VL_EDATASIZE};
            Ast_முனை_கோவை* மாறிலி hip
                = hiMaskp ? புதிய AstAnd{flp, hiPartp, hiMaskp->clone_மரம்_Pure(பொய்)} : hiPartp;
            // Combine them
            addWordAssign(nodep, w, புதிய AstOr{nodep->fileline(), hip, lop});
        }

        // Delete parts not captured during construction
        எனில் (!wordIdxp->backp()) VL_DO_DANGLING(wordIdxp->delete_மரம்(), wordIdxp);
        எனில் (!loShftp->backp()) VL_DO_DANGLING(loShftp->delete_மரம்(), loShftp);
        எனில் (!hiShftp->backp()) VL_DO_DANGLING(hiShftp->delete_மரம்(), hiShftp);
        எனில் (hiMaskp && !hiMaskp->backp()) VL_DO_DANGLING(hiMaskp->delete_மரம்(), hiMaskp);

        திரும்பு உண்மை;
    }

    தருக்க expandLhs(Ast_முனை_Assign* nodep, AstSel* lhsp) {
        // Possibilities
        //      destp: wide or narrow
        //      rhsp:  wide (destp must be wide), narrow, or 1 bit wide
        //      rhsp:  may be allones and can remove AND NOT gate
        //      lsbp:  constant or variable
        // Yuk.
        எனில் (isImpure(nodep)) திரும்பு பொய்;
        File_வரி* மாறிலி nfl = nodep->fileline();
        File_வரி* மாறிலி lfl = lhsp->fileline();
        மாறிலி தருக்க destwide = lhsp->fromp()->isWide();
        மாறிலி தருக்க ones = nodep->rhsp()->isAllOnesV();
        எனில் (VN_IS(lhsp->lsbp(), Const)) {
            // The code should work without this constant test, but it won't
            // constify as nicely as we'd like.
            Ast_முனை_கோவை* rhsp = nodep->rhsp()->unlinkFrBack();
            Ast_முனை_கோவை* மாறிலி destp = lhsp->fromp()->unlinkFrBack();
            மாறிலி முழு lsb = lhsp->lsbConst();
            மாறிலி முழு msb = lhsp->msbConst();
            V3_எண் maskset{nodep, destp->widthMin()};
            maskset.setMask(msb + 1 - lsb, lsb);
            V3_எண் maskold{nodep, destp->widthMin()};
            maskold.opNot(maskset);
            எனில் (destwide) {
                didPattern(VExpandPattern::ASSIGN_TO_SEL_CONST_WIDE, nodep);
                க்கு (முழு w = 0; w < destp->width_சொற்கள்(); ++w) {
                    எனில் (w >= VL_BITWORD_E(lsb) && w <= VL_BITWORD_E(msb)) {
                        // else we would just be setting it to the same exact value
                        Ast_முனை_கோவை* oldvalp = newAstWordSelClone(destp, w);
                        fixCloneLvalue(oldvalp);
                        எனில் (!ones) {
                            oldvalp = புதிய AstAnd{
                                lfl,
                                புதிய AstConst{lfl, AstConst::SizedEData{}, maskold.edataWord(w)},
                                oldvalp};
                        }

                        // Appropriate word of new value to insert:
                        Ast_முனை_கோவை* newp = newWordGrabShift(lfl, w, rhsp, lsb);

                        // Apply cleaning at the top word of the destination
                        // (no cleaning to do if dst's width is a whole number
                        // of words).
                        எனில் (w == destp->width_சொற்கள்() - 1 && VL_BITBIT_E(destp->widthMin()) != 0) {
                            V3_எண் cleanmask{nodep, VL_EDATASIZE};
                            cleanmask.setMask(VL_BITBIT_E(destp->widthMin()));
                            newp = புதிய AstAnd{lfl, newp, புதிய AstConst{lfl, cleanmask}};
                        }
                        Ast_முனை_கோவை* மாறிலி orp
                            = V3Const::constifyEditCpp(புதிய AstOr{lfl, oldvalp, newp});
                        addWordAssign(nodep, w, destp, orp);
                    }
                }
                VL_DO_DANGLING(rhsp->delete_மரம்(), rhsp);
                VL_DO_DANGLING(destp->delete_மரம்(), destp);
            } இல்லையெனில் {
                didPattern(VExpandPattern::ASSIGN_TO_SEL_CONST_NARROW, nodep);
                எனில் (destp->isQuad() && !rhsp->isQuad()) rhsp = புதிய AstCCast{nfl, rhsp, nodep};
                Ast_முனை_கோவை* oldvalp = destp->clone_மரம்_Pure(உண்மை);
                fixCloneLvalue(oldvalp);
                எனில் (!ones) oldvalp = புதிய AstAnd{lfl, புதிய AstConst{lfl, maskold}, oldvalp};

                // The bit-select can refer to bits outside the width of nodep
                // which we aren't allowed to assign to.  This is a mask of the
                // valid range of nodep which we apply to the new shifted RHS.
                V3_எண் cleanmask{nodep, destp->widthMin()};
                cleanmask.setMask(destp->widthMin());
                Ast_முனை_கோவை* மாறிலி shifted = புதிய AstShiftL{
                    lfl, rhsp, புதிய AstConst{lfl, static_cast<uint32_t>(lsb)}, destp->width()};
                Ast_முனை_கோவை* மாறிலி cleaned
                    = புதிய AstAnd{lfl, shifted, புதிய AstConst{lfl, cleanmask}};
                Ast_முனை_கோவை* மாறிலி orp
                    = V3Const::constifyEditCpp(புதிய AstOr{lfl, oldvalp, cleaned});
                செருகு_Before(nodep, புதிய AstAssign{nfl, destp, orp});
            }
            திரும்பு உண்மை;
        } இல்லையெனில் {  // non-const select offset
            எனில் (destwide && lhsp->widthConst() == 1) {
                didPattern(VExpandPattern::ASSIGN_TO_SEL_VAR_WIDE_ONE_BIT, nodep);
                Ast_முனை_கோவை* மாறிலி rhsp = nodep->rhsp()->unlinkFrBack();
                Ast_முனை_கோவை* மாறிலி destp = lhsp->fromp()->unlinkFrBack();
                Ast_முனை_கோவை* oldvalp
                    = newWordSelBit(lfl, destp->clone_மரம்_Pure(பொய்), lhsp->lsbp());
                fixCloneLvalue(oldvalp);
                எனில் (!ones) {
                    oldvalp = புதிய AstAnd{
                        lfl,
                        புதிய AstNot{
                            lfl, புதிய AstShiftL{lfl, புதிய AstConst{nfl, 1},
                                               // newSelBitBit may exceed the MSB of this variable.
                                               // That's ok as we'd just AND with a larger value,
                                               // but oldval would clip the upper bits to sanity
                                               newSelBitBit(lhsp->lsbp()), VL_EDATASIZE}},
                        oldvalp};
                }
                // Restrict the shift amount to 0-31, see bug804.
                Ast_முனை_கோவை* மாறிலி shiftp = புதிய AstAnd{nfl, lhsp->lsbp()->clone_மரம்_Pure(உண்மை),
                                                       புதிய AstConst{nfl, VL_EDATASIZE - 1}};
                Ast_முனை* மாறிலி newp = புதிய AstAssign{
                    nfl, newWordSelBit(nfl, destp, lhsp->lsbp()),
                    புதிய AstOr{lfl, oldvalp, புதிய AstShiftL{lfl, rhsp, shiftp, VL_EDATASIZE}}};
                செருகு_Before(nodep, newp);
                திரும்பு உண்மை;
            } இல்லையெனில் எனில் (destwide) {
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
                திரும்பு பொய்;
            } இல்லையெனில் {
                didPattern(VExpandPattern::ASSIGN_TO_SEL_VAR_NARROW, nodep);
                // nodep->dumpTree("-  old: ");
                Ast_முனை_கோவை* rhsp = nodep->rhsp()->unlinkFrBack();
                Ast_முனை_கோவை* மாறிலி destp = lhsp->fromp()->unlinkFrBack();
                Ast_முனை_கோவை* oldvalp = destp->clone_மரம்_Pure(உண்மை);
                fixCloneLvalue(oldvalp);

                V3_எண் maskwidth{nodep, destp->widthMin()};
                maskwidth.setMask(lhsp->widthConst());

                எனில் (destp->isQuad() && !rhsp->isQuad()) rhsp = புதிய AstCCast{nfl, rhsp, nodep};
                எனில் (!ones) {
                    oldvalp = புதிய AstAnd{
                        lfl,
                        புதிய AstNot{lfl, புதிய AstShiftL{lfl, புதிய AstConst{nfl, maskwidth},
                                                      lhsp->lsbp()->clone_மரம்_Pure(உண்மை),
                                                      destp->width()}},
                        oldvalp};
                }
                Ast_முனை_கோவை* newp
                    = புதிய AstShiftL{lfl, rhsp, lhsp->lsbp()->clone_மரம்_Pure(உண்மை), destp->width()};
                // Apply cleaning to the new value being inserted.  Mask is
                // slightly wider than necessary to avoid an AND with all ones
                // being optimized out.  No need to clean if destp is
                // quad-sized as there are no extra bits to contaminate
                எனில் (destp->widthMin() != 64) {
                    V3_எண் cleanmask{nodep, destp->widthMin() + 1};
                    cleanmask.setMask(destp->widthMin());
                    newp = புதிய AstAnd{lfl, newp, புதிய AstConst{lfl, cleanmask}};
                }

                செருகு_Before(nodep, புதிய AstAssign{nfl, destp, புதிய AstOr{lfl, oldvalp, newp}});
                திரும்பு உண்மை;
            }
        }
    }

    வெறுமை visit(AstConcat* nodep) override {
        எனில் (nodep->user1SetOnce()) திரும்பு;  // Process once
        iterateChildren(nodep);
        எனில் (nodep->isWide()) {
            // See under ASSIGN(WIDE)
        } இல்லையெனில் {
            // No isImpure() check - can handle side effects in below
            didPattern(VExpandPattern::CONCAT, nodep);
            File_வரி* மாறிலி fl = nodep->fileline();
            Ast_முனை_கோவை* lhsp = nodep->lhsp()->unlinkFrBack();
            Ast_முனை_கோவை* rhsp = nodep->rhsp()->unlinkFrBack();
            மாறிலி uint32_t rhsshift = rhsp->widthMin();
            எனில் (nodep->isQuad() && !lhsp->isQuad()) lhsp = புதிய AstCCast{fl, lhsp, nodep};
            எனில் (nodep->isQuad() && !rhsp->isQuad()) rhsp = புதிய AstCCast{fl, rhsp, nodep};
            Ast_முனை_கோவை* மாறிலி newp = புதிய AstOr{
                fl, புதிய AstShiftL{fl, lhsp, புதிய AstConst{fl, rhsshift}, nodep->width()}, rhsp};
            newp->dtypeFrom(nodep);  // Unsigned
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        }
    }
    தருக்க expandWide(Ast_முனை_Assign* nodep, AstConcat* rhsp) {
        எனில் (!doExpandWide(rhsp)) திரும்பு பொய்;
        didPattern(VExpandPattern::ASSIGN_CONCAT, nodep);
        File_வரி* மாறிலி fl = rhsp->fileline();
        // Lhs or Rhs may be word, long, or quad.
        // newAstWordSelClone nicely abstracts the difference.
        மாறிலி முழு rhsshift = rhsp->rhsp()->widthMin();
        // Sometimes doing the words backwards is preferable.
        // When we have x={x,foo} backwards is better, when x={foo,x} forward is better
        // However V3Subst tends to rip this up, so not worth optimizing now.
        க்கு (முழு w = 0; w < rhsp->width_சொற்கள்(); ++w) {
            addWordAssign(nodep, w,
                          புதிய AstOr{fl, newWordGrabShift(fl, w, rhsp->lhsp(), rhsshift),
                                    newAstWordSelClone(rhsp->rhsp(), w)});
        }
        திரும்பு உண்மை;
    }

    வெறுமை visit(AstReplicate* nodep) override {
        எனில் (nodep->user1SetOnce()) திரும்பு;  // Process once
        iterateChildren(nodep);
        எனில் (nodep->isWide()) {
            // See under ASSIGN(WIDE)
        } இல்லையெனில் {
            எனில் (isImpure(nodep)) திரும்பு;
            File_வரி* மாறிலி fl = nodep->fileline();
            Ast_முனை_கோவை* lhsp = nodep->srcp()->unlinkFrBack();
            Ast_முனை_கோவை* newp;
            மாறிலி முழு lhswidth = lhsp->widthMin();
            எனில் (lhswidth == 1) {
                didPattern(VExpandPattern::REPLICATE_ONE_BIT, nodep);
                newp = புதிய AstNegate{fl, lhsp};
            } இல்லையெனில் {
                didPattern(VExpandPattern::REPLICATE, nodep);
                மாறிலி AstConst* மாறிலி constp = VN_AS(nodep->countp(), Const);
                UASSERT_OBJ(constp, nodep,
                            "Replication value isn't a constant.  Checked earlier!");
                மாறிலி uint32_t times = constp->toUInt();
                எனில் (nodep->isQuad() && !lhsp->isQuad()) lhsp = புதிய AstCCast{fl, lhsp, nodep};
                newp = lhsp->clone_மரம்_Pure(உண்மை);
                க்கு (unsigned repnum = 1; repnum < times; repnum++) {
                    மாறிலி முழு rhsshift = repnum * lhswidth;
                    newp = புதிய AstOr{
                        fl,
                        புதிய AstShiftL{fl, lhsp->clone_மரம்_Pure(உண்மை),
                                      புதிய AstConst{fl, static_cast<uint32_t>(rhsshift)},
                                      nodep->width()},
                        newp};
                    newp->dtypeFrom(nodep);  // Unsigned
                }
                VL_DO_DANGLING(lhsp->delete_மரம்(), lhsp);  // Never used
            }
            newp->dtypeFrom(nodep);  // Unsigned
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        }
    }
    தருக்க expandWide(Ast_முனை_Assign* nodep, AstReplicate* rhsp) {
        எனில் (!doExpandWide(rhsp)) திரும்பு பொய்;
        didPattern(VExpandPattern::ASSIGN_REPLICATE, nodep);
        File_வரி* மாறிலி fl = nodep->fileline();
        Ast_முனை_கோவை* மாறிலி lhsp = rhsp->srcp();
        மாறிலி முழு lhswidth = lhsp->widthMin();
        மாறிலி AstConst* மாறிலி constp = VN_AS(rhsp->countp(), Const);
        UASSERT_OBJ(constp, rhsp, "Replication value isn't a constant.  Checked earlier!");
        மாறிலி uint32_t times = constp->toUInt();
        க்கு (முழு w = 0; w < rhsp->width_சொற்கள்(); ++w) {
            Ast_முனை_கோவை* newp;
            எனில் (lhswidth == 1) {
                newp = புதிய AstNegate{fl, lhsp->clone_மரம்_Pure(உண்மை)};
                // Replicate always unsigned
                newp->dtypeChgWidthSigned(VL_EDATASIZE, VL_EDATASIZE, VSigning::UNSIGNED);
            } இல்லையெனில் {
                newp = newAstWordSelClone(lhsp, w);
                File_வரி* மாறிலி rfl = rhsp->fileline();
                க்கு (unsigned repnum = 1; repnum < times; repnum++) {
                    newp = புதிய AstOr{fl, newWordGrabShift(rfl, w, lhsp, lhswidth * repnum), newp};
                }
            }
            addWordAssign(nodep, w, newp);
        }
        திரும்பு உண்மை;
    }

    வெறுமை visitEqNeq(Ast_முனை_Biop* nodep) {
        எனில் (nodep->user1SetOnce()) திரும்பு;  // Process once
        iterateChildren(nodep);
        எனில் (nodep->lhsp()->isWide()
            && !(VN_IS(nodep->lhsp()->dtypep()->skipRefp(), StreamDType)
                 || VN_IS(nodep->rhsp()->dtypep()->skipRefp(), StreamDType))) {
            எனில் (isImpure(nodep)) திரும்பு;
            எனில் (!doExpandWide(nodep->lhsp())) திரும்பு;
            எனில் (!doExpandWide(nodep->rhsp())) திரும்பு;
            didPattern(VN_IS(nodep, Neq) ? VExpandPattern::WORDIZE_NEQ
                                         : VExpandPattern::WORDIZE_EQ,
                       nodep);
            // -> (0=={or{for each_word{WORDSEL(lhs,#)^WORDSEL(rhs,#)}}}
            File_வரி* மாறிலி fl = nodep->fileline();
            Ast_முனை_கோவை* newp = nullptr;
            க்கு (முழு w = 0; w < nodep->lhsp()->width_சொற்கள்(); ++w) {
                Ast_முனை_கோவை* மாறிலி eqp = புதிய AstXor{fl, newAstWordSelClone(nodep->lhsp(), w),
                                                    newAstWordSelClone(nodep->rhsp(), w)};
                newp = newp ? புதிய AstOr{fl, newp, eqp} : eqp;
            }
            எனில் (VN_IS(nodep, Neq)) {
                newp = புதிய AstNeq{fl, புதிய AstConst{fl, AstConst::SizedEData{}, 0}, newp};
            } இல்லையெனில் {
                newp = புதிய AstEq{fl, புதிய AstConst{fl, AstConst::SizedEData{}, 0}, newp};
            }
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        }
    }
    வெறுமை visit(AstEq* nodep) override { visitEqNeq(nodep); }
    வெறுமை visit(AstNeq* nodep) override { visitEqNeq(nodep); }

    வெறுமை visit(AstRedOr* nodep) override {
        எனில் (nodep->user1SetOnce()) திரும்பு;  // Process once
        iterateChildren(nodep);
        File_வரி* மாறிலி fl = nodep->fileline();
        எனில் (nodep->lhsp()->isWide()) {
            எனில் (isImpure(nodep)) திரும்பு;
            எனில் (!doExpandWide(nodep->lhsp())) திரும்பு;
            didPattern(VExpandPattern::WORDIZE_REDOR, nodep);
            // -> (0!={or{for each_word{WORDSEL(lhs,#)}}}
            Ast_முனை_கோவை* newp = nullptr;
            க்கு (முழு w = 0; w < nodep->lhsp()->width_சொற்கள்(); ++w) {
                Ast_முனை_கோவை* மாறிலி eqp = newAstWordSelClone(nodep->lhsp(), w);
                newp = newp ? புதிய AstOr{fl, newp, eqp} : eqp;
            }
            newp = புதிய AstNeq{fl, புதிய AstConst{fl, AstConst::SizedEData{}, 0}, newp};
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        } இல்லையெனில் {
            // No isImpure() check - can handle side effects in below
            didPattern(VExpandPattern::REDOR_TO_NEQ, nodep);
            Ast_முனை_கோவை* மாறிலி lhsp = nodep->lhsp()->unlinkFrBack();
            Ast_முனை_கோவை* மாறிலி newp = புதிய AstNeq{
                fl, புதிய AstConst{fl, AstConst::Widthed_மதிப்பு{}, longOrQuadWidth(nodep), 0}, lhsp};
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        }
    }
    வெறுமை visit(AstRedAnd* nodep) override {
        எனில் (nodep->user1SetOnce()) திரும்பு;  // Process once
        iterateChildren(nodep);
        File_வரி* மாறிலி fl = nodep->fileline();
        எனில் (nodep->lhsp()->isWide()) {
            எனில் (isImpure(nodep)) திரும்பு;
            எனில் (!doExpandWide(nodep->lhsp())) திரும்பு;
            didPattern(VExpandPattern::WORDIZE_REDAND, nodep);
            // -> (0!={and{for each_word{WORDSEL(lhs,#)}}}
            Ast_முனை_கோவை* newp = nullptr;
            க்கு (முழு w = 0; w < nodep->lhsp()->width_சொற்கள்(); ++w) {
                Ast_முனை_கோவை* eqp = newAstWordSelClone(nodep->lhsp(), w);
                எனில் (w == nodep->lhsp()->width_சொற்கள்() - 1) {
                    // Rather than doing a (slowish) ==##, we OR in the
                    // bits that aren't part of the mask
                    eqp = புதிய AstOr{fl, புதிய AstConst{fl, notWideMask(nodep->lhsp())},
                                    // Bug in cppcheck
                                    // cppcheck-suppress memleak
                                    eqp};
                }
                newp = newp ? புதிய AstAnd{fl, newp, eqp} : eqp;
            }
            newp = புதிய AstEq{fl, புதிய AstConst{fl, AstConst::SizedEData{}, VL_MASK_E(VL_EDATASIZE)},
                             newp};
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        } இல்லையெனில் {
            // No isImpure() check - can handle side effects in below
            didPattern(VExpandPattern::REDAND_TO_EQ, nodep);
            Ast_முனை_கோவை* மாறிலி lhsp = nodep->lhsp()->unlinkFrBack();
            Ast_முனை_கோவை* மாறிலி newp = புதிய AstEq{fl, புதிய AstConst{fl, wordMask(lhsp)}, lhsp};
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        }
    }
    வெறுமை visit(AstRedXor* nodep) override {
        எனில் (nodep->user1SetOnce()) திரும்பு;  // Process once
        iterateChildren(nodep);
        எனில் (nodep->lhsp()->isWide()) {
            எனில் (isImpure(nodep)) திரும்பு;
            எனில் (!doExpandWide(nodep->lhsp())) திரும்பு;
            didPattern(VExpandPattern::WORDIZE_REDXOR, nodep);
            // -> (0!={redxor{for each_word{XOR(WORDSEL(lhs,#))}}}
            File_வரி* மாறிலி fl = nodep->fileline();
            Ast_முனை_கோவை* newp = nullptr;
            க்கு (முழு w = 0; w < nodep->lhsp()->width_சொற்கள்(); ++w) {
                Ast_முனை_கோவை* மாறிலி eqp = newAstWordSelClone(nodep->lhsp(), w);
                newp = newp ? புதிய AstXor{fl, newp, eqp} : eqp;
            }
            newp = புதிய AstRedXor{fl, newp};
            UINFO(8, "    Wordize REDXORnew " << newp);
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        }
        // We don't reduce non-wide XORs, as its more efficient to use a temp register,
        // which the inlined function does nicely.
    }

    வெறுமை visit(Ast_முனை_Stmt* nodep) override {
        எனில் (nodep->user1SetOnce()) திரும்பு;  // Process once
        VL_RESTORER(m_stmtp);
        m_stmtp = nodep;
        iterateChildren(nodep);
    }
    வெறுமை visit(Ast_முனை_Assign* nodep) override {
        எனில் (nodep->user1SetOnce()) திரும்பு;  // Process once
        எனில் (VN_IS(nodep->dtypep()->skipRefp(), UnpackArrayDType)) {
            திரும்பு;  // Skip for UnpackArrayDType
        }

        VL_RESTORER(m_stmtp);
        m_stmtp = nodep;
        iterateChildren(nodep);
        தருக்க did = பொய்;
        எனில் (nodep->isWide()  //
            && ((VN_IS(nodep->lhsp(), VarRef) || VN_IS(nodep->lhsp(), ArraySel)))
            && !AstVar::scVarRecurse(nodep->lhsp())  // Need special function for SC
            && !AstVar::scVarRecurse(nodep->rhsp())) {
            எனில் (AstConst* மாறிலி rhsp = VN_CAST(nodep->rhsp(), Const)) {
                did = expandWide(nodep, rhsp);
            } இல்லையெனில் எனில் (AstVarRef* மாறிலி rhsp = VN_CAST(nodep->rhsp(), VarRef)) {
                did = expandWide(nodep, rhsp);
            } இல்லையெனில் எனில் (AstSel* மாறிலி rhsp = VN_CAST(nodep->rhsp(), Sel)) {
                did = expandWide(nodep, rhsp);
            } இல்லையெனில் எனில் (AstArraySel* மாறிலி rhsp = VN_CAST(nodep->rhsp(), ArraySel)) {
                did = expandWide(nodep, rhsp);
            } இல்லையெனில் எனில் (AstConcat* மாறிலி rhsp = VN_CAST(nodep->rhsp(), Concat)) {
                did = expandWide(nodep, rhsp);
            } இல்லையெனில் எனில் (AstExtend* மாறிலி rhsp = VN_CAST(nodep->rhsp(), Extend)) {
                did = expandWide(nodep, rhsp);
            } இல்லையெனில் எனில் (AstReplicate* மாறிலி rhsp = VN_CAST(nodep->rhsp(), Replicate)) {
                did = expandWide(nodep, rhsp);
            } இல்லையெனில் எனில் (AstAnd* மாறிலி rhsp = VN_CAST(nodep->rhsp(), And)) {
                did = expandWide(nodep, rhsp);
            } இல்லையெனில் எனில் (AstOr* மாறிலி rhsp = VN_CAST(nodep->rhsp(), Or)) {
                did = expandWide(nodep, rhsp);
            } இல்லையெனில் எனில் (AstNot* மாறிலி rhsp = VN_CAST(nodep->rhsp(), Not)) {
                did = expandWide(nodep, rhsp);
            } இல்லையெனில் எனில் (AstXor* மாறிலி rhsp = VN_CAST(nodep->rhsp(), Xor)) {
                did = expandWide(nodep, rhsp);
            } இல்லையெனில் எனில் (AstShiftL* மாறிலி rhsp = VN_CAST(nodep->rhsp(), ShiftL)) {
                did = expandWideShift(nodep, rhsp, /* isLeftShift: */ உண்மை);
            } இல்லையெனில் எனில் (AstShiftR* மாறிலி rhsp = VN_CAST(nodep->rhsp(), ShiftR)) {
                did = expandWideShift(nodep, rhsp, /* isLeftShift: */ பொய்);
            } இல்லையெனில் எனில் (AstCond* மாறிலி rhsp = VN_CAST(nodep->rhsp(), Cond)) {
                did = expandWide(nodep, rhsp);
            }
        } இல்லையெனில் எனில் (AstSel* மாறிலி lhsp = VN_CAST(nodep->lhsp(), Sel)) {
            did = expandLhs(nodep, lhsp);
        }
        // Cleanup common code
        எனில் (did) VL_DO_DANGLING(nodep->unlinkFrBack()->delete_மரம்(), nodep);
    }

    //--------------------
    வெறுமை visit(AstVar*) override {}  // Don't hit varrefs under vars
    வெறுமை visit(Ast_முனை* nodep) override { iterateChildren(nodep); }

பொது:
    // CONSTRUCTORS
    explicit ExpandVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~ExpandVisitor() override {
        V3Stats::addStat("Optimizations, Expand, expanded wides", m_statWides);
        V3Stats::addStat("Optimizations, Expand, expanded wide words", m_stat_Wide_சொற்கள்);
        V3Stats::addStat("Optimizations, Expand, not expanded impure", m_statImpure);
        V3Stats::addStat("Optimizations, Expand, not expanded limit", m_statWideLimited);
        V3Stats::addStat("Optimizations, Expand, temporaries created", m_statTemps);
        மாறிலி auto emitStat = [இது](VExpandPattern pattern) {
            std::string str{pattern.ascii()};
            std::transform(str.begin(), str.end(), str.begin(), [](unsigned எழுத்து c) {  //
                திரும்பு c == '_' ? ' ' : std::tolower(c);
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

வெறுமை V3Expand::expandAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        மாறிலி ExpandOkVisitor okVisitor{nodep};
        ExpandVisitor{nodep};
    }  // Destruct before checking
    V3Global::dump_Check_Global_மரம்("expand", 0, dump_மரம்_Either_Level() >= 3);
}
