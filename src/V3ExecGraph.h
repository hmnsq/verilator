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

#ಅನ್ತರ್ಭಾವ "config_build.h"
#ಅನ್ತರ್ಭಾವ "verilatedos.h"

#ಅನ್ತರ್ಭಾವ "V3Graph.h"

#ಅನ್ತರ್ಭಾವ <atomic>

class AstNetlist;
class AstCFunc;
class AstExecGraph;
class Ast_ಗಂಟು_Stmt;
class AstScope;

//*************************************************************************
// MTasks and graph structures

class ExecMTask final : ಸಾರ್ವಜನಿಕ V3GraphVertex {
    VL_RTTI_IMPL(ExecMTask, V3GraphVertex)
ಗುಪ್ತ:
    ಧ್ರುವ uint32_t m_id;  // Unique ID of this ExecMTask.
    ಸ್ಥಿರ std::atomic<uint32_t> s_nextId;  // Next ID to use
    AstCFunc* ಧ್ರುವ m_funcp;  // The function that contains the task body
    ಧ್ರುವ std::string m_hashName;  // Hashed name based on body for profile-driven optimization
    // Predicted critical path from the start of this mtask to the ends of the graph that are
    // reachable from this mtask. In abstract time units.
    uint32_t m_priority = 0;
    // Predicted runtime of this mtask, in the same abstract time units as priority().
    uint32_t m_cost = 0;
    uint64_t m_predictStart = 0;  // Predicted start time of task
    ಪೂರ್ಣಾಙ್ಕ m_threads = 1;  // Threads used by this mtask
    VL_UNCOPYABLE(ExecMTask);

    ಸ್ಥಿರ AstCFunc* createCFunc(AstExecGraph* execGraphp, AstScope* scopep, Ast_ಗಂಟು_Stmt* stmtsp,
                                 uint32_t id);

ಸಾರ್ವಜನಿಕ:
    ExecMTask(AstExecGraph* execGraphp, AstScope* scopep, Ast_ಗಂಟು_Stmt* stmtsp) VL_MT_DISABLED;
    AstCFunc* funcp() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_funcp; }
    uint32_t id() ಧ್ರುವ VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ m_id; }
    uint32_t priority() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_priority; }
    ಶೂನ್ಯ priority(uint32_t pri) { m_priority = pri; }
    uint32_t cost() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_cost; }
    ಶೂನ್ಯ cost(uint32_t cost) { m_cost = cost; }
    uint64_t predictStart() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_predictStart; }
    ಶೂನ್ಯ predictStart(uint64_t time) { m_predictStart = time; }
    string name() ಧ್ರುವ override VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ "mt"s + std::to_string(id()); }
    string hashName() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_hashName; }
    ಶೂನ್ಯ threads(ಪೂರ್ಣಾಙ್ಕ threads) { m_threads = threads; }
    ಪೂರ್ಣಾಙ್ಕ threads() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_threads; }
    ಶೂನ್ಯ dump(std::ostream& str) ಧ್ರುವ;

    ಸ್ಥಿರ uint32_t ಅಂಕೆ_Used_Ids() VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ s_nextId; }
};

ಅನ್ತಃಸ್ಥ std::ostream& operator<<(std::ostream& os, ಧ್ರುವ ExecMTask& rhs) {
    rhs.dump(os);
    ಪ್ರತ್ಯಾಗಮ os;
}

namespace V3ExecGraph {
ಶೂನ್ಯ implement(AstNetlist*) VL_MT_DISABLED;

ಶೂನ್ಯ selfTest() VL_MT_DISABLED;
}  //namespace V3ExecGraph

#endif  // Guard
