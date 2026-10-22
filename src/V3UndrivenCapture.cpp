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

#ضمن "V3UndrivenCapture.h"

#ضمن "V3Error.h"
#ضمن "V3Global.h"

VL_DEFINE_DEBUG_FUNCTIONS;

نطاق {

صنف CaptureUtil final {
عام:
    ساكن std::string taskNameQ(ثابت Ast_گرێ_FTask* taskp) {
        إذا (!taskp) أرجع "<null>";
        أرجع taskp->prettyNameQ();
    }
};

صنف CaptureVisitor final : عام VNVisitorConst {
    V3UndrivenCapture& m_cap;
    ثابت Ast_گرێ_FTask* m_curTaskp = nullptr;  // Current task
    منطقي m_inInitialSetup = خطأ;  // In InitialAutomatic*/InitialStatic* assignment LHS

عام:
    explicit CaptureVisitor(V3UndrivenCapture& cap, AstNetlist* netlistp)
        : m_cap{cap} {
        iterateConst(netlistp);
    }

خاص:
    // Visit a task/function definition and collect direct writes and direct callees.
    فراغ visit(Ast_گرێ_FTask* nodep) override {
        VL_RESTORER(m_curTaskp);
        m_curTaskp = nodep;
        UINFO(9, "undriven capture enter ftask " << nodep << " " << nodep->prettyNameQ());
        m_cap.noteTask(nodep);
        iterateAndNextConstNull(nodep->stmtsp());
    }

    فراغ visit(AstInitialAutomaticStmt* nodep) override {
        VL_RESTORER(m_inInitialSetup);
        m_inInitialSetup = true;
        iterateChildrenConst(nodep);
    }
    فراغ visit(AstInitialStaticStmt* nodep) override {
        VL_RESTORER(m_inInitialSetup);
        m_inInitialSetup = true;
        iterateChildrenConst(nodep);
    }
    فراغ visit(AstAssign* nodep) override {
        {
            VL_RESTORER(m_inInitialSetup);
            m_inInitialSetup = خطأ;
            iterateConst(nodep->rhsp());
        }
        iterateConst(nodep->lhsp());
    }

    فراغ visit(Ast_گرێ_Var_Ref* nodep) override {
        إذا (m_curTaskp && nodep->access().isWriteOrRW()
            && (nodep->varp()->hasUserInit() || !m_inInitialSetup)) {
            UINFO(9, "undriven capture direct write in " << CaptureUtil::taskNameQ(m_curTaskp)
                                                         << " var=" << nodep->varp()->prettyNameQ()
                                                         << " at " << nodep->fileline());

            m_cap.noteDirectWrite(m_curTaskp, nodep->varp());
        }
        iterateChildrenConst(nodep);
    }

    فراغ visit(Ast_گرێ_FTask_Ref* nodep) override {
        // Record the call edge if resolved
        إذا (m_curTaskp) {
            إذا (ثابت Ast_گرێ_FTask* ثابت calleep = nodep->taskp()) {
                UINFO(9, "undriven capture call edge " << CaptureUtil::taskNameQ(
                             m_curTaskp) << " -> " << CaptureUtil::taskNameQ(calleep));
                m_cap.noteCallEdge(m_curTaskp, calleep);
            }
        }
        iterateChildrenConst(nodep);  // still scan pins/args
    }

    فراغ visit(Ast_گرێ* nodep) override { iterateChildrenConst(nodep); }
};

}  // namespace

V3UndrivenCapture::V3UndrivenCapture(AstNetlist* netlistp) {
    gather(netlistp);

    // Compute summaries for all tasks
    لكل (ثابت auto& kv : m_info) (فراغ)computeWriteSummary(kv.first);

    // Release the filter memory
    لكل (auto& kv : m_info) {
        kv.second.calleesSet.clear();
        kv.second.calleesSet.rehash(0);
        kv.second.directWritesSet.clear();
        kv.second.directWritesSet.rehash(0);
    }
}

فراغ V3UndrivenCapture::gather(AstNetlist* netlistp) {
    // Walk netlist and populate m_info with direct writes + call edges
    CaptureVisitor{*هذا, netlistp};
}

ثابت std::vector<AstVar*>& V3UndrivenCapture::writeSummary(ثابت Ast_گرێ_FTask* taskp) {
    // Ensure entry exists even if empty
    (فراغ)m_info[taskp];
    أرجع computeWriteSummary(taskp);
}

ثابت std::vector<AstVar*>& V3UndrivenCapture::computeWriteSummary(ثابت Ast_گرێ_FTask* taskp) {
    FTaskInfo& info = m_info[taskp];

    إذا (info.state == State::DONE) {
        UINFO(9, "undriven capture writeSummary cached size="
                     << info.writeSummary.size() << " for " << CaptureUtil::taskNameQ(taskp));
        أرجع info.writeSummary;
    }
    إذا (info.state == State::VISITING) {
        UINFO(9, "undriven capture recursion detected at "
                     << CaptureUtil::taskNameQ(taskp)
                     << " returning directWrites size=" << info.directWrites.size());
        // Cycle detected. return directWrites only to guarantee termination.
        إذا (info.writeSummary.empty()) info.writeSummary = info.directWrites;
        أرجع info.writeSummary;
    }

    info.state = State::VISITING;

    info.writeSummary.clear();

    // Prevent duplicates across all sources that can contribute to a write summary (direct writes
    // and call chains)
    std::unordered_set<AstVar*> seen;

    // Simple lambda for filtering duplicates
    auto addVar = [&](AstVar* v) {
        إذا (seen.زیادکردن(v).second) info.writeSummary.پاڵنان_back(v);
    };

    // Start with direct writes
    لكل (AstVar* v : info.directWrites) addVar(v);

    // Add callee summaries
    لكل (ثابت Ast_گرێ_FTask* calleep : info.callees) {
        إذا (m_info.find(calleep) == m_info.end()) استمر;
        ثابت std::vector<AstVar*>& sub = computeWriteSummary(calleep);
        لكل (AstVar* v : sub) addVar(v);
    }

    UINFO(9, "undriven capture writeSummary computed size=" << info.writeSummary.size() << " for "
                                                            << CaptureUtil::taskNameQ(taskp));

    // We are done, so set the m_info state correctly and return the vector of variables
    info.state = State::DONE;
    أرجع info.writeSummary;
}

فراغ V3UndrivenCapture::noteTask(ثابت Ast_گرێ_FTask* taskp) { (فراغ)m_info[taskp]; }

فراغ V3UndrivenCapture::noteDirectWrite(ثابت Ast_گرێ_FTask* taskp, AstVar* varp) {
    FTaskInfo& info = m_info[taskp];

    // Exclude function return variable (not an externally visible side-effect)
    AstVar* ثابت retVarp = VN_CAST(taskp->fvarp(), Var);
    إذا (retVarp && varp == retVarp) أرجع;

    // Filter out duplicates.
    إذا (info.directWritesSet.زیادکردن(varp).second) info.directWrites.پاڵنان_back(varp);
}

فراغ V3UndrivenCapture::noteCallEdge(ثابت Ast_گرێ_FTask* callerp, ثابت Ast_گرێ_FTask* calleep) {
    FTaskInfo& callerInfo = m_info[callerp];
    // Prevents duplicate entries from being appended, if calleep already exists then insert will
    // return false, and then is not inserted into the callees vector.
    إذا (callerInfo.calleesSet.زیادکردن(calleep).second) { callerInfo.callees.پاڵنان_back(calleep); }
    // Ensure callee entry exists, if already exists then this is a no-op.  unordered_map<> so
    // cheap.
    (فراغ)m_info[calleep];
}
