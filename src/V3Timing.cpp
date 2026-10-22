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

#അന്തര്ഭാവ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#അന്തര്ഭാവ "V3Timing.h"

#അന്തര്ഭാവ "V3Const.h"
#അന്തര്ഭാവ "V3EmitV.h"
#അന്തര്ഭാവ "V3File.h"
#അന്തര്ഭാവ "V3Global.h"
#അന്തര്ഭാവ "V3Graph.h"
#അന്തര്ഭാവ "V3MemberMap.h"
#അന്തര്ഭാവ "V3SenExprBuilder.h"
#അന്തര്ഭാവ "V3SenTree.h"
#അന്തര്ഭാവ "V3SharedTmps.h"
#അന്തര്ഭാവ "V3Stats.h"
#അന്തര്ഭാവ "V3UniqueNames.h"

#അന്തര്ഭാവ <limits>
#അന്തര്ഭാവ <queue>

VL_DEFINE_DEBUG_FUNCTIONS;

// ######################################################################

എണ്ണൽ കണ്ണി_Flag : uint8_t {
    T_SUSPENDEE = 1 << 0,  // Suspendable (due to dependence on another suspendable)
    T_SUSPENDER = 1 << 1,  // Suspendable (has timing control)
    T_ALLOCS_PROC = 1 << 2,  // Can allocate VlProcess
    T_FORCES_PROC = 1 << 3,  // Forces VlProcess allocation
    T_NEEDS_PROC = 1 << 4,  // Needs access to VlProcess if it's allocated
    T_HAS_PROC = 1 << 5,  // Has VlProcess argument in the signature
    T_NBA_UPDATE = 1 << 6,  // Fork branch of a pending NBA update, which is not a subprocess
};

എണ്ണൽ ForkType : uint8_t {
    F_NONE = 0,  // Not under a fork
    F_MIGHT_SUSPEND = 1 << 0,  // Fork might suspend the execution of current process
    F_MIGHT_NEED_PROC = 1 << 1,  // Fork might need a process (any fork really)
};

എണ്ണൽ PropagationType : uint8_t {
    P_CALL = 1,  // Propagation through call to a function/task/method
    P_FORK = 2,  // Propagation due to fork's behavior
    P_SIGNATURE = 3,  // Propagation required to maintain C++ function's signature requirements
};

// Add timing flag to a node
സ്ഥിതം ശൂന്യം addFlags(Ast_കണ്ണി* സ്ഥിരം nodep, uint8_t flags) { nodep->user2Or(flags); }
// Check if a node has ALL of the expected flags set
സ്ഥിതം തർക്കം hasFlags(Ast_കണ്ണി* സ്ഥിരം nodep, uint8_t flags) { തിരികെ !(~nodep->user2() & flags); }

// ######################################################################
//  Detect nodes affected by timing and/or requiring a process

