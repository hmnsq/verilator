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

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ಅನ್ತರ್ಭಾವ "V3Timing.h"

#ಅನ್ತರ್ಭಾವ "V3Const.h"
#ಅನ್ತರ್ಭಾವ "V3EmitV.h"
#ಅನ್ತರ್ಭಾವ "V3File.h"
#ಅನ್ತರ್ಭಾವ "V3Global.h"
#ಅನ್ತರ್ಭಾವ "V3Graph.h"
#ಅನ್ತರ್ಭಾವ "V3MemberMap.h"
#ಅನ್ತರ್ಭಾವ "V3SenExprBuilder.h"
#ಅನ್ತರ್ಭಾವ "V3SenTree.h"
#ಅನ್ತರ್ಭಾವ "V3SharedTmps.h"
#ಅನ್ತರ್ಭಾವ "V3Stats.h"
#ಅನ್ತರ್ಭಾವ "V3UniqueNames.h"

#ಅನ್ತರ್ಭಾವ <limits>
#ಅನ್ತರ್ಭಾವ <queue>

VL_DEFINE_DEBUG_FUNCTIONS;

// ######################################################################

ಎಣಿಕೆ ಗಂಟು_Flag : uint8_t {
    T_SUSPENDEE = 1 << 0,  // Suspendable (due to dependence on another suspendable)
    T_SUSPENDER = 1 << 1,  // Suspendable (has timing control)
    T_ALLOCS_PROC = 1 << 2,  // Can allocate VlProcess
    T_FORCES_PROC = 1 << 3,  // Forces VlProcess allocation
    T_NEEDS_PROC = 1 << 4,  // Needs access to VlProcess if it's allocated
    T_HAS_PROC = 1 << 5,  // Has VlProcess argument in the signature
    T_NBA_UPDATE = 1 << 6,  // Fork branch of a pending NBA update, which is not a subprocess
};

ಎಣಿಕೆ ForkType : uint8_t {
    F_NONE = 0,  // Not under a fork
    F_MIGHT_SUSPEND = 1 << 0,  // Fork might suspend the execution of current process
    F_MIGHT_NEED_PROC = 1 << 1,  // Fork might need a process (any fork really)
};

ಎಣಿಕೆ ಪ್ರಸರಣ_Type : uint8_t {
    P_CALL = 1,  // Propagation through call to a function/task/method
    P_FORK = 2,  // Propagation due to fork's behavior
    P_SIGNATURE = 3,  // Propagation required to maintain C++ function's signature requirements
};

// Add timing flag to a node
ಸ್ಥಿರ ಶೂನ್ಯ ಕೂಡಿಸು_Flags(Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ nodep, uint8_t flags) { nodep->user2Or(flags); }
// Check if a node has ALL of the expected flags set
ಸ್ಥಿರ ತಾರ್ಕಿಕ hasFlags(Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ nodep, uint8_t flags) { ಹಿಂತಿರುಗಿ !(~nodep->user2() & flags); }

// ######################################################################
//  Detect nodes affected by timing and/or requiring a process

