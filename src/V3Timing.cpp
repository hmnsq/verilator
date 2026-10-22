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

#अन्तर्भाव "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#अन्तर्भाव "V3Timing.h"

#अन्तर्भाव "V3Const.h"
#अन्तर्भाव "V3EmitV.h"
#अन्तर्भाव "V3File.h"
#अन्तर्भाव "V3Global.h"
#अन्तर्भाव "V3Graph.h"
#अन्तर्भाव "V3MemberMap.h"
#अन्तर्भाव "V3SenExprBuilder.h"
#अन्तर्भाव "V3SenTree.h"
#अन्तर्भाव "V3SharedTmps.h"
#अन्तर्भाव "V3Stats.h"
#अन्तर्भाव "V3UniqueNames.h"

#अन्तर्भाव <limits>
#अन्तर्भाव <queue>

VL_DEFINE_DEBUG_FUNCTIONS;

// ######################################################################

गणना गाँठ_Flag : uint8_t {
    T_SUSPENDEE = 1 << 0,  // Suspendable (due to dependence on another suspendable)
    T_SUSPENDER = 1 << 1,  // Suspendable (has timing control)
    T_ALLOCS_PROC = 1 << 2,  // Can allocate VlProcess
    T_FORCES_PROC = 1 << 3,  // Forces VlProcess allocation
    T_NEEDS_PROC = 1 << 4,  // Needs access to VlProcess if it's allocated
    T_HAS_PROC = 1 << 5,  // Has VlProcess argument in the signature
    T_NBA_UPDATE = 1 << 6,  // Fork branch of a pending NBA update, which is not a subprocess
};

गणना ForkType : uint8_t {
    F_NONE = 0,  // Not under a fork
    F_MIGHT_SUSPEND = 1 << 0,  // Fork might suspend the execution of current process
    F_MIGHT_NEED_PROC = 1 << 1,  // Fork might need a process (any fork really)
};

गणना PropagationType : uint8_t {
    P_CALL = 1,  // Propagation through call to a function/task/method
    P_FORK = 2,  // Propagation due to fork's behavior
    P_SIGNATURE = 3,  // Propagation required to maintain C++ function's signature requirements
};

// Add timing flag to a node
स्थिर शून्य addFlags(Ast_गाँठ* ध्रुव nodep, uint8_t flags) { nodep->user2Or(flags); }
// Check if a node has ALL of the expected flags set
स्थिर तार्किक hasFlags(Ast_गाँठ* ध्रुव nodep, uint8_t flags) { प्रत्यागम !(~nodep->user2() & flags); }

// ######################################################################
//  Detect nodes affected by timing and/or requiring a process

