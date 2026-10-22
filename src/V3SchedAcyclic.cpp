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

#شامل "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#شامل "V3EmitV.h"
#شامل "V3File.h"
#شامل "V3Graph.h"
#شامل "V3Sched.h"
#شامل "V3SenTree.h"
#شامل "V3Stats.h"

#شامل <map>
#شامل <unordered_map>
#شامل <unordered_set>
#شامل <utility>
#شامل <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

نامخلا V3Sched {

نامخلا {

// ##############################################################################
//  Data structures (graph types)

جماعت Sched_Acyclic_Logic_راس final : عوامی V3_گراف_راس {
    VL_RTTI_IMPL(Sched_Acyclic_Logic_راس, V3_گراف_راس)
    Ast_گرہ* ثابت m_logicp;  // The logic node this vertex represents
    AstScope* ثابت m_scopep;  // The enclosing AstScope of the logic node

عوامی:
    Sched_Acyclic_Logic_راس(V3_گراف* graphp, Ast_گرہ* logicp, AstScope* scopep)
        : V3_گراف_راس{graphp}
        , m_logicp{logicp}
        , m_scopep{scopep} {}
    V3_گراف_راس* clone(V3_گراف* graphp) ثابت override {
        واپس نیا Sched_Acyclic_Logic_راس{graphp, logicp(), scopep()};
    }

    Ast_گرہ* logicp() ثابت { واپس m_logicp; }
    AstScope* scopep() ثابت { واپس m_scopep; }

    // LCOV_EXCL_START // Debug code
    string name() ثابت override VL_MT_STABLE { واپس m_logicp->fileline()->ascii(); };
    string dotShape() ثابت override { واپس "rectangle2"; }
    // LCOV_EXCL_STOP
};

جماعت Sched_Acyclic_Var_راس final : عوامی V3_گراف_راس {
    VL_RTTI_IMPL(Sched_Acyclic_Var_راس, V3_گراف_راس)
    AstVarScope* ثابت m_vscp;  // The AstVarScope this vertex represents

عوامی:
    Sched_Acyclic_Var_راس(V3_گراف* graphp, AstVarScope* vscp)
        : V3_گراف_راس{graphp}
        , m_vscp{vscp} {}
    AstVarScope* vscp() ثابت { واپس m_vscp; }
    AstVar* varp() ثابت { واپس m_vscp->varp(); }
    V3_گراف_راس* clone(V3_گراف* graphp) ثابت override {
        واپس نیا Sched_Acyclic_Var_راس{graphp, vscp()};
    }

    // LCOV_EXCL_START // Debug code
    string name() ثابت override VL_MT_STABLE { واپس m_vscp->name(); }
    string dotShape() ثابت override { واپس "ellipse"; }
    string dotColor() ثابت override { واپس "blue"; }
    // LCOV_EXCL_STOP
};

جماعت گراف_2 final : عوامی V3_گراف {
    string loops_راس_Cb(V3_گراف_راس* vtxp) override {
        اگر (Sched_Acyclic_Logic_راس* ثابت lvtxp = vtxp->cast<Sched_Acyclic_Logic_راس>()) {
            Ast_گرہ* ثابت logicp = lvtxp->logicp();
            std::string logicName = logicp->prettyTypeName();
            اگر (ثابت AstAlways* ثابت alwaysp = VN_CAST(logicp, Always)) {
                اگر (alwaysp->keyword() == VAlwaysKwd::CONT_تفویض) {
                    logicName = "ASSIGNW";  // Keep using historiacl name until we have better
                }
            }
            واپس logicp->fileline()->warnOther() + "     Example path: " + logicName + "\n";
        } ورنہ {
            Sched_Acyclic_Var_راس* ثابت vvtxp = vtxp->as<Sched_Acyclic_Var_راس>();
            AstVarScope* ثابت vscp = vvtxp->vscp();
            واپس vscp->fileline()->warnOther() + "     Example path: " + vscp->prettyName()
                   + "\n";
        }
    }
};

//##############################################################################
// Algorithm implementation

std::unique_ptr<گراف_2> build_گراف(ثابت LogicByScope& lbs) {
    std::unique_ptr<گراف_2> graphp{نیا گراف_2};

    // AstVarScope::user1() -> VarVertx
    ثابت VNUser1_اندر_Use user1_اندر_Use;
    ثابت خودکار get_Var_راس = [&](AstVarScope* vscp) {
        اگر (!vscp->user1p()) vscp->user1p(نیا Sched_Acyclic_Var_راس{graphp.get(), vscp});
        واپس vscp->user1u().مقصد<Sched_Acyclic_Var_راس*>();
    };

    ثابت خودکار جوڑو_کنارہ = [&](V3_گراف_راس* fromp, V3_گراف_راس* چوٹی, صحیح وزن_2, منطقی cuttable) {
        نیا V3_گراف_کنارہ{graphp.get(), fromp, چوٹی, وزن_2, cuttable};
    };

    برائے (ثابت خودکار& اندراج_2 : lbs) {
        AstScope* ثابت scopep = اندراج_2.m_scopep;
        Ast_فعال* ثابت activep = اندراج_2.m_activep;
        UASSERT_OBJ(activep->hasCombo(), activep, "Not combinational logic");
        برائے (Ast_گرہ* nodep = activep->stmtsp(); nodep; nodep = nodep->nextp()) {
            // Can safely ignore Postponed as we generate them all
            اگر (VN_IS(nodep, AlwaysPostponed)) جاری;

            Sched_Acyclic_Logic_راس* ثابت lvtxp
                = نیا Sched_Acyclic_Logic_راس{graphp.get(), nodep, scopep};
            ثابت VNUser2_اندر_Use user2_اندر_Use;
            ثابت VNUser3_اندر_Use user3_اندر_Use;

            V3Sched::util::Var_Scope_سیٹ force_Read_کنارہ_Ignores;
            V3Sched::util::collect_Force_Read_کنارہ_Ignores(nodep, force_Read_کنارہ_Ignores);

            nodep->foreach([&](AstVarRef* refp) {
                AstVarScope* ثابت vscp = refp->varScopep();
                Sched_Acyclic_Var_راس* ثابت vvtxp = get_Var_راس(vscp);
                // We want to cut the narrowest signals
                ثابت صحیح وزن_2 = vscp->چوڑائی() / 8 + 1;
                // If written, add logic -> var edge
                اگر (refp->access().isWriteOrRW() && !refp->varp()->ignoreSchedWrite()
                    && !vscp->user2_سیٹ_Once())
                    جوڑو_کنارہ(lvtxp, vvtxp, وزن_2, سچ);
                // If read, add var -> logic edge
                // Note: Use same heuristic as ordering does to ignore written variables
                // TODO: Use live variable analysis.
                اگر (refp->access().isReadOrRW() && !vscp->user3_سیٹ_Once() && !vscp->user2()
                    && !force_Read_کنارہ_Ignores.گنتی(vscp))
                    جوڑو_کنارہ(vvtxp, lvtxp, وزن_2, سچ);
            });
        }
    }

    واپس graphp;
}

خالی removeNonCyclic(گراف_2* graphp) {
    // Work queue
    std::سمتیہ<V3_گراف_راس*> قطار;

    ثابت خودکار قطار_میں_ڈالو = [&](V3_گراف_راس* vtxp) {
        اگر (vtxp->user()) واپس;  // Already in queue
        vtxp->user(1);
        قطار.ڈالو_عقب(vtxp);
    };

    // Start with vertices with no inputs or outputs
    برائے (V3_گراف_راس& vtx : graphp->vertices()) {
        اگر (vtx.اندر_خالی_خانے() || vtx.برآمد_خالی_خانے()) قطار_میں_ڈالو(&vtx);
    }

    // Iterate while we still have candidates
    جب (!قطار.خالی_خانے()) {
        // Pop next candidate
        V3_گراف_راس* ثابت vtxp = قطار.عقب();
        قطار.نکالو_عقب();
        vtxp->user(0);  // No longer in queue

        اگر (vtxp->اندر_خالی_خانے()) {
            // Enqueue children for consideration, remove out edges, and delete this vertex
            برائے (V3_گراف_کنارہ* ثابت edgep : vtxp->برآمد_کنارے().unlinkable()) {
                قطار_میں_ڈالو(edgep->چوٹی());
                VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
            }
            VL_DO_DANGLING(vtxp->unlinkDelete(graphp), vtxp);
        } ورنہ اگر (vtxp->برآمد_خالی_خانے()) {
            // Enqueue parents for consideration, remove in edges, and delete this vertex
            برائے (V3_گراف_کنارہ* ثابت edgep : vtxp->اندر_کنارے().unlinkable()) {
                قطار_میں_ڈالو(edgep->fromp());
                VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
            }
            VL_DO_DANGLING(vtxp->unlinkDelete(graphp), vtxp);
        }
    }
}

// Has this VarVertex been cut? (any edges in or out has been cut)
منطقی isCut(ثابت Sched_Acyclic_Var_راس* vtxp) {
    برائے (ثابت V3_گراف_کنارہ& کنارہ : vtxp->اندر_کنارے()) {
        اگر (کنارہ.وزن_2() == 0) واپس سچ;
    }
    برائے (ثابت V3_گراف_کنارہ& کنارہ : vtxp->برآمد_کنارے()) {
        اگر (کنارہ.وزن_2() == 0) واپس سچ;
    }
    واپس جھوٹ;
}

std::سمتیہ<Sched_Acyclic_Var_راس*> ڈھونڈو_Cut_Vertices(گراف_2* graphp) {
    // List of cut vertices being computed here
    std::سمتیہ<Sched_Acyclic_Var_راس*> نتیجہ;
    ثابت VNUser1_اندر_Use user1_اندر_Use;  // bool: already added to result

    // Statistics
    سائز_t nCyclicVtxs = 0;  // Number of vertices that are part of an SCC (cycle)
    سائز_t nCyclicVars = 0;  // Number of variables that are part of an SCC (cycle)
    std::unordered_سیٹ<uint32_t> sccs;  // Unique SCC colors

    برائے (V3_گراف_راس& vtx : graphp->vertices()) {
        اگر (!vtx.color()) جاری;  // Not part of an SCC (cycle), can ignore
        ++nCyclicVtxs;
        اگر (Sched_Acyclic_Var_راس* ثابت vvtxp = vtx.cast<Sched_Acyclic_Var_راس>()) {
            ++nCyclicVars;
            اگر (!vvtxp->vscp()->user1_سیٹ_Once() && isCut(vvtxp)) نتیجہ.ڈالو_عقب(vvtxp);
        }
        // Don't bother counting if not dumping statistics
        اگر (v3Global.opt.stats()) sccs.داخل_کرو(vtx.color());
    }

    V3Stats::جوڑو_Stat("Scheduling, Cycles, cyclic variables", nCyclicVars);
    V3Stats::جوڑو_Stat("Scheduling, Cycles, cyclic logic blocks", nCyclicVtxs - nCyclicVars);
    V3Stats::جوڑو_Stat("Scheduling, Cycles, unique SCCs", sccs.سائز());
    V3Stats::جوڑو_Stat("Scheduling, Cycles, cut variables", نتیجہ.سائز());

    واپس نتیجہ;
}

خالی reset_کنارہ_Weights(ثابت std::سمتیہ<Sched_Acyclic_Var_راس*>& cutVertices) {
    برائے (Sched_Acyclic_Var_راس* ثابت vvtxp : cutVertices) {
        برائے (V3_گراف_کنارہ& e : vvtxp->اندر_کنارے()) e.وزن_2(1);
        برائے (V3_گراف_کنارہ& e : vvtxp->برآمد_کنارے()) e.وزن_2(1);
    }
}

// A VarVertex together with its fanout
استعمال Candidate = std::pair<Sched_Acyclic_Var_راس*, غیرعلامتی>;

// Gather all splitting candidates that are in the same SCC as the given vertex
خالی gatherSCCCandidates(V3_گراف_راس* vtxp, std::سمتیہ<Candidate>& candidates) {
    اگر (vtxp->user()) واپس;  // Already done
    vtxp->user(سچ);

    اگر (Sched_Acyclic_Var_راس* ثابت vvtxp = vtxp->cast<Sched_Acyclic_Var_راس>()) {
        AstVar* ثابت varp = vvtxp->varp();
        ثابت string name = varp->prettyName();
        اگر (!varp->user3_سیٹ_Once()  // Only consider each AstVar once
            && varp->چوڑائی() != 1  // Ignore 1-bit signals (they cannot be split further)
            && name.ڈھونڈو("__Vdly") == string::npos  // Ignore internal signals
            && name.ڈھونڈو("__Vcell") == string::npos) {
            // Also compute the fanout of this vertex
            ثابت غیرعلامتی fanout = vtxp->برآمد_کنارے().سائز();
            candidates.emplace_عقب(vvtxp, fanout);
        }
    }

    // Iterate through all the vertices within the same strongly connected component (same color)
    برائے (V3_گراف_کنارہ& کنارہ : vtxp->برآمد_کنارے()) {
        V3_گراف_راس* ثابت چوٹی = کنارہ.چوٹی();
        اگر (چوٹی->color() == vtxp->color()) gatherSCCCandidates(چوٹی, candidates);
    }
    برائے (V3_گراف_کنارہ& کنارہ : vtxp->اندر_کنارے()) {
        V3_گراف_راس* ثابت fromp = کنارہ.fromp();
        اگر (fromp->color() == vtxp->color()) gatherSCCCandidates(fromp, candidates);
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
        ثابت VNUser3_اندر_Use user3_اندر_Use;
        graphp->userClearVertices();
        gatherSCCCandidates(vvtxp, candidates);
        graphp->userClearVertices();
    }

    // Possible we only have candidates the user cannot do anything about, so don't bother them.
    اگر (candidates.خالی_خانے()) واپس "";

    // There may be a very large number of candidates, so only report up to 10 of the "most
    // important" signals.
    غیرعلامتی splittable = 0;
    ثابت خودکار reportFirst10
        = [&](std::function<منطقی(ثابت Candidate&, ثابت Candidate&)> less) -> string {
        std::stable_ترتیب_دو(candidates.begin(), candidates.انتہا(), less);
        std::ostringstream ss2;
        برائے (سائز_t i = 0; i < 10; i++) {
            اگر (i == candidates.سائز()) توڑو;
            ثابت Candidate& candidate = candidates[i];
            AstVar* ثابت varp = candidate.پہلا->varp();

            ss2 << V3Error::warn_مزید() << "    " << varp->fileline() << ' ' << varp->prettyName()
                << ", width " << std::dec << varp->چوڑائی() << ", circular fanout "
                << candidate.دوسرا;
            اگر (!varp->cannot_بانٹو_قسم_Reason()) {
                ss2 << ", can split_var";
                ++splittable;
            }
            ss2 << '\n';
        }
        واپس ss2.str();
    };

    // Widest variables
    ss << V3Error::warn_مزید() << "... Widest variables candidate to splitting:\n"
       << reportFirst10([](ثابت Candidate& a, ثابت Candidate& b) {
              واپس a.پہلا->varp()->چوڑائی() > b.پہلا->varp()->چوڑائی();
          });

    // Highest fanout
    ss << V3Error::warn_مزید() << "... Candidates with the highest fanout:\n"
       << reportFirst10([](ثابت Candidate& a, ثابت Candidate& b) {  //
              واپس a.دوسرا > b.دوسرا;
          });

    اگر (splittable) {
        ss << V3Error::warn_مزید()
           << "... Suggest add /*verilator split_var*/ to appropriate variables above.\n";
    }
    V3Stats::جوڑو_Stat("Scheduling, split_var, candidates", splittable);
    واپس ss.str();
}

خالی report_ادوار(گراف_2* graphp, ثابت std::سمتیہ<Sched_Acyclic_Var_راس*>& cutVertices) {
    برائے (Sched_Acyclic_Var_راس* vvtxp : cutVertices) {
        AstVarScope* ثابت vscp = vvtxp->vscp();
        File_سطر* ثابت flp = vscp->fileline();

        // First v3warn not inside warnIsOff so we can see the suppressions with --debug
        اگر (flp->warnIsOff(V3_Error_رمز::UNOPTFLAT)) {
            // First v3warn not inside warnIsOff so we can see the suppressions with --debug
            vscp->v3warn(UNOPTFLAT, "Signal unoptimizable: Circular combinational logic: "
                                        << vscp->prettyNameQ());
        } ورنہ {
            vscp->v3warn(UNOPTFLAT,
                         "Signal unoptimizable: Circular combinational logic: "
                             << vscp->prettyNameQ() << '\n'
                             << vscp->warnContextPrimary()
                             << V3Error::warnAdditionalInfo()
                             // Calls Graph::loopsVertexCb
                             << graphp->reportLoops(&V3_گراف_کنارہ::followAlwaysTrue, vvtxp)
                             // Report candidate variables for splitting
                             << (v3Global.opt.reportUnoptflat()
                                     ? reportLoopVars(vscp->fileline(), graphp, vvtxp)
                                     : ""));
            // Complain just once
            flp->modifyWarnOff(V3_Error_رمز::UNOPTFLAT, سچ);
            // Create a subgraph for the UNOPTFLAT loop
            اگر (v3Global.opt.reportUnoptflat()) {
                V3_گراف loop_گراف;
                graphp->subtreeLoops(&V3_گراف_کنارہ::followAlwaysTrue, vvtxp, &loop_گراف);
                loop_گراف.dumpDotFilePrefixedAlways("unoptflat");
            }
        }
    }
}

خالی dumpSccs(V3_گراف* graphp) {
    // Map from SCC color to vertices in that SCC
    std::map<uint32_t, std::سمتیہ<V3_گراف_راس*>> scc2Vtxps;

    // Gather all vertices in each SCC
    برائے (V3_گراف_راس& vtx : graphp->vertices()) {
        اگر (!vtx.color()) جاری;
        scc2Vtxps[vtx.color()].ڈالو_عقب(&vtx);
    }

    // Dump Verilog for each SCC into separate files
    برائے (ثابت خودکار& pair : scc2Vtxps) {
        ثابت uint32_t color = pair.پہلا;
        ثابت std::سمتیہ<V3_گراف_راس*>& vtxps = pair.دوسرا;

        // Open dump file
        ثابت std::string fname
            = v3Global.debugFilename("sched_scc_" + std::مقصد_string(color) + ".v");
        ثابت std::unique_ptr<std::ofstream> ofp{V3File::new_ofstream(fname)};
        اگر (ofp->fail()) v3fatal("Can't write file: " << fname);

        // Write header
        *ofp << "// SCC " << color << ", size: " << vtxps.سائز() << "\n\n";

        // Dump variables
        *ofp << "//////////////////////////////////////////////////////////////////////\n";
        *ofp << "// Variables\n";
        *ofp << "//////////////////////////////////////////////////////////////////////\n";
        *ofp << "\n";
        برائے (V3_گراف_راس* vtxp : vtxps) {
            ثابت Sched_Acyclic_Var_راس* ثابت vvtxp = vtxp->cast<Sched_Acyclic_Var_راس>();
            اگر (!vvtxp) جاری;
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
        برائے (V3_گراف_راس* vtxp : vtxps) {
            ثابت Sched_Acyclic_Logic_راس* ثابت lvtxp = vtxp->cast<Sched_Acyclic_Logic_راس>();
            اگر (!lvtxp) جاری;
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
        ثابت VNUser1_اندر_Use user1_اندر_Use;  // bool: already added to 'lvtxps'
        برائے (Sched_Acyclic_Var_راس* ثابت vvtxp : cutVertices) {
            برائے (V3_گراف_کنارہ& کنارہ : vvtxp->برآمد_کنارے()) {
                Sched_Acyclic_Logic_راس* ثابت lvtxp
                    = static_cast<Sched_Acyclic_Logic_راس*>(کنارہ.چوٹی());
                اگر (!lvtxp->logicp()->user1_سیٹ_Once()) lvtxps.ڈالو_عقب(lvtxp);
                lvtx2Cuts[lvtxp].ڈالو_عقب(vvtxp->vscp());
            }
        }
    }

    // Make the logic reading cut vertices use a hybrid sensitivity (combinational, but with some
    // explicit additional triggers on the cut variables)
    LogicByScope نتیجہ;
    Sen_درخت_Finder finder{netlistp};
    برائے (Sched_Acyclic_Logic_راس* ثابت lvtxp : lvtxps) {
        Ast_گرہ* ثابت logicp = lvtxp->logicp();
        logicp->unlink_Fr_عقب();
        File_سطر* ثابت flp = logicp->fileline();
        // Build the hybrid sensitivity list
        Ast_Sen_شے* senItemsp = خالی_اشاریہ;
        برائے (AstVarScope* ثابت vscp : lvtx2Cuts[lvtxp]) {
            AstVarRef* ثابت refp = نیا AstVarRef{flp, vscp, VAccess::READ};
            Ast_Sen_شے* ثابت nextp = نیا Ast_Sen_شے{flp, VEdgeType::ET_HYBRID, refp};
            senItemsp = Ast_گرہ::جوڑو_اگلا(senItemsp, nextp);
        }
        Ast_Sen_درخت* ثابت sen_درخت = نیا Ast_Sen_درخت{flp, senItemsp};
        // Add logic to result with new sensitivity
        نتیجہ.جوڑو(lvtxp->scopep(), finder.get_Sen_درخت(sen_درخت), logicp);
        // SenTreeFinder::getSenTree clones, so clean up
        VL_DO_DANGLING(sen_درخت->delete_درخت(), sen_درخت);
    }
    واپس نتیجہ;
}

}  // namespace

LogicByScope break_ادوار(AstNetlist* netlistp, ثابت LogicByScope& combinationalLogic) {
    // Build the dataflow (dependency) graph
    ثابت std::unique_ptr<گراف_2> graphp = build_گراف(combinationalLogic);

    // Remove nodes that don't form part of a cycle
    removeNonCyclic(graphp.get());

    // Nothing to do if no cycles, yay!
    اگر (graphp->خالی_خانے()) واپس LogicByScope{};

    // Dump for debug
    اگر (dump_گراف_Level() >= 6) graphp->dumpDotFilePrefixed("sched-comb-cycles");

    // Make graph acyclic by cutting some edges. Note: This also colors strongly connected
    // components which reportCycles uses to print each SCCs separately.
    // TODO: A more optimal algorithm that cuts by removing/marking VarVertex vertices is possible
    //       Search for "Feedback vertex set" (current algorithm is "Feedback arc set")
    graphp->acyclic(&V3_گراف_کنارہ::followAlwaysTrue);

    // Find all cut vertices
    ثابت std::سمتیہ<Sched_Acyclic_Var_راس*> cutVertices = ڈھونڈو_Cut_Vertices(graphp.get());

    // Reset edge weights for reporting
    reset_کنارہ_Weights(cutVertices);

    // Report warnings/diagnostics
    report_ادوار(graphp.get(), cutVertices);

    // Debug dump
    اگر (dumpLevel() >= 6) dumpSccs(graphp.get());

    // Fix cuts by converting dependent logic to use hybrid sensitivities
    واپس fixCuts(netlistp, cutVertices);
}

}  // namespace V3Sched
