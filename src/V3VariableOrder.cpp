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

استفاده MTaskIdVec = std::وکتور<منطقي>;  // Used as a bit-set indexed by MTask ID
// Writing MTasks of each variable accessed by any MTask
استفاده MTaskWritersMap = std::unordered_map<ثابت AstVar*, MTaskIdVec>;

// Trace through code reachable from an MTask and record the writers of referenced variables
صنف GatherMTaskWriters final : VNVisitorConst {
    // NODE STATE
    //  AstCFunc::user1()  // bool: Already traced this function
    //  AstVar::user1()  // bool: Already traced this variable
    ثابت VNUser1_درون_Use m_user1_درون_Use;

    // STATE
    MTaskWritersMap& m_results;  // The result map being built;
    ثابت uint32_t m_id;  // Id of mtask being analysed
    ثابت اندازه_t m_استفاده_شده_Ids = ExecMTask::رقم_استفاده_شده_Ids();  // Value of max id + 1

    // CONSTRUCTOR
    GatherMTaskWriters(ثابت ExecMTask* mTaskp, MTaskWritersMap& results)
        : m_results{results}
        , m_id{mTaskp->id()} {
        iterateConst(mTaskp->funcp());
    }
    ~GatherMTaskWriters() = افتراضي;
    VL_UNMOVABLE(GatherMTaskWriters);

    //بازدید
    فراغ بازدید(Ast_گره_Var_Ref* nodep) override {
        // Cheaper than relying on emplace().second
        إذا (nodep->user1_مجموعه_Once()) أرجع;
        AstVar* ثابت varp = nodep->varp();
        // Record the access, and the writer bit if written
        MTaskIdVec& writers = m_results
                                  .emplace(std::piecewise_construct,  //
                                           std::forward_as_tuple(varp),  //
                                           std::forward_as_tuple(m_استفاده_شده_Ids))
                                  .اولی->دومی;
        إذا (nodep->access().isWriteOrRW()) writers[m_id] = درست;
    }

    فراغ بازدید(AstCFunc* nodep) override {
        إذا (nodep->user1_مجموعه_Once()) أرجع;  // Prevent repeat traversals/recursion
        iterateChildrenConst(nodep);
    }

    فراغ بازدید(Ast_گره_CCall* nodep) override {
        iterateChildrenConst(nodep);  // Arguments
        iterateConst(nodep->funcp());  // Callee
    }

    فراغ بازدید(Ast_گره* nodep) override { iterateChildrenConst(nodep); }

عام:
    ساكن فراغ apply(ثابت ExecMTask* mTaskp, MTaskWritersMap& results) {
        GatherMTaskWriters{mTaskp, results};
    }
};

