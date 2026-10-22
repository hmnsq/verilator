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

#অন্তর্ভাব "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#অন্তর্ভাব "V3Timing.h"

#অন্তর্ভাব "V3Const.h"
#অন্তর্ভাব "V3EmitV.h"
#অন্তর্ভাব "V3File.h"
#অন্তর্ভাব "V3Global.h"
#অন্তর্ভাব "V3Graph.h"
#অন্তর্ভাব "V3MemberMap.h"
#অন্তর্ভাব "V3SenExprBuilder.h"
#অন্তর্ভাব "V3SenTree.h"
#অন্তর্ভাব "V3SharedTmps.h"
#অন্তর্ভাব "V3Stats.h"
#অন্তর্ভাব "V3UniqueNames.h"

#অন্তর্ভাব <limits>
#অন্তর্ভাব <queue>

VL_DEFINE_DEBUG_FUNCTIONS;

// ######################################################################

গণনা গ্রন্থি_Flag : uint8_t {
    T_SUSPENDEE = 1 << 0,  // Suspendable (due to dependence on another suspendable)
    T_SUSPENDER = 1 << 1,  // Suspendable (has timing control)
    T_ALLOCS_PROC = 1 << 2,  // Can allocate VlProcess
    T_FORCES_PROC = 1 << 3,  // Forces VlProcess allocation
    T_NEEDS_PROC = 1 << 4,  // Needs access to VlProcess if it's allocated
    T_HAS_PROC = 1 << 5,  // Has VlProcess argument in the signature
    T_NBA_UPDATE = 1 << 6,  // Fork branch of a pending NBA update, which is not a subprocess
};

গণনা ForkType : uint8_t {
    F_NONE = 0,  // Not under a fork
    F_MIGHT_SUSPEND = 1 << 0,  // Fork might suspend the execution of current process
    F_MIGHT_NEED_PROC = 1 << 1,  // Fork might need a process (any fork really)
};

গণনা PropagationType : uint8_t {
    P_CALL = 1,  // Propagation through call to a function/task/method
    P_FORK = 2,  // Propagation due to fork's behavior
    P_SIGNATURE = 3,  // Propagation required to maintain C++ function's signature requirements
};

// Add timing flag to a node
স্থির শূন্য addFlags(Ast_গ্রন্থি* ধ্রুব nodep, uint8_t flags) { nodep->user2Or(flags); }
// Check if a node has ALL of the expected flags set
স্থির তার্কিক hasFlags(Ast_গ্রন্থি* ধ্রুব nodep, uint8_t flags) { প্রত্যাগম !(~nodep->user2() & flags); }

// ######################################################################
//  Detect nodes affected by timing and/or requiring a process

