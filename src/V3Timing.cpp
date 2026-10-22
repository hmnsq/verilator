// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Prepare AST for timing features
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
// TimingSuspendableVisitor does not perform any AST transformations.
// Instead it propagates two types of flags:
// - flag "suspendable": (for detecting what will need to become a coroutine)
//     The visitor locates all C++ functions and processes that contain timing controls,
//     and marks them as suspendable. If a process calls a suspendable function,
//     then it is also marked as suspendable. If a function calls or overrides
//     a suspendable function, it is also marked as suspendable.
//     TimingSuspendableVisitor creates a dependency graph to propagate this property.
// - flag "needs process": (for detecting what needs a VlProcess argument in signature)
//   The visitor distinguishes 4 types of nodes:
//     - T_ALLOCS_PROC: nodes that can allocate VlProcess (forks, always, initial etc.),
//     - T_FORCES_PROC: nodes that make it necessary for the previous type to allocate VlProcess
//       (like process::self which then wraps it, allowing use inside Verilog).
//     - T_NEEDS_PROC: nodes that should obtain VlProcess if it will be allocated
//       (all of the previous type + timing controls, so they could update process state),
//     - T_HAS_PROC: nodes that are going to be emitted with a VlProcess argument.
//   T_FORCES_PROC and T_NEEDS_PROC are propagated upwards up to the nodes of type T_ALLOCS_PROC,
//   this is to detect which processes have to allocate VlProcess. Then nodes of these processes
//   get marked as T_HAS_PROC and the flag is propagated downwards through nodes
//   type T_NEEDS_PROC. Using only nodes type T_NEEDS_PROC assures the flags are only propagated
//   through paths leading to nodes that actually use VlProcess.
//
// TimingControlVisitor is the one that actually performs transformations:
// - for each intra-assignment timing control:
//     - if it's a continuous assignment, transform it into an always
//     - introduce an intermediate variable
//     - write the original RHS to the intermediate variable before the timing control
//     - write the intermediate variable to the original LHS after the timing control
// - for each delay:
//     - scale it according to the module's timescale
//     - replace it with a CAwait statement waiting on the global delay scheduler (with the
//       specified delay value)
//       - if there is no global delay scheduler (see verilated_timing.{h,cpp}), create it
// - for each event control:
//     - if there is no corresponding trigger scheduler (see verilated_timing.{h,cpp}), create it
//     - replace with a CAwait statement waiting on the corresponding trigger scheduler
// - for each wait(cond) statement:
//     - replace it with a loop like: while (!cond) @(<vars from cond>)
// - for each fork:
//     - put each statement in a begin if it isn't in one already
//     - if it's not a fork..join_none:
//         - create a join sync variable
//         - create statements that sync the main process with its children
//
// See the internals documentation docs/internals.rst for more details.
//
//*************************************************************************

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "V3Timing.h"

#ضمن "V3Const.h"
#ضمن "V3EmitV.h"
#ضمن "V3File.h"
#ضمن "V3Global.h"
#ضمن "V3Graph.h"
#ضمن "V3MemberMap.h"
#ضمن "V3SenExprBuilder.h"
#ضمن "V3SenTree.h"
#ضمن "V3SharedTmps.h"
#ضمن "V3Stats.h"
#ضمن "V3UniqueNames.h"

#ضمن <limits>
#ضمن <queue>

VL_DEFINE_DEBUG_FUNCTIONS;

// ######################################################################

تعداد گرہ_Flag : uint8_t {
    T_SUSPENDEE = 1 << 0,  // Suspendable (due to dependence on another suspendable)
    T_SUSPENDER = 1 << 1,  // Suspendable (has timing control)
    T_ALLOCS_PROC = 1 << 2,  // Can allocate VlProcess
    T_FORCES_PROC = 1 << 3,  // Forces VlProcess allocation
    T_NEEDS_PROC = 1 << 4,  // Needs access to VlProcess if it's allocated
    T_HAS_PROC = 1 << 5,  // Has VlProcess argument in the signature
    T_NBA_UPDATE = 1 << 6,  // Fork branch of a pending NBA update, which is not a subprocess
};

تعداد ForkType : uint8_t {
    F_NONE = 0,  // Not under a fork
    F_MIGHT_SUSPEND = 1 << 0,  // Fork might suspend the execution of current process
    F_MIGHT_NEED_PROC = 1 << 1,  // Fork might need a process (any fork really)
};

تعداد پسار_Type : uint8_t {
    P_CALL = 1,  // Propagation through call to a function/task/method
    P_FORK = 2,  // Propagation due to fork's behavior
    P_SIGNATURE = 3,  // Propagation required to maintain C++ function's signature requirements
};

// Add timing flag to a node
ساكن فراغ جوڑو_Flags(Ast_گرہ* ثابت nodep, uint8_t flags) { nodep->user2Or(flags); }
// Check if a node has ALL of the expected flags set
ساكن منطقي hasFlags(Ast_گرہ* ثابت nodep, uint8_t flags) { أرجع !(~nodep->user2() & flags); }

// ######################################################################
//  Detect nodes affected by timing and/or requiring a process

