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

#ifndef VERILATOR_V3EXECGRAPH_H_
#define VERILATOR_V3EXECGRAPH_H_

#અન્તર્ભાવ "config_build.h"
#અન્તર્ભાવ "verilatedos.h"

#અન્તર્ભાવ "V3Graph.h"

#અન્તર્ભાવ <atomic>

class AstNetlist;
class AstCFunc;
class AstExecGraph;
class Ast_ગાંઠ_Stmt;
class AstScope;

//*************************************************************************
// MTasks and graph structures

class ExecMTask final : જાહેર V3GraphVertex {
    VL_RTTI_IMPL(ExecMTask, V3GraphVertex)
ખાનગી:
    અચલ uint32_t m_id;  // Unique ID of this ExecMTask.
    સ્થિર std::atomic<uint32_t> s_nextId;  // Next ID to use
    AstCFunc* અચલ m_funcp;  // The function that contains the task body
    અચલ std::string m_hashName;  // Hashed name based on body for profile-driven optimization
    // Predicted critical path from the start of this mtask to the ends of the graph that are
    // reachable from this mtask. In abstract time units.
    uint32_t m_priority = 0;
    // Predicted runtime of this mtask, in the same abstract time units as priority().
    uint32_t m_cost = 0;
    uint64_t m_predictStart = 0;  // Predicted start time of task
    પૂર્ણાંક m_threads = 1;  // Threads used by this mtask
    VL_UNCOPYABLE(ExecMTask);

    સ્થિર AstCFunc* createCFunc(AstExecGraph* execGraphp, AstScope* scopep, Ast_ગાંઠ_Stmt* stmtsp,
                                 uint32_t id);

જાહેર:
    ExecMTask(AstExecGraph* execGraphp, AstScope* scopep, Ast_ગાંઠ_Stmt* stmtsp) VL_MT_DISABLED;
    AstCFunc* funcp() અચલ { પાછા m_funcp; }
    uint32_t id() અચલ VL_MT_SAFE { પાછા m_id; }
    uint32_t priority() અચલ { પાછા m_priority; }
    શૂન્ય priority(uint32_t pri) { m_priority = pri; }
    uint32_t cost() અચલ { પાછા m_cost; }
    શૂન્ય cost(uint32_t cost) { m_cost = cost; }
    uint64_t predictStart() અચલ { પાછા m_predictStart; }
    શૂન્ય predictStart(uint64_t time) { m_predictStart = time; }
    string name() અચલ override VL_MT_STABLE { પાછા "mt"s + std::to_string(id()); }
    string hashName() અચલ { પાછા m_hashName; }
    શૂન્ય threads(પૂર્ણાંક threads) { m_threads = threads; }
    પૂર્ણાંક threads() અચલ { પાછા m_threads; }
    શૂન્ય dump(std::ostream& str) અચલ;

    સ્થિર uint32_t અંક_Used_Ids() VL_MT_SAFE { પાછા s_nextId; }
};

અંતઃરેખ std::ostream& operator<<(std::ostream& os, અચલ ExecMTask& rhs) {
    rhs.dump(os);
    પાછા os;
}

namespace V3ExecGraph {
શૂન્ય implement(AstNetlist*) VL_MT_DISABLED;

શૂન્ય selfTest() VL_MT_DISABLED;
}  //namespace V3ExecGraph

#endif  // Guard
