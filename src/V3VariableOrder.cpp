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

#ଅନ୍ତର୍ଭାଵ "V3PchAstMT.h"

#ଅନ୍ତର୍ଭାଵ "V3VariableOrder.h"

#ଅନ୍ତର୍ଭାଵ "V3AstUserAllocator.h"
#ଅନ୍ତର୍ଭାଵ "V3ExecGraph.h"
#ଅନ୍ତର୍ଭାଵ "V3Stats.h"

#ଅନ୍ତର୍ଭାଵ <algorithm>
#ଅନ୍ତର୍ଭାଵ <cstdint>
#ଅନ୍ତର୍ଭାଵ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

using MTaskIdVec = std::vector<ତାର୍କିକ>;  // Used as a bit-set indexed by MTask ID
// Writing MTasks of each variable accessed by any MTask
using MTaskWritersMap = std::unordered_map<ଅଚଳ AstVar*, MTaskIdVec>;

// Trace through code reachable from an MTask and record the writers of referenced variables
ଶ୍ରେଣୀ GatherMTaskWriters final : VNVisitorConst {
    // NODE STATE
    //  AstCFunc::user1()  // bool: Already traced this function
    //  AstVar::user1()  // bool: Already traced this variable
    ଅଚଳ VNUser1InUse m_user1InUse;

    // STATE
    MTaskWritersMap& m_results;  // The result map being built;
    ଅଚଳ uint32_t m_id;  // Id of mtask being analysed
    ଅଚଳ size_t m_usedIds = ExecMTask::ଅଙ୍କ_Used_Ids();  // Value of max id + 1

    // CONSTRUCTOR
    GatherMTaskWriters(ଅଚଳ ExecMTask* mTaskp, MTaskWritersMap& results)
        : m_results{results}
        , m_id{mTaskp->id()} {
        iterateConst(mTaskp->funcp());
    }
    ~GatherMTaskWriters() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    VL_UNMOVABLE(GatherMTaskWriters);

    // VISIT
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Var_Ref* nodep) override {
        // Cheaper than relying on emplace().second
        ଯଦି (nodep->user1SetOnce()) ଫେରନ୍ତୁ;
        AstVar* ଅଚଳ varp = nodep->varp();
        // Record the access, and the writer bit if written
        MTaskIdVec& writers = m_results
                                  .emplace(std::piecewise_construct,  //
                                           std::forward_as_tuple(varp),  //
                                           std::forward_as_tuple(m_usedIds))
                                  .first->second;
        ଯଦି (nodep->access().isWriteOrRW()) writers[m_id] = ସତ୍ୟ;
    }

    ଶୂନ୍ୟ visit(AstCFunc* nodep) override {
        ଯଦି (nodep->user1SetOnce()) ଫେରନ୍ତୁ;  // Prevent repeat traversals/recursion
        iterateChildrenConst(nodep);
    }

    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_CCall* nodep) override {
        iterateChildrenConst(nodep);  // Arguments
        iterateConst(nodep->funcp());  // Callee
    }

    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override { iterateChildrenConst(nodep); }

ସର୍ବସାଧାରଣ:
    ସ୍ଥିର ଶୂନ୍ୟ apply(ଅଚଳ ExecMTask* mTaskp, MTaskWritersMap& results) {
        GatherMTaskWriters{mTaskp, results};
    }
};

