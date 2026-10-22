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

#अन्तर्भाव "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#अन्तर्भाव "V3Subst.h"

#अन्तर्भाव "V3Const.h"
#अन्तर्भाव "V3Stats.h"

#अन्तर्भाव <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Data strored for every variable that tracks assignments and usage

वर्ग SubstVarEntry final {
    friend वर्ग SubstValidVisitor;

    // SubstVarEntry contains a Record for each word, and one extra for the whole variable
    संरचना Record final {
        // MEMBERS
        Ast_गाँठ_Assign* m_assignp;  // Last assignment to this part
        uint32_t m_step;  // Step number of last assignment. 0 means invalid entry.
        तार्किक m_used;  // True if consumed. Can be set even if entry is invalid.
        // CONSTRUCTOR
        Record() { invalidate(); }
        // METHODS
        शून्य invalidate() {
            m_assignp = nullptr;
            m_step = 0;
            m_used = असत्य;
        }
    };

    // MEMBERS
    // Variable this SubstVarEntry tracks
    AstVar* ध्रुव m_varp;
    // The recrod for whole variable tracking
    Record m_wholeRecord;
    // A record for each word in the variable
    std::vector<Record> m_wordRecords{static_cast<size_t>(m_varp->width_शब्दगण()), Record{}};

    // METHDOS
    शून्य deleteAssignmentIfUnused(Record& record, size_t& nAssignDeleted) {
        यदि (!record.m_assignp) प्रत्यागम;
        यदि (record.m_used) प्रत्यागम;
        ++nAssignDeleted;
        VL_DO_DANGLING(record.m_assignp->unlinkFrBack()->delete_वृक्ष(), record.m_assignp);
    }

    Ast_गाँठ_व्यंजक* substRecord(Record& record);

सार्वजनिक:
    // CONSTRUCTORS
    explicit SubstVarEntry(AstVar* varp)
        : m_varp{varp} {}
    ~SubstVarEntry() = स्वभाव;

    // Record assignment of whole variable. The given 'assp' can be null, which means
    // the variable is known to be assigned, but to an unknown value.
    शून्य assignWhole(Ast_गाँठ_Assign* assp, uint32_t step) {
        // Invalidate all word records
        प्रति (Record& wordRecord : m_wordRecords) wordRecord.invalidate();
        // Set whole record
        m_wholeRecord.m_assignp = assp;
        m_wholeRecord.m_step = step;
        m_wholeRecord.m_used = असत्य;
    }

    // Like assignWhole word, but records assignment to a specific word of the variable.
    शून्य assignWord(Ast_गाँठ_Assign* assp, uint32_t step, uint32_t word) {
        // Invalidate whole record
        m_wholeRecord.invalidate();
        // Set word record
        Record& wordRecord = m_wordRecords[word];
        wordRecord.m_assignp = assp;
        wordRecord.m_step = step;
        wordRecord.m_used = असत्य;
    }

    // Mark the whole variable as used (value consumed)
    शून्य usedWhole() {
        m_wholeRecord.m_used = सत्य;
        प्रति (Record& wordRecord : m_wordRecords) wordRecord.m_used = सत्य;
    }

    // Mark the specific word as used (value consumed)
    शून्य usedWord(uint32_t word) {
        m_wholeRecord.m_used = सत्य;
        m_wordRecords[word].m_used = सत्य;
    }

    // Returns substitution of whole word, or nullptr if not known/stale
    Ast_गाँठ_व्यंजक* substWhole() { प्रत्यागम substRecord(m_wholeRecord); }
    // Returns substitution of whole word, or nullptr if not known/stale
    Ast_गाँठ_व्यंजक* substWord(uint32_t word) { प्रत्यागम substRecord(m_wordRecords[word]); }

    शून्य deleteUnusedAssignments(size_t& nWordAssignDeleted, size_t& nWholeAssignDeleted) {
        // Delete assignments to temporaries if they are not used
        यदि (!m_varp->isStatementTemp()) प्रत्यागम;
        प्रति (Record& wordRecord : m_wordRecords)
            deleteAssignmentIfUnused(wordRecord, nWordAssignDeleted);
        deleteAssignmentIfUnused(m_wholeRecord, nWholeAssignDeleted);
    }
};

//######################################################################
// See if any variables in the expression we are considering to
// substitute have changed value since we recorded the assignment.

