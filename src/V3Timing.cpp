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

#અન્તર્ભાવ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#અન્તર્ભાવ "V3Timing.h"

#અન્તર્ભાવ "V3Const.h"
#અન્તર્ભાવ "V3EmitV.h"
#અન્તર્ભાવ "V3File.h"
#અન્તર્ભાવ "V3Global.h"
#અન્તર્ભાવ "V3Graph.h"
#અન્તર્ભાવ "V3MemberMap.h"
#અન્તર્ભાવ "V3SenExprBuilder.h"
#અન્તર્ભાવ "V3SenTree.h"
#અન્તર્ભાવ "V3SharedTmps.h"
#અન્તર્ભાવ "V3Stats.h"
#અન્તર્ભાવ "V3UniqueNames.h"

#અન્તર્ભાવ <limits>
#અન્તર્ભાવ <queue>

VL_DEFINE_DEBUG_FUNCTIONS;

// ######################################################################

ગણના ગાંઠ_Flag : uint8_t {
    T_SUSPENDEE = 1 << 0,  // Suspendable (due to dependence on another suspendable)
    T_SUSPENDER = 1 << 1,  // Suspendable (has timing control)
    T_ALLOCS_PROC = 1 << 2,  // Can allocate VlProcess
    T_FORCES_PROC = 1 << 3,  // Forces VlProcess allocation
    T_NEEDS_PROC = 1 << 4,  // Needs access to VlProcess if it's allocated
    T_HAS_PROC = 1 << 5,  // Has VlProcess argument in the signature
    T_NBA_UPDATE = 1 << 6,  // Fork branch of a pending NBA update, which is not a subprocess
};

ગણના ForkType : uint8_t {
    F_NONE = 0,  // Not under a fork
    F_MIGHT_SUSPEND = 1 << 0,  // Fork might suspend the execution of current process
    F_MIGHT_NEED_PROC = 1 << 1,  // Fork might need a process (any fork really)
};

ગણના PropagationType : uint8_t {
    P_CALL = 1,  // Propagation through call to a function/task/method
    P_FORK = 2,  // Propagation due to fork's behavior
    P_SIGNATURE = 3,  // Propagation required to maintain C++ function's signature requirements
};

// Add timing flag to a node
સ્થિર શૂન્ય addFlags(Ast_ગાંઠ* અચલ nodep, uint8_t flags) { nodep->user2Or(flags); }
// Check if a node has ALL of the expected flags set
સ્થિર તાર્કિક hasFlags(Ast_ગાંઠ* અચલ nodep, uint8_t flags) { પાછા !(~nodep->user2() & flags); }

// ######################################################################
//  Detect nodes affected by timing and/or requiring a process