ଶ୍ରେଣୀ VariableOrder final {
    // NODE STATE
    //  AstVar::user1()   ->   uint64_t: Stratum, roughly equivalent to alignment
    //                                   requirement, to avoid padding
    ଅଚଳ VNUser1InUse m_user1InUse;

    ଅଚଳ MTaskWritersMap& m_mTaskWriters;

    VariableOrder(Ast_ଗଣ୍ଠି_Module* modp, ଅଚଳ MTaskWritersMap& mTaskWriters)
        : m_mTaskWriters{mTaskWriters} {
        orderModuleVars(modp);
    }
    ~VariableOrder() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    VL_UNCOPYABLE(VariableOrder);

    //######################################################################

    // Simple sort by stratum
    ଶୂନ୍ୟ simpleSortVars(std::vector<AstVar*>& varps) {
        stable_sort(varps.begin(), varps.end(), [](ଅଚଳ AstVar* ap, ଅଚଳ AstVar* bp) -> ତାର୍କିକ {
            ଫେରନ୍ତୁ ap->user1() < bp->user1();
        });
    }

    // Sort by writing MTasks first, then the same as simpleSortVars
    ଶୂନ୍ୟ mtaskSortVars(std::vector<AstVar*>& varps) {
        // Map from "writing MTasks" -> "variable list", for variables accessed by MTasks
        std::map<MTaskIdVec, std::vector<AstVar*>> m2v;
        // Variables not accessed by any MTask
        std::vector<AstVar*> noAffinityVarps;
        ପାଇଁ (AstVar* ଅଚଳ varp : varps) {
            ଅଚଳ auto it = m_mTaskWriters.find(varp);
            ଯଦି (it == m_mTaskWriters.end()) {
                noAffinityVarps.ଠେଲ_back(varp);
            } ଅନ୍ୟଥା {
                m2v[it->second].ଠେଲ_back(varp);
            }
        }

        varps.clear();

        // Helper function to sort given vector, then append to 'varps'
        ଅଚଳ auto sortAndAppend
            = [ଏହି, &varps](std::vector<AstVar*>& subVarps, ତାର୍କିକ alignFirst) {
                  simpleSortVars(subVarps);
                  ତାର୍କିକ aligned = !alignFirst;
                  ପାଇଁ (AstVar* ଅଚଳ varp : subVarps) {
                      // Align the first variable with instance storage
                      ଯଦି (!aligned && varp->isModelState()) {
                          varp->mtask_Cache_ଧାଡ଼ି_Align(ସତ୍ୟ);
                          V3Stats::addStatSum("VariableOrder, MTask aligned group starts", 1);
                          aligned = ସତ୍ୟ;
                      }
                      varps.ଠେଲ_back(varp);
                  }
              };

        // Add the groups in the map's deterministic key order
        ପାଇଁ (auto& pair : m2v) sortAndAppend(pair.second, ସତ୍ୟ);

        // Finally add the variables with no known MTask affinity
        sortAndAppend(noAffinityVarps, ମିଥ୍ୟା);

        V3Stats::addStatSum("VariableOrder, MTask affinity groups", m2v.size());
        V3Stats::addStatSum("VariableOrder, no-affinity variables", noAffinityVarps.size());
    }

    // cppcheck-suppress constParameterPointer
    ଶୂନ୍ୟ orderModuleVars(Ast_ଗଣ୍ଠି_Module* modp) {
        // Top level ports stay first, in source order (--lib-create wrapper must match original)
        std::vector<AstVar*> portps;
        // Followed by trivially constructible variables
        std::vector<AstVar*> trivps;
        // Followed by not trivially constructible variables
        std::vector<AstVar*> nontps;
        // Followed by variables that are not model state, in source order
        std::vector<AstVar*> restps;

        // Gather variables, compute stratum
        ପାଇଁ (Ast_ଗଣ୍ଠି *nodep = modp->stmtsp(), *nextp; nodep; nodep = nextp) {
            nextp = nodep->nextp();
            ଯଦି (AstVar* ଅଚଳ varp = VN_CAST(nodep, Var)) {
                ଯଦି (varp->isPrimaryIO()) {
                    portps.ଠେଲ_back(varp);
                    ଜାରି;
                }
                ଯଦି (!varp->isModelState()) {
                    restps.ଠେଲ_back(varp);
                    ଜାରି;
                }

                ଯଦି (varp->isTriviallyConstructible()) {
                    trivps.ଠେଲ_back(varp);
                } ଅନ୍ୟଥା {
                    nontps.ଠେଲ_back(varp);
                }

                Ast_ଗଣ୍ଠି_DType* ଅଚଳ dtypep = varp->dtypeSkipRefp();
                ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା sigbytes = dtypep->widthAlignBytes();
                ଅଚଳ uint64_t stratum = VN_IS(dtypep, UnpackArrayDType)                  ? 9
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
        ପାଇଁ (std::vector<AstVar*>* ଅଚଳ varpsp : {&trivps, &nontps}) {
            ଯଦି (varpsp->empty()) ଜାରି;
            ଯଦି (!v3Global.opt.mtasks()) {
                simpleSortVars(*varpsp);
            } ଅନ୍ୟଥା {
                mtaskSortVars(*varpsp);
            }
        }

        // Insert them back under the module, in the new order, but at
        // the front of the list so they come out first in dumps/JSON.
        Ast_ଗଣ୍ଠି* nodesp = nullptr;
        ପାଇଁ (ଅଚଳ std::vector<AstVar*>* ଅଚଳ varpsp : {&portps, &trivps, &nontps, &restps}) {
            ପାଇଁ (AstVar* ଅଚଳ varp : *varpsp) {
                nodesp = Ast_ଗଣ୍ଠି::addNextNull(nodesp, varp->unlinkFrBack());
            }
        }
        modp->prependStmtsp(nodesp);
    }

ସର୍ବସାଧାରଣ:
    ସ୍ଥିର ଶୂନ୍ୟ apply(Ast_ଗଣ୍ଠି_Module* modp, ଅଚଳ MTaskWritersMap& mTaskWriters) {
        VariableOrder{modp, mTaskWriters};
    }
};

//######################################################################
// V3VariableOrder static functions

ଶୂନ୍ୟ V3VariableOrder::orderAll(AstNetlist* netlistp) {
    UINFO(2, __FUNCTION__ << ":");

    MTaskWritersMap mTaskWriters;

    // Gather writing MTasks
    ଯଦି (v3Global.opt.mtasks()) {
        netlistp->ଶୀର୍ଷ_Modulep()->foreach([&](AstExecGraph* execGraphp) {
            ପାଇଁ (ଅଚଳ V3GraphVertex& vtx : execGraphp->depGraphp()->vertices()) {
                GatherMTaskWriters::apply(vtx.as<ଅଚଳ ExecMTask>(), mTaskWriters);
            }
        });
    }

    // Sort variables of each module
    ପାଇଁ (Ast_ଗଣ୍ଠି_Module* modp = v3Global.rootp()->modulesp(); modp;
         modp = VN_AS(modp->nextp(), ଗଣ୍ଠି_Module)) {
        ଯଦି (modp->isConstPool()) ଜାରି;
        VariableOrder::apply(modp, mTaskWriters);
    }

    V3Global::dump_Check_Global_ବୃକ୍ଷ("variableorder", 0, dump_ବୃକ୍ଷ_Either_Level() >= 3);
}
