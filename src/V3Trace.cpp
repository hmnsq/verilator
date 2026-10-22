// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Waves tracing
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
// V3Trace's Transformations:
//
//  Examine whole design and build a graph describing which function call
//  may result in a write to a traced variable. This is done in 2 passes:
//
//  Pass 1:
//      Add vertices for TraceDecl, CFunc, CCall and VarRef nodes, add
//      edges from CCall -> CFunc, VarRef -> TraceDecl, also add edges
//      for public entry points to CFuncs (these are like a spontaneous
//      call)
//
//  Pass 2:
//      Add edges from CFunc -> VarRef being written
//
//  Finally:
//      Process graph to determine when traced variables can change, allocate
//      activity flags, insert nodes to set activity flags, allocate signal
//      numbers (codes), and construct the const, full and incremental trace
//      functions, together with all other trace support functions.
//
//*************************************************************************

#অন্তর্ভাব "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#অন্তর্ভাব "V3Trace.h"

#অন্তর্ভাব "V3Ast.h"
#অন্তর্ভাব "V3DupFinder.h"
#অন্তর্ভাব "V3EmitCBase.h"
#অন্তর্ভাব "V3Graph.h"
#অন্তর্ভাব "V3Stats.h"
#অন্তর্ভাব "V3UniqueNames.h"

#অন্তর্ভাব <limits>
#অন্তর্ভাব <map>
#অন্তর্ভাব <set>
#অন্তর্ভাব <unordered_map>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Graph vertexes

বর্গ TraceActivityVertex final : সার্বজনিক V3GraphVertex {
    VL_RTTI_IMPL(TraceActivityVertex, V3GraphVertex)
    Ast_গ্রন্থি* ধ্রুব m_insertp;
    int32_t m_activityCode;
    তার্কিক m_slow;  // If always slow, we can use the same code
সার্বজনিক:
    গণনা { ACTIVITY_NEVER = ((1UL << 31) - 1) };
    গণনা { ACTIVITY_ALWAYS = ((1UL << 31) - 2) };
    গণনা { ACTIVITY_SLOW = 0 };
    TraceActivityVertex(V3Graph* graphp, Ast_গ্রন্থি* nodep, তার্কিক slow)
        : V3GraphVertex{graphp}
        , m_insertp{nodep} {
        m_activityCode = 0;
        m_slow = slow;
    }
    TraceActivityVertex(V3Graph* graphp, int32_t code)
        : V3GraphVertex{graphp}
        , m_insertp{nullptr} {
        m_activityCode = code;
        m_slow = অসত্য;
    }
    ~TraceActivityVertex() override = স্বভাব;
    // ACCESSORS
    Ast_গ্রন্থি* insertp() ধ্রুব {
        UASSERT(m_insertp, "Null insertp; probably called on a special always/slow");
        প্রত্যাগম m_insertp;
    }
    string name() ধ্রুব override {
        যদি (activityAlways()) {
            প্রত্যাগম "*ALWAYS*";
        } অন্যথা {
            প্রত্যাগম std::string{slow() ? "*SLOW* " : ""} + insertp()->name();
        }
    }
    string dotColor() ধ্রুব override { প্রত্যাগম slow() ? "yellowGreen" : "green"; }
    int32_t activityCode() ধ্রুব { প্রত্যাগম m_activityCode; }
    তার্কিক activityAlways() ধ্রুব { প্রত্যাগম activityCode() == ACTIVITY_ALWAYS; }
    তার্কিক activitySlow() ধ্রুব { প্রত্যাগম activityCode() == ACTIVITY_SLOW; }
    শূন্য activityCode(int32_t code) { m_activityCode = code; }
    তার্কিক slow() ধ্রুব { প্রত্যাগম m_slow; }
    শূন্য slow(তার্কিক flag) {
        যদি (!flag) m_slow = অসত্য;
    }
};

বর্গ TraceCFuncVertex final : সার্বজনিক V3GraphVertex {
    VL_RTTI_IMPL(TraceCFuncVertex, V3GraphVertex)
    AstCFunc* ধ্রুব m_nodep;

সার্বজনিক:
    TraceCFuncVertex(V3Graph* graphp, AstCFunc* nodep)
        : V3GraphVertex{graphp}
        , m_nodep{nodep} {}
    ~TraceCFuncVertex() override = স্বভাব;
    // ACCESSORS
    AstCFunc* nodep() ধ্রুব { প্রত্যাগম m_nodep; }
    string name() ধ্রুব override { প্রত্যাগম nodep()->name(); }
    string dotColor() ধ্রুব override { প্রত্যাগম "yellow"; }
    File_সারি* fileline() ধ্রুব override { প্রত্যাগম nodep()->fileline(); }
};

বর্গ TraceTraceVertex final : সার্বজনিক V3GraphVertex {
    VL_RTTI_IMPL(TraceTraceVertex, V3GraphVertex)
    AstTraceDecl* ধ্রুব m_nodep;  // TRACEINC this represents
    // nullptr, or other vertex with the real code() that duplicates this one
    TraceTraceVertex* m_duplicatep = nullptr;
    // When aliasing to a dtype parent, offset of the member within the dtype
    uint32_t m_dtypeAliasOffset = 0;

সার্বজনিক:
    TraceTraceVertex(V3Graph* graphp, AstTraceDecl* nodep)
        : V3GraphVertex{graphp}
        , m_nodep{nodep} {}
    ~TraceTraceVertex() override = স্বভাব;
    // ACCESSORS
    AstTraceDecl* nodep() ধ্রুব { প্রত্যাগম m_nodep; }
    string name() ধ্রুব override { প্রত্যাগম nodep()->name(); }
    string dotColor() ধ্রুব override { প্রত্যাগম "red"; }
    File_সারি* fileline() ধ্রুব override { প্রত্যাগম nodep()->fileline(); }
    TraceTraceVertex* duplicatep() ধ্রুব { প্রত্যাগম m_duplicatep; }
    শূন্য duplicatep(TraceTraceVertex* dupp) {
        UASSERT_OBJ(!duplicatep(), nodep(), "Assigning duplicatep() to already duplicated node");
        m_duplicatep = dupp;
    }
    শূন্য redirectDuplicatep(TraceTraceVertex* dupp) { m_duplicatep = dupp; }
    uint32_t dtypeAliasOffset() ধ্রুব { প্রত্যাগম m_dtypeAliasOffset; }
    শূন্য dtypeAliasOffset(uint32_t offset) { m_dtypeAliasOffset = offset; }
};

বর্গ TraceVarVertex final : সার্বজনিক V3GraphVertex {
    VL_RTTI_IMPL(TraceVarVertex, V3GraphVertex)
    AstVarScope* ধ্রুব m_nodep;

সার্বজনিক:
    TraceVarVertex(V3Graph* graphp, AstVarScope* nodep)
        : V3GraphVertex{graphp}
        , m_nodep{nodep} {}
    ~TraceVarVertex() override = স্বভাব;
    // ACCESSORS
    AstVarScope* nodep() ধ্রুব { প্রত্যাগম m_nodep; }
    string name() ধ্রুব override { প্রত্যাগম nodep()->name(); }
    string dotColor() ধ্রুব override { প্রত্যাগম "skyblue"; }
    File_সারি* fileline() ধ্রুব override { প্রত্যাগম nodep()->fileline(); }
};

//######################################################################
// Trace state, as a visitor of each AstNode