ವರ್ಗ TimingSuspendableVisitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
    // TYPES
    // Vertex of a dependency graph of suspendable nodes, e.g. if a node (process or task) is
    // suspendable, all its dependents should also be suspendable
    ವರ್ಗ DepVtx VL_NOT_FINAL : ಸಾರ್ವಜನಿಕ V3_ನಕ್ಷೆ_ಶೃಂಗ {
        VL_RTTI_IMPL(DepVtx, V3_ನಕ್ಷೆ_ಶೃಂಗ)
        AstClass* ಸ್ಥಿರಾಂಕ m_classp;  // Class associated with a method
        Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ m_nodep;  // AST node represented by this graph vertex

        // ACCESSORS
        string name() ಸ್ಥಿರಾಂಕ override {
            ಆದರೆ (m_classp) {
                ಆದರೆ (VN_IS(nodep(), CFunc)) {
                    ಹಿಂತಿರುಗಿ cvt_ಗಮ್ಯ_Hex(nodep()) + ' ' + classp()->name() + "::" + nodep()->name();
                }
            }
            ಹಿಂತಿರುಗಿ cvt_ಗಮ್ಯ_Hex(nodep()) + ' ' + nodep()->prettyTypeName();
        }
        File_ಸಾಲು* fileline() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ nodep()->fileline(); }

    ಸಾರ್ವಜನಿಕ:
        // CONSTRUCTORS
        DepVtx(V3_ನಕ್ಷೆ* graphp, Ast_ಗಂಟು* nodep, AstClass* classp)
            : V3_ನಕ್ಷೆ_ಶೃಂಗ{graphp}
            , m_classp{classp}
            , m_nodep{nodep} {}
        ~DepVtx() override = ಪೂರ್ವನಿಯೋಜಿತ;

        // ACCESSORS
        ವಾಸ್ತವಿಕ Ast_ಗಂಟು* nodep() ಸ್ಥಿರಾಂಕ VL_MT_STABLE { ಹಿಂತಿರುಗಿ m_nodep; }
        ವಾಸ್ತವಿಕ Ast_ಗಂಟು* classp() ಸ್ಥಿರಾಂಕ VL_MT_STABLE { ಹಿಂತಿರುಗಿ m_classp; }
    };

    ವರ್ಗ SuspendDepVtx final : ಸಾರ್ವಜನಿಕ DepVtx {
        VL_RTTI_IMPL(SuspendDepVtx, DepVtx)
        string dotColor() ಸ್ಥಿರಾಂಕ override {
            ಆದರೆ (hasFlags(nodep(), T_SUSPENDER)) ಹಿಂತಿರುಗಿ "red";
            ಆದರೆ (hasFlags(nodep(), T_SUSPENDEE)) ಹಿಂತಿರುಗಿ "blue";
            ಹಿಂತಿರುಗಿ "black";
        }

    ಸಾರ್ವಜನಿಕ:
        SuspendDepVtx(V3_ನಕ್ಷೆ* graphp, Ast_ಗಂಟು* nodep, AstClass* classp)
            : DepVtx{graphp, nodep, classp} {}
        ~SuspendDepVtx() override = ಪೂರ್ವನಿಯೋಜಿತ;
    };

    ವರ್ಗ NeedsProcDepVtx final : ಸಾರ್ವಜನಿಕ DepVtx {
        VL_RTTI_IMPL(NeedsProcDepVtx, DepVtx)
        string dotColor() ಸ್ಥಿರಾಂಕ override {
            ಆದರೆ (hasFlags(nodep(), T_HAS_PROC)) ಹಿಂತಿರುಗಿ "blue";
            ಆದರೆ (hasFlags(nodep(), T_NEEDS_PROC)) ಹಿಂತಿರುಗಿ "green";
            ಆದರೆ (hasFlags(nodep(), T_FORCES_PROC)) ಹಿಂತಿರುಗಿ "red";
            ಹಿಂತಿರುಗಿ "black";
        }

    ಸಾರ್ವಜನಿಕ:
        NeedsProcDepVtx(V3_ನಕ್ಷೆ* graphp, Ast_ಗಂಟು* nodep, AstClass* classp)
            : DepVtx{graphp, nodep, classp} {}
        ~NeedsProcDepVtx() override = ಪೂರ್ವನಿಯೋಜಿತ;
    };

    // NODE STATE
    //  AstClass::user1() -> bool.  Class member cache has been refreshed.
    //  Ast{NodeProcedure,CFunc,Begin}::user2()  -> uint64_t.  Set to >= T_SUSP if
    //                                                         process/task suspendable
    //                                                         and to T_PROC if it
    //                                                         needs process metadata.
    //  Ast{NodeProcedure,CFunc,Begin}::user3()  -> DependencyVertex*.  Vertex in m_suspGraph
    //  Ast{NodeProcedure,CFunc,Begin}::user3()  -> DependencyVertex*.  Vertex in m_procGraph
    ಸ್ಥಿರಾಂಕ VNUser3_ಒಳ_Use m_user3_ಒಳ_Use;
    ಸ್ಥಿರಾಂಕ VNUser4_ಒಳ_Use m_user4_ಒಳ_Use;

    // STATE
    VMemberMap m_memberMap;  // Member names cached for fast lookup
    AstClass* m_classp = ಶೂನ್ಯಸೂಚಕ;  // Current class
    Ast_ಗಂಟು* m_procp = ಶೂನ್ಯಸೂಚಕ;  // NodeProcedure/CFunc/Begin we're under
    uint8_t m_underFork = F_NONE;  // F_NONE or flags of a fork we are under
    V3_ನಕ್ಷೆ m_susp_ನಕ್ಷೆ;  // Dependency graph where a node is a dependency of another if it being
                          // suspendable makes the other node suspendable
    V3_ನಕ್ಷೆ m_proc_ನಕ್ಷೆ;  // Dependency graph where a node is a dependency of another if it being
                          // suspendable makes the other node suspendable

    // METHODS
    // Get or create the dependency vertex for the given node
    DepVtx* getSuspendDepVtx(Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ nodep) {
        AstClass* classp = ಶೂನ್ಯಸೂಚಕ;
        ಆದರೆ (AstCFunc* funcp = VN_CAST(nodep, CFunc)) {
            ಆದರೆ (funcp->scopep() && funcp->scopep()->modp()) {
                classp = VN_CAST(funcp->scopep()->modp(), Class);
            }
        }
        ಆದರೆ (!nodep->user3p()) nodep->user3p(ಹೊಸ SuspendDepVtx{&m_susp_ನಕ್ಷೆ, nodep, classp});
        ಹಿಂತಿರುಗಿ nodep->user3u().ಗಮ್ಯ<SuspendDepVtx*>();
    }
    DepVtx* getNeedsProcDepVtx(Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ nodep) {
        AstClass* classp = ಶೂನ್ಯಸೂಚಕ;
        ಆದರೆ (AstCFunc* funcp = VN_CAST(nodep, CFunc)) {
            ಆದರೆ (funcp->scopep() && funcp->scopep()->modp()) {
                classp = VN_CAST(funcp->scopep()->modp(), Class);
            }
        }
        ಆದರೆ (!nodep->user4p()) nodep->user4p(ಹೊಸ NeedsProcDepVtx{&m_proc_ನಕ್ಷೆ, nodep, classp});
        ಹಿಂತಿರುಗಿ nodep->user4u().ಗಮ್ಯ<NeedsProcDepVtx*>();
    }
    // Pass timing flag between nodes
    ತಾರ್ಕಿಕ passFlag(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* from, Ast_ಗಂಟು* ಗಮ್ಯ, ಗಂಟು_Flag flag) {
        ಆದರೆ ((from->user2() & flag) && !(ಗಮ್ಯ->user2() & flag)) {
            ಕೂಡಿಸು_Flags(ಗಮ್ಯ, flag);
            ಹಿಂತಿರುಗಿ ನಿಜ;
        }
        ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    }
    // Propagate flag to all nodes that depend on the given one
    ಶೂನ್ಯ propagateFlags(DepVtx* ಸ್ಥಿರಾಂಕ vxp, ಗಂಟು_Flag flag) {
        ಸ್ವಯಂಚಾಲಿತ* ಸ್ಥಿರಾಂಕ parentp = vxp->nodep();
        ಗಾಗಿ (V3_ನಕ್ಷೆ_ಅಂಚು& ಅಂಚು : vxp->ಹೊರಹೋಗು_ಅಂಚುಗಳು()) {
            ಸ್ವಯಂಚಾಲಿತ* ಸ್ಥಿರಾಂಕ depVxp = static_cast<DepVtx*>(ಅಂಚು.ತುದಿ());
            Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ depp = depVxp->nodep();
            ಆದರೆ (passFlag(parentp, depp, flag)) propagateFlags(depVxp, flag);
        }
    }
    ಮಾದರಿ <ಬಗೆ_ಹೆಸರು T_Predicate>
    ಶೂನ್ಯ propagateFlagsIf(DepVtx* ಸ್ಥಿರಾಂಕ vxp, ಗಂಟು_Flag flag, T_Predicate p) {
        ಸ್ವಯಂಚಾಲಿತ* ಸ್ಥಿರಾಂಕ parentp = vxp->nodep();
        ಗಾಗಿ (V3_ನಕ್ಷೆ_ಅಂಚು& ಅಂಚು : vxp->ಹೊರಹೋಗು_ಅಂಚುಗಳು()) {
            ಸ್ವಯಂಚಾಲಿತ* ಸ್ಥಿರಾಂಕ depVxp = static_cast<DepVtx*>(ಅಂಚು.ತುದಿ());
            Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ depp = depVxp->nodep();
            ಆದರೆ (p(&ಅಂಚು) && passFlag(parentp, depp, flag)) propagateFlagsIf(depVxp, flag, p);
        }
    }
    ಮಾದರಿ <ಬಗೆ_ಹೆಸರು T_Predicate>
    ಶೂನ್ಯ propagateFlagsReversedIf(DepVtx* ಸ್ಥಿರಾಂಕ vxp, ಗಂಟು_Flag flag, T_Predicate p) {
        ಸ್ವಯಂಚಾಲಿತ* ಸ್ಥಿರಾಂಕ parentp = vxp->nodep();
        ಗಾಗಿ (V3_ನಕ್ಷೆ_ಅಂಚು& ಅಂಚು : vxp->ಒಳ_ಅಂಚುಗಳು()) {
            ಸ್ವಯಂಚಾಲಿತ* ಸ್ಥಿರಾಂಕ depVxp = static_cast<DepVtx*>(ಅಂಚು.fromp());
            Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ depp = depVxp->nodep();
            ಆದರೆ (p(&ಅಂಚು) && passFlag(parentp, depp, flag))
                propagateFlagsReversedIf(depVxp, flag, p);
        }
    }

    // VISITORS
    ಶೂನ್ಯ ಭೇಟಿ(AstClass* nodep) override {
        UASSERT_OBJ(!m_classp, nodep, "Class under class");
        VL_RESTORER(m_classp);
        m_classp = nodep;
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Procedure* nodep) override {
        VL_RESTORER(m_procp);
        m_procp = nodep;
        getNeedsProcDepVtx(nodep);
        ಕೂಡಿಸು_Flags(nodep, T_ALLOCS_PROC);
        ಆದರೆ (VN_IS(nodep, Always)) {
            UINFO(9, "Always does " << (nodep->needProcess() ? "" : "NOT ") << "need process");
        }
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstDisableFork* nodep) override {
        ಭೇಟಿ(static_cast<Ast_ಗಂಟು*>(nodep));
        ಕೂಡಿಸು_Flags(m_procp, T_FORCES_PROC | T_NEEDS_PROC);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstWaitFork* nodep) override {
        ಭೇಟಿ(static_cast<Ast_ಗಂಟು*>(nodep));
        ಕೂಡಿಸು_Flags(m_procp, T_FORCES_PROC | T_NEEDS_PROC);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstWait* nodep) override {
        UINFO(9, "suspendable-visit " << nodep);
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ condp = V3Const::constifyEdit(nodep->condp());
        ಆದರೆ (AstConst* ಸ್ಥಿರಾಂಕ constp = VN_CAST(condp, Const)) {
            ಆದರೆ (!nodep->fileline()->warnIsOff(V3_Error_ಸಂಕೇತ::WAITCONST)) {
                condp->v3warn(WAITCONST, "Wait statement condition is constant");
            }
            ಆದರೆ (!constp->isZero()) {
                // Remove AstWait before we track process as T_SUSPENDER
                ಆದರೆ (Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ stmtsp = nodep->stmtsp()) {
                    stmtsp->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ();
                    nodep->replaceWith(stmtsp);
                } ಇಲ್ಲದಿದ್ದರೆ {
                    nodep->unlink_Fr_ಹಿಂಭಾಗ();
                }
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                ಹಿಂತಿರುಗಿ;
            }
        }
        v3Global.ಗಣ_ಬಳಸುತ್ತದೆ_Timing();
        ಆದರೆ (m_procp) ಕೂಡಿಸು_Flags(m_procp, T_SUSPENDEE | T_SUSPENDER | T_NEEDS_PROC);
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCFunc* nodep) override {
        VL_RESTORER(m_procp);
        m_procp = nodep;
        iterateChildren(nodep);
        ಆದರೆ (nodep->needProcess()) ಕೂಡಿಸು_Flags(nodep, T_FORCES_PROC | T_NEEDS_PROC);
        DepVtx* ಸ್ಥಿರಾಂಕ sVxp = getSuspendDepVtx(nodep);
        DepVtx* ಸ್ಥಿರಾಂಕ pVxp = getNeedsProcDepVtx(nodep);
        ಆದರೆ (!m_classp) ಹಿಂತಿರುಗಿ;

        // Go over overridden functions

        std::ಸರತಿ<AstClassExtends*> extends;
        ಆದರೆ (m_classp->extendsp()) extends.ತಳ್ಳು(m_classp->extendsp());

        ಆಗ (!extends.ಖಾಲಿಗಳು()) {
            AstClassExtends* ext_list = extends.front();
            extends.ತೆಗೆ_2();

            ಗಾಗಿ (AstClassExtends* cextp = ext_list; cextp;
                 cextp = VN_AS(cextp->nextp(), ClassExtends)) {
                // TODO: It is possible that a methods the same name in the base class is not
                // actually overridden by our method. If this causes a problem, traverse to
                // the root of the inheritance hierarchy and check if the original method is
                // virtual or not.
                ಆದರೆ (ಸ್ವಯಂಚಾಲಿತ* ಸ್ಥಿರಾಂಕ overriddenp
                    = VN_CAST(m_memberMap.ಕಂಡುಹಿಡಿ_Member(cextp->classp(), nodep->name()), CFunc)) {
                    // Suspendability and process affects typing, so they propagate both ways
                    DepVtx* ಸ್ಥಿರಾಂಕ overriddenSVxp = getSuspendDepVtx(overriddenp);
                    DepVtx* ಸ್ಥಿರಾಂಕ overriddenPVxp = getNeedsProcDepVtx(overriddenp);
                    ಹೊಸ V3_ನಕ್ಷೆ_ಅಂಚು{&m_susp_ನಕ್ಷೆ, sVxp, overriddenSVxp, P_SIGNATURE};
                    ಹೊಸ V3_ನಕ್ಷೆ_ಅಂಚು{&m_susp_ನಕ್ಷೆ, overriddenSVxp, sVxp, P_SIGNATURE};
                    ಹೊಸ V3_ನಕ್ಷೆ_ಅಂಚು{&m_proc_ನಕ್ಷೆ, pVxp, overriddenPVxp, P_SIGNATURE};
                    ಹೊಸ V3_ನಕ್ಷೆ_ಅಂಚು{&m_proc_ನಕ್ಷೆ, overriddenPVxp, pVxp, P_SIGNATURE};
                } ಇಲ್ಲದಿದ್ದರೆ {
                    AstClassExtends* ಇನ್ನಷ್ಟು_extends = cextp->classp()->extendsp();
                    ಆದರೆ (ಇನ್ನಷ್ಟು_extends) extends.ತಳ್ಳು(ಇನ್ನಷ್ಟು_extends);
                }
            }
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_CCall* nodep) override {
        AstCFunc* ಸ್ಥಿರಾಂಕ funcp = nodep->funcp();
        UASSERT_OBJ(funcp, nodep, "AstNodeCCall must have non-null funcp post-link");
        UASSERT_OBJ(m_procp, nodep, "AstNodeCCall must be inside a procedure/CFunc/Begin");

        UINFO(9, "V3Timing: Processing CCall to " << funcp->name() << " in dependency graph\n");
        ಹೊಸ V3_ನಕ್ಷೆ_ಅಂಚು{&m_susp_ನಕ್ಷೆ, getSuspendDepVtx(funcp), getSuspendDepVtx(m_procp), P_CALL};

        ಹೊಸ V3_ನಕ್ಷೆ_ಅಂಚು{&m_proc_ನಕ್ಷೆ, getNeedsProcDepVtx(funcp), getNeedsProcDepVtx(m_procp),
                        P_CALL};

        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstBegin* nodep) override {
        VL_RESTORER(m_procp);
        VL_RESTORER(m_underFork);

        ಆದರೆ (!m_underFork || (m_underFork & F_MIGHT_SUSPEND))
            ಹೊಸ V3_ನಕ್ಷೆ_ಅಂಚು{&m_susp_ನಕ್ಷೆ, getSuspendDepVtx(nodep), getSuspendDepVtx(m_procp),
                            m_underFork ? P_FORK : P_CALL};

        ಹೊಸ V3_ನಕ್ಷೆ_ಅಂಚು{&m_proc_ನಕ್ಷೆ, getNeedsProcDepVtx(nodep), getNeedsProcDepVtx(m_procp),
                        P_CALL};

        ಆದರೆ (m_underFork) ಕೂಡಿಸು_Flags(nodep, T_NEEDS_PROC | T_ALLOCS_PROC);

        m_procp = nodep;
        m_underFork = 0;
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstFork* nodep) override {
        UINFO(9, "suspendable-visit " << nodep);
        VL_RESTORER(m_underFork);

        v3Global.ಗಣ_ಬಳಸುತ್ತದೆ_Timing();  // Even if there are no event controls, we have to set this flag
                                   // so that transformForks() in V3SchedTiming gets called and
                                   // removes all forks and begins
        ಆದರೆ (nodep->isTimingControl() && m_procp) {
            ಕೂಡಿಸು_Flags(m_procp, T_SUSPENDEE | T_SUSPENDER);
            m_underFork |= F_MIGHT_SUSPEND;
        }
        m_underFork |= F_MIGHT_NEED_PROC;
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ನಿಯೋಜಿಸು_Dly* nodep) override {
        ಆದರೆ (!VN_IS(m_procp, ಗಂಟು_Procedure)) v3Global.ಗಣ_ಬಳಸುತ್ತದೆ_Timing();
        ಭೇಟಿ(static_cast<Ast_ಗಂಟು*>(nodep));
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstFireEvent* nodep) override {
        ಆದರೆ (nodep->isDelayed()
            && (!VN_IS(m_procp, ಗಂಟು_Procedure) || !VN_IS(nodep->operandp(), VarRef))) {
            v3Global.ಗಣ_ಬಳಸುತ್ತದೆ_Timing();
        }
        ಭೇಟಿ(static_cast<Ast_ಗಂಟು*>(nodep));
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ನಿಯೋಜಿಸು_W* nodep) override {
        ಆದರೆ (nodep->timingControlp()) v3Global.ಗಣ_ಬಳಸುತ್ತದೆ_Timing();
        // Containing process will not suspend, don't mark it
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override {
        ಆದರೆ (nodep->isTimingControl()) {
            v3Global.ಗಣ_ಬಳಸುತ್ತದೆ_Timing();
            ಆದರೆ (m_procp) ಕೂಡಿಸು_Flags(m_procp, T_SUSPENDEE | T_SUSPENDER | T_NEEDS_PROC);
        }
        iterateChildren(nodep);
    }

    //--------------------
    ಶೂನ್ಯ ಭೇಟಿ(AstVar*) override {}  // Accelerate

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    ಸ್ಪಷ್ಟ TimingSuspendableVisitor(AstNetlist* nodep) {
        iterate(nodep);
        m_susp_ನಕ್ಷೆ.remove_ಸಂಕ್ರಾಮಿಕ_ಅಂಚುಗಳು();
        m_proc_ನಕ್ಷೆ.remove_ಸಂಕ್ರಾಮಿಕ_ಅಂಚುಗಳು();
        // Propagate suspendability
        ಗಾಗಿ (V3_ನಕ್ಷೆ_ಶೃಂಗ& vtx : m_susp_ನಕ್ಷೆ.vertices()) {
            DepVtx& depVtx = static_cast<DepVtx&>(vtx);
            ಆದರೆ (hasFlags(depVtx.nodep(), T_SUSPENDEE)) propagateFlags(&depVtx, T_SUSPENDEE);
        }
        ಆದರೆ (dump_ನಕ್ಷೆ_Level() >= 6) m_susp_ನಕ್ಷೆ.dumpDotFilePrefixed("timing_deps");

        // Propagate T_HAS_PROCESS
        ಗಾಗಿ (V3_ನಕ್ಷೆ_ಶೃಂಗ& vtx : m_proc_ನಕ್ಷೆ.vertices()) {
            DepVtx& depVtx = static_cast<DepVtx&>(vtx);
            // Find processes that'll allocate VlProcess
            ಆದರೆ (hasFlags(depVtx.nodep(), T_FORCES_PROC)) {
                propagateFlagsIf(&depVtx, T_FORCES_PROC, [&](ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಅಂಚು* e) -> ತಾರ್ಕಿಕ {
                    ಹಿಂತಿರುಗಿ !hasFlags(static_cast<DepVtx*>(e->fromp())->nodep(), T_ALLOCS_PROC);
                });
            }
            // Mark nodes on paths between processes and statements that use VlProcess
            ಆದರೆ (hasFlags(depVtx.nodep(), T_NEEDS_PROC)) {
                propagateFlagsIf(&depVtx, T_NEEDS_PROC, [&](ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಅಂಚು* e) -> ತಾರ್ಕಿಕ {
                    ಹಿಂತಿರುಗಿ !hasFlags(static_cast<DepVtx*>(e->ತುದಿ())->nodep(), T_ALLOCS_PROC);
                });
            }
        }
        ಗಾಗಿ (V3_ನಕ್ಷೆ_ಶೃಂಗ& vtx : m_proc_ನಕ್ಷೆ.vertices()) {
            DepVtx& depVtx = static_cast<DepVtx&>(vtx);
            // Mark nodes that will be emitted with a VlProcess argument
            ಆದರೆ (hasFlags(depVtx.nodep(), T_ALLOCS_PROC | T_FORCES_PROC)) {
                ಕೂಡಿಸು_Flags(depVtx.nodep(), T_HAS_PROC);
                propagateFlagsReversedIf(&depVtx, T_HAS_PROC, [&](ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಅಂಚು* e) -> ತಾರ್ಕಿಕ {
                    ಹಿಂತಿರುಗಿ hasFlags(static_cast<DepVtx*>(e->fromp())->nodep(), T_NEEDS_PROC);
                });
            }
        }
        ಆದರೆ (dump_ನಕ್ಷೆ_Level() >= 6) m_proc_ನಕ್ಷೆ.dumpDotFilePrefixed("proc_deps");
    }
    ~TimingSuspendableVisitor() override = ಪೂರ್ವನಿಯೋಜಿತ;
};

// ######################################################################
//  Transform nodes affected by timing

ವರ್ಗ TimingControlVisitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
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
    AstNetlist* ಸ್ಥಿರಾಂಕ m_netlistp;  // Root node
    Ast_ತುದಿ_Scope* ಸ್ಥಿರಾಂಕ m_ತುದಿ_Scopep = m_netlistp->ತುದಿ_Scopep();  // The AstTopScope
    AstScope* ಸ್ಥಿರಾಂಕ m_scopeTopp = m_ತುದಿ_Scopep->scopep();  // Scope at the top
    AstClass* m_classp = ಶೂನ್ಯಸೂಚಕ;  // Current class
    AstScope* m_scopep = ಶೂನ್ಯಸೂಚಕ;  // Current scope
    Ast_ಸಕ್ರಿಯ* m_activep = ಶೂನ್ಯಸೂಚಕ;  // Current active
    Ast_ಗಂಟು* m_procp = ಶೂನ್ಯಸೂಚಕ;  // NodeProcedure/CFunc/Begin we're under
    ತಾರ್ಕಿಕ m_hasProcess = ಸುಳ್ಳು;  // True if current scope has a VlProcess handle available
    ಪೂರ್ಣಾಂಕ m_forkCnt = 0;  // Number of forks inside a module
    ತಾರ್ಕಿಕ m_under_Jump_ಖಂಡ = ಸುಳ್ಳು;  // True if we are inside of a jump-block
    ತಾರ್ಕಿಕ m_underProcedure = ಸುಳ್ಳು;  // True if we are under an always or initial
    ತಾರ್ಕಿಕ m_underIfaceCFunc = ಸುಳ್ಳು;  // True if we are under a CFunc owned by an interface scope
    ತಾರ್ಕಿಕ m_hasStaticZeroDelay = ಸುಳ್ಳು;  // True if we have a static #0 delay
    std::ಸದಿಶ<File_ಸಾಲು*> m_unknownDelayFlps;  // Locations of AstDelay with non-constant value

    // Unique names
    V3_Unique_ಹೆಸರುಗಳು m_dlyfork_ಹೆಸರುಗಳು{"__Vdlyfork"};  // Names for temp AssignW vars
    V3_Unique_ಹೆಸರುಗಳು m_intra_ಮೌಲ್ಯ_ಹೆಸರುಗಳು{"__Vintraval"};  // Intra assign delay value var names
    V3_Unique_ಹೆಸರುಗಳು m_intra_ಸೂಚ್ಯಂಕ_ಹೆಸರುಗಳು{"__Vintraidx"};  // Intra assign delay index var names
    V3_Unique_ಹೆಸರುಗಳು m_intra_Lsb_ಹೆಸರುಗಳು{"__Vintralsb"};  // Intra assign delay LSB var names
    V3_Unique_ಹೆಸರುಗಳು m_trig_Sched_ಹೆಸರುಗಳು{"__VtrigSched"};  // Trigger scheduler name generator
    V3_Unique_ಹೆಸರುಗಳು m_dyn_Trig_ಹೆಸರುಗಳು{"__VdynTrigger"};  // Dynamic trigger name generator
    // Module level temporary variables, shared by instances
    V3SharedTmps m_ನಿಯೋಜಿಸು_WTmps{"__VassignWtmp", VVarType::MODULETEMP};  // Delayed AssignW values
    V3SharedTmps m_ನಿಯೋಜಿಸು_WGens{"__VassignWgen", VVarType::MODULETEMP};  // AssignW generations

    // DTypes
    AstBasicDType* m_forkDtp = ಶೂನ್ಯಸೂಚಕ;  // Fork variable type
    AstBasicDType* m_trigSchedDtp = ಶೂನ್ಯಸೂಚಕ;  // Trigger scheduler type

    // Timing-related globals
    AstVarScope* m_delaySchedp = ಶೂನ್ಯಸೂಚಕ;  // Global delay scheduler
    AstVarScope* m_dynamicSchedp = ಶೂನ್ಯಸೂಚಕ;  // Global dynamic trigger scheduler
    Ast_Sen_ವೃಕ್ಷ* m_delaySensesp = ಶೂನ್ಯಸೂಚಕ;  // Domain to trigger if a delayed coroutine is resumed
    Ast_Sen_ವೃಕ್ಷ* m_dynamicSensesp = ಶೂನ್ಯಸೂಚಕ;  // Domain to trigger if a dynamic trigger is set

    // Other
    Sen_ವೃಕ್ಷ_Finder m_finder{m_netlistp};  // Sentree finder and uniquifier
    Sen_ಅಭಿವ್ಯಕ್ತಿ_Builder* m_sen_ಅಭಿವ್ಯಕ್ತಿ_Builderp = ಶೂನ್ಯಸೂಚಕ;  // Sens expression builder for current m_scope

    // Stats
    ಗಾತ್ರ_t m_statZeroDelays = 0;  // Number of statically known #0 delays
    ಗಾತ್ರ_t m_statConstDelays = 0;  // Number of statically known #const (non-zero) delays
    ಗಾತ್ರ_t m_statVariableDelays = 0;  // Number of delays with value unknown at compile time

    // METHODS
    // Transform an assignment with an intra timing control into a timing control with the
    // assignment under it
    Ast_ಗಂಟು* ಗುಣಕ_ಹೊರಹೋಗು_Timing_Control(Ast_ಗಂಟು_ನಿಯೋಜಿಸು* nodep) ಸ್ಥಿರಾಂಕ {
        Ast_ಗಂಟು* stmtp = nodep;
        Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ controlp = nodep->timingControlp();
        ಆದರೆ (AstDelay* ಸ್ಥಿರಾಂಕ delayp = VN_CAST(controlp, Delay)) {
            stmtp->replaceWith(delayp->unlink_Fr_ಹಿಂಭಾಗ());
            delayp->ಕೂಡಿಸು_Stmtsp(stmtp);
            stmtp = delayp;
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (Ast_Sen_ವೃಕ್ಷ* ಸ್ಥಿರಾಂಕ sentreep = VN_CAST(controlp, Sen_ವೃಕ್ಷ)) {
            AstEventControl* ಸ್ಥಿರಾಂಕ eventControlp
                = ಹೊಸ AstEventControl{sentreep->fileline(), sentreep->unlink_Fr_ಹಿಂಭಾಗ(), ಶೂನ್ಯಸೂಚಕ};
            stmtp->replaceWith(eventControlp);
            eventControlp->ಕೂಡಿಸು_Stmtsp(stmtp);
            stmtp = eventControlp;
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (AstBegin* ಸ್ಥಿರಾಂಕ beginp = VN_CAST(controlp, Begin)) {
            // Begin from V3AssertPre
            stmtp->replaceWith(beginp->unlink_Fr_ಹಿಂಭಾಗ());
            beginp->ಕೂಡಿಸು_Stmtsp(stmtp);
            stmtp = beginp;
        }
        ಹಿಂತಿರುಗಿ stmtp == nodep ? ಶೂನ್ಯಸೂಚಕ : stmtp;
    }
    ಸ್ಥಿರ ತಾರ್ಕಿಕ staticallyNonZeroDelay(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* valuep) {
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ constp = VN_CAST(valuep, Const)) ಹಿಂತಿರುಗಿ !constp->isZero();
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstCond* ಸ್ಥಿರಾಂಕ condp = VN_CAST(valuep, Cond)) {
            ಹಿಂತಿರುಗಿ staticallyNonZeroDelay(condp->thenp())
                   && staticallyNonZeroDelay(condp->elsep());
        }
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstMul* ಸ್ಥಿರಾಂಕ mulp = VN_CAST(valuep, Mul)) {
            ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ lhsConstp = VN_CAST(mulp->lhsp(), Const);
            ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ rhsConstp = VN_CAST(mulp->rhsp(), Const);
            ಆದರೆ (lhsConstp && !lhsConstp->isZero()) ಹಿಂತಿರುಗಿ staticallyNonZeroDelay(mulp->rhsp());
            ಆದರೆ (rhsConstp && !rhsConstp->isZero()) ಹಿಂತಿರುಗಿ staticallyNonZeroDelay(mulp->lhsp());
        }
        ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    }
    // Creates the global delay scheduler variable
    AstVarScope* getCreateDelayScheduler() {
        ಆದರೆ (m_delaySchedp) ಹಿಂತಿರುಗಿ m_delaySchedp;
        ಸ್ವಯಂಚಾಲಿತ* ಸ್ಥಿರಾಂಕ dlySchedDtp = ಹೊಸ AstBasicDType{
            m_scopeTopp->fileline(), VBasicDTypeKwd::DELAY_SCHEDULER, VSigning::UNSIGNED};
        m_netlistp->typeTablep()->ಕೂಡಿಸು_Typesp(dlySchedDtp);
        m_delaySchedp = m_ತುದಿ_Scopep->create_ತಾತ್ಕಾಲಿಕ("__VdlySched", dlySchedDtp);
        // Delay scheduler has to be accessible from top
        m_delaySchedp->varp()->sig_ಬಹಿರಂಗ(ನಿಜ);
        m_netlistp->delaySchedulerp(m_delaySchedp->varp());
        ಹಿಂತಿರುಗಿ m_delaySchedp;
    }
    // Creates the delay sentree
    Ast_Sen_ವೃಕ್ಷ* get_Create_Delay_Sen_ವೃಕ್ಷ() {
        ಆದರೆ (m_delaySensesp) ಹಿಂತಿರುಗಿ m_delaySensesp;
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = m_scopeTopp->fileline();
        ಸ್ವಯಂಚಾಲಿತ* ಸ್ಥಿರಾಂಕ awaitingCurrentTimep
            = ಹೊಸ AstCMethodHard{flp, ಹೊಸ AstVarRef{flp, getCreateDelayScheduler(), VAccess::READ},
                                 VCMethod::SCHED_AWAITING_CURRENT_ಸಮಯ};
        awaitingCurrentTimep->dtype_ಗಣ_ಬಿಟ್();
        m_delaySensesp
            = ಹೊಸ Ast_Sen_ವೃಕ್ಷ{flp, ಹೊಸ Ast_Sen_ಅಂಶ{flp, VEdgeType::ET_TRUE, awaitingCurrentTimep}};
        m_netlistp->ತುದಿ_Scopep()->ಕೂಡಿಸು_Sen_Treesp(m_delaySensesp);
        ಹಿಂತಿರುಗಿ m_delaySensesp;
    }
    // Creates the global dynamic trigger scheduler variable
    AstVarScope* getCreateDynamicTriggerScheduler() {
        ಆದರೆ (m_dynamicSchedp) ಹಿಂತಿರುಗಿ m_dynamicSchedp;
        ಸ್ವಯಂಚಾಲಿತ* ಸ್ಥಿರಾಂಕ dynSchedDtp
            = ಹೊಸ AstBasicDType{m_scopeTopp->fileline(), VBasicDTypeKwd::DYNAMIC_TRIGGER_SCHEDULER,
                                VSigning::UNSIGNED};
        m_netlistp->typeTablep()->ಕೂಡಿಸು_Typesp(dynSchedDtp);
        m_dynamicSchedp = m_ತುದಿ_Scopep->create_ತಾತ್ಕಾಲಿಕ("__VdynSched", dynSchedDtp);
        ಹಿಂತಿರುಗಿ m_dynamicSchedp;
    }
    // Creates the dynamic trigger sentree
    Ast_Sen_ವೃಕ್ಷ* get_Create_Dynamic_Trigger_Sen_ವೃಕ್ಷ() {
        ಆದರೆ (m_dynamicSensesp) ಹಿಂತಿರುಗಿ m_dynamicSensesp;
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = m_scopeTopp->fileline();
        ಸ್ವಯಂಚಾಲಿತ* ಸ್ಥಿರಾಂಕ awaitingCurrentTimep = ಹೊಸ AstCMethodHard{
            flp, ಹೊಸ AstVarRef{flp, getCreateDynamicTriggerScheduler(), VAccess::READ},
            VCMethod::SCHED_EVALUATE};
        awaitingCurrentTimep->dtype_ಗಣ_ಬಿಟ್();
        m_dynamicSensesp
            = ಹೊಸ Ast_Sen_ವೃಕ್ಷ{flp, ಹೊಸ Ast_Sen_ಅಂಶ{flp, VEdgeType::ET_TRUE, awaitingCurrentTimep}};
        m_netlistp->ತುದಿ_Scopep()->ಕೂಡಿಸು_Sen_Treesp(m_dynamicSensesp);
        ಹಿಂತಿರುಗಿ m_dynamicSensesp;
    }
    // Creates the event variable to trigger in NBA region
    AstEventControl* createNbaEventControl(File_ಸಾಲು* flp) {
        ಆದರೆ (!m_netlistp->nbaEventp()) {
            ಸ್ವಯಂಚಾಲಿತ* ಸ್ಥಿರಾಂಕ nbaEventDtp = ಹೊಸ AstBasicDType{m_scopeTopp->fileline(),
                                                        VBasicDTypeKwd::EVENT, VSigning::UNSIGNED};
            m_netlistp->typeTablep()->ಕೂಡಿಸು_Typesp(nbaEventDtp);
            m_netlistp->nbaEventp(m_ತುದಿ_Scopep->create_ತಾತ್ಕಾಲಿಕ("__VnbaEvent", nbaEventDtp));
            v3Global.ಗಣ_Has_Events();
        }
        ಹಿಂತಿರುಗಿ ಹೊಸ AstEventControl{
            flp,
            ಹೊಸ Ast_Sen_ವೃಕ್ಷ{
                flp, ಹೊಸ Ast_Sen_ಅಂಶ{flp, VEdgeType::ET_EVENT,
                                    ಹೊಸ AstVarRef{flp, m_netlistp->nbaEventp(), VAccess::READ}}},
            ಶೂನ್ಯಸೂಚಕ};
    }
    // Creates the variable that, if set, causes the NBA event to be triggered
    Ast_ನಿಯೋಜಿಸು* createNbaEventTriggerAssignment(File_ಸಾಲು* flp) {
        ಆದರೆ (!m_netlistp->nbaEventTriggerp()) {
            m_netlistp->nbaEventTriggerp(m_ತುದಿ_Scopep->create_ತಾತ್ಕಾಲಿಕ("__VnbaEventTrigger", 1));
        }
        ಹಿಂತಿರುಗಿ ಹೊಸ Ast_ನಿಯೋಜಿಸು{flp,
                             ಹೊಸ AstVarRef{flp, m_netlistp->nbaEventTriggerp(), VAccess::WRITE},
                             ಹೊಸ AstConst{flp, AstConst::ಬಿಟ್_True{}}};
    }
    // Returns true if we are under a class or interface function, or the tree references locals.
    // These are cases where static, globally-evaluated triggers are not suitable.
    ತಾರ್ಕಿಕ needDynamicTrigger(Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ nodep) ಸ್ಥಿರಾಂಕ {
        ಹಿಂತಿರುಗಿ m_classp || m_underIfaceCFunc || nodep->exists([](Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ nodep) {
            ಆದರೆ (Ast_ಗಂಟು_Var_Ref* varp = VN_CAST(nodep, ಗಂಟು_Var_Ref)) {
                ಹಿಂತಿರುಗಿ varp->varp()->isFuncLocal();
            }
            ಹಿಂತಿರುಗಿ !nodep->is_ಶುದ್ಧ();
        });
    }
    // Creates a trigger scheduler variable
    AstVarScope* getCreateTriggerSchedulerp(Ast_Sen_ವೃಕ್ಷ* ಸ್ಥಿರಾಂಕ sentreep) {
        ಆದರೆ (!sentreep->user1p()) {
            ಆದರೆ (!m_trigSchedDtp) {
                m_trigSchedDtp
                    = ಹೊಸ AstBasicDType{m_scopeTopp->fileline(), VBasicDTypeKwd::TRIGGER_SCHEDULER,
                                        VSigning::UNSIGNED};
                m_netlistp->typeTablep()->ಕೂಡಿಸು_Typesp(m_trigSchedDtp);
            }
            AstVarScope* ಸ್ಥಿರಾಂಕ trigSchedp
                = m_ತುದಿ_Scopep->create_ತಾತ್ಕಾಲಿಕ(m_trig_Sched_ಹೆಸರುಗಳು.get(sentreep), m_trigSchedDtp);
            sentreep->user1p(trigSchedp);
        }
        ಹಿಂತಿರುಗಿ VN_AS(sentreep->user1p(), VarScope);
    }
    // Creates a string describing the sentree
    AstCExpr* createEventDescription(Ast_Sen_ವೃಕ್ಷ* ಸ್ಥಿರಾಂಕ sentreep) ಸ್ಥಿರಾಂಕ {
        ಆದರೆ (!sentreep->user2p()) {
            std::stringstream ss;
            V3EmitV::verilog_For_ವೃಕ್ಷ(sentreep, ss);
            // possibly a multiline string
            // Names were hashed by V3Name, show the user the original text
            std::string comment = VName::dehash(ss.str());
            std::replace(comment.begin(), comment.ಕೊನೆ(), '\n', ' ');
            // Escape quotes, e.g. from string-indexed event array selects
            comment = '"' + V3_ಹೊರಹೋಗು_Formatter::quoteNameControls(comment) + '"';
            AstCExpr* ಸ್ಥಿರಾಂಕ commentp = ಹೊಸ AstCExpr{sentreep->fileline(), comment};
            commentp->dtype_ಗಣ_String();
            sentreep->user2p(commentp);
            ಹಿಂತಿರುಗಿ commentp;
        }
        ಹಿಂತಿರುಗಿ VN_AS(sentreep->user2p(), CExpr)->clone_ವೃಕ್ಷ(ಸುಳ್ಳು);
    }
    // Adds debug info to a hardcoded method call
    ಶೂನ್ಯ ಕೂಡಿಸು_Debug_Info(AstCMethodHard* ಸ್ಥಿರಾಂಕ methodp) ಸ್ಥಿರಾಂಕ {
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = methodp->fileline();
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ protectIds = v3Global.opt.protectIds();
        AstCExpr* ಸ್ಥಿರಾಂಕ ap
            = ಹೊಸ AstCExpr{flp, protectIds ? "VL_UNKNOWN" : '"' + flp->filenameEsc() + '"'};
        ap->dtype_ಗಣ_String();
        methodp->ಕೂಡಿಸು_Pinsp(ap);
        AstCExpr* ಸ್ಥಿರಾಂಕ bp = ಹೊಸ AstCExpr{flp, protectIds ? "0" : cvt_ಗಮ್ಯ_Str(flp->lineno())};
        bp->dtype_ಗಣ_String();
        methodp->ಕೂಡಿಸು_Pinsp(bp);
    }
    // Adds debug info to a trigSched.trigger() call
    ಶೂನ್ಯ ಕೂಡಿಸು_Event_Debug_Info(AstCMethodHard* ಸ್ಥಿರಾಂಕ methodp, Ast_Sen_ವೃಕ್ಷ* ಸ್ಥಿರಾಂಕ sentreep) ಸ್ಥಿರಾಂಕ {
        ಆದರೆ (v3Global.opt.protectIds()) {
            File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = sentreep->fileline();
            AstCExpr* ಸ್ಥಿರಾಂಕ descp = ಹೊಸ AstCExpr{flp, "VL_UNKNOWN"};
            descp->dtype_ಗಣ_String();
            methodp->ಕೂಡಿಸು_Pinsp(descp);
            ಕೂಡಿಸು_Debug_Info(methodp);
            ಹಿಂತಿರುಗಿ;
        }
        methodp->ಕೂಡಿಸು_Pinsp(createEventDescription(sentreep));
        ಕೂಡಿಸು_Debug_Info(methodp);
    }
    // Adds process pointer to a hardcoded method call
    ಶೂನ್ಯ ಕೂಡಿಸು_Process_Info(AstCMethodHard* ಸ್ಥಿರಾಂಕ methodp) ಸ್ಥಿರಾಂಕ {
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = methodp->fileline();
        AstCExpr* ಸ್ಥಿರಾಂಕ ap
            = ಹೊಸ AstCExpr{flp, (m_procp && m_hasProcess) ? "vlProcess" : "nullptr"};
        methodp->ಕೂಡಿಸು_Pinsp(ap);
    }
    // Creates the fork handle type and returns it
    AstBasicDType* getCreateForkSyncDTypep() {
        ಆದರೆ (m_forkDtp) ಹಿಂತಿರುಗಿ m_forkDtp;
        m_forkDtp = ಹೊಸ AstBasicDType{m_scopeTopp->fileline(), VBasicDTypeKwd::FORK_SYNC,
                                      VSigning::UNSIGNED};
        m_netlistp->typeTablep()->ಕೂಡಿಸು_Typesp(m_forkDtp);
        ಹಿಂತಿರುಗಿ m_forkDtp;
    }
    // Move `nodep` into `AstCLocalScope`.
    AstCLocalScope* ಕೂಡಿಸು_CLocal_Scope(File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp, Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ nodep) ಸ್ಥಿರಾಂಕ {
        ಆದರೆ (!nodep) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
        VNRelinker handle;
        nodep->unlink_Fr_ಹಿಂಭಾಗ(&handle);
        AstCLocalScope* ಸ್ಥಿರಾಂಕ cscopep = ಹೊಸ AstCLocalScope{flp, nodep};
        handle.relink(cscopep);
        ಹಿಂತಿರುಗಿ cscopep;
    }
    // Create a temp variable and optionally put it before the specified node (mark local if so)
    AstVarScope* create_ತಾತ್ಕಾಲಿಕ(File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp, ಸ್ಥಿರಾಂಕ std::string& name,
                            Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ dtypep, Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ ಸೇರಿಸು_Beforep = ಶೂನ್ಯಸೂಚಕ) {
        AstVar* varp;
        ಆದರೆ (ಸೇರಿಸು_Beforep) {
            varp = ಹೊಸ AstVar{flp, VVarType::BLOCKTEMP, name, dtypep};
            varp->funcLocal(ನಿಜ);
            ಸೇರಿಸು_Beforep->ಕೂಡಿಸು_ಇಲ್ಲಿ_This_As_ಮುಂದಿನ(varp);
        } ಇಲ್ಲದಿದ್ದರೆ {
            varp = ಹೊಸ AstVar{flp, VVarType::MODULETEMP, name, dtypep};
            m_scopep->modp()->ಕೂಡಿಸು_Stmtsp(varp);
        }
        AstVarScope* ಸ್ಥಿರಾಂಕ vscp = ಹೊಸ AstVarScope{flp, m_scopep, varp};
        m_scopep->ಕೂಡಿಸು_Varsp(vscp);
        ಹಿಂತಿರುಗಿ vscp;
    }
    // Add a done() call on the fork sync
    ಶೂನ್ಯ ಕೂಡಿಸು_Fork_ಮುಗಿದ(AstBegin* ಸ್ಥಿರಾಂಕ beginp, AstVarScope* ಸ್ಥಿರಾಂಕ forkVscp) ಸ್ಥಿರಾಂಕ {
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = beginp->fileline();
        ಸ್ವಯಂಚಾಲಿತ* ಸ್ಥಿರಾಂಕ donep = ಹೊಸ AstCMethodHard{
            beginp->fileline(), ಹೊಸ AstVarRef{flp, forkVscp, VAccess::WRITE}, VCMethod::FORK_ಮುಗಿದ};
        donep->dtype_ಗಣ_Void();
        ಕೂಡಿಸು_Debug_Info(donep);
        beginp->ಕೂಡಿಸು_Stmtsp(donep->makeStmt());
    }
    ಸ್ಥಿರ ತಾರ್ಕಿಕ has_Disable_ಸರತಿ_ತಳ್ಳು_Self_ಪೂರ್ವಪ್ರತ್ಯಯ(AstBegin* ಸ್ಥಿರಾಂಕ beginp) {
        // LinkJump prepends disable-by-name registration as:
        //   __VprocessQueue_*.push_back(std::process::self())
        // V3LiftExpr lifts std::process::self() into an assignment to a temporary, then V3Task
        // lowers that assignment's function call into a leading comment and AstStmtExpr. Thus the
        // push_back is no longer necessarily the first statement. Scan across this generated
        // prefix for it, stopping at the fork-start sentinel or the branch body. Registrations for
        // nested forks live in sub-blocks and so are not in this statement list.
        ಗಾಗಿ (Ast_ಗಂಟು* stmtp = beginp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ಆದರೆ (VN_IS(stmtp, Comment)) ಮುಂದುವರಿ;
            Ast_Stmt_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ stmtExprp = VN_CAST(stmtp, Stmt_ಅಭಿವ್ಯಕ್ತಿ);
            ಆದರೆ (!stmtExprp) ನಿಲ್ಲಿಸು;
            ಆದರೆ (stmtExprp->is_Disable_ಸರತಿ_ತಳ್ಳು_Self_Stmt()) ಹಿಂತಿರುಗಿ ನಿಜ;
        }
        ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    }
    // Register a callback so killing a process-backed fork branch decrements the join counter
    ಶೂನ್ಯ ಕೂಡಿಸು_Fork_On_Kill(AstBegin* ಸ್ಥಿರಾಂಕ beginp, AstVarScope* ಸ್ಥಿರಾಂಕ forkVscp) ಸ್ಥಿರಾಂಕ {
        ಆದರೆ (!beginp->needProcess() && !has_Disable_ಸರತಿ_ತಳ್ಳು_Self_ಪೂರ್ವಪ್ರತ್ಯಯ(beginp)) ಹಿಂತಿರುಗಿ;
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = beginp->fileline();
        AstCMethodHard* ಸ್ಥಿರಾಂಕ onKillp = ಹೊಸ AstCMethodHard{
            flp, ಹೊಸ AstVarRef{flp, forkVscp, VAccess::WRITE}, VCMethod::FORK_ON_KILL};
        onKillp->dtype_ಗಣ_Void();
        AstCExpr* ಸ್ಥಿರಾಂಕ processp = ಹೊಸ AstCExpr{flp, "vlProcess"};
        processp
            ->dtype_ಗಣ_Void();  // Opaque process reference; type is irrelevant for hardcoded emit
        onKillp->ಕೂಡಿಸು_Pinsp(processp);
        Ast_ಗಂಟು_Stmt* ಸ್ಥಿರಾಂಕ stmtp = onKillp->makeStmt();
        beginp->prependStmtsp(stmtp);
    }
    // Handle the 'join' part of a fork..join
    ಶೂನ್ಯ makeForkJoin(AstFork* ಸ್ಥಿರಾಂಕ forkp) {
        // Create a fork sync var
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = forkp->fileline();
        // Insert the sync var directly before the fork
        Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ ಸೇರಿಸು_Beforep = forkp;
        // Make sure all references to the fork var are contained within a single scope to prevent
        // moving statements accessing it in case of large functions splitting.
        ಕೂಡಿಸು_CLocal_Scope(flp, ಸೇರಿಸು_Beforep);
        AstVarScope* forkVscp
            = create_ತಾತ್ಕಾಲಿಕ(flp, forkp->name() + "__sync", getCreateForkSyncDTypep(), ಸೇರಿಸು_Beforep);
        ಚಿಹ್ನೆರಹಿತ join_ಎಣಿಕೆ = 0;  // Needed for join counter
        // Add a <fork sync>.done() to each begin
        ಗಾಗಿ (Ast_ಗಂಟು* beginp = forkp->forksp(); beginp; beginp = beginp->nextp()) {
            ಕೂಡಿಸು_Fork_On_Kill(VN_AS(beginp, Begin), forkVscp);
            ಕೂಡಿಸು_Fork_ಮುಗಿದ(VN_AS(beginp, Begin), forkVscp);
            join_ಎಣಿಕೆ++;
        }
        ಆದರೆ (forkp->joinType().joinAny()) join_ಎಣಿಕೆ = 1;
        // Set the join counter
        ಸ್ವಯಂಚಾಲಿತ* ಸ್ಥಿರಾಂಕ initp = ಹೊಸ AstCMethodHard{flp, ಹೊಸ AstVarRef{flp, forkVscp, VAccess::WRITE},
                                               VCMethod::FORK_INIT, ಹೊಸ AstConst{flp, join_ಎಣಿಕೆ}};
        initp->dtype_ಗಣ_Void();
        ಕೂಡಿಸು_Process_Info(initp);
        forkp->ಕೂಡಿಸು_ಇಲ್ಲಿ_This_As_ಮುಂದಿನ(initp->makeStmt());
        // Await the join at the end
        ಸ್ವಯಂಚಾಲಿತ* ಸ್ಥಿರಾಂಕ joinp = ಹೊಸ AstCMethodHard{flp, ಹೊಸ AstVarRef{flp, forkVscp, VAccess::WRITE},
                                               VCMethod::FORK_JOIN};
        joinp->dtype_ಗಣ_Void();
        ಕೂಡಿಸು_Process_Info(joinp);
        ಕೂಡಿಸು_Debug_Info(joinp);
        forkp->ಕೂಡಿಸು_ಮುಂದಿನ_ಇಲ್ಲಿ(ಹೊಸ AstCAwait{flp, joinp});
    }

    // `procp` shall be a NodeProcedure/CFunc/Begin and within it vars from `varsp` will be placed.
    // `varsp` vector of vars which shall be localized.
    ಸ್ಥಿರ ಶೂನ್ಯ localizeVars(Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ procp, ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<AstVar*>& varsp) {
        UASSERT(procp, "procp is nullptr");
        AstVar* newVarsp = ಶೂನ್ಯಸೂಚಕ;
        ಗಾಗಿ (AstVar* ಸ್ಥಿರಾಂಕ varp : varsp) {
            varp->funcLocal(ನಿಜ);
            newVarsp = Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ_Null(newVarsp, varp->unlink_Fr_ಹಿಂಭಾಗ());
        }
        ಆದರೆ (Ast_ಗಂಟು_Procedure* ಸ್ಥಿರಾಂಕ ಗಂಟು_Procp = VN_CAST(procp, ಗಂಟು_Procedure)) {
            ಗಂಟು_Procp->prependStmtsp(newVarsp);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (AstCFunc* ಸ್ಥಿರಾಂಕ cfuncp = VN_CAST(procp, CFunc)) {
            cfuncp->prependVarsp(newVarsp);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (AstBegin* ಸ್ಥಿರಾಂಕ beginp = VN_CAST(procp, Begin)) {
            beginp->prependStmtsp(newVarsp);
        } ಇಲ್ಲದಿದ್ದರೆ {
            procp->v3fatalSrc(
                procp->prettyNameQ()
                << " is not of an expected type NodeProcedure/CFunc/Begin instead it is: "
                << procp->prettyTypeName());
        }
    }

    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* delay_ಗಮ್ಯ_Integral(AstDelay* nodep, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* valuep) {
        ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ constp = VN_CAST(valuep, Const);
        // The fork start sentinel is already in ticks
        ಆದರೆ (constp && constp->ಗಮ್ಯ_UQuad() == std::numeric_limits<uint64_t>::max()) ಹಿಂತಿರುಗಿ valuep;
        ಹಿಂತಿರುಗಿ V3Timing::delay_ಗಮ್ಯ_Ticks(valuep, nodep->timeunit());
    }

    // VISITORS
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Module* nodep) override {
        UASSERT_OBJ(!m_classp, nodep, "Module or class under class");
        VL_RESTORER(m_classp);
        m_classp = VN_CAST(nodep, Class);
        VL_RESTORER(m_forkCnt);
        m_forkCnt = 0;
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstScope* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        Sen_ಅಭಿವ್ಯಕ್ತಿ_Builder sen_ಅಭಿವ್ಯಕ್ತಿ_Builder{m_scopep};
        {  // Restore m_senExprBuilderp before destroying senExprBuilder
            VL_RESTORER(m_sen_ಅಭಿವ್ಯಕ್ತಿ_Builderp);
            m_sen_ಅಭಿವ್ಯಕ್ತಿ_Builderp = &sen_ಅಭಿವ್ಯಕ್ತಿ_Builder;
            iterateChildren(nodep);
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಸಕ್ರಿಯ* nodep) override {
        VL_RESTORER(m_activep);
        m_activep = nodep;
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Procedure* nodep) override {
        VL_RESTORER(m_procp);
        VL_RESTORER(m_hasProcess);
        m_procp = nodep;
        m_hasProcess = hasFlags(nodep, T_HAS_PROC);
        VL_RESTORER(m_underProcedure);
        m_underProcedure = ನಿಜ;
        iterateChildren(nodep);
        ಆದರೆ (hasFlags(nodep, T_SUSPENDEE)) nodep->ಗಣ_Suspendable();
        ಆದರೆ (hasFlags(nodep, T_HAS_PROC)) nodep->ಗಣ_Need_Process();
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstInitial* nodep) override {
        ಭೇಟಿ(static_cast<Ast_ಗಂಟು_Procedure*>(nodep));
        ಆದರೆ (nodep->needProcess() && !nodep->user1_ಗಣ_Once()) {
            nodep->ಕೂಡಿಸು_Stmtsp(
                ಹೊಸ AstCStmt{nodep->fileline(), "vlProcess->state(VlProcess::FINISHED);"});
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_Jump_ಖಂಡ* nodep) override {
        VL_RESTORER(m_under_Jump_ಖಂಡ);
        m_under_Jump_ಖಂಡ = ನಿಜ;
        ಭೇಟಿ(static_cast<Ast_ಗಂಟು_Stmt*>(nodep));
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstAlways* nodep) override {
        ಆದರೆ (nodep->user1_ಗಣ_Once()) ಹಿಂತಿರುಗಿ;
        VL_RESTORER(m_procp);
        VL_RESTORER(m_hasProcess);
        m_procp = nodep;
        m_hasProcess = hasFlags(nodep, T_HAS_PROC);
        VL_RESTORER(m_underProcedure);
        m_underProcedure = ನಿಜ;
        // Workaround for killing `always` processes (doing that is pretty much UB)
        // TODO: Disallow killing `always` at runtime (throw an error)
        // Skip for combinational blocks; if the body has timing controls
        // iterateChildren will add T_SUSPENDEE, otherwise it would spin.
        ಆದರೆ (hasFlags(nodep, T_HAS_PROC)
            && !(m_activep && m_activep->sentreep() && m_activep->sentreep()->hasCombo()))
            ಕೂಡಿಸು_Flags(nodep, T_SUSPENDEE);

        iterateChildren(nodep);
        ಆದರೆ (hasFlags(nodep, T_HAS_PROC)) nodep->ಗಣ_Need_Process();
        ಆದರೆ (!hasFlags(nodep, T_SUSPENDEE)) ಹಿಂತಿರುಗಿ;
        nodep->ಗಣ_Suspendable();
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = nodep->fileline();
        Ast_Sen_ವೃಕ್ಷ* ಸ್ಥಿರಾಂಕ sentreep = m_activep->sentreep();
        ಆದರೆ (sentreep->hasClocked()) {
            Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ bodysp = nodep->stmtsp()->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ();
            ಸ್ವಯಂಚಾಲಿತ* ಸ್ಥಿರಾಂಕ controlp = ಹೊಸ AstEventControl{flp, sentreep->clone_ವೃಕ್ಷ(ಸುಳ್ಳು), bodysp};
            nodep->ಕೂಡಿಸು_Stmtsp(controlp);
            iterate(controlp);
        }
        // Note: The 'while (true)' outer loop will be added in V3Sched
        ಸ್ವಯಂಚಾಲಿತ* ಸ್ಥಿರಾಂಕ activep = ಹೊಸ Ast_ಸಕ್ರಿಯ{
            flp, "", ಹೊಸ Ast_Sen_ವೃಕ್ಷ{flp, ಹೊಸ Ast_Sen_ಅಂಶ{flp, Ast_Sen_ಅಂಶ::Initial{}}}};
        activep->sen_ವೃಕ್ಷ_Storep(activep->sentreep());
        activep->ಕೂಡಿಸು_Stmtsp(nodep->unlink_Fr_ಹಿಂಭಾಗ());
        m_activep->ಕೂಡಿಸು_ಮುಂದಿನ_ಇಲ್ಲಿ(activep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCFunc* nodep) override {
        VL_RESTORER(m_procp);
        VL_RESTORER(m_hasProcess);
        VL_RESTORER(m_underIfaceCFunc);
        m_procp = nodep;
        m_hasProcess = hasFlags(nodep, T_HAS_PROC);
        m_underIfaceCFunc = VN_IS(m_scopep->modp(), Iface);
        iterateChildren(nodep);
        ಆದರೆ (hasFlags(nodep, T_HAS_PROC)) nodep->ಗಣ_Need_Process();
        ಆದರೆ (!(hasFlags(nodep, T_SUSPENDEE))) ಹಿಂತಿರುಗಿ;

        nodep->rtnType("VlCoroutine");
        // If in a class, create a shared pointer to 'this'
        ಆದರೆ (m_classp) {
            AstCStmt* ಸ್ಥಿರಾಂಕ cstmtp = ಹೊಸ AstCStmt{nodep->fileline(), "VL_KEEP_THIS;"};
            nodep->prependStmtsp(cstmtp);
        }
        ಆದರೆ (nodep->dpiExportImpl()) {
            nodep->exists([](AstCAwait* ಸ್ಥಿರಾಂಕ awaitp) -> ತಾರ್ಕಿಕ {
                // A DPI-exported coroutine won't be able to block the calling code
                // Error on the await node; fall back to the function node
                awaitp->v3warn(E_UNSUPPORTED,
                               "Unsupported: Timing controls inside DPI-exported tasks");
                ಹಿಂತಿರುಗಿ ನಿಜ;
            });
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_CCall* nodep) override {
        AstCFunc* ಸ್ಥಿರಾಂಕ funcp = nodep->funcp();

        // Skip automatic covergroup sampling calls
        ಆದರೆ (funcp->isCovergroupSample()) {
            iterateChildren(nodep);
            ಹಿಂತಿರುಗಿ;
        }

        ಆದರೆ (funcp->needProcess()) m_hasProcess = ನಿಜ;
        ಆದರೆ (hasFlags(funcp, T_SUSPENDEE) && !nodep->user1_ಗಣ_Once()) {  // If suspendable
            // Calls to suspendables are always void return type, hence parent must be StmtExpr
            Ast_Stmt_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ stmtp = VN_AS(nodep->backp(), Stmt_ಅಭಿವ್ಯಕ್ತಿ);
            stmtp->replaceWith(ಹೊಸ AstCAwait{nodep->fileline(), nodep->unlink_Fr_ಹಿಂಭಾಗ()});
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(stmtp), stmtp);
        }
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstDelay* nodep) override {
        UASSERT_OBJ(!nodep->is_ಚಕ್ರ_Delay(), nodep,
                    "Cycle delays should have been handled in V3AssertPre");
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = nodep->fileline();

        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* valuep = delay_ಗಮ್ಯ_Integral(nodep, nodep->lhsp()->unlink_Fr_ಹಿಂಭಾಗ());

        // Statistics
        ಆದರೆ (valuep->isZero()) {
            ++m_statZeroDelays;
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VN_IS(valuep, Const)) {
            ++m_statConstDelays;
        } ಇಲ್ಲದಿದ್ದರೆ {
            ++m_statVariableDelays;
        }

        // Decide scheduling support for #0
        ಆದರೆ (v3Global.opt.schedZeroDelay().is_ಗಣ_True()) {
            // User said to schedule for #0 support, nothing else to do
            v3Global.ಗಣ_ಬಳಸುತ್ತದೆ_Zero_Delay();
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (v3Global.opt.schedZeroDelay().is_ಗಣ_False()) {
            // User said to schedule without #0 support. Still warn if a static #0 delay exists
            ಆದರೆ (valuep->isZero()) {
                nodep->v3warn(
                    ZERODLY,
                    "Static #0 delay exists, but '--no-sched-zero-delay' was given.\n"
                        << nodep->warn_ಇನ್ನಷ್ಟು()  //
                        << "... Can proceed, but this will fail at runtime if executed.");
            }
        } ಇಲ್ಲದಿದ್ದರೆ {
            // User did not express preference, decide based on presence of delays
            ಆದರೆ (valuep->isZero()) {
                // Statically known #0 delay exists, schedule for #0 support
                v3Global.ಗಣ_ಬಳಸುತ್ತದೆ_Zero_Delay();
                m_hasStaticZeroDelay = ನಿಜ;
                // Don't warn on variable delays, as no point
                m_unknownDelayFlps.clear();
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (staticallyNonZeroDelay(valuep)) {
                // Delay is dynamic, but every statically known outcome is non-zero.
                // So we don't need #0 delay support and there should be no warning.
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (!VN_IS(valuep, Const)) {
                // Delay is not known at compiile time. Conservatively schedule for #0 support,
                // but warn if no static #0 delays used as performance might be improved
                v3Global.ಗಣ_ಬಳಸುತ್ತದೆ_Zero_Delay();
                ಆದರೆ (!m_hasStaticZeroDelay) m_unknownDelayFlps.ತಳ್ಳು_ಹಿಂಭಾಗ(nodep->fileline());
            }
        }

        // Replace self with a 'co_await dlySched.delay(<valuep>)'
        AstCMethodHard* ಸ್ಥಿರಾಂಕ delayMethodp = ಹೊಸ AstCMethodHard{
            flp, ಹೊಸ AstVarRef{flp, getCreateDelayScheduler(), VAccess::WRITE},
            VCMethod::SCHED_DELAY, valuep};
        delayMethodp->dtype_ಗಣ_Void();
        ಕೂಡಿಸು_Process_Info(delayMethodp);
        ಕೂಡಿಸು_Debug_Info(delayMethodp);
        // Create the co_await
        Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ awaitp = ಹೊಸ AstCAwait{flp, delayMethodp, get_Create_Delay_Sen_ವೃಕ್ಷ()};
        // Relink child statements after the co_await
        ಆದರೆ (Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ stmtsp = nodep->stmtsp()) {
            awaitp->ಕೂಡಿಸು_ಮುಂದಿನ(stmtsp->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ());
        }
        nodep->replaceWith(awaitp);
        VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstEventControl* nodep) override {
        // Do not allow waiting on local named events, as they get enqueued for clearing, but can
        // go out of scope before that happens
        ಆದರೆ (!nodep->sentreep()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Event control with implicit sensitivity (@*)"
                          " in this context; use an explicit sensitivity list instead");
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlink_Fr_ಹಿಂಭಾಗ()), nodep);
            ಹಿಂತಿರುಗಿ;
        }
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = nodep->fileline();
        // Relink child statements after the event control
        ಆದರೆ (nodep->stmtsp()) nodep->ಕೂಡಿಸು_ಮುಂದಿನ_ಇಲ್ಲಿ(nodep->stmtsp()->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ());
        ಆದರೆ (needDynamicTrigger(nodep->sentreep())) {
            // Create the trigger variable and init it with 0
            AstVarScope* ಸ್ಥಿರಾಂಕ trigvscp
                = create_ತಾತ್ಕಾಲಿಕ(flp, m_dyn_Trig_ಹೆಸರುಗಳು.get(nodep), nodep->ಕಂಡುಹಿಡಿ_ಬಿಟ್_DType(), nodep);
            ಸ್ವಯಂಚಾಲಿತ* ಸ್ಥಿರಾಂಕ initp = ಹೊಸ Ast_ನಿಯೋಜಿಸು{flp, ಹೊಸ AstVarRef{flp, trigvscp, VAccess::WRITE},
                                              ಹೊಸ AstConst{flp, AstConst::ಬಿಟ್_False{}}};
            // Await the eval step with the dynamic trigger scheduler. First, create the method
            // call
            ಸ್ವಯಂಚಾಲಿತ* ಸ್ಥಿರಾಂಕ evalMethodp = ಹೊಸ AstCMethodHard{
                flp, ಹೊಸ AstVarRef{flp, getCreateDynamicTriggerScheduler(), VAccess::WRITE},
                VCMethod::SCHED_EVALUATION};
            evalMethodp->dtype_ಗಣ_Void();
            ಕೂಡಿಸು_Process_Info(evalMethodp);
            ಸ್ವಯಂಚಾಲಿತ* ಸ್ಥಿರಾಂಕ sentreep = nodep->sentreep();
            ಕೂಡಿಸು_Event_Debug_Info(evalMethodp, sentreep);
            // Create the co_await
            AstCAwait* ಸ್ಥಿರಾಂಕ awaitEvalp
                = ಹೊಸ AstCAwait{flp, evalMethodp, get_Create_Dynamic_Trigger_Sen_ವೃಕ್ಷ()};
            // Construct the sen expression for this sentree
            UASSERT_OBJ(m_sen_ಅಭಿವ್ಯಕ್ತಿ_Builderp, nodep, "No SenExprBuilder for this scope");
            ಸ್ವಯಂಚಾಲಿತ* ಸ್ಥಿರಾಂಕ assignp = ಹೊಸ Ast_ನಿಯೋಜಿಸು{flp, ಹೊಸ AstVarRef{flp, trigvscp, VAccess::WRITE},
                                                m_sen_ಅಭಿವ್ಯಕ್ತಿ_Builderp->build(sentreep).ಮೊದಲ};
            // Get the SenExprBuilder results
            ಸ್ಥಿರಾಂಕ Sen_ಅಭಿವ್ಯಕ್ತಿ_Builder::Results senResults = m_sen_ಅಭಿವ್ಯಕ್ತಿ_Builderp->getAndClearResults();
            localizeVars(m_procp, senResults.m_vars);
            // If post updates are destructive (e.g. clearFired on events), perform a
            // conservative pre-clear once before entering the wait loop so stale state from a
            // previous wait does not cause an immediate false-positive trigger.
            ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ hasDestructivePostUpdates = !senResults.m_destructivePostUpdates.ಖಾಲಿಗಳು();
            // Create the trigger eval loop, which will await the evaluation step and check the
            // trigger
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ condp
                = ಹೊಸ AstLogNot{flp, ಹೊಸ AstVarRef{flp, trigvscp, VAccess::READ}};
            AstLoop* ಸ್ಥಿರಾಂಕ loopp = ಹೊಸ AstLoop{flp};
            loopp->ಕೂಡಿಸು_Stmtsp(ಹೊಸ AstLoopTest{flp, loopp, condp});
            loopp->ಕೂಡಿಸು_Stmtsp(awaitEvalp);
            // Put pre updates before the trigger check and assignment
            ಗಾಗಿ (Ast_ಗಂಟು_Stmt* ಸ್ಥಿರಾಂಕ stmtp : senResults.m_preUpdates) loopp->ಕೂಡಿಸು_Stmtsp(stmtp);
            // Then the trigger check and assignment
            loopp->ಕೂಡಿಸು_Stmtsp(assignp);
            // Let the dynamic trigger scheduler know if this trigger was set
            // If it was, a call to the scheduler's evaluate() will return true
            AstCMethodHard* ಸ್ಥಿರಾಂಕ anyTriggeredMethodp = ಹೊಸ AstCMethodHard{
                flp, ಹೊಸ AstVarRef{flp, getCreateDynamicTriggerScheduler(), VAccess::WRITE},
                VCMethod::SCHED_ANY_TRIGGERED, ಹೊಸ AstVarRef{flp, trigvscp, VAccess::READ}};
            anyTriggeredMethodp->dtype_ಗಣ_Void();
            loopp->ಕೂಡಿಸು_Stmtsp(anyTriggeredMethodp->makeStmt());
            // If the post update is destructive (e.g. event vars are cleared), create an await for
            // the post update step
            ಆದರೆ (hasDestructivePostUpdates) {
                AstCAwait* ಸ್ಥಿರಾಂಕ awaitPostUpdatep = awaitEvalp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು);
                VN_AS(awaitPostUpdatep->exprp(), CMethodHard)->ವಿಧಾನ(VCMethod::SCHED_POST_UPDATE);
                loopp->ಕೂಡಿಸು_Stmtsp(awaitPostUpdatep);
            }
            // Put the post updates at the end of the loop
            ಗಾಗಿ (Ast_ಗಂಟು_Stmt* ಸ್ಥಿರಾಂಕ stmtp : senResults.m_postUpdates) loopp->ಕೂಡಿಸು_Stmtsp(stmtp);
            // Finally, await the resumption step in 'act'
            AstCAwait* ಸ್ಥಿರಾಂಕ awaitResumep = awaitEvalp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು);
            VN_AS(awaitResumep->exprp(), CMethodHard)->ವಿಧಾನ(VCMethod::SCHED_RESUMPTION);
            Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ<Ast_ಗಂಟು_Stmt, Ast_ಗಂಟು_Stmt>(loopp, awaitResumep);
            // Replace the event control with one explicit stmt chain:
            //   init -> [inits] -> [optional destructive pre-clears] -> loop -> awaitResumption
            Ast_ಗಂಟು_Stmt* chainp = ಶೂನ್ಯಸೂಚಕ;
            chainp = Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ_Null(chainp, initp);
            ಗಾಗಿ (Ast_ಗಂಟು_Stmt* ಸ್ಥಿರಾಂಕ stmtp : senResults.m_inits) {
                chainp = Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ_Null(chainp, stmtp);
            }
            ಆದರೆ (hasDestructivePostUpdates) {
                ಗಾಗಿ (Ast_ಗಂಟು_Stmt* ಸ್ಥಿರಾಂಕ stmtp : senResults.m_destructivePostUpdates) {
                    chainp = Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ_Null(chainp, stmtp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು));
                }
            }
            chainp = Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ_Null(chainp, loopp);
            nodep->replaceWith(chainp);
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಸ್ವಯಂಚಾಲಿತ* ಸ್ಥಿರಾಂಕ sentreep = m_finder.get_Sen_ವೃಕ್ಷ(nodep->sentreep());
            nodep->sentreep()->unlink_Fr_ಹಿಂಭಾಗ()->delete_ವೃಕ್ಷ();
            // Get this sentree's trigger scheduler
            // Replace self with a 'co_await trigSched.trigger()'
            ಸ್ವಯಂಚಾಲಿತ* ಸ್ಥಿರಾಂಕ triggerMethodp = ಹೊಸ AstCMethodHard{
                flp, ಹೊಸ AstVarRef{flp, getCreateTriggerSchedulerp(sentreep), VAccess::WRITE},
                VCMethod::SCHED_TRIGGER};
            triggerMethodp->dtype_ಗಣ_Void();
            // If it should be committed immediately, pass true, otherwise false
            triggerMethodp->ಕೂಡಿಸು_Pinsp(nodep->user2() ? ಹೊಸ AstConst{flp, AstConst::ಬಿಟ್_True{}}
                                                    : ಹೊಸ AstConst{flp, AstConst::ಬಿಟ್_False{}});
            ಕೂಡಿಸು_Process_Info(triggerMethodp);
            ಕೂಡಿಸು_Event_Debug_Info(triggerMethodp, sentreep);
            // Create the co_await
            AstCAwait* ಸ್ಥಿರಾಂಕ awaitp = ಹೊಸ AstCAwait{flp, triggerMethodp, sentreep};
            nodep->replaceWith(awaitp);
        }
        VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_ನಿಯೋಜಿಸು* nodep) override {
        // Only process once to avoid infinite loops (due to the net delay)
        ಆದರೆ (nodep->user1_ಗಣ_Once()) ಹಿಂತಿರುಗಿ;
        UINFO(9, "control-visit " << nodep);
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = nodep->fileline();
        Ast_ಗಂಟು* controlp = ಗುಣಕ_ಹೊರಹೋಗು_Timing_Control(nodep);
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ ಒಳ_ನಿಯೋಜಿಸು_Dly = VN_IS(nodep, ನಿಯೋಜಿಸು_Dly);
        // Handle the intra assignment timing control
        // Transform if:
        // * there's a timing control in the assignment
        // * the assignment is an AssignDly and it's in a non-inlined function
        ಆದರೆ (!controlp && (!ಒಳ_ನಿಯೋಜಿಸು_Dly || m_underProcedure)) {
            iterateChildren(nodep);
            ಹಿಂತಿರುಗಿ;
        }
        // The timing control, if any, which is put in a fork for an NBA
        Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ timingp = controlp;
        // Insert new vars before the timing control if we're in a function; in a process we can't
        // do that, as after V3Begin only modules and functions declare variables. These
        // intra-assignment vars will later be passed to forked processes by value.
        Ast_ಗಂಟು* ಸೇರಿಸು_Beforep = m_underProcedure ? ಶೂನ್ಯಸೂಚಕ : controlp;
        // Special case for NBA
        ಆದರೆ (ಒಳ_ನಿಯೋಜಿಸು_Dly) {
            // Put it in a fork so it doesn't block
            // Could already be the only thing directly under a fork, reuse that if possible
            AstFork* forkp = !nodep->nextp() ? VN_CAST(nodep->ಮೊದಲ_Abovep(), Fork) : ಶೂನ್ಯಸೂಚಕ;
            ಆದರೆ (!forkp) forkp = ಹೊಸ AstFork{flp, VJoinType::JOIN_NONE};
            ಆದರೆ (!m_underProcedure) {
                // If it's in a function, it won't be handled by V3Delayed
                // Put it behind an additional named event that gets triggered in the NBA region
                AstEventControl* ಸ್ಥಿರಾಂಕ nbaEventControlp = createNbaEventControl(flp);
                Ast_ನಿಯೋಜಿಸು* ಸ್ಥಿರಾಂಕ trigAssignp = createNbaEventTriggerAssignment(flp);
                nodep->replaceWith(trigAssignp);
                trigAssignp->ಕೂಡಿಸು_ಮುಂದಿನ_ಇಲ್ಲಿ(nbaEventControlp);
                nbaEventControlp->ಕೂಡಿಸು_Stmtsp(nodep);
                ಸೇರಿಸು_Beforep = forkp;
                ಆದರೆ (!controlp) controlp = nbaEventControlp;
            }
            controlp->replaceWith(forkp);
            AstBegin* beginp = VN_CAST(controlp, Begin);
            ಆದರೆ (!beginp) beginp = ಹೊಸ AstBegin{nodep->fileline(), "", controlp, ಸುಳ್ಳು};
            forkp->ಕೂಡಿಸು_Forksp(beginp);
            ಕೂಡಿಸು_Flags(beginp, T_NBA_UPDATE);
            controlp = forkp;
        }
        UASSERT_OBJ(nodep, controlp, "Assignment should have timing control");
        // Move `insertBeforep` into `AstCLocalScope` if necessary to avoid jumping over
        // a variable initialization that would be inserted before `insertBeforep`. All
        // access to this variable should be contained within returned `AstCLocalScope`.
        ಆದರೆ (m_under_Jump_ಖಂಡ) ಕೂಡಿಸು_CLocal_Scope(flp, ಸೇರಿಸು_Beforep);
        // Function for replacing values with intermediate variables
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ replaceWithIntermediate = [&](Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ valuep,
                                                 ಸ್ಥಿರಾಂಕ std::string& name) {
            AstVarScope* ಸ್ಥಿರಾಂಕ newvscp = create_ತಾತ್ಕಾಲಿಕ(flp, name, valuep->dtypep(), ಸೇರಿಸು_Beforep);
            // A pending update keeps its own value, even if its process schedules more
            ಆದರೆ (ಒಳ_ನಿಯೋಜಿಸು_Dly && !ಸೇರಿಸು_Beforep) newvscp->fork_By_ಮೌಲ್ಯ(ನಿಜ);
            valuep->replaceWith(ಹೊಸ AstVarRef{flp, newvscp, VAccess::READ});
            controlp->ಕೂಡಿಸು_ಇಲ್ಲಿ_This_As_ಮುಂದಿನ(
                ಹೊಸ Ast_ನಿಯೋಜಿಸು{flp, ಹೊಸ AstVarRef{flp, newvscp, VAccess::WRITE}, valuep});
        };
        // NBAs with delays evaluate the indices selecting the target now (IEEE 1800-2023 10.4.2),
        // in order, each as a whole, also the selects and function calls in it. So follow the
        // selects of the target up from the variable, not into their indices. As an example, this
        // transforms 'x[a[i]][f(j)] <= #1 y' into 't0 = a[i]; t1 = f(j); x[t0][t1] <= #1 y'.
        // V3Const has split NBAs to concatenations, also streamed, into NBAs to each part.
        ಆದರೆ (ಒಳ_ನಿಯೋಜಿಸು_Dly) {
            ಗಾಗಿ (Ast_ಗಂಟು* exprp = nodep->lhsp()->ತಳ_Fromp(ನಿಜ); exprp != nodep;
                 exprp = exprp->ಮೊದಲ_Abovep()) {
                ಆದರೆ (AstSel* ಸ್ಥಿರಾಂಕ selp = VN_CAST(exprp, Sel)) {
                    // widthp should be const
                    ಆದರೆ (!VN_IS(selp->lsbp(), Const)) {
                        replaceWithIntermediate(selp->lsbp(), m_intra_Lsb_ಹೆಸರುಗಳು.get(nodep));
                    }
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (Ast_ಗಂಟು_Sel* ಸ್ಥಿರಾಂಕ selp = VN_CAST(exprp, ಗಂಟು_Sel)) {
                    ಆದರೆ (!VN_IS(selp->bitp(), Const)) {
                        replaceWithIntermediate(selp->bitp(), m_intra_ಸೂಚ್ಯಂಕ_ಹೆಸರುಗಳು.get(nodep));
                    }
                }
            }
        }
        // Replace the RHS with an intermediate value var
        replaceWithIntermediate(nodep->rhsp(), m_intra_ಮೌಲ್ಯ_ಹೆಸರುಗಳು.get(nodep));
        ಆದರೆ (!ಒಳ_ನಿಯೋಜಿಸು_Dly) ಹಿಂತಿರುಗಿ;
        // The process executing the NBA evaluates its delay (IEEE 1800-2023 4.9.4), not the forked
        // process, if it has side effects, e.g. calling a function that checks its process. A pure
        // one is evaluated as the forked process starts, at once.
        ಆದರೆ (AstDelay* ಸ್ಥಿರಾಂಕ delayp = VN_CAST(timingp, Delay)) {
            ಆದರೆ (!delayp->lhsp()->is_ಶುದ್ಧ()) {
                replaceWithIntermediate(delayp->lhsp(), m_intra_ಮೌಲ್ಯ_ಹೆಸರುಗಳು.get(nodep));
            }
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstFireEvent* nodep) override {
        // V3Delayed handles '->>' of a variable in a process. Like NBAs in non-inlined functions,
        // trigger other events from a fork awaiting the NBA region.
        ಆದರೆ (!nodep->isDelayed() || (m_underProcedure && VN_IS(nodep->operandp(), VarRef))) {
            iterateChildren(nodep);
            ಹಿಂತಿರುಗಿ;
        }
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = nodep->fileline();
        Ast_ನಿಯೋಜಿಸು* ಸ್ಥಿರಾಂಕ trigAssignp = createNbaEventTriggerAssignment(flp);
        nodep->replaceWith(trigAssignp);
        AstFork* ಸ್ಥಿರಾಂಕ forkp = ಹೊಸ AstFork{flp, VJoinType::JOIN_NONE};
        trigAssignp->ಕೂಡಿಸು_ಮುಂದಿನ_ಇಲ್ಲಿ(forkp);
        ಆದರೆ (m_under_Jump_ಖಂಡ) ಕೂಡಿಸು_CLocal_Scope(flp, forkp);
        // The triggered event is the one referenced now, so evaluate handles and indices now
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ eventp = nodep->operandp()->unlink_Fr_ಹಿಂಭಾಗ();
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ evalNow = [&](Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ valuep, V3_Unique_ಹೆಸರುಗಳು& ಹೆಸರುಗಳು) {
            // Like for an NBA, a process uses a module variable, passed to the fork by value
            AstVarScope* ಸ್ಥಿರಾಂಕ vscp = create_ತಾತ್ಕಾಲಿಕ(flp, ಹೆಸರುಗಳು.get(nodep), valuep->dtypep(),
                                                 m_underProcedure ? ಶೂನ್ಯಸೂಚಕ : forkp);
            ಆದರೆ (m_underProcedure) vscp->fork_By_ಮೌಲ್ಯ(ನಿಜ);
            valuep->replaceWith(ಹೊಸ AstVarRef{flp, vscp, VAccess::READ});
            // Unlike an event, a handle selecting one is only read
            valuep->foreach([](Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ np) {
                ಆದರೆ (Ast_ಗಂಟು_Var_Ref* ಸ್ಥಿರಾಂಕ refp = VN_CAST(np, ಗಂಟು_Var_Ref)) {
                    refp->access(VAccess::READ);
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (AstMemberSel* ಸ್ಥಿರಾಂಕ selp = VN_CAST(np, MemberSel)) {
                    selp->access(VAccess::READ);
                }
            });
            forkp->ಕೂಡಿಸು_ಇಲ್ಲಿ_This_As_ಮುಂದಿನ(
                ಹೊಸ Ast_ನಿಯೋಜಿಸು{flp, ಹೊಸ AstVarRef{flp, vscp, VAccess::WRITE}, valuep});
        };
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* refp = eventp;
        ಆಗ (Ast_ಗಂಟು_Sel* ಸ್ಥಿರಾಂಕ selp = VN_CAST(refp, ಗಂಟು_Sel)) {
            ಆದರೆ (!VN_IS(selp->bitp(), Const)) evalNow(selp->bitp(), m_intra_ಸೂಚ್ಯಂಕ_ಹೆಸರುಗಳು);
            refp = selp->fromp();
        }
        // Handle of a class (or virtual interface)
        ಆದರೆ (AstMemberSel* ಸ್ಥಿರಾಂಕ selp = VN_CAST(refp, MemberSel)) {
            evalNow(selp->fromp(), m_intra_ಮೌಲ್ಯ_ಹೆಸರುಗಳು);
        }
        AstEventControl* ಸ್ಥಿರಾಂಕ controlp = createNbaEventControl(flp);
        controlp->ಕೂಡಿಸು_Stmtsp(ಹೊಸ AstFireEvent{flp, eventp, ಸುಳ್ಳು});
        AstBegin* ಸ್ಥಿರಾಂಕ beginp = ಹೊಸ AstBegin{flp, "", controlp, ಸುಳ್ಳು};
        ಕೂಡಿಸು_Flags(beginp, T_NBA_UPDATE);
        forkp->ಕೂಡಿಸು_Forksp(beginp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ನಿಯೋಜಿಸು_W* nodep) override {
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = nodep->fileline();
        // Get the net delay unless this assignment was created for handling the net delay (user1)
        AstDelay* ಸ್ಥಿರಾಂಕ netDelayp = nodep->user1() ? ಶೂನ್ಯಸೂಚಕ : nodep->getLhsNetDelay();
        ಆದರೆ (netDelayp) {
            ಆದರೆ (nodep->timingControlp()) {
                // If this assignment has a delay, create another one to handle the net delay
                AstVarScope* ಸ್ಥಿರಾಂಕ newvscp = m_ನಿಯೋಜಿಸು_WTmps.make(flp, m_scopep, nodep->dtypep());
                Ast_ನಿಯೋಜಿಸು_W* assignp = ಹೊಸ Ast_ನಿಯೋಜಿಸು_W{
                    nodep->fileline(), nodep->lhsp()->unlink_Fr_ಹಿಂಭಾಗ(),
                    ಹೊಸ AstVarRef{flp, newvscp, VAccess::READ}, netDelayp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು)};
                assignp->user1(ನಿಜ);
                nodep->ಕೂಡಿಸು_ಮುಂದಿನ_ಇಲ್ಲಿ(assignp);
                nodep->lhsp(ಹೊಸ AstVarRef{flp, newvscp, VAccess::WRITE});
            } ಇಲ್ಲದಿದ್ದರೆ {
                // Else just use this one with the net delay
                nodep->timingControlp(netDelayp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು));
            }
        }
        ಆದರೆ (!nodep->timingControlp()) ಹಿಂತಿರುಗಿ;
        // There will be some circular logic here, suppress the warning for newly created vars
        flp->modifyWarnOff(V3_Error_ಸಂಕೇತ::UNOPTFLAT, ನಿಜ);
        // Also suppress the warning for the LHS var, for cases like `assign #1 clk = ~clk;`
        // TODO: Restore the warning for other, non-delayed drivers
        nodep->lhsp()->foreach([](AstVarRef* refp) {
            ಆದರೆ (refp->access().isWriteOrRW()) {
                refp->varp()->fileline()->modifyWarnOff(V3_Error_ಸಂಕೇತ::UNOPTFLAT, ನಿಜ);
            }
        });
        // Should be under an always
        AstAlways* ಸ್ಥಿರಾಂಕ alwaysp = VN_AS(m_procp, Always);
        // Convert it to an Assign; the new assign with intra delay will be handled by
        // visit(AstNodeAssign*)
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ lhs1p = nodep->lhsp()->unlink_Fr_ಹಿಂಭಾಗ();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ rhs1p = nodep->rhsp()->unlink_Fr_ಹಿಂಭಾಗ();
        Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ controlp = nodep->timingControlp()->unlink_Fr_ಹಿಂಭಾಗ();
        ಆದರೆ (AstDelay* ಸ್ಥಿರಾಂಕ delayp = VN_CAST(controlp, Delay)) {
            ಆದರೆ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* fallDelayp = delayp->fallDelay()) {
                fallDelayp = fallDelayp->unlink_Fr_ಹಿಂಭಾಗ();
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp = delayp->lhsp()->unlink_Fr_ಹಿಂಭಾಗ();
                // Use fall only for an all-zero value, rise otherwise.
                lhsp = delay_ಗಮ್ಯ_Integral(delayp, lhsp);
                fallDelayp = delay_ಗಮ್ಯ_Integral(delayp, fallDelayp);
                delayp->lhsp(
                    ಹೊಸ AstCond{flp, ಹೊಸ AstEq{flp, rhs1p->clone_ವೃಕ್ಷ(ಸುಳ್ಳು), ಹೊಸ AstConst{flp, 0}},
                                fallDelayp, lhsp});
            }
        }
        Ast_ನಿಯೋಜಿಸು* ಸ್ಥಿರಾಂಕ assignp = ಹೊಸ Ast_ನಿಯೋಜಿಸು{nodep->fileline(), lhs1p, rhs1p, controlp};
        // Put the assignment in a fork..join_none.
        AstFork* ಸ್ಥಿರಾಂಕ forkp = ಹೊಸ AstFork{flp, VJoinType::JOIN_NONE};
        nodep->replaceWith(forkp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        AstBegin* ಸ್ಥಿರಾಂಕ beginp = ಹೊಸ AstBegin{flp, "", assignp, ಸುಳ್ಳು};
        forkp->ಕೂಡಿಸು_Forksp(beginp);
        ಭೇಟಿ(forkp);  // Visit now as we need to do some post-processing
        // IEEE 1800-2023 10.3.3 - if the RHS value differs from the currently scheduled value to
        // be assigned, the currently scheduled assignment is descheduled. To keep track if an
        // assignment should be descheduled, each scheduled assignment event has a 'generation',
        // and if at assignment time its generation differs from the current generation, it won't
        // be performed
        Ast_ನಿಯೋಜಿಸು* ಸ್ಥಿರಾಂಕ preAssignp = VN_AS(beginp->stmtsp(), ನಿಯೋಜಿಸು);
        UASSERT_OBJ(preAssignp, alwaysp, "Pre-assign should be there from visit(AstFork)");
        Ast_ನಿಯೋಜಿಸು* ಸ್ಥಿರಾಂಕ postAssignp = VN_AS(preAssignp->nextp()->nextp(), ನಿಯೋಜಿಸು);
        UASSERT_OBJ(postAssignp, alwaysp, "Post-assign should be there from visit(AstFork)");
        // Increment generation and copy it to a local
        AstVarScope* ಸ್ಥಿರಾಂಕ generationVarp
            = m_ನಿಯೋಜಿಸು_WGens.make(flp, m_scopep, alwaysp->ಕಂಡುಹಿಡಿ_UInt64_DType());
        AstVarScope* ಸ್ಥಿರಾಂಕ genLocalVarp
            = create_ತಾತ್ಕಾಲಿಕ(flp, generationVarp->varp()->name() + "__local",
                         alwaysp->ಕಂಡುಹಿಡಿ_UInt64_DType(), preAssignp);
        preAssignp->ಕೂಡಿಸು_ಇಲ್ಲಿ_This_As_ಮುಂದಿನ(
            ಹೊಸ Ast_ನಿಯೋಜಿಸು{flp, ಹೊಸ AstVarRef{flp, generationVarp, VAccess::WRITE},
                          ಹೊಸ Ast_ಕೂಡಿಸು{flp, ಹೊಸ AstVarRef{flp, generationVarp, VAccess::READ},
                                     ಹೊಸ AstConst{flp, 1}}});
        preAssignp->ಕೂಡಿಸು_ಇಲ್ಲಿ_This_As_ಮುಂದಿನ(
            ಹೊಸ Ast_ನಿಯೋಜಿಸು{flp, ಹೊಸ AstVarRef{flp, genLocalVarp, VAccess::WRITE},
                          ಹೊಸ AstVarRef{flp, generationVarp, VAccess::READ}});
        // If the current generation is same as the one saved in the local var, assign
        beginp->ಕೂಡಿಸು_Stmtsp(
            ಹೊಸ AstIf{flp,
                      ಹೊಸ AstEq{flp, ಹೊಸ AstVarRef{flp, generationVarp, VAccess::READ},
                                ಹೊಸ AstVarRef{flp, genLocalVarp, VAccess::READ}},
                      postAssignp->unlink_Fr_ಹಿಂಭಾಗ()});
        // Save scheduled RHS value before delay
        AstVarScope* ಸ್ಥಿರಾಂಕ tmpVarp
            = m_ನಿಯೋಜಿಸು_WTmps.make(flp, m_scopep, preAssignp->rhsp()->dtypep());
        AstVarRef* ಸ್ಥಿರಾಂಕ tmp_ನಿಯೋಜಿಸು_Rhsp = VN_AS(preAssignp->lhsp(), VarRef)->clone_ವೃಕ್ಷ(ಸುಳ್ಳು);
        tmp_ನಿಯೋಜಿಸು_Rhsp->access(VAccess::WRITE);
        preAssignp->ಕೂಡಿಸು_ಮುಂದಿನ_ಇಲ್ಲಿ(
            ಹೊಸ Ast_ನಿಯೋಜಿಸು{flp, ಹೊಸ AstVarRef{flp, tmpVarp, VAccess::WRITE}, tmp_ನಿಯೋಜಿಸು_Rhsp});
        // If the RHS is different from the currently scheduled value, schedule the new assignment
        // The generation will increase, effectively 'descheduling' the previous assignment.
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ didNotInitp
            = ಹೊಸ AstLogNot{flp, ಹೊಸ AstCExpr{flp, "vlSymsp->__Vm_didInit", 1}};
        AstVarRef* ಸ್ಥಿರಾಂಕ ಮೊದಲ_Iterp
            = ಹೊಸ AstVarRef{flp, m_netlistp->stl_ಮೊದಲ_Iterationp(), VAccess::READ};
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ schedCondp
            = ಹೊಸ AstLogOr{flp,
                           ಹೊಸ AstNeq{flp, preAssignp->rhsp()->clone_ವೃಕ್ಷ(ಸುಳ್ಳು),
                                      ಹೊಸ AstVarRef{flp, tmpVarp, VAccess::READ}},
                           ಹೊಸ AstLogAnd{flp, didNotInitp, ಮೊದಲ_Iterp}};
        alwaysp->ಕೂಡಿಸು_Stmtsp(ಹೊಸ AstIf{flp, schedCondp, forkp->unlink_Fr_ಹಿಂಭಾಗ()});
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstDisableFork* nodep) override {
        ಆದರೆ (m_hasProcess) ಹಿಂತಿರುಗಿ;
        // never reached by any process; remove to avoid compilation error
        VL_DO_DANGLING(nodep->unlink_Fr_ಹಿಂಭಾಗ()->delete_ವೃಕ್ಷ(), nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstWaitFork* nodep) override {
        ಆದರೆ (m_hasProcess) {
            File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = nodep->fileline();
            AstCExpr* ಸ್ಥಿರಾಂಕ exprp = ಹೊಸ AstCExpr{flp, "vlProcess->completedFork()", 1};
            AstWait* ಸ್ಥಿರಾಂಕ waitp = ಹೊಸ AstWait{flp, exprp, ಶೂನ್ಯಸೂಚಕ};
            nodep->replaceWith(waitp);
        } ಇಲ್ಲದಿದ್ದರೆ {
            // never reached by any process; remove to avoid compilation error
            nodep->unlink_Fr_ಹಿಂಭಾಗ();
        }
        VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstWait* nodep) override {
        // Wait on changed events related to the vars in the wait statement
        UINFO(9, "control-visit " << nodep);
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = nodep->fileline();
        Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ stmtsp = nodep->stmtsp();
        ಆದರೆ (stmtsp) stmtsp->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ condp = V3Const::constifyEdit(nodep->condp()->unlink_Fr_ಹಿಂಭಾಗ());
        ಸ್ವಯಂಚಾಲಿತ* ಸ್ಥಿರಾಂಕ constp = VN_CAST(condp, Const);
        ಆದರೆ (constp) {
            ಆದರೆ (constp->isZero()) {
                // We have to await forever instead of simply returning in case we're deep in a
                // callstack
                AstCMethodHard* ಸ್ಥಿರಾಂಕ foreverMethodp = ಹೊಸ AstCMethodHard{
                    flp, ಹೊಸ AstVarRef{flp, getCreateDelayScheduler(), VAccess::WRITE},
                    VCMethod::SCHED_WAIT_FOREVER};
                foreverMethodp->dtype_ಗಣ_Void();
                ಕೂಡಿಸು_Process_Info(foreverMethodp);
                ಕೂಡಿಸು_Debug_Info(foreverMethodp);
                AstCAwait* ಸ್ಥಿರಾಂಕ awaitp = ಹೊಸ AstCAwait{flp, foreverMethodp};
                nodep->replaceWith(awaitp);
                ಆದರೆ (stmtsp) VL_DO_DANGLING(stmtsp->delete_ವೃಕ್ಷ(), stmtsp);
                VL_DO_DANGLING(condp->delete_ವೃಕ್ಷ(), condp);
            } ಇಲ್ಲದಿದ್ದರೆ {
                nodep->v3fatalSrc("constant wait should have been removed in "
                                  "TimingSuspendableVisitor::visit(AstWait)");
            }
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (needDynamicTrigger(condp)) {
            // No point in making a sentree, just use the expression as sensitivity
            // Put the event control in an if so we only wait if the condition isn't met already
            ಸ್ವಯಂಚಾಲಿತ* ಸ್ಥಿರಾಂಕ ifp = ಹೊಸ AstIf{
                flp, ಹೊಸ AstLogNot{flp, condp},
                ಹೊಸ AstEventControl{flp,
                                    ಹೊಸ Ast_Sen_ವೃಕ್ಷ{flp, ಹೊಸ Ast_Sen_ಅಂಶ{flp, VEdgeType::ET_TRUE,
                                                                       condp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು)}},
                                    ಶೂನ್ಯಸೂಚಕ}};
            ಆದರೆ (stmtsp) Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ<Ast_ಗಂಟು, Ast_ಗಂಟು>(ifp, stmtsp);
            nodep->replaceWith(ifp);
        } ಇಲ್ಲದಿದ್ದರೆ {
            // We are using a global sentree, so we cannot use ET_TRUE, as that could lead to the
            // active region never converging. Because of this, we need to use ET_CHANGED in a
            // loop.
            AstEventControl* ಸ್ಥಿರಾಂಕ controlp = ಹೊಸ AstEventControl{
                flp,
                ಹೊಸ Ast_Sen_ವೃಕ್ಷ{
                    flp, ಹೊಸ Ast_Sen_ಅಂಶ{flp, VEdgeType::ET_ಬದಲಾಯಿತು, condp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು)}},
                ಶೂನ್ಯಸೂಚಕ};
            controlp->user2(ನಿಜ);  // Commit immediately
            AstLoop* ಸ್ಥಿರಾಂಕ loopp = ಹೊಸ AstLoop{flp};
            loopp->ಕೂಡಿಸು_Stmtsp(ಹೊಸ AstLoopTest{flp, loopp, ಹೊಸ AstLogNot{flp, condp}});
            loopp->ಕೂಡಿಸು_Stmtsp(controlp);
            ಆದರೆ (stmtsp) Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ<Ast_ಗಂಟು, Ast_ಗಂಟು>(loopp, stmtsp);
            nodep->replaceWith(loopp);
        }
        VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstBegin* nodep) override {
        VL_RESTORER(m_procp);
        VL_RESTORER(m_hasProcess);
        m_hasProcess |= hasFlags(nodep, T_HAS_PROC);
        // A pending NBA update is not a subprocess of the process that scheduled it, so it is
        // unaffected by 'disable fork' and 'wait fork' (IEEE 1800-2023 9.6.1, 9.6.3)
        ಆದರೆ (hasFlags(nodep, T_NBA_UPDATE)) m_hasProcess = ಸುಳ್ಳು;
        m_procp = nodep;
        ಆದರೆ (m_hasProcess) nodep->ಗಣ_Need_Process();
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstFork* nodep) override {
        ಆದರೆ (nodep->user1_ಗಣ_Once()) ಹಿಂತಿರುಗಿ;
        UINFO(9, "control-visit " << nodep);
        v3Global.ಗಣ_ಬಳಸುತ್ತದೆ_Timing();

        // Create a unique name for this fork
        nodep->name("__Vfork_" + std::ಗಮ್ಯ_string(++m_forkCnt));

        // TODO: Should process nodep->stmtsp() in case an earlier pass
        //       inserted something there, but as of today we don't need to.

        // Process and name each fork
        ಗಾತ್ರ_t idx = 0;  // Index for naming begins
        ಗಾಗಿ (AstBegin *itemp = nodep->forksp(), *nextp; itemp; itemp = nextp) {
            nextp = VN_AS(itemp->nextp(), Begin);
            iterate(itemp);
            // Note: Even if we do not find any awaits, we cannot simply inline
            // the process here, as new awaits could be added later.

            // Name the begin (later the name will be used for a new function)
            itemp->name(nodep->name() + "__" + std::ಗಮ್ಯ_string(idx++));
            ಆದರೆ (itemp->needProcess()) {
                itemp->ಕೂಡಿಸು_Stmtsp(
                    ಹೊಸ AstCStmt{itemp->fileline(), "vlProcess->state(VlProcess::FINISHED);"});
            }
        }
        ಆದರೆ (!nodep->joinType().joinNone()) makeForkJoin(nodep);
    }

    //--------------------
    ಶೂನ್ಯ ಭೇಟಿ(AstVar*) override {}  // Accelerate
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    ಸ್ಪಷ್ಟ TimingControlVisitor(AstNetlist* nodep)
        : m_netlistp{nodep} {
        iterate(nodep);

        // If there is no static #0 in the design, but an unknown delay was found,
        // and the user did not specify preference, then warn on all unknown delays
        // as we will be assuming they can be #0, which can cause performance degradation.
        ಗಾಗಿ (File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp : m_unknownDelayFlps) {
            flp->v3warn(ZERODLY,
                        "Value of # delay control statically unknown. Assuming it can be #0.\n"
                            << flp->warn_ಇನ್ನಷ್ಟು()  //
                            << "... If all # delays are non-zero at runtime,\n"
                            << flp->warn_ಇನ್ನಷ್ಟು()  //
                            << "... use '--no-sched-zero-delay' for improved performance.\n"
                            << flp->warn_ಇನ್ನಷ್ಟು()  //
                            << "... If a real #0 is expected at runtime,\n"
                            << flp->warn_ಇನ್ನಷ್ಟು()  //
                            << "... use '--sched-zero-delay' to suppress this warning.");
        }
    }
    ~TimingControlVisitor() override {
        V3Stats::ಕೂಡಿಸು_Stat("Timing, known #0 delays", m_statZeroDelays);
        V3Stats::ಕೂಡಿಸು_Stat("Timing, known #const delays", m_statConstDelays);
        V3Stats::ಕೂಡಿಸು_Stat("Timing, unknown #variable delays", m_statVariableDelays);
    }
};

//######################################################################
// Timing class functions

Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* V3Timing::delay_ಗಮ್ಯ_Ticks(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* valuep, VTimescale timeunit) {
    File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = valuep->fileline();
    ಆದರೆ (ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ constp = VN_CAST(valuep, Const)) {
        ಆದರೆ (constp->ಅಂಕೆ().is1_ಹೆಜ್ಜೆ()) {
            // A step is one tick of the global time precision (IEEE 1800-2023 3.14.3)
            VL_DO_DANGLING(valuep->delete_ವೃಕ್ಷ(), valuep);
            AstConst* ಸ್ಥಿರಾಂಕ tickp = ಹೊಸ AstConst{flp, AstConst::Unsized64{}, 1};
            tickp->dtype_ಗಣ_ಬಿಟ್_Sized(64, VSigning::UNSIGNED);
            ಹಿಂತಿರುಗಿ tickp;
        }
        ಆದರೆ (constp->isZero()) ಹಿಂತಿರುಗಿ valuep;
    }
    UASSERT_OBJ(!timeunit.isNone(), valuep, "timeunit must be set");
    ಸ್ಥಿರಾಂಕ ದ್ವಿಗುಣ timescale_ಗುಣಕ_D
        = std::ಘಾತ(10.0, timeunit.power_ದ_Ten() - v3Global.rootp()->timeprecision().power_ದ_Ten());
    ಆದರೆ (valuep->dtypep()->skipRefp()->isDouble()) {
        AstConst* ಸ್ಥಿರಾಂಕ tsfp = ಹೊಸ AstConst{flp, AstConst::RealDouble{}, timescale_ಗುಣಕ_D};
        valuep = ಹೊಸ AstMulD{flp, valuep, tsfp};
        valuep = ಹೊಸ AstRToIRoundS{flp, valuep};
        valuep->dtype_ಗಣ_ಬಿಟ್_Sized(64, VSigning::UNSIGNED);
    } ಇಲ್ಲದಿದ್ದರೆ {
        valuep->dtype_ಗಣ_ಬಿಟ್_Sized(64, VSigning::UNSIGNED);
        ಸ್ಥಿರಾಂಕ uint64_t timescale_ಗುಣಕ_U = static_cast<uint64_t>(timescale_ಗುಣಕ_D);
        AstConst* ಸ್ಥಿರಾಂಕ tsfp = ಹೊಸ AstConst{flp, AstConst::Unsized64{}, timescale_ಗುಣಕ_U};
        valuep = ಹೊಸ AstMul{flp, valuep, tsfp};
    }
    ಹಿಂತಿರುಗಿ V3Const::constifyEdit(valuep);  // Simplify
}

ಶೂನ್ಯ V3Timing::timingAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        ಸ್ಥಿರಾಂಕ VNUser1_ಒಳ_Use m_user1_ಒಳ_Use;
        ಸ್ಥಿರಾಂಕ VNUser2_ಒಳ_Use m_user2_ಒಳ_Use;
        { TimingSuspendableVisitor{nodep}; }
        ಆದರೆ (v3Global.ಬಳಸುತ್ತದೆ_Timing()) TimingControlVisitor{nodep};
    }
    V3Global::dump_Check_Global_ವೃಕ್ಷ("timing", 0, dump_ವೃಕ್ಷ_Either_Level() >= 3);
}
