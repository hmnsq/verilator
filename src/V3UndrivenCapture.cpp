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

نامخلا {

جماعت CaptureUtil final {
عوامی:
    ساکن std::string taskNameQ(ثابت Ast_گرہ_FTask* taskp) {
        اگر (!taskp) واپس "<null>";
        واپس taskp->prettyNameQ();
    }
};

جماعت CaptureVisitor final : عوامی VNVisitorConst {
    V3UndrivenCapture& m_cap;
    ثابت Ast_گرہ_FTask* m_curTaskp = خالی_اشاریہ;  // Current task
    منطقی m_اندر_Initial_تیاری = جھوٹ;  // In InitialAutomatic*/InitialStatic* assignment LHS

عوامی:
    صریح CaptureVisitor(V3UndrivenCapture& cap, AstNetlist* netlistp)
        : m_cap{cap} {
        iterateConst(netlistp);
    }

نجی:
    // Visit a task/function definition and collect direct writes and direct callees.
    خالی دورہ(Ast_گرہ_FTask* nodep) override {
        VL_RESTORER(m_curTaskp);
        m_curTaskp = nodep;
        UINFO(9, "undriven capture enter ftask " << nodep << " " << nodep->prettyNameQ());
        m_cap.noteTask(nodep);
        iterate_And_اگلا_Const_Null(nodep->stmtsp());
    }

    خالی دورہ(AstInitialAutomaticStmt* nodep) override {
        VL_RESTORER(m_اندر_Initial_تیاری);
        m_اندر_Initial_تیاری = سچ;
        iterateChildrenConst(nodep);
    }
    خالی دورہ(AstInitialStaticStmt* nodep) override {
        VL_RESTORER(m_اندر_Initial_تیاری);
        m_اندر_Initial_تیاری = سچ;
        iterateChildrenConst(nodep);
    }
    خالی دورہ(Ast_تفویض* nodep) override {
        {
            VL_RESTORER(m_اندر_Initial_تیاری);
            m_اندر_Initial_تیاری = جھوٹ;
            iterateConst(nodep->rhsp());
        }
        iterateConst(nodep->lhsp());
    }

    خالی دورہ(Ast_گرہ_Var_Ref* nodep) override {
        اگر (m_curTaskp && nodep->access().isWriteOrRW()
            && (nodep->varp()->hasUserInit() || !m_اندر_Initial_تیاری)) {
            UINFO(9, "undriven capture direct write in " << CaptureUtil::taskNameQ(m_curTaskp)
                                                         << " var=" << nodep->varp()->prettyNameQ()
                                                         << " at " << nodep->fileline());

            m_cap.note_براہ_راست_Write(m_curTaskp, nodep->varp());
        }
        iterateChildrenConst(nodep);
    }

    خالی دورہ(Ast_گرہ_FTask_Ref* nodep) override {
        // Record the call edge if resolved
        اگر (m_curTaskp) {
            اگر (ثابت Ast_گرہ_FTask* ثابت calleep = nodep->taskp()) {
                UINFO(9, "undriven capture call edge " << CaptureUtil::taskNameQ(
                             m_curTaskp) << " -> " << CaptureUtil::taskNameQ(calleep));
                m_cap.note_Call_کنارہ(m_curTaskp, calleep);
            }
        }
        iterateChildrenConst(nodep);  // still scan pins/args
    }

    خالی دورہ(Ast_گرہ* nodep) override { iterateChildrenConst(nodep); }
};

}  // namespace

V3UndrivenCapture::V3UndrivenCapture(AstNetlist* netlistp) {
    gather(netlistp);

    // Compute summaries for all tasks
    برائے (ثابت خودکار& kv : m_info) (خالی)computeWriteSummary(kv.پہلا);

    // Release the filter memory
    برائے (خودکار& kv : m_info) {
        kv.دوسرا.callees_سیٹ.clear();
        kv.دوسرا.callees_سیٹ.rehash(0);
        kv.دوسرا.براہ_راست_Writes_سیٹ.clear();
        kv.دوسرا.براہ_راست_Writes_سیٹ.rehash(0);
    }
}

