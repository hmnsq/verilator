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

#অন্তর্ভাৱ "V3PchAstMT.h"

#অন্তর্ভাৱ "V3VariableOrder.h"

#অন্তর্ভাৱ "V3AstUserAllocator.h"
#অন্তর্ভাৱ "V3ExecGraph.h"
#অন্তর্ভাৱ "V3Stats.h"

#অন্তর্ভাৱ <algorithm>
#অন্তর্ভাৱ <cstdint>
#অন্তর্ভাৱ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

using MTaskIdVec = std::vector<তার্কিক>;  // Used as a bit-set indexed by MTask ID
// Writing MTasks of each variable accessed by any MTask
using MTaskWritersMap = std::unordered_map<ধ্রুৱ AstVar*, MTaskIdVec>;

// Trace through code reachable from an MTask and record the writers of referenced variables
শ্ৰেণী GatherMTaskWriters final : VNVisitorConst {
    // NODE STATE
    //  AstCFunc::user1()  // bool: Already traced this function
    //  AstVar::user1()  // bool: Already traced this variable
    ধ্রুৱ VNUser1InUse m_user1InUse;

    // STATE
    MTaskWritersMap& m_results;  // The result map being built;
    ধ্রুৱ uint32_t m_id;  // Id of mtask being analysed
    ধ্রুৱ size_t m_usedIds = ExecMTask::অঙ্ক_Used_Ids();  // Value of max id + 1

    // CONSTRUCTOR
    GatherMTaskWriters(ধ্রুৱ ExecMTask* mTaskp, MTaskWritersMap& results)
        : m_results{results}
        , m_id{mTaskp->id()} {
        iterateConst(mTaskp->funcp());
    }
    ~GatherMTaskWriters() = স্ৱভাৱ;
    VL_UNMOVABLE(GatherMTaskWriters);

    // VISIT
    শূন্য visit(Ast_গ্ৰন্থি_Var_Ref* nodep) override {
        // Cheaper than relying on emplace().second
        যদি (nodep->user1SetOnce()) ঘূৰাই;
        AstVar* ধ্রুৱ varp = nodep->varp();
        // Record the access, and the writer bit if written
        MTaskIdVec& writers = m_results
                                  .emplace(std::piecewise_construct,  //
                                           std::forward_as_tuple(varp),  //
                                           std::forward_as_tuple(m_usedIds))
                                  .first->second;
        যদি (nodep->access().isWriteOrRW()) writers[m_id] = সঁচা;
    }

    শূন্য visit(AstCFunc* nodep) override {
        যদি (nodep->user1SetOnce()) ঘূৰাই;  // Prevent repeat traversals/recursion
        iterateChildrenConst(nodep);
    }

    শূন্য visit(Ast_গ্ৰন্থি_CCall* nodep) override {
        iterateChildrenConst(nodep);  // Arguments
        iterateConst(nodep->funcp());  // Callee
    }

    শূন্য visit(Ast_গ্ৰন্থি* nodep) override { iterateChildrenConst(nodep); }

সার্ৱজনিক:
    স্থির শূন্য apply(ধ্রুৱ ExecMTask* mTaskp, MTaskWritersMap& results) {
        GatherMTaskWriters{mTaskp, results};
    }
};

