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

using MTaskIdVec = std::vector<तार्किक>;  // Used as a bit-set indexed by MTask ID
// Writing MTasks of each variable accessed by any MTask
using MTaskWritersMap = std::unordered_map<अचर AstVar*, MTaskIdVec>;

// Trace through code reachable from an MTask and record the writers of referenced variables
वर्ग GatherMTaskWriters final : VNVisitorConst {
    // NODE STATE
    //  AstCFunc::user1()  // bool: Already traced this function
    //  AstVar::user1()  // bool: Already traced this variable
    अचर VNUser1InUse m_user1InUse;

    // STATE
    MTaskWritersMap& m_results;  // The result map being built;
    अचर uint32_t m_id;  // Id of mtask being analysed
    अचर माप_प्रकार m_usedIds = ExecMTask::अङ्क_Used_Ids();  // Value of max id + 1

    // CONSTRUCTOR
    GatherMTaskWriters(अचर ExecMTask* mTaskp, MTaskWritersMap& results)
        : m_results{results}
        , m_id{mTaskp->id()} {
        iterateConst(mTaskp->funcp());
    }
    ~GatherMTaskWriters() = स्वभाव;
    VL_UNMOVABLE(GatherMTaskWriters);

    // VISIT
    शून्य visit(Ast_गाँठो_Var_Ref* nodep) override {
        // Cheaper than relying on emplace().second
        यदि (nodep->user1SetOnce()) फर्काऊ;
        AstVar* अचर varp = nodep->varp();
        // Record the access, and the writer bit if written
        MTaskIdVec& writers = m_results
                                  .emplace(std::piecewise_construct,  //
                                           std::forward_as_tuple(varp),  //
                                           std::forward_as_tuple(m_usedIds))
                                  .first->second;
        यदि (nodep->access().isWriteOrRW()) writers[m_id] = सत्य;
    }

    शून्य visit(AstCFunc* nodep) override {
        यदि (nodep->user1SetOnce()) फर्काऊ;  // Prevent repeat traversals/recursion
        iterateChildrenConst(nodep);
    }

    शून्य visit(Ast_गाँठो_CCall* nodep) override {
        iterateChildrenConst(nodep);  // Arguments
        iterateConst(nodep->funcp());  // Callee
    }

    शून्य visit(Ast_गाँठो* nodep) override { iterateChildrenConst(nodep); }

सार्वजनिक:
    स्थिर शून्य apply(अचर ExecMTask* mTaskp, MTaskWritersMap& results) {
        GatherMTaskWriters{mTaskp, results};
    }
};

