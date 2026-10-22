// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Scheduling - break combinational cycles
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
//*************************************************************************
//
// Combinational loops are broken by the introduction of instances of the
// 'hybrid' logic. Hybrid logic is like combinational logic, but also has
// explicit sensitivities. Any explicit sensitivity of hybrid logic suppresses
// the implicit sensitivity of the logic on the same variable. This enables us
// to cut combinational logic loops and perform ordering as if the logic is
// acyclic.  See the internals documentation for more details.
//
// To achieve this we build a dependency graph of all combinational logic in
// the design, and then breaks all combinational cycles by converting all
// combinational logic that consumes a variable driven via a 'back-edge' into
// hybrid logic. Here back-edge' just means a graph edge that points from a
// higher rank vertex to a lower rank vertex in some consistent ranking of
// the directed graph. Variables driven via a back-edge in the dependency
// graph are marked, and all combinational logic that depends on such
// variables is converted into hybrid logic, with the back-edge driven
// variables listed as explicit 'changed' sensitivities.
//
//*************************************************************************

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "V3EmitV.h"
#ضمن "V3File.h"
#ضمن "V3Graph.h"
#ضمن "V3Sched.h"
#ضمن "V3SenTree.h"
#ضمن "V3Stats.h"

#ضمن <map>
#ضمن <unordered_map>
#ضمن <unordered_set>
#ضمن <utility>
#ضمن <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

