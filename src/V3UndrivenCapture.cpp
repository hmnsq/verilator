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

#অন্তর্ভাৱ "V3UndrivenCapture.h"

#অন্তর্ভাৱ "V3Error.h"
#অন্তর্ভাৱ "V3Global.h"

VL_DEFINE_DEBUG_FUNCTIONS;

নামাকাশ {

শ্ৰেণী CaptureUtil final {
সার্ৱজনিক:
    স্থির std::string taskNameQ(ধ্রুৱ Ast_গ্ৰন্থি_FTask* taskp) {
        যদি (!taskp) ঘূৰাই "<null>";
        ঘূৰাই taskp->prettyNameQ();
    }
};

শ্ৰেণী CaptureVisitor final : সার্ৱজনিক VNVisitorConst {
    V3UndrivenCapture& m_cap;
    ধ্রুৱ Ast_গ্ৰন্থি_FTask* m_curTaskp = nullptr;  // Current task
    তার্কিক m_inInitialSetup = মিছা;  // In InitialAutomatic*/InitialStatic* assignment LHS

সার্ৱজনিক:
    explicit CaptureVisitor(V3UndrivenCapture& cap, AstNetlist* netlistp)
        : m_cap{cap} {
        iterateConst(netlistp);
    }

গুপ্ত:
    // Visit a task/function definition and collect direct writes and direct callees.
    শূন্য visit(Ast_গ্ৰন্থি_FTask* nodep) override {
        VL_RESTORER(m_curTaskp);
        m_curTaskp = nodep;
        UINFO(9, "undriven capture enter ftask " << nodep << " " << nodep->prettyNameQ());
        m_cap.noteTask(nodep);
        iterateAndNextConstNull(nodep->stmtsp());
    }

    শূন্য visit(AstInitialAutomaticStmt* nodep) override {
        VL_RESTORER(m_inInitialSetup);
        m_inInitialSetup = সঁচা;
        iterateChildrenConst(nodep);
    }
    শূন্য visit(AstInitialStaticStmt* nodep) override {
        VL_RESTORER(m_inInitialSetup);
        m_inInitialSetup = সঁচা;
        iterateChildrenConst(nodep);
    }
    শূন্য visit(AstAssign* nodep) override {
        {
            VL_RESTORER(m_inInitialSetup);
            m_inInitialSetup = মিছা;
            iterateConst(nodep->rhsp());
        }
        iterateConst(nodep->lhsp());
    }

    শূন্য visit(Ast_গ্ৰন্থি_Var_Ref* nodep) override {
        যদি (m_curTaskp && nodep->access().isWriteOrRW()
            && (nodep->varp()->hasUserInit() || !m_inInitialSetup)) {
            UINFO(9, "undriven capture direct write in " << CaptureUtil::taskNameQ(m_curTaskp)
                                                         << " var=" << nodep->varp()->prettyNameQ()
                                                         << " at " << nodep->fileline());

            m_cap.noteDirectWrite(m_curTaskp, nodep->varp());
        }
        iterateChildrenConst(nodep);
    }

    শূন্য visit(Ast_গ্ৰন্থি_FTask_Ref* nodep) override {
        // Record the call edge if resolved
        যদি (m_curTaskp) {
            যদি (ধ্রুৱ Ast_গ্ৰন্থি_FTask* ধ্রুৱ calleep = nodep->taskp()) {
                UINFO(9, "undriven capture call edge " << CaptureUtil::taskNameQ(
                             m_curTaskp) << " -> " << CaptureUtil::taskNameQ(calleep));
                m_cap.noteCallEdge(m_curTaskp, calleep);
            }
        }
        iterateChildrenConst(nodep);  // still scan pins/args
    }

    শূন্য visit(Ast_গ্ৰন্থি* nodep) override { iterateChildrenConst(nodep); }
};

}  // namespace

V3UndrivenCapture::V3UndrivenCapture(AstNetlist* netlistp) {
    gather(netlistp);

    // Compute summaries for all tasks
    বাবে (ধ্রুৱ auto& kv : m_info) (শূন্য)computeWriteSummary(kv.first);

    // Release the filter memory
    বাবে (auto& kv : m_info) {
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

ধ্রুৱ std::vector<AstVar*>& V3UndrivenCapture::writeSummary(ধ্রুৱ Ast_গ্ৰন্থি_FTask* taskp) {
    // Ensure entry exists even if empty
    (শূন্য)m_info[taskp];
    ঘূৰাই computeWriteSummary(taskp);
}

ধ্রুৱ std::vector<AstVar*>& V3UndrivenCapture::computeWriteSummary(ধ্রুৱ Ast_গ্ৰন্থি_FTask* taskp) {
    FTaskInfo& info = m_info[taskp];

    যদি (info.state == State::DONE) {
        UINFO(9, "undriven capture writeSummary cached size="
                     << info.writeSummary.size() << " for " << CaptureUtil::taskNameQ(taskp));
        ঘূৰাই info.writeSummary;
    }
    যদি (info.state == State::VISITING) {
        UINFO(9, "undriven capture recursion detected at "
                     << CaptureUtil::taskNameQ(taskp)
                     << " returning directWrites size=" << info.directWrites.size());
        // Cycle detected. return directWrites only to guarantee termination.
        যদি (info.writeSummary.empty()) info.writeSummary = info.directWrites;
        ঘূৰাই info.writeSummary;
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
    বাবে (AstVar* v : info.directWrites) addVar(v);

    // Add callee summaries
    বাবে (ধ্রুৱ Ast_গ্ৰন্থি_FTask* calleep : info.callees) {
        যদি (m_info.find(calleep) == m_info.end()) অনুৱর্ত;
        ধ্রুৱ std::vector<AstVar*>& sub = computeWriteSummary(calleep);
        বাবে (AstVar* v : sub) addVar(v);
    }

    UINFO(9, "undriven capture writeSummary computed size=" << info.writeSummary.size() << " for "
                                                            << CaptureUtil::taskNameQ(taskp));

    // We are done, so set the m_info state correctly and return the vector of variables
    info.state = State::DONE;
    ঘূৰাই info.writeSummary;
}

শূন্য V3UndrivenCapture::noteTask(ধ্রুৱ Ast_গ্ৰন্থি_FTask* taskp) { (শূন্য)m_info[taskp]; }

শূন্য V3UndrivenCapture::noteDirectWrite(ধ্রুৱ Ast_গ্ৰন্থি_FTask* taskp, AstVar* varp) {
    FTaskInfo& info = m_info[taskp];

    // Exclude function return variable (not an externally visible side-effect)
    AstVar* ধ্রুৱ retVarp = VN_CAST(taskp->fvarp(), Var);
    যদি (retVarp && varp == retVarp) ঘূৰাই;

    // Filter out duplicates.
    যদি (info.directWritesSet.ঢোকাও(varp).second) info.directWrites.ঠেলো_back(varp);
}

শূন্য V3UndrivenCapture::noteCallEdge(ধ্রুৱ Ast_গ্ৰন্থি_FTask* callerp, ধ্রুৱ Ast_গ্ৰন্থি_FTask* calleep) {
    FTaskInfo& callerInfo = m_info[callerp];
    // Prevents duplicate entries from being appended, if calleep already exists then insert will
    // return false, and then is not inserted into the callees vector.
    যদি (callerInfo.calleesSet.ঢোকাও(calleep).second) { callerInfo.callees.ঠেলো_back(calleep); }
    // Ensure callee entry exists, if already exists then this is a no-op.  unordered_map<> so
    // cheap.
    (শূন্য)m_info[calleep];
}
