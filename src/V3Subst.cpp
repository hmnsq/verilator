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

#அந்தர்பாவ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#அந்தர்பாவ "V3Subst.h"

#அந்தர்பாவ "V3Const.h"
#அந்தர்பாவ "V3Stats.h"

#அந்தர்பாவ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Data strored for every variable that tracks assignments and usage

வகுப்பு SubstVarEntry final {
    friend வகுப்பு SubstValidVisitor;

    // SubstVarEntry contains a Record for each word, and one extra for the whole variable
    கட்டமைப்பு Record final {
        // MEMBERS
        Ast_முனை_Assign* m_assignp;  // Last assignment to this part
        uint32_t m_step;  // Step number of last assignment. 0 means invalid entry.
        தருக்க m_used;  // True if consumed. Can be set even if entry is invalid.
        // CONSTRUCTOR
        Record() { invalidate(); }
        // METHODS
        வெறுமை invalidate() {
            m_assignp = nullptr;
            m_step = 0;
            m_used = பொய்;
        }
    };

    // MEMBERS
    // Variable this SubstVarEntry tracks
    AstVar* மாறிலி m_varp;
    // The recrod for whole variable tracking
    Record m_wholeRecord;
    // A record for each word in the variable
    std::vector<Record> m_wordRecords{static_cast<size_t>(m_varp->width_சொற்கள்()), Record{}};

    // METHDOS
    வெறுமை deleteAssignmentIfUnused(Record& record, size_t& nAssignDeleted) {
        எனில் (!record.m_assignp) திரும்பு;
        எனில் (record.m_used) திரும்பு;
        ++nAssignDeleted;
        VL_DO_DANGLING(record.m_assignp->unlinkFrBack()->delete_மரம்(), record.m_assignp);
    }

    Ast_முனை_கோவை* substRecord(Record& record);

பொது:
    // CONSTRUCTORS
    explicit SubstVarEntry(AstVar* varp)
        : m_varp{varp} {}
    ~SubstVarEntry() = இயல்பு;

    // Record assignment of whole variable. The given 'assp' can be null, which means
    // the variable is known to be assigned, but to an unknown value.
    வெறுமை assignWhole(Ast_முனை_Assign* assp, uint32_t step) {
        // Invalidate all word records
        க்கு (Record& wordRecord : m_wordRecords) wordRecord.invalidate();
        // Set whole record
        m_wholeRecord.m_assignp = assp;
        m_wholeRecord.m_step = step;
        m_wholeRecord.m_used = பொய்;
    }

    // Like assignWhole word, but records assignment to a specific word of the variable.
    வெறுமை assignWord(Ast_முனை_Assign* assp, uint32_t step, uint32_t word) {
        // Invalidate whole record
        m_wholeRecord.invalidate();
        // Set word record
        Record& wordRecord = m_wordRecords[word];
        wordRecord.m_assignp = assp;
        wordRecord.m_step = step;
        wordRecord.m_used = பொய்;
    }

    // Mark the whole variable as used (value consumed)
    வெறுமை usedWhole() {
        m_wholeRecord.m_used = உண்மை;
        க்கு (Record& wordRecord : m_wordRecords) wordRecord.m_used = உண்மை;
    }

    // Mark the specific word as used (value consumed)
    வெறுமை usedWord(uint32_t word) {
        m_wholeRecord.m_used = உண்மை;
        m_wordRecords[word].m_used = உண்மை;
    }

    // Returns substitution of whole word, or nullptr if not known/stale
    Ast_முனை_கோவை* substWhole() { திரும்பு substRecord(m_wholeRecord); }
    // Returns substitution of whole word, or nullptr if not known/stale
    Ast_முனை_கோவை* substWord(uint32_t word) { திரும்பு substRecord(m_wordRecords[word]); }

    வெறுமை deleteUnusedAssignments(size_t& nWordAssignDeleted, size_t& nWholeAssignDeleted) {
        // Delete assignments to temporaries if they are not used
        எனில் (!m_varp->isStatementTemp()) திரும்பு;
        க்கு (Record& wordRecord : m_wordRecords)
            deleteAssignmentIfUnused(wordRecord, nWordAssignDeleted);
        deleteAssignmentIfUnused(m_wholeRecord, nWholeAssignDeleted);
    }
};

//######################################################################
// See if any variables in the expression we are considering to
// substitute have changed value since we recorded the assignment.

