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

#ಅನ್ತರ್ಭಾವ "V3UndrivenCapture.h"

#ಅನ್ತರ್ಭಾವ "V3Error.h"
#ಅನ್ತರ್ಭಾವ "V3Global.h"

VL_DEFINE_DEBUG_FUNCTIONS;

ನಾಮಾಕಾಶ {

ವರ್ಗ CaptureUtil final {
ಸಾರ್ವಜನಿಕ:
    ಸ್ಥಿರ std::string taskNameQ(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_FTask* taskp) {
        ಆದರೆ (!taskp) ಹಿಂತಿರುಗಿ "<null>";
        ಹಿಂತಿರುಗಿ taskp->prettyNameQ();
    }
};

ವರ್ಗ CaptureVisitor final : ಸಾರ್ವಜನಿಕ VNVisitorConst {
    V3UndrivenCapture& m_cap;
    ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_FTask* m_curTaskp = ಶೂನ್ಯಸೂಚಕ;  // Current task
    ತಾರ್ಕಿಕ m_ಒಳ_Initial_ಸಿದ್ಧತೆ = ಸುಳ್ಳು;  // In InitialAutomatic*/InitialStatic* assignment LHS

ಸಾರ್ವಜನಿಕ:
    ಸ್ಪಷ್ಟ CaptureVisitor(V3UndrivenCapture& cap, AstNetlist* netlistp)
        : m_cap{cap} {
        iterateConst(netlistp);
    }

ಖಾಸಗಿ:
    // Visit a task/function definition and collect direct writes and direct callees.
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_FTask* nodep) override {
        VL_RESTORER(m_curTaskp);
        m_curTaskp = nodep;
        UINFO(9, "undriven capture enter ftask " << nodep << " " << nodep->prettyNameQ());
        m_cap.noteTask(nodep);
        iterate_And_ಮುಂದಿನ_Const_Null(nodep->stmtsp());
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstInitialAutomaticStmt* nodep) override {
        VL_RESTORER(m_ಒಳ_Initial_ಸಿದ್ಧತೆ);
        m_ಒಳ_Initial_ಸಿದ್ಧತೆ = ನಿಜ;
        iterateChildrenConst(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstInitialStaticStmt* nodep) override {
        VL_RESTORER(m_ಒಳ_Initial_ಸಿದ್ಧತೆ);
        m_ಒಳ_Initial_ಸಿದ್ಧತೆ = ನಿಜ;
        iterateChildrenConst(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ನಿಯೋಜಿಸು* nodep) override {
        {
            VL_RESTORER(m_ಒಳ_Initial_ಸಿದ್ಧತೆ);
            m_ಒಳ_Initial_ಸಿದ್ಧತೆ = ಸುಳ್ಳು;
            iterateConst(nodep->rhsp());
        }
        iterateConst(nodep->lhsp());
    }

    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Var_Ref* nodep) override {
        ಆದರೆ (m_curTaskp && nodep->access().isWriteOrRW()
            && (nodep->varp()->hasUserInit() || !m_ಒಳ_Initial_ಸಿದ್ಧತೆ)) {
            UINFO(9, "undriven capture direct write in " << CaptureUtil::taskNameQ(m_curTaskp)
                                                         << " var=" << nodep->varp()->prettyNameQ()
                                                         << " at " << nodep->fileline());

            m_cap.note_ನೇರ_Write(m_curTaskp, nodep->varp());
        }
        iterateChildrenConst(nodep);
    }

    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_FTask_Ref* nodep) override {
        // Record the call edge if resolved
        ಆದರೆ (m_curTaskp) {
            ಆದರೆ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_FTask* ಸ್ಥಿರಾಂಕ calleep = nodep->taskp()) {
                UINFO(9, "undriven capture call edge " << CaptureUtil::taskNameQ(
                             m_curTaskp) << " -> " << CaptureUtil::taskNameQ(calleep));
                m_cap.note_Call_ಅಂಚು(m_curTaskp, calleep);
            }
        }
        iterateChildrenConst(nodep);  // still scan pins/args
    }

    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override { iterateChildrenConst(nodep); }
};

}  // namespace

V3UndrivenCapture::V3UndrivenCapture(AstNetlist* netlistp) {
    gather(netlistp);

    // Compute summaries for all tasks
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& kv : m_info) (ಶೂನ್ಯ)computeWriteSummary(kv.ಮೊದಲ);

    // Release the filter memory
    ಗಾಗಿ (ಸ್ವಯಂಚಾಲಿತ& kv : m_info) {
        kv.ಎರಡನೇ.callees_ಗಣ.clear();
        kv.ಎರಡನೇ.callees_ಗಣ.rehash(0);
        kv.ಎರಡನೇ.ನೇರ_Writes_ಗಣ.clear();
        kv.ಎರಡನೇ.ನೇರ_Writes_ಗಣ.rehash(0);
    }
}

