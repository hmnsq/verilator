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

#شامل "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#شامل "V3Timing.h"

#شامل "V3Const.h"
#شامل "V3EmitV.h"
#شامل "V3File.h"
#شامل "V3Global.h"
#شامل "V3Graph.h"
#شامل "V3MemberMap.h"
#شامل "V3SenExprBuilder.h"
#شامل "V3SenTree.h"
#شامل "V3SharedTmps.h"
#شامل "V3Stats.h"
#شامل "V3UniqueNames.h"

#شامل <limits>
#شامل <queue>

VL_DEFINE_DEBUG_FUNCTIONS;

// ######################################################################

شمارش گره_Flag : uint8_t {
    T_SUSPENDEE = 1 << 0,  // Suspendable (due to dependence on another suspendable)
    T_SUSPENDER = 1 << 1,  // Suspendable (has timing control)
    T_ALLOCS_PROC = 1 << 2,  // Can allocate VlProcess
    T_FORCES_PROC = 1 << 3,  // Forces VlProcess allocation
    T_NEEDS_PROC = 1 << 4,  // Needs access to VlProcess if it's allocated
    T_HAS_PROC = 1 << 5,  // Has VlProcess argument in the signature
    T_NBA_UPDATE = 1 << 6,  // Fork branch of a pending NBA update, which is not a subprocess
};

شمارش ForkType : uint8_t {
    F_NONE = 0,  // Not under a fork
    F_MIGHT_SUSPEND = 1 << 0,  // Fork might suspend the execution of current process
    F_MIGHT_NEED_PROC = 1 << 1,  // Fork might need a process (any fork really)
};

شمارش PropagationType : uint8_t {
    P_CALL = 1,  // Propagation through call to a function/task/method
    P_FORK = 2,  // Propagation due to fork's behavior
    P_SIGNATURE = 3,  // Propagation required to maintain C++ function's signature requirements
};

// Add timing flag to a node
ایستا تهی addFlags(Ast_گره* ثابت nodep, uint8_t flags) { nodep->user2Or(flags); }
// Check if a node has ALL of the expected flags set
ایستا منطقی hasFlags(Ast_گره* ثابت nodep, uint8_t flags) { بازگشت !(~nodep->user2() & flags); }

// ######################################################################
//  Detect nodes affected by timing and/or requiring a process