वर्ग SubstValidVisitor final : सार्वजनिक VNVisitorConst {
    // NODE STATE
    // See SubstVisitor

    // STATE
    ध्रुव uint32_t m_step;  // Step number where assignment was recorded
    तार्किक m_valid = सत्य;  // Is the expression we are considering to substitute valid

    // METHODS
    SubstVarEntry& getEntry(AstVarRef* nodep) {
        // This vistor is always invoked on the RHS of an assignment we are considering to
        // substitute. Variable references must have all been recorded when visiting the
        // assignments RHS, so the SubstVarEntry must exist for each referenced variable.
        प्रत्यागम *nodep->varp()->user1u().to<SubstVarEntry*>();
    }

    // VISITORS
    शून्य visit(AstWordSel* nodep) override {
        यदि (!m_valid) प्रत्यागम;

        यदि (AstVarRef* ध्रुव refp = VN_CAST(nodep->fromp(), VarRef)) {
            यदि (AstConst* ध्रुव idxp = VN_CAST(nodep->bitp(), Const)) {
                SubstVarEntry& entry = getEntry(refp);
                // If either the whole variable, or the indexed word was written to
                // after the original assignment was recorded, the value is invalid.
                यदि (m_step < entry.m_wholeRecord.m_step) m_valid = असत्य;
                यदि (m_step < entry.m_wordRecords[idxp->toUInt()].m_step) m_valid = असत्य;
                प्रत्यागम;
            }
        }
        iterateChildrenConst(nodep);
    }

    शून्य visit(AstVarRef* nodep) override {
        यदि (!m_valid) प्रत्यागम;

        // If either the whole variable, or any of the words were written to
        // after the original assignment was recorded, the value is invalid.
        ध्रुव SubstVarEntry& entry = getEntry(nodep);
        यदि (m_step < entry.m_wholeRecord.m_step) {
            m_valid = असत्य;
            प्रत्यागम;
        }
        प्रति (ध्रुव SubstVarEntry::Record& wordRecord : entry.m_wordRecords) {
            यदि (m_step < wordRecord.m_step) {
                m_valid = असत्य;
                प्रत्यागम;
            }
        }
    }

    शून्य visit(AstConst*) override {}  // Accelerate
    शून्य visit(AstText*) override {}  // CExpr/CStmt literal text has no variable dependencies

    शून्य visit(Ast_गाँठ_व्यंजक* nodep) override {
        यदि (!m_valid) प्रत्यागम;
        iterateChildrenConst(nodep);
    }

    शून्य visit(Ast_गाँठ* nodep) override { nodep->v3fatalSrc("Non AstNodeExpr under AstNodeExpr"); }

    // CONSTRUCTORS
    explicit SubstValidVisitor(SubstVarEntry::Record& record)
        : m_step{record.m_step} {
        iterateConst(record.m_assignp->rhsp());
    }
    ~SubstValidVisitor() override = स्वभाव;

सार्वजनिक:
    स्थिर तार्किक valid(SubstVarEntry::Record& record) {
        यदि (!record.m_assignp) प्रत्यागम असत्य;
        प्रत्यागम SubstValidVisitor{record}.m_valid;
    }
};

Ast_गाँठ_व्यंजक* SubstVarEntry::substRecord(SubstVarEntry::Record& record) {
    यदि (!SubstValidVisitor::valid(record)) प्रत्यागम nullptr;
    Ast_गाँठ_व्यंजक* ध्रुव rhsp = record.m_assignp->rhsp();
    UDEBUGONLY(UASSERT_OBJ(rhsp->isPure(), record.m_assignp, "Substituting impure expression"););
    प्रत्यागम rhsp;
}

//######################################################################
// Substitution visitor

