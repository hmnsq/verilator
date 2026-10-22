// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Substitute constants and expressions in expr temp's
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
// V3Subst's Transformations:
//
// Each module:
//      Search all ASSIGN(WORDSEL(...)) and build what it's assigned to
//      Later usages of that word may then be replaced as long as
//      the RHS hasn't changed value.
//
//*************************************************************************

#ਅਨ੍ਤਰ੍ਭਾਵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Subst.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Const.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Stats.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Data strored for every variable that tracks assignments and usage

ਵਰਗ Subst_Var_ਇੰਦਰਾਜ final {
    ਮਿੱਤਰ ਵਰਗ SubstValidVisitor;

    // SubstVarEntry contains a Record for each word, and one extra for the whole variable
    ਬਣਤਰ ਰਿਕਾਰਡ_2 final {
        // MEMBERS
        Ast_ਗੰਢ_ਸੌਂਪੋ* m_assignp;  // Last assignment to this part
        uint32_t m_ਕਦਮ;  // Step number of last assignment. 0 means invalid entry.
        ਤਾਰਕਿਕ m_ਵਰਤਿਆ;  // True if consumed. Can be set even if entry is invalid.
        // CONSTRUCTOR
        ਰਿਕਾਰਡ_2() { invalidate(); }
        // METHODS
        ਖਾਲੀ invalidate() {
            m_assignp = ਖਾਲੀ_ਸੂਚਕ;
            m_ਕਦਮ = 0;
            m_ਵਰਤਿਆ = ਝੂਠ;
        }
    };

    // MEMBERS
    // Variable this SubstVarEntry tracks
    AstVar* ਅਚਲ m_varp;
    // The recrod for whole variable tracking
    ਰਿਕਾਰਡ_2 m_ਸਾਰਾ_ਰਿਕਾਰਡ;
    // A record for each word in the variable
    std::ਸਦਿਸ਼<ਰਿਕਾਰਡ_2> m_ਸ਼ਬਦ_ਰਿਕਾਰਡਾਂ{static_cast<ਮਾਪ_t>(m_varp->ਚੌੜਾਈ_ਸ਼ਬਦ()), ਰਿਕਾਰਡ_2{}};

    // METHDOS
    ਖਾਲੀ delete_Assignment_If_ਅਣਵਰਤਿਆ(ਰਿਕਾਰਡ_2& ਰਿਕਾਰਡ, ਮਾਪ_t& n_ਸੌਂਪੋ_Deleted) {
        ਜੇ (!ਰਿਕਾਰਡ.m_assignp) ਵਾਪਸ;
        ਜੇ (ਰਿਕਾਰਡ.m_ਵਰਤਿਆ) ਵਾਪਸ;
        ++n_ਸੌਂਪੋ_Deleted;
        VL_DO_DANGLING(ਰਿਕਾਰਡ.m_assignp->unlink_Fr_ਪਿੱਠ()->delete_ਰੁੱਖ(), ਰਿਕਾਰਡ.m_assignp);
    }

    Ast_ਗੰਢ_ਵਿਅੰਜਕ* subst_ਰਿਕਾਰਡ(ਰਿਕਾਰਡ_2& ਰਿਕਾਰਡ);

ਜਨਤਕ:
    // CONSTRUCTORS
    ਸਪਸ਼ਟ Subst_Var_ਇੰਦਰਾਜ(AstVar* varp)
        : m_varp{varp} {}
    ~Subst_Var_ਇੰਦਰਾਜ() = ਮੂਲ;

    // Record assignment of whole variable. The given 'assp' can be null, which means
    // the variable is known to be assigned, but to an unknown value.
    ਖਾਲੀ ਸੌਂਪੋ_ਸਾਰਾ(Ast_ਗੰਢ_ਸੌਂਪੋ* assp, uint32_t ਕਦਮ) {
        // Invalidate all word records
        ਲਈ (ਰਿਕਾਰਡ_2& ਸ਼ਬਦ_ਰਿਕਾਰਡ : m_ਸ਼ਬਦ_ਰਿਕਾਰਡਾਂ) ਸ਼ਬਦ_ਰਿਕਾਰਡ.invalidate();
        // Set whole record
        m_ਸਾਰਾ_ਰਿਕਾਰਡ.m_assignp = assp;
        m_ਸਾਰਾ_ਰਿਕਾਰਡ.m_ਕਦਮ = ਕਦਮ;
        m_ਸਾਰਾ_ਰਿਕਾਰਡ.m_ਵਰਤਿਆ = ਝੂਠ;
    }

    // Like assignWhole word, but records assignment to a specific word of the variable.
    ਖਾਲੀ ਸੌਂਪੋ_ਸ਼ਬਦ(Ast_ਗੰਢ_ਸੌਂਪੋ* assp, uint32_t ਕਦਮ, uint32_t ਸ਼ਬਦ) {
        // Invalidate whole record
        m_ਸਾਰਾ_ਰਿਕਾਰਡ.invalidate();
        // Set word record
        ਰਿਕਾਰਡ_2& ਸ਼ਬਦ_ਰਿਕਾਰਡ = m_ਸ਼ਬਦ_ਰਿਕਾਰਡਾਂ[ਸ਼ਬਦ];
        ਸ਼ਬਦ_ਰਿਕਾਰਡ.m_assignp = assp;
        ਸ਼ਬਦ_ਰਿਕਾਰਡ.m_ਕਦਮ = ਕਦਮ;
        ਸ਼ਬਦ_ਰਿਕਾਰਡ.m_ਵਰਤਿਆ = ਝੂਠ;
    }

    // Mark the whole variable as used (value consumed)
    ਖਾਲੀ ਵਰਤਿਆ_ਸਾਰਾ() {
        m_ਸਾਰਾ_ਰਿਕਾਰਡ.m_ਵਰਤਿਆ = ਸੱਚ;
        ਲਈ (ਰਿਕਾਰਡ_2& ਸ਼ਬਦ_ਰਿਕਾਰਡ : m_ਸ਼ਬਦ_ਰਿਕਾਰਡਾਂ) ਸ਼ਬਦ_ਰਿਕਾਰਡ.m_ਵਰਤਿਆ = ਸੱਚ;
    }

    // Mark the specific word as used (value consumed)
    ਖਾਲੀ ਵਰਤਿਆ_ਸ਼ਬਦ(uint32_t ਸ਼ਬਦ) {
        m_ਸਾਰਾ_ਰਿਕਾਰਡ.m_ਵਰਤਿਆ = ਸੱਚ;
        m_ਸ਼ਬਦ_ਰਿਕਾਰਡਾਂ[ਸ਼ਬਦ].m_ਵਰਤਿਆ = ਸੱਚ;
    }

    // Returns substitution of whole word, or nullptr if not known/stale
    Ast_ਗੰਢ_ਵਿਅੰਜਕ* subst_ਸਾਰਾ() { ਵਾਪਸ subst_ਰਿਕਾਰਡ(m_ਸਾਰਾ_ਰਿਕਾਰਡ); }
    // Returns substitution of whole word, or nullptr if not known/stale
    Ast_ਗੰਢ_ਵਿਅੰਜਕ* subst_ਸ਼ਬਦ(uint32_t ਸ਼ਬਦ) { ਵਾਪਸ subst_ਰਿਕਾਰਡ(m_ਸ਼ਬਦ_ਰਿਕਾਰਡਾਂ[ਸ਼ਬਦ]); }

    ਖਾਲੀ delete_ਅਣਵਰਤਿਆ_Assignments(ਮਾਪ_t& n_ਸ਼ਬਦ_ਸੌਂਪੋ_Deleted, ਮਾਪ_t& n_ਸਾਰਾ_ਸੌਂਪੋ_Deleted) {
        // Delete assignments to temporaries if they are not used
        ਜੇ (!m_varp->is_Statement_ਆਰਜ਼ੀ()) ਵਾਪਸ;
        ਲਈ (ਰਿਕਾਰਡ_2& ਸ਼ਬਦ_ਰਿਕਾਰਡ : m_ਸ਼ਬਦ_ਰਿਕਾਰਡਾਂ)
            delete_Assignment_If_ਅਣਵਰਤਿਆ(ਸ਼ਬਦ_ਰਿਕਾਰਡ, n_ਸ਼ਬਦ_ਸੌਂਪੋ_Deleted);
        delete_Assignment_If_ਅਣਵਰਤਿਆ(m_ਸਾਰਾ_ਰਿਕਾਰਡ, n_ਸਾਰਾ_ਸੌਂਪੋ_Deleted);
    }
};

