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

#அந்தர்பாவ "config_build.h"
#அந்தர்பாவ "verilatedos.h"

#அந்தர்பாவ "V3Graph.h"

#அந்தர்பாவ <atomic>

class AstNetlist;
class AstCFunc;
class AstExecGraph;
class Ast_முனை_Stmt;
class AstScope;

//*************************************************************************
// MTasks and graph structures

class ExecMTask final : பொது V3GraphVertex {
    VL_RTTI_IMPL(ExecMTask, V3GraphVertex)
தனிப்பட்ட:
    மாறிலி uint32_t m_id;  // Unique ID of this ExecMTask.
    நிலையான std::atomic<uint32_t> s_nextId;  // Next ID to use
    AstCFunc* மாறிலி m_funcp;  // The function that contains the task body
    மாறிலி std::string m_hashName;  // Hashed name based on body for profile-driven optimization
    // Predicted critical path from the start of this mtask to the ends of the graph that are
    // reachable from this mtask. In abstract time units.
    uint32_t m_priority = 0;
    // Predicted runtime of this mtask, in the same abstract time units as priority().
    uint32_t m_cost = 0;
    uint64_t m_predictStart = 0;  // Predicted start time of task
    முழு m_threads = 1;  // Threads used by this mtask
    VL_UNCOPYABLE(ExecMTask);

    நிலையான AstCFunc* createCFunc(AstExecGraph* execGraphp, AstScope* scopep, Ast_முனை_Stmt* stmtsp,
                                 uint32_t id);

பொது:
    ExecMTask(AstExecGraph* execGraphp, AstScope* scopep, Ast_முனை_Stmt* stmtsp) VL_MT_DISABLED;
    AstCFunc* funcp() மாறிலி { திரும்பு m_funcp; }
    uint32_t id() மாறிலி VL_MT_SAFE { திரும்பு m_id; }
    uint32_t priority() மாறிலி { திரும்பு m_priority; }
    வெறுமை priority(uint32_t pri) { m_priority = pri; }
    uint32_t cost() மாறிலி { திரும்பு m_cost; }
    வெறுமை cost(uint32_t cost) { m_cost = cost; }
    uint64_t predictStart() மாறிலி { திரும்பு m_predictStart; }
    வெறுமை predictStart(uint64_t time) { m_predictStart = time; }
    string name() மாறிலி override VL_MT_STABLE { திரும்பு "mt"s + std::to_string(id()); }
    string hashName() மாறிலி { திரும்பு m_hashName; }
    வெறுமை threads(முழு threads) { m_threads = threads; }
    முழு threads() மாறிலி { திரும்பு m_threads; }
    வெறுமை dump(std::ostream& str) மாறிலி;

    நிலையான uint32_t இலக்கம்_Used_Ids() VL_MT_SAFE { திரும்பு s_nextId; }
};

உள்வரி std::ostream& operator<<(std::ostream& os, மாறிலி ExecMTask& rhs) {
    rhs.dump(os);
    திரும்பு os;
}

namespace V3ExecGraph {
வெறுமை implement(AstNetlist*) VL_MT_DISABLED;

வெறுமை selfTest() VL_MT_DISABLED;
}  //namespace V3ExecGraph

#endif  // Guard