শ্ৰেণী VariableOrder final {
    // NODE STATE
    //  AstVar::user1()   ->   uint64_t: Stratum, roughly equivalent to alignment
    //                                   requirement, to avoid padding
    ধ্রুৱ VNUser1InUse m_user1InUse;

    ধ্রুৱ MTaskWritersMap& m_mTaskWriters;

    VariableOrder(Ast_গ্ৰন্থি_Module* modp, ধ্রুৱ MTaskWritersMap& mTaskWriters)
        : m_mTaskWriters{mTaskWriters} {
        orderModuleVars(modp);
    }
    ~VariableOrder() = স্ৱভাৱ;
    VL_UNCOPYABLE(VariableOrder);

    //######################################################################

    // Simple sort by stratum
    শূন্য simpleSortVars(std::vector<AstVar*>& varps) {
        stable_sort(varps.begin(), varps.end(), [](ধ্রুৱ AstVar* ap, ধ্রুৱ AstVar* bp) -> তার্কিক {
            ঘূৰাই ap->user1() < bp->user1();
        });
    }

    // Sort by writing MTasks first, then the same as simpleSortVars
    শূন্য mtaskSortVars(std::vector<AstVar*>& varps) {
        // Map from "writing MTasks" -> "variable list", for variables accessed by MTasks
        std::map<MTaskIdVec, std::vector<AstVar*>> m2v;
        // Variables not accessed by any MTask
        std::vector<AstVar*> noAffinityVarps;
        বাবে (AstVar* ধ্রুৱ varp : varps) {
            ধ্রুৱ auto it = m_mTaskWriters.find(varp);
            যদি (it == m_mTaskWriters.end()) {
                noAffinityVarps.ঠেলো_back(varp);
            } অন্যথা {
                m2v[it->second].ঠেলো_back(varp);
            }
        }

        varps.clear();

        // Helper function to sort given vector, then append to 'varps'
        ধ্রুৱ auto sortAndAppend
            = [এতত্, &varps](std::vector<AstVar*>& subVarps, তার্কিক alignFirst) {
                  simpleSortVars(subVarps);
                  তার্কিক aligned = !alignFirst;
                  বাবে (AstVar* ধ্রুৱ varp : subVarps) {
                      // Align the first variable with instance storage
                      যদি (!aligned && varp->isModelState()) {
                          varp->mtask_Cache_সাৰি_Align(সঁচা);
                          V3Stats::addStatSum("VariableOrder, MTask aligned group starts", 1);
                          aligned = সঁচা;
                      }
                      varps.ঠেলো_back(varp);
                  }
              };

        // Add the groups in the map's deterministic key order
        বাবে (auto& pair : m2v) sortAndAppend(pair.second, সঁচা);

        // Finally add the variables with no known MTask affinity
        sortAndAppend(noAffinityVarps, মিছা);

        V3Stats::addStatSum("VariableOrder, MTask affinity groups", m2v.size());
        V3Stats::addStatSum("VariableOrder, no-affinity variables", noAffinityVarps.size());
    }

    // cppcheck-suppress constParameterPointer
    শূন্য orderModuleVars(Ast_গ্ৰন্থি_Module* modp) {
        // Top level ports stay first, in source order (--lib-create wrapper must match original)
        std::vector<AstVar*> portps;
        // Followed by trivially constructible variables
        std::vector<AstVar*> trivps;
        // Followed by not trivially constructible variables
        std::vector<AstVar*> nontps;
        // Followed by variables that are not model state, in source order
        std::vector<AstVar*> restps;

        // Gather variables, compute stratum
        বাবে (Ast_গ্ৰন্থি *nodep = modp->stmtsp(), *nextp; nodep; nodep = nextp) {
            nextp = nodep->nextp();
            যদি (AstVar* ধ্রুৱ varp = VN_CAST(nodep, Var)) {
                যদি (varp->isPrimaryIO()) {
                    portps.ঠেলো_back(varp);
                    অনুৱর্ত;
                }
                যদি (!varp->isModelState()) {
                    restps.ঠেলো_back(varp);
                    অনুৱর্ত;
                }

                যদি (varp->isTriviallyConstructible()) {
                    trivps.ঠেলো_back(varp);
                } অন্যথা {
                    nontps.ঠেলো_back(varp);
                }

                Ast_গ্ৰন্থি_DType* ধ্রুৱ dtypep = varp->dtypeSkipRefp();
                ধ্রুৱ পূর্ণাঙ্ক sigbytes = dtypep->widthAlignBytes();
                ধ্রুৱ uint64_t stratum = VN_IS(dtypep, UnpackArrayDType)                  ? 9
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
        বাবে (std::vector<AstVar*>* ধ্রুৱ varpsp : {&trivps, &nontps}) {
            যদি (varpsp->empty()) অনুৱর্ত;
            যদি (!v3Global.opt.mtasks()) {
                simpleSortVars(*varpsp);
            } অন্যথা {
                mtaskSortVars(*varpsp);
            }
        }

        // Insert them back under the module, in the new order, but at
        // the front of the list so they come out first in dumps/JSON.
        Ast_গ্ৰন্থি* nodesp = nullptr;
        বাবে (ধ্রুৱ std::vector<AstVar*>* ধ্রুৱ varpsp : {&portps, &trivps, &nontps, &restps}) {
            বাবে (AstVar* ধ্রুৱ varp : *varpsp) {
                nodesp = Ast_গ্ৰন্থি::addNextNull(nodesp, varp->unlinkFrBack());
            }
        }
        modp->prependStmtsp(nodesp);
    }

সার্ৱজনিক:
    স্থির শূন্য apply(Ast_গ্ৰন্থি_Module* modp, ধ্রুৱ MTaskWritersMap& mTaskWriters) {
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
        netlistp->শীৰ্ষ_Modulep()->foreach([&](AstExecGraph* execGraphp) {
            বাবে (ধ্রুৱ V3GraphVertex& vtx : execGraphp->depGraphp()->vertices()) {
                GatherMTaskWriters::apply(vtx.as<ধ্রুৱ ExecMTask>(), mTaskWriters);
            }
        });
    }

    // Sort variables of each module
    বাবে (Ast_গ্ৰন্থি_Module* modp = v3Global.rootp()->modulesp(); modp;
         modp = VN_AS(modp->nextp(), গ্ৰন্থি_Module)) {
        যদি (modp->isConstPool()) অনুৱর্ত;
        VariableOrder::apply(modp, mTaskWriters);
    }

    V3Global::dump_Check_Global_বৃক্ষ("variableorder", 0, dump_বৃক্ষ_Either_Level() >= 3);
}
