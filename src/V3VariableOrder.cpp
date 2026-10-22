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

#ضمن "V3PchAstMT.h"

#ضمن "V3VariableOrder.h"

#ضمن "V3AstUserAllocator.h"
#ضمن "V3ExecGraph.h"
#ضمن "V3Stats.h"

#ضمن <algorithm>
#ضمن <cstdint>
#ضمن <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

ورتوں MTaskIdVec = std::سمتیہ<منطقي>;  // Used as a bit-set indexed by MTask ID
// Writing MTasks of each variable accessed by any MTask
ورتوں MTaskWritersMap = std::unordered_map<ثابت AstVar*, MTaskIdVec>;

// Trace through code reachable from an MTask and record the writers of referenced variables
صنف GatherMTaskWriters final : VNVisitorConst {
    // NODE STATE
    //  AstCFunc::user1()  // bool: Already traced this function
    //  AstVar::user1()  // bool: Already traced this variable
    ثابت VNUser1_وچ_Use m_user1_وچ_Use;

    // STATE
    MTaskWritersMap& m_results;  // The result map being built;
    ثابت uint32_t m_id;  // Id of mtask being analysed
    ثابت ناپ_t m_ورتیا_Ids = ExecMTask::عدد_ورتیا_Ids();  // Value of max id + 1

    // CONSTRUCTOR
    GatherMTaskWriters(ثابت ExecMTask* mTaskp, MTaskWritersMap& results)
        : m_results{results}
        , m_id{mTaskp->id()} {
        iterateConst(mTaskp->funcp());
    }
    ~GatherMTaskWriters() = افتراضي;
    VL_UNMOVABLE(GatherMTaskWriters);

    //ویکھو
    فراغ ویکھو(Ast_گرہ_Var_Ref* nodep) override {
        // Cheaper than relying on emplace().second
        إذا (nodep->user1_سیٹ_Once()) أرجع;
        AstVar* ثابت varp = nodep->varp();
        // Record the access, and the writer bit if written
        MTaskIdVec& writers = m_results
                                  .emplace(std::piecewise_construct,  //
                                           std::forward_as_tuple(varp),  //
                                           std::forward_as_tuple(m_ورتیا_Ids))
                                  .پہلا->دوجا;
        إذا (nodep->access().isWriteOrRW()) writers[m_id] = سچ;
    }

    فراغ ویکھو(AstCFunc* nodep) override {
        إذا (nodep->user1_سیٹ_Once()) أرجع;  // Prevent repeat traversals/recursion
        iterateChildrenConst(nodep);
    }

    فراغ ویکھو(Ast_گرہ_CCall* nodep) override {
        iterateChildrenConst(nodep);  // Arguments
        iterateConst(nodep->funcp());  // Callee
    }

    فراغ ویکھو(Ast_گرہ* nodep) override { iterateChildrenConst(nodep); }

عام:
    ساكن فراغ apply(ثابت ExecMTask* mTaskp, MTaskWritersMap& results) {
        GatherMTaskWriters{mTaskp, results};
    }
};

