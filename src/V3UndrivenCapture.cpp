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

#അന്തര്ഭാവ "V3UndrivenCapture.h"

#അന്തര്ഭാവ "V3Error.h"
#അന്തര്ഭാവ "V3Global.h"

VL_DEFINE_DEBUG_FUNCTIONS;

നാമാകാശ {

വർഗ്ഗം CaptureUtil final {
പൊതു:
    സ്ഥിതം std::string taskNameQ(സ്ഥിരം Ast_കണ്ണി_FTask* taskp) {
        എങ്കിൽ (!taskp) തിരികെ "<null>";
        തിരികെ taskp->prettyNameQ();
    }
};

വർഗ്ഗം CaptureVisitor final : പൊതു VNVisitorConst {
    V3UndrivenCapture& m_cap;
    സ്ഥിരം Ast_കണ്ണി_FTask* m_curTaskp = nullptr;  // Current task
    തർക്കം m_inInitialSetup = അസത്യം;  // In InitialAutomatic*/InitialStatic* assignment LHS

പൊതു:
    explicit CaptureVisitor(V3UndrivenCapture& cap, AstNetlist* netlistp)
        : m_cap{cap} {
        iterateConst(netlistp);
    }

സ്വകാര്യ:
    // Visit a task/function definition and collect direct writes and direct callees.
    ശൂന്യം visit(Ast_കണ്ണി_FTask* nodep) override {
        VL_RESTORER(m_curTaskp);
        m_curTaskp = nodep;
        UINFO(9, "undriven capture enter ftask " << nodep << " " << nodep->prettyNameQ());
        m_cap.noteTask(nodep);
        iterateAndNextConstNull(nodep->stmtsp());
    }

    ശൂന്യം visit(AstInitialAutomaticStmt* nodep) override {
        VL_RESTORER(m_inInitialSetup);
        m_inInitialSetup = സത്യം;
        iterateChildrenConst(nodep);
    }
    ശൂന്യം visit(AstInitialStaticStmt* nodep) override {
        VL_RESTORER(m_inInitialSetup);
        m_inInitialSetup = സത്യം;
        iterateChildrenConst(nodep);
    }
    ശൂന്യം visit(AstAssign* nodep) override {
        {
            VL_RESTORER(m_inInitialSetup);
            m_inInitialSetup = അസത്യം;
            iterateConst(nodep->rhsp());
        }
        iterateConst(nodep->lhsp());
    }

    ശൂന്യം visit(Ast_കണ്ണി_Var_Ref* nodep) override {
        എങ്കിൽ (m_curTaskp && nodep->access().isWriteOrRW()
            && (nodep->varp()->hasUserInit() || !m_inInitialSetup)) {
            UINFO(9, "undriven capture direct write in " << CaptureUtil::taskNameQ(m_curTaskp)
                                                         << " var=" << nodep->varp()->prettyNameQ()
                                                         << " at " << nodep->fileline());

            m_cap.noteDirectWrite(m_curTaskp, nodep->varp());
        }
        iterateChildrenConst(nodep);
    }

    ശൂന്യം visit(Ast_കണ്ണി_FTask_Ref* nodep) override {
        // Record the call edge if resolved
        എങ്കിൽ (m_curTaskp) {
            എങ്കിൽ (സ്ഥിരം Ast_കണ്ണി_FTask* സ്ഥിരം calleep = nodep->taskp()) {
                UINFO(9, "undriven capture call edge " << CaptureUtil::taskNameQ(
                             m_curTaskp) << " -> " << CaptureUtil::taskNameQ(calleep));
                m_cap.noteCallEdge(m_curTaskp, calleep);
            }
        }
        iterateChildrenConst(nodep);  // still scan pins/args
    }

    ശൂന്യം visit(Ast_കണ്ണി* nodep) override { iterateChildrenConst(nodep); }
};

}  // namespace

V3UndrivenCapture::V3UndrivenCapture(AstNetlist* netlistp) {
    gather(netlistp);

    // Compute summaries for all tasks
    വേണ്ടി (സ്ഥിരം auto& kv : m_info) (ശൂന്യം)computeWriteSummary(kv.first);

    // Release the filter memory
    വേണ്ടി (auto& kv : m_info) {
        kv.second.calleesSet.clear();
        kv.second.calleesSet.rehash(0);
        kv.second.directWritesSet.clear();
        kv.second.directWritesSet.rehash(0);
    }
}

