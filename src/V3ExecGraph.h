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

#اگر_غیر_تعریف_شدہ VERILATOR_V3EXECGRAPH_H_
#تعریف_کرو VERILATOR_V3EXECGRAPH_H_

#شامل "config_build.h"
#شامل "verilatedos.h"

#شامل "V3Graph.h"

#شامل <atomic>

class AstNetlist;
class AstCFunc;
class Ast_Exec_گراف;
class Ast_گرہ_Stmt;
class AstScope;

//*************************************************************************
// MTasks and graph structures

class ExecMTask final : عوامی V3_گراف_راس {
    VL_RTTI_IMPL(ExecMTask, V3_گراف_راس)
نجی:
    ثابت uint32_t m_id;  // Unique ID of this ExecMTask.
    ساکن std::atomic<uint32_t> s_اگلا_Id;  // Next ID to use
    AstCFunc* ثابت m_funcp;  // The function that contains the task body
    ثابت std::string m_hashName;  // Hashed name based on body for profile-driven optimization
    // Predicted critical path from the start of this mtask to the ends of the graph that are
    // reachable from this mtask. In abstract time units.
    uint32_t m_ترجیح = 0;
    // Predicted runtime of this mtask, in the same abstract time units as priority().
    uint32_t m_لاگت = 0;
    uint64_t m_predict_آغاز = 0;  // Predicted start time of task
    صحیح m_دھاگے = 1;  // Threads used by this mtask
    VL_UNCOPYABLE(ExecMTask);

    ساکن AstCFunc* createCFunc(Ast_Exec_گراف* execGraphp, AstScope* scopep, Ast_گرہ_Stmt* stmtsp,
                                 uint32_t id);

عوامی:
    ExecMTask(Ast_Exec_گراف* execGraphp, AstScope* scopep, Ast_گرہ_Stmt* stmtsp) VL_MT_DISABLED;
    AstCFunc* funcp() ثابت { واپس m_funcp; }
    uint32_t id() ثابت VL_MT_محفوظ { واپس m_id; }
    uint32_t ترجیح_2() ثابت { واپس m_ترجیح; }
    خالی ترجیح_2(uint32_t pri) { m_ترجیح = pri; }
    uint32_t لاگت() ثابت { واپس m_لاگت; }
    خالی لاگت(uint32_t لاگت) { m_لاگت = لاگت; }
    uint64_t predict_آغاز() ثابت { واپس m_predict_آغاز; }
    خالی predict_آغاز(uint64_t وقت_2) { m_predict_آغاز = وقت_2; }
    string name() ثابت override VL_MT_STABLE { واپس "mt"s + std::مقصد_string(id()); }
    string hashName() ثابت { واپس m_hashName; }
    خالی دھاگے(صحیح دھاگے) { m_دھاگے = دھاگے; }
    صحیح دھاگے() ثابت { واپس m_دھاگے; }
    خالی dump(std::ostream& str) ثابت;

    ساکن uint32_t عدد_مستعمل_Ids() VL_MT_محفوظ { واپس s_اگلا_Id; }
};

اندرونی std::ostream& عامل<<(std::ostream& os, ثابت ExecMTask& rhs) {
    rhs.dump(os);
    واپس os;
}

namespace V3_Exec_گراف {
خالی implement(AstNetlist*) VL_MT_DISABLED;

خالی selfTest() VL_MT_DISABLED;
}  //namespace V3ExecGraph

#اگر_ختم  // Guard
