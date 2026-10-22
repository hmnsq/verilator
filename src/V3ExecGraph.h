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

#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ VERILATOR_V3EXECGRAPH_H_
#ವ್ಯಾಖ್ಯಾನಿಸು VERILATOR_V3EXECGRAPH_H_

#ಅನ್ತರ್ಭಾವ "config_build.h"
#ಅನ್ತರ್ಭಾವ "verilatedos.h"

#ಅನ್ತರ್ಭಾವ "V3Graph.h"

#ಅನ್ತರ್ಭಾವ <atomic>

class AstNetlist;
class AstCFunc;
class Ast_Exec_ನಕ್ಷೆ;
class Ast_ಗಂಟು_Stmt;
class AstScope;

//*************************************************************************
// MTasks and graph structures

class ExecMTask final : ಸಾರ್ವಜನಿಕ V3_ನಕ್ಷೆ_ಶೃಂಗ {
    VL_RTTI_IMPL(ExecMTask, V3_ನಕ್ಷೆ_ಶೃಂಗ)
ಖಾಸಗಿ:
    ಸ್ಥಿರಾಂಕ uint32_t m_id;  // Unique ID of this ExecMTask.
    ಸ್ಥಿರ std::atomic<uint32_t> s_ಮುಂದಿನ_Id;  // Next ID to use
    AstCFunc* ಸ್ಥಿರಾಂಕ m_funcp;  // The function that contains the task body
    ಸ್ಥಿರಾಂಕ std::string m_hashName;  // Hashed name based on body for profile-driven optimization
    // Predicted critical path from the start of this mtask to the ends of the graph that are
    // reachable from this mtask. In abstract time units.
    uint32_t m_ಆದ್ಯತೆ = 0;
    // Predicted runtime of this mtask, in the same abstract time units as priority().
    uint32_t m_ವೆಚ್ಚ = 0;
    uint64_t m_predict_ಆರಂಭ = 0;  // Predicted start time of task
    ಪೂರ್ಣಾಂಕ m_ಎಳೆಗಳು = 1;  // Threads used by this mtask
    VL_UNCOPYABLE(ExecMTask);

    ಸ್ಥಿರ AstCFunc* createCFunc(Ast_Exec_ನಕ್ಷೆ* execGraphp, AstScope* scopep, Ast_ಗಂಟು_Stmt* stmtsp,
                                 uint32_t id);

ಸಾರ್ವಜನಿಕ:
    ExecMTask(Ast_Exec_ನಕ್ಷೆ* execGraphp, AstScope* scopep, Ast_ಗಂಟು_Stmt* stmtsp) VL_MT_DISABLED;
    AstCFunc* funcp() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_funcp; }
    uint32_t id() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ m_id; }
    uint32_t ಆದ್ಯತೆ_2() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಆದ್ಯತೆ; }
    ಶೂನ್ಯ ಆದ್ಯತೆ_2(uint32_t pri) { m_ಆದ್ಯತೆ = pri; }
    uint32_t ವೆಚ್ಚ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ವೆಚ್ಚ; }
    ಶೂನ್ಯ ವೆಚ್ಚ(uint32_t ವೆಚ್ಚ) { m_ವೆಚ್ಚ = ವೆಚ್ಚ; }
    uint64_t predict_ಆರಂಭ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_predict_ಆರಂಭ; }
    ಶೂನ್ಯ predict_ಆರಂಭ(uint64_t ಸಮಯ_2) { m_predict_ಆರಂಭ = ಸಮಯ_2; }
    string name() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE { ಹಿಂತಿರುಗಿ "mt"s + std::ಗಮ್ಯ_string(id()); }
    string hashName() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_hashName; }
    ಶೂನ್ಯ ಎಳೆಗಳು(ಪೂರ್ಣಾಂಕ ಎಳೆಗಳು) { m_ಎಳೆಗಳು = ಎಳೆಗಳು; }
    ಪೂರ್ಣಾಂಕ ಎಳೆಗಳು() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಎಳೆಗಳು; }
    ಶೂನ್ಯ dump(std::ostream& str) ಸ್ಥಿರಾಂಕ;

    ಸ್ಥಿರ uint32_t ಅಂಕೆ_ಬಳಸಿದ_Ids() VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ s_ಮುಂದಿನ_Id; }
};

ಸಾಲೊಳಗೆ std::ostream& ಕ್ರಿಯಾಕಾರಕ<<(std::ostream& os, ಸ್ಥಿರಾಂಕ ExecMTask& rhs) {
    rhs.dump(os);
    ಹಿಂತಿರುಗಿ os;
}

namespace V3_Exec_ನಕ್ಷೆ {
ಶೂನ್ಯ implement(AstNetlist*) VL_MT_DISABLED;

ಶೂನ್ಯ selfTest() VL_MT_DISABLED;
}  //namespace V3ExecGraph

#ಆದರೆ_ಅಂತ್ಯ  // Guard