বর্গ TimingSuspendableVisitor final : সার্বজনিক VNVisitor {
    // TYPES
    // Vertex of a dependency graph of suspendable nodes, e.g. if a node (process or task) is
    // suspendable, all its dependents should also be suspendable
    বর্গ DepVtx VL_NOT_FINAL : সার্বজনিক V3GraphVertex {
        VL_RTTI_IMPL(DepVtx, V3GraphVertex)
        AstClass* ধ্রুব m_classp;  // Class associated with a method
        Ast_গ্রন্থি* ধ্রুব m_nodep;  // AST node represented by this graph vertex

        // ACCESSORS
        string name() ধ্রুব override {
            যদি (m_classp) {
                যদি (VN_IS(nodep(), CFunc)) {
                    প্রত্যাগম cvtToHex(nodep()) + ' ' + classp()->name() + "::" + nodep()->name();
                }
            }
            প্রত্যাগম cvtToHex(nodep()) + ' ' + nodep()->prettyTypeName();
        }
        File_সারি* fileline() ধ্রুব override { প্রত্যাগম nodep()->fileline(); }

    সার্বজনিক:
        // CONSTRUCTORS
        DepVtx(V3Graph* graphp, Ast_গ্রন্থি* nodep, AstClass* classp)
            : V3GraphVertex{graphp}
            , m_classp{classp}
            , m_nodep{nodep} {}
        ~DepVtx() override = স্বভাব;

        // ACCESSORS
        আভাস Ast_গ্রন্থি* nodep() ধ্রুব VL_MT_STABLE { প্রত্যাগম m_nodep; }
        আভাস Ast_গ্রন্থি* classp() ধ্রুব VL_MT_STABLE { প্রত্যাগম m_classp; }
    };

    বর্গ SuspendDepVtx final : সার্বজনিক DepVtx {
        VL_RTTI_IMPL(SuspendDepVtx, DepVtx)
        string dotColor() ধ্রুব override {
            যদি (hasFlags(nodep(), T_SUSPENDER)) প্রত্যাগম "red";
            যদি (hasFlags(nodep(), T_SUSPENDEE)) প্রত্যাগম "blue";
            প্রত্যাগম "black";
        }

    সার্বজনিক:
        SuspendDepVtx(V3Graph* graphp, Ast_গ্রন্থি* nodep, AstClass* classp)
            : DepVtx{graphp, nodep, classp} {}
        ~SuspendDepVtx() override = স্বভাব;
    };

    বর্গ NeedsProcDepVtx final : সার্বজনিক DepVtx {
        VL_RTTI_IMPL(NeedsProcDepVtx, DepVtx)
        string dotColor() ধ্রুব override {
            যদি (hasFlags(nodep(), T_HAS_PROC)) প্রত্যাগম "blue";
            যদি (hasFlags(nodep(), T_NEEDS_PROC)) প্রত্যাগম "green";
            যদি (hasFlags(nodep(), T_FORCES_PROC)) প্রত্যাগম "red";
            প্রত্যাগম "black";
        }

    সার্বজনিক:
        NeedsProcDepVtx(V3Graph* graphp, Ast_গ্রন্থি* nodep, AstClass* classp)
            : DepVtx{graphp, nodep, classp} {}
        ~NeedsProcDepVtx() override = স্বভাব;
    };

    // NODE STATE
    //  AstClass::user1() -> bool.  Class member cache has been refreshed.
    //  Ast{NodeProcedure,CFunc,Begin}::user2()  -> uint64_t.  Set to >= T_SUSP if
    //                                                         process/task suspendable
    //                                                         and to T_PROC if it
    //                                                         needs process metadata.
    //  Ast{NodeProcedure,CFunc,Begin}::user3()  -> DependencyVertex*.  Vertex in m_suspGraph
    //  Ast{NodeProcedure,CFunc,Begin}::user3()  -> DependencyVertex*.  Vertex in m_procGraph
    ধ্রুব VNUser3InUse m_user3InUse;
    ধ্রুব VNUser4InUse m_user4InUse;

    // STATE
    VMemberMap m_memberMap;  // Member names cached for fast lookup
    AstClass* m_classp = nullptr;  // Current class
    Ast_গ্রন্থি* m_procp = nullptr;  // NodeProcedure/CFunc/Begin we're under
    uint8_t m_underFork = F_NONE;  // F_NONE or flags of a fork we are under
    V3Graph m_suspGraph;  // Dependency graph where a node is a dependency of another if it being
                          // suspendable makes the other node suspendable
    V3Graph m_procGraph;  // Dependency graph where a node is a dependency of another if it being
                          // suspendable makes the other node suspendable

    // METHODS
    // Get or create the dependency vertex for the given node
    DepVtx* getSuspendDepVtx(Ast_গ্রন্থি* ধ্রুব nodep) {
        AstClass* classp = nullptr;
        যদি (AstCFunc* funcp = VN_CAST(nodep, CFunc)) {
            যদি (funcp->scopep() && funcp->scopep()->modp()) {
                classp = VN_CAST(funcp->scopep()->modp(), Class);
            }
        }
        যদি (!nodep->user3p()) nodep->user3p(নূতন SuspendDepVtx{&m_suspGraph, nodep, classp});
        প্রত্যাগম nodep->user3u().to<SuspendDepVtx*>();
    }
    DepVtx* getNeedsProcDepVtx(Ast_গ্রন্থি* ধ্রুব nodep) {
        AstClass* classp = nullptr;
        যদি (AstCFunc* funcp = VN_CAST(nodep, CFunc)) {
            যদি (funcp->scopep() && funcp->scopep()->modp()) {
                classp = VN_CAST(funcp->scopep()->modp(), Class);
            }
        }
        যদি (!nodep->user4p()) nodep->user4p(নূতন NeedsProcDepVtx{&m_procGraph, nodep, classp});
        প্রত্যাগম nodep->user4u().to<NeedsProcDepVtx*>();
    }
    // Pass timing flag between nodes
    তার্কিক passFlag(ধ্রুব Ast_গ্রন্থি* from, Ast_গ্রন্থি* to, গ্রন্থি_Flag flag) {
        যদি ((from->user2() & flag) && !(to->user2() & flag)) {
            addFlags(to, flag);
            প্রত্যাগম সত্য;
        }
        প্রত্যাগম অসত্য;
    }
    // Propagate flag to all nodes that depend on the given one
    শূন্য propagateFlags(DepVtx* ধ্রুব vxp, গ্রন্থি_Flag flag) {
        auto* ধ্রুব parentp = vxp->nodep();
        প্রতি (V3GraphEdge& edge : vxp->outEdges()) {
            auto* ধ্রুব depVxp = static_cast<DepVtx*>(edge.শীর্ষ());
            Ast_গ্রন্থি* ধ্রুব depp = depVxp->nodep();
            যদি (passFlag(parentp, depp, flag)) propagateFlags(depVxp, flag);
        }
    }
    template <typename T_Predicate>
    শূন্য propagateFlagsIf(DepVtx* ধ্রুব vxp, গ্রন্থি_Flag flag, T_Predicate p) {
        auto* ধ্রুব parentp = vxp->nodep();
        প্রতি (V3GraphEdge& edge : vxp->outEdges()) {
            auto* ধ্রুব depVxp = static_cast<DepVtx*>(edge.শীর্ষ());
            Ast_গ্রন্থি* ধ্রুব depp = depVxp->nodep();
            যদি (p(&edge) && passFlag(parentp, depp, flag)) propagateFlagsIf(depVxp, flag, p);
        }
    }
    template <typename T_Predicate>
    শূন্য propagateFlagsReversedIf(DepVtx* ধ্রুব vxp, গ্রন্থি_Flag flag, T_Predicate p) {
        auto* ধ্রুব parentp = vxp->nodep();
        প্রতি (V3GraphEdge& edge : vxp->inEdges()) {
            auto* ধ্রুব depVxp = static_cast<DepVtx*>(edge.fromp());
            Ast_গ্রন্থি* ধ্রুব depp = depVxp->nodep();
            যদি (p(&edge) && passFlag(parentp, depp, flag))
                propagateFlagsReversedIf(depVxp, flag, p);
        }
    }

    // VISITORS
    শূন্য visit(AstClass* nodep) override {
        UASSERT_OBJ(!m_classp, nodep, "Class under class");
        VL_RESTORER(m_classp);
        m_classp = nodep;
        iterateChildren(nodep);
    }
    শূন্য visit(Ast_গ্রন্থি_Procedure* nodep) override {
        VL_RESTORER(m_procp);
        m_procp = nodep;
        getNeedsProcDepVtx(nodep);
        addFlags(nodep, T_ALLOCS_PROC);
        যদি (VN_IS(nodep, Always)) {
            UINFO(9, "Always does " << (nodep->needProcess() ? "" : "NOT ") << "need process");
        }
        iterateChildren(nodep);
    }
    শূন্য visit(AstDisableFork* nodep) override {
        visit(static_cast<Ast_গ্রন্থি*>(nodep));
        addFlags(m_procp, T_FORCES_PROC | T_NEEDS_PROC);
    }
    শূন্য visit(AstWaitFork* nodep) override {
        visit(static_cast<Ast_গ্রন্থি*>(nodep));
        addFlags(m_procp, T_FORCES_PROC | T_NEEDS_PROC);
    }
    শূন্য visit(AstWait* nodep) override {
        UINFO(9, "suspendable-visit " << nodep);
        Ast_গ্রন্থি_রাশিমালা* ধ্রুব condp = V3Const::constifyEdit(nodep->condp());
        যদি (AstConst* ধ্রুব constp = VN_CAST(condp, Const)) {
            যদি (!nodep->fileline()->warnIsOff(V3ErrorCode::WAITCONST)) {
                condp->v3warn(WAITCONST, "Wait statement condition is constant");
            }
            যদি (!constp->isZero()) {
                // Remove AstWait before we track process as T_SUSPENDER
                যদি (Ast_গ্রন্থি* ধ্রুব stmtsp = nodep->stmtsp()) {
                    stmtsp->unlinkFrBackWithNext();
                    nodep->replaceWith(stmtsp);
                } অন্যথা {
                    nodep->unlinkFrBack();
                }
                VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
                প্রত্যাগম;
            }
        }
        v3Global.setUsesTiming();
        যদি (m_procp) addFlags(m_procp, T_SUSPENDEE | T_SUSPENDER | T_NEEDS_PROC);
        iterateChildren(nodep);
    }
    শূন্য visit(AstCFunc* nodep) override {
        VL_RESTORER(m_procp);
        m_procp = nodep;
        iterateChildren(nodep);
        যদি (nodep->needProcess()) addFlags(nodep, T_FORCES_PROC | T_NEEDS_PROC);
        DepVtx* ধ্রুব sVxp = getSuspendDepVtx(nodep);
        DepVtx* ধ্রুব pVxp = getNeedsProcDepVtx(nodep);
        যদি (!m_classp) প্রত্যাগম;

        // Go over overridden functions

        std::queue<AstClassExtends*> extends;
        যদি (m_classp->extendsp()) extends.ঠেলো(m_classp->extendsp());

        যাবত্ (!extends.empty()) {
            AstClassExtends* ext_list = extends.front();
            extends.বের_করো();

            প্রতি (AstClassExtends* cextp = ext_list; cextp;
                 cextp = VN_AS(cextp->nextp(), ClassExtends)) {
                // TODO: It is possible that a methods the same name in the base class is not
                // actually overridden by our method. If this causes a problem, traverse to
                // the root of the inheritance hierarchy and check if the original method is
                // virtual or not.
                যদি (auto* ধ্রুব overriddenp
                    = VN_CAST(m_memberMap.findMember(cextp->classp(), nodep->name()), CFunc)) {
                    // Suspendability and process affects typing, so they propagate both ways
                    DepVtx* ধ্রুব overriddenSVxp = getSuspendDepVtx(overriddenp);
                    DepVtx* ধ্রুব overriddenPVxp = getNeedsProcDepVtx(overriddenp);
                    নূতন V3GraphEdge{&m_suspGraph, sVxp, overriddenSVxp, P_SIGNATURE};
                    নূতন V3GraphEdge{&m_suspGraph, overriddenSVxp, sVxp, P_SIGNATURE};
                    নূতন V3GraphEdge{&m_procGraph, pVxp, overriddenPVxp, P_SIGNATURE};
                    নূতন V3GraphEdge{&m_procGraph, overriddenPVxp, pVxp, P_SIGNATURE};
                } অন্যথা {
                    AstClassExtends* more_extends = cextp->classp()->extendsp();
                    যদি (more_extends) extends.ঠেলো(more_extends);
                }
            }
        }
    }
    শূন্য visit(Ast_গ্রন্থি_CCall* nodep) override {
        AstCFunc* ধ্রুব funcp = nodep->funcp();
        UASSERT_OBJ(funcp, nodep, "AstNodeCCall must have non-null funcp post-link");
        UASSERT_OBJ(m_procp, nodep, "AstNodeCCall must be inside a procedure/CFunc/Begin");

        UINFO(9, "V3Timing: Processing CCall to " << funcp->name() << " in dependency graph\n");
        নূতন V3GraphEdge{&m_suspGraph, getSuspendDepVtx(funcp), getSuspendDepVtx(m_procp), P_CALL};

        নূতন V3GraphEdge{&m_procGraph, getNeedsProcDepVtx(funcp), getNeedsProcDepVtx(m_procp),
                        P_CALL};

        iterateChildren(nodep);
    }
    শূন্য visit(AstBegin* nodep) override {
        VL_RESTORER(m_procp);
        VL_RESTORER(m_underFork);

        যদি (!m_underFork || (m_underFork & F_MIGHT_SUSPEND))
            নূতন V3GraphEdge{&m_suspGraph, getSuspendDepVtx(nodep), getSuspendDepVtx(m_procp),
                            m_underFork ? P_FORK : P_CALL};

        নূতন V3GraphEdge{&m_procGraph, getNeedsProcDepVtx(nodep), getNeedsProcDepVtx(m_procp),
                        P_CALL};

        যদি (m_underFork) addFlags(nodep, T_NEEDS_PROC | T_ALLOCS_PROC);

        m_procp = nodep;
        m_underFork = 0;
        iterateChildren(nodep);
    }
    শূন্য visit(AstFork* nodep) override {
        UINFO(9, "suspendable-visit " << nodep);
        VL_RESTORER(m_underFork);

        v3Global.setUsesTiming();  // Even if there are no event controls, we have to set this flag
                                   // so that transformForks() in V3SchedTiming gets called and
                                   // removes all forks and begins
        যদি (nodep->isTimingControl() && m_procp) {
            addFlags(m_procp, T_SUSPENDEE | T_SUSPENDER);
            m_underFork |= F_MIGHT_SUSPEND;
        }
        m_underFork |= F_MIGHT_NEED_PROC;
        iterateChildren(nodep);
    }
    শূন্য visit(AstAssignDly* nodep) override {
        যদি (!VN_IS(m_procp, গ্রন্থি_Procedure)) v3Global.setUsesTiming();
        visit(static_cast<Ast_গ্রন্থি*>(nodep));
    }
    শূন্য visit(AstFireEvent* nodep) override {
        যদি (nodep->isDelayed()
            && (!VN_IS(m_procp, গ্রন্থি_Procedure) || !VN_IS(nodep->operandp(), VarRef))) {
            v3Global.setUsesTiming();
        }
        visit(static_cast<Ast_গ্রন্থি*>(nodep));
    }
    শূন্য visit(AstAssignW* nodep) override {
        যদি (nodep->timingControlp()) v3Global.setUsesTiming();
        // Containing process will not suspend, don't mark it
    }
    শূন্য visit(Ast_গ্রন্থি* nodep) override {
        যদি (nodep->isTimingControl()) {
            v3Global.setUsesTiming();
            যদি (m_procp) addFlags(m_procp, T_SUSPENDEE | T_SUSPENDER | T_NEEDS_PROC);
        }
        iterateChildren(nodep);
    }

    //--------------------
    শূন্য visit(AstVar*) override {}  // Accelerate

সার্বজনিক:
    // CONSTRUCTORS
    explicit TimingSuspendableVisitor(AstNetlist* nodep) {
        iterate(nodep);
        m_suspGraph.removeTransitiveEdges();
        m_procGraph.removeTransitiveEdges();
        // Propagate suspendability
        প্রতি (V3GraphVertex& vtx : m_suspGraph.vertices()) {
            DepVtx& depVtx = static_cast<DepVtx&>(vtx);
            যদি (hasFlags(depVtx.nodep(), T_SUSPENDEE)) propagateFlags(&depVtx, T_SUSPENDEE);
        }
        যদি (dumpGraphLevel() >= 6) m_suspGraph.dumpDotFilePrefixed("timing_deps");

        // Propagate T_HAS_PROCESS
        প্রতি (V3GraphVertex& vtx : m_procGraph.vertices()) {
            DepVtx& depVtx = static_cast<DepVtx&>(vtx);
            // Find processes that'll allocate VlProcess
            যদি (hasFlags(depVtx.nodep(), T_FORCES_PROC)) {
                propagateFlagsIf(&depVtx, T_FORCES_PROC, [&](ধ্রুব V3GraphEdge* e) -> তার্কিক {
                    প্রত্যাগম !hasFlags(static_cast<DepVtx*>(e->fromp())->nodep(), T_ALLOCS_PROC);
                });
            }
            // Mark nodes on paths between processes and statements that use VlProcess
            যদি (hasFlags(depVtx.nodep(), T_NEEDS_PROC)) {
                propagateFlagsIf(&depVtx, T_NEEDS_PROC, [&](ধ্রুব V3GraphEdge* e) -> তার্কিক {
                    প্রত্যাগম !hasFlags(static_cast<DepVtx*>(e->শীর্ষ())->nodep(), T_ALLOCS_PROC);
                });
            }
        }
        প্রতি (V3GraphVertex& vtx : m_procGraph.vertices()) {
            DepVtx& depVtx = static_cast<DepVtx&>(vtx);
            // Mark nodes that will be emitted with a VlProcess argument
            যদি (hasFlags(depVtx.nodep(), T_ALLOCS_PROC | T_FORCES_PROC)) {
                addFlags(depVtx.nodep(), T_HAS_PROC);
                propagateFlagsReversedIf(&depVtx, T_HAS_PROC, [&](ধ্রুব V3GraphEdge* e) -> তার্কিক {
                    প্রত্যাগম hasFlags(static_cast<DepVtx*>(e->fromp())->nodep(), T_NEEDS_PROC);
                });
            }
        }
        যদি (dumpGraphLevel() >= 6) m_procGraph.dumpDotFilePrefixed("proc_deps");
    }
    ~TimingSuspendableVisitor() override = স্বভাব;
};

// ######################################################################
//  Transform nodes affected by timing