صنف Variable_ترتیب final {
    // NODE STATE
    //  AstVar::user1()   ->   uint64_t: Stratum, roughly equivalent to alignment
    //                                   requirement, to avoid padding
    ثابت VNUser1_درون_Use m_user1_درون_Use;

    ثابت MTaskWritersMap& m_mTaskWriters;

    Variable_ترتیب(Ast_گره_Module* modp, ثابت MTaskWritersMap& mTaskWriters)
        : m_mTaskWriters{mTaskWriters} {
        ترتیب_Module_Vars(modp);
    }
    ~Variable_ترتیب() = افتراضي;
    VL_UNCOPYABLE(Variable_ترتیب);

    //######################################################################

    // Simple sort by stratum
    فراغ simple_مرتب_سازی_Vars(std::وکتور<AstVar*>& varps) {
        stable_مرتب_سازی(varps.begin(), varps.انتها(), [](ثابت AstVar* ap, ثابت AstVar* bp) -> منطقي {
            أرجع ap->user1() < bp->user1();
        });
    }

    // Sort by writing MTasks first, then the same as simpleSortVars
    فراغ mtask_مرتب_سازی_Vars(std::وکتور<AstVar*>& varps) {
        // Map from "writing MTasks" -> "variable list", for variables accessed by MTasks
        std::map<MTaskIdVec, std::وکتور<AstVar*>> m2v;
        // Variables not accessed by any MTask
        std::وکتور<AstVar*> noAffinityVarps;
        لكل (AstVar* ثابت varp : varps) {
            ثابت خودکار it = m_mTaskWriters.پیدا_کن(varp);
            إذا (it == m_mTaskWriters.انتها()) {
                noAffinityVarps.افزودن_پشت(varp);
            } وإلا {
                m2v[it->دومی].افزودن_پشت(varp);
            }
        }

        varps.clear();

        // Helper function to sort given vector, then append to 'varps'
        ثابت خودکار مرتب_سازی_And_Append
            = [هذا, &varps](std::وکتور<AstVar*>& subVarps, منطقي align_اولی) {
                  simple_مرتب_سازی_Vars(subVarps);
                  منطقي aligned = !align_اولی;
                  لكل (AstVar* ثابت varp : subVarps) {
                      // Align the first variable with instance storage
                      إذا (!aligned && varp->isModelState()) {
                          varp->mtask_Cache_سطر_Align(درست);
                          V3Stats::جمع_کن_Stat_جمع("VariableOrder, MTask aligned group starts", 1);
                          aligned = درست;
                      }
                      varps.افزودن_پشت(varp);
                  }
              };

        // Add the groups in the map's deterministic key order
        لكل (خودکار& pair : m2v) مرتب_سازی_And_Append(pair.دومی, درست);

        // Finally add the variables with no known MTask affinity
        مرتب_سازی_And_Append(noAffinityVarps, خطأ);

        V3Stats::جمع_کن_Stat_جمع("VariableOrder, MTask affinity groups", m2v.اندازه());
        V3Stats::جمع_کن_Stat_جمع("VariableOrder, no-affinity variables", noAffinityVarps.اندازه());
    }

    // cppcheck-suppress constParameterPointer
    فراغ ترتیب_Module_Vars(Ast_گره_Module* modp) {
        // Top level ports stay first, in source order (--lib-create wrapper must match original)
        std::وکتور<AstVar*> portps;
        // Followed by trivially constructible variables
        std::وکتور<AstVar*> trivps;
        // Followed by not trivially constructible variables
        std::وکتور<AstVar*> nontps;
        // Followed by variables that are not model state, in source order
        std::وکتور<AstVar*> restps;

        // Gather variables, compute stratum
        لكل (Ast_گره *nodep = modp->stmtsp(), *nextp; nodep; nodep = nextp) {
            nextp = nodep->nextp();
            إذا (AstVar* ثابت varp = VN_CAST(nodep, Var)) {
                إذا (varp->isPrimaryIO()) {
                    portps.افزودن_پشت(varp);
                    استمر;
                }
                إذا (!varp->isModelState()) {
                    restps.افزودن_پشت(varp);
                    استمر;
                }

                إذا (varp->isTriviallyConstructible()) {
                    trivps.افزودن_پشت(varp);
                } وإلا {
                    nontps.افزودن_پشت(varp);
                }

                Ast_گره_DType* ثابت dtypep = varp->dtypeSkipRefp();
                ثابت صحيح sigbytes = dtypep->عرض_Align_بایتها();
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
        لكل (std::وکتور<AstVar*>* ثابت varpsp : {&trivps, &nontps}) {
            إذا (varpsp->خالی()) استمر;
            إذا (!v3Global.opt.mtasks()) {
                simple_مرتب_سازی_Vars(*varpsp);
            } وإلا {
                mtask_مرتب_سازی_Vars(*varpsp);
            }
        }

        // Insert them back under the module, in the new order, but at
        // the front of the list so they come out first in dumps/JSON.
        Ast_گره* nodesp = اشاره_تهی;
        لكل (ثابت std::وکتور<AstVar*>* ثابت varpsp : {&portps, &trivps, &nontps, &restps}) {
            لكل (AstVar* ثابت varp : *varpsp) {
                nodesp = Ast_گره::جمع_کن_بعدی_Null(nodesp, varp->unlink_Fr_پشت());
            }
        }
        modp->prependStmtsp(nodesp);
    }

عام:
    ساكن فراغ apply(Ast_گره_Module* modp, ثابت MTaskWritersMap& mTaskWriters) {
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
        netlistp->بالاترین_Modulep()->foreach([&](Ast_Exec_گراف* execGraphp) {
            لكل (ثابت V3_گراف_رأس& vtx : execGraphp->depGraphp()->vertices()) {
                GatherMTaskWriters::apply(vtx.as<ثابت ExecMTask>(), mTaskWriters);
            }
        });
    }

    // Sort variables of each module
    لكل (Ast_گره_Module* modp = v3Global.rootp()->modulesp(); modp;
         modp = VN_AS(modp->nextp(), گره_Module)) {
        إذا (modp->isConstPool()) استمر;
        Variable_ترتیب::apply(modp, mTaskWriters);
    }

    V3Global::dump_Check_Global_درخت("variableorder", 0, dump_درخت_Either_Level() >= 3);
}
