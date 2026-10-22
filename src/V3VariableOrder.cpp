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

#அந்தர்பாவ "V3PchAstMT.h"

#அந்தர்பாவ "V3VariableOrder.h"

#அந்தர்பாவ "V3AstUserAllocator.h"
#அந்தர்பாவ "V3ExecGraph.h"
#அந்தர்பாவ "V3Stats.h"

#அந்தர்பாவ <algorithm>
#அந்தர்பாவ <cstdint>
#அந்தர்பாவ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

using MTaskIdVec = std::vector<தருக்க>;  // Used as a bit-set indexed by MTask ID
// Writing MTasks of each variable accessed by any MTask
using MTaskWritersMap = std::unordered_map<மாறிலி AstVar*, MTaskIdVec>;

// Trace through code reachable from an MTask and record the writers of referenced variables
வகுப்பு GatherMTaskWriters final : VNVisitorConst {
    // NODE STATE
    //  AstCFunc::user1()  // bool: Already traced this function
    //  AstVar::user1()  // bool: Already traced this variable
    மாறிலி VNUser1InUse m_user1InUse;

    // STATE
    MTaskWritersMap& m_results;  // The result map being built;
    மாறிலி uint32_t m_id;  // Id of mtask being analysed
    மாறிலி size_t m_usedIds = ExecMTask::இலக்கம்_Used_Ids();  // Value of max id + 1

    // CONSTRUCTOR
    GatherMTaskWriters(மாறிலி ExecMTask* mTaskp, MTaskWritersMap& results)
        : m_results{results}
        , m_id{mTaskp->id()} {
        iterateConst(mTaskp->funcp());
    }
    ~GatherMTaskWriters() = இயல்பு;
    VL_UNMOVABLE(GatherMTaskWriters);

    // VISIT
    வெறுமை visit(Ast_முனை_Var_Ref* nodep) override {
        // Cheaper than relying on emplace().second
        எனில் (nodep->user1SetOnce()) திரும்பு;
        AstVar* மாறிலி varp = nodep->varp();
        // Record the access, and the writer bit if written
        MTaskIdVec& writers = m_results
                                  .emplace(std::piecewise_construct,  //
                                           std::forward_as_tuple(varp),  //
                                           std::forward_as_tuple(m_usedIds))
                                  .first->second;
        எனில் (nodep->access().isWriteOrRW()) writers[m_id] = உண்மை;
    }

    வெறுமை visit(AstCFunc* nodep) override {
        எனில் (nodep->user1SetOnce()) திரும்பு;  // Prevent repeat traversals/recursion
        iterateChildrenConst(nodep);
    }

    வெறுமை visit(Ast_முனை_CCall* nodep) override {
        iterateChildrenConst(nodep);  // Arguments
        iterateConst(nodep->funcp());  // Callee
    }

    வெறுமை visit(Ast_முனை* nodep) override { iterateChildrenConst(nodep); }

பொது:
    நிலையான வெறுமை apply(மாறிலி ExecMTask* mTaskp, MTaskWritersMap& results) {
        GatherMTaskWriters{mTaskp, results};
    }
};

