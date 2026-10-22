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

تعداد غوټه_Flag : uint8_t {
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

تعداد انتشار_Type : uint8_t {
    P_CALL = 1,  // Propagation through call to a function/task/method
    P_FORK = 2,  // Propagation due to fork's behavior
    P_SIGNATURE = 3,  // Propagation required to maintain C++ function's signature requirements
};

// Add timing flag to a node
ساكن فراغ جمع_کړه_Flags(Ast_غوټه* ثابت nodep, uint8_t flags) { nodep->user2Or(flags); }
// Check if a node has ALL of the expected flags set
ساكن منطقي hasFlags(Ast_غوټه* ثابت nodep, uint8_t flags) { أرجع !(~nodep->user2() & flags); }

// ######################################################################
//  Detect nodes affected by timing and/or requiring a process

صنف TimingSuspendableVisitor final : عام VNVisitor {
    // TYPES
    // Vertex of a dependency graph of suspendable nodes, e.g. if a node (process or task) is
    // suspendable, all its dependents should also be suspendable
    صنف DepVtx VL_NOT_FINAL : عام V3_ګراف_راس {
        VL_RTTI_IMPL(DepVtx, V3_ګراف_راس)
        AstClass* ثابت m_classp;  // Class associated with a method
        Ast_غوټه* ثابت m_nodep;  // AST node represented by this graph vertex

        // ACCESSORS
        string name() ثابت override {
            إذا (m_classp) {
                إذا (VN_IS(nodep(), CFunc)) {
                    أرجع cvt_منزل_Hex(nodep()) + ' ' + classp()->name() + "::" + nodep()->name();
                }
            }
            أرجع cvt_منزل_Hex(nodep()) + ' ' + nodep()->prettyTypeName();
        }
        File_کرښه* fileline() ثابت override { أرجع nodep()->fileline(); }

    عام:
        // CONSTRUCTORS
        DepVtx(V3_ګراف* graphp, Ast_غوټه* nodep, AstClass* classp)
            : V3_ګراف_راس{graphp}
            , m_classp{classp}
            , m_nodep{nodep} {}
        ~DepVtx() override = افتراضي;

        // ACCESSORS
        مجازي Ast_غوټه* nodep() ثابت VL_MT_STABLE { أرجع m_nodep; }
        مجازي Ast_غوټه* classp() ثابت VL_MT_STABLE { أرجع m_classp; }
    };

    صنف SuspendDepVtx final : عام DepVtx {
        VL_RTTI_IMPL(SuspendDepVtx, DepVtx)
        string dotColor() ثابت override {
            إذا (hasFlags(nodep(), T_SUSPENDER)) أرجع "red";
            إذا (hasFlags(nodep(), T_SUSPENDEE)) أرجع "blue";
            أرجع "black";
        }

    عام:
        SuspendDepVtx(V3_ګراف* graphp, Ast_غوټه* nodep, AstClass* classp)
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
        NeedsProcDepVtx(V3_ګراف* graphp, Ast_غوټه* nodep, AstClass* classp)
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
    ثابت VNUser3_دننه_Use m_user3_دننه_Use;
    ثابت VNUser4_دننه_Use m_user4_دننه_Use;

    // STATE
    VMemberMap m_memberMap;  // Member names cached for fast lookup
    AstClass* m_classp = تش_اشاره;  // Current class
    Ast_غوټه* m_procp = تش_اشاره;  // NodeProcedure/CFunc/Begin we're under
    uint8_t m_underFork = F_NONE;  // F_NONE or flags of a fork we are under
    V3_ګراف m_susp_ګراف;  // Dependency graph where a node is a dependency of another if it being
                          // suspendable makes the other node suspendable
    V3_ګراف m_proc_ګراف;  // Dependency graph where a node is a dependency of another if it being
                          // suspendable makes the other node suspendable

    // METHODS
    // Get or create the dependency vertex for the given node
    DepVtx* getSuspendDepVtx(Ast_غوټه* ثابت nodep) {
        AstClass* classp = تش_اشاره;
        إذا (AstCFunc* funcp = VN_CAST(nodep, CFunc)) {
            إذا (funcp->scopep() && funcp->scopep()->modp()) {
                classp = VN_CAST(funcp->scopep()->modp(), Class);
            }
        }
        إذا (!nodep->user3p()) nodep->user3p(جديد SuspendDepVtx{&m_susp_ګراف, nodep, classp});
        أرجع nodep->user3u().منزل<SuspendDepVtx*>();
    }
    DepVtx* getNeedsProcDepVtx(Ast_غوټه* ثابت nodep) {
        AstClass* classp = تش_اشاره;
        إذا (AstCFunc* funcp = VN_CAST(nodep, CFunc)) {
            إذا (funcp->scopep() && funcp->scopep()->modp()) {
                classp = VN_CAST(funcp->scopep()->modp(), Class);
            }
        }
        إذا (!nodep->user4p()) nodep->user4p(جديد NeedsProcDepVtx{&m_proc_ګراف, nodep, classp});
        أرجع nodep->user4u().منزل<NeedsProcDepVtx*>();
    }
    // Pass timing flag between nodes
    منطقي passFlag(ثابت Ast_غوټه* from, Ast_غوټه* منزل, غوټه_Flag flag) {
        إذا ((from->user2() & flag) && !(منزل->user2() & flag)) {
            جمع_کړه_Flags(منزل, flag);
            أرجع ریښتیا;
        }
        أرجع خطأ;
    }
    // Propagate flag to all nodes that depend on the given one
    فراغ propagateFlags(DepVtx* ثابت vxp, غوټه_Flag flag) {
        خپلکاری* ثابت parentp = vxp->nodep();
        لكل (V3_ګراف_څنډه& څنډه : vxp->بهر_څنډې()) {
            خپلکاری* ثابت depVxp = static_cast<DepVtx*>(څنډه.سر());
            Ast_غوټه* ثابت depp = depVxp->nodep();
            إذا (passFlag(parentp, depp, flag)) propagateFlags(depVxp, flag);
        }
    }
    کينډۍ <ډول_نوم T_Predicate>
    فراغ propagateFlagsIf(DepVtx* ثابت vxp, غوټه_Flag flag, T_Predicate p) {
        خپلکاری* ثابت parentp = vxp->nodep();
        لكل (V3_ګراف_څنډه& څنډه : vxp->بهر_څنډې()) {
            خپلکاری* ثابت depVxp = static_cast<DepVtx*>(څنډه.سر());
            Ast_غوټه* ثابت depp = depVxp->nodep();
            إذا (p(&څنډه) && passFlag(parentp, depp, flag)) propagateFlagsIf(depVxp, flag, p);
        }
    }
    کينډۍ <ډول_نوم T_Predicate>
    فراغ propagateFlagsReversedIf(DepVtx* ثابت vxp, غوټه_Flag flag, T_Predicate p) {
        خپلکاری* ثابت parentp = vxp->nodep();
        لكل (V3_ګراف_څنډه& څنډه : vxp->دننه_څنډې()) {
            خپلکاری* ثابت depVxp = static_cast<DepVtx*>(څنډه.fromp());
            Ast_غوټه* ثابت depp = depVxp->nodep();
            إذا (p(&څنډه) && passFlag(parentp, depp, flag))
                propagateFlagsReversedIf(depVxp, flag, p);
        }
    }

    // VISITORS
    فراغ لیدنه(AstClass* nodep) override {
        UASSERT_OBJ(!m_classp, nodep, "Class under class");
        VL_RESTORER(m_classp);
        m_classp = nodep;
        iterateChildren(nodep);
    }
    فراغ لیدنه(Ast_غوټه_Procedure* nodep) override {
        VL_RESTORER(m_procp);
        m_procp = nodep;
        getNeedsProcDepVtx(nodep);
        جمع_کړه_Flags(nodep, T_ALLOCS_PROC);
        إذا (VN_IS(nodep, Always)) {
            UINFO(9, "Always does " << (nodep->needProcess() ? "" : "NOT ") << "need process");
        }
        iterateChildren(nodep);
    }
    فراغ لیدنه(AstDisableFork* nodep) override {
        لیدنه(static_cast<Ast_غوټه*>(nodep));
        جمع_کړه_Flags(m_procp, T_FORCES_PROC | T_NEEDS_PROC);
    }
    فراغ لیدنه(AstWaitFork* nodep) override {
        لیدنه(static_cast<Ast_غوټه*>(nodep));
        جمع_کړه_Flags(m_procp, T_FORCES_PROC | T_NEEDS_PROC);
    }
    فراغ لیدنه(AstWait* nodep) override {
        UINFO(9, "suspendable-visit " << nodep);
        Ast_غوټه_بیان* ثابت condp = V3Const::constifyEdit(nodep->condp());
        إذا (AstConst* ثابت constp = VN_CAST(condp, Const)) {
            إذا (!nodep->fileline()->warnIsOff(V3_Error_کوډ::WAITCONST)) {
                condp->v3warn(WAITCONST, "Wait statement condition is constant");
            }
            إذا (!constp->isZero()) {
                // Remove AstWait before we track process as T_SUSPENDER
                إذا (Ast_غوټه* ثابت stmtsp = nodep->stmtsp()) {
                    stmtsp->unlink_Fr_شا_With_بل();
                    nodep->replaceWith(stmtsp);
                } وإلا {
                    nodep->unlink_Fr_شا();
                }
                VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
                أرجع;
            }
        }
        v3Global.سېټ_کاروي_Timing();
        إذا (m_procp) جمع_کړه_Flags(m_procp, T_SUSPENDEE | T_SUSPENDER | T_NEEDS_PROC);
        iterateChildren(nodep);
    }
    فراغ لیدنه(AstCFunc* nodep) override {
        VL_RESTORER(m_procp);
        m_procp = nodep;
        iterateChildren(nodep);
        إذا (nodep->needProcess()) جمع_کړه_Flags(nodep, T_FORCES_PROC | T_NEEDS_PROC);
        DepVtx* ثابت sVxp = getSuspendDepVtx(nodep);
        DepVtx* ثابت pVxp = getNeedsProcDepVtx(nodep);
        إذا (!m_classp) أرجع;

        // Go over overridden functions

        std::کتار<AstClassExtends*> extends;
        إذا (m_classp->extendsp()) extends.ټېل(m_classp->extendsp());

        طالما (!extends.تشه()) {
            AstClassExtends* ext_list = extends.front();
            extends.راوباسه();

            لكل (AstClassExtends* cextp = ext_list; cextp;
                 cextp = VN_AS(cextp->nextp(), ClassExtends)) {
                // TODO: It is possible that a methods the same name in the base class is not
                // actually overridden by our method. If this causes a problem, traverse to
                // the root of the inheritance hierarchy and check if the original method is
                // virtual or not.
                إذا (خپلکاری* ثابت overriddenp
                    = VN_CAST(m_memberMap.پیدا_کړه_Member(cextp->classp(), nodep->name()), CFunc)) {
                    // Suspendability and process affects typing, so they propagate both ways
                    DepVtx* ثابت overriddenSVxp = getSuspendDepVtx(overriddenp);
                    DepVtx* ثابت overriddenPVxp = getNeedsProcDepVtx(overriddenp);
                    جديد V3_ګراف_څنډه{&m_susp_ګراف, sVxp, overriddenSVxp, P_SIGNATURE};
                    جديد V3_ګراف_څنډه{&m_susp_ګراف, overriddenSVxp, sVxp, P_SIGNATURE};
                    جديد V3_ګراف_څنډه{&m_proc_ګراف, pVxp, overriddenPVxp, P_SIGNATURE};
                    جديد V3_ګراف_څنډه{&m_proc_ګراف, overriddenPVxp, pVxp, P_SIGNATURE};
                } وإلا {
                    AstClassExtends* نور_extends = cextp->classp()->extendsp();
                    إذا (نور_extends) extends.ټېل(نور_extends);
                }
            }
        }
    }
    فراغ لیدنه(Ast_غوټه_CCall* nodep) override {
        AstCFunc* ثابت funcp = nodep->funcp();
        UASSERT_OBJ(funcp, nodep, "AstNodeCCall must have non-null funcp post-link");
        UASSERT_OBJ(m_procp, nodep, "AstNodeCCall must be inside a procedure/CFunc/Begin");

        UINFO(9, "V3Timing: Processing CCall to " << funcp->name() << " in dependency graph\n");
        جديد V3_ګراف_څنډه{&m_susp_ګراف, getSuspendDepVtx(funcp), getSuspendDepVtx(m_procp), P_CALL};

        جديد V3_ګراف_څنډه{&m_proc_ګراف, getNeedsProcDepVtx(funcp), getNeedsProcDepVtx(m_procp),
                        P_CALL};

        iterateChildren(nodep);
    }
    فراغ لیدنه(AstBegin* nodep) override {
        VL_RESTORER(m_procp);
        VL_RESTORER(m_underFork);

        إذا (!m_underFork || (m_underFork & F_MIGHT_SUSPEND))
            جديد V3_ګراف_څنډه{&m_susp_ګراف, getSuspendDepVtx(nodep), getSuspendDepVtx(m_procp),
                            m_underFork ? P_FORK : P_CALL};

        جديد V3_ګراف_څنډه{&m_proc_ګراف, getNeedsProcDepVtx(nodep), getNeedsProcDepVtx(m_procp),
                        P_CALL};

        إذا (m_underFork) جمع_کړه_Flags(nodep, T_NEEDS_PROC | T_ALLOCS_PROC);

        m_procp = nodep;
        m_underFork = 0;
        iterateChildren(nodep);
    }
    فراغ لیدنه(AstFork* nodep) override {
        UINFO(9, "suspendable-visit " << nodep);
        VL_RESTORER(m_underFork);

        v3Global.سېټ_کاروي_Timing();  // Even if there are no event controls, we have to set this flag
                                   // so that transformForks() in V3SchedTiming gets called and
                                   // removes all forks and begins
        إذا (nodep->isTimingControl() && m_procp) {
            جمع_کړه_Flags(m_procp, T_SUSPENDEE | T_SUSPENDER);
            m_underFork |= F_MIGHT_SUSPEND;
        }
        m_underFork |= F_MIGHT_NEED_PROC;
        iterateChildren(nodep);
    }
    فراغ لیدنه(Ast_ټاکل_Dly* nodep) override {
        إذا (!VN_IS(m_procp, غوټه_Procedure)) v3Global.سېټ_کاروي_Timing();
        لیدنه(static_cast<Ast_غوټه*>(nodep));
    }
    فراغ لیدنه(AstFireEvent* nodep) override {
        إذا (nodep->isDelayed()
            && (!VN_IS(m_procp, غوټه_Procedure) || !VN_IS(nodep->operandp(), VarRef))) {
            v3Global.سېټ_کاروي_Timing();
        }
        لیدنه(static_cast<Ast_غوټه*>(nodep));
    }
    فراغ لیدنه(Ast_ټاکل_W* nodep) override {
        إذا (nodep->timingControlp()) v3Global.سېټ_کاروي_Timing();
        // Containing process will not suspend, don't mark it
    }
    فراغ لیدنه(Ast_غوټه* nodep) override {
        إذا (nodep->isTimingControl()) {
            v3Global.سېټ_کاروي_Timing();
            إذا (m_procp) جمع_کړه_Flags(m_procp, T_SUSPENDEE | T_SUSPENDER | T_NEEDS_PROC);
        }
        iterateChildren(nodep);
    }

    //--------------------
    فراغ لیدنه(AstVar*) override {}  // Accelerate

عام:
    // CONSTRUCTORS
    څرګند TimingSuspendableVisitor(AstNetlist* nodep) {
        iterate(nodep);
        m_susp_ګراف.remove_متعدي_څنډې();
        m_proc_ګراف.remove_متعدي_څنډې();
        // Propagate suspendability
        لكل (V3_ګراف_راس& vtx : m_susp_ګراف.vertices()) {
            DepVtx& depVtx = static_cast<DepVtx&>(vtx);
            إذا (hasFlags(depVtx.nodep(), T_SUSPENDEE)) propagateFlags(&depVtx, T_SUSPENDEE);
        }
        إذا (dump_ګراف_Level() >= 6) m_susp_ګراف.dumpDotFilePrefixed("timing_deps");

        // Propagate T_HAS_PROCESS
        لكل (V3_ګراف_راس& vtx : m_proc_ګراف.vertices()) {
            DepVtx& depVtx = static_cast<DepVtx&>(vtx);
            // Find processes that'll allocate VlProcess
            إذا (hasFlags(depVtx.nodep(), T_FORCES_PROC)) {
                propagateFlagsIf(&depVtx, T_FORCES_PROC, [&](ثابت V3_ګراف_څنډه* e) -> منطقي {
                    أرجع !hasFlags(static_cast<DepVtx*>(e->fromp())->nodep(), T_ALLOCS_PROC);
                });
            }
            // Mark nodes on paths between processes and statements that use VlProcess
            إذا (hasFlags(depVtx.nodep(), T_NEEDS_PROC)) {
                propagateFlagsIf(&depVtx, T_NEEDS_PROC, [&](ثابت V3_ګراف_څنډه* e) -> منطقي {
                    أرجع !hasFlags(static_cast<DepVtx*>(e->سر())->nodep(), T_ALLOCS_PROC);
                });
            }
        }
        لكل (V3_ګراف_راس& vtx : m_proc_ګراف.vertices()) {
            DepVtx& depVtx = static_cast<DepVtx&>(vtx);
            // Mark nodes that will be emitted with a VlProcess argument
            إذا (hasFlags(depVtx.nodep(), T_ALLOCS_PROC | T_FORCES_PROC)) {
                جمع_کړه_Flags(depVtx.nodep(), T_HAS_PROC);
                propagateFlagsReversedIf(&depVtx, T_HAS_PROC, [&](ثابت V3_ګراف_څنډه* e) -> منطقي {
                    أرجع hasFlags(static_cast<DepVtx*>(e->fromp())->nodep(), T_NEEDS_PROC);
                });
            }
        }
        إذا (dump_ګراف_Level() >= 6) m_proc_ګراف.dumpDotFilePrefixed("proc_deps");
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
    Ast_سر_Scope* ثابت m_سر_Scopep = m_netlistp->سر_Scopep();  // The AstTopScope
    AstScope* ثابت m_scopeTopp = m_سر_Scopep->scopep();  // Scope at the top
    AstClass* m_classp = تش_اشاره;  // Current class
    AstScope* m_scopep = تش_اشاره;  // Current scope
    Ast_فعال* m_activep = تش_اشاره;  // Current active
    Ast_غوټه* m_procp = تش_اشاره;  // NodeProcedure/CFunc/Begin we're under
    منطقي m_hasProcess = خطأ;  // True if current scope has a VlProcess handle available
    صحيح m_forkCnt = 0;  // Number of forks inside a module
    منطقي m_under_Jump_بلاک = خطأ;  // True if we are inside of a jump-block
    منطقي m_underProcedure = خطأ;  // True if we are under an always or initial
    منطقي m_underIfaceCFunc = خطأ;  // True if we are under a CFunc owned by an interface scope
    منطقي m_hasStaticZeroDelay = خطأ;  // True if we have a static #0 delay
    std::ویکټور<File_کرښه*> m_unknownDelayFlps;  // Locations of AstDelay with non-constant value

    // Unique names
    V3_Unique_نومونه m_dlyfork_نومونه{"__Vdlyfork"};  // Names for temp AssignW vars
    V3_Unique_نومونه m_intra_ارزښت_نومونه{"__Vintraval"};  // Intra assign delay value var names
    V3_Unique_نومونه m_intra_شاخص_نومونه{"__Vintraidx"};  // Intra assign delay index var names
    V3_Unique_نومونه m_intra_Lsb_نومونه{"__Vintralsb"};  // Intra assign delay LSB var names
    V3_Unique_نومونه m_trig_Sched_نومونه{"__VtrigSched"};  // Trigger scheduler name generator
    V3_Unique_نومونه m_dyn_Trig_نومونه{"__VdynTrigger"};  // Dynamic trigger name generator
    // Module level temporary variables, shared by instances
    V3SharedTmps m_ټاکل_WTmps{"__VassignWtmp", VVarType::MODULETEMP};  // Delayed AssignW values
    V3SharedTmps m_ټاکل_WGens{"__VassignWgen", VVarType::MODULETEMP};  // AssignW generations

    // DTypes
    AstBasicDType* m_forkDtp = تش_اشاره;  // Fork variable type
    AstBasicDType* m_trigSchedDtp = تش_اشاره;  // Trigger scheduler type

    // Timing-related globals
    AstVarScope* m_delaySchedp = تش_اشاره;  // Global delay scheduler
    AstVarScope* m_dynamicSchedp = تش_اشاره;  // Global dynamic trigger scheduler
    Ast_Sen_ونه* m_delaySensesp = تش_اشاره;  // Domain to trigger if a delayed coroutine is resumed
    Ast_Sen_ونه* m_dynamicSensesp = تش_اشاره;  // Domain to trigger if a dynamic trigger is set

    // Other
    Sen_ونه_Finder m_finder{m_netlistp};  // Sentree finder and uniquifier
    Sen_بیان_Builder* m_sen_بیان_Builderp = تش_اشاره;  // Sens expression builder for current m_scope

    // Stats
    اندازه_t m_statZeroDelays = 0;  // Number of statically known #0 delays
    اندازه_t m_statConstDelays = 0;  // Number of statically known #const (non-zero) delays
    اندازه_t m_statVariableDelays = 0;  // Number of delays with value unknown at compile time

    // METHODS
    // Transform an assignment with an intra timing control into a timing control with the
    // assignment under it
    Ast_غوټه* ضریب_بهر_Timing_Control(Ast_غوټه_ټاکل* nodep) ثابت {
        Ast_غوټه* stmtp = nodep;
        Ast_غوټه* ثابت controlp = nodep->timingControlp();
        إذا (AstDelay* ثابت delayp = VN_CAST(controlp, Delay)) {
            stmtp->replaceWith(delayp->unlink_Fr_شا());
            delayp->جمع_کړه_Stmtsp(stmtp);
            stmtp = delayp;
        } وإلا إذا (Ast_Sen_ونه* ثابت sentreep = VN_CAST(controlp, Sen_ونه)) {
            AstEventControl* ثابت eventControlp
                = جديد AstEventControl{sentreep->fileline(), sentreep->unlink_Fr_شا(), تش_اشاره};
            stmtp->replaceWith(eventControlp);
            eventControlp->جمع_کړه_Stmtsp(stmtp);
            stmtp = eventControlp;
        } وإلا إذا (AstBegin* ثابت beginp = VN_CAST(controlp, Begin)) {
            // Begin from V3AssertPre
            stmtp->replaceWith(beginp->unlink_Fr_شا());
            beginp->جمع_کړه_Stmtsp(stmtp);
            stmtp = beginp;
        }
        أرجع stmtp == nodep ? تش_اشاره : stmtp;
    }
    ساكن منطقي staticallyNonZeroDelay(ثابت Ast_غوټه_بیان* valuep) {
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
        خپلکاری* ثابت dlySchedDtp = جديد AstBasicDType{
            m_scopeTopp->fileline(), VBasicDTypeKwd::DELAY_SCHEDULER, VSigning::UNSIGNED};
        m_netlistp->typeTablep()->جمع_کړه_Typesp(dlySchedDtp);
        m_delaySchedp = m_سر_Scopep->create_لنډمهاله("__VdlySched", dlySchedDtp);
        // Delay scheduler has to be accessible from top
        m_delaySchedp->varp()->sig_عامه(ریښتیا);
        m_netlistp->delaySchedulerp(m_delaySchedp->varp());
        أرجع m_delaySchedp;
    }
    // Creates the delay sentree
    Ast_Sen_ونه* get_Create_Delay_Sen_ونه() {
        إذا (m_delaySensesp) أرجع m_delaySensesp;
        File_کرښه* ثابت flp = m_scopeTopp->fileline();
        خپلکاری* ثابت awaitingCurrentTimep
            = جديد AstCMethodHard{flp, جديد AstVarRef{flp, getCreateDelayScheduler(), VAccess::READ},
                                 VCMethod::SCHED_AWAITING_CURRENT_وخت};
        awaitingCurrentTimep->dtype_سېټ_بټ();
        m_delaySensesp
            = جديد Ast_Sen_ونه{flp, جديد Ast_Sen_توکی{flp, VEdgeType::ET_TRUE, awaitingCurrentTimep}};
        m_netlistp->سر_Scopep()->جمع_کړه_Sen_Treesp(m_delaySensesp);
        أرجع m_delaySensesp;
    }
    // Creates the global dynamic trigger scheduler variable
    AstVarScope* getCreateDynamicTriggerScheduler() {
        إذا (m_dynamicSchedp) أرجع m_dynamicSchedp;
        خپلکاری* ثابت dynSchedDtp
            = جديد AstBasicDType{m_scopeTopp->fileline(), VBasicDTypeKwd::DYNAMIC_TRIGGER_SCHEDULER,
                                VSigning::UNSIGNED};
        m_netlistp->typeTablep()->جمع_کړه_Typesp(dynSchedDtp);
        m_dynamicSchedp = m_سر_Scopep->create_لنډمهاله("__VdynSched", dynSchedDtp);
        أرجع m_dynamicSchedp;
    }
    // Creates the dynamic trigger sentree
    Ast_Sen_ونه* get_Create_Dynamic_Trigger_Sen_ونه() {
        إذا (m_dynamicSensesp) أرجع m_dynamicSensesp;
        File_کرښه* ثابت flp = m_scopeTopp->fileline();
        خپلکاری* ثابت awaitingCurrentTimep = جديد AstCMethodHard{
            flp, جديد AstVarRef{flp, getCreateDynamicTriggerScheduler(), VAccess::READ},
            VCMethod::SCHED_EVALUATE};
        awaitingCurrentTimep->dtype_سېټ_بټ();
        m_dynamicSensesp
            = جديد Ast_Sen_ونه{flp, جديد Ast_Sen_توکی{flp, VEdgeType::ET_TRUE, awaitingCurrentTimep}};
        m_netlistp->سر_Scopep()->جمع_کړه_Sen_Treesp(m_dynamicSensesp);
        أرجع m_dynamicSensesp;
    }
    // Creates the event variable to trigger in NBA region
    AstEventControl* createNbaEventControl(File_کرښه* flp) {
        إذا (!m_netlistp->nbaEventp()) {
            خپلکاری* ثابت nbaEventDtp = جديد AstBasicDType{m_scopeTopp->fileline(),
                                                        VBasicDTypeKwd::EVENT, VSigning::UNSIGNED};
            m_netlistp->typeTablep()->جمع_کړه_Typesp(nbaEventDtp);
            m_netlistp->nbaEventp(m_سر_Scopep->create_لنډمهاله("__VnbaEvent", nbaEventDtp));
            v3Global.سېټ_Has_Events();
        }
        أرجع جديد AstEventControl{
            flp,
            جديد Ast_Sen_ونه{
                flp, جديد Ast_Sen_توکی{flp, VEdgeType::ET_EVENT,
                                    جديد AstVarRef{flp, m_netlistp->nbaEventp(), VAccess::READ}}},
            تش_اشاره};
    }
    // Creates the variable that, if set, causes the NBA event to be triggered
    Ast_ټاکل* createNbaEventTriggerAssignment(File_کرښه* flp) {
        إذا (!m_netlistp->nbaEventTriggerp()) {
            m_netlistp->nbaEventTriggerp(m_سر_Scopep->create_لنډمهاله("__VnbaEventTrigger", 1));
        }
        أرجع جديد Ast_ټاکل{flp,
                             جديد AstVarRef{flp, m_netlistp->nbaEventTriggerp(), VAccess::WRITE},
                             جديد AstConst{flp, AstConst::بټ_True{}}};
    }
    // Returns true if we are under a class or interface function, or the tree references locals.
    // These are cases where static, globally-evaluated triggers are not suitable.
    منطقي needDynamicTrigger(Ast_غوټه* ثابت nodep) ثابت {
        أرجع m_classp || m_underIfaceCFunc || nodep->exists([](Ast_غوټه* ثابت nodep) {
            إذا (Ast_غوټه_Var_Ref* varp = VN_CAST(nodep, غوټه_Var_Ref)) {
                أرجع varp->varp()->isFuncLocal();
            }
            أرجع !nodep->is_خالص();
        });
    }
    // Creates a trigger scheduler variable
    AstVarScope* getCreateTriggerSchedulerp(Ast_Sen_ونه* ثابت sentreep) {
        إذا (!sentreep->user1p()) {
            إذا (!m_trigSchedDtp) {
                m_trigSchedDtp
                    = جديد AstBasicDType{m_scopeTopp->fileline(), VBasicDTypeKwd::TRIGGER_SCHEDULER,
                                        VSigning::UNSIGNED};
                m_netlistp->typeTablep()->جمع_کړه_Typesp(m_trigSchedDtp);
            }
            AstVarScope* ثابت trigSchedp
                = m_سر_Scopep->create_لنډمهاله(m_trig_Sched_نومونه.get(sentreep), m_trigSchedDtp);
            sentreep->user1p(trigSchedp);
        }
        أرجع VN_AS(sentreep->user1p(), VarScope);
    }
    // Creates a string describing the sentree
    AstCExpr* createEventDescription(Ast_Sen_ونه* ثابت sentreep) ثابت {
        إذا (!sentreep->user2p()) {
            std::stringstream ss;
            V3EmitV::verilog_For_ونه(sentreep, ss);
            // possibly a multiline string
            // Names were hashed by V3Name, show the user the original text
            std::string comment = VName::dehash(ss.str());
            std::replace(comment.begin(), comment.پای_ټکی(), '\n', ' ');
            // Escape quotes, e.g. from string-indexed event array selects
            comment = '"' + V3_بهر_Formatter::quoteNameControls(comment) + '"';
            AstCExpr* ثابت commentp = جديد AstCExpr{sentreep->fileline(), comment};
            commentp->dtype_سېټ_String();
            sentreep->user2p(commentp);
            أرجع commentp;
        }
        أرجع VN_AS(sentreep->user2p(), CExpr)->clone_ونه(خطأ);
    }
    // Adds debug info to a hardcoded method call
    فراغ جمع_کړه_Debug_Info(AstCMethodHard* ثابت methodp) ثابت {
        File_کرښه* ثابت flp = methodp->fileline();
        ثابت منطقي protectIds = v3Global.opt.protectIds();
        AstCExpr* ثابت ap
            = جديد AstCExpr{flp, protectIds ? "VL_UNKNOWN" : '"' + flp->filenameEsc() + '"'};
        ap->dtype_سېټ_String();
        methodp->جمع_کړه_Pinsp(ap);
        AstCExpr* ثابت bp = جديد AstCExpr{flp, protectIds ? "0" : cvt_منزل_Str(flp->lineno())};
        bp->dtype_سېټ_String();
        methodp->جمع_کړه_Pinsp(bp);
    }
    // Adds debug info to a trigSched.trigger() call
    فراغ جمع_کړه_Event_Debug_Info(AstCMethodHard* ثابت methodp, Ast_Sen_ونه* ثابت sentreep) ثابت {
        إذا (v3Global.opt.protectIds()) {
            File_کرښه* ثابت flp = sentreep->fileline();
            AstCExpr* ثابت descp = جديد AstCExpr{flp, "VL_UNKNOWN"};
            descp->dtype_سېټ_String();
            methodp->جمع_کړه_Pinsp(descp);
            جمع_کړه_Debug_Info(methodp);
            أرجع;
        }
        methodp->جمع_کړه_Pinsp(createEventDescription(sentreep));
        جمع_کړه_Debug_Info(methodp);
    }
    // Adds process pointer to a hardcoded method call
    فراغ جمع_کړه_Process_Info(AstCMethodHard* ثابت methodp) ثابت {
        File_کرښه* ثابت flp = methodp->fileline();
        AstCExpr* ثابت ap
            = جديد AstCExpr{flp, (m_procp && m_hasProcess) ? "vlProcess" : "nullptr"};
        methodp->جمع_کړه_Pinsp(ap);
    }
    // Creates the fork handle type and returns it
    AstBasicDType* getCreateForkSyncDTypep() {
        إذا (m_forkDtp) أرجع m_forkDtp;
        m_forkDtp = جديد AstBasicDType{m_scopeTopp->fileline(), VBasicDTypeKwd::FORK_SYNC,
                                      VSigning::UNSIGNED};
        m_netlistp->typeTablep()->جمع_کړه_Typesp(m_forkDtp);
        أرجع m_forkDtp;
    }
    // Move `nodep` into `AstCLocalScope`.
    AstCLocalScope* جمع_کړه_CLocal_Scope(File_کرښه* ثابت flp, Ast_غوټه* ثابت nodep) ثابت {
        إذا (!nodep) أرجع تش_اشاره;
        VNRelinker handle;
        nodep->unlink_Fr_شا(&handle);
        AstCLocalScope* ثابت cscopep = جديد AstCLocalScope{flp, nodep};
        handle.relink(cscopep);
        أرجع cscopep;
    }
    // Create a temp variable and optionally put it before the specified node (mark local if so)
    AstVarScope* create_لنډمهاله(File_کرښه* ثابت flp, ثابت std::string& name,
                            Ast_غوټه_DType* ثابت dtypep, Ast_غوټه* ثابت ورننباسه_Beforep = تش_اشاره) {
        AstVar* varp;
        إذا (ورننباسه_Beforep) {
            varp = جديد AstVar{flp, VVarType::BLOCKTEMP, name, dtypep};
            varp->funcLocal(ریښتیا);
            ورننباسه_Beforep->جمع_کړه_دلته_This_As_بل(varp);
        } وإلا {
            varp = جديد AstVar{flp, VVarType::MODULETEMP, name, dtypep};
            m_scopep->modp()->جمع_کړه_Stmtsp(varp);
        }
        AstVarScope* ثابت vscp = جديد AstVarScope{flp, m_scopep, varp};
        m_scopep->جمع_کړه_Varsp(vscp);
        أرجع vscp;
    }
    // Add a done() call on the fork sync
    فراغ جمع_کړه_Fork_بشپړ(AstBegin* ثابت beginp, AstVarScope* ثابت forkVscp) ثابت {
        File_کرښه* ثابت flp = beginp->fileline();
        خپلکاری* ثابت donep = جديد AstCMethodHard{
            beginp->fileline(), جديد AstVarRef{flp, forkVscp, VAccess::WRITE}, VCMethod::FORK_بشپړ};
        donep->dtype_سېټ_Void();
        جمع_کړه_Debug_Info(donep);
        beginp->جمع_کړه_Stmtsp(donep->makeStmt());
    }
    ساكن منطقي has_Disable_کتار_ټېل_Self_مختاړی(AstBegin* ثابت beginp) {
        // LinkJump prepends disable-by-name registration as:
        //   __VprocessQueue_*.push_back(std::process::self())
        // V3LiftExpr lifts std::process::self() into an assignment to a temporary, then V3Task
        // lowers that assignment's function call into a leading comment and AstStmtExpr. Thus the
        // push_back is no longer necessarily the first statement. Scan across this generated
        // prefix for it, stopping at the fork-start sentinel or the branch body. Registrations for
        // nested forks live in sub-blocks and so are not in this statement list.
        لكل (Ast_غوټه* stmtp = beginp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            إذا (VN_IS(stmtp, Comment)) استمر;
            Ast_Stmt_بیان* ثابت stmtExprp = VN_CAST(stmtp, Stmt_بیان);
            إذا (!stmtExprp) اكسر;
            إذا (stmtExprp->is_Disable_کتار_ټېل_Self_Stmt()) أرجع ریښتیا;
        }
        أرجع خطأ;
    }
    // Register a callback so killing a process-backed fork branch decrements the join counter
    فراغ جمع_کړه_Fork_On_Kill(AstBegin* ثابت beginp, AstVarScope* ثابت forkVscp) ثابت {
        إذا (!beginp->needProcess() && !has_Disable_کتار_ټېل_Self_مختاړی(beginp)) أرجع;
        File_کرښه* ثابت flp = beginp->fileline();
        AstCMethodHard* ثابت onKillp = جديد AstCMethodHard{
            flp, جديد AstVarRef{flp, forkVscp, VAccess::WRITE}, VCMethod::FORK_ON_KILL};
        onKillp->dtype_سېټ_Void();
        AstCExpr* ثابت processp = جديد AstCExpr{flp, "vlProcess"};
        processp
            ->dtype_سېټ_Void();  // Opaque process reference; type is irrelevant for hardcoded emit
        onKillp->جمع_کړه_Pinsp(processp);
        Ast_غوټه_Stmt* ثابت stmtp = onKillp->makeStmt();
        beginp->prependStmtsp(stmtp);
    }
    // Handle the 'join' part of a fork..join
    فراغ makeForkJoin(AstFork* ثابت forkp) {
        // Create a fork sync var
        File_کرښه* ثابت flp = forkp->fileline();
        // Insert the sync var directly before the fork
        Ast_غوټه* ثابت ورننباسه_Beforep = forkp;
        // Make sure all references to the fork var are contained within a single scope to prevent
        // moving statements accessing it in case of large functions splitting.
        جمع_کړه_CLocal_Scope(flp, ورننباسه_Beforep);
        AstVarScope* forkVscp
            = create_لنډمهاله(flp, forkp->name() + "__sync", getCreateForkSyncDTypep(), ورننباسه_Beforep);
        بینښه join_شمېر = 0;  // Needed for join counter
        // Add a <fork sync>.done() to each begin
        لكل (Ast_غوټه* beginp = forkp->forksp(); beginp; beginp = beginp->nextp()) {
            جمع_کړه_Fork_On_Kill(VN_AS(beginp, Begin), forkVscp);
            جمع_کړه_Fork_بشپړ(VN_AS(beginp, Begin), forkVscp);
            join_شمېر++;
        }
        إذا (forkp->joinType().joinAny()) join_شمېر = 1;
        // Set the join counter
        خپلکاری* ثابت initp = جديد AstCMethodHard{flp, جديد AstVarRef{flp, forkVscp, VAccess::WRITE},
                                               VCMethod::FORK_INIT, جديد AstConst{flp, join_شمېر}};
        initp->dtype_سېټ_Void();
        جمع_کړه_Process_Info(initp);
        forkp->جمع_کړه_دلته_This_As_بل(initp->makeStmt());
        // Await the join at the end
        خپلکاری* ثابت joinp = جديد AstCMethodHard{flp, جديد AstVarRef{flp, forkVscp, VAccess::WRITE},
                                               VCMethod::FORK_JOIN};
        joinp->dtype_سېټ_Void();
        جمع_کړه_Process_Info(joinp);
        جمع_کړه_Debug_Info(joinp);
        forkp->جمع_کړه_بل_دلته(جديد AstCAwait{flp, joinp});
    }

    // `procp` shall be a NodeProcedure/CFunc/Begin and within it vars from `varsp` will be placed.
    // `varsp` vector of vars which shall be localized.
    ساكن فراغ localizeVars(Ast_غوټه* ثابت procp, ثابت std::ویکټور<AstVar*>& varsp) {
        UASSERT(procp, "procp is nullptr");
        AstVar* newVarsp = تش_اشاره;
        لكل (AstVar* ثابت varp : varsp) {
            varp->funcLocal(ریښتیا);
            newVarsp = Ast_غوټه::جمع_کړه_بل_Null(newVarsp, varp->unlink_Fr_شا());
        }
        إذا (Ast_غوټه_Procedure* ثابت غوټه_Procp = VN_CAST(procp, غوټه_Procedure)) {
            غوټه_Procp->prependStmtsp(newVarsp);
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

    Ast_غوټه_بیان* delay_منزل_Integral(AstDelay* nodep, Ast_غوټه_بیان* valuep) {
        ثابت AstConst* ثابت constp = VN_CAST(valuep, Const);
        // The fork start sentinel is already in ticks
        إذا (constp && constp->منزل_UQuad() == std::numeric_limits<uint64_t>::max()) أرجع valuep;
        أرجع V3Timing::delay_منزل_Ticks(valuep, nodep->timeunit());
    }

    // VISITORS
    فراغ لیدنه(Ast_غوټه_Module* nodep) override {
        UASSERT_OBJ(!m_classp, nodep, "Module or class under class");
        VL_RESTORER(m_classp);
        m_classp = VN_CAST(nodep, Class);
        VL_RESTORER(m_forkCnt);
        m_forkCnt = 0;
        iterateChildren(nodep);
    }
    فراغ لیدنه(AstScope* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        Sen_بیان_Builder sen_بیان_Builder{m_scopep};
        {  // Restore m_senExprBuilderp before destroying senExprBuilder
            VL_RESTORER(m_sen_بیان_Builderp);
            m_sen_بیان_Builderp = &sen_بیان_Builder;
            iterateChildren(nodep);
        }
    }
    فراغ لیدنه(Ast_فعال* nodep) override {
        VL_RESTORER(m_activep);
        m_activep = nodep;
        iterateChildren(nodep);
    }
    فراغ لیدنه(Ast_غوټه_Procedure* nodep) override {
        VL_RESTORER(m_procp);
        VL_RESTORER(m_hasProcess);
        m_procp = nodep;
        m_hasProcess = hasFlags(nodep, T_HAS_PROC);
        VL_RESTORER(m_underProcedure);
        m_underProcedure = ریښتیا;
        iterateChildren(nodep);
        إذا (hasFlags(nodep, T_SUSPENDEE)) nodep->سېټ_Suspendable();
        إذا (hasFlags(nodep, T_HAS_PROC)) nodep->سېټ_Need_Process();
    }
    فراغ لیدنه(AstInitial* nodep) override {
        لیدنه(static_cast<Ast_غوټه_Procedure*>(nodep));
        إذا (nodep->needProcess() && !nodep->user1_سېټ_Once()) {
            nodep->جمع_کړه_Stmtsp(
                جديد AstCStmt{nodep->fileline(), "vlProcess->state(VlProcess::FINISHED);"});
        }
    }
    فراغ لیدنه(Ast_Jump_بلاک* nodep) override {
        VL_RESTORER(m_under_Jump_بلاک);
        m_under_Jump_بلاک = ریښتیا;
        لیدنه(static_cast<Ast_غوټه_Stmt*>(nodep));
    }
    فراغ لیدنه(AstAlways* nodep) override {
        إذا (nodep->user1_سېټ_Once()) أرجع;
        VL_RESTORER(m_procp);
        VL_RESTORER(m_hasProcess);
        m_procp = nodep;
        m_hasProcess = hasFlags(nodep, T_HAS_PROC);
        VL_RESTORER(m_underProcedure);
        m_underProcedure = ریښتیا;
        // Workaround for killing `always` processes (doing that is pretty much UB)
        // TODO: Disallow killing `always` at runtime (throw an error)
        // Skip for combinational blocks; if the body has timing controls
        // iterateChildren will add T_SUSPENDEE, otherwise it would spin.
        إذا (hasFlags(nodep, T_HAS_PROC)
            && !(m_activep && m_activep->sentreep() && m_activep->sentreep()->hasCombo()))
            جمع_کړه_Flags(nodep, T_SUSPENDEE);

        iterateChildren(nodep);
        إذا (hasFlags(nodep, T_HAS_PROC)) nodep->سېټ_Need_Process();
        إذا (!hasFlags(nodep, T_SUSPENDEE)) أرجع;
        nodep->سېټ_Suspendable();
        File_کرښه* ثابت flp = nodep->fileline();
        Ast_Sen_ونه* ثابت sentreep = m_activep->sentreep();
        إذا (sentreep->hasClocked()) {
            Ast_غوټه* ثابت bodysp = nodep->stmtsp()->unlink_Fr_شا_With_بل();
            خپلکاری* ثابت controlp = جديد AstEventControl{flp, sentreep->clone_ونه(خطأ), bodysp};
            nodep->جمع_کړه_Stmtsp(controlp);
            iterate(controlp);
        }
        // Note: The 'while (true)' outer loop will be added in V3Sched
        خپلکاری* ثابت activep = جديد Ast_فعال{
            flp, "", جديد Ast_Sen_ونه{flp, جديد Ast_Sen_توکی{flp, Ast_Sen_توکی::Initial{}}}};
        activep->sen_ونه_Storep(activep->sentreep());
        activep->جمع_کړه_Stmtsp(nodep->unlink_Fr_شا());
        m_activep->جمع_کړه_بل_دلته(activep);
    }
    فراغ لیدنه(AstCFunc* nodep) override {
        VL_RESTORER(m_procp);
        VL_RESTORER(m_hasProcess);
        VL_RESTORER(m_underIfaceCFunc);
        m_procp = nodep;
        m_hasProcess = hasFlags(nodep, T_HAS_PROC);
        m_underIfaceCFunc = VN_IS(m_scopep->modp(), Iface);
        iterateChildren(nodep);
        إذا (hasFlags(nodep, T_HAS_PROC)) nodep->سېټ_Need_Process();
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
                أرجع ریښتیا;
            });
        }
    }
    فراغ لیدنه(Ast_غوټه_CCall* nodep) override {
        AstCFunc* ثابت funcp = nodep->funcp();

        // Skip automatic covergroup sampling calls
        إذا (funcp->isCovergroupSample()) {
            iterateChildren(nodep);
            أرجع;
        }

        إذا (funcp->needProcess()) m_hasProcess = ریښتیا;
        إذا (hasFlags(funcp, T_SUSPENDEE) && !nodep->user1_سېټ_Once()) {  // If suspendable
            // Calls to suspendables are always void return type, hence parent must be StmtExpr
            Ast_Stmt_بیان* ثابت stmtp = VN_AS(nodep->backp(), Stmt_بیان);
            stmtp->replaceWith(جديد AstCAwait{nodep->fileline(), nodep->unlink_Fr_شا()});
            VL_DO_DANGLING(ټېل_Deletep(stmtp), stmtp);
        }
        iterateChildren(nodep);
    }
    فراغ لیدنه(AstDelay* nodep) override {
        UASSERT_OBJ(!nodep->is_دوره_Delay(), nodep,
                    "Cycle delays should have been handled in V3AssertPre");
        File_کرښه* ثابت flp = nodep->fileline();

        Ast_غوټه_بیان* valuep = delay_منزل_Integral(nodep, nodep->lhsp()->unlink_Fr_شا());

        // Statistics
        إذا (valuep->isZero()) {
            ++m_statZeroDelays;
        } وإلا إذا (VN_IS(valuep, Const)) {
            ++m_statConstDelays;
        } وإلا {
            ++m_statVariableDelays;
        }

        // Decide scheduling support for #0
        إذا (v3Global.opt.schedZeroDelay().is_سېټ_True()) {
            // User said to schedule for #0 support, nothing else to do
            v3Global.سېټ_کاروي_Zero_Delay();
        } وإلا إذا (v3Global.opt.schedZeroDelay().is_سېټ_False()) {
            // User said to schedule without #0 support. Still warn if a static #0 delay exists
            إذا (valuep->isZero()) {
                nodep->v3warn(
                    ZERODLY,
                    "Static #0 delay exists, but '--no-sched-zero-delay' was given.\n"
                        << nodep->warn_نور()  //
                        << "... Can proceed, but this will fail at runtime if executed.");
            }
        } وإلا {
            // User did not express preference, decide based on presence of delays
            إذا (valuep->isZero()) {
                // Statically known #0 delay exists, schedule for #0 support
                v3Global.سېټ_کاروي_Zero_Delay();
                m_hasStaticZeroDelay = ریښتیا;
                // Don't warn on variable delays, as no point
                m_unknownDelayFlps.clear();
            } وإلا إذا (staticallyNonZeroDelay(valuep)) {
                // Delay is dynamic, but every statically known outcome is non-zero.
                // So we don't need #0 delay support and there should be no warning.
            } وإلا إذا (!VN_IS(valuep, Const)) {
                // Delay is not known at compiile time. Conservatively schedule for #0 support,
                // but warn if no static #0 delays used as performance might be improved
                v3Global.سېټ_کاروي_Zero_Delay();
                إذا (!m_hasStaticZeroDelay) m_unknownDelayFlps.ټېل_شا(nodep->fileline());
            }
        }

        // Replace self with a 'co_await dlySched.delay(<valuep>)'
        AstCMethodHard* ثابت delayMethodp = جديد AstCMethodHard{
            flp, جديد AstVarRef{flp, getCreateDelayScheduler(), VAccess::WRITE},
            VCMethod::SCHED_DELAY, valuep};
        delayMethodp->dtype_سېټ_Void();
        جمع_کړه_Process_Info(delayMethodp);
        جمع_کړه_Debug_Info(delayMethodp);
        // Create the co_await
        Ast_غوټه* ثابت awaitp = جديد AstCAwait{flp, delayMethodp, get_Create_Delay_Sen_ونه()};
        // Relink child statements after the co_await
        إذا (Ast_غوټه* ثابت stmtsp = nodep->stmtsp()) {
            awaitp->جمع_کړه_بل(stmtsp->unlink_Fr_شا_With_بل());
        }
        nodep->replaceWith(awaitp);
        VL_DO_DANGLING(nodep->delete_ونه(), nodep);
    }
    فراغ لیدنه(AstEventControl* nodep) override {
        // Do not allow waiting on local named events, as they get enqueued for clearing, but can
        // go out of scope before that happens
        إذا (!nodep->sentreep()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Event control with implicit sensitivity (@*)"
                          " in this context; use an explicit sensitivity list instead");
            VL_DO_DANGLING(ټېل_Deletep(nodep->unlink_Fr_شا()), nodep);
            أرجع;
        }
        File_کرښه* ثابت flp = nodep->fileline();
        // Relink child statements after the event control
        إذا (nodep->stmtsp()) nodep->جمع_کړه_بل_دلته(nodep->stmtsp()->unlink_Fr_شا_With_بل());
        إذا (needDynamicTrigger(nodep->sentreep())) {
            // Create the trigger variable and init it with 0
            AstVarScope* ثابت trigvscp
                = create_لنډمهاله(flp, m_dyn_Trig_نومونه.get(nodep), nodep->پیدا_کړه_بټ_DType(), nodep);
            خپلکاری* ثابت initp = جديد Ast_ټاکل{flp, جديد AstVarRef{flp, trigvscp, VAccess::WRITE},
                                              جديد AstConst{flp, AstConst::بټ_False{}}};
            // Await the eval step with the dynamic trigger scheduler. First, create the method
            // call
            خپلکاری* ثابت evalMethodp = جديد AstCMethodHard{
                flp, جديد AstVarRef{flp, getCreateDynamicTriggerScheduler(), VAccess::WRITE},
                VCMethod::SCHED_EVALUATION};
            evalMethodp->dtype_سېټ_Void();
            جمع_کړه_Process_Info(evalMethodp);
            خپلکاری* ثابت sentreep = nodep->sentreep();
            جمع_کړه_Event_Debug_Info(evalMethodp, sentreep);
            // Create the co_await
            AstCAwait* ثابت awaitEvalp
                = جديد AstCAwait{flp, evalMethodp, get_Create_Dynamic_Trigger_Sen_ونه()};
            // Construct the sen expression for this sentree
            UASSERT_OBJ(m_sen_بیان_Builderp, nodep, "No SenExprBuilder for this scope");
            خپلکاری* ثابت assignp = جديد Ast_ټاکل{flp, جديد AstVarRef{flp, trigvscp, VAccess::WRITE},
                                                m_sen_بیان_Builderp->build(sentreep).لومړی};
            // Get the SenExprBuilder results
            ثابت Sen_بیان_Builder::Results senResults = m_sen_بیان_Builderp->getAndClearResults();
            localizeVars(m_procp, senResults.m_vars);
            // If post updates are destructive (e.g. clearFired on events), perform a
            // conservative pre-clear once before entering the wait loop so stale state from a
            // previous wait does not cause an immediate false-positive trigger.
            ثابت منطقي hasDestructivePostUpdates = !senResults.m_destructivePostUpdates.تشه();
            // Create the trigger eval loop, which will await the evaluation step and check the
            // trigger
            Ast_غوټه_بیان* ثابت condp
                = جديد AstLogNot{flp, جديد AstVarRef{flp, trigvscp, VAccess::READ}};
            AstLoop* ثابت loopp = جديد AstLoop{flp};
            loopp->جمع_کړه_Stmtsp(جديد AstLoopTest{flp, loopp, condp});
            loopp->جمع_کړه_Stmtsp(awaitEvalp);
            // Put pre updates before the trigger check and assignment
            لكل (Ast_غوټه_Stmt* ثابت stmtp : senResults.m_preUpdates) loopp->جمع_کړه_Stmtsp(stmtp);
            // Then the trigger check and assignment
            loopp->جمع_کړه_Stmtsp(assignp);
            // Let the dynamic trigger scheduler know if this trigger was set
            // If it was, a call to the scheduler's evaluate() will return true
            AstCMethodHard* ثابت anyTriggeredMethodp = جديد AstCMethodHard{
                flp, جديد AstVarRef{flp, getCreateDynamicTriggerScheduler(), VAccess::WRITE},
                VCMethod::SCHED_ANY_TRIGGERED, جديد AstVarRef{flp, trigvscp, VAccess::READ}};
            anyTriggeredMethodp->dtype_سېټ_Void();
            loopp->جمع_کړه_Stmtsp(anyTriggeredMethodp->makeStmt());
            // If the post update is destructive (e.g. event vars are cleared), create an await for
            // the post update step
            إذا (hasDestructivePostUpdates) {
                AstCAwait* ثابت awaitPostUpdatep = awaitEvalp->clone_ونه(خطأ);
                VN_AS(awaitPostUpdatep->exprp(), CMethodHard)->لاره(VCMethod::SCHED_POST_UPDATE);
                loopp->جمع_کړه_Stmtsp(awaitPostUpdatep);
            }
            // Put the post updates at the end of the loop
            لكل (Ast_غوټه_Stmt* ثابت stmtp : senResults.m_postUpdates) loopp->جمع_کړه_Stmtsp(stmtp);
            // Finally, await the resumption step in 'act'
            AstCAwait* ثابت awaitResumep = awaitEvalp->clone_ونه(خطأ);
            VN_AS(awaitResumep->exprp(), CMethodHard)->لاره(VCMethod::SCHED_RESUMPTION);
            Ast_غوټه::جمع_کړه_بل<Ast_غوټه_Stmt, Ast_غوټه_Stmt>(loopp, awaitResumep);
            // Replace the event control with one explicit stmt chain:
            //   init -> [inits] -> [optional destructive pre-clears] -> loop -> awaitResumption
            Ast_غوټه_Stmt* chainp = تش_اشاره;
            chainp = Ast_غوټه::جمع_کړه_بل_Null(chainp, initp);
            لكل (Ast_غوټه_Stmt* ثابت stmtp : senResults.m_inits) {
                chainp = Ast_غوټه::جمع_کړه_بل_Null(chainp, stmtp);
            }
            إذا (hasDestructivePostUpdates) {
                لكل (Ast_غوټه_Stmt* ثابت stmtp : senResults.m_destructivePostUpdates) {
                    chainp = Ast_غوټه::جمع_کړه_بل_Null(chainp, stmtp->clone_ونه(خطأ));
                }
            }
            chainp = Ast_غوټه::جمع_کړه_بل_Null(chainp, loopp);
            nodep->replaceWith(chainp);
        } وإلا {
            خپلکاری* ثابت sentreep = m_finder.get_Sen_ونه(nodep->sentreep());
            nodep->sentreep()->unlink_Fr_شا()->delete_ونه();
            // Get this sentree's trigger scheduler
            // Replace self with a 'co_await trigSched.trigger()'
            خپلکاری* ثابت triggerMethodp = جديد AstCMethodHard{
                flp, جديد AstVarRef{flp, getCreateTriggerSchedulerp(sentreep), VAccess::WRITE},
                VCMethod::SCHED_TRIGGER};
            triggerMethodp->dtype_سېټ_Void();
            // If it should be committed immediately, pass true, otherwise false
            triggerMethodp->جمع_کړه_Pinsp(nodep->user2() ? جديد AstConst{flp, AstConst::بټ_True{}}
                                                    : جديد AstConst{flp, AstConst::بټ_False{}});
            جمع_کړه_Process_Info(triggerMethodp);
            جمع_کړه_Event_Debug_Info(triggerMethodp, sentreep);
            // Create the co_await
            AstCAwait* ثابت awaitp = جديد AstCAwait{flp, triggerMethodp, sentreep};
            nodep->replaceWith(awaitp);
        }
        VL_DO_DANGLING(nodep->delete_ونه(), nodep);
    }
    فراغ لیدنه(Ast_غوټه_ټاکل* nodep) override {
        // Only process once to avoid infinite loops (due to the net delay)
        إذا (nodep->user1_سېټ_Once()) أرجع;
        UINFO(9, "control-visit " << nodep);
        File_کرښه* ثابت flp = nodep->fileline();
        Ast_غوټه* controlp = ضریب_بهر_Timing_Control(nodep);
        ثابت منطقي دننه_ټاکل_Dly = VN_IS(nodep, ټاکل_Dly);
        // Handle the intra assignment timing control
        // Transform if:
        // * there's a timing control in the assignment
        // * the assignment is an AssignDly and it's in a non-inlined function
        إذا (!controlp && (!دننه_ټاکل_Dly || m_underProcedure)) {
            iterateChildren(nodep);
            أرجع;
        }
        // The timing control, if any, which is put in a fork for an NBA
        Ast_غوټه* ثابت timingp = controlp;
        // Insert new vars before the timing control if we're in a function; in a process we can't
        // do that, as after V3Begin only modules and functions declare variables. These
        // intra-assignment vars will later be passed to forked processes by value.
        Ast_غوټه* ورننباسه_Beforep = m_underProcedure ? تش_اشاره : controlp;
        // Special case for NBA
        إذا (دننه_ټاکل_Dly) {
            // Put it in a fork so it doesn't block
            // Could already be the only thing directly under a fork, reuse that if possible
            AstFork* forkp = !nodep->nextp() ? VN_CAST(nodep->لومړی_Abovep(), Fork) : تش_اشاره;
            إذا (!forkp) forkp = جديد AstFork{flp, VJoinType::JOIN_NONE};
            إذا (!m_underProcedure) {
                // If it's in a function, it won't be handled by V3Delayed
                // Put it behind an additional named event that gets triggered in the NBA region
                AstEventControl* ثابت nbaEventControlp = createNbaEventControl(flp);
                Ast_ټاکل* ثابت trigAssignp = createNbaEventTriggerAssignment(flp);
                nodep->replaceWith(trigAssignp);
                trigAssignp->جمع_کړه_بل_دلته(nbaEventControlp);
                nbaEventControlp->جمع_کړه_Stmtsp(nodep);
                ورننباسه_Beforep = forkp;
                إذا (!controlp) controlp = nbaEventControlp;
            }
            controlp->replaceWith(forkp);
            AstBegin* beginp = VN_CAST(controlp, Begin);
            إذا (!beginp) beginp = جديد AstBegin{nodep->fileline(), "", controlp, خطأ};
            forkp->جمع_کړه_Forksp(beginp);
            جمع_کړه_Flags(beginp, T_NBA_UPDATE);
            controlp = forkp;
        }
        UASSERT_OBJ(nodep, controlp, "Assignment should have timing control");
        // Move `insertBeforep` into `AstCLocalScope` if necessary to avoid jumping over
        // a variable initialization that would be inserted before `insertBeforep`. All
        // access to this variable should be contained within returned `AstCLocalScope`.
        إذا (m_under_Jump_بلاک) جمع_کړه_CLocal_Scope(flp, ورننباسه_Beforep);
        // Function for replacing values with intermediate variables
        ثابت خپلکاری replaceWithIntermediate = [&](Ast_غوټه_بیان* ثابت valuep,
                                                 ثابت std::string& name) {
            AstVarScope* ثابت newvscp = create_لنډمهاله(flp, name, valuep->dtypep(), ورننباسه_Beforep);
            // A pending update keeps its own value, even if its process schedules more
            إذا (دننه_ټاکل_Dly && !ورننباسه_Beforep) newvscp->fork_By_ارزښت(ریښتیا);
            valuep->replaceWith(جديد AstVarRef{flp, newvscp, VAccess::READ});
            controlp->جمع_کړه_دلته_This_As_بل(
                جديد Ast_ټاکل{flp, جديد AstVarRef{flp, newvscp, VAccess::WRITE}, valuep});
        };
        // NBAs with delays evaluate the indices selecting the target now (IEEE 1800-2023 10.4.2),
        // in order, each as a whole, also the selects and function calls in it. So follow the
        // selects of the target up from the variable, not into their indices. As an example, this
        // transforms 'x[a[i]][f(j)] <= #1 y' into 't0 = a[i]; t1 = f(j); x[t0][t1] <= #1 y'.
        // V3Const has split NBAs to concatenations, also streamed, into NBAs to each part.
        إذا (دننه_ټاکل_Dly) {
            لكل (Ast_غوټه* exprp = nodep->lhsp()->بنسټ_Fromp(ریښتیا); exprp != nodep;
                 exprp = exprp->لومړی_Abovep()) {
                إذا (AstSel* ثابت selp = VN_CAST(exprp, Sel)) {
                    // widthp should be const
                    إذا (!VN_IS(selp->lsbp(), Const)) {
                        replaceWithIntermediate(selp->lsbp(), m_intra_Lsb_نومونه.get(nodep));
                    }
                } وإلا إذا (Ast_غوټه_Sel* ثابت selp = VN_CAST(exprp, غوټه_Sel)) {
                    إذا (!VN_IS(selp->bitp(), Const)) {
                        replaceWithIntermediate(selp->bitp(), m_intra_شاخص_نومونه.get(nodep));
                    }
                }
            }
        }
        // Replace the RHS with an intermediate value var
        replaceWithIntermediate(nodep->rhsp(), m_intra_ارزښت_نومونه.get(nodep));
        إذا (!دننه_ټاکل_Dly) أرجع;
        // The process executing the NBA evaluates its delay (IEEE 1800-2023 4.9.4), not the forked
        // process, if it has side effects, e.g. calling a function that checks its process. A pure
        // one is evaluated as the forked process starts, at once.
        إذا (AstDelay* ثابت delayp = VN_CAST(timingp, Delay)) {
            إذا (!delayp->lhsp()->is_خالص()) {
                replaceWithIntermediate(delayp->lhsp(), m_intra_ارزښت_نومونه.get(nodep));
            }
        }
    }
    فراغ لیدنه(AstFireEvent* nodep) override {
        // V3Delayed handles '->>' of a variable in a process. Like NBAs in non-inlined functions,
        // trigger other events from a fork awaiting the NBA region.
        إذا (!nodep->isDelayed() || (m_underProcedure && VN_IS(nodep->operandp(), VarRef))) {
            iterateChildren(nodep);
            أرجع;
        }
        File_کرښه* ثابت flp = nodep->fileline();
        Ast_ټاکل* ثابت trigAssignp = createNbaEventTriggerAssignment(flp);
        nodep->replaceWith(trigAssignp);
        AstFork* ثابت forkp = جديد AstFork{flp, VJoinType::JOIN_NONE};
        trigAssignp->جمع_کړه_بل_دلته(forkp);
        إذا (m_under_Jump_بلاک) جمع_کړه_CLocal_Scope(flp, forkp);
        // The triggered event is the one referenced now, so evaluate handles and indices now
        Ast_غوټه_بیان* ثابت eventp = nodep->operandp()->unlink_Fr_شا();
        ثابت خپلکاری evalNow = [&](Ast_غوټه_بیان* ثابت valuep, V3_Unique_نومونه& نومونه) {
            // Like for an NBA, a process uses a module variable, passed to the fork by value
            AstVarScope* ثابت vscp = create_لنډمهاله(flp, نومونه.get(nodep), valuep->dtypep(),
                                                 m_underProcedure ? تش_اشاره : forkp);
            إذا (m_underProcedure) vscp->fork_By_ارزښت(ریښتیا);
            valuep->replaceWith(جديد AstVarRef{flp, vscp, VAccess::READ});
            // Unlike an event, a handle selecting one is only read
            valuep->foreach([](Ast_غوټه* ثابت np) {
                إذا (Ast_غوټه_Var_Ref* ثابت refp = VN_CAST(np, غوټه_Var_Ref)) {
                    refp->access(VAccess::READ);
                } وإلا إذا (AstMemberSel* ثابت selp = VN_CAST(np, MemberSel)) {
                    selp->access(VAccess::READ);
                }
            });
            forkp->جمع_کړه_دلته_This_As_بل(
                جديد Ast_ټاکل{flp, جديد AstVarRef{flp, vscp, VAccess::WRITE}, valuep});
        };
        Ast_غوټه_بیان* refp = eventp;
        طالما (Ast_غوټه_Sel* ثابت selp = VN_CAST(refp, غوټه_Sel)) {
            إذا (!VN_IS(selp->bitp(), Const)) evalNow(selp->bitp(), m_intra_شاخص_نومونه);
            refp = selp->fromp();
        }
        // Handle of a class (or virtual interface)
        إذا (AstMemberSel* ثابت selp = VN_CAST(refp, MemberSel)) {
            evalNow(selp->fromp(), m_intra_ارزښت_نومونه);
        }
        AstEventControl* ثابت controlp = createNbaEventControl(flp);
        controlp->جمع_کړه_Stmtsp(جديد AstFireEvent{flp, eventp, خطأ});
        AstBegin* ثابت beginp = جديد AstBegin{flp, "", controlp, خطأ};
        جمع_کړه_Flags(beginp, T_NBA_UPDATE);
        forkp->جمع_کړه_Forksp(beginp);
        VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
    }
    فراغ لیدنه(Ast_ټاکل_W* nodep) override {
        File_کرښه* ثابت flp = nodep->fileline();
        // Get the net delay unless this assignment was created for handling the net delay (user1)
        AstDelay* ثابت netDelayp = nodep->user1() ? تش_اشاره : nodep->getLhsNetDelay();
        إذا (netDelayp) {
            إذا (nodep->timingControlp()) {
                // If this assignment has a delay, create another one to handle the net delay
                AstVarScope* ثابت newvscp = m_ټاکل_WTmps.make(flp, m_scopep, nodep->dtypep());
                Ast_ټاکل_W* assignp = جديد Ast_ټاکل_W{
                    nodep->fileline(), nodep->lhsp()->unlink_Fr_شا(),
                    جديد AstVarRef{flp, newvscp, VAccess::READ}, netDelayp->clone_ونه(خطأ)};
                assignp->user1(ریښتیا);
                nodep->جمع_کړه_بل_دلته(assignp);
                nodep->lhsp(جديد AstVarRef{flp, newvscp, VAccess::WRITE});
            } وإلا {
                // Else just use this one with the net delay
                nodep->timingControlp(netDelayp->clone_ونه(خطأ));
            }
        }
        إذا (!nodep->timingControlp()) أرجع;
        // There will be some circular logic here, suppress the warning for newly created vars
        flp->modifyWarnOff(V3_Error_کوډ::UNOPTFLAT, ریښتیا);
        // Also suppress the warning for the LHS var, for cases like `assign #1 clk = ~clk;`
        // TODO: Restore the warning for other, non-delayed drivers
        nodep->lhsp()->foreach([](AstVarRef* refp) {
            إذا (refp->access().isWriteOrRW()) {
                refp->varp()->fileline()->modifyWarnOff(V3_Error_کوډ::UNOPTFLAT, ریښتیا);
            }
        });
        // Should be under an always
        AstAlways* ثابت alwaysp = VN_AS(m_procp, Always);
        // Convert it to an Assign; the new assign with intra delay will be handled by
        // visit(AstNodeAssign*)
        Ast_غوټه_بیان* ثابت lhs1p = nodep->lhsp()->unlink_Fr_شا();
        Ast_غوټه_بیان* ثابت rhs1p = nodep->rhsp()->unlink_Fr_شا();
        Ast_غوټه* ثابت controlp = nodep->timingControlp()->unlink_Fr_شا();
        إذا (AstDelay* ثابت delayp = VN_CAST(controlp, Delay)) {
            إذا (Ast_غوټه_بیان* fallDelayp = delayp->fallDelay()) {
                fallDelayp = fallDelayp->unlink_Fr_شا();
                Ast_غوټه_بیان* lhsp = delayp->lhsp()->unlink_Fr_شا();
                // Use fall only for an all-zero value, rise otherwise.
                lhsp = delay_منزل_Integral(delayp, lhsp);
                fallDelayp = delay_منزل_Integral(delayp, fallDelayp);
                delayp->lhsp(
                    جديد AstCond{flp, جديد AstEq{flp, rhs1p->clone_ونه(خطأ), جديد AstConst{flp, 0}},
                                fallDelayp, lhsp});
            }
        }
        Ast_ټاکل* ثابت assignp = جديد Ast_ټاکل{nodep->fileline(), lhs1p, rhs1p, controlp};
        // Put the assignment in a fork..join_none.
        AstFork* ثابت forkp = جديد AstFork{flp, VJoinType::JOIN_NONE};
        nodep->replaceWith(forkp);
        VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
        AstBegin* ثابت beginp = جديد AstBegin{flp, "", assignp, خطأ};
        forkp->جمع_کړه_Forksp(beginp);
        لیدنه(forkp);  // Visit now as we need to do some post-processing
        // IEEE 1800-2023 10.3.3 - if the RHS value differs from the currently scheduled value to
        // be assigned, the currently scheduled assignment is descheduled. To keep track if an
        // assignment should be descheduled, each scheduled assignment event has a 'generation',
        // and if at assignment time its generation differs from the current generation, it won't
        // be performed
        Ast_ټاکل* ثابت preAssignp = VN_AS(beginp->stmtsp(), ټاکل);
        UASSERT_OBJ(preAssignp, alwaysp, "Pre-assign should be there from visit(AstFork)");
        Ast_ټاکل* ثابت postAssignp = VN_AS(preAssignp->nextp()->nextp(), ټاکل);
        UASSERT_OBJ(postAssignp, alwaysp, "Post-assign should be there from visit(AstFork)");
        // Increment generation and copy it to a local
        AstVarScope* ثابت generationVarp
            = m_ټاکل_WGens.make(flp, m_scopep, alwaysp->پیدا_کړه_UInt64_DType());
        AstVarScope* ثابت genLocalVarp
            = create_لنډمهاله(flp, generationVarp->varp()->name() + "__local",
                         alwaysp->پیدا_کړه_UInt64_DType(), preAssignp);
        preAssignp->جمع_کړه_دلته_This_As_بل(
            جديد Ast_ټاکل{flp, جديد AstVarRef{flp, generationVarp, VAccess::WRITE},
                          جديد Ast_جمع_کړه{flp, جديد AstVarRef{flp, generationVarp, VAccess::READ},
                                     جديد AstConst{flp, 1}}});
        preAssignp->جمع_کړه_دلته_This_As_بل(
            جديد Ast_ټاکل{flp, جديد AstVarRef{flp, genLocalVarp, VAccess::WRITE},
                          جديد AstVarRef{flp, generationVarp, VAccess::READ}});
        // If the current generation is same as the one saved in the local var, assign
        beginp->جمع_کړه_Stmtsp(
            جديد AstIf{flp,
                      جديد AstEq{flp, جديد AstVarRef{flp, generationVarp, VAccess::READ},
                                جديد AstVarRef{flp, genLocalVarp, VAccess::READ}},
                      postAssignp->unlink_Fr_شا()});
        // Save scheduled RHS value before delay
        AstVarScope* ثابت tmpVarp
            = m_ټاکل_WTmps.make(flp, m_scopep, preAssignp->rhsp()->dtypep());
        AstVarRef* ثابت tmp_ټاکل_Rhsp = VN_AS(preAssignp->lhsp(), VarRef)->clone_ونه(خطأ);
        tmp_ټاکل_Rhsp->access(VAccess::WRITE);
        preAssignp->جمع_کړه_بل_دلته(
            جديد Ast_ټاکل{flp, جديد AstVarRef{flp, tmpVarp, VAccess::WRITE}, tmp_ټاکل_Rhsp});
        // If the RHS is different from the currently scheduled value, schedule the new assignment
        // The generation will increase, effectively 'descheduling' the previous assignment.
        Ast_غوټه_بیان* ثابت didNotInitp
            = جديد AstLogNot{flp, جديد AstCExpr{flp, "vlSymsp->__Vm_didInit", 1}};
        AstVarRef* ثابت لومړی_Iterp
            = جديد AstVarRef{flp, m_netlistp->stl_لومړی_Iterationp(), VAccess::READ};
        Ast_غوټه_بیان* ثابت schedCondp
            = جديد AstLogOr{flp,
                           جديد AstNeq{flp, preAssignp->rhsp()->clone_ونه(خطأ),
                                      جديد AstVarRef{flp, tmpVarp, VAccess::READ}},
                           جديد AstLogAnd{flp, didNotInitp, لومړی_Iterp}};
        alwaysp->جمع_کړه_Stmtsp(جديد AstIf{flp, schedCondp, forkp->unlink_Fr_شا()});
    }
    فراغ لیدنه(AstDisableFork* nodep) override {
        إذا (m_hasProcess) أرجع;
        // never reached by any process; remove to avoid compilation error
        VL_DO_DANGLING(nodep->unlink_Fr_شا()->delete_ونه(), nodep);
    }
    فراغ لیدنه(AstWaitFork* nodep) override {
        إذا (m_hasProcess) {
            File_کرښه* ثابت flp = nodep->fileline();
            AstCExpr* ثابت exprp = جديد AstCExpr{flp, "vlProcess->completedFork()", 1};
            AstWait* ثابت waitp = جديد AstWait{flp, exprp, تش_اشاره};
            nodep->replaceWith(waitp);
        } وإلا {
            // never reached by any process; remove to avoid compilation error
            nodep->unlink_Fr_شا();
        }
        VL_DO_DANGLING(nodep->delete_ونه(), nodep);
    }
    فراغ لیدنه(AstWait* nodep) override {
        // Wait on changed events related to the vars in the wait statement
        UINFO(9, "control-visit " << nodep);
        File_کرښه* ثابت flp = nodep->fileline();
        Ast_غوټه* ثابت stmtsp = nodep->stmtsp();
        إذا (stmtsp) stmtsp->unlink_Fr_شا_With_بل();
        Ast_غوټه_بیان* ثابت condp = V3Const::constifyEdit(nodep->condp()->unlink_Fr_شا());
        خپلکاری* ثابت constp = VN_CAST(condp, Const);
        إذا (constp) {
            إذا (constp->isZero()) {
                // We have to await forever instead of simply returning in case we're deep in a
                // callstack
                AstCMethodHard* ثابت foreverMethodp = جديد AstCMethodHard{
                    flp, جديد AstVarRef{flp, getCreateDelayScheduler(), VAccess::WRITE},
                    VCMethod::SCHED_WAIT_FOREVER};
                foreverMethodp->dtype_سېټ_Void();
                جمع_کړه_Process_Info(foreverMethodp);
                جمع_کړه_Debug_Info(foreverMethodp);
                AstCAwait* ثابت awaitp = جديد AstCAwait{flp, foreverMethodp};
                nodep->replaceWith(awaitp);
                إذا (stmtsp) VL_DO_DANGLING(stmtsp->delete_ونه(), stmtsp);
                VL_DO_DANGLING(condp->delete_ونه(), condp);
            } وإلا {
                nodep->v3fatalSrc("constant wait should have been removed in "
                                  "TimingSuspendableVisitor::visit(AstWait)");
            }
        } وإلا إذا (needDynamicTrigger(condp)) {
            // No point in making a sentree, just use the expression as sensitivity
            // Put the event control in an if so we only wait if the condition isn't met already
            خپلکاری* ثابت ifp = جديد AstIf{
                flp, جديد AstLogNot{flp, condp},
                جديد AstEventControl{flp,
                                    جديد Ast_Sen_ونه{flp, جديد Ast_Sen_توکی{flp, VEdgeType::ET_TRUE,
                                                                       condp->clone_ونه(خطأ)}},
                                    تش_اشاره}};
            إذا (stmtsp) Ast_غوټه::جمع_کړه_بل<Ast_غوټه, Ast_غوټه>(ifp, stmtsp);
            nodep->replaceWith(ifp);
        } وإلا {
            // We are using a global sentree, so we cannot use ET_TRUE, as that could lead to the
            // active region never converging. Because of this, we need to use ET_CHANGED in a
            // loop.
            AstEventControl* ثابت controlp = جديد AstEventControl{
                flp,
                جديد Ast_Sen_ونه{
                    flp, جديد Ast_Sen_توکی{flp, VEdgeType::ET_بدل_شوی, condp->clone_ونه(خطأ)}},
                تش_اشاره};
            controlp->user2(ریښتیا);  // Commit immediately
            AstLoop* ثابت loopp = جديد AstLoop{flp};
            loopp->جمع_کړه_Stmtsp(جديد AstLoopTest{flp, loopp, جديد AstLogNot{flp, condp}});
            loopp->جمع_کړه_Stmtsp(controlp);
            إذا (stmtsp) Ast_غوټه::جمع_کړه_بل<Ast_غوټه, Ast_غوټه>(loopp, stmtsp);
            nodep->replaceWith(loopp);
        }
        VL_DO_DANGLING(nodep->delete_ونه(), nodep);
    }
    فراغ لیدنه(AstBegin* nodep) override {
        VL_RESTORER(m_procp);
        VL_RESTORER(m_hasProcess);
        m_hasProcess |= hasFlags(nodep, T_HAS_PROC);
        // A pending NBA update is not a subprocess of the process that scheduled it, so it is
        // unaffected by 'disable fork' and 'wait fork' (IEEE 1800-2023 9.6.1, 9.6.3)
        إذا (hasFlags(nodep, T_NBA_UPDATE)) m_hasProcess = خطأ;
        m_procp = nodep;
        إذا (m_hasProcess) nodep->سېټ_Need_Process();
        iterateChildren(nodep);
    }
    فراغ لیدنه(AstFork* nodep) override {
        إذا (nodep->user1_سېټ_Once()) أرجع;
        UINFO(9, "control-visit " << nodep);
        v3Global.سېټ_کاروي_Timing();

        // Create a unique name for this fork
        nodep->name("__Vfork_" + std::منزل_string(++m_forkCnt));

        // TODO: Should process nodep->stmtsp() in case an earlier pass
        //       inserted something there, but as of today we don't need to.

        // Process and name each fork
        اندازه_t idx = 0;  // Index for naming begins
        لكل (AstBegin *itemp = nodep->forksp(), *nextp; itemp; itemp = nextp) {
            nextp = VN_AS(itemp->nextp(), Begin);
            iterate(itemp);
            // Note: Even if we do not find any awaits, we cannot simply inline
            // the process here, as new awaits could be added later.

            // Name the begin (later the name will be used for a new function)
            itemp->name(nodep->name() + "__" + std::منزل_string(idx++));
            إذا (itemp->needProcess()) {
                itemp->جمع_کړه_Stmtsp(
                    جديد AstCStmt{itemp->fileline(), "vlProcess->state(VlProcess::FINISHED);"});
            }
        }
        إذا (!nodep->joinType().joinNone()) makeForkJoin(nodep);
    }

    //--------------------
    فراغ لیدنه(AstVar*) override {}  // Accelerate
    فراغ لیدنه(Ast_غوټه* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    څرګند TimingControlVisitor(AstNetlist* nodep)
        : m_netlistp{nodep} {
        iterate(nodep);

        // If there is no static #0 in the design, but an unknown delay was found,
        // and the user did not specify preference, then warn on all unknown delays
        // as we will be assuming they can be #0, which can cause performance degradation.
        لكل (File_کرښه* ثابت flp : m_unknownDelayFlps) {
            flp->v3warn(ZERODLY,
                        "Value of # delay control statically unknown. Assuming it can be #0.\n"
                            << flp->warn_نور()  //
                            << "... If all # delays are non-zero at runtime,\n"
                            << flp->warn_نور()  //
                            << "... use '--no-sched-zero-delay' for improved performance.\n"
                            << flp->warn_نور()  //
                            << "... If a real #0 is expected at runtime,\n"
                            << flp->warn_نور()  //
                            << "... use '--sched-zero-delay' to suppress this warning.");
        }
    }
    ~TimingControlVisitor() override {
        V3Stats::جمع_کړه_Stat("Timing, known #0 delays", m_statZeroDelays);
        V3Stats::جمع_کړه_Stat("Timing, known #const delays", m_statConstDelays);
        V3Stats::جمع_کړه_Stat("Timing, unknown #variable delays", m_statVariableDelays);
    }
};

//######################################################################
// Timing class functions

Ast_غوټه_بیان* V3Timing::delay_منزل_Ticks(Ast_غوټه_بیان* valuep, VTimescale timeunit) {
    File_کرښه* ثابت flp = valuep->fileline();
    إذا (ثابت AstConst* ثابت constp = VN_CAST(valuep, Const)) {
        إذا (constp->عدد().is1_ګام()) {
            // A step is one tick of the global time precision (IEEE 1800-2023 3.14.3)
            VL_DO_DANGLING(valuep->delete_ونه(), valuep);
            AstConst* ثابت tickp = جديد AstConst{flp, AstConst::Unsized64{}, 1};
            tickp->dtype_سېټ_بټ_Sized(64, VSigning::UNSIGNED);
            أرجع tickp;
        }
        إذا (constp->isZero()) أرجع valuep;
    }
    UASSERT_OBJ(!timeunit.isNone(), valuep, "timeunit must be set");
    ثابت مزدوج timescale_ضریب_D
        = std::توان(10.0, timeunit.power_د_Ten() - v3Global.rootp()->timeprecision().power_د_Ten());
    إذا (valuep->dtypep()->skipRefp()->isDouble()) {
        AstConst* ثابت tsfp = جديد AstConst{flp, AstConst::RealDouble{}, timescale_ضریب_D};
        valuep = جديد AstMulD{flp, valuep, tsfp};
        valuep = جديد AstRToIRoundS{flp, valuep};
        valuep->dtype_سېټ_بټ_Sized(64, VSigning::UNSIGNED);
    } وإلا {
        valuep->dtype_سېټ_بټ_Sized(64, VSigning::UNSIGNED);
        ثابت uint64_t timescale_ضریب_U = static_cast<uint64_t>(timescale_ضریب_D);
        AstConst* ثابت tsfp = جديد AstConst{flp, AstConst::Unsized64{}, timescale_ضریب_U};
        valuep = جديد AstMul{flp, valuep, tsfp};
    }
    أرجع V3Const::constifyEdit(valuep);  // Simplify
}

فراغ V3Timing::timingAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        ثابت VNUser1_دننه_Use m_user1_دننه_Use;
        ثابت VNUser2_دننه_Use m_user2_دننه_Use;
        { TimingSuspendableVisitor{nodep}; }
        إذا (v3Global.کاروي_Timing()) TimingControlVisitor{nodep};
    }
    V3Global::dump_Check_Global_ونه("timing", 0, dump_ونه_Either_Level() >= 3);
}
