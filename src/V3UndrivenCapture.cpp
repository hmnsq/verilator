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

#ਅਨ੍ਤਰ੍ਭਾਵ "V3UndrivenCapture.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Error.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Global.h"

VL_DEFINE_DEBUG_FUNCTIONS;

ਨਾਮਾਕਾਸ਼ {

ਵਰਗ CaptureUtil final {
ਜਨਤਕ:
    ਸਥਿਰ std::string taskNameQ(ਅਚਲ Ast_ਗੰਢ_FTask* taskp) {
        ਜੇ (!taskp) ਵਾਪਸ "<null>";
        ਵਾਪਸ taskp->prettyNameQ();
    }
};

ਵਰਗ CaptureVisitor final : ਜਨਤਕ VNVisitorConst {
    V3UndrivenCapture& m_cap;
    ਅਚਲ Ast_ਗੰਢ_FTask* m_curTaskp = ਖਾਲੀ_ਸੂਚਕ;  // Current task
    ਤਾਰਕਿਕ m_ਵਿੱਚ_Initial_ਤਿਆਰੀ = ਝੂਠ;  // In InitialAutomatic*/InitialStatic* assignment LHS

ਜਨਤਕ:
    ਸਪਸ਼ਟ CaptureVisitor(V3UndrivenCapture& cap, AstNetlist* netlistp)
        : m_cap{cap} {
        iterateConst(netlistp);
    }

ਨਿਜੀ:
    // Visit a task/function definition and collect direct writes and direct callees.
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_FTask* nodep) override {
        VL_RESTORER(m_curTaskp);
        m_curTaskp = nodep;
        UINFO(9, "undriven capture enter ftask " << nodep << " " << nodep->prettyNameQ());
        m_cap.noteTask(nodep);
        iterate_And_ਅਗਲਾ_Const_Null(nodep->stmtsp());
    }

    ਖਾਲੀ ਦੇਖੋ(AstInitialAutomaticStmt* nodep) override {
        VL_RESTORER(m_ਵਿੱਚ_Initial_ਤਿਆਰੀ);
        m_ਵਿੱਚ_Initial_ਤਿਆਰੀ = ਸੱਚ;
        iterateChildrenConst(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstInitialStaticStmt* nodep) override {
        VL_RESTORER(m_ਵਿੱਚ_Initial_ਤਿਆਰੀ);
        m_ਵਿੱਚ_Initial_ਤਿਆਰੀ = ਸੱਚ;
        iterateChildrenConst(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਸੌਂਪੋ* nodep) override {
        {
            VL_RESTORER(m_ਵਿੱਚ_Initial_ਤਿਆਰੀ);
            m_ਵਿੱਚ_Initial_ਤਿਆਰੀ = ਝੂਠ;
            iterateConst(nodep->rhsp());
        }
        iterateConst(nodep->lhsp());
    }

    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Var_Ref* nodep) override {
        ਜੇ (m_curTaskp && nodep->access().isWriteOrRW()
            && (nodep->varp()->hasUserInit() || !m_ਵਿੱਚ_Initial_ਤਿਆਰੀ)) {
            UINFO(9, "undriven capture direct write in " << CaptureUtil::taskNameQ(m_curTaskp)
                                                         << " var=" << nodep->varp()->prettyNameQ()
                                                         << " at " << nodep->fileline());

            m_cap.note_ਸਿੱਧਾ_Write(m_curTaskp, nodep->varp());
        }
        iterateChildrenConst(nodep);
    }

    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_FTask_Ref* nodep) override {
        // Record the call edge if resolved
        ਜੇ (m_curTaskp) {
            ਜੇ (ਅਚਲ Ast_ਗੰਢ_FTask* ਅਚਲ calleep = nodep->taskp()) {
                UINFO(9, "undriven capture call edge " << CaptureUtil::taskNameQ(
                             m_curTaskp) << " -> " << CaptureUtil::taskNameQ(calleep));
                m_cap.note_Call_ਕਿਨਾਰਾ(m_curTaskp, calleep);
            }
        }
        iterateChildrenConst(nodep);  // still scan pins/args
    }

    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override { iterateChildrenConst(nodep); }
};

}  // namespace

V3UndrivenCapture::V3UndrivenCapture(AstNetlist* netlistp) {
    gather(netlistp);

    // Compute summaries for all tasks
    ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& kv : m_info) (ਖਾਲੀ)computeWriteSummary(kv.ਪਹਿਲਾ);

    // Release the filter memory
    ਲਈ (ਸਵੈਚਾਲਿਤ& kv : m_info) {
        kv.ਦੂਜਾ.callees_ਸੈੱਟ.clear();
        kv.ਦੂਜਾ.callees_ਸੈੱਟ.rehash(0);
        kv.ਦੂਜਾ.ਸਿੱਧਾ_Writes_ਸੈੱਟ.clear();
        kv.ਦੂਜਾ.ਸਿੱਧਾ_Writes_ਸੈੱਟ.rehash(0);
    }
}

