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

#ಅನ್ತರ್ಭಾವ "V3PchAstMT.h"

#ಅನ್ತರ್ಭಾವ "V3VariableOrder.h"

#ಅನ್ತರ್ಭಾವ "V3AstUserAllocator.h"
#ಅನ್ತರ್ಭಾವ "V3ExecGraph.h"
#ಅನ್ತರ್ಭಾವ "V3Stats.h"

#ಅನ್ತರ್ಭಾವ <algorithm>
#ಅನ್ತರ್ಭಾವ <cstdint>
#ಅನ್ತರ್ಭಾವ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

using MTaskIdVec = std::vector<ತಾರ್ಕಿಕ>;  // Used as a bit-set indexed by MTask ID
// Writing MTasks of each variable accessed by any MTask
using MTaskWritersMap = std::unordered_map<ಧ್ರುವ AstVar*, MTaskIdVec>;

// Trace through code reachable from an MTask and record the writers of referenced variables
ವರ್ಗ GatherMTaskWriters final : VNVisitorConst {
    // NODE STATE
    //  AstCFunc::user1()  // bool: Already traced this function
    //  AstVar::user1()  // bool: Already traced this variable
    ಧ್ರುವ VNUser1InUse m_user1InUse;

    // STATE
    MTaskWritersMap& m_results;  // The result map being built;
    ಧ್ರುವ uint32_t m_id;  // Id of mtask being analysed
    ಧ್ರುವ size_t m_usedIds = ExecMTask::ಅಂಕೆ_Used_Ids();  // Value of max id + 1

    // CONSTRUCTOR
    GatherMTaskWriters(ಧ್ರುವ ExecMTask* mTaskp, MTaskWritersMap& results)
        : m_results{results}
        , m_id{mTaskp->id()} {
        iterateConst(mTaskp->funcp());
    }
    ~GatherMTaskWriters() = ಸ್ವಭಾವ;
    VL_UNMOVABLE(GatherMTaskWriters);

    // VISIT
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Var_Ref* nodep) override {
        // Cheaper than relying on emplace().second
        ಯದಿ (nodep->user1SetOnce()) ಪ್ರತ್ಯಾಗಮ;
        AstVar* ಧ್ರುವ varp = nodep->varp();
        // Record the access, and the writer bit if written
        MTaskIdVec& writers = m_results
                                  .emplace(std::piecewise_construct,  //
                                           std::forward_as_tuple(varp),  //
                                           std::forward_as_tuple(m_usedIds))
                                  .first->second;
        ಯದಿ (nodep->access().isWriteOrRW()) writers[m_id] = ಸತ್ಯ;
    }

    ಶೂನ್ಯ visit(AstCFunc* nodep) override {
        ಯದಿ (nodep->user1SetOnce()) ಪ್ರತ್ಯಾಗಮ;  // Prevent repeat traversals/recursion
        iterateChildrenConst(nodep);
    }

    ಶೂನ್ಯ visit(Ast_ಗಂಟು_CCall* nodep) override {
        iterateChildrenConst(nodep);  // Arguments
        iterateConst(nodep->funcp());  // Callee
    }

    ಶೂನ್ಯ visit(Ast_ಗಂಟು* nodep) override { iterateChildrenConst(nodep); }

ಸಾರ್ವಜನಿಕ:
    ಸ್ಥಿರ ಶೂನ್ಯ apply(ಧ್ರುವ ExecMTask* mTaskp, MTaskWritersMap& results) {
        GatherMTaskWriters{mTaskp, results};
    }
};

