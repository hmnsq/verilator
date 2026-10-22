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
    ساكن std::string taskNameQ(ثابت Ast_گره_FTask* taskp) {
        إذا (!taskp) أرجع "<null>";
        أرجع taskp->prettyNameQ();
    }
};

صنف CaptureVisitor final : عام VNVisitorConst {
    V3UndrivenCapture& m_cap;
    ثابت Ast_گره_FTask* m_curTaskp = اشاره_تهی;  // Current task
    منطقي m_درون_Initial_راهاندازی = خطأ;  // In InitialAutomatic*/InitialStatic* assignment LHS

عام:
    صریح CaptureVisitor(V3UndrivenCapture& cap, AstNetlist* netlistp)
        : m_cap{cap} {
        iterateConst(netlistp);
    }

خاص:
    // Visit a task/function definition and collect direct writes and direct callees.
    فراغ بازدید(Ast_گره_FTask* nodep) override {
        VL_RESTORER(m_curTaskp);
        m_curTaskp = nodep;
        UINFO(9, "undriven capture enter ftask " << nodep << " " << nodep->prettyNameQ());
        m_cap.noteTask(nodep);
        iterate_And_بعدی_Const_Null(nodep->stmtsp());
    }

    فراغ بازدید(AstInitialAutomaticStmt* nodep) override {
        VL_RESTORER(m_درون_Initial_راهاندازی);
        m_درون_Initial_راهاندازی = درست;
        iterateChildrenConst(nodep);
    }
    فراغ بازدید(AstInitialStaticStmt* nodep) override {
        VL_RESTORER(m_درون_Initial_راهاندازی);
        m_درون_Initial_راهاندازی = درست;
        iterateChildrenConst(nodep);
    }
    فراغ بازدید(Ast_تخصیص_بده* nodep) override {
        {
            VL_RESTORER(m_درون_Initial_راهاندازی);
            m_درون_Initial_راهاندازی = خطأ;
            iterateConst(nodep->rhsp());
        }
        iterateConst(nodep->lhsp());
    }

    فراغ بازدید(Ast_گره_Var_Ref* nodep) override {
        إذا (m_curTaskp && nodep->access().isWriteOrRW()
            && (nodep->varp()->hasUserInit() || !m_درون_Initial_راهاندازی)) {
            UINFO(9, "undriven capture direct write in " << CaptureUtil::taskNameQ(m_curTaskp)
                                                         << " var=" << nodep->varp()->prettyNameQ()
                                                         << " at " << nodep->fileline());

            m_cap.note_مستقیم_Write(m_curTaskp, nodep->varp());
        }
        iterateChildrenConst(nodep);
    }

    فراغ بازدید(Ast_گره_FTask_Ref* nodep) override {
        // Record the call edge if resolved
        إذا (m_curTaskp) {
            إذا (ثابت Ast_گره_FTask* ثابت calleep = nodep->taskp()) {
                UINFO(9, "undriven capture call edge " << CaptureUtil::taskNameQ(
                             m_curTaskp) << " -> " << CaptureUtil::taskNameQ(calleep));
                m_cap.note_Call_یال(m_curTaskp, calleep);
            }
        }
        iterateChildrenConst(nodep);  // still scan pins/args
    }

    فراغ بازدید(Ast_گره* nodep) override { iterateChildrenConst(nodep); }
};

}  // namespace

V3UndrivenCapture::V3UndrivenCapture(AstNetlist* netlistp) {
    gather(netlistp);

    // Compute summaries for all tasks
    لكل (ثابت خودکار& kv : m_info) (فراغ)computeWriteSummary(kv.اولی);

    // Release the filter memory
    لكل (خودکار& kv : m_info) {
        kv.دومی.callees_مجموعه.clear();
        kv.دومی.callees_مجموعه.rehash(0);
        kv.دومی.مستقیم_Writes_مجموعه.clear();
        kv.دومی.مستقیم_Writes_مجموعه.rehash(0);
    }
}

