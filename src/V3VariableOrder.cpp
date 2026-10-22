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

#شامل "V3PchAstMT.h"

#شامل "V3VariableOrder.h"

#شامل "V3AstUserAllocator.h"
#شامل "V3ExecGraph.h"
#شامل "V3Stats.h"

#شامل <algorithm>
#شامل <cstdint>
#شامل <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

using MTaskIdVec = std::vector<منطقی>;  // Used as a bit-set indexed by MTask ID
// Writing MTasks of each variable accessed by any MTask
using MTaskWritersMap = std::unordered_map<ثابت AstVar*, MTaskIdVec>;

// Trace through code reachable from an MTask and record the writers of referenced variables
رده GatherMTaskWriters final : VNVisitorConst {
    // NODE STATE
    //  AstCFunc::user1()  // bool: Already traced this function
    //  AstVar::user1()  // bool: Already traced this variable
    ثابت VNUser1InUse m_user1InUse;

    // STATE
    MTaskWritersMap& m_results;  // The result map being built;
    ثابت uint32_t m_id;  // Id of mtask being analysed
    ثابت size_t m_usedIds = ExecMTask::رقم_Used_Ids();  // Value of max id + 1

    // CONSTRUCTOR
    GatherMTaskWriters(ثابت ExecMTask* mTaskp, MTaskWritersMap& results)
        : m_results{results}
        , m_id{mTaskp->id()} {
        iterateConst(mTaskp->funcp());
    }
    ~GatherMTaskWriters() = پیشفرض;
    VL_UNMOVABLE(GatherMTaskWriters);

    // VISIT
    تهی visit(Ast_گره_Var_Ref* nodep) override {
        // Cheaper than relying on emplace().second
        اگر (nodep->user1SetOnce()) بازگشت;
        AstVar* ثابت varp = nodep->varp();
        // Record the access, and the writer bit if written
        MTaskIdVec& writers = m_results
                                  .emplace(std::piecewise_construct,  //
                                           std::forward_as_tuple(varp),  //
                                           std::forward_as_tuple(m_usedIds))
                                  .first->second;
        اگر (nodep->access().isWriteOrRW()) writers[m_id] = درست;
    }

    تهی visit(AstCFunc* nodep) override {
        اگر (nodep->user1SetOnce()) بازگشت;  // Prevent repeat traversals/recursion
        iterateChildrenConst(nodep);
    }

    تهی visit(Ast_گره_CCall* nodep) override {
        iterateChildrenConst(nodep);  // Arguments
        iterateConst(nodep->funcp());  // Callee
    }

    تهی visit(Ast_گره* nodep) override { iterateChildrenConst(nodep); }

عمومی:
    ایستا تهی apply(ثابت ExecMTask* mTaskp, MTaskWritersMap& results) {
        GatherMTaskWriters{mTaskp, results};
    }
};