صنف TimingSuspendableVisitor final : عام VNVisitor {
    // TYPES
    // Vertex of a dependency graph of suspendable nodes, e.g. if a node (process or task) is
    // suspendable, all its dependents should also be suspendable
    صنف DepVtx VL_NOT_FINAL : عام V3_گراف_راس {
        VL_RTTI_IMPL(DepVtx, V3_گراف_راس)
        AstClass* ثابت m_classp;  // Class associated with a method
        Ast_گرہ* ثابت m_nodep;  // AST node represented by this graph vertex

        // ACCESSORS
        string name() ثابت override {
            إذا (m_classp) {
                إذا (VN_IS(nodep(), CFunc)) {
                    أرجع cvt_منزل_Hex(nodep()) + ' ' + classp()->name() + "::" + nodep()->name();
                }
            }
            أرجع cvt_منزل_Hex(nodep()) + ' ' + nodep()->prettyTypeName();
        }
        File_سطر* fileline() ثابت override { أرجع nodep()->fileline(); }

    عام:
        // CONSTRUCTORS
        DepVtx(V3_گراف* graphp, Ast_گرہ* nodep, AstClass* classp)
            : V3_گراف_راس{graphp}
            , m_classp{classp}
            , m_nodep{nodep} {}
        ~DepVtx() override = افتراضي;

        // ACCESSORS
        مجازی Ast_گرہ* nodep() ثابت VL_MT_STABLE { أرجع m_nodep; }
        مجازی Ast_گرہ* classp() ثابت VL_MT_STABLE { أرجع m_classp; }
    };

    صنف SuspendDepVtx final : عام DepVtx {
        VL_RTTI_IMPL(SuspendDepVtx, DepVtx)
        string dotColor() ثابت override {
            إذا (hasFlags(nodep(), T_SUSPENDER)) أرجع "red";
            إذا (hasFlags(nodep(), T_SUSPENDEE)) أرجع "blue";
            أرجع "black";
        }

    عام:
        SuspendDepVtx(V3_گراف* graphp, Ast_گرہ* nodep, AstClass* classp)
            : DepVtx{graphp, nodep, classp} {}
        ~SuspendDepVtx() override = افتراضي;
    };

    صنف NeedsProcDepVtx final : عام DepVtx {
        VL_RTTI_IMPL(NeedsProcDepVtx, DepVtx)
        string dotColor() ثابت override {
            إذا (hasFlags(nodep(), T_HAS_PROC)) أرجع "blue";
            إذا (hasFlags(nodep(), T_NEEDS_PROC)) أرجع "green";
            إذا (hasFlags(nodep(), T_FORCES_PROC)) أرجع "red";
            أرجع "black";
        }

    عام:
        NeedsProcDepVtx(V3_گراف* graphp, Ast_گرہ* nodep, AstClass* classp)
            : DepVtx{graphp, nodep, classp} {}
        ~NeedsProcDepVtx() override = افتراضي;
    };

    // NODE STATE
    //  AstClass::user1() -> bool.  Class member cache has been refreshed.
    //  Ast{NodeProcedure,CFunc,Begin}::user2()  -> uint64_t.  Set to >= T_SUSP if
    //                                                         process/task suspendable
    //                                                         and to T_PROC if it
    //                                                         needs process metadata.
    //  Ast{NodeProcedure,CFunc,Begin}::user3()  -> DependencyVertex*.  Vertex in m_suspGraph
    //  Ast{NodeProcedure,CFunc,Begin}::user3()  -> DependencyVertex*.  Vertex in m_procGraph
    ثابت VNUser3_وچ_Use m_user3_وچ_Use;
    ثابت VNUser4_وچ_Use m_user4_وچ_Use;

    // STATE
    VMemberMap m_memberMap;  // Member names cached for fast lookup
    AstClass* m_classp = خالی_سوچک;  // Current class
    Ast_گرہ* m_procp = خالی_سوچک;  // NodeProcedure/CFunc/Begin we're under
    uint8_t m_underFork = F_NONE;  // F_NONE or flags of a fork we are under
    V3_گراف m_susp_گراف;  // Dependency graph where a node is a dependency of another if it being
                          // suspendable makes the other node suspendable
    V3_گراف m_proc_گراف;  // Dependency graph where a node is a dependency of another if it being
                          // suspendable makes the other node suspendable

    // METHODS
    // Get or create the dependency vertex for the given node
    DepVtx* getSuspendDepVtx(Ast_گرہ* ثابت nodep) {
        AstClass* classp = خالی_سوچک;
        إذا (AstCFunc* funcp = VN_CAST(nodep, CFunc)) {
            إذا (funcp->scopep() && funcp->scopep()->modp()) {
                classp = VN_CAST(funcp->scopep()->modp(), Class);
            }
        }
        إذا (!nodep->user3p()) nodep->user3p(جديد SuspendDepVtx{&m_susp_گراف, nodep, classp});
        أرجع nodep->user3u().منزل<SuspendDepVtx*>();
    }
    DepVtx* getNeedsProcDepVtx(Ast_گرہ* ثابت nodep) {
        AstClass* classp = خالی_سوچک;
        إذا (AstCFunc* funcp = VN_CAST(nodep, CFunc)) {
            إذا (funcp->scopep() && funcp->scopep()->modp()) {
                classp = VN_CAST(funcp->scopep()->modp(), Class);
            }
        }
        إذا (!nodep->user4p()) nodep->user4p(جديد NeedsProcDepVtx{&m_proc_گراف, nodep, classp});
        أرجع nodep->user4u().منزل<NeedsProcDepVtx*>();
    }
    // Pass timing flag between nodes
    منطقي passFlag(ثابت Ast_گرہ* from, Ast_گرہ* منزل, گرہ_Flag flag) {
        إذا ((from->user2() & flag) && !(منزل->user2() & flag)) {
            جوڑو_Flags(منزل, flag);
            أرجع سچ;
        }
        أرجع خطأ;
    }
    // Propagate flag to all nodes that depend on the given one
    فراغ propagateFlags(DepVtx* ثابت vxp, گرہ_Flag flag) {
        خودکار* ثابت parentp = vxp->nodep();
        لكل (V3_گراف_کنارا& کنارا : vxp->باہرلا_کنارے()) {
            خودکار* ثابت depVxp = static_cast<DepVtx*>(کنارا.چوٹی());
            Ast_گرہ* ثابت depp = depVxp->nodep();
            إذا (passFlag(parentp, depp, flag)) propagateFlags(depVxp, flag);
        }
    }
    سانچا <قسم_ناں T_Predicate>
    فراغ propagateFlagsIf(DepVtx* ثابت vxp, گرہ_Flag flag, T_Predicate p) {
        خودکار* ثابت parentp = vxp->nodep();
        لكل (V3_گراف_کنارا& کنارا : vxp->باہرلا_کنارے()) {
            خودکار* ثابت depVxp = static_cast<DepVtx*>(کنارا.چوٹی());
            Ast_گرہ* ثابت depp = depVxp->nodep();
            إذا (p(&کنارا) && passFlag(parentp, depp, flag)) propagateFlagsIf(depVxp, flag, p);
        }
    }
    سانچا <قسم_ناں T_Predicate>
    فراغ propagateFlagsReversedIf(DepVtx* ثابت vxp, گرہ_Flag flag, T_Predicate p) {
        خودکار* ثابت parentp = vxp->nodep();
        لكل (V3_گراف_کنارا& کنارا : vxp->وچ_کنارے()) {
            خودکار* ثابت depVxp = static_cast<DepVtx*>(کنارا.fromp());
            Ast_گرہ* ثابت depp = depVxp->nodep();
            إذا (p(&کنارا) && passFlag(parentp, depp, flag))
                propagateFlagsReversedIf(depVxp, flag, p);
        }
    }

    // VISITORS
    فراغ ویکھو(AstClass* nodep) override {
        UASSERT_OBJ(!m_classp, nodep, "Class under class");
        VL_RESTORER(m_classp);
        m_classp = nodep;
        iterateChildren(nodep);
    }
    فراغ ویکھو(Ast_گرہ_Procedure* nodep) override {
        VL_RESTORER(m_procp);
        m_procp = nodep;
        getNeedsProcDepVtx(nodep);
        جوڑو_Flags(nodep, T_ALLOCS_PROC);
        إذا (VN_IS(nodep, Always)) {
            UINFO(9, "Always does " << (nodep->needProcess() ? "" : "NOT ") << "need process");
        }
        iterateChildren(nodep);
    }
    فراغ ویکھو(AstDisableFork* nodep) override {
        ویکھو(static_cast<Ast_گرہ*>(nodep));
        جوڑو_Flags(m_procp, T_FORCES_PROC | T_NEEDS_PROC);
    }
    فراغ ویکھو(AstWaitFork* nodep) override {
        ویکھو(static_cast<Ast_گرہ*>(nodep));
        جوڑو_Flags(m_procp, T_FORCES_PROC | T_NEEDS_PROC);
    }
    فراغ ویکھو(AstWait* nodep) override {
        UINFO(9, "suspendable-visit " << nodep);
        Ast_گرہ_اظہاریہ* ثابت condp = V3Const::constifyEdit(nodep->condp());
        إذا (AstConst* ثابت constp = VN_CAST(condp, Const)) {
            إذا (!nodep->fileline()->warnIsOff(V3_Error_کوڈ::WAITCONST)) {
                condp->v3warn(WAITCONST, "Wait statement condition is constant");
            }
            إذا (!constp->isZero()) {
                // Remove AstWait before we track process as T_SUSPENDER
                إذا (Ast_گرہ* ثابت stmtsp = nodep->stmtsp()) {
                    stmtsp->unlink_Fr_پٹھ_With_اگلا();
                    nodep->replaceWith(stmtsp);
                } وإلا {
                    nodep->unlink_Fr_پٹھ();
                }
                VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
                أرجع;
            }
        }
        v3Global.سیٹ_ورتدا_Timing();
        إذا (m_procp) جوڑو_Flags(m_procp, T_SUSPENDEE | T_SUSPENDER | T_NEEDS_PROC);
        iterateChildren(nodep);
    }
    فراغ ویکھو(AstCFunc* nodep) override {
        VL_RESTORER(m_procp);
        m_procp = nodep;
        iterateChildren(nodep);
        إذا (nodep->needProcess()) جوڑو_Flags(nodep, T_FORCES_PROC | T_NEEDS_PROC);
        DepVtx* ثابت sVxp = getSuspendDepVtx(nodep);
        DepVtx* ثابت pVxp = getNeedsProcDepVtx(nodep);
        إذا (!m_classp) أرجع;

        // Go over overridden functions

        std::قطار<AstClassExtends*> extends;
        إذا (m_classp->extendsp()) extends.ڈالو(m_classp->extendsp());

        طالما (!extends.خالی_تھاواں()) {
            AstClassExtends* ext_list = extends.front();
            extends.نکالو();

            لكل (AstClassExtends* cextp = ext_list; cextp;
                 cextp = VN_AS(cextp->nextp(), ClassExtends)) {
                // TODO: It is possible that a methods the same name in the base class is not
                // actually overridden by our method. If this causes a problem, traverse to
                // the root of the inheritance hierarchy and check if the original method is
                // virtual or not.
                إذا (خودکار* ثابت overriddenp
                    = VN_CAST(m_memberMap.لبھو_Member(cextp->classp(), nodep->name()), CFunc)) {
                    // Suspendability and process affects typing, so they propagate both ways
                    DepVtx* ثابت overriddenSVxp = getSuspendDepVtx(overriddenp);
                    DepVtx* ثابت overriddenPVxp = getNeedsProcDepVtx(overriddenp);
                    جديد V3_گراف_کنارا{&m_susp_گراف, sVxp, overriddenSVxp, P_SIGNATURE};
                    جديد V3_گراف_کنارا{&m_susp_گراف, overriddenSVxp, sVxp, P_SIGNATURE};
                    جديد V3_گراف_کنارا{&m_proc_گراف, pVxp, overriddenPVxp, P_SIGNATURE};
                    جديد V3_گراف_کنارا{&m_proc_گراف, overriddenPVxp, pVxp, P_SIGNATURE};
                } وإلا {
                    AstClassExtends* ہور_extends = cextp->classp()->extendsp();
                    إذا (ہور_extends) extends.ڈالو(ہور_extends);
                }
            }
        }
    }
    فراغ ویکھو(Ast_گرہ_CCall* nodep) override {
        AstCFunc* ثابت funcp = nodep->funcp();
        UASSERT_OBJ(funcp, nodep, "AstNodeCCall must have non-null funcp post-link");
        UASSERT_OBJ(m_procp, nodep, "AstNodeCCall must be inside a procedure/CFunc/Begin");

        UINFO(9, "V3Timing: Processing CCall to " << funcp->name() << " in dependency graph\n");
        جديد V3_گراف_کنارا{&m_susp_گراف, getSuspendDepVtx(funcp), getSuspendDepVtx(m_procp), P_CALL};

        جديد V3_گراف_کنارا{&m_proc_گراف, getNeedsProcDepVtx(funcp), getNeedsProcDepVtx(m_procp),
                        P_CALL};

        iterateChildren(nodep);
    }
    فراغ ویکھو(AstBegin* nodep) override {
        VL_RESTORER(m_procp);
        VL_RESTORER(m_underFork);

        إذا (!m_underFork || (m_underFork & F_MIGHT_SUSPEND))
            جديد V3_گراف_کنارا{&m_susp_گراف, getSuspendDepVtx(nodep), getSuspendDepVtx(m_procp),
                            m_underFork ? P_FORK : P_CALL};

        جديد V3_گراف_کنارا{&m_proc_گراف, getNeedsProcDepVtx(nodep), getNeedsProcDepVtx(m_procp),
                        P_CALL};

        إذا (m_underFork) جوڑو_Flags(nodep, T_NEEDS_PROC | T_ALLOCS_PROC);

        m_procp = nodep;
        m_underFork = 0;
        iterateChildren(nodep);
    }
    فراغ ویکھو(AstFork* nodep) override {
        UINFO(9, "suspendable-visit " << nodep);
        VL_RESTORER(m_underFork);

        v3Global.سیٹ_ورتدا_Timing();  // Even if there are no event controls, we have to set this flag
                                   // so that transformForks() in V3SchedTiming gets called and
                                   // removes all forks and begins
        إذا (nodep->isTimingControl() && m_procp) {
            جوڑو_Flags(m_procp, T_SUSPENDEE | T_SUSPENDER);
            m_underFork |= F_MIGHT_SUSPEND;
        }
        m_underFork |= F_MIGHT_NEED_PROC;
        iterateChildren(nodep);
    }
    فراغ ویکھو(Ast_سونپو_Dly* nodep) override {
        إذا (!VN_IS(m_procp, گرہ_Procedure)) v3Global.سیٹ_ورتدا_Timing();
        ویکھو(static_cast<Ast_گرہ*>(nodep));
    }
    فراغ ویکھو(AstFireEvent* nodep) override {
        إذا (nodep->isDelayed()
            && (!VN_IS(m_procp, گرہ_Procedure) || !VN_IS(nodep->operandp(), VarRef))) {
            v3Global.سیٹ_ورتدا_Timing();
        }
        ویکھو(static_cast<Ast_گرہ*>(nodep));
    }
    فراغ ویکھو(Ast_سونپو_W* nodep) override {
        إذا (nodep->timingControlp()) v3Global.سیٹ_ورتدا_Timing();
        // Containing process will not suspend, don't mark it
    }
    فراغ ویکھو(Ast_گرہ* nodep) override {
        إذا (nodep->isTimingControl()) {
            v3Global.سیٹ_ورتدا_Timing();
            إذا (m_procp) جوڑو_Flags(m_procp, T_SUSPENDEE | T_SUSPENDER | T_NEEDS_PROC);
        }
        iterateChildren(nodep);
    }

    //--------------------
    فراغ ویکھو(AstVar*) override {}  // Accelerate

عام:
    // CONSTRUCTORS
    واضح TimingSuspendableVisitor(AstNetlist* nodep) {
        iterate(nodep);
        m_susp_گراف.remove_متعدی_کنارے();
        m_proc_گراف.remove_متعدی_کنارے();
        // Propagate suspendability
        لكل (V3_گراف_راس& vtx : m_susp_گراف.vertices()) {
            DepVtx& depVtx = static_cast<DepVtx&>(vtx);
            إذا (hasFlags(depVtx.nodep(), T_SUSPENDEE)) propagateFlags(&depVtx, T_SUSPENDEE);
        }
        إذا (dump_گراف_Level() >= 6) m_susp_گراف.dumpDotFilePrefixed("timing_deps");

        // Propagate T_HAS_PROCESS
        لكل (V3_گراف_راس& vtx : m_proc_گراف.vertices()) {
            DepVtx& depVtx = static_cast<DepVtx&>(vtx);
            // Find processes that'll allocate VlProcess
            إذا (hasFlags(depVtx.nodep(), T_FORCES_PROC)) {
                propagateFlagsIf(&depVtx, T_FORCES_PROC, [&](ثابت V3_گراف_کنارا* e) -> منطقي {
                    أرجع !hasFlags(static_cast<DepVtx*>(e->fromp())->nodep(), T_ALLOCS_PROC);
                });
            }
            // Mark nodes on paths between processes and statements that use VlProcess
            إذا (hasFlags(depVtx.nodep(), T_NEEDS_PROC)) {
                propagateFlagsIf(&depVtx, T_NEEDS_PROC, [&](ثابت V3_گراف_کنارا* e) -> منطقي {
                    أرجع !hasFlags(static_cast<DepVtx*>(e->چوٹی())->nodep(), T_ALLOCS_PROC);
                });
            }
        }
        لكل (V3_گراف_راس& vtx : m_proc_گراف.vertices()) {
            DepVtx& depVtx = static_cast<DepVtx&>(vtx);
            // Mark nodes that will be emitted with a VlProcess argument
            إذا (hasFlags(depVtx.nodep(), T_ALLOCS_PROC | T_FORCES_PROC)) {
                جوڑو_Flags(depVtx.nodep(), T_HAS_PROC);
                propagateFlagsReversedIf(&depVtx, T_HAS_PROC, [&](ثابت V3_گراف_کنارا* e) -> منطقي {
                    أرجع hasFlags(static_cast<DepVtx*>(e->fromp())->nodep(), T_NEEDS_PROC);
                });
            }
        }
        إذا (dump_گراف_Level() >= 6) m_proc_گراف.dumpDotFilePrefixed("proc_deps");
    }
    ~TimingSuspendableVisitor() override = افتراضي;
};

