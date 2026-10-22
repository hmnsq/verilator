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

#समावेश "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#समावेश "V3Timing.h"

#समावेश "V3Const.h"
#समावेश "V3EmitV.h"
#समावेश "V3File.h"
#समावेश "V3Global.h"
#समावेश "V3Graph.h"
#समावेश "V3MemberMap.h"
#समावेश "V3SenExprBuilder.h"
#समावेश "V3SenTree.h"
#समावेश "V3SharedTmps.h"
#समावेश "V3Stats.h"
#समावेश "V3UniqueNames.h"

#समावेश <limits>
#समावेश <queue>

VL_DEFINE_DEBUG_FUNCTIONS;

// ######################################################################

क्रमागत गाँठ_Flag : uint8_t {
    T_SUSPENDEE = 1 << 0,  // Suspendable (due to dependence on another suspendable)
    T_SUSPENDER = 1 << 1,  // Suspendable (has timing control)
    T_ALLOCS_PROC = 1 << 2,  // Can allocate VlProcess
    T_FORCES_PROC = 1 << 3,  // Forces VlProcess allocation
    T_NEEDS_PROC = 1 << 4,  // Needs access to VlProcess if it's allocated
    T_HAS_PROC = 1 << 5,  // Has VlProcess argument in the signature
    T_NBA_UPDATE = 1 << 6,  // Fork branch of a pending NBA update, which is not a subprocess
};

क्रमागत ForkType : uint8_t {
    F_NONE = 0,  // Not under a fork
    F_MIGHT_SUSPEND = 1 << 0,  // Fork might suspend the execution of current process
    F_MIGHT_NEED_PROC = 1 << 1,  // Fork might need a process (any fork really)
};

क्रमागत PropagationType : uint8_t {
    P_CALL = 1,  // Propagation through call to a function/task/method
    P_FORK = 2,  // Propagation due to fork's behavior
    P_SIGNATURE = 3,  // Propagation required to maintain C++ function's signature requirements
};

// Add timing flag to a node
अटल व्योम addFlags(Ast_गाँठ* स्थिर nodep, uint8_t flags) { nodep->user2Or(flags); }
// Check if a node has ALL of the expected flags set
अटल bool hasFlags(Ast_गाँठ* स्थिर nodep, uint8_t flags) { वापस !(~nodep->user2() & flags); }

// ######################################################################
//  Detect nodes affected by timing and/or requiring a process

