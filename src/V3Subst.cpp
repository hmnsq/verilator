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

#شامل "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#شامل "V3Subst.h"

#شامل "V3Const.h"
#شامل "V3Stats.h"

#شامل <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Data strored for every variable that tracks assignments and usage

رده SubstVarEntry final {
    friend رده SubstValidVisitor;

    // SubstVarEntry contains a Record for each word, and one extra for the whole variable
    ساختار Record final {
        // MEMBERS
        Ast_گره_Assign* m_assignp;  // Last assignment to this part
        uint32_t m_step;  // Step number of last assignment. 0 means invalid entry.
        منطقی m_used;  // True if consumed. Can be set even if entry is invalid.
        // CONSTRUCTOR
        Record() { invalidate(); }
        // METHODS
        تهی invalidate() {
            m_assignp = nullptr;
            m_step = 0;
            m_used = نادرست;
        }
    };

    // MEMBERS
    // Variable this SubstVarEntry tracks
    AstVar* ثابت m_varp;
    // The recrod for whole variable tracking
    Record m_wholeRecord;
    // A record for each word in the variable
    std::vector<Record> m_wordRecords{static_cast<size_t>(m_varp->width_واژگان()), Record{}};

    // METHDOS
    تهی deleteAssignmentIfUnused(Record& record, size_t& nAssignDeleted) {
        اگر (!record.m_assignp) بازگشت;
        اگر (record.m_used) بازگشت;
        ++nAssignDeleted;
        VL_DO_DANGLING(record.m_assignp->unlinkFrBack()->delete_درخت(), record.m_assignp);
    }

    Ast_گره_عبارت* substRecord(Record& record);

عمومی:
    // CONSTRUCTORS
    explicit SubstVarEntry(AstVar* varp)
        : m_varp{varp} {}
    ~SubstVarEntry() = پیشفرض;

    // Record assignment of whole variable. The given 'assp' can be null, which means
    // the variable is known to be assigned, but to an unknown value.
    تهی assignWhole(Ast_گره_Assign* assp, uint32_t step) {
        // Invalidate all word records
        برای (Record& wordRecord : m_wordRecords) wordRecord.invalidate();
        // Set whole record
        m_wholeRecord.m_assignp = assp;
        m_wholeRecord.m_step = step;
        m_wholeRecord.m_used = نادرست;
    }

    // Like assignWhole word, but records assignment to a specific word of the variable.
    تهی assignWord(Ast_گره_Assign* assp, uint32_t step, uint32_t word) {
        // Invalidate whole record
        m_wholeRecord.invalidate();
        // Set word record
        Record& wordRecord = m_wordRecords[word];
        wordRecord.m_assignp = assp;
        wordRecord.m_step = step;
        wordRecord.m_used = نادرست;
    }

    // Mark the whole variable as used (value consumed)
    تهی usedWhole() {
        m_wholeRecord.m_used = درست;
        برای (Record& wordRecord : m_wordRecords) wordRecord.m_used = درست;
    }

    // Mark the specific word as used (value consumed)
    تهی usedWord(uint32_t word) {
        m_wholeRecord.m_used = درست;
        m_wordRecords[word].m_used = درست;
    }

    // Returns substitution of whole word, or nullptr if not known/stale
    Ast_گره_عبارت* substWhole() { بازگشت substRecord(m_wholeRecord); }
    // Returns substitution of whole word, or nullptr if not known/stale
    Ast_گره_عبارت* substWord(uint32_t word) { بازگشت substRecord(m_wordRecords[word]); }

    تهی deleteUnusedAssignments(size_t& nWordAssignDeleted, size_t& nWholeAssignDeleted) {
        // Delete assignments to temporaries if they are not used
        اگر (!m_varp->isStatementTemp()) بازگشت;
        برای (Record& wordRecord : m_wordRecords)
            deleteAssignmentIfUnused(wordRecord, nWordAssignDeleted);
        deleteAssignmentIfUnused(m_wholeRecord, nWholeAssignDeleted);
    }
};

//######################################################################
// See if any variables in the expression we are considering to
// substitute have changed value since we recorded the assignment.

