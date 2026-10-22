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
    मित्र वर्ग SubstValidVisitor;

    // SubstVarEntry contains a Record for each word, and one extra for the whole variable
    संरचना Record final {
        // MEMBERS
        Ast_गाठ_Assign* m_assignp;  // Last assignment to this part
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
    AstVar* स्थिरांक m_varp;
    // The recrod for whole variable tracking
    Record m_wholeRecord;
    // A record for each word in the variable
    std::vector<Record> m_wordRecords{static_cast<माप_प्रकार>(m_varp->width_शब्द()), Record{}};

    // METHDOS
    शून्य deleteAssignmentIfUnused(Record& record, माप_प्रकार& nAssignDeleted) {
        जर (!record.m_assignp) परत;
        जर (record.m_used) परत;
        ++nAssignDeleted;
        VL_DO_DANGLING(record.m_assignp->unlinkFrBack()->delete_वृक्ष(), record.m_assignp);
    }

    Ast_गाठ_अभिव्यक्ती* substRecord(Record& record);

सार्वजनिक:
    // CONSTRUCTORS
    explicit SubstVarEntry(AstVar* varp)
        : m_varp{varp} {}
    ~SubstVarEntry() = पूर्वनिर्धारित;

    // Record assignment of whole variable. The given 'assp' can be null, which means
    // the variable is known to be assigned, but to an unknown value.
    शून्य assignWhole(Ast_गाठ_Assign* assp, uint32_t step) {
        // Invalidate all word records
        साठी (Record& wordRecord : m_wordRecords) wordRecord.invalidate();
        // Set whole record
        m_wholeRecord.m_assignp = assp;
        m_wholeRecord.m_step = step;
        m_wholeRecord.m_used = असत्य;
    }

    // Like assignWhole word, but records assignment to a specific word of the variable.
    शून्य assignWord(Ast_गाठ_Assign* assp, uint32_t step, uint32_t word) {
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
        साठी (Record& wordRecord : m_wordRecords) wordRecord.m_used = सत्य;
    }

    // Mark the specific word as used (value consumed)
    शून्य usedWord(uint32_t word) {
        m_wholeRecord.m_used = सत्य;
        m_wordRecords[word].m_used = सत्य;
    }

    // Returns substitution of whole word, or nullptr if not known/stale
    Ast_गाठ_अभिव्यक्ती* substWhole() { परत substRecord(m_wholeRecord); }
    // Returns substitution of whole word, or nullptr if not known/stale
    Ast_गाठ_अभिव्यक्ती* substWord(uint32_t word) { परत substRecord(m_wordRecords[word]); }

    शून्य deleteUnusedAssignments(माप_प्रकार& nWordAssignDeleted, माप_प्रकार& nWholeAssignDeleted) {
        // Delete assignments to temporaries if they are not used
        जर (!m_varp->isStatementTemp()) परत;
        साठी (Record& wordRecord : m_wordRecords)
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
    स्थिरांक uint32_t m_step;  // Step number where assignment was recorded
    तार्किक m_valid = सत्य;  // Is the expression we are considering to substitute valid

    // METHODS
    SubstVarEntry& getEntry(AstVarRef* nodep) {
        // This vistor is always invoked on the RHS of an assignment we are considering to
        // substitute. Variable references must have all been recorded when visiting the
        // assignments RHS, so the SubstVarEntry must exist for each referenced variable.
        परत *nodep->varp()->user1u().to<SubstVarEntry*>();
    }

    // VISITORS
    शून्य visit(AstWordSel* nodep) override {
        जर (!m_valid) परत;

        जर (AstVarRef* स्थिरांक refp = VN_CAST(nodep->fromp(), VarRef)) {
            जर (AstConst* स्थिरांक idxp = VN_CAST(nodep->bitp(), Const)) {
                SubstVarEntry& entry = getEntry(refp);
                // If either the whole variable, or the indexed word was written to
                // after the original assignment was recorded, the value is invalid.
                जर (m_step < entry.m_wholeRecord.m_step) m_valid = असत्य;
                जर (m_step < entry.m_wordRecords[idxp->toUInt()].m_step) m_valid = असत्य;
                परत;
            }
        }
        iterateChildrenConst(nodep);
    }

    शून्य visit(AstVarRef* nodep) override {
        जर (!m_valid) परत;

        // If either the whole variable, or any of the words were written to
        // after the original assignment was recorded, the value is invalid.
        स्थिरांक SubstVarEntry& entry = getEntry(nodep);
        जर (m_step < entry.m_wholeRecord.m_step) {
            m_valid = असत्य;
            परत;
        }
        साठी (स्थिरांक SubstVarEntry::Record& wordRecord : entry.m_wordRecords) {
            जर (m_step < wordRecord.m_step) {
                m_valid = असत्य;
                परत;
            }
        }
    }

    शून्य visit(AstConst*) override {}  // Accelerate
    शून्य visit(AstText*) override {}  // CExpr/CStmt literal text has no variable dependencies

    शून्य visit(Ast_गाठ_अभिव्यक्ती* nodep) override {
        जर (!m_valid) परत;
        iterateChildrenConst(nodep);
    }

    शून्य visit(Ast_गाठ* nodep) override { nodep->v3fatalSrc("Non AstNodeExpr under AstNodeExpr"); }

    // CONSTRUCTORS
    explicit SubstValidVisitor(SubstVarEntry::Record& record)
        : m_step{record.m_step} {
        iterateConst(record.m_assignp->rhsp());
    }
    ~SubstValidVisitor() override = पूर्वनिर्धारित;

सार्वजनिक:
    स्थिर तार्किक valid(SubstVarEntry::Record& record) {
        जर (!record.m_assignp) परत असत्य;
        परत SubstValidVisitor{record}.m_valid;
    }
};

Ast_गाठ_अभिव्यक्ती* SubstVarEntry::substRecord(SubstVarEntry::Record& record) {
    जर (!SubstValidVisitor::valid(record)) परत nullptr;
    Ast_गाठ_अभिव्यक्ती* स्थिरांक rhsp = record.m_assignp->rhsp();
    UDEBUGONLY(UASSERT_OBJ(rhsp->isPure(), record.m_assignp, "Substituting impure expression"););
    परत rhsp;
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
    स्थिरांक AstCFunc* m_funcp = nullptr;  // Current function we are under
    माप_प्रकार m_nSubst = 0;  // Number of substitutions performed - for avoiding constant folding
    // Statistics
    माप_प्रकार m_nWordSubstituted = 0;  // Number of words substituted
    माप_प्रकार m_nWholeSubstituted = 0;  // Number of whole variables substituted
    माप_प्रकार m_nWordAssignDeleted = 0;  // Number of word assignments deleted
    माप_प्रकार m_nWholeAssignDeleted = 0;  // Number of whole variable assignments deleted
    माप_प्रकार m_n_Const_शब्द_Reinlined = 0;  // Number of constant words substituted

    स्थिर constexpr uint32_t SUBST_MAX_OPS_SUBST = 30;  // Maximum number of ops to substitute in
    स्थिर constexpr uint32_t SUBST_MAX_OPS_NA = 9999;  // Not allowed to substitute

    // METHODS
    SubstVarEntry& getEntry(AstVarRef* nodep) {
        AstVar* स्थिरांक varp = nodep->varp();
        जर (!varp->user1p()) {
            m_entries.emplace_back(varp);
            varp->user1p(&m_entries.back());
        }
        परत *varp->user1u().to<SubstVarEntry*>();
    }

    शून्य simplify(Ast_गाठ_अभिव्यक्ती* exprp) {
        // Often constant, so short circuit
        जर (VN_IS(exprp, Const)) परत;
        // Iterate expression, then constant fold it if anything changed
        स्थिरांक माप_प्रकार nSubstBefore = m_nSubst;
        exprp = VN_AS(iterateSubtreeReturnEdits(exprp), गाठ_अभिव्यक्ती);
        जर (nSubstBefore != m_nSubst) V3Const::constifyEditCpp(exprp);
    }

    // The way the analysis algorithm works based on statement numbering only works
    // for variables that are written in the same basic block (ignoring AstCond) as
    // where they are consumed. The temporaries introduced in V3Premit are such, so
    // only substitute those for now.
    स्थिर तार्किक isSubstitutable(AstVar* nodep) {
        परत nodep->isStatementTemp() && !nodep->noSubst();
    }

    शून्य substitute(Ast_गाठ* nodep, Ast_गाठ_अभिव्यक्ती* substp) {
        Ast_गाठ_अभिव्यक्ती* newp = substp->backp() ? substp->clone_वृक्ष_Pure(सत्य) : substp;
        जर (!nodep->isQuad() && newp->isQuad()) {
            newp = नवीन AstCCast{newp->fileline(), newp, nodep};
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
            स्थिरांक VNUser1InUse m_inuser1;
            iterateChildren(nodep);
            // Deletes unused assignments and clear entries
            साठी (SubstVarEntry& entry : m_entries) {
                entry.deleteUnusedAssignments(m_nWordAssignDeleted, m_nWholeAssignDeleted);
            }
            m_entries.clear();
        }

        // Constant fold here, as Ast size can likely be reduced
        जर (v3Global.opt.fConstEager()) V3Const::constifyEditCpp(nodep);
    }

    शून्य visit(Ast_गाठ_Assign* nodep) override {
        जर (!m_funcp) परत;

        स्थिरांक uint32_t ops = [&]() {
            VL_RESTORER(m_ops);
            m_ops = 0;
            // Simplify the RHS
            simplify(nodep->rhsp());
            // If the became constant, we can continue substituting it
            परत VN_IS(nodep->rhsp(), Const) ? 0 : m_ops;
        }();

        // Assignment that defines the value, which is either this 'nodep',
        // or nullptr, if the value should not be substituted.
        स्थिरांक स्वतः getAssignp = [&](AstVarRef* refp) -> Ast_गाठ_Assign* {
            // If too complex, don't substitute
            जर (ops > SUBST_MAX_OPS_SUBST) परत nullptr;
            // AstCvtPackedToArray can't be anywhere else than on the RHS of assignment
            जर (VN_IS(nodep->rhsp(), CvtPackedToArray)) परत nullptr;
            // If non-const but want const subtitutions only
            जर (refp->varp()->substConstOnly() && !VN_IS(nodep->rhsp(), Const)) परत nullptr;
            // Otherwise can substitute based on the assignment
            परत nodep;
        };

        // If LHS is a whole variable reference, track the whole variable
        जर (AstVarRef* स्थिरांक refp = VN_CAST(nodep->lhsp(), VarRef)) {
            getEntry(refp).assignWhole(getAssignp(refp), ++m_assignStep);
            परत;
        }

        // If LHS is a known word reference, track the word
        जर (स्थिरांक AstWordSel* स्थिरांक selp = VN_CAST(nodep->lhsp(), WordSel)) {
            जर (AstVarRef* स्थिरांक refp = VN_CAST(selp->fromp(), VarRef)) {
                // Simplify the index
                simplify(selp->bitp());
                जर (स्थिरांक AstConst* स्थिरांक idxp = VN_CAST(selp->bitp(), Const)) {
                    getEntry(refp).assignWord(getAssignp(refp), ++m_assignStep, idxp->toUInt());
                    परत;
                }
            }
        }

        // Not tracked, iterate LHS to simplify/reset tracking
        iterate(nodep->lhsp());
    }

    शून्य visit(AstWordSel* nodep) override {
        जर (!m_funcp) परत;

        // Simplify the index
        simplify(nodep->bitp());

        // If this is a known word reference, track/substitute it
        जर (AstVarRef* स्थिरांक refp = VN_CAST(nodep->fromp(), VarRef)) {
            जर (स्थिरांक AstConst* स्थिरांक idxp = VN_CAST(nodep->bitp(), Const)) {
                SubstVarEntry& entry = getEntry(refp);
                स्थिरांक uint32_t word = idxp->toUInt();

                // If it's a write, reset tracking as we don't know the assigned value,
                // otherwise we would have picked it up in visit(AstNodeAssign*)
                जर (refp->access().isWriteOrRW()) {
                    entry.assignWord(nullptr, ++m_assignStep, word);
                    परत;
                }

                // Otherwise it's a read,
                UASSERT_OBJ(refp->access().isReadOnly(), nodep, "Invalid access");

                // If it's a constant pool variable, substiute with the constant word
                AstVar* स्थिरांक varp = refp->varp();
                जर (varp->constPoolEntry()) {
                    AstConst* स्थिरांक constp = VN_AS(varp->valuep(), Const);
                    स्थिरांक uint32_t मूल्य = constp->अंक().edataWord(word);
                    File_ओळ* स्थिरांक flp = nodep->fileline();
                    ++m_n_Const_शब्द_Reinlined;
                    substitute(nodep, नवीन AstConst{flp, AstConst::SizedEData{}, मूल्य});
                    परत;
                }

                // Substitute other variables if possible
                जर (isSubstitutable(refp->varp())) {
                    जर (Ast_गाठ_अभिव्यक्ती* स्थिरांक substp = entry.substWord(word)) {
                        ++m_nWordSubstituted;
                        substitute(nodep, substp);
                        परत;
                    }
                }

                // If not substituted, mark the assignment setting this word as used
                entry.usedWord(word);
                परत;
            }
        }

        // If not a known word reference, iterate fromp to simplify/reset tracking
        iterate(nodep->fromp());
    }

    शून्य visit(AstVarRef* nodep) override {
        जर (!m_funcp) परत;

        SubstVarEntry& entry = getEntry(nodep);

        // If it's a write, reset tracking as we don't know the assigned value,
        // otherwise we would have picked it up in visit(AstNodeAssign*)
        जर (nodep->access().isWriteOrRW()) {
            entry.assignWhole(nullptr, ++m_assignStep);
            परत;
        }

        // Otherwise it's a read, substitute it if possible
        UASSERT_OBJ(nodep->access().isReadOnly(), nodep, "Invalid access");
        जर (isSubstitutable(nodep->varp())) {
            जर (Ast_गाठ_अभिव्यक्ती* स्थिरांक substp = entry.substWhole()) {
                // Do not substitute a compound wide expression.
                // The whole point of adding temporaries is to eliminate them.
                जर (!nodep->isWide() || VN_IS(substp, VarRef)) {
                    ++m_nWholeSubstituted;
                    substitute(nodep, substp);
                    परत;
                }
            }
        }

        // If not substituted, mark the assignment setting this variable as used
        entry.usedWhole();
    }

    शून्य visit(Ast_गाठ_अभिव्यक्ती* nodep) override {
        जर (!m_funcp) परत;

        // Count nodes as we descend to track complexity
        ++m_ops;
        // First iterate children, this can cache child purity
        iterateChildren(nodep);
        // Do not substitute impure expressions
        जर (!nodep->isPure()) m_ops = SUBST_MAX_OPS_NA;
    }

    शून्य visit(AstVar*) override {}  // Accelerate
    शून्य visit(AstConst*) override {}  // Accelerate

    शून्य visit(Ast_गाठ* nodep) override { iterateChildren(nodep); }

सार्वजनिक:
    // CONSTRUCTORS
    explicit SubstVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~SubstVisitor() override {
        V3Stats::addStat("Optimizations, Subst, Substituted temps", m_nSubst);
        V3Stats::addStat("Optimizations, Subst, Constant words reinlined", m_n_Const_शब्द_Reinlined);
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
