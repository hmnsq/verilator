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

#અન્તર્ભાવ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#અન્તર્ભાવ "V3Subst.h"

#અન્તર્ભાવ "V3Const.h"
#અન્તર્ભાવ "V3Stats.h"

#અન્તર્ભાવ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Data strored for every variable that tracks assignments and usage

વર્ગ SubstVarEntry final {
    friend વર્ગ SubstValidVisitor;

    // SubstVarEntry contains a Record for each word, and one extra for the whole variable
    સંરચના Record final {
        // MEMBERS
        Ast_ગાંઠ_Assign* m_assignp;  // Last assignment to this part
        uint32_t m_step;  // Step number of last assignment. 0 means invalid entry.
        તાર્કિક m_used;  // True if consumed. Can be set even if entry is invalid.
        // CONSTRUCTOR
        Record() { invalidate(); }
        // METHODS
        શૂન્ય invalidate() {
            m_assignp = nullptr;
            m_step = 0;
            m_used = ખોટું;
        }
    };

    // MEMBERS
    // Variable this SubstVarEntry tracks
    AstVar* અચલ m_varp;
    // The recrod for whole variable tracking
    Record m_wholeRecord;
    // A record for each word in the variable
    std::vector<Record> m_wordRecords{static_cast<size_t>(m_varp->width_શબ્દો()), Record{}};

    // METHDOS
    શૂન્ય deleteAssignmentIfUnused(Record& record, size_t& nAssignDeleted) {
        જો (!record.m_assignp) પાછા;
        જો (record.m_used) પાછા;
        ++nAssignDeleted;
        VL_DO_DANGLING(record.m_assignp->unlinkFrBack()->delete_વૃક્ષ(), record.m_assignp);
    }

    Ast_ગાંઠ_પદાવલિ* substRecord(Record& record);

જાહેર:
    // CONSTRUCTORS
    explicit SubstVarEntry(AstVar* varp)
        : m_varp{varp} {}
    ~SubstVarEntry() = મૂળભૂત;

    // Record assignment of whole variable. The given 'assp' can be null, which means
    // the variable is known to be assigned, but to an unknown value.
    શૂન્ય assignWhole(Ast_ગાંઠ_Assign* assp, uint32_t step) {
        // Invalidate all word records
        માટે (Record& wordRecord : m_wordRecords) wordRecord.invalidate();
        // Set whole record
        m_wholeRecord.m_assignp = assp;
        m_wholeRecord.m_step = step;
        m_wholeRecord.m_used = ખોટું;
    }

    // Like assignWhole word, but records assignment to a specific word of the variable.
    શૂન્ય assignWord(Ast_ગાંઠ_Assign* assp, uint32_t step, uint32_t word) {
        // Invalidate whole record
        m_wholeRecord.invalidate();
        // Set word record
        Record& wordRecord = m_wordRecords[word];
        wordRecord.m_assignp = assp;
        wordRecord.m_step = step;
        wordRecord.m_used = ખોટું;
    }

    // Mark the whole variable as used (value consumed)
    શૂન્ય usedWhole() {
        m_wholeRecord.m_used = સાચું;
        માટે (Record& wordRecord : m_wordRecords) wordRecord.m_used = સાચું;
    }

    // Mark the specific word as used (value consumed)
    શૂન્ય usedWord(uint32_t word) {
        m_wholeRecord.m_used = સાચું;
        m_wordRecords[word].m_used = સાચું;
    }

    // Returns substitution of whole word, or nullptr if not known/stale
    Ast_ગાંઠ_પદાવલિ* substWhole() { પાછા substRecord(m_wholeRecord); }
    // Returns substitution of whole word, or nullptr if not known/stale
    Ast_ગાંઠ_પદાવલિ* substWord(uint32_t word) { પાછા substRecord(m_wordRecords[word]); }

    શૂન્ય deleteUnusedAssignments(size_t& nWordAssignDeleted, size_t& nWholeAssignDeleted) {
        // Delete assignments to temporaries if they are not used
        જો (!m_varp->isStatementTemp()) પાછા;
        માટે (Record& wordRecord : m_wordRecords)
            deleteAssignmentIfUnused(wordRecord, nWordAssignDeleted);
        deleteAssignmentIfUnused(m_wholeRecord, nWholeAssignDeleted);
    }
};

//######################################################################
// See if any variables in the expression we are considering to
// substitute have changed value since we recorded the assignment.

