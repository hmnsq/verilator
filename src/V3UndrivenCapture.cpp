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

#অন্তর্ভাব "V3UndrivenCapture.h"

#অন্তর্ভাব "V3Error.h"
#অন্তর্ভাব "V3Global.h"

VL_DEFINE_DEBUG_FUNCTIONS;

নামাকাশ {

বর্গ CaptureUtil final {
সার্বজনিক:
    স্থির std::string taskNameQ(ধ্রুব Ast_গ্রন্থি_FTask* taskp) {
        যদি (!taskp) প্রত্যাগম "<null>";
        প্রত্যাগম taskp->prettyNameQ();
    }
};

বর্গ CaptureVisitor final : সার্বজনিক VNVisitorConst {
    V3UndrivenCapture& m_cap;
    ধ্রুব Ast_গ্রন্থি_FTask* m_curTaskp = nullptr;  // Current task
    তার্কিক m_inInitialSetup = অসত্য;  // In InitialAutomatic*/InitialStatic* assignment LHS

সার্বজনিক:
    explicit CaptureVisitor(V3UndrivenCapture& cap, AstNetlist* netlistp)
        : m_cap{cap} {
        iterateConst(netlistp);
    }

গুপ্ত:
    // Visit a task/function definition and collect direct writes and direct callees.
    শূন্য visit(Ast_গ্রন্থি_FTask* nodep) override {
        VL_RESTORER(m_curTaskp);
        m_curTaskp = nodep;
        UINFO(9, "undriven capture enter ftask " << nodep << " " << nodep->prettyNameQ());
        m_cap.noteTask(nodep);
        iterateAndNextConstNull(nodep->stmtsp());
    }

    শূন্য visit(AstInitialAutomaticStmt* nodep) override {
        VL_RESTORER(m_inInitialSetup);
        m_inInitialSetup = সত্য;
        iterateChildrenConst(nodep);
    }
    শূন্য visit(AstInitialStaticStmt* nodep) override {
        VL_RESTORER(m_inInitialSetup);
        m_inInitialSetup = সত্য;
        iterateChildrenConst(nodep);
    }
    শূন্য visit(AstAssign* nodep) override {
        {
            VL_RESTORER(m_inInitialSetup);
            m_inInitialSetup = অসত্য;
            iterateConst(nodep->rhsp());
        }
        iterateConst(nodep->lhsp());
    }

    শূন্য visit(Ast_গ্রন্থি_Var_Ref* nodep) override {
        যদি (m_curTaskp && nodep->access().isWriteOrRW()
            && (nodep->varp()->hasUserInit() || !m_inInitialSetup)) {
            UINFO(9, "undriven capture direct write in " << CaptureUtil::taskNameQ(m_curTaskp)
                                                         << " var=" << nodep->varp()->prettyNameQ()
                                                         << " at " << nodep->fileline());

            m_cap.noteDirectWrite(m_curTaskp, nodep->varp());
        }
        iterateChildrenConst(nodep);
    }

    শূন্য visit(Ast_গ্রন্থি_FTask_Ref* nodep) override {
        // Record the call edge if resolved
        যদি (m_curTaskp) {
            যদি (ধ্রুব Ast_গ্রন্থি_FTask* ধ্রুব calleep = nodep->taskp()) {
                UINFO(9, "undriven capture call edge " << CaptureUtil::taskNameQ(
                             m_curTaskp) << " -> " << CaptureUtil::taskNameQ(calleep));
                m_cap.noteCallEdge(m_curTaskp, calleep);
            }
        }
        iterateChildrenConst(nodep);  // still scan pins/args
    }

    শূন্য visit(Ast_গ্রন্থি* nodep) override { iterateChildrenConst(nodep); }
};

}  // namespace

V3UndrivenCapture::V3UndrivenCapture(AstNetlist* netlistp) {
    gather(netlistp);

    // Compute summaries for all tasks
    প্রতি (ধ্রুব auto& kv : m_info) (শূন্য)computeWriteSummary(kv.first);

    // Release the filter memory
    প্রতি (auto& kv : m_info) {
        kv.second.calleesSet.clear();
        kv.second.calleesSet.rehash(0);
        kv.second.directWritesSet.clear();
        kv.second.directWritesSet.rehash(0);
    }
}

