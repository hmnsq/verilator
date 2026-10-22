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
#परिभाषय FOR_EACH_EXPAND_आदर्शः(macro) \
    macro(नियोजय_AND) \
    macro(नियोजय_ARRAYSEL) \
    macro(नियोजय_CONCAT) \
    macro(नियोजय_COND) \
    macro(नियोजय_CONST) \
    macro(नियोजय_EXTEND) \
    macro(नियोजय_NOT) \
    macro(नियोजय_OR) \
    macro(नियोजय_REPLICATE) \
    macro(नियोजय_SEL_ALIGNED) \
    macro(नियोजय_SEL_UNALIGNED) \
    macro(नियोजय_SHIFTL) \
    macro(नियोजय_SHIFTR) \
    macro(नियोजय_गन्तव्यम्_SEL_CONST_NARROW) \
    macro(नियोजय_गन्तव्यम्_SEL_CONST_WIDE) \
    macro(नियोजय_गन्तव्यम्_SEL_VAR_NARROW) \
    macro(नियोजय_गन्तव्यम्_SEL_VAR_WIDE_ONE_BIT) \
    macro(नियोजय_VARREF) \
    macro(नियोजय_XOR) \
    macro(CONCAT) \
    macro(REDAND_गन्तव्यम्_EQ) \
    macro(REDOR_गन्तव्यम्_NEQ) \
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

वर्ग VExpand_आदर्शः final {
सार्वजनिक:
    गणना en : uint8_t {
#परिभाषय EXPAND_आदर्शः_ID(id) id,
        FOR_EACH_EXPAND_आदर्शः(EXPAND_आदर्शः_ID)
#अपरिभाषय EXPAND_आदर्शः_ID
            ENUM_अन्तः
    };
    गणना en m_e;
    ध्रुव वर्ण* ascii() ध्रुव {
        स्थिर ध्रुव वर्ण* ध्रुव नामानि[] = {
#परिभाषय EXPAND_आदर्शः_NAME(id) #id,
            FOR_EACH_EXPAND_आदर्शः(EXPAND_आदर्शः_NAME)
#अपरिभाषय EXPAND_आदर्शः_NAME
                "_ENUM_END"  //
        };
        प्रत्यागम नामानि[m_e];
    }

    // cppcheck-suppress noExplicitConstructor
    VExpand_आदर्शः(en _e)
        : m_e{_e} {}
    संक्रियकः en() ध्रुव { प्रत्यागम m_e; }
};

//######################################################################
// Find nodes with side effects, to mark as non-expandable