વર્ગ TimingSuspendableVisitor final : જાહેર VNVisitor {
    // TYPES
    // Vertex of a dependency graph of suspendable nodes, e.g. if a node (process or task) is
    // suspendable, all its dependents should also be suspendable
    વર્ગ DepVtx VL_NOT_FINAL : જાહેર V3GraphVertex {
        VL_RTTI_IMPL(DepVtx, V3GraphVertex)
        AstClass* અચલ m_classp;  // Class associated with a method
        Ast_ગાંઠ* અચલ m_nodep;  // AST node represented by this graph vertex

        // ACCESSORS
        string name() અચલ override {
            જો (m_classp) {
                જો (VN_IS(nodep(), CFunc)) {
                    પાછા cvtToHex(nodep()) + ' ' + classp()->name() + "::" + nodep()->name();
                }
            }
            પાછા cvtToHex(nodep()) + ' ' + nodep()->prettyTypeName();
        }
        File_લીટી* fileline() અચલ override { પાછા nodep()->fileline(); }

    જાહેર:
        // CONSTRUCTORS
        DepVtx(V3Graph* graphp, Ast_ગાંઠ* nodep, AstClass* classp)
            : V3GraphVertex{graphp}
            , m_classp{classp}
            , m_nodep{nodep} {}
        ~DepVtx() override = મૂળભૂત;

        // ACCESSORS
        આભાસી Ast_ગાંઠ* nodep() અચલ VL_MT_STABLE { પાછા m_nodep; }
        આભાસી Ast_ગાંઠ* classp() અચલ VL_MT_STABLE { પાછા m_classp; }
    };

    વર્ગ SuspendDepVtx final : જાહેર DepVtx {
        VL_RTTI_IMPL(SuspendDepVtx, DepVtx)
        string dotColor() અચલ override {
            જો (hasFlags(nodep(), T_SUSPENDER)) પાછા "red";
            જો (hasFlags(nodep(), T_SUSPENDEE)) પાછા "blue";
            પાછા "black";
        }

    જાહેર:
        SuspendDepVtx(V3Graph* graphp, Ast_ગાંઠ* nodep, AstClass* classp)
            : DepVtx{graphp, nodep, classp} {}
        ~SuspendDepVtx() override = મૂળભૂત;
    };

    વર્ગ NeedsProcDepVtx final : જાહેર DepVtx {
        VL_RTTI_IMPL(NeedsProcDepVtx, DepVtx)
        string dotColor() અચલ override {
            જો (hasFlags(nodep(), T_HAS_PROC)) પાછા "blue";
            જો (hasFlags(nodep(), T_NEEDS_PROC)) પાછા "green";
            જો (hasFlags(nodep(), T_FORCES_PROC)) પાછા "red";
            પાછા "black";
        }

    જાહેર:
        NeedsProcDepVtx(V3Graph* graphp, Ast_ગાંઠ* nodep, AstClass* classp)
            : DepVtx{graphp, nodep, classp} {}
        ~NeedsProcDepVtx() override = મૂળભૂત;
    };

    // NODE STATE
    //  AstClass::user1() -> bool.  Class member cache has been refreshed.
    //  Ast{NodeProcedure,CFunc,Begin}::user2()  -> uint64_t.  Set to >= T_SUSP if
    //                                                         process/task suspendable
    //                                                         and to T_PROC if it
    //                                                         needs process metadata.
    //  Ast{NodeProcedure,CFunc,Begin}::user3()  -> DependencyVertex*.  Vertex in m_suspGraph
    //  Ast{NodeProcedure,CFunc,Begin}::user3()  -> DependencyVertex*.  Vertex in m_procGraph
    અચલ VNUser3InUse m_user3InUse;
    અચલ VNUser4InUse m_user4InUse;

    // STATE
    VMemberMap m_memberMap;  // Member names cached for fast lookup
    AstClass* m_classp = nullptr;  // Current class
    Ast_ગાંઠ* m_procp = nullptr;  // NodeProcedure/CFunc/Begin we're under
    uint8_t m_underFork = F_NONE;  // F_NONE or flags of a fork we are under
    V3Graph m_suspGraph;  // Dependency graph where a node is a dependency of another if it being
                          // suspendable makes the other node suspendable
    V3Graph m_procGraph;  // Dependency graph where a node is a dependency of another if it being
                          // suspendable makes the other node suspendable

    // METHODS
    // Get or create the dependency vertex for the given node
    DepVtx* getSuspendDepVtx(Ast_ગાંઠ* અચલ nodep) {
        AstClass* classp = nullptr;
        જો (AstCFunc* funcp = VN_CAST(nodep, CFunc)) {
            જો (funcp->scopep() && funcp->scopep()->modp()) {
                classp = VN_CAST(funcp->scopep()->modp(), Class);
            }
        }
        જો (!nodep->user3p()) nodep->user3p(નવું SuspendDepVtx{&m_suspGraph, nodep, classp});
        પાછા nodep->user3u().to<SuspendDepVtx*>();
    }
    DepVtx* getNeedsProcDepVtx(Ast_ગાંઠ* અચલ nodep) {
        AstClass* classp = nullptr;
        જો (AstCFunc* funcp = VN_CAST(nodep, CFunc)) {
            જો (funcp->scopep() && funcp->scopep()->modp()) {
                classp = VN_CAST(funcp->scopep()->modp(), Class);
            }
        }
        જો (!nodep->user4p()) nodep->user4p(નવું NeedsProcDepVtx{&m_procGraph, nodep, classp});
        પાછા nodep->user4u().to<NeedsProcDepVtx*>();
    }
    // Pass timing flag between nodes
    તાર્કિક passFlag(અચલ Ast_ગાંઠ* from, Ast_ગાંઠ* to, ગાંઠ_Flag flag) {
        જો ((from->user2() & flag) && !(to->user2() & flag)) {
            addFlags(to, flag);
            પાછા સાચું;
        }
        પાછા ખોટું;
    }
    // Propagate flag to all nodes that depend on the given one
    શૂન્ય propagateFlags(DepVtx* અચલ vxp, ગાંઠ_Flag flag) {
        auto* અચલ parentp = vxp->nodep();
        માટે (V3GraphEdge& edge : vxp->outEdges()) {
            auto* અચલ depVxp = static_cast<DepVtx*>(edge.ટોચ());
            Ast_ગાંઠ* અચલ depp = depVxp->nodep();
            જો (passFlag(parentp, depp, flag)) propagateFlags(depVxp, flag);
        }
    }
    template <typename T_Predicate>
    શૂન્ય propagateFlagsIf(DepVtx* અચલ vxp, ગાંઠ_Flag flag, T_Predicate p) {
        auto* અચલ parentp = vxp->nodep();
        માટે (V3GraphEdge& edge : vxp->outEdges()) {
            auto* અચલ depVxp = static_cast<DepVtx*>(edge.ટોચ());
            Ast_ગાંઠ* અચલ depp = depVxp->nodep();
            જો (p(&edge) && passFlag(parentp, depp, flag)) propagateFlagsIf(depVxp, flag, p);
        }
    }
    template <typename T_Predicate>
    શૂન્ય propagateFlagsReversedIf(DepVtx* અચલ vxp, ગાંઠ_Flag flag, T_Predicate p) {
        auto* અચલ parentp = vxp->nodep();
        માટે (V3GraphEdge& edge : vxp->inEdges()) {
            auto* અચલ depVxp = static_cast<DepVtx*>(edge.fromp());
            Ast_ગાંઠ* અચલ depp = depVxp->nodep();
            જો (p(&edge) && passFlag(parentp, depp, flag))
                propagateFlagsReversedIf(depVxp, flag, p);
        }
    }

    // VISITORS
    શૂન્ય visit(AstClass* nodep) override {
        UASSERT_OBJ(!m_classp, nodep, "Class under class");
        VL_RESTORER(m_classp);
        m_classp = nodep;
        iterateChildren(nodep);
    }
    શૂન્ય visit(Ast_ગાંઠ_Procedure* nodep) override {
        VL_RESTORER(m_procp);
        m_procp = nodep;
        getNeedsProcDepVtx(nodep);
        addFlags(nodep, T_ALLOCS_PROC);
        જો (VN_IS(nodep, Always)) {
            UINFO(9, "Always does " << (nodep->needProcess() ? "" : "NOT ") << "need process");
        }
        iterateChildren(nodep);
    }
    શૂન્ય visit(AstDisableFork* nodep) override {
        visit(static_cast<Ast_ગાંઠ*>(nodep));
        addFlags(m_procp, T_FORCES_PROC | T_NEEDS_PROC);
    }
    શૂન્ય visit(AstWaitFork* nodep) override {
        visit(static_cast<Ast_ગાંઠ*>(nodep));
        addFlags(m_procp, T_FORCES_PROC | T_NEEDS_PROC);
    }
    શૂન્ય visit(AstWait* nodep) override {
        UINFO(9, "suspendable-visit " << nodep);
        Ast_ગાંઠ_પદાવલિ* અચલ condp = V3Const::constifyEdit(nodep->condp());
        જો (AstConst* અચલ constp = VN_CAST(condp, Const)) {
            જો (!nodep->fileline()->warnIsOff(V3ErrorCode::WAITCONST)) {
                condp->v3warn(WAITCONST, "Wait statement condition is constant");
            }
            જો (!constp->isZero()) {
                // Remove AstWait before we track process as T_SUSPENDER
                જો (Ast_ગાંઠ* અચલ stmtsp = nodep->stmtsp()) {
                    stmtsp->unlinkFrBackWithNext();
                    nodep->replaceWith(stmtsp);
                } નહીંતર {
                    nodep->unlinkFrBack();
                }
                VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
                પાછા;
            }
        }
        v3Global.setUsesTiming();
        જો (m_procp) addFlags(m_procp, T_SUSPENDEE | T_SUSPENDER | T_NEEDS_PROC);
        iterateChildren(nodep);
    }
    શૂન્ય visit(AstCFunc* nodep) override {
        VL_RESTORER(m_procp);
        m_procp = nodep;
        iterateChildren(nodep);
        જો (nodep->needProcess()) addFlags(nodep, T_FORCES_PROC | T_NEEDS_PROC);
        DepVtx* અચલ sVxp = getSuspendDepVtx(nodep);
        DepVtx* અચલ pVxp = getNeedsProcDepVtx(nodep);
        જો (!m_classp) પાછા;

        // Go over overridden functions

        std::queue<AstClassExtends*> extends;
        જો (m_classp->extendsp()) extends.ધકેલો(m_classp->extendsp());

        જ્યારે (!extends.empty()) {
            AstClassExtends* ext_list = extends.front();
            extends.કાઢો();

            માટે (AstClassExtends* cextp = ext_list; cextp;
                 cextp = VN_AS(cextp->nextp(), ClassExtends)) {
                // TODO: It is possible that a methods the same name in the base class is not
                // actually overridden by our method. If this causes a problem, traverse to
                // the root of the inheritance hierarchy and check if the original method is
                // virtual or not.
                જો (auto* અચલ overriddenp
                    = VN_CAST(m_memberMap.findMember(cextp->classp(), nodep->name()), CFunc)) {
                    // Suspendability and process affects typing, so they propagate both ways
                    DepVtx* અચલ overriddenSVxp = getSuspendDepVtx(overriddenp);
                    DepVtx* અચલ overriddenPVxp = getNeedsProcDepVtx(overriddenp);
                    નવું V3GraphEdge{&m_suspGraph, sVxp, overriddenSVxp, P_SIGNATURE};
                    નવું V3GraphEdge{&m_suspGraph, overriddenSVxp, sVxp, P_SIGNATURE};
                    નવું V3GraphEdge{&m_procGraph, pVxp, overriddenPVxp, P_SIGNATURE};
                    નવું V3GraphEdge{&m_procGraph, overriddenPVxp, pVxp, P_SIGNATURE};
                } નહીંતર {
                    AstClassExtends* more_extends = cextp->classp()->extendsp();
                    જો (more_extends) extends.ધકેલો(more_extends);
                }
            }
        }
    }
    શૂન્ય visit(Ast_ગાંઠ_CCall* nodep) override {
        AstCFunc* અચલ funcp = nodep->funcp();
        UASSERT_OBJ(funcp, nodep, "AstNodeCCall must have non-null funcp post-link");
        UASSERT_OBJ(m_procp, nodep, "AstNodeCCall must be inside a procedure/CFunc/Begin");

        UINFO(9, "V3Timing: Processing CCall to " << funcp->name() << " in dependency graph\n");
        નવું V3GraphEdge{&m_suspGraph, getSuspendDepVtx(funcp), getSuspendDepVtx(m_procp), P_CALL};

        નવું V3GraphEdge{&m_procGraph, getNeedsProcDepVtx(funcp), getNeedsProcDepVtx(m_procp),
                        P_CALL};

        iterateChildren(nodep);
    }
    શૂન્ય visit(AstBegin* nodep) override {
        VL_RESTORER(m_procp);
        VL_RESTORER(m_underFork);

        જો (!m_underFork || (m_underFork & F_MIGHT_SUSPEND))
            નવું V3GraphEdge{&m_suspGraph, getSuspendDepVtx(nodep), getSuspendDepVtx(m_procp),
                            m_underFork ? P_FORK : P_CALL};

        નવું V3GraphEdge{&m_procGraph, getNeedsProcDepVtx(nodep), getNeedsProcDepVtx(m_procp),
                        P_CALL};

        જો (m_underFork) addFlags(nodep, T_NEEDS_PROC | T_ALLOCS_PROC);

        m_procp = nodep;
        m_underFork = 0;
        iterateChildren(nodep);
    }
    શૂન્ય visit(AstFork* nodep) override {
        UINFO(9, "suspendable-visit " << nodep);
        VL_RESTORER(m_underFork);

        v3Global.setUsesTiming();  // Even if there are no event controls, we have to set this flag
                                   // so that transformForks() in V3SchedTiming gets called and
                                   // removes all forks and begins
        જો (nodep->isTimingControl() && m_procp) {
            addFlags(m_procp, T_SUSPENDEE | T_SUSPENDER);
            m_underFork |= F_MIGHT_SUSPEND;
        }
        m_underFork |= F_MIGHT_NEED_PROC;
        iterateChildren(nodep);
    }
    શૂન્ય visit(AstAssignDly* nodep) override {
        જો (!VN_IS(m_procp, ગાંઠ_Procedure)) v3Global.setUsesTiming();
        visit(static_cast<Ast_ગાંઠ*>(nodep));
    }
    શૂન્ય visit(AstFireEvent* nodep) override {
        જો (nodep->isDelayed()
            && (!VN_IS(m_procp, ગાંઠ_Procedure) || !VN_IS(nodep->operandp(), VarRef))) {
            v3Global.setUsesTiming();
        }
        visit(static_cast<Ast_ગાંઠ*>(nodep));
    }
    શૂન્ય visit(AstAssignW* nodep) override {
        જો (nodep->timingControlp()) v3Global.setUsesTiming();
        // Containing process will not suspend, don't mark it
    }
    શૂન્ય visit(Ast_ગાંઠ* nodep) override {
        જો (nodep->isTimingControl()) {
            v3Global.setUsesTiming();
            જો (m_procp) addFlags(m_procp, T_SUSPENDEE | T_SUSPENDER | T_NEEDS_PROC);
        }
        iterateChildren(nodep);
    }

    //--------------------
    શૂન્ય visit(AstVar*) override {}  // Accelerate

જાહેર:
    // CONSTRUCTORS
    explicit TimingSuspendableVisitor(AstNetlist* nodep) {
        iterate(nodep);
        m_suspGraph.removeTransitiveEdges();
        m_procGraph.removeTransitiveEdges();
        // Propagate suspendability
        માટે (V3GraphVertex& vtx : m_suspGraph.vertices()) {
            DepVtx& depVtx = static_cast<DepVtx&>(vtx);
            જો (hasFlags(depVtx.nodep(), T_SUSPENDEE)) propagateFlags(&depVtx, T_SUSPENDEE);
        }
        જો (dumpGraphLevel() >= 6) m_suspGraph.dumpDotFilePrefixed("timing_deps");

        // Propagate T_HAS_PROCESS
        માટે (V3GraphVertex& vtx : m_procGraph.vertices()) {
            DepVtx& depVtx = static_cast<DepVtx&>(vtx);
            // Find processes that'll allocate VlProcess
            જો (hasFlags(depVtx.nodep(), T_FORCES_PROC)) {
                propagateFlagsIf(&depVtx, T_FORCES_PROC, [&](અચલ V3GraphEdge* e) -> તાર્કિક {
                    પાછા !hasFlags(static_cast<DepVtx*>(e->fromp())->nodep(), T_ALLOCS_PROC);
                });
            }
            // Mark nodes on paths between processes and statements that use VlProcess
            જો (hasFlags(depVtx.nodep(), T_NEEDS_PROC)) {
                propagateFlagsIf(&depVtx, T_NEEDS_PROC, [&](અચલ V3GraphEdge* e) -> તાર્કિક {
                    પાછા !hasFlags(static_cast<DepVtx*>(e->ટોચ())->nodep(), T_ALLOCS_PROC);
                });
            }
        }
        માટે (V3GraphVertex& vtx : m_procGraph.vertices()) {
            DepVtx& depVtx = static_cast<DepVtx&>(vtx);
            // Mark nodes that will be emitted with a VlProcess argument
            જો (hasFlags(depVtx.nodep(), T_ALLOCS_PROC | T_FORCES_PROC)) {
                addFlags(depVtx.nodep(), T_HAS_PROC);
                propagateFlagsReversedIf(&depVtx, T_HAS_PROC, [&](અચલ V3GraphEdge* e) -> તાર્કિક {
                    પાછા hasFlags(static_cast<DepVtx*>(e->fromp())->nodep(), T_NEEDS_PROC);
                });
            }
        }
        જો (dumpGraphLevel() >= 6) m_procGraph.dumpDotFilePrefixed("proc_deps");
    }
    ~TimingSuspendableVisitor() override = મૂળભૂત;
};

// ######################################################################
//  Transform nodes affected by timing