વર્ગ SubstValidVisitor final : જાહેર VNVisitorConst {
    // NODE STATE
    // See SubstVisitor

    // STATE
    અચલ uint32_t m_step;  // Step number where assignment was recorded
    તાર્કિક m_valid = સાચું;  // Is the expression we are considering to substitute valid

    // METHODS
    SubstVarEntry& getEntry(AstVarRef* nodep) {
        // This vistor is always invoked on the RHS of an assignment we are considering to
        // substitute. Variable references must have all been recorded when visiting the
        // assignments RHS, so the SubstVarEntry must exist for each referenced variable.
        પાછા *nodep->varp()->user1u().to<SubstVarEntry*>();
    }

    // VISITORS
    શૂન્ય visit(AstWordSel* nodep) override {
        જો (!m_valid) પાછા;

        જો (AstVarRef* અચલ refp = VN_CAST(nodep->fromp(), VarRef)) {
            જો (AstConst* અચલ idxp = VN_CAST(nodep->bitp(), Const)) {
                SubstVarEntry& entry = getEntry(refp);
                // If either the whole variable, or the indexed word was written to
                // after the original assignment was recorded, the value is invalid.
                જો (m_step < entry.m_wholeRecord.m_step) m_valid = ખોટું;
                જો (m_step < entry.m_wordRecords[idxp->toUInt()].m_step) m_valid = ખોટું;
                પાછા;
            }
        }
        iterateChildrenConst(nodep);
    }

    શૂન્ય visit(AstVarRef* nodep) override {
        જો (!m_valid) પાછા;

        // If either the whole variable, or any of the words were written to
        // after the original assignment was recorded, the value is invalid.
        અચલ SubstVarEntry& entry = getEntry(nodep);
        જો (m_step < entry.m_wholeRecord.m_step) {
            m_valid = ખોટું;
            પાછા;
        }
        માટે (અચલ SubstVarEntry::Record& wordRecord : entry.m_wordRecords) {
            જો (m_step < wordRecord.m_step) {
                m_valid = ખોટું;
                પાછા;
            }
        }
    }

    શૂન્ય visit(AstConst*) override {}  // Accelerate
    શૂન્ય visit(AstText*) override {}  // CExpr/CStmt literal text has no variable dependencies

    શૂન્ય visit(Ast_ગાંઠ_પદાવલિ* nodep) override {
        જો (!m_valid) પાછા;
        iterateChildrenConst(nodep);
    }

    શૂન્ય visit(Ast_ગાંઠ* nodep) override { nodep->v3fatalSrc("Non AstNodeExpr under AstNodeExpr"); }

    // CONSTRUCTORS
    explicit SubstValidVisitor(SubstVarEntry::Record& record)
        : m_step{record.m_step} {
        iterateConst(record.m_assignp->rhsp());
    }
    ~SubstValidVisitor() override = મૂળભૂત;

જાહેર:
    સ્થિર તાર્કિક valid(SubstVarEntry::Record& record) {
        જો (!record.m_assignp) પાછા ખોટું;
        પાછા SubstValidVisitor{record}.m_valid;
    }
};

Ast_ગાંઠ_પદાવલિ* SubstVarEntry::substRecord(SubstVarEntry::Record& record) {
    જો (!SubstValidVisitor::valid(record)) પાછા nullptr;
    Ast_ગાંઠ_પદાવલિ* અચલ rhsp = record.m_assignp->rhsp();
    UDEBUGONLY(UASSERT_OBJ(rhsp->isPure(), record.m_assignp, "Substituting impure expression"););
    પાછા rhsp;
}

//######################################################################
// Substitution visitor

