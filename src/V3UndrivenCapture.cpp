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
    ساكن std::string taskNameQ(ثابت Ast_گرہ_FTask* taskp) {
        إذا (!taskp) أرجع "<null>";
        أرجع taskp->prettyNameQ();
    }
};

صنف CaptureVisitor final : عام VNVisitorConst {
    V3UndrivenCapture& m_cap;
    ثابت Ast_گرہ_FTask* m_curTaskp = خالی_سوچک;  // Current task
    منطقي m_وچ_Initial_تیاری = خطأ;  // In InitialAutomatic*/InitialStatic* assignment LHS

عام:
    واضح CaptureVisitor(V3UndrivenCapture& cap, AstNetlist* netlistp)
        : m_cap{cap} {
        iterateConst(netlistp);
    }

خاص:
    // Visit a task/function definition and collect direct writes and direct callees.
    فراغ ویکھو(Ast_گرہ_FTask* nodep) override {
        VL_RESTORER(m_curTaskp);
        m_curTaskp = nodep;
        UINFO(9, "undriven capture enter ftask " << nodep << " " << nodep->prettyNameQ());
        m_cap.noteTask(nodep);
        iterate_And_اگلا_Const_Null(nodep->stmtsp());
    }

    فراغ ویکھو(AstInitialAutomaticStmt* nodep) override {
        VL_RESTORER(m_وچ_Initial_تیاری);
        m_وچ_Initial_تیاری = سچ;
        iterateChildrenConst(nodep);
    }
    فراغ ویکھو(AstInitialStaticStmt* nodep) override {
        VL_RESTORER(m_وچ_Initial_تیاری);
        m_وچ_Initial_تیاری = سچ;
        iterateChildrenConst(nodep);
    }
    فراغ ویکھو(Ast_سونپو* nodep) override {
        {
            VL_RESTORER(m_وچ_Initial_تیاری);
            m_وچ_Initial_تیاری = خطأ;
            iterateConst(nodep->rhsp());
        }
        iterateConst(nodep->lhsp());
    }

    فراغ ویکھو(Ast_گرہ_Var_Ref* nodep) override {
        إذا (m_curTaskp && nodep->access().isWriteOrRW()
            && (nodep->varp()->hasUserInit() || !m_وچ_Initial_تیاری)) {
            UINFO(9, "undriven capture direct write in " << CaptureUtil::taskNameQ(m_curTaskp)
                                                         << " var=" << nodep->varp()->prettyNameQ()
                                                         << " at " << nodep->fileline());

            m_cap.note_سدھا_Write(m_curTaskp, nodep->varp());
        }
        iterateChildrenConst(nodep);
    }

    فراغ ویکھو(Ast_گرہ_FTask_Ref* nodep) override {
        // Record the call edge if resolved
        إذا (m_curTaskp) {
            إذا (ثابت Ast_گرہ_FTask* ثابت calleep = nodep->taskp()) {
                UINFO(9, "undriven capture call edge " << CaptureUtil::taskNameQ(
                             m_curTaskp) << " -> " << CaptureUtil::taskNameQ(calleep));
                m_cap.note_Call_کنارا(m_curTaskp, calleep);
            }
        }
        iterateChildrenConst(nodep);  // still scan pins/args
    }

    فراغ ویکھو(Ast_گرہ* nodep) override { iterateChildrenConst(nodep); }
};

}  // namespace

V3UndrivenCapture::V3UndrivenCapture(AstNetlist* netlistp) {
    gather(netlistp);

    // Compute summaries for all tasks
    لكل (ثابت خودکار& kv : m_info) (فراغ)computeWriteSummary(kv.پہلا);

    // Release the filter memory
    لكل (خودکار& kv : m_info) {
        kv.دوجا.callees_سیٹ.clear();
        kv.دوجا.callees_سیٹ.rehash(0);
        kv.دوجا.سدھا_Writes_سیٹ.clear();
        kv.دوجا.سدھا_Writes_سیٹ.rehash(0);
    }
}

