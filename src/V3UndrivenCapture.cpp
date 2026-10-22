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

#अन्तर्भाव "V3UndrivenCapture.h"

#अन्तर्भाव "V3Error.h"
#अन्तर्भाव "V3Global.h"

VL_DEFINE_DEBUG_FUNCTIONS;

नामाकाश {

वग्ग CaptureUtil final {
सार्वजनिक:
    स्थिर std::string taskNameQ(ध्रुव Ast_ग्रन्थिः_FTask* taskp) {
        सचे (!taskp) पटिनिवत्त "<null>";
        पटिनिवत्त taskp->prettyNameQ();
    }
};

वग्ग CaptureVisitor final : सार्वजनिक VNVisitorConst {
    V3UndrivenCapture& m_cap;
    ध्रुव Ast_ग्रन्थिः_FTask* m_curTaskp = nullptr;  // Current task
    तार्किक m_inInitialSetup = मिच्छा;  // In InitialAutomatic*/InitialStatic* assignment LHS

सार्वजनिक:
    explicit CaptureVisitor(V3UndrivenCapture& cap, AstNetlist* netlistp)
        : m_cap{cap} {
        iterateConst(netlistp);
    }

गुप्त:
    // Visit a task/function definition and collect direct writes and direct callees.
    शून्य visit(Ast_ग्रन्थिः_FTask* nodep) override {
        VL_RESTORER(m_curTaskp);
        m_curTaskp = nodep;
        UINFO(9, "undriven capture enter ftask " << nodep << " " << nodep->prettyNameQ());
        m_cap.noteTask(nodep);
        iterateAndNextConstNull(nodep->stmtsp());
    }

    शून्य visit(AstInitialAutomaticStmt* nodep) override {
        VL_RESTORER(m_inInitialSetup);
        m_inInitialSetup = सच्च;
        iterateChildrenConst(nodep);
    }
    शून्य visit(AstInitialStaticStmt* nodep) override {
        VL_RESTORER(m_inInitialSetup);
        m_inInitialSetup = सच्च;
        iterateChildrenConst(nodep);
    }
    शून्य visit(AstAssign* nodep) override {
        {
            VL_RESTORER(m_inInitialSetup);
            m_inInitialSetup = मिच्छा;
            iterateConst(nodep->rhsp());
        }
        iterateConst(nodep->lhsp());
    }

    शून्य visit(Ast_ग्रन्थिः_Var_Ref* nodep) override {
        सचे (m_curTaskp && nodep->access().isWriteOrRW()
            && (nodep->varp()->hasUserInit() || !m_inInitialSetup)) {
            UINFO(9, "undriven capture direct write in " << CaptureUtil::taskNameQ(m_curTaskp)
                                                         << " var=" << nodep->varp()->prettyNameQ()
                                                         << " at " << nodep->fileline());

            m_cap.noteDirectWrite(m_curTaskp, nodep->varp());
        }
        iterateChildrenConst(nodep);
    }

    शून्य visit(Ast_ग्रन्थिः_FTask_Ref* nodep) override {
        // Record the call edge if resolved
        सचे (m_curTaskp) {
            सचे (ध्रुव Ast_ग्रन्थिः_FTask* ध्रुव calleep = nodep->taskp()) {
                UINFO(9, "undriven capture call edge " << CaptureUtil::taskNameQ(
                             m_curTaskp) << " -> " << CaptureUtil::taskNameQ(calleep));
                m_cap.noteCallEdge(m_curTaskp, calleep);
            }
        }
        iterateChildrenConst(nodep);  // still scan pins/args
    }

    शून्य visit(Ast_ग्रन्थिः* nodep) override { iterateChildrenConst(nodep); }
};

}  // namespace

V3UndrivenCapture::V3UndrivenCapture(AstNetlist* netlistp) {
    gather(netlistp);

    // Compute summaries for all tasks
    अत्थाय (ध्रुव auto& kv : m_info) (शून्य)computeWriteSummary(kv.first);

    // Release the filter memory
    अत्थाय (auto& kv : m_info) {
        kv.second.calleesSet.clear();
        kv.second.calleesSet.rehash(0);
        kv.second.directWritesSet.clear();
        kv.second.directWritesSet.rehash(0);
    }
}

