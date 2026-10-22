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

#அந்தர்பாவ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#அந்தர்பாவ "V3Timing.h"

#அந்தர்பாவ "V3Const.h"
#அந்தர்பாவ "V3EmitV.h"
#அந்தர்பாவ "V3File.h"
#அந்தர்பாவ "V3Global.h"
#அந்தர்பாவ "V3Graph.h"
#அந்தர்பாவ "V3MemberMap.h"
#அந்தர்பாவ "V3SenExprBuilder.h"
#அந்தர்பாவ "V3SenTree.h"
#அந்தர்பாவ "V3SharedTmps.h"
#அந்தர்பாவ "V3Stats.h"
#அந்தர்பாவ "V3UniqueNames.h"

#அந்தர்பாவ <limits>
#அந்தர்பாவ <queue>

VL_DEFINE_DEBUG_FUNCTIONS;

// ######################################################################

எண்ணிக்கை முனை_Flag : uint8_t {
    T_SUSPENDEE = 1 << 0,  // Suspendable (due to dependence on another suspendable)
    T_SUSPENDER = 1 << 1,  // Suspendable (has timing control)
    T_ALLOCS_PROC = 1 << 2,  // Can allocate VlProcess
    T_FORCES_PROC = 1 << 3,  // Forces VlProcess allocation
    T_NEEDS_PROC = 1 << 4,  // Needs access to VlProcess if it's allocated
    T_HAS_PROC = 1 << 5,  // Has VlProcess argument in the signature
    T_NBA_UPDATE = 1 << 6,  // Fork branch of a pending NBA update, which is not a subprocess
};

எண்ணிக்கை ForkType : uint8_t {
    F_NONE = 0,  // Not under a fork
    F_MIGHT_SUSPEND = 1 << 0,  // Fork might suspend the execution of current process
    F_MIGHT_NEED_PROC = 1 << 1,  // Fork might need a process (any fork really)
};

எண்ணிக்கை PropagationType : uint8_t {
    P_CALL = 1,  // Propagation through call to a function/task/method
    P_FORK = 2,  // Propagation due to fork's behavior
    P_SIGNATURE = 3,  // Propagation required to maintain C++ function's signature requirements
};

// Add timing flag to a node
நிலையான வெறுமை addFlags(Ast_முனை* மாறிலி nodep, uint8_t flags) { nodep->user2Or(flags); }
// Check if a node has ALL of the expected flags set
நிலையான தருக்க hasFlags(Ast_முனை* மாறிலி nodep, uint8_t flags) { திரும்பு !(~nodep->user2() & flags); }

// ######################################################################
//  Detect nodes affected by timing and/or requiring a process