வகுப்பு VariableOrder final {
    // NODE STATE
    //  AstVar::user1()   ->   uint64_t: Stratum, roughly equivalent to alignment
    //                                   requirement, to avoid padding
    மாறிலி VNUser1InUse m_user1InUse;

    மாறிலி MTaskWritersMap& m_mTaskWriters;

    VariableOrder(Ast_முனை_Module* modp, மாறிலி MTaskWritersMap& mTaskWriters)
        : m_mTaskWriters{mTaskWriters} {
        orderModuleVars(modp);
    }
    ~VariableOrder() = இயல்பு;
    VL_UNCOPYABLE(VariableOrder);

    //######################################################################

    // Simple sort by stratum
    வெறுமை simpleSortVars(std::vector<AstVar*>& varps) {
        stable_sort(varps.begin(), varps.end(), [](மாறிலி AstVar* ap, மாறிலி AstVar* bp) -> தருக்க {
            திரும்பு ap->user1() < bp->user1();
        });
    }

    // Sort by writing MTasks first, then the same as simpleSortVars
    வெறுமை mtaskSortVars(std::vector<AstVar*>& varps) {
        // Map from "writing MTasks" -> "variable list", for variables accessed by MTasks
        std::map<MTaskIdVec, std::vector<AstVar*>> m2v;
        // Variables not accessed by any MTask
        std::vector<AstVar*> noAffinityVarps;
        க்கு (AstVar* மாறிலி varp : varps) {
            மாறிலி auto it = m_mTaskWriters.find(varp);
            எனில் (it == m_mTaskWriters.end()) {
                noAffinityVarps.தள்ளு_back(varp);
            } இல்லையெனில் {
                m2v[it->second].தள்ளு_back(varp);
            }
        }

        varps.clear();

        // Helper function to sort given vector, then append to 'varps'
        மாறிலி auto sortAndAppend
            = [இது, &varps](std::vector<AstVar*>& subVarps, தருக்க alignFirst) {
                  simpleSortVars(subVarps);
                  தருக்க aligned = !alignFirst;
                  க்கு (AstVar* மாறிலி varp : subVarps) {
                      // Align the first variable with instance storage
                      எனில் (!aligned && varp->isModelState()) {
                          varp->mtask_Cache_வரி_Align(உண்மை);
                          V3Stats::addStatSum("VariableOrder, MTask aligned group starts", 1);
                          aligned = உண்மை;
                      }
                      varps.தள்ளு_back(varp);
                  }
              };

        // Add the groups in the map's deterministic key order
        க்கு (auto& pair : m2v) sortAndAppend(pair.second, உண்மை);

        // Finally add the variables with no known MTask affinity
        sortAndAppend(noAffinityVarps, பொய்);

        V3Stats::addStatSum("VariableOrder, MTask affinity groups", m2v.size());
        V3Stats::addStatSum("VariableOrder, no-affinity variables", noAffinityVarps.size());
    }

    // cppcheck-suppress constParameterPointer
    வெறுமை orderModuleVars(Ast_முனை_Module* modp) {
        // Top level ports stay first, in source order (--lib-create wrapper must match original)
        std::vector<AstVar*> portps;
        // Followed by trivially constructible variables
        std::vector<AstVar*> trivps;
        // Followed by not trivially constructible variables
        std::vector<AstVar*> nontps;
        // Followed by variables that are not model state, in source order
        std::vector<AstVar*> restps;

        // Gather variables, compute stratum
        க்கு (Ast_முனை *nodep = modp->stmtsp(), *nextp; nodep; nodep = nextp) {
            nextp = nodep->nextp();
            எனில் (AstVar* மாறிலி varp = VN_CAST(nodep, Var)) {
                எனில் (varp->isPrimaryIO()) {
                    portps.தள்ளு_back(varp);
                    தொடர்;
                }
                எனில் (!varp->isModelState()) {
                    restps.தள்ளு_back(varp);
                    தொடர்;
                }

                எனில் (varp->isTriviallyConstructible()) {
                    trivps.தள்ளு_back(varp);
                } இல்லையெனில் {
                    nontps.தள்ளு_back(varp);
                }

                Ast_முனை_DType* மாறிலி dtypep = varp->dtypeSkipRefp();
                மாறிலி முழு sigbytes = dtypep->widthAlignBytes();
                மாறிலி uint64_t stratum = VN_IS(dtypep, UnpackArrayDType)                  ? 9
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
        க்கு (std::vector<AstVar*>* மாறிலி varpsp : {&trivps, &nontps}) {
            எனில் (varpsp->empty()) தொடர்;
            எனில் (!v3Global.opt.mtasks()) {
                simpleSortVars(*varpsp);
            } இல்லையெனில் {
                mtaskSortVars(*varpsp);
            }
        }

        // Insert them back under the module, in the new order, but at
        // the front of the list so they come out first in dumps/JSON.
        Ast_முனை* nodesp = nullptr;
        க்கு (மாறிலி std::vector<AstVar*>* மாறிலி varpsp : {&portps, &trivps, &nontps, &restps}) {
            க்கு (AstVar* மாறிலி varp : *varpsp) {
                nodesp = Ast_முனை::addNextNull(nodesp, varp->unlinkFrBack());
            }
        }
        modp->prependStmtsp(nodesp);
    }

பொது:
    நிலையான வெறுமை apply(Ast_முனை_Module* modp, மாறிலி MTaskWritersMap& mTaskWriters) {
        VariableOrder{modp, mTaskWriters};
    }
};

//######################################################################
// V3VariableOrder static functions

வெறுமை V3VariableOrder::orderAll(AstNetlist* netlistp) {
    UINFO(2, __FUNCTION__ << ":");

    MTaskWritersMap mTaskWriters;

    // Gather writing MTasks
    எனில் (v3Global.opt.mtasks()) {
        netlistp->உச்சி_Modulep()->foreach([&](AstExecGraph* execGraphp) {
            க்கு (மாறிலி V3GraphVertex& vtx : execGraphp->depGraphp()->vertices()) {
                GatherMTaskWriters::apply(vtx.as<மாறிலி ExecMTask>(), mTaskWriters);
            }
        });
    }

    // Sort variables of each module
    க்கு (Ast_முனை_Module* modp = v3Global.rootp()->modulesp(); modp;
         modp = VN_AS(modp->nextp(), முனை_Module)) {
        எனில் (modp->isConstPool()) தொடர்;
        VariableOrder::apply(modp, mTaskWriters);
    }

    V3Global::dump_Check_Global_மரம்("variableorder", 0, dump_மரம்_Either_Level() >= 3);
}
