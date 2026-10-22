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
    Ast_گرہ* ثابت m_logicp;  // The logic node this vertex represents
    AstScope* ثابت m_scopep;  // The enclosing AstScope of the logic node

عام:
    Sched_Acyclic_Logic_راس(V3_گراف* graphp, Ast_گرہ* logicp, AstScope* scopep)
        : V3_گراف_راس{graphp}
        , m_logicp{logicp}
        , m_scopep{scopep} {}
    V3_گراف_راس* clone(V3_گراف* graphp) ثابت override {
        أرجع جديد Sched_Acyclic_Logic_راس{graphp, logicp(), scopep()};
    }

    Ast_گرہ* logicp() ثابت { أرجع m_logicp; }
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
            Ast_گرہ* ثابت logicp = lvtxp->logicp();
            std::string logicName = logicp->prettyTypeName();
            إذا (ثابت AstAlways* ثابت alwaysp = VN_CAST(logicp, Always)) {
                إذا (alwaysp->keyword() == VAlwaysKwd::CONT_سونپو) {
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
    ثابت VNUser1_وچ_Use user1_وچ_Use;
    ثابت خودکار get_Var_راس = [&](AstVarScope* vscp) {
        إذا (!vscp->user1p()) vscp->user1p(جديد Sched_Acyclic_Var_راس{graphp.get(), vscp});
        أرجع vscp->user1u().منزل<Sched_Acyclic_Var_راس*>();
    };

    ثابت خودکار جوڑو_کنارا = [&](V3_گراف_راس* fromp, V3_گراف_راس* چوٹی, صحيح بھار_2, منطقي cuttable) {
        جديد V3_گراف_کنارا{graphp.get(), fromp, چوٹی, بھار_2, cuttable};
    };

    لكل (ثابت خودکار& اندراج : lbs) {
        AstScope* ثابت scopep = اندراج.m_scopep;
        Ast_سرگرم* ثابت activep = اندراج.m_activep;
        UASSERT_OBJ(activep->hasCombo(), activep, "Not combinational logic");
        لكل (Ast_گرہ* nodep = activep->stmtsp(); nodep; nodep = nodep->nextp()) {
            // Can safely ignore Postponed as we generate them all
            إذا (VN_IS(nodep, AlwaysPostponed)) استمر;

            Sched_Acyclic_Logic_راس* ثابت lvtxp
                = جديد Sched_Acyclic_Logic_راس{graphp.get(), nodep, scopep};
            ثابت VNUser2_وچ_Use user2_وچ_Use;
            ثابت VNUser3_وچ_Use user3_وچ_Use;

            V3Sched::util::Var_Scope_سیٹ force_Read_کنارا_Ignores;
            V3Sched::util::collect_Force_Read_کنارا_Ignores(nodep, force_Read_کنارا_Ignores);

            nodep->foreach([&](AstVarRef* refp) {
                AstVarScope* ثابت vscp = refp->varScopep();
                Sched_Acyclic_Var_راس* ثابت vvtxp = get_Var_راس(vscp);
                // We want to cut the narrowest signals
                ثابت صحيح بھار_2 = vscp->چوڑائی() / 8 + 1;
                // If written, add logic -> var edge
                إذا (refp->access().isWriteOrRW() && !refp->varp()->ignoreSchedWrite()
                    && !vscp->user2_سیٹ_Once())
                    جوڑو_کنارا(lvtxp, vvtxp, بھار_2, سچ);
                // If read, add var -> logic edge
                // Note: Use same heuristic as ordering does to ignore written variables
                // TODO: Use live variable analysis.
                إذا (refp->access().isReadOrRW() && !vscp->user3_سیٹ_Once() && !vscp->user2()
                    && !force_Read_کنارا_Ignores.گنتی(vscp))
                    جوڑو_کنارا(vvtxp, lvtxp, بھار_2, سچ);
            });
        }
    }

    أرجع graphp;
}

فراغ removeNonCyclic(گراف_2* graphp) {
    // Work queue
    std::سمتیہ<V3_گراف_راس*> قطار;

    ثابت خودکار قطار_وچ_پاؤ = [&](V3_گراف_راس* vtxp) {
        إذا (vtxp->user()) أرجع;  // Already in queue
        vtxp->user(1);
        قطار.ڈالو_پٹھ(vtxp);
    };

    // Start with vertices with no inputs or outputs
    لكل (V3_گراف_راس& vtx : graphp->vertices()) {
        إذا (vtx.وچ_خالی_تھاواں() || vtx.باہرلا_خالی_تھاواں()) قطار_وچ_پاؤ(&vtx);
    }

    // Iterate while we still have candidates
    طالما (!قطار.خالی_تھاواں()) {
        // Pop next candidate
        V3_گراف_راس* ثابت vtxp = قطار.پٹھ();
        قطار.نکالو_پٹھ();
        vtxp->user(0);  // No longer in queue

        إذا (vtxp->وچ_خالی_تھاواں()) {
            // Enqueue children for consideration, remove out edges, and delete this vertex
            لكل (V3_گراف_کنارا* ثابت edgep : vtxp->باہرلا_کنارے().unlinkable()) {
                قطار_وچ_پاؤ(edgep->چوٹی());
                VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
            }
            VL_DO_DANGLING(vtxp->unlinkDelete(graphp), vtxp);
        } وإلا إذا (vtxp->باہرلا_خالی_تھاواں()) {
            // Enqueue parents for consideration, remove in edges, and delete this vertex
            لكل (V3_گراف_کنارا* ثابت edgep : vtxp->وچ_کنارے().unlinkable()) {
                قطار_وچ_پاؤ(edgep->fromp());
                VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
            }
            VL_DO_DANGLING(vtxp->unlinkDelete(graphp), vtxp);
        }
    }
}

// Has this VarVertex been cut? (any edges in or out has been cut)
منطقي isCut(ثابت Sched_Acyclic_Var_راس* vtxp) {
    لكل (ثابت V3_گراف_کنارا& کنارا : vtxp->وچ_کنارے()) {
        إذا (کنارا.بھار_2() == 0) أرجع سچ;
    }
    لكل (ثابت V3_گراف_کنارا& کنارا : vtxp->باہرلا_کنارے()) {
        إذا (کنارا.بھار_2() == 0) أرجع سچ;
    }
    أرجع خطأ;
}

std::سمتیہ<Sched_Acyclic_Var_راس*> لبھو_Cut_Vertices(گراف_2* graphp) {
    // List of cut vertices being computed here
    std::سمتیہ<Sched_Acyclic_Var_راس*> نتیجہ;
    ثابت VNUser1_وچ_Use user1_وچ_Use;  // bool: already added to result

    // Statistics
    ناپ_t nCyclicVtxs = 0;  // Number of vertices that are part of an SCC (cycle)
    ناپ_t nCyclicVars = 0;  // Number of variables that are part of an SCC (cycle)
    std::unordered_سیٹ<uint32_t> sccs;  // Unique SCC colors

    لكل (V3_گراف_راس& vtx : graphp->vertices()) {
        إذا (!vtx.color()) استمر;  // Not part of an SCC (cycle), can ignore
        ++nCyclicVtxs;
        إذا (Sched_Acyclic_Var_راس* ثابت vvtxp = vtx.cast<Sched_Acyclic_Var_راس>()) {
            ++nCyclicVars;
            إذا (!vvtxp->vscp()->user1_سیٹ_Once() && isCut(vvtxp)) نتیجہ.ڈالو_پٹھ(vvtxp);
        }
        // Don't bother counting if not dumping statistics
        إذا (v3Global.opt.stats()) sccs.داخل_کرو(vtx.color());
    }

    V3Stats::جوڑو_Stat("Scheduling, Cycles, cyclic variables", nCyclicVars);
    V3Stats::جوڑو_Stat("Scheduling, Cycles, cyclic logic blocks", nCyclicVtxs - nCyclicVars);
    V3Stats::جوڑو_Stat("Scheduling, Cycles, unique SCCs", sccs.ناپ());
    V3Stats::جوڑو_Stat("Scheduling, Cycles, cut variables", نتیجہ.ناپ());

    أرجع نتیجہ;
}

فراغ reset_کنارا_Weights(ثابت std::سمتیہ<Sched_Acyclic_Var_راس*>& cutVertices) {
    لكل (Sched_Acyclic_Var_راس* ثابت vvtxp : cutVertices) {
        لكل (V3_گراف_کنارا& e : vvtxp->وچ_کنارے()) e.بھار_2(1);
        لكل (V3_گراف_کنارا& e : vvtxp->باہرلا_کنارے()) e.بھار_2(1);
    }
}

// A VarVertex together with its fanout
ورتوں Candidate = std::pair<Sched_Acyclic_Var_راس*, بنانشانی>;

// Gather all splitting candidates that are in the same SCC as the given vertex
فراغ gatherSCCCandidates(V3_گراف_راس* vtxp, std::سمتیہ<Candidate>& candidates) {
    إذا (vtxp->user()) أرجع;  // Already done
    vtxp->user(سچ);

    إذا (Sched_Acyclic_Var_راس* ثابت vvtxp = vtxp->cast<Sched_Acyclic_Var_راس>()) {
        AstVar* ثابت varp = vvtxp->varp();
        ثابت string name = varp->prettyName();
        إذا (!varp->user3_سیٹ_Once()  // Only consider each AstVar once
            && varp->چوڑائی() != 1  // Ignore 1-bit signals (they cannot be split further)
            && name.لبھو("__Vdly") == string::npos  // Ignore internal signals
            && name.لبھو("__Vcell") == string::npos) {
            // Also compute the fanout of this vertex
            ثابت بنانشانی fanout = vtxp->باہرلا_کنارے().ناپ();
            candidates.emplace_پٹھ(vvtxp, fanout);
        }
    }

    // Iterate through all the vertices within the same strongly connected component (same color)
    لكل (V3_گراف_کنارا& کنارا : vtxp->باہرلا_کنارے()) {
        V3_گراف_راس* ثابت چوٹی = کنارا.چوٹی();
        إذا (چوٹی->color() == vtxp->color()) gatherSCCCandidates(چوٹی, candidates);
    }
    لكل (V3_گراف_کنارا& کنارا : vtxp->وچ_کنارے()) {
        V3_گراف_راس* ثابت fromp = کنارا.fromp();
        إذا (fromp->color() == vtxp->color()) gatherSCCCandidates(fromp, candidates);
    }
}

// Find all variables in a loop (SCC) that are candidates for splitting to break loops.
std::string reportLoopVars(File_سطر* /*warnFl*/, گراف_2* graphp, Sched_Acyclic_Var_راس* vvtxp) {
    std::ostringstream ss;
    // Vector of variables in UNOPTFLAT loop that are candidates for splitting.
    std::سمتیہ<Candidate> candidates;
    {
        // AstNode::user3 is used to mark if we have done a particular variable.
        // V3GraphVertex::user is used to mark if we have seen this vertex before.
        ثابت VNUser3_وچ_Use user3_وچ_Use;
        graphp->userClearVertices();
        gatherSCCCandidates(vvtxp, candidates);
        graphp->userClearVertices();
    }

    // Possible we only have candidates the user cannot do anything about, so don't bother them.
    إذا (candidates.خالی_تھاواں()) أرجع "";

    // There may be a very large number of candidates, so only report up to 10 of the "most
    // important" signals.
    بنانشانی splittable = 0;
    ثابت خودکار reportFirst10
        = [&](std::function<منطقي(ثابت Candidate&, ثابت Candidate&)> less) -> string {
        std::stable_ترتیب_دیو(candidates.begin(), candidates.سرا(), less);
        std::ostringstream ss2;
        لكل (ناپ_t i = 0; i < 10; i++) {
            إذا (i == candidates.ناپ()) اكسر;
            ثابت Candidate& candidate = candidates[i];
            AstVar* ثابت varp = candidate.پہلا->varp();

            ss2 << V3Error::warn_ہور() << "    " << varp->fileline() << ' ' << varp->prettyName()
                << ", width " << std::dec << varp->چوڑائی() << ", circular fanout "
                << candidate.دوجا;
            إذا (!varp->cannot_ونڈو_قسم_Reason()) {
                ss2 << ", can split_var";
                ++splittable;
            }
            ss2 << '\n';
        }
        أرجع ss2.str();
    };

    // Widest variables
    ss << V3Error::warn_ہور() << "... Widest variables candidate to splitting:\n"
       << reportFirst10([](ثابت Candidate& a, ثابت Candidate& b) {
              أرجع a.پہلا->varp()->چوڑائی() > b.پہلا->varp()->چوڑائی();
          });

    // Highest fanout
    ss << V3Error::warn_ہور() << "... Candidates with the highest fanout:\n"
       << reportFirst10([](ثابت Candidate& a, ثابت Candidate& b) {  //
              أرجع a.دوجا > b.دوجا;
          });

    إذا (splittable) {
        ss << V3Error::warn_ہور()
           << "... Suggest add /*verilator split_var*/ to appropriate variables above.\n";
    }
    V3Stats::جوڑو_Stat("Scheduling, split_var, candidates", splittable);
    أرجع ss.str();
}

فراغ report_چکراں(گراف_2* graphp, ثابت std::سمتیہ<Sched_Acyclic_Var_راس*>& cutVertices) {
    لكل (Sched_Acyclic_Var_راس* vvtxp : cutVertices) {
        AstVarScope* ثابت vscp = vvtxp->vscp();
        File_سطر* ثابت flp = vscp->fileline();

        // First v3warn not inside warnIsOff so we can see the suppressions with --debug
        إذا (flp->warnIsOff(V3_Error_کوڈ::UNOPTFLAT)) {
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
                             << graphp->reportLoops(&V3_گراف_کنارا::followAlwaysTrue, vvtxp)
                             // Report candidate variables for splitting
                             << (v3Global.opt.reportUnoptflat()
                                     ? reportLoopVars(vscp->fileline(), graphp, vvtxp)
                                     : ""));
            // Complain just once
            flp->modifyWarnOff(V3_Error_کوڈ::UNOPTFLAT, سچ);
            // Create a subgraph for the UNOPTFLAT loop
            إذا (v3Global.opt.reportUnoptflat()) {
                V3_گراف loop_گراف;
                graphp->subtreeLoops(&V3_گراف_کنارا::followAlwaysTrue, vvtxp, &loop_گراف);
                loop_گراف.dumpDotFilePrefixedAlways("unoptflat");
            }
        }
    }
}