বর্গ TimingControlVisitor final : সার্বজনিক VNVisitor {
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
    AstNetlist* ধ্রুব m_netlistp;  // Root node
    Ast_শীর্ষ_Scope* ধ্রুব m_শীর্ষ_Scopep = m_netlistp->শীর্ষ_Scopep();  // The AstTopScope
    AstScope* ধ্রুব m_scopeTopp = m_শীর্ষ_Scopep->scopep();  // Scope at the top
    AstClass* m_classp = nullptr;  // Current class
    AstScope* m_scopep = nullptr;  // Current scope
    AstActive* m_activep = nullptr;  // Current active
    Ast_গ্রন্থি* m_procp = nullptr;  // NodeProcedure/CFunc/Begin we're under
    তার্কিক m_hasProcess = অসত্য;  // True if current scope has a VlProcess handle available
    পূর্ণাঙ্ক m_forkCnt = 0;  // Number of forks inside a module
    তার্কিক m_underJumpBlock = অসত্য;  // True if we are inside of a jump-block
    তার্কিক m_underProcedure = অসত্য;  // True if we are under an always or initial
    তার্কিক m_underIfaceCFunc = অসত্য;  // True if we are under a CFunc owned by an interface scope
    তার্কিক m_hasStaticZeroDelay = অসত্য;  // True if we have a static #0 delay
    std::vector<File_সারি*> m_unknownDelayFlps;  // Locations of AstDelay with non-constant value

    // Unique names
    V3UniqueNames m_dlyforkNames{"__Vdlyfork"};  // Names for temp AssignW vars
    V3UniqueNames m_intra_মান_Names{"__Vintraval"};  // Intra assign delay value var names
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
    Ast_Sen_বৃক্ষ* m_delaySensesp = nullptr;  // Domain to trigger if a delayed coroutine is resumed
    Ast_Sen_বৃক্ষ* m_dynamicSensesp = nullptr;  // Domain to trigger if a dynamic trigger is set

    // Other
    Sen_বৃক্ষ_Finder m_finder{m_netlistp};  // Sentree finder and uniquifier
    Sen_রাশিমালা_Builder* m_sen_রাশিমালা_Builderp = nullptr;  // Sens expression builder for current m_scope

    // Stats
    size_t m_statZeroDelays = 0;  // Number of statically known #0 delays
    size_t m_statConstDelays = 0;  // Number of statically known #const (non-zero) delays
    size_t m_statVariableDelays = 0;  // Number of delays with value unknown at compile time

    // METHODS
    // Transform an assignment with an intra timing control into a timing control with the
    // assignment under it
    Ast_গ্রন্থি* factorOutTimingControl(Ast_গ্রন্থি_Assign* nodep) ধ্রুব {
        Ast_গ্রন্থি* stmtp = nodep;
        Ast_গ্রন্থি* ধ্রুব controlp = nodep->timingControlp();
        যদি (AstDelay* ধ্রুব delayp = VN_CAST(controlp, Delay)) {
            stmtp->replaceWith(delayp->unlinkFrBack());
            delayp->addStmtsp(stmtp);
            stmtp = delayp;
        } অন্যথা যদি (Ast_Sen_বৃক্ষ* ধ্রুব sentreep = VN_CAST(controlp, Sen_বৃক্ষ)) {
            AstEventControl* ধ্রুব eventControlp
                = নূতন AstEventControl{sentreep->fileline(), sentreep->unlinkFrBack(), nullptr};
            stmtp->replaceWith(eventControlp);
            eventControlp->addStmtsp(stmtp);
            stmtp = eventControlp;
        } অন্যথা যদি (AstBegin* ধ্রুব beginp = VN_CAST(controlp, Begin)) {
            // Begin from V3AssertPre
            stmtp->replaceWith(beginp->unlinkFrBack());
            beginp->addStmtsp(stmtp);
            stmtp = beginp;
        }
        প্রত্যাগম stmtp == nodep ? nullptr : stmtp;
    }
    স্থির তার্কিক staticallyNonZeroDelay(ধ্রুব Ast_গ্রন্থি_রাশিমালা* valuep) {
        যদি (ধ্রুব AstConst* ধ্রুব constp = VN_CAST(valuep, Const)) প্রত্যাগম !constp->isZero();
        যদি (ধ্রুব AstCond* ধ্রুব condp = VN_CAST(valuep, Cond)) {
            প্রত্যাগম staticallyNonZeroDelay(condp->thenp())
                   && staticallyNonZeroDelay(condp->elsep());
        }
        যদি (ধ্রুব AstMul* ধ্রুব mulp = VN_CAST(valuep, Mul)) {
            ধ্রুব AstConst* ধ্রুব lhsConstp = VN_CAST(mulp->lhsp(), Const);
            ধ্রুব AstConst* ধ্রুব rhsConstp = VN_CAST(mulp->rhsp(), Const);
            যদি (lhsConstp && !lhsConstp->isZero()) প্রত্যাগম staticallyNonZeroDelay(mulp->rhsp());
            যদি (rhsConstp && !rhsConstp->isZero()) প্রত্যাগম staticallyNonZeroDelay(mulp->lhsp());
        }
        প্রত্যাগম অসত্য;
    }
    // Creates the global delay scheduler variable
    AstVarScope* getCreateDelayScheduler() {
        যদি (m_delaySchedp) প্রত্যাগম m_delaySchedp;
        auto* ধ্রুব dlySchedDtp = নূতন AstBasicDType{
            m_scopeTopp->fileline(), VBasicDTypeKwd::DELAY_SCHEDULER, VSigning::UNSIGNED};
        m_netlistp->typeTablep()->addTypesp(dlySchedDtp);
        m_delaySchedp = m_শীর্ষ_Scopep->createTemp("__VdlySched", dlySchedDtp);
        // Delay scheduler has to be accessible from top
        m_delaySchedp->varp()->sigPublic(সত্য);
        m_netlistp->delaySchedulerp(m_delaySchedp->varp());
        প্রত্যাগম m_delaySchedp;
    }
    // Creates the delay sentree
    Ast_Sen_বৃক্ষ* get_Create_Delay_Sen_বৃক্ষ() {
        যদি (m_delaySensesp) প্রত্যাগম m_delaySensesp;
        File_সারি* ধ্রুব flp = m_scopeTopp->fileline();
        auto* ধ্রুব awaitingCurrentTimep
            = নূতন AstCMethodHard{flp, নূতন AstVarRef{flp, getCreateDelayScheduler(), VAccess::READ},
                                 VCMethod::SCHED_AWAITING_CURRENT_TIME};
        awaitingCurrentTimep->dtypeSetBit();
        m_delaySensesp
            = নূতন Ast_Sen_বৃক্ষ{flp, নূতন AstSenItem{flp, VEdgeType::ET_TRUE, awaitingCurrentTimep}};
        m_netlistp->শীর্ষ_Scopep()->addSenTreesp(m_delaySensesp);
        প্রত্যাগম m_delaySensesp;
    }
    // Creates the global dynamic trigger scheduler variable
    AstVarScope* getCreateDynamicTriggerScheduler() {
        যদি (m_dynamicSchedp) প্রত্যাগম m_dynamicSchedp;
        auto* ধ্রুব dynSchedDtp
            = নূতন AstBasicDType{m_scopeTopp->fileline(), VBasicDTypeKwd::DYNAMIC_TRIGGER_SCHEDULER,
                                VSigning::UNSIGNED};
        m_netlistp->typeTablep()->addTypesp(dynSchedDtp);
        m_dynamicSchedp = m_শীর্ষ_Scopep->createTemp("__VdynSched", dynSchedDtp);
        প্রত্যাগম m_dynamicSchedp;
    }
    // Creates the dynamic trigger sentree
    Ast_Sen_বৃক্ষ* get_Create_Dynamic_Trigger_Sen_বৃক্ষ() {
        যদি (m_dynamicSensesp) প্রত্যাগম m_dynamicSensesp;
        File_সারি* ধ্রুব flp = m_scopeTopp->fileline();
        auto* ধ্রুব awaitingCurrentTimep = নূতন AstCMethodHard{
            flp, নূতন AstVarRef{flp, getCreateDynamicTriggerScheduler(), VAccess::READ},
            VCMethod::SCHED_EVALUATE};
        awaitingCurrentTimep->dtypeSetBit();
        m_dynamicSensesp
            = নূতন Ast_Sen_বৃক্ষ{flp, নূতন AstSenItem{flp, VEdgeType::ET_TRUE, awaitingCurrentTimep}};
        m_netlistp->শীর্ষ_Scopep()->addSenTreesp(m_dynamicSensesp);
        প্রত্যাগম m_dynamicSensesp;
    }
    // Creates the event variable to trigger in NBA region
    AstEventControl* createNbaEventControl(File_সারি* flp) {
        যদি (!m_netlistp->nbaEventp()) {
            auto* ধ্রুব nbaEventDtp = নূতন AstBasicDType{m_scopeTopp->fileline(),
                                                        VBasicDTypeKwd::EVENT, VSigning::UNSIGNED};
            m_netlistp->typeTablep()->addTypesp(nbaEventDtp);
            m_netlistp->nbaEventp(m_শীর্ষ_Scopep->createTemp("__VnbaEvent", nbaEventDtp));
            v3Global.setHasEvents();
        }
        প্রত্যাগম নূতন AstEventControl{
            flp,
            নূতন Ast_Sen_বৃক্ষ{
                flp, নূতন AstSenItem{flp, VEdgeType::ET_EVENT,
                                    নূতন AstVarRef{flp, m_netlistp->nbaEventp(), VAccess::READ}}},
            nullptr};
    }
    // Creates the variable that, if set, causes the NBA event to be triggered
    AstAssign* createNbaEventTriggerAssignment(File_সারি* flp) {
        যদি (!m_netlistp->nbaEventTriggerp()) {
            m_netlistp->nbaEventTriggerp(m_শীর্ষ_Scopep->createTemp("__VnbaEventTrigger", 1));
        }
        প্রত্যাগম নূতন AstAssign{flp,
                             নূতন AstVarRef{flp, m_netlistp->nbaEventTriggerp(), VAccess::WRITE},
                             নূতন AstConst{flp, AstConst::BitTrue{}}};
    }
    // Returns true if we are under a class or interface function, or the tree references locals.
    // These are cases where static, globally-evaluated triggers are not suitable.
    তার্কিক needDynamicTrigger(Ast_গ্রন্থি* ধ্রুব nodep) ধ্রুব {
        প্রত্যাগম m_classp || m_underIfaceCFunc || nodep->exists([](Ast_গ্রন্থি* ধ্রুব nodep) {
            যদি (Ast_গ্রন্থি_Var_Ref* varp = VN_CAST(nodep, গ্রন্থি_Var_Ref)) {
                প্রত্যাগম varp->varp()->isFuncLocal();
            }
            প্রত্যাগম !nodep->isPure();
        });
    }
    // Creates a trigger scheduler variable
    AstVarScope* getCreateTriggerSchedulerp(Ast_Sen_বৃক্ষ* ধ্রুব sentreep) {
        যদি (!sentreep->user1p()) {
            যদি (!m_trigSchedDtp) {
                m_trigSchedDtp
                    = নূতন AstBasicDType{m_scopeTopp->fileline(), VBasicDTypeKwd::TRIGGER_SCHEDULER,
                                        VSigning::UNSIGNED};
                m_netlistp->typeTablep()->addTypesp(m_trigSchedDtp);
            }
            AstVarScope* ধ্রুব trigSchedp
                = m_শীর্ষ_Scopep->createTemp(m_trigSchedNames.get(sentreep), m_trigSchedDtp);
            sentreep->user1p(trigSchedp);
        }
        প্রত্যাগম VN_AS(sentreep->user1p(), VarScope);
    }
    // Creates a string describing the sentree
    AstCExpr* createEventDescription(Ast_Sen_বৃক্ষ* ধ্রুব sentreep) ধ্রুব {
        যদি (!sentreep->user2p()) {
            std::stringstream ss;
            V3EmitV::verilog_For_বৃক্ষ(sentreep, ss);
            // possibly a multiline string
            // Names were hashed by V3Name, show the user the original text
            std::string comment = VName::dehash(ss.str());
            std::replace(comment.begin(), comment.end(), '\n', ' ');
            // Escape quotes, e.g. from string-indexed event array selects
            comment = '"' + V3OutFormatter::quoteNameControls(comment) + '"';
            AstCExpr* ধ্রুব commentp = নূতন AstCExpr{sentreep->fileline(), comment};
            commentp->dtypeSetString();
            sentreep->user2p(commentp);
            প্রত্যাগম commentp;
        }
        প্রত্যাগম VN_AS(sentreep->user2p(), CExpr)->clone_বৃক্ষ(অসত্য);
    }
    // Adds debug info to a hardcoded method call
    শূন্য addDebugInfo(AstCMethodHard* ধ্রুব methodp) ধ্রুব {
        File_সারি* ধ্রুব flp = methodp->fileline();
        ধ্রুব তার্কিক protectIds = v3Global.opt.protectIds();
        AstCExpr* ধ্রুব ap
            = নূতন AstCExpr{flp, protectIds ? "VL_UNKNOWN" : '"' + flp->filenameEsc() + '"'};
        ap->dtypeSetString();
        methodp->addPinsp(ap);
        AstCExpr* ধ্রুব bp = নূতন AstCExpr{flp, protectIds ? "0" : cvtToStr(flp->lineno())};
        bp->dtypeSetString();
        methodp->addPinsp(bp);
    }
    // Adds debug info to a trigSched.trigger() call
    শূন্য addEventDebugInfo(AstCMethodHard* ধ্রুব methodp, Ast_Sen_বৃক্ষ* ধ্রুব sentreep) ধ্রুব {
        যদি (v3Global.opt.protectIds()) {
            File_সারি* ধ্রুব flp = sentreep->fileline();
            AstCExpr* ধ্রুব descp = নূতন AstCExpr{flp, "VL_UNKNOWN"};
            descp->dtypeSetString();
            methodp->addPinsp(descp);
            addDebugInfo(methodp);
            প্রত্যাগম;
        }
        methodp->addPinsp(createEventDescription(sentreep));
        addDebugInfo(methodp);
    }
    // Adds process pointer to a hardcoded method call
    শূন্য addProcessInfo(AstCMethodHard* ধ্রুব methodp) ধ্রুব {
        File_সারি* ধ্রুব flp = methodp->fileline();
        AstCExpr* ধ্রুব ap
            = নূতন AstCExpr{flp, (m_procp && m_hasProcess) ? "vlProcess" : "nullptr"};
        methodp->addPinsp(ap);
    }
    // Creates the fork handle type and returns it
    AstBasicDType* getCreateForkSyncDTypep() {
        যদি (m_forkDtp) প্রত্যাগম m_forkDtp;
        m_forkDtp = নূতন AstBasicDType{m_scopeTopp->fileline(), VBasicDTypeKwd::FORK_SYNC,
                                      VSigning::UNSIGNED};
        m_netlistp->typeTablep()->addTypesp(m_forkDtp);
        প্রত্যাগম m_forkDtp;
    }
    // Move `nodep` into `AstCLocalScope`.
    AstCLocalScope* addCLocalScope(File_সারি* ধ্রুব flp, Ast_গ্রন্থি* ধ্রুব nodep) ধ্রুব {
        যদি (!nodep) প্রত্যাগম nullptr;
        VNRelinker handle;
        nodep->unlinkFrBack(&handle);
        AstCLocalScope* ধ্রুব cscopep = নূতন AstCLocalScope{flp, nodep};
        handle.relink(cscopep);
        প্রত্যাগম cscopep;
    }
    // Create a temp variable and optionally put it before the specified node (mark local if so)
    AstVarScope* createTemp(File_সারি* ধ্রুব flp, ধ্রুব std::string& name,
                            Ast_গ্রন্থি_DType* ধ্রুব dtypep, Ast_গ্রন্থি* ধ্রুব ঢোকাও_Beforep = nullptr) {
        AstVar* varp;
        যদি (ঢোকাও_Beforep) {
            varp = নূতন AstVar{flp, VVarType::BLOCKTEMP, name, dtypep};
            varp->funcLocal(সত্য);
            ঢোকাও_Beforep->addHereThisAsNext(varp);
        } অন্যথা {
            varp = নূতন AstVar{flp, VVarType::MODULETEMP, name, dtypep};
            m_scopep->modp()->addStmtsp(varp);
        }
        AstVarScope* ধ্রুব vscp = নূতন AstVarScope{flp, m_scopep, varp};
        m_scopep->addVarsp(vscp);
        প্রত্যাগম vscp;
    }
    // Add a done() call on the fork sync
    শূন্য addForkDone(AstBegin* ধ্রুব beginp, AstVarScope* ধ্রুব forkVscp) ধ্রুব {
        File_সারি* ধ্রুব flp = beginp->fileline();
        auto* ধ্রুব donep = নূতন AstCMethodHard{
            beginp->fileline(), নূতন AstVarRef{flp, forkVscp, VAccess::WRITE}, VCMethod::FORK_DONE};
        donep->dtypeSetVoid();
        addDebugInfo(donep);
        beginp->addStmtsp(donep->makeStmt());
    }
    স্থির তার্কিক has_Disable_Queue_ঠেলো_Self_Prefix(AstBegin* ধ্রুব beginp) {
        // LinkJump prepends disable-by-name registration as:
        //   __VprocessQueue_*.push_back(std::process::self())
        // V3LiftExpr lifts std::process::self() into an assignment to a temporary, then V3Task
        // lowers that assignment's function call into a leading comment and AstStmtExpr. Thus the
        // push_back is no longer necessarily the first statement. Scan across this generated
        // prefix for it, stopping at the fork-start sentinel or the branch body. Registrations for
        // nested forks live in sub-blocks and so are not in this statement list.
        প্রতি (Ast_গ্রন্থি* stmtp = beginp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            যদি (VN_IS(stmtp, Comment)) অনুবর্ত;
            Ast_Stmt_রাশিমালা* ধ্রুব stmtExprp = VN_CAST(stmtp, Stmt_রাশিমালা);
            যদি (!stmtExprp) ভঙ্গ;
            যদি (stmtExprp->is_Disable_Queue_ঠেলো_Self_Stmt()) প্রত্যাগম সত্য;
        }
        প্রত্যাগম অসত্য;
    }
    // Register a callback so killing a process-backed fork branch decrements the join counter
    শূন্য addForkOnKill(AstBegin* ধ্রুব beginp, AstVarScope* ধ্রুব forkVscp) ধ্রুব {
        যদি (!beginp->needProcess() && !has_Disable_Queue_ঠেলো_Self_Prefix(beginp)) প্রত্যাগম;
        File_সারি* ধ্রুব flp = beginp->fileline();
        AstCMethodHard* ধ্রুব onKillp = নূতন AstCMethodHard{
            flp, নূতন AstVarRef{flp, forkVscp, VAccess::WRITE}, VCMethod::FORK_ON_KILL};
        onKillp->dtypeSetVoid();
        AstCExpr* ধ্রুব processp = নূতন AstCExpr{flp, "vlProcess"};
        processp
            ->dtypeSetVoid();  // Opaque process reference; type is irrelevant for hardcoded emit
        onKillp->addPinsp(processp);
        Ast_গ্রন্থি_Stmt* ধ্রুব stmtp = onKillp->makeStmt();
        beginp->prependStmtsp(stmtp);
    }
    // Handle the 'join' part of a fork..join
    শূন্য makeForkJoin(AstFork* ধ্রুব forkp) {
        // Create a fork sync var
        File_সারি* ধ্রুব flp = forkp->fileline();
        // Insert the sync var directly before the fork
        Ast_গ্রন্থি* ধ্রুব ঢোকাও_Beforep = forkp;
        // Make sure all references to the fork var are contained within a single scope to prevent
        // moving statements accessing it in case of large functions splitting.
        addCLocalScope(flp, ঢোকাও_Beforep);
        AstVarScope* forkVscp
            = createTemp(flp, forkp->name() + "__sync", getCreateForkSyncDTypep(), ঢোকাও_Beforep);
        unsigned join_গুনতি = 0;  // Needed for join counter
        // Add a <fork sync>.done() to each begin
        প্রতি (Ast_গ্রন্থি* beginp = forkp->forksp(); beginp; beginp = beginp->nextp()) {
            addForkOnKill(VN_AS(beginp, Begin), forkVscp);
            addForkDone(VN_AS(beginp, Begin), forkVscp);
            join_গুনতি++;
        }
        যদি (forkp->joinType().joinAny()) join_গুনতি = 1;
        // Set the join counter
        auto* ধ্রুব initp = নূতন AstCMethodHard{flp, নূতন AstVarRef{flp, forkVscp, VAccess::WRITE},
                                               VCMethod::FORK_INIT, নূতন AstConst{flp, join_গুনতি}};
        initp->dtypeSetVoid();
        addProcessInfo(initp);
        forkp->addHereThisAsNext(initp->makeStmt());
        // Await the join at the end
        auto* ধ্রুব joinp = নূতন AstCMethodHard{flp, নূতন AstVarRef{flp, forkVscp, VAccess::WRITE},
                                               VCMethod::FORK_JOIN};
        joinp->dtypeSetVoid();
        addProcessInfo(joinp);
        addDebugInfo(joinp);
        forkp->addNextHere(নূতন AstCAwait{flp, joinp});
    }

    // `procp` shall be a NodeProcedure/CFunc/Begin and within it vars from `varsp` will be placed.
    // `varsp` vector of vars which shall be localized.
    স্থির শূন্য localizeVars(Ast_গ্রন্থি* ধ্রুব procp, ধ্রুব std::vector<AstVar*>& varsp) {
        UASSERT(procp, "procp is nullptr");
        AstVar* newVarsp = nullptr;
        প্রতি (AstVar* ধ্রুব varp : varsp) {
            varp->funcLocal(সত্য);
            newVarsp = Ast_গ্রন্থি::addNextNull(newVarsp, varp->unlinkFrBack());
        }
        যদি (Ast_গ্রন্থি_Procedure* ধ্রুব গ্রন্থি_Procp = VN_CAST(procp, গ্রন্থি_Procedure)) {
            গ্রন্থি_Procp->prependStmtsp(newVarsp);
        } অন্যথা যদি (AstCFunc* ধ্রুব cfuncp = VN_CAST(procp, CFunc)) {
            cfuncp->prependVarsp(newVarsp);
        } অন্যথা যদি (AstBegin* ধ্রুব beginp = VN_CAST(procp, Begin)) {
            beginp->prependStmtsp(newVarsp);
        } অন্যথা {
            procp->v3fatalSrc(
                procp->prettyNameQ()
                << " is not of an expected type NodeProcedure/CFunc/Begin instead it is: "
                << procp->prettyTypeName());
        }
    }

    Ast_গ্রন্থি_রাশিমালা* delayToIntegral(AstDelay* nodep, Ast_গ্রন্থি_রাশিমালা* valuep) {
        ধ্রুব AstConst* ধ্রুব constp = VN_CAST(valuep, Const);
        // The fork start sentinel is already in ticks
        যদি (constp && constp->toUQuad() == std::numeric_limits<uint64_t>::max()) প্রত্যাগম valuep;
        প্রত্যাগম V3Timing::delayToTicks(valuep, nodep->timeunit());
    }

    // VISITORS
    শূন্য visit(Ast_গ্রন্থি_Module* nodep) override {
        UASSERT_OBJ(!m_classp, nodep, "Module or class under class");
        VL_RESTORER(m_classp);
        m_classp = VN_CAST(nodep, Class);
        VL_RESTORER(m_forkCnt);
        m_forkCnt = 0;
        iterateChildren(nodep);
    }
    শূন্য visit(AstScope* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        Sen_রাশিমালা_Builder sen_রাশিমালা_Builder{m_scopep};
        {  // Restore m_senExprBuilderp before destroying senExprBuilder
            VL_RESTORER(m_sen_রাশিমালা_Builderp);
            m_sen_রাশিমালা_Builderp = &sen_রাশিমালা_Builder;
            iterateChildren(nodep);
        }
    }
    শূন্য visit(AstActive* nodep) override {
        VL_RESTORER(m_activep);
        m_activep = nodep;
        iterateChildren(nodep);
    }
    শূন্য visit(Ast_গ্রন্থি_Procedure* nodep) override {
        VL_RESTORER(m_procp);
        VL_RESTORER(m_hasProcess);
        m_procp = nodep;
        m_hasProcess = hasFlags(nodep, T_HAS_PROC);
        VL_RESTORER(m_underProcedure);
        m_underProcedure = সত্য;
        iterateChildren(nodep);
        যদি (hasFlags(nodep, T_SUSPENDEE)) nodep->setSuspendable();
        যদি (hasFlags(nodep, T_HAS_PROC)) nodep->setNeedProcess();
    }
    শূন্য visit(AstInitial* nodep) override {
        visit(static_cast<Ast_গ্রন্থি_Procedure*>(nodep));
        যদি (nodep->needProcess() && !nodep->user1SetOnce()) {
            nodep->addStmtsp(
                নূতন AstCStmt{nodep->fileline(), "vlProcess->state(VlProcess::FINISHED);"});
        }
    }
    শূন্য visit(AstJumpBlock* nodep) override {
        VL_RESTORER(m_underJumpBlock);
        m_underJumpBlock = সত্য;
        visit(static_cast<Ast_গ্রন্থি_Stmt*>(nodep));
    }
    শূন্য visit(AstAlways* nodep) override {
        যদি (nodep->user1SetOnce()) প্রত্যাগম;
        VL_RESTORER(m_procp);
        VL_RESTORER(m_hasProcess);
        m_procp = nodep;
        m_hasProcess = hasFlags(nodep, T_HAS_PROC);
        VL_RESTORER(m_underProcedure);
        m_underProcedure = সত্য;
        // Workaround for killing `always` processes (doing that is pretty much UB)
        // TODO: Disallow killing `always` at runtime (throw an error)
        // Skip for combinational blocks; if the body has timing controls
        // iterateChildren will add T_SUSPENDEE, otherwise it would spin.
        যদি (hasFlags(nodep, T_HAS_PROC)
            && !(m_activep && m_activep->sentreep() && m_activep->sentreep()->hasCombo()))
            addFlags(nodep, T_SUSPENDEE);

        iterateChildren(nodep);
        যদি (hasFlags(nodep, T_HAS_PROC)) nodep->setNeedProcess();
        যদি (!hasFlags(nodep, T_SUSPENDEE)) প্রত্যাগম;
        nodep->setSuspendable();
        File_সারি* ধ্রুব flp = nodep->fileline();
        Ast_Sen_বৃক্ষ* ধ্রুব sentreep = m_activep->sentreep();
        যদি (sentreep->hasClocked()) {
            Ast_গ্রন্থি* ধ্রুব bodysp = nodep->stmtsp()->unlinkFrBackWithNext();
            auto* ধ্রুব controlp = নূতন AstEventControl{flp, sentreep->clone_বৃক্ষ(অসত্য), bodysp};
            nodep->addStmtsp(controlp);
            iterate(controlp);
        }
        // Note: The 'while (true)' outer loop will be added in V3Sched
        auto* ধ্রুব activep = নূতন AstActive{
            flp, "", নূতন Ast_Sen_বৃক্ষ{flp, নূতন AstSenItem{flp, AstSenItem::Initial{}}}};
        activep->sen_বৃক্ষ_Storep(activep->sentreep());
        activep->addStmtsp(nodep->unlinkFrBack());
        m_activep->addNextHere(activep);
    }
    শূন্য visit(AstCFunc* nodep) override {
        VL_RESTORER(m_procp);
        VL_RESTORER(m_hasProcess);
        VL_RESTORER(m_underIfaceCFunc);
        m_procp = nodep;
        m_hasProcess = hasFlags(nodep, T_HAS_PROC);
        m_underIfaceCFunc = VN_IS(m_scopep->modp(), Iface);
        iterateChildren(nodep);
        যদি (hasFlags(nodep, T_HAS_PROC)) nodep->setNeedProcess();
        যদি (!(hasFlags(nodep, T_SUSPENDEE))) প্রত্যাগম;

        nodep->rtnType("VlCoroutine");
        // If in a class, create a shared pointer to 'this'
        যদি (m_classp) {
            AstCStmt* ধ্রুব cstmtp = নূতন AstCStmt{nodep->fileline(), "VL_KEEP_THIS;"};
            nodep->prependStmtsp(cstmtp);
        }
        যদি (nodep->dpiExportImpl()) {
            nodep->exists([](AstCAwait* ধ্রুব awaitp) -> তার্কিক {
                // A DPI-exported coroutine won't be able to block the calling code
                // Error on the await node; fall back to the function node
                awaitp->v3warn(E_UNSUPPORTED,
                               "Unsupported: Timing controls inside DPI-exported tasks");
                প্রত্যাগম সত্য;
            });
        }
    }
    শূন্য visit(Ast_গ্রন্থি_CCall* nodep) override {
        AstCFunc* ধ্রুব funcp = nodep->funcp();

        // Skip automatic covergroup sampling calls
        যদি (funcp->isCovergroupSample()) {
            iterateChildren(nodep);
            প্রত্যাগম;
        }

        যদি (funcp->needProcess()) m_hasProcess = সত্য;
        যদি (hasFlags(funcp, T_SUSPENDEE) && !nodep->user1SetOnce()) {  // If suspendable
            // Calls to suspendables are always void return type, hence parent must be StmtExpr
            Ast_Stmt_রাশিমালা* ধ্রুব stmtp = VN_AS(nodep->backp(), Stmt_রাশিমালা);
            stmtp->replaceWith(নূতন AstCAwait{nodep->fileline(), nodep->unlinkFrBack()});
            VL_DO_DANGLING(ঠেলো_Deletep(stmtp), stmtp);
        }
        iterateChildren(nodep);
    }
    শূন্য visit(AstDelay* nodep) override {
        UASSERT_OBJ(!nodep->isCycleDelay(), nodep,
                    "Cycle delays should have been handled in V3AssertPre");
        File_সারি* ধ্রুব flp = nodep->fileline();

        Ast_গ্রন্থি_রাশিমালা* valuep = delayToIntegral(nodep, nodep->lhsp()->unlinkFrBack());

        // Statistics
        যদি (valuep->isZero()) {
            ++m_statZeroDelays;
        } অন্যথা যদি (VN_IS(valuep, Const)) {
            ++m_statConstDelays;
        } অন্যথা {
            ++m_statVariableDelays;
        }

        // Decide scheduling support for #0
        যদি (v3Global.opt.schedZeroDelay().isSetTrue()) {
            // User said to schedule for #0 support, nothing else to do
            v3Global.setUsesZeroDelay();
        } অন্যথা যদি (v3Global.opt.schedZeroDelay().isSetFalse()) {
            // User said to schedule without #0 support. Still warn if a static #0 delay exists
            যদি (valuep->isZero()) {
                nodep->v3warn(
                    ZERODLY,
                    "Static #0 delay exists, but '--no-sched-zero-delay' was given.\n"
                        << nodep->warnMore()  //
                        << "... Can proceed, but this will fail at runtime if executed.");
            }
        } অন্যথা {
            // User did not express preference, decide based on presence of delays
            যদি (valuep->isZero()) {
                // Statically known #0 delay exists, schedule for #0 support
                v3Global.setUsesZeroDelay();
                m_hasStaticZeroDelay = সত্য;
                // Don't warn on variable delays, as no point
                m_unknownDelayFlps.clear();
            } অন্যথা যদি (staticallyNonZeroDelay(valuep)) {
                // Delay is dynamic, but every statically known outcome is non-zero.
                // So we don't need #0 delay support and there should be no warning.
            } অন্যথা যদি (!VN_IS(valuep, Const)) {
                // Delay is not known at compiile time. Conservatively schedule for #0 support,
                // but warn if no static #0 delays used as performance might be improved
                v3Global.setUsesZeroDelay();
                যদি (!m_hasStaticZeroDelay) m_unknownDelayFlps.ঠেলো_back(nodep->fileline());
            }
        }

        // Replace self with a 'co_await dlySched.delay(<valuep>)'
        AstCMethodHard* ধ্রুব delayMethodp = নূতন AstCMethodHard{
            flp, নূতন AstVarRef{flp, getCreateDelayScheduler(), VAccess::WRITE},
            VCMethod::SCHED_DELAY, valuep};
        delayMethodp->dtypeSetVoid();
        addProcessInfo(delayMethodp);
        addDebugInfo(delayMethodp);
        // Create the co_await
        Ast_গ্রন্থি* ধ্রুব awaitp = নূতন AstCAwait{flp, delayMethodp, get_Create_Delay_Sen_বৃক্ষ()};
        // Relink child statements after the co_await
        যদি (Ast_গ্রন্থি* ধ্রুব stmtsp = nodep->stmtsp()) {
            awaitp->addNext(stmtsp->unlinkFrBackWithNext());
        }
        nodep->replaceWith(awaitp);
        VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
    }
    শূন্য visit(AstEventControl* nodep) override {
        // Do not allow waiting on local named events, as they get enqueued for clearing, but can
        // go out of scope before that happens
        যদি (!nodep->sentreep()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Event control with implicit sensitivity (@*)"
                          " in this context; use an explicit sensitivity list instead");
            VL_DO_DANGLING(ঠেলো_Deletep(nodep->unlinkFrBack()), nodep);
            প্রত্যাগম;
        }
        File_সারি* ধ্রুব flp = nodep->fileline();
        // Relink child statements after the event control
        যদি (nodep->stmtsp()) nodep->addNextHere(nodep->stmtsp()->unlinkFrBackWithNext());
        যদি (needDynamicTrigger(nodep->sentreep())) {
            // Create the trigger variable and init it with 0
            AstVarScope* ধ্রুব trigvscp
                = createTemp(flp, m_dynTrigNames.get(nodep), nodep->findBitDType(), nodep);
            auto* ধ্রুব initp = নূতন AstAssign{flp, নূতন AstVarRef{flp, trigvscp, VAccess::WRITE},
                                              নূতন AstConst{flp, AstConst::BitFalse{}}};
            // Await the eval step with the dynamic trigger scheduler. First, create the method
            // call
            auto* ধ্রুব evalMethodp = নূতন AstCMethodHard{
                flp, নূতন AstVarRef{flp, getCreateDynamicTriggerScheduler(), VAccess::WRITE},
                VCMethod::SCHED_EVALUATION};
            evalMethodp->dtypeSetVoid();
            addProcessInfo(evalMethodp);
            auto* ধ্রুব sentreep = nodep->sentreep();
            addEventDebugInfo(evalMethodp, sentreep);
            // Create the co_await
            AstCAwait* ধ্রুব awaitEvalp
                = নূতন AstCAwait{flp, evalMethodp, get_Create_Dynamic_Trigger_Sen_বৃক্ষ()};
            // Construct the sen expression for this sentree
            UASSERT_OBJ(m_sen_রাশিমালা_Builderp, nodep, "No SenExprBuilder for this scope");
            auto* ধ্রুব assignp = নূতন AstAssign{flp, নূতন AstVarRef{flp, trigvscp, VAccess::WRITE},
                                                m_sen_রাশিমালা_Builderp->build(sentreep).first};
            // Get the SenExprBuilder results
            ধ্রুব Sen_রাশিমালা_Builder::Results senResults = m_sen_রাশিমালা_Builderp->getAndClearResults();
            localizeVars(m_procp, senResults.m_vars);
            // If post updates are destructive (e.g. clearFired on events), perform a
            // conservative pre-clear once before entering the wait loop so stale state from a
            // previous wait does not cause an immediate false-positive trigger.
            ধ্রুব তার্কিক hasDestructivePostUpdates = !senResults.m_destructivePostUpdates.empty();
            // Create the trigger eval loop, which will await the evaluation step and check the
            // trigger
            Ast_গ্রন্থি_রাশিমালা* ধ্রুব condp
                = নূতন AstLogNot{flp, নূতন AstVarRef{flp, trigvscp, VAccess::READ}};
            AstLoop* ধ্রুব loopp = নূতন AstLoop{flp};
            loopp->addStmtsp(নূতন AstLoopTest{flp, loopp, condp});
            loopp->addStmtsp(awaitEvalp);
            // Put pre updates before the trigger check and assignment
            প্রতি (Ast_গ্রন্থি_Stmt* ধ্রুব stmtp : senResults.m_preUpdates) loopp->addStmtsp(stmtp);
            // Then the trigger check and assignment
            loopp->addStmtsp(assignp);
            // Let the dynamic trigger scheduler know if this trigger was set
            // If it was, a call to the scheduler's evaluate() will return true
            AstCMethodHard* ধ্রুব anyTriggeredMethodp = নূতন AstCMethodHard{
                flp, নূতন AstVarRef{flp, getCreateDynamicTriggerScheduler(), VAccess::WRITE},
                VCMethod::SCHED_ANY_TRIGGERED, নূতন AstVarRef{flp, trigvscp, VAccess::READ}};
            anyTriggeredMethodp->dtypeSetVoid();
            loopp->addStmtsp(anyTriggeredMethodp->makeStmt());
            // If the post update is destructive (e.g. event vars are cleared), create an await for
            // the post update step
            যদি (hasDestructivePostUpdates) {
                AstCAwait* ধ্রুব awaitPostUpdatep = awaitEvalp->clone_বৃক্ষ(অসত্য);
                VN_AS(awaitPostUpdatep->exprp(), CMethodHard)->method(VCMethod::SCHED_POST_UPDATE);
                loopp->addStmtsp(awaitPostUpdatep);
            }
            // Put the post updates at the end of the loop
            প্রতি (Ast_গ্রন্থি_Stmt* ধ্রুব stmtp : senResults.m_postUpdates) loopp->addStmtsp(stmtp);
            // Finally, await the resumption step in 'act'
            AstCAwait* ধ্রুব awaitResumep = awaitEvalp->clone_বৃক্ষ(অসত্য);
            VN_AS(awaitResumep->exprp(), CMethodHard)->method(VCMethod::SCHED_RESUMPTION);
            Ast_গ্রন্থি::addNext<Ast_গ্রন্থি_Stmt, Ast_গ্রন্থি_Stmt>(loopp, awaitResumep);
            // Replace the event control with one explicit stmt chain:
            //   init -> [inits] -> [optional destructive pre-clears] -> loop -> awaitResumption
            Ast_গ্রন্থি_Stmt* chainp = nullptr;
            chainp = Ast_গ্রন্থি::addNextNull(chainp, initp);
            প্রতি (Ast_গ্রন্থি_Stmt* ধ্রুব stmtp : senResults.m_inits) {
                chainp = Ast_গ্রন্থি::addNextNull(chainp, stmtp);
            }
            যদি (hasDestructivePostUpdates) {
                প্রতি (Ast_গ্রন্থি_Stmt* ধ্রুব stmtp : senResults.m_destructivePostUpdates) {
                    chainp = Ast_গ্রন্থি::addNextNull(chainp, stmtp->clone_বৃক্ষ(অসত্য));
                }
            }
            chainp = Ast_গ্রন্থি::addNextNull(chainp, loopp);
            nodep->replaceWith(chainp);
        } অন্যথা {
            auto* ধ্রুব sentreep = m_finder.get_Sen_বৃক্ষ(nodep->sentreep());
            nodep->sentreep()->unlinkFrBack()->delete_বৃক্ষ();
            // Get this sentree's trigger scheduler
            // Replace self with a 'co_await trigSched.trigger()'
            auto* ধ্রুব triggerMethodp = নূতন AstCMethodHard{
                flp, নূতন AstVarRef{flp, getCreateTriggerSchedulerp(sentreep), VAccess::WRITE},
                VCMethod::SCHED_TRIGGER};
            triggerMethodp->dtypeSetVoid();
            // If it should be committed immediately, pass true, otherwise false
            triggerMethodp->addPinsp(nodep->user2() ? নূতন AstConst{flp, AstConst::BitTrue{}}
                                                    : নূতন AstConst{flp, AstConst::BitFalse{}});
            addProcessInfo(triggerMethodp);
            addEventDebugInfo(triggerMethodp, sentreep);
            // Create the co_await
            AstCAwait* ধ্রুব awaitp = নূতন AstCAwait{flp, triggerMethodp, sentreep};
            nodep->replaceWith(awaitp);
        }
        VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
    }
    শূন্য visit(Ast_গ্রন্থি_Assign* nodep) override {
        // Only process once to avoid infinite loops (due to the net delay)
        যদি (nodep->user1SetOnce()) প্রত্যাগম;
        UINFO(9, "control-visit " << nodep);
        File_সারি* ধ্রুব flp = nodep->fileline();
        Ast_গ্রন্থি* controlp = factorOutTimingControl(nodep);
        ধ্রুব তার্কিক inAssignDly = VN_IS(nodep, AssignDly);
        // Handle the intra assignment timing control
        // Transform if:
        // * there's a timing control in the assignment
        // * the assignment is an AssignDly and it's in a non-inlined function
        যদি (!controlp && (!inAssignDly || m_underProcedure)) {
            iterateChildren(nodep);
            প্রত্যাগম;
        }
        // The timing control, if any, which is put in a fork for an NBA
        Ast_গ্রন্থি* ধ্রুব timingp = controlp;
        // Insert new vars before the timing control if we're in a function; in a process we can't
        // do that, as after V3Begin only modules and functions declare variables. These
        // intra-assignment vars will later be passed to forked processes by value.
        Ast_গ্রন্থি* ঢোকাও_Beforep = m_underProcedure ? nullptr : controlp;
        // Special case for NBA
        যদি (inAssignDly) {
            // Put it in a fork so it doesn't block
            // Could already be the only thing directly under a fork, reuse that if possible
            AstFork* forkp = !nodep->nextp() ? VN_CAST(nodep->firstAbovep(), Fork) : nullptr;
            যদি (!forkp) forkp = নূতন AstFork{flp, VJoinType::JOIN_NONE};
            যদি (!m_underProcedure) {
                // If it's in a function, it won't be handled by V3Delayed
                // Put it behind an additional named event that gets triggered in the NBA region
                AstEventControl* ধ্রুব nbaEventControlp = createNbaEventControl(flp);
                AstAssign* ধ্রুব trigAssignp = createNbaEventTriggerAssignment(flp);
                nodep->replaceWith(trigAssignp);
                trigAssignp->addNextHere(nbaEventControlp);
                nbaEventControlp->addStmtsp(nodep);
                ঢোকাও_Beforep = forkp;
                যদি (!controlp) controlp = nbaEventControlp;
            }
            controlp->replaceWith(forkp);
            AstBegin* beginp = VN_CAST(controlp, Begin);
            যদি (!beginp) beginp = নূতন AstBegin{nodep->fileline(), "", controlp, অসত্য};
            forkp->addForksp(beginp);
            addFlags(beginp, T_NBA_UPDATE);
            controlp = forkp;
        }
        UASSERT_OBJ(nodep, controlp, "Assignment should have timing control");
        // Move `insertBeforep` into `AstCLocalScope` if necessary to avoid jumping over
        // a variable initialization that would be inserted before `insertBeforep`. All
        // access to this variable should be contained within returned `AstCLocalScope`.
        যদি (m_underJumpBlock) addCLocalScope(flp, ঢোকাও_Beforep);
        // Function for replacing values with intermediate variables
        ধ্রুব auto replaceWithIntermediate = [&](Ast_গ্রন্থি_রাশিমালা* ধ্রুব valuep,
                                                 ধ্রুব std::string& name) {
            AstVarScope* ধ্রুব newvscp = createTemp(flp, name, valuep->dtypep(), ঢোকাও_Beforep);
            // A pending update keeps its own value, even if its process schedules more
            যদি (inAssignDly && !ঢোকাও_Beforep) newvscp->fork_By_মান(সত্য);
            valuep->replaceWith(নূতন AstVarRef{flp, newvscp, VAccess::READ});
            controlp->addHereThisAsNext(
                নূতন AstAssign{flp, নূতন AstVarRef{flp, newvscp, VAccess::WRITE}, valuep});
        };
        // NBAs with delays evaluate the indices selecting the target now (IEEE 1800-2023 10.4.2),
        // in order, each as a whole, also the selects and function calls in it. So follow the
        // selects of the target up from the variable, not into their indices. As an example, this
        // transforms 'x[a[i]][f(j)] <= #1 y' into 't0 = a[i]; t1 = f(j); x[t0][t1] <= #1 y'.
        // V3Const has split NBAs to concatenations, also streamed, into NBAs to each part.
        যদি (inAssignDly) {
            প্রতি (Ast_গ্রন্থি* exprp = nodep->lhsp()->baseFromp(সত্য); exprp != nodep;
                 exprp = exprp->firstAbovep()) {
                যদি (AstSel* ধ্রুব selp = VN_CAST(exprp, Sel)) {
                    // widthp should be const
                    যদি (!VN_IS(selp->lsbp(), Const)) {
                        replaceWithIntermediate(selp->lsbp(), m_intraLsbNames.get(nodep));
                    }
                } অন্যথা যদি (Ast_গ্রন্থি_Sel* ধ্রুব selp = VN_CAST(exprp, গ্রন্থি_Sel)) {
                    যদি (!VN_IS(selp->bitp(), Const)) {
                        replaceWithIntermediate(selp->bitp(), m_intraIndexNames.get(nodep));
                    }
                }
            }
        }
        // Replace the RHS with an intermediate value var
        replaceWithIntermediate(nodep->rhsp(), m_intra_মান_Names.get(nodep));
        যদি (!inAssignDly) প্রত্যাগম;
        // The process executing the NBA evaluates its delay (IEEE 1800-2023 4.9.4), not the forked
        // process, if it has side effects, e.g. calling a function that checks its process. A pure
        // one is evaluated as the forked process starts, at once.
        যদি (AstDelay* ধ্রুব delayp = VN_CAST(timingp, Delay)) {
            যদি (!delayp->lhsp()->isPure()) {
                replaceWithIntermediate(delayp->lhsp(), m_intra_মান_Names.get(nodep));
            }
        }
    }
    শূন্য visit(AstFireEvent* nodep) override {
        // V3Delayed handles '->>' of a variable in a process. Like NBAs in non-inlined functions,
        // trigger other events from a fork awaiting the NBA region.
        যদি (!nodep->isDelayed() || (m_underProcedure && VN_IS(nodep->operandp(), VarRef))) {
            iterateChildren(nodep);
            প্রত্যাগম;
        }
        File_সারি* ধ্রুব flp = nodep->fileline();
        AstAssign* ধ্রুব trigAssignp = createNbaEventTriggerAssignment(flp);
        nodep->replaceWith(trigAssignp);
        AstFork* ধ্রুব forkp = নূতন AstFork{flp, VJoinType::JOIN_NONE};
        trigAssignp->addNextHere(forkp);
        যদি (m_underJumpBlock) addCLocalScope(flp, forkp);
        // The triggered event is the one referenced now, so evaluate handles and indices now
        Ast_গ্রন্থি_রাশিমালা* ধ্রুব eventp = nodep->operandp()->unlinkFrBack();
        ধ্রুব auto evalNow = [&](Ast_গ্রন্থি_রাশিমালা* ধ্রুব valuep, V3UniqueNames& names) {
            // Like for an NBA, a process uses a module variable, passed to the fork by value
            AstVarScope* ধ্রুব vscp = createTemp(flp, names.get(nodep), valuep->dtypep(),
                                                 m_underProcedure ? nullptr : forkp);
            যদি (m_underProcedure) vscp->fork_By_মান(সত্য);
            valuep->replaceWith(নূতন AstVarRef{flp, vscp, VAccess::READ});
            // Unlike an event, a handle selecting one is only read
            valuep->foreach([](Ast_গ্রন্থি* ধ্রুব np) {
                যদি (Ast_গ্রন্থি_Var_Ref* ধ্রুব refp = VN_CAST(np, গ্রন্থি_Var_Ref)) {
                    refp->access(VAccess::READ);
                } অন্যথা যদি (AstMemberSel* ধ্রুব selp = VN_CAST(np, MemberSel)) {
                    selp->access(VAccess::READ);
                }
            });
            forkp->addHereThisAsNext(
                নূতন AstAssign{flp, নূতন AstVarRef{flp, vscp, VAccess::WRITE}, valuep});
        };
        Ast_গ্রন্থি_রাশিমালা* refp = eventp;
        যাবত্ (Ast_গ্রন্থি_Sel* ধ্রুব selp = VN_CAST(refp, গ্রন্থি_Sel)) {
            যদি (!VN_IS(selp->bitp(), Const)) evalNow(selp->bitp(), m_intraIndexNames);
            refp = selp->fromp();
        }
        // Handle of a class (or virtual interface)
        যদি (AstMemberSel* ধ্রুব selp = VN_CAST(refp, MemberSel)) {
            evalNow(selp->fromp(), m_intra_মান_Names);
        }
        AstEventControl* ধ্রুব controlp = createNbaEventControl(flp);
        controlp->addStmtsp(নূতন AstFireEvent{flp, eventp, অসত্য});
        AstBegin* ধ্রুব beginp = নূতন AstBegin{flp, "", controlp, অসত্য};
        addFlags(beginp, T_NBA_UPDATE);
        forkp->addForksp(beginp);
        VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
    }
    শূন্য visit(AstAssignW* nodep) override {
        File_সারি* ধ্রুব flp = nodep->fileline();
        // Get the net delay unless this assignment was created for handling the net delay (user1)
        AstDelay* ধ্রুব netDelayp = nodep->user1() ? nullptr : nodep->getLhsNetDelay();
        যদি (netDelayp) {
            যদি (nodep->timingControlp()) {
                // If this assignment has a delay, create another one to handle the net delay
                AstVarScope* ধ্রুব newvscp = m_assignWTmps.make(flp, m_scopep, nodep->dtypep());
                AstAssignW* assignp = নূতন AstAssignW{
                    nodep->fileline(), nodep->lhsp()->unlinkFrBack(),
                    নূতন AstVarRef{flp, newvscp, VAccess::READ}, netDelayp->clone_বৃক্ষ(অসত্য)};
                assignp->user1(সত্য);
                nodep->addNextHere(assignp);
                nodep->lhsp(নূতন AstVarRef{flp, newvscp, VAccess::WRITE});
            } অন্যথা {
                // Else just use this one with the net delay
                nodep->timingControlp(netDelayp->clone_বৃক্ষ(অসত্য));
            }
        }
        যদি (!nodep->timingControlp()) প্রত্যাগম;
        // There will be some circular logic here, suppress the warning for newly created vars
        flp->modifyWarnOff(V3ErrorCode::UNOPTFLAT, সত্য);
        // Also suppress the warning for the LHS var, for cases like `assign #1 clk = ~clk;`
        // TODO: Restore the warning for other, non-delayed drivers
        nodep->lhsp()->foreach([](AstVarRef* refp) {
            যদি (refp->access().isWriteOrRW()) {
                refp->varp()->fileline()->modifyWarnOff(V3ErrorCode::UNOPTFLAT, সত্য);
            }
        });
        // Should be under an always
        AstAlways* ধ্রুব alwaysp = VN_AS(m_procp, Always);
        // Convert it to an Assign; the new assign with intra delay will be handled by
        // visit(AstNodeAssign*)
        Ast_গ্রন্থি_রাশিমালা* ধ্রুব lhs1p = nodep->lhsp()->unlinkFrBack();
        Ast_গ্রন্থি_রাশিমালা* ধ্রুব rhs1p = nodep->rhsp()->unlinkFrBack();
        Ast_গ্রন্থি* ধ্রুব controlp = nodep->timingControlp()->unlinkFrBack();
        যদি (AstDelay* ধ্রুব delayp = VN_CAST(controlp, Delay)) {
            যদি (Ast_গ্রন্থি_রাশিমালা* fallDelayp = delayp->fallDelay()) {
                fallDelayp = fallDelayp->unlinkFrBack();
                Ast_গ্রন্থি_রাশিমালা* lhsp = delayp->lhsp()->unlinkFrBack();
                // Use fall only for an all-zero value, rise otherwise.
                lhsp = delayToIntegral(delayp, lhsp);
                fallDelayp = delayToIntegral(delayp, fallDelayp);
                delayp->lhsp(
                    নূতন AstCond{flp, নূতন AstEq{flp, rhs1p->clone_বৃক্ষ(অসত্য), নূতন AstConst{flp, 0}},
                                fallDelayp, lhsp});
            }
        }
        AstAssign* ধ্রুব assignp = নূতন AstAssign{nodep->fileline(), lhs1p, rhs1p, controlp};
        // Put the assignment in a fork..join_none.
        AstFork* ধ্রুব forkp = নূতন AstFork{flp, VJoinType::JOIN_NONE};
        nodep->replaceWith(forkp);
        VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
        AstBegin* ধ্রুব beginp = নূতন AstBegin{flp, "", assignp, অসত্য};
        forkp->addForksp(beginp);
        visit(forkp);  // Visit now as we need to do some post-processing
        // IEEE 1800-2023 10.3.3 - if the RHS value differs from the currently scheduled value to
        // be assigned, the currently scheduled assignment is descheduled. To keep track if an
        // assignment should be descheduled, each scheduled assignment event has a 'generation',
        // and if at assignment time its generation differs from the current generation, it won't
        // be performed
        AstAssign* ধ্রুব preAssignp = VN_AS(beginp->stmtsp(), Assign);
        UASSERT_OBJ(preAssignp, alwaysp, "Pre-assign should be there from visit(AstFork)");
        AstAssign* ধ্রুব postAssignp = VN_AS(preAssignp->nextp()->nextp(), Assign);
        UASSERT_OBJ(postAssignp, alwaysp, "Post-assign should be there from visit(AstFork)");
        // Increment generation and copy it to a local
        AstVarScope* ধ্রুব generationVarp
            = m_assignWGens.make(flp, m_scopep, alwaysp->findUInt64DType());
        AstVarScope* ধ্রুব genLocalVarp
            = createTemp(flp, generationVarp->varp()->name() + "__local",
                         alwaysp->findUInt64DType(), preAssignp);
        preAssignp->addHereThisAsNext(
            নূতন AstAssign{flp, নূতন AstVarRef{flp, generationVarp, VAccess::WRITE},
                          নূতন AstAdd{flp, নূতন AstVarRef{flp, generationVarp, VAccess::READ},
                                     নূতন AstConst{flp, 1}}});
        preAssignp->addHereThisAsNext(
            নূতন AstAssign{flp, নূতন AstVarRef{flp, genLocalVarp, VAccess::WRITE},
                          নূতন AstVarRef{flp, generationVarp, VAccess::READ}});
        // If the current generation is same as the one saved in the local var, assign
        beginp->addStmtsp(
            নূতন AstIf{flp,
                      নূতন AstEq{flp, নূতন AstVarRef{flp, generationVarp, VAccess::READ},
                                নূতন AstVarRef{flp, genLocalVarp, VAccess::READ}},
                      postAssignp->unlinkFrBack()});
        // Save scheduled RHS value before delay
        AstVarScope* ধ্রুব tmpVarp
            = m_assignWTmps.make(flp, m_scopep, preAssignp->rhsp()->dtypep());
        AstVarRef* ধ্রুব tmpAssignRhsp = VN_AS(preAssignp->lhsp(), VarRef)->clone_বৃক্ষ(অসত্য);
        tmpAssignRhsp->access(VAccess::WRITE);
        preAssignp->addNextHere(
            নূতন AstAssign{flp, নূতন AstVarRef{flp, tmpVarp, VAccess::WRITE}, tmpAssignRhsp});
        // If the RHS is different from the currently scheduled value, schedule the new assignment
        // The generation will increase, effectively 'descheduling' the previous assignment.
        Ast_গ্রন্থি_রাশিমালা* ধ্রুব didNotInitp
            = নূতন AstLogNot{flp, নূতন AstCExpr{flp, "vlSymsp->__Vm_didInit", 1}};
        AstVarRef* ধ্রুব firstIterp
            = নূতন AstVarRef{flp, m_netlistp->stlFirstIterationp(), VAccess::READ};
        Ast_গ্রন্থি_রাশিমালা* ধ্রুব schedCondp
            = নূতন AstLogOr{flp,
                           নূতন AstNeq{flp, preAssignp->rhsp()->clone_বৃক্ষ(অসত্য),
                                      নূতন AstVarRef{flp, tmpVarp, VAccess::READ}},
                           নূতন AstLogAnd{flp, didNotInitp, firstIterp}};
        alwaysp->addStmtsp(নূতন AstIf{flp, schedCondp, forkp->unlinkFrBack()});
    }
    শূন্য visit(AstDisableFork* nodep) override {
        যদি (m_hasProcess) প্রত্যাগম;
        // never reached by any process; remove to avoid compilation error
        VL_DO_DANGLING(nodep->unlinkFrBack()->delete_বৃক্ষ(), nodep);
    }
    শূন্য visit(AstWaitFork* nodep) override {
        যদি (m_hasProcess) {
            File_সারি* ধ্রুব flp = nodep->fileline();
            AstCExpr* ধ্রুব exprp = নূতন AstCExpr{flp, "vlProcess->completedFork()", 1};
            AstWait* ধ্রুব waitp = নূতন AstWait{flp, exprp, nullptr};
            nodep->replaceWith(waitp);
        } অন্যথা {
            // never reached by any process; remove to avoid compilation error
            nodep->unlinkFrBack();
        }
        VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
    }
    শূন্য visit(AstWait* nodep) override {
        // Wait on changed events related to the vars in the wait statement
        UINFO(9, "control-visit " << nodep);
        File_সারি* ধ্রুব flp = nodep->fileline();
        Ast_গ্রন্থি* ধ্রুব stmtsp = nodep->stmtsp();
        যদি (stmtsp) stmtsp->unlinkFrBackWithNext();
        Ast_গ্রন্থি_রাশিমালা* ধ্রুব condp = V3Const::constifyEdit(nodep->condp()->unlinkFrBack());
        auto* ধ্রুব constp = VN_CAST(condp, Const);
        যদি (constp) {
            যদি (constp->isZero()) {
                // We have to await forever instead of simply returning in case we're deep in a
                // callstack
                AstCMethodHard* ধ্রুব foreverMethodp = নূতন AstCMethodHard{
                    flp, নূতন AstVarRef{flp, getCreateDelayScheduler(), VAccess::WRITE},
                    VCMethod::SCHED_WAIT_FOREVER};
                foreverMethodp->dtypeSetVoid();
                addProcessInfo(foreverMethodp);
                addDebugInfo(foreverMethodp);
                AstCAwait* ধ্রুব awaitp = নূতন AstCAwait{flp, foreverMethodp};
                nodep->replaceWith(awaitp);
                যদি (stmtsp) VL_DO_DANGLING(stmtsp->delete_বৃক্ষ(), stmtsp);
                VL_DO_DANGLING(condp->delete_বৃক্ষ(), condp);
            } অন্যথা {
                nodep->v3fatalSrc("constant wait should have been removed in "
                                  "TimingSuspendableVisitor::visit(AstWait)");
            }
        } অন্যথা যদি (needDynamicTrigger(condp)) {
            // No point in making a sentree, just use the expression as sensitivity
            // Put the event control in an if so we only wait if the condition isn't met already
            auto* ধ্রুব ifp = নূতন AstIf{
                flp, নূতন AstLogNot{flp, condp},
                নূতন AstEventControl{flp,
                                    নূতন Ast_Sen_বৃক্ষ{flp, নূতন AstSenItem{flp, VEdgeType::ET_TRUE,
                                                                       condp->clone_বৃক্ষ(অসত্য)}},
                                    nullptr}};
            যদি (stmtsp) Ast_গ্রন্থি::addNext<Ast_গ্রন্থি, Ast_গ্রন্থি>(ifp, stmtsp);
            nodep->replaceWith(ifp);
        } অন্যথা {
            // We are using a global sentree, so we cannot use ET_TRUE, as that could lead to the
            // active region never converging. Because of this, we need to use ET_CHANGED in a
            // loop.
            AstEventControl* ধ্রুব controlp = নূতন AstEventControl{
                flp,
                নূতন Ast_Sen_বৃক্ষ{
                    flp, নূতন AstSenItem{flp, VEdgeType::ET_CHANGED, condp->clone_বৃক্ষ(অসত্য)}},
                nullptr};
            controlp->user2(সত্য);  // Commit immediately
            AstLoop* ধ্রুব loopp = নূতন AstLoop{flp};
            loopp->addStmtsp(নূতন AstLoopTest{flp, loopp, নূতন AstLogNot{flp, condp}});
            loopp->addStmtsp(controlp);
            যদি (stmtsp) Ast_গ্রন্থি::addNext<Ast_গ্রন্থি, Ast_গ্রন্থি>(loopp, stmtsp);
            nodep->replaceWith(loopp);
        }
        VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
    }
    শূন্য visit(AstBegin* nodep) override {
        VL_RESTORER(m_procp);
        VL_RESTORER(m_hasProcess);
        m_hasProcess |= hasFlags(nodep, T_HAS_PROC);
        // A pending NBA update is not a subprocess of the process that scheduled it, so it is
        // unaffected by 'disable fork' and 'wait fork' (IEEE 1800-2023 9.6.1, 9.6.3)
        যদি (hasFlags(nodep, T_NBA_UPDATE)) m_hasProcess = অসত্য;
        m_procp = nodep;
        যদি (m_hasProcess) nodep->setNeedProcess();
        iterateChildren(nodep);
    }
    শূন্য visit(AstFork* nodep) override {
        যদি (nodep->user1SetOnce()) প্রত্যাগম;
        UINFO(9, "control-visit " << nodep);
        v3Global.setUsesTiming();

        // Create a unique name for this fork
        nodep->name("__Vfork_" + std::to_string(++m_forkCnt));

        // TODO: Should process nodep->stmtsp() in case an earlier pass
        //       inserted something there, but as of today we don't need to.

        // Process and name each fork
        size_t idx = 0;  // Index for naming begins
        প্রতি (AstBegin *itemp = nodep->forksp(), *nextp; itemp; itemp = nextp) {
            nextp = VN_AS(itemp->nextp(), Begin);
            iterate(itemp);
            // Note: Even if we do not find any awaits, we cannot simply inline
            // the process here, as new awaits could be added later.

            // Name the begin (later the name will be used for a new function)
            itemp->name(nodep->name() + "__" + std::to_string(idx++));
            যদি (itemp->needProcess()) {
                itemp->addStmtsp(
                    নূতন AstCStmt{itemp->fileline(), "vlProcess->state(VlProcess::FINISHED);"});
            }
        }
        যদি (!nodep->joinType().joinNone()) makeForkJoin(nodep);
    }

    //--------------------
    শূন্য visit(AstVar*) override {}  // Accelerate
    শূন্য visit(Ast_গ্রন্থি* nodep) override { iterateChildren(nodep); }

