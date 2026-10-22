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

#അന്തര്ഭാവ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#അന്തര്ഭാവ "V3Expand.h"

#അന്തര്ഭാവ "V3Const.h"
#അന്തര്ഭാവ "V3Stats.h"

#അന്തര്ഭാവ <algorithm>
#അന്തര്ഭാവ <array>

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

വർഗ്ഗം VExpandPattern final {
പൊതു:
    എണ്ണൽ en : uint8_t {
#define EXPAND_PATTERN_ID(id) id,
        FOR_EACH_EXPAND_PATTERN(EXPAND_PATTERN_ID)
#undef EXPAND_PATTERN_ID
            _ENUM_END
    };
    എണ്ണൽ en m_e;
    സ്ഥിരം അക്ഷരം* ascii() സ്ഥിരം {
        സ്ഥിതം സ്ഥിരം അക്ഷരം* സ്ഥിരം names[] = {
#define EXPAND_PATTERN_NAME(id) #id,
            FOR_EACH_EXPAND_PATTERN(EXPAND_PATTERN_NAME)
#undef EXPAND_PATTERN_NAME
                "_ENUM_END"  //
        };
        തിരികെ names[m_e];
    }

    // cppcheck-suppress noExplicitConstructor
    VExpandPattern(en _e)
        : m_e{_e} {}
    operator en() സ്ഥിരം { തിരികെ m_e; }
};

//######################################################################
// Find nodes with side effects, to mark as non-expandable

വർഗ്ഗം ExpandOkVisitor final : പൊതു VNVisitorConst {
    // NODE STATE
    //  AstNode::user2()        -> bool.  Is pure (along with all children)
    സ്ഥിരം VNUser2InUse m_inuser2;

    // STATE - for current visit position (use VL_RESTORER)
    // Tracks similar to AstNode::isTreePureRecurse(), but avoid O(n^2)
    // False = pure, as nodes that ExpandVisitor inserts preserve pureness
    തർക്കം m_isImpure = സത്യം;  // Currently pure

    ശൂന്യം visit(Ast_കണ്ണി* nodep) override {
        തർക്കം selfImpure = !nodep->isPure();
        {
            VL_RESTORER(m_isImpure);
            m_isImpure = അസത്യം;
            iterateChildrenConst(nodep);
            selfImpure |= m_isImpure;
            nodep->user2(selfImpure);
        }
        m_isImpure |= selfImpure;
    }

പൊതു:
    // CONSTRUCTORS
    explicit ExpandOkVisitor(AstNetlist* nodep) { iterateConst(nodep); }
    ~ExpandOkVisitor() = സ്ഥിരസ്ഥിതി;
};

//######################################################################
// Expand state, as a visitor of each AstNode

