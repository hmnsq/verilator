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

#समावेश "V3PchAstMT.h"

#समावेश "V3VariableOrder.h"

#समावेश "V3AstUserAllocator.h"
#समावेश "V3ExecGraph.h"
#समावेश "V3Stats.h"

#समावेश <algorithm>
#समावेश <cstdint>
#समावेश <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

using MTaskIdVec = std::vector<bool>;  // Used as a bit-set indexed by MTask ID
// Writing MTasks of each variable accessed by any MTask
using MTaskWritersMap = std::unordered_map<स्थिर AstVar*, MTaskIdVec>;

// Trace through code reachable from an MTask and record the writers of referenced variables
श्रेणी GatherMTaskWriters final : VNVisitorConst {
    // NODE STATE
    //  AstCFunc::user1()  // bool: Already traced this function
    //  AstVar::user1()  // bool: Already traced this variable
    स्थिर VNUser1InUse m_user1InUse;

    // STATE
    MTaskWritersMap& m_results;  // The result map being built;
    स्थिर uint32_t m_id;  // Id of mtask being analysed
    स्थिर माप_प्रकार m_usedIds = ExecMTask::अंक_Used_Ids();  // Value of max id + 1

    // CONSTRUCTOR
    GatherMTaskWriters(स्थिर ExecMTask* mTaskp, MTaskWritersMap& results)
        : m_results{results}
        , m_id{mTaskp->id()} {
        iterateConst(mTaskp->funcp());
    }
    ~GatherMTaskWriters() = शेष;
    VL_UNMOVABLE(GatherMTaskWriters);

    // VISIT
    व्योम visit(Ast_गाँठ_Var_Ref* nodep) override {
        // Cheaper than relying on emplace().second
        अगर (nodep->user1SetOnce()) वापस;
        AstVar* स्थिर varp = nodep->varp();
        // Record the access, and the writer bit if written
        MTaskIdVec& writers = m_results
                                  .emplace(std::piecewise_construct,  //
                                           std::forward_as_tuple(varp),  //
                                           std::forward_as_tuple(m_usedIds))
                                  .first->second;
        अगर (nodep->access().isWriteOrRW()) writers[m_id] = true;
    }

    व्योम visit(AstCFunc* nodep) override {
        अगर (nodep->user1SetOnce()) वापस;  // Prevent repeat traversals/recursion
        iterateChildrenConst(nodep);
    }

    व्योम visit(Ast_गाँठ_CCall* nodep) override {
        iterateChildrenConst(nodep);  // Arguments
        iterateConst(nodep->funcp());  // Callee
    }

    व्योम visit(Ast_गाँठ* nodep) override { iterateChildrenConst(nodep); }

खुला:
    अटल व्योम apply(स्थिर ExecMTask* mTaskp, MTaskWritersMap& results) {
        GatherMTaskWriters{mTaskp, results};
    }
};

