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

#అన్తర్భావ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#అన్తర్భావ "V3Expand.h"

#అన్తర్భావ "V3Const.h"
#అన్తర్భావ "V3Stats.h"

#అన్తర్భావ <algorithm>
#అన్తర్భావ <array>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// The distinct expansion patterns, for statistics. One entry for each
// unique transformation ExpandVisitor applies. Keep sorted.

// clang-format off
#నిర్వచించు FOR_EACH_EXPAND_నమూనా(macro) \
    macro(కేటాయించు_AND) \
    macro(కేటాయించు_ARRAYSEL) \
    macro(కేటాయించు_CONCAT) \
    macro(కేటాయించు_COND) \
    macro(కేటాయించు_CONST) \
    macro(కేటాయించు_EXTEND) \
    macro(కేటాయించు_NOT) \
    macro(కేటాయించు_OR) \
    macro(కేటాయించు_REPLICATE) \
    macro(కేటాయించు_SEL_ALIGNED) \
    macro(కేటాయించు_SEL_UNALIGNED) \
    macro(కేటాయించు_SHIFTL) \
    macro(కేటాయించు_SHIFTR) \
    macro(కేటాయించు_గమ్యం_SEL_CONST_NARROW) \
    macro(కేటాయించు_గమ్యం_SEL_CONST_WIDE) \
    macro(కేటాయించు_గమ్యం_SEL_VAR_NARROW) \
    macro(కేటాయించు_గమ్యం_SEL_VAR_WIDE_ONE_బిట్టు) \
    macro(కేటాయించు_VARREF) \
    macro(కేటాయించు_XOR) \
    macro(CONCAT) \
    macro(REDAND_గమ్యం_EQ) \
    macro(REDOR_గమ్యం_NEQ) \
    macro(REPLICATE) \
    macro(REPLICATE_ONE_బిట్టు) \
    macro(SEL_FROM_NARROW) \
    macro(SEL_FROM_WIDE) \
    macro(WORDIZE_EQ) \
    macro(WORDIZE_NEQ) \
    macro(WORDIZE_REDAND) \
    macro(WORDIZE_REDOR) \
    macro(WORDIZE_REDXOR)
// clang-format on

తరగతి VExpand_నమూనా final {
బహిరంగ:
    గణన en : uint8_t {
#నిర్వచించు EXPAND_నమూనా_ID(id) id,
        FOR_EACH_EXPAND_నమూనా(EXPAND_నమూనా_ID)
#నిర్వచనం_రద్దు EXPAND_నమూనా_ID
            ENUM_చివర
    };
    గణన en m_e;
    స్థిరరాశి అక్షరం* ascii() స్థిరరాశి {
        స్థిర స్థిరరాశి అక్షరం* స్థిరరాశి పేర్లు[] = {
#నిర్వచించు EXPAND_నమూనా_NAME(id) #id,
            FOR_EACH_EXPAND_నమూనా(EXPAND_నమూనా_NAME)
#నిర్వచనం_రద్దు EXPAND_నమూనా_NAME
                "_ENUM_END"  //
        };
        తిరిగి పేర్లు[m_e];
    }

    // cppcheck-suppress noExplicitConstructor
    VExpand_నమూనా(en _e)
        : m_e{_e} {}
    క్రియాకారకం en() స్థిరరాశి { తిరిగి m_e; }
};

//######################################################################
// Find nodes with side effects, to mark as non-expandable

తరగతి ExpandOkVisitor final : బహిరంగ VNVisitorConst {
    // NODE STATE
    //  AstNode::user2()        -> bool.  Is pure (along with all children)
    స్థిరరాశి VNUser2_లోపల_Use m_inuser2;

    // STATE - for current visit position (use VL_RESTORER)
    // Tracks similar to AstNode::isTreePureRecurse(), but avoid O(n^2)
    // False = pure, as nodes that ExpandVisitor inserts preserve pureness
    తార్కిక m_isImpure = నిజం;  // Currently pure

    శూన్య సందర్శించు(Ast_కణుపు* nodep) override {
        తార్కిక selfImpure = !nodep->is_శుద్ధ();
        {
            VL_RESTORER(m_isImpure);
            m_isImpure = అబద్ధం;
            iterateChildrenConst(nodep);
            selfImpure |= m_isImpure;
            nodep->user2(selfImpure);
        }
        m_isImpure |= selfImpure;
    }

బహిరంగ:
    // CONSTRUCTORS
    స్పష్టమైన ExpandOkVisitor(AstNetlist* nodep) { iterateConst(nodep); }
    ~ExpandOkVisitor() = అప్రమేయ;
};

//######################################################################
// Expand state, as a visitor of each AstNode