വർഗ്ഗം ExpandVisitor final : പൊതു VNVisitor {
    // NODE STATE
    //  AstNode::user1()        -> bool.  Processed
    //  AstNode::user2()        -> See ExpandOkVisitor
    സ്ഥിരം VNUser1InUse m_inuser1;

    // STATE - for current visit position (use VL_RESTORER)
    Ast_കണ്ണി* m_stmtp = nullptr;  // Current statement

    // STATE - across all visitors
    AstCFunc* m_funcp = nullptr;  // Current function
    VDouble0 m_statImpure;  // Statistic tracking
    VDouble0 m_statWides;  // Statistic tracking
    VDouble0 m_stat_Wide_വാക്കുകൾ;  // Statistic tracking
    VDouble0 m_statWideLimited;  // Statistic tracking
    VDouble0 m_statTemps;  // Statistic tracking
    // Number of applications of each expansion pattern for statistics
    std::array<VDouble0, VExpandPattern::_ENUM_END> m_statPatterns;

    // STATE - for current function
    size_t m_nTmps = 0;  // Sequence numbers for temopraries

    // METHODS
    // Note the application of the given expansion pattern to 'nodep'
    ശൂന്യം didPattern(VExpandPattern pattern, Ast_കണ്ണി* nodep) {
        UINFO(8, "    " << pattern.ascii() << " " << nodep);
        ++m_statPatterns[pattern];
    }

    // Use state that ExpandOkVisitor calculated
    സ്ഥിതം തർക്കം isImpure(Ast_കണ്ണി* nodep) {
        സ്ഥിരം തർക്കം impure = nodep->user2();
        എങ്കിൽ (impure) UINFO(9, "      impure " << nodep);
        തിരികെ impure;
    }

    തർക്കം doExpandWide(Ast_കണ്ണി* nodep) {
        എങ്കിൽ (isImpure(nodep)) {
            ++m_statImpure;
            തിരികെ അസത്യം;
        }
        എങ്കിൽ (nodep->width_വാക്കുകൾ() <= v3Global.opt.expandLimit()) {
            ++m_statWides;
            m_stat_Wide_വാക്കുകൾ += nodep->width_വാക്കുകൾ();
            തിരികെ സത്യം;
        } അല്ലെങ്കിൽ {
            ++m_statWideLimited;
            തിരികെ അസത്യം;
        }
    }

    സ്ഥിതം പൂർണ്ണസംഖ്യ longOrQuadWidth(Ast_കണ്ണി* nodep) {
        തിരികെ (nodep->width() + (VL_EDATASIZE - 1)) & ~(VL_EDATASIZE - 1);
    }
    സ്ഥിതം V3_സംഖ്യ notWideMask(Ast_കണ്ണി* nodep) {
        തിരികെ V3_സംഖ്യ{nodep, VL_EDATASIZE, ~VL_MASK_E(nodep->widthMin())};
    }
    സ്ഥിതം V3_സംഖ്യ wordMask(Ast_കണ്ണി* nodep) {
        എങ്കിൽ (nodep->isWide()) {
            തിരികെ V3_സംഖ്യ{nodep, VL_EDATASIZE, VL_MASK_E(nodep->widthMin())};
        } അല്ലെങ്കിൽ {
            V3_സംഖ്യ mask{nodep, longOrQuadWidth(nodep)};
            mask.setMask(nodep->widthMin());
            തിരികെ mask;
        }
    }

    സ്ഥിതം ശൂന്യം തിരുകുക_Before(Ast_കണ്ണി* placep, Ast_കണ്ണി* newp) {
        newp->user1(1);  // Already processed, don't need to re-iterate
        placep->addHereThisAsNext(newp);
    }
    സ്ഥിതം ശൂന്യം replaceWithDelete(Ast_കണ്ണി* nodep, Ast_കണ്ണി* newp) {
        newp->user1(1);  // Already processed, don't need to re-iterate
        എങ്കിൽ (newp->width() != nodep->width()) {
            UASSERT_OBJ(newp->widthMin() == nodep->widthMin(), nodep,
                        "Replacement width mismatch");
            newp->dtypeChgWidth(nodep->width(), nodep->widthMin());
        }
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_വൃക്ഷം(), nodep);
    }
    സ്ഥിതം Ast_കണ്ണി* newWordAssign(Ast_കണ്ണി_Assign* placep, പൂർണ്ണസംഖ്യ word, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp,
                                  Ast_കണ്ണി_വ്യഞ്ജകം* rhsp) {
        File_വരി* സ്ഥിരം fl = placep->fileline();
        തിരികെ പുതിയ AstAssign{fl,
                             പുതിയ AstWordSel{fl, lhsp->clone_വൃക്ഷം_Pure(സത്യം),
                                            പുതിയ AstConst{fl, static_cast<uint32_t>(word)}},
                             rhsp};
    }
    സ്ഥിതം ശൂന്യം addWordAssign(Ast_കണ്ണി_Assign* placep, പൂർണ്ണസംഖ്യ word, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp,
                              Ast_കണ്ണി_വ്യഞ്ജകം* rhsp) {
        തിരുകുക_Before(placep, newWordAssign(placep, word, lhsp, rhsp));
    }
    സ്ഥിതം ശൂന്യം addWordAssign(Ast_കണ്ണി_Assign* placep, പൂർണ്ണസംഖ്യ word, Ast_കണ്ണി_വ്യഞ്ജകം* rhsp) {
        addWordAssign(placep, word, placep->lhsp(), rhsp);
    }

    AstVar* addLocalTmp(Ast_കണ്ണി* placep, സ്ഥിരം അക്ഷരം* namep, Ast_കണ്ണി_വ്യഞ്ജകം* valuep) {
        സ്ഥിരം std::string name = "__V"s + namep + "_"s + std::to_string(m_nTmps);
        File_വരി* സ്ഥിരം flp = placep->fileline();
        AstVar* സ്ഥിരം tmpp = പുതിയ AstVar{flp, VVarType::STMTTEMP, name, valuep->dtypep()};
        tmpp->funcLocal(സത്യം);
        tmpp->isInternal(സത്യം);
        tmpp->noReset(സത്യം);
        tmpp->substConstOnly(സത്യം);
        m_funcp->addVarsp(tmpp);
        തിരുകുക_Before(placep, പുതിയ AstAssign{flp, പുതിയ AstVarRef{flp, tmpp, VAccess::WRITE}, valuep});
        ++m_statTemps;
        തിരികെ tmpp;
    }

    സ്ഥിതം ശൂന്യം fixCloneLvalue(Ast_കണ്ണി* nodep) {
        // In AstSel transforms, we call clone() on VarRefs that were lvalues,
        // but are now being used on the RHS of the assignment
        എങ്കിൽ (VN_IS(nodep, VarRef)) VN_AS(nodep, VarRef)->access(VAccess::READ);
        // Iterate
        എങ്കിൽ (Ast_കണ്ണി* സ്ഥിരം refp = nodep->op1p()) fixCloneLvalue(refp);
        എങ്കിൽ (Ast_കണ്ണി* സ്ഥിരം refp = nodep->op2p()) fixCloneLvalue(refp);
        എങ്കിൽ (Ast_കണ്ണി* സ്ഥിരം refp = nodep->op3p()) fixCloneLvalue(refp);
        എങ്കിൽ (Ast_കണ്ണി* സ്ഥിരം refp = nodep->op4p()) fixCloneLvalue(refp);
    }

    സ്ഥിതം Ast_കണ്ണി_വ്യഞ്ജകം* newAstWordSelClone(Ast_കണ്ണി_വ്യഞ്ജകം* nodep, പൂർണ്ണസംഖ്യ word) {
        // Get the specified word number from a wide array
        // Or, if it's a long/quad, do appropriate conversion to wide
        // Concat may pass negative word numbers, that means it wants a zero
        File_വരി* സ്ഥിരം fl = nodep->fileline();
        എങ്കിൽ (nodep->isWide() && word >= 0 && word < nodep->width_വാക്കുകൾ()) {
            തിരികെ പുതിയ AstWordSel{fl, nodep->clone_വൃക്ഷം_Pure(സത്യം),
                                  പുതിയ AstConst{fl, static_cast<uint32_t>(word)}};
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->isQuad() && word == 0) {
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം quadfromp = nodep->clone_വൃക്ഷം_Pure(സത്യം);
            quadfromp->dtypeSetBitUnsized(VL_QUADSIZE, quadfromp->widthMin(), VSigning::UNSIGNED);
            തിരികെ പുതിയ AstCCast{fl, quadfromp, VL_EDATASIZE};
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->isQuad() && word == 1) {
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം quadfromp = nodep->clone_വൃക്ഷം_Pure(സത്യം);
            quadfromp->dtypeSetBitUnsized(VL_QUADSIZE, quadfromp->widthMin(), VSigning::UNSIGNED);
            തിരികെ പുതിയ AstCCast{
                fl, പുതിയ AstShiftR{fl, quadfromp, പുതിയ AstConst{fl, VL_EDATASIZE}, VL_EDATASIZE},
                VL_EDATASIZE};
        } അല്ലെങ്കിൽ എങ്കിൽ (!nodep->isWide() && !nodep->isQuad() && word == 0) {
            തിരികെ nodep->clone_വൃക്ഷം_Pure(സത്യം);
        } അല്ലെങ്കിൽ {  // Out of bounds
            തിരികെ പുതിയ AstConst{fl, 0};
        }
    }

    സ്ഥിതം Ast_കണ്ണി_വ്യഞ്ജകം* newWordGrabShift(File_വരി* fl, പൂർണ്ണസംഖ്യ word, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, പൂർണ്ണസംഖ്യ shift) {
        // Extract the expression to grab the value for the specified word, if it's the shift
        // of shift bits from lhsp
        // Negative word numbers requested for lhs when it's "before" what we want.
        // We get a 0 then.
        സ്ഥിരം പൂർണ്ണസംഖ്യ othword = word - shift / VL_EDATASIZE;
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം lop = newAstWordSelClone(lhsp, othword);
        സ്ഥിരം uint32_t loShift = VL_BITBIT_E(shift);
        എങ്കിൽ (!loShift) തിരികെ lop;
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം hip = newAstWordSelClone(lhsp, othword - 1);
        സ്ഥിരം uint32_t hiShift = VL_EDATASIZE - loShift;  // Complement offset
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം newp
            = പുതിയ AstOr{fl,  //
                        പുതിയ AstShiftR{fl, hip, പുതിയ AstConst{fl, hiShift}, VL_EDATASIZE},
                        പുതിയ AstShiftL{fl, lop, പുതിയ AstConst{fl, loShift}, VL_EDATASIZE}};
        തിരികെ V3Const::constifyEditCpp(newp);
    }

    // Return expression indexing the word that contains 'lsbp' + the given word offset
    സ്ഥിതം Ast_കണ്ണി_വ്യഞ്ജകം* newWordIndex(Ast_കണ്ണി_വ്യഞ്ജകം* lsbp, uint32_t wordOffset = 0) {
        // This is indexing a WordSel, so a 32 bit constants are fine
        File_വരി* സ്ഥിരം flp = lsbp->fileline();
        എങ്കിൽ (സ്ഥിരം AstConst* constp = VN_CAST(lsbp, Const)) {
            തിരികെ പുതിയ AstConst{flp, wordOffset + VL_BITWORD_E(constp->toUInt())};
        }

        എങ്കിൽ (lsbp->backp()) lsbp = lsbp->clone_വൃക്ഷം_Pure(അസത്യം);
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം wwl2p = പുതിയ AstConst{flp, VL_EDATASIZE_LOG2};  // Word width log 2
        Ast_കണ്ണി_വ്യഞ്ജകം* indexp = പുതിയ AstShiftR{flp, lsbp, wwl2p, VL_EDATASIZE};
        എങ്കിൽ (wordOffset) indexp = പുതിയ AstAdd{flp, പുതിയ AstConst{flp, wordOffset}, indexp};
        തിരികെ indexp;
    }

    // Return word of fromp that contains word indexp + the given word offset.
    സ്ഥിതം Ast_കണ്ണി_വ്യഞ്ജകം* newWordSelWord(File_വരി* flp, Ast_കണ്ണി_വ്യഞ്ജകം* fromp, Ast_കണ്ണി_വ്യഞ്ജകം* indexp,
                                       uint32_t wordOffset = 0) {
        UASSERT_OBJ(fromp->isWide(), fromp, "Only need AstWordSel on wide from's");

        എങ്കിൽ (സ്ഥിരം AstConst* സ്ഥിരം constp = VN_CAST(indexp, Const)) {
            indexp = nullptr;
            wordOffset += constp->toUInt();
        }

        // e.g. "logic [95:0] var[0]; logic [0] sel; out = var[sel];"
        // Squash before C++ to avoid getting a C++ compiler warning
        // (even though code would be unreachable as presumably a
        // AstCond is protecting above this node.
        എങ്കിൽ (wordOffset >= static_cast<uint32_t>(fromp->width_വാക്കുകൾ())) {
            തിരികെ പുതിയ AstConst{flp, AstConst::SizedEData{}, 0};
        }

        എങ്കിൽ (fromp->backp()) fromp = fromp->clone_വൃക്ഷം_Pure(അസത്യം);

        // If indexp was constant, just use it
        എങ്കിൽ (!indexp) തിരികെ പുതിയ AstWordSel{flp, fromp, പുതിയ AstConst{flp, wordOffset}};

        // Otherwise compute at runtime
        എങ്കിൽ (indexp->backp()) indexp = indexp->clone_വൃക്ഷം_Pure(അസത്യം);
        എങ്കിൽ (wordOffset) indexp = പുതിയ AstAdd{flp, പുതിയ AstConst{flp, wordOffset}, indexp};
        തിരികെ പുതിയ AstWordSel{flp, fromp, indexp};
    }

    // Return word of fromp that contains bit lsbp + the given word offset.
    സ്ഥിതം Ast_കണ്ണി_വ്യഞ്ജകം* newWordSelBit(File_വരി* flp, Ast_കണ്ണി_വ്യഞ്ജകം* fromp, Ast_കണ്ണി_വ്യഞ്ജകം* lsbp,
                                      uint32_t wordOffset = 0) {
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം indexp = newWordIndex(lsbp, wordOffset);
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം wordSelp = newWordSelWord(flp, fromp, indexp);
        എങ്കിൽ (!indexp->backp()) VL_DO_DANGLING(indexp->delete_വൃക്ഷം(), indexp);
        തിരികെ wordSelp;
    }

    സ്ഥിതം Ast_കണ്ണി_വ്യഞ്ജകം* newSelBitBit(Ast_കണ്ണി_വ്യഞ്ജകം* lsbp) {
        // Return equation to get the VL_BITBIT of a constant or non-constant
        File_വരി* സ്ഥിരം fl = lsbp->fileline();
        എങ്കിൽ (VN_IS(lsbp, Const)) {
            തിരികെ പുതിയ AstConst{fl, VL_BITBIT_E(VN_AS(lsbp, Const)->toUInt())};
        } അല്ലെങ്കിൽ {
            തിരികെ പുതിയ AstAnd{fl, പുതിയ AstConst{fl, VL_EDATASIZE - 1}, lsbp->clone_വൃക്ഷം_Pure(സത്യം)};
        }
    }

    //====================

    തർക്കം expandWide(Ast_കണ്ണി_Assign* nodep, AstConst* rhsp) {
        എങ്കിൽ (!doExpandWide(nodep)) തിരികെ അസത്യം;
        didPattern(VExpandPattern::ASSIGN_CONST, nodep);
        // -> {for each_word{ ASSIGN(WORDSEL(wide,#),WORDSEL(CONST,#))}}
        എങ്കിൽ (rhsp->അക്കം().isFourState()) {
            rhsp->v3warn(E_UNSUPPORTED,  // LCOV_EXCL_LINE  // impossible?
                         "Unsupported: 4-state numbers in this context");
        }
        File_വരി* സ്ഥിരം fl = nodep->fileline();
        വേണ്ടി (പൂർണ്ണസംഖ്യ w = 0; w < nodep->width_വാക്കുകൾ(); ++w) {
            addWordAssign(nodep, w,
                          പുതിയ AstConst{fl, AstConst::SizedEData{}, rhsp->അക്കം().edataWord(w)});
        }
        തിരികെ സത്യം;
    }
    //-------- Uniops
    തർക്കം expandWide(Ast_കണ്ണി_Assign* nodep, AstVarRef* rhsp) {
        // Special case: do not expand assignment of constant pool variables.
        // V3Subst undestands these directly.
        എങ്കിൽ (rhsp->varp()->constPoolEntry()) തിരികെ അസത്യം;
        എങ്കിൽ (!doExpandWide(nodep)) തിരികെ അസത്യം;
        didPattern(VExpandPattern::ASSIGN_VARREF, nodep);
        വേണ്ടി (പൂർണ്ണസംഖ്യ w = 0; w < nodep->width_വാക്കുകൾ(); ++w) {
            addWordAssign(nodep, w, newAstWordSelClone(rhsp, w));
        }
        തിരികെ സത്യം;
    }
    തർക്കം expandWide(Ast_കണ്ണി_Assign* nodep, AstArraySel* rhsp) {
        UASSERT_OBJ(!VN_IS(nodep->dtypep()->skipRefp(), UnpackArrayDType), nodep,
                    "ArraySel with unpacked arrays should have been removed in V3Slice");
        എങ്കിൽ (!doExpandWide(nodep)) തിരികെ അസത്യം;
        didPattern(VExpandPattern::ASSIGN_ARRAYSEL, nodep);
        വേണ്ടി (പൂർണ്ണസംഖ്യ w = 0; w < nodep->width_വാക്കുകൾ(); ++w) {
            addWordAssign(nodep, w, newAstWordSelClone(rhsp, w));
        }
        തിരികെ സത്യം;
    }
    തർക്കം expandWide(Ast_കണ്ണി_Assign* nodep, AstNot* rhsp) {
        // -> {for each_word{ ASSIGN(WORDSEL(wide,#),NOT(WORDSEL(lhs,#))) }}
        എങ്കിൽ (!doExpandWide(nodep)) തിരികെ അസത്യം;
        didPattern(VExpandPattern::ASSIGN_NOT, nodep);
        File_വരി* സ്ഥിരം fl = rhsp->fileline();
        വേണ്ടി (പൂർണ്ണസംഖ്യ w = 0; w < nodep->width_വാക്കുകൾ(); ++w) {
            addWordAssign(nodep, w, പുതിയ AstNot{fl, newAstWordSelClone(rhsp->lhsp(), w)});
        }
        തിരികെ സത്യം;
    }
    //-------- Biops
    തർക്കം expandWide(Ast_കണ്ണി_Assign* nodep, AstAnd* rhsp) {
        എങ്കിൽ (!doExpandWide(nodep)) തിരികെ അസത്യം;
        didPattern(VExpandPattern::ASSIGN_AND, nodep);
        File_വരി* സ്ഥിരം fl = nodep->fileline();
        വേണ്ടി (പൂർണ്ണസംഖ്യ w = 0; w < nodep->width_വാക്കുകൾ(); ++w) {
            addWordAssign(nodep, w,
                          പുതിയ AstAnd{fl, newAstWordSelClone(rhsp->lhsp(), w),
                                     newAstWordSelClone(rhsp->rhsp(), w)});
        }
        തിരികെ സത്യം;
    }
    തർക്കം expandWide(Ast_കണ്ണി_Assign* nodep, AstOr* rhsp) {
        എങ്കിൽ (!doExpandWide(nodep)) തിരികെ അസത്യം;
        didPattern(VExpandPattern::ASSIGN_OR, nodep);
        File_വരി* സ്ഥിരം fl = nodep->fileline();
        വേണ്ടി (പൂർണ്ണസംഖ്യ w = 0; w < nodep->width_വാക്കുകൾ(); ++w) {
            addWordAssign(nodep, w,
                          പുതിയ AstOr{fl, newAstWordSelClone(rhsp->lhsp(), w),
                                    newAstWordSelClone(rhsp->rhsp(), w)});
        }
        തിരികെ സത്യം;
    }
    തർക്കം expandWide(Ast_കണ്ണി_Assign* nodep, AstXor* rhsp) {
        എങ്കിൽ (!doExpandWide(nodep)) തിരികെ അസത്യം;
        didPattern(VExpandPattern::ASSIGN_XOR, nodep);
        File_വരി* സ്ഥിരം fl = nodep->fileline();
        വേണ്ടി (പൂർണ്ണസംഖ്യ w = 0; w < nodep->width_വാക്കുകൾ(); ++w) {
            addWordAssign(nodep, w,
                          പുതിയ AstXor{fl, newAstWordSelClone(rhsp->lhsp(), w),
                                     newAstWordSelClone(rhsp->rhsp(), w)});
        }
        തിരികെ സത്യം;
    }
    തർക്കം expandWideShift(Ast_കണ്ണി_Assign* nodep, Ast_കണ്ണി_Biop* rhsp, തർക്കം is_ഇടത്_Shift) {
        എങ്കിൽ (!doExpandWide(nodep)) തിരികെ അസത്യം;

        // Simplify the shift amount, in case it becomes a constant
        V3Const::constifyEditCpp(rhsp->rhsp());

        // If it's a constant shift by whole words, expand it so V3Subst can substitute it
        എങ്കിൽ (സ്ഥിരം AstConst* സ്ഥിരം rhsConstp = VN_CAST(rhsp->rhsp(), Const)) {
            സ്ഥിരം uint32_t shiftBits = rhsConstp->toUInt();
            എങ്കിൽ (VL_BITBIT_E(shiftBits) == 0) {
                സ്ഥിരം പൂർണ്ണസംഖ്യ width_വാക്കുകൾ = nodep->width_വാക്കുകൾ();
                സ്ഥിരം പൂർണ്ണസംഖ്യ shift_വാക്കുകൾ = std::min<പൂർണ്ണസംഖ്യ>(VL_BITWORD_E(shiftBits), width_വാക്കുകൾ);
                File_വരി* സ്ഥിരം flp = rhsp->fileline();
                എങ്കിൽ (is_ഇടത്_Shift) {
                    didPattern(VExpandPattern::ASSIGN_SHIFTL, nodep);
                    // Low words of the result are zero
                    വേണ്ടി (പൂർണ്ണസംഖ്യ w = 0; w < shift_വാക്കുകൾ; ++w) {
                        addWordAssign(nodep, w, പുതിയ AstConst{flp, AstConst::SizedEData{}, 0});
                    }
                    // High words of the result are copied from higher words of the source
                    വേണ്ടി (പൂർണ്ണസംഖ്യ w = shift_വാക്കുകൾ; w < width_വാക്കുകൾ; ++w) {
                        addWordAssign(nodep, w, newAstWordSelClone(rhsp->lhsp(), w - shift_വാക്കുകൾ));
                    }
                } അല്ലെങ്കിൽ {
                    didPattern(VExpandPattern::ASSIGN_SHIFTR, nodep);
                    // Low words of the result are copied from higher words of the source
                    വേണ്ടി (പൂർണ്ണസംഖ്യ w = 0; w < width_വാക്കുകൾ - shift_വാക്കുകൾ; ++w) {
                        addWordAssign(nodep, w, newAstWordSelClone(rhsp->lhsp(), w + shift_വാക്കുകൾ));
                    }
                    // High words of the result are zero
                    വേണ്ടി (പൂർണ്ണസംഖ്യ w = width_വാക്കുകൾ - shift_വാക്കുകൾ; w < width_വാക്കുകൾ; ++w) {
                        addWordAssign(nodep, w, പുതിയ AstConst{flp, AstConst::SizedEData{}, 0});
                    }
                }
                തിരികെ സത്യം;
            }
        }

        തിരികെ അസത്യം;
    }

    //-------- Triops
    തർക്കം expandWide(Ast_കണ്ണി_Assign* nodep, AstCond* rhsp) {
        എങ്കിൽ (!doExpandWide(nodep)) തിരികെ അസത്യം;
        didPattern(VExpandPattern::ASSIGN_COND, nodep);
        File_വരി* സ്ഥിരം fl = nodep->fileline();
        വേണ്ടി (പൂർണ്ണസംഖ്യ w = 0; w < nodep->width_വാക്കുകൾ(); ++w) {
            addWordAssign(nodep, w,
                          പുതിയ AstCond{fl, rhsp->condp()->clone_വൃക്ഷം_Pure(സത്യം),
                                      newAstWordSelClone(rhsp->thenp(), w),
                                      newAstWordSelClone(rhsp->elsep(), w)});
        }
        തിരികെ സത്യം;
    }

    // VISITORS
    ശൂന്യം visit(AstNetlist* nodep) override { iterateAndNextNull(nodep->modulesp()); }

    ശൂന്യം visit(AstCFunc* nodep) override {
        VL_RESTORER(m_funcp);
        VL_RESTORER(m_nTmps);
        m_funcp = nodep;
        m_nTmps = 0;
        സ്ഥിരം VDouble0 statWidesBefore = m_statWides;
        iterateChildren(nodep);

        // Constant fold here if anything was expanded, as Ast size can likely be reduced
        എങ്കിൽ (v3Global.opt.fConstEager() && m_statWides != statWidesBefore) {
            Ast_കണ്ണി* സ്ഥിരം editedp = V3Const::constifyEditCpp(nodep);
            UASSERT_OBJ(editedp == nodep, editedp, "Should not have replaced CFunc");
        }
    }

    ശൂന്യം visit(AstExtend* nodep) override {
        എങ്കിൽ (nodep->user1SetOnce()) തിരികെ;  // Process once
        iterateChildren(nodep);
        എങ്കിൽ (nodep->isWide()) {
            // See under ASSIGN(EXTEND)
        } അല്ലെങ്കിൽ {
            എങ്കിൽ (isImpure(nodep)) തിരികെ;
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം lhsp = nodep->lhsp()->unlinkFrBack();
            Ast_കണ്ണി_വ്യഞ്ജകം* newp = lhsp;
            എങ്കിൽ (nodep->isQuad()) {
                എങ്കിൽ (lhsp->isQuad()) {
                    lhsp->dtypeFrom(nodep);  // Just mark it, else nop
                } അല്ലെങ്കിൽ എങ്കിൽ (lhsp->isWide()) {
                    nodep->v3fatalSrc("extending larger thing into smaller?");
                } അല്ലെങ്കിൽ {
                    newp = പുതിയ AstCCast{nodep->fileline(), lhsp, nodep};
                }
            } അല്ലെങ്കിൽ {  // Long
                UASSERT_OBJ(!(lhsp->isQuad() || lhsp->isWide()), nodep,
                            "extending larger thing into smaller?");
                lhsp->dtypeFrom(nodep);  // Just mark it, else nop
            }
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        }
    }
    തർക്കം expandWide(Ast_കണ്ണി_Assign* nodep, AstExtend* rhsp) {
        എങ്കിൽ (!doExpandWide(nodep)) തിരികെ അസത്യം;
        didPattern(VExpandPattern::ASSIGN_EXTEND, nodep);
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം rlhsp = rhsp->lhsp();
        വേണ്ടി (പൂർണ്ണസംഖ്യ w = 0; w < rlhsp->width_വാക്കുകൾ(); ++w) {
            addWordAssign(nodep, w, newAstWordSelClone(rlhsp, w));
        }
        വേണ്ടി (പൂർണ്ണസംഖ്യ w = rlhsp->width_വാക്കുകൾ(); w < nodep->width_വാക്കുകൾ(); ++w) {
            addWordAssign(nodep, w, പുതിയ AstConst{nodep->fileline(), AstConst::SizedEData{}, 0});
        }
        തിരികെ സത്യം;
    }

    ശൂന്യം visit(AstSel* nodep) override {
        എങ്കിൽ (nodep->user1SetOnce()) തിരികെ;  // Process once
        iterateChildren(nodep);

        സ്ഥിരം uint32_t width = static_cast<uint32_t>(nodep->widthConst());
        UASSERT_OBJ(nodep->widthMin() == static_cast<പൂർണ്ണസംഖ്യ>(width), nodep, "Width mismatch");

        // Skip if Sel is an LHS assignment select
        എങ്കിൽ (Ast_കണ്ണി_Assign* സ്ഥിരം assignp = VN_CAST(nodep->backp(), കണ്ണി_Assign)) {
            എങ്കിൽ (nodep == assignp->lhsp()) തിരികെ;
        }

        // Skip if wide: See under ASSIGN(WIDE)
        എങ്കിൽ (nodep->isWide()) തിരികെ;

        // Skip Sel from stream or queue
        എങ്കിൽ (VN_IS(nodep->fromp()->dtypep(), StreamDType)) തിരികെ;
        എങ്കിൽ (VN_IS(nodep->fromp()->dtypep(), QueueDType)) തിരികെ;

        // Result must be non-wide after skipping all the above
        UASSERT_OBJ(width <= 64, nodep, "Inconsistent result width");

        എങ്കിൽ (nodep->fromp()->isWide()) {  // Long/Quad from Wide
            എങ്കിൽ (isImpure(nodep)) തിരികെ;
            didPattern(VExpandPattern::SEL_FROM_WIDE, nodep);
            File_വരി* സ്ഥിരം nfl = nodep->fileline();
            File_വരി* സ്ഥിരം lfl = nodep->lsbp()->fileline();
            File_വരി* സ്ഥിരം ffl = nodep->fromp()->fileline();
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം fromp = nodep->fromp();

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
            സ്ഥിരം auto tryFold = [&](Ast_കണ്ണി_വ്യഞ്ജകം* exprp) -> Ast_കണ്ണി_വ്യഞ്ജകം* {
                // Size limit for inlining expressions (any arbitrary value results in
                // a bounded expansion. Picked a value to allow common simple forms.)
                സ്ഥിതം constexpr പൂർണ്ണസംഖ്യ EXPAND_SEL_LSB_LIMIT = 8;
                // Simplify
                exprp = V3Const::constifyEditCpp(exprp);
                // Accept if not larger than the limit
                എങ്കിൽ (!exprp->isLargerThan(EXPAND_SEL_LSB_LIMIT)) തിരികെ exprp;
                // Delete if rejected
                VL_DO_DANGLING(exprp->delete_വൃക്ഷം(), exprp);
                തിരികെ nullptr;
            };

            // Return the index of the word holding the bit 'offset' bits above the LSB
            സ്ഥിരം auto wordIdx = [&](uint32_t offset) -> Ast_കണ്ണി_വ്യഞ്ജകം* {
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം msbp = പുതിയ AstAdd{lfl, പുതിയ AstConst{lfl, offset},
                                                     nodep->lsbp()->clone_വൃക്ഷം_Pure(അസത്യം)};
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം idxp = newWordIndex(msbp);
                എങ്കിൽ (!msbp->backp()) VL_DO_DANGLING(msbp->delete_വൃക്ഷം(), msbp);
                തിരികെ idxp;
            };

            // Bit index of the selected LSB within its word
            Ast_കണ്ണി_വ്യഞ്ജകം* lBitp = tryFold(newSelBitBit(nodep->lsbp()));
            സ്ഥിരം തർക്കം aligned = lBitp && lBitp->isZero();

            // Which of the other 2 words the expansion might need
            സ്ഥിരം തർക്കം mNeeded = width > 1 && !aligned;
            സ്ഥിരം തർക്കം hNeeded = width > VL_EDATASIZE;
            UASSERT_OBJ(!hNeeded || nodep->isQuad(), nodep, "Width mismatch");

            // Word indices
            സ്ഥിരം uint32_t mMsbOffset = std::min<uint32_t>(width, VL_EDATASIZE) - 1;
            സ്ഥിരം uint32_t hMsbOffset = width - 1;
            Ast_കണ്ണി_വ്യഞ്ജകം* lIdxp = tryFold(newWordIndex(nodep->lsbp()));
            Ast_കണ്ണി_വ്യഞ്ജകം* mIdxp = mNeeded ? tryFold(wordIdx(mMsbOffset)) : nullptr;
            Ast_കണ്ണി_വ്യഞ്ജകം* hIdxp = hNeeded ? tryFold(wordIdx(hMsbOffset)) : nullptr;

            // If the LSB expression is needed multiple times, evaluate it into a temporary
            സ്ഥിരം പൂർണ്ണസംഖ്യ nCopies = !lBitp + !lIdxp + (mNeeded && !mIdxp) + (hNeeded && !hIdxp);
            എങ്കിൽ (nCopies > 1 && m_funcp && m_stmtp && !isImpure(m_stmtp)) {
                ++m_nTmps;  // Use fresh set of temporaries
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം lsbp = nodep->lsbp()->unlinkFrBack();
                AstVar* സ്ഥിരം tmpp = addLocalTmp(m_stmtp, "ExpandSel_Lsb", lsbp);
                nodep->lsbp(പുതിയ AstVarRef{lfl, tmpp, VAccess::READ});
            }

            // Rebuild remaining expressions derived from LSB if needed, now using the temporary
            എങ്കിൽ (!lBitp) lBitp = newSelBitBit(nodep->lsbp());
            എങ്കിൽ (!lIdxp) lIdxp = newWordIndex(nodep->lsbp());
            എങ്കിൽ (!mIdxp) mIdxp = mNeeded ? wordIdx(mMsbOffset) : nullptr;
            എങ്കിൽ (!hIdxp) hIdxp = hNeeded ? wordIdx(hMsbOffset) : nullptr;

            // Return word 'idxp' of 'fromp', without consuming 'idxp'
            സ്ഥിരം auto wordSel = [&](Ast_കണ്ണി_വ്യഞ്ജകം* idxp) -> Ast_കണ്ണി_വ്യഞ്ജകം* {
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം clonep = idxp->clone_വൃക്ഷം_Pure(അസത്യം);
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം wordp = newWordSelWord(ffl, fromp, clonep);
                എങ്കിൽ (!clonep->backp()) VL_DO_DANGLING(clonep->delete_വൃക്ഷം(), clonep);
                തിരികെ wordp;
            };

            // Construct term containing the bits of the low word - always needed
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം lTermp = [&]() -> Ast_കണ്ണി_വ്യഞ്ജകം* {
                Ast_കണ്ണി_വ്യഞ്ജകം* lWordp = wordSel(lIdxp);
                എങ്കിൽ (nodep->isQuad()) lWordp = പുതിയ AstCCast{nfl, lWordp, nodep};
                തിരികെ പുതിയ AstShiftR{nfl, lWordp, lBitp->clone_വൃക്ഷം_Pure(അസത്യം), nodep->width()};
            }();

            // Construct term containing the bits of the middle word - if needed
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം mTermp = [&]() -> Ast_കണ്ണി_വ്യഞ്ജകം* {
                എങ്കിൽ (!mNeeded) തിരികെ nullptr;

                Ast_കണ്ണി_വ്യഞ്ജകം* mWordp = wordSel(mIdxp);
                എങ്കിൽ (nodep->isQuad()) mWordp = പുതിയ AstCCast{nfl, mWordp, nodep};
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം mShiftp = പുതിയ AstSub{lfl, പുതിയ AstConst{lfl, VL_EDATASIZE},
                                                        lBitp->clone_വൃക്ഷം_Pure(അസത്യം)};
                // If we're selecting bit zero, then all 32 bits in the mid word
                // get shifted << by 32 bits, so ignore them.
                സ്ഥിരം V3_സംഖ്യ zero{nodep, longOrQuadWidth(nodep)};
                തിരികെ പുതിയ AstCond{
                    nfl,
                    // lsb % VL_EDATASIZE == 0 ?
                    പുതിയ AstEq{nfl, പുതിയ AstConst{nfl, 0}, lBitp->clone_വൃക്ഷം_Pure(അസത്യം)},
                    // 0 :
                    പുതിയ AstConst{nfl, zero},
                    //  midword >> (VL_EDATASIZE - (lbs % VL_EDATASIZE))
                    പുതിയ AstShiftL{nfl, mWordp, mShiftp, nodep->width()}};
            }();

            // Construct term containing the bits of the high word - if needed
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം hTermp = [&]() -> Ast_കണ്ണി_വ്യഞ്ജകം* {
                എങ്കിൽ (!hNeeded) തിരികെ nullptr;

                Ast_കണ്ണി_വ്യഞ്ജകം* hWordp = wordSel(hIdxp);
                hWordp = പുതിയ AstCCast{nfl, hWordp, nodep};
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം hShiftp = പുതിയ AstCond{
                    nfl,
                    // lsb % VL_EDATASIZE == 0 ?
                    പുതിയ AstEq{nfl, പുതിയ AstConst{nfl, 0}, lBitp->clone_വൃക്ഷം_Pure(അസത്യം)},
                    // VL_EDATASIZE :
                    പുതിയ AstConst{lfl, VL_EDATASIZE},
                    // 64 - (lbs % VL_EDATASIZE)
                    പുതിയ AstSub{lfl, പുതിയ AstConst{lfl, 64}, lBitp->clone_വൃക്ഷം_Pure(അസത്യം)}};
                തിരികെ പുതിയ AstShiftL{nfl, hWordp, hShiftp, nodep->width()};
            }();

            // Delete parts not captured during construction
            VL_DO_DANGLING(lBitp->delete_വൃക്ഷം(), lBitp);
            VL_DO_DANGLING(lIdxp->delete_വൃക്ഷം(), lIdxp);
            എങ്കിൽ (mIdxp) VL_DO_DANGLING(mIdxp->delete_വൃക്ഷം(), mIdxp);
            എങ്കിൽ (hIdxp) VL_DO_DANGLING(hIdxp->delete_വൃക്ഷം(), hIdxp);

            // Or reduce the terms
            Ast_കണ്ണി_വ്യഞ്ജകം* newp = lTermp;
            എങ്കിൽ (mTermp) newp = പുതിയ AstOr{nfl, mTermp, newp};
            എങ്കിൽ (hTermp) newp = പുതിയ AstOr{nfl, hTermp, newp};
            newp->dtypeFrom(nodep);
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        } അല്ലെങ്കിൽ {  // Long/Quad from Long/Quad
            // No isImpure() check - can handle side effects in below
            didPattern(VExpandPattern::SEL_FROM_NARROW, nodep);
            File_വരി* സ്ഥിരം fl = nodep->fileline();
            Ast_കണ്ണി_വ്യഞ്ജകം* fromp = nodep->fromp()->unlinkFrBack();
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം lsbp = nodep->lsbp()->unlinkFrBack();
            എങ്കിൽ (nodep->isQuad() && !fromp->isQuad()) fromp = പുതിയ AstCCast{fl, fromp, nodep};
            // {large}>>32 requires 64-bit shift operation; then cast
            Ast_കണ്ണി_വ്യഞ്ജകം* newp = പുതിയ AstShiftR{fl, fromp, lsbp, fromp->width()};
            newp->dtypeFrom(fromp);
            എങ്കിൽ (!nodep->isQuad() && fromp->isQuad()) newp = പുതിയ AstCCast{fl, newp, nodep};
            newp->dtypeFrom(nodep);
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        }
    }

    തർക്കം expandWide(Ast_കണ്ണി_Assign* nodep, AstSel* rhsp) {
        UASSERT_OBJ(nodep->widthMin() == rhsp->widthConst(), nodep, "Width mismatch");
        എങ്കിൽ (!doExpandWide(nodep)) തിരികെ അസത്യം;

        // Simplify the index, in case it becomes a constant
        V3Const::constifyEditCpp(rhsp->lsbp());

        // If it's a constant select and aligned, we can just copy the words
        എങ്കിൽ (സ്ഥിരം AstConst* സ്ഥിരം lsbConstp = VN_CAST(rhsp->lsbp(), Const)) {
            സ്ഥിരം uint32_t lsb = lsbConstp->toUInt();
            എങ്കിൽ (VL_BITBIT_E(lsb) == 0) {
                didPattern(VExpandPattern::ASSIGN_SEL_ALIGNED, nodep);
                സ്ഥിരം uint32_t word = VL_BITWORD_E(lsb);
                വേണ്ടി (പൂർണ്ണസംഖ്യ w = 0; w < nodep->width_വാക്കുകൾ(); ++w) {
                    addWordAssign(nodep, w, newAstWordSelClone(rhsp->fromp(), w + word));
                }
                തിരികെ സത്യം;
            }
        }

        didPattern(VExpandPattern::ASSIGN_SEL_UNALIGNED, nodep);
        File_വരി* സ്ഥിരം flp = rhsp->fileline();

        // Use fresh set of temporaries
        ++m_nTmps;

        // Compute word index of LSB, store to temporary if not constant
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം wordLsbp = rhsp->lsbp()->clone_വൃക്ഷം_Pure(അസത്യം);
        Ast_കണ്ണി_വ്യഞ്ജകം* wordIdxp = newWordIndex(wordLsbp);
        എങ്കിൽ (!wordLsbp->backp()) VL_DO_DANGLING(wordLsbp->delete_വൃക്ഷം(), wordLsbp);
        wordIdxp = V3Const::constifyEditCpp(wordIdxp);
        എങ്കിൽ (!VN_IS(wordIdxp, Const)) {
            AstVar* സ്ഥിരം tmpp = addLocalTmp(nodep, "ExpandSel_WordIdx", wordIdxp);
            wordIdxp = പുതിയ AstVarRef{flp, tmpp, VAccess::READ};
        }

        // Compute shift amounts and mask, store to temporaries if not constants
        Ast_കണ്ണി_വ്യഞ്ജകം* loShftp = nullptr;
        Ast_കണ്ണി_വ്യഞ്ജകം* hiShftp = nullptr;
        Ast_കണ്ണി_വ്യഞ്ജകം* hiMaskp = nullptr;
        എങ്കിൽ (സ്ഥിരം AstConst* സ്ഥിരം lsbConstp = VN_CAST(rhsp->lsbp(), Const)) {
            സ്ഥിരം uint32_t bitOffset = VL_BITBIT_E(lsbConstp->toUInt());
            // Must be unaligned, otherwise we would have handled it above
            UASSERT_OBJ(bitOffset, nodep, "Missed aligned wide select");
            loShftp = പുതിയ AstConst{flp, bitOffset};
            hiShftp = പുതിയ AstConst{flp, VL_EDATASIZE - bitOffset};
            hiMaskp = nullptr;
        } അല്ലെങ്കിൽ {
            // Compute Low word shift amount: bottom bits of lsb index
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം lsbp = rhsp->lsbp()->clone_വൃക്ഷം_Pure(അസത്യം);
            loShftp = പുതിയ AstAnd{flp, പുതിയ AstConst{flp, VL_SIZEBITS_E}, lsbp};
            AstVar* സ്ഥിരം loTmpp = addLocalTmp(nodep, "ExpandSel_LoShift", loShftp);
            loShftp = പുതിയ AstVarRef{flp, loTmpp, VAccess::READ};

            // Compute if aligned: Low word shift amount is 0
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം zerop = പുതിയ AstConst{flp, 0};
            Ast_കണ്ണി_വ്യഞ്ജകം* alignedp = പുതിയ AstEq{flp, zerop, loShftp->clone_വൃക്ഷം_Pure(അസത്യം)};
            AstVar* സ്ഥിരം alignedTmpp = addLocalTmp(nodep, "ExpandSel_Aligned", alignedp);
            alignedp = പുതിയ AstVarRef{flp, alignedTmpp, VAccess::READ};

            // Computed High word shift amount: 0 if aligned else VL_EDATASIZE - Low word shift
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം edsp = പുതിയ AstConst{flp, VL_EDATASIZE};
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം subp = പുതിയ AstSub{flp, edsp, loShftp->clone_വൃക്ഷം_Pure(അസത്യം)};
            hiShftp = പുതിയ AstCond{flp, alignedp, പുതിയ AstConst{flp, 0}, subp};
            AstVar* സ്ഥിരം hiTmpp = addLocalTmp(nodep, "ExpandSel_HiShift", hiShftp);
            hiShftp = പുതിയ AstVarRef{flp, hiTmpp, VAccess::READ};

            // Compute the High word mask: 0 if aligned, ones otherwise
            hiMaskp = പുതിയ AstCond{flp, alignedp->clone_വൃക്ഷം_Pure(അസത്യം),
                                  പുതിയ AstConst{flp, AstConst::SizedEData{}, 0},
                                  പുതിയ AstConst{flp, AstConst::SizedEData{}, -1ULL}};
            AstVar* സ്ഥിരം maskTmpp = addLocalTmp(nodep, "ExpandSel_HiMask", hiMaskp);
            hiMaskp = പുതിയ AstVarRef{flp, maskTmpp, VAccess::READ};
        }

        // Create each word of the selected result
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം fromp = rhsp->fromp();
        സ്ഥിരം പൂർണ്ണസംഖ്യ sel_വാക്കുകൾ = nodep->width_വാക്കുകൾ();
        വേണ്ടി (പൂർണ്ണസംഖ്യ w = 0; w < sel_വാക്കുകൾ; ++w) {
            // Grab bits from word 'VL_BITWORD_E(lsb) + w'
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം loWordp = newWordSelWord(fromp->fileline(), fromp, wordIdxp, w);
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം loShftClonep = loShftp->clone_വൃക്ഷം_Pure(അസത്യം);
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം lop = പുതിയ AstShiftR{flp, loWordp, loShftClonep, VL_EDATASIZE};
            // Grab bits from word 'VL_BITWORD_E(lsb) + w + 1'
            Ast_കണ്ണി_വ്യഞ്ജകം* hiWordp = newWordSelWord(fromp->fileline(), fromp, wordIdxp, w + 1);
            // For the last word of the result, avoid an OOB access when the Sel is not OOB
            എങ്കിൽ (w == sel_വാക്കുകൾ - 1) {
                സ്ഥിരം uint32_t max = fromp->width_വാക്കുകൾ() - w - 1;
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം maxp = പുതിയ AstConst{flp, max};
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം condp = പുതിയ AstGte{flp, wordIdxp->clone_വൃക്ഷം_Pure(അസത്യം), maxp};
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം zerop = പുതിയ AstConst{flp, AstConst::SizedEData{}, 0};
                hiWordp = പുതിയ AstCond{flp, condp, zerop, hiWordp};
            }
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം hiShftClonep = hiShftp->clone_വൃക്ഷം_Pure(അസത്യം);
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം hiPartp = പുതിയ AstShiftL{flp, hiWordp, hiShftClonep, VL_EDATASIZE};
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം hip
                = hiMaskp ? പുതിയ AstAnd{flp, hiPartp, hiMaskp->clone_വൃക്ഷം_Pure(അസത്യം)} : hiPartp;
            // Combine them
            addWordAssign(nodep, w, പുതിയ AstOr{nodep->fileline(), hip, lop});
        }

        // Delete parts not captured during construction
        എങ്കിൽ (!wordIdxp->backp()) VL_DO_DANGLING(wordIdxp->delete_വൃക്ഷം(), wordIdxp);
        എങ്കിൽ (!loShftp->backp()) VL_DO_DANGLING(loShftp->delete_വൃക്ഷം(), loShftp);
        എങ്കിൽ (!hiShftp->backp()) VL_DO_DANGLING(hiShftp->delete_വൃക്ഷം(), hiShftp);
        എങ്കിൽ (hiMaskp && !hiMaskp->backp()) VL_DO_DANGLING(hiMaskp->delete_വൃക്ഷം(), hiMaskp);

        തിരികെ സത്യം;
    }

    തർക്കം expandLhs(Ast_കണ്ണി_Assign* nodep, AstSel* lhsp) {
        // Possibilities
        //      destp: wide or narrow
        //      rhsp:  wide (destp must be wide), narrow, or 1 bit wide
        //      rhsp:  may be allones and can remove AND NOT gate
        //      lsbp:  constant or variable
        // Yuk.
        എങ്കിൽ (isImpure(nodep)) തിരികെ അസത്യം;
        File_വരി* സ്ഥിരം nfl = nodep->fileline();
        File_വരി* സ്ഥിരം lfl = lhsp->fileline();
        സ്ഥിരം തർക്കം destwide = lhsp->fromp()->isWide();
        സ്ഥിരം തർക്കം ones = nodep->rhsp()->isAllOnesV();
        എങ്കിൽ (VN_IS(lhsp->lsbp(), Const)) {
            // The code should work without this constant test, but it won't
            // constify as nicely as we'd like.
            Ast_കണ്ണി_വ്യഞ്ജകം* rhsp = nodep->rhsp()->unlinkFrBack();
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം destp = lhsp->fromp()->unlinkFrBack();
            സ്ഥിരം പൂർണ്ണസംഖ്യ lsb = lhsp->lsbConst();
            സ്ഥിരം പൂർണ്ണസംഖ്യ msb = lhsp->msbConst();
            V3_സംഖ്യ maskset{nodep, destp->widthMin()};
            maskset.setMask(msb + 1 - lsb, lsb);
            V3_സംഖ്യ maskold{nodep, destp->widthMin()};
            maskold.opNot(maskset);
            എങ്കിൽ (destwide) {
                didPattern(VExpandPattern::ASSIGN_TO_SEL_CONST_WIDE, nodep);
                വേണ്ടി (പൂർണ്ണസംഖ്യ w = 0; w < destp->width_വാക്കുകൾ(); ++w) {
                    എങ്കിൽ (w >= VL_BITWORD_E(lsb) && w <= VL_BITWORD_E(msb)) {
                        // else we would just be setting it to the same exact value
                        Ast_കണ്ണി_വ്യഞ്ജകം* oldvalp = newAstWordSelClone(destp, w);
                        fixCloneLvalue(oldvalp);
                        എങ്കിൽ (!ones) {
                            oldvalp = പുതിയ AstAnd{
                                lfl,
                                പുതിയ AstConst{lfl, AstConst::SizedEData{}, maskold.edataWord(w)},
                                oldvalp};
                        }

                        // Appropriate word of new value to insert:
                        Ast_കണ്ണി_വ്യഞ്ജകം* newp = newWordGrabShift(lfl, w, rhsp, lsb);

                        // Apply cleaning at the top word of the destination
                        // (no cleaning to do if dst's width is a whole number
                        // of words).
                        എങ്കിൽ (w == destp->width_വാക്കുകൾ() - 1 && VL_BITBIT_E(destp->widthMin()) != 0) {
                            V3_സംഖ്യ cleanmask{nodep, VL_EDATASIZE};
                            cleanmask.setMask(VL_BITBIT_E(destp->widthMin()));
                            newp = പുതിയ AstAnd{lfl, newp, പുതിയ AstConst{lfl, cleanmask}};
                        }
                        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം orp
                            = V3Const::constifyEditCpp(പുതിയ AstOr{lfl, oldvalp, newp});
                        addWordAssign(nodep, w, destp, orp);
                    }
                }
                VL_DO_DANGLING(rhsp->delete_വൃക്ഷം(), rhsp);
                VL_DO_DANGLING(destp->delete_വൃക്ഷം(), destp);
            } അല്ലെങ്കിൽ {
                didPattern(VExpandPattern::ASSIGN_TO_SEL_CONST_NARROW, nodep);
                എങ്കിൽ (destp->isQuad() && !rhsp->isQuad()) rhsp = പുതിയ AstCCast{nfl, rhsp, nodep};
                Ast_കണ്ണി_വ്യഞ്ജകം* oldvalp = destp->clone_വൃക്ഷം_Pure(സത്യം);
                fixCloneLvalue(oldvalp);
                എങ്കിൽ (!ones) oldvalp = പുതിയ AstAnd{lfl, പുതിയ AstConst{lfl, maskold}, oldvalp};

                // The bit-select can refer to bits outside the width of nodep
                // which we aren't allowed to assign to.  This is a mask of the
                // valid range of nodep which we apply to the new shifted RHS.
                V3_സംഖ്യ cleanmask{nodep, destp->widthMin()};
                cleanmask.setMask(destp->widthMin());
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം shifted = പുതിയ AstShiftL{
                    lfl, rhsp, പുതിയ AstConst{lfl, static_cast<uint32_t>(lsb)}, destp->width()};
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം cleaned
                    = പുതിയ AstAnd{lfl, shifted, പുതിയ AstConst{lfl, cleanmask}};
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം orp
                    = V3Const::constifyEditCpp(പുതിയ AstOr{lfl, oldvalp, cleaned});
                തിരുകുക_Before(nodep, പുതിയ AstAssign{nfl, destp, orp});
            }
            തിരികെ സത്യം;
        } അല്ലെങ്കിൽ {  // non-const select offset
            എങ്കിൽ (destwide && lhsp->widthConst() == 1) {
                didPattern(VExpandPattern::ASSIGN_TO_SEL_VAR_WIDE_ONE_BIT, nodep);
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം rhsp = nodep->rhsp()->unlinkFrBack();
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം destp = lhsp->fromp()->unlinkFrBack();
                Ast_കണ്ണി_വ്യഞ്ജകം* oldvalp
                    = newWordSelBit(lfl, destp->clone_വൃക്ഷം_Pure(അസത്യം), lhsp->lsbp());
                fixCloneLvalue(oldvalp);
                എങ്കിൽ (!ones) {
                    oldvalp = പുതിയ AstAnd{
                        lfl,
                        പുതിയ AstNot{
                            lfl, പുതിയ AstShiftL{lfl, പുതിയ AstConst{nfl, 1},
                                               // newSelBitBit may exceed the MSB of this variable.
                                               // That's ok as we'd just AND with a larger value,
                                               // but oldval would clip the upper bits to sanity
                                               newSelBitBit(lhsp->lsbp()), VL_EDATASIZE}},
                        oldvalp};
                }
                // Restrict the shift amount to 0-31, see bug804.
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം shiftp = പുതിയ AstAnd{nfl, lhsp->lsbp()->clone_വൃക്ഷം_Pure(സത്യം),
                                                       പുതിയ AstConst{nfl, VL_EDATASIZE - 1}};
                Ast_കണ്ണി* സ്ഥിരം newp = പുതിയ AstAssign{
                    nfl, newWordSelBit(nfl, destp, lhsp->lsbp()),
                    പുതിയ AstOr{lfl, oldvalp, പുതിയ AstShiftL{lfl, rhsp, shiftp, VL_EDATASIZE}}};
                തിരുകുക_Before(nodep, newp);
                തിരികെ സത്യം;
            } അല്ലെങ്കിൽ എങ്കിൽ (destwide) {
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
                തിരികെ അസത്യം;
            } അല്ലെങ്കിൽ {
                didPattern(VExpandPattern::ASSIGN_TO_SEL_VAR_NARROW, nodep);
                // nodep->dumpTree("-  old: ");
                Ast_കണ്ണി_വ്യഞ്ജകം* rhsp = nodep->rhsp()->unlinkFrBack();
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം destp = lhsp->fromp()->unlinkFrBack();
                Ast_കണ്ണി_വ്യഞ്ജകം* oldvalp = destp->clone_വൃക്ഷം_Pure(സത്യം);
                fixCloneLvalue(oldvalp);

                V3_സംഖ്യ maskwidth{nodep, destp->widthMin()};
                maskwidth.setMask(lhsp->widthConst());

                എങ്കിൽ (destp->isQuad() && !rhsp->isQuad()) rhsp = പുതിയ AstCCast{nfl, rhsp, nodep};
                എങ്കിൽ (!ones) {
                    oldvalp = പുതിയ AstAnd{
                        lfl,
                        പുതിയ AstNot{lfl, പുതിയ AstShiftL{lfl, പുതിയ AstConst{nfl, maskwidth},
                                                      lhsp->lsbp()->clone_വൃക്ഷം_Pure(സത്യം),
                                                      destp->width()}},
                        oldvalp};
                }
                Ast_കണ്ണി_വ്യഞ്ജകം* newp
                    = പുതിയ AstShiftL{lfl, rhsp, lhsp->lsbp()->clone_വൃക്ഷം_Pure(സത്യം), destp->width()};
                // Apply cleaning to the new value being inserted.  Mask is
                // slightly wider than necessary to avoid an AND with all ones
                // being optimized out.  No need to clean if destp is
                // quad-sized as there are no extra bits to contaminate
                എങ്കിൽ (destp->widthMin() != 64) {
                    V3_സംഖ്യ cleanmask{nodep, destp->widthMin() + 1};
                    cleanmask.setMask(destp->widthMin());
                    newp = പുതിയ AstAnd{lfl, newp, പുതിയ AstConst{lfl, cleanmask}};
                }

                തിരുകുക_Before(nodep, പുതിയ AstAssign{nfl, destp, പുതിയ AstOr{lfl, oldvalp, newp}});
                തിരികെ സത്യം;
            }
        }
    }

    ശൂന്യം visit(AstConcat* nodep) override {
        എങ്കിൽ (nodep->user1SetOnce()) തിരികെ;  // Process once
        iterateChildren(nodep);
        എങ്കിൽ (nodep->isWide()) {
            // See under ASSIGN(WIDE)
        } അല്ലെങ്കിൽ {
            // No isImpure() check - can handle side effects in below
            didPattern(VExpandPattern::CONCAT, nodep);
            File_വരി* സ്ഥിരം fl = nodep->fileline();
            Ast_കണ്ണി_വ്യഞ്ജകം* lhsp = nodep->lhsp()->unlinkFrBack();
            Ast_കണ്ണി_വ്യഞ്ജകം* rhsp = nodep->rhsp()->unlinkFrBack();
            സ്ഥിരം uint32_t rhsshift = rhsp->widthMin();
            എങ്കിൽ (nodep->isQuad() && !lhsp->isQuad()) lhsp = പുതിയ AstCCast{fl, lhsp, nodep};
            എങ്കിൽ (nodep->isQuad() && !rhsp->isQuad()) rhsp = പുതിയ AstCCast{fl, rhsp, nodep};
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം newp = പുതിയ AstOr{
                fl, പുതിയ AstShiftL{fl, lhsp, പുതിയ AstConst{fl, rhsshift}, nodep->width()}, rhsp};
            newp->dtypeFrom(nodep);  // Unsigned
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        }
    }
    തർക്കം expandWide(Ast_കണ്ണി_Assign* nodep, AstConcat* rhsp) {
        എങ്കിൽ (!doExpandWide(rhsp)) തിരികെ അസത്യം;
        didPattern(VExpandPattern::ASSIGN_CONCAT, nodep);
        File_വരി* സ്ഥിരം fl = rhsp->fileline();
        // Lhs or Rhs may be word, long, or quad.
        // newAstWordSelClone nicely abstracts the difference.
        സ്ഥിരം പൂർണ്ണസംഖ്യ rhsshift = rhsp->rhsp()->widthMin();
        // Sometimes doing the words backwards is preferable.
        // When we have x={x,foo} backwards is better, when x={foo,x} forward is better
        // However V3Subst tends to rip this up, so not worth optimizing now.
        വേണ്ടി (പൂർണ്ണസംഖ്യ w = 0; w < rhsp->width_വാക്കുകൾ(); ++w) {
            addWordAssign(nodep, w,
                          പുതിയ AstOr{fl, newWordGrabShift(fl, w, rhsp->lhsp(), rhsshift),
                                    newAstWordSelClone(rhsp->rhsp(), w)});
        }
        തിരികെ സത്യം;
    }

    ശൂന്യം visit(AstReplicate* nodep) override {
        എങ്കിൽ (nodep->user1SetOnce()) തിരികെ;  // Process once
        iterateChildren(nodep);
        എങ്കിൽ (nodep->isWide()) {
            // See under ASSIGN(WIDE)
        } അല്ലെങ്കിൽ {
            എങ്കിൽ (isImpure(nodep)) തിരികെ;
            File_വരി* സ്ഥിരം fl = nodep->fileline();
            Ast_കണ്ണി_വ്യഞ്ജകം* lhsp = nodep->srcp()->unlinkFrBack();
            Ast_കണ്ണി_വ്യഞ്ജകം* newp;
            സ്ഥിരം പൂർണ്ണസംഖ്യ lhswidth = lhsp->widthMin();
            എങ്കിൽ (lhswidth == 1) {
                didPattern(VExpandPattern::REPLICATE_ONE_BIT, nodep);
                newp = പുതിയ AstNegate{fl, lhsp};
            } അല്ലെങ്കിൽ {
                didPattern(VExpandPattern::REPLICATE, nodep);
                സ്ഥിരം AstConst* സ്ഥിരം constp = VN_AS(nodep->countp(), Const);
                UASSERT_OBJ(constp, nodep,
                            "Replication value isn't a constant.  Checked earlier!");
                സ്ഥിരം uint32_t times = constp->toUInt();
                എങ്കിൽ (nodep->isQuad() && !lhsp->isQuad()) lhsp = പുതിയ AstCCast{fl, lhsp, nodep};
                newp = lhsp->clone_വൃക്ഷം_Pure(സത്യം);
                വേണ്ടി (unsigned repnum = 1; repnum < times; repnum++) {
                    സ്ഥിരം പൂർണ്ണസംഖ്യ rhsshift = repnum * lhswidth;
                    newp = പുതിയ AstOr{
                        fl,
                        പുതിയ AstShiftL{fl, lhsp->clone_വൃക്ഷം_Pure(സത്യം),
                                      പുതിയ AstConst{fl, static_cast<uint32_t>(rhsshift)},
                                      nodep->width()},
                        newp};
                    newp->dtypeFrom(nodep);  // Unsigned
                }
                VL_DO_DANGLING(lhsp->delete_വൃക്ഷം(), lhsp);  // Never used
            }
            newp->dtypeFrom(nodep);  // Unsigned
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        }
    }
    തർക്കം expandWide(Ast_കണ്ണി_Assign* nodep, AstReplicate* rhsp) {
        എങ്കിൽ (!doExpandWide(rhsp)) തിരികെ അസത്യം;
        didPattern(VExpandPattern::ASSIGN_REPLICATE, nodep);
        File_വരി* സ്ഥിരം fl = nodep->fileline();
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം lhsp = rhsp->srcp();
        സ്ഥിരം പൂർണ്ണസംഖ്യ lhswidth = lhsp->widthMin();
        സ്ഥിരം AstConst* സ്ഥിരം constp = VN_AS(rhsp->countp(), Const);
        UASSERT_OBJ(constp, rhsp, "Replication value isn't a constant.  Checked earlier!");
        സ്ഥിരം uint32_t times = constp->toUInt();
        വേണ്ടി (പൂർണ്ണസംഖ്യ w = 0; w < rhsp->width_വാക്കുകൾ(); ++w) {
            Ast_കണ്ണി_വ്യഞ്ജകം* newp;
            എങ്കിൽ (lhswidth == 1) {
                newp = പുതിയ AstNegate{fl, lhsp->clone_വൃക്ഷം_Pure(സത്യം)};
                // Replicate always unsigned
                newp->dtypeChgWidthSigned(VL_EDATASIZE, VL_EDATASIZE, VSigning::UNSIGNED);
            } അല്ലെങ്കിൽ {
                newp = newAstWordSelClone(lhsp, w);
                File_വരി* സ്ഥിരം rfl = rhsp->fileline();
                വേണ്ടി (unsigned repnum = 1; repnum < times; repnum++) {
                    newp = പുതിയ AstOr{fl, newWordGrabShift(rfl, w, lhsp, lhswidth * repnum), newp};
                }
            }
            addWordAssign(nodep, w, newp);
        }
        തിരികെ സത്യം;
    }

    ശൂന്യം visitEqNeq(Ast_കണ്ണി_Biop* nodep) {
        എങ്കിൽ (nodep->user1SetOnce()) തിരികെ;  // Process once
        iterateChildren(nodep);
        എങ്കിൽ (nodep->lhsp()->isWide()
            && !(VN_IS(nodep->lhsp()->dtypep()->skipRefp(), StreamDType)
                 || VN_IS(nodep->rhsp()->dtypep()->skipRefp(), StreamDType))) {
            എങ്കിൽ (isImpure(nodep)) തിരികെ;
            എങ്കിൽ (!doExpandWide(nodep->lhsp())) തിരികെ;
            എങ്കിൽ (!doExpandWide(nodep->rhsp())) തിരികെ;
            didPattern(VN_IS(nodep, Neq) ? VExpandPattern::WORDIZE_NEQ
                                         : VExpandPattern::WORDIZE_EQ,
                       nodep);
            // -> (0=={or{for each_word{WORDSEL(lhs,#)^WORDSEL(rhs,#)}}}
            File_വരി* സ്ഥിരം fl = nodep->fileline();
            Ast_കണ്ണി_വ്യഞ്ജകം* newp = nullptr;
            വേണ്ടി (പൂർണ്ണസംഖ്യ w = 0; w < nodep->lhsp()->width_വാക്കുകൾ(); ++w) {
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം eqp = പുതിയ AstXor{fl, newAstWordSelClone(nodep->lhsp(), w),
                                                    newAstWordSelClone(nodep->rhsp(), w)};
                newp = newp ? പുതിയ AstOr{fl, newp, eqp} : eqp;
            }
            എങ്കിൽ (VN_IS(nodep, Neq)) {
                newp = പുതിയ AstNeq{fl, പുതിയ AstConst{fl, AstConst::SizedEData{}, 0}, newp};
            } അല്ലെങ്കിൽ {
                newp = പുതിയ AstEq{fl, പുതിയ AstConst{fl, AstConst::SizedEData{}, 0}, newp};
            }
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        }
    }
    ശൂന്യം visit(AstEq* nodep) override { visitEqNeq(nodep); }
    ശൂന്യം visit(AstNeq* nodep) override { visitEqNeq(nodep); }

    ശൂന്യം visit(AstRedOr* nodep) override {
        എങ്കിൽ (nodep->user1SetOnce()) തിരികെ;  // Process once
        iterateChildren(nodep);
        File_വരി* സ്ഥിരം fl = nodep->fileline();
        എങ്കിൽ (nodep->lhsp()->isWide()) {
            എങ്കിൽ (isImpure(nodep)) തിരികെ;
            എങ്കിൽ (!doExpandWide(nodep->lhsp())) തിരികെ;
            didPattern(VExpandPattern::WORDIZE_REDOR, nodep);
            // -> (0!={or{for each_word{WORDSEL(lhs,#)}}}
            Ast_കണ്ണി_വ്യഞ്ജകം* newp = nullptr;
            വേണ്ടി (പൂർണ്ണസംഖ്യ w = 0; w < nodep->lhsp()->width_വാക്കുകൾ(); ++w) {
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം eqp = newAstWordSelClone(nodep->lhsp(), w);
                newp = newp ? പുതിയ AstOr{fl, newp, eqp} : eqp;
            }
            newp = പുതിയ AstNeq{fl, പുതിയ AstConst{fl, AstConst::SizedEData{}, 0}, newp};
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        } അല്ലെങ്കിൽ {
            // No isImpure() check - can handle side effects in below
            didPattern(VExpandPattern::REDOR_TO_NEQ, nodep);
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം lhsp = nodep->lhsp()->unlinkFrBack();
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം newp = പുതിയ AstNeq{
                fl, പുതിയ AstConst{fl, AstConst::Widthed_മൂല്യം{}, longOrQuadWidth(nodep), 0}, lhsp};
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        }
    }
    ശൂന്യം visit(AstRedAnd* nodep) override {
        എങ്കിൽ (nodep->user1SetOnce()) തിരികെ;  // Process once
        iterateChildren(nodep);
        File_വരി* സ്ഥിരം fl = nodep->fileline();
        എങ്കിൽ (nodep->lhsp()->isWide()) {
            എങ്കിൽ (isImpure(nodep)) തിരികെ;
            എങ്കിൽ (!doExpandWide(nodep->lhsp())) തിരികെ;
            didPattern(VExpandPattern::WORDIZE_REDAND, nodep);
            // -> (0!={and{for each_word{WORDSEL(lhs,#)}}}
            Ast_കണ്ണി_വ്യഞ്ജകം* newp = nullptr;
            വേണ്ടി (പൂർണ്ണസംഖ്യ w = 0; w < nodep->lhsp()->width_വാക്കുകൾ(); ++w) {
                Ast_കണ്ണി_വ്യഞ്ജകം* eqp = newAstWordSelClone(nodep->lhsp(), w);
                എങ്കിൽ (w == nodep->lhsp()->width_വാക്കുകൾ() - 1) {
                    // Rather than doing a (slowish) ==##, we OR in the
                    // bits that aren't part of the mask
                    eqp = പുതിയ AstOr{fl, പുതിയ AstConst{fl, notWideMask(nodep->lhsp())},
                                    // Bug in cppcheck
                                    // cppcheck-suppress memleak
                                    eqp};
                }
                newp = newp ? പുതിയ AstAnd{fl, newp, eqp} : eqp;
            }
            newp = പുതിയ AstEq{fl, പുതിയ AstConst{fl, AstConst::SizedEData{}, VL_MASK_E(VL_EDATASIZE)},
                             newp};
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        } അല്ലെങ്കിൽ {
            // No isImpure() check - can handle side effects in below
            didPattern(VExpandPattern::REDAND_TO_EQ, nodep);
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം lhsp = nodep->lhsp()->unlinkFrBack();
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം newp = പുതിയ AstEq{fl, പുതിയ AstConst{fl, wordMask(lhsp)}, lhsp};
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        }
    }
    ശൂന്യം visit(AstRedXor* nodep) override {
        എങ്കിൽ (nodep->user1SetOnce()) തിരികെ;  // Process once
        iterateChildren(nodep);
        എങ്കിൽ (nodep->lhsp()->isWide()) {
            എങ്കിൽ (isImpure(nodep)) തിരികെ;
            എങ്കിൽ (!doExpandWide(nodep->lhsp())) തിരികെ;
            didPattern(VExpandPattern::WORDIZE_REDXOR, nodep);
            // -> (0!={redxor{for each_word{XOR(WORDSEL(lhs,#))}}}
            File_വരി* സ്ഥിരം fl = nodep->fileline();
            Ast_കണ്ണി_വ്യഞ്ജകം* newp = nullptr;
            വേണ്ടി (പൂർണ്ണസംഖ്യ w = 0; w < nodep->lhsp()->width_വാക്കുകൾ(); ++w) {
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം eqp = newAstWordSelClone(nodep->lhsp(), w);
                newp = newp ? പുതിയ AstXor{fl, newp, eqp} : eqp;
            }
            newp = പുതിയ AstRedXor{fl, newp};
            UINFO(8, "    Wordize REDXORnew " << newp);
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        }
        // We don't reduce non-wide XORs, as its more efficient to use a temp register,
        // which the inlined function does nicely.
    }

    ശൂന്യം visit(Ast_കണ്ണി_Stmt* nodep) override {
        എങ്കിൽ (nodep->user1SetOnce()) തിരികെ;  // Process once
        VL_RESTORER(m_stmtp);
        m_stmtp = nodep;
        iterateChildren(nodep);
    }
    ശൂന്യം visit(Ast_കണ്ണി_Assign* nodep) override {
        എങ്കിൽ (nodep->user1SetOnce()) തിരികെ;  // Process once
        എങ്കിൽ (VN_IS(nodep->dtypep()->skipRefp(), UnpackArrayDType)) {
            തിരികെ;  // Skip for UnpackArrayDType
        }

        VL_RESTORER(m_stmtp);
        m_stmtp = nodep;
        iterateChildren(nodep);
        തർക്കം did = അസത്യം;
        എങ്കിൽ (nodep->isWide()  //
            && ((VN_IS(nodep->lhsp(), VarRef) || VN_IS(nodep->lhsp(), ArraySel)))
            && !AstVar::scVarRecurse(nodep->lhsp())  // Need special function for SC
            && !AstVar::scVarRecurse(nodep->rhsp())) {
            എങ്കിൽ (AstConst* സ്ഥിരം rhsp = VN_CAST(nodep->rhsp(), Const)) {
                did = expandWide(nodep, rhsp);
            } അല്ലെങ്കിൽ എങ്കിൽ (AstVarRef* സ്ഥിരം rhsp = VN_CAST(nodep->rhsp(), VarRef)) {
                did = expandWide(nodep, rhsp);
            } അല്ലെങ്കിൽ എങ്കിൽ (AstSel* സ്ഥിരം rhsp = VN_CAST(nodep->rhsp(), Sel)) {
                did = expandWide(nodep, rhsp);
            } അല്ലെങ്കിൽ എങ്കിൽ (AstArraySel* സ്ഥിരം rhsp = VN_CAST(nodep->rhsp(), ArraySel)) {
                did = expandWide(nodep, rhsp);
            } അല്ലെങ്കിൽ എങ്കിൽ (AstConcat* സ്ഥിരം rhsp = VN_CAST(nodep->rhsp(), Concat)) {
                did = expandWide(nodep, rhsp);
            } അല്ലെങ്കിൽ എങ്കിൽ (AstExtend* സ്ഥിരം rhsp = VN_CAST(nodep->rhsp(), Extend)) {
                did = expandWide(nodep, rhsp);
            } അല്ലെങ്കിൽ എങ്കിൽ (AstReplicate* സ്ഥിരം rhsp = VN_CAST(nodep->rhsp(), Replicate)) {
                did = expandWide(nodep, rhsp);
            } അല്ലെങ്കിൽ എങ്കിൽ (AstAnd* സ്ഥിരം rhsp = VN_CAST(nodep->rhsp(), And)) {
                did = expandWide(nodep, rhsp);
            } അല്ലെങ്കിൽ എങ്കിൽ (AstOr* സ്ഥിരം rhsp = VN_CAST(nodep->rhsp(), Or)) {
                did = expandWide(nodep, rhsp);
            } അല്ലെങ്കിൽ എങ്കിൽ (AstNot* സ്ഥിരം rhsp = VN_CAST(nodep->rhsp(), Not)) {
                did = expandWide(nodep, rhsp);
            } അല്ലെങ്കിൽ എങ്കിൽ (AstXor* സ്ഥിരം rhsp = VN_CAST(nodep->rhsp(), Xor)) {
                did = expandWide(nodep, rhsp);
            } അല്ലെങ്കിൽ എങ്കിൽ (AstShiftL* സ്ഥിരം rhsp = VN_CAST(nodep->rhsp(), ShiftL)) {
                did = expandWideShift(nodep, rhsp, /* isLeftShift: */ സത്യം);
            } അല്ലെങ്കിൽ എങ്കിൽ (AstShiftR* സ്ഥിരം rhsp = VN_CAST(nodep->rhsp(), ShiftR)) {
                did = expandWideShift(nodep, rhsp, /* isLeftShift: */ അസത്യം);
            } അല്ലെങ്കിൽ എങ്കിൽ (AstCond* സ്ഥിരം rhsp = VN_CAST(nodep->rhsp(), Cond)) {
                did = expandWide(nodep, rhsp);
            }
        } അല്ലെങ്കിൽ എങ്കിൽ (AstSel* സ്ഥിരം lhsp = VN_CAST(nodep->lhsp(), Sel)) {
            did = expandLhs(nodep, lhsp);
        }
        // Cleanup common code
        എങ്കിൽ (did) VL_DO_DANGLING(nodep->unlinkFrBack()->delete_വൃക്ഷം(), nodep);
    }

    //--------------------
    ശൂന്യം visit(AstVar*) override {}  // Don't hit varrefs under vars
    ശൂന്യം visit(Ast_കണ്ണി* nodep) override { iterateChildren(nodep); }

പൊതു:
    // CONSTRUCTORS
    explicit ExpandVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~ExpandVisitor() override {
        V3Stats::addStat("Optimizations, Expand, expanded wides", m_statWides);
        V3Stats::addStat("Optimizations, Expand, expanded wide words", m_stat_Wide_വാക്കുകൾ);
        V3Stats::addStat("Optimizations, Expand, not expanded impure", m_statImpure);
        V3Stats::addStat("Optimizations, Expand, not expanded limit", m_statWideLimited);
        V3Stats::addStat("Optimizations, Expand, temporaries created", m_statTemps);
        സ്ഥിരം auto emitStat = [ഇത്](VExpandPattern pattern) {
            std::string str{pattern.ascii()};
            std::transform(str.begin(), str.end(), str.begin(), [](unsigned അക്ഷരം c) {  //
                തിരികെ c == '_' ? ' ' : std::tolower(c);
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

ശൂന്യം V3Expand::expandAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        സ്ഥിരം ExpandOkVisitor okVisitor{nodep};
        ExpandVisitor{nodep};
    }  // Destruct before checking
    V3Global::dump_Check_Global_വൃക്ഷം("expand", 0, dump_വൃക്ഷം_Either_Level() >= 3);
}
