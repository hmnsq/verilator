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

#ଅନ୍ତର୍ଭାଵ "V3UndrivenCapture.h"

#ଅନ୍ତର୍ଭାଵ "V3Error.h"
#ଅନ୍ତର୍ଭାଵ "V3Global.h"

VL_DEFINE_DEBUG_FUNCTIONS;

ନାମାକାଶ {

ଶ୍ରେଣୀ CaptureUtil final {
ସର୍ବସାଧାରଣ:
    ସ୍ଥିର std::string taskNameQ(ଅଚଳ Ast_ଗଣ୍ଠି_FTask* taskp) {
        ଯଦି (!taskp) ଫେରନ୍ତୁ "<null>";
        ଫେରନ୍ତୁ taskp->prettyNameQ();
    }
};

ଶ୍ରେଣୀ CaptureVisitor final : ସର୍ବସାଧାରଣ VNVisitorConst {
    V3UndrivenCapture& m_cap;
    ଅଚଳ Ast_ଗଣ୍ଠି_FTask* m_curTaskp = nullptr;  // Current task
    ତାର୍କିକ m_inInitialSetup = ମିଥ୍ୟା;  // In InitialAutomatic*/InitialStatic* assignment LHS

ସର୍ବସାଧାରଣ:
    explicit CaptureVisitor(V3UndrivenCapture& cap, AstNetlist* netlistp)
        : m_cap{cap} {
        iterateConst(netlistp);
    }

ବ୍ୟକ୍ତିଗତ:
    // Visit a task/function definition and collect direct writes and direct callees.
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_FTask* nodep) override {
        VL_RESTORER(m_curTaskp);
        m_curTaskp = nodep;
        UINFO(9, "undriven capture enter ftask " << nodep << " " << nodep->prettyNameQ());
        m_cap.noteTask(nodep);
        iterateAndNextConstNull(nodep->stmtsp());
    }

    ଶୂନ୍ୟ visit(AstInitialAutomaticStmt* nodep) override {
        VL_RESTORER(m_inInitialSetup);
        m_inInitialSetup = ସତ୍ୟ;
        iterateChildrenConst(nodep);
    }
    ଶୂନ୍ୟ visit(AstInitialStaticStmt* nodep) override {
        VL_RESTORER(m_inInitialSetup);
        m_inInitialSetup = ସତ୍ୟ;
        iterateChildrenConst(nodep);
    }
    ଶୂନ୍ୟ visit(AstAssign* nodep) override {
        {
            VL_RESTORER(m_inInitialSetup);
            m_inInitialSetup = ମିଥ୍ୟା;
            iterateConst(nodep->rhsp());
        }
        iterateConst(nodep->lhsp());
    }

    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Var_Ref* nodep) override {
        ଯଦି (m_curTaskp && nodep->access().isWriteOrRW()
            && (nodep->varp()->hasUserInit() || !m_inInitialSetup)) {
            UINFO(9, "undriven capture direct write in " << CaptureUtil::taskNameQ(m_curTaskp)
                                                         << " var=" << nodep->varp()->prettyNameQ()
                                                         << " at " << nodep->fileline());

            m_cap.noteDirectWrite(m_curTaskp, nodep->varp());
        }
        iterateChildrenConst(nodep);
    }

    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_FTask_Ref* nodep) override {
        // Record the call edge if resolved
        ଯଦି (m_curTaskp) {
            ଯଦି (ଅଚଳ Ast_ଗଣ୍ଠି_FTask* ଅଚଳ calleep = nodep->taskp()) {
                UINFO(9, "undriven capture call edge " << CaptureUtil::taskNameQ(
                             m_curTaskp) << " -> " << CaptureUtil::taskNameQ(calleep));
                m_cap.noteCallEdge(m_curTaskp, calleep);
            }
        }
        iterateChildrenConst(nodep);  // still scan pins/args
    }

    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override { iterateChildrenConst(nodep); }
};

}  // namespace

V3UndrivenCapture::V3UndrivenCapture(AstNetlist* netlistp) {
    gather(netlistp);

    // Compute summaries for all tasks
    ପାଇଁ (ଅଚଳ auto& kv : m_info) (ଶୂନ୍ୟ)computeWriteSummary(kv.first);

    // Release the filter memory
    ପାଇଁ (auto& kv : m_info) {
        kv.second.calleesSet.clear();
        kv.second.calleesSet.rehash(0);
        kv.second.directWritesSet.clear();
        kv.second.directWritesSet.rehash(0);
    }
}