نطاق V3Sched {

نطاق {

// ##############################################################################
//  Data structures (graph types)

صنف Sched_Acyclic_Logic_راس final : عام V3_گراف_راس {
    VL_RTTI_IMPL(Sched_Acyclic_Logic_راس, V3_گراف_راس)
    Ast_ڳنڍ* ثابت m_logicp;  // The logic node this vertex represents
    AstScope* ثابت m_scopep;  // The enclosing AstScope of the logic node

عام:
    Sched_Acyclic_Logic_راس(V3_گراف* graphp, Ast_ڳنڍ* logicp, AstScope* scopep)
        : V3_گراف_راس{graphp}
        , m_logicp{logicp}
        , m_scopep{scopep} {}
    V3_گراف_راس* clone(V3_گراف* graphp) ثابت override {
        أرجع جديد Sched_Acyclic_Logic_راس{graphp, logicp(), scopep()};
    }

    Ast_ڳنڍ* logicp() ثابت { أرجع m_logicp; }
    AstScope* scopep() ثابت { أرجع m_scopep; }

    // LCOV_EXCL_START // Debug code
    string name() ثابت override VL_MT_STABLE { أرجع m_logicp->fileline()->ascii(); };
    string dotShape() ثابت override { أرجع "rectangle2"; }
    // LCOV_EXCL_STOP
};

صنف Sched_Acyclic_Var_راس final : عام V3_گراف_راس {
    VL_RTTI_IMPL(Sched_Acyclic_Var_راس, V3_گراف_راس)
    AstVarScope* ثابت m_vscp;  // The AstVarScope this vertex represents

عام:
    Sched_Acyclic_Var_راس(V3_گراف* graphp, AstVarScope* vscp)
        : V3_گراف_راس{graphp}
        , m_vscp{vscp} {}
    AstVarScope* vscp() ثابت { أرجع m_vscp; }
    AstVar* varp() ثابت { أرجع m_vscp->varp(); }
    V3_گراف_راس* clone(V3_گراف* graphp) ثابت override {
        أرجع جديد Sched_Acyclic_Var_راس{graphp, vscp()};
    }

    // LCOV_EXCL_START // Debug code
    string name() ثابت override VL_MT_STABLE { أرجع m_vscp->name(); }
    string dotShape() ثابت override { أرجع "ellipse"; }
    string dotColor() ثابت override { أرجع "blue"; }
    // LCOV_EXCL_STOP
};

صنف گراف_2 final : عام V3_گراف {
    string loops_راس_Cb(V3_گراف_راس* vtxp) override {
        إذا (Sched_Acyclic_Logic_راس* ثابت lvtxp = vtxp->cast<Sched_Acyclic_Logic_راس>()) {
            Ast_ڳنڍ* ثابت logicp = lvtxp->logicp();
            std::string logicName = logicp->prettyTypeName();
            إذا (ثابت AstAlways* ثابت alwaysp = VN_CAST(logicp, Always)) {
                إذا (alwaysp->keyword() == VAlwaysKwd::CONT_سونپيو) {
                    logicName = "ASSIGNW";  // Keep using historiacl name until we have better
                }
            }
            أرجع logicp->fileline()->warnOther() + "     Example path: " + logicName + "\n";
        } وإلا {
            Sched_Acyclic_Var_راس* ثابت vvtxp = vtxp->as<Sched_Acyclic_Var_راس>();
            AstVarScope* ثابت vscp = vvtxp->vscp();
            أرجع vscp->fileline()->warnOther() + "     Example path: " + vscp->prettyName()
                   + "\n";
        }
    }
};

//##############################################################################
// Algorithm implementation

std::unique_ptr<گراف_2> build_گراف(ثابت LogicByScope& lbs) {
    std::unique_ptr<گراف_2> graphp{جديد گراف_2};

    // AstVarScope::user1() -> VarVertx
    ثابت VNUser1_اندريون_Use user1_اندريون_Use;
    ثابت خودڪار get_Var_راس = [&](AstVarScope* vscp) {
        إذا (!vscp->user1p()) vscp->user1p(جديد Sched_Acyclic_Var_راس{graphp.get(), vscp});
        أرجع vscp->user1u().منزل<Sched_Acyclic_Var_راس*>();
    };

    ثابت خودڪار جوڙيو_ڪنارو = [&](V3_گراف_راس* fromp, V3_گراف_راس* چوٽي, صحيح وزن_2, منطقي cuttable) {
        جديد V3_گراف_ڪنارو{graphp.get(), fromp, چوٽي, وزن_2, cuttable};
    };

    لكل (ثابت خودڪار& داخلا_2 : lbs) {
        AstScope* ثابت scopep = داخلا_2.m_scopep;
        Ast_سرگرم* ثابت activep = داخلا_2.m_activep;
        UASSERT_OBJ(activep->hasCombo(), activep, "Not combinational logic");
        لكل (Ast_ڳنڍ* nodep = activep->stmtsp(); nodep; nodep = nodep->nextp()) {
            // Can safely ignore Postponed as we generate them all
            إذا (VN_IS(nodep, AlwaysPostponed)) استمر;

            Sched_Acyclic_Logic_راس* ثابت lvtxp
                = جديد Sched_Acyclic_Logic_راس{graphp.get(), nodep, scopep};
            ثابت VNUser2_اندريون_Use user2_اندريون_Use;
            ثابت VNUser3_اندريون_Use user3_اندريون_Use;

            V3Sched::util::Var_Scope_سيٽ force_Read_ڪنارو_Ignores;
            V3Sched::util::collect_Force_Read_ڪنارو_Ignores(nodep, force_Read_ڪنارو_Ignores);

            nodep->foreach([&](AstVarRef* refp) {
                AstVarScope* ثابت vscp = refp->varScopep();
                Sched_Acyclic_Var_راس* ثابت vvtxp = get_Var_راس(vscp);
                // We want to cut the narrowest signals
                ثابت صحيح وزن_2 = vscp->ويڪر() / 8 + 1;
                // If written, add logic -> var edge
                إذا (refp->access().isWriteOrRW() && !refp->varp()->ignoreSchedWrite()
                    && !vscp->user2_سيٽ_Once())
                    جوڙيو_ڪنارو(lvtxp, vvtxp, وزن_2, سچ);
                // If read, add var -> logic edge
                // Note: Use same heuristic as ordering does to ignore written variables
                // TODO: Use live variable analysis.
                إذا (refp->access().isReadOrRW() && !vscp->user3_سيٽ_Once() && !vscp->user2()
                    && !force_Read_ڪنارو_Ignores.ڳڻپ(vscp))
                    جوڙيو_ڪنارو(vvtxp, lvtxp, وزن_2, سچ);
            });
        }
    }

    أرجع graphp;
}

فراغ removeNonCyclic(گراف_2* graphp) {
    // Work queue
    std::ويڪٽر<V3_گراف_راس*> قطار;

    ثابت خودڪار قطار_اندر_وجهو = [&](V3_گراف_راس* vtxp) {
        إذا (vtxp->user()) أرجع;  // Already in queue
        vtxp->user(1);
        قطار.ڌڪو_پٺ(vtxp);
    };

    // Start with vertices with no inputs or outputs
    لكل (V3_گراف_راس& vtx : graphp->vertices()) {
        إذا (vtx.اندريون_خالي_جايون() || vtx.ٻاهريون_خالي_جايون()) قطار_اندر_وجهو(&vtx);
    }

    // Iterate while we still have candidates
    طالما (!قطار.خالي_جايون()) {
        // Pop next candidate
        V3_گراف_راس* ثابت vtxp = قطار.پٺ();
        قطار.ڪڍو_پٺ();
        vtxp->user(0);  // No longer in queue

        إذا (vtxp->اندريون_خالي_جايون()) {
            // Enqueue children for consideration, remove out edges, and delete this vertex
            لكل (V3_گراف_ڪنارو* ثابت edgep : vtxp->ٻاهريون_ڪنارا().unlinkable()) {
                قطار_اندر_وجهو(edgep->چوٽي());
                VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
            }
            VL_DO_DANGLING(vtxp->unlinkDelete(graphp), vtxp);
        } وإلا إذا (vtxp->ٻاهريون_خالي_جايون()) {
            // Enqueue parents for consideration, remove in edges, and delete this vertex
            لكل (V3_گراف_ڪنارو* ثابت edgep : vtxp->اندريون_ڪنارا().unlinkable()) {
                قطار_اندر_وجهو(edgep->fromp());
                VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
            }
            VL_DO_DANGLING(vtxp->unlinkDelete(graphp), vtxp);
        }
    }
}

// Has this VarVertex been cut? (any edges in or out has been cut)
منطقي isCut(ثابت Sched_Acyclic_Var_راس* vtxp) {
    لكل (ثابت V3_گراف_ڪنارو& ڪنارو : vtxp->اندريون_ڪنارا()) {
        إذا (ڪنارو.وزن_2() == 0) أرجع سچ;
    }
    لكل (ثابت V3_گراف_ڪنارو& ڪنارو : vtxp->ٻاهريون_ڪنارا()) {
        إذا (ڪنارو.وزن_2() == 0) أرجع سچ;
    }
    أرجع خطأ;
}

std::ويڪٽر<Sched_Acyclic_Var_راس*> ڳوليو_Cut_Vertices(گراف_2* graphp) {
    // List of cut vertices being computed here
    std::ويڪٽر<Sched_Acyclic_Var_راس*> نتيجو;
    ثابت VNUser1_اندريون_Use user1_اندريون_Use;  // bool: already added to result

    // Statistics
    سائيز_t nCyclicVtxs = 0;  // Number of vertices that are part of an SCC (cycle)
    سائيز_t nCyclicVars = 0;  // Number of variables that are part of an SCC (cycle)
    std::unordered_سيٽ<uint32_t> sccs;  // Unique SCC colors

    لكل (V3_گراف_راس& vtx : graphp->vertices()) {
        إذا (!vtx.color()) استمر;  // Not part of an SCC (cycle), can ignore
        ++nCyclicVtxs;
        إذا (Sched_Acyclic_Var_راس* ثابت vvtxp = vtx.cast<Sched_Acyclic_Var_راس>()) {
            ++nCyclicVars;
            إذا (!vvtxp->vscp()->user1_سيٽ_Once() && isCut(vvtxp)) نتيجو.ڌڪو_پٺ(vvtxp);
        }
        // Don't bother counting if not dumping statistics
        إذا (v3Global.opt.stats()) sccs.وجهو(vtx.color());
    }

    V3Stats::جوڙيو_Stat("Scheduling, Cycles, cyclic variables", nCyclicVars);
    V3Stats::جوڙيو_Stat("Scheduling, Cycles, cyclic logic blocks", nCyclicVtxs - nCyclicVars);
    V3Stats::جوڙيو_Stat("Scheduling, Cycles, unique SCCs", sccs.سائيز());
    V3Stats::جوڙيو_Stat("Scheduling, Cycles, cut variables", نتيجو.سائيز());

    أرجع نتيجو;
}

فراغ reset_ڪنارو_Weights(ثابت std::ويڪٽر<Sched_Acyclic_Var_راس*>& cutVertices) {
    لكل (Sched_Acyclic_Var_راس* ثابت vvtxp : cutVertices) {
        لكل (V3_گراف_ڪنارو& e : vvtxp->اندريون_ڪنارا()) e.وزن_2(1);
        لكل (V3_گراف_ڪنارو& e : vvtxp->ٻاهريون_ڪنارا()) e.وزن_2(1);
    }
}

// A VarVertex together with its fanout
استعمال Candidate = std::pair<Sched_Acyclic_Var_راس*, بنانشان>;

// Gather all splitting candidates that are in the same SCC as the given vertex
فراغ gatherSCCCandidates(V3_گراف_راس* vtxp, std::ويڪٽر<Candidate>& candidates) {
    إذا (vtxp->user()) أرجع;  // Already done
    vtxp->user(سچ);

    إذا (Sched_Acyclic_Var_راس* ثابت vvtxp = vtxp->cast<Sched_Acyclic_Var_راس>()) {
        AstVar* ثابت varp = vvtxp->varp();
        ثابت string name = varp->prettyName();
        إذا (!varp->user3_سيٽ_Once()  // Only consider each AstVar once
            && varp->ويڪر() != 1  // Ignore 1-bit signals (they cannot be split further)
            && name.ڳوليو("__Vdly") == string::npos  // Ignore internal signals
            && name.ڳوليو("__Vcell") == string::npos) {
            // Also compute the fanout of this vertex
            ثابت بنانشان fanout = vtxp->ٻاهريون_ڪنارا().سائيز();
            candidates.emplace_پٺ(vvtxp, fanout);
        }
    }

    // Iterate through all the vertices within the same strongly connected component (same color)
    لكل (V3_گراف_ڪنارو& ڪنارو : vtxp->ٻاهريون_ڪنارا()) {
        V3_گراف_راس* ثابت چوٽي = ڪنارو.چوٽي();
        إذا (چوٽي->color() == vtxp->color()) gatherSCCCandidates(چوٽي, candidates);
    }
    لكل (V3_گراف_ڪنارو& ڪنارو : vtxp->اندريون_ڪنارا()) {
        V3_گراف_راس* ثابت fromp = ڪنارو.fromp();
        إذا (fromp->color() == vtxp->color()) gatherSCCCandidates(fromp, candidates);
    }
}

// Find all variables in a loop (SCC) that are candidates for splitting to break loops.
std::string reportLoopVars(File_سٽ* /*warnFl*/, گراف_2* graphp, Sched_Acyclic_Var_راس* vvtxp) {
    std::ostringstream ss;
    // Vector of variables in UNOPTFLAT loop that are candidates for splitting.
    std::ويڪٽر<Candidate> candidates;
    {
        // AstNode::user3 is used to mark if we have done a particular variable.
        // V3GraphVertex::user is used to mark if we have seen this vertex before.
        ثابت VNUser3_اندريون_Use user3_اندريون_Use;
        graphp->userClearVertices();
        gatherSCCCandidates(vvtxp, candidates);
        graphp->userClearVertices();
    }

    // Possible we only have candidates the user cannot do anything about, so don't bother them.
    إذا (candidates.خالي_جايون()) أرجع "";

    // There may be a very large number of candidates, so only report up to 10 of the "most
    // important" signals.
    بنانشان splittable = 0;
    ثابت خودڪار reportFirst10
        = [&](std::function<منطقي(ثابت Candidate&, ثابت Candidate&)> less) -> string {
        std::stable_ترتيب_ڏيو(candidates.begin(), candidates.پڇاڙي(), less);
        std::ostringstream ss2;
        لكل (سائيز_t i = 0; i < 10; i++) {
            إذا (i == candidates.سائيز()) اكسر;
            ثابت Candidate& candidate = candidates[i];
            AstVar* ثابت varp = candidate.پهريون->varp();

            ss2 << V3Error::warn_وڌيڪ() << "    " << varp->fileline() << ' ' << varp->prettyName()
                << ", width " << std::dec << varp->ويڪر() << ", circular fanout "
                << candidate.ٻيو;
            إذا (!varp->cannot_ورهايو_قسم_Reason()) {
                ss2 << ", can split_var";
                ++splittable;
            }
            ss2 << '\n';
        }
        أرجع ss2.str();
    };

    // Widest variables
    ss << V3Error::warn_وڌيڪ() << "... Widest variables candidate to splitting:\n"
       << reportFirst10([](ثابت Candidate& a, ثابت Candidate& b) {
              أرجع a.پهريون->varp()->ويڪر() > b.پهريون->varp()->ويڪر();
          });

    // Highest fanout
    ss << V3Error::warn_وڌيڪ() << "... Candidates with the highest fanout:\n"
       << reportFirst10([](ثابت Candidate& a, ثابت Candidate& b) {  //
              أرجع a.ٻيو > b.ٻيو;
          });

    إذا (splittable) {
        ss << V3Error::warn_وڌيڪ()
           << "... Suggest add /*verilator split_var*/ to appropriate variables above.\n";
    }
    V3Stats::جوڙيو_Stat("Scheduling, split_var, candidates", splittable);
    أرجع ss.str();
}

فراغ report_چڪرن(گراف_2* graphp, ثابت std::ويڪٽر<Sched_Acyclic_Var_راس*>& cutVertices) {
    لكل (Sched_Acyclic_Var_راس* vvtxp : cutVertices) {
        AstVarScope* ثابت vscp = vvtxp->vscp();
        File_سٽ* ثابت flp = vscp->fileline();

        // First v3warn not inside warnIsOff so we can see the suppressions with --debug
        إذا (flp->warnIsOff(V3_Error_ڪوڊ::UNOPTFLAT)) {
            // First v3warn not inside warnIsOff so we can see the suppressions with --debug
            vscp->v3warn(UNOPTFLAT, "Signal unoptimizable: Circular combinational logic: "
                                        << vscp->prettyNameQ());
        } وإلا {
            vscp->v3warn(UNOPTFLAT,
                         "Signal unoptimizable: Circular combinational logic: "
                             << vscp->prettyNameQ() << '\n'
                             << vscp->warnContextPrimary()
                             << V3Error::warnAdditionalInfo()
                             // Calls Graph::loopsVertexCb
                             << graphp->reportLoops(&V3_گراف_ڪنارو::followAlwaysTrue, vvtxp)
                             // Report candidate variables for splitting
                             << (v3Global.opt.reportUnoptflat()
                                     ? reportLoopVars(vscp->fileline(), graphp, vvtxp)
                                     : ""));
            // Complain just once
            flp->modifyWarnOff(V3_Error_ڪوڊ::UNOPTFLAT, سچ);
            // Create a subgraph for the UNOPTFLAT loop
            إذا (v3Global.opt.reportUnoptflat()) {
                V3_گراف loop_گراف;
                graphp->subtreeLoops(&V3_گراف_ڪنارو::followAlwaysTrue, vvtxp, &loop_گراف);
                loop_گراف.dumpDotFilePrefixedAlways("unoptflat");
            }
        }
    }
}

فراغ dumpSccs(V3_گراف* graphp) {
    // Map from SCC color to vertices in that SCC
    std::map<uint32_t, std::ويڪٽر<V3_گراف_راس*>> scc2Vtxps;

    // Gather all vertices in each SCC
    لكل (V3_گراف_راس& vtx : graphp->vertices()) {
        إذا (!vtx.color()) استمر;
        scc2Vtxps[vtx.color()].ڌڪو_پٺ(&vtx);
    }

    // Dump Verilog for each SCC into separate files
    لكل (ثابت خودڪار& pair : scc2Vtxps) {
        ثابت uint32_t color = pair.پهريون;
        ثابت std::ويڪٽر<V3_گراف_راس*>& vtxps = pair.ٻيو;

        // Open dump file
        ثابت std::string fname
            = v3Global.debugFilename("sched_scc_" + std::منزل_string(color) + ".v");
        ثابت std::unique_ptr<std::ofstream> ofp{V3File::new_ofstream(fname)};
        إذا (ofp->fail()) v3fatal("Can't write file: " << fname);

        // Write header
        *ofp << "// SCC " << color << ", size: " << vtxps.سائيز() << "\n\n";

        // Dump variables
        *ofp << "//////////////////////////////////////////////////////////////////////\n";
        *ofp << "// Variables\n";
        *ofp << "//////////////////////////////////////////////////////////////////////\n";
        *ofp << "\n";
        لكل (V3_گراف_راس* vtxp : vtxps) {
            ثابت Sched_Acyclic_Var_راس* ثابت vvtxp = vtxp->cast<Sched_Acyclic_Var_راس>();
            إذا (!vvtxp) استمر;
            AstVarScope* ثابت vscp = vvtxp->vscp();
            *ofp << "// " << vscp->fileline()->ascii() << "\n";
            *ofp << "// " << vscp->prettyName() << "\n";
            V3EmitV::debug_Verilog_For_وڻ(vscp->varp(), *ofp);
            *ofp << "\n";
        }

        // Dump logic
        *ofp << "\n";
        *ofp << "//////////////////////////////////////////////////////////////////////\n";
        *ofp << "// Logic\n";
        *ofp << "//////////////////////////////////////////////////////////////////////\n";
        *ofp << "\n";
        لكل (V3_گراف_راس* vtxp : vtxps) {
            ثابت Sched_Acyclic_Logic_راس* ثابت lvtxp = vtxp->cast<Sched_Acyclic_Logic_راس>();
            إذا (!lvtxp) استمر;
            *ofp << "// " << lvtxp->logicp()->fileline()->ascii() << "\n";
            V3EmitV::debug_Verilog_For_وڻ(lvtxp->logicp(), *ofp);
            *ofp << "\n";
        }
    }
}

LogicByScope fixCuts(AstNetlist* netlistp,
                     ثابت std::ويڪٽر<Sched_Acyclic_Var_راس*>& cutVertices) {
    // For all logic that reads a cut vertex, build a map from logic -> list of cut AstVarScope
    // they read. Also build a vector of the involved logic for deterministic results.
    std::unordered_map<Sched_Acyclic_Logic_راس*, std::ويڪٽر<AstVarScope*>> lvtx2Cuts;
    std::ويڪٽر<Sched_Acyclic_Logic_راس*> lvtxps;
    {
        ثابت VNUser1_اندريون_Use user1_اندريون_Use;  // bool: already added to 'lvtxps'
        لكل (Sched_Acyclic_Var_راس* ثابت vvtxp : cutVertices) {
            لكل (V3_گراف_ڪنارو& ڪنارو : vvtxp->ٻاهريون_ڪنارا()) {
                Sched_Acyclic_Logic_راس* ثابت lvtxp
                    = static_cast<Sched_Acyclic_Logic_راس*>(ڪنارو.چوٽي());
                إذا (!lvtxp->logicp()->user1_سيٽ_Once()) lvtxps.ڌڪو_پٺ(lvtxp);
                lvtx2Cuts[lvtxp].ڌڪو_پٺ(vvtxp->vscp());
            }
        }
    }

    // Make the logic reading cut vertices use a hybrid sensitivity (combinational, but with some
    // explicit additional triggers on the cut variables)
    LogicByScope نتيجو;
    Sen_وڻ_Finder finder{netlistp};
    لكل (Sched_Acyclic_Logic_راس* ثابت lvtxp : lvtxps) {
        Ast_ڳنڍ* ثابت logicp = lvtxp->logicp();
        logicp->unlink_Fr_پٺ();
        File_سٽ* ثابت flp = logicp->fileline();
        // Build the hybrid sensitivity list
        Ast_Sen_شي* senItemsp = خالي_اشارو;
        لكل (AstVarScope* ثابت vscp : lvtx2Cuts[lvtxp]) {
            AstVarRef* ثابت refp = جديد AstVarRef{flp, vscp, VAccess::READ};
            Ast_Sen_شي* ثابت nextp = جديد Ast_Sen_شي{flp, VEdgeType::ET_HYBRID, refp};
            senItemsp = Ast_ڳنڍ::جوڙيو_ايندڙ(senItemsp, nextp);
        }
        Ast_Sen_وڻ* ثابت sen_وڻ = جديد Ast_Sen_وڻ{flp, senItemsp};
        // Add logic to result with new sensitivity
        نتيجو.جوڙيو(lvtxp->scopep(), finder.get_Sen_وڻ(sen_وڻ), logicp);
        // SenTreeFinder::getSenTree clones, so clean up
        VL_DO_DANGLING(sen_وڻ->delete_وڻ(), sen_وڻ);
    }
    أرجع نتيجو;
}

}  // namespace

LogicByScope break_چڪرن(AstNetlist* netlistp, ثابت LogicByScope& combinationalLogic) {
    // Build the dataflow (dependency) graph
    ثابت std::unique_ptr<گراف_2> graphp = build_گراف(combinationalLogic);

    // Remove nodes that don't form part of a cycle
    removeNonCyclic(graphp.get());

    // Nothing to do if no cycles, yay!
    إذا (graphp->خالي_جايون()) أرجع LogicByScope{};

    // Dump for debug
    إذا (dump_گراف_Level() >= 6) graphp->dumpDotFilePrefixed("sched-comb-cycles");

    // Make graph acyclic by cutting some edges. Note: This also colors strongly connected
    // components which reportCycles uses to print each SCCs separately.
    // TODO: A more optimal algorithm that cuts by removing/marking VarVertex vertices is possible
    //       Search for "Feedback vertex set" (current algorithm is "Feedback arc set")
    graphp->acyclic(&V3_گراف_ڪنارو::followAlwaysTrue);

    // Find all cut vertices
    ثابت std::ويڪٽر<Sched_Acyclic_Var_راس*> cutVertices = ڳوليو_Cut_Vertices(graphp.get());

    // Reset edge weights for reporting
    reset_ڪنارو_Weights(cutVertices);

    // Report warnings/diagnostics
    report_چڪرن(graphp.get(), cutVertices);

    // Debug dump
    إذا (dumpLevel() >= 6) dumpSccs(graphp.get());

    // Fix cuts by converting dependent logic to use hybrid sensitivities
    أرجع fixCuts(netlistp, cutVertices);
}

}  // namespace V3Sched
