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

#ਅਨ੍ਤਰ੍ਭਾਵ "V3PchAstMT.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3VariableOrder.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3AstUserAllocator.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3ExecGraph.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Stats.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <algorithm>
#ਅਨ੍ਤਰ੍ਭਾਵ <cstdint>
#ਅਨ੍ਤਰ੍ਭਾਵ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

ਵਰਤੋਂ MTaskIdVec = std::ਸਦਿਸ਼<ਤਾਰਕਿਕ>;  // Used as a bit-set indexed by MTask ID
// Writing MTasks of each variable accessed by any MTask
ਵਰਤੋਂ MTaskWritersMap = std::unordered_map<ਅਚਲ AstVar*, MTaskIdVec>;

// Trace through code reachable from an MTask and record the writers of referenced variables
ਵਰਗ GatherMTaskWriters final : VNVisitorConst {
    // NODE STATE
    //  AstCFunc::user1()  // bool: Already traced this function
    //  AstVar::user1()  // bool: Already traced this variable
    ਅਚਲ VNUser1_ਵਿੱਚ_Use m_user1_ਵਿੱਚ_Use;

    // STATE
    MTaskWritersMap& m_results;  // The result map being built;
    ਅਚਲ uint32_t m_id;  // Id of mtask being analysed
    ਅਚਲ ਮਾਪ_t m_ਵਰਤਿਆ_Ids = ExecMTask::ਅੰਕ_ਵਰਤਿਆ_Ids();  // Value of max id + 1

    // CONSTRUCTOR
    GatherMTaskWriters(ਅਚਲ ExecMTask* mTaskp, MTaskWritersMap& results)
        : m_results{results}
        , m_id{mTaskp->id()} {
        iterateConst(mTaskp->funcp());
    }
    ~GatherMTaskWriters() = ਮੂਲ;
    VL_UNMOVABLE(GatherMTaskWriters);

    //ਦੇਖੋ
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Var_Ref* nodep) override {
        // Cheaper than relying on emplace().second
        ਜੇ (nodep->user1_ਸੈੱਟ_Once()) ਵਾਪਸ;
        AstVar* ਅਚਲ varp = nodep->varp();
        // Record the access, and the writer bit if written
        MTaskIdVec& writers = m_results
                                  .emplace(std::piecewise_construct,  //
                                           std::forward_as_tuple(varp),  //
                                           std::forward_as_tuple(m_ਵਰਤਿਆ_Ids))
                                  .ਪਹਿਲਾ->ਦੂਜਾ;
        ਜੇ (nodep->access().isWriteOrRW()) writers[m_id] = ਸੱਚ;
    }

    ਖਾਲੀ ਦੇਖੋ(AstCFunc* nodep) override {
        ਜੇ (nodep->user1_ਸੈੱਟ_Once()) ਵਾਪਸ;  // Prevent repeat traversals/recursion
        iterateChildrenConst(nodep);
    }

    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_CCall* nodep) override {
        iterateChildrenConst(nodep);  // Arguments
        iterateConst(nodep->funcp());  // Callee
    }

    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override { iterateChildrenConst(nodep); }

ਜਨਤਕ:
    ਸਥਿਰ ਖਾਲੀ apply(ਅਚਲ ExecMTask* mTaskp, MTaskWritersMap& results) {
        GatherMTaskWriters{mTaskp, results};
    }
};

