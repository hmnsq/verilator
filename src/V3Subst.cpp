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

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "V3Subst.h"

#ضمن "V3Const.h"
#ضمن "V3Stats.h"

#ضمن <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Data strored for every variable that tracks assignments and usage

صنف SubstVarEntry final {
    friend صنف SubstValidVisitor;

    // SubstVarEntry contains a Record for each word, and one extra for the whole variable
    بنية Record final {
        // MEMBERS
        AstNodeAssign* m_assignp;  // Last assignment to this part
        uint32_t m_step;  // Step number of last assignment. 0 means invalid entry.
        منطقي m_used;  // True if consumed. Can be set even if entry is invalid.
        // CONSTRUCTOR
        Record() { invalidate(); }
        // METHODS
        فراغ invalidate() {
            m_assignp = nullptr;
            m_step = 0;
            m_used = خطأ;
        }
    };

    // MEMBERS
    // Variable this SubstVarEntry tracks
    AstVar* ثابت m_varp;
    // The recrod for whole variable tracking
    Record m_wholeRecord;
    // A record for each word in the variable
    std::vector<Record> m_wordRecords{static_cast<size_t>(m_varp->widthWords()), Record{}};

    // METHDOS
    فراغ deleteAssignmentIfUnused(Record& record, size_t& nAssignDeleted) {
        𐡄𐡍 (!record.m_assignp) أرجع;
        𐡄𐡍 (record.m_used) أرجع;
        ++nAssignDeleted;
        VL_DO_DANGLING(record.m_assignp->unlinkFrBack()->deleteTree(), record.m_assignp);
    }

    AstNodeExpr* substRecord(Record& record);

عام:
    // CONSTRUCTORS
    explicit SubstVarEntry(AstVar* varp)
        : m_varp{varp} {}
    ~SubstVarEntry() = افتراضي;

    // Record assignment of whole variable. The given 'assp' can be null, which means
    // the variable is known to be assigned, but to an unknown value.
    فراغ assignWhole(AstNodeAssign* assp, uint32_t step) {
        // Invalidate all word records
        لكل (Record& wordRecord : m_wordRecords) wordRecord.invalidate();
        // Set whole record
        m_wholeRecord.m_assignp = assp;
        m_wholeRecord.m_step = step;
        m_wholeRecord.m_used = خطأ;
    }

    // Like assignWhole word, but records assignment to a specific word of the variable.
    فراغ assignWord(AstNodeAssign* assp, uint32_t step, uint32_t word) {
        // Invalidate whole record
        m_wholeRecord.invalidate();
        // Set word record
        Record& wordRecord = m_wordRecords[word];
        wordRecord.m_assignp = assp;
        wordRecord.m_step = step;
        wordRecord.m_used = خطأ;
    }

    // Mark the whole variable as used (value consumed)
    فراغ usedWhole() {
        m_wholeRecord.m_used = 𐡒𐡔𐡈;
        لكل (Record& wordRecord : m_wordRecords) wordRecord.m_used = 𐡒𐡔𐡈;
    }

    // Mark the specific word as used (value consumed)
    فراغ usedWord(uint32_t word) {
        m_wholeRecord.m_used = 𐡒𐡔𐡈;
        m_wordRecords[word].m_used = 𐡒𐡔𐡈;
    }

    // Returns substitution of whole word, or nullptr if not known/stale
    AstNodeExpr* substWhole() { أرجع substRecord(m_wholeRecord); }
    // Returns substitution of whole word, or nullptr if not known/stale
    AstNodeExpr* substWord(uint32_t word) { أرجع substRecord(m_wordRecords[word]); }

    فراغ deleteUnusedAssignments(size_t& nWordAssignDeleted, size_t& nWholeAssignDeleted) {
        // Delete assignments to temporaries if they are not used
        𐡄𐡍 (!m_varp->isStatementTemp()) أرجع;
        لكل (Record& wordRecord : m_wordRecords)
            deleteAssignmentIfUnused(wordRecord, nWordAssignDeleted);
        deleteAssignmentIfUnused(m_wholeRecord, nWholeAssignDeleted);
    }
};

//######################################################################
// See if any variables in the expression we are considering to
// substitute have changed value since we recorded the assignment.