ಶೂನ್ಯ V3UndrivenCapture::gather(AstNetlist* netlistp) {
    // Walk netlist and populate m_info with direct writes + call edges
    CaptureVisitor{*ಇದು, netlistp};
}

ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<AstVar*>& V3UndrivenCapture::writeSummary(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_FTask* taskp) {
    // Ensure entry exists even if empty
    (ಶೂನ್ಯ)m_info[taskp];
    ಹಿಂತಿರುಗಿ computeWriteSummary(taskp);
}

ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<AstVar*>& V3UndrivenCapture::computeWriteSummary(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_FTask* taskp) {
    FTaskInfo& info = m_info[taskp];

    ಆದರೆ (info.state == State::ಮುಗಿದ) {
        UINFO(9, "undriven capture writeSummary cached size="
                     << info.writeSummary.ಗಾತ್ರ_2() << " for " << CaptureUtil::taskNameQ(taskp));
        ಹಿಂತಿರುಗಿ info.writeSummary;
    }
    ಆದರೆ (info.state == State::VISITING) {
        UINFO(9, "undriven capture recursion detected at "
                     << CaptureUtil::taskNameQ(taskp)
                     << " returning directWrites size=" << info.ನೇರ_Writes.ಗಾತ್ರ_2());
        // Cycle detected. return directWrites only to guarantee termination.
        ಆದರೆ (info.writeSummary.ಖಾಲಿಗಳು()) info.writeSummary = info.ನೇರ_Writes;
        ಹಿಂತಿರುಗಿ info.writeSummary;
    }

    info.state = State::VISITING;

    info.writeSummary.clear();

    // Prevent duplicates across all sources that can contribute to a write summary (direct writes
    // and call chains)
    std::unordered_ಗಣ<AstVar*> ನೋಡಿದ;

    // Simple lambda for filtering duplicates
    ಸ್ವಯಂಚಾಲಿತ ಕೂಡಿಸು_Var = [&](AstVar* v) {
        ಆದರೆ (ನೋಡಿದ.ಸೇರಿಸು(v).ಎರಡನೇ) info.writeSummary.ತಳ್ಳು_ಹಿಂಭಾಗ(v);
    };

    // Start with direct writes
    ಗಾಗಿ (AstVar* v : info.ನೇರ_Writes) ಕೂಡಿಸು_Var(v);

    // Add callee summaries
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_FTask* calleep : info.callees) {
        ಆದರೆ (m_info.ಕಂಡುಹಿಡಿ(calleep) == m_info.ಕೊನೆ()) ಮುಂದುವರಿ;
        ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<AstVar*>& sub = computeWriteSummary(calleep);
        ಗಾಗಿ (AstVar* v : sub) ಕೂಡಿಸು_Var(v);
    }

    UINFO(9, "undriven capture writeSummary computed size=" << info.writeSummary.ಗಾತ್ರ_2() << " for "
                                                            << CaptureUtil::taskNameQ(taskp));

    // We are done, so set the m_info state correctly and return the vector of variables
    info.state = State::ಮುಗಿದ;
    ಹಿಂತಿರುಗಿ info.writeSummary;
}

ಶೂನ್ಯ V3UndrivenCapture::noteTask(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_FTask* taskp) { (ಶೂನ್ಯ)m_info[taskp]; }

ಶೂನ್ಯ V3UndrivenCapture::note_ನೇರ_Write(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_FTask* taskp, AstVar* varp) {
    FTaskInfo& info = m_info[taskp];

    // Exclude function return variable (not an externally visible side-effect)
    AstVar* ಸ್ಥಿರಾಂಕ retVarp = VN_CAST(taskp->fvarp(), Var);
    ಆದರೆ (retVarp && varp == retVarp) ಹಿಂತಿರುಗಿ;

    // Filter out duplicates.
    ಆದರೆ (info.ನೇರ_Writes_ಗಣ.ಸೇರಿಸು(varp).ಎರಡನೇ) info.ನೇರ_Writes.ತಳ್ಳು_ಹಿಂಭಾಗ(varp);
}

ಶೂನ್ಯ V3UndrivenCapture::note_Call_ಅಂಚು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_FTask* callerp, ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_FTask* calleep) {
    FTaskInfo& callerInfo = m_info[callerp];
    // Prevents duplicate entries from being appended, if calleep already exists then insert will
    // return false, and then is not inserted into the callees vector.
    ಆದರೆ (callerInfo.callees_ಗಣ.ಸೇರಿಸು(calleep).ಎರಡನೇ) { callerInfo.callees.ತಳ್ಳು_ಹಿಂಭಾಗ(calleep); }
    // Ensure callee entry exists, if already exists then this is a no-op.  unordered_map<> so
    // cheap.
    (ಶೂನ್ಯ)m_info[calleep];
}
