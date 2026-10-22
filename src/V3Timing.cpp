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

#అన్తర్భావ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#అన్తర్భావ "V3Timing.h"

#అన్తర్భావ "V3Const.h"
#అన్తర్భావ "V3EmitV.h"
#అన్తర్భావ "V3File.h"
#అన్తర్భావ "V3Global.h"
#అన్తర్భావ "V3Graph.h"
#అన్తర్భావ "V3MemberMap.h"
#అన్తర్భావ "V3SenExprBuilder.h"
#అన్తర్భావ "V3SenTree.h"
#అన్తర్భావ "V3SharedTmps.h"
#అన్తర్భావ "V3Stats.h"
#అన్తర్భావ "V3UniqueNames.h"

#అన్తర్భావ <limits>
#అన్తర్భావ <queue>

VL_DEFINE_DEBUG_FUNCTIONS;

// ######################################################################

గణన కణుపు_Flag : uint8_t {
    T_SUSPENDEE = 1 << 0,  // Suspendable (due to dependence on another suspendable)
    T_SUSPENDER = 1 << 1,  // Suspendable (has timing control)
    T_ALLOCS_PROC = 1 << 2,  // Can allocate VlProcess
    T_FORCES_PROC = 1 << 3,  // Forces VlProcess allocation
    T_NEEDS_PROC = 1 << 4,  // Needs access to VlProcess if it's allocated
    T_HAS_PROC = 1 << 5,  // Has VlProcess argument in the signature
    T_NBA_UPDATE = 1 << 6,  // Fork branch of a pending NBA update, which is not a subprocess
};

గణన ForkType : uint8_t {
    F_NONE = 0,  // Not under a fork
    F_MIGHT_SUSPEND = 1 << 0,  // Fork might suspend the execution of current process
    F_MIGHT_NEED_PROC = 1 << 1,  // Fork might need a process (any fork really)
};

గణన ప్రసరణ_Type : uint8_t {
    P_CALL = 1,  // Propagation through call to a function/task/method
    P_FORK = 2,  // Propagation due to fork's behavior
    P_SIGNATURE = 3,  // Propagation required to maintain C++ function's signature requirements
};

// Add timing flag to a node
స్థిర శూన్య కూడు_Flags(Ast_కణుపు* స్థిరరాశి nodep, uint8_t flags) { nodep->user2Or(flags); }
// Check if a node has ALL of the expected flags set
స్థిర తార్కిక hasFlags(Ast_కణుపు* స్థిరరాశి nodep, uint8_t flags) { తిరిగి !(~nodep->user2() & flags); }

// ######################################################################
//  Detect nodes affected by timing and/or requiring a process

