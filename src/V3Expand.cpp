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

#अन्तर्भाव "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#अन्तर्भाव "V3Expand.h"

#अन्तर्भाव "V3Const.h"
#अन्तर्भाव "V3Stats.h"

#अन्तर्भाव <algorithm>
#अन्तर्भाव <array>

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

वर्ग VExpandPattern final {
सार्वजनिक:
    गणना en : uint8_t {
#define EXPAND_PATTERN_ID(id) id,
        FOR_EACH_EXPAND_PATTERN(EXPAND_PATTERN_ID)
#undef EXPAND_PATTERN_ID
            _ENUM_END
    };
    गणना en m_e;
    अचर वर्ण* ascii() अचर {
        स्थिर अचर वर्ण* अचर names[] = {
#define EXPAND_PATTERN_NAME(id) #id,
            FOR_EACH_EXPAND_PATTERN(EXPAND_PATTERN_NAME)
#undef EXPAND_PATTERN_NAME
                "_ENUM_END"  //
        };
        फर्काऊ names[m_e];
    }

    // cppcheck-suppress noExplicitConstructor
    VExpandPattern(en _e)
        : m_e{_e} {}
    चालक en() अचर { फर्काऊ m_e; }
};

//######################################################################
// Find nodes with side effects, to mark as non-expandable

वर्ग ExpandOkVisitor final : सार्वजनिक VNVisitorConst {
    // NODE STATE
    //  AstNode::user2()        -> bool.  Is pure (along with all children)
    अचर VNUser2InUse m_inuser2;

    // STATE - for current visit position (use VL_RESTORER)
    // Tracks similar to AstNode::isTreePureRecurse(), but avoid O(n^2)
    // False = pure, as nodes that ExpandVisitor inserts preserve pureness
    तार्किक m_isImpure = सत्य;  // Currently pure

    शून्य visit(Ast_गाँठो* nodep) override {
        तार्किक selfImpure = !nodep->isPure();
        {
            VL_RESTORER(m_isImpure);
            m_isImpure = असत्य;
            iterateChildrenConst(nodep);
            selfImpure |= m_isImpure;
            nodep->user2(selfImpure);
        }
        m_isImpure |= selfImpure;
    }

सार्वजनिक:
    // CONSTRUCTORS
    explicit ExpandOkVisitor(AstNetlist* nodep) { iterateConst(nodep); }
    ~ExpandOkVisitor() = स्वभाव;
};

//######################################################################
// Expand state, as a visitor of each AstNode

