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

#অন্তর্ভাৱ "config_build.h"
#অন্তর্ভাৱ "verilatedos.h"

#অন্তর্ভাৱ "V3Graph.h"

#অন্তর্ভাৱ <atomic>

class AstNetlist;
class AstCFunc;
class AstExecGraph;
class Ast_গ্ৰন্থি_Stmt;
class AstScope;

//*************************************************************************
// MTasks and graph structures

class ExecMTask final : সার্ৱজনিক V3GraphVertex {
    VL_RTTI_IMPL(ExecMTask, V3GraphVertex)
গুপ্ত:
    ধ্রুৱ uint32_t m_id;  // Unique ID of this ExecMTask.
    স্থির std::atomic<uint32_t> s_nextId;  // Next ID to use
    AstCFunc* ধ্রুৱ m_funcp;  // The function that contains the task body
    ধ্রুৱ std::string m_hashName;  // Hashed name based on body for profile-driven optimization
    // Predicted critical path from the start of this mtask to the ends of the graph that are
    // reachable from this mtask. In abstract time units.
    uint32_t m_priority = 0;
    // Predicted runtime of this mtask, in the same abstract time units as priority().
    uint32_t m_cost = 0;
    uint64_t m_predictStart = 0;  // Predicted start time of task
    পূর্ণাঙ্ক m_threads = 1;  // Threads used by this mtask
    VL_UNCOPYABLE(ExecMTask);

    স্থির AstCFunc* createCFunc(AstExecGraph* execGraphp, AstScope* scopep, Ast_গ্ৰন্থি_Stmt* stmtsp,
                                 uint32_t id);

সার্ৱজনিক:
    ExecMTask(AstExecGraph* execGraphp, AstScope* scopep, Ast_গ্ৰন্থি_Stmt* stmtsp) VL_MT_DISABLED;
    AstCFunc* funcp() ধ্রুৱ { ঘূৰাই m_funcp; }
    uint32_t id() ধ্রুৱ VL_MT_SAFE { ঘূৰাই m_id; }
    uint32_t priority() ধ্রুৱ { ঘূৰাই m_priority; }
    শূন্য priority(uint32_t pri) { m_priority = pri; }
    uint32_t cost() ধ্রুৱ { ঘূৰাই m_cost; }
    শূন্য cost(uint32_t cost) { m_cost = cost; }
    uint64_t predictStart() ধ্রুৱ { ঘূৰাই m_predictStart; }
    শূন্য predictStart(uint64_t time) { m_predictStart = time; }
    string name() ধ্রুৱ override VL_MT_STABLE { ঘূৰাই "mt"s + std::to_string(id()); }
    string hashName() ধ্রুৱ { ঘূৰাই m_hashName; }
    শূন্য threads(পূর্ণাঙ্ক threads) { m_threads = threads; }
    পূর্ণাঙ্ক threads() ধ্রুৱ { ঘূৰাই m_threads; }
    শূন্য dump(std::ostream& str) ধ্রুৱ;

    স্থির uint32_t অঙ্ক_Used_Ids() VL_MT_SAFE { ঘূৰাই s_nextId; }
};

অন্তঃস্থ std::ostream& operator<<(std::ostream& os, ধ্রুৱ ExecMTask& rhs) {
    rhs.dump(os);
    ঘূৰাই os;
}

namespace V3ExecGraph {
শূন্য implement(AstNetlist*) VL_MT_DISABLED;

শূন্য selfTest() VL_MT_DISABLED;
}  //namespace V3ExecGraph

#endif  // Guard
