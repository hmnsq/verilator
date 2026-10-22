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

#അന്തര്ഭാവ "config_build.h"
#അന്തര്ഭാവ "verilatedos.h"

#അന്തര്ഭാവ "V3Graph.h"

#അന്തര്ഭാവ <atomic>

class AstNetlist;
class AstCFunc;
class AstExecGraph;
class Ast_കണ്ണി_Stmt;
class AstScope;

//*************************************************************************
// MTasks and graph structures

class ExecMTask final : പൊതു V3GraphVertex {
    VL_RTTI_IMPL(ExecMTask, V3GraphVertex)
സ്വകാര്യ:
    സ്ഥിരം uint32_t m_id;  // Unique ID of this ExecMTask.
    സ്ഥിതം std::atomic<uint32_t> s_nextId;  // Next ID to use
    AstCFunc* സ്ഥിരം m_funcp;  // The function that contains the task body
    സ്ഥിരം std::string m_hashName;  // Hashed name based on body for profile-driven optimization
    // Predicted critical path from the start of this mtask to the ends of the graph that are
    // reachable from this mtask. In abstract time units.
    uint32_t m_priority = 0;
    // Predicted runtime of this mtask, in the same abstract time units as priority().
    uint32_t m_cost = 0;
    uint64_t m_predictStart = 0;  // Predicted start time of task
    പൂർണ്ണസംഖ്യ m_threads = 1;  // Threads used by this mtask
    VL_UNCOPYABLE(ExecMTask);

    സ്ഥിതം AstCFunc* createCFunc(AstExecGraph* execGraphp, AstScope* scopep, Ast_കണ്ണി_Stmt* stmtsp,
                                 uint32_t id);

പൊതു:
    ExecMTask(AstExecGraph* execGraphp, AstScope* scopep, Ast_കണ്ണി_Stmt* stmtsp) VL_MT_DISABLED;
    AstCFunc* funcp() സ്ഥിരം { തിരികെ m_funcp; }
    uint32_t id() സ്ഥിരം VL_MT_SAFE { തിരികെ m_id; }
    uint32_t priority() സ്ഥിരം { തിരികെ m_priority; }
    ശൂന്യം priority(uint32_t pri) { m_priority = pri; }
    uint32_t cost() സ്ഥിരം { തിരികെ m_cost; }
    ശൂന്യം cost(uint32_t cost) { m_cost = cost; }
    uint64_t predictStart() സ്ഥിരം { തിരികെ m_predictStart; }
    ശൂന്യം predictStart(uint64_t time) { m_predictStart = time; }
    string name() സ്ഥിരം override VL_MT_STABLE { തിരികെ "mt"s + std::to_string(id()); }
    string hashName() സ്ഥിരം { തിരികെ m_hashName; }
    ശൂന്യം threads(പൂർണ്ണസംഖ്യ threads) { m_threads = threads; }
    പൂർണ്ണസംഖ്യ threads() സ്ഥിരം { തിരികെ m_threads; }
    ശൂന്യം dump(std::ostream& str) സ്ഥിരം;

    സ്ഥിതം uint32_t അക്കം_Used_Ids() VL_MT_SAFE { തിരികെ s_nextId; }
};

വരിയിൽ std::ostream& operator<<(std::ostream& os, സ്ഥിരം ExecMTask& rhs) {
    rhs.dump(os);
    തിരികെ os;
}

namespace V3ExecGraph {
ശൂന്യം implement(AstNetlist*) VL_MT_DISABLED;

ശൂന്യം selfTest() VL_MT_DISABLED;
}  //namespace V3ExecGraph

#endif  // Guard