رده SubstValidVisitor final : عمومی VNVisitorConst {
    // NODE STATE
    // See SubstVisitor

    // STATE
    ثابت uint32_t m_step;  // Step number where assignment was recorded
    منطقی m_valid = درست;  // Is the expression we are considering to substitute valid

    // METHODS
    SubstVarEntry& getEntry(AstVarRef* nodep) {
        // This vistor is always invoked on the RHS of an assignment we are considering to
        // substitute. Variable references must have all been recorded when visiting the
        // assignments RHS, so the SubstVarEntry must exist for each referenced variable.
        بازگشت *nodep->varp()->user1u().to<SubstVarEntry*>();
    }

    // VISITORS
    تهی visit(AstWordSel* nodep) override {
        اگر (!m_valid) بازگشت;

        اگر (AstVarRef* ثابت refp = VN_CAST(nodep->fromp(), VarRef)) {
            اگر (AstConst* ثابت idxp = VN_CAST(nodep->bitp(), Const)) {
                SubstVarEntry& entry = getEntry(refp);
                // If either the whole variable, or the indexed word was written to
                // after the original assignment was recorded, the value is invalid.
                اگر (m_step < entry.m_wholeRecord.m_step) m_valid = نادرست;
                اگر (m_step < entry.m_wordRecords[idxp->toUInt()].m_step) m_valid = نادرست;
                بازگشت;
            }
        }
        iterateChildrenConst(nodep);
    }

    تهی visit(AstVarRef* nodep) override {
        اگر (!m_valid) بازگشت;

        // If either the whole variable, or any of the words were written to
        // after the original assignment was recorded, the value is invalid.
        ثابت SubstVarEntry& entry = getEntry(nodep);
        اگر (m_step < entry.m_wholeRecord.m_step) {
            m_valid = نادرست;
            بازگشت;
        }
        برای (ثابت SubstVarEntry::Record& wordRecord : entry.m_wordRecords) {
            اگر (m_step < wordRecord.m_step) {
                m_valid = نادرست;
                بازگشت;
            }
        }
    }

    تهی visit(AstConst*) override {}  // Accelerate
    تهی visit(AstText*) override {}  // CExpr/CStmt literal text has no variable dependencies

    تهی visit(Ast_گره_عبارت* nodep) override {
        اگر (!m_valid) بازگشت;
        iterateChildrenConst(nodep);
    }

    تهی visit(Ast_گره* nodep) override { nodep->v3fatalSrc("Non AstNodeExpr under AstNodeExpr"); }

    // CONSTRUCTORS
    explicit SubstValidVisitor(SubstVarEntry::Record& record)
        : m_step{record.m_step} {
        iterateConst(record.m_assignp->rhsp());
    }
    ~SubstValidVisitor() override = پیشفرض;

عمومی:
    ایستا منطقی valid(SubstVarEntry::Record& record) {
        اگر (!record.m_assignp) بازگشت نادرست;
        بازگشت SubstValidVisitor{record}.m_valid;
    }
};

Ast_گره_عبارت* SubstVarEntry::substRecord(SubstVarEntry::Record& record) {
    اگر (!SubstValidVisitor::valid(record)) بازگشت nullptr;
    Ast_گره_عبارت* ثابت rhsp = record.m_assignp->rhsp();
    UDEBUGONLY(UASSERT_OBJ(rhsp->isPure(), record.m_assignp, "Substituting impure expression"););
    بازگشت rhsp;
}

//######################################################################
// Substitution visitor

