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

ಬಳಸಿ MTaskIdVec = std::ಸದಿಶ<ತಾರ್ಕಿಕ>;  // Used as a bit-set indexed by MTask ID
// Writing MTasks of each variable accessed by any MTask
ಬಳಸಿ MTaskWritersMap = std::unordered_map<ಸ್ಥಿರಾಂಕ AstVar*, MTaskIdVec>;

// Trace through code reachable from an MTask and record the writers of referenced variables
ವರ್ಗ GatherMTaskWriters final : VNVisitorConst {
    // NODE STATE
    //  AstCFunc::user1()  // bool: Already traced this function
    //  AstVar::user1()  // bool: Already traced this variable
    ಸ್ಥಿರಾಂಕ VNUser1_ಒಳ_Use m_user1_ಒಳ_Use;

    // STATE
    MTaskWritersMap& m_results;  // The result map being built;
    ಸ್ಥಿರಾಂಕ uint32_t m_id;  // Id of mtask being analysed
    ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t m_ಬಳಸಿದ_Ids = ExecMTask::ಅಂಕೆ_ಬಳಸಿದ_Ids();  // Value of max id + 1

    // CONSTRUCTOR
    GatherMTaskWriters(ಸ್ಥಿರಾಂಕ ExecMTask* mTaskp, MTaskWritersMap& results)
        : m_results{results}
        , m_id{mTaskp->id()} {
        iterateConst(mTaskp->funcp());
    }
    ~GatherMTaskWriters() = ಪೂರ್ವನಿಯೋಜಿತ;
    VL_UNMOVABLE(GatherMTaskWriters);

    //ಭೇಟಿ
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Var_Ref* nodep) override {
        // Cheaper than relying on emplace().second
        ಆದರೆ (nodep->user1_ಗಣ_Once()) ಹಿಂತಿರುಗಿ;
        AstVar* ಸ್ಥಿರಾಂಕ varp = nodep->varp();
        // Record the access, and the writer bit if written
        MTaskIdVec& writers = m_results
                                  .emplace(std::piecewise_construct,  //
                                           std::forward_as_tuple(varp),  //
                                           std::forward_as_tuple(m_ಬಳಸಿದ_Ids))
                                  .ಮೊದಲ->ಎರಡನೇ;
        ಆದರೆ (nodep->access().isWriteOrRW()) writers[m_id] = ನಿಜ;
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstCFunc* nodep) override {
        ಆದರೆ (nodep->user1_ಗಣ_Once()) ಹಿಂತಿರುಗಿ;  // Prevent repeat traversals/recursion
        iterateChildrenConst(nodep);
    }

    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_CCall* nodep) override {
        iterateChildrenConst(nodep);  // Arguments
        iterateConst(nodep->funcp());  // Callee
    }

    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override { iterateChildrenConst(nodep); }

ಸಾರ್ವಜನಿಕ:
    ಸ್ಥಿರ ಶೂನ್ಯ apply(ಸ್ಥಿರಾಂಕ ExecMTask* mTaskp, MTaskWritersMap& results) {
        GatherMTaskWriters{mTaskp, results};
    }
};