فراغ dumpSccs(V3_گراف* graphp) {
    // Map from SCC color to vertices in that SCC
    std::map<uint32_t, std::سمتیہ<V3_گراف_راس*>> scc2Vtxps;

    // Gather all vertices in each SCC
    لكل (V3_گراف_راس& vtx : graphp->vertices()) {
        إذا (!vtx.color()) استمر;
        scc2Vtxps[vtx.color()].ڈالو_پٹھ(&vtx);
    }

    // Dump Verilog for each SCC into separate files
    لكل (ثابت خودکار& pair : scc2Vtxps) {
        ثابت uint32_t color = pair.پہلا;
        ثابت std::سمتیہ<V3_گراف_راس*>& vtxps = pair.دوجا;

        // Open dump file
        ثابت std::string fname
            = v3Global.debugFilename("sched_scc_" + std::منزل_string(color) + ".v");
        ثابت std::unique_ptr<std::ofstream> ofp{V3File::new_ofstream(fname)};
        إذا (ofp->fail()) v3fatal("Can't write file: " << fname);

        // Write header
        *ofp << "// SCC " << color << ", size: " << vtxps.ناپ() << "\n\n";

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
            V3EmitV::debug_Verilog_For_درخت(vscp->varp(), *ofp);
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
            V3EmitV::debug_Verilog_For_درخت(lvtxp->logicp(), *ofp);
            *ofp << "\n";
        }
    }
}

