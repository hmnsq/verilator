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

#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ VERILATOR_V3EXECGRAPH_H_
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VERILATOR_V3EXECGRAPH_H_

#ਅਨ੍ਤਰ੍ਭਾਵ "config_build.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "verilatedos.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Graph.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <atomic>

class AstNetlist;
class AstCFunc;
class Ast_Exec_ਗ੍ਰਾਫ;
class Ast_ਗੰਢ_Stmt;
class AstScope;

//*************************************************************************
// MTasks and graph structures

class ExecMTask final : ਜਨਤਕ V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼ {
    VL_RTTI_IMPL(ExecMTask, V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼)
ਨਿਜੀ:
    ਅਚਲ uint32_t m_id;  // Unique ID of this ExecMTask.
    ਸਥਿਰ std::atomic<uint32_t> s_ਅਗਲਾ_Id;  // Next ID to use
    AstCFunc* ਅਚਲ m_funcp;  // The function that contains the task body
    ਅਚਲ std::string m_hashName;  // Hashed name based on body for profile-driven optimization
    // Predicted critical path from the start of this mtask to the ends of the graph that are
    // reachable from this mtask. In abstract time units.
    uint32_t m_ਤਰਜੀਹ = 0;
    // Predicted runtime of this mtask, in the same abstract time units as priority().
    uint32_t m_ਲਾਗਤ = 0;
    uint64_t m_predict_ਸ਼ੁਰੂ = 0;  // Predicted start time of task
    ਪੂਰਨ m_ਧਾਗੇ = 1;  // Threads used by this mtask
    VL_UNCOPYABLE(ExecMTask);

    ਸਥਿਰ AstCFunc* createCFunc(Ast_Exec_ਗ੍ਰਾਫ* execGraphp, AstScope* scopep, Ast_ਗੰਢ_Stmt* stmtsp,
                                 uint32_t id);

ਜਨਤਕ:
    ExecMTask(Ast_Exec_ਗ੍ਰਾਫ* execGraphp, AstScope* scopep, Ast_ਗੰਢ_Stmt* stmtsp) VL_MT_DISABLED;
    AstCFunc* funcp() ਅਚਲ { ਵਾਪਸ m_funcp; }
    uint32_t id() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ m_id; }
    uint32_t ਤਰਜੀਹ_2() ਅਚਲ { ਵਾਪਸ m_ਤਰਜੀਹ; }
    ਖਾਲੀ ਤਰਜੀਹ_2(uint32_t pri) { m_ਤਰਜੀਹ = pri; }
    uint32_t ਲਾਗਤ() ਅਚਲ { ਵਾਪਸ m_ਲਾਗਤ; }
    ਖਾਲੀ ਲਾਗਤ(uint32_t ਲਾਗਤ) { m_ਲਾਗਤ = ਲਾਗਤ; }
    uint64_t predict_ਸ਼ੁਰੂ() ਅਚਲ { ਵਾਪਸ m_predict_ਸ਼ੁਰੂ; }
    ਖਾਲੀ predict_ਸ਼ੁਰੂ(uint64_t ਸਮਾਂ_2) { m_predict_ਸ਼ੁਰੂ = ਸਮਾਂ_2; }
    string name() ਅਚਲ override VL_MT_STABLE { ਵਾਪਸ "mt"s + std::ਮੰਜ਼ਿਲ_string(id()); }
    string hashName() ਅਚਲ { ਵਾਪਸ m_hashName; }
    ਖਾਲੀ ਧਾਗੇ(ਪੂਰਨ ਧਾਗੇ) { m_ਧਾਗੇ = ਧਾਗੇ; }
    ਪੂਰਨ ਧਾਗੇ() ਅਚਲ { ਵਾਪਸ m_ਧਾਗੇ; }
    ਖਾਲੀ dump(std::ostream& str) ਅਚਲ;

    ਸਥਿਰ uint32_t ਅੰਕ_ਵਰਤਿਆ_Ids() VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ s_ਅਗਲਾ_Id; }
};

ਸਤਰਅੰਦਰ std::ostream& ਸੰਚਾਲਕ<<(std::ostream& os, ਅਚਲ ExecMTask& rhs) {
    rhs.dump(os);
    ਵਾਪਸ os;
}

namespace V3_Exec_ਗ੍ਰਾਫ {
ਖਾਲੀ implement(AstNetlist*) VL_MT_DISABLED;

ਖਾਲੀ selfTest() VL_MT_DISABLED;
}  //namespace V3ExecGraph

#ਜੇ_ਅੰਤ  // Guard
