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

#ଅନ୍ତର୍ଭାଵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ଅନ୍ତର୍ଭାଵ "V3Subst.h"

#ଅନ୍ତର୍ଭାଵ "V3Const.h"
#ଅନ୍ତର୍ଭାଵ "V3Stats.h"

#ଅନ୍ତର୍ଭାଵ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Data strored for every variable that tracks assignments and usage

ଶ୍ରେଣୀ SubstVarEntry final {
    friend ଶ୍ରେଣୀ SubstValidVisitor;

    // SubstVarEntry contains a Record for each word, and one extra for the whole variable
    ସଂରଚନା Record final {
        // MEMBERS
        Ast_ଗଣ୍ଠି_Assign* m_assignp;  // Last assignment to this part
        uint32_t m_step;  // Step number of last assignment. 0 means invalid entry.
        ତାର୍କିକ m_used;  // True if consumed. Can be set even if entry is invalid.
        // CONSTRUCTOR
        Record() { invalidate(); }
        // METHODS
        ଶୂନ୍ୟ invalidate() {
            m_assignp = nullptr;
            m_step = 0;
            m_used = ମିଥ୍ୟା;
        }
    };

    // MEMBERS
    // Variable this SubstVarEntry tracks
    AstVar* ଅଚଳ m_varp;
    // The recrod for whole variable tracking
    Record m_wholeRecord;
    // A record for each word in the variable
    std::vector<Record> m_wordRecords{static_cast<size_t>(m_varp->width_ଶବ୍ଦଗୁଡ଼ିକ()), Record{}};

    // METHDOS
    ଶୂନ୍ୟ deleteAssignmentIfUnused(Record& record, size_t& nAssignDeleted) {
        ଯଦି (!record.m_assignp) ଫେରନ୍ତୁ;
        ଯଦି (record.m_used) ଫେରନ୍ତୁ;
        ++nAssignDeleted;
        VL_DO_DANGLING(record.m_assignp->unlinkFrBack()->delete_ବୃକ୍ଷ(), record.m_assignp);
    }

    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* substRecord(Record& record);

ସର୍ବସାଧାରଣ:
    // CONSTRUCTORS
    explicit SubstVarEntry(AstVar* varp)
        : m_varp{varp} {}
    ~SubstVarEntry() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;

    // Record assignment of whole variable. The given 'assp' can be null, which means
    // the variable is known to be assigned, but to an unknown value.
    ଶୂନ୍ୟ assignWhole(Ast_ଗଣ୍ଠି_Assign* assp, uint32_t step) {
        // Invalidate all word records
        ପାଇଁ (Record& wordRecord : m_wordRecords) wordRecord.invalidate();
        // Set whole record
        m_wholeRecord.m_assignp = assp;
        m_wholeRecord.m_step = step;
        m_wholeRecord.m_used = ମିଥ୍ୟା;
    }

    // Like assignWhole word, but records assignment to a specific word of the variable.
    ଶୂନ୍ୟ assignWord(Ast_ଗଣ୍ଠି_Assign* assp, uint32_t step, uint32_t word) {
        // Invalidate whole record
        m_wholeRecord.invalidate();
        // Set word record
        Record& wordRecord = m_wordRecords[word];
        wordRecord.m_assignp = assp;
        wordRecord.m_step = step;
        wordRecord.m_used = ମିଥ୍ୟା;
    }

    // Mark the whole variable as used (value consumed)
    ଶୂନ୍ୟ usedWhole() {
        m_wholeRecord.m_used = ସତ୍ୟ;
        ପାଇଁ (Record& wordRecord : m_wordRecords) wordRecord.m_used = ସତ୍ୟ;
    }

    // Mark the specific word as used (value consumed)
    ଶୂନ୍ୟ usedWord(uint32_t word) {
        m_wholeRecord.m_used = ସତ୍ୟ;
        m_wordRecords[word].m_used = ସତ୍ୟ;
    }

    // Returns substitution of whole word, or nullptr if not known/stale
    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* substWhole() { ଫେରନ୍ତୁ substRecord(m_wholeRecord); }
    // Returns substitution of whole word, or nullptr if not known/stale
    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* substWord(uint32_t word) { ଫେରନ୍ତୁ substRecord(m_wordRecords[word]); }

    ଶୂନ୍ୟ deleteUnusedAssignments(size_t& nWordAssignDeleted, size_t& nWholeAssignDeleted) {
        // Delete assignments to temporaries if they are not used
        ଯଦି (!m_varp->isStatementTemp()) ଫେରନ୍ତୁ;
        ପାଇଁ (Record& wordRecord : m_wordRecords)
            deleteAssignmentIfUnused(wordRecord, nWordAssignDeleted);
        deleteAssignmentIfUnused(m_wholeRecord, nWholeAssignDeleted);
    }
};

