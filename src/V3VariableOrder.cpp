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

استعمال MTaskIdVec = std::سمتیہ<منطقی>;  // Used as a bit-set indexed by MTask ID
// Writing MTasks of each variable accessed by any MTask
استعمال MTaskWritersMap = std::unordered_map<ثابت AstVar*, MTaskIdVec>;

// Trace through code reachable from an MTask and record the writers of referenced variables
جماعت GatherMTaskWriters final : VNVisitorConst {
    // NODE STATE
    //  AstCFunc::user1()  // bool: Already traced this function
    //  AstVar::user1()  // bool: Already traced this variable
    ثابت VNUser1_اندر_Use m_user1_اندر_Use;

    // STATE
    MTaskWritersMap& m_results;  // The result map being built;
    ثابت uint32_t m_id;  // Id of mtask being analysed
    ثابت سائز_t m_مستعمل_Ids = ExecMTask::عدد_مستعمل_Ids();  // Value of max id + 1

    // CONSTRUCTOR
    GatherMTaskWriters(ثابت ExecMTask* mTaskp, MTaskWritersMap& results)
        : m_results{results}
        , m_id{mTaskp->id()} {
        iterateConst(mTaskp->funcp());
    }
    ~GatherMTaskWriters() = طےشدہ;
    VL_UNMOVABLE(GatherMTaskWriters);

    //دورہ
    خالی دورہ(Ast_گرہ_Var_Ref* nodep) override {
        // Cheaper than relying on emplace().second
        اگر (nodep->user1_سیٹ_Once()) واپس;
        AstVar* ثابت varp = nodep->varp();
        // Record the access, and the writer bit if written
        MTaskIdVec& writers = m_results
                                  .emplace(std::piecewise_construct,  //
                                           std::forward_as_tuple(varp),  //
                                           std::forward_as_tuple(m_مستعمل_Ids))
                                  .پہلا->دوسرا;
        اگر (nodep->access().isWriteOrRW()) writers[m_id] = سچ;
    }

    خالی دورہ(AstCFunc* nodep) override {
        اگر (nodep->user1_سیٹ_Once()) واپس;  // Prevent repeat traversals/recursion
        iterateChildrenConst(nodep);
    }

    خالی دورہ(Ast_گرہ_CCall* nodep) override {
        iterateChildrenConst(nodep);  // Arguments
        iterateConst(nodep->funcp());  // Callee
    }

    خالی دورہ(Ast_گرہ* nodep) override { iterateChildrenConst(nodep); }

عوامی:
    ساکن خالی apply(ثابت ExecMTask* mTaskp, MTaskWritersMap& results) {
        GatherMTaskWriters{mTaskp, results};
    }
};

