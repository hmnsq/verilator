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

#ಅನ್ತರ್ಭಾವ "V3UndrivenCapture.h"

#ಅನ್ತರ್ಭಾವ "V3Error.h"
#ಅನ್ತರ್ಭಾವ "V3Global.h"

VL_DEFINE_DEBUG_FUNCTIONS;

ನಾಮಾಕಾಶ {

ವರ್ಗ CaptureUtil final {
ಸಾರ್ವಜನಿಕ:
    ಸ್ಥಿರ std::string taskNameQ(ಧ್ರುವ Ast_ಗಂಟು_FTask* taskp) {
        ಯದಿ (!taskp) ಪ್ರತ್ಯಾಗಮ "<null>";
        ಪ್ರತ್ಯಾಗಮ taskp->prettyNameQ();
    }
};

ವರ್ಗ CaptureVisitor final : ಸಾರ್ವಜನಿಕ VNVisitorConst {
    V3UndrivenCapture& m_cap;
    ಧ್ರುವ Ast_ಗಂಟು_FTask* m_curTaskp = nullptr;  // Current task
    ತಾರ್ಕಿಕ m_inInitialSetup = ಅಸತ್ಯ;  // In InitialAutomatic*/InitialStatic* assignment LHS

ಸಾರ್ವಜನಿಕ:
    explicit CaptureVisitor(V3UndrivenCapture& cap, AstNetlist* netlistp)
        : m_cap{cap} {
        iterateConst(netlistp);
    }

ಗುಪ್ತ:
    // Visit a task/function definition and collect direct writes and direct callees.
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_FTask* nodep) override {
        VL_RESTORER(m_curTaskp);
        m_curTaskp = nodep;
        UINFO(9, "undriven capture enter ftask " << nodep << " " << nodep->prettyNameQ());
        m_cap.noteTask(nodep);
        iterateAndNextConstNull(nodep->stmtsp());
    }

    ಶೂನ್ಯ visit(AstInitialAutomaticStmt* nodep) override {
        VL_RESTORER(m_inInitialSetup);
        m_inInitialSetup = ಸತ್ಯ;
        iterateChildrenConst(nodep);
    }
    ಶೂನ್ಯ visit(AstInitialStaticStmt* nodep) override {
        VL_RESTORER(m_inInitialSetup);
        m_inInitialSetup = ಸತ್ಯ;
        iterateChildrenConst(nodep);
    }
    ಶೂನ್ಯ visit(AstAssign* nodep) override {
        {
            VL_RESTORER(m_inInitialSetup);
            m_inInitialSetup = ಅಸತ್ಯ;
            iterateConst(nodep->rhsp());
        }
        iterateConst(nodep->lhsp());
    }

    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Var_Ref* nodep) override {
        ಯದಿ (m_curTaskp && nodep->access().isWriteOrRW()
            && (nodep->varp()->hasUserInit() || !m_inInitialSetup)) {
            UINFO(9, "undriven capture direct write in " << CaptureUtil::taskNameQ(m_curTaskp)
                                                         << " var=" << nodep->varp()->prettyNameQ()
                                                         << " at " << nodep->fileline());

            m_cap.noteDirectWrite(m_curTaskp, nodep->varp());
        }
        iterateChildrenConst(nodep);
    }

    ಶೂನ್ಯ visit(Ast_ಗಂಟು_FTask_Ref* nodep) override {
        // Record the call edge if resolved
        ಯದಿ (m_curTaskp) {
            ಯದಿ (ಧ್ರುವ Ast_ಗಂಟು_FTask* ಧ್ರುವ calleep = nodep->taskp()) {
                UINFO(9, "undriven capture call edge " << CaptureUtil::taskNameQ(
                             m_curTaskp) << " -> " << CaptureUtil::taskNameQ(calleep));
                m_cap.noteCallEdge(m_curTaskp, calleep);
            }
        }
        iterateChildrenConst(nodep);  // still scan pins/args
    }

    ಶೂನ್ಯ visit(Ast_ಗಂಟು* nodep) override { iterateChildrenConst(nodep); }
};

}  // namespace

V3UndrivenCapture::V3UndrivenCapture(AstNetlist* netlistp) {
    gather(netlistp);

    // Compute summaries for all tasks
    ಪ್ರತಿ (ಧ್ರುವ auto& kv : m_info) (ಶೂನ್ಯ)computeWriteSummary(kv.first);

    // Release the filter memory
    ಪ್ರತಿ (auto& kv : m_info) {
        kv.second.calleesSet.clear();
        kv.second.calleesSet.rehash(0);
        kv.second.directWritesSet.clear();
        kv.second.directWritesSet.rehash(0);
    }
}

