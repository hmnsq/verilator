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

#ਅਨ੍ਤਰ੍ਭਾਵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Expand.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Const.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Stats.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <algorithm>
#ਅਨ੍ਤਰ੍ਭਾਵ <array>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// The distinct expansion patterns, for statistics. One entry for each
// unique transformation ExpandVisitor applies. Keep sorted.

// clang-format off
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ FOR_EACH_EXPAND_ਨਮੂਨਾ(macro) \
    macro(ਸੌਂਪੋ_AND) \
    macro(ਸੌਂਪੋ_ARRAYSEL) \
    macro(ਸੌਂਪੋ_CONCAT) \
    macro(ਸੌਂਪੋ_COND) \
    macro(ਸੌਂਪੋ_CONST) \
    macro(ਸੌਂਪੋ_EXTEND) \
    macro(ਸੌਂਪੋ_NOT) \
    macro(ਸੌਂਪੋ_OR) \
    macro(ਸੌਂਪੋ_REPLICATE) \
    macro(ਸੌਂਪੋ_SEL_ALIGNED) \
    macro(ਸੌਂਪੋ_SEL_UNALIGNED) \
    macro(ਸੌਂਪੋ_SHIFTL) \
    macro(ਸੌਂਪੋ_SHIFTR) \
    macro(ਸੌਂਪੋ_ਮੰਜ਼ਿਲ_SEL_CONST_NARROW) \
    macro(ਸੌਂਪੋ_ਮੰਜ਼ਿਲ_SEL_CONST_WIDE) \
    macro(ਸੌਂਪੋ_ਮੰਜ਼ਿਲ_SEL_VAR_NARROW) \
    macro(ਸੌਂਪੋ_ਮੰਜ਼ਿਲ_SEL_VAR_WIDE_ONE_ਦੋਆਧਾਰੀ_ਅੰਕ) \
    macro(ਸੌਂਪੋ_VARREF) \
    macro(ਸੌਂਪੋ_XOR) \
    macro(CONCAT) \
    macro(REDAND_ਮੰਜ਼ਿਲ_EQ) \
    macro(REDOR_ਮੰਜ਼ਿਲ_NEQ) \
    macro(REPLICATE) \
    macro(REPLICATE_ONE_ਦੋਆਧਾਰੀ_ਅੰਕ) \
    macro(SEL_FROM_NARROW) \
    macro(SEL_FROM_WIDE) \
    macro(WORDIZE_EQ) \
    macro(WORDIZE_NEQ) \
    macro(WORDIZE_REDAND) \
    macro(WORDIZE_REDOR) \
    macro(WORDIZE_REDXOR)
// clang-format on

ਵਰਗ VExpand_ਨਮੂਨਾ final {
ਜਨਤਕ:
    ਗਿਣਤੀ en : uint8_t {
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ EXPAND_ਨਮੂਨਾ_ID(id) id,
        FOR_EACH_EXPAND_ਨਮੂਨਾ(EXPAND_ਨਮੂਨਾ_ID)
#ਪਰਿਭਾਸ਼ਾ_ਰੱਦ EXPAND_ਨਮੂਨਾ_ID
            ENUM_ਸਿਰਾ
    };
    ਗਿਣਤੀ en m_e;
    ਅਚਲ ਅੱਖਰ* ascii() ਅਚਲ {
        ਸਥਿਰ ਅਚਲ ਅੱਖਰ* ਅਚਲ ਨਾਂ[] = {
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ EXPAND_ਨਮੂਨਾ_NAME(id) #id,
            FOR_EACH_EXPAND_ਨਮੂਨਾ(EXPAND_ਨਮੂਨਾ_NAME)
#ਪਰਿਭਾਸ਼ਾ_ਰੱਦ EXPAND_ਨਮੂਨਾ_NAME
                "_ENUM_END"  //
        };
        ਵਾਪਸ ਨਾਂ[m_e];
    }

    // cppcheck-suppress noExplicitConstructor
    VExpand_ਨਮੂਨਾ(en _e)
        : m_e{_e} {}
    ਸੰਚਾਲਕ en() ਅਚਲ { ਵਾਪਸ m_e; }
};

//######################################################################
// Find nodes with side effects, to mark as non-expandable

ਵਰਗ ExpandOkVisitor final : ਜਨਤਕ VNVisitorConst {
    // NODE STATE
    //  AstNode::user2()        -> bool.  Is pure (along with all children)
    ਅਚਲ VNUser2_ਵਿੱਚ_Use m_inuser2;

    // STATE - for current visit position (use VL_RESTORER)
    // Tracks similar to AstNode::isTreePureRecurse(), but avoid O(n^2)
    // False = pure, as nodes that ExpandVisitor inserts preserve pureness
    ਤਾਰਕਿਕ m_isImpure = ਸੱਚ;  // Currently pure

    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override {
        ਤਾਰਕਿਕ selfImpure = !nodep->is_ਸ਼ੁੱਧ();
        {
            VL_RESTORER(m_isImpure);
            m_isImpure = ਝੂਠ;
            iterateChildrenConst(nodep);
            selfImpure |= m_isImpure;
            nodep->user2(selfImpure);
        }
        m_isImpure |= selfImpure;
    }

ਜਨਤਕ:
    // CONSTRUCTORS
    ਸਪਸ਼ਟ ExpandOkVisitor(AstNetlist* nodep) { iterateConst(nodep); }
    ~ExpandOkVisitor() = ਮੂਲ;
};

//######################################################################
// Expand state, as a visitor of each AstNode