//######################################################################
// See if any variables in the expression we are considering to
// substitute have changed value since we recorded the assignment.

ଶ୍ରେଣୀ SubstValidVisitor final : ସର୍ବସାଧାରଣ VNVisitorConst {
    // NODE STATE
    // See SubstVisitor

    // STATE
    ଅଚଳ uint32_t m_step;  // Step number where assignment was recorded
    ତାର୍କିକ m_valid = ସତ୍ୟ;  // Is the expression we are considering to substitute valid

    // METHODS
    SubstVarEntry& getEntry(AstVarRef* nodep) {
        // This vistor is always invoked on the RHS of an assignment we are considering to
        // substitute. Variable references must have all been recorded when visiting the
        // assignments RHS, so the SubstVarEntry must exist for each referenced variable.
        ଫେରନ୍ତୁ *nodep->varp()->user1u().to<SubstVarEntry*>();
    }

    // VISITORS
    ଶୂନ୍ୟ visit(AstWordSel* nodep) override {
        ଯଦି (!m_valid) ଫେରନ୍ତୁ;

        ଯଦି (AstVarRef* ଅଚଳ refp = VN_CAST(nodep->fromp(), VarRef)) {
            ଯଦି (AstConst* ଅଚଳ idxp = VN_CAST(nodep->bitp(), Const)) {
                SubstVarEntry& entry = getEntry(refp);
                // If either the whole variable, or the indexed word was written to
                // after the original assignment was recorded, the value is invalid.
                ଯଦି (m_step < entry.m_wholeRecord.m_step) m_valid = ମିଥ୍ୟା;
                ଯଦି (m_step < entry.m_wordRecords[idxp->toUInt()].m_step) m_valid = ମିଥ୍ୟା;
                ଫେରନ୍ତୁ;
            }
        }
        iterateChildrenConst(nodep);
    }

    ଶୂନ୍ୟ visit(AstVarRef* nodep) override {
        ଯଦି (!m_valid) ଫେରନ୍ତୁ;

        // If either the whole variable, or any of the words were written to
        // after the original assignment was recorded, the value is invalid.
        ଅଚଳ SubstVarEntry& entry = getEntry(nodep);
        ଯଦି (m_step < entry.m_wholeRecord.m_step) {
            m_valid = ମିଥ୍ୟା;
            ଫେରନ୍ତୁ;
        }
        ପାଇଁ (ଅଚଳ SubstVarEntry::Record& wordRecord : entry.m_wordRecords) {
            ଯଦି (m_step < wordRecord.m_step) {
                m_valid = ମିଥ୍ୟା;
                ଫେରନ୍ତୁ;
            }
        }
    }

    ଶୂନ୍ୟ visit(AstConst*) override {}  // Accelerate
    ଶୂନ୍ୟ visit(AstText*) override {}  // CExpr/CStmt literal text has no variable dependencies

    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* nodep) override {
        ଯଦି (!m_valid) ଫେରନ୍ତୁ;
        iterateChildrenConst(nodep);
    }

    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override { nodep->v3fatalSrc("Non AstNodeExpr under AstNodeExpr"); }

    // CONSTRUCTORS
    explicit SubstValidVisitor(SubstVarEntry::Record& record)
        : m_step{record.m_step} {
        iterateConst(record.m_assignp->rhsp());
    }
    ~SubstValidVisitor() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;

