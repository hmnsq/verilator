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

#અન્તર્ભાવ "V3UndrivenCapture.h"

#અન્તર્ભાવ "V3Error.h"
#અન્તર્ભાવ "V3Global.h"

VL_DEFINE_DEBUG_FUNCTIONS;

નામાકાશ {

વર્ગ CaptureUtil final {
જાહેર:
    સ્થિર std::string taskNameQ(અચલ Ast_ગાંઠ_FTask* taskp) {
        જો (!taskp) પાછા "<null>";
        પાછા taskp->prettyNameQ();
    }
};

વર્ગ CaptureVisitor final : જાહેર VNVisitorConst {
    V3UndrivenCapture& m_cap;
    અચલ Ast_ગાંઠ_FTask* m_curTaskp = nullptr;  // Current task
    તાર્કિક m_inInitialSetup = ખોટું;  // In InitialAutomatic*/InitialStatic* assignment LHS

જાહેર:
    explicit CaptureVisitor(V3UndrivenCapture& cap, AstNetlist* netlistp)
        : m_cap{cap} {
        iterateConst(netlistp);
    }

ખાનગી:
    // Visit a task/function definition and collect direct writes and direct callees.
    શૂન્ય visit(Ast_ગાંઠ_FTask* nodep) override {
        VL_RESTORER(m_curTaskp);
        m_curTaskp = nodep;
        UINFO(9, "undriven capture enter ftask " << nodep << " " << nodep->prettyNameQ());
        m_cap.noteTask(nodep);
        iterateAndNextConstNull(nodep->stmtsp());
    }

    શૂન્ય visit(AstInitialAutomaticStmt* nodep) override {
        VL_RESTORER(m_inInitialSetup);
        m_inInitialSetup = સાચું;
        iterateChildrenConst(nodep);
    }
    શૂન્ય visit(AstInitialStaticStmt* nodep) override {
        VL_RESTORER(m_inInitialSetup);
        m_inInitialSetup = સાચું;
        iterateChildrenConst(nodep);
    }
    શૂન્ય visit(AstAssign* nodep) override {
        {
            VL_RESTORER(m_inInitialSetup);
            m_inInitialSetup = ખોટું;
            iterateConst(nodep->rhsp());
        }
        iterateConst(nodep->lhsp());
    }

    શૂન્ય visit(Ast_ગાંઠ_Var_Ref* nodep) override {
        જો (m_curTaskp && nodep->access().isWriteOrRW()
            && (nodep->varp()->hasUserInit() || !m_inInitialSetup)) {
            UINFO(9, "undriven capture direct write in " << CaptureUtil::taskNameQ(m_curTaskp)
                                                         << " var=" << nodep->varp()->prettyNameQ()
                                                         << " at " << nodep->fileline());

            m_cap.noteDirectWrite(m_curTaskp, nodep->varp());
        }
        iterateChildrenConst(nodep);
    }

    શૂન્ય visit(Ast_ગાંઠ_FTask_Ref* nodep) override {
        // Record the call edge if resolved
        જો (m_curTaskp) {
            જો (અચલ Ast_ગાંઠ_FTask* અચલ calleep = nodep->taskp()) {
                UINFO(9, "undriven capture call edge " << CaptureUtil::taskNameQ(
                             m_curTaskp) << " -> " << CaptureUtil::taskNameQ(calleep));
                m_cap.noteCallEdge(m_curTaskp, calleep);
            }
        }
        iterateChildrenConst(nodep);  // still scan pins/args
    }

    શૂન્ય visit(Ast_ગાંઠ* nodep) override { iterateChildrenConst(nodep); }
};

}  // namespace

V3UndrivenCapture::V3UndrivenCapture(AstNetlist* netlistp) {
    gather(netlistp);

    // Compute summaries for all tasks
    માટે (અચલ auto& kv : m_info) (શૂન્ય)computeWriteSummary(kv.first);

    // Release the filter memory
    માટે (auto& kv : m_info) {
        kv.second.calleesSet.clear();
        kv.second.calleesSet.rehash(0);
        kv.second.directWritesSet.clear();
        kv.second.directWritesSet.rehash(0);
    }
}