తరగతి TimingSuspendableVisitor final : బహిరంగ VNVisitor {
    // TYPES
    // Vertex of a dependency graph of suspendable nodes, e.g. if a node (process or task) is
    // suspendable, all its dependents should also be suspendable
    తరగతి DepVtx VL_NOT_FINAL : బహిరంగ V3_లేఖాచిత్రం_శీర్షం {
        VL_RTTI_IMPL(DepVtx, V3_లేఖాచిత్రం_శీర్షం)
        AstClass* స్థిరరాశి m_classp;  // Class associated with a method
        Ast_కణుపు* స్థిరరాశి m_nodep;  // AST node represented by this graph vertex

        // ACCESSORS
        string name() స్థిరరాశి override {
            ఒకవేళ (m_classp) {
                ఒకవేళ (VN_IS(nodep(), CFunc)) {
                    తిరిగి cvt_గమ్యం_Hex(nodep()) + ' ' + classp()->name() + "::" + nodep()->name();
                }
            }
            తిరిగి cvt_గమ్యం_Hex(nodep()) + ' ' + nodep()->prettyTypeName();
        }
        File_పంక్తి* fileline() స్థిరరాశి override { తిరిగి nodep()->fileline(); }

    బహిరంగ:
        // CONSTRUCTORS
        DepVtx(V3_లేఖాచిత్రం* graphp, Ast_కణుపు* nodep, AstClass* classp)
            : V3_లేఖాచిత్రం_శీర్షం{graphp}
            , m_classp{classp}
            , m_nodep{nodep} {}
        ~DepVtx() override = అప్రమేయ;

        // ACCESSORS
        మాయా Ast_కణుపు* nodep() స్థిరరాశి VL_MT_STABLE { తిరిగి m_nodep; }
        మాయా Ast_కణుపు* classp() స్థిరరాశి VL_MT_STABLE { తిరిగి m_classp; }
    };

    తరగతి SuspendDepVtx final : బహిరంగ DepVtx {
        VL_RTTI_IMPL(SuspendDepVtx, DepVtx)
        string dotColor() స్థిరరాశి override {
            ఒకవేళ (hasFlags(nodep(), T_SUSPENDER)) తిరిగి "red";
            ఒకవేళ (hasFlags(nodep(), T_SUSPENDEE)) తిరిగి "blue";
            తిరిగి "black";
        }

    బహిరంగ:
        SuspendDepVtx(V3_లేఖాచిత్రం* graphp, Ast_కణుపు* nodep, AstClass* classp)
            : DepVtx{graphp, nodep, classp} {}
        ~SuspendDepVtx() override = అప్రమేయ;
    };

    తరగతి NeedsProcDepVtx final : బహిరంగ DepVtx {
        VL_RTTI_IMPL(NeedsProcDepVtx, DepVtx)
        string dotColor() స్థిరరాశి override {
            ఒకవేళ (hasFlags(nodep(), T_HAS_PROC)) తిరిగి "blue";
            ఒకవేళ (hasFlags(nodep(), T_NEEDS_PROC)) తిరిగి "green";
            ఒకవేళ (hasFlags(nodep(), T_FORCES_PROC)) తిరిగి "red";
            తిరిగి "black";
        }

    బహిరంగ:
        NeedsProcDepVtx(V3_లేఖాచిత్రం* graphp, Ast_కణుపు* nodep, AstClass* classp)
            : DepVtx{graphp, nodep, classp} {}
        ~NeedsProcDepVtx() override = అప్రమేయ;
    };

    // NODE STATE
    //  AstClass::user1() -> bool.  Class member cache has been refreshed.
    //  Ast{NodeProcedure,CFunc,Begin}::user2()  -> uint64_t.  Set to >= T_SUSP if
    //                                                         process/task suspendable
    //                                                         and to T_PROC if it
    //                                                         needs process metadata.
    //  Ast{NodeProcedure,CFunc,Begin}::user3()  -> DependencyVertex*.  Vertex in m_suspGraph
    //  Ast{NodeProcedure,CFunc,Begin}::user3()  -> DependencyVertex*.  Vertex in m_procGraph
    స్థిరరాశి VNUser3_లోపల_Use m_user3_లోపల_Use;
    స్థిరరాశి VNUser4_లోపల_Use m_user4_లోపల_Use;

    // STATE
    VMemberMap m_memberMap;  // Member names cached for fast lookup
    AstClass* m_classp = శూన్యసూచిక;  // Current class
    Ast_కణుపు* m_procp = శూన్యసూచిక;  // NodeProcedure/CFunc/Begin we're under
    uint8_t m_underFork = F_NONE;  // F_NONE or flags of a fork we are under
    V3_లేఖాచిత్రం m_susp_లేఖాచిత్రం;  // Dependency graph where a node is a dependency of another if it being
                          // suspendable makes the other node suspendable
    V3_లేఖాచిత్రం m_proc_లేఖాచిత్రం;  // Dependency graph where a node is a dependency of another if it being
                          // suspendable makes the other node suspendable

    // METHODS
    // Get or create the dependency vertex for the given node
    DepVtx* getSuspendDepVtx(Ast_కణుపు* స్థిరరాశి nodep) {
        AstClass* classp = శూన్యసూచిక;
        ఒకవేళ (AstCFunc* funcp = VN_CAST(nodep, CFunc)) {
            ఒకవేళ (funcp->scopep() && funcp->scopep()->modp()) {
                classp = VN_CAST(funcp->scopep()->modp(), Class);
            }
        }
        ఒకవేళ (!nodep->user3p()) nodep->user3p(కొత్త SuspendDepVtx{&m_susp_లేఖాచిత్రం, nodep, classp});
        తిరిగి nodep->user3u().గమ్యం<SuspendDepVtx*>();
    }
    DepVtx* getNeedsProcDepVtx(Ast_కణుపు* స్థిరరాశి nodep) {
        AstClass* classp = శూన్యసూచిక;
        ఒకవేళ (AstCFunc* funcp = VN_CAST(nodep, CFunc)) {
            ఒకవేళ (funcp->scopep() && funcp->scopep()->modp()) {
                classp = VN_CAST(funcp->scopep()->modp(), Class);
            }
        }
        ఒకవేళ (!nodep->user4p()) nodep->user4p(కొత్త NeedsProcDepVtx{&m_proc_లేఖాచిత్రం, nodep, classp});
        తిరిగి nodep->user4u().గమ్యం<NeedsProcDepVtx*>();
    }
    // Pass timing flag between nodes
    తార్కిక passFlag(స్థిరరాశి Ast_కణుపు* from, Ast_కణుపు* గమ్యం, కణుపు_Flag flag) {
        ఒకవేళ ((from->user2() & flag) && !(గమ్యం->user2() & flag)) {
            కూడు_Flags(గమ్యం, flag);
            తిరిగి నిజం;
        }
        తిరిగి అబద్ధం;
    }
    // Propagate flag to all nodes that depend on the given one
    శూన్య propagateFlags(DepVtx* స్థిరరాశి vxp, కణుపు_Flag flag) {
        స్వయంచాలక* స్థిరరాశి parentp = vxp->nodep();
        కొరకు (V3_లేఖాచిత్రం_అంచు& అంచు : vxp->బహిర్గతం_అంచులు()) {
            స్వయంచాలక* స్థిరరాశి depVxp = static_cast<DepVtx*>(అంచు.శిఖరం());
            Ast_కణుపు* స్థిరరాశి depp = depVxp->nodep();
            ఒకవేళ (passFlag(parentp, depp, flag)) propagateFlags(depVxp, flag);
        }
    }
    మూస <రకం_పేరు T_Predicate>
    శూన్య propagateFlagsIf(DepVtx* స్థిరరాశి vxp, కణుపు_Flag flag, T_Predicate p) {
        స్వయంచాలక* స్థిరరాశి parentp = vxp->nodep();
        కొరకు (V3_లేఖాచిత్రం_అంచు& అంచు : vxp->బహిర్గతం_అంచులు()) {
            స్వయంచాలక* స్థిరరాశి depVxp = static_cast<DepVtx*>(అంచు.శిఖరం());
            Ast_కణుపు* స్థిరరాశి depp = depVxp->nodep();
            ఒకవేళ (p(&అంచు) && passFlag(parentp, depp, flag)) propagateFlagsIf(depVxp, flag, p);
        }
    }
    మూస <రకం_పేరు T_Predicate>
    శూన్య propagateFlagsReversedIf(DepVtx* స్థిరరాశి vxp, కణుపు_Flag flag, T_Predicate p) {
        స్వయంచాలక* స్థిరరాశి parentp = vxp->nodep();
        కొరకు (V3_లేఖాచిత్రం_అంచు& అంచు : vxp->లోపల_అంచులు()) {
            స్వయంచాలక* స్థిరరాశి depVxp = static_cast<DepVtx*>(అంచు.fromp());
            Ast_కణుపు* స్థిరరాశి depp = depVxp->nodep();
            ఒకవేళ (p(&అంచు) && passFlag(parentp, depp, flag))
                propagateFlagsReversedIf(depVxp, flag, p);
        }
    }

    // VISITORS
    శూన్య సందర్శించు(AstClass* nodep) override {
        UASSERT_OBJ(!m_classp, nodep, "Class under class");
        VL_RESTORER(m_classp);
        m_classp = nodep;
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(Ast_కణుపు_Procedure* nodep) override {
        VL_RESTORER(m_procp);
        m_procp = nodep;
        getNeedsProcDepVtx(nodep);
        కూడు_Flags(nodep, T_ALLOCS_PROC);
        ఒకవేళ (VN_IS(nodep, Always)) {
            UINFO(9, "Always does " << (nodep->needProcess() ? "" : "NOT ") << "need process");
        }
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(AstDisableFork* nodep) override {
        సందర్శించు(static_cast<Ast_కణుపు*>(nodep));
        కూడు_Flags(m_procp, T_FORCES_PROC | T_NEEDS_PROC);
    }
    శూన్య సందర్శించు(AstWaitFork* nodep) override {
        సందర్శించు(static_cast<Ast_కణుపు*>(nodep));
        కూడు_Flags(m_procp, T_FORCES_PROC | T_NEEDS_PROC);
    }
    శూన్య సందర్శించు(AstWait* nodep) override {
        UINFO(9, "suspendable-visit " << nodep);
        Ast_కణుపు_సమాసం* స్థిరరాశి condp = V3Const::constifyEdit(nodep->condp());
        ఒకవేళ (AstConst* స్థిరరాశి constp = VN_CAST(condp, Const)) {
            ఒకవేళ (!nodep->fileline()->warnIsOff(V3_Error_సంకేతం::WAITCONST)) {
                condp->v3warn(WAITCONST, "Wait statement condition is constant");
            }
            ఒకవేళ (!constp->isZero()) {
                // Remove AstWait before we track process as T_SUSPENDER
                ఒకవేళ (Ast_కణుపు* స్థిరరాశి stmtsp = nodep->stmtsp()) {
                    stmtsp->unlink_Fr_వెనుక_With_తదుపరి();
                    nodep->replaceWith(stmtsp);
                } లేకపోతే {
                    nodep->unlink_Fr_వెనుక();
                }
                VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
                తిరిగి;
            }
        }
        v3Global.సమితి_ఉపయోగిస్తుంది_Timing();
        ఒకవేళ (m_procp) కూడు_Flags(m_procp, T_SUSPENDEE | T_SUSPENDER | T_NEEDS_PROC);
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(AstCFunc* nodep) override {
        VL_RESTORER(m_procp);
        m_procp = nodep;
        iterateChildren(nodep);
        ఒకవేళ (nodep->needProcess()) కూడు_Flags(nodep, T_FORCES_PROC | T_NEEDS_PROC);
        DepVtx* స్థిరరాశి sVxp = getSuspendDepVtx(nodep);
        DepVtx* స్థిరరాశి pVxp = getNeedsProcDepVtx(nodep);
        ఒకవేళ (!m_classp) తిరిగి;

        // Go over overridden functions

        std::వరుస<AstClassExtends*> extends;
        ఒకవేళ (m_classp->extendsp()) extends.నెట్టు(m_classp->extendsp());

        అయితే (!extends.ఖాళీలు()) {
            AstClassExtends* ext_list = extends.front();
            extends.తీయి();

            కొరకు (AstClassExtends* cextp = ext_list; cextp;
                 cextp = VN_AS(cextp->nextp(), ClassExtends)) {
                // TODO: It is possible that a methods the same name in the base class is not
                // actually overridden by our method. If this causes a problem, traverse to
                // the root of the inheritance hierarchy and check if the original method is
                // virtual or not.
                ఒకవేళ (స్వయంచాలక* స్థిరరాశి overriddenp
                    = VN_CAST(m_memberMap.కనుగొను_Member(cextp->classp(), nodep->name()), CFunc)) {
                    // Suspendability and process affects typing, so they propagate both ways
                    DepVtx* స్థిరరాశి overriddenSVxp = getSuspendDepVtx(overriddenp);
                    DepVtx* స్థిరరాశి overriddenPVxp = getNeedsProcDepVtx(overriddenp);
                    కొత్త V3_లేఖాచిత్రం_అంచు{&m_susp_లేఖాచిత్రం, sVxp, overriddenSVxp, P_SIGNATURE};
                    కొత్త V3_లేఖాచిత్రం_అంచు{&m_susp_లేఖాచిత్రం, overriddenSVxp, sVxp, P_SIGNATURE};
                    కొత్త V3_లేఖాచిత్రం_అంచు{&m_proc_లేఖాచిత్రం, pVxp, overriddenPVxp, P_SIGNATURE};
                    కొత్త V3_లేఖాచిత్రం_అంచు{&m_proc_లేఖాచిత్రం, overriddenPVxp, pVxp, P_SIGNATURE};
                } లేకపోతే {
                    AstClassExtends* మరిన్ని_extends = cextp->classp()->extendsp();
                    ఒకవేళ (మరిన్ని_extends) extends.నెట్టు(మరిన్ని_extends);
                }
            }
        }
    }
    శూన్య సందర్శించు(Ast_కణుపు_CCall* nodep) override {
        AstCFunc* స్థిరరాశి funcp = nodep->funcp();
        UASSERT_OBJ(funcp, nodep, "AstNodeCCall must have non-null funcp post-link");
        UASSERT_OBJ(m_procp, nodep, "AstNodeCCall must be inside a procedure/CFunc/Begin");

        UINFO(9, "V3Timing: Processing CCall to " << funcp->name() << " in dependency graph\n");
        కొత్త V3_లేఖాచిత్రం_అంచు{&m_susp_లేఖాచిత్రం, getSuspendDepVtx(funcp), getSuspendDepVtx(m_procp), P_CALL};

        కొత్త V3_లేఖాచిత్రం_అంచు{&m_proc_లేఖాచిత్రం, getNeedsProcDepVtx(funcp), getNeedsProcDepVtx(m_procp),
                        P_CALL};

        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(AstBegin* nodep) override {
        VL_RESTORER(m_procp);
        VL_RESTORER(m_underFork);

        ఒకవేళ (!m_underFork || (m_underFork & F_MIGHT_SUSPEND))
            కొత్త V3_లేఖాచిత్రం_అంచు{&m_susp_లేఖాచిత్రం, getSuspendDepVtx(nodep), getSuspendDepVtx(m_procp),
                            m_underFork ? P_FORK : P_CALL};

        కొత్త V3_లేఖాచిత్రం_అంచు{&m_proc_లేఖాచిత్రం, getNeedsProcDepVtx(nodep), getNeedsProcDepVtx(m_procp),
                        P_CALL};

        ఒకవేళ (m_underFork) కూడు_Flags(nodep, T_NEEDS_PROC | T_ALLOCS_PROC);

        m_procp = nodep;
        m_underFork = 0;
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(AstFork* nodep) override {
        UINFO(9, "suspendable-visit " << nodep);
        VL_RESTORER(m_underFork);

        v3Global.సమితి_ఉపయోగిస్తుంది_Timing();  // Even if there are no event controls, we have to set this flag
                                   // so that transformForks() in V3SchedTiming gets called and
                                   // removes all forks and begins
        ఒకవేళ (nodep->isTimingControl() && m_procp) {
            కూడు_Flags(m_procp, T_SUSPENDEE | T_SUSPENDER);
            m_underFork |= F_MIGHT_SUSPEND;
        }
        m_underFork |= F_MIGHT_NEED_PROC;
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(Ast_కేటాయించు_Dly* nodep) override {
        ఒకవేళ (!VN_IS(m_procp, కణుపు_Procedure)) v3Global.సమితి_ఉపయోగిస్తుంది_Timing();
        సందర్శించు(static_cast<Ast_కణుపు*>(nodep));
    }
    శూన్య సందర్శించు(AstFireEvent* nodep) override {
        ఒకవేళ (nodep->isDelayed()
            && (!VN_IS(m_procp, కణుపు_Procedure) || !VN_IS(nodep->operandp(), VarRef))) {
            v3Global.సమితి_ఉపయోగిస్తుంది_Timing();
        }
        సందర్శించు(static_cast<Ast_కణుపు*>(nodep));
    }
    శూన్య సందర్శించు(Ast_కేటాయించు_W* nodep) override {
        ఒకవేళ (nodep->timingControlp()) v3Global.సమితి_ఉపయోగిస్తుంది_Timing();
        // Containing process will not suspend, don't mark it
    }
    శూన్య సందర్శించు(Ast_కణుపు* nodep) override {
        ఒకవేళ (nodep->isTimingControl()) {
            v3Global.సమితి_ఉపయోగిస్తుంది_Timing();
            ఒకవేళ (m_procp) కూడు_Flags(m_procp, T_SUSPENDEE | T_SUSPENDER | T_NEEDS_PROC);
        }
        iterateChildren(nodep);
    }

    //--------------------
    శూన్య సందర్శించు(AstVar*) override {}  // Accelerate

బహిరంగ:
    // CONSTRUCTORS
    స్పష్టమైన TimingSuspendableVisitor(AstNetlist* nodep) {
        iterate(nodep);
        m_susp_లేఖాచిత్రం.remove_సంక్రమణ_అంచులు();
        m_proc_లేఖాచిత్రం.remove_సంక్రమణ_అంచులు();
        // Propagate suspendability
        కొరకు (V3_లేఖాచిత్రం_శీర్షం& vtx : m_susp_లేఖాచిత్రం.vertices()) {
            DepVtx& depVtx = static_cast<DepVtx&>(vtx);
            ఒకవేళ (hasFlags(depVtx.nodep(), T_SUSPENDEE)) propagateFlags(&depVtx, T_SUSPENDEE);
        }
        ఒకవేళ (dump_లేఖాచిత్రం_Level() >= 6) m_susp_లేఖాచిత్రం.dumpDotFilePrefixed("timing_deps");

        // Propagate T_HAS_PROCESS
        కొరకు (V3_లేఖాచిత్రం_శీర్షం& vtx : m_proc_లేఖాచిత్రం.vertices()) {
            DepVtx& depVtx = static_cast<DepVtx&>(vtx);
            // Find processes that'll allocate VlProcess
            ఒకవేళ (hasFlags(depVtx.nodep(), T_FORCES_PROC)) {
                propagateFlagsIf(&depVtx, T_FORCES_PROC, [&](స్థిరరాశి V3_లేఖాచిత్రం_అంచు* e) -> తార్కిక {
                    తిరిగి !hasFlags(static_cast<DepVtx*>(e->fromp())->nodep(), T_ALLOCS_PROC);
                });
            }
            // Mark nodes on paths between processes and statements that use VlProcess
            ఒకవేళ (hasFlags(depVtx.nodep(), T_NEEDS_PROC)) {
                propagateFlagsIf(&depVtx, T_NEEDS_PROC, [&](స్థిరరాశి V3_లేఖాచిత్రం_అంచు* e) -> తార్కిక {
                    తిరిగి !hasFlags(static_cast<DepVtx*>(e->శిఖరం())->nodep(), T_ALLOCS_PROC);
                });
            }
        }
        కొరకు (V3_లేఖాచిత్రం_శీర్షం& vtx : m_proc_లేఖాచిత్రం.vertices()) {
            DepVtx& depVtx = static_cast<DepVtx&>(vtx);
            // Mark nodes that will be emitted with a VlProcess argument
            ఒకవేళ (hasFlags(depVtx.nodep(), T_ALLOCS_PROC | T_FORCES_PROC)) {
                కూడు_Flags(depVtx.nodep(), T_HAS_PROC);
                propagateFlagsReversedIf(&depVtx, T_HAS_PROC, [&](స్థిరరాశి V3_లేఖాచిత్రం_అంచు* e) -> తార్కిక {
                    తిరిగి hasFlags(static_cast<DepVtx*>(e->fromp())->nodep(), T_NEEDS_PROC);
                });
            }
        }
        ఒకవేళ (dump_లేఖాచిత్రం_Level() >= 6) m_proc_లేఖాచిత్రం.dumpDotFilePrefixed("proc_deps");
    }
    ~TimingSuspendableVisitor() override = అప్రమేయ;
};

// ######################################################################
//  Transform nodes affected by timing

తరగతి TimingControlVisitor final : బహిరంగ VNVisitor {
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
    AstNetlist* స్థిరరాశి m_netlistp;  // Root node
    Ast_శిఖరం_Scope* స్థిరరాశి m_శిఖరం_Scopep = m_netlistp->శిఖరం_Scopep();  // The AstTopScope
    AstScope* స్థిరరాశి m_scopeTopp = m_శిఖరం_Scopep->scopep();  // Scope at the top
    AstClass* m_classp = శూన్యసూచిక;  // Current class
    AstScope* m_scopep = శూన్యసూచిక;  // Current scope
    Ast_చురుకైన* m_activep = శూన్యసూచిక;  // Current active
    Ast_కణుపు* m_procp = శూన్యసూచిక;  // NodeProcedure/CFunc/Begin we're under
    తార్కిక m_hasProcess = అబద్ధం;  // True if current scope has a VlProcess handle available
    పూర్ణాంకం m_forkCnt = 0;  // Number of forks inside a module
    తార్కిక m_under_Jump_ఖండం = అబద్ధం;  // True if we are inside of a jump-block
    తార్కిక m_underProcedure = అబద్ధం;  // True if we are under an always or initial
    తార్కిక m_underIfaceCFunc = అబద్ధం;  // True if we are under a CFunc owned by an interface scope
    తార్కిక m_hasStaticZeroDelay = అబద్ధం;  // True if we have a static #0 delay
    std::సదిశ<File_పంక్తి*> m_unknownDelayFlps;  // Locations of AstDelay with non-constant value

    // Unique names
    V3_Unique_పేర్లు m_dlyfork_పేర్లు{"__Vdlyfork"};  // Names for temp AssignW vars
    V3_Unique_పేర్లు m_intra_విలువ_పేర్లు{"__Vintraval"};  // Intra assign delay value var names
    V3_Unique_పేర్లు m_intra_సూచీ_పేర్లు{"__Vintraidx"};  // Intra assign delay index var names
    V3_Unique_పేర్లు m_intra_Lsb_పేర్లు{"__Vintralsb"};  // Intra assign delay LSB var names
    V3_Unique_పేర్లు m_trig_Sched_పేర్లు{"__VtrigSched"};  // Trigger scheduler name generator
    V3_Unique_పేర్లు m_dyn_Trig_పేర్లు{"__VdynTrigger"};  // Dynamic trigger name generator
    // Module level temporary variables, shared by instances
    V3SharedTmps m_కేటాయించు_WTmps{"__VassignWtmp", VVarType::MODULETEMP};  // Delayed AssignW values
    V3SharedTmps m_కేటాయించు_WGens{"__VassignWgen", VVarType::MODULETEMP};  // AssignW generations

    // DTypes
    AstBasicDType* m_forkDtp = శూన్యసూచిక;  // Fork variable type
    AstBasicDType* m_trigSchedDtp = శూన్యసూచిక;  // Trigger scheduler type

    // Timing-related globals
    AstVarScope* m_delaySchedp = శూన్యసూచిక;  // Global delay scheduler
    AstVarScope* m_dynamicSchedp = శూన్యసూచిక;  // Global dynamic trigger scheduler
    Ast_Sen_వృక్షం* m_delaySensesp = శూన్యసూచిక;  // Domain to trigger if a delayed coroutine is resumed
    Ast_Sen_వృక్షం* m_dynamicSensesp = శూన్యసూచిక;  // Domain to trigger if a dynamic trigger is set

    // Other
    Sen_వృక్షం_Finder m_finder{m_netlistp};  // Sentree finder and uniquifier
    Sen_సమాసం_Builder* m_sen_సమాసం_Builderp = శూన్యసూచిక;  // Sens expression builder for current m_scope

    // Stats
    కొలత_t m_statZeroDelays = 0;  // Number of statically known #0 delays
    కొలత_t m_statConstDelays = 0;  // Number of statically known #const (non-zero) delays
    కొలత_t m_statVariableDelays = 0;  // Number of delays with value unknown at compile time

    // METHODS
    // Transform an assignment with an intra timing control into a timing control with the
    // assignment under it
    Ast_కణుపు* గుణకం_బహిర్గతం_Timing_Control(Ast_కణుపు_కేటాయించు* nodep) స్థిరరాశి {
        Ast_కణుపు* stmtp = nodep;
        Ast_కణుపు* స్థిరరాశి controlp = nodep->timingControlp();
        ఒకవేళ (AstDelay* స్థిరరాశి delayp = VN_CAST(controlp, Delay)) {
            stmtp->replaceWith(delayp->unlink_Fr_వెనుక());
            delayp->కూడు_Stmtsp(stmtp);
            stmtp = delayp;
        } లేకపోతే ఒకవేళ (Ast_Sen_వృక్షం* స్థిరరాశి sentreep = VN_CAST(controlp, Sen_వృక్షం)) {
            AstEventControl* స్థిరరాశి eventControlp
                = కొత్త AstEventControl{sentreep->fileline(), sentreep->unlink_Fr_వెనుక(), శూన్యసూచిక};
            stmtp->replaceWith(eventControlp);
            eventControlp->కూడు_Stmtsp(stmtp);
            stmtp = eventControlp;
        } లేకపోతే ఒకవేళ (AstBegin* స్థిరరాశి beginp = VN_CAST(controlp, Begin)) {
            // Begin from V3AssertPre
            stmtp->replaceWith(beginp->unlink_Fr_వెనుక());
            beginp->కూడు_Stmtsp(stmtp);
            stmtp = beginp;
        }
        తిరిగి stmtp == nodep ? శూన్యసూచిక : stmtp;
    }
    స్థిర తార్కిక staticallyNonZeroDelay(స్థిరరాశి Ast_కణుపు_సమాసం* valuep) {
        ఒకవేళ (స్థిరరాశి AstConst* స్థిరరాశి constp = VN_CAST(valuep, Const)) తిరిగి !constp->isZero();
        ఒకవేళ (స్థిరరాశి AstCond* స్థిరరాశి condp = VN_CAST(valuep, Cond)) {
            తిరిగి staticallyNonZeroDelay(condp->thenp())
                   && staticallyNonZeroDelay(condp->elsep());
        }
        ఒకవేళ (స్థిరరాశి AstMul* స్థిరరాశి mulp = VN_CAST(valuep, Mul)) {
            స్థిరరాశి AstConst* స్థిరరాశి lhsConstp = VN_CAST(mulp->lhsp(), Const);
            స్థిరరాశి AstConst* స్థిరరాశి rhsConstp = VN_CAST(mulp->rhsp(), Const);
            ఒకవేళ (lhsConstp && !lhsConstp->isZero()) తిరిగి staticallyNonZeroDelay(mulp->rhsp());
            ఒకవేళ (rhsConstp && !rhsConstp->isZero()) తిరిగి staticallyNonZeroDelay(mulp->lhsp());
        }
        తిరిగి అబద్ధం;
    }
    // Creates the global delay scheduler variable
    AstVarScope* getCreateDelayScheduler() {
        ఒకవేళ (m_delaySchedp) తిరిగి m_delaySchedp;
        స్వయంచాలక* స్థిరరాశి dlySchedDtp = కొత్త AstBasicDType{
            m_scopeTopp->fileline(), VBasicDTypeKwd::DELAY_SCHEDULER, VSigning::UNSIGNED};
        m_netlistp->typeTablep()->కూడు_Typesp(dlySchedDtp);
        m_delaySchedp = m_శిఖరం_Scopep->create_తాత్కాలిక("__VdlySched", dlySchedDtp);
        // Delay scheduler has to be accessible from top
        m_delaySchedp->varp()->sig_ప్రజా(నిజం);
        m_netlistp->delaySchedulerp(m_delaySchedp->varp());
        తిరిగి m_delaySchedp;
    }
    // Creates the delay sentree
    Ast_Sen_వృక్షం* get_Create_Delay_Sen_వృక్షం() {
        ఒకవేళ (m_delaySensesp) తిరిగి m_delaySensesp;
        File_పంక్తి* స్థిరరాశి flp = m_scopeTopp->fileline();
        స్వయంచాలక* స్థిరరాశి awaitingCurrentTimep
            = కొత్త AstCMethodHard{flp, కొత్త AstVarRef{flp, getCreateDelayScheduler(), VAccess::READ},
                                 VCMethod::SCHED_AWAITING_CURRENT_సమయం};
        awaitingCurrentTimep->dtype_సమితి_బిట్టు();
        m_delaySensesp
            = కొత్త Ast_Sen_వృక్షం{flp, కొత్త Ast_Sen_అంశం{flp, VEdgeType::ET_TRUE, awaitingCurrentTimep}};
        m_netlistp->శిఖరం_Scopep()->కూడు_Sen_Treesp(m_delaySensesp);
        తిరిగి m_delaySensesp;
    }
    // Creates the global dynamic trigger scheduler variable
    AstVarScope* getCreateDynamicTriggerScheduler() {
        ఒకవేళ (m_dynamicSchedp) తిరిగి m_dynamicSchedp;
        స్వయంచాలక* స్థిరరాశి dynSchedDtp
            = కొత్త AstBasicDType{m_scopeTopp->fileline(), VBasicDTypeKwd::DYNAMIC_TRIGGER_SCHEDULER,
                                VSigning::UNSIGNED};
        m_netlistp->typeTablep()->కూడు_Typesp(dynSchedDtp);
        m_dynamicSchedp = m_శిఖరం_Scopep->create_తాత్కాలిక("__VdynSched", dynSchedDtp);
        తిరిగి m_dynamicSchedp;
    }
    // Creates the dynamic trigger sentree
    Ast_Sen_వృక్షం* get_Create_Dynamic_Trigger_Sen_వృక్షం() {
        ఒకవేళ (m_dynamicSensesp) తిరిగి m_dynamicSensesp;
        File_పంక్తి* స్థిరరాశి flp = m_scopeTopp->fileline();
        స్వయంచాలక* స్థిరరాశి awaitingCurrentTimep = కొత్త AstCMethodHard{
            flp, కొత్త AstVarRef{flp, getCreateDynamicTriggerScheduler(), VAccess::READ},
            VCMethod::SCHED_EVALUATE};
        awaitingCurrentTimep->dtype_సమితి_బిట్టు();
        m_dynamicSensesp
            = కొత్త Ast_Sen_వృక్షం{flp, కొత్త Ast_Sen_అంశం{flp, VEdgeType::ET_TRUE, awaitingCurrentTimep}};
        m_netlistp->శిఖరం_Scopep()->కూడు_Sen_Treesp(m_dynamicSensesp);
        తిరిగి m_dynamicSensesp;
    }
    // Creates the event variable to trigger in NBA region
    AstEventControl* createNbaEventControl(File_పంక్తి* flp) {
        ఒకవేళ (!m_netlistp->nbaEventp()) {
            స్వయంచాలక* స్థిరరాశి nbaEventDtp = కొత్త AstBasicDType{m_scopeTopp->fileline(),
                                                        VBasicDTypeKwd::EVENT, VSigning::UNSIGNED};
            m_netlistp->typeTablep()->కూడు_Typesp(nbaEventDtp);
            m_netlistp->nbaEventp(m_శిఖరం_Scopep->create_తాత్కాలిక("__VnbaEvent", nbaEventDtp));
            v3Global.సమితి_Has_Events();
        }
        తిరిగి కొత్త AstEventControl{
            flp,
            కొత్త Ast_Sen_వృక్షం{
                flp, కొత్త Ast_Sen_అంశం{flp, VEdgeType::ET_EVENT,
                                    కొత్త AstVarRef{flp, m_netlistp->nbaEventp(), VAccess::READ}}},
            శూన్యసూచిక};
    }
    // Creates the variable that, if set, causes the NBA event to be triggered
    Ast_కేటాయించు* createNbaEventTriggerAssignment(File_పంక్తి* flp) {
        ఒకవేళ (!m_netlistp->nbaEventTriggerp()) {
            m_netlistp->nbaEventTriggerp(m_శిఖరం_Scopep->create_తాత్కాలిక("__VnbaEventTrigger", 1));
        }
        తిరిగి కొత్త Ast_కేటాయించు{flp,
                             కొత్త AstVarRef{flp, m_netlistp->nbaEventTriggerp(), VAccess::WRITE},
                             కొత్త AstConst{flp, AstConst::బిట్టు_True{}}};
    }
    // Returns true if we are under a class or interface function, or the tree references locals.
    // These are cases where static, globally-evaluated triggers are not suitable.
    తార్కిక needDynamicTrigger(Ast_కణుపు* స్థిరరాశి nodep) స్థిరరాశి {
        తిరిగి m_classp || m_underIfaceCFunc || nodep->exists([](Ast_కణుపు* స్థిరరాశి nodep) {
            ఒకవేళ (Ast_కణుపు_Var_Ref* varp = VN_CAST(nodep, కణుపు_Var_Ref)) {
                తిరిగి varp->varp()->isFuncLocal();
            }
            తిరిగి !nodep->is_శుద్ధ();
        });
    }
    // Creates a trigger scheduler variable
    AstVarScope* getCreateTriggerSchedulerp(Ast_Sen_వృక్షం* స్థిరరాశి sentreep) {
        ఒకవేళ (!sentreep->user1p()) {
            ఒకవేళ (!m_trigSchedDtp) {
                m_trigSchedDtp
                    = కొత్త AstBasicDType{m_scopeTopp->fileline(), VBasicDTypeKwd::TRIGGER_SCHEDULER,
                                        VSigning::UNSIGNED};
                m_netlistp->typeTablep()->కూడు_Typesp(m_trigSchedDtp);
            }
            AstVarScope* స్థిరరాశి trigSchedp
                = m_శిఖరం_Scopep->create_తాత్కాలిక(m_trig_Sched_పేర్లు.get(sentreep), m_trigSchedDtp);
            sentreep->user1p(trigSchedp);
        }
        తిరిగి VN_AS(sentreep->user1p(), VarScope);
    }
    // Creates a string describing the sentree
    AstCExpr* createEventDescription(Ast_Sen_వృక్షం* స్థిరరాశి sentreep) స్థిరరాశి {
        ఒకవేళ (!sentreep->user2p()) {
            std::stringstream ss;
            V3EmitV::verilog_For_వృక్షం(sentreep, ss);
            // possibly a multiline string
            // Names were hashed by V3Name, show the user the original text
            std::string comment = VName::dehash(ss.str());
            std::replace(comment.begin(), comment.చివర(), '\n', ' ');
            // Escape quotes, e.g. from string-indexed event array selects
            comment = '"' + V3_బహిర్గతం_Formatter::quoteNameControls(comment) + '"';
            AstCExpr* స్థిరరాశి commentp = కొత్త AstCExpr{sentreep->fileline(), comment};
            commentp->dtype_సమితి_String();
            sentreep->user2p(commentp);
            తిరిగి commentp;
        }
        తిరిగి VN_AS(sentreep->user2p(), CExpr)->clone_వృక్షం(అబద్ధం);
    }
    // Adds debug info to a hardcoded method call
    శూన్య కూడు_Debug_Info(AstCMethodHard* స్థిరరాశి methodp) స్థిరరాశి {
        File_పంక్తి* స్థిరరాశి flp = methodp->fileline();
        స్థిరరాశి తార్కిక protectIds = v3Global.opt.protectIds();
        AstCExpr* స్థిరరాశి ap
            = కొత్త AstCExpr{flp, protectIds ? "VL_UNKNOWN" : '"' + flp->filenameEsc() + '"'};
        ap->dtype_సమితి_String();
        methodp->కూడు_Pinsp(ap);
        AstCExpr* స్థిరరాశి bp = కొత్త AstCExpr{flp, protectIds ? "0" : cvt_గమ్యం_Str(flp->lineno())};
        bp->dtype_సమితి_String();
        methodp->కూడు_Pinsp(bp);
    }
    // Adds debug info to a trigSched.trigger() call
    శూన్య కూడు_Event_Debug_Info(AstCMethodHard* స్థిరరాశి methodp, Ast_Sen_వృక్షం* స్థిరరాశి sentreep) స్థిరరాశి {
        ఒకవేళ (v3Global.opt.protectIds()) {
            File_పంక్తి* స్థిరరాశి flp = sentreep->fileline();
            AstCExpr* స్థిరరాశి descp = కొత్త AstCExpr{flp, "VL_UNKNOWN"};
            descp->dtype_సమితి_String();
            methodp->కూడు_Pinsp(descp);
            కూడు_Debug_Info(methodp);
            తిరిగి;
        }
        methodp->కూడు_Pinsp(createEventDescription(sentreep));
        కూడు_Debug_Info(methodp);
    }
    // Adds process pointer to a hardcoded method call
    శూన్య కూడు_Process_Info(AstCMethodHard* స్థిరరాశి methodp) స్థిరరాశి {
        File_పంక్తి* స్థిరరాశి flp = methodp->fileline();
        AstCExpr* స్థిరరాశి ap
            = కొత్త AstCExpr{flp, (m_procp && m_hasProcess) ? "vlProcess" : "nullptr"};
        methodp->కూడు_Pinsp(ap);
    }
    // Creates the fork handle type and returns it
    AstBasicDType* getCreateForkSyncDTypep() {
        ఒకవేళ (m_forkDtp) తిరిగి m_forkDtp;
        m_forkDtp = కొత్త AstBasicDType{m_scopeTopp->fileline(), VBasicDTypeKwd::FORK_SYNC,
                                      VSigning::UNSIGNED};
        m_netlistp->typeTablep()->కూడు_Typesp(m_forkDtp);
        తిరిగి m_forkDtp;
    }
    // Move `nodep` into `AstCLocalScope`.
    AstCLocalScope* కూడు_CLocal_Scope(File_పంక్తి* స్థిరరాశి flp, Ast_కణుపు* స్థిరరాశి nodep) స్థిరరాశి {
        ఒకవేళ (!nodep) తిరిగి శూన్యసూచిక;
        VNRelinker handle;
        nodep->unlink_Fr_వెనుక(&handle);
        AstCLocalScope* స్థిరరాశి cscopep = కొత్త AstCLocalScope{flp, nodep};
        handle.relink(cscopep);
        తిరిగి cscopep;
    }
    // Create a temp variable and optionally put it before the specified node (mark local if so)
    AstVarScope* create_తాత్కాలిక(File_పంక్తి* స్థిరరాశి flp, స్థిరరాశి std::string& name,
                            Ast_కణుపు_DType* స్థిరరాశి dtypep, Ast_కణుపు* స్థిరరాశి చొప్పించు_Beforep = శూన్యసూచిక) {
        AstVar* varp;
        ఒకవేళ (చొప్పించు_Beforep) {
            varp = కొత్త AstVar{flp, VVarType::BLOCKTEMP, name, dtypep};
            varp->funcLocal(నిజం);
            చొప్పించు_Beforep->కూడు_ఇక్కడ_This_As_తదుపరి(varp);
        } లేకపోతే {
            varp = కొత్త AstVar{flp, VVarType::MODULETEMP, name, dtypep};
            m_scopep->modp()->కూడు_Stmtsp(varp);
        }
        AstVarScope* స్థిరరాశి vscp = కొత్త AstVarScope{flp, m_scopep, varp};
        m_scopep->కూడు_Varsp(vscp);
        తిరిగి vscp;
    }
    // Add a done() call on the fork sync
    శూన్య కూడు_Fork_పూర్తి(AstBegin* స్థిరరాశి beginp, AstVarScope* స్థిరరాశి forkVscp) స్థిరరాశి {
        File_పంక్తి* స్థిరరాశి flp = beginp->fileline();
        స్వయంచాలక* స్థిరరాశి donep = కొత్త AstCMethodHard{
            beginp->fileline(), కొత్త AstVarRef{flp, forkVscp, VAccess::WRITE}, VCMethod::FORK_పూర్తి};
        donep->dtype_సమితి_Void();
        కూడు_Debug_Info(donep);
        beginp->కూడు_Stmtsp(donep->makeStmt());
    }
    స్థిర తార్కిక has_Disable_వరుస_నెట్టు_Self_ఉపసర్గ(AstBegin* స్థిరరాశి beginp) {
        // LinkJump prepends disable-by-name registration as:
        //   __VprocessQueue_*.push_back(std::process::self())
        // V3LiftExpr lifts std::process::self() into an assignment to a temporary, then V3Task
        // lowers that assignment's function call into a leading comment and AstStmtExpr. Thus the
        // push_back is no longer necessarily the first statement. Scan across this generated
        // prefix for it, stopping at the fork-start sentinel or the branch body. Registrations for
        // nested forks live in sub-blocks and so are not in this statement list.
        కొరకు (Ast_కణుపు* stmtp = beginp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ఒకవేళ (VN_IS(stmtp, Comment)) కొనసాగు;
            Ast_Stmt_సమాసం* స్థిరరాశి stmtExprp = VN_CAST(stmtp, Stmt_సమాసం);
            ఒకవేళ (!stmtExprp) ఆపు;
            ఒకవేళ (stmtExprp->is_Disable_వరుస_నెట్టు_Self_Stmt()) తిరిగి నిజం;
        }
        తిరిగి అబద్ధం;
    }
    // Register a callback so killing a process-backed fork branch decrements the join counter
    శూన్య కూడు_Fork_On_Kill(AstBegin* స్థిరరాశి beginp, AstVarScope* స్థిరరాశి forkVscp) స్థిరరాశి {
        ఒకవేళ (!beginp->needProcess() && !has_Disable_వరుస_నెట్టు_Self_ఉపసర్గ(beginp)) తిరిగి;
        File_పంక్తి* స్థిరరాశి flp = beginp->fileline();
        AstCMethodHard* స్థిరరాశి onKillp = కొత్త AstCMethodHard{
            flp, కొత్త AstVarRef{flp, forkVscp, VAccess::WRITE}, VCMethod::FORK_ON_KILL};
        onKillp->dtype_సమితి_Void();
        AstCExpr* స్థిరరాశి processp = కొత్త AstCExpr{flp, "vlProcess"};
        processp
            ->dtype_సమితి_Void();  // Opaque process reference; type is irrelevant for hardcoded emit
        onKillp->కూడు_Pinsp(processp);
        Ast_కణుపు_Stmt* స్థిరరాశి stmtp = onKillp->makeStmt();
        beginp->prependStmtsp(stmtp);
    }
    // Handle the 'join' part of a fork..join
    శూన్య makeForkJoin(AstFork* స్థిరరాశి forkp) {
        // Create a fork sync var
        File_పంక్తి* స్థిరరాశి flp = forkp->fileline();
        // Insert the sync var directly before the fork
        Ast_కణుపు* స్థిరరాశి చొప్పించు_Beforep = forkp;
        // Make sure all references to the fork var are contained within a single scope to prevent
        // moving statements accessing it in case of large functions splitting.
        కూడు_CLocal_Scope(flp, చొప్పించు_Beforep);
        AstVarScope* forkVscp
            = create_తాత్కాలిక(flp, forkp->name() + "__sync", getCreateForkSyncDTypep(), చొప్పించు_Beforep);
        గుర్తులేని join_లెక్క = 0;  // Needed for join counter
        // Add a <fork sync>.done() to each begin
        కొరకు (Ast_కణుపు* beginp = forkp->forksp(); beginp; beginp = beginp->nextp()) {
            కూడు_Fork_On_Kill(VN_AS(beginp, Begin), forkVscp);
            కూడు_Fork_పూర్తి(VN_AS(beginp, Begin), forkVscp);
            join_లెక్క++;
        }
        ఒకవేళ (forkp->joinType().joinAny()) join_లెక్క = 1;
        // Set the join counter
        స్వయంచాలక* స్థిరరాశి initp = కొత్త AstCMethodHard{flp, కొత్త AstVarRef{flp, forkVscp, VAccess::WRITE},
                                               VCMethod::FORK_INIT, కొత్త AstConst{flp, join_లెక్క}};
        initp->dtype_సమితి_Void();
        కూడు_Process_Info(initp);
        forkp->కూడు_ఇక్కడ_This_As_తదుపరి(initp->makeStmt());
        // Await the join at the end
        స్వయంచాలక* స్థిరరాశి joinp = కొత్త AstCMethodHard{flp, కొత్త AstVarRef{flp, forkVscp, VAccess::WRITE},
                                               VCMethod::FORK_JOIN};
        joinp->dtype_సమితి_Void();
        కూడు_Process_Info(joinp);
        కూడు_Debug_Info(joinp);
        forkp->కూడు_తదుపరి_ఇక్కడ(కొత్త AstCAwait{flp, joinp});
    }

    // `procp` shall be a NodeProcedure/CFunc/Begin and within it vars from `varsp` will be placed.
    // `varsp` vector of vars which shall be localized.
    స్థిర శూన్య localizeVars(Ast_కణుపు* స్థిరరాశి procp, స్థిరరాశి std::సదిశ<AstVar*>& varsp) {
        UASSERT(procp, "procp is nullptr");
        AstVar* newVarsp = శూన్యసూచిక;
        కొరకు (AstVar* స్థిరరాశి varp : varsp) {
            varp->funcLocal(నిజం);
            newVarsp = Ast_కణుపు::కూడు_తదుపరి_Null(newVarsp, varp->unlink_Fr_వెనుక());
        }
        ఒకవేళ (Ast_కణుపు_Procedure* స్థిరరాశి కణుపు_Procp = VN_CAST(procp, కణుపు_Procedure)) {
            కణుపు_Procp->prependStmtsp(newVarsp);
        } లేకపోతే ఒకవేళ (AstCFunc* స్థిరరాశి cfuncp = VN_CAST(procp, CFunc)) {
            cfuncp->prependVarsp(newVarsp);
        } లేకపోతే ఒకవేళ (AstBegin* స్థిరరాశి beginp = VN_CAST(procp, Begin)) {
            beginp->prependStmtsp(newVarsp);
        } లేకపోతే {
            procp->v3fatalSrc(
                procp->prettyNameQ()
                << " is not of an expected type NodeProcedure/CFunc/Begin instead it is: "
                << procp->prettyTypeName());
        }
    }

    Ast_కణుపు_సమాసం* delay_గమ్యం_Integral(AstDelay* nodep, Ast_కణుపు_సమాసం* valuep) {
        స్థిరరాశి AstConst* స్థిరరాశి constp = VN_CAST(valuep, Const);
        // The fork start sentinel is already in ticks
        ఒకవేళ (constp && constp->గమ్యం_UQuad() == std::numeric_limits<uint64_t>::max()) తిరిగి valuep;
        తిరిగి V3Timing::delay_గమ్యం_Ticks(valuep, nodep->timeunit());
    }

    // VISITORS
    శూన్య సందర్శించు(Ast_కణుపు_Module* nodep) override {
        UASSERT_OBJ(!m_classp, nodep, "Module or class under class");
        VL_RESTORER(m_classp);
        m_classp = VN_CAST(nodep, Class);
        VL_RESTORER(m_forkCnt);
        m_forkCnt = 0;
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(AstScope* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        Sen_సమాసం_Builder sen_సమాసం_Builder{m_scopep};
        {  // Restore m_senExprBuilderp before destroying senExprBuilder
            VL_RESTORER(m_sen_సమాసం_Builderp);
            m_sen_సమాసం_Builderp = &sen_సమాసం_Builder;
            iterateChildren(nodep);
        }
    }
    శూన్య సందర్శించు(Ast_చురుకైన* nodep) override {
        VL_RESTORER(m_activep);
        m_activep = nodep;
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(Ast_కణుపు_Procedure* nodep) override {
        VL_RESTORER(m_procp);
        VL_RESTORER(m_hasProcess);
        m_procp = nodep;
        m_hasProcess = hasFlags(nodep, T_HAS_PROC);
        VL_RESTORER(m_underProcedure);
        m_underProcedure = నిజం;
        iterateChildren(nodep);
        ఒకవేళ (hasFlags(nodep, T_SUSPENDEE)) nodep->సమితి_Suspendable();
        ఒకవేళ (hasFlags(nodep, T_HAS_PROC)) nodep->సమితి_Need_Process();
    }
    శూన్య సందర్శించు(AstInitial* nodep) override {
        సందర్శించు(static_cast<Ast_కణుపు_Procedure*>(nodep));
        ఒకవేళ (nodep->needProcess() && !nodep->user1_సమితి_Once()) {
            nodep->కూడు_Stmtsp(
                కొత్త AstCStmt{nodep->fileline(), "vlProcess->state(VlProcess::FINISHED);"});
        }
    }
    శూన్య సందర్శించు(Ast_Jump_ఖండం* nodep) override {
        VL_RESTORER(m_under_Jump_ఖండం);
        m_under_Jump_ఖండం = నిజం;
        సందర్శించు(static_cast<Ast_కణుపు_Stmt*>(nodep));
    }
    శూన్య సందర్శించు(AstAlways* nodep) override {
        ఒకవేళ (nodep->user1_సమితి_Once()) తిరిగి;
        VL_RESTORER(m_procp);
        VL_RESTORER(m_hasProcess);
        m_procp = nodep;
        m_hasProcess = hasFlags(nodep, T_HAS_PROC);
        VL_RESTORER(m_underProcedure);
        m_underProcedure = నిజం;
        // Workaround for killing `always` processes (doing that is pretty much UB)
        // TODO: Disallow killing `always` at runtime (throw an error)
        // Skip for combinational blocks; if the body has timing controls
        // iterateChildren will add T_SUSPENDEE, otherwise it would spin.
        ఒకవేళ (hasFlags(nodep, T_HAS_PROC)
            && !(m_activep && m_activep->sentreep() && m_activep->sentreep()->hasCombo()))
            కూడు_Flags(nodep, T_SUSPENDEE);

        iterateChildren(nodep);
        ఒకవేళ (hasFlags(nodep, T_HAS_PROC)) nodep->సమితి_Need_Process();
        ఒకవేళ (!hasFlags(nodep, T_SUSPENDEE)) తిరిగి;
        nodep->సమితి_Suspendable();
        File_పంక్తి* స్థిరరాశి flp = nodep->fileline();
        Ast_Sen_వృక్షం* స్థిరరాశి sentreep = m_activep->sentreep();
        ఒకవేళ (sentreep->hasClocked()) {
            Ast_కణుపు* స్థిరరాశి bodysp = nodep->stmtsp()->unlink_Fr_వెనుక_With_తదుపరి();
            స్వయంచాలక* స్థిరరాశి controlp = కొత్త AstEventControl{flp, sentreep->clone_వృక్షం(అబద్ధం), bodysp};
            nodep->కూడు_Stmtsp(controlp);
            iterate(controlp);
        }
        // Note: The 'while (true)' outer loop will be added in V3Sched
        స్వయంచాలక* స్థిరరాశి activep = కొత్త Ast_చురుకైన{
            flp, "", కొత్త Ast_Sen_వృక్షం{flp, కొత్త Ast_Sen_అంశం{flp, Ast_Sen_అంశం::Initial{}}}};
        activep->sen_వృక్షం_Storep(activep->sentreep());
        activep->కూడు_Stmtsp(nodep->unlink_Fr_వెనుక());
        m_activep->కూడు_తదుపరి_ఇక్కడ(activep);
    }
    శూన్య సందర్శించు(AstCFunc* nodep) override {
        VL_RESTORER(m_procp);
        VL_RESTORER(m_hasProcess);
        VL_RESTORER(m_underIfaceCFunc);
        m_procp = nodep;
        m_hasProcess = hasFlags(nodep, T_HAS_PROC);
        m_underIfaceCFunc = VN_IS(m_scopep->modp(), Iface);
        iterateChildren(nodep);
        ఒకవేళ (hasFlags(nodep, T_HAS_PROC)) nodep->సమితి_Need_Process();
        ఒకవేళ (!(hasFlags(nodep, T_SUSPENDEE))) తిరిగి;

        nodep->rtnType("VlCoroutine");
        // If in a class, create a shared pointer to 'this'
        ఒకవేళ (m_classp) {
            AstCStmt* స్థిరరాశి cstmtp = కొత్త AstCStmt{nodep->fileline(), "VL_KEEP_THIS;"};
            nodep->prependStmtsp(cstmtp);
        }
        ఒకవేళ (nodep->dpiExportImpl()) {
            nodep->exists([](AstCAwait* స్థిరరాశి awaitp) -> తార్కిక {
                // A DPI-exported coroutine won't be able to block the calling code
                // Error on the await node; fall back to the function node
                awaitp->v3warn(E_UNSUPPORTED,
                               "Unsupported: Timing controls inside DPI-exported tasks");
                తిరిగి నిజం;
            });
        }
    }
    శూన్య సందర్శించు(Ast_కణుపు_CCall* nodep) override {
        AstCFunc* స్థిరరాశి funcp = nodep->funcp();

        // Skip automatic covergroup sampling calls
        ఒకవేళ (funcp->isCovergroupSample()) {
            iterateChildren(nodep);
            తిరిగి;
        }

        ఒకవేళ (funcp->needProcess()) m_hasProcess = నిజం;
        ఒకవేళ (hasFlags(funcp, T_SUSPENDEE) && !nodep->user1_సమితి_Once()) {  // If suspendable
            // Calls to suspendables are always void return type, hence parent must be StmtExpr
            Ast_Stmt_సమాసం* స్థిరరాశి stmtp = VN_AS(nodep->backp(), Stmt_సమాసం);
            stmtp->replaceWith(కొత్త AstCAwait{nodep->fileline(), nodep->unlink_Fr_వెనుక()});
            VL_DO_DANGLING(నెట్టు_Deletep(stmtp), stmtp);
        }
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(AstDelay* nodep) override {
        UASSERT_OBJ(!nodep->is_చక్రం_Delay(), nodep,
                    "Cycle delays should have been handled in V3AssertPre");
        File_పంక్తి* స్థిరరాశి flp = nodep->fileline();

        Ast_కణుపు_సమాసం* valuep = delay_గమ్యం_Integral(nodep, nodep->lhsp()->unlink_Fr_వెనుక());

        // Statistics
        ఒకవేళ (valuep->isZero()) {
            ++m_statZeroDelays;
        } లేకపోతే ఒకవేళ (VN_IS(valuep, Const)) {
            ++m_statConstDelays;
        } లేకపోతే {
            ++m_statVariableDelays;
        }

        // Decide scheduling support for #0
        ఒకవేళ (v3Global.opt.schedZeroDelay().is_సమితి_True()) {
            // User said to schedule for #0 support, nothing else to do
            v3Global.సమితి_ఉపయోగిస్తుంది_Zero_Delay();
        } లేకపోతే ఒకవేళ (v3Global.opt.schedZeroDelay().is_సమితి_False()) {
            // User said to schedule without #0 support. Still warn if a static #0 delay exists
            ఒకవేళ (valuep->isZero()) {
                nodep->v3warn(
                    ZERODLY,
                    "Static #0 delay exists, but '--no-sched-zero-delay' was given.\n"
                        << nodep->warn_మరిన్ని()  //
                        << "... Can proceed, but this will fail at runtime if executed.");
            }
        } లేకపోతే {
            // User did not express preference, decide based on presence of delays
            ఒకవేళ (valuep->isZero()) {
                // Statically known #0 delay exists, schedule for #0 support
                v3Global.సమితి_ఉపయోగిస్తుంది_Zero_Delay();
                m_hasStaticZeroDelay = నిజం;
                // Don't warn on variable delays, as no point
                m_unknownDelayFlps.clear();
            } లేకపోతే ఒకవేళ (staticallyNonZeroDelay(valuep)) {
                // Delay is dynamic, but every statically known outcome is non-zero.
                // So we don't need #0 delay support and there should be no warning.
            } లేకపోతే ఒకవేళ (!VN_IS(valuep, Const)) {
                // Delay is not known at compiile time. Conservatively schedule for #0 support,
                // but warn if no static #0 delays used as performance might be improved
                v3Global.సమితి_ఉపయోగిస్తుంది_Zero_Delay();
                ఒకవేళ (!m_hasStaticZeroDelay) m_unknownDelayFlps.నెట్టు_వెనుక(nodep->fileline());
            }
        }

        // Replace self with a 'co_await dlySched.delay(<valuep>)'
        AstCMethodHard* స్థిరరాశి delayMethodp = కొత్త AstCMethodHard{
            flp, కొత్త AstVarRef{flp, getCreateDelayScheduler(), VAccess::WRITE},
            VCMethod::SCHED_DELAY, valuep};
        delayMethodp->dtype_సమితి_Void();
        కూడు_Process_Info(delayMethodp);
        కూడు_Debug_Info(delayMethodp);
        // Create the co_await
        Ast_కణుపు* స్థిరరాశి awaitp = కొత్త AstCAwait{flp, delayMethodp, get_Create_Delay_Sen_వృక్షం()};
        // Relink child statements after the co_await
        ఒకవేళ (Ast_కణుపు* స్థిరరాశి stmtsp = nodep->stmtsp()) {
            awaitp->కూడు_తదుపరి(stmtsp->unlink_Fr_వెనుక_With_తదుపరి());
        }
        nodep->replaceWith(awaitp);
        VL_DO_DANGLING(nodep->delete_వృక్షం(), nodep);
    }
    శూన్య సందర్శించు(AstEventControl* nodep) override {
        // Do not allow waiting on local named events, as they get enqueued for clearing, but can
        // go out of scope before that happens
        ఒకవేళ (!nodep->sentreep()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Event control with implicit sensitivity (@*)"
                          " in this context; use an explicit sensitivity list instead");
            VL_DO_DANGLING(నెట్టు_Deletep(nodep->unlink_Fr_వెనుక()), nodep);
            తిరిగి;
        }
        File_పంక్తి* స్థిరరాశి flp = nodep->fileline();
        // Relink child statements after the event control
        ఒకవేళ (nodep->stmtsp()) nodep->కూడు_తదుపరి_ఇక్కడ(nodep->stmtsp()->unlink_Fr_వెనుక_With_తదుపరి());
        ఒకవేళ (needDynamicTrigger(nodep->sentreep())) {
            // Create the trigger variable and init it with 0
            AstVarScope* స్థిరరాశి trigvscp
                = create_తాత్కాలిక(flp, m_dyn_Trig_పేర్లు.get(nodep), nodep->కనుగొను_బిట్టు_DType(), nodep);
            స్వయంచాలక* స్థిరరాశి initp = కొత్త Ast_కేటాయించు{flp, కొత్త AstVarRef{flp, trigvscp, VAccess::WRITE},
                                              కొత్త AstConst{flp, AstConst::బిట్టు_False{}}};
            // Await the eval step with the dynamic trigger scheduler. First, create the method
            // call
            స్వయంచాలక* స్థిరరాశి evalMethodp = కొత్త AstCMethodHard{
                flp, కొత్త AstVarRef{flp, getCreateDynamicTriggerScheduler(), VAccess::WRITE},
                VCMethod::SCHED_EVALUATION};
            evalMethodp->dtype_సమితి_Void();
            కూడు_Process_Info(evalMethodp);
            స్వయంచాలక* స్థిరరాశి sentreep = nodep->sentreep();
            కూడు_Event_Debug_Info(evalMethodp, sentreep);
            // Create the co_await
            AstCAwait* స్థిరరాశి awaitEvalp
                = కొత్త AstCAwait{flp, evalMethodp, get_Create_Dynamic_Trigger_Sen_వృక్షం()};
            // Construct the sen expression for this sentree
            UASSERT_OBJ(m_sen_సమాసం_Builderp, nodep, "No SenExprBuilder for this scope");
            స్వయంచాలక* స్థిరరాశి assignp = కొత్త Ast_కేటాయించు{flp, కొత్త AstVarRef{flp, trigvscp, VAccess::WRITE},
                                                m_sen_సమాసం_Builderp->build(sentreep).మొదటి};
            // Get the SenExprBuilder results
            స్థిరరాశి Sen_సమాసం_Builder::Results senResults = m_sen_సమాసం_Builderp->getAndClearResults();
            localizeVars(m_procp, senResults.m_vars);
            // If post updates are destructive (e.g. clearFired on events), perform a
            // conservative pre-clear once before entering the wait loop so stale state from a
            // previous wait does not cause an immediate false-positive trigger.
            స్థిరరాశి తార్కిక hasDestructivePostUpdates = !senResults.m_destructivePostUpdates.ఖాళీలు();
            // Create the trigger eval loop, which will await the evaluation step and check the
            // trigger
            Ast_కణుపు_సమాసం* స్థిరరాశి condp
                = కొత్త AstLogNot{flp, కొత్త AstVarRef{flp, trigvscp, VAccess::READ}};
            AstLoop* స్థిరరాశి loopp = కొత్త AstLoop{flp};
            loopp->కూడు_Stmtsp(కొత్త AstLoopTest{flp, loopp, condp});
            loopp->కూడు_Stmtsp(awaitEvalp);
            // Put pre updates before the trigger check and assignment
            కొరకు (Ast_కణుపు_Stmt* స్థిరరాశి stmtp : senResults.m_preUpdates) loopp->కూడు_Stmtsp(stmtp);
            // Then the trigger check and assignment
            loopp->కూడు_Stmtsp(assignp);
            // Let the dynamic trigger scheduler know if this trigger was set
            // If it was, a call to the scheduler's evaluate() will return true
            AstCMethodHard* స్థిరరాశి anyTriggeredMethodp = కొత్త AstCMethodHard{
                flp, కొత్త AstVarRef{flp, getCreateDynamicTriggerScheduler(), VAccess::WRITE},
                VCMethod::SCHED_ANY_TRIGGERED, కొత్త AstVarRef{flp, trigvscp, VAccess::READ}};
            anyTriggeredMethodp->dtype_సమితి_Void();
            loopp->కూడు_Stmtsp(anyTriggeredMethodp->makeStmt());
            // If the post update is destructive (e.g. event vars are cleared), create an await for
            // the post update step
            ఒకవేళ (hasDestructivePostUpdates) {
                AstCAwait* స్థిరరాశి awaitPostUpdatep = awaitEvalp->clone_వృక్షం(అబద్ధం);
                VN_AS(awaitPostUpdatep->exprp(), CMethodHard)->పద్ధతి(VCMethod::SCHED_POST_UPDATE);
                loopp->కూడు_Stmtsp(awaitPostUpdatep);
            }
            // Put the post updates at the end of the loop
            కొరకు (Ast_కణుపు_Stmt* స్థిరరాశి stmtp : senResults.m_postUpdates) loopp->కూడు_Stmtsp(stmtp);
            // Finally, await the resumption step in 'act'
            AstCAwait* స్థిరరాశి awaitResumep = awaitEvalp->clone_వృక్షం(అబద్ధం);
            VN_AS(awaitResumep->exprp(), CMethodHard)->పద్ధతి(VCMethod::SCHED_RESUMPTION);
            Ast_కణుపు::కూడు_తదుపరి<Ast_కణుపు_Stmt, Ast_కణుపు_Stmt>(loopp, awaitResumep);
            // Replace the event control with one explicit stmt chain:
            //   init -> [inits] -> [optional destructive pre-clears] -> loop -> awaitResumption
            Ast_కణుపు_Stmt* chainp = శూన్యసూచిక;
            chainp = Ast_కణుపు::కూడు_తదుపరి_Null(chainp, initp);
            కొరకు (Ast_కణుపు_Stmt* స్థిరరాశి stmtp : senResults.m_inits) {
                chainp = Ast_కణుపు::కూడు_తదుపరి_Null(chainp, stmtp);
            }
            ఒకవేళ (hasDestructivePostUpdates) {
                కొరకు (Ast_కణుపు_Stmt* స్థిరరాశి stmtp : senResults.m_destructivePostUpdates) {
                    chainp = Ast_కణుపు::కూడు_తదుపరి_Null(chainp, stmtp->clone_వృక్షం(అబద్ధం));
                }
            }
            chainp = Ast_కణుపు::కూడు_తదుపరి_Null(chainp, loopp);
            nodep->replaceWith(chainp);
        } లేకపోతే {
            స్వయంచాలక* స్థిరరాశి sentreep = m_finder.get_Sen_వృక్షం(nodep->sentreep());
            nodep->sentreep()->unlink_Fr_వెనుక()->delete_వృక్షం();
            // Get this sentree's trigger scheduler
            // Replace self with a 'co_await trigSched.trigger()'
            స్వయంచాలక* స్థిరరాశి triggerMethodp = కొత్త AstCMethodHard{
                flp, కొత్త AstVarRef{flp, getCreateTriggerSchedulerp(sentreep), VAccess::WRITE},
                VCMethod::SCHED_TRIGGER};
            triggerMethodp->dtype_సమితి_Void();
            // If it should be committed immediately, pass true, otherwise false
            triggerMethodp->కూడు_Pinsp(nodep->user2() ? కొత్త AstConst{flp, AstConst::బిట్టు_True{}}
                                                    : కొత్త AstConst{flp, AstConst::బిట్టు_False{}});
            కూడు_Process_Info(triggerMethodp);
            కూడు_Event_Debug_Info(triggerMethodp, sentreep);
            // Create the co_await
            AstCAwait* స్థిరరాశి awaitp = కొత్త AstCAwait{flp, triggerMethodp, sentreep};
            nodep->replaceWith(awaitp);
        }
        VL_DO_DANGLING(nodep->delete_వృక్షం(), nodep);
    }
    శూన్య సందర్శించు(Ast_కణుపు_కేటాయించు* nodep) override {
        // Only process once to avoid infinite loops (due to the net delay)
        ఒకవేళ (nodep->user1_సమితి_Once()) తిరిగి;
        UINFO(9, "control-visit " << nodep);
        File_పంక్తి* స్థిరరాశి flp = nodep->fileline();
        Ast_కణుపు* controlp = గుణకం_బహిర్గతం_Timing_Control(nodep);
        స్థిరరాశి తార్కిక లోపల_కేటాయించు_Dly = VN_IS(nodep, కేటాయించు_Dly);
        // Handle the intra assignment timing control
        // Transform if:
        // * there's a timing control in the assignment
        // * the assignment is an AssignDly and it's in a non-inlined function
        ఒకవేళ (!controlp && (!లోపల_కేటాయించు_Dly || m_underProcedure)) {
            iterateChildren(nodep);
            తిరిగి;
        }
        // The timing control, if any, which is put in a fork for an NBA
        Ast_కణుపు* స్థిరరాశి timingp = controlp;
        // Insert new vars before the timing control if we're in a function; in a process we can't
        // do that, as after V3Begin only modules and functions declare variables. These
        // intra-assignment vars will later be passed to forked processes by value.
        Ast_కణుపు* చొప్పించు_Beforep = m_underProcedure ? శూన్యసూచిక : controlp;
        // Special case for NBA
        ఒకవేళ (లోపల_కేటాయించు_Dly) {
            // Put it in a fork so it doesn't block
            // Could already be the only thing directly under a fork, reuse that if possible
            AstFork* forkp = !nodep->nextp() ? VN_CAST(nodep->మొదటి_Abovep(), Fork) : శూన్యసూచిక;
            ఒకవేళ (!forkp) forkp = కొత్త AstFork{flp, VJoinType::JOIN_NONE};
            ఒకవేళ (!m_underProcedure) {
                // If it's in a function, it won't be handled by V3Delayed
                // Put it behind an additional named event that gets triggered in the NBA region
                AstEventControl* స్థిరరాశి nbaEventControlp = createNbaEventControl(flp);
                Ast_కేటాయించు* స్థిరరాశి trigAssignp = createNbaEventTriggerAssignment(flp);
                nodep->replaceWith(trigAssignp);
                trigAssignp->కూడు_తదుపరి_ఇక్కడ(nbaEventControlp);
                nbaEventControlp->కూడు_Stmtsp(nodep);
                చొప్పించు_Beforep = forkp;
                ఒకవేళ (!controlp) controlp = nbaEventControlp;
            }
            controlp->replaceWith(forkp);
            AstBegin* beginp = VN_CAST(controlp, Begin);
            ఒకవేళ (!beginp) beginp = కొత్త AstBegin{nodep->fileline(), "", controlp, అబద్ధం};
            forkp->కూడు_Forksp(beginp);
            కూడు_Flags(beginp, T_NBA_UPDATE);
            controlp = forkp;
        }
        UASSERT_OBJ(nodep, controlp, "Assignment should have timing control");
        // Move `insertBeforep` into `AstCLocalScope` if necessary to avoid jumping over
        // a variable initialization that would be inserted before `insertBeforep`. All
        // access to this variable should be contained within returned `AstCLocalScope`.
        ఒకవేళ (m_under_Jump_ఖండం) కూడు_CLocal_Scope(flp, చొప్పించు_Beforep);
        // Function for replacing values with intermediate variables
        స్థిరరాశి స్వయంచాలక replaceWithIntermediate = [&](Ast_కణుపు_సమాసం* స్థిరరాశి valuep,
                                                 స్థిరరాశి std::string& name) {
            AstVarScope* స్థిరరాశి newvscp = create_తాత్కాలిక(flp, name, valuep->dtypep(), చొప్పించు_Beforep);
            // A pending update keeps its own value, even if its process schedules more
            ఒకవేళ (లోపల_కేటాయించు_Dly && !చొప్పించు_Beforep) newvscp->fork_By_విలువ(నిజం);
            valuep->replaceWith(కొత్త AstVarRef{flp, newvscp, VAccess::READ});
            controlp->కూడు_ఇక్కడ_This_As_తదుపరి(
                కొత్త Ast_కేటాయించు{flp, కొత్త AstVarRef{flp, newvscp, VAccess::WRITE}, valuep});
        };
        // NBAs with delays evaluate the indices selecting the target now (IEEE 1800-2023 10.4.2),
        // in order, each as a whole, also the selects and function calls in it. So follow the
        // selects of the target up from the variable, not into their indices. As an example, this
        // transforms 'x[a[i]][f(j)] <= #1 y' into 't0 = a[i]; t1 = f(j); x[t0][t1] <= #1 y'.
        // V3Const has split NBAs to concatenations, also streamed, into NBAs to each part.
        ఒకవేళ (లోపల_కేటాయించు_Dly) {
            కొరకు (Ast_కణుపు* exprp = nodep->lhsp()->పునాది_Fromp(నిజం); exprp != nodep;
                 exprp = exprp->మొదటి_Abovep()) {
                ఒకవేళ (AstSel* స్థిరరాశి selp = VN_CAST(exprp, Sel)) {
                    // widthp should be const
                    ఒకవేళ (!VN_IS(selp->lsbp(), Const)) {
                        replaceWithIntermediate(selp->lsbp(), m_intra_Lsb_పేర్లు.get(nodep));
                    }
                } లేకపోతే ఒకవేళ (Ast_కణుపు_Sel* స్థిరరాశి selp = VN_CAST(exprp, కణుపు_Sel)) {
                    ఒకవేళ (!VN_IS(selp->bitp(), Const)) {
                        replaceWithIntermediate(selp->bitp(), m_intra_సూచీ_పేర్లు.get(nodep));
                    }
                }
            }
        }
        // Replace the RHS with an intermediate value var
        replaceWithIntermediate(nodep->rhsp(), m_intra_విలువ_పేర్లు.get(nodep));
        ఒకవేళ (!లోపల_కేటాయించు_Dly) తిరిగి;
        // The process executing the NBA evaluates its delay (IEEE 1800-2023 4.9.4), not the forked
        // process, if it has side effects, e.g. calling a function that checks its process. A pure
        // one is evaluated as the forked process starts, at once.
        ఒకవేళ (AstDelay* స్థిరరాశి delayp = VN_CAST(timingp, Delay)) {
            ఒకవేళ (!delayp->lhsp()->is_శుద్ధ()) {
                replaceWithIntermediate(delayp->lhsp(), m_intra_విలువ_పేర్లు.get(nodep));
            }
        }
    }
    శూన్య సందర్శించు(AstFireEvent* nodep) override {
        // V3Delayed handles '->>' of a variable in a process. Like NBAs in non-inlined functions,
        // trigger other events from a fork awaiting the NBA region.
        ఒకవేళ (!nodep->isDelayed() || (m_underProcedure && VN_IS(nodep->operandp(), VarRef))) {
            iterateChildren(nodep);
            తిరిగి;
        }
        File_పంక్తి* స్థిరరాశి flp = nodep->fileline();
        Ast_కేటాయించు* స్థిరరాశి trigAssignp = createNbaEventTriggerAssignment(flp);
        nodep->replaceWith(trigAssignp);
        AstFork* స్థిరరాశి forkp = కొత్త AstFork{flp, VJoinType::JOIN_NONE};
        trigAssignp->కూడు_తదుపరి_ఇక్కడ(forkp);
        ఒకవేళ (m_under_Jump_ఖండం) కూడు_CLocal_Scope(flp, forkp);
        // The triggered event is the one referenced now, so evaluate handles and indices now
        Ast_కణుపు_సమాసం* స్థిరరాశి eventp = nodep->operandp()->unlink_Fr_వెనుక();
        స్థిరరాశి స్వయంచాలక evalNow = [&](Ast_కణుపు_సమాసం* స్థిరరాశి valuep, V3_Unique_పేర్లు& పేర్లు) {
            // Like for an NBA, a process uses a module variable, passed to the fork by value
            AstVarScope* స్థిరరాశి vscp = create_తాత్కాలిక(flp, పేర్లు.get(nodep), valuep->dtypep(),
                                                 m_underProcedure ? శూన్యసూచిక : forkp);
            ఒకవేళ (m_underProcedure) vscp->fork_By_విలువ(నిజం);
            valuep->replaceWith(కొత్త AstVarRef{flp, vscp, VAccess::READ});
            // Unlike an event, a handle selecting one is only read
            valuep->foreach([](Ast_కణుపు* స్థిరరాశి np) {
                ఒకవేళ (Ast_కణుపు_Var_Ref* స్థిరరాశి refp = VN_CAST(np, కణుపు_Var_Ref)) {
                    refp->access(VAccess::READ);
                } లేకపోతే ఒకవేళ (AstMemberSel* స్థిరరాశి selp = VN_CAST(np, MemberSel)) {
                    selp->access(VAccess::READ);
                }
            });
            forkp->కూడు_ఇక్కడ_This_As_తదుపరి(
                కొత్త Ast_కేటాయించు{flp, కొత్త AstVarRef{flp, vscp, VAccess::WRITE}, valuep});
        };
        Ast_కణుపు_సమాసం* refp = eventp;
        అయితే (Ast_కణుపు_Sel* స్థిరరాశి selp = VN_CAST(refp, కణుపు_Sel)) {
            ఒకవేళ (!VN_IS(selp->bitp(), Const)) evalNow(selp->bitp(), m_intra_సూచీ_పేర్లు);
            refp = selp->fromp();
        }
        // Handle of a class (or virtual interface)
        ఒకవేళ (AstMemberSel* స్థిరరాశి selp = VN_CAST(refp, MemberSel)) {
            evalNow(selp->fromp(), m_intra_విలువ_పేర్లు);
        }
        AstEventControl* స్థిరరాశి controlp = createNbaEventControl(flp);
        controlp->కూడు_Stmtsp(కొత్త AstFireEvent{flp, eventp, అబద్ధం});
        AstBegin* స్థిరరాశి beginp = కొత్త AstBegin{flp, "", controlp, అబద్ధం};
        కూడు_Flags(beginp, T_NBA_UPDATE);
        forkp->కూడు_Forksp(beginp);
        VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
    }
    శూన్య సందర్శించు(Ast_కేటాయించు_W* nodep) override {
        File_పంక్తి* స్థిరరాశి flp = nodep->fileline();
        // Get the net delay unless this assignment was created for handling the net delay (user1)
        AstDelay* స్థిరరాశి netDelayp = nodep->user1() ? శూన్యసూచిక : nodep->getLhsNetDelay();
        ఒకవేళ (netDelayp) {
            ఒకవేళ (nodep->timingControlp()) {
                // If this assignment has a delay, create another one to handle the net delay
                AstVarScope* స్థిరరాశి newvscp = m_కేటాయించు_WTmps.make(flp, m_scopep, nodep->dtypep());
                Ast_కేటాయించు_W* assignp = కొత్త Ast_కేటాయించు_W{
                    nodep->fileline(), nodep->lhsp()->unlink_Fr_వెనుక(),
                    కొత్త AstVarRef{flp, newvscp, VAccess::READ}, netDelayp->clone_వృక్షం(అబద్ధం)};
                assignp->user1(నిజం);
                nodep->కూడు_తదుపరి_ఇక్కడ(assignp);
                nodep->lhsp(కొత్త AstVarRef{flp, newvscp, VAccess::WRITE});
            } లేకపోతే {
                // Else just use this one with the net delay
                nodep->timingControlp(netDelayp->clone_వృక్షం(అబద్ధం));
            }
        }
        ఒకవేళ (!nodep->timingControlp()) తిరిగి;
        // There will be some circular logic here, suppress the warning for newly created vars
        flp->modifyWarnOff(V3_Error_సంకేతం::UNOPTFLAT, నిజం);
        // Also suppress the warning for the LHS var, for cases like `assign #1 clk = ~clk;`
        // TODO: Restore the warning for other, non-delayed drivers
        nodep->lhsp()->foreach([](AstVarRef* refp) {
            ఒకవేళ (refp->access().isWriteOrRW()) {
                refp->varp()->fileline()->modifyWarnOff(V3_Error_సంకేతం::UNOPTFLAT, నిజం);
            }
        });
        // Should be under an always
        AstAlways* స్థిరరాశి alwaysp = VN_AS(m_procp, Always);
        // Convert it to an Assign; the new assign with intra delay will be handled by
        // visit(AstNodeAssign*)
        Ast_కణుపు_సమాసం* స్థిరరాశి lhs1p = nodep->lhsp()->unlink_Fr_వెనుక();
        Ast_కణుపు_సమాసం* స్థిరరాశి rhs1p = nodep->rhsp()->unlink_Fr_వెనుక();
        Ast_కణుపు* స్థిరరాశి controlp = nodep->timingControlp()->unlink_Fr_వెనుక();
        ఒకవేళ (AstDelay* స్థిరరాశి delayp = VN_CAST(controlp, Delay)) {
            ఒకవేళ (Ast_కణుపు_సమాసం* fallDelayp = delayp->fallDelay()) {
                fallDelayp = fallDelayp->unlink_Fr_వెనుక();
                Ast_కణుపు_సమాసం* lhsp = delayp->lhsp()->unlink_Fr_వెనుక();
                // Use fall only for an all-zero value, rise otherwise.
                lhsp = delay_గమ్యం_Integral(delayp, lhsp);
                fallDelayp = delay_గమ్యం_Integral(delayp, fallDelayp);
                delayp->lhsp(
                    కొత్త AstCond{flp, కొత్త AstEq{flp, rhs1p->clone_వృక్షం(అబద్ధం), కొత్త AstConst{flp, 0}},
                                fallDelayp, lhsp});
            }
        }
        Ast_కేటాయించు* స్థిరరాశి assignp = కొత్త Ast_కేటాయించు{nodep->fileline(), lhs1p, rhs1p, controlp};
        // Put the assignment in a fork..join_none.
        AstFork* స్థిరరాశి forkp = కొత్త AstFork{flp, VJoinType::JOIN_NONE};
        nodep->replaceWith(forkp);
        VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
        AstBegin* స్థిరరాశి beginp = కొత్త AstBegin{flp, "", assignp, అబద్ధం};
        forkp->కూడు_Forksp(beginp);
        సందర్శించు(forkp);  // Visit now as we need to do some post-processing
        // IEEE 1800-2023 10.3.3 - if the RHS value differs from the currently scheduled value to
        // be assigned, the currently scheduled assignment is descheduled. To keep track if an
        // assignment should be descheduled, each scheduled assignment event has a 'generation',
        // and if at assignment time its generation differs from the current generation, it won't
        // be performed
        Ast_కేటాయించు* స్థిరరాశి preAssignp = VN_AS(beginp->stmtsp(), కేటాయించు);
        UASSERT_OBJ(preAssignp, alwaysp, "Pre-assign should be there from visit(AstFork)");
        Ast_కేటాయించు* స్థిరరాశి postAssignp = VN_AS(preAssignp->nextp()->nextp(), కేటాయించు);
        UASSERT_OBJ(postAssignp, alwaysp, "Post-assign should be there from visit(AstFork)");
        // Increment generation and copy it to a local
        AstVarScope* స్థిరరాశి generationVarp
            = m_కేటాయించు_WGens.make(flp, m_scopep, alwaysp->కనుగొను_UInt64_DType());
        AstVarScope* స్థిరరాశి genLocalVarp
            = create_తాత్కాలిక(flp, generationVarp->varp()->name() + "__local",
                         alwaysp->కనుగొను_UInt64_DType(), preAssignp);
        preAssignp->కూడు_ఇక్కడ_This_As_తదుపరి(
            కొత్త Ast_కేటాయించు{flp, కొత్త AstVarRef{flp, generationVarp, VAccess::WRITE},
                          కొత్త Ast_కూడు{flp, కొత్త AstVarRef{flp, generationVarp, VAccess::READ},
                                     కొత్త AstConst{flp, 1}}});
        preAssignp->కూడు_ఇక్కడ_This_As_తదుపరి(
            కొత్త Ast_కేటాయించు{flp, కొత్త AstVarRef{flp, genLocalVarp, VAccess::WRITE},
                          కొత్త AstVarRef{flp, generationVarp, VAccess::READ}});
        // If the current generation is same as the one saved in the local var, assign
        beginp->కూడు_Stmtsp(
            కొత్త AstIf{flp,
                      కొత్త AstEq{flp, కొత్త AstVarRef{flp, generationVarp, VAccess::READ},
                                కొత్త AstVarRef{flp, genLocalVarp, VAccess::READ}},
                      postAssignp->unlink_Fr_వెనుక()});
        // Save scheduled RHS value before delay
        AstVarScope* స్థిరరాశి tmpVarp
            = m_కేటాయించు_WTmps.make(flp, m_scopep, preAssignp->rhsp()->dtypep());
        AstVarRef* స్థిరరాశి tmp_కేటాయించు_Rhsp = VN_AS(preAssignp->lhsp(), VarRef)->clone_వృక్షం(అబద్ధం);
        tmp_కేటాయించు_Rhsp->access(VAccess::WRITE);
        preAssignp->కూడు_తదుపరి_ఇక్కడ(
            కొత్త Ast_కేటాయించు{flp, కొత్త AstVarRef{flp, tmpVarp, VAccess::WRITE}, tmp_కేటాయించు_Rhsp});
        // If the RHS is different from the currently scheduled value, schedule the new assignment
        // The generation will increase, effectively 'descheduling' the previous assignment.
        Ast_కణుపు_సమాసం* స్థిరరాశి didNotInitp
            = కొత్త AstLogNot{flp, కొత్త AstCExpr{flp, "vlSymsp->__Vm_didInit", 1}};
        AstVarRef* స్థిరరాశి మొదటి_Iterp
            = కొత్త AstVarRef{flp, m_netlistp->stl_మొదటి_Iterationp(), VAccess::READ};
        Ast_కణుపు_సమాసం* స్థిరరాశి schedCondp
            = కొత్త AstLogOr{flp,
                           కొత్త AstNeq{flp, preAssignp->rhsp()->clone_వృక్షం(అబద్ధం),
                                      కొత్త AstVarRef{flp, tmpVarp, VAccess::READ}},
                           కొత్త AstLogAnd{flp, didNotInitp, మొదటి_Iterp}};
        alwaysp->కూడు_Stmtsp(కొత్త AstIf{flp, schedCondp, forkp->unlink_Fr_వెనుక()});
    }
    శూన్య సందర్శించు(AstDisableFork* nodep) override {
        ఒకవేళ (m_hasProcess) తిరిగి;
        // never reached by any process; remove to avoid compilation error
        VL_DO_DANGLING(nodep->unlink_Fr_వెనుక()->delete_వృక్షం(), nodep);
    }
    శూన్య సందర్శించు(AstWaitFork* nodep) override {
        ఒకవేళ (m_hasProcess) {
            File_పంక్తి* స్థిరరాశి flp = nodep->fileline();
            AstCExpr* స్థిరరాశి exprp = కొత్త AstCExpr{flp, "vlProcess->completedFork()", 1};
            AstWait* స్థిరరాశి waitp = కొత్త AstWait{flp, exprp, శూన్యసూచిక};
            nodep->replaceWith(waitp);
        } లేకపోతే {
            // never reached by any process; remove to avoid compilation error
            nodep->unlink_Fr_వెనుక();
        }
        VL_DO_DANGLING(nodep->delete_వృక్షం(), nodep);
    }
    శూన్య సందర్శించు(AstWait* nodep) override {
        // Wait on changed events related to the vars in the wait statement
        UINFO(9, "control-visit " << nodep);
        File_పంక్తి* స్థిరరాశి flp = nodep->fileline();
        Ast_కణుపు* స్థిరరాశి stmtsp = nodep->stmtsp();
        ఒకవేళ (stmtsp) stmtsp->unlink_Fr_వెనుక_With_తదుపరి();
        Ast_కణుపు_సమాసం* స్థిరరాశి condp = V3Const::constifyEdit(nodep->condp()->unlink_Fr_వెనుక());
        స్వయంచాలక* స్థిరరాశి constp = VN_CAST(condp, Const);
        ఒకవేళ (constp) {
            ఒకవేళ (constp->isZero()) {
                // We have to await forever instead of simply returning in case we're deep in a
                // callstack
                AstCMethodHard* స్థిరరాశి foreverMethodp = కొత్త AstCMethodHard{
                    flp, కొత్త AstVarRef{flp, getCreateDelayScheduler(), VAccess::WRITE},
                    VCMethod::SCHED_WAIT_FOREVER};
                foreverMethodp->dtype_సమితి_Void();
                కూడు_Process_Info(foreverMethodp);
                కూడు_Debug_Info(foreverMethodp);
                AstCAwait* స్థిరరాశి awaitp = కొత్త AstCAwait{flp, foreverMethodp};
                nodep->replaceWith(awaitp);
                ఒకవేళ (stmtsp) VL_DO_DANGLING(stmtsp->delete_వృక్షం(), stmtsp);
                VL_DO_DANGLING(condp->delete_వృక్షం(), condp);
            } లేకపోతే {
                nodep->v3fatalSrc("constant wait should have been removed in "
                                  "TimingSuspendableVisitor::visit(AstWait)");
            }
        } లేకపోతే ఒకవేళ (needDynamicTrigger(condp)) {
            // No point in making a sentree, just use the expression as sensitivity
            // Put the event control in an if so we only wait if the condition isn't met already
            స్వయంచాలక* స్థిరరాశి ifp = కొత్త AstIf{
                flp, కొత్త AstLogNot{flp, condp},
                కొత్త AstEventControl{flp,
                                    కొత్త Ast_Sen_వృక్షం{flp, కొత్త Ast_Sen_అంశం{flp, VEdgeType::ET_TRUE,
                                                                       condp->clone_వృక్షం(అబద్ధం)}},
                                    శూన్యసూచిక}};
            ఒకవేళ (stmtsp) Ast_కణుపు::కూడు_తదుపరి<Ast_కణుపు, Ast_కణుపు>(ifp, stmtsp);
            nodep->replaceWith(ifp);
        } లేకపోతే {
            // We are using a global sentree, so we cannot use ET_TRUE, as that could lead to the
            // active region never converging. Because of this, we need to use ET_CHANGED in a
            // loop.
            AstEventControl* స్థిరరాశి controlp = కొత్త AstEventControl{
                flp,
                కొత్త Ast_Sen_వృక్షం{
                    flp, కొత్త Ast_Sen_అంశం{flp, VEdgeType::ET_మారింది, condp->clone_వృక్షం(అబద్ధం)}},
                శూన్యసూచిక};
            controlp->user2(నిజం);  // Commit immediately
            AstLoop* స్థిరరాశి loopp = కొత్త AstLoop{flp};
            loopp->కూడు_Stmtsp(కొత్త AstLoopTest{flp, loopp, కొత్త AstLogNot{flp, condp}});
            loopp->కూడు_Stmtsp(controlp);
            ఒకవేళ (stmtsp) Ast_కణుపు::కూడు_తదుపరి<Ast_కణుపు, Ast_కణుపు>(loopp, stmtsp);
            nodep->replaceWith(loopp);
        }
        VL_DO_DANGLING(nodep->delete_వృక్షం(), nodep);
    }
    శూన్య సందర్శించు(AstBegin* nodep) override {
        VL_RESTORER(m_procp);
        VL_RESTORER(m_hasProcess);
        m_hasProcess |= hasFlags(nodep, T_HAS_PROC);
        // A pending NBA update is not a subprocess of the process that scheduled it, so it is
        // unaffected by 'disable fork' and 'wait fork' (IEEE 1800-2023 9.6.1, 9.6.3)
        ఒకవేళ (hasFlags(nodep, T_NBA_UPDATE)) m_hasProcess = అబద్ధం;
        m_procp = nodep;
        ఒకవేళ (m_hasProcess) nodep->సమితి_Need_Process();
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(AstFork* nodep) override {
        ఒకవేళ (nodep->user1_సమితి_Once()) తిరిగి;
        UINFO(9, "control-visit " << nodep);
        v3Global.సమితి_ఉపయోగిస్తుంది_Timing();

        // Create a unique name for this fork
        nodep->name("__Vfork_" + std::గమ్యం_string(++m_forkCnt));

        // TODO: Should process nodep->stmtsp() in case an earlier pass
        //       inserted something there, but as of today we don't need to.

        // Process and name each fork
        కొలత_t idx = 0;  // Index for naming begins
        కొరకు (AstBegin *itemp = nodep->forksp(), *nextp; itemp; itemp = nextp) {
            nextp = VN_AS(itemp->nextp(), Begin);
            iterate(itemp);
            // Note: Even if we do not find any awaits, we cannot simply inline
            // the process here, as new awaits could be added later.

            // Name the begin (later the name will be used for a new function)
            itemp->name(nodep->name() + "__" + std::గమ్యం_string(idx++));
            ఒకవేళ (itemp->needProcess()) {
                itemp->కూడు_Stmtsp(
                    కొత్త AstCStmt{itemp->fileline(), "vlProcess->state(VlProcess::FINISHED);"});
            }
        }
        ఒకవేళ (!nodep->joinType().joinNone()) makeForkJoin(nodep);
    }

    //--------------------
    శూన్య సందర్శించు(AstVar*) override {}  // Accelerate
    శూన్య సందర్శించు(Ast_కణుపు* nodep) override { iterateChildren(nodep); }

బహిరంగ:
    // CONSTRUCTORS
    స్పష్టమైన TimingControlVisitor(AstNetlist* nodep)
        : m_netlistp{nodep} {
        iterate(nodep);

        // If there is no static #0 in the design, but an unknown delay was found,
        // and the user did not specify preference, then warn on all unknown delays
        // as we will be assuming they can be #0, which can cause performance degradation.
        కొరకు (File_పంక్తి* స్థిరరాశి flp : m_unknownDelayFlps) {
            flp->v3warn(ZERODLY,
                        "Value of # delay control statically unknown. Assuming it can be #0.\n"
                            << flp->warn_మరిన్ని()  //
                            << "... If all # delays are non-zero at runtime,\n"
                            << flp->warn_మరిన్ని()  //
                            << "... use '--no-sched-zero-delay' for improved performance.\n"
                            << flp->warn_మరిన్ని()  //
                            << "... If a real #0 is expected at runtime,\n"
                            << flp->warn_మరిన్ని()  //
                            << "... use '--sched-zero-delay' to suppress this warning.");
        }
    }
    ~TimingControlVisitor() override {
        V3Stats::కూడు_Stat("Timing, known #0 delays", m_statZeroDelays);
        V3Stats::కూడు_Stat("Timing, known #const delays", m_statConstDelays);
        V3Stats::కూడు_Stat("Timing, unknown #variable delays", m_statVariableDelays);
    }
};

//######################################################################
// Timing class functions

Ast_కణుపు_సమాసం* V3Timing::delay_గమ్యం_Ticks(Ast_కణుపు_సమాసం* valuep, VTimescale timeunit) {
    File_పంక్తి* స్థిరరాశి flp = valuep->fileline();
    ఒకవేళ (స్థిరరాశి AstConst* స్థిరరాశి constp = VN_CAST(valuep, Const)) {
        ఒకవేళ (constp->అంకె().is1_అడుగు()) {
            // A step is one tick of the global time precision (IEEE 1800-2023 3.14.3)
            VL_DO_DANGLING(valuep->delete_వృక్షం(), valuep);
            AstConst* స్థిరరాశి tickp = కొత్త AstConst{flp, AstConst::Unsized64{}, 1};
            tickp->dtype_సమితి_బిట్టు_Sized(64, VSigning::UNSIGNED);
            తిరిగి tickp;
        }
        ఒకవేళ (constp->isZero()) తిరిగి valuep;
    }
    UASSERT_OBJ(!timeunit.isNone(), valuep, "timeunit must be set");
    స్థిరరాశి ద్విగుణ timescale_గుణకం_D
        = std::ఘాతం(10.0, timeunit.power_యొక్క_Ten() - v3Global.rootp()->timeprecision().power_యొక్క_Ten());
    ఒకవేళ (valuep->dtypep()->skipRefp()->isDouble()) {
        AstConst* స్థిరరాశి tsfp = కొత్త AstConst{flp, AstConst::RealDouble{}, timescale_గుణకం_D};
        valuep = కొత్త AstMulD{flp, valuep, tsfp};
        valuep = కొత్త AstRToIRoundS{flp, valuep};
        valuep->dtype_సమితి_బిట్టు_Sized(64, VSigning::UNSIGNED);
    } లేకపోతే {
        valuep->dtype_సమితి_బిట్టు_Sized(64, VSigning::UNSIGNED);
        స్థిరరాశి uint64_t timescale_గుణకం_U = static_cast<uint64_t>(timescale_గుణకం_D);
        AstConst* స్థిరరాశి tsfp = కొత్త AstConst{flp, AstConst::Unsized64{}, timescale_గుణకం_U};
        valuep = కొత్త AstMul{flp, valuep, tsfp};
    }
    తిరిగి V3Const::constifyEdit(valuep);  // Simplify
}

శూన్య V3Timing::timingAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        స్థిరరాశి VNUser1_లోపల_Use m_user1_లోపల_Use;
        స్థిరరాశి VNUser2_లోపల_Use m_user2_లోపల_Use;
        { TimingSuspendableVisitor{nodep}; }
        ఒకవేళ (v3Global.ఉపయోగిస్తుంది_Timing()) TimingControlVisitor{nodep};
    }
    V3Global::dump_Check_Global_వృక్షం("timing", 0, dump_వృక్షం_Either_Level() >= 3);
}
