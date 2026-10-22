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

#यदि_अपरिभाषितम् VERILATOR_V3EXECGRAPH_H_
#परिभाषय VERILATOR_V3EXECGRAPH_H_

#अन्तर्भाव "config_build.h"
#अन्तर्भाव "verilatedos.h"

#अन्तर्भाव "V3Graph.h"

#अन्तर्भाव <atomic>

class AstNetlist;
class AstCFunc;
class AstExecGraph;
class Ast_ग्रन्थिः_Stmt;
class AstScope;

//*************************************************************************
// MTasks and graph structures

class ExecMTask final : सार्वजनिक V3GraphVertex {
    VL_RTTI_IMPL(ExecMTask, V3GraphVertex)
गुप्त:
    ध्रुव uint32_t m_id;  // Unique ID of this ExecMTask.
    स्थिर std::atomic<uint32_t> s_अग्र_Id;  // Next ID to use
    AstCFunc* ध्रुव m_funcp;  // The function that contains the task body
    ध्रुव std::string m_hashName;  // Hashed name based on body for profile-driven optimization
    // Predicted critical path from the start of this mtask to the ends of the graph that are
    // reachable from this mtask. In abstract time units.
    uint32_t m_प्राथमिकता = 0;
    // Predicted runtime of this mtask, in the same abstract time units as priority().
    uint32_t m_cost = 0;
    uint64_t m_predictStart = 0;  // Predicted start time of task
    पूर्णाङ्क m_तन्तवः = 1;  // Threads used by this mtask
    VL_UNCOPYABLE(ExecMTask);

    स्थिर AstCFunc* createCFunc(AstExecGraph* execGraphp, AstScope* scopep, Ast_ग्रन्थिः_Stmt* stmtsp,
                                 uint32_t id);

सार्वजनिक:
    ExecMTask(AstExecGraph* execGraphp, AstScope* scopep, Ast_ग्रन्थिः_Stmt* stmtsp) VL_MT_DISABLED;
    AstCFunc* funcp() ध्रुव { प्रत्यागम m_funcp; }
    uint32_t id() ध्रुव VL_MT_SAFE { प्रत्यागम m_id; }
    uint32_t प्राथमिकता_2() ध्रुव { प्रत्यागम m_प्राथमिकता; }
    शून्य प्राथमिकता_2(uint32_t pri) { m_प्राथमिकता = pri; }
    uint32_t cost() ध्रुव { प्रत्यागम m_cost; }
    शून्य cost(uint32_t cost) { m_cost = cost; }
    uint64_t predictStart() ध्रुव { प्रत्यागम m_predictStart; }
    शून्य predictStart(uint64_t time) { m_predictStart = time; }
    string name() ध्रुव override VL_MT_STABLE { प्रत्यागम "mt"s + std::गन्तव्यम्_string(id()); }
    string hashName() ध्रुव { प्रत्यागम m_hashName; }
    शून्य तन्तवः(पूर्णाङ्क तन्तवः) { m_तन्तवः = तन्तवः; }
    पूर्णाङ्क तन्तवः() ध्रुव { प्रत्यागम m_तन्तवः; }
    शून्य dump(std::ostream& str) ध्रुव;

    स्थिर uint32_t अङ्कः_प्रयुक्तम्_Ids() VL_MT_SAFE { प्रत्यागम s_अग्र_Id; }
};

अन्तःस्थ std::ostream& संक्रियकः<<(std::ostream& os, ध्रुव ExecMTask& rhs) {
    rhs.dump(os);
    प्रत्यागम os;
}

namespace V3ExecGraph {
शून्य implement(AstNetlist*) VL_MT_DISABLED;

शून्य selfTest() VL_MT_DISABLED;
}  //namespace V3ExecGraph

#यदि_अन्तः  // Guard
