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

using MTaskIdVec = std::vector<منطقي>;  // Used as a bit-set indexed by MTask ID
// Writing MTasks of each variable accessed by any MTask
using MTaskWritersMap = std::unordered_map<ثابت AstVar*, MTaskIdVec>;

// Trace through code reachable from an MTask and record the writers of referenced variables
صنف GatherMTaskWriters final : VNVisitorConst {
    // NODE STATE
    //  AstCFunc::user1()  // bool: Already traced this function
    //  AstVar::user1()  // bool: Already traced this variable
    ثابت VNUser1InUse m_user1InUse;

    // STATE
    MTaskWritersMap& m_results;  // The result map being built;
    ثابت uint32_t m_id;  // Id of mtask being analysed
    ثابت size_t m_usedIds = ExecMTask::ژمارە_Used_Ids();  // Value of max id + 1

    // CONSTRUCTOR
    GatherMTaskWriters(ثابت ExecMTask* mTaskp, MTaskWritersMap& results)
        : m_results{results}
        , m_id{mTaskp->id()} {
        iterateConst(mTaskp->funcp());
    }
    ~GatherMTaskWriters() = افتراضي;
    VL_UNMOVABLE(GatherMTaskWriters);

    // VISIT
    فراغ visit(Ast_گرێ_Var_Ref* nodep) override {
        // Cheaper than relying on emplace().second
        إذا (nodep->user1SetOnce()) أرجع;
        AstVar* ثابت varp = nodep->varp();
        // Record the access, and the writer bit if written
        MTaskIdVec& writers = m_results
                                  .emplace(std::piecewise_construct,  //
                                           std::forward_as_tuple(varp),  //
                                           std::forward_as_tuple(m_usedIds))
                                  .first->second;
        إذا (nodep->access().isWriteOrRW()) writers[m_id] = true;
    }

    فراغ visit(AstCFunc* nodep) override {
        إذا (nodep->user1SetOnce()) أرجع;  // Prevent repeat traversals/recursion
        iterateChildrenConst(nodep);
    }

    فراغ visit(Ast_گرێ_CCall* nodep) override {
        iterateChildrenConst(nodep);  // Arguments
        iterateConst(nodep->funcp());  // Callee
    }

    فراغ visit(Ast_گرێ* nodep) override { iterateChildrenConst(nodep); }

عام:
    ساكن فراغ apply(ثابت ExecMTask* mTaskp, MTaskWritersMap& results) {
        GatherMTaskWriters{mTaskp, results};
    }
};