LogicByScope fixCuts(AstNetlist* netlistp,
                     ثابت std::سمتیہ<Sched_Acyclic_Var_راس*>& cutVertices) {
    // For all logic that reads a cut vertex, build a map from logic -> list of cut AstVarScope
    // they read. Also build a vector of the involved logic for deterministic results.
    std::unordered_map<Sched_Acyclic_Logic_راس*, std::سمتیہ<AstVarScope*>> lvtx2Cuts;
    std::سمتیہ<Sched_Acyclic_Logic_راس*> lvtxps;
    {
        ثابت VNUser1_وچ_Use user1_وچ_Use;  // bool: already added to 'lvtxps'
        لكل (Sched_Acyclic_Var_راس* ثابت vvtxp : cutVertices) {
            لكل (V3_گراف_کنارا& کنارا : vvtxp->باہرلا_کنارے()) {
                Sched_Acyclic_Logic_راس* ثابت lvtxp
                    = static_cast<Sched_Acyclic_Logic_راس*>(کنارا.چوٹی());
                إذا (!lvtxp->logicp()->user1_سیٹ_Once()) lvtxps.ڈالو_پٹھ(lvtxp);
                lvtx2Cuts[lvtxp].ڈالو_پٹھ(vvtxp->vscp());
            }
        }
    }

    // Make the logic reading cut vertices use a hybrid sensitivity (combinational, but with some
    // explicit additional triggers on the cut variables)
    LogicByScope نتیجہ;
    Sen_درخت_Finder finder{netlistp};
    لكل (Sched_Acyclic_Logic_راس* ثابت lvtxp : lvtxps) {
        Ast_گرہ* ثابت logicp = lvtxp->logicp();
        logicp->unlink_Fr_پٹھ();
        File_سطر* ثابت flp = logicp->fileline();
        // Build the hybrid sensitivity list
        Ast_Sen_شے* senItemsp = خالی_سوچک;
        لكل (AstVarScope* ثابت vscp : lvtx2Cuts[lvtxp]) {
            AstVarRef* ثابت refp = جديد AstVarRef{flp, vscp, VAccess::READ};
            Ast_Sen_شے* ثابت nextp = جديد Ast_Sen_شے{flp, VEdgeType::ET_HYBRID, refp};
            senItemsp = Ast_گرہ::جوڑو_اگلا(senItemsp, nextp);
        }
        Ast_Sen_درخت* ثابت sen_درخت = جديد Ast_Sen_درخت{flp, senItemsp};
        // Add logic to result with new sensitivity
        نتیجہ.جوڑو(lvtxp->scopep(), finder.get_Sen_درخت(sen_درخت), logicp);
        // SenTreeFinder::getSenTree clones, so clean up
        VL_DO_DANGLING(sen_درخت->delete_درخت(), sen_درخت);
    }
    أرجع نتیجہ;
}

}  // namespace