श्रेणी TimingSuspendableVisitor final : खुला VNVisitor {
    // TYPES
    // Vertex of a dependency graph of suspendable nodes, e.g. if a node (process or task) is
    // suspendable, all its dependents should also be suspendable
    श्रेणी DepVtx VL_NOT_FINAL : खुला V3GraphVertex {
        VL_RTTI_IMPL(DepVtx, V3GraphVertex)
        AstClass* स्थिर m_classp;  // Class associated with a method
        Ast_गाँठ* स्थिर m_nodep;  // AST node represented by this graph vertex

        // ACCESSORS
        string name() स्थिर override {
            अगर (m_classp) {
                अगर (VN_IS(nodep(), CFunc)) {
                    वापस cvtToHex(nodep()) + ' ' + classp()->name() + "::" + nodep()->name();
                }
            }
            वापस cvtToHex(nodep()) + ' ' + nodep()->prettyTypeName();
        }
        File_पंक्ति* fileline() स्थिर override { वापस nodep()->fileline(); }

    खुला:
        // CONSTRUCTORS
        DepVtx(V3Graph* graphp, Ast_गाँठ* nodep, AstClass* classp)
            : V3GraphVertex{graphp}
            , m_classp{classp}
            , m_nodep{nodep} {}
        ~DepVtx() override = शेष;

        // ACCESSORS
        भव Ast_गाँठ* nodep() स्थिर VL_MT_STABLE { वापस m_nodep; }
        भव Ast_गाँठ* classp() स्थिर VL_MT_STABLE { वापस m_classp; }
    };

    श्रेणी SuspendDepVtx final : खुला DepVtx {
        VL_RTTI_IMPL(SuspendDepVtx, DepVtx)
        string dotColor() स्थिर override {
            अगर (hasFlags(nodep(), T_SUSPENDER)) वापस "red";
            अगर (hasFlags(nodep(), T_SUSPENDEE)) वापस "blue";
            वापस "black";
        }

    खुला:
        SuspendDepVtx(V3Graph* graphp, Ast_गाँठ* nodep, AstClass* classp)
            : DepVtx{graphp, nodep, classp} {}
        ~SuspendDepVtx() override = शेष;
    };

    श्रेणी NeedsProcDepVtx final : खुला DepVtx {
        VL_RTTI_IMPL(NeedsProcDepVtx, DepVtx)
        string dotColor() स्थिर override {
            अगर (hasFlags(nodep(), T_HAS_PROC)) वापस "blue";
            अगर (hasFlags(nodep(), T_NEEDS_PROC)) वापस "green";
            अगर (hasFlags(nodep(), T_FORCES_PROC)) वापस "red";
            वापस "black";
        }

    खुला:
        NeedsProcDepVtx(V3Graph* graphp, Ast_गाँठ* nodep, AstClass* classp)
            : DepVtx{graphp, nodep, classp} {}
        ~NeedsProcDepVtx() override = शेष;
    };

    // NODE STATE
    //  AstClass::user1() -> bool.  Class member cache has been refreshed.
    //  Ast{NodeProcedure,CFunc,Begin}::user2()  -> uint64_t.  Set to >= T_SUSP if
    //                                                         process/task suspendable
    //                                                         and to T_PROC if it
    //                                                         needs process metadata.
    //  Ast{NodeProcedure,CFunc,Begin}::user3()  -> DependencyVertex*.  Vertex in m_suspGraph
    //  Ast{NodeProcedure,CFunc,Begin}::user3()  -> DependencyVertex*.  Vertex in m_procGraph
    स्थिर VNUser3InUse m_user3InUse;
    स्थिर VNUser4InUse m_user4InUse;

    // STATE
    VMemberMap m_memberMap;  // Member names cached for fast lookup
    AstClass* m_classp = nullptr;  // Current class
    Ast_गाँठ* m_procp = nullptr;  // NodeProcedure/CFunc/Begin we're under
    uint8_t m_underFork = F_NONE;  // F_NONE or flags of a fork we are under
    V3Graph m_suspGraph;  // Dependency graph where a node is a dependency of another if it being
                          // suspendable makes the other node suspendable
    V3Graph m_procGraph;  // Dependency graph where a node is a dependency of another if it being
                          // suspendable makes the other node suspendable

    // METHODS
    // Get or create the dependency vertex for the given node
    DepVtx* getSuspendDepVtx(Ast_गाँठ* स्थिर nodep) {
        AstClass* classp = nullptr;
        अगर (AstCFunc* funcp = VN_CAST(nodep, CFunc)) {
            अगर (funcp->scopep() && funcp->scopep()->modp()) {
                classp = VN_CAST(funcp->scopep()->modp(), Class);
            }
        }
        अगर (!nodep->user3p()) nodep->user3p(नया SuspendDepVtx{&m_suspGraph, nodep, classp});
        वापस nodep->user3u().to<SuspendDepVtx*>();
    }
    DepVtx* getNeedsProcDepVtx(Ast_गाँठ* स्थिर nodep) {
        AstClass* classp = nullptr;
        अगर (AstCFunc* funcp = VN_CAST(nodep, CFunc)) {
            अगर (funcp->scopep() && funcp->scopep()->modp()) {
                classp = VN_CAST(funcp->scopep()->modp(), Class);
            }
        }
        अगर (!nodep->user4p()) nodep->user4p(नया NeedsProcDepVtx{&m_procGraph, nodep, classp});
        वापस nodep->user4u().to<NeedsProcDepVtx*>();
    }
    // Pass timing flag between nodes
    bool passFlag(स्थिर Ast_गाँठ* from, Ast_गाँठ* to, गाँठ_Flag flag) {
        अगर ((from->user2() & flag) && !(to->user2() & flag)) {
            addFlags(to, flag);
            वापस true;
        }
        वापस false;
    }
    // Propagate flag to all nodes that depend on the given one
    व्योम propagateFlags(DepVtx* स्थिर vxp, गाँठ_Flag flag) {
        स्वतः* स्थिर parentp = vxp->nodep();
        क्रम (V3GraphEdge& edge : vxp->outEdges()) {
            स्वतः* स्थिर depVxp = static_cast<DepVtx*>(edge.शीर्ष());
            Ast_गाँठ* स्थिर depp = depVxp->nodep();
            अगर (passFlag(parentp, depp, flag)) propagateFlags(depVxp, flag);
        }
    }
    ढाँचा <typename T_Predicate>
    व्योम propagateFlagsIf(DepVtx* स्थिर vxp, गाँठ_Flag flag, T_Predicate p) {
        स्वतः* स्थिर parentp = vxp->nodep();
        क्रम (V3GraphEdge& edge : vxp->outEdges()) {
            स्वतः* स्थिर depVxp = static_cast<DepVtx*>(edge.शीर्ष());
            Ast_गाँठ* स्थिर depp = depVxp->nodep();
            अगर (p(&edge) && passFlag(parentp, depp, flag)) propagateFlagsIf(depVxp, flag, p);
        }
    }
    ढाँचा <typename T_Predicate>
    व्योम propagateFlagsReversedIf(DepVtx* स्थिर vxp, गाँठ_Flag flag, T_Predicate p) {
        स्वतः* स्थिर parentp = vxp->nodep();
        क्रम (V3GraphEdge& edge : vxp->inEdges()) {
            स्वतः* स्थिर depVxp = static_cast<DepVtx*>(edge.fromp());
            Ast_गाँठ* स्थिर depp = depVxp->nodep();
            अगर (p(&edge) && passFlag(parentp, depp, flag))
                propagateFlagsReversedIf(depVxp, flag, p);
        }
    }

    // VISITORS
    व्योम visit(AstClass* nodep) override {
        UASSERT_OBJ(!m_classp, nodep, "Class under class");
        VL_RESTORER(m_classp);
        m_classp = nodep;
        iterateChildren(nodep);
    }
    व्योम visit(Ast_गाँठ_Procedure* nodep) override {
        VL_RESTORER(m_procp);
        m_procp = nodep;
        getNeedsProcDepVtx(nodep);
        addFlags(nodep, T_ALLOCS_PROC);
        अगर (VN_IS(nodep, Always)) {
            UINFO(9, "Always does " << (nodep->needProcess() ? "" : "NOT ") << "need process");
        }
        iterateChildren(nodep);
    }
    व्योम visit(AstDisableFork* nodep) override {
        visit(static_cast<Ast_गाँठ*>(nodep));
        addFlags(m_procp, T_FORCES_PROC | T_NEEDS_PROC);
    }
    व्योम visit(AstWaitFork* nodep) override {
        visit(static_cast<Ast_गाँठ*>(nodep));
        addFlags(m_procp, T_FORCES_PROC | T_NEEDS_PROC);
    }
    व्योम visit(AstWait* nodep) override {
        UINFO(9, "suspendable-visit " << nodep);
        Ast_गाँठ_व्यंजक* स्थिर condp = V3Const::constifyEdit(nodep->condp());
        अगर (AstConst* स्थिर constp = VN_CAST(condp, Const)) {
            अगर (!nodep->fileline()->warnIsOff(V3ErrorCode::WAITCONST)) {
                condp->v3warn(WAITCONST, "Wait statement condition is constant");
            }
            अगर (!constp->isZero()) {
                // Remove AstWait before we track process as T_SUSPENDER
                अगर (Ast_गाँठ* स्थिर stmtsp = nodep->stmtsp()) {
                    stmtsp->unlink_Fr_Back_With_अग्र();
                    nodep->replaceWith(stmtsp);
                } अन्यथा {
                    nodep->unlinkFrBack();
                }
                VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
                वापस;
            }
        }
        v3Global.setUsesTiming();
        अगर (m_procp) addFlags(m_procp, T_SUSPENDEE | T_SUSPENDER | T_NEEDS_PROC);
        iterateChildren(nodep);
    }
    व्योम visit(AstCFunc* nodep) override {
        VL_RESTORER(m_procp);
        m_procp = nodep;
        iterateChildren(nodep);
        अगर (nodep->needProcess()) addFlags(nodep, T_FORCES_PROC | T_NEEDS_PROC);
        DepVtx* स्थिर sVxp = getSuspendDepVtx(nodep);
        DepVtx* स्थिर pVxp = getNeedsProcDepVtx(nodep);
        अगर (!m_classp) वापस;

        // Go over overridden functions

        std::queue<AstClassExtends*> extends;
        अगर (m_classp->extendsp()) extends.धकेलो(m_classp->extendsp());

        जबतक (!extends.empty()) {
            AstClassExtends* ext_list = extends.front();
            extends.निकालो();

            क्रम (AstClassExtends* cextp = ext_list; cextp;
                 cextp = VN_AS(cextp->nextp(), ClassExtends)) {
                // TODO: It is possible that a methods the same name in the base class is not
                // actually overridden by our method. If this causes a problem, traverse to
                // the root of the inheritance hierarchy and check if the original method is
                // virtual or not.
                अगर (स्वतः* स्थिर overriddenp
                    = VN_CAST(m_memberMap.findMember(cextp->classp(), nodep->name()), CFunc)) {
                    // Suspendability and process affects typing, so they propagate both ways
                    DepVtx* स्थिर overriddenSVxp = getSuspendDepVtx(overriddenp);
                    DepVtx* स्थिर overriddenPVxp = getNeedsProcDepVtx(overriddenp);
                    नया V3GraphEdge{&m_suspGraph, sVxp, overriddenSVxp, P_SIGNATURE};
                    नया V3GraphEdge{&m_suspGraph, overriddenSVxp, sVxp, P_SIGNATURE};
                    नया V3GraphEdge{&m_procGraph, pVxp, overriddenPVxp, P_SIGNATURE};
                    नया V3GraphEdge{&m_procGraph, overriddenPVxp, pVxp, P_SIGNATURE};
                } अन्यथा {
                    AstClassExtends* more_extends = cextp->classp()->extendsp();
                    अगर (more_extends) extends.धकेलो(more_extends);
                }
            }
        }
    }
    व्योम visit(Ast_गाँठ_CCall* nodep) override {
        AstCFunc* स्थिर funcp = nodep->funcp();
        UASSERT_OBJ(funcp, nodep, "AstNodeCCall must have non-null funcp post-link");
        UASSERT_OBJ(m_procp, nodep, "AstNodeCCall must be inside a procedure/CFunc/Begin");

        UINFO(9, "V3Timing: Processing CCall to " << funcp->name() << " in dependency graph\n");
        नया V3GraphEdge{&m_suspGraph, getSuspendDepVtx(funcp), getSuspendDepVtx(m_procp), P_CALL};

        नया V3GraphEdge{&m_procGraph, getNeedsProcDepVtx(funcp), getNeedsProcDepVtx(m_procp),
                        P_CALL};

        iterateChildren(nodep);
    }
    व्योम visit(AstBegin* nodep) override {
        VL_RESTORER(m_procp);
        VL_RESTORER(m_underFork);

        अगर (!m_underFork || (m_underFork & F_MIGHT_SUSPEND))
            नया V3GraphEdge{&m_suspGraph, getSuspendDepVtx(nodep), getSuspendDepVtx(m_procp),
                            m_underFork ? P_FORK : P_CALL};

        नया V3GraphEdge{&m_procGraph, getNeedsProcDepVtx(nodep), getNeedsProcDepVtx(m_procp),
                        P_CALL};

        अगर (m_underFork) addFlags(nodep, T_NEEDS_PROC | T_ALLOCS_PROC);

        m_procp = nodep;
        m_underFork = 0;
        iterateChildren(nodep);
    }
    व्योम visit(AstFork* nodep) override {
        UINFO(9, "suspendable-visit " << nodep);
        VL_RESTORER(m_underFork);

        v3Global.setUsesTiming();  // Even if there are no event controls, we have to set this flag
                                   // so that transformForks() in V3SchedTiming gets called and
                                   // removes all forks and begins
        अगर (nodep->isTimingControl() && m_procp) {
            addFlags(m_procp, T_SUSPENDEE | T_SUSPENDER);
            m_underFork |= F_MIGHT_SUSPEND;
        }
        m_underFork |= F_MIGHT_NEED_PROC;
        iterateChildren(nodep);
    }
    व्योम visit(AstAssignDly* nodep) override {
        अगर (!VN_IS(m_procp, गाँठ_Procedure)) v3Global.setUsesTiming();
        visit(static_cast<Ast_गाँठ*>(nodep));
    }
    व्योम visit(AstFireEvent* nodep) override {
        अगर (nodep->isDelayed()
            && (!VN_IS(m_procp, गाँठ_Procedure) || !VN_IS(nodep->operandp(), VarRef))) {
            v3Global.setUsesTiming();
        }
        visit(static_cast<Ast_गाँठ*>(nodep));
    }
    व्योम visit(AstAssignW* nodep) override {
        अगर (nodep->timingControlp()) v3Global.setUsesTiming();
        // Containing process will not suspend, don't mark it
    }
    व्योम visit(Ast_गाँठ* nodep) override {
        अगर (nodep->isTimingControl()) {
            v3Global.setUsesTiming();
            अगर (m_procp) addFlags(m_procp, T_SUSPENDEE | T_SUSPENDER | T_NEEDS_PROC);
        }
        iterateChildren(nodep);
    }

    //--------------------
    व्योम visit(AstVar*) override {}  // Accelerate

खुला:
    // CONSTRUCTORS
    explicit TimingSuspendableVisitor(AstNetlist* nodep) {
        iterate(nodep);
        m_suspGraph.removeTransitiveEdges();
        m_procGraph.removeTransitiveEdges();
        // Propagate suspendability
        क्रम (V3GraphVertex& vtx : m_suspGraph.vertices()) {
            DepVtx& depVtx = static_cast<DepVtx&>(vtx);
            अगर (hasFlags(depVtx.nodep(), T_SUSPENDEE)) propagateFlags(&depVtx, T_SUSPENDEE);
        }
        अगर (dumpGraphLevel() >= 6) m_suspGraph.dumpDotFilePrefixed("timing_deps");

        // Propagate T_HAS_PROCESS
        क्रम (V3GraphVertex& vtx : m_procGraph.vertices()) {
            DepVtx& depVtx = static_cast<DepVtx&>(vtx);
            // Find processes that'll allocate VlProcess
            अगर (hasFlags(depVtx.nodep(), T_FORCES_PROC)) {
                propagateFlagsIf(&depVtx, T_FORCES_PROC, [&](स्थिर V3GraphEdge* e) -> bool {
                    वापस !hasFlags(static_cast<DepVtx*>(e->fromp())->nodep(), T_ALLOCS_PROC);
                });
            }
            // Mark nodes on paths between processes and statements that use VlProcess
            अगर (hasFlags(depVtx.nodep(), T_NEEDS_PROC)) {
                propagateFlagsIf(&depVtx, T_NEEDS_PROC, [&](स्थिर V3GraphEdge* e) -> bool {
                    वापस !hasFlags(static_cast<DepVtx*>(e->शीर्ष())->nodep(), T_ALLOCS_PROC);
                });
            }
        }
        क्रम (V3GraphVertex& vtx : m_procGraph.vertices()) {
            DepVtx& depVtx = static_cast<DepVtx&>(vtx);
            // Mark nodes that will be emitted with a VlProcess argument
            अगर (hasFlags(depVtx.nodep(), T_ALLOCS_PROC | T_FORCES_PROC)) {
                addFlags(depVtx.nodep(), T_HAS_PROC);
                propagateFlagsReversedIf(&depVtx, T_HAS_PROC, [&](स्थिर V3GraphEdge* e) -> bool {
                    वापस hasFlags(static_cast<DepVtx*>(e->fromp())->nodep(), T_NEEDS_PROC);
                });
            }
        }
        अगर (dumpGraphLevel() >= 6) m_procGraph.dumpDotFilePrefixed("proc_deps");
    }
    ~TimingSuspendableVisitor() override = शेष;
};

// ######################################################################
//  Transform nodes affected by timing