वर्ग TimingSuspendableVisitor final : सार्वजनिक VNVisitor {
    // TYPES
    // Vertex of a dependency graph of suspendable nodes, e.g. if a node (process or task) is
    // suspendable, all its dependents should also be suspendable
    वर्ग DepVtx VL_NOT_FINAL : सार्वजनिक V3GraphVertex {
        VL_RTTI_IMPL(DepVtx, V3GraphVertex)
        AstClass* ध्रुव m_classp;  // Class associated with a method
        Ast_गाँठ* ध्रुव m_nodep;  // AST node represented by this graph vertex

        // ACCESSORS
        string name() ध्रुव override {
            यदि (m_classp) {
                यदि (VN_IS(nodep(), CFunc)) {
                    प्रत्यागम cvtToHex(nodep()) + ' ' + classp()->name() + "::" + nodep()->name();
                }
            }
            प्रत्यागम cvtToHex(nodep()) + ' ' + nodep()->prettyTypeName();
        }
        File_पंक्ति* fileline() ध्रुव override { प्रत्यागम nodep()->fileline(); }

    सार्वजनिक:
        // CONSTRUCTORS
        DepVtx(V3Graph* graphp, Ast_गाँठ* nodep, AstClass* classp)
            : V3GraphVertex{graphp}
            , m_classp{classp}
            , m_nodep{nodep} {}
        ~DepVtx() override = स्वभाव;

        // ACCESSORS
        आभास Ast_गाँठ* nodep() ध्रुव VL_MT_STABLE { प्रत्यागम m_nodep; }
        आभास Ast_गाँठ* classp() ध्रुव VL_MT_STABLE { प्रत्यागम m_classp; }
    };

    वर्ग SuspendDepVtx final : सार्वजनिक DepVtx {
        VL_RTTI_IMPL(SuspendDepVtx, DepVtx)
        string dotColor() ध्रुव override {
            यदि (hasFlags(nodep(), T_SUSPENDER)) प्रत्यागम "red";
            यदि (hasFlags(nodep(), T_SUSPENDEE)) प्रत्यागम "blue";
            प्रत्यागम "black";
        }

    सार्वजनिक:
        SuspendDepVtx(V3Graph* graphp, Ast_गाँठ* nodep, AstClass* classp)
            : DepVtx{graphp, nodep, classp} {}
        ~SuspendDepVtx() override = स्वभाव;
    };

    वर्ग NeedsProcDepVtx final : सार्वजनिक DepVtx {
        VL_RTTI_IMPL(NeedsProcDepVtx, DepVtx)
        string dotColor() ध्रुव override {
            यदि (hasFlags(nodep(), T_HAS_PROC)) प्रत्यागम "blue";
            यदि (hasFlags(nodep(), T_NEEDS_PROC)) प्रत्यागम "green";
            यदि (hasFlags(nodep(), T_FORCES_PROC)) प्रत्यागम "red";
            प्रत्यागम "black";
        }

    सार्वजनिक:
        NeedsProcDepVtx(V3Graph* graphp, Ast_गाँठ* nodep, AstClass* classp)
            : DepVtx{graphp, nodep, classp} {}
        ~NeedsProcDepVtx() override = स्वभाव;
    };

    // NODE STATE
    //  AstClass::user1() -> bool.  Class member cache has been refreshed.
    //  Ast{NodeProcedure,CFunc,Begin}::user2()  -> uint64_t.  Set to >= T_SUSP if
    //                                                         process/task suspendable
    //                                                         and to T_PROC if it
    //                                                         needs process metadata.
    //  Ast{NodeProcedure,CFunc,Begin}::user3()  -> DependencyVertex*.  Vertex in m_suspGraph
    //  Ast{NodeProcedure,CFunc,Begin}::user3()  -> DependencyVertex*.  Vertex in m_procGraph
    ध्रुव VNUser3InUse m_user3InUse;
    ध्रुव VNUser4InUse m_user4InUse;

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
    DepVtx* getSuspendDepVtx(Ast_गाँठ* ध्रुव nodep) {
        AstClass* classp = nullptr;
        यदि (AstCFunc* funcp = VN_CAST(nodep, CFunc)) {
            यदि (funcp->scopep() && funcp->scopep()->modp()) {
                classp = VN_CAST(funcp->scopep()->modp(), Class);
            }
        }
        यदि (!nodep->user3p()) nodep->user3p(नूतन SuspendDepVtx{&m_suspGraph, nodep, classp});
        प्रत्यागम nodep->user3u().to<SuspendDepVtx*>();
    }
    DepVtx* getNeedsProcDepVtx(Ast_गाँठ* ध्रुव nodep) {
        AstClass* classp = nullptr;
        यदि (AstCFunc* funcp = VN_CAST(nodep, CFunc)) {
            यदि (funcp->scopep() && funcp->scopep()->modp()) {
                classp = VN_CAST(funcp->scopep()->modp(), Class);
            }
        }
        यदि (!nodep->user4p()) nodep->user4p(नूतन NeedsProcDepVtx{&m_procGraph, nodep, classp});
        प्रत्यागम nodep->user4u().to<NeedsProcDepVtx*>();
    }
    // Pass timing flag between nodes
    तार्किक passFlag(ध्रुव Ast_गाँठ* from, Ast_गाँठ* to, गाँठ_Flag flag) {
        यदि ((from->user2() & flag) && !(to->user2() & flag)) {
            addFlags(to, flag);
            प्रत्यागम सत्य;
        }
        प्रत्यागम असत्य;
    }
    // Propagate flag to all nodes that depend on the given one
    शून्य propagateFlags(DepVtx* ध्रुव vxp, गाँठ_Flag flag) {
        auto* ध्रुव parentp = vxp->nodep();
        प्रति (V3GraphEdge& edge : vxp->outEdges()) {
            auto* ध्रुव depVxp = static_cast<DepVtx*>(edge.शीर्ष());
            Ast_गाँठ* ध्रुव depp = depVxp->nodep();
            यदि (passFlag(parentp, depp, flag)) propagateFlags(depVxp, flag);
        }
    }
    template <typename T_Predicate>
    शून्य propagateFlagsIf(DepVtx* ध्रुव vxp, गाँठ_Flag flag, T_Predicate p) {
        auto* ध्रुव parentp = vxp->nodep();
        प्रति (V3GraphEdge& edge : vxp->outEdges()) {
            auto* ध्रुव depVxp = static_cast<DepVtx*>(edge.शीर्ष());
            Ast_गाँठ* ध्रुव depp = depVxp->nodep();
            यदि (p(&edge) && passFlag(parentp, depp, flag)) propagateFlagsIf(depVxp, flag, p);
        }
    }
    template <typename T_Predicate>
    शून्य propagateFlagsReversedIf(DepVtx* ध्रुव vxp, गाँठ_Flag flag, T_Predicate p) {
        auto* ध्रुव parentp = vxp->nodep();
        प्रति (V3GraphEdge& edge : vxp->inEdges()) {
            auto* ध्रुव depVxp = static_cast<DepVtx*>(edge.fromp());
            Ast_गाँठ* ध्रुव depp = depVxp->nodep();
            यदि (p(&edge) && passFlag(parentp, depp, flag))
                propagateFlagsReversedIf(depVxp, flag, p);
        }
    }

    // VISITORS
    शून्य visit(AstClass* nodep) override {
        UASSERT_OBJ(!m_classp, nodep, "Class under class");
        VL_RESTORER(m_classp);
        m_classp = nodep;
        iterateChildren(nodep);
    }
    शून्य visit(Ast_गाँठ_Procedure* nodep) override {
        VL_RESTORER(m_procp);
        m_procp = nodep;
        getNeedsProcDepVtx(nodep);
        addFlags(nodep, T_ALLOCS_PROC);
        यदि (VN_IS(nodep, Always)) {
            UINFO(9, "Always does " << (nodep->needProcess() ? "" : "NOT ") << "need process");
        }
        iterateChildren(nodep);
    }
    शून्य visit(AstDisableFork* nodep) override {
        visit(static_cast<Ast_गाँठ*>(nodep));
        addFlags(m_procp, T_FORCES_PROC | T_NEEDS_PROC);
    }
    शून्य visit(AstWaitFork* nodep) override {
        visit(static_cast<Ast_गाँठ*>(nodep));
        addFlags(m_procp, T_FORCES_PROC | T_NEEDS_PROC);
    }
    शून्य visit(AstWait* nodep) override {
        UINFO(9, "suspendable-visit " << nodep);
        Ast_गाँठ_व्यंजक* ध्रुव condp = V3Const::constifyEdit(nodep->condp());
        यदि (AstConst* ध्रुव constp = VN_CAST(condp, Const)) {
            यदि (!nodep->fileline()->warnIsOff(V3ErrorCode::WAITCONST)) {
                condp->v3warn(WAITCONST, "Wait statement condition is constant");
            }
            यदि (!constp->isZero()) {
                // Remove AstWait before we track process as T_SUSPENDER
                यदि (Ast_गाँठ* ध्रुव stmtsp = nodep->stmtsp()) {
                    stmtsp->unlinkFrBackWithNext();
                    nodep->replaceWith(stmtsp);
                } अन्यथा {
                    nodep->unlinkFrBack();
                }
                VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
                प्रत्यागम;
            }
        }
        v3Global.setUsesTiming();
        यदि (m_procp) addFlags(m_procp, T_SUSPENDEE | T_SUSPENDER | T_NEEDS_PROC);
        iterateChildren(nodep);
    }
    शून्य visit(AstCFunc* nodep) override {
        VL_RESTORER(m_procp);
        m_procp = nodep;
        iterateChildren(nodep);
        यदि (nodep->needProcess()) addFlags(nodep, T_FORCES_PROC | T_NEEDS_PROC);
        DepVtx* ध्रुव sVxp = getSuspendDepVtx(nodep);
        DepVtx* ध्रुव pVxp = getNeedsProcDepVtx(nodep);
        यदि (!m_classp) प्रत्यागम;

        // Go over overridden functions

        std::queue<AstClassExtends*> extends;
        यदि (m_classp->extendsp()) extends.धकेलो(m_classp->extendsp());

        यावत् (!extends.empty()) {
            AstClassExtends* ext_list = extends.front();
            extends.निकालो();

            प्रति (AstClassExtends* cextp = ext_list; cextp;
                 cextp = VN_AS(cextp->nextp(), ClassExtends)) {
                // TODO: It is possible that a methods the same name in the base class is not
                // actually overridden by our method. If this causes a problem, traverse to
                // the root of the inheritance hierarchy and check if the original method is
                // virtual or not.
                यदि (auto* ध्रुव overriddenp
                    = VN_CAST(m_memberMap.findMember(cextp->classp(), nodep->name()), CFunc)) {
                    // Suspendability and process affects typing, so they propagate both ways
                    DepVtx* ध्रुव overriddenSVxp = getSuspendDepVtx(overriddenp);
                    DepVtx* ध्रुव overriddenPVxp = getNeedsProcDepVtx(overriddenp);
                    नूतन V3GraphEdge{&m_suspGraph, sVxp, overriddenSVxp, P_SIGNATURE};
                    नूतन V3GraphEdge{&m_suspGraph, overriddenSVxp, sVxp, P_SIGNATURE};
                    नूतन V3GraphEdge{&m_procGraph, pVxp, overriddenPVxp, P_SIGNATURE};
                    नूतन V3GraphEdge{&m_procGraph, overriddenPVxp, pVxp, P_SIGNATURE};
                } अन्यथा {
                    AstClassExtends* more_extends = cextp->classp()->extendsp();
                    यदि (more_extends) extends.धकेलो(more_extends);
                }
            }
        }
    }
    शून्य visit(Ast_गाँठ_CCall* nodep) override {
        AstCFunc* ध्रुव funcp = nodep->funcp();
        UASSERT_OBJ(funcp, nodep, "AstNodeCCall must have non-null funcp post-link");
        UASSERT_OBJ(m_procp, nodep, "AstNodeCCall must be inside a procedure/CFunc/Begin");

        UINFO(9, "V3Timing: Processing CCall to " << funcp->name() << " in dependency graph\n");
        नूतन V3GraphEdge{&m_suspGraph, getSuspendDepVtx(funcp), getSuspendDepVtx(m_procp), P_CALL};

        नूतन V3GraphEdge{&m_procGraph, getNeedsProcDepVtx(funcp), getNeedsProcDepVtx(m_procp),
                        P_CALL};

        iterateChildren(nodep);
    }
    शून्य visit(AstBegin* nodep) override {
        VL_RESTORER(m_procp);
        VL_RESTORER(m_underFork);

        यदि (!m_underFork || (m_underFork & F_MIGHT_SUSPEND))
            नूतन V3GraphEdge{&m_suspGraph, getSuspendDepVtx(nodep), getSuspendDepVtx(m_procp),
                            m_underFork ? P_FORK : P_CALL};

        नूतन V3GraphEdge{&m_procGraph, getNeedsProcDepVtx(nodep), getNeedsProcDepVtx(m_procp),
                        P_CALL};

        यदि (m_underFork) addFlags(nodep, T_NEEDS_PROC | T_ALLOCS_PROC);

        m_procp = nodep;
        m_underFork = 0;
        iterateChildren(nodep);
    }
    शून्य visit(AstFork* nodep) override {
        UINFO(9, "suspendable-visit " << nodep);
        VL_RESTORER(m_underFork);

        v3Global.setUsesTiming();  // Even if there are no event controls, we have to set this flag
                                   // so that transformForks() in V3SchedTiming gets called and
                                   // removes all forks and begins
        यदि (nodep->isTimingControl() && m_procp) {
            addFlags(m_procp, T_SUSPENDEE | T_SUSPENDER);
            m_underFork |= F_MIGHT_SUSPEND;
        }
        m_underFork |= F_MIGHT_NEED_PROC;
        iterateChildren(nodep);
    }
    शून्य visit(AstAssignDly* nodep) override {
        यदि (!VN_IS(m_procp, गाँठ_Procedure)) v3Global.setUsesTiming();
        visit(static_cast<Ast_गाँठ*>(nodep));
    }
    शून्य visit(AstFireEvent* nodep) override {
        यदि (nodep->isDelayed()
            && (!VN_IS(m_procp, गाँठ_Procedure) || !VN_IS(nodep->operandp(), VarRef))) {
            v3Global.setUsesTiming();
        }
        visit(static_cast<Ast_गाँठ*>(nodep));
    }
    शून्य visit(AstAssignW* nodep) override {
        यदि (nodep->timingControlp()) v3Global.setUsesTiming();
        // Containing process will not suspend, don't mark it
    }
    शून्य visit(Ast_गाँठ* nodep) override {
        यदि (nodep->isTimingControl()) {
            v3Global.setUsesTiming();
            यदि (m_procp) addFlags(m_procp, T_SUSPENDEE | T_SUSPENDER | T_NEEDS_PROC);
        }
        iterateChildren(nodep);
    }

    //--------------------
    शून्य visit(AstVar*) override {}  // Accelerate

सार्वजनिक:
    // CONSTRUCTORS
    explicit TimingSuspendableVisitor(AstNetlist* nodep) {
        iterate(nodep);
        m_suspGraph.removeTransitiveEdges();
        m_procGraph.removeTransitiveEdges();
        // Propagate suspendability
        प्रति (V3GraphVertex& vtx : m_suspGraph.vertices()) {
            DepVtx& depVtx = static_cast<DepVtx&>(vtx);
            यदि (hasFlags(depVtx.nodep(), T_SUSPENDEE)) propagateFlags(&depVtx, T_SUSPENDEE);
        }
        यदि (dumpGraphLevel() >= 6) m_suspGraph.dumpDotFilePrefixed("timing_deps");

        // Propagate T_HAS_PROCESS
        प्रति (V3GraphVertex& vtx : m_procGraph.vertices()) {
            DepVtx& depVtx = static_cast<DepVtx&>(vtx);
            // Find processes that'll allocate VlProcess
            यदि (hasFlags(depVtx.nodep(), T_FORCES_PROC)) {
                propagateFlagsIf(&depVtx, T_FORCES_PROC, [&](ध्रुव V3GraphEdge* e) -> तार्किक {
                    प्रत्यागम !hasFlags(static_cast<DepVtx*>(e->fromp())->nodep(), T_ALLOCS_PROC);
                });
            }
            // Mark nodes on paths between processes and statements that use VlProcess
            यदि (hasFlags(depVtx.nodep(), T_NEEDS_PROC)) {
                propagateFlagsIf(&depVtx, T_NEEDS_PROC, [&](ध्रुव V3GraphEdge* e) -> तार्किक {
                    प्रत्यागम !hasFlags(static_cast<DepVtx*>(e->शीर्ष())->nodep(), T_ALLOCS_PROC);
                });
            }
        }
        प्रति (V3GraphVertex& vtx : m_procGraph.vertices()) {
            DepVtx& depVtx = static_cast<DepVtx&>(vtx);
            // Mark nodes that will be emitted with a VlProcess argument
            यदि (hasFlags(depVtx.nodep(), T_ALLOCS_PROC | T_FORCES_PROC)) {
                addFlags(depVtx.nodep(), T_HAS_PROC);
                propagateFlagsReversedIf(&depVtx, T_HAS_PROC, [&](ध्रुव V3GraphEdge* e) -> तार्किक {
                    प्रत्यागम hasFlags(static_cast<DepVtx*>(e->fromp())->nodep(), T_NEEDS_PROC);
                });
            }
        }
        यदि (dumpGraphLevel() >= 6) m_procGraph.dumpDotFilePrefixed("proc_deps");
    }
    ~TimingSuspendableVisitor() override = स्वभाव;
};

// ######################################################################
//  Transform nodes affected by timing

