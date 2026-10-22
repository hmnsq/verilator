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

#ଅନ୍ତର୍ଭାଵ "config_build.h"
#ଅନ୍ତର୍ଭାଵ "verilatedos.h"

#ଅନ୍ତର୍ଭାଵ "V3Graph.h"

#ଅନ୍ତର୍ଭାଵ <atomic>

class AstNetlist;
class AstCFunc;
class AstExecGraph;
class Ast_ଗଣ୍ଠି_Stmt;
class AstScope;

//*************************************************************************
// MTasks and graph structures

class ExecMTask final : ସର୍ବସାଧାରଣ V3GraphVertex {
    VL_RTTI_IMPL(ExecMTask, V3GraphVertex)
ବ୍ୟକ୍ତିଗତ:
    ଅଚଳ uint32_t m_id;  // Unique ID of this ExecMTask.
    ସ୍ଥିର std::atomic<uint32_t> s_nextId;  // Next ID to use
    AstCFunc* ଅଚଳ m_funcp;  // The function that contains the task body
    ଅଚଳ std::string m_hashName;  // Hashed name based on body for profile-driven optimization
    // Predicted critical path from the start of this mtask to the ends of the graph that are
    // reachable from this mtask. In abstract time units.
    uint32_t m_priority = 0;
    // Predicted runtime of this mtask, in the same abstract time units as priority().
    uint32_t m_cost = 0;
    uint64_t m_predictStart = 0;  // Predicted start time of task
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_threads = 1;  // Threads used by this mtask
    VL_UNCOPYABLE(ExecMTask);

    ସ୍ଥିର AstCFunc* createCFunc(AstExecGraph* execGraphp, AstScope* scopep, Ast_ଗଣ୍ଠି_Stmt* stmtsp,
                                 uint32_t id);

ସର୍ବସାଧାରଣ:
    ExecMTask(AstExecGraph* execGraphp, AstScope* scopep, Ast_ଗଣ୍ଠି_Stmt* stmtsp) VL_MT_DISABLED;
    AstCFunc* funcp() ଅଚଳ { ଫେରନ୍ତୁ m_funcp; }
    uint32_t id() ଅଚଳ VL_MT_SAFE { ଫେରନ୍ତୁ m_id; }
    uint32_t priority() ଅଚଳ { ଫେରନ୍ତୁ m_priority; }
    ଶୂନ୍ୟ priority(uint32_t pri) { m_priority = pri; }
    uint32_t cost() ଅଚଳ { ଫେରନ୍ତୁ m_cost; }
    ଶୂନ୍ୟ cost(uint32_t cost) { m_cost = cost; }
    uint64_t predictStart() ଅଚଳ { ଫେରନ୍ତୁ m_predictStart; }
    ଶୂନ୍ୟ predictStart(uint64_t time) { m_predictStart = time; }
    string name() ଅଚଳ override VL_MT_STABLE { ଫେରନ୍ତୁ "mt"s + std::to_string(id()); }
    string hashName() ଅଚଳ { ଫେରନ୍ତୁ m_hashName; }
    ଶୂନ୍ୟ threads(ପୂର୍ଣ୍ଣସଂଖ୍ୟା threads) { m_threads = threads; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା threads() ଅଚଳ { ଫେରନ୍ତୁ m_threads; }
    ଶୂନ୍ୟ dump(std::ostream& str) ଅଚଳ;

    ସ୍ଥିର uint32_t ଅଙ୍କ_Used_Ids() VL_MT_SAFE { ଫେରନ୍ତୁ s_nextId; }
};

ଧାଡ଼ିଭିତରେ std::ostream& operator<<(std::ostream& os, ଅଚଳ ExecMTask& rhs) {
    rhs.dump(os);
    ଫେରନ୍ତୁ os;
}

namespace V3ExecGraph {
ଶୂନ୍ୟ implement(AstNetlist*) VL_MT_DISABLED;

ଶୂନ୍ୟ selfTest() VL_MT_DISABLED;
}  //namespace V3ExecGraph

#endif  // Guard
