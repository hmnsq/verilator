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

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ಅನ್ತರ್ಭಾವ "V3Subst.h"

#ಅನ್ತರ್ಭಾವ "V3Const.h"
#ಅನ್ತರ್ಭಾವ "V3Stats.h"

#ಅನ್ತರ್ಭಾವ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Data strored for every variable that tracks assignments and usage

ವರ್ಗ Subst_Var_ನಮೂದು final {
    ಮಿತ್ರ ವರ್ಗ SubstValidVisitor;

    // SubstVarEntry contains a Record for each word, and one extra for the whole variable
    ರಚನೆ ದಾಖಲೆ_2 final {
        // MEMBERS
        Ast_ಗಂಟು_ನಿಯೋಜಿಸು* m_assignp;  // Last assignment to this part
        uint32_t m_ಹೆಜ್ಜೆ;  // Step number of last assignment. 0 means invalid entry.
        ತಾರ್ಕಿಕ m_ಬಳಸಿದ;  // True if consumed. Can be set even if entry is invalid.
        // CONSTRUCTOR
        ದಾಖಲೆ_2() { invalidate(); }
        // METHODS
        ಶೂನ್ಯ invalidate() {
            m_assignp = ಶೂನ್ಯಸೂಚಕ;
            m_ಹೆಜ್ಜೆ = 0;
            m_ಬಳಸಿದ = ಸುಳ್ಳು;
        }
    };

    // MEMBERS
    // Variable this SubstVarEntry tracks
    AstVar* ಸ್ಥಿರಾಂಕ m_varp;
    // The recrod for whole variable tracking
    ದಾಖಲೆ_2 m_ಸಂಪೂರ್ಣ_ದಾಖಲೆ;
    // A record for each word in the variable
    std::ಸದಿಶ<ದಾಖಲೆ_2> m_ಶಬ್ದ_ದಾಖಲೆಗಳು{static_cast<ಗಾತ್ರ_t>(m_varp->ಅಗಲ_ಪದಗಳು()), ದಾಖಲೆ_2{}};

    // METHDOS
    ಶೂನ್ಯ delete_Assignment_If_ಬಳಸದ(ದಾಖಲೆ_2& ದಾಖಲೆ, ಗಾತ್ರ_t& n_ನಿಯೋಜಿಸು_Deleted) {
        ಆದರೆ (!ದಾಖಲೆ.m_assignp) ಹಿಂತಿರುಗಿ;
        ಆದರೆ (ದಾಖಲೆ.m_ಬಳಸಿದ) ಹಿಂತಿರುಗಿ;
        ++n_ನಿಯೋಜಿಸು_Deleted;
        VL_DO_DANGLING(ದಾಖಲೆ.m_assignp->unlink_Fr_ಹಿಂಭಾಗ()->delete_ವೃಕ್ಷ(), ದಾಖಲೆ.m_assignp);
    }

    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* subst_ದಾಖಲೆ(ದಾಖಲೆ_2& ದಾಖಲೆ);

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    ಸ್ಪಷ್ಟ Subst_Var_ನಮೂದು(AstVar* varp)
        : m_varp{varp} {}
    ~Subst_Var_ನಮೂದು() = ಪೂರ್ವನಿಯೋಜಿತ;

    // Record assignment of whole variable. The given 'assp' can be null, which means
    // the variable is known to be assigned, but to an unknown value.
    ಶೂನ್ಯ ನಿಯೋಜಿಸು_ಸಂಪೂರ್ಣ(Ast_ಗಂಟು_ನಿಯೋಜಿಸು* assp, uint32_t ಹೆಜ್ಜೆ) {
        // Invalidate all word records
        ಗಾಗಿ (ದಾಖಲೆ_2& ಶಬ್ದ_ದಾಖಲೆ : m_ಶಬ್ದ_ದಾಖಲೆಗಳು) ಶಬ್ದ_ದಾಖಲೆ.invalidate();
        // Set whole record
        m_ಸಂಪೂರ್ಣ_ದಾಖಲೆ.m_assignp = assp;
        m_ಸಂಪೂರ್ಣ_ದಾಖಲೆ.m_ಹೆಜ್ಜೆ = ಹೆಜ್ಜೆ;
        m_ಸಂಪೂರ್ಣ_ದಾಖಲೆ.m_ಬಳಸಿದ = ಸುಳ್ಳು;
    }

    // Like assignWhole word, but records assignment to a specific word of the variable.
    ಶೂನ್ಯ ನಿಯೋಜಿಸು_ಶಬ್ದ(Ast_ಗಂಟು_ನಿಯೋಜಿಸು* assp, uint32_t ಹೆಜ್ಜೆ, uint32_t ಶಬ್ದ) {
        // Invalidate whole record
        m_ಸಂಪೂರ್ಣ_ದಾಖಲೆ.invalidate();
        // Set word record
        ದಾಖಲೆ_2& ಶಬ್ದ_ದಾಖಲೆ = m_ಶಬ್ದ_ದಾಖಲೆಗಳು[ಶಬ್ದ];
        ಶಬ್ದ_ದಾಖಲೆ.m_assignp = assp;
        ಶಬ್ದ_ದಾಖಲೆ.m_ಹೆಜ್ಜೆ = ಹೆಜ್ಜೆ;
        ಶಬ್ದ_ದಾಖಲೆ.m_ಬಳಸಿದ = ಸುಳ್ಳು;
    }

    // Mark the whole variable as used (value consumed)
    ಶೂನ್ಯ ಬಳಸಿದ_ಸಂಪೂರ್ಣ() {
        m_ಸಂಪೂರ್ಣ_ದಾಖಲೆ.m_ಬಳಸಿದ = ನಿಜ;
        ಗಾಗಿ (ದಾಖಲೆ_2& ಶಬ್ದ_ದಾಖಲೆ : m_ಶಬ್ದ_ದಾಖಲೆಗಳು) ಶಬ್ದ_ದಾಖಲೆ.m_ಬಳಸಿದ = ನಿಜ;
    }

    // Mark the specific word as used (value consumed)
    ಶೂನ್ಯ ಬಳಸಿದ_ಶಬ್ದ(uint32_t ಶಬ್ದ) {
        m_ಸಂಪೂರ್ಣ_ದಾಖಲೆ.m_ಬಳಸಿದ = ನಿಜ;
        m_ಶಬ್ದ_ದಾಖಲೆಗಳು[ಶಬ್ದ].m_ಬಳಸಿದ = ನಿಜ;
    }

    // Returns substitution of whole word, or nullptr if not known/stale
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* subst_ಸಂಪೂರ್ಣ() { ಹಿಂತಿರುಗಿ subst_ದಾಖಲೆ(m_ಸಂಪೂರ್ಣ_ದಾಖಲೆ); }
    // Returns substitution of whole word, or nullptr if not known/stale
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* subst_ಶಬ್ದ(uint32_t ಶಬ್ದ) { ಹಿಂತಿರುಗಿ subst_ದಾಖಲೆ(m_ಶಬ್ದ_ದಾಖಲೆಗಳು[ಶಬ್ದ]); }

    ಶೂನ್ಯ delete_ಬಳಸದ_Assignments(ಗಾತ್ರ_t& n_ಶಬ್ದ_ನಿಯೋಜಿಸು_Deleted, ಗಾತ್ರ_t& n_ಸಂಪೂರ್ಣ_ನಿಯೋಜಿಸು_Deleted) {
        // Delete assignments to temporaries if they are not used
        ಆದರೆ (!m_varp->is_Statement_ತಾತ್ಕಾಲಿಕ()) ಹಿಂತಿರುಗಿ;
        ಗಾಗಿ (ದಾಖಲೆ_2& ಶಬ್ದ_ದಾಖಲೆ : m_ಶಬ್ದ_ದಾಖಲೆಗಳು)
            delete_Assignment_If_ಬಳಸದ(ಶಬ್ದ_ದಾಖಲೆ, n_ಶಬ್ದ_ನಿಯೋಜಿಸು_Deleted);
        delete_Assignment_If_ಬಳಸದ(m_ಸಂಪೂರ್ಣ_ದಾಖಲೆ, n_ಸಂಪೂರ್ಣ_ನಿಯೋಜಿಸು_Deleted);
    }
};

