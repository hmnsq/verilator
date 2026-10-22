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

#অন্তর্ভাব "config_build.h"
#অন্তর্ভাব "verilatedos.h"

#অন্তর্ভাব "V3Graph.h"

#অন্তর্ভাব <atomic>

class AstNetlist;
class AstCFunc;
class AstExecGraph;
class Ast_গ্রন্থি_Stmt;
class AstScope;

//*************************************************************************
// MTasks and graph structures

class ExecMTask final : সর্বজনীন V3GraphVertex {
    VL_RTTI_IMPL(ExecMTask, V3GraphVertex)
ব্যক্তিগত:
    ধ্রুবক uint32_t m_id;  // Unique ID of this ExecMTask.
    স্থির std::atomic<uint32_t> s_পরবর্তী_Id;  // Next ID to use
    AstCFunc* ধ্রুবক m_funcp;  // The function that contains the task body
    ধ্রুবক std::string m_hashName;  // Hashed name based on body for profile-driven optimization
    // Predicted critical path from the start of this mtask to the ends of the graph that are
    // reachable from this mtask. In abstract time units.
    uint32_t m_priority = 0;
    // Predicted runtime of this mtask, in the same abstract time units as priority().
    uint32_t m_cost = 0;
    uint64_t m_predictStart = 0;  // Predicted start time of task
    পূর্ণসংখ্যা m_threads = 1;  // Threads used by this mtask
    VL_UNCOPYABLE(ExecMTask);

    স্থির AstCFunc* createCFunc(AstExecGraph* execGraphp, AstScope* scopep, Ast_গ্রন্থি_Stmt* stmtsp,
                                 uint32_t id);

সর্বজনীন:
    ExecMTask(AstExecGraph* execGraphp, AstScope* scopep, Ast_গ্রন্থি_Stmt* stmtsp) VL_MT_DISABLED;
    AstCFunc* funcp() ধ্রুবক { ফেরত m_funcp; }
    uint32_t id() ধ্রুবক VL_MT_SAFE { ফেরত m_id; }
    uint32_t priority() ধ্রুবক { ফেরত m_priority; }
    শূন্য priority(uint32_t pri) { m_priority = pri; }
    uint32_t cost() ধ্রুবক { ফেরত m_cost; }
    শূন্য cost(uint32_t cost) { m_cost = cost; }
    uint64_t predictStart() ধ্রুবক { ফেরত m_predictStart; }
    শূন্য predictStart(uint64_t time) { m_predictStart = time; }
    string name() ধ্রুবক override VL_MT_STABLE { ফেরত "mt"s + std::to_string(id()); }
    string hashName() ধ্রুবক { ফেরত m_hashName; }
    শূন্য threads(পূর্ণসংখ্যা threads) { m_threads = threads; }
    পূর্ণসংখ্যা threads() ধ্রুবক { ফেরত m_threads; }
    শূন্য dump(std::ostream& str) ধ্রুবক;

    স্থির uint32_t অঙ্ক_Used_Ids() VL_MT_SAFE { ফেরত s_পরবর্তী_Id; }
};

অন্তঃরেখ std::ostream& operator<<(std::ostream& os, ধ্রুবক ExecMTask& rhs) {
    rhs.dump(os);
    ফেরত os;
}

namespace V3ExecGraph {
শূন্য implement(AstNetlist*) VL_MT_DISABLED;

শূন্য selfTest() VL_MT_DISABLED;
}  //namespace V3ExecGraph

#endif  // Guard