வகுப்பு SubstValidVisitor final : பொது VNVisitorConst {
    // NODE STATE
    // See SubstVisitor

    // STATE
    மாறிலி uint32_t m_step;  // Step number where assignment was recorded
    தருக்க m_valid = உண்மை;  // Is the expression we are considering to substitute valid

    // METHODS
    SubstVarEntry& getEntry(AstVarRef* nodep) {
        // This vistor is always invoked on the RHS of an assignment we are considering to
        // substitute. Variable references must have all been recorded when visiting the
        // assignments RHS, so the SubstVarEntry must exist for each referenced variable.
        திரும்பு *nodep->varp()->user1u().to<SubstVarEntry*>();
    }

    // VISITORS
    வெறுமை visit(AstWordSel* nodep) override {
        எனில் (!m_valid) திரும்பு;

        எனில் (AstVarRef* மாறிலி refp = VN_CAST(nodep->fromp(), VarRef)) {
            எனில் (AstConst* மாறிலி idxp = VN_CAST(nodep->bitp(), Const)) {
                SubstVarEntry& entry = getEntry(refp);
                // If either the whole variable, or the indexed word was written to
                // after the original assignment was recorded, the value is invalid.
                எனில் (m_step < entry.m_wholeRecord.m_step) m_valid = பொய்;
                எனில் (m_step < entry.m_wordRecords[idxp->toUInt()].m_step) m_valid = பொய்;
                திரும்பு;
            }
        }
        iterateChildrenConst(nodep);
    }

    வெறுமை visit(AstVarRef* nodep) override {
        எனில் (!m_valid) திரும்பு;

        // If either the whole variable, or any of the words were written to
        // after the original assignment was recorded, the value is invalid.
        மாறிலி SubstVarEntry& entry = getEntry(nodep);
        எனில் (m_step < entry.m_wholeRecord.m_step) {
            m_valid = பொய்;
            திரும்பு;
        }
        க்கு (மாறிலி SubstVarEntry::Record& wordRecord : entry.m_wordRecords) {
            எனில் (m_step < wordRecord.m_step) {
                m_valid = பொய்;
                திரும்பு;
            }
        }
    }

    வெறுமை visit(AstConst*) override {}  // Accelerate
    வெறுமை visit(AstText*) override {}  // CExpr/CStmt literal text has no variable dependencies

    வெறுமை visit(Ast_முனை_கோவை* nodep) override {
        எனில் (!m_valid) திரும்பு;
        iterateChildrenConst(nodep);
    }

    வெறுமை visit(Ast_முனை* nodep) override { nodep->v3fatalSrc("Non AstNodeExpr under AstNodeExpr"); }

    // CONSTRUCTORS
    explicit SubstValidVisitor(SubstVarEntry::Record& record)
        : m_step{record.m_step} {
        iterateConst(record.m_assignp->rhsp());
    }
    ~SubstValidVisitor() override = இயல்பு;

பொது:
    நிலையான தருக்க valid(SubstVarEntry::Record& record) {
        எனில் (!record.m_assignp) திரும்பு பொய்;
        திரும்பு SubstValidVisitor{record}.m_valid;
    }
};

Ast_முனை_கோவை* SubstVarEntry::substRecord(SubstVarEntry::Record& record) {
    எனில் (!SubstValidVisitor::valid(record)) திரும்பு nullptr;
    Ast_முனை_கோவை* மாறிலி rhsp = record.m_assignp->rhsp();
    UDEBUGONLY(UASSERT_OBJ(rhsp->isPure(), record.m_assignp, "Substituting impure expression"););
    திரும்பு rhsp;
}

//######################################################################
// Substitution visitor

