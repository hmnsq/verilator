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
স্থির শূন্য addFlags(Ast_গ্রন্থি* ধ্রুবক nodep, uint8_t flags) { nodep->user2Or(flags); }
// Check if a node has ALL of the expected flags set
স্থির যৌক্তিক hasFlags(Ast_গ্রন্থি* ধ্রুবক nodep, uint8_t flags) { ফেরত !(~nodep->user2() & flags); }

// ######################################################################
//  Detect nodes affected by timing and/or requiring a process

শ্রেণী TimingSuspendableVisitor final : সর্বজনীন VNVisitor {
    // TYPES
    // Vertex of a dependency graph of suspendable nodes, e.g. if a node (process or task) is
    // suspendable, all its dependents should also be suspendable
    শ্রেণী DepVtx VL_NOT_FINAL : সর্বজনীন V3GraphVertex {
        VL_RTTI_IMPL(DepVtx, V3GraphVertex)
        AstClass* ধ্রুবক m_classp;  // Class associated with a method
        Ast_গ্রন্থি* ধ্রুবক m_nodep;  // AST node represented by this graph vertex

        // ACCESSORS
        string name() ধ্রুবক override {
            যদি (m_classp) {
                যদি (VN_IS(nodep(), CFunc)) {
                    ফেরত cvtToHex(nodep()) + ' ' + classp()->name() + "::" + nodep()->name();
                }
            }
            ফেরত cvtToHex(nodep()) + ' ' + nodep()->prettyTypeName();
        }
        File_সারি* fileline() ধ্রুবক override { ফেরত nodep()->fileline(); }

    সর্বজনীন:
        // CONSTRUCTORS
        DepVtx(V3Graph* graphp, Ast_গ্রন্থি* nodep, AstClass* classp)
            : V3GraphVertex{graphp}
            , m_classp{classp}
            , m_nodep{nodep} {}
        ~DepVtx() override = পূর্বনির্ধারিত;

        // ACCESSORS
        আভাসী Ast_গ্রন্থি* nodep() ধ্রুবক VL_MT_STABLE { ফেরত m_nodep; }
        আভাসী Ast_গ্রন্থি* classp() ধ্রুবক VL_MT_STABLE { ফেরত m_classp; }
    };

    শ্রেণী SuspendDepVtx final : সর্বজনীন DepVtx {
        VL_RTTI_IMPL(SuspendDepVtx, DepVtx)
        string dotColor() ধ্রুবক override {
            যদি (hasFlags(nodep(), T_SUSPENDER)) ফেরত "red";
            যদি (hasFlags(nodep(), T_SUSPENDEE)) ফেরত "blue";
            ফেরত "black";
        }

    সর্বজনীন:
        SuspendDepVtx(V3Graph* graphp, Ast_গ্রন্থি* nodep, AstClass* classp)
            : DepVtx{graphp, nodep, classp} {}
        ~SuspendDepVtx() override = পূর্বনির্ধারিত;
    };

    শ্রেণী NeedsProcDepVtx final : সর্বজনীন DepVtx {
        VL_RTTI_IMPL(NeedsProcDepVtx, DepVtx)
        string dotColor() ধ্রুবক override {
            যদি (hasFlags(nodep(), T_HAS_PROC)) ফেরত "blue";
            যদি (hasFlags(nodep(), T_NEEDS_PROC)) ফেরত "green";
            যদি (hasFlags(nodep(), T_FORCES_PROC)) ফেরত "red";
            ফেরত "black";
        }

    সর্বজনীন:
        NeedsProcDepVtx(V3Graph* graphp, Ast_গ্রন্থি* nodep, AstClass* classp)
            : DepVtx{graphp, nodep, classp} {}
        ~NeedsProcDepVtx() override = পূর্বনির্ধারিত;
    };

    // NODE STATE
    //  AstClass::user1() -> bool.  Class member cache has been refreshed.
    //  Ast{NodeProcedure,CFunc,Begin}::user2()  -> uint64_t.  Set to >= T_SUSP if
    //                                                         process/task suspendable
    //                                                         and to T_PROC if it
    //                                                         needs process metadata.
    //  Ast{NodeProcedure,CFunc,Begin}::user3()  -> DependencyVertex*.  Vertex in m_suspGraph
    //  Ast{NodeProcedure,CFunc,Begin}::user3()  -> DependencyVertex*.  Vertex in m_procGraph
    ধ্রুবক VNUser3InUse m_user3InUse;
    ধ্রুবক VNUser4InUse m_user4InUse;

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
    DepVtx* getSuspendDepVtx(Ast_গ্রন্থি* ধ্রুবক nodep) {
        AstClass* classp = nullptr;
        যদি (AstCFunc* funcp = VN_CAST(nodep, CFunc)) {
            যদি (funcp->scopep() && funcp->scopep()->modp()) {
                classp = VN_CAST(funcp->scopep()->modp(), Class);
            }
        }
        যদি (!nodep->user3p()) nodep->user3p(নতুন SuspendDepVtx{&m_suspGraph, nodep, classp});
        ফেরত nodep->user3u().to<SuspendDepVtx*>();
    }
    DepVtx* getNeedsProcDepVtx(Ast_গ্রন্থি* ধ্রুবক nodep) {
        AstClass* classp = nullptr;
        যদি (AstCFunc* funcp = VN_CAST(nodep, CFunc)) {
            যদি (funcp->scopep() && funcp->scopep()->modp()) {
                classp = VN_CAST(funcp->scopep()->modp(), Class);
            }
        }
        যদি (!nodep->user4p()) nodep->user4p(নতুন NeedsProcDepVtx{&m_procGraph, nodep, classp});
        ফেরত nodep->user4u().to<NeedsProcDepVtx*>();
    }
    // Pass timing flag between nodes
    যৌক্তিক passFlag(ধ্রুবক Ast_গ্রন্থি* from, Ast_গ্রন্থি* to, গ্রন্থি_Flag flag) {
        যদি ((from->user2() & flag) && !(to->user2() & flag)) {
            addFlags(to, flag);
            ফেরত সত্য;
        }
        ফেরত মিথ্যা;
    }
    // Propagate flag to all nodes that depend on the given one
    শূন্য propagateFlags(DepVtx* ধ্রুবক vxp, গ্রন্থি_Flag flag) {
        auto* ধ্রুবক parentp = vxp->nodep();
        জন্য (V3GraphEdge& edge : vxp->outEdges()) {
            auto* ধ্রুবক depVxp = static_cast<DepVtx*>(edge.শীর্ষ());
            Ast_গ্রন্থি* ধ্রুবক depp = depVxp->nodep();
            যদি (passFlag(parentp, depp, flag)) propagateFlags(depVxp, flag);
        }
    }
    template <typename T_Predicate>
    শূন্য propagateFlagsIf(DepVtx* ধ্রুবক vxp, গ্রন্থি_Flag flag, T_Predicate p) {
        auto* ধ্রুবক parentp = vxp->nodep();
        জন্য (V3GraphEdge& edge : vxp->outEdges()) {
            auto* ধ্রুবক depVxp = static_cast<DepVtx*>(edge.শীর্ষ());
            Ast_গ্রন্থি* ধ্রুবক depp = depVxp->nodep();
            যদি (p(&edge) && passFlag(parentp, depp, flag)) propagateFlagsIf(depVxp, flag, p);
        }
    }
    template <typename T_Predicate>
    শূন্য propagateFlagsReversedIf(DepVtx* ধ্রুবক vxp, গ্রন্থি_Flag flag, T_Predicate p) {
        auto* ধ্রুবক parentp = vxp->nodep();
        জন্য (V3GraphEdge& edge : vxp->inEdges()) {
            auto* ধ্রুবক depVxp = static_cast<DepVtx*>(edge.fromp());
            Ast_গ্রন্থি* ধ্রুবক depp = depVxp->nodep();
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
        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক condp = V3Const::constifyEdit(nodep->condp());
        যদি (AstConst* ধ্রুবক constp = VN_CAST(condp, Const)) {
            যদি (!nodep->fileline()->warnIsOff(V3ErrorCode::WAITCONST)) {
                condp->v3warn(WAITCONST, "Wait statement condition is constant");
            }
            যদি (!constp->isZero()) {
                // Remove AstWait before we track process as T_SUSPENDER
                যদি (Ast_গ্রন্থি* ধ্রুবক stmtsp = nodep->stmtsp()) {
                    stmtsp->unlink_Fr_Back_With_পরবর্তী();
                    nodep->replaceWith(stmtsp);
                } অন্যথা {
                    nodep->unlinkFrBack();
                }
                VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
                ফেরত;
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
        DepVtx* ধ্রুবক sVxp = getSuspendDepVtx(nodep);
        DepVtx* ধ্রুবক pVxp = getNeedsProcDepVtx(nodep);
        যদি (!m_classp) ফেরত;

        // Go over overridden functions

        std::queue<AstClassExtends*> extends;
        যদি (m_classp->extendsp()) extends.ঠেলো(m_classp->extendsp());

        যখন (!extends.empty()) {
            AstClassExtends* ext_list = extends.front();
            extends.বের_করো();

            জন্য (AstClassExtends* cextp = ext_list; cextp;
                 cextp = VN_AS(cextp->nextp(), ClassExtends)) {
                // TODO: It is possible that a methods the same name in the base class is not
                // actually overridden by our method. If this causes a problem, traverse to
                // the root of the inheritance hierarchy and check if the original method is
                // virtual or not.
                যদি (auto* ধ্রুবক overriddenp
                    = VN_CAST(m_memberMap.findMember(cextp->classp(), nodep->name()), CFunc)) {
                    // Suspendability and process affects typing, so they propagate both ways
                    DepVtx* ধ্রুবক overriddenSVxp = getSuspendDepVtx(overriddenp);
                    DepVtx* ধ্রুবক overriddenPVxp = getNeedsProcDepVtx(overriddenp);
                    নতুন V3GraphEdge{&m_suspGraph, sVxp, overriddenSVxp, P_SIGNATURE};
                    নতুন V3GraphEdge{&m_suspGraph, overriddenSVxp, sVxp, P_SIGNATURE};
                    নতুন V3GraphEdge{&m_procGraph, pVxp, overriddenPVxp, P_SIGNATURE};
                    নতুন V3GraphEdge{&m_procGraph, overriddenPVxp, pVxp, P_SIGNATURE};
                } অন্যথা {
                    AstClassExtends* more_extends = cextp->classp()->extendsp();
                    যদি (more_extends) extends.ঠেলো(more_extends);
                }
            }
        }
    }
    শূন্য visit(Ast_গ্রন্থি_CCall* nodep) override {
        AstCFunc* ধ্রুবক funcp = nodep->funcp();
        UASSERT_OBJ(funcp, nodep, "AstNodeCCall must have non-null funcp post-link");
        UASSERT_OBJ(m_procp, nodep, "AstNodeCCall must be inside a procedure/CFunc/Begin");

        UINFO(9, "V3Timing: Processing CCall to " << funcp->name() << " in dependency graph\n");
        নতুন V3GraphEdge{&m_suspGraph, getSuspendDepVtx(funcp), getSuspendDepVtx(m_procp), P_CALL};

        নতুন V3GraphEdge{&m_procGraph, getNeedsProcDepVtx(funcp), getNeedsProcDepVtx(m_procp),
                        P_CALL};

        iterateChildren(nodep);
    }
    শূন্য visit(AstBegin* nodep) override {
        VL_RESTORER(m_procp);
        VL_RESTORER(m_underFork);

        যদি (!m_underFork || (m_underFork & F_MIGHT_SUSPEND))
            নতুন V3GraphEdge{&m_suspGraph, getSuspendDepVtx(nodep), getSuspendDepVtx(m_procp),
                            m_underFork ? P_FORK : P_CALL};

        নতুন V3GraphEdge{&m_procGraph, getNeedsProcDepVtx(nodep), getNeedsProcDepVtx(m_procp),
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

সর্বজনীন:
    // CONSTRUCTORS
    explicit TimingSuspendableVisitor(AstNetlist* nodep) {
        iterate(nodep);
        m_suspGraph.removeTransitiveEdges();
        m_procGraph.removeTransitiveEdges();
        // Propagate suspendability
        জন্য (V3GraphVertex& vtx : m_suspGraph.vertices()) {
            DepVtx& depVtx = static_cast<DepVtx&>(vtx);
            যদি (hasFlags(depVtx.nodep(), T_SUSPENDEE)) propagateFlags(&depVtx, T_SUSPENDEE);
        }
        যদি (dumpGraphLevel() >= 6) m_suspGraph.dumpDotFilePrefixed("timing_deps");

        // Propagate T_HAS_PROCESS
        জন্য (V3GraphVertex& vtx : m_procGraph.vertices()) {
            DepVtx& depVtx = static_cast<DepVtx&>(vtx);
            // Find processes that'll allocate VlProcess
            যদি (hasFlags(depVtx.nodep(), T_FORCES_PROC)) {
                propagateFlagsIf(&depVtx, T_FORCES_PROC, [&](ধ্রুবক V3GraphEdge* e) -> যৌক্তিক {
                    ফেরত !hasFlags(static_cast<DepVtx*>(e->fromp())->nodep(), T_ALLOCS_PROC);
                });
            }
            // Mark nodes on paths between processes and statements that use VlProcess
            যদি (hasFlags(depVtx.nodep(), T_NEEDS_PROC)) {
                propagateFlagsIf(&depVtx, T_NEEDS_PROC, [&](ধ্রুবক V3GraphEdge* e) -> যৌক্তিক {
                    ফেরত !hasFlags(static_cast<DepVtx*>(e->শীর্ষ())->nodep(), T_ALLOCS_PROC);
                });
            }
        }
        জন্য (V3GraphVertex& vtx : m_procGraph.vertices()) {
            DepVtx& depVtx = static_cast<DepVtx&>(vtx);
            // Mark nodes that will be emitted with a VlProcess argument
            যদি (hasFlags(depVtx.nodep(), T_ALLOCS_PROC | T_FORCES_PROC)) {
                addFlags(depVtx.nodep(), T_HAS_PROC);
                propagateFlagsReversedIf(&depVtx, T_HAS_PROC, [&](ধ্রুবক V3GraphEdge* e) -> যৌক্তিক {
                    ফেরত hasFlags(static_cast<DepVtx*>(e->fromp())->nodep(), T_NEEDS_PROC);
                });
            }
        }
        যদি (dumpGraphLevel() >= 6) m_procGraph.dumpDotFilePrefixed("proc_deps");
    }
    ~TimingSuspendableVisitor() override = পূর্বনির্ধারিত;
};

// ######################################################################
//  Transform nodes affected by timing

শ্রেণী TimingControlVisitor final : সর্বজনীন VNVisitor {
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
    AstNetlist* ধ্রুবক m_netlistp;  // Root node
    Ast_শীর্ষ_Scope* ধ্রুবক m_শীর্ষ_Scopep = m_netlistp->শীর্ষ_Scopep();  // The AstTopScope
    AstScope* ধ্রুবক m_scopeTopp = m_শীর্ষ_Scopep->scopep();  // Scope at the top
    AstClass* m_classp = nullptr;  // Current class
    AstScope* m_scopep = nullptr;  // Current scope
    AstActive* m_activep = nullptr;  // Current active
    Ast_গ্রন্থি* m_procp = nullptr;  // NodeProcedure/CFunc/Begin we're under
    যৌক্তিক m_hasProcess = মিথ্যা;  // True if current scope has a VlProcess handle available
    পূর্ণসংখ্যা m_forkCnt = 0;  // Number of forks inside a module
    যৌক্তিক m_underJumpBlock = মিথ্যা;  // True if we are inside of a jump-block
    যৌক্তিক m_underProcedure = মিথ্যা;  // True if we are under an always or initial
    যৌক্তিক m_underIfaceCFunc = মিথ্যা;  // True if we are under a CFunc owned by an interface scope
    যৌক্তিক m_hasStaticZeroDelay = মিথ্যা;  // True if we have a static #0 delay
    std::vector<File_সারি*> m_unknownDelayFlps;  // Locations of AstDelay with non-constant value

    // Unique names
    V3UniqueNames m_dlyforkNames{"__Vdlyfork"};  // Names for temp AssignW vars
    V3UniqueNames m_intra_মান_Names{"__Vintraval"};  // Intra assign delay value var names
    V3UniqueNames m_intra_সূচক_Names{"__Vintraidx"};  // Intra assign delay index var names
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
    আকার_t m_statZeroDelays = 0;  // Number of statically known #0 delays
    আকার_t m_statConstDelays = 0;  // Number of statically known #const (non-zero) delays
    আকার_t m_statVariableDelays = 0;  // Number of delays with value unknown at compile time

    // METHODS
    // Transform an assignment with an intra timing control into a timing control with the
    // assignment under it
    Ast_গ্রন্থি* factorOutTimingControl(Ast_গ্রন্থি_Assign* nodep) ধ্রুবক {
        Ast_গ্রন্থি* stmtp = nodep;
        Ast_গ্রন্থি* ধ্রুবক controlp = nodep->timingControlp();
        যদি (AstDelay* ধ্রুবক delayp = VN_CAST(controlp, Delay)) {
            stmtp->replaceWith(delayp->unlinkFrBack());
            delayp->addStmtsp(stmtp);
            stmtp = delayp;
        } অন্যথা যদি (Ast_Sen_বৃক্ষ* ধ্রুবক sentreep = VN_CAST(controlp, Sen_বৃক্ষ)) {
            AstEventControl* ধ্রুবক eventControlp
                = নতুন AstEventControl{sentreep->fileline(), sentreep->unlinkFrBack(), nullptr};
            stmtp->replaceWith(eventControlp);
            eventControlp->addStmtsp(stmtp);
            stmtp = eventControlp;
        } অন্যথা যদি (AstBegin* ধ্রুবক beginp = VN_CAST(controlp, Begin)) {
            // Begin from V3AssertPre
            stmtp->replaceWith(beginp->unlinkFrBack());
            beginp->addStmtsp(stmtp);
            stmtp = beginp;
        }
        ফেরত stmtp == nodep ? nullptr : stmtp;
    }
    স্থির যৌক্তিক staticallyNonZeroDelay(ধ্রুবক Ast_গ্রন্থি_রাশিমালা* valuep) {
        যদি (ধ্রুবক AstConst* ধ্রুবক constp = VN_CAST(valuep, Const)) ফেরত !constp->isZero();
        যদি (ধ্রুবক AstCond* ধ্রুবক condp = VN_CAST(valuep, Cond)) {
            ফেরত staticallyNonZeroDelay(condp->thenp())
                   && staticallyNonZeroDelay(condp->elsep());
        }
        যদি (ধ্রুবক AstMul* ধ্রুবক mulp = VN_CAST(valuep, Mul)) {
            ধ্রুবক AstConst* ধ্রুবক lhsConstp = VN_CAST(mulp->lhsp(), Const);
            ধ্রুবক AstConst* ধ্রুবক rhsConstp = VN_CAST(mulp->rhsp(), Const);
            যদি (lhsConstp && !lhsConstp->isZero()) ফেরত staticallyNonZeroDelay(mulp->rhsp());
            যদি (rhsConstp && !rhsConstp->isZero()) ফেরত staticallyNonZeroDelay(mulp->lhsp());
        }
        ফেরত মিথ্যা;
    }
    // Creates the global delay scheduler variable
    AstVarScope* getCreateDelayScheduler() {
        যদি (m_delaySchedp) ফেরত m_delaySchedp;
        auto* ধ্রুবক dlySchedDtp = নতুন AstBasicDType{
            m_scopeTopp->fileline(), VBasicDTypeKwd::DELAY_SCHEDULER, VSigning::UNSIGNED};
        m_netlistp->typeTablep()->addTypesp(dlySchedDtp);
        m_delaySchedp = m_শীর্ষ_Scopep->create_অস্থায়ী("__VdlySched", dlySchedDtp);
        // Delay scheduler has to be accessible from top
        m_delaySchedp->varp()->sigPublic(সত্য);
        m_netlistp->delaySchedulerp(m_delaySchedp->varp());
        ফেরত m_delaySchedp;
    }
    // Creates the delay sentree
    Ast_Sen_বৃক্ষ* get_Create_Delay_Sen_বৃক্ষ() {
        যদি (m_delaySensesp) ফেরত m_delaySensesp;
        File_সারি* ধ্রুবক flp = m_scopeTopp->fileline();
        auto* ধ্রুবক awaitingCurrentTimep
            = নতুন AstCMethodHard{flp, নতুন AstVarRef{flp, getCreateDelayScheduler(), VAccess::READ},
                                 VCMethod::SCHED_AWAITING_CURRENT_TIME};
        awaitingCurrentTimep->dtypeSetBit();
        m_delaySensesp
            = নতুন Ast_Sen_বৃক্ষ{flp, নতুন AstSenItem{flp, VEdgeType::ET_TRUE, awaitingCurrentTimep}};
        m_netlistp->শীর্ষ_Scopep()->addSenTreesp(m_delaySensesp);
        ফেরত m_delaySensesp;
    }
    // Creates the global dynamic trigger scheduler variable
    AstVarScope* getCreateDynamicTriggerScheduler() {
        যদি (m_dynamicSchedp) ফেরত m_dynamicSchedp;
        auto* ধ্রুবক dynSchedDtp
            = নতুন AstBasicDType{m_scopeTopp->fileline(), VBasicDTypeKwd::DYNAMIC_TRIGGER_SCHEDULER,
                                VSigning::UNSIGNED};
        m_netlistp->typeTablep()->addTypesp(dynSchedDtp);
        m_dynamicSchedp = m_শীর্ষ_Scopep->create_অস্থায়ী("__VdynSched", dynSchedDtp);
        ফেরত m_dynamicSchedp;
    }
    // Creates the dynamic trigger sentree
    Ast_Sen_বৃক্ষ* get_Create_Dynamic_Trigger_Sen_বৃক্ষ() {
        যদি (m_dynamicSensesp) ফেরত m_dynamicSensesp;
        File_সারি* ধ্রুবক flp = m_scopeTopp->fileline();
        auto* ধ্রুবক awaitingCurrentTimep = নতুন AstCMethodHard{
            flp, নতুন AstVarRef{flp, getCreateDynamicTriggerScheduler(), VAccess::READ},
            VCMethod::SCHED_EVALUATE};
        awaitingCurrentTimep->dtypeSetBit();
        m_dynamicSensesp
            = নতুন Ast_Sen_বৃক্ষ{flp, নতুন AstSenItem{flp, VEdgeType::ET_TRUE, awaitingCurrentTimep}};
        m_netlistp->শীর্ষ_Scopep()->addSenTreesp(m_dynamicSensesp);
        ফেরত m_dynamicSensesp;
    }
    // Creates the event variable to trigger in NBA region
    AstEventControl* createNbaEventControl(File_সারি* flp) {
        যদি (!m_netlistp->nbaEventp()) {
            auto* ধ্রুবক nbaEventDtp = নতুন AstBasicDType{m_scopeTopp->fileline(),
                                                        VBasicDTypeKwd::EVENT, VSigning::UNSIGNED};
            m_netlistp->typeTablep()->addTypesp(nbaEventDtp);
            m_netlistp->nbaEventp(m_শীর্ষ_Scopep->create_অস্থায়ী("__VnbaEvent", nbaEventDtp));
            v3Global.setHasEvents();
        }
        ফেরত নতুন AstEventControl{
            flp,
            নতুন Ast_Sen_বৃক্ষ{
                flp, নতুন AstSenItem{flp, VEdgeType::ET_EVENT,
                                    নতুন AstVarRef{flp, m_netlistp->nbaEventp(), VAccess::READ}}},
            nullptr};
    }
    // Creates the variable that, if set, causes the NBA event to be triggered
    AstAssign* createNbaEventTriggerAssignment(File_সারি* flp) {
        যদি (!m_netlistp->nbaEventTriggerp()) {
            m_netlistp->nbaEventTriggerp(m_শীর্ষ_Scopep->create_অস্থায়ী("__VnbaEventTrigger", 1));
        }
        ফেরত নতুন AstAssign{flp,
                             নতুন AstVarRef{flp, m_netlistp->nbaEventTriggerp(), VAccess::WRITE},
                             নতুন AstConst{flp, AstConst::BitTrue{}}};
    }
    // Returns true if we are under a class or interface function, or the tree references locals.
    // These are cases where static, globally-evaluated triggers are not suitable.
    যৌক্তিক needDynamicTrigger(Ast_গ্রন্থি* ধ্রুবক nodep) ধ্রুবক {
        ফেরত m_classp || m_underIfaceCFunc || nodep->exists([](Ast_গ্রন্থি* ধ্রুবক nodep) {
            যদি (Ast_গ্রন্থি_Var_Ref* varp = VN_CAST(nodep, গ্রন্থি_Var_Ref)) {
                ফেরত varp->varp()->isFuncLocal();
            }
            ফেরত !nodep->isPure();
        });
    }
    // Creates a trigger scheduler variable
    AstVarScope* getCreateTriggerSchedulerp(Ast_Sen_বৃক্ষ* ধ্রুবক sentreep) {
        যদি (!sentreep->user1p()) {
            যদি (!m_trigSchedDtp) {
                m_trigSchedDtp
                    = নতুন AstBasicDType{m_scopeTopp->fileline(), VBasicDTypeKwd::TRIGGER_SCHEDULER,
                                        VSigning::UNSIGNED};
                m_netlistp->typeTablep()->addTypesp(m_trigSchedDtp);
            }
            AstVarScope* ধ্রুবক trigSchedp
                = m_শীর্ষ_Scopep->create_অস্থায়ী(m_trigSchedNames.get(sentreep), m_trigSchedDtp);
            sentreep->user1p(trigSchedp);
        }
        ফেরত VN_AS(sentreep->user1p(), VarScope);
    }
    // Creates a string describing the sentree
    AstCExpr* createEventDescription(Ast_Sen_বৃক্ষ* ধ্রুবক sentreep) ধ্রুবক {
        যদি (!sentreep->user2p()) {
            std::stringstream ss;
            V3EmitV::verilog_For_বৃক্ষ(sentreep, ss);
            // possibly a multiline string
            // Names were hashed by V3Name, show the user the original text
            std::string comment = VName::dehash(ss.str());
            std::replace(comment.begin(), comment.end(), '\n', ' ');
            // Escape quotes, e.g. from string-indexed event array selects
            comment = '"' + V3OutFormatter::quoteNameControls(comment) + '"';
            AstCExpr* ধ্রুবক commentp = নতুন AstCExpr{sentreep->fileline(), comment};
            commentp->dtypeSetString();
            sentreep->user2p(commentp);
            ফেরত commentp;
        }
        ফেরত VN_AS(sentreep->user2p(), CExpr)->clone_বৃক্ষ(মিথ্যা);
    }
    // Adds debug info to a hardcoded method call
    শূন্য addDebugInfo(AstCMethodHard* ধ্রুবক methodp) ধ্রুবক {
        File_সারি* ধ্রুবক flp = methodp->fileline();
        ধ্রুবক যৌক্তিক protectIds = v3Global.opt.protectIds();
        AstCExpr* ধ্রুবক ap
            = নতুন AstCExpr{flp, protectIds ? "VL_UNKNOWN" : '"' + flp->filenameEsc() + '"'};
        ap->dtypeSetString();
        methodp->addPinsp(ap);
        AstCExpr* ধ্রুবক bp = নতুন AstCExpr{flp, protectIds ? "0" : cvtToStr(flp->lineno())};
        bp->dtypeSetString();
        methodp->addPinsp(bp);
    }
    // Adds debug info to a trigSched.trigger() call
    শূন্য addEventDebugInfo(AstCMethodHard* ধ্রুবক methodp, Ast_Sen_বৃক্ষ* ধ্রুবক sentreep) ধ্রুবক {
        যদি (v3Global.opt.protectIds()) {
            File_সারি* ধ্রুবক flp = sentreep->fileline();
            AstCExpr* ধ্রুবক descp = নতুন AstCExpr{flp, "VL_UNKNOWN"};
            descp->dtypeSetString();
            methodp->addPinsp(descp);
            addDebugInfo(methodp);
            ফেরত;
        }
        methodp->addPinsp(createEventDescription(sentreep));
        addDebugInfo(methodp);
    }
    // Adds process pointer to a hardcoded method call
    শূন্য addProcessInfo(AstCMethodHard* ধ্রুবক methodp) ধ্রুবক {
        File_সারি* ধ্রুবক flp = methodp->fileline();
        AstCExpr* ধ্রুবক ap
            = নতুন AstCExpr{flp, (m_procp && m_hasProcess) ? "vlProcess" : "nullptr"};
        methodp->addPinsp(ap);
    }
    // Creates the fork handle type and returns it
    AstBasicDType* getCreateForkSyncDTypep() {
        যদি (m_forkDtp) ফেরত m_forkDtp;
        m_forkDtp = নতুন AstBasicDType{m_scopeTopp->fileline(), VBasicDTypeKwd::FORK_SYNC,
                                      VSigning::UNSIGNED};
        m_netlistp->typeTablep()->addTypesp(m_forkDtp);
        ফেরত m_forkDtp;
    }
    // Move `nodep` into `AstCLocalScope`.
    AstCLocalScope* addCLocalScope(File_সারি* ধ্রুবক flp, Ast_গ্রন্থি* ধ্রুবক nodep) ধ্রুবক {
        যদি (!nodep) ফেরত nullptr;
        VNRelinker handle;
        nodep->unlinkFrBack(&handle);
        AstCLocalScope* ধ্রুবক cscopep = নতুন AstCLocalScope{flp, nodep};
        handle.relink(cscopep);
        ফেরত cscopep;
    }
    // Create a temp variable and optionally put it before the specified node (mark local if so)
    AstVarScope* create_অস্থায়ী(File_সারি* ধ্রুবক flp, ধ্রুবক std::string& name,
                            Ast_গ্রন্থি_DType* ধ্রুবক dtypep, Ast_গ্রন্থি* ধ্রুবক ঢোকাও_Beforep = nullptr) {
        AstVar* varp;
        যদি (ঢোকাও_Beforep) {
            varp = নতুন AstVar{flp, VVarType::BLOCKTEMP, name, dtypep};
            varp->funcLocal(সত্য);
            ঢোকাও_Beforep->add_Here_This_As_পরবর্তী(varp);
        } অন্যথা {
            varp = নতুন AstVar{flp, VVarType::MODULETEMP, name, dtypep};
            m_scopep->modp()->addStmtsp(varp);
        }
        AstVarScope* ধ্রুবক vscp = নতুন AstVarScope{flp, m_scopep, varp};
        m_scopep->addVarsp(vscp);
        ফেরত vscp;
    }
    // Add a done() call on the fork sync
    শূন্য addForkDone(AstBegin* ধ্রুবক beginp, AstVarScope* ধ্রুবক forkVscp) ধ্রুবক {
        File_সারি* ধ্রুবক flp = beginp->fileline();
        auto* ধ্রুবক donep = নতুন AstCMethodHard{
            beginp->fileline(), নতুন AstVarRef{flp, forkVscp, VAccess::WRITE}, VCMethod::FORK_DONE};
        donep->dtypeSetVoid();
        addDebugInfo(donep);
        beginp->addStmtsp(donep->makeStmt());
    }
    স্থির যৌক্তিক has_Disable_Queue_ঠেলো_Self_Prefix(AstBegin* ধ্রুবক beginp) {
        // LinkJump prepends disable-by-name registration as:
        //   __VprocessQueue_*.push_back(std::process::self())
        // V3LiftExpr lifts std::process::self() into an assignment to a temporary, then V3Task
        // lowers that assignment's function call into a leading comment and AstStmtExpr. Thus the
        // push_back is no longer necessarily the first statement. Scan across this generated
        // prefix for it, stopping at the fork-start sentinel or the branch body. Registrations for
        // nested forks live in sub-blocks and so are not in this statement list.
        জন্য (Ast_গ্রন্থি* stmtp = beginp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            যদি (VN_IS(stmtp, Comment)) চালু;
            Ast_Stmt_রাশিমালা* ধ্রুবক stmtExprp = VN_CAST(stmtp, Stmt_রাশিমালা);
            যদি (!stmtExprp) বিরতি;
            যদি (stmtExprp->is_Disable_Queue_ঠেলো_Self_Stmt()) ফেরত সত্য;
        }
        ফেরত মিথ্যা;
    }
    // Register a callback so killing a process-backed fork branch decrements the join counter
    শূন্য addForkOnKill(AstBegin* ধ্রুবক beginp, AstVarScope* ধ্রুবক forkVscp) ধ্রুবক {
        যদি (!beginp->needProcess() && !has_Disable_Queue_ঠেলো_Self_Prefix(beginp)) ফেরত;
        File_সারি* ধ্রুবক flp = beginp->fileline();
        AstCMethodHard* ধ্রুবক onKillp = নতুন AstCMethodHard{
            flp, নতুন AstVarRef{flp, forkVscp, VAccess::WRITE}, VCMethod::FORK_ON_KILL};
        onKillp->dtypeSetVoid();
        AstCExpr* ধ্রুবক processp = নতুন AstCExpr{flp, "vlProcess"};
        processp
            ->dtypeSetVoid();  // Opaque process reference; type is irrelevant for hardcoded emit
        onKillp->addPinsp(processp);
        Ast_গ্রন্থি_Stmt* ধ্রুবক stmtp = onKillp->makeStmt();
        beginp->prependStmtsp(stmtp);
    }
    // Handle the 'join' part of a fork..join
    শূন্য makeForkJoin(AstFork* ধ্রুবক forkp) {
        // Create a fork sync var
        File_সারি* ধ্রুবক flp = forkp->fileline();
        // Insert the sync var directly before the fork
        Ast_গ্রন্থি* ধ্রুবক ঢোকাও_Beforep = forkp;
        // Make sure all references to the fork var are contained within a single scope to prevent
        // moving statements accessing it in case of large functions splitting.
        addCLocalScope(flp, ঢোকাও_Beforep);
        AstVarScope* forkVscp
            = create_অস্থায়ী(flp, forkp->name() + "__sync", getCreateForkSyncDTypep(), ঢোকাও_Beforep);
        unsigned join_গুনতি = 0;  // Needed for join counter
        // Add a <fork sync>.done() to each begin
        জন্য (Ast_গ্রন্থি* beginp = forkp->forksp(); beginp; beginp = beginp->nextp()) {
            addForkOnKill(VN_AS(beginp, Begin), forkVscp);
            addForkDone(VN_AS(beginp, Begin), forkVscp);
            join_গুনতি++;
        }
        যদি (forkp->joinType().joinAny()) join_গুনতি = 1;
        // Set the join counter
        auto* ধ্রুবক initp = নতুন AstCMethodHard{flp, নতুন AstVarRef{flp, forkVscp, VAccess::WRITE},
                                               VCMethod::FORK_INIT, নতুন AstConst{flp, join_গুনতি}};
        initp->dtypeSetVoid();
        addProcessInfo(initp);
        forkp->add_Here_This_As_পরবর্তী(initp->makeStmt());
        // Await the join at the end
        auto* ধ্রুবক joinp = নতুন AstCMethodHard{flp, নতুন AstVarRef{flp, forkVscp, VAccess::WRITE},
                                               VCMethod::FORK_JOIN};
        joinp->dtypeSetVoid();
        addProcessInfo(joinp);
        addDebugInfo(joinp);
        forkp->add_পরবর্তী_Here(নতুন AstCAwait{flp, joinp});
    }

    // `procp` shall be a NodeProcedure/CFunc/Begin and within it vars from `varsp` will be placed.
    // `varsp` vector of vars which shall be localized.
    স্থির শূন্য localizeVars(Ast_গ্রন্থি* ধ্রুবক procp, ধ্রুবক std::vector<AstVar*>& varsp) {
        UASSERT(procp, "procp is nullptr");
        AstVar* newVarsp = nullptr;
        জন্য (AstVar* ধ্রুবক varp : varsp) {
            varp->funcLocal(সত্য);
            newVarsp = Ast_গ্রন্থি::add_পরবর্তী_Null(newVarsp, varp->unlinkFrBack());
        }
        যদি (Ast_গ্রন্থি_Procedure* ধ্রুবক গ্রন্থি_Procp = VN_CAST(procp, গ্রন্থি_Procedure)) {
            গ্রন্থি_Procp->prependStmtsp(newVarsp);
        } অন্যথা যদি (AstCFunc* ধ্রুবক cfuncp = VN_CAST(procp, CFunc)) {
            cfuncp->prependVarsp(newVarsp);
        } অন্যথা যদি (AstBegin* ধ্রুবক beginp = VN_CAST(procp, Begin)) {
            beginp->prependStmtsp(newVarsp);
        } অন্যথা {
            procp->v3fatalSrc(
                procp->prettyNameQ()
                << " is not of an expected type NodeProcedure/CFunc/Begin instead it is: "
                << procp->prettyTypeName());
        }
    }

    Ast_গ্রন্থি_রাশিমালা* delayToIntegral(AstDelay* nodep, Ast_গ্রন্থি_রাশিমালা* valuep) {
        ধ্রুবক AstConst* ধ্রুবক constp = VN_CAST(valuep, Const);
        // The fork start sentinel is already in ticks
        যদি (constp && constp->toUQuad() == std::numeric_limits<uint64_t>::max()) ফেরত valuep;
        ফেরত V3Timing::delayToTicks(valuep, nodep->timeunit());
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
                নতুন AstCStmt{nodep->fileline(), "vlProcess->state(VlProcess::FINISHED);"});
        }
    }
    শূন্য visit(AstJumpBlock* nodep) override {
        VL_RESTORER(m_underJumpBlock);
        m_underJumpBlock = সত্য;
        visit(static_cast<Ast_গ্রন্থি_Stmt*>(nodep));
    }
    শূন্য visit(AstAlways* nodep) override {
        যদি (nodep->user1SetOnce()) ফেরত;
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
        যদি (!hasFlags(nodep, T_SUSPENDEE)) ফেরত;
        nodep->setSuspendable();
        File_সারি* ধ্রুবক flp = nodep->fileline();
        Ast_Sen_বৃক্ষ* ধ্রুবক sentreep = m_activep->sentreep();
        যদি (sentreep->hasClocked()) {
            Ast_গ্রন্থি* ধ্রুবক bodysp = nodep->stmtsp()->unlink_Fr_Back_With_পরবর্তী();
            auto* ধ্রুবক controlp = নতুন AstEventControl{flp, sentreep->clone_বৃক্ষ(মিথ্যা), bodysp};
            nodep->addStmtsp(controlp);
            iterate(controlp);
        }
        // Note: The 'while (true)' outer loop will be added in V3Sched
        auto* ধ্রুবক activep = নতুন AstActive{
            flp, "", নতুন Ast_Sen_বৃক্ষ{flp, নতুন AstSenItem{flp, AstSenItem::Initial{}}}};
        activep->sen_বৃক্ষ_Storep(activep->sentreep());
        activep->addStmtsp(nodep->unlinkFrBack());
        m_activep->add_পরবর্তী_Here(activep);
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
        যদি (!(hasFlags(nodep, T_SUSPENDEE))) ফেরত;

        nodep->rtnType("VlCoroutine");
        // If in a class, create a shared pointer to 'this'
        যদি (m_classp) {
            AstCStmt* ধ্রুবক cstmtp = নতুন AstCStmt{nodep->fileline(), "VL_KEEP_THIS;"};
            nodep->prependStmtsp(cstmtp);
        }
        যদি (nodep->dpiExportImpl()) {
            nodep->exists([](AstCAwait* ধ্রুবক awaitp) -> যৌক্তিক {
                // A DPI-exported coroutine won't be able to block the calling code
                // Error on the await node; fall back to the function node
                awaitp->v3warn(E_UNSUPPORTED,
                               "Unsupported: Timing controls inside DPI-exported tasks");
                ফেরত সত্য;
            });
        }
    }
    শূন্য visit(Ast_গ্রন্থি_CCall* nodep) override {
        AstCFunc* ধ্রুবক funcp = nodep->funcp();

        // Skip automatic covergroup sampling calls
        যদি (funcp->isCovergroupSample()) {
            iterateChildren(nodep);
            ফেরত;
        }

        যদি (funcp->needProcess()) m_hasProcess = সত্য;
        যদি (hasFlags(funcp, T_SUSPENDEE) && !nodep->user1SetOnce()) {  // If suspendable
            // Calls to suspendables are always void return type, hence parent must be StmtExpr
            Ast_Stmt_রাশিমালা* ধ্রুবক stmtp = VN_AS(nodep->backp(), Stmt_রাশিমালা);
            stmtp->replaceWith(নতুন AstCAwait{nodep->fileline(), nodep->unlinkFrBack()});
            VL_DO_DANGLING(ঠেলো_Deletep(stmtp), stmtp);
        }
        iterateChildren(nodep);
    }
    শূন্য visit(AstDelay* nodep) override {
        UASSERT_OBJ(!nodep->isCycleDelay(), nodep,
                    "Cycle delays should have been handled in V3AssertPre");
        File_সারি* ধ্রুবক flp = nodep->fileline();

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
        AstCMethodHard* ধ্রুবক delayMethodp = নতুন AstCMethodHard{
            flp, নতুন AstVarRef{flp, getCreateDelayScheduler(), VAccess::WRITE},
            VCMethod::SCHED_DELAY, valuep};
        delayMethodp->dtypeSetVoid();
        addProcessInfo(delayMethodp);
        addDebugInfo(delayMethodp);
        // Create the co_await
        Ast_গ্রন্থি* ধ্রুবক awaitp = নতুন AstCAwait{flp, delayMethodp, get_Create_Delay_Sen_বৃক্ষ()};
        // Relink child statements after the co_await
        যদি (Ast_গ্রন্থি* ধ্রুবক stmtsp = nodep->stmtsp()) {
            awaitp->add_পরবর্তী(stmtsp->unlink_Fr_Back_With_পরবর্তী());
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
            ফেরত;
        }
        File_সারি* ধ্রুবক flp = nodep->fileline();
        // Relink child statements after the event control
        যদি (nodep->stmtsp()) nodep->add_পরবর্তী_Here(nodep->stmtsp()->unlink_Fr_Back_With_পরবর্তী());
        যদি (needDynamicTrigger(nodep->sentreep())) {
            // Create the trigger variable and init it with 0
            AstVarScope* ধ্রুবক trigvscp
                = create_অস্থায়ী(flp, m_dynTrigNames.get(nodep), nodep->findBitDType(), nodep);
            auto* ধ্রুবক initp = নতুন AstAssign{flp, নতুন AstVarRef{flp, trigvscp, VAccess::WRITE},
                                              নতুন AstConst{flp, AstConst::BitFalse{}}};
            // Await the eval step with the dynamic trigger scheduler. First, create the method
            // call
            auto* ধ্রুবক evalMethodp = নতুন AstCMethodHard{
                flp, নতুন AstVarRef{flp, getCreateDynamicTriggerScheduler(), VAccess::WRITE},
                VCMethod::SCHED_EVALUATION};
            evalMethodp->dtypeSetVoid();
            addProcessInfo(evalMethodp);
            auto* ধ্রুবক sentreep = nodep->sentreep();
            addEventDebugInfo(evalMethodp, sentreep);
            // Create the co_await
            AstCAwait* ধ্রুবক awaitEvalp
                = নতুন AstCAwait{flp, evalMethodp, get_Create_Dynamic_Trigger_Sen_বৃক্ষ()};
            // Construct the sen expression for this sentree
            UASSERT_OBJ(m_sen_রাশিমালা_Builderp, nodep, "No SenExprBuilder for this scope");
            auto* ধ্রুবক assignp = নতুন AstAssign{flp, নতুন AstVarRef{flp, trigvscp, VAccess::WRITE},
                                                m_sen_রাশিমালা_Builderp->build(sentreep).first};
            // Get the SenExprBuilder results
            ধ্রুবক Sen_রাশিমালা_Builder::Results senResults = m_sen_রাশিমালা_Builderp->getAndClearResults();
            localizeVars(m_procp, senResults.m_vars);
            // If post updates are destructive (e.g. clearFired on events), perform a
            // conservative pre-clear once before entering the wait loop so stale state from a
            // previous wait does not cause an immediate false-positive trigger.
            ধ্রুবক যৌক্তিক hasDestructivePostUpdates = !senResults.m_destructivePostUpdates.empty();
            // Create the trigger eval loop, which will await the evaluation step and check the
            // trigger
            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক condp
                = নতুন AstLogNot{flp, নতুন AstVarRef{flp, trigvscp, VAccess::READ}};
            AstLoop* ধ্রুবক loopp = নতুন AstLoop{flp};
            loopp->addStmtsp(নতুন AstLoopTest{flp, loopp, condp});
            loopp->addStmtsp(awaitEvalp);
            // Put pre updates before the trigger check and assignment
            জন্য (Ast_গ্রন্থি_Stmt* ধ্রুবক stmtp : senResults.m_preUpdates) loopp->addStmtsp(stmtp);
            // Then the trigger check and assignment
            loopp->addStmtsp(assignp);
            // Let the dynamic trigger scheduler know if this trigger was set
            // If it was, a call to the scheduler's evaluate() will return true
            AstCMethodHard* ধ্রুবক anyTriggeredMethodp = নতুন AstCMethodHard{
                flp, নতুন AstVarRef{flp, getCreateDynamicTriggerScheduler(), VAccess::WRITE},
                VCMethod::SCHED_ANY_TRIGGERED, নতুন AstVarRef{flp, trigvscp, VAccess::READ}};
            anyTriggeredMethodp->dtypeSetVoid();
            loopp->addStmtsp(anyTriggeredMethodp->makeStmt());
            // If the post update is destructive (e.g. event vars are cleared), create an await for
            // the post update step
            যদি (hasDestructivePostUpdates) {
                AstCAwait* ধ্রুবক awaitPostUpdatep = awaitEvalp->clone_বৃক্ষ(মিথ্যা);
                VN_AS(awaitPostUpdatep->exprp(), CMethodHard)->method(VCMethod::SCHED_POST_UPDATE);
                loopp->addStmtsp(awaitPostUpdatep);
            }
            // Put the post updates at the end of the loop
            জন্য (Ast_গ্রন্থি_Stmt* ধ্রুবক stmtp : senResults.m_postUpdates) loopp->addStmtsp(stmtp);
            // Finally, await the resumption step in 'act'
            AstCAwait* ধ্রুবক awaitResumep = awaitEvalp->clone_বৃক্ষ(মিথ্যা);
            VN_AS(awaitResumep->exprp(), CMethodHard)->method(VCMethod::SCHED_RESUMPTION);
            Ast_গ্রন্থি::add_পরবর্তী<Ast_গ্রন্থি_Stmt, Ast_গ্রন্থি_Stmt>(loopp, awaitResumep);
            // Replace the event control with one explicit stmt chain:
            //   init -> [inits] -> [optional destructive pre-clears] -> loop -> awaitResumption
            Ast_গ্রন্থি_Stmt* chainp = nullptr;
            chainp = Ast_গ্রন্থি::add_পরবর্তী_Null(chainp, initp);
            জন্য (Ast_গ্রন্থি_Stmt* ধ্রুবক stmtp : senResults.m_inits) {
                chainp = Ast_গ্রন্থি::add_পরবর্তী_Null(chainp, stmtp);
            }
            যদি (hasDestructivePostUpdates) {
                জন্য (Ast_গ্রন্থি_Stmt* ধ্রুবক stmtp : senResults.m_destructivePostUpdates) {
                    chainp = Ast_গ্রন্থি::add_পরবর্তী_Null(chainp, stmtp->clone_বৃক্ষ(মিথ্যা));
                }
            }
            chainp = Ast_গ্রন্থি::add_পরবর্তী_Null(chainp, loopp);
            nodep->replaceWith(chainp);
        } অন্যথা {
            auto* ধ্রুবক sentreep = m_finder.get_Sen_বৃক্ষ(nodep->sentreep());
            nodep->sentreep()->unlinkFrBack()->delete_বৃক্ষ();
            // Get this sentree's trigger scheduler
            // Replace self with a 'co_await trigSched.trigger()'
            auto* ধ্রুবক triggerMethodp = নতুন AstCMethodHard{
                flp, নতুন AstVarRef{flp, getCreateTriggerSchedulerp(sentreep), VAccess::WRITE},
                VCMethod::SCHED_TRIGGER};
            triggerMethodp->dtypeSetVoid();
            // If it should be committed immediately, pass true, otherwise false
            triggerMethodp->addPinsp(nodep->user2() ? নতুন AstConst{flp, AstConst::BitTrue{}}
                                                    : নতুন AstConst{flp, AstConst::BitFalse{}});
            addProcessInfo(triggerMethodp);
            addEventDebugInfo(triggerMethodp, sentreep);
            // Create the co_await
            AstCAwait* ধ্রুবক awaitp = নতুন AstCAwait{flp, triggerMethodp, sentreep};
            nodep->replaceWith(awaitp);
        }
        VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
    }
    শূন্য visit(Ast_গ্রন্থি_Assign* nodep) override {
        // Only process once to avoid infinite loops (due to the net delay)
        যদি (nodep->user1SetOnce()) ফেরত;
        UINFO(9, "control-visit " << nodep);
        File_সারি* ধ্রুবক flp = nodep->fileline();
        Ast_গ্রন্থি* controlp = factorOutTimingControl(nodep);
        ধ্রুবক যৌক্তিক inAssignDly = VN_IS(nodep, AssignDly);
        // Handle the intra assignment timing control
        // Transform if:
        // * there's a timing control in the assignment
        // * the assignment is an AssignDly and it's in a non-inlined function
        যদি (!controlp && (!inAssignDly || m_underProcedure)) {
            iterateChildren(nodep);
            ফেরত;
        }
        // The timing control, if any, which is put in a fork for an NBA
        Ast_গ্রন্থি* ধ্রুবক timingp = controlp;
        // Insert new vars before the timing control if we're in a function; in a process we can't
        // do that, as after V3Begin only modules and functions declare variables. These
        // intra-assignment vars will later be passed to forked processes by value.
        Ast_গ্রন্থি* ঢোকাও_Beforep = m_underProcedure ? nullptr : controlp;
        // Special case for NBA
        যদি (inAssignDly) {
            // Put it in a fork so it doesn't block
            // Could already be the only thing directly under a fork, reuse that if possible
            AstFork* forkp = !nodep->nextp() ? VN_CAST(nodep->firstAbovep(), Fork) : nullptr;
            যদি (!forkp) forkp = নতুন AstFork{flp, VJoinType::JOIN_NONE};
            যদি (!m_underProcedure) {
                // If it's in a function, it won't be handled by V3Delayed
                // Put it behind an additional named event that gets triggered in the NBA region
                AstEventControl* ধ্রুবক nbaEventControlp = createNbaEventControl(flp);
                AstAssign* ধ্রুবক trigAssignp = createNbaEventTriggerAssignment(flp);
                nodep->replaceWith(trigAssignp);
                trigAssignp->add_পরবর্তী_Here(nbaEventControlp);
                nbaEventControlp->addStmtsp(nodep);
                ঢোকাও_Beforep = forkp;
                যদি (!controlp) controlp = nbaEventControlp;
            }
            controlp->replaceWith(forkp);
            AstBegin* beginp = VN_CAST(controlp, Begin);
            যদি (!beginp) beginp = নতুন AstBegin{nodep->fileline(), "", controlp, মিথ্যা};
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
        ধ্রুবক auto replaceWithIntermediate = [&](Ast_গ্রন্থি_রাশিমালা* ধ্রুবক valuep,
                                                 ধ্রুবক std::string& name) {
            AstVarScope* ধ্রুবক newvscp = create_অস্থায়ী(flp, name, valuep->dtypep(), ঢোকাও_Beforep);
            // A pending update keeps its own value, even if its process schedules more
            যদি (inAssignDly && !ঢোকাও_Beforep) newvscp->fork_By_মান(সত্য);
            valuep->replaceWith(নতুন AstVarRef{flp, newvscp, VAccess::READ});
            controlp->add_Here_This_As_পরবর্তী(
                নতুন AstAssign{flp, নতুন AstVarRef{flp, newvscp, VAccess::WRITE}, valuep});
        };
        // NBAs with delays evaluate the indices selecting the target now (IEEE 1800-2023 10.4.2),
        // in order, each as a whole, also the selects and function calls in it. So follow the
        // selects of the target up from the variable, not into their indices. As an example, this
        // transforms 'x[a[i]][f(j)] <= #1 y' into 't0 = a[i]; t1 = f(j); x[t0][t1] <= #1 y'.
        // V3Const has split NBAs to concatenations, also streamed, into NBAs to each part.
        যদি (inAssignDly) {
            জন্য (Ast_গ্রন্থি* exprp = nodep->lhsp()->baseFromp(সত্য); exprp != nodep;
                 exprp = exprp->firstAbovep()) {
                যদি (AstSel* ধ্রুবক selp = VN_CAST(exprp, Sel)) {
                    // widthp should be const
                    যদি (!VN_IS(selp->lsbp(), Const)) {
                        replaceWithIntermediate(selp->lsbp(), m_intraLsbNames.get(nodep));
                    }
                } অন্যথা যদি (Ast_গ্রন্থি_Sel* ধ্রুবক selp = VN_CAST(exprp, গ্রন্থি_Sel)) {
                    যদি (!VN_IS(selp->bitp(), Const)) {
                        replaceWithIntermediate(selp->bitp(), m_intra_সূচক_Names.get(nodep));
                    }
                }
            }
        }
        // Replace the RHS with an intermediate value var
        replaceWithIntermediate(nodep->rhsp(), m_intra_মান_Names.get(nodep));
        যদি (!inAssignDly) ফেরত;
        // The process executing the NBA evaluates its delay (IEEE 1800-2023 4.9.4), not the forked
        // process, if it has side effects, e.g. calling a function that checks its process. A pure
        // one is evaluated as the forked process starts, at once.
        যদি (AstDelay* ধ্রুবক delayp = VN_CAST(timingp, Delay)) {
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
            ফেরত;
        }
        File_সারি* ধ্রুবক flp = nodep->fileline();
        AstAssign* ধ্রুবক trigAssignp = createNbaEventTriggerAssignment(flp);
        nodep->replaceWith(trigAssignp);
        AstFork* ধ্রুবক forkp = নতুন AstFork{flp, VJoinType::JOIN_NONE};
        trigAssignp->add_পরবর্তী_Here(forkp);
        যদি (m_underJumpBlock) addCLocalScope(flp, forkp);
        // The triggered event is the one referenced now, so evaluate handles and indices now
        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক eventp = nodep->operandp()->unlinkFrBack();
        ধ্রুবক auto evalNow = [&](Ast_গ্রন্থি_রাশিমালা* ধ্রুবক valuep, V3UniqueNames& names) {
            // Like for an NBA, a process uses a module variable, passed to the fork by value
            AstVarScope* ধ্রুবক vscp = create_অস্থায়ী(flp, names.get(nodep), valuep->dtypep(),
                                                 m_underProcedure ? nullptr : forkp);
            যদি (m_underProcedure) vscp->fork_By_মান(সত্য);
            valuep->replaceWith(নতুন AstVarRef{flp, vscp, VAccess::READ});
            // Unlike an event, a handle selecting one is only read
            valuep->foreach([](Ast_গ্রন্থি* ধ্রুবক np) {
                যদি (Ast_গ্রন্থি_Var_Ref* ধ্রুবক refp = VN_CAST(np, গ্রন্থি_Var_Ref)) {
                    refp->access(VAccess::READ);
                } অন্যথা যদি (AstMemberSel* ধ্রুবক selp = VN_CAST(np, MemberSel)) {
                    selp->access(VAccess::READ);
                }
            });
            forkp->add_Here_This_As_পরবর্তী(
                নতুন AstAssign{flp, নতুন AstVarRef{flp, vscp, VAccess::WRITE}, valuep});
        };
        Ast_গ্রন্থি_রাশিমালা* refp = eventp;
        যখন (Ast_গ্রন্থি_Sel* ধ্রুবক selp = VN_CAST(refp, গ্রন্থি_Sel)) {
            যদি (!VN_IS(selp->bitp(), Const)) evalNow(selp->bitp(), m_intra_সূচক_Names);
            refp = selp->fromp();
        }
        // Handle of a class (or virtual interface)
        যদি (AstMemberSel* ধ্রুবক selp = VN_CAST(refp, MemberSel)) {
            evalNow(selp->fromp(), m_intra_মান_Names);
        }
        AstEventControl* ধ্রুবক controlp = createNbaEventControl(flp);
        controlp->addStmtsp(নতুন AstFireEvent{flp, eventp, মিথ্যা});
        AstBegin* ধ্রুবক beginp = নতুন AstBegin{flp, "", controlp, মিথ্যা};
        addFlags(beginp, T_NBA_UPDATE);
        forkp->addForksp(beginp);
        VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
    }
    শূন্য visit(AstAssignW* nodep) override {
        File_সারি* ধ্রুবক flp = nodep->fileline();
        // Get the net delay unless this assignment was created for handling the net delay (user1)
        AstDelay* ধ্রুবক netDelayp = nodep->user1() ? nullptr : nodep->getLhsNetDelay();
        যদি (netDelayp) {
            যদি (nodep->timingControlp()) {
                // If this assignment has a delay, create another one to handle the net delay
                AstVarScope* ধ্রুবক newvscp = m_assignWTmps.make(flp, m_scopep, nodep->dtypep());
                AstAssignW* assignp = নতুন AstAssignW{
                    nodep->fileline(), nodep->lhsp()->unlinkFrBack(),
                    নতুন AstVarRef{flp, newvscp, VAccess::READ}, netDelayp->clone_বৃক্ষ(মিথ্যা)};
                assignp->user1(সত্য);
                nodep->add_পরবর্তী_Here(assignp);
                nodep->lhsp(নতুন AstVarRef{flp, newvscp, VAccess::WRITE});
            } অন্যথা {
                // Else just use this one with the net delay
                nodep->timingControlp(netDelayp->clone_বৃক্ষ(মিথ্যা));
            }
        }
        যদি (!nodep->timingControlp()) ফেরত;
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
        AstAlways* ধ্রুবক alwaysp = VN_AS(m_procp, Always);
        // Convert it to an Assign; the new assign with intra delay will be handled by
        // visit(AstNodeAssign*)
        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক lhs1p = nodep->lhsp()->unlinkFrBack();
        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক rhs1p = nodep->rhsp()->unlinkFrBack();
        Ast_গ্রন্থি* ধ্রুবক controlp = nodep->timingControlp()->unlinkFrBack();
        যদি (AstDelay* ধ্রুবক delayp = VN_CAST(controlp, Delay)) {
            যদি (Ast_গ্রন্থি_রাশিমালা* fallDelayp = delayp->fallDelay()) {
                fallDelayp = fallDelayp->unlinkFrBack();
                Ast_গ্রন্থি_রাশিমালা* lhsp = delayp->lhsp()->unlinkFrBack();
                // Use fall only for an all-zero value, rise otherwise.
                lhsp = delayToIntegral(delayp, lhsp);
                fallDelayp = delayToIntegral(delayp, fallDelayp);
                delayp->lhsp(
                    নতুন AstCond{flp, নতুন AstEq{flp, rhs1p->clone_বৃক্ষ(মিথ্যা), নতুন AstConst{flp, 0}},
                                fallDelayp, lhsp});
            }
        }
        AstAssign* ধ্রুবক assignp = নতুন AstAssign{nodep->fileline(), lhs1p, rhs1p, controlp};
        // Put the assignment in a fork..join_none.
        AstFork* ধ্রুবক forkp = নতুন AstFork{flp, VJoinType::JOIN_NONE};
        nodep->replaceWith(forkp);
        VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
        AstBegin* ধ্রুবক beginp = নতুন AstBegin{flp, "", assignp, মিথ্যা};
        forkp->addForksp(beginp);
        visit(forkp);  // Visit now as we need to do some post-processing
        // IEEE 1800-2023 10.3.3 - if the RHS value differs from the currently scheduled value to
        // be assigned, the currently scheduled assignment is descheduled. To keep track if an
        // assignment should be descheduled, each scheduled assignment event has a 'generation',
        // and if at assignment time its generation differs from the current generation, it won't
        // be performed
        AstAssign* ধ্রুবক preAssignp = VN_AS(beginp->stmtsp(), Assign);
        UASSERT_OBJ(preAssignp, alwaysp, "Pre-assign should be there from visit(AstFork)");
        AstAssign* ধ্রুবক postAssignp = VN_AS(preAssignp->nextp()->nextp(), Assign);
        UASSERT_OBJ(postAssignp, alwaysp, "Post-assign should be there from visit(AstFork)");
        // Increment generation and copy it to a local
        AstVarScope* ধ্রুবক generationVarp
            = m_assignWGens.make(flp, m_scopep, alwaysp->findUInt64DType());
        AstVarScope* ধ্রুবক genLocalVarp
            = create_অস্থায়ী(flp, generationVarp->varp()->name() + "__local",
                         alwaysp->findUInt64DType(), preAssignp);
        preAssignp->add_Here_This_As_পরবর্তী(
            নতুন AstAssign{flp, নতুন AstVarRef{flp, generationVarp, VAccess::WRITE},
                          নতুন AstAdd{flp, নতুন AstVarRef{flp, generationVarp, VAccess::READ},
                                     নতুন AstConst{flp, 1}}});
        preAssignp->add_Here_This_As_পরবর্তী(
            নতুন AstAssign{flp, নতুন AstVarRef{flp, genLocalVarp, VAccess::WRITE},
                          নতুন AstVarRef{flp, generationVarp, VAccess::READ}});
        // If the current generation is same as the one saved in the local var, assign
        beginp->addStmtsp(
            নতুন AstIf{flp,
                      নতুন AstEq{flp, নতুন AstVarRef{flp, generationVarp, VAccess::READ},
                                নতুন AstVarRef{flp, genLocalVarp, VAccess::READ}},
                      postAssignp->unlinkFrBack()});
        // Save scheduled RHS value before delay
        AstVarScope* ধ্রুবক tmpVarp
            = m_assignWTmps.make(flp, m_scopep, preAssignp->rhsp()->dtypep());
        AstVarRef* ধ্রুবক tmpAssignRhsp = VN_AS(preAssignp->lhsp(), VarRef)->clone_বৃক্ষ(মিথ্যা);
        tmpAssignRhsp->access(VAccess::WRITE);
        preAssignp->add_পরবর্তী_Here(
            নতুন AstAssign{flp, নতুন AstVarRef{flp, tmpVarp, VAccess::WRITE}, tmpAssignRhsp});
        // If the RHS is different from the currently scheduled value, schedule the new assignment
        // The generation will increase, effectively 'descheduling' the previous assignment.
        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক didNotInitp
            = নতুন AstLogNot{flp, নতুন AstCExpr{flp, "vlSymsp->__Vm_didInit", 1}};
        AstVarRef* ধ্রুবক firstIterp
            = নতুন AstVarRef{flp, m_netlistp->stlFirstIterationp(), VAccess::READ};
        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক schedCondp
            = নতুন AstLogOr{flp,
                           নতুন AstNeq{flp, preAssignp->rhsp()->clone_বৃক্ষ(মিথ্যা),
                                      নতুন AstVarRef{flp, tmpVarp, VAccess::READ}},
                           নতুন AstLogAnd{flp, didNotInitp, firstIterp}};
        alwaysp->addStmtsp(নতুন AstIf{flp, schedCondp, forkp->unlinkFrBack()});
    }
    শূন্য visit(AstDisableFork* nodep) override {
        যদি (m_hasProcess) ফেরত;
        // never reached by any process; remove to avoid compilation error
        VL_DO_DANGLING(nodep->unlinkFrBack()->delete_বৃক্ষ(), nodep);
    }
    শূন্য visit(AstWaitFork* nodep) override {
        যদি (m_hasProcess) {
            File_সারি* ধ্রুবক flp = nodep->fileline();
            AstCExpr* ধ্রুবক exprp = নতুন AstCExpr{flp, "vlProcess->completedFork()", 1};
            AstWait* ধ্রুবক waitp = নতুন AstWait{flp, exprp, nullptr};
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
        File_সারি* ধ্রুবক flp = nodep->fileline();
        Ast_গ্রন্থি* ধ্রুবক stmtsp = nodep->stmtsp();
        যদি (stmtsp) stmtsp->unlink_Fr_Back_With_পরবর্তী();
        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক condp = V3Const::constifyEdit(nodep->condp()->unlinkFrBack());
        auto* ধ্রুবক constp = VN_CAST(condp, Const);
        যদি (constp) {
            যদি (constp->isZero()) {
                // We have to await forever instead of simply returning in case we're deep in a
                // callstack
                AstCMethodHard* ধ্রুবক foreverMethodp = নতুন AstCMethodHard{
                    flp, নতুন AstVarRef{flp, getCreateDelayScheduler(), VAccess::WRITE},
                    VCMethod::SCHED_WAIT_FOREVER};
                foreverMethodp->dtypeSetVoid();
                addProcessInfo(foreverMethodp);
                addDebugInfo(foreverMethodp);
                AstCAwait* ধ্রুবক awaitp = নতুন AstCAwait{flp, foreverMethodp};
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
            auto* ধ্রুবক ifp = নতুন AstIf{
                flp, নতুন AstLogNot{flp, condp},
                নতুন AstEventControl{flp,
                                    নতুন Ast_Sen_বৃক্ষ{flp, নতুন AstSenItem{flp, VEdgeType::ET_TRUE,
                                                                       condp->clone_বৃক্ষ(মিথ্যা)}},
                                    nullptr}};
            যদি (stmtsp) Ast_গ্রন্থি::add_পরবর্তী<Ast_গ্রন্থি, Ast_গ্রন্থি>(ifp, stmtsp);
            nodep->replaceWith(ifp);
        } অন্যথা {
            // We are using a global sentree, so we cannot use ET_TRUE, as that could lead to the
            // active region never converging. Because of this, we need to use ET_CHANGED in a
            // loop.
            AstEventControl* ধ্রুবক controlp = নতুন AstEventControl{
                flp,
                নতুন Ast_Sen_বৃক্ষ{
                    flp, নতুন AstSenItem{flp, VEdgeType::ET_CHANGED, condp->clone_বৃক্ষ(মিথ্যা)}},
                nullptr};
            controlp->user2(সত্য);  // Commit immediately
            AstLoop* ধ্রুবক loopp = নতুন AstLoop{flp};
            loopp->addStmtsp(নতুন AstLoopTest{flp, loopp, নতুন AstLogNot{flp, condp}});
            loopp->addStmtsp(controlp);
            যদি (stmtsp) Ast_গ্রন্থি::add_পরবর্তী<Ast_গ্রন্থি, Ast_গ্রন্থি>(loopp, stmtsp);
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
        যদি (hasFlags(nodep, T_NBA_UPDATE)) m_hasProcess = মিথ্যা;
        m_procp = nodep;
        যদি (m_hasProcess) nodep->setNeedProcess();
        iterateChildren(nodep);
    }
    শূন্য visit(AstFork* nodep) override {
        যদি (nodep->user1SetOnce()) ফেরত;
        UINFO(9, "control-visit " << nodep);
        v3Global.setUsesTiming();

        // Create a unique name for this fork
        nodep->name("__Vfork_" + std::to_string(++m_forkCnt));

        // TODO: Should process nodep->stmtsp() in case an earlier pass
        //       inserted something there, but as of today we don't need to.

        // Process and name each fork
        আকার_t idx = 0;  // Index for naming begins
        জন্য (AstBegin *itemp = nodep->forksp(), *nextp; itemp; itemp = nextp) {
            nextp = VN_AS(itemp->nextp(), Begin);
            iterate(itemp);
            // Note: Even if we do not find any awaits, we cannot simply inline
            // the process here, as new awaits could be added later.

            // Name the begin (later the name will be used for a new function)
            itemp->name(nodep->name() + "__" + std::to_string(idx++));
            যদি (itemp->needProcess()) {
                itemp->addStmtsp(
                    নতুন AstCStmt{itemp->fileline(), "vlProcess->state(VlProcess::FINISHED);"});
            }
        }
        যদি (!nodep->joinType().joinNone()) makeForkJoin(nodep);
    }

    //--------------------
    শূন্য visit(AstVar*) override {}  // Accelerate
    শূন্য visit(Ast_গ্রন্থি* nodep) override { iterateChildren(nodep); }

সর্বজনীন:
    // CONSTRUCTORS
    explicit TimingControlVisitor(AstNetlist* nodep)
        : m_netlistp{nodep} {
        iterate(nodep);

        // If there is no static #0 in the design, but an unknown delay was found,
        // and the user did not specify preference, then warn on all unknown delays
        // as we will be assuming they can be #0, which can cause performance degradation.
        জন্য (File_সারি* ধ্রুবক flp : m_unknownDelayFlps) {
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
    File_সারি* ধ্রুবক flp = valuep->fileline();
    যদি (ধ্রুবক AstConst* ধ্রুবক constp = VN_CAST(valuep, Const)) {
        যদি (constp->অঙ্ক().is1Step()) {
            // A step is one tick of the global time precision (IEEE 1800-2023 3.14.3)
            VL_DO_DANGLING(valuep->delete_বৃক্ষ(), valuep);
            AstConst* ধ্রুবক tickp = নতুন AstConst{flp, AstConst::Unsized64{}, 1};
            tickp->dtypeSetBitSized(64, VSigning::UNSIGNED);
            ফেরত tickp;
        }
        যদি (constp->isZero()) ফেরত valuep;
    }
    UASSERT_OBJ(!timeunit.isNone(), valuep, "timeunit must be set");
    ধ্রুবক দ্বিগুণ timescaleFactorD
        = std::pow(10.0, timeunit.powerOfTen() - v3Global.rootp()->timeprecision().powerOfTen());
    যদি (valuep->dtypep()->skipRefp()->isDouble()) {
        AstConst* ধ্রুবক tsfp = নতুন AstConst{flp, AstConst::RealDouble{}, timescaleFactorD};
        valuep = নতুন AstMulD{flp, valuep, tsfp};
        valuep = নতুন AstRToIRoundS{flp, valuep};
        valuep->dtypeSetBitSized(64, VSigning::UNSIGNED);
    } অন্যথা {
        valuep->dtypeSetBitSized(64, VSigning::UNSIGNED);
        ধ্রুবক uint64_t timescaleFactorU = static_cast<uint64_t>(timescaleFactorD);
        AstConst* ধ্রুবক tsfp = নতুন AstConst{flp, AstConst::Unsized64{}, timescaleFactorU};
        valuep = নতুন AstMul{flp, valuep, tsfp};
    }
    ফেরত V3Const::constifyEdit(valuep);  // Simplify
}

শূন্য V3Timing::timingAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        ধ্রুবক VNUser1InUse m_user1InUse;
        ধ্রুবক VNUser2InUse m_user2InUse;
        { TimingSuspendableVisitor{nodep}; }
        যদি (v3Global.usesTiming()) TimingControlVisitor{nodep};
    }
    V3Global::dump_Check_Global_বৃক্ষ("timing", 0, dump_বৃক্ষ_Either_Level() >= 3);
}