رده TimingSuspendableVisitor final : عمومی VNVisitor {
    // TYPES
    // Vertex of a dependency graph of suspendable nodes, e.g. if a node (process or task) is
    // suspendable, all its dependents should also be suspendable
    رده DepVtx VL_NOT_FINAL : عمومی V3GraphVertex {
        VL_RTTI_IMPL(DepVtx, V3GraphVertex)
        AstClass* ثابت m_classp;  // Class associated with a method
        Ast_گره* ثابت m_nodep;  // AST node represented by this graph vertex

        // ACCESSORS
        string name() ثابت override {
            اگر (m_classp) {
                اگر (VN_IS(nodep(), CFunc)) {
                    بازگشت cvtToHex(nodep()) + ' ' + classp()->name() + "::" + nodep()->name();
                }
            }
            بازگشت cvtToHex(nodep()) + ' ' + nodep()->prettyTypeName();
        }
        File_سطر* fileline() ثابت override { بازگشت nodep()->fileline(); }

    عمومی:
        // CONSTRUCTORS
        DepVtx(V3Graph* graphp, Ast_گره* nodep, AstClass* classp)
            : V3GraphVertex{graphp}
            , m_classp{classp}
            , m_nodep{nodep} {}
        ~DepVtx() override = پیشفرض;

        // ACCESSORS
        مجازی Ast_گره* nodep() ثابت VL_MT_STABLE { بازگشت m_nodep; }
        مجازی Ast_گره* classp() ثابت VL_MT_STABLE { بازگشت m_classp; }
    };

    رده SuspendDepVtx final : عمومی DepVtx {
        VL_RTTI_IMPL(SuspendDepVtx, DepVtx)
        string dotColor() ثابت override {
            اگر (hasFlags(nodep(), T_SUSPENDER)) بازگشت "red";
            اگر (hasFlags(nodep(), T_SUSPENDEE)) بازگشت "blue";
            بازگشت "black";
        }

    عمومی:
        SuspendDepVtx(V3Graph* graphp, Ast_گره* nodep, AstClass* classp)
            : DepVtx{graphp, nodep, classp} {}
        ~SuspendDepVtx() override = پیشفرض;
    };

    رده NeedsProcDepVtx final : عمومی DepVtx {
        VL_RTTI_IMPL(NeedsProcDepVtx, DepVtx)
        string dotColor() ثابت override {
            اگر (hasFlags(nodep(), T_HAS_PROC)) بازگشت "blue";
            اگر (hasFlags(nodep(), T_NEEDS_PROC)) بازگشت "green";
            اگر (hasFlags(nodep(), T_FORCES_PROC)) بازگشت "red";
            بازگشت "black";
        }

    عمومی:
        NeedsProcDepVtx(V3Graph* graphp, Ast_گره* nodep, AstClass* classp)
            : DepVtx{graphp, nodep, classp} {}
        ~NeedsProcDepVtx() override = پیشفرض;
    };

    // NODE STATE
    //  AstClass::user1() -> bool.  Class member cache has been refreshed.
    //  Ast{NodeProcedure,CFunc,Begin}::user2()  -> uint64_t.  Set to >= T_SUSP if
    //                                                         process/task suspendable
    //                                                         and to T_PROC if it
    //                                                         needs process metadata.
    //  Ast{NodeProcedure,CFunc,Begin}::user3()  -> DependencyVertex*.  Vertex in m_suspGraph
    //  Ast{NodeProcedure,CFunc,Begin}::user3()  -> DependencyVertex*.  Vertex in m_procGraph
    ثابت VNUser3InUse m_user3InUse;
    ثابت VNUser4InUse m_user4InUse;

    // STATE
    VMemberMap m_memberMap;  // Member names cached for fast lookup
    AstClass* m_classp = nullptr;  // Current class
    Ast_گره* m_procp = nullptr;  // NodeProcedure/CFunc/Begin we're under
    uint8_t m_underFork = F_NONE;  // F_NONE or flags of a fork we are under
    V3Graph m_suspGraph;  // Dependency graph where a node is a dependency of another if it being
                          // suspendable makes the other node suspendable
    V3Graph m_procGraph;  // Dependency graph where a node is a dependency of another if it being
                          // suspendable makes the other node suspendable

    // METHODS
    // Get or create the dependency vertex for the given node
    DepVtx* getSuspendDepVtx(Ast_گره* ثابت nodep) {
        AstClass* classp = nullptr;
        اگر (AstCFunc* funcp = VN_CAST(nodep, CFunc)) {
            اگر (funcp->scopep() && funcp->scopep()->modp()) {
                classp = VN_CAST(funcp->scopep()->modp(), Class);
            }
        }
        اگر (!nodep->user3p()) nodep->user3p(جدید SuspendDepVtx{&m_suspGraph, nodep, classp});
        بازگشت nodep->user3u().to<SuspendDepVtx*>();
    }
    DepVtx* getNeedsProcDepVtx(Ast_گره* ثابت nodep) {
        AstClass* classp = nullptr;
        اگر (AstCFunc* funcp = VN_CAST(nodep, CFunc)) {
            اگر (funcp->scopep() && funcp->scopep()->modp()) {
                classp = VN_CAST(funcp->scopep()->modp(), Class);
            }
        }
        اگر (!nodep->user4p()) nodep->user4p(جدید NeedsProcDepVtx{&m_procGraph, nodep, classp});
        بازگشت nodep->user4u().to<NeedsProcDepVtx*>();
    }
    // Pass timing flag between nodes
    منطقی passFlag(ثابت Ast_گره* from, Ast_گره* to, گره_Flag flag) {
        اگر ((from->user2() & flag) && !(to->user2() & flag)) {
            addFlags(to, flag);
            بازگشت درست;
        }
        بازگشت نادرست;
    }
    // Propagate flag to all nodes that depend on the given one
    تهی propagateFlags(DepVtx* ثابت vxp, گره_Flag flag) {
        auto* ثابت parentp = vxp->nodep();
        برای (V3GraphEdge& edge : vxp->outEdges()) {
            auto* ثابت depVxp = static_cast<DepVtx*>(edge.بالاترین());
            Ast_گره* ثابت depp = depVxp->nodep();
            اگر (passFlag(parentp, depp, flag)) propagateFlags(depVxp, flag);
        }
    }
    template <typename T_Predicate>
    تهی propagateFlagsIf(DepVtx* ثابت vxp, گره_Flag flag, T_Predicate p) {
        auto* ثابت parentp = vxp->nodep();
        برای (V3GraphEdge& edge : vxp->outEdges()) {
            auto* ثابت depVxp = static_cast<DepVtx*>(edge.بالاترین());
            Ast_گره* ثابت depp = depVxp->nodep();
            اگر (p(&edge) && passFlag(parentp, depp, flag)) propagateFlagsIf(depVxp, flag, p);
        }
    }
    template <typename T_Predicate>
    تهی propagateFlagsReversedIf(DepVtx* ثابت vxp, گره_Flag flag, T_Predicate p) {
        auto* ثابت parentp = vxp->nodep();
        برای (V3GraphEdge& edge : vxp->inEdges()) {
            auto* ثابت depVxp = static_cast<DepVtx*>(edge.fromp());
            Ast_گره* ثابت depp = depVxp->nodep();
            اگر (p(&edge) && passFlag(parentp, depp, flag))
                propagateFlagsReversedIf(depVxp, flag, p);
        }
    }

    // VISITORS
    تهی visit(AstClass* nodep) override {
        UASSERT_OBJ(!m_classp, nodep, "Class under class");
        VL_RESTORER(m_classp);
        m_classp = nodep;
        iterateChildren(nodep);
    }
    تهی visit(Ast_گره_Procedure* nodep) override {
        VL_RESTORER(m_procp);
        m_procp = nodep;
        getNeedsProcDepVtx(nodep);
        addFlags(nodep, T_ALLOCS_PROC);
        اگر (VN_IS(nodep, Always)) {
            UINFO(9, "Always does " << (nodep->needProcess() ? "" : "NOT ") << "need process");
        }
        iterateChildren(nodep);
    }
    تهی visit(AstDisableFork* nodep) override {
        visit(static_cast<Ast_گره*>(nodep));
        addFlags(m_procp, T_FORCES_PROC | T_NEEDS_PROC);
    }
    تهی visit(AstWaitFork* nodep) override {
        visit(static_cast<Ast_گره*>(nodep));
        addFlags(m_procp, T_FORCES_PROC | T_NEEDS_PROC);
    }
    تهی visit(AstWait* nodep) override {
        UINFO(9, "suspendable-visit " << nodep);
        Ast_گره_عبارت* ثابت condp = V3Const::constifyEdit(nodep->condp());
        اگر (AstConst* ثابت constp = VN_CAST(condp, Const)) {
            اگر (!nodep->fileline()->warnIsOff(V3ErrorCode::WAITCONST)) {
                condp->v3warn(WAITCONST, "Wait statement condition is constant");
            }
            اگر (!constp->isZero()) {
                // Remove AstWait before we track process as T_SUSPENDER
                اگر (Ast_گره* ثابت stmtsp = nodep->stmtsp()) {
                    stmtsp->unlinkFrBackWithNext();
                    nodep->replaceWith(stmtsp);
                } وگرنه {
                    nodep->unlinkFrBack();
                }
                VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
                بازگشت;
            }
        }
        v3Global.setUsesTiming();
        اگر (m_procp) addFlags(m_procp, T_SUSPENDEE | T_SUSPENDER | T_NEEDS_PROC);
        iterateChildren(nodep);
    }
    تهی visit(AstCFunc* nodep) override {
        VL_RESTORER(m_procp);
        m_procp = nodep;
        iterateChildren(nodep);
        اگر (nodep->needProcess()) addFlags(nodep, T_FORCES_PROC | T_NEEDS_PROC);
        DepVtx* ثابت sVxp = getSuspendDepVtx(nodep);
        DepVtx* ثابت pVxp = getNeedsProcDepVtx(nodep);
        اگر (!m_classp) بازگشت;

        // Go over overridden functions

        std::queue<AstClassExtends*> extends;
        اگر (m_classp->extendsp()) extends.افزودن(m_classp->extendsp());

        وقتی (!extends.empty()) {
            AstClassExtends* ext_list = extends.front();
            extends.برداشت();

            برای (AstClassExtends* cextp = ext_list; cextp;
                 cextp = VN_AS(cextp->nextp(), ClassExtends)) {
                // TODO: It is possible that a methods the same name in the base class is not
                // actually overridden by our method. If this causes a problem, traverse to
                // the root of the inheritance hierarchy and check if the original method is
                // virtual or not.
                اگر (auto* ثابت overriddenp
                    = VN_CAST(m_memberMap.findMember(cextp->classp(), nodep->name()), CFunc)) {
                    // Suspendability and process affects typing, so they propagate both ways
                    DepVtx* ثابت overriddenSVxp = getSuspendDepVtx(overriddenp);
                    DepVtx* ثابت overriddenPVxp = getNeedsProcDepVtx(overriddenp);
                    جدید V3GraphEdge{&m_suspGraph, sVxp, overriddenSVxp, P_SIGNATURE};
                    جدید V3GraphEdge{&m_suspGraph, overriddenSVxp, sVxp, P_SIGNATURE};
                    جدید V3GraphEdge{&m_procGraph, pVxp, overriddenPVxp, P_SIGNATURE};
                    جدید V3GraphEdge{&m_procGraph, overriddenPVxp, pVxp, P_SIGNATURE};
                } وگرنه {
                    AstClassExtends* more_extends = cextp->classp()->extendsp();
                    اگر (more_extends) extends.افزودن(more_extends);
                }
            }
        }
    }
    تهی visit(Ast_گره_CCall* nodep) override {
        AstCFunc* ثابت funcp = nodep->funcp();
        UASSERT_OBJ(funcp, nodep, "AstNodeCCall must have non-null funcp post-link");
        UASSERT_OBJ(m_procp, nodep, "AstNodeCCall must be inside a procedure/CFunc/Begin");

        UINFO(9, "V3Timing: Processing CCall to " << funcp->name() << " in dependency graph\n");
        جدید V3GraphEdge{&m_suspGraph, getSuspendDepVtx(funcp), getSuspendDepVtx(m_procp), P_CALL};

        جدید V3GraphEdge{&m_procGraph, getNeedsProcDepVtx(funcp), getNeedsProcDepVtx(m_procp),
                        P_CALL};

        iterateChildren(nodep);
    }
    تهی visit(AstBegin* nodep) override {
        VL_RESTORER(m_procp);
        VL_RESTORER(m_underFork);

        اگر (!m_underFork || (m_underFork & F_MIGHT_SUSPEND))
            جدید V3GraphEdge{&m_suspGraph, getSuspendDepVtx(nodep), getSuspendDepVtx(m_procp),
                            m_underFork ? P_FORK : P_CALL};

        جدید V3GraphEdge{&m_procGraph, getNeedsProcDepVtx(nodep), getNeedsProcDepVtx(m_procp),
                        P_CALL};

        اگر (m_underFork) addFlags(nodep, T_NEEDS_PROC | T_ALLOCS_PROC);

        m_procp = nodep;
        m_underFork = 0;
        iterateChildren(nodep);
    }
    تهی visit(AstFork* nodep) override {
        UINFO(9, "suspendable-visit " << nodep);
        VL_RESTORER(m_underFork);

        v3Global.setUsesTiming();  // Even if there are no event controls, we have to set this flag
                                   // so that transformForks() in V3SchedTiming gets called and
                                   // removes all forks and begins
        اگر (nodep->isTimingControl() && m_procp) {
            addFlags(m_procp, T_SUSPENDEE | T_SUSPENDER);
            m_underFork |= F_MIGHT_SUSPEND;
        }
        m_underFork |= F_MIGHT_NEED_PROC;
        iterateChildren(nodep);
    }
    تهی visit(AstAssignDly* nodep) override {
        اگر (!VN_IS(m_procp, گره_Procedure)) v3Global.setUsesTiming();
        visit(static_cast<Ast_گره*>(nodep));
    }
    تهی visit(AstFireEvent* nodep) override {
        اگر (nodep->isDelayed()
            && (!VN_IS(m_procp, گره_Procedure) || !VN_IS(nodep->operandp(), VarRef))) {
            v3Global.setUsesTiming();
        }
        visit(static_cast<Ast_گره*>(nodep));
    }
    تهی visit(AstAssignW* nodep) override {
        اگر (nodep->timingControlp()) v3Global.setUsesTiming();
        // Containing process will not suspend, don't mark it
    }
    تهی visit(Ast_گره* nodep) override {
        اگر (nodep->isTimingControl()) {
            v3Global.setUsesTiming();
            اگر (m_procp) addFlags(m_procp, T_SUSPENDEE | T_SUSPENDER | T_NEEDS_PROC);
        }
        iterateChildren(nodep);
    }

    //--------------------
    تهی visit(AstVar*) override {}  // Accelerate

عمومی:
    // CONSTRUCTORS
    explicit TimingSuspendableVisitor(AstNetlist* nodep) {
        iterate(nodep);
        m_suspGraph.removeTransitiveEdges();
        m_procGraph.removeTransitiveEdges();
        // Propagate suspendability
        برای (V3GraphVertex& vtx : m_suspGraph.vertices()) {
            DepVtx& depVtx = static_cast<DepVtx&>(vtx);
            اگر (hasFlags(depVtx.nodep(), T_SUSPENDEE)) propagateFlags(&depVtx, T_SUSPENDEE);
        }
        اگر (dumpGraphLevel() >= 6) m_suspGraph.dumpDotFilePrefixed("timing_deps");

        // Propagate T_HAS_PROCESS
        برای (V3GraphVertex& vtx : m_procGraph.vertices()) {
            DepVtx& depVtx = static_cast<DepVtx&>(vtx);
            // Find processes that'll allocate VlProcess
            اگر (hasFlags(depVtx.nodep(), T_FORCES_PROC)) {
                propagateFlagsIf(&depVtx, T_FORCES_PROC, [&](ثابت V3GraphEdge* e) -> منطقی {
                    بازگشت !hasFlags(static_cast<DepVtx*>(e->fromp())->nodep(), T_ALLOCS_PROC);
                });
            }
            // Mark nodes on paths between processes and statements that use VlProcess
            اگر (hasFlags(depVtx.nodep(), T_NEEDS_PROC)) {
                propagateFlagsIf(&depVtx, T_NEEDS_PROC, [&](ثابت V3GraphEdge* e) -> منطقی {
                    بازگشت !hasFlags(static_cast<DepVtx*>(e->بالاترین())->nodep(), T_ALLOCS_PROC);
                });
            }
        }
        برای (V3GraphVertex& vtx : m_procGraph.vertices()) {
            DepVtx& depVtx = static_cast<DepVtx&>(vtx);
            // Mark nodes that will be emitted with a VlProcess argument
            اگر (hasFlags(depVtx.nodep(), T_ALLOCS_PROC | T_FORCES_PROC)) {
                addFlags(depVtx.nodep(), T_HAS_PROC);
                propagateFlagsReversedIf(&depVtx, T_HAS_PROC, [&](ثابت V3GraphEdge* e) -> منطقی {
                    بازگشت hasFlags(static_cast<DepVtx*>(e->fromp())->nodep(), T_NEEDS_PROC);
                });
            }
        }
        اگر (dumpGraphLevel() >= 6) m_procGraph.dumpDotFilePrefixed("proc_deps");
    }
    ~TimingSuspendableVisitor() override = پیشفرض;
};

// ######################################################################
//  Transform nodes affected by timing

