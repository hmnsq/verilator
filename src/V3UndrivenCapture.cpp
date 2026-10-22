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

#అన్తర్భావ "V3UndrivenCapture.h"

#అన్తర్భావ "V3Error.h"
#అన్తర్భావ "V3Global.h"

VL_DEFINE_DEBUG_FUNCTIONS;

నామాకాశ {

తరగతి CaptureUtil final {
బహిరంగ:
    స్థిర std::string taskNameQ(స్థిరరాశి Ast_కణుపు_FTask* taskp) {
        ఒకవేళ (!taskp) తిరిగి "<null>";
        తిరిగి taskp->prettyNameQ();
    }
};

తరగతి CaptureVisitor final : బహిరంగ VNVisitorConst {
    V3UndrivenCapture& m_cap;
    స్థిరరాశి Ast_కణుపు_FTask* m_curTaskp = శూన్యసూచిక;  // Current task
    తార్కిక m_లోపల_Initial_అమరిక = అబద్ధం;  // In InitialAutomatic*/InitialStatic* assignment LHS

బహిరంగ:
    స్పష్టమైన CaptureVisitor(V3UndrivenCapture& cap, AstNetlist* netlistp)
        : m_cap{cap} {
        iterateConst(netlistp);
    }

అంతరంగ:
    // Visit a task/function definition and collect direct writes and direct callees.
    శూన్య సందర్శించు(Ast_కణుపు_FTask* nodep) override {
        VL_RESTORER(m_curTaskp);
        m_curTaskp = nodep;
        UINFO(9, "undriven capture enter ftask " << nodep << " " << nodep->prettyNameQ());
        m_cap.noteTask(nodep);
        iterate_And_తదుపరి_Const_Null(nodep->stmtsp());
    }

    శూన్య సందర్శించు(AstInitialAutomaticStmt* nodep) override {
        VL_RESTORER(m_లోపల_Initial_అమరిక);
        m_లోపల_Initial_అమరిక = నిజం;
        iterateChildrenConst(nodep);
    }
    శూన్య సందర్శించు(AstInitialStaticStmt* nodep) override {
        VL_RESTORER(m_లోపల_Initial_అమరిక);
        m_లోపల_Initial_అమరిక = నిజం;
        iterateChildrenConst(nodep);
    }
    శూన్య సందర్శించు(Ast_కేటాయించు* nodep) override {
        {
            VL_RESTORER(m_లోపల_Initial_అమరిక);
            m_లోపల_Initial_అమరిక = అబద్ధం;
            iterateConst(nodep->rhsp());
        }
        iterateConst(nodep->lhsp());
    }

    శూన్య సందర్శించు(Ast_కణుపు_Var_Ref* nodep) override {
        ఒకవేళ (m_curTaskp && nodep->access().isWriteOrRW()
            && (nodep->varp()->hasUserInit() || !m_లోపల_Initial_అమరిక)) {
            UINFO(9, "undriven capture direct write in " << CaptureUtil::taskNameQ(m_curTaskp)
                                                         << " var=" << nodep->varp()->prettyNameQ()
                                                         << " at " << nodep->fileline());

            m_cap.note_ప్రత్యక్ష_Write(m_curTaskp, nodep->varp());
        }
        iterateChildrenConst(nodep);
    }

    శూన్య సందర్శించు(Ast_కణుపు_FTask_Ref* nodep) override {
        // Record the call edge if resolved
        ఒకవేళ (m_curTaskp) {
            ఒకవేళ (స్థిరరాశి Ast_కణుపు_FTask* స్థిరరాశి calleep = nodep->taskp()) {
                UINFO(9, "undriven capture call edge " << CaptureUtil::taskNameQ(
                             m_curTaskp) << " -> " << CaptureUtil::taskNameQ(calleep));
                m_cap.note_Call_అంచు(m_curTaskp, calleep);
            }
        }
        iterateChildrenConst(nodep);  // still scan pins/args
    }

    శూన్య సందర్శించు(Ast_కణుపు* nodep) override { iterateChildrenConst(nodep); }
};

}  // namespace

V3UndrivenCapture::V3UndrivenCapture(AstNetlist* netlistp) {
    gather(netlistp);

    // Compute summaries for all tasks
    కొరకు (స్థిరరాశి స్వయంచాలక& kv : m_info) (శూన్య)computeWriteSummary(kv.మొదటి);

    // Release the filter memory
    కొరకు (స్వయంచాలక& kv : m_info) {
        kv.రెండవ.callees_సమితి.clear();
        kv.రెండవ.callees_సమితి.rehash(0);
        kv.రెండవ.ప్రత్యక్ష_Writes_సమితి.clear();
        kv.రెండవ.ప్రత్యక్ష_Writes_సమితి.rehash(0);
    }
}

