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

#ଅନ୍ତର୍ଭାଵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ଅନ୍ତର୍ଭାଵ "V3Timing.h"

#ଅନ୍ତର୍ଭାଵ "V3Const.h"
#ଅନ୍ତର୍ଭାଵ "V3EmitV.h"
#ଅନ୍ତର୍ଭାଵ "V3File.h"
#ଅନ୍ତର୍ଭାଵ "V3Global.h"
#ଅନ୍ତର୍ଭାଵ "V3Graph.h"
#ଅନ୍ତର୍ଭାଵ "V3MemberMap.h"
#ଅନ୍ତର୍ଭାଵ "V3SenExprBuilder.h"
#ଅନ୍ତର୍ଭାଵ "V3SenTree.h"
#ଅନ୍ତର୍ଭାଵ "V3SharedTmps.h"
#ଅନ୍ତର୍ଭାଵ "V3Stats.h"
#ଅନ୍ତର୍ଭାଵ "V3UniqueNames.h"

#ଅନ୍ତର୍ଭାଵ <limits>
#ଅନ୍ତର୍ଭାଵ <queue>

VL_DEFINE_DEBUG_FUNCTIONS;

// ######################################################################

ଗଣନା ଗଣ୍ଠି_Flag : uint8_t {
    T_SUSPENDEE = 1 << 0,  // Suspendable (due to dependence on another suspendable)
    T_SUSPENDER = 1 << 1,  // Suspendable (has timing control)
    T_ALLOCS_PROC = 1 << 2,  // Can allocate VlProcess
    T_FORCES_PROC = 1 << 3,  // Forces VlProcess allocation
    T_NEEDS_PROC = 1 << 4,  // Needs access to VlProcess if it's allocated
    T_HAS_PROC = 1 << 5,  // Has VlProcess argument in the signature
    T_NBA_UPDATE = 1 << 6,  // Fork branch of a pending NBA update, which is not a subprocess
};

ଗଣନା ForkType : uint8_t {
    F_NONE = 0,  // Not under a fork
    F_MIGHT_SUSPEND = 1 << 0,  // Fork might suspend the execution of current process
    F_MIGHT_NEED_PROC = 1 << 1,  // Fork might need a process (any fork really)
};

ଗଣନା PropagationType : uint8_t {
    P_CALL = 1,  // Propagation through call to a function/task/method
    P_FORK = 2,  // Propagation due to fork's behavior
    P_SIGNATURE = 3,  // Propagation required to maintain C++ function's signature requirements
};

// Add timing flag to a node
ସ୍ଥିର ଶୂନ୍ୟ addFlags(Ast_ଗଣ୍ଠି* ଅଚଳ nodep, uint8_t flags) { nodep->user2Or(flags); }
// Check if a node has ALL of the expected flags set
ସ୍ଥିର ତାର୍କିକ hasFlags(Ast_ଗଣ୍ଠି* ଅଚଳ nodep, uint8_t flags) { ଫେରନ୍ତୁ !(~nodep->user2() & flags); }

// ######################################################################
//  Detect nodes affected by timing and/or requiring a process

