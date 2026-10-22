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

using MTaskIdVec = std::vector<যৌক্তিক>;  // Used as a bit-set indexed by MTask ID
// Writing MTasks of each variable accessed by any MTask
using MTaskWritersMap = std::unordered_map<ধ্রুবক AstVar*, MTaskIdVec>;

// Trace through code reachable from an MTask and record the writers of referenced variables
শ্রেণী GatherMTaskWriters final : VNVisitorConst {
    // NODE STATE
    //  AstCFunc::user1()  // bool: Already traced this function
    //  AstVar::user1()  // bool: Already traced this variable
    ধ্রুবক VNUser1InUse m_user1InUse;

    // STATE
    MTaskWritersMap& m_results;  // The result map being built;
    ধ্রুবক uint32_t m_id;  // Id of mtask being analysed
    ধ্রুবক আকার_t m_usedIds = ExecMTask::অঙ্ক_Used_Ids();  // Value of max id + 1

    // CONSTRUCTOR
    GatherMTaskWriters(ধ্রুবক ExecMTask* mTaskp, MTaskWritersMap& results)
        : m_results{results}
        , m_id{mTaskp->id()} {
        iterateConst(mTaskp->funcp());
    }
    ~GatherMTaskWriters() = পূর্বনির্ধারিত;
    VL_UNMOVABLE(GatherMTaskWriters);

    // VISIT
    শূন্য visit(Ast_গ্রন্থি_Var_Ref* nodep) override {
        // Cheaper than relying on emplace().second
        যদি (nodep->user1SetOnce()) ফেরত;
        AstVar* ধ্রুবক varp = nodep->varp();
        // Record the access, and the writer bit if written
        MTaskIdVec& writers = m_results
                                  .emplace(std::piecewise_construct,  //
                                           std::forward_as_tuple(varp),  //
                                           std::forward_as_tuple(m_usedIds))
                                  .first->second;
        যদি (nodep->access().isWriteOrRW()) writers[m_id] = সত্য;
    }

    শূন্য visit(AstCFunc* nodep) override {
        যদি (nodep->user1SetOnce()) ফেরত;  // Prevent repeat traversals/recursion
        iterateChildrenConst(nodep);
    }

    শূন্য visit(Ast_গ্রন্থি_CCall* nodep) override {
        iterateChildrenConst(nodep);  // Arguments
        iterateConst(nodep->funcp());  // Callee
    }

    শূন্য visit(Ast_গ্রন্থি* nodep) override { iterateChildrenConst(nodep); }

সর্বজনীন:
    স্থির শূন্য apply(ধ্রুবক ExecMTask* mTaskp, MTaskWritersMap& results) {
        GatherMTaskWriters{mTaskp, results};
    }
};