//######################################################################
// See if any variables in the expression we are considering to
// substitute have changed value since we recorded the assignment.

ਵਰਗ SubstValidVisitor final : ਜਨਤਕ VNVisitorConst {
    // NODE STATE
    // See SubstVisitor

    // STATE
    ਅਚਲ uint32_t m_ਕਦਮ;  // Step number where assignment was recorded
    ਤਾਰਕਿਕ m_valid = ਸੱਚ;  // Is the expression we are considering to substitute valid

    // METHODS
    Subst_Var_ਇੰਦਰਾਜ& get_ਇੰਦਰਾਜ(AstVarRef* nodep) {
        // This vistor is always invoked on the RHS of an assignment we are considering to
        // substitute. Variable references must have all been recorded when visiting the
        // assignments RHS, so the SubstVarEntry must exist for each referenced variable.
        ਵਾਪਸ *nodep->varp()->user1u().ਮੰਜ਼ਿਲ<Subst_Var_ਇੰਦਰਾਜ*>();
    }

    // VISITORS
    ਖਾਲੀ ਦੇਖੋ(Ast_ਸ਼ਬਦ_Sel* nodep) override {
        ਜੇ (!m_valid) ਵਾਪਸ;

        ਜੇ (AstVarRef* ਅਚਲ refp = VN_CAST(nodep->fromp(), VarRef)) {
            ਜੇ (AstConst* ਅਚਲ idxp = VN_CAST(nodep->bitp(), Const)) {
                Subst_Var_ਇੰਦਰਾਜ& ਇੰਦਰਾਜ = get_ਇੰਦਰਾਜ(refp);
                // If either the whole variable, or the indexed word was written to
                // after the original assignment was recorded, the value is invalid.
                ਜੇ (m_ਕਦਮ < ਇੰਦਰਾਜ.m_ਸਾਰਾ_ਰਿਕਾਰਡ.m_ਕਦਮ) m_valid = ਝੂਠ;
                ਜੇ (m_ਕਦਮ < ਇੰਦਰਾਜ.m_ਸ਼ਬਦ_ਰਿਕਾਰਡਾਂ[idxp->ਮੰਜ਼ਿਲ_UInt()].m_ਕਦਮ) m_valid = ਝੂਠ;
                ਵਾਪਸ;
            }
        }
        iterateChildrenConst(nodep);
    }

    ਖਾਲੀ ਦੇਖੋ(AstVarRef* nodep) override {
        ਜੇ (!m_valid) ਵਾਪਸ;

        // If either the whole variable, or any of the words were written to
        // after the original assignment was recorded, the value is invalid.
        ਅਚਲ Subst_Var_ਇੰਦਰਾਜ& ਇੰਦਰਾਜ = get_ਇੰਦਰਾਜ(nodep);
        ਜੇ (m_ਕਦਮ < ਇੰਦਰਾਜ.m_ਸਾਰਾ_ਰਿਕਾਰਡ.m_ਕਦਮ) {
            m_valid = ਝੂਠ;
            ਵਾਪਸ;
        }
        ਲਈ (ਅਚਲ Subst_Var_ਇੰਦਰਾਜ::ਰਿਕਾਰਡ_2& ਸ਼ਬਦ_ਰਿਕਾਰਡ : ਇੰਦਰਾਜ.m_ਸ਼ਬਦ_ਰਿਕਾਰਡਾਂ) {
            ਜੇ (m_ਕਦਮ < ਸ਼ਬਦ_ਰਿਕਾਰਡ.m_ਕਦਮ) {
                m_valid = ਝੂਠ;
                ਵਾਪਸ;
            }
        }
    }

    ਖਾਲੀ ਦੇਖੋ(AstConst*) override {}  // Accelerate
    ਖਾਲੀ ਦੇਖੋ(Ast_ਪਾਠ*) override {}  // CExpr/CStmt literal text has no variable dependencies

    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_ਵਿਅੰਜਕ* nodep) override {
        ਜੇ (!m_valid) ਵਾਪਸ;
        iterateChildrenConst(nodep);
    }

    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override { nodep->v3fatalSrc("Non AstNodeExpr under AstNodeExpr"); }

    // CONSTRUCTORS
    ਸਪਸ਼ਟ SubstValidVisitor(Subst_Var_ਇੰਦਰਾਜ::ਰਿਕਾਰਡ_2& ਰਿਕਾਰਡ)
        : m_ਕਦਮ{ਰਿਕਾਰਡ.m_ਕਦਮ} {
        iterateConst(ਰਿਕਾਰਡ.m_assignp->rhsp());
    }
    ~SubstValidVisitor() override = ਮੂਲ;

