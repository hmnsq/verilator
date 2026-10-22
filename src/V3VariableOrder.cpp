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

#അന്തര്ഭാവ "V3PchAstMT.h"

#അന്തര്ഭാവ "V3VariableOrder.h"

#അന്തര്ഭാവ "V3AstUserAllocator.h"
#അന്തര്ഭാവ "V3ExecGraph.h"
#അന്തര്ഭാവ "V3Stats.h"

#അന്തര്ഭാവ <algorithm>
#അന്തര്ഭാവ <cstdint>
#അന്തര്ഭാവ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

using MTaskIdVec = std::vector<തർക്കം>;  // Used as a bit-set indexed by MTask ID
// Writing MTasks of each variable accessed by any MTask
using MTaskWritersMap = std::unordered_map<സ്ഥിരം AstVar*, MTaskIdVec>;

// Trace through code reachable from an MTask and record the writers of referenced variables
വർഗ്ഗം GatherMTaskWriters final : VNVisitorConst {
    // NODE STATE
    //  AstCFunc::user1()  // bool: Already traced this function
    //  AstVar::user1()  // bool: Already traced this variable
    സ്ഥിരം VNUser1InUse m_user1InUse;

    // STATE
    MTaskWritersMap& m_results;  // The result map being built;
    സ്ഥിരം uint32_t m_id;  // Id of mtask being analysed
    സ്ഥിരം size_t m_usedIds = ExecMTask::അക്കം_Used_Ids();  // Value of max id + 1

    // CONSTRUCTOR
    GatherMTaskWriters(സ്ഥിരം ExecMTask* mTaskp, MTaskWritersMap& results)
        : m_results{results}
        , m_id{mTaskp->id()} {
        iterateConst(mTaskp->funcp());
    }
    ~GatherMTaskWriters() = സ്ഥിരസ്ഥിതി;
    VL_UNMOVABLE(GatherMTaskWriters);

    // VISIT
    ശൂന്യം visit(Ast_കണ്ണി_Var_Ref* nodep) override {
        // Cheaper than relying on emplace().second
        എങ്കിൽ (nodep->user1SetOnce()) തിരികെ;
        AstVar* സ്ഥിരം varp = nodep->varp();
        // Record the access, and the writer bit if written
        MTaskIdVec& writers = m_results
                                  .emplace(std::piecewise_construct,  //
                                           std::forward_as_tuple(varp),  //
                                           std::forward_as_tuple(m_usedIds))
                                  .first->second;
        എങ്കിൽ (nodep->access().isWriteOrRW()) writers[m_id] = സത്യം;
    }

    ശൂന്യം visit(AstCFunc* nodep) override {
        എങ്കിൽ (nodep->user1SetOnce()) തിരികെ;  // Prevent repeat traversals/recursion
        iterateChildrenConst(nodep);
    }

    ശൂന്യം visit(Ast_കണ്ണി_CCall* nodep) override {
        iterateChildrenConst(nodep);  // Arguments
        iterateConst(nodep->funcp());  // Callee
    }

    ശൂന്യം visit(Ast_കണ്ണി* nodep) override { iterateChildrenConst(nodep); }

പൊതു:
    സ്ഥിതം ശൂന്യം apply(സ്ഥിരം ExecMTask* mTaskp, MTaskWritersMap& results) {
        GatherMTaskWriters{mTaskp, results};
    }
};

