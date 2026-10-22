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

#అన్తర్భావ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#అన్తర్భావ "V3Subst.h"

#అన్తర్భావ "V3Const.h"
#అన్తర్భావ "V3Stats.h"

#అన్తర్భావ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Data strored for every variable that tracks assignments and usage

తరగతి Subst_Var_నమోదు final {
    మిత్ర తరగతి SubstValidVisitor;

    // SubstVarEntry contains a Record for each word, and one extra for the whole variable
    నిర్మాణం రికార్డు_2 final {
        // MEMBERS
        Ast_కణుపు_కేటాయించు* m_assignp;  // Last assignment to this part
        uint32_t m_అడుగు;  // Step number of last assignment. 0 means invalid entry.
        తార్కిక m_వాడిన;  // True if consumed. Can be set even if entry is invalid.
        // CONSTRUCTOR
        రికార్డు_2() { invalidate(); }
        // METHODS
        శూన్య invalidate() {
            m_assignp = శూన్యసూచిక;
            m_అడుగు = 0;
            m_వాడిన = అబద్ధం;
        }
    };

    // MEMBERS
    // Variable this SubstVarEntry tracks
    AstVar* స్థిరరాశి m_varp;
    // The recrod for whole variable tracking
    రికార్డు_2 m_సంపూర్ణ_రికార్డు;
    // A record for each word in the variable
    std::సదిశ<రికార్డు_2> m_మాట_రికార్డులు{static_cast<కొలత_t>(m_varp->వెడల్పు_పదాలు()), రికార్డు_2{}};

    // METHDOS
    శూన్య delete_Assignment_If_వాడని(రికార్డు_2& రికార్డు, కొలత_t& n_కేటాయించు_Deleted) {
        ఒకవేళ (!రికార్డు.m_assignp) తిరిగి;
        ఒకవేళ (రికార్డు.m_వాడిన) తిరిగి;
        ++n_కేటాయించు_Deleted;
        VL_DO_DANGLING(రికార్డు.m_assignp->unlink_Fr_వెనుక()->delete_వృక్షం(), రికార్డు.m_assignp);
    }

    Ast_కణుపు_సమాసం* subst_రికార్డు(రికార్డు_2& రికార్డు);

బహిరంగ:
    // CONSTRUCTORS
    స్పష్టమైన Subst_Var_నమోదు(AstVar* varp)
        : m_varp{varp} {}
    ~Subst_Var_నమోదు() = అప్రమేయ;

    // Record assignment of whole variable. The given 'assp' can be null, which means
    // the variable is known to be assigned, but to an unknown value.
    శూన్య కేటాయించు_సంపూర్ణ(Ast_కణుపు_కేటాయించు* assp, uint32_t అడుగు) {
        // Invalidate all word records
        కొరకు (రికార్డు_2& మాట_రికార్డు : m_మాట_రికార్డులు) మాట_రికార్డు.invalidate();
        // Set whole record
        m_సంపూర్ణ_రికార్డు.m_assignp = assp;
        m_సంపూర్ణ_రికార్డు.m_అడుగు = అడుగు;
        m_సంపూర్ణ_రికార్డు.m_వాడిన = అబద్ధం;
    }

    // Like assignWhole word, but records assignment to a specific word of the variable.
    శూన్య కేటాయించు_మాట(Ast_కణుపు_కేటాయించు* assp, uint32_t అడుగు, uint32_t మాట) {
        // Invalidate whole record
        m_సంపూర్ణ_రికార్డు.invalidate();
        // Set word record
        రికార్డు_2& మాట_రికార్డు = m_మాట_రికార్డులు[మాట];
        మాట_రికార్డు.m_assignp = assp;
        మాట_రికార్డు.m_అడుగు = అడుగు;
        మాట_రికార్డు.m_వాడిన = అబద్ధం;
    }

    // Mark the whole variable as used (value consumed)
    శూన్య వాడిన_సంపూర్ణ() {
        m_సంపూర్ణ_రికార్డు.m_వాడిన = నిజం;
        కొరకు (రికార్డు_2& మాట_రికార్డు : m_మాట_రికార్డులు) మాట_రికార్డు.m_వాడిన = నిజం;
    }

    // Mark the specific word as used (value consumed)
    శూన్య వాడిన_మాట(uint32_t మాట) {
        m_సంపూర్ణ_రికార్డు.m_వాడిన = నిజం;
        m_మాట_రికార్డులు[మాట].m_వాడిన = నిజం;
    }

    // Returns substitution of whole word, or nullptr if not known/stale
    Ast_కణుపు_సమాసం* subst_సంపూర్ణ() { తిరిగి subst_రికార్డు(m_సంపూర్ణ_రికార్డు); }
    // Returns substitution of whole word, or nullptr if not known/stale
    Ast_కణుపు_సమాసం* subst_మాట(uint32_t మాట) { తిరిగి subst_రికార్డు(m_మాట_రికార్డులు[మాట]); }

    శూన్య delete_వాడని_Assignments(కొలత_t& n_మాట_కేటాయించు_Deleted, కొలత_t& n_సంపూర్ణ_కేటాయించు_Deleted) {
        // Delete assignments to temporaries if they are not used
        ఒకవేళ (!m_varp->is_Statement_తాత్కాలిక()) తిరిగి;
        కొరకు (రికార్డు_2& మాట_రికార్డు : m_మాట_రికార్డులు)
            delete_Assignment_If_వాడని(మాట_రికార్డు, n_మాట_కేటాయించు_Deleted);
        delete_Assignment_If_వాడని(m_సంపూర్ణ_రికార్డు, n_సంపూర్ణ_కేటాయించు_Deleted);
    }
};