ସର୍ବସାଧାରଣ:
    ସ୍ଥିର ତାର୍କିକ valid(SubstVarEntry::Record& record) {
        ଯଦି (!record.m_assignp) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଫେରନ୍ତୁ SubstValidVisitor{record}.m_valid;
    }
};

Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* SubstVarEntry::substRecord(SubstVarEntry::Record& record) {
    ଯଦି (!SubstValidVisitor::valid(record)) ଫେରନ୍ତୁ nullptr;
    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ rhsp = record.m_assignp->rhsp();
    UDEBUGONLY(UASSERT_OBJ(rhsp->isPure(), record.m_assignp, "Substituting impure expression"););
    ଫେରନ୍ତୁ rhsp;
}

//######################################################################
// Substitution visitor

ଶ୍ରେଣୀ SubstVisitor final : ସର୍ବସାଧାରଣ VNVisitor {
    // NODE STATE - only Under AstCFunc
    // AstVar::user1p -> SubstVarEntry* for assignment tracking. Also used by SubstValidVisitor

    // STATE
    std::deque<SubstVarEntry> m_entries;  // Storage for SubstVarEntry instances
    uint32_t m_ops = 0;  // Number of nodes on the RHS of an assignment
    uint32_t m_assignStep = 0;  // Assignment number to determine variable lifetimes
    ଅଚଳ AstCFunc* m_funcp = nullptr;  // Current function we are under
    size_t m_nSubst = 0;  // Number of substitutions performed - for avoiding constant folding
    // Statistics
    size_t m_nWordSubstituted = 0;  // Number of words substituted
    size_t m_nWholeSubstituted = 0;  // Number of whole variables substituted
    size_t m_nWordAssignDeleted = 0;  // Number of word assignments deleted
    size_t m_nWholeAssignDeleted = 0;  // Number of whole variable assignments deleted
    size_t m_n_Const_ଶବ୍ଦଗୁଡ଼ିକ_Reinlined = 0;  // Number of constant words substituted

    ସ୍ଥିର constexpr uint32_t SUBST_MAX_OPS_SUBST = 30;  // Maximum number of ops to substitute in
    ସ୍ଥିର constexpr uint32_t SUBST_MAX_OPS_NA = 9999;  // Not allowed to substitute

    // METHODS
    SubstVarEntry& getEntry(AstVarRef* nodep) {
        AstVar* ଅଚଳ varp = nodep->varp();
        ଯଦି (!varp->user1p()) {
            m_entries.emplace_back(varp);
            varp->user1p(&m_entries.back());
        }
        ଫେରନ୍ତୁ *varp->user1u().to<SubstVarEntry*>();
    }

    ଶୂନ୍ୟ simplify(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp) {
        // Often constant, so short circuit
        ଯଦି (VN_IS(exprp, Const)) ଫେରନ୍ତୁ;
        // Iterate expression, then constant fold it if anything changed
        ଅଚଳ size_t nSubstBefore = m_nSubst;
        exprp = VN_AS(iterateSubtreeReturnEdits(exprp), ଗଣ୍ଠି_ପଦସମଷ୍ଟି);
        ଯଦି (nSubstBefore != m_nSubst) V3Const::constifyEditCpp(exprp);
    }

    // The way the analysis algorithm works based on statement numbering only works
    // for variables that are written in the same basic block (ignoring AstCond) as
    // where they are consumed. The temporaries introduced in V3Premit are such, so
    // only substitute those for now.
    ସ୍ଥିର ତାର୍କିକ isSubstitutable(AstVar* nodep) {
        ଫେରନ୍ତୁ nodep->isStatementTemp() && !nodep->noSubst();
    }

    ଶୂନ୍ୟ substitute(Ast_ଗଣ୍ଠି* nodep, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* substp) {
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* newp = substp->backp() ? substp->clone_ବୃକ୍ଷ_Pure(ସତ୍ୟ) : substp;
        ଯଦି (!nodep->isQuad() && newp->isQuad()) {
            newp = ନୂତନ AstCCast{newp->fileline(), newp, nodep};
        }
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_ବୃକ୍ଷ(), nodep);
        ++m_nSubst;
    }

    // VISITORS

    ଶୂନ୍ୟ visit(AstNetlist* nodep) override { iterateAndNextNull(nodep->modulesp()); }

    ଶୂନ୍ୟ visit(AstCFunc* nodep) override {
        UASSERT_OBJ(!m_funcp, nodep, "Should not nest");
        UASSERT_OBJ(m_entries.empty(), nodep, "Should not visit outside functions");

        // Process the function body
        {
            VL_RESTORER(m_funcp);
            m_funcp = nodep;
            ଅଚଳ VNUser1InUse m_inuser1;
            iterateChildren(nodep);
            // Deletes unused assignments and clear entries
            ପାଇଁ (SubstVarEntry& entry : m_entries) {
                entry.deleteUnusedAssignments(m_nWordAssignDeleted, m_nWholeAssignDeleted);
            }
            m_entries.clear();
        }

        // Constant fold here, as Ast size can likely be reduced
        ଯଦି (v3Global.opt.fConstEager()) V3Const::constifyEditCpp(nodep);
    }

    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Assign* nodep) override {
        ଯଦି (!m_funcp) ଫେରନ୍ତୁ;

        ଅଚଳ uint32_t ops = [&]() {
            VL_RESTORER(m_ops);
            m_ops = 0;
            // Simplify the RHS
            simplify(nodep->rhsp());
            // If the became constant, we can continue substituting it
            ଫେରନ୍ତୁ VN_IS(nodep->rhsp(), Const) ? 0 : m_ops;
        }();

        // Assignment that defines the value, which is either this 'nodep',
        // or nullptr, if the value should not be substituted.
        ଅଚଳ auto getAssignp = [&](AstVarRef* refp) -> Ast_ଗଣ୍ଠି_Assign* {
            // If too complex, don't substitute
            ଯଦି (ops > SUBST_MAX_OPS_SUBST) ଫେରନ୍ତୁ nullptr;
            // AstCvtPackedToArray can't be anywhere else than on the RHS of assignment
            ଯଦି (VN_IS(nodep->rhsp(), CvtPackedToArray)) ଫେରନ୍ତୁ nullptr;
            // If non-const but want const subtitutions only
            ଯଦି (refp->varp()->substConstOnly() && !VN_IS(nodep->rhsp(), Const)) ଫେରନ୍ତୁ nullptr;
            // Otherwise can substitute based on the assignment
            ଫେରନ୍ତୁ nodep;
        };

        // If LHS is a whole variable reference, track the whole variable
        ଯଦି (AstVarRef* ଅଚଳ refp = VN_CAST(nodep->lhsp(), VarRef)) {
            getEntry(refp).assignWhole(getAssignp(refp), ++m_assignStep);
            ଫେରନ୍ତୁ;
        }

        // If LHS is a known word reference, track the word
        ଯଦି (ଅଚଳ AstWordSel* ଅଚଳ selp = VN_CAST(nodep->lhsp(), WordSel)) {
            ଯଦି (AstVarRef* ଅଚଳ refp = VN_CAST(selp->fromp(), VarRef)) {
                // Simplify the index
                simplify(selp->bitp());
                ଯଦି (ଅଚଳ AstConst* ଅଚଳ idxp = VN_CAST(selp->bitp(), Const)) {
                    getEntry(refp).assignWord(getAssignp(refp), ++m_assignStep, idxp->toUInt());
                    ଫେରନ୍ତୁ;
                }
            }
        }

        // Not tracked, iterate LHS to simplify/reset tracking
        iterate(nodep->lhsp());
    }

    ଶୂନ୍ୟ visit(AstWordSel* nodep) override {
        ଯଦି (!m_funcp) ଫେରନ୍ତୁ;

        // Simplify the index
        simplify(nodep->bitp());

        // If this is a known word reference, track/substitute it
        ଯଦି (AstVarRef* ଅଚଳ refp = VN_CAST(nodep->fromp(), VarRef)) {
            ଯଦି (ଅଚଳ AstConst* ଅଚଳ idxp = VN_CAST(nodep->bitp(), Const)) {
                SubstVarEntry& entry = getEntry(refp);
                ଅଚଳ uint32_t word = idxp->toUInt();

                // If it's a write, reset tracking as we don't know the assigned value,
                // otherwise we would have picked it up in visit(AstNodeAssign*)
                ଯଦି (refp->access().isWriteOrRW()) {
                    entry.assignWord(nullptr, ++m_assignStep, word);
                    ଫେରନ୍ତୁ;
                }

                // Otherwise it's a read,
                UASSERT_OBJ(refp->access().isReadOnly(), nodep, "Invalid access");

                // If it's a constant pool variable, substiute with the constant word
                AstVar* ଅଚଳ varp = refp->varp();
                ଯଦି (varp->constPoolEntry()) {
                    AstConst* ଅଚଳ constp = VN_AS(varp->valuep(), Const);
                    ଅଚଳ uint32_t ମୂଲ୍ୟ = constp->ଅଙ୍କ().edataWord(word);
                    File_ଧାଡ଼ି* ଅଚଳ flp = nodep->fileline();
                    ++m_n_Const_ଶବ୍ଦଗୁଡ଼ିକ_Reinlined;
                    substitute(nodep, ନୂତନ AstConst{flp, AstConst::SizedEData{}, ମୂଲ୍ୟ});
                    ଫେରନ୍ତୁ;
                }

                // Substitute other variables if possible
                ଯଦି (isSubstitutable(refp->varp())) {
                    ଯଦି (Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ substp = entry.substWord(word)) {
                        ++m_nWordSubstituted;
                        substitute(nodep, substp);
                        ଫେରନ୍ତୁ;
                    }
                }

                // If not substituted, mark the assignment setting this word as used
                entry.usedWord(word);
                ଫେରନ୍ତୁ;
            }
        }

        // If not a known word reference, iterate fromp to simplify/reset tracking
        iterate(nodep->fromp());
    }

    ଶୂନ୍ୟ visit(AstVarRef* nodep) override {
        ଯଦି (!m_funcp) ଫେରନ୍ତୁ;

        SubstVarEntry& entry = getEntry(nodep);

        // If it's a write, reset tracking as we don't know the assigned value,
        // otherwise we would have picked it up in visit(AstNodeAssign*)
        ଯଦି (nodep->access().isWriteOrRW()) {
            entry.assignWhole(nullptr, ++m_assignStep);
            ଫେରନ୍ତୁ;
        }

        // Otherwise it's a read, substitute it if possible
        UASSERT_OBJ(nodep->access().isReadOnly(), nodep, "Invalid access");
        ଯଦି (isSubstitutable(nodep->varp())) {
            ଯଦି (Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ substp = entry.substWhole()) {
                // Do not substitute a compound wide expression.
                // The whole point of adding temporaries is to eliminate them.
                ଯଦି (!nodep->isWide() || VN_IS(substp, VarRef)) {
                    ++m_nWholeSubstituted;
                    substitute(nodep, substp);
                    ଫେରନ୍ତୁ;
                }
            }
        }

        // If not substituted, mark the assignment setting this variable as used
        entry.usedWhole();
    }

    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* nodep) override {
        ଯଦି (!m_funcp) ଫେରନ୍ତୁ;

        // Count nodes as we descend to track complexity
        ++m_ops;
        // First iterate children, this can cache child purity
        iterateChildren(nodep);
        // Do not substitute impure expressions
        ଯଦି (!nodep->isPure()) m_ops = SUBST_MAX_OPS_NA;
    }

    ଶୂନ୍ୟ visit(AstVar*) override {}  // Accelerate
    ଶୂନ୍ୟ visit(AstConst*) override {}  // Accelerate

    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override { iterateChildren(nodep); }

ସର୍ବସାଧାରଣ:
    // CONSTRUCTORS
    explicit SubstVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~SubstVisitor() override {
        V3Stats::addStat("Optimizations, Subst, Substituted temps", m_nSubst);
        V3Stats::addStat("Optimizations, Subst, Constant words reinlined", m_n_Const_ଶବ୍ଦଗୁଡ଼ିକ_Reinlined);
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

ଶୂନ୍ୟ V3Subst::substituteAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { SubstVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_ବୃକ୍ଷ("subst", 0, dump_ବୃକ୍ଷ_Either_Level() >= 3);
}