वर्ग ExpandOkVisitor final : सार्वजनिक VNVisitorConst {
    // NODE STATE
    //  AstNode::user2()        -> bool.  Is pure (along with all children)
    ध्रुव VNUser2_अन्तर्_Use m_inuser2;

    // STATE - for current visit position (use VL_RESTORER)
    // Tracks similar to AstNode::isTreePureRecurse(), but avoid O(n^2)
    // False = pure, as nodes that ExpandVisitor inserts preserve pureness
    तार्किक m_isImpure = सत्य;  // Currently pure

    शून्य दर्शय(Ast_ग्रन्थिः* nodep) override {
        तार्किक selfImpure = !nodep->is_विशुद्धम्();
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
    स्पष्ट ExpandOkVisitor(AstNetlist* nodep) { iterateConst(nodep); }
    ~ExpandOkVisitor() = स्वभाव;
};

//######################################################################
// Expand state, as a visitor of each AstNode

वर्ग ExpandVisitor final : सार्वजनिक VNVisitor {
    // NODE STATE
    //  AstNode::user1()        -> bool.  Processed
    //  AstNode::user2()        -> See ExpandOkVisitor
    ध्रुव VNUser1_अन्तर्_Use m_inuser1;

    // STATE - for current visit position (use VL_RESTORER)
    Ast_ग्रन्थिः* m_stmtp = शून्यसूचकः;  // Current statement

    // STATE - across all visitors
    AstCFunc* m_funcp = शून्यसूचकः;  // Current function
    VDouble0 m_statImpure;  // Statistic tracking
    VDouble0 m_statWides;  // Statistic tracking
    VDouble0 m_stat_Wide_शब्दाः;  // Statistic tracking
    VDouble0 m_statWideLimited;  // Statistic tracking
    VDouble0 m_statTemps;  // Statistic tracking
    // Number of applications of each expansion pattern for statistics
    std::array<VDouble0, VExpand_आदर्शः::ENUM_अन्तः> m_statPatterns;

    // STATE - for current function
    आकार_t m_nTmps = 0;  // Sequence numbers for temopraries

    // METHODS
    // Note the application of the given expansion pattern to 'nodep'
    शून्य did_आदर्शः(VExpand_आदर्शः आदर्शः_2, Ast_ग्रन्थिः* nodep) {
        UINFO(8, "    " << आदर्शः_2.ascii() << " " << nodep);
        ++m_statPatterns[आदर्शः_2];
    }

    // Use state that ExpandOkVisitor calculated
    स्थिर तार्किक isImpure(Ast_ग्रन्थिः* nodep) {
        ध्रुव तार्किक impure = nodep->user2();
        यदि (impure) UINFO(9, "      impure " << nodep);
        प्रत्यागम impure;
    }

    तार्किक doExpandWide(Ast_ग्रन्थिः* nodep) {
        यदि (isImpure(nodep)) {
            ++m_statImpure;
            प्रत्यागम असत्य;
        }
        यदि (nodep->विस्तारः_शब्दाः() <= v3Global.opt.expand_सीमा()) {
            ++m_statWides;
            m_stat_Wide_शब्दाः += nodep->विस्तारः_शब्दाः();
            प्रत्यागम सत्य;
        } अन्यथा {
            ++m_statWideLimited;
            प्रत्यागम असत्य;
        }
    }

    स्थिर पूर्णाङ्क long_Or_Quad_विस्तारः(Ast_ग्रन्थिः* nodep) {
        प्रत्यागम (nodep->विस्तारः() + (VL_EDATASIZE - 1)) & ~(VL_EDATASIZE - 1);
    }
    स्थिर V3_सङ्ख्या not_Wide_आवरणम्(Ast_ग्रन्थिः* nodep) {
        प्रत्यागम V3_सङ्ख्या{nodep, VL_EDATASIZE, ~VL_आवरणम्_E(nodep->विस्तारः_Min())};
    }
    स्थिर V3_सङ्ख्या word_आवरणम्(Ast_ग्रन्थिः* nodep) {
        यदि (nodep->isWide()) {
            प्रत्यागम V3_सङ्ख्या{nodep, VL_EDATASIZE, VL_आवरणम्_E(nodep->विस्तारः_Min())};
        } अन्यथा {
            V3_सङ्ख्या आवरणम्{nodep, long_Or_Quad_विस्तारः(nodep)};
            आवरणम्.समुच्चयः_आवरणम्(nodep->विस्तारः_Min());
            प्रत्यागम आवरणम्;
        }
    }

    स्थिर शून्य निवेशय_Before(Ast_ग्रन्थिः* placep, Ast_ग्रन्थिः* newp) {
        newp->user1(1);  // Already processed, don't need to re-iterate
        placep->सङ्कलय_अत्र_This_As_अग्र(newp);
    }
    स्थिर शून्य replaceWithDelete(Ast_ग्रन्थिः* nodep, Ast_ग्रन्थिः* newp) {
        newp->user1(1);  // Already processed, don't need to re-iterate
        यदि (newp->विस्तारः() != nodep->विस्तारः()) {
            UASSERT_OBJ(newp->विस्तारः_Min() == nodep->विस्तारः_Min(), nodep,
                        "Replacement width mismatch");
            newp->dtype_Chg_विस्तारः(nodep->विस्तारः(), nodep->विस्तारः_Min());
        }
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
    }
    स्थिर Ast_ग्रन्थिः* new_Word_नियोजय(Ast_ग्रन्थिः_नियोजय* placep, पूर्णाङ्क word, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp,
                                  Ast_ग्रन्थिः_व्यञ्जकम्* rhsp) {
        File_पङ्क्तिः* ध्रुव fl = placep->fileline();
        प्रत्यागम नूतन Ast_नियोजय{fl,
                             नूतन AstWordSel{fl, lhsp->clone_वृक्षः_विशुद्धम्(सत्य),
                                            नूतन AstConst{fl, static_cast<uint32_t>(word)}},
                             rhsp};
    }
    स्थिर शून्य सङ्कलय_Word_नियोजय(Ast_ग्रन्थिः_नियोजय* placep, पूर्णाङ्क word, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp,
                              Ast_ग्रन्थिः_व्यञ्जकम्* rhsp) {
        निवेशय_Before(placep, new_Word_नियोजय(placep, word, lhsp, rhsp));
    }
    स्थिर शून्य सङ्कलय_Word_नियोजय(Ast_ग्रन्थिः_नियोजय* placep, पूर्णाङ्क word, Ast_ग्रन्थिः_व्यञ्जकम्* rhsp) {
        सङ्कलय_Word_नियोजय(placep, word, placep->lhsp(), rhsp);
    }

    AstVar* सङ्कलय_Local_Tmp(Ast_ग्रन्थिः* placep, ध्रुव वर्ण* namep, Ast_ग्रन्थिः_व्यञ्जकम्* valuep) {
        ध्रुव std::string name = "__V"s + namep + "_"s + std::गन्तव्यम्_string(m_nTmps);
        File_पङ्क्तिः* ध्रुव flp = placep->fileline();
        AstVar* ध्रुव tmpp = नूतन AstVar{flp, VVarType::STMTTEMP, name, valuep->dtypep()};
        tmpp->funcLocal(सत्य);
        tmpp->is_आन्तरिकम्(सत्य);
        tmpp->noReset(सत्य);
        tmpp->substConstOnly(सत्य);
        m_funcp->सङ्कलय_Varsp(tmpp);
        निवेशय_Before(placep, नूतन Ast_नियोजय{flp, नूतन AstVarRef{flp, tmpp, VAccess::WRITE}, valuep});
        ++m_statTemps;
        प्रत्यागम tmpp;
    }

    स्थिर शून्य fixCloneLvalue(Ast_ग्रन्थिः* nodep) {
        // In AstSel transforms, we call clone() on VarRefs that were lvalues,
        // but are now being used on the RHS of the assignment
        यदि (VN_IS(nodep, VarRef)) VN_AS(nodep, VarRef)->access(VAccess::READ);
        // Iterate
        यदि (Ast_ग्रन्थिः* ध्रुव refp = nodep->op1p()) fixCloneLvalue(refp);
        यदि (Ast_ग्रन्थिः* ध्रुव refp = nodep->op2p()) fixCloneLvalue(refp);
        यदि (Ast_ग्रन्थिः* ध्रुव refp = nodep->op3p()) fixCloneLvalue(refp);
        यदि (Ast_ग्रन्थिः* ध्रुव refp = nodep->op4p()) fixCloneLvalue(refp);
    }

    स्थिर Ast_ग्रन्थिः_व्यञ्जकम्* newAstWordSelClone(Ast_ग्रन्थिः_व्यञ्जकम्* nodep, पूर्णाङ्क word) {
        // Get the specified word number from a wide array
        // Or, if it's a long/quad, do appropriate conversion to wide
        // Concat may pass negative word numbers, that means it wants a zero
        File_पङ्क्तिः* ध्रुव fl = nodep->fileline();
        यदि (nodep->isWide() && word >= 0 && word < nodep->विस्तारः_शब्दाः()) {
            प्रत्यागम नूतन AstWordSel{fl, nodep->clone_वृक्षः_विशुद्धम्(सत्य),
                                  नूतन AstConst{fl, static_cast<uint32_t>(word)}};
        } अन्यथा यदि (nodep->isQuad() && word == 0) {
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव quadfromp = nodep->clone_वृक्षः_विशुद्धम्(सत्य);
            quadfromp->dtype_समुच्चयः_Bit_Unsized(VL_QUADSIZE, quadfromp->विस्तारः_Min(), VSigning::UNSIGNED);
            प्रत्यागम नूतन AstCCast{fl, quadfromp, VL_EDATASIZE};
        } अन्यथा यदि (nodep->isQuad() && word == 1) {
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव quadfromp = nodep->clone_वृक्षः_विशुद्धम्(सत्य);
            quadfromp->dtype_समुच्चयः_Bit_Unsized(VL_QUADSIZE, quadfromp->विस्तारः_Min(), VSigning::UNSIGNED);
            प्रत्यागम नूतन AstCCast{
                fl, नूतन Ast_सरणम्_R{fl, quadfromp, नूतन AstConst{fl, VL_EDATASIZE}, VL_EDATASIZE},
                VL_EDATASIZE};
        } अन्यथा यदि (!nodep->isWide() && !nodep->isQuad() && word == 0) {
            प्रत्यागम nodep->clone_वृक्षः_विशुद्धम्(सत्य);
        } अन्यथा {  // Out of bounds
            प्रत्यागम नूतन AstConst{fl, 0};
        }
    }

    स्थिर Ast_ग्रन्थिः_व्यञ्जकम्* new_Word_Grab_सरणम्(File_पङ्क्तिः* fl, पूर्णाङ्क word, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, पूर्णाङ्क सरणम्_2) {
        // Extract the expression to grab the value for the specified word, if it's the shift
        // of shift bits from lhsp
        // Negative word numbers requested for lhs when it's "before" what we want.
        // We get a 0 then.
        ध्रुव पूर्णाङ्क othword = word - सरणम्_2 / VL_EDATASIZE;
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lop = newAstWordSelClone(lhsp, othword);
        ध्रुव uint32_t lo_सरणम् = VL_BITBIT_E(सरणम्_2);
        यदि (!lo_सरणम्) प्रत्यागम lop;
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव hip = newAstWordSelClone(lhsp, othword - 1);
        ध्रुव uint32_t hi_सरणम् = VL_EDATASIZE - lo_सरणम्;  // Complement offset
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव newp
            = नूतन AstOr{fl,  //
                        नूतन Ast_सरणम्_R{fl, hip, नूतन AstConst{fl, hi_सरणम्}, VL_EDATASIZE},
                        नूतन Ast_सरणम्_L{fl, lop, नूतन AstConst{fl, lo_सरणम्}, VL_EDATASIZE}};
        प्रत्यागम V3Const::constifyEditCpp(newp);
    }

    // Return expression indexing the word that contains 'lsbp' + the given word offset
    स्थिर Ast_ग्रन्थिः_व्यञ्जकम्* new_Word_सूचकांक(Ast_ग्रन्थिः_व्यञ्जकम्* lsbp, uint32_t word_विस्थापनम् = 0) {
        // This is indexing a WordSel, so a 32 bit constants are fine
        File_पङ्क्तिः* ध्रुव flp = lsbp->fileline();
        यदि (ध्रुव AstConst* constp = VN_CAST(lsbp, Const)) {
            प्रत्यागम नूतन AstConst{flp, word_विस्थापनम् + VL_BITWORD_E(constp->गन्तव्यम्_UInt())};
        }

        यदि (lsbp->backp()) lsbp = lsbp->clone_वृक्षः_विशुद्धम्(असत्य);
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव wwl2p = नूतन AstConst{flp, VL_EDATASIZE_LOG2};  // Word width log 2
        Ast_ग्रन्थिः_व्यञ्जकम्* indexp = नूतन Ast_सरणम्_R{flp, lsbp, wwl2p, VL_EDATASIZE};
        यदि (word_विस्थापनम्) indexp = नूतन Ast_सङ्कलय{flp, नूतन AstConst{flp, word_विस्थापनम्}, indexp};
        प्रत्यागम indexp;
    }

    // Return word of fromp that contains word indexp + the given word offset.
    स्थिर Ast_ग्रन्थिः_व्यञ्जकम्* newWordSelWord(File_पङ्क्तिः* flp, Ast_ग्रन्थिः_व्यञ्जकम्* fromp, Ast_ग्रन्थिः_व्यञ्जकम्* indexp,
                                       uint32_t word_विस्थापनम् = 0) {
        UASSERT_OBJ(fromp->isWide(), fromp, "Only need AstWordSel on wide from's");

        यदि (ध्रुव AstConst* ध्रुव constp = VN_CAST(indexp, Const)) {
            indexp = शून्यसूचकः;
            word_विस्थापनम् += constp->गन्तव्यम्_UInt();
        }

        // e.g. "logic [95:0] var[0]; logic [0] sel; out = var[sel];"
        // Squash before C++ to avoid getting a C++ compiler warning
        // (even though code would be unreachable as presumably a
        // AstCond is protecting above this node.
        यदि (word_विस्थापनम् >= static_cast<uint32_t>(fromp->विस्तारः_शब्दाः())) {
            प्रत्यागम नूतन AstConst{flp, AstConst::SizedEData{}, 0};
        }

        यदि (fromp->backp()) fromp = fromp->clone_वृक्षः_विशुद्धम्(असत्य);

        // If indexp was constant, just use it
        यदि (!indexp) प्रत्यागम नूतन AstWordSel{flp, fromp, नूतन AstConst{flp, word_विस्थापनम्}};

        // Otherwise compute at runtime
        यदि (indexp->backp()) indexp = indexp->clone_वृक्षः_विशुद्धम्(असत्य);
        यदि (word_विस्थापनम्) indexp = नूतन Ast_सङ्कलय{flp, नूतन AstConst{flp, word_विस्थापनम्}, indexp};
        प्रत्यागम नूतन AstWordSel{flp, fromp, indexp};
    }

    // Return word of fromp that contains bit lsbp + the given word offset.
    स्थिर Ast_ग्रन्थिः_व्यञ्जकम्* newWordSelBit(File_पङ्क्तिः* flp, Ast_ग्रन्थिः_व्यञ्जकम्* fromp, Ast_ग्रन्थिः_व्यञ्जकम्* lsbp,
                                      uint32_t word_विस्थापनम् = 0) {
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव indexp = new_Word_सूचकांक(lsbp, word_विस्थापनम्);
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव wordSelp = newWordSelWord(flp, fromp, indexp);
        यदि (!indexp->backp()) VL_DO_DANGLING(indexp->delete_वृक्षः(), indexp);
        प्रत्यागम wordSelp;
    }

    स्थिर Ast_ग्रन्थिः_व्यञ्जकम्* newSelBitBit(Ast_ग्रन्थिः_व्यञ्जकम्* lsbp) {
        // Return equation to get the VL_BITBIT of a constant or non-constant
        File_पङ्क्तिः* ध्रुव fl = lsbp->fileline();
        यदि (VN_IS(lsbp, Const)) {
            प्रत्यागम नूतन AstConst{fl, VL_BITBIT_E(VN_AS(lsbp, Const)->गन्तव्यम्_UInt())};
        } अन्यथा {
            प्रत्यागम नूतन AstAnd{fl, नूतन AstConst{fl, VL_EDATASIZE - 1}, lsbp->clone_वृक्षः_विशुद्धम्(सत्य)};
        }
    }

    //====================

    तार्किक expandWide(Ast_ग्रन्थिः_नियोजय* nodep, AstConst* rhsp) {
        यदि (!doExpandWide(nodep)) प्रत्यागम असत्य;
        did_आदर्शः(VExpand_आदर्शः::नियोजय_CONST, nodep);
        // -> {for each_word{ ASSIGN(WORDSEL(wide,#),WORDSEL(CONST,#))}}
        यदि (rhsp->अङ्कः().isFourState()) {
            rhsp->v3warn(E_UNSUPPORTED,  // LCOV_EXCL_LINE  // impossible?
                         "Unsupported: 4-state numbers in this context");
        }
        File_पङ्क्तिः* ध्रुव fl = nodep->fileline();
        प्रति (पूर्णाङ्क w = 0; w < nodep->विस्तारः_शब्दाः(); ++w) {
            सङ्कलय_Word_नियोजय(nodep, w,
                          नूतन AstConst{fl, AstConst::SizedEData{}, rhsp->अङ्कः().edataWord(w)});
        }
        प्रत्यागम सत्य;
    }
    //-------- Uniops
    तार्किक expandWide(Ast_ग्रन्थिः_नियोजय* nodep, AstVarRef* rhsp) {
        // Special case: do not expand assignment of constant pool variables.
        // V3Subst undestands these directly.
        यदि (rhsp->varp()->const_Pool_प्रविष्टिः()) प्रत्यागम असत्य;
        यदि (!doExpandWide(nodep)) प्रत्यागम असत्य;
        did_आदर्शः(VExpand_आदर्शः::नियोजय_VARREF, nodep);
        प्रति (पूर्णाङ्क w = 0; w < nodep->विस्तारः_शब्दाः(); ++w) {
            सङ्कलय_Word_नियोजय(nodep, w, newAstWordSelClone(rhsp, w));
        }
        प्रत्यागम सत्य;
    }
    तार्किक expandWide(Ast_ग्रन्थिः_नियोजय* nodep, AstArraySel* rhsp) {
        UASSERT_OBJ(!VN_IS(nodep->dtypep()->skipRefp(), UnpackArrayDType), nodep,
                    "ArraySel with unpacked arrays should have been removed in V3Slice");
        यदि (!doExpandWide(nodep)) प्रत्यागम असत्य;
        did_आदर्शः(VExpand_आदर्शः::नियोजय_ARRAYSEL, nodep);
        प्रति (पूर्णाङ्क w = 0; w < nodep->विस्तारः_शब्दाः(); ++w) {
            सङ्कलय_Word_नियोजय(nodep, w, newAstWordSelClone(rhsp, w));
        }
        प्रत्यागम सत्य;
    }
    तार्किक expandWide(Ast_ग्रन्थिः_नियोजय* nodep, AstNot* rhsp) {
        // -> {for each_word{ ASSIGN(WORDSEL(wide,#),NOT(WORDSEL(lhs,#))) }}
        यदि (!doExpandWide(nodep)) प्रत्यागम असत्य;
        did_आदर्शः(VExpand_आदर्शः::नियोजय_NOT, nodep);
        File_पङ्क्तिः* ध्रुव fl = rhsp->fileline();
        प्रति (पूर्णाङ्क w = 0; w < nodep->विस्तारः_शब्दाः(); ++w) {
            सङ्कलय_Word_नियोजय(nodep, w, नूतन AstNot{fl, newAstWordSelClone(rhsp->lhsp(), w)});
        }
        प्रत्यागम सत्य;
    }
    //-------- Biops
    तार्किक expandWide(Ast_ग्रन्थिः_नियोजय* nodep, AstAnd* rhsp) {
        यदि (!doExpandWide(nodep)) प्रत्यागम असत्य;
        did_आदर्शः(VExpand_आदर्शः::नियोजय_AND, nodep);
        File_पङ्क्तिः* ध्रुव fl = nodep->fileline();
        प्रति (पूर्णाङ्क w = 0; w < nodep->विस्तारः_शब्दाः(); ++w) {
            सङ्कलय_Word_नियोजय(nodep, w,
                          नूतन AstAnd{fl, newAstWordSelClone(rhsp->lhsp(), w),
                                     newAstWordSelClone(rhsp->rhsp(), w)});
        }
        प्रत्यागम सत्य;
    }
    तार्किक expandWide(Ast_ग्रन्थिः_नियोजय* nodep, AstOr* rhsp) {
        यदि (!doExpandWide(nodep)) प्रत्यागम असत्य;
        did_आदर्शः(VExpand_आदर्शः::नियोजय_OR, nodep);
        File_पङ्क्तिः* ध्रुव fl = nodep->fileline();
        प्रति (पूर्णाङ्क w = 0; w < nodep->विस्तारः_शब्दाः(); ++w) {
            सङ्कलय_Word_नियोजय(nodep, w,
                          नूतन AstOr{fl, newAstWordSelClone(rhsp->lhsp(), w),
                                    newAstWordSelClone(rhsp->rhsp(), w)});
        }
        प्रत्यागम सत्य;
    }
    तार्किक expandWide(Ast_ग्रन्थिः_नियोजय* nodep, AstXor* rhsp) {
        यदि (!doExpandWide(nodep)) प्रत्यागम असत्य;
        did_आदर्शः(VExpand_आदर्शः::नियोजय_XOR, nodep);
        File_पङ्क्तिः* ध्रुव fl = nodep->fileline();
        प्रति (पूर्णाङ्क w = 0; w < nodep->विस्तारः_शब्दाः(); ++w) {
            सङ्कलय_Word_नियोजय(nodep, w,
                          नूतन AstXor{fl, newAstWordSelClone(rhsp->lhsp(), w),
                                     newAstWordSelClone(rhsp->rhsp(), w)});
        }
        प्रत्यागम सत्य;
    }
    तार्किक expand_Wide_सरणम्(Ast_ग्रन्थिः_नियोजय* nodep, Ast_ग्रन्थिः_Biop* rhsp, तार्किक is_वामः_सरणम्) {
        यदि (!doExpandWide(nodep)) प्रत्यागम असत्य;

        // Simplify the shift amount, in case it becomes a constant
        V3Const::constifyEditCpp(rhsp->rhsp());

        // If it's a constant shift by whole words, expand it so V3Subst can substitute it
        यदि (ध्रुव AstConst* ध्रुव rhsConstp = VN_CAST(rhsp->rhsp(), Const)) {
            ध्रुव uint32_t सरणम्_द्वयङ्काः = rhsConstp->गन्तव्यम्_UInt();
            यदि (VL_BITBIT_E(सरणम्_द्वयङ्काः) == 0) {
                ध्रुव पूर्णाङ्क विस्तारः_शब्दाः = nodep->विस्तारः_शब्दाः();
                ध्रुव पूर्णाङ्क सरणम्_शब्दाः = std::min<पूर्णाङ्क>(VL_BITWORD_E(सरणम्_द्वयङ्काः), विस्तारः_शब्दाः);
                File_पङ्क्तिः* ध्रुव flp = rhsp->fileline();
                यदि (is_वामः_सरणम्) {
                    did_आदर्शः(VExpand_आदर्शः::नियोजय_SHIFTL, nodep);
                    // Low words of the result are zero
                    प्रति (पूर्णाङ्क w = 0; w < सरणम्_शब्दाः; ++w) {
                        सङ्कलय_Word_नियोजय(nodep, w, नूतन AstConst{flp, AstConst::SizedEData{}, 0});
                    }
                    // High words of the result are copied from higher words of the source
                    प्रति (पूर्णाङ्क w = सरणम्_शब्दाः; w < विस्तारः_शब्दाः; ++w) {
                        सङ्कलय_Word_नियोजय(nodep, w, newAstWordSelClone(rhsp->lhsp(), w - सरणम्_शब्दाः));
                    }
                } अन्यथा {
                    did_आदर्शः(VExpand_आदर्शः::नियोजय_SHIFTR, nodep);
                    // Low words of the result are copied from higher words of the source
                    प्रति (पूर्णाङ्क w = 0; w < विस्तारः_शब्दाः - सरणम्_शब्दाः; ++w) {
                        सङ्कलय_Word_नियोजय(nodep, w, newAstWordSelClone(rhsp->lhsp(), w + सरणम्_शब्दाः));
                    }
                    // High words of the result are zero
                    प्रति (पूर्णाङ्क w = विस्तारः_शब्दाः - सरणम्_शब्दाः; w < विस्तारः_शब्दाः; ++w) {
                        सङ्कलय_Word_नियोजय(nodep, w, नूतन AstConst{flp, AstConst::SizedEData{}, 0});
                    }
                }
                प्रत्यागम सत्य;
            }
        }

        प्रत्यागम असत्य;
    }

    //-------- Triops
    तार्किक expandWide(Ast_ग्रन्थिः_नियोजय* nodep, AstCond* rhsp) {
        यदि (!doExpandWide(nodep)) प्रत्यागम असत्य;
        did_आदर्शः(VExpand_आदर्शः::नियोजय_COND, nodep);
        File_पङ्क्तिः* ध्रुव fl = nodep->fileline();
        प्रति (पूर्णाङ्क w = 0; w < nodep->विस्तारः_शब्दाः(); ++w) {
            सङ्कलय_Word_नियोजय(nodep, w,
                          नूतन AstCond{fl, rhsp->condp()->clone_वृक्षः_विशुद्धम्(सत्य),
                                      newAstWordSelClone(rhsp->thenp(), w),
                                      newAstWordSelClone(rhsp->elsep(), w)});
        }
        प्रत्यागम सत्य;
    }

    // VISITORS
    शून्य दर्शय(AstNetlist* nodep) override { iterate_And_अग्र_Null(nodep->modulesp()); }

    शून्य दर्शय(AstCFunc* nodep) override {
        VL_RESTORER(m_funcp);
        VL_RESTORER(m_nTmps);
        m_funcp = nodep;
        m_nTmps = 0;
        ध्रुव VDouble0 statWidesBefore = m_statWides;
        iterateChildren(nodep);

        // Constant fold here if anything was expanded, as Ast size can likely be reduced
        यदि (v3Global.opt.fConstEager() && m_statWides != statWidesBefore) {
            Ast_ग्रन्थिः* ध्रुव editedp = V3Const::constifyEditCpp(nodep);
            UASSERT_OBJ(editedp == nodep, editedp, "Should not have replaced CFunc");
        }
    }

    शून्य दर्शय(AstExtend* nodep) override {
        यदि (nodep->user1_समुच्चयः_Once()) प्रत्यागम;  // Process once
        iterateChildren(nodep);
        यदि (nodep->isWide()) {
            // See under ASSIGN(EXTEND)
        } अन्यथा {
            यदि (isImpure(nodep)) प्रत्यागम;
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lhsp = nodep->lhsp()->unlink_Fr_पृष्ठभागः();
            Ast_ग्रन्थिः_व्यञ्जकम्* newp = lhsp;
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
    तार्किक expandWide(Ast_ग्रन्थिः_नियोजय* nodep, AstExtend* rhsp) {
        यदि (!doExpandWide(nodep)) प्रत्यागम असत्य;
        did_आदर्शः(VExpand_आदर्शः::नियोजय_EXTEND, nodep);
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rlhsp = rhsp->lhsp();
        प्रति (पूर्णाङ्क w = 0; w < rlhsp->विस्तारः_शब्दाः(); ++w) {
            सङ्कलय_Word_नियोजय(nodep, w, newAstWordSelClone(rlhsp, w));
        }
        प्रति (पूर्णाङ्क w = rlhsp->विस्तारः_शब्दाः(); w < nodep->विस्तारः_शब्दाः(); ++w) {
            सङ्कलय_Word_नियोजय(nodep, w, नूतन AstConst{nodep->fileline(), AstConst::SizedEData{}, 0});
        }
        प्रत्यागम सत्य;
    }

    शून्य दर्शय(AstSel* nodep) override {
        यदि (nodep->user1_समुच्चयः_Once()) प्रत्यागम;  // Process once
        iterateChildren(nodep);

        ध्रुव uint32_t विस्तारः = static_cast<uint32_t>(nodep->विस्तारः_Const());
        UASSERT_OBJ(nodep->विस्तारः_Min() == static_cast<पूर्णाङ्क>(विस्तारः), nodep, "Width mismatch");

        // Skip if Sel is an LHS assignment select
        यदि (Ast_ग्रन्थिः_नियोजय* ध्रुव assignp = VN_CAST(nodep->backp(), ग्रन्थिः_नियोजय)) {
            यदि (nodep == assignp->lhsp()) प्रत्यागम;
        }

        // Skip if wide: See under ASSIGN(WIDE)
        यदि (nodep->isWide()) प्रत्यागम;

        // Skip Sel from stream or queue
        यदि (VN_IS(nodep->fromp()->dtypep(), प्रवाहः_DType)) प्रत्यागम;
        यदि (VN_IS(nodep->fromp()->dtypep(), QueueDType)) प्रत्यागम;

        // Result must be non-wide after skipping all the above
        UASSERT_OBJ(विस्तारः <= 64, nodep, "Inconsistent result width");

        यदि (nodep->fromp()->isWide()) {  // Long/Quad from Wide
            यदि (isImpure(nodep)) प्रत्यागम;
            did_आदर्शः(VExpand_आदर्शः::SEL_FROM_WIDE, nodep);
            File_पङ्क्तिः* ध्रुव nfl = nodep->fileline();
            File_पङ्क्तिः* ध्रुव lfl = nodep->lsbp()->fileline();
            File_पङ्क्तिः* ध्रुव ffl = nodep->fromp()->fileline();
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव fromp = nodep->fromp();

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
            ध्रुव स्वचालित tryFold = [&](Ast_ग्रन्थिः_व्यञ्जकम्* exprp) -> Ast_ग्रन्थिः_व्यञ्जकम्* {
                // Size limit for inlining expressions (any arbitrary value results in
                // a bounded expansion. Picked a value to allow common simple forms.)
                स्थिर constexpr पूर्णाङ्क EXPAND_SEL_LSB_सीमा = 8;
                // Simplify
                exprp = V3Const::constifyEditCpp(exprp);
                // Accept if not larger than the limit
                यदि (!exprp->isLargerThan(EXPAND_SEL_LSB_सीमा)) प्रत्यागम exprp;
                // Delete if rejected
                VL_DO_DANGLING(exprp->delete_वृक्षः(), exprp);
                प्रत्यागम शून्यसूचकः;
            };

            // Return the index of the word holding the bit 'offset' bits above the LSB
            ध्रुव स्वचालित wordIdx = [&](uint32_t विस्थापनम्) -> Ast_ग्रन्थिः_व्यञ्जकम्* {
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव msbp = नूतन Ast_सङ्कलय{lfl, नूतन AstConst{lfl, विस्थापनम्},
                                                     nodep->lsbp()->clone_वृक्षः_विशुद्धम्(असत्य)};
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव idxp = new_Word_सूचकांक(msbp);
                यदि (!msbp->backp()) VL_DO_DANGLING(msbp->delete_वृक्षः(), msbp);
                प्रत्यागम idxp;
            };

            // Bit index of the selected LSB within its word
            Ast_ग्रन्थिः_व्यञ्जकम्* lBitp = tryFold(newSelBitBit(nodep->lsbp()));
            ध्रुव तार्किक aligned = lBitp && lBitp->isZero();

            // Which of the other 2 words the expansion might need
            ध्रुव तार्किक mNeeded = विस्तारः > 1 && !aligned;
            ध्रुव तार्किक hNeeded = विस्तारः > VL_EDATASIZE;
            UASSERT_OBJ(!hNeeded || nodep->isQuad(), nodep, "Width mismatch");

            // Word indices
            ध्रुव uint32_t m_Msb_विस्थापनम् = std::min<uint32_t>(विस्तारः, VL_EDATASIZE) - 1;
            ध्रुव uint32_t h_Msb_विस्थापनम् = विस्तारः - 1;
            Ast_ग्रन्थिः_व्यञ्जकम्* lIdxp = tryFold(new_Word_सूचकांक(nodep->lsbp()));
            Ast_ग्रन्थिः_व्यञ्जकम्* mIdxp = mNeeded ? tryFold(wordIdx(m_Msb_विस्थापनम्)) : शून्यसूचकः;
            Ast_ग्रन्थिः_व्यञ्जकम्* hIdxp = hNeeded ? tryFold(wordIdx(h_Msb_विस्थापनम्)) : शून्यसूचकः;

            // If the LSB expression is needed multiple times, evaluate it into a temporary
            ध्रुव पूर्णाङ्क nCopies = !lBitp + !lIdxp + (mNeeded && !mIdxp) + (hNeeded && !hIdxp);
            यदि (nCopies > 1 && m_funcp && m_stmtp && !isImpure(m_stmtp)) {
                ++m_nTmps;  // Use fresh set of temporaries
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lsbp = nodep->lsbp()->unlink_Fr_पृष्ठभागः();
                AstVar* ध्रुव tmpp = सङ्कलय_Local_Tmp(m_stmtp, "ExpandSel_Lsb", lsbp);
                nodep->lsbp(नूतन AstVarRef{lfl, tmpp, VAccess::READ});
            }

            // Rebuild remaining expressions derived from LSB if needed, now using the temporary
            यदि (!lBitp) lBitp = newSelBitBit(nodep->lsbp());
            यदि (!lIdxp) lIdxp = new_Word_सूचकांक(nodep->lsbp());
            यदि (!mIdxp) mIdxp = mNeeded ? wordIdx(m_Msb_विस्थापनम्) : शून्यसूचकः;
            यदि (!hIdxp) hIdxp = hNeeded ? wordIdx(h_Msb_विस्थापनम्) : शून्यसूचकः;

            // Return word 'idxp' of 'fromp', without consuming 'idxp'
            ध्रुव स्वचालित wordSel = [&](Ast_ग्रन्थिः_व्यञ्जकम्* idxp) -> Ast_ग्रन्थिः_व्यञ्जकम्* {
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव clonep = idxp->clone_वृक्षः_विशुद्धम्(असत्य);
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव wordp = newWordSelWord(ffl, fromp, clonep);
                यदि (!clonep->backp()) VL_DO_DANGLING(clonep->delete_वृक्षः(), clonep);
                प्रत्यागम wordp;
            };

            // Construct term containing the bits of the low word - always needed
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lTermp = [&]() -> Ast_ग्रन्थिः_व्यञ्जकम्* {
                Ast_ग्रन्थिः_व्यञ्जकम्* lWordp = wordSel(lIdxp);
                यदि (nodep->isQuad()) lWordp = नूतन AstCCast{nfl, lWordp, nodep};
                प्रत्यागम नूतन Ast_सरणम्_R{nfl, lWordp, lBitp->clone_वृक्षः_विशुद्धम्(असत्य), nodep->विस्तारः()};
            }();

            // Construct term containing the bits of the middle word - if needed
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव mTermp = [&]() -> Ast_ग्रन्थिः_व्यञ्जकम्* {
                यदि (!mNeeded) प्रत्यागम शून्यसूचकः;

                Ast_ग्रन्थिः_व्यञ्जकम्* mWordp = wordSel(mIdxp);
                यदि (nodep->isQuad()) mWordp = नूतन AstCCast{nfl, mWordp, nodep};
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव mShiftp = नूतन AstSub{lfl, नूतन AstConst{lfl, VL_EDATASIZE},
                                                        lBitp->clone_वृक्षः_विशुद्धम्(असत्य)};
                // If we're selecting bit zero, then all 32 bits in the mid word
                // get shifted << by 32 bits, so ignore them.
                ध्रुव V3_सङ्ख्या zero{nodep, long_Or_Quad_विस्तारः(nodep)};
                प्रत्यागम नूतन AstCond{
                    nfl,
                    // lsb % VL_EDATASIZE == 0 ?
                    नूतन AstEq{nfl, नूतन AstConst{nfl, 0}, lBitp->clone_वृक्षः_विशुद्धम्(असत्य)},
                    // 0 :
                    नूतन AstConst{nfl, zero},
                    //  midword >> (VL_EDATASIZE - (lbs % VL_EDATASIZE))
                    नूतन Ast_सरणम्_L{nfl, mWordp, mShiftp, nodep->विस्तारः()}};
            }();

            // Construct term containing the bits of the high word - if needed
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव hTermp = [&]() -> Ast_ग्रन्थिः_व्यञ्जकम्* {
                यदि (!hNeeded) प्रत्यागम शून्यसूचकः;

                Ast_ग्रन्थिः_व्यञ्जकम्* hWordp = wordSel(hIdxp);
                hWordp = नूतन AstCCast{nfl, hWordp, nodep};
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव hShiftp = नूतन AstCond{
                    nfl,
                    // lsb % VL_EDATASIZE == 0 ?
                    नूतन AstEq{nfl, नूतन AstConst{nfl, 0}, lBitp->clone_वृक्षः_विशुद्धम्(असत्य)},
                    // VL_EDATASIZE :
                    नूतन AstConst{lfl, VL_EDATASIZE},
                    // 64 - (lbs % VL_EDATASIZE)
                    नूतन AstSub{lfl, नूतन AstConst{lfl, 64}, lBitp->clone_वृक्षः_विशुद्धम्(असत्य)}};
                प्रत्यागम नूतन Ast_सरणम्_L{nfl, hWordp, hShiftp, nodep->विस्तारः()};
            }();

            // Delete parts not captured during construction
            VL_DO_DANGLING(lBitp->delete_वृक्षः(), lBitp);
            VL_DO_DANGLING(lIdxp->delete_वृक्षः(), lIdxp);
            यदि (mIdxp) VL_DO_DANGLING(mIdxp->delete_वृक्षः(), mIdxp);
            यदि (hIdxp) VL_DO_DANGLING(hIdxp->delete_वृक्षः(), hIdxp);

            // Or reduce the terms
            Ast_ग्रन्थिः_व्यञ्जकम्* newp = lTermp;
            यदि (mTermp) newp = नूतन AstOr{nfl, mTermp, newp};
            यदि (hTermp) newp = नूतन AstOr{nfl, hTermp, newp};
            newp->dtypeFrom(nodep);
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        } अन्यथा {  // Long/Quad from Long/Quad
            // No isImpure() check - can handle side effects in below
            did_आदर्शः(VExpand_आदर्शः::SEL_FROM_NARROW, nodep);
            File_पङ्क्तिः* ध्रुव fl = nodep->fileline();
            Ast_ग्रन्थिः_व्यञ्जकम्* fromp = nodep->fromp()->unlink_Fr_पृष्ठभागः();
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lsbp = nodep->lsbp()->unlink_Fr_पृष्ठभागः();
            यदि (nodep->isQuad() && !fromp->isQuad()) fromp = नूतन AstCCast{fl, fromp, nodep};
            // {large}>>32 requires 64-bit shift operation; then cast
            Ast_ग्रन्थिः_व्यञ्जकम्* newp = नूतन Ast_सरणम्_R{fl, fromp, lsbp, fromp->विस्तारः()};
            newp->dtypeFrom(fromp);
            यदि (!nodep->isQuad() && fromp->isQuad()) newp = नूतन AstCCast{fl, newp, nodep};
            newp->dtypeFrom(nodep);
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        }
    }

    तार्किक expandWide(Ast_ग्रन्थिः_नियोजय* nodep, AstSel* rhsp) {
        UASSERT_OBJ(nodep->विस्तारः_Min() == rhsp->विस्तारः_Const(), nodep, "Width mismatch");
        यदि (!doExpandWide(nodep)) प्रत्यागम असत्य;

        // Simplify the index, in case it becomes a constant
        V3Const::constifyEditCpp(rhsp->lsbp());

        // If it's a constant select and aligned, we can just copy the words
        यदि (ध्रुव AstConst* ध्रुव lsbConstp = VN_CAST(rhsp->lsbp(), Const)) {
            ध्रुव uint32_t lsb = lsbConstp->गन्तव्यम्_UInt();
            यदि (VL_BITBIT_E(lsb) == 0) {
                did_आदर्शः(VExpand_आदर्शः::नियोजय_SEL_ALIGNED, nodep);
                ध्रुव uint32_t word = VL_BITWORD_E(lsb);
                प्रति (पूर्णाङ्क w = 0; w < nodep->विस्तारः_शब्दाः(); ++w) {
                    सङ्कलय_Word_नियोजय(nodep, w, newAstWordSelClone(rhsp->fromp(), w + word));
                }
                प्रत्यागम सत्य;
            }
        }

        did_आदर्शः(VExpand_आदर्शः::नियोजय_SEL_UNALIGNED, nodep);
        File_पङ्क्तिः* ध्रुव flp = rhsp->fileline();

        // Use fresh set of temporaries
        ++m_nTmps;

        // Compute word index of LSB, store to temporary if not constant
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव wordLsbp = rhsp->lsbp()->clone_वृक्षः_विशुद्धम्(असत्य);
        Ast_ग्रन्थिः_व्यञ्जकम्* wordIdxp = new_Word_सूचकांक(wordLsbp);
        यदि (!wordLsbp->backp()) VL_DO_DANGLING(wordLsbp->delete_वृक्षः(), wordLsbp);
        wordIdxp = V3Const::constifyEditCpp(wordIdxp);
        यदि (!VN_IS(wordIdxp, Const)) {
            AstVar* ध्रुव tmpp = सङ्कलय_Local_Tmp(nodep, "ExpandSel_WordIdx", wordIdxp);
            wordIdxp = नूतन AstVarRef{flp, tmpp, VAccess::READ};
        }

        // Compute shift amounts and mask, store to temporaries if not constants
        Ast_ग्रन्थिः_व्यञ्जकम्* loShftp = शून्यसूचकः;
        Ast_ग्रन्थिः_व्यञ्जकम्* hiShftp = शून्यसूचकः;
        Ast_ग्रन्थिः_व्यञ्जकम्* hiMaskp = शून्यसूचकः;
        यदि (ध्रुव AstConst* ध्रुव lsbConstp = VN_CAST(rhsp->lsbp(), Const)) {
            ध्रुव uint32_t bit_विस्थापनम् = VL_BITBIT_E(lsbConstp->गन्तव्यम्_UInt());
            // Must be unaligned, otherwise we would have handled it above
            UASSERT_OBJ(bit_विस्थापनम्, nodep, "Missed aligned wide select");
            loShftp = नूतन AstConst{flp, bit_विस्थापनम्};
            hiShftp = नूतन AstConst{flp, VL_EDATASIZE - bit_विस्थापनम्};
            hiMaskp = शून्यसूचकः;
        } अन्यथा {
            // Compute Low word shift amount: bottom bits of lsb index
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lsbp = rhsp->lsbp()->clone_वृक्षः_विशुद्धम्(असत्य);
            loShftp = नूतन AstAnd{flp, नूतन AstConst{flp, VL_SIZEBITS_E}, lsbp};
            AstVar* ध्रुव loTmpp = सङ्कलय_Local_Tmp(nodep, "ExpandSel_LoShift", loShftp);
            loShftp = नूतन AstVarRef{flp, loTmpp, VAccess::READ};

            // Compute if aligned: Low word shift amount is 0
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव zerop = नूतन AstConst{flp, 0};
            Ast_ग्रन्थिः_व्यञ्जकम्* alignedp = नूतन AstEq{flp, zerop, loShftp->clone_वृक्षः_विशुद्धम्(असत्य)};
            AstVar* ध्रुव alignedTmpp = सङ्कलय_Local_Tmp(nodep, "ExpandSel_Aligned", alignedp);
            alignedp = नूतन AstVarRef{flp, alignedTmpp, VAccess::READ};

            // Computed High word shift amount: 0 if aligned else VL_EDATASIZE - Low word shift
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव edsp = नूतन AstConst{flp, VL_EDATASIZE};
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव subp = नूतन AstSub{flp, edsp, loShftp->clone_वृक्षः_विशुद्धम्(असत्य)};
            hiShftp = नूतन AstCond{flp, alignedp, नूतन AstConst{flp, 0}, subp};
            AstVar* ध्रुव hiTmpp = सङ्कलय_Local_Tmp(nodep, "ExpandSel_HiShift", hiShftp);
            hiShftp = नूतन AstVarRef{flp, hiTmpp, VAccess::READ};

            // Compute the High word mask: 0 if aligned, ones otherwise
            hiMaskp = नूतन AstCond{flp, alignedp->clone_वृक्षः_विशुद्धम्(असत्य),
                                  नूतन AstConst{flp, AstConst::SizedEData{}, 0},
                                  नूतन AstConst{flp, AstConst::SizedEData{}, -1ULL}};
            AstVar* ध्रुव आवरणम्_Tmpp = सङ्कलय_Local_Tmp(nodep, "ExpandSel_HiMask", hiMaskp);
            hiMaskp = नूतन AstVarRef{flp, आवरणम्_Tmpp, VAccess::READ};
        }

        // Create each word of the selected result
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव fromp = rhsp->fromp();
        ध्रुव पूर्णाङ्क sel_शब्दाः = nodep->विस्तारः_शब्दाः();
        प्रति (पूर्णाङ्क w = 0; w < sel_शब्दाः; ++w) {
            // Grab bits from word 'VL_BITWORD_E(lsb) + w'
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव loWordp = newWordSelWord(fromp->fileline(), fromp, wordIdxp, w);
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव loShftClonep = loShftp->clone_वृक्षः_विशुद्धम्(असत्य);
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lop = नूतन Ast_सरणम्_R{flp, loWordp, loShftClonep, VL_EDATASIZE};
            // Grab bits from word 'VL_BITWORD_E(lsb) + w + 1'
            Ast_ग्रन्थिः_व्यञ्जकम्* hiWordp = newWordSelWord(fromp->fileline(), fromp, wordIdxp, w + 1);
            // For the last word of the result, avoid an OOB access when the Sel is not OOB
            यदि (w == sel_शब्दाः - 1) {
                ध्रुव uint32_t max = fromp->विस्तारः_शब्दाः() - w - 1;
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव maxp = नूतन AstConst{flp, max};
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव condp = नूतन AstGte{flp, wordIdxp->clone_वृक्षः_विशुद्धम्(असत्य), maxp};
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव zerop = नूतन AstConst{flp, AstConst::SizedEData{}, 0};
                hiWordp = नूतन AstCond{flp, condp, zerop, hiWordp};
            }
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव hiShftClonep = hiShftp->clone_वृक्षः_विशुद्धम्(असत्य);
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव hiPartp = नूतन Ast_सरणम्_L{flp, hiWordp, hiShftClonep, VL_EDATASIZE};
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव hip
                = hiMaskp ? नूतन AstAnd{flp, hiPartp, hiMaskp->clone_वृक्षः_विशुद्धम्(असत्य)} : hiPartp;
            // Combine them
            सङ्कलय_Word_नियोजय(nodep, w, नूतन AstOr{nodep->fileline(), hip, lop});
        }

        // Delete parts not captured during construction
        यदि (!wordIdxp->backp()) VL_DO_DANGLING(wordIdxp->delete_वृक्षः(), wordIdxp);
        यदि (!loShftp->backp()) VL_DO_DANGLING(loShftp->delete_वृक्षः(), loShftp);
        यदि (!hiShftp->backp()) VL_DO_DANGLING(hiShftp->delete_वृक्षः(), hiShftp);
        यदि (hiMaskp && !hiMaskp->backp()) VL_DO_DANGLING(hiMaskp->delete_वृक्षः(), hiMaskp);

        प्रत्यागम सत्य;
    }

    तार्किक expandLhs(Ast_ग्रन्थिः_नियोजय* nodep, AstSel* lhsp) {
        // Possibilities
        //      destp: wide or narrow
        //      rhsp:  wide (destp must be wide), narrow, or 1 bit wide
        //      rhsp:  may be allones and can remove AND NOT gate
        //      lsbp:  constant or variable
        // Yuk.
        यदि (isImpure(nodep)) प्रत्यागम असत्य;
        File_पङ्क्तिः* ध्रुव nfl = nodep->fileline();
        File_पङ्क्तिः* ध्रुव lfl = lhsp->fileline();
        ध्रुव तार्किक destwide = lhsp->fromp()->isWide();
        ध्रुव तार्किक ones = nodep->rhsp()->isAllOnesV();
        यदि (VN_IS(lhsp->lsbp(), Const)) {
            // The code should work without this constant test, but it won't
            // constify as nicely as we'd like.
            Ast_ग्रन्थिः_व्यञ्जकम्* rhsp = nodep->rhsp()->unlink_Fr_पृष्ठभागः();
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव destp = lhsp->fromp()->unlink_Fr_पृष्ठभागः();
            ध्रुव पूर्णाङ्क lsb = lhsp->lsbConst();
            ध्रुव पूर्णाङ्क msb = lhsp->msbConst();
            V3_सङ्ख्या maskset{nodep, destp->विस्तारः_Min()};
            maskset.समुच्चयः_आवरणम्(msb + 1 - lsb, lsb);
            V3_सङ्ख्या maskold{nodep, destp->विस्तारः_Min()};
            maskold.opNot(maskset);
            यदि (destwide) {
                did_आदर्शः(VExpand_आदर्शः::नियोजय_गन्तव्यम्_SEL_CONST_WIDE, nodep);
                प्रति (पूर्णाङ्क w = 0; w < destp->विस्तारः_शब्दाः(); ++w) {
                    यदि (w >= VL_BITWORD_E(lsb) && w <= VL_BITWORD_E(msb)) {
                        // else we would just be setting it to the same exact value
                        Ast_ग्रन्थिः_व्यञ्जकम्* oldvalp = newAstWordSelClone(destp, w);
                        fixCloneLvalue(oldvalp);
                        यदि (!ones) {
                            oldvalp = नूतन AstAnd{
                                lfl,
                                नूतन AstConst{lfl, AstConst::SizedEData{}, maskold.edataWord(w)},
                                oldvalp};
                        }

                        // Appropriate word of new value to insert:
                        Ast_ग्रन्थिः_व्यञ्जकम्* newp = new_Word_Grab_सरणम्(lfl, w, rhsp, lsb);

                        // Apply cleaning at the top word of the destination
                        // (no cleaning to do if dst's width is a whole number
                        // of words).
                        यदि (w == destp->विस्तारः_शब्दाः() - 1 && VL_BITBIT_E(destp->विस्तारः_Min()) != 0) {
                            V3_सङ्ख्या cleanmask{nodep, VL_EDATASIZE};
                            cleanmask.समुच्चयः_आवरणम्(VL_BITBIT_E(destp->विस्तारः_Min()));
                            newp = नूतन AstAnd{lfl, newp, नूतन AstConst{lfl, cleanmask}};
                        }
                        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव orp
                            = V3Const::constifyEditCpp(नूतन AstOr{lfl, oldvalp, newp});
                        सङ्कलय_Word_नियोजय(nodep, w, destp, orp);
                    }
                }
                VL_DO_DANGLING(rhsp->delete_वृक्षः(), rhsp);
                VL_DO_DANGLING(destp->delete_वृक्षः(), destp);
            } अन्यथा {
                did_आदर्शः(VExpand_आदर्शः::नियोजय_गन्तव्यम्_SEL_CONST_NARROW, nodep);
                यदि (destp->isQuad() && !rhsp->isQuad()) rhsp = नूतन AstCCast{nfl, rhsp, nodep};
                Ast_ग्रन्थिः_व्यञ्जकम्* oldvalp = destp->clone_वृक्षः_विशुद्धम्(सत्य);
                fixCloneLvalue(oldvalp);
                यदि (!ones) oldvalp = नूतन AstAnd{lfl, नूतन AstConst{lfl, maskold}, oldvalp};

                // The bit-select can refer to bits outside the width of nodep
                // which we aren't allowed to assign to.  This is a mask of the
                // valid range of nodep which we apply to the new shifted RHS.
                V3_सङ्ख्या cleanmask{nodep, destp->विस्तारः_Min()};
                cleanmask.समुच्चयः_आवरणम्(destp->विस्तारः_Min());
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव shifted = नूतन Ast_सरणम्_L{
                    lfl, rhsp, नूतन AstConst{lfl, static_cast<uint32_t>(lsb)}, destp->विस्तारः()};
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव cleaned
                    = नूतन AstAnd{lfl, shifted, नूतन AstConst{lfl, cleanmask}};
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव orp
                    = V3Const::constifyEditCpp(नूतन AstOr{lfl, oldvalp, cleaned});
                निवेशय_Before(nodep, नूतन Ast_नियोजय{nfl, destp, orp});
            }
            प्रत्यागम सत्य;
        } अन्यथा {  // non-const select offset
            यदि (destwide && lhsp->विस्तारः_Const() == 1) {
                did_आदर्शः(VExpand_आदर्शः::नियोजय_गन्तव्यम्_SEL_VAR_WIDE_ONE_BIT, nodep);
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rhsp = nodep->rhsp()->unlink_Fr_पृष्ठभागः();
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव destp = lhsp->fromp()->unlink_Fr_पृष्ठभागः();
                Ast_ग्रन्थिः_व्यञ्जकम्* oldvalp
                    = newWordSelBit(lfl, destp->clone_वृक्षः_विशुद्धम्(असत्य), lhsp->lsbp());
                fixCloneLvalue(oldvalp);
                यदि (!ones) {
                    oldvalp = नूतन AstAnd{
                        lfl,
                        नूतन AstNot{
                            lfl, नूतन Ast_सरणम्_L{lfl, नूतन AstConst{nfl, 1},
                                               // newSelBitBit may exceed the MSB of this variable.
                                               // That's ok as we'd just AND with a larger value,
                                               // but oldval would clip the upper bits to sanity
                                               newSelBitBit(lhsp->lsbp()), VL_EDATASIZE}},
                        oldvalp};
                }
                // Restrict the shift amount to 0-31, see bug804.
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव shiftp = नूतन AstAnd{nfl, lhsp->lsbp()->clone_वृक्षः_विशुद्धम्(सत्य),
                                                       नूतन AstConst{nfl, VL_EDATASIZE - 1}};
                Ast_ग्रन्थिः* ध्रुव newp = नूतन Ast_नियोजय{
                    nfl, newWordSelBit(nfl, destp, lhsp->lsbp()),
                    नूतन AstOr{lfl, oldvalp, नूतन Ast_सरणम्_L{lfl, rhsp, shiftp, VL_EDATASIZE}}};
                निवेशय_Before(nodep, newp);
                प्रत्यागम सत्य;
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
                प्रत्यागम असत्य;
            } अन्यथा {
                did_आदर्शः(VExpand_आदर्शः::नियोजय_गन्तव्यम्_SEL_VAR_NARROW, nodep);
                // nodep->dumpTree("-  old: ");
                Ast_ग्रन्थिः_व्यञ्जकम्* rhsp = nodep->rhsp()->unlink_Fr_पृष्ठभागः();
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव destp = lhsp->fromp()->unlink_Fr_पृष्ठभागः();
                Ast_ग्रन्थिः_व्यञ्जकम्* oldvalp = destp->clone_वृक्षः_विशुद्धम्(सत्य);
                fixCloneLvalue(oldvalp);

                V3_सङ्ख्या maskwidth{nodep, destp->विस्तारः_Min()};
                maskwidth.समुच्चयः_आवरणम्(lhsp->विस्तारः_Const());

                यदि (destp->isQuad() && !rhsp->isQuad()) rhsp = नूतन AstCCast{nfl, rhsp, nodep};
                यदि (!ones) {
                    oldvalp = नूतन AstAnd{
                        lfl,
                        नूतन AstNot{lfl, नूतन Ast_सरणम्_L{lfl, नूतन AstConst{nfl, maskwidth},
                                                      lhsp->lsbp()->clone_वृक्षः_विशुद्धम्(सत्य),
                                                      destp->विस्तारः()}},
                        oldvalp};
                }
                Ast_ग्रन्थिः_व्यञ्जकम्* newp
                    = नूतन Ast_सरणम्_L{lfl, rhsp, lhsp->lsbp()->clone_वृक्षः_विशुद्धम्(सत्य), destp->विस्तारः()};
                // Apply cleaning to the new value being inserted.  Mask is
                // slightly wider than necessary to avoid an AND with all ones
                // being optimized out.  No need to clean if destp is
                // quad-sized as there are no extra bits to contaminate
                यदि (destp->विस्तारः_Min() != 64) {
                    V3_सङ्ख्या cleanmask{nodep, destp->विस्तारः_Min() + 1};
                    cleanmask.समुच्चयः_आवरणम्(destp->विस्तारः_Min());
                    newp = नूतन AstAnd{lfl, newp, नूतन AstConst{lfl, cleanmask}};
                }

                निवेशय_Before(nodep, नूतन Ast_नियोजय{nfl, destp, नूतन AstOr{lfl, oldvalp, newp}});
                प्रत्यागम सत्य;
            }
        }
    }

    शून्य दर्शय(AstConcat* nodep) override {
        यदि (nodep->user1_समुच्चयः_Once()) प्रत्यागम;  // Process once
        iterateChildren(nodep);
        यदि (nodep->isWide()) {
            // See under ASSIGN(WIDE)
        } अन्यथा {
            // No isImpure() check - can handle side effects in below
            did_आदर्शः(VExpand_आदर्शः::CONCAT, nodep);
            File_पङ्क्तिः* ध्रुव fl = nodep->fileline();
            Ast_ग्रन्थिः_व्यञ्जकम्* lhsp = nodep->lhsp()->unlink_Fr_पृष्ठभागः();
            Ast_ग्रन्थिः_व्यञ्जकम्* rhsp = nodep->rhsp()->unlink_Fr_पृष्ठभागः();
            ध्रुव uint32_t rhsshift = rhsp->विस्तारः_Min();
            यदि (nodep->isQuad() && !lhsp->isQuad()) lhsp = नूतन AstCCast{fl, lhsp, nodep};
            यदि (nodep->isQuad() && !rhsp->isQuad()) rhsp = नूतन AstCCast{fl, rhsp, nodep};
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव newp = नूतन AstOr{
                fl, नूतन Ast_सरणम्_L{fl, lhsp, नूतन AstConst{fl, rhsshift}, nodep->विस्तारः()}, rhsp};
            newp->dtypeFrom(nodep);  // Unsigned
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        }
    }
    तार्किक expandWide(Ast_ग्रन्थिः_नियोजय* nodep, AstConcat* rhsp) {
        यदि (!doExpandWide(rhsp)) प्रत्यागम असत्य;
        did_आदर्शः(VExpand_आदर्शः::नियोजय_CONCAT, nodep);
        File_पङ्क्तिः* ध्रुव fl = rhsp->fileline();
        // Lhs or Rhs may be word, long, or quad.
        // newAstWordSelClone nicely abstracts the difference.
        ध्रुव पूर्णाङ्क rhsshift = rhsp->rhsp()->विस्तारः_Min();
        // Sometimes doing the words backwards is preferable.
        // When we have x={x,foo} backwards is better, when x={foo,x} forward is better
        // However V3Subst tends to rip this up, so not worth optimizing now.
        प्रति (पूर्णाङ्क w = 0; w < rhsp->विस्तारः_शब्दाः(); ++w) {
            सङ्कलय_Word_नियोजय(nodep, w,
                          नूतन AstOr{fl, new_Word_Grab_सरणम्(fl, w, rhsp->lhsp(), rhsshift),
                                    newAstWordSelClone(rhsp->rhsp(), w)});
        }
        प्रत्यागम सत्य;
    }

    शून्य दर्शय(AstReplicate* nodep) override {
        यदि (nodep->user1_समुच्चयः_Once()) प्रत्यागम;  // Process once
        iterateChildren(nodep);
        यदि (nodep->isWide()) {
            // See under ASSIGN(WIDE)
        } अन्यथा {
            यदि (isImpure(nodep)) प्रत्यागम;
            File_पङ्क्तिः* ध्रुव fl = nodep->fileline();
            Ast_ग्रन्थिः_व्यञ्जकम्* lhsp = nodep->srcp()->unlink_Fr_पृष्ठभागः();
            Ast_ग्रन्थिः_व्यञ्जकम्* newp;
            ध्रुव पूर्णाङ्क lhswidth = lhsp->विस्तारः_Min();
            यदि (lhswidth == 1) {
                did_आदर्शः(VExpand_आदर्शः::REPLICATE_ONE_BIT, nodep);
                newp = नूतन AstNegate{fl, lhsp};
            } अन्यथा {
                did_आदर्शः(VExpand_आदर्शः::REPLICATE, nodep);
                ध्रुव AstConst* ध्रुव constp = VN_AS(nodep->countp(), Const);
                UASSERT_OBJ(constp, nodep,
                            "Replication value isn't a constant.  Checked earlier!");
                ध्रुव uint32_t times = constp->गन्तव्यम्_UInt();
                यदि (nodep->isQuad() && !lhsp->isQuad()) lhsp = नूतन AstCCast{fl, lhsp, nodep};
                newp = lhsp->clone_वृक्षः_विशुद्धम्(सत्य);
                प्रति (अचिह्नित repnum = 1; repnum < times; repnum++) {
                    ध्रुव पूर्णाङ्क rhsshift = repnum * lhswidth;
                    newp = नूतन AstOr{
                        fl,
                        नूतन Ast_सरणम्_L{fl, lhsp->clone_वृक्षः_विशुद्धम्(सत्य),
                                      नूतन AstConst{fl, static_cast<uint32_t>(rhsshift)},
                                      nodep->विस्तारः()},
                        newp};
                    newp->dtypeFrom(nodep);  // Unsigned
                }
                VL_DO_DANGLING(lhsp->delete_वृक्षः(), lhsp);  // Never used
            }
            newp->dtypeFrom(nodep);  // Unsigned
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        }
    }
    तार्किक expandWide(Ast_ग्रन्थिः_नियोजय* nodep, AstReplicate* rhsp) {
        यदि (!doExpandWide(rhsp)) प्रत्यागम असत्य;
        did_आदर्शः(VExpand_आदर्शः::नियोजय_REPLICATE, nodep);
        File_पङ्क्तिः* ध्रुव fl = nodep->fileline();
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lhsp = rhsp->srcp();
        ध्रुव पूर्णाङ्क lhswidth = lhsp->विस्तारः_Min();
        ध्रुव AstConst* ध्रुव constp = VN_AS(rhsp->countp(), Const);
        UASSERT_OBJ(constp, rhsp, "Replication value isn't a constant.  Checked earlier!");
        ध्रुव uint32_t times = constp->गन्तव्यम्_UInt();
        प्रति (पूर्णाङ्क w = 0; w < rhsp->विस्तारः_शब्दाः(); ++w) {
            Ast_ग्रन्थिः_व्यञ्जकम्* newp;
            यदि (lhswidth == 1) {
                newp = नूतन AstNegate{fl, lhsp->clone_वृक्षः_विशुद्धम्(सत्य)};
                // Replicate always unsigned
                newp->dtype_Chg_विस्तारः_Signed(VL_EDATASIZE, VL_EDATASIZE, VSigning::UNSIGNED);
            } अन्यथा {
                newp = newAstWordSelClone(lhsp, w);
                File_पङ्क्तिः* ध्रुव rfl = rhsp->fileline();
                प्रति (अचिह्नित repnum = 1; repnum < times; repnum++) {
                    newp = नूतन AstOr{fl, new_Word_Grab_सरणम्(rfl, w, lhsp, lhswidth * repnum), newp};
                }
            }
            सङ्कलय_Word_नियोजय(nodep, w, newp);
        }
        प्रत्यागम सत्य;
    }

    शून्य दर्शय_Eq_Neq(Ast_ग्रन्थिः_Biop* nodep) {
        यदि (nodep->user1_समुच्चयः_Once()) प्रत्यागम;  // Process once
        iterateChildren(nodep);
        यदि (nodep->lhsp()->isWide()
            && !(VN_IS(nodep->lhsp()->dtypep()->skipRefp(), प्रवाहः_DType)
                 || VN_IS(nodep->rhsp()->dtypep()->skipRefp(), प्रवाहः_DType))) {
            यदि (isImpure(nodep)) प्रत्यागम;
            यदि (!doExpandWide(nodep->lhsp())) प्रत्यागम;
            यदि (!doExpandWide(nodep->rhsp())) प्रत्यागम;
            did_आदर्शः(VN_IS(nodep, Neq) ? VExpand_आदर्शः::WORDIZE_NEQ
                                         : VExpand_आदर्शः::WORDIZE_EQ,
                       nodep);
            // -> (0=={or{for each_word{WORDSEL(lhs,#)^WORDSEL(rhs,#)}}}
            File_पङ्क्तिः* ध्रुव fl = nodep->fileline();
            Ast_ग्रन्थिः_व्यञ्जकम्* newp = शून्यसूचकः;
            प्रति (पूर्णाङ्क w = 0; w < nodep->lhsp()->विस्तारः_शब्दाः(); ++w) {
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव eqp = नूतन AstXor{fl, newAstWordSelClone(nodep->lhsp(), w),
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
    शून्य दर्शय(AstEq* nodep) override { दर्शय_Eq_Neq(nodep); }
    शून्य दर्शय(AstNeq* nodep) override { दर्शय_Eq_Neq(nodep); }

    शून्य दर्शय(AstRedOr* nodep) override {
        यदि (nodep->user1_समुच्चयः_Once()) प्रत्यागम;  // Process once
        iterateChildren(nodep);
        File_पङ्क्तिः* ध्रुव fl = nodep->fileline();
        यदि (nodep->lhsp()->isWide()) {
            यदि (isImpure(nodep)) प्रत्यागम;
            यदि (!doExpandWide(nodep->lhsp())) प्रत्यागम;
            did_आदर्शः(VExpand_आदर्शः::WORDIZE_REDOR, nodep);
            // -> (0!={or{for each_word{WORDSEL(lhs,#)}}}
            Ast_ग्रन्थिः_व्यञ्जकम्* newp = शून्यसूचकः;
            प्रति (पूर्णाङ्क w = 0; w < nodep->lhsp()->विस्तारः_शब्दाः(); ++w) {
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव eqp = newAstWordSelClone(nodep->lhsp(), w);
                newp = newp ? नूतन AstOr{fl, newp, eqp} : eqp;
            }
            newp = नूतन AstNeq{fl, नूतन AstConst{fl, AstConst::SizedEData{}, 0}, newp};
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        } अन्यथा {
            // No isImpure() check - can handle side effects in below
            did_आदर्शः(VExpand_आदर्शः::REDOR_गन्तव्यम्_NEQ, nodep);
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lhsp = nodep->lhsp()->unlink_Fr_पृष्ठभागः();
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव newp = नूतन AstNeq{
                fl, नूतन AstConst{fl, AstConst::Widthed_मानम्{}, long_Or_Quad_विस्तारः(nodep), 0}, lhsp};
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        }
    }
    शून्य दर्शय(AstRedAnd* nodep) override {
        यदि (nodep->user1_समुच्चयः_Once()) प्रत्यागम;  // Process once
        iterateChildren(nodep);
        File_पङ्क्तिः* ध्रुव fl = nodep->fileline();
        यदि (nodep->lhsp()->isWide()) {
            यदि (isImpure(nodep)) प्रत्यागम;
            यदि (!doExpandWide(nodep->lhsp())) प्रत्यागम;
            did_आदर्शः(VExpand_आदर्शः::WORDIZE_REDAND, nodep);
            // -> (0!={and{for each_word{WORDSEL(lhs,#)}}}
            Ast_ग्रन्थिः_व्यञ्जकम्* newp = शून्यसूचकः;
            प्रति (पूर्णाङ्क w = 0; w < nodep->lhsp()->विस्तारः_शब्दाः(); ++w) {
                Ast_ग्रन्थिः_व्यञ्जकम्* eqp = newAstWordSelClone(nodep->lhsp(), w);
                यदि (w == nodep->lhsp()->विस्तारः_शब्दाः() - 1) {
                    // Rather than doing a (slowish) ==##, we OR in the
                    // bits that aren't part of the mask
                    eqp = नूतन AstOr{fl, नूतन AstConst{fl, not_Wide_आवरणम्(nodep->lhsp())},
                                    // Bug in cppcheck
                                    // cppcheck-suppress memleak
                                    eqp};
                }
                newp = newp ? नूतन AstAnd{fl, newp, eqp} : eqp;
            }
            newp = नूतन AstEq{fl, नूतन AstConst{fl, AstConst::SizedEData{}, VL_आवरणम्_E(VL_EDATASIZE)},
                             newp};
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        } अन्यथा {
            // No isImpure() check - can handle side effects in below
            did_आदर्शः(VExpand_आदर्शः::REDAND_गन्तव्यम्_EQ, nodep);
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lhsp = nodep->lhsp()->unlink_Fr_पृष्ठभागः();
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव newp = नूतन AstEq{fl, नूतन AstConst{fl, word_आवरणम्(lhsp)}, lhsp};
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        }
    }
    शून्य दर्शय(AstRedXor* nodep) override {
        यदि (nodep->user1_समुच्चयः_Once()) प्रत्यागम;  // Process once
        iterateChildren(nodep);
        यदि (nodep->lhsp()->isWide()) {
            यदि (isImpure(nodep)) प्रत्यागम;
            यदि (!doExpandWide(nodep->lhsp())) प्रत्यागम;
            did_आदर्शः(VExpand_आदर्शः::WORDIZE_REDXOR, nodep);
            // -> (0!={redxor{for each_word{XOR(WORDSEL(lhs,#))}}}
            File_पङ्क्तिः* ध्रुव fl = nodep->fileline();
            Ast_ग्रन्थिः_व्यञ्जकम्* newp = शून्यसूचकः;
            प्रति (पूर्णाङ्क w = 0; w < nodep->lhsp()->विस्तारः_शब्दाः(); ++w) {
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव eqp = newAstWordSelClone(nodep->lhsp(), w);
                newp = newp ? नूतन AstXor{fl, newp, eqp} : eqp;
            }
            newp = नूतन AstRedXor{fl, newp};
            UINFO(8, "    Wordize REDXORnew " << newp);
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        }
        // We don't reduce non-wide XORs, as its more efficient to use a temp register,
        // which the inlined function does nicely.
    }

    शून्य दर्शय(Ast_ग्रन्थिः_Stmt* nodep) override {
        यदि (nodep->user1_समुच्चयः_Once()) प्रत्यागम;  // Process once
        VL_RESTORER(m_stmtp);
        m_stmtp = nodep;
        iterateChildren(nodep);
    }
    शून्य दर्शय(Ast_ग्रन्थिः_नियोजय* nodep) override {
        यदि (nodep->user1_समुच्चयः_Once()) प्रत्यागम;  // Process once
        यदि (VN_IS(nodep->dtypep()->skipRefp(), UnpackArrayDType)) {
            प्रत्यागम;  // Skip for UnpackArrayDType
        }

        VL_RESTORER(m_stmtp);
        m_stmtp = nodep;
        iterateChildren(nodep);
        तार्किक did = असत्य;
        यदि (nodep->isWide()  //
            && ((VN_IS(nodep->lhsp(), VarRef) || VN_IS(nodep->lhsp(), ArraySel)))
            && !AstVar::scVarRecurse(nodep->lhsp())  // Need special function for SC
            && !AstVar::scVarRecurse(nodep->rhsp())) {
            यदि (AstConst* ध्रुव rhsp = VN_CAST(nodep->rhsp(), Const)) {
                did = expandWide(nodep, rhsp);
            } अन्यथा यदि (AstVarRef* ध्रुव rhsp = VN_CAST(nodep->rhsp(), VarRef)) {
                did = expandWide(nodep, rhsp);
            } अन्यथा यदि (AstSel* ध्रुव rhsp = VN_CAST(nodep->rhsp(), Sel)) {
                did = expandWide(nodep, rhsp);
            } अन्यथा यदि (AstArraySel* ध्रुव rhsp = VN_CAST(nodep->rhsp(), ArraySel)) {
                did = expandWide(nodep, rhsp);
            } अन्यथा यदि (AstConcat* ध्रुव rhsp = VN_CAST(nodep->rhsp(), Concat)) {
                did = expandWide(nodep, rhsp);
            } अन्यथा यदि (AstExtend* ध्रुव rhsp = VN_CAST(nodep->rhsp(), Extend)) {
                did = expandWide(nodep, rhsp);
            } अन्यथा यदि (AstReplicate* ध्रुव rhsp = VN_CAST(nodep->rhsp(), Replicate)) {
                did = expandWide(nodep, rhsp);
            } अन्यथा यदि (AstAnd* ध्रुव rhsp = VN_CAST(nodep->rhsp(), And)) {
                did = expandWide(nodep, rhsp);
            } अन्यथा यदि (AstOr* ध्रुव rhsp = VN_CAST(nodep->rhsp(), Or)) {
                did = expandWide(nodep, rhsp);
            } अन्यथा यदि (AstNot* ध्रुव rhsp = VN_CAST(nodep->rhsp(), Not)) {
                did = expandWide(nodep, rhsp);
            } अन्यथा यदि (AstXor* ध्रुव rhsp = VN_CAST(nodep->rhsp(), Xor)) {
                did = expandWide(nodep, rhsp);
            } अन्यथा यदि (Ast_सरणम्_L* ध्रुव rhsp = VN_CAST(nodep->rhsp(), सरणम्_L)) {
                did = expand_Wide_सरणम्(nodep, rhsp, /* isLeftShift: */ सत्य);
            } अन्यथा यदि (Ast_सरणम्_R* ध्रुव rhsp = VN_CAST(nodep->rhsp(), सरणम्_R)) {
                did = expand_Wide_सरणम्(nodep, rhsp, /* isLeftShift: */ असत्य);
            } अन्यथा यदि (AstCond* ध्रुव rhsp = VN_CAST(nodep->rhsp(), Cond)) {
                did = expandWide(nodep, rhsp);
            }
        } अन्यथा यदि (AstSel* ध्रुव lhsp = VN_CAST(nodep->lhsp(), Sel)) {
            did = expandLhs(nodep, lhsp);
        }
        // Cleanup common code
        यदि (did) VL_DO_DANGLING(nodep->unlink_Fr_पृष्ठभागः()->delete_वृक्षः(), nodep);
    }

    //--------------------
    शून्य दर्शय(AstVar*) override {}  // Don't hit varrefs under vars
    शून्य दर्शय(Ast_ग्रन्थिः* nodep) override { iterateChildren(nodep); }

सार्वजनिक:
    // CONSTRUCTORS
    स्पष्ट ExpandVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~ExpandVisitor() override {
        V3Stats::सङ्कलय_Stat("Optimizations, Expand, expanded wides", m_statWides);
        V3Stats::सङ्कलय_Stat("Optimizations, Expand, expanded wide words", m_stat_Wide_शब्दाः);
        V3Stats::सङ्कलय_Stat("Optimizations, Expand, not expanded impure", m_statImpure);
        V3Stats::सङ्कलय_Stat("Optimizations, Expand, not expanded limit", m_statWideLimited);
        V3Stats::सङ्कलय_Stat("Optimizations, Expand, temporaries created", m_statTemps);
        ध्रुव स्वचालित emitStat = [एतत्](VExpand_आदर्शः आदर्शः_2) {
            std::string str{आदर्शः_2.ascii()};
            std::transform(str.begin(), str.अन्तः_2(), str.begin(), [](अचिह्नित वर्ण c) {  //
                प्रत्यागम c == '_' ? ' ' : std::tolower(c);
            });
            V3Stats::सङ्कलय_Stat("Optimizations, Expand, pattern " + str, m_statPatterns[आदर्शः_2]);
        };
#परिभाषय EXPAND_आदर्शः_EMIT_STAT(id) emitStat(VExpand_आदर्शः::id);
        FOR_EACH_EXPAND_आदर्शः(EXPAND_आदर्शः_EMIT_STAT)
#अपरिभाषय EXPAND_आदर्शः_EMIT_STAT
    }
};

//######################################################################
// Expand class functions

शून्य V3Expand::expandAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        ध्रुव ExpandOkVisitor okVisitor{nodep};
        ExpandVisitor{nodep};
    }  // Destruct before checking
    V3Global::dump_Check_Global_वृक्षः("expand", 0, dump_वृक्षः_Either_Level() >= 3);
}
