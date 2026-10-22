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

#अन्तर्भाव "V3UndrivenCapture.h"

#अन्तर्भाव "V3Error.h"
#अन्तर्भाव "V3Global.h"

VL_DEFINE_DEBUG_FUNCTIONS;

नामाकाश {

वर्ग CaptureUtil final {
सार्वजनिक:
    स्थिर std::string taskNameQ(ध्रुव Ast_ग्रन्थिः_FTask* taskp) {
        यदि (!taskp) प्रत्यागम "<null>";
        प्रत्यागम taskp->prettyNameQ();
    }
};

वर्ग CaptureVisitor final : सार्वजनिक VNVisitorConst {
    V3UndrivenCapture& m_cap;
    ध्रुव Ast_ग्रन्थिः_FTask* m_curTaskp = शून्यसूचकः;  // Current task
    तार्किक m_अन्तर्_Initial_सज्जता = असत्य;  // In InitialAutomatic*/InitialStatic* assignment LHS

सार्वजनिक:
    स्पष्ट CaptureVisitor(V3UndrivenCapture& cap, AstNetlist* netlistp)
        : m_cap{cap} {
        iterateConst(netlistp);
    }

गुप्त:
    // Visit a task/function definition and collect direct writes and direct callees.
    शून्य दर्शय(Ast_ग्रन्थिः_FTask* nodep) override {
        VL_RESTORER(m_curTaskp);
        m_curTaskp = nodep;
        UINFO(9, "undriven capture enter ftask " << nodep << " " << nodep->prettyNameQ());
        m_cap.noteTask(nodep);
        iterate_And_अग्र_Const_Null(nodep->stmtsp());
    }

    शून्य दर्शय(AstInitialAutomaticStmt* nodep) override {
        VL_RESTORER(m_अन्तर्_Initial_सज्जता);
        m_अन्तर्_Initial_सज्जता = सत्य;
        iterateChildrenConst(nodep);
    }
    शून्य दर्शय(AstInitialStaticStmt* nodep) override {
        VL_RESTORER(m_अन्तर्_Initial_सज्जता);
        m_अन्तर्_Initial_सज्जता = सत्य;
        iterateChildrenConst(nodep);
    }
    शून्य दर्शय(Ast_नियोजय* nodep) override {
        {
            VL_RESTORER(m_अन्तर्_Initial_सज्जता);
            m_अन्तर्_Initial_सज्जता = असत्य;
            iterateConst(nodep->rhsp());
        }
        iterateConst(nodep->lhsp());
    }

    शून्य दर्शय(Ast_ग्रन्थिः_Var_Ref* nodep) override {
        यदि (m_curTaskp && nodep->access().isWriteOrRW()
            && (nodep->varp()->hasUserInit() || !m_अन्तर्_Initial_सज्जता)) {
            UINFO(9, "undriven capture direct write in " << CaptureUtil::taskNameQ(m_curTaskp)
                                                         << " var=" << nodep->varp()->prettyNameQ()
                                                         << " at " << nodep->fileline());

            m_cap.note_प्रत्यक्षम्_Write(m_curTaskp, nodep->varp());
        }
        iterateChildrenConst(nodep);
    }

    शून्य दर्शय(Ast_ग्रन्थिः_FTask_Ref* nodep) override {
        // Record the call edge if resolved
        यदि (m_curTaskp) {
            यदि (ध्रुव Ast_ग्रन्थिः_FTask* ध्रुव calleep = nodep->taskp()) {
                UINFO(9, "undriven capture call edge " << CaptureUtil::taskNameQ(
                             m_curTaskp) << " -> " << CaptureUtil::taskNameQ(calleep));
                m_cap.noteCallEdge(m_curTaskp, calleep);
            }
        }
        iterateChildrenConst(nodep);  // still scan pins/args
    }

    शून्य दर्शय(Ast_ग्रन्थिः* nodep) override { iterateChildrenConst(nodep); }
};

}  // namespace

V3UndrivenCapture::V3UndrivenCapture(AstNetlist* netlistp) {
    gather(netlistp);

    // Compute summaries for all tasks
    प्रति (ध्रुव स्वचालित& kv : m_info) (शून्य)computeWriteSummary(kv.first);

    // Release the filter memory
    प्रति (स्वचालित& kv : m_info) {
        kv.second.callees_समुच्चयः.clear();
        kv.second.callees_समुच्चयः.rehash(0);
        kv.second.प्रत्यक्षम्_Writes_समुच्चयः.clear();
        kv.second.प्रत्यक्षम्_Writes_समुच्चयः.rehash(0);
    }
}