वर्ग TimingControlVisitor final : सार्वजनिक VNVisitor {
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
    AstNetlist* ध्रुव m_netlistp;  // Root node
    Ast_शीर्ष_Scope* ध्रुव m_शीर्ष_Scopep = m_netlistp->शीर्ष_Scopep();  // The AstTopScope
    AstScope* ध्रुव m_scopeTopp = m_शीर्ष_Scopep->scopep();  // Scope at the top
    AstClass* m_classp = nullptr;  // Current class
    AstScope* m_scopep = nullptr;  // Current scope
    AstActive* m_activep = nullptr;  // Current active
    Ast_गाँठ* m_procp = nullptr;  // NodeProcedure/CFunc/Begin we're under
    तार्किक m_hasProcess = असत्य;  // True if current scope has a VlProcess handle available
    पूर्णाङ्क m_forkCnt = 0;  // Number of forks inside a module
    तार्किक m_underJumpBlock = असत्य;  // True if we are inside of a jump-block
    तार्किक m_underProcedure = असत्य;  // True if we are under an always or initial
    तार्किक m_underIfaceCFunc = असत्य;  // True if we are under a CFunc owned by an interface scope
    तार्किक m_hasStaticZeroDelay = असत्य;  // True if we have a static #0 delay
    std::vector<File_पंक्ति*> m_unknownDelayFlps;  // Locations of AstDelay with non-constant value

    // Unique names
    V3UniqueNames m_dlyforkNames{"__Vdlyfork"};  // Names for temp AssignW vars
    V3UniqueNames m_intra_मूल्य_Names{"__Vintraval"};  // Intra assign delay value var names
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
    Ast_Sen_वृक्ष* m_delaySensesp = nullptr;  // Domain to trigger if a delayed coroutine is resumed
    Ast_Sen_वृक्ष* m_dynamicSensesp = nullptr;  // Domain to trigger if a dynamic trigger is set

    // Other
    Sen_वृक्ष_Finder m_finder{m_netlistp};  // Sentree finder and uniquifier
    Sen_व्यंजक_Builder* m_sen_व्यंजक_Builderp = nullptr;  // Sens expression builder for current m_scope

    // Stats
    size_t m_statZeroDelays = 0;  // Number of statically known #0 delays
    size_t m_statConstDelays = 0;  // Number of statically known #const (non-zero) delays
    size_t m_statVariableDelays = 0;  // Number of delays with value unknown at compile time

    // METHODS
    // Transform an assignment with an intra timing control into a timing control with the
    // assignment under it
    Ast_गाँठ* factorOutTimingControl(Ast_गाँठ_Assign* nodep) ध्रुव {
        Ast_गाँठ* stmtp = nodep;
        Ast_गाँठ* ध्रुव controlp = nodep->timingControlp();
        यदि (AstDelay* ध्रुव delayp = VN_CAST(controlp, Delay)) {
            stmtp->replaceWith(delayp->unlinkFrBack());
            delayp->addStmtsp(stmtp);
            stmtp = delayp;
        } अन्यथा यदि (Ast_Sen_वृक्ष* ध्रुव sentreep = VN_CAST(controlp, Sen_वृक्ष)) {
            AstEventControl* ध्रुव eventControlp
                = नूतन AstEventControl{sentreep->fileline(), sentreep->unlinkFrBack(), nullptr};
            stmtp->replaceWith(eventControlp);
            eventControlp->addStmtsp(stmtp);
            stmtp = eventControlp;
        } अन्यथा यदि (AstBegin* ध्रुव beginp = VN_CAST(controlp, Begin)) {
            // Begin from V3AssertPre
            stmtp->replaceWith(beginp->unlinkFrBack());
            beginp->addStmtsp(stmtp);
            stmtp = beginp;
        }
        प्रत्यागम stmtp == nodep ? nullptr : stmtp;
    }
    स्थिर तार्किक staticallyNonZeroDelay(ध्रुव Ast_गाँठ_व्यंजक* valuep) {
        यदि (ध्रुव AstConst* ध्रुव constp = VN_CAST(valuep, Const)) प्रत्यागम !constp->isZero();
        यदि (ध्रुव AstCond* ध्रुव condp = VN_CAST(valuep, Cond)) {
            प्रत्यागम staticallyNonZeroDelay(condp->thenp())
                   && staticallyNonZeroDelay(condp->elsep());
        }
        यदि (ध्रुव AstMul* ध्रुव mulp = VN_CAST(valuep, Mul)) {
            ध्रुव AstConst* ध्रुव lhsConstp = VN_CAST(mulp->lhsp(), Const);
            ध्रुव AstConst* ध्रुव rhsConstp = VN_CAST(mulp->rhsp(), Const);
            यदि (lhsConstp && !lhsConstp->isZero()) प्रत्यागम staticallyNonZeroDelay(mulp->rhsp());
            यदि (rhsConstp && !rhsConstp->isZero()) प्रत्यागम staticallyNonZeroDelay(mulp->lhsp());
        }
        प्रत्यागम असत्य;
    }
    // Creates the global delay scheduler variable
    AstVarScope* getCreateDelayScheduler() {
        यदि (m_delaySchedp) प्रत्यागम m_delaySchedp;
        auto* ध्रुव dlySchedDtp = नूतन AstBasicDType{
            m_scopeTopp->fileline(), VBasicDTypeKwd::DELAY_SCHEDULER, VSigning::UNSIGNED};
        m_netlistp->typeTablep()->addTypesp(dlySchedDtp);
        m_delaySchedp = m_शीर्ष_Scopep->createTemp("__VdlySched", dlySchedDtp);
        // Delay scheduler has to be accessible from top
        m_delaySchedp->varp()->sigPublic(सत्य);
        m_netlistp->delaySchedulerp(m_delaySchedp->varp());
        प्रत्यागम m_delaySchedp;
    }
    // Creates the delay sentree
    Ast_Sen_वृक्ष* get_Create_Delay_Sen_वृक्ष() {
        यदि (m_delaySensesp) प्रत्यागम m_delaySensesp;
        File_पंक्ति* ध्रुव flp = m_scopeTopp->fileline();
        auto* ध्रुव awaitingCurrentTimep
            = नूतन AstCMethodHard{flp, नूतन AstVarRef{flp, getCreateDelayScheduler(), VAccess::READ},
                                 VCMethod::SCHED_AWAITING_CURRENT_TIME};
        awaitingCurrentTimep->dtypeSetBit();
        m_delaySensesp
            = नूतन Ast_Sen_वृक्ष{flp, नूतन AstSenItem{flp, VEdgeType::ET_TRUE, awaitingCurrentTimep}};
        m_netlistp->शीर्ष_Scopep()->addSenTreesp(m_delaySensesp);
        प्रत्यागम m_delaySensesp;
    }
    // Creates the global dynamic trigger scheduler variable
    AstVarScope* getCreateDynamicTriggerScheduler() {
        यदि (m_dynamicSchedp) प्रत्यागम m_dynamicSchedp;
        auto* ध्रुव dynSchedDtp
            = नूतन AstBasicDType{m_scopeTopp->fileline(), VBasicDTypeKwd::DYNAMIC_TRIGGER_SCHEDULER,
                                VSigning::UNSIGNED};
        m_netlistp->typeTablep()->addTypesp(dynSchedDtp);
        m_dynamicSchedp = m_शीर्ष_Scopep->createTemp("__VdynSched", dynSchedDtp);
        प्रत्यागम m_dynamicSchedp;
    }
    // Creates the dynamic trigger sentree
    Ast_Sen_वृक्ष* get_Create_Dynamic_Trigger_Sen_वृक्ष() {
        यदि (m_dynamicSensesp) प्रत्यागम m_dynamicSensesp;
        File_पंक्ति* ध्रुव flp = m_scopeTopp->fileline();
        auto* ध्रुव awaitingCurrentTimep = नूतन AstCMethodHard{
            flp, नूतन AstVarRef{flp, getCreateDynamicTriggerScheduler(), VAccess::READ},
            VCMethod::SCHED_EVALUATE};
        awaitingCurrentTimep->dtypeSetBit();
        m_dynamicSensesp
            = नूतन Ast_Sen_वृक्ष{flp, नूतन AstSenItem{flp, VEdgeType::ET_TRUE, awaitingCurrentTimep}};
        m_netlistp->शीर्ष_Scopep()->addSenTreesp(m_dynamicSensesp);
        प्रत्यागम m_dynamicSensesp;
    }
    // Creates the event variable to trigger in NBA region
    AstEventControl* createNbaEventControl(File_पंक्ति* flp) {
        यदि (!m_netlistp->nbaEventp()) {
            auto* ध्रुव nbaEventDtp = नूतन AstBasicDType{m_scopeTopp->fileline(),
                                                        VBasicDTypeKwd::EVENT, VSigning::UNSIGNED};
            m_netlistp->typeTablep()->addTypesp(nbaEventDtp);
            m_netlistp->nbaEventp(m_शीर्ष_Scopep->createTemp("__VnbaEvent", nbaEventDtp));
            v3Global.setHasEvents();
        }
        प्रत्यागम नूतन AstEventControl{
            flp,
            नूतन Ast_Sen_वृक्ष{
                flp, नूतन AstSenItem{flp, VEdgeType::ET_EVENT,
                                    नूतन AstVarRef{flp, m_netlistp->nbaEventp(), VAccess::READ}}},
            nullptr};
    }
    // Creates the variable that, if set, causes the NBA event to be triggered
    AstAssign* createNbaEventTriggerAssignment(File_पंक्ति* flp) {
        यदि (!m_netlistp->nbaEventTriggerp()) {
            m_netlistp->nbaEventTriggerp(m_शीर्ष_Scopep->createTemp("__VnbaEventTrigger", 1));
        }
        प्रत्यागम नूतन AstAssign{flp,
                             नूतन AstVarRef{flp, m_netlistp->nbaEventTriggerp(), VAccess::WRITE},
                             नूतन AstConst{flp, AstConst::BitTrue{}}};
    }
    // Returns true if we are under a class or interface function, or the tree references locals.
    // These are cases where static, globally-evaluated triggers are not suitable.
    तार्किक needDynamicTrigger(Ast_गाँठ* ध्रुव nodep) ध्रुव {
        प्रत्यागम m_classp || m_underIfaceCFunc || nodep->exists([](Ast_गाँठ* ध्रुव nodep) {
            यदि (Ast_गाँठ_Var_Ref* varp = VN_CAST(nodep, गाँठ_Var_Ref)) {
                प्रत्यागम varp->varp()->isFuncLocal();
            }
            प्रत्यागम !nodep->isPure();
        });
    }
    // Creates a trigger scheduler variable
    AstVarScope* getCreateTriggerSchedulerp(Ast_Sen_वृक्ष* ध्रुव sentreep) {
        यदि (!sentreep->user1p()) {
            यदि (!m_trigSchedDtp) {
                m_trigSchedDtp
                    = नूतन AstBasicDType{m_scopeTopp->fileline(), VBasicDTypeKwd::TRIGGER_SCHEDULER,
                                        VSigning::UNSIGNED};
                m_netlistp->typeTablep()->addTypesp(m_trigSchedDtp);
            }
            AstVarScope* ध्रुव trigSchedp
                = m_शीर्ष_Scopep->createTemp(m_trigSchedNames.get(sentreep), m_trigSchedDtp);
            sentreep->user1p(trigSchedp);
        }
        प्रत्यागम VN_AS(sentreep->user1p(), VarScope);
    }
    // Creates a string describing the sentree
    AstCExpr* createEventDescription(Ast_Sen_वृक्ष* ध्रुव sentreep) ध्रुव {
        यदि (!sentreep->user2p()) {
            std::stringstream ss;
            V3EmitV::verilog_For_वृक्ष(sentreep, ss);
            // possibly a multiline string
            // Names were hashed by V3Name, show the user the original text
            std::string comment = VName::dehash(ss.str());
            std::replace(comment.begin(), comment.end(), '\n', ' ');
            // Escape quotes, e.g. from string-indexed event array selects
            comment = '"' + V3OutFormatter::quoteNameControls(comment) + '"';
            AstCExpr* ध्रुव commentp = नूतन AstCExpr{sentreep->fileline(), comment};
            commentp->dtypeSetString();
            sentreep->user2p(commentp);
            प्रत्यागम commentp;
        }
        प्रत्यागम VN_AS(sentreep->user2p(), CExpr)->clone_वृक्ष(असत्य);
    }
    // Adds debug info to a hardcoded method call
    शून्य addDebugInfo(AstCMethodHard* ध्रुव methodp) ध्रुव {
        File_पंक्ति* ध्रुव flp = methodp->fileline();
        ध्रुव तार्किक protectIds = v3Global.opt.protectIds();
        AstCExpr* ध्रुव ap
            = नूतन AstCExpr{flp, protectIds ? "VL_UNKNOWN" : '"' + flp->filenameEsc() + '"'};
        ap->dtypeSetString();
        methodp->addPinsp(ap);
        AstCExpr* ध्रुव bp = नूतन AstCExpr{flp, protectIds ? "0" : cvtToStr(flp->lineno())};
        bp->dtypeSetString();
        methodp->addPinsp(bp);
    }
    // Adds debug info to a trigSched.trigger() call
    शून्य addEventDebugInfo(AstCMethodHard* ध्रुव methodp, Ast_Sen_वृक्ष* ध्रुव sentreep) ध्रुव {
        यदि (v3Global.opt.protectIds()) {
            File_पंक्ति* ध्रुव flp = sentreep->fileline();
            AstCExpr* ध्रुव descp = नूतन AstCExpr{flp, "VL_UNKNOWN"};
            descp->dtypeSetString();
            methodp->addPinsp(descp);
            addDebugInfo(methodp);
            प्रत्यागम;
        }
        methodp->addPinsp(createEventDescription(sentreep));
        addDebugInfo(methodp);
    }
    // Adds process pointer to a hardcoded method call
    शून्य addProcessInfo(AstCMethodHard* ध्रुव methodp) ध्रुव {
        File_पंक्ति* ध्रुव flp = methodp->fileline();
        AstCExpr* ध्रुव ap
            = नूतन AstCExpr{flp, (m_procp && m_hasProcess) ? "vlProcess" : "nullptr"};
        methodp->addPinsp(ap);
    }
    // Creates the fork handle type and returns it
    AstBasicDType* getCreateForkSyncDTypep() {
        यदि (m_forkDtp) प्रत्यागम m_forkDtp;
        m_forkDtp = नूतन AstBasicDType{m_scopeTopp->fileline(), VBasicDTypeKwd::FORK_SYNC,
                                      VSigning::UNSIGNED};
        m_netlistp->typeTablep()->addTypesp(m_forkDtp);
        प्रत्यागम m_forkDtp;
    }
    // Move `nodep` into `AstCLocalScope`.
    AstCLocalScope* addCLocalScope(File_पंक्ति* ध्रुव flp, Ast_गाँठ* ध्रुव nodep) ध्रुव {
        यदि (!nodep) प्रत्यागम nullptr;
        VNRelinker handle;
        nodep->unlinkFrBack(&handle);
        AstCLocalScope* ध्रुव cscopep = नूतन AstCLocalScope{flp, nodep};
        handle.relink(cscopep);
        प्रत्यागम cscopep;
    }
    // Create a temp variable and optionally put it before the specified node (mark local if so)
    AstVarScope* createTemp(File_पंक्ति* ध्रुव flp, ध्रुव std::string& name,
                            Ast_गाँठ_DType* ध्रुव dtypep, Ast_गाँठ* ध्रुव जोड़ो_Beforep = nullptr) {
        AstVar* varp;
        यदि (जोड़ो_Beforep) {
            varp = नूतन AstVar{flp, VVarType::BLOCKTEMP, name, dtypep};
            varp->funcLocal(सत्य);
            जोड़ो_Beforep->addHereThisAsNext(varp);
        } अन्यथा {
            varp = नूतन AstVar{flp, VVarType::MODULETEMP, name, dtypep};
            m_scopep->modp()->addStmtsp(varp);
        }
        AstVarScope* ध्रुव vscp = नूतन AstVarScope{flp, m_scopep, varp};
        m_scopep->addVarsp(vscp);
        प्रत्यागम vscp;
    }
    // Add a done() call on the fork sync
    शून्य addForkDone(AstBegin* ध्रुव beginp, AstVarScope* ध्रुव forkVscp) ध्रुव {
        File_पंक्ति* ध्रुव flp = beginp->fileline();
        auto* ध्रुव donep = नूतन AstCMethodHard{
            beginp->fileline(), नूतन AstVarRef{flp, forkVscp, VAccess::WRITE}, VCMethod::FORK_DONE};
        donep->dtypeSetVoid();
        addDebugInfo(donep);
        beginp->addStmtsp(donep->makeStmt());
    }
    स्थिर तार्किक has_Disable_Queue_धकेलो_Self_Prefix(AstBegin* ध्रुव beginp) {
        // LinkJump prepends disable-by-name registration as:
        //   __VprocessQueue_*.push_back(std::process::self())
        // V3LiftExpr lifts std::process::self() into an assignment to a temporary, then V3Task
        // lowers that assignment's function call into a leading comment and AstStmtExpr. Thus the
        // push_back is no longer necessarily the first statement. Scan across this generated
        // prefix for it, stopping at the fork-start sentinel or the branch body. Registrations for
        // nested forks live in sub-blocks and so are not in this statement list.
        प्रति (Ast_गाँठ* stmtp = beginp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            यदि (VN_IS(stmtp, Comment)) अनुवर्त;
            Ast_Stmt_व्यंजक* ध्रुव stmtExprp = VN_CAST(stmtp, Stmt_व्यंजक);
            यदि (!stmtExprp) भङ्ग;
            यदि (stmtExprp->is_Disable_Queue_धकेलो_Self_Stmt()) प्रत्यागम सत्य;
        }
        प्रत्यागम असत्य;
    }
    // Register a callback so killing a process-backed fork branch decrements the join counter
    शून्य addForkOnKill(AstBegin* ध्रुव beginp, AstVarScope* ध्रुव forkVscp) ध्रुव {
        यदि (!beginp->needProcess() && !has_Disable_Queue_धकेलो_Self_Prefix(beginp)) प्रत्यागम;
        File_पंक्ति* ध्रुव flp = beginp->fileline();
        AstCMethodHard* ध्रुव onKillp = नूतन AstCMethodHard{
            flp, नूतन AstVarRef{flp, forkVscp, VAccess::WRITE}, VCMethod::FORK_ON_KILL};
        onKillp->dtypeSetVoid();
        AstCExpr* ध्रुव processp = नूतन AstCExpr{flp, "vlProcess"};
        processp
            ->dtypeSetVoid();  // Opaque process reference; type is irrelevant for hardcoded emit
        onKillp->addPinsp(processp);
        Ast_गाँठ_Stmt* ध्रुव stmtp = onKillp->makeStmt();
        beginp->prependStmtsp(stmtp);
    }
    // Handle the 'join' part of a fork..join
    शून्य makeForkJoin(AstFork* ध्रुव forkp) {
        // Create a fork sync var
        File_पंक्ति* ध्रुव flp = forkp->fileline();
        // Insert the sync var directly before the fork
        Ast_गाँठ* ध्रुव जोड़ो_Beforep = forkp;
        // Make sure all references to the fork var are contained within a single scope to prevent
        // moving statements accessing it in case of large functions splitting.
        addCLocalScope(flp, जोड़ो_Beforep);
        AstVarScope* forkVscp
            = createTemp(flp, forkp->name() + "__sync", getCreateForkSyncDTypep(), जोड़ो_Beforep);
        unsigned join_गिनती = 0;  // Needed for join counter
        // Add a <fork sync>.done() to each begin
        प्रति (Ast_गाँठ* beginp = forkp->forksp(); beginp; beginp = beginp->nextp()) {
            addForkOnKill(VN_AS(beginp, Begin), forkVscp);
            addForkDone(VN_AS(beginp, Begin), forkVscp);
            join_गिनती++;
        }
        यदि (forkp->joinType().joinAny()) join_गिनती = 1;
        // Set the join counter
        auto* ध्रुव initp = नूतन AstCMethodHard{flp, नूतन AstVarRef{flp, forkVscp, VAccess::WRITE},
                                               VCMethod::FORK_INIT, नूतन AstConst{flp, join_गिनती}};
        initp->dtypeSetVoid();
        addProcessInfo(initp);
        forkp->addHereThisAsNext(initp->makeStmt());
        // Await the join at the end
        auto* ध्रुव joinp = नूतन AstCMethodHard{flp, नूतन AstVarRef{flp, forkVscp, VAccess::WRITE},
                                               VCMethod::FORK_JOIN};
        joinp->dtypeSetVoid();
        addProcessInfo(joinp);
        addDebugInfo(joinp);
        forkp->addNextHere(नूतन AstCAwait{flp, joinp});
    }

    // `procp` shall be a NodeProcedure/CFunc/Begin and within it vars from `varsp` will be placed.
    // `varsp` vector of vars which shall be localized.
    स्थिर शून्य localizeVars(Ast_गाँठ* ध्रुव procp, ध्रुव std::vector<AstVar*>& varsp) {
        UASSERT(procp, "procp is nullptr");
        AstVar* newVarsp = nullptr;
        प्रति (AstVar* ध्रुव varp : varsp) {
            varp->funcLocal(सत्य);
            newVarsp = Ast_गाँठ::addNextNull(newVarsp, varp->unlinkFrBack());
        }
        यदि (Ast_गाँठ_Procedure* ध्रुव गाँठ_Procp = VN_CAST(procp, गाँठ_Procedure)) {
            गाँठ_Procp->prependStmtsp(newVarsp);
        } अन्यथा यदि (AstCFunc* ध्रुव cfuncp = VN_CAST(procp, CFunc)) {
            cfuncp->prependVarsp(newVarsp);
        } अन्यथा यदि (AstBegin* ध्रुव beginp = VN_CAST(procp, Begin)) {
            beginp->prependStmtsp(newVarsp);
        } अन्यथा {
            procp->v3fatalSrc(
                procp->prettyNameQ()
                << " is not of an expected type NodeProcedure/CFunc/Begin instead it is: "
                << procp->prettyTypeName());
        }
    }

    Ast_गाँठ_व्यंजक* delayToIntegral(AstDelay* nodep, Ast_गाँठ_व्यंजक* valuep) {
        ध्रुव AstConst* ध्रुव constp = VN_CAST(valuep, Const);
        // The fork start sentinel is already in ticks
        यदि (constp && constp->toUQuad() == std::numeric_limits<uint64_t>::max()) प्रत्यागम valuep;
        प्रत्यागम V3Timing::delayToTicks(valuep, nodep->timeunit());
    }

    // VISITORS
    शून्य visit(Ast_गाँठ_Module* nodep) override {
        UASSERT_OBJ(!m_classp, nodep, "Module or class under class");
        VL_RESTORER(m_classp);
        m_classp = VN_CAST(nodep, Class);
        VL_RESTORER(m_forkCnt);
        m_forkCnt = 0;
        iterateChildren(nodep);
    }
    शून्य visit(AstScope* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        Sen_व्यंजक_Builder sen_व्यंजक_Builder{m_scopep};
        {  // Restore m_senExprBuilderp before destroying senExprBuilder
            VL_RESTORER(m_sen_व्यंजक_Builderp);
            m_sen_व्यंजक_Builderp = &sen_व्यंजक_Builder;
            iterateChildren(nodep);
        }
    }
    शून्य visit(AstActive* nodep) override {
        VL_RESTORER(m_activep);
        m_activep = nodep;
        iterateChildren(nodep);
    }
    शून्य visit(Ast_गाँठ_Procedure* nodep) override {
        VL_RESTORER(m_procp);
        VL_RESTORER(m_hasProcess);
        m_procp = nodep;
        m_hasProcess = hasFlags(nodep, T_HAS_PROC);
        VL_RESTORER(m_underProcedure);
        m_underProcedure = सत्य;
        iterateChildren(nodep);
        यदि (hasFlags(nodep, T_SUSPENDEE)) nodep->setSuspendable();
        यदि (hasFlags(nodep, T_HAS_PROC)) nodep->setNeedProcess();
    }
    शून्य visit(AstInitial* nodep) override {
        visit(static_cast<Ast_गाँठ_Procedure*>(nodep));
        यदि (nodep->needProcess() && !nodep->user1SetOnce()) {
            nodep->addStmtsp(
                नूतन AstCStmt{nodep->fileline(), "vlProcess->state(VlProcess::FINISHED);"});
        }
    }
    शून्य visit(AstJumpBlock* nodep) override {
        VL_RESTORER(m_underJumpBlock);
        m_underJumpBlock = सत्य;
        visit(static_cast<Ast_गाँठ_Stmt*>(nodep));
    }
    शून्य visit(AstAlways* nodep) override {
        यदि (nodep->user1SetOnce()) प्रत्यागम;
        VL_RESTORER(m_procp);
        VL_RESTORER(m_hasProcess);
        m_procp = nodep;
        m_hasProcess = hasFlags(nodep, T_HAS_PROC);
        VL_RESTORER(m_underProcedure);
        m_underProcedure = सत्य;
        // Workaround for killing `always` processes (doing that is pretty much UB)
        // TODO: Disallow killing `always` at runtime (throw an error)
        // Skip for combinational blocks; if the body has timing controls
        // iterateChildren will add T_SUSPENDEE, otherwise it would spin.
        यदि (hasFlags(nodep, T_HAS_PROC)
            && !(m_activep && m_activep->sentreep() && m_activep->sentreep()->hasCombo()))
            addFlags(nodep, T_SUSPENDEE);

        iterateChildren(nodep);
        यदि (hasFlags(nodep, T_HAS_PROC)) nodep->setNeedProcess();
        यदि (!hasFlags(nodep, T_SUSPENDEE)) प्रत्यागम;
        nodep->setSuspendable();
        File_पंक्ति* ध्रुव flp = nodep->fileline();
        Ast_Sen_वृक्ष* ध्रुव sentreep = m_activep->sentreep();
        यदि (sentreep->hasClocked()) {
            Ast_गाँठ* ध्रुव bodysp = nodep->stmtsp()->unlinkFrBackWithNext();
            auto* ध्रुव controlp = नूतन AstEventControl{flp, sentreep->clone_वृक्ष(असत्य), bodysp};
            nodep->addStmtsp(controlp);
            iterate(controlp);
        }
        // Note: The 'while (true)' outer loop will be added in V3Sched
        auto* ध्रुव activep = नूतन AstActive{
            flp, "", नूतन Ast_Sen_वृक्ष{flp, नूतन AstSenItem{flp, AstSenItem::Initial{}}}};
        activep->sen_वृक्ष_Storep(activep->sentreep());
        activep->addStmtsp(nodep->unlinkFrBack());
        m_activep->addNextHere(activep);
    }
    शून्य visit(AstCFunc* nodep) override {
        VL_RESTORER(m_procp);
        VL_RESTORER(m_hasProcess);
        VL_RESTORER(m_underIfaceCFunc);
        m_procp = nodep;
        m_hasProcess = hasFlags(nodep, T_HAS_PROC);
        m_underIfaceCFunc = VN_IS(m_scopep->modp(), Iface);
        iterateChildren(nodep);
        यदि (hasFlags(nodep, T_HAS_PROC)) nodep->setNeedProcess();
        यदि (!(hasFlags(nodep, T_SUSPENDEE))) प्रत्यागम;

        nodep->rtnType("VlCoroutine");
        // If in a class, create a shared pointer to 'this'
        यदि (m_classp) {
            AstCStmt* ध्रुव cstmtp = नूतन AstCStmt{nodep->fileline(), "VL_KEEP_THIS;"};
            nodep->prependStmtsp(cstmtp);
        }
        यदि (nodep->dpiExportImpl()) {
            nodep->exists([](AstCAwait* ध्रुव awaitp) -> तार्किक {
                // A DPI-exported coroutine won't be able to block the calling code
                // Error on the await node; fall back to the function node
                awaitp->v3warn(E_UNSUPPORTED,
                               "Unsupported: Timing controls inside DPI-exported tasks");
                प्रत्यागम सत्य;
            });
        }
    }
    शून्य visit(Ast_गाँठ_CCall* nodep) override {
        AstCFunc* ध्रुव funcp = nodep->funcp();

        // Skip automatic covergroup sampling calls
        यदि (funcp->isCovergroupSample()) {
            iterateChildren(nodep);
            प्रत्यागम;
        }

        यदि (funcp->needProcess()) m_hasProcess = सत्य;
        यदि (hasFlags(funcp, T_SUSPENDEE) && !nodep->user1SetOnce()) {  // If suspendable
            // Calls to suspendables are always void return type, hence parent must be StmtExpr
            Ast_Stmt_व्यंजक* ध्रुव stmtp = VN_AS(nodep->backp(), Stmt_व्यंजक);
            stmtp->replaceWith(नूतन AstCAwait{nodep->fileline(), nodep->unlinkFrBack()});
            VL_DO_DANGLING(धकेलो_Deletep(stmtp), stmtp);
        }
        iterateChildren(nodep);
    }
    शून्य visit(AstDelay* nodep) override {
        UASSERT_OBJ(!nodep->isCycleDelay(), nodep,
                    "Cycle delays should have been handled in V3AssertPre");
        File_पंक्ति* ध्रुव flp = nodep->fileline();

        Ast_गाँठ_व्यंजक* valuep = delayToIntegral(nodep, nodep->lhsp()->unlinkFrBack());

        // Statistics
        यदि (valuep->isZero()) {
            ++m_statZeroDelays;
        } अन्यथा यदि (VN_IS(valuep, Const)) {
            ++m_statConstDelays;
        } अन्यथा {
            ++m_statVariableDelays;
        }

        // Decide scheduling support for #0
        यदि (v3Global.opt.schedZeroDelay().isSetTrue()) {
            // User said to schedule for #0 support, nothing else to do
            v3Global.setUsesZeroDelay();
        } अन्यथा यदि (v3Global.opt.schedZeroDelay().isSetFalse()) {
            // User said to schedule without #0 support. Still warn if a static #0 delay exists
            यदि (valuep->isZero()) {
                nodep->v3warn(
                    ZERODLY,
                    "Static #0 delay exists, but '--no-sched-zero-delay' was given.\n"
                        << nodep->warnMore()  //
                        << "... Can proceed, but this will fail at runtime if executed.");
            }
        } अन्यथा {
            // User did not express preference, decide based on presence of delays
            यदि (valuep->isZero()) {
                // Statically known #0 delay exists, schedule for #0 support
                v3Global.setUsesZeroDelay();
                m_hasStaticZeroDelay = सत्य;
                // Don't warn on variable delays, as no point
                m_unknownDelayFlps.clear();
            } अन्यथा यदि (staticallyNonZeroDelay(valuep)) {
                // Delay is dynamic, but every statically known outcome is non-zero.
                // So we don't need #0 delay support and there should be no warning.
            } अन्यथा यदि (!VN_IS(valuep, Const)) {
                // Delay is not known at compiile time. Conservatively schedule for #0 support,
                // but warn if no static #0 delays used as performance might be improved
                v3Global.setUsesZeroDelay();
                यदि (!m_hasStaticZeroDelay) m_unknownDelayFlps.धकेलो_back(nodep->fileline());
            }
        }

        // Replace self with a 'co_await dlySched.delay(<valuep>)'
        AstCMethodHard* ध्रुव delayMethodp = नूतन AstCMethodHard{
            flp, नूतन AstVarRef{flp, getCreateDelayScheduler(), VAccess::WRITE},
            VCMethod::SCHED_DELAY, valuep};
        delayMethodp->dtypeSetVoid();
        addProcessInfo(delayMethodp);
        addDebugInfo(delayMethodp);
        // Create the co_await
        Ast_गाँठ* ध्रुव awaitp = नूतन AstCAwait{flp, delayMethodp, get_Create_Delay_Sen_वृक्ष()};
        // Relink child statements after the co_await
        यदि (Ast_गाँठ* ध्रुव stmtsp = nodep->stmtsp()) {
            awaitp->addNext(stmtsp->unlinkFrBackWithNext());
        }
        nodep->replaceWith(awaitp);
        VL_DO_DANGLING(nodep->delete_वृक्ष(), nodep);
    }
    शून्य visit(AstEventControl* nodep) override {
        // Do not allow waiting on local named events, as they get enqueued for clearing, but can
        // go out of scope before that happens
        यदि (!nodep->sentreep()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Event control with implicit sensitivity (@*)"
                          " in this context; use an explicit sensitivity list instead");
            VL_DO_DANGLING(धकेलो_Deletep(nodep->unlinkFrBack()), nodep);
            प्रत्यागम;
        }
        File_पंक्ति* ध्रुव flp = nodep->fileline();
        // Relink child statements after the event control
        यदि (nodep->stmtsp()) nodep->addNextHere(nodep->stmtsp()->unlinkFrBackWithNext());
        यदि (needDynamicTrigger(nodep->sentreep())) {
            // Create the trigger variable and init it with 0
            AstVarScope* ध्रुव trigvscp
                = createTemp(flp, m_dynTrigNames.get(nodep), nodep->findBitDType(), nodep);
            auto* ध्रुव initp = नूतन AstAssign{flp, नूतन AstVarRef{flp, trigvscp, VAccess::WRITE},
                                              नूतन AstConst{flp, AstConst::BitFalse{}}};
            // Await the eval step with the dynamic trigger scheduler. First, create the method
            // call
            auto* ध्रुव evalMethodp = नूतन AstCMethodHard{
                flp, नूतन AstVarRef{flp, getCreateDynamicTriggerScheduler(), VAccess::WRITE},
                VCMethod::SCHED_EVALUATION};
            evalMethodp->dtypeSetVoid();
            addProcessInfo(evalMethodp);
            auto* ध्रुव sentreep = nodep->sentreep();
            addEventDebugInfo(evalMethodp, sentreep);
            // Create the co_await
            AstCAwait* ध्रुव awaitEvalp
                = नूतन AstCAwait{flp, evalMethodp, get_Create_Dynamic_Trigger_Sen_वृक्ष()};
            // Construct the sen expression for this sentree
            UASSERT_OBJ(m_sen_व्यंजक_Builderp, nodep, "No SenExprBuilder for this scope");
            auto* ध्रुव assignp = नूतन AstAssign{flp, नूतन AstVarRef{flp, trigvscp, VAccess::WRITE},
                                                m_sen_व्यंजक_Builderp->build(sentreep).first};
            // Get the SenExprBuilder results
            ध्रुव Sen_व्यंजक_Builder::Results senResults = m_sen_व्यंजक_Builderp->getAndClearResults();
            localizeVars(m_procp, senResults.m_vars);
            // If post updates are destructive (e.g. clearFired on events), perform a
            // conservative pre-clear once before entering the wait loop so stale state from a
            // previous wait does not cause an immediate false-positive trigger.
            ध्रुव तार्किक hasDestructivePostUpdates = !senResults.m_destructivePostUpdates.empty();
            // Create the trigger eval loop, which will await the evaluation step and check the
            // trigger
            Ast_गाँठ_व्यंजक* ध्रुव condp
                = नूतन AstLogNot{flp, नूतन AstVarRef{flp, trigvscp, VAccess::READ}};
            AstLoop* ध्रुव loopp = नूतन AstLoop{flp};
            loopp->addStmtsp(नूतन AstLoopTest{flp, loopp, condp});
            loopp->addStmtsp(awaitEvalp);
            // Put pre updates before the trigger check and assignment
            प्रति (Ast_गाँठ_Stmt* ध्रुव stmtp : senResults.m_preUpdates) loopp->addStmtsp(stmtp);
            // Then the trigger check and assignment
            loopp->addStmtsp(assignp);
            // Let the dynamic trigger scheduler know if this trigger was set
            // If it was, a call to the scheduler's evaluate() will return true
            AstCMethodHard* ध्रुव anyTriggeredMethodp = नूतन AstCMethodHard{
                flp, नूतन AstVarRef{flp, getCreateDynamicTriggerScheduler(), VAccess::WRITE},
                VCMethod::SCHED_ANY_TRIGGERED, नूतन AstVarRef{flp, trigvscp, VAccess::READ}};
            anyTriggeredMethodp->dtypeSetVoid();
            loopp->addStmtsp(anyTriggeredMethodp->makeStmt());
            // If the post update is destructive (e.g. event vars are cleared), create an await for
            // the post update step
            यदि (hasDestructivePostUpdates) {
                AstCAwait* ध्रुव awaitPostUpdatep = awaitEvalp->clone_वृक्ष(असत्य);
                VN_AS(awaitPostUpdatep->exprp(), CMethodHard)->method(VCMethod::SCHED_POST_UPDATE);
                loopp->addStmtsp(awaitPostUpdatep);
            }
            // Put the post updates at the end of the loop
            प्रति (Ast_गाँठ_Stmt* ध्रुव stmtp : senResults.m_postUpdates) loopp->addStmtsp(stmtp);
            // Finally, await the resumption step in 'act'
            AstCAwait* ध्रुव awaitResumep = awaitEvalp->clone_वृक्ष(असत्य);
            VN_AS(awaitResumep->exprp(), CMethodHard)->method(VCMethod::SCHED_RESUMPTION);
            Ast_गाँठ::addNext<Ast_गाँठ_Stmt, Ast_गाँठ_Stmt>(loopp, awaitResumep);
            // Replace the event control with one explicit stmt chain:
            //   init -> [inits] -> [optional destructive pre-clears] -> loop -> awaitResumption
            Ast_गाँठ_Stmt* chainp = nullptr;
            chainp = Ast_गाँठ::addNextNull(chainp, initp);
            प्रति (Ast_गाँठ_Stmt* ध्रुव stmtp : senResults.m_inits) {
                chainp = Ast_गाँठ::addNextNull(chainp, stmtp);
            }
            यदि (hasDestructivePostUpdates) {
                प्रति (Ast_गाँठ_Stmt* ध्रुव stmtp : senResults.m_destructivePostUpdates) {
                    chainp = Ast_गाँठ::addNextNull(chainp, stmtp->clone_वृक्ष(असत्य));
                }
            }
            chainp = Ast_गाँठ::addNextNull(chainp, loopp);
            nodep->replaceWith(chainp);
        } अन्यथा {
            auto* ध्रुव sentreep = m_finder.get_Sen_वृक्ष(nodep->sentreep());
            nodep->sentreep()->unlinkFrBack()->delete_वृक्ष();
            // Get this sentree's trigger scheduler
            // Replace self with a 'co_await trigSched.trigger()'
            auto* ध्रुव triggerMethodp = नूतन AstCMethodHard{
                flp, नूतन AstVarRef{flp, getCreateTriggerSchedulerp(sentreep), VAccess::WRITE},
                VCMethod::SCHED_TRIGGER};
            triggerMethodp->dtypeSetVoid();
            // If it should be committed immediately, pass true, otherwise false
            triggerMethodp->addPinsp(nodep->user2() ? नूतन AstConst{flp, AstConst::BitTrue{}}
                                                    : नूतन AstConst{flp, AstConst::BitFalse{}});
            addProcessInfo(triggerMethodp);
            addEventDebugInfo(triggerMethodp, sentreep);
            // Create the co_await
            AstCAwait* ध्रुव awaitp = नूतन AstCAwait{flp, triggerMethodp, sentreep};
            nodep->replaceWith(awaitp);
        }
        VL_DO_DANGLING(nodep->delete_वृक्ष(), nodep);
    }
    शून्य visit(Ast_गाँठ_Assign* nodep) override {
        // Only process once to avoid infinite loops (due to the net delay)
        यदि (nodep->user1SetOnce()) प्रत्यागम;
        UINFO(9, "control-visit " << nodep);
        File_पंक्ति* ध्रुव flp = nodep->fileline();
        Ast_गाँठ* controlp = factorOutTimingControl(nodep);
        ध्रुव तार्किक inAssignDly = VN_IS(nodep, AssignDly);
        // Handle the intra assignment timing control
        // Transform if:
        // * there's a timing control in the assignment
        // * the assignment is an AssignDly and it's in a non-inlined function
        यदि (!controlp && (!inAssignDly || m_underProcedure)) {
            iterateChildren(nodep);
            प्रत्यागम;
        }
        // The timing control, if any, which is put in a fork for an NBA
        Ast_गाँठ* ध्रुव timingp = controlp;
        // Insert new vars before the timing control if we're in a function; in a process we can't
        // do that, as after V3Begin only modules and functions declare variables. These
        // intra-assignment vars will later be passed to forked processes by value.
        Ast_गाँठ* जोड़ो_Beforep = m_underProcedure ? nullptr : controlp;
        // Special case for NBA
        यदि (inAssignDly) {
            // Put it in a fork so it doesn't block
            // Could already be the only thing directly under a fork, reuse that if possible
            AstFork* forkp = !nodep->nextp() ? VN_CAST(nodep->firstAbovep(), Fork) : nullptr;
            यदि (!forkp) forkp = नूतन AstFork{flp, VJoinType::JOIN_NONE};
            यदि (!m_underProcedure) {
                // If it's in a function, it won't be handled by V3Delayed
                // Put it behind an additional named event that gets triggered in the NBA region
                AstEventControl* ध्रुव nbaEventControlp = createNbaEventControl(flp);
                AstAssign* ध्रुव trigAssignp = createNbaEventTriggerAssignment(flp);
                nodep->replaceWith(trigAssignp);
                trigAssignp->addNextHere(nbaEventControlp);
                nbaEventControlp->addStmtsp(nodep);
                जोड़ो_Beforep = forkp;
                यदि (!controlp) controlp = nbaEventControlp;
            }
            controlp->replaceWith(forkp);
            AstBegin* beginp = VN_CAST(controlp, Begin);
            यदि (!beginp) beginp = नूतन AstBegin{nodep->fileline(), "", controlp, असत्य};
            forkp->addForksp(beginp);
            addFlags(beginp, T_NBA_UPDATE);
            controlp = forkp;
        }
        UASSERT_OBJ(nodep, controlp, "Assignment should have timing control");
        // Move `insertBeforep` into `AstCLocalScope` if necessary to avoid jumping over
        // a variable initialization that would be inserted before `insertBeforep`. All
        // access to this variable should be contained within returned `AstCLocalScope`.
        यदि (m_underJumpBlock) addCLocalScope(flp, जोड़ो_Beforep);
        // Function for replacing values with intermediate variables
        ध्रुव auto replaceWithIntermediate = [&](Ast_गाँठ_व्यंजक* ध्रुव valuep,
                                                 ध्रुव std::string& name) {
            AstVarScope* ध्रुव newvscp = createTemp(flp, name, valuep->dtypep(), जोड़ो_Beforep);
            // A pending update keeps its own value, even if its process schedules more
            यदि (inAssignDly && !जोड़ो_Beforep) newvscp->fork_By_मूल्य(सत्य);
            valuep->replaceWith(नूतन AstVarRef{flp, newvscp, VAccess::READ});
            controlp->addHereThisAsNext(
                नूतन AstAssign{flp, नूतन AstVarRef{flp, newvscp, VAccess::WRITE}, valuep});
        };
        // NBAs with delays evaluate the indices selecting the target now (IEEE 1800-2023 10.4.2),
        // in order, each as a whole, also the selects and function calls in it. So follow the
        // selects of the target up from the variable, not into their indices. As an example, this
        // transforms 'x[a[i]][f(j)] <= #1 y' into 't0 = a[i]; t1 = f(j); x[t0][t1] <= #1 y'.
        // V3Const has split NBAs to concatenations, also streamed, into NBAs to each part.
        यदि (inAssignDly) {
            प्रति (Ast_गाँठ* exprp = nodep->lhsp()->baseFromp(सत्य); exprp != nodep;
                 exprp = exprp->firstAbovep()) {
                यदि (AstSel* ध्रुव selp = VN_CAST(exprp, Sel)) {
                    // widthp should be const
                    यदि (!VN_IS(selp->lsbp(), Const)) {
                        replaceWithIntermediate(selp->lsbp(), m_intraLsbNames.get(nodep));
                    }
                } अन्यथा यदि (Ast_गाँठ_Sel* ध्रुव selp = VN_CAST(exprp, गाँठ_Sel)) {
                    यदि (!VN_IS(selp->bitp(), Const)) {
                        replaceWithIntermediate(selp->bitp(), m_intraIndexNames.get(nodep));
                    }
                }
            }
        }
        // Replace the RHS with an intermediate value var
        replaceWithIntermediate(nodep->rhsp(), m_intra_मूल्य_Names.get(nodep));
        यदि (!inAssignDly) प्रत्यागम;
        // The process executing the NBA evaluates its delay (IEEE 1800-2023 4.9.4), not the forked
        // process, if it has side effects, e.g. calling a function that checks its process. A pure
        // one is evaluated as the forked process starts, at once.
        यदि (AstDelay* ध्रुव delayp = VN_CAST(timingp, Delay)) {
            यदि (!delayp->lhsp()->isPure()) {
                replaceWithIntermediate(delayp->lhsp(), m_intra_मूल्य_Names.get(nodep));
            }
        }
    }
    शून्य visit(AstFireEvent* nodep) override {
        // V3Delayed handles '->>' of a variable in a process. Like NBAs in non-inlined functions,
        // trigger other events from a fork awaiting the NBA region.
        यदि (!nodep->isDelayed() || (m_underProcedure && VN_IS(nodep->operandp(), VarRef))) {
            iterateChildren(nodep);
            प्रत्यागम;
        }
        File_पंक्ति* ध्रुव flp = nodep->fileline();
        AstAssign* ध्रुव trigAssignp = createNbaEventTriggerAssignment(flp);
        nodep->replaceWith(trigAssignp);
        AstFork* ध्रुव forkp = नूतन AstFork{flp, VJoinType::JOIN_NONE};
        trigAssignp->addNextHere(forkp);
        यदि (m_underJumpBlock) addCLocalScope(flp, forkp);
        // The triggered event is the one referenced now, so evaluate handles and indices now
        Ast_गाँठ_व्यंजक* ध्रुव eventp = nodep->operandp()->unlinkFrBack();
        ध्रुव auto evalNow = [&](Ast_गाँठ_व्यंजक* ध्रुव valuep, V3UniqueNames& names) {
            // Like for an NBA, a process uses a module variable, passed to the fork by value
            AstVarScope* ध्रुव vscp = createTemp(flp, names.get(nodep), valuep->dtypep(),
                                                 m_underProcedure ? nullptr : forkp);
            यदि (m_underProcedure) vscp->fork_By_मूल्य(सत्य);
            valuep->replaceWith(नूतन AstVarRef{flp, vscp, VAccess::READ});
            // Unlike an event, a handle selecting one is only read
            valuep->foreach([](Ast_गाँठ* ध्रुव np) {
                यदि (Ast_गाँठ_Var_Ref* ध्रुव refp = VN_CAST(np, गाँठ_Var_Ref)) {
                    refp->access(VAccess::READ);
                } अन्यथा यदि (AstMemberSel* ध्रुव selp = VN_CAST(np, MemberSel)) {
                    selp->access(VAccess::READ);
                }
            });
            forkp->addHereThisAsNext(
                नूतन AstAssign{flp, नूतन AstVarRef{flp, vscp, VAccess::WRITE}, valuep});
        };
        Ast_गाँठ_व्यंजक* refp = eventp;
        यावत् (Ast_गाँठ_Sel* ध्रुव selp = VN_CAST(refp, गाँठ_Sel)) {
            यदि (!VN_IS(selp->bitp(), Const)) evalNow(selp->bitp(), m_intraIndexNames);
            refp = selp->fromp();
        }
        // Handle of a class (or virtual interface)
        यदि (AstMemberSel* ध्रुव selp = VN_CAST(refp, MemberSel)) {
            evalNow(selp->fromp(), m_intra_मूल्य_Names);
        }
        AstEventControl* ध्रुव controlp = createNbaEventControl(flp);
        controlp->addStmtsp(नूतन AstFireEvent{flp, eventp, असत्य});
        AstBegin* ध्रुव beginp = नूतन AstBegin{flp, "", controlp, असत्य};
        addFlags(beginp, T_NBA_UPDATE);
        forkp->addForksp(beginp);
        VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
    }
    शून्य visit(AstAssignW* nodep) override {
        File_पंक्ति* ध्रुव flp = nodep->fileline();
        // Get the net delay unless this assignment was created for handling the net delay (user1)
        AstDelay* ध्रुव netDelayp = nodep->user1() ? nullptr : nodep->getLhsNetDelay();
        यदि (netDelayp) {
            यदि (nodep->timingControlp()) {
                // If this assignment has a delay, create another one to handle the net delay
                AstVarScope* ध्रुव newvscp = m_assignWTmps.make(flp, m_scopep, nodep->dtypep());
                AstAssignW* assignp = नूतन AstAssignW{
                    nodep->fileline(), nodep->lhsp()->unlinkFrBack(),
                    नूतन AstVarRef{flp, newvscp, VAccess::READ}, netDelayp->clone_वृक्ष(असत्य)};
                assignp->user1(सत्य);
                nodep->addNextHere(assignp);
                nodep->lhsp(नूतन AstVarRef{flp, newvscp, VAccess::WRITE});
            } अन्यथा {
                // Else just use this one with the net delay
                nodep->timingControlp(netDelayp->clone_वृक्ष(असत्य));
            }
        }
        यदि (!nodep->timingControlp()) प्रत्यागम;
        // There will be some circular logic here, suppress the warning for newly created vars
        flp->modifyWarnOff(V3ErrorCode::UNOPTFLAT, सत्य);
        // Also suppress the warning for the LHS var, for cases like `assign #1 clk = ~clk;`
        // TODO: Restore the warning for other, non-delayed drivers
        nodep->lhsp()->foreach([](AstVarRef* refp) {
            यदि (refp->access().isWriteOrRW()) {
                refp->varp()->fileline()->modifyWarnOff(V3ErrorCode::UNOPTFLAT, सत्य);
            }
        });
        // Should be under an always
        AstAlways* ध्रुव alwaysp = VN_AS(m_procp, Always);
        // Convert it to an Assign; the new assign with intra delay will be handled by
        // visit(AstNodeAssign*)
        Ast_गाँठ_व्यंजक* ध्रुव lhs1p = nodep->lhsp()->unlinkFrBack();
        Ast_गाँठ_व्यंजक* ध्रुव rhs1p = nodep->rhsp()->unlinkFrBack();
        Ast_गाँठ* ध्रुव controlp = nodep->timingControlp()->unlinkFrBack();
        यदि (AstDelay* ध्रुव delayp = VN_CAST(controlp, Delay)) {
            यदि (Ast_गाँठ_व्यंजक* fallDelayp = delayp->fallDelay()) {
                fallDelayp = fallDelayp->unlinkFrBack();
                Ast_गाँठ_व्यंजक* lhsp = delayp->lhsp()->unlinkFrBack();
                // Use fall only for an all-zero value, rise otherwise.
                lhsp = delayToIntegral(delayp, lhsp);
                fallDelayp = delayToIntegral(delayp, fallDelayp);
                delayp->lhsp(
                    नूतन AstCond{flp, नूतन AstEq{flp, rhs1p->clone_वृक्ष(असत्य), नूतन AstConst{flp, 0}},
                                fallDelayp, lhsp});
            }
        }
        AstAssign* ध्रुव assignp = नूतन AstAssign{nodep->fileline(), lhs1p, rhs1p, controlp};
        // Put the assignment in a fork..join_none.
        AstFork* ध्रुव forkp = नूतन AstFork{flp, VJoinType::JOIN_NONE};
        nodep->replaceWith(forkp);
        VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
        AstBegin* ध्रुव beginp = नूतन AstBegin{flp, "", assignp, असत्य};
        forkp->addForksp(beginp);
        visit(forkp);  // Visit now as we need to do some post-processing
        // IEEE 1800-2023 10.3.3 - if the RHS value differs from the currently scheduled value to
        // be assigned, the currently scheduled assignment is descheduled. To keep track if an
        // assignment should be descheduled, each scheduled assignment event has a 'generation',
        // and if at assignment time its generation differs from the current generation, it won't
        // be performed
        AstAssign* ध्रुव preAssignp = VN_AS(beginp->stmtsp(), Assign);
        UASSERT_OBJ(preAssignp, alwaysp, "Pre-assign should be there from visit(AstFork)");
        AstAssign* ध्रुव postAssignp = VN_AS(preAssignp->nextp()->nextp(), Assign);
        UASSERT_OBJ(postAssignp, alwaysp, "Post-assign should be there from visit(AstFork)");
        // Increment generation and copy it to a local
        AstVarScope* ध्रुव generationVarp
            = m_assignWGens.make(flp, m_scopep, alwaysp->findUInt64DType());
        AstVarScope* ध्रुव genLocalVarp
            = createTemp(flp, generationVarp->varp()->name() + "__local",
                         alwaysp->findUInt64DType(), preAssignp);
        preAssignp->addHereThisAsNext(
            नूतन AstAssign{flp, नूतन AstVarRef{flp, generationVarp, VAccess::WRITE},
                          नूतन AstAdd{flp, नूतन AstVarRef{flp, generationVarp, VAccess::READ},
                                     नूतन AstConst{flp, 1}}});
        preAssignp->addHereThisAsNext(
            नूतन AstAssign{flp, नूतन AstVarRef{flp, genLocalVarp, VAccess::WRITE},
                          नूतन AstVarRef{flp, generationVarp, VAccess::READ}});
        // If the current generation is same as the one saved in the local var, assign
        beginp->addStmtsp(
            नूतन AstIf{flp,
                      नूतन AstEq{flp, नूतन AstVarRef{flp, generationVarp, VAccess::READ},
                                नूतन AstVarRef{flp, genLocalVarp, VAccess::READ}},
                      postAssignp->unlinkFrBack()});
        // Save scheduled RHS value before delay
        AstVarScope* ध्रुव tmpVarp
            = m_assignWTmps.make(flp, m_scopep, preAssignp->rhsp()->dtypep());
        AstVarRef* ध्रुव tmpAssignRhsp = VN_AS(preAssignp->lhsp(), VarRef)->clone_वृक्ष(असत्य);
        tmpAssignRhsp->access(VAccess::WRITE);
        preAssignp->addNextHere(
            नूतन AstAssign{flp, नूतन AstVarRef{flp, tmpVarp, VAccess::WRITE}, tmpAssignRhsp});
        // If the RHS is different from the currently scheduled value, schedule the new assignment
        // The generation will increase, effectively 'descheduling' the previous assignment.
        Ast_गाँठ_व्यंजक* ध्रुव didNotInitp
            = नूतन AstLogNot{flp, नूतन AstCExpr{flp, "vlSymsp->__Vm_didInit", 1}};
        AstVarRef* ध्रुव firstIterp
            = नूतन AstVarRef{flp, m_netlistp->stlFirstIterationp(), VAccess::READ};
        Ast_गाँठ_व्यंजक* ध्रुव schedCondp
            = नूतन AstLogOr{flp,
                           नूतन AstNeq{flp, preAssignp->rhsp()->clone_वृक्ष(असत्य),
                                      नूतन AstVarRef{flp, tmpVarp, VAccess::READ}},
                           नूतन AstLogAnd{flp, didNotInitp, firstIterp}};
        alwaysp->addStmtsp(नूतन AstIf{flp, schedCondp, forkp->unlinkFrBack()});
    }
    शून्य visit(AstDisableFork* nodep) override {
        यदि (m_hasProcess) प्रत्यागम;
        // never reached by any process; remove to avoid compilation error
        VL_DO_DANGLING(nodep->unlinkFrBack()->delete_वृक्ष(), nodep);
    }
    शून्य visit(AstWaitFork* nodep) override {
        यदि (m_hasProcess) {
            File_पंक्ति* ध्रुव flp = nodep->fileline();
            AstCExpr* ध्रुव exprp = नूतन AstCExpr{flp, "vlProcess->completedFork()", 1};
            AstWait* ध्रुव waitp = नूतन AstWait{flp, exprp, nullptr};
            nodep->replaceWith(waitp);
        } अन्यथा {
            // never reached by any process; remove to avoid compilation error
            nodep->unlinkFrBack();
        }
        VL_DO_DANGLING(nodep->delete_वृक्ष(), nodep);
    }
    शून्य visit(AstWait* nodep) override {
        // Wait on changed events related to the vars in the wait statement
        UINFO(9, "control-visit " << nodep);
        File_पंक्ति* ध्रुव flp = nodep->fileline();
        Ast_गाँठ* ध्रुव stmtsp = nodep->stmtsp();
        यदि (stmtsp) stmtsp->unlinkFrBackWithNext();
        Ast_गाँठ_व्यंजक* ध्रुव condp = V3Const::constifyEdit(nodep->condp()->unlinkFrBack());
        auto* ध्रुव constp = VN_CAST(condp, Const);
        यदि (constp) {
            यदि (constp->isZero()) {
                // We have to await forever instead of simply returning in case we're deep in a
                // callstack
                AstCMethodHard* ध्रुव foreverMethodp = नूतन AstCMethodHard{
                    flp, नूतन AstVarRef{flp, getCreateDelayScheduler(), VAccess::WRITE},
                    VCMethod::SCHED_WAIT_FOREVER};
                foreverMethodp->dtypeSetVoid();
                addProcessInfo(foreverMethodp);
                addDebugInfo(foreverMethodp);
                AstCAwait* ध्रुव awaitp = नूतन AstCAwait{flp, foreverMethodp};
                nodep->replaceWith(awaitp);
                यदि (stmtsp) VL_DO_DANGLING(stmtsp->delete_वृक्ष(), stmtsp);
                VL_DO_DANGLING(condp->delete_वृक्ष(), condp);
            } अन्यथा {
                nodep->v3fatalSrc("constant wait should have been removed in "
                                  "TimingSuspendableVisitor::visit(AstWait)");
            }
        } अन्यथा यदि (needDynamicTrigger(condp)) {
            // No point in making a sentree, just use the expression as sensitivity
            // Put the event control in an if so we only wait if the condition isn't met already
            auto* ध्रुव ifp = नूतन AstIf{
                flp, नूतन AstLogNot{flp, condp},
                नूतन AstEventControl{flp,
                                    नूतन Ast_Sen_वृक्ष{flp, नूतन AstSenItem{flp, VEdgeType::ET_TRUE,
                                                                       condp->clone_वृक्ष(असत्य)}},
                                    nullptr}};
            यदि (stmtsp) Ast_गाँठ::addNext<Ast_गाँठ, Ast_गाँठ>(ifp, stmtsp);
            nodep->replaceWith(ifp);
        } अन्यथा {
            // We are using a global sentree, so we cannot use ET_TRUE, as that could lead to the
            // active region never converging. Because of this, we need to use ET_CHANGED in a
            // loop.
            AstEventControl* ध्रुव controlp = नूतन AstEventControl{
                flp,
                नूतन Ast_Sen_वृक्ष{
                    flp, नूतन AstSenItem{flp, VEdgeType::ET_CHANGED, condp->clone_वृक्ष(असत्य)}},
                nullptr};
            controlp->user2(सत्य);  // Commit immediately
            AstLoop* ध्रुव loopp = नूतन AstLoop{flp};
            loopp->addStmtsp(नूतन AstLoopTest{flp, loopp, नूतन AstLogNot{flp, condp}});
            loopp->addStmtsp(controlp);
            यदि (stmtsp) Ast_गाँठ::addNext<Ast_गाँठ, Ast_गाँठ>(loopp, stmtsp);
            nodep->replaceWith(loopp);
        }
        VL_DO_DANGLING(nodep->delete_वृक्ष(), nodep);
    }
    शून्य visit(AstBegin* nodep) override {
        VL_RESTORER(m_procp);
        VL_RESTORER(m_hasProcess);
        m_hasProcess |= hasFlags(nodep, T_HAS_PROC);
        // A pending NBA update is not a subprocess of the process that scheduled it, so it is
        // unaffected by 'disable fork' and 'wait fork' (IEEE 1800-2023 9.6.1, 9.6.3)
        यदि (hasFlags(nodep, T_NBA_UPDATE)) m_hasProcess = असत्य;
        m_procp = nodep;
        यदि (m_hasProcess) nodep->setNeedProcess();
        iterateChildren(nodep);
    }
    शून्य visit(AstFork* nodep) override {
        यदि (nodep->user1SetOnce()) प्रत्यागम;
        UINFO(9, "control-visit " << nodep);
        v3Global.setUsesTiming();

        // Create a unique name for this fork
        nodep->name("__Vfork_" + std::to_string(++m_forkCnt));

        // TODO: Should process nodep->stmtsp() in case an earlier pass
        //       inserted something there, but as of today we don't need to.

        // Process and name each fork
        size_t idx = 0;  // Index for naming begins
        प्रति (AstBegin *itemp = nodep->forksp(), *nextp; itemp; itemp = nextp) {
            nextp = VN_AS(itemp->nextp(), Begin);
            iterate(itemp);
            // Note: Even if we do not find any awaits, we cannot simply inline
            // the process here, as new awaits could be added later.

            // Name the begin (later the name will be used for a new function)
            itemp->name(nodep->name() + "__" + std::to_string(idx++));
            यदि (itemp->needProcess()) {
                itemp->addStmtsp(
                    नूतन AstCStmt{itemp->fileline(), "vlProcess->state(VlProcess::FINISHED);"});
            }
        }
        यदि (!nodep->joinType().joinNone()) makeForkJoin(nodep);
    }

    //--------------------
    शून्य visit(AstVar*) override {}  // Accelerate
    शून्य visit(Ast_गाँठ* nodep) override { iterateChildren(nodep); }