श्रेणी TimingControlVisitor final : खुला VNVisitor {
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
    AstNetlist* स्थिर m_netlistp;  // Root node
    Ast_शीर्ष_Scope* स्थिर m_शीर्ष_Scopep = m_netlistp->शीर्ष_Scopep();  // The AstTopScope
    AstScope* स्थिर m_scopeTopp = m_शीर्ष_Scopep->scopep();  // Scope at the top
    AstClass* m_classp = nullptr;  // Current class
    AstScope* m_scopep = nullptr;  // Current scope
    AstActive* m_activep = nullptr;  // Current active
    Ast_गाँठ* m_procp = nullptr;  // NodeProcedure/CFunc/Begin we're under
    bool m_hasProcess = false;  // True if current scope has a VlProcess handle available
    पूर्णांक m_forkCnt = 0;  // Number of forks inside a module
    bool m_underJumpBlock = false;  // True if we are inside of a jump-block
    bool m_underProcedure = false;  // True if we are under an always or initial
    bool m_underIfaceCFunc = false;  // True if we are under a CFunc owned by an interface scope
    bool m_hasStaticZeroDelay = false;  // True if we have a static #0 delay
    std::vector<File_पंक्ति*> m_unknownDelayFlps;  // Locations of AstDelay with non-constant value

    // Unique names
    V3UniqueNames m_dlyforkNames{"__Vdlyfork"};  // Names for temp AssignW vars
    V3UniqueNames m_intra_मूल्य_Names{"__Vintraval"};  // Intra assign delay value var names
    V3UniqueNames m_intra_सूचकांक_Names{"__Vintraidx"};  // Intra assign delay index var names
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
    Ast_Sen_वृक्ष* m_delaySensesp = nullptr;  // Domain to trigger if a delayed coroutine is resumed
    Ast_Sen_वृक्ष* m_dynamicSensesp = nullptr;  // Domain to trigger if a dynamic trigger is set

    // Other
    Sen_वृक्ष_Finder m_finder{m_netlistp};  // Sentree finder and uniquifier
    Sen_व्यंजक_Builder* m_sen_व्यंजक_Builderp = nullptr;  // Sens expression builder for current m_scope

    // Stats
    माप_प्रकार m_statZeroDelays = 0;  // Number of statically known #0 delays
    माप_प्रकार m_statConstDelays = 0;  // Number of statically known #const (non-zero) delays
    माप_प्रकार m_statVariableDelays = 0;  // Number of delays with value unknown at compile time

    // METHODS
    // Transform an assignment with an intra timing control into a timing control with the
    // assignment under it
    Ast_गाँठ* factorOutTimingControl(Ast_गाँठ_Assign* nodep) स्थिर {
        Ast_गाँठ* stmtp = nodep;
        Ast_गाँठ* स्थिर controlp = nodep->timingControlp();
        अगर (AstDelay* स्थिर delayp = VN_CAST(controlp, Delay)) {
            stmtp->replaceWith(delayp->unlinkFrBack());
            delayp->addStmtsp(stmtp);
            stmtp = delayp;
        } अन्यथा अगर (Ast_Sen_वृक्ष* स्थिर sentreep = VN_CAST(controlp, Sen_वृक्ष)) {
            AstEventControl* स्थिर eventControlp
                = नया AstEventControl{sentreep->fileline(), sentreep->unlinkFrBack(), nullptr};
            stmtp->replaceWith(eventControlp);
            eventControlp->addStmtsp(stmtp);
            stmtp = eventControlp;
        } अन्यथा अगर (AstBegin* स्थिर beginp = VN_CAST(controlp, Begin)) {
            // Begin from V3AssertPre
            stmtp->replaceWith(beginp->unlinkFrBack());
            beginp->addStmtsp(stmtp);
            stmtp = beginp;
        }
        वापस stmtp == nodep ? nullptr : stmtp;
    }
    अटल bool staticallyNonZeroDelay(स्थिर Ast_गाँठ_व्यंजक* valuep) {
        अगर (स्थिर AstConst* स्थिर constp = VN_CAST(valuep, Const)) वापस !constp->isZero();
        अगर (स्थिर AstCond* स्थिर condp = VN_CAST(valuep, Cond)) {
            वापस staticallyNonZeroDelay(condp->thenp())
                   && staticallyNonZeroDelay(condp->elsep());
        }
        अगर (स्थिर AstMul* स्थिर mulp = VN_CAST(valuep, Mul)) {
            स्थिर AstConst* स्थिर lhsConstp = VN_CAST(mulp->lhsp(), Const);
            स्थिर AstConst* स्थिर rhsConstp = VN_CAST(mulp->rhsp(), Const);
            अगर (lhsConstp && !lhsConstp->isZero()) वापस staticallyNonZeroDelay(mulp->rhsp());
            अगर (rhsConstp && !rhsConstp->isZero()) वापस staticallyNonZeroDelay(mulp->lhsp());
        }
        वापस false;
    }
    // Creates the global delay scheduler variable
    AstVarScope* getCreateDelayScheduler() {
        अगर (m_delaySchedp) वापस m_delaySchedp;
        स्वतः* स्थिर dlySchedDtp = नया AstBasicDType{
            m_scopeTopp->fileline(), VBasicDTypeKwd::DELAY_SCHEDULER, VSigning::UNSIGNED};
        m_netlistp->typeTablep()->addTypesp(dlySchedDtp);
        m_delaySchedp = m_शीर्ष_Scopep->create_अस्थायी("__VdlySched", dlySchedDtp);
        // Delay scheduler has to be accessible from top
        m_delaySchedp->varp()->sigPublic(true);
        m_netlistp->delaySchedulerp(m_delaySchedp->varp());
        वापस m_delaySchedp;
    }
    // Creates the delay sentree
    Ast_Sen_वृक्ष* get_Create_Delay_Sen_वृक्ष() {
        अगर (m_delaySensesp) वापस m_delaySensesp;
        File_पंक्ति* स्थिर flp = m_scopeTopp->fileline();
        स्वतः* स्थिर awaitingCurrentTimep
            = नया AstCMethodHard{flp, नया AstVarRef{flp, getCreateDelayScheduler(), VAccess::READ},
                                 VCMethod::SCHED_AWAITING_CURRENT_TIME};
        awaitingCurrentTimep->dtypeSetBit();
        m_delaySensesp
            = नया Ast_Sen_वृक्ष{flp, नया AstSenItem{flp, VEdgeType::ET_TRUE, awaitingCurrentTimep}};
        m_netlistp->शीर्ष_Scopep()->addSenTreesp(m_delaySensesp);
        वापस m_delaySensesp;
    }
    // Creates the global dynamic trigger scheduler variable
    AstVarScope* getCreateDynamicTriggerScheduler() {
        अगर (m_dynamicSchedp) वापस m_dynamicSchedp;
        स्वतः* स्थिर dynSchedDtp
            = नया AstBasicDType{m_scopeTopp->fileline(), VBasicDTypeKwd::DYNAMIC_TRIGGER_SCHEDULER,
                                VSigning::UNSIGNED};
        m_netlistp->typeTablep()->addTypesp(dynSchedDtp);
        m_dynamicSchedp = m_शीर्ष_Scopep->create_अस्थायी("__VdynSched", dynSchedDtp);
        वापस m_dynamicSchedp;
    }
    // Creates the dynamic trigger sentree
    Ast_Sen_वृक्ष* get_Create_Dynamic_Trigger_Sen_वृक्ष() {
        अगर (m_dynamicSensesp) वापस m_dynamicSensesp;
        File_पंक्ति* स्थिर flp = m_scopeTopp->fileline();
        स्वतः* स्थिर awaitingCurrentTimep = नया AstCMethodHard{
            flp, नया AstVarRef{flp, getCreateDynamicTriggerScheduler(), VAccess::READ},
            VCMethod::SCHED_EVALUATE};
        awaitingCurrentTimep->dtypeSetBit();
        m_dynamicSensesp
            = नया Ast_Sen_वृक्ष{flp, नया AstSenItem{flp, VEdgeType::ET_TRUE, awaitingCurrentTimep}};
        m_netlistp->शीर्ष_Scopep()->addSenTreesp(m_dynamicSensesp);
        वापस m_dynamicSensesp;
    }
    // Creates the event variable to trigger in NBA region
    AstEventControl* createNbaEventControl(File_पंक्ति* flp) {
        अगर (!m_netlistp->nbaEventp()) {
            स्वतः* स्थिर nbaEventDtp = नया AstBasicDType{m_scopeTopp->fileline(),
                                                        VBasicDTypeKwd::EVENT, VSigning::UNSIGNED};
            m_netlistp->typeTablep()->addTypesp(nbaEventDtp);
            m_netlistp->nbaEventp(m_शीर्ष_Scopep->create_अस्थायी("__VnbaEvent", nbaEventDtp));
            v3Global.setHasEvents();
        }
        वापस नया AstEventControl{
            flp,
            नया Ast_Sen_वृक्ष{
                flp, नया AstSenItem{flp, VEdgeType::ET_EVENT,
                                    नया AstVarRef{flp, m_netlistp->nbaEventp(), VAccess::READ}}},
            nullptr};
    }
    // Creates the variable that, if set, causes the NBA event to be triggered
    AstAssign* createNbaEventTriggerAssignment(File_पंक्ति* flp) {
        अगर (!m_netlistp->nbaEventTriggerp()) {
            m_netlistp->nbaEventTriggerp(m_शीर्ष_Scopep->create_अस्थायी("__VnbaEventTrigger", 1));
        }
        वापस नया AstAssign{flp,
                             नया AstVarRef{flp, m_netlistp->nbaEventTriggerp(), VAccess::WRITE},
                             नया AstConst{flp, AstConst::BitTrue{}}};
    }
    // Returns true if we are under a class or interface function, or the tree references locals.
    // These are cases where static, globally-evaluated triggers are not suitable.
    bool needDynamicTrigger(Ast_गाँठ* स्थिर nodep) स्थिर {
        वापस m_classp || m_underIfaceCFunc || nodep->exists([](Ast_गाँठ* स्थिर nodep) {
            अगर (Ast_गाँठ_Var_Ref* varp = VN_CAST(nodep, गाँठ_Var_Ref)) {
                वापस varp->varp()->isFuncLocal();
            }
            वापस !nodep->isPure();
        });
    }
    // Creates a trigger scheduler variable
    AstVarScope* getCreateTriggerSchedulerp(Ast_Sen_वृक्ष* स्थिर sentreep) {
        अगर (!sentreep->user1p()) {
            अगर (!m_trigSchedDtp) {
                m_trigSchedDtp
                    = नया AstBasicDType{m_scopeTopp->fileline(), VBasicDTypeKwd::TRIGGER_SCHEDULER,
                                        VSigning::UNSIGNED};
                m_netlistp->typeTablep()->addTypesp(m_trigSchedDtp);
            }
            AstVarScope* स्थिर trigSchedp
                = m_शीर्ष_Scopep->create_अस्थायी(m_trigSchedNames.लो(sentreep), m_trigSchedDtp);
            sentreep->user1p(trigSchedp);
        }
        वापस VN_AS(sentreep->user1p(), VarScope);
    }
    // Creates a string describing the sentree
    AstCExpr* createEventDescription(Ast_Sen_वृक्ष* स्थिर sentreep) स्थिर {
        अगर (!sentreep->user2p()) {
            std::stringstream ss;
            V3EmitV::verilog_For_वृक्ष(sentreep, ss);
            // possibly a multiline string
            // Names were hashed by V3Name, show the user the original text
            std::string comment = VName::dehash(ss.str());
            std::replace(comment.begin(), comment.end(), '\n', ' ');
            // Escape quotes, e.g. from string-indexed event array selects
            comment = '"' + V3OutFormatter::quoteNameControls(comment) + '"';
            AstCExpr* स्थिर commentp = नया AstCExpr{sentreep->fileline(), comment};
            commentp->dtypeSetString();
            sentreep->user2p(commentp);
            वापस commentp;
        }
        वापस VN_AS(sentreep->user2p(), CExpr)->clone_वृक्ष(false);
    }
    // Adds debug info to a hardcoded method call
    व्योम addDebugInfo(AstCMethodHard* स्थिर methodp) स्थिर {
        File_पंक्ति* स्थिर flp = methodp->fileline();
        स्थिर bool protectIds = v3Global.opt.protectIds();
        AstCExpr* स्थिर ap
            = नया AstCExpr{flp, protectIds ? "VL_UNKNOWN" : '"' + flp->filenameEsc() + '"'};
        ap->dtypeSetString();
        methodp->addPinsp(ap);
        AstCExpr* स्थिर bp = नया AstCExpr{flp, protectIds ? "0" : cvtToStr(flp->lineno())};
        bp->dtypeSetString();
        methodp->addPinsp(bp);
    }
    // Adds debug info to a trigSched.trigger() call
    व्योम addEventDebugInfo(AstCMethodHard* स्थिर methodp, Ast_Sen_वृक्ष* स्थिर sentreep) स्थिर {
        अगर (v3Global.opt.protectIds()) {
            File_पंक्ति* स्थिर flp = sentreep->fileline();
            AstCExpr* स्थिर descp = नया AstCExpr{flp, "VL_UNKNOWN"};
            descp->dtypeSetString();
            methodp->addPinsp(descp);
            addDebugInfo(methodp);
            वापस;
        }
        methodp->addPinsp(createEventDescription(sentreep));
        addDebugInfo(methodp);
    }
    // Adds process pointer to a hardcoded method call
    व्योम addProcessInfo(AstCMethodHard* स्थिर methodp) स्थिर {
        File_पंक्ति* स्थिर flp = methodp->fileline();
        AstCExpr* स्थिर ap
            = नया AstCExpr{flp, (m_procp && m_hasProcess) ? "vlProcess" : "nullptr"};
        methodp->addPinsp(ap);
    }
    // Creates the fork handle type and returns it
    AstBasicDType* getCreateForkSyncDTypep() {
        अगर (m_forkDtp) वापस m_forkDtp;
        m_forkDtp = नया AstBasicDType{m_scopeTopp->fileline(), VBasicDTypeKwd::FORK_SYNC,
                                      VSigning::UNSIGNED};
        m_netlistp->typeTablep()->addTypesp(m_forkDtp);
        वापस m_forkDtp;
    }
    // Move `nodep` into `AstCLocalScope`.
    AstCLocalScope* addCLocalScope(File_पंक्ति* स्थिर flp, Ast_गाँठ* स्थिर nodep) स्थिर {
        अगर (!nodep) वापस nullptr;
        VNRelinker handle;
        nodep->unlinkFrBack(&handle);
        AstCLocalScope* स्थिर cscopep = नया AstCLocalScope{flp, nodep};
        handle.relink(cscopep);
        वापस cscopep;
    }
    // Create a temp variable and optionally put it before the specified node (mark local if so)
    AstVarScope* create_अस्थायी(File_पंक्ति* स्थिर flp, स्थिर std::string& name,
                            Ast_गाँठ_DType* स्थिर dtypep, Ast_गाँठ* स्थिर जोड़ो_Beforep = nullptr) {
        AstVar* varp;
        अगर (जोड़ो_Beforep) {
            varp = नया AstVar{flp, VVarType::BLOCKTEMP, name, dtypep};
            varp->funcLocal(true);
            जोड़ो_Beforep->add_Here_This_As_अग्र(varp);
        } अन्यथा {
            varp = नया AstVar{flp, VVarType::MODULETEMP, name, dtypep};
            m_scopep->modp()->addStmtsp(varp);
        }
        AstVarScope* स्थिर vscp = नया AstVarScope{flp, m_scopep, varp};
        m_scopep->addVarsp(vscp);
        वापस vscp;
    }
    // Add a done() call on the fork sync
    व्योम addForkDone(AstBegin* स्थिर beginp, AstVarScope* स्थिर forkVscp) स्थिर {
        File_पंक्ति* स्थिर flp = beginp->fileline();
        स्वतः* स्थिर donep = नया AstCMethodHard{
            beginp->fileline(), नया AstVarRef{flp, forkVscp, VAccess::WRITE}, VCMethod::FORK_DONE};
        donep->dtypeSetVoid();
        addDebugInfo(donep);
        beginp->addStmtsp(donep->makeStmt());
    }
    अटल bool has_Disable_Queue_धकेलो_Self_Prefix(AstBegin* स्थिर beginp) {
        // LinkJump prepends disable-by-name registration as:
        //   __VprocessQueue_*.push_back(std::process::self())
        // V3LiftExpr lifts std::process::self() into an assignment to a temporary, then V3Task
        // lowers that assignment's function call into a leading comment and AstStmtExpr. Thus the
        // push_back is no longer necessarily the first statement. Scan across this generated
        // prefix for it, stopping at the fork-start sentinel or the branch body. Registrations for
        // nested forks live in sub-blocks and so are not in this statement list.
        क्रम (Ast_गाँठ* stmtp = beginp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            अगर (VN_IS(stmtp, Comment)) जारी;
            Ast_Stmt_व्यंजक* स्थिर stmtExprp = VN_CAST(stmtp, Stmt_व्यंजक);
            अगर (!stmtExprp) अवरोध;
            अगर (stmtExprp->is_Disable_Queue_धकेलो_Self_Stmt()) वापस true;
        }
        वापस false;
    }
    // Register a callback so killing a process-backed fork branch decrements the join counter
    व्योम addForkOnKill(AstBegin* स्थिर beginp, AstVarScope* स्थिर forkVscp) स्थिर {
        अगर (!beginp->needProcess() && !has_Disable_Queue_धकेलो_Self_Prefix(beginp)) वापस;
        File_पंक्ति* स्थिर flp = beginp->fileline();
        AstCMethodHard* स्थिर onKillp = नया AstCMethodHard{
            flp, नया AstVarRef{flp, forkVscp, VAccess::WRITE}, VCMethod::FORK_ON_KILL};
        onKillp->dtypeSetVoid();
        AstCExpr* स्थिर processp = नया AstCExpr{flp, "vlProcess"};
        processp
            ->dtypeSetVoid();  // Opaque process reference; type is irrelevant for hardcoded emit
        onKillp->addPinsp(processp);
        Ast_गाँठ_Stmt* स्थिर stmtp = onKillp->makeStmt();
        beginp->prependStmtsp(stmtp);
    }
    // Handle the 'join' part of a fork..join
    व्योम makeForkJoin(AstFork* स्थिर forkp) {
        // Create a fork sync var
        File_पंक्ति* स्थिर flp = forkp->fileline();
        // Insert the sync var directly before the fork
        Ast_गाँठ* स्थिर जोड़ो_Beforep = forkp;
        // Make sure all references to the fork var are contained within a single scope to prevent
        // moving statements accessing it in case of large functions splitting.
        addCLocalScope(flp, जोड़ो_Beforep);
        AstVarScope* forkVscp
            = create_अस्थायी(flp, forkp->name() + "__sync", getCreateForkSyncDTypep(), जोड़ो_Beforep);
        अचिन्हित join_गिनती = 0;  // Needed for join counter
        // Add a <fork sync>.done() to each begin
        क्रम (Ast_गाँठ* beginp = forkp->forksp(); beginp; beginp = beginp->nextp()) {
            addForkOnKill(VN_AS(beginp, Begin), forkVscp);
            addForkDone(VN_AS(beginp, Begin), forkVscp);
            join_गिनती++;
        }
        अगर (forkp->joinType().joinAny()) join_गिनती = 1;
        // Set the join counter
        स्वतः* स्थिर initp = नया AstCMethodHard{flp, नया AstVarRef{flp, forkVscp, VAccess::WRITE},
                                               VCMethod::FORK_INIT, नया AstConst{flp, join_गिनती}};
        initp->dtypeSetVoid();
        addProcessInfo(initp);
        forkp->add_Here_This_As_अग्र(initp->makeStmt());
        // Await the join at the end
        स्वतः* स्थिर joinp = नया AstCMethodHard{flp, नया AstVarRef{flp, forkVscp, VAccess::WRITE},
                                               VCMethod::FORK_JOIN};
        joinp->dtypeSetVoid();
        addProcessInfo(joinp);
        addDebugInfo(joinp);
        forkp->add_अग्र_Here(नया AstCAwait{flp, joinp});
    }

    // `procp` shall be a NodeProcedure/CFunc/Begin and within it vars from `varsp` will be placed.
    // `varsp` vector of vars which shall be localized.
    अटल व्योम localizeVars(Ast_गाँठ* स्थिर procp, स्थिर std::vector<AstVar*>& varsp) {
        UASSERT(procp, "procp is nullptr");
        AstVar* newVarsp = nullptr;
        क्रम (AstVar* स्थिर varp : varsp) {
            varp->funcLocal(true);
            newVarsp = Ast_गाँठ::add_अग्र_Null(newVarsp, varp->unlinkFrBack());
        }
        अगर (Ast_गाँठ_Procedure* स्थिर गाँठ_Procp = VN_CAST(procp, गाँठ_Procedure)) {
            गाँठ_Procp->prependStmtsp(newVarsp);
        } अन्यथा अगर (AstCFunc* स्थिर cfuncp = VN_CAST(procp, CFunc)) {
            cfuncp->prependVarsp(newVarsp);
        } अन्यथा अगर (AstBegin* स्थिर beginp = VN_CAST(procp, Begin)) {
            beginp->prependStmtsp(newVarsp);
        } अन्यथा {
            procp->v3fatalSrc(
                procp->prettyNameQ()
                << " is not of an expected type NodeProcedure/CFunc/Begin instead it is: "
                << procp->prettyTypeName());
        }
    }

    Ast_गाँठ_व्यंजक* delayToIntegral(AstDelay* nodep, Ast_गाँठ_व्यंजक* valuep) {
        स्थिर AstConst* स्थिर constp = VN_CAST(valuep, Const);
        // The fork start sentinel is already in ticks
        अगर (constp && constp->toUQuad() == std::numeric_limits<uint64_t>::max()) वापस valuep;
        वापस V3Timing::delayToTicks(valuep, nodep->timeunit());
    }

    // VISITORS
    व्योम visit(Ast_गाँठ_Module* nodep) override {
        UASSERT_OBJ(!m_classp, nodep, "Module or class under class");
        VL_RESTORER(m_classp);
        m_classp = VN_CAST(nodep, Class);
        VL_RESTORER(m_forkCnt);
        m_forkCnt = 0;
        iterateChildren(nodep);
    }
    व्योम visit(AstScope* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        Sen_व्यंजक_Builder sen_व्यंजक_Builder{m_scopep};
        {  // Restore m_senExprBuilderp before destroying senExprBuilder
            VL_RESTORER(m_sen_व्यंजक_Builderp);
            m_sen_व्यंजक_Builderp = &sen_व्यंजक_Builder;
            iterateChildren(nodep);
        }
    }
    व्योम visit(AstActive* nodep) override {
        VL_RESTORER(m_activep);
        m_activep = nodep;
        iterateChildren(nodep);
    }
    व्योम visit(Ast_गाँठ_Procedure* nodep) override {
        VL_RESTORER(m_procp);
        VL_RESTORER(m_hasProcess);
        m_procp = nodep;
        m_hasProcess = hasFlags(nodep, T_HAS_PROC);
        VL_RESTORER(m_underProcedure);
        m_underProcedure = true;
        iterateChildren(nodep);
        अगर (hasFlags(nodep, T_SUSPENDEE)) nodep->setSuspendable();
        अगर (hasFlags(nodep, T_HAS_PROC)) nodep->setNeedProcess();
    }
    व्योम visit(AstInitial* nodep) override {
        visit(static_cast<Ast_गाँठ_Procedure*>(nodep));
        अगर (nodep->needProcess() && !nodep->user1SetOnce()) {
            nodep->addStmtsp(
                नया AstCStmt{nodep->fileline(), "vlProcess->state(VlProcess::FINISHED);"});
        }
    }
    व्योम visit(AstJumpBlock* nodep) override {
        VL_RESTORER(m_underJumpBlock);
        m_underJumpBlock = true;
        visit(static_cast<Ast_गाँठ_Stmt*>(nodep));
    }
    व्योम visit(AstAlways* nodep) override {
        अगर (nodep->user1SetOnce()) वापस;
        VL_RESTORER(m_procp);
        VL_RESTORER(m_hasProcess);
        m_procp = nodep;
        m_hasProcess = hasFlags(nodep, T_HAS_PROC);
        VL_RESTORER(m_underProcedure);
        m_underProcedure = true;
        // Workaround for killing `always` processes (doing that is pretty much UB)
        // TODO: Disallow killing `always` at runtime (throw an error)
        // Skip for combinational blocks; if the body has timing controls
        // iterateChildren will add T_SUSPENDEE, otherwise it would spin.
        अगर (hasFlags(nodep, T_HAS_PROC)
            && !(m_activep && m_activep->sentreep() && m_activep->sentreep()->hasCombo()))
            addFlags(nodep, T_SUSPENDEE);

        iterateChildren(nodep);
        अगर (hasFlags(nodep, T_HAS_PROC)) nodep->setNeedProcess();
        अगर (!hasFlags(nodep, T_SUSPENDEE)) वापस;
        nodep->setSuspendable();
        File_पंक्ति* स्थिर flp = nodep->fileline();
        Ast_Sen_वृक्ष* स्थिर sentreep = m_activep->sentreep();
        अगर (sentreep->hasClocked()) {
            Ast_गाँठ* स्थिर bodysp = nodep->stmtsp()->unlink_Fr_Back_With_अग्र();
            स्वतः* स्थिर controlp = नया AstEventControl{flp, sentreep->clone_वृक्ष(false), bodysp};
            nodep->addStmtsp(controlp);
            iterate(controlp);
        }
        // Note: The 'while (true)' outer loop will be added in V3Sched
        स्वतः* स्थिर activep = नया AstActive{
            flp, "", नया Ast_Sen_वृक्ष{flp, नया AstSenItem{flp, AstSenItem::Initial{}}}};
        activep->sen_वृक्ष_Storep(activep->sentreep());
        activep->addStmtsp(nodep->unlinkFrBack());
        m_activep->add_अग्र_Here(activep);
    }
    व्योम visit(AstCFunc* nodep) override {
        VL_RESTORER(m_procp);
        VL_RESTORER(m_hasProcess);
        VL_RESTORER(m_underIfaceCFunc);
        m_procp = nodep;
        m_hasProcess = hasFlags(nodep, T_HAS_PROC);
        m_underIfaceCFunc = VN_IS(m_scopep->modp(), Iface);
        iterateChildren(nodep);
        अगर (hasFlags(nodep, T_HAS_PROC)) nodep->setNeedProcess();
        अगर (!(hasFlags(nodep, T_SUSPENDEE))) वापस;

        nodep->rtnType("VlCoroutine");
        // If in a class, create a shared pointer to 'this'
        अगर (m_classp) {
            AstCStmt* स्थिर cstmtp = नया AstCStmt{nodep->fileline(), "VL_KEEP_THIS;"};
            nodep->prependStmtsp(cstmtp);
        }
        अगर (nodep->dpiExportImpl()) {
            nodep->exists([](AstCAwait* स्थिर awaitp) -> bool {
                // A DPI-exported coroutine won't be able to block the calling code
                // Error on the await node; fall back to the function node
                awaitp->v3warn(E_UNSUPPORTED,
                               "Unsupported: Timing controls inside DPI-exported tasks");
                वापस true;
            });
        }
    }
    व्योम visit(Ast_गाँठ_CCall* nodep) override {
        AstCFunc* स्थिर funcp = nodep->funcp();

        // Skip automatic covergroup sampling calls
        अगर (funcp->isCovergroupSample()) {
            iterateChildren(nodep);
            वापस;
        }

        अगर (funcp->needProcess()) m_hasProcess = true;
        अगर (hasFlags(funcp, T_SUSPENDEE) && !nodep->user1SetOnce()) {  // If suspendable
            // Calls to suspendables are always void return type, hence parent must be StmtExpr
            Ast_Stmt_व्यंजक* स्थिर stmtp = VN_AS(nodep->backp(), Stmt_व्यंजक);
            stmtp->replaceWith(नया AstCAwait{nodep->fileline(), nodep->unlinkFrBack()});
            VL_DO_DANGLING(धकेलो_Deletep(stmtp), stmtp);
        }
        iterateChildren(nodep);
    }
    व्योम visit(AstDelay* nodep) override {
        UASSERT_OBJ(!nodep->isCycleDelay(), nodep,
                    "Cycle delays should have been handled in V3AssertPre");
        File_पंक्ति* स्थिर flp = nodep->fileline();

        Ast_गाँठ_व्यंजक* valuep = delayToIntegral(nodep, nodep->lhsp()->unlinkFrBack());

        // Statistics
        अगर (valuep->isZero()) {
            ++m_statZeroDelays;
        } अन्यथा अगर (VN_IS(valuep, Const)) {
            ++m_statConstDelays;
        } अन्यथा {
            ++m_statVariableDelays;
        }

        // Decide scheduling support for #0
        अगर (v3Global.opt.schedZeroDelay().isSetTrue()) {
            // User said to schedule for #0 support, nothing else to do
            v3Global.setUsesZeroDelay();
        } अन्यथा अगर (v3Global.opt.schedZeroDelay().isSetFalse()) {
            // User said to schedule without #0 support. Still warn if a static #0 delay exists
            अगर (valuep->isZero()) {
                nodep->v3warn(
                    ZERODLY,
                    "Static #0 delay exists, but '--no-sched-zero-delay' was given.\n"
                        << nodep->warnMore()  //
                        << "... Can proceed, but this will fail at runtime if executed.");
            }
        } अन्यथा {
            // User did not express preference, decide based on presence of delays
            अगर (valuep->isZero()) {
                // Statically known #0 delay exists, schedule for #0 support
                v3Global.setUsesZeroDelay();
                m_hasStaticZeroDelay = true;
                // Don't warn on variable delays, as no point
                m_unknownDelayFlps.सफा();
            } अन्यथा अगर (staticallyNonZeroDelay(valuep)) {
                // Delay is dynamic, but every statically known outcome is non-zero.
                // So we don't need #0 delay support and there should be no warning.
            } अन्यथा अगर (!VN_IS(valuep, Const)) {
                // Delay is not known at compiile time. Conservatively schedule for #0 support,
                // but warn if no static #0 delays used as performance might be improved
                v3Global.setUsesZeroDelay();
                अगर (!m_hasStaticZeroDelay) m_unknownDelayFlps.धकेलो_back(nodep->fileline());
            }
        }

        // Replace self with a 'co_await dlySched.delay(<valuep>)'
        AstCMethodHard* स्थिर delayMethodp = नया AstCMethodHard{
            flp, नया AstVarRef{flp, getCreateDelayScheduler(), VAccess::WRITE},
            VCMethod::SCHED_DELAY, valuep};
        delayMethodp->dtypeSetVoid();
        addProcessInfo(delayMethodp);
        addDebugInfo(delayMethodp);
        // Create the co_await
        Ast_गाँठ* स्थिर awaitp = नया AstCAwait{flp, delayMethodp, get_Create_Delay_Sen_वृक्ष()};
        // Relink child statements after the co_await
        अगर (Ast_गाँठ* स्थिर stmtsp = nodep->stmtsp()) {
            awaitp->add_अग्र(stmtsp->unlink_Fr_Back_With_अग्र());
        }
        nodep->replaceWith(awaitp);
        VL_DO_DANGLING(nodep->delete_वृक्ष(), nodep);
    }
    व्योम visit(AstEventControl* nodep) override {
        // Do not allow waiting on local named events, as they get enqueued for clearing, but can
        // go out of scope before that happens
        अगर (!nodep->sentreep()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Event control with implicit sensitivity (@*)"
                          " in this context; use an explicit sensitivity list instead");
            VL_DO_DANGLING(धकेलो_Deletep(nodep->unlinkFrBack()), nodep);
            वापस;
        }
        File_पंक्ति* स्थिर flp = nodep->fileline();
        // Relink child statements after the event control
        अगर (nodep->stmtsp()) nodep->add_अग्र_Here(nodep->stmtsp()->unlink_Fr_Back_With_अग्र());
        अगर (needDynamicTrigger(nodep->sentreep())) {
            // Create the trigger variable and init it with 0
            AstVarScope* स्थिर trigvscp
                = create_अस्थायी(flp, m_dynTrigNames.लो(nodep), nodep->findBitDType(), nodep);
            स्वतः* स्थिर initp = नया AstAssign{flp, नया AstVarRef{flp, trigvscp, VAccess::WRITE},
                                              नया AstConst{flp, AstConst::BitFalse{}}};
            // Await the eval step with the dynamic trigger scheduler. First, create the method
            // call
            स्वतः* स्थिर evalMethodp = नया AstCMethodHard{
                flp, नया AstVarRef{flp, getCreateDynamicTriggerScheduler(), VAccess::WRITE},
                VCMethod::SCHED_EVALUATION};
            evalMethodp->dtypeSetVoid();
            addProcessInfo(evalMethodp);
            स्वतः* स्थिर sentreep = nodep->sentreep();
            addEventDebugInfo(evalMethodp, sentreep);
            // Create the co_await
            AstCAwait* स्थिर awaitEvalp
                = नया AstCAwait{flp, evalMethodp, get_Create_Dynamic_Trigger_Sen_वृक्ष()};
            // Construct the sen expression for this sentree
            UASSERT_OBJ(m_sen_व्यंजक_Builderp, nodep, "No SenExprBuilder for this scope");
            स्वतः* स्थिर assignp = नया AstAssign{flp, नया AstVarRef{flp, trigvscp, VAccess::WRITE},
                                                m_sen_व्यंजक_Builderp->build(sentreep).first};
            // Get the SenExprBuilder results
            स्थिर Sen_व्यंजक_Builder::Results senResults = m_sen_व्यंजक_Builderp->getAndClearResults();
            localizeVars(m_procp, senResults.m_vars);
            // If post updates are destructive (e.g. clearFired on events), perform a
            // conservative pre-clear once before entering the wait loop so stale state from a
            // previous wait does not cause an immediate false-positive trigger.
            स्थिर bool hasDestructivePostUpdates = !senResults.m_destructivePostUpdates.empty();
            // Create the trigger eval loop, which will await the evaluation step and check the
            // trigger
            Ast_गाँठ_व्यंजक* स्थिर condp
                = नया AstLogNot{flp, नया AstVarRef{flp, trigvscp, VAccess::READ}};
            AstLoop* स्थिर loopp = नया AstLoop{flp};
            loopp->addStmtsp(नया AstLoopTest{flp, loopp, condp});
            loopp->addStmtsp(awaitEvalp);
            // Put pre updates before the trigger check and assignment
            क्रम (Ast_गाँठ_Stmt* स्थिर stmtp : senResults.m_preUpdates) loopp->addStmtsp(stmtp);
            // Then the trigger check and assignment
            loopp->addStmtsp(assignp);
            // Let the dynamic trigger scheduler know if this trigger was set
            // If it was, a call to the scheduler's evaluate() will return true
            AstCMethodHard* स्थिर anyTriggeredMethodp = नया AstCMethodHard{
                flp, नया AstVarRef{flp, getCreateDynamicTriggerScheduler(), VAccess::WRITE},
                VCMethod::SCHED_ANY_TRIGGERED, नया AstVarRef{flp, trigvscp, VAccess::READ}};
            anyTriggeredMethodp->dtypeSetVoid();
            loopp->addStmtsp(anyTriggeredMethodp->makeStmt());
            // If the post update is destructive (e.g. event vars are cleared), create an await for
            // the post update step
            अगर (hasDestructivePostUpdates) {
                AstCAwait* स्थिर awaitPostUpdatep = awaitEvalp->clone_वृक्ष(false);
                VN_AS(awaitPostUpdatep->exprp(), CMethodHard)->method(VCMethod::SCHED_POST_UPDATE);
                loopp->addStmtsp(awaitPostUpdatep);
            }
            // Put the post updates at the end of the loop
            क्रम (Ast_गाँठ_Stmt* स्थिर stmtp : senResults.m_postUpdates) loopp->addStmtsp(stmtp);
            // Finally, await the resumption step in 'act'
            AstCAwait* स्थिर awaitResumep = awaitEvalp->clone_वृक्ष(false);
            VN_AS(awaitResumep->exprp(), CMethodHard)->method(VCMethod::SCHED_RESUMPTION);
            Ast_गाँठ::add_अग्र<Ast_गाँठ_Stmt, Ast_गाँठ_Stmt>(loopp, awaitResumep);
            // Replace the event control with one explicit stmt chain:
            //   init -> [inits] -> [optional destructive pre-clears] -> loop -> awaitResumption
            Ast_गाँठ_Stmt* chainp = nullptr;
            chainp = Ast_गाँठ::add_अग्र_Null(chainp, initp);
            क्रम (Ast_गाँठ_Stmt* स्थिर stmtp : senResults.m_inits) {
                chainp = Ast_गाँठ::add_अग्र_Null(chainp, stmtp);
            }
            अगर (hasDestructivePostUpdates) {
                क्रम (Ast_गाँठ_Stmt* स्थिर stmtp : senResults.m_destructivePostUpdates) {
                    chainp = Ast_गाँठ::add_अग्र_Null(chainp, stmtp->clone_वृक्ष(false));
                }
            }
            chainp = Ast_गाँठ::add_अग्र_Null(chainp, loopp);
            nodep->replaceWith(chainp);
        } अन्यथा {
            स्वतः* स्थिर sentreep = m_finder.get_Sen_वृक्ष(nodep->sentreep());
            nodep->sentreep()->unlinkFrBack()->delete_वृक्ष();
            // Get this sentree's trigger scheduler
            // Replace self with a 'co_await trigSched.trigger()'
            स्वतः* स्थिर triggerMethodp = नया AstCMethodHard{
                flp, नया AstVarRef{flp, getCreateTriggerSchedulerp(sentreep), VAccess::WRITE},
                VCMethod::SCHED_TRIGGER};
            triggerMethodp->dtypeSetVoid();
            // If it should be committed immediately, pass true, otherwise false
            triggerMethodp->addPinsp(nodep->user2() ? नया AstConst{flp, AstConst::BitTrue{}}
                                                    : नया AstConst{flp, AstConst::BitFalse{}});
            addProcessInfo(triggerMethodp);
            addEventDebugInfo(triggerMethodp, sentreep);
            // Create the co_await
            AstCAwait* स्थिर awaitp = नया AstCAwait{flp, triggerMethodp, sentreep};
            nodep->replaceWith(awaitp);
        }
        VL_DO_DANGLING(nodep->delete_वृक्ष(), nodep);
    }
    व्योम visit(Ast_गाँठ_Assign* nodep) override {
        // Only process once to avoid infinite loops (due to the net delay)
        अगर (nodep->user1SetOnce()) वापस;
        UINFO(9, "control-visit " << nodep);
        File_पंक्ति* स्थिर flp = nodep->fileline();
        Ast_गाँठ* controlp = factorOutTimingControl(nodep);
        स्थिर bool inAssignDly = VN_IS(nodep, AssignDly);
        // Handle the intra assignment timing control
        // Transform if:
        // * there's a timing control in the assignment
        // * the assignment is an AssignDly and it's in a non-inlined function
        अगर (!controlp && (!inAssignDly || m_underProcedure)) {
            iterateChildren(nodep);
            वापस;
        }
        // The timing control, if any, which is put in a fork for an NBA
        Ast_गाँठ* स्थिर timingp = controlp;
        // Insert new vars before the timing control if we're in a function; in a process we can't
        // do that, as after V3Begin only modules and functions declare variables. These
        // intra-assignment vars will later be passed to forked processes by value.
        Ast_गाँठ* जोड़ो_Beforep = m_underProcedure ? nullptr : controlp;
        // Special case for NBA
        अगर (inAssignDly) {
            // Put it in a fork so it doesn't block
            // Could already be the only thing directly under a fork, reuse that if possible
            AstFork* forkp = !nodep->nextp() ? VN_CAST(nodep->firstAbovep(), Fork) : nullptr;
            अगर (!forkp) forkp = नया AstFork{flp, VJoinType::JOIN_NONE};
            अगर (!m_underProcedure) {
                // If it's in a function, it won't be handled by V3Delayed
                // Put it behind an additional named event that gets triggered in the NBA region
                AstEventControl* स्थिर nbaEventControlp = createNbaEventControl(flp);
                AstAssign* स्थिर trigAssignp = createNbaEventTriggerAssignment(flp);
                nodep->replaceWith(trigAssignp);
                trigAssignp->add_अग्र_Here(nbaEventControlp);
                nbaEventControlp->addStmtsp(nodep);
                जोड़ो_Beforep = forkp;
                अगर (!controlp) controlp = nbaEventControlp;
            }
            controlp->replaceWith(forkp);
            AstBegin* beginp = VN_CAST(controlp, Begin);
            अगर (!beginp) beginp = नया AstBegin{nodep->fileline(), "", controlp, false};
            forkp->addForksp(beginp);
            addFlags(beginp, T_NBA_UPDATE);
            controlp = forkp;
        }
        UASSERT_OBJ(nodep, controlp, "Assignment should have timing control");
        // Move `insertBeforep` into `AstCLocalScope` if necessary to avoid jumping over
        // a variable initialization that would be inserted before `insertBeforep`. All
        // access to this variable should be contained within returned `AstCLocalScope`.
        अगर (m_underJumpBlock) addCLocalScope(flp, जोड़ो_Beforep);
        // Function for replacing values with intermediate variables
        स्थिर स्वतः replaceWithIntermediate = [&](Ast_गाँठ_व्यंजक* स्थिर valuep,
                                                 स्थिर std::string& name) {
            AstVarScope* स्थिर newvscp = create_अस्थायी(flp, name, valuep->dtypep(), जोड़ो_Beforep);
            // A pending update keeps its own value, even if its process schedules more
            अगर (inAssignDly && !जोड़ो_Beforep) newvscp->fork_By_मूल्य(true);
            valuep->replaceWith(नया AstVarRef{flp, newvscp, VAccess::READ});
            controlp->add_Here_This_As_अग्र(
                नया AstAssign{flp, नया AstVarRef{flp, newvscp, VAccess::WRITE}, valuep});
        };
        // NBAs with delays evaluate the indices selecting the target now (IEEE 1800-2023 10.4.2),
        // in order, each as a whole, also the selects and function calls in it. So follow the
        // selects of the target up from the variable, not into their indices. As an example, this
        // transforms 'x[a[i]][f(j)] <= #1 y' into 't0 = a[i]; t1 = f(j); x[t0][t1] <= #1 y'.
        // V3Const has split NBAs to concatenations, also streamed, into NBAs to each part.
        अगर (inAssignDly) {
            क्रम (Ast_गाँठ* exprp = nodep->lhsp()->baseFromp(true); exprp != nodep;
                 exprp = exprp->firstAbovep()) {
                अगर (AstSel* स्थिर selp = VN_CAST(exprp, Sel)) {
                    // widthp should be const
                    अगर (!VN_IS(selp->lsbp(), Const)) {
                        replaceWithIntermediate(selp->lsbp(), m_intraLsbNames.लो(nodep));
                    }
                } अन्यथा अगर (Ast_गाँठ_Sel* स्थिर selp = VN_CAST(exprp, गाँठ_Sel)) {
                    अगर (!VN_IS(selp->bitp(), Const)) {
                        replaceWithIntermediate(selp->bitp(), m_intra_सूचकांक_Names.लो(nodep));
                    }
                }
            }
        }
        // Replace the RHS with an intermediate value var
        replaceWithIntermediate(nodep->rhsp(), m_intra_मूल्य_Names.लो(nodep));
        अगर (!inAssignDly) वापस;
        // The process executing the NBA evaluates its delay (IEEE 1800-2023 4.9.4), not the forked
        // process, if it has side effects, e.g. calling a function that checks its process. A pure
        // one is evaluated as the forked process starts, at once.
        अगर (AstDelay* स्थिर delayp = VN_CAST(timingp, Delay)) {
            अगर (!delayp->lhsp()->isPure()) {
                replaceWithIntermediate(delayp->lhsp(), m_intra_मूल्य_Names.लो(nodep));
            }
        }
    }
    व्योम visit(AstFireEvent* nodep) override {
        // V3Delayed handles '->>' of a variable in a process. Like NBAs in non-inlined functions,
        // trigger other events from a fork awaiting the NBA region.
        अगर (!nodep->isDelayed() || (m_underProcedure && VN_IS(nodep->operandp(), VarRef))) {
            iterateChildren(nodep);
            वापस;
        }
        File_पंक्ति* स्थिर flp = nodep->fileline();
        AstAssign* स्थिर trigAssignp = createNbaEventTriggerAssignment(flp);
        nodep->replaceWith(trigAssignp);
        AstFork* स्थिर forkp = नया AstFork{flp, VJoinType::JOIN_NONE};
        trigAssignp->add_अग्र_Here(forkp);
        अगर (m_underJumpBlock) addCLocalScope(flp, forkp);
        // The triggered event is the one referenced now, so evaluate handles and indices now
        Ast_गाँठ_व्यंजक* स्थिर eventp = nodep->operandp()->unlinkFrBack();
        स्थिर स्वतः evalNow = [&](Ast_गाँठ_व्यंजक* स्थिर valuep, V3UniqueNames& names) {
            // Like for an NBA, a process uses a module variable, passed to the fork by value
            AstVarScope* स्थिर vscp = create_अस्थायी(flp, names.लो(nodep), valuep->dtypep(),
                                                 m_underProcedure ? nullptr : forkp);
            अगर (m_underProcedure) vscp->fork_By_मूल्य(true);
            valuep->replaceWith(नया AstVarRef{flp, vscp, VAccess::READ});
            // Unlike an event, a handle selecting one is only read
            valuep->foreach([](Ast_गाँठ* स्थिर np) {
                अगर (Ast_गाँठ_Var_Ref* स्थिर refp = VN_CAST(np, गाँठ_Var_Ref)) {
                    refp->access(VAccess::READ);
                } अन्यथा अगर (AstMemberSel* स्थिर selp = VN_CAST(np, MemberSel)) {
                    selp->access(VAccess::READ);
                }
            });
            forkp->add_Here_This_As_अग्र(
                नया AstAssign{flp, नया AstVarRef{flp, vscp, VAccess::WRITE}, valuep});
        };
        Ast_गाँठ_व्यंजक* refp = eventp;
        जबतक (Ast_गाँठ_Sel* स्थिर selp = VN_CAST(refp, गाँठ_Sel)) {
            अगर (!VN_IS(selp->bitp(), Const)) evalNow(selp->bitp(), m_intra_सूचकांक_Names);
            refp = selp->fromp();
        }
        // Handle of a class (or virtual interface)
        अगर (AstMemberSel* स्थिर selp = VN_CAST(refp, MemberSel)) {
            evalNow(selp->fromp(), m_intra_मूल्य_Names);
        }
        AstEventControl* स्थिर controlp = createNbaEventControl(flp);
        controlp->addStmtsp(नया AstFireEvent{flp, eventp, false});
        AstBegin* स्थिर beginp = नया AstBegin{flp, "", controlp, false};
        addFlags(beginp, T_NBA_UPDATE);
        forkp->addForksp(beginp);
        VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
    }
    व्योम visit(AstAssignW* nodep) override {
        File_पंक्ति* स्थिर flp = nodep->fileline();
        // Get the net delay unless this assignment was created for handling the net delay (user1)
        AstDelay* स्थिर netDelayp = nodep->user1() ? nullptr : nodep->getLhsNetDelay();
        अगर (netDelayp) {
            अगर (nodep->timingControlp()) {
                // If this assignment has a delay, create another one to handle the net delay
                AstVarScope* स्थिर newvscp = m_assignWTmps.make(flp, m_scopep, nodep->dtypep());
                AstAssignW* assignp = नया AstAssignW{
                    nodep->fileline(), nodep->lhsp()->unlinkFrBack(),
                    नया AstVarRef{flp, newvscp, VAccess::READ}, netDelayp->clone_वृक्ष(false)};
                assignp->user1(true);
                nodep->add_अग्र_Here(assignp);
                nodep->lhsp(नया AstVarRef{flp, newvscp, VAccess::WRITE});
            } अन्यथा {
                // Else just use this one with the net delay
                nodep->timingControlp(netDelayp->clone_वृक्ष(false));
            }
        }
        अगर (!nodep->timingControlp()) वापस;
        // There will be some circular logic here, suppress the warning for newly created vars
        flp->modifyWarnOff(V3ErrorCode::UNOPTFLAT, true);
        // Also suppress the warning for the LHS var, for cases like `assign #1 clk = ~clk;`
        // TODO: Restore the warning for other, non-delayed drivers
        nodep->lhsp()->foreach([](AstVarRef* refp) {
            अगर (refp->access().isWriteOrRW()) {
                refp->varp()->fileline()->modifyWarnOff(V3ErrorCode::UNOPTFLAT, true);
            }
        });
        // Should be under an always
        AstAlways* स्थिर alwaysp = VN_AS(m_procp, Always);
        // Convert it to an Assign; the new assign with intra delay will be handled by
        // visit(AstNodeAssign*)
        Ast_गाँठ_व्यंजक* स्थिर lhs1p = nodep->lhsp()->unlinkFrBack();
        Ast_गाँठ_व्यंजक* स्थिर rhs1p = nodep->rhsp()->unlinkFrBack();
        Ast_गाँठ* स्थिर controlp = nodep->timingControlp()->unlinkFrBack();
        अगर (AstDelay* स्थिर delayp = VN_CAST(controlp, Delay)) {
            अगर (Ast_गाँठ_व्यंजक* fallDelayp = delayp->fallDelay()) {
                fallDelayp = fallDelayp->unlinkFrBack();
                Ast_गाँठ_व्यंजक* lhsp = delayp->lhsp()->unlinkFrBack();
                // Use fall only for an all-zero value, rise otherwise.
                lhsp = delayToIntegral(delayp, lhsp);
                fallDelayp = delayToIntegral(delayp, fallDelayp);
                delayp->lhsp(
                    नया AstCond{flp, नया AstEq{flp, rhs1p->clone_वृक्ष(false), नया AstConst{flp, 0}},
                                fallDelayp, lhsp});
            }
        }
        AstAssign* स्थिर assignp = नया AstAssign{nodep->fileline(), lhs1p, rhs1p, controlp};
        // Put the assignment in a fork..join_none.
        AstFork* स्थिर forkp = नया AstFork{flp, VJoinType::JOIN_NONE};
        nodep->replaceWith(forkp);
        VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
        AstBegin* स्थिर beginp = नया AstBegin{flp, "", assignp, false};
        forkp->addForksp(beginp);
        visit(forkp);  // Visit now as we need to do some post-processing
        // IEEE 1800-2023 10.3.3 - if the RHS value differs from the currently scheduled value to
        // be assigned, the currently scheduled assignment is descheduled. To keep track if an
        // assignment should be descheduled, each scheduled assignment event has a 'generation',
        // and if at assignment time its generation differs from the current generation, it won't
        // be performed
        AstAssign* स्थिर preAssignp = VN_AS(beginp->stmtsp(), Assign);
        UASSERT_OBJ(preAssignp, alwaysp, "Pre-assign should be there from visit(AstFork)");
        AstAssign* स्थिर postAssignp = VN_AS(preAssignp->nextp()->nextp(), Assign);
        UASSERT_OBJ(postAssignp, alwaysp, "Post-assign should be there from visit(AstFork)");
        // Increment generation and copy it to a local
        AstVarScope* स्थिर generationVarp
            = m_assignWGens.make(flp, m_scopep, alwaysp->findUInt64DType());
        AstVarScope* स्थिर genLocalVarp
            = create_अस्थायी(flp, generationVarp->varp()->name() + "__local",
                         alwaysp->findUInt64DType(), preAssignp);
        preAssignp->add_Here_This_As_अग्र(
            नया AstAssign{flp, नया AstVarRef{flp, generationVarp, VAccess::WRITE},
                          नया AstAdd{flp, नया AstVarRef{flp, generationVarp, VAccess::READ},
                                     नया AstConst{flp, 1}}});
        preAssignp->add_Here_This_As_अग्र(
            नया AstAssign{flp, नया AstVarRef{flp, genLocalVarp, VAccess::WRITE},
                          नया AstVarRef{flp, generationVarp, VAccess::READ}});
        // If the current generation is same as the one saved in the local var, assign
        beginp->addStmtsp(
            नया AstIf{flp,
                      नया AstEq{flp, नया AstVarRef{flp, generationVarp, VAccess::READ},
                                नया AstVarRef{flp, genLocalVarp, VAccess::READ}},
                      postAssignp->unlinkFrBack()});
        // Save scheduled RHS value before delay
        AstVarScope* स्थिर tmpVarp
            = m_assignWTmps.make(flp, m_scopep, preAssignp->rhsp()->dtypep());
        AstVarRef* स्थिर tmpAssignRhsp = VN_AS(preAssignp->lhsp(), VarRef)->clone_वृक्ष(false);
        tmpAssignRhsp->access(VAccess::WRITE);
        preAssignp->add_अग्र_Here(
            नया AstAssign{flp, नया AstVarRef{flp, tmpVarp, VAccess::WRITE}, tmpAssignRhsp});
        // If the RHS is different from the currently scheduled value, schedule the new assignment
        // The generation will increase, effectively 'descheduling' the previous assignment.
        Ast_गाँठ_व्यंजक* स्थिर didNotInitp
            = नया AstLogNot{flp, नया AstCExpr{flp, "vlSymsp->__Vm_didInit", 1}};
        AstVarRef* स्थिर firstIterp
            = नया AstVarRef{flp, m_netlistp->stlFirstIterationp(), VAccess::READ};
        Ast_गाँठ_व्यंजक* स्थिर schedCondp
            = नया AstLogOr{flp,
                           नया AstNeq{flp, preAssignp->rhsp()->clone_वृक्ष(false),
                                      नया AstVarRef{flp, tmpVarp, VAccess::READ}},
                           नया AstLogAnd{flp, didNotInitp, firstIterp}};
        alwaysp->addStmtsp(नया AstIf{flp, schedCondp, forkp->unlinkFrBack()});
    }
    व्योम visit(AstDisableFork* nodep) override {
        अगर (m_hasProcess) वापस;
        // never reached by any process; remove to avoid compilation error
        VL_DO_DANGLING(nodep->unlinkFrBack()->delete_वृक्ष(), nodep);
    }
    व्योम visit(AstWaitFork* nodep) override {
        अगर (m_hasProcess) {
            File_पंक्ति* स्थिर flp = nodep->fileline();
            AstCExpr* स्थिर exprp = नया AstCExpr{flp, "vlProcess->completedFork()", 1};
            AstWait* स्थिर waitp = नया AstWait{flp, exprp, nullptr};
            nodep->replaceWith(waitp);
        } अन्यथा {
            // never reached by any process; remove to avoid compilation error
            nodep->unlinkFrBack();
        }
        VL_DO_DANGLING(nodep->delete_वृक्ष(), nodep);
    }
    व्योम visit(AstWait* nodep) override {
        // Wait on changed events related to the vars in the wait statement
        UINFO(9, "control-visit " << nodep);
        File_पंक्ति* स्थिर flp = nodep->fileline();
        Ast_गाँठ* स्थिर stmtsp = nodep->stmtsp();
        अगर (stmtsp) stmtsp->unlink_Fr_Back_With_अग्र();
        Ast_गाँठ_व्यंजक* स्थिर condp = V3Const::constifyEdit(nodep->condp()->unlinkFrBack());
        स्वतः* स्थिर constp = VN_CAST(condp, Const);
        अगर (constp) {
            अगर (constp->isZero()) {
                // We have to await forever instead of simply returning in case we're deep in a
                // callstack
                AstCMethodHard* स्थिर foreverMethodp = नया AstCMethodHard{
                    flp, नया AstVarRef{flp, getCreateDelayScheduler(), VAccess::WRITE},
                    VCMethod::SCHED_WAIT_FOREVER};
                foreverMethodp->dtypeSetVoid();
                addProcessInfo(foreverMethodp);
                addDebugInfo(foreverMethodp);
                AstCAwait* स्थिर awaitp = नया AstCAwait{flp, foreverMethodp};
                nodep->replaceWith(awaitp);
                अगर (stmtsp) VL_DO_DANGLING(stmtsp->delete_वृक्ष(), stmtsp);
                VL_DO_DANGLING(condp->delete_वृक्ष(), condp);
            } अन्यथा {
                nodep->v3fatalSrc("constant wait should have been removed in "
                                  "TimingSuspendableVisitor::visit(AstWait)");
            }
        } अन्यथा अगर (needDynamicTrigger(condp)) {
            // No point in making a sentree, just use the expression as sensitivity
            // Put the event control in an if so we only wait if the condition isn't met already
            स्वतः* स्थिर ifp = नया AstIf{
                flp, नया AstLogNot{flp, condp},
                नया AstEventControl{flp,
                                    नया Ast_Sen_वृक्ष{flp, नया AstSenItem{flp, VEdgeType::ET_TRUE,
                                                                       condp->clone_वृक्ष(false)}},
                                    nullptr}};
            अगर (stmtsp) Ast_गाँठ::add_अग्र<Ast_गाँठ, Ast_गाँठ>(ifp, stmtsp);
            nodep->replaceWith(ifp);
        } अन्यथा {
            // We are using a global sentree, so we cannot use ET_TRUE, as that could lead to the
            // active region never converging. Because of this, we need to use ET_CHANGED in a
            // loop.
            AstEventControl* स्थिर controlp = नया AstEventControl{
                flp,
                नया Ast_Sen_वृक्ष{
                    flp, नया AstSenItem{flp, VEdgeType::ET_CHANGED, condp->clone_वृक्ष(false)}},
                nullptr};
            controlp->user2(true);  // Commit immediately
            AstLoop* स्थिर loopp = नया AstLoop{flp};
            loopp->addStmtsp(नया AstLoopTest{flp, loopp, नया AstLogNot{flp, condp}});
            loopp->addStmtsp(controlp);
            अगर (stmtsp) Ast_गाँठ::add_अग्र<Ast_गाँठ, Ast_गाँठ>(loopp, stmtsp);
            nodep->replaceWith(loopp);
        }
        VL_DO_DANGLING(nodep->delete_वृक्ष(), nodep);
    }
    व्योम visit(AstBegin* nodep) override {
        VL_RESTORER(m_procp);
        VL_RESTORER(m_hasProcess);
        m_hasProcess |= hasFlags(nodep, T_HAS_PROC);
        // A pending NBA update is not a subprocess of the process that scheduled it, so it is
        // unaffected by 'disable fork' and 'wait fork' (IEEE 1800-2023 9.6.1, 9.6.3)
        अगर (hasFlags(nodep, T_NBA_UPDATE)) m_hasProcess = false;
        m_procp = nodep;
        अगर (m_hasProcess) nodep->setNeedProcess();
        iterateChildren(nodep);
    }
    व्योम visit(AstFork* nodep) override {
        अगर (nodep->user1SetOnce()) वापस;
        UINFO(9, "control-visit " << nodep);
        v3Global.setUsesTiming();

        // Create a unique name for this fork
        nodep->name("__Vfork_" + std::to_string(++m_forkCnt));

        // TODO: Should process nodep->stmtsp() in case an earlier pass
        //       inserted something there, but as of today we don't need to.

        // Process and name each fork
        माप_प्रकार idx = 0;  // Index for naming begins
        क्रम (AstBegin *itemp = nodep->forksp(), *nextp; itemp; itemp = nextp) {
            nextp = VN_AS(itemp->nextp(), Begin);
            iterate(itemp);
            // Note: Even if we do not find any awaits, we cannot simply inline
            // the process here, as new awaits could be added later.

            // Name the begin (later the name will be used for a new function)
            itemp->name(nodep->name() + "__" + std::to_string(idx++));
            अगर (itemp->needProcess()) {
                itemp->addStmtsp(
                    नया AstCStmt{itemp->fileline(), "vlProcess->state(VlProcess::FINISHED);"});
            }
        }
        अगर (!nodep->joinType().joinNone()) makeForkJoin(nodep);
    }

    //--------------------
    व्योम visit(AstVar*) override {}  // Accelerate
    व्योम visit(Ast_गाँठ* nodep) override { iterateChildren(nodep); }