ವರ್ಗ VariableOrder final {
    // NODE STATE
    //  AstVar::user1()   ->   uint64_t: Stratum, roughly equivalent to alignment
    //                                   requirement, to avoid padding
    ಧ್ರುವ VNUser1InUse m_user1InUse;

    ಧ್ರುವ MTaskWritersMap& m_mTaskWriters;

    VariableOrder(Ast_ಗಂಟು_Module* modp, ಧ್ರುವ MTaskWritersMap& mTaskWriters)
        : m_mTaskWriters{mTaskWriters} {
        orderModuleVars(modp);
    }
    ~VariableOrder() = ಸ್ವಭಾವ;
    VL_UNCOPYABLE(VariableOrder);

    //######################################################################

    // Simple sort by stratum
    ಶೂನ್ಯ simpleSortVars(std::vector<AstVar*>& varps) {
        stable_sort(varps.begin(), varps.end(), [](ಧ್ರುವ AstVar* ap, ಧ್ರುವ AstVar* bp) -> ತಾರ್ಕಿಕ {
            ಪ್ರತ್ಯಾಗಮ ap->user1() < bp->user1();
        });
    }

    // Sort by writing MTasks first, then the same as simpleSortVars
    ಶೂನ್ಯ mtaskSortVars(std::vector<AstVar*>& varps) {
        // Map from "writing MTasks" -> "variable list", for variables accessed by MTasks
        std::map<MTaskIdVec, std::vector<AstVar*>> m2v;
        // Variables not accessed by any MTask
        std::vector<AstVar*> noAffinityVarps;
        ಪ್ರತಿ (AstVar* ಧ್ರುವ varp : varps) {
            ಧ್ರುವ auto it = m_mTaskWriters.find(varp);
            ಯದಿ (it == m_mTaskWriters.end()) {
                noAffinityVarps.ತಳ್ಳು_back(varp);
            } ಅನ್ಯಥಾ {
                m2v[it->second].ತಳ್ಳು_back(varp);
            }
        }

        varps.clear();

        // Helper function to sort given vector, then append to 'varps'
        ಧ್ರುವ auto sortAndAppend
            = [ಏತತ್, &varps](std::vector<AstVar*>& subVarps, ತಾರ್ಕಿಕ alignFirst) {
                  simpleSortVars(subVarps);
                  ತಾರ್ಕಿಕ aligned = !alignFirst;
                  ಪ್ರತಿ (AstVar* ಧ್ರುವ varp : subVarps) {
                      // Align the first variable with instance storage
                      ಯದಿ (!aligned && varp->isModelState()) {
                          varp->mtask_Cache_ಸಾಲು_Align(ಸತ್ಯ);
                          V3Stats::addStatSum("VariableOrder, MTask aligned group starts", 1);
                          aligned = ಸತ್ಯ;
                      }
                      varps.ತಳ್ಳು_back(varp);
                  }
              };

        // Add the groups in the map's deterministic key order
        ಪ್ರತಿ (auto& pair : m2v) sortAndAppend(pair.second, ಸತ್ಯ);

        // Finally add the variables with no known MTask affinity
        sortAndAppend(noAffinityVarps, ಅಸತ್ಯ);

        V3Stats::addStatSum("VariableOrder, MTask affinity groups", m2v.size());
        V3Stats::addStatSum("VariableOrder, no-affinity variables", noAffinityVarps.size());
    }

    // cppcheck-suppress constParameterPointer
    ಶೂನ್ಯ orderModuleVars(Ast_ಗಂಟು_Module* modp) {
        // Top level ports stay first, in source order (--lib-create wrapper must match original)
        std::vector<AstVar*> portps;
        // Followed by trivially constructible variables
        std::vector<AstVar*> trivps;
        // Followed by not trivially constructible variables
        std::vector<AstVar*> nontps;
        // Followed by variables that are not model state, in source order
        std::vector<AstVar*> restps;

        // Gather variables, compute stratum
        ಪ್ರತಿ (Ast_ಗಂಟು *nodep = modp->stmtsp(), *nextp; nodep; nodep = nextp) {
            nextp = nodep->nextp();
            ಯದಿ (AstVar* ಧ್ರುವ varp = VN_CAST(nodep, Var)) {
                ಯದಿ (varp->isPrimaryIO()) {
                    portps.ತಳ್ಳು_back(varp);
                    ಅನುವರ್ತ;
                }
                ಯದಿ (!varp->isModelState()) {
                    restps.ತಳ್ಳು_back(varp);
                    ಅನುವರ್ತ;
                }

                ಯದಿ (varp->isTriviallyConstructible()) {
                    trivps.ತಳ್ಳು_back(varp);
                } ಅನ್ಯಥಾ {
                    nontps.ತಳ್ಳು_back(varp);
                }

                Ast_ಗಂಟು_DType* ಧ್ರುವ dtypep = varp->dtypeSkipRefp();
                ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ sigbytes = dtypep->widthAlignBytes();
                ಧ್ರುವ uint64_t stratum = VN_IS(dtypep, UnpackArrayDType)                  ? 9
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
        ಪ್ರತಿ (std::vector<AstVar*>* ಧ್ರುವ varpsp : {&trivps, &nontps}) {
            ಯದಿ (varpsp->empty()) ಅನುವರ್ತ;
            ಯದಿ (!v3Global.opt.mtasks()) {
                simpleSortVars(*varpsp);
            } ಅನ್ಯಥಾ {
                mtaskSortVars(*varpsp);
            }
        }

        // Insert them back under the module, in the new order, but at
        // the front of the list so they come out first in dumps/JSON.
        Ast_ಗಂಟು* nodesp = nullptr;
        ಪ್ರತಿ (ಧ್ರುವ std::vector<AstVar*>* ಧ್ರುವ varpsp : {&portps, &trivps, &nontps, &restps}) {
            ಪ್ರತಿ (AstVar* ಧ್ರುವ varp : *varpsp) {
                nodesp = Ast_ಗಂಟು::addNextNull(nodesp, varp->unlinkFrBack());
            }
        }
        modp->prependStmtsp(nodesp);
    }

ಸಾರ್ವಜನಿಕ:
    ಸ್ಥಿರ ಶೂನ್ಯ apply(Ast_ಗಂಟು_Module* modp, ಧ್ರುವ MTaskWritersMap& mTaskWriters) {
        VariableOrder{modp, mTaskWriters};
    }
};

//######################################################################
// V3VariableOrder static functions

ಶೂನ್ಯ V3VariableOrder::orderAll(AstNetlist* netlistp) {
    UINFO(2, __FUNCTION__ << ":");

    MTaskWritersMap mTaskWriters;

    // Gather writing MTasks
    ಯದಿ (v3Global.opt.mtasks()) {
        netlistp->ತುದಿ_Modulep()->foreach([&](AstExecGraph* execGraphp) {
            ಪ್ರತಿ (ಧ್ರುವ V3GraphVertex& vtx : execGraphp->depGraphp()->vertices()) {
                GatherMTaskWriters::apply(vtx.as<ಧ್ರುವ ExecMTask>(), mTaskWriters);
            }
        });
    }

    // Sort variables of each module
    ಪ್ರತಿ (Ast_ಗಂಟು_Module* modp = v3Global.rootp()->modulesp(); modp;
         modp = VN_AS(modp->nextp(), ಗಂಟು_Module)) {
        ಯದಿ (modp->isConstPool()) ಅನುವರ್ತ;
        VariableOrder::apply(modp, mTaskWriters);
    }

    V3Global::dump_Check_Global_ವೃಕ್ಷ("variableorder", 0, dump_ವೃಕ್ಷ_Either_Level() >= 3);
}