வகுப்பு SubstVisitor final : பொது VNVisitor {
    // NODE STATE - only Under AstCFunc
    // AstVar::user1p -> SubstVarEntry* for assignment tracking. Also used by SubstValidVisitor

    // STATE
    std::deque<SubstVarEntry> m_entries;  // Storage for SubstVarEntry instances
    uint32_t m_ops = 0;  // Number of nodes on the RHS of an assignment
    uint32_t m_assignStep = 0;  // Assignment number to determine variable lifetimes
    மாறிலி AstCFunc* m_funcp = nullptr;  // Current function we are under
    size_t m_nSubst = 0;  // Number of substitutions performed - for avoiding constant folding
    // Statistics
    size_t m_nWordSubstituted = 0;  // Number of words substituted
    size_t m_nWholeSubstituted = 0;  // Number of whole variables substituted
    size_t m_nWordAssignDeleted = 0;  // Number of word assignments deleted
    size_t m_nWholeAssignDeleted = 0;  // Number of whole variable assignments deleted
    size_t m_n_Const_சொற்கள்_Reinlined = 0;  // Number of constant words substituted

    நிலையான constexpr uint32_t SUBST_MAX_OPS_SUBST = 30;  // Maximum number of ops to substitute in
    நிலையான constexpr uint32_t SUBST_MAX_OPS_NA = 9999;  // Not allowed to substitute

    // METHODS
    SubstVarEntry& getEntry(AstVarRef* nodep) {
        AstVar* மாறிலி varp = nodep->varp();
        எனில் (!varp->user1p()) {
            m_entries.emplace_back(varp);
            varp->user1p(&m_entries.back());
        }
        திரும்பு *varp->user1u().to<SubstVarEntry*>();
    }

    வெறுமை simplify(Ast_முனை_கோவை* exprp) {
        // Often constant, so short circuit
        எனில் (VN_IS(exprp, Const)) திரும்பு;
        // Iterate expression, then constant fold it if anything changed
        மாறிலி size_t nSubstBefore = m_nSubst;
        exprp = VN_AS(iterateSubtreeReturnEdits(exprp), முனை_கோவை);
        எனில் (nSubstBefore != m_nSubst) V3Const::constifyEditCpp(exprp);
    }

    // The way the analysis algorithm works based on statement numbering only works
    // for variables that are written in the same basic block (ignoring AstCond) as
    // where they are consumed. The temporaries introduced in V3Premit are such, so
    // only substitute those for now.
    நிலையான தருக்க isSubstitutable(AstVar* nodep) {
        திரும்பு nodep->isStatementTemp() && !nodep->noSubst();
    }

    வெறுமை substitute(Ast_முனை* nodep, Ast_முனை_கோவை* substp) {
        Ast_முனை_கோவை* newp = substp->backp() ? substp->clone_மரம்_Pure(உண்மை) : substp;
        எனில் (!nodep->isQuad() && newp->isQuad()) {
            newp = புதிய AstCCast{newp->fileline(), newp, nodep};
        }
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_மரம்(), nodep);
        ++m_nSubst;
    }

    // VISITORS

    வெறுமை visit(AstNetlist* nodep) override { iterateAndNextNull(nodep->modulesp()); }

    வெறுமை visit(AstCFunc* nodep) override {
        UASSERT_OBJ(!m_funcp, nodep, "Should not nest");
        UASSERT_OBJ(m_entries.empty(), nodep, "Should not visit outside functions");

        // Process the function body
        {
            VL_RESTORER(m_funcp);
            m_funcp = nodep;
            மாறிலி VNUser1InUse m_inuser1;
            iterateChildren(nodep);
            // Deletes unused assignments and clear entries
            க்கு (SubstVarEntry& entry : m_entries) {
                entry.deleteUnusedAssignments(m_nWordAssignDeleted, m_nWholeAssignDeleted);
            }
            m_entries.clear();
        }

        // Constant fold here, as Ast size can likely be reduced
        எனில் (v3Global.opt.fConstEager()) V3Const::constifyEditCpp(nodep);
    }

    வெறுமை visit(Ast_முனை_Assign* nodep) override {
        எனில் (!m_funcp) திரும்பு;

        மாறிலி uint32_t ops = [&]() {
            VL_RESTORER(m_ops);
            m_ops = 0;
            // Simplify the RHS
            simplify(nodep->rhsp());
            // If the became constant, we can continue substituting it
            திரும்பு VN_IS(nodep->rhsp(), Const) ? 0 : m_ops;
        }();

        // Assignment that defines the value, which is either this 'nodep',
        // or nullptr, if the value should not be substituted.
        மாறிலி auto getAssignp = [&](AstVarRef* refp) -> Ast_முனை_Assign* {
            // If too complex, don't substitute
            எனில் (ops > SUBST_MAX_OPS_SUBST) திரும்பு nullptr;
            // AstCvtPackedToArray can't be anywhere else than on the RHS of assignment
            எனில் (VN_IS(nodep->rhsp(), CvtPackedToArray)) திரும்பு nullptr;
            // If non-const but want const subtitutions only
            எனில் (refp->varp()->substConstOnly() && !VN_IS(nodep->rhsp(), Const)) திரும்பு nullptr;
            // Otherwise can substitute based on the assignment
            திரும்பு nodep;
        };

        // If LHS is a whole variable reference, track the whole variable
        எனில் (AstVarRef* மாறிலி refp = VN_CAST(nodep->lhsp(), VarRef)) {
            getEntry(refp).assignWhole(getAssignp(refp), ++m_assignStep);
            திரும்பு;
        }

        // If LHS is a known word reference, track the word
        எனில் (மாறிலி AstWordSel* மாறிலி selp = VN_CAST(nodep->lhsp(), WordSel)) {
            எனில் (AstVarRef* மாறிலி refp = VN_CAST(selp->fromp(), VarRef)) {
                // Simplify the index
                simplify(selp->bitp());
                எனில் (மாறிலி AstConst* மாறிலி idxp = VN_CAST(selp->bitp(), Const)) {
                    getEntry(refp).assignWord(getAssignp(refp), ++m_assignStep, idxp->toUInt());
                    திரும்பு;
                }
            }
        }

        // Not tracked, iterate LHS to simplify/reset tracking
        iterate(nodep->lhsp());
    }

    வெறுமை visit(AstWordSel* nodep) override {
        எனில் (!m_funcp) திரும்பு;

        // Simplify the index
        simplify(nodep->bitp());

        // If this is a known word reference, track/substitute it
        எனில் (AstVarRef* மாறிலி refp = VN_CAST(nodep->fromp(), VarRef)) {
            எனில் (மாறிலி AstConst* மாறிலி idxp = VN_CAST(nodep->bitp(), Const)) {
                SubstVarEntry& entry = getEntry(refp);
                மாறிலி uint32_t word = idxp->toUInt();

                // If it's a write, reset tracking as we don't know the assigned value,
                // otherwise we would have picked it up in visit(AstNodeAssign*)
                எனில் (refp->access().isWriteOrRW()) {
                    entry.assignWord(nullptr, ++m_assignStep, word);
                    திரும்பு;
                }

                // Otherwise it's a read,
                UASSERT_OBJ(refp->access().isReadOnly(), nodep, "Invalid access");

                // If it's a constant pool variable, substiute with the constant word
                AstVar* மாறிலி varp = refp->varp();
                எனில் (varp->constPoolEntry()) {
                    AstConst* மாறிலி constp = VN_AS(varp->valuep(), Const);
                    மாறிலி uint32_t மதிப்பு = constp->இலக்கம்().edataWord(word);
                    File_வரி* மாறிலி flp = nodep->fileline();
                    ++m_n_Const_சொற்கள்_Reinlined;
                    substitute(nodep, புதிய AstConst{flp, AstConst::SizedEData{}, மதிப்பு});
                    திரும்பு;
                }

                // Substitute other variables if possible
                எனில் (isSubstitutable(refp->varp())) {
                    எனில் (Ast_முனை_கோவை* மாறிலி substp = entry.substWord(word)) {
                        ++m_nWordSubstituted;
                        substitute(nodep, substp);
                        திரும்பு;
                    }
                }

                // If not substituted, mark the assignment setting this word as used
                entry.usedWord(word);
                திரும்பு;
            }
        }

        // If not a known word reference, iterate fromp to simplify/reset tracking
        iterate(nodep->fromp());
    }

    வெறுமை visit(AstVarRef* nodep) override {
        எனில் (!m_funcp) திரும்பு;

        SubstVarEntry& entry = getEntry(nodep);

        // If it's a write, reset tracking as we don't know the assigned value,
        // otherwise we would have picked it up in visit(AstNodeAssign*)
        எனில் (nodep->access().isWriteOrRW()) {
            entry.assignWhole(nullptr, ++m_assignStep);
            திரும்பு;
        }

        // Otherwise it's a read, substitute it if possible
        UASSERT_OBJ(nodep->access().isReadOnly(), nodep, "Invalid access");
        எனில் (isSubstitutable(nodep->varp())) {
            எனில் (Ast_முனை_கோவை* மாறிலி substp = entry.substWhole()) {
                // Do not substitute a compound wide expression.
                // The whole point of adding temporaries is to eliminate them.
                எனில் (!nodep->isWide() || VN_IS(substp, VarRef)) {
                    ++m_nWholeSubstituted;
                    substitute(nodep, substp);
                    திரும்பு;
                }
            }
        }

        // If not substituted, mark the assignment setting this variable as used
        entry.usedWhole();
    }

    வெறுமை visit(Ast_முனை_கோவை* nodep) override {
        எனில் (!m_funcp) திரும்பு;

        // Count nodes as we descend to track complexity
        ++m_ops;
        // First iterate children, this can cache child purity
        iterateChildren(nodep);
        // Do not substitute impure expressions
        எனில் (!nodep->isPure()) m_ops = SUBST_MAX_OPS_NA;
    }

    வெறுமை visit(AstVar*) override {}  // Accelerate
    வெறுமை visit(AstConst*) override {}  // Accelerate

    வெறுமை visit(Ast_முனை* nodep) override { iterateChildren(nodep); }

பொது:
    // CONSTRUCTORS
    explicit SubstVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~SubstVisitor() override {
        V3Stats::addStat("Optimizations, Subst, Substituted temps", m_nSubst);
        V3Stats::addStat("Optimizations, Subst, Constant words reinlined", m_n_Const_சொற்கள்_Reinlined);
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

வெறுமை V3Subst::substituteAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { SubstVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_மரம்("subst", 0, dump_மரம்_Either_Level() >= 3);
}