வகுப்பு TimingSuspendableVisitor final : பொது VNVisitor {
    // TYPES
    // Vertex of a dependency graph of suspendable nodes, e.g. if a node (process or task) is
    // suspendable, all its dependents should also be suspendable
    வகுப்பு DepVtx VL_NOT_FINAL : பொது V3GraphVertex {
        VL_RTTI_IMPL(DepVtx, V3GraphVertex)
        AstClass* மாறிலி m_classp;  // Class associated with a method
        Ast_முனை* மாறிலி m_nodep;  // AST node represented by this graph vertex

        // ACCESSORS
        string name() மாறிலி override {
            எனில் (m_classp) {
                எனில் (VN_IS(nodep(), CFunc)) {
                    திரும்பு cvtToHex(nodep()) + ' ' + classp()->name() + "::" + nodep()->name();
                }
            }
            திரும்பு cvtToHex(nodep()) + ' ' + nodep()->prettyTypeName();
        }
        File_வரி* fileline() மாறிலி override { திரும்பு nodep()->fileline(); }

    பொது:
        // CONSTRUCTORS
        DepVtx(V3Graph* graphp, Ast_முனை* nodep, AstClass* classp)
            : V3GraphVertex{graphp}
            , m_classp{classp}
            , m_nodep{nodep} {}
        ~DepVtx() override = இயல்பு;

        // ACCESSORS
        மெய்நிகர் Ast_முனை* nodep() மாறிலி VL_MT_STABLE { திரும்பு m_nodep; }
        மெய்நிகர் Ast_முனை* classp() மாறிலி VL_MT_STABLE { திரும்பு m_classp; }
    };

    வகுப்பு SuspendDepVtx final : பொது DepVtx {
        VL_RTTI_IMPL(SuspendDepVtx, DepVtx)
        string dotColor() மாறிலி override {
            எனில் (hasFlags(nodep(), T_SUSPENDER)) திரும்பு "red";
            எனில் (hasFlags(nodep(), T_SUSPENDEE)) திரும்பு "blue";
            திரும்பு "black";
        }

    பொது:
        SuspendDepVtx(V3Graph* graphp, Ast_முனை* nodep, AstClass* classp)
            : DepVtx{graphp, nodep, classp} {}
        ~SuspendDepVtx() override = இயல்பு;
    };

    வகுப்பு NeedsProcDepVtx final : பொது DepVtx {
        VL_RTTI_IMPL(NeedsProcDepVtx, DepVtx)
        string dotColor() மாறிலி override {
            எனில் (hasFlags(nodep(), T_HAS_PROC)) திரும்பு "blue";
            எனில் (hasFlags(nodep(), T_NEEDS_PROC)) திரும்பு "green";
            எனில் (hasFlags(nodep(), T_FORCES_PROC)) திரும்பு "red";
            திரும்பு "black";
        }

    பொது:
        NeedsProcDepVtx(V3Graph* graphp, Ast_முனை* nodep, AstClass* classp)
            : DepVtx{graphp, nodep, classp} {}
        ~NeedsProcDepVtx() override = இயல்பு;
    };

    // NODE STATE
    //  AstClass::user1() -> bool.  Class member cache has been refreshed.
    //  Ast{NodeProcedure,CFunc,Begin}::user2()  -> uint64_t.  Set to >= T_SUSP if
    //                                                         process/task suspendable
    //                                                         and to T_PROC if it
    //                                                         needs process metadata.
    //  Ast{NodeProcedure,CFunc,Begin}::user3()  -> DependencyVertex*.  Vertex in m_suspGraph
    //  Ast{NodeProcedure,CFunc,Begin}::user3()  -> DependencyVertex*.  Vertex in m_procGraph
    மாறிலி VNUser3InUse m_user3InUse;
    மாறிலி VNUser4InUse m_user4InUse;

    // STATE
    VMemberMap m_memberMap;  // Member names cached for fast lookup
    AstClass* m_classp = nullptr;  // Current class
    Ast_முனை* m_procp = nullptr;  // NodeProcedure/CFunc/Begin we're under
    uint8_t m_underFork = F_NONE;  // F_NONE or flags of a fork we are under
    V3Graph m_suspGraph;  // Dependency graph where a node is a dependency of another if it being
                          // suspendable makes the other node suspendable
    V3Graph m_procGraph;  // Dependency graph where a node is a dependency of another if it being
                          // suspendable makes the other node suspendable

    // METHODS
    // Get or create the dependency vertex for the given node
    DepVtx* getSuspendDepVtx(Ast_முனை* மாறிலி nodep) {
        AstClass* classp = nullptr;
        எனில் (AstCFunc* funcp = VN_CAST(nodep, CFunc)) {
            எனில் (funcp->scopep() && funcp->scopep()->modp()) {
                classp = VN_CAST(funcp->scopep()->modp(), Class);
            }
        }
        எனில் (!nodep->user3p()) nodep->user3p(புதிய SuspendDepVtx{&m_suspGraph, nodep, classp});
        திரும்பு nodep->user3u().to<SuspendDepVtx*>();
    }
    DepVtx* getNeedsProcDepVtx(Ast_முனை* மாறிலி nodep) {
        AstClass* classp = nullptr;
        எனில் (AstCFunc* funcp = VN_CAST(nodep, CFunc)) {
            எனில் (funcp->scopep() && funcp->scopep()->modp()) {
                classp = VN_CAST(funcp->scopep()->modp(), Class);
            }
        }
        எனில் (!nodep->user4p()) nodep->user4p(புதிய NeedsProcDepVtx{&m_procGraph, nodep, classp});
        திரும்பு nodep->user4u().to<NeedsProcDepVtx*>();
    }
    // Pass timing flag between nodes
    தருக்க passFlag(மாறிலி Ast_முனை* from, Ast_முனை* to, முனை_Flag flag) {
        எனில் ((from->user2() & flag) && !(to->user2() & flag)) {
            addFlags(to, flag);
            திரும்பு உண்மை;
        }
        திரும்பு பொய்;
    }
    // Propagate flag to all nodes that depend on the given one
    வெறுமை propagateFlags(DepVtx* மாறிலி vxp, முனை_Flag flag) {
        auto* மாறிலி parentp = vxp->nodep();
        க்கு (V3GraphEdge& edge : vxp->outEdges()) {
            auto* மாறிலி depVxp = static_cast<DepVtx*>(edge.உச்சி());
            Ast_முனை* மாறிலி depp = depVxp->nodep();
            எனில் (passFlag(parentp, depp, flag)) propagateFlags(depVxp, flag);
        }
    }
    template <typename T_Predicate>
    வெறுமை propagateFlagsIf(DepVtx* மாறிலி vxp, முனை_Flag flag, T_Predicate p) {
        auto* மாறிலி parentp = vxp->nodep();
        க்கு (V3GraphEdge& edge : vxp->outEdges()) {
            auto* மாறிலி depVxp = static_cast<DepVtx*>(edge.உச்சி());
            Ast_முனை* மாறிலி depp = depVxp->nodep();
            எனில் (p(&edge) && passFlag(parentp, depp, flag)) propagateFlagsIf(depVxp, flag, p);
        }
    }
    template <typename T_Predicate>
    வெறுமை propagateFlagsReversedIf(DepVtx* மாறிலி vxp, முனை_Flag flag, T_Predicate p) {
        auto* மாறிலி parentp = vxp->nodep();
        க்கு (V3GraphEdge& edge : vxp->inEdges()) {
            auto* மாறிலி depVxp = static_cast<DepVtx*>(edge.fromp());
            Ast_முனை* மாறிலி depp = depVxp->nodep();
            எனில் (p(&edge) && passFlag(parentp, depp, flag))
                propagateFlagsReversedIf(depVxp, flag, p);
        }
    }

    // VISITORS
    வெறுமை visit(AstClass* nodep) override {
        UASSERT_OBJ(!m_classp, nodep, "Class under class");
        VL_RESTORER(m_classp);
        m_classp = nodep;
        iterateChildren(nodep);
    }
    வெறுமை visit(Ast_முனை_Procedure* nodep) override {
        VL_RESTORER(m_procp);
        m_procp = nodep;
        getNeedsProcDepVtx(nodep);
        addFlags(nodep, T_ALLOCS_PROC);
        எனில் (VN_IS(nodep, Always)) {
            UINFO(9, "Always does " << (nodep->needProcess() ? "" : "NOT ") << "need process");
        }
        iterateChildren(nodep);
    }
    வெறுமை visit(AstDisableFork* nodep) override {
        visit(static_cast<Ast_முனை*>(nodep));
        addFlags(m_procp, T_FORCES_PROC | T_NEEDS_PROC);
    }
    வெறுமை visit(AstWaitFork* nodep) override {
        visit(static_cast<Ast_முனை*>(nodep));
        addFlags(m_procp, T_FORCES_PROC | T_NEEDS_PROC);
    }
    வெறுமை visit(AstWait* nodep) override {
        UINFO(9, "suspendable-visit " << nodep);
        Ast_முனை_கோவை* மாறிலி condp = V3Const::constifyEdit(nodep->condp());
        எனில் (AstConst* மாறிலி constp = VN_CAST(condp, Const)) {
            எனில் (!nodep->fileline()->warnIsOff(V3ErrorCode::WAITCONST)) {
                condp->v3warn(WAITCONST, "Wait statement condition is constant");
            }
            எனில் (!constp->isZero()) {
                // Remove AstWait before we track process as T_SUSPENDER
                எனில் (Ast_முனை* மாறிலி stmtsp = nodep->stmtsp()) {
                    stmtsp->unlinkFrBackWithNext();
                    nodep->replaceWith(stmtsp);
                } இல்லையெனில் {
                    nodep->unlinkFrBack();
                }
                VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
                திரும்பு;
            }
        }
        v3Global.setUsesTiming();
        எனில் (m_procp) addFlags(m_procp, T_SUSPENDEE | T_SUSPENDER | T_NEEDS_PROC);
        iterateChildren(nodep);
    }
    வெறுமை visit(AstCFunc* nodep) override {
        VL_RESTORER(m_procp);
        m_procp = nodep;
        iterateChildren(nodep);
        எனில் (nodep->needProcess()) addFlags(nodep, T_FORCES_PROC | T_NEEDS_PROC);
        DepVtx* மாறிலி sVxp = getSuspendDepVtx(nodep);
        DepVtx* மாறிலி pVxp = getNeedsProcDepVtx(nodep);
        எனில் (!m_classp) திரும்பு;

        // Go over overridden functions

        std::queue<AstClassExtends*> extends;
        எனில் (m_classp->extendsp()) extends.தள்ளு(m_classp->extendsp());

        போது (!extends.empty()) {
            AstClassExtends* ext_list = extends.front();
            extends.எடு();

            க்கு (AstClassExtends* cextp = ext_list; cextp;
                 cextp = VN_AS(cextp->nextp(), ClassExtends)) {
                // TODO: It is possible that a methods the same name in the base class is not
                // actually overridden by our method. If this causes a problem, traverse to
                // the root of the inheritance hierarchy and check if the original method is
                // virtual or not.
                எனில் (auto* மாறிலி overriddenp
                    = VN_CAST(m_memberMap.findMember(cextp->classp(), nodep->name()), CFunc)) {
                    // Suspendability and process affects typing, so they propagate both ways
                    DepVtx* மாறிலி overriddenSVxp = getSuspendDepVtx(overriddenp);
                    DepVtx* மாறிலி overriddenPVxp = getNeedsProcDepVtx(overriddenp);
                    புதிய V3GraphEdge{&m_suspGraph, sVxp, overriddenSVxp, P_SIGNATURE};
                    புதிய V3GraphEdge{&m_suspGraph, overriddenSVxp, sVxp, P_SIGNATURE};
                    புதிய V3GraphEdge{&m_procGraph, pVxp, overriddenPVxp, P_SIGNATURE};
                    புதிய V3GraphEdge{&m_procGraph, overriddenPVxp, pVxp, P_SIGNATURE};
                } இல்லையெனில் {
                    AstClassExtends* more_extends = cextp->classp()->extendsp();
                    எனில் (more_extends) extends.தள்ளு(more_extends);
                }
            }
        }
    }
    வெறுமை visit(Ast_முனை_CCall* nodep) override {
        AstCFunc* மாறிலி funcp = nodep->funcp();
        UASSERT_OBJ(funcp, nodep, "AstNodeCCall must have non-null funcp post-link");
        UASSERT_OBJ(m_procp, nodep, "AstNodeCCall must be inside a procedure/CFunc/Begin");

        UINFO(9, "V3Timing: Processing CCall to " << funcp->name() << " in dependency graph\n");
        புதிய V3GraphEdge{&m_suspGraph, getSuspendDepVtx(funcp), getSuspendDepVtx(m_procp), P_CALL};

        புதிய V3GraphEdge{&m_procGraph, getNeedsProcDepVtx(funcp), getNeedsProcDepVtx(m_procp),
                        P_CALL};

        iterateChildren(nodep);
    }
    வெறுமை visit(AstBegin* nodep) override {
        VL_RESTORER(m_procp);
        VL_RESTORER(m_underFork);

        எனில் (!m_underFork || (m_underFork & F_MIGHT_SUSPEND))
            புதிய V3GraphEdge{&m_suspGraph, getSuspendDepVtx(nodep), getSuspendDepVtx(m_procp),
                            m_underFork ? P_FORK : P_CALL};

        புதிய V3GraphEdge{&m_procGraph, getNeedsProcDepVtx(nodep), getNeedsProcDepVtx(m_procp),
                        P_CALL};

        எனில் (m_underFork) addFlags(nodep, T_NEEDS_PROC | T_ALLOCS_PROC);

        m_procp = nodep;
        m_underFork = 0;
        iterateChildren(nodep);
    }
    வெறுமை visit(AstFork* nodep) override {
        UINFO(9, "suspendable-visit " << nodep);
        VL_RESTORER(m_underFork);

        v3Global.setUsesTiming();  // Even if there are no event controls, we have to set this flag
                                   // so that transformForks() in V3SchedTiming gets called and
                                   // removes all forks and begins
        எனில் (nodep->isTimingControl() && m_procp) {
            addFlags(m_procp, T_SUSPENDEE | T_SUSPENDER);
            m_underFork |= F_MIGHT_SUSPEND;
        }
        m_underFork |= F_MIGHT_NEED_PROC;
        iterateChildren(nodep);
    }
    வெறுமை visit(AstAssignDly* nodep) override {
        எனில் (!VN_IS(m_procp, முனை_Procedure)) v3Global.setUsesTiming();
        visit(static_cast<Ast_முனை*>(nodep));
    }
    வெறுமை visit(AstFireEvent* nodep) override {
        எனில் (nodep->isDelayed()
            && (!VN_IS(m_procp, முனை_Procedure) || !VN_IS(nodep->operandp(), VarRef))) {
            v3Global.setUsesTiming();
        }
        visit(static_cast<Ast_முனை*>(nodep));
    }
    வெறுமை visit(AstAssignW* nodep) override {
        எனில் (nodep->timingControlp()) v3Global.setUsesTiming();
        // Containing process will not suspend, don't mark it
    }
    வெறுமை visit(Ast_முனை* nodep) override {
        எனில் (nodep->isTimingControl()) {
            v3Global.setUsesTiming();
            எனில் (m_procp) addFlags(m_procp, T_SUSPENDEE | T_SUSPENDER | T_NEEDS_PROC);
        }
        iterateChildren(nodep);
    }

    //--------------------
    வெறுமை visit(AstVar*) override {}  // Accelerate

பொது:
    // CONSTRUCTORS
    explicit TimingSuspendableVisitor(AstNetlist* nodep) {
        iterate(nodep);
        m_suspGraph.removeTransitiveEdges();
        m_procGraph.removeTransitiveEdges();
        // Propagate suspendability
        க்கு (V3GraphVertex& vtx : m_suspGraph.vertices()) {
            DepVtx& depVtx = static_cast<DepVtx&>(vtx);
            எனில் (hasFlags(depVtx.nodep(), T_SUSPENDEE)) propagateFlags(&depVtx, T_SUSPENDEE);
        }
        எனில் (dumpGraphLevel() >= 6) m_suspGraph.dumpDotFilePrefixed("timing_deps");

        // Propagate T_HAS_PROCESS
        க்கு (V3GraphVertex& vtx : m_procGraph.vertices()) {
            DepVtx& depVtx = static_cast<DepVtx&>(vtx);
            // Find processes that'll allocate VlProcess
            எனில் (hasFlags(depVtx.nodep(), T_FORCES_PROC)) {
                propagateFlagsIf(&depVtx, T_FORCES_PROC, [&](மாறிலி V3GraphEdge* e) -> தருக்க {
                    திரும்பு !hasFlags(static_cast<DepVtx*>(e->fromp())->nodep(), T_ALLOCS_PROC);
                });
            }
            // Mark nodes on paths between processes and statements that use VlProcess
            எனில் (hasFlags(depVtx.nodep(), T_NEEDS_PROC)) {
                propagateFlagsIf(&depVtx, T_NEEDS_PROC, [&](மாறிலி V3GraphEdge* e) -> தருக்க {
                    திரும்பு !hasFlags(static_cast<DepVtx*>(e->உச்சி())->nodep(), T_ALLOCS_PROC);
                });
            }
        }
        க்கு (V3GraphVertex& vtx : m_procGraph.vertices()) {
            DepVtx& depVtx = static_cast<DepVtx&>(vtx);
            // Mark nodes that will be emitted with a VlProcess argument
            எனில் (hasFlags(depVtx.nodep(), T_ALLOCS_PROC | T_FORCES_PROC)) {
                addFlags(depVtx.nodep(), T_HAS_PROC);
                propagateFlagsReversedIf(&depVtx, T_HAS_PROC, [&](மாறிலி V3GraphEdge* e) -> தருக்க {
                    திரும்பு hasFlags(static_cast<DepVtx*>(e->fromp())->nodep(), T_NEEDS_PROC);
                });
            }
        }
        எனில் (dumpGraphLevel() >= 6) m_procGraph.dumpDotFilePrefixed("proc_deps");
    }
    ~TimingSuspendableVisitor() override = இயல்பு;
};

// ######################################################################
//  Transform nodes affected by timing

