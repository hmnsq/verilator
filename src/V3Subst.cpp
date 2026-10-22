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

#অন্তর্ভাব "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#অন্তর্ভাব "V3Subst.h"

#অন্তর্ভাব "V3Const.h"
#অন্তর্ভাব "V3Stats.h"

#অন্তর্ভাব <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Data strored for every variable that tracks assignments and usage

শ্রেণী SubstVarEntry final {
    friend শ্রেণী SubstValidVisitor;

    // SubstVarEntry contains a Record for each word, and one extra for the whole variable
    কাঠামো Record final {
        // MEMBERS
        Ast_গ্রন্থি_Assign* m_assignp;  // Last assignment to this part
        uint32_t m_step;  // Step number of last assignment. 0 means invalid entry.
        যৌক্তিক m_used;  // True if consumed. Can be set even if entry is invalid.
        // CONSTRUCTOR
        Record() { invalidate(); }
        // METHODS
        শূন্য invalidate() {
            m_assignp = nullptr;
            m_step = 0;
            m_used = মিথ্যা;
        }
    };

    // MEMBERS
    // Variable this SubstVarEntry tracks
    AstVar* ধ্রুবক m_varp;
    // The recrod for whole variable tracking
    Record m_wholeRecord;
    // A record for each word in the variable
    std::vector<Record> m_wordRecords{static_cast<আকার_t>(m_varp->width_শব্দগুলি()), Record{}};

    // METHDOS
    শূন্য deleteAssignmentIfUnused(Record& record, আকার_t& nAssignDeleted) {
        যদি (!record.m_assignp) ফেরত;
        যদি (record.m_used) ফেরত;
        ++nAssignDeleted;
        VL_DO_DANGLING(record.m_assignp->unlinkFrBack()->delete_বৃক্ষ(), record.m_assignp);
    }

    Ast_গ্রন্থি_রাশিমালা* substRecord(Record& record);

সর্বজনীন:
    // CONSTRUCTORS
    explicit SubstVarEntry(AstVar* varp)
        : m_varp{varp} {}
    ~SubstVarEntry() = পূর্বনির্ধারিত;

    // Record assignment of whole variable. The given 'assp' can be null, which means
    // the variable is known to be assigned, but to an unknown value.
    শূন্য assignWhole(Ast_গ্রন্থি_Assign* assp, uint32_t step) {
        // Invalidate all word records
        জন্য (Record& wordRecord : m_wordRecords) wordRecord.invalidate();
        // Set whole record
        m_wholeRecord.m_assignp = assp;
        m_wholeRecord.m_step = step;
        m_wholeRecord.m_used = মিথ্যা;
    }

    // Like assignWhole word, but records assignment to a specific word of the variable.
    শূন্য assignWord(Ast_গ্রন্থি_Assign* assp, uint32_t step, uint32_t word) {
        // Invalidate whole record
        m_wholeRecord.invalidate();
        // Set word record
        Record& wordRecord = m_wordRecords[word];
        wordRecord.m_assignp = assp;
        wordRecord.m_step = step;
        wordRecord.m_used = মিথ্যা;
    }

    // Mark the whole variable as used (value consumed)
    শূন্য usedWhole() {
        m_wholeRecord.m_used = সত্য;
        জন্য (Record& wordRecord : m_wordRecords) wordRecord.m_used = সত্য;
    }

    // Mark the specific word as used (value consumed)
    শূন্য usedWord(uint32_t word) {
        m_wholeRecord.m_used = সত্য;
        m_wordRecords[word].m_used = সত্য;
    }

    // Returns substitution of whole word, or nullptr if not known/stale
    Ast_গ্রন্থি_রাশিমালা* substWhole() { ফেরত substRecord(m_wholeRecord); }
    // Returns substitution of whole word, or nullptr if not known/stale
    Ast_গ্রন্থি_রাশিমালা* substWord(uint32_t word) { ফেরত substRecord(m_wordRecords[word]); }

    শূন্য deleteUnusedAssignments(আকার_t& nWordAssignDeleted, আকার_t& nWholeAssignDeleted) {
        // Delete assignments to temporaries if they are not used
        যদি (!m_varp->is_Statement_অস্থায়ী()) ফেরত;
        জন্য (Record& wordRecord : m_wordRecords)
            deleteAssignmentIfUnused(wordRecord, nWordAssignDeleted);
        deleteAssignmentIfUnused(m_wholeRecord, nWholeAssignDeleted);
    }
};

//######################################################################
// See if any variables in the expression we are considering to
// substitute have changed value since we recorded the assignment.