ਜਨਤਕ:
    ਸਥਿਰ ਤਾਰਕਿਕ valid(Subst_Var_ਇੰਦਰਾਜ::ਰਿਕਾਰਡ_2& ਰਿਕਾਰਡ) {
        ਜੇ (!ਰਿਕਾਰਡ.m_assignp) ਵਾਪਸ ਝੂਠ;
        ਵਾਪਸ SubstValidVisitor{ਰਿਕਾਰਡ}.m_valid;
    }
};

Ast_ਗੰਢ_ਵਿਅੰਜਕ* Subst_Var_ਇੰਦਰਾਜ::subst_ਰਿਕਾਰਡ(Subst_Var_ਇੰਦਰਾਜ::ਰਿਕਾਰਡ_2& ਰਿਕਾਰਡ) {
    ਜੇ (!SubstValidVisitor::valid(ਰਿਕਾਰਡ)) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
    Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ rhsp = ਰਿਕਾਰਡ.m_assignp->rhsp();
    UDEBUGONLY(UASSERT_OBJ(rhsp->is_ਸ਼ੁੱਧ(), ਰਿਕਾਰਡ.m_assignp, "Substituting impure expression"););
    ਵਾਪਸ rhsp;
}

//######################################################################
// Substitution visitor

ਵਰਗ SubstVisitor final : ਜਨਤਕ VNVisitor {
    // NODE STATE - only Under AstCFunc
    // AstVar::user1p -> SubstVarEntry* for assignment tracking. Also used by SubstValidVisitor

    // STATE
    std::deque<Subst_Var_ਇੰਦਰਾਜ> m_entries;  // Storage for SubstVarEntry instances
    uint32_t m_ops = 0;  // Number of nodes on the RHS of an assignment
    uint32_t m_ਸੌਂਪੋ_ਕਦਮ = 0;  // Assignment number to determine variable lifetimes
    ਅਚਲ AstCFunc* m_funcp = ਖਾਲੀ_ਸੂਚਕ;  // Current function we are under
    ਮਾਪ_t m_nSubst = 0;  // Number of substitutions performed - for avoiding constant folding
    // Statistics
    ਮਾਪ_t m_n_ਸ਼ਬਦ_Substituted = 0;  // Number of words substituted
    ਮਾਪ_t m_n_ਸਾਰਾ_Substituted = 0;  // Number of whole variables substituted
    ਮਾਪ_t m_n_ਸ਼ਬਦ_ਸੌਂਪੋ_Deleted = 0;  // Number of word assignments deleted
    ਮਾਪ_t m_n_ਸਾਰਾ_ਸੌਂਪੋ_Deleted = 0;  // Number of whole variable assignments deleted
    ਮਾਪ_t m_n_Const_ਸ਼ਬਦ_Reinlined = 0;  // Number of constant words substituted

    ਸਥਿਰ constexpr uint32_t SUBST_MAX_OPS_SUBST = 30;  // Maximum number of ops to substitute in
    ਸਥਿਰ constexpr uint32_t SUBST_MAX_OPS_NA = 9999;  // Not allowed to substitute

    // METHODS
    Subst_Var_ਇੰਦਰਾਜ& get_ਇੰਦਰਾਜ(AstVarRef* nodep) {
        AstVar* ਅਚਲ varp = nodep->varp();
        ਜੇ (!varp->user1p()) {
            m_entries.emplace_ਪਿੱਠ(varp);
            varp->user1p(&m_entries.ਪਿੱਠ());
        }
        ਵਾਪਸ *varp->user1u().ਮੰਜ਼ਿਲ<Subst_Var_ਇੰਦਰਾਜ*>();
    }

    ਖਾਲੀ simplify(Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp) {
        // Often constant, so short circuit
        ਜੇ (VN_IS(exprp, Const)) ਵਾਪਸ;
        // Iterate expression, then constant fold it if anything changed
        ਅਚਲ ਮਾਪ_t nSubstBefore = m_nSubst;
        exprp = VN_AS(iterateSubtreeReturnEdits(exprp), ਗੰਢ_ਵਿਅੰਜਕ);
        ਜੇ (nSubstBefore != m_nSubst) V3Const::constifyEditCpp(exprp);
    }

    // The way the analysis algorithm works based on statement numbering only works
    // for variables that are written in the same basic block (ignoring AstCond) as
    // where they are consumed. The temporaries introduced in V3Premit are such, so
    // only substitute those for now.
    ਸਥਿਰ ਤਾਰਕਿਕ isSubstitutable(AstVar* nodep) {
        ਵਾਪਸ nodep->is_Statement_ਆਰਜ਼ੀ() && !nodep->noSubst();
    }

    ਖਾਲੀ substitute(Ast_ਗੰਢ* nodep, Ast_ਗੰਢ_ਵਿਅੰਜਕ* substp) {
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* newp = substp->backp() ? substp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਸੱਚ) : substp;
        ਜੇ (!nodep->isQuad() && newp->isQuad()) {
            newp = ਨਵਾਂ AstCCast{newp->fileline(), newp, nodep};
        }
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_ਰੁੱਖ(), nodep);
        ++m_nSubst;
    }

    // VISITORS

    ਖਾਲੀ ਦੇਖੋ(AstNetlist* nodep) override { iterate_And_ਅਗਲਾ_Null(nodep->modulesp()); }

    ਖਾਲੀ ਦੇਖੋ(AstCFunc* nodep) override {
        UASSERT_OBJ(!m_funcp, nodep, "Should not nest");
        UASSERT_OBJ(m_entries.ਖਾਲੀ_ਥਾਵਾਂ(), nodep, "Should not visit outside functions");

        // Process the function body
        {
            VL_RESTORER(m_funcp);
            m_funcp = nodep;
            ਅਚਲ VNUser1_ਵਿੱਚ_Use m_inuser1;
            iterateChildren(nodep);
            // Deletes unused assignments and clear entries
            ਲਈ (Subst_Var_ਇੰਦਰਾਜ& ਇੰਦਰਾਜ : m_entries) {
                ਇੰਦਰਾਜ.delete_ਅਣਵਰਤਿਆ_Assignments(m_n_ਸ਼ਬਦ_ਸੌਂਪੋ_Deleted, m_n_ਸਾਰਾ_ਸੌਂਪੋ_Deleted);
            }
            m_entries.clear();
        }

        // Constant fold here, as Ast size can likely be reduced
        ਜੇ (v3Global.opt.fConstEager()) V3Const::constifyEditCpp(nodep);
    }

    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_ਸੌਂਪੋ* nodep) override {
        ਜੇ (!m_funcp) ਵਾਪਸ;

        ਅਚਲ uint32_t ops = [&]() {
            VL_RESTORER(m_ops);
            m_ops = 0;
            // Simplify the RHS
            simplify(nodep->rhsp());
            // If the became constant, we can continue substituting it
            ਵਾਪਸ VN_IS(nodep->rhsp(), Const) ? 0 : m_ops;
        }();

        // Assignment that defines the value, which is either this 'nodep',
        // or nullptr, if the value should not be substituted.
        ਅਚਲ ਸਵੈਚਾਲਿਤ getAssignp = [&](AstVarRef* refp) -> Ast_ਗੰਢ_ਸੌਂਪੋ* {
            // If too complex, don't substitute
            ਜੇ (ops > SUBST_MAX_OPS_SUBST) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
            // AstCvtPackedToArray can't be anywhere else than on the RHS of assignment
            ਜੇ (VN_IS(nodep->rhsp(), Cvt_Packed_ਮੰਜ਼ਿਲ_Array)) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
            // If non-const but want const subtitutions only
            ਜੇ (refp->varp()->substConstOnly() && !VN_IS(nodep->rhsp(), Const)) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
            // Otherwise can substitute based on the assignment
            ਵਾਪਸ nodep;
        };

        // If LHS is a whole variable reference, track the whole variable
        ਜੇ (AstVarRef* ਅਚਲ refp = VN_CAST(nodep->lhsp(), VarRef)) {
            get_ਇੰਦਰਾਜ(refp).ਸੌਂਪੋ_ਸਾਰਾ(getAssignp(refp), ++m_ਸੌਂਪੋ_ਕਦਮ);
            ਵਾਪਸ;
        }

        // If LHS is a known word reference, track the word
        ਜੇ (ਅਚਲ Ast_ਸ਼ਬਦ_Sel* ਅਚਲ selp = VN_CAST(nodep->lhsp(), ਸ਼ਬਦ_Sel)) {
            ਜੇ (AstVarRef* ਅਚਲ refp = VN_CAST(selp->fromp(), VarRef)) {
                // Simplify the index
                simplify(selp->bitp());
                ਜੇ (ਅਚਲ AstConst* ਅਚਲ idxp = VN_CAST(selp->bitp(), Const)) {
                    get_ਇੰਦਰਾਜ(refp).ਸੌਂਪੋ_ਸ਼ਬਦ(getAssignp(refp), ++m_ਸੌਂਪੋ_ਕਦਮ, idxp->ਮੰਜ਼ਿਲ_UInt());
                    ਵਾਪਸ;
                }
            }
        }

        // Not tracked, iterate LHS to simplify/reset tracking
        iterate(nodep->lhsp());
    }

    ਖਾਲੀ ਦੇਖੋ(Ast_ਸ਼ਬਦ_Sel* nodep) override {
        ਜੇ (!m_funcp) ਵਾਪਸ;

        // Simplify the index
        simplify(nodep->bitp());

        // If this is a known word reference, track/substitute it
        ਜੇ (AstVarRef* ਅਚਲ refp = VN_CAST(nodep->fromp(), VarRef)) {
            ਜੇ (ਅਚਲ AstConst* ਅਚਲ idxp = VN_CAST(nodep->bitp(), Const)) {
                Subst_Var_ਇੰਦਰਾਜ& ਇੰਦਰਾਜ = get_ਇੰਦਰਾਜ(refp);
                ਅਚਲ uint32_t ਸ਼ਬਦ = idxp->ਮੰਜ਼ਿਲ_UInt();

                // If it's a write, reset tracking as we don't know the assigned value,
                // otherwise we would have picked it up in visit(AstNodeAssign*)
                ਜੇ (refp->access().isWriteOrRW()) {
                    ਇੰਦਰਾਜ.ਸੌਂਪੋ_ਸ਼ਬਦ(ਖਾਲੀ_ਸੂਚਕ, ++m_ਸੌਂਪੋ_ਕਦਮ, ਸ਼ਬਦ);
                    ਵਾਪਸ;
                }

                // Otherwise it's a read,
                UASSERT_OBJ(refp->access().isReadOnly(), nodep, "Invalid access");

                // If it's a constant pool variable, substiute with the constant word
                AstVar* ਅਚਲ varp = refp->varp();
                ਜੇ (varp->const_Pool_ਇੰਦਰਾਜ()) {
                    AstConst* ਅਚਲ constp = VN_AS(varp->valuep(), Const);
                    ਅਚਲ uint32_t ਮੁੱਲ = constp->ਅੰਕ().edata_ਸ਼ਬਦ(ਸ਼ਬਦ);
                    File_ਸਤਰ* ਅਚਲ flp = nodep->fileline();
                    ++m_n_Const_ਸ਼ਬਦ_Reinlined;
                    substitute(nodep, ਨਵਾਂ AstConst{flp, AstConst::SizedEData{}, ਮੁੱਲ});
                    ਵਾਪਸ;
                }

                // Substitute other variables if possible
                ਜੇ (isSubstitutable(refp->varp())) {
                    ਜੇ (Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ substp = ਇੰਦਰਾਜ.subst_ਸ਼ਬਦ(ਸ਼ਬਦ)) {
                        ++m_n_ਸ਼ਬਦ_Substituted;
                        substitute(nodep, substp);
                        ਵਾਪਸ;
                    }
                }

                // If not substituted, mark the assignment setting this word as used
                ਇੰਦਰਾਜ.ਵਰਤਿਆ_ਸ਼ਬਦ(ਸ਼ਬਦ);
                ਵਾਪਸ;
            }
        }

        // If not a known word reference, iterate fromp to simplify/reset tracking
        iterate(nodep->fromp());
    }

    ਖਾਲੀ ਦੇਖੋ(AstVarRef* nodep) override {
        ਜੇ (!m_funcp) ਵਾਪਸ;

        Subst_Var_ਇੰਦਰਾਜ& ਇੰਦਰਾਜ = get_ਇੰਦਰਾਜ(nodep);

        // If it's a write, reset tracking as we don't know the assigned value,
        // otherwise we would have picked it up in visit(AstNodeAssign*)
        ਜੇ (nodep->access().isWriteOrRW()) {
            ਇੰਦਰਾਜ.ਸੌਂਪੋ_ਸਾਰਾ(ਖਾਲੀ_ਸੂਚਕ, ++m_ਸੌਂਪੋ_ਕਦਮ);
            ਵਾਪਸ;
        }

        // Otherwise it's a read, substitute it if possible
        UASSERT_OBJ(nodep->access().isReadOnly(), nodep, "Invalid access");
        ਜੇ (isSubstitutable(nodep->varp())) {
            ਜੇ (Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ substp = ਇੰਦਰਾਜ.subst_ਸਾਰਾ()) {
                // Do not substitute a compound wide expression.
                // The whole point of adding temporaries is to eliminate them.
                ਜੇ (!nodep->isWide() || VN_IS(substp, VarRef)) {
                    ++m_n_ਸਾਰਾ_Substituted;
                    substitute(nodep, substp);
                    ਵਾਪਸ;
                }
            }
        }

        // If not substituted, mark the assignment setting this variable as used
        ਇੰਦਰਾਜ.ਵਰਤਿਆ_ਸਾਰਾ();
    }

    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_ਵਿਅੰਜਕ* nodep) override {
        ਜੇ (!m_funcp) ਵਾਪਸ;

        // Count nodes as we descend to track complexity
        ++m_ops;
        // First iterate children, this can cache child purity
        iterateChildren(nodep);
        // Do not substitute impure expressions
        ਜੇ (!nodep->is_ਸ਼ੁੱਧ()) m_ops = SUBST_MAX_OPS_NA;
    }

    ਖਾਲੀ ਦੇਖੋ(AstVar*) override {}  // Accelerate
    ਖਾਲੀ ਦੇਖੋ(AstConst*) override {}  // Accelerate

    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override { iterateChildren(nodep); }