વર્ગ SubstVisitor final : જાહેર VNVisitor {
    // NODE STATE - only Under AstCFunc
    // AstVar::user1p -> SubstVarEntry* for assignment tracking. Also used by SubstValidVisitor

    // STATE
    std::deque<SubstVarEntry> m_entries;  // Storage for SubstVarEntry instances
    uint32_t m_ops = 0;  // Number of nodes on the RHS of an assignment
    uint32_t m_assignStep = 0;  // Assignment number to determine variable lifetimes
    અચલ AstCFunc* m_funcp = nullptr;  // Current function we are under
    size_t m_nSubst = 0;  // Number of substitutions performed - for avoiding constant folding
    // Statistics
    size_t m_nWordSubstituted = 0;  // Number of words substituted
    size_t m_nWholeSubstituted = 0;  // Number of whole variables substituted
    size_t m_nWordAssignDeleted = 0;  // Number of word assignments deleted
    size_t m_nWholeAssignDeleted = 0;  // Number of whole variable assignments deleted
    size_t m_n_Const_શબ્દો_Reinlined = 0;  // Number of constant words substituted

    સ્થિર constexpr uint32_t SUBST_MAX_OPS_SUBST = 30;  // Maximum number of ops to substitute in
    સ્થિર constexpr uint32_t SUBST_MAX_OPS_NA = 9999;  // Not allowed to substitute

    // METHODS
    SubstVarEntry& getEntry(AstVarRef* nodep) {
        AstVar* અચલ varp = nodep->varp();
        જો (!varp->user1p()) {
            m_entries.emplace_back(varp);
            varp->user1p(&m_entries.back());
        }
        પાછા *varp->user1u().to<SubstVarEntry*>();
    }

    શૂન્ય simplify(Ast_ગાંઠ_પદાવલિ* exprp) {
        // Often constant, so short circuit
        જો (VN_IS(exprp, Const)) પાછા;
        // Iterate expression, then constant fold it if anything changed
        અચલ size_t nSubstBefore = m_nSubst;
        exprp = VN_AS(iterateSubtreeReturnEdits(exprp), ગાંઠ_પદાવલિ);
        જો (nSubstBefore != m_nSubst) V3Const::constifyEditCpp(exprp);
    }

    // The way the analysis algorithm works based on statement numbering only works
    // for variables that are written in the same basic block (ignoring AstCond) as
    // where they are consumed. The temporaries introduced in V3Premit are such, so
    // only substitute those for now.
    સ્થિર તાર્કિક isSubstitutable(AstVar* nodep) {
        પાછા nodep->isStatementTemp() && !nodep->noSubst();
    }

    શૂન્ય substitute(Ast_ગાંઠ* nodep, Ast_ગાંઠ_પદાવલિ* substp) {
        Ast_ગાંઠ_પદાવલિ* newp = substp->backp() ? substp->clone_વૃક્ષ_Pure(સાચું) : substp;
        જો (!nodep->isQuad() && newp->isQuad()) {
            newp = નવું AstCCast{newp->fileline(), newp, nodep};
        }
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_વૃક્ષ(), nodep);
        ++m_nSubst;
    }

    // VISITORS

    શૂન્ય visit(AstNetlist* nodep) override { iterateAndNextNull(nodep->modulesp()); }

    શૂન્ય visit(AstCFunc* nodep) override {
        UASSERT_OBJ(!m_funcp, nodep, "Should not nest");
        UASSERT_OBJ(m_entries.empty(), nodep, "Should not visit outside functions");

        // Process the function body
        {
            VL_RESTORER(m_funcp);
            m_funcp = nodep;
            અચલ VNUser1InUse m_inuser1;
            iterateChildren(nodep);
            // Deletes unused assignments and clear entries
            માટે (SubstVarEntry& entry : m_entries) {
                entry.deleteUnusedAssignments(m_nWordAssignDeleted, m_nWholeAssignDeleted);
            }
            m_entries.clear();
        }

        // Constant fold here, as Ast size can likely be reduced
        જો (v3Global.opt.fConstEager()) V3Const::constifyEditCpp(nodep);
    }

    શૂન્ય visit(Ast_ગાંઠ_Assign* nodep) override {
        જો (!m_funcp) પાછા;

        અચલ uint32_t ops = [&]() {
            VL_RESTORER(m_ops);
            m_ops = 0;
            // Simplify the RHS
            simplify(nodep->rhsp());
            // If the became constant, we can continue substituting it
            પાછા VN_IS(nodep->rhsp(), Const) ? 0 : m_ops;
        }();

        // Assignment that defines the value, which is either this 'nodep',
        // or nullptr, if the value should not be substituted.
        અચલ auto getAssignp = [&](AstVarRef* refp) -> Ast_ગાંઠ_Assign* {
            // If too complex, don't substitute
            જો (ops > SUBST_MAX_OPS_SUBST) પાછા nullptr;
            // AstCvtPackedToArray can't be anywhere else than on the RHS of assignment
            જો (VN_IS(nodep->rhsp(), CvtPackedToArray)) પાછા nullptr;
            // If non-const but want const subtitutions only
            જો (refp->varp()->substConstOnly() && !VN_IS(nodep->rhsp(), Const)) પાછા nullptr;
            // Otherwise can substitute based on the assignment
            પાછા nodep;
        };

        // If LHS is a whole variable reference, track the whole variable
        જો (AstVarRef* અચલ refp = VN_CAST(nodep->lhsp(), VarRef)) {
            getEntry(refp).assignWhole(getAssignp(refp), ++m_assignStep);
            પાછા;
        }

        // If LHS is a known word reference, track the word
        જો (અચલ AstWordSel* અચલ selp = VN_CAST(nodep->lhsp(), WordSel)) {
            જો (AstVarRef* અચલ refp = VN_CAST(selp->fromp(), VarRef)) {
                // Simplify the index
                simplify(selp->bitp());
                જો (અચલ AstConst* અચલ idxp = VN_CAST(selp->bitp(), Const)) {
                    getEntry(refp).assignWord(getAssignp(refp), ++m_assignStep, idxp->toUInt());
                    પાછા;
                }
            }
        }

        // Not tracked, iterate LHS to simplify/reset tracking
        iterate(nodep->lhsp());
    }

    શૂન્ય visit(AstWordSel* nodep) override {
        જો (!m_funcp) પાછા;

        // Simplify the index
        simplify(nodep->bitp());

        // If this is a known word reference, track/substitute it
        જો (AstVarRef* અચલ refp = VN_CAST(nodep->fromp(), VarRef)) {
            જો (અચલ AstConst* અચલ idxp = VN_CAST(nodep->bitp(), Const)) {
                SubstVarEntry& entry = getEntry(refp);
                અચલ uint32_t word = idxp->toUInt();

                // If it's a write, reset tracking as we don't know the assigned value,
                // otherwise we would have picked it up in visit(AstNodeAssign*)
                જો (refp->access().isWriteOrRW()) {
                    entry.assignWord(nullptr, ++m_assignStep, word);
                    પાછા;
                }

                // Otherwise it's a read,
                UASSERT_OBJ(refp->access().isReadOnly(), nodep, "Invalid access");

                // If it's a constant pool variable, substiute with the constant word
                AstVar* અચલ varp = refp->varp();
                જો (varp->constPoolEntry()) {
                    AstConst* અચલ constp = VN_AS(varp->valuep(), Const);
                    અચલ uint32_t કિંમત = constp->અંક().edataWord(word);
                    File_લીટી* અચલ flp = nodep->fileline();
                    ++m_n_Const_શબ્દો_Reinlined;
                    substitute(nodep, નવું AstConst{flp, AstConst::SizedEData{}, કિંમત});
                    પાછા;
                }

                // Substitute other variables if possible
                જો (isSubstitutable(refp->varp())) {
                    જો (Ast_ગાંઠ_પદાવલિ* અચલ substp = entry.substWord(word)) {
                        ++m_nWordSubstituted;
                        substitute(nodep, substp);
                        પાછા;
                    }
                }

                // If not substituted, mark the assignment setting this word as used
                entry.usedWord(word);
                પાછા;
            }
        }

        // If not a known word reference, iterate fromp to simplify/reset tracking
        iterate(nodep->fromp());
    }

    શૂન્ય visit(AstVarRef* nodep) override {
        જો (!m_funcp) પાછા;

        SubstVarEntry& entry = getEntry(nodep);

        // If it's a write, reset tracking as we don't know the assigned value,
        // otherwise we would have picked it up in visit(AstNodeAssign*)
        જો (nodep->access().isWriteOrRW()) {
            entry.assignWhole(nullptr, ++m_assignStep);
            પાછા;
        }

        // Otherwise it's a read, substitute it if possible
        UASSERT_OBJ(nodep->access().isReadOnly(), nodep, "Invalid access");
        જો (isSubstitutable(nodep->varp())) {
            જો (Ast_ગાંઠ_પદાવલિ* અચલ substp = entry.substWhole()) {
                // Do not substitute a compound wide expression.
                // The whole point of adding temporaries is to eliminate them.
                જો (!nodep->isWide() || VN_IS(substp, VarRef)) {
                    ++m_nWholeSubstituted;
                    substitute(nodep, substp);
                    પાછા;
                }
            }
        }

        // If not substituted, mark the assignment setting this variable as used
        entry.usedWhole();
    }

    શૂન્ય visit(Ast_ગાંઠ_પદાવલિ* nodep) override {
        જો (!m_funcp) પાછા;

        // Count nodes as we descend to track complexity
        ++m_ops;
        // First iterate children, this can cache child purity
        iterateChildren(nodep);
        // Do not substitute impure expressions
        જો (!nodep->isPure()) m_ops = SUBST_MAX_OPS_NA;
    }

    શૂન્ય visit(AstVar*) override {}  // Accelerate
    શૂન્ય visit(AstConst*) override {}  // Accelerate

    શૂન્ય visit(Ast_ગાંઠ* nodep) override { iterateChildren(nodep); }

જાહેર:
    // CONSTRUCTORS
    explicit SubstVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~SubstVisitor() override {
        V3Stats::addStat("Optimizations, Subst, Substituted temps", m_nSubst);
        V3Stats::addStat("Optimizations, Subst, Constant words reinlined", m_n_Const_શબ્દો_Reinlined);
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

શૂન્ય V3Subst::substituteAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { SubstVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_વૃક્ષ("subst", 0, dump_વૃક્ષ_Either_Level() >= 3);
}