ଶୂନ୍ୟ V3UndrivenCapture::gather(AstNetlist* netlistp) {
    // Walk netlist and populate m_info with direct writes + call edges
    CaptureVisitor{*ଏହି, netlistp};
}

ଅଚଳ std::vector<AstVar*>& V3UndrivenCapture::writeSummary(ଅଚଳ Ast_ଗଣ୍ଠି_FTask* taskp) {
    // Ensure entry exists even if empty
    (ଶୂନ୍ୟ)m_info[taskp];
    ଫେରନ୍ତୁ computeWriteSummary(taskp);
}

ଅଚଳ std::vector<AstVar*>& V3UndrivenCapture::computeWriteSummary(ଅଚଳ Ast_ଗଣ୍ଠି_FTask* taskp) {
    FTaskInfo& info = m_info[taskp];

    ଯଦି (info.state == State::DONE) {
        UINFO(9, "undriven capture writeSummary cached size="
                     << info.writeSummary.size() << " for " << CaptureUtil::taskNameQ(taskp));
        ଫେରନ୍ତୁ info.writeSummary;
    }
    ଯଦି (info.state == State::VISITING) {
        UINFO(9, "undriven capture recursion detected at "
                     << CaptureUtil::taskNameQ(taskp)
                     << " returning directWrites size=" << info.directWrites.size());
        // Cycle detected. return directWrites only to guarantee termination.
        ଯଦି (info.writeSummary.empty()) info.writeSummary = info.directWrites;
        ଫେରନ୍ତୁ info.writeSummary;
    }

    info.state = State::VISITING;

    info.writeSummary.clear();

    // Prevent duplicates across all sources that can contribute to a write summary (direct writes
    // and call chains)
    std::unordered_set<AstVar*> seen;

    // Simple lambda for filtering duplicates
    auto addVar = [&](AstVar* v) {
        ଯଦି (seen.ଭର୍ତ୍ତି(v).second) info.writeSummary.ଠେଲ_back(v);
    };

    // Start with direct writes
    ପାଇଁ (AstVar* v : info.directWrites) addVar(v);

    // Add callee summaries
    ପାଇଁ (ଅଚଳ Ast_ଗଣ୍ଠି_FTask* calleep : info.callees) {
        ଯଦି (m_info.find(calleep) == m_info.end()) ଜାରି;
        ଅଚଳ std::vector<AstVar*>& sub = computeWriteSummary(calleep);
        ପାଇଁ (AstVar* v : sub) addVar(v);
    }

    UINFO(9, "undriven capture writeSummary computed size=" << info.writeSummary.size() << " for "
                                                            << CaptureUtil::taskNameQ(taskp));

    // We are done, so set the m_info state correctly and return the vector of variables
    info.state = State::DONE;
    ଫେରନ୍ତୁ info.writeSummary;
}

ଶୂନ୍ୟ V3UndrivenCapture::noteTask(ଅଚଳ Ast_ଗଣ୍ଠି_FTask* taskp) { (ଶୂନ୍ୟ)m_info[taskp]; }

ଶୂନ୍ୟ V3UndrivenCapture::noteDirectWrite(ଅଚଳ Ast_ଗଣ୍ଠି_FTask* taskp, AstVar* varp) {
    FTaskInfo& info = m_info[taskp];

    // Exclude function return variable (not an externally visible side-effect)
    AstVar* ଅଚଳ retVarp = VN_CAST(taskp->fvarp(), Var);
    ଯଦି (retVarp && varp == retVarp) ଫେରନ୍ତୁ;

    // Filter out duplicates.
    ଯଦି (info.directWritesSet.ଭର୍ତ୍ତି(varp).second) info.directWrites.ଠେଲ_back(varp);
}

ଶୂନ୍ୟ V3UndrivenCapture::noteCallEdge(ଅଚଳ Ast_ଗଣ୍ଠି_FTask* callerp, ଅଚଳ Ast_ଗଣ୍ଠି_FTask* calleep) {
    FTaskInfo& callerInfo = m_info[callerp];
    // Prevents duplicate entries from being appended, if calleep already exists then insert will
    // return false, and then is not inserted into the callees vector.
    ଯଦି (callerInfo.calleesSet.ଭର୍ତ୍ତି(calleep).second) { callerInfo.callees.ଠେଲ_back(calleep); }
    // Ensure callee entry exists, if already exists then this is a no-op.  unordered_map<> so
    // cheap.
    (ଶୂନ୍ୟ)m_info[calleep];
}
