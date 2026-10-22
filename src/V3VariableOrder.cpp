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

#অন্তর্ভাব "V3PchAstMT.h"

#অন্তর্ভাব "V3VariableOrder.h"

#অন্তর্ভাব "V3AstUserAllocator.h"
#অন্তর্ভাব "V3ExecGraph.h"
#অন্তর্ভাব "V3Stats.h"

#অন্তর্ভাব <algorithm>
#অন্তর্ভাব <cstdint>
#অন্তর্ভাব <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

using MTaskIdVec = std::vector<তার্কিক>;  // Used as a bit-set indexed by MTask ID
// Writing MTasks of each variable accessed by any MTask
using MTaskWritersMap = std::unordered_map<ধ্রুব AstVar*, MTaskIdVec>;

// Trace through code reachable from an MTask and record the writers of referenced variables
বর্গ GatherMTaskWriters final : VNVisitorConst {
    // NODE STATE
    //  AstCFunc::user1()  // bool: Already traced this function
    //  AstVar::user1()  // bool: Already traced this variable
    ধ্রুব VNUser1InUse m_user1InUse;

    // STATE
    MTaskWritersMap& m_results;  // The result map being built;
    ধ্রুব uint32_t m_id;  // Id of mtask being analysed
    ধ্রুব size_t m_usedIds = ExecMTask::অঙ্ক_Used_Ids();  // Value of max id + 1

    // CONSTRUCTOR
    GatherMTaskWriters(ধ্রুব ExecMTask* mTaskp, MTaskWritersMap& results)
        : m_results{results}
        , m_id{mTaskp->id()} {
        iterateConst(mTaskp->funcp());
    }
    ~GatherMTaskWriters() = স্বভাব;
    VL_UNMOVABLE(GatherMTaskWriters);

    // VISIT
    শূন্য visit(Ast_গ্রন্থি_Var_Ref* nodep) override {
        // Cheaper than relying on emplace().second
        যদি (nodep->user1SetOnce()) প্রত্যাগম;
        AstVar* ধ্রুব varp = nodep->varp();
        // Record the access, and the writer bit if written
        MTaskIdVec& writers = m_results
                                  .emplace(std::piecewise_construct,  //
                                           std::forward_as_tuple(varp),  //
                                           std::forward_as_tuple(m_usedIds))
                                  .first->second;
        যদি (nodep->access().isWriteOrRW()) writers[m_id] = সত্য;
    }

    শূন্য visit(AstCFunc* nodep) override {
        যদি (nodep->user1SetOnce()) প্রত্যাগম;  // Prevent repeat traversals/recursion
        iterateChildrenConst(nodep);
    }

    শূন্য visit(Ast_গ্রন্থি_CCall* nodep) override {
        iterateChildrenConst(nodep);  // Arguments
        iterateConst(nodep->funcp());  // Callee
    }

    শূন্য visit(Ast_গ্রন্থি* nodep) override { iterateChildrenConst(nodep); }

সার্বজনিক:
    স্থির শূন্য apply(ধ্রুব ExecMTask* mTaskp, MTaskWritersMap& results) {
        GatherMTaskWriters{mTaskp, results};
    }
};

