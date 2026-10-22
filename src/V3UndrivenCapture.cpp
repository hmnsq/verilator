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

#شامل "V3UndrivenCapture.h"

#شامل "V3Error.h"
#شامل "V3Global.h"

VL_DEFINE_DEBUG_FUNCTIONS;

فضاینام {

رده CaptureUtil final {
عمومی:
    ایستا std::string taskNameQ(ثابت Ast_گره_FTask* taskp) {
        اگر (!taskp) بازگشت "<null>";
        بازگشت taskp->prettyNameQ();
    }
};

رده CaptureVisitor final : عمومی VNVisitorConst {
    V3UndrivenCapture& m_cap;
    ثابت Ast_گره_FTask* m_curTaskp = nullptr;  // Current task
    منطقی m_inInitialSetup = نادرست;  // In InitialAutomatic*/InitialStatic* assignment LHS

عمومی:
    explicit CaptureVisitor(V3UndrivenCapture& cap, AstNetlist* netlistp)
        : m_cap{cap} {
        iterateConst(netlistp);
    }

خصوصی:
    // Visit a task/function definition and collect direct writes and direct callees.
    تهی visit(Ast_گره_FTask* nodep) override {
        VL_RESTORER(m_curTaskp);
        m_curTaskp = nodep;
        UINFO(9, "undriven capture enter ftask " << nodep << " " << nodep->prettyNameQ());
        m_cap.noteTask(nodep);
        iterateAndNextConstNull(nodep->stmtsp());
    }

    تهی visit(AstInitialAutomaticStmt* nodep) override {
        VL_RESTORER(m_inInitialSetup);
        m_inInitialSetup = درست;
        iterateChildrenConst(nodep);
    }
    تهی visit(AstInitialStaticStmt* nodep) override {
        VL_RESTORER(m_inInitialSetup);
        m_inInitialSetup = درست;
        iterateChildrenConst(nodep);
    }
    تهی visit(AstAssign* nodep) override {
        {
            VL_RESTORER(m_inInitialSetup);
            m_inInitialSetup = نادرست;
            iterateConst(nodep->rhsp());
        }
        iterateConst(nodep->lhsp());
    }

    تهی visit(Ast_گره_Var_Ref* nodep) override {
        اگر (m_curTaskp && nodep->access().isWriteOrRW()
            && (nodep->varp()->hasUserInit() || !m_inInitialSetup)) {
            UINFO(9, "undriven capture direct write in " << CaptureUtil::taskNameQ(m_curTaskp)
                                                         << " var=" << nodep->varp()->prettyNameQ()
                                                         << " at " << nodep->fileline());

            m_cap.noteDirectWrite(m_curTaskp, nodep->varp());
        }
        iterateChildrenConst(nodep);
    }

    تهی visit(Ast_گره_FTask_Ref* nodep) override {
        // Record the call edge if resolved
        اگر (m_curTaskp) {
            اگر (ثابت Ast_گره_FTask* ثابت calleep = nodep->taskp()) {
                UINFO(9, "undriven capture call edge " << CaptureUtil::taskNameQ(
                             m_curTaskp) << " -> " << CaptureUtil::taskNameQ(calleep));
                m_cap.noteCallEdge(m_curTaskp, calleep);
            }
        }
        iterateChildrenConst(nodep);  // still scan pins/args
    }

    تهی visit(Ast_گره* nodep) override { iterateChildrenConst(nodep); }
};

}  // namespace

V3UndrivenCapture::V3UndrivenCapture(AstNetlist* netlistp) {
    gather(netlistp);

    // Compute summaries for all tasks
    برای (ثابت auto& kv : m_info) (تهی)computeWriteSummary(kv.first);

    // Release the filter memory
    برای (auto& kv : m_info) {
        kv.second.calleesSet.clear();
        kv.second.calleesSet.rehash(0);
        kv.second.directWritesSet.clear();
        kv.second.directWritesSet.rehash(0);
    }
}

تهی V3UndrivenCapture::gather(AstNetlist* netlistp) {
    // Walk netlist and populate m_info with direct writes + call edges
    CaptureVisitor{*این, netlistp};
}

ثابت std::vector<AstVar*>& V3UndrivenCapture::writeSummary(ثابت Ast_گره_FTask* taskp) {
    // Ensure entry exists even if empty
    (تهی)m_info[taskp];
    بازگشت computeWriteSummary(taskp);
}

ثابت std::vector<AstVar*>& V3UndrivenCapture::computeWriteSummary(ثابت Ast_گره_FTask* taskp) {
    FTaskInfo& info = m_info[taskp];

    اگر (info.state == State::DONE) {
        UINFO(9, "undriven capture writeSummary cached size="
                     << info.writeSummary.size() << " for " << CaptureUtil::taskNameQ(taskp));
        بازگشت info.writeSummary;
    }
    اگر (info.state == State::VISITING) {
        UINFO(9, "undriven capture recursion detected at "
                     << CaptureUtil::taskNameQ(taskp)
                     << " returning directWrites size=" << info.directWrites.size());
        // Cycle detected. return directWrites only to guarantee termination.
        اگر (info.writeSummary.empty()) info.writeSummary = info.directWrites;
        بازگشت info.writeSummary;
    }

    info.state = State::VISITING;

    info.writeSummary.clear();

    // Prevent duplicates across all sources that can contribute to a write summary (direct writes
    // and call chains)
    std::unordered_set<AstVar*> seen;

    // Simple lambda for filtering duplicates
    auto addVar = [&](AstVar* v) {
        اگر (seen.درج(v).second) info.writeSummary.افزودن_back(v);
    };

    // Start with direct writes
    برای (AstVar* v : info.directWrites) addVar(v);

    // Add callee summaries
    برای (ثابت Ast_گره_FTask* calleep : info.callees) {
        اگر (m_info.find(calleep) == m_info.end()) ادامه;
        ثابت std::vector<AstVar*>& sub = computeWriteSummary(calleep);
        برای (AstVar* v : sub) addVar(v);
    }

    UINFO(9, "undriven capture writeSummary computed size=" << info.writeSummary.size() << " for "
                                                            << CaptureUtil::taskNameQ(taskp));

    // We are done, so set the m_info state correctly and return the vector of variables
    info.state = State::DONE;
    بازگشت info.writeSummary;
}

تهی V3UndrivenCapture::noteTask(ثابت Ast_گره_FTask* taskp) { (تهی)m_info[taskp]; }

تهی V3UndrivenCapture::noteDirectWrite(ثابت Ast_گره_FTask* taskp, AstVar* varp) {
    FTaskInfo& info = m_info[taskp];

    // Exclude function return variable (not an externally visible side-effect)
    AstVar* ثابت retVarp = VN_CAST(taskp->fvarp(), Var);
    اگر (retVarp && varp == retVarp) بازگشت;

    // Filter out duplicates.
    اگر (info.directWritesSet.درج(varp).second) info.directWrites.افزودن_back(varp);
}

تهی V3UndrivenCapture::noteCallEdge(ثابت Ast_گره_FTask* callerp, ثابت Ast_گره_FTask* calleep) {
    FTaskInfo& callerInfo = m_info[callerp];
    // Prevents duplicate entries from being appended, if calleep already exists then insert will
    // return false, and then is not inserted into the callees vector.
    اگر (callerInfo.calleesSet.درج(calleep).second) { callerInfo.callees.افزودن_back(calleep); }
    // Ensure callee entry exists, if already exists then this is a no-op.  unordered_map<> so
    // cheap.
    (تهی)m_info[calleep];
}