//######################################################################
// See if any variables in the expression we are considering to
// substitute have changed value since we recorded the assignment.

ವರ್ಗ SubstValidVisitor final : ಸಾರ್ವಜನಿಕ VNVisitorConst {
    // NODE STATE
    // See SubstVisitor

    // STATE
    ಸ್ಥಿರಾಂಕ uint32_t m_ಹೆಜ್ಜೆ;  // Step number where assignment was recorded
    ತಾರ್ಕಿಕ m_valid = ನಿಜ;  // Is the expression we are considering to substitute valid

    // METHODS
    Subst_Var_ನಮೂದು& get_ನಮೂದು(AstVarRef* nodep) {
        // This vistor is always invoked on the RHS of an assignment we are considering to
        // substitute. Variable references must have all been recorded when visiting the
        // assignments RHS, so the SubstVarEntry must exist for each referenced variable.
        ಹಿಂತಿರುಗಿ *nodep->varp()->user1u().ಗಮ್ಯ<Subst_Var_ನಮೂದು*>();
    }

    // VISITORS
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಶಬ್ದ_Sel* nodep) override {
        ಆದರೆ (!m_valid) ಹಿಂತಿರುಗಿ;

        ಆದರೆ (AstVarRef* ಸ್ಥಿರಾಂಕ refp = VN_CAST(nodep->fromp(), VarRef)) {
            ಆದರೆ (AstConst* ಸ್ಥಿರಾಂಕ idxp = VN_CAST(nodep->bitp(), Const)) {
                Subst_Var_ನಮೂದು& ನಮೂದು = get_ನಮೂದು(refp);
                // If either the whole variable, or the indexed word was written to
                // after the original assignment was recorded, the value is invalid.
                ಆದರೆ (m_ಹೆಜ್ಜೆ < ನಮೂದು.m_ಸಂಪೂರ್ಣ_ದಾಖಲೆ.m_ಹೆಜ್ಜೆ) m_valid = ಸುಳ್ಳು;
                ಆದರೆ (m_ಹೆಜ್ಜೆ < ನಮೂದು.m_ಶಬ್ದ_ದಾಖಲೆಗಳು[idxp->ಗಮ್ಯ_UInt()].m_ಹೆಜ್ಜೆ) m_valid = ಸುಳ್ಳು;
                ಹಿಂತಿರುಗಿ;
            }
        }
        iterateChildrenConst(nodep);
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstVarRef* nodep) override {
        ಆದರೆ (!m_valid) ಹಿಂತಿರುಗಿ;

        // If either the whole variable, or any of the words were written to
        // after the original assignment was recorded, the value is invalid.
        ಸ್ಥಿರಾಂಕ Subst_Var_ನಮೂದು& ನಮೂದು = get_ನಮೂದು(nodep);
        ಆದರೆ (m_ಹೆಜ್ಜೆ < ನಮೂದು.m_ಸಂಪೂರ್ಣ_ದಾಖಲೆ.m_ಹೆಜ್ಜೆ) {
            m_valid = ಸುಳ್ಳು;
            ಹಿಂತಿರುಗಿ;
        }
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Subst_Var_ನಮೂದು::ದಾಖಲೆ_2& ಶಬ್ದ_ದಾಖಲೆ : ನಮೂದು.m_ಶಬ್ದ_ದಾಖಲೆಗಳು) {
            ಆದರೆ (m_ಹೆಜ್ಜೆ < ಶಬ್ದ_ದಾಖಲೆ.m_ಹೆಜ್ಜೆ) {
                m_valid = ಸುಳ್ಳು;
                ಹಿಂತಿರುಗಿ;
            }
        }
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstConst*) override {}  // Accelerate
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಪಠ್ಯ*) override {}  // CExpr/CStmt literal text has no variable dependencies

    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep) override {
        ಆದರೆ (!m_valid) ಹಿಂತಿರುಗಿ;
        iterateChildrenConst(nodep);
    }

    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override { nodep->v3fatalSrc("Non AstNodeExpr under AstNodeExpr"); }

    // CONSTRUCTORS
    ಸ್ಪಷ್ಟ SubstValidVisitor(Subst_Var_ನಮೂದು::ದಾಖಲೆ_2& ದಾಖಲೆ)
        : m_ಹೆಜ್ಜೆ{ದಾಖಲೆ.m_ಹೆಜ್ಜೆ} {
        iterateConst(ದಾಖಲೆ.m_assignp->rhsp());
    }
    ~SubstValidVisitor() override = ಪೂರ್ವನಿಯೋಜಿತ;

