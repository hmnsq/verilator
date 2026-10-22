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

#ਅਨ੍ਤਰ੍ਭਾਵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Timing.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Const.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3EmitV.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3File.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Global.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Graph.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3MemberMap.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3SenExprBuilder.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3SenTree.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3SharedTmps.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Stats.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3UniqueNames.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <limits>
#ਅਨ੍ਤਰ੍ਭਾਵ <queue>

VL_DEFINE_DEBUG_FUNCTIONS;

// ######################################################################

ਗਿਣਤੀ ਗੰਢ_Flag : uint8_t {
    T_SUSPENDEE = 1 << 0,  // Suspendable (due to dependence on another suspendable)
    T_SUSPENDER = 1 << 1,  // Suspendable (has timing control)
    T_ALLOCS_PROC = 1 << 2,  // Can allocate VlProcess
    T_FORCES_PROC = 1 << 3,  // Forces VlProcess allocation
    T_NEEDS_PROC = 1 << 4,  // Needs access to VlProcess if it's allocated
    T_HAS_PROC = 1 << 5,  // Has VlProcess argument in the signature
    T_NBA_UPDATE = 1 << 6,  // Fork branch of a pending NBA update, which is not a subprocess
};

ਗਿਣਤੀ ForkType : uint8_t {
    F_NONE = 0,  // Not under a fork
    F_MIGHT_SUSPEND = 1 << 0,  // Fork might suspend the execution of current process
    F_MIGHT_NEED_PROC = 1 << 1,  // Fork might need a process (any fork really)
};

ਗਿਣਤੀ ਪ੍ਰਸਾਰ_Type : uint8_t {
    P_CALL = 1,  // Propagation through call to a function/task/method
    P_FORK = 2,  // Propagation due to fork's behavior
    P_SIGNATURE = 3,  // Propagation required to maintain C++ function's signature requirements
};

// Add timing flag to a node
ਸਥਿਰ ਖਾਲੀ ਜੋੜੋ_Flags(Ast_ਗੰਢ* ਅਚਲ nodep, uint8_t flags) { nodep->user2Or(flags); }
// Check if a node has ALL of the expected flags set
ਸਥਿਰ ਤਾਰਕਿਕ hasFlags(Ast_ਗੰਢ* ਅਚਲ nodep, uint8_t flags) { ਵਾਪਸ !(~nodep->user2() & flags); }

// ######################################################################
//  Detect nodes affected by timing and/or requiring a process