శూన్య V3UndrivenCapture::gather(AstNetlist* netlistp) {
    // Walk netlist and populate m_info with direct writes + call edges
    CaptureVisitor{*ఇది, netlistp};
}

స్థిరరాశి std::సదిశ<AstVar*>& V3UndrivenCapture::writeSummary(స్థిరరాశి Ast_కణుపు_FTask* taskp) {
    // Ensure entry exists even if empty
    (శూన్య)m_info[taskp];
    తిరిగి computeWriteSummary(taskp);
}

స్థిరరాశి std::సదిశ<AstVar*>& V3UndrivenCapture::computeWriteSummary(స్థిరరాశి Ast_కణుపు_FTask* taskp) {
    FTaskInfo& info = m_info[taskp];

    ఒకవేళ (info.state == State::పూర్తి) {
        UINFO(9, "undriven capture writeSummary cached size="
                     << info.writeSummary.కొలత() << " for " << CaptureUtil::taskNameQ(taskp));
        తిరిగి info.writeSummary;
    }
    ఒకవేళ (info.state == State::VISITING) {
        UINFO(9, "undriven capture recursion detected at "
                     << CaptureUtil::taskNameQ(taskp)
                     << " returning directWrites size=" << info.ప్రత్యక్ష_Writes.కొలత());
        // Cycle detected. return directWrites only to guarantee termination.
        ఒకవేళ (info.writeSummary.ఖాళీలు()) info.writeSummary = info.ప్రత్యక్ష_Writes;
        తిరిగి info.writeSummary;
    }

    info.state = State::VISITING;

    info.writeSummary.clear();

    // Prevent duplicates across all sources that can contribute to a write summary (direct writes
    // and call chains)
    std::unordered_సమితి<AstVar*> చూసిన;

    // Simple lambda for filtering duplicates
    స్వయంచాలక కూడు_Var = [&](AstVar* v) {
        ఒకవేళ (చూసిన.చొప్పించు(v).రెండవ) info.writeSummary.నెట్టు_వెనుక(v);
    };

    // Start with direct writes
    కొరకు (AstVar* v : info.ప్రత్యక్ష_Writes) కూడు_Var(v);

    // Add callee summaries
    కొరకు (స్థిరరాశి Ast_కణుపు_FTask* calleep : info.callees) {
        ఒకవేళ (m_info.కనుగొను(calleep) == m_info.చివర()) కొనసాగు;
        స్థిరరాశి std::సదిశ<AstVar*>& sub = computeWriteSummary(calleep);
        కొరకు (AstVar* v : sub) కూడు_Var(v);
    }

    UINFO(9, "undriven capture writeSummary computed size=" << info.writeSummary.కొలత() << " for "
                                                            << CaptureUtil::taskNameQ(taskp));

    // We are done, so set the m_info state correctly and return the vector of variables
    info.state = State::పూర్తి;
    తిరిగి info.writeSummary;
}

శూన్య V3UndrivenCapture::noteTask(స్థిరరాశి Ast_కణుపు_FTask* taskp) { (శూన్య)m_info[taskp]; }

శూన్య V3UndrivenCapture::note_ప్రత్యక్ష_Write(స్థిరరాశి Ast_కణుపు_FTask* taskp, AstVar* varp) {
    FTaskInfo& info = m_info[taskp];

    // Exclude function return variable (not an externally visible side-effect)
    AstVar* స్థిరరాశి retVarp = VN_CAST(taskp->fvarp(), Var);
    ఒకవేళ (retVarp && varp == retVarp) తిరిగి;

    // Filter out duplicates.
    ఒకవేళ (info.ప్రత్యక్ష_Writes_సమితి.చొప్పించు(varp).రెండవ) info.ప్రత్యక్ష_Writes.నెట్టు_వెనుక(varp);
}

శూన్య V3UndrivenCapture::note_Call_అంచు(స్థిరరాశి Ast_కణుపు_FTask* callerp, స్థిరరాశి Ast_కణుపు_FTask* calleep) {
    FTaskInfo& callerInfo = m_info[callerp];
    // Prevents duplicate entries from being appended, if calleep already exists then insert will
    // return false, and then is not inserted into the callees vector.
    ఒకవేళ (callerInfo.callees_సమితి.చొప్పించు(calleep).రెండవ) { callerInfo.callees.నెట్టు_వెనుక(calleep); }
    // Ensure callee entry exists, if already exists then this is a no-op.  unordered_map<> so
    // cheap.
    (శూన్య)m_info[calleep];
}
