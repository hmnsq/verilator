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

#અન્તર્ભાવ "V3PchAstMT.h"

#અન્તર્ભાવ "V3VariableOrder.h"

#અન્તર્ભાવ "V3AstUserAllocator.h"
#અન્તર્ભાવ "V3ExecGraph.h"
#અન્તર્ભાવ "V3Stats.h"

#અન્તર્ભાવ <algorithm>
#અન્તર્ભાવ <cstdint>
#અન્તર્ભાવ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

using MTaskIdVec = std::vector<તાર્કિક>;  // Used as a bit-set indexed by MTask ID
// Writing MTasks of each variable accessed by any MTask
using MTaskWritersMap = std::unordered_map<અચલ AstVar*, MTaskIdVec>;

// Trace through code reachable from an MTask and record the writers of referenced variables
વર્ગ GatherMTaskWriters final : VNVisitorConst {
    // NODE STATE
    //  AstCFunc::user1()  // bool: Already traced this function
    //  AstVar::user1()  // bool: Already traced this variable
    અચલ VNUser1InUse m_user1InUse;

    // STATE
    MTaskWritersMap& m_results;  // The result map being built;
    અચલ uint32_t m_id;  // Id of mtask being analysed
    અચલ size_t m_usedIds = ExecMTask::અંક_Used_Ids();  // Value of max id + 1

    // CONSTRUCTOR
    GatherMTaskWriters(અચલ ExecMTask* mTaskp, MTaskWritersMap& results)
        : m_results{results}
        , m_id{mTaskp->id()} {
        iterateConst(mTaskp->funcp());
    }
    ~GatherMTaskWriters() = મૂળભૂત;
    VL_UNMOVABLE(GatherMTaskWriters);

    // VISIT
    શૂન્ય visit(Ast_ગાંઠ_Var_Ref* nodep) override {
        // Cheaper than relying on emplace().second
        જો (nodep->user1SetOnce()) પાછા;
        AstVar* અચલ varp = nodep->varp();
        // Record the access, and the writer bit if written
        MTaskIdVec& writers = m_results
                                  .emplace(std::piecewise_construct,  //
                                           std::forward_as_tuple(varp),  //
                                           std::forward_as_tuple(m_usedIds))
                                  .first->second;
        જો (nodep->access().isWriteOrRW()) writers[m_id] = સાચું;
    }

    શૂન્ય visit(AstCFunc* nodep) override {
        જો (nodep->user1SetOnce()) પાછા;  // Prevent repeat traversals/recursion
        iterateChildrenConst(nodep);
    }

    શૂન્ય visit(Ast_ગાંઠ_CCall* nodep) override {
        iterateChildrenConst(nodep);  // Arguments
        iterateConst(nodep->funcp());  // Callee
    }

    શૂન્ય visit(Ast_ગાંઠ* nodep) override { iterateChildrenConst(nodep); }

જાહેર:
    સ્થિર શૂન્ય apply(અચલ ExecMTask* mTaskp, MTaskWritersMap& results) {
        GatherMTaskWriters{mTaskp, results};
    }
};

