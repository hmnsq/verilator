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

#جے_بنا_تعریف VERILATOR_V3EXECGRAPH_H_
#تعریف_کرو VERILATOR_V3EXECGRAPH_H_

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن "V3Graph.h"

#ضمن <atomic>

class AstNetlist;
class AstCFunc;
class Ast_Exec_گراف;
class Ast_گرہ_Stmt;
class AstScope;

//*************************************************************************
// MTasks and graph structures

class ExecMTask final : عام V3_گراف_راس {
    VL_RTTI_IMPL(ExecMTask, V3_گراف_راس)
خاص:
    ثابت uint32_t m_id;  // Unique ID of this ExecMTask.
    ساكن std::atomic<uint32_t> s_اگلا_Id;  // Next ID to use
    AstCFunc* ثابت m_funcp;  // The function that contains the task body
    ثابت std::string m_hashName;  // Hashed name based on body for profile-driven optimization
    // Predicted critical path from the start of this mtask to the ends of the graph that are
    // reachable from this mtask. In abstract time units.
    uint32_t m_ترجیح = 0;
    // Predicted runtime of this mtask, in the same abstract time units as priority().
    uint32_t m_لاگت = 0;
    uint64_t m_predict_شروع = 0;  // Predicted start time of task
    صحيح m_دھاگے = 1;  // Threads used by this mtask
    VL_UNCOPYABLE(ExecMTask);

    ساكن AstCFunc* createCFunc(Ast_Exec_گراف* execGraphp, AstScope* scopep, Ast_گرہ_Stmt* stmtsp,
                                 uint32_t id);

عام:
    ExecMTask(Ast_Exec_گراف* execGraphp, AstScope* scopep, Ast_گرہ_Stmt* stmtsp) VL_MT_DISABLED;
    AstCFunc* funcp() ثابت { أرجع m_funcp; }
    uint32_t id() ثابت VL_MT_محفوظ { أرجع m_id; }
    uint32_t ترجیح_2() ثابت { أرجع m_ترجیح; }
    فراغ ترجیح_2(uint32_t pri) { m_ترجیح = pri; }
    uint32_t لاگت() ثابت { أرجع m_لاگت; }
    فراغ لاگت(uint32_t لاگت) { m_لاگت = لاگت; }
    uint64_t predict_شروع() ثابت { أرجع m_predict_شروع; }
    فراغ predict_شروع(uint64_t ویلا_2) { m_predict_شروع = ویلا_2; }
    string name() ثابت override VL_MT_STABLE { أرجع "mt"s + std::منزل_string(id()); }
    string hashName() ثابت { أرجع m_hashName; }
    فراغ دھاگے(صحيح دھاگے) { m_دھاگے = دھاگے; }
    صحيح دھاگے() ثابت { أرجع m_دھاگے; }
    فراغ dump(std::ostream& str) ثابت;

    ساكن uint32_t عدد_ورتیا_Ids() VL_MT_محفوظ { أرجع s_اگلا_Id; }
};

مضمن std::ostream& عامل<<(std::ostream& os, ثابت ExecMTask& rhs) {
    rhs.dump(os);
    أرجع os;
}

namespace V3_Exec_گراف {
فراغ implement(AstNetlist*) VL_MT_DISABLED;

فراغ selfTest() VL_MT_DISABLED;
}  //namespace V3ExecGraph

#جے_انت  // Guard