صنف SubstValidVisitor final : عام VNVisitorConst {
    // NODE STATE
    // See SubstVisitor

    // STATE
    ثابت uint32_t m_step;  // Step number where assignment was recorded
    منطقي m_valid = 𐡒𐡔𐡈;  // Is the expression we are considering to substitute valid

    // METHODS
    SubstVarEntry& getEntry(AstVarRef* nodep) {
        // This vistor is always invoked on the RHS of an assignment we are considering to
        // substitute. Variable references must have all been recorded when visiting the
        // assignments RHS, so the SubstVarEntry must exist for each referenced variable.
        أرجع *nodep->varp()->user1u().to<SubstVarEntry*>();
    }

    // VISITORS
    فراغ visit(AstWordSel* nodep) override {
        𐡄𐡍 (!m_valid) أرجع;

        𐡄𐡍 (AstVarRef* ثابت refp = VN_CAST(nodep->fromp(), VarRef)) {
            𐡄𐡍 (AstConst* ثابت idxp = VN_CAST(nodep->bitp(), Const)) {
                SubstVarEntry& entry = getEntry(refp);
                // If either the whole variable, or the indexed word was written to
                // after the original assignment was recorded, the value is invalid.
                𐡄𐡍 (m_step < entry.m_wholeRecord.m_step) m_valid = خطأ;
                𐡄𐡍 (m_step < entry.m_wordRecords[idxp->toUInt()].m_step) m_valid = خطأ;
                أرجع;
            }
        }
        iterateChildrenConst(nodep);
    }

    فراغ visit(AstVarRef* nodep) override {
        𐡄𐡍 (!m_valid) أرجع;

        // If either the whole variable, or any of the words were written to
        // after the original assignment was recorded, the value is invalid.
        ثابت SubstVarEntry& entry = getEntry(nodep);
        𐡄𐡍 (m_step < entry.m_wholeRecord.m_step) {
            m_valid = خطأ;
            أرجع;
        }
        لكل (ثابت SubstVarEntry::Record& wordRecord : entry.m_wordRecords) {
            𐡄𐡍 (m_step < wordRecord.m_step) {
                m_valid = خطأ;
                أرجع;
            }
        }
    }

    فراغ visit(AstConst*) override {}  // Accelerate
    فراغ visit(AstText*) override {}  // CExpr/CStmt literal text has no variable dependencies

    فراغ visit(AstNodeExpr* nodep) override {
        𐡄𐡍 (!m_valid) أرجع;
        iterateChildrenConst(nodep);
    }

    فراغ visit(AstNode* nodep) override { nodep->v3fatalSrc("Non AstNodeExpr under AstNodeExpr"); }

    // CONSTRUCTORS
    explicit SubstValidVisitor(SubstVarEntry::Record& record)
        : m_step{record.m_step} {
        iterateConst(record.m_assignp->rhsp());
    }
    ~SubstValidVisitor() override = افتراضي;

عام:
    ساكن منطقي valid(SubstVarEntry::Record& record) {
        𐡄𐡍 (!record.m_assignp) أرجع خطأ;
        أرجع SubstValidVisitor{record}.m_valid;
    }
};

AstNodeExpr* SubstVarEntry::substRecord(SubstVarEntry::Record& record) {
    𐡄𐡍 (!SubstValidVisitor::valid(record)) أرجع nullptr;
    AstNodeExpr* ثابت rhsp = record.m_assignp->rhsp();
    UDEBUGONLY(UASSERT_OBJ(rhsp->isPure(), record.m_assignp, "Substituting impure expression"););
    أرجع rhsp;
}

//######################################################################
// Substitution visitor