رده TimingControlVisitor final : عمومی VNVisitor {
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
    Ast_بالاترین_Scope* ثابت m_بالاترین_Scopep = m_netlistp->بالاترین_Scopep();  // The AstTopScope
    AstScope* ثابت m_scopeTopp = m_بالاترین_Scopep->scopep();  // Scope at the top
    AstClass* m_classp = nullptr;  // Current class
    AstScope* m_scopep = nullptr;  // Current scope
    AstActive* m_activep = nullptr;  // Current active
    Ast_گره* m_procp = nullptr;  // NodeProcedure/CFunc/Begin we're under
    منطقی m_hasProcess = نادرست;  // True if current scope has a VlProcess handle available
    صحیح m_forkCnt = 0;  // Number of forks inside a module
    منطقی m_underJumpBlock = نادرست;  // True if we are inside of a jump-block
    منطقی m_underProcedure = نادرست;  // True if we are under an always or initial
    منطقی m_underIfaceCFunc = نادرست;  // True if we are under a CFunc owned by an interface scope
    منطقی m_hasStaticZeroDelay = نادرست;  // True if we have a static #0 delay
    std::vector<File_سطر*> m_unknownDelayFlps;  // Locations of AstDelay with non-constant value

    // Unique names
    V3UniqueNames m_dlyforkNames{"__Vdlyfork"};  // Names for temp AssignW vars
    V3UniqueNames m_intra_مقدار_Names{"__Vintraval"};  // Intra assign delay value var names
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
    Ast_Sen_درخت* m_delaySensesp = nullptr;  // Domain to trigger if a delayed coroutine is resumed
    Ast_Sen_درخت* m_dynamicSensesp = nullptr;  // Domain to trigger if a dynamic trigger is set

    // Other
    Sen_درخت_Finder m_finder{m_netlistp};  // Sentree finder and uniquifier
    Sen_عبارت_Builder* m_sen_عبارت_Builderp = nullptr;  // Sens expression builder for current m_scope

    // Stats
    size_t m_statZeroDelays = 0;  // Number of statically known #0 delays
    size_t m_statConstDelays = 0;  // Number of statically known #const (non-zero) delays
    size_t m_statVariableDelays = 0;  // Number of delays with value unknown at compile time

    // METHODS
    // Transform an assignment with an intra timing control into a timing control with the
    // assignment under it
    Ast_گره* factorOutTimingControl(Ast_گره_Assign* nodep) ثابت {
        Ast_گره* stmtp = nodep;
        Ast_گره* ثابت controlp = nodep->timingControlp();
        اگر (AstDelay* ثابت delayp = VN_CAST(controlp, Delay)) {
            stmtp->replaceWith(delayp->unlinkFrBack());
            delayp->addStmtsp(stmtp);
            stmtp = delayp;
        } وگرنه اگر (Ast_Sen_درخت* ثابت sentreep = VN_CAST(controlp, Sen_درخت)) {
            AstEventControl* ثابت eventControlp
                = جدید AstEventControl{sentreep->fileline(), sentreep->unlinkFrBack(), nullptr};
            stmtp->replaceWith(eventControlp);
            eventControlp->addStmtsp(stmtp);
            stmtp = eventControlp;
        } وگرنه اگر (AstBegin* ثابت beginp = VN_CAST(controlp, Begin)) {
            // Begin from V3AssertPre
            stmtp->replaceWith(beginp->unlinkFrBack());
            beginp->addStmtsp(stmtp);
            stmtp = beginp;
        }
        بازگشت stmtp == nodep ? nullptr : stmtp;
    }
    ایستا منطقی staticallyNonZeroDelay(ثابت Ast_گره_عبارت* valuep) {
        اگر (ثابت AstConst* ثابت constp = VN_CAST(valuep, Const)) بازگشت !constp->isZero();
        اگر (ثابت AstCond* ثابت condp = VN_CAST(valuep, Cond)) {
            بازگشت staticallyNonZeroDelay(condp->thenp())
                   && staticallyNonZeroDelay(condp->elsep());
        }
        اگر (ثابت AstMul* ثابت mulp = VN_CAST(valuep, Mul)) {
            ثابت AstConst* ثابت lhsConstp = VN_CAST(mulp->lhsp(), Const);
            ثابت AstConst* ثابت rhsConstp = VN_CAST(mulp->rhsp(), Const);
            اگر (lhsConstp && !lhsConstp->isZero()) بازگشت staticallyNonZeroDelay(mulp->rhsp());
            اگر (rhsConstp && !rhsConstp->isZero()) بازگشت staticallyNonZeroDelay(mulp->lhsp());
        }
        بازگشت نادرست;
    }
    // Creates the global delay scheduler variable
    AstVarScope* getCreateDelayScheduler() {
        اگر (m_delaySchedp) بازگشت m_delaySchedp;
        auto* ثابت dlySchedDtp = جدید AstBasicDType{
            m_scopeTopp->fileline(), VBasicDTypeKwd::DELAY_SCHEDULER, VSigning::UNSIGNED};
        m_netlistp->typeTablep()->addTypesp(dlySchedDtp);
        m_delaySchedp = m_بالاترین_Scopep->createTemp("__VdlySched", dlySchedDtp);
        // Delay scheduler has to be accessible from top
        m_delaySchedp->varp()->sigPublic(درست);
        m_netlistp->delaySchedulerp(m_delaySchedp->varp());
        بازگشت m_delaySchedp;
    }
    // Creates the delay sentree
    Ast_Sen_درخت* get_Create_Delay_Sen_درخت() {
        اگر (m_delaySensesp) بازگشت m_delaySensesp;
        File_سطر* ثابت flp = m_scopeTopp->fileline();
        auto* ثابت awaitingCurrentTimep
            = جدید AstCMethodHard{flp, جدید AstVarRef{flp, getCreateDelayScheduler(), VAccess::READ},
                                 VCMethod::SCHED_AWAITING_CURRENT_TIME};
        awaitingCurrentTimep->dtypeSetBit();
        m_delaySensesp
            = جدید Ast_Sen_درخت{flp, جدید AstSenItem{flp, VEdgeType::ET_TRUE, awaitingCurrentTimep}};
        m_netlistp->بالاترین_Scopep()->addSenTreesp(m_delaySensesp);
        بازگشت m_delaySensesp;
    }
    // Creates the global dynamic trigger scheduler variable
    AstVarScope* getCreateDynamicTriggerScheduler() {
        اگر (m_dynamicSchedp) بازگشت m_dynamicSchedp;
        auto* ثابت dynSchedDtp
            = جدید AstBasicDType{m_scopeTopp->fileline(), VBasicDTypeKwd::DYNAMIC_TRIGGER_SCHEDULER,
                                VSigning::UNSIGNED};
        m_netlistp->typeTablep()->addTypesp(dynSchedDtp);
        m_dynamicSchedp = m_بالاترین_Scopep->createTemp("__VdynSched", dynSchedDtp);
        بازگشت m_dynamicSchedp;
    }
    // Creates the dynamic trigger sentree
    Ast_Sen_درخت* get_Create_Dynamic_Trigger_Sen_درخت() {
        اگر (m_dynamicSensesp) بازگشت m_dynamicSensesp;
        File_سطر* ثابت flp = m_scopeTopp->fileline();
        auto* ثابت awaitingCurrentTimep = جدید AstCMethodHard{
            flp, جدید AstVarRef{flp, getCreateDynamicTriggerScheduler(), VAccess::READ},
            VCMethod::SCHED_EVALUATE};
        awaitingCurrentTimep->dtypeSetBit();
        m_dynamicSensesp
            = جدید Ast_Sen_درخت{flp, جدید AstSenItem{flp, VEdgeType::ET_TRUE, awaitingCurrentTimep}};
        m_netlistp->بالاترین_Scopep()->addSenTreesp(m_dynamicSensesp);
        بازگشت m_dynamicSensesp;
    }
    // Creates the event variable to trigger in NBA region
    AstEventControl* createNbaEventControl(File_سطر* flp) {
        اگر (!m_netlistp->nbaEventp()) {
            auto* ثابت nbaEventDtp = جدید AstBasicDType{m_scopeTopp->fileline(),
                                                        VBasicDTypeKwd::EVENT, VSigning::UNSIGNED};
            m_netlistp->typeTablep()->addTypesp(nbaEventDtp);
            m_netlistp->nbaEventp(m_بالاترین_Scopep->createTemp("__VnbaEvent", nbaEventDtp));
            v3Global.setHasEvents();
        }
        بازگشت جدید AstEventControl{
            flp,
            جدید Ast_Sen_درخت{
                flp, جدید AstSenItem{flp, VEdgeType::ET_EVENT,
                                    جدید AstVarRef{flp, m_netlistp->nbaEventp(), VAccess::READ}}},
            nullptr};
    }
    // Creates the variable that, if set, causes the NBA event to be triggered
    AstAssign* createNbaEventTriggerAssignment(File_سطر* flp) {
        اگر (!m_netlistp->nbaEventTriggerp()) {
            m_netlistp->nbaEventTriggerp(m_بالاترین_Scopep->createTemp("__VnbaEventTrigger", 1));
        }
        بازگشت جدید AstAssign{flp,
                             جدید AstVarRef{flp, m_netlistp->nbaEventTriggerp(), VAccess::WRITE},
                             جدید AstConst{flp, AstConst::BitTrue{}}};
    }
    // Returns true if we are under a class or interface function, or the tree references locals.
    // These are cases where static, globally-evaluated triggers are not suitable.
    منطقی needDynamicTrigger(Ast_گره* ثابت nodep) ثابت {
        بازگشت m_classp || m_underIfaceCFunc || nodep->exists([](Ast_گره* ثابت nodep) {
            اگر (Ast_گره_Var_Ref* varp = VN_CAST(nodep, گره_Var_Ref)) {
                بازگشت varp->varp()->isFuncLocal();
            }
            بازگشت !nodep->isPure();
        });
    }
    // Creates a trigger scheduler variable
    AstVarScope* getCreateTriggerSchedulerp(Ast_Sen_درخت* ثابت sentreep) {
        اگر (!sentreep->user1p()) {
            اگر (!m_trigSchedDtp) {
                m_trigSchedDtp
                    = جدید AstBasicDType{m_scopeTopp->fileline(), VBasicDTypeKwd::TRIGGER_SCHEDULER,
                                        VSigning::UNSIGNED};
                m_netlistp->typeTablep()->addTypesp(m_trigSchedDtp);
            }
            AstVarScope* ثابت trigSchedp
                = m_بالاترین_Scopep->createTemp(m_trigSchedNames.get(sentreep), m_trigSchedDtp);
            sentreep->user1p(trigSchedp);
        }
        بازگشت VN_AS(sentreep->user1p(), VarScope);
    }
    // Creates a string describing the sentree
    AstCExpr* createEventDescription(Ast_Sen_درخت* ثابت sentreep) ثابت {
        اگر (!sentreep->user2p()) {
            std::stringstream ss;
            V3EmitV::verilog_For_درخت(sentreep, ss);
            // possibly a multiline string
            // Names were hashed by V3Name, show the user the original text
            std::string comment = VName::dehash(ss.str());
            std::replace(comment.begin(), comment.end(), '\n', ' ');
            // Escape quotes, e.g. from string-indexed event array selects
            comment = '"' + V3OutFormatter::quoteNameControls(comment) + '"';
            AstCExpr* ثابت commentp = جدید AstCExpr{sentreep->fileline(), comment};
            commentp->dtypeSetString();
            sentreep->user2p(commentp);
            بازگشت commentp;
        }
        بازگشت VN_AS(sentreep->user2p(), CExpr)->clone_درخت(نادرست);
    }
    // Adds debug info to a hardcoded method call
    تهی addDebugInfo(AstCMethodHard* ثابت methodp) ثابت {
        File_سطر* ثابت flp = methodp->fileline();
        ثابت منطقی protectIds = v3Global.opt.protectIds();
        AstCExpr* ثابت ap
            = جدید AstCExpr{flp, protectIds ? "VL_UNKNOWN" : '"' + flp->filenameEsc() + '"'};
        ap->dtypeSetString();
        methodp->addPinsp(ap);
        AstCExpr* ثابت bp = جدید AstCExpr{flp, protectIds ? "0" : cvtToStr(flp->lineno())};
        bp->dtypeSetString();
        methodp->addPinsp(bp);
    }
    // Adds debug info to a trigSched.trigger() call
    تهی addEventDebugInfo(AstCMethodHard* ثابت methodp, Ast_Sen_درخت* ثابت sentreep) ثابت {
        اگر (v3Global.opt.protectIds()) {
            File_سطر* ثابت flp = sentreep->fileline();
            AstCExpr* ثابت descp = جدید AstCExpr{flp, "VL_UNKNOWN"};
            descp->dtypeSetString();
            methodp->addPinsp(descp);
            addDebugInfo(methodp);
            بازگشت;
        }
        methodp->addPinsp(createEventDescription(sentreep));
        addDebugInfo(methodp);
    }
    // Adds process pointer to a hardcoded method call
    تهی addProcessInfo(AstCMethodHard* ثابت methodp) ثابت {
        File_سطر* ثابت flp = methodp->fileline();
        AstCExpr* ثابت ap
            = جدید AstCExpr{flp, (m_procp && m_hasProcess) ? "vlProcess" : "nullptr"};
        methodp->addPinsp(ap);
    }
    // Creates the fork handle type and returns it
    AstBasicDType* getCreateForkSyncDTypep() {
        اگر (m_forkDtp) بازگشت m_forkDtp;
        m_forkDtp = جدید AstBasicDType{m_scopeTopp->fileline(), VBasicDTypeKwd::FORK_SYNC,
                                      VSigning::UNSIGNED};
        m_netlistp->typeTablep()->addTypesp(m_forkDtp);
        بازگشت m_forkDtp;
    }
    // Move `nodep` into `AstCLocalScope`.
    AstCLocalScope* addCLocalScope(File_سطر* ثابت flp, Ast_گره* ثابت nodep) ثابت {
        اگر (!nodep) بازگشت nullptr;
        VNRelinker handle;
        nodep->unlinkFrBack(&handle);
        AstCLocalScope* ثابت cscopep = جدید AstCLocalScope{flp, nodep};
        handle.relink(cscopep);
        بازگشت cscopep;
    }
    // Create a temp variable and optionally put it before the specified node (mark local if so)
    AstVarScope* createTemp(File_سطر* ثابت flp, ثابت std::string& name,
                            Ast_گره_DType* ثابت dtypep, Ast_گره* ثابت درج_Beforep = nullptr) {
        AstVar* varp;
        اگر (درج_Beforep) {
            varp = جدید AstVar{flp, VVarType::BLOCKTEMP, name, dtypep};
            varp->funcLocal(درست);
            درج_Beforep->addHereThisAsNext(varp);
        } وگرنه {
            varp = جدید AstVar{flp, VVarType::MODULETEMP, name, dtypep};
            m_scopep->modp()->addStmtsp(varp);
        }
        AstVarScope* ثابت vscp = جدید AstVarScope{flp, m_scopep, varp};
        m_scopep->addVarsp(vscp);
        بازگشت vscp;
    }
    // Add a done() call on the fork sync
    تهی addForkDone(AstBegin* ثابت beginp, AstVarScope* ثابت forkVscp) ثابت {
        File_سطر* ثابت flp = beginp->fileline();
        auto* ثابت donep = جدید AstCMethodHard{
            beginp->fileline(), جدید AstVarRef{flp, forkVscp, VAccess::WRITE}, VCMethod::FORK_DONE};
        donep->dtypeSetVoid();
        addDebugInfo(donep);
        beginp->addStmtsp(donep->makeStmt());
    }
    ایستا منطقی has_Disable_Queue_افزودن_Self_Prefix(AstBegin* ثابت beginp) {
        // LinkJump prepends disable-by-name registration as:
        //   __VprocessQueue_*.push_back(std::process::self())
        // V3LiftExpr lifts std::process::self() into an assignment to a temporary, then V3Task
        // lowers that assignment's function call into a leading comment and AstStmtExpr. Thus the
        // push_back is no longer necessarily the first statement. Scan across this generated
        // prefix for it, stopping at the fork-start sentinel or the branch body. Registrations for
        // nested forks live in sub-blocks and so are not in this statement list.
        برای (Ast_گره* stmtp = beginp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            اگر (VN_IS(stmtp, Comment)) ادامه;
            Ast_Stmt_عبارت* ثابت stmtExprp = VN_CAST(stmtp, Stmt_عبارت);
            اگر (!stmtExprp) بشکن;
            اگر (stmtExprp->is_Disable_Queue_افزودن_Self_Stmt()) بازگشت درست;
        }
        بازگشت نادرست;
    }
    // Register a callback so killing a process-backed fork branch decrements the join counter
    تهی addForkOnKill(AstBegin* ثابت beginp, AstVarScope* ثابت forkVscp) ثابت {
        اگر (!beginp->needProcess() && !has_Disable_Queue_افزودن_Self_Prefix(beginp)) بازگشت;
        File_سطر* ثابت flp = beginp->fileline();
        AstCMethodHard* ثابت onKillp = جدید AstCMethodHard{
            flp, جدید AstVarRef{flp, forkVscp, VAccess::WRITE}, VCMethod::FORK_ON_KILL};
        onKillp->dtypeSetVoid();
        AstCExpr* ثابت processp = جدید AstCExpr{flp, "vlProcess"};
        processp
            ->dtypeSetVoid();  // Opaque process reference; type is irrelevant for hardcoded emit
        onKillp->addPinsp(processp);
        Ast_گره_Stmt* ثابت stmtp = onKillp->makeStmt();
        beginp->prependStmtsp(stmtp);
    }
    // Handle the 'join' part of a fork..join
    تهی makeForkJoin(AstFork* ثابت forkp) {
        // Create a fork sync var
        File_سطر* ثابت flp = forkp->fileline();
        // Insert the sync var directly before the fork
        Ast_گره* ثابت درج_Beforep = forkp;
        // Make sure all references to the fork var are contained within a single scope to prevent
        // moving statements accessing it in case of large functions splitting.
        addCLocalScope(flp, درج_Beforep);
        AstVarScope* forkVscp
            = createTemp(flp, forkp->name() + "__sync", getCreateForkSyncDTypep(), درج_Beforep);
        unsigned join_شمار = 0;  // Needed for join counter
        // Add a <fork sync>.done() to each begin
        برای (Ast_گره* beginp = forkp->forksp(); beginp; beginp = beginp->nextp()) {
            addForkOnKill(VN_AS(beginp, Begin), forkVscp);
            addForkDone(VN_AS(beginp, Begin), forkVscp);
            join_شمار++;
        }
        اگر (forkp->joinType().joinAny()) join_شمار = 1;
        // Set the join counter
        auto* ثابت initp = جدید AstCMethodHard{flp, جدید AstVarRef{flp, forkVscp, VAccess::WRITE},
                                               VCMethod::FORK_INIT, جدید AstConst{flp, join_شمار}};
        initp->dtypeSetVoid();
        addProcessInfo(initp);
        forkp->addHereThisAsNext(initp->makeStmt());
        // Await the join at the end
        auto* ثابت joinp = جدید AstCMethodHard{flp, جدید AstVarRef{flp, forkVscp, VAccess::WRITE},
                                               VCMethod::FORK_JOIN};
        joinp->dtypeSetVoid();
        addProcessInfo(joinp);
        addDebugInfo(joinp);
        forkp->addNextHere(جدید AstCAwait{flp, joinp});
    }

    // `procp` shall be a NodeProcedure/CFunc/Begin and within it vars from `varsp` will be placed.
    // `varsp` vector of vars which shall be localized.
    ایستا تهی localizeVars(Ast_گره* ثابت procp, ثابت std::vector<AstVar*>& varsp) {
        UASSERT(procp, "procp is nullptr");
        AstVar* newVarsp = nullptr;
        برای (AstVar* ثابت varp : varsp) {
            varp->funcLocal(درست);
            newVarsp = Ast_گره::addNextNull(newVarsp, varp->unlinkFrBack());
        }
        اگر (Ast_گره_Procedure* ثابت گره_Procp = VN_CAST(procp, گره_Procedure)) {
            گره_Procp->prependStmtsp(newVarsp);
        } وگرنه اگر (AstCFunc* ثابت cfuncp = VN_CAST(procp, CFunc)) {
            cfuncp->prependVarsp(newVarsp);
        } وگرنه اگر (AstBegin* ثابت beginp = VN_CAST(procp, Begin)) {
            beginp->prependStmtsp(newVarsp);
        } وگرنه {
            procp->v3fatalSrc(
                procp->prettyNameQ()
                << " is not of an expected type NodeProcedure/CFunc/Begin instead it is: "
                << procp->prettyTypeName());
        }
    }

    Ast_گره_عبارت* delayToIntegral(AstDelay* nodep, Ast_گره_عبارت* valuep) {
        ثابت AstConst* ثابت constp = VN_CAST(valuep, Const);
        // The fork start sentinel is already in ticks
        اگر (constp && constp->toUQuad() == std::numeric_limits<uint64_t>::max()) بازگشت valuep;
        بازگشت V3Timing::delayToTicks(valuep, nodep->timeunit());
    }

    // VISITORS
    تهی visit(Ast_گره_Module* nodep) override {
        UASSERT_OBJ(!m_classp, nodep, "Module or class under class");
        VL_RESTORER(m_classp);
        m_classp = VN_CAST(nodep, Class);
        VL_RESTORER(m_forkCnt);
        m_forkCnt = 0;
        iterateChildren(nodep);
    }
    تهی visit(AstScope* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        Sen_عبارت_Builder sen_عبارت_Builder{m_scopep};
        {  // Restore m_senExprBuilderp before destroying senExprBuilder
            VL_RESTORER(m_sen_عبارت_Builderp);
            m_sen_عبارت_Builderp = &sen_عبارت_Builder;
            iterateChildren(nodep);
        }
    }
    تهی visit(AstActive* nodep) override {
        VL_RESTORER(m_activep);
        m_activep = nodep;
        iterateChildren(nodep);
    }
    تهی visit(Ast_گره_Procedure* nodep) override {
        VL_RESTORER(m_procp);
        VL_RESTORER(m_hasProcess);
        m_procp = nodep;
        m_hasProcess = hasFlags(nodep, T_HAS_PROC);
        VL_RESTORER(m_underProcedure);
        m_underProcedure = درست;
        iterateChildren(nodep);
        اگر (hasFlags(nodep, T_SUSPENDEE)) nodep->setSuspendable();
        اگر (hasFlags(nodep, T_HAS_PROC)) nodep->setNeedProcess();
    }
    تهی visit(AstInitial* nodep) override {
        visit(static_cast<Ast_گره_Procedure*>(nodep));
        اگر (nodep->needProcess() && !nodep->user1SetOnce()) {
            nodep->addStmtsp(
                جدید AstCStmt{nodep->fileline(), "vlProcess->state(VlProcess::FINISHED);"});
        }
    }
    تهی visit(AstJumpBlock* nodep) override {
        VL_RESTORER(m_underJumpBlock);
        m_underJumpBlock = درست;
        visit(static_cast<Ast_گره_Stmt*>(nodep));
    }
    تهی visit(AstAlways* nodep) override {
        اگر (nodep->user1SetOnce()) بازگشت;
        VL_RESTORER(m_procp);
        VL_RESTORER(m_hasProcess);
        m_procp = nodep;
        m_hasProcess = hasFlags(nodep, T_HAS_PROC);
        VL_RESTORER(m_underProcedure);
        m_underProcedure = درست;
        // Workaround for killing `always` processes (doing that is pretty much UB)
        // TODO: Disallow killing `always` at runtime (throw an error)
        // Skip for combinational blocks; if the body has timing controls
        // iterateChildren will add T_SUSPENDEE, otherwise it would spin.
        اگر (hasFlags(nodep, T_HAS_PROC)
            && !(m_activep && m_activep->sentreep() && m_activep->sentreep()->hasCombo()))
            addFlags(nodep, T_SUSPENDEE);

        iterateChildren(nodep);
        اگر (hasFlags(nodep, T_HAS_PROC)) nodep->setNeedProcess();
        اگر (!hasFlags(nodep, T_SUSPENDEE)) بازگشت;
        nodep->setSuspendable();
        File_سطر* ثابت flp = nodep->fileline();
        Ast_Sen_درخت* ثابت sentreep = m_activep->sentreep();
        اگر (sentreep->hasClocked()) {
            Ast_گره* ثابت bodysp = nodep->stmtsp()->unlinkFrBackWithNext();
            auto* ثابت controlp = جدید AstEventControl{flp, sentreep->clone_درخت(نادرست), bodysp};
            nodep->addStmtsp(controlp);
            iterate(controlp);
        }
        // Note: The 'while (true)' outer loop will be added in V3Sched
        auto* ثابت activep = جدید AstActive{
            flp, "", جدید Ast_Sen_درخت{flp, جدید AstSenItem{flp, AstSenItem::Initial{}}}};
        activep->sen_درخت_Storep(activep->sentreep());
        activep->addStmtsp(nodep->unlinkFrBack());
        m_activep->addNextHere(activep);
    }
    تهی visit(AstCFunc* nodep) override {
        VL_RESTORER(m_procp);
        VL_RESTORER(m_hasProcess);
        VL_RESTORER(m_underIfaceCFunc);
        m_procp = nodep;
        m_hasProcess = hasFlags(nodep, T_HAS_PROC);
        m_underIfaceCFunc = VN_IS(m_scopep->modp(), Iface);
        iterateChildren(nodep);
        اگر (hasFlags(nodep, T_HAS_PROC)) nodep->setNeedProcess();
        اگر (!(hasFlags(nodep, T_SUSPENDEE))) بازگشت;

        nodep->rtnType("VlCoroutine");
        // If in a class, create a shared pointer to 'this'
        اگر (m_classp) {
            AstCStmt* ثابت cstmtp = جدید AstCStmt{nodep->fileline(), "VL_KEEP_THIS;"};
            nodep->prependStmtsp(cstmtp);
        }
        اگر (nodep->dpiExportImpl()) {
            nodep->exists([](AstCAwait* ثابت awaitp) -> منطقی {
                // A DPI-exported coroutine won't be able to block the calling code
                // Error on the await node; fall back to the function node
                awaitp->v3warn(E_UNSUPPORTED,
                               "Unsupported: Timing controls inside DPI-exported tasks");
                بازگشت درست;
            });
        }
    }
    تهی visit(Ast_گره_CCall* nodep) override {
        AstCFunc* ثابت funcp = nodep->funcp();

        // Skip automatic covergroup sampling calls
        اگر (funcp->isCovergroupSample()) {
            iterateChildren(nodep);
            بازگشت;
        }

        اگر (funcp->needProcess()) m_hasProcess = درست;
        اگر (hasFlags(funcp, T_SUSPENDEE) && !nodep->user1SetOnce()) {  // If suspendable
            // Calls to suspendables are always void return type, hence parent must be StmtExpr
            Ast_Stmt_عبارت* ثابت stmtp = VN_AS(nodep->backp(), Stmt_عبارت);
            stmtp->replaceWith(جدید AstCAwait{nodep->fileline(), nodep->unlinkFrBack()});
            VL_DO_DANGLING(افزودن_Deletep(stmtp), stmtp);
        }
        iterateChildren(nodep);
    }
    تهی visit(AstDelay* nodep) override {
        UASSERT_OBJ(!nodep->isCycleDelay(), nodep,
                    "Cycle delays should have been handled in V3AssertPre");
        File_سطر* ثابت flp = nodep->fileline();

        Ast_گره_عبارت* valuep = delayToIntegral(nodep, nodep->lhsp()->unlinkFrBack());

        // Statistics
        اگر (valuep->isZero()) {
            ++m_statZeroDelays;
        } وگرنه اگر (VN_IS(valuep, Const)) {
            ++m_statConstDelays;
        } وگرنه {
            ++m_statVariableDelays;
        }

        // Decide scheduling support for #0
        اگر (v3Global.opt.schedZeroDelay().isSetTrue()) {
            // User said to schedule for #0 support, nothing else to do
            v3Global.setUsesZeroDelay();
        } وگرنه اگر (v3Global.opt.schedZeroDelay().isSetFalse()) {
            // User said to schedule without #0 support. Still warn if a static #0 delay exists
            اگر (valuep->isZero()) {
                nodep->v3warn(
                    ZERODLY,
                    "Static #0 delay exists, but '--no-sched-zero-delay' was given.\n"
                        << nodep->warnMore()  //
                        << "... Can proceed, but this will fail at runtime if executed.");
            }
        } وگرنه {
            // User did not express preference, decide based on presence of delays
            اگر (valuep->isZero()) {
                // Statically known #0 delay exists, schedule for #0 support
                v3Global.setUsesZeroDelay();
                m_hasStaticZeroDelay = درست;
                // Don't warn on variable delays, as no point
                m_unknownDelayFlps.clear();
            } وگرنه اگر (staticallyNonZeroDelay(valuep)) {
                // Delay is dynamic, but every statically known outcome is non-zero.
                // So we don't need #0 delay support and there should be no warning.
            } وگرنه اگر (!VN_IS(valuep, Const)) {
                // Delay is not known at compiile time. Conservatively schedule for #0 support,
                // but warn if no static #0 delays used as performance might be improved
                v3Global.setUsesZeroDelay();
                اگر (!m_hasStaticZeroDelay) m_unknownDelayFlps.افزودن_back(nodep->fileline());
            }
        }

        // Replace self with a 'co_await dlySched.delay(<valuep>)'
        AstCMethodHard* ثابت delayMethodp = جدید AstCMethodHard{
            flp, جدید AstVarRef{flp, getCreateDelayScheduler(), VAccess::WRITE},
            VCMethod::SCHED_DELAY, valuep};
        delayMethodp->dtypeSetVoid();
        addProcessInfo(delayMethodp);
        addDebugInfo(delayMethodp);
        // Create the co_await
        Ast_گره* ثابت awaitp = جدید AstCAwait{flp, delayMethodp, get_Create_Delay_Sen_درخت()};
        // Relink child statements after the co_await
        اگر (Ast_گره* ثابت stmtsp = nodep->stmtsp()) {
            awaitp->addNext(stmtsp->unlinkFrBackWithNext());
        }
        nodep->replaceWith(awaitp);
        VL_DO_DANGLING(nodep->delete_درخت(), nodep);
    }
    تهی visit(AstEventControl* nodep) override {
        // Do not allow waiting on local named events, as they get enqueued for clearing, but can
        // go out of scope before that happens
        اگر (!nodep->sentreep()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Event control with implicit sensitivity (@*)"
                          " in this context; use an explicit sensitivity list instead");
            VL_DO_DANGLING(افزودن_Deletep(nodep->unlinkFrBack()), nodep);
            بازگشت;
        }
        File_سطر* ثابت flp = nodep->fileline();
        // Relink child statements after the event control
        اگر (nodep->stmtsp()) nodep->addNextHere(nodep->stmtsp()->unlinkFrBackWithNext());
        اگر (needDynamicTrigger(nodep->sentreep())) {
            // Create the trigger variable and init it with 0
            AstVarScope* ثابت trigvscp
                = createTemp(flp, m_dynTrigNames.get(nodep), nodep->findBitDType(), nodep);
            auto* ثابت initp = جدید AstAssign{flp, جدید AstVarRef{flp, trigvscp, VAccess::WRITE},
                                              جدید AstConst{flp, AstConst::BitFalse{}}};
            // Await the eval step with the dynamic trigger scheduler. First, create the method
            // call
            auto* ثابت evalMethodp = جدید AstCMethodHard{
                flp, جدید AstVarRef{flp, getCreateDynamicTriggerScheduler(), VAccess::WRITE},
                VCMethod::SCHED_EVALUATION};
            evalMethodp->dtypeSetVoid();
            addProcessInfo(evalMethodp);
            auto* ثابت sentreep = nodep->sentreep();
            addEventDebugInfo(evalMethodp, sentreep);
            // Create the co_await
            AstCAwait* ثابت awaitEvalp
                = جدید AstCAwait{flp, evalMethodp, get_Create_Dynamic_Trigger_Sen_درخت()};
            // Construct the sen expression for this sentree
            UASSERT_OBJ(m_sen_عبارت_Builderp, nodep, "No SenExprBuilder for this scope");
            auto* ثابت assignp = جدید AstAssign{flp, جدید AstVarRef{flp, trigvscp, VAccess::WRITE},
                                                m_sen_عبارت_Builderp->build(sentreep).first};
            // Get the SenExprBuilder results
            ثابت Sen_عبارت_Builder::Results senResults = m_sen_عبارت_Builderp->getAndClearResults();
            localizeVars(m_procp, senResults.m_vars);
            // If post updates are destructive (e.g. clearFired on events), perform a
            // conservative pre-clear once before entering the wait loop so stale state from a
            // previous wait does not cause an immediate false-positive trigger.
            ثابت منطقی hasDestructivePostUpdates = !senResults.m_destructivePostUpdates.empty();
            // Create the trigger eval loop, which will await the evaluation step and check the
            // trigger
            Ast_گره_عبارت* ثابت condp
                = جدید AstLogNot{flp, جدید AstVarRef{flp, trigvscp, VAccess::READ}};
            AstLoop* ثابت loopp = جدید AstLoop{flp};
            loopp->addStmtsp(جدید AstLoopTest{flp, loopp, condp});
            loopp->addStmtsp(awaitEvalp);
            // Put pre updates before the trigger check and assignment
            برای (Ast_گره_Stmt* ثابت stmtp : senResults.m_preUpdates) loopp->addStmtsp(stmtp);
            // Then the trigger check and assignment
            loopp->addStmtsp(assignp);
            // Let the dynamic trigger scheduler know if this trigger was set
            // If it was, a call to the scheduler's evaluate() will return true
            AstCMethodHard* ثابت anyTriggeredMethodp = جدید AstCMethodHard{
                flp, جدید AstVarRef{flp, getCreateDynamicTriggerScheduler(), VAccess::WRITE},
                VCMethod::SCHED_ANY_TRIGGERED, جدید AstVarRef{flp, trigvscp, VAccess::READ}};
            anyTriggeredMethodp->dtypeSetVoid();
            loopp->addStmtsp(anyTriggeredMethodp->makeStmt());
            // If the post update is destructive (e.g. event vars are cleared), create an await for
            // the post update step
            اگر (hasDestructivePostUpdates) {
                AstCAwait* ثابت awaitPostUpdatep = awaitEvalp->clone_درخت(نادرست);
                VN_AS(awaitPostUpdatep->exprp(), CMethodHard)->method(VCMethod::SCHED_POST_UPDATE);
                loopp->addStmtsp(awaitPostUpdatep);
            }
            // Put the post updates at the end of the loop
            برای (Ast_گره_Stmt* ثابت stmtp : senResults.m_postUpdates) loopp->addStmtsp(stmtp);
            // Finally, await the resumption step in 'act'
            AstCAwait* ثابت awaitResumep = awaitEvalp->clone_درخت(نادرست);
            VN_AS(awaitResumep->exprp(), CMethodHard)->method(VCMethod::SCHED_RESUMPTION);
            Ast_گره::addNext<Ast_گره_Stmt, Ast_گره_Stmt>(loopp, awaitResumep);
            // Replace the event control with one explicit stmt chain:
            //   init -> [inits] -> [optional destructive pre-clears] -> loop -> awaitResumption
            Ast_گره_Stmt* chainp = nullptr;
            chainp = Ast_گره::addNextNull(chainp, initp);
            برای (Ast_گره_Stmt* ثابت stmtp : senResults.m_inits) {
                chainp = Ast_گره::addNextNull(chainp, stmtp);
            }
            اگر (hasDestructivePostUpdates) {
                برای (Ast_گره_Stmt* ثابت stmtp : senResults.m_destructivePostUpdates) {
                    chainp = Ast_گره::addNextNull(chainp, stmtp->clone_درخت(نادرست));
                }
            }
            chainp = Ast_گره::addNextNull(chainp, loopp);
            nodep->replaceWith(chainp);
        } وگرنه {
            auto* ثابت sentreep = m_finder.get_Sen_درخت(nodep->sentreep());
            nodep->sentreep()->unlinkFrBack()->delete_درخت();
            // Get this sentree's trigger scheduler
            // Replace self with a 'co_await trigSched.trigger()'
            auto* ثابت triggerMethodp = جدید AstCMethodHard{
                flp, جدید AstVarRef{flp, getCreateTriggerSchedulerp(sentreep), VAccess::WRITE},
                VCMethod::SCHED_TRIGGER};
            triggerMethodp->dtypeSetVoid();
            // If it should be committed immediately, pass true, otherwise false
            triggerMethodp->addPinsp(nodep->user2() ? جدید AstConst{flp, AstConst::BitTrue{}}
                                                    : جدید AstConst{flp, AstConst::BitFalse{}});
            addProcessInfo(triggerMethodp);
            addEventDebugInfo(triggerMethodp, sentreep);
            // Create the co_await
            AstCAwait* ثابت awaitp = جدید AstCAwait{flp, triggerMethodp, sentreep};
            nodep->replaceWith(awaitp);
        }
        VL_DO_DANGLING(nodep->delete_درخت(), nodep);
    }
    تهی visit(Ast_گره_Assign* nodep) override {
        // Only process once to avoid infinite loops (due to the net delay)
        اگر (nodep->user1SetOnce()) بازگشت;
        UINFO(9, "control-visit " << nodep);
        File_سطر* ثابت flp = nodep->fileline();
        Ast_گره* controlp = factorOutTimingControl(nodep);
        ثابت منطقی inAssignDly = VN_IS(nodep, AssignDly);
        // Handle the intra assignment timing control
        // Transform if:
        // * there's a timing control in the assignment
        // * the assignment is an AssignDly and it's in a non-inlined function
        اگر (!controlp && (!inAssignDly || m_underProcedure)) {
            iterateChildren(nodep);
            بازگشت;
        }
        // The timing control, if any, which is put in a fork for an NBA
        Ast_گره* ثابت timingp = controlp;
        // Insert new vars before the timing control if we're in a function; in a process we can't
        // do that, as after V3Begin only modules and functions declare variables. These
        // intra-assignment vars will later be passed to forked processes by value.
        Ast_گره* درج_Beforep = m_underProcedure ? nullptr : controlp;
        // Special case for NBA
        اگر (inAssignDly) {
            // Put it in a fork so it doesn't block
            // Could already be the only thing directly under a fork, reuse that if possible
            AstFork* forkp = !nodep->nextp() ? VN_CAST(nodep->firstAbovep(), Fork) : nullptr;
            اگر (!forkp) forkp = جدید AstFork{flp, VJoinType::JOIN_NONE};
            اگر (!m_underProcedure) {
                // If it's in a function, it won't be handled by V3Delayed
                // Put it behind an additional named event that gets triggered in the NBA region
                AstEventControl* ثابت nbaEventControlp = createNbaEventControl(flp);
                AstAssign* ثابت trigAssignp = createNbaEventTriggerAssignment(flp);
                nodep->replaceWith(trigAssignp);
                trigAssignp->addNextHere(nbaEventControlp);
                nbaEventControlp->addStmtsp(nodep);
                درج_Beforep = forkp;
                اگر (!controlp) controlp = nbaEventControlp;
            }
            controlp->replaceWith(forkp);
            AstBegin* beginp = VN_CAST(controlp, Begin);
            اگر (!beginp) beginp = جدید AstBegin{nodep->fileline(), "", controlp, نادرست};
            forkp->addForksp(beginp);
            addFlags(beginp, T_NBA_UPDATE);
            controlp = forkp;
        }
        UASSERT_OBJ(nodep, controlp, "Assignment should have timing control");
        // Move `insertBeforep` into `AstCLocalScope` if necessary to avoid jumping over
        // a variable initialization that would be inserted before `insertBeforep`. All
        // access to this variable should be contained within returned `AstCLocalScope`.
        اگر (m_underJumpBlock) addCLocalScope(flp, درج_Beforep);
        // Function for replacing values with intermediate variables
        ثابت auto replaceWithIntermediate = [&](Ast_گره_عبارت* ثابت valuep,
                                                 ثابت std::string& name) {
            AstVarScope* ثابت newvscp = createTemp(flp, name, valuep->dtypep(), درج_Beforep);
            // A pending update keeps its own value, even if its process schedules more
            اگر (inAssignDly && !درج_Beforep) newvscp->fork_By_مقدار(درست);
            valuep->replaceWith(جدید AstVarRef{flp, newvscp, VAccess::READ});
            controlp->addHereThisAsNext(
                جدید AstAssign{flp, جدید AstVarRef{flp, newvscp, VAccess::WRITE}, valuep});
        };
        // NBAs with delays evaluate the indices selecting the target now (IEEE 1800-2023 10.4.2),
        // in order, each as a whole, also the selects and function calls in it. So follow the
        // selects of the target up from the variable, not into their indices. As an example, this
        // transforms 'x[a[i]][f(j)] <= #1 y' into 't0 = a[i]; t1 = f(j); x[t0][t1] <= #1 y'.
        // V3Const has split NBAs to concatenations, also streamed, into NBAs to each part.
        اگر (inAssignDly) {
            برای (Ast_گره* exprp = nodep->lhsp()->baseFromp(درست); exprp != nodep;
                 exprp = exprp->firstAbovep()) {
                اگر (AstSel* ثابت selp = VN_CAST(exprp, Sel)) {
                    // widthp should be const
                    اگر (!VN_IS(selp->lsbp(), Const)) {
                        replaceWithIntermediate(selp->lsbp(), m_intraLsbNames.get(nodep));
                    }
                } وگرنه اگر (Ast_گره_Sel* ثابت selp = VN_CAST(exprp, گره_Sel)) {
                    اگر (!VN_IS(selp->bitp(), Const)) {
                        replaceWithIntermediate(selp->bitp(), m_intraIndexNames.get(nodep));
                    }
                }
            }
        }
        // Replace the RHS with an intermediate value var
        replaceWithIntermediate(nodep->rhsp(), m_intra_مقدار_Names.get(nodep));
        اگر (!inAssignDly) بازگشت;
        // The process executing the NBA evaluates its delay (IEEE 1800-2023 4.9.4), not the forked
        // process, if it has side effects, e.g. calling a function that checks its process. A pure
        // one is evaluated as the forked process starts, at once.
        اگر (AstDelay* ثابت delayp = VN_CAST(timingp, Delay)) {
            اگر (!delayp->lhsp()->isPure()) {
                replaceWithIntermediate(delayp->lhsp(), m_intra_مقدار_Names.get(nodep));
            }
        }
    }
    تهی visit(AstFireEvent* nodep) override {
        // V3Delayed handles '->>' of a variable in a process. Like NBAs in non-inlined functions,
        // trigger other events from a fork awaiting the NBA region.
        اگر (!nodep->isDelayed() || (m_underProcedure && VN_IS(nodep->operandp(), VarRef))) {
            iterateChildren(nodep);
            بازگشت;
        }
        File_سطر* ثابت flp = nodep->fileline();
        AstAssign* ثابت trigAssignp = createNbaEventTriggerAssignment(flp);
        nodep->replaceWith(trigAssignp);
        AstFork* ثابت forkp = جدید AstFork{flp, VJoinType::JOIN_NONE};
        trigAssignp->addNextHere(forkp);
        اگر (m_underJumpBlock) addCLocalScope(flp, forkp);
        // The triggered event is the one referenced now, so evaluate handles and indices now
        Ast_گره_عبارت* ثابت eventp = nodep->operandp()->unlinkFrBack();
        ثابت auto evalNow = [&](Ast_گره_عبارت* ثابت valuep, V3UniqueNames& names) {
            // Like for an NBA, a process uses a module variable, passed to the fork by value
            AstVarScope* ثابت vscp = createTemp(flp, names.get(nodep), valuep->dtypep(),
                                                 m_underProcedure ? nullptr : forkp);
            اگر (m_underProcedure) vscp->fork_By_مقدار(درست);
            valuep->replaceWith(جدید AstVarRef{flp, vscp, VAccess::READ});
            // Unlike an event, a handle selecting one is only read
            valuep->foreach([](Ast_گره* ثابت np) {
                اگر (Ast_گره_Var_Ref* ثابت refp = VN_CAST(np, گره_Var_Ref)) {
                    refp->access(VAccess::READ);
                } وگرنه اگر (AstMemberSel* ثابت selp = VN_CAST(np, MemberSel)) {
                    selp->access(VAccess::READ);
                }
            });
            forkp->addHereThisAsNext(
                جدید AstAssign{flp, جدید AstVarRef{flp, vscp, VAccess::WRITE}, valuep});
        };
        Ast_گره_عبارت* refp = eventp;
        وقتی (Ast_گره_Sel* ثابت selp = VN_CAST(refp, گره_Sel)) {
            اگر (!VN_IS(selp->bitp(), Const)) evalNow(selp->bitp(), m_intraIndexNames);
            refp = selp->fromp();
        }
        // Handle of a class (or virtual interface)
        اگر (AstMemberSel* ثابت selp = VN_CAST(refp, MemberSel)) {
            evalNow(selp->fromp(), m_intra_مقدار_Names);
        }
        AstEventControl* ثابت controlp = createNbaEventControl(flp);
        controlp->addStmtsp(جدید AstFireEvent{flp, eventp, نادرست});
        AstBegin* ثابت beginp = جدید AstBegin{flp, "", controlp, نادرست};
        addFlags(beginp, T_NBA_UPDATE);
        forkp->addForksp(beginp);
        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
    }
    تهی visit(AstAssignW* nodep) override {
        File_سطر* ثابت flp = nodep->fileline();
        // Get the net delay unless this assignment was created for handling the net delay (user1)
        AstDelay* ثابت netDelayp = nodep->user1() ? nullptr : nodep->getLhsNetDelay();
        اگر (netDelayp) {
            اگر (nodep->timingControlp()) {
                // If this assignment has a delay, create another one to handle the net delay
                AstVarScope* ثابت newvscp = m_assignWTmps.make(flp, m_scopep, nodep->dtypep());
                AstAssignW* assignp = جدید AstAssignW{
                    nodep->fileline(), nodep->lhsp()->unlinkFrBack(),
                    جدید AstVarRef{flp, newvscp, VAccess::READ}, netDelayp->clone_درخت(نادرست)};
                assignp->user1(درست);
                nodep->addNextHere(assignp);
                nodep->lhsp(جدید AstVarRef{flp, newvscp, VAccess::WRITE});
            } وگرنه {
                // Else just use this one with the net delay
                nodep->timingControlp(netDelayp->clone_درخت(نادرست));
            }
        }
        اگر (!nodep->timingControlp()) بازگشت;
        // There will be some circular logic here, suppress the warning for newly created vars
        flp->modifyWarnOff(V3ErrorCode::UNOPTFLAT, درست);
        // Also suppress the warning for the LHS var, for cases like `assign #1 clk = ~clk;`
        // TODO: Restore the warning for other, non-delayed drivers
        nodep->lhsp()->foreach([](AstVarRef* refp) {
            اگر (refp->access().isWriteOrRW()) {
                refp->varp()->fileline()->modifyWarnOff(V3ErrorCode::UNOPTFLAT, درست);
            }
        });
        // Should be under an always
        AstAlways* ثابت alwaysp = VN_AS(m_procp, Always);
        // Convert it to an Assign; the new assign with intra delay will be handled by
        // visit(AstNodeAssign*)
        Ast_گره_عبارت* ثابت lhs1p = nodep->lhsp()->unlinkFrBack();
        Ast_گره_عبارت* ثابت rhs1p = nodep->rhsp()->unlinkFrBack();
        Ast_گره* ثابت controlp = nodep->timingControlp()->unlinkFrBack();
        اگر (AstDelay* ثابت delayp = VN_CAST(controlp, Delay)) {
            اگر (Ast_گره_عبارت* fallDelayp = delayp->fallDelay()) {
                fallDelayp = fallDelayp->unlinkFrBack();
                Ast_گره_عبارت* lhsp = delayp->lhsp()->unlinkFrBack();
                // Use fall only for an all-zero value, rise otherwise.
                lhsp = delayToIntegral(delayp, lhsp);
                fallDelayp = delayToIntegral(delayp, fallDelayp);
                delayp->lhsp(
                    جدید AstCond{flp, جدید AstEq{flp, rhs1p->clone_درخت(نادرست), جدید AstConst{flp, 0}},
                                fallDelayp, lhsp});
            }
        }
        AstAssign* ثابت assignp = جدید AstAssign{nodep->fileline(), lhs1p, rhs1p, controlp};
        // Put the assignment in a fork..join_none.
        AstFork* ثابت forkp = جدید AstFork{flp, VJoinType::JOIN_NONE};
        nodep->replaceWith(forkp);
        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
        AstBegin* ثابت beginp = جدید AstBegin{flp, "", assignp, نادرست};
        forkp->addForksp(beginp);
        visit(forkp);  // Visit now as we need to do some post-processing
        // IEEE 1800-2023 10.3.3 - if the RHS value differs from the currently scheduled value to
        // be assigned, the currently scheduled assignment is descheduled. To keep track if an
        // assignment should be descheduled, each scheduled assignment event has a 'generation',
        // and if at assignment time its generation differs from the current generation, it won't
        // be performed
        AstAssign* ثابت preAssignp = VN_AS(beginp->stmtsp(), Assign);
        UASSERT_OBJ(preAssignp, alwaysp, "Pre-assign should be there from visit(AstFork)");
        AstAssign* ثابت postAssignp = VN_AS(preAssignp->nextp()->nextp(), Assign);
        UASSERT_OBJ(postAssignp, alwaysp, "Post-assign should be there from visit(AstFork)");
        // Increment generation and copy it to a local
        AstVarScope* ثابت generationVarp
            = m_assignWGens.make(flp, m_scopep, alwaysp->findUInt64DType());
        AstVarScope* ثابت genLocalVarp
            = createTemp(flp, generationVarp->varp()->name() + "__local",
                         alwaysp->findUInt64DType(), preAssignp);
        preAssignp->addHereThisAsNext(
            جدید AstAssign{flp, جدید AstVarRef{flp, generationVarp, VAccess::WRITE},
                          جدید AstAdd{flp, جدید AstVarRef{flp, generationVarp, VAccess::READ},
                                     جدید AstConst{flp, 1}}});
        preAssignp->addHereThisAsNext(
            جدید AstAssign{flp, جدید AstVarRef{flp, genLocalVarp, VAccess::WRITE},
                          جدید AstVarRef{flp, generationVarp, VAccess::READ}});
        // If the current generation is same as the one saved in the local var, assign
        beginp->addStmtsp(
            جدید AstIf{flp,
                      جدید AstEq{flp, جدید AstVarRef{flp, generationVarp, VAccess::READ},
                                جدید AstVarRef{flp, genLocalVarp, VAccess::READ}},
                      postAssignp->unlinkFrBack()});
        // Save scheduled RHS value before delay
        AstVarScope* ثابت tmpVarp
            = m_assignWTmps.make(flp, m_scopep, preAssignp->rhsp()->dtypep());
        AstVarRef* ثابت tmpAssignRhsp = VN_AS(preAssignp->lhsp(), VarRef)->clone_درخت(نادرست);
        tmpAssignRhsp->access(VAccess::WRITE);
        preAssignp->addNextHere(
            جدید AstAssign{flp, جدید AstVarRef{flp, tmpVarp, VAccess::WRITE}, tmpAssignRhsp});
        // If the RHS is different from the currently scheduled value, schedule the new assignment
        // The generation will increase, effectively 'descheduling' the previous assignment.
        Ast_گره_عبارت* ثابت didNotInitp
            = جدید AstLogNot{flp, جدید AstCExpr{flp, "vlSymsp->__Vm_didInit", 1}};
        AstVarRef* ثابت firstIterp
            = جدید AstVarRef{flp, m_netlistp->stlFirstIterationp(), VAccess::READ};
        Ast_گره_عبارت* ثابت schedCondp
            = جدید AstLogOr{flp,
                           جدید AstNeq{flp, preAssignp->rhsp()->clone_درخت(نادرست),
                                      جدید AstVarRef{flp, tmpVarp, VAccess::READ}},
                           جدید AstLogAnd{flp, didNotInitp, firstIterp}};
        alwaysp->addStmtsp(جدید AstIf{flp, schedCondp, forkp->unlinkFrBack()});
    }
    تهی visit(AstDisableFork* nodep) override {
        اگر (m_hasProcess) بازگشت;
        // never reached by any process; remove to avoid compilation error
        VL_DO_DANGLING(nodep->unlinkFrBack()->delete_درخت(), nodep);
    }
    تهی visit(AstWaitFork* nodep) override {
        اگر (m_hasProcess) {
            File_سطر* ثابت flp = nodep->fileline();
            AstCExpr* ثابت exprp = جدید AstCExpr{flp, "vlProcess->completedFork()", 1};
            AstWait* ثابت waitp = جدید AstWait{flp, exprp, nullptr};
            nodep->replaceWith(waitp);
        } وگرنه {
            // never reached by any process; remove to avoid compilation error
            nodep->unlinkFrBack();
        }
        VL_DO_DANGLING(nodep->delete_درخت(), nodep);
    }
    تهی visit(AstWait* nodep) override {
        // Wait on changed events related to the vars in the wait statement
        UINFO(9, "control-visit " << nodep);
        File_سطر* ثابت flp = nodep->fileline();
        Ast_گره* ثابت stmtsp = nodep->stmtsp();
        اگر (stmtsp) stmtsp->unlinkFrBackWithNext();
        Ast_گره_عبارت* ثابت condp = V3Const::constifyEdit(nodep->condp()->unlinkFrBack());
        auto* ثابت constp = VN_CAST(condp, Const);
        اگر (constp) {
            اگر (constp->isZero()) {
                // We have to await forever instead of simply returning in case we're deep in a
                // callstack
                AstCMethodHard* ثابت foreverMethodp = جدید AstCMethodHard{
                    flp, جدید AstVarRef{flp, getCreateDelayScheduler(), VAccess::WRITE},
                    VCMethod::SCHED_WAIT_FOREVER};
                foreverMethodp->dtypeSetVoid();
                addProcessInfo(foreverMethodp);
                addDebugInfo(foreverMethodp);
                AstCAwait* ثابت awaitp = جدید AstCAwait{flp, foreverMethodp};
                nodep->replaceWith(awaitp);
                اگر (stmtsp) VL_DO_DANGLING(stmtsp->delete_درخت(), stmtsp);
                VL_DO_DANGLING(condp->delete_درخت(), condp);
            } وگرنه {
                nodep->v3fatalSrc("constant wait should have been removed in "
                                  "TimingSuspendableVisitor::visit(AstWait)");
            }
        } وگرنه اگر (needDynamicTrigger(condp)) {
            // No point in making a sentree, just use the expression as sensitivity
            // Put the event control in an if so we only wait if the condition isn't met already
            auto* ثابت ifp = جدید AstIf{
                flp, جدید AstLogNot{flp, condp},
                جدید AstEventControl{flp,
                                    جدید Ast_Sen_درخت{flp, جدید AstSenItem{flp, VEdgeType::ET_TRUE,
                                                                       condp->clone_درخت(نادرست)}},
                                    nullptr}};
            اگر (stmtsp) Ast_گره::addNext<Ast_گره, Ast_گره>(ifp, stmtsp);
            nodep->replaceWith(ifp);
        } وگرنه {
            // We are using a global sentree, so we cannot use ET_TRUE, as that could lead to the
            // active region never converging. Because of this, we need to use ET_CHANGED in a
            // loop.
            AstEventControl* ثابت controlp = جدید AstEventControl{
                flp,
                جدید Ast_Sen_درخت{
                    flp, جدید AstSenItem{flp, VEdgeType::ET_CHANGED, condp->clone_درخت(نادرست)}},
                nullptr};
            controlp->user2(درست);  // Commit immediately
            AstLoop* ثابت loopp = جدید AstLoop{flp};
            loopp->addStmtsp(جدید AstLoopTest{flp, loopp, جدید AstLogNot{flp, condp}});
            loopp->addStmtsp(controlp);
            اگر (stmtsp) Ast_گره::addNext<Ast_گره, Ast_گره>(loopp, stmtsp);
            nodep->replaceWith(loopp);
        }
        VL_DO_DANGLING(nodep->delete_درخت(), nodep);
    }
    تهی visit(AstBegin* nodep) override {
        VL_RESTORER(m_procp);
        VL_RESTORER(m_hasProcess);
        m_hasProcess |= hasFlags(nodep, T_HAS_PROC);
        // A pending NBA update is not a subprocess of the process that scheduled it, so it is
        // unaffected by 'disable fork' and 'wait fork' (IEEE 1800-2023 9.6.1, 9.6.3)
        اگر (hasFlags(nodep, T_NBA_UPDATE)) m_hasProcess = نادرست;
        m_procp = nodep;
        اگر (m_hasProcess) nodep->setNeedProcess();
        iterateChildren(nodep);
    }
    تهی visit(AstFork* nodep) override {
        اگر (nodep->user1SetOnce()) بازگشت;
        UINFO(9, "control-visit " << nodep);
        v3Global.setUsesTiming();

        // Create a unique name for this fork
        nodep->name("__Vfork_" + std::to_string(++m_forkCnt));

        // TODO: Should process nodep->stmtsp() in case an earlier pass
        //       inserted something there, but as of today we don't need to.

        // Process and name each fork
        size_t idx = 0;  // Index for naming begins
        برای (AstBegin *itemp = nodep->forksp(), *nextp; itemp; itemp = nextp) {
            nextp = VN_AS(itemp->nextp(), Begin);
            iterate(itemp);
            // Note: Even if we do not find any awaits, we cannot simply inline
            // the process here, as new awaits could be added later.

            // Name the begin (later the name will be used for a new function)
            itemp->name(nodep->name() + "__" + std::to_string(idx++));
            اگر (itemp->needProcess()) {
                itemp->addStmtsp(
                    جدید AstCStmt{itemp->fileline(), "vlProcess->state(VlProcess::FINISHED);"});
            }
        }
        اگر (!nodep->joinType().joinNone()) makeForkJoin(nodep);
    }

    //--------------------
    تهی visit(AstVar*) override {}  // Accelerate
    تهی visit(Ast_گره* nodep) override { iterateChildren(nodep); }