ਵਰਗ TimingSuspendableVisitor final : ਜਨਤਕ VNVisitor {
    // TYPES
    // Vertex of a dependency graph of suspendable nodes, e.g. if a node (process or task) is
    // suspendable, all its dependents should also be suspendable
    ਵਰਗ DepVtx VL_NOT_FINAL : ਜਨਤਕ V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼ {
        VL_RTTI_IMPL(DepVtx, V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼)
        AstClass* ਅਚਲ m_classp;  // Class associated with a method
        Ast_ਗੰਢ* ਅਚਲ m_nodep;  // AST node represented by this graph vertex

        // ACCESSORS
        string name() ਅਚਲ override {
            ਜੇ (m_classp) {
                ਜੇ (VN_IS(nodep(), CFunc)) {
                    ਵਾਪਸ cvt_ਮੰਜ਼ਿਲ_Hex(nodep()) + ' ' + classp()->name() + "::" + nodep()->name();
                }
            }
            ਵਾਪਸ cvt_ਮੰਜ਼ਿਲ_Hex(nodep()) + ' ' + nodep()->prettyTypeName();
        }
        File_ਸਤਰ* fileline() ਅਚਲ override { ਵਾਪਸ nodep()->fileline(); }

    ਜਨਤਕ:
        // CONSTRUCTORS
        DepVtx(V3_ਗ੍ਰਾਫ* graphp, Ast_ਗੰਢ* nodep, AstClass* classp)
            : V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼{graphp}
            , m_classp{classp}
            , m_nodep{nodep} {}
        ~DepVtx() override = ਮੂਲ;

        // ACCESSORS
        ਆਭਾਸੀ Ast_ਗੰਢ* nodep() ਅਚਲ VL_MT_STABLE { ਵਾਪਸ m_nodep; }
        ਆਭਾਸੀ Ast_ਗੰਢ* classp() ਅਚਲ VL_MT_STABLE { ਵਾਪਸ m_classp; }
    };

    ਵਰਗ SuspendDepVtx final : ਜਨਤਕ DepVtx {
        VL_RTTI_IMPL(SuspendDepVtx, DepVtx)
        string dotColor() ਅਚਲ override {
            ਜੇ (hasFlags(nodep(), T_SUSPENDER)) ਵਾਪਸ "red";
            ਜੇ (hasFlags(nodep(), T_SUSPENDEE)) ਵਾਪਸ "blue";
            ਵਾਪਸ "black";
        }

    ਜਨਤਕ:
        SuspendDepVtx(V3_ਗ੍ਰਾਫ* graphp, Ast_ਗੰਢ* nodep, AstClass* classp)
            : DepVtx{graphp, nodep, classp} {}
        ~SuspendDepVtx() override = ਮੂਲ;
    };

    ਵਰਗ NeedsProcDepVtx final : ਜਨਤਕ DepVtx {
        VL_RTTI_IMPL(NeedsProcDepVtx, DepVtx)
        string dotColor() ਅਚਲ override {
            ਜੇ (hasFlags(nodep(), T_HAS_PROC)) ਵਾਪਸ "blue";
            ਜੇ (hasFlags(nodep(), T_NEEDS_PROC)) ਵਾਪਸ "green";
            ਜੇ (hasFlags(nodep(), T_FORCES_PROC)) ਵਾਪਸ "red";
            ਵਾਪਸ "black";
        }

    ਜਨਤਕ:
        NeedsProcDepVtx(V3_ਗ੍ਰਾਫ* graphp, Ast_ਗੰਢ* nodep, AstClass* classp)
            : DepVtx{graphp, nodep, classp} {}
        ~NeedsProcDepVtx() override = ਮੂਲ;
    };

    // NODE STATE
    //  AstClass::user1() -> bool.  Class member cache has been refreshed.
    //  Ast{NodeProcedure,CFunc,Begin}::user2()  -> uint64_t.  Set to >= T_SUSP if
    //                                                         process/task suspendable
    //                                                         and to T_PROC if it
    //                                                         needs process metadata.
    //  Ast{NodeProcedure,CFunc,Begin}::user3()  -> DependencyVertex*.  Vertex in m_suspGraph
    //  Ast{NodeProcedure,CFunc,Begin}::user3()  -> DependencyVertex*.  Vertex in m_procGraph
    ਅਚਲ VNUser3_ਵਿੱਚ_Use m_user3_ਵਿੱਚ_Use;
    ਅਚਲ VNUser4_ਵਿੱਚ_Use m_user4_ਵਿੱਚ_Use;

    // STATE
    VMemberMap m_memberMap;  // Member names cached for fast lookup
    AstClass* m_classp = ਖਾਲੀ_ਸੂਚਕ;  // Current class
    Ast_ਗੰਢ* m_procp = ਖਾਲੀ_ਸੂਚਕ;  // NodeProcedure/CFunc/Begin we're under
    uint8_t m_underFork = F_NONE;  // F_NONE or flags of a fork we are under
    V3_ਗ੍ਰਾਫ m_susp_ਗ੍ਰਾਫ;  // Dependency graph where a node is a dependency of another if it being
                          // suspendable makes the other node suspendable
    V3_ਗ੍ਰਾਫ m_proc_ਗ੍ਰਾਫ;  // Dependency graph where a node is a dependency of another if it being
                          // suspendable makes the other node suspendable

    // METHODS
    // Get or create the dependency vertex for the given node
    DepVtx* getSuspendDepVtx(Ast_ਗੰਢ* ਅਚਲ nodep) {
        AstClass* classp = ਖਾਲੀ_ਸੂਚਕ;
        ਜੇ (AstCFunc* funcp = VN_CAST(nodep, CFunc)) {
            ਜੇ (funcp->scopep() && funcp->scopep()->modp()) {
                classp = VN_CAST(funcp->scopep()->modp(), Class);
            }
        }
        ਜੇ (!nodep->user3p()) nodep->user3p(ਨਵਾਂ SuspendDepVtx{&m_susp_ਗ੍ਰਾਫ, nodep, classp});
        ਵਾਪਸ nodep->user3u().ਮੰਜ਼ਿਲ<SuspendDepVtx*>();
    }
    DepVtx* getNeedsProcDepVtx(Ast_ਗੰਢ* ਅਚਲ nodep) {
        AstClass* classp = ਖਾਲੀ_ਸੂਚਕ;
        ਜੇ (AstCFunc* funcp = VN_CAST(nodep, CFunc)) {
            ਜੇ (funcp->scopep() && funcp->scopep()->modp()) {
                classp = VN_CAST(funcp->scopep()->modp(), Class);
            }
        }
        ਜੇ (!nodep->user4p()) nodep->user4p(ਨਵਾਂ NeedsProcDepVtx{&m_proc_ਗ੍ਰਾਫ, nodep, classp});
        ਵਾਪਸ nodep->user4u().ਮੰਜ਼ਿਲ<NeedsProcDepVtx*>();
    }
    // Pass timing flag between nodes
    ਤਾਰਕਿਕ passFlag(ਅਚਲ Ast_ਗੰਢ* from, Ast_ਗੰਢ* ਮੰਜ਼ਿਲ, ਗੰਢ_Flag flag) {
        ਜੇ ((from->user2() & flag) && !(ਮੰਜ਼ਿਲ->user2() & flag)) {
            ਜੋੜੋ_Flags(ਮੰਜ਼ਿਲ, flag);
            ਵਾਪਸ ਸੱਚ;
        }
        ਵਾਪਸ ਝੂਠ;
    }
    // Propagate flag to all nodes that depend on the given one
    ਖਾਲੀ propagateFlags(DepVtx* ਅਚਲ vxp, ਗੰਢ_Flag flag) {
        ਸਵੈਚਾਲਿਤ* ਅਚਲ parentp = vxp->nodep();
        ਲਈ (V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ& ਕਿਨਾਰਾ : vxp->ਬਾਹਰਲਾ_ਕਿਨਾਰੇ()) {
            ਸਵੈਚਾਲਿਤ* ਅਚਲ depVxp = static_cast<DepVtx*>(ਕਿਨਾਰਾ.ਸਿਖਰ());
            Ast_ਗੰਢ* ਅਚਲ depp = depVxp->nodep();
            ਜੇ (passFlag(parentp, depp, flag)) propagateFlags(depVxp, flag);
        }
    }
    ਸਾਂਚਾ <ਕਿਸਮ_ਨਾਂ T_Predicate>
    ਖਾਲੀ propagateFlagsIf(DepVtx* ਅਚਲ vxp, ਗੰਢ_Flag flag, T_Predicate p) {
        ਸਵੈਚਾਲਿਤ* ਅਚਲ parentp = vxp->nodep();
        ਲਈ (V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ& ਕਿਨਾਰਾ : vxp->ਬਾਹਰਲਾ_ਕਿਨਾਰੇ()) {
            ਸਵੈਚਾਲਿਤ* ਅਚਲ depVxp = static_cast<DepVtx*>(ਕਿਨਾਰਾ.ਸਿਖਰ());
            Ast_ਗੰਢ* ਅਚਲ depp = depVxp->nodep();
            ਜੇ (p(&ਕਿਨਾਰਾ) && passFlag(parentp, depp, flag)) propagateFlagsIf(depVxp, flag, p);
        }
    }
    ਸਾਂਚਾ <ਕਿਸਮ_ਨਾਂ T_Predicate>
    ਖਾਲੀ propagateFlagsReversedIf(DepVtx* ਅਚਲ vxp, ਗੰਢ_Flag flag, T_Predicate p) {
        ਸਵੈਚਾਲਿਤ* ਅਚਲ parentp = vxp->nodep();
        ਲਈ (V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ& ਕਿਨਾਰਾ : vxp->ਵਿੱਚ_ਕਿਨਾਰੇ()) {
            ਸਵੈਚਾਲਿਤ* ਅਚਲ depVxp = static_cast<DepVtx*>(ਕਿਨਾਰਾ.fromp());
            Ast_ਗੰਢ* ਅਚਲ depp = depVxp->nodep();
            ਜੇ (p(&ਕਿਨਾਰਾ) && passFlag(parentp, depp, flag))
                propagateFlagsReversedIf(depVxp, flag, p);
        }
    }

    // VISITORS
    ਖਾਲੀ ਦੇਖੋ(AstClass* nodep) override {
        UASSERT_OBJ(!m_classp, nodep, "Class under class");
        VL_RESTORER(m_classp);
        m_classp = nodep;
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Procedure* nodep) override {
        VL_RESTORER(m_procp);
        m_procp = nodep;
        getNeedsProcDepVtx(nodep);
        ਜੋੜੋ_Flags(nodep, T_ALLOCS_PROC);
        ਜੇ (VN_IS(nodep, Always)) {
            UINFO(9, "Always does " << (nodep->needProcess() ? "" : "NOT ") << "need process");
        }
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstDisableFork* nodep) override {
        ਦੇਖੋ(static_cast<Ast_ਗੰਢ*>(nodep));
        ਜੋੜੋ_Flags(m_procp, T_FORCES_PROC | T_NEEDS_PROC);
    }
    ਖਾਲੀ ਦੇਖੋ(AstWaitFork* nodep) override {
        ਦੇਖੋ(static_cast<Ast_ਗੰਢ*>(nodep));
        ਜੋੜੋ_Flags(m_procp, T_FORCES_PROC | T_NEEDS_PROC);
    }
    ਖਾਲੀ ਦੇਖੋ(AstWait* nodep) override {
        UINFO(9, "suspendable-visit " << nodep);
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ condp = V3Const::constifyEdit(nodep->condp());
        ਜੇ (AstConst* ਅਚਲ constp = VN_CAST(condp, Const)) {
            ਜੇ (!nodep->fileline()->warnIsOff(V3_Error_ਕੋਡ::WAITCONST)) {
                condp->v3warn(WAITCONST, "Wait statement condition is constant");
            }
            ਜੇ (!constp->isZero()) {
                // Remove AstWait before we track process as T_SUSPENDER
                ਜੇ (Ast_ਗੰਢ* ਅਚਲ stmtsp = nodep->stmtsp()) {
                    stmtsp->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ();
                    nodep->replaceWith(stmtsp);
                } ਨਹੀਂਤਾਂ {
                    nodep->unlink_Fr_ਪਿੱਠ();
                }
                VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
                ਵਾਪਸ;
            }
        }
        v3Global.ਸੈੱਟ_ਵਰਤਦਾ_Timing();
        ਜੇ (m_procp) ਜੋੜੋ_Flags(m_procp, T_SUSPENDEE | T_SUSPENDER | T_NEEDS_PROC);
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstCFunc* nodep) override {
        VL_RESTORER(m_procp);
        m_procp = nodep;
        iterateChildren(nodep);
        ਜੇ (nodep->needProcess()) ਜੋੜੋ_Flags(nodep, T_FORCES_PROC | T_NEEDS_PROC);
        DepVtx* ਅਚਲ sVxp = getSuspendDepVtx(nodep);
        DepVtx* ਅਚਲ pVxp = getNeedsProcDepVtx(nodep);
        ਜੇ (!m_classp) ਵਾਪਸ;

        // Go over overridden functions

        std::ਕਤਾਰ<AstClassExtends*> extends;
        ਜੇ (m_classp->extendsp()) extends.ਧੱਕੋ(m_classp->extendsp());

        ਜਦੋਂ (!extends.ਖਾਲੀ_ਥਾਵਾਂ()) {
            AstClassExtends* ext_list = extends.front();
            extends.ਕੱਢੋ();

            ਲਈ (AstClassExtends* cextp = ext_list; cextp;
                 cextp = VN_AS(cextp->nextp(), ClassExtends)) {
                // TODO: It is possible that a methods the same name in the base class is not
                // actually overridden by our method. If this causes a problem, traverse to
                // the root of the inheritance hierarchy and check if the original method is
                // virtual or not.
                ਜੇ (ਸਵੈਚਾਲਿਤ* ਅਚਲ overriddenp
                    = VN_CAST(m_memberMap.ਲੱਭੋ_Member(cextp->classp(), nodep->name()), CFunc)) {
                    // Suspendability and process affects typing, so they propagate both ways
                    DepVtx* ਅਚਲ overriddenSVxp = getSuspendDepVtx(overriddenp);
                    DepVtx* ਅਚਲ overriddenPVxp = getNeedsProcDepVtx(overriddenp);
                    ਨਵਾਂ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ{&m_susp_ਗ੍ਰਾਫ, sVxp, overriddenSVxp, P_SIGNATURE};
                    ਨਵਾਂ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ{&m_susp_ਗ੍ਰਾਫ, overriddenSVxp, sVxp, P_SIGNATURE};
                    ਨਵਾਂ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ{&m_proc_ਗ੍ਰਾਫ, pVxp, overriddenPVxp, P_SIGNATURE};
                    ਨਵਾਂ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ{&m_proc_ਗ੍ਰਾਫ, overriddenPVxp, pVxp, P_SIGNATURE};
                } ਨਹੀਂਤਾਂ {
                    AstClassExtends* ਹੋਰ_extends = cextp->classp()->extendsp();
                    ਜੇ (ਹੋਰ_extends) extends.ਧੱਕੋ(ਹੋਰ_extends);
                }
            }
        }
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_CCall* nodep) override {
        AstCFunc* ਅਚਲ funcp = nodep->funcp();
        UASSERT_OBJ(funcp, nodep, "AstNodeCCall must have non-null funcp post-link");
        UASSERT_OBJ(m_procp, nodep, "AstNodeCCall must be inside a procedure/CFunc/Begin");

        UINFO(9, "V3Timing: Processing CCall to " << funcp->name() << " in dependency graph\n");
        ਨਵਾਂ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ{&m_susp_ਗ੍ਰਾਫ, getSuspendDepVtx(funcp), getSuspendDepVtx(m_procp), P_CALL};

        ਨਵਾਂ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ{&m_proc_ਗ੍ਰਾਫ, getNeedsProcDepVtx(funcp), getNeedsProcDepVtx(m_procp),
                        P_CALL};

        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstBegin* nodep) override {
        VL_RESTORER(m_procp);
        VL_RESTORER(m_underFork);

        ਜੇ (!m_underFork || (m_underFork & F_MIGHT_SUSPEND))
            ਨਵਾਂ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ{&m_susp_ਗ੍ਰਾਫ, getSuspendDepVtx(nodep), getSuspendDepVtx(m_procp),
                            m_underFork ? P_FORK : P_CALL};

        ਨਵਾਂ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ{&m_proc_ਗ੍ਰਾਫ, getNeedsProcDepVtx(nodep), getNeedsProcDepVtx(m_procp),
                        P_CALL};

        ਜੇ (m_underFork) ਜੋੜੋ_Flags(nodep, T_NEEDS_PROC | T_ALLOCS_PROC);

        m_procp = nodep;
        m_underFork = 0;
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstFork* nodep) override {
        UINFO(9, "suspendable-visit " << nodep);
        VL_RESTORER(m_underFork);

        v3Global.ਸੈੱਟ_ਵਰਤਦਾ_Timing();  // Even if there are no event controls, we have to set this flag
                                   // so that transformForks() in V3SchedTiming gets called and
                                   // removes all forks and begins
        ਜੇ (nodep->isTimingControl() && m_procp) {
            ਜੋੜੋ_Flags(m_procp, T_SUSPENDEE | T_SUSPENDER);
            m_underFork |= F_MIGHT_SUSPEND;
        }
        m_underFork |= F_MIGHT_NEED_PROC;
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਸੌਂਪੋ_Dly* nodep) override {
        ਜੇ (!VN_IS(m_procp, ਗੰਢ_Procedure)) v3Global.ਸੈੱਟ_ਵਰਤਦਾ_Timing();
        ਦੇਖੋ(static_cast<Ast_ਗੰਢ*>(nodep));
    }
    ਖਾਲੀ ਦੇਖੋ(AstFireEvent* nodep) override {
        ਜੇ (nodep->isDelayed()
            && (!VN_IS(m_procp, ਗੰਢ_Procedure) || !VN_IS(nodep->operandp(), VarRef))) {
            v3Global.ਸੈੱਟ_ਵਰਤਦਾ_Timing();
        }
        ਦੇਖੋ(static_cast<Ast_ਗੰਢ*>(nodep));
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਸੌਂਪੋ_W* nodep) override {
        ਜੇ (nodep->timingControlp()) v3Global.ਸੈੱਟ_ਵਰਤਦਾ_Timing();
        // Containing process will not suspend, don't mark it
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override {
        ਜੇ (nodep->isTimingControl()) {
            v3Global.ਸੈੱਟ_ਵਰਤਦਾ_Timing();
            ਜੇ (m_procp) ਜੋੜੋ_Flags(m_procp, T_SUSPENDEE | T_SUSPENDER | T_NEEDS_PROC);
        }
        iterateChildren(nodep);
    }

    //--------------------
    ਖਾਲੀ ਦੇਖੋ(AstVar*) override {}  // Accelerate

ਜਨਤਕ:
    // CONSTRUCTORS
    ਸਪਸ਼ਟ TimingSuspendableVisitor(AstNetlist* nodep) {
        iterate(nodep);
        m_susp_ਗ੍ਰਾਫ.remove_ਸੰਕ੍ਰਾਮਕ_ਕਿਨਾਰੇ();
        m_proc_ਗ੍ਰਾਫ.remove_ਸੰਕ੍ਰਾਮਕ_ਕਿਨਾਰੇ();
        // Propagate suspendability
        ਲਈ (V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼& vtx : m_susp_ਗ੍ਰਾਫ.vertices()) {
            DepVtx& depVtx = static_cast<DepVtx&>(vtx);
            ਜੇ (hasFlags(depVtx.nodep(), T_SUSPENDEE)) propagateFlags(&depVtx, T_SUSPENDEE);
        }
        ਜੇ (dump_ਗ੍ਰਾਫ_Level() >= 6) m_susp_ਗ੍ਰਾਫ.dumpDotFilePrefixed("timing_deps");

        // Propagate T_HAS_PROCESS
        ਲਈ (V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼& vtx : m_proc_ਗ੍ਰਾਫ.vertices()) {
            DepVtx& depVtx = static_cast<DepVtx&>(vtx);
            // Find processes that'll allocate VlProcess
            ਜੇ (hasFlags(depVtx.nodep(), T_FORCES_PROC)) {
                propagateFlagsIf(&depVtx, T_FORCES_PROC, [&](ਅਚਲ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ* e) -> ਤਾਰਕਿਕ {
                    ਵਾਪਸ !hasFlags(static_cast<DepVtx*>(e->fromp())->nodep(), T_ALLOCS_PROC);
                });
            }
            // Mark nodes on paths between processes and statements that use VlProcess
            ਜੇ (hasFlags(depVtx.nodep(), T_NEEDS_PROC)) {
                propagateFlagsIf(&depVtx, T_NEEDS_PROC, [&](ਅਚਲ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ* e) -> ਤਾਰਕਿਕ {
                    ਵਾਪਸ !hasFlags(static_cast<DepVtx*>(e->ਸਿਖਰ())->nodep(), T_ALLOCS_PROC);
                });
            }
        }
        ਲਈ (V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼& vtx : m_proc_ਗ੍ਰਾਫ.vertices()) {
            DepVtx& depVtx = static_cast<DepVtx&>(vtx);
            // Mark nodes that will be emitted with a VlProcess argument
            ਜੇ (hasFlags(depVtx.nodep(), T_ALLOCS_PROC | T_FORCES_PROC)) {
                ਜੋੜੋ_Flags(depVtx.nodep(), T_HAS_PROC);
                propagateFlagsReversedIf(&depVtx, T_HAS_PROC, [&](ਅਚਲ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ* e) -> ਤਾਰਕਿਕ {
                    ਵਾਪਸ hasFlags(static_cast<DepVtx*>(e->fromp())->nodep(), T_NEEDS_PROC);
                });
            }
        }
        ਜੇ (dump_ਗ੍ਰਾਫ_Level() >= 6) m_proc_ਗ੍ਰਾਫ.dumpDotFilePrefixed("proc_deps");
    }
    ~TimingSuspendableVisitor() override = ਮੂਲ;
};

// ######################################################################
//  Transform nodes affected by timing

ਵਰਗ TimingControlVisitor final : ਜਨਤਕ VNVisitor {
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
    AstNetlist* ਅਚਲ m_netlistp;  // Root node
    Ast_ਸਿਖਰ_Scope* ਅਚਲ m_ਸਿਖਰ_Scopep = m_netlistp->ਸਿਖਰ_Scopep();  // The AstTopScope
    AstScope* ਅਚਲ m_scopeTopp = m_ਸਿਖਰ_Scopep->scopep();  // Scope at the top
    AstClass* m_classp = ਖਾਲੀ_ਸੂਚਕ;  // Current class
    AstScope* m_scopep = ਖਾਲੀ_ਸੂਚਕ;  // Current scope
    Ast_ਸਰਗਰਮ* m_activep = ਖਾਲੀ_ਸੂਚਕ;  // Current active
    Ast_ਗੰਢ* m_procp = ਖਾਲੀ_ਸੂਚਕ;  // NodeProcedure/CFunc/Begin we're under
    ਤਾਰਕਿਕ m_hasProcess = ਝੂਠ;  // True if current scope has a VlProcess handle available
    ਪੂਰਨ m_forkCnt = 0;  // Number of forks inside a module
    ਤਾਰਕਿਕ m_under_Jump_ਖੰਡ = ਝੂਠ;  // True if we are inside of a jump-block
    ਤਾਰਕਿਕ m_underProcedure = ਝੂਠ;  // True if we are under an always or initial
    ਤਾਰਕਿਕ m_underIfaceCFunc = ਝੂਠ;  // True if we are under a CFunc owned by an interface scope
    ਤਾਰਕਿਕ m_hasStaticZeroDelay = ਝੂਠ;  // True if we have a static #0 delay
    std::ਸਦਿਸ਼<File_ਸਤਰ*> m_unknownDelayFlps;  // Locations of AstDelay with non-constant value

    // Unique names
    V3_Unique_ਨਾਂ m_dlyfork_ਨਾਂ{"__Vdlyfork"};  // Names for temp AssignW vars
    V3_Unique_ਨਾਂ m_intra_ਮੁੱਲ_ਨਾਂ{"__Vintraval"};  // Intra assign delay value var names
    V3_Unique_ਨਾਂ m_intra_ਸੂਚਕਾਂਕ_ਨਾਂ{"__Vintraidx"};  // Intra assign delay index var names
    V3_Unique_ਨਾਂ m_intra_Lsb_ਨਾਂ{"__Vintralsb"};  // Intra assign delay LSB var names
    V3_Unique_ਨਾਂ m_trig_Sched_ਨਾਂ{"__VtrigSched"};  // Trigger scheduler name generator
    V3_Unique_ਨਾਂ m_dyn_Trig_ਨਾਂ{"__VdynTrigger"};  // Dynamic trigger name generator
    // Module level temporary variables, shared by instances
    V3SharedTmps m_ਸੌਂਪੋ_WTmps{"__VassignWtmp", VVarType::MODULETEMP};  // Delayed AssignW values
    V3SharedTmps m_ਸੌਂਪੋ_WGens{"__VassignWgen", VVarType::MODULETEMP};  // AssignW generations

    // DTypes
    AstBasicDType* m_forkDtp = ਖਾਲੀ_ਸੂਚਕ;  // Fork variable type
    AstBasicDType* m_trigSchedDtp = ਖਾਲੀ_ਸੂਚਕ;  // Trigger scheduler type

    // Timing-related globals
    AstVarScope* m_delaySchedp = ਖਾਲੀ_ਸੂਚਕ;  // Global delay scheduler
    AstVarScope* m_dynamicSchedp = ਖਾਲੀ_ਸੂਚਕ;  // Global dynamic trigger scheduler
    Ast_Sen_ਰੁੱਖ* m_delaySensesp = ਖਾਲੀ_ਸੂਚਕ;  // Domain to trigger if a delayed coroutine is resumed
    Ast_Sen_ਰੁੱਖ* m_dynamicSensesp = ਖਾਲੀ_ਸੂਚਕ;  // Domain to trigger if a dynamic trigger is set

    // Other
    Sen_ਰੁੱਖ_Finder m_finder{m_netlistp};  // Sentree finder and uniquifier
    Sen_ਵਿਅੰਜਕ_Builder* m_sen_ਵਿਅੰਜਕ_Builderp = ਖਾਲੀ_ਸੂਚਕ;  // Sens expression builder for current m_scope

    // Stats
    ਮਾਪ_t m_statZeroDelays = 0;  // Number of statically known #0 delays
    ਮਾਪ_t m_statConstDelays = 0;  // Number of statically known #const (non-zero) delays
    ਮਾਪ_t m_statVariableDelays = 0;  // Number of delays with value unknown at compile time

    // METHODS
    // Transform an assignment with an intra timing control into a timing control with the
    // assignment under it
    Ast_ਗੰਢ* ਗੁਣਕ_ਬਾਹਰਲਾ_Timing_Control(Ast_ਗੰਢ_ਸੌਂਪੋ* nodep) ਅਚਲ {
        Ast_ਗੰਢ* stmtp = nodep;
        Ast_ਗੰਢ* ਅਚਲ controlp = nodep->timingControlp();
        ਜੇ (AstDelay* ਅਚਲ delayp = VN_CAST(controlp, Delay)) {
            stmtp->replaceWith(delayp->unlink_Fr_ਪਿੱਠ());
            delayp->ਜੋੜੋ_Stmtsp(stmtp);
            stmtp = delayp;
        } ਨਹੀਂਤਾਂ ਜੇ (Ast_Sen_ਰੁੱਖ* ਅਚਲ sentreep = VN_CAST(controlp, Sen_ਰੁੱਖ)) {
            AstEventControl* ਅਚਲ eventControlp
                = ਨਵਾਂ AstEventControl{sentreep->fileline(), sentreep->unlink_Fr_ਪਿੱਠ(), ਖਾਲੀ_ਸੂਚਕ};
            stmtp->replaceWith(eventControlp);
            eventControlp->ਜੋੜੋ_Stmtsp(stmtp);
            stmtp = eventControlp;
        } ਨਹੀਂਤਾਂ ਜੇ (AstBegin* ਅਚਲ beginp = VN_CAST(controlp, Begin)) {
            // Begin from V3AssertPre
            stmtp->replaceWith(beginp->unlink_Fr_ਪਿੱਠ());
            beginp->ਜੋੜੋ_Stmtsp(stmtp);
            stmtp = beginp;
        }
        ਵਾਪਸ stmtp == nodep ? ਖਾਲੀ_ਸੂਚਕ : stmtp;
    }
    ਸਥਿਰ ਤਾਰਕਿਕ staticallyNonZeroDelay(ਅਚਲ Ast_ਗੰਢ_ਵਿਅੰਜਕ* valuep) {
        ਜੇ (ਅਚਲ AstConst* ਅਚਲ constp = VN_CAST(valuep, Const)) ਵਾਪਸ !constp->isZero();
        ਜੇ (ਅਚਲ AstCond* ਅਚਲ condp = VN_CAST(valuep, Cond)) {
            ਵਾਪਸ staticallyNonZeroDelay(condp->thenp())
                   && staticallyNonZeroDelay(condp->elsep());
        }
        ਜੇ (ਅਚਲ AstMul* ਅਚਲ mulp = VN_CAST(valuep, Mul)) {
            ਅਚਲ AstConst* ਅਚਲ lhsConstp = VN_CAST(mulp->lhsp(), Const);
            ਅਚਲ AstConst* ਅਚਲ rhsConstp = VN_CAST(mulp->rhsp(), Const);
            ਜੇ (lhsConstp && !lhsConstp->isZero()) ਵਾਪਸ staticallyNonZeroDelay(mulp->rhsp());
            ਜੇ (rhsConstp && !rhsConstp->isZero()) ਵਾਪਸ staticallyNonZeroDelay(mulp->lhsp());
        }
        ਵਾਪਸ ਝੂਠ;
    }
    // Creates the global delay scheduler variable
    AstVarScope* getCreateDelayScheduler() {
        ਜੇ (m_delaySchedp) ਵਾਪਸ m_delaySchedp;
        ਸਵੈਚਾਲਿਤ* ਅਚਲ dlySchedDtp = ਨਵਾਂ AstBasicDType{
            m_scopeTopp->fileline(), VBasicDTypeKwd::DELAY_SCHEDULER, VSigning::UNSIGNED};
        m_netlistp->typeTablep()->ਜੋੜੋ_Typesp(dlySchedDtp);
        m_delaySchedp = m_ਸਿਖਰ_Scopep->create_ਆਰਜ਼ੀ("__VdlySched", dlySchedDtp);
        // Delay scheduler has to be accessible from top
        m_delaySchedp->varp()->sig_ਜਨਤਕ(ਸੱਚ);
        m_netlistp->delaySchedulerp(m_delaySchedp->varp());
        ਵਾਪਸ m_delaySchedp;
    }
    // Creates the delay sentree
    Ast_Sen_ਰੁੱਖ* get_Create_Delay_Sen_ਰੁੱਖ() {
        ਜੇ (m_delaySensesp) ਵਾਪਸ m_delaySensesp;
        File_ਸਤਰ* ਅਚਲ flp = m_scopeTopp->fileline();
        ਸਵੈਚਾਲਿਤ* ਅਚਲ awaitingCurrentTimep
            = ਨਵਾਂ AstCMethodHard{flp, ਨਵਾਂ AstVarRef{flp, getCreateDelayScheduler(), VAccess::READ},
                                 VCMethod::SCHED_AWAITING_CURRENT_ਸਮਾਂ};
        awaitingCurrentTimep->dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ();
        m_delaySensesp
            = ਨਵਾਂ Ast_Sen_ਰੁੱਖ{flp, ਨਵਾਂ Ast_Sen_ਵਸਤੂ{flp, VEdgeType::ET_TRUE, awaitingCurrentTimep}};
        m_netlistp->ਸਿਖਰ_Scopep()->ਜੋੜੋ_Sen_Treesp(m_delaySensesp);
        ਵਾਪਸ m_delaySensesp;
    }
    // Creates the global dynamic trigger scheduler variable
    AstVarScope* getCreateDynamicTriggerScheduler() {
        ਜੇ (m_dynamicSchedp) ਵਾਪਸ m_dynamicSchedp;
        ਸਵੈਚਾਲਿਤ* ਅਚਲ dynSchedDtp
            = ਨਵਾਂ AstBasicDType{m_scopeTopp->fileline(), VBasicDTypeKwd::DYNAMIC_TRIGGER_SCHEDULER,
                                VSigning::UNSIGNED};
        m_netlistp->typeTablep()->ਜੋੜੋ_Typesp(dynSchedDtp);
        m_dynamicSchedp = m_ਸਿਖਰ_Scopep->create_ਆਰਜ਼ੀ("__VdynSched", dynSchedDtp);
        ਵਾਪਸ m_dynamicSchedp;
    }
    // Creates the dynamic trigger sentree
    Ast_Sen_ਰੁੱਖ* get_Create_Dynamic_Trigger_Sen_ਰੁੱਖ() {
        ਜੇ (m_dynamicSensesp) ਵਾਪਸ m_dynamicSensesp;
        File_ਸਤਰ* ਅਚਲ flp = m_scopeTopp->fileline();
        ਸਵੈਚਾਲਿਤ* ਅਚਲ awaitingCurrentTimep = ਨਵਾਂ AstCMethodHard{
            flp, ਨਵਾਂ AstVarRef{flp, getCreateDynamicTriggerScheduler(), VAccess::READ},
            VCMethod::SCHED_EVALUATE};
        awaitingCurrentTimep->dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ();
        m_dynamicSensesp
            = ਨਵਾਂ Ast_Sen_ਰੁੱਖ{flp, ਨਵਾਂ Ast_Sen_ਵਸਤੂ{flp, VEdgeType::ET_TRUE, awaitingCurrentTimep}};
        m_netlistp->ਸਿਖਰ_Scopep()->ਜੋੜੋ_Sen_Treesp(m_dynamicSensesp);
        ਵਾਪਸ m_dynamicSensesp;
    }
    // Creates the event variable to trigger in NBA region
    AstEventControl* createNbaEventControl(File_ਸਤਰ* flp) {
        ਜੇ (!m_netlistp->nbaEventp()) {
            ਸਵੈਚਾਲਿਤ* ਅਚਲ nbaEventDtp = ਨਵਾਂ AstBasicDType{m_scopeTopp->fileline(),
                                                        VBasicDTypeKwd::EVENT, VSigning::UNSIGNED};
            m_netlistp->typeTablep()->ਜੋੜੋ_Typesp(nbaEventDtp);
            m_netlistp->nbaEventp(m_ਸਿਖਰ_Scopep->create_ਆਰਜ਼ੀ("__VnbaEvent", nbaEventDtp));
            v3Global.ਸੈੱਟ_Has_Events();
        }
        ਵਾਪਸ ਨਵਾਂ AstEventControl{
            flp,
            ਨਵਾਂ Ast_Sen_ਰੁੱਖ{
                flp, ਨਵਾਂ Ast_Sen_ਵਸਤੂ{flp, VEdgeType::ET_EVENT,
                                    ਨਵਾਂ AstVarRef{flp, m_netlistp->nbaEventp(), VAccess::READ}}},
            ਖਾਲੀ_ਸੂਚਕ};
    }
    // Creates the variable that, if set, causes the NBA event to be triggered
    Ast_ਸੌਂਪੋ* createNbaEventTriggerAssignment(File_ਸਤਰ* flp) {
        ਜੇ (!m_netlistp->nbaEventTriggerp()) {
            m_netlistp->nbaEventTriggerp(m_ਸਿਖਰ_Scopep->create_ਆਰਜ਼ੀ("__VnbaEventTrigger", 1));
        }
        ਵਾਪਸ ਨਵਾਂ Ast_ਸੌਂਪੋ{flp,
                             ਨਵਾਂ AstVarRef{flp, m_netlistp->nbaEventTriggerp(), VAccess::WRITE},
                             ਨਵਾਂ AstConst{flp, AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_True{}}};
    }
    // Returns true if we are under a class or interface function, or the tree references locals.
    // These are cases where static, globally-evaluated triggers are not suitable.
    ਤਾਰਕਿਕ needDynamicTrigger(Ast_ਗੰਢ* ਅਚਲ nodep) ਅਚਲ {
        ਵਾਪਸ m_classp || m_underIfaceCFunc || nodep->exists([](Ast_ਗੰਢ* ਅਚਲ nodep) {
            ਜੇ (Ast_ਗੰਢ_Var_Ref* varp = VN_CAST(nodep, ਗੰਢ_Var_Ref)) {
                ਵਾਪਸ varp->varp()->isFuncLocal();
            }
            ਵਾਪਸ !nodep->is_ਸ਼ੁੱਧ();
        });
    }
    // Creates a trigger scheduler variable
    AstVarScope* getCreateTriggerSchedulerp(Ast_Sen_ਰੁੱਖ* ਅਚਲ sentreep) {
        ਜੇ (!sentreep->user1p()) {
            ਜੇ (!m_trigSchedDtp) {
                m_trigSchedDtp
                    = ਨਵਾਂ AstBasicDType{m_scopeTopp->fileline(), VBasicDTypeKwd::TRIGGER_SCHEDULER,
                                        VSigning::UNSIGNED};
                m_netlistp->typeTablep()->ਜੋੜੋ_Typesp(m_trigSchedDtp);
            }
            AstVarScope* ਅਚਲ trigSchedp
                = m_ਸਿਖਰ_Scopep->create_ਆਰਜ਼ੀ(m_trig_Sched_ਨਾਂ.get(sentreep), m_trigSchedDtp);
            sentreep->user1p(trigSchedp);
        }
        ਵਾਪਸ VN_AS(sentreep->user1p(), VarScope);
    }
    // Creates a string describing the sentree
    AstCExpr* createEventDescription(Ast_Sen_ਰੁੱਖ* ਅਚਲ sentreep) ਅਚਲ {
        ਜੇ (!sentreep->user2p()) {
            std::stringstream ss;
            V3EmitV::verilog_For_ਰੁੱਖ(sentreep, ss);
            // possibly a multiline string
            // Names were hashed by V3Name, show the user the original text
            std::string comment = VName::dehash(ss.str());
            std::replace(comment.begin(), comment.ਸਿਰਾ(), '\n', ' ');
            // Escape quotes, e.g. from string-indexed event array selects
            comment = '"' + V3_ਬਾਹਰਲਾ_Formatter::quoteNameControls(comment) + '"';
            AstCExpr* ਅਚਲ commentp = ਨਵਾਂ AstCExpr{sentreep->fileline(), comment};
            commentp->dtype_ਸੈੱਟ_String();
            sentreep->user2p(commentp);
            ਵਾਪਸ commentp;
        }
        ਵਾਪਸ VN_AS(sentreep->user2p(), CExpr)->clone_ਰੁੱਖ(ਝੂਠ);
    }
    // Adds debug info to a hardcoded method call
    ਖਾਲੀ ਜੋੜੋ_Debug_Info(AstCMethodHard* ਅਚਲ methodp) ਅਚਲ {
        File_ਸਤਰ* ਅਚਲ flp = methodp->fileline();
        ਅਚਲ ਤਾਰਕਿਕ protectIds = v3Global.opt.protectIds();
        AstCExpr* ਅਚਲ ap
            = ਨਵਾਂ AstCExpr{flp, protectIds ? "VL_UNKNOWN" : '"' + flp->filenameEsc() + '"'};
        ap->dtype_ਸੈੱਟ_String();
        methodp->ਜੋੜੋ_Pinsp(ap);
        AstCExpr* ਅਚਲ bp = ਨਵਾਂ AstCExpr{flp, protectIds ? "0" : cvt_ਮੰਜ਼ਿਲ_Str(flp->lineno())};
        bp->dtype_ਸੈੱਟ_String();
        methodp->ਜੋੜੋ_Pinsp(bp);
    }
    // Adds debug info to a trigSched.trigger() call
    ਖਾਲੀ ਜੋੜੋ_Event_Debug_Info(AstCMethodHard* ਅਚਲ methodp, Ast_Sen_ਰੁੱਖ* ਅਚਲ sentreep) ਅਚਲ {
        ਜੇ (v3Global.opt.protectIds()) {
            File_ਸਤਰ* ਅਚਲ flp = sentreep->fileline();
            AstCExpr* ਅਚਲ descp = ਨਵਾਂ AstCExpr{flp, "VL_UNKNOWN"};
            descp->dtype_ਸੈੱਟ_String();
            methodp->ਜੋੜੋ_Pinsp(descp);
            ਜੋੜੋ_Debug_Info(methodp);
            ਵਾਪਸ;
        }
        methodp->ਜੋੜੋ_Pinsp(createEventDescription(sentreep));
        ਜੋੜੋ_Debug_Info(methodp);
    }
    // Adds process pointer to a hardcoded method call
    ਖਾਲੀ ਜੋੜੋ_Process_Info(AstCMethodHard* ਅਚਲ methodp) ਅਚਲ {
        File_ਸਤਰ* ਅਚਲ flp = methodp->fileline();
        AstCExpr* ਅਚਲ ap
            = ਨਵਾਂ AstCExpr{flp, (m_procp && m_hasProcess) ? "vlProcess" : "nullptr"};
        methodp->ਜੋੜੋ_Pinsp(ap);
    }
    // Creates the fork handle type and returns it
    AstBasicDType* getCreateForkSyncDTypep() {
        ਜੇ (m_forkDtp) ਵਾਪਸ m_forkDtp;
        m_forkDtp = ਨਵਾਂ AstBasicDType{m_scopeTopp->fileline(), VBasicDTypeKwd::FORK_SYNC,
                                      VSigning::UNSIGNED};
        m_netlistp->typeTablep()->ਜੋੜੋ_Typesp(m_forkDtp);
        ਵਾਪਸ m_forkDtp;
    }
    // Move `nodep` into `AstCLocalScope`.
    AstCLocalScope* ਜੋੜੋ_CLocal_Scope(File_ਸਤਰ* ਅਚਲ flp, Ast_ਗੰਢ* ਅਚਲ nodep) ਅਚਲ {
        ਜੇ (!nodep) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
        VNRelinker handle;
        nodep->unlink_Fr_ਪਿੱਠ(&handle);
        AstCLocalScope* ਅਚਲ cscopep = ਨਵਾਂ AstCLocalScope{flp, nodep};
        handle.relink(cscopep);
        ਵਾਪਸ cscopep;
    }
    // Create a temp variable and optionally put it before the specified node (mark local if so)
    AstVarScope* create_ਆਰਜ਼ੀ(File_ਸਤਰ* ਅਚਲ flp, ਅਚਲ std::string& name,
                            Ast_ਗੰਢ_DType* ਅਚਲ dtypep, Ast_ਗੰਢ* ਅਚਲ ਪਾਓ_Beforep = ਖਾਲੀ_ਸੂਚਕ) {
        AstVar* varp;
        ਜੇ (ਪਾਓ_Beforep) {
            varp = ਨਵਾਂ AstVar{flp, VVarType::BLOCKTEMP, name, dtypep};
            varp->funcLocal(ਸੱਚ);
            ਪਾਓ_Beforep->ਜੋੜੋ_ਇੱਥੇ_This_As_ਅਗਲਾ(varp);
        } ਨਹੀਂਤਾਂ {
            varp = ਨਵਾਂ AstVar{flp, VVarType::MODULETEMP, name, dtypep};
            m_scopep->modp()->ਜੋੜੋ_Stmtsp(varp);
        }
        AstVarScope* ਅਚਲ vscp = ਨਵਾਂ AstVarScope{flp, m_scopep, varp};
        m_scopep->ਜੋੜੋ_Varsp(vscp);
        ਵਾਪਸ vscp;
    }
    // Add a done() call on the fork sync
    ਖਾਲੀ ਜੋੜੋ_Fork_ਪੂਰਾ(AstBegin* ਅਚਲ beginp, AstVarScope* ਅਚਲ forkVscp) ਅਚਲ {
        File_ਸਤਰ* ਅਚਲ flp = beginp->fileline();
        ਸਵੈਚਾਲਿਤ* ਅਚਲ donep = ਨਵਾਂ AstCMethodHard{
            beginp->fileline(), ਨਵਾਂ AstVarRef{flp, forkVscp, VAccess::WRITE}, VCMethod::FORK_ਪੂਰਾ};
        donep->dtype_ਸੈੱਟ_Void();
        ਜੋੜੋ_Debug_Info(donep);
        beginp->ਜੋੜੋ_Stmtsp(donep->makeStmt());
    }
    ਸਥਿਰ ਤਾਰਕਿਕ has_Disable_ਕਤਾਰ_ਧੱਕੋ_Self_ਅਗੇਤਰ(AstBegin* ਅਚਲ beginp) {
        // LinkJump prepends disable-by-name registration as:
        //   __VprocessQueue_*.push_back(std::process::self())
        // V3LiftExpr lifts std::process::self() into an assignment to a temporary, then V3Task
        // lowers that assignment's function call into a leading comment and AstStmtExpr. Thus the
        // push_back is no longer necessarily the first statement. Scan across this generated
        // prefix for it, stopping at the fork-start sentinel or the branch body. Registrations for
        // nested forks live in sub-blocks and so are not in this statement list.
        ਲਈ (Ast_ਗੰਢ* stmtp = beginp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ਜੇ (VN_IS(stmtp, Comment)) ਜਾਰੀ;
            Ast_Stmt_ਵਿਅੰਜਕ* ਅਚਲ stmtExprp = VN_CAST(stmtp, Stmt_ਵਿਅੰਜਕ);
            ਜੇ (!stmtExprp) ਰੋਕੋ;
            ਜੇ (stmtExprp->is_Disable_ਕਤਾਰ_ਧੱਕੋ_Self_Stmt()) ਵਾਪਸ ਸੱਚ;
        }
        ਵਾਪਸ ਝੂਠ;
    }
    // Register a callback so killing a process-backed fork branch decrements the join counter
    ਖਾਲੀ ਜੋੜੋ_Fork_On_Kill(AstBegin* ਅਚਲ beginp, AstVarScope* ਅਚਲ forkVscp) ਅਚਲ {
        ਜੇ (!beginp->needProcess() && !has_Disable_ਕਤਾਰ_ਧੱਕੋ_Self_ਅਗੇਤਰ(beginp)) ਵਾਪਸ;
        File_ਸਤਰ* ਅਚਲ flp = beginp->fileline();
        AstCMethodHard* ਅਚਲ onKillp = ਨਵਾਂ AstCMethodHard{
            flp, ਨਵਾਂ AstVarRef{flp, forkVscp, VAccess::WRITE}, VCMethod::FORK_ON_KILL};
        onKillp->dtype_ਸੈੱਟ_Void();
        AstCExpr* ਅਚਲ processp = ਨਵਾਂ AstCExpr{flp, "vlProcess"};
        processp
            ->dtype_ਸੈੱਟ_Void();  // Opaque process reference; type is irrelevant for hardcoded emit
        onKillp->ਜੋੜੋ_Pinsp(processp);
        Ast_ਗੰਢ_Stmt* ਅਚਲ stmtp = onKillp->makeStmt();
        beginp->prependStmtsp(stmtp);
    }
    // Handle the 'join' part of a fork..join
    ਖਾਲੀ makeForkJoin(AstFork* ਅਚਲ forkp) {
        // Create a fork sync var
        File_ਸਤਰ* ਅਚਲ flp = forkp->fileline();
        // Insert the sync var directly before the fork
        Ast_ਗੰਢ* ਅਚਲ ਪਾਓ_Beforep = forkp;
        // Make sure all references to the fork var are contained within a single scope to prevent
        // moving statements accessing it in case of large functions splitting.
        ਜੋੜੋ_CLocal_Scope(flp, ਪਾਓ_Beforep);
        AstVarScope* forkVscp
            = create_ਆਰਜ਼ੀ(flp, forkp->name() + "__sync", getCreateForkSyncDTypep(), ਪਾਓ_Beforep);
        ਬਿਨਾਨਿਸ਼ਾਨੀ join_ਗਿਣਤੀ = 0;  // Needed for join counter
        // Add a <fork sync>.done() to each begin
        ਲਈ (Ast_ਗੰਢ* beginp = forkp->forksp(); beginp; beginp = beginp->nextp()) {
            ਜੋੜੋ_Fork_On_Kill(VN_AS(beginp, Begin), forkVscp);
            ਜੋੜੋ_Fork_ਪੂਰਾ(VN_AS(beginp, Begin), forkVscp);
            join_ਗਿਣਤੀ++;
        }
        ਜੇ (forkp->joinType().joinAny()) join_ਗਿਣਤੀ = 1;
        // Set the join counter
        ਸਵੈਚਾਲਿਤ* ਅਚਲ initp = ਨਵਾਂ AstCMethodHard{flp, ਨਵਾਂ AstVarRef{flp, forkVscp, VAccess::WRITE},
                                               VCMethod::FORK_INIT, ਨਵਾਂ AstConst{flp, join_ਗਿਣਤੀ}};
        initp->dtype_ਸੈੱਟ_Void();
        ਜੋੜੋ_Process_Info(initp);
        forkp->ਜੋੜੋ_ਇੱਥੇ_This_As_ਅਗਲਾ(initp->makeStmt());
        // Await the join at the end
        ਸਵੈਚਾਲਿਤ* ਅਚਲ joinp = ਨਵਾਂ AstCMethodHard{flp, ਨਵਾਂ AstVarRef{flp, forkVscp, VAccess::WRITE},
                                               VCMethod::FORK_JOIN};
        joinp->dtype_ਸੈੱਟ_Void();
        ਜੋੜੋ_Process_Info(joinp);
        ਜੋੜੋ_Debug_Info(joinp);
        forkp->ਜੋੜੋ_ਅਗਲਾ_ਇੱਥੇ(ਨਵਾਂ AstCAwait{flp, joinp});
    }

    // `procp` shall be a NodeProcedure/CFunc/Begin and within it vars from `varsp` will be placed.
    // `varsp` vector of vars which shall be localized.
    ਸਥਿਰ ਖਾਲੀ localizeVars(Ast_ਗੰਢ* ਅਚਲ procp, ਅਚਲ std::ਸਦਿਸ਼<AstVar*>& varsp) {
        UASSERT(procp, "procp is nullptr");
        AstVar* newVarsp = ਖਾਲੀ_ਸੂਚਕ;
        ਲਈ (AstVar* ਅਚਲ varp : varsp) {
            varp->funcLocal(ਸੱਚ);
            newVarsp = Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ_Null(newVarsp, varp->unlink_Fr_ਪਿੱਠ());
        }
        ਜੇ (Ast_ਗੰਢ_Procedure* ਅਚਲ ਗੰਢ_Procp = VN_CAST(procp, ਗੰਢ_Procedure)) {
            ਗੰਢ_Procp->prependStmtsp(newVarsp);
        } ਨਹੀਂਤਾਂ ਜੇ (AstCFunc* ਅਚਲ cfuncp = VN_CAST(procp, CFunc)) {
            cfuncp->prependVarsp(newVarsp);
        } ਨਹੀਂਤਾਂ ਜੇ (AstBegin* ਅਚਲ beginp = VN_CAST(procp, Begin)) {
            beginp->prependStmtsp(newVarsp);
        } ਨਹੀਂਤਾਂ {
            procp->v3fatalSrc(
                procp->prettyNameQ()
                << " is not of an expected type NodeProcedure/CFunc/Begin instead it is: "
                << procp->prettyTypeName());
        }
    }

    Ast_ਗੰਢ_ਵਿਅੰਜਕ* delay_ਮੰਜ਼ਿਲ_Integral(AstDelay* nodep, Ast_ਗੰਢ_ਵਿਅੰਜਕ* valuep) {
        ਅਚਲ AstConst* ਅਚਲ constp = VN_CAST(valuep, Const);
        // The fork start sentinel is already in ticks
        ਜੇ (constp && constp->ਮੰਜ਼ਿਲ_UQuad() == std::numeric_limits<uint64_t>::max()) ਵਾਪਸ valuep;
        ਵਾਪਸ V3Timing::delay_ਮੰਜ਼ਿਲ_Ticks(valuep, nodep->timeunit());
    }

    // VISITORS
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Module* nodep) override {
        UASSERT_OBJ(!m_classp, nodep, "Module or class under class");
        VL_RESTORER(m_classp);
        m_classp = VN_CAST(nodep, Class);
        VL_RESTORER(m_forkCnt);
        m_forkCnt = 0;
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstScope* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        Sen_ਵਿਅੰਜਕ_Builder sen_ਵਿਅੰਜਕ_Builder{m_scopep};
        {  // Restore m_senExprBuilderp before destroying senExprBuilder
            VL_RESTORER(m_sen_ਵਿਅੰਜਕ_Builderp);
            m_sen_ਵਿਅੰਜਕ_Builderp = &sen_ਵਿਅੰਜਕ_Builder;
            iterateChildren(nodep);
        }
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਸਰਗਰਮ* nodep) override {
        VL_RESTORER(m_activep);
        m_activep = nodep;
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Procedure* nodep) override {
        VL_RESTORER(m_procp);
        VL_RESTORER(m_hasProcess);
        m_procp = nodep;
        m_hasProcess = hasFlags(nodep, T_HAS_PROC);
        VL_RESTORER(m_underProcedure);
        m_underProcedure = ਸੱਚ;
        iterateChildren(nodep);
        ਜੇ (hasFlags(nodep, T_SUSPENDEE)) nodep->ਸੈੱਟ_Suspendable();
        ਜੇ (hasFlags(nodep, T_HAS_PROC)) nodep->ਸੈੱਟ_Need_Process();
    }
    ਖਾਲੀ ਦੇਖੋ(AstInitial* nodep) override {
        ਦੇਖੋ(static_cast<Ast_ਗੰਢ_Procedure*>(nodep));
        ਜੇ (nodep->needProcess() && !nodep->user1_ਸੈੱਟ_Once()) {
            nodep->ਜੋੜੋ_Stmtsp(
                ਨਵਾਂ AstCStmt{nodep->fileline(), "vlProcess->state(VlProcess::FINISHED);"});
        }
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_Jump_ਖੰਡ* nodep) override {
        VL_RESTORER(m_under_Jump_ਖੰਡ);
        m_under_Jump_ਖੰਡ = ਸੱਚ;
        ਦੇਖੋ(static_cast<Ast_ਗੰਢ_Stmt*>(nodep));
    }
    ਖਾਲੀ ਦੇਖੋ(AstAlways* nodep) override {
        ਜੇ (nodep->user1_ਸੈੱਟ_Once()) ਵਾਪਸ;
        VL_RESTORER(m_procp);
        VL_RESTORER(m_hasProcess);
        m_procp = nodep;
        m_hasProcess = hasFlags(nodep, T_HAS_PROC);
        VL_RESTORER(m_underProcedure);
        m_underProcedure = ਸੱਚ;
        // Workaround for killing `always` processes (doing that is pretty much UB)
        // TODO: Disallow killing `always` at runtime (throw an error)
        // Skip for combinational blocks; if the body has timing controls
        // iterateChildren will add T_SUSPENDEE, otherwise it would spin.
        ਜੇ (hasFlags(nodep, T_HAS_PROC)
            && !(m_activep && m_activep->sentreep() && m_activep->sentreep()->hasCombo()))
            ਜੋੜੋ_Flags(nodep, T_SUSPENDEE);

        iterateChildren(nodep);
        ਜੇ (hasFlags(nodep, T_HAS_PROC)) nodep->ਸੈੱਟ_Need_Process();
        ਜੇ (!hasFlags(nodep, T_SUSPENDEE)) ਵਾਪਸ;
        nodep->ਸੈੱਟ_Suspendable();
        File_ਸਤਰ* ਅਚਲ flp = nodep->fileline();
        Ast_Sen_ਰੁੱਖ* ਅਚਲ sentreep = m_activep->sentreep();
        ਜੇ (sentreep->hasClocked()) {
            Ast_ਗੰਢ* ਅਚਲ bodysp = nodep->stmtsp()->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ();
            ਸਵੈਚਾਲਿਤ* ਅਚਲ controlp = ਨਵਾਂ AstEventControl{flp, sentreep->clone_ਰੁੱਖ(ਝੂਠ), bodysp};
            nodep->ਜੋੜੋ_Stmtsp(controlp);
            iterate(controlp);
        }
        // Note: The 'while (true)' outer loop will be added in V3Sched
        ਸਵੈਚਾਲਿਤ* ਅਚਲ activep = ਨਵਾਂ Ast_ਸਰਗਰਮ{
            flp, "", ਨਵਾਂ Ast_Sen_ਰੁੱਖ{flp, ਨਵਾਂ Ast_Sen_ਵਸਤੂ{flp, Ast_Sen_ਵਸਤੂ::Initial{}}}};
        activep->sen_ਰੁੱਖ_Storep(activep->sentreep());
        activep->ਜੋੜੋ_Stmtsp(nodep->unlink_Fr_ਪਿੱਠ());
        m_activep->ਜੋੜੋ_ਅਗਲਾ_ਇੱਥੇ(activep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstCFunc* nodep) override {
        VL_RESTORER(m_procp);
        VL_RESTORER(m_hasProcess);
        VL_RESTORER(m_underIfaceCFunc);
        m_procp = nodep;
        m_hasProcess = hasFlags(nodep, T_HAS_PROC);
        m_underIfaceCFunc = VN_IS(m_scopep->modp(), Iface);
        iterateChildren(nodep);
        ਜੇ (hasFlags(nodep, T_HAS_PROC)) nodep->ਸੈੱਟ_Need_Process();
        ਜੇ (!(hasFlags(nodep, T_SUSPENDEE))) ਵਾਪਸ;

        nodep->rtnType("VlCoroutine");
        // If in a class, create a shared pointer to 'this'
        ਜੇ (m_classp) {
            AstCStmt* ਅਚਲ cstmtp = ਨਵਾਂ AstCStmt{nodep->fileline(), "VL_KEEP_THIS;"};
            nodep->prependStmtsp(cstmtp);
        }
        ਜੇ (nodep->dpiExportImpl()) {
            nodep->exists([](AstCAwait* ਅਚਲ awaitp) -> ਤਾਰਕਿਕ {
                // A DPI-exported coroutine won't be able to block the calling code
                // Error on the await node; fall back to the function node
                awaitp->v3warn(E_UNSUPPORTED,
                               "Unsupported: Timing controls inside DPI-exported tasks");
                ਵਾਪਸ ਸੱਚ;
            });
        }
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_CCall* nodep) override {
        AstCFunc* ਅਚਲ funcp = nodep->funcp();

        // Skip automatic covergroup sampling calls
        ਜੇ (funcp->isCovergroupSample()) {
            iterateChildren(nodep);
            ਵਾਪਸ;
        }

        ਜੇ (funcp->needProcess()) m_hasProcess = ਸੱਚ;
        ਜੇ (hasFlags(funcp, T_SUSPENDEE) && !nodep->user1_ਸੈੱਟ_Once()) {  // If suspendable
            // Calls to suspendables are always void return type, hence parent must be StmtExpr
            Ast_Stmt_ਵਿਅੰਜਕ* ਅਚਲ stmtp = VN_AS(nodep->backp(), Stmt_ਵਿਅੰਜਕ);
            stmtp->replaceWith(ਨਵਾਂ AstCAwait{nodep->fileline(), nodep->unlink_Fr_ਪਿੱਠ()});
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(stmtp), stmtp);
        }
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstDelay* nodep) override {
        UASSERT_OBJ(!nodep->is_ਚੱਕਰ_Delay(), nodep,
                    "Cycle delays should have been handled in V3AssertPre");
        File_ਸਤਰ* ਅਚਲ flp = nodep->fileline();

        Ast_ਗੰਢ_ਵਿਅੰਜਕ* valuep = delay_ਮੰਜ਼ਿਲ_Integral(nodep, nodep->lhsp()->unlink_Fr_ਪਿੱਠ());

        // Statistics
        ਜੇ (valuep->isZero()) {
            ++m_statZeroDelays;
        } ਨਹੀਂਤਾਂ ਜੇ (VN_IS(valuep, Const)) {
            ++m_statConstDelays;
        } ਨਹੀਂਤਾਂ {
            ++m_statVariableDelays;
        }

        // Decide scheduling support for #0
        ਜੇ (v3Global.opt.schedZeroDelay().is_ਸੈੱਟ_True()) {
            // User said to schedule for #0 support, nothing else to do
            v3Global.ਸੈੱਟ_ਵਰਤਦਾ_Zero_Delay();
        } ਨਹੀਂਤਾਂ ਜੇ (v3Global.opt.schedZeroDelay().is_ਸੈੱਟ_False()) {
            // User said to schedule without #0 support. Still warn if a static #0 delay exists
            ਜੇ (valuep->isZero()) {
                nodep->v3warn(
                    ZERODLY,
                    "Static #0 delay exists, but '--no-sched-zero-delay' was given.\n"
                        << nodep->warn_ਹੋਰ()  //
                        << "... Can proceed, but this will fail at runtime if executed.");
            }
        } ਨਹੀਂਤਾਂ {
            // User did not express preference, decide based on presence of delays
            ਜੇ (valuep->isZero()) {
                // Statically known #0 delay exists, schedule for #0 support
                v3Global.ਸੈੱਟ_ਵਰਤਦਾ_Zero_Delay();
                m_hasStaticZeroDelay = ਸੱਚ;
                // Don't warn on variable delays, as no point
                m_unknownDelayFlps.clear();
            } ਨਹੀਂਤਾਂ ਜੇ (staticallyNonZeroDelay(valuep)) {
                // Delay is dynamic, but every statically known outcome is non-zero.
                // So we don't need #0 delay support and there should be no warning.
            } ਨਹੀਂਤਾਂ ਜੇ (!VN_IS(valuep, Const)) {
                // Delay is not known at compiile time. Conservatively schedule for #0 support,
                // but warn if no static #0 delays used as performance might be improved
                v3Global.ਸੈੱਟ_ਵਰਤਦਾ_Zero_Delay();
                ਜੇ (!m_hasStaticZeroDelay) m_unknownDelayFlps.ਧੱਕੋ_ਪਿੱਠ(nodep->fileline());
            }
        }

        // Replace self with a 'co_await dlySched.delay(<valuep>)'
        AstCMethodHard* ਅਚਲ delayMethodp = ਨਵਾਂ AstCMethodHard{
            flp, ਨਵਾਂ AstVarRef{flp, getCreateDelayScheduler(), VAccess::WRITE},
            VCMethod::SCHED_DELAY, valuep};
        delayMethodp->dtype_ਸੈੱਟ_Void();
        ਜੋੜੋ_Process_Info(delayMethodp);
        ਜੋੜੋ_Debug_Info(delayMethodp);
        // Create the co_await
        Ast_ਗੰਢ* ਅਚਲ awaitp = ਨਵਾਂ AstCAwait{flp, delayMethodp, get_Create_Delay_Sen_ਰੁੱਖ()};
        // Relink child statements after the co_await
        ਜੇ (Ast_ਗੰਢ* ਅਚਲ stmtsp = nodep->stmtsp()) {
            awaitp->ਜੋੜੋ_ਅਗਲਾ(stmtsp->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ());
        }
        nodep->replaceWith(awaitp);
        VL_DO_DANGLING(nodep->delete_ਰੁੱਖ(), nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstEventControl* nodep) override {
        // Do not allow waiting on local named events, as they get enqueued for clearing, but can
        // go out of scope before that happens
        ਜੇ (!nodep->sentreep()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Event control with implicit sensitivity (@*)"
                          " in this context; use an explicit sensitivity list instead");
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep->unlink_Fr_ਪਿੱਠ()), nodep);
            ਵਾਪਸ;
        }
        File_ਸਤਰ* ਅਚਲ flp = nodep->fileline();
        // Relink child statements after the event control
        ਜੇ (nodep->stmtsp()) nodep->ਜੋੜੋ_ਅਗਲਾ_ਇੱਥੇ(nodep->stmtsp()->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ());
        ਜੇ (needDynamicTrigger(nodep->sentreep())) {
            // Create the trigger variable and init it with 0
            AstVarScope* ਅਚਲ trigvscp
                = create_ਆਰਜ਼ੀ(flp, m_dyn_Trig_ਨਾਂ.get(nodep), nodep->ਲੱਭੋ_ਦੋਆਧਾਰੀ_ਅੰਕ_DType(), nodep);
            ਸਵੈਚਾਲਿਤ* ਅਚਲ initp = ਨਵਾਂ Ast_ਸੌਂਪੋ{flp, ਨਵਾਂ AstVarRef{flp, trigvscp, VAccess::WRITE},
                                              ਨਵਾਂ AstConst{flp, AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_False{}}};
            // Await the eval step with the dynamic trigger scheduler. First, create the method
            // call
            ਸਵੈਚਾਲਿਤ* ਅਚਲ evalMethodp = ਨਵਾਂ AstCMethodHard{
                flp, ਨਵਾਂ AstVarRef{flp, getCreateDynamicTriggerScheduler(), VAccess::WRITE},
                VCMethod::SCHED_EVALUATION};
            evalMethodp->dtype_ਸੈੱਟ_Void();
            ਜੋੜੋ_Process_Info(evalMethodp);
            ਸਵੈਚਾਲਿਤ* ਅਚਲ sentreep = nodep->sentreep();
            ਜੋੜੋ_Event_Debug_Info(evalMethodp, sentreep);
            // Create the co_await
            AstCAwait* ਅਚਲ awaitEvalp
                = ਨਵਾਂ AstCAwait{flp, evalMethodp, get_Create_Dynamic_Trigger_Sen_ਰੁੱਖ()};
            // Construct the sen expression for this sentree
            UASSERT_OBJ(m_sen_ਵਿਅੰਜਕ_Builderp, nodep, "No SenExprBuilder for this scope");
            ਸਵੈਚਾਲਿਤ* ਅਚਲ assignp = ਨਵਾਂ Ast_ਸੌਂਪੋ{flp, ਨਵਾਂ AstVarRef{flp, trigvscp, VAccess::WRITE},
                                                m_sen_ਵਿਅੰਜਕ_Builderp->build(sentreep).ਪਹਿਲਾ};
            // Get the SenExprBuilder results
            ਅਚਲ Sen_ਵਿਅੰਜਕ_Builder::Results senResults = m_sen_ਵਿਅੰਜਕ_Builderp->getAndClearResults();
            localizeVars(m_procp, senResults.m_vars);
            // If post updates are destructive (e.g. clearFired on events), perform a
            // conservative pre-clear once before entering the wait loop so stale state from a
            // previous wait does not cause an immediate false-positive trigger.
            ਅਚਲ ਤਾਰਕਿਕ hasDestructivePostUpdates = !senResults.m_destructivePostUpdates.ਖਾਲੀ_ਥਾਵਾਂ();
            // Create the trigger eval loop, which will await the evaluation step and check the
            // trigger
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ condp
                = ਨਵਾਂ AstLogNot{flp, ਨਵਾਂ AstVarRef{flp, trigvscp, VAccess::READ}};
            AstLoop* ਅਚਲ loopp = ਨਵਾਂ AstLoop{flp};
            loopp->ਜੋੜੋ_Stmtsp(ਨਵਾਂ AstLoopTest{flp, loopp, condp});
            loopp->ਜੋੜੋ_Stmtsp(awaitEvalp);
            // Put pre updates before the trigger check and assignment
            ਲਈ (Ast_ਗੰਢ_Stmt* ਅਚਲ stmtp : senResults.m_preUpdates) loopp->ਜੋੜੋ_Stmtsp(stmtp);
            // Then the trigger check and assignment
            loopp->ਜੋੜੋ_Stmtsp(assignp);
            // Let the dynamic trigger scheduler know if this trigger was set
            // If it was, a call to the scheduler's evaluate() will return true
            AstCMethodHard* ਅਚਲ anyTriggeredMethodp = ਨਵਾਂ AstCMethodHard{
                flp, ਨਵਾਂ AstVarRef{flp, getCreateDynamicTriggerScheduler(), VAccess::WRITE},
                VCMethod::SCHED_ANY_TRIGGERED, ਨਵਾਂ AstVarRef{flp, trigvscp, VAccess::READ}};
            anyTriggeredMethodp->dtype_ਸੈੱਟ_Void();
            loopp->ਜੋੜੋ_Stmtsp(anyTriggeredMethodp->makeStmt());
            // If the post update is destructive (e.g. event vars are cleared), create an await for
            // the post update step
            ਜੇ (hasDestructivePostUpdates) {
                AstCAwait* ਅਚਲ awaitPostUpdatep = awaitEvalp->clone_ਰੁੱਖ(ਝੂਠ);
                VN_AS(awaitPostUpdatep->exprp(), CMethodHard)->ਵਿਧੀ(VCMethod::SCHED_POST_UPDATE);
                loopp->ਜੋੜੋ_Stmtsp(awaitPostUpdatep);
            }
            // Put the post updates at the end of the loop
            ਲਈ (Ast_ਗੰਢ_Stmt* ਅਚਲ stmtp : senResults.m_postUpdates) loopp->ਜੋੜੋ_Stmtsp(stmtp);
            // Finally, await the resumption step in 'act'
            AstCAwait* ਅਚਲ awaitResumep = awaitEvalp->clone_ਰੁੱਖ(ਝੂਠ);
            VN_AS(awaitResumep->exprp(), CMethodHard)->ਵਿਧੀ(VCMethod::SCHED_RESUMPTION);
            Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ<Ast_ਗੰਢ_Stmt, Ast_ਗੰਢ_Stmt>(loopp, awaitResumep);
            // Replace the event control with one explicit stmt chain:
            //   init -> [inits] -> [optional destructive pre-clears] -> loop -> awaitResumption
            Ast_ਗੰਢ_Stmt* chainp = ਖਾਲੀ_ਸੂਚਕ;
            chainp = Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ_Null(chainp, initp);
            ਲਈ (Ast_ਗੰਢ_Stmt* ਅਚਲ stmtp : senResults.m_inits) {
                chainp = Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ_Null(chainp, stmtp);
            }
            ਜੇ (hasDestructivePostUpdates) {
                ਲਈ (Ast_ਗੰਢ_Stmt* ਅਚਲ stmtp : senResults.m_destructivePostUpdates) {
                    chainp = Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ_Null(chainp, stmtp->clone_ਰੁੱਖ(ਝੂਠ));
                }
            }
            chainp = Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ_Null(chainp, loopp);
            nodep->replaceWith(chainp);
        } ਨਹੀਂਤਾਂ {
            ਸਵੈਚਾਲਿਤ* ਅਚਲ sentreep = m_finder.get_Sen_ਰੁੱਖ(nodep->sentreep());
            nodep->sentreep()->unlink_Fr_ਪਿੱਠ()->delete_ਰੁੱਖ();
            // Get this sentree's trigger scheduler
            // Replace self with a 'co_await trigSched.trigger()'
            ਸਵੈਚਾਲਿਤ* ਅਚਲ triggerMethodp = ਨਵਾਂ AstCMethodHard{
                flp, ਨਵਾਂ AstVarRef{flp, getCreateTriggerSchedulerp(sentreep), VAccess::WRITE},
                VCMethod::SCHED_TRIGGER};
            triggerMethodp->dtype_ਸੈੱਟ_Void();
            // If it should be committed immediately, pass true, otherwise false
            triggerMethodp->ਜੋੜੋ_Pinsp(nodep->user2() ? ਨਵਾਂ AstConst{flp, AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_True{}}
                                                    : ਨਵਾਂ AstConst{flp, AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_False{}});
            ਜੋੜੋ_Process_Info(triggerMethodp);
            ਜੋੜੋ_Event_Debug_Info(triggerMethodp, sentreep);
            // Create the co_await
            AstCAwait* ਅਚਲ awaitp = ਨਵਾਂ AstCAwait{flp, triggerMethodp, sentreep};
            nodep->replaceWith(awaitp);
        }
        VL_DO_DANGLING(nodep->delete_ਰੁੱਖ(), nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_ਸੌਂਪੋ* nodep) override {
        // Only process once to avoid infinite loops (due to the net delay)
        ਜੇ (nodep->user1_ਸੈੱਟ_Once()) ਵਾਪਸ;
        UINFO(9, "control-visit " << nodep);
        File_ਸਤਰ* ਅਚਲ flp = nodep->fileline();
        Ast_ਗੰਢ* controlp = ਗੁਣਕ_ਬਾਹਰਲਾ_Timing_Control(nodep);
        ਅਚਲ ਤਾਰਕਿਕ ਵਿੱਚ_ਸੌਂਪੋ_Dly = VN_IS(nodep, ਸੌਂਪੋ_Dly);
        // Handle the intra assignment timing control
        // Transform if:
        // * there's a timing control in the assignment
        // * the assignment is an AssignDly and it's in a non-inlined function
        ਜੇ (!controlp && (!ਵਿੱਚ_ਸੌਂਪੋ_Dly || m_underProcedure)) {
            iterateChildren(nodep);
            ਵਾਪਸ;
        }
        // The timing control, if any, which is put in a fork for an NBA
        Ast_ਗੰਢ* ਅਚਲ timingp = controlp;
        // Insert new vars before the timing control if we're in a function; in a process we can't
        // do that, as after V3Begin only modules and functions declare variables. These
        // intra-assignment vars will later be passed to forked processes by value.
        Ast_ਗੰਢ* ਪਾਓ_Beforep = m_underProcedure ? ਖਾਲੀ_ਸੂਚਕ : controlp;
        // Special case for NBA
        ਜੇ (ਵਿੱਚ_ਸੌਂਪੋ_Dly) {
            // Put it in a fork so it doesn't block
            // Could already be the only thing directly under a fork, reuse that if possible
            AstFork* forkp = !nodep->nextp() ? VN_CAST(nodep->ਪਹਿਲਾ_Abovep(), Fork) : ਖਾਲੀ_ਸੂਚਕ;
            ਜੇ (!forkp) forkp = ਨਵਾਂ AstFork{flp, VJoinType::JOIN_NONE};
            ਜੇ (!m_underProcedure) {
                // If it's in a function, it won't be handled by V3Delayed
                // Put it behind an additional named event that gets triggered in the NBA region
                AstEventControl* ਅਚਲ nbaEventControlp = createNbaEventControl(flp);
                Ast_ਸੌਂਪੋ* ਅਚਲ trigAssignp = createNbaEventTriggerAssignment(flp);
                nodep->replaceWith(trigAssignp);
                trigAssignp->ਜੋੜੋ_ਅਗਲਾ_ਇੱਥੇ(nbaEventControlp);
                nbaEventControlp->ਜੋੜੋ_Stmtsp(nodep);
                ਪਾਓ_Beforep = forkp;
                ਜੇ (!controlp) controlp = nbaEventControlp;
            }
            controlp->replaceWith(forkp);
            AstBegin* beginp = VN_CAST(controlp, Begin);
            ਜੇ (!beginp) beginp = ਨਵਾਂ AstBegin{nodep->fileline(), "", controlp, ਝੂਠ};
            forkp->ਜੋੜੋ_Forksp(beginp);
            ਜੋੜੋ_Flags(beginp, T_NBA_UPDATE);
            controlp = forkp;
        }
        UASSERT_OBJ(nodep, controlp, "Assignment should have timing control");
        // Move `insertBeforep` into `AstCLocalScope` if necessary to avoid jumping over
        // a variable initialization that would be inserted before `insertBeforep`. All
        // access to this variable should be contained within returned `AstCLocalScope`.
        ਜੇ (m_under_Jump_ਖੰਡ) ਜੋੜੋ_CLocal_Scope(flp, ਪਾਓ_Beforep);
        // Function for replacing values with intermediate variables
        ਅਚਲ ਸਵੈਚਾਲਿਤ replaceWithIntermediate = [&](Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ valuep,
                                                 ਅਚਲ std::string& name) {
            AstVarScope* ਅਚਲ newvscp = create_ਆਰਜ਼ੀ(flp, name, valuep->dtypep(), ਪਾਓ_Beforep);
            // A pending update keeps its own value, even if its process schedules more
            ਜੇ (ਵਿੱਚ_ਸੌਂਪੋ_Dly && !ਪਾਓ_Beforep) newvscp->fork_By_ਮੁੱਲ(ਸੱਚ);
            valuep->replaceWith(ਨਵਾਂ AstVarRef{flp, newvscp, VAccess::READ});
            controlp->ਜੋੜੋ_ਇੱਥੇ_This_As_ਅਗਲਾ(
                ਨਵਾਂ Ast_ਸੌਂਪੋ{flp, ਨਵਾਂ AstVarRef{flp, newvscp, VAccess::WRITE}, valuep});
        };
        // NBAs with delays evaluate the indices selecting the target now (IEEE 1800-2023 10.4.2),
        // in order, each as a whole, also the selects and function calls in it. So follow the
        // selects of the target up from the variable, not into their indices. As an example, this
        // transforms 'x[a[i]][f(j)] <= #1 y' into 't0 = a[i]; t1 = f(j); x[t0][t1] <= #1 y'.
        // V3Const has split NBAs to concatenations, also streamed, into NBAs to each part.
        ਜੇ (ਵਿੱਚ_ਸੌਂਪੋ_Dly) {
            ਲਈ (Ast_ਗੰਢ* exprp = nodep->lhsp()->ਅਧਾਰ_Fromp(ਸੱਚ); exprp != nodep;
                 exprp = exprp->ਪਹਿਲਾ_Abovep()) {
                ਜੇ (AstSel* ਅਚਲ selp = VN_CAST(exprp, Sel)) {
                    // widthp should be const
                    ਜੇ (!VN_IS(selp->lsbp(), Const)) {
                        replaceWithIntermediate(selp->lsbp(), m_intra_Lsb_ਨਾਂ.get(nodep));
                    }
                } ਨਹੀਂਤਾਂ ਜੇ (Ast_ਗੰਢ_Sel* ਅਚਲ selp = VN_CAST(exprp, ਗੰਢ_Sel)) {
                    ਜੇ (!VN_IS(selp->bitp(), Const)) {
                        replaceWithIntermediate(selp->bitp(), m_intra_ਸੂਚਕਾਂਕ_ਨਾਂ.get(nodep));
                    }
                }
            }
        }
        // Replace the RHS with an intermediate value var
        replaceWithIntermediate(nodep->rhsp(), m_intra_ਮੁੱਲ_ਨਾਂ.get(nodep));
        ਜੇ (!ਵਿੱਚ_ਸੌਂਪੋ_Dly) ਵਾਪਸ;
        // The process executing the NBA evaluates its delay (IEEE 1800-2023 4.9.4), not the forked
        // process, if it has side effects, e.g. calling a function that checks its process. A pure
        // one is evaluated as the forked process starts, at once.
        ਜੇ (AstDelay* ਅਚਲ delayp = VN_CAST(timingp, Delay)) {
            ਜੇ (!delayp->lhsp()->is_ਸ਼ੁੱਧ()) {
                replaceWithIntermediate(delayp->lhsp(), m_intra_ਮੁੱਲ_ਨਾਂ.get(nodep));
            }
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstFireEvent* nodep) override {
        // V3Delayed handles '->>' of a variable in a process. Like NBAs in non-inlined functions,
        // trigger other events from a fork awaiting the NBA region.
        ਜੇ (!nodep->isDelayed() || (m_underProcedure && VN_IS(nodep->operandp(), VarRef))) {
            iterateChildren(nodep);
            ਵਾਪਸ;
        }
        File_ਸਤਰ* ਅਚਲ flp = nodep->fileline();
        Ast_ਸੌਂਪੋ* ਅਚਲ trigAssignp = createNbaEventTriggerAssignment(flp);
        nodep->replaceWith(trigAssignp);
        AstFork* ਅਚਲ forkp = ਨਵਾਂ AstFork{flp, VJoinType::JOIN_NONE};
        trigAssignp->ਜੋੜੋ_ਅਗਲਾ_ਇੱਥੇ(forkp);
        ਜੇ (m_under_Jump_ਖੰਡ) ਜੋੜੋ_CLocal_Scope(flp, forkp);
        // The triggered event is the one referenced now, so evaluate handles and indices now
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ eventp = nodep->operandp()->unlink_Fr_ਪਿੱਠ();
        ਅਚਲ ਸਵੈਚਾਲਿਤ evalNow = [&](Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ valuep, V3_Unique_ਨਾਂ& ਨਾਂ) {
            // Like for an NBA, a process uses a module variable, passed to the fork by value
            AstVarScope* ਅਚਲ vscp = create_ਆਰਜ਼ੀ(flp, ਨਾਂ.get(nodep), valuep->dtypep(),
                                                 m_underProcedure ? ਖਾਲੀ_ਸੂਚਕ : forkp);
            ਜੇ (m_underProcedure) vscp->fork_By_ਮੁੱਲ(ਸੱਚ);
            valuep->replaceWith(ਨਵਾਂ AstVarRef{flp, vscp, VAccess::READ});
            // Unlike an event, a handle selecting one is only read
            valuep->foreach([](Ast_ਗੰਢ* ਅਚਲ np) {
                ਜੇ (Ast_ਗੰਢ_Var_Ref* ਅਚਲ refp = VN_CAST(np, ਗੰਢ_Var_Ref)) {
                    refp->access(VAccess::READ);
                } ਨਹੀਂਤਾਂ ਜੇ (AstMemberSel* ਅਚਲ selp = VN_CAST(np, MemberSel)) {
                    selp->access(VAccess::READ);
                }
            });
            forkp->ਜੋੜੋ_ਇੱਥੇ_This_As_ਅਗਲਾ(
                ਨਵਾਂ Ast_ਸੌਂਪੋ{flp, ਨਵਾਂ AstVarRef{flp, vscp, VAccess::WRITE}, valuep});
        };
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* refp = eventp;
        ਜਦੋਂ (Ast_ਗੰਢ_Sel* ਅਚਲ selp = VN_CAST(refp, ਗੰਢ_Sel)) {
            ਜੇ (!VN_IS(selp->bitp(), Const)) evalNow(selp->bitp(), m_intra_ਸੂਚਕਾਂਕ_ਨਾਂ);
            refp = selp->fromp();
        }
        // Handle of a class (or virtual interface)
        ਜੇ (AstMemberSel* ਅਚਲ selp = VN_CAST(refp, MemberSel)) {
            evalNow(selp->fromp(), m_intra_ਮੁੱਲ_ਨਾਂ);
        }
        AstEventControl* ਅਚਲ controlp = createNbaEventControl(flp);
        controlp->ਜੋੜੋ_Stmtsp(ਨਵਾਂ AstFireEvent{flp, eventp, ਝੂਠ});
        AstBegin* ਅਚਲ beginp = ਨਵਾਂ AstBegin{flp, "", controlp, ਝੂਠ};
        ਜੋੜੋ_Flags(beginp, T_NBA_UPDATE);
        forkp->ਜੋੜੋ_Forksp(beginp);
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਸੌਂਪੋ_W* nodep) override {
        File_ਸਤਰ* ਅਚਲ flp = nodep->fileline();
        // Get the net delay unless this assignment was created for handling the net delay (user1)
        AstDelay* ਅਚਲ netDelayp = nodep->user1() ? ਖਾਲੀ_ਸੂਚਕ : nodep->getLhsNetDelay();
        ਜੇ (netDelayp) {
            ਜੇ (nodep->timingControlp()) {
                // If this assignment has a delay, create another one to handle the net delay
                AstVarScope* ਅਚਲ newvscp = m_ਸੌਂਪੋ_WTmps.make(flp, m_scopep, nodep->dtypep());
                Ast_ਸੌਂਪੋ_W* assignp = ਨਵਾਂ Ast_ਸੌਂਪੋ_W{
                    nodep->fileline(), nodep->lhsp()->unlink_Fr_ਪਿੱਠ(),
                    ਨਵਾਂ AstVarRef{flp, newvscp, VAccess::READ}, netDelayp->clone_ਰੁੱਖ(ਝੂਠ)};
                assignp->user1(ਸੱਚ);
                nodep->ਜੋੜੋ_ਅਗਲਾ_ਇੱਥੇ(assignp);
                nodep->lhsp(ਨਵਾਂ AstVarRef{flp, newvscp, VAccess::WRITE});
            } ਨਹੀਂਤਾਂ {
                // Else just use this one with the net delay
                nodep->timingControlp(netDelayp->clone_ਰੁੱਖ(ਝੂਠ));
            }
        }
        ਜੇ (!nodep->timingControlp()) ਵਾਪਸ;
        // There will be some circular logic here, suppress the warning for newly created vars
        flp->modifyWarnOff(V3_Error_ਕੋਡ::UNOPTFLAT, ਸੱਚ);
        // Also suppress the warning for the LHS var, for cases like `assign #1 clk = ~clk;`
        // TODO: Restore the warning for other, non-delayed drivers
        nodep->lhsp()->foreach([](AstVarRef* refp) {
            ਜੇ (refp->access().isWriteOrRW()) {
                refp->varp()->fileline()->modifyWarnOff(V3_Error_ਕੋਡ::UNOPTFLAT, ਸੱਚ);
            }
        });
        // Should be under an always
        AstAlways* ਅਚਲ alwaysp = VN_AS(m_procp, Always);
        // Convert it to an Assign; the new assign with intra delay will be handled by
        // visit(AstNodeAssign*)
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ lhs1p = nodep->lhsp()->unlink_Fr_ਪਿੱਠ();
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ rhs1p = nodep->rhsp()->unlink_Fr_ਪਿੱਠ();
        Ast_ਗੰਢ* ਅਚਲ controlp = nodep->timingControlp()->unlink_Fr_ਪਿੱਠ();
        ਜੇ (AstDelay* ਅਚਲ delayp = VN_CAST(controlp, Delay)) {
            ਜੇ (Ast_ਗੰਢ_ਵਿਅੰਜਕ* fallDelayp = delayp->fallDelay()) {
                fallDelayp = fallDelayp->unlink_Fr_ਪਿੱਠ();
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp = delayp->lhsp()->unlink_Fr_ਪਿੱਠ();
                // Use fall only for an all-zero value, rise otherwise.
                lhsp = delay_ਮੰਜ਼ਿਲ_Integral(delayp, lhsp);
                fallDelayp = delay_ਮੰਜ਼ਿਲ_Integral(delayp, fallDelayp);
                delayp->lhsp(
                    ਨਵਾਂ AstCond{flp, ਨਵਾਂ AstEq{flp, rhs1p->clone_ਰੁੱਖ(ਝੂਠ), ਨਵਾਂ AstConst{flp, 0}},
                                fallDelayp, lhsp});
            }
        }
        Ast_ਸੌਂਪੋ* ਅਚਲ assignp = ਨਵਾਂ Ast_ਸੌਂਪੋ{nodep->fileline(), lhs1p, rhs1p, controlp};
        // Put the assignment in a fork..join_none.
        AstFork* ਅਚਲ forkp = ਨਵਾਂ AstFork{flp, VJoinType::JOIN_NONE};
        nodep->replaceWith(forkp);
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
        AstBegin* ਅਚਲ beginp = ਨਵਾਂ AstBegin{flp, "", assignp, ਝੂਠ};
        forkp->ਜੋੜੋ_Forksp(beginp);
        ਦੇਖੋ(forkp);  // Visit now as we need to do some post-processing
        // IEEE 1800-2023 10.3.3 - if the RHS value differs from the currently scheduled value to
        // be assigned, the currently scheduled assignment is descheduled. To keep track if an
        // assignment should be descheduled, each scheduled assignment event has a 'generation',
        // and if at assignment time its generation differs from the current generation, it won't
        // be performed
        Ast_ਸੌਂਪੋ* ਅਚਲ preAssignp = VN_AS(beginp->stmtsp(), ਸੌਂਪੋ);
        UASSERT_OBJ(preAssignp, alwaysp, "Pre-assign should be there from visit(AstFork)");
        Ast_ਸੌਂਪੋ* ਅਚਲ postAssignp = VN_AS(preAssignp->nextp()->nextp(), ਸੌਂਪੋ);
        UASSERT_OBJ(postAssignp, alwaysp, "Post-assign should be there from visit(AstFork)");
        // Increment generation and copy it to a local
        AstVarScope* ਅਚਲ generationVarp
            = m_ਸੌਂਪੋ_WGens.make(flp, m_scopep, alwaysp->ਲੱਭੋ_UInt64_DType());
        AstVarScope* ਅਚਲ genLocalVarp
            = create_ਆਰਜ਼ੀ(flp, generationVarp->varp()->name() + "__local",
                         alwaysp->ਲੱਭੋ_UInt64_DType(), preAssignp);
        preAssignp->ਜੋੜੋ_ਇੱਥੇ_This_As_ਅਗਲਾ(
            ਨਵਾਂ Ast_ਸੌਂਪੋ{flp, ਨਵਾਂ AstVarRef{flp, generationVarp, VAccess::WRITE},
                          ਨਵਾਂ Ast_ਜੋੜੋ{flp, ਨਵਾਂ AstVarRef{flp, generationVarp, VAccess::READ},
                                     ਨਵਾਂ AstConst{flp, 1}}});
        preAssignp->ਜੋੜੋ_ਇੱਥੇ_This_As_ਅਗਲਾ(
            ਨਵਾਂ Ast_ਸੌਂਪੋ{flp, ਨਵਾਂ AstVarRef{flp, genLocalVarp, VAccess::WRITE},
                          ਨਵਾਂ AstVarRef{flp, generationVarp, VAccess::READ}});
        // If the current generation is same as the one saved in the local var, assign
        beginp->ਜੋੜੋ_Stmtsp(
            ਨਵਾਂ AstIf{flp,
                      ਨਵਾਂ AstEq{flp, ਨਵਾਂ AstVarRef{flp, generationVarp, VAccess::READ},
                                ਨਵਾਂ AstVarRef{flp, genLocalVarp, VAccess::READ}},
                      postAssignp->unlink_Fr_ਪਿੱਠ()});
        // Save scheduled RHS value before delay
        AstVarScope* ਅਚਲ tmpVarp
            = m_ਸੌਂਪੋ_WTmps.make(flp, m_scopep, preAssignp->rhsp()->dtypep());
        AstVarRef* ਅਚਲ tmp_ਸੌਂਪੋ_Rhsp = VN_AS(preAssignp->lhsp(), VarRef)->clone_ਰੁੱਖ(ਝੂਠ);
        tmp_ਸੌਂਪੋ_Rhsp->access(VAccess::WRITE);
        preAssignp->ਜੋੜੋ_ਅਗਲਾ_ਇੱਥੇ(
            ਨਵਾਂ Ast_ਸੌਂਪੋ{flp, ਨਵਾਂ AstVarRef{flp, tmpVarp, VAccess::WRITE}, tmp_ਸੌਂਪੋ_Rhsp});
        // If the RHS is different from the currently scheduled value, schedule the new assignment
        // The generation will increase, effectively 'descheduling' the previous assignment.
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ didNotInitp
            = ਨਵਾਂ AstLogNot{flp, ਨਵਾਂ AstCExpr{flp, "vlSymsp->__Vm_didInit", 1}};
        AstVarRef* ਅਚਲ ਪਹਿਲਾ_Iterp
            = ਨਵਾਂ AstVarRef{flp, m_netlistp->stl_ਪਹਿਲਾ_Iterationp(), VAccess::READ};
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ schedCondp
            = ਨਵਾਂ AstLogOr{flp,
                           ਨਵਾਂ AstNeq{flp, preAssignp->rhsp()->clone_ਰੁੱਖ(ਝੂਠ),
                                      ਨਵਾਂ AstVarRef{flp, tmpVarp, VAccess::READ}},
                           ਨਵਾਂ AstLogAnd{flp, didNotInitp, ਪਹਿਲਾ_Iterp}};
        alwaysp->ਜੋੜੋ_Stmtsp(ਨਵਾਂ AstIf{flp, schedCondp, forkp->unlink_Fr_ਪਿੱਠ()});
    }
    ਖਾਲੀ ਦੇਖੋ(AstDisableFork* nodep) override {
        ਜੇ (m_hasProcess) ਵਾਪਸ;
        // never reached by any process; remove to avoid compilation error
        VL_DO_DANGLING(nodep->unlink_Fr_ਪਿੱਠ()->delete_ਰੁੱਖ(), nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstWaitFork* nodep) override {
        ਜੇ (m_hasProcess) {
            File_ਸਤਰ* ਅਚਲ flp = nodep->fileline();
            AstCExpr* ਅਚਲ exprp = ਨਵਾਂ AstCExpr{flp, "vlProcess->completedFork()", 1};
            AstWait* ਅਚਲ waitp = ਨਵਾਂ AstWait{flp, exprp, ਖਾਲੀ_ਸੂਚਕ};
            nodep->replaceWith(waitp);
        } ਨਹੀਂਤਾਂ {
            // never reached by any process; remove to avoid compilation error
            nodep->unlink_Fr_ਪਿੱਠ();
        }
        VL_DO_DANGLING(nodep->delete_ਰੁੱਖ(), nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstWait* nodep) override {
        // Wait on changed events related to the vars in the wait statement
        UINFO(9, "control-visit " << nodep);
        File_ਸਤਰ* ਅਚਲ flp = nodep->fileline();
        Ast_ਗੰਢ* ਅਚਲ stmtsp = nodep->stmtsp();
        ਜੇ (stmtsp) stmtsp->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ();
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ condp = V3Const::constifyEdit(nodep->condp()->unlink_Fr_ਪਿੱਠ());
        ਸਵੈਚਾਲਿਤ* ਅਚਲ constp = VN_CAST(condp, Const);
        ਜੇ (constp) {
            ਜੇ (constp->isZero()) {
                // We have to await forever instead of simply returning in case we're deep in a
                // callstack
                AstCMethodHard* ਅਚਲ foreverMethodp = ਨਵਾਂ AstCMethodHard{
                    flp, ਨਵਾਂ AstVarRef{flp, getCreateDelayScheduler(), VAccess::WRITE},
                    VCMethod::SCHED_WAIT_FOREVER};
                foreverMethodp->dtype_ਸੈੱਟ_Void();
                ਜੋੜੋ_Process_Info(foreverMethodp);
                ਜੋੜੋ_Debug_Info(foreverMethodp);
                AstCAwait* ਅਚਲ awaitp = ਨਵਾਂ AstCAwait{flp, foreverMethodp};
                nodep->replaceWith(awaitp);
                ਜੇ (stmtsp) VL_DO_DANGLING(stmtsp->delete_ਰੁੱਖ(), stmtsp);
                VL_DO_DANGLING(condp->delete_ਰੁੱਖ(), condp);
            } ਨਹੀਂਤਾਂ {
                nodep->v3fatalSrc("constant wait should have been removed in "
                                  "TimingSuspendableVisitor::visit(AstWait)");
            }
        } ਨਹੀਂਤਾਂ ਜੇ (needDynamicTrigger(condp)) {
            // No point in making a sentree, just use the expression as sensitivity
            // Put the event control in an if so we only wait if the condition isn't met already
            ਸਵੈਚਾਲਿਤ* ਅਚਲ ifp = ਨਵਾਂ AstIf{
                flp, ਨਵਾਂ AstLogNot{flp, condp},
                ਨਵਾਂ AstEventControl{flp,
                                    ਨਵਾਂ Ast_Sen_ਰੁੱਖ{flp, ਨਵਾਂ Ast_Sen_ਵਸਤੂ{flp, VEdgeType::ET_TRUE,
                                                                       condp->clone_ਰੁੱਖ(ਝੂਠ)}},
                                    ਖਾਲੀ_ਸੂਚਕ}};
            ਜੇ (stmtsp) Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ<Ast_ਗੰਢ, Ast_ਗੰਢ>(ifp, stmtsp);
            nodep->replaceWith(ifp);
        } ਨਹੀਂਤਾਂ {
            // We are using a global sentree, so we cannot use ET_TRUE, as that could lead to the
            // active region never converging. Because of this, we need to use ET_CHANGED in a
            // loop.
            AstEventControl* ਅਚਲ controlp = ਨਵਾਂ AstEventControl{
                flp,
                ਨਵਾਂ Ast_Sen_ਰੁੱਖ{
                    flp, ਨਵਾਂ Ast_Sen_ਵਸਤੂ{flp, VEdgeType::ET_ਬਦਲਿਆ, condp->clone_ਰੁੱਖ(ਝੂਠ)}},
                ਖਾਲੀ_ਸੂਚਕ};
            controlp->user2(ਸੱਚ);  // Commit immediately
            AstLoop* ਅਚਲ loopp = ਨਵਾਂ AstLoop{flp};
            loopp->ਜੋੜੋ_Stmtsp(ਨਵਾਂ AstLoopTest{flp, loopp, ਨਵਾਂ AstLogNot{flp, condp}});
            loopp->ਜੋੜੋ_Stmtsp(controlp);
            ਜੇ (stmtsp) Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ<Ast_ਗੰਢ, Ast_ਗੰਢ>(loopp, stmtsp);
            nodep->replaceWith(loopp);
        }
        VL_DO_DANGLING(nodep->delete_ਰੁੱਖ(), nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstBegin* nodep) override {
        VL_RESTORER(m_procp);
        VL_RESTORER(m_hasProcess);
        m_hasProcess |= hasFlags(nodep, T_HAS_PROC);
        // A pending NBA update is not a subprocess of the process that scheduled it, so it is
        // unaffected by 'disable fork' and 'wait fork' (IEEE 1800-2023 9.6.1, 9.6.3)
        ਜੇ (hasFlags(nodep, T_NBA_UPDATE)) m_hasProcess = ਝੂਠ;
        m_procp = nodep;
        ਜੇ (m_hasProcess) nodep->ਸੈੱਟ_Need_Process();
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstFork* nodep) override {
        ਜੇ (nodep->user1_ਸੈੱਟ_Once()) ਵਾਪਸ;
        UINFO(9, "control-visit " << nodep);
        v3Global.ਸੈੱਟ_ਵਰਤਦਾ_Timing();

        // Create a unique name for this fork
        nodep->name("__Vfork_" + std::ਮੰਜ਼ਿਲ_string(++m_forkCnt));

        // TODO: Should process nodep->stmtsp() in case an earlier pass
        //       inserted something there, but as of today we don't need to.

        // Process and name each fork
        ਮਾਪ_t idx = 0;  // Index for naming begins
        ਲਈ (AstBegin *itemp = nodep->forksp(), *nextp; itemp; itemp = nextp) {
            nextp = VN_AS(itemp->nextp(), Begin);
            iterate(itemp);
            // Note: Even if we do not find any awaits, we cannot simply inline
            // the process here, as new awaits could be added later.

            // Name the begin (later the name will be used for a new function)
            itemp->name(nodep->name() + "__" + std::ਮੰਜ਼ਿਲ_string(idx++));
            ਜੇ (itemp->needProcess()) {
                itemp->ਜੋੜੋ_Stmtsp(
                    ਨਵਾਂ AstCStmt{itemp->fileline(), "vlProcess->state(VlProcess::FINISHED);"});
            }
        }
        ਜੇ (!nodep->joinType().joinNone()) makeForkJoin(nodep);
    }

    //--------------------
    ਖਾਲੀ ਦੇਖੋ(AstVar*) override {}  // Accelerate
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override { iterateChildren(nodep); }

ਜਨਤਕ:
    // CONSTRUCTORS
    ਸਪਸ਼ਟ TimingControlVisitor(AstNetlist* nodep)
        : m_netlistp{nodep} {
        iterate(nodep);

        // If there is no static #0 in the design, but an unknown delay was found,
        // and the user did not specify preference, then warn on all unknown delays
        // as we will be assuming they can be #0, which can cause performance degradation.
        ਲਈ (File_ਸਤਰ* ਅਚਲ flp : m_unknownDelayFlps) {
            flp->v3warn(ZERODLY,
                        "Value of # delay control statically unknown. Assuming it can be #0.\n"
                            << flp->warn_ਹੋਰ()  //
                            << "... If all # delays are non-zero at runtime,\n"
                            << flp->warn_ਹੋਰ()  //
                            << "... use '--no-sched-zero-delay' for improved performance.\n"
                            << flp->warn_ਹੋਰ()  //
                            << "... If a real #0 is expected at runtime,\n"
                            << flp->warn_ਹੋਰ()  //
                            << "... use '--sched-zero-delay' to suppress this warning.");
        }
    }
    ~TimingControlVisitor() override {
        V3Stats::ਜੋੜੋ_Stat("Timing, known #0 delays", m_statZeroDelays);
        V3Stats::ਜੋੜੋ_Stat("Timing, known #const delays", m_statConstDelays);
        V3Stats::ਜੋੜੋ_Stat("Timing, unknown #variable delays", m_statVariableDelays);
    }
};

//######################################################################
// Timing class functions

Ast_ਗੰਢ_ਵਿਅੰਜਕ* V3Timing::delay_ਮੰਜ਼ਿਲ_Ticks(Ast_ਗੰਢ_ਵਿਅੰਜਕ* valuep, VTimescale timeunit) {
    File_ਸਤਰ* ਅਚਲ flp = valuep->fileline();
    ਜੇ (ਅਚਲ AstConst* ਅਚਲ constp = VN_CAST(valuep, Const)) {
        ਜੇ (constp->ਅੰਕ().is1_ਕਦਮ()) {
            // A step is one tick of the global time precision (IEEE 1800-2023 3.14.3)
            VL_DO_DANGLING(valuep->delete_ਰੁੱਖ(), valuep);
            AstConst* ਅਚਲ tickp = ਨਵਾਂ AstConst{flp, AstConst::Unsized64{}, 1};
            tickp->dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ_Sized(64, VSigning::UNSIGNED);
            ਵਾਪਸ tickp;
        }
        ਜੇ (constp->isZero()) ਵਾਪਸ valuep;
    }
    UASSERT_OBJ(!timeunit.isNone(), valuep, "timeunit must be set");
    ਅਚਲ ਦੁੱਗਣਾ timescale_ਗੁਣਕ_D
        = std::ਘਾਤ(10.0, timeunit.power_ਦਾ_Ten() - v3Global.rootp()->timeprecision().power_ਦਾ_Ten());
    ਜੇ (valuep->dtypep()->skipRefp()->isDouble()) {
        AstConst* ਅਚਲ tsfp = ਨਵਾਂ AstConst{flp, AstConst::RealDouble{}, timescale_ਗੁਣਕ_D};
        valuep = ਨਵਾਂ AstMulD{flp, valuep, tsfp};
        valuep = ਨਵਾਂ AstRToIRoundS{flp, valuep};
        valuep->dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ_Sized(64, VSigning::UNSIGNED);
    } ਨਹੀਂਤਾਂ {
        valuep->dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ_Sized(64, VSigning::UNSIGNED);
        ਅਚਲ uint64_t timescale_ਗੁਣਕ_U = static_cast<uint64_t>(timescale_ਗੁਣਕ_D);
        AstConst* ਅਚਲ tsfp = ਨਵਾਂ AstConst{flp, AstConst::Unsized64{}, timescale_ਗੁਣਕ_U};
        valuep = ਨਵਾਂ AstMul{flp, valuep, tsfp};
    }
    ਵਾਪਸ V3Const::constifyEdit(valuep);  // Simplify
}

ਖਾਲੀ V3Timing::timingAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        ਅਚਲ VNUser1_ਵਿੱਚ_Use m_user1_ਵਿੱਚ_Use;
        ਅਚਲ VNUser2_ਵਿੱਚ_Use m_user2_ਵਿੱਚ_Use;
        { TimingSuspendableVisitor{nodep}; }
        ਜੇ (v3Global.ਵਰਤਦਾ_Timing()) TimingControlVisitor{nodep};
    }
    V3Global::dump_Check_Global_ਰੁੱਖ("timing", 0, dump_ਰੁੱਖ_Either_Level() >= 3);
}