رده VariableOrder final {
    // NODE STATE
    //  AstVar::user1()   ->   uint64_t: Stratum, roughly equivalent to alignment
    //                                   requirement, to avoid padding
    ثابت VNUser1InUse m_user1InUse;

    ثابت MTaskWritersMap& m_mTaskWriters;

    VariableOrder(Ast_گره_Module* modp, ثابت MTaskWritersMap& mTaskWriters)
        : m_mTaskWriters{mTaskWriters} {
        orderModuleVars(modp);
    }
    ~VariableOrder() = پیشفرض;
    VL_UNCOPYABLE(VariableOrder);

    //######################################################################

    // Simple sort by stratum
    تهی simpleSortVars(std::vector<AstVar*>& varps) {
        stable_sort(varps.begin(), varps.end(), [](ثابت AstVar* ap, ثابت AstVar* bp) -> منطقی {
            بازگشت ap->user1() < bp->user1();
        });
    }

    // Sort by writing MTasks first, then the same as simpleSortVars
    تهی mtaskSortVars(std::vector<AstVar*>& varps) {
        // Map from "writing MTasks" -> "variable list", for variables accessed by MTasks
        std::map<MTaskIdVec, std::vector<AstVar*>> m2v;
        // Variables not accessed by any MTask
        std::vector<AstVar*> noAffinityVarps;
        برای (AstVar* ثابت varp : varps) {
            ثابت auto it = m_mTaskWriters.find(varp);
            اگر (it == m_mTaskWriters.end()) {
                noAffinityVarps.افزودن_back(varp);
            } وگرنه {
                m2v[it->second].افزودن_back(varp);
            }
        }

        varps.clear();

        // Helper function to sort given vector, then append to 'varps'
        ثابت auto sortAndAppend
            = [این, &varps](std::vector<AstVar*>& subVarps, منطقی alignFirst) {
                  simpleSortVars(subVarps);
                  منطقی aligned = !alignFirst;
                  برای (AstVar* ثابت varp : subVarps) {
                      // Align the first variable with instance storage
                      اگر (!aligned && varp->isModelState()) {
                          varp->mtask_Cache_سطر_Align(درست);
                          V3Stats::addStatSum("VariableOrder, MTask aligned group starts", 1);
                          aligned = درست;
                      }
                      varps.افزودن_back(varp);
                  }
              };

        // Add the groups in the map's deterministic key order
        برای (auto& pair : m2v) sortAndAppend(pair.second, درست);

        // Finally add the variables with no known MTask affinity
        sortAndAppend(noAffinityVarps, نادرست);

        V3Stats::addStatSum("VariableOrder, MTask affinity groups", m2v.size());
        V3Stats::addStatSum("VariableOrder, no-affinity variables", noAffinityVarps.size());
    }

    // cppcheck-suppress constParameterPointer
    تهی orderModuleVars(Ast_گره_Module* modp) {
        // Top level ports stay first, in source order (--lib-create wrapper must match original)
        std::vector<AstVar*> portps;
        // Followed by trivially constructible variables
        std::vector<AstVar*> trivps;
        // Followed by not trivially constructible variables
        std::vector<AstVar*> nontps;
        // Followed by variables that are not model state, in source order
        std::vector<AstVar*> restps;

        // Gather variables, compute stratum
        برای (Ast_گره *nodep = modp->stmtsp(), *nextp; nodep; nodep = nextp) {
            nextp = nodep->nextp();
            اگر (AstVar* ثابت varp = VN_CAST(nodep, Var)) {
                اگر (varp->isPrimaryIO()) {
                    portps.افزودن_back(varp);
                    ادامه;
                }
                اگر (!varp->isModelState()) {
                    restps.افزودن_back(varp);
                    ادامه;
                }

                اگر (varp->isTriviallyConstructible()) {
                    trivps.افزودن_back(varp);
                } وگرنه {
                    nontps.افزودن_back(varp);
                }

                Ast_گره_DType* ثابت dtypep = varp->dtypeSkipRefp();
                ثابت صحیح sigbytes = dtypep->widthAlignBytes();
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
        برای (std::vector<AstVar*>* ثابت varpsp : {&trivps, &nontps}) {
            اگر (varpsp->empty()) ادامه;
            اگر (!v3Global.opt.mtasks()) {
                simpleSortVars(*varpsp);
            } وگرنه {
                mtaskSortVars(*varpsp);
            }
        }

        // Insert them back under the module, in the new order, but at
        // the front of the list so they come out first in dumps/JSON.
        Ast_گره* nodesp = nullptr;
        برای (ثابت std::vector<AstVar*>* ثابت varpsp : {&portps, &trivps, &nontps, &restps}) {
            برای (AstVar* ثابت varp : *varpsp) {
                nodesp = Ast_گره::addNextNull(nodesp, varp->unlinkFrBack());
            }
        }
        modp->prependStmtsp(nodesp);
    }

عمومی:
    ایستا تهی apply(Ast_گره_Module* modp, ثابت MTaskWritersMap& mTaskWriters) {
        VariableOrder{modp, mTaskWriters};
    }
};

//######################################################################
// V3VariableOrder static functions

تهی V3VariableOrder::orderAll(AstNetlist* netlistp) {
    UINFO(2, __FUNCTION__ << ":");

    MTaskWritersMap mTaskWriters;

    // Gather writing MTasks
    اگر (v3Global.opt.mtasks()) {
        netlistp->بالاترین_Modulep()->foreach([&](AstExecGraph* execGraphp) {
            برای (ثابت V3GraphVertex& vtx : execGraphp->depGraphp()->vertices()) {
                GatherMTaskWriters::apply(vtx.as<ثابت ExecMTask>(), mTaskWriters);
            }
        });
    }

    // Sort variables of each module
    برای (Ast_گره_Module* modp = v3Global.rootp()->modulesp(); modp;
         modp = VN_AS(modp->nextp(), گره_Module)) {
        اگر (modp->isConstPool()) ادامه;
        VariableOrder::apply(modp, mTaskWriters);
    }

    V3Global::dump_Check_Global_درخت("variableorder", 0, dump_درخت_Either_Level() >= 3);
}