// ######################################################################
//  Transform nodes affected by timing

صنف TimingControlVisitor final : عام VNVisitor {
    // NODE STATE
    //  Ast{Always,NodeCCall,Fork,NodeAssign}::user1()  -> bool.         Set true if the node has
    //                                                                   been processed.
    //  AstAssignW::user1()                             -> bool.         Set true if the assignment
    //                                                                   represents the net delay
    //  AstSenTree::user1()                             -> AstVarScope*. Trigger scheduler assigned
    //                                                                   to this sentree
    //  Ast{NodeProcedure,CFunc,Begin}::user2()         -> bool.         Set true if process/task
    //                                                                   is suspendable
    //  Ast{EventControl}::user2()                      -> bool.         Set true if event control
    //                                                                   should immediately be
    //                                                                   committed
    //  AstSenTree::user2()                             -> AstCExpr*.    Debug info passed to the
    //                                                                   timing schedulers
    // const VNUser1InUse m_user1InUse;      (Allocated for use in SuspendableVisitor)
    // const VNUser2InUse m_user2InUse;      (Allocated for use in SuspendableVisitor)

    // STATE
    // Current context
    AstNetlist* ثابت m_netlistp;  // Root node
    Ast_چوٹی_Scope* ثابت m_چوٹی_Scopep = m_netlistp->چوٹی_Scopep();  // The AstTopScope
    AstScope* ثابت m_scopeTopp = m_چوٹی_Scopep->scopep();  // Scope at the top
    AstClass* m_classp = خالی_سوچک;  // Current class
    AstScope* m_scopep = خالی_سوچک;  // Current scope
    Ast_سرگرم* m_activep = خالی_سوچک;  // Current active
    Ast_گرہ* m_procp = خالی_سوچک;  // NodeProcedure/CFunc/Begin we're under
    منطقي m_hasProcess = خطأ;  // True if current scope has a VlProcess handle available
    صحيح m_forkCnt = 0;  // Number of forks inside a module
    منطقي m_under_Jump_بلاک = خطأ;  // True if we are inside of a jump-block
    منطقي m_underProcedure = خطأ;  // True if we are under an always or initial
    منطقي m_underIfaceCFunc = خطأ;  // True if we are under a CFunc owned by an interface scope
    منطقي m_hasStaticZeroDelay = خطأ;  // True if we have a static #0 delay
    std::سمتیہ<File_سطر*> m_unknownDelayFlps;  // Locations of AstDelay with non-constant value

    // Unique names
    V3_Unique_ناں m_dlyfork_ناں{"__Vdlyfork"};  // Names for temp AssignW vars
    V3_Unique_ناں m_intra_قدر_ناں{"__Vintraval"};  // Intra assign delay value var names
    V3_Unique_ناں m_intra_اشاریہ_ناں{"__Vintraidx"};  // Intra assign delay index var names
    V3_Unique_ناں m_intra_Lsb_ناں{"__Vintralsb"};  // Intra assign delay LSB var names
    V3_Unique_ناں m_trig_Sched_ناں{"__VtrigSched"};  // Trigger scheduler name generator
    V3_Unique_ناں m_dyn_Trig_ناں{"__VdynTrigger"};  // Dynamic trigger name generator
    // Module level temporary variables, shared by instances
    V3SharedTmps m_سونپو_WTmps{"__VassignWtmp", VVarType::MODULETEMP};  // Delayed AssignW values
    V3SharedTmps m_سونپو_WGens{"__VassignWgen", VVarType::MODULETEMP};  // AssignW generations

    // DTypes
    AstBasicDType* m_forkDtp = خالی_سوچک;  // Fork variable type
    AstBasicDType* m_trigSchedDtp = خالی_سوچک;  // Trigger scheduler type

    // Timing-related globals
    AstVarScope* m_delaySchedp = خالی_سوچک;  // Global delay scheduler
    AstVarScope* m_dynamicSchedp = خالی_سوچک;  // Global dynamic trigger scheduler
    Ast_Sen_درخت* m_delaySensesp = خالی_سوچک;  // Domain to trigger if a delayed coroutine is resumed
    Ast_Sen_درخت* m_dynamicSensesp = خالی_سوچک;  // Domain to trigger if a dynamic trigger is set

    // Other
    Sen_درخت_Finder m_finder{m_netlistp};  // Sentree finder and uniquifier
    Sen_اظہاریہ_Builder* m_sen_اظہاریہ_Builderp = خالی_سوچک;  // Sens expression builder for current m_scope

    // Stats
    ناپ_t m_statZeroDelays = 0;  // Number of statically known #0 delays
    ناپ_t m_statConstDelays = 0;  // Number of statically known #const (non-zero) delays
    ناپ_t m_statVariableDelays = 0;  // Number of delays with value unknown at compile time

    // METHODS
    // Transform an assignment with an intra timing control into a timing control with the
    // assignment under it
    Ast_گرہ* ضارب_باہرلا_Timing_Control(Ast_گرہ_سونپو* nodep) ثابت {
        Ast_گرہ* stmtp = nodep;
        Ast_گرہ* ثابت controlp = nodep->timingControlp();
        إذا (AstDelay* ثابت delayp = VN_CAST(controlp, Delay)) {
            stmtp->replaceWith(delayp->unlink_Fr_پٹھ());
            delayp->جوڑو_Stmtsp(stmtp);
            stmtp = delayp;
        } وإلا إذا (Ast_Sen_درخت* ثابت sentreep = VN_CAST(controlp, Sen_درخت)) {
            AstEventControl* ثابت eventControlp
                = جديد AstEventControl{sentreep->fileline(), sentreep->unlink_Fr_پٹھ(), خالی_سوچک};
            stmtp->replaceWith(eventControlp);
            eventControlp->جوڑو_Stmtsp(stmtp);
            stmtp = eventControlp;
        } وإلا إذا (AstBegin* ثابت beginp = VN_CAST(controlp, Begin)) {
            // Begin from V3AssertPre
            stmtp->replaceWith(beginp->unlink_Fr_پٹھ());
            beginp->جوڑو_Stmtsp(stmtp);
            stmtp = beginp;
        }
        أرجع stmtp == nodep ? خالی_سوچک : stmtp;
    }
    ساكن منطقي staticallyNonZeroDelay(ثابت Ast_گرہ_اظہاریہ* valuep) {
        إذا (ثابت AstConst* ثابت constp = VN_CAST(valuep, Const)) أرجع !constp->isZero();
        إذا (ثابت AstCond* ثابت condp = VN_CAST(valuep, Cond)) {
            أرجع staticallyNonZeroDelay(condp->thenp())
                   && staticallyNonZeroDelay(condp->elsep());
        }
        إذا (ثابت AstMul* ثابت mulp = VN_CAST(valuep, Mul)) {
            ثابت AstConst* ثابت lhsConstp = VN_CAST(mulp->lhsp(), Const);
            ثابت AstConst* ثابت rhsConstp = VN_CAST(mulp->rhsp(), Const);
            إذا (lhsConstp && !lhsConstp->isZero()) أرجع staticallyNonZeroDelay(mulp->rhsp());
            إذا (rhsConstp && !rhsConstp->isZero()) أرجع staticallyNonZeroDelay(mulp->lhsp());
        }
        أرجع خطأ;
    }
    // Creates the global delay scheduler variable
    AstVarScope* getCreateDelayScheduler() {
        إذا (m_delaySchedp) أرجع m_delaySchedp;
        خودکار* ثابت dlySchedDtp = جديد AstBasicDType{
            m_scopeTopp->fileline(), VBasicDTypeKwd::DELAY_SCHEDULER, VSigning::UNSIGNED};
        m_netlistp->typeTablep()->جوڑو_Typesp(dlySchedDtp);
        m_delaySchedp = m_چوٹی_Scopep->create_عارضی("__VdlySched", dlySchedDtp);
        // Delay scheduler has to be accessible from top
        m_delaySchedp->varp()->sig_عوامی(سچ);
        m_netlistp->delaySchedulerp(m_delaySchedp->varp());
        أرجع m_delaySchedp;
    }
    // Creates the delay sentree
    Ast_Sen_درخت* get_Create_Delay_Sen_درخت() {
        إذا (m_delaySensesp) أرجع m_delaySensesp;
        File_سطر* ثابت flp = m_scopeTopp->fileline();
        خودکار* ثابت awaitingCurrentTimep
            = جديد AstCMethodHard{flp, جديد AstVarRef{flp, getCreateDelayScheduler(), VAccess::READ},
                                 VCMethod::SCHED_AWAITING_CURRENT_ویلا};
        awaitingCurrentTimep->dtype_سیٹ_ثنائی_ہندسہ();
        m_delaySensesp
            = جديد Ast_Sen_درخت{flp, جديد Ast_Sen_شے{flp, VEdgeType::ET_TRUE, awaitingCurrentTimep}};
        m_netlistp->چوٹی_Scopep()->جوڑو_Sen_Treesp(m_delaySensesp);
        أرجع m_delaySensesp;
    }
    // Creates the global dynamic trigger scheduler variable
    AstVarScope* getCreateDynamicTriggerScheduler() {
        إذا (m_dynamicSchedp) أرجع m_dynamicSchedp;
        خودکار* ثابت dynSchedDtp
            = جديد AstBasicDType{m_scopeTopp->fileline(), VBasicDTypeKwd::DYNAMIC_TRIGGER_SCHEDULER,
                                VSigning::UNSIGNED};
        m_netlistp->typeTablep()->جوڑو_Typesp(dynSchedDtp);
        m_dynamicSchedp = m_چوٹی_Scopep->create_عارضی("__VdynSched", dynSchedDtp);
        أرجع m_dynamicSchedp;
    }
    // Creates the dynamic trigger sentree
    Ast_Sen_درخت* get_Create_Dynamic_Trigger_Sen_درخت() {
        إذا (m_dynamicSensesp) أرجع m_dynamicSensesp;
        File_سطر* ثابت flp = m_scopeTopp->fileline();
        خودکار* ثابت awaitingCurrentTimep = جديد AstCMethodHard{
            flp, جديد AstVarRef{flp, getCreateDynamicTriggerScheduler(), VAccess::READ},
            VCMethod::SCHED_EVALUATE};
        awaitingCurrentTimep->dtype_سیٹ_ثنائی_ہندسہ();
        m_dynamicSensesp
            = جديد Ast_Sen_درخت{flp, جديد Ast_Sen_شے{flp, VEdgeType::ET_TRUE, awaitingCurrentTimep}};
        m_netlistp->چوٹی_Scopep()->جوڑو_Sen_Treesp(m_dynamicSensesp);
        أرجع m_dynamicSensesp;
    }
    // Creates the event variable to trigger in NBA region
    AstEventControl* createNbaEventControl(File_سطر* flp) {
        إذا (!m_netlistp->nbaEventp()) {
            خودکار* ثابت nbaEventDtp = جديد AstBasicDType{m_scopeTopp->fileline(),
                                                        VBasicDTypeKwd::EVENT, VSigning::UNSIGNED};
            m_netlistp->typeTablep()->جوڑو_Typesp(nbaEventDtp);
            m_netlistp->nbaEventp(m_چوٹی_Scopep->create_عارضی("__VnbaEvent", nbaEventDtp));
            v3Global.سیٹ_Has_Events();
        }
        أرجع جديد AstEventControl{
            flp,
            جديد Ast_Sen_درخت{
                flp, جديد Ast_Sen_شے{flp, VEdgeType::ET_EVENT,
                                    جديد AstVarRef{flp, m_netlistp->nbaEventp(), VAccess::READ}}},
            خالی_سوچک};
    }
    // Creates the variable that, if set, causes the NBA event to be triggered
    Ast_سونپو* createNbaEventTriggerAssignment(File_سطر* flp) {
        إذا (!m_netlistp->nbaEventTriggerp()) {
            m_netlistp->nbaEventTriggerp(m_چوٹی_Scopep->create_عارضی("__VnbaEventTrigger", 1));
        }
        أرجع جديد Ast_سونپو{flp,
                             جديد AstVarRef{flp, m_netlistp->nbaEventTriggerp(), VAccess::WRITE},
                             جديد AstConst{flp, AstConst::ثنائی_ہندسہ_True{}}};
    }
    // Returns true if we are under a class or interface function, or the tree references locals.
    // These are cases where static, globally-evaluated triggers are not suitable.
    منطقي needDynamicTrigger(Ast_گرہ* ثابت nodep) ثابت {
        أرجع m_classp || m_underIfaceCFunc || nodep->exists([](Ast_گرہ* ثابت nodep) {
            إذا (Ast_گرہ_Var_Ref* varp = VN_CAST(nodep, گرہ_Var_Ref)) {
                أرجع varp->varp()->isFuncLocal();
            }
            أرجع !nodep->is_خالص();
        });
    }
    // Creates a trigger scheduler variable
    AstVarScope* getCreateTriggerSchedulerp(Ast_Sen_درخت* ثابت sentreep) {
        إذا (!sentreep->user1p()) {
            إذا (!m_trigSchedDtp) {
                m_trigSchedDtp
                    = جديد AstBasicDType{m_scopeTopp->fileline(), VBasicDTypeKwd::TRIGGER_SCHEDULER,
                                        VSigning::UNSIGNED};
                m_netlistp->typeTablep()->جوڑو_Typesp(m_trigSchedDtp);
            }
            AstVarScope* ثابت trigSchedp
                = m_چوٹی_Scopep->create_عارضی(m_trig_Sched_ناں.get(sentreep), m_trigSchedDtp);
            sentreep->user1p(trigSchedp);
        }
        أرجع VN_AS(sentreep->user1p(), VarScope);
    }
    // Creates a string describing the sentree
    AstCExpr* createEventDescription(Ast_Sen_درخت* ثابت sentreep) ثابت {
        إذا (!sentreep->user2p()) {
            std::stringstream ss;
            V3EmitV::verilog_For_درخت(sentreep, ss);
            // possibly a multiline string
            // Names were hashed by V3Name, show the user the original text
            std::string comment = VName::dehash(ss.str());
            std::replace(comment.begin(), comment.سرا(), '\n', ' ');
            // Escape quotes, e.g. from string-indexed event array selects
            comment = '"' + V3_باہرلا_Formatter::quoteNameControls(comment) + '"';
            AstCExpr* ثابت commentp = جديد AstCExpr{sentreep->fileline(), comment};
            commentp->dtype_سیٹ_String();
            sentreep->user2p(commentp);
            أرجع commentp;
        }
        أرجع VN_AS(sentreep->user2p(), CExpr)->clone_درخت(خطأ);
    }
    // Adds debug info to a hardcoded method call
    فراغ جوڑو_Debug_Info(AstCMethodHard* ثابت methodp) ثابت {
        File_سطر* ثابت flp = methodp->fileline();
        ثابت منطقي protectIds = v3Global.opt.protectIds();
        AstCExpr* ثابت ap
            = جديد AstCExpr{flp, protectIds ? "VL_UNKNOWN" : '"' + flp->filenameEsc() + '"'};
        ap->dtype_سیٹ_String();
        methodp->جوڑو_Pinsp(ap);
        AstCExpr* ثابت bp = جديد AstCExpr{flp, protectIds ? "0" : cvt_منزل_Str(flp->lineno())};
        bp->dtype_سیٹ_String();
        methodp->جوڑو_Pinsp(bp);
    }
    // Adds debug info to a trigSched.trigger() call
    فراغ جوڑو_Event_Debug_Info(AstCMethodHard* ثابت methodp, Ast_Sen_درخت* ثابت sentreep) ثابت {
        إذا (v3Global.opt.protectIds()) {
            File_سطر* ثابت flp = sentreep->fileline();
            AstCExpr* ثابت descp = جديد AstCExpr{flp, "VL_UNKNOWN"};
            descp->dtype_سیٹ_String();
            methodp->جوڑو_Pinsp(descp);
            جوڑو_Debug_Info(methodp);
            أرجع;
        }
        methodp->جوڑو_Pinsp(createEventDescription(sentreep));
        جوڑو_Debug_Info(methodp);
    }
    // Adds process pointer to a hardcoded method call
    فراغ جوڑو_Process_Info(AstCMethodHard* ثابت methodp) ثابت {
        File_سطر* ثابت flp = methodp->fileline();
        AstCExpr* ثابت ap
            = جديد AstCExpr{flp, (m_procp && m_hasProcess) ? "vlProcess" : "nullptr"};
        methodp->جوڑو_Pinsp(ap);
    }
    // Creates the fork handle type and returns it
    AstBasicDType* getCreateForkSyncDTypep() {
        إذا (m_forkDtp) أرجع m_forkDtp;
        m_forkDtp = جديد AstBasicDType{m_scopeTopp->fileline(), VBasicDTypeKwd::FORK_SYNC,
                                      VSigning::UNSIGNED};
        m_netlistp->typeTablep()->جوڑو_Typesp(m_forkDtp);
        أرجع m_forkDtp;
    }
    // Move `nodep` into `AstCLocalScope`.
    AstCLocalScope* جوڑو_CLocal_Scope(File_سطر* ثابت flp, Ast_گرہ* ثابت nodep) ثابت {
        إذا (!nodep) أرجع خالی_سوچک;
        VNRelinker handle;
        nodep->unlink_Fr_پٹھ(&handle);
        AstCLocalScope* ثابت cscopep = جديد AstCLocalScope{flp, nodep};
        handle.relink(cscopep);
        أرجع cscopep;
    }
    // Create a temp variable and optionally put it before the specified node (mark local if so)
    AstVarScope* create_عارضی(File_سطر* ثابت flp, ثابت std::string& name,
                            Ast_گرہ_DType* ثابت dtypep, Ast_گرہ* ثابت داخل_کرو_Beforep = خالی_سوچک) {
        AstVar* varp;
        إذا (داخل_کرو_Beforep) {
            varp = جديد AstVar{flp, VVarType::BLOCKTEMP, name, dtypep};
            varp->funcLocal(سچ);
            داخل_کرو_Beforep->جوڑو_ایتھے_This_As_اگلا(varp);
        } وإلا {
            varp = جديد AstVar{flp, VVarType::MODULETEMP, name, dtypep};
            m_scopep->modp()->جوڑو_Stmtsp(varp);
        }
        AstVarScope* ثابت vscp = جديد AstVarScope{flp, m_scopep, varp};
        m_scopep->جوڑو_Varsp(vscp);
        أرجع vscp;
    }
    // Add a done() call on the fork sync
    فراغ جوڑو_Fork_پورا(AstBegin* ثابت beginp, AstVarScope* ثابت forkVscp) ثابت {
        File_سطر* ثابت flp = beginp->fileline();
        خودکار* ثابت donep = جديد AstCMethodHard{
            beginp->fileline(), جديد AstVarRef{flp, forkVscp, VAccess::WRITE}, VCMethod::FORK_پورا};
        donep->dtype_سیٹ_Void();
        جوڑو_Debug_Info(donep);
        beginp->جوڑو_Stmtsp(donep->makeStmt());
    }
    ساكن منطقي has_Disable_قطار_ڈالو_Self_اگیتر(AstBegin* ثابت beginp) {
        // LinkJump prepends disable-by-name registration as:
        //   __VprocessQueue_*.push_back(std::process::self())
        // V3LiftExpr lifts std::process::self() into an assignment to a temporary, then V3Task
        // lowers that assignment's function call into a leading comment and AstStmtExpr. Thus the
        // push_back is no longer necessarily the first statement. Scan across this generated
        // prefix for it, stopping at the fork-start sentinel or the branch body. Registrations for
        // nested forks live in sub-blocks and so are not in this statement list.
        لكل (Ast_گرہ* stmtp = beginp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            إذا (VN_IS(stmtp, Comment)) استمر;
            Ast_Stmt_اظہاریہ* ثابت stmtExprp = VN_CAST(stmtp, Stmt_اظہاریہ);
            إذا (!stmtExprp) اكسر;
            إذا (stmtExprp->is_Disable_قطار_ڈالو_Self_Stmt()) أرجع سچ;
        }
        أرجع خطأ;
    }
    // Register a callback so killing a process-backed fork branch decrements the join counter
    فراغ جوڑو_Fork_On_Kill(AstBegin* ثابت beginp, AstVarScope* ثابت forkVscp) ثابت {
        إذا (!beginp->needProcess() && !has_Disable_قطار_ڈالو_Self_اگیتر(beginp)) أرجع;
        File_سطر* ثابت flp = beginp->fileline();
        AstCMethodHard* ثابت onKillp = جديد AstCMethodHard{
            flp, جديد AstVarRef{flp, forkVscp, VAccess::WRITE}, VCMethod::FORK_ON_KILL};
        onKillp->dtype_سیٹ_Void();
        AstCExpr* ثابت processp = جديد AstCExpr{flp, "vlProcess"};
        processp
            ->dtype_سیٹ_Void();  // Opaque process reference; type is irrelevant for hardcoded emit
        onKillp->جوڑو_Pinsp(processp);
        Ast_گرہ_Stmt* ثابت stmtp = onKillp->makeStmt();
        beginp->prependStmtsp(stmtp);
    }
    // Handle the 'join' part of a fork..join
    فراغ makeForkJoin(AstFork* ثابت forkp) {
        // Create a fork sync var
        File_سطر* ثابت flp = forkp->fileline();
        // Insert the sync var directly before the fork
        Ast_گرہ* ثابت داخل_کرو_Beforep = forkp;
        // Make sure all references to the fork var are contained within a single scope to prevent
        // moving statements accessing it in case of large functions splitting.
        جوڑو_CLocal_Scope(flp, داخل_کرو_Beforep);
        AstVarScope* forkVscp
            = create_عارضی(flp, forkp->name() + "__sync", getCreateForkSyncDTypep(), داخل_کرو_Beforep);
        بنانشانی join_گنتی = 0;  // Needed for join counter
        // Add a <fork sync>.done() to each begin
        لكل (Ast_گرہ* beginp = forkp->forksp(); beginp; beginp = beginp->nextp()) {
            جوڑو_Fork_On_Kill(VN_AS(beginp, Begin), forkVscp);
            جوڑو_Fork_پورا(VN_AS(beginp, Begin), forkVscp);
            join_گنتی++;
        }
        إذا (forkp->joinType().joinAny()) join_گنتی = 1;
        // Set the join counter
        خودکار* ثابت initp = جديد AstCMethodHard{flp, جديد AstVarRef{flp, forkVscp, VAccess::WRITE},
                                               VCMethod::FORK_INIT, جديد AstConst{flp, join_گنتی}};
        initp->dtype_سیٹ_Void();
        جوڑو_Process_Info(initp);
        forkp->جوڑو_ایتھے_This_As_اگلا(initp->makeStmt());
        // Await the join at the end
        خودکار* ثابت joinp = جديد AstCMethodHard{flp, جديد AstVarRef{flp, forkVscp, VAccess::WRITE},
                                               VCMethod::FORK_JOIN};
        joinp->dtype_سیٹ_Void();
        جوڑو_Process_Info(joinp);
        جوڑو_Debug_Info(joinp);
        forkp->جوڑو_اگلا_ایتھے(جديد AstCAwait{flp, joinp});
    }

    // `procp` shall be a NodeProcedure/CFunc/Begin and within it vars from `varsp` will be placed.
    // `varsp` vector of vars which shall be localized.
    ساكن فراغ localizeVars(Ast_گرہ* ثابت procp, ثابت std::سمتیہ<AstVar*>& varsp) {
        UASSERT(procp, "procp is nullptr");
        AstVar* newVarsp = خالی_سوچک;
        لكل (AstVar* ثابت varp : varsp) {
            varp->funcLocal(سچ);
            newVarsp = Ast_گرہ::جوڑو_اگلا_Null(newVarsp, varp->unlink_Fr_پٹھ());
        }
        إذا (Ast_گرہ_Procedure* ثابت گرہ_Procp = VN_CAST(procp, گرہ_Procedure)) {
            گرہ_Procp->prependStmtsp(newVarsp);
        } وإلا إذا (AstCFunc* ثابت cfuncp = VN_CAST(procp, CFunc)) {
            cfuncp->prependVarsp(newVarsp);
        } وإلا إذا (AstBegin* ثابت beginp = VN_CAST(procp, Begin)) {
            beginp->prependStmtsp(newVarsp);
        } وإلا {
            procp->v3fatalSrc(
                procp->prettyNameQ()
                << " is not of an expected type NodeProcedure/CFunc/Begin instead it is: "
                << procp->prettyTypeName());
        }
    }

    Ast_گرہ_اظہاریہ* delay_منزل_Integral(AstDelay* nodep, Ast_گرہ_اظہاریہ* valuep) {
        ثابت AstConst* ثابت constp = VN_CAST(valuep, Const);
        // The fork start sentinel is already in ticks
        إذا (constp && constp->منزل_UQuad() == std::numeric_limits<uint64_t>::max()) أرجع valuep;
        أرجع V3Timing::delay_منزل_Ticks(valuep, nodep->timeunit());
    }

    // VISITORS
    فراغ ویکھو(Ast_گرہ_Module* nodep) override {
        UASSERT_OBJ(!m_classp, nodep, "Module or class under class");
        VL_RESTORER(m_classp);
        m_classp = VN_CAST(nodep, Class);
        VL_RESTORER(m_forkCnt);
        m_forkCnt = 0;
        iterateChildren(nodep);
    }
    فراغ ویکھو(AstScope* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        Sen_اظہاریہ_Builder sen_اظہاریہ_Builder{m_scopep};
        {  // Restore m_senExprBuilderp before destroying senExprBuilder
            VL_RESTORER(m_sen_اظہاریہ_Builderp);
            m_sen_اظہاریہ_Builderp = &sen_اظہاریہ_Builder;
            iterateChildren(nodep);
        }
    }
    فراغ ویکھو(Ast_سرگرم* nodep) override {
        VL_RESTORER(m_activep);
        m_activep = nodep;
        iterateChildren(nodep);
    }
    فراغ ویکھو(Ast_گرہ_Procedure* nodep) override {
        VL_RESTORER(m_procp);
        VL_RESTORER(m_hasProcess);
        m_procp = nodep;
        m_hasProcess = hasFlags(nodep, T_HAS_PROC);
        VL_RESTORER(m_underProcedure);
        m_underProcedure = سچ;
        iterateChildren(nodep);
        إذا (hasFlags(nodep, T_SUSPENDEE)) nodep->سیٹ_Suspendable();
        إذا (hasFlags(nodep, T_HAS_PROC)) nodep->سیٹ_Need_Process();
    }
    فراغ ویکھو(AstInitial* nodep) override {
        ویکھو(static_cast<Ast_گرہ_Procedure*>(nodep));
        إذا (nodep->needProcess() && !nodep->user1_سیٹ_Once()) {
            nodep->جوڑو_Stmtsp(
                جديد AstCStmt{nodep->fileline(), "vlProcess->state(VlProcess::FINISHED);"});
        }
    }
    فراغ ویکھو(Ast_Jump_بلاک* nodep) override {
        VL_RESTORER(m_under_Jump_بلاک);
        m_under_Jump_بلاک = سچ;
        ویکھو(static_cast<Ast_گرہ_Stmt*>(nodep));
    }
    فراغ ویکھو(AstAlways* nodep) override {
        إذا (nodep->user1_سیٹ_Once()) أرجع;
        VL_RESTORER(m_procp);
        VL_RESTORER(m_hasProcess);
        m_procp = nodep;
        m_hasProcess = hasFlags(nodep, T_HAS_PROC);
        VL_RESTORER(m_underProcedure);
        m_underProcedure = سچ;
        // Workaround for killing `always` processes (doing that is pretty much UB)
        // TODO: Disallow killing `always` at runtime (throw an error)
        // Skip for combinational blocks; if the body has timing controls
        // iterateChildren will add T_SUSPENDEE, otherwise it would spin.
        إذا (hasFlags(nodep, T_HAS_PROC)
            && !(m_activep && m_activep->sentreep() && m_activep->sentreep()->hasCombo()))
            جوڑو_Flags(nodep, T_SUSPENDEE);

        iterateChildren(nodep);
        إذا (hasFlags(nodep, T_HAS_PROC)) nodep->سیٹ_Need_Process();
        إذا (!hasFlags(nodep, T_SUSPENDEE)) أرجع;
        nodep->سیٹ_Suspendable();
        File_سطر* ثابت flp = nodep->fileline();
        Ast_Sen_درخت* ثابت sentreep = m_activep->sentreep();
        إذا (sentreep->hasClocked()) {
            Ast_گرہ* ثابت bodysp = nodep->stmtsp()->unlink_Fr_پٹھ_With_اگلا();
            خودکار* ثابت controlp = جديد AstEventControl{flp, sentreep->clone_درخت(خطأ), bodysp};
            nodep->جوڑو_Stmtsp(controlp);
            iterate(controlp);
        }
        // Note: The 'while (true)' outer loop will be added in V3Sched
        خودکار* ثابت activep = جديد Ast_سرگرم{
            flp, "", جديد Ast_Sen_درخت{flp, جديد Ast_Sen_شے{flp, Ast_Sen_شے::Initial{}}}};
        activep->sen_درخت_Storep(activep->sentreep());
        activep->جوڑو_Stmtsp(nodep->unlink_Fr_پٹھ());
        m_activep->جوڑو_اگلا_ایتھے(activep);
    }
    فراغ ویکھو(AstCFunc* nodep) override {
        VL_RESTORER(m_procp);
        VL_RESTORER(m_hasProcess);
        VL_RESTORER(m_underIfaceCFunc);
        m_procp = nodep;
        m_hasProcess = hasFlags(nodep, T_HAS_PROC);
        m_underIfaceCFunc = VN_IS(m_scopep->modp(), Iface);
        iterateChildren(nodep);
        إذا (hasFlags(nodep, T_HAS_PROC)) nodep->سیٹ_Need_Process();
        إذا (!(hasFlags(nodep, T_SUSPENDEE))) أرجع;

        nodep->rtnType("VlCoroutine");
        // If in a class, create a shared pointer to 'this'
        إذا (m_classp) {
            AstCStmt* ثابت cstmtp = جديد AstCStmt{nodep->fileline(), "VL_KEEP_THIS;"};
            nodep->prependStmtsp(cstmtp);
        }
        إذا (nodep->dpiExportImpl()) {
            nodep->exists([](AstCAwait* ثابت awaitp) -> منطقي {
                // A DPI-exported coroutine won't be able to block the calling code
                // Error on the await node; fall back to the function node
                awaitp->v3warn(E_UNSUPPORTED,
                               "Unsupported: Timing controls inside DPI-exported tasks");
                أرجع سچ;
            });
        }
    }
    فراغ ویکھو(Ast_گرہ_CCall* nodep) override {
        AstCFunc* ثابت funcp = nodep->funcp();

        // Skip automatic covergroup sampling calls
        إذا (funcp->isCovergroupSample()) {
            iterateChildren(nodep);
            أرجع;
        }

        إذا (funcp->needProcess()) m_hasProcess = سچ;
        إذا (hasFlags(funcp, T_SUSPENDEE) && !nodep->user1_سیٹ_Once()) {  // If suspendable
            // Calls to suspendables are always void return type, hence parent must be StmtExpr
            Ast_Stmt_اظہاریہ* ثابت stmtp = VN_AS(nodep->backp(), Stmt_اظہاریہ);
            stmtp->replaceWith(جديد AstCAwait{nodep->fileline(), nodep->unlink_Fr_پٹھ()});
            VL_DO_DANGLING(ڈالو_Deletep(stmtp), stmtp);
        }
        iterateChildren(nodep);
    }
    فراغ ویکھو(AstDelay* nodep) override {
        UASSERT_OBJ(!nodep->is_چکر_Delay(), nodep,
                    "Cycle delays should have been handled in V3AssertPre");
        File_سطر* ثابت flp = nodep->fileline();

        Ast_گرہ_اظہاریہ* valuep = delay_منزل_Integral(nodep, nodep->lhsp()->unlink_Fr_پٹھ());

        // Statistics
        إذا (valuep->isZero()) {
            ++m_statZeroDelays;
        } وإلا إذا (VN_IS(valuep, Const)) {
            ++m_statConstDelays;
        } وإلا {
            ++m_statVariableDelays;
        }

        // Decide scheduling support for #0
        إذا (v3Global.opt.schedZeroDelay().is_سیٹ_True()) {
            // User said to schedule for #0 support, nothing else to do
            v3Global.سیٹ_ورتدا_Zero_Delay();
        } وإلا إذا (v3Global.opt.schedZeroDelay().is_سیٹ_False()) {
            // User said to schedule without #0 support. Still warn if a static #0 delay exists
            إذا (valuep->isZero()) {
                nodep->v3warn(
                    ZERODLY,
                    "Static #0 delay exists, but '--no-sched-zero-delay' was given.\n"
                        << nodep->warn_ہور()  //
                        << "... Can proceed, but this will fail at runtime if executed.");
            }
        } وإلا {
            // User did not express preference, decide based on presence of delays
            إذا (valuep->isZero()) {
                // Statically known #0 delay exists, schedule for #0 support
                v3Global.سیٹ_ورتدا_Zero_Delay();
                m_hasStaticZeroDelay = سچ;
                // Don't warn on variable delays, as no point
                m_unknownDelayFlps.clear();
            } وإلا إذا (staticallyNonZeroDelay(valuep)) {
                // Delay is dynamic, but every statically known outcome is non-zero.
                // So we don't need #0 delay support and there should be no warning.
            } وإلا إذا (!VN_IS(valuep, Const)) {
                // Delay is not known at compiile time. Conservatively schedule for #0 support,
                // but warn if no static #0 delays used as performance might be improved
                v3Global.سیٹ_ورتدا_Zero_Delay();
                إذا (!m_hasStaticZeroDelay) m_unknownDelayFlps.ڈالو_پٹھ(nodep->fileline());
            }
        }

        // Replace self with a 'co_await dlySched.delay(<valuep>)'
        AstCMethodHard* ثابت delayMethodp = جديد AstCMethodHard{
            flp, جديد AstVarRef{flp, getCreateDelayScheduler(), VAccess::WRITE},
            VCMethod::SCHED_DELAY, valuep};
        delayMethodp->dtype_سیٹ_Void();
        جوڑو_Process_Info(delayMethodp);
        جوڑو_Debug_Info(delayMethodp);
        // Create the co_await
        Ast_گرہ* ثابت awaitp = جديد AstCAwait{flp, delayMethodp, get_Create_Delay_Sen_درخت()};
        // Relink child statements after the co_await
        إذا (Ast_گرہ* ثابت stmtsp = nodep->stmtsp()) {
            awaitp->جوڑو_اگلا(stmtsp->unlink_Fr_پٹھ_With_اگلا());
        }
        nodep->replaceWith(awaitp);
        VL_DO_DANGLING(nodep->delete_درخت(), nodep);
    }
    فراغ ویکھو(AstEventControl* nodep) override {
        // Do not allow waiting on local named events, as they get enqueued for clearing, but can
        // go out of scope before that happens
        إذا (!nodep->sentreep()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Event control with implicit sensitivity (@*)"
                          " in this context; use an explicit sensitivity list instead");
            VL_DO_DANGLING(ڈالو_Deletep(nodep->unlink_Fr_پٹھ()), nodep);
            أرجع;
        }
        File_سطر* ثابت flp = nodep->fileline();
        // Relink child statements after the event control
        إذا (nodep->stmtsp()) nodep->جوڑو_اگلا_ایتھے(nodep->stmtsp()->unlink_Fr_پٹھ_With_اگلا());
        إذا (needDynamicTrigger(nodep->sentreep())) {
            // Create the trigger variable and init it with 0
            AstVarScope* ثابت trigvscp
                = create_عارضی(flp, m_dyn_Trig_ناں.get(nodep), nodep->لبھو_ثنائی_ہندسہ_DType(), nodep);
            خودکار* ثابت initp = جديد Ast_سونپو{flp, جديد AstVarRef{flp, trigvscp, VAccess::WRITE},
                                              جديد AstConst{flp, AstConst::ثنائی_ہندسہ_False{}}};
            // Await the eval step with the dynamic trigger scheduler. First, create the method
            // call
            خودکار* ثابت evalMethodp = جديد AstCMethodHard{
                flp, جديد AstVarRef{flp, getCreateDynamicTriggerScheduler(), VAccess::WRITE},
                VCMethod::SCHED_EVALUATION};
            evalMethodp->dtype_سیٹ_Void();
            جوڑو_Process_Info(evalMethodp);
            خودکار* ثابت sentreep = nodep->sentreep();
            جوڑو_Event_Debug_Info(evalMethodp, sentreep);
            // Create the co_await
            AstCAwait* ثابت awaitEvalp
                = جديد AstCAwait{flp, evalMethodp, get_Create_Dynamic_Trigger_Sen_درخت()};
            // Construct the sen expression for this sentree
            UASSERT_OBJ(m_sen_اظہاریہ_Builderp, nodep, "No SenExprBuilder for this scope");
            خودکار* ثابت assignp = جديد Ast_سونپو{flp, جديد AstVarRef{flp, trigvscp, VAccess::WRITE},
                                                m_sen_اظہاریہ_Builderp->build(sentreep).پہلا};
            // Get the SenExprBuilder results
            ثابت Sen_اظہاریہ_Builder::Results senResults = m_sen_اظہاریہ_Builderp->getAndClearResults();
            localizeVars(m_procp, senResults.m_vars);
            // If post updates are destructive (e.g. clearFired on events), perform a
            // conservative pre-clear once before entering the wait loop so stale state from a
            // previous wait does not cause an immediate false-positive trigger.
            ثابت منطقي hasDestructivePostUpdates = !senResults.m_destructivePostUpdates.خالی_تھاواں();
            // Create the trigger eval loop, which will await the evaluation step and check the
            // trigger
            Ast_گرہ_اظہاریہ* ثابت condp
                = جديد AstLogNot{flp, جديد AstVarRef{flp, trigvscp, VAccess::READ}};
            AstLoop* ثابت loopp = جديد AstLoop{flp};
            loopp->جوڑو_Stmtsp(جديد AstLoopTest{flp, loopp, condp});
            loopp->جوڑو_Stmtsp(awaitEvalp);
            // Put pre updates before the trigger check and assignment
            لكل (Ast_گرہ_Stmt* ثابت stmtp : senResults.m_preUpdates) loopp->جوڑو_Stmtsp(stmtp);
            // Then the trigger check and assignment
            loopp->جوڑو_Stmtsp(assignp);
            // Let the dynamic trigger scheduler know if this trigger was set
            // If it was, a call to the scheduler's evaluate() will return true
            AstCMethodHard* ثابت anyTriggeredMethodp = جديد AstCMethodHard{
                flp, جديد AstVarRef{flp, getCreateDynamicTriggerScheduler(), VAccess::WRITE},
                VCMethod::SCHED_ANY_TRIGGERED, جديد AstVarRef{flp, trigvscp, VAccess::READ}};
            anyTriggeredMethodp->dtype_سیٹ_Void();
            loopp->جوڑو_Stmtsp(anyTriggeredMethodp->makeStmt());
            // If the post update is destructive (e.g. event vars are cleared), create an await for
            // the post update step
            إذا (hasDestructivePostUpdates) {
                AstCAwait* ثابت awaitPostUpdatep = awaitEvalp->clone_درخت(خطأ);
                VN_AS(awaitPostUpdatep->exprp(), CMethodHard)->طریقہ(VCMethod::SCHED_POST_UPDATE);
                loopp->جوڑو_Stmtsp(awaitPostUpdatep);
            }
            // Put the post updates at the end of the loop
            لكل (Ast_گرہ_Stmt* ثابت stmtp : senResults.m_postUpdates) loopp->جوڑو_Stmtsp(stmtp);
            // Finally, await the resumption step in 'act'
            AstCAwait* ثابت awaitResumep = awaitEvalp->clone_درخت(خطأ);
            VN_AS(awaitResumep->exprp(), CMethodHard)->طریقہ(VCMethod::SCHED_RESUMPTION);
            Ast_گرہ::جوڑو_اگلا<Ast_گرہ_Stmt, Ast_گرہ_Stmt>(loopp, awaitResumep);
            // Replace the event control with one explicit stmt chain:
            //   init -> [inits] -> [optional destructive pre-clears] -> loop -> awaitResumption
            Ast_گرہ_Stmt* chainp = خالی_سوچک;
            chainp = Ast_گرہ::جوڑو_اگلا_Null(chainp, initp);
            لكل (Ast_گرہ_Stmt* ثابت stmtp : senResults.m_inits) {
                chainp = Ast_گرہ::جوڑو_اگلا_Null(chainp, stmtp);
            }
            إذا (hasDestructivePostUpdates) {
                لكل (Ast_گرہ_Stmt* ثابت stmtp : senResults.m_destructivePostUpdates) {
                    chainp = Ast_گرہ::جوڑو_اگلا_Null(chainp, stmtp->clone_درخت(خطأ));
                }
            }
            chainp = Ast_گرہ::جوڑو_اگلا_Null(chainp, loopp);
            nodep->replaceWith(chainp);
        } وإلا {
            خودکار* ثابت sentreep = m_finder.get_Sen_درخت(nodep->sentreep());
            nodep->sentreep()->unlink_Fr_پٹھ()->delete_درخت();
            // Get this sentree's trigger scheduler
            // Replace self with a 'co_await trigSched.trigger()'
            خودکار* ثابت triggerMethodp = جديد AstCMethodHard{
                flp, جديد AstVarRef{flp, getCreateTriggerSchedulerp(sentreep), VAccess::WRITE},
                VCMethod::SCHED_TRIGGER};
            triggerMethodp->dtype_سیٹ_Void();
            // If it should be committed immediately, pass true, otherwise false
            triggerMethodp->جوڑو_Pinsp(nodep->user2() ? جديد AstConst{flp, AstConst::ثنائی_ہندسہ_True{}}
                                                    : جديد AstConst{flp, AstConst::ثنائی_ہندسہ_False{}});
            جوڑو_Process_Info(triggerMethodp);
            جوڑو_Event_Debug_Info(triggerMethodp, sentreep);
            // Create the co_await
            AstCAwait* ثابت awaitp = جديد AstCAwait{flp, triggerMethodp, sentreep};
            nodep->replaceWith(awaitp);
        }
        VL_DO_DANGLING(nodep->delete_درخت(), nodep);
    }
    فراغ ویکھو(Ast_گرہ_سونپو* nodep) override {
        // Only process once to avoid infinite loops (due to the net delay)
        إذا (nodep->user1_سیٹ_Once()) أرجع;
        UINFO(9, "control-visit " << nodep);
        File_سطر* ثابت flp = nodep->fileline();
        Ast_گرہ* controlp = ضارب_باہرلا_Timing_Control(nodep);
        ثابت منطقي وچ_سونپو_Dly = VN_IS(nodep, سونپو_Dly);
        // Handle the intra assignment timing control
        // Transform if:
        // * there's a timing control in the assignment
        // * the assignment is an AssignDly and it's in a non-inlined function
        إذا (!controlp && (!وچ_سونپو_Dly || m_underProcedure)) {
            iterateChildren(nodep);
            أرجع;
        }
        // The timing control, if any, which is put in a fork for an NBA
        Ast_گرہ* ثابت timingp = controlp;
        // Insert new vars before the timing control if we're in a function; in a process we can't
        // do that, as after V3Begin only modules and functions declare variables. These
        // intra-assignment vars will later be passed to forked processes by value.
        Ast_گرہ* داخل_کرو_Beforep = m_underProcedure ? خالی_سوچک : controlp;
        // Special case for NBA
        إذا (وچ_سونپو_Dly) {
            // Put it in a fork so it doesn't block
            // Could already be the only thing directly under a fork, reuse that if possible
            AstFork* forkp = !nodep->nextp() ? VN_CAST(nodep->پہلا_Abovep(), Fork) : خالی_سوچک;
            إذا (!forkp) forkp = جديد AstFork{flp, VJoinType::JOIN_NONE};
            إذا (!m_underProcedure) {
                // If it's in a function, it won't be handled by V3Delayed
                // Put it behind an additional named event that gets triggered in the NBA region
                AstEventControl* ثابت nbaEventControlp = createNbaEventControl(flp);
                Ast_سونپو* ثابت trigAssignp = createNbaEventTriggerAssignment(flp);
                nodep->replaceWith(trigAssignp);
                trigAssignp->جوڑو_اگلا_ایتھے(nbaEventControlp);
                nbaEventControlp->جوڑو_Stmtsp(nodep);
                داخل_کرو_Beforep = forkp;
                إذا (!controlp) controlp = nbaEventControlp;
            }
            controlp->replaceWith(forkp);
            AstBegin* beginp = VN_CAST(controlp, Begin);
            إذا (!beginp) beginp = جديد AstBegin{nodep->fileline(), "", controlp, خطأ};
            forkp->جوڑو_Forksp(beginp);
            جوڑو_Flags(beginp, T_NBA_UPDATE);
            controlp = forkp;
        }
        UASSERT_OBJ(nodep, controlp, "Assignment should have timing control");
        // Move `insertBeforep` into `AstCLocalScope` if necessary to avoid jumping over
        // a variable initialization that would be inserted before `insertBeforep`. All
        // access to this variable should be contained within returned `AstCLocalScope`.
        إذا (m_under_Jump_بلاک) جوڑو_CLocal_Scope(flp, داخل_کرو_Beforep);
        // Function for replacing values with intermediate variables
        ثابت خودکار replaceWithIntermediate = [&](Ast_گرہ_اظہاریہ* ثابت valuep,
                                                 ثابت std::string& name) {
            AstVarScope* ثابت newvscp = create_عارضی(flp, name, valuep->dtypep(), داخل_کرو_Beforep);
            // A pending update keeps its own value, even if its process schedules more
            إذا (وچ_سونپو_Dly && !داخل_کرو_Beforep) newvscp->fork_By_قدر(سچ);
            valuep->replaceWith(جديد AstVarRef{flp, newvscp, VAccess::READ});
            controlp->جوڑو_ایتھے_This_As_اگلا(
                جديد Ast_سونپو{flp, جديد AstVarRef{flp, newvscp, VAccess::WRITE}, valuep});
        };
        // NBAs with delays evaluate the indices selecting the target now (IEEE 1800-2023 10.4.2),
        // in order, each as a whole, also the selects and function calls in it. So follow the
        // selects of the target up from the variable, not into their indices. As an example, this
        // transforms 'x[a[i]][f(j)] <= #1 y' into 't0 = a[i]; t1 = f(j); x[t0][t1] <= #1 y'.
        // V3Const has split NBAs to concatenations, also streamed, into NBAs to each part.
        إذا (وچ_سونپو_Dly) {
            لكل (Ast_گرہ* exprp = nodep->lhsp()->ادھار_Fromp(سچ); exprp != nodep;
                 exprp = exprp->پہلا_Abovep()) {
                إذا (AstSel* ثابت selp = VN_CAST(exprp, Sel)) {
                    // widthp should be const
                    إذا (!VN_IS(selp->lsbp(), Const)) {
                        replaceWithIntermediate(selp->lsbp(), m_intra_Lsb_ناں.get(nodep));
                    }
                } وإلا إذا (Ast_گرہ_Sel* ثابت selp = VN_CAST(exprp, گرہ_Sel)) {
                    إذا (!VN_IS(selp->bitp(), Const)) {
                        replaceWithIntermediate(selp->bitp(), m_intra_اشاریہ_ناں.get(nodep));
                    }
                }
            }
        }
        // Replace the RHS with an intermediate value var
        replaceWithIntermediate(nodep->rhsp(), m_intra_قدر_ناں.get(nodep));
        إذا (!وچ_سونپو_Dly) أرجع;
        // The process executing the NBA evaluates its delay (IEEE 1800-2023 4.9.4), not the forked
        // process, if it has side effects, e.g. calling a function that checks its process. A pure
        // one is evaluated as the forked process starts, at once.
        إذا (AstDelay* ثابت delayp = VN_CAST(timingp, Delay)) {
            إذا (!delayp->lhsp()->is_خالص()) {
                replaceWithIntermediate(delayp->lhsp(), m_intra_قدر_ناں.get(nodep));
            }
        }
    }
    فراغ ویکھو(AstFireEvent* nodep) override {
        // V3Delayed handles '->>' of a variable in a process. Like NBAs in non-inlined functions,
        // trigger other events from a fork awaiting the NBA region.
        إذا (!nodep->isDelayed() || (m_underProcedure && VN_IS(nodep->operandp(), VarRef))) {
            iterateChildren(nodep);
            أرجع;
        }
        File_سطر* ثابت flp = nodep->fileline();
        Ast_سونپو* ثابت trigAssignp = createNbaEventTriggerAssignment(flp);
        nodep->replaceWith(trigAssignp);
        AstFork* ثابت forkp = جديد AstFork{flp, VJoinType::JOIN_NONE};
        trigAssignp->جوڑو_اگلا_ایتھے(forkp);
        إذا (m_under_Jump_بلاک) جوڑو_CLocal_Scope(flp, forkp);
        // The triggered event is the one referenced now, so evaluate handles and indices now
        Ast_گرہ_اظہاریہ* ثابت eventp = nodep->operandp()->unlink_Fr_پٹھ();
        ثابت خودکار evalNow = [&](Ast_گرہ_اظہاریہ* ثابت valuep, V3_Unique_ناں& ناں) {
            // Like for an NBA, a process uses a module variable, passed to the fork by value
            AstVarScope* ثابت vscp = create_عارضی(flp, ناں.get(nodep), valuep->dtypep(),
                                                 m_underProcedure ? خالی_سوچک : forkp);
            إذا (m_underProcedure) vscp->fork_By_قدر(سچ);
            valuep->replaceWith(جديد AstVarRef{flp, vscp, VAccess::READ});
            // Unlike an event, a handle selecting one is only read
            valuep->foreach([](Ast_گرہ* ثابت np) {
                إذا (Ast_گرہ_Var_Ref* ثابت refp = VN_CAST(np, گرہ_Var_Ref)) {
                    refp->access(VAccess::READ);
                } وإلا إذا (AstMemberSel* ثابت selp = VN_CAST(np, MemberSel)) {
                    selp->access(VAccess::READ);
                }
            });
            forkp->جوڑو_ایتھے_This_As_اگلا(
                جديد Ast_سونپو{flp, جديد AstVarRef{flp, vscp, VAccess::WRITE}, valuep});
        };
        Ast_گرہ_اظہاریہ* refp = eventp;
        طالما (Ast_گرہ_Sel* ثابت selp = VN_CAST(refp, گرہ_Sel)) {
            إذا (!VN_IS(selp->bitp(), Const)) evalNow(selp->bitp(), m_intra_اشاریہ_ناں);
            refp = selp->fromp();
        }
        // Handle of a class (or virtual interface)
        إذا (AstMemberSel* ثابت selp = VN_CAST(refp, MemberSel)) {
            evalNow(selp->fromp(), m_intra_قدر_ناں);
        }
        AstEventControl* ثابت controlp = createNbaEventControl(flp);
        controlp->جوڑو_Stmtsp(جديد AstFireEvent{flp, eventp, خطأ});
        AstBegin* ثابت beginp = جديد AstBegin{flp, "", controlp, خطأ};
        جوڑو_Flags(beginp, T_NBA_UPDATE);
        forkp->جوڑو_Forksp(beginp);
        VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
    }
    فراغ ویکھو(Ast_سونپو_W* nodep) override {
        File_سطر* ثابت flp = nodep->fileline();
        // Get the net delay unless this assignment was created for handling the net delay (user1)
        AstDelay* ثابت netDelayp = nodep->user1() ? خالی_سوچک : nodep->getLhsNetDelay();
        إذا (netDelayp) {
            إذا (nodep->timingControlp()) {
                // If this assignment has a delay, create another one to handle the net delay
                AstVarScope* ثابت newvscp = m_سونپو_WTmps.make(flp, m_scopep, nodep->dtypep());
                Ast_سونپو_W* assignp = جديد Ast_سونپو_W{
                    nodep->fileline(), nodep->lhsp()->unlink_Fr_پٹھ(),
                    جديد AstVarRef{flp, newvscp, VAccess::READ}, netDelayp->clone_درخت(خطأ)};
                assignp->user1(سچ);
                nodep->جوڑو_اگلا_ایتھے(assignp);
                nodep->lhsp(جديد AstVarRef{flp, newvscp, VAccess::WRITE});
            } وإلا {
                // Else just use this one with the net delay
                nodep->timingControlp(netDelayp->clone_درخت(خطأ));
            }
        }
        إذا (!nodep->timingControlp()) أرجع;
        // There will be some circular logic here, suppress the warning for newly created vars
        flp->modifyWarnOff(V3_Error_کوڈ::UNOPTFLAT, سچ);
        // Also suppress the warning for the LHS var, for cases like `assign #1 clk = ~clk;`
        // TODO: Restore the warning for other, non-delayed drivers
        nodep->lhsp()->foreach([](AstVarRef* refp) {
            إذا (refp->access().isWriteOrRW()) {
                refp->varp()->fileline()->modifyWarnOff(V3_Error_کوڈ::UNOPTFLAT, سچ);
            }
        });
        // Should be under an always
        AstAlways* ثابت alwaysp = VN_AS(m_procp, Always);
        // Convert it to an Assign; the new assign with intra delay will be handled by
        // visit(AstNodeAssign*)
        Ast_گرہ_اظہاریہ* ثابت lhs1p = nodep->lhsp()->unlink_Fr_پٹھ();
        Ast_گرہ_اظہاریہ* ثابت rhs1p = nodep->rhsp()->unlink_Fr_پٹھ();
        Ast_گرہ* ثابت controlp = nodep->timingControlp()->unlink_Fr_پٹھ();
        إذا (AstDelay* ثابت delayp = VN_CAST(controlp, Delay)) {
            إذا (Ast_گرہ_اظہاریہ* fallDelayp = delayp->fallDelay()) {
                fallDelayp = fallDelayp->unlink_Fr_پٹھ();
                Ast_گرہ_اظہاریہ* lhsp = delayp->lhsp()->unlink_Fr_پٹھ();
                // Use fall only for an all-zero value, rise otherwise.
                lhsp = delay_منزل_Integral(delayp, lhsp);
                fallDelayp = delay_منزل_Integral(delayp, fallDelayp);
                delayp->lhsp(
                    جديد AstCond{flp, جديد AstEq{flp, rhs1p->clone_درخت(خطأ), جديد AstConst{flp, 0}},
                                fallDelayp, lhsp});
            }
        }
        Ast_سونپو* ثابت assignp = جديد Ast_سونپو{nodep->fileline(), lhs1p, rhs1p, controlp};
        // Put the assignment in a fork..join_none.
        AstFork* ثابت forkp = جديد AstFork{flp, VJoinType::JOIN_NONE};
        nodep->replaceWith(forkp);
        VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
        AstBegin* ثابت beginp = جديد AstBegin{flp, "", assignp, خطأ};
        forkp->جوڑو_Forksp(beginp);
        ویکھو(forkp);  // Visit now as we need to do some post-processing
        // IEEE 1800-2023 10.3.3 - if the RHS value differs from the currently scheduled value to
        // be assigned, the currently scheduled assignment is descheduled. To keep track if an
        // assignment should be descheduled, each scheduled assignment event has a 'generation',
        // and if at assignment time its generation differs from the current generation, it won't
        // be performed
        Ast_سونپو* ثابت preAssignp = VN_AS(beginp->stmtsp(), سونپو);
        UASSERT_OBJ(preAssignp, alwaysp, "Pre-assign should be there from visit(AstFork)");
        Ast_سونپو* ثابت postAssignp = VN_AS(preAssignp->nextp()->nextp(), سونپو);
        UASSERT_OBJ(postAssignp, alwaysp, "Post-assign should be there from visit(AstFork)");
        // Increment generation and copy it to a local
        AstVarScope* ثابت generationVarp
            = m_سونپو_WGens.make(flp, m_scopep, alwaysp->لبھو_UInt64_DType());
        AstVarScope* ثابت genLocalVarp
            = create_عارضی(flp, generationVarp->varp()->name() + "__local",
                         alwaysp->لبھو_UInt64_DType(), preAssignp);
        preAssignp->جوڑو_ایتھے_This_As_اگلا(
            جديد Ast_سونپو{flp, جديد AstVarRef{flp, generationVarp, VAccess::WRITE},
                          جديد Ast_جوڑو{flp, جديد AstVarRef{flp, generationVarp, VAccess::READ},
                                     جديد AstConst{flp, 1}}});
        preAssignp->جوڑو_ایتھے_This_As_اگلا(
            جديد Ast_سونپو{flp, جديد AstVarRef{flp, genLocalVarp, VAccess::WRITE},
                          جديد AstVarRef{flp, generationVarp, VAccess::READ}});
        // If the current generation is same as the one saved in the local var, assign
        beginp->جوڑو_Stmtsp(
            جديد AstIf{flp,
                      جديد AstEq{flp, جديد AstVarRef{flp, generationVarp, VAccess::READ},
                                جديد AstVarRef{flp, genLocalVarp, VAccess::READ}},
                      postAssignp->unlink_Fr_پٹھ()});
        // Save scheduled RHS value before delay
        AstVarScope* ثابت tmpVarp
            = m_سونپو_WTmps.make(flp, m_scopep, preAssignp->rhsp()->dtypep());
        AstVarRef* ثابت tmp_سونپو_Rhsp = VN_AS(preAssignp->lhsp(), VarRef)->clone_درخت(خطأ);
        tmp_سونپو_Rhsp->access(VAccess::WRITE);
        preAssignp->جوڑو_اگلا_ایتھے(
            جديد Ast_سونپو{flp, جديد AstVarRef{flp, tmpVarp, VAccess::WRITE}, tmp_سونپو_Rhsp});
        // If the RHS is different from the currently scheduled value, schedule the new assignment
        // The generation will increase, effectively 'descheduling' the previous assignment.
        Ast_گرہ_اظہاریہ* ثابت didNotInitp
            = جديد AstLogNot{flp, جديد AstCExpr{flp, "vlSymsp->__Vm_didInit", 1}};
        AstVarRef* ثابت پہلا_Iterp
            = جديد AstVarRef{flp, m_netlistp->stl_پہلا_Iterationp(), VAccess::READ};
        Ast_گرہ_اظہاریہ* ثابت schedCondp
            = جديد AstLogOr{flp,
                           جديد AstNeq{flp, preAssignp->rhsp()->clone_درخت(خطأ),
                                      جديد AstVarRef{flp, tmpVarp, VAccess::READ}},
                           جديد AstLogAnd{flp, didNotInitp, پہلا_Iterp}};
        alwaysp->جوڑو_Stmtsp(جديد AstIf{flp, schedCondp, forkp->unlink_Fr_پٹھ()});
    }
    فراغ ویکھو(AstDisableFork* nodep) override {
        إذا (m_hasProcess) أرجع;
        // never reached by any process; remove to avoid compilation error
        VL_DO_DANGLING(nodep->unlink_Fr_پٹھ()->delete_درخت(), nodep);
    }
    فراغ ویکھو(AstWaitFork* nodep) override {
        إذا (m_hasProcess) {
            File_سطر* ثابت flp = nodep->fileline();
            AstCExpr* ثابت exprp = جديد AstCExpr{flp, "vlProcess->completedFork()", 1};
            AstWait* ثابت waitp = جديد AstWait{flp, exprp, خالی_سوچک};
            nodep->replaceWith(waitp);
        } وإلا {
            // never reached by any process; remove to avoid compilation error
            nodep->unlink_Fr_پٹھ();
        }
        VL_DO_DANGLING(nodep->delete_درخت(), nodep);
    }
    فراغ ویکھو(AstWait* nodep) override {
        // Wait on changed events related to the vars in the wait statement
        UINFO(9, "control-visit " << nodep);
        File_سطر* ثابت flp = nodep->fileline();
        Ast_گرہ* ثابت stmtsp = nodep->stmtsp();
        إذا (stmtsp) stmtsp->unlink_Fr_پٹھ_With_اگلا();
        Ast_گرہ_اظہاریہ* ثابت condp = V3Const::constifyEdit(nodep->condp()->unlink_Fr_پٹھ());
        خودکار* ثابت constp = VN_CAST(condp, Const);
        إذا (constp) {
            إذا (constp->isZero()) {
                // We have to await forever instead of simply returning in case we're deep in a
                // callstack
                AstCMethodHard* ثابت foreverMethodp = جديد AstCMethodHard{
                    flp, جديد AstVarRef{flp, getCreateDelayScheduler(), VAccess::WRITE},
                    VCMethod::SCHED_WAIT_FOREVER};
                foreverMethodp->dtype_سیٹ_Void();
                جوڑو_Process_Info(foreverMethodp);
                جوڑو_Debug_Info(foreverMethodp);
                AstCAwait* ثابت awaitp = جديد AstCAwait{flp, foreverMethodp};
                nodep->replaceWith(awaitp);
                إذا (stmtsp) VL_DO_DANGLING(stmtsp->delete_درخت(), stmtsp);
                VL_DO_DANGLING(condp->delete_درخت(), condp);
            } وإلا {
                nodep->v3fatalSrc("constant wait should have been removed in "
                                  "TimingSuspendableVisitor::visit(AstWait)");
            }
        } وإلا إذا (needDynamicTrigger(condp)) {
            // No point in making a sentree, just use the expression as sensitivity
            // Put the event control in an if so we only wait if the condition isn't met already
            خودکار* ثابت ifp = جديد AstIf{
                flp, جديد AstLogNot{flp, condp},
                جديد AstEventControl{flp,
                                    جديد Ast_Sen_درخت{flp, جديد Ast_Sen_شے{flp, VEdgeType::ET_TRUE,
                                                                       condp->clone_درخت(خطأ)}},
                                    خالی_سوچک}};
            إذا (stmtsp) Ast_گرہ::جوڑو_اگلا<Ast_گرہ, Ast_گرہ>(ifp, stmtsp);
            nodep->replaceWith(ifp);
        } وإلا {
            // We are using a global sentree, so we cannot use ET_TRUE, as that could lead to the
            // active region never converging. Because of this, we need to use ET_CHANGED in a
            // loop.
            AstEventControl* ثابت controlp = جديد AstEventControl{
                flp,
                جديد Ast_Sen_درخت{
                    flp, جديد Ast_Sen_شے{flp, VEdgeType::ET_بدلیا, condp->clone_درخت(خطأ)}},
                خالی_سوچک};
            controlp->user2(سچ);  // Commit immediately
            AstLoop* ثابت loopp = جديد AstLoop{flp};
            loopp->جوڑو_Stmtsp(جديد AstLoopTest{flp, loopp, جديد AstLogNot{flp, condp}});
            loopp->جوڑو_Stmtsp(controlp);
            إذا (stmtsp) Ast_گرہ::جوڑو_اگلا<Ast_گرہ, Ast_گرہ>(loopp, stmtsp);
            nodep->replaceWith(loopp);
        }
        VL_DO_DANGLING(nodep->delete_درخت(), nodep);
    }
    فراغ ویکھو(AstBegin* nodep) override {
        VL_RESTORER(m_procp);
        VL_RESTORER(m_hasProcess);
        m_hasProcess |= hasFlags(nodep, T_HAS_PROC);
        // A pending NBA update is not a subprocess of the process that scheduled it, so it is
        // unaffected by 'disable fork' and 'wait fork' (IEEE 1800-2023 9.6.1, 9.6.3)
        إذا (hasFlags(nodep, T_NBA_UPDATE)) m_hasProcess = خطأ;
        m_procp = nodep;
        إذا (m_hasProcess) nodep->سیٹ_Need_Process();
        iterateChildren(nodep);
    }
    فراغ ویکھو(AstFork* nodep) override {
        إذا (nodep->user1_سیٹ_Once()) أرجع;
        UINFO(9, "control-visit " << nodep);
        v3Global.سیٹ_ورتدا_Timing();

        // Create a unique name for this fork
        nodep->name("__Vfork_" + std::منزل_string(++m_forkCnt));

        // TODO: Should process nodep->stmtsp() in case an earlier pass
        //       inserted something there, but as of today we don't need to.

        // Process and name each fork
        ناپ_t idx = 0;  // Index for naming begins
        لكل (AstBegin *itemp = nodep->forksp(), *nextp; itemp; itemp = nextp) {
            nextp = VN_AS(itemp->nextp(), Begin);
            iterate(itemp);
            // Note: Even if we do not find any awaits, we cannot simply inline
            // the process here, as new awaits could be added later.

            // Name the begin (later the name will be used for a new function)
            itemp->name(nodep->name() + "__" + std::منزل_string(idx++));
            إذا (itemp->needProcess()) {
                itemp->جوڑو_Stmtsp(
                    جديد AstCStmt{itemp->fileline(), "vlProcess->state(VlProcess::FINISHED);"});
            }
        }
        إذا (!nodep->joinType().joinNone()) makeForkJoin(nodep);
    }

    //--------------------
    فراغ ویکھو(AstVar*) override {}  // Accelerate
    فراغ ویکھو(Ast_گرہ* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    واضح TimingControlVisitor(AstNetlist* nodep)
        : m_netlistp{nodep} {
        iterate(nodep);

        // If there is no static #0 in the design, but an unknown delay was found,
        // and the user did not specify preference, then warn on all unknown delays
        // as we will be assuming they can be #0, which can cause performance degradation.
        لكل (File_سطر* ثابت flp : m_unknownDelayFlps) {
            flp->v3warn(ZERODLY,
                        "Value of # delay control statically unknown. Assuming it can be #0.\n"
                            << flp->warn_ہور()  //
                            << "... If all # delays are non-zero at runtime,\n"
                            << flp->warn_ہور()  //
                            << "... use '--no-sched-zero-delay' for improved performance.\n"
                            << flp->warn_ہور()  //
                            << "... If a real #0 is expected at runtime,\n"
                            << flp->warn_ہور()  //
                            << "... use '--sched-zero-delay' to suppress this warning.");
        }
    }
    ~TimingControlVisitor() override {
        V3Stats::جوڑو_Stat("Timing, known #0 delays", m_statZeroDelays);
        V3Stats::جوڑو_Stat("Timing, known #const delays", m_statConstDelays);
        V3Stats::جوڑو_Stat("Timing, unknown #variable delays", m_statVariableDelays);
    }
};

