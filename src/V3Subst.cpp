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

ವರ್ಗ SubstVarEntry final {
    friend ವರ್ಗ SubstValidVisitor;

    // SubstVarEntry contains a Record for each word, and one extra for the whole variable
    ಸಂರಚನಾ Record final {
        // MEMBERS
        Ast_ಗಂಟು_Assign* m_assignp;  // Last assignment to this part
        uint32_t m_step;  // Step number of last assignment. 0 means invalid entry.
        ತಾರ್ಕಿಕ m_used;  // True if consumed. Can be set even if entry is invalid.
        // CONSTRUCTOR
        Record() { invalidate(); }
        // METHODS
        ಶೂನ್ಯ invalidate() {
            m_assignp = nullptr;
            m_step = 0;
            m_used = ಅಸತ್ಯ;
        }
    };

    // MEMBERS
    // Variable this SubstVarEntry tracks
    AstVar* ಧ್ರುವ m_varp;
    // The recrod for whole variable tracking
    Record m_wholeRecord;
    // A record for each word in the variable
    std::vector<Record> m_wordRecords{static_cast<size_t>(m_varp->width_ಪದಗಳು()), Record{}};

    // METHDOS
    ಶೂನ್ಯ deleteAssignmentIfUnused(Record& record, size_t& nAssignDeleted) {
        ಯದಿ (!record.m_assignp) ಪ್ರತ್ಯಾಗಮ;
        ಯದಿ (record.m_used) ಪ್ರತ್ಯಾಗಮ;
        ++nAssignDeleted;
        VL_DO_DANGLING(record.m_assignp->unlinkFrBack()->delete_ವೃಕ್ಷ(), record.m_assignp);
    }

    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* substRecord(Record& record);

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    explicit SubstVarEntry(AstVar* varp)
        : m_varp{varp} {}
    ~SubstVarEntry() = ಸ್ವಭಾವ;

    // Record assignment of whole variable. The given 'assp' can be null, which means
    // the variable is known to be assigned, but to an unknown value.
    ಶೂನ್ಯ assignWhole(Ast_ಗಂಟು_Assign* assp, uint32_t step) {
        // Invalidate all word records
        ಪ್ರತಿ (Record& wordRecord : m_wordRecords) wordRecord.invalidate();
        // Set whole record
        m_wholeRecord.m_assignp = assp;
        m_wholeRecord.m_step = step;
        m_wholeRecord.m_used = ಅಸತ್ಯ;
    }

    // Like assignWhole word, but records assignment to a specific word of the variable.
    ಶೂನ್ಯ assignWord(Ast_ಗಂಟು_Assign* assp, uint32_t step, uint32_t word) {
        // Invalidate whole record
        m_wholeRecord.invalidate();
        // Set word record
        Record& wordRecord = m_wordRecords[word];
        wordRecord.m_assignp = assp;
        wordRecord.m_step = step;
        wordRecord.m_used = ಅಸತ್ಯ;
    }

    // Mark the whole variable as used (value consumed)
    ಶೂನ್ಯ usedWhole() {
        m_wholeRecord.m_used = ಸತ್ಯ;
        ಪ್ರತಿ (Record& wordRecord : m_wordRecords) wordRecord.m_used = ಸತ್ಯ;
    }

    // Mark the specific word as used (value consumed)
    ಶೂನ್ಯ usedWord(uint32_t word) {
        m_wholeRecord.m_used = ಸತ್ಯ;
        m_wordRecords[word].m_used = ಸತ್ಯ;
    }

    // Returns substitution of whole word, or nullptr if not known/stale
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* substWhole() { ಪ್ರತ್ಯಾಗಮ substRecord(m_wholeRecord); }
    // Returns substitution of whole word, or nullptr if not known/stale
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* substWord(uint32_t word) { ಪ್ರತ್ಯಾಗಮ substRecord(m_wordRecords[word]); }

    ಶೂನ್ಯ deleteUnusedAssignments(size_t& nWordAssignDeleted, size_t& nWholeAssignDeleted) {
        // Delete assignments to temporaries if they are not used
        ಯದಿ (!m_varp->isStatementTemp()) ಪ್ರತ್ಯಾಗಮ;
        ಪ್ರತಿ (Record& wordRecord : m_wordRecords)
            deleteAssignmentIfUnused(wordRecord, nWordAssignDeleted);
        deleteAssignmentIfUnused(m_wholeRecord, nWholeAssignDeleted);
    }
};