ಸಾರ್ವಜನಿಕ:
    ಸ್ಥಿರ ತಾರ್ಕಿಕ valid(Subst_Var_ನಮೂದು::ದಾಖಲೆ_2& ದಾಖಲೆ) {
        ಆದರೆ (!ದಾಖಲೆ.m_assignp) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಹಿಂತಿರುಗಿ SubstValidVisitor{ದಾಖಲೆ}.m_valid;
    }
};

Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* Subst_Var_ನಮೂದು::subst_ದಾಖಲೆ(Subst_Var_ನಮೂದು::ದಾಖಲೆ_2& ದಾಖಲೆ) {
    ಆದರೆ (!SubstValidVisitor::valid(ದಾಖಲೆ)) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ rhsp = ದಾಖಲೆ.m_assignp->rhsp();
    UDEBUGONLY(UASSERT_OBJ(rhsp->is_ಶುದ್ಧ(), ದಾಖಲೆ.m_assignp, "Substituting impure expression"););
    ಹಿಂತಿರುಗಿ rhsp;
}

//######################################################################
// Substitution visitor

ವರ್ಗ SubstVisitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
    // NODE STATE - only Under AstCFunc
    // AstVar::user1p -> SubstVarEntry* for assignment tracking. Also used by SubstValidVisitor

    // STATE
    std::deque<Subst_Var_ನಮೂದು> m_entries;  // Storage for SubstVarEntry instances
    uint32_t m_ops = 0;  // Number of nodes on the RHS of an assignment
    uint32_t m_ನಿಯೋಜಿಸು_ಹೆಜ್ಜೆ = 0;  // Assignment number to determine variable lifetimes
    ಸ್ಥಿರಾಂಕ AstCFunc* m_funcp = ಶೂನ್ಯಸೂಚಕ;  // Current function we are under
    ಗಾತ್ರ_t m_nSubst = 0;  // Number of substitutions performed - for avoiding constant folding
    // Statistics
    ಗಾತ್ರ_t m_n_ಶಬ್ದ_Substituted = 0;  // Number of words substituted
    ಗಾತ್ರ_t m_n_ಸಂಪೂರ್ಣ_Substituted = 0;  // Number of whole variables substituted
    ಗಾತ್ರ_t m_n_ಶಬ್ದ_ನಿಯೋಜಿಸು_Deleted = 0;  // Number of word assignments deleted
    ಗಾತ್ರ_t m_n_ಸಂಪೂರ್ಣ_ನಿಯೋಜಿಸು_Deleted = 0;  // Number of whole variable assignments deleted
    ಗಾತ್ರ_t m_n_Const_ಪದಗಳು_Reinlined = 0;  // Number of constant words substituted

    ಸ್ಥಿರ constexpr uint32_t SUBST_MAX_OPS_SUBST = 30;  // Maximum number of ops to substitute in
    ಸ್ಥಿರ constexpr uint32_t SUBST_MAX_OPS_NA = 9999;  // Not allowed to substitute

    // METHODS
    Subst_Var_ನಮೂದು& get_ನಮೂದು(AstVarRef* nodep) {
        AstVar* ಸ್ಥಿರಾಂಕ varp = nodep->varp();
        ಆದರೆ (!varp->user1p()) {
            m_entries.emplace_ಹಿಂಭಾಗ(varp);
            varp->user1p(&m_entries.ಹಿಂಭಾಗ());
        }
        ಹಿಂತಿರುಗಿ *varp->user1u().ಗಮ್ಯ<Subst_Var_ನಮೂದು*>();
    }

    ಶೂನ್ಯ simplify(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp) {
        // Often constant, so short circuit
        ಆದರೆ (VN_IS(exprp, Const)) ಹಿಂತಿರುಗಿ;
        // Iterate expression, then constant fold it if anything changed
        ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t nSubstBefore = m_nSubst;
        exprp = VN_AS(iterateSubtreeReturnEdits(exprp), ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ);
        ಆದರೆ (nSubstBefore != m_nSubst) V3Const::constifyEditCpp(exprp);
    }

    // The way the analysis algorithm works based on statement numbering only works
    // for variables that are written in the same basic block (ignoring AstCond) as
    // where they are consumed. The temporaries introduced in V3Premit are such, so
    // only substitute those for now.
    ಸ್ಥಿರ ತಾರ್ಕಿಕ isSubstitutable(AstVar* nodep) {
        ಹಿಂತಿರುಗಿ nodep->is_Statement_ತಾತ್ಕಾಲಿಕ() && !nodep->noSubst();
    }

    ಶೂನ್ಯ substitute(Ast_ಗಂಟು* nodep, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* substp) {
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newp = substp->backp() ? substp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ನಿಜ) : substp;
        ಆದರೆ (!nodep->isQuad() && newp->isQuad()) {
            newp = ಹೊಸ AstCCast{newp->fileline(), newp, nodep};
        }
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
        ++m_nSubst;
    }

    // VISITORS

    ಶೂನ್ಯ ಭೇಟಿ(AstNetlist* nodep) override { iterate_And_ಮುಂದಿನ_Null(nodep->modulesp()); }

    ಶೂನ್ಯ ಭೇಟಿ(AstCFunc* nodep) override {
        UASSERT_OBJ(!m_funcp, nodep, "Should not nest");
        UASSERT_OBJ(m_entries.ಖಾಲಿಗಳು(), nodep, "Should not visit outside functions");

        // Process the function body
        {
            VL_RESTORER(m_funcp);
            m_funcp = nodep;
            ಸ್ಥಿರಾಂಕ VNUser1_ಒಳ_Use m_inuser1;
            iterateChildren(nodep);
            // Deletes unused assignments and clear entries
            ಗಾಗಿ (Subst_Var_ನಮೂದು& ನಮೂದು : m_entries) {
                ನಮೂದು.delete_ಬಳಸದ_Assignments(m_n_ಶಬ್ದ_ನಿಯೋಜಿಸು_Deleted, m_n_ಸಂಪೂರ್ಣ_ನಿಯೋಜಿಸು_Deleted);
            }
            m_entries.clear();
        }

        // Constant fold here, as Ast size can likely be reduced
        ಆದರೆ (v3Global.opt.fConstEager()) V3Const::constifyEditCpp(nodep);
    }

    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_ನಿಯೋಜಿಸು* nodep) override {
        ಆದರೆ (!m_funcp) ಹಿಂತಿರುಗಿ;

        ಸ್ಥಿರಾಂಕ uint32_t ops = [&]() {
            VL_RESTORER(m_ops);
            m_ops = 0;
            // Simplify the RHS
            simplify(nodep->rhsp());
            // If the became constant, we can continue substituting it
            ಹಿಂತಿರುಗಿ VN_IS(nodep->rhsp(), Const) ? 0 : m_ops;
        }();

        // Assignment that defines the value, which is either this 'nodep',
        // or nullptr, if the value should not be substituted.
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ getAssignp = [&](AstVarRef* refp) -> Ast_ಗಂಟು_ನಿಯೋಜಿಸು* {
            // If too complex, don't substitute
            ಆದರೆ (ops > SUBST_MAX_OPS_SUBST) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
            // AstCvtPackedToArray can't be anywhere else than on the RHS of assignment
            ಆದರೆ (VN_IS(nodep->rhsp(), Cvt_Packed_ಗಮ್ಯ_Array)) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
            // If non-const but want const subtitutions only
            ಆದರೆ (refp->varp()->substConstOnly() && !VN_IS(nodep->rhsp(), Const)) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
            // Otherwise can substitute based on the assignment
            ಹಿಂತಿರುಗಿ nodep;
        };

        // If LHS is a whole variable reference, track the whole variable
        ಆದರೆ (AstVarRef* ಸ್ಥಿರಾಂಕ refp = VN_CAST(nodep->lhsp(), VarRef)) {
            get_ನಮೂದು(refp).ನಿಯೋಜಿಸು_ಸಂಪೂರ್ಣ(getAssignp(refp), ++m_ನಿಯೋಜಿಸು_ಹೆಜ್ಜೆ);
            ಹಿಂತಿರುಗಿ;
        }

        // If LHS is a known word reference, track the word
        ಆದರೆ (ಸ್ಥಿರಾಂಕ Ast_ಶಬ್ದ_Sel* ಸ್ಥಿರಾಂಕ selp = VN_CAST(nodep->lhsp(), ಶಬ್ದ_Sel)) {
            ಆದರೆ (AstVarRef* ಸ್ಥಿರಾಂಕ refp = VN_CAST(selp->fromp(), VarRef)) {
                // Simplify the index
                simplify(selp->bitp());
                ಆದರೆ (ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ idxp = VN_CAST(selp->bitp(), Const)) {
                    get_ನಮೂದು(refp).ನಿಯೋಜಿಸು_ಶಬ್ದ(getAssignp(refp), ++m_ನಿಯೋಜಿಸು_ಹೆಜ್ಜೆ, idxp->ಗಮ್ಯ_UInt());
                    ಹಿಂತಿರುಗಿ;
                }
            }
        }

        // Not tracked, iterate LHS to simplify/reset tracking
        iterate(nodep->lhsp());
    }

    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಶಬ್ದ_Sel* nodep) override {
        ಆದರೆ (!m_funcp) ಹಿಂತಿರುಗಿ;

        // Simplify the index
        simplify(nodep->bitp());

        // If this is a known word reference, track/substitute it
        ಆದರೆ (AstVarRef* ಸ್ಥಿರಾಂಕ refp = VN_CAST(nodep->fromp(), VarRef)) {
            ಆದರೆ (ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ idxp = VN_CAST(nodep->bitp(), Const)) {
                Subst_Var_ನಮೂದು& ನಮೂದು = get_ನಮೂದು(refp);
                ಸ್ಥಿರಾಂಕ uint32_t ಶಬ್ದ = idxp->ಗಮ್ಯ_UInt();

                // If it's a write, reset tracking as we don't know the assigned value,
                // otherwise we would have picked it up in visit(AstNodeAssign*)
                ಆದರೆ (refp->access().isWriteOrRW()) {
                    ನಮೂದು.ನಿಯೋಜಿಸು_ಶಬ್ದ(ಶೂನ್ಯಸೂಚಕ, ++m_ನಿಯೋಜಿಸು_ಹೆಜ್ಜೆ, ಶಬ್ದ);
                    ಹಿಂತಿರುಗಿ;
                }

                // Otherwise it's a read,
                UASSERT_OBJ(refp->access().isReadOnly(), nodep, "Invalid access");

                // If it's a constant pool variable, substiute with the constant word
                AstVar* ಸ್ಥಿರಾಂಕ varp = refp->varp();
                ಆದರೆ (varp->const_Pool_ನಮೂದು()) {
                    AstConst* ಸ್ಥಿರಾಂಕ constp = VN_AS(varp->valuep(), Const);
                    ಸ್ಥಿರಾಂಕ uint32_t ಮೌಲ್ಯ = constp->ಅಂಕೆ().edata_ಶಬ್ದ(ಶಬ್ದ);
                    File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = nodep->fileline();
                    ++m_n_Const_ಪದಗಳು_Reinlined;
                    substitute(nodep, ಹೊಸ AstConst{flp, AstConst::SizedEData{}, ಮೌಲ್ಯ});
                    ಹಿಂತಿರುಗಿ;
                }

                // Substitute other variables if possible
                ಆದರೆ (isSubstitutable(refp->varp())) {
                    ಆದರೆ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ substp = ನಮೂದು.subst_ಶಬ್ದ(ಶಬ್ದ)) {
                        ++m_n_ಶಬ್ದ_Substituted;
                        substitute(nodep, substp);
                        ಹಿಂತಿರುಗಿ;
                    }
                }

                // If not substituted, mark the assignment setting this word as used
                ನಮೂದು.ಬಳಸಿದ_ಶಬ್ದ(ಶಬ್ದ);
                ಹಿಂತಿರುಗಿ;
            }
        }

        // If not a known word reference, iterate fromp to simplify/reset tracking
        iterate(nodep->fromp());
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstVarRef* nodep) override {
        ಆದರೆ (!m_funcp) ಹಿಂತಿರುಗಿ;

        Subst_Var_ನಮೂದು& ನಮೂದು = get_ನಮೂದು(nodep);

        // If it's a write, reset tracking as we don't know the assigned value,
        // otherwise we would have picked it up in visit(AstNodeAssign*)
        ಆದರೆ (nodep->access().isWriteOrRW()) {
            ನಮೂದು.ನಿಯೋಜಿಸು_ಸಂಪೂರ್ಣ(ಶೂನ್ಯಸೂಚಕ, ++m_ನಿಯೋಜಿಸು_ಹೆಜ್ಜೆ);
            ಹಿಂತಿರುಗಿ;
        }

        // Otherwise it's a read, substitute it if possible
        UASSERT_OBJ(nodep->access().isReadOnly(), nodep, "Invalid access");
        ಆದರೆ (isSubstitutable(nodep->varp())) {
            ಆದರೆ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ substp = ನಮೂದು.subst_ಸಂಪೂರ್ಣ()) {
                // Do not substitute a compound wide expression.
                // The whole point of adding temporaries is to eliminate them.
                ಆದರೆ (!nodep->isWide() || VN_IS(substp, VarRef)) {
                    ++m_n_ಸಂಪೂರ್ಣ_Substituted;
                    substitute(nodep, substp);
                    ಹಿಂತಿರುಗಿ;
                }
            }
        }

        // If not substituted, mark the assignment setting this variable as used
        ನಮೂದು.ಬಳಸಿದ_ಸಂಪೂರ್ಣ();
    }

    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep) override {
        ಆದರೆ (!m_funcp) ಹಿಂತಿರುಗಿ;

        // Count nodes as we descend to track complexity
        ++m_ops;
        // First iterate children, this can cache child purity
        iterateChildren(nodep);
        // Do not substitute impure expressions
        ಆದರೆ (!nodep->is_ಶುದ್ಧ()) m_ops = SUBST_MAX_OPS_NA;
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstVar*) override {}  // Accelerate
    ಶೂನ್ಯ ಭೇಟಿ(AstConst*) override {}  // Accelerate

    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    ಸ್ಪಷ್ಟ SubstVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~SubstVisitor() override {
        V3Stats::ಕೂಡಿಸು_Stat("Optimizations, Subst, Substituted temps", m_nSubst);
        V3Stats::ಕೂಡಿಸು_Stat("Optimizations, Subst, Constant words reinlined", m_n_Const_ಪದಗಳು_Reinlined);
        V3Stats::ಕೂಡಿಸು_Stat("Optimizations, Subst, Whole variable assignments deleted",
                         m_n_ಸಂಪೂರ್ಣ_ನಿಯೋಜಿಸು_Deleted);
        V3Stats::ಕೂಡಿಸು_Stat("Optimizations, Subst, Whole variables substituted", m_n_ಸಂಪೂರ್ಣ_Substituted);
        V3Stats::ಕೂಡಿಸು_Stat("Optimizations, Subst, Word assignments deleted", m_n_ಶಬ್ದ_ನಿಯೋಜಿಸು_Deleted);
        V3Stats::ಕೂಡಿಸು_Stat("Optimizations, Subst, Words substituted", m_n_ಶಬ್ದ_Substituted);
        UASSERT(m_entries.ಖಾಲಿಗಳು(), "Should not visit outside functions");
    }
};

//######################################################################
// Subst class functions

ಶೂನ್ಯ V3Subst::substituteAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { SubstVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_ವೃಕ್ಷ("subst", 0, dump_ವೃಕ್ಷ_Either_Level() >= 3);
}
