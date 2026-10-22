// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Variable ordering
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
// V3VariableOrder's Transformations:
//
// Each module:
//   Order module variables
//
// With multithreading, variables accessed by MTasks are grouped by the exact
// set of MTasks writing them (variables MTasks only read form one group), and
// the first instance field of each group is aligned to a cache line. Variables
// no MTask accesses follow, unaligned.
//
// Separating writers is enough to avoid false sharing. Every MTask accessing a
// variable is ordered relative to the variable's writers by the MTask graph, so
// a cache line holding one group is never written while another thread is using
// it, however the MTasks are packed onto threads. Grouping by readers as well
// would only add padding.
//
//*************************************************************************

#ضمن "V3PchAstMT.h"

#ضمن "V3VariableOrder.h"

#ضمن "V3AstUserAllocator.h"
#ضمن "V3ExecGraph.h"
#ضمن "V3Stats.h"

#ضمن <algorithm>
#ضمن <cstdint>
#ضمن <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

کارول MTaskIdVec = std::ویکټور<منطقي>;  // Used as a bit-set indexed by MTask ID
// Writing MTasks of each variable accessed by any MTask
کارول MTaskWritersMap = std::unordered_map<ثابت AstVar*, MTaskIdVec>;

// Trace through code reachable from an MTask and record the writers of referenced variables
صنف GatherMTaskWriters final : VNVisitorConst {
    // NODE STATE
    //  AstCFunc::user1()  // bool: Already traced this function
    //  AstVar::user1()  // bool: Already traced this variable
    ثابت VNUser1_دننه_Use m_user1_دننه_Use;

    // STATE
    MTaskWritersMap& m_results;  // The result map being built;
    ثابت uint32_t m_id;  // Id of mtask being analysed
    ثابت اندازه_t m_کارول_شوی_Ids = ExecMTask::عدد_کارول_شوی_Ids();  // Value of max id + 1

    // CONSTRUCTOR
    GatherMTaskWriters(ثابت ExecMTask* mTaskp, MTaskWritersMap& results)
        : m_results{results}
        , m_id{mTaskp->id()} {
        iterateConst(mTaskp->funcp());
    }
    ~GatherMTaskWriters() = افتراضي;
    VL_UNMOVABLE(GatherMTaskWriters);

    //لیدنه
    فراغ لیدنه(Ast_غوټه_Var_Ref* nodep) override {
        // Cheaper than relying on emplace().second
        إذا (nodep->user1_سېټ_Once()) أرجع;
        AstVar* ثابت varp = nodep->varp();
        // Record the access, and the writer bit if written
        MTaskIdVec& writers = m_results
                                  .emplace(std::piecewise_construct,  //
                                           std::forward_as_tuple(varp),  //
                                           std::forward_as_tuple(m_کارول_شوی_Ids))
                                  .لومړی->دوهم;
        إذا (nodep->access().isWriteOrRW()) writers[m_id] = ریښتیا;
    }

    فراغ لیدنه(AstCFunc* nodep) override {
        إذا (nodep->user1_سېټ_Once()) أرجع;  // Prevent repeat traversals/recursion
        iterateChildrenConst(nodep);
    }

    فراغ لیدنه(Ast_غوټه_CCall* nodep) override {
        iterateChildrenConst(nodep);  // Arguments
        iterateConst(nodep->funcp());  // Callee
    }

    فراغ لیدنه(Ast_غوټه* nodep) override { iterateChildrenConst(nodep); }

عام:
    ساكن فراغ apply(ثابت ExecMTask* mTaskp, MTaskWritersMap& results) {
        GatherMTaskWriters{mTaskp, results};
    }
};

