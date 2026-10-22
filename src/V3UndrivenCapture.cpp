// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Capture task/function write summaries for undriven checks
//
// Code available from: https://verilator.org
//
//*************************************************************************
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of either the GNU Lesser General Public License Version 3
// or the Perl Artistic License Version 2.0.
// SPDX-FileCopyrightText: 2003-2025 Wilson Snyder
// SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0
//
//*************************************************************************

#समावेश "V3UndrivenCapture.h"

#समावेश "V3Error.h"
#समावेश "V3Global.h"

VL_DEFINE_DEBUG_FUNCTIONS;

namespace {

श्रेणी CaptureUtil final {
खुला:
    अटल std::string taskNameQ(स्थिर Ast_गाँठ_FTask* taskp) {
        अगर (!taskp) वापस "<null>";
        वापस taskp->prettyNameQ();
    }
};

श्रेणी CaptureVisitor final : खुला VNVisitorConst {
    V3UndrivenCapture& m_cap;
    स्थिर Ast_गाँठ_FTask* m_curTaskp = nullptr;  // Current task
    bool m_inInitialSetup = false;  // In InitialAutomatic*/InitialStatic* assignment LHS

खुला:
    explicit CaptureVisitor(V3UndrivenCapture& cap, AstNetlist* netlistp)
        : m_cap{cap} {
        iterateConst(netlistp);
    }

निजी:
    // Visit a task/function definition and collect direct writes and direct callees.
    व्योम visit(Ast_गाँठ_FTask* nodep) override {
        VL_RESTORER(m_curTaskp);
        m_curTaskp = nodep;
        UINFO(9, "undriven capture enter ftask " << nodep << " " << nodep->prettyNameQ());
        m_cap.noteTask(nodep);
        iterate_And_अग्र_Const_Null(nodep->stmtsp());
    }

    व्योम visit(AstInitialAutomaticStmt* nodep) override {
        VL_RESTORER(m_inInitialSetup);
        m_inInitialSetup = true;
        iterateChildrenConst(nodep);
    }
    व्योम visit(AstInitialStaticStmt* nodep) override {
        VL_RESTORER(m_inInitialSetup);
        m_inInitialSetup = true;
        iterateChildrenConst(nodep);
    }
    व्योम visit(AstAssign* nodep) override {
        {
            VL_RESTORER(m_inInitialSetup);
            m_inInitialSetup = false;
            iterateConst(nodep->rhsp());
        }
        iterateConst(nodep->lhsp());
    }

    व्योम visit(Ast_गाँठ_Var_Ref* nodep) override {
        अगर (m_curTaskp && nodep->access().isWriteOrRW()
            && (nodep->varp()->hasUserInit() || !m_inInitialSetup)) {
            UINFO(9, "undriven capture direct write in " << CaptureUtil::taskNameQ(m_curTaskp)
                                                         << " var=" << nodep->varp()->prettyNameQ()
                                                         << " at " << nodep->fileline());

            m_cap.noteDirectWrite(m_curTaskp, nodep->varp());
        }
        iterateChildrenConst(nodep);
    }

    व्योम visit(Ast_गाँठ_FTask_Ref* nodep) override {
        // Record the call edge if resolved
        अगर (m_curTaskp) {
            अगर (स्थिर Ast_गाँठ_FTask* स्थिर calleep = nodep->taskp()) {
                UINFO(9, "undriven capture call edge " << CaptureUtil::taskNameQ(
                             m_curTaskp) << " -> " << CaptureUtil::taskNameQ(calleep));
                m_cap.noteCallEdge(m_curTaskp, calleep);
            }
        }
        iterateChildrenConst(nodep);  // still scan pins/args
    }

    व्योम visit(Ast_गाँठ* nodep) override { iterateChildrenConst(nodep); }
};

}  // namespace

V3UndrivenCapture::V3UndrivenCapture(AstNetlist* netlistp) {
    gather(netlistp);

    // Compute summaries for all tasks
    क्रम (स्थिर स्वतः& kv : m_info) (व्योम)computeWriteSummary(kv.first);

    // Release the filter memory
    क्रम (स्वतः& kv : m_info) {
        kv.second.calleesSet.सफा();
        kv.second.calleesSet.rehash(0);
        kv.second.directWritesSet.सफा();
        kv.second.directWritesSet.rehash(0);
    }
}

