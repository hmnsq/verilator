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

#अन्तर्भाव "V3PchAstMT.h"

#अन्तर्भाव "V3VariableOrder.h"

#अन्तर्भाव "V3AstUserAllocator.h"
#अन्तर्भाव "V3ExecGraph.h"
#अन्तर्भाव "V3Stats.h"

#अन्तर्भाव <algorithm>
#अन्तर्भाव <cstdint>
#अन्तर्भाव <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

उपयुज्य MTaskIdVec = std::सदिशम्<तार्किक>;  // Used as a bit-set indexed by MTask ID
// Writing MTasks of each variable accessed by any MTask
उपयुज्य MTaskWritersMap = std::unordered_map<ध्रुव AstVar*, MTaskIdVec>;

// Trace through code reachable from an MTask and record the writers of referenced variables
वर्ग GatherMTaskWriters final : VNVisitorConst {
    // NODE STATE
    //  AstCFunc::user1()  // bool: Already traced this function
    //  AstVar::user1()  // bool: Already traced this variable
    ध्रुव VNUser1_अन्तर्_Use m_user1_अन्तर्_Use;

    // STATE
    MTaskWritersMap& m_results;  // The result map being built;
    ध्रुव uint32_t m_id;  // Id of mtask being analysed
    ध्रुव आकार_t m_प्रयुक्तम्_Ids = ExecMTask::अङ्कः_प्रयुक्तम्_Ids();  // Value of max id + 1

    // CONSTRUCTOR
    GatherMTaskWriters(ध्रुव ExecMTask* mTaskp, MTaskWritersMap& results)
        : m_results{results}
        , m_id{mTaskp->id()} {
        iterateConst(mTaskp->funcp());
    }
    ~GatherMTaskWriters() = स्वभाव;
    VL_UNMOVABLE(GatherMTaskWriters);

    //दर्शय
    शून्य दर्शय(Ast_ग्रन्थिः_Var_Ref* nodep) override {
        // Cheaper than relying on emplace().second
        यदि (nodep->user1_समुच्चयः_Once()) प्रत्यागम;
        AstVar* ध्रुव varp = nodep->varp();
        // Record the access, and the writer bit if written
        MTaskIdVec& writers = m_results
                                  .emplace(std::piecewise_construct,  //
                                           std::forward_as_tuple(varp),  //
                                           std::forward_as_tuple(m_प्रयुक्तम्_Ids))
                                  .first->second;
        यदि (nodep->access().isWriteOrRW()) writers[m_id] = सत्य;
    }

    शून्य दर्शय(AstCFunc* nodep) override {
        यदि (nodep->user1_समुच्चयः_Once()) प्रत्यागम;  // Prevent repeat traversals/recursion
        iterateChildrenConst(nodep);
    }

    शून्य दर्शय(Ast_ग्रन्थिः_CCall* nodep) override {
        iterateChildrenConst(nodep);  // Arguments
        iterateConst(nodep->funcp());  // Callee
    }

    शून्य दर्शय(Ast_ग्रन्थिः* nodep) override { iterateChildrenConst(nodep); }

सार्वजनिक:
    स्थिर शून्य apply(ध्रुव ExecMTask* mTaskp, MTaskWritersMap& results) {
        GatherMTaskWriters{mTaskp, results};
    }
};