ವರ್ಗ Variable_ಕ್ರಮ final {
    // NODE STATE
    //  AstVar::user1()   ->   uint64_t: Stratum, roughly equivalent to alignment
    //                                   requirement, to avoid padding
    ಸ್ಥಿರಾಂಕ VNUser1_ಒಳ_Use m_user1_ಒಳ_Use;

    ಸ್ಥಿರಾಂಕ MTaskWritersMap& m_mTaskWriters;

    Variable_ಕ್ರಮ(Ast_ಗಂಟು_Module* modp, ಸ್ಥಿರಾಂಕ MTaskWritersMap& mTaskWriters)
        : m_mTaskWriters{mTaskWriters} {
        ಕ್ರಮ_Module_Vars(modp);
    }
    ~Variable_ಕ್ರಮ() = ಪೂರ್ವನಿಯೋಜಿತ;
    VL_UNCOPYABLE(Variable_ಕ್ರಮ);

    //######################################################################

    // Simple sort by stratum
    ಶೂನ್ಯ simple_ವಿಂಗಡಿಸು_Vars(std::ಸದಿಶ<AstVar*>& varps) {
        stable_ವಿಂಗಡಿಸು(varps.begin(), varps.ಕೊನೆ(), [](ಸ್ಥಿರಾಂಕ AstVar* ap, ಸ್ಥಿರಾಂಕ AstVar* bp) -> ತಾರ್ಕಿಕ {
            ಹಿಂತಿರುಗಿ ap->user1() < bp->user1();
        });
    }

    // Sort by writing MTasks first, then the same as simpleSortVars
    ಶೂನ್ಯ mtask_ವಿಂಗಡಿಸು_Vars(std::ಸದಿಶ<AstVar*>& varps) {
        // Map from "writing MTasks" -> "variable list", for variables accessed by MTasks
        std::map<MTaskIdVec, std::ಸದಿಶ<AstVar*>> m2v;
        // Variables not accessed by any MTask
        std::ಸದಿಶ<AstVar*> noAffinityVarps;
        ಗಾಗಿ (AstVar* ಸ್ಥಿರಾಂಕ varp : varps) {
            ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ it = m_mTaskWriters.ಕಂಡುಹಿಡಿ(varp);
            ಆದರೆ (it == m_mTaskWriters.ಕೊನೆ()) {
                noAffinityVarps.ತಳ್ಳು_ಹಿಂಭಾಗ(varp);
            } ಇಲ್ಲದಿದ್ದರೆ {
                m2v[it->ಎರಡನೇ].ತಳ್ಳು_ಹಿಂಭಾಗ(varp);
            }
        }

        varps.clear();

        // Helper function to sort given vector, then append to 'varps'
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ ವಿಂಗಡಿಸು_And_Append
            = [ಇದು, &varps](std::ಸದಿಶ<AstVar*>& subVarps, ತಾರ್ಕಿಕ align_ಮೊದಲ) {
                  simple_ವಿಂಗಡಿಸು_Vars(subVarps);
                  ತಾರ್ಕಿಕ aligned = !align_ಮೊದಲ;
                  ಗಾಗಿ (AstVar* ಸ್ಥಿರಾಂಕ varp : subVarps) {
                      // Align the first variable with instance storage
                      ಆದರೆ (!aligned && varp->isModelState()) {
                          varp->mtask_Cache_ಸಾಲು_Align(ನಿಜ);
                          V3Stats::ಕೂಡಿಸು_Stat_ಮೊತ್ತ("VariableOrder, MTask aligned group starts", 1);
                          aligned = ನಿಜ;
                      }
                      varps.ತಳ್ಳು_ಹಿಂಭಾಗ(varp);
                  }
              };

        // Add the groups in the map's deterministic key order
        ಗಾಗಿ (ಸ್ವಯಂಚಾಲಿತ& pair : m2v) ವಿಂಗಡಿಸು_And_Append(pair.ಎರಡನೇ, ನಿಜ);

        // Finally add the variables with no known MTask affinity
        ವಿಂಗಡಿಸು_And_Append(noAffinityVarps, ಸುಳ್ಳು);

        V3Stats::ಕೂಡಿಸು_Stat_ಮೊತ್ತ("VariableOrder, MTask affinity groups", m2v.ಗಾತ್ರ_2());
        V3Stats::ಕೂಡಿಸು_Stat_ಮೊತ್ತ("VariableOrder, no-affinity variables", noAffinityVarps.ಗಾತ್ರ_2());
    }

    // cppcheck-suppress constParameterPointer
    ಶೂನ್ಯ ಕ್ರಮ_Module_Vars(Ast_ಗಂಟು_Module* modp) {
        // Top level ports stay first, in source order (--lib-create wrapper must match original)
        std::ಸದಿಶ<AstVar*> portps;
        // Followed by trivially constructible variables
        std::ಸದಿಶ<AstVar*> trivps;
        // Followed by not trivially constructible variables
        std::ಸದಿಶ<AstVar*> nontps;
        // Followed by variables that are not model state, in source order
        std::ಸದಿಶ<AstVar*> restps;

        // Gather variables, compute stratum
        ಗಾಗಿ (Ast_ಗಂಟು *nodep = modp->stmtsp(), *nextp; nodep; nodep = nextp) {
            nextp = nodep->nextp();
            ಆದರೆ (AstVar* ಸ್ಥಿರಾಂಕ varp = VN_CAST(nodep, Var)) {
                ಆದರೆ (varp->isPrimaryIO()) {
                    portps.ತಳ್ಳು_ಹಿಂಭಾಗ(varp);
                    ಮುಂದುವರಿ;
                }
                ಆದರೆ (!varp->isModelState()) {
                    restps.ತಳ್ಳು_ಹಿಂಭಾಗ(varp);
                    ಮುಂದುವರಿ;
                }

                ಆದರೆ (varp->isTriviallyConstructible()) {
                    trivps.ತಳ್ಳು_ಹಿಂಭಾಗ(varp);
                } ಇಲ್ಲದಿದ್ದರೆ {
                    nontps.ತಳ್ಳು_ಹಿಂಭಾಗ(varp);
                }

                Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ dtypep = varp->dtypeSkipRefp();
                ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ sigbytes = dtypep->ಅಗಲ_Align_ಬೈಟುಗಳು();
                ಸ್ಥಿರಾಂಕ uint64_t stratum = VN_IS(dtypep, UnpackArrayDType)                  ? 9
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
        ಗಾಗಿ (std::ಸದಿಶ<AstVar*>* ಸ್ಥಿರಾಂಕ varpsp : {&trivps, &nontps}) {
            ಆದರೆ (varpsp->ಖಾಲಿಗಳು()) ಮುಂದುವರಿ;
            ಆದರೆ (!v3Global.opt.mtasks()) {
                simple_ವಿಂಗಡಿಸು_Vars(*varpsp);
            } ಇಲ್ಲದಿದ್ದರೆ {
                mtask_ವಿಂಗಡಿಸು_Vars(*varpsp);
            }
        }

        // Insert them back under the module, in the new order, but at
        // the front of the list so they come out first in dumps/JSON.
        Ast_ಗಂಟು* nodesp = ಶೂನ್ಯಸೂಚಕ;
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<AstVar*>* ಸ್ಥಿರಾಂಕ varpsp : {&portps, &trivps, &nontps, &restps}) {
            ಗಾಗಿ (AstVar* ಸ್ಥಿರಾಂಕ varp : *varpsp) {
                nodesp = Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ_Null(nodesp, varp->unlink_Fr_ಹಿಂಭಾಗ());
            }
        }
        modp->prependStmtsp(nodesp);
    }

ಸಾರ್ವಜನಿಕ:
    ಸ್ಥಿರ ಶೂನ್ಯ apply(Ast_ಗಂಟು_Module* modp, ಸ್ಥಿರಾಂಕ MTaskWritersMap& mTaskWriters) {
        Variable_ಕ್ರಮ{modp, mTaskWriters};
    }
};

//######################################################################
// V3VariableOrder static functions

ಶೂನ್ಯ V3_Variable_ಕ್ರಮ::ಕ್ರಮ_All(AstNetlist* netlistp) {
    UINFO(2, __FUNCTION__ << ":");

    MTaskWritersMap mTaskWriters;

    // Gather writing MTasks
    ಆದರೆ (v3Global.opt.mtasks()) {
        netlistp->ತುದಿ_Modulep()->foreach([&](Ast_Exec_ನಕ್ಷೆ* execGraphp) {
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಶೃಂಗ& vtx : execGraphp->depGraphp()->vertices()) {
                GatherMTaskWriters::apply(vtx.as<ಸ್ಥಿರಾಂಕ ExecMTask>(), mTaskWriters);
            }
        });
    }

    // Sort variables of each module
    ಗಾಗಿ (Ast_ಗಂಟು_Module* modp = v3Global.rootp()->modulesp(); modp;
         modp = VN_AS(modp->nextp(), ಗಂಟು_Module)) {
        ಆದರೆ (modp->isConstPool()) ಮುಂದುವರಿ;
        Variable_ಕ್ರಮ::apply(modp, mTaskWriters);
    }

    V3Global::dump_Check_Global_ವೃಕ್ಷ("variableorder", 0, dump_ವೃಕ್ಷ_Either_Level() >= 3);
}
