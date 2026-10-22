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
class Ast_गाँठो_Stmt;
class AstScope;

//*************************************************************************
// MTasks and graph structures

class ExecMTask final : सार्वजनिक V3GraphVertex {
    VL_RTTI_IMPL(ExecMTask, V3GraphVertex)
गुप्त:
    अचर uint32_t m_id;  // Unique ID of this ExecMTask.
    स्थिर std::atomic<uint32_t> s_nextId;  // Next ID to use
    AstCFunc* अचर m_funcp;  // The function that contains the task body
    अचर std::string m_hashName;  // Hashed name based on body for profile-driven optimization
    // Predicted critical path from the start of this mtask to the ends of the graph that are
    // reachable from this mtask. In abstract time units.
    uint32_t m_priority = 0;
    // Predicted runtime of this mtask, in the same abstract time units as priority().
    uint32_t m_cost = 0;
    uint64_t m_predictStart = 0;  // Predicted start time of task
    पूर्णाङ्क m_threads = 1;  // Threads used by this mtask
    VL_UNCOPYABLE(ExecMTask);

    स्थिर AstCFunc* createCFunc(AstExecGraph* execGraphp, AstScope* scopep, Ast_गाँठो_Stmt* stmtsp,
                                 uint32_t id);

सार्वजनिक:
    ExecMTask(AstExecGraph* execGraphp, AstScope* scopep, Ast_गाँठो_Stmt* stmtsp) VL_MT_DISABLED;
    AstCFunc* funcp() अचर { फर्काऊ m_funcp; }
    uint32_t id() अचर VL_MT_SAFE { फर्काऊ m_id; }
    uint32_t priority() अचर { फर्काऊ m_priority; }
    शून्य priority(uint32_t pri) { m_priority = pri; }
    uint32_t cost() अचर { फर्काऊ m_cost; }
    शून्य cost(uint32_t cost) { m_cost = cost; }
    uint64_t predictStart() अचर { फर्काऊ m_predictStart; }
    शून्य predictStart(uint64_t समय) { m_predictStart = समय; }
    string name() अचर override VL_MT_STABLE { फर्काऊ "mt"s + std::to_string(id()); }
    string hashName() अचर { फर्काऊ m_hashName; }
    शून्य threads(पूर्णाङ्क threads) { m_threads = threads; }
    पूर्णाङ्क threads() अचर { फर्काऊ m_threads; }
    शून्य dump(std::ostream& str) अचर;

    स्थिर uint32_t अङ्क_Used_Ids() VL_MT_SAFE { फर्काऊ s_nextId; }
};

अन्तःस्थ std::ostream& चालक<<(std::ostream& os, अचर ExecMTask& rhs) {
    rhs.dump(os);
    फर्काऊ os;
}

namespace V3ExecGraph {
शून्य implement(AstNetlist*) VL_MT_DISABLED;

शून्य selfTest() VL_MT_DISABLED;
}  //namespace V3ExecGraph

#endif  // Guard