वर्ग SubstVisitor final : सार्वजनिक VNVisitor {
    // NODE STATE - only Under AstCFunc
    // AstVar::user1p -> SubstVarEntry* for assignment tracking. Also used by SubstValidVisitor

    // STATE
    std::deque<SubstVarEntry> m_entries;  // Storage for SubstVarEntry instances
    uint32_t m_ops = 0;  // Number of nodes on the RHS of an assignment
    uint32_t m_assignStep = 0;  // Assignment number to determine variable lifetimes
    ध्रुव AstCFunc* m_funcp = nullptr;  // Current function we are under
    size_t m_nSubst = 0;  // Number of substitutions performed - for avoiding constant folding
    // Statistics
    size_t m_nWordSubstituted = 0;  // Number of words substituted
    size_t m_nWholeSubstituted = 0;  // Number of whole variables substituted
    size_t m_nWordAssignDeleted = 0;  // Number of word assignments deleted
    size_t m_nWholeAssignDeleted = 0;  // Number of whole variable assignments deleted
    size_t m_n_Const_शब्दगण_Reinlined = 0;  // Number of constant words substituted

    स्थिर constexpr uint32_t SUBST_MAX_OPS_SUBST = 30;  // Maximum number of ops to substitute in
    स्थिर constexpr uint32_t SUBST_MAX_OPS_NA = 9999;  // Not allowed to substitute

    // METHODS
    SubstVarEntry& getEntry(AstVarRef* nodep) {
        AstVar* ध्रुव varp = nodep->varp();
        यदि (!varp->user1p()) {
            m_entries.emplace_back(varp);
            varp->user1p(&m_entries.back());
        }
        प्रत्यागम *varp->user1u().to<SubstVarEntry*>();
    }

    शून्य simplify(Ast_गाँठ_व्यंजक* exprp) {
        // Often constant, so short circuit
        यदि (VN_IS(exprp, Const)) प्रत्यागम;
        // Iterate expression, then constant fold it if anything changed
        ध्रुव size_t nSubstBefore = m_nSubst;
        exprp = VN_AS(iterateSubtreeReturnEdits(exprp), गाँठ_व्यंजक);
        यदि (nSubstBefore != m_nSubst) V3Const::constifyEditCpp(exprp);
    }

    // The way the analysis algorithm works based on statement numbering only works
    // for variables that are written in the same basic block (ignoring AstCond) as
    // where they are consumed. The temporaries introduced in V3Premit are such, so
    // only substitute those for now.
    स्थिर तार्किक isSubstitutable(AstVar* nodep) {
        प्रत्यागम nodep->isStatementTemp() && !nodep->noSubst();
    }

    शून्य substitute(Ast_गाँठ* nodep, Ast_गाँठ_व्यंजक* substp) {
        Ast_गाँठ_व्यंजक* newp = substp->backp() ? substp->clone_वृक्ष_Pure(सत्य) : substp;
        यदि (!nodep->isQuad() && newp->isQuad()) {
            newp = नूतन AstCCast{newp->fileline(), newp, nodep};
        }
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_वृक्ष(), nodep);
        ++m_nSubst;
    }

    // VISITORS

    शून्य visit(AstNetlist* nodep) override { iterateAndNextNull(nodep->modulesp()); }

    शून्य visit(AstCFunc* nodep) override {
        UASSERT_OBJ(!m_funcp, nodep, "Should not nest");
        UASSERT_OBJ(m_entries.empty(), nodep, "Should not visit outside functions");

        // Process the function body
        {
            VL_RESTORER(m_funcp);
            m_funcp = nodep;
            ध्रुव VNUser1InUse m_inuser1;
            iterateChildren(nodep);
            // Deletes unused assignments and clear entries
            प्रति (SubstVarEntry& entry : m_entries) {
                entry.deleteUnusedAssignments(m_nWordAssignDeleted, m_nWholeAssignDeleted);
            }
            m_entries.clear();
        }

        // Constant fold here, as Ast size can likely be reduced
        यदि (v3Global.opt.fConstEager()) V3Const::constifyEditCpp(nodep);
    }

    शून्य visit(Ast_गाँठ_Assign* nodep) override {
        यदि (!m_funcp) प्रत्यागम;

        ध्रुव uint32_t ops = [&]() {
            VL_RESTORER(m_ops);
            m_ops = 0;
            // Simplify the RHS
            simplify(nodep->rhsp());
            // If the became constant, we can continue substituting it
            प्रत्यागम VN_IS(nodep->rhsp(), Const) ? 0 : m_ops;
        }();

        // Assignment that defines the value, which is either this 'nodep',
        // or nullptr, if the value should not be substituted.
        ध्रुव auto getAssignp = [&](AstVarRef* refp) -> Ast_गाँठ_Assign* {
            // If too complex, don't substitute
            यदि (ops > SUBST_MAX_OPS_SUBST) प्रत्यागम nullptr;
            // AstCvtPackedToArray can't be anywhere else than on the RHS of assignment
            यदि (VN_IS(nodep->rhsp(), CvtPackedToArray)) प्रत्यागम nullptr;
            // If non-const but want const subtitutions only
            यदि (refp->varp()->substConstOnly() && !VN_IS(nodep->rhsp(), Const)) प्रत्यागम nullptr;
            // Otherwise can substitute based on the assignment
            प्रत्यागम nodep;
        };

        // If LHS is a whole variable reference, track the whole variable
        यदि (AstVarRef* ध्रुव refp = VN_CAST(nodep->lhsp(), VarRef)) {
            getEntry(refp).assignWhole(getAssignp(refp), ++m_assignStep);
            प्रत्यागम;
        }

        // If LHS is a known word reference, track the word
        यदि (ध्रुव AstWordSel* ध्रुव selp = VN_CAST(nodep->lhsp(), WordSel)) {
            यदि (AstVarRef* ध्रुव refp = VN_CAST(selp->fromp(), VarRef)) {
                // Simplify the index
                simplify(selp->bitp());
                यदि (ध्रुव AstConst* ध्रुव idxp = VN_CAST(selp->bitp(), Const)) {
                    getEntry(refp).assignWord(getAssignp(refp), ++m_assignStep, idxp->toUInt());
                    प्रत्यागम;
                }
            }
        }

        // Not tracked, iterate LHS to simplify/reset tracking
        iterate(nodep->lhsp());
    }

    शून्य visit(AstWordSel* nodep) override {
        यदि (!m_funcp) प्रत्यागम;

        // Simplify the index
        simplify(nodep->bitp());

        // If this is a known word reference, track/substitute it
        यदि (AstVarRef* ध्रुव refp = VN_CAST(nodep->fromp(), VarRef)) {
            यदि (ध्रुव AstConst* ध्रुव idxp = VN_CAST(nodep->bitp(), Const)) {
                SubstVarEntry& entry = getEntry(refp);
                ध्रुव uint32_t word = idxp->toUInt();

                // If it's a write, reset tracking as we don't know the assigned value,
                // otherwise we would have picked it up in visit(AstNodeAssign*)
                यदि (refp->access().isWriteOrRW()) {
                    entry.assignWord(nullptr, ++m_assignStep, word);
                    प्रत्यागम;
                }

                // Otherwise it's a read,
                UASSERT_OBJ(refp->access().isReadOnly(), nodep, "Invalid access");

                // If it's a constant pool variable, substiute with the constant word
                AstVar* ध्रुव varp = refp->varp();
                यदि (varp->constPoolEntry()) {
                    AstConst* ध्रुव constp = VN_AS(varp->valuep(), Const);
                    ध्रुव uint32_t मूल्य = constp->अंक().edataWord(word);
                    File_पंक्ति* ध्रुव flp = nodep->fileline();
                    ++m_n_Const_शब्दगण_Reinlined;
                    substitute(nodep, नूतन AstConst{flp, AstConst::SizedEData{}, मूल्य});
                    प्रत्यागम;
                }

                // Substitute other variables if possible
                यदि (isSubstitutable(refp->varp())) {
                    यदि (Ast_गाँठ_व्यंजक* ध्रुव substp = entry.substWord(word)) {
                        ++m_nWordSubstituted;
                        substitute(nodep, substp);
                        प्रत्यागम;
                    }
                }

                // If not substituted, mark the assignment setting this word as used
                entry.usedWord(word);
                प्रत्यागम;
            }
        }

        // If not a known word reference, iterate fromp to simplify/reset tracking
        iterate(nodep->fromp());
    }

    शून्य visit(AstVarRef* nodep) override {
        यदि (!m_funcp) प्रत्यागम;

        SubstVarEntry& entry = getEntry(nodep);

        // If it's a write, reset tracking as we don't know the assigned value,
        // otherwise we would have picked it up in visit(AstNodeAssign*)
        यदि (nodep->access().isWriteOrRW()) {
            entry.assignWhole(nullptr, ++m_assignStep);
            प्रत्यागम;
        }

        // Otherwise it's a read, substitute it if possible
        UASSERT_OBJ(nodep->access().isReadOnly(), nodep, "Invalid access");
        यदि (isSubstitutable(nodep->varp())) {
            यदि (Ast_गाँठ_व्यंजक* ध्रुव substp = entry.substWhole()) {
                // Do not substitute a compound wide expression.
                // The whole point of adding temporaries is to eliminate them.
                यदि (!nodep->isWide() || VN_IS(substp, VarRef)) {
                    ++m_nWholeSubstituted;
                    substitute(nodep, substp);
                    प्रत्यागम;
                }
            }
        }

        // If not substituted, mark the assignment setting this variable as used
        entry.usedWhole();
    }

    शून्य visit(Ast_गाँठ_व्यंजक* nodep) override {
        यदि (!m_funcp) प्रत्यागम;

        // Count nodes as we descend to track complexity
        ++m_ops;
        // First iterate children, this can cache child purity
        iterateChildren(nodep);
        // Do not substitute impure expressions
        यदि (!nodep->isPure()) m_ops = SUBST_MAX_OPS_NA;
    }

    शून्य visit(AstVar*) override {}  // Accelerate
    शून्य visit(AstConst*) override {}  // Accelerate

    शून्य visit(Ast_गाँठ* nodep) override { iterateChildren(nodep); }

सार्वजनिक:
    // CONSTRUCTORS
    explicit SubstVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~SubstVisitor() override {
        V3Stats::addStat("Optimizations, Subst, Substituted temps", m_nSubst);
        V3Stats::addStat("Optimizations, Subst, Constant words reinlined", m_n_Const_शब्दगण_Reinlined);
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

शून्य V3Subst::substituteAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { SubstVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_वृक्ष("subst", 0, dump_वृक्ष_Either_Level() >= 3);
}