শ্রেণী VariableOrder final {
    // NODE STATE
    //  AstVar::user1()   ->   uint64_t: Stratum, roughly equivalent to alignment
    //                                   requirement, to avoid padding
    ধ্রুবক VNUser1InUse m_user1InUse;

    ধ্রুবক MTaskWritersMap& m_mTaskWriters;

    VariableOrder(Ast_গ্রন্থি_Module* modp, ধ্রুবক MTaskWritersMap& mTaskWriters)
        : m_mTaskWriters{mTaskWriters} {
        orderModuleVars(modp);
    }
    ~VariableOrder() = পূর্বনির্ধারিত;
    VL_UNCOPYABLE(VariableOrder);

    //######################################################################

    // Simple sort by stratum
    শূন্য simpleSortVars(std::vector<AstVar*>& varps) {
        stable_sort(varps.begin(), varps.end(), [](ধ্রুবক AstVar* ap, ধ্রুবক AstVar* bp) -> যৌক্তিক {
            ফেরত ap->user1() < bp->user1();
        });
    }

    // Sort by writing MTasks first, then the same as simpleSortVars
    শূন্য mtaskSortVars(std::vector<AstVar*>& varps) {
        // Map from "writing MTasks" -> "variable list", for variables accessed by MTasks
        std::map<MTaskIdVec, std::vector<AstVar*>> m2v;
        // Variables not accessed by any MTask
        std::vector<AstVar*> noAffinityVarps;
        জন্য (AstVar* ধ্রুবক varp : varps) {
            ধ্রুবক auto it = m_mTaskWriters.find(varp);
            যদি (it == m_mTaskWriters.end()) {
                noAffinityVarps.ঠেলো_back(varp);
            } অন্যথা {
                m2v[it->second].ঠেলো_back(varp);
            }
        }

        varps.clear();

        // Helper function to sort given vector, then append to 'varps'
        ধ্রুবক auto sortAndAppend
            = [এই, &varps](std::vector<AstVar*>& subVarps, যৌক্তিক alignFirst) {
                  simpleSortVars(subVarps);
                  যৌক্তিক aligned = !alignFirst;
                  জন্য (AstVar* ধ্রুবক varp : subVarps) {
                      // Align the first variable with instance storage
                      যদি (!aligned && varp->isModelState()) {
                          varp->mtask_Cache_সারি_Align(সত্য);
                          V3Stats::add_Stat_যোগ("VariableOrder, MTask aligned group starts", 1);
                          aligned = সত্য;
                      }
                      varps.ঠেলো_back(varp);
                  }
              };

        // Add the groups in the map's deterministic key order
        জন্য (auto& pair : m2v) sortAndAppend(pair.second, সত্য);

        // Finally add the variables with no known MTask affinity
        sortAndAppend(noAffinityVarps, মিথ্যা);

        V3Stats::add_Stat_যোগ("VariableOrder, MTask affinity groups", m2v.আকার_2());
        V3Stats::add_Stat_যোগ("VariableOrder, no-affinity variables", noAffinityVarps.আকার_2());
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
        জন্য (Ast_গ্রন্থি *nodep = modp->stmtsp(), *nextp; nodep; nodep = nextp) {
            nextp = nodep->nextp();
            যদি (AstVar* ধ্রুবক varp = VN_CAST(nodep, Var)) {
                যদি (varp->isPrimaryIO()) {
                    portps.ঠেলো_back(varp);
                    চালু;
                }
                যদি (!varp->isModelState()) {
                    restps.ঠেলো_back(varp);
                    চালু;
                }

                যদি (varp->isTriviallyConstructible()) {
                    trivps.ঠেলো_back(varp);
                } অন্যথা {
                    nontps.ঠেলো_back(varp);
                }

                Ast_গ্রন্থি_DType* ধ্রুবক dtypep = varp->dtypeSkipRefp();
                ধ্রুবক পূর্ণসংখ্যা sigbytes = dtypep->widthAlignBytes();
                ধ্রুবক uint64_t stratum = VN_IS(dtypep, UnpackArrayDType)                  ? 9
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
        জন্য (std::vector<AstVar*>* ধ্রুবক varpsp : {&trivps, &nontps}) {
            যদি (varpsp->empty()) চালু;
            যদি (!v3Global.opt.mtasks()) {
                simpleSortVars(*varpsp);
            } অন্যথা {
                mtaskSortVars(*varpsp);
            }
        }

        // Insert them back under the module, in the new order, but at
        // the front of the list so they come out first in dumps/JSON.
        Ast_গ্রন্থি* nodesp = nullptr;
        জন্য (ধ্রুবক std::vector<AstVar*>* ধ্রুবক varpsp : {&portps, &trivps, &nontps, &restps}) {
            জন্য (AstVar* ধ্রুবক varp : *varpsp) {
                nodesp = Ast_গ্রন্থি::add_পরবর্তী_Null(nodesp, varp->unlinkFrBack());
            }
        }
        modp->prependStmtsp(nodesp);
    }

সর্বজনীন:
    স্থির শূন্য apply(Ast_গ্রন্থি_Module* modp, ধ্রুবক MTaskWritersMap& mTaskWriters) {
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
            জন্য (ধ্রুবক V3GraphVertex& vtx : execGraphp->depGraphp()->vertices()) {
                GatherMTaskWriters::apply(vtx.as<ধ্রুবক ExecMTask>(), mTaskWriters);
            }
        });
    }

    // Sort variables of each module
    জন্য (Ast_গ্রন্থি_Module* modp = v3Global.rootp()->modulesp(); modp;
         modp = VN_AS(modp->nextp(), গ্রন্থি_Module)) {
        যদি (modp->isConstPool()) চালু;
        VariableOrder::apply(modp, mTaskWriters);
    }

    V3Global::dump_Check_Global_বৃক্ষ("variableorder", 0, dump_বৃক্ষ_Either_Level() >= 3);
}