فراغ V3UndrivenCapture::gather(AstNetlist* netlistp) {
    // Walk netlist and populate m_info with direct writes + call edges
    CaptureVisitor{*هذا, netlistp};
}

ثابت std::سمتیہ<AstVar*>& V3UndrivenCapture::writeSummary(ثابت Ast_گرہ_FTask* taskp) {
    // Ensure entry exists even if empty
    (فراغ)m_info[taskp];
    أرجع computeWriteSummary(taskp);
}

ثابت std::سمتیہ<AstVar*>& V3UndrivenCapture::computeWriteSummary(ثابت Ast_گرہ_FTask* taskp) {
    FTaskInfo& info = m_info[taskp];

    إذا (info.state == State::پورا) {
        UINFO(9, "undriven capture writeSummary cached size="
                     << info.writeSummary.ناپ() << " for " << CaptureUtil::taskNameQ(taskp));
        أرجع info.writeSummary;
    }
    إذا (info.state == State::VISITING) {
        UINFO(9, "undriven capture recursion detected at "
                     << CaptureUtil::taskNameQ(taskp)
                     << " returning directWrites size=" << info.سدھا_Writes.ناپ());
        // Cycle detected. return directWrites only to guarantee termination.
        إذا (info.writeSummary.خالی_تھاواں()) info.writeSummary = info.سدھا_Writes;
        أرجع info.writeSummary;
    }

    info.state = State::VISITING;

    info.writeSummary.clear();

    // Prevent duplicates across all sources that can contribute to a write summary (direct writes
    // and call chains)
    std::unordered_سیٹ<AstVar*> تکیا;

    // Simple lambda for filtering duplicates
    خودکار جوڑو_Var = [&](AstVar* v) {
        إذا (تکیا.داخل_کرو(v).دوجا) info.writeSummary.ڈالو_پٹھ(v);
    };

    // Start with direct writes
    لكل (AstVar* v : info.سدھا_Writes) جوڑو_Var(v);

    // Add callee summaries
    لكل (ثابت Ast_گرہ_FTask* calleep : info.callees) {
        إذا (m_info.لبھو(calleep) == m_info.سرا()) استمر;
        ثابت std::سمتیہ<AstVar*>& sub = computeWriteSummary(calleep);
        لكل (AstVar* v : sub) جوڑو_Var(v);
    }

    UINFO(9, "undriven capture writeSummary computed size=" << info.writeSummary.ناپ() << " for "
                                                            << CaptureUtil::taskNameQ(taskp));

    // We are done, so set the m_info state correctly and return the vector of variables
    info.state = State::پورا;
    أرجع info.writeSummary;
}

فراغ V3UndrivenCapture::noteTask(ثابت Ast_گرہ_FTask* taskp) { (فراغ)m_info[taskp]; }

فراغ V3UndrivenCapture::note_سدھا_Write(ثابت Ast_گرہ_FTask* taskp, AstVar* varp) {
    FTaskInfo& info = m_info[taskp];

    // Exclude function return variable (not an externally visible side-effect)
    AstVar* ثابت retVarp = VN_CAST(taskp->fvarp(), Var);
    إذا (retVarp && varp == retVarp) أرجع;

    // Filter out duplicates.
    إذا (info.سدھا_Writes_سیٹ.داخل_کرو(varp).دوجا) info.سدھا_Writes.ڈالو_پٹھ(varp);
}

فراغ V3UndrivenCapture::note_Call_کنارا(ثابت Ast_گرہ_FTask* callerp, ثابت Ast_گرہ_FTask* calleep) {
    FTaskInfo& callerInfo = m_info[callerp];
    // Prevents duplicate entries from being appended, if calleep already exists then insert will
    // return false, and then is not inserted into the callees vector.
    إذا (callerInfo.callees_سیٹ.داخل_کرو(calleep).دوجا) { callerInfo.callees.ڈالو_پٹھ(calleep); }
    // Ensure callee entry exists, if already exists then this is a no-op.  unordered_map<> so
    // cheap.
    (فراغ)m_info[calleep];
}