//######################################################################
// See if any variables in the expression we are considering to
// substitute have changed value since we recorded the assignment.

తరగతి SubstValidVisitor final : బహిరంగ VNVisitorConst {
    // NODE STATE
    // See SubstVisitor

    // STATE
    స్థిరరాశి uint32_t m_అడుగు;  // Step number where assignment was recorded
    తార్కిక m_valid = నిజం;  // Is the expression we are considering to substitute valid

    // METHODS
    Subst_Var_నమోదు& get_నమోదు(AstVarRef* nodep) {
        // This vistor is always invoked on the RHS of an assignment we are considering to
        // substitute. Variable references must have all been recorded when visiting the
        // assignments RHS, so the SubstVarEntry must exist for each referenced variable.
        తిరిగి *nodep->varp()->user1u().గమ్యం<Subst_Var_నమోదు*>();
    }

    // VISITORS
    శూన్య సందర్శించు(Ast_మాట_Sel* nodep) override {
        ఒకవేళ (!m_valid) తిరిగి;

        ఒకవేళ (AstVarRef* స్థిరరాశి refp = VN_CAST(nodep->fromp(), VarRef)) {
            ఒకవేళ (AstConst* స్థిరరాశి idxp = VN_CAST(nodep->bitp(), Const)) {
                Subst_Var_నమోదు& నమోదు_2 = get_నమోదు(refp);
                // If either the whole variable, or the indexed word was written to
                // after the original assignment was recorded, the value is invalid.
                ఒకవేళ (m_అడుగు < నమోదు_2.m_సంపూర్ణ_రికార్డు.m_అడుగు) m_valid = అబద్ధం;
                ఒకవేళ (m_అడుగు < నమోదు_2.m_మాట_రికార్డులు[idxp->గమ్యం_UInt()].m_అడుగు) m_valid = అబద్ధం;
                తిరిగి;
            }
        }
        iterateChildrenConst(nodep);
    }

    శూన్య సందర్శించు(AstVarRef* nodep) override {
        ఒకవేళ (!m_valid) తిరిగి;

        // If either the whole variable, or any of the words were written to
        // after the original assignment was recorded, the value is invalid.
        స్థిరరాశి Subst_Var_నమోదు& నమోదు_2 = get_నమోదు(nodep);
        ఒకవేళ (m_అడుగు < నమోదు_2.m_సంపూర్ణ_రికార్డు.m_అడుగు) {
            m_valid = అబద్ధం;
            తిరిగి;
        }
        కొరకు (స్థిరరాశి Subst_Var_నమోదు::రికార్డు_2& మాట_రికార్డు : నమోదు_2.m_మాట_రికార్డులు) {
            ఒకవేళ (m_అడుగు < మాట_రికార్డు.m_అడుగు) {
                m_valid = అబద్ధం;
                తిరిగి;
            }
        }
    }

    శూన్య సందర్శించు(AstConst*) override {}  // Accelerate
    శూన్య సందర్శించు(Ast_పాఠ్యం*) override {}  // CExpr/CStmt literal text has no variable dependencies

    శూన్య సందర్శించు(Ast_కణుపు_సమాసం* nodep) override {
        ఒకవేళ (!m_valid) తిరిగి;
        iterateChildrenConst(nodep);
    }

    శూన్య సందర్శించు(Ast_కణుపు* nodep) override { nodep->v3fatalSrc("Non AstNodeExpr under AstNodeExpr"); }

    // CONSTRUCTORS
    స్పష్టమైన SubstValidVisitor(Subst_Var_నమోదు::రికార్డు_2& రికార్డు)
        : m_అడుగు{రికార్డు.m_అడుగు} {
        iterateConst(రికార్డు.m_assignp->rhsp());
    }
    ~SubstValidVisitor() override = అప్రమేయ;

బహిరంగ:
    స్థిర తార్కిక valid(Subst_Var_నమోదు::రికార్డు_2& రికార్డు) {
        ఒకవేళ (!రికార్డు.m_assignp) తిరిగి అబద్ధం;
        తిరిగి SubstValidVisitor{రికార్డు}.m_valid;
    }
};