ಶೂನ್ಯ V3UndrivenCapture::gather(AstNetlist* netlistp) {
    // Walk netlist and populate m_info with direct writes + call edges
    CaptureVisitor{*ಏತತ್, netlistp};
}

ಧ್ರುವ std::vector<AstVar*>& V3UndrivenCapture::writeSummary(ಧ್ರುವ Ast_ಗಂಟು_FTask* taskp) {
    // Ensure entry exists even if empty
    (ಶೂನ್ಯ)m_info[taskp];
    ಪ್ರತ್ಯಾಗಮ computeWriteSummary(taskp);
}

ಧ್ರುವ std::vector<AstVar*>& V3UndrivenCapture::computeWriteSummary(ಧ್ರುವ Ast_ಗಂಟು_FTask* taskp) {
    FTaskInfo& info = m_info[taskp];

    ಯದಿ (info.state == State::DONE) {
        UINFO(9, "undriven capture writeSummary cached size="
                     << info.writeSummary.size() << " for " << CaptureUtil::taskNameQ(taskp));
        ಪ್ರತ್ಯಾಗಮ info.writeSummary;
    }
    ಯದಿ (info.state == State::VISITING) {
        UINFO(9, "undriven capture recursion detected at "
                     << CaptureUtil::taskNameQ(taskp)
                     << " returning directWrites size=" << info.directWrites.size());
        // Cycle detected. return directWrites only to guarantee termination.
        ಯದಿ (info.writeSummary.empty()) info.writeSummary = info.directWrites;
        ಪ್ರತ್ಯಾಗಮ info.writeSummary;
    }

    info.state = State::VISITING;

    info.writeSummary.clear();

    // Prevent duplicates across all sources that can contribute to a write summary (direct writes
    // and call chains)
    std::unordered_set<AstVar*> seen;

    // Simple lambda for filtering duplicates
    auto addVar = [&](AstVar* v) {
        ಯದಿ (seen.ಸೇರಿಸು(v).second) info.writeSummary.ತಳ್ಳು_back(v);
    };

    // Start with direct writes
    ಪ್ರತಿ (AstVar* v : info.directWrites) addVar(v);

    // Add callee summaries
    ಪ್ರತಿ (ಧ್ರುವ Ast_ಗಂಟು_FTask* calleep : info.callees) {
        ಯದಿ (m_info.find(calleep) == m_info.end()) ಅನುವರ್ತ;
        ಧ್ರುವ std::vector<AstVar*>& sub = computeWriteSummary(calleep);
        ಪ್ರತಿ (AstVar* v : sub) addVar(v);
    }

    UINFO(9, "undriven capture writeSummary computed size=" << info.writeSummary.size() << " for "
                                                            << CaptureUtil::taskNameQ(taskp));

    // We are done, so set the m_info state correctly and return the vector of variables
    info.state = State::DONE;
    ಪ್ರತ್ಯಾಗಮ info.writeSummary;
}

ಶೂನ್ಯ V3UndrivenCapture::noteTask(ಧ್ರುವ Ast_ಗಂಟು_FTask* taskp) { (ಶೂನ್ಯ)m_info[taskp]; }

ಶೂನ್ಯ V3UndrivenCapture::noteDirectWrite(ಧ್ರುವ Ast_ಗಂಟು_FTask* taskp, AstVar* varp) {
    FTaskInfo& info = m_info[taskp];

    // Exclude function return variable (not an externally visible side-effect)
    AstVar* ಧ್ರುವ retVarp = VN_CAST(taskp->fvarp(), Var);
    ಯದಿ (retVarp && varp == retVarp) ಪ್ರತ್ಯಾಗಮ;

    // Filter out duplicates.
    ಯದಿ (info.directWritesSet.ಸೇರಿಸು(varp).second) info.directWrites.ತಳ್ಳು_back(varp);
}

ಶೂನ್ಯ V3UndrivenCapture::noteCallEdge(ಧ್ರುವ Ast_ಗಂಟು_FTask* callerp, ಧ್ರುವ Ast_ಗಂಟು_FTask* calleep) {
    FTaskInfo& callerInfo = m_info[callerp];
    // Prevents duplicate entries from being appended, if calleep already exists then insert will
    // return false, and then is not inserted into the callees vector.
    ಯದಿ (callerInfo.calleesSet.ಸೇರಿಸು(calleep).second) { callerInfo.callees.ತಳ್ಳು_back(calleep); }
    // Ensure callee entry exists, if already exists then this is a no-op.  unordered_map<> so
    // cheap.
    (ಶೂನ್ಯ)m_info[calleep];
}