वर्ग Variable_क्रमः final {
    // NODE STATE
    //  AstVar::user1()   ->   uint64_t: Stratum, roughly equivalent to alignment
    //                                   requirement, to avoid padding
    ध्रुव VNUser1_अन्तर्_Use m_user1_अन्तर्_Use;

    ध्रुव MTaskWritersMap& m_mTaskWriters;

    Variable_क्रमः(Ast_ग्रन्थिः_Module* modp, ध्रुव MTaskWritersMap& mTaskWriters)
        : m_mTaskWriters{mTaskWriters} {
        क्रमः_Module_Vars(modp);
    }
    ~Variable_क्रमः() = स्वभाव;
    VL_UNCOPYABLE(Variable_क्रमः);

    //######################################################################

    // Simple sort by stratum
    शून्य simple_क्रमय_Vars(std::सदिशम्<AstVar*>& varps) {
        stable_क्रमय(varps.begin(), varps.अन्तः_2(), [](ध्रुव AstVar* ap, ध्रुव AstVar* bp) -> तार्किक {
            प्रत्यागम ap->user1() < bp->user1();
        });
    }

    // Sort by writing MTasks first, then the same as simpleSortVars
    शून्य mtask_क्रमय_Vars(std::सदिशम्<AstVar*>& varps) {
        // Map from "writing MTasks" -> "variable list", for variables accessed by MTasks
        std::map<MTaskIdVec, std::सदिशम्<AstVar*>> m2v;
        // Variables not accessed by any MTask
        std::सदिशम्<AstVar*> noAffinityVarps;
        प्रति (AstVar* ध्रुव varp : varps) {
            ध्रुव स्वचालित it = m_mTaskWriters.find(varp);
            यदि (it == m_mTaskWriters.अन्तः_2()) {
                noAffinityVarps.प्रक्षिप_पृष्ठभागः(varp);
            } अन्यथा {
                m2v[it->second].प्रक्षिप_पृष्ठभागः(varp);
            }
        }

        varps.clear();

        // Helper function to sort given vector, then append to 'varps'
        ध्रुव स्वचालित क्रमय_And_Append
            = [एतत्, &varps](std::सदिशम्<AstVar*>& subVarps, तार्किक alignFirst) {
                  simple_क्रमय_Vars(subVarps);
                  तार्किक aligned = !alignFirst;
                  प्रति (AstVar* ध्रुव varp : subVarps) {
                      // Align the first variable with instance storage
                      यदि (!aligned && varp->isModelState()) {
                          varp->mtask_Cache_पङ्क्तिः_Align(सत्य);
                          V3Stats::सङ्कलय_Stat_योग("VariableOrder, MTask aligned group starts", 1);
                          aligned = सत्य;
                      }
                      varps.प्रक्षिप_पृष्ठभागः(varp);
                  }
              };

        // Add the groups in the map's deterministic key order
        प्रति (स्वचालित& pair : m2v) क्रमय_And_Append(pair.second, सत्य);

        // Finally add the variables with no known MTask affinity
        क्रमय_And_Append(noAffinityVarps, असत्य);

        V3Stats::सङ्कलय_Stat_योग("VariableOrder, MTask affinity groups", m2v.आकार());
        V3Stats::सङ्कलय_Stat_योग("VariableOrder, no-affinity variables", noAffinityVarps.आकार());
    }

    // cppcheck-suppress constParameterPointer
    शून्य क्रमः_Module_Vars(Ast_ग्रन्थिः_Module* modp) {
        // Top level ports stay first, in source order (--lib-create wrapper must match original)
        std::सदिशम्<AstVar*> portps;
        // Followed by trivially constructible variables
        std::सदिशम्<AstVar*> trivps;
        // Followed by not trivially constructible variables
        std::सदिशम्<AstVar*> nontps;
        // Followed by variables that are not model state, in source order
        std::सदिशम्<AstVar*> restps;

        // Gather variables, compute stratum
        प्रति (Ast_ग्रन्थिः *nodep = modp->stmtsp(), *nextp; nodep; nodep = nextp) {
            nextp = nodep->nextp();
            यदि (AstVar* ध्रुव varp = VN_CAST(nodep, Var)) {
                यदि (varp->isPrimaryIO()) {
                    portps.प्रक्षिप_पृष्ठभागः(varp);
                    अनुवर्त;
                }
                यदि (!varp->isModelState()) {
                    restps.प्रक्षिप_पृष्ठभागः(varp);
                    अनुवर्त;
                }

                यदि (varp->isTriviallyConstructible()) {
                    trivps.प्रक्षिप_पृष्ठभागः(varp);
                } अन्यथा {
                    nontps.प्रक्षिप_पृष्ठभागः(varp);
                }

                Ast_ग्रन्थिः_DType* ध्रुव dtypep = varp->dtypeSkipRefp();
                ध्रुव पूर्णाङ्क sigbytes = dtypep->विस्तारः_Align_अष्टकानि();
                ध्रुव uint64_t stratum = VN_IS(dtypep, UnpackArrayDType)                  ? 9
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
        प्रति (std::सदिशम्<AstVar*>* ध्रुव varpsp : {&trivps, &nontps}) {
            यदि (varpsp->रिक्तानि()) अनुवर्त;
            यदि (!v3Global.opt.mtasks()) {
                simple_क्रमय_Vars(*varpsp);
            } अन्यथा {
                mtask_क्रमय_Vars(*varpsp);
            }
        }

        // Insert them back under the module, in the new order, but at
        // the front of the list so they come out first in dumps/JSON.
        Ast_ग्रन्थिः* nodesp = शून्यसूचकः;
        प्रति (ध्रुव std::सदिशम्<AstVar*>* ध्रुव varpsp : {&portps, &trivps, &nontps, &restps}) {
            प्रति (AstVar* ध्रुव varp : *varpsp) {
                nodesp = Ast_ग्रन्थिः::सङ्कलय_अग्र_Null(nodesp, varp->unlink_Fr_पृष्ठभागः());
            }
        }
        modp->prependStmtsp(nodesp);
    }

सार्वजनिक:
    स्थिर शून्य apply(Ast_ग्रन्थिः_Module* modp, ध्रुव MTaskWritersMap& mTaskWriters) {
        Variable_क्रमः{modp, mTaskWriters};
    }
};

//######################################################################
// V3VariableOrder static functions

शून्य V3_Variable_क्रमः::क्रमः_All(AstNetlist* netlistp) {
    UINFO(2, __FUNCTION__ << ":");

    MTaskWritersMap mTaskWriters;

    // Gather writing MTasks
    यदि (v3Global.opt.mtasks()) {
        netlistp->शिखरम्_Modulep()->foreach([&](AstExecGraph* execGraphp) {
            प्रति (ध्रुव V3GraphVertex& vtx : execGraphp->depGraphp()->vertices()) {
                GatherMTaskWriters::apply(vtx.as<ध्रुव ExecMTask>(), mTaskWriters);
            }
        });
    }

    // Sort variables of each module
    प्रति (Ast_ग्रन्थिः_Module* modp = v3Global.rootp()->modulesp(); modp;
         modp = VN_AS(modp->nextp(), ग्रन्थिः_Module)) {
        यदि (modp->isConstPool()) अनुवर्त;
        Variable_क्रमः::apply(modp, mTaskWriters);
    }

    V3Global::dump_Check_Global_वृक्षः("variableorder", 0, dump_वृक्षः_Either_Level() >= 3);
}