शून्य V3UndrivenCapture::gather(AstNetlist* netlistp) {
    // Walk netlist and populate m_info with direct writes + call edges
    CaptureVisitor{*एतत्, netlistp};
}

ध्रुव std::vector<AstVar*>& V3UndrivenCapture::writeSummary(ध्रुव Ast_ग्रन्थिः_FTask* taskp) {
    // Ensure entry exists even if empty
    (शून्य)m_info[taskp];
    पटिनिवत्त computeWriteSummary(taskp);
}

ध्रुव std::vector<AstVar*>& V3UndrivenCapture::computeWriteSummary(ध्रुव Ast_ग्रन्थिः_FTask* taskp) {
    FTaskInfo& info = m_info[taskp];

    सचे (info.state == State::DONE) {
        UINFO(9, "undriven capture writeSummary cached size="
                     << info.writeSummary.size() << " for " << CaptureUtil::taskNameQ(taskp));
        पटिनिवत्त info.writeSummary;
    }
    सचे (info.state == State::VISITING) {
        UINFO(9, "undriven capture recursion detected at "
                     << CaptureUtil::taskNameQ(taskp)
                     << " returning directWrites size=" << info.directWrites.size());
        // Cycle detected. return directWrites only to guarantee termination.
        सचे (info.writeSummary.empty()) info.writeSummary = info.directWrites;
        पटिनिवत्त info.writeSummary;
    }

    info.state = State::VISITING;

    info.writeSummary.clear();

    // Prevent duplicates across all sources that can contribute to a write summary (direct writes
    // and call chains)
    std::unordered_set<AstVar*> seen;

    // Simple lambda for filtering duplicates
    auto addVar = [&](AstVar* v) {
        सचे (seen.निवेशय(v).second) info.writeSummary.प्रक्षिप_back(v);
    };

    // Start with direct writes
    अत्थाय (AstVar* v : info.directWrites) addVar(v);

    // Add callee summaries
    अत्थाय (ध्रुव Ast_ग्रन्थिः_FTask* calleep : info.callees) {
        सचे (m_info.find(calleep) == m_info.end()) अनुवर्त;
        ध्रुव std::vector<AstVar*>& sub = computeWriteSummary(calleep);
        अत्थाय (AstVar* v : sub) addVar(v);
    }

    UINFO(9, "undriven capture writeSummary computed size=" << info.writeSummary.size() << " for "
                                                            << CaptureUtil::taskNameQ(taskp));

    // We are done, so set the m_info state correctly and return the vector of variables
    info.state = State::DONE;
    पटिनिवत्त info.writeSummary;
}

शून्य V3UndrivenCapture::noteTask(ध्रुव Ast_ग्रन्थिः_FTask* taskp) { (शून्य)m_info[taskp]; }

शून्य V3UndrivenCapture::noteDirectWrite(ध्रुव Ast_ग्रन्थिः_FTask* taskp, AstVar* varp) {
    FTaskInfo& info = m_info[taskp];

    // Exclude function return variable (not an externally visible side-effect)
    AstVar* ध्रुव retVarp = VN_CAST(taskp->fvarp(), Var);
    सचे (retVarp && varp == retVarp) पटिनिवत्त;

    // Filter out duplicates.
    सचे (info.directWritesSet.निवेशय(varp).second) info.directWrites.प्रक्षिप_back(varp);
}

शून्य V3UndrivenCapture::noteCallEdge(ध्रुव Ast_ग्रन्थिः_FTask* callerp, ध्रुव Ast_ग्रन्थिः_FTask* calleep) {
    FTaskInfo& callerInfo = m_info[callerp];
    // Prevents duplicate entries from being appended, if calleep already exists then insert will
    // return false, and then is not inserted into the callees vector.
    सचे (callerInfo.calleesSet.निवेशय(calleep).second) { callerInfo.callees.प्रक्षिप_back(calleep); }
    // Ensure callee entry exists, if already exists then this is a no-op.  unordered_map<> so
    // cheap.
    (शून्य)m_info[calleep];
}