വർഗ്ഗം TimingSuspendableVisitor final : പൊതു VNVisitor {
    // TYPES
    // Vertex of a dependency graph of suspendable nodes, e.g. if a node (process or task) is
    // suspendable, all its dependents should also be suspendable
    വർഗ്ഗം DepVtx VL_NOT_FINAL : പൊതു V3GraphVertex {
        VL_RTTI_IMPL(DepVtx, V3GraphVertex)
        AstClass* സ്ഥിരം m_classp;  // Class associated with a method
        Ast_കണ്ണി* സ്ഥിരം m_nodep;  // AST node represented by this graph vertex

        // ACCESSORS
        string name() സ്ഥിരം override {
            എങ്കിൽ (m_classp) {
                എങ്കിൽ (VN_IS(nodep(), CFunc)) {
                    തിരികെ cvtToHex(nodep()) + ' ' + classp()->name() + "::" + nodep()->name();
                }
            }
            തിരികെ cvtToHex(nodep()) + ' ' + nodep()->prettyTypeName();
        }
        File_വരി* fileline() സ്ഥിരം override { തിരികെ nodep()->fileline(); }

    പൊതു:
        // CONSTRUCTORS
        DepVtx(V3Graph* graphp, Ast_കണ്ണി* nodep, AstClass* classp)
            : V3GraphVertex{graphp}
            , m_classp{classp}
            , m_nodep{nodep} {}
        ~DepVtx() override = സ്ഥിരസ്ഥിതി;

        // ACCESSORS
        മായാ Ast_കണ്ണി* nodep() സ്ഥിരം VL_MT_STABLE { തിരികെ m_nodep; }
        മായാ Ast_കണ്ണി* classp() സ്ഥിരം VL_MT_STABLE { തിരികെ m_classp; }
    };

    വർഗ്ഗം SuspendDepVtx final : പൊതു DepVtx {
        VL_RTTI_IMPL(SuspendDepVtx, DepVtx)
        string dotColor() സ്ഥിരം override {
            എങ്കിൽ (hasFlags(nodep(), T_SUSPENDER)) തിരികെ "red";
            എങ്കിൽ (hasFlags(nodep(), T_SUSPENDEE)) തിരികെ "blue";
            തിരികെ "black";
        }

    പൊതു:
        SuspendDepVtx(V3Graph* graphp, Ast_കണ്ണി* nodep, AstClass* classp)
            : DepVtx{graphp, nodep, classp} {}
        ~SuspendDepVtx() override = സ്ഥിരസ്ഥിതി;
    };

    വർഗ്ഗം NeedsProcDepVtx final : പൊതു DepVtx {
        VL_RTTI_IMPL(NeedsProcDepVtx, DepVtx)
        string dotColor() സ്ഥിരം override {
            എങ്കിൽ (hasFlags(nodep(), T_HAS_PROC)) തിരികെ "blue";
            എങ്കിൽ (hasFlags(nodep(), T_NEEDS_PROC)) തിരികെ "green";
            എങ്കിൽ (hasFlags(nodep(), T_FORCES_PROC)) തിരികെ "red";
            തിരികെ "black";
        }

    പൊതു:
        NeedsProcDepVtx(V3Graph* graphp, Ast_കണ്ണി* nodep, AstClass* classp)
            : DepVtx{graphp, nodep, classp} {}
        ~NeedsProcDepVtx() override = സ്ഥിരസ്ഥിതി;
    };

    // NODE STATE
    //  AstClass::user1() -> bool.  Class member cache has been refreshed.
    //  Ast{NodeProcedure,CFunc,Begin}::user2()  -> uint64_t.  Set to >= T_SUSP if
    //                                                         process/task suspendable
    //                                                         and to T_PROC if it
    //                                                         needs process metadata.
    //  Ast{NodeProcedure,CFunc,Begin}::user3()  -> DependencyVertex*.  Vertex in m_suspGraph
    //  Ast{NodeProcedure,CFunc,Begin}::user3()  -> DependencyVertex*.  Vertex in m_procGraph
    സ്ഥിരം VNUser3InUse m_user3InUse;
    സ്ഥിരം VNUser4InUse m_user4InUse;

    // STATE
    VMemberMap m_memberMap;  // Member names cached for fast lookup
    AstClass* m_classp = nullptr;  // Current class
    Ast_കണ്ണി* m_procp = nullptr;  // NodeProcedure/CFunc/Begin we're under
    uint8_t m_underFork = F_NONE;  // F_NONE or flags of a fork we are under
    V3Graph m_suspGraph;  // Dependency graph where a node is a dependency of another if it being
                          // suspendable makes the other node suspendable
    V3Graph m_procGraph;  // Dependency graph where a node is a dependency of another if it being
                          // suspendable makes the other node suspendable

    // METHODS
    // Get or create the dependency vertex for the given node
    DepVtx* getSuspendDepVtx(Ast_കണ്ണി* സ്ഥിരം nodep) {
        AstClass* classp = nullptr;
        എങ്കിൽ (AstCFunc* funcp = VN_CAST(nodep, CFunc)) {
            എങ്കിൽ (funcp->scopep() && funcp->scopep()->modp()) {
                classp = VN_CAST(funcp->scopep()->modp(), Class);
            }
        }
        എങ്കിൽ (!nodep->user3p()) nodep->user3p(പുതിയ SuspendDepVtx{&m_suspGraph, nodep, classp});
        തിരികെ nodep->user3u().to<SuspendDepVtx*>();
    }
    DepVtx* getNeedsProcDepVtx(Ast_കണ്ണി* സ്ഥിരം nodep) {
        AstClass* classp = nullptr;
        എങ്കിൽ (AstCFunc* funcp = VN_CAST(nodep, CFunc)) {
            എങ്കിൽ (funcp->scopep() && funcp->scopep()->modp()) {
                classp = VN_CAST(funcp->scopep()->modp(), Class);
            }
        }
        എങ്കിൽ (!nodep->user4p()) nodep->user4p(പുതിയ NeedsProcDepVtx{&m_procGraph, nodep, classp});
        തിരികെ nodep->user4u().to<NeedsProcDepVtx*>();
    }
    // Pass timing flag between nodes
    തർക്കം passFlag(സ്ഥിരം Ast_കണ്ണി* from, Ast_കണ്ണി* to, കണ്ണി_Flag flag) {
        എങ്കിൽ ((from->user2() & flag) && !(to->user2() & flag)) {
            addFlags(to, flag);
            തിരികെ സത്യം;
        }
        തിരികെ അസത്യം;
    }
    // Propagate flag to all nodes that depend on the given one
    ശൂന്യം propagateFlags(DepVtx* സ്ഥിരം vxp, കണ്ണി_Flag flag) {
        auto* സ്ഥിരം parentp = vxp->nodep();
        വേണ്ടി (V3GraphEdge& edge : vxp->outEdges()) {
            auto* സ്ഥിരം depVxp = static_cast<DepVtx*>(edge.മുകൾ());
            Ast_കണ്ണി* സ്ഥിരം depp = depVxp->nodep();
            എങ്കിൽ (passFlag(parentp, depp, flag)) propagateFlags(depVxp, flag);
        }
    }
    template <typename T_Predicate>
    ശൂന്യം propagateFlagsIf(DepVtx* സ്ഥിരം vxp, കണ്ണി_Flag flag, T_Predicate p) {
        auto* സ്ഥിരം parentp = vxp->nodep();
        വേണ്ടി (V3GraphEdge& edge : vxp->outEdges()) {
            auto* സ്ഥിരം depVxp = static_cast<DepVtx*>(edge.മുകൾ());
            Ast_കണ്ണി* സ്ഥിരം depp = depVxp->nodep();
            എങ്കിൽ (p(&edge) && passFlag(parentp, depp, flag)) propagateFlagsIf(depVxp, flag, p);
        }
    }
    template <typename T_Predicate>
    ശൂന്യം propagateFlagsReversedIf(DepVtx* സ്ഥിരം vxp, കണ്ണി_Flag flag, T_Predicate p) {
        auto* സ്ഥിരം parentp = vxp->nodep();
        വേണ്ടി (V3GraphEdge& edge : vxp->inEdges()) {
            auto* സ്ഥിരം depVxp = static_cast<DepVtx*>(edge.fromp());
            Ast_കണ്ണി* സ്ഥിരം depp = depVxp->nodep();
            എങ്കിൽ (p(&edge) && passFlag(parentp, depp, flag))
                propagateFlagsReversedIf(depVxp, flag, p);
        }
    }

    // VISITORS
    ശൂന്യം visit(AstClass* nodep) override {
        UASSERT_OBJ(!m_classp, nodep, "Class under class");
        VL_RESTORER(m_classp);
        m_classp = nodep;
        iterateChildren(nodep);
    }
    ശൂന്യം visit(Ast_കണ്ണി_Procedure* nodep) override {
        VL_RESTORER(m_procp);
        m_procp = nodep;
        getNeedsProcDepVtx(nodep);
        addFlags(nodep, T_ALLOCS_PROC);
        എങ്കിൽ (VN_IS(nodep, Always)) {
            UINFO(9, "Always does " << (nodep->needProcess() ? "" : "NOT ") << "need process");
        }
        iterateChildren(nodep);
    }
    ശൂന്യം visit(AstDisableFork* nodep) override {
        visit(static_cast<Ast_കണ്ണി*>(nodep));
        addFlags(m_procp, T_FORCES_PROC | T_NEEDS_PROC);
    }
    ശൂന്യം visit(AstWaitFork* nodep) override {
        visit(static_cast<Ast_കണ്ണി*>(nodep));
        addFlags(m_procp, T_FORCES_PROC | T_NEEDS_PROC);
    }
    ശൂന്യം visit(AstWait* nodep) override {
        UINFO(9, "suspendable-visit " << nodep);
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം condp = V3Const::constifyEdit(nodep->condp());
        എങ്കിൽ (AstConst* സ്ഥിരം constp = VN_CAST(condp, Const)) {
            എങ്കിൽ (!nodep->fileline()->warnIsOff(V3ErrorCode::WAITCONST)) {
                condp->v3warn(WAITCONST, "Wait statement condition is constant");
            }
            എങ്കിൽ (!constp->isZero()) {
                // Remove AstWait before we track process as T_SUSPENDER
                എങ്കിൽ (Ast_കണ്ണി* സ്ഥിരം stmtsp = nodep->stmtsp()) {
                    stmtsp->unlinkFrBackWithNext();
                    nodep->replaceWith(stmtsp);
                } അല്ലെങ്കിൽ {
                    nodep->unlinkFrBack();
                }
                VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
                തിരികെ;
            }
        }
        v3Global.setUsesTiming();
        എങ്കിൽ (m_procp) addFlags(m_procp, T_SUSPENDEE | T_SUSPENDER | T_NEEDS_PROC);
        iterateChildren(nodep);
    }
    ശൂന്യം visit(AstCFunc* nodep) override {
        VL_RESTORER(m_procp);
        m_procp = nodep;
        iterateChildren(nodep);
        എങ്കിൽ (nodep->needProcess()) addFlags(nodep, T_FORCES_PROC | T_NEEDS_PROC);
        DepVtx* സ്ഥിരം sVxp = getSuspendDepVtx(nodep);
        DepVtx* സ്ഥിരം pVxp = getNeedsProcDepVtx(nodep);
        എങ്കിൽ (!m_classp) തിരികെ;

        // Go over overridden functions

        std::queue<AstClassExtends*> extends;
        എങ്കിൽ (m_classp->extendsp()) extends.തള്ളുക(m_classp->extendsp());

        ആകുമ്പോൾ (!extends.empty()) {
            AstClassExtends* ext_list = extends.front();
            extends.എടുക്കുക();

            വേണ്ടി (AstClassExtends* cextp = ext_list; cextp;
                 cextp = VN_AS(cextp->nextp(), ClassExtends)) {
                // TODO: It is possible that a methods the same name in the base class is not
                // actually overridden by our method. If this causes a problem, traverse to
                // the root of the inheritance hierarchy and check if the original method is
                // virtual or not.
                എങ്കിൽ (auto* സ്ഥിരം overriddenp
                    = VN_CAST(m_memberMap.findMember(cextp->classp(), nodep->name()), CFunc)) {
                    // Suspendability and process affects typing, so they propagate both ways
                    DepVtx* സ്ഥിരം overriddenSVxp = getSuspendDepVtx(overriddenp);
                    DepVtx* സ്ഥിരം overriddenPVxp = getNeedsProcDepVtx(overriddenp);
                    പുതിയ V3GraphEdge{&m_suspGraph, sVxp, overriddenSVxp, P_SIGNATURE};
                    പുതിയ V3GraphEdge{&m_suspGraph, overriddenSVxp, sVxp, P_SIGNATURE};
                    പുതിയ V3GraphEdge{&m_procGraph, pVxp, overriddenPVxp, P_SIGNATURE};
                    പുതിയ V3GraphEdge{&m_procGraph, overriddenPVxp, pVxp, P_SIGNATURE};
                } അല്ലെങ്കിൽ {
                    AstClassExtends* more_extends = cextp->classp()->extendsp();
                    എങ്കിൽ (more_extends) extends.തള്ളുക(more_extends);
                }
            }
        }
    }
    ശൂന്യം visit(Ast_കണ്ണി_CCall* nodep) override {
        AstCFunc* സ്ഥിരം funcp = nodep->funcp();
        UASSERT_OBJ(funcp, nodep, "AstNodeCCall must have non-null funcp post-link");
        UASSERT_OBJ(m_procp, nodep, "AstNodeCCall must be inside a procedure/CFunc/Begin");

        UINFO(9, "V3Timing: Processing CCall to " << funcp->name() << " in dependency graph\n");
        പുതിയ V3GraphEdge{&m_suspGraph, getSuspendDepVtx(funcp), getSuspendDepVtx(m_procp), P_CALL};

        പുതിയ V3GraphEdge{&m_procGraph, getNeedsProcDepVtx(funcp), getNeedsProcDepVtx(m_procp),
                        P_CALL};

        iterateChildren(nodep);
    }
    ശൂന്യം visit(AstBegin* nodep) override {
        VL_RESTORER(m_procp);
        VL_RESTORER(m_underFork);

        എങ്കിൽ (!m_underFork || (m_underFork & F_MIGHT_SUSPEND))
            പുതിയ V3GraphEdge{&m_suspGraph, getSuspendDepVtx(nodep), getSuspendDepVtx(m_procp),
                            m_underFork ? P_FORK : P_CALL};

        പുതിയ V3GraphEdge{&m_procGraph, getNeedsProcDepVtx(nodep), getNeedsProcDepVtx(m_procp),
                        P_CALL};

        എങ്കിൽ (m_underFork) addFlags(nodep, T_NEEDS_PROC | T_ALLOCS_PROC);

        m_procp = nodep;
        m_underFork = 0;
        iterateChildren(nodep);
    }
    ശൂന്യം visit(AstFork* nodep) override {
        UINFO(9, "suspendable-visit " << nodep);
        VL_RESTORER(m_underFork);

        v3Global.setUsesTiming();  // Even if there are no event controls, we have to set this flag
                                   // so that transformForks() in V3SchedTiming gets called and
                                   // removes all forks and begins
        എങ്കിൽ (nodep->isTimingControl() && m_procp) {
            addFlags(m_procp, T_SUSPENDEE | T_SUSPENDER);
            m_underFork |= F_MIGHT_SUSPEND;
        }
        m_underFork |= F_MIGHT_NEED_PROC;
        iterateChildren(nodep);
    }
    ശൂന്യം visit(AstAssignDly* nodep) override {
        എങ്കിൽ (!VN_IS(m_procp, കണ്ണി_Procedure)) v3Global.setUsesTiming();
        visit(static_cast<Ast_കണ്ണി*>(nodep));
    }
    ശൂന്യം visit(AstFireEvent* nodep) override {
        എങ്കിൽ (nodep->isDelayed()
            && (!VN_IS(m_procp, കണ്ണി_Procedure) || !VN_IS(nodep->operandp(), VarRef))) {
            v3Global.setUsesTiming();
        }
        visit(static_cast<Ast_കണ്ണി*>(nodep));
    }
    ശൂന്യം visit(AstAssignW* nodep) override {
        എങ്കിൽ (nodep->timingControlp()) v3Global.setUsesTiming();
        // Containing process will not suspend, don't mark it
    }
    ശൂന്യം visit(Ast_കണ്ണി* nodep) override {
        എങ്കിൽ (nodep->isTimingControl()) {
            v3Global.setUsesTiming();
            എങ്കിൽ (m_procp) addFlags(m_procp, T_SUSPENDEE | T_SUSPENDER | T_NEEDS_PROC);
        }
        iterateChildren(nodep);
    }

    //--------------------
    ശൂന്യം visit(AstVar*) override {}  // Accelerate

പൊതു:
    // CONSTRUCTORS
    explicit TimingSuspendableVisitor(AstNetlist* nodep) {
        iterate(nodep);
        m_suspGraph.removeTransitiveEdges();
        m_procGraph.removeTransitiveEdges();
        // Propagate suspendability
        വേണ്ടി (V3GraphVertex& vtx : m_suspGraph.vertices()) {
            DepVtx& depVtx = static_cast<DepVtx&>(vtx);
            എങ്കിൽ (hasFlags(depVtx.nodep(), T_SUSPENDEE)) propagateFlags(&depVtx, T_SUSPENDEE);
        }
        എങ്കിൽ (dumpGraphLevel() >= 6) m_suspGraph.dumpDotFilePrefixed("timing_deps");

        // Propagate T_HAS_PROCESS
        വേണ്ടി (V3GraphVertex& vtx : m_procGraph.vertices()) {
            DepVtx& depVtx = static_cast<DepVtx&>(vtx);
            // Find processes that'll allocate VlProcess
            എങ്കിൽ (hasFlags(depVtx.nodep(), T_FORCES_PROC)) {
                propagateFlagsIf(&depVtx, T_FORCES_PROC, [&](സ്ഥിരം V3GraphEdge* e) -> തർക്കം {
                    തിരികെ !hasFlags(static_cast<DepVtx*>(e->fromp())->nodep(), T_ALLOCS_PROC);
                });
            }
            // Mark nodes on paths between processes and statements that use VlProcess
            എങ്കിൽ (hasFlags(depVtx.nodep(), T_NEEDS_PROC)) {
                propagateFlagsIf(&depVtx, T_NEEDS_PROC, [&](സ്ഥിരം V3GraphEdge* e) -> തർക്കം {
                    തിരികെ !hasFlags(static_cast<DepVtx*>(e->മുകൾ())->nodep(), T_ALLOCS_PROC);
                });
            }
        }
        വേണ്ടി (V3GraphVertex& vtx : m_procGraph.vertices()) {
            DepVtx& depVtx = static_cast<DepVtx&>(vtx);
            // Mark nodes that will be emitted with a VlProcess argument
            എങ്കിൽ (hasFlags(depVtx.nodep(), T_ALLOCS_PROC | T_FORCES_PROC)) {
                addFlags(depVtx.nodep(), T_HAS_PROC);
                propagateFlagsReversedIf(&depVtx, T_HAS_PROC, [&](സ്ഥിരം V3GraphEdge* e) -> തർക്കം {
                    തിരികെ hasFlags(static_cast<DepVtx*>(e->fromp())->nodep(), T_NEEDS_PROC);
                });
            }
        }
        എങ്കിൽ (dumpGraphLevel() >= 6) m_procGraph.dumpDotFilePrefixed("proc_deps");
    }
    ~TimingSuspendableVisitor() override = സ്ഥിരസ്ഥിതി;
};

// ######################################################################
//  Transform nodes affected by timing

