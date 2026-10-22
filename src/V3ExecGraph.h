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

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن "V3Graph.h"

#ضمن <atomic>

class AstNetlist;
class AstCFunc;
class AstExecGraph;
class AstNodeStmt;
class AstScope;

//*************************************************************************
// MTasks and graph structures

class ExecMTask final : عام V3GraphVertex {
    VL_RTTI_IMPL(ExecMTask, V3GraphVertex)
خاص:
    ثابت uint32_t m_id;  // Unique ID of this ExecMTask.
    ساكن std::atomic<uint32_t> s_nextId;  // Next ID to use
    AstCFunc* ثابت m_funcp;  // The function that contains the task body
    ثابت std::string m_hashName;  // Hashed name based on body for profile-driven optimization
    // Predicted critical path from the start of this mtask to the ends of the graph that are
    // reachable from this mtask. In abstract time units.
    uint32_t m_priority = 0;
    // Predicted runtime of this mtask, in the same abstract time units as priority().
    uint32_t m_cost = 0;
    uint64_t m_predictStart = 0;  // Predicted start time of task
    صحيح m_threads = 1;  // Threads used by this mtask
    VL_UNCOPYABLE(ExecMTask);

    ساكن AstCFunc* createCFunc(AstExecGraph* execGraphp, AstScope* scopep, AstNodeStmt* stmtsp,
                                 uint32_t id);

عام:
    ExecMTask(AstExecGraph* execGraphp, AstScope* scopep, AstNodeStmt* stmtsp) VL_MT_DISABLED;
    AstCFunc* funcp() ثابت { ܦܢܐ m_funcp; }
    uint32_t id() ثابت VL_MT_SAFE { ܦܢܐ m_id; }
    uint32_t priority() ثابت { ܦܢܐ m_priority; }
    فراغ priority(uint32_t pri) { m_priority = pri; }
    uint32_t cost() ثابت { ܦܢܐ m_cost; }
    فراغ cost(uint32_t cost) { m_cost = cost; }
    uint64_t predictStart() ثابت { ܦܢܐ m_predictStart; }
    فراغ predictStart(uint64_t time) { m_predictStart = time; }
    string name() ثابت override VL_MT_STABLE { ܦܢܐ "mt"s + std::to_string(id()); }
    string hashName() ثابت { ܦܢܐ m_hashName; }
    فراغ threads(صحيح threads) { m_threads = threads; }
    صحيح threads() ثابت { ܦܢܐ m_threads; }
    فراغ dump(std::ostream& str) ثابت;

    ساكن uint32_t numUsedIds() VL_MT_SAFE { ܦܢܐ s_nextId; }
};

مضمن std::ostream& operator<<(std::ostream& os, ثابت ExecMTask& rhs) {
    rhs.dump(os);
    ܦܢܐ os;
}

namespace V3ExecGraph {
فراغ implement(AstNetlist*) VL_MT_DISABLED;

فراغ selfTest() VL_MT_DISABLED;
}  //namespace V3ExecGraph

#endif  // Guard