শ্রেণী SubstValidVisitor final : সর্বজনীন VNVisitorConst {
    // NODE STATE
    // See SubstVisitor

    // STATE
    ধ্রুবক uint32_t m_step;  // Step number where assignment was recorded
    যৌক্তিক m_valid = সত্য;  // Is the expression we are considering to substitute valid

    // METHODS
    SubstVarEntry& getEntry(AstVarRef* nodep) {
        // This vistor is always invoked on the RHS of an assignment we are considering to
        // substitute. Variable references must have all been recorded when visiting the
        // assignments RHS, so the SubstVarEntry must exist for each referenced variable.
        ফেরত *nodep->varp()->user1u().to<SubstVarEntry*>();
    }

    // VISITORS
    শূন্য visit(AstWordSel* nodep) override {
        যদি (!m_valid) ফেরত;

        যদি (AstVarRef* ধ্রুবক refp = VN_CAST(nodep->fromp(), VarRef)) {
            যদি (AstConst* ধ্রুবক idxp = VN_CAST(nodep->bitp(), Const)) {
                SubstVarEntry& entry = getEntry(refp);
                // If either the whole variable, or the indexed word was written to
                // after the original assignment was recorded, the value is invalid.
                যদি (m_step < entry.m_wholeRecord.m_step) m_valid = মিথ্যা;
                যদি (m_step < entry.m_wordRecords[idxp->toUInt()].m_step) m_valid = মিথ্যা;
                ফেরত;
            }
        }
        iterateChildrenConst(nodep);
    }

    শূন্য visit(AstVarRef* nodep) override {
        যদি (!m_valid) ফেরত;

        // If either the whole variable, or any of the words were written to
        // after the original assignment was recorded, the value is invalid.
        ধ্রুবক SubstVarEntry& entry = getEntry(nodep);
        যদি (m_step < entry.m_wholeRecord.m_step) {
            m_valid = মিথ্যা;
            ফেরত;
        }
        জন্য (ধ্রুবক SubstVarEntry::Record& wordRecord : entry.m_wordRecords) {
            যদি (m_step < wordRecord.m_step) {
                m_valid = মিথ্যা;
                ফেরত;
            }
        }
    }

    শূন্য visit(AstConst*) override {}  // Accelerate
    শূন্য visit(AstText*) override {}  // CExpr/CStmt literal text has no variable dependencies

    শূন্য visit(Ast_গ্রন্থি_রাশিমালা* nodep) override {
        যদি (!m_valid) ফেরত;
        iterateChildrenConst(nodep);
    }

    শূন্য visit(Ast_গ্রন্থি* nodep) override { nodep->v3fatalSrc("Non AstNodeExpr under AstNodeExpr"); }

    // CONSTRUCTORS
    explicit SubstValidVisitor(SubstVarEntry::Record& record)
        : m_step{record.m_step} {
        iterateConst(record.m_assignp->rhsp());
    }
    ~SubstValidVisitor() override = পূর্বনির্ধারিত;

সর্বজনীন:
    স্থির যৌক্তিক valid(SubstVarEntry::Record& record) {
        যদি (!record.m_assignp) ফেরত মিথ্যা;
        ফেরত SubstValidVisitor{record}.m_valid;
    }
};

Ast_গ্রন্থি_রাশিমালা* SubstVarEntry::substRecord(SubstVarEntry::Record& record) {
    যদি (!SubstValidVisitor::valid(record)) ফেরত nullptr;
    Ast_গ্রন্থি_রাশিমালা* ধ্রুবক rhsp = record.m_assignp->rhsp();
    UDEBUGONLY(UASSERT_OBJ(rhsp->isPure(), record.m_assignp, "Substituting impure expression"););
    ফেরত rhsp;
}

//######################################################################
// Substitution visitor

