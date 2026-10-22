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

#అన్తర్భావ "V3PchAstMT.h"

#అన్తర్భావ "V3VariableOrder.h"

#అన్తర్భావ "V3AstUserAllocator.h"
#అన్తర్భావ "V3ExecGraph.h"
#అన్తర్భావ "V3Stats.h"

#అన్తర్భావ <algorithm>
#అన్తర్భావ <cstdint>
#అన్తర్భావ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

ఉపయోగించి MTaskIdVec = std::సదిశ<తార్కిక>;  // Used as a bit-set indexed by MTask ID
// Writing MTasks of each variable accessed by any MTask
ఉపయోగించి MTaskWritersMap = std::unordered_map<స్థిరరాశి AstVar*, MTaskIdVec>;

// Trace through code reachable from an MTask and record the writers of referenced variables
తరగతి GatherMTaskWriters final : VNVisitorConst {
    // NODE STATE
    //  AstCFunc::user1()  // bool: Already traced this function
    //  AstVar::user1()  // bool: Already traced this variable
    స్థిరరాశి VNUser1_లోపల_Use m_user1_లోపల_Use;

    // STATE
    MTaskWritersMap& m_results;  // The result map being built;
    స్థిరరాశి uint32_t m_id;  // Id of mtask being analysed
    స్థిరరాశి కొలత_t m_వాడిన_Ids = ExecMTask::అంకె_వాడిన_Ids();  // Value of max id + 1

    // CONSTRUCTOR
    GatherMTaskWriters(స్థిరరాశి ExecMTask* mTaskp, MTaskWritersMap& results)
        : m_results{results}
        , m_id{mTaskp->id()} {
        iterateConst(mTaskp->funcp());
    }
    ~GatherMTaskWriters() = అప్రమేయ;
    VL_UNMOVABLE(GatherMTaskWriters);

    //సందర్శించు
    శూన్య సందర్శించు(Ast_కణుపు_Var_Ref* nodep) override {
        // Cheaper than relying on emplace().second
        ఒకవేళ (nodep->user1_సమితి_Once()) తిరిగి;
        AstVar* స్థిరరాశి varp = nodep->varp();
        // Record the access, and the writer bit if written
        MTaskIdVec& writers = m_results
                                  .emplace(std::piecewise_construct,  //
                                           std::forward_as_tuple(varp),  //
                                           std::forward_as_tuple(m_వాడిన_Ids))
                                  .మొదటి->రెండవ;
        ఒకవేళ (nodep->access().isWriteOrRW()) writers[m_id] = నిజం;
    }

    శూన్య సందర్శించు(AstCFunc* nodep) override {
        ఒకవేళ (nodep->user1_సమితి_Once()) తిరిగి;  // Prevent repeat traversals/recursion
        iterateChildrenConst(nodep);
    }

    శూన్య సందర్శించు(Ast_కణుపు_CCall* nodep) override {
        iterateChildrenConst(nodep);  // Arguments
        iterateConst(nodep->funcp());  // Callee
    }

    శూన్య సందర్శించు(Ast_కణుపు* nodep) override { iterateChildrenConst(nodep); }

బహిరంగ:
    స్థిర శూన్య apply(స్థిరరాశి ExecMTask* mTaskp, MTaskWritersMap& results) {
        GatherMTaskWriters{mTaskp, results};
    }
};