తరగతి ExpandVisitor final : బహిరంగ VNVisitor {
    // NODE STATE
    //  AstNode::user1()        -> bool.  Processed
    //  AstNode::user2()        -> See ExpandOkVisitor
    స్థిరరాశి VNUser1_లోపల_Use m_inuser1;

    // STATE - for current visit position (use VL_RESTORER)
    Ast_కణుపు* m_stmtp = శూన్యసూచిక;  // Current statement

    // STATE - across all visitors
    AstCFunc* m_funcp = శూన్యసూచిక;  // Current function
    VDouble0 m_statImpure;  // Statistic tracking
    VDouble0 m_statWides;  // Statistic tracking
    VDouble0 m_stat_Wide_పదాలు;  // Statistic tracking
    VDouble0 m_statWideLimited;  // Statistic tracking
    VDouble0 m_statTemps;  // Statistic tracking
    // Number of applications of each expansion pattern for statistics
    std::array<VDouble0, VExpand_నమూనా::ENUM_చివర> m_statPatterns;

    // STATE - for current function
    కొలత_t m_nTmps = 0;  // Sequence numbers for temopraries

    // METHODS
    // Note the application of the given expansion pattern to 'nodep'
    శూన్య did_నమూనా(VExpand_నమూనా నమూనా_2, Ast_కణుపు* nodep) {
        UINFO(8, "    " << నమూనా_2.ascii() << " " << nodep);
        ++m_statPatterns[నమూనా_2];
    }

    // Use state that ExpandOkVisitor calculated
    స్థిర తార్కిక isImpure(Ast_కణుపు* nodep) {
        స్థిరరాశి తార్కిక impure = nodep->user2();
        ఒకవేళ (impure) UINFO(9, "      impure " << nodep);
        తిరిగి impure;
    }

    తార్కిక doExpandWide(Ast_కణుపు* nodep) {
        ఒకవేళ (isImpure(nodep)) {
            ++m_statImpure;
            తిరిగి అబద్ధం;
        }
        ఒకవేళ (nodep->వెడల్పు_పదాలు() <= v3Global.opt.expand_హద్దు()) {
            ++m_statWides;
            m_stat_Wide_పదాలు += nodep->వెడల్పు_పదాలు();
            తిరిగి నిజం;
        } లేకపోతే {
            ++m_statWideLimited;
            తిరిగి అబద్ధం;
        }
    }

    స్థిర పూర్ణాంకం long_Or_Quad_వెడల్పు(Ast_కణుపు* nodep) {
        తిరిగి (nodep->వెడల్పు() + (VL_EDATASIZE - 1)) & ~(VL_EDATASIZE - 1);
    }
    స్థిర V3_సంఖ్య not_Wide_ముసుగు(Ast_కణుపు* nodep) {
        తిరిగి V3_సంఖ్య{nodep, VL_EDATASIZE, ~VL_ముసుగు_E(nodep->వెడల్పు_Min())};
    }
    స్థిర V3_సంఖ్య మాట_ముసుగు(Ast_కణుపు* nodep) {
        ఒకవేళ (nodep->isWide()) {
            తిరిగి V3_సంఖ్య{nodep, VL_EDATASIZE, VL_ముసుగు_E(nodep->వెడల్పు_Min())};
        } లేకపోతే {
            V3_సంఖ్య ముసుగు{nodep, long_Or_Quad_వెడల్పు(nodep)};
            ముసుగు.సమితి_ముసుగు(nodep->వెడల్పు_Min());
            తిరిగి ముసుగు;
        }
    }

    స్థిర శూన్య చొప్పించు_Before(Ast_కణుపు* placep, Ast_కణుపు* newp) {
        newp->user1(1);  // Already processed, don't need to re-iterate
        placep->కూడు_ఇక్కడ_This_As_తదుపరి(newp);
    }
    స్థిర శూన్య replaceWithDelete(Ast_కణుపు* nodep, Ast_కణుపు* newp) {
        newp->user1(1);  // Already processed, don't need to re-iterate
        ఒకవేళ (newp->వెడల్పు() != nodep->వెడల్పు()) {
            UASSERT_OBJ(newp->వెడల్పు_Min() == nodep->వెడల్పు_Min(), nodep,
                        "Replacement width mismatch");
            newp->dtype_Chg_వెడల్పు(nodep->వెడల్పు(), nodep->వెడల్పు_Min());
        }
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_వృక్షం(), nodep);
    }
    స్థిర Ast_కణుపు* new_మాట_కేటాయించు(Ast_కణుపు_కేటాయించు* placep, పూర్ణాంకం మాట, Ast_కణుపు_సమాసం* lhsp,
                                  Ast_కణుపు_సమాసం* rhsp) {
        File_పంక్తి* స్థిరరాశి fl = placep->fileline();
        తిరిగి కొత్త Ast_కేటాయించు{fl,
                             కొత్త Ast_మాట_Sel{fl, lhsp->clone_వృక్షం_శుద్ధ(నిజం),
                                            కొత్త AstConst{fl, static_cast<uint32_t>(మాట)}},
                             rhsp};
    }
    స్థిర శూన్య కూడు_మాట_కేటాయించు(Ast_కణుపు_కేటాయించు* placep, పూర్ణాంకం మాట, Ast_కణుపు_సమాసం* lhsp,
                              Ast_కణుపు_సమాసం* rhsp) {
        చొప్పించు_Before(placep, new_మాట_కేటాయించు(placep, మాట, lhsp, rhsp));
    }
    స్థిర శూన్య కూడు_మాట_కేటాయించు(Ast_కణుపు_కేటాయించు* placep, పూర్ణాంకం మాట, Ast_కణుపు_సమాసం* rhsp) {
        కూడు_మాట_కేటాయించు(placep, మాట, placep->lhsp(), rhsp);
    }

    AstVar* కూడు_Local_Tmp(Ast_కణుపు* placep, స్థిరరాశి అక్షరం* namep, Ast_కణుపు_సమాసం* valuep) {
        స్థిరరాశి std::string name = "__V"s + namep + "_"s + std::గమ్యం_string(m_nTmps);
        File_పంక్తి* స్థిరరాశి flp = placep->fileline();
        AstVar* స్థిరరాశి tmpp = కొత్త AstVar{flp, VVarType::STMTTEMP, name, valuep->dtypep()};
        tmpp->funcLocal(నిజం);
        tmpp->is_అంతర్గత(నిజం);
        tmpp->noReset(నిజం);
        tmpp->substConstOnly(నిజం);
        m_funcp->కూడు_Varsp(tmpp);
        చొప్పించు_Before(placep, కొత్త Ast_కేటాయించు{flp, కొత్త AstVarRef{flp, tmpp, VAccess::WRITE}, valuep});
        ++m_statTemps;
        తిరిగి tmpp;
    }

    స్థిర శూన్య fixCloneLvalue(Ast_కణుపు* nodep) {
        // In AstSel transforms, we call clone() on VarRefs that were lvalues,
        // but are now being used on the RHS of the assignment
        ఒకవేళ (VN_IS(nodep, VarRef)) VN_AS(nodep, VarRef)->access(VAccess::READ);
        // Iterate
        ఒకవేళ (Ast_కణుపు* స్థిరరాశి refp = nodep->op1p()) fixCloneLvalue(refp);
        ఒకవేళ (Ast_కణుపు* స్థిరరాశి refp = nodep->op2p()) fixCloneLvalue(refp);
        ఒకవేళ (Ast_కణుపు* స్థిరరాశి refp = nodep->op3p()) fixCloneLvalue(refp);
        ఒకవేళ (Ast_కణుపు* స్థిరరాశి refp = nodep->op4p()) fixCloneLvalue(refp);
    }

    స్థిర Ast_కణుపు_సమాసం* new_Ast_మాట_Sel_Clone(Ast_కణుపు_సమాసం* nodep, పూర్ణాంకం మాట) {
        // Get the specified word number from a wide array
        // Or, if it's a long/quad, do appropriate conversion to wide
        // Concat may pass negative word numbers, that means it wants a zero
        File_పంక్తి* స్థిరరాశి fl = nodep->fileline();
        ఒకవేళ (nodep->isWide() && మాట >= 0 && మాట < nodep->వెడల్పు_పదాలు()) {
            తిరిగి కొత్త Ast_మాట_Sel{fl, nodep->clone_వృక్షం_శుద్ధ(నిజం),
                                  కొత్త AstConst{fl, static_cast<uint32_t>(మాట)}};
        } లేకపోతే ఒకవేళ (nodep->isQuad() && మాట == 0) {
            Ast_కణుపు_సమాసం* స్థిరరాశి quadfromp = nodep->clone_వృక్షం_శుద్ధ(నిజం);
            quadfromp->dtype_సమితి_బిట్టు_Unsized(VL_QUADSIZE, quadfromp->వెడల్పు_Min(), VSigning::UNSIGNED);
            తిరిగి కొత్త AstCCast{fl, quadfromp, VL_EDATASIZE};
        } లేకపోతే ఒకవేళ (nodep->isQuad() && మాట == 1) {
            Ast_కణుపు_సమాసం* స్థిరరాశి quadfromp = nodep->clone_వృక్షం_శుద్ధ(నిజం);
            quadfromp->dtype_సమితి_బిట్టు_Unsized(VL_QUADSIZE, quadfromp->వెడల్పు_Min(), VSigning::UNSIGNED);
            తిరిగి కొత్త AstCCast{
                fl, కొత్త Ast_జరుపు_R{fl, quadfromp, కొత్త AstConst{fl, VL_EDATASIZE}, VL_EDATASIZE},
                VL_EDATASIZE};
        } లేకపోతే ఒకవేళ (!nodep->isWide() && !nodep->isQuad() && మాట == 0) {
            తిరిగి nodep->clone_వృక్షం_శుద్ధ(నిజం);
        } లేకపోతే {  // Out of bounds
            తిరిగి కొత్త AstConst{fl, 0};
        }
    }

    స్థిర Ast_కణుపు_సమాసం* new_మాట_Grab_జరుపు(File_పంక్తి* fl, పూర్ణాంకం మాట, Ast_కణుపు_సమాసం* lhsp, పూర్ణాంకం జరుపు_2) {
        // Extract the expression to grab the value for the specified word, if it's the shift
        // of shift bits from lhsp
        // Negative word numbers requested for lhs when it's "before" what we want.
        // We get a 0 then.
        స్థిరరాశి పూర్ణాంకం othword = మాట - జరుపు_2 / VL_EDATASIZE;
        Ast_కణుపు_సమాసం* స్థిరరాశి lop = new_Ast_మాట_Sel_Clone(lhsp, othword);
        స్థిరరాశి uint32_t lo_జరుపు = VL_BITBIT_E(జరుపు_2);
        ఒకవేళ (!lo_జరుపు) తిరిగి lop;
        Ast_కణుపు_సమాసం* స్థిరరాశి hip = new_Ast_మాట_Sel_Clone(lhsp, othword - 1);
        స్థిరరాశి uint32_t hi_జరుపు = VL_EDATASIZE - lo_జరుపు;  // Complement offset
        Ast_కణుపు_సమాసం* స్థిరరాశి newp
            = కొత్త AstOr{fl,  //
                        కొత్త Ast_జరుపు_R{fl, hip, కొత్త AstConst{fl, hi_జరుపు}, VL_EDATASIZE},
                        కొత్త Ast_జరుపు_L{fl, lop, కొత్త AstConst{fl, lo_జరుపు}, VL_EDATASIZE}};
        తిరిగి V3Const::constifyEditCpp(newp);
    }

    // Return expression indexing the word that contains 'lsbp' + the given word offset
    స్థిర Ast_కణుపు_సమాసం* new_మాట_సూచీ(Ast_కణుపు_సమాసం* lsbp, uint32_t మాట_స్థానభ్రంశం = 0) {
        // This is indexing a WordSel, so a 32 bit constants are fine
        File_పంక్తి* స్థిరరాశి flp = lsbp->fileline();
        ఒకవేళ (స్థిరరాశి AstConst* constp = VN_CAST(lsbp, Const)) {
            తిరిగి కొత్త AstConst{flp, మాట_స్థానభ్రంశం + VL_BITWORD_E(constp->గమ్యం_UInt())};
        }

        ఒకవేళ (lsbp->backp()) lsbp = lsbp->clone_వృక్షం_శుద్ధ(అబద్ధం);
        Ast_కణుపు_సమాసం* స్థిరరాశి wwl2p = కొత్త AstConst{flp, VL_EDATASIZE_LOG2};  // Word width log 2
        Ast_కణుపు_సమాసం* indexp = కొత్త Ast_జరుపు_R{flp, lsbp, wwl2p, VL_EDATASIZE};
        ఒకవేళ (మాట_స్థానభ్రంశం) indexp = కొత్త Ast_కూడు{flp, కొత్త AstConst{flp, మాట_స్థానభ్రంశం}, indexp};
        తిరిగి indexp;
    }

    // Return word of fromp that contains word indexp + the given word offset.
    స్థిర Ast_కణుపు_సమాసం* new_మాట_Sel_మాట(File_పంక్తి* flp, Ast_కణుపు_సమాసం* fromp, Ast_కణుపు_సమాసం* indexp,
                                       uint32_t మాట_స్థానభ్రంశం = 0) {
        UASSERT_OBJ(fromp->isWide(), fromp, "Only need AstWordSel on wide from's");

        ఒకవేళ (స్థిరరాశి AstConst* స్థిరరాశి constp = VN_CAST(indexp, Const)) {
            indexp = శూన్యసూచిక;
            మాట_స్థానభ్రంశం += constp->గమ్యం_UInt();
        }

        // e.g. "logic [95:0] var[0]; logic [0] sel; out = var[sel];"
        // Squash before C++ to avoid getting a C++ compiler warning
        // (even though code would be unreachable as presumably a
        // AstCond is protecting above this node.
        ఒకవేళ (మాట_స్థానభ్రంశం >= static_cast<uint32_t>(fromp->వెడల్పు_పదాలు())) {
            తిరిగి కొత్త AstConst{flp, AstConst::SizedEData{}, 0};
        }

        ఒకవేళ (fromp->backp()) fromp = fromp->clone_వృక్షం_శుద్ధ(అబద్ధం);

        // If indexp was constant, just use it
        ఒకవేళ (!indexp) తిరిగి కొత్త Ast_మాట_Sel{flp, fromp, కొత్త AstConst{flp, మాట_స్థానభ్రంశం}};

        // Otherwise compute at runtime
        ఒకవేళ (indexp->backp()) indexp = indexp->clone_వృక్షం_శుద్ధ(అబద్ధం);
        ఒకవేళ (మాట_స్థానభ్రంశం) indexp = కొత్త Ast_కూడు{flp, కొత్త AstConst{flp, మాట_స్థానభ్రంశం}, indexp};
        తిరిగి కొత్త Ast_మాట_Sel{flp, fromp, indexp};
    }

    // Return word of fromp that contains bit lsbp + the given word offset.
    స్థిర Ast_కణుపు_సమాసం* new_మాట_Sel_బిట్టు(File_పంక్తి* flp, Ast_కణుపు_సమాసం* fromp, Ast_కణుపు_సమాసం* lsbp,
                                      uint32_t మాట_స్థానభ్రంశం = 0) {
        Ast_కణుపు_సమాసం* స్థిరరాశి indexp = new_మాట_సూచీ(lsbp, మాట_స్థానభ్రంశం);
        Ast_కణుపు_సమాసం* స్థిరరాశి మాట_Selp = new_మాట_Sel_మాట(flp, fromp, indexp);
        ఒకవేళ (!indexp->backp()) VL_DO_DANGLING(indexp->delete_వృక్షం(), indexp);
        తిరిగి మాట_Selp;
    }

    స్థిర Ast_కణుపు_సమాసం* new_Sel_బిట్టు_బిట్టు(Ast_కణుపు_సమాసం* lsbp) {
        // Return equation to get the VL_BITBIT of a constant or non-constant
        File_పంక్తి* స్థిరరాశి fl = lsbp->fileline();
        ఒకవేళ (VN_IS(lsbp, Const)) {
            తిరిగి కొత్త AstConst{fl, VL_BITBIT_E(VN_AS(lsbp, Const)->గమ్యం_UInt())};
        } లేకపోతే {
            తిరిగి కొత్త AstAnd{fl, కొత్త AstConst{fl, VL_EDATASIZE - 1}, lsbp->clone_వృక్షం_శుద్ధ(నిజం)};
        }
    }

    //====================

    తార్కిక expandWide(Ast_కణుపు_కేటాయించు* nodep, AstConst* rhsp) {
        ఒకవేళ (!doExpandWide(nodep)) తిరిగి అబద్ధం;
        did_నమూనా(VExpand_నమూనా::కేటాయించు_CONST, nodep);
        // -> {for each_word{ ASSIGN(WORDSEL(wide,#),WORDSEL(CONST,#))}}
        ఒకవేళ (rhsp->అంకె().isFourState()) {
            rhsp->v3warn(E_UNSUPPORTED,  // LCOV_EXCL_LINE  // impossible?
                         "Unsupported: 4-state numbers in this context");
        }
        File_పంక్తి* స్థిరరాశి fl = nodep->fileline();
        కొరకు (పూర్ణాంకం w = 0; w < nodep->వెడల్పు_పదాలు(); ++w) {
            కూడు_మాట_కేటాయించు(nodep, w,
                          కొత్త AstConst{fl, AstConst::SizedEData{}, rhsp->అంకె().edata_మాట(w)});
        }
        తిరిగి నిజం;
    }
    //-------- Uniops
    తార్కిక expandWide(Ast_కణుపు_కేటాయించు* nodep, AstVarRef* rhsp) {
        // Special case: do not expand assignment of constant pool variables.
        // V3Subst undestands these directly.
        ఒకవేళ (rhsp->varp()->const_Pool_నమోదు()) తిరిగి అబద్ధం;
        ఒకవేళ (!doExpandWide(nodep)) తిరిగి అబద్ధం;
        did_నమూనా(VExpand_నమూనా::కేటాయించు_VARREF, nodep);
        కొరకు (పూర్ణాంకం w = 0; w < nodep->వెడల్పు_పదాలు(); ++w) {
            కూడు_మాట_కేటాయించు(nodep, w, new_Ast_మాట_Sel_Clone(rhsp, w));
        }
        తిరిగి నిజం;
    }
    తార్కిక expandWide(Ast_కణుపు_కేటాయించు* nodep, AstArraySel* rhsp) {
        UASSERT_OBJ(!VN_IS(nodep->dtypep()->skipRefp(), UnpackArrayDType), nodep,
                    "ArraySel with unpacked arrays should have been removed in V3Slice");
        ఒకవేళ (!doExpandWide(nodep)) తిరిగి అబద్ధం;
        did_నమూనా(VExpand_నమూనా::కేటాయించు_ARRAYSEL, nodep);
        కొరకు (పూర్ణాంకం w = 0; w < nodep->వెడల్పు_పదాలు(); ++w) {
            కూడు_మాట_కేటాయించు(nodep, w, new_Ast_మాట_Sel_Clone(rhsp, w));
        }
        తిరిగి నిజం;
    }
    తార్కిక expandWide(Ast_కణుపు_కేటాయించు* nodep, AstNot* rhsp) {
        // -> {for each_word{ ASSIGN(WORDSEL(wide,#),NOT(WORDSEL(lhs,#))) }}
        ఒకవేళ (!doExpandWide(nodep)) తిరిగి అబద్ధం;
        did_నమూనా(VExpand_నమూనా::కేటాయించు_NOT, nodep);
        File_పంక్తి* స్థిరరాశి fl = rhsp->fileline();
        కొరకు (పూర్ణాంకం w = 0; w < nodep->వెడల్పు_పదాలు(); ++w) {
            కూడు_మాట_కేటాయించు(nodep, w, కొత్త AstNot{fl, new_Ast_మాట_Sel_Clone(rhsp->lhsp(), w)});
        }
        తిరిగి నిజం;
    }
    //-------- Biops
    తార్కిక expandWide(Ast_కణుపు_కేటాయించు* nodep, AstAnd* rhsp) {
        ఒకవేళ (!doExpandWide(nodep)) తిరిగి అబద్ధం;
        did_నమూనా(VExpand_నమూనా::కేటాయించు_AND, nodep);
        File_పంక్తి* స్థిరరాశి fl = nodep->fileline();
        కొరకు (పూర్ణాంకం w = 0; w < nodep->వెడల్పు_పదాలు(); ++w) {
            కూడు_మాట_కేటాయించు(nodep, w,
                          కొత్త AstAnd{fl, new_Ast_మాట_Sel_Clone(rhsp->lhsp(), w),
                                     new_Ast_మాట_Sel_Clone(rhsp->rhsp(), w)});
        }
        తిరిగి నిజం;
    }
    తార్కిక expandWide(Ast_కణుపు_కేటాయించు* nodep, AstOr* rhsp) {
        ఒకవేళ (!doExpandWide(nodep)) తిరిగి అబద్ధం;
        did_నమూనా(VExpand_నమూనా::కేటాయించు_OR, nodep);
        File_పంక్తి* స్థిరరాశి fl = nodep->fileline();
        కొరకు (పూర్ణాంకం w = 0; w < nodep->వెడల్పు_పదాలు(); ++w) {
            కూడు_మాట_కేటాయించు(nodep, w,
                          కొత్త AstOr{fl, new_Ast_మాట_Sel_Clone(rhsp->lhsp(), w),
                                    new_Ast_మాట_Sel_Clone(rhsp->rhsp(), w)});
        }
        తిరిగి నిజం;
    }
    తార్కిక expandWide(Ast_కణుపు_కేటాయించు* nodep, AstXor* rhsp) {
        ఒకవేళ (!doExpandWide(nodep)) తిరిగి అబద్ధం;
        did_నమూనా(VExpand_నమూనా::కేటాయించు_XOR, nodep);
        File_పంక్తి* స్థిరరాశి fl = nodep->fileline();
        కొరకు (పూర్ణాంకం w = 0; w < nodep->వెడల్పు_పదాలు(); ++w) {
            కూడు_మాట_కేటాయించు(nodep, w,
                          కొత్త AstXor{fl, new_Ast_మాట_Sel_Clone(rhsp->lhsp(), w),
                                     new_Ast_మాట_Sel_Clone(rhsp->rhsp(), w)});
        }
        తిరిగి నిజం;
    }
    తార్కిక expand_Wide_జరుపు(Ast_కణుపు_కేటాయించు* nodep, Ast_కణుపు_Biop* rhsp, తార్కిక is_ఎడమ_జరుపు) {
        ఒకవేళ (!doExpandWide(nodep)) తిరిగి అబద్ధం;

        // Simplify the shift amount, in case it becomes a constant
        V3Const::constifyEditCpp(rhsp->rhsp());

        // If it's a constant shift by whole words, expand it so V3Subst can substitute it
        ఒకవేళ (స్థిరరాశి AstConst* స్థిరరాశి rhsConstp = VN_CAST(rhsp->rhsp(), Const)) {
            స్థిరరాశి uint32_t జరుపు_బిట్లు = rhsConstp->గమ్యం_UInt();
            ఒకవేళ (VL_BITBIT_E(జరుపు_బిట్లు) == 0) {
                స్థిరరాశి పూర్ణాంకం వెడల్పు_పదాలు = nodep->వెడల్పు_పదాలు();
                స్థిరరాశి పూర్ణాంకం జరుపు_పదాలు = std::min<పూర్ణాంకం>(VL_BITWORD_E(జరుపు_బిట్లు), వెడల్పు_పదాలు);
                File_పంక్తి* స్థిరరాశి flp = rhsp->fileline();
                ఒకవేళ (is_ఎడమ_జరుపు) {
                    did_నమూనా(VExpand_నమూనా::కేటాయించు_SHIFTL, nodep);
                    // Low words of the result are zero
                    కొరకు (పూర్ణాంకం w = 0; w < జరుపు_పదాలు; ++w) {
                        కూడు_మాట_కేటాయించు(nodep, w, కొత్త AstConst{flp, AstConst::SizedEData{}, 0});
                    }
                    // High words of the result are copied from higher words of the source
                    కొరకు (పూర్ణాంకం w = జరుపు_పదాలు; w < వెడల్పు_పదాలు; ++w) {
                        కూడు_మాట_కేటాయించు(nodep, w, new_Ast_మాట_Sel_Clone(rhsp->lhsp(), w - జరుపు_పదాలు));
                    }
                } లేకపోతే {
                    did_నమూనా(VExpand_నమూనా::కేటాయించు_SHIFTR, nodep);
                    // Low words of the result are copied from higher words of the source
                    కొరకు (పూర్ణాంకం w = 0; w < వెడల్పు_పదాలు - జరుపు_పదాలు; ++w) {
                        కూడు_మాట_కేటాయించు(nodep, w, new_Ast_మాట_Sel_Clone(rhsp->lhsp(), w + జరుపు_పదాలు));
                    }
                    // High words of the result are zero
                    కొరకు (పూర్ణాంకం w = వెడల్పు_పదాలు - జరుపు_పదాలు; w < వెడల్పు_పదాలు; ++w) {
                        కూడు_మాట_కేటాయించు(nodep, w, కొత్త AstConst{flp, AstConst::SizedEData{}, 0});
                    }
                }
                తిరిగి నిజం;
            }
        }

        తిరిగి అబద్ధం;
    }

    //-------- Triops
    తార్కిక expandWide(Ast_కణుపు_కేటాయించు* nodep, AstCond* rhsp) {
        ఒకవేళ (!doExpandWide(nodep)) తిరిగి అబద్ధం;
        did_నమూనా(VExpand_నమూనా::కేటాయించు_COND, nodep);
        File_పంక్తి* స్థిరరాశి fl = nodep->fileline();
        కొరకు (పూర్ణాంకం w = 0; w < nodep->వెడల్పు_పదాలు(); ++w) {
            కూడు_మాట_కేటాయించు(nodep, w,
                          కొత్త AstCond{fl, rhsp->condp()->clone_వృక్షం_శుద్ధ(నిజం),
                                      new_Ast_మాట_Sel_Clone(rhsp->thenp(), w),
                                      new_Ast_మాట_Sel_Clone(rhsp->elsep(), w)});
        }
        తిరిగి నిజం;
    }

    // VISITORS
    శూన్య సందర్శించు(AstNetlist* nodep) override { iterate_And_తదుపరి_Null(nodep->modulesp()); }

    శూన్య సందర్శించు(AstCFunc* nodep) override {
        VL_RESTORER(m_funcp);
        VL_RESTORER(m_nTmps);
        m_funcp = nodep;
        m_nTmps = 0;
        స్థిరరాశి VDouble0 statWidesBefore = m_statWides;
        iterateChildren(nodep);

        // Constant fold here if anything was expanded, as Ast size can likely be reduced
        ఒకవేళ (v3Global.opt.fConstEager() && m_statWides != statWidesBefore) {
            Ast_కణుపు* స్థిరరాశి editedp = V3Const::constifyEditCpp(nodep);
            UASSERT_OBJ(editedp == nodep, editedp, "Should not have replaced CFunc");
        }
    }

    శూన్య సందర్శించు(AstExtend* nodep) override {
        ఒకవేళ (nodep->user1_సమితి_Once()) తిరిగి;  // Process once
        iterateChildren(nodep);
        ఒకవేళ (nodep->isWide()) {
            // See under ASSIGN(EXTEND)
        } లేకపోతే {
            ఒకవేళ (isImpure(nodep)) తిరిగి;
            Ast_కణుపు_సమాసం* స్థిరరాశి lhsp = nodep->lhsp()->unlink_Fr_వెనుక();
            Ast_కణుపు_సమాసం* newp = lhsp;
            ఒకవేళ (nodep->isQuad()) {
                ఒకవేళ (lhsp->isQuad()) {
                    lhsp->dtypeFrom(nodep);  // Just mark it, else nop
                } లేకపోతే ఒకవేళ (lhsp->isWide()) {
                    nodep->v3fatalSrc("extending larger thing into smaller?");
                } లేకపోతే {
                    newp = కొత్త AstCCast{nodep->fileline(), lhsp, nodep};
                }
            } లేకపోతే {  // Long
                UASSERT_OBJ(!(lhsp->isQuad() || lhsp->isWide()), nodep,
                            "extending larger thing into smaller?");
                lhsp->dtypeFrom(nodep);  // Just mark it, else nop
            }
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        }
    }
    తార్కిక expandWide(Ast_కణుపు_కేటాయించు* nodep, AstExtend* rhsp) {
        ఒకవేళ (!doExpandWide(nodep)) తిరిగి అబద్ధం;
        did_నమూనా(VExpand_నమూనా::కేటాయించు_EXTEND, nodep);
        Ast_కణుపు_సమాసం* స్థిరరాశి rlhsp = rhsp->lhsp();
        కొరకు (పూర్ణాంకం w = 0; w < rlhsp->వెడల్పు_పదాలు(); ++w) {
            కూడు_మాట_కేటాయించు(nodep, w, new_Ast_మాట_Sel_Clone(rlhsp, w));
        }
        కొరకు (పూర్ణాంకం w = rlhsp->వెడల్పు_పదాలు(); w < nodep->వెడల్పు_పదాలు(); ++w) {
            కూడు_మాట_కేటాయించు(nodep, w, కొత్త AstConst{nodep->fileline(), AstConst::SizedEData{}, 0});
        }
        తిరిగి నిజం;
    }

    శూన్య సందర్శించు(AstSel* nodep) override {
        ఒకవేళ (nodep->user1_సమితి_Once()) తిరిగి;  // Process once
        iterateChildren(nodep);

        స్థిరరాశి uint32_t వెడల్పు = static_cast<uint32_t>(nodep->వెడల్పు_Const());
        UASSERT_OBJ(nodep->వెడల్పు_Min() == static_cast<పూర్ణాంకం>(వెడల్పు), nodep, "Width mismatch");

        // Skip if Sel is an LHS assignment select
        ఒకవేళ (Ast_కణుపు_కేటాయించు* స్థిరరాశి assignp = VN_CAST(nodep->backp(), కణుపు_కేటాయించు)) {
            ఒకవేళ (nodep == assignp->lhsp()) తిరిగి;
        }

        // Skip if wide: See under ASSIGN(WIDE)
        ఒకవేళ (nodep->isWide()) తిరిగి;

        // Skip Sel from stream or queue
        ఒకవేళ (VN_IS(nodep->fromp()->dtypep(), ప్రవాహం_DType)) తిరిగి;
        ఒకవేళ (VN_IS(nodep->fromp()->dtypep(), వరుస_DType)) తిరిగి;

        // Result must be non-wide after skipping all the above
        UASSERT_OBJ(వెడల్పు <= 64, nodep, "Inconsistent result width");

        ఒకవేళ (nodep->fromp()->isWide()) {  // Long/Quad from Wide
            ఒకవేళ (isImpure(nodep)) తిరిగి;
            did_నమూనా(VExpand_నమూనా::SEL_FROM_WIDE, nodep);
            File_పంక్తి* స్థిరరాశి nfl = nodep->fileline();
            File_పంక్తి* స్థిరరాశి lfl = nodep->lsbp()->fileline();
            File_పంక్తి* స్థిరరాశి ffl = nodep->fromp()->fileline();
            Ast_కణుపు_సమాసం* స్థిరరాశి fromp = nodep->fromp();

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
            స్థిరరాశి స్వయంచాలక tryFold = [&](Ast_కణుపు_సమాసం* exprp) -> Ast_కణుపు_సమాసం* {
                // Size limit for inlining expressions (any arbitrary value results in
                // a bounded expansion. Picked a value to allow common simple forms.)
                స్థిర constexpr పూర్ణాంకం EXPAND_SEL_LSB_హద్దు = 8;
                // Simplify
                exprp = V3Const::constifyEditCpp(exprp);
                // Accept if not larger than the limit
                ఒకవేళ (!exprp->isLargerThan(EXPAND_SEL_LSB_హద్దు)) తిరిగి exprp;
                // Delete if rejected
                VL_DO_DANGLING(exprp->delete_వృక్షం(), exprp);
                తిరిగి శూన్యసూచిక;
            };

            // Return the index of the word holding the bit 'offset' bits above the LSB
            స్థిరరాశి స్వయంచాలక మాట_Idx = [&](uint32_t స్థానభ్రంశం) -> Ast_కణుపు_సమాసం* {
                Ast_కణుపు_సమాసం* స్థిరరాశి msbp = కొత్త Ast_కూడు{lfl, కొత్త AstConst{lfl, స్థానభ్రంశం},
                                                     nodep->lsbp()->clone_వృక్షం_శుద్ధ(అబద్ధం)};
                Ast_కణుపు_సమాసం* స్థిరరాశి idxp = new_మాట_సూచీ(msbp);
                ఒకవేళ (!msbp->backp()) VL_DO_DANGLING(msbp->delete_వృక్షం(), msbp);
                తిరిగి idxp;
            };

            // Bit index of the selected LSB within its word
            Ast_కణుపు_సమాసం* lBitp = tryFold(new_Sel_బిట్టు_బిట్టు(nodep->lsbp()));
            స్థిరరాశి తార్కిక aligned = lBitp && lBitp->isZero();

            // Which of the other 2 words the expansion might need
            స్థిరరాశి తార్కిక mNeeded = వెడల్పు > 1 && !aligned;
            స్థిరరాశి తార్కిక hNeeded = వెడల్పు > VL_EDATASIZE;
            UASSERT_OBJ(!hNeeded || nodep->isQuad(), nodep, "Width mismatch");

            // Word indices
            స్థిరరాశి uint32_t m_Msb_స్థానభ్రంశం = std::min<uint32_t>(వెడల్పు, VL_EDATASIZE) - 1;
            స్థిరరాశి uint32_t h_Msb_స్థానభ్రంశం = వెడల్పు - 1;
            Ast_కణుపు_సమాసం* lIdxp = tryFold(new_మాట_సూచీ(nodep->lsbp()));
            Ast_కణుపు_సమాసం* mIdxp = mNeeded ? tryFold(మాట_Idx(m_Msb_స్థానభ్రంశం)) : శూన్యసూచిక;
            Ast_కణుపు_సమాసం* hIdxp = hNeeded ? tryFold(మాట_Idx(h_Msb_స్థానభ్రంశం)) : శూన్యసూచిక;

            // If the LSB expression is needed multiple times, evaluate it into a temporary
            స్థిరరాశి పూర్ణాంకం nCopies = !lBitp + !lIdxp + (mNeeded && !mIdxp) + (hNeeded && !hIdxp);
            ఒకవేళ (nCopies > 1 && m_funcp && m_stmtp && !isImpure(m_stmtp)) {
                ++m_nTmps;  // Use fresh set of temporaries
                Ast_కణుపు_సమాసం* స్థిరరాశి lsbp = nodep->lsbp()->unlink_Fr_వెనుక();
                AstVar* స్థిరరాశి tmpp = కూడు_Local_Tmp(m_stmtp, "ExpandSel_Lsb", lsbp);
                nodep->lsbp(కొత్త AstVarRef{lfl, tmpp, VAccess::READ});
            }

            // Rebuild remaining expressions derived from LSB if needed, now using the temporary
            ఒకవేళ (!lBitp) lBitp = new_Sel_బిట్టు_బిట్టు(nodep->lsbp());
            ఒకవేళ (!lIdxp) lIdxp = new_మాట_సూచీ(nodep->lsbp());
            ఒకవేళ (!mIdxp) mIdxp = mNeeded ? మాట_Idx(m_Msb_స్థానభ్రంశం) : శూన్యసూచిక;
            ఒకవేళ (!hIdxp) hIdxp = hNeeded ? మాట_Idx(h_Msb_స్థానభ్రంశం) : శూన్యసూచిక;

            // Return word 'idxp' of 'fromp', without consuming 'idxp'
            స్థిరరాశి స్వయంచాలక మాట_Sel_2 = [&](Ast_కణుపు_సమాసం* idxp) -> Ast_కణుపు_సమాసం* {
                Ast_కణుపు_సమాసం* స్థిరరాశి clonep = idxp->clone_వృక్షం_శుద్ధ(అబద్ధం);
                Ast_కణుపు_సమాసం* స్థిరరాశి wordp = new_మాట_Sel_మాట(ffl, fromp, clonep);
                ఒకవేళ (!clonep->backp()) VL_DO_DANGLING(clonep->delete_వృక్షం(), clonep);
                తిరిగి wordp;
            };

            // Construct term containing the bits of the low word - always needed
            Ast_కణుపు_సమాసం* స్థిరరాశి lTermp = [&]() -> Ast_కణుపు_సమాసం* {
                Ast_కణుపు_సమాసం* lWordp = మాట_Sel_2(lIdxp);
                ఒకవేళ (nodep->isQuad()) lWordp = కొత్త AstCCast{nfl, lWordp, nodep};
                తిరిగి కొత్త Ast_జరుపు_R{nfl, lWordp, lBitp->clone_వృక్షం_శుద్ధ(అబద్ధం), nodep->వెడల్పు()};
            }();

            // Construct term containing the bits of the middle word - if needed
            Ast_కణుపు_సమాసం* స్థిరరాశి mTermp = [&]() -> Ast_కణుపు_సమాసం* {
                ఒకవేళ (!mNeeded) తిరిగి శూన్యసూచిక;

                Ast_కణుపు_సమాసం* mWordp = మాట_Sel_2(mIdxp);
                ఒకవేళ (nodep->isQuad()) mWordp = కొత్త AstCCast{nfl, mWordp, nodep};
                Ast_కణుపు_సమాసం* స్థిరరాశి mShiftp = కొత్త AstSub{lfl, కొత్త AstConst{lfl, VL_EDATASIZE},
                                                        lBitp->clone_వృక్షం_శుద్ధ(అబద్ధం)};
                // If we're selecting bit zero, then all 32 bits in the mid word
                // get shifted << by 32 bits, so ignore them.
                స్థిరరాశి V3_సంఖ్య zero{nodep, long_Or_Quad_వెడల్పు(nodep)};
                తిరిగి కొత్త AstCond{
                    nfl,
                    // lsb % VL_EDATASIZE == 0 ?
                    కొత్త AstEq{nfl, కొత్త AstConst{nfl, 0}, lBitp->clone_వృక్షం_శుద్ధ(అబద్ధం)},
                    // 0 :
                    కొత్త AstConst{nfl, zero},
                    //  midword >> (VL_EDATASIZE - (lbs % VL_EDATASIZE))
                    కొత్త Ast_జరుపు_L{nfl, mWordp, mShiftp, nodep->వెడల్పు()}};
            }();

            // Construct term containing the bits of the high word - if needed
            Ast_కణుపు_సమాసం* స్థిరరాశి hTermp = [&]() -> Ast_కణుపు_సమాసం* {
                ఒకవేళ (!hNeeded) తిరిగి శూన్యసూచిక;

                Ast_కణుపు_సమాసం* hWordp = మాట_Sel_2(hIdxp);
                hWordp = కొత్త AstCCast{nfl, hWordp, nodep};
                Ast_కణుపు_సమాసం* స్థిరరాశి hShiftp = కొత్త AstCond{
                    nfl,
                    // lsb % VL_EDATASIZE == 0 ?
                    కొత్త AstEq{nfl, కొత్త AstConst{nfl, 0}, lBitp->clone_వృక్షం_శుద్ధ(అబద్ధం)},
                    // VL_EDATASIZE :
                    కొత్త AstConst{lfl, VL_EDATASIZE},
                    // 64 - (lbs % VL_EDATASIZE)
                    కొత్త AstSub{lfl, కొత్త AstConst{lfl, 64}, lBitp->clone_వృక్షం_శుద్ధ(అబద్ధం)}};
                తిరిగి కొత్త Ast_జరుపు_L{nfl, hWordp, hShiftp, nodep->వెడల్పు()};
            }();

            // Delete parts not captured during construction
            VL_DO_DANGLING(lBitp->delete_వృక్షం(), lBitp);
            VL_DO_DANGLING(lIdxp->delete_వృక్షం(), lIdxp);
            ఒకవేళ (mIdxp) VL_DO_DANGLING(mIdxp->delete_వృక్షం(), mIdxp);
            ఒకవేళ (hIdxp) VL_DO_DANGLING(hIdxp->delete_వృక్షం(), hIdxp);

            // Or reduce the terms
            Ast_కణుపు_సమాసం* newp = lTermp;
            ఒకవేళ (mTermp) newp = కొత్త AstOr{nfl, mTermp, newp};
            ఒకవేళ (hTermp) newp = కొత్త AstOr{nfl, hTermp, newp};
            newp->dtypeFrom(nodep);
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        } లేకపోతే {  // Long/Quad from Long/Quad
            // No isImpure() check - can handle side effects in below
            did_నమూనా(VExpand_నమూనా::SEL_FROM_NARROW, nodep);
            File_పంక్తి* స్థిరరాశి fl = nodep->fileline();
            Ast_కణుపు_సమాసం* fromp = nodep->fromp()->unlink_Fr_వెనుక();
            Ast_కణుపు_సమాసం* స్థిరరాశి lsbp = nodep->lsbp()->unlink_Fr_వెనుక();
            ఒకవేళ (nodep->isQuad() && !fromp->isQuad()) fromp = కొత్త AstCCast{fl, fromp, nodep};
            // {large}>>32 requires 64-bit shift operation; then cast
            Ast_కణుపు_సమాసం* newp = కొత్త Ast_జరుపు_R{fl, fromp, lsbp, fromp->వెడల్పు()};
            newp->dtypeFrom(fromp);
            ఒకవేళ (!nodep->isQuad() && fromp->isQuad()) newp = కొత్త AstCCast{fl, newp, nodep};
            newp->dtypeFrom(nodep);
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        }
    }

    తార్కిక expandWide(Ast_కణుపు_కేటాయించు* nodep, AstSel* rhsp) {
        UASSERT_OBJ(nodep->వెడల్పు_Min() == rhsp->వెడల్పు_Const(), nodep, "Width mismatch");
        ఒకవేళ (!doExpandWide(nodep)) తిరిగి అబద్ధం;

        // Simplify the index, in case it becomes a constant
        V3Const::constifyEditCpp(rhsp->lsbp());

        // If it's a constant select and aligned, we can just copy the words
        ఒకవేళ (స్థిరరాశి AstConst* స్థిరరాశి lsbConstp = VN_CAST(rhsp->lsbp(), Const)) {
            స్థిరరాశి uint32_t lsb = lsbConstp->గమ్యం_UInt();
            ఒకవేళ (VL_BITBIT_E(lsb) == 0) {
                did_నమూనా(VExpand_నమూనా::కేటాయించు_SEL_ALIGNED, nodep);
                స్థిరరాశి uint32_t మాట = VL_BITWORD_E(lsb);
                కొరకు (పూర్ణాంకం w = 0; w < nodep->వెడల్పు_పదాలు(); ++w) {
                    కూడు_మాట_కేటాయించు(nodep, w, new_Ast_మాట_Sel_Clone(rhsp->fromp(), w + మాట));
                }
                తిరిగి నిజం;
            }
        }

        did_నమూనా(VExpand_నమూనా::కేటాయించు_SEL_UNALIGNED, nodep);
        File_పంక్తి* స్థిరరాశి flp = rhsp->fileline();

        // Use fresh set of temporaries
        ++m_nTmps;

        // Compute word index of LSB, store to temporary if not constant
        Ast_కణుపు_సమాసం* స్థిరరాశి మాట_Lsbp = rhsp->lsbp()->clone_వృక్షం_శుద్ధ(అబద్ధం);
        Ast_కణుపు_సమాసం* మాట_Idxp = new_మాట_సూచీ(మాట_Lsbp);
        ఒకవేళ (!మాట_Lsbp->backp()) VL_DO_DANGLING(మాట_Lsbp->delete_వృక్షం(), మాట_Lsbp);
        మాట_Idxp = V3Const::constifyEditCpp(మాట_Idxp);
        ఒకవేళ (!VN_IS(మాట_Idxp, Const)) {
            AstVar* స్థిరరాశి tmpp = కూడు_Local_Tmp(nodep, "ExpandSel_WordIdx", మాట_Idxp);
            మాట_Idxp = కొత్త AstVarRef{flp, tmpp, VAccess::READ};
        }

        // Compute shift amounts and mask, store to temporaries if not constants
        Ast_కణుపు_సమాసం* loShftp = శూన్యసూచిక;
        Ast_కణుపు_సమాసం* hiShftp = శూన్యసూచిక;
        Ast_కణుపు_సమాసం* hiMaskp = శూన్యసూచిక;
        ఒకవేళ (స్థిరరాశి AstConst* స్థిరరాశి lsbConstp = VN_CAST(rhsp->lsbp(), Const)) {
            స్థిరరాశి uint32_t బిట్టు_స్థానభ్రంశం = VL_BITBIT_E(lsbConstp->గమ్యం_UInt());
            // Must be unaligned, otherwise we would have handled it above
            UASSERT_OBJ(బిట్టు_స్థానభ్రంశం, nodep, "Missed aligned wide select");
            loShftp = కొత్త AstConst{flp, బిట్టు_స్థానభ్రంశం};
            hiShftp = కొత్త AstConst{flp, VL_EDATASIZE - బిట్టు_స్థానభ్రంశం};
            hiMaskp = శూన్యసూచిక;
        } లేకపోతే {
            // Compute Low word shift amount: bottom bits of lsb index
            Ast_కణుపు_సమాసం* స్థిరరాశి lsbp = rhsp->lsbp()->clone_వృక్షం_శుద్ధ(అబద్ధం);
            loShftp = కొత్త AstAnd{flp, కొత్త AstConst{flp, VL_SIZEBITS_E}, lsbp};
            AstVar* స్థిరరాశి loTmpp = కూడు_Local_Tmp(nodep, "ExpandSel_LoShift", loShftp);
            loShftp = కొత్త AstVarRef{flp, loTmpp, VAccess::READ};

            // Compute if aligned: Low word shift amount is 0
            Ast_కణుపు_సమాసం* స్థిరరాశి zerop = కొత్త AstConst{flp, 0};
            Ast_కణుపు_సమాసం* alignedp = కొత్త AstEq{flp, zerop, loShftp->clone_వృక్షం_శుద్ధ(అబద్ధం)};
            AstVar* స్థిరరాశి alignedTmpp = కూడు_Local_Tmp(nodep, "ExpandSel_Aligned", alignedp);
            alignedp = కొత్త AstVarRef{flp, alignedTmpp, VAccess::READ};

            // Computed High word shift amount: 0 if aligned else VL_EDATASIZE - Low word shift
            Ast_కణుపు_సమాసం* స్థిరరాశి edsp = కొత్త AstConst{flp, VL_EDATASIZE};
            Ast_కణుపు_సమాసం* స్థిరరాశి subp = కొత్త AstSub{flp, edsp, loShftp->clone_వృక్షం_శుద్ధ(అబద్ధం)};
            hiShftp = కొత్త AstCond{flp, alignedp, కొత్త AstConst{flp, 0}, subp};
            AstVar* స్థిరరాశి hiTmpp = కూడు_Local_Tmp(nodep, "ExpandSel_HiShift", hiShftp);
            hiShftp = కొత్త AstVarRef{flp, hiTmpp, VAccess::READ};

            // Compute the High word mask: 0 if aligned, ones otherwise
            hiMaskp = కొత్త AstCond{flp, alignedp->clone_వృక్షం_శుద్ధ(అబద్ధం),
                                  కొత్త AstConst{flp, AstConst::SizedEData{}, 0},
                                  కొత్త AstConst{flp, AstConst::SizedEData{}, -1ULL}};
            AstVar* స్థిరరాశి ముసుగు_Tmpp = కూడు_Local_Tmp(nodep, "ExpandSel_HiMask", hiMaskp);
            hiMaskp = కొత్త AstVarRef{flp, ముసుగు_Tmpp, VAccess::READ};
        }

        // Create each word of the selected result
        Ast_కణుపు_సమాసం* స్థిరరాశి fromp = rhsp->fromp();
        స్థిరరాశి పూర్ణాంకం sel_పదాలు = nodep->వెడల్పు_పదాలు();
        కొరకు (పూర్ణాంకం w = 0; w < sel_పదాలు; ++w) {
            // Grab bits from word 'VL_BITWORD_E(lsb) + w'
            Ast_కణుపు_సమాసం* స్థిరరాశి loWordp = new_మాట_Sel_మాట(fromp->fileline(), fromp, మాట_Idxp, w);
            Ast_కణుపు_సమాసం* స్థిరరాశి loShftClonep = loShftp->clone_వృక్షం_శుద్ధ(అబద్ధం);
            Ast_కణుపు_సమాసం* స్థిరరాశి lop = కొత్త Ast_జరుపు_R{flp, loWordp, loShftClonep, VL_EDATASIZE};
            // Grab bits from word 'VL_BITWORD_E(lsb) + w + 1'
            Ast_కణుపు_సమాసం* hiWordp = new_మాట_Sel_మాట(fromp->fileline(), fromp, మాట_Idxp, w + 1);
            // For the last word of the result, avoid an OOB access when the Sel is not OOB
            ఒకవేళ (w == sel_పదాలు - 1) {
                స్థిరరాశి uint32_t max = fromp->వెడల్పు_పదాలు() - w - 1;
                Ast_కణుపు_సమాసం* స్థిరరాశి maxp = కొత్త AstConst{flp, max};
                Ast_కణుపు_సమాసం* స్థిరరాశి condp = కొత్త AstGte{flp, మాట_Idxp->clone_వృక్షం_శుద్ధ(అబద్ధం), maxp};
                Ast_కణుపు_సమాసం* స్థిరరాశి zerop = కొత్త AstConst{flp, AstConst::SizedEData{}, 0};
                hiWordp = కొత్త AstCond{flp, condp, zerop, hiWordp};
            }
            Ast_కణుపు_సమాసం* స్థిరరాశి hiShftClonep = hiShftp->clone_వృక్షం_శుద్ధ(అబద్ధం);
            Ast_కణుపు_సమాసం* స్థిరరాశి hiPartp = కొత్త Ast_జరుపు_L{flp, hiWordp, hiShftClonep, VL_EDATASIZE};
            Ast_కణుపు_సమాసం* స్థిరరాశి hip
                = hiMaskp ? కొత్త AstAnd{flp, hiPartp, hiMaskp->clone_వృక్షం_శుద్ధ(అబద్ధం)} : hiPartp;
            // Combine them
            కూడు_మాట_కేటాయించు(nodep, w, కొత్త AstOr{nodep->fileline(), hip, lop});
        }

        // Delete parts not captured during construction
        ఒకవేళ (!మాట_Idxp->backp()) VL_DO_DANGLING(మాట_Idxp->delete_వృక్షం(), మాట_Idxp);
        ఒకవేళ (!loShftp->backp()) VL_DO_DANGLING(loShftp->delete_వృక్షం(), loShftp);
        ఒకవేళ (!hiShftp->backp()) VL_DO_DANGLING(hiShftp->delete_వృక్షం(), hiShftp);
        ఒకవేళ (hiMaskp && !hiMaskp->backp()) VL_DO_DANGLING(hiMaskp->delete_వృక్షం(), hiMaskp);

        తిరిగి నిజం;
    }

    తార్కిక expandLhs(Ast_కణుపు_కేటాయించు* nodep, AstSel* lhsp) {
        // Possibilities
        //      destp: wide or narrow
        //      rhsp:  wide (destp must be wide), narrow, or 1 bit wide
        //      rhsp:  may be allones and can remove AND NOT gate
        //      lsbp:  constant or variable
        // Yuk.
        ఒకవేళ (isImpure(nodep)) తిరిగి అబద్ధం;
        File_పంక్తి* స్థిరరాశి nfl = nodep->fileline();
        File_పంక్తి* స్థిరరాశి lfl = lhsp->fileline();
        స్థిరరాశి తార్కిక destwide = lhsp->fromp()->isWide();
        స్థిరరాశి తార్కిక ones = nodep->rhsp()->isAllOnesV();
        ఒకవేళ (VN_IS(lhsp->lsbp(), Const)) {
            // The code should work without this constant test, but it won't
            // constify as nicely as we'd like.
            Ast_కణుపు_సమాసం* rhsp = nodep->rhsp()->unlink_Fr_వెనుక();
            Ast_కణుపు_సమాసం* స్థిరరాశి destp = lhsp->fromp()->unlink_Fr_వెనుక();
            స్థిరరాశి పూర్ణాంకం lsb = lhsp->lsbConst();
            స్థిరరాశి పూర్ణాంకం msb = lhsp->msbConst();
            V3_సంఖ్య maskset{nodep, destp->వెడల్పు_Min()};
            maskset.సమితి_ముసుగు(msb + 1 - lsb, lsb);
            V3_సంఖ్య maskold{nodep, destp->వెడల్పు_Min()};
            maskold.opNot(maskset);
            ఒకవేళ (destwide) {
                did_నమూనా(VExpand_నమూనా::కేటాయించు_గమ్యం_SEL_CONST_WIDE, nodep);
                కొరకు (పూర్ణాంకం w = 0; w < destp->వెడల్పు_పదాలు(); ++w) {
                    ఒకవేళ (w >= VL_BITWORD_E(lsb) && w <= VL_BITWORD_E(msb)) {
                        // else we would just be setting it to the same exact value
                        Ast_కణుపు_సమాసం* oldvalp = new_Ast_మాట_Sel_Clone(destp, w);
                        fixCloneLvalue(oldvalp);
                        ఒకవేళ (!ones) {
                            oldvalp = కొత్త AstAnd{
                                lfl,
                                కొత్త AstConst{lfl, AstConst::SizedEData{}, maskold.edata_మాట(w)},
                                oldvalp};
                        }

                        // Appropriate word of new value to insert:
                        Ast_కణుపు_సమాసం* newp = new_మాట_Grab_జరుపు(lfl, w, rhsp, lsb);

                        // Apply cleaning at the top word of the destination
                        // (no cleaning to do if dst's width is a whole number
                        // of words).
                        ఒకవేళ (w == destp->వెడల్పు_పదాలు() - 1 && VL_BITBIT_E(destp->వెడల్పు_Min()) != 0) {
                            V3_సంఖ్య cleanmask{nodep, VL_EDATASIZE};
                            cleanmask.సమితి_ముసుగు(VL_BITBIT_E(destp->వెడల్పు_Min()));
                            newp = కొత్త AstAnd{lfl, newp, కొత్త AstConst{lfl, cleanmask}};
                        }
                        Ast_కణుపు_సమాసం* స్థిరరాశి orp
                            = V3Const::constifyEditCpp(కొత్త AstOr{lfl, oldvalp, newp});
                        కూడు_మాట_కేటాయించు(nodep, w, destp, orp);
                    }
                }
                VL_DO_DANGLING(rhsp->delete_వృక్షం(), rhsp);
                VL_DO_DANGLING(destp->delete_వృక్షం(), destp);
            } లేకపోతే {
                did_నమూనా(VExpand_నమూనా::కేటాయించు_గమ్యం_SEL_CONST_NARROW, nodep);
                ఒకవేళ (destp->isQuad() && !rhsp->isQuad()) rhsp = కొత్త AstCCast{nfl, rhsp, nodep};
                Ast_కణుపు_సమాసం* oldvalp = destp->clone_వృక్షం_శుద్ధ(నిజం);
                fixCloneLvalue(oldvalp);
                ఒకవేళ (!ones) oldvalp = కొత్త AstAnd{lfl, కొత్త AstConst{lfl, maskold}, oldvalp};

                // The bit-select can refer to bits outside the width of nodep
                // which we aren't allowed to assign to.  This is a mask of the
                // valid range of nodep which we apply to the new shifted RHS.
                V3_సంఖ్య cleanmask{nodep, destp->వెడల్పు_Min()};
                cleanmask.సమితి_ముసుగు(destp->వెడల్పు_Min());
                Ast_కణుపు_సమాసం* స్థిరరాశి shifted = కొత్త Ast_జరుపు_L{
                    lfl, rhsp, కొత్త AstConst{lfl, static_cast<uint32_t>(lsb)}, destp->వెడల్పు()};
                Ast_కణుపు_సమాసం* స్థిరరాశి cleaned
                    = కొత్త AstAnd{lfl, shifted, కొత్త AstConst{lfl, cleanmask}};
                Ast_కణుపు_సమాసం* స్థిరరాశి orp
                    = V3Const::constifyEditCpp(కొత్త AstOr{lfl, oldvalp, cleaned});
                చొప్పించు_Before(nodep, కొత్త Ast_కేటాయించు{nfl, destp, orp});
            }
            తిరిగి నిజం;
        } లేకపోతే {  // non-const select offset
            ఒకవేళ (destwide && lhsp->వెడల్పు_Const() == 1) {
                did_నమూనా(VExpand_నమూనా::కేటాయించు_గమ్యం_SEL_VAR_WIDE_ONE_బిట్టు, nodep);
                Ast_కణుపు_సమాసం* స్థిరరాశి rhsp = nodep->rhsp()->unlink_Fr_వెనుక();
                Ast_కణుపు_సమాసం* స్థిరరాశి destp = lhsp->fromp()->unlink_Fr_వెనుక();
                Ast_కణుపు_సమాసం* oldvalp
                    = new_మాట_Sel_బిట్టు(lfl, destp->clone_వృక్షం_శుద్ధ(అబద్ధం), lhsp->lsbp());
                fixCloneLvalue(oldvalp);
                ఒకవేళ (!ones) {
                    oldvalp = కొత్త AstAnd{
                        lfl,
                        కొత్త AstNot{
                            lfl, కొత్త Ast_జరుపు_L{lfl, కొత్త AstConst{nfl, 1},
                                               // newSelBitBit may exceed the MSB of this variable.
                                               // That's ok as we'd just AND with a larger value,
                                               // but oldval would clip the upper bits to sanity
                                               new_Sel_బిట్టు_బిట్టు(lhsp->lsbp()), VL_EDATASIZE}},
                        oldvalp};
                }
                // Restrict the shift amount to 0-31, see bug804.
                Ast_కణుపు_సమాసం* స్థిరరాశి shiftp = కొత్త AstAnd{nfl, lhsp->lsbp()->clone_వృక్షం_శుద్ధ(నిజం),
                                                       కొత్త AstConst{nfl, VL_EDATASIZE - 1}};
                Ast_కణుపు* స్థిరరాశి newp = కొత్త Ast_కేటాయించు{
                    nfl, new_మాట_Sel_బిట్టు(nfl, destp, lhsp->lsbp()),
                    కొత్త AstOr{lfl, oldvalp, కొత్త Ast_జరుపు_L{lfl, rhsp, shiftp, VL_EDATASIZE}}};
                చొప్పించు_Before(nodep, newp);
                తిరిగి నిజం;
            } లేకపోతే ఒకవేళ (destwide) {
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
                తిరిగి అబద్ధం;
            } లేకపోతే {
                did_నమూనా(VExpand_నమూనా::కేటాయించు_గమ్యం_SEL_VAR_NARROW, nodep);
                // nodep->dumpTree("-  old: ");
                Ast_కణుపు_సమాసం* rhsp = nodep->rhsp()->unlink_Fr_వెనుక();
                Ast_కణుపు_సమాసం* స్థిరరాశి destp = lhsp->fromp()->unlink_Fr_వెనుక();
                Ast_కణుపు_సమాసం* oldvalp = destp->clone_వృక్షం_శుద్ధ(నిజం);
                fixCloneLvalue(oldvalp);

                V3_సంఖ్య maskwidth{nodep, destp->వెడల్పు_Min()};
                maskwidth.సమితి_ముసుగు(lhsp->వెడల్పు_Const());

                ఒకవేళ (destp->isQuad() && !rhsp->isQuad()) rhsp = కొత్త AstCCast{nfl, rhsp, nodep};
                ఒకవేళ (!ones) {
                    oldvalp = కొత్త AstAnd{
                        lfl,
                        కొత్త AstNot{lfl, కొత్త Ast_జరుపు_L{lfl, కొత్త AstConst{nfl, maskwidth},
                                                      lhsp->lsbp()->clone_వృక్షం_శుద్ధ(నిజం),
                                                      destp->వెడల్పు()}},
                        oldvalp};
                }
                Ast_కణుపు_సమాసం* newp
                    = కొత్త Ast_జరుపు_L{lfl, rhsp, lhsp->lsbp()->clone_వృక్షం_శుద్ధ(నిజం), destp->వెడల్పు()};
                // Apply cleaning to the new value being inserted.  Mask is
                // slightly wider than necessary to avoid an AND with all ones
                // being optimized out.  No need to clean if destp is
                // quad-sized as there are no extra bits to contaminate
                ఒకవేళ (destp->వెడల్పు_Min() != 64) {
                    V3_సంఖ్య cleanmask{nodep, destp->వెడల్పు_Min() + 1};
                    cleanmask.సమితి_ముసుగు(destp->వెడల్పు_Min());
                    newp = కొత్త AstAnd{lfl, newp, కొత్త AstConst{lfl, cleanmask}};
                }

                చొప్పించు_Before(nodep, కొత్త Ast_కేటాయించు{nfl, destp, కొత్త AstOr{lfl, oldvalp, newp}});
                తిరిగి నిజం;
            }
        }
    }

    శూన్య సందర్శించు(AstConcat* nodep) override {
        ఒకవేళ (nodep->user1_సమితి_Once()) తిరిగి;  // Process once
        iterateChildren(nodep);
        ఒకవేళ (nodep->isWide()) {
            // See under ASSIGN(WIDE)
        } లేకపోతే {
            // No isImpure() check - can handle side effects in below
            did_నమూనా(VExpand_నమూనా::CONCAT, nodep);
            File_పంక్తి* స్థిరరాశి fl = nodep->fileline();
            Ast_కణుపు_సమాసం* lhsp = nodep->lhsp()->unlink_Fr_వెనుక();
            Ast_కణుపు_సమాసం* rhsp = nodep->rhsp()->unlink_Fr_వెనుక();
            స్థిరరాశి uint32_t rhsshift = rhsp->వెడల్పు_Min();
            ఒకవేళ (nodep->isQuad() && !lhsp->isQuad()) lhsp = కొత్త AstCCast{fl, lhsp, nodep};
            ఒకవేళ (nodep->isQuad() && !rhsp->isQuad()) rhsp = కొత్త AstCCast{fl, rhsp, nodep};
            Ast_కణుపు_సమాసం* స్థిరరాశి newp = కొత్త AstOr{
                fl, కొత్త Ast_జరుపు_L{fl, lhsp, కొత్త AstConst{fl, rhsshift}, nodep->వెడల్పు()}, rhsp};
            newp->dtypeFrom(nodep);  // Unsigned
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        }
    }
    తార్కిక expandWide(Ast_కణుపు_కేటాయించు* nodep, AstConcat* rhsp) {
        ఒకవేళ (!doExpandWide(rhsp)) తిరిగి అబద్ధం;
        did_నమూనా(VExpand_నమూనా::కేటాయించు_CONCAT, nodep);
        File_పంక్తి* స్థిరరాశి fl = rhsp->fileline();
        // Lhs or Rhs may be word, long, or quad.
        // newAstWordSelClone nicely abstracts the difference.
        స్థిరరాశి పూర్ణాంకం rhsshift = rhsp->rhsp()->వెడల్పు_Min();
        // Sometimes doing the words backwards is preferable.
        // When we have x={x,foo} backwards is better, when x={foo,x} forward is better
        // However V3Subst tends to rip this up, so not worth optimizing now.
        కొరకు (పూర్ణాంకం w = 0; w < rhsp->వెడల్పు_పదాలు(); ++w) {
            కూడు_మాట_కేటాయించు(nodep, w,
                          కొత్త AstOr{fl, new_మాట_Grab_జరుపు(fl, w, rhsp->lhsp(), rhsshift),
                                    new_Ast_మాట_Sel_Clone(rhsp->rhsp(), w)});
        }
        తిరిగి నిజం;
    }

    శూన్య సందర్శించు(AstReplicate* nodep) override {
        ఒకవేళ (nodep->user1_సమితి_Once()) తిరిగి;  // Process once
        iterateChildren(nodep);
        ఒకవేళ (nodep->isWide()) {
            // See under ASSIGN(WIDE)
        } లేకపోతే {
            ఒకవేళ (isImpure(nodep)) తిరిగి;
            File_పంక్తి* స్థిరరాశి fl = nodep->fileline();
            Ast_కణుపు_సమాసం* lhsp = nodep->srcp()->unlink_Fr_వెనుక();
            Ast_కణుపు_సమాసం* newp;
            స్థిరరాశి పూర్ణాంకం lhswidth = lhsp->వెడల్పు_Min();
            ఒకవేళ (lhswidth == 1) {
                did_నమూనా(VExpand_నమూనా::REPLICATE_ONE_బిట్టు, nodep);
                newp = కొత్త AstNegate{fl, lhsp};
            } లేకపోతే {
                did_నమూనా(VExpand_నమూనా::REPLICATE, nodep);
                స్థిరరాశి AstConst* స్థిరరాశి constp = VN_AS(nodep->countp(), Const);
                UASSERT_OBJ(constp, nodep,
                            "Replication value isn't a constant.  Checked earlier!");
                స్థిరరాశి uint32_t times = constp->గమ్యం_UInt();
                ఒకవేళ (nodep->isQuad() && !lhsp->isQuad()) lhsp = కొత్త AstCCast{fl, lhsp, nodep};
                newp = lhsp->clone_వృక్షం_శుద్ధ(నిజం);
                కొరకు (గుర్తులేని repnum = 1; repnum < times; repnum++) {
                    స్థిరరాశి పూర్ణాంకం rhsshift = repnum * lhswidth;
                    newp = కొత్త AstOr{
                        fl,
                        కొత్త Ast_జరుపు_L{fl, lhsp->clone_వృక్షం_శుద్ధ(నిజం),
                                      కొత్త AstConst{fl, static_cast<uint32_t>(rhsshift)},
                                      nodep->వెడల్పు()},
                        newp};
                    newp->dtypeFrom(nodep);  // Unsigned
                }
                VL_DO_DANGLING(lhsp->delete_వృక్షం(), lhsp);  // Never used
            }
            newp->dtypeFrom(nodep);  // Unsigned
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        }
    }
    తార్కిక expandWide(Ast_కణుపు_కేటాయించు* nodep, AstReplicate* rhsp) {
        ఒకవేళ (!doExpandWide(rhsp)) తిరిగి అబద్ధం;
        did_నమూనా(VExpand_నమూనా::కేటాయించు_REPLICATE, nodep);
        File_పంక్తి* స్థిరరాశి fl = nodep->fileline();
        Ast_కణుపు_సమాసం* స్థిరరాశి lhsp = rhsp->srcp();
        స్థిరరాశి పూర్ణాంకం lhswidth = lhsp->వెడల్పు_Min();
        స్థిరరాశి AstConst* స్థిరరాశి constp = VN_AS(rhsp->countp(), Const);
        UASSERT_OBJ(constp, rhsp, "Replication value isn't a constant.  Checked earlier!");
        స్థిరరాశి uint32_t times = constp->గమ్యం_UInt();
        కొరకు (పూర్ణాంకం w = 0; w < rhsp->వెడల్పు_పదాలు(); ++w) {
            Ast_కణుపు_సమాసం* newp;
            ఒకవేళ (lhswidth == 1) {
                newp = కొత్త AstNegate{fl, lhsp->clone_వృక్షం_శుద్ధ(నిజం)};
                // Replicate always unsigned
                newp->dtype_Chg_వెడల్పు_Signed(VL_EDATASIZE, VL_EDATASIZE, VSigning::UNSIGNED);
            } లేకపోతే {
                newp = new_Ast_మాట_Sel_Clone(lhsp, w);
                File_పంక్తి* స్థిరరాశి rfl = rhsp->fileline();
                కొరకు (గుర్తులేని repnum = 1; repnum < times; repnum++) {
                    newp = కొత్త AstOr{fl, new_మాట_Grab_జరుపు(rfl, w, lhsp, lhswidth * repnum), newp};
                }
            }
            కూడు_మాట_కేటాయించు(nodep, w, newp);
        }
        తిరిగి నిజం;
    }

    శూన్య సందర్శించు_Eq_Neq(Ast_కణుపు_Biop* nodep) {
        ఒకవేళ (nodep->user1_సమితి_Once()) తిరిగి;  // Process once
        iterateChildren(nodep);
        ఒకవేళ (nodep->lhsp()->isWide()
            && !(VN_IS(nodep->lhsp()->dtypep()->skipRefp(), ప్రవాహం_DType)
                 || VN_IS(nodep->rhsp()->dtypep()->skipRefp(), ప్రవాహం_DType))) {
            ఒకవేళ (isImpure(nodep)) తిరిగి;
            ఒకవేళ (!doExpandWide(nodep->lhsp())) తిరిగి;
            ఒకవేళ (!doExpandWide(nodep->rhsp())) తిరిగి;
            did_నమూనా(VN_IS(nodep, Neq) ? VExpand_నమూనా::WORDIZE_NEQ
                                         : VExpand_నమూనా::WORDIZE_EQ,
                       nodep);
            // -> (0=={or{for each_word{WORDSEL(lhs,#)^WORDSEL(rhs,#)}}}
            File_పంక్తి* స్థిరరాశి fl = nodep->fileline();
            Ast_కణుపు_సమాసం* newp = శూన్యసూచిక;
            కొరకు (పూర్ణాంకం w = 0; w < nodep->lhsp()->వెడల్పు_పదాలు(); ++w) {
                Ast_కణుపు_సమాసం* స్థిరరాశి eqp = కొత్త AstXor{fl, new_Ast_మాట_Sel_Clone(nodep->lhsp(), w),
                                                    new_Ast_మాట_Sel_Clone(nodep->rhsp(), w)};
                newp = newp ? కొత్త AstOr{fl, newp, eqp} : eqp;
            }
            ఒకవేళ (VN_IS(nodep, Neq)) {
                newp = కొత్త AstNeq{fl, కొత్త AstConst{fl, AstConst::SizedEData{}, 0}, newp};
            } లేకపోతే {
                newp = కొత్త AstEq{fl, కొత్త AstConst{fl, AstConst::SizedEData{}, 0}, newp};
            }
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        }
    }
    శూన్య సందర్శించు(AstEq* nodep) override { సందర్శించు_Eq_Neq(nodep); }
    శూన్య సందర్శించు(AstNeq* nodep) override { సందర్శించు_Eq_Neq(nodep); }

    శూన్య సందర్శించు(AstRedOr* nodep) override {
        ఒకవేళ (nodep->user1_సమితి_Once()) తిరిగి;  // Process once
        iterateChildren(nodep);
        File_పంక్తి* స్థిరరాశి fl = nodep->fileline();
        ఒకవేళ (nodep->lhsp()->isWide()) {
            ఒకవేళ (isImpure(nodep)) తిరిగి;
            ఒకవేళ (!doExpandWide(nodep->lhsp())) తిరిగి;
            did_నమూనా(VExpand_నమూనా::WORDIZE_REDOR, nodep);
            // -> (0!={or{for each_word{WORDSEL(lhs,#)}}}
            Ast_కణుపు_సమాసం* newp = శూన్యసూచిక;
            కొరకు (పూర్ణాంకం w = 0; w < nodep->lhsp()->వెడల్పు_పదాలు(); ++w) {
                Ast_కణుపు_సమాసం* స్థిరరాశి eqp = new_Ast_మాట_Sel_Clone(nodep->lhsp(), w);
                newp = newp ? కొత్త AstOr{fl, newp, eqp} : eqp;
            }
            newp = కొత్త AstNeq{fl, కొత్త AstConst{fl, AstConst::SizedEData{}, 0}, newp};
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        } లేకపోతే {
            // No isImpure() check - can handle side effects in below
            did_నమూనా(VExpand_నమూనా::REDOR_గమ్యం_NEQ, nodep);
            Ast_కణుపు_సమాసం* స్థిరరాశి lhsp = nodep->lhsp()->unlink_Fr_వెనుక();
            Ast_కణుపు_సమాసం* స్థిరరాశి newp = కొత్త AstNeq{
                fl, కొత్త AstConst{fl, AstConst::Widthed_విలువ{}, long_Or_Quad_వెడల్పు(nodep), 0}, lhsp};
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        }
    }
    శూన్య సందర్శించు(AstRedAnd* nodep) override {
        ఒకవేళ (nodep->user1_సమితి_Once()) తిరిగి;  // Process once
        iterateChildren(nodep);
        File_పంక్తి* స్థిరరాశి fl = nodep->fileline();
        ఒకవేళ (nodep->lhsp()->isWide()) {
            ఒకవేళ (isImpure(nodep)) తిరిగి;
            ఒకవేళ (!doExpandWide(nodep->lhsp())) తిరిగి;
            did_నమూనా(VExpand_నమూనా::WORDIZE_REDAND, nodep);
            // -> (0!={and{for each_word{WORDSEL(lhs,#)}}}
            Ast_కణుపు_సమాసం* newp = శూన్యసూచిక;
            కొరకు (పూర్ణాంకం w = 0; w < nodep->lhsp()->వెడల్పు_పదాలు(); ++w) {
                Ast_కణుపు_సమాసం* eqp = new_Ast_మాట_Sel_Clone(nodep->lhsp(), w);
                ఒకవేళ (w == nodep->lhsp()->వెడల్పు_పదాలు() - 1) {
                    // Rather than doing a (slowish) ==##, we OR in the
                    // bits that aren't part of the mask
                    eqp = కొత్త AstOr{fl, కొత్త AstConst{fl, not_Wide_ముసుగు(nodep->lhsp())},
                                    // Bug in cppcheck
                                    // cppcheck-suppress memleak
                                    eqp};
                }
                newp = newp ? కొత్త AstAnd{fl, newp, eqp} : eqp;
            }
            newp = కొత్త AstEq{fl, కొత్త AstConst{fl, AstConst::SizedEData{}, VL_ముసుగు_E(VL_EDATASIZE)},
                             newp};
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        } లేకపోతే {
            // No isImpure() check - can handle side effects in below
            did_నమూనా(VExpand_నమూనా::REDAND_గమ్యం_EQ, nodep);
            Ast_కణుపు_సమాసం* స్థిరరాశి lhsp = nodep->lhsp()->unlink_Fr_వెనుక();
            Ast_కణుపు_సమాసం* స్థిరరాశి newp = కొత్త AstEq{fl, కొత్త AstConst{fl, మాట_ముసుగు(lhsp)}, lhsp};
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        }
    }
    శూన్య సందర్శించు(AstRedXor* nodep) override {
        ఒకవేళ (nodep->user1_సమితి_Once()) తిరిగి;  // Process once
        iterateChildren(nodep);
        ఒకవేళ (nodep->lhsp()->isWide()) {
            ఒకవేళ (isImpure(nodep)) తిరిగి;
            ఒకవేళ (!doExpandWide(nodep->lhsp())) తిరిగి;
            did_నమూనా(VExpand_నమూనా::WORDIZE_REDXOR, nodep);
            // -> (0!={redxor{for each_word{XOR(WORDSEL(lhs,#))}}}
            File_పంక్తి* స్థిరరాశి fl = nodep->fileline();
            Ast_కణుపు_సమాసం* newp = శూన్యసూచిక;
            కొరకు (పూర్ణాంకం w = 0; w < nodep->lhsp()->వెడల్పు_పదాలు(); ++w) {
                Ast_కణుపు_సమాసం* స్థిరరాశి eqp = new_Ast_మాట_Sel_Clone(nodep->lhsp(), w);
                newp = newp ? కొత్త AstXor{fl, newp, eqp} : eqp;
            }
            newp = కొత్త AstRedXor{fl, newp};
            UINFO(8, "    Wordize REDXORnew " << newp);
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        }
        // We don't reduce non-wide XORs, as its more efficient to use a temp register,
        // which the inlined function does nicely.
    }

    శూన్య సందర్శించు(Ast_కణుపు_Stmt* nodep) override {
        ఒకవేళ (nodep->user1_సమితి_Once()) తిరిగి;  // Process once
        VL_RESTORER(m_stmtp);
        m_stmtp = nodep;
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(Ast_కణుపు_కేటాయించు* nodep) override {
        ఒకవేళ (nodep->user1_సమితి_Once()) తిరిగి;  // Process once
        ఒకవేళ (VN_IS(nodep->dtypep()->skipRefp(), UnpackArrayDType)) {
            తిరిగి;  // Skip for UnpackArrayDType
        }

        VL_RESTORER(m_stmtp);
        m_stmtp = nodep;
        iterateChildren(nodep);
        తార్కిక did = అబద్ధం;
        ఒకవేళ (nodep->isWide()  //
            && ((VN_IS(nodep->lhsp(), VarRef) || VN_IS(nodep->lhsp(), ArraySel)))
            && !AstVar::scVarRecurse(nodep->lhsp())  // Need special function for SC
            && !AstVar::scVarRecurse(nodep->rhsp())) {
            ఒకవేళ (AstConst* స్థిరరాశి rhsp = VN_CAST(nodep->rhsp(), Const)) {
                did = expandWide(nodep, rhsp);
            } లేకపోతే ఒకవేళ (AstVarRef* స్థిరరాశి rhsp = VN_CAST(nodep->rhsp(), VarRef)) {
                did = expandWide(nodep, rhsp);
            } లేకపోతే ఒకవేళ (AstSel* స్థిరరాశి rhsp = VN_CAST(nodep->rhsp(), Sel)) {
                did = expandWide(nodep, rhsp);
            } లేకపోతే ఒకవేళ (AstArraySel* స్థిరరాశి rhsp = VN_CAST(nodep->rhsp(), ArraySel)) {
                did = expandWide(nodep, rhsp);
            } లేకపోతే ఒకవేళ (AstConcat* స్థిరరాశి rhsp = VN_CAST(nodep->rhsp(), Concat)) {
                did = expandWide(nodep, rhsp);
            } లేకపోతే ఒకవేళ (AstExtend* స్థిరరాశి rhsp = VN_CAST(nodep->rhsp(), Extend)) {
                did = expandWide(nodep, rhsp);
            } లేకపోతే ఒకవేళ (AstReplicate* స్థిరరాశి rhsp = VN_CAST(nodep->rhsp(), Replicate)) {
                did = expandWide(nodep, rhsp);
            } లేకపోతే ఒకవేళ (AstAnd* స్థిరరాశి rhsp = VN_CAST(nodep->rhsp(), And)) {
                did = expandWide(nodep, rhsp);
            } లేకపోతే ఒకవేళ (AstOr* స్థిరరాశి rhsp = VN_CAST(nodep->rhsp(), Or)) {
                did = expandWide(nodep, rhsp);
            } లేకపోతే ఒకవేళ (AstNot* స్థిరరాశి rhsp = VN_CAST(nodep->rhsp(), Not)) {
                did = expandWide(nodep, rhsp);
            } లేకపోతే ఒకవేళ (AstXor* స్థిరరాశి rhsp = VN_CAST(nodep->rhsp(), Xor)) {
                did = expandWide(nodep, rhsp);
            } లేకపోతే ఒకవేళ (Ast_జరుపు_L* స్థిరరాశి rhsp = VN_CAST(nodep->rhsp(), జరుపు_L)) {
                did = expand_Wide_జరుపు(nodep, rhsp, /* isLeftShift: */ నిజం);
            } లేకపోతే ఒకవేళ (Ast_జరుపు_R* స్థిరరాశి rhsp = VN_CAST(nodep->rhsp(), జరుపు_R)) {
                did = expand_Wide_జరుపు(nodep, rhsp, /* isLeftShift: */ అబద్ధం);
            } లేకపోతే ఒకవేళ (AstCond* స్థిరరాశి rhsp = VN_CAST(nodep->rhsp(), Cond)) {
                did = expandWide(nodep, rhsp);
            }
        } లేకపోతే ఒకవేళ (AstSel* స్థిరరాశి lhsp = VN_CAST(nodep->lhsp(), Sel)) {
            did = expandLhs(nodep, lhsp);
        }
        // Cleanup common code
        ఒకవేళ (did) VL_DO_DANGLING(nodep->unlink_Fr_వెనుక()->delete_వృక్షం(), nodep);
    }

    //--------------------
    శూన్య సందర్శించు(AstVar*) override {}  // Don't hit varrefs under vars
    శూన్య సందర్శించు(Ast_కణుపు* nodep) override { iterateChildren(nodep); }

బహిరంగ:
    // CONSTRUCTORS
    స్పష్టమైన ExpandVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~ExpandVisitor() override {
        V3Stats::కూడు_Stat("Optimizations, Expand, expanded wides", m_statWides);
        V3Stats::కూడు_Stat("Optimizations, Expand, expanded wide words", m_stat_Wide_పదాలు);
        V3Stats::కూడు_Stat("Optimizations, Expand, not expanded impure", m_statImpure);
        V3Stats::కూడు_Stat("Optimizations, Expand, not expanded limit", m_statWideLimited);
        V3Stats::కూడు_Stat("Optimizations, Expand, temporaries created", m_statTemps);
        స్థిరరాశి స్వయంచాలక emitStat = [ఇది](VExpand_నమూనా నమూనా_2) {
            std::string str{నమూనా_2.ascii()};
            std::transform(str.begin(), str.చివర(), str.begin(), [](గుర్తులేని అక్షరం c) {  //
                తిరిగి c == '_' ? ' ' : std::tolower(c);
            });
            V3Stats::కూడు_Stat("Optimizations, Expand, pattern " + str, m_statPatterns[నమూనా_2]);
        };
#నిర్వచించు EXPAND_నమూనా_EMIT_STAT(id) emitStat(VExpand_నమూనా::id);
        FOR_EACH_EXPAND_నమూనా(EXPAND_నమూనా_EMIT_STAT)
#నిర్వచనం_రద్దు EXPAND_నమూనా_EMIT_STAT
    }
};

//######################################################################
// Expand class functions

శూన్య V3Expand::expandAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        స్థిరరాశి ExpandOkVisitor okVisitor{nodep};
        ExpandVisitor{nodep};
    }  // Destruct before checking
    V3Global::dump_Check_Global_వృక్షం("expand", 0, dump_వృక్షం_Either_Level() >= 3);
}
