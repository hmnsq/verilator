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

#अन्तर्भाव "config_build.h"
#अन्तर्भाव "verilatedos.h"

#अन्तर्भाव "V3Graph.h"

#अन्तर्भाव <atomic>

class AstNetlist;
class AstCFunc;
class AstExecGraph;
class Ast_गाठ_Stmt;
class AstScope;

//*************************************************************************
// MTasks and graph structures

class ExecMTask final : सार्वजनिक V3GraphVertex {
    VL_RTTI_IMPL(ExecMTask, V3GraphVertex)
खाजगी:
    स्थिरांक uint32_t m_id;  // Unique ID of this ExecMTask.
    स्थिर std::atomic<uint32_t> s_nextId;  // Next ID to use
    AstCFunc* स्थिरांक m_funcp;  // The function that contains the task body
    स्थिरांक std::string m_hashName;  // Hashed name based on body for profile-driven optimization
    // Predicted critical path from the start of this mtask to the ends of the graph that are
    // reachable from this mtask. In abstract time units.
    uint32_t m_priority = 0;
    // Predicted runtime of this mtask, in the same abstract time units as priority().
    uint32_t m_cost = 0;
    uint64_t m_predictStart = 0;  // Predicted start time of task
    पूर्णांक m_threads = 1;  // Threads used by this mtask
    VL_UNCOPYABLE(ExecMTask);

    स्थिर AstCFunc* createCFunc(AstExecGraph* execGraphp, AstScope* scopep, Ast_गाठ_Stmt* stmtsp,
                                 uint32_t id);

सार्वजनिक:
    ExecMTask(AstExecGraph* execGraphp, AstScope* scopep, Ast_गाठ_Stmt* stmtsp) VL_MT_DISABLED;
    AstCFunc* funcp() स्थिरांक { परत m_funcp; }
    uint32_t id() स्थिरांक VL_MT_SAFE { परत m_id; }
    uint32_t priority() स्थिरांक { परत m_priority; }
    शून्य priority(uint32_t pri) { m_priority = pri; }
    uint32_t cost() स्थिरांक { परत m_cost; }
    शून्य cost(uint32_t cost) { m_cost = cost; }
    uint64_t predictStart() स्थिरांक { परत m_predictStart; }
    शून्य predictStart(uint64_t समय) { m_predictStart = समय; }
    string name() स्थिरांक override VL_MT_STABLE { परत "mt"s + std::to_string(id()); }
    string hashName() स्थिरांक { परत m_hashName; }
    शून्य threads(पूर्णांक threads) { m_threads = threads; }
    पूर्णांक threads() स्थिरांक { परत m_threads; }
    शून्य dump(std::ostream& str) स्थिरांक;

    स्थिर uint32_t अंक_Used_Ids() VL_MT_SAFE { परत s_nextId; }
};

ओळीत std::ostream& चालक<<(std::ostream& os, स्थिरांक ExecMTask& rhs) {
    rhs.dump(os);
    परत os;
}

namespace V3ExecGraph {
शून्य implement(AstNetlist*) VL_MT_DISABLED;

शून्य selfTest() VL_MT_DISABLED;
}  //namespace V3ExecGraph

#endif  // Guard