ਖਾਲੀ V3UndrivenCapture::gather(AstNetlist* netlistp) {
    // Walk netlist and populate m_info with direct writes + call edges
    CaptureVisitor{*ਇਹ, netlistp};
}

ਅਚਲ std::ਸਦਿਸ਼<AstVar*>& V3UndrivenCapture::writeSummary(ਅਚਲ Ast_ਗੰਢ_FTask* taskp) {
    // Ensure entry exists even if empty
    (ਖਾਲੀ)m_info[taskp];
    ਵਾਪਸ computeWriteSummary(taskp);
}

ਅਚਲ std::ਸਦਿਸ਼<AstVar*>& V3UndrivenCapture::computeWriteSummary(ਅਚਲ Ast_ਗੰਢ_FTask* taskp) {
    FTaskInfo& info = m_info[taskp];

    ਜੇ (info.state == State::ਪੂਰਾ) {
        UINFO(9, "undriven capture writeSummary cached size="
                     << info.writeSummary.ਮਾਪ() << " for " << CaptureUtil::taskNameQ(taskp));
        ਵਾਪਸ info.writeSummary;
    }
    ਜੇ (info.state == State::VISITING) {
        UINFO(9, "undriven capture recursion detected at "
                     << CaptureUtil::taskNameQ(taskp)
                     << " returning directWrites size=" << info.ਸਿੱਧਾ_Writes.ਮਾਪ());
        // Cycle detected. return directWrites only to guarantee termination.
        ਜੇ (info.writeSummary.ਖਾਲੀ_ਥਾਵਾਂ()) info.writeSummary = info.ਸਿੱਧਾ_Writes;
        ਵਾਪਸ info.writeSummary;
    }

    info.state = State::VISITING;

    info.writeSummary.clear();

    // Prevent duplicates across all sources that can contribute to a write summary (direct writes
    // and call chains)
    std::unordered_ਸੈੱਟ<AstVar*> ਵੇਖਿਆ;

    // Simple lambda for filtering duplicates
    ਸਵੈਚਾਲਿਤ ਜੋੜੋ_Var = [&](AstVar* v) {
        ਜੇ (ਵੇਖਿਆ.ਪਾਓ(v).ਦੂਜਾ) info.writeSummary.ਧੱਕੋ_ਪਿੱਠ(v);
    };

    // Start with direct writes
    ਲਈ (AstVar* v : info.ਸਿੱਧਾ_Writes) ਜੋੜੋ_Var(v);

    // Add callee summaries
    ਲਈ (ਅਚਲ Ast_ਗੰਢ_FTask* calleep : info.callees) {
        ਜੇ (m_info.ਲੱਭੋ(calleep) == m_info.ਸਿਰਾ()) ਜਾਰੀ;
        ਅਚਲ std::ਸਦਿਸ਼<AstVar*>& sub = computeWriteSummary(calleep);
        ਲਈ (AstVar* v : sub) ਜੋੜੋ_Var(v);
    }

    UINFO(9, "undriven capture writeSummary computed size=" << info.writeSummary.ਮਾਪ() << " for "
                                                            << CaptureUtil::taskNameQ(taskp));

    // We are done, so set the m_info state correctly and return the vector of variables
    info.state = State::ਪੂਰਾ;
    ਵਾਪਸ info.writeSummary;
}

ਖਾਲੀ V3UndrivenCapture::noteTask(ਅਚਲ Ast_ਗੰਢ_FTask* taskp) { (ਖਾਲੀ)m_info[taskp]; }

ਖਾਲੀ V3UndrivenCapture::note_ਸਿੱਧਾ_Write(ਅਚਲ Ast_ਗੰਢ_FTask* taskp, AstVar* varp) {
    FTaskInfo& info = m_info[taskp];

    // Exclude function return variable (not an externally visible side-effect)
    AstVar* ਅਚਲ retVarp = VN_CAST(taskp->fvarp(), Var);
    ਜੇ (retVarp && varp == retVarp) ਵਾਪਸ;

    // Filter out duplicates.
    ਜੇ (info.ਸਿੱਧਾ_Writes_ਸੈੱਟ.ਪਾਓ(varp).ਦੂਜਾ) info.ਸਿੱਧਾ_Writes.ਧੱਕੋ_ਪਿੱਠ(varp);
}

ਖਾਲੀ V3UndrivenCapture::note_Call_ਕਿਨਾਰਾ(ਅਚਲ Ast_ਗੰਢ_FTask* callerp, ਅਚਲ Ast_ਗੰਢ_FTask* calleep) {
    FTaskInfo& callerInfo = m_info[callerp];
    // Prevents duplicate entries from being appended, if calleep already exists then insert will
    // return false, and then is not inserted into the callees vector.
    ਜੇ (callerInfo.callees_ਸੈੱਟ.ਪਾਓ(calleep).ਦੂਜਾ) { callerInfo.callees.ਧੱਕੋ_ਪਿੱਠ(calleep); }
    // Ensure callee entry exists, if already exists then this is a no-op.  unordered_map<> so
    // cheap.
    (ਖਾਲੀ)m_info[calleep];
}