सार्वजनिक:
    // CONSTRUCTORS
    explicit TimingControlVisitor(AstNetlist* nodep)
        : m_netlistp{nodep} {
        iterate(nodep);

        // If there is no static #0 in the design, but an unknown delay was found,
        // and the user did not specify preference, then warn on all unknown delays
        // as we will be assuming they can be #0, which can cause performance degradation.
        प्रति (File_पंक्ति* ध्रुव flp : m_unknownDelayFlps) {
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
    File_पंक्ति* ध्रुव flp = valuep->fileline();
    यदि (ध्रुव AstConst* ध्रुव constp = VN_CAST(valuep, Const)) {
        यदि (constp->अंक().is1Step()) {
            // A step is one tick of the global time precision (IEEE 1800-2023 3.14.3)
            VL_DO_DANGLING(valuep->delete_वृक्ष(), valuep);
            AstConst* ध्रुव tickp = नूतन AstConst{flp, AstConst::Unsized64{}, 1};
            tickp->dtypeSetBitSized(64, VSigning::UNSIGNED);
            प्रत्यागम tickp;
        }
        यदि (constp->isZero()) प्रत्यागम valuep;
    }
    UASSERT_OBJ(!timeunit.isNone(), valuep, "timeunit must be set");
    ध्रुव द्विगुण timescaleFactorD
        = std::pow(10.0, timeunit.powerOfTen() - v3Global.rootp()->timeprecision().powerOfTen());
    यदि (valuep->dtypep()->skipRefp()->isDouble()) {
        AstConst* ध्रुव tsfp = नूतन AstConst{flp, AstConst::RealDouble{}, timescaleFactorD};
        valuep = नूतन AstMulD{flp, valuep, tsfp};
        valuep = नूतन AstRToIRoundS{flp, valuep};
        valuep->dtypeSetBitSized(64, VSigning::UNSIGNED);
    } अन्यथा {
        valuep->dtypeSetBitSized(64, VSigning::UNSIGNED);
        ध्रुव uint64_t timescaleFactorU = static_cast<uint64_t>(timescaleFactorD);
        AstConst* ध्रुव tsfp = नूतन AstConst{flp, AstConst::Unsized64{}, timescaleFactorU};
        valuep = नूतन AstMul{flp, valuep, tsfp};
    }
    प्रत्यागम V3Const::constifyEdit(valuep);  // Simplify
}

शून्य V3Timing::timingAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        ध्रुव VNUser1InUse m_user1InUse;
        ध्रुव VNUser2InUse m_user2InUse;
        { TimingSuspendableVisitor{nodep}; }
        यदि (v3Global.usesTiming()) TimingControlVisitor{nodep};
    }
    V3Global::dump_Check_Global_वृक्ष("timing", 0, dump_वृक्ष_Either_Level() >= 3);
}