عمومی:
    // CONSTRUCTORS
    explicit TimingControlVisitor(AstNetlist* nodep)
        : m_netlistp{nodep} {
        iterate(nodep);

        // If there is no static #0 in the design, but an unknown delay was found,
        // and the user did not specify preference, then warn on all unknown delays
        // as we will be assuming they can be #0, which can cause performance degradation.
        برای (File_سطر* ثابت flp : m_unknownDelayFlps) {
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

Ast_گره_عبارت* V3Timing::delayToTicks(Ast_گره_عبارت* valuep, VTimescale timeunit) {
    File_سطر* ثابت flp = valuep->fileline();
    اگر (ثابت AstConst* ثابت constp = VN_CAST(valuep, Const)) {
        اگر (constp->رقم().is1Step()) {
            // A step is one tick of the global time precision (IEEE 1800-2023 3.14.3)
            VL_DO_DANGLING(valuep->delete_درخت(), valuep);
            AstConst* ثابت tickp = جدید AstConst{flp, AstConst::Unsized64{}, 1};
            tickp->dtypeSetBitSized(64, VSigning::UNSIGNED);
            بازگشت tickp;
        }
        اگر (constp->isZero()) بازگشت valuep;
    }
    UASSERT_OBJ(!timeunit.isNone(), valuep, "timeunit must be set");
    ثابت دوگانه timescaleFactorD
        = std::pow(10.0, timeunit.powerOfTen() - v3Global.rootp()->timeprecision().powerOfTen());
    اگر (valuep->dtypep()->skipRefp()->isDouble()) {
        AstConst* ثابت tsfp = جدید AstConst{flp, AstConst::RealDouble{}, timescaleFactorD};
        valuep = جدید AstMulD{flp, valuep, tsfp};
        valuep = جدید AstRToIRoundS{flp, valuep};
        valuep->dtypeSetBitSized(64, VSigning::UNSIGNED);
    } وگرنه {
        valuep->dtypeSetBitSized(64, VSigning::UNSIGNED);
        ثابت uint64_t timescaleFactorU = static_cast<uint64_t>(timescaleFactorD);
        AstConst* ثابت tsfp = جدید AstConst{flp, AstConst::Unsized64{}, timescaleFactorU};
        valuep = جدید AstMul{flp, valuep, tsfp};
    }
    بازگشت V3Const::constifyEdit(valuep);  // Simplify
}

تهی V3Timing::timingAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        ثابت VNUser1InUse m_user1InUse;
        ثابت VNUser2InUse m_user2InUse;
        { TimingSuspendableVisitor{nodep}; }
        اگر (v3Global.usesTiming()) TimingControlVisitor{nodep};
    }
    V3Global::dump_Check_Global_درخت("timing", 0, dump_درخت_Either_Level() >= 3);
}