//######################################################################
// See if any variables in the expression we are considering to
// substitute have changed value since we recorded the assignment.

ವರ್ಗ SubstValidVisitor final : ಸಾರ್ವಜನಿಕ VNVisitorConst {
    // NODE STATE
    // See SubstVisitor

    // STATE
    ಧ್ರುವ uint32_t m_step;  // Step number where assignment was recorded
    ತಾರ್ಕಿಕ m_valid = ಸತ್ಯ;  // Is the expression we are considering to substitute valid

    // METHODS
    SubstVarEntry& getEntry(AstVarRef* nodep) {
        // This vistor is always invoked on the RHS of an assignment we are considering to
        // substitute. Variable references must have all been recorded when visiting the
        // assignments RHS, so the SubstVarEntry must exist for each referenced variable.
        ಪ್ರತ್ಯಾಗಮ *nodep->varp()->user1u().to<SubstVarEntry*>();
    }

    // VISITORS
    ಶೂನ್ಯ visit(AstWordSel* nodep) override {
        ಯದಿ (!m_valid) ಪ್ರತ್ಯಾಗಮ;

        ಯದಿ (AstVarRef* ಧ್ರುವ refp = VN_CAST(nodep->fromp(), VarRef)) {
            ಯದಿ (AstConst* ಧ್ರುವ idxp = VN_CAST(nodep->bitp(), Const)) {
                SubstVarEntry& entry = getEntry(refp);
                // If either the whole variable, or the indexed word was written to
                // after the original assignment was recorded, the value is invalid.
                ಯದಿ (m_step < entry.m_wholeRecord.m_step) m_valid = ಅಸತ್ಯ;
                ಯದಿ (m_step < entry.m_wordRecords[idxp->toUInt()].m_step) m_valid = ಅಸತ್ಯ;
                ಪ್ರತ್ಯಾಗಮ;
            }
        }
        iterateChildrenConst(nodep);
    }

    ಶೂನ್ಯ visit(AstVarRef* nodep) override {
        ಯದಿ (!m_valid) ಪ್ರತ್ಯಾಗಮ;

        // If either the whole variable, or any of the words were written to
        // after the original assignment was recorded, the value is invalid.
        ಧ್ರುವ SubstVarEntry& entry = getEntry(nodep);
        ಯದಿ (m_step < entry.m_wholeRecord.m_step) {
            m_valid = ಅಸತ್ಯ;
            ಪ್ರತ್ಯಾಗಮ;
        }
        ಪ್ರತಿ (ಧ್ರುವ SubstVarEntry::Record& wordRecord : entry.m_wordRecords) {
            ಯದಿ (m_step < wordRecord.m_step) {
                m_valid = ಅಸತ್ಯ;
                ಪ್ರತ್ಯಾಗಮ;
            }
        }
    }

    ಶೂನ್ಯ visit(AstConst*) override {}  // Accelerate
    ಶೂನ್ಯ visit(AstText*) override {}  // CExpr/CStmt literal text has no variable dependencies

    ಶೂನ್ಯ visit(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep) override {
        ಯದಿ (!m_valid) ಪ್ರತ್ಯಾಗಮ;
        iterateChildrenConst(nodep);
    }

    ಶೂನ್ಯ visit(Ast_ಗಂಟು* nodep) override { nodep->v3fatalSrc("Non AstNodeExpr under AstNodeExpr"); }

    // CONSTRUCTORS
    explicit SubstValidVisitor(SubstVarEntry::Record& record)
        : m_step{record.m_step} {
        iterateConst(record.m_assignp->rhsp());
    }
    ~SubstValidVisitor() override = ಸ್ವಭಾವ;

ಸಾರ್ವಜನಿಕ:
    ಸ್ಥಿರ ತಾರ್ಕಿಕ valid(SubstVarEntry::Record& record) {
        ಯದಿ (!record.m_assignp) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಪ್ರತ್ಯಾಗಮ SubstValidVisitor{record}.m_valid;
    }
};

Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* SubstVarEntry::substRecord(SubstVarEntry::Record& record) {
    ಯದಿ (!SubstValidVisitor::valid(record)) ಪ್ರತ್ಯಾಗಮ nullptr;
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ rhsp = record.m_assignp->rhsp();
    UDEBUGONLY(UASSERT_OBJ(rhsp->isPure(), record.m_assignp, "Substituting impure expression"););
    ಪ್ರತ್ಯಾಗಮ rhsp;
}