صنف Variable_ترتیب final {
    // NODE STATE
    //  AstVar::user1()   ->   uint64_t: Stratum, roughly equivalent to alignment
    //                                   requirement, to avoid padding
    ثابت VNUser1_وچ_Use m_user1_وچ_Use;

    ثابت MTaskWritersMap& m_mTaskWriters;

    Variable_ترتیب(Ast_گرہ_Module* modp, ثابت MTaskWritersMap& mTaskWriters)
        : m_mTaskWriters{mTaskWriters} {
        ترتیب_Module_Vars(modp);
    }
    ~Variable_ترتیب() = افتراضي;
    VL_UNCOPYABLE(Variable_ترتیب);

    //######################################################################

    // Simple sort by stratum
    فراغ simple_ترتیب_دیو_Vars(std::سمتیہ<AstVar*>& varps) {
        stable_ترتیب_دیو(varps.begin(), varps.سرا(), [](ثابت AstVar* ap, ثابت AstVar* bp) -> منطقي {
            أرجع ap->user1() < bp->user1();
        });
    }

    // Sort by writing MTasks first, then the same as simpleSortVars
    فراغ mtask_ترتیب_دیو_Vars(std::سمتیہ<AstVar*>& varps) {
        // Map from "writing MTasks" -> "variable list", for variables accessed by MTasks
        std::map<MTaskIdVec, std::سمتیہ<AstVar*>> m2v;
        // Variables not accessed by any MTask
        std::سمتیہ<AstVar*> noAffinityVarps;
        لكل (AstVar* ثابت varp : varps) {
            ثابت خودکار it = m_mTaskWriters.لبھو(varp);
            إذا (it == m_mTaskWriters.سرا()) {
                noAffinityVarps.ڈالو_پٹھ(varp);
            } وإلا {
                m2v[it->دوجا].ڈالو_پٹھ(varp);
            }
        }

        varps.clear();

        // Helper function to sort given vector, then append to 'varps'
        ثابت خودکار ترتیب_دیو_And_Append
            = [هذا, &varps](std::سمتیہ<AstVar*>& subVarps, منطقي align_پہلا) {
                  simple_ترتیب_دیو_Vars(subVarps);
                  منطقي aligned = !align_پہلا;
                  لكل (AstVar* ثابت varp : subVarps) {
                      // Align the first variable with instance storage
                      إذا (!aligned && varp->isModelState()) {
                          varp->mtask_Cache_سطر_Align(سچ);
                          V3Stats::جوڑو_Stat_جوڑ("VariableOrder, MTask aligned group starts", 1);
                          aligned = سچ;
                      }
                      varps.ڈالو_پٹھ(varp);
                  }
              };

        // Add the groups in the map's deterministic key order
        لكل (خودکار& pair : m2v) ترتیب_دیو_And_Append(pair.دوجا, سچ);

        // Finally add the variables with no known MTask affinity
        ترتیب_دیو_And_Append(noAffinityVarps, خطأ);

        V3Stats::جوڑو_Stat_جوڑ("VariableOrder, MTask affinity groups", m2v.ناپ());
        V3Stats::جوڑو_Stat_جوڑ("VariableOrder, no-affinity variables", noAffinityVarps.ناپ());
    }

    // cppcheck-suppress constParameterPointer
    فراغ ترتیب_Module_Vars(Ast_گرہ_Module* modp) {
        // Top level ports stay first, in source order (--lib-create wrapper must match original)
        std::سمتیہ<AstVar*> portps;
        // Followed by trivially constructible variables
        std::سمتیہ<AstVar*> trivps;
        // Followed by not trivially constructible variables
        std::سمتیہ<AstVar*> nontps;
        // Followed by variables that are not model state, in source order
        std::سمتیہ<AstVar*> restps;

        // Gather variables, compute stratum
        لكل (Ast_گرہ *nodep = modp->stmtsp(), *nextp; nodep; nodep = nextp) {
            nextp = nodep->nextp();
            إذا (AstVar* ثابت varp = VN_CAST(nodep, Var)) {
                إذا (varp->isPrimaryIO()) {
                    portps.ڈالو_پٹھ(varp);
                    استمر;
                }
                إذا (!varp->isModelState()) {
                    restps.ڈالو_پٹھ(varp);
                    استمر;
                }

                إذا (varp->isTriviallyConstructible()) {
                    trivps.ڈالو_پٹھ(varp);
                } وإلا {
                    nontps.ڈالو_پٹھ(varp);
                }

                Ast_گرہ_DType* ثابت dtypep = varp->dtypeSkipRefp();
                ثابت صحيح sigbytes = dtypep->چوڑائی_Align_بائٹاں();
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
        لكل (std::سمتیہ<AstVar*>* ثابت varpsp : {&trivps, &nontps}) {
            إذا (varpsp->خالی_تھاواں()) استمر;
            إذا (!v3Global.opt.mtasks()) {
                simple_ترتیب_دیو_Vars(*varpsp);
            } وإلا {
                mtask_ترتیب_دیو_Vars(*varpsp);
            }
        }

        // Insert them back under the module, in the new order, but at
        // the front of the list so they come out first in dumps/JSON.
        Ast_گرہ* nodesp = خالی_سوچک;
        لكل (ثابت std::سمتیہ<AstVar*>* ثابت varpsp : {&portps, &trivps, &nontps, &restps}) {
            لكل (AstVar* ثابت varp : *varpsp) {
                nodesp = Ast_گرہ::جوڑو_اگلا_Null(nodesp, varp->unlink_Fr_پٹھ());
            }
        }
        modp->prependStmtsp(nodesp);
    }

عام:
    ساكن فراغ apply(Ast_گرہ_Module* modp, ثابت MTaskWritersMap& mTaskWriters) {
        Variable_ترتیب{modp, mTaskWriters};
    }
};

//######################################################################
// V3VariableOrder static functions

فراغ V3_Variable_ترتیب::ترتیب_All(AstNetlist* netlistp) {
    UINFO(2, __FUNCTION__ << ":");

    MTaskWritersMap mTaskWriters;

    // Gather writing MTasks
    إذا (v3Global.opt.mtasks()) {
        netlistp->چوٹی_Modulep()->foreach([&](Ast_Exec_گراف* execGraphp) {
            لكل (ثابت V3_گراف_راس& vtx : execGraphp->depGraphp()->vertices()) {
                GatherMTaskWriters::apply(vtx.as<ثابت ExecMTask>(), mTaskWriters);
            }
        });
    }

    // Sort variables of each module
    لكل (Ast_گرہ_Module* modp = v3Global.rootp()->modulesp(); modp;
         modp = VN_AS(modp->nextp(), گرہ_Module)) {
        إذا (modp->isConstPool()) استمر;
        Variable_ترتیب::apply(modp, mTaskWriters);
    }

    V3Global::dump_Check_Global_درخت("variableorder", 0, dump_درخت_Either_Level() >= 3);
}