বর্গ VariableOrder final {
    // NODE STATE
    //  AstVar::user1()   ->   uint64_t: Stratum, roughly equivalent to alignment
    //                                   requirement, to avoid padding
    ধ্রুব VNUser1InUse m_user1InUse;

    ধ্রুব MTaskWritersMap& m_mTaskWriters;

    VariableOrder(Ast_গ্রন্থি_Module* modp, ধ্রুব MTaskWritersMap& mTaskWriters)
        : m_mTaskWriters{mTaskWriters} {
        orderModuleVars(modp);
    }
    ~VariableOrder() = স্বভাব;
    VL_UNCOPYABLE(VariableOrder);

    //######################################################################

    // Simple sort by stratum
    শূন্য simpleSortVars(std::vector<AstVar*>& varps) {
        stable_sort(varps.begin(), varps.end(), [](ধ্রুব AstVar* ap, ধ্রুব AstVar* bp) -> তার্কিক {
            প্রত্যাগম ap->user1() < bp->user1();
        });
    }

    // Sort by writing MTasks first, then the same as simpleSortVars
    শূন্য mtaskSortVars(std::vector<AstVar*>& varps) {
        // Map from "writing MTasks" -> "variable list", for variables accessed by MTasks
        std::map<MTaskIdVec, std::vector<AstVar*>> m2v;
        // Variables not accessed by any MTask
        std::vector<AstVar*> noAffinityVarps;
        প্রতি (AstVar* ধ্রুব varp : varps) {
            ধ্রুব auto it = m_mTaskWriters.find(varp);
            যদি (it == m_mTaskWriters.end()) {
                noAffinityVarps.ঠেলো_back(varp);
            } অন্যথা {
                m2v[it->second].ঠেলো_back(varp);
            }
        }

        varps.clear();

        // Helper function to sort given vector, then append to 'varps'
        ধ্রুব auto sortAndAppend
            = [এতত্, &varps](std::vector<AstVar*>& subVarps, তার্কিক alignFirst) {
                  simpleSortVars(subVarps);
                  তার্কিক aligned = !alignFirst;
                  প্রতি (AstVar* ধ্রুব varp : subVarps) {
                      // Align the first variable with instance storage
                      যদি (!aligned && varp->isModelState()) {
                          varp->mtask_Cache_সারি_Align(সত্য);
                          V3Stats::addStatSum("VariableOrder, MTask aligned group starts", 1);
                          aligned = সত্য;
                      }
                      varps.ঠেলো_back(varp);
                  }
              };

        // Add the groups in the map's deterministic key order
        প্রতি (auto& pair : m2v) sortAndAppend(pair.second, সত্য);

        // Finally add the variables with no known MTask affinity
        sortAndAppend(noAffinityVarps, অসত্য);

        V3Stats::addStatSum("VariableOrder, MTask affinity groups", m2v.size());
        V3Stats::addStatSum("VariableOrder, no-affinity variables", noAffinityVarps.size());
    }

    // cppcheck-suppress constParameterPointer
    শূন্য orderModuleVars(Ast_গ্রন্থি_Module* modp) {
        // Top level ports stay first, in source order (--lib-create wrapper must match original)
        std::vector<AstVar*> portps;
        // Followed by trivially constructible variables
        std::vector<AstVar*> trivps;
        // Followed by not trivially constructible variables
        std::vector<AstVar*> nontps;
        // Followed by variables that are not model state, in source order
        std::vector<AstVar*> restps;

        // Gather variables, compute stratum
        প্রতি (Ast_গ্রন্থি *nodep = modp->stmtsp(), *nextp; nodep; nodep = nextp) {
            nextp = nodep->nextp();
            যদি (AstVar* ধ্রুব varp = VN_CAST(nodep, Var)) {
                যদি (varp->isPrimaryIO()) {
                    portps.ঠেলো_back(varp);
                    অনুবর্ত;
                }
                যদি (!varp->isModelState()) {
                    restps.ঠেলো_back(varp);
                    অনুবর্ত;
                }

                যদি (varp->isTriviallyConstructible()) {
                    trivps.ঠেলো_back(varp);
                } অন্যথা {
                    nontps.ঠেলো_back(varp);
                }

                Ast_গ্রন্থি_DType* ধ্রুব dtypep = varp->dtypeSkipRefp();
                ধ্রুব পূর্ণাঙ্ক sigbytes = dtypep->widthAlignBytes();
                ধ্রুব uint64_t stratum = VN_IS(dtypep, UnpackArrayDType)                  ? 9
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
        প্রতি (std::vector<AstVar*>* ধ্রুব varpsp : {&trivps, &nontps}) {
            যদি (varpsp->empty()) অনুবর্ত;
            যদি (!v3Global.opt.mtasks()) {
                simpleSortVars(*varpsp);
            } অন্যথা {
                mtaskSortVars(*varpsp);
            }
        }

        // Insert them back under the module, in the new order, but at
        // the front of the list so they come out first in dumps/JSON.
        Ast_গ্রন্থি* nodesp = nullptr;
        প্রতি (ধ্রুব std::vector<AstVar*>* ধ্রুব varpsp : {&portps, &trivps, &nontps, &restps}) {
            প্রতি (AstVar* ধ্রুব varp : *varpsp) {
                nodesp = Ast_গ্রন্থি::addNextNull(nodesp, varp->unlinkFrBack());
            }
        }
        modp->prependStmtsp(nodesp);
    }

সার্বজনিক:
    স্থির শূন্য apply(Ast_গ্রন্থি_Module* modp, ধ্রুব MTaskWritersMap& mTaskWriters) {
        VariableOrder{modp, mTaskWriters};
    }
};

//######################################################################
// V3VariableOrder static functions

শূন্য V3VariableOrder::orderAll(AstNetlist* netlistp) {
    UINFO(2, __FUNCTION__ << ":");

    MTaskWritersMap mTaskWriters;

    // Gather writing MTasks
    যদি (v3Global.opt.mtasks()) {
        netlistp->শীর্ষ_Modulep()->foreach([&](AstExecGraph* execGraphp) {
            প্রতি (ধ্রুব V3GraphVertex& vtx : execGraphp->depGraphp()->vertices()) {
                GatherMTaskWriters::apply(vtx.as<ধ্রুব ExecMTask>(), mTaskWriters);
            }
        });
    }

    // Sort variables of each module
    প্রতি (Ast_গ্রন্থি_Module* modp = v3Global.rootp()->modulesp(); modp;
         modp = VN_AS(modp->nextp(), গ্রন্থি_Module)) {
        যদি (modp->isConstPool()) অনুবর্ত;
        VariableOrder::apply(modp, mTaskWriters);
    }

    V3Global::dump_Check_Global_বৃক্ষ("variableorder", 0, dump_বৃক্ষ_Either_Level() >= 3);
}