शून्य V3UndrivenCapture::gather(AstNetlist* netlistp) {
    // Walk netlist and populate m_info with direct writes + call edges
    CaptureVisitor{*एतत्, netlistp};
}

ध्रुव std::सदिशम्<AstVar*>& V3UndrivenCapture::writeSummary(ध्रुव Ast_ग्रन्थिः_FTask* taskp) {
    // Ensure entry exists even if empty
    (शून्य)m_info[taskp];
    प्रत्यागम computeWriteSummary(taskp);
}

ध्रुव std::सदिशम्<AstVar*>& V3UndrivenCapture::computeWriteSummary(ध्रुव Ast_ग्रन्थिः_FTask* taskp) {
    FTaskInfo& info = m_info[taskp];

    यदि (info.state == State::समाप्तम्) {
        UINFO(9, "undriven capture writeSummary cached size="
                     << info.writeSummary.आकार() << " for " << CaptureUtil::taskNameQ(taskp));
        प्रत्यागम info.writeSummary;
    }
    यदि (info.state == State::VISITING) {
        UINFO(9, "undriven capture recursion detected at "
                     << CaptureUtil::taskNameQ(taskp)
                     << " returning directWrites size=" << info.प्रत्यक्षम्_Writes.आकार());
        // Cycle detected. return directWrites only to guarantee termination.
        यदि (info.writeSummary.रिक्तानि()) info.writeSummary = info.प्रत्यक्षम्_Writes;
        प्रत्यागम info.writeSummary;
    }

    info.state = State::VISITING;

    info.writeSummary.clear();

    // Prevent duplicates across all sources that can contribute to a write summary (direct writes
    // and call chains)
    std::unordered_समुच्चयः<AstVar*> आलोकितम्;

    // Simple lambda for filtering duplicates
    स्वचालित सङ्कलय_Var = [&](AstVar* v) {
        यदि (आलोकितम्.निवेशय(v).second) info.writeSummary.प्रक्षिप_पृष्ठभागः(v);
    };

    // Start with direct writes
    प्रति (AstVar* v : info.प्रत्यक्षम्_Writes) सङ्कलय_Var(v);

    // Add callee summaries
    प्रति (ध्रुव Ast_ग्रन्थिः_FTask* calleep : info.callees) {
        यदि (m_info.find(calleep) == m_info.अन्तः_2()) अनुवर्त;
        ध्रुव std::सदिशम्<AstVar*>& sub = computeWriteSummary(calleep);
        प्रति (AstVar* v : sub) सङ्कलय_Var(v);
    }

    UINFO(9, "undriven capture writeSummary computed size=" << info.writeSummary.आकार() << " for "
                                                            << CaptureUtil::taskNameQ(taskp));

    // We are done, so set the m_info state correctly and return the vector of variables
    info.state = State::समाप्तम्;
    प्रत्यागम info.writeSummary;
}

शून्य V3UndrivenCapture::noteTask(ध्रुव Ast_ग्रन्थिः_FTask* taskp) { (शून्य)m_info[taskp]; }

शून्य V3UndrivenCapture::note_प्रत्यक्षम्_Write(ध्रुव Ast_ग्रन्थिः_FTask* taskp, AstVar* varp) {
    FTaskInfo& info = m_info[taskp];

    // Exclude function return variable (not an externally visible side-effect)
    AstVar* ध्रुव retVarp = VN_CAST(taskp->fvarp(), Var);
    यदि (retVarp && varp == retVarp) प्रत्यागम;

    // Filter out duplicates.
    यदि (info.प्रत्यक्षम्_Writes_समुच्चयः.निवेशय(varp).second) info.प्रत्यक्षम्_Writes.प्रक्षिप_पृष्ठभागः(varp);
}

शून्य V3UndrivenCapture::noteCallEdge(ध्रुव Ast_ग्रन्थिः_FTask* callerp, ध्रुव Ast_ग्रन्थिः_FTask* calleep) {
    FTaskInfo& callerInfo = m_info[callerp];
    // Prevents duplicate entries from being appended, if calleep already exists then insert will
    // return false, and then is not inserted into the callees vector.
    यदि (callerInfo.callees_समुच्चयः.निवेशय(calleep).second) { callerInfo.callees.प्रक्षिप_पृष्ठभागः(calleep); }
    // Ensure callee entry exists, if already exists then this is a no-op.  unordered_map<> so
    // cheap.
    (शून्य)m_info[calleep];
}