//######################################################################
// Timing class functions

Ast_گرہ_اظہاریہ* V3Timing::delay_منزل_Ticks(Ast_گرہ_اظہاریہ* valuep, VTimescale timeunit) {
    File_سطر* ثابت flp = valuep->fileline();
    إذا (ثابت AstConst* ثابت constp = VN_CAST(valuep, Const)) {
        إذا (constp->عدد().is1_قدم()) {
            // A step is one tick of the global time precision (IEEE 1800-2023 3.14.3)
            VL_DO_DANGLING(valuep->delete_درخت(), valuep);
            AstConst* ثابت tickp = جديد AstConst{flp, AstConst::Unsized64{}, 1};
            tickp->dtype_سیٹ_ثنائی_ہندسہ_Sized(64, VSigning::UNSIGNED);
            أرجع tickp;
        }
        إذا (constp->isZero()) أرجع valuep;
    }
    UASSERT_OBJ(!timeunit.isNone(), valuep, "timeunit must be set");
    ثابت مزدوج timescale_ضارب_D
        = std::قوت(10.0, timeunit.power_دا_Ten() - v3Global.rootp()->timeprecision().power_دا_Ten());
    إذا (valuep->dtypep()->skipRefp()->isDouble()) {
        AstConst* ثابت tsfp = جديد AstConst{flp, AstConst::RealDouble{}, timescale_ضارب_D};
        valuep = جديد AstMulD{flp, valuep, tsfp};
        valuep = جديد AstRToIRoundS{flp, valuep};
        valuep->dtype_سیٹ_ثنائی_ہندسہ_Sized(64, VSigning::UNSIGNED);
    } وإلا {
        valuep->dtype_سیٹ_ثنائی_ہندسہ_Sized(64, VSigning::UNSIGNED);
        ثابت uint64_t timescale_ضارب_U = static_cast<uint64_t>(timescale_ضارب_D);
        AstConst* ثابت tsfp = جديد AstConst{flp, AstConst::Unsized64{}, timescale_ضارب_U};
        valuep = جديد AstMul{flp, valuep, tsfp};
    }
    أرجع V3Const::constifyEdit(valuep);  // Simplify
}

فراغ V3Timing::timingAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        ثابت VNUser1_وچ_Use m_user1_وچ_Use;
        ثابت VNUser2_وچ_Use m_user2_وچ_Use;
        { TimingSuspendableVisitor{nodep}; }
        إذا (v3Global.ورتدا_Timing()) TimingControlVisitor{nodep};
    }
    V3Global::dump_Check_Global_درخت("timing", 0, dump_درخت_Either_Level() >= 3);
}
