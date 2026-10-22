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

वर्ग CaptureUtil final {
सार्वजनिक:
    स्थिर std::string taskNameQ(अचर Ast_गाँठो_FTask* taskp) {
        यदि (!taskp) फर्काऊ "<null>";
        फर्काऊ taskp->prettyNameQ();
    }
};

वर्ग CaptureVisitor final : सार्वजनिक VNVisitorConst {
    V3UndrivenCapture& m_cap;
    अचर Ast_गाँठो_FTask* m_curTaskp = nullptr;  // Current task
    तार्किक m_inInitialSetup = असत्य;  // In InitialAutomatic*/InitialStatic* assignment LHS

सार्वजनिक:
    explicit CaptureVisitor(V3UndrivenCapture& cap, AstNetlist* netlistp)
        : m_cap{cap} {
        iterateConst(netlistp);
    }

गुप्त:
    // Visit a task/function definition and collect direct writes and direct callees.
    शून्य visit(Ast_गाँठो_FTask* nodep) override {
        VL_RESTORER(m_curTaskp);
        m_curTaskp = nodep;
        UINFO(9, "undriven capture enter ftask " << nodep << " " << nodep->prettyNameQ());
        m_cap.noteTask(nodep);
        iterateAndNextConstNull(nodep->stmtsp());
    }

    शून्य visit(AstInitialAutomaticStmt* nodep) override {
        VL_RESTORER(m_inInitialSetup);
        m_inInitialSetup = सत्य;
        iterateChildrenConst(nodep);
    }
    शून्य visit(AstInitialStaticStmt* nodep) override {
        VL_RESTORER(m_inInitialSetup);
        m_inInitialSetup = सत्य;
        iterateChildrenConst(nodep);
    }
    शून्य visit(AstAssign* nodep) override {
        {
            VL_RESTORER(m_inInitialSetup);
            m_inInitialSetup = असत्य;
            iterateConst(nodep->rhsp());
        }
        iterateConst(nodep->lhsp());
    }

    शून्य visit(Ast_गाँठो_Var_Ref* nodep) override {
        यदि (m_curTaskp && nodep->access().isWriteOrRW()
            && (nodep->varp()->hasUserInit() || !m_inInitialSetup)) {
            UINFO(9, "undriven capture direct write in " << CaptureUtil::taskNameQ(m_curTaskp)
                                                         << " var=" << nodep->varp()->prettyNameQ()
                                                         << " at " << nodep->fileline());

            m_cap.noteDirectWrite(m_curTaskp, nodep->varp());
        }
        iterateChildrenConst(nodep);
    }

    शून्य visit(Ast_गाँठो_FTask_Ref* nodep) override {
        // Record the call edge if resolved
        यदि (m_curTaskp) {
            यदि (अचर Ast_गाँठो_FTask* अचर calleep = nodep->taskp()) {
                UINFO(9, "undriven capture call edge " << CaptureUtil::taskNameQ(
                             m_curTaskp) << " -> " << CaptureUtil::taskNameQ(calleep));
                m_cap.noteCallEdge(m_curTaskp, calleep);
            }
        }
        iterateChildrenConst(nodep);  // still scan pins/args
    }

    शून्य visit(Ast_गाँठो* nodep) override { iterateChildrenConst(nodep); }
};

}  // namespace

V3UndrivenCapture::V3UndrivenCapture(AstNetlist* netlistp) {
    gather(netlistp);

    // Compute summaries for all tasks
    लागि (अचर स्वतः& kv : m_info) (शून्य)computeWriteSummary(kv.first);

    // Release the filter memory
    लागि (स्वतः& kv : m_info) {
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

अचर std::vector<AstVar*>& V3UndrivenCapture::writeSummary(अचर Ast_गाँठो_FTask* taskp) {
    // Ensure entry exists even if empty
    (शून्य)m_info[taskp];
    फर्काऊ computeWriteSummary(taskp);
}

अचर std::vector<AstVar*>& V3UndrivenCapture::computeWriteSummary(अचर Ast_गाँठो_FTask* taskp) {
    FTaskInfo& info = m_info[taskp];

    यदि (info.state == State::DONE) {
        UINFO(9, "undriven capture writeSummary cached size="
                     << info.writeSummary.size() << " for " << CaptureUtil::taskNameQ(taskp));
        फर्काऊ info.writeSummary;
    }
    यदि (info.state == State::VISITING) {
        UINFO(9, "undriven capture recursion detected at "
                     << CaptureUtil::taskNameQ(taskp)
                     << " returning directWrites size=" << info.directWrites.size());
        // Cycle detected. return directWrites only to guarantee termination.
        यदि (info.writeSummary.empty()) info.writeSummary = info.directWrites;
        फर्काऊ info.writeSummary;
    }

    info.state = State::VISITING;

    info.writeSummary.clear();

    // Prevent duplicates across all sources that can contribute to a write summary (direct writes
    // and call chains)
    std::unordered_set<AstVar*> seen;

    // Simple lambda for filtering duplicates
    स्वतः addVar = [&](AstVar* v) {
        यदि (seen.घुसाउ(v).second) info.writeSummary.थप_back(v);
    };

    // Start with direct writes
    लागि (AstVar* v : info.directWrites) addVar(v);

    // Add callee summaries
    लागि (अचर Ast_गाँठो_FTask* calleep : info.callees) {
        यदि (m_info.find(calleep) == m_info.end()) जारी;
        अचर std::vector<AstVar*>& sub = computeWriteSummary(calleep);
        लागि (AstVar* v : sub) addVar(v);
    }

    UINFO(9, "undriven capture writeSummary computed size=" << info.writeSummary.size() << " for "
                                                            << CaptureUtil::taskNameQ(taskp));

    // We are done, so set the m_info state correctly and return the vector of variables
    info.state = State::DONE;
    फर्काऊ info.writeSummary;
}

शून्य V3UndrivenCapture::noteTask(अचर Ast_गाँठो_FTask* taskp) { (शून्य)m_info[taskp]; }

शून्य V3UndrivenCapture::noteDirectWrite(अचर Ast_गाँठो_FTask* taskp, AstVar* varp) {
    FTaskInfo& info = m_info[taskp];

    // Exclude function return variable (not an externally visible side-effect)
    AstVar* अचर retVarp = VN_CAST(taskp->fvarp(), Var);
    यदि (retVarp && varp == retVarp) फर्काऊ;

    // Filter out duplicates.
    यदि (info.directWritesSet.घुसाउ(varp).second) info.directWrites.थप_back(varp);
}

शून्य V3UndrivenCapture::noteCallEdge(अचर Ast_गाँठो_FTask* callerp, अचर Ast_गाँठो_FTask* calleep) {
    FTaskInfo& callerInfo = m_info[callerp];
    // Prevents duplicate entries from being appended, if calleep already exists then insert will
    // return false, and then is not inserted into the callees vector.
    यदि (callerInfo.calleesSet.घुसाउ(calleep).second) { callerInfo.callees.थप_back(calleep); }
    // Ensure callee entry exists, if already exists then this is a no-op.  unordered_map<> so
    // cheap.
    (शून्य)m_info[calleep];
}