ਵਰਗ Variable_ਕ੍ਰਮ final {
    // NODE STATE
    //  AstVar::user1()   ->   uint64_t: Stratum, roughly equivalent to alignment
    //                                   requirement, to avoid padding
    ਅਚਲ VNUser1_ਵਿੱਚ_Use m_user1_ਵਿੱਚ_Use;

    ਅਚਲ MTaskWritersMap& m_mTaskWriters;

    Variable_ਕ੍ਰਮ(Ast_ਗੰਢ_Module* modp, ਅਚਲ MTaskWritersMap& mTaskWriters)
        : m_mTaskWriters{mTaskWriters} {
        ਕ੍ਰਮ_Module_Vars(modp);
    }
    ~Variable_ਕ੍ਰਮ() = ਮੂਲ;
    VL_UNCOPYABLE(Variable_ਕ੍ਰਮ);

    //######################################################################

    // Simple sort by stratum
    ਖਾਲੀ simple_ਛਾਂਟੋ_Vars(std::ਸਦਿਸ਼<AstVar*>& varps) {
        stable_ਛਾਂਟੋ(varps.begin(), varps.ਸਿਰਾ(), [](ਅਚਲ AstVar* ap, ਅਚਲ AstVar* bp) -> ਤਾਰਕਿਕ {
            ਵਾਪਸ ap->user1() < bp->user1();
        });
    }

    // Sort by writing MTasks first, then the same as simpleSortVars
    ਖਾਲੀ mtask_ਛਾਂਟੋ_Vars(std::ਸਦਿਸ਼<AstVar*>& varps) {
        // Map from "writing MTasks" -> "variable list", for variables accessed by MTasks
        std::map<MTaskIdVec, std::ਸਦਿਸ਼<AstVar*>> m2v;
        // Variables not accessed by any MTask
        std::ਸਦਿਸ਼<AstVar*> noAffinityVarps;
        ਲਈ (AstVar* ਅਚਲ varp : varps) {
            ਅਚਲ ਸਵੈਚਾਲਿਤ it = m_mTaskWriters.ਲੱਭੋ(varp);
            ਜੇ (it == m_mTaskWriters.ਸਿਰਾ()) {
                noAffinityVarps.ਧੱਕੋ_ਪਿੱਠ(varp);
            } ਨਹੀਂਤਾਂ {
                m2v[it->ਦੂਜਾ].ਧੱਕੋ_ਪਿੱਠ(varp);
            }
        }

        varps.clear();

        // Helper function to sort given vector, then append to 'varps'
        ਅਚਲ ਸਵੈਚਾਲਿਤ ਛਾਂਟੋ_And_Append
            = [ਇਹ, &varps](std::ਸਦਿਸ਼<AstVar*>& subVarps, ਤਾਰਕਿਕ align_ਪਹਿਲਾ) {
                  simple_ਛਾਂਟੋ_Vars(subVarps);
                  ਤਾਰਕਿਕ aligned = !align_ਪਹਿਲਾ;
                  ਲਈ (AstVar* ਅਚਲ varp : subVarps) {
                      // Align the first variable with instance storage
                      ਜੇ (!aligned && varp->isModelState()) {
                          varp->mtask_Cache_ਸਤਰ_Align(ਸੱਚ);
                          V3Stats::ਜੋੜੋ_Stat_ਜੋੜ("VariableOrder, MTask aligned group starts", 1);
                          aligned = ਸੱਚ;
                      }
                      varps.ਧੱਕੋ_ਪਿੱਠ(varp);
                  }
              };

        // Add the groups in the map's deterministic key order
        ਲਈ (ਸਵੈਚਾਲਿਤ& pair : m2v) ਛਾਂਟੋ_And_Append(pair.ਦੂਜਾ, ਸੱਚ);

        // Finally add the variables with no known MTask affinity
        ਛਾਂਟੋ_And_Append(noAffinityVarps, ਝੂਠ);

        V3Stats::ਜੋੜੋ_Stat_ਜੋੜ("VariableOrder, MTask affinity groups", m2v.ਮਾਪ());
        V3Stats::ਜੋੜੋ_Stat_ਜੋੜ("VariableOrder, no-affinity variables", noAffinityVarps.ਮਾਪ());
    }

    // cppcheck-suppress constParameterPointer
    ਖਾਲੀ ਕ੍ਰਮ_Module_Vars(Ast_ਗੰਢ_Module* modp) {
        // Top level ports stay first, in source order (--lib-create wrapper must match original)
        std::ਸਦਿਸ਼<AstVar*> portps;
        // Followed by trivially constructible variables
        std::ਸਦਿਸ਼<AstVar*> trivps;
        // Followed by not trivially constructible variables
        std::ਸਦਿਸ਼<AstVar*> nontps;
        // Followed by variables that are not model state, in source order
        std::ਸਦਿਸ਼<AstVar*> restps;

        // Gather variables, compute stratum
        ਲਈ (Ast_ਗੰਢ *nodep = modp->stmtsp(), *nextp; nodep; nodep = nextp) {
            nextp = nodep->nextp();
            ਜੇ (AstVar* ਅਚਲ varp = VN_CAST(nodep, Var)) {
                ਜੇ (varp->isPrimaryIO()) {
                    portps.ਧੱਕੋ_ਪਿੱਠ(varp);
                    ਜਾਰੀ;
                }
                ਜੇ (!varp->isModelState()) {
                    restps.ਧੱਕੋ_ਪਿੱਠ(varp);
                    ਜਾਰੀ;
                }

                ਜੇ (varp->isTriviallyConstructible()) {
                    trivps.ਧੱਕੋ_ਪਿੱਠ(varp);
                } ਨਹੀਂਤਾਂ {
                    nontps.ਧੱਕੋ_ਪਿੱਠ(varp);
                }

                Ast_ਗੰਢ_DType* ਅਚਲ dtypep = varp->dtypeSkipRefp();
                ਅਚਲ ਪੂਰਨ sigbytes = dtypep->ਚੌੜਾਈ_Align_ਬਾਈਟਾਂ();
                ਅਚਲ uint64_t stratum = VN_IS(dtypep, UnpackArrayDType)                  ? 9
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
        ਲਈ (std::ਸਦਿਸ਼<AstVar*>* ਅਚਲ varpsp : {&trivps, &nontps}) {
            ਜੇ (varpsp->ਖਾਲੀ_ਥਾਵਾਂ()) ਜਾਰੀ;
            ਜੇ (!v3Global.opt.mtasks()) {
                simple_ਛਾਂਟੋ_Vars(*varpsp);
            } ਨਹੀਂਤਾਂ {
                mtask_ਛਾਂਟੋ_Vars(*varpsp);
            }
        }

        // Insert them back under the module, in the new order, but at
        // the front of the list so they come out first in dumps/JSON.
        Ast_ਗੰਢ* nodesp = ਖਾਲੀ_ਸੂਚਕ;
        ਲਈ (ਅਚਲ std::ਸਦਿਸ਼<AstVar*>* ਅਚਲ varpsp : {&portps, &trivps, &nontps, &restps}) {
            ਲਈ (AstVar* ਅਚਲ varp : *varpsp) {
                nodesp = Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ_Null(nodesp, varp->unlink_Fr_ਪਿੱਠ());
            }
        }
        modp->prependStmtsp(nodesp);
    }

ਜਨਤਕ:
    ਸਥਿਰ ਖਾਲੀ apply(Ast_ਗੰਢ_Module* modp, ਅਚਲ MTaskWritersMap& mTaskWriters) {
        Variable_ਕ੍ਰਮ{modp, mTaskWriters};
    }
};

//######################################################################
// V3VariableOrder static functions

ਖਾਲੀ V3_Variable_ਕ੍ਰਮ::ਕ੍ਰਮ_All(AstNetlist* netlistp) {
    UINFO(2, __FUNCTION__ << ":");

    MTaskWritersMap mTaskWriters;

    // Gather writing MTasks
    ਜੇ (v3Global.opt.mtasks()) {
        netlistp->ਸਿਖਰ_Modulep()->foreach([&](Ast_Exec_ਗ੍ਰਾਫ* execGraphp) {
            ਲਈ (ਅਚਲ V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼& vtx : execGraphp->depGraphp()->vertices()) {
                GatherMTaskWriters::apply(vtx.as<ਅਚਲ ExecMTask>(), mTaskWriters);
            }
        });
    }

    // Sort variables of each module
    ਲਈ (Ast_ਗੰਢ_Module* modp = v3Global.rootp()->modulesp(); modp;
         modp = VN_AS(modp->nextp(), ਗੰਢ_Module)) {
        ਜੇ (modp->isConstPool()) ਜਾਰੀ;
        Variable_ਕ੍ਰਮ::apply(modp, mTaskWriters);
    }

    V3Global::dump_Check_Global_ਰੁੱਖ("variableorder", 0, dump_ਰੁੱਖ_Either_Level() >= 3);
}