Ast_కణుపు_సమాసం* Subst_Var_నమోదు::subst_రికార్డు(Subst_Var_నమోదు::రికార్డు_2& రికార్డు) {
    ఒకవేళ (!SubstValidVisitor::valid(రికార్డు)) తిరిగి శూన్యసూచిక;
    Ast_కణుపు_సమాసం* స్థిరరాశి rhsp = రికార్డు.m_assignp->rhsp();
    UDEBUGONLY(UASSERT_OBJ(rhsp->is_శుద్ధ(), రికార్డు.m_assignp, "Substituting impure expression"););
    తిరిగి rhsp;
}

//######################################################################
// Substitution visitor

తరగతి SubstVisitor final : బహిరంగ VNVisitor {
    // NODE STATE - only Under AstCFunc
    // AstVar::user1p -> SubstVarEntry* for assignment tracking. Also used by SubstValidVisitor

    // STATE
    std::deque<Subst_Var_నమోదు> m_entries;  // Storage for SubstVarEntry instances
    uint32_t m_ops = 0;  // Number of nodes on the RHS of an assignment
    uint32_t m_కేటాయించు_అడుగు = 0;  // Assignment number to determine variable lifetimes
    స్థిరరాశి AstCFunc* m_funcp = శూన్యసూచిక;  // Current function we are under
    కొలత_t m_nSubst = 0;  // Number of substitutions performed - for avoiding constant folding
    // Statistics
    కొలత_t m_n_మాట_Substituted = 0;  // Number of words substituted
    కొలత_t m_n_సంపూర్ణ_Substituted = 0;  // Number of whole variables substituted
    కొలత_t m_n_మాట_కేటాయించు_Deleted = 0;  // Number of word assignments deleted
    కొలత_t m_n_సంపూర్ణ_కేటాయించు_Deleted = 0;  // Number of whole variable assignments deleted
    కొలత_t m_n_Const_పదాలు_Reinlined = 0;  // Number of constant words substituted

    స్థిర constexpr uint32_t SUBST_MAX_OPS_SUBST = 30;  // Maximum number of ops to substitute in
    స్థిర constexpr uint32_t SUBST_MAX_OPS_NA = 9999;  // Not allowed to substitute

    // METHODS
    Subst_Var_నమోదు& get_నమోదు(AstVarRef* nodep) {
        AstVar* స్థిరరాశి varp = nodep->varp();
        ఒకవేళ (!varp->user1p()) {
            m_entries.emplace_వెనుక(varp);
            varp->user1p(&m_entries.వెనుక());
        }
        తిరిగి *varp->user1u().గమ్యం<Subst_Var_నమోదు*>();
    }

    శూన్య simplify(Ast_కణుపు_సమాసం* exprp) {
        // Often constant, so short circuit
        ఒకవేళ (VN_IS(exprp, Const)) తిరిగి;
        // Iterate expression, then constant fold it if anything changed
        స్థిరరాశి కొలత_t nSubstBefore = m_nSubst;
        exprp = VN_AS(iterateSubtreeReturnEdits(exprp), కణుపు_సమాసం);
        ఒకవేళ (nSubstBefore != m_nSubst) V3Const::constifyEditCpp(exprp);
    }

    // The way the analysis algorithm works based on statement numbering only works
    // for variables that are written in the same basic block (ignoring AstCond) as
    // where they are consumed. The temporaries introduced in V3Premit are such, so
    // only substitute those for now.
    స్థిర తార్కిక isSubstitutable(AstVar* nodep) {
        తిరిగి nodep->is_Statement_తాత్కాలిక() && !nodep->noSubst();
    }

    శూన్య substitute(Ast_కణుపు* nodep, Ast_కణుపు_సమాసం* substp) {
        Ast_కణుపు_సమాసం* newp = substp->backp() ? substp->clone_వృక్షం_శుద్ధ(నిజం) : substp;
        ఒకవేళ (!nodep->isQuad() && newp->isQuad()) {
            newp = కొత్త AstCCast{newp->fileline(), newp, nodep};
        }
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_వృక్షం(), nodep);
        ++m_nSubst;
    }

    // VISITORS

    శూన్య సందర్శించు(AstNetlist* nodep) override { iterate_And_తదుపరి_Null(nodep->modulesp()); }

    శూన్య సందర్శించు(AstCFunc* nodep) override {
        UASSERT_OBJ(!m_funcp, nodep, "Should not nest");
        UASSERT_OBJ(m_entries.ఖాళీలు(), nodep, "Should not visit outside functions");

        // Process the function body
        {
            VL_RESTORER(m_funcp);
            m_funcp = nodep;
            స్థిరరాశి VNUser1_లోపల_Use m_inuser1;
            iterateChildren(nodep);
            // Deletes unused assignments and clear entries
            కొరకు (Subst_Var_నమోదు& నమోదు_2 : m_entries) {
                నమోదు_2.delete_వాడని_Assignments(m_n_మాట_కేటాయించు_Deleted, m_n_సంపూర్ణ_కేటాయించు_Deleted);
            }
            m_entries.clear();
        }

        // Constant fold here, as Ast size can likely be reduced
        ఒకవేళ (v3Global.opt.fConstEager()) V3Const::constifyEditCpp(nodep);
    }

    శూన్య సందర్శించు(Ast_కణుపు_కేటాయించు* nodep) override {
        ఒకవేళ (!m_funcp) తిరిగి;

        స్థిరరాశి uint32_t ops = [&]() {
            VL_RESTORER(m_ops);
            m_ops = 0;
            // Simplify the RHS
            simplify(nodep->rhsp());
            // If the became constant, we can continue substituting it
            తిరిగి VN_IS(nodep->rhsp(), Const) ? 0 : m_ops;
        }();

        // Assignment that defines the value, which is either this 'nodep',
        // or nullptr, if the value should not be substituted.
        స్థిరరాశి స్వయంచాలక getAssignp = [&](AstVarRef* refp) -> Ast_కణుపు_కేటాయించు* {
            // If too complex, don't substitute
            ఒకవేళ (ops > SUBST_MAX_OPS_SUBST) తిరిగి శూన్యసూచిక;
            // AstCvtPackedToArray can't be anywhere else than on the RHS of assignment
            ఒకవేళ (VN_IS(nodep->rhsp(), Cvt_Packed_గమ్యం_Array)) తిరిగి శూన్యసూచిక;
            // If non-const but want const subtitutions only
            ఒకవేళ (refp->varp()->substConstOnly() && !VN_IS(nodep->rhsp(), Const)) తిరిగి శూన్యసూచిక;
            // Otherwise can substitute based on the assignment
            తిరిగి nodep;
        };

        // If LHS is a whole variable reference, track the whole variable
        ఒకవేళ (AstVarRef* స్థిరరాశి refp = VN_CAST(nodep->lhsp(), VarRef)) {
            get_నమోదు(refp).కేటాయించు_సంపూర్ణ(getAssignp(refp), ++m_కేటాయించు_అడుగు);
            తిరిగి;
        }

        // If LHS is a known word reference, track the word
        ఒకవేళ (స్థిరరాశి Ast_మాట_Sel* స్థిరరాశి selp = VN_CAST(nodep->lhsp(), మాట_Sel)) {
            ఒకవేళ (AstVarRef* స్థిరరాశి refp = VN_CAST(selp->fromp(), VarRef)) {
                // Simplify the index
                simplify(selp->bitp());
                ఒకవేళ (స్థిరరాశి AstConst* స్థిరరాశి idxp = VN_CAST(selp->bitp(), Const)) {
                    get_నమోదు(refp).కేటాయించు_మాట(getAssignp(refp), ++m_కేటాయించు_అడుగు, idxp->గమ్యం_UInt());
                    తిరిగి;
                }
            }
        }

        // Not tracked, iterate LHS to simplify/reset tracking
        iterate(nodep->lhsp());
    }

    శూన్య సందర్శించు(Ast_మాట_Sel* nodep) override {
        ఒకవేళ (!m_funcp) తిరిగి;

        // Simplify the index
        simplify(nodep->bitp());

        // If this is a known word reference, track/substitute it
        ఒకవేళ (AstVarRef* స్థిరరాశి refp = VN_CAST(nodep->fromp(), VarRef)) {
            ఒకవేళ (స్థిరరాశి AstConst* స్థిరరాశి idxp = VN_CAST(nodep->bitp(), Const)) {
                Subst_Var_నమోదు& నమోదు_2 = get_నమోదు(refp);
                స్థిరరాశి uint32_t మాట = idxp->గమ్యం_UInt();

                // If it's a write, reset tracking as we don't know the assigned value,
                // otherwise we would have picked it up in visit(AstNodeAssign*)
                ఒకవేళ (refp->access().isWriteOrRW()) {
                    నమోదు_2.కేటాయించు_మాట(శూన్యసూచిక, ++m_కేటాయించు_అడుగు, మాట);
                    తిరిగి;
                }

                // Otherwise it's a read,
                UASSERT_OBJ(refp->access().isReadOnly(), nodep, "Invalid access");

                // If it's a constant pool variable, substiute with the constant word
                AstVar* స్థిరరాశి varp = refp->varp();
                ఒకవేళ (varp->const_Pool_నమోదు()) {
                    AstConst* స్థిరరాశి constp = VN_AS(varp->valuep(), Const);
                    స్థిరరాశి uint32_t విలువ = constp->అంకె().edata_మాట(మాట);
                    File_పంక్తి* స్థిరరాశి flp = nodep->fileline();
                    ++m_n_Const_పదాలు_Reinlined;
                    substitute(nodep, కొత్త AstConst{flp, AstConst::SizedEData{}, విలువ});
                    తిరిగి;
                }

                // Substitute other variables if possible
                ఒకవేళ (isSubstitutable(refp->varp())) {
                    ఒకవేళ (Ast_కణుపు_సమాసం* స్థిరరాశి substp = నమోదు_2.subst_మాట(మాట)) {
                        ++m_n_మాట_Substituted;
                        substitute(nodep, substp);
                        తిరిగి;
                    }
                }

                // If not substituted, mark the assignment setting this word as used
                నమోదు_2.వాడిన_మాట(మాట);
                తిరిగి;
            }
        }

        // If not a known word reference, iterate fromp to simplify/reset tracking
        iterate(nodep->fromp());
    }

    శూన్య సందర్శించు(AstVarRef* nodep) override {
        ఒకవేళ (!m_funcp) తిరిగి;

        Subst_Var_నమోదు& నమోదు_2 = get_నమోదు(nodep);

        // If it's a write, reset tracking as we don't know the assigned value,
        // otherwise we would have picked it up in visit(AstNodeAssign*)
        ఒకవేళ (nodep->access().isWriteOrRW()) {
            నమోదు_2.కేటాయించు_సంపూర్ణ(శూన్యసూచిక, ++m_కేటాయించు_అడుగు);
            తిరిగి;
        }

        // Otherwise it's a read, substitute it if possible
        UASSERT_OBJ(nodep->access().isReadOnly(), nodep, "Invalid access");
        ఒకవేళ (isSubstitutable(nodep->varp())) {
            ఒకవేళ (Ast_కణుపు_సమాసం* స్థిరరాశి substp = నమోదు_2.subst_సంపూర్ణ()) {
                // Do not substitute a compound wide expression.
                // The whole point of adding temporaries is to eliminate them.
                ఒకవేళ (!nodep->isWide() || VN_IS(substp, VarRef)) {
                    ++m_n_సంపూర్ణ_Substituted;
                    substitute(nodep, substp);
                    తిరిగి;
                }
            }
        }

        // If not substituted, mark the assignment setting this variable as used
        నమోదు_2.వాడిన_సంపూర్ణ();
    }

    శూన్య సందర్శించు(Ast_కణుపు_సమాసం* nodep) override {
        ఒకవేళ (!m_funcp) తిరిగి;

        // Count nodes as we descend to track complexity
        ++m_ops;
        // First iterate children, this can cache child purity
        iterateChildren(nodep);
        // Do not substitute impure expressions
        ఒకవేళ (!nodep->is_శుద్ధ()) m_ops = SUBST_MAX_OPS_NA;
    }

    శూన్య సందర్శించు(AstVar*) override {}  // Accelerate
    శూన్య సందర్శించు(AstConst*) override {}  // Accelerate

    శూన్య సందర్శించు(Ast_కణుపు* nodep) override { iterateChildren(nodep); }

బహిరంగ:
    // CONSTRUCTORS
    స్పష్టమైన SubstVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~SubstVisitor() override {
        V3Stats::కూడు_Stat("Optimizations, Subst, Substituted temps", m_nSubst);
        V3Stats::కూడు_Stat("Optimizations, Subst, Constant words reinlined", m_n_Const_పదాలు_Reinlined);
        V3Stats::కూడు_Stat("Optimizations, Subst, Whole variable assignments deleted",
                         m_n_సంపూర్ణ_కేటాయించు_Deleted);
        V3Stats::కూడు_Stat("Optimizations, Subst, Whole variables substituted", m_n_సంపూర్ణ_Substituted);
        V3Stats::కూడు_Stat("Optimizations, Subst, Word assignments deleted", m_n_మాట_కేటాయించు_Deleted);
        V3Stats::కూడు_Stat("Optimizations, Subst, Words substituted", m_n_మాట_Substituted);
        UASSERT(m_entries.ఖాళీలు(), "Should not visit outside functions");
    }
};

//######################################################################
// Subst class functions

శూన్య V3Subst::substituteAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { SubstVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_వృక్షం("subst", 0, dump_వృక్షం_Either_Level() >= 3);
}