শূন্য V3UndrivenCapture::gather(AstNetlist* netlistp) {
    // Walk netlist and populate m_info with direct writes + call edges
    CaptureVisitor{*এতত্, netlistp};
}

ধ্রুব std::vector<AstVar*>& V3UndrivenCapture::writeSummary(ধ্রুব Ast_গ্রন্থি_FTask* taskp) {
    // Ensure entry exists even if empty
    (শূন্য)m_info[taskp];
    প্রত্যাগম computeWriteSummary(taskp);
}

ধ্রুব std::vector<AstVar*>& V3UndrivenCapture::computeWriteSummary(ধ্রুব Ast_গ্রন্থি_FTask* taskp) {
    FTaskInfo& info = m_info[taskp];

    যদি (info.state == State::DONE) {
        UINFO(9, "undriven capture writeSummary cached size="
                     << info.writeSummary.size() << " for " << CaptureUtil::taskNameQ(taskp));
        প্রত্যাগম info.writeSummary;
    }
    যদি (info.state == State::VISITING) {
        UINFO(9, "undriven capture recursion detected at "
                     << CaptureUtil::taskNameQ(taskp)
                     << " returning directWrites size=" << info.directWrites.size());
        // Cycle detected. return directWrites only to guarantee termination.
        যদি (info.writeSummary.empty()) info.writeSummary = info.directWrites;
        প্রত্যাগম info.writeSummary;
    }

    info.state = State::VISITING;

    info.writeSummary.clear();

    // Prevent duplicates across all sources that can contribute to a write summary (direct writes
    // and call chains)
    std::unordered_set<AstVar*> seen;

    // Simple lambda for filtering duplicates
    auto addVar = [&](AstVar* v) {
        যদি (seen.ঢোকাও(v).second) info.writeSummary.ঠেলো_back(v);
    };

    // Start with direct writes
    প্রতি (AstVar* v : info.directWrites) addVar(v);

    // Add callee summaries
    প্রতি (ধ্রুব Ast_গ্রন্থি_FTask* calleep : info.callees) {
        যদি (m_info.find(calleep) == m_info.end()) অনুবর্ত;
        ধ্রুব std::vector<AstVar*>& sub = computeWriteSummary(calleep);
        প্রতি (AstVar* v : sub) addVar(v);
    }

    UINFO(9, "undriven capture writeSummary computed size=" << info.writeSummary.size() << " for "
                                                            << CaptureUtil::taskNameQ(taskp));

    // We are done, so set the m_info state correctly and return the vector of variables
    info.state = State::DONE;
    প্রত্যাগম info.writeSummary;
}

শূন্য V3UndrivenCapture::noteTask(ধ্রুব Ast_গ্রন্থি_FTask* taskp) { (শূন্য)m_info[taskp]; }

শূন্য V3UndrivenCapture::noteDirectWrite(ধ্রুব Ast_গ্রন্থি_FTask* taskp, AstVar* varp) {
    FTaskInfo& info = m_info[taskp];

    // Exclude function return variable (not an externally visible side-effect)
    AstVar* ধ্রুব retVarp = VN_CAST(taskp->fvarp(), Var);
    যদি (retVarp && varp == retVarp) প্রত্যাগম;

    // Filter out duplicates.
    যদি (info.directWritesSet.ঢোকাও(varp).second) info.directWrites.ঠেলো_back(varp);
}

শূন্য V3UndrivenCapture::noteCallEdge(ধ্রুব Ast_গ্রন্থি_FTask* callerp, ধ্রুব Ast_গ্রন্থি_FTask* calleep) {
    FTaskInfo& callerInfo = m_info[callerp];
    // Prevents duplicate entries from being appended, if calleep already exists then insert will
    // return false, and then is not inserted into the callees vector.
    যদি (callerInfo.calleesSet.ঢোকাও(calleep).second) { callerInfo.callees.ঠেলো_back(calleep); }
    // Ensure callee entry exists, if already exists then this is a no-op.  unordered_map<> so
    // cheap.
    (শূন্য)m_info[calleep];
}