खुला:
    // CONSTRUCTORS
    explicit TimingControlVisitor(AstNetlist* nodep)
        : m_netlistp{nodep} {
        iterate(nodep);

        // If there is no static #0 in the design, but an unknown delay was found,
        // and the user did not specify preference, then warn on all unknown delays
        // as we will be assuming they can be #0, which can cause performance degradation.
        क्रम (File_पंक्ति* स्थिर flp : m_unknownDelayFlps) {
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

Ast_गाँठ_व्यंजक* V3Timing::delayToTicks(Ast_गाँठ_व्यंजक* valuep, VTimescale timeunit) {
    File_पंक्ति* स्थिर flp = valuep->fileline();
    अगर (स्थिर AstConst* स्थिर constp = VN_CAST(valuep, Const)) {
        अगर (constp->अंक().is1Step()) {
            // A step is one tick of the global time precision (IEEE 1800-2023 3.14.3)
            VL_DO_DANGLING(valuep->delete_वृक्ष(), valuep);
            AstConst* स्थिर tickp = नया AstConst{flp, AstConst::Unsized64{}, 1};
            tickp->dtypeSetBitSized(64, VSigning::UNSIGNED);
            वापस tickp;
        }
        अगर (constp->isZero()) वापस valuep;
    }
    UASSERT_OBJ(!timeunit.isNone(), valuep, "timeunit must be set");
    स्थिर द्विगुन timescaleFactorD
        = std::घात(10.0, timeunit.powerOfTen() - v3Global.rootp()->timeprecision().powerOfTen());
    अगर (valuep->dtypep()->skipRefp()->isDouble()) {
        AstConst* स्थिर tsfp = नया AstConst{flp, AstConst::RealDouble{}, timescaleFactorD};
        valuep = नया AstMulD{flp, valuep, tsfp};
        valuep = नया AstRToIRoundS{flp, valuep};
        valuep->dtypeSetBitSized(64, VSigning::UNSIGNED);
    } अन्यथा {
        valuep->dtypeSetBitSized(64, VSigning::UNSIGNED);
        स्थिर uint64_t timescaleFactorU = static_cast<uint64_t>(timescaleFactorD);
        AstConst* स्थिर tsfp = नया AstConst{flp, AstConst::Unsized64{}, timescaleFactorU};
        valuep = नया AstMul{flp, valuep, tsfp};
    }
    वापस V3Const::constifyEdit(valuep);  // Simplify
}

व्योम V3Timing::timingAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        स्थिर VNUser1InUse m_user1InUse;
        स्थिर VNUser2InUse m_user2InUse;
        { TimingSuspendableVisitor{nodep}; }
        अगर (v3Global.usesTiming()) TimingControlVisitor{nodep};
    }
    V3Global::dump_Check_Global_वृक्ष("timing", 0, dump_वृक्ष_Either_Level() >= 3);
}