ਵਰਗ ExpandVisitor final : ਜਨਤਕ VNVisitor {
    // NODE STATE
    //  AstNode::user1()        -> bool.  Processed
    //  AstNode::user2()        -> See ExpandOkVisitor
    ਅਚਲ VNUser1_ਵਿੱਚ_Use m_inuser1;

    // STATE - for current visit position (use VL_RESTORER)
    Ast_ਗੰਢ* m_stmtp = ਖਾਲੀ_ਸੂਚਕ;  // Current statement

    // STATE - across all visitors
    AstCFunc* m_funcp = ਖਾਲੀ_ਸੂਚਕ;  // Current function
    VDouble0 m_statImpure;  // Statistic tracking
    VDouble0 m_statWides;  // Statistic tracking
    VDouble0 m_stat_Wide_ਸ਼ਬਦ;  // Statistic tracking
    VDouble0 m_statWideLimited;  // Statistic tracking
    VDouble0 m_statTemps;  // Statistic tracking
    // Number of applications of each expansion pattern for statistics
    std::array<VDouble0, VExpand_ਨਮੂਨਾ::ENUM_ਸਿਰਾ> m_statPatterns;

    // STATE - for current function
    ਮਾਪ_t m_nTmps = 0;  // Sequence numbers for temopraries

    // METHODS
    // Note the application of the given expansion pattern to 'nodep'
    ਖਾਲੀ did_ਨਮੂਨਾ(VExpand_ਨਮੂਨਾ ਨਮੂਨਾ_2, Ast_ਗੰਢ* nodep) {
        UINFO(8, "    " << ਨਮੂਨਾ_2.ascii() << " " << nodep);
        ++m_statPatterns[ਨਮੂਨਾ_2];
    }

    // Use state that ExpandOkVisitor calculated
    ਸਥਿਰ ਤਾਰਕਿਕ isImpure(Ast_ਗੰਢ* nodep) {
        ਅਚਲ ਤਾਰਕਿਕ impure = nodep->user2();
        ਜੇ (impure) UINFO(9, "      impure " << nodep);
        ਵਾਪਸ impure;
    }

    ਤਾਰਕਿਕ doExpandWide(Ast_ਗੰਢ* nodep) {
        ਜੇ (isImpure(nodep)) {
            ++m_statImpure;
            ਵਾਪਸ ਝੂਠ;
        }
        ਜੇ (nodep->ਚੌੜਾਈ_ਸ਼ਬਦ() <= v3Global.opt.expand_ਹੱਦਬੰਦੀ()) {
            ++m_statWides;
            m_stat_Wide_ਸ਼ਬਦ += nodep->ਚੌੜਾਈ_ਸ਼ਬਦ();
            ਵਾਪਸ ਸੱਚ;
        } ਨਹੀਂਤਾਂ {
            ++m_statWideLimited;
            ਵਾਪਸ ਝੂਠ;
        }
    }

    ਸਥਿਰ ਪੂਰਨ long_Or_Quad_ਚੌੜਾਈ(Ast_ਗੰਢ* nodep) {
        ਵਾਪਸ (nodep->ਚੌੜਾਈ() + (VL_EDATASIZE - 1)) & ~(VL_EDATASIZE - 1);
    }
    ਸਥਿਰ V3_ਸੰਖਿਆ not_Wide_ਨਕਾਬ(Ast_ਗੰਢ* nodep) {
        ਵਾਪਸ V3_ਸੰਖਿਆ{nodep, VL_EDATASIZE, ~VL_ਨਕਾਬ_E(nodep->ਚੌੜਾਈ_Min())};
    }
    ਸਥਿਰ V3_ਸੰਖਿਆ ਸ਼ਬਦ_ਨਕਾਬ(Ast_ਗੰਢ* nodep) {
        ਜੇ (nodep->isWide()) {
            ਵਾਪਸ V3_ਸੰਖਿਆ{nodep, VL_EDATASIZE, VL_ਨਕਾਬ_E(nodep->ਚੌੜਾਈ_Min())};
        } ਨਹੀਂਤਾਂ {
            V3_ਸੰਖਿਆ ਨਕਾਬ{nodep, long_Or_Quad_ਚੌੜਾਈ(nodep)};
            ਨਕਾਬ.ਸੈੱਟ_ਨਕਾਬ(nodep->ਚੌੜਾਈ_Min());
            ਵਾਪਸ ਨਕਾਬ;
        }
    }

    ਸਥਿਰ ਖਾਲੀ ਪਾਓ_Before(Ast_ਗੰਢ* placep, Ast_ਗੰਢ* newp) {
        newp->user1(1);  // Already processed, don't need to re-iterate
        placep->ਜੋੜੋ_ਇੱਥੇ_This_As_ਅਗਲਾ(newp);
    }
    ਸਥਿਰ ਖਾਲੀ replaceWithDelete(Ast_ਗੰਢ* nodep, Ast_ਗੰਢ* newp) {
        newp->user1(1);  // Already processed, don't need to re-iterate
        ਜੇ (newp->ਚੌੜਾਈ() != nodep->ਚੌੜਾਈ()) {
            UASSERT_OBJ(newp->ਚੌੜਾਈ_Min() == nodep->ਚੌੜਾਈ_Min(), nodep,
                        "Replacement width mismatch");
            newp->dtype_Chg_ਚੌੜਾਈ(nodep->ਚੌੜਾਈ(), nodep->ਚੌੜਾਈ_Min());
        }
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_ਰੁੱਖ(), nodep);
    }
    ਸਥਿਰ Ast_ਗੰਢ* new_ਸ਼ਬਦ_ਸੌਂਪੋ(Ast_ਗੰਢ_ਸੌਂਪੋ* placep, ਪੂਰਨ ਸ਼ਬਦ, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp,
                                  Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp) {
        File_ਸਤਰ* ਅਚਲ fl = placep->fileline();
        ਵਾਪਸ ਨਵਾਂ Ast_ਸੌਂਪੋ{fl,
                             ਨਵਾਂ Ast_ਸ਼ਬਦ_Sel{fl, lhsp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਸੱਚ),
                                            ਨਵਾਂ AstConst{fl, static_cast<uint32_t>(ਸ਼ਬਦ)}},
                             rhsp};
    }
    ਸਥਿਰ ਖਾਲੀ ਜੋੜੋ_ਸ਼ਬਦ_ਸੌਂਪੋ(Ast_ਗੰਢ_ਸੌਂਪੋ* placep, ਪੂਰਨ ਸ਼ਬਦ, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp,
                              Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp) {
        ਪਾਓ_Before(placep, new_ਸ਼ਬਦ_ਸੌਂਪੋ(placep, ਸ਼ਬਦ, lhsp, rhsp));
    }
    ਸਥਿਰ ਖਾਲੀ ਜੋੜੋ_ਸ਼ਬਦ_ਸੌਂਪੋ(Ast_ਗੰਢ_ਸੌਂਪੋ* placep, ਪੂਰਨ ਸ਼ਬਦ, Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp) {
        ਜੋੜੋ_ਸ਼ਬਦ_ਸੌਂਪੋ(placep, ਸ਼ਬਦ, placep->lhsp(), rhsp);
    }

    AstVar* ਜੋੜੋ_Local_Tmp(Ast_ਗੰਢ* placep, ਅਚਲ ਅੱਖਰ* namep, Ast_ਗੰਢ_ਵਿਅੰਜਕ* valuep) {
        ਅਚਲ std::string name = "__V"s + namep + "_"s + std::ਮੰਜ਼ਿਲ_string(m_nTmps);
        File_ਸਤਰ* ਅਚਲ flp = placep->fileline();
        AstVar* ਅਚਲ tmpp = ਨਵਾਂ AstVar{flp, VVarType::STMTTEMP, name, valuep->dtypep()};
        tmpp->funcLocal(ਸੱਚ);
        tmpp->is_ਅੰਦਰੂਨੀ(ਸੱਚ);
        tmpp->noReset(ਸੱਚ);
        tmpp->substConstOnly(ਸੱਚ);
        m_funcp->ਜੋੜੋ_Varsp(tmpp);
        ਪਾਓ_Before(placep, ਨਵਾਂ Ast_ਸੌਂਪੋ{flp, ਨਵਾਂ AstVarRef{flp, tmpp, VAccess::WRITE}, valuep});
        ++m_statTemps;
        ਵਾਪਸ tmpp;
    }

    ਸਥਿਰ ਖਾਲੀ fixCloneLvalue(Ast_ਗੰਢ* nodep) {
        // In AstSel transforms, we call clone() on VarRefs that were lvalues,
        // but are now being used on the RHS of the assignment
        ਜੇ (VN_IS(nodep, VarRef)) VN_AS(nodep, VarRef)->access(VAccess::READ);
        // Iterate
        ਜੇ (Ast_ਗੰਢ* ਅਚਲ refp = nodep->op1p()) fixCloneLvalue(refp);
        ਜੇ (Ast_ਗੰਢ* ਅਚਲ refp = nodep->op2p()) fixCloneLvalue(refp);
        ਜੇ (Ast_ਗੰਢ* ਅਚਲ refp = nodep->op3p()) fixCloneLvalue(refp);
        ਜੇ (Ast_ਗੰਢ* ਅਚਲ refp = nodep->op4p()) fixCloneLvalue(refp);
    }

    ਸਥਿਰ Ast_ਗੰਢ_ਵਿਅੰਜਕ* new_Ast_ਸ਼ਬਦ_Sel_Clone(Ast_ਗੰਢ_ਵਿਅੰਜਕ* nodep, ਪੂਰਨ ਸ਼ਬਦ) {
        // Get the specified word number from a wide array
        // Or, if it's a long/quad, do appropriate conversion to wide
        // Concat may pass negative word numbers, that means it wants a zero
        File_ਸਤਰ* ਅਚਲ fl = nodep->fileline();
        ਜੇ (nodep->isWide() && ਸ਼ਬਦ >= 0 && ਸ਼ਬਦ < nodep->ਚੌੜਾਈ_ਸ਼ਬਦ()) {
            ਵਾਪਸ ਨਵਾਂ Ast_ਸ਼ਬਦ_Sel{fl, nodep->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਸੱਚ),
                                  ਨਵਾਂ AstConst{fl, static_cast<uint32_t>(ਸ਼ਬਦ)}};
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->isQuad() && ਸ਼ਬਦ == 0) {
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ quadfromp = nodep->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਸੱਚ);
            quadfromp->dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ_Unsized(VL_QUADSIZE, quadfromp->ਚੌੜਾਈ_Min(), VSigning::UNSIGNED);
            ਵਾਪਸ ਨਵਾਂ AstCCast{fl, quadfromp, VL_EDATASIZE};
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->isQuad() && ਸ਼ਬਦ == 1) {
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ quadfromp = nodep->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਸੱਚ);
            quadfromp->dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ_Unsized(VL_QUADSIZE, quadfromp->ਚੌੜਾਈ_Min(), VSigning::UNSIGNED);
            ਵਾਪਸ ਨਵਾਂ AstCCast{
                fl, ਨਵਾਂ Ast_ਖਿਸਕਾਓ_R{fl, quadfromp, ਨਵਾਂ AstConst{fl, VL_EDATASIZE}, VL_EDATASIZE},
                VL_EDATASIZE};
        } ਨਹੀਂਤਾਂ ਜੇ (!nodep->isWide() && !nodep->isQuad() && ਸ਼ਬਦ == 0) {
            ਵਾਪਸ nodep->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਸੱਚ);
        } ਨਹੀਂਤਾਂ {  // Out of bounds
            ਵਾਪਸ ਨਵਾਂ AstConst{fl, 0};
        }
    }

    ਸਥਿਰ Ast_ਗੰਢ_ਵਿਅੰਜਕ* new_ਸ਼ਬਦ_Grab_ਖਿਸਕਾਓ(File_ਸਤਰ* fl, ਪੂਰਨ ਸ਼ਬਦ, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, ਪੂਰਨ ਖਿਸਕਾਓ_2) {
        // Extract the expression to grab the value for the specified word, if it's the shift
        // of shift bits from lhsp
        // Negative word numbers requested for lhs when it's "before" what we want.
        // We get a 0 then.
        ਅਚਲ ਪੂਰਨ othword = ਸ਼ਬਦ - ਖਿਸਕਾਓ_2 / VL_EDATASIZE;
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ lop = new_Ast_ਸ਼ਬਦ_Sel_Clone(lhsp, othword);
        ਅਚਲ uint32_t lo_ਖਿਸਕਾਓ = VL_BITBIT_E(ਖਿਸਕਾਓ_2);
        ਜੇ (!lo_ਖਿਸਕਾਓ) ਵਾਪਸ lop;
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ hip = new_Ast_ਸ਼ਬਦ_Sel_Clone(lhsp, othword - 1);
        ਅਚਲ uint32_t hi_ਖਿਸਕਾਓ = VL_EDATASIZE - lo_ਖਿਸਕਾਓ;  // Complement offset
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ newp
            = ਨਵਾਂ AstOr{fl,  //
                        ਨਵਾਂ Ast_ਖਿਸਕਾਓ_R{fl, hip, ਨਵਾਂ AstConst{fl, hi_ਖਿਸਕਾਓ}, VL_EDATASIZE},
                        ਨਵਾਂ Ast_ਖਿਸਕਾਓ_L{fl, lop, ਨਵਾਂ AstConst{fl, lo_ਖਿਸਕਾਓ}, VL_EDATASIZE}};
        ਵਾਪਸ V3Const::constifyEditCpp(newp);
    }

    // Return expression indexing the word that contains 'lsbp' + the given word offset
    ਸਥਿਰ Ast_ਗੰਢ_ਵਿਅੰਜਕ* new_ਸ਼ਬਦ_ਸੂਚਕਾਂਕ(Ast_ਗੰਢ_ਵਿਅੰਜਕ* lsbp, uint32_t ਸ਼ਬਦ_ਵਿੱਥ = 0) {
        // This is indexing a WordSel, so a 32 bit constants are fine
        File_ਸਤਰ* ਅਚਲ flp = lsbp->fileline();
        ਜੇ (ਅਚਲ AstConst* constp = VN_CAST(lsbp, Const)) {
            ਵਾਪਸ ਨਵਾਂ AstConst{flp, ਸ਼ਬਦ_ਵਿੱਥ + VL_BITWORD_E(constp->ਮੰਜ਼ਿਲ_UInt())};
        }

        ਜੇ (lsbp->backp()) lsbp = lsbp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ);
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ wwl2p = ਨਵਾਂ AstConst{flp, VL_EDATASIZE_LOG2};  // Word width log 2
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* indexp = ਨਵਾਂ Ast_ਖਿਸਕਾਓ_R{flp, lsbp, wwl2p, VL_EDATASIZE};
        ਜੇ (ਸ਼ਬਦ_ਵਿੱਥ) indexp = ਨਵਾਂ Ast_ਜੋੜੋ{flp, ਨਵਾਂ AstConst{flp, ਸ਼ਬਦ_ਵਿੱਥ}, indexp};
        ਵਾਪਸ indexp;
    }

    // Return word of fromp that contains word indexp + the given word offset.
    ਸਥਿਰ Ast_ਗੰਢ_ਵਿਅੰਜਕ* new_ਸ਼ਬਦ_Sel_ਸ਼ਬਦ(File_ਸਤਰ* flp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* fromp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* indexp,
                                       uint32_t ਸ਼ਬਦ_ਵਿੱਥ = 0) {
        UASSERT_OBJ(fromp->isWide(), fromp, "Only need AstWordSel on wide from's");

        ਜੇ (ਅਚਲ AstConst* ਅਚਲ constp = VN_CAST(indexp, Const)) {
            indexp = ਖਾਲੀ_ਸੂਚਕ;
            ਸ਼ਬਦ_ਵਿੱਥ += constp->ਮੰਜ਼ਿਲ_UInt();
        }

        // e.g. "logic [95:0] var[0]; logic [0] sel; out = var[sel];"
        // Squash before C++ to avoid getting a C++ compiler warning
        // (even though code would be unreachable as presumably a
        // AstCond is protecting above this node.
        ਜੇ (ਸ਼ਬਦ_ਵਿੱਥ >= static_cast<uint32_t>(fromp->ਚੌੜਾਈ_ਸ਼ਬਦ())) {
            ਵਾਪਸ ਨਵਾਂ AstConst{flp, AstConst::SizedEData{}, 0};
        }

        ਜੇ (fromp->backp()) fromp = fromp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ);

        // If indexp was constant, just use it
        ਜੇ (!indexp) ਵਾਪਸ ਨਵਾਂ Ast_ਸ਼ਬਦ_Sel{flp, fromp, ਨਵਾਂ AstConst{flp, ਸ਼ਬਦ_ਵਿੱਥ}};

        // Otherwise compute at runtime
        ਜੇ (indexp->backp()) indexp = indexp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ);
        ਜੇ (ਸ਼ਬਦ_ਵਿੱਥ) indexp = ਨਵਾਂ Ast_ਜੋੜੋ{flp, ਨਵਾਂ AstConst{flp, ਸ਼ਬਦ_ਵਿੱਥ}, indexp};
        ਵਾਪਸ ਨਵਾਂ Ast_ਸ਼ਬਦ_Sel{flp, fromp, indexp};
    }

    // Return word of fromp that contains bit lsbp + the given word offset.
    ਸਥਿਰ Ast_ਗੰਢ_ਵਿਅੰਜਕ* new_ਸ਼ਬਦ_Sel_ਦੋਆਧਾਰੀ_ਅੰਕ(File_ਸਤਰ* flp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* fromp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lsbp,
                                      uint32_t ਸ਼ਬਦ_ਵਿੱਥ = 0) {
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ indexp = new_ਸ਼ਬਦ_ਸੂਚਕਾਂਕ(lsbp, ਸ਼ਬਦ_ਵਿੱਥ);
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ ਸ਼ਬਦ_Selp = new_ਸ਼ਬਦ_Sel_ਸ਼ਬਦ(flp, fromp, indexp);
        ਜੇ (!indexp->backp()) VL_DO_DANGLING(indexp->delete_ਰੁੱਖ(), indexp);
        ਵਾਪਸ ਸ਼ਬਦ_Selp;
    }

    ਸਥਿਰ Ast_ਗੰਢ_ਵਿਅੰਜਕ* new_Sel_ਦੋਆਧਾਰੀ_ਅੰਕ_ਦੋਆਧਾਰੀ_ਅੰਕ(Ast_ਗੰਢ_ਵਿਅੰਜਕ* lsbp) {
        // Return equation to get the VL_BITBIT of a constant or non-constant
        File_ਸਤਰ* ਅਚਲ fl = lsbp->fileline();
        ਜੇ (VN_IS(lsbp, Const)) {
            ਵਾਪਸ ਨਵਾਂ AstConst{fl, VL_BITBIT_E(VN_AS(lsbp, Const)->ਮੰਜ਼ਿਲ_UInt())};
        } ਨਹੀਂਤਾਂ {
            ਵਾਪਸ ਨਵਾਂ AstAnd{fl, ਨਵਾਂ AstConst{fl, VL_EDATASIZE - 1}, lsbp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਸੱਚ)};
        }
    }

    //====================

    ਤਾਰਕਿਕ expandWide(Ast_ਗੰਢ_ਸੌਂਪੋ* nodep, AstConst* rhsp) {
        ਜੇ (!doExpandWide(nodep)) ਵਾਪਸ ਝੂਠ;
        did_ਨਮੂਨਾ(VExpand_ਨਮੂਨਾ::ਸੌਂਪੋ_CONST, nodep);
        // -> {for each_word{ ASSIGN(WORDSEL(wide,#),WORDSEL(CONST,#))}}
        ਜੇ (rhsp->ਅੰਕ().isFourState()) {
            rhsp->v3warn(E_UNSUPPORTED,  // LCOV_EXCL_LINE  // impossible?
                         "Unsupported: 4-state numbers in this context");
        }
        File_ਸਤਰ* ਅਚਲ fl = nodep->fileline();
        ਲਈ (ਪੂਰਨ w = 0; w < nodep->ਚੌੜਾਈ_ਸ਼ਬਦ(); ++w) {
            ਜੋੜੋ_ਸ਼ਬਦ_ਸੌਂਪੋ(nodep, w,
                          ਨਵਾਂ AstConst{fl, AstConst::SizedEData{}, rhsp->ਅੰਕ().edata_ਸ਼ਬਦ(w)});
        }
        ਵਾਪਸ ਸੱਚ;
    }
    //-------- Uniops
    ਤਾਰਕਿਕ expandWide(Ast_ਗੰਢ_ਸੌਂਪੋ* nodep, AstVarRef* rhsp) {
        // Special case: do not expand assignment of constant pool variables.
        // V3Subst undestands these directly.
        ਜੇ (rhsp->varp()->const_Pool_ਇੰਦਰਾਜ()) ਵਾਪਸ ਝੂਠ;
        ਜੇ (!doExpandWide(nodep)) ਵਾਪਸ ਝੂਠ;
        did_ਨਮੂਨਾ(VExpand_ਨਮੂਨਾ::ਸੌਂਪੋ_VARREF, nodep);
        ਲਈ (ਪੂਰਨ w = 0; w < nodep->ਚੌੜਾਈ_ਸ਼ਬਦ(); ++w) {
            ਜੋੜੋ_ਸ਼ਬਦ_ਸੌਂਪੋ(nodep, w, new_Ast_ਸ਼ਬਦ_Sel_Clone(rhsp, w));
        }
        ਵਾਪਸ ਸੱਚ;
    }
    ਤਾਰਕਿਕ expandWide(Ast_ਗੰਢ_ਸੌਂਪੋ* nodep, AstArraySel* rhsp) {
        UASSERT_OBJ(!VN_IS(nodep->dtypep()->skipRefp(), UnpackArrayDType), nodep,
                    "ArraySel with unpacked arrays should have been removed in V3Slice");
        ਜੇ (!doExpandWide(nodep)) ਵਾਪਸ ਝੂਠ;
        did_ਨਮੂਨਾ(VExpand_ਨਮੂਨਾ::ਸੌਂਪੋ_ARRAYSEL, nodep);
        ਲਈ (ਪੂਰਨ w = 0; w < nodep->ਚੌੜਾਈ_ਸ਼ਬਦ(); ++w) {
            ਜੋੜੋ_ਸ਼ਬਦ_ਸੌਂਪੋ(nodep, w, new_Ast_ਸ਼ਬਦ_Sel_Clone(rhsp, w));
        }
        ਵਾਪਸ ਸੱਚ;
    }
    ਤਾਰਕਿਕ expandWide(Ast_ਗੰਢ_ਸੌਂਪੋ* nodep, AstNot* rhsp) {
        // -> {for each_word{ ASSIGN(WORDSEL(wide,#),NOT(WORDSEL(lhs,#))) }}
        ਜੇ (!doExpandWide(nodep)) ਵਾਪਸ ਝੂਠ;
        did_ਨਮੂਨਾ(VExpand_ਨਮੂਨਾ::ਸੌਂਪੋ_NOT, nodep);
        File_ਸਤਰ* ਅਚਲ fl = rhsp->fileline();
        ਲਈ (ਪੂਰਨ w = 0; w < nodep->ਚੌੜਾਈ_ਸ਼ਬਦ(); ++w) {
            ਜੋੜੋ_ਸ਼ਬਦ_ਸੌਂਪੋ(nodep, w, ਨਵਾਂ AstNot{fl, new_Ast_ਸ਼ਬਦ_Sel_Clone(rhsp->lhsp(), w)});
        }
        ਵਾਪਸ ਸੱਚ;
    }
    //-------- Biops
    ਤਾਰਕਿਕ expandWide(Ast_ਗੰਢ_ਸੌਂਪੋ* nodep, AstAnd* rhsp) {
        ਜੇ (!doExpandWide(nodep)) ਵਾਪਸ ਝੂਠ;
        did_ਨਮੂਨਾ(VExpand_ਨਮੂਨਾ::ਸੌਂਪੋ_AND, nodep);
        File_ਸਤਰ* ਅਚਲ fl = nodep->fileline();
        ਲਈ (ਪੂਰਨ w = 0; w < nodep->ਚੌੜਾਈ_ਸ਼ਬਦ(); ++w) {
            ਜੋੜੋ_ਸ਼ਬਦ_ਸੌਂਪੋ(nodep, w,
                          ਨਵਾਂ AstAnd{fl, new_Ast_ਸ਼ਬਦ_Sel_Clone(rhsp->lhsp(), w),
                                     new_Ast_ਸ਼ਬਦ_Sel_Clone(rhsp->rhsp(), w)});
        }
        ਵਾਪਸ ਸੱਚ;
    }
    ਤਾਰਕਿਕ expandWide(Ast_ਗੰਢ_ਸੌਂਪੋ* nodep, AstOr* rhsp) {
        ਜੇ (!doExpandWide(nodep)) ਵਾਪਸ ਝੂਠ;
        did_ਨਮੂਨਾ(VExpand_ਨਮੂਨਾ::ਸੌਂਪੋ_OR, nodep);
        File_ਸਤਰ* ਅਚਲ fl = nodep->fileline();
        ਲਈ (ਪੂਰਨ w = 0; w < nodep->ਚੌੜਾਈ_ਸ਼ਬਦ(); ++w) {
            ਜੋੜੋ_ਸ਼ਬਦ_ਸੌਂਪੋ(nodep, w,
                          ਨਵਾਂ AstOr{fl, new_Ast_ਸ਼ਬਦ_Sel_Clone(rhsp->lhsp(), w),
                                    new_Ast_ਸ਼ਬਦ_Sel_Clone(rhsp->rhsp(), w)});
        }
        ਵਾਪਸ ਸੱਚ;
    }
    ਤਾਰਕਿਕ expandWide(Ast_ਗੰਢ_ਸੌਂਪੋ* nodep, AstXor* rhsp) {
        ਜੇ (!doExpandWide(nodep)) ਵਾਪਸ ਝੂਠ;
        did_ਨਮੂਨਾ(VExpand_ਨਮੂਨਾ::ਸੌਂਪੋ_XOR, nodep);
        File_ਸਤਰ* ਅਚਲ fl = nodep->fileline();
        ਲਈ (ਪੂਰਨ w = 0; w < nodep->ਚੌੜਾਈ_ਸ਼ਬਦ(); ++w) {
            ਜੋੜੋ_ਸ਼ਬਦ_ਸੌਂਪੋ(nodep, w,
                          ਨਵਾਂ AstXor{fl, new_Ast_ਸ਼ਬਦ_Sel_Clone(rhsp->lhsp(), w),
                                     new_Ast_ਸ਼ਬਦ_Sel_Clone(rhsp->rhsp(), w)});
        }
        ਵਾਪਸ ਸੱਚ;
    }
    ਤਾਰਕਿਕ expand_Wide_ਖਿਸਕਾਓ(Ast_ਗੰਢ_ਸੌਂਪੋ* nodep, Ast_ਗੰਢ_Biop* rhsp, ਤਾਰਕਿਕ is_ਖੱਬਾ_ਖਿਸਕਾਓ) {
        ਜੇ (!doExpandWide(nodep)) ਵਾਪਸ ਝੂਠ;

        // Simplify the shift amount, in case it becomes a constant
        V3Const::constifyEditCpp(rhsp->rhsp());

        // If it's a constant shift by whole words, expand it so V3Subst can substitute it
        ਜੇ (ਅਚਲ AstConst* ਅਚਲ rhsConstp = VN_CAST(rhsp->rhsp(), Const)) {
            ਅਚਲ uint32_t ਖਿਸਕਾਓ_ਬਿੱਟ = rhsConstp->ਮੰਜ਼ਿਲ_UInt();
            ਜੇ (VL_BITBIT_E(ਖਿਸਕਾਓ_ਬਿੱਟ) == 0) {
                ਅਚਲ ਪੂਰਨ ਚੌੜਾਈ_ਸ਼ਬਦ = nodep->ਚੌੜਾਈ_ਸ਼ਬਦ();
                ਅਚਲ ਪੂਰਨ ਖਿਸਕਾਓ_ਸ਼ਬਦ = std::min<ਪੂਰਨ>(VL_BITWORD_E(ਖਿਸਕਾਓ_ਬਿੱਟ), ਚੌੜਾਈ_ਸ਼ਬਦ);
                File_ਸਤਰ* ਅਚਲ flp = rhsp->fileline();
                ਜੇ (is_ਖੱਬਾ_ਖਿਸਕਾਓ) {
                    did_ਨਮੂਨਾ(VExpand_ਨਮੂਨਾ::ਸੌਂਪੋ_SHIFTL, nodep);
                    // Low words of the result are zero
                    ਲਈ (ਪੂਰਨ w = 0; w < ਖਿਸਕਾਓ_ਸ਼ਬਦ; ++w) {
                        ਜੋੜੋ_ਸ਼ਬਦ_ਸੌਂਪੋ(nodep, w, ਨਵਾਂ AstConst{flp, AstConst::SizedEData{}, 0});
                    }
                    // High words of the result are copied from higher words of the source
                    ਲਈ (ਪੂਰਨ w = ਖਿਸਕਾਓ_ਸ਼ਬਦ; w < ਚੌੜਾਈ_ਸ਼ਬਦ; ++w) {
                        ਜੋੜੋ_ਸ਼ਬਦ_ਸੌਂਪੋ(nodep, w, new_Ast_ਸ਼ਬਦ_Sel_Clone(rhsp->lhsp(), w - ਖਿਸਕਾਓ_ਸ਼ਬਦ));
                    }
                } ਨਹੀਂਤਾਂ {
                    did_ਨਮੂਨਾ(VExpand_ਨਮੂਨਾ::ਸੌਂਪੋ_SHIFTR, nodep);
                    // Low words of the result are copied from higher words of the source
                    ਲਈ (ਪੂਰਨ w = 0; w < ਚੌੜਾਈ_ਸ਼ਬਦ - ਖਿਸਕਾਓ_ਸ਼ਬਦ; ++w) {
                        ਜੋੜੋ_ਸ਼ਬਦ_ਸੌਂਪੋ(nodep, w, new_Ast_ਸ਼ਬਦ_Sel_Clone(rhsp->lhsp(), w + ਖਿਸਕਾਓ_ਸ਼ਬਦ));
                    }
                    // High words of the result are zero
                    ਲਈ (ਪੂਰਨ w = ਚੌੜਾਈ_ਸ਼ਬਦ - ਖਿਸਕਾਓ_ਸ਼ਬਦ; w < ਚੌੜਾਈ_ਸ਼ਬਦ; ++w) {
                        ਜੋੜੋ_ਸ਼ਬਦ_ਸੌਂਪੋ(nodep, w, ਨਵਾਂ AstConst{flp, AstConst::SizedEData{}, 0});
                    }
                }
                ਵਾਪਸ ਸੱਚ;
            }
        }

        ਵਾਪਸ ਝੂਠ;
    }

    //-------- Triops
    ਤਾਰਕਿਕ expandWide(Ast_ਗੰਢ_ਸੌਂਪੋ* nodep, AstCond* rhsp) {
        ਜੇ (!doExpandWide(nodep)) ਵਾਪਸ ਝੂਠ;
        did_ਨਮੂਨਾ(VExpand_ਨਮੂਨਾ::ਸੌਂਪੋ_COND, nodep);
        File_ਸਤਰ* ਅਚਲ fl = nodep->fileline();
        ਲਈ (ਪੂਰਨ w = 0; w < nodep->ਚੌੜਾਈ_ਸ਼ਬਦ(); ++w) {
            ਜੋੜੋ_ਸ਼ਬਦ_ਸੌਂਪੋ(nodep, w,
                          ਨਵਾਂ AstCond{fl, rhsp->condp()->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਸੱਚ),
                                      new_Ast_ਸ਼ਬਦ_Sel_Clone(rhsp->thenp(), w),
                                      new_Ast_ਸ਼ਬਦ_Sel_Clone(rhsp->elsep(), w)});
        }
        ਵਾਪਸ ਸੱਚ;
    }

    // VISITORS
    ਖਾਲੀ ਦੇਖੋ(AstNetlist* nodep) override { iterate_And_ਅਗਲਾ_Null(nodep->modulesp()); }

    ਖਾਲੀ ਦੇਖੋ(AstCFunc* nodep) override {
        VL_RESTORER(m_funcp);
        VL_RESTORER(m_nTmps);
        m_funcp = nodep;
        m_nTmps = 0;
        ਅਚਲ VDouble0 statWidesBefore = m_statWides;
        iterateChildren(nodep);

        // Constant fold here if anything was expanded, as Ast size can likely be reduced
        ਜੇ (v3Global.opt.fConstEager() && m_statWides != statWidesBefore) {
            Ast_ਗੰਢ* ਅਚਲ editedp = V3Const::constifyEditCpp(nodep);
            UASSERT_OBJ(editedp == nodep, editedp, "Should not have replaced CFunc");
        }
    }

    ਖਾਲੀ ਦੇਖੋ(AstExtend* nodep) override {
        ਜੇ (nodep->user1_ਸੈੱਟ_Once()) ਵਾਪਸ;  // Process once
        iterateChildren(nodep);
        ਜੇ (nodep->isWide()) {
            // See under ASSIGN(EXTEND)
        } ਨਹੀਂਤਾਂ {
            ਜੇ (isImpure(nodep)) ਵਾਪਸ;
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ lhsp = nodep->lhsp()->unlink_Fr_ਪਿੱਠ();
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* newp = lhsp;
            ਜੇ (nodep->isQuad()) {
                ਜੇ (lhsp->isQuad()) {
                    lhsp->dtypeFrom(nodep);  // Just mark it, else nop
                } ਨਹੀਂਤਾਂ ਜੇ (lhsp->isWide()) {
                    nodep->v3fatalSrc("extending larger thing into smaller?");
                } ਨਹੀਂਤਾਂ {
                    newp = ਨਵਾਂ AstCCast{nodep->fileline(), lhsp, nodep};
                }
            } ਨਹੀਂਤਾਂ {  // Long
                UASSERT_OBJ(!(lhsp->isQuad() || lhsp->isWide()), nodep,
                            "extending larger thing into smaller?");
                lhsp->dtypeFrom(nodep);  // Just mark it, else nop
            }
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        }
    }
    ਤਾਰਕਿਕ expandWide(Ast_ਗੰਢ_ਸੌਂਪੋ* nodep, AstExtend* rhsp) {
        ਜੇ (!doExpandWide(nodep)) ਵਾਪਸ ਝੂਠ;
        did_ਨਮੂਨਾ(VExpand_ਨਮੂਨਾ::ਸੌਂਪੋ_EXTEND, nodep);
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ rlhsp = rhsp->lhsp();
        ਲਈ (ਪੂਰਨ w = 0; w < rlhsp->ਚੌੜਾਈ_ਸ਼ਬਦ(); ++w) {
            ਜੋੜੋ_ਸ਼ਬਦ_ਸੌਂਪੋ(nodep, w, new_Ast_ਸ਼ਬਦ_Sel_Clone(rlhsp, w));
        }
        ਲਈ (ਪੂਰਨ w = rlhsp->ਚੌੜਾਈ_ਸ਼ਬਦ(); w < nodep->ਚੌੜਾਈ_ਸ਼ਬਦ(); ++w) {
            ਜੋੜੋ_ਸ਼ਬਦ_ਸੌਂਪੋ(nodep, w, ਨਵਾਂ AstConst{nodep->fileline(), AstConst::SizedEData{}, 0});
        }
        ਵਾਪਸ ਸੱਚ;
    }

    ਖਾਲੀ ਦੇਖੋ(AstSel* nodep) override {
        ਜੇ (nodep->user1_ਸੈੱਟ_Once()) ਵਾਪਸ;  // Process once
        iterateChildren(nodep);

        ਅਚਲ uint32_t ਚੌੜਾਈ = static_cast<uint32_t>(nodep->ਚੌੜਾਈ_Const());
        UASSERT_OBJ(nodep->ਚੌੜਾਈ_Min() == static_cast<ਪੂਰਨ>(ਚੌੜਾਈ), nodep, "Width mismatch");

        // Skip if Sel is an LHS assignment select
        ਜੇ (Ast_ਗੰਢ_ਸੌਂਪੋ* ਅਚਲ assignp = VN_CAST(nodep->backp(), ਗੰਢ_ਸੌਂਪੋ)) {
            ਜੇ (nodep == assignp->lhsp()) ਵਾਪਸ;
        }

        // Skip if wide: See under ASSIGN(WIDE)
        ਜੇ (nodep->isWide()) ਵਾਪਸ;

        // Skip Sel from stream or queue
        ਜੇ (VN_IS(nodep->fromp()->dtypep(), ਧਾਰਾ_DType)) ਵਾਪਸ;
        ਜੇ (VN_IS(nodep->fromp()->dtypep(), ਕਤਾਰ_DType)) ਵਾਪਸ;

        // Result must be non-wide after skipping all the above
        UASSERT_OBJ(ਚੌੜਾਈ <= 64, nodep, "Inconsistent result width");

        ਜੇ (nodep->fromp()->isWide()) {  // Long/Quad from Wide
            ਜੇ (isImpure(nodep)) ਵਾਪਸ;
            did_ਨਮੂਨਾ(VExpand_ਨਮੂਨਾ::SEL_FROM_WIDE, nodep);
            File_ਸਤਰ* ਅਚਲ nfl = nodep->fileline();
            File_ਸਤਰ* ਅਚਲ lfl = nodep->lsbp()->fileline();
            File_ਸਤਰ* ਅਚਲ ffl = nodep->fromp()->fileline();
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ fromp = nodep->fromp();

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
            ਅਚਲ ਸਵੈਚਾਲਿਤ tryFold = [&](Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp) -> Ast_ਗੰਢ_ਵਿਅੰਜਕ* {
                // Size limit for inlining expressions (any arbitrary value results in
                // a bounded expansion. Picked a value to allow common simple forms.)
                ਸਥਿਰ constexpr ਪੂਰਨ EXPAND_SEL_LSB_ਹੱਦਬੰਦੀ = 8;
                // Simplify
                exprp = V3Const::constifyEditCpp(exprp);
                // Accept if not larger than the limit
                ਜੇ (!exprp->isLargerThan(EXPAND_SEL_LSB_ਹੱਦਬੰਦੀ)) ਵਾਪਸ exprp;
                // Delete if rejected
                VL_DO_DANGLING(exprp->delete_ਰੁੱਖ(), exprp);
                ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
            };

            // Return the index of the word holding the bit 'offset' bits above the LSB
            ਅਚਲ ਸਵੈਚਾਲਿਤ ਸ਼ਬਦ_Idx = [&](uint32_t ਵਿੱਥ) -> Ast_ਗੰਢ_ਵਿਅੰਜਕ* {
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ msbp = ਨਵਾਂ Ast_ਜੋੜੋ{lfl, ਨਵਾਂ AstConst{lfl, ਵਿੱਥ},
                                                     nodep->lsbp()->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ)};
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ idxp = new_ਸ਼ਬਦ_ਸੂਚਕਾਂਕ(msbp);
                ਜੇ (!msbp->backp()) VL_DO_DANGLING(msbp->delete_ਰੁੱਖ(), msbp);
                ਵਾਪਸ idxp;
            };

            // Bit index of the selected LSB within its word
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* lBitp = tryFold(new_Sel_ਦੋਆਧਾਰੀ_ਅੰਕ_ਦੋਆਧਾਰੀ_ਅੰਕ(nodep->lsbp()));
            ਅਚਲ ਤਾਰਕਿਕ aligned = lBitp && lBitp->isZero();

            // Which of the other 2 words the expansion might need
            ਅਚਲ ਤਾਰਕਿਕ mNeeded = ਚੌੜਾਈ > 1 && !aligned;
            ਅਚਲ ਤਾਰਕਿਕ hNeeded = ਚੌੜਾਈ > VL_EDATASIZE;
            UASSERT_OBJ(!hNeeded || nodep->isQuad(), nodep, "Width mismatch");

            // Word indices
            ਅਚਲ uint32_t m_Msb_ਵਿੱਥ = std::min<uint32_t>(ਚੌੜਾਈ, VL_EDATASIZE) - 1;
            ਅਚਲ uint32_t h_Msb_ਵਿੱਥ = ਚੌੜਾਈ - 1;
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* lIdxp = tryFold(new_ਸ਼ਬਦ_ਸੂਚਕਾਂਕ(nodep->lsbp()));
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* mIdxp = mNeeded ? tryFold(ਸ਼ਬਦ_Idx(m_Msb_ਵਿੱਥ)) : ਖਾਲੀ_ਸੂਚਕ;
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* hIdxp = hNeeded ? tryFold(ਸ਼ਬਦ_Idx(h_Msb_ਵਿੱਥ)) : ਖਾਲੀ_ਸੂਚਕ;

            // If the LSB expression is needed multiple times, evaluate it into a temporary
            ਅਚਲ ਪੂਰਨ nCopies = !lBitp + !lIdxp + (mNeeded && !mIdxp) + (hNeeded && !hIdxp);
            ਜੇ (nCopies > 1 && m_funcp && m_stmtp && !isImpure(m_stmtp)) {
                ++m_nTmps;  // Use fresh set of temporaries
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ lsbp = nodep->lsbp()->unlink_Fr_ਪਿੱਠ();
                AstVar* ਅਚਲ tmpp = ਜੋੜੋ_Local_Tmp(m_stmtp, "ExpandSel_Lsb", lsbp);
                nodep->lsbp(ਨਵਾਂ AstVarRef{lfl, tmpp, VAccess::READ});
            }

            // Rebuild remaining expressions derived from LSB if needed, now using the temporary
            ਜੇ (!lBitp) lBitp = new_Sel_ਦੋਆਧਾਰੀ_ਅੰਕ_ਦੋਆਧਾਰੀ_ਅੰਕ(nodep->lsbp());
            ਜੇ (!lIdxp) lIdxp = new_ਸ਼ਬਦ_ਸੂਚਕਾਂਕ(nodep->lsbp());
            ਜੇ (!mIdxp) mIdxp = mNeeded ? ਸ਼ਬਦ_Idx(m_Msb_ਵਿੱਥ) : ਖਾਲੀ_ਸੂਚਕ;
            ਜੇ (!hIdxp) hIdxp = hNeeded ? ਸ਼ਬਦ_Idx(h_Msb_ਵਿੱਥ) : ਖਾਲੀ_ਸੂਚਕ;

            // Return word 'idxp' of 'fromp', without consuming 'idxp'
            ਅਚਲ ਸਵੈਚਾਲਿਤ ਸ਼ਬਦ_Sel_2 = [&](Ast_ਗੰਢ_ਵਿਅੰਜਕ* idxp) -> Ast_ਗੰਢ_ਵਿਅੰਜਕ* {
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ clonep = idxp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ);
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ wordp = new_ਸ਼ਬਦ_Sel_ਸ਼ਬਦ(ffl, fromp, clonep);
                ਜੇ (!clonep->backp()) VL_DO_DANGLING(clonep->delete_ਰੁੱਖ(), clonep);
                ਵਾਪਸ wordp;
            };

            // Construct term containing the bits of the low word - always needed
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ lTermp = [&]() -> Ast_ਗੰਢ_ਵਿਅੰਜਕ* {
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* lWordp = ਸ਼ਬਦ_Sel_2(lIdxp);
                ਜੇ (nodep->isQuad()) lWordp = ਨਵਾਂ AstCCast{nfl, lWordp, nodep};
                ਵਾਪਸ ਨਵਾਂ Ast_ਖਿਸਕਾਓ_R{nfl, lWordp, lBitp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ), nodep->ਚੌੜਾਈ()};
            }();

            // Construct term containing the bits of the middle word - if needed
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ mTermp = [&]() -> Ast_ਗੰਢ_ਵਿਅੰਜਕ* {
                ਜੇ (!mNeeded) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;

                Ast_ਗੰਢ_ਵਿਅੰਜਕ* mWordp = ਸ਼ਬਦ_Sel_2(mIdxp);
                ਜੇ (nodep->isQuad()) mWordp = ਨਵਾਂ AstCCast{nfl, mWordp, nodep};
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ mShiftp = ਨਵਾਂ AstSub{lfl, ਨਵਾਂ AstConst{lfl, VL_EDATASIZE},
                                                        lBitp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ)};
                // If we're selecting bit zero, then all 32 bits in the mid word
                // get shifted << by 32 bits, so ignore them.
                ਅਚਲ V3_ਸੰਖਿਆ zero{nodep, long_Or_Quad_ਚੌੜਾਈ(nodep)};
                ਵਾਪਸ ਨਵਾਂ AstCond{
                    nfl,
                    // lsb % VL_EDATASIZE == 0 ?
                    ਨਵਾਂ AstEq{nfl, ਨਵਾਂ AstConst{nfl, 0}, lBitp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ)},
                    // 0 :
                    ਨਵਾਂ AstConst{nfl, zero},
                    //  midword >> (VL_EDATASIZE - (lbs % VL_EDATASIZE))
                    ਨਵਾਂ Ast_ਖਿਸਕਾਓ_L{nfl, mWordp, mShiftp, nodep->ਚੌੜਾਈ()}};
            }();

            // Construct term containing the bits of the high word - if needed
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ hTermp = [&]() -> Ast_ਗੰਢ_ਵਿਅੰਜਕ* {
                ਜੇ (!hNeeded) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;

                Ast_ਗੰਢ_ਵਿਅੰਜਕ* hWordp = ਸ਼ਬਦ_Sel_2(hIdxp);
                hWordp = ਨਵਾਂ AstCCast{nfl, hWordp, nodep};
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ hShiftp = ਨਵਾਂ AstCond{
                    nfl,
                    // lsb % VL_EDATASIZE == 0 ?
                    ਨਵਾਂ AstEq{nfl, ਨਵਾਂ AstConst{nfl, 0}, lBitp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ)},
                    // VL_EDATASIZE :
                    ਨਵਾਂ AstConst{lfl, VL_EDATASIZE},
                    // 64 - (lbs % VL_EDATASIZE)
                    ਨਵਾਂ AstSub{lfl, ਨਵਾਂ AstConst{lfl, 64}, lBitp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ)}};
                ਵਾਪਸ ਨਵਾਂ Ast_ਖਿਸਕਾਓ_L{nfl, hWordp, hShiftp, nodep->ਚੌੜਾਈ()};
            }();

            // Delete parts not captured during construction
            VL_DO_DANGLING(lBitp->delete_ਰੁੱਖ(), lBitp);
            VL_DO_DANGLING(lIdxp->delete_ਰੁੱਖ(), lIdxp);
            ਜੇ (mIdxp) VL_DO_DANGLING(mIdxp->delete_ਰੁੱਖ(), mIdxp);
            ਜੇ (hIdxp) VL_DO_DANGLING(hIdxp->delete_ਰੁੱਖ(), hIdxp);

            // Or reduce the terms
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* newp = lTermp;
            ਜੇ (mTermp) newp = ਨਵਾਂ AstOr{nfl, mTermp, newp};
            ਜੇ (hTermp) newp = ਨਵਾਂ AstOr{nfl, hTermp, newp};
            newp->dtypeFrom(nodep);
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        } ਨਹੀਂਤਾਂ {  // Long/Quad from Long/Quad
            // No isImpure() check - can handle side effects in below
            did_ਨਮੂਨਾ(VExpand_ਨਮੂਨਾ::SEL_FROM_NARROW, nodep);
            File_ਸਤਰ* ਅਚਲ fl = nodep->fileline();
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* fromp = nodep->fromp()->unlink_Fr_ਪਿੱਠ();
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ lsbp = nodep->lsbp()->unlink_Fr_ਪਿੱਠ();
            ਜੇ (nodep->isQuad() && !fromp->isQuad()) fromp = ਨਵਾਂ AstCCast{fl, fromp, nodep};
            // {large}>>32 requires 64-bit shift operation; then cast
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* newp = ਨਵਾਂ Ast_ਖਿਸਕਾਓ_R{fl, fromp, lsbp, fromp->ਚੌੜਾਈ()};
            newp->dtypeFrom(fromp);
            ਜੇ (!nodep->isQuad() && fromp->isQuad()) newp = ਨਵਾਂ AstCCast{fl, newp, nodep};
            newp->dtypeFrom(nodep);
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        }
    }

    ਤਾਰਕਿਕ expandWide(Ast_ਗੰਢ_ਸੌਂਪੋ* nodep, AstSel* rhsp) {
        UASSERT_OBJ(nodep->ਚੌੜਾਈ_Min() == rhsp->ਚੌੜਾਈ_Const(), nodep, "Width mismatch");
        ਜੇ (!doExpandWide(nodep)) ਵਾਪਸ ਝੂਠ;

        // Simplify the index, in case it becomes a constant
        V3Const::constifyEditCpp(rhsp->lsbp());

        // If it's a constant select and aligned, we can just copy the words
        ਜੇ (ਅਚਲ AstConst* ਅਚਲ lsbConstp = VN_CAST(rhsp->lsbp(), Const)) {
            ਅਚਲ uint32_t lsb = lsbConstp->ਮੰਜ਼ਿਲ_UInt();
            ਜੇ (VL_BITBIT_E(lsb) == 0) {
                did_ਨਮੂਨਾ(VExpand_ਨਮੂਨਾ::ਸੌਂਪੋ_SEL_ALIGNED, nodep);
                ਅਚਲ uint32_t ਸ਼ਬਦ = VL_BITWORD_E(lsb);
                ਲਈ (ਪੂਰਨ w = 0; w < nodep->ਚੌੜਾਈ_ਸ਼ਬਦ(); ++w) {
                    ਜੋੜੋ_ਸ਼ਬਦ_ਸੌਂਪੋ(nodep, w, new_Ast_ਸ਼ਬਦ_Sel_Clone(rhsp->fromp(), w + ਸ਼ਬਦ));
                }
                ਵਾਪਸ ਸੱਚ;
            }
        }

        did_ਨਮੂਨਾ(VExpand_ਨਮੂਨਾ::ਸੌਂਪੋ_SEL_UNALIGNED, nodep);
        File_ਸਤਰ* ਅਚਲ flp = rhsp->fileline();

        // Use fresh set of temporaries
        ++m_nTmps;

        // Compute word index of LSB, store to temporary if not constant
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ ਸ਼ਬਦ_Lsbp = rhsp->lsbp()->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ);
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਸ਼ਬਦ_Idxp = new_ਸ਼ਬਦ_ਸੂਚਕਾਂਕ(ਸ਼ਬਦ_Lsbp);
        ਜੇ (!ਸ਼ਬਦ_Lsbp->backp()) VL_DO_DANGLING(ਸ਼ਬਦ_Lsbp->delete_ਰੁੱਖ(), ਸ਼ਬਦ_Lsbp);
        ਸ਼ਬਦ_Idxp = V3Const::constifyEditCpp(ਸ਼ਬਦ_Idxp);
        ਜੇ (!VN_IS(ਸ਼ਬਦ_Idxp, Const)) {
            AstVar* ਅਚਲ tmpp = ਜੋੜੋ_Local_Tmp(nodep, "ExpandSel_WordIdx", ਸ਼ਬਦ_Idxp);
            ਸ਼ਬਦ_Idxp = ਨਵਾਂ AstVarRef{flp, tmpp, VAccess::READ};
        }

        // Compute shift amounts and mask, store to temporaries if not constants
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* loShftp = ਖਾਲੀ_ਸੂਚਕ;
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* hiShftp = ਖਾਲੀ_ਸੂਚਕ;
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* hiMaskp = ਖਾਲੀ_ਸੂਚਕ;
        ਜੇ (ਅਚਲ AstConst* ਅਚਲ lsbConstp = VN_CAST(rhsp->lsbp(), Const)) {
            ਅਚਲ uint32_t ਦੋਆਧਾਰੀ_ਅੰਕ_ਵਿੱਥ = VL_BITBIT_E(lsbConstp->ਮੰਜ਼ਿਲ_UInt());
            // Must be unaligned, otherwise we would have handled it above
            UASSERT_OBJ(ਦੋਆਧਾਰੀ_ਅੰਕ_ਵਿੱਥ, nodep, "Missed aligned wide select");
            loShftp = ਨਵਾਂ AstConst{flp, ਦੋਆਧਾਰੀ_ਅੰਕ_ਵਿੱਥ};
            hiShftp = ਨਵਾਂ AstConst{flp, VL_EDATASIZE - ਦੋਆਧਾਰੀ_ਅੰਕ_ਵਿੱਥ};
            hiMaskp = ਖਾਲੀ_ਸੂਚਕ;
        } ਨਹੀਂਤਾਂ {
            // Compute Low word shift amount: bottom bits of lsb index
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ lsbp = rhsp->lsbp()->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ);
            loShftp = ਨਵਾਂ AstAnd{flp, ਨਵਾਂ AstConst{flp, VL_SIZEBITS_E}, lsbp};
            AstVar* ਅਚਲ loTmpp = ਜੋੜੋ_Local_Tmp(nodep, "ExpandSel_LoShift", loShftp);
            loShftp = ਨਵਾਂ AstVarRef{flp, loTmpp, VAccess::READ};

            // Compute if aligned: Low word shift amount is 0
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ zerop = ਨਵਾਂ AstConst{flp, 0};
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* alignedp = ਨਵਾਂ AstEq{flp, zerop, loShftp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ)};
            AstVar* ਅਚਲ alignedTmpp = ਜੋੜੋ_Local_Tmp(nodep, "ExpandSel_Aligned", alignedp);
            alignedp = ਨਵਾਂ AstVarRef{flp, alignedTmpp, VAccess::READ};

            // Computed High word shift amount: 0 if aligned else VL_EDATASIZE - Low word shift
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ edsp = ਨਵਾਂ AstConst{flp, VL_EDATASIZE};
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ subp = ਨਵਾਂ AstSub{flp, edsp, loShftp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ)};
            hiShftp = ਨਵਾਂ AstCond{flp, alignedp, ਨਵਾਂ AstConst{flp, 0}, subp};
            AstVar* ਅਚਲ hiTmpp = ਜੋੜੋ_Local_Tmp(nodep, "ExpandSel_HiShift", hiShftp);
            hiShftp = ਨਵਾਂ AstVarRef{flp, hiTmpp, VAccess::READ};

            // Compute the High word mask: 0 if aligned, ones otherwise
            hiMaskp = ਨਵਾਂ AstCond{flp, alignedp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ),
                                  ਨਵਾਂ AstConst{flp, AstConst::SizedEData{}, 0},
                                  ਨਵਾਂ AstConst{flp, AstConst::SizedEData{}, -1ULL}};
            AstVar* ਅਚਲ ਨਕਾਬ_Tmpp = ਜੋੜੋ_Local_Tmp(nodep, "ExpandSel_HiMask", hiMaskp);
            hiMaskp = ਨਵਾਂ AstVarRef{flp, ਨਕਾਬ_Tmpp, VAccess::READ};
        }

        // Create each word of the selected result
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ fromp = rhsp->fromp();
        ਅਚਲ ਪੂਰਨ sel_ਸ਼ਬਦ = nodep->ਚੌੜਾਈ_ਸ਼ਬਦ();
        ਲਈ (ਪੂਰਨ w = 0; w < sel_ਸ਼ਬਦ; ++w) {
            // Grab bits from word 'VL_BITWORD_E(lsb) + w'
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ loWordp = new_ਸ਼ਬਦ_Sel_ਸ਼ਬਦ(fromp->fileline(), fromp, ਸ਼ਬਦ_Idxp, w);
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ loShftClonep = loShftp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ);
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ lop = ਨਵਾਂ Ast_ਖਿਸਕਾਓ_R{flp, loWordp, loShftClonep, VL_EDATASIZE};
            // Grab bits from word 'VL_BITWORD_E(lsb) + w + 1'
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* hiWordp = new_ਸ਼ਬਦ_Sel_ਸ਼ਬਦ(fromp->fileline(), fromp, ਸ਼ਬਦ_Idxp, w + 1);
            // For the last word of the result, avoid an OOB access when the Sel is not OOB
            ਜੇ (w == sel_ਸ਼ਬਦ - 1) {
                ਅਚਲ uint32_t max = fromp->ਚੌੜਾਈ_ਸ਼ਬਦ() - w - 1;
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ maxp = ਨਵਾਂ AstConst{flp, max};
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ condp = ਨਵਾਂ AstGte{flp, ਸ਼ਬਦ_Idxp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ), maxp};
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ zerop = ਨਵਾਂ AstConst{flp, AstConst::SizedEData{}, 0};
                hiWordp = ਨਵਾਂ AstCond{flp, condp, zerop, hiWordp};
            }
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ hiShftClonep = hiShftp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ);
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ hiPartp = ਨਵਾਂ Ast_ਖਿਸਕਾਓ_L{flp, hiWordp, hiShftClonep, VL_EDATASIZE};
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ hip
                = hiMaskp ? ਨਵਾਂ AstAnd{flp, hiPartp, hiMaskp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ)} : hiPartp;
            // Combine them
            ਜੋੜੋ_ਸ਼ਬਦ_ਸੌਂਪੋ(nodep, w, ਨਵਾਂ AstOr{nodep->fileline(), hip, lop});
        }

        // Delete parts not captured during construction
        ਜੇ (!ਸ਼ਬਦ_Idxp->backp()) VL_DO_DANGLING(ਸ਼ਬਦ_Idxp->delete_ਰੁੱਖ(), ਸ਼ਬਦ_Idxp);
        ਜੇ (!loShftp->backp()) VL_DO_DANGLING(loShftp->delete_ਰੁੱਖ(), loShftp);
        ਜੇ (!hiShftp->backp()) VL_DO_DANGLING(hiShftp->delete_ਰੁੱਖ(), hiShftp);
        ਜੇ (hiMaskp && !hiMaskp->backp()) VL_DO_DANGLING(hiMaskp->delete_ਰੁੱਖ(), hiMaskp);

        ਵਾਪਸ ਸੱਚ;
    }

    ਤਾਰਕਿਕ expandLhs(Ast_ਗੰਢ_ਸੌਂਪੋ* nodep, AstSel* lhsp) {
        // Possibilities
        //      destp: wide or narrow
        //      rhsp:  wide (destp must be wide), narrow, or 1 bit wide
        //      rhsp:  may be allones and can remove AND NOT gate
        //      lsbp:  constant or variable
        // Yuk.
        ਜੇ (isImpure(nodep)) ਵਾਪਸ ਝੂਠ;
        File_ਸਤਰ* ਅਚਲ nfl = nodep->fileline();
        File_ਸਤਰ* ਅਚਲ lfl = lhsp->fileline();
        ਅਚਲ ਤਾਰਕਿਕ destwide = lhsp->fromp()->isWide();
        ਅਚਲ ਤਾਰਕਿਕ ones = nodep->rhsp()->isAllOnesV();
        ਜੇ (VN_IS(lhsp->lsbp(), Const)) {
            // The code should work without this constant test, but it won't
            // constify as nicely as we'd like.
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp = nodep->rhsp()->unlink_Fr_ਪਿੱਠ();
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ destp = lhsp->fromp()->unlink_Fr_ਪਿੱਠ();
            ਅਚਲ ਪੂਰਨ lsb = lhsp->lsbConst();
            ਅਚਲ ਪੂਰਨ msb = lhsp->msbConst();
            V3_ਸੰਖਿਆ maskset{nodep, destp->ਚੌੜਾਈ_Min()};
            maskset.ਸੈੱਟ_ਨਕਾਬ(msb + 1 - lsb, lsb);
            V3_ਸੰਖਿਆ maskold{nodep, destp->ਚੌੜਾਈ_Min()};
            maskold.opNot(maskset);
            ਜੇ (destwide) {
                did_ਨਮੂਨਾ(VExpand_ਨਮੂਨਾ::ਸੌਂਪੋ_ਮੰਜ਼ਿਲ_SEL_CONST_WIDE, nodep);
                ਲਈ (ਪੂਰਨ w = 0; w < destp->ਚੌੜਾਈ_ਸ਼ਬਦ(); ++w) {
                    ਜੇ (w >= VL_BITWORD_E(lsb) && w <= VL_BITWORD_E(msb)) {
                        // else we would just be setting it to the same exact value
                        Ast_ਗੰਢ_ਵਿਅੰਜਕ* oldvalp = new_Ast_ਸ਼ਬਦ_Sel_Clone(destp, w);
                        fixCloneLvalue(oldvalp);
                        ਜੇ (!ones) {
                            oldvalp = ਨਵਾਂ AstAnd{
                                lfl,
                                ਨਵਾਂ AstConst{lfl, AstConst::SizedEData{}, maskold.edata_ਸ਼ਬਦ(w)},
                                oldvalp};
                        }

                        // Appropriate word of new value to insert:
                        Ast_ਗੰਢ_ਵਿਅੰਜਕ* newp = new_ਸ਼ਬਦ_Grab_ਖਿਸਕਾਓ(lfl, w, rhsp, lsb);

                        // Apply cleaning at the top word of the destination
                        // (no cleaning to do if dst's width is a whole number
                        // of words).
                        ਜੇ (w == destp->ਚੌੜਾਈ_ਸ਼ਬਦ() - 1 && VL_BITBIT_E(destp->ਚੌੜਾਈ_Min()) != 0) {
                            V3_ਸੰਖਿਆ cleanmask{nodep, VL_EDATASIZE};
                            cleanmask.ਸੈੱਟ_ਨਕਾਬ(VL_BITBIT_E(destp->ਚੌੜਾਈ_Min()));
                            newp = ਨਵਾਂ AstAnd{lfl, newp, ਨਵਾਂ AstConst{lfl, cleanmask}};
                        }
                        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ orp
                            = V3Const::constifyEditCpp(ਨਵਾਂ AstOr{lfl, oldvalp, newp});
                        ਜੋੜੋ_ਸ਼ਬਦ_ਸੌਂਪੋ(nodep, w, destp, orp);
                    }
                }
                VL_DO_DANGLING(rhsp->delete_ਰੁੱਖ(), rhsp);
                VL_DO_DANGLING(destp->delete_ਰੁੱਖ(), destp);
            } ਨਹੀਂਤਾਂ {
                did_ਨਮੂਨਾ(VExpand_ਨਮੂਨਾ::ਸੌਂਪੋ_ਮੰਜ਼ਿਲ_SEL_CONST_NARROW, nodep);
                ਜੇ (destp->isQuad() && !rhsp->isQuad()) rhsp = ਨਵਾਂ AstCCast{nfl, rhsp, nodep};
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* oldvalp = destp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਸੱਚ);
                fixCloneLvalue(oldvalp);
                ਜੇ (!ones) oldvalp = ਨਵਾਂ AstAnd{lfl, ਨਵਾਂ AstConst{lfl, maskold}, oldvalp};

                // The bit-select can refer to bits outside the width of nodep
                // which we aren't allowed to assign to.  This is a mask of the
                // valid range of nodep which we apply to the new shifted RHS.
                V3_ਸੰਖਿਆ cleanmask{nodep, destp->ਚੌੜਾਈ_Min()};
                cleanmask.ਸੈੱਟ_ਨਕਾਬ(destp->ਚੌੜਾਈ_Min());
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ shifted = ਨਵਾਂ Ast_ਖਿਸਕਾਓ_L{
                    lfl, rhsp, ਨਵਾਂ AstConst{lfl, static_cast<uint32_t>(lsb)}, destp->ਚੌੜਾਈ()};
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ cleaned
                    = ਨਵਾਂ AstAnd{lfl, shifted, ਨਵਾਂ AstConst{lfl, cleanmask}};
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ orp
                    = V3Const::constifyEditCpp(ਨਵਾਂ AstOr{lfl, oldvalp, cleaned});
                ਪਾਓ_Before(nodep, ਨਵਾਂ Ast_ਸੌਂਪੋ{nfl, destp, orp});
            }
            ਵਾਪਸ ਸੱਚ;
        } ਨਹੀਂਤਾਂ {  // non-const select offset
            ਜੇ (destwide && lhsp->ਚੌੜਾਈ_Const() == 1) {
                did_ਨਮੂਨਾ(VExpand_ਨਮੂਨਾ::ਸੌਂਪੋ_ਮੰਜ਼ਿਲ_SEL_VAR_WIDE_ONE_ਦੋਆਧਾਰੀ_ਅੰਕ, nodep);
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ rhsp = nodep->rhsp()->unlink_Fr_ਪਿੱਠ();
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ destp = lhsp->fromp()->unlink_Fr_ਪਿੱਠ();
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* oldvalp
                    = new_ਸ਼ਬਦ_Sel_ਦੋਆਧਾਰੀ_ਅੰਕ(lfl, destp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ), lhsp->lsbp());
                fixCloneLvalue(oldvalp);
                ਜੇ (!ones) {
                    oldvalp = ਨਵਾਂ AstAnd{
                        lfl,
                        ਨਵਾਂ AstNot{
                            lfl, ਨਵਾਂ Ast_ਖਿਸਕਾਓ_L{lfl, ਨਵਾਂ AstConst{nfl, 1},
                                               // newSelBitBit may exceed the MSB of this variable.
                                               // That's ok as we'd just AND with a larger value,
                                               // but oldval would clip the upper bits to sanity
                                               new_Sel_ਦੋਆਧਾਰੀ_ਅੰਕ_ਦੋਆਧਾਰੀ_ਅੰਕ(lhsp->lsbp()), VL_EDATASIZE}},
                        oldvalp};
                }
                // Restrict the shift amount to 0-31, see bug804.
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ shiftp = ਨਵਾਂ AstAnd{nfl, lhsp->lsbp()->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਸੱਚ),
                                                       ਨਵਾਂ AstConst{nfl, VL_EDATASIZE - 1}};
                Ast_ਗੰਢ* ਅਚਲ newp = ਨਵਾਂ Ast_ਸੌਂਪੋ{
                    nfl, new_ਸ਼ਬਦ_Sel_ਦੋਆਧਾਰੀ_ਅੰਕ(nfl, destp, lhsp->lsbp()),
                    ਨਵਾਂ AstOr{lfl, oldvalp, ਨਵਾਂ Ast_ਖਿਸਕਾਓ_L{lfl, rhsp, shiftp, VL_EDATASIZE}}};
                ਪਾਓ_Before(nodep, newp);
                ਵਾਪਸ ਸੱਚ;
            } ਨਹੀਂਤਾਂ ਜੇ (destwide) {
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
                ਵਾਪਸ ਝੂਠ;
            } ਨਹੀਂਤਾਂ {
                did_ਨਮੂਨਾ(VExpand_ਨਮੂਨਾ::ਸੌਂਪੋ_ਮੰਜ਼ਿਲ_SEL_VAR_NARROW, nodep);
                // nodep->dumpTree("-  old: ");
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp = nodep->rhsp()->unlink_Fr_ਪਿੱਠ();
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ destp = lhsp->fromp()->unlink_Fr_ਪਿੱਠ();
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* oldvalp = destp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਸੱਚ);
                fixCloneLvalue(oldvalp);

                V3_ਸੰਖਿਆ maskwidth{nodep, destp->ਚੌੜਾਈ_Min()};
                maskwidth.ਸੈੱਟ_ਨਕਾਬ(lhsp->ਚੌੜਾਈ_Const());

                ਜੇ (destp->isQuad() && !rhsp->isQuad()) rhsp = ਨਵਾਂ AstCCast{nfl, rhsp, nodep};
                ਜੇ (!ones) {
                    oldvalp = ਨਵਾਂ AstAnd{
                        lfl,
                        ਨਵਾਂ AstNot{lfl, ਨਵਾਂ Ast_ਖਿਸਕਾਓ_L{lfl, ਨਵਾਂ AstConst{nfl, maskwidth},
                                                      lhsp->lsbp()->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਸੱਚ),
                                                      destp->ਚੌੜਾਈ()}},
                        oldvalp};
                }
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* newp
                    = ਨਵਾਂ Ast_ਖਿਸਕਾਓ_L{lfl, rhsp, lhsp->lsbp()->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਸੱਚ), destp->ਚੌੜਾਈ()};
                // Apply cleaning to the new value being inserted.  Mask is
                // slightly wider than necessary to avoid an AND with all ones
                // being optimized out.  No need to clean if destp is
                // quad-sized as there are no extra bits to contaminate
                ਜੇ (destp->ਚੌੜਾਈ_Min() != 64) {
                    V3_ਸੰਖਿਆ cleanmask{nodep, destp->ਚੌੜਾਈ_Min() + 1};
                    cleanmask.ਸੈੱਟ_ਨਕਾਬ(destp->ਚੌੜਾਈ_Min());
                    newp = ਨਵਾਂ AstAnd{lfl, newp, ਨਵਾਂ AstConst{lfl, cleanmask}};
                }

                ਪਾਓ_Before(nodep, ਨਵਾਂ Ast_ਸੌਂਪੋ{nfl, destp, ਨਵਾਂ AstOr{lfl, oldvalp, newp}});
                ਵਾਪਸ ਸੱਚ;
            }
        }
    }

    ਖਾਲੀ ਦੇਖੋ(AstConcat* nodep) override {
        ਜੇ (nodep->user1_ਸੈੱਟ_Once()) ਵਾਪਸ;  // Process once
        iterateChildren(nodep);
        ਜੇ (nodep->isWide()) {
            // See under ASSIGN(WIDE)
        } ਨਹੀਂਤਾਂ {
            // No isImpure() check - can handle side effects in below
            did_ਨਮੂਨਾ(VExpand_ਨਮੂਨਾ::CONCAT, nodep);
            File_ਸਤਰ* ਅਚਲ fl = nodep->fileline();
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp = nodep->lhsp()->unlink_Fr_ਪਿੱਠ();
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp = nodep->rhsp()->unlink_Fr_ਪਿੱਠ();
            ਅਚਲ uint32_t rhsshift = rhsp->ਚੌੜਾਈ_Min();
            ਜੇ (nodep->isQuad() && !lhsp->isQuad()) lhsp = ਨਵਾਂ AstCCast{fl, lhsp, nodep};
            ਜੇ (nodep->isQuad() && !rhsp->isQuad()) rhsp = ਨਵਾਂ AstCCast{fl, rhsp, nodep};
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ newp = ਨਵਾਂ AstOr{
                fl, ਨਵਾਂ Ast_ਖਿਸਕਾਓ_L{fl, lhsp, ਨਵਾਂ AstConst{fl, rhsshift}, nodep->ਚੌੜਾਈ()}, rhsp};
            newp->dtypeFrom(nodep);  // Unsigned
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        }
    }
    ਤਾਰਕਿਕ expandWide(Ast_ਗੰਢ_ਸੌਂਪੋ* nodep, AstConcat* rhsp) {
        ਜੇ (!doExpandWide(rhsp)) ਵਾਪਸ ਝੂਠ;
        did_ਨਮੂਨਾ(VExpand_ਨਮੂਨਾ::ਸੌਂਪੋ_CONCAT, nodep);
        File_ਸਤਰ* ਅਚਲ fl = rhsp->fileline();
        // Lhs or Rhs may be word, long, or quad.
        // newAstWordSelClone nicely abstracts the difference.
        ਅਚਲ ਪੂਰਨ rhsshift = rhsp->rhsp()->ਚੌੜਾਈ_Min();
        // Sometimes doing the words backwards is preferable.
        // When we have x={x,foo} backwards is better, when x={foo,x} forward is better
        // However V3Subst tends to rip this up, so not worth optimizing now.
        ਲਈ (ਪੂਰਨ w = 0; w < rhsp->ਚੌੜਾਈ_ਸ਼ਬਦ(); ++w) {
            ਜੋੜੋ_ਸ਼ਬਦ_ਸੌਂਪੋ(nodep, w,
                          ਨਵਾਂ AstOr{fl, new_ਸ਼ਬਦ_Grab_ਖਿਸਕਾਓ(fl, w, rhsp->lhsp(), rhsshift),
                                    new_Ast_ਸ਼ਬਦ_Sel_Clone(rhsp->rhsp(), w)});
        }
        ਵਾਪਸ ਸੱਚ;
    }

    ਖਾਲੀ ਦੇਖੋ(AstReplicate* nodep) override {
        ਜੇ (nodep->user1_ਸੈੱਟ_Once()) ਵਾਪਸ;  // Process once
        iterateChildren(nodep);
        ਜੇ (nodep->isWide()) {
            // See under ASSIGN(WIDE)
        } ਨਹੀਂਤਾਂ {
            ਜੇ (isImpure(nodep)) ਵਾਪਸ;
            File_ਸਤਰ* ਅਚਲ fl = nodep->fileline();
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp = nodep->srcp()->unlink_Fr_ਪਿੱਠ();
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* newp;
            ਅਚਲ ਪੂਰਨ lhswidth = lhsp->ਚੌੜਾਈ_Min();
            ਜੇ (lhswidth == 1) {
                did_ਨਮੂਨਾ(VExpand_ਨਮੂਨਾ::REPLICATE_ONE_ਦੋਆਧਾਰੀ_ਅੰਕ, nodep);
                newp = ਨਵਾਂ AstNegate{fl, lhsp};
            } ਨਹੀਂਤਾਂ {
                did_ਨਮੂਨਾ(VExpand_ਨਮੂਨਾ::REPLICATE, nodep);
                ਅਚਲ AstConst* ਅਚਲ constp = VN_AS(nodep->countp(), Const);
                UASSERT_OBJ(constp, nodep,
                            "Replication value isn't a constant.  Checked earlier!");
                ਅਚਲ uint32_t times = constp->ਮੰਜ਼ਿਲ_UInt();
                ਜੇ (nodep->isQuad() && !lhsp->isQuad()) lhsp = ਨਵਾਂ AstCCast{fl, lhsp, nodep};
                newp = lhsp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਸੱਚ);
                ਲਈ (ਬਿਨਾਨਿਸ਼ਾਨੀ repnum = 1; repnum < times; repnum++) {
                    ਅਚਲ ਪੂਰਨ rhsshift = repnum * lhswidth;
                    newp = ਨਵਾਂ AstOr{
                        fl,
                        ਨਵਾਂ Ast_ਖਿਸਕਾਓ_L{fl, lhsp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਸੱਚ),
                                      ਨਵਾਂ AstConst{fl, static_cast<uint32_t>(rhsshift)},
                                      nodep->ਚੌੜਾਈ()},
                        newp};
                    newp->dtypeFrom(nodep);  // Unsigned
                }
                VL_DO_DANGLING(lhsp->delete_ਰੁੱਖ(), lhsp);  // Never used
            }
            newp->dtypeFrom(nodep);  // Unsigned
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        }
    }
    ਤਾਰਕਿਕ expandWide(Ast_ਗੰਢ_ਸੌਂਪੋ* nodep, AstReplicate* rhsp) {
        ਜੇ (!doExpandWide(rhsp)) ਵਾਪਸ ਝੂਠ;
        did_ਨਮੂਨਾ(VExpand_ਨਮੂਨਾ::ਸੌਂਪੋ_REPLICATE, nodep);
        File_ਸਤਰ* ਅਚਲ fl = nodep->fileline();
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ lhsp = rhsp->srcp();
        ਅਚਲ ਪੂਰਨ lhswidth = lhsp->ਚੌੜਾਈ_Min();
        ਅਚਲ AstConst* ਅਚਲ constp = VN_AS(rhsp->countp(), Const);
        UASSERT_OBJ(constp, rhsp, "Replication value isn't a constant.  Checked earlier!");
        ਅਚਲ uint32_t times = constp->ਮੰਜ਼ਿਲ_UInt();
        ਲਈ (ਪੂਰਨ w = 0; w < rhsp->ਚੌੜਾਈ_ਸ਼ਬਦ(); ++w) {
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* newp;
            ਜੇ (lhswidth == 1) {
                newp = ਨਵਾਂ AstNegate{fl, lhsp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਸੱਚ)};
                // Replicate always unsigned
                newp->dtype_Chg_ਚੌੜਾਈ_Signed(VL_EDATASIZE, VL_EDATASIZE, VSigning::UNSIGNED);
            } ਨਹੀਂਤਾਂ {
                newp = new_Ast_ਸ਼ਬਦ_Sel_Clone(lhsp, w);
                File_ਸਤਰ* ਅਚਲ rfl = rhsp->fileline();
                ਲਈ (ਬਿਨਾਨਿਸ਼ਾਨੀ repnum = 1; repnum < times; repnum++) {
                    newp = ਨਵਾਂ AstOr{fl, new_ਸ਼ਬਦ_Grab_ਖਿਸਕਾਓ(rfl, w, lhsp, lhswidth * repnum), newp};
                }
            }
            ਜੋੜੋ_ਸ਼ਬਦ_ਸੌਂਪੋ(nodep, w, newp);
        }
        ਵਾਪਸ ਸੱਚ;
    }

    ਖਾਲੀ ਦੇਖੋ_Eq_Neq(Ast_ਗੰਢ_Biop* nodep) {
        ਜੇ (nodep->user1_ਸੈੱਟ_Once()) ਵਾਪਸ;  // Process once
        iterateChildren(nodep);
        ਜੇ (nodep->lhsp()->isWide()
            && !(VN_IS(nodep->lhsp()->dtypep()->skipRefp(), ਧਾਰਾ_DType)
                 || VN_IS(nodep->rhsp()->dtypep()->skipRefp(), ਧਾਰਾ_DType))) {
            ਜੇ (isImpure(nodep)) ਵਾਪਸ;
            ਜੇ (!doExpandWide(nodep->lhsp())) ਵਾਪਸ;
            ਜੇ (!doExpandWide(nodep->rhsp())) ਵਾਪਸ;
            did_ਨਮੂਨਾ(VN_IS(nodep, Neq) ? VExpand_ਨਮੂਨਾ::WORDIZE_NEQ
                                         : VExpand_ਨਮੂਨਾ::WORDIZE_EQ,
                       nodep);
            // -> (0=={or{for each_word{WORDSEL(lhs,#)^WORDSEL(rhs,#)}}}
            File_ਸਤਰ* ਅਚਲ fl = nodep->fileline();
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* newp = ਖਾਲੀ_ਸੂਚਕ;
            ਲਈ (ਪੂਰਨ w = 0; w < nodep->lhsp()->ਚੌੜਾਈ_ਸ਼ਬਦ(); ++w) {
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ eqp = ਨਵਾਂ AstXor{fl, new_Ast_ਸ਼ਬਦ_Sel_Clone(nodep->lhsp(), w),
                                                    new_Ast_ਸ਼ਬਦ_Sel_Clone(nodep->rhsp(), w)};
                newp = newp ? ਨਵਾਂ AstOr{fl, newp, eqp} : eqp;
            }
            ਜੇ (VN_IS(nodep, Neq)) {
                newp = ਨਵਾਂ AstNeq{fl, ਨਵਾਂ AstConst{fl, AstConst::SizedEData{}, 0}, newp};
            } ਨਹੀਂਤਾਂ {
                newp = ਨਵਾਂ AstEq{fl, ਨਵਾਂ AstConst{fl, AstConst::SizedEData{}, 0}, newp};
            }
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstEq* nodep) override { ਦੇਖੋ_Eq_Neq(nodep); }
    ਖਾਲੀ ਦੇਖੋ(AstNeq* nodep) override { ਦੇਖੋ_Eq_Neq(nodep); }

    ਖਾਲੀ ਦੇਖੋ(AstRedOr* nodep) override {
        ਜੇ (nodep->user1_ਸੈੱਟ_Once()) ਵਾਪਸ;  // Process once
        iterateChildren(nodep);
        File_ਸਤਰ* ਅਚਲ fl = nodep->fileline();
        ਜੇ (nodep->lhsp()->isWide()) {
            ਜੇ (isImpure(nodep)) ਵਾਪਸ;
            ਜੇ (!doExpandWide(nodep->lhsp())) ਵਾਪਸ;
            did_ਨਮੂਨਾ(VExpand_ਨਮੂਨਾ::WORDIZE_REDOR, nodep);
            // -> (0!={or{for each_word{WORDSEL(lhs,#)}}}
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* newp = ਖਾਲੀ_ਸੂਚਕ;
            ਲਈ (ਪੂਰਨ w = 0; w < nodep->lhsp()->ਚੌੜਾਈ_ਸ਼ਬਦ(); ++w) {
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ eqp = new_Ast_ਸ਼ਬਦ_Sel_Clone(nodep->lhsp(), w);
                newp = newp ? ਨਵਾਂ AstOr{fl, newp, eqp} : eqp;
            }
            newp = ਨਵਾਂ AstNeq{fl, ਨਵਾਂ AstConst{fl, AstConst::SizedEData{}, 0}, newp};
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        } ਨਹੀਂਤਾਂ {
            // No isImpure() check - can handle side effects in below
            did_ਨਮੂਨਾ(VExpand_ਨਮੂਨਾ::REDOR_ਮੰਜ਼ਿਲ_NEQ, nodep);
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ lhsp = nodep->lhsp()->unlink_Fr_ਪਿੱਠ();
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ newp = ਨਵਾਂ AstNeq{
                fl, ਨਵਾਂ AstConst{fl, AstConst::Widthed_ਮੁੱਲ{}, long_Or_Quad_ਚੌੜਾਈ(nodep), 0}, lhsp};
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstRedAnd* nodep) override {
        ਜੇ (nodep->user1_ਸੈੱਟ_Once()) ਵਾਪਸ;  // Process once
        iterateChildren(nodep);
        File_ਸਤਰ* ਅਚਲ fl = nodep->fileline();
        ਜੇ (nodep->lhsp()->isWide()) {
            ਜੇ (isImpure(nodep)) ਵਾਪਸ;
            ਜੇ (!doExpandWide(nodep->lhsp())) ਵਾਪਸ;
            did_ਨਮੂਨਾ(VExpand_ਨਮੂਨਾ::WORDIZE_REDAND, nodep);
            // -> (0!={and{for each_word{WORDSEL(lhs,#)}}}
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* newp = ਖਾਲੀ_ਸੂਚਕ;
            ਲਈ (ਪੂਰਨ w = 0; w < nodep->lhsp()->ਚੌੜਾਈ_ਸ਼ਬਦ(); ++w) {
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* eqp = new_Ast_ਸ਼ਬਦ_Sel_Clone(nodep->lhsp(), w);
                ਜੇ (w == nodep->lhsp()->ਚੌੜਾਈ_ਸ਼ਬਦ() - 1) {
                    // Rather than doing a (slowish) ==##, we OR in the
                    // bits that aren't part of the mask
                    eqp = ਨਵਾਂ AstOr{fl, ਨਵਾਂ AstConst{fl, not_Wide_ਨਕਾਬ(nodep->lhsp())},
                                    // Bug in cppcheck
                                    // cppcheck-suppress memleak
                                    eqp};
                }
                newp = newp ? ਨਵਾਂ AstAnd{fl, newp, eqp} : eqp;
            }
            newp = ਨਵਾਂ AstEq{fl, ਨਵਾਂ AstConst{fl, AstConst::SizedEData{}, VL_ਨਕਾਬ_E(VL_EDATASIZE)},
                             newp};
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        } ਨਹੀਂਤਾਂ {
            // No isImpure() check - can handle side effects in below
            did_ਨਮੂਨਾ(VExpand_ਨਮੂਨਾ::REDAND_ਮੰਜ਼ਿਲ_EQ, nodep);
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ lhsp = nodep->lhsp()->unlink_Fr_ਪਿੱਠ();
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ newp = ਨਵਾਂ AstEq{fl, ਨਵਾਂ AstConst{fl, ਸ਼ਬਦ_ਨਕਾਬ(lhsp)}, lhsp};
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstRedXor* nodep) override {
        ਜੇ (nodep->user1_ਸੈੱਟ_Once()) ਵਾਪਸ;  // Process once
        iterateChildren(nodep);
        ਜੇ (nodep->lhsp()->isWide()) {
            ਜੇ (isImpure(nodep)) ਵਾਪਸ;
            ਜੇ (!doExpandWide(nodep->lhsp())) ਵਾਪਸ;
            did_ਨਮੂਨਾ(VExpand_ਨਮੂਨਾ::WORDIZE_REDXOR, nodep);
            // -> (0!={redxor{for each_word{XOR(WORDSEL(lhs,#))}}}
            File_ਸਤਰ* ਅਚਲ fl = nodep->fileline();
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* newp = ਖਾਲੀ_ਸੂਚਕ;
            ਲਈ (ਪੂਰਨ w = 0; w < nodep->lhsp()->ਚੌੜਾਈ_ਸ਼ਬਦ(); ++w) {
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ eqp = new_Ast_ਸ਼ਬਦ_Sel_Clone(nodep->lhsp(), w);
                newp = newp ? ਨਵਾਂ AstXor{fl, newp, eqp} : eqp;
            }
            newp = ਨਵਾਂ AstRedXor{fl, newp};
            UINFO(8, "    Wordize REDXORnew " << newp);
            VL_DO_DANGLING(replaceWithDelete(nodep, newp), nodep);
        }
        // We don't reduce non-wide XORs, as its more efficient to use a temp register,
        // which the inlined function does nicely.
    }

    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Stmt* nodep) override {
        ਜੇ (nodep->user1_ਸੈੱਟ_Once()) ਵਾਪਸ;  // Process once
        VL_RESTORER(m_stmtp);
        m_stmtp = nodep;
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_ਸੌਂਪੋ* nodep) override {
        ਜੇ (nodep->user1_ਸੈੱਟ_Once()) ਵਾਪਸ;  // Process once
        ਜੇ (VN_IS(nodep->dtypep()->skipRefp(), UnpackArrayDType)) {
            ਵਾਪਸ;  // Skip for UnpackArrayDType
        }

        VL_RESTORER(m_stmtp);
        m_stmtp = nodep;
        iterateChildren(nodep);
        ਤਾਰਕਿਕ did = ਝੂਠ;
        ਜੇ (nodep->isWide()  //
            && ((VN_IS(nodep->lhsp(), VarRef) || VN_IS(nodep->lhsp(), ArraySel)))
            && !AstVar::scVarRecurse(nodep->lhsp())  // Need special function for SC
            && !AstVar::scVarRecurse(nodep->rhsp())) {
            ਜੇ (AstConst* ਅਚਲ rhsp = VN_CAST(nodep->rhsp(), Const)) {
                did = expandWide(nodep, rhsp);
            } ਨਹੀਂਤਾਂ ਜੇ (AstVarRef* ਅਚਲ rhsp = VN_CAST(nodep->rhsp(), VarRef)) {
                did = expandWide(nodep, rhsp);
            } ਨਹੀਂਤਾਂ ਜੇ (AstSel* ਅਚਲ rhsp = VN_CAST(nodep->rhsp(), Sel)) {
                did = expandWide(nodep, rhsp);
            } ਨਹੀਂਤਾਂ ਜੇ (AstArraySel* ਅਚਲ rhsp = VN_CAST(nodep->rhsp(), ArraySel)) {
                did = expandWide(nodep, rhsp);
            } ਨਹੀਂਤਾਂ ਜੇ (AstConcat* ਅਚਲ rhsp = VN_CAST(nodep->rhsp(), Concat)) {
                did = expandWide(nodep, rhsp);
            } ਨਹੀਂਤਾਂ ਜੇ (AstExtend* ਅਚਲ rhsp = VN_CAST(nodep->rhsp(), Extend)) {
                did = expandWide(nodep, rhsp);
            } ਨਹੀਂਤਾਂ ਜੇ (AstReplicate* ਅਚਲ rhsp = VN_CAST(nodep->rhsp(), Replicate)) {
                did = expandWide(nodep, rhsp);
            } ਨਹੀਂਤਾਂ ਜੇ (AstAnd* ਅਚਲ rhsp = VN_CAST(nodep->rhsp(), And)) {
                did = expandWide(nodep, rhsp);
            } ਨਹੀਂਤਾਂ ਜੇ (AstOr* ਅਚਲ rhsp = VN_CAST(nodep->rhsp(), Or)) {
                did = expandWide(nodep, rhsp);
            } ਨਹੀਂਤਾਂ ਜੇ (AstNot* ਅਚਲ rhsp = VN_CAST(nodep->rhsp(), Not)) {
                did = expandWide(nodep, rhsp);
            } ਨਹੀਂਤਾਂ ਜੇ (AstXor* ਅਚਲ rhsp = VN_CAST(nodep->rhsp(), Xor)) {
                did = expandWide(nodep, rhsp);
            } ਨਹੀਂਤਾਂ ਜੇ (Ast_ਖਿਸਕਾਓ_L* ਅਚਲ rhsp = VN_CAST(nodep->rhsp(), ਖਿਸਕਾਓ_L)) {
                did = expand_Wide_ਖਿਸਕਾਓ(nodep, rhsp, /* isLeftShift: */ ਸੱਚ);
            } ਨਹੀਂਤਾਂ ਜੇ (Ast_ਖਿਸਕਾਓ_R* ਅਚਲ rhsp = VN_CAST(nodep->rhsp(), ਖਿਸਕਾਓ_R)) {
                did = expand_Wide_ਖਿਸਕਾਓ(nodep, rhsp, /* isLeftShift: */ ਝੂਠ);
            } ਨਹੀਂਤਾਂ ਜੇ (AstCond* ਅਚਲ rhsp = VN_CAST(nodep->rhsp(), Cond)) {
                did = expandWide(nodep, rhsp);
            }
        } ਨਹੀਂਤਾਂ ਜੇ (AstSel* ਅਚਲ lhsp = VN_CAST(nodep->lhsp(), Sel)) {
            did = expandLhs(nodep, lhsp);
        }
        // Cleanup common code
        ਜੇ (did) VL_DO_DANGLING(nodep->unlink_Fr_ਪਿੱਠ()->delete_ਰੁੱਖ(), nodep);
    }

    //--------------------
    ਖਾਲੀ ਦੇਖੋ(AstVar*) override {}  // Don't hit varrefs under vars
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override { iterateChildren(nodep); }

ਜਨਤਕ:
    // CONSTRUCTORS
    ਸਪਸ਼ਟ ExpandVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~ExpandVisitor() override {
        V3Stats::ਜੋੜੋ_Stat("Optimizations, Expand, expanded wides", m_statWides);
        V3Stats::ਜੋੜੋ_Stat("Optimizations, Expand, expanded wide words", m_stat_Wide_ਸ਼ਬਦ);
        V3Stats::ਜੋੜੋ_Stat("Optimizations, Expand, not expanded impure", m_statImpure);
        V3Stats::ਜੋੜੋ_Stat("Optimizations, Expand, not expanded limit", m_statWideLimited);
        V3Stats::ਜੋੜੋ_Stat("Optimizations, Expand, temporaries created", m_statTemps);
        ਅਚਲ ਸਵੈਚਾਲਿਤ emitStat = [ਇਹ](VExpand_ਨਮੂਨਾ ਨਮੂਨਾ_2) {
            std::string str{ਨਮੂਨਾ_2.ascii()};
            std::transform(str.begin(), str.ਸਿਰਾ(), str.begin(), [](ਬਿਨਾਨਿਸ਼ਾਨੀ ਅੱਖਰ c) {  //
                ਵਾਪਸ c == '_' ? ' ' : std::tolower(c);
            });
            V3Stats::ਜੋੜੋ_Stat("Optimizations, Expand, pattern " + str, m_statPatterns[ਨਮੂਨਾ_2]);
        };
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ EXPAND_ਨਮੂਨਾ_EMIT_STAT(id) emitStat(VExpand_ਨਮੂਨਾ::id);
        FOR_EACH_EXPAND_ਨਮੂਨਾ(EXPAND_ਨਮੂਨਾ_EMIT_STAT)
#ਪਰਿਭਾਸ਼ਾ_ਰੱਦ EXPAND_ਨਮੂਨਾ_EMIT_STAT
    }
};

//######################################################################
// Expand class functions

ਖਾਲੀ V3Expand::expandAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        ਅਚਲ ExpandOkVisitor okVisitor{nodep};
        ExpandVisitor{nodep};
    }  // Destruct before checking
    V3Global::dump_Check_Global_ਰੁੱਖ("expand", 0, dump_ਰੁੱਖ_Either_Level() >= 3);
}