श्रेणी VariableOrder final {
    // NODE STATE
    //  AstVar::user1()   ->   uint64_t: Stratum, roughly equivalent to alignment
    //                                   requirement, to avoid padding
    स्थिर VNUser1InUse m_user1InUse;

    स्थिर MTaskWritersMap& m_mTaskWriters;

    VariableOrder(Ast_गाँठ_Module* modp, स्थिर MTaskWritersMap& mTaskWriters)
        : m_mTaskWriters{mTaskWriters} {
        orderModuleVars(modp);
    }
    ~VariableOrder() = शेष;
    VL_UNCOPYABLE(VariableOrder);

    //######################################################################

    // Simple sort by stratum
    व्योम simpleSortVars(std::vector<AstVar*>& varps) {
        stable_sort(varps.begin(), varps.end(), [](स्थिर AstVar* ap, स्थिर AstVar* bp) -> bool {
            वापस ap->user1() < bp->user1();
        });
    }

    // Sort by writing MTasks first, then the same as simpleSortVars
    व्योम mtaskSortVars(std::vector<AstVar*>& varps) {
        // Map from "writing MTasks" -> "variable list", for variables accessed by MTasks
        std::map<MTaskIdVec, std::vector<AstVar*>> m2v;
        // Variables not accessed by any MTask
        std::vector<AstVar*> noAffinityVarps;
        क्रम (AstVar* स्थिर varp : varps) {
            स्थिर स्वतः it = m_mTaskWriters.find(varp);
            अगर (it == m_mTaskWriters.end()) {
                noAffinityVarps.धकेलो_back(varp);
            } अन्यथा {
                m2v[it->second].धकेलो_back(varp);
            }
        }

        varps.सफा();

        // Helper function to sort given vector, then append to 'varps'
        स्थिर स्वतः sortAndAppend
            = [यह, &varps](std::vector<AstVar*>& subVarps, bool alignFirst) {
                  simpleSortVars(subVarps);
                  bool aligned = !alignFirst;
                  क्रम (AstVar* स्थिर varp : subVarps) {
                      // Align the first variable with instance storage
                      अगर (!aligned && varp->isModelState()) {
                          varp->mtask_Cache_पंक्ति_Align(true);
                          V3Stats::add_Stat_योग("VariableOrder, MTask aligned group starts", 1);
                          aligned = true;
                      }
                      varps.धकेलो_back(varp);
                  }
              };

        // Add the groups in the map's deterministic key order
        क्रम (स्वतः& pair : m2v) sortAndAppend(pair.second, true);

        // Finally add the variables with no known MTask affinity
        sortAndAppend(noAffinityVarps, false);

        V3Stats::add_Stat_योग("VariableOrder, MTask affinity groups", m2v.आकार());
        V3Stats::add_Stat_योग("VariableOrder, no-affinity variables", noAffinityVarps.आकार());
    }

    // cppcheck-suppress constParameterPointer
    व्योम orderModuleVars(Ast_गाँठ_Module* modp) {
        // Top level ports stay first, in source order (--lib-create wrapper must match original)
        std::vector<AstVar*> portps;
        // Followed by trivially constructible variables
        std::vector<AstVar*> trivps;
        // Followed by not trivially constructible variables
        std::vector<AstVar*> nontps;
        // Followed by variables that are not model state, in source order
        std::vector<AstVar*> restps;

        // Gather variables, compute stratum
        क्रम (Ast_गाँठ *nodep = modp->stmtsp(), *nextp; nodep; nodep = nextp) {
            nextp = nodep->nextp();
            अगर (AstVar* स्थिर varp = VN_CAST(nodep, Var)) {
                अगर (varp->isPrimaryIO()) {
                    portps.धकेलो_back(varp);
                    जारी;
                }
                अगर (!varp->isModelState()) {
                    restps.धकेलो_back(varp);
                    जारी;
                }

                अगर (varp->isTriviallyConstructible()) {
                    trivps.धकेलो_back(varp);
                } अन्यथा {
                    nontps.धकेलो_back(varp);
                }

                Ast_गाँठ_DType* स्थिर dtypep = varp->dtypeSkipRefp();
                स्थिर पूर्णांक sigbytes = dtypep->widthAlignBytes();
                स्थिर uint64_t stratum = VN_IS(dtypep, UnpackArrayDType)                  ? 9
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
        क्रम (std::vector<AstVar*>* स्थिर varpsp : {&trivps, &nontps}) {
            अगर (varpsp->empty()) जारी;
            अगर (!v3Global.opt.mtasks()) {
                simpleSortVars(*varpsp);
            } अन्यथा {
                mtaskSortVars(*varpsp);
            }
        }

        // Insert them back under the module, in the new order, but at
        // the front of the list so they come out first in dumps/JSON.
        Ast_गाँठ* nodesp = nullptr;
        क्रम (स्थिर std::vector<AstVar*>* स्थिर varpsp : {&portps, &trivps, &nontps, &restps}) {
            क्रम (AstVar* स्थिर varp : *varpsp) {
                nodesp = Ast_गाँठ::add_अग्र_Null(nodesp, varp->unlinkFrBack());
            }
        }
        modp->prependStmtsp(nodesp);
    }

खुला:
    अटल व्योम apply(Ast_गाँठ_Module* modp, स्थिर MTaskWritersMap& mTaskWriters) {
        VariableOrder{modp, mTaskWriters};
    }
};

//######################################################################
// V3VariableOrder static functions

व्योम V3VariableOrder::orderAll(AstNetlist* netlistp) {
    UINFO(2, __FUNCTION__ << ":");

    MTaskWritersMap mTaskWriters;

    // Gather writing MTasks
    अगर (v3Global.opt.mtasks()) {
        netlistp->शीर्ष_Modulep()->foreach([&](AstExecGraph* execGraphp) {
            क्रम (स्थिर V3GraphVertex& vtx : execGraphp->depGraphp()->vertices()) {
                GatherMTaskWriters::apply(vtx.as<स्थिर ExecMTask>(), mTaskWriters);
            }
        });
    }

    // Sort variables of each module
    क्रम (Ast_गाँठ_Module* modp = v3Global.rootp()->modulesp(); modp;
         modp = VN_AS(modp->nextp(), गाँठ_Module)) {
        अगर (modp->isConstPool()) जारी;
        VariableOrder::apply(modp, mTaskWriters);
    }

    V3Global::dump_Check_Global_वृक्ष("variableorder", 0, dump_वृक्ष_Either_Level() >= 3);
}