व्योम V3UndrivenCapture::gather(AstNetlist* netlistp) {
    // Walk netlist and populate m_info with direct writes + call edges
    CaptureVisitor{*यह, netlistp};
}

स्थिर std::vector<AstVar*>& V3UndrivenCapture::writeSummary(स्थिर Ast_गाँठ_FTask* taskp) {
    // Ensure entry exists even if empty
    (व्योम)m_info[taskp];
    वापस computeWriteSummary(taskp);
}

स्थिर std::vector<AstVar*>& V3UndrivenCapture::computeWriteSummary(स्थिर Ast_गाँठ_FTask* taskp) {
    FTaskInfo& info = m_info[taskp];

    अगर (info.state == State::DONE) {
        UINFO(9, "undriven capture writeSummary cached size="
                     << info.writeSummary.आकार() << " for " << CaptureUtil::taskNameQ(taskp));
        वापस info.writeSummary;
    }
    अगर (info.state == State::VISITING) {
        UINFO(9, "undriven capture recursion detected at "
                     << CaptureUtil::taskNameQ(taskp)
                     << " returning directWrites size=" << info.directWrites.आकार());
        // Cycle detected. return directWrites only to guarantee termination.
        अगर (info.writeSummary.empty()) info.writeSummary = info.directWrites;
        वापस info.writeSummary;
    }

    info.state = State::VISITING;

    info.writeSummary.सफा();

    // Prevent duplicates across all sources that can contribute to a write summary (direct writes
    // and call chains)
    std::unordered_set<AstVar*> seen;

    // Simple lambda for filtering duplicates
    स्वतः addVar = [&](AstVar* v) {
        अगर (seen.जोड़ो(v).second) info.writeSummary.धकेलो_back(v);
    };

    // Start with direct writes
    क्रम (AstVar* v : info.directWrites) addVar(v);

    // Add callee summaries
    क्रम (स्थिर Ast_गाँठ_FTask* calleep : info.callees) {
        अगर (m_info.find(calleep) == m_info.end()) जारी;
        स्थिर std::vector<AstVar*>& sub = computeWriteSummary(calleep);
        क्रम (AstVar* v : sub) addVar(v);
    }

    UINFO(9, "undriven capture writeSummary computed size=" << info.writeSummary.आकार() << " for "
                                                            << CaptureUtil::taskNameQ(taskp));

    // We are done, so set the m_info state correctly and return the vector of variables
    info.state = State::DONE;
    वापस info.writeSummary;
}

व्योम V3UndrivenCapture::noteTask(स्थिर Ast_गाँठ_FTask* taskp) { (व्योम)m_info[taskp]; }

व्योम V3UndrivenCapture::noteDirectWrite(स्थिर Ast_गाँठ_FTask* taskp, AstVar* varp) {
    FTaskInfo& info = m_info[taskp];

    // Exclude function return variable (not an externally visible side-effect)
    AstVar* स्थिर retVarp = VN_CAST(taskp->fvarp(), Var);
    अगर (retVarp && varp == retVarp) वापस;

    // Filter out duplicates.
    अगर (info.directWritesSet.जोड़ो(varp).second) info.directWrites.धकेलो_back(varp);
}

व्योम V3UndrivenCapture::noteCallEdge(स्थिर Ast_गाँठ_FTask* callerp, स्थिर Ast_गाँठ_FTask* calleep) {
    FTaskInfo& callerInfo = m_info[callerp];
    // Prevents duplicate entries from being appended, if calleep already exists then insert will
    // return false, and then is not inserted into the callees vector.
    अगर (callerInfo.calleesSet.जोड़ो(calleep).second) { callerInfo.callees.धकेलो_back(calleep); }
    // Ensure callee entry exists, if already exists then this is a no-op.  unordered_map<> so
    // cheap.
    (व्योम)m_info[calleep];
}