తరగతి Variable_క్రమం final {
    // NODE STATE
    //  AstVar::user1()   ->   uint64_t: Stratum, roughly equivalent to alignment
    //                                   requirement, to avoid padding
    స్థిరరాశి VNUser1_లోపల_Use m_user1_లోపల_Use;

    స్థిరరాశి MTaskWritersMap& m_mTaskWriters;

    Variable_క్రమం(Ast_కణుపు_Module* modp, స్థిరరాశి MTaskWritersMap& mTaskWriters)
        : m_mTaskWriters{mTaskWriters} {
        క్రమం_Module_Vars(modp);
    }
    ~Variable_క్రమం() = అప్రమేయ;
    VL_UNCOPYABLE(Variable_క్రమం);

    //######################################################################

    // Simple sort by stratum
    శూన్య simple_క్రమీకరించు_Vars(std::సదిశ<AstVar*>& varps) {
        stable_క్రమీకరించు(varps.begin(), varps.చివర(), [](స్థిరరాశి AstVar* ap, స్థిరరాశి AstVar* bp) -> తార్కిక {
            తిరిగి ap->user1() < bp->user1();
        });
    }

    // Sort by writing MTasks first, then the same as simpleSortVars
    శూన్య mtask_క్రమీకరించు_Vars(std::సదిశ<AstVar*>& varps) {
        // Map from "writing MTasks" -> "variable list", for variables accessed by MTasks
        std::map<MTaskIdVec, std::సదిశ<AstVar*>> m2v;
        // Variables not accessed by any MTask
        std::సదిశ<AstVar*> noAffinityVarps;
        కొరకు (AstVar* స్థిరరాశి varp : varps) {
            స్థిరరాశి స్వయంచాలక it = m_mTaskWriters.కనుగొను(varp);
            ఒకవేళ (it == m_mTaskWriters.చివర()) {
                noAffinityVarps.నెట్టు_వెనుక(varp);
            } లేకపోతే {
                m2v[it->రెండవ].నెట్టు_వెనుక(varp);
            }
        }

        varps.clear();

        // Helper function to sort given vector, then append to 'varps'
        స్థిరరాశి స్వయంచాలక క్రమీకరించు_And_Append
            = [ఇది, &varps](std::సదిశ<AstVar*>& subVarps, తార్కిక align_మొదటి) {
                  simple_క్రమీకరించు_Vars(subVarps);
                  తార్కిక aligned = !align_మొదటి;
                  కొరకు (AstVar* స్థిరరాశి varp : subVarps) {
                      // Align the first variable with instance storage
                      ఒకవేళ (!aligned && varp->isModelState()) {
                          varp->mtask_Cache_పంక్తి_Align(నిజం);
                          V3Stats::కూడు_Stat_కూడిక("VariableOrder, MTask aligned group starts", 1);
                          aligned = నిజం;
                      }
                      varps.నెట్టు_వెనుక(varp);
                  }
              };

        // Add the groups in the map's deterministic key order
        కొరకు (స్వయంచాలక& pair : m2v) క్రమీకరించు_And_Append(pair.రెండవ, నిజం);

        // Finally add the variables with no known MTask affinity
        క్రమీకరించు_And_Append(noAffinityVarps, అబద్ధం);

        V3Stats::కూడు_Stat_కూడిక("VariableOrder, MTask affinity groups", m2v.కొలత());
        V3Stats::కూడు_Stat_కూడిక("VariableOrder, no-affinity variables", noAffinityVarps.కొలత());
    }

    // cppcheck-suppress constParameterPointer
    శూన్య క్రమం_Module_Vars(Ast_కణుపు_Module* modp) {
        // Top level ports stay first, in source order (--lib-create wrapper must match original)
        std::సదిశ<AstVar*> portps;
        // Followed by trivially constructible variables
        std::సదిశ<AstVar*> trivps;
        // Followed by not trivially constructible variables
        std::సదిశ<AstVar*> nontps;
        // Followed by variables that are not model state, in source order
        std::సదిశ<AstVar*> restps;

        // Gather variables, compute stratum
        కొరకు (Ast_కణుపు *nodep = modp->stmtsp(), *nextp; nodep; nodep = nextp) {
            nextp = nodep->nextp();
            ఒకవేళ (AstVar* స్థిరరాశి varp = VN_CAST(nodep, Var)) {
                ఒకవేళ (varp->isPrimaryIO()) {
                    portps.నెట్టు_వెనుక(varp);
                    కొనసాగు;
                }
                ఒకవేళ (!varp->isModelState()) {
                    restps.నెట్టు_వెనుక(varp);
                    కొనసాగు;
                }

                ఒకవేళ (varp->isTriviallyConstructible()) {
                    trivps.నెట్టు_వెనుక(varp);
                } లేకపోతే {
                    nontps.నెట్టు_వెనుక(varp);
                }

                Ast_కణుపు_DType* స్థిరరాశి dtypep = varp->dtypeSkipRefp();
                స్థిరరాశి పూర్ణాంకం sigbytes = dtypep->వెడల్పు_Align_బైట్లు();
                స్థిరరాశి uint64_t stratum = VN_IS(dtypep, UnpackArrayDType)                  ? 9
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
        కొరకు (std::సదిశ<AstVar*>* స్థిరరాశి varpsp : {&trivps, &nontps}) {
            ఒకవేళ (varpsp->ఖాళీలు()) కొనసాగు;
            ఒకవేళ (!v3Global.opt.mtasks()) {
                simple_క్రమీకరించు_Vars(*varpsp);
            } లేకపోతే {
                mtask_క్రమీకరించు_Vars(*varpsp);
            }
        }

        // Insert them back under the module, in the new order, but at
        // the front of the list so they come out first in dumps/JSON.
        Ast_కణుపు* nodesp = శూన్యసూచిక;
        కొరకు (స్థిరరాశి std::సదిశ<AstVar*>* స్థిరరాశి varpsp : {&portps, &trivps, &nontps, &restps}) {
            కొరకు (AstVar* స్థిరరాశి varp : *varpsp) {
                nodesp = Ast_కణుపు::కూడు_తదుపరి_Null(nodesp, varp->unlink_Fr_వెనుక());
            }
        }
        modp->prependStmtsp(nodesp);
    }

బహిరంగ:
    స్థిర శూన్య apply(Ast_కణుపు_Module* modp, స్థిరరాశి MTaskWritersMap& mTaskWriters) {
        Variable_క్రమం{modp, mTaskWriters};
    }
};

//######################################################################
// V3VariableOrder static functions

శూన్య V3_Variable_క్రమం::క్రమం_All(AstNetlist* netlistp) {
    UINFO(2, __FUNCTION__ << ":");

    MTaskWritersMap mTaskWriters;

    // Gather writing MTasks
    ఒకవేళ (v3Global.opt.mtasks()) {
        netlistp->శిఖరం_Modulep()->foreach([&](Ast_Exec_లేఖాచిత్రం* execGraphp) {
            కొరకు (స్థిరరాశి V3_లేఖాచిత్రం_శీర్షం& vtx : execGraphp->depGraphp()->vertices()) {
                GatherMTaskWriters::apply(vtx.as<స్థిరరాశి ExecMTask>(), mTaskWriters);
            }
        });
    }

    // Sort variables of each module
    కొరకు (Ast_కణుపు_Module* modp = v3Global.rootp()->modulesp(); modp;
         modp = VN_AS(modp->nextp(), కణుపు_Module)) {
        ఒకవేళ (modp->isConstPool()) కొనసాగు;
        Variable_క్రమం::apply(modp, mTaskWriters);
    }

    V3Global::dump_Check_Global_వృక్షం("variableorder", 0, dump_వృక్షం_Either_Level() >= 3);
}