વર્ગ TimingControlVisitor final : જાહેર VNVisitor {
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
    AstNetlist* અચલ m_netlistp;  // Root node
    Ast_ટોચ_Scope* અચલ m_ટોચ_Scopep = m_netlistp->ટોચ_Scopep();  // The AstTopScope
    AstScope* અચલ m_scopeTopp = m_ટોચ_Scopep->scopep();  // Scope at the top
    AstClass* m_classp = nullptr;  // Current class
    AstScope* m_scopep = nullptr;  // Current scope
    AstActive* m_activep = nullptr;  // Current active
    Ast_ગાંઠ* m_procp = nullptr;  // NodeProcedure/CFunc/Begin we're under
    તાર્કિક m_hasProcess = ખોટું;  // True if current scope has a VlProcess handle available
    પૂર્ણાંક m_forkCnt = 0;  // Number of forks inside a module
    તાર્કિક m_underJumpBlock = ખોટું;  // True if we are inside of a jump-block
    તાર્કિક m_underProcedure = ખોટું;  // True if we are under an always or initial
    તાર્કિક m_underIfaceCFunc = ખોટું;  // True if we are under a CFunc owned by an interface scope
    તાર્કિક m_hasStaticZeroDelay = ખોટું;  // True if we have a static #0 delay
    std::vector<File_લીટી*> m_unknownDelayFlps;  // Locations of AstDelay with non-constant value

    // Unique names
    V3UniqueNames m_dlyforkNames{"__Vdlyfork"};  // Names for temp AssignW vars
    V3UniqueNames m_intra_કિંમત_Names{"__Vintraval"};  // Intra assign delay value var names
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
    Ast_Sen_વૃક્ષ* m_delaySensesp = nullptr;  // Domain to trigger if a delayed coroutine is resumed
    Ast_Sen_વૃક્ષ* m_dynamicSensesp = nullptr;  // Domain to trigger if a dynamic trigger is set

    // Other
    Sen_વૃક્ષ_Finder m_finder{m_netlistp};  // Sentree finder and uniquifier
    Sen_પદાવલિ_Builder* m_sen_પદાવલિ_Builderp = nullptr;  // Sens expression builder for current m_scope

    // Stats
    size_t m_statZeroDelays = 0;  // Number of statically known #0 delays
    size_t m_statConstDelays = 0;  // Number of statically known #const (non-zero) delays
    size_t m_statVariableDelays = 0;  // Number of delays with value unknown at compile time

    // METHODS
    // Transform an assignment with an intra timing control into a timing control with the
    // assignment under it
    Ast_ગાંઠ* factorOutTimingControl(Ast_ગાંઠ_Assign* nodep) અચલ {
        Ast_ગાંઠ* stmtp = nodep;
        Ast_ગાંઠ* અચલ controlp = nodep->timingControlp();
        જો (AstDelay* અચલ delayp = VN_CAST(controlp, Delay)) {
            stmtp->replaceWith(delayp->unlinkFrBack());
            delayp->addStmtsp(stmtp);
            stmtp = delayp;
        } નહીંતર જો (Ast_Sen_વૃક્ષ* અચલ sentreep = VN_CAST(controlp, Sen_વૃક્ષ)) {
            AstEventControl* અચલ eventControlp
                = નવું AstEventControl{sentreep->fileline(), sentreep->unlinkFrBack(), nullptr};
            stmtp->replaceWith(eventControlp);
            eventControlp->addStmtsp(stmtp);
            stmtp = eventControlp;
        } નહીંતર જો (AstBegin* અચલ beginp = VN_CAST(controlp, Begin)) {
            // Begin from V3AssertPre
            stmtp->replaceWith(beginp->unlinkFrBack());
            beginp->addStmtsp(stmtp);
            stmtp = beginp;
        }
        પાછા stmtp == nodep ? nullptr : stmtp;
    }
    સ્થિર તાર્કિક staticallyNonZeroDelay(અચલ Ast_ગાંઠ_પદાવલિ* valuep) {
        જો (અચલ AstConst* અચલ constp = VN_CAST(valuep, Const)) પાછા !constp->isZero();
        જો (અચલ AstCond* અચલ condp = VN_CAST(valuep, Cond)) {
            પાછા staticallyNonZeroDelay(condp->thenp())
                   && staticallyNonZeroDelay(condp->elsep());
        }
        જો (અચલ AstMul* અચલ mulp = VN_CAST(valuep, Mul)) {
            અચલ AstConst* અચલ lhsConstp = VN_CAST(mulp->lhsp(), Const);
            અચલ AstConst* અચલ rhsConstp = VN_CAST(mulp->rhsp(), Const);
            જો (lhsConstp && !lhsConstp->isZero()) પાછા staticallyNonZeroDelay(mulp->rhsp());
            જો (rhsConstp && !rhsConstp->isZero()) પાછા staticallyNonZeroDelay(mulp->lhsp());
        }
        પાછા ખોટું;
    }
    // Creates the global delay scheduler variable
    AstVarScope* getCreateDelayScheduler() {
        જો (m_delaySchedp) પાછા m_delaySchedp;
        auto* અચલ dlySchedDtp = નવું AstBasicDType{
            m_scopeTopp->fileline(), VBasicDTypeKwd::DELAY_SCHEDULER, VSigning::UNSIGNED};
        m_netlistp->typeTablep()->addTypesp(dlySchedDtp);
        m_delaySchedp = m_ટોચ_Scopep->createTemp("__VdlySched", dlySchedDtp);
        // Delay scheduler has to be accessible from top
        m_delaySchedp->varp()->sigPublic(સાચું);
        m_netlistp->delaySchedulerp(m_delaySchedp->varp());
        પાછા m_delaySchedp;
    }
    // Creates the delay sentree
    Ast_Sen_વૃક્ષ* get_Create_Delay_Sen_વૃક્ષ() {
        જો (m_delaySensesp) પાછા m_delaySensesp;
        File_લીટી* અચલ flp = m_scopeTopp->fileline();
        auto* અચલ awaitingCurrentTimep
            = નવું AstCMethodHard{flp, નવું AstVarRef{flp, getCreateDelayScheduler(), VAccess::READ},
                                 VCMethod::SCHED_AWAITING_CURRENT_TIME};
        awaitingCurrentTimep->dtypeSetBit();
        m_delaySensesp
            = નવું Ast_Sen_વૃક્ષ{flp, નવું AstSenItem{flp, VEdgeType::ET_TRUE, awaitingCurrentTimep}};
        m_netlistp->ટોચ_Scopep()->addSenTreesp(m_delaySensesp);
        પાછા m_delaySensesp;
    }
    // Creates the global dynamic trigger scheduler variable
    AstVarScope* getCreateDynamicTriggerScheduler() {
        જો (m_dynamicSchedp) પાછા m_dynamicSchedp;
        auto* અચલ dynSchedDtp
            = નવું AstBasicDType{m_scopeTopp->fileline(), VBasicDTypeKwd::DYNAMIC_TRIGGER_SCHEDULER,
                                VSigning::UNSIGNED};
        m_netlistp->typeTablep()->addTypesp(dynSchedDtp);
        m_dynamicSchedp = m_ટોચ_Scopep->createTemp("__VdynSched", dynSchedDtp);
        પાછા m_dynamicSchedp;
    }
    // Creates the dynamic trigger sentree
    Ast_Sen_વૃક્ષ* get_Create_Dynamic_Trigger_Sen_વૃક્ષ() {
        જો (m_dynamicSensesp) પાછા m_dynamicSensesp;
        File_લીટી* અચલ flp = m_scopeTopp->fileline();
        auto* અચલ awaitingCurrentTimep = નવું AstCMethodHard{
            flp, નવું AstVarRef{flp, getCreateDynamicTriggerScheduler(), VAccess::READ},
            VCMethod::SCHED_EVALUATE};
        awaitingCurrentTimep->dtypeSetBit();
        m_dynamicSensesp
            = નવું Ast_Sen_વૃક્ષ{flp, નવું AstSenItem{flp, VEdgeType::ET_TRUE, awaitingCurrentTimep}};
        m_netlistp->ટોચ_Scopep()->addSenTreesp(m_dynamicSensesp);
        પાછા m_dynamicSensesp;
    }
    // Creates the event variable to trigger in NBA region
    AstEventControl* createNbaEventControl(File_લીટી* flp) {
        જો (!m_netlistp->nbaEventp()) {
            auto* અચલ nbaEventDtp = નવું AstBasicDType{m_scopeTopp->fileline(),
                                                        VBasicDTypeKwd::EVENT, VSigning::UNSIGNED};
            m_netlistp->typeTablep()->addTypesp(nbaEventDtp);
            m_netlistp->nbaEventp(m_ટોચ_Scopep->createTemp("__VnbaEvent", nbaEventDtp));
            v3Global.setHasEvents();
        }
        પાછા નવું AstEventControl{
            flp,
            નવું Ast_Sen_વૃક્ષ{
                flp, નવું AstSenItem{flp, VEdgeType::ET_EVENT,
                                    નવું AstVarRef{flp, m_netlistp->nbaEventp(), VAccess::READ}}},
            nullptr};
    }
    // Creates the variable that, if set, causes the NBA event to be triggered
    AstAssign* createNbaEventTriggerAssignment(File_લીટી* flp) {
        જો (!m_netlistp->nbaEventTriggerp()) {
            m_netlistp->nbaEventTriggerp(m_ટોચ_Scopep->createTemp("__VnbaEventTrigger", 1));
        }
        પાછા નવું AstAssign{flp,
                             નવું AstVarRef{flp, m_netlistp->nbaEventTriggerp(), VAccess::WRITE},
                             નવું AstConst{flp, AstConst::BitTrue{}}};
    }
    // Returns true if we are under a class or interface function, or the tree references locals.
    // These are cases where static, globally-evaluated triggers are not suitable.
    તાર્કિક needDynamicTrigger(Ast_ગાંઠ* અચલ nodep) અચલ {
        પાછા m_classp || m_underIfaceCFunc || nodep->exists([](Ast_ગાંઠ* અચલ nodep) {
            જો (Ast_ગાંઠ_Var_Ref* varp = VN_CAST(nodep, ગાંઠ_Var_Ref)) {
                પાછા varp->varp()->isFuncLocal();
            }
            પાછા !nodep->isPure();
        });
    }
    // Creates a trigger scheduler variable
    AstVarScope* getCreateTriggerSchedulerp(Ast_Sen_વૃક્ષ* અચલ sentreep) {
        જો (!sentreep->user1p()) {
            જો (!m_trigSchedDtp) {
                m_trigSchedDtp
                    = નવું AstBasicDType{m_scopeTopp->fileline(), VBasicDTypeKwd::TRIGGER_SCHEDULER,
                                        VSigning::UNSIGNED};
                m_netlistp->typeTablep()->addTypesp(m_trigSchedDtp);
            }
            AstVarScope* અચલ trigSchedp
                = m_ટોચ_Scopep->createTemp(m_trigSchedNames.get(sentreep), m_trigSchedDtp);
            sentreep->user1p(trigSchedp);
        }
        પાછા VN_AS(sentreep->user1p(), VarScope);
    }
    // Creates a string describing the sentree
    AstCExpr* createEventDescription(Ast_Sen_વૃક્ષ* અચલ sentreep) અચલ {
        જો (!sentreep->user2p()) {
            std::stringstream ss;
            V3EmitV::verilog_For_વૃક્ષ(sentreep, ss);
            // possibly a multiline string
            // Names were hashed by V3Name, show the user the original text
            std::string comment = VName::dehash(ss.str());
            std::replace(comment.begin(), comment.end(), '\n', ' ');
            // Escape quotes, e.g. from string-indexed event array selects
            comment = '"' + V3OutFormatter::quoteNameControls(comment) + '"';
            AstCExpr* અચલ commentp = નવું AstCExpr{sentreep->fileline(), comment};
            commentp->dtypeSetString();
            sentreep->user2p(commentp);
            પાછા commentp;
        }
        પાછા VN_AS(sentreep->user2p(), CExpr)->clone_વૃક્ષ(ખોટું);
    }
    // Adds debug info to a hardcoded method call
    શૂન્ય addDebugInfo(AstCMethodHard* અચલ methodp) અચલ {
        File_લીટી* અચલ flp = methodp->fileline();
        અચલ તાર્કિક protectIds = v3Global.opt.protectIds();
        AstCExpr* અચલ ap
            = નવું AstCExpr{flp, protectIds ? "VL_UNKNOWN" : '"' + flp->filenameEsc() + '"'};
        ap->dtypeSetString();
        methodp->addPinsp(ap);
        AstCExpr* અચલ bp = નવું AstCExpr{flp, protectIds ? "0" : cvtToStr(flp->lineno())};
        bp->dtypeSetString();
        methodp->addPinsp(bp);
    }
    // Adds debug info to a trigSched.trigger() call
    શૂન્ય addEventDebugInfo(AstCMethodHard* અચલ methodp, Ast_Sen_વૃક્ષ* અચલ sentreep) અચલ {
        જો (v3Global.opt.protectIds()) {
            File_લીટી* અચલ flp = sentreep->fileline();
            AstCExpr* અચલ descp = નવું AstCExpr{flp, "VL_UNKNOWN"};
            descp->dtypeSetString();
            methodp->addPinsp(descp);
            addDebugInfo(methodp);
            પાછા;
        }
        methodp->addPinsp(createEventDescription(sentreep));
        addDebugInfo(methodp);
    }
    // Adds process pointer to a hardcoded method call
    શૂન્ય addProcessInfo(AstCMethodHard* અચલ methodp) અચલ {
        File_લીટી* અચલ flp = methodp->fileline();
        AstCExpr* અચલ ap
            = નવું AstCExpr{flp, (m_procp && m_hasProcess) ? "vlProcess" : "nullptr"};
        methodp->addPinsp(ap);
    }
    // Creates the fork handle type and returns it
    AstBasicDType* getCreateForkSyncDTypep() {
        જો (m_forkDtp) પાછા m_forkDtp;
        m_forkDtp = નવું AstBasicDType{m_scopeTopp->fileline(), VBasicDTypeKwd::FORK_SYNC,
                                      VSigning::UNSIGNED};
        m_netlistp->typeTablep()->addTypesp(m_forkDtp);
        પાછા m_forkDtp;
    }
    // Move `nodep` into `AstCLocalScope`.
    AstCLocalScope* addCLocalScope(File_લીટી* અચલ flp, Ast_ગાંઠ* અચલ nodep) અચલ {
        જો (!nodep) પાછા nullptr;
        VNRelinker handle;
        nodep->unlinkFrBack(&handle);
        AstCLocalScope* અચલ cscopep = નવું AstCLocalScope{flp, nodep};
        handle.relink(cscopep);
        પાછા cscopep;
    }
    // Create a temp variable and optionally put it before the specified node (mark local if so)
    AstVarScope* createTemp(File_લીટી* અચલ flp, અચલ std::string& name,
                            Ast_ગાંઠ_DType* અચલ dtypep, Ast_ગાંઠ* અચલ ઉમેરો_Beforep = nullptr) {
        AstVar* varp;
        જો (ઉમેરો_Beforep) {
            varp = નવું AstVar{flp, VVarType::BLOCKTEMP, name, dtypep};
            varp->funcLocal(સાચું);
            ઉમેરો_Beforep->addHereThisAsNext(varp);
        } નહીંતર {
            varp = નવું AstVar{flp, VVarType::MODULETEMP, name, dtypep};
            m_scopep->modp()->addStmtsp(varp);
        }
        AstVarScope* અચલ vscp = નવું AstVarScope{flp, m_scopep, varp};
        m_scopep->addVarsp(vscp);
        પાછા vscp;
    }
    // Add a done() call on the fork sync
    શૂન્ય addForkDone(AstBegin* અચલ beginp, AstVarScope* અચલ forkVscp) અચલ {
        File_લીટી* અચલ flp = beginp->fileline();
        auto* અચલ donep = નવું AstCMethodHard{
            beginp->fileline(), નવું AstVarRef{flp, forkVscp, VAccess::WRITE}, VCMethod::FORK_DONE};
        donep->dtypeSetVoid();
        addDebugInfo(donep);
        beginp->addStmtsp(donep->makeStmt());
    }
    સ્થિર તાર્કિક has_Disable_Queue_ધકેલો_Self_Prefix(AstBegin* અચલ beginp) {
        // LinkJump prepends disable-by-name registration as:
        //   __VprocessQueue_*.push_back(std::process::self())
        // V3LiftExpr lifts std::process::self() into an assignment to a temporary, then V3Task
        // lowers that assignment's function call into a leading comment and AstStmtExpr. Thus the
        // push_back is no longer necessarily the first statement. Scan across this generated
        // prefix for it, stopping at the fork-start sentinel or the branch body. Registrations for
        // nested forks live in sub-blocks and so are not in this statement list.
        માટે (Ast_ગાંઠ* stmtp = beginp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            જો (VN_IS(stmtp, Comment)) ચાલુ;
            Ast_Stmt_પદાવલિ* અચલ stmtExprp = VN_CAST(stmtp, Stmt_પદાવલિ);
            જો (!stmtExprp) તોડો;
            જો (stmtExprp->is_Disable_Queue_ધકેલો_Self_Stmt()) પાછા સાચું;
        }
        પાછા ખોટું;
    }
    // Register a callback so killing a process-backed fork branch decrements the join counter
    શૂન્ય addForkOnKill(AstBegin* અચલ beginp, AstVarScope* અચલ forkVscp) અચલ {
        જો (!beginp->needProcess() && !has_Disable_Queue_ધકેલો_Self_Prefix(beginp)) પાછા;
        File_લીટી* અચલ flp = beginp->fileline();
        AstCMethodHard* અચલ onKillp = નવું AstCMethodHard{
            flp, નવું AstVarRef{flp, forkVscp, VAccess::WRITE}, VCMethod::FORK_ON_KILL};
        onKillp->dtypeSetVoid();
        AstCExpr* અચલ processp = નવું AstCExpr{flp, "vlProcess"};
        processp
            ->dtypeSetVoid();  // Opaque process reference; type is irrelevant for hardcoded emit
        onKillp->addPinsp(processp);
        Ast_ગાંઠ_Stmt* અચલ stmtp = onKillp->makeStmt();
        beginp->prependStmtsp(stmtp);
    }
    // Handle the 'join' part of a fork..join
    શૂન્ય makeForkJoin(AstFork* અચલ forkp) {
        // Create a fork sync var
        File_લીટી* અચલ flp = forkp->fileline();
        // Insert the sync var directly before the fork
        Ast_ગાંઠ* અચલ ઉમેરો_Beforep = forkp;
        // Make sure all references to the fork var are contained within a single scope to prevent
        // moving statements accessing it in case of large functions splitting.
        addCLocalScope(flp, ઉમેરો_Beforep);
        AstVarScope* forkVscp
            = createTemp(flp, forkp->name() + "__sync", getCreateForkSyncDTypep(), ઉમેરો_Beforep);
        unsigned join_ગણતરી = 0;  // Needed for join counter
        // Add a <fork sync>.done() to each begin
        માટે (Ast_ગાંઠ* beginp = forkp->forksp(); beginp; beginp = beginp->nextp()) {
            addForkOnKill(VN_AS(beginp, Begin), forkVscp);
            addForkDone(VN_AS(beginp, Begin), forkVscp);
            join_ગણતરી++;
        }
        જો (forkp->joinType().joinAny()) join_ગણતરી = 1;
        // Set the join counter
        auto* અચલ initp = નવું AstCMethodHard{flp, નવું AstVarRef{flp, forkVscp, VAccess::WRITE},
                                               VCMethod::FORK_INIT, નવું AstConst{flp, join_ગણતરી}};
        initp->dtypeSetVoid();
        addProcessInfo(initp);
        forkp->addHereThisAsNext(initp->makeStmt());
        // Await the join at the end
        auto* અચલ joinp = નવું AstCMethodHard{flp, નવું AstVarRef{flp, forkVscp, VAccess::WRITE},
                                               VCMethod::FORK_JOIN};
        joinp->dtypeSetVoid();
        addProcessInfo(joinp);
        addDebugInfo(joinp);
        forkp->addNextHere(નવું AstCAwait{flp, joinp});
    }

    // `procp` shall be a NodeProcedure/CFunc/Begin and within it vars from `varsp` will be placed.
    // `varsp` vector of vars which shall be localized.
    સ્થિર શૂન્ય localizeVars(Ast_ગાંઠ* અચલ procp, અચલ std::vector<AstVar*>& varsp) {
        UASSERT(procp, "procp is nullptr");
        AstVar* newVarsp = nullptr;
        માટે (AstVar* અચલ varp : varsp) {
            varp->funcLocal(સાચું);
            newVarsp = Ast_ગાંઠ::addNextNull(newVarsp, varp->unlinkFrBack());
        }
        જો (Ast_ગાંઠ_Procedure* અચલ ગાંઠ_Procp = VN_CAST(procp, ગાંઠ_Procedure)) {
            ગાંઠ_Procp->prependStmtsp(newVarsp);
        } નહીંતર જો (AstCFunc* અચલ cfuncp = VN_CAST(procp, CFunc)) {
            cfuncp->prependVarsp(newVarsp);
        } નહીંતર જો (AstBegin* અચલ beginp = VN_CAST(procp, Begin)) {
            beginp->prependStmtsp(newVarsp);
        } નહીંતર {
            procp->v3fatalSrc(
                procp->prettyNameQ()
                << " is not of an expected type NodeProcedure/CFunc/Begin instead it is: "
                << procp->prettyTypeName());
        }
    }

    Ast_ગાંઠ_પદાવલિ* delayToIntegral(AstDelay* nodep, Ast_ગાંઠ_પદાવલિ* valuep) {
        અચલ AstConst* અચલ constp = VN_CAST(valuep, Const);
        // The fork start sentinel is already in ticks
        જો (constp && constp->toUQuad() == std::numeric_limits<uint64_t>::max()) પાછા valuep;
        પાછા V3Timing::delayToTicks(valuep, nodep->timeunit());
    }

    // VISITORS
    શૂન્ય visit(Ast_ગાંઠ_Module* nodep) override {
        UASSERT_OBJ(!m_classp, nodep, "Module or class under class");
        VL_RESTORER(m_classp);
        m_classp = VN_CAST(nodep, Class);
        VL_RESTORER(m_forkCnt);
        m_forkCnt = 0;
        iterateChildren(nodep);
    }
    શૂન્ય visit(AstScope* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        Sen_પદાવલિ_Builder sen_પદાવલિ_Builder{m_scopep};
        {  // Restore m_senExprBuilderp before destroying senExprBuilder
            VL_RESTORER(m_sen_પદાવલિ_Builderp);
            m_sen_પદાવલિ_Builderp = &sen_પદાવલિ_Builder;
            iterateChildren(nodep);
        }
    }
    શૂન્ય visit(AstActive* nodep) override {
        VL_RESTORER(m_activep);
        m_activep = nodep;
        iterateChildren(nodep);
    }
    શૂન્ય visit(Ast_ગાંઠ_Procedure* nodep) override {
        VL_RESTORER(m_procp);
        VL_RESTORER(m_hasProcess);
        m_procp = nodep;
        m_hasProcess = hasFlags(nodep, T_HAS_PROC);
        VL_RESTORER(m_underProcedure);
        m_underProcedure = સાચું;
        iterateChildren(nodep);
        જો (hasFlags(nodep, T_SUSPENDEE)) nodep->setSuspendable();
        જો (hasFlags(nodep, T_HAS_PROC)) nodep->setNeedProcess();
    }
    શૂન્ય visit(AstInitial* nodep) override {
        visit(static_cast<Ast_ગાંઠ_Procedure*>(nodep));
        જો (nodep->needProcess() && !nodep->user1SetOnce()) {
            nodep->addStmtsp(
                નવું AstCStmt{nodep->fileline(), "vlProcess->state(VlProcess::FINISHED);"});
        }
    }
    શૂન્ય visit(AstJumpBlock* nodep) override {
        VL_RESTORER(m_underJumpBlock);
        m_underJumpBlock = સાચું;
        visit(static_cast<Ast_ગાંઠ_Stmt*>(nodep));
    }
    શૂન્ય visit(AstAlways* nodep) override {
        જો (nodep->user1SetOnce()) પાછા;
        VL_RESTORER(m_procp);
        VL_RESTORER(m_hasProcess);
        m_procp = nodep;
        m_hasProcess = hasFlags(nodep, T_HAS_PROC);
        VL_RESTORER(m_underProcedure);
        m_underProcedure = સાચું;
        // Workaround for killing `always` processes (doing that is pretty much UB)
        // TODO: Disallow killing `always` at runtime (throw an error)
        // Skip for combinational blocks; if the body has timing controls
        // iterateChildren will add T_SUSPENDEE, otherwise it would spin.
        જો (hasFlags(nodep, T_HAS_PROC)
            && !(m_activep && m_activep->sentreep() && m_activep->sentreep()->hasCombo()))
            addFlags(nodep, T_SUSPENDEE);

        iterateChildren(nodep);
        જો (hasFlags(nodep, T_HAS_PROC)) nodep->setNeedProcess();
        જો (!hasFlags(nodep, T_SUSPENDEE)) પાછા;
        nodep->setSuspendable();
        File_લીટી* અચલ flp = nodep->fileline();
        Ast_Sen_વૃક્ષ* અચલ sentreep = m_activep->sentreep();
        જો (sentreep->hasClocked()) {
            Ast_ગાંઠ* અચલ bodysp = nodep->stmtsp()->unlinkFrBackWithNext();
            auto* અચલ controlp = નવું AstEventControl{flp, sentreep->clone_વૃક્ષ(ખોટું), bodysp};
            nodep->addStmtsp(controlp);
            iterate(controlp);
        }
        // Note: The 'while (true)' outer loop will be added in V3Sched
        auto* અચલ activep = નવું AstActive{
            flp, "", નવું Ast_Sen_વૃક્ષ{flp, નવું AstSenItem{flp, AstSenItem::Initial{}}}};
        activep->sen_વૃક્ષ_Storep(activep->sentreep());
        activep->addStmtsp(nodep->unlinkFrBack());
        m_activep->addNextHere(activep);
    }
    શૂન્ય visit(AstCFunc* nodep) override {
        VL_RESTORER(m_procp);
        VL_RESTORER(m_hasProcess);
        VL_RESTORER(m_underIfaceCFunc);
        m_procp = nodep;
        m_hasProcess = hasFlags(nodep, T_HAS_PROC);
        m_underIfaceCFunc = VN_IS(m_scopep->modp(), Iface);
        iterateChildren(nodep);
        જો (hasFlags(nodep, T_HAS_PROC)) nodep->setNeedProcess();
        જો (!(hasFlags(nodep, T_SUSPENDEE))) પાછા;

        nodep->rtnType("VlCoroutine");
        // If in a class, create a shared pointer to 'this'
        જો (m_classp) {
            AstCStmt* અચલ cstmtp = નવું AstCStmt{nodep->fileline(), "VL_KEEP_THIS;"};
            nodep->prependStmtsp(cstmtp);
        }
        જો (nodep->dpiExportImpl()) {
            nodep->exists([](AstCAwait* અચલ awaitp) -> તાર્કિક {
                // A DPI-exported coroutine won't be able to block the calling code
                // Error on the await node; fall back to the function node
                awaitp->v3warn(E_UNSUPPORTED,
                               "Unsupported: Timing controls inside DPI-exported tasks");
                પાછા સાચું;
            });
        }
    }
    શૂન્ય visit(Ast_ગાંઠ_CCall* nodep) override {
        AstCFunc* અચલ funcp = nodep->funcp();

        // Skip automatic covergroup sampling calls
        જો (funcp->isCovergroupSample()) {
            iterateChildren(nodep);
            પાછા;
        }

        જો (funcp->needProcess()) m_hasProcess = સાચું;
        જો (hasFlags(funcp, T_SUSPENDEE) && !nodep->user1SetOnce()) {  // If suspendable
            // Calls to suspendables are always void return type, hence parent must be StmtExpr
            Ast_Stmt_પદાવલિ* અચલ stmtp = VN_AS(nodep->backp(), Stmt_પદાવલિ);
            stmtp->replaceWith(નવું AstCAwait{nodep->fileline(), nodep->unlinkFrBack()});
            VL_DO_DANGLING(ધકેલો_Deletep(stmtp), stmtp);
        }
        iterateChildren(nodep);
    }
    શૂન્ય visit(AstDelay* nodep) override {
        UASSERT_OBJ(!nodep->isCycleDelay(), nodep,
                    "Cycle delays should have been handled in V3AssertPre");
        File_લીટી* અચલ flp = nodep->fileline();

        Ast_ગાંઠ_પદાવલિ* valuep = delayToIntegral(nodep, nodep->lhsp()->unlinkFrBack());

        // Statistics
        જો (valuep->isZero()) {
            ++m_statZeroDelays;
        } નહીંતર જો (VN_IS(valuep, Const)) {
            ++m_statConstDelays;
        } નહીંતર {
            ++m_statVariableDelays;
        }

        // Decide scheduling support for #0
        જો (v3Global.opt.schedZeroDelay().isSetTrue()) {
            // User said to schedule for #0 support, nothing else to do
            v3Global.setUsesZeroDelay();
        } નહીંતર જો (v3Global.opt.schedZeroDelay().isSetFalse()) {
            // User said to schedule without #0 support. Still warn if a static #0 delay exists
            જો (valuep->isZero()) {
                nodep->v3warn(
                    ZERODLY,
                    "Static #0 delay exists, but '--no-sched-zero-delay' was given.\n"
                        << nodep->warnMore()  //
                        << "... Can proceed, but this will fail at runtime if executed.");
            }
        } નહીંતર {
            // User did not express preference, decide based on presence of delays
            જો (valuep->isZero()) {
                // Statically known #0 delay exists, schedule for #0 support
                v3Global.setUsesZeroDelay();
                m_hasStaticZeroDelay = સાચું;
                // Don't warn on variable delays, as no point
                m_unknownDelayFlps.clear();
            } નહીંતર જો (staticallyNonZeroDelay(valuep)) {
                // Delay is dynamic, but every statically known outcome is non-zero.
                // So we don't need #0 delay support and there should be no warning.
            } નહીંતર જો (!VN_IS(valuep, Const)) {
                // Delay is not known at compiile time. Conservatively schedule for #0 support,
                // but warn if no static #0 delays used as performance might be improved
                v3Global.setUsesZeroDelay();
                જો (!m_hasStaticZeroDelay) m_unknownDelayFlps.ધકેલો_back(nodep->fileline());
            }
        }

        // Replace self with a 'co_await dlySched.delay(<valuep>)'
        AstCMethodHard* અચલ delayMethodp = નવું AstCMethodHard{
            flp, નવું AstVarRef{flp, getCreateDelayScheduler(), VAccess::WRITE},
            VCMethod::SCHED_DELAY, valuep};
        delayMethodp->dtypeSetVoid();
        addProcessInfo(delayMethodp);
        addDebugInfo(delayMethodp);
        // Create the co_await
        Ast_ગાંઠ* અચલ awaitp = નવું AstCAwait{flp, delayMethodp, get_Create_Delay_Sen_વૃક્ષ()};
        // Relink child statements after the co_await
        જો (Ast_ગાંઠ* અચલ stmtsp = nodep->stmtsp()) {
            awaitp->addNext(stmtsp->unlinkFrBackWithNext());
        }
        nodep->replaceWith(awaitp);
        VL_DO_DANGLING(nodep->delete_વૃક્ષ(), nodep);
    }
    શૂન્ય visit(AstEventControl* nodep) override {
        // Do not allow waiting on local named events, as they get enqueued for clearing, but can
        // go out of scope before that happens
        જો (!nodep->sentreep()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Event control with implicit sensitivity (@*)"
                          " in this context; use an explicit sensitivity list instead");
            VL_DO_DANGLING(ધકેલો_Deletep(nodep->unlinkFrBack()), nodep);
            પાછા;
        }
        File_લીટી* અચલ flp = nodep->fileline();
        // Relink child statements after the event control
        જો (nodep->stmtsp()) nodep->addNextHere(nodep->stmtsp()->unlinkFrBackWithNext());
        જો (needDynamicTrigger(nodep->sentreep())) {
            // Create the trigger variable and init it with 0
            AstVarScope* અચલ trigvscp
                = createTemp(flp, m_dynTrigNames.get(nodep), nodep->findBitDType(), nodep);
            auto* અચલ initp = નવું AstAssign{flp, નવું AstVarRef{flp, trigvscp, VAccess::WRITE},
                                              નવું AstConst{flp, AstConst::BitFalse{}}};
            // Await the eval step with the dynamic trigger scheduler. First, create the method
            // call
            auto* અચલ evalMethodp = નવું AstCMethodHard{
                flp, નવું AstVarRef{flp, getCreateDynamicTriggerScheduler(), VAccess::WRITE},
                VCMethod::SCHED_EVALUATION};
            evalMethodp->dtypeSetVoid();
            addProcessInfo(evalMethodp);
            auto* અચલ sentreep = nodep->sentreep();
            addEventDebugInfo(evalMethodp, sentreep);
            // Create the co_await
            AstCAwait* અચલ awaitEvalp
                = નવું AstCAwait{flp, evalMethodp, get_Create_Dynamic_Trigger_Sen_વૃક્ષ()};
            // Construct the sen expression for this sentree
            UASSERT_OBJ(m_sen_પદાવલિ_Builderp, nodep, "No SenExprBuilder for this scope");
            auto* અચલ assignp = નવું AstAssign{flp, નવું AstVarRef{flp, trigvscp, VAccess::WRITE},
                                                m_sen_પદાવલિ_Builderp->build(sentreep).first};
            // Get the SenExprBuilder results
            અચલ Sen_પદાવલિ_Builder::Results senResults = m_sen_પદાવલિ_Builderp->getAndClearResults();
            localizeVars(m_procp, senResults.m_vars);
            // If post updates are destructive (e.g. clearFired on events), perform a
            // conservative pre-clear once before entering the wait loop so stale state from a
            // previous wait does not cause an immediate false-positive trigger.
            અચલ તાર્કિક hasDestructivePostUpdates = !senResults.m_destructivePostUpdates.empty();
            // Create the trigger eval loop, which will await the evaluation step and check the
            // trigger
            Ast_ગાંઠ_પદાવલિ* અચલ condp
                = નવું AstLogNot{flp, નવું AstVarRef{flp, trigvscp, VAccess::READ}};
            AstLoop* અચલ loopp = નવું AstLoop{flp};
            loopp->addStmtsp(નવું AstLoopTest{flp, loopp, condp});
            loopp->addStmtsp(awaitEvalp);
            // Put pre updates before the trigger check and assignment
            માટે (Ast_ગાંઠ_Stmt* અચલ stmtp : senResults.m_preUpdates) loopp->addStmtsp(stmtp);
            // Then the trigger check and assignment
            loopp->addStmtsp(assignp);
            // Let the dynamic trigger scheduler know if this trigger was set
            // If it was, a call to the scheduler's evaluate() will return true
            AstCMethodHard* અચલ anyTriggeredMethodp = નવું AstCMethodHard{
                flp, નવું AstVarRef{flp, getCreateDynamicTriggerScheduler(), VAccess::WRITE},
                VCMethod::SCHED_ANY_TRIGGERED, નવું AstVarRef{flp, trigvscp, VAccess::READ}};
            anyTriggeredMethodp->dtypeSetVoid();
            loopp->addStmtsp(anyTriggeredMethodp->makeStmt());
            // If the post update is destructive (e.g. event vars are cleared), create an await for
            // the post update step
            જો (hasDestructivePostUpdates) {
                AstCAwait* અચલ awaitPostUpdatep = awaitEvalp->clone_વૃક્ષ(ખોટું);
                VN_AS(awaitPostUpdatep->exprp(), CMethodHard)->method(VCMethod::SCHED_POST_UPDATE);
                loopp->addStmtsp(awaitPostUpdatep);
            }
            // Put the post updates at the end of the loop
            માટે (Ast_ગાંઠ_Stmt* અચલ stmtp : senResults.m_postUpdates) loopp->addStmtsp(stmtp);
            // Finally, await the resumption step in 'act'
            AstCAwait* અચલ awaitResumep = awaitEvalp->clone_વૃક્ષ(ખોટું);
            VN_AS(awaitResumep->exprp(), CMethodHard)->method(VCMethod::SCHED_RESUMPTION);
            Ast_ગાંઠ::addNext<Ast_ગાંઠ_Stmt, Ast_ગાંઠ_Stmt>(loopp, awaitResumep);
            // Replace the event control with one explicit stmt chain:
            //   init -> [inits] -> [optional destructive pre-clears] -> loop -> awaitResumption
            Ast_ગાંઠ_Stmt* chainp = nullptr;
            chainp = Ast_ગાંઠ::addNextNull(chainp, initp);
            માટે (Ast_ગાંઠ_Stmt* અચલ stmtp : senResults.m_inits) {
                chainp = Ast_ગાંઠ::addNextNull(chainp, stmtp);
            }
            જો (hasDestructivePostUpdates) {
                માટે (Ast_ગાંઠ_Stmt* અચલ stmtp : senResults.m_destructivePostUpdates) {
                    chainp = Ast_ગાંઠ::addNextNull(chainp, stmtp->clone_વૃક્ષ(ખોટું));
                }
            }
            chainp = Ast_ગાંઠ::addNextNull(chainp, loopp);
            nodep->replaceWith(chainp);
        } નહીંતર {
            auto* અચલ sentreep = m_finder.get_Sen_વૃક્ષ(nodep->sentreep());
            nodep->sentreep()->unlinkFrBack()->delete_વૃક્ષ();
            // Get this sentree's trigger scheduler
            // Replace self with a 'co_await trigSched.trigger()'
            auto* અચલ triggerMethodp = નવું AstCMethodHard{
                flp, નવું AstVarRef{flp, getCreateTriggerSchedulerp(sentreep), VAccess::WRITE},
                VCMethod::SCHED_TRIGGER};
            triggerMethodp->dtypeSetVoid();
            // If it should be committed immediately, pass true, otherwise false
            triggerMethodp->addPinsp(nodep->user2() ? નવું AstConst{flp, AstConst::BitTrue{}}
                                                    : નવું AstConst{flp, AstConst::BitFalse{}});
            addProcessInfo(triggerMethodp);
            addEventDebugInfo(triggerMethodp, sentreep);
            // Create the co_await
            AstCAwait* અચલ awaitp = નવું AstCAwait{flp, triggerMethodp, sentreep};
            nodep->replaceWith(awaitp);
        }
        VL_DO_DANGLING(nodep->delete_વૃક્ષ(), nodep);
    }
    શૂન્ય visit(Ast_ગાંઠ_Assign* nodep) override {
        // Only process once to avoid infinite loops (due to the net delay)
        જો (nodep->user1SetOnce()) પાછા;
        UINFO(9, "control-visit " << nodep);
        File_લીટી* અચલ flp = nodep->fileline();
        Ast_ગાંઠ* controlp = factorOutTimingControl(nodep);
        અચલ તાર્કિક inAssignDly = VN_IS(nodep, AssignDly);
        // Handle the intra assignment timing control
        // Transform if:
        // * there's a timing control in the assignment
        // * the assignment is an AssignDly and it's in a non-inlined function
        જો (!controlp && (!inAssignDly || m_underProcedure)) {
            iterateChildren(nodep);
            પાછા;
        }
        // The timing control, if any, which is put in a fork for an NBA
        Ast_ગાંઠ* અચલ timingp = controlp;
        // Insert new vars before the timing control if we're in a function; in a process we can't
        // do that, as after V3Begin only modules and functions declare variables. These
        // intra-assignment vars will later be passed to forked processes by value.
        Ast_ગાંઠ* ઉમેરો_Beforep = m_underProcedure ? nullptr : controlp;
        // Special case for NBA
        જો (inAssignDly) {
            // Put it in a fork so it doesn't block
            // Could already be the only thing directly under a fork, reuse that if possible
            AstFork* forkp = !nodep->nextp() ? VN_CAST(nodep->firstAbovep(), Fork) : nullptr;
            જો (!forkp) forkp = નવું AstFork{flp, VJoinType::JOIN_NONE};
            જો (!m_underProcedure) {
                // If it's in a function, it won't be handled by V3Delayed
                // Put it behind an additional named event that gets triggered in the NBA region
                AstEventControl* અચલ nbaEventControlp = createNbaEventControl(flp);
                AstAssign* અચલ trigAssignp = createNbaEventTriggerAssignment(flp);
                nodep->replaceWith(trigAssignp);
                trigAssignp->addNextHere(nbaEventControlp);
                nbaEventControlp->addStmtsp(nodep);
                ઉમેરો_Beforep = forkp;
                જો (!controlp) controlp = nbaEventControlp;
            }
            controlp->replaceWith(forkp);
            AstBegin* beginp = VN_CAST(controlp, Begin);
            જો (!beginp) beginp = નવું AstBegin{nodep->fileline(), "", controlp, ખોટું};
            forkp->addForksp(beginp);
            addFlags(beginp, T_NBA_UPDATE);
            controlp = forkp;
        }
        UASSERT_OBJ(nodep, controlp, "Assignment should have timing control");
        // Move `insertBeforep` into `AstCLocalScope` if necessary to avoid jumping over
        // a variable initialization that would be inserted before `insertBeforep`. All
        // access to this variable should be contained within returned `AstCLocalScope`.
        જો (m_underJumpBlock) addCLocalScope(flp, ઉમેરો_Beforep);
        // Function for replacing values with intermediate variables
        અચલ auto replaceWithIntermediate = [&](Ast_ગાંઠ_પદાવલિ* અચલ valuep,
                                                 અચલ std::string& name) {
            AstVarScope* અચલ newvscp = createTemp(flp, name, valuep->dtypep(), ઉમેરો_Beforep);
            // A pending update keeps its own value, even if its process schedules more
            જો (inAssignDly && !ઉમેરો_Beforep) newvscp->fork_By_કિંમત(સાચું);
            valuep->replaceWith(નવું AstVarRef{flp, newvscp, VAccess::READ});
            controlp->addHereThisAsNext(
                નવું AstAssign{flp, નવું AstVarRef{flp, newvscp, VAccess::WRITE}, valuep});
        };
        // NBAs with delays evaluate the indices selecting the target now (IEEE 1800-2023 10.4.2),
        // in order, each as a whole, also the selects and function calls in it. So follow the
        // selects of the target up from the variable, not into their indices. As an example, this
        // transforms 'x[a[i]][f(j)] <= #1 y' into 't0 = a[i]; t1 = f(j); x[t0][t1] <= #1 y'.
        // V3Const has split NBAs to concatenations, also streamed, into NBAs to each part.
        જો (inAssignDly) {
            માટે (Ast_ગાંઠ* exprp = nodep->lhsp()->baseFromp(સાચું); exprp != nodep;
                 exprp = exprp->firstAbovep()) {
                જો (AstSel* અચલ selp = VN_CAST(exprp, Sel)) {
                    // widthp should be const
                    જો (!VN_IS(selp->lsbp(), Const)) {
                        replaceWithIntermediate(selp->lsbp(), m_intraLsbNames.get(nodep));
                    }
                } નહીંતર જો (Ast_ગાંઠ_Sel* અચલ selp = VN_CAST(exprp, ગાંઠ_Sel)) {
                    જો (!VN_IS(selp->bitp(), Const)) {
                        replaceWithIntermediate(selp->bitp(), m_intraIndexNames.get(nodep));
                    }
                }
            }
        }
        // Replace the RHS with an intermediate value var
        replaceWithIntermediate(nodep->rhsp(), m_intra_કિંમત_Names.get(nodep));
        જો (!inAssignDly) પાછા;
        // The process executing the NBA evaluates its delay (IEEE 1800-2023 4.9.4), not the forked
        // process, if it has side effects, e.g. calling a function that checks its process. A pure
        // one is evaluated as the forked process starts, at once.
        જો (AstDelay* અચલ delayp = VN_CAST(timingp, Delay)) {
            જો (!delayp->lhsp()->isPure()) {
                replaceWithIntermediate(delayp->lhsp(), m_intra_કિંમત_Names.get(nodep));
            }
        }
    }
    શૂન્ય visit(AstFireEvent* nodep) override {
        // V3Delayed handles '->>' of a variable in a process. Like NBAs in non-inlined functions,
        // trigger other events from a fork awaiting the NBA region.
        જો (!nodep->isDelayed() || (m_underProcedure && VN_IS(nodep->operandp(), VarRef))) {
            iterateChildren(nodep);
            પાછા;
        }
        File_લીટી* અચલ flp = nodep->fileline();
        AstAssign* અચલ trigAssignp = createNbaEventTriggerAssignment(flp);
        nodep->replaceWith(trigAssignp);
        AstFork* અચલ forkp = નવું AstFork{flp, VJoinType::JOIN_NONE};
        trigAssignp->addNextHere(forkp);
        જો (m_underJumpBlock) addCLocalScope(flp, forkp);
        // The triggered event is the one referenced now, so evaluate handles and indices now
        Ast_ગાંઠ_પદાવલિ* અચલ eventp = nodep->operandp()->unlinkFrBack();
        અચલ auto evalNow = [&](Ast_ગાંઠ_પદાવલિ* અચલ valuep, V3UniqueNames& names) {
            // Like for an NBA, a process uses a module variable, passed to the fork by value
            AstVarScope* અચલ vscp = createTemp(flp, names.get(nodep), valuep->dtypep(),
                                                 m_underProcedure ? nullptr : forkp);
            જો (m_underProcedure) vscp->fork_By_કિંમત(સાચું);
            valuep->replaceWith(નવું AstVarRef{flp, vscp, VAccess::READ});
            // Unlike an event, a handle selecting one is only read
            valuep->foreach([](Ast_ગાંઠ* અચલ np) {
                જો (Ast_ગાંઠ_Var_Ref* અચલ refp = VN_CAST(np, ગાંઠ_Var_Ref)) {
                    refp->access(VAccess::READ);
                } નહીંતર જો (AstMemberSel* અચલ selp = VN_CAST(np, MemberSel)) {
                    selp->access(VAccess::READ);
                }
            });
            forkp->addHereThisAsNext(
                નવું AstAssign{flp, નવું AstVarRef{flp, vscp, VAccess::WRITE}, valuep});
        };
        Ast_ગાંઠ_પદાવલિ* refp = eventp;
        જ્યારે (Ast_ગાંઠ_Sel* અચલ selp = VN_CAST(refp, ગાંઠ_Sel)) {
            જો (!VN_IS(selp->bitp(), Const)) evalNow(selp->bitp(), m_intraIndexNames);
            refp = selp->fromp();
        }
        // Handle of a class (or virtual interface)
        જો (AstMemberSel* અચલ selp = VN_CAST(refp, MemberSel)) {
            evalNow(selp->fromp(), m_intra_કિંમત_Names);
        }
        AstEventControl* અચલ controlp = createNbaEventControl(flp);
        controlp->addStmtsp(નવું AstFireEvent{flp, eventp, ખોટું});
        AstBegin* અચલ beginp = નવું AstBegin{flp, "", controlp, ખોટું};
        addFlags(beginp, T_NBA_UPDATE);
        forkp->addForksp(beginp);
        VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
    }
    શૂન્ય visit(AstAssignW* nodep) override {
        File_લીટી* અચલ flp = nodep->fileline();
        // Get the net delay unless this assignment was created for handling the net delay (user1)
        AstDelay* અચલ netDelayp = nodep->user1() ? nullptr : nodep->getLhsNetDelay();
        જો (netDelayp) {
            જો (nodep->timingControlp()) {
                // If this assignment has a delay, create another one to handle the net delay
                AstVarScope* અચલ newvscp = m_assignWTmps.make(flp, m_scopep, nodep->dtypep());
                AstAssignW* assignp = નવું AstAssignW{
                    nodep->fileline(), nodep->lhsp()->unlinkFrBack(),
                    નવું AstVarRef{flp, newvscp, VAccess::READ}, netDelayp->clone_વૃક્ષ(ખોટું)};
                assignp->user1(સાચું);
                nodep->addNextHere(assignp);
                nodep->lhsp(નવું AstVarRef{flp, newvscp, VAccess::WRITE});
            } નહીંતર {
                // Else just use this one with the net delay
                nodep->timingControlp(netDelayp->clone_વૃક્ષ(ખોટું));
            }
        }
        જો (!nodep->timingControlp()) પાછા;
        // There will be some circular logic here, suppress the warning for newly created vars
        flp->modifyWarnOff(V3ErrorCode::UNOPTFLAT, સાચું);
        // Also suppress the warning for the LHS var, for cases like `assign #1 clk = ~clk;`
        // TODO: Restore the warning for other, non-delayed drivers
        nodep->lhsp()->foreach([](AstVarRef* refp) {
            જો (refp->access().isWriteOrRW()) {
                refp->varp()->fileline()->modifyWarnOff(V3ErrorCode::UNOPTFLAT, સાચું);
            }
        });
        // Should be under an always
        AstAlways* અચલ alwaysp = VN_AS(m_procp, Always);
        // Convert it to an Assign; the new assign with intra delay will be handled by
        // visit(AstNodeAssign*)
        Ast_ગાંઠ_પદાવલિ* અચલ lhs1p = nodep->lhsp()->unlinkFrBack();
        Ast_ગાંઠ_પદાવલિ* અચલ rhs1p = nodep->rhsp()->unlinkFrBack();
        Ast_ગાંઠ* અચલ controlp = nodep->timingControlp()->unlinkFrBack();
        જો (AstDelay* અચલ delayp = VN_CAST(controlp, Delay)) {
            જો (Ast_ગાંઠ_પદાવલિ* fallDelayp = delayp->fallDelay()) {
                fallDelayp = fallDelayp->unlinkFrBack();
                Ast_ગાંઠ_પદાવલિ* lhsp = delayp->lhsp()->unlinkFrBack();
                // Use fall only for an all-zero value, rise otherwise.
                lhsp = delayToIntegral(delayp, lhsp);
                fallDelayp = delayToIntegral(delayp, fallDelayp);
                delayp->lhsp(
                    નવું AstCond{flp, નવું AstEq{flp, rhs1p->clone_વૃક્ષ(ખોટું), નવું AstConst{flp, 0}},
                                fallDelayp, lhsp});
            }
        }
        AstAssign* અચલ assignp = નવું AstAssign{nodep->fileline(), lhs1p, rhs1p, controlp};
        // Put the assignment in a fork..join_none.
        AstFork* અચલ forkp = નવું AstFork{flp, VJoinType::JOIN_NONE};
        nodep->replaceWith(forkp);
        VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
        AstBegin* અચલ beginp = નવું AstBegin{flp, "", assignp, ખોટું};
        forkp->addForksp(beginp);
        visit(forkp);  // Visit now as we need to do some post-processing
        // IEEE 1800-2023 10.3.3 - if the RHS value differs from the currently scheduled value to
        // be assigned, the currently scheduled assignment is descheduled. To keep track if an
        // assignment should be descheduled, each scheduled assignment event has a 'generation',
        // and if at assignment time its generation differs from the current generation, it won't
        // be performed
        AstAssign* અચલ preAssignp = VN_AS(beginp->stmtsp(), Assign);
        UASSERT_OBJ(preAssignp, alwaysp, "Pre-assign should be there from visit(AstFork)");
        AstAssign* અચલ postAssignp = VN_AS(preAssignp->nextp()->nextp(), Assign);
        UASSERT_OBJ(postAssignp, alwaysp, "Post-assign should be there from visit(AstFork)");
        // Increment generation and copy it to a local
        AstVarScope* અચલ generationVarp
            = m_assignWGens.make(flp, m_scopep, alwaysp->findUInt64DType());
        AstVarScope* અચલ genLocalVarp
            = createTemp(flp, generationVarp->varp()->name() + "__local",
                         alwaysp->findUInt64DType(), preAssignp);
        preAssignp->addHereThisAsNext(
            નવું AstAssign{flp, નવું AstVarRef{flp, generationVarp, VAccess::WRITE},
                          નવું AstAdd{flp, નવું AstVarRef{flp, generationVarp, VAccess::READ},
                                     નવું AstConst{flp, 1}}});
        preAssignp->addHereThisAsNext(
            નવું AstAssign{flp, નવું AstVarRef{flp, genLocalVarp, VAccess::WRITE},
                          નવું AstVarRef{flp, generationVarp, VAccess::READ}});
        // If the current generation is same as the one saved in the local var, assign
        beginp->addStmtsp(
            નવું AstIf{flp,
                      નવું AstEq{flp, નવું AstVarRef{flp, generationVarp, VAccess::READ},
                                નવું AstVarRef{flp, genLocalVarp, VAccess::READ}},
                      postAssignp->unlinkFrBack()});
        // Save scheduled RHS value before delay
        AstVarScope* અચલ tmpVarp
            = m_assignWTmps.make(flp, m_scopep, preAssignp->rhsp()->dtypep());
        AstVarRef* અચલ tmpAssignRhsp = VN_AS(preAssignp->lhsp(), VarRef)->clone_વૃક્ષ(ખોટું);
        tmpAssignRhsp->access(VAccess::WRITE);
        preAssignp->addNextHere(
            નવું AstAssign{flp, નવું AstVarRef{flp, tmpVarp, VAccess::WRITE}, tmpAssignRhsp});
        // If the RHS is different from the currently scheduled value, schedule the new assignment
        // The generation will increase, effectively 'descheduling' the previous assignment.
        Ast_ગાંઠ_પદાવલિ* અચલ didNotInitp
            = નવું AstLogNot{flp, નવું AstCExpr{flp, "vlSymsp->__Vm_didInit", 1}};
        AstVarRef* અચલ firstIterp
            = નવું AstVarRef{flp, m_netlistp->stlFirstIterationp(), VAccess::READ};
        Ast_ગાંઠ_પદાવલિ* અચલ schedCondp
            = નવું AstLogOr{flp,
                           નવું AstNeq{flp, preAssignp->rhsp()->clone_વૃક્ષ(ખોટું),
                                      નવું AstVarRef{flp, tmpVarp, VAccess::READ}},
                           નવું AstLogAnd{flp, didNotInitp, firstIterp}};
        alwaysp->addStmtsp(નવું AstIf{flp, schedCondp, forkp->unlinkFrBack()});
    }
    શૂન્ય visit(AstDisableFork* nodep) override {
        જો (m_hasProcess) પાછા;
        // never reached by any process; remove to avoid compilation error
        VL_DO_DANGLING(nodep->unlinkFrBack()->delete_વૃક્ષ(), nodep);
    }
    શૂન્ય visit(AstWaitFork* nodep) override {
        જો (m_hasProcess) {
            File_લીટી* અચલ flp = nodep->fileline();
            AstCExpr* અચલ exprp = નવું AstCExpr{flp, "vlProcess->completedFork()", 1};
            AstWait* અચલ waitp = નવું AstWait{flp, exprp, nullptr};
            nodep->replaceWith(waitp);
        } નહીંતર {
            // never reached by any process; remove to avoid compilation error
            nodep->unlinkFrBack();
        }
        VL_DO_DANGLING(nodep->delete_વૃક્ષ(), nodep);
    }
    શૂન્ય visit(AstWait* nodep) override {
        // Wait on changed events related to the vars in the wait statement
        UINFO(9, "control-visit " << nodep);
        File_લીટી* અચલ flp = nodep->fileline();
        Ast_ગાંઠ* અચલ stmtsp = nodep->stmtsp();
        જો (stmtsp) stmtsp->unlinkFrBackWithNext();
        Ast_ગાંઠ_પદાવલિ* અચલ condp = V3Const::constifyEdit(nodep->condp()->unlinkFrBack());
        auto* અચલ constp = VN_CAST(condp, Const);
        જો (constp) {
            જો (constp->isZero()) {
                // We have to await forever instead of simply returning in case we're deep in a
                // callstack
                AstCMethodHard* અચલ foreverMethodp = નવું AstCMethodHard{
                    flp, નવું AstVarRef{flp, getCreateDelayScheduler(), VAccess::WRITE},
                    VCMethod::SCHED_WAIT_FOREVER};
                foreverMethodp->dtypeSetVoid();
                addProcessInfo(foreverMethodp);
                addDebugInfo(foreverMethodp);
                AstCAwait* અચલ awaitp = નવું AstCAwait{flp, foreverMethodp};
                nodep->replaceWith(awaitp);
                જો (stmtsp) VL_DO_DANGLING(stmtsp->delete_વૃક્ષ(), stmtsp);
                VL_DO_DANGLING(condp->delete_વૃક્ષ(), condp);
            } નહીંતર {
                nodep->v3fatalSrc("constant wait should have been removed in "
                                  "TimingSuspendableVisitor::visit(AstWait)");
            }
        } નહીંતર જો (needDynamicTrigger(condp)) {
            // No point in making a sentree, just use the expression as sensitivity
            // Put the event control in an if so we only wait if the condition isn't met already
            auto* અચલ ifp = નવું AstIf{
                flp, નવું AstLogNot{flp, condp},
                નવું AstEventControl{flp,
                                    નવું Ast_Sen_વૃક્ષ{flp, નવું AstSenItem{flp, VEdgeType::ET_TRUE,
                                                                       condp->clone_વૃક્ષ(ખોટું)}},
                                    nullptr}};
            જો (stmtsp) Ast_ગાંઠ::addNext<Ast_ગાંઠ, Ast_ગાંઠ>(ifp, stmtsp);
            nodep->replaceWith(ifp);
        } નહીંતર {
            // We are using a global sentree, so we cannot use ET_TRUE, as that could lead to the
            // active region never converging. Because of this, we need to use ET_CHANGED in a
            // loop.
            AstEventControl* અચલ controlp = નવું AstEventControl{
                flp,
                નવું Ast_Sen_વૃક્ષ{
                    flp, નવું AstSenItem{flp, VEdgeType::ET_CHANGED, condp->clone_વૃક્ષ(ખોટું)}},
                nullptr};
            controlp->user2(સાચું);  // Commit immediately
            AstLoop* અચલ loopp = નવું AstLoop{flp};
            loopp->addStmtsp(નવું AstLoopTest{flp, loopp, નવું AstLogNot{flp, condp}});
            loopp->addStmtsp(controlp);
            જો (stmtsp) Ast_ગાંઠ::addNext<Ast_ગાંઠ, Ast_ગાંઠ>(loopp, stmtsp);
            nodep->replaceWith(loopp);
        }
        VL_DO_DANGLING(nodep->delete_વૃક્ષ(), nodep);
    }
    શૂન્ય visit(AstBegin* nodep) override {
        VL_RESTORER(m_procp);
        VL_RESTORER(m_hasProcess);
        m_hasProcess |= hasFlags(nodep, T_HAS_PROC);
        // A pending NBA update is not a subprocess of the process that scheduled it, so it is
        // unaffected by 'disable fork' and 'wait fork' (IEEE 1800-2023 9.6.1, 9.6.3)
        જો (hasFlags(nodep, T_NBA_UPDATE)) m_hasProcess = ખોટું;
        m_procp = nodep;
        જો (m_hasProcess) nodep->setNeedProcess();
        iterateChildren(nodep);
    }
    શૂન્ય visit(AstFork* nodep) override {
        જો (nodep->user1SetOnce()) પાછા;
        UINFO(9, "control-visit " << nodep);
        v3Global.setUsesTiming();

        // Create a unique name for this fork
        nodep->name("__Vfork_" + std::to_string(++m_forkCnt));

        // TODO: Should process nodep->stmtsp() in case an earlier pass
        //       inserted something there, but as of today we don't need to.

        // Process and name each fork
        size_t idx = 0;  // Index for naming begins
        માટે (AstBegin *itemp = nodep->forksp(), *nextp; itemp; itemp = nextp) {
            nextp = VN_AS(itemp->nextp(), Begin);
            iterate(itemp);
            // Note: Even if we do not find any awaits, we cannot simply inline
            // the process here, as new awaits could be added later.

            // Name the begin (later the name will be used for a new function)
            itemp->name(nodep->name() + "__" + std::to_string(idx++));
            જો (itemp->needProcess()) {
                itemp->addStmtsp(
                    નવું AstCStmt{itemp->fileline(), "vlProcess->state(VlProcess::FINISHED);"});
            }
        }
        જો (!nodep->joinType().joinNone()) makeForkJoin(nodep);
    }

    //--------------------
    શૂન્ય visit(AstVar*) override {}  // Accelerate
    શૂન્ય visit(Ast_ગાંઠ* nodep) override { iterateChildren(nodep); }