صنف VariableOrder final {
    // NODE STATE
    //  AstVar::user1()   ->   uint64_t: Stratum, roughly equivalent to alignment
    //                                   requirement, to avoid padding
    ثابت VNUser1InUse m_user1InUse;

    ثابت MTaskWritersMap& m_mTaskWriters;

    VariableOrder(Ast_گرێ_Module* modp, ثابت MTaskWritersMap& mTaskWriters)
        : m_mTaskWriters{mTaskWriters} {
        orderModuleVars(modp);
    }
    ~VariableOrder() = افتراضي;
    VL_UNCOPYABLE(VariableOrder);

    //######################################################################

    // Simple sort by stratum
    فراغ simpleSortVars(std::vector<AstVar*>& varps) {
        stable_sort(varps.begin(), varps.end(), [](ثابت AstVar* ap, ثابت AstVar* bp) -> منطقي {
            أرجع ap->user1() < bp->user1();
        });
    }

    // Sort by writing MTasks first, then the same as simpleSortVars
    فراغ mtaskSortVars(std::vector<AstVar*>& varps) {
        // Map from "writing MTasks" -> "variable list", for variables accessed by MTasks
        std::map<MTaskIdVec, std::vector<AstVar*>> m2v;
        // Variables not accessed by any MTask
        std::vector<AstVar*> noAffinityVarps;
        لكل (AstVar* ثابت varp : varps) {
            ثابت auto it = m_mTaskWriters.find(varp);
            إذا (it == m_mTaskWriters.end()) {
                noAffinityVarps.پاڵنان_back(varp);
            } وإلا {
                m2v[it->second].پاڵنان_back(varp);
            }
        }

        varps.clear();

        // Helper function to sort given vector, then append to 'varps'
        ثابت auto sortAndAppend
            = [هذا, &varps](std::vector<AstVar*>& subVarps, منطقي alignFirst) {
                  simpleSortVars(subVarps);
                  منطقي aligned = !alignFirst;
                  لكل (AstVar* ثابت varp : subVarps) {
                      // Align the first variable with instance storage
                      إذا (!aligned && varp->isModelState()) {
                          varp->mtask_Cache_هێڵ_Align(true);
                          V3Stats::addStatSum("VariableOrder, MTask aligned group starts", 1);
                          aligned = true;
                      }
                      varps.پاڵنان_back(varp);
                  }
              };

        // Add the groups in the map's deterministic key order
        لكل (auto& pair : m2v) sortAndAppend(pair.second, true);

        // Finally add the variables with no known MTask affinity
        sortAndAppend(noAffinityVarps, خطأ);

        V3Stats::addStatSum("VariableOrder, MTask affinity groups", m2v.size());
        V3Stats::addStatSum("VariableOrder, no-affinity variables", noAffinityVarps.size());
    }

    // cppcheck-suppress constParameterPointer
    فراغ orderModuleVars(Ast_گرێ_Module* modp) {
        // Top level ports stay first, in source order (--lib-create wrapper must match original)
        std::vector<AstVar*> portps;
        // Followed by trivially constructible variables
        std::vector<AstVar*> trivps;
        // Followed by not trivially constructible variables
        std::vector<AstVar*> nontps;
        // Followed by variables that are not model state, in source order
        std::vector<AstVar*> restps;

        // Gather variables, compute stratum
        لكل (Ast_گرێ *nodep = modp->stmtsp(), *nextp; nodep; nodep = nextp) {
            nextp = nodep->nextp();
            إذا (AstVar* ثابت varp = VN_CAST(nodep, Var)) {
                إذا (varp->isPrimaryIO()) {
                    portps.پاڵنان_back(varp);
                    استمر;
                }
                إذا (!varp->isModelState()) {
                    restps.پاڵنان_back(varp);
                    استمر;
                }

                إذا (varp->isTriviallyConstructible()) {
                    trivps.پاڵنان_back(varp);
                } وإلا {
                    nontps.پاڵنان_back(varp);
                }

                Ast_گرێ_DType* ثابت dtypep = varp->dtypeSkipRefp();
                ثابت صحيح sigbytes = dtypep->widthAlignBytes();
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
        لكل (std::vector<AstVar*>* ثابت varpsp : {&trivps, &nontps}) {
            إذا (varpsp->empty()) استمر;
            إذا (!v3Global.opt.mtasks()) {
                simpleSortVars(*varpsp);
            } وإلا {
                mtaskSortVars(*varpsp);
            }
        }

        // Insert them back under the module, in the new order, but at
        // the front of the list so they come out first in dumps/JSON.
        Ast_گرێ* nodesp = nullptr;
        لكل (ثابت std::vector<AstVar*>* ثابت varpsp : {&portps, &trivps, &nontps, &restps}) {
            لكل (AstVar* ثابت varp : *varpsp) {
                nodesp = Ast_گرێ::addNextNull(nodesp, varp->unlinkFrBack());
            }
        }
        modp->prependStmtsp(nodesp);
    }

عام:
    ساكن فراغ apply(Ast_گرێ_Module* modp, ثابت MTaskWritersMap& mTaskWriters) {
        VariableOrder{modp, mTaskWriters};
    }
};

//######################################################################
// V3VariableOrder static functions

فراغ V3VariableOrder::orderAll(AstNetlist* netlistp) {
    UINFO(2, __FUNCTION__ << ":");

    MTaskWritersMap mTaskWriters;

    // Gather writing MTasks
    إذا (v3Global.opt.mtasks()) {
        netlistp->لووتکە_Modulep()->foreach([&](AstExecGraph* execGraphp) {
            لكل (ثابت V3GraphVertex& vtx : execGraphp->depGraphp()->vertices()) {
                GatherMTaskWriters::apply(vtx.as<ثابت ExecMTask>(), mTaskWriters);
            }
        });
    }

    // Sort variables of each module
    لكل (Ast_گرێ_Module* modp = v3Global.rootp()->modulesp(); modp;
         modp = VN_AS(modp->nextp(), گرێ_Module)) {
        إذا (modp->isConstPool()) استمر;
        VariableOrder::apply(modp, mTaskWriters);
    }

    V3Global::dump_Check_Global_درەخت("variableorder", 0, dump_درەخت_Either_Level() >= 3);
}