ശൂന്യം V3UndrivenCapture::gather(AstNetlist* netlistp) {
    // Walk netlist and populate m_info with direct writes + call edges
    CaptureVisitor{*ഇത്, netlistp};
}

സ്ഥിരം std::vector<AstVar*>& V3UndrivenCapture::writeSummary(സ്ഥിരം Ast_കണ്ണി_FTask* taskp) {
    // Ensure entry exists even if empty
    (ശൂന്യം)m_info[taskp];
    തിരികെ computeWriteSummary(taskp);
}

സ്ഥിരം std::vector<AstVar*>& V3UndrivenCapture::computeWriteSummary(സ്ഥിരം Ast_കണ്ണി_FTask* taskp) {
    FTaskInfo& info = m_info[taskp];

    എങ്കിൽ (info.state == State::DONE) {
        UINFO(9, "undriven capture writeSummary cached size="
                     << info.writeSummary.size() << " for " << CaptureUtil::taskNameQ(taskp));
        തിരികെ info.writeSummary;
    }
    എങ്കിൽ (info.state == State::VISITING) {
        UINFO(9, "undriven capture recursion detected at "
                     << CaptureUtil::taskNameQ(taskp)
                     << " returning directWrites size=" << info.directWrites.size());
        // Cycle detected. return directWrites only to guarantee termination.
        എങ്കിൽ (info.writeSummary.empty()) info.writeSummary = info.directWrites;
        തിരികെ info.writeSummary;
    }

    info.state = State::VISITING;

    info.writeSummary.clear();

    // Prevent duplicates across all sources that can contribute to a write summary (direct writes
    // and call chains)
    std::unordered_set<AstVar*> seen;

    // Simple lambda for filtering duplicates
    auto addVar = [&](AstVar* v) {
        എങ്കിൽ (seen.തിരുകുക(v).second) info.writeSummary.തള്ളുക_back(v);
    };

    // Start with direct writes
    വേണ്ടി (AstVar* v : info.directWrites) addVar(v);

    // Add callee summaries
    വേണ്ടി (സ്ഥിരം Ast_കണ്ണി_FTask* calleep : info.callees) {
        എങ്കിൽ (m_info.find(calleep) == m_info.end()) തുടരുക;
        സ്ഥിരം std::vector<AstVar*>& sub = computeWriteSummary(calleep);
        വേണ്ടി (AstVar* v : sub) addVar(v);
    }

    UINFO(9, "undriven capture writeSummary computed size=" << info.writeSummary.size() << " for "
                                                            << CaptureUtil::taskNameQ(taskp));

    // We are done, so set the m_info state correctly and return the vector of variables
    info.state = State::DONE;
    തിരികെ info.writeSummary;
}

ശൂന്യം V3UndrivenCapture::noteTask(സ്ഥിരം Ast_കണ്ണി_FTask* taskp) { (ശൂന്യം)m_info[taskp]; }

ശൂന്യം V3UndrivenCapture::noteDirectWrite(സ്ഥിരം Ast_കണ്ണി_FTask* taskp, AstVar* varp) {
    FTaskInfo& info = m_info[taskp];

    // Exclude function return variable (not an externally visible side-effect)
    AstVar* സ്ഥിരം retVarp = VN_CAST(taskp->fvarp(), Var);
    എങ്കിൽ (retVarp && varp == retVarp) തിരികെ;

    // Filter out duplicates.
    എങ്കിൽ (info.directWritesSet.തിരുകുക(varp).second) info.directWrites.തള്ളുക_back(varp);
}

ശൂന്യം V3UndrivenCapture::noteCallEdge(സ്ഥിരം Ast_കണ്ണി_FTask* callerp, സ്ഥിരം Ast_കണ്ണി_FTask* calleep) {
    FTaskInfo& callerInfo = m_info[callerp];
    // Prevents duplicate entries from being appended, if calleep already exists then insert will
    // return false, and then is not inserted into the callees vector.
    എങ്കിൽ (callerInfo.calleesSet.തിരുകുക(calleep).second) { callerInfo.callees.തള്ളുക_back(calleep); }
    // Ensure callee entry exists, if already exists then this is a no-op.  unordered_map<> so
    // cheap.
    (ശൂന്യം)m_info[calleep];
}