വർഗ്ഗം TimingControlVisitor final : പൊതു VNVisitor {
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
    AstNetlist* സ്ഥിരം m_netlistp;  // Root node
    Ast_മുകൾ_Scope* സ്ഥിരം m_മുകൾ_Scopep = m_netlistp->മുകൾ_Scopep();  // The AstTopScope
    AstScope* സ്ഥിരം m_scopeTopp = m_മുകൾ_Scopep->scopep();  // Scope at the top
    AstClass* m_classp = nullptr;  // Current class
    AstScope* m_scopep = nullptr;  // Current scope
    AstActive* m_activep = nullptr;  // Current active
    Ast_കണ്ണി* m_procp = nullptr;  // NodeProcedure/CFunc/Begin we're under
    തർക്കം m_hasProcess = അസത്യം;  // True if current scope has a VlProcess handle available
    പൂർണ്ണസംഖ്യ m_forkCnt = 0;  // Number of forks inside a module
    തർക്കം m_underJumpBlock = അസത്യം;  // True if we are inside of a jump-block
    തർക്കം m_underProcedure = അസത്യം;  // True if we are under an always or initial
    തർക്കം m_underIfaceCFunc = അസത്യം;  // True if we are under a CFunc owned by an interface scope
    തർക്കം m_hasStaticZeroDelay = അസത്യം;  // True if we have a static #0 delay
    std::vector<File_വരി*> m_unknownDelayFlps;  // Locations of AstDelay with non-constant value

    // Unique names
    V3UniqueNames m_dlyforkNames{"__Vdlyfork"};  // Names for temp AssignW vars
    V3UniqueNames m_intra_മൂല്യം_Names{"__Vintraval"};  // Intra assign delay value var names
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
    Ast_Sen_വൃക്ഷം* m_delaySensesp = nullptr;  // Domain to trigger if a delayed coroutine is resumed
    Ast_Sen_വൃക്ഷം* m_dynamicSensesp = nullptr;  // Domain to trigger if a dynamic trigger is set

    // Other
    Sen_വൃക്ഷം_Finder m_finder{m_netlistp};  // Sentree finder and uniquifier
    Sen_വ്യഞ്ജകം_Builder* m_sen_വ്യഞ്ജകം_Builderp = nullptr;  // Sens expression builder for current m_scope

    // Stats
    size_t m_statZeroDelays = 0;  // Number of statically known #0 delays
    size_t m_statConstDelays = 0;  // Number of statically known #const (non-zero) delays
    size_t m_statVariableDelays = 0;  // Number of delays with value unknown at compile time

    // METHODS
    // Transform an assignment with an intra timing control into a timing control with the
    // assignment under it
    Ast_കണ്ണി* factorOutTimingControl(Ast_കണ്ണി_Assign* nodep) സ്ഥിരം {
        Ast_കണ്ണി* stmtp = nodep;
        Ast_കണ്ണി* സ്ഥിരം controlp = nodep->timingControlp();
        എങ്കിൽ (AstDelay* സ്ഥിരം delayp = VN_CAST(controlp, Delay)) {
            stmtp->replaceWith(delayp->unlinkFrBack());
            delayp->addStmtsp(stmtp);
            stmtp = delayp;
        } അല്ലെങ്കിൽ എങ്കിൽ (Ast_Sen_വൃക്ഷം* സ്ഥിരം sentreep = VN_CAST(controlp, Sen_വൃക്ഷം)) {
            AstEventControl* സ്ഥിരം eventControlp
                = പുതിയ AstEventControl{sentreep->fileline(), sentreep->unlinkFrBack(), nullptr};
            stmtp->replaceWith(eventControlp);
            eventControlp->addStmtsp(stmtp);
            stmtp = eventControlp;
        } അല്ലെങ്കിൽ എങ്കിൽ (AstBegin* സ്ഥിരം beginp = VN_CAST(controlp, Begin)) {
            // Begin from V3AssertPre
            stmtp->replaceWith(beginp->unlinkFrBack());
            beginp->addStmtsp(stmtp);
            stmtp = beginp;
        }
        തിരികെ stmtp == nodep ? nullptr : stmtp;
    }
    സ്ഥിതം തർക്കം staticallyNonZeroDelay(സ്ഥിരം Ast_കണ്ണി_വ്യഞ്ജകം* valuep) {
        എങ്കിൽ (സ്ഥിരം AstConst* സ്ഥിരം constp = VN_CAST(valuep, Const)) തിരികെ !constp->isZero();
        എങ്കിൽ (സ്ഥിരം AstCond* സ്ഥിരം condp = VN_CAST(valuep, Cond)) {
            തിരികെ staticallyNonZeroDelay(condp->thenp())
                   && staticallyNonZeroDelay(condp->elsep());
        }
        എങ്കിൽ (സ്ഥിരം AstMul* സ്ഥിരം mulp = VN_CAST(valuep, Mul)) {
            സ്ഥിരം AstConst* സ്ഥിരം lhsConstp = VN_CAST(mulp->lhsp(), Const);
            സ്ഥിരം AstConst* സ്ഥിരം rhsConstp = VN_CAST(mulp->rhsp(), Const);
            എങ്കിൽ (lhsConstp && !lhsConstp->isZero()) തിരികെ staticallyNonZeroDelay(mulp->rhsp());
            എങ്കിൽ (rhsConstp && !rhsConstp->isZero()) തിരികെ staticallyNonZeroDelay(mulp->lhsp());
        }
        തിരികെ അസത്യം;
    }
    // Creates the global delay scheduler variable
    AstVarScope* getCreateDelayScheduler() {
        എങ്കിൽ (m_delaySchedp) തിരികെ m_delaySchedp;
        auto* സ്ഥിരം dlySchedDtp = പുതിയ AstBasicDType{
            m_scopeTopp->fileline(), VBasicDTypeKwd::DELAY_SCHEDULER, VSigning::UNSIGNED};
        m_netlistp->typeTablep()->addTypesp(dlySchedDtp);
        m_delaySchedp = m_മുകൾ_Scopep->createTemp("__VdlySched", dlySchedDtp);
        // Delay scheduler has to be accessible from top
        m_delaySchedp->varp()->sigPublic(സത്യം);
        m_netlistp->delaySchedulerp(m_delaySchedp->varp());
        തിരികെ m_delaySchedp;
    }
    // Creates the delay sentree
    Ast_Sen_വൃക്ഷം* get_Create_Delay_Sen_വൃക്ഷം() {
        എങ്കിൽ (m_delaySensesp) തിരികെ m_delaySensesp;
        File_വരി* സ്ഥിരം flp = m_scopeTopp->fileline();
        auto* സ്ഥിരം awaitingCurrentTimep
            = പുതിയ AstCMethodHard{flp, പുതിയ AstVarRef{flp, getCreateDelayScheduler(), VAccess::READ},
                                 VCMethod::SCHED_AWAITING_CURRENT_TIME};
        awaitingCurrentTimep->dtypeSetBit();
        m_delaySensesp
            = പുതിയ Ast_Sen_വൃക്ഷം{flp, പുതിയ AstSenItem{flp, VEdgeType::ET_TRUE, awaitingCurrentTimep}};
        m_netlistp->മുകൾ_Scopep()->addSenTreesp(m_delaySensesp);
        തിരികെ m_delaySensesp;
    }
    // Creates the global dynamic trigger scheduler variable
    AstVarScope* getCreateDynamicTriggerScheduler() {
        എങ്കിൽ (m_dynamicSchedp) തിരികെ m_dynamicSchedp;
        auto* സ്ഥിരം dynSchedDtp
            = പുതിയ AstBasicDType{m_scopeTopp->fileline(), VBasicDTypeKwd::DYNAMIC_TRIGGER_SCHEDULER,
                                VSigning::UNSIGNED};
        m_netlistp->typeTablep()->addTypesp(dynSchedDtp);
        m_dynamicSchedp = m_മുകൾ_Scopep->createTemp("__VdynSched", dynSchedDtp);
        തിരികെ m_dynamicSchedp;
    }
    // Creates the dynamic trigger sentree
    Ast_Sen_വൃക്ഷം* get_Create_Dynamic_Trigger_Sen_വൃക്ഷം() {
        എങ്കിൽ (m_dynamicSensesp) തിരികെ m_dynamicSensesp;
        File_വരി* സ്ഥിരം flp = m_scopeTopp->fileline();
        auto* സ്ഥിരം awaitingCurrentTimep = പുതിയ AstCMethodHard{
            flp, പുതിയ AstVarRef{flp, getCreateDynamicTriggerScheduler(), VAccess::READ},
            VCMethod::SCHED_EVALUATE};
        awaitingCurrentTimep->dtypeSetBit();
        m_dynamicSensesp
            = പുതിയ Ast_Sen_വൃക്ഷം{flp, പുതിയ AstSenItem{flp, VEdgeType::ET_TRUE, awaitingCurrentTimep}};
        m_netlistp->മുകൾ_Scopep()->addSenTreesp(m_dynamicSensesp);
        തിരികെ m_dynamicSensesp;
    }
    // Creates the event variable to trigger in NBA region
    AstEventControl* createNbaEventControl(File_വരി* flp) {
        എങ്കിൽ (!m_netlistp->nbaEventp()) {
            auto* സ്ഥിരം nbaEventDtp = പുതിയ AstBasicDType{m_scopeTopp->fileline(),
                                                        VBasicDTypeKwd::EVENT, VSigning::UNSIGNED};
            m_netlistp->typeTablep()->addTypesp(nbaEventDtp);
            m_netlistp->nbaEventp(m_മുകൾ_Scopep->createTemp("__VnbaEvent", nbaEventDtp));
            v3Global.setHasEvents();
        }
        തിരികെ പുതിയ AstEventControl{
            flp,
            പുതിയ Ast_Sen_വൃക്ഷം{
                flp, പുതിയ AstSenItem{flp, VEdgeType::ET_EVENT,
                                    പുതിയ AstVarRef{flp, m_netlistp->nbaEventp(), VAccess::READ}}},
            nullptr};
    }
    // Creates the variable that, if set, causes the NBA event to be triggered
    AstAssign* createNbaEventTriggerAssignment(File_വരി* flp) {
        എങ്കിൽ (!m_netlistp->nbaEventTriggerp()) {
            m_netlistp->nbaEventTriggerp(m_മുകൾ_Scopep->createTemp("__VnbaEventTrigger", 1));
        }
        തിരികെ പുതിയ AstAssign{flp,
                             പുതിയ AstVarRef{flp, m_netlistp->nbaEventTriggerp(), VAccess::WRITE},
                             പുതിയ AstConst{flp, AstConst::BitTrue{}}};
    }
    // Returns true if we are under a class or interface function, or the tree references locals.
    // These are cases where static, globally-evaluated triggers are not suitable.
    തർക്കം needDynamicTrigger(Ast_കണ്ണി* സ്ഥിരം nodep) സ്ഥിരം {
        തിരികെ m_classp || m_underIfaceCFunc || nodep->exists([](Ast_കണ്ണി* സ്ഥിരം nodep) {
            എങ്കിൽ (Ast_കണ്ണി_Var_Ref* varp = VN_CAST(nodep, കണ്ണി_Var_Ref)) {
                തിരികെ varp->varp()->isFuncLocal();
            }
            തിരികെ !nodep->isPure();
        });
    }
    // Creates a trigger scheduler variable
    AstVarScope* getCreateTriggerSchedulerp(Ast_Sen_വൃക്ഷം* സ്ഥിരം sentreep) {
        എങ്കിൽ (!sentreep->user1p()) {
            എങ്കിൽ (!m_trigSchedDtp) {
                m_trigSchedDtp
                    = പുതിയ AstBasicDType{m_scopeTopp->fileline(), VBasicDTypeKwd::TRIGGER_SCHEDULER,
                                        VSigning::UNSIGNED};
                m_netlistp->typeTablep()->addTypesp(m_trigSchedDtp);
            }
            AstVarScope* സ്ഥിരം trigSchedp
                = m_മുകൾ_Scopep->createTemp(m_trigSchedNames.get(sentreep), m_trigSchedDtp);
            sentreep->user1p(trigSchedp);
        }
        തിരികെ VN_AS(sentreep->user1p(), VarScope);
    }
    // Creates a string describing the sentree
    AstCExpr* createEventDescription(Ast_Sen_വൃക്ഷം* സ്ഥിരം sentreep) സ്ഥിരം {
        എങ്കിൽ (!sentreep->user2p()) {
            std::stringstream ss;
            V3EmitV::verilog_For_വൃക്ഷം(sentreep, ss);
            // possibly a multiline string
            // Names were hashed by V3Name, show the user the original text
            std::string comment = VName::dehash(ss.str());
            std::replace(comment.begin(), comment.end(), '\n', ' ');
            // Escape quotes, e.g. from string-indexed event array selects
            comment = '"' + V3OutFormatter::quoteNameControls(comment) + '"';
            AstCExpr* സ്ഥിരം commentp = പുതിയ AstCExpr{sentreep->fileline(), comment};
            commentp->dtypeSetString();
            sentreep->user2p(commentp);
            തിരികെ commentp;
        }
        തിരികെ VN_AS(sentreep->user2p(), CExpr)->clone_വൃക്ഷം(അസത്യം);
    }
    // Adds debug info to a hardcoded method call
    ശൂന്യം addDebugInfo(AstCMethodHard* സ്ഥിരം methodp) സ്ഥിരം {
        File_വരി* സ്ഥിരം flp = methodp->fileline();
        സ്ഥിരം തർക്കം protectIds = v3Global.opt.protectIds();
        AstCExpr* സ്ഥിരം ap
            = പുതിയ AstCExpr{flp, protectIds ? "VL_UNKNOWN" : '"' + flp->filenameEsc() + '"'};
        ap->dtypeSetString();
        methodp->addPinsp(ap);
        AstCExpr* സ്ഥിരം bp = പുതിയ AstCExpr{flp, protectIds ? "0" : cvtToStr(flp->lineno())};
        bp->dtypeSetString();
        methodp->addPinsp(bp);
    }
    // Adds debug info to a trigSched.trigger() call
    ശൂന്യം addEventDebugInfo(AstCMethodHard* സ്ഥിരം methodp, Ast_Sen_വൃക്ഷം* സ്ഥിരം sentreep) സ്ഥിരം {
        എങ്കിൽ (v3Global.opt.protectIds()) {
            File_വരി* സ്ഥിരം flp = sentreep->fileline();
            AstCExpr* സ്ഥിരം descp = പുതിയ AstCExpr{flp, "VL_UNKNOWN"};
            descp->dtypeSetString();
            methodp->addPinsp(descp);
            addDebugInfo(methodp);
            തിരികെ;
        }
        methodp->addPinsp(createEventDescription(sentreep));
        addDebugInfo(methodp);
    }
    // Adds process pointer to a hardcoded method call
    ശൂന്യം addProcessInfo(AstCMethodHard* സ്ഥിരം methodp) സ്ഥിരം {
        File_വരി* സ്ഥിരം flp = methodp->fileline();
        AstCExpr* സ്ഥിരം ap
            = പുതിയ AstCExpr{flp, (m_procp && m_hasProcess) ? "vlProcess" : "nullptr"};
        methodp->addPinsp(ap);
    }
    // Creates the fork handle type and returns it
    AstBasicDType* getCreateForkSyncDTypep() {
        എങ്കിൽ (m_forkDtp) തിരികെ m_forkDtp;
        m_forkDtp = പുതിയ AstBasicDType{m_scopeTopp->fileline(), VBasicDTypeKwd::FORK_SYNC,
                                      VSigning::UNSIGNED};
        m_netlistp->typeTablep()->addTypesp(m_forkDtp);
        തിരികെ m_forkDtp;
    }
    // Move `nodep` into `AstCLocalScope`.
    AstCLocalScope* addCLocalScope(File_വരി* സ്ഥിരം flp, Ast_കണ്ണി* സ്ഥിരം nodep) സ്ഥിരം {
        എങ്കിൽ (!nodep) തിരികെ nullptr;
        VNRelinker handle;
        nodep->unlinkFrBack(&handle);
        AstCLocalScope* സ്ഥിരം cscopep = പുതിയ AstCLocalScope{flp, nodep};
        handle.relink(cscopep);
        തിരികെ cscopep;
    }
    // Create a temp variable and optionally put it before the specified node (mark local if so)
    AstVarScope* createTemp(File_വരി* സ്ഥിരം flp, സ്ഥിരം std::string& name,
                            Ast_കണ്ണി_DType* സ്ഥിരം dtypep, Ast_കണ്ണി* സ്ഥിരം തിരുകുക_Beforep = nullptr) {
        AstVar* varp;
        എങ്കിൽ (തിരുകുക_Beforep) {
            varp = പുതിയ AstVar{flp, VVarType::BLOCKTEMP, name, dtypep};
            varp->funcLocal(സത്യം);
            തിരുകുക_Beforep->addHereThisAsNext(varp);
        } അല്ലെങ്കിൽ {
            varp = പുതിയ AstVar{flp, VVarType::MODULETEMP, name, dtypep};
            m_scopep->modp()->addStmtsp(varp);
        }
        AstVarScope* സ്ഥിരം vscp = പുതിയ AstVarScope{flp, m_scopep, varp};
        m_scopep->addVarsp(vscp);
        തിരികെ vscp;
    }
    // Add a done() call on the fork sync
    ശൂന്യം addForkDone(AstBegin* സ്ഥിരം beginp, AstVarScope* സ്ഥിരം forkVscp) സ്ഥിരം {
        File_വരി* സ്ഥിരം flp = beginp->fileline();
        auto* സ്ഥിരം donep = പുതിയ AstCMethodHard{
            beginp->fileline(), പുതിയ AstVarRef{flp, forkVscp, VAccess::WRITE}, VCMethod::FORK_DONE};
        donep->dtypeSetVoid();
        addDebugInfo(donep);
        beginp->addStmtsp(donep->makeStmt());
    }
    സ്ഥിതം തർക്കം has_Disable_Queue_തള്ളുക_Self_Prefix(AstBegin* സ്ഥിരം beginp) {
        // LinkJump prepends disable-by-name registration as:
        //   __VprocessQueue_*.push_back(std::process::self())
        // V3LiftExpr lifts std::process::self() into an assignment to a temporary, then V3Task
        // lowers that assignment's function call into a leading comment and AstStmtExpr. Thus the
        // push_back is no longer necessarily the first statement. Scan across this generated
        // prefix for it, stopping at the fork-start sentinel or the branch body. Registrations for
        // nested forks live in sub-blocks and so are not in this statement list.
        വേണ്ടി (Ast_കണ്ണി* stmtp = beginp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            എങ്കിൽ (VN_IS(stmtp, Comment)) തുടരുക;
            Ast_Stmt_വ്യഞ്ജകം* സ്ഥിരം stmtExprp = VN_CAST(stmtp, Stmt_വ്യഞ്ജകം);
            എങ്കിൽ (!stmtExprp) നിർത്തുക;
            എങ്കിൽ (stmtExprp->is_Disable_Queue_തള്ളുക_Self_Stmt()) തിരികെ സത്യം;
        }
        തിരികെ അസത്യം;
    }
    // Register a callback so killing a process-backed fork branch decrements the join counter
    ശൂന്യം addForkOnKill(AstBegin* സ്ഥിരം beginp, AstVarScope* സ്ഥിരം forkVscp) സ്ഥിരം {
        എങ്കിൽ (!beginp->needProcess() && !has_Disable_Queue_തള്ളുക_Self_Prefix(beginp)) തിരികെ;
        File_വരി* സ്ഥിരം flp = beginp->fileline();
        AstCMethodHard* സ്ഥിരം onKillp = പുതിയ AstCMethodHard{
            flp, പുതിയ AstVarRef{flp, forkVscp, VAccess::WRITE}, VCMethod::FORK_ON_KILL};
        onKillp->dtypeSetVoid();
        AstCExpr* സ്ഥിരം processp = പുതിയ AstCExpr{flp, "vlProcess"};
        processp
            ->dtypeSetVoid();  // Opaque process reference; type is irrelevant for hardcoded emit
        onKillp->addPinsp(processp);
        Ast_കണ്ണി_Stmt* സ്ഥിരം stmtp = onKillp->makeStmt();
        beginp->prependStmtsp(stmtp);
    }
    // Handle the 'join' part of a fork..join
    ശൂന്യം makeForkJoin(AstFork* സ്ഥിരം forkp) {
        // Create a fork sync var
        File_വരി* സ്ഥിരം flp = forkp->fileline();
        // Insert the sync var directly before the fork
        Ast_കണ്ണി* സ്ഥിരം തിരുകുക_Beforep = forkp;
        // Make sure all references to the fork var are contained within a single scope to prevent
        // moving statements accessing it in case of large functions splitting.
        addCLocalScope(flp, തിരുകുക_Beforep);
        AstVarScope* forkVscp
            = createTemp(flp, forkp->name() + "__sync", getCreateForkSyncDTypep(), തിരുകുക_Beforep);
        unsigned join_എണ്ണം = 0;  // Needed for join counter
        // Add a <fork sync>.done() to each begin
        വേണ്ടി (Ast_കണ്ണി* beginp = forkp->forksp(); beginp; beginp = beginp->nextp()) {
            addForkOnKill(VN_AS(beginp, Begin), forkVscp);
            addForkDone(VN_AS(beginp, Begin), forkVscp);
            join_എണ്ണം++;
        }
        എങ്കിൽ (forkp->joinType().joinAny()) join_എണ്ണം = 1;
        // Set the join counter
        auto* സ്ഥിരം initp = പുതിയ AstCMethodHard{flp, പുതിയ AstVarRef{flp, forkVscp, VAccess::WRITE},
                                               VCMethod::FORK_INIT, പുതിയ AstConst{flp, join_എണ്ണം}};
        initp->dtypeSetVoid();
        addProcessInfo(initp);
        forkp->addHereThisAsNext(initp->makeStmt());
        // Await the join at the end
        auto* സ്ഥിരം joinp = പുതിയ AstCMethodHard{flp, പുതിയ AstVarRef{flp, forkVscp, VAccess::WRITE},
                                               VCMethod::FORK_JOIN};
        joinp->dtypeSetVoid();
        addProcessInfo(joinp);
        addDebugInfo(joinp);
        forkp->addNextHere(പുതിയ AstCAwait{flp, joinp});
    }

    // `procp` shall be a NodeProcedure/CFunc/Begin and within it vars from `varsp` will be placed.
    // `varsp` vector of vars which shall be localized.
    സ്ഥിതം ശൂന്യം localizeVars(Ast_കണ്ണി* സ്ഥിരം procp, സ്ഥിരം std::vector<AstVar*>& varsp) {
        UASSERT(procp, "procp is nullptr");
        AstVar* newVarsp = nullptr;
        വേണ്ടി (AstVar* സ്ഥിരം varp : varsp) {
            varp->funcLocal(സത്യം);
            newVarsp = Ast_കണ്ണി::addNextNull(newVarsp, varp->unlinkFrBack());
        }
        എങ്കിൽ (Ast_കണ്ണി_Procedure* സ്ഥിരം കണ്ണി_Procp = VN_CAST(procp, കണ്ണി_Procedure)) {
            കണ്ണി_Procp->prependStmtsp(newVarsp);
        } അല്ലെങ്കിൽ എങ്കിൽ (AstCFunc* സ്ഥിരം cfuncp = VN_CAST(procp, CFunc)) {
            cfuncp->prependVarsp(newVarsp);
        } അല്ലെങ്കിൽ എങ്കിൽ (AstBegin* സ്ഥിരം beginp = VN_CAST(procp, Begin)) {
            beginp->prependStmtsp(newVarsp);
        } അല്ലെങ്കിൽ {
            procp->v3fatalSrc(
                procp->prettyNameQ()
                << " is not of an expected type NodeProcedure/CFunc/Begin instead it is: "
                << procp->prettyTypeName());
        }
    }

    Ast_കണ്ണി_വ്യഞ്ജകം* delayToIntegral(AstDelay* nodep, Ast_കണ്ണി_വ്യഞ്ജകം* valuep) {
        സ്ഥിരം AstConst* സ്ഥിരം constp = VN_CAST(valuep, Const);
        // The fork start sentinel is already in ticks
        എങ്കിൽ (constp && constp->toUQuad() == std::numeric_limits<uint64_t>::max()) തിരികെ valuep;
        തിരികെ V3Timing::delayToTicks(valuep, nodep->timeunit());
    }

    // VISITORS
    ശൂന്യം visit(Ast_കണ്ണി_Module* nodep) override {
        UASSERT_OBJ(!m_classp, nodep, "Module or class under class");
        VL_RESTORER(m_classp);
        m_classp = VN_CAST(nodep, Class);
        VL_RESTORER(m_forkCnt);
        m_forkCnt = 0;
        iterateChildren(nodep);
    }
    ശൂന്യം visit(AstScope* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        Sen_വ്യഞ്ജകം_Builder sen_വ്യഞ്ജകം_Builder{m_scopep};
        {  // Restore m_senExprBuilderp before destroying senExprBuilder
            VL_RESTORER(m_sen_വ്യഞ്ജകം_Builderp);
            m_sen_വ്യഞ്ജകം_Builderp = &sen_വ്യഞ്ജകം_Builder;
            iterateChildren(nodep);
        }
    }
    ശൂന്യം visit(AstActive* nodep) override {
        VL_RESTORER(m_activep);
        m_activep = nodep;
        iterateChildren(nodep);
    }
    ശൂന്യം visit(Ast_കണ്ണി_Procedure* nodep) override {
        VL_RESTORER(m_procp);
        VL_RESTORER(m_hasProcess);
        m_procp = nodep;
        m_hasProcess = hasFlags(nodep, T_HAS_PROC);
        VL_RESTORER(m_underProcedure);
        m_underProcedure = സത്യം;
        iterateChildren(nodep);
        എങ്കിൽ (hasFlags(nodep, T_SUSPENDEE)) nodep->setSuspendable();
        എങ്കിൽ (hasFlags(nodep, T_HAS_PROC)) nodep->setNeedProcess();
    }
    ശൂന്യം visit(AstInitial* nodep) override {
        visit(static_cast<Ast_കണ്ണി_Procedure*>(nodep));
        എങ്കിൽ (nodep->needProcess() && !nodep->user1SetOnce()) {
            nodep->addStmtsp(
                പുതിയ AstCStmt{nodep->fileline(), "vlProcess->state(VlProcess::FINISHED);"});
        }
    }
    ശൂന്യം visit(AstJumpBlock* nodep) override {
        VL_RESTORER(m_underJumpBlock);
        m_underJumpBlock = സത്യം;
        visit(static_cast<Ast_കണ്ണി_Stmt*>(nodep));
    }
    ശൂന്യം visit(AstAlways* nodep) override {
        എങ്കിൽ (nodep->user1SetOnce()) തിരികെ;
        VL_RESTORER(m_procp);
        VL_RESTORER(m_hasProcess);
        m_procp = nodep;
        m_hasProcess = hasFlags(nodep, T_HAS_PROC);
        VL_RESTORER(m_underProcedure);
        m_underProcedure = സത്യം;
        // Workaround for killing `always` processes (doing that is pretty much UB)
        // TODO: Disallow killing `always` at runtime (throw an error)
        // Skip for combinational blocks; if the body has timing controls
        // iterateChildren will add T_SUSPENDEE, otherwise it would spin.
        എങ്കിൽ (hasFlags(nodep, T_HAS_PROC)
            && !(m_activep && m_activep->sentreep() && m_activep->sentreep()->hasCombo()))
            addFlags(nodep, T_SUSPENDEE);

        iterateChildren(nodep);
        എങ്കിൽ (hasFlags(nodep, T_HAS_PROC)) nodep->setNeedProcess();
        എങ്കിൽ (!hasFlags(nodep, T_SUSPENDEE)) തിരികെ;
        nodep->setSuspendable();
        File_വരി* സ്ഥിരം flp = nodep->fileline();
        Ast_Sen_വൃക്ഷം* സ്ഥിരം sentreep = m_activep->sentreep();
        എങ്കിൽ (sentreep->hasClocked()) {
            Ast_കണ്ണി* സ്ഥിരം bodysp = nodep->stmtsp()->unlinkFrBackWithNext();
            auto* സ്ഥിരം controlp = പുതിയ AstEventControl{flp, sentreep->clone_വൃക്ഷം(അസത്യം), bodysp};
            nodep->addStmtsp(controlp);
            iterate(controlp);
        }
        // Note: The 'while (true)' outer loop will be added in V3Sched
        auto* സ്ഥിരം activep = പുതിയ AstActive{
            flp, "", പുതിയ Ast_Sen_വൃക്ഷം{flp, പുതിയ AstSenItem{flp, AstSenItem::Initial{}}}};
        activep->sen_വൃക്ഷം_Storep(activep->sentreep());
        activep->addStmtsp(nodep->unlinkFrBack());
        m_activep->addNextHere(activep);
    }
    ശൂന്യം visit(AstCFunc* nodep) override {
        VL_RESTORER(m_procp);
        VL_RESTORER(m_hasProcess);
        VL_RESTORER(m_underIfaceCFunc);
        m_procp = nodep;
        m_hasProcess = hasFlags(nodep, T_HAS_PROC);
        m_underIfaceCFunc = VN_IS(m_scopep->modp(), Iface);
        iterateChildren(nodep);
        എങ്കിൽ (hasFlags(nodep, T_HAS_PROC)) nodep->setNeedProcess();
        എങ്കിൽ (!(hasFlags(nodep, T_SUSPENDEE))) തിരികെ;

        nodep->rtnType("VlCoroutine");
        // If in a class, create a shared pointer to 'this'
        എങ്കിൽ (m_classp) {
            AstCStmt* സ്ഥിരം cstmtp = പുതിയ AstCStmt{nodep->fileline(), "VL_KEEP_THIS;"};
            nodep->prependStmtsp(cstmtp);
        }
        എങ്കിൽ (nodep->dpiExportImpl()) {
            nodep->exists([](AstCAwait* സ്ഥിരം awaitp) -> തർക്കം {
                // A DPI-exported coroutine won't be able to block the calling code
                // Error on the await node; fall back to the function node
                awaitp->v3warn(E_UNSUPPORTED,
                               "Unsupported: Timing controls inside DPI-exported tasks");
                തിരികെ സത്യം;
            });
        }
    }
    ശൂന്യം visit(Ast_കണ്ണി_CCall* nodep) override {
        AstCFunc* സ്ഥിരം funcp = nodep->funcp();

        // Skip automatic covergroup sampling calls
        എങ്കിൽ (funcp->isCovergroupSample()) {
            iterateChildren(nodep);
            തിരികെ;
        }

        എങ്കിൽ (funcp->needProcess()) m_hasProcess = സത്യം;
        എങ്കിൽ (hasFlags(funcp, T_SUSPENDEE) && !nodep->user1SetOnce()) {  // If suspendable
            // Calls to suspendables are always void return type, hence parent must be StmtExpr
            Ast_Stmt_വ്യഞ്ജകം* സ്ഥിരം stmtp = VN_AS(nodep->backp(), Stmt_വ്യഞ്ജകം);
            stmtp->replaceWith(പുതിയ AstCAwait{nodep->fileline(), nodep->unlinkFrBack()});
            VL_DO_DANGLING(തള്ളുക_Deletep(stmtp), stmtp);
        }
        iterateChildren(nodep);
    }
    ശൂന്യം visit(AstDelay* nodep) override {
        UASSERT_OBJ(!nodep->isCycleDelay(), nodep,
                    "Cycle delays should have been handled in V3AssertPre");
        File_വരി* സ്ഥിരം flp = nodep->fileline();

        Ast_കണ്ണി_വ്യഞ്ജകം* valuep = delayToIntegral(nodep, nodep->lhsp()->unlinkFrBack());

        // Statistics
        എങ്കിൽ (valuep->isZero()) {
            ++m_statZeroDelays;
        } അല്ലെങ്കിൽ എങ്കിൽ (VN_IS(valuep, Const)) {
            ++m_statConstDelays;
        } അല്ലെങ്കിൽ {
            ++m_statVariableDelays;
        }

        // Decide scheduling support for #0
        എങ്കിൽ (v3Global.opt.schedZeroDelay().isSetTrue()) {
            // User said to schedule for #0 support, nothing else to do
            v3Global.setUsesZeroDelay();
        } അല്ലെങ്കിൽ എങ്കിൽ (v3Global.opt.schedZeroDelay().isSetFalse()) {
            // User said to schedule without #0 support. Still warn if a static #0 delay exists
            എങ്കിൽ (valuep->isZero()) {
                nodep->v3warn(
                    ZERODLY,
                    "Static #0 delay exists, but '--no-sched-zero-delay' was given.\n"
                        << nodep->warnMore()  //
                        << "... Can proceed, but this will fail at runtime if executed.");
            }
        } അല്ലെങ്കിൽ {
            // User did not express preference, decide based on presence of delays
            എങ്കിൽ (valuep->isZero()) {
                // Statically known #0 delay exists, schedule for #0 support
                v3Global.setUsesZeroDelay();
                m_hasStaticZeroDelay = സത്യം;
                // Don't warn on variable delays, as no point
                m_unknownDelayFlps.clear();
            } അല്ലെങ്കിൽ എങ്കിൽ (staticallyNonZeroDelay(valuep)) {
                // Delay is dynamic, but every statically known outcome is non-zero.
                // So we don't need #0 delay support and there should be no warning.
            } അല്ലെങ്കിൽ എങ്കിൽ (!VN_IS(valuep, Const)) {
                // Delay is not known at compiile time. Conservatively schedule for #0 support,
                // but warn if no static #0 delays used as performance might be improved
                v3Global.setUsesZeroDelay();
                എങ്കിൽ (!m_hasStaticZeroDelay) m_unknownDelayFlps.തള്ളുക_back(nodep->fileline());
            }
        }

        // Replace self with a 'co_await dlySched.delay(<valuep>)'
        AstCMethodHard* സ്ഥിരം delayMethodp = പുതിയ AstCMethodHard{
            flp, പുതിയ AstVarRef{flp, getCreateDelayScheduler(), VAccess::WRITE},
            VCMethod::SCHED_DELAY, valuep};
        delayMethodp->dtypeSetVoid();
        addProcessInfo(delayMethodp);
        addDebugInfo(delayMethodp);
        // Create the co_await
        Ast_കണ്ണി* സ്ഥിരം awaitp = പുതിയ AstCAwait{flp, delayMethodp, get_Create_Delay_Sen_വൃക്ഷം()};
        // Relink child statements after the co_await
        എങ്കിൽ (Ast_കണ്ണി* സ്ഥിരം stmtsp = nodep->stmtsp()) {
            awaitp->addNext(stmtsp->unlinkFrBackWithNext());
        }
        nodep->replaceWith(awaitp);
        VL_DO_DANGLING(nodep->delete_വൃക്ഷം(), nodep);
    }
    ശൂന്യം visit(AstEventControl* nodep) override {
        // Do not allow waiting on local named events, as they get enqueued for clearing, but can
        // go out of scope before that happens
        എങ്കിൽ (!nodep->sentreep()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Event control with implicit sensitivity (@*)"
                          " in this context; use an explicit sensitivity list instead");
            VL_DO_DANGLING(തള്ളുക_Deletep(nodep->unlinkFrBack()), nodep);
            തിരികെ;
        }
        File_വരി* സ്ഥിരം flp = nodep->fileline();
        // Relink child statements after the event control
        എങ്കിൽ (nodep->stmtsp()) nodep->addNextHere(nodep->stmtsp()->unlinkFrBackWithNext());
        എങ്കിൽ (needDynamicTrigger(nodep->sentreep())) {
            // Create the trigger variable and init it with 0
            AstVarScope* സ്ഥിരം trigvscp
                = createTemp(flp, m_dynTrigNames.get(nodep), nodep->findBitDType(), nodep);
            auto* സ്ഥിരം initp = പുതിയ AstAssign{flp, പുതിയ AstVarRef{flp, trigvscp, VAccess::WRITE},
                                              പുതിയ AstConst{flp, AstConst::BitFalse{}}};
            // Await the eval step with the dynamic trigger scheduler. First, create the method
            // call
            auto* സ്ഥിരം evalMethodp = പുതിയ AstCMethodHard{
                flp, പുതിയ AstVarRef{flp, getCreateDynamicTriggerScheduler(), VAccess::WRITE},
                VCMethod::SCHED_EVALUATION};
            evalMethodp->dtypeSetVoid();
            addProcessInfo(evalMethodp);
            auto* സ്ഥിരം sentreep = nodep->sentreep();
            addEventDebugInfo(evalMethodp, sentreep);
            // Create the co_await
            AstCAwait* സ്ഥിരം awaitEvalp
                = പുതിയ AstCAwait{flp, evalMethodp, get_Create_Dynamic_Trigger_Sen_വൃക്ഷം()};
            // Construct the sen expression for this sentree
            UASSERT_OBJ(m_sen_വ്യഞ്ജകം_Builderp, nodep, "No SenExprBuilder for this scope");
            auto* സ്ഥിരം assignp = പുതിയ AstAssign{flp, പുതിയ AstVarRef{flp, trigvscp, VAccess::WRITE},
                                                m_sen_വ്യഞ്ജകം_Builderp->build(sentreep).first};
            // Get the SenExprBuilder results
            സ്ഥിരം Sen_വ്യഞ്ജകം_Builder::Results senResults = m_sen_വ്യഞ്ജകം_Builderp->getAndClearResults();
            localizeVars(m_procp, senResults.m_vars);
            // If post updates are destructive (e.g. clearFired on events), perform a
            // conservative pre-clear once before entering the wait loop so stale state from a
            // previous wait does not cause an immediate false-positive trigger.
            സ്ഥിരം തർക്കം hasDestructivePostUpdates = !senResults.m_destructivePostUpdates.empty();
            // Create the trigger eval loop, which will await the evaluation step and check the
            // trigger
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം condp
                = പുതിയ AstLogNot{flp, പുതിയ AstVarRef{flp, trigvscp, VAccess::READ}};
            AstLoop* സ്ഥിരം loopp = പുതിയ AstLoop{flp};
            loopp->addStmtsp(പുതിയ AstLoopTest{flp, loopp, condp});
            loopp->addStmtsp(awaitEvalp);
            // Put pre updates before the trigger check and assignment
            വേണ്ടി (Ast_കണ്ണി_Stmt* സ്ഥിരം stmtp : senResults.m_preUpdates) loopp->addStmtsp(stmtp);
            // Then the trigger check and assignment
            loopp->addStmtsp(assignp);
            // Let the dynamic trigger scheduler know if this trigger was set
            // If it was, a call to the scheduler's evaluate() will return true
            AstCMethodHard* സ്ഥിരം anyTriggeredMethodp = പുതിയ AstCMethodHard{
                flp, പുതിയ AstVarRef{flp, getCreateDynamicTriggerScheduler(), VAccess::WRITE},
                VCMethod::SCHED_ANY_TRIGGERED, പുതിയ AstVarRef{flp, trigvscp, VAccess::READ}};
            anyTriggeredMethodp->dtypeSetVoid();
            loopp->addStmtsp(anyTriggeredMethodp->makeStmt());
            // If the post update is destructive (e.g. event vars are cleared), create an await for
            // the post update step
            എങ്കിൽ (hasDestructivePostUpdates) {
                AstCAwait* സ്ഥിരം awaitPostUpdatep = awaitEvalp->clone_വൃക്ഷം(അസത്യം);
                VN_AS(awaitPostUpdatep->exprp(), CMethodHard)->method(VCMethod::SCHED_POST_UPDATE);
                loopp->addStmtsp(awaitPostUpdatep);
            }
            // Put the post updates at the end of the loop
            വേണ്ടി (Ast_കണ്ണി_Stmt* സ്ഥിരം stmtp : senResults.m_postUpdates) loopp->addStmtsp(stmtp);
            // Finally, await the resumption step in 'act'
            AstCAwait* സ്ഥിരം awaitResumep = awaitEvalp->clone_വൃക്ഷം(അസത്യം);
            VN_AS(awaitResumep->exprp(), CMethodHard)->method(VCMethod::SCHED_RESUMPTION);
            Ast_കണ്ണി::addNext<Ast_കണ്ണി_Stmt, Ast_കണ്ണി_Stmt>(loopp, awaitResumep);
            // Replace the event control with one explicit stmt chain:
            //   init -> [inits] -> [optional destructive pre-clears] -> loop -> awaitResumption
            Ast_കണ്ണി_Stmt* chainp = nullptr;
            chainp = Ast_കണ്ണി::addNextNull(chainp, initp);
            വേണ്ടി (Ast_കണ്ണി_Stmt* സ്ഥിരം stmtp : senResults.m_inits) {
                chainp = Ast_കണ്ണി::addNextNull(chainp, stmtp);
            }
            എങ്കിൽ (hasDestructivePostUpdates) {
                വേണ്ടി (Ast_കണ്ണി_Stmt* സ്ഥിരം stmtp : senResults.m_destructivePostUpdates) {
                    chainp = Ast_കണ്ണി::addNextNull(chainp, stmtp->clone_വൃക്ഷം(അസത്യം));
                }
            }
            chainp = Ast_കണ്ണി::addNextNull(chainp, loopp);
            nodep->replaceWith(chainp);
        } അല്ലെങ്കിൽ {
            auto* സ്ഥിരം sentreep = m_finder.get_Sen_വൃക്ഷം(nodep->sentreep());
            nodep->sentreep()->unlinkFrBack()->delete_വൃക്ഷം();
            // Get this sentree's trigger scheduler
            // Replace self with a 'co_await trigSched.trigger()'
            auto* സ്ഥിരം triggerMethodp = പുതിയ AstCMethodHard{
                flp, പുതിയ AstVarRef{flp, getCreateTriggerSchedulerp(sentreep), VAccess::WRITE},
                VCMethod::SCHED_TRIGGER};
            triggerMethodp->dtypeSetVoid();
            // If it should be committed immediately, pass true, otherwise false
            triggerMethodp->addPinsp(nodep->user2() ? പുതിയ AstConst{flp, AstConst::BitTrue{}}
                                                    : പുതിയ AstConst{flp, AstConst::BitFalse{}});
            addProcessInfo(triggerMethodp);
            addEventDebugInfo(triggerMethodp, sentreep);
            // Create the co_await
            AstCAwait* സ്ഥിരം awaitp = പുതിയ AstCAwait{flp, triggerMethodp, sentreep};
            nodep->replaceWith(awaitp);
        }
        VL_DO_DANGLING(nodep->delete_വൃക്ഷം(), nodep);
    }
    ശൂന്യം visit(Ast_കണ്ണി_Assign* nodep) override {
        // Only process once to avoid infinite loops (due to the net delay)
        എങ്കിൽ (nodep->user1SetOnce()) തിരികെ;
        UINFO(9, "control-visit " << nodep);
        File_വരി* സ്ഥിരം flp = nodep->fileline();
        Ast_കണ്ണി* controlp = factorOutTimingControl(nodep);
        സ്ഥിരം തർക്കം inAssignDly = VN_IS(nodep, AssignDly);
        // Handle the intra assignment timing control
        // Transform if:
        // * there's a timing control in the assignment
        // * the assignment is an AssignDly and it's in a non-inlined function
        എങ്കിൽ (!controlp && (!inAssignDly || m_underProcedure)) {
            iterateChildren(nodep);
            തിരികെ;
        }
        // The timing control, if any, which is put in a fork for an NBA
        Ast_കണ്ണി* സ്ഥിരം timingp = controlp;
        // Insert new vars before the timing control if we're in a function; in a process we can't
        // do that, as after V3Begin only modules and functions declare variables. These
        // intra-assignment vars will later be passed to forked processes by value.
        Ast_കണ്ണി* തിരുകുക_Beforep = m_underProcedure ? nullptr : controlp;
        // Special case for NBA
        എങ്കിൽ (inAssignDly) {
            // Put it in a fork so it doesn't block
            // Could already be the only thing directly under a fork, reuse that if possible
            AstFork* forkp = !nodep->nextp() ? VN_CAST(nodep->firstAbovep(), Fork) : nullptr;
            എങ്കിൽ (!forkp) forkp = പുതിയ AstFork{flp, VJoinType::JOIN_NONE};
            എങ്കിൽ (!m_underProcedure) {
                // If it's in a function, it won't be handled by V3Delayed
                // Put it behind an additional named event that gets triggered in the NBA region
                AstEventControl* സ്ഥിരം nbaEventControlp = createNbaEventControl(flp);
                AstAssign* സ്ഥിരം trigAssignp = createNbaEventTriggerAssignment(flp);
                nodep->replaceWith(trigAssignp);
                trigAssignp->addNextHere(nbaEventControlp);
                nbaEventControlp->addStmtsp(nodep);
                തിരുകുക_Beforep = forkp;
                എങ്കിൽ (!controlp) controlp = nbaEventControlp;
            }
            controlp->replaceWith(forkp);
            AstBegin* beginp = VN_CAST(controlp, Begin);
            എങ്കിൽ (!beginp) beginp = പുതിയ AstBegin{nodep->fileline(), "", controlp, അസത്യം};
            forkp->addForksp(beginp);
            addFlags(beginp, T_NBA_UPDATE);
            controlp = forkp;
        }
        UASSERT_OBJ(nodep, controlp, "Assignment should have timing control");
        // Move `insertBeforep` into `AstCLocalScope` if necessary to avoid jumping over
        // a variable initialization that would be inserted before `insertBeforep`. All
        // access to this variable should be contained within returned `AstCLocalScope`.
        എങ്കിൽ (m_underJumpBlock) addCLocalScope(flp, തിരുകുക_Beforep);
        // Function for replacing values with intermediate variables
        സ്ഥിരം auto replaceWithIntermediate = [&](Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം valuep,
                                                 സ്ഥിരം std::string& name) {
            AstVarScope* സ്ഥിരം newvscp = createTemp(flp, name, valuep->dtypep(), തിരുകുക_Beforep);
            // A pending update keeps its own value, even if its process schedules more
            എങ്കിൽ (inAssignDly && !തിരുകുക_Beforep) newvscp->fork_By_മൂല്യം(സത്യം);
            valuep->replaceWith(പുതിയ AstVarRef{flp, newvscp, VAccess::READ});
            controlp->addHereThisAsNext(
                പുതിയ AstAssign{flp, പുതിയ AstVarRef{flp, newvscp, VAccess::WRITE}, valuep});
        };
        // NBAs with delays evaluate the indices selecting the target now (IEEE 1800-2023 10.4.2),
        // in order, each as a whole, also the selects and function calls in it. So follow the
        // selects of the target up from the variable, not into their indices. As an example, this
        // transforms 'x[a[i]][f(j)] <= #1 y' into 't0 = a[i]; t1 = f(j); x[t0][t1] <= #1 y'.
        // V3Const has split NBAs to concatenations, also streamed, into NBAs to each part.
        എങ്കിൽ (inAssignDly) {
            വേണ്ടി (Ast_കണ്ണി* exprp = nodep->lhsp()->baseFromp(സത്യം); exprp != nodep;
                 exprp = exprp->firstAbovep()) {
                എങ്കിൽ (AstSel* സ്ഥിരം selp = VN_CAST(exprp, Sel)) {
                    // widthp should be const
                    എങ്കിൽ (!VN_IS(selp->lsbp(), Const)) {
                        replaceWithIntermediate(selp->lsbp(), m_intraLsbNames.get(nodep));
                    }
                } അല്ലെങ്കിൽ എങ്കിൽ (Ast_കണ്ണി_Sel* സ്ഥിരം selp = VN_CAST(exprp, കണ്ണി_Sel)) {
                    എങ്കിൽ (!VN_IS(selp->bitp(), Const)) {
                        replaceWithIntermediate(selp->bitp(), m_intraIndexNames.get(nodep));
                    }
                }
            }
        }
        // Replace the RHS with an intermediate value var
        replaceWithIntermediate(nodep->rhsp(), m_intra_മൂല്യം_Names.get(nodep));
        എങ്കിൽ (!inAssignDly) തിരികെ;
        // The process executing the NBA evaluates its delay (IEEE 1800-2023 4.9.4), not the forked
        // process, if it has side effects, e.g. calling a function that checks its process. A pure
        // one is evaluated as the forked process starts, at once.
        എങ്കിൽ (AstDelay* സ്ഥിരം delayp = VN_CAST(timingp, Delay)) {
            എങ്കിൽ (!delayp->lhsp()->isPure()) {
                replaceWithIntermediate(delayp->lhsp(), m_intra_മൂല്യം_Names.get(nodep));
            }
        }
    }
    ശൂന്യം visit(AstFireEvent* nodep) override {
        // V3Delayed handles '->>' of a variable in a process. Like NBAs in non-inlined functions,
        // trigger other events from a fork awaiting the NBA region.
        എങ്കിൽ (!nodep->isDelayed() || (m_underProcedure && VN_IS(nodep->operandp(), VarRef))) {
            iterateChildren(nodep);
            തിരികെ;
        }
        File_വരി* സ്ഥിരം flp = nodep->fileline();
        AstAssign* സ്ഥിരം trigAssignp = createNbaEventTriggerAssignment(flp);
        nodep->replaceWith(trigAssignp);
        AstFork* സ്ഥിരം forkp = പുതിയ AstFork{flp, VJoinType::JOIN_NONE};
        trigAssignp->addNextHere(forkp);
        എങ്കിൽ (m_underJumpBlock) addCLocalScope(flp, forkp);
        // The triggered event is the one referenced now, so evaluate handles and indices now
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം eventp = nodep->operandp()->unlinkFrBack();
        സ്ഥിരം auto evalNow = [&](Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം valuep, V3UniqueNames& names) {
            // Like for an NBA, a process uses a module variable, passed to the fork by value
            AstVarScope* സ്ഥിരം vscp = createTemp(flp, names.get(nodep), valuep->dtypep(),
                                                 m_underProcedure ? nullptr : forkp);
            എങ്കിൽ (m_underProcedure) vscp->fork_By_മൂല്യം(സത്യം);
            valuep->replaceWith(പുതിയ AstVarRef{flp, vscp, VAccess::READ});
            // Unlike an event, a handle selecting one is only read
            valuep->foreach([](Ast_കണ്ണി* സ്ഥിരം np) {
                എങ്കിൽ (Ast_കണ്ണി_Var_Ref* സ്ഥിരം refp = VN_CAST(np, കണ്ണി_Var_Ref)) {
                    refp->access(VAccess::READ);
                } അല്ലെങ്കിൽ എങ്കിൽ (AstMemberSel* സ്ഥിരം selp = VN_CAST(np, MemberSel)) {
                    selp->access(VAccess::READ);
                }
            });
            forkp->addHereThisAsNext(
                പുതിയ AstAssign{flp, പുതിയ AstVarRef{flp, vscp, VAccess::WRITE}, valuep});
        };
        Ast_കണ്ണി_വ്യഞ്ജകം* refp = eventp;
        ആകുമ്പോൾ (Ast_കണ്ണി_Sel* സ്ഥിരം selp = VN_CAST(refp, കണ്ണി_Sel)) {
            എങ്കിൽ (!VN_IS(selp->bitp(), Const)) evalNow(selp->bitp(), m_intraIndexNames);
            refp = selp->fromp();
        }
        // Handle of a class (or virtual interface)
        എങ്കിൽ (AstMemberSel* സ്ഥിരം selp = VN_CAST(refp, MemberSel)) {
            evalNow(selp->fromp(), m_intra_മൂല്യം_Names);
        }
        AstEventControl* സ്ഥിരം controlp = createNbaEventControl(flp);
        controlp->addStmtsp(പുതിയ AstFireEvent{flp, eventp, അസത്യം});
        AstBegin* സ്ഥിരം beginp = പുതിയ AstBegin{flp, "", controlp, അസത്യം};
        addFlags(beginp, T_NBA_UPDATE);
        forkp->addForksp(beginp);
        VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
    }
    ശൂന്യം visit(AstAssignW* nodep) override {
        File_വരി* സ്ഥിരം flp = nodep->fileline();
        // Get the net delay unless this assignment was created for handling the net delay (user1)
        AstDelay* സ്ഥിരം netDelayp = nodep->user1() ? nullptr : nodep->getLhsNetDelay();
        എങ്കിൽ (netDelayp) {
            എങ്കിൽ (nodep->timingControlp()) {
                // If this assignment has a delay, create another one to handle the net delay
                AstVarScope* സ്ഥിരം newvscp = m_assignWTmps.make(flp, m_scopep, nodep->dtypep());
                AstAssignW* assignp = പുതിയ AstAssignW{
                    nodep->fileline(), nodep->lhsp()->unlinkFrBack(),
                    പുതിയ AstVarRef{flp, newvscp, VAccess::READ}, netDelayp->clone_വൃക്ഷം(അസത്യം)};
                assignp->user1(സത്യം);
                nodep->addNextHere(assignp);
                nodep->lhsp(പുതിയ AstVarRef{flp, newvscp, VAccess::WRITE});
            } അല്ലെങ്കിൽ {
                // Else just use this one with the net delay
                nodep->timingControlp(netDelayp->clone_വൃക്ഷം(അസത്യം));
            }
        }
        എങ്കിൽ (!nodep->timingControlp()) തിരികെ;
        // There will be some circular logic here, suppress the warning for newly created vars
        flp->modifyWarnOff(V3ErrorCode::UNOPTFLAT, സത്യം);
        // Also suppress the warning for the LHS var, for cases like `assign #1 clk = ~clk;`
        // TODO: Restore the warning for other, non-delayed drivers
        nodep->lhsp()->foreach([](AstVarRef* refp) {
            എങ്കിൽ (refp->access().isWriteOrRW()) {
                refp->varp()->fileline()->modifyWarnOff(V3ErrorCode::UNOPTFLAT, സത്യം);
            }
        });
        // Should be under an always
        AstAlways* സ്ഥിരം alwaysp = VN_AS(m_procp, Always);
        // Convert it to an Assign; the new assign with intra delay will be handled by
        // visit(AstNodeAssign*)
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം lhs1p = nodep->lhsp()->unlinkFrBack();
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം rhs1p = nodep->rhsp()->unlinkFrBack();
        Ast_കണ്ണി* സ്ഥിരം controlp = nodep->timingControlp()->unlinkFrBack();
        എങ്കിൽ (AstDelay* സ്ഥിരം delayp = VN_CAST(controlp, Delay)) {
            എങ്കിൽ (Ast_കണ്ണി_വ്യഞ്ജകം* fallDelayp = delayp->fallDelay()) {
                fallDelayp = fallDelayp->unlinkFrBack();
                Ast_കണ്ണി_വ്യഞ്ജകം* lhsp = delayp->lhsp()->unlinkFrBack();
                // Use fall only for an all-zero value, rise otherwise.
                lhsp = delayToIntegral(delayp, lhsp);
                fallDelayp = delayToIntegral(delayp, fallDelayp);
                delayp->lhsp(
                    പുതിയ AstCond{flp, പുതിയ AstEq{flp, rhs1p->clone_വൃക്ഷം(അസത്യം), പുതിയ AstConst{flp, 0}},
                                fallDelayp, lhsp});
            }
        }
        AstAssign* സ്ഥിരം assignp = പുതിയ AstAssign{nodep->fileline(), lhs1p, rhs1p, controlp};
        // Put the assignment in a fork..join_none.
        AstFork* സ്ഥിരം forkp = പുതിയ AstFork{flp, VJoinType::JOIN_NONE};
        nodep->replaceWith(forkp);
        VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
        AstBegin* സ്ഥിരം beginp = പുതിയ AstBegin{flp, "", assignp, അസത്യം};
        forkp->addForksp(beginp);
        visit(forkp);  // Visit now as we need to do some post-processing
        // IEEE 1800-2023 10.3.3 - if the RHS value differs from the currently scheduled value to
        // be assigned, the currently scheduled assignment is descheduled. To keep track if an
        // assignment should be descheduled, each scheduled assignment event has a 'generation',
        // and if at assignment time its generation differs from the current generation, it won't
        // be performed
        AstAssign* സ്ഥിരം preAssignp = VN_AS(beginp->stmtsp(), Assign);
        UASSERT_OBJ(preAssignp, alwaysp, "Pre-assign should be there from visit(AstFork)");
        AstAssign* സ്ഥിരം postAssignp = VN_AS(preAssignp->nextp()->nextp(), Assign);
        UASSERT_OBJ(postAssignp, alwaysp, "Post-assign should be there from visit(AstFork)");
        // Increment generation and copy it to a local
        AstVarScope* സ്ഥിരം generationVarp
            = m_assignWGens.make(flp, m_scopep, alwaysp->findUInt64DType());
        AstVarScope* സ്ഥിരം genLocalVarp
            = createTemp(flp, generationVarp->varp()->name() + "__local",
                         alwaysp->findUInt64DType(), preAssignp);
        preAssignp->addHereThisAsNext(
            പുതിയ AstAssign{flp, പുതിയ AstVarRef{flp, generationVarp, VAccess::WRITE},
                          പുതിയ AstAdd{flp, പുതിയ AstVarRef{flp, generationVarp, VAccess::READ},
                                     പുതിയ AstConst{flp, 1}}});
        preAssignp->addHereThisAsNext(
            പുതിയ AstAssign{flp, പുതിയ AstVarRef{flp, genLocalVarp, VAccess::WRITE},
                          പുതിയ AstVarRef{flp, generationVarp, VAccess::READ}});
        // If the current generation is same as the one saved in the local var, assign
        beginp->addStmtsp(
            പുതിയ AstIf{flp,
                      പുതിയ AstEq{flp, പുതിയ AstVarRef{flp, generationVarp, VAccess::READ},
                                പുതിയ AstVarRef{flp, genLocalVarp, VAccess::READ}},
                      postAssignp->unlinkFrBack()});
        // Save scheduled RHS value before delay
        AstVarScope* സ്ഥിരം tmpVarp
            = m_assignWTmps.make(flp, m_scopep, preAssignp->rhsp()->dtypep());
        AstVarRef* സ്ഥിരം tmpAssignRhsp = VN_AS(preAssignp->lhsp(), VarRef)->clone_വൃക്ഷം(അസത്യം);
        tmpAssignRhsp->access(VAccess::WRITE);
        preAssignp->addNextHere(
            പുതിയ AstAssign{flp, പുതിയ AstVarRef{flp, tmpVarp, VAccess::WRITE}, tmpAssignRhsp});
        // If the RHS is different from the currently scheduled value, schedule the new assignment
        // The generation will increase, effectively 'descheduling' the previous assignment.
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം didNotInitp
            = പുതിയ AstLogNot{flp, പുതിയ AstCExpr{flp, "vlSymsp->__Vm_didInit", 1}};
        AstVarRef* സ്ഥിരം firstIterp
            = പുതിയ AstVarRef{flp, m_netlistp->stlFirstIterationp(), VAccess::READ};
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം schedCondp
            = പുതിയ AstLogOr{flp,
                           പുതിയ AstNeq{flp, preAssignp->rhsp()->clone_വൃക്ഷം(അസത്യം),
                                      പുതിയ AstVarRef{flp, tmpVarp, VAccess::READ}},
                           പുതിയ AstLogAnd{flp, didNotInitp, firstIterp}};
        alwaysp->addStmtsp(പുതിയ AstIf{flp, schedCondp, forkp->unlinkFrBack()});
    }
    ശൂന്യം visit(AstDisableFork* nodep) override {
        എങ്കിൽ (m_hasProcess) തിരികെ;
        // never reached by any process; remove to avoid compilation error
        VL_DO_DANGLING(nodep->unlinkFrBack()->delete_വൃക്ഷം(), nodep);
    }
    ശൂന്യം visit(AstWaitFork* nodep) override {
        എങ്കിൽ (m_hasProcess) {
            File_വരി* സ്ഥിരം flp = nodep->fileline();
            AstCExpr* സ്ഥിരം exprp = പുതിയ AstCExpr{flp, "vlProcess->completedFork()", 1};
            AstWait* സ്ഥിരം waitp = പുതിയ AstWait{flp, exprp, nullptr};
            nodep->replaceWith(waitp);
        } അല്ലെങ്കിൽ {
            // never reached by any process; remove to avoid compilation error
            nodep->unlinkFrBack();
        }
        VL_DO_DANGLING(nodep->delete_വൃക്ഷം(), nodep);
    }
    ശൂന്യം visit(AstWait* nodep) override {
        // Wait on changed events related to the vars in the wait statement
        UINFO(9, "control-visit " << nodep);
        File_വരി* സ്ഥിരം flp = nodep->fileline();
        Ast_കണ്ണി* സ്ഥിരം stmtsp = nodep->stmtsp();
        എങ്കിൽ (stmtsp) stmtsp->unlinkFrBackWithNext();
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം condp = V3Const::constifyEdit(nodep->condp()->unlinkFrBack());
        auto* സ്ഥിരം constp = VN_CAST(condp, Const);
        എങ്കിൽ (constp) {
            എങ്കിൽ (constp->isZero()) {
                // We have to await forever instead of simply returning in case we're deep in a
                // callstack
                AstCMethodHard* സ്ഥിരം foreverMethodp = പുതിയ AstCMethodHard{
                    flp, പുതിയ AstVarRef{flp, getCreateDelayScheduler(), VAccess::WRITE},
                    VCMethod::SCHED_WAIT_FOREVER};
                foreverMethodp->dtypeSetVoid();
                addProcessInfo(foreverMethodp);
                addDebugInfo(foreverMethodp);
                AstCAwait* സ്ഥിരം awaitp = പുതിയ AstCAwait{flp, foreverMethodp};
                nodep->replaceWith(awaitp);
                എങ്കിൽ (stmtsp) VL_DO_DANGLING(stmtsp->delete_വൃക്ഷം(), stmtsp);
                VL_DO_DANGLING(condp->delete_വൃക്ഷം(), condp);
            } അല്ലെങ്കിൽ {
                nodep->v3fatalSrc("constant wait should have been removed in "
                                  "TimingSuspendableVisitor::visit(AstWait)");
            }
        } അല്ലെങ്കിൽ എങ്കിൽ (needDynamicTrigger(condp)) {
            // No point in making a sentree, just use the expression as sensitivity
            // Put the event control in an if so we only wait if the condition isn't met already
            auto* സ്ഥിരം ifp = പുതിയ AstIf{
                flp, പുതിയ AstLogNot{flp, condp},
                പുതിയ AstEventControl{flp,
                                    പുതിയ Ast_Sen_വൃക്ഷം{flp, പുതിയ AstSenItem{flp, VEdgeType::ET_TRUE,
                                                                       condp->clone_വൃക്ഷം(അസത്യം)}},
                                    nullptr}};
            എങ്കിൽ (stmtsp) Ast_കണ്ണി::addNext<Ast_കണ്ണി, Ast_കണ്ണി>(ifp, stmtsp);
            nodep->replaceWith(ifp);
        } അല്ലെങ്കിൽ {
            // We are using a global sentree, so we cannot use ET_TRUE, as that could lead to the
            // active region never converging. Because of this, we need to use ET_CHANGED in a
            // loop.
            AstEventControl* സ്ഥിരം controlp = പുതിയ AstEventControl{
                flp,
                പുതിയ Ast_Sen_വൃക്ഷം{
                    flp, പുതിയ AstSenItem{flp, VEdgeType::ET_CHANGED, condp->clone_വൃക്ഷം(അസത്യം)}},
                nullptr};
            controlp->user2(സത്യം);  // Commit immediately
            AstLoop* സ്ഥിരം loopp = പുതിയ AstLoop{flp};
            loopp->addStmtsp(പുതിയ AstLoopTest{flp, loopp, പുതിയ AstLogNot{flp, condp}});
            loopp->addStmtsp(controlp);
            എങ്കിൽ (stmtsp) Ast_കണ്ണി::addNext<Ast_കണ്ണി, Ast_കണ്ണി>(loopp, stmtsp);
            nodep->replaceWith(loopp);
        }
        VL_DO_DANGLING(nodep->delete_വൃക്ഷം(), nodep);
    }
    ശൂന്യം visit(AstBegin* nodep) override {
        VL_RESTORER(m_procp);
        VL_RESTORER(m_hasProcess);
        m_hasProcess |= hasFlags(nodep, T_HAS_PROC);
        // A pending NBA update is not a subprocess of the process that scheduled it, so it is
        // unaffected by 'disable fork' and 'wait fork' (IEEE 1800-2023 9.6.1, 9.6.3)
        എങ്കിൽ (hasFlags(nodep, T_NBA_UPDATE)) m_hasProcess = അസത്യം;
        m_procp = nodep;
        എങ്കിൽ (m_hasProcess) nodep->setNeedProcess();
        iterateChildren(nodep);
    }
    ശൂന്യം visit(AstFork* nodep) override {
        എങ്കിൽ (nodep->user1SetOnce()) തിരികെ;
        UINFO(9, "control-visit " << nodep);
        v3Global.setUsesTiming();

        // Create a unique name for this fork
        nodep->name("__Vfork_" + std::to_string(++m_forkCnt));

        // TODO: Should process nodep->stmtsp() in case an earlier pass
        //       inserted something there, but as of today we don't need to.

        // Process and name each fork
        size_t idx = 0;  // Index for naming begins
        വേണ്ടി (AstBegin *itemp = nodep->forksp(), *nextp; itemp; itemp = nextp) {
            nextp = VN_AS(itemp->nextp(), Begin);
            iterate(itemp);
            // Note: Even if we do not find any awaits, we cannot simply inline
            // the process here, as new awaits could be added later.

            // Name the begin (later the name will be used for a new function)
            itemp->name(nodep->name() + "__" + std::to_string(idx++));
            എങ്കിൽ (itemp->needProcess()) {
                itemp->addStmtsp(
                    പുതിയ AstCStmt{itemp->fileline(), "vlProcess->state(VlProcess::FINISHED);"});
            }
        }
        എങ്കിൽ (!nodep->joinType().joinNone()) makeForkJoin(nodep);
    }

    //--------------------
    ശൂന്യം visit(AstVar*) override {}  // Accelerate
    ശൂന്യം visit(Ast_കണ്ണി* nodep) override { iterateChildren(nodep); }

