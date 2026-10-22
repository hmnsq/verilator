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

تعداد ڳنڍ_Flag : uint8_t {
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

تعداد ڦهلاء_Type : uint8_t {
    P_CALL = 1,  // Propagation through call to a function/task/method
    P_FORK = 2,  // Propagation due to fork's behavior
    P_SIGNATURE = 3,  // Propagation required to maintain C++ function's signature requirements
};

// Add timing flag to a node
ساكن فراغ جوڙيو_Flags(Ast_ڳنڍ* ثابت nodep, uint8_t flags) { nodep->user2Or(flags); }
// Check if a node has ALL of the expected flags set
ساكن منطقي hasFlags(Ast_ڳنڍ* ثابت nodep, uint8_t flags) { أرجع !(~nodep->user2() & flags); }

// ######################################################################
//  Detect nodes affected by timing and/or requiring a process

صنف TimingSuspendableVisitor final : عام VNVisitor {
    // TYPES
    // Vertex of a dependency graph of suspendable nodes, e.g. if a node (process or task) is
    // suspendable, all its dependents should also be suspendable
    صنف DepVtx VL_NOT_FINAL : عام V3_گراف_راس {
        VL_RTTI_IMPL(DepVtx, V3_گراف_راس)
        AstClass* ثابت m_classp;  // Class associated with a method
        Ast_ڳنڍ* ثابت m_nodep;  // AST node represented by this graph vertex

        // ACCESSORS
        string name() ثابت override {
            إذا (m_classp) {
                إذا (VN_IS(nodep(), CFunc)) {
                    أرجع cvt_منزل_Hex(nodep()) + ' ' + classp()->name() + "::" + nodep()->name();
                }
            }
            أرجع cvt_منزل_Hex(nodep()) + ' ' + nodep()->prettyTypeName();
        }
        File_سٽ* fileline() ثابت override { أرجع nodep()->fileline(); }

    عام:
        // CONSTRUCTORS
        DepVtx(V3_گراف* graphp, Ast_ڳنڍ* nodep, AstClass* classp)
            : V3_گراف_راس{graphp}
            , m_classp{classp}
            , m_nodep{nodep} {}
        ~DepVtx() override = افتراضي;

        // ACCESSORS
        مجازي Ast_ڳنڍ* nodep() ثابت VL_MT_STABLE { أرجع m_nodep; }
        مجازي Ast_ڳنڍ* classp() ثابت VL_MT_STABLE { أرجع m_classp; }
    };

    صنف SuspendDepVtx final : عام DepVtx {
        VL_RTTI_IMPL(SuspendDepVtx, DepVtx)
        string dotColor() ثابت override {
            إذا (hasFlags(nodep(), T_SUSPENDER)) أرجع "red";
            إذا (hasFlags(nodep(), T_SUSPENDEE)) أرجع "blue";
            أرجع "black";
        }

    عام:
        SuspendDepVtx(V3_گراف* graphp, Ast_ڳنڍ* nodep, AstClass* classp)
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
        NeedsProcDepVtx(V3_گراف* graphp, Ast_ڳنڍ* nodep, AstClass* classp)
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
    ثابت VNUser3_اندريون_Use m_user3_اندريون_Use;
    ثابت VNUser4_اندريون_Use m_user4_اندريون_Use;

    // STATE
    VMemberMap m_memberMap;  // Member names cached for fast lookup
    AstClass* m_classp = خالي_اشارو;  // Current class
    Ast_ڳنڍ* m_procp = خالي_اشارو;  // NodeProcedure/CFunc/Begin we're under
    uint8_t m_underFork = F_NONE;  // F_NONE or flags of a fork we are under
    V3_گراف m_susp_گراف;  // Dependency graph where a node is a dependency of another if it being
                          // suspendable makes the other node suspendable
    V3_گراف m_proc_گراف;  // Dependency graph where a node is a dependency of another if it being
                          // suspendable makes the other node suspendable

    // METHODS
    // Get or create the dependency vertex for the given node
    DepVtx* getSuspendDepVtx(Ast_ڳنڍ* ثابت nodep) {
        AstClass* classp = خالي_اشارو;
        إذا (AstCFunc* funcp = VN_CAST(nodep, CFunc)) {
            إذا (funcp->scopep() && funcp->scopep()->modp()) {
                classp = VN_CAST(funcp->scopep()->modp(), Class);
            }
        }
        إذا (!nodep->user3p()) nodep->user3p(جديد SuspendDepVtx{&m_susp_گراف, nodep, classp});
        أرجع nodep->user3u().منزل<SuspendDepVtx*>();
    }
    DepVtx* getNeedsProcDepVtx(Ast_ڳنڍ* ثابت nodep) {
        AstClass* classp = خالي_اشارو;
        إذا (AstCFunc* funcp = VN_CAST(nodep, CFunc)) {
            إذا (funcp->scopep() && funcp->scopep()->modp()) {
                classp = VN_CAST(funcp->scopep()->modp(), Class);
            }
        }
        إذا (!nodep->user4p()) nodep->user4p(جديد NeedsProcDepVtx{&m_proc_گراف, nodep, classp});
        أرجع nodep->user4u().منزل<NeedsProcDepVtx*>();
    }
    // Pass timing flag between nodes
    منطقي passFlag(ثابت Ast_ڳنڍ* from, Ast_ڳنڍ* منزل, ڳنڍ_Flag flag) {
        إذا ((from->user2() & flag) && !(منزل->user2() & flag)) {
            جوڙيو_Flags(منزل, flag);
            أرجع سچ;
        }
        أرجع خطأ;
    }
    // Propagate flag to all nodes that depend on the given one
    فراغ propagateFlags(DepVtx* ثابت vxp, ڳنڍ_Flag flag) {
        خودڪار* ثابت parentp = vxp->nodep();
        لكل (V3_گراف_ڪنارو& ڪنارو : vxp->ٻاهريون_ڪنارا()) {
            خودڪار* ثابت depVxp = static_cast<DepVtx*>(ڪنارو.چوٽي());
            Ast_ڳنڍ* ثابت depp = depVxp->nodep();
            إذا (passFlag(parentp, depp, flag)) propagateFlags(depVxp, flag);
        }
    }
    سانچو <قسم_نالو T_Predicate>
    فراغ propagateFlagsIf(DepVtx* ثابت vxp, ڳنڍ_Flag flag, T_Predicate p) {
        خودڪار* ثابت parentp = vxp->nodep();
        لكل (V3_گراف_ڪنارو& ڪنارو : vxp->ٻاهريون_ڪنارا()) {
            خودڪار* ثابت depVxp = static_cast<DepVtx*>(ڪنارو.چوٽي());
            Ast_ڳنڍ* ثابت depp = depVxp->nodep();
            إذا (p(&ڪنارو) && passFlag(parentp, depp, flag)) propagateFlagsIf(depVxp, flag, p);
        }
    }
    سانچو <قسم_نالو T_Predicate>
    فراغ propagateFlagsReversedIf(DepVtx* ثابت vxp, ڳنڍ_Flag flag, T_Predicate p) {
        خودڪار* ثابت parentp = vxp->nodep();
        لكل (V3_گراف_ڪنارو& ڪنارو : vxp->اندريون_ڪنارا()) {
            خودڪار* ثابت depVxp = static_cast<DepVtx*>(ڪنارو.fromp());
            Ast_ڳنڍ* ثابت depp = depVxp->nodep();
            إذا (p(&ڪنارو) && passFlag(parentp, depp, flag))
                propagateFlagsReversedIf(depVxp, flag, p);
        }
    }

    // VISITORS
    فراغ گهمو(AstClass* nodep) override {
        UASSERT_OBJ(!m_classp, nodep, "Class under class");
        VL_RESTORER(m_classp);
        m_classp = nodep;
        iterateChildren(nodep);
    }
    فراغ گهمو(Ast_ڳنڍ_Procedure* nodep) override {
        VL_RESTORER(m_procp);
        m_procp = nodep;
        getNeedsProcDepVtx(nodep);
        جوڙيو_Flags(nodep, T_ALLOCS_PROC);
        إذا (VN_IS(nodep, Always)) {
            UINFO(9, "Always does " << (nodep->needProcess() ? "" : "NOT ") << "need process");
        }
        iterateChildren(nodep);
    }
    فراغ گهمو(AstDisableFork* nodep) override {
        گهمو(static_cast<Ast_ڳنڍ*>(nodep));
        جوڙيو_Flags(m_procp, T_FORCES_PROC | T_NEEDS_PROC);
    }
    فراغ گهمو(AstWaitFork* nodep) override {
        گهمو(static_cast<Ast_ڳنڍ*>(nodep));
        جوڙيو_Flags(m_procp, T_FORCES_PROC | T_NEEDS_PROC);
    }
    فراغ گهمو(AstWait* nodep) override {
        UINFO(9, "suspendable-visit " << nodep);
        Ast_ڳنڍ_اظهار* ثابت condp = V3Const::constifyEdit(nodep->condp());
        إذا (AstConst* ثابت constp = VN_CAST(condp, Const)) {
            إذا (!nodep->fileline()->warnIsOff(V3_Error_ڪوڊ::WAITCONST)) {
                condp->v3warn(WAITCONST, "Wait statement condition is constant");
            }
            إذا (!constp->isZero()) {
                // Remove AstWait before we track process as T_SUSPENDER
                إذا (Ast_ڳنڍ* ثابت stmtsp = nodep->stmtsp()) {
                    stmtsp->unlink_Fr_پٺ_With_ايندڙ();
                    nodep->replaceWith(stmtsp);
                } وإلا {
                    nodep->unlink_Fr_پٺ();
                }
                VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
                أرجع;
            }
        }
        v3Global.سيٽ_استعمال_ڪري_Timing();
        إذا (m_procp) جوڙيو_Flags(m_procp, T_SUSPENDEE | T_SUSPENDER | T_NEEDS_PROC);
        iterateChildren(nodep);
    }
    فراغ گهمو(AstCFunc* nodep) override {
        VL_RESTORER(m_procp);
        m_procp = nodep;
        iterateChildren(nodep);
        إذا (nodep->needProcess()) جوڙيو_Flags(nodep, T_FORCES_PROC | T_NEEDS_PROC);
        DepVtx* ثابت sVxp = getSuspendDepVtx(nodep);
        DepVtx* ثابت pVxp = getNeedsProcDepVtx(nodep);
        إذا (!m_classp) أرجع;

        // Go over overridden functions

        std::قطار<AstClassExtends*> extends;
        إذا (m_classp->extendsp()) extends.ڌڪو(m_classp->extendsp());

        طالما (!extends.خالي_جايون()) {
            AstClassExtends* ext_list = extends.front();
            extends.ڪڍو();

            لكل (AstClassExtends* cextp = ext_list; cextp;
                 cextp = VN_AS(cextp->nextp(), ClassExtends)) {
                // TODO: It is possible that a methods the same name in the base class is not
                // actually overridden by our method. If this causes a problem, traverse to
                // the root of the inheritance hierarchy and check if the original method is
                // virtual or not.
                إذا (خودڪار* ثابت overriddenp
                    = VN_CAST(m_memberMap.ڳوليو_Member(cextp->classp(), nodep->name()), CFunc)) {
                    // Suspendability and process affects typing, so they propagate both ways
                    DepVtx* ثابت overriddenSVxp = getSuspendDepVtx(overriddenp);
                    DepVtx* ثابت overriddenPVxp = getNeedsProcDepVtx(overriddenp);
                    جديد V3_گراف_ڪنارو{&m_susp_گراف, sVxp, overriddenSVxp, P_SIGNATURE};
                    جديد V3_گراف_ڪنارو{&m_susp_گراف, overriddenSVxp, sVxp, P_SIGNATURE};
                    جديد V3_گراف_ڪنارو{&m_proc_گراف, pVxp, overriddenPVxp, P_SIGNATURE};
                    جديد V3_گراف_ڪنارو{&m_proc_گراف, overriddenPVxp, pVxp, P_SIGNATURE};
                } وإلا {
                    AstClassExtends* وڌيڪ_extends = cextp->classp()->extendsp();
                    إذا (وڌيڪ_extends) extends.ڌڪو(وڌيڪ_extends);
                }
            }
        }
    }
    فراغ گهمو(Ast_ڳنڍ_CCall* nodep) override {
        AstCFunc* ثابت funcp = nodep->funcp();
        UASSERT_OBJ(funcp, nodep, "AstNodeCCall must have non-null funcp post-link");
        UASSERT_OBJ(m_procp, nodep, "AstNodeCCall must be inside a procedure/CFunc/Begin");

        UINFO(9, "V3Timing: Processing CCall to " << funcp->name() << " in dependency graph\n");
        جديد V3_گراف_ڪنارو{&m_susp_گراف, getSuspendDepVtx(funcp), getSuspendDepVtx(m_procp), P_CALL};

        جديد V3_گراف_ڪنارو{&m_proc_گراف, getNeedsProcDepVtx(funcp), getNeedsProcDepVtx(m_procp),
                        P_CALL};

        iterateChildren(nodep);
    }
    فراغ گهمو(AstBegin* nodep) override {
        VL_RESTORER(m_procp);
        VL_RESTORER(m_underFork);

        إذا (!m_underFork || (m_underFork & F_MIGHT_SUSPEND))
            جديد V3_گراف_ڪنارو{&m_susp_گراف, getSuspendDepVtx(nodep), getSuspendDepVtx(m_procp),
                            m_underFork ? P_FORK : P_CALL};

        جديد V3_گراف_ڪنارو{&m_proc_گراف, getNeedsProcDepVtx(nodep), getNeedsProcDepVtx(m_procp),
                        P_CALL};

        إذا (m_underFork) جوڙيو_Flags(nodep, T_NEEDS_PROC | T_ALLOCS_PROC);

        m_procp = nodep;
        m_underFork = 0;
        iterateChildren(nodep);
    }
    فراغ گهمو(AstFork* nodep) override {
        UINFO(9, "suspendable-visit " << nodep);
        VL_RESTORER(m_underFork);

        v3Global.سيٽ_استعمال_ڪري_Timing();  // Even if there are no event controls, we have to set this flag
                                   // so that transformForks() in V3SchedTiming gets called and
                                   // removes all forks and begins
        إذا (nodep->isTimingControl() && m_procp) {
            جوڙيو_Flags(m_procp, T_SUSPENDEE | T_SUSPENDER);
            m_underFork |= F_MIGHT_SUSPEND;
        }
        m_underFork |= F_MIGHT_NEED_PROC;
        iterateChildren(nodep);
    }
    فراغ گهمو(Ast_سونپيو_Dly* nodep) override {
        إذا (!VN_IS(m_procp, ڳنڍ_Procedure)) v3Global.سيٽ_استعمال_ڪري_Timing();
        گهمو(static_cast<Ast_ڳنڍ*>(nodep));
    }
    فراغ گهمو(AstFireEvent* nodep) override {
        إذا (nodep->isDelayed()
            && (!VN_IS(m_procp, ڳنڍ_Procedure) || !VN_IS(nodep->operandp(), VarRef))) {
            v3Global.سيٽ_استعمال_ڪري_Timing();
        }
        گهمو(static_cast<Ast_ڳنڍ*>(nodep));
    }
    فراغ گهمو(Ast_سونپيو_W* nodep) override {
        إذا (nodep->timingControlp()) v3Global.سيٽ_استعمال_ڪري_Timing();
        // Containing process will not suspend, don't mark it
    }
    فراغ گهمو(Ast_ڳنڍ* nodep) override {
        إذا (nodep->isTimingControl()) {
            v3Global.سيٽ_استعمال_ڪري_Timing();
            إذا (m_procp) جوڙيو_Flags(m_procp, T_SUSPENDEE | T_SUSPENDER | T_NEEDS_PROC);
        }
        iterateChildren(nodep);
    }

    //--------------------
    فراغ گهمو(AstVar*) override {}  // Accelerate

عام:
    // CONSTRUCTORS
    واضح TimingSuspendableVisitor(AstNetlist* nodep) {
        iterate(nodep);
        m_susp_گراف.remove_متعدي_ڪنارا();
        m_proc_گراف.remove_متعدي_ڪنارا();
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
                propagateFlagsIf(&depVtx, T_FORCES_PROC, [&](ثابت V3_گراف_ڪنارو* e) -> منطقي {
                    أرجع !hasFlags(static_cast<DepVtx*>(e->fromp())->nodep(), T_ALLOCS_PROC);
                });
            }
            // Mark nodes on paths between processes and statements that use VlProcess
            إذا (hasFlags(depVtx.nodep(), T_NEEDS_PROC)) {
                propagateFlagsIf(&depVtx, T_NEEDS_PROC, [&](ثابت V3_گراف_ڪنارو* e) -> منطقي {
                    أرجع !hasFlags(static_cast<DepVtx*>(e->چوٽي())->nodep(), T_ALLOCS_PROC);
                });
            }
        }
        لكل (V3_گراف_راس& vtx : m_proc_گراف.vertices()) {
            DepVtx& depVtx = static_cast<DepVtx&>(vtx);
            // Mark nodes that will be emitted with a VlProcess argument
            إذا (hasFlags(depVtx.nodep(), T_ALLOCS_PROC | T_FORCES_PROC)) {
                جوڙيو_Flags(depVtx.nodep(), T_HAS_PROC);
                propagateFlagsReversedIf(&depVtx, T_HAS_PROC, [&](ثابت V3_گراف_ڪنارو* e) -> منطقي {
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
    Ast_چوٽي_Scope* ثابت m_چوٽي_Scopep = m_netlistp->چوٽي_Scopep();  // The AstTopScope
    AstScope* ثابت m_scopeTopp = m_چوٽي_Scopep->scopep();  // Scope at the top
    AstClass* m_classp = خالي_اشارو;  // Current class
    AstScope* m_scopep = خالي_اشارو;  // Current scope
    Ast_سرگرم* m_activep = خالي_اشارو;  // Current active
    Ast_ڳنڍ* m_procp = خالي_اشارو;  // NodeProcedure/CFunc/Begin we're under
    منطقي m_hasProcess = خطأ;  // True if current scope has a VlProcess handle available
    صحيح m_forkCnt = 0;  // Number of forks inside a module
    منطقي m_under_Jump_بلاڪ = خطأ;  // True if we are inside of a jump-block
    منطقي m_underProcedure = خطأ;  // True if we are under an always or initial
    منطقي m_underIfaceCFunc = خطأ;  // True if we are under a CFunc owned by an interface scope
    منطقي m_hasStaticZeroDelay = خطأ;  // True if we have a static #0 delay
    std::ويڪٽر<File_سٽ*> m_unknownDelayFlps;  // Locations of AstDelay with non-constant value

    // Unique names
    V3_Unique_نالا m_dlyfork_نالا{"__Vdlyfork"};  // Names for temp AssignW vars
    V3_Unique_نالا m_intra_قدر_نالا{"__Vintraval"};  // Intra assign delay value var names
    V3_Unique_نالا m_intra_اشاريو_نالا{"__Vintraidx"};  // Intra assign delay index var names
    V3_Unique_نالا m_intra_Lsb_نالا{"__Vintralsb"};  // Intra assign delay LSB var names
    V3_Unique_نالا m_trig_Sched_نالا{"__VtrigSched"};  // Trigger scheduler name generator
    V3_Unique_نالا m_dyn_Trig_نالا{"__VdynTrigger"};  // Dynamic trigger name generator
    // Module level temporary variables, shared by instances
    V3SharedTmps m_سونپيو_WTmps{"__VassignWtmp", VVarType::MODULETEMP};  // Delayed AssignW values
    V3SharedTmps m_سونپيو_WGens{"__VassignWgen", VVarType::MODULETEMP};  // AssignW generations

    // DTypes
    AstBasicDType* m_forkDtp = خالي_اشارو;  // Fork variable type
    AstBasicDType* m_trigSchedDtp = خالي_اشارو;  // Trigger scheduler type

    // Timing-related globals
    AstVarScope* m_delaySchedp = خالي_اشارو;  // Global delay scheduler
    AstVarScope* m_dynamicSchedp = خالي_اشارو;  // Global dynamic trigger scheduler
    Ast_Sen_وڻ* m_delaySensesp = خالي_اشارو;  // Domain to trigger if a delayed coroutine is resumed
    Ast_Sen_وڻ* m_dynamicSensesp = خالي_اشارو;  // Domain to trigger if a dynamic trigger is set

    // Other
    Sen_وڻ_Finder m_finder{m_netlistp};  // Sentree finder and uniquifier
    Sen_اظهار_Builder* m_sen_اظهار_Builderp = خالي_اشارو;  // Sens expression builder for current m_scope

    // Stats
    سائيز_t m_statZeroDelays = 0;  // Number of statically known #0 delays
    سائيز_t m_statConstDelays = 0;  // Number of statically known #const (non-zero) delays
    سائيز_t m_statVariableDelays = 0;  // Number of delays with value unknown at compile time

    // METHODS
    // Transform an assignment with an intra timing control into a timing control with the
    // assignment under it
    Ast_ڳنڍ* ضرب_ڪندڙ_ٻاهريون_Timing_Control(Ast_ڳنڍ_سونپيو* nodep) ثابت {
        Ast_ڳنڍ* stmtp = nodep;
        Ast_ڳنڍ* ثابت controlp = nodep->timingControlp();
        إذا (AstDelay* ثابت delayp = VN_CAST(controlp, Delay)) {
            stmtp->replaceWith(delayp->unlink_Fr_پٺ());
            delayp->جوڙيو_Stmtsp(stmtp);
            stmtp = delayp;
        } وإلا إذا (Ast_Sen_وڻ* ثابت sentreep = VN_CAST(controlp, Sen_وڻ)) {
            AstEventControl* ثابت eventControlp
                = جديد AstEventControl{sentreep->fileline(), sentreep->unlink_Fr_پٺ(), خالي_اشارو};
            stmtp->replaceWith(eventControlp);
            eventControlp->جوڙيو_Stmtsp(stmtp);
            stmtp = eventControlp;
        } وإلا إذا (AstBegin* ثابت beginp = VN_CAST(controlp, Begin)) {
            // Begin from V3AssertPre
            stmtp->replaceWith(beginp->unlink_Fr_پٺ());
            beginp->جوڙيو_Stmtsp(stmtp);
            stmtp = beginp;
        }
        أرجع stmtp == nodep ? خالي_اشارو : stmtp;
    }
    ساكن منطقي staticallyNonZeroDelay(ثابت Ast_ڳنڍ_اظهار* valuep) {
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
        خودڪار* ثابت dlySchedDtp = جديد AstBasicDType{
            m_scopeTopp->fileline(), VBasicDTypeKwd::DELAY_SCHEDULER, VSigning::UNSIGNED};
        m_netlistp->typeTablep()->جوڙيو_Typesp(dlySchedDtp);
        m_delaySchedp = m_چوٽي_Scopep->create_عارضي("__VdlySched", dlySchedDtp);
        // Delay scheduler has to be accessible from top
        m_delaySchedp->varp()->sig_عوامي(سچ);
        m_netlistp->delaySchedulerp(m_delaySchedp->varp());
        أرجع m_delaySchedp;
    }
    // Creates the delay sentree
    Ast_Sen_وڻ* get_Create_Delay_Sen_وڻ() {
        إذا (m_delaySensesp) أرجع m_delaySensesp;
        File_سٽ* ثابت flp = m_scopeTopp->fileline();
        خودڪار* ثابت awaitingCurrentTimep
            = جديد AstCMethodHard{flp, جديد AstVarRef{flp, getCreateDelayScheduler(), VAccess::READ},
                                 VCMethod::SCHED_AWAITING_CURRENT_وقت};
        awaitingCurrentTimep->dtype_سيٽ_ٻٽو_انگ();
        m_delaySensesp
            = جديد Ast_Sen_وڻ{flp, جديد Ast_Sen_شي{flp, VEdgeType::ET_TRUE, awaitingCurrentTimep}};
        m_netlistp->چوٽي_Scopep()->جوڙيو_Sen_Treesp(m_delaySensesp);
        أرجع m_delaySensesp;
    }
    // Creates the global dynamic trigger scheduler variable
    AstVarScope* getCreateDynamicTriggerScheduler() {
        إذا (m_dynamicSchedp) أرجع m_dynamicSchedp;
        خودڪار* ثابت dynSchedDtp
            = جديد AstBasicDType{m_scopeTopp->fileline(), VBasicDTypeKwd::DYNAMIC_TRIGGER_SCHEDULER,
                                VSigning::UNSIGNED};
        m_netlistp->typeTablep()->جوڙيو_Typesp(dynSchedDtp);
        m_dynamicSchedp = m_چوٽي_Scopep->create_عارضي("__VdynSched", dynSchedDtp);
        أرجع m_dynamicSchedp;
    }
    // Creates the dynamic trigger sentree
    Ast_Sen_وڻ* get_Create_Dynamic_Trigger_Sen_وڻ() {
        إذا (m_dynamicSensesp) أرجع m_dynamicSensesp;
        File_سٽ* ثابت flp = m_scopeTopp->fileline();
        خودڪار* ثابت awaitingCurrentTimep = جديد AstCMethodHard{
            flp, جديد AstVarRef{flp, getCreateDynamicTriggerScheduler(), VAccess::READ},
            VCMethod::SCHED_EVALUATE};
        awaitingCurrentTimep->dtype_سيٽ_ٻٽو_انگ();
        m_dynamicSensesp
            = جديد Ast_Sen_وڻ{flp, جديد Ast_Sen_شي{flp, VEdgeType::ET_TRUE, awaitingCurrentTimep}};
        m_netlistp->چوٽي_Scopep()->جوڙيو_Sen_Treesp(m_dynamicSensesp);
        أرجع m_dynamicSensesp;
    }
    // Creates the event variable to trigger in NBA region
    AstEventControl* createNbaEventControl(File_سٽ* flp) {
        إذا (!m_netlistp->nbaEventp()) {
            خودڪار* ثابت nbaEventDtp = جديد AstBasicDType{m_scopeTopp->fileline(),
                                                        VBasicDTypeKwd::EVENT, VSigning::UNSIGNED};
            m_netlistp->typeTablep()->جوڙيو_Typesp(nbaEventDtp);
            m_netlistp->nbaEventp(m_چوٽي_Scopep->create_عارضي("__VnbaEvent", nbaEventDtp));
            v3Global.سيٽ_Has_Events();
        }
        أرجع جديد AstEventControl{
            flp,
            جديد Ast_Sen_وڻ{
                flp, جديد Ast_Sen_شي{flp, VEdgeType::ET_EVENT,
                                    جديد AstVarRef{flp, m_netlistp->nbaEventp(), VAccess::READ}}},
            خالي_اشارو};
    }
    // Creates the variable that, if set, causes the NBA event to be triggered
    Ast_سونپيو* createNbaEventTriggerAssignment(File_سٽ* flp) {
        إذا (!m_netlistp->nbaEventTriggerp()) {
            m_netlistp->nbaEventTriggerp(m_چوٽي_Scopep->create_عارضي("__VnbaEventTrigger", 1));
        }
        أرجع جديد Ast_سونپيو{flp,
                             جديد AstVarRef{flp, m_netlistp->nbaEventTriggerp(), VAccess::WRITE},
                             جديد AstConst{flp, AstConst::ٻٽو_انگ_True{}}};
    }
    // Returns true if we are under a class or interface function, or the tree references locals.
    // These are cases where static, globally-evaluated triggers are not suitable.
    منطقي needDynamicTrigger(Ast_ڳنڍ* ثابت nodep) ثابت {
        أرجع m_classp || m_underIfaceCFunc || nodep->exists([](Ast_ڳنڍ* ثابت nodep) {
            إذا (Ast_ڳنڍ_Var_Ref* varp = VN_CAST(nodep, ڳنڍ_Var_Ref)) {
                أرجع varp->varp()->isFuncLocal();
            }
            أرجع !nodep->is_خالص();
        });
    }
    // Creates a trigger scheduler variable
    AstVarScope* getCreateTriggerSchedulerp(Ast_Sen_وڻ* ثابت sentreep) {
        إذا (!sentreep->user1p()) {
            إذا (!m_trigSchedDtp) {
                m_trigSchedDtp
                    = جديد AstBasicDType{m_scopeTopp->fileline(), VBasicDTypeKwd::TRIGGER_SCHEDULER,
                                        VSigning::UNSIGNED};
                m_netlistp->typeTablep()->جوڙيو_Typesp(m_trigSchedDtp);
            }
            AstVarScope* ثابت trigSchedp
                = m_چوٽي_Scopep->create_عارضي(m_trig_Sched_نالا.get(sentreep), m_trigSchedDtp);
            sentreep->user1p(trigSchedp);
        }
        أرجع VN_AS(sentreep->user1p(), VarScope);
    }
    // Creates a string describing the sentree
    AstCExpr* createEventDescription(Ast_Sen_وڻ* ثابت sentreep) ثابت {
        إذا (!sentreep->user2p()) {
            std::stringstream ss;
            V3EmitV::verilog_For_وڻ(sentreep, ss);
            // possibly a multiline string
            // Names were hashed by V3Name, show the user the original text
            std::string comment = VName::dehash(ss.str());
            std::replace(comment.begin(), comment.پڇاڙي(), '\n', ' ');
            // Escape quotes, e.g. from string-indexed event array selects
            comment = '"' + V3_ٻاهريون_Formatter::quoteNameControls(comment) + '"';
            AstCExpr* ثابت commentp = جديد AstCExpr{sentreep->fileline(), comment};
            commentp->dtype_سيٽ_String();
            sentreep->user2p(commentp);
            أرجع commentp;
        }
        أرجع VN_AS(sentreep->user2p(), CExpr)->clone_وڻ(خطأ);
    }
    // Adds debug info to a hardcoded method call
    فراغ جوڙيو_Debug_Info(AstCMethodHard* ثابت methodp) ثابت {
        File_سٽ* ثابت flp = methodp->fileline();
        ثابت منطقي protectIds = v3Global.opt.protectIds();
        AstCExpr* ثابت ap
            = جديد AstCExpr{flp, protectIds ? "VL_UNKNOWN" : '"' + flp->filenameEsc() + '"'};
        ap->dtype_سيٽ_String();
        methodp->جوڙيو_Pinsp(ap);
        AstCExpr* ثابت bp = جديد AstCExpr{flp, protectIds ? "0" : cvt_منزل_Str(flp->lineno())};
        bp->dtype_سيٽ_String();
        methodp->جوڙيو_Pinsp(bp);
    }
    // Adds debug info to a trigSched.trigger() call
    فراغ جوڙيو_Event_Debug_Info(AstCMethodHard* ثابت methodp, Ast_Sen_وڻ* ثابت sentreep) ثابت {
        إذا (v3Global.opt.protectIds()) {
            File_سٽ* ثابت flp = sentreep->fileline();
            AstCExpr* ثابت descp = جديد AstCExpr{flp, "VL_UNKNOWN"};
            descp->dtype_سيٽ_String();
            methodp->جوڙيو_Pinsp(descp);
            جوڙيو_Debug_Info(methodp);
            أرجع;
        }
        methodp->جوڙيو_Pinsp(createEventDescription(sentreep));
        جوڙيو_Debug_Info(methodp);
    }
    // Adds process pointer to a hardcoded method call
    فراغ جوڙيو_Process_Info(AstCMethodHard* ثابت methodp) ثابت {
        File_سٽ* ثابت flp = methodp->fileline();
        AstCExpr* ثابت ap
            = جديد AstCExpr{flp, (m_procp && m_hasProcess) ? "vlProcess" : "nullptr"};
        methodp->جوڙيو_Pinsp(ap);
    }
    // Creates the fork handle type and returns it
    AstBasicDType* getCreateForkSyncDTypep() {
        إذا (m_forkDtp) أرجع m_forkDtp;
        m_forkDtp = جديد AstBasicDType{m_scopeTopp->fileline(), VBasicDTypeKwd::FORK_SYNC,
                                      VSigning::UNSIGNED};
        m_netlistp->typeTablep()->جوڙيو_Typesp(m_forkDtp);
        أرجع m_forkDtp;
    }
    // Move `nodep` into `AstCLocalScope`.
    AstCLocalScope* جوڙيو_CLocal_Scope(File_سٽ* ثابت flp, Ast_ڳنڍ* ثابت nodep) ثابت {
        إذا (!nodep) أرجع خالي_اشارو;
        VNRelinker handle;
        nodep->unlink_Fr_پٺ(&handle);
        AstCLocalScope* ثابت cscopep = جديد AstCLocalScope{flp, nodep};
        handle.relink(cscopep);
        أرجع cscopep;
    }
    // Create a temp variable and optionally put it before the specified node (mark local if so)
    AstVarScope* create_عارضي(File_سٽ* ثابت flp, ثابت std::string& name,
                            Ast_ڳنڍ_DType* ثابت dtypep, Ast_ڳنڍ* ثابت وجهو_Beforep = خالي_اشارو) {
        AstVar* varp;
        إذا (وجهو_Beforep) {
            varp = جديد AstVar{flp, VVarType::BLOCKTEMP, name, dtypep};
            varp->funcLocal(سچ);
            وجهو_Beforep->جوڙيو_هتي_This_As_ايندڙ(varp);
        } وإلا {
            varp = جديد AstVar{flp, VVarType::MODULETEMP, name, dtypep};
            m_scopep->modp()->جوڙيو_Stmtsp(varp);
        }
        AstVarScope* ثابت vscp = جديد AstVarScope{flp, m_scopep, varp};
        m_scopep->جوڙيو_Varsp(vscp);
        أرجع vscp;
    }
    // Add a done() call on the fork sync
    فراغ جوڙيو_Fork_پورو(AstBegin* ثابت beginp, AstVarScope* ثابت forkVscp) ثابت {
        File_سٽ* ثابت flp = beginp->fileline();
        خودڪار* ثابت donep = جديد AstCMethodHard{
            beginp->fileline(), جديد AstVarRef{flp, forkVscp, VAccess::WRITE}, VCMethod::FORK_پورو};
        donep->dtype_سيٽ_Void();
        جوڙيو_Debug_Info(donep);
        beginp->جوڙيو_Stmtsp(donep->makeStmt());
    }
    ساكن منطقي has_Disable_قطار_ڌڪو_Self_اڳياڙي(AstBegin* ثابت beginp) {
        // LinkJump prepends disable-by-name registration as:
        //   __VprocessQueue_*.push_back(std::process::self())
        // V3LiftExpr lifts std::process::self() into an assignment to a temporary, then V3Task
        // lowers that assignment's function call into a leading comment and AstStmtExpr. Thus the
        // push_back is no longer necessarily the first statement. Scan across this generated
        // prefix for it, stopping at the fork-start sentinel or the branch body. Registrations for
        // nested forks live in sub-blocks and so are not in this statement list.
        لكل (Ast_ڳنڍ* stmtp = beginp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            إذا (VN_IS(stmtp, Comment)) استمر;
            Ast_Stmt_اظهار* ثابت stmtExprp = VN_CAST(stmtp, Stmt_اظهار);
            إذا (!stmtExprp) اكسر;
            إذا (stmtExprp->is_Disable_قطار_ڌڪو_Self_Stmt()) أرجع سچ;
        }
        أرجع خطأ;
    }
    // Register a callback so killing a process-backed fork branch decrements the join counter
    فراغ جوڙيو_Fork_On_Kill(AstBegin* ثابت beginp, AstVarScope* ثابت forkVscp) ثابت {
        إذا (!beginp->needProcess() && !has_Disable_قطار_ڌڪو_Self_اڳياڙي(beginp)) أرجع;
        File_سٽ* ثابت flp = beginp->fileline();
        AstCMethodHard* ثابت onKillp = جديد AstCMethodHard{
            flp, جديد AstVarRef{flp, forkVscp, VAccess::WRITE}, VCMethod::FORK_ON_KILL};
        onKillp->dtype_سيٽ_Void();
        AstCExpr* ثابت processp = جديد AstCExpr{flp, "vlProcess"};
        processp
            ->dtype_سيٽ_Void();  // Opaque process reference; type is irrelevant for hardcoded emit
        onKillp->جوڙيو_Pinsp(processp);
        Ast_ڳنڍ_Stmt* ثابت stmtp = onKillp->makeStmt();
        beginp->prependStmtsp(stmtp);
    }
    // Handle the 'join' part of a fork..join
    فراغ makeForkJoin(AstFork* ثابت forkp) {
        // Create a fork sync var
        File_سٽ* ثابت flp = forkp->fileline();
        // Insert the sync var directly before the fork
        Ast_ڳنڍ* ثابت وجهو_Beforep = forkp;
        // Make sure all references to the fork var are contained within a single scope to prevent
        // moving statements accessing it in case of large functions splitting.
        جوڙيو_CLocal_Scope(flp, وجهو_Beforep);
        AstVarScope* forkVscp
            = create_عارضي(flp, forkp->name() + "__sync", getCreateForkSyncDTypep(), وجهو_Beforep);
        بنانشان join_ڳڻپ = 0;  // Needed for join counter
        // Add a <fork sync>.done() to each begin
        لكل (Ast_ڳنڍ* beginp = forkp->forksp(); beginp; beginp = beginp->nextp()) {
            جوڙيو_Fork_On_Kill(VN_AS(beginp, Begin), forkVscp);
            جوڙيو_Fork_پورو(VN_AS(beginp, Begin), forkVscp);
            join_ڳڻپ++;
        }
        إذا (forkp->joinType().joinAny()) join_ڳڻپ = 1;
        // Set the join counter
        خودڪار* ثابت initp = جديد AstCMethodHard{flp, جديد AstVarRef{flp, forkVscp, VAccess::WRITE},
                                               VCMethod::FORK_INIT, جديد AstConst{flp, join_ڳڻپ}};
        initp->dtype_سيٽ_Void();
        جوڙيو_Process_Info(initp);
        forkp->جوڙيو_هتي_This_As_ايندڙ(initp->makeStmt());
        // Await the join at the end
        خودڪار* ثابت joinp = جديد AstCMethodHard{flp, جديد AstVarRef{flp, forkVscp, VAccess::WRITE},
                                               VCMethod::FORK_JOIN};
        joinp->dtype_سيٽ_Void();
        جوڙيو_Process_Info(joinp);
        جوڙيو_Debug_Info(joinp);
        forkp->جوڙيو_ايندڙ_هتي(جديد AstCAwait{flp, joinp});
    }

    // `procp` shall be a NodeProcedure/CFunc/Begin and within it vars from `varsp` will be placed.
    // `varsp` vector of vars which shall be localized.
    ساكن فراغ localizeVars(Ast_ڳنڍ* ثابت procp, ثابت std::ويڪٽر<AstVar*>& varsp) {
        UASSERT(procp, "procp is nullptr");
        AstVar* newVarsp = خالي_اشارو;
        لكل (AstVar* ثابت varp : varsp) {
            varp->funcLocal(سچ);
            newVarsp = Ast_ڳنڍ::جوڙيو_ايندڙ_Null(newVarsp, varp->unlink_Fr_پٺ());
        }
        إذا (Ast_ڳنڍ_Procedure* ثابت ڳنڍ_Procp = VN_CAST(procp, ڳنڍ_Procedure)) {
            ڳنڍ_Procp->prependStmtsp(newVarsp);
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

    Ast_ڳنڍ_اظهار* delay_منزل_Integral(AstDelay* nodep, Ast_ڳنڍ_اظهار* valuep) {
        ثابت AstConst* ثابت constp = VN_CAST(valuep, Const);
        // The fork start sentinel is already in ticks
        إذا (constp && constp->منزل_UQuad() == std::numeric_limits<uint64_t>::max()) أرجع valuep;
        أرجع V3Timing::delay_منزل_Ticks(valuep, nodep->timeunit());
    }

    // VISITORS
    فراغ گهمو(Ast_ڳنڍ_Module* nodep) override {
        UASSERT_OBJ(!m_classp, nodep, "Module or class under class");
        VL_RESTORER(m_classp);
        m_classp = VN_CAST(nodep, Class);
        VL_RESTORER(m_forkCnt);
        m_forkCnt = 0;
        iterateChildren(nodep);
    }
    فراغ گهمو(AstScope* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        Sen_اظهار_Builder sen_اظهار_Builder{m_scopep};
        {  // Restore m_senExprBuilderp before destroying senExprBuilder
            VL_RESTORER(m_sen_اظهار_Builderp);
            m_sen_اظهار_Builderp = &sen_اظهار_Builder;
            iterateChildren(nodep);
        }
    }
    فراغ گهمو(Ast_سرگرم* nodep) override {
        VL_RESTORER(m_activep);
        m_activep = nodep;
        iterateChildren(nodep);
    }
    فراغ گهمو(Ast_ڳنڍ_Procedure* nodep) override {
        VL_RESTORER(m_procp);
        VL_RESTORER(m_hasProcess);
        m_procp = nodep;
        m_hasProcess = hasFlags(nodep, T_HAS_PROC);
        VL_RESTORER(m_underProcedure);
        m_underProcedure = سچ;
        iterateChildren(nodep);
        إذا (hasFlags(nodep, T_SUSPENDEE)) nodep->سيٽ_Suspendable();
        إذا (hasFlags(nodep, T_HAS_PROC)) nodep->سيٽ_Need_Process();
    }
    فراغ گهمو(AstInitial* nodep) override {
        گهمو(static_cast<Ast_ڳنڍ_Procedure*>(nodep));
        إذا (nodep->needProcess() && !nodep->user1_سيٽ_Once()) {
            nodep->جوڙيو_Stmtsp(
                جديد AstCStmt{nodep->fileline(), "vlProcess->state(VlProcess::FINISHED);"});
        }
    }
    فراغ گهمو(Ast_Jump_بلاڪ* nodep) override {
        VL_RESTORER(m_under_Jump_بلاڪ);
        m_under_Jump_بلاڪ = سچ;
        گهمو(static_cast<Ast_ڳنڍ_Stmt*>(nodep));
    }
    فراغ گهمو(AstAlways* nodep) override {
        إذا (nodep->user1_سيٽ_Once()) أرجع;
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
            جوڙيو_Flags(nodep, T_SUSPENDEE);

        iterateChildren(nodep);
        إذا (hasFlags(nodep, T_HAS_PROC)) nodep->سيٽ_Need_Process();
        إذا (!hasFlags(nodep, T_SUSPENDEE)) أرجع;
        nodep->سيٽ_Suspendable();
        File_سٽ* ثابت flp = nodep->fileline();
        Ast_Sen_وڻ* ثابت sentreep = m_activep->sentreep();
        إذا (sentreep->hasClocked()) {
            Ast_ڳنڍ* ثابت bodysp = nodep->stmtsp()->unlink_Fr_پٺ_With_ايندڙ();
            خودڪار* ثابت controlp = جديد AstEventControl{flp, sentreep->clone_وڻ(خطأ), bodysp};
            nodep->جوڙيو_Stmtsp(controlp);
            iterate(controlp);
        }
        // Note: The 'while (true)' outer loop will be added in V3Sched
        خودڪار* ثابت activep = جديد Ast_سرگرم{
            flp, "", جديد Ast_Sen_وڻ{flp, جديد Ast_Sen_شي{flp, Ast_Sen_شي::Initial{}}}};
        activep->sen_وڻ_Storep(activep->sentreep());
        activep->جوڙيو_Stmtsp(nodep->unlink_Fr_پٺ());
        m_activep->جوڙيو_ايندڙ_هتي(activep);
    }
    فراغ گهمو(AstCFunc* nodep) override {
        VL_RESTORER(m_procp);
        VL_RESTORER(m_hasProcess);
        VL_RESTORER(m_underIfaceCFunc);
        m_procp = nodep;
        m_hasProcess = hasFlags(nodep, T_HAS_PROC);
        m_underIfaceCFunc = VN_IS(m_scopep->modp(), Iface);
        iterateChildren(nodep);
        إذا (hasFlags(nodep, T_HAS_PROC)) nodep->سيٽ_Need_Process();
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
    فراغ گهمو(Ast_ڳنڍ_CCall* nodep) override {
        AstCFunc* ثابت funcp = nodep->funcp();

        // Skip automatic covergroup sampling calls
        إذا (funcp->isCovergroupSample()) {
            iterateChildren(nodep);
            أرجع;
        }

        إذا (funcp->needProcess()) m_hasProcess = سچ;
        إذا (hasFlags(funcp, T_SUSPENDEE) && !nodep->user1_سيٽ_Once()) {  // If suspendable
            // Calls to suspendables are always void return type, hence parent must be StmtExpr
            Ast_Stmt_اظهار* ثابت stmtp = VN_AS(nodep->backp(), Stmt_اظهار);
            stmtp->replaceWith(جديد AstCAwait{nodep->fileline(), nodep->unlink_Fr_پٺ()});
            VL_DO_DANGLING(ڌڪو_Deletep(stmtp), stmtp);
        }
        iterateChildren(nodep);
    }
    فراغ گهمو(AstDelay* nodep) override {
        UASSERT_OBJ(!nodep->is_چڪر_Delay(), nodep,
                    "Cycle delays should have been handled in V3AssertPre");
        File_سٽ* ثابت flp = nodep->fileline();

        Ast_ڳنڍ_اظهار* valuep = delay_منزل_Integral(nodep, nodep->lhsp()->unlink_Fr_پٺ());

        // Statistics
        إذا (valuep->isZero()) {
            ++m_statZeroDelays;
        } وإلا إذا (VN_IS(valuep, Const)) {
            ++m_statConstDelays;
        } وإلا {
            ++m_statVariableDelays;
        }

        // Decide scheduling support for #0
        إذا (v3Global.opt.schedZeroDelay().is_سيٽ_True()) {
            // User said to schedule for #0 support, nothing else to do
            v3Global.سيٽ_استعمال_ڪري_Zero_Delay();
        } وإلا إذا (v3Global.opt.schedZeroDelay().is_سيٽ_False()) {
            // User said to schedule without #0 support. Still warn if a static #0 delay exists
            إذا (valuep->isZero()) {
                nodep->v3warn(
                    ZERODLY,
                    "Static #0 delay exists, but '--no-sched-zero-delay' was given.\n"
                        << nodep->warn_وڌيڪ()  //
                        << "... Can proceed, but this will fail at runtime if executed.");
            }
        } وإلا {
            // User did not express preference, decide based on presence of delays
            إذا (valuep->isZero()) {
                // Statically known #0 delay exists, schedule for #0 support
                v3Global.سيٽ_استعمال_ڪري_Zero_Delay();
                m_hasStaticZeroDelay = سچ;
                // Don't warn on variable delays, as no point
                m_unknownDelayFlps.clear();
            } وإلا إذا (staticallyNonZeroDelay(valuep)) {
                // Delay is dynamic, but every statically known outcome is non-zero.
                // So we don't need #0 delay support and there should be no warning.
            } وإلا إذا (!VN_IS(valuep, Const)) {
                // Delay is not known at compiile time. Conservatively schedule for #0 support,
                // but warn if no static #0 delays used as performance might be improved
                v3Global.سيٽ_استعمال_ڪري_Zero_Delay();
                إذا (!m_hasStaticZeroDelay) m_unknownDelayFlps.ڌڪو_پٺ(nodep->fileline());
            }
        }

        // Replace self with a 'co_await dlySched.delay(<valuep>)'
        AstCMethodHard* ثابت delayMethodp = جديد AstCMethodHard{
            flp, جديد AstVarRef{flp, getCreateDelayScheduler(), VAccess::WRITE},
            VCMethod::SCHED_DELAY, valuep};
        delayMethodp->dtype_سيٽ_Void();
        جوڙيو_Process_Info(delayMethodp);
        جوڙيو_Debug_Info(delayMethodp);
        // Create the co_await
        Ast_ڳنڍ* ثابت awaitp = جديد AstCAwait{flp, delayMethodp, get_Create_Delay_Sen_وڻ()};
        // Relink child statements after the co_await
        إذا (Ast_ڳنڍ* ثابت stmtsp = nodep->stmtsp()) {
            awaitp->جوڙيو_ايندڙ(stmtsp->unlink_Fr_پٺ_With_ايندڙ());
        }
        nodep->replaceWith(awaitp);
        VL_DO_DANGLING(nodep->delete_وڻ(), nodep);
    }
    فراغ گهمو(AstEventControl* nodep) override {
        // Do not allow waiting on local named events, as they get enqueued for clearing, but can
        // go out of scope before that happens
        إذا (!nodep->sentreep()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Event control with implicit sensitivity (@*)"
                          " in this context; use an explicit sensitivity list instead");
            VL_DO_DANGLING(ڌڪو_Deletep(nodep->unlink_Fr_پٺ()), nodep);
            أرجع;
        }
        File_سٽ* ثابت flp = nodep->fileline();
        // Relink child statements after the event control
        إذا (nodep->stmtsp()) nodep->جوڙيو_ايندڙ_هتي(nodep->stmtsp()->unlink_Fr_پٺ_With_ايندڙ());
        إذا (needDynamicTrigger(nodep->sentreep())) {
            // Create the trigger variable and init it with 0
            AstVarScope* ثابت trigvscp
                = create_عارضي(flp, m_dyn_Trig_نالا.get(nodep), nodep->ڳوليو_ٻٽو_انگ_DType(), nodep);
            خودڪار* ثابت initp = جديد Ast_سونپيو{flp, جديد AstVarRef{flp, trigvscp, VAccess::WRITE},
                                              جديد AstConst{flp, AstConst::ٻٽو_انگ_False{}}};
            // Await the eval step with the dynamic trigger scheduler. First, create the method
            // call
            خودڪار* ثابت evalMethodp = جديد AstCMethodHard{
                flp, جديد AstVarRef{flp, getCreateDynamicTriggerScheduler(), VAccess::WRITE},
                VCMethod::SCHED_EVALUATION};
            evalMethodp->dtype_سيٽ_Void();
            جوڙيو_Process_Info(evalMethodp);
            خودڪار* ثابت sentreep = nodep->sentreep();
            جوڙيو_Event_Debug_Info(evalMethodp, sentreep);
            // Create the co_await
            AstCAwait* ثابت awaitEvalp
                = جديد AstCAwait{flp, evalMethodp, get_Create_Dynamic_Trigger_Sen_وڻ()};
            // Construct the sen expression for this sentree
            UASSERT_OBJ(m_sen_اظهار_Builderp, nodep, "No SenExprBuilder for this scope");
            خودڪار* ثابت assignp = جديد Ast_سونپيو{flp, جديد AstVarRef{flp, trigvscp, VAccess::WRITE},
                                                m_sen_اظهار_Builderp->build(sentreep).پهريون};
            // Get the SenExprBuilder results
            ثابت Sen_اظهار_Builder::Results senResults = m_sen_اظهار_Builderp->getAndClearResults();
            localizeVars(m_procp, senResults.m_vars);
            // If post updates are destructive (e.g. clearFired on events), perform a
            // conservative pre-clear once before entering the wait loop so stale state from a
            // previous wait does not cause an immediate false-positive trigger.
            ثابت منطقي hasDestructivePostUpdates = !senResults.m_destructivePostUpdates.خالي_جايون();
            // Create the trigger eval loop, which will await the evaluation step and check the
            // trigger
            Ast_ڳنڍ_اظهار* ثابت condp
                = جديد AstLogNot{flp, جديد AstVarRef{flp, trigvscp, VAccess::READ}};
            AstLoop* ثابت loopp = جديد AstLoop{flp};
            loopp->جوڙيو_Stmtsp(جديد AstLoopTest{flp, loopp, condp});
            loopp->جوڙيو_Stmtsp(awaitEvalp);
            // Put pre updates before the trigger check and assignment
            لكل (Ast_ڳنڍ_Stmt* ثابت stmtp : senResults.m_preUpdates) loopp->جوڙيو_Stmtsp(stmtp);
            // Then the trigger check and assignment
            loopp->جوڙيو_Stmtsp(assignp);
            // Let the dynamic trigger scheduler know if this trigger was set
            // If it was, a call to the scheduler's evaluate() will return true
            AstCMethodHard* ثابت anyTriggeredMethodp = جديد AstCMethodHard{
                flp, جديد AstVarRef{flp, getCreateDynamicTriggerScheduler(), VAccess::WRITE},
                VCMethod::SCHED_ANY_TRIGGERED, جديد AstVarRef{flp, trigvscp, VAccess::READ}};
            anyTriggeredMethodp->dtype_سيٽ_Void();
            loopp->جوڙيو_Stmtsp(anyTriggeredMethodp->makeStmt());
            // If the post update is destructive (e.g. event vars are cleared), create an await for
            // the post update step
            إذا (hasDestructivePostUpdates) {
                AstCAwait* ثابت awaitPostUpdatep = awaitEvalp->clone_وڻ(خطأ);
                VN_AS(awaitPostUpdatep->exprp(), CMethodHard)->طريقو(VCMethod::SCHED_POST_UPDATE);
                loopp->جوڙيو_Stmtsp(awaitPostUpdatep);
            }
            // Put the post updates at the end of the loop
            لكل (Ast_ڳنڍ_Stmt* ثابت stmtp : senResults.m_postUpdates) loopp->جوڙيو_Stmtsp(stmtp);
            // Finally, await the resumption step in 'act'
            AstCAwait* ثابت awaitResumep = awaitEvalp->clone_وڻ(خطأ);
            VN_AS(awaitResumep->exprp(), CMethodHard)->طريقو(VCMethod::SCHED_RESUMPTION);
            Ast_ڳنڍ::جوڙيو_ايندڙ<Ast_ڳنڍ_Stmt, Ast_ڳنڍ_Stmt>(loopp, awaitResumep);
            // Replace the event control with one explicit stmt chain:
            //   init -> [inits] -> [optional destructive pre-clears] -> loop -> awaitResumption
            Ast_ڳنڍ_Stmt* chainp = خالي_اشارو;
            chainp = Ast_ڳنڍ::جوڙيو_ايندڙ_Null(chainp, initp);
            لكل (Ast_ڳنڍ_Stmt* ثابت stmtp : senResults.m_inits) {
                chainp = Ast_ڳنڍ::جوڙيو_ايندڙ_Null(chainp, stmtp);
            }
            إذا (hasDestructivePostUpdates) {
                لكل (Ast_ڳنڍ_Stmt* ثابت stmtp : senResults.m_destructivePostUpdates) {
                    chainp = Ast_ڳنڍ::جوڙيو_ايندڙ_Null(chainp, stmtp->clone_وڻ(خطأ));
                }
            }
            chainp = Ast_ڳنڍ::جوڙيو_ايندڙ_Null(chainp, loopp);
            nodep->replaceWith(chainp);
        } وإلا {
            خودڪار* ثابت sentreep = m_finder.get_Sen_وڻ(nodep->sentreep());
            nodep->sentreep()->unlink_Fr_پٺ()->delete_وڻ();
            // Get this sentree's trigger scheduler
            // Replace self with a 'co_await trigSched.trigger()'
            خودڪار* ثابت triggerMethodp = جديد AstCMethodHard{
                flp, جديد AstVarRef{flp, getCreateTriggerSchedulerp(sentreep), VAccess::WRITE},
                VCMethod::SCHED_TRIGGER};
            triggerMethodp->dtype_سيٽ_Void();
            // If it should be committed immediately, pass true, otherwise false
            triggerMethodp->جوڙيو_Pinsp(nodep->user2() ? جديد AstConst{flp, AstConst::ٻٽو_انگ_True{}}
                                                    : جديد AstConst{flp, AstConst::ٻٽو_انگ_False{}});
            جوڙيو_Process_Info(triggerMethodp);
            جوڙيو_Event_Debug_Info(triggerMethodp, sentreep);
            // Create the co_await
            AstCAwait* ثابت awaitp = جديد AstCAwait{flp, triggerMethodp, sentreep};
            nodep->replaceWith(awaitp);
        }
        VL_DO_DANGLING(nodep->delete_وڻ(), nodep);
    }
    فراغ گهمو(Ast_ڳنڍ_سونپيو* nodep) override {
        // Only process once to avoid infinite loops (due to the net delay)
        إذا (nodep->user1_سيٽ_Once()) أرجع;
        UINFO(9, "control-visit " << nodep);
        File_سٽ* ثابت flp = nodep->fileline();
        Ast_ڳنڍ* controlp = ضرب_ڪندڙ_ٻاهريون_Timing_Control(nodep);
        ثابت منطقي اندريون_سونپيو_Dly = VN_IS(nodep, سونپيو_Dly);
        // Handle the intra assignment timing control
        // Transform if:
        // * there's a timing control in the assignment
        // * the assignment is an AssignDly and it's in a non-inlined function
        إذا (!controlp && (!اندريون_سونپيو_Dly || m_underProcedure)) {
            iterateChildren(nodep);
            أرجع;
        }
        // The timing control, if any, which is put in a fork for an NBA
        Ast_ڳنڍ* ثابت timingp = controlp;
        // Insert new vars before the timing control if we're in a function; in a process we can't
        // do that, as after V3Begin only modules and functions declare variables. These
        // intra-assignment vars will later be passed to forked processes by value.
        Ast_ڳنڍ* وجهو_Beforep = m_underProcedure ? خالي_اشارو : controlp;
        // Special case for NBA
        إذا (اندريون_سونپيو_Dly) {
            // Put it in a fork so it doesn't block
            // Could already be the only thing directly under a fork, reuse that if possible
            AstFork* forkp = !nodep->nextp() ? VN_CAST(nodep->پهريون_Abovep(), Fork) : خالي_اشارو;
            إذا (!forkp) forkp = جديد AstFork{flp, VJoinType::JOIN_NONE};
            إذا (!m_underProcedure) {
                // If it's in a function, it won't be handled by V3Delayed
                // Put it behind an additional named event that gets triggered in the NBA region
                AstEventControl* ثابت nbaEventControlp = createNbaEventControl(flp);
                Ast_سونپيو* ثابت trigAssignp = createNbaEventTriggerAssignment(flp);
                nodep->replaceWith(trigAssignp);
                trigAssignp->جوڙيو_ايندڙ_هتي(nbaEventControlp);
                nbaEventControlp->جوڙيو_Stmtsp(nodep);
                وجهو_Beforep = forkp;
                إذا (!controlp) controlp = nbaEventControlp;
            }
            controlp->replaceWith(forkp);
            AstBegin* beginp = VN_CAST(controlp, Begin);
            إذا (!beginp) beginp = جديد AstBegin{nodep->fileline(), "", controlp, خطأ};
            forkp->جوڙيو_Forksp(beginp);
            جوڙيو_Flags(beginp, T_NBA_UPDATE);
            controlp = forkp;
        }
        UASSERT_OBJ(nodep, controlp, "Assignment should have timing control");
        // Move `insertBeforep` into `AstCLocalScope` if necessary to avoid jumping over
        // a variable initialization that would be inserted before `insertBeforep`. All
        // access to this variable should be contained within returned `AstCLocalScope`.
        إذا (m_under_Jump_بلاڪ) جوڙيو_CLocal_Scope(flp, وجهو_Beforep);
        // Function for replacing values with intermediate variables
        ثابت خودڪار replaceWithIntermediate = [&](Ast_ڳنڍ_اظهار* ثابت valuep,
                                                 ثابت std::string& name) {
            AstVarScope* ثابت newvscp = create_عارضي(flp, name, valuep->dtypep(), وجهو_Beforep);
            // A pending update keeps its own value, even if its process schedules more
            إذا (اندريون_سونپيو_Dly && !وجهو_Beforep) newvscp->fork_By_قدر(سچ);
            valuep->replaceWith(جديد AstVarRef{flp, newvscp, VAccess::READ});
            controlp->جوڙيو_هتي_This_As_ايندڙ(
                جديد Ast_سونپيو{flp, جديد AstVarRef{flp, newvscp, VAccess::WRITE}, valuep});
        };
        // NBAs with delays evaluate the indices selecting the target now (IEEE 1800-2023 10.4.2),
        // in order, each as a whole, also the selects and function calls in it. So follow the
        // selects of the target up from the variable, not into their indices. As an example, this
        // transforms 'x[a[i]][f(j)] <= #1 y' into 't0 = a[i]; t1 = f(j); x[t0][t1] <= #1 y'.
        // V3Const has split NBAs to concatenations, also streamed, into NBAs to each part.
        إذا (اندريون_سونپيو_Dly) {
            لكل (Ast_ڳنڍ* exprp = nodep->lhsp()->بنياد_Fromp(سچ); exprp != nodep;
                 exprp = exprp->پهريون_Abovep()) {
                إذا (AstSel* ثابت selp = VN_CAST(exprp, Sel)) {
                    // widthp should be const
                    إذا (!VN_IS(selp->lsbp(), Const)) {
                        replaceWithIntermediate(selp->lsbp(), m_intra_Lsb_نالا.get(nodep));
                    }
                } وإلا إذا (Ast_ڳنڍ_Sel* ثابت selp = VN_CAST(exprp, ڳنڍ_Sel)) {
                    إذا (!VN_IS(selp->bitp(), Const)) {
                        replaceWithIntermediate(selp->bitp(), m_intra_اشاريو_نالا.get(nodep));
                    }
                }
            }
        }
        // Replace the RHS with an intermediate value var
        replaceWithIntermediate(nodep->rhsp(), m_intra_قدر_نالا.get(nodep));
        إذا (!اندريون_سونپيو_Dly) أرجع;
        // The process executing the NBA evaluates its delay (IEEE 1800-2023 4.9.4), not the forked
        // process, if it has side effects, e.g. calling a function that checks its process. A pure
        // one is evaluated as the forked process starts, at once.
        إذا (AstDelay* ثابت delayp = VN_CAST(timingp, Delay)) {
            إذا (!delayp->lhsp()->is_خالص()) {
                replaceWithIntermediate(delayp->lhsp(), m_intra_قدر_نالا.get(nodep));
            }
        }
    }
    فراغ گهمو(AstFireEvent* nodep) override {
        // V3Delayed handles '->>' of a variable in a process. Like NBAs in non-inlined functions,
        // trigger other events from a fork awaiting the NBA region.
        إذا (!nodep->isDelayed() || (m_underProcedure && VN_IS(nodep->operandp(), VarRef))) {
            iterateChildren(nodep);
            أرجع;
        }
        File_سٽ* ثابت flp = nodep->fileline();
        Ast_سونپيو* ثابت trigAssignp = createNbaEventTriggerAssignment(flp);
        nodep->replaceWith(trigAssignp);
        AstFork* ثابت forkp = جديد AstFork{flp, VJoinType::JOIN_NONE};
        trigAssignp->جوڙيو_ايندڙ_هتي(forkp);
        إذا (m_under_Jump_بلاڪ) جوڙيو_CLocal_Scope(flp, forkp);
        // The triggered event is the one referenced now, so evaluate handles and indices now
        Ast_ڳنڍ_اظهار* ثابت eventp = nodep->operandp()->unlink_Fr_پٺ();
        ثابت خودڪار evalNow = [&](Ast_ڳنڍ_اظهار* ثابت valuep, V3_Unique_نالا& نالا) {
            // Like for an NBA, a process uses a module variable, passed to the fork by value
            AstVarScope* ثابت vscp = create_عارضي(flp, نالا.get(nodep), valuep->dtypep(),
                                                 m_underProcedure ? خالي_اشارو : forkp);
            إذا (m_underProcedure) vscp->fork_By_قدر(سچ);
            valuep->replaceWith(جديد AstVarRef{flp, vscp, VAccess::READ});
            // Unlike an event, a handle selecting one is only read
            valuep->foreach([](Ast_ڳنڍ* ثابت np) {
                إذا (Ast_ڳنڍ_Var_Ref* ثابت refp = VN_CAST(np, ڳنڍ_Var_Ref)) {
                    refp->access(VAccess::READ);
                } وإلا إذا (AstMemberSel* ثابت selp = VN_CAST(np, MemberSel)) {
                    selp->access(VAccess::READ);
                }
            });
            forkp->جوڙيو_هتي_This_As_ايندڙ(
                جديد Ast_سونپيو{flp, جديد AstVarRef{flp, vscp, VAccess::WRITE}, valuep});
        };
        Ast_ڳنڍ_اظهار* refp = eventp;
        طالما (Ast_ڳنڍ_Sel* ثابت selp = VN_CAST(refp, ڳنڍ_Sel)) {
            إذا (!VN_IS(selp->bitp(), Const)) evalNow(selp->bitp(), m_intra_اشاريو_نالا);
            refp = selp->fromp();
        }
        // Handle of a class (or virtual interface)
        إذا (AstMemberSel* ثابت selp = VN_CAST(refp, MemberSel)) {
            evalNow(selp->fromp(), m_intra_قدر_نالا);
        }
        AstEventControl* ثابت controlp = createNbaEventControl(flp);
        controlp->جوڙيو_Stmtsp(جديد AstFireEvent{flp, eventp, خطأ});
        AstBegin* ثابت beginp = جديد AstBegin{flp, "", controlp, خطأ};
        جوڙيو_Flags(beginp, T_NBA_UPDATE);
        forkp->جوڙيو_Forksp(beginp);
        VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
    }
    فراغ گهمو(Ast_سونپيو_W* nodep) override {
        File_سٽ* ثابت flp = nodep->fileline();
        // Get the net delay unless this assignment was created for handling the net delay (user1)
        AstDelay* ثابت netDelayp = nodep->user1() ? خالي_اشارو : nodep->getLhsNetDelay();
        إذا (netDelayp) {
            إذا (nodep->timingControlp()) {
                // If this assignment has a delay, create another one to handle the net delay
                AstVarScope* ثابت newvscp = m_سونپيو_WTmps.make(flp, m_scopep, nodep->dtypep());
                Ast_سونپيو_W* assignp = جديد Ast_سونپيو_W{
                    nodep->fileline(), nodep->lhsp()->unlink_Fr_پٺ(),
                    جديد AstVarRef{flp, newvscp, VAccess::READ}, netDelayp->clone_وڻ(خطأ)};
                assignp->user1(سچ);
                nodep->جوڙيو_ايندڙ_هتي(assignp);
                nodep->lhsp(جديد AstVarRef{flp, newvscp, VAccess::WRITE});
            } وإلا {
                // Else just use this one with the net delay
                nodep->timingControlp(netDelayp->clone_وڻ(خطأ));
            }
        }
        إذا (!nodep->timingControlp()) أرجع;
        // There will be some circular logic here, suppress the warning for newly created vars
        flp->modifyWarnOff(V3_Error_ڪوڊ::UNOPTFLAT, سچ);
        // Also suppress the warning for the LHS var, for cases like `assign #1 clk = ~clk;`
        // TODO: Restore the warning for other, non-delayed drivers
        nodep->lhsp()->foreach([](AstVarRef* refp) {
            إذا (refp->access().isWriteOrRW()) {
                refp->varp()->fileline()->modifyWarnOff(V3_Error_ڪوڊ::UNOPTFLAT, سچ);
            }
        });
        // Should be under an always
        AstAlways* ثابت alwaysp = VN_AS(m_procp, Always);
        // Convert it to an Assign; the new assign with intra delay will be handled by
        // visit(AstNodeAssign*)
        Ast_ڳنڍ_اظهار* ثابت lhs1p = nodep->lhsp()->unlink_Fr_پٺ();
        Ast_ڳنڍ_اظهار* ثابت rhs1p = nodep->rhsp()->unlink_Fr_پٺ();
        Ast_ڳنڍ* ثابت controlp = nodep->timingControlp()->unlink_Fr_پٺ();
        إذا (AstDelay* ثابت delayp = VN_CAST(controlp, Delay)) {
            إذا (Ast_ڳنڍ_اظهار* fallDelayp = delayp->fallDelay()) {
                fallDelayp = fallDelayp->unlink_Fr_پٺ();
                Ast_ڳنڍ_اظهار* lhsp = delayp->lhsp()->unlink_Fr_پٺ();
                // Use fall only for an all-zero value, rise otherwise.
                lhsp = delay_منزل_Integral(delayp, lhsp);
                fallDelayp = delay_منزل_Integral(delayp, fallDelayp);
                delayp->lhsp(
                    جديد AstCond{flp, جديد AstEq{flp, rhs1p->clone_وڻ(خطأ), جديد AstConst{flp, 0}},
                                fallDelayp, lhsp});
            }
        }
        Ast_سونپيو* ثابت assignp = جديد Ast_سونپيو{nodep->fileline(), lhs1p, rhs1p, controlp};
        // Put the assignment in a fork..join_none.
        AstFork* ثابت forkp = جديد AstFork{flp, VJoinType::JOIN_NONE};
        nodep->replaceWith(forkp);
        VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
        AstBegin* ثابت beginp = جديد AstBegin{flp, "", assignp, خطأ};
        forkp->جوڙيو_Forksp(beginp);
        گهمو(forkp);  // Visit now as we need to do some post-processing
        // IEEE 1800-2023 10.3.3 - if the RHS value differs from the currently scheduled value to
        // be assigned, the currently scheduled assignment is descheduled. To keep track if an
        // assignment should be descheduled, each scheduled assignment event has a 'generation',
        // and if at assignment time its generation differs from the current generation, it won't
        // be performed
        Ast_سونپيو* ثابت preAssignp = VN_AS(beginp->stmtsp(), سونپيو);
        UASSERT_OBJ(preAssignp, alwaysp, "Pre-assign should be there from visit(AstFork)");
        Ast_سونپيو* ثابت postAssignp = VN_AS(preAssignp->nextp()->nextp(), سونپيو);
        UASSERT_OBJ(postAssignp, alwaysp, "Post-assign should be there from visit(AstFork)");
        // Increment generation and copy it to a local
        AstVarScope* ثابت generationVarp
            = m_سونپيو_WGens.make(flp, m_scopep, alwaysp->ڳوليو_UInt64_DType());
        AstVarScope* ثابت genLocalVarp
            = create_عارضي(flp, generationVarp->varp()->name() + "__local",
                         alwaysp->ڳوليو_UInt64_DType(), preAssignp);
        preAssignp->جوڙيو_هتي_This_As_ايندڙ(
            جديد Ast_سونپيو{flp, جديد AstVarRef{flp, generationVarp, VAccess::WRITE},
                          جديد Ast_جوڙيو{flp, جديد AstVarRef{flp, generationVarp, VAccess::READ},
                                     جديد AstConst{flp, 1}}});
        preAssignp->جوڙيو_هتي_This_As_ايندڙ(
            جديد Ast_سونپيو{flp, جديد AstVarRef{flp, genLocalVarp, VAccess::WRITE},
                          جديد AstVarRef{flp, generationVarp, VAccess::READ}});
        // If the current generation is same as the one saved in the local var, assign
        beginp->جوڙيو_Stmtsp(
            جديد AstIf{flp,
                      جديد AstEq{flp, جديد AstVarRef{flp, generationVarp, VAccess::READ},
                                جديد AstVarRef{flp, genLocalVarp, VAccess::READ}},
                      postAssignp->unlink_Fr_پٺ()});
        // Save scheduled RHS value before delay
        AstVarScope* ثابت tmpVarp
            = m_سونپيو_WTmps.make(flp, m_scopep, preAssignp->rhsp()->dtypep());
        AstVarRef* ثابت tmp_سونپيو_Rhsp = VN_AS(preAssignp->lhsp(), VarRef)->clone_وڻ(خطأ);
        tmp_سونپيو_Rhsp->access(VAccess::WRITE);
        preAssignp->جوڙيو_ايندڙ_هتي(
            جديد Ast_سونپيو{flp, جديد AstVarRef{flp, tmpVarp, VAccess::WRITE}, tmp_سونپيو_Rhsp});
        // If the RHS is different from the currently scheduled value, schedule the new assignment
        // The generation will increase, effectively 'descheduling' the previous assignment.
        Ast_ڳنڍ_اظهار* ثابت didNotInitp
            = جديد AstLogNot{flp, جديد AstCExpr{flp, "vlSymsp->__Vm_didInit", 1}};
        AstVarRef* ثابت پهريون_Iterp
            = جديد AstVarRef{flp, m_netlistp->stl_پهريون_Iterationp(), VAccess::READ};
        Ast_ڳنڍ_اظهار* ثابت schedCondp
            = جديد AstLogOr{flp,
                           جديد AstNeq{flp, preAssignp->rhsp()->clone_وڻ(خطأ),
                                      جديد AstVarRef{flp, tmpVarp, VAccess::READ}},
                           جديد AstLogAnd{flp, didNotInitp, پهريون_Iterp}};
        alwaysp->جوڙيو_Stmtsp(جديد AstIf{flp, schedCondp, forkp->unlink_Fr_پٺ()});
    }
    فراغ گهمو(AstDisableFork* nodep) override {
        إذا (m_hasProcess) أرجع;
        // never reached by any process; remove to avoid compilation error
        VL_DO_DANGLING(nodep->unlink_Fr_پٺ()->delete_وڻ(), nodep);
    }
    فراغ گهمو(AstWaitFork* nodep) override {
        إذا (m_hasProcess) {
            File_سٽ* ثابت flp = nodep->fileline();
            AstCExpr* ثابت exprp = جديد AstCExpr{flp, "vlProcess->completedFork()", 1};
            AstWait* ثابت waitp = جديد AstWait{flp, exprp, خالي_اشارو};
            nodep->replaceWith(waitp);
        } وإلا {
            // never reached by any process; remove to avoid compilation error
            nodep->unlink_Fr_پٺ();
        }
        VL_DO_DANGLING(nodep->delete_وڻ(), nodep);
    }
    فراغ گهمو(AstWait* nodep) override {
        // Wait on changed events related to the vars in the wait statement
        UINFO(9, "control-visit " << nodep);
        File_سٽ* ثابت flp = nodep->fileline();
        Ast_ڳنڍ* ثابت stmtsp = nodep->stmtsp();
        إذا (stmtsp) stmtsp->unlink_Fr_پٺ_With_ايندڙ();
        Ast_ڳنڍ_اظهار* ثابت condp = V3Const::constifyEdit(nodep->condp()->unlink_Fr_پٺ());
        خودڪار* ثابت constp = VN_CAST(condp, Const);
        إذا (constp) {
            إذا (constp->isZero()) {
                // We have to await forever instead of simply returning in case we're deep in a
                // callstack
                AstCMethodHard* ثابت foreverMethodp = جديد AstCMethodHard{
                    flp, جديد AstVarRef{flp, getCreateDelayScheduler(), VAccess::WRITE},
                    VCMethod::SCHED_WAIT_FOREVER};
                foreverMethodp->dtype_سيٽ_Void();
                جوڙيو_Process_Info(foreverMethodp);
                جوڙيو_Debug_Info(foreverMethodp);
                AstCAwait* ثابت awaitp = جديد AstCAwait{flp, foreverMethodp};
                nodep->replaceWith(awaitp);
                إذا (stmtsp) VL_DO_DANGLING(stmtsp->delete_وڻ(), stmtsp);
                VL_DO_DANGLING(condp->delete_وڻ(), condp);
            } وإلا {
                nodep->v3fatalSrc("constant wait should have been removed in "
                                  "TimingSuspendableVisitor::visit(AstWait)");
            }
        } وإلا إذا (needDynamicTrigger(condp)) {
            // No point in making a sentree, just use the expression as sensitivity
            // Put the event control in an if so we only wait if the condition isn't met already
            خودڪار* ثابت ifp = جديد AstIf{
                flp, جديد AstLogNot{flp, condp},
                جديد AstEventControl{flp,
                                    جديد Ast_Sen_وڻ{flp, جديد Ast_Sen_شي{flp, VEdgeType::ET_TRUE,
                                                                       condp->clone_وڻ(خطأ)}},
                                    خالي_اشارو}};
            إذا (stmtsp) Ast_ڳنڍ::جوڙيو_ايندڙ<Ast_ڳنڍ, Ast_ڳنڍ>(ifp, stmtsp);
            nodep->replaceWith(ifp);
        } وإلا {
            // We are using a global sentree, so we cannot use ET_TRUE, as that could lead to the
            // active region never converging. Because of this, we need to use ET_CHANGED in a
            // loop.
            AstEventControl* ثابت controlp = جديد AstEventControl{
                flp,
                جديد Ast_Sen_وڻ{
                    flp, جديد Ast_Sen_شي{flp, VEdgeType::ET_بدليو, condp->clone_وڻ(خطأ)}},
                خالي_اشارو};
            controlp->user2(سچ);  // Commit immediately
            AstLoop* ثابت loopp = جديد AstLoop{flp};
            loopp->جوڙيو_Stmtsp(جديد AstLoopTest{flp, loopp, جديد AstLogNot{flp, condp}});
            loopp->جوڙيو_Stmtsp(controlp);
            إذا (stmtsp) Ast_ڳنڍ::جوڙيو_ايندڙ<Ast_ڳنڍ, Ast_ڳنڍ>(loopp, stmtsp);
            nodep->replaceWith(loopp);
        }
        VL_DO_DANGLING(nodep->delete_وڻ(), nodep);
    }
    فراغ گهمو(AstBegin* nodep) override {
        VL_RESTORER(m_procp);
        VL_RESTORER(m_hasProcess);
        m_hasProcess |= hasFlags(nodep, T_HAS_PROC);
        // A pending NBA update is not a subprocess of the process that scheduled it, so it is
        // unaffected by 'disable fork' and 'wait fork' (IEEE 1800-2023 9.6.1, 9.6.3)
        إذا (hasFlags(nodep, T_NBA_UPDATE)) m_hasProcess = خطأ;
        m_procp = nodep;
        إذا (m_hasProcess) nodep->سيٽ_Need_Process();
        iterateChildren(nodep);
    }
    فراغ گهمو(AstFork* nodep) override {
        إذا (nodep->user1_سيٽ_Once()) أرجع;
        UINFO(9, "control-visit " << nodep);
        v3Global.سيٽ_استعمال_ڪري_Timing();

        // Create a unique name for this fork
        nodep->name("__Vfork_" + std::منزل_string(++m_forkCnt));

        // TODO: Should process nodep->stmtsp() in case an earlier pass
        //       inserted something there, but as of today we don't need to.

        // Process and name each fork
        سائيز_t idx = 0;  // Index for naming begins
        لكل (AstBegin *itemp = nodep->forksp(), *nextp; itemp; itemp = nextp) {
            nextp = VN_AS(itemp->nextp(), Begin);
            iterate(itemp);
            // Note: Even if we do not find any awaits, we cannot simply inline
            // the process here, as new awaits could be added later.

            // Name the begin (later the name will be used for a new function)
            itemp->name(nodep->name() + "__" + std::منزل_string(idx++));
            إذا (itemp->needProcess()) {
                itemp->جوڙيو_Stmtsp(
                    جديد AstCStmt{itemp->fileline(), "vlProcess->state(VlProcess::FINISHED);"});
            }
        }
        إذا (!nodep->joinType().joinNone()) makeForkJoin(nodep);
    }

    //--------------------
    فراغ گهمو(AstVar*) override {}  // Accelerate
    فراغ گهمو(Ast_ڳنڍ* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    واضح TimingControlVisitor(AstNetlist* nodep)
        : m_netlistp{nodep} {
        iterate(nodep);

        // If there is no static #0 in the design, but an unknown delay was found,
        // and the user did not specify preference, then warn on all unknown delays
        // as we will be assuming they can be #0, which can cause performance degradation.
        لكل (File_سٽ* ثابت flp : m_unknownDelayFlps) {
            flp->v3warn(ZERODLY,
                        "Value of # delay control statically unknown. Assuming it can be #0.\n"
                            << flp->warn_وڌيڪ()  //
                            << "... If all # delays are non-zero at runtime,\n"
                            << flp->warn_وڌيڪ()  //
                            << "... use '--no-sched-zero-delay' for improved performance.\n"
                            << flp->warn_وڌيڪ()  //
                            << "... If a real #0 is expected at runtime,\n"
                            << flp->warn_وڌيڪ()  //
                            << "... use '--sched-zero-delay' to suppress this warning.");
        }
    }
    ~TimingControlVisitor() override {
        V3Stats::جوڙيو_Stat("Timing, known #0 delays", m_statZeroDelays);
        V3Stats::جوڙيو_Stat("Timing, known #const delays", m_statConstDelays);
        V3Stats::جوڙيو_Stat("Timing, unknown #variable delays", m_statVariableDelays);
    }
};

//######################################################################
// Timing class functions

Ast_ڳنڍ_اظهار* V3Timing::delay_منزل_Ticks(Ast_ڳنڍ_اظهار* valuep, VTimescale timeunit) {
    File_سٽ* ثابت flp = valuep->fileline();
    إذا (ثابت AstConst* ثابت constp = VN_CAST(valuep, Const)) {
        إذا (constp->انگ().is1_قدم()) {
            // A step is one tick of the global time precision (IEEE 1800-2023 3.14.3)
            VL_DO_DANGLING(valuep->delete_وڻ(), valuep);
            AstConst* ثابت tickp = جديد AstConst{flp, AstConst::Unsized64{}, 1};
            tickp->dtype_سيٽ_ٻٽو_انگ_Sized(64, VSigning::UNSIGNED);
            أرجع tickp;
        }
        إذا (constp->isZero()) أرجع valuep;
    }
    UASSERT_OBJ(!timeunit.isNone(), valuep, "timeunit must be set");
    ثابت مزدوج timescale_ضرب_ڪندڙ_D
        = std::قوت(10.0, timeunit.power_جو_Ten() - v3Global.rootp()->timeprecision().power_جو_Ten());
    إذا (valuep->dtypep()->skipRefp()->isDouble()) {
        AstConst* ثابت tsfp = جديد AstConst{flp, AstConst::RealDouble{}, timescale_ضرب_ڪندڙ_D};
        valuep = جديد AstMulD{flp, valuep, tsfp};
        valuep = جديد AstRToIRoundS{flp, valuep};
        valuep->dtype_سيٽ_ٻٽو_انگ_Sized(64, VSigning::UNSIGNED);
    } وإلا {
        valuep->dtype_سيٽ_ٻٽو_انگ_Sized(64, VSigning::UNSIGNED);
        ثابت uint64_t timescale_ضرب_ڪندڙ_U = static_cast<uint64_t>(timescale_ضرب_ڪندڙ_D);
        AstConst* ثابت tsfp = جديد AstConst{flp, AstConst::Unsized64{}, timescale_ضرب_ڪندڙ_U};
        valuep = جديد AstMul{flp, valuep, tsfp};
    }
    أرجع V3Const::constifyEdit(valuep);  // Simplify
}

فراغ V3Timing::timingAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        ثابت VNUser1_اندريون_Use m_user1_اندريون_Use;
        ثابت VNUser2_اندريون_Use m_user2_اندريون_Use;
        { TimingSuspendableVisitor{nodep}; }
        إذا (v3Global.استعمال_ڪري_Timing()) TimingControlVisitor{nodep};
    }
    V3Global::dump_Check_Global_وڻ("timing", 0, dump_وڻ_Either_Level() >= 3);
}