ଶ୍ରେଣୀ TimingSuspendableVisitor final : ସର୍ବସାଧାରଣ VNVisitor {
    // TYPES
    // Vertex of a dependency graph of suspendable nodes, e.g. if a node (process or task) is
    // suspendable, all its dependents should also be suspendable
    ଶ୍ରେଣୀ DepVtx VL_NOT_FINAL : ସର୍ବସାଧାରଣ V3GraphVertex {
        VL_RTTI_IMPL(DepVtx, V3GraphVertex)
        AstClass* ଅଚଳ m_classp;  // Class associated with a method
        Ast_ଗଣ୍ଠି* ଅଚଳ m_nodep;  // AST node represented by this graph vertex

        // ACCESSORS
        string name() ଅଚଳ override {
            ଯଦି (m_classp) {
                ଯଦି (VN_IS(nodep(), CFunc)) {
                    ଫେରନ୍ତୁ cvtToHex(nodep()) + ' ' + classp()->name() + "::" + nodep()->name();
                }
            }
            ଫେରନ୍ତୁ cvtToHex(nodep()) + ' ' + nodep()->prettyTypeName();
        }
        File_ଧାଡ଼ି* fileline() ଅଚଳ override { ଫେରନ୍ତୁ nodep()->fileline(); }

    ସର୍ବସାଧାରଣ:
        // CONSTRUCTORS
        DepVtx(V3Graph* graphp, Ast_ଗଣ୍ଠି* nodep, AstClass* classp)
            : V3GraphVertex{graphp}
            , m_classp{classp}
            , m_nodep{nodep} {}
        ~DepVtx() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;

        // ACCESSORS
        ଆଭାସୀ Ast_ଗଣ୍ଠି* nodep() ଅଚଳ VL_MT_STABLE { ଫେରନ୍ତୁ m_nodep; }
        ଆଭାସୀ Ast_ଗଣ୍ଠି* classp() ଅଚଳ VL_MT_STABLE { ଫେରନ୍ତୁ m_classp; }
    };

    ଶ୍ରେଣୀ SuspendDepVtx final : ସର୍ବସାଧାରଣ DepVtx {
        VL_RTTI_IMPL(SuspendDepVtx, DepVtx)
        string dotColor() ଅଚଳ override {
            ଯଦି (hasFlags(nodep(), T_SUSPENDER)) ଫେରନ୍ତୁ "red";
            ଯଦି (hasFlags(nodep(), T_SUSPENDEE)) ଫେରନ୍ତୁ "blue";
            ଫେରନ୍ତୁ "black";
        }

    ସର୍ବସାଧାରଣ:
        SuspendDepVtx(V3Graph* graphp, Ast_ଗଣ୍ଠି* nodep, AstClass* classp)
            : DepVtx{graphp, nodep, classp} {}
        ~SuspendDepVtx() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    };

    ଶ୍ରେଣୀ NeedsProcDepVtx final : ସର୍ବସାଧାରଣ DepVtx {
        VL_RTTI_IMPL(NeedsProcDepVtx, DepVtx)
        string dotColor() ଅଚଳ override {
            ଯଦି (hasFlags(nodep(), T_HAS_PROC)) ଫେରନ୍ତୁ "blue";
            ଯଦି (hasFlags(nodep(), T_NEEDS_PROC)) ଫେରନ୍ତୁ "green";
            ଯଦି (hasFlags(nodep(), T_FORCES_PROC)) ଫେରନ୍ତୁ "red";
            ଫେରନ୍ତୁ "black";
        }

    ସର୍ବସାଧାରଣ:
        NeedsProcDepVtx(V3Graph* graphp, Ast_ଗଣ୍ଠି* nodep, AstClass* classp)
            : DepVtx{graphp, nodep, classp} {}
        ~NeedsProcDepVtx() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    };

    // NODE STATE
    //  AstClass::user1() -> bool.  Class member cache has been refreshed.
    //  Ast{NodeProcedure,CFunc,Begin}::user2()  -> uint64_t.  Set to >= T_SUSP if
    //                                                         process/task suspendable
    //                                                         and to T_PROC if it
    //                                                         needs process metadata.
    //  Ast{NodeProcedure,CFunc,Begin}::user3()  -> DependencyVertex*.  Vertex in m_suspGraph
    //  Ast{NodeProcedure,CFunc,Begin}::user3()  -> DependencyVertex*.  Vertex in m_procGraph
    ଅଚଳ VNUser3InUse m_user3InUse;
    ଅଚଳ VNUser4InUse m_user4InUse;

    // STATE
    VMemberMap m_memberMap;  // Member names cached for fast lookup
    AstClass* m_classp = nullptr;  // Current class
    Ast_ଗଣ୍ଠି* m_procp = nullptr;  // NodeProcedure/CFunc/Begin we're under
    uint8_t m_underFork = F_NONE;  // F_NONE or flags of a fork we are under
    V3Graph m_suspGraph;  // Dependency graph where a node is a dependency of another if it being
                          // suspendable makes the other node suspendable
    V3Graph m_procGraph;  // Dependency graph where a node is a dependency of another if it being
                          // suspendable makes the other node suspendable

    // METHODS
    // Get or create the dependency vertex for the given node
    DepVtx* getSuspendDepVtx(Ast_ଗଣ୍ଠି* ଅଚଳ nodep) {
        AstClass* classp = nullptr;
        ଯଦି (AstCFunc* funcp = VN_CAST(nodep, CFunc)) {
            ଯଦି (funcp->scopep() && funcp->scopep()->modp()) {
                classp = VN_CAST(funcp->scopep()->modp(), Class);
            }
        }
        ଯଦି (!nodep->user3p()) nodep->user3p(ନୂତନ SuspendDepVtx{&m_suspGraph, nodep, classp});
        ଫେରନ୍ତୁ nodep->user3u().to<SuspendDepVtx*>();
    }
    DepVtx* getNeedsProcDepVtx(Ast_ଗଣ୍ଠି* ଅଚଳ nodep) {
        AstClass* classp = nullptr;
        ଯଦି (AstCFunc* funcp = VN_CAST(nodep, CFunc)) {
            ଯଦି (funcp->scopep() && funcp->scopep()->modp()) {
                classp = VN_CAST(funcp->scopep()->modp(), Class);
            }
        }
        ଯଦି (!nodep->user4p()) nodep->user4p(ନୂତନ NeedsProcDepVtx{&m_procGraph, nodep, classp});
        ଫେରନ୍ତୁ nodep->user4u().to<NeedsProcDepVtx*>();
    }
    // Pass timing flag between nodes
    ତାର୍କିକ passFlag(ଅଚଳ Ast_ଗଣ୍ଠି* from, Ast_ଗଣ୍ଠି* to, ଗଣ୍ଠି_Flag flag) {
        ଯଦି ((from->user2() & flag) && !(to->user2() & flag)) {
            addFlags(to, flag);
            ଫେରନ୍ତୁ ସତ୍ୟ;
        }
        ଫେରନ୍ତୁ ମିଥ୍ୟା;
    }
    // Propagate flag to all nodes that depend on the given one
    ଶୂନ୍ୟ propagateFlags(DepVtx* ଅଚଳ vxp, ଗଣ୍ଠି_Flag flag) {
        auto* ଅଚଳ parentp = vxp->nodep();
        ପାଇଁ (V3GraphEdge& edge : vxp->outEdges()) {
            auto* ଅଚଳ depVxp = static_cast<DepVtx*>(edge.ଶୀର୍ଷ());
            Ast_ଗଣ୍ଠି* ଅଚଳ depp = depVxp->nodep();
            ଯଦି (passFlag(parentp, depp, flag)) propagateFlags(depVxp, flag);
        }
    }
    template <typename T_Predicate>
    ଶୂନ୍ୟ propagateFlagsIf(DepVtx* ଅଚଳ vxp, ଗଣ୍ଠି_Flag flag, T_Predicate p) {
        auto* ଅଚଳ parentp = vxp->nodep();
        ପାଇଁ (V3GraphEdge& edge : vxp->outEdges()) {
            auto* ଅଚଳ depVxp = static_cast<DepVtx*>(edge.ଶୀର୍ଷ());
            Ast_ଗଣ୍ଠି* ଅଚଳ depp = depVxp->nodep();
            ଯଦି (p(&edge) && passFlag(parentp, depp, flag)) propagateFlagsIf(depVxp, flag, p);
        }
    }
    template <typename T_Predicate>
    ଶୂନ୍ୟ propagateFlagsReversedIf(DepVtx* ଅଚଳ vxp, ଗଣ୍ଠି_Flag flag, T_Predicate p) {
        auto* ଅଚଳ parentp = vxp->nodep();
        ପାଇଁ (V3GraphEdge& edge : vxp->inEdges()) {
            auto* ଅଚଳ depVxp = static_cast<DepVtx*>(edge.fromp());
            Ast_ଗଣ୍ଠି* ଅଚଳ depp = depVxp->nodep();
            ଯଦି (p(&edge) && passFlag(parentp, depp, flag))
                propagateFlagsReversedIf(depVxp, flag, p);
        }
    }

    // VISITORS
    ଶୂନ୍ୟ visit(AstClass* nodep) override {
        UASSERT_OBJ(!m_classp, nodep, "Class under class");
        VL_RESTORER(m_classp);
        m_classp = nodep;
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Procedure* nodep) override {
        VL_RESTORER(m_procp);
        m_procp = nodep;
        getNeedsProcDepVtx(nodep);
        addFlags(nodep, T_ALLOCS_PROC);
        ଯଦି (VN_IS(nodep, Always)) {
            UINFO(9, "Always does " << (nodep->needProcess() ? "" : "NOT ") << "need process");
        }
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(AstDisableFork* nodep) override {
        visit(static_cast<Ast_ଗଣ୍ଠି*>(nodep));
        addFlags(m_procp, T_FORCES_PROC | T_NEEDS_PROC);
    }
    ଶୂନ୍ୟ visit(AstWaitFork* nodep) override {
        visit(static_cast<Ast_ଗଣ୍ଠି*>(nodep));
        addFlags(m_procp, T_FORCES_PROC | T_NEEDS_PROC);
    }
    ଶୂନ୍ୟ visit(AstWait* nodep) override {
        UINFO(9, "suspendable-visit " << nodep);
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ condp = V3Const::constifyEdit(nodep->condp());
        ଯଦି (AstConst* ଅଚଳ constp = VN_CAST(condp, Const)) {
            ଯଦି (!nodep->fileline()->warnIsOff(V3ErrorCode::WAITCONST)) {
                condp->v3warn(WAITCONST, "Wait statement condition is constant");
            }
            ଯଦି (!constp->isZero()) {
                // Remove AstWait before we track process as T_SUSPENDER
                ଯଦି (Ast_ଗଣ୍ଠି* ଅଚଳ stmtsp = nodep->stmtsp()) {
                    stmtsp->unlinkFrBackWithNext();
                    nodep->replaceWith(stmtsp);
                } ଅନ୍ୟଥା {
                    nodep->unlinkFrBack();
                }
                VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
                ଫେରନ୍ତୁ;
            }
        }
        v3Global.setUsesTiming();
        ଯଦି (m_procp) addFlags(m_procp, T_SUSPENDEE | T_SUSPENDER | T_NEEDS_PROC);
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(AstCFunc* nodep) override {
        VL_RESTORER(m_procp);
        m_procp = nodep;
        iterateChildren(nodep);
        ଯଦି (nodep->needProcess()) addFlags(nodep, T_FORCES_PROC | T_NEEDS_PROC);
        DepVtx* ଅଚଳ sVxp = getSuspendDepVtx(nodep);
        DepVtx* ଅଚଳ pVxp = getNeedsProcDepVtx(nodep);
        ଯଦି (!m_classp) ଫେରନ୍ତୁ;

        // Go over overridden functions

        std::queue<AstClassExtends*> extends;
        ଯଦି (m_classp->extendsp()) extends.ଠେଲ(m_classp->extendsp());

        ଯେତେବେଳେ (!extends.empty()) {
            AstClassExtends* ext_list = extends.front();
            extends.କାଢ଼();

            ପାଇଁ (AstClassExtends* cextp = ext_list; cextp;
                 cextp = VN_AS(cextp->nextp(), ClassExtends)) {
                // TODO: It is possible that a methods the same name in the base class is not
                // actually overridden by our method. If this causes a problem, traverse to
                // the root of the inheritance hierarchy and check if the original method is
                // virtual or not.
                ଯଦି (auto* ଅଚଳ overriddenp
                    = VN_CAST(m_memberMap.findMember(cextp->classp(), nodep->name()), CFunc)) {
                    // Suspendability and process affects typing, so they propagate both ways
                    DepVtx* ଅଚଳ overriddenSVxp = getSuspendDepVtx(overriddenp);
                    DepVtx* ଅଚଳ overriddenPVxp = getNeedsProcDepVtx(overriddenp);
                    ନୂତନ V3GraphEdge{&m_suspGraph, sVxp, overriddenSVxp, P_SIGNATURE};
                    ନୂତନ V3GraphEdge{&m_suspGraph, overriddenSVxp, sVxp, P_SIGNATURE};
                    ନୂତନ V3GraphEdge{&m_procGraph, pVxp, overriddenPVxp, P_SIGNATURE};
                    ନୂତନ V3GraphEdge{&m_procGraph, overriddenPVxp, pVxp, P_SIGNATURE};
                } ଅନ୍ୟଥା {
                    AstClassExtends* more_extends = cextp->classp()->extendsp();
                    ଯଦି (more_extends) extends.ଠେଲ(more_extends);
                }
            }
        }
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_CCall* nodep) override {
        AstCFunc* ଅଚଳ funcp = nodep->funcp();
        UASSERT_OBJ(funcp, nodep, "AstNodeCCall must have non-null funcp post-link");
        UASSERT_OBJ(m_procp, nodep, "AstNodeCCall must be inside a procedure/CFunc/Begin");

        UINFO(9, "V3Timing: Processing CCall to " << funcp->name() << " in dependency graph\n");
        ନୂତନ V3GraphEdge{&m_suspGraph, getSuspendDepVtx(funcp), getSuspendDepVtx(m_procp), P_CALL};

        ନୂତନ V3GraphEdge{&m_procGraph, getNeedsProcDepVtx(funcp), getNeedsProcDepVtx(m_procp),
                        P_CALL};

        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(AstBegin* nodep) override {
        VL_RESTORER(m_procp);
        VL_RESTORER(m_underFork);

        ଯଦି (!m_underFork || (m_underFork & F_MIGHT_SUSPEND))
            ନୂତନ V3GraphEdge{&m_suspGraph, getSuspendDepVtx(nodep), getSuspendDepVtx(m_procp),
                            m_underFork ? P_FORK : P_CALL};

        ନୂତନ V3GraphEdge{&m_procGraph, getNeedsProcDepVtx(nodep), getNeedsProcDepVtx(m_procp),
                        P_CALL};

        ଯଦି (m_underFork) addFlags(nodep, T_NEEDS_PROC | T_ALLOCS_PROC);

        m_procp = nodep;
        m_underFork = 0;
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(AstFork* nodep) override {
        UINFO(9, "suspendable-visit " << nodep);
        VL_RESTORER(m_underFork);

        v3Global.setUsesTiming();  // Even if there are no event controls, we have to set this flag
                                   // so that transformForks() in V3SchedTiming gets called and
                                   // removes all forks and begins
        ଯଦି (nodep->isTimingControl() && m_procp) {
            addFlags(m_procp, T_SUSPENDEE | T_SUSPENDER);
            m_underFork |= F_MIGHT_SUSPEND;
        }
        m_underFork |= F_MIGHT_NEED_PROC;
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(AstAssignDly* nodep) override {
        ଯଦି (!VN_IS(m_procp, ଗଣ୍ଠି_Procedure)) v3Global.setUsesTiming();
        visit(static_cast<Ast_ଗଣ୍ଠି*>(nodep));
    }
    ଶୂନ୍ୟ visit(AstFireEvent* nodep) override {
        ଯଦି (nodep->isDelayed()
            && (!VN_IS(m_procp, ଗଣ୍ଠି_Procedure) || !VN_IS(nodep->operandp(), VarRef))) {
            v3Global.setUsesTiming();
        }
        visit(static_cast<Ast_ଗଣ୍ଠି*>(nodep));
    }
    ଶୂନ୍ୟ visit(AstAssignW* nodep) override {
        ଯଦି (nodep->timingControlp()) v3Global.setUsesTiming();
        // Containing process will not suspend, don't mark it
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override {
        ଯଦି (nodep->isTimingControl()) {
            v3Global.setUsesTiming();
            ଯଦି (m_procp) addFlags(m_procp, T_SUSPENDEE | T_SUSPENDER | T_NEEDS_PROC);
        }
        iterateChildren(nodep);
    }

    //--------------------
    ଶୂନ୍ୟ visit(AstVar*) override {}  // Accelerate

ସର୍ବସାଧାରଣ:
    // CONSTRUCTORS
    explicit TimingSuspendableVisitor(AstNetlist* nodep) {
        iterate(nodep);
        m_suspGraph.removeTransitiveEdges();
        m_procGraph.removeTransitiveEdges();
        // Propagate suspendability
        ପାଇଁ (V3GraphVertex& vtx : m_suspGraph.vertices()) {
            DepVtx& depVtx = static_cast<DepVtx&>(vtx);
            ଯଦି (hasFlags(depVtx.nodep(), T_SUSPENDEE)) propagateFlags(&depVtx, T_SUSPENDEE);
        }
        ଯଦି (dumpGraphLevel() >= 6) m_suspGraph.dumpDotFilePrefixed("timing_deps");

        // Propagate T_HAS_PROCESS
        ପାଇଁ (V3GraphVertex& vtx : m_procGraph.vertices()) {
            DepVtx& depVtx = static_cast<DepVtx&>(vtx);
            // Find processes that'll allocate VlProcess
            ଯଦି (hasFlags(depVtx.nodep(), T_FORCES_PROC)) {
                propagateFlagsIf(&depVtx, T_FORCES_PROC, [&](ଅଚଳ V3GraphEdge* e) -> ତାର୍କିକ {
                    ଫେରନ୍ତୁ !hasFlags(static_cast<DepVtx*>(e->fromp())->nodep(), T_ALLOCS_PROC);
                });
            }
            // Mark nodes on paths between processes and statements that use VlProcess
            ଯଦି (hasFlags(depVtx.nodep(), T_NEEDS_PROC)) {
                propagateFlagsIf(&depVtx, T_NEEDS_PROC, [&](ଅଚଳ V3GraphEdge* e) -> ତାର୍କିକ {
                    ଫେରନ୍ତୁ !hasFlags(static_cast<DepVtx*>(e->ଶୀର୍ଷ())->nodep(), T_ALLOCS_PROC);
                });
            }
        }
        ପାଇଁ (V3GraphVertex& vtx : m_procGraph.vertices()) {
            DepVtx& depVtx = static_cast<DepVtx&>(vtx);
            // Mark nodes that will be emitted with a VlProcess argument
            ଯଦି (hasFlags(depVtx.nodep(), T_ALLOCS_PROC | T_FORCES_PROC)) {
                addFlags(depVtx.nodep(), T_HAS_PROC);
                propagateFlagsReversedIf(&depVtx, T_HAS_PROC, [&](ଅଚଳ V3GraphEdge* e) -> ତାର୍କିକ {
                    ଫେରନ୍ତୁ hasFlags(static_cast<DepVtx*>(e->fromp())->nodep(), T_NEEDS_PROC);
                });
            }
        }
        ଯଦି (dumpGraphLevel() >= 6) m_procGraph.dumpDotFilePrefixed("proc_deps");
    }
    ~TimingSuspendableVisitor() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
};

// ######################################################################
//  Transform nodes affected by timing

ଶ୍ରେଣୀ TimingControlVisitor final : ସର୍ବସାଧାରଣ VNVisitor {
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
    AstNetlist* ଅଚଳ m_netlistp;  // Root node
    Ast_ଶୀର୍ଷ_Scope* ଅଚଳ m_ଶୀର୍ଷ_Scopep = m_netlistp->ଶୀର୍ଷ_Scopep();  // The AstTopScope
    AstScope* ଅଚଳ m_scopeTopp = m_ଶୀର୍ଷ_Scopep->scopep();  // Scope at the top
    AstClass* m_classp = nullptr;  // Current class
    AstScope* m_scopep = nullptr;  // Current scope
    AstActive* m_activep = nullptr;  // Current active
    Ast_ଗଣ୍ଠି* m_procp = nullptr;  // NodeProcedure/CFunc/Begin we're under
    ତାର୍କିକ m_hasProcess = ମିଥ୍ୟା;  // True if current scope has a VlProcess handle available
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_forkCnt = 0;  // Number of forks inside a module
    ତାର୍କିକ m_underJumpBlock = ମିଥ୍ୟା;  // True if we are inside of a jump-block
    ତାର୍କିକ m_underProcedure = ମିଥ୍ୟା;  // True if we are under an always or initial
    ତାର୍କିକ m_underIfaceCFunc = ମିଥ୍ୟା;  // True if we are under a CFunc owned by an interface scope
    ତାର୍କିକ m_hasStaticZeroDelay = ମିଥ୍ୟା;  // True if we have a static #0 delay
    std::vector<File_ଧାଡ଼ି*> m_unknownDelayFlps;  // Locations of AstDelay with non-constant value

    // Unique names
    V3UniqueNames m_dlyforkNames{"__Vdlyfork"};  // Names for temp AssignW vars
    V3UniqueNames m_intra_ମୂଲ୍ୟ_Names{"__Vintraval"};  // Intra assign delay value var names
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
    Ast_Sen_ବୃକ୍ଷ* m_delaySensesp = nullptr;  // Domain to trigger if a delayed coroutine is resumed
    Ast_Sen_ବୃକ୍ଷ* m_dynamicSensesp = nullptr;  // Domain to trigger if a dynamic trigger is set

    // Other
    Sen_ବୃକ୍ଷ_Finder m_finder{m_netlistp};  // Sentree finder and uniquifier
    Sen_ପଦସମଷ୍ଟି_Builder* m_sen_ପଦସମଷ୍ଟି_Builderp = nullptr;  // Sens expression builder for current m_scope

    // Stats
    size_t m_statZeroDelays = 0;  // Number of statically known #0 delays
    size_t m_statConstDelays = 0;  // Number of statically known #const (non-zero) delays
    size_t m_statVariableDelays = 0;  // Number of delays with value unknown at compile time

    // METHODS
    // Transform an assignment with an intra timing control into a timing control with the
    // assignment under it
    Ast_ଗଣ୍ଠି* factorOutTimingControl(Ast_ଗଣ୍ଠି_Assign* nodep) ଅଚଳ {
        Ast_ଗଣ୍ଠି* stmtp = nodep;
        Ast_ଗଣ୍ଠି* ଅଚଳ controlp = nodep->timingControlp();
        ଯଦି (AstDelay* ଅଚଳ delayp = VN_CAST(controlp, Delay)) {
            stmtp->replaceWith(delayp->unlinkFrBack());
            delayp->addStmtsp(stmtp);
            stmtp = delayp;
        } ଅନ୍ୟଥା ଯଦି (Ast_Sen_ବୃକ୍ଷ* ଅଚଳ sentreep = VN_CAST(controlp, Sen_ବୃକ୍ଷ)) {
            AstEventControl* ଅଚଳ eventControlp
                = ନୂତନ AstEventControl{sentreep->fileline(), sentreep->unlinkFrBack(), nullptr};
            stmtp->replaceWith(eventControlp);
            eventControlp->addStmtsp(stmtp);
            stmtp = eventControlp;
        } ଅନ୍ୟଥା ଯଦି (AstBegin* ଅଚଳ beginp = VN_CAST(controlp, Begin)) {
            // Begin from V3AssertPre
            stmtp->replaceWith(beginp->unlinkFrBack());
            beginp->addStmtsp(stmtp);
            stmtp = beginp;
        }
        ଫେରନ୍ତୁ stmtp == nodep ? nullptr : stmtp;
    }
    ସ୍ଥିର ତାର୍କିକ staticallyNonZeroDelay(ଅଚଳ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* valuep) {
        ଯଦି (ଅଚଳ AstConst* ଅଚଳ constp = VN_CAST(valuep, Const)) ଫେରନ୍ତୁ !constp->isZero();
        ଯଦି (ଅଚଳ AstCond* ଅଚଳ condp = VN_CAST(valuep, Cond)) {
            ଫେରନ୍ତୁ staticallyNonZeroDelay(condp->thenp())
                   && staticallyNonZeroDelay(condp->elsep());
        }
        ଯଦି (ଅଚଳ AstMul* ଅଚଳ mulp = VN_CAST(valuep, Mul)) {
            ଅଚଳ AstConst* ଅଚଳ lhsConstp = VN_CAST(mulp->lhsp(), Const);
            ଅଚଳ AstConst* ଅଚଳ rhsConstp = VN_CAST(mulp->rhsp(), Const);
            ଯଦି (lhsConstp && !lhsConstp->isZero()) ଫେରନ୍ତୁ staticallyNonZeroDelay(mulp->rhsp());
            ଯଦି (rhsConstp && !rhsConstp->isZero()) ଫେରନ୍ତୁ staticallyNonZeroDelay(mulp->lhsp());
        }
        ଫେରନ୍ତୁ ମିଥ୍ୟା;
    }
    // Creates the global delay scheduler variable
    AstVarScope* getCreateDelayScheduler() {
        ଯଦି (m_delaySchedp) ଫେରନ୍ତୁ m_delaySchedp;
        auto* ଅଚଳ dlySchedDtp = ନୂତନ AstBasicDType{
            m_scopeTopp->fileline(), VBasicDTypeKwd::DELAY_SCHEDULER, VSigning::UNSIGNED};
        m_netlistp->typeTablep()->addTypesp(dlySchedDtp);
        m_delaySchedp = m_ଶୀର୍ଷ_Scopep->createTemp("__VdlySched", dlySchedDtp);
        // Delay scheduler has to be accessible from top
        m_delaySchedp->varp()->sigPublic(ସତ୍ୟ);
        m_netlistp->delaySchedulerp(m_delaySchedp->varp());
        ଫେରନ୍ତୁ m_delaySchedp;
    }
    // Creates the delay sentree
    Ast_Sen_ବୃକ୍ଷ* get_Create_Delay_Sen_ବୃକ୍ଷ() {
        ଯଦି (m_delaySensesp) ଫେରନ୍ତୁ m_delaySensesp;
        File_ଧାଡ଼ି* ଅଚଳ flp = m_scopeTopp->fileline();
        auto* ଅଚଳ awaitingCurrentTimep
            = ନୂତନ AstCMethodHard{flp, ନୂତନ AstVarRef{flp, getCreateDelayScheduler(), VAccess::READ},
                                 VCMethod::SCHED_AWAITING_CURRENT_TIME};
        awaitingCurrentTimep->dtypeSetBit();
        m_delaySensesp
            = ନୂତନ Ast_Sen_ବୃକ୍ଷ{flp, ନୂତନ AstSenItem{flp, VEdgeType::ET_TRUE, awaitingCurrentTimep}};
        m_netlistp->ଶୀର୍ଷ_Scopep()->addSenTreesp(m_delaySensesp);
        ଫେରନ୍ତୁ m_delaySensesp;
    }
    // Creates the global dynamic trigger scheduler variable
    AstVarScope* getCreateDynamicTriggerScheduler() {
        ଯଦି (m_dynamicSchedp) ଫେରନ୍ତୁ m_dynamicSchedp;
        auto* ଅଚଳ dynSchedDtp
            = ନୂତନ AstBasicDType{m_scopeTopp->fileline(), VBasicDTypeKwd::DYNAMIC_TRIGGER_SCHEDULER,
                                VSigning::UNSIGNED};
        m_netlistp->typeTablep()->addTypesp(dynSchedDtp);
        m_dynamicSchedp = m_ଶୀର୍ଷ_Scopep->createTemp("__VdynSched", dynSchedDtp);
        ଫେରନ୍ତୁ m_dynamicSchedp;
    }
    // Creates the dynamic trigger sentree
    Ast_Sen_ବୃକ୍ଷ* get_Create_Dynamic_Trigger_Sen_ବୃକ୍ଷ() {
        ଯଦି (m_dynamicSensesp) ଫେରନ୍ତୁ m_dynamicSensesp;
        File_ଧାଡ଼ି* ଅଚଳ flp = m_scopeTopp->fileline();
        auto* ଅଚଳ awaitingCurrentTimep = ନୂତନ AstCMethodHard{
            flp, ନୂତନ AstVarRef{flp, getCreateDynamicTriggerScheduler(), VAccess::READ},
            VCMethod::SCHED_EVALUATE};
        awaitingCurrentTimep->dtypeSetBit();
        m_dynamicSensesp
            = ନୂତନ Ast_Sen_ବୃକ୍ଷ{flp, ନୂତନ AstSenItem{flp, VEdgeType::ET_TRUE, awaitingCurrentTimep}};
        m_netlistp->ଶୀର୍ଷ_Scopep()->addSenTreesp(m_dynamicSensesp);
        ଫେରନ୍ତୁ m_dynamicSensesp;
    }
    // Creates the event variable to trigger in NBA region
    AstEventControl* createNbaEventControl(File_ଧାଡ଼ି* flp) {
        ଯଦି (!m_netlistp->nbaEventp()) {
            auto* ଅଚଳ nbaEventDtp = ନୂତନ AstBasicDType{m_scopeTopp->fileline(),
                                                        VBasicDTypeKwd::EVENT, VSigning::UNSIGNED};
            m_netlistp->typeTablep()->addTypesp(nbaEventDtp);
            m_netlistp->nbaEventp(m_ଶୀର୍ଷ_Scopep->createTemp("__VnbaEvent", nbaEventDtp));
            v3Global.setHasEvents();
        }
        ଫେରନ୍ତୁ ନୂତନ AstEventControl{
            flp,
            ନୂତନ Ast_Sen_ବୃକ୍ଷ{
                flp, ନୂତନ AstSenItem{flp, VEdgeType::ET_EVENT,
                                    ନୂତନ AstVarRef{flp, m_netlistp->nbaEventp(), VAccess::READ}}},
            nullptr};
    }
    // Creates the variable that, if set, causes the NBA event to be triggered
    AstAssign* createNbaEventTriggerAssignment(File_ଧାଡ଼ି* flp) {
        ଯଦି (!m_netlistp->nbaEventTriggerp()) {
            m_netlistp->nbaEventTriggerp(m_ଶୀର୍ଷ_Scopep->createTemp("__VnbaEventTrigger", 1));
        }
        ଫେରନ୍ତୁ ନୂତନ AstAssign{flp,
                             ନୂତନ AstVarRef{flp, m_netlistp->nbaEventTriggerp(), VAccess::WRITE},
                             ନୂତନ AstConst{flp, AstConst::BitTrue{}}};
    }
    // Returns true if we are under a class or interface function, or the tree references locals.
    // These are cases where static, globally-evaluated triggers are not suitable.
    ତାର୍କିକ needDynamicTrigger(Ast_ଗଣ୍ଠି* ଅଚଳ nodep) ଅଚଳ {
        ଫେରନ୍ତୁ m_classp || m_underIfaceCFunc || nodep->exists([](Ast_ଗଣ୍ଠି* ଅଚଳ nodep) {
            ଯଦି (Ast_ଗଣ୍ଠି_Var_Ref* varp = VN_CAST(nodep, ଗଣ୍ଠି_Var_Ref)) {
                ଫେରନ୍ତୁ varp->varp()->isFuncLocal();
            }
            ଫେରନ୍ତୁ !nodep->isPure();
        });
    }
    // Creates a trigger scheduler variable
    AstVarScope* getCreateTriggerSchedulerp(Ast_Sen_ବୃକ୍ଷ* ଅଚଳ sentreep) {
        ଯଦି (!sentreep->user1p()) {
            ଯଦି (!m_trigSchedDtp) {
                m_trigSchedDtp
                    = ନୂତନ AstBasicDType{m_scopeTopp->fileline(), VBasicDTypeKwd::TRIGGER_SCHEDULER,
                                        VSigning::UNSIGNED};
                m_netlistp->typeTablep()->addTypesp(m_trigSchedDtp);
            }
            AstVarScope* ଅଚଳ trigSchedp
                = m_ଶୀର୍ଷ_Scopep->createTemp(m_trigSchedNames.get(sentreep), m_trigSchedDtp);
            sentreep->user1p(trigSchedp);
        }
        ଫେରନ୍ତୁ VN_AS(sentreep->user1p(), VarScope);
    }
    // Creates a string describing the sentree
    AstCExpr* createEventDescription(Ast_Sen_ବୃକ୍ଷ* ଅଚଳ sentreep) ଅଚଳ {
        ଯଦି (!sentreep->user2p()) {
            std::stringstream ss;
            V3EmitV::verilog_For_ବୃକ୍ଷ(sentreep, ss);
            // possibly a multiline string
            // Names were hashed by V3Name, show the user the original text
            std::string comment = VName::dehash(ss.str());
            std::replace(comment.begin(), comment.end(), '\n', ' ');
            // Escape quotes, e.g. from string-indexed event array selects
            comment = '"' + V3OutFormatter::quoteNameControls(comment) + '"';
            AstCExpr* ଅଚଳ commentp = ନୂତନ AstCExpr{sentreep->fileline(), comment};
            commentp->dtypeSetString();
            sentreep->user2p(commentp);
            ଫେରନ୍ତୁ commentp;
        }
        ଫେରନ୍ତୁ VN_AS(sentreep->user2p(), CExpr)->clone_ବୃକ୍ଷ(ମିଥ୍ୟା);
    }
    // Adds debug info to a hardcoded method call
    ଶୂନ୍ୟ addDebugInfo(AstCMethodHard* ଅଚଳ methodp) ଅଚଳ {
        File_ଧାଡ଼ି* ଅଚଳ flp = methodp->fileline();
        ଅଚଳ ତାର୍କିକ protectIds = v3Global.opt.protectIds();
        AstCExpr* ଅଚଳ ap
            = ନୂତନ AstCExpr{flp, protectIds ? "VL_UNKNOWN" : '"' + flp->filenameEsc() + '"'};
        ap->dtypeSetString();
        methodp->addPinsp(ap);
        AstCExpr* ଅଚଳ bp = ନୂତନ AstCExpr{flp, protectIds ? "0" : cvtToStr(flp->lineno())};
        bp->dtypeSetString();
        methodp->addPinsp(bp);
    }
    // Adds debug info to a trigSched.trigger() call
    ଶୂନ୍ୟ addEventDebugInfo(AstCMethodHard* ଅଚଳ methodp, Ast_Sen_ବୃକ୍ଷ* ଅଚଳ sentreep) ଅଚଳ {
        ଯଦି (v3Global.opt.protectIds()) {
            File_ଧାଡ଼ି* ଅଚଳ flp = sentreep->fileline();
            AstCExpr* ଅଚଳ descp = ନୂତନ AstCExpr{flp, "VL_UNKNOWN"};
            descp->dtypeSetString();
            methodp->addPinsp(descp);
            addDebugInfo(methodp);
            ଫେରନ୍ତୁ;
        }
        methodp->addPinsp(createEventDescription(sentreep));
        addDebugInfo(methodp);
    }
    // Adds process pointer to a hardcoded method call
    ଶୂନ୍ୟ addProcessInfo(AstCMethodHard* ଅଚଳ methodp) ଅଚଳ {
        File_ଧାଡ଼ି* ଅଚଳ flp = methodp->fileline();
        AstCExpr* ଅଚଳ ap
            = ନୂତନ AstCExpr{flp, (m_procp && m_hasProcess) ? "vlProcess" : "nullptr"};
        methodp->addPinsp(ap);
    }
    // Creates the fork handle type and returns it
    AstBasicDType* getCreateForkSyncDTypep() {
        ଯଦି (m_forkDtp) ଫେରନ୍ତୁ m_forkDtp;
        m_forkDtp = ନୂତନ AstBasicDType{m_scopeTopp->fileline(), VBasicDTypeKwd::FORK_SYNC,
                                      VSigning::UNSIGNED};
        m_netlistp->typeTablep()->addTypesp(m_forkDtp);
        ଫେରନ୍ତୁ m_forkDtp;
    }
    // Move `nodep` into `AstCLocalScope`.
    AstCLocalScope* addCLocalScope(File_ଧାଡ଼ି* ଅଚଳ flp, Ast_ଗଣ୍ଠି* ଅଚଳ nodep) ଅଚଳ {
        ଯଦି (!nodep) ଫେରନ୍ତୁ nullptr;
        VNRelinker handle;
        nodep->unlinkFrBack(&handle);
        AstCLocalScope* ଅଚଳ cscopep = ନୂତନ AstCLocalScope{flp, nodep};
        handle.relink(cscopep);
        ଫେରନ୍ତୁ cscopep;
    }
    // Create a temp variable and optionally put it before the specified node (mark local if so)
    AstVarScope* createTemp(File_ଧାଡ଼ି* ଅଚଳ flp, ଅଚଳ std::string& name,
                            Ast_ଗଣ୍ଠି_DType* ଅଚଳ dtypep, Ast_ଗଣ୍ଠି* ଅଚଳ ଭର୍ତ୍ତି_Beforep = nullptr) {
        AstVar* varp;
        ଯଦି (ଭର୍ତ୍ତି_Beforep) {
            varp = ନୂତନ AstVar{flp, VVarType::BLOCKTEMP, name, dtypep};
            varp->funcLocal(ସତ୍ୟ);
            ଭର୍ତ୍ତି_Beforep->addHereThisAsNext(varp);
        } ଅନ୍ୟଥା {
            varp = ନୂତନ AstVar{flp, VVarType::MODULETEMP, name, dtypep};
            m_scopep->modp()->addStmtsp(varp);
        }
        AstVarScope* ଅଚଳ vscp = ନୂତନ AstVarScope{flp, m_scopep, varp};
        m_scopep->addVarsp(vscp);
        ଫେରନ୍ତୁ vscp;
    }
    // Add a done() call on the fork sync
    ଶୂନ୍ୟ addForkDone(AstBegin* ଅଚଳ beginp, AstVarScope* ଅଚଳ forkVscp) ଅଚଳ {
        File_ଧାଡ଼ି* ଅଚଳ flp = beginp->fileline();
        auto* ଅଚଳ donep = ନୂତନ AstCMethodHard{
            beginp->fileline(), ନୂତନ AstVarRef{flp, forkVscp, VAccess::WRITE}, VCMethod::FORK_DONE};
        donep->dtypeSetVoid();
        addDebugInfo(donep);
        beginp->addStmtsp(donep->makeStmt());
    }
    ସ୍ଥିର ତାର୍କିକ has_Disable_Queue_ଠେଲ_Self_Prefix(AstBegin* ଅଚଳ beginp) {
        // LinkJump prepends disable-by-name registration as:
        //   __VprocessQueue_*.push_back(std::process::self())
        // V3LiftExpr lifts std::process::self() into an assignment to a temporary, then V3Task
        // lowers that assignment's function call into a leading comment and AstStmtExpr. Thus the
        // push_back is no longer necessarily the first statement. Scan across this generated
        // prefix for it, stopping at the fork-start sentinel or the branch body. Registrations for
        // nested forks live in sub-blocks and so are not in this statement list.
        ପାଇଁ (Ast_ଗଣ୍ଠି* stmtp = beginp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ଯଦି (VN_IS(stmtp, Comment)) ଜାରି;
            Ast_Stmt_ପଦସମଷ୍ଟି* ଅଚଳ stmtExprp = VN_CAST(stmtp, Stmt_ପଦସମଷ୍ଟି);
            ଯଦି (!stmtExprp) ଭାଙ୍ଗ;
            ଯଦି (stmtExprp->is_Disable_Queue_ଠେଲ_Self_Stmt()) ଫେରନ୍ତୁ ସତ୍ୟ;
        }
        ଫେରନ୍ତୁ ମିଥ୍ୟା;
    }
    // Register a callback so killing a process-backed fork branch decrements the join counter
    ଶୂନ୍ୟ addForkOnKill(AstBegin* ଅଚଳ beginp, AstVarScope* ଅଚଳ forkVscp) ଅଚଳ {
        ଯଦି (!beginp->needProcess() && !has_Disable_Queue_ଠେଲ_Self_Prefix(beginp)) ଫେରନ୍ତୁ;
        File_ଧାଡ଼ି* ଅଚଳ flp = beginp->fileline();
        AstCMethodHard* ଅଚଳ onKillp = ନୂତନ AstCMethodHard{
            flp, ନୂତନ AstVarRef{flp, forkVscp, VAccess::WRITE}, VCMethod::FORK_ON_KILL};
        onKillp->dtypeSetVoid();
        AstCExpr* ଅଚଳ processp = ନୂତନ AstCExpr{flp, "vlProcess"};
        processp
            ->dtypeSetVoid();  // Opaque process reference; type is irrelevant for hardcoded emit
        onKillp->addPinsp(processp);
        Ast_ଗଣ୍ଠି_Stmt* ଅଚଳ stmtp = onKillp->makeStmt();
        beginp->prependStmtsp(stmtp);
    }
    // Handle the 'join' part of a fork..join
    ଶୂନ୍ୟ makeForkJoin(AstFork* ଅଚଳ forkp) {
        // Create a fork sync var
        File_ଧାଡ଼ି* ଅଚଳ flp = forkp->fileline();
        // Insert the sync var directly before the fork
        Ast_ଗଣ୍ଠି* ଅଚଳ ଭର୍ତ୍ତି_Beforep = forkp;
        // Make sure all references to the fork var are contained within a single scope to prevent
        // moving statements accessing it in case of large functions splitting.
        addCLocalScope(flp, ଭର୍ତ୍ତି_Beforep);
        AstVarScope* forkVscp
            = createTemp(flp, forkp->name() + "__sync", getCreateForkSyncDTypep(), ଭର୍ତ୍ତି_Beforep);
        unsigned join_ଗଣନା = 0;  // Needed for join counter
        // Add a <fork sync>.done() to each begin
        ପାଇଁ (Ast_ଗଣ୍ଠି* beginp = forkp->forksp(); beginp; beginp = beginp->nextp()) {
            addForkOnKill(VN_AS(beginp, Begin), forkVscp);
            addForkDone(VN_AS(beginp, Begin), forkVscp);
            join_ଗଣନା++;
        }
        ଯଦି (forkp->joinType().joinAny()) join_ଗଣନା = 1;
        // Set the join counter
        auto* ଅଚଳ initp = ନୂତନ AstCMethodHard{flp, ନୂତନ AstVarRef{flp, forkVscp, VAccess::WRITE},
                                               VCMethod::FORK_INIT, ନୂତନ AstConst{flp, join_ଗଣନା}};
        initp->dtypeSetVoid();
        addProcessInfo(initp);
        forkp->addHereThisAsNext(initp->makeStmt());
        // Await the join at the end
        auto* ଅଚଳ joinp = ନୂତନ AstCMethodHard{flp, ନୂତନ AstVarRef{flp, forkVscp, VAccess::WRITE},
                                               VCMethod::FORK_JOIN};
        joinp->dtypeSetVoid();
        addProcessInfo(joinp);
        addDebugInfo(joinp);
        forkp->addNextHere(ନୂତନ AstCAwait{flp, joinp});
    }

    // `procp` shall be a NodeProcedure/CFunc/Begin and within it vars from `varsp` will be placed.
    // `varsp` vector of vars which shall be localized.
    ସ୍ଥିର ଶୂନ୍ୟ localizeVars(Ast_ଗଣ୍ଠି* ଅଚଳ procp, ଅଚଳ std::vector<AstVar*>& varsp) {
        UASSERT(procp, "procp is nullptr");
        AstVar* newVarsp = nullptr;
        ପାଇଁ (AstVar* ଅଚଳ varp : varsp) {
            varp->funcLocal(ସତ୍ୟ);
            newVarsp = Ast_ଗଣ୍ଠି::addNextNull(newVarsp, varp->unlinkFrBack());
        }
        ଯଦି (Ast_ଗଣ୍ଠି_Procedure* ଅଚଳ ଗଣ୍ଠି_Procp = VN_CAST(procp, ଗଣ୍ଠି_Procedure)) {
            ଗଣ୍ଠି_Procp->prependStmtsp(newVarsp);
        } ଅନ୍ୟଥା ଯଦି (AstCFunc* ଅଚଳ cfuncp = VN_CAST(procp, CFunc)) {
            cfuncp->prependVarsp(newVarsp);
        } ଅନ୍ୟଥା ଯଦି (AstBegin* ଅଚଳ beginp = VN_CAST(procp, Begin)) {
            beginp->prependStmtsp(newVarsp);
        } ଅନ୍ୟଥା {
            procp->v3fatalSrc(
                procp->prettyNameQ()
                << " is not of an expected type NodeProcedure/CFunc/Begin instead it is: "
                << procp->prettyTypeName());
        }
    }

    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* delayToIntegral(AstDelay* nodep, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* valuep) {
        ଅଚଳ AstConst* ଅଚଳ constp = VN_CAST(valuep, Const);
        // The fork start sentinel is already in ticks
        ଯଦି (constp && constp->toUQuad() == std::numeric_limits<uint64_t>::max()) ଫେରନ୍ତୁ valuep;
        ଫେରନ୍ତୁ V3Timing::delayToTicks(valuep, nodep->timeunit());
    }

    // VISITORS
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Module* nodep) override {
        UASSERT_OBJ(!m_classp, nodep, "Module or class under class");
        VL_RESTORER(m_classp);
        m_classp = VN_CAST(nodep, Class);
        VL_RESTORER(m_forkCnt);
        m_forkCnt = 0;
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(AstScope* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        Sen_ପଦସମଷ୍ଟି_Builder sen_ପଦସମଷ୍ଟି_Builder{m_scopep};
        {  // Restore m_senExprBuilderp before destroying senExprBuilder
            VL_RESTORER(m_sen_ପଦସମଷ୍ଟି_Builderp);
            m_sen_ପଦସମଷ୍ଟି_Builderp = &sen_ପଦସମଷ୍ଟି_Builder;
            iterateChildren(nodep);
        }
    }
    ଶୂନ୍ୟ visit(AstActive* nodep) override {
        VL_RESTORER(m_activep);
        m_activep = nodep;
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Procedure* nodep) override {
        VL_RESTORER(m_procp);
        VL_RESTORER(m_hasProcess);
        m_procp = nodep;
        m_hasProcess = hasFlags(nodep, T_HAS_PROC);
        VL_RESTORER(m_underProcedure);
        m_underProcedure = ସତ୍ୟ;
        iterateChildren(nodep);
        ଯଦି (hasFlags(nodep, T_SUSPENDEE)) nodep->setSuspendable();
        ଯଦି (hasFlags(nodep, T_HAS_PROC)) nodep->setNeedProcess();
    }
    ଶୂନ୍ୟ visit(AstInitial* nodep) override {
        visit(static_cast<Ast_ଗଣ୍ଠି_Procedure*>(nodep));
        ଯଦି (nodep->needProcess() && !nodep->user1SetOnce()) {
            nodep->addStmtsp(
                ନୂତନ AstCStmt{nodep->fileline(), "vlProcess->state(VlProcess::FINISHED);"});
        }
    }
    ଶୂନ୍ୟ visit(AstJumpBlock* nodep) override {
        VL_RESTORER(m_underJumpBlock);
        m_underJumpBlock = ସତ୍ୟ;
        visit(static_cast<Ast_ଗଣ୍ଠି_Stmt*>(nodep));
    }
    ଶୂନ୍ୟ visit(AstAlways* nodep) override {
        ଯଦି (nodep->user1SetOnce()) ଫେରନ୍ତୁ;
        VL_RESTORER(m_procp);
        VL_RESTORER(m_hasProcess);
        m_procp = nodep;
        m_hasProcess = hasFlags(nodep, T_HAS_PROC);
        VL_RESTORER(m_underProcedure);
        m_underProcedure = ସତ୍ୟ;
        // Workaround for killing `always` processes (doing that is pretty much UB)
        // TODO: Disallow killing `always` at runtime (throw an error)
        // Skip for combinational blocks; if the body has timing controls
        // iterateChildren will add T_SUSPENDEE, otherwise it would spin.
        ଯଦି (hasFlags(nodep, T_HAS_PROC)
            && !(m_activep && m_activep->sentreep() && m_activep->sentreep()->hasCombo()))
            addFlags(nodep, T_SUSPENDEE);

        iterateChildren(nodep);
        ଯଦି (hasFlags(nodep, T_HAS_PROC)) nodep->setNeedProcess();
        ଯଦି (!hasFlags(nodep, T_SUSPENDEE)) ଫେରନ୍ତୁ;
        nodep->setSuspendable();
        File_ଧାଡ଼ି* ଅଚଳ flp = nodep->fileline();
        Ast_Sen_ବୃକ୍ଷ* ଅଚଳ sentreep = m_activep->sentreep();
        ଯଦି (sentreep->hasClocked()) {
            Ast_ଗଣ୍ଠି* ଅଚଳ bodysp = nodep->stmtsp()->unlinkFrBackWithNext();
            auto* ଅଚଳ controlp = ନୂତନ AstEventControl{flp, sentreep->clone_ବୃକ୍ଷ(ମିଥ୍ୟା), bodysp};
            nodep->addStmtsp(controlp);
            iterate(controlp);
        }
        // Note: The 'while (true)' outer loop will be added in V3Sched
        auto* ଅଚଳ activep = ନୂତନ AstActive{
            flp, "", ନୂତନ Ast_Sen_ବୃକ୍ଷ{flp, ନୂତନ AstSenItem{flp, AstSenItem::Initial{}}}};
        activep->sen_ବୃକ୍ଷ_Storep(activep->sentreep());
        activep->addStmtsp(nodep->unlinkFrBack());
        m_activep->addNextHere(activep);
    }
    ଶୂନ୍ୟ visit(AstCFunc* nodep) override {
        VL_RESTORER(m_procp);
        VL_RESTORER(m_hasProcess);
        VL_RESTORER(m_underIfaceCFunc);
        m_procp = nodep;
        m_hasProcess = hasFlags(nodep, T_HAS_PROC);
        m_underIfaceCFunc = VN_IS(m_scopep->modp(), Iface);
        iterateChildren(nodep);
        ଯଦି (hasFlags(nodep, T_HAS_PROC)) nodep->setNeedProcess();
        ଯଦି (!(hasFlags(nodep, T_SUSPENDEE))) ଫେରନ୍ତୁ;

        nodep->rtnType("VlCoroutine");
        // If in a class, create a shared pointer to 'this'
        ଯଦି (m_classp) {
            AstCStmt* ଅଚଳ cstmtp = ନୂତନ AstCStmt{nodep->fileline(), "VL_KEEP_THIS;"};
            nodep->prependStmtsp(cstmtp);
        }
        ଯଦି (nodep->dpiExportImpl()) {
            nodep->exists([](AstCAwait* ଅଚଳ awaitp) -> ତାର୍କିକ {
                // A DPI-exported coroutine won't be able to block the calling code
                // Error on the await node; fall back to the function node
                awaitp->v3warn(E_UNSUPPORTED,
                               "Unsupported: Timing controls inside DPI-exported tasks");
                ଫେରନ୍ତୁ ସତ୍ୟ;
            });
        }
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_CCall* nodep) override {
        AstCFunc* ଅଚଳ funcp = nodep->funcp();

        // Skip automatic covergroup sampling calls
        ଯଦି (funcp->isCovergroupSample()) {
            iterateChildren(nodep);
            ଫେରନ୍ତୁ;
        }

        ଯଦି (funcp->needProcess()) m_hasProcess = ସତ୍ୟ;
        ଯଦି (hasFlags(funcp, T_SUSPENDEE) && !nodep->user1SetOnce()) {  // If suspendable
            // Calls to suspendables are always void return type, hence parent must be StmtExpr
            Ast_Stmt_ପଦସମଷ୍ଟି* ଅଚଳ stmtp = VN_AS(nodep->backp(), Stmt_ପଦସମଷ୍ଟି);
            stmtp->replaceWith(ନୂତନ AstCAwait{nodep->fileline(), nodep->unlinkFrBack()});
            VL_DO_DANGLING(ଠେଲ_Deletep(stmtp), stmtp);
        }
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(AstDelay* nodep) override {
        UASSERT_OBJ(!nodep->isCycleDelay(), nodep,
                    "Cycle delays should have been handled in V3AssertPre");
        File_ଧାଡ଼ି* ଅଚଳ flp = nodep->fileline();

        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* valuep = delayToIntegral(nodep, nodep->lhsp()->unlinkFrBack());

        // Statistics
        ଯଦି (valuep->isZero()) {
            ++m_statZeroDelays;
        } ଅନ୍ୟଥା ଯଦି (VN_IS(valuep, Const)) {
            ++m_statConstDelays;
        } ଅନ୍ୟଥା {
            ++m_statVariableDelays;
        }

        // Decide scheduling support for #0
        ଯଦି (v3Global.opt.schedZeroDelay().isSetTrue()) {
            // User said to schedule for #0 support, nothing else to do
            v3Global.setUsesZeroDelay();
        } ଅନ୍ୟଥା ଯଦି (v3Global.opt.schedZeroDelay().isSetFalse()) {
            // User said to schedule without #0 support. Still warn if a static #0 delay exists
            ଯଦି (valuep->isZero()) {
                nodep->v3warn(
                    ZERODLY,
                    "Static #0 delay exists, but '--no-sched-zero-delay' was given.\n"
                        << nodep->warnMore()  //
                        << "... Can proceed, but this will fail at runtime if executed.");
            }
        } ଅନ୍ୟଥା {
            // User did not express preference, decide based on presence of delays
            ଯଦି (valuep->isZero()) {
                // Statically known #0 delay exists, schedule for #0 support
                v3Global.setUsesZeroDelay();
                m_hasStaticZeroDelay = ସତ୍ୟ;
                // Don't warn on variable delays, as no point
                m_unknownDelayFlps.clear();
            } ଅନ୍ୟଥା ଯଦି (staticallyNonZeroDelay(valuep)) {
                // Delay is dynamic, but every statically known outcome is non-zero.
                // So we don't need #0 delay support and there should be no warning.
            } ଅନ୍ୟଥା ଯଦି (!VN_IS(valuep, Const)) {
                // Delay is not known at compiile time. Conservatively schedule for #0 support,
                // but warn if no static #0 delays used as performance might be improved
                v3Global.setUsesZeroDelay();
                ଯଦି (!m_hasStaticZeroDelay) m_unknownDelayFlps.ଠେଲ_back(nodep->fileline());
            }
        }

        // Replace self with a 'co_await dlySched.delay(<valuep>)'
        AstCMethodHard* ଅଚଳ delayMethodp = ନୂତନ AstCMethodHard{
            flp, ନୂତନ AstVarRef{flp, getCreateDelayScheduler(), VAccess::WRITE},
            VCMethod::SCHED_DELAY, valuep};
        delayMethodp->dtypeSetVoid();
        addProcessInfo(delayMethodp);
        addDebugInfo(delayMethodp);
        // Create the co_await
        Ast_ଗଣ୍ଠି* ଅଚଳ awaitp = ନୂତନ AstCAwait{flp, delayMethodp, get_Create_Delay_Sen_ବୃକ୍ଷ()};
        // Relink child statements after the co_await
        ଯଦି (Ast_ଗଣ୍ଠି* ଅଚଳ stmtsp = nodep->stmtsp()) {
            awaitp->addNext(stmtsp->unlinkFrBackWithNext());
        }
        nodep->replaceWith(awaitp);
        VL_DO_DANGLING(nodep->delete_ବୃକ୍ଷ(), nodep);
    }
    ଶୂନ୍ୟ visit(AstEventControl* nodep) override {
        // Do not allow waiting on local named events, as they get enqueued for clearing, but can
        // go out of scope before that happens
        ଯଦି (!nodep->sentreep()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Event control with implicit sensitivity (@*)"
                          " in this context; use an explicit sensitivity list instead");
            VL_DO_DANGLING(ଠେଲ_Deletep(nodep->unlinkFrBack()), nodep);
            ଫେରନ୍ତୁ;
        }
        File_ଧାଡ଼ି* ଅଚଳ flp = nodep->fileline();
        // Relink child statements after the event control
        ଯଦି (nodep->stmtsp()) nodep->addNextHere(nodep->stmtsp()->unlinkFrBackWithNext());
        ଯଦି (needDynamicTrigger(nodep->sentreep())) {
            // Create the trigger variable and init it with 0
            AstVarScope* ଅଚଳ trigvscp
                = createTemp(flp, m_dynTrigNames.get(nodep), nodep->findBitDType(), nodep);
            auto* ଅଚଳ initp = ନୂତନ AstAssign{flp, ନୂତନ AstVarRef{flp, trigvscp, VAccess::WRITE},
                                              ନୂତନ AstConst{flp, AstConst::BitFalse{}}};
            // Await the eval step with the dynamic trigger scheduler. First, create the method
            // call
            auto* ଅଚଳ evalMethodp = ନୂତନ AstCMethodHard{
                flp, ନୂତନ AstVarRef{flp, getCreateDynamicTriggerScheduler(), VAccess::WRITE},
                VCMethod::SCHED_EVALUATION};
            evalMethodp->dtypeSetVoid();
            addProcessInfo(evalMethodp);
            auto* ଅଚଳ sentreep = nodep->sentreep();
            addEventDebugInfo(evalMethodp, sentreep);
            // Create the co_await
            AstCAwait* ଅଚଳ awaitEvalp
                = ନୂତନ AstCAwait{flp, evalMethodp, get_Create_Dynamic_Trigger_Sen_ବୃକ୍ଷ()};
            // Construct the sen expression for this sentree
            UASSERT_OBJ(m_sen_ପଦସମଷ୍ଟି_Builderp, nodep, "No SenExprBuilder for this scope");
            auto* ଅଚଳ assignp = ନୂତନ AstAssign{flp, ନୂତନ AstVarRef{flp, trigvscp, VAccess::WRITE},
                                                m_sen_ପଦସମଷ୍ଟି_Builderp->build(sentreep).first};
            // Get the SenExprBuilder results
            ଅଚଳ Sen_ପଦସମଷ୍ଟି_Builder::Results senResults = m_sen_ପଦସମଷ୍ଟି_Builderp->getAndClearResults();
            localizeVars(m_procp, senResults.m_vars);
            // If post updates are destructive (e.g. clearFired on events), perform a
            // conservative pre-clear once before entering the wait loop so stale state from a
            // previous wait does not cause an immediate false-positive trigger.
            ଅଚଳ ତାର୍କିକ hasDestructivePostUpdates = !senResults.m_destructivePostUpdates.empty();
            // Create the trigger eval loop, which will await the evaluation step and check the
            // trigger
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ condp
                = ନୂତନ AstLogNot{flp, ନୂତନ AstVarRef{flp, trigvscp, VAccess::READ}};
            AstLoop* ଅଚଳ loopp = ନୂତନ AstLoop{flp};
            loopp->addStmtsp(ନୂତନ AstLoopTest{flp, loopp, condp});
            loopp->addStmtsp(awaitEvalp);
            // Put pre updates before the trigger check and assignment
            ପାଇଁ (Ast_ଗଣ୍ଠି_Stmt* ଅଚଳ stmtp : senResults.m_preUpdates) loopp->addStmtsp(stmtp);
            // Then the trigger check and assignment
            loopp->addStmtsp(assignp);
            // Let the dynamic trigger scheduler know if this trigger was set
            // If it was, a call to the scheduler's evaluate() will return true
            AstCMethodHard* ଅଚଳ anyTriggeredMethodp = ନୂତନ AstCMethodHard{
                flp, ନୂତନ AstVarRef{flp, getCreateDynamicTriggerScheduler(), VAccess::WRITE},
                VCMethod::SCHED_ANY_TRIGGERED, ନୂତନ AstVarRef{flp, trigvscp, VAccess::READ}};
            anyTriggeredMethodp->dtypeSetVoid();
            loopp->addStmtsp(anyTriggeredMethodp->makeStmt());
            // If the post update is destructive (e.g. event vars are cleared), create an await for
            // the post update step
            ଯଦି (hasDestructivePostUpdates) {
                AstCAwait* ଅଚଳ awaitPostUpdatep = awaitEvalp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା);
                VN_AS(awaitPostUpdatep->exprp(), CMethodHard)->method(VCMethod::SCHED_POST_UPDATE);
                loopp->addStmtsp(awaitPostUpdatep);
            }
            // Put the post updates at the end of the loop
            ପାଇଁ (Ast_ଗଣ୍ଠି_Stmt* ଅଚଳ stmtp : senResults.m_postUpdates) loopp->addStmtsp(stmtp);
            // Finally, await the resumption step in 'act'
            AstCAwait* ଅଚଳ awaitResumep = awaitEvalp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା);
            VN_AS(awaitResumep->exprp(), CMethodHard)->method(VCMethod::SCHED_RESUMPTION);
            Ast_ଗଣ୍ଠି::addNext<Ast_ଗଣ୍ଠି_Stmt, Ast_ଗଣ୍ଠି_Stmt>(loopp, awaitResumep);
            // Replace the event control with one explicit stmt chain:
            //   init -> [inits] -> [optional destructive pre-clears] -> loop -> awaitResumption
            Ast_ଗଣ୍ଠି_Stmt* chainp = nullptr;
            chainp = Ast_ଗଣ୍ଠି::addNextNull(chainp, initp);
            ପାଇଁ (Ast_ଗଣ୍ଠି_Stmt* ଅଚଳ stmtp : senResults.m_inits) {
                chainp = Ast_ଗଣ୍ଠି::addNextNull(chainp, stmtp);
            }
            ଯଦି (hasDestructivePostUpdates) {
                ପାଇଁ (Ast_ଗଣ୍ଠି_Stmt* ଅଚଳ stmtp : senResults.m_destructivePostUpdates) {
                    chainp = Ast_ଗଣ୍ଠି::addNextNull(chainp, stmtp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା));
                }
            }
            chainp = Ast_ଗଣ୍ଠି::addNextNull(chainp, loopp);
            nodep->replaceWith(chainp);
        } ଅନ୍ୟଥା {
            auto* ଅଚଳ sentreep = m_finder.get_Sen_ବୃକ୍ଷ(nodep->sentreep());
            nodep->sentreep()->unlinkFrBack()->delete_ବୃକ୍ଷ();
            // Get this sentree's trigger scheduler
            // Replace self with a 'co_await trigSched.trigger()'
            auto* ଅଚଳ triggerMethodp = ନୂତନ AstCMethodHard{
                flp, ନୂତନ AstVarRef{flp, getCreateTriggerSchedulerp(sentreep), VAccess::WRITE},
                VCMethod::SCHED_TRIGGER};
            triggerMethodp->dtypeSetVoid();
            // If it should be committed immediately, pass true, otherwise false
            triggerMethodp->addPinsp(nodep->user2() ? ନୂତନ AstConst{flp, AstConst::BitTrue{}}
                                                    : ନୂତନ AstConst{flp, AstConst::BitFalse{}});
            addProcessInfo(triggerMethodp);
            addEventDebugInfo(triggerMethodp, sentreep);
            // Create the co_await
            AstCAwait* ଅଚଳ awaitp = ନୂତନ AstCAwait{flp, triggerMethodp, sentreep};
            nodep->replaceWith(awaitp);
        }
        VL_DO_DANGLING(nodep->delete_ବୃକ୍ଷ(), nodep);
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Assign* nodep) override {
        // Only process once to avoid infinite loops (due to the net delay)
        ଯଦି (nodep->user1SetOnce()) ଫେରନ୍ତୁ;
        UINFO(9, "control-visit " << nodep);
        File_ଧାଡ଼ି* ଅଚଳ flp = nodep->fileline();
        Ast_ଗଣ୍ଠି* controlp = factorOutTimingControl(nodep);
        ଅଚଳ ତାର୍କିକ inAssignDly = VN_IS(nodep, AssignDly);
        // Handle the intra assignment timing control
        // Transform if:
        // * there's a timing control in the assignment
        // * the assignment is an AssignDly and it's in a non-inlined function
        ଯଦି (!controlp && (!inAssignDly || m_underProcedure)) {
            iterateChildren(nodep);
            ଫେରନ୍ତୁ;
        }
        // The timing control, if any, which is put in a fork for an NBA
        Ast_ଗଣ୍ଠି* ଅଚଳ timingp = controlp;
        // Insert new vars before the timing control if we're in a function; in a process we can't
        // do that, as after V3Begin only modules and functions declare variables. These
        // intra-assignment vars will later be passed to forked processes by value.
        Ast_ଗଣ୍ଠି* ଭର୍ତ୍ତି_Beforep = m_underProcedure ? nullptr : controlp;
        // Special case for NBA
        ଯଦି (inAssignDly) {
            // Put it in a fork so it doesn't block
            // Could already be the only thing directly under a fork, reuse that if possible
            AstFork* forkp = !nodep->nextp() ? VN_CAST(nodep->firstAbovep(), Fork) : nullptr;
            ଯଦି (!forkp) forkp = ନୂତନ AstFork{flp, VJoinType::JOIN_NONE};
            ଯଦି (!m_underProcedure) {
                // If it's in a function, it won't be handled by V3Delayed
                // Put it behind an additional named event that gets triggered in the NBA region
                AstEventControl* ଅଚଳ nbaEventControlp = createNbaEventControl(flp);
                AstAssign* ଅଚଳ trigAssignp = createNbaEventTriggerAssignment(flp);
                nodep->replaceWith(trigAssignp);
                trigAssignp->addNextHere(nbaEventControlp);
                nbaEventControlp->addStmtsp(nodep);
                ଭର୍ତ୍ତି_Beforep = forkp;
                ଯଦି (!controlp) controlp = nbaEventControlp;
            }
            controlp->replaceWith(forkp);
            AstBegin* beginp = VN_CAST(controlp, Begin);
            ଯଦି (!beginp) beginp = ନୂତନ AstBegin{nodep->fileline(), "", controlp, ମିଥ୍ୟା};
            forkp->addForksp(beginp);
            addFlags(beginp, T_NBA_UPDATE);
            controlp = forkp;
        }
        UASSERT_OBJ(nodep, controlp, "Assignment should have timing control");
        // Move `insertBeforep` into `AstCLocalScope` if necessary to avoid jumping over
        // a variable initialization that would be inserted before `insertBeforep`. All
        // access to this variable should be contained within returned `AstCLocalScope`.
        ଯଦି (m_underJumpBlock) addCLocalScope(flp, ଭର୍ତ୍ତି_Beforep);
        // Function for replacing values with intermediate variables
        ଅଚଳ auto replaceWithIntermediate = [&](Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ valuep,
                                                 ଅଚଳ std::string& name) {
            AstVarScope* ଅଚଳ newvscp = createTemp(flp, name, valuep->dtypep(), ଭର୍ତ୍ତି_Beforep);
            // A pending update keeps its own value, even if its process schedules more
            ଯଦି (inAssignDly && !ଭର୍ତ୍ତି_Beforep) newvscp->fork_By_ମୂଲ୍ୟ(ସତ୍ୟ);
            valuep->replaceWith(ନୂତନ AstVarRef{flp, newvscp, VAccess::READ});
            controlp->addHereThisAsNext(
                ନୂତନ AstAssign{flp, ନୂତନ AstVarRef{flp, newvscp, VAccess::WRITE}, valuep});
        };
        // NBAs with delays evaluate the indices selecting the target now (IEEE 1800-2023 10.4.2),
        // in order, each as a whole, also the selects and function calls in it. So follow the
        // selects of the target up from the variable, not into their indices. As an example, this
        // transforms 'x[a[i]][f(j)] <= #1 y' into 't0 = a[i]; t1 = f(j); x[t0][t1] <= #1 y'.
        // V3Const has split NBAs to concatenations, also streamed, into NBAs to each part.
        ଯଦି (inAssignDly) {
            ପାଇଁ (Ast_ଗଣ୍ଠି* exprp = nodep->lhsp()->baseFromp(ସତ୍ୟ); exprp != nodep;
                 exprp = exprp->firstAbovep()) {
                ଯଦି (AstSel* ଅଚଳ selp = VN_CAST(exprp, Sel)) {
                    // widthp should be const
                    ଯଦି (!VN_IS(selp->lsbp(), Const)) {
                        replaceWithIntermediate(selp->lsbp(), m_intraLsbNames.get(nodep));
                    }
                } ଅନ୍ୟଥା ଯଦି (Ast_ଗଣ୍ଠି_Sel* ଅଚଳ selp = VN_CAST(exprp, ଗଣ୍ଠି_Sel)) {
                    ଯଦି (!VN_IS(selp->bitp(), Const)) {
                        replaceWithIntermediate(selp->bitp(), m_intraIndexNames.get(nodep));
                    }
                }
            }
        }
        // Replace the RHS with an intermediate value var
        replaceWithIntermediate(nodep->rhsp(), m_intra_ମୂଲ୍ୟ_Names.get(nodep));
        ଯଦି (!inAssignDly) ଫେରନ୍ତୁ;
        // The process executing the NBA evaluates its delay (IEEE 1800-2023 4.9.4), not the forked
        // process, if it has side effects, e.g. calling a function that checks its process. A pure
        // one is evaluated as the forked process starts, at once.
        ଯଦି (AstDelay* ଅଚଳ delayp = VN_CAST(timingp, Delay)) {
            ଯଦି (!delayp->lhsp()->isPure()) {
                replaceWithIntermediate(delayp->lhsp(), m_intra_ମୂଲ୍ୟ_Names.get(nodep));
            }
        }
    }
    ଶୂନ୍ୟ visit(AstFireEvent* nodep) override {
        // V3Delayed handles '->>' of a variable in a process. Like NBAs in non-inlined functions,
        // trigger other events from a fork awaiting the NBA region.
        ଯଦି (!nodep->isDelayed() || (m_underProcedure && VN_IS(nodep->operandp(), VarRef))) {
            iterateChildren(nodep);
            ଫେରନ୍ତୁ;
        }
        File_ଧାଡ଼ି* ଅଚଳ flp = nodep->fileline();
        AstAssign* ଅଚଳ trigAssignp = createNbaEventTriggerAssignment(flp);
        nodep->replaceWith(trigAssignp);
        AstFork* ଅଚଳ forkp = ନୂତନ AstFork{flp, VJoinType::JOIN_NONE};
        trigAssignp->addNextHere(forkp);
        ଯଦି (m_underJumpBlock) addCLocalScope(flp, forkp);
        // The triggered event is the one referenced now, so evaluate handles and indices now
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ eventp = nodep->operandp()->unlinkFrBack();
        ଅଚଳ auto evalNow = [&](Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ valuep, V3UniqueNames& names) {
            // Like for an NBA, a process uses a module variable, passed to the fork by value
            AstVarScope* ଅଚଳ vscp = createTemp(flp, names.get(nodep), valuep->dtypep(),
                                                 m_underProcedure ? nullptr : forkp);
            ଯଦି (m_underProcedure) vscp->fork_By_ମୂଲ୍ୟ(ସତ୍ୟ);
            valuep->replaceWith(ନୂତନ AstVarRef{flp, vscp, VAccess::READ});
            // Unlike an event, a handle selecting one is only read
            valuep->foreach([](Ast_ଗଣ୍ଠି* ଅଚଳ np) {
                ଯଦି (Ast_ଗଣ୍ଠି_Var_Ref* ଅଚଳ refp = VN_CAST(np, ଗଣ୍ଠି_Var_Ref)) {
                    refp->access(VAccess::READ);
                } ଅନ୍ୟଥା ଯଦି (AstMemberSel* ଅଚଳ selp = VN_CAST(np, MemberSel)) {
                    selp->access(VAccess::READ);
                }
            });
            forkp->addHereThisAsNext(
                ନୂତନ AstAssign{flp, ନୂତନ AstVarRef{flp, vscp, VAccess::WRITE}, valuep});
        };
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* refp = eventp;
        ଯେତେବେଳେ (Ast_ଗଣ୍ଠି_Sel* ଅଚଳ selp = VN_CAST(refp, ଗଣ୍ଠି_Sel)) {
            ଯଦି (!VN_IS(selp->bitp(), Const)) evalNow(selp->bitp(), m_intraIndexNames);
            refp = selp->fromp();
        }
        // Handle of a class (or virtual interface)
        ଯଦି (AstMemberSel* ଅଚଳ selp = VN_CAST(refp, MemberSel)) {
            evalNow(selp->fromp(), m_intra_ମୂଲ୍ୟ_Names);
        }
        AstEventControl* ଅଚଳ controlp = createNbaEventControl(flp);
        controlp->addStmtsp(ନୂତନ AstFireEvent{flp, eventp, ମିଥ୍ୟା});
        AstBegin* ଅଚଳ beginp = ନୂତନ AstBegin{flp, "", controlp, ମିଥ୍ୟା};
        addFlags(beginp, T_NBA_UPDATE);
        forkp->addForksp(beginp);
        VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
    }
    ଶୂନ୍ୟ visit(AstAssignW* nodep) override {
        File_ଧାଡ଼ି* ଅଚଳ flp = nodep->fileline();
        // Get the net delay unless this assignment was created for handling the net delay (user1)
        AstDelay* ଅଚଳ netDelayp = nodep->user1() ? nullptr : nodep->getLhsNetDelay();
        ଯଦି (netDelayp) {
            ଯଦି (nodep->timingControlp()) {
                // If this assignment has a delay, create another one to handle the net delay
                AstVarScope* ଅଚଳ newvscp = m_assignWTmps.make(flp, m_scopep, nodep->dtypep());
                AstAssignW* assignp = ନୂତନ AstAssignW{
                    nodep->fileline(), nodep->lhsp()->unlinkFrBack(),
                    ନୂତନ AstVarRef{flp, newvscp, VAccess::READ}, netDelayp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା)};
                assignp->user1(ସତ୍ୟ);
                nodep->addNextHere(assignp);
                nodep->lhsp(ନୂତନ AstVarRef{flp, newvscp, VAccess::WRITE});
            } ଅନ୍ୟଥା {
                // Else just use this one with the net delay
                nodep->timingControlp(netDelayp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା));
            }
        }
        ଯଦି (!nodep->timingControlp()) ଫେରନ୍ତୁ;
        // There will be some circular logic here, suppress the warning for newly created vars
        flp->modifyWarnOff(V3ErrorCode::UNOPTFLAT, ସତ୍ୟ);
        // Also suppress the warning for the LHS var, for cases like `assign #1 clk = ~clk;`
        // TODO: Restore the warning for other, non-delayed drivers
        nodep->lhsp()->foreach([](AstVarRef* refp) {
            ଯଦି (refp->access().isWriteOrRW()) {
                refp->varp()->fileline()->modifyWarnOff(V3ErrorCode::UNOPTFLAT, ସତ୍ୟ);
            }
        });
        // Should be under an always
        AstAlways* ଅଚଳ alwaysp = VN_AS(m_procp, Always);
        // Convert it to an Assign; the new assign with intra delay will be handled by
        // visit(AstNodeAssign*)
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ lhs1p = nodep->lhsp()->unlinkFrBack();
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ rhs1p = nodep->rhsp()->unlinkFrBack();
        Ast_ଗଣ୍ଠି* ଅଚଳ controlp = nodep->timingControlp()->unlinkFrBack();
        ଯଦି (AstDelay* ଅଚଳ delayp = VN_CAST(controlp, Delay)) {
            ଯଦି (Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* fallDelayp = delayp->fallDelay()) {
                fallDelayp = fallDelayp->unlinkFrBack();
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp = delayp->lhsp()->unlinkFrBack();
                // Use fall only for an all-zero value, rise otherwise.
                lhsp = delayToIntegral(delayp, lhsp);
                fallDelayp = delayToIntegral(delayp, fallDelayp);
                delayp->lhsp(
                    ନୂତନ AstCond{flp, ନୂତନ AstEq{flp, rhs1p->clone_ବୃକ୍ଷ(ମିଥ୍ୟା), ନୂତନ AstConst{flp, 0}},
                                fallDelayp, lhsp});
            }
        }
        AstAssign* ଅଚଳ assignp = ନୂତନ AstAssign{nodep->fileline(), lhs1p, rhs1p, controlp};
        // Put the assignment in a fork..join_none.
        AstFork* ଅଚଳ forkp = ନୂତନ AstFork{flp, VJoinType::JOIN_NONE};
        nodep->replaceWith(forkp);
        VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
        AstBegin* ଅଚଳ beginp = ନୂତନ AstBegin{flp, "", assignp, ମିଥ୍ୟା};
        forkp->addForksp(beginp);
        visit(forkp);  // Visit now as we need to do some post-processing
        // IEEE 1800-2023 10.3.3 - if the RHS value differs from the currently scheduled value to
        // be assigned, the currently scheduled assignment is descheduled. To keep track if an
        // assignment should be descheduled, each scheduled assignment event has a 'generation',
        // and if at assignment time its generation differs from the current generation, it won't
        // be performed
        AstAssign* ଅଚଳ preAssignp = VN_AS(beginp->stmtsp(), Assign);
        UASSERT_OBJ(preAssignp, alwaysp, "Pre-assign should be there from visit(AstFork)");
        AstAssign* ଅଚଳ postAssignp = VN_AS(preAssignp->nextp()->nextp(), Assign);
        UASSERT_OBJ(postAssignp, alwaysp, "Post-assign should be there from visit(AstFork)");
        // Increment generation and copy it to a local
        AstVarScope* ଅଚଳ generationVarp
            = m_assignWGens.make(flp, m_scopep, alwaysp->findUInt64DType());
        AstVarScope* ଅଚଳ genLocalVarp
            = createTemp(flp, generationVarp->varp()->name() + "__local",
                         alwaysp->findUInt64DType(), preAssignp);
        preAssignp->addHereThisAsNext(
            ନୂତନ AstAssign{flp, ନୂତନ AstVarRef{flp, generationVarp, VAccess::WRITE},
                          ନୂତନ AstAdd{flp, ନୂତନ AstVarRef{flp, generationVarp, VAccess::READ},
                                     ନୂତନ AstConst{flp, 1}}});
        preAssignp->addHereThisAsNext(
            ନୂତନ AstAssign{flp, ନୂତନ AstVarRef{flp, genLocalVarp, VAccess::WRITE},
                          ନୂତନ AstVarRef{flp, generationVarp, VAccess::READ}});
        // If the current generation is same as the one saved in the local var, assign
        beginp->addStmtsp(
            ନୂତନ AstIf{flp,
                      ନୂତନ AstEq{flp, ନୂତନ AstVarRef{flp, generationVarp, VAccess::READ},
                                ନୂତନ AstVarRef{flp, genLocalVarp, VAccess::READ}},
                      postAssignp->unlinkFrBack()});
        // Save scheduled RHS value before delay
        AstVarScope* ଅଚଳ tmpVarp
            = m_assignWTmps.make(flp, m_scopep, preAssignp->rhsp()->dtypep());
        AstVarRef* ଅଚଳ tmpAssignRhsp = VN_AS(preAssignp->lhsp(), VarRef)->clone_ବୃକ୍ଷ(ମିଥ୍ୟା);
        tmpAssignRhsp->access(VAccess::WRITE);
        preAssignp->addNextHere(
            ନୂତନ AstAssign{flp, ନୂତନ AstVarRef{flp, tmpVarp, VAccess::WRITE}, tmpAssignRhsp});
        // If the RHS is different from the currently scheduled value, schedule the new assignment
        // The generation will increase, effectively 'descheduling' the previous assignment.
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ didNotInitp
            = ନୂତନ AstLogNot{flp, ନୂତନ AstCExpr{flp, "vlSymsp->__Vm_didInit", 1}};
        AstVarRef* ଅଚଳ firstIterp
            = ନୂତନ AstVarRef{flp, m_netlistp->stlFirstIterationp(), VAccess::READ};
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ schedCondp
            = ନୂତନ AstLogOr{flp,
                           ନୂତନ AstNeq{flp, preAssignp->rhsp()->clone_ବୃକ୍ଷ(ମିଥ୍ୟା),
                                      ନୂତନ AstVarRef{flp, tmpVarp, VAccess::READ}},
                           ନୂତନ AstLogAnd{flp, didNotInitp, firstIterp}};
        alwaysp->addStmtsp(ନୂତନ AstIf{flp, schedCondp, forkp->unlinkFrBack()});
    }
    ଶୂନ୍ୟ visit(AstDisableFork* nodep) override {
        ଯଦି (m_hasProcess) ଫେରନ୍ତୁ;
        // never reached by any process; remove to avoid compilation error
        VL_DO_DANGLING(nodep->unlinkFrBack()->delete_ବୃକ୍ଷ(), nodep);
    }
    ଶୂନ୍ୟ visit(AstWaitFork* nodep) override {
        ଯଦି (m_hasProcess) {
            File_ଧାଡ଼ି* ଅଚଳ flp = nodep->fileline();
            AstCExpr* ଅଚଳ exprp = ନୂତନ AstCExpr{flp, "vlProcess->completedFork()", 1};
            AstWait* ଅଚଳ waitp = ନୂତନ AstWait{flp, exprp, nullptr};
            nodep->replaceWith(waitp);
        } ଅନ୍ୟଥା {
            // never reached by any process; remove to avoid compilation error
            nodep->unlinkFrBack();
        }
        VL_DO_DANGLING(nodep->delete_ବୃକ୍ଷ(), nodep);
    }
    ଶୂନ୍ୟ visit(AstWait* nodep) override {
        // Wait on changed events related to the vars in the wait statement
        UINFO(9, "control-visit " << nodep);
        File_ଧାଡ଼ି* ଅଚଳ flp = nodep->fileline();
        Ast_ଗଣ୍ଠି* ଅଚଳ stmtsp = nodep->stmtsp();
        ଯଦି (stmtsp) stmtsp->unlinkFrBackWithNext();
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ condp = V3Const::constifyEdit(nodep->condp()->unlinkFrBack());
        auto* ଅଚଳ constp = VN_CAST(condp, Const);
        ଯଦି (constp) {
            ଯଦି (constp->isZero()) {
                // We have to await forever instead of simply returning in case we're deep in a
                // callstack
                AstCMethodHard* ଅଚଳ foreverMethodp = ନୂତନ AstCMethodHard{
                    flp, ନୂତନ AstVarRef{flp, getCreateDelayScheduler(), VAccess::WRITE},
                    VCMethod::SCHED_WAIT_FOREVER};
                foreverMethodp->dtypeSetVoid();
                addProcessInfo(foreverMethodp);
                addDebugInfo(foreverMethodp);
                AstCAwait* ଅଚଳ awaitp = ନୂତନ AstCAwait{flp, foreverMethodp};
                nodep->replaceWith(awaitp);
                ଯଦି (stmtsp) VL_DO_DANGLING(stmtsp->delete_ବୃକ୍ଷ(), stmtsp);
                VL_DO_DANGLING(condp->delete_ବୃକ୍ଷ(), condp);
            } ଅନ୍ୟଥା {
                nodep->v3fatalSrc("constant wait should have been removed in "
                                  "TimingSuspendableVisitor::visit(AstWait)");
            }
        } ଅନ୍ୟଥା ଯଦି (needDynamicTrigger(condp)) {
            // No point in making a sentree, just use the expression as sensitivity
            // Put the event control in an if so we only wait if the condition isn't met already
            auto* ଅଚଳ ifp = ନୂତନ AstIf{
                flp, ନୂତନ AstLogNot{flp, condp},
                ନୂତନ AstEventControl{flp,
                                    ନୂତନ Ast_Sen_ବୃକ୍ଷ{flp, ନୂତନ AstSenItem{flp, VEdgeType::ET_TRUE,
                                                                       condp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା)}},
                                    nullptr}};
            ଯଦି (stmtsp) Ast_ଗଣ୍ଠି::addNext<Ast_ଗଣ୍ଠି, Ast_ଗଣ୍ଠି>(ifp, stmtsp);
            nodep->replaceWith(ifp);
        } ଅନ୍ୟଥା {
            // We are using a global sentree, so we cannot use ET_TRUE, as that could lead to the
            // active region never converging. Because of this, we need to use ET_CHANGED in a
            // loop.
            AstEventControl* ଅଚଳ controlp = ନୂତନ AstEventControl{
                flp,
                ନୂତନ Ast_Sen_ବୃକ୍ଷ{
                    flp, ନୂତନ AstSenItem{flp, VEdgeType::ET_CHANGED, condp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା)}},
                nullptr};
            controlp->user2(ସତ୍ୟ);  // Commit immediately
            AstLoop* ଅଚଳ loopp = ନୂତନ AstLoop{flp};
            loopp->addStmtsp(ନୂତନ AstLoopTest{flp, loopp, ନୂତନ AstLogNot{flp, condp}});
            loopp->addStmtsp(controlp);
            ଯଦି (stmtsp) Ast_ଗଣ୍ଠି::addNext<Ast_ଗଣ୍ଠି, Ast_ଗଣ୍ଠି>(loopp, stmtsp);
            nodep->replaceWith(loopp);
        }
        VL_DO_DANGLING(nodep->delete_ବୃକ୍ଷ(), nodep);
    }
    ଶୂନ୍ୟ visit(AstBegin* nodep) override {
        VL_RESTORER(m_procp);
        VL_RESTORER(m_hasProcess);
        m_hasProcess |= hasFlags(nodep, T_HAS_PROC);
        // A pending NBA update is not a subprocess of the process that scheduled it, so it is
        // unaffected by 'disable fork' and 'wait fork' (IEEE 1800-2023 9.6.1, 9.6.3)
        ଯଦି (hasFlags(nodep, T_NBA_UPDATE)) m_hasProcess = ମିଥ୍ୟା;
        m_procp = nodep;
        ଯଦି (m_hasProcess) nodep->setNeedProcess();
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(AstFork* nodep) override {
        ଯଦି (nodep->user1SetOnce()) ଫେରନ୍ତୁ;
        UINFO(9, "control-visit " << nodep);
        v3Global.setUsesTiming();

        // Create a unique name for this fork
        nodep->name("__Vfork_" + std::to_string(++m_forkCnt));

        // TODO: Should process nodep->stmtsp() in case an earlier pass
        //       inserted something there, but as of today we don't need to.

        // Process and name each fork
        size_t idx = 0;  // Index for naming begins
        ପାଇଁ (AstBegin *itemp = nodep->forksp(), *nextp; itemp; itemp = nextp) {
            nextp = VN_AS(itemp->nextp(), Begin);
            iterate(itemp);
            // Note: Even if we do not find any awaits, we cannot simply inline
            // the process here, as new awaits could be added later.

            // Name the begin (later the name will be used for a new function)
            itemp->name(nodep->name() + "__" + std::to_string(idx++));
            ଯଦି (itemp->needProcess()) {
                itemp->addStmtsp(
                    ନୂତନ AstCStmt{itemp->fileline(), "vlProcess->state(VlProcess::FINISHED);"});
            }
        }
        ଯଦି (!nodep->joinType().joinNone()) makeForkJoin(nodep);
    }

    //--------------------
    ଶୂନ୍ୟ visit(AstVar*) override {}  // Accelerate
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override { iterateChildren(nodep); }

ସର୍ବସାଧାରଣ:
    // CONSTRUCTORS
    explicit TimingControlVisitor(AstNetlist* nodep)
        : m_netlistp{nodep} {
        iterate(nodep);

        // If there is no static #0 in the design, but an unknown delay was found,
        // and the user did not specify preference, then warn on all unknown delays
        // as we will be assuming they can be #0, which can cause performance degradation.
        ପାଇଁ (File_ଧାଡ଼ି* ଅଚଳ flp : m_unknownDelayFlps) {
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

Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* V3Timing::delayToTicks(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* valuep, VTimescale timeunit) {
    File_ଧାଡ଼ି* ଅଚଳ flp = valuep->fileline();
    ଯଦି (ଅଚଳ AstConst* ଅଚଳ constp = VN_CAST(valuep, Const)) {
        ଯଦି (constp->ଅଙ୍କ().is1Step()) {
            // A step is one tick of the global time precision (IEEE 1800-2023 3.14.3)
            VL_DO_DANGLING(valuep->delete_ବୃକ୍ଷ(), valuep);
            AstConst* ଅଚଳ tickp = ନୂତନ AstConst{flp, AstConst::Unsized64{}, 1};
            tickp->dtypeSetBitSized(64, VSigning::UNSIGNED);
            ଫେରନ୍ତୁ tickp;
        }
        ଯଦି (constp->isZero()) ଫେରନ୍ତୁ valuep;
    }
    UASSERT_OBJ(!timeunit.isNone(), valuep, "timeunit must be set");
    ଅଚଳ ଦ୍ୱିଗୁଣ timescaleFactorD
        = std::pow(10.0, timeunit.powerOfTen() - v3Global.rootp()->timeprecision().powerOfTen());
    ଯଦି (valuep->dtypep()->skipRefp()->isDouble()) {
        AstConst* ଅଚଳ tsfp = ନୂତନ AstConst{flp, AstConst::RealDouble{}, timescaleFactorD};
        valuep = ନୂତନ AstMulD{flp, valuep, tsfp};
        valuep = ନୂତନ AstRToIRoundS{flp, valuep};
        valuep->dtypeSetBitSized(64, VSigning::UNSIGNED);
    } ଅନ୍ୟଥା {
        valuep->dtypeSetBitSized(64, VSigning::UNSIGNED);
        ଅଚଳ uint64_t timescaleFactorU = static_cast<uint64_t>(timescaleFactorD);
        AstConst* ଅଚଳ tsfp = ନୂତନ AstConst{flp, AstConst::Unsized64{}, timescaleFactorU};
        valuep = ନୂତନ AstMul{flp, valuep, tsfp};
    }
    ଫେରନ୍ତୁ V3Const::constifyEdit(valuep);  // Simplify
}

ଶୂନ୍ୟ V3Timing::timingAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        ଅଚଳ VNUser1InUse m_user1InUse;
        ଅଚଳ VNUser2InUse m_user2InUse;
        { TimingSuspendableVisitor{nodep}; }
        ଯଦି (v3Global.usesTiming()) TimingControlVisitor{nodep};
    }
    V3Global::dump_Check_Global_ବୃକ୍ଷ("timing", 0, dump_ବୃକ୍ଷ_Either_Level() >= 3);
}