പൊതു:
    // CONSTRUCTORS
    explicit TimingControlVisitor(AstNetlist* nodep)
        : m_netlistp{nodep} {
        iterate(nodep);

        // If there is no static #0 in the design, but an unknown delay was found,
        // and the user did not specify preference, then warn on all unknown delays
        // as we will be assuming they can be #0, which can cause performance degradation.
        വേണ്ടി (File_വരി* സ്ഥിരം flp : m_unknownDelayFlps) {
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

Ast_കണ്ണി_വ്യഞ്ജകം* V3Timing::delayToTicks(Ast_കണ്ണി_വ്യഞ്ജകം* valuep, VTimescale timeunit) {
    File_വരി* സ്ഥിരം flp = valuep->fileline();
    എങ്കിൽ (സ്ഥിരം AstConst* സ്ഥിരം constp = VN_CAST(valuep, Const)) {
        എങ്കിൽ (constp->അക്കം().is1Step()) {
            // A step is one tick of the global time precision (IEEE 1800-2023 3.14.3)
            VL_DO_DANGLING(valuep->delete_വൃക്ഷം(), valuep);
            AstConst* സ്ഥിരം tickp = പുതിയ AstConst{flp, AstConst::Unsized64{}, 1};
            tickp->dtypeSetBitSized(64, VSigning::UNSIGNED);
            തിരികെ tickp;
        }
        എങ്കിൽ (constp->isZero()) തിരികെ valuep;
    }
    UASSERT_OBJ(!timeunit.isNone(), valuep, "timeunit must be set");
    സ്ഥിരം ഇരട്ട timescaleFactorD
        = std::pow(10.0, timeunit.powerOfTen() - v3Global.rootp()->timeprecision().powerOfTen());
    എങ്കിൽ (valuep->dtypep()->skipRefp()->isDouble()) {
        AstConst* സ്ഥിരം tsfp = പുതിയ AstConst{flp, AstConst::RealDouble{}, timescaleFactorD};
        valuep = പുതിയ AstMulD{flp, valuep, tsfp};
        valuep = പുതിയ AstRToIRoundS{flp, valuep};
        valuep->dtypeSetBitSized(64, VSigning::UNSIGNED);
    } അല്ലെങ്കിൽ {
        valuep->dtypeSetBitSized(64, VSigning::UNSIGNED);
        സ്ഥിരം uint64_t timescaleFactorU = static_cast<uint64_t>(timescaleFactorD);
        AstConst* സ്ഥിരം tsfp = പുതിയ AstConst{flp, AstConst::Unsized64{}, timescaleFactorU};
        valuep = പുതിയ AstMul{flp, valuep, tsfp};
    }
    തിരികെ V3Const::constifyEdit(valuep);  // Simplify
}

ശൂന്യം V3Timing::timingAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        സ്ഥിരം VNUser1InUse m_user1InUse;
        സ്ഥിരം VNUser2InUse m_user2InUse;
        { TimingSuspendableVisitor{nodep}; }
        എങ്കിൽ (v3Global.usesTiming()) TimingControlVisitor{nodep};
    }
    V3Global::dump_Check_Global_വൃക്ഷം("timing", 0, dump_വൃക്ഷം_Either_Level() >= 3);
}