বর্গ TraceVisitor final : সার্বজনিক VNVisitor {
    // NODE STATE
    // V3Hasher in V3DupFinder
    //  Ast*::user4()                   // V3Hasher calculation
    // Cleared entire netlist
    //  AstCFunc::user1()               // V3GraphVertex* for this node
    //  AstCFunc::user2()               // bool; func contains trace decls (needs splitting)
    //  AstTraceDecl::user1()           // V3GraphVertex* for this node
    //  AstTraceDecl::user2()           // dtype decl cannot be used for _chg
    //  AstVarScope::user1()            // V3GraphVertex* for this node
    //  AstStmtExpr::user2()            // bool; walked next list for other ccalls
    //  AstVarRef::user2()              // dtype V3TraceDecl* for this node
    //  Ast*::user3()                   // TraceActivityVertex* for this node
    ধ্রুব VNUser1InUse m_inuser1;
    ধ্রুব VNUser2InUse m_inuser2;
    ধ্রুব VNUser3InUse m_inuser3;
    // VNUser4InUse     In V3Hasher via V3DupFinder

    // STATE
    Ast_গ্রন্থি_Module* m_শীর্ষ_Modp = nullptr;  // Module to add variables to
    AstScope* ধ্রুব m_শীর্ষ_Scopep = v3Global.rootp()->শীর্ষ_Scopep()->scopep();  // The top AstScope
    AstCFunc* m_cfuncp = nullptr;  // C function adding to graph
    AstCFunc* m_regFuncp = nullptr;  // Trace registration function
    AstCFunc* m_actAllFuncp = nullptr;  // Set all activity function
    AstTraceDecl* m_tracep = nullptr;  // Trace function adding to graph
    AstVarScope* m_activityVscp = nullptr;  // Activity variable
    uint32_t m_activity_সংখ্যা = 0;  // Count of fields in activity variable
    uint32_t m_code = 0;  // Trace ident code# being assigned
    V3Graph m_graph;  // Var/CFunc tracking
    TraceActivityVertex* ধ্রুব m_alwaysVtxp;  // "Always trace" vertex
    তার্কিক m_finding = অসত্য;  // Pass one of algorithm?
    সংরচনা DtypeFuncs final {
    সার্বজনিক:
        AstCFunc* fullFuncp = nullptr;
        AstCFunc* chgFuncp = nullptr;
    };
    std::unordered_map<ধ্রুব Ast_গ্রন্থি_DType*, DtypeFuncs>
        m_dtypeNonConstFuncs;  // Full / Chg funcs per type
    std::unordered_map<ধ্রুব Ast_গ্রন্থি_DType*, AstCFunc*> m_dtypeConstFuncs;  // Const func per type
    V3UniqueNames m_dtypeNames{""};  // Unique type func names

    // Trace parallelism. Only VCD tracing can be parallelized at this time.
    ধ্রুব uint32_t m_parallelism
        = v3Global.opt.useTraceParallel() ? static_cast<uint32_t>(v3Global.opt.threads()) : 1;

    VDouble0 m_statSetters;  // Statistic tracking
    VDouble0 m_statSettersSlow;  // Statistic tracking
    VDouble0 m_statUniqSigs;  // Statistic tracking

    // All activity numbers applying to a given trace
    using ActCodeSet = std::set<uint32_t>;
    // For activity set, what traces apply
    using TraceVec = std::multimap<ActCodeSet, TraceTraceVertex*>;
    // Candidate interface-member VarScopes keyed by (interface type, member name)
    std::map<std::pair<ধ্রুব AstIface*, std::string>, std::vector<AstVarScope*>>
        m_ifaceMemberVscps;

    বর্গ TraceInitDeclCollector final : সার্বজনিক VNVisitor {
        std::vector<AstTraceDecl*>& m_declps;  // Output: trace declarations found so far
        std::set<ধ্রুব AstCFunc*> m_seenFuncps;

        শূন্য visit(AstTraceDecl* nodep) override { m_declps.ঠেলো_back(nodep); }
        শূন্য visit(AstCCall* nodep) override {
            যদি (AstCFunc* ধ্রুব funcp = nodep->funcp()) collect(funcp);
        }
        শূন্য visit(Ast_গ্রন্থি* nodep) override { iterateChildren(nodep); }

    সার্বজনিক:
        explicit TraceInitDeclCollector(std::vector<AstTraceDecl*>& declps)
            : m_declps{declps} {}
        শূন্য collect(AstCFunc* funcp) {
            যদি (funcp && m_seenFuncps.ঢোকাও(funcp).second) iterate(funcp);
        }
    };

    // METHODS

    স্থির তার্কিক same_মূল_Init_Alias(ধ্রুব AstTraceDecl* মূল_Declp, ধ্রুব AstTraceDecl* শীর্ষ_Declp) {
        ধ্রুব VNumRange& মূল_Bit_Range = মূল_Declp->bitRange();
        ধ্রুব VNumRange& শীর্ষ_Bit_Range = শীর্ষ_Declp->bitRange();
        প্রত্যাগম মূল_Declp->showname() == শীর্ষ_Declp->showname()
               && মূল_Declp->declDirection() == শীর্ষ_Declp->declDirection()
               && মূল_Declp->widthMin() == শীর্ষ_Declp->widthMin()
               && মূল_Bit_Range.ranged() == শীর্ষ_Bit_Range.ranged()
               && (!মূল_Bit_Range.ranged()
                   || (মূল_Bit_Range.বাম() == শীর্ষ_Bit_Range.বাম()
                       && মূল_Bit_Range.ডান() == শীর্ষ_Bit_Range.ডান()));
    }

    শূন্য detectDuplicates() {
        UINFO(9, "Finding duplicates");
        // Note uses user4
        V3DupFinder dupFinder;  // Duplicate code detection

        // Compute member offsets within dtype instances incrementally.
        // For each dtype member TraceDecl, stores its code offset within the
        // dtype parent's code range.
        std::unordered_map<ধ্রুব AstTraceDecl*, uint32_t> memberOffsets;
        std::unordered_map<ধ্রুব AstTraceDecl*, uint32_t> runningOffset;

        // For fixup pass: track dups redirected to dtype parents, so we can
        // fix them if the parent's user2 is later set during the loop.
        // Maps dup vertex -> original canonical member vertex.
        std::vector<std::pair<TraceTraceVertex*, TraceTraceVertex*>> dtypeParentRedirects;

        // Hash all of the traced values and find if there are any duplicates
        প্রতি (V3GraphVertex& vtx : m_graph.vertices()) {
            যদি (TraceTraceVertex* ধ্রুব vvertexp = vtx.cast<TraceTraceVertex>()) {
                AstTraceDecl* ধ্রুব nodep = vvertexp->nodep();
                যদি (nodep->dtypeCallp()) অনুবর্ত;

                যদি (nodep->dtypeDeclp()) {
                    uint32_t& offset = runningOffset[nodep->dtypeDeclp()];
                    memberOffsets[nodep] = offset;
                    offset += nodep->codeInc();
                }

                UASSERT_OBJ(!vvertexp->duplicatep(), nodep, "Should not be a duplicate");
                ধ্রুব auto dupit = dupFinder.findDuplicate(nodep);
                যদি (dupit == dupFinder.end()) {
                    dupFinder.ঢোকাও(nodep);
                } অন্যথা {
                    AstTraceDecl* ধ্রুব dupDeclp = VN_AS(dupit->second, TraceDecl);
                    UASSERT_OBJ(dupDeclp, nodep, "Trace duplicate of wrong type");
                    TraceTraceVertex* ধ্রুব dupvertexp
                        = dupDeclp->user1u().toGraphVertex()->cast<TraceTraceVertex>();
                    UINFO(8, "  Orig " << dupDeclp << endl);
                    UINFO(8, "   dup " << nodep << endl);

                    যদি (dupDeclp->dtypeDeclp() && !dupDeclp->dtypeDeclp()->user2()) {
                        AstTraceDecl* ধ্রুব dtypeParentp = dupDeclp->dtypeDeclp();
                        TraceTraceVertex* ধ্রুব dtypeVtxp
                            = dtypeParentp->user1u().toGraphVertex()->cast<TraceTraceVertex>();
                        ধ্রুব auto it2 = memberOffsets.find(dupDeclp);
                        UASSERT_OBJ(it2 != memberOffsets.end(), dupDeclp,
                                    "Member offset not precomputed");
                        vvertexp->duplicatep(dtypeVtxp);
                        vvertexp->dtypeAliasOffset(it2->second);
                        dtypeParentRedirects.emplace_back(vvertexp, dupvertexp);
                    } অন্যথা {
                        vvertexp->duplicatep(dupvertexp);
                    }
                    যদি (nodep->dtypeDeclp()) nodep->dtypeDeclp()->user2(সত্য);
                }
            }
        }

        প্রতি (ধ্রুব auto& pair : dtypeParentRedirects) {
            TraceTraceVertex* ধ্রুব dupVtxp = pair.first;
            TraceTraceVertex* ধ্রুব memberVtxp = pair.second;
            ধ্রুব AstTraceDecl* ধ্রুব parentDeclp = dupVtxp->duplicatep()->nodep();
            যদি (parentDeclp->user2()) {
                dupVtxp->redirectDuplicatep(memberVtxp);
                dupVtxp->dtypeAliasOffset(0);
            }
        }

        যদি (dumpLevel() || debug() >= 9)
            dupFinder.dumpFile(v3Global.debugFilename("trace") + ".hash", অসত্য);
    }

    শূন্য graphDtypePrune() {
        প্রতি (V3GraphVertex* ধ্রুব vtxp : m_graph.vertices().unlinkable()) {
            যদি (TraceTraceVertex* ধ্রুব vvertexp = vtxp->cast<TraceTraceVertex>()) {
                AstTraceDecl* ধ্রুব declp = vvertexp->nodep();
                // This skips the dtype sub-func optimization if a var is affected by multiple
                // activities.  We really only need to do this for _chg funcs (and not decls,
                // _const and _full) but it's simpiler to do it all one way or the other.
                যদি (declp) {
                    যদি (declp->user2() || (declp->dtypeDeclp() && !declp->dtypeDeclp()->user2())) {
                        AstCCall* ধ্রুব callp = declp->dtypeCallp();
                        যদি (callp) {
                            Ast_গ্রন্থি* stmtexprp = callp->backp();
                            VL_DO_DANGLING(ঠেলো_Deletep(stmtexprp->unlinkFrBack()), stmtexprp);
                        } অন্যথা {
                            তার্কিক emptyScope;
                            কুরু {
                                emptyScope = অসত্য;
                                Ast_গ্রন্থি* ধ্রুব declBackp = declp->backp();
                                Ast_গ্রন্থি* ধ্রুব declNextp = declp->nextp();
                                যদি (VN_IS(declBackp, Trace_ঠেলো_Prefix)
                                    && VN_IS(declNextp, Trace_বের_করো_Prefix)) {
                                    VL_DO_DANGLING(ঠেলো_Deletep(declBackp->unlinkFrBack()),
                                                   declBackp);
                                    VL_DO_DANGLING(ঠেলো_Deletep(declNextp->unlinkFrBack()),
                                                   declNextp);
                                    emptyScope = সত্য;
                                }
                            } যাবত্ (emptyScope);
                        }
                        // Can't purge until we finish this pass
                        ঠেলো_Deletep(declp->unlinkFrBack());  // declp used below
                        vvertexp->rerouteEdges(&m_graph);
                        vvertexp->unlinkDelete(&m_graph);
                    }
                    declp->dtypeDeclp(nullptr);
                }
            }
        }
    }

    শূন্য splitTraceDeclFuncs() {
        ধ্রুব পূর্ণাঙ্ক splitLimit = v3Global.opt.outputSplitCTrace() ? v3Global.opt.outputSplitCTrace()
                                                                : std::numeric_limits<পূর্ণাঙ্ক>::max();
        যদি (splitLimit == std::numeric_limits<পূর্ণাঙ্ক>::max()) প্রত্যাগম;

        প্রতি (Ast_গ্রন্থি* nodep = m_শীর্ষ_Scopep->blocksp(); nodep; nodep = nodep->nextp()) {
            AstCFunc* ধ্রুব funcp = VN_CAST(nodep, CFunc);
            যদি (!funcp || !funcp->user2()) অনুবর্ত;

            ধ্রুব string baseName = funcp->name();
            uint32_t sub_অঙ্ক = 1;
            পূর্ণাঙ্ক curSize = 0;
            AstCFunc* curFuncp = funcp;
            std::vector<Ast_গ্রন্থি_Stmt*> callStmts;

            auto startNewSubFunc = [&]() {
                File_সারি* ধ্রুব flp = funcp->fileline();
                ধ্রুব string newName = baseName + "_" + cvtToStr(sub_অঙ্ক++);
                AstCFunc* ধ্রুব newFuncp = নূতন AstCFunc{flp, newName, m_শীর্ষ_Scopep};
                newFuncp->argTypes(v3Global.opt.traceClassBase() + "* tracep");
                newFuncp->isTrace(সত্য);
                newFuncp->isStatic(অসত্য);
                newFuncp->isLoose(সত্য);
                newFuncp->slow(সত্য);
                newFuncp->dontCombine(সত্য);
                m_শীর্ষ_Scopep->addBlocksp(newFuncp);
                newFuncp->addStmtsp(নূতন AstCStmt{flp, "const int c = vlSymsp->__Vm_baseCode;"});
                AstCCall* ধ্রুব callp = নূতন AstCCall{flp, newFuncp};
                callp->dtypeSetVoid();
                callp->argTypes("tracep");
                callStmts.ঠেলো_back(callp->makeStmt());
                curFuncp = newFuncp;
                curSize = 0;
            };

            প্রতি (Ast_গ্রন্থি *stmtp = funcp->stmtsp(), *nextp; stmtp; stmtp = nextp) {
                nextp = stmtp->nextp();
                curSize += stmtp->গ্রন্থি_গুনতি();

                যদি (curFuncp != funcp) {
                    stmtp->unlinkFrBack();
                    curFuncp->addStmtsp(stmtp);
                }

                যদি (curSize > splitLimit && nextp) startNewSubFunc();
            }

            প্রতি (Ast_গ্রন্থি_Stmt* ধ্রুব callStmtp : callStmts) { funcp->addStmtsp(callStmtp); }
        }
    }

    শূন্য graphSimplify(তার্কিক initial) {
        যদি (initial) {
            // Remove all variable nodes
            প্রতি (V3GraphVertex* ধ্রুব vtxp : m_graph.vertices().unlinkable()) {
                যদি (TraceVarVertex* ধ্রুব vvertexp = vtxp->cast<TraceVarVertex>()) {
                    vvertexp->rerouteEdges(&m_graph);
                    vvertexp->unlinkDelete(&m_graph);
                }
            }
            // Remove multiple variables connecting funcs to traces
            // We do this twice, as then we have fewer edges to multiply out in the below
            // expansion.
            m_graph.removeRedundantEdgesMax(&V3GraphEdge::followAlwaysTrue);
            // Remove all Cfunc nodes
            প্রতি (V3GraphVertex* ধ্রুব vtxp : m_graph.vertices().unlinkable()) {
                যদি (TraceCFuncVertex* ধ্রুব vvertexp = vtxp->cast<TraceCFuncVertex>()) {
                    vvertexp->rerouteEdges(&m_graph);
                    vvertexp->unlinkDelete(&m_graph);
                }
            }
        }

        // Remove multiple variables connecting funcs to traces
        m_graph.removeRedundantEdgesMax(&V3GraphEdge::followAlwaysTrue);

        // If there are any edges from a always, keep only the always
        প্রতি (V3GraphVertex& vtx : m_graph.vertices()) {
            যদি (TraceTraceVertex* ধ্রুব vvertexp = vtx.cast<TraceTraceVertex>()) {
                // Search for the incoming always edge
                ধ্রুব V3GraphEdge* alwaysEdgep = nullptr;
                প্রতি (ধ্রুব V3GraphEdge& edge : vvertexp->inEdges()) {
                    ধ্রুব TraceActivityVertex* ধ্রুব actVtxp
                        = edge.fromp()->as<ধ্রুব TraceActivityVertex>();
                    যদি (actVtxp->activityAlways()) {
                        alwaysEdgep = &edge;
                        ভঙ্গ;
                    }
                }
                // If always edge exists, remove all other edges
                যদি (alwaysEdgep) {
                    প্রতি (V3GraphEdge* ধ্রুব edgep : vvertexp->inEdges().unlinkable()) {
                        যদি (edgep != alwaysEdgep) VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
                    }
                }
            }
        }

        // Activity points with no outputs can be removed
        প্রতি (V3GraphVertex* ধ্রুব vtxp : m_graph.vertices().unlinkable()) {
            যদি (TraceActivityVertex* ধ্রুব aVtxp = vtxp->cast<TraceActivityVertex>()) {
                // Leave in the always vertex for later use.
                যদি (aVtxp != m_alwaysVtxp && aVtxp->outEmpty()) {
                    VL_DO_DANGLING(aVtxp->unlinkDelete(&m_graph), aVtxp);
                }
            }
        }
    }

    uint32_t assignactivityNumbers() {
        uint32_t activity_সংখ্যা = 1;  // Note 0 indicates "slow" only
        প্রতি (V3GraphVertex& vtx : m_graph.vertices()) {
            যদি (TraceActivityVertex* ধ্রুব vvertexp = vtx.cast<TraceActivityVertex>()) {
                যদি (vvertexp != m_alwaysVtxp) {
                    যদি (vvertexp->slow()) {
                        vvertexp->activityCode(TraceActivityVertex::ACTIVITY_SLOW);
                    } অন্যথা {
                        vvertexp->activityCode(activity_সংখ্যা++);
                    }
                }
            }
        }
        প্রত্যাগম activity_সংখ্যা;
    }

    শূন্য sortTraces(TraceVec& traces, uint32_t& nNonConstCodes) {
        // Populate sort structure
        traces.clear();
        nNonConstCodes = 0;
        প্রতি (V3GraphVertex& vtx : m_graph.vertices()) {
            যদি (TraceTraceVertex* ধ্রুব vtxp = vtx.cast<TraceTraceVertex>()) {
                ActCodeSet actSet;
                UINFO(9, "  Add to sort: " << vtxp);
                UINFOTREE(9, vtxp->nodep(), "", "trnode");
                প্রতি (ধ্রুব V3GraphEdge& edge : vtxp->inEdges()) {
                    ধ্রুব TraceActivityVertex* ধ্রুব cfvertexp
                        = edge.fromp()->cast<ধ্রুব TraceActivityVertex>();
                    UASSERT_OBJ(cfvertexp, vtxp->nodep(),
                                "Should have been function pointing to this trace");
                    UINFO(9, "   Activity: " << cfvertexp);
                    যদি (cfvertexp->activityAlways()) {
                        // If code 0, we always trace; ignore other codes
                        actSet.ঢোকাও(TraceActivityVertex::ACTIVITY_ALWAYS);
                    } অন্যথা {
                        actSet.ঢোকাও(cfvertexp->activityCode());
                    }
                }
                UASSERT_OBJ(actSet.গুনতি(TraceActivityVertex::ACTIVITY_ALWAYS) == 0
                                || actSet.size() == 1,
                            vtxp->nodep(), "Always active trace has further triggers");
                // Count nodes
                যদি (!vtxp->duplicatep() && !actSet.empty())
                    nNonConstCodes += vtxp->nodep()->codeInc();
                যদি (actSet.empty()) {
                    // If a trace doesn't have activity, it's constant, and we
                    // don't need to track changes on it.
                    actSet.ঢোকাও(TraceActivityVertex::ACTIVITY_NEVER);
                } অন্যথা যদি (actSet.গুনতি(TraceActivityVertex::ACTIVITY_SLOW) && actSet.size() > 1) {
                    // If a trace depends on the slow flag as well as other
                    // flags, remove the dependency on the slow flag. We will
                    // make slow routines set all activity flags.
                    actSet.erase(TraceActivityVertex::ACTIVITY_SLOW);
                }
                traces.emplace(actSet, vtxp);
            }
        }
    }

    শূন্য graphOptimize() {
        // Assign initial activity numbers to activity vertices
        assignactivityNumbers();

        // Sort the traces by activity sets
        TraceVec traces;
        uint32_t unused1;
        sortTraces(traces, unused1);

        // For each activity set with only a small number of signals, make those
        // signals always traced, as it's cheaper to check a few value changes
        // than to test a lot of activity flags
        auto it = traces.begin();
        ধ্রুব auto end = traces.end();
        যাবত্ (it != end) {
            auto head = it;
            // Approximate the complexity of the value change check
            uint32_t complexity = 0;
            ধ্রুব ActCodeSet& actSet = it->first;
            প্রতি (; it != end && it->first == actSet; ++it) {
                যদি (!it->second->duplicatep()) {
                    uint32_t cost = 0;
                    ধ্রুব AstTraceDecl* ধ্রুব declp = it->second->nodep();
                    // The number of comparisons required by bufp->chg*
                    cost += declp->isWide() ? declp->codeInc() : 1;
                    // Arrays are traced by element
                    cost *= declp->arrayRange().ranged() ? declp->arrayRange().elements() : 1;
                    // Note: Experiments factoring in the size of declp->valuep()
                    // showed no benefit in tracing speed, even for large trees,
                    // so we will leave those out for now.
                    complexity += cost;
                }
            }
            // Leave alone always changing, never changing and signals only set in slow code
            যদি (actSet.গুনতি(TraceActivityVertex::ACTIVITY_ALWAYS)) অনুবর্ত;
            যদি (actSet.গুনতি(TraceActivityVertex::ACTIVITY_NEVER)) অনুবর্ত;
            যদি (actSet.গুনতি(TraceActivityVertex::ACTIVITY_SLOW)) অনুবর্ত;
            // If the value comparisons are cheaper to perform than checking the
            // activity flags make the signals always traced. Note this cost
            // equation is heuristic.
            যদি (complexity <= actSet.size() * 2) {
                প্রতি (; head != it; ++head) {
                    নূতন V3GraphEdge{&m_graph, m_alwaysVtxp, head->second, 1};
                }
            }
        }

        graphSimplify(অসত্য);
    }

    Ast_গ্রন্থি_রাশিমালা* selectActivity(File_সারি* flp, uint32_t acode, ধ্রুব VAccess& access) {
        প্রত্যাগম নূতন AstArraySel(flp, নূতন AstVarRef{flp, m_activityVscp, access}, acode);
    }

    Ast_গ্রন্থি* newActivitySetter(Ast_গ্রন্থি* insertp, uint32_t code) {
        ++m_statSetters;
        File_সারি* ধ্রুব fl = insertp->fileline();
        AstAssign* ধ্রুব setterp = নূতন AstAssign{fl, selectActivity(fl, code, VAccess::WRITE),
                                                 নূতন AstConst{fl, AstConst::BitTrue{}}};
        প্রত্যাগম setterp;
    }

    Ast_গ্রন্থি* newActivityAll(Ast_গ্রন্থি* insertp) {
        ++m_statSettersSlow;
        যদি (!m_actAllFuncp) {
            File_সারি* ধ্রুব flp = m_শীর্ষ_Scopep->fileline();
            AstCFunc* ধ্রুব funcp = নূতন AstCFunc{flp, "__VtraceActivitySetAll", m_শীর্ষ_Scopep};
            funcp->slow(সত্য);
            funcp->isStatic(অসত্য);
            funcp->isLoose(সত্য);
            m_শীর্ষ_Scopep->addBlocksp(funcp);
            প্রতি (uint32_t code = 0; code < m_activity_সংখ্যা; ++code) {
                Ast_গ্রন্থি* ধ্রুব setterp = newActivitySetter(insertp, code);
                funcp->addStmtsp(setterp);
            }
            m_actAllFuncp = funcp;
        }
        AstCCall* ধ্রুব callp = নূতন AstCCall{insertp->fileline(), m_actAllFuncp};
        callp->dtypeSetVoid();
        প্রত্যাগম callp->makeStmt();
    }

    শূন্য createActivityFlags() {
        // Assign final activity numbers
        m_activity_সংখ্যা = assignactivityNumbers();

        // Create an array of bytes, not a bit vector, as they can be set
        // atomically by mtasks, and are cheaper to set (no need for
        // read-modify-write on the C type), and the speed of the tracing code
        // is the same on largish designs.
        File_সারি* ধ্রুব flp = m_শীর্ষ_Scopep->fileline();
        Ast_গ্রন্থি_DType* ধ্রুব newScalarDtp = নূতন AstBasicDType{flp, VFlagBitPacked{}, 1};
        v3Global.rootp()->typeTablep()->addTypesp(newScalarDtp);
        AstRange* ধ্রুব newArange
            = নূতন AstRange{flp, VNumRange{static_cast<পূর্ণাঙ্ক>(m_activity_সংখ্যা) - 1, 0}};
        Ast_গ্রন্থি_DType* ধ্রুব newArrDtp = নূতন AstUnpackArrayDType{flp, newScalarDtp, newArange};
        v3Global.rootp()->typeTablep()->addTypesp(newArrDtp);
        m_activityVscp = v3Global.rootp()->শীর্ষ_Scopep()->createTemp("__VtraceActivity", newArrDtp);

        // Insert activity setters
        প্রতি (ধ্রুব V3GraphVertex& vtx : m_graph.vertices()) {
            যদি (ধ্রুব TraceActivityVertex* ধ্রুব vtxp = vtx.cast<ধ্রুব TraceActivityVertex>()) {
                Ast_গ্রন্থি* setterp = nullptr;
                যদি (vtxp->activitySlow()) {
                    setterp = newActivityAll(vtxp->insertp());
                    // Just set all flags in slow code as it should be rare.
                    // This will be rolled up into a loop by V3Reloop.
                } অন্যথা যদি (!vtxp->activityAlways()) {
                    setterp = newActivitySetter(vtxp->insertp(), vtxp->activityCode());
                }
                যদি (setterp) {
                    Ast_গ্রন্থি* ধ্রুব insertp = vtxp->insertp();
                    যদি (Ast_Stmt_রাশিমালা* ধ্রুব stmtp = VN_CAST(insertp, Stmt_রাশিমালা)) {
                        stmtp->addNextHere(setterp);
                    } অন্যথা যদি (AstCFunc* ধ্রুব funcp = VN_CAST(insertp, CFunc)) {
                        // If there are awaits, insert the setter after each await
                        যদি (funcp->isCoroutine() && funcp->stmtsp()) {
                            funcp->stmtsp()->foreachAndNext([setterp](AstCAwait* awaitp) {
                                awaitp->addNextHere(setterp->clone_বৃক্ষ(অসত্য));
                            });
                        }
                        funcp->addStmtsp(setterp);
                    } অন্যথা {
                        insertp->v3fatalSrc("Bad trace activity vertex");
                    }
                }
            }
        }
    }

    AstCFunc* newCFunc(VTraceType traceType, AstCFunc* শীর্ষ_Funcp, uint32_t func_অঙ্ক,
                       uint32_t baseCode = 0, ধ্রুব AstTraceDecl* ধ্রুব declp = nullptr,
                       তার্কিক declSub = অসত্য) {
        // Create new function
        ধ্রুব তার্কিক is_শীর্ষ_Func = !declp && শীর্ষ_Funcp == nullptr;
        std::string funcName;
        যদি (is_শীর্ষ_Func || declp) {
            funcName = traceType.func_prefix();
        } অন্যথা {
            funcName = শীর্ষ_Funcp->name();
            funcName += "_sub";
        }
        যদি (declp) {
            funcName += "_dtype";
            যদি (declSub) funcName += "_sub";
            funcName += "__";
            funcName = m_dtypeNames.get(funcName);
        } অন্যথা {
            funcName += "_";
            funcName += cvtToStr(func_অঙ্ক);
        }

        File_সারি* ধ্রুব flp = m_শীর্ষ_Scopep->fileline();
        AstCFunc* ধ্রুব funcp = নূতন AstCFunc{flp, funcName, m_শীর্ষ_Scopep};
        funcp->isTrace(সত্য);
        funcp->dontCombine(সত্য);
        funcp->isLoose(সত্য);
        funcp->slow(traceType != VTraceType::CHANGE);
        funcp->isStatic(is_শীর্ষ_Func);
        // Add it to top scope
        m_শীর্ষ_Scopep->addBlocksp(funcp);
        ধ্রুব std::string bufArg
            = v3Global.opt.traceClassBase() + "::Buffer* bufp"
              + (declp ? (", uint32_t offset, const " + declp->dtypep()->cType("", সত্য, সত্য)
                          + " __VdtypeVar")
                       : "");
        যদি (is_শীর্ষ_Func) {
            // Top functions
            funcp->argTypes("void* voidSelf, " + bufArg);
            funcp->addStmtsp(নূতন AstCStmt{flp, EmitCUtil::voidSelfAssign(m_শীর্ষ_Modp)});
            funcp->addStmtsp(নূতন AstCStmt{flp, EmitCUtil::symClassAssign()});
            // Add global activity check to change dump functions
            যদি (traceType == VTraceType::CHANGE) {  //
                funcp->addStmtsp(
                    নূতন AstCStmt{flp, "if (VL_UNLIKELY(!vlSymsp->__Vm_activity)) return;"});
            }
            // Register function
            AstCStmt* ধ্রুব cstmtp = নূতন AstCStmt{flp};
            m_regFuncp->addStmtsp(cstmtp);
            যদি (traceType == VTraceType::CONSTANT) {
                cstmtp->add("tracep->addConstCb(");
                cstmtp->add(নূতন AstAddrOfCFunc{flp, funcp});
                cstmtp->add(", " + std::to_string(func_অঙ্ক) + ", vlSelf);");
            } অন্যথা যদি (traceType == VTraceType::FULL) {
                cstmtp->add("tracep->addFullCb(");
                cstmtp->add(নূতন AstAddrOfCFunc{flp, funcp});
                cstmtp->add(", " + std::to_string(func_অঙ্ক) + ", vlSelf);");
            } অন্যথা {
                cstmtp->add("tracep->addChgCb(");
                cstmtp->add(নূতন AstAddrOfCFunc{flp, funcp});
                cstmtp->add(", " + std::to_string(func_অঙ্ক) + ", vlSelf);");
            }
        } অন্যথা {
            // Sub functions
            funcp->argTypes(bufArg);
            // Setup base references. Note in rare occasions we can end up with an empty trace
            // sub function, hence the VL_ATTR_UNUSED attributes.
            যদি (traceType != VTraceType::CHANGE) {
                // Full dump sub function
                funcp->addStmtsp(নূতন AstCStmt{flp,  //
                                              string("uint32_t* const oldp VL_ATTR_UNUSED = "
                                                     "bufp->oldp(vlSymsp->__Vm_baseCode")
                                                  + (declp ? " + offset" : "") + ");\n"});
            } অন্যথা {
                // Change dump sub function
                funcp->addStmtsp(নূতন AstCStmt{flp,  //
                                              "uint32_t* const oldp VL_ATTR_UNUSED = "
                                              "bufp->oldp(vlSymsp->__Vm_baseCode + "
                                                  + (declp ? " offset" : cvtToStr(baseCode))
                                                  + ");\n"});
            }
            যদি (!declp) {
                // Add call to top function
                AstCCall* ধ্রুব callp = নূতন AstCCall{funcp->fileline(), funcp};
                callp->dtypeSetVoid();
                callp->argTypes("bufp");
                শীর্ষ_Funcp->addStmtsp(callp->makeStmt());
            }
        }
        // Done
        UINFO(5, "  newCFunc " << funcp);
        প্রত্যাগম funcp;
    }

    AstCFunc* newCDtypeSubFunc(VTraceType traceType, ধ্রুব AstTraceDecl* ধ্রুব declp,
                               AstCFunc* parentp) {
        AstCFunc* ধ্রুব funcp = newCFunc(traceType, nullptr, 0, 0, declp, সত্য);
        File_সারি* ধ্রুব flp = m_শীর্ষ_Scopep->fileline();
        AstCCall* ধ্রুব callp = নূতন AstCCall{flp, funcp};
        callp->dtypeSetVoid();
        callp->argTypes("bufp, offset, __VdtypeVar");
        parentp->addStmtsp(callp->makeStmt());
        প্রত্যাগম funcp;
    }

    AstCFunc* createConstDtypeTraceFunctions(ধ্রুব AstTraceDecl* declp) {
        ধ্রুব Ast_গ্রন্থি_DType* ধ্রুব dtypep = declp->dtypep()->skipRefp();
        auto pair = m_dtypeConstFuncs.emplace(dtypep, nullptr);
        যদি (pair.second) {
            File_সারি* ধ্রুব flp = declp->fileline();
            AstCFunc* ধ্রুব funcp = newCFunc(VTraceType::CONSTANT, nullptr, 0, 0, declp);

            তার্কিক first = সত্য;
            প্রতি (Ast_গ্রন্থি* callStmtp = declp->dtypeCallp()->funcp()->stmtsp(); callStmtp;
                 callStmtp = callStmtp->nextp()) {
                ধ্রুব AstCCall* ধ্রুব callp
                    = VN_CAST(VN_CAST(callStmtp, Stmt_রাশিমালা)->exprp(), CCall);
                তার্কিক onlyOne = first && !callStmtp->nextp();
                AstCFunc* ধ্রুব subFuncp
                    = onlyOne ? funcp : newCDtypeSubFunc(VTraceType::CONSTANT, declp, funcp);
                প্রতি (Ast_গ্রন্থি* stmtp = callp->funcp()->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    যদি (AstTraceDecl* ধ্রুব fieldDeclp = VN_CAST(stmtp, TraceDecl)) {
                        AstTraceInc* ধ্রুব incp
                            = নূতন AstTraceInc{flp, fieldDeclp, VTraceType::CONSTANT};
                        subFuncp->addStmtsp(incp);
                    }
                }
                first = অসত্য;
            }

            pair.first->second = funcp;
        }

        প্রত্যাগম pair.first->second;
    }

    শূন্য createConstTraceFunctions(ধ্রুব TraceVec& traces) {
        ধ্রুব পূর্ণাঙ্ক splitLimit = v3Global.opt.outputSplitCTrace() ? v3Global.opt.outputSplitCTrace()
                                                                : std::numeric_limits<পূর্ণাঙ্ক>::max();

        AstCFunc* ধ্রুব শীর্ষ_Funcp = newCFunc(VTraceType::CONSTANT, nullptr, 0);
        uint32_t sub_Func_অঙ্ক = 0;
        AstCFunc* subFuncp = nullptr;
        পূর্ণাঙ্ক subStmts = 0;
        std::vector<ধ্রুব TraceTraceVertex*> duplicates;
        প্রতি (auto it = traces.cbegin(); it != traces.end(); ++it) {
            ধ্রুব TraceTraceVertex* ধ্রুব vtxp = it->second;
            AstTraceDecl* ধ্রুব declp = vtxp->nodep();
            যদি (vtxp->duplicatep()) {
                duplicates.ঠেলো_back(vtxp);
                অনুবর্ত;
            }

            // This is a canonical trace node. Assign trace code (signal number).
            UASSERT_OBJ(!declp->codeAssigned(), declp,
                        "Canonical node should not have code assigned yet");
            declp->code(m_code);
            ধ্রুব uint32_t codeInc = declp->codeInc();
            m_code += codeInc;
            ++m_statUniqSigs;

            // If this is a const signal, add the AstTraceInc
            ধ্রুব ActCodeSet& actSet = it->first;
            যদি (actSet.গুনতি(TraceActivityVertex::ACTIVITY_NEVER)) {
                // Crate new sub function if required
                যদি (!subFuncp || subStmts > splitLimit) {
                    subStmts = 0;
                    subFuncp = newCFunc(VTraceType::CONSTANT, শীর্ষ_Funcp, sub_Func_অঙ্ক);
                    ++sub_Func_অঙ্ক;
                }
                File_সারি* ধ্রুব flp = declp->fileline();
                যদি (declp->dtypeCallp()) {
                    AstCFunc* ধ্রুব funcp = createConstDtypeTraceFunctions(declp);
                    Ast_গ্রন্থি_রাশিমালা* argsp = nullptr;
                    argsp = Ast_গ্রন্থি::addNext(argsp, declp->valuep()->clone_বৃক্ষ(অসত্য));
                    AstCCall* ধ্রুব callp = নূতন AstCCall{flp, funcp, argsp};
                    callp->dtypeSetVoid();
                    callp->argTypes(callp->argTypes() + "bufp, " + std::to_string(declp->code()));
                    subFuncp->addStmtsp(callp->makeStmt());

                    subStmts += 1;
                } অন্যথা {
                    AstTraceInc* ধ্রুব incp = নূতন AstTraceInc{flp, declp, VTraceType::CONSTANT};
                    subFuncp->addStmtsp(incp);
                    subStmts += incp->গ্রন্থি_গুনতি();
                }
            }
        }

        // Assign codes to duplicate nodes from their canonicals
        প্রতি (ধ্রুব TraceTraceVertex* ধ্রুব vtxp : duplicates) {
            AstTraceDecl* ধ্রুব declp = vtxp->nodep();
            ধ্রুব TraceTraceVertex* ধ্রুব canonVtxp = vtxp->duplicatep();
            ধ্রুব AstTraceDecl* ধ্রুব canonDeclp = canonVtxp->nodep();
            UASSERT_OBJ(!canonVtxp->duplicatep(), canonDeclp, "Canonical node is a duplicate");
            UASSERT_OBJ(canonDeclp->codeAssigned(), canonDeclp,
                        "Canonical node should have code assigned already");
            declp->code(canonDeclp->code() + vtxp->dtypeAliasOffset());
        }
    }

    DtypeFuncs createNonConstDtypeTraceFunctions(ধ্রুব AstTraceDecl* declp) {
        Ast_গ্রন্থি_DType* dtypep = declp->dtypep()->skipRefp();
        auto pair = m_dtypeNonConstFuncs.emplace(dtypep, DtypeFuncs{});
        যদি (pair.second) {
            File_সারি* ধ্রুব flp = declp->fileline();
            AstCFunc* ধ্রুব fullFuncp = newCFunc(VTraceType::FULL, nullptr, 0, 0, declp);
            AstCFunc* ধ্রুব chgFuncp = newCFunc(VTraceType::CHANGE, nullptr, 0, 0, declp);

            তার্কিক first = সত্য;
            প্রতি (Ast_গ্রন্থি* callStmtp = declp->dtypeCallp()->funcp()->stmtsp(); callStmtp;
                 callStmtp = callStmtp->nextp()) {
                ধ্রুব AstCCall* ধ্রুব callp
                    = VN_CAST(VN_CAST(callStmtp, Stmt_রাশিমালা)->exprp(), CCall);
                তার্কিক onlyOne = first && !callStmtp->nextp();
                AstCFunc* ধ্রুব fullSubFuncp
                    = onlyOne ? fullFuncp : newCDtypeSubFunc(VTraceType::FULL, declp, fullFuncp);
                AstCFunc* ধ্রুব chgSubFuncp
                    = onlyOne ? chgFuncp : newCDtypeSubFunc(VTraceType::CHANGE, declp, chgFuncp);
                প্রতি (Ast_গ্রন্থি* stmtp = callp->funcp()->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    যদি (AstTraceDecl* ধ্রুব fieldDeclp = VN_CAST(stmtp, TraceDecl)) {
                        AstTraceInc* ধ্রুব incFullp
                            = নূতন AstTraceInc{flp, fieldDeclp, VTraceType::FULL};
                        fullSubFuncp->addStmtsp(incFullp);
                        AstTraceInc* ধ্রুব incChgp
                            = নূতন AstTraceInc{flp, fieldDeclp, VTraceType::CHANGE};
                        chgSubFuncp->addStmtsp(incChgp);
                    }
                }
                first = অসত্য;
            }

            pair.first->second = {.fullFuncp = fullFuncp, .chgFuncp = chgFuncp};
        }

        প্রত্যাগম pair.first->second;
    }

    শূন্য createNonConstTraceFunctions(ধ্রুব TraceVec& traces, uint32_t nAllCodes,
                                      uint32_t parallelism) {
        ধ্রুব পূর্ণাঙ্ক splitLimit = v3Global.opt.outputSplitCTrace() ? v3Global.opt.outputSplitCTrace()
                                                                : std::numeric_limits<পূর্ণাঙ্ক>::max();

        // pre-incremented, so starts at 0
        uint32_t শীর্ষ_Func_অঙ্ক = std::numeric_limits<uint32_t>::max();
        TraceVec::const_iterator it = traces.begin();
        যাবত্ (it != traces.end()) {
            AstCFunc* শীর্ষ_Ful_Funcp = nullptr;
            AstCFunc* শীর্ষ_Chg_Funcp = nullptr;
            AstCFunc* subFulFuncp = nullptr;
            AstCFunc* subChgFuncp = nullptr;
            uint32_t sub_Func_অঙ্ক = 0;
            পূর্ণাঙ্ক subStmts = 0;
            ধ্রুব uint32_t maxCodes = std::max((nAllCodes + parallelism - 1) / parallelism, 1U);
            uint32_t nCodes = 0;
            ধ্রুব ActCodeSet* prevActSet = nullptr;
            AstIf* ifp = nullptr;
            uint32_t baseCode = 0;
            প্রতি (; nCodes < maxCodes && it != traces.end(); ++it) {
                ধ্রুব ActCodeSet& actSet = it->first;
                // Traced value never changes, no need to add it
                যদি (actSet.গুনতি(TraceActivityVertex::ACTIVITY_NEVER)) অনুবর্ত;

                ধ্রুব TraceTraceVertex* ধ্রুব vtxp = it->second;
                AstTraceDecl* ধ্রুব declp = vtxp->nodep();

                // This is a duplicate decl, no need to add it, but must set the
                // function index to the same as the canonical node.
                যদি (ধ্রুব TraceTraceVertex* ধ্রুব canonVtxp = vtxp->duplicatep()) {
                    declp->fidx(canonVtxp->nodep()->fidx());
                    অনুবর্ত;
                }

                // Create top function if not yet created
                যদি (!শীর্ষ_Ful_Funcp) {
                    ++শীর্ষ_Func_অঙ্ক;
                    শীর্ষ_Ful_Funcp = newCFunc(VTraceType::FULL, nullptr, শীর্ষ_Func_অঙ্ক);
                    শীর্ষ_Chg_Funcp = newCFunc(VTraceType::CHANGE, nullptr, শীর্ষ_Func_অঙ্ক);
                }

                // Create new sub function if required
                যদি (!subFulFuncp || subStmts > splitLimit) {
                    baseCode = declp->code();
                    subStmts = 0;
                    subFulFuncp = newCFunc(VTraceType::FULL, শীর্ষ_Ful_Funcp, sub_Func_অঙ্ক, baseCode);
                    subChgFuncp = newCFunc(VTraceType::CHANGE, শীর্ষ_Chg_Funcp, sub_Func_অঙ্ক, baseCode);
                    ++sub_Func_অঙ্ক;
                    prevActSet = nullptr;
                    ifp = nullptr;
                }

                // If required, create the conditional node checking the activity flags
                যদি (!prevActSet || actSet != *prevActSet) {
                    File_সারি* ধ্রুব flp = m_শীর্ষ_Scopep->fileline();
                    ধ্রুব তার্কিক always = actSet.গুনতি(TraceActivityVertex::ACTIVITY_ALWAYS) != 0;
                    Ast_গ্রন্থি_রাশিমালা* condp = nullptr;
                    যদি (always) {
                        condp = নূতন AstConst{flp, 1};  // Always true, will be folded later
                    } অন্যথা {
                        প্রতি (ধ্রুব uint32_t actCode : actSet) {
                            Ast_গ্রন্থি_রাশিমালা* ধ্রুব selp = selectActivity(flp, actCode, VAccess::READ);
                            condp = condp ? নূতন AstOr{flp, condp, selp} : selp;
                        }
                    }
                    ifp = নূতন AstIf{flp, condp};
                    যদি (!always) ifp->branchPred(VBranchPred::BP_UNLIKELY);
                    subChgFuncp->addStmtsp(ifp);
                    subStmts += ifp->গ্রন্থি_গুনতি();
                    prevActSet = &actSet;
                }

                // Add TraceInc nodes
                File_সারি* ধ্রুব flp = declp->fileline();
                যদি (declp->dtypeCallp()) {
                    DtypeFuncs funcs = createNonConstDtypeTraceFunctions(declp);
                    Ast_গ্রন্থি_রাশিমালা* argsp = nullptr;
                    argsp = Ast_গ্রন্থি::addNext(argsp, declp->valuep()->clone_বৃক্ষ(অসত্য));
                    AstCCall* ধ্রুব callFullp = নূতন AstCCall{flp, funcs.fullFuncp, argsp};
                    callFullp->dtypeSetVoid();
                    callFullp->argTypes(callFullp->argTypes() + "bufp, "
                                        + std::to_string(declp->code()));
                    subFulFuncp->addStmtsp(callFullp->makeStmt());
                    argsp = nullptr;
                    argsp = Ast_গ্রন্থি::addNext(argsp, declp->valuep()->clone_বৃক্ষ(অসত্য));
                    AstCCall* ধ্রুব callChgp = নূতন AstCCall{flp, funcs.chgFuncp, argsp};
                    callChgp->dtypeSetVoid();
                    callChgp->argTypes(callChgp->argTypes() + "bufp, "
                                       + std::to_string(declp->code()));
                    ifp->addThensp(callChgp->makeStmt());

                    subStmts += 2;
                } অন্যথা {
                    AstTraceInc* ধ্রুব incFulp = নূতন AstTraceInc{flp, declp, VTraceType::FULL};
                    subFulFuncp->addStmtsp(incFulp);
                    AstTraceInc* ধ্রুব incChgp
                        = নূতন AstTraceInc{flp, declp, VTraceType::CHANGE, baseCode};
                    ifp->addThensp(incChgp);

                    // Set the function index of the decl
                    declp->fidx(শীর্ষ_Func_অঙ্ক);

                    // Track splitting due to size
                    UASSERT_OBJ(incFulp->গ্রন্থি_গুনতি() == incChgp->গ্রন্থি_গুনতি(), declp,
                                "Should have equal cost");
                    ধ্রুব VNumRange range = declp->arrayRange();
                    যদি (range.ranged()) {
                        // 2x because each element is a TraceInc and a VarRef
                        subStmts += range.elements() * 2;
                    } অন্যথা {
                        subStmts += incChgp->গ্রন্থি_গুনতি();
                    }
                }

                // Track partitioning
                nCodes += declp->codeInc();
            }
        }
    }

    শূন্য createCleanupFunction() {
        File_সারি* ধ্রুব fl = m_শীর্ষ_Scopep->fileline();
        AstCFunc* ধ্রুব cleanupFuncp = নূতন AstCFunc{fl, "trace_cleanup", m_শীর্ষ_Scopep};
        cleanupFuncp->argTypes("void* voidSelf, " + v3Global.opt.traceClassBase()
                               + "* /*unused*/");
        cleanupFuncp->isTrace(সত্য);
        cleanupFuncp->slow(অসত্য);
        cleanupFuncp->isStatic(সত্য);
        cleanupFuncp->isLoose(সত্য);
        m_শীর্ষ_Scopep->addBlocksp(cleanupFuncp);
        cleanupFuncp->addStmtsp(নূতন AstCStmt{fl, EmitCUtil::voidSelfAssign(m_শীর্ষ_Modp)});
        cleanupFuncp->addStmtsp(নূতন AstCStmt{fl, EmitCUtil::symClassAssign()});

        // Register it
        {
            AstCStmt* ধ্রুব cstmtp = নূতন AstCStmt{fl};
            m_regFuncp->addStmtsp(cstmtp);
            cstmtp->add("tracep->addCleanupCb(");
            cstmtp->add(নূতন AstAddrOfCFunc{fl, cleanupFuncp});
            cstmtp->add(", vlSelf);");
        }

        // Clear global activity flag
        cleanupFuncp->addStmtsp(
            নূতন AstCStmt{m_শীর্ষ_Scopep->fileline(), "vlSymsp->__Vm_activity = false;"s});

        // Clear fine grained activity flags
        প্রতি (uint32_t i = 0; i < m_activity_সংখ্যা; ++i) {
            Ast_গ্রন্থি* ধ্রুব clrp = নূতন AstAssign{fl, selectActivity(fl, i, VAccess::WRITE),
                                                নূতন AstConst{fl, AstConst::BitFalse{}}};
            cleanupFuncp->addStmtsp(clrp);
        }
    }

    শূন্য createTraceFunctions() {
        // Detect and remove duplicate values
        detectDuplicates();

        graphDtypePrune();
        splitTraceDeclFuncs();
        m_graph.removeRedundantEdgesMax(&V3GraphEdge::followAlwaysTrue);

        // Simplify & optimize the graph
        যদি (dumpGraphLevel() >= 6) m_graph.dumpDotFilePrefixed("trace_pre");
        graphSimplify(সত্য);
        যদি (dumpGraphLevel() >= 6) m_graph.dumpDotFilePrefixed("trace_simplified");
        graphOptimize();
        যদি (dumpGraphLevel() >= 6) m_graph.dumpDotFilePrefixed("trace_optimized");

        // Create the fine grained activity flags
        createActivityFlags();

        // Form a sorted list of the traces we are interested in
        TraceVec traces;  // The sorted traces
        // We will split functions such that each have to dump roughly the same amount of data
        // for this we need to keep tack of the number of codes used by the trace functions.
        uint32_t nNonConstCodes = 0;
        sortTraces(traces, nNonConstCodes);

        // Our keys are now sorted to have same activity number adjacent, then
        // by trace order. (Better would be execution order for cache
        // efficiency....) Last are constants and non-changers, as then the
        // last value vector is more compact

        // Create the trace registration function
        m_regFuncp = নূতন AstCFunc{m_শীর্ষ_Scopep->fileline(), "trace_register", m_শীর্ষ_Scopep};
        m_regFuncp->argTypes(v3Global.opt.traceClassBase() + "* tracep");
        m_regFuncp->entryPoint(সত্য);
        m_regFuncp->isTrace(সত্য);
        m_regFuncp->slow(সত্য);
        m_regFuncp->isStatic(অসত্য);
        m_regFuncp->isLoose(সত্য);
        m_শীর্ষ_Scopep->addBlocksp(m_regFuncp);

        // Create the const dump functions. Also allocates trace codes.
        createConstTraceFunctions(traces);

        // Create the full and incremental dump functions
        createNonConstTraceFunctions(traces, nNonConstCodes, m_parallelism);

        // Root-traced libraries alias wrapper IOs onto the existing top-module codes.
        যদি (!v3Global.opt.libCreate().empty()) {
            std::vector<AstTraceDecl*> মূল_Declps;
            std::vector<AstTraceDecl*> শীর্ষ_Declps;
            TraceInitDeclCollector মূল_Collector{মূল_Declps};
            TraceInitDeclCollector শীর্ষ_Collector{শীর্ষ_Declps};
            প্রতি (Ast_গ্রন্থি* blockp = m_শীর্ষ_Scopep->blocksp(); blockp; blockp = blockp->nextp()) {
                AstCFunc* ধ্রুব funcp = VN_CAST(blockp, CFunc);
                যদি (funcp && VString::startsWith(funcp->name(), "trace_init_leaf_root__")) {
                    মূল_Collector.collect(funcp);
                } অন্যথা যদি (funcp && VString::startsWith(funcp->name(), "trace_init_leaf_top__")) {
                    শীর্ষ_Collector.collect(funcp);
                }
            }

            std::vector<তার্কিক> used(শীর্ষ_Declps.size(), অসত্য);
            প্রতি (AstTraceDecl* ধ্রুব মূল_Declp : মূল_Declps) {
                প্রতি (size_t i = 0; i < শীর্ষ_Declps.size(); ++i) {
                    যদি (used[i] || !same_মূল_Init_Alias(মূল_Declp, শীর্ষ_Declps[i])) অনুবর্ত;
                    মূল_Declp->code(শীর্ষ_Declps[i]->code());
                    used[i] = সত্য;
                    ভঙ্গ;
                }
            }
        }

        // Remove refs to traced values from TraceDecl nodes, these have now moved under
        // TraceInc
        প্রতি (ধ্রুব auto& i : traces) {
            Ast_গ্রন্থি* ধ্রুব valuep = i.second->nodep()->valuep();
            VL_DO_DANGLING(valuep->unlinkFrBack()->delete_বৃক্ষ(), valuep);
        }

        // Create the trace cleanup function clearing the activity flags
        createCleanupFunction();
    }

    TraceCFuncVertex* getCFuncVertexp(AstCFunc* nodep) {
        V3GraphVertex* ধ্রুব vtxp = nodep->user1u().toGraphVertex();
        TraceCFuncVertex* vertexp = vtxp ? vtxp->cast<TraceCFuncVertex>() : nullptr;
        যদি (!vertexp) {
            vertexp = নূতন TraceCFuncVertex{&m_graph, nodep};
            nodep->user1p(vertexp);
        }
        প্রত্যাগম vertexp;
    }
    TraceActivityVertex* getActivityVertexp(Ast_গ্রন্থি* nodep, তার্কিক slow) {
        V3GraphVertex* ধ্রুব vtxp = nodep->user3u().toGraphVertex();
        TraceActivityVertex* vertexp = vtxp ? vtxp->cast<TraceActivityVertex>() : nullptr;
        যদি (!vertexp) {
            vertexp = নূতন TraceActivityVertex{&m_graph, nodep, slow};
            nodep->user3p(vertexp);
        }
        vertexp->slow(slow);
        প্রত্যাগম vertexp;
    }

    // VISITORS
    শূন্য visit(AstNetlist* nodep) override {
        // Add vertexes for all TraceDecl, and edges from VARs each trace looks at
        m_finding = অসত্য;
        iterateChildren(nodep);

        // Add vertexes for all CFUNCs, and edges to VARs the func sets
        m_finding = সত্য;
        iterateChildren(nodep);

        // Create the trace functions and insert them into the tree
        createTraceFunctions();

        // Save number of trace codes used
        nodep->nTraceCodes(m_code);
    }
    শূন্য visit(Ast_গ্রন্থি_Module* nodep) override {
        যদি (nodep->is_শীর্ষ()) m_শীর্ষ_Modp = nodep;
        iterateChildren(nodep);
    }
    শূন্য visit(AstVarScope* nodep) override {
        যদি (!m_finding) {
            যদি (ধ্রুব AstIface* ধ্রুব ifacep = nodep->varp()->sensIfacep()) {
                m_ifaceMemberVscps[{ifacep, nodep->varp()->name()}].ঠেলো_back(nodep);
            }
        }
    }
    শূন্য visit(Ast_Stmt_রাশিমালা* nodep) override {
        যদি (!m_finding && !nodep->user2()) {
            যদি (AstCCall* ধ্রুব callp = VN_CAST(nodep->exprp(), CCall)) {
                UINFO(8, "   CCALL " << callp);
                // See if there are other calls in same statement list;
                // If so, all funcs might share the same activity code
                TraceActivityVertex* ধ্রুব activityVtxp
                    = getActivityVertexp(nodep, callp->funcp()->slow());
                প্রতি (Ast_গ্রন্থি* nextp = nodep; nextp; nextp = nextp->nextp()) {
                    যদি (Ast_Stmt_রাশিমালা* ধ্রুব stmtp = VN_CAST(nextp, Stmt_রাশিমালা)) {
                        যদি (AstCCall* ধ্রুব ccallp = VN_CAST(stmtp->exprp(), CCall)) {
                            stmtp->user2(সত্য);  // Processed
                            UINFO(8, "     SubCCALL " << ccallp);
                            V3GraphVertex* ধ্রুব ccallFuncVtxp = getCFuncVertexp(ccallp->funcp());
                            activityVtxp->slow(ccallp->funcp()->slow());
                            নূতন V3GraphEdge{&m_graph, activityVtxp, ccallFuncVtxp, 1};
                        }
                    }
                }
            }
        }
        iterateChildren(nodep);
    }
    শূন্য visit(AstCFunc* nodep) override {
        UINFO(8, "   CFUNC " << nodep);
        V3GraphVertex* ধ্রুব funcVtxp = getCFuncVertexp(nodep);
        যদি (!m_finding) {  // If public, we need a unique activity code to allow for sets
                           // directly in this func
            যদি (nodep->funcPublic() || nodep->dpiExportImpl() || nodep->entryPoint()
                || nodep->isCoroutine()) {
                // Cannot treat a coroutine as slow, it may be resumed later
                ধ্রুব তার্কিক slow = nodep->slow() && !nodep->isCoroutine();
                TraceActivityVertex* ধ্রুব activityVtxp = getActivityVertexp(nodep, slow);
                নূতন V3GraphEdge{&m_graph, activityVtxp, funcVtxp, 1};
            }
        }
        VL_RESTORER(m_cfuncp);
        m_cfuncp = nodep;
        iterateChildren(nodep);
    }
    শূন্য visit(AstTraceDecl* nodep) override {
        UINFO(8, "   TRACE " << nodep);
        যদি (!m_finding && !nodep->inDtypeFunc()) {
            // Skip decls inside trace_init_leaf_root__* functions: these are
            // duplicates of decls in trace_init_leaf_top__* (root vs regular
            // scope path) and should share the top decl's code via the
            // sameRootInitAlias post-pass at createTraceFunctions(). Putting
            // them in the dedup graph would allocate distinct trace codes for
            // them, growing nTraceCodes past what trace_init_top actually
            // declares -- so the parent's full callback writes to codes the
            // SAIF/VCD runtime never registered, tripping
            //   verilated_saif_c.cpp: Activity must be declared earlier
            //   verilated_vcd_c.cpp: finishLine: suffixp[0] failed
            // (cf. t_lib, t_trace_hier_block_*).
            যদি (m_cfuncp && VString::startsWith(m_cfuncp->name(), "trace_init_leaf_root__")) {
                প্রত্যাগম;
            }
            V3GraphVertex* ধ্রুব vertexp = নূতন TraceTraceVertex{&m_graph, nodep};
            nodep->user1p(vertexp);

            UASSERT_OBJ(m_cfuncp, nodep, "Trace not under func");
            m_cfuncp->user2(সত্য);
            VL_RESTORER(m_tracep);
            m_tracep = nodep;
            iterateChildren(nodep);
        }
    }
    শূন্য visit(AstVarRef* nodep) override {
        UASSERT_OBJ(nodep->varScopep(), nodep, "No var scope?");
        AstVarScope* ধ্রুব varscopep = nodep->varScopep();
        V3GraphVertex* varVtxp = varscopep->user1u().toGraphVertex();
        যদি (m_tracep) {
            UASSERT_OBJ(nodep->access().isReadOnly(), nodep, "Lvalue in trace?  Should be const.");
            যদি (!varVtxp) {
                varVtxp = নূতন TraceVarVertex{&m_graph, nodep->varScopep()};
                nodep->varScopep()->user1p(varVtxp);
            }
            V3GraphVertex* ধ্রুব traceVtxp = m_tracep->user1u().toGraphVertex();
            নূতন V3GraphEdge{&m_graph, varVtxp, traceVtxp, 1};
            যদি (nodep->varp()->isPrimaryInish()  // Always need to trace primary inputs
                || nodep->varp()->isSigPublic()) {  // Or ones user can change
                নূতন V3GraphEdge{&m_graph, m_alwaysVtxp, traceVtxp, 1};
            }
            যদি (m_tracep->dtypeCallp()) varscopep->user2p(m_tracep);
        } অন্যথা যদি (m_cfuncp && m_finding && nodep->access().isWriteOrRW()) {
            V3GraphVertex* ধ্রুব funcVtxp = getCFuncVertexp(m_cfuncp);
            যদি (varVtxp) {  // else we're not tracing this signal
                নূতন V3GraphEdge{&m_graph, funcVtxp, varVtxp, 1};
                AstTraceDecl* ধ্রুব declp = VN_AS(varscopep->user2p(), TraceDecl);
                যদি (declp) {
                    V3GraphVertex* ধ্রুব cFuncVtxp = getCFuncVertexp(m_cfuncp);
                    প্রতি (ধ্রুব V3GraphEdge& edge : cFuncVtxp->inEdges()) {
                        V3GraphVertex* ধ্রুব activityp = edge.fromp();
                        যদি (!declp->user3p()) {
                            declp->user3p(activityp);
                        } অন্যথা যদি (declp->user3u().toGraphVertex() != activityp) {
                            declp->user2(সত্য);
                        }
                    }
                }
            }
        }
    }
    শূন্য visit(AstMemberSel* nodep) override {
        যদি (m_cfuncp && m_finding && nodep->access().isWriteOrRW()) {
            AstIfaceRefDType* ধ্রুব dtypep
                = VN_CAST(nodep->fromp()->dtypep()->skipRefp(), IfaceRefDType);
            যদি (dtypep && dtypep->isVirtual()) {
                ধ্রুব auto it = m_ifaceMemberVscps.find({dtypep->ifacep(), nodep->varp()->name()});
                যদি (it != m_ifaceMemberVscps.end()) {
                    V3GraphVertex* ধ্রুব funcVtxp = getCFuncVertexp(m_cfuncp);
                    প্রতি (AstVarScope* ধ্রুব vscp : it->second) {
                        V3GraphVertex* varVtxp = vscp->user1u().toGraphVertex();
                        যদি (!varVtxp) {
                            varVtxp = নূতন TraceVarVertex{&m_graph, vscp};
                            vscp->user1p(varVtxp);
                        }
                        নূতন V3GraphEdge{&m_graph, funcVtxp, varVtxp, 1};
                    }
                }
            }
        }
        iterateChildren(nodep);
    }
    //--------------------
    শূন্য visit(Ast_গ্রন্থি* nodep) override { iterateChildren(nodep); }

সার্বজনিক:
    // CONSTRUCTORS
    explicit TraceVisitor(AstNetlist* nodep)
        : m_alwaysVtxp{নূতন TraceActivityVertex{&m_graph, TraceActivityVertex::ACTIVITY_ALWAYS}} {
        nodep->user2_Clear_বৃক্ষ();  // TraceDecl multiple activities flag
        nodep->user3_Clear_বৃক্ষ();  // TraceDecl TraceActivityVertex (assumes we start at nullptr)
        iterate(nodep);
        nodep->foreach([](AstTraceDecl* ধ্রুব declp) {
            যদি (declp->inDtypeFunc()) {
                declp->valuep()->unlinkFrBack()->delete_বৃক্ষ();
                declp->valuep(nullptr);
            }
        });
    }
    ~TraceVisitor() override {
        V3Stats::addStat("Tracing, Activity setters", m_statSetters);
        V3Stats::addStat("Tracing, Activity slow blocks", m_statSettersSlow);
        V3Stats::addStat("Tracing, Unique trace codes", m_code);
        V3Stats::addStat("Tracing, Unique traced signals", m_statUniqSigs);
    }
};

//######################################################################
// Trace class functions

শূন্য V3Trace::traceAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { TraceVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_বৃক্ষ("trace", 0, dump_বৃক্ষ_Either_Level() >= 3);
}