જાહેર:
    // CONSTRUCTORS
    explicit TimingControlVisitor(AstNetlist* nodep)
        : m_netlistp{nodep} {
        iterate(nodep);

        // If there is no static #0 in the design, but an unknown delay was found,
        // and the user did not specify preference, then warn on all unknown delays
        // as we will be assuming they can be #0, which can cause performance degradation.
        માટે (File_લીટી* અચલ flp : m_unknownDelayFlps) {
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

Ast_ગાંઠ_પદાવલિ* V3Timing::delayToTicks(Ast_ગાંઠ_પદાવલિ* valuep, VTimescale timeunit) {
    File_લીટી* અચલ flp = valuep->fileline();
    જો (અચલ AstConst* અચલ constp = VN_CAST(valuep, Const)) {
        જો (constp->અંક().is1Step()) {
            // A step is one tick of the global time precision (IEEE 1800-2023 3.14.3)
            VL_DO_DANGLING(valuep->delete_વૃક્ષ(), valuep);
            AstConst* અચલ tickp = નવું AstConst{flp, AstConst::Unsized64{}, 1};
            tickp->dtypeSetBitSized(64, VSigning::UNSIGNED);
            પાછા tickp;
        }
        જો (constp->isZero()) પાછા valuep;
    }
    UASSERT_OBJ(!timeunit.isNone(), valuep, "timeunit must be set");
    અચલ દ્વિગુણ timescaleFactorD
        = std::pow(10.0, timeunit.powerOfTen() - v3Global.rootp()->timeprecision().powerOfTen());
    જો (valuep->dtypep()->skipRefp()->isDouble()) {
        AstConst* અચલ tsfp = નવું AstConst{flp, AstConst::RealDouble{}, timescaleFactorD};
        valuep = નવું AstMulD{flp, valuep, tsfp};
        valuep = નવું AstRToIRoundS{flp, valuep};
        valuep->dtypeSetBitSized(64, VSigning::UNSIGNED);
    } નહીંતર {
        valuep->dtypeSetBitSized(64, VSigning::UNSIGNED);
        અચલ uint64_t timescaleFactorU = static_cast<uint64_t>(timescaleFactorD);
        AstConst* અચલ tsfp = નવું AstConst{flp, AstConst::Unsized64{}, timescaleFactorU};
        valuep = નવું AstMul{flp, valuep, tsfp};
    }
    પાછા V3Const::constifyEdit(valuep);  // Simplify
}

શૂન્ય V3Timing::timingAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        અચલ VNUser1InUse m_user1InUse;
        અચલ VNUser2InUse m_user2InUse;
        { TimingSuspendableVisitor{nodep}; }
        જો (v3Global.usesTiming()) TimingControlVisitor{nodep};
    }
    V3Global::dump_Check_Global_વૃક્ષ("timing", 0, dump_વૃક્ષ_Either_Level() >= 3);
}