//######################################################################
// Substitution visitor

ವರ್ಗ SubstVisitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
    // NODE STATE - only Under AstCFunc
    // AstVar::user1p -> SubstVarEntry* for assignment tracking. Also used by SubstValidVisitor

    // STATE
    std::deque<SubstVarEntry> m_entries;  // Storage for SubstVarEntry instances
    uint32_t m_ops = 0;  // Number of nodes on the RHS of an assignment
    uint32_t m_assignStep = 0;  // Assignment number to determine variable lifetimes
    ಧ್ರುವ AstCFunc* m_funcp = nullptr;  // Current function we are under
    size_t m_nSubst = 0;  // Number of substitutions performed - for avoiding constant folding
    // Statistics
    size_t m_nWordSubstituted = 0;  // Number of words substituted
    size_t m_nWholeSubstituted = 0;  // Number of whole variables substituted
    size_t m_nWordAssignDeleted = 0;  // Number of word assignments deleted
    size_t m_nWholeAssignDeleted = 0;  // Number of whole variable assignments deleted
    size_t m_n_Const_ಪದಗಳು_Reinlined = 0;  // Number of constant words substituted

    ಸ್ಥಿರ constexpr uint32_t SUBST_MAX_OPS_SUBST = 30;  // Maximum number of ops to substitute in
    ಸ್ಥಿರ constexpr uint32_t SUBST_MAX_OPS_NA = 9999;  // Not allowed to substitute

    // METHODS
    SubstVarEntry& getEntry(AstVarRef* nodep) {
        AstVar* ಧ್ರುವ varp = nodep->varp();
        ಯದಿ (!varp->user1p()) {
            m_entries.emplace_back(varp);
            varp->user1p(&m_entries.back());
        }
        ಪ್ರತ್ಯಾಗಮ *varp->user1u().to<SubstVarEntry*>();
    }

    ಶೂನ್ಯ simplify(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp) {
        // Often constant, so short circuit
        ಯದಿ (VN_IS(exprp, Const)) ಪ್ರತ್ಯಾಗಮ;
        // Iterate expression, then constant fold it if anything changed
        ಧ್ರುವ size_t nSubstBefore = m_nSubst;
        exprp = VN_AS(iterateSubtreeReturnEdits(exprp), ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ);
        ಯದಿ (nSubstBefore != m_nSubst) V3Const::constifyEditCpp(exprp);
    }

    // The way the analysis algorithm works based on statement numbering only works
    // for variables that are written in the same basic block (ignoring AstCond) as
    // where they are consumed. The temporaries introduced in V3Premit are such, so
    // only substitute those for now.
    ಸ್ಥಿರ ತಾರ್ಕಿಕ isSubstitutable(AstVar* nodep) {
        ಪ್ರತ್ಯಾಗಮ nodep->isStatementTemp() && !nodep->noSubst();
    }

    ಶೂನ್ಯ substitute(Ast_ಗಂಟು* nodep, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* substp) {
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newp = substp->backp() ? substp->clone_ವೃಕ್ಷ_Pure(ಸತ್ಯ) : substp;
        ಯದಿ (!nodep->isQuad() && newp->isQuad()) {
            newp = ನೂತನ AstCCast{newp->fileline(), newp, nodep};
        }
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
        ++m_nSubst;
    }

    // VISITORS

    ಶೂನ್ಯ visit(AstNetlist* nodep) override { iterateAndNextNull(nodep->modulesp()); }

    ಶೂನ್ಯ visit(AstCFunc* nodep) override {
        UASSERT_OBJ(!m_funcp, nodep, "Should not nest");
        UASSERT_OBJ(m_entries.empty(), nodep, "Should not visit outside functions");

        // Process the function body
        {
            VL_RESTORER(m_funcp);
            m_funcp = nodep;
            ಧ್ರುವ VNUser1InUse m_inuser1;
            iterateChildren(nodep);
            // Deletes unused assignments and clear entries
            ಪ್ರತಿ (SubstVarEntry& entry : m_entries) {
                entry.deleteUnusedAssignments(m_nWordAssignDeleted, m_nWholeAssignDeleted);
            }
            m_entries.clear();
        }

        // Constant fold here, as Ast size can likely be reduced
        ಯದಿ (v3Global.opt.fConstEager()) V3Const::constifyEditCpp(nodep);
    }

    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Assign* nodep) override {
        ಯದಿ (!m_funcp) ಪ್ರತ್ಯಾಗಮ;

        ಧ್ರುವ uint32_t ops = [&]() {
            VL_RESTORER(m_ops);
            m_ops = 0;
            // Simplify the RHS
            simplify(nodep->rhsp());
            // If the became constant, we can continue substituting it
            ಪ್ರತ್ಯಾಗಮ VN_IS(nodep->rhsp(), Const) ? 0 : m_ops;
        }();

        // Assignment that defines the value, which is either this 'nodep',
        // or nullptr, if the value should not be substituted.
        ಧ್ರುವ auto getAssignp = [&](AstVarRef* refp) -> Ast_ಗಂಟು_Assign* {
            // If too complex, don't substitute
            ಯದಿ (ops > SUBST_MAX_OPS_SUBST) ಪ್ರತ್ಯಾಗಮ nullptr;
            // AstCvtPackedToArray can't be anywhere else than on the RHS of assignment
            ಯದಿ (VN_IS(nodep->rhsp(), CvtPackedToArray)) ಪ್ರತ್ಯಾಗಮ nullptr;
            // If non-const but want const subtitutions only
            ಯದಿ (refp->varp()->substConstOnly() && !VN_IS(nodep->rhsp(), Const)) ಪ್ರತ್ಯಾಗಮ nullptr;
            // Otherwise can substitute based on the assignment
            ಪ್ರತ್ಯಾಗಮ nodep;
        };

        // If LHS is a whole variable reference, track the whole variable
        ಯದಿ (AstVarRef* ಧ್ರುವ refp = VN_CAST(nodep->lhsp(), VarRef)) {
            getEntry(refp).assignWhole(getAssignp(refp), ++m_assignStep);
            ಪ್ರತ್ಯಾಗಮ;
        }

        // If LHS is a known word reference, track the word
        ಯದಿ (ಧ್ರುವ AstWordSel* ಧ್ರುವ selp = VN_CAST(nodep->lhsp(), WordSel)) {
            ಯದಿ (AstVarRef* ಧ್ರುವ refp = VN_CAST(selp->fromp(), VarRef)) {
                // Simplify the index
                simplify(selp->bitp());
                ಯದಿ (ಧ್ರುವ AstConst* ಧ್ರುವ idxp = VN_CAST(selp->bitp(), Const)) {
                    getEntry(refp).assignWord(getAssignp(refp), ++m_assignStep, idxp->toUInt());
                    ಪ್ರತ್ಯಾಗಮ;
                }
            }
        }

        // Not tracked, iterate LHS to simplify/reset tracking
        iterate(nodep->lhsp());
    }

    ಶೂನ್ಯ visit(AstWordSel* nodep) override {
        ಯದಿ (!m_funcp) ಪ್ರತ್ಯಾಗಮ;

        // Simplify the index
        simplify(nodep->bitp());

        // If this is a known word reference, track/substitute it
        ಯದಿ (AstVarRef* ಧ್ರುವ refp = VN_CAST(nodep->fromp(), VarRef)) {
            ಯದಿ (ಧ್ರುವ AstConst* ಧ್ರುವ idxp = VN_CAST(nodep->bitp(), Const)) {
                SubstVarEntry& entry = getEntry(refp);
                ಧ್ರುವ uint32_t word = idxp->toUInt();

                // If it's a write, reset tracking as we don't know the assigned value,
                // otherwise we would have picked it up in visit(AstNodeAssign*)
                ಯದಿ (refp->access().isWriteOrRW()) {
                    entry.assignWord(nullptr, ++m_assignStep, word);
                    ಪ್ರತ್ಯಾಗಮ;
                }

                // Otherwise it's a read,
                UASSERT_OBJ(refp->access().isReadOnly(), nodep, "Invalid access");

                // If it's a constant pool variable, substiute with the constant word
                AstVar* ಧ್ರುವ varp = refp->varp();
                ಯದಿ (varp->constPoolEntry()) {
                    AstConst* ಧ್ರುವ constp = VN_AS(varp->valuep(), Const);
                    ಧ್ರುವ uint32_t ಮೌಲ್ಯ = constp->ಅಂಕೆ().edataWord(word);
                    File_ಸಾಲು* ಧ್ರುವ flp = nodep->fileline();
                    ++m_n_Const_ಪದಗಳು_Reinlined;
                    substitute(nodep, ನೂತನ AstConst{flp, AstConst::SizedEData{}, ಮೌಲ್ಯ});
                    ಪ್ರತ್ಯಾಗಮ;
                }

                // Substitute other variables if possible
                ಯದಿ (isSubstitutable(refp->varp())) {
                    ಯದಿ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ substp = entry.substWord(word)) {
                        ++m_nWordSubstituted;
                        substitute(nodep, substp);
                        ಪ್ರತ್ಯಾಗಮ;
                    }
                }

                // If not substituted, mark the assignment setting this word as used
                entry.usedWord(word);
                ಪ್ರತ್ಯಾಗಮ;
            }
        }

        // If not a known word reference, iterate fromp to simplify/reset tracking
        iterate(nodep->fromp());
    }

    ಶೂನ್ಯ visit(AstVarRef* nodep) override {
        ಯದಿ (!m_funcp) ಪ್ರತ್ಯಾಗಮ;

        SubstVarEntry& entry = getEntry(nodep);

        // If it's a write, reset tracking as we don't know the assigned value,
        // otherwise we would have picked it up in visit(AstNodeAssign*)
        ಯದಿ (nodep->access().isWriteOrRW()) {
            entry.assignWhole(nullptr, ++m_assignStep);
            ಪ್ರತ್ಯಾಗಮ;
        }

        // Otherwise it's a read, substitute it if possible
        UASSERT_OBJ(nodep->access().isReadOnly(), nodep, "Invalid access");
        ಯದಿ (isSubstitutable(nodep->varp())) {
            ಯದಿ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ substp = entry.substWhole()) {
                // Do not substitute a compound wide expression.
                // The whole point of adding temporaries is to eliminate them.
                ಯದಿ (!nodep->isWide() || VN_IS(substp, VarRef)) {
                    ++m_nWholeSubstituted;
                    substitute(nodep, substp);
                    ಪ್ರತ್ಯಾಗಮ;
                }
            }
        }

        // If not substituted, mark the assignment setting this variable as used
        entry.usedWhole();
    }

    ಶೂನ್ಯ visit(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep) override {
        ಯದಿ (!m_funcp) ಪ್ರತ್ಯಾಗಮ;

        // Count nodes as we descend to track complexity
        ++m_ops;
        // First iterate children, this can cache child purity
        iterateChildren(nodep);
        // Do not substitute impure expressions
        ಯದಿ (!nodep->isPure()) m_ops = SUBST_MAX_OPS_NA;
    }

    ಶೂನ್ಯ visit(AstVar*) override {}  // Accelerate
    ಶೂನ್ಯ visit(AstConst*) override {}  // Accelerate

    ಶೂನ್ಯ visit(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    explicit SubstVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~SubstVisitor() override {
        V3Stats::addStat("Optimizations, Subst, Substituted temps", m_nSubst);
        V3Stats::addStat("Optimizations, Subst, Constant words reinlined", m_n_Const_ಪದಗಳು_Reinlined);
        V3Stats::addStat("Optimizations, Subst, Whole variable assignments deleted",
                         m_nWholeAssignDeleted);
        V3Stats::addStat("Optimizations, Subst, Whole variables substituted", m_nWholeSubstituted);
        V3Stats::addStat("Optimizations, Subst, Word assignments deleted", m_nWordAssignDeleted);
        V3Stats::addStat("Optimizations, Subst, Words substituted", m_nWordSubstituted);
        UASSERT(m_entries.empty(), "Should not visit outside functions");
    }
};

//######################################################################
// Subst class functions

ಶೂನ್ಯ V3Subst::substituteAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { SubstVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_ವೃಕ್ಷ("subst", 0, dump_ವೃಕ್ಷ_Either_Level() >= 3);
}