વર્ગ VariableOrder final {
    // NODE STATE
    //  AstVar::user1()   ->   uint64_t: Stratum, roughly equivalent to alignment
    //                                   requirement, to avoid padding
    અચલ VNUser1InUse m_user1InUse;

    અચલ MTaskWritersMap& m_mTaskWriters;

    VariableOrder(Ast_ગાંઠ_Module* modp, અચલ MTaskWritersMap& mTaskWriters)
        : m_mTaskWriters{mTaskWriters} {
        orderModuleVars(modp);
    }
    ~VariableOrder() = મૂળભૂત;
    VL_UNCOPYABLE(VariableOrder);

    //######################################################################

    // Simple sort by stratum
    શૂન્ય simpleSortVars(std::vector<AstVar*>& varps) {
        stable_sort(varps.begin(), varps.end(), [](અચલ AstVar* ap, અચલ AstVar* bp) -> તાર્કિક {
            પાછા ap->user1() < bp->user1();
        });
    }

    // Sort by writing MTasks first, then the same as simpleSortVars
    શૂન્ય mtaskSortVars(std::vector<AstVar*>& varps) {
        // Map from "writing MTasks" -> "variable list", for variables accessed by MTasks
        std::map<MTaskIdVec, std::vector<AstVar*>> m2v;
        // Variables not accessed by any MTask
        std::vector<AstVar*> noAffinityVarps;
        માટે (AstVar* અચલ varp : varps) {
            અચલ auto it = m_mTaskWriters.find(varp);
            જો (it == m_mTaskWriters.end()) {
                noAffinityVarps.ધકેલો_back(varp);
            } નહીંતર {
                m2v[it->second].ધકેલો_back(varp);
            }
        }

        varps.clear();

        // Helper function to sort given vector, then append to 'varps'
        અચલ auto sortAndAppend
            = [આ, &varps](std::vector<AstVar*>& subVarps, તાર્કિક alignFirst) {
                  simpleSortVars(subVarps);
                  તાર્કિક aligned = !alignFirst;
                  માટે (AstVar* અચલ varp : subVarps) {
                      // Align the first variable with instance storage
                      જો (!aligned && varp->isModelState()) {
                          varp->mtask_Cache_લીટી_Align(સાચું);
                          V3Stats::addStatSum("VariableOrder, MTask aligned group starts", 1);
                          aligned = સાચું;
                      }
                      varps.ધકેલો_back(varp);
                  }
              };

        // Add the groups in the map's deterministic key order
        માટે (auto& pair : m2v) sortAndAppend(pair.second, સાચું);

        // Finally add the variables with no known MTask affinity
        sortAndAppend(noAffinityVarps, ખોટું);

        V3Stats::addStatSum("VariableOrder, MTask affinity groups", m2v.size());
        V3Stats::addStatSum("VariableOrder, no-affinity variables", noAffinityVarps.size());
    }

    // cppcheck-suppress constParameterPointer
    શૂન્ય orderModuleVars(Ast_ગાંઠ_Module* modp) {
        // Top level ports stay first, in source order (--lib-create wrapper must match original)
        std::vector<AstVar*> portps;
        // Followed by trivially constructible variables
        std::vector<AstVar*> trivps;
        // Followed by not trivially constructible variables
        std::vector<AstVar*> nontps;
        // Followed by variables that are not model state, in source order
        std::vector<AstVar*> restps;

        // Gather variables, compute stratum
        માટે (Ast_ગાંઠ *nodep = modp->stmtsp(), *nextp; nodep; nodep = nextp) {
            nextp = nodep->nextp();
            જો (AstVar* અચલ varp = VN_CAST(nodep, Var)) {
                જો (varp->isPrimaryIO()) {
                    portps.ધકેલો_back(varp);
                    ચાલુ;
                }
                જો (!varp->isModelState()) {
                    restps.ધકેલો_back(varp);
                    ચાલુ;
                }

                જો (varp->isTriviallyConstructible()) {
                    trivps.ધકેલો_back(varp);
                } નહીંતર {
                    nontps.ધકેલો_back(varp);
                }

                Ast_ગાંઠ_DType* અચલ dtypep = varp->dtypeSkipRefp();
                અચલ પૂર્ણાંક sigbytes = dtypep->widthAlignBytes();
                અચલ uint64_t stratum = VN_IS(dtypep, UnpackArrayDType)                  ? 9
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
        માટે (std::vector<AstVar*>* અચલ varpsp : {&trivps, &nontps}) {
            જો (varpsp->empty()) ચાલુ;
            જો (!v3Global.opt.mtasks()) {
                simpleSortVars(*varpsp);
            } નહીંતર {
                mtaskSortVars(*varpsp);
            }
        }

        // Insert them back under the module, in the new order, but at
        // the front of the list so they come out first in dumps/JSON.
        Ast_ગાંઠ* nodesp = nullptr;
        માટે (અચલ std::vector<AstVar*>* અચલ varpsp : {&portps, &trivps, &nontps, &restps}) {
            માટે (AstVar* અચલ varp : *varpsp) {
                nodesp = Ast_ગાંઠ::addNextNull(nodesp, varp->unlinkFrBack());
            }
        }
        modp->prependStmtsp(nodesp);
    }

જાહેર:
    સ્થિર શૂન્ય apply(Ast_ગાંઠ_Module* modp, અચલ MTaskWritersMap& mTaskWriters) {
        VariableOrder{modp, mTaskWriters};
    }
};

//######################################################################
// V3VariableOrder static functions

શૂન્ય V3VariableOrder::orderAll(AstNetlist* netlistp) {
    UINFO(2, __FUNCTION__ << ":");

    MTaskWritersMap mTaskWriters;

    // Gather writing MTasks
    જો (v3Global.opt.mtasks()) {
        netlistp->ટોચ_Modulep()->foreach([&](AstExecGraph* execGraphp) {
            માટે (અચલ V3GraphVertex& vtx : execGraphp->depGraphp()->vertices()) {
                GatherMTaskWriters::apply(vtx.as<અચલ ExecMTask>(), mTaskWriters);
            }
        });
    }

    // Sort variables of each module
    માટે (Ast_ગાંઠ_Module* modp = v3Global.rootp()->modulesp(); modp;
         modp = VN_AS(modp->nextp(), ગાંઠ_Module)) {
        જો (modp->isConstPool()) ચાલુ;
        VariableOrder::apply(modp, mTaskWriters);
    }

    V3Global::dump_Check_Global_વૃક્ષ("variableorder", 0, dump_વૃક્ષ_Either_Level() >= 3);
}
