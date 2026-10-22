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

#که_نه_تعریف_شوی VERILATOR_V3EXECGRAPH_H_
#تعریف_کړه VERILATOR_V3EXECGRAPH_H_

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن "V3Graph.h"

#ضمن <atomic>

class AstNetlist;
class AstCFunc;
class Ast_Exec_ګراف;
class Ast_غوټه_Stmt;
class AstScope;

//*************************************************************************
// MTasks and graph structures

class ExecMTask final : عام V3_ګراف_راس {
    VL_RTTI_IMPL(ExecMTask, V3_ګراف_راس)
خاص:
    ثابت uint32_t m_id;  // Unique ID of this ExecMTask.
    ساكن std::atomic<uint32_t> s_بل_Id;  // Next ID to use
    AstCFunc* ثابت m_funcp;  // The function that contains the task body
    ثابت std::string m_hashName;  // Hashed name based on body for profile-driven optimization
    // Predicted critical path from the start of this mtask to the ends of the graph that are
    // reachable from this mtask. In abstract time units.
    uint32_t m_لومړیتوب = 0;
    // Predicted runtime of this mtask, in the same abstract time units as priority().
    uint32_t m_لګښت = 0;
    uint64_t m_predict_پیل = 0;  // Predicted start time of task
    صحيح m_تارونه = 1;  // Threads used by this mtask
    VL_UNCOPYABLE(ExecMTask);

    ساكن AstCFunc* createCFunc(Ast_Exec_ګراف* execGraphp, AstScope* scopep, Ast_غوټه_Stmt* stmtsp,
                                 uint32_t id);

عام:
    ExecMTask(Ast_Exec_ګراف* execGraphp, AstScope* scopep, Ast_غوټه_Stmt* stmtsp) VL_MT_DISABLED;
    AstCFunc* funcp() ثابت { أرجع m_funcp; }
    uint32_t id() ثابت VL_MT_خوندي { أرجع m_id; }
    uint32_t لومړیتوب_2() ثابت { أرجع m_لومړیتوب; }
    فراغ لومړیتوب_2(uint32_t pri) { m_لومړیتوب = pri; }
    uint32_t لګښت() ثابت { أرجع m_لګښت; }
    فراغ لګښت(uint32_t لګښت) { m_لګښت = لګښت; }
    uint64_t predict_پیل() ثابت { أرجع m_predict_پیل; }
    فراغ predict_پیل(uint64_t وخت_2) { m_predict_پیل = وخت_2; }
    string name() ثابت override VL_MT_STABLE { أرجع "mt"s + std::منزل_string(id()); }
    string hashName() ثابت { أرجع m_hashName; }
    فراغ تارونه(صحيح تارونه) { m_تارونه = تارونه; }
    صحيح تارونه() ثابت { أرجع m_تارونه; }
    فراغ dump(std::ostream& str) ثابت;

    ساكن uint32_t عدد_کارول_شوی_Ids() VL_MT_خوندي { أرجع s_بل_Id; }
};

مضمن std::ostream& چلوونکی<<(std::ostream& os, ثابت ExecMTask& rhs) {
    rhs.dump(os);
    أرجع os;
}

namespace V3_Exec_ګراف {
فراغ implement(AstNetlist*) VL_MT_DISABLED;

فراغ selfTest() VL_MT_DISABLED;
}  //namespace V3ExecGraph

#د_که_پای  // Guard