শ্রেণী SubstVisitor final : সর্বজনীন VNVisitor {
    // NODE STATE - only Under AstCFunc
    // AstVar::user1p -> SubstVarEntry* for assignment tracking. Also used by SubstValidVisitor

    // STATE
    std::deque<SubstVarEntry> m_entries;  // Storage for SubstVarEntry instances
    uint32_t m_ops = 0;  // Number of nodes on the RHS of an assignment
    uint32_t m_assignStep = 0;  // Assignment number to determine variable lifetimes
    ধ্রুবক AstCFunc* m_funcp = nullptr;  // Current function we are under
    আকার_t m_nSubst = 0;  // Number of substitutions performed - for avoiding constant folding
    // Statistics
    আকার_t m_nWordSubstituted = 0;  // Number of words substituted
    আকার_t m_nWholeSubstituted = 0;  // Number of whole variables substituted
    আকার_t m_nWordAssignDeleted = 0;  // Number of word assignments deleted
    আকার_t m_nWholeAssignDeleted = 0;  // Number of whole variable assignments deleted
    আকার_t m_n_Const_শব্দগুলি_Reinlined = 0;  // Number of constant words substituted

    স্থির constexpr uint32_t SUBST_MAX_OPS_SUBST = 30;  // Maximum number of ops to substitute in
    স্থির constexpr uint32_t SUBST_MAX_OPS_NA = 9999;  // Not allowed to substitute

    // METHODS
    SubstVarEntry& getEntry(AstVarRef* nodep) {
        AstVar* ধ্রুবক varp = nodep->varp();
        যদি (!varp->user1p()) {
            m_entries.emplace_back(varp);
            varp->user1p(&m_entries.back());
        }
        ফেরত *varp->user1u().to<SubstVarEntry*>();
    }

    শূন্য simplify(Ast_গ্রন্থি_রাশিমালা* exprp) {
        // Often constant, so short circuit
        যদি (VN_IS(exprp, Const)) ফেরত;
        // Iterate expression, then constant fold it if anything changed
        ধ্রুবক আকার_t nSubstBefore = m_nSubst;
        exprp = VN_AS(iterateSubtreeReturnEdits(exprp), গ্রন্থি_রাশিমালা);
        যদি (nSubstBefore != m_nSubst) V3Const::constifyEditCpp(exprp);
    }

    // The way the analysis algorithm works based on statement numbering only works
    // for variables that are written in the same basic block (ignoring AstCond) as
    // where they are consumed. The temporaries introduced in V3Premit are such, so
    // only substitute those for now.
    স্থির যৌক্তিক isSubstitutable(AstVar* nodep) {
        ফেরত nodep->is_Statement_অস্থায়ী() && !nodep->noSubst();
    }

    শূন্য substitute(Ast_গ্রন্থি* nodep, Ast_গ্রন্থি_রাশিমালা* substp) {
        Ast_গ্রন্থি_রাশিমালা* newp = substp->backp() ? substp->clone_বৃক্ষ_Pure(সত্য) : substp;
        যদি (!nodep->isQuad() && newp->isQuad()) {
            newp = নতুন AstCCast{newp->fileline(), newp, nodep};
        }
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
        ++m_nSubst;
    }

    // VISITORS

    শূন্য visit(AstNetlist* nodep) override { iterate_And_পরবর্তী_Null(nodep->modulesp()); }

    শূন্য visit(AstCFunc* nodep) override {
        UASSERT_OBJ(!m_funcp, nodep, "Should not nest");
        UASSERT_OBJ(m_entries.empty(), nodep, "Should not visit outside functions");

        // Process the function body
        {
            VL_RESTORER(m_funcp);
            m_funcp = nodep;
            ধ্রুবক VNUser1InUse m_inuser1;
            iterateChildren(nodep);
            // Deletes unused assignments and clear entries
            জন্য (SubstVarEntry& entry : m_entries) {
                entry.deleteUnusedAssignments(m_nWordAssignDeleted, m_nWholeAssignDeleted);
            }
            m_entries.clear();
        }

        // Constant fold here, as Ast size can likely be reduced
        যদি (v3Global.opt.fConstEager()) V3Const::constifyEditCpp(nodep);
    }

    শূন্য visit(Ast_গ্রন্থি_Assign* nodep) override {
        যদি (!m_funcp) ফেরত;

        ধ্রুবক uint32_t ops = [&]() {
            VL_RESTORER(m_ops);
            m_ops = 0;
            // Simplify the RHS
            simplify(nodep->rhsp());
            // If the became constant, we can continue substituting it
            ফেরত VN_IS(nodep->rhsp(), Const) ? 0 : m_ops;
        }();

        // Assignment that defines the value, which is either this 'nodep',
        // or nullptr, if the value should not be substituted.
        ধ্রুবক auto getAssignp = [&](AstVarRef* refp) -> Ast_গ্রন্থি_Assign* {
            // If too complex, don't substitute
            যদি (ops > SUBST_MAX_OPS_SUBST) ফেরত nullptr;
            // AstCvtPackedToArray can't be anywhere else than on the RHS of assignment
            যদি (VN_IS(nodep->rhsp(), CvtPackedToArray)) ফেরত nullptr;
            // If non-const but want const subtitutions only
            যদি (refp->varp()->substConstOnly() && !VN_IS(nodep->rhsp(), Const)) ফেরত nullptr;
            // Otherwise can substitute based on the assignment
            ফেরত nodep;
        };

        // If LHS is a whole variable reference, track the whole variable
        যদি (AstVarRef* ধ্রুবক refp = VN_CAST(nodep->lhsp(), VarRef)) {
            getEntry(refp).assignWhole(getAssignp(refp), ++m_assignStep);
            ফেরত;
        }

        // If LHS is a known word reference, track the word
        যদি (ধ্রুবক AstWordSel* ধ্রুবক selp = VN_CAST(nodep->lhsp(), WordSel)) {
            যদি (AstVarRef* ধ্রুবক refp = VN_CAST(selp->fromp(), VarRef)) {
                // Simplify the index
                simplify(selp->bitp());
                যদি (ধ্রুবক AstConst* ধ্রুবক idxp = VN_CAST(selp->bitp(), Const)) {
                    getEntry(refp).assignWord(getAssignp(refp), ++m_assignStep, idxp->toUInt());
                    ফেরত;
                }
            }
        }

        // Not tracked, iterate LHS to simplify/reset tracking
        iterate(nodep->lhsp());
    }

    শূন্য visit(AstWordSel* nodep) override {
        যদি (!m_funcp) ফেরত;

        // Simplify the index
        simplify(nodep->bitp());

        // If this is a known word reference, track/substitute it
        যদি (AstVarRef* ধ্রুবক refp = VN_CAST(nodep->fromp(), VarRef)) {
            যদি (ধ্রুবক AstConst* ধ্রুবক idxp = VN_CAST(nodep->bitp(), Const)) {
                SubstVarEntry& entry = getEntry(refp);
                ধ্রুবক uint32_t word = idxp->toUInt();

                // If it's a write, reset tracking as we don't know the assigned value,
                // otherwise we would have picked it up in visit(AstNodeAssign*)
                যদি (refp->access().isWriteOrRW()) {
                    entry.assignWord(nullptr, ++m_assignStep, word);
                    ফেরত;
                }

                // Otherwise it's a read,
                UASSERT_OBJ(refp->access().isReadOnly(), nodep, "Invalid access");

                // If it's a constant pool variable, substiute with the constant word
                AstVar* ধ্রুবক varp = refp->varp();
                যদি (varp->constPoolEntry()) {
                    AstConst* ধ্রুবক constp = VN_AS(varp->valuep(), Const);
                    ধ্রুবক uint32_t মান = constp->অঙ্ক().edataWord(word);
                    File_সারি* ধ্রুবক flp = nodep->fileline();
                    ++m_n_Const_শব্দগুলি_Reinlined;
                    substitute(nodep, নতুন AstConst{flp, AstConst::SizedEData{}, মান});
                    ফেরত;
                }

                // Substitute other variables if possible
                যদি (isSubstitutable(refp->varp())) {
                    যদি (Ast_গ্রন্থি_রাশিমালা* ধ্রুবক substp = entry.substWord(word)) {
                        ++m_nWordSubstituted;
                        substitute(nodep, substp);
                        ফেরত;
                    }
                }

                // If not substituted, mark the assignment setting this word as used
                entry.usedWord(word);
                ফেরত;
            }
        }

        // If not a known word reference, iterate fromp to simplify/reset tracking
        iterate(nodep->fromp());
    }

    শূন্য visit(AstVarRef* nodep) override {
        যদি (!m_funcp) ফেরত;

        SubstVarEntry& entry = getEntry(nodep);

        // If it's a write, reset tracking as we don't know the assigned value,
        // otherwise we would have picked it up in visit(AstNodeAssign*)
        যদি (nodep->access().isWriteOrRW()) {
            entry.assignWhole(nullptr, ++m_assignStep);
            ফেরত;
        }

        // Otherwise it's a read, substitute it if possible
        UASSERT_OBJ(nodep->access().isReadOnly(), nodep, "Invalid access");
        যদি (isSubstitutable(nodep->varp())) {
            যদি (Ast_গ্রন্থি_রাশিমালা* ধ্রুবক substp = entry.substWhole()) {
                // Do not substitute a compound wide expression.
                // The whole point of adding temporaries is to eliminate them.
                যদি (!nodep->isWide() || VN_IS(substp, VarRef)) {
                    ++m_nWholeSubstituted;
                    substitute(nodep, substp);
                    ফেরত;
                }
            }
        }

        // If not substituted, mark the assignment setting this variable as used
        entry.usedWhole();
    }

    শূন্য visit(Ast_গ্রন্থি_রাশিমালা* nodep) override {
        যদি (!m_funcp) ফেরত;

        // Count nodes as we descend to track complexity
        ++m_ops;
        // First iterate children, this can cache child purity
        iterateChildren(nodep);
        // Do not substitute impure expressions
        যদি (!nodep->isPure()) m_ops = SUBST_MAX_OPS_NA;
    }

    শূন্য visit(AstVar*) override {}  // Accelerate
    শূন্য visit(AstConst*) override {}  // Accelerate

    শূন্য visit(Ast_গ্রন্থি* nodep) override { iterateChildren(nodep); }

সর্বজনীন:
    // CONSTRUCTORS
    explicit SubstVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~SubstVisitor() override {
        V3Stats::addStat("Optimizations, Subst, Substituted temps", m_nSubst);
        V3Stats::addStat("Optimizations, Subst, Constant words reinlined", m_n_Const_শব্দগুলি_Reinlined);
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

শূন্য V3Subst::substituteAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { SubstVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_বৃক্ষ("subst", 0, dump_বৃক্ষ_Either_Level() >= 3);
}