جماعت Variable_ترتیب final {
    // NODE STATE
    //  AstVar::user1()   ->   uint64_t: Stratum, roughly equivalent to alignment
    //                                   requirement, to avoid padding
    ثابت VNUser1_اندر_Use m_user1_اندر_Use;

    ثابت MTaskWritersMap& m_mTaskWriters;

    Variable_ترتیب(Ast_گرہ_Module* modp, ثابت MTaskWritersMap& mTaskWriters)
        : m_mTaskWriters{mTaskWriters} {
        ترتیب_Module_Vars(modp);
    }
    ~Variable_ترتیب() = طےشدہ;
    VL_UNCOPYABLE(Variable_ترتیب);

    //######################################################################

    // Simple sort by stratum
    خالی simple_ترتیب_دو_Vars(std::سمتیہ<AstVar*>& varps) {
        stable_ترتیب_دو(varps.begin(), varps.انتہا(), [](ثابت AstVar* ap, ثابت AstVar* bp) -> منطقی {
            واپس ap->user1() < bp->user1();
        });
    }

    // Sort by writing MTasks first, then the same as simpleSortVars
    خالی mtask_ترتیب_دو_Vars(std::سمتیہ<AstVar*>& varps) {
        // Map from "writing MTasks" -> "variable list", for variables accessed by MTasks
        std::map<MTaskIdVec, std::سمتیہ<AstVar*>> m2v;
        // Variables not accessed by any MTask
        std::سمتیہ<AstVar*> noAffinityVarps;
        برائے (AstVar* ثابت varp : varps) {
            ثابت خودکار it = m_mTaskWriters.ڈھونڈو(varp);
            اگر (it == m_mTaskWriters.انتہا()) {
                noAffinityVarps.ڈالو_عقب(varp);
            } ورنہ {
                m2v[it->دوسرا].ڈالو_عقب(varp);
            }
        }

        varps.clear();

        // Helper function to sort given vector, then append to 'varps'
        ثابت خودکار ترتیب_دو_And_Append
            = [یہ, &varps](std::سمتیہ<AstVar*>& subVarps, منطقی align_پہلا) {
                  simple_ترتیب_دو_Vars(subVarps);
                  منطقی aligned = !align_پہلا;
                  برائے (AstVar* ثابت varp : subVarps) {
                      // Align the first variable with instance storage
                      اگر (!aligned && varp->isModelState()) {
                          varp->mtask_Cache_سطر_Align(سچ);
                          V3Stats::جوڑو_Stat_مجموع("VariableOrder, MTask aligned group starts", 1);
                          aligned = سچ;
                      }
                      varps.ڈالو_عقب(varp);
                  }
              };

        // Add the groups in the map's deterministic key order
        برائے (خودکار& pair : m2v) ترتیب_دو_And_Append(pair.دوسرا, سچ);

        // Finally add the variables with no known MTask affinity
        ترتیب_دو_And_Append(noAffinityVarps, جھوٹ);

        V3Stats::جوڑو_Stat_مجموع("VariableOrder, MTask affinity groups", m2v.سائز());
        V3Stats::جوڑو_Stat_مجموع("VariableOrder, no-affinity variables", noAffinityVarps.سائز());
    }

    // cppcheck-suppress constParameterPointer
    خالی ترتیب_Module_Vars(Ast_گرہ_Module* modp) {
        // Top level ports stay first, in source order (--lib-create wrapper must match original)
        std::سمتیہ<AstVar*> portps;
        // Followed by trivially constructible variables
        std::سمتیہ<AstVar*> trivps;
        // Followed by not trivially constructible variables
        std::سمتیہ<AstVar*> nontps;
        // Followed by variables that are not model state, in source order
        std::سمتیہ<AstVar*> restps;

        // Gather variables, compute stratum
        برائے (Ast_گرہ *nodep = modp->stmtsp(), *nextp; nodep; nodep = nextp) {
            nextp = nodep->nextp();
            اگر (AstVar* ثابت varp = VN_CAST(nodep, Var)) {
                اگر (varp->isPrimaryIO()) {
                    portps.ڈالو_عقب(varp);
                    جاری;
                }
                اگر (!varp->isModelState()) {
                    restps.ڈالو_عقب(varp);
                    جاری;
                }

                اگر (varp->isTriviallyConstructible()) {
                    trivps.ڈالو_عقب(varp);
                } ورنہ {
                    nontps.ڈالو_عقب(varp);
                }

                Ast_گرہ_DType* ثابت dtypep = varp->dtypeSkipRefp();
                ثابت صحیح sigbytes = dtypep->چوڑائی_Align_بائٹیں();
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
        برائے (std::سمتیہ<AstVar*>* ثابت varpsp : {&trivps, &nontps}) {
            اگر (varpsp->خالی_خانے()) جاری;
            اگر (!v3Global.opt.mtasks()) {
                simple_ترتیب_دو_Vars(*varpsp);
            } ورنہ {
                mtask_ترتیب_دو_Vars(*varpsp);
            }
        }

        // Insert them back under the module, in the new order, but at
        // the front of the list so they come out first in dumps/JSON.
        Ast_گرہ* nodesp = خالی_اشاریہ;
        برائے (ثابت std::سمتیہ<AstVar*>* ثابت varpsp : {&portps, &trivps, &nontps, &restps}) {
            برائے (AstVar* ثابت varp : *varpsp) {
                nodesp = Ast_گرہ::جوڑو_اگلا_Null(nodesp, varp->unlink_Fr_عقب());
            }
        }
        modp->prependStmtsp(nodesp);
    }

عوامی:
    ساکن خالی apply(Ast_گرہ_Module* modp, ثابت MTaskWritersMap& mTaskWriters) {
        Variable_ترتیب{modp, mTaskWriters};
    }
};

//######################################################################
// V3VariableOrder static functions

خالی V3_Variable_ترتیب::ترتیب_All(AstNetlist* netlistp) {
    UINFO(2, __FUNCTION__ << ":");

    MTaskWritersMap mTaskWriters;

    // Gather writing MTasks
    اگر (v3Global.opt.mtasks()) {
        netlistp->چوٹی_Modulep()->foreach([&](Ast_Exec_گراف* execGraphp) {
            برائے (ثابت V3_گراف_راس& vtx : execGraphp->depGraphp()->vertices()) {
                GatherMTaskWriters::apply(vtx.as<ثابت ExecMTask>(), mTaskWriters);
            }
        });
    }

    // Sort variables of each module
    برائے (Ast_گرہ_Module* modp = v3Global.rootp()->modulesp(); modp;
         modp = VN_AS(modp->nextp(), گرہ_Module)) {
        اگر (modp->isConstPool()) جاری;
        Variable_ترتیب::apply(modp, mTaskWriters);
    }

    V3Global::dump_Check_Global_درخت("variableorder", 0, dump_درخت_Either_Level() >= 3);
}