رده SubstVisitor final : عمومی VNVisitor {
    // NODE STATE - only Under AstCFunc
    // AstVar::user1p -> SubstVarEntry* for assignment tracking. Also used by SubstValidVisitor

    // STATE
    std::deque<SubstVarEntry> m_entries;  // Storage for SubstVarEntry instances
    uint32_t m_ops = 0;  // Number of nodes on the RHS of an assignment
    uint32_t m_assignStep = 0;  // Assignment number to determine variable lifetimes
    ثابت AstCFunc* m_funcp = nullptr;  // Current function we are under
    size_t m_nSubst = 0;  // Number of substitutions performed - for avoiding constant folding
    // Statistics
    size_t m_nWordSubstituted = 0;  // Number of words substituted
    size_t m_nWholeSubstituted = 0;  // Number of whole variables substituted
    size_t m_nWordAssignDeleted = 0;  // Number of word assignments deleted
    size_t m_nWholeAssignDeleted = 0;  // Number of whole variable assignments deleted
    size_t m_n_Const_واژگان_Reinlined = 0;  // Number of constant words substituted

    ایستا constexpr uint32_t SUBST_MAX_OPS_SUBST = 30;  // Maximum number of ops to substitute in
    ایستا constexpr uint32_t SUBST_MAX_OPS_NA = 9999;  // Not allowed to substitute

    // METHODS
    SubstVarEntry& getEntry(AstVarRef* nodep) {
        AstVar* ثابت varp = nodep->varp();
        اگر (!varp->user1p()) {
            m_entries.emplace_back(varp);
            varp->user1p(&m_entries.back());
        }
        بازگشت *varp->user1u().to<SubstVarEntry*>();
    }

    تهی simplify(Ast_گره_عبارت* exprp) {
        // Often constant, so short circuit
        اگر (VN_IS(exprp, Const)) بازگشت;
        // Iterate expression, then constant fold it if anything changed
        ثابت size_t nSubstBefore = m_nSubst;
        exprp = VN_AS(iterateSubtreeReturnEdits(exprp), گره_عبارت);
        اگر (nSubstBefore != m_nSubst) V3Const::constifyEditCpp(exprp);
    }

    // The way the analysis algorithm works based on statement numbering only works
    // for variables that are written in the same basic block (ignoring AstCond) as
    // where they are consumed. The temporaries introduced in V3Premit are such, so
    // only substitute those for now.
    ایستا منطقی isSubstitutable(AstVar* nodep) {
        بازگشت nodep->isStatementTemp() && !nodep->noSubst();
    }

    تهی substitute(Ast_گره* nodep, Ast_گره_عبارت* substp) {
        Ast_گره_عبارت* newp = substp->backp() ? substp->clone_درخت_Pure(درست) : substp;
        اگر (!nodep->isQuad() && newp->isQuad()) {
            newp = جدید AstCCast{newp->fileline(), newp, nodep};
        }
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_درخت(), nodep);
        ++m_nSubst;
    }

    // VISITORS

    تهی visit(AstNetlist* nodep) override { iterateAndNextNull(nodep->modulesp()); }

    تهی visit(AstCFunc* nodep) override {
        UASSERT_OBJ(!m_funcp, nodep, "Should not nest");
        UASSERT_OBJ(m_entries.empty(), nodep, "Should not visit outside functions");

        // Process the function body
        {
            VL_RESTORER(m_funcp);
            m_funcp = nodep;
            ثابت VNUser1InUse m_inuser1;
            iterateChildren(nodep);
            // Deletes unused assignments and clear entries
            برای (SubstVarEntry& entry : m_entries) {
                entry.deleteUnusedAssignments(m_nWordAssignDeleted, m_nWholeAssignDeleted);
            }
            m_entries.clear();
        }

        // Constant fold here, as Ast size can likely be reduced
        اگر (v3Global.opt.fConstEager()) V3Const::constifyEditCpp(nodep);
    }

    تهی visit(Ast_گره_Assign* nodep) override {
        اگر (!m_funcp) بازگشت;

        ثابت uint32_t ops = [&]() {
            VL_RESTORER(m_ops);
            m_ops = 0;
            // Simplify the RHS
            simplify(nodep->rhsp());
            // If the became constant, we can continue substituting it
            بازگشت VN_IS(nodep->rhsp(), Const) ? 0 : m_ops;
        }();

        // Assignment that defines the value, which is either this 'nodep',
        // or nullptr, if the value should not be substituted.
        ثابت auto getAssignp = [&](AstVarRef* refp) -> Ast_گره_Assign* {
            // If too complex, don't substitute
            اگر (ops > SUBST_MAX_OPS_SUBST) بازگشت nullptr;
            // AstCvtPackedToArray can't be anywhere else than on the RHS of assignment
            اگر (VN_IS(nodep->rhsp(), CvtPackedToArray)) بازگشت nullptr;
            // If non-const but want const subtitutions only
            اگر (refp->varp()->substConstOnly() && !VN_IS(nodep->rhsp(), Const)) بازگشت nullptr;
            // Otherwise can substitute based on the assignment
            بازگشت nodep;
        };

        // If LHS is a whole variable reference, track the whole variable
        اگر (AstVarRef* ثابت refp = VN_CAST(nodep->lhsp(), VarRef)) {
            getEntry(refp).assignWhole(getAssignp(refp), ++m_assignStep);
            بازگشت;
        }

        // If LHS is a known word reference, track the word
        اگر (ثابت AstWordSel* ثابت selp = VN_CAST(nodep->lhsp(), WordSel)) {
            اگر (AstVarRef* ثابت refp = VN_CAST(selp->fromp(), VarRef)) {
                // Simplify the index
                simplify(selp->bitp());
                اگر (ثابت AstConst* ثابت idxp = VN_CAST(selp->bitp(), Const)) {
                    getEntry(refp).assignWord(getAssignp(refp), ++m_assignStep, idxp->toUInt());
                    بازگشت;
                }
            }
        }

        // Not tracked, iterate LHS to simplify/reset tracking
        iterate(nodep->lhsp());
    }

    تهی visit(AstWordSel* nodep) override {
        اگر (!m_funcp) بازگشت;

        // Simplify the index
        simplify(nodep->bitp());

        // If this is a known word reference, track/substitute it
        اگر (AstVarRef* ثابت refp = VN_CAST(nodep->fromp(), VarRef)) {
            اگر (ثابت AstConst* ثابت idxp = VN_CAST(nodep->bitp(), Const)) {
                SubstVarEntry& entry = getEntry(refp);
                ثابت uint32_t word = idxp->toUInt();

                // If it's a write, reset tracking as we don't know the assigned value,
                // otherwise we would have picked it up in visit(AstNodeAssign*)
                اگر (refp->access().isWriteOrRW()) {
                    entry.assignWord(nullptr, ++m_assignStep, word);
                    بازگشت;
                }

                // Otherwise it's a read,
                UASSERT_OBJ(refp->access().isReadOnly(), nodep, "Invalid access");

                // If it's a constant pool variable, substiute with the constant word
                AstVar* ثابت varp = refp->varp();
                اگر (varp->constPoolEntry()) {
                    AstConst* ثابت constp = VN_AS(varp->valuep(), Const);
                    ثابت uint32_t مقدار = constp->رقم().edataWord(word);
                    File_سطر* ثابت flp = nodep->fileline();
                    ++m_n_Const_واژگان_Reinlined;
                    substitute(nodep, جدید AstConst{flp, AstConst::SizedEData{}, مقدار});
                    بازگشت;
                }

                // Substitute other variables if possible
                اگر (isSubstitutable(refp->varp())) {
                    اگر (Ast_گره_عبارت* ثابت substp = entry.substWord(word)) {
                        ++m_nWordSubstituted;
                        substitute(nodep, substp);
                        بازگشت;
                    }
                }

                // If not substituted, mark the assignment setting this word as used
                entry.usedWord(word);
                بازگشت;
            }
        }

        // If not a known word reference, iterate fromp to simplify/reset tracking
        iterate(nodep->fromp());
    }

    تهی visit(AstVarRef* nodep) override {
        اگر (!m_funcp) بازگشت;

        SubstVarEntry& entry = getEntry(nodep);

        // If it's a write, reset tracking as we don't know the assigned value,
        // otherwise we would have picked it up in visit(AstNodeAssign*)
        اگر (nodep->access().isWriteOrRW()) {
            entry.assignWhole(nullptr, ++m_assignStep);
            بازگشت;
        }

        // Otherwise it's a read, substitute it if possible
        UASSERT_OBJ(nodep->access().isReadOnly(), nodep, "Invalid access");
        اگر (isSubstitutable(nodep->varp())) {
            اگر (Ast_گره_عبارت* ثابت substp = entry.substWhole()) {
                // Do not substitute a compound wide expression.
                // The whole point of adding temporaries is to eliminate them.
                اگر (!nodep->isWide() || VN_IS(substp, VarRef)) {
                    ++m_nWholeSubstituted;
                    substitute(nodep, substp);
                    بازگشت;
                }
            }
        }

        // If not substituted, mark the assignment setting this variable as used
        entry.usedWhole();
    }

    تهی visit(Ast_گره_عبارت* nodep) override {
        اگر (!m_funcp) بازگشت;

        // Count nodes as we descend to track complexity
        ++m_ops;
        // First iterate children, this can cache child purity
        iterateChildren(nodep);
        // Do not substitute impure expressions
        اگر (!nodep->isPure()) m_ops = SUBST_MAX_OPS_NA;
    }

    تهی visit(AstVar*) override {}  // Accelerate
    تهی visit(AstConst*) override {}  // Accelerate

    تهی visit(Ast_گره* nodep) override { iterateChildren(nodep); }

عمومی:
    // CONSTRUCTORS
    explicit SubstVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~SubstVisitor() override {
        V3Stats::addStat("Optimizations, Subst, Substituted temps", m_nSubst);
        V3Stats::addStat("Optimizations, Subst, Constant words reinlined", m_n_Const_واژگان_Reinlined);
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

تهی V3Subst::substituteAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { SubstVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_درخت("subst", 0, dump_درخت_Either_Level() >= 3);
}