সার্বজনিক:
    // CONSTRUCTORS
    explicit TimingControlVisitor(AstNetlist* nodep)
        : m_netlistp{nodep} {
        iterate(nodep);

        // If there is no static #0 in the design, but an unknown delay was found,
        // and the user did not specify preference, then warn on all unknown delays
        // as we will be assuming they can be #0, which can cause performance degradation.
        প্রতি (File_সারি* ধ্রুব flp : m_unknownDelayFlps) {
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

Ast_গ্রন্থি_রাশিমালা* V3Timing::delayToTicks(Ast_গ্রন্থি_রাশিমালা* valuep, VTimescale timeunit) {
    File_সারি* ধ্রুব flp = valuep->fileline();
    যদি (ধ্রুব AstConst* ধ্রুব constp = VN_CAST(valuep, Const)) {
        যদি (constp->অঙ্ক().is1Step()) {
            // A step is one tick of the global time precision (IEEE 1800-2023 3.14.3)
            VL_DO_DANGLING(valuep->delete_বৃক্ষ(), valuep);
            AstConst* ধ্রুব tickp = নূতন AstConst{flp, AstConst::Unsized64{}, 1};
            tickp->dtypeSetBitSized(64, VSigning::UNSIGNED);
            প্রত্যাগম tickp;
        }
        যদি (constp->isZero()) প্রত্যাগম valuep;
    }
    UASSERT_OBJ(!timeunit.isNone(), valuep, "timeunit must be set");
    ধ্রুব দ্বিগুণ timescaleFactorD
        = std::pow(10.0, timeunit.powerOfTen() - v3Global.rootp()->timeprecision().powerOfTen());
    যদি (valuep->dtypep()->skipRefp()->isDouble()) {
        AstConst* ধ্রুব tsfp = নূতন AstConst{flp, AstConst::RealDouble{}, timescaleFactorD};
        valuep = নূতন AstMulD{flp, valuep, tsfp};
        valuep = নূতন AstRToIRoundS{flp, valuep};
        valuep->dtypeSetBitSized(64, VSigning::UNSIGNED);
    } অন্যথা {
        valuep->dtypeSetBitSized(64, VSigning::UNSIGNED);
        ধ্রুব uint64_t timescaleFactorU = static_cast<uint64_t>(timescaleFactorD);
        AstConst* ধ্রুব tsfp = নূতন AstConst{flp, AstConst::Unsized64{}, timescaleFactorU};
        valuep = নূতন AstMul{flp, valuep, tsfp};
    }
    প্রত্যাগম V3Const::constifyEdit(valuep);  // Simplify
}

শূন্য V3Timing::timingAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        ধ্রুব VNUser1InUse m_user1InUse;
        ধ্রুব VNUser2InUse m_user2InUse;
        { TimingSuspendableVisitor{nodep}; }
        যদি (v3Global.usesTiming()) TimingControlVisitor{nodep};
    }
    V3Global::dump_Check_Global_বৃক্ষ("timing", 0, dump_বৃক্ষ_Either_Level() >= 3);
}
