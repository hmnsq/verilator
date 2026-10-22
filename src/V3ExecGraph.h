// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: AstExecGraph code construction
//
// Code available from: https://verilator.org
//
//*************************************************************************
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of either the GNU Lesser General Public License Version 3
// or the Perl Artistic License Version 2.0.
// SPDX-FileCopyrightText: 2003-2026 Wilson Snyder
// SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0
//
//*************************************************************************

#నిర్వచితం_కాకపోతే VERILATOR_V3EXECGRAPH_H_
#నిర్వచించు VERILATOR_V3EXECGRAPH_H_

#అన్తర్భావ "config_build.h"
#అన్తర్భావ "verilatedos.h"

#అన్తర్భావ "V3Graph.h"

#అన్తర్భావ <atomic>

class AstNetlist;
class AstCFunc;
class Ast_Exec_లేఖాచిత్రం;
class Ast_కణుపు_Stmt;
class AstScope;

//*************************************************************************
// MTasks and graph structures

class ExecMTask final : బహిరంగ V3_లేఖాచిత్రం_శీర్షం {
    VL_RTTI_IMPL(ExecMTask, V3_లేఖాచిత్రం_శీర్షం)
అంతరంగ:
    స్థిరరాశి uint32_t m_id;  // Unique ID of this ExecMTask.
    స్థిర std::atomic<uint32_t> s_తదుపరి_Id;  // Next ID to use
    AstCFunc* స్థిరరాశి m_funcp;  // The function that contains the task body
    స్థిరరాశి std::string m_hashName;  // Hashed name based on body for profile-driven optimization
    // Predicted critical path from the start of this mtask to the ends of the graph that are
    // reachable from this mtask. In abstract time units.
    uint32_t m_ప్రాధాన్యత = 0;
    // Predicted runtime of this mtask, in the same abstract time units as priority().
    uint32_t m_ఖర్చు = 0;
    uint64_t m_predict_ప్రారంభం = 0;  // Predicted start time of task
    పూర్ణాంకం m_దారాలు = 1;  // Threads used by this mtask
    VL_UNCOPYABLE(ExecMTask);

    స్థిర AstCFunc* createCFunc(Ast_Exec_లేఖాచిత్రం* execGraphp, AstScope* scopep, Ast_కణుపు_Stmt* stmtsp,
                                 uint32_t id);

బహిరంగ:
    ExecMTask(Ast_Exec_లేఖాచిత్రం* execGraphp, AstScope* scopep, Ast_కణుపు_Stmt* stmtsp) VL_MT_DISABLED;
    AstCFunc* funcp() స్థిరరాశి { తిరిగి m_funcp; }
    uint32_t id() స్థిరరాశి VL_MT_సురక్షిత { తిరిగి m_id; }
    uint32_t ప్రాధాన్యత_2() స్థిరరాశి { తిరిగి m_ప్రాధాన్యత; }
    శూన్య ప్రాధాన్యత_2(uint32_t pri) { m_ప్రాధాన్యత = pri; }
    uint32_t ఖర్చు() స్థిరరాశి { తిరిగి m_ఖర్చు; }
    శూన్య ఖర్చు(uint32_t ఖర్చు) { m_ఖర్చు = ఖర్చు; }
    uint64_t predict_ప్రారంభం() స్థిరరాశి { తిరిగి m_predict_ప్రారంభం; }
    శూన్య predict_ప్రారంభం(uint64_t సమయం_2) { m_predict_ప్రారంభం = సమయం_2; }
    string name() స్థిరరాశి override VL_MT_STABLE { తిరిగి "mt"s + std::గమ్యం_string(id()); }
    string hashName() స్థిరరాశి { తిరిగి m_hashName; }
    శూన్య దారాలు(పూర్ణాంకం దారాలు) { m_దారాలు = దారాలు; }
    పూర్ణాంకం దారాలు() స్థిరరాశి { తిరిగి m_దారాలు; }
    శూన్య dump(std::ostream& str) స్థిరరాశి;

    స్థిర uint32_t అంకె_వాడిన_Ids() VL_MT_సురక్షిత { తిరిగి s_తదుపరి_Id; }
};

అంతఃవరుస std::ostream& క్రియాకారకం<<(std::ostream& os, స్థిరరాశి ExecMTask& rhs) {
    rhs.dump(os);
    తిరిగి os;
}

namespace V3_Exec_లేఖాచిత్రం {
శూన్య implement(AstNetlist*) VL_MT_DISABLED;

శూన్య selfTest() VL_MT_DISABLED;
}  //namespace V3ExecGraph

#అయితే_ముగింపు  // Guard