خالی V3UndrivenCapture::gather(AstNetlist* netlistp) {
    // Walk netlist and populate m_info with direct writes + call edges
    CaptureVisitor{*یہ, netlistp};
}

ثابت std::سمتیہ<AstVar*>& V3UndrivenCapture::writeSummary(ثابت Ast_گرہ_FTask* taskp) {
    // Ensure entry exists even if empty
    (خالی)m_info[taskp];
    واپس computeWriteSummary(taskp);
}

ثابت std::سمتیہ<AstVar*>& V3UndrivenCapture::computeWriteSummary(ثابت Ast_گرہ_FTask* taskp) {
    FTaskInfo& info = m_info[taskp];

    اگر (info.state == State::مکمل) {
        UINFO(9, "undriven capture writeSummary cached size="
                     << info.writeSummary.سائز() << " for " << CaptureUtil::taskNameQ(taskp));
        واپس info.writeSummary;
    }
    اگر (info.state == State::VISITING) {
        UINFO(9, "undriven capture recursion detected at "
                     << CaptureUtil::taskNameQ(taskp)
                     << " returning directWrites size=" << info.براہ_راست_Writes.سائز());
        // Cycle detected. return directWrites only to guarantee termination.
        اگر (info.writeSummary.خالی_خانے()) info.writeSummary = info.براہ_راست_Writes;
        واپس info.writeSummary;
    }

    info.state = State::VISITING;

    info.writeSummary.clear();

    // Prevent duplicates across all sources that can contribute to a write summary (direct writes
    // and call chains)
    std::unordered_سیٹ<AstVar*> نظر_آیا;

    // Simple lambda for filtering duplicates
    خودکار جوڑو_Var = [&](AstVar* v) {
        اگر (نظر_آیا.داخل_کرو(v).دوسرا) info.writeSummary.ڈالو_عقب(v);
    };

    // Start with direct writes
    برائے (AstVar* v : info.براہ_راست_Writes) جوڑو_Var(v);

    // Add callee summaries
    برائے (ثابت Ast_گرہ_FTask* calleep : info.callees) {
        اگر (m_info.ڈھونڈو(calleep) == m_info.انتہا()) جاری;
        ثابت std::سمتیہ<AstVar*>& sub = computeWriteSummary(calleep);
        برائے (AstVar* v : sub) جوڑو_Var(v);
    }

    UINFO(9, "undriven capture writeSummary computed size=" << info.writeSummary.سائز() << " for "
                                                            << CaptureUtil::taskNameQ(taskp));

    // We are done, so set the m_info state correctly and return the vector of variables
    info.state = State::مکمل;
    واپس info.writeSummary;
}

خالی V3UndrivenCapture::noteTask(ثابت Ast_گرہ_FTask* taskp) { (خالی)m_info[taskp]; }

خالی V3UndrivenCapture::note_براہ_راست_Write(ثابت Ast_گرہ_FTask* taskp, AstVar* varp) {
    FTaskInfo& info = m_info[taskp];

    // Exclude function return variable (not an externally visible side-effect)
    AstVar* ثابت retVarp = VN_CAST(taskp->fvarp(), Var);
    اگر (retVarp && varp == retVarp) واپس;

    // Filter out duplicates.
    اگر (info.براہ_راست_Writes_سیٹ.داخل_کرو(varp).دوسرا) info.براہ_راست_Writes.ڈالو_عقب(varp);
}

خالی V3UndrivenCapture::note_Call_کنارہ(ثابت Ast_گرہ_FTask* callerp, ثابت Ast_گرہ_FTask* calleep) {
    FTaskInfo& callerInfo = m_info[callerp];
    // Prevents duplicate entries from being appended, if calleep already exists then insert will
    // return false, and then is not inserted into the callees vector.
    اگر (callerInfo.callees_سیٹ.داخل_کرو(calleep).دوسرا) { callerInfo.callees.ڈالو_عقب(calleep); }
    // Ensure callee entry exists, if already exists then this is a no-op.  unordered_map<> so
    // cheap.
    (خالی)m_info[calleep];
}