શૂન્ય V3UndrivenCapture::gather(AstNetlist* netlistp) {
    // Walk netlist and populate m_info with direct writes + call edges
    CaptureVisitor{*આ, netlistp};
}

અચલ std::vector<AstVar*>& V3UndrivenCapture::writeSummary(અચલ Ast_ગાંઠ_FTask* taskp) {
    // Ensure entry exists even if empty
    (શૂન્ય)m_info[taskp];
    પાછા computeWriteSummary(taskp);
}

અચલ std::vector<AstVar*>& V3UndrivenCapture::computeWriteSummary(અચલ Ast_ગાંઠ_FTask* taskp) {
    FTaskInfo& info = m_info[taskp];

    જો (info.state == State::DONE) {
        UINFO(9, "undriven capture writeSummary cached size="
                     << info.writeSummary.size() << " for " << CaptureUtil::taskNameQ(taskp));
        પાછા info.writeSummary;
    }
    જો (info.state == State::VISITING) {
        UINFO(9, "undriven capture recursion detected at "
                     << CaptureUtil::taskNameQ(taskp)
                     << " returning directWrites size=" << info.directWrites.size());
        // Cycle detected. return directWrites only to guarantee termination.
        જો (info.writeSummary.empty()) info.writeSummary = info.directWrites;
        પાછા info.writeSummary;
    }

    info.state = State::VISITING;

    info.writeSummary.clear();

    // Prevent duplicates across all sources that can contribute to a write summary (direct writes
    // and call chains)
    std::unordered_set<AstVar*> seen;

    // Simple lambda for filtering duplicates
    auto addVar = [&](AstVar* v) {
        જો (seen.ઉમેરો(v).second) info.writeSummary.ધકેલો_back(v);
    };

    // Start with direct writes
    માટે (AstVar* v : info.directWrites) addVar(v);

    // Add callee summaries
    માટે (અચલ Ast_ગાંઠ_FTask* calleep : info.callees) {
        જો (m_info.find(calleep) == m_info.end()) ચાલુ;
        અચલ std::vector<AstVar*>& sub = computeWriteSummary(calleep);
        માટે (AstVar* v : sub) addVar(v);
    }

    UINFO(9, "undriven capture writeSummary computed size=" << info.writeSummary.size() << " for "
                                                            << CaptureUtil::taskNameQ(taskp));

    // We are done, so set the m_info state correctly and return the vector of variables
    info.state = State::DONE;
    પાછા info.writeSummary;
}

શૂન્ય V3UndrivenCapture::noteTask(અચલ Ast_ગાંઠ_FTask* taskp) { (શૂન્ય)m_info[taskp]; }

શૂન્ય V3UndrivenCapture::noteDirectWrite(અચલ Ast_ગાંઠ_FTask* taskp, AstVar* varp) {
    FTaskInfo& info = m_info[taskp];

    // Exclude function return variable (not an externally visible side-effect)
    AstVar* અચલ retVarp = VN_CAST(taskp->fvarp(), Var);
    જો (retVarp && varp == retVarp) પાછા;

    // Filter out duplicates.
    જો (info.directWritesSet.ઉમેરો(varp).second) info.directWrites.ધકેલો_back(varp);
}

શૂન્ય V3UndrivenCapture::noteCallEdge(અચલ Ast_ગાંઠ_FTask* callerp, અચલ Ast_ગાંઠ_FTask* calleep) {
    FTaskInfo& callerInfo = m_info[callerp];
    // Prevents duplicate entries from being appended, if calleep already exists then insert will
    // return false, and then is not inserted into the callees vector.
    જો (callerInfo.calleesSet.ઉમેરો(calleep).second) { callerInfo.callees.ધકેલો_back(calleep); }
    // Ensure callee entry exists, if already exists then this is a no-op.  unordered_map<> so
    // cheap.
    (શૂન્ય)m_info[calleep];
}
