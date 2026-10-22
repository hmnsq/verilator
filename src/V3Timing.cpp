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

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ಅನ್ತರ್ಭಾವ "V3Timing.h"

#ಅನ್ತರ್ಭಾವ "V3Const.h"
#ಅನ್ತರ್ಭಾವ "V3EmitV.h"
#ಅನ್ತರ್ಭಾವ "V3File.h"
#ಅನ್ತರ್ಭಾವ "V3Global.h"
#ಅನ್ತರ್ಭಾವ "V3Graph.h"
#ಅನ್ತರ್ಭಾವ "V3MemberMap.h"
#ಅನ್ತರ್ಭಾವ "V3SenExprBuilder.h"
#ಅನ್ತರ್ಭಾವ "V3SenTree.h"
#ಅನ್ತರ್ಭಾವ "V3SharedTmps.h"
#ಅನ್ತರ್ಭಾವ "V3Stats.h"
#ಅನ್ತರ್ಭಾವ "V3UniqueNames.h"

#ಅನ್ತರ್ಭಾವ <limits>
#ಅನ್ತರ್ಭಾವ <queue>

VL_DEFINE_DEBUG_FUNCTIONS;

// ######################################################################

ಗಣನಾ ಗಂಟು_Flag : uint8_t {
    T_SUSPENDEE = 1 << 0,  // Suspendable (due to dependence on another suspendable)
    T_SUSPENDER = 1 << 1,  // Suspendable (has timing control)
    T_ALLOCS_PROC = 1 << 2,  // Can allocate VlProcess
    T_FORCES_PROC = 1 << 3,  // Forces VlProcess allocation
    T_NEEDS_PROC = 1 << 4,  // Needs access to VlProcess if it's allocated
    T_HAS_PROC = 1 << 5,  // Has VlProcess argument in the signature
    T_NBA_UPDATE = 1 << 6,  // Fork branch of a pending NBA update, which is not a subprocess
};

ಗಣನಾ ForkType : uint8_t {
    F_NONE = 0,  // Not under a fork
    F_MIGHT_SUSPEND = 1 << 0,  // Fork might suspend the execution of current process
    F_MIGHT_NEED_PROC = 1 << 1,  // Fork might need a process (any fork really)
};

ಗಣನಾ PropagationType : uint8_t {
    P_CALL = 1,  // Propagation through call to a function/task/method
    P_FORK = 2,  // Propagation due to fork's behavior
    P_SIGNATURE = 3,  // Propagation required to maintain C++ function's signature requirements
};

// Add timing flag to a node
ಸ್ಥಿರ ಶೂನ್ಯ addFlags(Ast_ಗಂಟು* ಧ್ರುವ nodep, uint8_t flags) { nodep->user2Or(flags); }
// Check if a node has ALL of the expected flags set
ಸ್ಥಿರ ತಾರ್ಕಿಕ hasFlags(Ast_ಗಂಟು* ಧ್ರುವ nodep, uint8_t flags) { ಪ್ರತ್ಯಾಗಮ !(~nodep->user2() & flags); }

// ######################################################################
//  Detect nodes affected by timing and/or requiring a process