LogicByScope break_چکراں(AstNetlist* netlistp, ثابت LogicByScope& combinationalLogic) {
    // Build the dataflow (dependency) graph
    ثابت std::unique_ptr<گراف_2> graphp = build_گراف(combinationalLogic);

    // Remove nodes that don't form part of a cycle
    removeNonCyclic(graphp.get());

    // Nothing to do if no cycles, yay!
    إذا (graphp->خالی_تھاواں()) أرجع LogicByScope{};

    // Dump for debug
    إذا (dump_گراف_Level() >= 6) graphp->dumpDotFilePrefixed("sched-comb-cycles");

    // Make graph acyclic by cutting some edges. Note: This also colors strongly connected
    // components which reportCycles uses to print each SCCs separately.
    // TODO: A more optimal algorithm that cuts by removing/marking VarVertex vertices is possible
    //       Search for "Feedback vertex set" (current algorithm is "Feedback arc set")
    graphp->acyclic(&V3_گراف_کنارا::followAlwaysTrue);

    // Find all cut vertices
    ثابت std::سمتیہ<Sched_Acyclic_Var_راس*> cutVertices = لبھو_Cut_Vertices(graphp.get());

    // Reset edge weights for reporting
    reset_کنارا_Weights(cutVertices);

    // Report warnings/diagnostics
    report_چکراں(graphp.get(), cutVertices);

    // Debug dump
    إذا (dumpLevel() >= 6) dumpSccs(graphp.get());

    // Fix cuts by converting dependent logic to use hybrid sensitivities
    أرجع fixCuts(netlistp, cutVertices);
}

}  // namespace V3Sched
