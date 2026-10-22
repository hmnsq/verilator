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

#شامل "config_build.h"
#شامل "verilatedos.h"

#شامل "V3Graph.h"

#شامل <atomic>

class AstNetlist;
class AstCFunc;
class AstExecGraph;
class Ast_گره_Stmt;
class AstScope;

//*************************************************************************
// MTasks and graph structures

class ExecMTask final : عمومی V3GraphVertex {
    VL_RTTI_IMPL(ExecMTask, V3GraphVertex)
خصوصی:
    ثابت uint32_t m_id;  // Unique ID of this ExecMTask.
    ایستا std::atomic<uint32_t> s_nextId;  // Next ID to use
    AstCFunc* ثابت m_funcp;  // The function that contains the task body
    ثابت std::string m_hashName;  // Hashed name based on body for profile-driven optimization
    // Predicted critical path from the start of this mtask to the ends of the graph that are
    // reachable from this mtask. In abstract time units.
    uint32_t m_priority = 0;
    // Predicted runtime of this mtask, in the same abstract time units as priority().
    uint32_t m_cost = 0;
    uint64_t m_predictStart = 0;  // Predicted start time of task
    صحیح m_threads = 1;  // Threads used by this mtask
    VL_UNCOPYABLE(ExecMTask);

    ایستا AstCFunc* createCFunc(AstExecGraph* execGraphp, AstScope* scopep, Ast_گره_Stmt* stmtsp,
                                 uint32_t id);

عمومی:
    ExecMTask(AstExecGraph* execGraphp, AstScope* scopep, Ast_گره_Stmt* stmtsp) VL_MT_DISABLED;
    AstCFunc* funcp() ثابت { بازگشت m_funcp; }
    uint32_t id() ثابت VL_MT_SAFE { بازگشت m_id; }
    uint32_t priority() ثابت { بازگشت m_priority; }
    تهی priority(uint32_t pri) { m_priority = pri; }
    uint32_t cost() ثابت { بازگشت m_cost; }
    تهی cost(uint32_t cost) { m_cost = cost; }
    uint64_t predictStart() ثابت { بازگشت m_predictStart; }
    تهی predictStart(uint64_t time) { m_predictStart = time; }
    string name() ثابت override VL_MT_STABLE { بازگشت "mt"s + std::to_string(id()); }
    string hashName() ثابت { بازگشت m_hashName; }
    تهی threads(صحیح threads) { m_threads = threads; }
    صحیح threads() ثابت { بازگشت m_threads; }
    تهی dump(std::ostream& str) ثابت;

    ایستا uint32_t رقم_Used_Ids() VL_MT_SAFE { بازگشت s_nextId; }
};

درونخطی std::ostream& operator<<(std::ostream& os, ثابت ExecMTask& rhs) {
    rhs.dump(os);
    بازگشت os;
}

namespace V3ExecGraph {
تهی implement(AstNetlist*) VL_MT_DISABLED;

تهی selfTest() VL_MT_DISABLED;
}  //namespace V3ExecGraph

#endif  // Guard