वर्ग ExpandVisitor final : सार्वजनिक VNVisitor {
    // NODE STATE
    //  AstNode::user1()        -> bool.  Processed
    //  AstNode::user2()        -> See ExpandOkVisitor
    अचर VNUser1InUse m_inuser1;

    // STATE - for current visit position (use VL_RESTORER)
    Ast_गाँठो* m_stmtp = nullptr;  // Current statement

    // STATE - across all visitors
    AstCFunc* m_funcp = nullptr;  // Current function
    VDouble0 m_statImpure;  // Statistic tracking
    VDouble0 m_statWides;  // Statistic tracking
    VDouble0 m_stat_Wide_शब्दहरू;  // Statistic tracking
    VDouble0 m_statWideLimited;  // Statistic tracking
    VDouble0 m_statTemps;  // Statistic tracking
    // Number of applications of each expansion pattern for statistics
    std::array<VDouble0, VExpandPattern::_ENUM_END> m_statPatterns;

    // STATE - for current function
    माप_प्रकार m_nTmps = 0;  // Sequence numbers for temopraries

    // METHODS
    // Note the application of the given expansion pattern to 'nodep'
    शून्य didPattern(VExpandPattern pattern, Ast_गाँठो* nodep) {
        UINFO(8, "    " << pattern.ascii() << " " << nodep);
        ++m_statPatterns[pattern];
    }

    // Use state that ExpandOkVisitor calculated
    स्थिर तार्किक isImpure(Ast_गाँठो* nodep) {
        अचर तार्किक impure = nodep->user2();
        यदि (impure) UINFO(9, "      impure " << nodep);
        फर्काऊ impure;
    }

    तार्किक doExpandWide(Ast_गाँठो* nodep) {
        यदि (isImpure(nodep)) {
            ++m_statImpure;
            फर्काऊ असत्य;
        }
        यदि (nodep->width_शब्दहरू() <= v3Global.opt.expandLimit()) {
            ++m_statWides;
            m_stat_Wide_शब्दहरू += nodep->width_शब्दहरू();
            फर्काऊ सत्य;
        } अन्यथा {
            ++m_statWideLimited;
            फर्काऊ असत्य;
        }
    }

    स्थिर पूर्णाङ्क longOrQuadWidth(Ast_गाँठो* nodep) {
        फर्काऊ (nodep->width() + (VL_EDATASIZE - 1)) & ~(VL_EDATASIZE - 1);
    }
    स्थिर V3_सङ्ख्या notWideMask(Ast_गाँठो* nodep) {
        फर्काऊ V3_सङ्ख्या{nodep, VL_EDATASIZE, ~VL_MASK_E(nodep->widthMin())};
    }
    स्थिर V3_सङ्ख्या wordMask(Ast_गाँठो* nodep) {
        यदि (nodep->isWide()) {
            फर्काऊ V3_सङ्ख्या{nodep, VL_EDATASIZE, VL_MASK_E(nodep->widthMin())};
        } अन्यथा {
            V3_सङ्ख्या mask{nodep, longOrQuadWidth(nodep)};
            mask.setMask(nodep->widthMin());
            फर्काऊ mask;
        }
    }

    स्थिर शून्य घुसाउ_Before(Ast_गाँठो* placep, Ast_गाँठो* newp) {
        newp->user1(1);  // Already processed, don't need to re-iterate
        placep->addHereThisAsNext(newp);
    }
    स्थिर शून्य replaceWithDelete(Ast_गाँठो* nodep, Ast_गाँठो* newp) {
        newp->user1(1);  // Already processed, don't need to re-iterate
        यदि (newp->width() != nodep->width()) {
            UASSERT_OBJ(newp->widthMin() == nodep->widthMin(), nodep,
                        "Replacement width mismatch");
            newp->dtypeChgWidth(nodep->width(), nodep->widthMin());
        }
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_रूख(), nodep);
    }
    स्थिर Ast_गाँठो* newWordAssign(Ast_गाँठो_Assign* placep, पूर्णाङ्क word, Ast_गाँठो_अभिव्यक्ति* lhsp,
                                  Ast_गाँठो_अभिव्यक्ति* rhsp) {
        File_पङ्क्ति* अचर fl = placep->fileline();
        फर्काऊ नूतन AstAssign{fl,
                             नूतन AstWordSel{fl, lhsp->clone_रूख_Pure(सत्य),
                                            नूतन AstConst{fl, static_cast<uint32_t>(word)}},
                             rhsp};
    }
    स्थिर शून्य addWordAssign(Ast_गाँठो_Assign* placep, पूर्णाङ्क word, Ast_गाँठो_अभिव्यक्ति* lhsp,
                              Ast_गाँठो_अभिव्यक्ति* rhsp) {
        घुसाउ_Before(placep, newWordAssign(placep, word, lhsp, rhsp));
    }
    स्थिर शून्य addWordAssign(Ast_गाँठो_Assign* placep, पूर्णाङ्क word, Ast_गाँठो_अभिव्यक्ति* rhsp) {
        addWordAssign(placep, word, placep->lhsp(), rhsp);
    }

    AstVar* addLocalTmp(Ast_गाँठो* placep, अचर वर्ण* namep, Ast_गाँठो_अभिव्यक्ति* valuep) {
        अचर std::string name = "__V"s + namep + "_"s + std::to_string(m_nTmps);
        File_पङ्क्ति* अचर flp = placep->fileline();
        AstVar* अचर tmpp = नूतन AstVar{flp, VVarType::STMTTEMP, name, valuep->dtypep()};
        tmpp->funcLocal(सत्य);
        tmpp->isInternal(सत्य);
        tmpp->noReset(सत्य);
        tmpp->substConstOnly(सत्य);
        m_funcp->addVarsp(tmpp);
        घुसाउ_Before(placep, नूतन AstAssign{flp, नूतन AstVarRef{flp, tmpp, VAccess::WRITE}, valuep});
        ++m_statTemps;
        फर्काऊ tmpp;
    }

    स्थिर शून्य fixCloneLvalue(Ast_गाँठो* nodep) {
        // In AstSel transforms, we call clone() on VarRefs that were lvalues,
        // but are now being used on the RHS of the assignment
        यदि (VN_IS(nodep, VarRef)) VN_AS(nodep, VarRef)->access(VAccess::READ);
        // Iterate
        यदि (Ast_गाँठो* अचर refp = nodep->op1p()) fixCloneLvalue(refp);
        यदि (Ast_गाँठो* अचर refp = nodep->op2p()) fixCloneLvalue(refp);
        यदि (Ast_गाँठो* अचर refp = nodep->op3p()) fixCloneLvalue(refp);
        यदि (Ast_गाँठो* अचर refp = nodep->op4p()) fixCloneLvalue(refp);
    }

    स्थिर Ast_गाँठो_अभिव्यक्ति* newAstWordSelClone(Ast_गाँठो_अभिव्यक्ति* nodep, पूर्णाङ्क word) {
        // Get the specified word number from a wide array
        // Or, if it's a long/quad, do appropriate conversion to wide
        // Concat may pass negative word numbers, that means it wants a zero
        File_पङ्क्ति* अचर fl = nodep->fileline();
        यदि (nodep->isWide() && word >= 0 && word < nodep->width_शब्दहरू()) {
            फर्काऊ नूतन AstWordSel{fl, nodep->clone_रूख_Pure(सत्य),
                                  नूतन AstConst{fl, static_cast<uint32_t>(word)}};
        } अन्यथा यदि (nodep->isQuad() && word == 0) {
            Ast_गाँठो_अभिव्यक्ति* अचर quadfromp = nodep->clone_रूख_Pure(सत्य);
            quadfromp->dtypeSetBitUnsized(VL_QUADSIZE, quadfromp->widthMin(), VSigning::UNSIGNED);
            फर्काऊ नूतन AstCCast{fl, quadfromp, VL_EDATASIZE};
        } अन्यथा यदि (nodep->isQuad() && word == 1) {
            Ast_गाँठो_अभिव्यक्ति* अचर quadfromp = nodep->clone_रूख_Pure(सत्य);
            quadfromp->dtypeSetBitUnsized(VL_QUADSIZE, quadfromp->widthMin(), VSigning::UNSIGNED);
            फर्काऊ नूतन AstCCast{
                fl, नूतन AstShiftR{fl, quadfromp, नूतन AstConst{fl, VL_EDATASIZE}, VL_EDATASIZE},
                VL_EDATASIZE};
        } अन्यथा यदि (!nodep->isWide() && !nodep->isQuad() && word == 0) {
            फर्काऊ nodep->clone_रूख_Pure(सत्य);
        } अन्यथा {  // Out of bounds
            फर्काऊ नूतन AstConst{fl, 0};
        }
    }

    स्थिर Ast_गाँठो_अभिव्यक्ति* newWordGrabShift(File_पङ्क्ति* fl, पूर्णाङ्क word, Ast_गाँठो_अभिव्यक्ति* lhsp, पूर्णाङ्क shift) {
        // Extract the expression to grab the value for the specified word, if it's the shift
        // of shift bits from lhsp
        // Negative word numbers requested for lhs when it's "before" what we want.
        // We get a 0 then.
        अचर पूर्णाङ्क othword = word - shift / VL_EDATASIZE;
        Ast_गाँठो_अभिव्यक्ति* अचर lop = newAstWordSelClone(lhsp, othword);
        अचर uint32_t loShift = VL_BITBIT_E(shift);
        यदि (!loShift) फर्काऊ lop;
        Ast_गाँठो_अभिव्यक्ति* अचर hip = newAstWordSelClone(lhsp, othword - 1);
        अचर uint32_t hiShift = VL_EDATASIZE - loShift;  // Complement offset
        Ast_गाँठो_अभिव्यक्ति* अचर newp
            = नूतन AstOr{fl,  //
                        नूतन AstShiftR{fl, hip, नूतन AstConst{fl, hiShift}, VL_EDATASIZE},
                        नूतन AstShiftL{fl, lop, नूतन AstConst{fl, loShift}, VL_EDATASIZE}};
        फर्काऊ V3Const::constifyEditCpp(newp);
    }

    // Return expression indexing the word that contains 'lsbp' + the given word offset
    स्थिर Ast_गाँठो_अभिव्यक्ति* newWordIndex(Ast_गाँठो_अभिव्यक्ति* lsbp, uint32_t wordOffset = 0) {
        // This is indexing a WordSel, so a 32 bit constants are fine
        File_पङ्क्ति* अचर flp = lsbp->fileline();
        यदि (अचर AstConst* constp = VN_CAST(lsbp, Const)) {
            फर्काऊ नूतन AstConst{flp, wordOffset + VL_BITWORD_E(constp->toUInt())};
        }

        यदि (lsbp->backp()) lsbp = lsbp->clone_रूख_Pure(असत्य);
        Ast_गाँठो_अभिव्यक्ति* अचर wwl2p = नूतन AstConst{flp, VL_EDATASIZE_LOG2};  // Word width log 2
        Ast_गाँठो_अभिव्यक्ति* indexp = नूतन AstShiftR{flp, lsbp, wwl2p, VL_EDATASIZE};
        यदि (wordOffset) indexp = नूतन AstAdd{flp, नूतन AstConst{flp, wordOffset}, indexp};
        फर्काऊ indexp;
    }

    // Return word of fromp that contains word indexp + the given word offset.
    स्थिर Ast_गाँठो_अभिव्यक्ति* newWordSelWord(File_पङ्क्ति* flp, Ast_गाँठो_अभिव्यक्ति* fromp, Ast_गाँठो_अभिव्यक्ति* indexp,
                                       uint32_t wordOffset = 0) {
        UASSERT_OBJ(fromp->isWide(), fromp, "Only need AstWordSel on wide from's");

        यदि (अचर AstConst* अचर constp = VN_CAST(indexp, Const)) {
            indexp = nullptr;
            wordOffset += constp->toUInt();
        }

        // e.g. "logic [95:0] var[0]; logic [0] sel; out = var[sel];"
        // Squash before C++ to avoid getting a C++ compiler warning
        // (even though code would be unreachable as presumably a
        // AstCond is protecting above this node.
        यदि (wordOffset >= static_cast<uint32_t>(fromp->width_शब्दहरू())) {
            फर्काऊ नूतन AstConst{flp, AstConst::SizedEData{}, 0};
        }

        यदि (fromp->backp()) fromp = fromp->clone_रूख_Pure(असत्य);

        // If indexp was constant, just use it
        यदि (!indexp) फर्काऊ नूतन AstWordSel{flp, fromp, नूतन AstConst{flp, wordOffset}};

        // Otherwise compute at runtime
        यदि (indexp->backp()) indexp = indexp->clone_रूख_Pure(असत्य);
        यदि (wordOffset) indexp = नूतन AstAdd{flp, नूतन AstConst{flp, wordOffset}, indexp};
        फर्काऊ नूतन AstWordSel{flp, fromp, indexp};
    }

    // Return word of fromp that contains bit lsbp + the given word offset.
    स्थिर Ast_गाँठो_अभिव्यक्ति* newWordSelBit(File_पङ्क्ति* flp, Ast_गाँठो_अभिव्यक्ति* fromp, Ast_गाँठो_अभिव्यक्ति* lsbp,
                                      uint32_t wordOffset = 0) {
        Ast_गाँठो_अभिव्यक्ति* अचर indexp = newWordIndex(lsbp, wordOffset);
        Ast_गाँठो_अभिव्यक्ति* अचर wordSelp = newWordSelWord(flp, fromp, indexp);
        यदि (!indexp->backp()) VL_DO_DANGLING(indexp->delete_रूख(), indexp);
        फर्काऊ wordSelp;
    }

    स्थिर Ast_गाँठो_अभिव्यक्ति* newSelBitBit(Ast_गाँठो_अभिव्यक्ति* lsbp) {
        // Return equation to get the VL_BITBIT of a constant or non-constant
        File_पङ्क्ति* अचर fl = lsbp->fileline();
        यदि (VN_IS(lsbp, Const)) {
            फर्काऊ नूतन AstConst{fl, VL_BITBIT_E(VN_AS(lsbp, Const)->toUInt())};
        } अन्यथा {
            फर्काऊ नूतन AstAnd{fl, नूतन AstConst{fl, VL_EDATASIZE - 1}, lsbp->clone_रूख_Pure(सत्य)};
        }
    }

    //====================

    तार्किक expandWide(Ast_गाँठो_Assign* nodep, AstConst* rhsp) {
        यदि (!doExpandWide(nodep)) फर्काऊ असत्य;
        didPattern(VExpandPattern::ASSIGN_CONST, nodep);
        // -> {for each_word{ ASSIGN(WORDSEL(wide,#),WORDSEL(CONST,#))}}
        यदि (rhsp->अङ्क().isFourState()) {
            rhsp->v3warn(E_UNSUPPORTED,  // LCOV_EXCL_LINE  // impossible?
                         "Unsupported: 4-state numbers in this context");
        }
        File_पङ्क्ति* अचर fl = nodep->fileline();
        लागि (पूर्णाङ्क w = 0; w < nodep->width_शब्दहरू(); ++w) {
            addWordAssign(nodep, w,
                          नूतन AstConst{fl, AstConst::SizedEData{}, rhsp->अङ्क().edataWord(w)});
        }
        फर्काऊ सत्य;
    }
    //-------- Uniops
    तार्किक expandWide(Ast_गाँठो_Assign* nodep, AstVarRef* rhsp) {
        // Special case: do not expand assignment of constant pool variables.
        // V3Subst undestands these directly.
        यदि (rhsp->varp()->constPoolEntry()) फर्काऊ असत्य;
        यदि (!doExpandWide(nodep)) फर्काऊ असत्य;
        didPattern(VExpandPattern::ASSIGN_VARREF, nodep);
        लागि (पूर्णाङ्क w = 0; w < nodep->width_शब्दहरू(); ++w) {
            addWordAssign(nodep, w, newAstWordSelClone(rhsp, w));
        }
        फर्काऊ सत्य;
    }
    तार्किक expandWide(Ast_गाँठो_Assign* nodep, AstArraySel* rhsp) {
        UASSERT_OBJ(!VN_IS(nodep->dtypep()->skipRefp(), UnpackArrayDType), nodep,
                    "ArraySel with unpacked arrays should have been removed in V3Slice");
        यदि (!doExpandWide(nodep)) फर्काऊ असत्य;
        didPattern(VExpandPattern::ASSIGN_ARRAYSEL, nodep);
        लागि (पूर्णाङ्क w = 0; w < nodep->width_शब्दहरू(); ++w) {
            addWordAssign(nodep, w, newAstWordSelClone(rhsp, w));
        }
        फर्काऊ सत्य;
    }
    तार्किक expandWide(Ast_गाँठो_Assign* nodep, AstNot* rhsp) {
        // -> {for each_word{ ASSIGN(WORDSEL(wide,#),NOT(WORDSEL(lhs,#))) }}
        यदि (!doExpandWide(nodep)) फर्काऊ असत्य;
        didPattern(VExpandPattern::ASSIGN_NOT, nodep);
        File_पङ्क्ति* अचर fl = rhsp->fileline();
        लागि (पूर्णाङ्क w = 0; w < nodep->width_शब्दहरू(); ++w) {
            addWordAssign(nodep, w, नूतन AstNot{fl, newAstWordSelClone(rhsp->lhsp(), w)});
        }
        फर्काऊ सत्य;
    }
    //-------- Biops
    तार्किक expandWide(Ast_गाँठो_Assign* nodep, AstAnd* rhsp) {
        यदि (!doExpandWide(nodep)) फर्काऊ असत्य;
        didPattern(VExpandPattern::ASSIGN_AND, nodep);
        File_पङ्क्ति* अचर fl = nodep->fileline();
        लागि (पूर्णाङ्क w = 0; w < nodep->width_शब्दहरू(); ++w) {
            addWordAssign(nodep, w,
                          नूतन AstAnd{fl, newAstWordSelClone(rhsp->lhsp(), w),
                                     newAstWordSelClone(rhsp->rhsp(), w)});
        }
        फर्काऊ सत्य;
    }
    तार्किक expandWide(Ast_गाँठो_Assign* nodep, AstOr* rhsp) {
        यदि (!doExpandWide(nodep)) फर्काऊ असत्य;
        didPattern(VExpandPattern::ASSIGN_OR, nodep);
        File_पङ्क्ति* अचर fl = nodep->fileline();
        लागि (पूर्णाङ्क w = 0; w < nodep->width_शब्दहरू(); ++w) {
            addWordAssign(nodep, w,
                          नूतन AstOr{fl, newAstWordSelClone(rhsp->lhsp(), w),
                                    newAstWordSelClone(rhsp->rhsp(), w)});
        }
        फर्काऊ सत्य;
    }
    तार्किक expandWide(Ast_गाँठो_Assign* nodep, AstXor* rhsp) {
        यदि (!doExpandWide(nodep)) फर्काऊ असत्य;
        didPattern(VExpandPattern::ASSIGN_XOR, nodep);
        File_पङ्क्ति* अचर fl = nodep->fileline();
        लागि (पूर्णाङ्क w = 0; w < nodep->width_शब्दहरू(); ++w) {
            addWordAssign(nodep, w,
                          नूतन AstXor{fl, newAstWordSelClone(rhsp->lhsp(), w),
                                     newAstWordSelClone(rhsp->rhsp(), w)});
        }
        फर्काऊ सत्य;
    }
    तार्किक expandWideShift(Ast_गाँठो_Assign* nodep, Ast_गाँठो_Biop* rhsp, तार्किक is_बायाँ_Shift) {
        यदि (!doExpandWide(nodep)) फर्काऊ असत्य;

        // Simplify the shift amount, in case it becomes a constant
        V3Const::constifyEditCpp(rhsp->rhsp());

        // If it's a constant shift by whole words, expand it so V3Subst can substitute it
        यदि (अचर AstConst* अचर rhsConstp = VN_CAST(rhsp->rhsp(), Const)) {
            अचर uint32_t shiftBits = rhsConstp->toUInt();
            यदि (VL_BITBIT_E(shiftBits) == 0) {
                अचर पूर्णाङ्क width_शब्दहरू = nodep->width_शब्दहरू();
                अचर पूर्णाङ्क shift_शब्दहरू = std::min<पूर्णाङ्क>(VL_BITWORD_E(shiftBits), width_शब्दहरू);
                File_पङ्क्ति* अचर flp = rhsp->fileline();
                यदि (is_बायाँ_Shift) {
                    didPattern(VExpandPattern::ASSIGN_SHIFTL, nodep);
                    // Low words of the result are zero
                    लागि (पूर्णाङ्क w = 0; w < shift_शब्दहरू; ++w) {
                        addWordAssign(nodep, w, नूतन AstConst{flp, AstConst::SizedEData{}, 0});
                    }
                    // High words of the result are copied from higher words of the source
                    लागि (पूर्णाङ्क w = shift_शब्दहरू; w < width_शब्दहरू; ++w) {
                        addWordAssign(nodep, w, newAstWordSelClone(rhsp->lhsp(), w - shift_शब्दहरू));
                    }
                } अन्यथा {
                    didPattern(VExpandPattern::ASSIGN_SHIFTR, nodep);
                    // Low words of the result are copied from higher words of the source
                    लागि (पूर्णाङ्क w = 0; w < width_शब्दहरू - shift_शब्दहरू; ++w) {
                        addWordAssign(nodep, w, newAstWordSelClone(rhsp->lhsp(), w + shift_शब्दहरू));
                    }
                    // High words of the result are zero
                    लागि (पूर्णाङ्क w = width_शब्दहरू - shift_शब्दहरू; w < width_शब्दहरू; ++w) {
                        addWordAssign(nodep, w, नूतन AstConst{flp, AstConst::SizedEData{}, 0});
                    }
                }
                फर्काऊ सत्य;
            }
        }

        फर्काऊ असत्य;
    }

    //-------- Triops
    तार्किक expandWide(Ast_गाँठो_Assign* nodep, AstCond* rhsp) {
        यदि (!doExpandWide(nodep)) फर्काऊ असत्य;
        didPattern(VExpandPattern::ASSIGN_COND, nodep);
        File_पङ्क्ति* अचर fl = nodep->fileline();
        लागि (पूर्णाङ्क w = 0; w < nodep->width_शब्दहरू(); ++w) {
            addWordAssign(nodep, w,
                          नूतन AstCond{fl, rhsp->condp()->clone_रूख_Pure(सत्य),
                                      newAstWordSelClone(rhsp->thenp(), w),
                                      newAstWordSelClone(rhsp->elsep(), w)});
        }
        फर्काऊ सत्य;
    }

    // VISITORS
    शून्य visit(AstNetlist* nodep) override { iterateAndNextNull(nodep->modulesp()); }

    शून्य visit(AstCFunc* nodep) override {
        VL_RESTORER(m_funcp);
        VL_RESTORER(m_nTmps);
        m_funcp = nodep;
        m_nTmps = 0;
        अचर VDouble0 statWidesBefore = m_statWides;
        iterateChildren(nodep);

        // Constant fold here if anything was expanded, as Ast size can likely be reduced
        यदि (v3Global.opt.fConstEager() && m_statWides != statWidesBefore) {
            Ast_गाँठो* अचर editedp = V3Const::constifyEditCpp(nodep);
            UASSERT_OBJ(editedp == nodep, editedp, "Should not have replaced CFunc");
        }
    }

    शून्य visit(AstExtend* nodep) override {
        यदि (nodep->user1SetOnce()) फर्काऊ;  // Process once
        iterateChildren(nodep);
        यदि (nodep->isWide()) {
            // See under ASSIGN(EXTEND)
        } अन्यथा {
            यदि (isImpure(nodep)) फर्काऊ;
            Ast_गाँठो_अभिव्यक्ति* अचर lhsp = nodep->lhsp()->unlinkFrBack();
            Ast_गाँठो_अभिव्यक्ति* newp = lhsp;
            यदि (nodep->isQuad()) {
                यदि (lhsp->isQuad()) {
                    lhsp->dtypeFrom(nodep);  // Just mark it, else nop
                } अन्यथा यदि (lhsp->isWide()) {
                    nodep->v3fatalSrc("extending larger thing into smaller?");
                } अन्यथा {
                    newp = नूतन AstCCast{nodep->fileline(), lhsp, nodep};
                }
            } अन्यथा {  // Long
                UASSERT_OBJ(!(lhsp->isQuad() || lhsp->isWide()), nodep,
                            "extending larger thing into smaller?");
                lhsp->dtypeFrom(nodep);  // Just mark it, else nop
            }
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        }
    }
    तार्किक expandWide(Ast_गाँठो_Assign* nodep, AstExtend* rhsp) {
        यदि (!doExpandWide(nodep)) फर्काऊ असत्य;
        didPattern(VExpandPattern::ASSIGN_EXTEND, nodep);
        Ast_गाँठो_अभिव्यक्ति* अचर rlhsp = rhsp->lhsp();
        लागि (पूर्णाङ्क w = 0; w < rlhsp->width_शब्दहरू(); ++w) {
            addWordAssign(nodep, w, newAstWordSelClone(rlhsp, w));
        }
        लागि (पूर्णाङ्क w = rlhsp->width_शब्दहरू(); w < nodep->width_शब्दहरू(); ++w) {
            addWordAssign(nodep, w, नूतन AstConst{nodep->fileline(), AstConst::SizedEData{}, 0});
        }
        फर्काऊ सत्य;
    }

    शून्य visit(AstSel* nodep) override {
        यदि (nodep->user1SetOnce()) फर्काऊ;  // Process once
        iterateChildren(nodep);

        अचर uint32_t width = static_cast<uint32_t>(nodep->widthConst());
        UASSERT_OBJ(nodep->widthMin() == static_cast<पूर्णाङ्क>(width), nodep, "Width mismatch");

        // Skip if Sel is an LHS assignment select
        यदि (Ast_गाँठो_Assign* अचर assignp = VN_CAST(nodep->backp(), गाँठो_Assign)) {
            यदि (nodep == assignp->lhsp()) फर्काऊ;
        }

        // Skip if wide: See under ASSIGN(WIDE)
        यदि (nodep->isWide()) फर्काऊ;

        // Skip Sel from stream or queue
        यदि (VN_IS(nodep->fromp()->dtypep(), StreamDType)) फर्काऊ;
        यदि (VN_IS(nodep->fromp()->dtypep(), QueueDType)) फर्काऊ;

        // Result must be non-wide after skipping all the above
        UASSERT_OBJ(width <= 64, nodep, "Inconsistent result width");

        यदि (nodep->fromp()->isWide()) {  // Long/Quad from Wide
            यदि (isImpure(nodep)) फर्काऊ;
            didPattern(VExpandPattern::SEL_FROM_WIDE, nodep);
            File_पङ्क्ति* अचर nfl = nodep->fileline();
            File_पङ्क्ति* अचर lfl = nodep->lsbp()->fileline();
            File_पङ्क्ति* अचर ffl = nodep->fromp()->fileline();
            Ast_गाँठो_अभिव्यक्ति* अचर fromp = nodep->fromp();

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
            अचर स्वतः tryFold = [&](Ast_गाँठो_अभिव्यक्ति* exprp) -> Ast_गाँठो_अभिव्यक्ति* {
                // Size limit for inlining expressions (any arbitrary value results in
                // a bounded expansion. Picked a value to allow common simple forms.)
                स्थिर constexpr पूर्णाङ्क EXPAND_SEL_LSB_LIMIT = 8;
                // Simplify
                exprp = V3Const::constifyEditCpp(exprp);
                // Accept if not larger than the limit
                यदि (!exprp->isLargerThan(EXPAND_SEL_LSB_LIMIT)) फर्काऊ exprp;
                // Delete if rejected
                VL_DO_DANGLING(exprp->delete_रूख(), exprp);
                फर्काऊ nullptr;
            };

            // Return the index of the word holding the bit 'offset' bits above the LSB
            अचर स्वतः wordIdx = [&](uint32_t offset) -> Ast_गाँठो_अभिव्यक्ति* {
                Ast_गाँठो_अभिव्यक्ति* अचर msbp = नूतन AstAdd{lfl, नूतन AstConst{lfl, offset},
                                                     nodep->lsbp()->clone_रूख_Pure(असत्य)};
                Ast_गाँठो_अभिव्यक्ति* अचर idxp = newWordIndex(msbp);
                यदि (!msbp->backp()) VL_DO_DANGLING(msbp->delete_रूख(), msbp);
                फर्काऊ idxp;
            };

            // Bit index of the selected LSB within its word
            Ast_गाँठो_अभिव्यक्ति* lBitp = tryFold(newSelBitBit(nodep->lsbp()));
            अचर तार्किक aligned = lBitp && lBitp->isZero();

            // Which of the other 2 words the expansion might need
            अचर तार्किक mNeeded = width > 1 && !aligned;
            अचर तार्किक hNeeded = width > VL_EDATASIZE;
            UASSERT_OBJ(!hNeeded || nodep->isQuad(), nodep, "Width mismatch");

            // Word indices
            अचर uint32_t mMsbOffset = std::min<uint32_t>(width, VL_EDATASIZE) - 1;
            अचर uint32_t hMsbOffset = width - 1;
            Ast_गाँठो_अभिव्यक्ति* lIdxp = tryFold(newWordIndex(nodep->lsbp()));
            Ast_गाँठो_अभिव्यक्ति* mIdxp = mNeeded ? tryFold(wordIdx(mMsbOffset)) : nullptr;
            Ast_गाँठो_अभिव्यक्ति* hIdxp = hNeeded ? tryFold(wordIdx(hMsbOffset)) : nullptr;

            // If the LSB expression is needed multiple times, evaluate it into a temporary
            अचर पूर्णाङ्क nCopies = !lBitp + !lIdxp + (mNeeded && !mIdxp) + (hNeeded && !hIdxp);
            यदि (nCopies > 1 && m_funcp && m_stmtp && !isImpure(m_stmtp)) {
                ++m_nTmps;  // Use fresh set of temporaries
                Ast_गाँठो_अभिव्यक्ति* अचर lsbp = nodep->lsbp()->unlinkFrBack();
                AstVar* अचर tmpp = addLocalTmp(m_stmtp, "ExpandSel_Lsb", lsbp);
                nodep->lsbp(नूतन AstVarRef{lfl, tmpp, VAccess::READ});
            }

            // Rebuild remaining expressions derived from LSB if needed, now using the temporary
            यदि (!lBitp) lBitp = newSelBitBit(nodep->lsbp());
            यदि (!lIdxp) lIdxp = newWordIndex(nodep->lsbp());
            यदि (!mIdxp) mIdxp = mNeeded ? wordIdx(mMsbOffset) : nullptr;
            यदि (!hIdxp) hIdxp = hNeeded ? wordIdx(hMsbOffset) : nullptr;

            // Return word 'idxp' of 'fromp', without consuming 'idxp'
            अचर स्वतः wordSel = [&](Ast_गाँठो_अभिव्यक्ति* idxp) -> Ast_गाँठो_अभिव्यक्ति* {
                Ast_गाँठो_अभिव्यक्ति* अचर clonep = idxp->clone_रूख_Pure(असत्य);
                Ast_गाँठो_अभिव्यक्ति* अचर wordp = newWordSelWord(ffl, fromp, clonep);
                यदि (!clonep->backp()) VL_DO_DANGLING(clonep->delete_रूख(), clonep);
                फर्काऊ wordp;
            };

            // Construct term containing the bits of the low word - always needed
            Ast_गाँठो_अभिव्यक्ति* अचर lTermp = [&]() -> Ast_गाँठो_अभिव्यक्ति* {
                Ast_गाँठो_अभिव्यक्ति* lWordp = wordSel(lIdxp);
                यदि (nodep->isQuad()) lWordp = नूतन AstCCast{nfl, lWordp, nodep};
                फर्काऊ नूतन AstShiftR{nfl, lWordp, lBitp->clone_रूख_Pure(असत्य), nodep->width()};
            }();

            // Construct term containing the bits of the middle word - if needed
            Ast_गाँठो_अभिव्यक्ति* अचर mTermp = [&]() -> Ast_गाँठो_अभिव्यक्ति* {
                यदि (!mNeeded) फर्काऊ nullptr;

                Ast_गाँठो_अभिव्यक्ति* mWordp = wordSel(mIdxp);
                यदि (nodep->isQuad()) mWordp = नूतन AstCCast{nfl, mWordp, nodep};
                Ast_गाँठो_अभिव्यक्ति* अचर mShiftp = नूतन AstSub{lfl, नूतन AstConst{lfl, VL_EDATASIZE},
                                                        lBitp->clone_रूख_Pure(असत्य)};
                // If we're selecting bit zero, then all 32 bits in the mid word
                // get shifted << by 32 bits, so ignore them.
                अचर V3_सङ्ख्या zero{nodep, longOrQuadWidth(nodep)};
                फर्काऊ नूतन AstCond{
                    nfl,
                    // lsb % VL_EDATASIZE == 0 ?
                    नूतन AstEq{nfl, नूतन AstConst{nfl, 0}, lBitp->clone_रूख_Pure(असत्य)},
                    // 0 :
                    नूतन AstConst{nfl, zero},
                    //  midword >> (VL_EDATASIZE - (lbs % VL_EDATASIZE))
                    नूतन AstShiftL{nfl, mWordp, mShiftp, nodep->width()}};
            }();

            // Construct term containing the bits of the high word - if needed
            Ast_गाँठो_अभिव्यक्ति* अचर hTermp = [&]() -> Ast_गाँठो_अभिव्यक्ति* {
                यदि (!hNeeded) फर्काऊ nullptr;

                Ast_गाँठो_अभिव्यक्ति* hWordp = wordSel(hIdxp);
                hWordp = नूतन AstCCast{nfl, hWordp, nodep};
                Ast_गाँठो_अभिव्यक्ति* अचर hShiftp = नूतन AstCond{
                    nfl,
                    // lsb % VL_EDATASIZE == 0 ?
                    नूतन AstEq{nfl, नूतन AstConst{nfl, 0}, lBitp->clone_रूख_Pure(असत्य)},
                    // VL_EDATASIZE :
                    नूतन AstConst{lfl, VL_EDATASIZE},
                    // 64 - (lbs % VL_EDATASIZE)
                    नूतन AstSub{lfl, नूतन AstConst{lfl, 64}, lBitp->clone_रूख_Pure(असत्य)}};
                फर्काऊ नूतन AstShiftL{nfl, hWordp, hShiftp, nodep->width()};
            }();

            // Delete parts not captured during construction
            VL_DO_DANGLING(lBitp->delete_रूख(), lBitp);
            VL_DO_DANGLING(lIdxp->delete_रूख(), lIdxp);
            यदि (mIdxp) VL_DO_DANGLING(mIdxp->delete_रूख(), mIdxp);
            यदि (hIdxp) VL_DO_DANGLING(hIdxp->delete_रूख(), hIdxp);

            // Or reduce the terms
            Ast_गाँठो_अभिव्यक्ति* newp = lTermp;
            यदि (mTermp) newp = नूतन AstOr{nfl, mTermp, newp};
            यदि (hTermp) newp = नूतन AstOr{nfl, hTermp, newp};
            newp->dtypeFrom(nodep);
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        } अन्यथा {  // Long/Quad from Long/Quad
            // No isImpure() check - can handle side effects in below
            didPattern(VExpandPattern::SEL_FROM_NARROW, nodep);
            File_पङ्क्ति* अचर fl = nodep->fileline();
            Ast_गाँठो_अभिव्यक्ति* fromp = nodep->fromp()->unlinkFrBack();
            Ast_गाँठो_अभिव्यक्ति* अचर lsbp = nodep->lsbp()->unlinkFrBack();
            यदि (nodep->isQuad() && !fromp->isQuad()) fromp = नूतन AstCCast{fl, fromp, nodep};
            // {large}>>32 requires 64-bit shift operation; then cast
            Ast_गाँठो_अभिव्यक्ति* newp = नूतन AstShiftR{fl, fromp, lsbp, fromp->width()};
            newp->dtypeFrom(fromp);
            यदि (!nodep->isQuad() && fromp->isQuad()) newp = नूतन AstCCast{fl, newp, nodep};
            newp->dtypeFrom(nodep);
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        }
    }

    तार्किक expandWide(Ast_गाँठो_Assign* nodep, AstSel* rhsp) {
        UASSERT_OBJ(nodep->widthMin() == rhsp->widthConst(), nodep, "Width mismatch");
        यदि (!doExpandWide(nodep)) फर्काऊ असत्य;

        // Simplify the index, in case it becomes a constant
        V3Const::constifyEditCpp(rhsp->lsbp());

        // If it's a constant select and aligned, we can just copy the words
        यदि (अचर AstConst* अचर lsbConstp = VN_CAST(rhsp->lsbp(), Const)) {
            अचर uint32_t lsb = lsbConstp->toUInt();
            यदि (VL_BITBIT_E(lsb) == 0) {
                didPattern(VExpandPattern::ASSIGN_SEL_ALIGNED, nodep);
                अचर uint32_t word = VL_BITWORD_E(lsb);
                लागि (पूर्णाङ्क w = 0; w < nodep->width_शब्दहरू(); ++w) {
                    addWordAssign(nodep, w, newAstWordSelClone(rhsp->fromp(), w + word));
                }
                फर्काऊ सत्य;
            }
        }

        didPattern(VExpandPattern::ASSIGN_SEL_UNALIGNED, nodep);
        File_पङ्क्ति* अचर flp = rhsp->fileline();

        // Use fresh set of temporaries
        ++m_nTmps;

        // Compute word index of LSB, store to temporary if not constant
        Ast_गाँठो_अभिव्यक्ति* अचर wordLsbp = rhsp->lsbp()->clone_रूख_Pure(असत्य);
        Ast_गाँठो_अभिव्यक्ति* wordIdxp = newWordIndex(wordLsbp);
        यदि (!wordLsbp->backp()) VL_DO_DANGLING(wordLsbp->delete_रूख(), wordLsbp);
        wordIdxp = V3Const::constifyEditCpp(wordIdxp);
        यदि (!VN_IS(wordIdxp, Const)) {
            AstVar* अचर tmpp = addLocalTmp(nodep, "ExpandSel_WordIdx", wordIdxp);
            wordIdxp = नूतन AstVarRef{flp, tmpp, VAccess::READ};
        }

        // Compute shift amounts and mask, store to temporaries if not constants
        Ast_गाँठो_अभिव्यक्ति* loShftp = nullptr;
        Ast_गाँठो_अभिव्यक्ति* hiShftp = nullptr;
        Ast_गाँठो_अभिव्यक्ति* hiMaskp = nullptr;
        यदि (अचर AstConst* अचर lsbConstp = VN_CAST(rhsp->lsbp(), Const)) {
            अचर uint32_t bitOffset = VL_BITBIT_E(lsbConstp->toUInt());
            // Must be unaligned, otherwise we would have handled it above
            UASSERT_OBJ(bitOffset, nodep, "Missed aligned wide select");
            loShftp = नूतन AstConst{flp, bitOffset};
            hiShftp = नूतन AstConst{flp, VL_EDATASIZE - bitOffset};
            hiMaskp = nullptr;
        } अन्यथा {
            // Compute Low word shift amount: bottom bits of lsb index
            Ast_गाँठो_अभिव्यक्ति* अचर lsbp = rhsp->lsbp()->clone_रूख_Pure(असत्य);
            loShftp = नूतन AstAnd{flp, नूतन AstConst{flp, VL_SIZEBITS_E}, lsbp};
            AstVar* अचर loTmpp = addLocalTmp(nodep, "ExpandSel_LoShift", loShftp);
            loShftp = नूतन AstVarRef{flp, loTmpp, VAccess::READ};

            // Compute if aligned: Low word shift amount is 0
            Ast_गाँठो_अभिव्यक्ति* अचर zerop = नूतन AstConst{flp, 0};
            Ast_गाँठो_अभिव्यक्ति* alignedp = नूतन AstEq{flp, zerop, loShftp->clone_रूख_Pure(असत्य)};
            AstVar* अचर alignedTmpp = addLocalTmp(nodep, "ExpandSel_Aligned", alignedp);
            alignedp = नूतन AstVarRef{flp, alignedTmpp, VAccess::READ};

            // Computed High word shift amount: 0 if aligned else VL_EDATASIZE - Low word shift
            Ast_गाँठो_अभिव्यक्ति* अचर edsp = नूतन AstConst{flp, VL_EDATASIZE};
            Ast_गाँठो_अभिव्यक्ति* अचर subp = नूतन AstSub{flp, edsp, loShftp->clone_रूख_Pure(असत्य)};
            hiShftp = नूतन AstCond{flp, alignedp, नूतन AstConst{flp, 0}, subp};
            AstVar* अचर hiTmpp = addLocalTmp(nodep, "ExpandSel_HiShift", hiShftp);
            hiShftp = नूतन AstVarRef{flp, hiTmpp, VAccess::READ};

            // Compute the High word mask: 0 if aligned, ones otherwise
            hiMaskp = नूतन AstCond{flp, alignedp->clone_रूख_Pure(असत्य),
                                  नूतन AstConst{flp, AstConst::SizedEData{}, 0},
                                  नूतन AstConst{flp, AstConst::SizedEData{}, -1ULL}};
            AstVar* अचर maskTmpp = addLocalTmp(nodep, "ExpandSel_HiMask", hiMaskp);
            hiMaskp = नूतन AstVarRef{flp, maskTmpp, VAccess::READ};
        }

        // Create each word of the selected result
        Ast_गाँठो_अभिव्यक्ति* अचर fromp = rhsp->fromp();
        अचर पूर्णाङ्क sel_शब्दहरू = nodep->width_शब्दहरू();
        लागि (पूर्णाङ्क w = 0; w < sel_शब्दहरू; ++w) {
            // Grab bits from word 'VL_BITWORD_E(lsb) + w'
            Ast_गाँठो_अभिव्यक्ति* अचर loWordp = newWordSelWord(fromp->fileline(), fromp, wordIdxp, w);
            Ast_गाँठो_अभिव्यक्ति* अचर loShftClonep = loShftp->clone_रूख_Pure(असत्य);
            Ast_गाँठो_अभिव्यक्ति* अचर lop = नूतन AstShiftR{flp, loWordp, loShftClonep, VL_EDATASIZE};
            // Grab bits from word 'VL_BITWORD_E(lsb) + w + 1'
            Ast_गाँठो_अभिव्यक्ति* hiWordp = newWordSelWord(fromp->fileline(), fromp, wordIdxp, w + 1);
            // For the last word of the result, avoid an OOB access when the Sel is not OOB
            यदि (w == sel_शब्दहरू - 1) {
                अचर uint32_t max = fromp->width_शब्दहरू() - w - 1;
                Ast_गाँठो_अभिव्यक्ति* अचर maxp = नूतन AstConst{flp, max};
                Ast_गाँठो_अभिव्यक्ति* अचर condp = नूतन AstGte{flp, wordIdxp->clone_रूख_Pure(असत्य), maxp};
                Ast_गाँठो_अभिव्यक्ति* अचर zerop = नूतन AstConst{flp, AstConst::SizedEData{}, 0};
                hiWordp = नूतन AstCond{flp, condp, zerop, hiWordp};
            }
            Ast_गाँठो_अभिव्यक्ति* अचर hiShftClonep = hiShftp->clone_रूख_Pure(असत्य);
            Ast_गाँठो_अभिव्यक्ति* अचर hiPartp = नूतन AstShiftL{flp, hiWordp, hiShftClonep, VL_EDATASIZE};
            Ast_गाँठो_अभिव्यक्ति* अचर hip
                = hiMaskp ? नूतन AstAnd{flp, hiPartp, hiMaskp->clone_रूख_Pure(असत्य)} : hiPartp;
            // Combine them
            addWordAssign(nodep, w, नूतन AstOr{nodep->fileline(), hip, lop});
        }

        // Delete parts not captured during construction
        यदि (!wordIdxp->backp()) VL_DO_DANGLING(wordIdxp->delete_रूख(), wordIdxp);
        यदि (!loShftp->backp()) VL_DO_DANGLING(loShftp->delete_रूख(), loShftp);
        यदि (!hiShftp->backp()) VL_DO_DANGLING(hiShftp->delete_रूख(), hiShftp);
        यदि (hiMaskp && !hiMaskp->backp()) VL_DO_DANGLING(hiMaskp->delete_रूख(), hiMaskp);

        फर्काऊ सत्य;
    }

    तार्किक expandLhs(Ast_गाँठो_Assign* nodep, AstSel* lhsp) {
        // Possibilities
        //      destp: wide or narrow
        //      rhsp:  wide (destp must be wide), narrow, or 1 bit wide
        //      rhsp:  may be allones and can remove AND NOT gate
        //      lsbp:  constant or variable
        // Yuk.
        यदि (isImpure(nodep)) फर्काऊ असत्य;
        File_पङ्क्ति* अचर nfl = nodep->fileline();
        File_पङ्क्ति* अचर lfl = lhsp->fileline();
        अचर तार्किक destwide = lhsp->fromp()->isWide();
        अचर तार्किक ones = nodep->rhsp()->isAllOnesV();
        यदि (VN_IS(lhsp->lsbp(), Const)) {
            // The code should work without this constant test, but it won't
            // constify as nicely as we'd like.
            Ast_गाँठो_अभिव्यक्ति* rhsp = nodep->rhsp()->unlinkFrBack();
            Ast_गाँठो_अभिव्यक्ति* अचर destp = lhsp->fromp()->unlinkFrBack();
            अचर पूर्णाङ्क lsb = lhsp->lsbConst();
            अचर पूर्णाङ्क msb = lhsp->msbConst();
            V3_सङ्ख्या maskset{nodep, destp->widthMin()};
            maskset.setMask(msb + 1 - lsb, lsb);
            V3_सङ्ख्या maskold{nodep, destp->widthMin()};
            maskold.opNot(maskset);
            यदि (destwide) {
                didPattern(VExpandPattern::ASSIGN_TO_SEL_CONST_WIDE, nodep);
                लागि (पूर्णाङ्क w = 0; w < destp->width_शब्दहरू(); ++w) {
                    यदि (w >= VL_BITWORD_E(lsb) && w <= VL_BITWORD_E(msb)) {
                        // else we would just be setting it to the same exact value
                        Ast_गाँठो_अभिव्यक्ति* oldvalp = newAstWordSelClone(destp, w);
                        fixCloneLvalue(oldvalp);
                        यदि (!ones) {
                            oldvalp = नूतन AstAnd{
                                lfl,
                                नूतन AstConst{lfl, AstConst::SizedEData{}, maskold.edataWord(w)},
                                oldvalp};
                        }

                        // Appropriate word of new value to insert:
                        Ast_गाँठो_अभिव्यक्ति* newp = newWordGrabShift(lfl, w, rhsp, lsb);

                        // Apply cleaning at the top word of the destination
                        // (no cleaning to do if dst's width is a whole number
                        // of words).
                        यदि (w == destp->width_शब्दहरू() - 1 && VL_BITBIT_E(destp->widthMin()) != 0) {
                            V3_सङ्ख्या cleanmask{nodep, VL_EDATASIZE};
                            cleanmask.setMask(VL_BITBIT_E(destp->widthMin()));
                            newp = नूतन AstAnd{lfl, newp, नूतन AstConst{lfl, cleanmask}};
                        }
                        Ast_गाँठो_अभिव्यक्ति* अचर orp
                            = V3Const::constifyEditCpp(नूतन AstOr{lfl, oldvalp, newp});
                        addWordAssign(nodep, w, destp, orp);
                    }
                }
                VL_DO_DANGLING(rhsp->delete_रूख(), rhsp);
                VL_DO_DANGLING(destp->delete_रूख(), destp);
            } अन्यथा {
                didPattern(VExpandPattern::ASSIGN_TO_SEL_CONST_NARROW, nodep);
                यदि (destp->isQuad() && !rhsp->isQuad()) rhsp = नूतन AstCCast{nfl, rhsp, nodep};
                Ast_गाँठो_अभिव्यक्ति* oldvalp = destp->clone_रूख_Pure(सत्य);
                fixCloneLvalue(oldvalp);
                यदि (!ones) oldvalp = नूतन AstAnd{lfl, नूतन AstConst{lfl, maskold}, oldvalp};

                // The bit-select can refer to bits outside the width of nodep
                // which we aren't allowed to assign to.  This is a mask of the
                // valid range of nodep which we apply to the new shifted RHS.
                V3_सङ्ख्या cleanmask{nodep, destp->widthMin()};
                cleanmask.setMask(destp->widthMin());
                Ast_गाँठो_अभिव्यक्ति* अचर shifted = नूतन AstShiftL{
                    lfl, rhsp, नूतन AstConst{lfl, static_cast<uint32_t>(lsb)}, destp->width()};
                Ast_गाँठो_अभिव्यक्ति* अचर cleaned
                    = नूतन AstAnd{lfl, shifted, नूतन AstConst{lfl, cleanmask}};
                Ast_गाँठो_अभिव्यक्ति* अचर orp
                    = V3Const::constifyEditCpp(नूतन AstOr{lfl, oldvalp, cleaned});
                घुसाउ_Before(nodep, नूतन AstAssign{nfl, destp, orp});
            }
            फर्काऊ सत्य;
        } अन्यथा {  // non-const select offset
            यदि (destwide && lhsp->widthConst() == 1) {
                didPattern(VExpandPattern::ASSIGN_TO_SEL_VAR_WIDE_ONE_BIT, nodep);
                Ast_गाँठो_अभिव्यक्ति* अचर rhsp = nodep->rhsp()->unlinkFrBack();
                Ast_गाँठो_अभिव्यक्ति* अचर destp = lhsp->fromp()->unlinkFrBack();
                Ast_गाँठो_अभिव्यक्ति* oldvalp
                    = newWordSelBit(lfl, destp->clone_रूख_Pure(असत्य), lhsp->lsbp());
                fixCloneLvalue(oldvalp);
                यदि (!ones) {
                    oldvalp = नूतन AstAnd{
                        lfl,
                        नूतन AstNot{
                            lfl, नूतन AstShiftL{lfl, नूतन AstConst{nfl, 1},
                                               // newSelBitBit may exceed the MSB of this variable.
                                               // That's ok as we'd just AND with a larger value,
                                               // but oldval would clip the upper bits to sanity
                                               newSelBitBit(lhsp->lsbp()), VL_EDATASIZE}},
                        oldvalp};
                }
                // Restrict the shift amount to 0-31, see bug804.
                Ast_गाँठो_अभिव्यक्ति* अचर shiftp = नूतन AstAnd{nfl, lhsp->lsbp()->clone_रूख_Pure(सत्य),
                                                       नूतन AstConst{nfl, VL_EDATASIZE - 1}};
                Ast_गाँठो* अचर newp = नूतन AstAssign{
                    nfl, newWordSelBit(nfl, destp, lhsp->lsbp()),
                    नूतन AstOr{lfl, oldvalp, नूतन AstShiftL{lfl, rhsp, shiftp, VL_EDATASIZE}}};
                घुसाउ_Before(nodep, newp);
                फर्काऊ सत्य;
            } अन्यथा यदि (destwide) {
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
                फर्काऊ असत्य;
            } अन्यथा {
                didPattern(VExpandPattern::ASSIGN_TO_SEL_VAR_NARROW, nodep);
                // nodep->dumpTree("-  old: ");
                Ast_गाँठो_अभिव्यक्ति* rhsp = nodep->rhsp()->unlinkFrBack();
                Ast_गाँठो_अभिव्यक्ति* अचर destp = lhsp->fromp()->unlinkFrBack();
                Ast_गाँठो_अभिव्यक्ति* oldvalp = destp->clone_रूख_Pure(सत्य);
                fixCloneLvalue(oldvalp);

                V3_सङ्ख्या maskwidth{nodep, destp->widthMin()};
                maskwidth.setMask(lhsp->widthConst());

                यदि (destp->isQuad() && !rhsp->isQuad()) rhsp = नूतन AstCCast{nfl, rhsp, nodep};
                यदि (!ones) {
                    oldvalp = नूतन AstAnd{
                        lfl,
                        नूतन AstNot{lfl, नूतन AstShiftL{lfl, नूतन AstConst{nfl, maskwidth},
                                                      lhsp->lsbp()->clone_रूख_Pure(सत्य),
                                                      destp->width()}},
                        oldvalp};
                }
                Ast_गाँठो_अभिव्यक्ति* newp
                    = नूतन AstShiftL{lfl, rhsp, lhsp->lsbp()->clone_रूख_Pure(सत्य), destp->width()};
                // Apply cleaning to the new value being inserted.  Mask is
                // slightly wider than necessary to avoid an AND with all ones
                // being optimized out.  No need to clean if destp is
                // quad-sized as there are no extra bits to contaminate
                यदि (destp->widthMin() != 64) {
                    V3_सङ्ख्या cleanmask{nodep, destp->widthMin() + 1};
                    cleanmask.setMask(destp->widthMin());
                    newp = नूतन AstAnd{lfl, newp, नूतन AstConst{lfl, cleanmask}};
                }

                घुसाउ_Before(nodep, नूतन AstAssign{nfl, destp, नूतन AstOr{lfl, oldvalp, newp}});
                फर्काऊ सत्य;
            }
        }
    }

    शून्य visit(AstConcat* nodep) override {
        यदि (nodep->user1SetOnce()) फर्काऊ;  // Process once
        iterateChildren(nodep);
        यदि (nodep->isWide()) {
            // See under ASSIGN(WIDE)
        } अन्यथा {
            // No isImpure() check - can handle side effects in below
            didPattern(VExpandPattern::CONCAT, nodep);
            File_पङ्क्ति* अचर fl = nodep->fileline();
            Ast_गाँठो_अभिव्यक्ति* lhsp = nodep->lhsp()->unlinkFrBack();
            Ast_गाँठो_अभिव्यक्ति* rhsp = nodep->rhsp()->unlinkFrBack();
            अचर uint32_t rhsshift = rhsp->widthMin();
            यदि (nodep->isQuad() && !lhsp->isQuad()) lhsp = नूतन AstCCast{fl, lhsp, nodep};
            यदि (nodep->isQuad() && !rhsp->isQuad()) rhsp = नूतन AstCCast{fl, rhsp, nodep};
            Ast_गाँठो_अभिव्यक्ति* अचर newp = नूतन AstOr{
                fl, नूतन AstShiftL{fl, lhsp, नूतन AstConst{fl, rhsshift}, nodep->width()}, rhsp};
            newp->dtypeFrom(nodep);  // Unsigned
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        }
    }
    तार्किक expandWide(Ast_गाँठो_Assign* nodep, AstConcat* rhsp) {
        यदि (!doExpandWide(rhsp)) फर्काऊ असत्य;
        didPattern(VExpandPattern::ASSIGN_CONCAT, nodep);
        File_पङ्क्ति* अचर fl = rhsp->fileline();
        // Lhs or Rhs may be word, long, or quad.
        // newAstWordSelClone nicely abstracts the difference.
        अचर पूर्णाङ्क rhsshift = rhsp->rhsp()->widthMin();
        // Sometimes doing the words backwards is preferable.
        // When we have x={x,foo} backwards is better, when x={foo,x} forward is better
        // However V3Subst tends to rip this up, so not worth optimizing now.
        लागि (पूर्णाङ्क w = 0; w < rhsp->width_शब्दहरू(); ++w) {
            addWordAssign(nodep, w,
                          नूतन AstOr{fl, newWordGrabShift(fl, w, rhsp->lhsp(), rhsshift),
                                    newAstWordSelClone(rhsp->rhsp(), w)});
        }
        फर्काऊ सत्य;
    }

    शून्य visit(AstReplicate* nodep) override {
        यदि (nodep->user1SetOnce()) फर्काऊ;  // Process once
        iterateChildren(nodep);
        यदि (nodep->isWide()) {
            // See under ASSIGN(WIDE)
        } अन्यथा {
            यदि (isImpure(nodep)) फर्काऊ;
            File_पङ्क्ति* अचर fl = nodep->fileline();
            Ast_गाँठो_अभिव्यक्ति* lhsp = nodep->srcp()->unlinkFrBack();
            Ast_गाँठो_अभिव्यक्ति* newp;
            अचर पूर्णाङ्क lhswidth = lhsp->widthMin();
            यदि (lhswidth == 1) {
                didPattern(VExpandPattern::REPLICATE_ONE_BIT, nodep);
                newp = नूतन AstNegate{fl, lhsp};
            } अन्यथा {
                didPattern(VExpandPattern::REPLICATE, nodep);
                अचर AstConst* अचर constp = VN_AS(nodep->countp(), Const);
                UASSERT_OBJ(constp, nodep,
                            "Replication value isn't a constant.  Checked earlier!");
                अचर uint32_t बार = constp->toUInt();
                यदि (nodep->isQuad() && !lhsp->isQuad()) lhsp = नूतन AstCCast{fl, lhsp, nodep};
                newp = lhsp->clone_रूख_Pure(सत्य);
                लागि (अचिन्हित repnum = 1; repnum < बार; repnum++) {
                    अचर पूर्णाङ्क rhsshift = repnum * lhswidth;
                    newp = नूतन AstOr{
                        fl,
                        नूतन AstShiftL{fl, lhsp->clone_रूख_Pure(सत्य),
                                      नूतन AstConst{fl, static_cast<uint32_t>(rhsshift)},
                                      nodep->width()},
                        newp};
                    newp->dtypeFrom(nodep);  // Unsigned
                }
                VL_DO_DANGLING(lhsp->delete_रूख(), lhsp);  // Never used
            }
            newp->dtypeFrom(nodep);  // Unsigned
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        }
    }
    तार्किक expandWide(Ast_गाँठो_Assign* nodep, AstReplicate* rhsp) {
        यदि (!doExpandWide(rhsp)) फर्काऊ असत्य;
        didPattern(VExpandPattern::ASSIGN_REPLICATE, nodep);
        File_पङ्क्ति* अचर fl = nodep->fileline();
        Ast_गाँठो_अभिव्यक्ति* अचर lhsp = rhsp->srcp();
        अचर पूर्णाङ्क lhswidth = lhsp->widthMin();
        अचर AstConst* अचर constp = VN_AS(rhsp->countp(), Const);
        UASSERT_OBJ(constp, rhsp, "Replication value isn't a constant.  Checked earlier!");
        अचर uint32_t बार = constp->toUInt();
        लागि (पूर्णाङ्क w = 0; w < rhsp->width_शब्दहरू(); ++w) {
            Ast_गाँठो_अभिव्यक्ति* newp;
            यदि (lhswidth == 1) {
                newp = नूतन AstNegate{fl, lhsp->clone_रूख_Pure(सत्य)};
                // Replicate always unsigned
                newp->dtypeChgWidthSigned(VL_EDATASIZE, VL_EDATASIZE, VSigning::UNSIGNED);
            } अन्यथा {
                newp = newAstWordSelClone(lhsp, w);
                File_पङ्क्ति* अचर rfl = rhsp->fileline();
                लागि (अचिन्हित repnum = 1; repnum < बार; repnum++) {
                    newp = नूतन AstOr{fl, newWordGrabShift(rfl, w, lhsp, lhswidth * repnum), newp};
                }
            }
            addWordAssign(nodep, w, newp);
        }
        फर्काऊ सत्य;
    }

    शून्य visitEqNeq(Ast_गाँठो_Biop* nodep) {
        यदि (nodep->user1SetOnce()) फर्काऊ;  // Process once
        iterateChildren(nodep);
        यदि (nodep->lhsp()->isWide()
            && !(VN_IS(nodep->lhsp()->dtypep()->skipRefp(), StreamDType)
                 || VN_IS(nodep->rhsp()->dtypep()->skipRefp(), StreamDType))) {
            यदि (isImpure(nodep)) फर्काऊ;
            यदि (!doExpandWide(nodep->lhsp())) फर्काऊ;
            यदि (!doExpandWide(nodep->rhsp())) फर्काऊ;
            didPattern(VN_IS(nodep, Neq) ? VExpandPattern::WORDIZE_NEQ
                                         : VExpandPattern::WORDIZE_EQ,
                       nodep);
            // -> (0=={or{for each_word{WORDSEL(lhs,#)^WORDSEL(rhs,#)}}}
            File_पङ्क्ति* अचर fl = nodep->fileline();
            Ast_गाँठो_अभिव्यक्ति* newp = nullptr;
            लागि (पूर्णाङ्क w = 0; w < nodep->lhsp()->width_शब्दहरू(); ++w) {
                Ast_गाँठो_अभिव्यक्ति* अचर eqp = नूतन AstXor{fl, newAstWordSelClone(nodep->lhsp(), w),
                                                    newAstWordSelClone(nodep->rhsp(), w)};
                newp = newp ? नूतन AstOr{fl, newp, eqp} : eqp;
            }
            यदि (VN_IS(nodep, Neq)) {
                newp = नूतन AstNeq{fl, नूतन AstConst{fl, AstConst::SizedEData{}, 0}, newp};
            } अन्यथा {
                newp = नूतन AstEq{fl, नूतन AstConst{fl, AstConst::SizedEData{}, 0}, newp};
            }
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        }
    }
    शून्य visit(AstEq* nodep) override { visitEqNeq(nodep); }
    शून्य visit(AstNeq* nodep) override { visitEqNeq(nodep); }

    शून्य visit(AstRedOr* nodep) override {
        यदि (nodep->user1SetOnce()) फर्काऊ;  // Process once
        iterateChildren(nodep);
        File_पङ्क्ति* अचर fl = nodep->fileline();
        यदि (nodep->lhsp()->isWide()) {
            यदि (isImpure(nodep)) फर्काऊ;
            यदि (!doExpandWide(nodep->lhsp())) फर्काऊ;
            didPattern(VExpandPattern::WORDIZE_REDOR, nodep);
            // -> (0!={or{for each_word{WORDSEL(lhs,#)}}}
            Ast_गाँठो_अभिव्यक्ति* newp = nullptr;
            लागि (पूर्णाङ्क w = 0; w < nodep->lhsp()->width_शब्दहरू(); ++w) {
                Ast_गाँठो_अभिव्यक्ति* अचर eqp = newAstWordSelClone(nodep->lhsp(), w);
                newp = newp ? नूतन AstOr{fl, newp, eqp} : eqp;
            }
            newp = नूतन AstNeq{fl, नूतन AstConst{fl, AstConst::SizedEData{}, 0}, newp};
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        } अन्यथा {
            // No isImpure() check - can handle side effects in below
            didPattern(VExpandPattern::REDOR_TO_NEQ, nodep);
            Ast_गाँठो_अभिव्यक्ति* अचर lhsp = nodep->lhsp()->unlinkFrBack();
            Ast_गाँठो_अभिव्यक्ति* अचर newp = नूतन AstNeq{
                fl, नूतन AstConst{fl, AstConst::Widthed_मान{}, longOrQuadWidth(nodep), 0}, lhsp};
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        }
    }
    शून्य visit(AstRedAnd* nodep) override {
        यदि (nodep->user1SetOnce()) फर्काऊ;  // Process once
        iterateChildren(nodep);
        File_पङ्क्ति* अचर fl = nodep->fileline();
        यदि (nodep->lhsp()->isWide()) {
            यदि (isImpure(nodep)) फर्काऊ;
            यदि (!doExpandWide(nodep->lhsp())) फर्काऊ;
            didPattern(VExpandPattern::WORDIZE_REDAND, nodep);
            // -> (0!={and{for each_word{WORDSEL(lhs,#)}}}
            Ast_गाँठो_अभिव्यक्ति* newp = nullptr;
            लागि (पूर्णाङ्क w = 0; w < nodep->lhsp()->width_शब्दहरू(); ++w) {
                Ast_गाँठो_अभिव्यक्ति* eqp = newAstWordSelClone(nodep->lhsp(), w);
                यदि (w == nodep->lhsp()->width_शब्दहरू() - 1) {
                    // Rather than doing a (slowish) ==##, we OR in the
                    // bits that aren't part of the mask
                    eqp = नूतन AstOr{fl, नूतन AstConst{fl, notWideMask(nodep->lhsp())},
                                    // Bug in cppcheck
                                    // cppcheck-suppress memleak
                                    eqp};
                }
                newp = newp ? नूतन AstAnd{fl, newp, eqp} : eqp;
            }
            newp = नूतन AstEq{fl, नूतन AstConst{fl, AstConst::SizedEData{}, VL_MASK_E(VL_EDATASIZE)},
                             newp};
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        } अन्यथा {
            // No isImpure() check - can handle side effects in below
            didPattern(VExpandPattern::REDAND_TO_EQ, nodep);
            Ast_गाँठो_अभिव्यक्ति* अचर lhsp = nodep->lhsp()->unlinkFrBack();
            Ast_गाँठो_अभिव्यक्ति* अचर newp = नूतन AstEq{fl, नूतन AstConst{fl, wordMask(lhsp)}, lhsp};
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        }
    }
    शून्य visit(AstRedXor* nodep) override {
        यदि (nodep->user1SetOnce()) फर्काऊ;  // Process once
        iterateChildren(nodep);
        यदि (nodep->lhsp()->isWide()) {
            यदि (isImpure(nodep)) फर्काऊ;
            यदि (!doExpandWide(nodep->lhsp())) फर्काऊ;
            didPattern(VExpandPattern::WORDIZE_REDXOR, nodep);
            // -> (0!={redxor{for each_word{XOR(WORDSEL(lhs,#))}}}
            File_पङ्क्ति* अचर fl = nodep->fileline();
            Ast_गाँठो_अभिव्यक्ति* newp = nullptr;
            लागि (पूर्णाङ्क w = 0; w < nodep->lhsp()->width_शब्दहरू(); ++w) {
                Ast_गाँठो_अभिव्यक्ति* अचर eqp = newAstWordSelClone(nodep->lhsp(), w);
                newp = newp ? नूतन AstXor{fl, newp, eqp} : eqp;
            }
            newp = नूतन AstRedXor{fl, newp};
            UINFO(8, "    Wordize REDXORnew " << newp);
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        }
        // We don't reduce non-wide XORs, as its more efficient to use a temp register,
        // which the inlined function does nicely.
    }

    शून्य visit(Ast_गाँठो_Stmt* nodep) override {
        यदि (nodep->user1SetOnce()) फर्काऊ;  // Process once
        VL_RESTORER(m_stmtp);
        m_stmtp = nodep;
        iterateChildren(nodep);
    }
    शून्य visit(Ast_गाँठो_Assign* nodep) override {
        यदि (nodep->user1SetOnce()) फर्काऊ;  // Process once
        यदि (VN_IS(nodep->dtypep()->skipRefp(), UnpackArrayDType)) {
            फर्काऊ;  // Skip for UnpackArrayDType
        }

        VL_RESTORER(m_stmtp);
        m_stmtp = nodep;
        iterateChildren(nodep);
        तार्किक did = असत्य;
        यदि (nodep->isWide()  //
            && ((VN_IS(nodep->lhsp(), VarRef) || VN_IS(nodep->lhsp(), ArraySel)))
            && !AstVar::scVarRecurse(nodep->lhsp())  // Need special function for SC
            && !AstVar::scVarRecurse(nodep->rhsp())) {
            यदि (AstConst* अचर rhsp = VN_CAST(nodep->rhsp(), Const)) {
                did = expandWide(nodep, rhsp);
            } अन्यथा यदि (AstVarRef* अचर rhsp = VN_CAST(nodep->rhsp(), VarRef)) {
                did = expandWide(nodep, rhsp);
            } अन्यथा यदि (AstSel* अचर rhsp = VN_CAST(nodep->rhsp(), Sel)) {
                did = expandWide(nodep, rhsp);
            } अन्यथा यदि (AstArraySel* अचर rhsp = VN_CAST(nodep->rhsp(), ArraySel)) {
                did = expandWide(nodep, rhsp);
            } अन्यथा यदि (AstConcat* अचर rhsp = VN_CAST(nodep->rhsp(), Concat)) {
                did = expandWide(nodep, rhsp);
            } अन्यथा यदि (AstExtend* अचर rhsp = VN_CAST(nodep->rhsp(), Extend)) {
                did = expandWide(nodep, rhsp);
            } अन्यथा यदि (AstReplicate* अचर rhsp = VN_CAST(nodep->rhsp(), Replicate)) {
                did = expandWide(nodep, rhsp);
            } अन्यथा यदि (AstAnd* अचर rhsp = VN_CAST(nodep->rhsp(), And)) {
                did = expandWide(nodep, rhsp);
            } अन्यथा यदि (AstOr* अचर rhsp = VN_CAST(nodep->rhsp(), Or)) {
                did = expandWide(nodep, rhsp);
            } अन्यथा यदि (AstNot* अचर rhsp = VN_CAST(nodep->rhsp(), Not)) {
                did = expandWide(nodep, rhsp);
            } अन्यथा यदि (AstXor* अचर rhsp = VN_CAST(nodep->rhsp(), Xor)) {
                did = expandWide(nodep, rhsp);
            } अन्यथा यदि (AstShiftL* अचर rhsp = VN_CAST(nodep->rhsp(), ShiftL)) {
                did = expandWideShift(nodep, rhsp, /* isLeftShift: */ सत्य);
            } अन्यथा यदि (AstShiftR* अचर rhsp = VN_CAST(nodep->rhsp(), ShiftR)) {
                did = expandWideShift(nodep, rhsp, /* isLeftShift: */ असत्य);
            } अन्यथा यदि (AstCond* अचर rhsp = VN_CAST(nodep->rhsp(), Cond)) {
                did = expandWide(nodep, rhsp);
            }
        } अन्यथा यदि (AstSel* अचर lhsp = VN_CAST(nodep->lhsp(), Sel)) {
            did = expandLhs(nodep, lhsp);
        }
        // Cleanup common code
        यदि (did) VL_DO_DANGLING(nodep->unlinkFrBack()->delete_रूख(), nodep);
    }

    //--------------------
    शून्य visit(AstVar*) override {}  // Don't hit varrefs under vars
    शून्य visit(Ast_गाँठो* nodep) override { iterateChildren(nodep); }

सार्वजनिक:
    // CONSTRUCTORS
    explicit ExpandVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~ExpandVisitor() override {
        V3Stats::addStat("Optimizations, Expand, expanded wides", m_statWides);
        V3Stats::addStat("Optimizations, Expand, expanded wide words", m_stat_Wide_शब्दहरू);
        V3Stats::addStat("Optimizations, Expand, not expanded impure", m_statImpure);
        V3Stats::addStat("Optimizations, Expand, not expanded limit", m_statWideLimited);
        V3Stats::addStat("Optimizations, Expand, temporaries created", m_statTemps);
        अचर स्वतः emitStat = [एतत्](VExpandPattern pattern) {
            std::string str{pattern.ascii()};
            std::transform(str.begin(), str.end(), str.begin(), [](अचिन्हित वर्ण c) {  //
                फर्काऊ c == '_' ? ' ' : std::छोटे(c);
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

शून्य V3Expand::expandAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        अचर ExpandOkVisitor okVisitor{nodep};
        ExpandVisitor{nodep};
    }  // Destruct before checking
    V3Global::dump_Check_Global_रूख("expand", 0, dump_रूख_Either_Level() >= 3);
}