صنف SubstVisitor final : عام VNVisitor {
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
    size_t m_nConstWordsReinlined = 0;  // Number of constant words substituted

    ساكن constexpr uint32_t SUBST_MAX_OPS_SUBST = 30;  // Maximum number of ops to substitute in
    ساكن constexpr uint32_t SUBST_MAX_OPS_NA = 9999;  // Not allowed to substitute

    // METHODS
    SubstVarEntry& getEntry(AstVarRef* nodep) {
        AstVar* ثابت varp = nodep->varp();
        𐡄𐡍 (!varp->user1p()) {
            m_entries.emplace_back(varp);
            varp->user1p(&m_entries.back());
        }
        أرجع *varp->user1u().to<SubstVarEntry*>();
    }

    فراغ simplify(AstNodeExpr* exprp) {
        // Often constant, so short circuit
        𐡄𐡍 (VN_IS(exprp, Const)) أرجع;
        // Iterate expression, then constant fold it if anything changed
        ثابت size_t nSubstBefore = m_nSubst;
        exprp = VN_AS(iterateSubtreeReturnEdits(exprp), NodeExpr);
        𐡄𐡍 (nSubstBefore != m_nSubst) V3Const::constifyEditCpp(exprp);
    }

    // The way the analysis algorithm works based on statement numbering only works
    // for variables that are written in the same basic block (ignoring AstCond) as
    // where they are consumed. The temporaries introduced in V3Premit are such, so
    // only substitute those for now.
    ساكن منطقي isSubstitutable(AstVar* nodep) {
        أرجع nodep->isStatementTemp() && !nodep->noSubst();
    }

    فراغ substitute(AstNode* nodep, AstNodeExpr* substp) {
        AstNodeExpr* newp = substp->backp() ? substp->cloneTreePure(𐡒𐡔𐡈) : substp;
        𐡄𐡍 (!nodep->isQuad() && newp->isQuad()) {
            newp = جديد AstCCast{newp->fileline(), newp, nodep};
        }
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->deleteTree(), nodep);
        ++m_nSubst;
    }

    // VISITORS

    فراغ visit(AstNetlist* nodep) override { iterateAndNextNull(nodep->modulesp()); }

    فراغ visit(AstCFunc* nodep) override {
        UASSERT_OBJ(!m_funcp, nodep, "Should not nest");
        UASSERT_OBJ(m_entries.empty(), nodep, "Should not visit outside functions");

        // Process the function body
        {
            VL_RESTORER(m_funcp);
            m_funcp = nodep;
            ثابت VNUser1InUse m_inuser1;
            iterateChildren(nodep);
            // Deletes unused assignments and clear entries
            لكل (SubstVarEntry& entry : m_entries) {
                entry.deleteUnusedAssignments(m_nWordAssignDeleted, m_nWholeAssignDeleted);
            }
            m_entries.clear();
        }

        // Constant fold here, as Ast size can likely be reduced
        𐡄𐡍 (v3Global.opt.fConstEager()) V3Const::constifyEditCpp(nodep);
    }

    فراغ visit(AstNodeAssign* nodep) override {
        𐡄𐡍 (!m_funcp) أرجع;

        ثابت uint32_t ops = [&]() {
            VL_RESTORER(m_ops);
            m_ops = 0;
            // Simplify the RHS
            simplify(nodep->rhsp());
            // If the became constant, we can continue substituting it
            أرجع VN_IS(nodep->rhsp(), Const) ? 0 : m_ops;
        }();

        // Assignment that defines the value, which is either this 'nodep',
        // or nullptr, if the value should not be substituted.
        ثابت auto getAssignp = [&](AstVarRef* refp) -> AstNodeAssign* {
            // If too complex, don't substitute
            𐡄𐡍 (ops > SUBST_MAX_OPS_SUBST) أرجع nullptr;
            // AstCvtPackedToArray can't be anywhere else than on the RHS of assignment
            𐡄𐡍 (VN_IS(nodep->rhsp(), CvtPackedToArray)) أرجع nullptr;
            // If non-const but want const subtitutions only
            𐡄𐡍 (refp->varp()->substConstOnly() && !VN_IS(nodep->rhsp(), Const)) أرجع nullptr;
            // Otherwise can substitute based on the assignment
            أرجع nodep;
        };

        // If LHS is a whole variable reference, track the whole variable
        𐡄𐡍 (AstVarRef* ثابت refp = VN_CAST(nodep->lhsp(), VarRef)) {
            getEntry(refp).assignWhole(getAssignp(refp), ++m_assignStep);
            أرجع;
        }

        // If LHS is a known word reference, track the word
        𐡄𐡍 (ثابت AstWordSel* ثابت selp = VN_CAST(nodep->lhsp(), WordSel)) {
            𐡄𐡍 (AstVarRef* ثابت refp = VN_CAST(selp->fromp(), VarRef)) {
                // Simplify the index
                simplify(selp->bitp());
                𐡄𐡍 (ثابت AstConst* ثابت idxp = VN_CAST(selp->bitp(), Const)) {
                    getEntry(refp).assignWord(getAssignp(refp), ++m_assignStep, idxp->toUInt());
                    أرجع;
                }
            }
        }

        // Not tracked, iterate LHS to simplify/reset tracking
        iterate(nodep->lhsp());
    }

    فراغ visit(AstWordSel* nodep) override {
        𐡄𐡍 (!m_funcp) أرجع;

        // Simplify the index
        simplify(nodep->bitp());

        // If this is a known word reference, track/substitute it
        𐡄𐡍 (AstVarRef* ثابت refp = VN_CAST(nodep->fromp(), VarRef)) {
            𐡄𐡍 (ثابت AstConst* ثابت idxp = VN_CAST(nodep->bitp(), Const)) {
                SubstVarEntry& entry = getEntry(refp);
                ثابت uint32_t word = idxp->toUInt();

                // If it's a write, reset tracking as we don't know the assigned value,
                // otherwise we would have picked it up in visit(AstNodeAssign*)
                𐡄𐡍 (refp->access().isWriteOrRW()) {
                    entry.assignWord(nullptr, ++m_assignStep, word);
                    أرجع;
                }

                // Otherwise it's a read,
                UASSERT_OBJ(refp->access().isReadOnly(), nodep, "Invalid access");

                // If it's a constant pool variable, substiute with the constant word
                AstVar* ثابت varp = refp->varp();
                𐡄𐡍 (varp->constPoolEntry()) {
                    AstConst* ثابت constp = VN_AS(varp->valuep(), Const);
                    ثابت uint32_t value = constp->num().edataWord(word);
                    FileLine* ثابت flp = nodep->fileline();
                    ++m_nConstWordsReinlined;
                    substitute(nodep, جديد AstConst{flp, AstConst::SizedEData{}, value});
                    أرجع;
                }

                // Substitute other variables if possible
                𐡄𐡍 (isSubstitutable(refp->varp())) {
                    𐡄𐡍 (AstNodeExpr* ثابت substp = entry.substWord(word)) {
                        ++m_nWordSubstituted;
                        substitute(nodep, substp);
                        أرجع;
                    }
                }

                // If not substituted, mark the assignment setting this word as used
                entry.usedWord(word);
                أرجع;
            }
        }

        // If not a known word reference, iterate fromp to simplify/reset tracking
        iterate(nodep->fromp());
    }

    فراغ visit(AstVarRef* nodep) override {
        𐡄𐡍 (!m_funcp) أرجع;

        SubstVarEntry& entry = getEntry(nodep);

        // If it's a write, reset tracking as we don't know the assigned value,
        // otherwise we would have picked it up in visit(AstNodeAssign*)
        𐡄𐡍 (nodep->access().isWriteOrRW()) {
            entry.assignWhole(nullptr, ++m_assignStep);
            أرجع;
        }

        // Otherwise it's a read, substitute it if possible
        UASSERT_OBJ(nodep->access().isReadOnly(), nodep, "Invalid access");
        𐡄𐡍 (isSubstitutable(nodep->varp())) {
            𐡄𐡍 (AstNodeExpr* ثابت substp = entry.substWhole()) {
                // Do not substitute a compound wide expression.
                // The whole point of adding temporaries is to eliminate them.
                𐡄𐡍 (!nodep->isWide() || VN_IS(substp, VarRef)) {
                    ++m_nWholeSubstituted;
                    substitute(nodep, substp);
                    أرجع;
                }
            }
        }

        // If not substituted, mark the assignment setting this variable as used
        entry.usedWhole();
    }

    فراغ visit(AstNodeExpr* nodep) override {
        𐡄𐡍 (!m_funcp) أرجع;

        // Count nodes as we descend to track complexity
        ++m_ops;
        // First iterate children, this can cache child purity
        iterateChildren(nodep);
        // Do not substitute impure expressions
        𐡄𐡍 (!nodep->isPure()) m_ops = SUBST_MAX_OPS_NA;
    }

    فراغ visit(AstVar*) override {}  // Accelerate
    فراغ visit(AstConst*) override {}  // Accelerate

    فراغ visit(AstNode* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    explicit SubstVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~SubstVisitor() override {
        V3Stats::addStat("Optimizations, Subst, Substituted temps", m_nSubst);
        V3Stats::addStat("Optimizations, Subst, Constant words reinlined", m_nConstWordsReinlined);
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

فراغ V3Subst::substituteAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { SubstVisitor{nodep}; }  // Destruct before checking
    V3Global::dumpCheckGlobalTree("subst", 0, dumpTreeEitherLevel() >= 3);
}