فراغ V3UndrivenCapture::gather(AstNetlist* netlistp) {
    // Walk netlist and populate m_info with direct writes + call edges
    CaptureVisitor{*هذا, netlistp};
}

ثابت std::وکتور<AstVar*>& V3UndrivenCapture::writeSummary(ثابت Ast_گره_FTask* taskp) {
    // Ensure entry exists even if empty
    (فراغ)m_info[taskp];
    أرجع computeWriteSummary(taskp);
}

ثابت std::وکتور<AstVar*>& V3UndrivenCapture::computeWriteSummary(ثابت Ast_گره_FTask* taskp) {
    FTaskInfo& info = m_info[taskp];

    إذا (info.state == State::انجامشده) {
        UINFO(9, "undriven capture writeSummary cached size="
                     << info.writeSummary.اندازه() << " for " << CaptureUtil::taskNameQ(taskp));
        أرجع info.writeSummary;
    }
    إذا (info.state == State::VISITING) {
        UINFO(9, "undriven capture recursion detected at "
                     << CaptureUtil::taskNameQ(taskp)
                     << " returning directWrites size=" << info.مستقیم_Writes.اندازه());
        // Cycle detected. return directWrites only to guarantee termination.
        إذا (info.writeSummary.خالی()) info.writeSummary = info.مستقیم_Writes;
        أرجع info.writeSummary;
    }

    info.state = State::VISITING;

    info.writeSummary.clear();

    // Prevent duplicates across all sources that can contribute to a write summary (direct writes
    // and call chains)
    std::unordered_مجموعه<AstVar*> دیده;

    // Simple lambda for filtering duplicates
    خودکار جمع_کن_Var = [&](AstVar* v) {
        إذا (دیده.درج(v).دومی) info.writeSummary.افزودن_پشت(v);
    };

    // Start with direct writes
    لكل (AstVar* v : info.مستقیم_Writes) جمع_کن_Var(v);

    // Add callee summaries
    لكل (ثابت Ast_گره_FTask* calleep : info.callees) {
        إذا (m_info.پیدا_کن(calleep) == m_info.انتها()) استمر;
        ثابت std::وکتور<AstVar*>& sub = computeWriteSummary(calleep);
        لكل (AstVar* v : sub) جمع_کن_Var(v);
    }

    UINFO(9, "undriven capture writeSummary computed size=" << info.writeSummary.اندازه() << " for "
                                                            << CaptureUtil::taskNameQ(taskp));

    // We are done, so set the m_info state correctly and return the vector of variables
    info.state = State::انجامشده;
    أرجع info.writeSummary;
}

فراغ V3UndrivenCapture::noteTask(ثابت Ast_گره_FTask* taskp) { (فراغ)m_info[taskp]; }

فراغ V3UndrivenCapture::note_مستقیم_Write(ثابت Ast_گره_FTask* taskp, AstVar* varp) {
    FTaskInfo& info = m_info[taskp];

    // Exclude function return variable (not an externally visible side-effect)
    AstVar* ثابت retVarp = VN_CAST(taskp->fvarp(), Var);
    إذا (retVarp && varp == retVarp) أرجع;

    // Filter out duplicates.
    إذا (info.مستقیم_Writes_مجموعه.درج(varp).دومی) info.مستقیم_Writes.افزودن_پشت(varp);
}

فراغ V3UndrivenCapture::note_Call_یال(ثابت Ast_گره_FTask* callerp, ثابت Ast_گره_FTask* calleep) {
    FTaskInfo& callerInfo = m_info[callerp];
    // Prevents duplicate entries from being appended, if calleep already exists then insert will
    // return false, and then is not inserted into the callees vector.
    إذا (callerInfo.callees_مجموعه.درج(calleep).دومی) { callerInfo.callees.افزودن_پشت(calleep); }
    // Ensure callee entry exists, if already exists then this is a no-op.  unordered_map<> so
    // cheap.
    (فراغ)m_info[calleep];
}
