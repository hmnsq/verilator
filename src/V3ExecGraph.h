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

#अगर_अघोषित VERILATOR_V3EXECGRAPH_H_
#घोषणा VERILATOR_V3EXECGRAPH_H_

#समावेश "config_build.h"
#समावेश "verilatedos.h"

#समावेश "V3Graph.h"

#समावेश <atomic>

class AstNetlist;
class AstCFunc;
class AstExecGraph;
class Ast_गाँठ_Stmt;
class AstScope;

//*************************************************************************
// MTasks and graph structures

class ExecMTask final : खुला V3GraphVertex {
    VL_RTTI_IMPL(ExecMTask, V3GraphVertex)
निजी:
    स्थिर uint32_t m_id;  // Unique ID of this ExecMTask.
    अटल std::atomic<uint32_t> s_अग्र_Id;  // Next ID to use
    AstCFunc* स्थिर m_funcp;  // The function that contains the task body
    स्थिर std::string m_hashName;  // Hashed name based on body for profile-driven optimization
    // Predicted critical path from the start of this mtask to the ends of the graph that are
    // reachable from this mtask. In abstract time units.
    uint32_t m_priority = 0;
    // Predicted runtime of this mtask, in the same abstract time units as priority().
    uint32_t m_cost = 0;
    uint64_t m_predictStart = 0;  // Predicted start time of task
    पूर्णांक m_threads = 1;  // Threads used by this mtask
    VL_UNCOPYABLE(ExecMTask);

    अटल AstCFunc* createCFunc(AstExecGraph* execGraphp, AstScope* scopep, Ast_गाँठ_Stmt* stmtsp,
                                 uint32_t id);

खुला:
    ExecMTask(AstExecGraph* execGraphp, AstScope* scopep, Ast_गाँठ_Stmt* stmtsp) VL_MT_DISABLED;
    AstCFunc* funcp() स्थिर { वापस m_funcp; }
    uint32_t id() स्थिर VL_MT_SAFE { वापस m_id; }
    uint32_t priority() स्थिर { वापस m_priority; }
    व्योम priority(uint32_t pri) { m_priority = pri; }
    uint32_t cost() स्थिर { वापस m_cost; }
    व्योम cost(uint32_t cost) { m_cost = cost; }
    uint64_t predictStart() स्थिर { वापस m_predictStart; }
    व्योम predictStart(uint64_t समय) { m_predictStart = समय; }
    string name() स्थिर override VL_MT_STABLE { वापस "mt"s + std::to_string(id()); }
    string hashName() स्थिर { वापस m_hashName; }
    व्योम threads(पूर्णांक threads) { m_threads = threads; }
    पूर्णांक threads() स्थिर { वापस m_threads; }
    व्योम dump(std::ostream& str) स्थिर;

    अटल uint32_t अंक_Used_Ids() VL_MT_SAFE { वापस s_अग्र_Id; }
};

अंतरभूत std::ostream& चालक<<(std::ostream& os, स्थिर ExecMTask& rhs) {
    rhs.dump(os);
    वापस os;
}

namespace V3ExecGraph {
व्योम implement(AstNetlist*) VL_MT_DISABLED;

व्योम selfTest() VL_MT_DISABLED;
}  //namespace V3ExecGraph

#पूर्ण_अगर  // Guard