வகுப்பு TimingControlVisitor final : பொது VNVisitor {
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
    AstNetlist* மாறிலி m_netlistp;  // Root node
    Ast_உச்சி_Scope* மாறிலி m_உச்சி_Scopep = m_netlistp->உச்சி_Scopep();  // The AstTopScope
    AstScope* மாறிலி m_scopeTopp = m_உச்சி_Scopep->scopep();  // Scope at the top
    AstClass* m_classp = nullptr;  // Current class
    AstScope* m_scopep = nullptr;  // Current scope
    AstActive* m_activep = nullptr;  // Current active
    Ast_முனை* m_procp = nullptr;  // NodeProcedure/CFunc/Begin we're under
    தருக்க m_hasProcess = பொய்;  // True if current scope has a VlProcess handle available
    முழு m_forkCnt = 0;  // Number of forks inside a module
    தருக்க m_underJumpBlock = பொய்;  // True if we are inside of a jump-block
    தருக்க m_underProcedure = பொய்;  // True if we are under an always or initial
    தருக்க m_underIfaceCFunc = பொய்;  // True if we are under a CFunc owned by an interface scope
    தருக்க m_hasStaticZeroDelay = பொய்;  // True if we have a static #0 delay
    std::vector<File_வரி*> m_unknownDelayFlps;  // Locations of AstDelay with non-constant value

    // Unique names
    V3UniqueNames m_dlyforkNames{"__Vdlyfork"};  // Names for temp AssignW vars
    V3UniqueNames m_intra_மதிப்பு_Names{"__Vintraval"};  // Intra assign delay value var names
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
    Ast_Sen_மரம்* m_delaySensesp = nullptr;  // Domain to trigger if a delayed coroutine is resumed
    Ast_Sen_மரம்* m_dynamicSensesp = nullptr;  // Domain to trigger if a dynamic trigger is set

    // Other
    Sen_மரம்_Finder m_finder{m_netlistp};  // Sentree finder and uniquifier
    Sen_கோவை_Builder* m_sen_கோவை_Builderp = nullptr;  // Sens expression builder for current m_scope

    // Stats
    size_t m_statZeroDelays = 0;  // Number of statically known #0 delays
    size_t m_statConstDelays = 0;  // Number of statically known #const (non-zero) delays
    size_t m_statVariableDelays = 0;  // Number of delays with value unknown at compile time

    // METHODS
    // Transform an assignment with an intra timing control into a timing control with the
    // assignment under it
    Ast_முனை* factorOutTimingControl(Ast_முனை_Assign* nodep) மாறிலி {
        Ast_முனை* stmtp = nodep;
        Ast_முனை* மாறிலி controlp = nodep->timingControlp();
        எனில் (AstDelay* மாறிலி delayp = VN_CAST(controlp, Delay)) {
            stmtp->replaceWith(delayp->unlinkFrBack());
            delayp->addStmtsp(stmtp);
            stmtp = delayp;
        } இல்லையெனில் எனில் (Ast_Sen_மரம்* மாறிலி sentreep = VN_CAST(controlp, Sen_மரம்)) {
            AstEventControl* மாறிலி eventControlp
                = புதிய AstEventControl{sentreep->fileline(), sentreep->unlinkFrBack(), nullptr};
            stmtp->replaceWith(eventControlp);
            eventControlp->addStmtsp(stmtp);
            stmtp = eventControlp;
        } இல்லையெனில் எனில் (AstBegin* மாறிலி beginp = VN_CAST(controlp, Begin)) {
            // Begin from V3AssertPre
            stmtp->replaceWith(beginp->unlinkFrBack());
            beginp->addStmtsp(stmtp);
            stmtp = beginp;
        }
        திரும்பு stmtp == nodep ? nullptr : stmtp;
    }
    நிலையான தருக்க staticallyNonZeroDelay(மாறிலி Ast_முனை_கோவை* valuep) {
        எனில் (மாறிலி AstConst* மாறிலி constp = VN_CAST(valuep, Const)) திரும்பு !constp->isZero();
        எனில் (மாறிலி AstCond* மாறிலி condp = VN_CAST(valuep, Cond)) {
            திரும்பு staticallyNonZeroDelay(condp->thenp())
                   && staticallyNonZeroDelay(condp->elsep());
        }
        எனில் (மாறிலி AstMul* மாறிலி mulp = VN_CAST(valuep, Mul)) {
            மாறிலி AstConst* மாறிலி lhsConstp = VN_CAST(mulp->lhsp(), Const);
            மாறிலி AstConst* மாறிலி rhsConstp = VN_CAST(mulp->rhsp(), Const);
            எனில் (lhsConstp && !lhsConstp->isZero()) திரும்பு staticallyNonZeroDelay(mulp->rhsp());
            எனில் (rhsConstp && !rhsConstp->isZero()) திரும்பு staticallyNonZeroDelay(mulp->lhsp());
        }
        திரும்பு பொய்;
    }
    // Creates the global delay scheduler variable
    AstVarScope* getCreateDelayScheduler() {
        எனில் (m_delaySchedp) திரும்பு m_delaySchedp;
        auto* மாறிலி dlySchedDtp = புதிய AstBasicDType{
            m_scopeTopp->fileline(), VBasicDTypeKwd::DELAY_SCHEDULER, VSigning::UNSIGNED};
        m_netlistp->typeTablep()->addTypesp(dlySchedDtp);
        m_delaySchedp = m_உச்சி_Scopep->createTemp("__VdlySched", dlySchedDtp);
        // Delay scheduler has to be accessible from top
        m_delaySchedp->varp()->sigPublic(உண்மை);
        m_netlistp->delaySchedulerp(m_delaySchedp->varp());
        திரும்பு m_delaySchedp;
    }
    // Creates the delay sentree
    Ast_Sen_மரம்* get_Create_Delay_Sen_மரம்() {
        எனில் (m_delaySensesp) திரும்பு m_delaySensesp;
        File_வரி* மாறிலி flp = m_scopeTopp->fileline();
        auto* மாறிலி awaitingCurrentTimep
            = புதிய AstCMethodHard{flp, புதிய AstVarRef{flp, getCreateDelayScheduler(), VAccess::READ},
                                 VCMethod::SCHED_AWAITING_CURRENT_TIME};
        awaitingCurrentTimep->dtypeSetBit();
        m_delaySensesp
            = புதிய Ast_Sen_மரம்{flp, புதிய AstSenItem{flp, VEdgeType::ET_TRUE, awaitingCurrentTimep}};
        m_netlistp->உச்சி_Scopep()->addSenTreesp(m_delaySensesp);
        திரும்பு m_delaySensesp;
    }
    // Creates the global dynamic trigger scheduler variable
    AstVarScope* getCreateDynamicTriggerScheduler() {
        எனில் (m_dynamicSchedp) திரும்பு m_dynamicSchedp;
        auto* மாறிலி dynSchedDtp
            = புதிய AstBasicDType{m_scopeTopp->fileline(), VBasicDTypeKwd::DYNAMIC_TRIGGER_SCHEDULER,
                                VSigning::UNSIGNED};
        m_netlistp->typeTablep()->addTypesp(dynSchedDtp);
        m_dynamicSchedp = m_உச்சி_Scopep->createTemp("__VdynSched", dynSchedDtp);
        திரும்பு m_dynamicSchedp;
    }
    // Creates the dynamic trigger sentree
    Ast_Sen_மரம்* get_Create_Dynamic_Trigger_Sen_மரம்() {
        எனில் (m_dynamicSensesp) திரும்பு m_dynamicSensesp;
        File_வரி* மாறிலி flp = m_scopeTopp->fileline();
        auto* மாறிலி awaitingCurrentTimep = புதிய AstCMethodHard{
            flp, புதிய AstVarRef{flp, getCreateDynamicTriggerScheduler(), VAccess::READ},
            VCMethod::SCHED_EVALUATE};
        awaitingCurrentTimep->dtypeSetBit();
        m_dynamicSensesp
            = புதிய Ast_Sen_மரம்{flp, புதிய AstSenItem{flp, VEdgeType::ET_TRUE, awaitingCurrentTimep}};
        m_netlistp->உச்சி_Scopep()->addSenTreesp(m_dynamicSensesp);
        திரும்பு m_dynamicSensesp;
    }
    // Creates the event variable to trigger in NBA region
    AstEventControl* createNbaEventControl(File_வரி* flp) {
        எனில் (!m_netlistp->nbaEventp()) {
            auto* மாறிலி nbaEventDtp = புதிய AstBasicDType{m_scopeTopp->fileline(),
                                                        VBasicDTypeKwd::EVENT, VSigning::UNSIGNED};
            m_netlistp->typeTablep()->addTypesp(nbaEventDtp);
            m_netlistp->nbaEventp(m_உச்சி_Scopep->createTemp("__VnbaEvent", nbaEventDtp));
            v3Global.setHasEvents();
        }
        திரும்பு புதிய AstEventControl{
            flp,
            புதிய Ast_Sen_மரம்{
                flp, புதிய AstSenItem{flp, VEdgeType::ET_EVENT,
                                    புதிய AstVarRef{flp, m_netlistp->nbaEventp(), VAccess::READ}}},
            nullptr};
    }
    // Creates the variable that, if set, causes the NBA event to be triggered
    AstAssign* createNbaEventTriggerAssignment(File_வரி* flp) {
        எனில் (!m_netlistp->nbaEventTriggerp()) {
            m_netlistp->nbaEventTriggerp(m_உச்சி_Scopep->createTemp("__VnbaEventTrigger", 1));
        }
        திரும்பு புதிய AstAssign{flp,
                             புதிய AstVarRef{flp, m_netlistp->nbaEventTriggerp(), VAccess::WRITE},
                             புதிய AstConst{flp, AstConst::BitTrue{}}};
    }
    // Returns true if we are under a class or interface function, or the tree references locals.
    // These are cases where static, globally-evaluated triggers are not suitable.
    தருக்க needDynamicTrigger(Ast_முனை* மாறிலி nodep) மாறிலி {
        திரும்பு m_classp || m_underIfaceCFunc || nodep->exists([](Ast_முனை* மாறிலி nodep) {
            எனில் (Ast_முனை_Var_Ref* varp = VN_CAST(nodep, முனை_Var_Ref)) {
                திரும்பு varp->varp()->isFuncLocal();
            }
            திரும்பு !nodep->isPure();
        });
    }
    // Creates a trigger scheduler variable
    AstVarScope* getCreateTriggerSchedulerp(Ast_Sen_மரம்* மாறிலி sentreep) {
        எனில் (!sentreep->user1p()) {
            எனில் (!m_trigSchedDtp) {
                m_trigSchedDtp
                    = புதிய AstBasicDType{m_scopeTopp->fileline(), VBasicDTypeKwd::TRIGGER_SCHEDULER,
                                        VSigning::UNSIGNED};
                m_netlistp->typeTablep()->addTypesp(m_trigSchedDtp);
            }
            AstVarScope* மாறிலி trigSchedp
                = m_உச்சி_Scopep->createTemp(m_trigSchedNames.get(sentreep), m_trigSchedDtp);
            sentreep->user1p(trigSchedp);
        }
        திரும்பு VN_AS(sentreep->user1p(), VarScope);
    }
    // Creates a string describing the sentree
    AstCExpr* createEventDescription(Ast_Sen_மரம்* மாறிலி sentreep) மாறிலி {
        எனில் (!sentreep->user2p()) {
            std::stringstream ss;
            V3EmitV::verilog_For_மரம்(sentreep, ss);
            // possibly a multiline string
            // Names were hashed by V3Name, show the user the original text
            std::string comment = VName::dehash(ss.str());
            std::replace(comment.begin(), comment.end(), '\n', ' ');
            // Escape quotes, e.g. from string-indexed event array selects
            comment = '"' + V3OutFormatter::quoteNameControls(comment) + '"';
            AstCExpr* மாறிலி commentp = புதிய AstCExpr{sentreep->fileline(), comment};
            commentp->dtypeSetString();
            sentreep->user2p(commentp);
            திரும்பு commentp;
        }
        திரும்பு VN_AS(sentreep->user2p(), CExpr)->clone_மரம்(பொய்);
    }
    // Adds debug info to a hardcoded method call
    வெறுமை addDebugInfo(AstCMethodHard* மாறிலி methodp) மாறிலி {
        File_வரி* மாறிலி flp = methodp->fileline();
        மாறிலி தருக்க protectIds = v3Global.opt.protectIds();
        AstCExpr* மாறிலி ap
            = புதிய AstCExpr{flp, protectIds ? "VL_UNKNOWN" : '"' + flp->filenameEsc() + '"'};
        ap->dtypeSetString();
        methodp->addPinsp(ap);
        AstCExpr* மாறிலி bp = புதிய AstCExpr{flp, protectIds ? "0" : cvtToStr(flp->lineno())};
        bp->dtypeSetString();
        methodp->addPinsp(bp);
    }
    // Adds debug info to a trigSched.trigger() call
    வெறுமை addEventDebugInfo(AstCMethodHard* மாறிலி methodp, Ast_Sen_மரம்* மாறிலி sentreep) மாறிலி {
        எனில் (v3Global.opt.protectIds()) {
            File_வரி* மாறிலி flp = sentreep->fileline();
            AstCExpr* மாறிலி descp = புதிய AstCExpr{flp, "VL_UNKNOWN"};
            descp->dtypeSetString();
            methodp->addPinsp(descp);
            addDebugInfo(methodp);
            திரும்பு;
        }
        methodp->addPinsp(createEventDescription(sentreep));
        addDebugInfo(methodp);
    }
    // Adds process pointer to a hardcoded method call
    வெறுமை addProcessInfo(AstCMethodHard* மாறிலி methodp) மாறிலி {
        File_வரி* மாறிலி flp = methodp->fileline();
        AstCExpr* மாறிலி ap
            = புதிய AstCExpr{flp, (m_procp && m_hasProcess) ? "vlProcess" : "nullptr"};
        methodp->addPinsp(ap);
    }
    // Creates the fork handle type and returns it
    AstBasicDType* getCreateForkSyncDTypep() {
        எனில் (m_forkDtp) திரும்பு m_forkDtp;
        m_forkDtp = புதிய AstBasicDType{m_scopeTopp->fileline(), VBasicDTypeKwd::FORK_SYNC,
                                      VSigning::UNSIGNED};
        m_netlistp->typeTablep()->addTypesp(m_forkDtp);
        திரும்பு m_forkDtp;
    }
    // Move `nodep` into `AstCLocalScope`.
    AstCLocalScope* addCLocalScope(File_வரி* மாறிலி flp, Ast_முனை* மாறிலி nodep) மாறிலி {
        எனில் (!nodep) திரும்பு nullptr;
        VNRelinker handle;
        nodep->unlinkFrBack(&handle);
        AstCLocalScope* மாறிலி cscopep = புதிய AstCLocalScope{flp, nodep};
        handle.relink(cscopep);
        திரும்பு cscopep;
    }
    // Create a temp variable and optionally put it before the specified node (mark local if so)
    AstVarScope* createTemp(File_வரி* மாறிலி flp, மாறிலி std::string& name,
                            Ast_முனை_DType* மாறிலி dtypep, Ast_முனை* மாறிலி செருகு_Beforep = nullptr) {
        AstVar* varp;
        எனில் (செருகு_Beforep) {
            varp = புதிய AstVar{flp, VVarType::BLOCKTEMP, name, dtypep};
            varp->funcLocal(உண்மை);
            செருகு_Beforep->addHereThisAsNext(varp);
        } இல்லையெனில் {
            varp = புதிய AstVar{flp, VVarType::MODULETEMP, name, dtypep};
            m_scopep->modp()->addStmtsp(varp);
        }
        AstVarScope* மாறிலி vscp = புதிய AstVarScope{flp, m_scopep, varp};
        m_scopep->addVarsp(vscp);
        திரும்பு vscp;
    }
    // Add a done() call on the fork sync
    வெறுமை addForkDone(AstBegin* மாறிலி beginp, AstVarScope* மாறிலி forkVscp) மாறிலி {
        File_வரி* மாறிலி flp = beginp->fileline();
        auto* மாறிலி donep = புதிய AstCMethodHard{
            beginp->fileline(), புதிய AstVarRef{flp, forkVscp, VAccess::WRITE}, VCMethod::FORK_DONE};
        donep->dtypeSetVoid();
        addDebugInfo(donep);
        beginp->addStmtsp(donep->makeStmt());
    }
    நிலையான தருக்க has_Disable_Queue_தள்ளு_Self_Prefix(AstBegin* மாறிலி beginp) {
        // LinkJump prepends disable-by-name registration as:
        //   __VprocessQueue_*.push_back(std::process::self())
        // V3LiftExpr lifts std::process::self() into an assignment to a temporary, then V3Task
        // lowers that assignment's function call into a leading comment and AstStmtExpr. Thus the
        // push_back is no longer necessarily the first statement. Scan across this generated
        // prefix for it, stopping at the fork-start sentinel or the branch body. Registrations for
        // nested forks live in sub-blocks and so are not in this statement list.
        க்கு (Ast_முனை* stmtp = beginp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            எனில் (VN_IS(stmtp, Comment)) தொடர்;
            Ast_Stmt_கோவை* மாறிலி stmtExprp = VN_CAST(stmtp, Stmt_கோவை);
            எனில் (!stmtExprp) நிறுத்து;
            எனில் (stmtExprp->is_Disable_Queue_தள்ளு_Self_Stmt()) திரும்பு உண்மை;
        }
        திரும்பு பொய்;
    }
    // Register a callback so killing a process-backed fork branch decrements the join counter
    வெறுமை addForkOnKill(AstBegin* மாறிலி beginp, AstVarScope* மாறிலி forkVscp) மாறிலி {
        எனில் (!beginp->needProcess() && !has_Disable_Queue_தள்ளு_Self_Prefix(beginp)) திரும்பு;
        File_வரி* மாறிலி flp = beginp->fileline();
        AstCMethodHard* மாறிலி onKillp = புதிய AstCMethodHard{
            flp, புதிய AstVarRef{flp, forkVscp, VAccess::WRITE}, VCMethod::FORK_ON_KILL};
        onKillp->dtypeSetVoid();
        AstCExpr* மாறிலி processp = புதிய AstCExpr{flp, "vlProcess"};
        processp
            ->dtypeSetVoid();  // Opaque process reference; type is irrelevant for hardcoded emit
        onKillp->addPinsp(processp);
        Ast_முனை_Stmt* மாறிலி stmtp = onKillp->makeStmt();
        beginp->prependStmtsp(stmtp);
    }
    // Handle the 'join' part of a fork..join
    வெறுமை makeForkJoin(AstFork* மாறிலி forkp) {
        // Create a fork sync var
        File_வரி* மாறிலி flp = forkp->fileline();
        // Insert the sync var directly before the fork
        Ast_முனை* மாறிலி செருகு_Beforep = forkp;
        // Make sure all references to the fork var are contained within a single scope to prevent
        // moving statements accessing it in case of large functions splitting.
        addCLocalScope(flp, செருகு_Beforep);
        AstVarScope* forkVscp
            = createTemp(flp, forkp->name() + "__sync", getCreateForkSyncDTypep(), செருகு_Beforep);
        unsigned join_மொத்தம் = 0;  // Needed for join counter
        // Add a <fork sync>.done() to each begin
        க்கு (Ast_முனை* beginp = forkp->forksp(); beginp; beginp = beginp->nextp()) {
            addForkOnKill(VN_AS(beginp, Begin), forkVscp);
            addForkDone(VN_AS(beginp, Begin), forkVscp);
            join_மொத்தம்++;
        }
        எனில் (forkp->joinType().joinAny()) join_மொத்தம் = 1;
        // Set the join counter
        auto* மாறிலி initp = புதிய AstCMethodHard{flp, புதிய AstVarRef{flp, forkVscp, VAccess::WRITE},
                                               VCMethod::FORK_INIT, புதிய AstConst{flp, join_மொத்தம்}};
        initp->dtypeSetVoid();
        addProcessInfo(initp);
        forkp->addHereThisAsNext(initp->makeStmt());
        // Await the join at the end
        auto* மாறிலி joinp = புதிய AstCMethodHard{flp, புதிய AstVarRef{flp, forkVscp, VAccess::WRITE},
                                               VCMethod::FORK_JOIN};
        joinp->dtypeSetVoid();
        addProcessInfo(joinp);
        addDebugInfo(joinp);
        forkp->addNextHere(புதிய AstCAwait{flp, joinp});
    }

    // `procp` shall be a NodeProcedure/CFunc/Begin and within it vars from `varsp` will be placed.
    // `varsp` vector of vars which shall be localized.
    நிலையான வெறுமை localizeVars(Ast_முனை* மாறிலி procp, மாறிலி std::vector<AstVar*>& varsp) {
        UASSERT(procp, "procp is nullptr");
        AstVar* newVarsp = nullptr;
        க்கு (AstVar* மாறிலி varp : varsp) {
            varp->funcLocal(உண்மை);
            newVarsp = Ast_முனை::addNextNull(newVarsp, varp->unlinkFrBack());
        }
        எனில் (Ast_முனை_Procedure* மாறிலி முனை_Procp = VN_CAST(procp, முனை_Procedure)) {
            முனை_Procp->prependStmtsp(newVarsp);
        } இல்லையெனில் எனில் (AstCFunc* மாறிலி cfuncp = VN_CAST(procp, CFunc)) {
            cfuncp->prependVarsp(newVarsp);
        } இல்லையெனில் எனில் (AstBegin* மாறிலி beginp = VN_CAST(procp, Begin)) {
            beginp->prependStmtsp(newVarsp);
        } இல்லையெனில் {
            procp->v3fatalSrc(
                procp->prettyNameQ()
                << " is not of an expected type NodeProcedure/CFunc/Begin instead it is: "
                << procp->prettyTypeName());
        }
    }

    Ast_முனை_கோவை* delayToIntegral(AstDelay* nodep, Ast_முனை_கோவை* valuep) {
        மாறிலி AstConst* மாறிலி constp = VN_CAST(valuep, Const);
        // The fork start sentinel is already in ticks
        எனில் (constp && constp->toUQuad() == std::numeric_limits<uint64_t>::max()) திரும்பு valuep;
        திரும்பு V3Timing::delayToTicks(valuep, nodep->timeunit());
    }

    // VISITORS
    வெறுமை visit(Ast_முனை_Module* nodep) override {
        UASSERT_OBJ(!m_classp, nodep, "Module or class under class");
        VL_RESTORER(m_classp);
        m_classp = VN_CAST(nodep, Class);
        VL_RESTORER(m_forkCnt);
        m_forkCnt = 0;
        iterateChildren(nodep);
    }
    வெறுமை visit(AstScope* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        Sen_கோவை_Builder sen_கோவை_Builder{m_scopep};
        {  // Restore m_senExprBuilderp before destroying senExprBuilder
            VL_RESTORER(m_sen_கோவை_Builderp);
            m_sen_கோவை_Builderp = &sen_கோவை_Builder;
            iterateChildren(nodep);
        }
    }
    வெறுமை visit(AstActive* nodep) override {
        VL_RESTORER(m_activep);
        m_activep = nodep;
        iterateChildren(nodep);
    }
    வெறுமை visit(Ast_முனை_Procedure* nodep) override {
        VL_RESTORER(m_procp);
        VL_RESTORER(m_hasProcess);
        m_procp = nodep;
        m_hasProcess = hasFlags(nodep, T_HAS_PROC);
        VL_RESTORER(m_underProcedure);
        m_underProcedure = உண்மை;
        iterateChildren(nodep);
        எனில் (hasFlags(nodep, T_SUSPENDEE)) nodep->setSuspendable();
        எனில் (hasFlags(nodep, T_HAS_PROC)) nodep->setNeedProcess();
    }
    வெறுமை visit(AstInitial* nodep) override {
        visit(static_cast<Ast_முனை_Procedure*>(nodep));
        எனில் (nodep->needProcess() && !nodep->user1SetOnce()) {
            nodep->addStmtsp(
                புதிய AstCStmt{nodep->fileline(), "vlProcess->state(VlProcess::FINISHED);"});
        }
    }
    வெறுமை visit(AstJumpBlock* nodep) override {
        VL_RESTORER(m_underJumpBlock);
        m_underJumpBlock = உண்மை;
        visit(static_cast<Ast_முனை_Stmt*>(nodep));
    }
    வெறுமை visit(AstAlways* nodep) override {
        எனில் (nodep->user1SetOnce()) திரும்பு;
        VL_RESTORER(m_procp);
        VL_RESTORER(m_hasProcess);
        m_procp = nodep;
        m_hasProcess = hasFlags(nodep, T_HAS_PROC);
        VL_RESTORER(m_underProcedure);
        m_underProcedure = உண்மை;
        // Workaround for killing `always` processes (doing that is pretty much UB)
        // TODO: Disallow killing `always` at runtime (throw an error)
        // Skip for combinational blocks; if the body has timing controls
        // iterateChildren will add T_SUSPENDEE, otherwise it would spin.
        எனில் (hasFlags(nodep, T_HAS_PROC)
            && !(m_activep && m_activep->sentreep() && m_activep->sentreep()->hasCombo()))
            addFlags(nodep, T_SUSPENDEE);

        iterateChildren(nodep);
        எனில் (hasFlags(nodep, T_HAS_PROC)) nodep->setNeedProcess();
        எனில் (!hasFlags(nodep, T_SUSPENDEE)) திரும்பு;
        nodep->setSuspendable();
        File_வரி* மாறிலி flp = nodep->fileline();
        Ast_Sen_மரம்* மாறிலி sentreep = m_activep->sentreep();
        எனில் (sentreep->hasClocked()) {
            Ast_முனை* மாறிலி bodysp = nodep->stmtsp()->unlinkFrBackWithNext();
            auto* மாறிலி controlp = புதிய AstEventControl{flp, sentreep->clone_மரம்(பொய்), bodysp};
            nodep->addStmtsp(controlp);
            iterate(controlp);
        }
        // Note: The 'while (true)' outer loop will be added in V3Sched
        auto* மாறிலி activep = புதிய AstActive{
            flp, "", புதிய Ast_Sen_மரம்{flp, புதிய AstSenItem{flp, AstSenItem::Initial{}}}};
        activep->sen_மரம்_Storep(activep->sentreep());
        activep->addStmtsp(nodep->unlinkFrBack());
        m_activep->addNextHere(activep);
    }
    வெறுமை visit(AstCFunc* nodep) override {
        VL_RESTORER(m_procp);
        VL_RESTORER(m_hasProcess);
        VL_RESTORER(m_underIfaceCFunc);
        m_procp = nodep;
        m_hasProcess = hasFlags(nodep, T_HAS_PROC);
        m_underIfaceCFunc = VN_IS(m_scopep->modp(), Iface);
        iterateChildren(nodep);
        எனில் (hasFlags(nodep, T_HAS_PROC)) nodep->setNeedProcess();
        எனில் (!(hasFlags(nodep, T_SUSPENDEE))) திரும்பு;

        nodep->rtnType("VlCoroutine");
        // If in a class, create a shared pointer to 'this'
        எனில் (m_classp) {
            AstCStmt* மாறிலி cstmtp = புதிய AstCStmt{nodep->fileline(), "VL_KEEP_THIS;"};
            nodep->prependStmtsp(cstmtp);
        }
        எனில் (nodep->dpiExportImpl()) {
            nodep->exists([](AstCAwait* மாறிலி awaitp) -> தருக்க {
                // A DPI-exported coroutine won't be able to block the calling code
                // Error on the await node; fall back to the function node
                awaitp->v3warn(E_UNSUPPORTED,
                               "Unsupported: Timing controls inside DPI-exported tasks");
                திரும்பு உண்மை;
            });
        }
    }
    வெறுமை visit(Ast_முனை_CCall* nodep) override {
        AstCFunc* மாறிலி funcp = nodep->funcp();

        // Skip automatic covergroup sampling calls
        எனில் (funcp->isCovergroupSample()) {
            iterateChildren(nodep);
            திரும்பு;
        }

        எனில் (funcp->needProcess()) m_hasProcess = உண்மை;
        எனில் (hasFlags(funcp, T_SUSPENDEE) && !nodep->user1SetOnce()) {  // If suspendable
            // Calls to suspendables are always void return type, hence parent must be StmtExpr
            Ast_Stmt_கோவை* மாறிலி stmtp = VN_AS(nodep->backp(), Stmt_கோவை);
            stmtp->replaceWith(புதிய AstCAwait{nodep->fileline(), nodep->unlinkFrBack()});
            VL_DO_DANGLING(தள்ளு_Deletep(stmtp), stmtp);
        }
        iterateChildren(nodep);
    }
    வெறுமை visit(AstDelay* nodep) override {
        UASSERT_OBJ(!nodep->isCycleDelay(), nodep,
                    "Cycle delays should have been handled in V3AssertPre");
        File_வரி* மாறிலி flp = nodep->fileline();

        Ast_முனை_கோவை* valuep = delayToIntegral(nodep, nodep->lhsp()->unlinkFrBack());

        // Statistics
        எனில் (valuep->isZero()) {
            ++m_statZeroDelays;
        } இல்லையெனில் எனில் (VN_IS(valuep, Const)) {
            ++m_statConstDelays;
        } இல்லையெனில் {
            ++m_statVariableDelays;
        }

        // Decide scheduling support for #0
        எனில் (v3Global.opt.schedZeroDelay().isSetTrue()) {
            // User said to schedule for #0 support, nothing else to do
            v3Global.setUsesZeroDelay();
        } இல்லையெனில் எனில் (v3Global.opt.schedZeroDelay().isSetFalse()) {
            // User said to schedule without #0 support. Still warn if a static #0 delay exists
            எனில் (valuep->isZero()) {
                nodep->v3warn(
                    ZERODLY,
                    "Static #0 delay exists, but '--no-sched-zero-delay' was given.\n"
                        << nodep->warnMore()  //
                        << "... Can proceed, but this will fail at runtime if executed.");
            }
        } இல்லையெனில் {
            // User did not express preference, decide based on presence of delays
            எனில் (valuep->isZero()) {
                // Statically known #0 delay exists, schedule for #0 support
                v3Global.setUsesZeroDelay();
                m_hasStaticZeroDelay = உண்மை;
                // Don't warn on variable delays, as no point
                m_unknownDelayFlps.clear();
            } இல்லையெனில் எனில் (staticallyNonZeroDelay(valuep)) {
                // Delay is dynamic, but every statically known outcome is non-zero.
                // So we don't need #0 delay support and there should be no warning.
            } இல்லையெனில் எனில் (!VN_IS(valuep, Const)) {
                // Delay is not known at compiile time. Conservatively schedule for #0 support,
                // but warn if no static #0 delays used as performance might be improved
                v3Global.setUsesZeroDelay();
                எனில் (!m_hasStaticZeroDelay) m_unknownDelayFlps.தள்ளு_back(nodep->fileline());
            }
        }

        // Replace self with a 'co_await dlySched.delay(<valuep>)'
        AstCMethodHard* மாறிலி delayMethodp = புதிய AstCMethodHard{
            flp, புதிய AstVarRef{flp, getCreateDelayScheduler(), VAccess::WRITE},
            VCMethod::SCHED_DELAY, valuep};
        delayMethodp->dtypeSetVoid();
        addProcessInfo(delayMethodp);
        addDebugInfo(delayMethodp);
        // Create the co_await
        Ast_முனை* மாறிலி awaitp = புதிய AstCAwait{flp, delayMethodp, get_Create_Delay_Sen_மரம்()};
        // Relink child statements after the co_await
        எனில் (Ast_முனை* மாறிலி stmtsp = nodep->stmtsp()) {
            awaitp->addNext(stmtsp->unlinkFrBackWithNext());
        }
        nodep->replaceWith(awaitp);
        VL_DO_DANGLING(nodep->delete_மரம்(), nodep);
    }
    வெறுமை visit(AstEventControl* nodep) override {
        // Do not allow waiting on local named events, as they get enqueued for clearing, but can
        // go out of scope before that happens
        எனில் (!nodep->sentreep()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Event control with implicit sensitivity (@*)"
                          " in this context; use an explicit sensitivity list instead");
            VL_DO_DANGLING(தள்ளு_Deletep(nodep->unlinkFrBack()), nodep);
            திரும்பு;
        }
        File_வரி* மாறிலி flp = nodep->fileline();
        // Relink child statements after the event control
        எனில் (nodep->stmtsp()) nodep->addNextHere(nodep->stmtsp()->unlinkFrBackWithNext());
        எனில் (needDynamicTrigger(nodep->sentreep())) {
            // Create the trigger variable and init it with 0
            AstVarScope* மாறிலி trigvscp
                = createTemp(flp, m_dynTrigNames.get(nodep), nodep->findBitDType(), nodep);
            auto* மாறிலி initp = புதிய AstAssign{flp, புதிய AstVarRef{flp, trigvscp, VAccess::WRITE},
                                              புதிய AstConst{flp, AstConst::BitFalse{}}};
            // Await the eval step with the dynamic trigger scheduler. First, create the method
            // call
            auto* மாறிலி evalMethodp = புதிய AstCMethodHard{
                flp, புதிய AstVarRef{flp, getCreateDynamicTriggerScheduler(), VAccess::WRITE},
                VCMethod::SCHED_EVALUATION};
            evalMethodp->dtypeSetVoid();
            addProcessInfo(evalMethodp);
            auto* மாறிலி sentreep = nodep->sentreep();
            addEventDebugInfo(evalMethodp, sentreep);
            // Create the co_await
            AstCAwait* மாறிலி awaitEvalp
                = புதிய AstCAwait{flp, evalMethodp, get_Create_Dynamic_Trigger_Sen_மரம்()};
            // Construct the sen expression for this sentree
            UASSERT_OBJ(m_sen_கோவை_Builderp, nodep, "No SenExprBuilder for this scope");
            auto* மாறிலி assignp = புதிய AstAssign{flp, புதிய AstVarRef{flp, trigvscp, VAccess::WRITE},
                                                m_sen_கோவை_Builderp->build(sentreep).first};
            // Get the SenExprBuilder results
            மாறிலி Sen_கோவை_Builder::Results senResults = m_sen_கோவை_Builderp->getAndClearResults();
            localizeVars(m_procp, senResults.m_vars);
            // If post updates are destructive (e.g. clearFired on events), perform a
            // conservative pre-clear once before entering the wait loop so stale state from a
            // previous wait does not cause an immediate false-positive trigger.
            மாறிலி தருக்க hasDestructivePostUpdates = !senResults.m_destructivePostUpdates.empty();
            // Create the trigger eval loop, which will await the evaluation step and check the
            // trigger
            Ast_முனை_கோவை* மாறிலி condp
                = புதிய AstLogNot{flp, புதிய AstVarRef{flp, trigvscp, VAccess::READ}};
            AstLoop* மாறிலி loopp = புதிய AstLoop{flp};
            loopp->addStmtsp(புதிய AstLoopTest{flp, loopp, condp});
            loopp->addStmtsp(awaitEvalp);
            // Put pre updates before the trigger check and assignment
            க்கு (Ast_முனை_Stmt* மாறிலி stmtp : senResults.m_preUpdates) loopp->addStmtsp(stmtp);
            // Then the trigger check and assignment
            loopp->addStmtsp(assignp);
            // Let the dynamic trigger scheduler know if this trigger was set
            // If it was, a call to the scheduler's evaluate() will return true
            AstCMethodHard* மாறிலி anyTriggeredMethodp = புதிய AstCMethodHard{
                flp, புதிய AstVarRef{flp, getCreateDynamicTriggerScheduler(), VAccess::WRITE},
                VCMethod::SCHED_ANY_TRIGGERED, புதிய AstVarRef{flp, trigvscp, VAccess::READ}};
            anyTriggeredMethodp->dtypeSetVoid();
            loopp->addStmtsp(anyTriggeredMethodp->makeStmt());
            // If the post update is destructive (e.g. event vars are cleared), create an await for
            // the post update step
            எனில் (hasDestructivePostUpdates) {
                AstCAwait* மாறிலி awaitPostUpdatep = awaitEvalp->clone_மரம்(பொய்);
                VN_AS(awaitPostUpdatep->exprp(), CMethodHard)->method(VCMethod::SCHED_POST_UPDATE);
                loopp->addStmtsp(awaitPostUpdatep);
            }
            // Put the post updates at the end of the loop
            க்கு (Ast_முனை_Stmt* மாறிலி stmtp : senResults.m_postUpdates) loopp->addStmtsp(stmtp);
            // Finally, await the resumption step in 'act'
            AstCAwait* மாறிலி awaitResumep = awaitEvalp->clone_மரம்(பொய்);
            VN_AS(awaitResumep->exprp(), CMethodHard)->method(VCMethod::SCHED_RESUMPTION);
            Ast_முனை::addNext<Ast_முனை_Stmt, Ast_முனை_Stmt>(loopp, awaitResumep);
            // Replace the event control with one explicit stmt chain:
            //   init -> [inits] -> [optional destructive pre-clears] -> loop -> awaitResumption
            Ast_முனை_Stmt* chainp = nullptr;
            chainp = Ast_முனை::addNextNull(chainp, initp);
            க்கு (Ast_முனை_Stmt* மாறிலி stmtp : senResults.m_inits) {
                chainp = Ast_முனை::addNextNull(chainp, stmtp);
            }
            எனில் (hasDestructivePostUpdates) {
                க்கு (Ast_முனை_Stmt* மாறிலி stmtp : senResults.m_destructivePostUpdates) {
                    chainp = Ast_முனை::addNextNull(chainp, stmtp->clone_மரம்(பொய்));
                }
            }
            chainp = Ast_முனை::addNextNull(chainp, loopp);
            nodep->replaceWith(chainp);
        } இல்லையெனில் {
            auto* மாறிலி sentreep = m_finder.get_Sen_மரம்(nodep->sentreep());
            nodep->sentreep()->unlinkFrBack()->delete_மரம்();
            // Get this sentree's trigger scheduler
            // Replace self with a 'co_await trigSched.trigger()'
            auto* மாறிலி triggerMethodp = புதிய AstCMethodHard{
                flp, புதிய AstVarRef{flp, getCreateTriggerSchedulerp(sentreep), VAccess::WRITE},
                VCMethod::SCHED_TRIGGER};
            triggerMethodp->dtypeSetVoid();
            // If it should be committed immediately, pass true, otherwise false
            triggerMethodp->addPinsp(nodep->user2() ? புதிய AstConst{flp, AstConst::BitTrue{}}
                                                    : புதிய AstConst{flp, AstConst::BitFalse{}});
            addProcessInfo(triggerMethodp);
            addEventDebugInfo(triggerMethodp, sentreep);
            // Create the co_await
            AstCAwait* மாறிலி awaitp = புதிய AstCAwait{flp, triggerMethodp, sentreep};
            nodep->replaceWith(awaitp);
        }
        VL_DO_DANGLING(nodep->delete_மரம்(), nodep);
    }
    வெறுமை visit(Ast_முனை_Assign* nodep) override {
        // Only process once to avoid infinite loops (due to the net delay)
        எனில் (nodep->user1SetOnce()) திரும்பு;
        UINFO(9, "control-visit " << nodep);
        File_வரி* மாறிலி flp = nodep->fileline();
        Ast_முனை* controlp = factorOutTimingControl(nodep);
        மாறிலி தருக்க inAssignDly = VN_IS(nodep, AssignDly);
        // Handle the intra assignment timing control
        // Transform if:
        // * there's a timing control in the assignment
        // * the assignment is an AssignDly and it's in a non-inlined function
        எனில் (!controlp && (!inAssignDly || m_underProcedure)) {
            iterateChildren(nodep);
            திரும்பு;
        }
        // The timing control, if any, which is put in a fork for an NBA
        Ast_முனை* மாறிலி timingp = controlp;
        // Insert new vars before the timing control if we're in a function; in a process we can't
        // do that, as after V3Begin only modules and functions declare variables. These
        // intra-assignment vars will later be passed to forked processes by value.
        Ast_முனை* செருகு_Beforep = m_underProcedure ? nullptr : controlp;
        // Special case for NBA
        எனில் (inAssignDly) {
            // Put it in a fork so it doesn't block
            // Could already be the only thing directly under a fork, reuse that if possible
            AstFork* forkp = !nodep->nextp() ? VN_CAST(nodep->firstAbovep(), Fork) : nullptr;
            எனில் (!forkp) forkp = புதிய AstFork{flp, VJoinType::JOIN_NONE};
            எனில் (!m_underProcedure) {
                // If it's in a function, it won't be handled by V3Delayed
                // Put it behind an additional named event that gets triggered in the NBA region
                AstEventControl* மாறிலி nbaEventControlp = createNbaEventControl(flp);
                AstAssign* மாறிலி trigAssignp = createNbaEventTriggerAssignment(flp);
                nodep->replaceWith(trigAssignp);
                trigAssignp->addNextHere(nbaEventControlp);
                nbaEventControlp->addStmtsp(nodep);
                செருகு_Beforep = forkp;
                எனில் (!controlp) controlp = nbaEventControlp;
            }
            controlp->replaceWith(forkp);
            AstBegin* beginp = VN_CAST(controlp, Begin);
            எனில் (!beginp) beginp = புதிய AstBegin{nodep->fileline(), "", controlp, பொய்};
            forkp->addForksp(beginp);
            addFlags(beginp, T_NBA_UPDATE);
            controlp = forkp;
        }
        UASSERT_OBJ(nodep, controlp, "Assignment should have timing control");
        // Move `insertBeforep` into `AstCLocalScope` if necessary to avoid jumping over
        // a variable initialization that would be inserted before `insertBeforep`. All
        // access to this variable should be contained within returned `AstCLocalScope`.
        எனில் (m_underJumpBlock) addCLocalScope(flp, செருகு_Beforep);
        // Function for replacing values with intermediate variables
        மாறிலி auto replaceWithIntermediate = [&](Ast_முனை_கோவை* மாறிலி valuep,
                                                 மாறிலி std::string& name) {
            AstVarScope* மாறிலி newvscp = createTemp(flp, name, valuep->dtypep(), செருகு_Beforep);
            // A pending update keeps its own value, even if its process schedules more
            எனில் (inAssignDly && !செருகு_Beforep) newvscp->fork_By_மதிப்பு(உண்மை);
            valuep->replaceWith(புதிய AstVarRef{flp, newvscp, VAccess::READ});
            controlp->addHereThisAsNext(
                புதிய AstAssign{flp, புதிய AstVarRef{flp, newvscp, VAccess::WRITE}, valuep});
        };
        // NBAs with delays evaluate the indices selecting the target now (IEEE 1800-2023 10.4.2),
        // in order, each as a whole, also the selects and function calls in it. So follow the
        // selects of the target up from the variable, not into their indices. As an example, this
        // transforms 'x[a[i]][f(j)] <= #1 y' into 't0 = a[i]; t1 = f(j); x[t0][t1] <= #1 y'.
        // V3Const has split NBAs to concatenations, also streamed, into NBAs to each part.
        எனில் (inAssignDly) {
            க்கு (Ast_முனை* exprp = nodep->lhsp()->baseFromp(உண்மை); exprp != nodep;
                 exprp = exprp->firstAbovep()) {
                எனில் (AstSel* மாறிலி selp = VN_CAST(exprp, Sel)) {
                    // widthp should be const
                    எனில் (!VN_IS(selp->lsbp(), Const)) {
                        replaceWithIntermediate(selp->lsbp(), m_intraLsbNames.get(nodep));
                    }
                } இல்லையெனில் எனில் (Ast_முனை_Sel* மாறிலி selp = VN_CAST(exprp, முனை_Sel)) {
                    எனில் (!VN_IS(selp->bitp(), Const)) {
                        replaceWithIntermediate(selp->bitp(), m_intraIndexNames.get(nodep));
                    }
                }
            }
        }
        // Replace the RHS with an intermediate value var
        replaceWithIntermediate(nodep->rhsp(), m_intra_மதிப்பு_Names.get(nodep));
        எனில் (!inAssignDly) திரும்பு;
        // The process executing the NBA evaluates its delay (IEEE 1800-2023 4.9.4), not the forked
        // process, if it has side effects, e.g. calling a function that checks its process. A pure
        // one is evaluated as the forked process starts, at once.
        எனில் (AstDelay* மாறிலி delayp = VN_CAST(timingp, Delay)) {
            எனில் (!delayp->lhsp()->isPure()) {
                replaceWithIntermediate(delayp->lhsp(), m_intra_மதிப்பு_Names.get(nodep));
            }
        }
    }
    வெறுமை visit(AstFireEvent* nodep) override {
        // V3Delayed handles '->>' of a variable in a process. Like NBAs in non-inlined functions,
        // trigger other events from a fork awaiting the NBA region.
        எனில் (!nodep->isDelayed() || (m_underProcedure && VN_IS(nodep->operandp(), VarRef))) {
            iterateChildren(nodep);
            திரும்பு;
        }
        File_வரி* மாறிலி flp = nodep->fileline();
        AstAssign* மாறிலி trigAssignp = createNbaEventTriggerAssignment(flp);
        nodep->replaceWith(trigAssignp);
        AstFork* மாறிலி forkp = புதிய AstFork{flp, VJoinType::JOIN_NONE};
        trigAssignp->addNextHere(forkp);
        எனில் (m_underJumpBlock) addCLocalScope(flp, forkp);
        // The triggered event is the one referenced now, so evaluate handles and indices now
        Ast_முனை_கோவை* மாறிலி eventp = nodep->operandp()->unlinkFrBack();
        மாறிலி auto evalNow = [&](Ast_முனை_கோவை* மாறிலி valuep, V3UniqueNames& names) {
            // Like for an NBA, a process uses a module variable, passed to the fork by value
            AstVarScope* மாறிலி vscp = createTemp(flp, names.get(nodep), valuep->dtypep(),
                                                 m_underProcedure ? nullptr : forkp);
            எனில் (m_underProcedure) vscp->fork_By_மதிப்பு(உண்மை);
            valuep->replaceWith(புதிய AstVarRef{flp, vscp, VAccess::READ});
            // Unlike an event, a handle selecting one is only read
            valuep->foreach([](Ast_முனை* மாறிலி np) {
                எனில் (Ast_முனை_Var_Ref* மாறிலி refp = VN_CAST(np, முனை_Var_Ref)) {
                    refp->access(VAccess::READ);
                } இல்லையெனில் எனில் (AstMemberSel* மாறிலி selp = VN_CAST(np, MemberSel)) {
                    selp->access(VAccess::READ);
                }
            });
            forkp->addHereThisAsNext(
                புதிய AstAssign{flp, புதிய AstVarRef{flp, vscp, VAccess::WRITE}, valuep});
        };
        Ast_முனை_கோவை* refp = eventp;
        போது (Ast_முனை_Sel* மாறிலி selp = VN_CAST(refp, முனை_Sel)) {
            எனில் (!VN_IS(selp->bitp(), Const)) evalNow(selp->bitp(), m_intraIndexNames);
            refp = selp->fromp();
        }
        // Handle of a class (or virtual interface)
        எனில் (AstMemberSel* மாறிலி selp = VN_CAST(refp, MemberSel)) {
            evalNow(selp->fromp(), m_intra_மதிப்பு_Names);
        }
        AstEventControl* மாறிலி controlp = createNbaEventControl(flp);
        controlp->addStmtsp(புதிய AstFireEvent{flp, eventp, பொய்});
        AstBegin* மாறிலி beginp = புதிய AstBegin{flp, "", controlp, பொய்};
        addFlags(beginp, T_NBA_UPDATE);
        forkp->addForksp(beginp);
        VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
    }
    வெறுமை visit(AstAssignW* nodep) override {
        File_வரி* மாறிலி flp = nodep->fileline();
        // Get the net delay unless this assignment was created for handling the net delay (user1)
        AstDelay* மாறிலி netDelayp = nodep->user1() ? nullptr : nodep->getLhsNetDelay();
        எனில் (netDelayp) {
            எனில் (nodep->timingControlp()) {
                // If this assignment has a delay, create another one to handle the net delay
                AstVarScope* மாறிலி newvscp = m_assignWTmps.make(flp, m_scopep, nodep->dtypep());
                AstAssignW* assignp = புதிய AstAssignW{
                    nodep->fileline(), nodep->lhsp()->unlinkFrBack(),
                    புதிய AstVarRef{flp, newvscp, VAccess::READ}, netDelayp->clone_மரம்(பொய்)};
                assignp->user1(உண்மை);
                nodep->addNextHere(assignp);
                nodep->lhsp(புதிய AstVarRef{flp, newvscp, VAccess::WRITE});
            } இல்லையெனில் {
                // Else just use this one with the net delay
                nodep->timingControlp(netDelayp->clone_மரம்(பொய்));
            }
        }
        எனில் (!nodep->timingControlp()) திரும்பு;
        // There will be some circular logic here, suppress the warning for newly created vars
        flp->modifyWarnOff(V3ErrorCode::UNOPTFLAT, உண்மை);
        // Also suppress the warning for the LHS var, for cases like `assign #1 clk = ~clk;`
        // TODO: Restore the warning for other, non-delayed drivers
        nodep->lhsp()->foreach([](AstVarRef* refp) {
            எனில் (refp->access().isWriteOrRW()) {
                refp->varp()->fileline()->modifyWarnOff(V3ErrorCode::UNOPTFLAT, உண்மை);
            }
        });
        // Should be under an always
        AstAlways* மாறிலி alwaysp = VN_AS(m_procp, Always);
        // Convert it to an Assign; the new assign with intra delay will be handled by
        // visit(AstNodeAssign*)
        Ast_முனை_கோவை* மாறிலி lhs1p = nodep->lhsp()->unlinkFrBack();
        Ast_முனை_கோவை* மாறிலி rhs1p = nodep->rhsp()->unlinkFrBack();
        Ast_முனை* மாறிலி controlp = nodep->timingControlp()->unlinkFrBack();
        எனில் (AstDelay* மாறிலி delayp = VN_CAST(controlp, Delay)) {
            எனில் (Ast_முனை_கோவை* fallDelayp = delayp->fallDelay()) {
                fallDelayp = fallDelayp->unlinkFrBack();
                Ast_முனை_கோவை* lhsp = delayp->lhsp()->unlinkFrBack();
                // Use fall only for an all-zero value, rise otherwise.
                lhsp = delayToIntegral(delayp, lhsp);
                fallDelayp = delayToIntegral(delayp, fallDelayp);
                delayp->lhsp(
                    புதிய AstCond{flp, புதிய AstEq{flp, rhs1p->clone_மரம்(பொய்), புதிய AstConst{flp, 0}},
                                fallDelayp, lhsp});
            }
        }
        AstAssign* மாறிலி assignp = புதிய AstAssign{nodep->fileline(), lhs1p, rhs1p, controlp};
        // Put the assignment in a fork..join_none.
        AstFork* மாறிலி forkp = புதிய AstFork{flp, VJoinType::JOIN_NONE};
        nodep->replaceWith(forkp);
        VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
        AstBegin* மாறிலி beginp = புதிய AstBegin{flp, "", assignp, பொய்};
        forkp->addForksp(beginp);
        visit(forkp);  // Visit now as we need to do some post-processing
        // IEEE 1800-2023 10.3.3 - if the RHS value differs from the currently scheduled value to
        // be assigned, the currently scheduled assignment is descheduled. To keep track if an
        // assignment should be descheduled, each scheduled assignment event has a 'generation',
        // and if at assignment time its generation differs from the current generation, it won't
        // be performed
        AstAssign* மாறிலி preAssignp = VN_AS(beginp->stmtsp(), Assign);
        UASSERT_OBJ(preAssignp, alwaysp, "Pre-assign should be there from visit(AstFork)");
        AstAssign* மாறிலி postAssignp = VN_AS(preAssignp->nextp()->nextp(), Assign);
        UASSERT_OBJ(postAssignp, alwaysp, "Post-assign should be there from visit(AstFork)");
        // Increment generation and copy it to a local
        AstVarScope* மாறிலி generationVarp
            = m_assignWGens.make(flp, m_scopep, alwaysp->findUInt64DType());
        AstVarScope* மாறிலி genLocalVarp
            = createTemp(flp, generationVarp->varp()->name() + "__local",
                         alwaysp->findUInt64DType(), preAssignp);
        preAssignp->addHereThisAsNext(
            புதிய AstAssign{flp, புதிய AstVarRef{flp, generationVarp, VAccess::WRITE},
                          புதிய AstAdd{flp, புதிய AstVarRef{flp, generationVarp, VAccess::READ},
                                     புதிய AstConst{flp, 1}}});
        preAssignp->addHereThisAsNext(
            புதிய AstAssign{flp, புதிய AstVarRef{flp, genLocalVarp, VAccess::WRITE},
                          புதிய AstVarRef{flp, generationVarp, VAccess::READ}});
        // If the current generation is same as the one saved in the local var, assign
        beginp->addStmtsp(
            புதிய AstIf{flp,
                      புதிய AstEq{flp, புதிய AstVarRef{flp, generationVarp, VAccess::READ},
                                புதிய AstVarRef{flp, genLocalVarp, VAccess::READ}},
                      postAssignp->unlinkFrBack()});
        // Save scheduled RHS value before delay
        AstVarScope* மாறிலி tmpVarp
            = m_assignWTmps.make(flp, m_scopep, preAssignp->rhsp()->dtypep());
        AstVarRef* மாறிலி tmpAssignRhsp = VN_AS(preAssignp->lhsp(), VarRef)->clone_மரம்(பொய்);
        tmpAssignRhsp->access(VAccess::WRITE);
        preAssignp->addNextHere(
            புதிய AstAssign{flp, புதிய AstVarRef{flp, tmpVarp, VAccess::WRITE}, tmpAssignRhsp});
        // If the RHS is different from the currently scheduled value, schedule the new assignment
        // The generation will increase, effectively 'descheduling' the previous assignment.
        Ast_முனை_கோவை* மாறிலி didNotInitp
            = புதிய AstLogNot{flp, புதிய AstCExpr{flp, "vlSymsp->__Vm_didInit", 1}};
        AstVarRef* மாறிலி firstIterp
            = புதிய AstVarRef{flp, m_netlistp->stlFirstIterationp(), VAccess::READ};
        Ast_முனை_கோவை* மாறிலி schedCondp
            = புதிய AstLogOr{flp,
                           புதிய AstNeq{flp, preAssignp->rhsp()->clone_மரம்(பொய்),
                                      புதிய AstVarRef{flp, tmpVarp, VAccess::READ}},
                           புதிய AstLogAnd{flp, didNotInitp, firstIterp}};
        alwaysp->addStmtsp(புதிய AstIf{flp, schedCondp, forkp->unlinkFrBack()});
    }
    வெறுமை visit(AstDisableFork* nodep) override {
        எனில் (m_hasProcess) திரும்பு;
        // never reached by any process; remove to avoid compilation error
        VL_DO_DANGLING(nodep->unlinkFrBack()->delete_மரம்(), nodep);
    }
    வெறுமை visit(AstWaitFork* nodep) override {
        எனில் (m_hasProcess) {
            File_வரி* மாறிலி flp = nodep->fileline();
            AstCExpr* மாறிலி exprp = புதிய AstCExpr{flp, "vlProcess->completedFork()", 1};
            AstWait* மாறிலி waitp = புதிய AstWait{flp, exprp, nullptr};
            nodep->replaceWith(waitp);
        } இல்லையெனில் {
            // never reached by any process; remove to avoid compilation error
            nodep->unlinkFrBack();
        }
        VL_DO_DANGLING(nodep->delete_மரம்(), nodep);
    }
    வெறுமை visit(AstWait* nodep) override {
        // Wait on changed events related to the vars in the wait statement
        UINFO(9, "control-visit " << nodep);
        File_வரி* மாறிலி flp = nodep->fileline();
        Ast_முனை* மாறிலி stmtsp = nodep->stmtsp();
        எனில் (stmtsp) stmtsp->unlinkFrBackWithNext();
        Ast_முனை_கோவை* மாறிலி condp = V3Const::constifyEdit(nodep->condp()->unlinkFrBack());
        auto* மாறிலி constp = VN_CAST(condp, Const);
        எனில் (constp) {
            எனில் (constp->isZero()) {
                // We have to await forever instead of simply returning in case we're deep in a
                // callstack
                AstCMethodHard* மாறிலி foreverMethodp = புதிய AstCMethodHard{
                    flp, புதிய AstVarRef{flp, getCreateDelayScheduler(), VAccess::WRITE},
                    VCMethod::SCHED_WAIT_FOREVER};
                foreverMethodp->dtypeSetVoid();
                addProcessInfo(foreverMethodp);
                addDebugInfo(foreverMethodp);
                AstCAwait* மாறிலி awaitp = புதிய AstCAwait{flp, foreverMethodp};
                nodep->replaceWith(awaitp);
                எனில் (stmtsp) VL_DO_DANGLING(stmtsp->delete_மரம்(), stmtsp);
                VL_DO_DANGLING(condp->delete_மரம்(), condp);
            } இல்லையெனில் {
                nodep->v3fatalSrc("constant wait should have been removed in "
                                  "TimingSuspendableVisitor::visit(AstWait)");
            }
        } இல்லையெனில் எனில் (needDynamicTrigger(condp)) {
            // No point in making a sentree, just use the expression as sensitivity
            // Put the event control in an if so we only wait if the condition isn't met already
            auto* மாறிலி ifp = புதிய AstIf{
                flp, புதிய AstLogNot{flp, condp},
                புதிய AstEventControl{flp,
                                    புதிய Ast_Sen_மரம்{flp, புதிய AstSenItem{flp, VEdgeType::ET_TRUE,
                                                                       condp->clone_மரம்(பொய்)}},
                                    nullptr}};
            எனில் (stmtsp) Ast_முனை::addNext<Ast_முனை, Ast_முனை>(ifp, stmtsp);
            nodep->replaceWith(ifp);
        } இல்லையெனில் {
            // We are using a global sentree, so we cannot use ET_TRUE, as that could lead to the
            // active region never converging. Because of this, we need to use ET_CHANGED in a
            // loop.
            AstEventControl* மாறிலி controlp = புதிய AstEventControl{
                flp,
                புதிய Ast_Sen_மரம்{
                    flp, புதிய AstSenItem{flp, VEdgeType::ET_CHANGED, condp->clone_மரம்(பொய்)}},
                nullptr};
            controlp->user2(உண்மை);  // Commit immediately
            AstLoop* மாறிலி loopp = புதிய AstLoop{flp};
            loopp->addStmtsp(புதிய AstLoopTest{flp, loopp, புதிய AstLogNot{flp, condp}});
            loopp->addStmtsp(controlp);
            எனில் (stmtsp) Ast_முனை::addNext<Ast_முனை, Ast_முனை>(loopp, stmtsp);
            nodep->replaceWith(loopp);
        }
        VL_DO_DANGLING(nodep->delete_மரம்(), nodep);
    }
    வெறுமை visit(AstBegin* nodep) override {
        VL_RESTORER(m_procp);
        VL_RESTORER(m_hasProcess);
        m_hasProcess |= hasFlags(nodep, T_HAS_PROC);
        // A pending NBA update is not a subprocess of the process that scheduled it, so it is
        // unaffected by 'disable fork' and 'wait fork' (IEEE 1800-2023 9.6.1, 9.6.3)
        எனில் (hasFlags(nodep, T_NBA_UPDATE)) m_hasProcess = பொய்;
        m_procp = nodep;
        எனில் (m_hasProcess) nodep->setNeedProcess();
        iterateChildren(nodep);
    }
    வெறுமை visit(AstFork* nodep) override {
        எனில் (nodep->user1SetOnce()) திரும்பு;
        UINFO(9, "control-visit " << nodep);
        v3Global.setUsesTiming();

        // Create a unique name for this fork
        nodep->name("__Vfork_" + std::to_string(++m_forkCnt));

        // TODO: Should process nodep->stmtsp() in case an earlier pass
        //       inserted something there, but as of today we don't need to.

        // Process and name each fork
        size_t idx = 0;  // Index for naming begins
        க்கு (AstBegin *itemp = nodep->forksp(), *nextp; itemp; itemp = nextp) {
            nextp = VN_AS(itemp->nextp(), Begin);
            iterate(itemp);
            // Note: Even if we do not find any awaits, we cannot simply inline
            // the process here, as new awaits could be added later.

            // Name the begin (later the name will be used for a new function)
            itemp->name(nodep->name() + "__" + std::to_string(idx++));
            எனில் (itemp->needProcess()) {
                itemp->addStmtsp(
                    புதிய AstCStmt{itemp->fileline(), "vlProcess->state(VlProcess::FINISHED);"});
            }
        }
        எனில் (!nodep->joinType().joinNone()) makeForkJoin(nodep);
    }

    //--------------------
    வெறுமை visit(AstVar*) override {}  // Accelerate
    வெறுமை visit(Ast_முனை* nodep) override { iterateChildren(nodep); }