صنف Variable_ترتیب final {
    // NODE STATE
    //  AstVar::user1()   ->   uint64_t: Stratum, roughly equivalent to alignment
    //                                   requirement, to avoid padding
    ثابت VNUser1_دننه_Use m_user1_دننه_Use;

    ثابت MTaskWritersMap& m_mTaskWriters;

    Variable_ترتیب(Ast_غوټه_Module* modp, ثابت MTaskWritersMap& mTaskWriters)
        : m_mTaskWriters{mTaskWriters} {
        ترتیب_Module_Vars(modp);
    }
    ~Variable_ترتیب() = افتراضي;
    VL_UNCOPYABLE(Variable_ترتیب);

    //######################################################################

    // Simple sort by stratum
    فراغ simple_ترتیبول_Vars(std::ویکټور<AstVar*>& varps) {
        stable_ترتیبول(varps.begin(), varps.پای_ټکی(), [](ثابت AstVar* ap, ثابت AstVar* bp) -> منطقي {
            أرجع ap->user1() < bp->user1();
        });
    }

    // Sort by writing MTasks first, then the same as simpleSortVars
    فراغ mtask_ترتیبول_Vars(std::ویکټور<AstVar*>& varps) {
        // Map from "writing MTasks" -> "variable list", for variables accessed by MTasks
        std::map<MTaskIdVec, std::ویکټور<AstVar*>> m2v;
        // Variables not accessed by any MTask
        std::ویکټور<AstVar*> noAffinityVarps;
        لكل (AstVar* ثابت varp : varps) {
            ثابت خپلکاری it = m_mTaskWriters.پیدا_کړه(varp);
            إذا (it == m_mTaskWriters.پای_ټکی()) {
                noAffinityVarps.ټېل_شا(varp);
            } وإلا {
                m2v[it->دوهم].ټېل_شا(varp);
            }
        }

        varps.clear();

        // Helper function to sort given vector, then append to 'varps'
        ثابت خپلکاری ترتیبول_And_Append
            = [هذا, &varps](std::ویکټور<AstVar*>& subVarps, منطقي align_لومړی) {
                  simple_ترتیبول_Vars(subVarps);
                  منطقي aligned = !align_لومړی;
                  لكل (AstVar* ثابت varp : subVarps) {
                      // Align the first variable with instance storage
                      إذا (!aligned && varp->isModelState()) {
                          varp->mtask_Cache_کرښه_Align(ریښتیا);
                          V3Stats::جمع_کړه_Stat_مجموعه("VariableOrder, MTask aligned group starts", 1);
                          aligned = ریښتیا;
                      }
                      varps.ټېل_شا(varp);
                  }
              };

        // Add the groups in the map's deterministic key order
        لكل (خپلکاری& pair : m2v) ترتیبول_And_Append(pair.دوهم, ریښتیا);

        // Finally add the variables with no known MTask affinity
        ترتیبول_And_Append(noAffinityVarps, خطأ);

        V3Stats::جمع_کړه_Stat_مجموعه("VariableOrder, MTask affinity groups", m2v.اندازه());
        V3Stats::جمع_کړه_Stat_مجموعه("VariableOrder, no-affinity variables", noAffinityVarps.اندازه());
    }

    // cppcheck-suppress constParameterPointer
    فراغ ترتیب_Module_Vars(Ast_غوټه_Module* modp) {
        // Top level ports stay first, in source order (--lib-create wrapper must match original)
        std::ویکټور<AstVar*> portps;
        // Followed by trivially constructible variables
        std::ویکټور<AstVar*> trivps;
        // Followed by not trivially constructible variables
        std::ویکټور<AstVar*> nontps;
        // Followed by variables that are not model state, in source order
        std::ویکټور<AstVar*> restps;

        // Gather variables, compute stratum
        لكل (Ast_غوټه *nodep = modp->stmtsp(), *nextp; nodep; nodep = nextp) {
            nextp = nodep->nextp();
            إذا (AstVar* ثابت varp = VN_CAST(nodep, Var)) {
                إذا (varp->isPrimaryIO()) {
                    portps.ټېل_شا(varp);
                    استمر;
                }
                إذا (!varp->isModelState()) {
                    restps.ټېل_شا(varp);
                    استمر;
                }

                إذا (varp->isTriviallyConstructible()) {
                    trivps.ټېل_شا(varp);
                } وإلا {
                    nontps.ټېل_شا(varp);
                }

                Ast_غوټه_DType* ثابت dtypep = varp->dtypeSkipRefp();
                ثابت صحيح sigbytes = dtypep->پلنوالی_Align_بایټونه();
                ثابت uint64_t stratum = VN_IS(dtypep, UnpackArrayDType)                  ? 9
                                         : (varp->basicp() && varp->basicp()->isString()) ? 4
                                         : (varp->basicp() && varp->basicp()->isOpaque()) ? 8
                                         : (sigbytes == 8)                                ? 3
                                         : (sigbytes == 4)                                ? 2
                                         : (sigbytes == 2)                                ? 1
                                         : (sigbytes == 1)                                ? 0
                                                                                          : 10;
                varp->user1(stratum);
            }
        }

        // Sort them
        لكل (std::ویکټور<AstVar*>* ثابت varpsp : {&trivps, &nontps}) {
            إذا (varpsp->تشه()) استمر;
            إذا (!v3Global.opt.mtasks()) {
                simple_ترتیبول_Vars(*varpsp);
            } وإلا {
                mtask_ترتیبول_Vars(*varpsp);
            }
        }

        // Insert them back under the module, in the new order, but at
        // the front of the list so they come out first in dumps/JSON.
        Ast_غوټه* nodesp = تش_اشاره;
        لكل (ثابت std::ویکټور<AstVar*>* ثابت varpsp : {&portps, &trivps, &nontps, &restps}) {
            لكل (AstVar* ثابت varp : *varpsp) {
                nodesp = Ast_غوټه::جمع_کړه_بل_Null(nodesp, varp->unlink_Fr_شا());
            }
        }
        modp->prependStmtsp(nodesp);
    }

عام:
    ساكن فراغ apply(Ast_غوټه_Module* modp, ثابت MTaskWritersMap& mTaskWriters) {
        Variable_ترتیب{modp, mTaskWriters};
    }
};

//######################################################################
// V3VariableOrder static functions

فراغ V3_Variable_ترتیب::ترتیب_All(AstNetlist* netlistp) {
    UINFO(2, __FUNCTION__ << ":");

    MTaskWritersMap mTaskWriters;

    // Gather writing MTasks
    إذا (v3Global.opt.mtasks()) {
        netlistp->سر_Modulep()->foreach([&](Ast_Exec_ګراف* execGraphp) {
            لكل (ثابت V3_ګراف_راس& vtx : execGraphp->depGraphp()->vertices()) {
                GatherMTaskWriters::apply(vtx.as<ثابت ExecMTask>(), mTaskWriters);
            }
        });
    }

    // Sort variables of each module
    لكل (Ast_غوټه_Module* modp = v3Global.rootp()->modulesp(); modp;
         modp = VN_AS(modp->nextp(), غوټه_Module)) {
        إذا (modp->isConstPool()) استمر;
        Variable_ترتیب::apply(modp, mTaskWriters);
    }

    V3Global::dump_Check_Global_ونه("variableorder", 0, dump_ونه_Either_Level() >= 3);
}