ਜਨਤਕ:
    // CONSTRUCTORS
    ਸਪਸ਼ਟ SubstVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~SubstVisitor() override {
        V3Stats::ਜੋੜੋ_Stat("Optimizations, Subst, Substituted temps", m_nSubst);
        V3Stats::ਜੋੜੋ_Stat("Optimizations, Subst, Constant words reinlined", m_n_Const_ਸ਼ਬਦ_Reinlined);
        V3Stats::ਜੋੜੋ_Stat("Optimizations, Subst, Whole variable assignments deleted",
                         m_n_ਸਾਰਾ_ਸੌਂਪੋ_Deleted);
        V3Stats::ਜੋੜੋ_Stat("Optimizations, Subst, Whole variables substituted", m_n_ਸਾਰਾ_Substituted);
        V3Stats::ਜੋੜੋ_Stat("Optimizations, Subst, Word assignments deleted", m_n_ਸ਼ਬਦ_ਸੌਂਪੋ_Deleted);
        V3Stats::ਜੋੜੋ_Stat("Optimizations, Subst, Words substituted", m_n_ਸ਼ਬਦ_Substituted);
        UASSERT(m_entries.ਖਾਲੀ_ਥਾਵਾਂ(), "Should not visit outside functions");
    }
};

//######################################################################
// Subst class functions

ਖਾਲੀ V3Subst::substituteAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { SubstVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_ਰੁੱਖ("subst", 0, dump_ਰੁੱਖ_Either_Level() >= 3);
}