பொது:
    // CONSTRUCTORS
    explicit TimingControlVisitor(AstNetlist* nodep)
        : m_netlistp{nodep} {
        iterate(nodep);

        // If there is no static #0 in the design, but an unknown delay was found,
        // and the user did not specify preference, then warn on all unknown delays
        // as we will be assuming they can be #0, which can cause performance degradation.
        க்கு (File_வரி* மாறிலி flp : m_unknownDelayFlps) {
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

Ast_முனை_கோவை* V3Timing::delayToTicks(Ast_முனை_கோவை* valuep, VTimescale timeunit) {
    File_வரி* மாறிலி flp = valuep->fileline();
    எனில் (மாறிலி AstConst* மாறிலி constp = VN_CAST(valuep, Const)) {
        எனில் (constp->இலக்கம்().is1Step()) {
            // A step is one tick of the global time precision (IEEE 1800-2023 3.14.3)
            VL_DO_DANGLING(valuep->delete_மரம்(), valuep);
            AstConst* மாறிலி tickp = புதிய AstConst{flp, AstConst::Unsized64{}, 1};
            tickp->dtypeSetBitSized(64, VSigning::UNSIGNED);
            திரும்பு tickp;
        }
        எனில் (constp->isZero()) திரும்பு valuep;
    }
    UASSERT_OBJ(!timeunit.isNone(), valuep, "timeunit must be set");
    மாறிலி இரட்டை timescaleFactorD
        = std::pow(10.0, timeunit.powerOfTen() - v3Global.rootp()->timeprecision().powerOfTen());
    எனில் (valuep->dtypep()->skipRefp()->isDouble()) {
        AstConst* மாறிலி tsfp = புதிய AstConst{flp, AstConst::RealDouble{}, timescaleFactorD};
        valuep = புதிய AstMulD{flp, valuep, tsfp};
        valuep = புதிய AstRToIRoundS{flp, valuep};
        valuep->dtypeSetBitSized(64, VSigning::UNSIGNED);
    } இல்லையெனில் {
        valuep->dtypeSetBitSized(64, VSigning::UNSIGNED);
        மாறிலி uint64_t timescaleFactorU = static_cast<uint64_t>(timescaleFactorD);
        AstConst* மாறிலி tsfp = புதிய AstConst{flp, AstConst::Unsized64{}, timescaleFactorU};
        valuep = புதிய AstMul{flp, valuep, tsfp};
    }
    திரும்பு V3Const::constifyEdit(valuep);  // Simplify
}

வெறுமை V3Timing::timingAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        மாறிலி VNUser1InUse m_user1InUse;
        மாறிலி VNUser2InUse m_user2InUse;
        { TimingSuspendableVisitor{nodep}; }
        எனில் (v3Global.usesTiming()) TimingControlVisitor{nodep};
    }
    V3Global::dump_Check_Global_மரம்("timing", 0, dump_மரம்_Either_Level() >= 3);
}