वर्ग VariableOrder final {
    // NODE STATE
    //  AstVar::user1()   ->   uint64_t: Stratum, roughly equivalent to alignment
    //                                   requirement, to avoid padding
    अचर VNUser1InUse m_user1InUse;

    अचर MTaskWritersMap& m_mTaskWriters;

    VariableOrder(Ast_गाँठो_Module* modp, अचर MTaskWritersMap& mTaskWriters)
        : m_mTaskWriters{mTaskWriters} {
        orderModuleVars(modp);
    }
    ~VariableOrder() = स्वभाव;
    VL_UNCOPYABLE(VariableOrder);

    //######################################################################

    // Simple sort by stratum
    शून्य simpleSortVars(std::vector<AstVar*>& varps) {
        stable_sort(varps.begin(), varps.end(), [](अचर AstVar* ap, अचर AstVar* bp) -> तार्किक {
            फर्काऊ ap->user1() < bp->user1();
        });
    }

    // Sort by writing MTasks first, then the same as simpleSortVars
    शून्य mtaskSortVars(std::vector<AstVar*>& varps) {
        // Map from "writing MTasks" -> "variable list", for variables accessed by MTasks
        std::map<MTaskIdVec, std::vector<AstVar*>> m2v;
        // Variables not accessed by any MTask
        std::vector<AstVar*> noAffinityVarps;
        लागि (AstVar* अचर varp : varps) {
            अचर स्वतः it = m_mTaskWriters.find(varp);
            यदि (it == m_mTaskWriters.end()) {
                noAffinityVarps.थप_back(varp);
            } अन्यथा {
                m2v[it->second].थप_back(varp);
            }
        }

        varps.clear();

        // Helper function to sort given vector, then append to 'varps'
        अचर स्वतः sortAndAppend
            = [एतत्, &varps](std::vector<AstVar*>& subVarps, तार्किक alignFirst) {
                  simpleSortVars(subVarps);
                  तार्किक aligned = !alignFirst;
                  लागि (AstVar* अचर varp : subVarps) {
                      // Align the first variable with instance storage
                      यदि (!aligned && varp->isModelState()) {
                          varp->mtask_Cache_पङ्क्ति_Align(सत्य);
                          V3Stats::addStatSum("VariableOrder, MTask aligned group starts", 1);
                          aligned = सत्य;
                      }
                      varps.थप_back(varp);
                  }
              };

        // Add the groups in the map's deterministic key order
        लागि (स्वतः& pair : m2v) sortAndAppend(pair.second, सत्य);

        // Finally add the variables with no known MTask affinity
        sortAndAppend(noAffinityVarps, असत्य);

        V3Stats::addStatSum("VariableOrder, MTask affinity groups", m2v.size());
        V3Stats::addStatSum("VariableOrder, no-affinity variables", noAffinityVarps.size());
    }

    // cppcheck-suppress constParameterPointer
    शून्य orderModuleVars(Ast_गाँठो_Module* modp) {
        // Top level ports stay first, in source order (--lib-create wrapper must match original)
        std::vector<AstVar*> portps;
        // Followed by trivially constructible variables
        std::vector<AstVar*> trivps;
        // Followed by not trivially constructible variables
        std::vector<AstVar*> nontps;
        // Followed by variables that are not model state, in source order
        std::vector<AstVar*> restps;

        // Gather variables, compute stratum
        लागि (Ast_गाँठो *nodep = modp->stmtsp(), *nextp; nodep; nodep = nextp) {
            nextp = nodep->nextp();
            यदि (AstVar* अचर varp = VN_CAST(nodep, Var)) {
                यदि (varp->isPrimaryIO()) {
                    portps.थप_back(varp);
                    जारी;
                }
                यदि (!varp->isModelState()) {
                    restps.थप_back(varp);
                    जारी;
                }

                यदि (varp->isTriviallyConstructible()) {
                    trivps.थप_back(varp);
                } अन्यथा {
                    nontps.थप_back(varp);
                }

                Ast_गाँठो_DType* अचर dtypep = varp->dtypeSkipRefp();
                अचर पूर्णाङ्क sigbytes = dtypep->widthAlignBytes();
                अचर uint64_t stratum = VN_IS(dtypep, UnpackArrayDType)                  ? 9
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
        लागि (std::vector<AstVar*>* अचर varpsp : {&trivps, &nontps}) {
            यदि (varpsp->empty()) जारी;
            यदि (!v3Global.opt.mtasks()) {
                simpleSortVars(*varpsp);
            } अन्यथा {
                mtaskSortVars(*varpsp);
            }
        }

        // Insert them back under the module, in the new order, but at
        // the front of the list so they come out first in dumps/JSON.
        Ast_गाँठो* nodesp = nullptr;
        लागि (अचर std::vector<AstVar*>* अचर varpsp : {&portps, &trivps, &nontps, &restps}) {
            लागि (AstVar* अचर varp : *varpsp) {
                nodesp = Ast_गाँठो::addNextNull(nodesp, varp->unlinkFrBack());
            }
        }
        modp->prependStmtsp(nodesp);
    }

सार्वजनिक:
    स्थिर शून्य apply(Ast_गाँठो_Module* modp, अचर MTaskWritersMap& mTaskWriters) {
        VariableOrder{modp, mTaskWriters};
    }
};

//######################################################################
// V3VariableOrder static functions

शून्य V3VariableOrder::orderAll(AstNetlist* netlistp) {
    UINFO(2, __FUNCTION__ << ":");

    MTaskWritersMap mTaskWriters;

    // Gather writing MTasks
    यदि (v3Global.opt.mtasks()) {
        netlistp->शीर्ष_Modulep()->foreach([&](AstExecGraph* execGraphp) {
            लागि (अचर V3GraphVertex& vtx : execGraphp->depGraphp()->vertices()) {
                GatherMTaskWriters::apply(vtx.as<अचर ExecMTask>(), mTaskWriters);
            }
        });
    }

    // Sort variables of each module
    लागि (Ast_गाँठो_Module* modp = v3Global.rootp()->modulesp(); modp;
         modp = VN_AS(modp->nextp(), गाँठो_Module)) {
        यदि (modp->isConstPool()) जारी;
        VariableOrder::apply(modp, mTaskWriters);
    }

    V3Global::dump_Check_Global_रूख("variableorder", 0, dump_रूख_Either_Level() >= 3);
}