ವರ್ಗ TimingSuspendableVisitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
    // TYPES
    // Vertex of a dependency graph of suspendable nodes, e.g. if a node (process or task) is
    // suspendable, all its dependents should also be suspendable
    ವರ್ಗ DepVtx VL_NOT_FINAL : ಸಾರ್ವಜನಿಕ V3GraphVertex {
        VL_RTTI_IMPL(DepVtx, V3GraphVertex)
        AstClass* ಧ್ರುವ m_classp;  // Class associated with a method
        Ast_ಗಂಟು* ಧ್ರುವ m_nodep;  // AST node represented by this graph vertex

        // ACCESSORS
        string name() ಧ್ರುವ override {
            ಯದಿ (m_classp) {
                ಯದಿ (VN_IS(nodep(), CFunc)) {
                    ಪ್ರತ್ಯಾಗಮ cvtToHex(nodep()) + ' ' + classp()->name() + "::" + nodep()->name();
                }
            }
            ಪ್ರತ್ಯಾಗಮ cvtToHex(nodep()) + ' ' + nodep()->prettyTypeName();
        }
        File_ಸಾಲು* fileline() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ nodep()->fileline(); }

    ಸಾರ್ವಜನಿಕ:
        // CONSTRUCTORS
        DepVtx(V3Graph* graphp, Ast_ಗಂಟು* nodep, AstClass* classp)
            : V3GraphVertex{graphp}
            , m_classp{classp}
            , m_nodep{nodep} {}
        ~DepVtx() override = ಸ್ವಭಾವ;

        // ACCESSORS
        ಆಭಾಸ Ast_ಗಂಟು* nodep() ಧ್ರುವ VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ m_nodep; }
        ಆಭಾಸ Ast_ಗಂಟು* classp() ಧ್ರುವ VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ m_classp; }
    };

    ವರ್ಗ SuspendDepVtx final : ಸಾರ್ವಜನಿಕ DepVtx {
        VL_RTTI_IMPL(SuspendDepVtx, DepVtx)
        string dotColor() ಧ್ರುವ override {
            ಯದಿ (hasFlags(nodep(), T_SUSPENDER)) ಪ್ರತ್ಯಾಗಮ "red";
            ಯದಿ (hasFlags(nodep(), T_SUSPENDEE)) ಪ್ರತ್ಯಾಗಮ "blue";
            ಪ್ರತ್ಯಾಗಮ "black";
        }

    ಸಾರ್ವಜನಿಕ:
        SuspendDepVtx(V3Graph* graphp, Ast_ಗಂಟು* nodep, AstClass* classp)
            : DepVtx{graphp, nodep, classp} {}
        ~SuspendDepVtx() override = ಸ್ವಭಾವ;
    };

    ವರ್ಗ NeedsProcDepVtx final : ಸಾರ್ವಜನಿಕ DepVtx {
        VL_RTTI_IMPL(NeedsProcDepVtx, DepVtx)
        string dotColor() ಧ್ರುವ override {
            ಯದಿ (hasFlags(nodep(), T_HAS_PROC)) ಪ್ರತ್ಯಾಗಮ "blue";
            ಯದಿ (hasFlags(nodep(), T_NEEDS_PROC)) ಪ್ರತ್ಯಾಗಮ "green";
            ಯದಿ (hasFlags(nodep(), T_FORCES_PROC)) ಪ್ರತ್ಯಾಗಮ "red";
            ಪ್ರತ್ಯಾಗಮ "black";
        }

    ಸಾರ್ವಜನಿಕ:
        NeedsProcDepVtx(V3Graph* graphp, Ast_ಗಂಟು* nodep, AstClass* classp)
            : DepVtx{graphp, nodep, classp} {}
        ~NeedsProcDepVtx() override = ಸ್ವಭಾವ;
    };

    // NODE STATE
    //  AstClass::user1() -> bool.  Class member cache has been refreshed.
    //  Ast{NodeProcedure,CFunc,Begin}::user2()  -> uint64_t.  Set to >= T_SUSP if
    //                                                         process/task suspendable
    //                                                         and to T_PROC if it
    //                                                         needs process metadata.
    //  Ast{NodeProcedure,CFunc,Begin}::user3()  -> DependencyVertex*.  Vertex in m_suspGraph
    //  Ast{NodeProcedure,CFunc,Begin}::user3()  -> DependencyVertex*.  Vertex in m_procGraph
    ಧ್ರುವ VNUser3InUse m_user3InUse;
    ಧ್ರುವ VNUser4InUse m_user4InUse;

    // STATE
    VMemberMap m_memberMap;  // Member names cached for fast lookup
    AstClass* m_classp = nullptr;  // Current class
    Ast_ಗಂಟು* m_procp = nullptr;  // NodeProcedure/CFunc/Begin we're under
    uint8_t m_underFork = F_NONE;  // F_NONE or flags of a fork we are under
    V3Graph m_suspGraph;  // Dependency graph where a node is a dependency of another if it being
                          // suspendable makes the other node suspendable
    V3Graph m_procGraph;  // Dependency graph where a node is a dependency of another if it being
                          // suspendable makes the other node suspendable

    // METHODS
    // Get or create the dependency vertex for the given node
    DepVtx* getSuspendDepVtx(Ast_ಗಂಟು* ಧ್ರುವ nodep) {
        AstClass* classp = nullptr;
        ಯದಿ (AstCFunc* funcp = VN_CAST(nodep, CFunc)) {
            ಯದಿ (funcp->scopep() && funcp->scopep()->modp()) {
                classp = VN_CAST(funcp->scopep()->modp(), Class);
            }
        }
        ಯದಿ (!nodep->user3p()) nodep->user3p(ನೂತನ SuspendDepVtx{&m_suspGraph, nodep, classp});
        ಪ್ರತ್ಯಾಗಮ nodep->user3u().to<SuspendDepVtx*>();
    }
    DepVtx* getNeedsProcDepVtx(Ast_ಗಂಟು* ಧ್ರುವ nodep) {
        AstClass* classp = nullptr;
        ಯದಿ (AstCFunc* funcp = VN_CAST(nodep, CFunc)) {
            ಯದಿ (funcp->scopep() && funcp->scopep()->modp()) {
                classp = VN_CAST(funcp->scopep()->modp(), Class);
            }
        }
        ಯದಿ (!nodep->user4p()) nodep->user4p(ನೂತನ NeedsProcDepVtx{&m_procGraph, nodep, classp});
        ಪ್ರತ್ಯಾಗಮ nodep->user4u().to<NeedsProcDepVtx*>();
    }
    // Pass timing flag between nodes
    ತಾರ್ಕಿಕ passFlag(ಧ್ರುವ Ast_ಗಂಟು* from, Ast_ಗಂಟು* to, ಗಂಟು_Flag flag) {
        ಯದಿ ((from->user2() & flag) && !(to->user2() & flag)) {
            addFlags(to, flag);
            ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        }
        ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    }
    // Propagate flag to all nodes that depend on the given one
    ಶೂನ್ಯ propagateFlags(DepVtx* ಧ್ರುವ vxp, ಗಂಟು_Flag flag) {
        auto* ಧ್ರುವ parentp = vxp->nodep();
        ಪ್ರತಿ (V3GraphEdge& edge : vxp->outEdges()) {
            auto* ಧ್ರುವ depVxp = static_cast<DepVtx*>(edge.ತುದಿ());
            Ast_ಗಂಟು* ಧ್ರುವ depp = depVxp->nodep();
            ಯದಿ (passFlag(parentp, depp, flag)) propagateFlags(depVxp, flag);
        }
    }
    template <typename T_Predicate>
    ಶೂನ್ಯ propagateFlagsIf(DepVtx* ಧ್ರುವ vxp, ಗಂಟು_Flag flag, T_Predicate p) {
        auto* ಧ್ರುವ parentp = vxp->nodep();
        ಪ್ರತಿ (V3GraphEdge& edge : vxp->outEdges()) {
            auto* ಧ್ರುವ depVxp = static_cast<DepVtx*>(edge.ತುದಿ());
            Ast_ಗಂಟು* ಧ್ರುವ depp = depVxp->nodep();
            ಯದಿ (p(&edge) && passFlag(parentp, depp, flag)) propagateFlagsIf(depVxp, flag, p);
        }
    }
    template <typename T_Predicate>
    ಶೂನ್ಯ propagateFlagsReversedIf(DepVtx* ಧ್ರುವ vxp, ಗಂಟು_Flag flag, T_Predicate p) {
        auto* ಧ್ರುವ parentp = vxp->nodep();
        ಪ್ರತಿ (V3GraphEdge& edge : vxp->inEdges()) {
            auto* ಧ್ರುವ depVxp = static_cast<DepVtx*>(edge.fromp());
            Ast_ಗಂಟು* ಧ್ರುವ depp = depVxp->nodep();
            ಯದಿ (p(&edge) && passFlag(parentp, depp, flag))
                propagateFlagsReversedIf(depVxp, flag, p);
        }
    }

    // VISITORS
    ಶೂನ್ಯ visit(AstClass* nodep) override {
        UASSERT_OBJ(!m_classp, nodep, "Class under class");
        VL_RESTORER(m_classp);
        m_classp = nodep;
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Procedure* nodep) override {
        VL_RESTORER(m_procp);
        m_procp = nodep;
        getNeedsProcDepVtx(nodep);
        addFlags(nodep, T_ALLOCS_PROC);
        ಯದಿ (VN_IS(nodep, Always)) {
            UINFO(9, "Always does " << (nodep->needProcess() ? "" : "NOT ") << "need process");
        }
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(AstDisableFork* nodep) override {
        visit(static_cast<Ast_ಗಂಟು*>(nodep));
        addFlags(m_procp, T_FORCES_PROC | T_NEEDS_PROC);
    }
    ಶೂನ್ಯ visit(AstWaitFork* nodep) override {
        visit(static_cast<Ast_ಗಂಟು*>(nodep));
        addFlags(m_procp, T_FORCES_PROC | T_NEEDS_PROC);
    }
    ಶೂನ್ಯ visit(AstWait* nodep) override {
        UINFO(9, "suspendable-visit " << nodep);
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ condp = V3Const::constifyEdit(nodep->condp());
        ಯದಿ (AstConst* ಧ್ರುವ constp = VN_CAST(condp, Const)) {
            ಯದಿ (!nodep->fileline()->warnIsOff(V3ErrorCode::WAITCONST)) {
                condp->v3warn(WAITCONST, "Wait statement condition is constant");
            }
            ಯದಿ (!constp->isZero()) {
                // Remove AstWait before we track process as T_SUSPENDER
                ಯದಿ (Ast_ಗಂಟು* ಧ್ರುವ stmtsp = nodep->stmtsp()) {
                    stmtsp->unlinkFrBackWithNext();
                    nodep->replaceWith(stmtsp);
                } ಅನ್ಯಥಾ {
                    nodep->unlinkFrBack();
                }
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                ಪ್ರತ್ಯಾಗಮ;
            }
        }
        v3Global.setUsesTiming();
        ಯದಿ (m_procp) addFlags(m_procp, T_SUSPENDEE | T_SUSPENDER | T_NEEDS_PROC);
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(AstCFunc* nodep) override {
        VL_RESTORER(m_procp);
        m_procp = nodep;
        iterateChildren(nodep);
        ಯದಿ (nodep->needProcess()) addFlags(nodep, T_FORCES_PROC | T_NEEDS_PROC);
        DepVtx* ಧ್ರುವ sVxp = getSuspendDepVtx(nodep);
        DepVtx* ಧ್ರುವ pVxp = getNeedsProcDepVtx(nodep);
        ಯದಿ (!m_classp) ಪ್ರತ್ಯಾಗಮ;

        // Go over overridden functions

        std::queue<AstClassExtends*> extends;
        ಯದಿ (m_classp->extendsp()) extends.ತಳ್ಳು(m_classp->extendsp());

        ಯಾವತ್ (!extends.empty()) {
            AstClassExtends* ext_list = extends.front();
            extends.ತೆಗೆ();

            ಪ್ರತಿ (AstClassExtends* cextp = ext_list; cextp;
                 cextp = VN_AS(cextp->nextp(), ClassExtends)) {
                // TODO: It is possible that a methods the same name in the base class is not
                // actually overridden by our method. If this causes a problem, traverse to
                // the root of the inheritance hierarchy and check if the original method is
                // virtual or not.
                ಯದಿ (auto* ಧ್ರುವ overriddenp
                    = VN_CAST(m_memberMap.findMember(cextp->classp(), nodep->name()), CFunc)) {
                    // Suspendability and process affects typing, so they propagate both ways
                    DepVtx* ಧ್ರುವ overriddenSVxp = getSuspendDepVtx(overriddenp);
                    DepVtx* ಧ್ರುವ overriddenPVxp = getNeedsProcDepVtx(overriddenp);
                    ನೂತನ V3GraphEdge{&m_suspGraph, sVxp, overriddenSVxp, P_SIGNATURE};
                    ನೂತನ V3GraphEdge{&m_suspGraph, overriddenSVxp, sVxp, P_SIGNATURE};
                    ನೂತನ V3GraphEdge{&m_procGraph, pVxp, overriddenPVxp, P_SIGNATURE};
                    ನೂತನ V3GraphEdge{&m_procGraph, overriddenPVxp, pVxp, P_SIGNATURE};
                } ಅನ್ಯಥಾ {
                    AstClassExtends* more_extends = cextp->classp()->extendsp();
                    ಯದಿ (more_extends) extends.ತಳ್ಳು(more_extends);
                }
            }
        }
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_CCall* nodep) override {
        AstCFunc* ಧ್ರುವ funcp = nodep->funcp();
        UASSERT_OBJ(funcp, nodep, "AstNodeCCall must have non-null funcp post-link");
        UASSERT_OBJ(m_procp, nodep, "AstNodeCCall must be inside a procedure/CFunc/Begin");

        UINFO(9, "V3Timing: Processing CCall to " << funcp->name() << " in dependency graph\n");
        ನೂತನ V3GraphEdge{&m_suspGraph, getSuspendDepVtx(funcp), getSuspendDepVtx(m_procp), P_CALL};

        ನೂತನ V3GraphEdge{&m_procGraph, getNeedsProcDepVtx(funcp), getNeedsProcDepVtx(m_procp),
                        P_CALL};

        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(AstBegin* nodep) override {
        VL_RESTORER(m_procp);
        VL_RESTORER(m_underFork);

        ಯದಿ (!m_underFork || (m_underFork & F_MIGHT_SUSPEND))
            ನೂತನ V3GraphEdge{&m_suspGraph, getSuspendDepVtx(nodep), getSuspendDepVtx(m_procp),
                            m_underFork ? P_FORK : P_CALL};

        ನೂತನ V3GraphEdge{&m_procGraph, getNeedsProcDepVtx(nodep), getNeedsProcDepVtx(m_procp),
                        P_CALL};

        ಯದಿ (m_underFork) addFlags(nodep, T_NEEDS_PROC | T_ALLOCS_PROC);

        m_procp = nodep;
        m_underFork = 0;
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(AstFork* nodep) override {
        UINFO(9, "suspendable-visit " << nodep);
        VL_RESTORER(m_underFork);

        v3Global.setUsesTiming();  // Even if there are no event controls, we have to set this flag
                                   // so that transformForks() in V3SchedTiming gets called and
                                   // removes all forks and begins
        ಯದಿ (nodep->isTimingControl() && m_procp) {
            addFlags(m_procp, T_SUSPENDEE | T_SUSPENDER);
            m_underFork |= F_MIGHT_SUSPEND;
        }
        m_underFork |= F_MIGHT_NEED_PROC;
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(AstAssignDly* nodep) override {
        ಯದಿ (!VN_IS(m_procp, ಗಂಟು_Procedure)) v3Global.setUsesTiming();
        visit(static_cast<Ast_ಗಂಟು*>(nodep));
    }
    ಶೂನ್ಯ visit(AstFireEvent* nodep) override {
        ಯದಿ (nodep->isDelayed()
            && (!VN_IS(m_procp, ಗಂಟು_Procedure) || !VN_IS(nodep->operandp(), VarRef))) {
            v3Global.setUsesTiming();
        }
        visit(static_cast<Ast_ಗಂಟು*>(nodep));
    }
    ಶೂನ್ಯ visit(AstAssignW* nodep) override {
        ಯದಿ (nodep->timingControlp()) v3Global.setUsesTiming();
        // Containing process will not suspend, don't mark it
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು* nodep) override {
        ಯದಿ (nodep->isTimingControl()) {
            v3Global.setUsesTiming();
            ಯದಿ (m_procp) addFlags(m_procp, T_SUSPENDEE | T_SUSPENDER | T_NEEDS_PROC);
        }
        iterateChildren(nodep);
    }

    //--------------------
    ಶೂನ್ಯ visit(AstVar*) override {}  // Accelerate

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    explicit TimingSuspendableVisitor(AstNetlist* nodep) {
        iterate(nodep);
        m_suspGraph.removeTransitiveEdges();
        m_procGraph.removeTransitiveEdges();
        // Propagate suspendability
        ಪ್ರತಿ (V3GraphVertex& vtx : m_suspGraph.vertices()) {
            DepVtx& depVtx = static_cast<DepVtx&>(vtx);
            ಯದಿ (hasFlags(depVtx.nodep(), T_SUSPENDEE)) propagateFlags(&depVtx, T_SUSPENDEE);
        }
        ಯದಿ (dumpGraphLevel() >= 6) m_suspGraph.dumpDotFilePrefixed("timing_deps");

        // Propagate T_HAS_PROCESS
        ಪ್ರತಿ (V3GraphVertex& vtx : m_procGraph.vertices()) {
            DepVtx& depVtx = static_cast<DepVtx&>(vtx);
            // Find processes that'll allocate VlProcess
            ಯದಿ (hasFlags(depVtx.nodep(), T_FORCES_PROC)) {
                propagateFlagsIf(&depVtx, T_FORCES_PROC, [&](ಧ್ರುವ V3GraphEdge* e) -> ತಾರ್ಕಿಕ {
                    ಪ್ರತ್ಯಾಗಮ !hasFlags(static_cast<DepVtx*>(e->fromp())->nodep(), T_ALLOCS_PROC);
                });
            }
            // Mark nodes on paths between processes and statements that use VlProcess
            ಯದಿ (hasFlags(depVtx.nodep(), T_NEEDS_PROC)) {
                propagateFlagsIf(&depVtx, T_NEEDS_PROC, [&](ಧ್ರುವ V3GraphEdge* e) -> ತಾರ್ಕಿಕ {
                    ಪ್ರತ್ಯಾಗಮ !hasFlags(static_cast<DepVtx*>(e->ತುದಿ())->nodep(), T_ALLOCS_PROC);
                });
            }
        }
        ಪ್ರತಿ (V3GraphVertex& vtx : m_procGraph.vertices()) {
            DepVtx& depVtx = static_cast<DepVtx&>(vtx);
            // Mark nodes that will be emitted with a VlProcess argument
            ಯದಿ (hasFlags(depVtx.nodep(), T_ALLOCS_PROC | T_FORCES_PROC)) {
                addFlags(depVtx.nodep(), T_HAS_PROC);
                propagateFlagsReversedIf(&depVtx, T_HAS_PROC, [&](ಧ್ರುವ V3GraphEdge* e) -> ತಾರ್ಕಿಕ {
                    ಪ್ರತ್ಯಾಗಮ hasFlags(static_cast<DepVtx*>(e->fromp())->nodep(), T_NEEDS_PROC);
                });
            }
        }
        ಯದಿ (dumpGraphLevel() >= 6) m_procGraph.dumpDotFilePrefixed("proc_deps");
    }
    ~TimingSuspendableVisitor() override = ಸ್ವಭಾವ;
};

// ######################################################################
//  Transform nodes affected by timing

ವರ್ಗ TimingControlVisitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
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
    AstNetlist* ಧ್ರುವ m_netlistp;  // Root node
    Ast_ತುದಿ_Scope* ಧ್ರುವ m_ತುದಿ_Scopep = m_netlistp->ತುದಿ_Scopep();  // The AstTopScope
    AstScope* ಧ್ರುವ m_scopeTopp = m_ತುದಿ_Scopep->scopep();  // Scope at the top
    AstClass* m_classp = nullptr;  // Current class
    AstScope* m_scopep = nullptr;  // Current scope
    AstActive* m_activep = nullptr;  // Current active
    Ast_ಗಂಟು* m_procp = nullptr;  // NodeProcedure/CFunc/Begin we're under
    ತಾರ್ಕಿಕ m_hasProcess = ಅಸತ್ಯ;  // True if current scope has a VlProcess handle available
    ಪೂರ್ಣಾಙ್ಕ m_forkCnt = 0;  // Number of forks inside a module
    ತಾರ್ಕಿಕ m_underJumpBlock = ಅಸತ್ಯ;  // True if we are inside of a jump-block
    ತಾರ್ಕಿಕ m_underProcedure = ಅಸತ್ಯ;  // True if we are under an always or initial
    ತಾರ್ಕಿಕ m_underIfaceCFunc = ಅಸತ್ಯ;  // True if we are under a CFunc owned by an interface scope
    ತಾರ್ಕಿಕ m_hasStaticZeroDelay = ಅಸತ್ಯ;  // True if we have a static #0 delay
    std::vector<File_ಸಾಲು*> m_unknownDelayFlps;  // Locations of AstDelay with non-constant value

    // Unique names
    V3UniqueNames m_dlyforkNames{"__Vdlyfork"};  // Names for temp AssignW vars
    V3UniqueNames m_intra_ಮೌಲ್ಯ_Names{"__Vintraval"};  // Intra assign delay value var names
    V3UniqueNames m_intraIndexNames{"__Vintraidx"};  // Intra assign delay index var names
    V3UniqueNames m_intraLsbNames{"__Vintralsb"};  // Intra assign delay LSB var names
    V3UniqueNames m_trigSchedNames{"__VtrigSched"};  // Trigger scheduler name generator
    V3UniqueNames m_dynTrigNames{"__VdynTrigger"};  // Dynamic trigger name generator
    // Module level temporary variables, shared by instances
    V3SharedTmps m_assignWTmps{"__VassignWtmp", VVarType::MODULETEMP};  // Delayed AssignW values
    V3SharedTmps m_assignWGens{"__VassignWgen", VVarType::MODULETEMP};  // AssignW generations

    // DTypes
    AstBasicDType* m_forkDtp = nullptr;  // Fork variable type
    AstBasicDType* m_trigSchedDtp = nullptr;  // Trigger scheduler type

    // Timing-related globals
    AstVarScope* m_delaySchedp = nullptr;  // Global delay scheduler
    AstVarScope* m_dynamicSchedp = nullptr;  // Global dynamic trigger scheduler
    Ast_Sen_ವೃಕ್ಷ* m_delaySensesp = nullptr;  // Domain to trigger if a delayed coroutine is resumed
    Ast_Sen_ವೃಕ್ಷ* m_dynamicSensesp = nullptr;  // Domain to trigger if a dynamic trigger is set

    // Other
    Sen_ವೃಕ್ಷ_Finder m_finder{m_netlistp};  // Sentree finder and uniquifier
    Sen_ಅಭಿವ್ಯಕ್ತಿ_Builder* m_sen_ಅಭಿವ್ಯಕ್ತಿ_Builderp = nullptr;  // Sens expression builder for current m_scope

    // Stats
    size_t m_statZeroDelays = 0;  // Number of statically known #0 delays
    size_t m_statConstDelays = 0;  // Number of statically known #const (non-zero) delays
    size_t m_statVariableDelays = 0;  // Number of delays with value unknown at compile time

    // METHODS
    // Transform an assignment with an intra timing control into a timing control with the
    // assignment under it
    Ast_ಗಂಟು* factorOutTimingControl(Ast_ಗಂಟು_Assign* nodep) ಧ್ರುವ {
        Ast_ಗಂಟು* stmtp = nodep;
        Ast_ಗಂಟು* ಧ್ರುವ controlp = nodep->timingControlp();
        ಯದಿ (AstDelay* ಧ್ರುವ delayp = VN_CAST(controlp, Delay)) {
            stmtp->replaceWith(delayp->unlinkFrBack());
            delayp->addStmtsp(stmtp);
            stmtp = delayp;
        } ಅನ್ಯಥಾ ಯದಿ (Ast_Sen_ವೃಕ್ಷ* ಧ್ರುವ sentreep = VN_CAST(controlp, Sen_ವೃಕ್ಷ)) {
            AstEventControl* ಧ್ರುವ eventControlp
                = ನೂತನ AstEventControl{sentreep->fileline(), sentreep->unlinkFrBack(), nullptr};
            stmtp->replaceWith(eventControlp);
            eventControlp->addStmtsp(stmtp);
            stmtp = eventControlp;
        } ಅನ್ಯಥಾ ಯದಿ (AstBegin* ಧ್ರುವ beginp = VN_CAST(controlp, Begin)) {
            // Begin from V3AssertPre
            stmtp->replaceWith(beginp->unlinkFrBack());
            beginp->addStmtsp(stmtp);
            stmtp = beginp;
        }
        ಪ್ರತ್ಯಾಗಮ stmtp == nodep ? nullptr : stmtp;
    }
    ಸ್ಥಿರ ತಾರ್ಕಿಕ staticallyNonZeroDelay(ಧ್ರುವ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* valuep) {
        ಯದಿ (ಧ್ರುವ AstConst* ಧ್ರುವ constp = VN_CAST(valuep, Const)) ಪ್ರತ್ಯಾಗಮ !constp->isZero();
        ಯದಿ (ಧ್ರುವ AstCond* ಧ್ರುವ condp = VN_CAST(valuep, Cond)) {
            ಪ್ರತ್ಯಾಗಮ staticallyNonZeroDelay(condp->thenp())
                   && staticallyNonZeroDelay(condp->elsep());
        }
        ಯದಿ (ಧ್ರುವ AstMul* ಧ್ರುವ mulp = VN_CAST(valuep, Mul)) {
            ಧ್ರುವ AstConst* ಧ್ರುವ lhsConstp = VN_CAST(mulp->lhsp(), Const);
            ಧ್ರುವ AstConst* ಧ್ರುವ rhsConstp = VN_CAST(mulp->rhsp(), Const);
            ಯದಿ (lhsConstp && !lhsConstp->isZero()) ಪ್ರತ್ಯಾಗಮ staticallyNonZeroDelay(mulp->rhsp());
            ಯದಿ (rhsConstp && !rhsConstp->isZero()) ಪ್ರತ್ಯಾಗಮ staticallyNonZeroDelay(mulp->lhsp());
        }
        ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    }
    // Creates the global delay scheduler variable
    AstVarScope* getCreateDelayScheduler() {
        ಯದಿ (m_delaySchedp) ಪ್ರತ್ಯಾಗಮ m_delaySchedp;
        auto* ಧ್ರುವ dlySchedDtp = ನೂತನ AstBasicDType{
            m_scopeTopp->fileline(), VBasicDTypeKwd::DELAY_SCHEDULER, VSigning::UNSIGNED};
        m_netlistp->typeTablep()->addTypesp(dlySchedDtp);
        m_delaySchedp = m_ತುದಿ_Scopep->createTemp("__VdlySched", dlySchedDtp);
        // Delay scheduler has to be accessible from top
        m_delaySchedp->varp()->sigPublic(ಸತ್ಯ);
        m_netlistp->delaySchedulerp(m_delaySchedp->varp());
        ಪ್ರತ್ಯಾಗಮ m_delaySchedp;
    }
    // Creates the delay sentree
    Ast_Sen_ವೃಕ್ಷ* get_Create_Delay_Sen_ವೃಕ್ಷ() {
        ಯದಿ (m_delaySensesp) ಪ್ರತ್ಯಾಗಮ m_delaySensesp;
        File_ಸಾಲು* ಧ್ರುವ flp = m_scopeTopp->fileline();
        auto* ಧ್ರುವ awaitingCurrentTimep
            = ನೂತನ AstCMethodHard{flp, ನೂತನ AstVarRef{flp, getCreateDelayScheduler(), VAccess::READ},
                                 VCMethod::SCHED_AWAITING_CURRENT_TIME};
        awaitingCurrentTimep->dtypeSetBit();
        m_delaySensesp
            = ನೂತನ Ast_Sen_ವೃಕ್ಷ{flp, ನೂತನ AstSenItem{flp, VEdgeType::ET_TRUE, awaitingCurrentTimep}};
        m_netlistp->ತುದಿ_Scopep()->addSenTreesp(m_delaySensesp);
        ಪ್ರತ್ಯಾಗಮ m_delaySensesp;
    }
    // Creates the global dynamic trigger scheduler variable
    AstVarScope* getCreateDynamicTriggerScheduler() {
        ಯದಿ (m_dynamicSchedp) ಪ್ರತ್ಯಾಗಮ m_dynamicSchedp;
        auto* ಧ್ರುವ dynSchedDtp
            = ನೂತನ AstBasicDType{m_scopeTopp->fileline(), VBasicDTypeKwd::DYNAMIC_TRIGGER_SCHEDULER,
                                VSigning::UNSIGNED};
        m_netlistp->typeTablep()->addTypesp(dynSchedDtp);
        m_dynamicSchedp = m_ತುದಿ_Scopep->createTemp("__VdynSched", dynSchedDtp);
        ಪ್ರತ್ಯಾಗಮ m_dynamicSchedp;
    }
    // Creates the dynamic trigger sentree
    Ast_Sen_ವೃಕ್ಷ* get_Create_Dynamic_Trigger_Sen_ವೃಕ್ಷ() {
        ಯದಿ (m_dynamicSensesp) ಪ್ರತ್ಯಾಗಮ m_dynamicSensesp;
        File_ಸಾಲು* ಧ್ರುವ flp = m_scopeTopp->fileline();
        auto* ಧ್ರುವ awaitingCurrentTimep = ನೂತನ AstCMethodHard{
            flp, ನೂತನ AstVarRef{flp, getCreateDynamicTriggerScheduler(), VAccess::READ},
            VCMethod::SCHED_EVALUATE};
        awaitingCurrentTimep->dtypeSetBit();
        m_dynamicSensesp
            = ನೂತನ Ast_Sen_ವೃಕ್ಷ{flp, ನೂತನ AstSenItem{flp, VEdgeType::ET_TRUE, awaitingCurrentTimep}};
        m_netlistp->ತುದಿ_Scopep()->addSenTreesp(m_dynamicSensesp);
        ಪ್ರತ್ಯಾಗಮ m_dynamicSensesp;
    }
    // Creates the event variable to trigger in NBA region
    AstEventControl* createNbaEventControl(File_ಸಾಲು* flp) {
        ಯದಿ (!m_netlistp->nbaEventp()) {
            auto* ಧ್ರುವ nbaEventDtp = ನೂತನ AstBasicDType{m_scopeTopp->fileline(),
                                                        VBasicDTypeKwd::EVENT, VSigning::UNSIGNED};
            m_netlistp->typeTablep()->addTypesp(nbaEventDtp);
            m_netlistp->nbaEventp(m_ತುದಿ_Scopep->createTemp("__VnbaEvent", nbaEventDtp));
            v3Global.setHasEvents();
        }
        ಪ್ರತ್ಯಾಗಮ ನೂತನ AstEventControl{
            flp,
            ನೂತನ Ast_Sen_ವೃಕ್ಷ{
                flp, ನೂತನ AstSenItem{flp, VEdgeType::ET_EVENT,
                                    ನೂತನ AstVarRef{flp, m_netlistp->nbaEventp(), VAccess::READ}}},
            nullptr};
    }
    // Creates the variable that, if set, causes the NBA event to be triggered
    AstAssign* createNbaEventTriggerAssignment(File_ಸಾಲು* flp) {
        ಯದಿ (!m_netlistp->nbaEventTriggerp()) {
            m_netlistp->nbaEventTriggerp(m_ತುದಿ_Scopep->createTemp("__VnbaEventTrigger", 1));
        }
        ಪ್ರತ್ಯಾಗಮ ನೂತನ AstAssign{flp,
                             ನೂತನ AstVarRef{flp, m_netlistp->nbaEventTriggerp(), VAccess::WRITE},
                             ನೂತನ AstConst{flp, AstConst::BitTrue{}}};
    }
    // Returns true if we are under a class or interface function, or the tree references locals.
    // These are cases where static, globally-evaluated triggers are not suitable.
    ತಾರ್ಕಿಕ needDynamicTrigger(Ast_ಗಂಟು* ಧ್ರುವ nodep) ಧ್ರುವ {
        ಪ್ರತ್ಯಾಗಮ m_classp || m_underIfaceCFunc || nodep->exists([](Ast_ಗಂಟು* ಧ್ರುವ nodep) {
            ಯದಿ (Ast_ಗಂಟು_Var_Ref* varp = VN_CAST(nodep, ಗಂಟು_Var_Ref)) {
                ಪ್ರತ್ಯಾಗಮ varp->varp()->isFuncLocal();
            }
            ಪ್ರತ್ಯಾಗಮ !nodep->isPure();
        });
    }
    // Creates a trigger scheduler variable
    AstVarScope* getCreateTriggerSchedulerp(Ast_Sen_ವೃಕ್ಷ* ಧ್ರುವ sentreep) {
        ಯದಿ (!sentreep->user1p()) {
            ಯದಿ (!m_trigSchedDtp) {
                m_trigSchedDtp
                    = ನೂತನ AstBasicDType{m_scopeTopp->fileline(), VBasicDTypeKwd::TRIGGER_SCHEDULER,
                                        VSigning::UNSIGNED};
                m_netlistp->typeTablep()->addTypesp(m_trigSchedDtp);
            }
            AstVarScope* ಧ್ರುವ trigSchedp
                = m_ತುದಿ_Scopep->createTemp(m_trigSchedNames.get(sentreep), m_trigSchedDtp);
            sentreep->user1p(trigSchedp);
        }
        ಪ್ರತ್ಯಾಗಮ VN_AS(sentreep->user1p(), VarScope);
    }
    // Creates a string describing the sentree
    AstCExpr* createEventDescription(Ast_Sen_ವೃಕ್ಷ* ಧ್ರುವ sentreep) ಧ್ರುವ {
        ಯದಿ (!sentreep->user2p()) {
            std::stringstream ss;
            V3EmitV::verilog_For_ವೃಕ್ಷ(sentreep, ss);
            // possibly a multiline string
            // Names were hashed by V3Name, show the user the original text
            std::string comment = VName::dehash(ss.str());
            std::replace(comment.begin(), comment.end(), '\n', ' ');
            // Escape quotes, e.g. from string-indexed event array selects
            comment = '"' + V3OutFormatter::quoteNameControls(comment) + '"';
            AstCExpr* ಧ್ರುವ commentp = ನೂತನ AstCExpr{sentreep->fileline(), comment};
            commentp->dtypeSetString();
            sentreep->user2p(commentp);
            ಪ್ರತ್ಯಾಗಮ commentp;
        }
        ಪ್ರತ್ಯಾಗಮ VN_AS(sentreep->user2p(), CExpr)->clone_ವೃಕ್ಷ(ಅಸತ್ಯ);
    }
    // Adds debug info to a hardcoded method call
    ಶೂನ್ಯ addDebugInfo(AstCMethodHard* ಧ್ರುವ methodp) ಧ್ರುವ {
        File_ಸಾಲು* ಧ್ರುವ flp = methodp->fileline();
        ಧ್ರುವ ತಾರ್ಕಿಕ protectIds = v3Global.opt.protectIds();
        AstCExpr* ಧ್ರುವ ap
            = ನೂತನ AstCExpr{flp, protectIds ? "VL_UNKNOWN" : '"' + flp->filenameEsc() + '"'};
        ap->dtypeSetString();
        methodp->addPinsp(ap);
        AstCExpr* ಧ್ರುವ bp = ನೂತನ AstCExpr{flp, protectIds ? "0" : cvtToStr(flp->lineno())};
        bp->dtypeSetString();
        methodp->addPinsp(bp);
    }
    // Adds debug info to a trigSched.trigger() call
    ಶೂನ್ಯ addEventDebugInfo(AstCMethodHard* ಧ್ರುವ methodp, Ast_Sen_ವೃಕ್ಷ* ಧ್ರುವ sentreep) ಧ್ರುವ {
        ಯದಿ (v3Global.opt.protectIds()) {
            File_ಸಾಲು* ಧ್ರುವ flp = sentreep->fileline();
            AstCExpr* ಧ್ರುವ descp = ನೂತನ AstCExpr{flp, "VL_UNKNOWN"};
            descp->dtypeSetString();
            methodp->addPinsp(descp);
            addDebugInfo(methodp);
            ಪ್ರತ್ಯಾಗಮ;
        }
        methodp->addPinsp(createEventDescription(sentreep));
        addDebugInfo(methodp);
    }
    // Adds process pointer to a hardcoded method call
    ಶೂನ್ಯ addProcessInfo(AstCMethodHard* ಧ್ರುವ methodp) ಧ್ರುವ {
        File_ಸಾಲು* ಧ್ರುವ flp = methodp->fileline();
        AstCExpr* ಧ್ರುವ ap
            = ನೂತನ AstCExpr{flp, (m_procp && m_hasProcess) ? "vlProcess" : "nullptr"};
        methodp->addPinsp(ap);
    }
    // Creates the fork handle type and returns it
    AstBasicDType* getCreateForkSyncDTypep() {
        ಯದಿ (m_forkDtp) ಪ್ರತ್ಯಾಗಮ m_forkDtp;
        m_forkDtp = ನೂತನ AstBasicDType{m_scopeTopp->fileline(), VBasicDTypeKwd::FORK_SYNC,
                                      VSigning::UNSIGNED};
        m_netlistp->typeTablep()->addTypesp(m_forkDtp);
        ಪ್ರತ್ಯಾಗಮ m_forkDtp;
    }
    // Move `nodep` into `AstCLocalScope`.
    AstCLocalScope* addCLocalScope(File_ಸಾಲು* ಧ್ರುವ flp, Ast_ಗಂಟು* ಧ್ರುವ nodep) ಧ್ರುವ {
        ಯದಿ (!nodep) ಪ್ರತ್ಯಾಗಮ nullptr;
        VNRelinker handle;
        nodep->unlinkFrBack(&handle);
        AstCLocalScope* ಧ್ರುವ cscopep = ನೂತನ AstCLocalScope{flp, nodep};
        handle.relink(cscopep);
        ಪ್ರತ್ಯಾಗಮ cscopep;
    }
    // Create a temp variable and optionally put it before the specified node (mark local if so)
    AstVarScope* createTemp(File_ಸಾಲು* ಧ್ರುವ flp, ಧ್ರುವ std::string& name,
                            Ast_ಗಂಟು_DType* ಧ್ರುವ dtypep, Ast_ಗಂಟು* ಧ್ರುವ ಸೇರಿಸು_Beforep = nullptr) {
        AstVar* varp;
        ಯದಿ (ಸೇರಿಸು_Beforep) {
            varp = ನೂತನ AstVar{flp, VVarType::BLOCKTEMP, name, dtypep};
            varp->funcLocal(ಸತ್ಯ);
            ಸೇರಿಸು_Beforep->addHereThisAsNext(varp);
        } ಅನ್ಯಥಾ {
            varp = ನೂತನ AstVar{flp, VVarType::MODULETEMP, name, dtypep};
            m_scopep->modp()->addStmtsp(varp);
        }
        AstVarScope* ಧ್ರುವ vscp = ನೂತನ AstVarScope{flp, m_scopep, varp};
        m_scopep->addVarsp(vscp);
        ಪ್ರತ್ಯಾಗಮ vscp;
    }
    // Add a done() call on the fork sync
    ಶೂನ್ಯ addForkDone(AstBegin* ಧ್ರುವ beginp, AstVarScope* ಧ್ರುವ forkVscp) ಧ್ರುವ {
        File_ಸಾಲು* ಧ್ರುವ flp = beginp->fileline();
        auto* ಧ್ರುವ donep = ನೂತನ AstCMethodHard{
            beginp->fileline(), ನೂತನ AstVarRef{flp, forkVscp, VAccess::WRITE}, VCMethod::FORK_DONE};
        donep->dtypeSetVoid();
        addDebugInfo(donep);
        beginp->addStmtsp(donep->makeStmt());
    }
    ಸ್ಥಿರ ತಾರ್ಕಿಕ has_Disable_Queue_ತಳ್ಳು_Self_Prefix(AstBegin* ಧ್ರುವ beginp) {
        // LinkJump prepends disable-by-name registration as:
        //   __VprocessQueue_*.push_back(std::process::self())
        // V3LiftExpr lifts std::process::self() into an assignment to a temporary, then V3Task
        // lowers that assignment's function call into a leading comment and AstStmtExpr. Thus the
        // push_back is no longer necessarily the first statement. Scan across this generated
        // prefix for it, stopping at the fork-start sentinel or the branch body. Registrations for
        // nested forks live in sub-blocks and so are not in this statement list.
        ಪ್ರತಿ (Ast_ಗಂಟು* stmtp = beginp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ಯದಿ (VN_IS(stmtp, Comment)) ಅನುವರ್ತ;
            Ast_Stmt_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ stmtExprp = VN_CAST(stmtp, Stmt_ಅಭಿವ್ಯಕ್ತಿ);
            ಯದಿ (!stmtExprp) ಭಙ್ಗ;
            ಯದಿ (stmtExprp->is_Disable_Queue_ತಳ್ಳು_Self_Stmt()) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        }
        ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    }
    // Register a callback so killing a process-backed fork branch decrements the join counter
    ಶೂನ್ಯ addForkOnKill(AstBegin* ಧ್ರುವ beginp, AstVarScope* ಧ್ರುವ forkVscp) ಧ್ರುವ {
        ಯದಿ (!beginp->needProcess() && !has_Disable_Queue_ತಳ್ಳು_Self_Prefix(beginp)) ಪ್ರತ್ಯಾಗಮ;
        File_ಸಾಲು* ಧ್ರುವ flp = beginp->fileline();
        AstCMethodHard* ಧ್ರುವ onKillp = ನೂತನ AstCMethodHard{
            flp, ನೂತನ AstVarRef{flp, forkVscp, VAccess::WRITE}, VCMethod::FORK_ON_KILL};
        onKillp->dtypeSetVoid();
        AstCExpr* ಧ್ರುವ processp = ನೂತನ AstCExpr{flp, "vlProcess"};
        processp
            ->dtypeSetVoid();  // Opaque process reference; type is irrelevant for hardcoded emit
        onKillp->addPinsp(processp);
        Ast_ಗಂಟು_Stmt* ಧ್ರುವ stmtp = onKillp->makeStmt();
        beginp->prependStmtsp(stmtp);
    }
    // Handle the 'join' part of a fork..join
    ಶೂನ್ಯ makeForkJoin(AstFork* ಧ್ರುವ forkp) {
        // Create a fork sync var
        File_ಸಾಲು* ಧ್ರುವ flp = forkp->fileline();
        // Insert the sync var directly before the fork
        Ast_ಗಂಟು* ಧ್ರುವ ಸೇರಿಸು_Beforep = forkp;
        // Make sure all references to the fork var are contained within a single scope to prevent
        // moving statements accessing it in case of large functions splitting.
        addCLocalScope(flp, ಸೇರಿಸು_Beforep);
        AstVarScope* forkVscp
            = createTemp(flp, forkp->name() + "__sync", getCreateForkSyncDTypep(), ಸೇರಿಸು_Beforep);
        unsigned join_ಎಣಿಕೆ = 0;  // Needed for join counter
        // Add a <fork sync>.done() to each begin
        ಪ್ರತಿ (Ast_ಗಂಟು* beginp = forkp->forksp(); beginp; beginp = beginp->nextp()) {
            addForkOnKill(VN_AS(beginp, Begin), forkVscp);
            addForkDone(VN_AS(beginp, Begin), forkVscp);
            join_ಎಣಿಕೆ++;
        }
        ಯದಿ (forkp->joinType().joinAny()) join_ಎಣಿಕೆ = 1;
        // Set the join counter
        auto* ಧ್ರುವ initp = ನೂತನ AstCMethodHard{flp, ನೂತನ AstVarRef{flp, forkVscp, VAccess::WRITE},
                                               VCMethod::FORK_INIT, ನೂತನ AstConst{flp, join_ಎಣಿಕೆ}};
        initp->dtypeSetVoid();
        addProcessInfo(initp);
        forkp->addHereThisAsNext(initp->makeStmt());
        // Await the join at the end
        auto* ಧ್ರುವ joinp = ನೂತನ AstCMethodHard{flp, ನೂತನ AstVarRef{flp, forkVscp, VAccess::WRITE},
                                               VCMethod::FORK_JOIN};
        joinp->dtypeSetVoid();
        addProcessInfo(joinp);
        addDebugInfo(joinp);
        forkp->addNextHere(ನೂತನ AstCAwait{flp, joinp});
    }

    // `procp` shall be a NodeProcedure/CFunc/Begin and within it vars from `varsp` will be placed.
    // `varsp` vector of vars which shall be localized.
    ಸ್ಥಿರ ಶೂನ್ಯ localizeVars(Ast_ಗಂಟು* ಧ್ರುವ procp, ಧ್ರುವ std::vector<AstVar*>& varsp) {
        UASSERT(procp, "procp is nullptr");
        AstVar* newVarsp = nullptr;
        ಪ್ರತಿ (AstVar* ಧ್ರುವ varp : varsp) {
            varp->funcLocal(ಸತ್ಯ);
            newVarsp = Ast_ಗಂಟು::addNextNull(newVarsp, varp->unlinkFrBack());
        }
        ಯದಿ (Ast_ಗಂಟು_Procedure* ಧ್ರುವ ಗಂಟು_Procp = VN_CAST(procp, ಗಂಟು_Procedure)) {
            ಗಂಟು_Procp->prependStmtsp(newVarsp);
        } ಅನ್ಯಥಾ ಯದಿ (AstCFunc* ಧ್ರುವ cfuncp = VN_CAST(procp, CFunc)) {
            cfuncp->prependVarsp(newVarsp);
        } ಅನ್ಯಥಾ ಯದಿ (AstBegin* ಧ್ರುವ beginp = VN_CAST(procp, Begin)) {
            beginp->prependStmtsp(newVarsp);
        } ಅನ್ಯಥಾ {
            procp->v3fatalSrc(
                procp->prettyNameQ()
                << " is not of an expected type NodeProcedure/CFunc/Begin instead it is: "
                << procp->prettyTypeName());
        }
    }

    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* delayToIntegral(AstDelay* nodep, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* valuep) {
        ಧ್ರುವ AstConst* ಧ್ರುವ constp = VN_CAST(valuep, Const);
        // The fork start sentinel is already in ticks
        ಯದಿ (constp && constp->toUQuad() == std::numeric_limits<uint64_t>::max()) ಪ್ರತ್ಯಾಗಮ valuep;
        ಪ್ರತ್ಯಾಗಮ V3Timing::delayToTicks(valuep, nodep->timeunit());
    }

    // VISITORS
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Module* nodep) override {
        UASSERT_OBJ(!m_classp, nodep, "Module or class under class");
        VL_RESTORER(m_classp);
        m_classp = VN_CAST(nodep, Class);
        VL_RESTORER(m_forkCnt);
        m_forkCnt = 0;
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(AstScope* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        Sen_ಅಭಿವ್ಯಕ್ತಿ_Builder sen_ಅಭಿವ್ಯಕ್ತಿ_Builder{m_scopep};
        {  // Restore m_senExprBuilderp before destroying senExprBuilder
            VL_RESTORER(m_sen_ಅಭಿವ್ಯಕ್ತಿ_Builderp);
            m_sen_ಅಭಿವ್ಯಕ್ತಿ_Builderp = &sen_ಅಭಿವ್ಯಕ್ತಿ_Builder;
            iterateChildren(nodep);
        }
    }
    ಶೂನ್ಯ visit(AstActive* nodep) override {
        VL_RESTORER(m_activep);
        m_activep = nodep;
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Procedure* nodep) override {
        VL_RESTORER(m_procp);
        VL_RESTORER(m_hasProcess);
        m_procp = nodep;
        m_hasProcess = hasFlags(nodep, T_HAS_PROC);
        VL_RESTORER(m_underProcedure);
        m_underProcedure = ಸತ್ಯ;
        iterateChildren(nodep);
        ಯದಿ (hasFlags(nodep, T_SUSPENDEE)) nodep->setSuspendable();
        ಯದಿ (hasFlags(nodep, T_HAS_PROC)) nodep->setNeedProcess();
    }
    ಶೂನ್ಯ visit(AstInitial* nodep) override {
        visit(static_cast<Ast_ಗಂಟು_Procedure*>(nodep));
        ಯದಿ (nodep->needProcess() && !nodep->user1SetOnce()) {
            nodep->addStmtsp(
                ನೂತನ AstCStmt{nodep->fileline(), "vlProcess->state(VlProcess::FINISHED);"});
        }
    }
    ಶೂನ್ಯ visit(AstJumpBlock* nodep) override {
        VL_RESTORER(m_underJumpBlock);
        m_underJumpBlock = ಸತ್ಯ;
        visit(static_cast<Ast_ಗಂಟು_Stmt*>(nodep));
    }
    ಶೂನ್ಯ visit(AstAlways* nodep) override {
        ಯದಿ (nodep->user1SetOnce()) ಪ್ರತ್ಯಾಗಮ;
        VL_RESTORER(m_procp);
        VL_RESTORER(m_hasProcess);
        m_procp = nodep;
        m_hasProcess = hasFlags(nodep, T_HAS_PROC);
        VL_RESTORER(m_underProcedure);
        m_underProcedure = ಸತ್ಯ;
        // Workaround for killing `always` processes (doing that is pretty much UB)
        // TODO: Disallow killing `always` at runtime (throw an error)
        // Skip for combinational blocks; if the body has timing controls
        // iterateChildren will add T_SUSPENDEE, otherwise it would spin.
        ಯದಿ (hasFlags(nodep, T_HAS_PROC)
            && !(m_activep && m_activep->sentreep() && m_activep->sentreep()->hasCombo()))
            addFlags(nodep, T_SUSPENDEE);

        iterateChildren(nodep);
        ಯದಿ (hasFlags(nodep, T_HAS_PROC)) nodep->setNeedProcess();
        ಯದಿ (!hasFlags(nodep, T_SUSPENDEE)) ಪ್ರತ್ಯಾಗಮ;
        nodep->setSuspendable();
        File_ಸಾಲು* ಧ್ರುವ flp = nodep->fileline();
        Ast_Sen_ವೃಕ್ಷ* ಧ್ರುವ sentreep = m_activep->sentreep();
        ಯದಿ (sentreep->hasClocked()) {
            Ast_ಗಂಟು* ಧ್ರುವ bodysp = nodep->stmtsp()->unlinkFrBackWithNext();
            auto* ಧ್ರುವ controlp = ನೂತನ AstEventControl{flp, sentreep->clone_ವೃಕ್ಷ(ಅಸತ್ಯ), bodysp};
            nodep->addStmtsp(controlp);
            iterate(controlp);
        }
        // Note: The 'while (true)' outer loop will be added in V3Sched
        auto* ಧ್ರುವ activep = ನೂತನ AstActive{
            flp, "", ನೂತನ Ast_Sen_ವೃಕ್ಷ{flp, ನೂತನ AstSenItem{flp, AstSenItem::Initial{}}}};
        activep->sen_ವೃಕ್ಷ_Storep(activep->sentreep());
        activep->addStmtsp(nodep->unlinkFrBack());
        m_activep->addNextHere(activep);
    }
    ಶೂನ್ಯ visit(AstCFunc* nodep) override {
        VL_RESTORER(m_procp);
        VL_RESTORER(m_hasProcess);
        VL_RESTORER(m_underIfaceCFunc);
        m_procp = nodep;
        m_hasProcess = hasFlags(nodep, T_HAS_PROC);
        m_underIfaceCFunc = VN_IS(m_scopep->modp(), Iface);
        iterateChildren(nodep);
        ಯದಿ (hasFlags(nodep, T_HAS_PROC)) nodep->setNeedProcess();
        ಯದಿ (!(hasFlags(nodep, T_SUSPENDEE))) ಪ್ರತ್ಯಾಗಮ;

        nodep->rtnType("VlCoroutine");
        // If in a class, create a shared pointer to 'this'
        ಯದಿ (m_classp) {
            AstCStmt* ಧ್ರುವ cstmtp = ನೂತನ AstCStmt{nodep->fileline(), "VL_KEEP_THIS;"};
            nodep->prependStmtsp(cstmtp);
        }
        ಯದಿ (nodep->dpiExportImpl()) {
            nodep->exists([](AstCAwait* ಧ್ರುವ awaitp) -> ತಾರ್ಕಿಕ {
                // A DPI-exported coroutine won't be able to block the calling code
                // Error on the await node; fall back to the function node
                awaitp->v3warn(E_UNSUPPORTED,
                               "Unsupported: Timing controls inside DPI-exported tasks");
                ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
            });
        }
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_CCall* nodep) override {
        AstCFunc* ಧ್ರುವ funcp = nodep->funcp();

        // Skip automatic covergroup sampling calls
        ಯದಿ (funcp->isCovergroupSample()) {
            iterateChildren(nodep);
            ಪ್ರತ್ಯಾಗಮ;
        }

        ಯದಿ (funcp->needProcess()) m_hasProcess = ಸತ್ಯ;
        ಯದಿ (hasFlags(funcp, T_SUSPENDEE) && !nodep->user1SetOnce()) {  // If suspendable
            // Calls to suspendables are always void return type, hence parent must be StmtExpr
            Ast_Stmt_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ stmtp = VN_AS(nodep->backp(), Stmt_ಅಭಿವ್ಯಕ್ತಿ);
            stmtp->replaceWith(ನೂತನ AstCAwait{nodep->fileline(), nodep->unlinkFrBack()});
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(stmtp), stmtp);
        }
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(AstDelay* nodep) override {
        UASSERT_OBJ(!nodep->isCycleDelay(), nodep,
                    "Cycle delays should have been handled in V3AssertPre");
        File_ಸಾಲು* ಧ್ರುವ flp = nodep->fileline();

        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* valuep = delayToIntegral(nodep, nodep->lhsp()->unlinkFrBack());

        // Statistics
        ಯದಿ (valuep->isZero()) {
            ++m_statZeroDelays;
        } ಅನ್ಯಥಾ ಯದಿ (VN_IS(valuep, Const)) {
            ++m_statConstDelays;
        } ಅನ್ಯಥಾ {
            ++m_statVariableDelays;
        }

        // Decide scheduling support for #0
        ಯದಿ (v3Global.opt.schedZeroDelay().isSetTrue()) {
            // User said to schedule for #0 support, nothing else to do
            v3Global.setUsesZeroDelay();
        } ಅನ್ಯಥಾ ಯದಿ (v3Global.opt.schedZeroDelay().isSetFalse()) {
            // User said to schedule without #0 support. Still warn if a static #0 delay exists
            ಯದಿ (valuep->isZero()) {
                nodep->v3warn(
                    ZERODLY,
                    "Static #0 delay exists, but '--no-sched-zero-delay' was given.\n"
                        << nodep->warnMore()  //
                        << "... Can proceed, but this will fail at runtime if executed.");
            }
        } ಅನ್ಯಥಾ {
            // User did not express preference, decide based on presence of delays
            ಯದಿ (valuep->isZero()) {
                // Statically known #0 delay exists, schedule for #0 support
                v3Global.setUsesZeroDelay();
                m_hasStaticZeroDelay = ಸತ್ಯ;
                // Don't warn on variable delays, as no point
                m_unknownDelayFlps.clear();
            } ಅನ್ಯಥಾ ಯದಿ (staticallyNonZeroDelay(valuep)) {
                // Delay is dynamic, but every statically known outcome is non-zero.
                // So we don't need #0 delay support and there should be no warning.
            } ಅನ್ಯಥಾ ಯದಿ (!VN_IS(valuep, Const)) {
                // Delay is not known at compiile time. Conservatively schedule for #0 support,
                // but warn if no static #0 delays used as performance might be improved
                v3Global.setUsesZeroDelay();
                ಯದಿ (!m_hasStaticZeroDelay) m_unknownDelayFlps.ತಳ್ಳು_back(nodep->fileline());
            }
        }

        // Replace self with a 'co_await dlySched.delay(<valuep>)'
        AstCMethodHard* ಧ್ರುವ delayMethodp = ನೂತನ AstCMethodHard{
            flp, ನೂತನ AstVarRef{flp, getCreateDelayScheduler(), VAccess::WRITE},
            VCMethod::SCHED_DELAY, valuep};
        delayMethodp->dtypeSetVoid();
        addProcessInfo(delayMethodp);
        addDebugInfo(delayMethodp);
        // Create the co_await
        Ast_ಗಂಟು* ಧ್ರುವ awaitp = ನೂತನ AstCAwait{flp, delayMethodp, get_Create_Delay_Sen_ವೃಕ್ಷ()};
        // Relink child statements after the co_await
        ಯದಿ (Ast_ಗಂಟು* ಧ್ರುವ stmtsp = nodep->stmtsp()) {
            awaitp->addNext(stmtsp->unlinkFrBackWithNext());
        }
        nodep->replaceWith(awaitp);
        VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
    }
    ಶೂನ್ಯ visit(AstEventControl* nodep) override {
        // Do not allow waiting on local named events, as they get enqueued for clearing, but can
        // go out of scope before that happens
        ಯದಿ (!nodep->sentreep()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Event control with implicit sensitivity (@*)"
                          " in this context; use an explicit sensitivity list instead");
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlinkFrBack()), nodep);
            ಪ್ರತ್ಯಾಗಮ;
        }
        File_ಸಾಲು* ಧ್ರುವ flp = nodep->fileline();
        // Relink child statements after the event control
        ಯದಿ (nodep->stmtsp()) nodep->addNextHere(nodep->stmtsp()->unlinkFrBackWithNext());
        ಯದಿ (needDynamicTrigger(nodep->sentreep())) {
            // Create the trigger variable and init it with 0
            AstVarScope* ಧ್ರುವ trigvscp
                = createTemp(flp, m_dynTrigNames.get(nodep), nodep->findBitDType(), nodep);
            auto* ಧ್ರುವ initp = ನೂತನ AstAssign{flp, ನೂತನ AstVarRef{flp, trigvscp, VAccess::WRITE},
                                              ನೂತನ AstConst{flp, AstConst::BitFalse{}}};
            // Await the eval step with the dynamic trigger scheduler. First, create the method
            // call
            auto* ಧ್ರುವ evalMethodp = ನೂತನ AstCMethodHard{
                flp, ನೂತನ AstVarRef{flp, getCreateDynamicTriggerScheduler(), VAccess::WRITE},
                VCMethod::SCHED_EVALUATION};
            evalMethodp->dtypeSetVoid();
            addProcessInfo(evalMethodp);
            auto* ಧ್ರುವ sentreep = nodep->sentreep();
            addEventDebugInfo(evalMethodp, sentreep);
            // Create the co_await
            AstCAwait* ಧ್ರುವ awaitEvalp
                = ನೂತನ AstCAwait{flp, evalMethodp, get_Create_Dynamic_Trigger_Sen_ವೃಕ್ಷ()};
            // Construct the sen expression for this sentree
            UASSERT_OBJ(m_sen_ಅಭಿವ್ಯಕ್ತಿ_Builderp, nodep, "No SenExprBuilder for this scope");
            auto* ಧ್ರುವ assignp = ನೂತನ AstAssign{flp, ನೂತನ AstVarRef{flp, trigvscp, VAccess::WRITE},
                                                m_sen_ಅಭಿವ್ಯಕ್ತಿ_Builderp->build(sentreep).first};
            // Get the SenExprBuilder results
            ಧ್ರುವ Sen_ಅಭಿವ್ಯಕ್ತಿ_Builder::Results senResults = m_sen_ಅಭಿವ್ಯಕ್ತಿ_Builderp->getAndClearResults();
            localizeVars(m_procp, senResults.m_vars);
            // If post updates are destructive (e.g. clearFired on events), perform a
            // conservative pre-clear once before entering the wait loop so stale state from a
            // previous wait does not cause an immediate false-positive trigger.
            ಧ್ರುವ ತಾರ್ಕಿಕ hasDestructivePostUpdates = !senResults.m_destructivePostUpdates.empty();
            // Create the trigger eval loop, which will await the evaluation step and check the
            // trigger
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ condp
                = ನೂತನ AstLogNot{flp, ನೂತನ AstVarRef{flp, trigvscp, VAccess::READ}};
            AstLoop* ಧ್ರುವ loopp = ನೂತನ AstLoop{flp};
            loopp->addStmtsp(ನೂತನ AstLoopTest{flp, loopp, condp});
            loopp->addStmtsp(awaitEvalp);
            // Put pre updates before the trigger check and assignment
            ಪ್ರತಿ (Ast_ಗಂಟು_Stmt* ಧ್ರುವ stmtp : senResults.m_preUpdates) loopp->addStmtsp(stmtp);
            // Then the trigger check and assignment
            loopp->addStmtsp(assignp);
            // Let the dynamic trigger scheduler know if this trigger was set
            // If it was, a call to the scheduler's evaluate() will return true
            AstCMethodHard* ಧ್ರುವ anyTriggeredMethodp = ನೂತನ AstCMethodHard{
                flp, ನೂತನ AstVarRef{flp, getCreateDynamicTriggerScheduler(), VAccess::WRITE},
                VCMethod::SCHED_ANY_TRIGGERED, ನೂತನ AstVarRef{flp, trigvscp, VAccess::READ}};
            anyTriggeredMethodp->dtypeSetVoid();
            loopp->addStmtsp(anyTriggeredMethodp->makeStmt());
            // If the post update is destructive (e.g. event vars are cleared), create an await for
            // the post update step
            ಯದಿ (hasDestructivePostUpdates) {
                AstCAwait* ಧ್ರುವ awaitPostUpdatep = awaitEvalp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ);
                VN_AS(awaitPostUpdatep->exprp(), CMethodHard)->method(VCMethod::SCHED_POST_UPDATE);
                loopp->addStmtsp(awaitPostUpdatep);
            }
            // Put the post updates at the end of the loop
            ಪ್ರತಿ (Ast_ಗಂಟು_Stmt* ಧ್ರುವ stmtp : senResults.m_postUpdates) loopp->addStmtsp(stmtp);
            // Finally, await the resumption step in 'act'
            AstCAwait* ಧ್ರುವ awaitResumep = awaitEvalp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ);
            VN_AS(awaitResumep->exprp(), CMethodHard)->method(VCMethod::SCHED_RESUMPTION);
            Ast_ಗಂಟು::addNext<Ast_ಗಂಟು_Stmt, Ast_ಗಂಟು_Stmt>(loopp, awaitResumep);
            // Replace the event control with one explicit stmt chain:
            //   init -> [inits] -> [optional destructive pre-clears] -> loop -> awaitResumption
            Ast_ಗಂಟು_Stmt* chainp = nullptr;
            chainp = Ast_ಗಂಟು::addNextNull(chainp, initp);
            ಪ್ರತಿ (Ast_ಗಂಟು_Stmt* ಧ್ರುವ stmtp : senResults.m_inits) {
                chainp = Ast_ಗಂಟು::addNextNull(chainp, stmtp);
            }
            ಯದಿ (hasDestructivePostUpdates) {
                ಪ್ರತಿ (Ast_ಗಂಟು_Stmt* ಧ್ರುವ stmtp : senResults.m_destructivePostUpdates) {
                    chainp = Ast_ಗಂಟು::addNextNull(chainp, stmtp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ));
                }
            }
            chainp = Ast_ಗಂಟು::addNextNull(chainp, loopp);
            nodep->replaceWith(chainp);
        } ಅನ್ಯಥಾ {
            auto* ಧ್ರುವ sentreep = m_finder.get_Sen_ವೃಕ್ಷ(nodep->sentreep());
            nodep->sentreep()->unlinkFrBack()->delete_ವೃಕ್ಷ();
            // Get this sentree's trigger scheduler
            // Replace self with a 'co_await trigSched.trigger()'
            auto* ಧ್ರುವ triggerMethodp = ನೂತನ AstCMethodHard{
                flp, ನೂತನ AstVarRef{flp, getCreateTriggerSchedulerp(sentreep), VAccess::WRITE},
                VCMethod::SCHED_TRIGGER};
            triggerMethodp->dtypeSetVoid();
            // If it should be committed immediately, pass true, otherwise false
            triggerMethodp->addPinsp(nodep->user2() ? ನೂತನ AstConst{flp, AstConst::BitTrue{}}
                                                    : ನೂತನ AstConst{flp, AstConst::BitFalse{}});
            addProcessInfo(triggerMethodp);
            addEventDebugInfo(triggerMethodp, sentreep);
            // Create the co_await
            AstCAwait* ಧ್ರುವ awaitp = ನೂತನ AstCAwait{flp, triggerMethodp, sentreep};
            nodep->replaceWith(awaitp);
        }
        VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Assign* nodep) override {
        // Only process once to avoid infinite loops (due to the net delay)
        ಯದಿ (nodep->user1SetOnce()) ಪ್ರತ್ಯಾಗಮ;
        UINFO(9, "control-visit " << nodep);
        File_ಸಾಲು* ಧ್ರುವ flp = nodep->fileline();
        Ast_ಗಂಟು* controlp = factorOutTimingControl(nodep);
        ಧ್ರುವ ತಾರ್ಕಿಕ inAssignDly = VN_IS(nodep, AssignDly);
        // Handle the intra assignment timing control
        // Transform if:
        // * there's a timing control in the assignment
        // * the assignment is an AssignDly and it's in a non-inlined function
        ಯದಿ (!controlp && (!inAssignDly || m_underProcedure)) {
            iterateChildren(nodep);
            ಪ್ರತ್ಯಾಗಮ;
        }
        // The timing control, if any, which is put in a fork for an NBA
        Ast_ಗಂಟು* ಧ್ರುವ timingp = controlp;
        // Insert new vars before the timing control if we're in a function; in a process we can't
        // do that, as after V3Begin only modules and functions declare variables. These
        // intra-assignment vars will later be passed to forked processes by value.
        Ast_ಗಂಟು* ಸೇರಿಸು_Beforep = m_underProcedure ? nullptr : controlp;
        // Special case for NBA
        ಯದಿ (inAssignDly) {
            // Put it in a fork so it doesn't block
            // Could already be the only thing directly under a fork, reuse that if possible
            AstFork* forkp = !nodep->nextp() ? VN_CAST(nodep->firstAbovep(), Fork) : nullptr;
            ಯದಿ (!forkp) forkp = ನೂತನ AstFork{flp, VJoinType::JOIN_NONE};
            ಯದಿ (!m_underProcedure) {
                // If it's in a function, it won't be handled by V3Delayed
                // Put it behind an additional named event that gets triggered in the NBA region
                AstEventControl* ಧ್ರುವ nbaEventControlp = createNbaEventControl(flp);
                AstAssign* ಧ್ರುವ trigAssignp = createNbaEventTriggerAssignment(flp);
                nodep->replaceWith(trigAssignp);
                trigAssignp->addNextHere(nbaEventControlp);
                nbaEventControlp->addStmtsp(nodep);
                ಸೇರಿಸು_Beforep = forkp;
                ಯದಿ (!controlp) controlp = nbaEventControlp;
            }
            controlp->replaceWith(forkp);
            AstBegin* beginp = VN_CAST(controlp, Begin);
            ಯದಿ (!beginp) beginp = ನೂತನ AstBegin{nodep->fileline(), "", controlp, ಅಸತ್ಯ};
            forkp->addForksp(beginp);
            addFlags(beginp, T_NBA_UPDATE);
            controlp = forkp;
        }
        UASSERT_OBJ(nodep, controlp, "Assignment should have timing control");
        // Move `insertBeforep` into `AstCLocalScope` if necessary to avoid jumping over
        // a variable initialization that would be inserted before `insertBeforep`. All
        // access to this variable should be contained within returned `AstCLocalScope`.
        ಯದಿ (m_underJumpBlock) addCLocalScope(flp, ಸೇರಿಸು_Beforep);
        // Function for replacing values with intermediate variables
        ಧ್ರುವ auto replaceWithIntermediate = [&](Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ valuep,
                                                 ಧ್ರುವ std::string& name) {
            AstVarScope* ಧ್ರುವ newvscp = createTemp(flp, name, valuep->dtypep(), ಸೇರಿಸು_Beforep);
            // A pending update keeps its own value, even if its process schedules more
            ಯದಿ (inAssignDly && !ಸೇರಿಸು_Beforep) newvscp->fork_By_ಮೌಲ್ಯ(ಸತ್ಯ);
            valuep->replaceWith(ನೂತನ AstVarRef{flp, newvscp, VAccess::READ});
            controlp->addHereThisAsNext(
                ನೂತನ AstAssign{flp, ನೂತನ AstVarRef{flp, newvscp, VAccess::WRITE}, valuep});
        };
        // NBAs with delays evaluate the indices selecting the target now (IEEE 1800-2023 10.4.2),
        // in order, each as a whole, also the selects and function calls in it. So follow the
        // selects of the target up from the variable, not into their indices. As an example, this
        // transforms 'x[a[i]][f(j)] <= #1 y' into 't0 = a[i]; t1 = f(j); x[t0][t1] <= #1 y'.
        // V3Const has split NBAs to concatenations, also streamed, into NBAs to each part.
        ಯದಿ (inAssignDly) {
            ಪ್ರತಿ (Ast_ಗಂಟು* exprp = nodep->lhsp()->baseFromp(ಸತ್ಯ); exprp != nodep;
                 exprp = exprp->firstAbovep()) {
                ಯದಿ (AstSel* ಧ್ರುವ selp = VN_CAST(exprp, Sel)) {
                    // widthp should be const
                    ಯದಿ (!VN_IS(selp->lsbp(), Const)) {
                        replaceWithIntermediate(selp->lsbp(), m_intraLsbNames.get(nodep));
                    }
                } ಅನ್ಯಥಾ ಯದಿ (Ast_ಗಂಟು_Sel* ಧ್ರುವ selp = VN_CAST(exprp, ಗಂಟು_Sel)) {
                    ಯದಿ (!VN_IS(selp->bitp(), Const)) {
                        replaceWithIntermediate(selp->bitp(), m_intraIndexNames.get(nodep));
                    }
                }
            }
        }
        // Replace the RHS with an intermediate value var
        replaceWithIntermediate(nodep->rhsp(), m_intra_ಮೌಲ್ಯ_Names.get(nodep));
        ಯದಿ (!inAssignDly) ಪ್ರತ್ಯಾಗಮ;
        // The process executing the NBA evaluates its delay (IEEE 1800-2023 4.9.4), not the forked
        // process, if it has side effects, e.g. calling a function that checks its process. A pure
        // one is evaluated as the forked process starts, at once.
        ಯದಿ (AstDelay* ಧ್ರುವ delayp = VN_CAST(timingp, Delay)) {
            ಯದಿ (!delayp->lhsp()->isPure()) {
                replaceWithIntermediate(delayp->lhsp(), m_intra_ಮೌಲ್ಯ_Names.get(nodep));
            }
        }
    }
    ಶೂನ್ಯ visit(AstFireEvent* nodep) override {
        // V3Delayed handles '->>' of a variable in a process. Like NBAs in non-inlined functions,
        // trigger other events from a fork awaiting the NBA region.
        ಯದಿ (!nodep->isDelayed() || (m_underProcedure && VN_IS(nodep->operandp(), VarRef))) {
            iterateChildren(nodep);
            ಪ್ರತ್ಯಾಗಮ;
        }
        File_ಸಾಲು* ಧ್ರುವ flp = nodep->fileline();
        AstAssign* ಧ್ರುವ trigAssignp = createNbaEventTriggerAssignment(flp);
        nodep->replaceWith(trigAssignp);
        AstFork* ಧ್ರುವ forkp = ನೂತನ AstFork{flp, VJoinType::JOIN_NONE};
        trigAssignp->addNextHere(forkp);
        ಯದಿ (m_underJumpBlock) addCLocalScope(flp, forkp);
        // The triggered event is the one referenced now, so evaluate handles and indices now
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ eventp = nodep->operandp()->unlinkFrBack();
        ಧ್ರುವ auto evalNow = [&](Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ valuep, V3UniqueNames& names) {
            // Like for an NBA, a process uses a module variable, passed to the fork by value
            AstVarScope* ಧ್ರುವ vscp = createTemp(flp, names.get(nodep), valuep->dtypep(),
                                                 m_underProcedure ? nullptr : forkp);
            ಯದಿ (m_underProcedure) vscp->fork_By_ಮೌಲ್ಯ(ಸತ್ಯ);
            valuep->replaceWith(ನೂತನ AstVarRef{flp, vscp, VAccess::READ});
            // Unlike an event, a handle selecting one is only read
            valuep->foreach([](Ast_ಗಂಟು* ಧ್ರುವ np) {
                ಯದಿ (Ast_ಗಂಟು_Var_Ref* ಧ್ರುವ refp = VN_CAST(np, ಗಂಟು_Var_Ref)) {
                    refp->access(VAccess::READ);
                } ಅನ್ಯಥಾ ಯದಿ (AstMemberSel* ಧ್ರುವ selp = VN_CAST(np, MemberSel)) {
                    selp->access(VAccess::READ);
                }
            });
            forkp->addHereThisAsNext(
                ನೂತನ AstAssign{flp, ನೂತನ AstVarRef{flp, vscp, VAccess::WRITE}, valuep});
        };
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* refp = eventp;
        ಯಾವತ್ (Ast_ಗಂಟು_Sel* ಧ್ರುವ selp = VN_CAST(refp, ಗಂಟು_Sel)) {
            ಯದಿ (!VN_IS(selp->bitp(), Const)) evalNow(selp->bitp(), m_intraIndexNames);
            refp = selp->fromp();
        }
        // Handle of a class (or virtual interface)
        ಯದಿ (AstMemberSel* ಧ್ರುವ selp = VN_CAST(refp, MemberSel)) {
            evalNow(selp->fromp(), m_intra_ಮೌಲ್ಯ_Names);
        }
        AstEventControl* ಧ್ರುವ controlp = createNbaEventControl(flp);
        controlp->addStmtsp(ನೂತನ AstFireEvent{flp, eventp, ಅಸತ್ಯ});
        AstBegin* ಧ್ರುವ beginp = ನೂತನ AstBegin{flp, "", controlp, ಅಸತ್ಯ};
        addFlags(beginp, T_NBA_UPDATE);
        forkp->addForksp(beginp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }
    ಶೂನ್ಯ visit(AstAssignW* nodep) override {
        File_ಸಾಲು* ಧ್ರುವ flp = nodep->fileline();
        // Get the net delay unless this assignment was created for handling the net delay (user1)
        AstDelay* ಧ್ರುವ netDelayp = nodep->user1() ? nullptr : nodep->getLhsNetDelay();
        ಯದಿ (netDelayp) {
            ಯದಿ (nodep->timingControlp()) {
                // If this assignment has a delay, create another one to handle the net delay
                AstVarScope* ಧ್ರುವ newvscp = m_assignWTmps.make(flp, m_scopep, nodep->dtypep());
                AstAssignW* assignp = ನೂತನ AstAssignW{
                    nodep->fileline(), nodep->lhsp()->unlinkFrBack(),
                    ನೂತನ AstVarRef{flp, newvscp, VAccess::READ}, netDelayp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ)};
                assignp->user1(ಸತ್ಯ);
                nodep->addNextHere(assignp);
                nodep->lhsp(ನೂತನ AstVarRef{flp, newvscp, VAccess::WRITE});
            } ಅನ್ಯಥಾ {
                // Else just use this one with the net delay
                nodep->timingControlp(netDelayp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ));
            }
        }
        ಯದಿ (!nodep->timingControlp()) ಪ್ರತ್ಯಾಗಮ;
        // There will be some circular logic here, suppress the warning for newly created vars
        flp->modifyWarnOff(V3ErrorCode::UNOPTFLAT, ಸತ್ಯ);
        // Also suppress the warning for the LHS var, for cases like `assign #1 clk = ~clk;`
        // TODO: Restore the warning for other, non-delayed drivers
        nodep->lhsp()->foreach([](AstVarRef* refp) {
            ಯದಿ (refp->access().isWriteOrRW()) {
                refp->varp()->fileline()->modifyWarnOff(V3ErrorCode::UNOPTFLAT, ಸತ್ಯ);
            }
        });
        // Should be under an always
        AstAlways* ಧ್ರುವ alwaysp = VN_AS(m_procp, Always);
        // Convert it to an Assign; the new assign with intra delay will be handled by
        // visit(AstNodeAssign*)
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ lhs1p = nodep->lhsp()->unlinkFrBack();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ rhs1p = nodep->rhsp()->unlinkFrBack();
        Ast_ಗಂಟು* ಧ್ರುವ controlp = nodep->timingControlp()->unlinkFrBack();
        ಯದಿ (AstDelay* ಧ್ರುವ delayp = VN_CAST(controlp, Delay)) {
            ಯದಿ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* fallDelayp = delayp->fallDelay()) {
                fallDelayp = fallDelayp->unlinkFrBack();
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp = delayp->lhsp()->unlinkFrBack();
                // Use fall only for an all-zero value, rise otherwise.
                lhsp = delayToIntegral(delayp, lhsp);
                fallDelayp = delayToIntegral(delayp, fallDelayp);
                delayp->lhsp(
                    ನೂತನ AstCond{flp, ನೂತನ AstEq{flp, rhs1p->clone_ವೃಕ್ಷ(ಅಸತ್ಯ), ನೂತನ AstConst{flp, 0}},
                                fallDelayp, lhsp});
            }
        }
        AstAssign* ಧ್ರುವ assignp = ನೂತನ AstAssign{nodep->fileline(), lhs1p, rhs1p, controlp};
        // Put the assignment in a fork..join_none.
        AstFork* ಧ್ರುವ forkp = ನೂತನ AstFork{flp, VJoinType::JOIN_NONE};
        nodep->replaceWith(forkp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        AstBegin* ಧ್ರುವ beginp = ನೂತನ AstBegin{flp, "", assignp, ಅಸತ್ಯ};
        forkp->addForksp(beginp);
        visit(forkp);  // Visit now as we need to do some post-processing
        // IEEE 1800-2023 10.3.3 - if the RHS value differs from the currently scheduled value to
        // be assigned, the currently scheduled assignment is descheduled. To keep track if an
        // assignment should be descheduled, each scheduled assignment event has a 'generation',
        // and if at assignment time its generation differs from the current generation, it won't
        // be performed
        AstAssign* ಧ್ರುವ preAssignp = VN_AS(beginp->stmtsp(), Assign);
        UASSERT_OBJ(preAssignp, alwaysp, "Pre-assign should be there from visit(AstFork)");
        AstAssign* ಧ್ರುವ postAssignp = VN_AS(preAssignp->nextp()->nextp(), Assign);
        UASSERT_OBJ(postAssignp, alwaysp, "Post-assign should be there from visit(AstFork)");
        // Increment generation and copy it to a local
        AstVarScope* ಧ್ರುವ generationVarp
            = m_assignWGens.make(flp, m_scopep, alwaysp->findUInt64DType());
        AstVarScope* ಧ್ರುವ genLocalVarp
            = createTemp(flp, generationVarp->varp()->name() + "__local",
                         alwaysp->findUInt64DType(), preAssignp);
        preAssignp->addHereThisAsNext(
            ನೂತನ AstAssign{flp, ನೂತನ AstVarRef{flp, generationVarp, VAccess::WRITE},
                          ನೂತನ AstAdd{flp, ನೂತನ AstVarRef{flp, generationVarp, VAccess::READ},
                                     ನೂತನ AstConst{flp, 1}}});
        preAssignp->addHereThisAsNext(
            ನೂತನ AstAssign{flp, ನೂತನ AstVarRef{flp, genLocalVarp, VAccess::WRITE},
                          ನೂತನ AstVarRef{flp, generationVarp, VAccess::READ}});
        // If the current generation is same as the one saved in the local var, assign
        beginp->addStmtsp(
            ನೂತನ AstIf{flp,
                      ನೂತನ AstEq{flp, ನೂತನ AstVarRef{flp, generationVarp, VAccess::READ},
                                ನೂತನ AstVarRef{flp, genLocalVarp, VAccess::READ}},
                      postAssignp->unlinkFrBack()});
        // Save scheduled RHS value before delay
        AstVarScope* ಧ್ರುವ tmpVarp
            = m_assignWTmps.make(flp, m_scopep, preAssignp->rhsp()->dtypep());
        AstVarRef* ಧ್ರುವ tmpAssignRhsp = VN_AS(preAssignp->lhsp(), VarRef)->clone_ವೃಕ್ಷ(ಅಸತ್ಯ);
        tmpAssignRhsp->access(VAccess::WRITE);
        preAssignp->addNextHere(
            ನೂತನ AstAssign{flp, ನೂತನ AstVarRef{flp, tmpVarp, VAccess::WRITE}, tmpAssignRhsp});
        // If the RHS is different from the currently scheduled value, schedule the new assignment
        // The generation will increase, effectively 'descheduling' the previous assignment.
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ didNotInitp
            = ನೂತನ AstLogNot{flp, ನೂತನ AstCExpr{flp, "vlSymsp->__Vm_didInit", 1}};
        AstVarRef* ಧ್ರುವ firstIterp
            = ನೂತನ AstVarRef{flp, m_netlistp->stlFirstIterationp(), VAccess::READ};
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ schedCondp
            = ನೂತನ AstLogOr{flp,
                           ನೂತನ AstNeq{flp, preAssignp->rhsp()->clone_ವೃಕ್ಷ(ಅಸತ್ಯ),
                                      ನೂತನ AstVarRef{flp, tmpVarp, VAccess::READ}},
                           ನೂತನ AstLogAnd{flp, didNotInitp, firstIterp}};
        alwaysp->addStmtsp(ನೂತನ AstIf{flp, schedCondp, forkp->unlinkFrBack()});
    }
    ಶೂನ್ಯ visit(AstDisableFork* nodep) override {
        ಯದಿ (m_hasProcess) ಪ್ರತ್ಯಾಗಮ;
        // never reached by any process; remove to avoid compilation error
        VL_DO_DANGLING(nodep->unlinkFrBack()->delete_ವೃಕ್ಷ(), nodep);
    }
    ಶೂನ್ಯ visit(AstWaitFork* nodep) override {
        ಯದಿ (m_hasProcess) {
            File_ಸಾಲು* ಧ್ರುವ flp = nodep->fileline();
            AstCExpr* ಧ್ರುವ exprp = ನೂತನ AstCExpr{flp, "vlProcess->completedFork()", 1};
            AstWait* ಧ್ರುವ waitp = ನೂತನ AstWait{flp, exprp, nullptr};
            nodep->replaceWith(waitp);
        } ಅನ್ಯಥಾ {
            // never reached by any process; remove to avoid compilation error
            nodep->unlinkFrBack();
        }
        VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
    }
    ಶೂನ್ಯ visit(AstWait* nodep) override {
        // Wait on changed events related to the vars in the wait statement
        UINFO(9, "control-visit " << nodep);
        File_ಸಾಲು* ಧ್ರುವ flp = nodep->fileline();
        Ast_ಗಂಟು* ಧ್ರುವ stmtsp = nodep->stmtsp();
        ಯದಿ (stmtsp) stmtsp->unlinkFrBackWithNext();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ condp = V3Const::constifyEdit(nodep->condp()->unlinkFrBack());
        auto* ಧ್ರುವ constp = VN_CAST(condp, Const);
        ಯದಿ (constp) {
            ಯದಿ (constp->isZero()) {
                // We have to await forever instead of simply returning in case we're deep in a
                // callstack
                AstCMethodHard* ಧ್ರುವ foreverMethodp = ನೂತನ AstCMethodHard{
                    flp, ನೂತನ AstVarRef{flp, getCreateDelayScheduler(), VAccess::WRITE},
                    VCMethod::SCHED_WAIT_FOREVER};
                foreverMethodp->dtypeSetVoid();
                addProcessInfo(foreverMethodp);
                addDebugInfo(foreverMethodp);
                AstCAwait* ಧ್ರುವ awaitp = ನೂತನ AstCAwait{flp, foreverMethodp};
                nodep->replaceWith(awaitp);
                ಯದಿ (stmtsp) VL_DO_DANGLING(stmtsp->delete_ವೃಕ್ಷ(), stmtsp);
                VL_DO_DANGLING(condp->delete_ವೃಕ್ಷ(), condp);
            } ಅನ್ಯಥಾ {
                nodep->v3fatalSrc("constant wait should have been removed in "
                                  "TimingSuspendableVisitor::visit(AstWait)");
            }
        } ಅನ್ಯಥಾ ಯದಿ (needDynamicTrigger(condp)) {
            // No point in making a sentree, just use the expression as sensitivity
            // Put the event control in an if so we only wait if the condition isn't met already
            auto* ಧ್ರುವ ifp = ನೂತನ AstIf{
                flp, ನೂತನ AstLogNot{flp, condp},
                ನೂತನ AstEventControl{flp,
                                    ನೂತನ Ast_Sen_ವೃಕ್ಷ{flp, ನೂತನ AstSenItem{flp, VEdgeType::ET_TRUE,
                                                                       condp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ)}},
                                    nullptr}};
            ಯದಿ (stmtsp) Ast_ಗಂಟು::addNext<Ast_ಗಂಟು, Ast_ಗಂಟು>(ifp, stmtsp);
            nodep->replaceWith(ifp);
        } ಅನ್ಯಥಾ {
            // We are using a global sentree, so we cannot use ET_TRUE, as that could lead to the
            // active region never converging. Because of this, we need to use ET_CHANGED in a
            // loop.
            AstEventControl* ಧ್ರುವ controlp = ನೂತನ AstEventControl{
                flp,
                ನೂತನ Ast_Sen_ವೃಕ್ಷ{
                    flp, ನೂತನ AstSenItem{flp, VEdgeType::ET_CHANGED, condp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ)}},
                nullptr};
            controlp->user2(ಸತ್ಯ);  // Commit immediately
            AstLoop* ಧ್ರುವ loopp = ನೂತನ AstLoop{flp};
            loopp->addStmtsp(ನೂತನ AstLoopTest{flp, loopp, ನೂತನ AstLogNot{flp, condp}});
            loopp->addStmtsp(controlp);
            ಯದಿ (stmtsp) Ast_ಗಂಟು::addNext<Ast_ಗಂಟು, Ast_ಗಂಟು>(loopp, stmtsp);
            nodep->replaceWith(loopp);
        }
        VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
    }
    ಶೂನ್ಯ visit(AstBegin* nodep) override {
        VL_RESTORER(m_procp);
        VL_RESTORER(m_hasProcess);
        m_hasProcess |= hasFlags(nodep, T_HAS_PROC);
        // A pending NBA update is not a subprocess of the process that scheduled it, so it is
        // unaffected by 'disable fork' and 'wait fork' (IEEE 1800-2023 9.6.1, 9.6.3)
        ಯದಿ (hasFlags(nodep, T_NBA_UPDATE)) m_hasProcess = ಅಸತ್ಯ;
        m_procp = nodep;
        ಯದಿ (m_hasProcess) nodep->setNeedProcess();
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(AstFork* nodep) override {
        ಯದಿ (nodep->user1SetOnce()) ಪ್ರತ್ಯಾಗಮ;
        UINFO(9, "control-visit " << nodep);
        v3Global.setUsesTiming();

        // Create a unique name for this fork
        nodep->name("__Vfork_" + std::to_string(++m_forkCnt));

        // TODO: Should process nodep->stmtsp() in case an earlier pass
        //       inserted something there, but as of today we don't need to.

        // Process and name each fork
        size_t idx = 0;  // Index for naming begins
        ಪ್ರತಿ (AstBegin *itemp = nodep->forksp(), *nextp; itemp; itemp = nextp) {
            nextp = VN_AS(itemp->nextp(), Begin);
            iterate(itemp);
            // Note: Even if we do not find any awaits, we cannot simply inline
            // the process here, as new awaits could be added later.

            // Name the begin (later the name will be used for a new function)
            itemp->name(nodep->name() + "__" + std::to_string(idx++));
            ಯದಿ (itemp->needProcess()) {
                itemp->addStmtsp(
                    ನೂತನ AstCStmt{itemp->fileline(), "vlProcess->state(VlProcess::FINISHED);"});
            }
        }
        ಯದಿ (!nodep->joinType().joinNone()) makeForkJoin(nodep);
    }

    //--------------------
    ಶೂನ್ಯ visit(AstVar*) override {}  // Accelerate
    ಶೂನ್ಯ visit(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    explicit TimingControlVisitor(AstNetlist* nodep)
        : m_netlistp{nodep} {
        iterate(nodep);

        // If there is no static #0 in the design, but an unknown delay was found,
        // and the user did not specify preference, then warn on all unknown delays
        // as we will be assuming they can be #0, which can cause performance degradation.
        ಪ್ರತಿ (File_ಸಾಲು* ಧ್ರುವ flp : m_unknownDelayFlps) {
            flp->v3warn(ZERODLY,
                        "Value of # delay control statically unknown. Assuming it can be #0.\n"
                            << flp->warnMore()  //
                            << "... If all # delays are non-zero at runtime,\n"
                            << flp->warnMore()  //
                            << "... use '--no-sched-zero-delay' for improved performance.\n"
                            << flp->warnMore()  //
                            << "... If a real #0 is expected at runtime,\n"
                            << flp->warnMore()  //
                            << "... use '--sched-zero-delay' to suppress this warning.");
        }
    }
    ~TimingControlVisitor() override {
        V3Stats::addStat("Timing, known #0 delays", m_statZeroDelays);
        V3Stats::addStat("Timing, known #const delays", m_statConstDelays);
        V3Stats::addStat("Timing, unknown #variable delays", m_statVariableDelays);
    }
};

//######################################################################
// Timing class functions

Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* V3Timing::delayToTicks(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* valuep, VTimescale timeunit) {
    File_ಸಾಲು* ಧ್ರುವ flp = valuep->fileline();
    ಯದಿ (ಧ್ರುವ AstConst* ಧ್ರುವ constp = VN_CAST(valuep, Const)) {
        ಯದಿ (constp->ಅಂಕೆ().is1Step()) {
            // A step is one tick of the global time precision (IEEE 1800-2023 3.14.3)
            VL_DO_DANGLING(valuep->delete_ವೃಕ್ಷ(), valuep);
            AstConst* ಧ್ರುವ tickp = ನೂತನ AstConst{flp, AstConst::Unsized64{}, 1};
            tickp->dtypeSetBitSized(64, VSigning::UNSIGNED);
            ಪ್ರತ್ಯಾಗಮ tickp;
        }
        ಯದಿ (constp->isZero()) ಪ್ರತ್ಯಾಗಮ valuep;
    }
    UASSERT_OBJ(!timeunit.isNone(), valuep, "timeunit must be set");
    ಧ್ರುವ ದ್ವಿಗುಣ timescaleFactorD
        = std::pow(10.0, timeunit.powerOfTen() - v3Global.rootp()->timeprecision().powerOfTen());
    ಯದಿ (valuep->dtypep()->skipRefp()->isDouble()) {
        AstConst* ಧ್ರುವ tsfp = ನೂತನ AstConst{flp, AstConst::RealDouble{}, timescaleFactorD};
        valuep = ನೂತನ AstMulD{flp, valuep, tsfp};
        valuep = ನೂತನ AstRToIRoundS{flp, valuep};
        valuep->dtypeSetBitSized(64, VSigning::UNSIGNED);
    } ಅನ್ಯಥಾ {
        valuep->dtypeSetBitSized(64, VSigning::UNSIGNED);
        ಧ್ರುವ uint64_t timescaleFactorU = static_cast<uint64_t>(timescaleFactorD);
        AstConst* ಧ್ರುವ tsfp = ನೂತನ AstConst{flp, AstConst::Unsized64{}, timescaleFactorU};
        valuep = ನೂತನ AstMul{flp, valuep, tsfp};
    }
    ಪ್ರತ್ಯಾಗಮ V3Const::constifyEdit(valuep);  // Simplify
}

ಶೂನ್ಯ V3Timing::timingAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        ಧ್ರುವ VNUser1InUse m_user1InUse;
        ಧ್ರುವ VNUser2InUse m_user2InUse;
        { TimingSuspendableVisitor{nodep}; }
        ಯದಿ (v3Global.usesTiming()) TimingControlVisitor{nodep};
    }
    V3Global::dump_Check_Global_ವೃಕ್ಷ("timing", 0, dump_ವೃಕ್ಷ_Either_Level() >= 3);
}