വർഗ്ഗം VariableOrder final {
    // NODE STATE
    //  AstVar::user1()   ->   uint64_t: Stratum, roughly equivalent to alignment
    //                                   requirement, to avoid padding
    സ്ഥിരം VNUser1InUse m_user1InUse;

    സ്ഥിരം MTaskWritersMap& m_mTaskWriters;

    VariableOrder(Ast_കണ്ണി_Module* modp, സ്ഥിരം MTaskWritersMap& mTaskWriters)
        : m_mTaskWriters{mTaskWriters} {
        orderModuleVars(modp);
    }
    ~VariableOrder() = സ്ഥിരസ്ഥിതി;
    VL_UNCOPYABLE(VariableOrder);

    //######################################################################

    // Simple sort by stratum
    ശൂന്യം simpleSortVars(std::vector<AstVar*>& varps) {
        stable_sort(varps.begin(), varps.end(), [](സ്ഥിരം AstVar* ap, സ്ഥിരം AstVar* bp) -> തർക്കം {
            തിരികെ ap->user1() < bp->user1();
        });
    }

    // Sort by writing MTasks first, then the same as simpleSortVars
    ശൂന്യം mtaskSortVars(std::vector<AstVar*>& varps) {
        // Map from "writing MTasks" -> "variable list", for variables accessed by MTasks
        std::map<MTaskIdVec, std::vector<AstVar*>> m2v;
        // Variables not accessed by any MTask
        std::vector<AstVar*> noAffinityVarps;
        വേണ്ടി (AstVar* സ്ഥിരം varp : varps) {
            സ്ഥിരം auto it = m_mTaskWriters.find(varp);
            എങ്കിൽ (it == m_mTaskWriters.end()) {
                noAffinityVarps.തള്ളുക_back(varp);
            } അല്ലെങ്കിൽ {
                m2v[it->second].തള്ളുക_back(varp);
            }
        }

        varps.clear();

        // Helper function to sort given vector, then append to 'varps'
        സ്ഥിരം auto sortAndAppend
            = [ഇത്, &varps](std::vector<AstVar*>& subVarps, തർക്കം alignFirst) {
                  simpleSortVars(subVarps);
                  തർക്കം aligned = !alignFirst;
                  വേണ്ടി (AstVar* സ്ഥിരം varp : subVarps) {
                      // Align the first variable with instance storage
                      എങ്കിൽ (!aligned && varp->isModelState()) {
                          varp->mtask_Cache_വരി_Align(സത്യം);
                          V3Stats::addStatSum("VariableOrder, MTask aligned group starts", 1);
                          aligned = സത്യം;
                      }
                      varps.തള്ളുക_back(varp);
                  }
              };

        // Add the groups in the map's deterministic key order
        വേണ്ടി (auto& pair : m2v) sortAndAppend(pair.second, സത്യം);

        // Finally add the variables with no known MTask affinity
        sortAndAppend(noAffinityVarps, അസത്യം);

        V3Stats::addStatSum("VariableOrder, MTask affinity groups", m2v.size());
        V3Stats::addStatSum("VariableOrder, no-affinity variables", noAffinityVarps.size());
    }

    // cppcheck-suppress constParameterPointer
    ശൂന്യം orderModuleVars(Ast_കണ്ണി_Module* modp) {
        // Top level ports stay first, in source order (--lib-create wrapper must match original)
        std::vector<AstVar*> portps;
        // Followed by trivially constructible variables
        std::vector<AstVar*> trivps;
        // Followed by not trivially constructible variables
        std::vector<AstVar*> nontps;
        // Followed by variables that are not model state, in source order
        std::vector<AstVar*> restps;

        // Gather variables, compute stratum
        വേണ്ടി (Ast_കണ്ണി *nodep = modp->stmtsp(), *nextp; nodep; nodep = nextp) {
            nextp = nodep->nextp();
            എങ്കിൽ (AstVar* സ്ഥിരം varp = VN_CAST(nodep, Var)) {
                എങ്കിൽ (varp->isPrimaryIO()) {
                    portps.തള്ളുക_back(varp);
                    തുടരുക;
                }
                എങ്കിൽ (!varp->isModelState()) {
                    restps.തള്ളുക_back(varp);
                    തുടരുക;
                }

                എങ്കിൽ (varp->isTriviallyConstructible()) {
                    trivps.തള്ളുക_back(varp);
                } അല്ലെങ്കിൽ {
                    nontps.തള്ളുക_back(varp);
                }

                Ast_കണ്ണി_DType* സ്ഥിരം dtypep = varp->dtypeSkipRefp();
                സ്ഥിരം പൂർണ്ണസംഖ്യ sigbytes = dtypep->widthAlignBytes();
                സ്ഥിരം uint64_t stratum = VN_IS(dtypep, UnpackArrayDType)                  ? 9
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
        വേണ്ടി (std::vector<AstVar*>* സ്ഥിരം varpsp : {&trivps, &nontps}) {
            എങ്കിൽ (varpsp->empty()) തുടരുക;
            എങ്കിൽ (!v3Global.opt.mtasks()) {
                simpleSortVars(*varpsp);
            } അല്ലെങ്കിൽ {
                mtaskSortVars(*varpsp);
            }
        }

        // Insert them back under the module, in the new order, but at
        // the front of the list so they come out first in dumps/JSON.
        Ast_കണ്ണി* nodesp = nullptr;
        വേണ്ടി (സ്ഥിരം std::vector<AstVar*>* സ്ഥിരം varpsp : {&portps, &trivps, &nontps, &restps}) {
            വേണ്ടി (AstVar* സ്ഥിരം varp : *varpsp) {
                nodesp = Ast_കണ്ണി::addNextNull(nodesp, varp->unlinkFrBack());
            }
        }
        modp->prependStmtsp(nodesp);
    }

പൊതു:
    സ്ഥിതം ശൂന്യം apply(Ast_കണ്ണി_Module* modp, സ്ഥിരം MTaskWritersMap& mTaskWriters) {
        VariableOrder{modp, mTaskWriters};
    }
};

//######################################################################
// V3VariableOrder static functions

ശൂന്യം V3VariableOrder::orderAll(AstNetlist* netlistp) {
    UINFO(2, __FUNCTION__ << ":");

    MTaskWritersMap mTaskWriters;

    // Gather writing MTasks
    എങ്കിൽ (v3Global.opt.mtasks()) {
        netlistp->മുകൾ_Modulep()->foreach([&](AstExecGraph* execGraphp) {
            വേണ്ടി (സ്ഥിരം V3GraphVertex& vtx : execGraphp->depGraphp()->vertices()) {
                GatherMTaskWriters::apply(vtx.as<സ്ഥിരം ExecMTask>(), mTaskWriters);
            }
        });
    }

    // Sort variables of each module
    വേണ്ടി (Ast_കണ്ണി_Module* modp = v3Global.rootp()->modulesp(); modp;
         modp = VN_AS(modp->nextp(), കണ്ണി_Module)) {
        എങ്കിൽ (modp->isConstPool()) തുടരുക;
        VariableOrder::apply(modp, mTaskWriters);
    }

    V3Global::dump_Check_Global_വൃക്ഷം("variableorder", 0, dump_വൃക്ഷം_Either_Level() >= 3);
}
