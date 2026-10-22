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

#অন্তর্ভাৱ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#অন্তর্ভাৱ "V3Trace.h"

#অন্তর্ভাৱ "V3Ast.h"
#অন্তর্ভাৱ "V3DupFinder.h"
#অন্তর্ভাৱ "V3EmitCBase.h"
#অন্তর্ভাৱ "V3Graph.h"
#অন্তর্ভাৱ "V3Stats.h"
#অন্তর্ভাৱ "V3UniqueNames.h"

#অন্তর্ভাৱ <limits>
#অন্তর্ভাৱ <map>
#অন্তর্ভাৱ <set>
#অন্তর্ভাৱ <unordered_map>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Graph vertexes

শ্ৰেণী TraceActivityVertex final : সার্ৱজনিক V3GraphVertex {
    VL_RTTI_IMPL(TraceActivityVertex, V3GraphVertex)
    Ast_গ্ৰন্থি* ধ্রুৱ m_insertp;
    int32_t m_activityCode;
    তার্কিক m_slow;  // If always slow, we can use the same code
সার্ৱজনিক:
    গণনা { ACTIVITY_NEVER = ((1UL << 31) - 1) };
    গণনা { ACTIVITY_ALWAYS = ((1UL << 31) - 2) };
    গণনা { ACTIVITY_SLOW = 0 };
    TraceActivityVertex(V3Graph* graphp, Ast_গ্ৰন্থি* nodep, তার্কিক slow)
        : V3GraphVertex{graphp}
        , m_insertp{nodep} {
        m_activityCode = 0;
        m_slow = slow;
    }
    TraceActivityVertex(V3Graph* graphp, int32_t code)
        : V3GraphVertex{graphp}
        , m_insertp{nullptr} {
        m_activityCode = code;
        m_slow = মিছা;
    }
    ~TraceActivityVertex() override = স্ৱভাৱ;
    // ACCESSORS
    Ast_গ্ৰন্থি* insertp() ধ্রুৱ {
        UASSERT(m_insertp, "Null insertp; probably called on a special always/slow");
        ঘূৰাই m_insertp;
    }
    string name() ধ্রুৱ override {
        যদি (activityAlways()) {
            ঘূৰাই "*ALWAYS*";
        } অন্যথা {
            ঘূৰাই std::string{slow() ? "*SLOW* " : ""} + insertp()->name();
        }
    }
    string dotColor() ধ্রুৱ override { ঘূৰাই slow() ? "yellowGreen" : "green"; }
    int32_t activityCode() ধ্রুৱ { ঘূৰাই m_activityCode; }
    তার্কিক activityAlways() ধ্রুৱ { ঘূৰাই activityCode() == ACTIVITY_ALWAYS; }
    তার্কিক activitySlow() ধ্রুৱ { ঘূৰাই activityCode() == ACTIVITY_SLOW; }
    শূন্য activityCode(int32_t code) { m_activityCode = code; }
    তার্কিক slow() ধ্রুৱ { ঘূৰাই m_slow; }
    শূন্য slow(তার্কিক flag) {
        যদি (!flag) m_slow = মিছা;
    }
};

শ্ৰেণী TraceCFuncVertex final : সার্ৱজনিক V3GraphVertex {
    VL_RTTI_IMPL(TraceCFuncVertex, V3GraphVertex)
    AstCFunc* ধ্রুৱ m_nodep;

সার্ৱজনিক:
    TraceCFuncVertex(V3Graph* graphp, AstCFunc* nodep)
        : V3GraphVertex{graphp}
        , m_nodep{nodep} {}
    ~TraceCFuncVertex() override = স্ৱভাৱ;
    // ACCESSORS
    AstCFunc* nodep() ধ্রুৱ { ঘূৰাই m_nodep; }
    string name() ধ্রুৱ override { ঘূৰাই nodep()->name(); }
    string dotColor() ধ্রুৱ override { ঘূৰাই "yellow"; }
    File_সাৰি* fileline() ধ্রুৱ override { ঘূৰাই nodep()->fileline(); }
};

শ্ৰেণী TraceTraceVertex final : সার্ৱজনিক V3GraphVertex {
    VL_RTTI_IMPL(TraceTraceVertex, V3GraphVertex)
    AstTraceDecl* ধ্রুৱ m_nodep;  // TRACEINC this represents
    // nullptr, or other vertex with the real code() that duplicates this one
    TraceTraceVertex* m_duplicatep = nullptr;
    // When aliasing to a dtype parent, offset of the member within the dtype
    uint32_t m_dtypeAliasOffset = 0;

সার্ৱজনিক:
    TraceTraceVertex(V3Graph* graphp, AstTraceDecl* nodep)
        : V3GraphVertex{graphp}
        , m_nodep{nodep} {}
    ~TraceTraceVertex() override = স্ৱভাৱ;
    // ACCESSORS
    AstTraceDecl* nodep() ধ্রুৱ { ঘূৰাই m_nodep; }
    string name() ধ্রুৱ override { ঘূৰাই nodep()->name(); }
    string dotColor() ধ্রুৱ override { ঘূৰাই "red"; }
    File_সাৰি* fileline() ধ্রুৱ override { ঘূৰাই nodep()->fileline(); }
    TraceTraceVertex* duplicatep() ধ্রুৱ { ঘূৰাই m_duplicatep; }
    শূন্য duplicatep(TraceTraceVertex* dupp) {
        UASSERT_OBJ(!duplicatep(), nodep(), "Assigning duplicatep() to already duplicated node");
        m_duplicatep = dupp;
    }
    শূন্য redirectDuplicatep(TraceTraceVertex* dupp) { m_duplicatep = dupp; }
    uint32_t dtypeAliasOffset() ধ্রুৱ { ঘূৰাই m_dtypeAliasOffset; }
    শূন্য dtypeAliasOffset(uint32_t offset) { m_dtypeAliasOffset = offset; }
};

শ্ৰেণী TraceVarVertex final : সার্ৱজনিক V3GraphVertex {
    VL_RTTI_IMPL(TraceVarVertex, V3GraphVertex)
    AstVarScope* ধ্রুৱ m_nodep;

সার্ৱজনিক:
    TraceVarVertex(V3Graph* graphp, AstVarScope* nodep)
        : V3GraphVertex{graphp}
        , m_nodep{nodep} {}
    ~TraceVarVertex() override = স্ৱভাৱ;
    // ACCESSORS
    AstVarScope* nodep() ধ্রুৱ { ঘূৰাই m_nodep; }
    string name() ধ্রুৱ override { ঘূৰাই nodep()->name(); }
    string dotColor() ধ্রুৱ override { ঘূৰাই "skyblue"; }
    File_সাৰি* fileline() ধ্রুৱ override { ঘূৰাই nodep()->fileline(); }
};

//######################################################################
// Trace state, as a visitor of each AstNode

শ্ৰেণী TraceVisitor final : সার্ৱজনিক VNVisitor {
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
    ধ্রুৱ VNUser1InUse m_inuser1;
    ধ্রুৱ VNUser2InUse m_inuser2;
    ধ্রুৱ VNUser3InUse m_inuser3;
    // VNUser4InUse     In V3Hasher via V3DupFinder

    // STATE
    Ast_গ্ৰন্থি_Module* m_শীৰ্ষ_Modp = nullptr;  // Module to add variables to
    AstScope* ধ্রুৱ m_শীৰ্ষ_Scopep = v3Global.rootp()->শীৰ্ষ_Scopep()->scopep();  // The top AstScope
    AstCFunc* m_cfuncp = nullptr;  // C function adding to graph
    AstCFunc* m_regFuncp = nullptr;  // Trace registration function
    AstCFunc* m_actAllFuncp = nullptr;  // Set all activity function
    AstTraceDecl* m_tracep = nullptr;  // Trace function adding to graph
    AstVarScope* m_activityVscp = nullptr;  // Activity variable
    uint32_t m_activity_সংখ্যা = 0;  // Count of fields in activity variable
    uint32_t m_code = 0;  // Trace ident code# being assigned
    V3Graph m_graph;  // Var/CFunc tracking
    TraceActivityVertex* ধ্রুৱ m_alwaysVtxp;  // "Always trace" vertex
    তার্কিক m_finding = মিছা;  // Pass one of algorithm?
    সংরচনা DtypeFuncs final {
    সার্ৱজনিক:
        AstCFunc* fullFuncp = nullptr;
        AstCFunc* chgFuncp = nullptr;
    };
    std::unordered_map<ধ্রুৱ Ast_গ্ৰন্থি_DType*, DtypeFuncs>
        m_dtypeNonConstFuncs;  // Full / Chg funcs per type
    std::unordered_map<ধ্রুৱ Ast_গ্ৰন্থি_DType*, AstCFunc*> m_dtypeConstFuncs;  // Const func per type
    V3UniqueNames m_dtypeNames{""};  // Unique type func names

    // Trace parallelism. Only VCD tracing can be parallelized at this time.
    ধ্রুৱ uint32_t m_parallelism
        = v3Global.opt.useTraceParallel() ? static_cast<uint32_t>(v3Global.opt.threads()) : 1;

    VDouble0 m_statSetters;  // Statistic tracking
    VDouble0 m_statSettersSlow;  // Statistic tracking
    VDouble0 m_statUniqSigs;  // Statistic tracking

    // All activity numbers applying to a given trace
    using ActCodeSet = std::set<uint32_t>;
    // For activity set, what traces apply
    using TraceVec = std::multimap<ActCodeSet, TraceTraceVertex*>;
    // Candidate interface-member VarScopes keyed by (interface type, member name)
    std::map<std::pair<ধ্রুৱ AstIface*, std::string>, std::vector<AstVarScope*>>
        m_ifaceMemberVscps;

    শ্ৰেণী TraceInitDeclCollector final : সার্ৱজনিক VNVisitor {
        std::vector<AstTraceDecl*>& m_declps;  // Output: trace declarations found so far
        std::set<ধ্রুৱ AstCFunc*> m_seenFuncps;

        শূন্য visit(AstTraceDecl* nodep) override { m_declps.ঠেলো_back(nodep); }
        শূন্য visit(AstCCall* nodep) override {
            যদি (AstCFunc* ধ্রুৱ funcp = nodep->funcp()) collect(funcp);
        }
        শূন্য visit(Ast_গ্ৰন্থি* nodep) override { iterateChildren(nodep); }

    সার্ৱজনিক:
        explicit TraceInitDeclCollector(std::vector<AstTraceDecl*>& declps)
            : m_declps{declps} {}
        শূন্য collect(AstCFunc* funcp) {
            যদি (funcp && m_seenFuncps.ঢোকাও(funcp).second) iterate(funcp);
        }
    };

    // METHODS

    স্থির তার্কিক same_মূল_Init_Alias(ধ্রুৱ AstTraceDecl* মূল_Declp, ধ্রুৱ AstTraceDecl* শীৰ্ষ_Declp) {
        ধ্রুৱ VNumRange& মূল_Bit_Range = মূল_Declp->bitRange();
        ধ্রুৱ VNumRange& শীৰ্ষ_Bit_Range = শীৰ্ষ_Declp->bitRange();
        ঘূৰাই মূল_Declp->showname() == শীৰ্ষ_Declp->showname()
               && মূল_Declp->declDirection() == শীৰ্ষ_Declp->declDirection()
               && মূল_Declp->widthMin() == শীৰ্ষ_Declp->widthMin()
               && মূল_Bit_Range.ranged() == শীৰ্ষ_Bit_Range.ranged()
               && (!মূল_Bit_Range.ranged()
                   || (মূল_Bit_Range.বাম() == শীৰ্ষ_Bit_Range.বাম()
                       && মূল_Bit_Range.ডান() == শীৰ্ষ_Bit_Range.ডান()));
    }

    শূন্য detectDuplicates() {
        UINFO(9, "Finding duplicates");
        // Note uses user4
        V3DupFinder dupFinder;  // Duplicate code detection

        // Compute member offsets within dtype instances incrementally.
        // For each dtype member TraceDecl, stores its code offset within the
        // dtype parent's code range.
        std::unordered_map<ধ্রুৱ AstTraceDecl*, uint32_t> memberOffsets;
        std::unordered_map<ধ্রুৱ AstTraceDecl*, uint32_t> runningOffset;

        // For fixup pass: track dups redirected to dtype parents, so we can
        // fix them if the parent's user2 is later set during the loop.
        // Maps dup vertex -> original canonical member vertex.
        std::vector<std::pair<TraceTraceVertex*, TraceTraceVertex*>> dtypeParentRedirects;

        // Hash all of the traced values and find if there are any duplicates
        বাবে (V3GraphVertex& vtx : m_graph.vertices()) {
            যদি (TraceTraceVertex* ধ্রুৱ vvertexp = vtx.cast<TraceTraceVertex>()) {
                AstTraceDecl* ধ্রুৱ nodep = vvertexp->nodep();
                যদি (nodep->dtypeCallp()) অনুৱর্ত;

                যদি (nodep->dtypeDeclp()) {
                    uint32_t& offset = runningOffset[nodep->dtypeDeclp()];
                    memberOffsets[nodep] = offset;
                    offset += nodep->codeInc();
                }

                UASSERT_OBJ(!vvertexp->duplicatep(), nodep, "Should not be a duplicate");
                ধ্রুৱ auto dupit = dupFinder.findDuplicate(nodep);
                যদি (dupit == dupFinder.end()) {
                    dupFinder.ঢোকাও(nodep);
                } অন্যথা {
                    AstTraceDecl* ধ্রুৱ dupDeclp = VN_AS(dupit->second, TraceDecl);
                    UASSERT_OBJ(dupDeclp, nodep, "Trace duplicate of wrong type");
                    TraceTraceVertex* ধ্রুৱ dupvertexp
                        = dupDeclp->user1u().toGraphVertex()->cast<TraceTraceVertex>();
                    UINFO(8, "  Orig " << dupDeclp << endl);
                    UINFO(8, "   dup " << nodep << endl);

                    যদি (dupDeclp->dtypeDeclp() && !dupDeclp->dtypeDeclp()->user2()) {
                        AstTraceDecl* ধ্রুৱ dtypeParentp = dupDeclp->dtypeDeclp();
                        TraceTraceVertex* ধ্রুৱ dtypeVtxp
                            = dtypeParentp->user1u().toGraphVertex()->cast<TraceTraceVertex>();
                        ধ্রুৱ auto it2 = memberOffsets.find(dupDeclp);
                        UASSERT_OBJ(it2 != memberOffsets.end(), dupDeclp,
                                    "Member offset not precomputed");
                        vvertexp->duplicatep(dtypeVtxp);
                        vvertexp->dtypeAliasOffset(it2->second);
                        dtypeParentRedirects.emplace_back(vvertexp, dupvertexp);
                    } অন্যথা {
                        vvertexp->duplicatep(dupvertexp);
                    }
                    যদি (nodep->dtypeDeclp()) nodep->dtypeDeclp()->user2(সঁচা);
                }
            }
        }

        বাবে (ধ্রুৱ auto& pair : dtypeParentRedirects) {
            TraceTraceVertex* ধ্রুৱ dupVtxp = pair.first;
            TraceTraceVertex* ধ্রুৱ memberVtxp = pair.second;
            ধ্রুৱ AstTraceDecl* ধ্রুৱ parentDeclp = dupVtxp->duplicatep()->nodep();
            যদি (parentDeclp->user2()) {
                dupVtxp->redirectDuplicatep(memberVtxp);
                dupVtxp->dtypeAliasOffset(0);
            }
        }

        যদি (dumpLevel() || debug() >= 9)
            dupFinder.dumpFile(v3Global.debugFilename("trace") + ".hash", মিছা);
    }

    শূন্য graphDtypePrune() {
        বাবে (V3GraphVertex* ধ্রুৱ vtxp : m_graph.vertices().unlinkable()) {
            যদি (TraceTraceVertex* ধ্রুৱ vvertexp = vtxp->cast<TraceTraceVertex>()) {
                AstTraceDecl* ধ্রুৱ declp = vvertexp->nodep();
                // This skips the dtype sub-func optimization if a var is affected by multiple
                // activities.  We really only need to do this for _chg funcs (and not decls,
                // _const and _full) but it's simpiler to do it all one way or the other.
                যদি (declp) {
                    যদি (declp->user2() || (declp->dtypeDeclp() && !declp->dtypeDeclp()->user2())) {
                        AstCCall* ধ্রুৱ callp = declp->dtypeCallp();
                        যদি (callp) {
                            Ast_গ্ৰন্থি* stmtexprp = callp->backp();
                            VL_DO_DANGLING(ঠেলো_Deletep(stmtexprp->unlinkFrBack()), stmtexprp);
                        } অন্যথা {
                            তার্কিক emptyScope;
                            কৰা {
                                emptyScope = মিছা;
                                Ast_গ্ৰন্থি* ধ্রুৱ declBackp = declp->backp();
                                Ast_গ্ৰন্থি* ধ্রুৱ declNextp = declp->nextp();
                                যদি (VN_IS(declBackp, Trace_ঠেলো_Prefix)
                                    && VN_IS(declNextp, Trace_বেৰ_কৰো_Prefix)) {
                                    VL_DO_DANGLING(ঠেলো_Deletep(declBackp->unlinkFrBack()),
                                                   declBackp);
                                    VL_DO_DANGLING(ঠেলো_Deletep(declNextp->unlinkFrBack()),
                                                   declNextp);
                                    emptyScope = সঁচা;
                                }
                            } যেতিয়ালৈ (emptyScope);
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
        ধ্রুৱ পূর্ণাঙ্ক splitLimit = v3Global.opt.outputSplitCTrace() ? v3Global.opt.outputSplitCTrace()
                                                                : std::numeric_limits<পূর্ণাঙ্ক>::max();
        যদি (splitLimit == std::numeric_limits<পূর্ণাঙ্ক>::max()) ঘূৰাই;

        বাবে (Ast_গ্ৰন্থি* nodep = m_শীৰ্ষ_Scopep->blocksp(); nodep; nodep = nodep->nextp()) {
            AstCFunc* ধ্রুৱ funcp = VN_CAST(nodep, CFunc);
            যদি (!funcp || !funcp->user2()) অনুৱর্ত;

            ধ্রুৱ string baseName = funcp->name();
            uint32_t sub_অঙ্ক = 1;
            পূর্ণাঙ্ক curSize = 0;
            AstCFunc* curFuncp = funcp;
            std::vector<Ast_গ্ৰন্থি_Stmt*> callStmts;

            auto startNewSubFunc = [&]() {
                File_সাৰি* ধ্রুৱ flp = funcp->fileline();
                ধ্রুৱ string newName = baseName + "_" + cvtToStr(sub_অঙ্ক++);
                AstCFunc* ধ্রুৱ newFuncp = নূতন AstCFunc{flp, newName, m_শীৰ্ষ_Scopep};
                newFuncp->argTypes(v3Global.opt.traceClassBase() + "* tracep");
                newFuncp->isTrace(সঁচা);
                newFuncp->isStatic(মিছা);
                newFuncp->isLoose(সঁচা);
                newFuncp->slow(সঁচা);
                newFuncp->dontCombine(সঁচা);
                m_শীৰ্ষ_Scopep->addBlocksp(newFuncp);
                newFuncp->addStmtsp(নূতন AstCStmt{flp, "const int c = vlSymsp->__Vm_baseCode;"});
                AstCCall* ধ্রুৱ callp = নূতন AstCCall{flp, newFuncp};
                callp->dtypeSetVoid();
                callp->argTypes("tracep");
                callStmts.ঠেলো_back(callp->makeStmt());
                curFuncp = newFuncp;
                curSize = 0;
            };

            বাবে (Ast_গ্ৰন্থি *stmtp = funcp->stmtsp(), *nextp; stmtp; stmtp = nextp) {
                nextp = stmtp->nextp();
                curSize += stmtp->গ্ৰন্থি_গুনতি();

                যদি (curFuncp != funcp) {
                    stmtp->unlinkFrBack();
                    curFuncp->addStmtsp(stmtp);
                }

                যদি (curSize > splitLimit && nextp) startNewSubFunc();
            }

            বাবে (Ast_গ্ৰন্থি_Stmt* ধ্রুৱ callStmtp : callStmts) { funcp->addStmtsp(callStmtp); }
        }
    }

    শূন্য graphSimplify(তার্কিক initial) {
        যদি (initial) {
            // Remove all variable nodes
            বাবে (V3GraphVertex* ধ্রুৱ vtxp : m_graph.vertices().unlinkable()) {
                যদি (TraceVarVertex* ধ্রুৱ vvertexp = vtxp->cast<TraceVarVertex>()) {
                    vvertexp->rerouteEdges(&m_graph);
                    vvertexp->unlinkDelete(&m_graph);
                }
            }
            // Remove multiple variables connecting funcs to traces
            // We do this twice, as then we have fewer edges to multiply out in the below
            // expansion.
            m_graph.removeRedundantEdgesMax(&V3GraphEdge::followAlwaysTrue);
            // Remove all Cfunc nodes
            বাবে (V3GraphVertex* ধ্রুৱ vtxp : m_graph.vertices().unlinkable()) {
                যদি (TraceCFuncVertex* ধ্রুৱ vvertexp = vtxp->cast<TraceCFuncVertex>()) {
                    vvertexp->rerouteEdges(&m_graph);
                    vvertexp->unlinkDelete(&m_graph);
                }
            }
        }

        // Remove multiple variables connecting funcs to traces
        m_graph.removeRedundantEdgesMax(&V3GraphEdge::followAlwaysTrue);

        // If there are any edges from a always, keep only the always
        বাবে (V3GraphVertex& vtx : m_graph.vertices()) {
            যদি (TraceTraceVertex* ধ্রুৱ vvertexp = vtx.cast<TraceTraceVertex>()) {
                // Search for the incoming always edge
                ধ্রুৱ V3GraphEdge* alwaysEdgep = nullptr;
                বাবে (ধ্রুৱ V3GraphEdge& edge : vvertexp->inEdges()) {
                    ধ্রুৱ TraceActivityVertex* ধ্রুৱ actVtxp
                        = edge.fromp()->as<ধ্রুৱ TraceActivityVertex>();
                    যদি (actVtxp->activityAlways()) {
                        alwaysEdgep = &edge;
                        ভঙ্গ;
                    }
                }
                // If always edge exists, remove all other edges
                যদি (alwaysEdgep) {
                    বাবে (V3GraphEdge* ধ্রুৱ edgep : vvertexp->inEdges().unlinkable()) {
                        যদি (edgep != alwaysEdgep) VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
                    }
                }
            }
        }

        // Activity points with no outputs can be removed
        বাবে (V3GraphVertex* ধ্রুৱ vtxp : m_graph.vertices().unlinkable()) {
            যদি (TraceActivityVertex* ধ্রুৱ aVtxp = vtxp->cast<TraceActivityVertex>()) {
                // Leave in the always vertex for later use.
                যদি (aVtxp != m_alwaysVtxp && aVtxp->outEmpty()) {
                    VL_DO_DANGLING(aVtxp->unlinkDelete(&m_graph), aVtxp);
                }
            }
        }
    }

    uint32_t assignactivityNumbers() {
        uint32_t activity_সংখ্যা = 1;  // Note 0 indicates "slow" only
        বাবে (V3GraphVertex& vtx : m_graph.vertices()) {
            যদি (TraceActivityVertex* ধ্রুৱ vvertexp = vtx.cast<TraceActivityVertex>()) {
                যদি (vvertexp != m_alwaysVtxp) {
                    যদি (vvertexp->slow()) {
                        vvertexp->activityCode(TraceActivityVertex::ACTIVITY_SLOW);
                    } অন্যথা {
                        vvertexp->activityCode(activity_সংখ্যা++);
                    }
                }
            }
        }
        ঘূৰাই activity_সংখ্যা;
    }

    শূন্য sortTraces(TraceVec& traces, uint32_t& nNonConstCodes) {
        // Populate sort structure
        traces.clear();
        nNonConstCodes = 0;
        বাবে (V3GraphVertex& vtx : m_graph.vertices()) {
            যদি (TraceTraceVertex* ধ্রুৱ vtxp = vtx.cast<TraceTraceVertex>()) {
                ActCodeSet actSet;
                UINFO(9, "  Add to sort: " << vtxp);
                UINFOTREE(9, vtxp->nodep(), "", "trnode");
                বাবে (ধ্রুৱ V3GraphEdge& edge : vtxp->inEdges()) {
                    ধ্রুৱ TraceActivityVertex* ধ্রুৱ cfvertexp
                        = edge.fromp()->cast<ধ্রুৱ TraceActivityVertex>();
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
        ধ্রুৱ auto end = traces.end();
        যেতিয়ালৈ (it != end) {
            auto head = it;
            // Approximate the complexity of the value change check
            uint32_t complexity = 0;
            ধ্রুৱ ActCodeSet& actSet = it->first;
            বাবে (; it != end && it->first == actSet; ++it) {
                যদি (!it->second->duplicatep()) {
                    uint32_t cost = 0;
                    ধ্রুৱ AstTraceDecl* ধ্রুৱ declp = it->second->nodep();
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
            যদি (actSet.গুনতি(TraceActivityVertex::ACTIVITY_ALWAYS)) অনুৱর্ত;
            যদি (actSet.গুনতি(TraceActivityVertex::ACTIVITY_NEVER)) অনুৱর্ত;
            যদি (actSet.গুনতি(TraceActivityVertex::ACTIVITY_SLOW)) অনুৱর্ত;
            // If the value comparisons are cheaper to perform than checking the
            // activity flags make the signals always traced. Note this cost
            // equation is heuristic.
            যদি (complexity <= actSet.size() * 2) {
                বাবে (; head != it; ++head) {
                    নূতন V3GraphEdge{&m_graph, m_alwaysVtxp, head->second, 1};
                }
            }
        }

        graphSimplify(মিছা);
    }

    Ast_গ্ৰন্থি_ৰাশিমালা* selectActivity(File_সাৰি* flp, uint32_t acode, ধ্রুৱ VAccess& access) {
        ঘূৰাই নূতন AstArraySel(flp, নূতন AstVarRef{flp, m_activityVscp, access}, acode);
    }

    Ast_গ্ৰন্থি* newActivitySetter(Ast_গ্ৰন্থি* insertp, uint32_t code) {
        ++m_statSetters;
        File_সাৰি* ধ্রুৱ fl = insertp->fileline();
        AstAssign* ধ্রুৱ setterp = নূতন AstAssign{fl, selectActivity(fl, code, VAccess::WRITE),
                                                 নূতন AstConst{fl, AstConst::BitTrue{}}};
        ঘূৰাই setterp;
    }

    Ast_গ্ৰন্থি* newActivityAll(Ast_গ্ৰন্থি* insertp) {
        ++m_statSettersSlow;
        যদি (!m_actAllFuncp) {
            File_সাৰি* ধ্রুৱ flp = m_শীৰ্ষ_Scopep->fileline();
            AstCFunc* ধ্রুৱ funcp = নূতন AstCFunc{flp, "__VtraceActivitySetAll", m_শীৰ্ষ_Scopep};
            funcp->slow(সঁচা);
            funcp->isStatic(মিছা);
            funcp->isLoose(সঁচা);
            m_শীৰ্ষ_Scopep->addBlocksp(funcp);
            বাবে (uint32_t code = 0; code < m_activity_সংখ্যা; ++code) {
                Ast_গ্ৰন্থি* ধ্রুৱ setterp = newActivitySetter(insertp, code);
                funcp->addStmtsp(setterp);
            }
            m_actAllFuncp = funcp;
        }
        AstCCall* ধ্রুৱ callp = নূতন AstCCall{insertp->fileline(), m_actAllFuncp};
        callp->dtypeSetVoid();
        ঘূৰাই callp->makeStmt();
    }

    শূন্য createActivityFlags() {
        // Assign final activity numbers
        m_activity_সংখ্যা = assignactivityNumbers();

        // Create an array of bytes, not a bit vector, as they can be set
        // atomically by mtasks, and are cheaper to set (no need for
        // read-modify-write on the C type), and the speed of the tracing code
        // is the same on largish designs.
        File_সাৰি* ধ্রুৱ flp = m_শীৰ্ষ_Scopep->fileline();
        Ast_গ্ৰন্থি_DType* ধ্রুৱ newScalarDtp = নূতন AstBasicDType{flp, VFlagBitPacked{}, 1};
        v3Global.rootp()->typeTablep()->addTypesp(newScalarDtp);
        AstRange* ধ্রুৱ newArange
            = নূতন AstRange{flp, VNumRange{static_cast<পূর্ণাঙ্ক>(m_activity_সংখ্যা) - 1, 0}};
        Ast_গ্ৰন্থি_DType* ধ্রুৱ newArrDtp = নূতন AstUnpackArrayDType{flp, newScalarDtp, newArange};
        v3Global.rootp()->typeTablep()->addTypesp(newArrDtp);
        m_activityVscp = v3Global.rootp()->শীৰ্ষ_Scopep()->createTemp("__VtraceActivity", newArrDtp);

        // Insert activity setters
        বাবে (ধ্রুৱ V3GraphVertex& vtx : m_graph.vertices()) {
            যদি (ধ্রুৱ TraceActivityVertex* ধ্রুৱ vtxp = vtx.cast<ধ্রুৱ TraceActivityVertex>()) {
                Ast_গ্ৰন্থি* setterp = nullptr;
                যদি (vtxp->activitySlow()) {
                    setterp = newActivityAll(vtxp->insertp());
                    // Just set all flags in slow code as it should be rare.
                    // This will be rolled up into a loop by V3Reloop.
                } অন্যথা যদি (!vtxp->activityAlways()) {
                    setterp = newActivitySetter(vtxp->insertp(), vtxp->activityCode());
                }
                যদি (setterp) {
                    Ast_গ্ৰন্থি* ধ্রুৱ insertp = vtxp->insertp();
                    যদি (Ast_Stmt_ৰাশিমালা* ধ্রুৱ stmtp = VN_CAST(insertp, Stmt_ৰাশিমালা)) {
                        stmtp->addNextHere(setterp);
                    } অন্যথা যদি (AstCFunc* ধ্রুৱ funcp = VN_CAST(insertp, CFunc)) {
                        // If there are awaits, insert the setter after each await
                        যদি (funcp->isCoroutine() && funcp->stmtsp()) {
                            funcp->stmtsp()->foreachAndNext([setterp](AstCAwait* awaitp) {
                                awaitp->addNextHere(setterp->clone_বৃক্ষ(মিছা));
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

    AstCFunc* newCFunc(VTraceType traceType, AstCFunc* শীৰ্ষ_Funcp, uint32_t func_অঙ্ক,
                       uint32_t baseCode = 0, ধ্রুৱ AstTraceDecl* ধ্রুৱ declp = nullptr,
                       তার্কিক declSub = মিছা) {
        // Create new function
        ধ্রুৱ তার্কিক is_শীৰ্ষ_Func = !declp && শীৰ্ষ_Funcp == nullptr;
        std::string funcName;
        যদি (is_শীৰ্ষ_Func || declp) {
            funcName = traceType.func_prefix();
        } অন্যথা {
            funcName = শীৰ্ষ_Funcp->name();
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

        File_সাৰি* ধ্রুৱ flp = m_শীৰ্ষ_Scopep->fileline();
        AstCFunc* ধ্রুৱ funcp = নূতন AstCFunc{flp, funcName, m_শীৰ্ষ_Scopep};
        funcp->isTrace(সঁচা);
        funcp->dontCombine(সঁচা);
        funcp->isLoose(সঁচা);
        funcp->slow(traceType != VTraceType::CHANGE);
        funcp->isStatic(is_শীৰ্ষ_Func);
        // Add it to top scope
        m_শীৰ্ষ_Scopep->addBlocksp(funcp);
        ধ্রুৱ std::string bufArg
            = v3Global.opt.traceClassBase() + "::Buffer* bufp"
              + (declp ? (", uint32_t offset, const " + declp->dtypep()->cType("", সঁচা, সঁচা)
                          + " __VdtypeVar")
                       : "");
        যদি (is_শীৰ্ষ_Func) {
            // Top functions
            funcp->argTypes("void* voidSelf, " + bufArg);
            funcp->addStmtsp(নূতন AstCStmt{flp, EmitCUtil::voidSelfAssign(m_শীৰ্ষ_Modp)});
            funcp->addStmtsp(নূতন AstCStmt{flp, EmitCUtil::symClassAssign()});
            // Add global activity check to change dump functions
            যদি (traceType == VTraceType::CHANGE) {  //
                funcp->addStmtsp(
                    নূতন AstCStmt{flp, "if (VL_UNLIKELY(!vlSymsp->__Vm_activity)) return;"});
            }
            // Register function
            AstCStmt* ধ্রুৱ cstmtp = নূতন AstCStmt{flp};
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
                AstCCall* ধ্রুৱ callp = নূতন AstCCall{funcp->fileline(), funcp};
                callp->dtypeSetVoid();
                callp->argTypes("bufp");
                শীৰ্ষ_Funcp->addStmtsp(callp->makeStmt());
            }
        }
        // Done
        UINFO(5, "  newCFunc " << funcp);
        ঘূৰাই funcp;
    }

    AstCFunc* newCDtypeSubFunc(VTraceType traceType, ধ্রুৱ AstTraceDecl* ধ্রুৱ declp,
                               AstCFunc* parentp) {
        AstCFunc* ধ্রুৱ funcp = newCFunc(traceType, nullptr, 0, 0, declp, সঁচা);
        File_সাৰি* ধ্রুৱ flp = m_শীৰ্ষ_Scopep->fileline();
        AstCCall* ধ্রুৱ callp = নূতন AstCCall{flp, funcp};
        callp->dtypeSetVoid();
        callp->argTypes("bufp, offset, __VdtypeVar");
        parentp->addStmtsp(callp->makeStmt());
        ঘূৰাই funcp;
    }

    AstCFunc* createConstDtypeTraceFunctions(ধ্রুৱ AstTraceDecl* declp) {
        ধ্রুৱ Ast_গ্ৰন্থি_DType* ধ্রুৱ dtypep = declp->dtypep()->skipRefp();
        auto pair = m_dtypeConstFuncs.emplace(dtypep, nullptr);
        যদি (pair.second) {
            File_সাৰি* ধ্রুৱ flp = declp->fileline();
            AstCFunc* ধ্রুৱ funcp = newCFunc(VTraceType::CONSTANT, nullptr, 0, 0, declp);

            তার্কিক first = সঁচা;
            বাবে (Ast_গ্ৰন্থি* callStmtp = declp->dtypeCallp()->funcp()->stmtsp(); callStmtp;
                 callStmtp = callStmtp->nextp()) {
                ধ্রুৱ AstCCall* ধ্রুৱ callp
                    = VN_CAST(VN_CAST(callStmtp, Stmt_ৰাশিমালা)->exprp(), CCall);
                তার্কিক onlyOne = first && !callStmtp->nextp();
                AstCFunc* ধ্রুৱ subFuncp
                    = onlyOne ? funcp : newCDtypeSubFunc(VTraceType::CONSTANT, declp, funcp);
                বাবে (Ast_গ্ৰন্থি* stmtp = callp->funcp()->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    যদি (AstTraceDecl* ধ্রুৱ fieldDeclp = VN_CAST(stmtp, TraceDecl)) {
                        AstTraceInc* ধ্রুৱ incp
                            = নূতন AstTraceInc{flp, fieldDeclp, VTraceType::CONSTANT};
                        subFuncp->addStmtsp(incp);
                    }
                }
                first = মিছা;
            }

            pair.first->second = funcp;
        }

        ঘূৰাই pair.first->second;
    }

    শূন্য createConstTraceFunctions(ধ্রুৱ TraceVec& traces) {
        ধ্রুৱ পূর্ণাঙ্ক splitLimit = v3Global.opt.outputSplitCTrace() ? v3Global.opt.outputSplitCTrace()
                                                                : std::numeric_limits<পূর্ণাঙ্ক>::max();

        AstCFunc* ধ্রুৱ শীৰ্ষ_Funcp = newCFunc(VTraceType::CONSTANT, nullptr, 0);
        uint32_t sub_Func_অঙ্ক = 0;
        AstCFunc* subFuncp = nullptr;
        পূর্ণাঙ্ক subStmts = 0;
        std::vector<ধ্রুৱ TraceTraceVertex*> duplicates;
        বাবে (auto it = traces.cbegin(); it != traces.end(); ++it) {
            ধ্রুৱ TraceTraceVertex* ধ্রুৱ vtxp = it->second;
            AstTraceDecl* ধ্রুৱ declp = vtxp->nodep();
            যদি (vtxp->duplicatep()) {
                duplicates.ঠেলো_back(vtxp);
                অনুৱর্ত;
            }

            // This is a canonical trace node. Assign trace code (signal number).
            UASSERT_OBJ(!declp->codeAssigned(), declp,
                        "Canonical node should not have code assigned yet");
            declp->code(m_code);
            ধ্রুৱ uint32_t codeInc = declp->codeInc();
            m_code += codeInc;
            ++m_statUniqSigs;

            // If this is a const signal, add the AstTraceInc
            ধ্রুৱ ActCodeSet& actSet = it->first;
            যদি (actSet.গুনতি(TraceActivityVertex::ACTIVITY_NEVER)) {
                // Crate new sub function if required
                যদি (!subFuncp || subStmts > splitLimit) {
                    subStmts = 0;
                    subFuncp = newCFunc(VTraceType::CONSTANT, শীৰ্ষ_Funcp, sub_Func_অঙ্ক);
                    ++sub_Func_অঙ্ক;
                }
                File_সাৰি* ধ্রুৱ flp = declp->fileline();
                যদি (declp->dtypeCallp()) {
                    AstCFunc* ধ্রুৱ funcp = createConstDtypeTraceFunctions(declp);
                    Ast_গ্ৰন্থি_ৰাশিমালা* argsp = nullptr;
                    argsp = Ast_গ্ৰন্থি::addNext(argsp, declp->valuep()->clone_বৃক্ষ(মিছা));
                    AstCCall* ধ্রুৱ callp = নূতন AstCCall{flp, funcp, argsp};
                    callp->dtypeSetVoid();
                    callp->argTypes(callp->argTypes() + "bufp, " + std::to_string(declp->code()));
                    subFuncp->addStmtsp(callp->makeStmt());

                    subStmts += 1;
                } অন্যথা {
                    AstTraceInc* ধ্রুৱ incp = নূতন AstTraceInc{flp, declp, VTraceType::CONSTANT};
                    subFuncp->addStmtsp(incp);
                    subStmts += incp->গ্ৰন্থি_গুনতি();
                }
            }
        }

        // Assign codes to duplicate nodes from their canonicals
        বাবে (ধ্রুৱ TraceTraceVertex* ধ্রুৱ vtxp : duplicates) {
            AstTraceDecl* ধ্রুৱ declp = vtxp->nodep();
            ধ্রুৱ TraceTraceVertex* ধ্রুৱ canonVtxp = vtxp->duplicatep();
            ধ্রুৱ AstTraceDecl* ধ্রুৱ canonDeclp = canonVtxp->nodep();
            UASSERT_OBJ(!canonVtxp->duplicatep(), canonDeclp, "Canonical node is a duplicate");
            UASSERT_OBJ(canonDeclp->codeAssigned(), canonDeclp,
                        "Canonical node should have code assigned already");
            declp->code(canonDeclp->code() + vtxp->dtypeAliasOffset());
        }
    }

    DtypeFuncs createNonConstDtypeTraceFunctions(ধ্রুৱ AstTraceDecl* declp) {
        Ast_গ্ৰন্থি_DType* dtypep = declp->dtypep()->skipRefp();
        auto pair = m_dtypeNonConstFuncs.emplace(dtypep, DtypeFuncs{});
        যদি (pair.second) {
            File_সাৰি* ধ্রুৱ flp = declp->fileline();
            AstCFunc* ধ্রুৱ fullFuncp = newCFunc(VTraceType::FULL, nullptr, 0, 0, declp);
            AstCFunc* ধ্রুৱ chgFuncp = newCFunc(VTraceType::CHANGE, nullptr, 0, 0, declp);

            তার্কিক first = সঁচা;
            বাবে (Ast_গ্ৰন্থি* callStmtp = declp->dtypeCallp()->funcp()->stmtsp(); callStmtp;
                 callStmtp = callStmtp->nextp()) {
                ধ্রুৱ AstCCall* ধ্রুৱ callp
                    = VN_CAST(VN_CAST(callStmtp, Stmt_ৰাশিমালা)->exprp(), CCall);
                তার্কিক onlyOne = first && !callStmtp->nextp();
                AstCFunc* ধ্রুৱ fullSubFuncp
                    = onlyOne ? fullFuncp : newCDtypeSubFunc(VTraceType::FULL, declp, fullFuncp);
                AstCFunc* ধ্রুৱ chgSubFuncp
                    = onlyOne ? chgFuncp : newCDtypeSubFunc(VTraceType::CHANGE, declp, chgFuncp);
                বাবে (Ast_গ্ৰন্থি* stmtp = callp->funcp()->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    যদি (AstTraceDecl* ধ্রুৱ fieldDeclp = VN_CAST(stmtp, TraceDecl)) {
                        AstTraceInc* ধ্রুৱ incFullp
                            = নূতন AstTraceInc{flp, fieldDeclp, VTraceType::FULL};
                        fullSubFuncp->addStmtsp(incFullp);
                        AstTraceInc* ধ্রুৱ incChgp
                            = নূতন AstTraceInc{flp, fieldDeclp, VTraceType::CHANGE};
                        chgSubFuncp->addStmtsp(incChgp);
                    }
                }
                first = মিছা;
            }

            pair.first->second = {.fullFuncp = fullFuncp, .chgFuncp = chgFuncp};
        }

        ঘূৰাই pair.first->second;
    }

    শূন্য createNonConstTraceFunctions(ধ্রুৱ TraceVec& traces, uint32_t nAllCodes,
                                      uint32_t parallelism) {
        ধ্রুৱ পূর্ণাঙ্ক splitLimit = v3Global.opt.outputSplitCTrace() ? v3Global.opt.outputSplitCTrace()
                                                                : std::numeric_limits<পূর্ণাঙ্ক>::max();

        // pre-incremented, so starts at 0
        uint32_t শীৰ্ষ_Func_অঙ্ক = std::numeric_limits<uint32_t>::max();
        TraceVec::const_iterator it = traces.begin();
        যেতিয়ালৈ (it != traces.end()) {
            AstCFunc* শীৰ্ষ_Ful_Funcp = nullptr;
            AstCFunc* শীৰ্ষ_Chg_Funcp = nullptr;
            AstCFunc* subFulFuncp = nullptr;
            AstCFunc* subChgFuncp = nullptr;
            uint32_t sub_Func_অঙ্ক = 0;
            পূর্ণাঙ্ক subStmts = 0;
            ধ্রুৱ uint32_t maxCodes = std::max((nAllCodes + parallelism - 1) / parallelism, 1U);
            uint32_t nCodes = 0;
            ধ্রুৱ ActCodeSet* prevActSet = nullptr;
            AstIf* ifp = nullptr;
            uint32_t baseCode = 0;
            বাবে (; nCodes < maxCodes && it != traces.end(); ++it) {
                ধ্রুৱ ActCodeSet& actSet = it->first;
                // Traced value never changes, no need to add it
                যদি (actSet.গুনতি(TraceActivityVertex::ACTIVITY_NEVER)) অনুৱর্ত;

                ধ্রুৱ TraceTraceVertex* ধ্রুৱ vtxp = it->second;
                AstTraceDecl* ধ্রুৱ declp = vtxp->nodep();

                // This is a duplicate decl, no need to add it, but must set the
                // function index to the same as the canonical node.
                যদি (ধ্রুৱ TraceTraceVertex* ধ্রুৱ canonVtxp = vtxp->duplicatep()) {
                    declp->fidx(canonVtxp->nodep()->fidx());
                    অনুৱর্ত;
                }

                // Create top function if not yet created
                যদি (!শীৰ্ষ_Ful_Funcp) {
                    ++শীৰ্ষ_Func_অঙ্ক;
                    শীৰ্ষ_Ful_Funcp = newCFunc(VTraceType::FULL, nullptr, শীৰ্ষ_Func_অঙ্ক);
                    শীৰ্ষ_Chg_Funcp = newCFunc(VTraceType::CHANGE, nullptr, শীৰ্ষ_Func_অঙ্ক);
                }

                // Create new sub function if required
                যদি (!subFulFuncp || subStmts > splitLimit) {
                    baseCode = declp->code();
                    subStmts = 0;
                    subFulFuncp = newCFunc(VTraceType::FULL, শীৰ্ষ_Ful_Funcp, sub_Func_অঙ্ক, baseCode);
                    subChgFuncp = newCFunc(VTraceType::CHANGE, শীৰ্ষ_Chg_Funcp, sub_Func_অঙ্ক, baseCode);
                    ++sub_Func_অঙ্ক;
                    prevActSet = nullptr;
                    ifp = nullptr;
                }

                // If required, create the conditional node checking the activity flags
                যদি (!prevActSet || actSet != *prevActSet) {
                    File_সাৰি* ধ্রুৱ flp = m_শীৰ্ষ_Scopep->fileline();
                    ধ্রুৱ তার্কিক always = actSet.গুনতি(TraceActivityVertex::ACTIVITY_ALWAYS) != 0;
                    Ast_গ্ৰন্থি_ৰাশিমালা* condp = nullptr;
                    যদি (always) {
                        condp = নূতন AstConst{flp, 1};  // Always true, will be folded later
                    } অন্যথা {
                        বাবে (ধ্রুৱ uint32_t actCode : actSet) {
                            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ selp = selectActivity(flp, actCode, VAccess::READ);
                            condp = condp ? নূতন AstOr{flp, condp, selp} : selp;
                        }
                    }
                    ifp = নূতন AstIf{flp, condp};
                    যদি (!always) ifp->branchPred(VBranchPred::BP_UNLIKELY);
                    subChgFuncp->addStmtsp(ifp);
                    subStmts += ifp->গ্ৰন্থি_গুনতি();
                    prevActSet = &actSet;
                }

                // Add TraceInc nodes
                File_সাৰি* ধ্রুৱ flp = declp->fileline();
                যদি (declp->dtypeCallp()) {
                    DtypeFuncs funcs = createNonConstDtypeTraceFunctions(declp);
                    Ast_গ্ৰন্থি_ৰাশিমালা* argsp = nullptr;
                    argsp = Ast_গ্ৰন্থি::addNext(argsp, declp->valuep()->clone_বৃক্ষ(মিছা));
                    AstCCall* ধ্রুৱ callFullp = নূতন AstCCall{flp, funcs.fullFuncp, argsp};
                    callFullp->dtypeSetVoid();
                    callFullp->argTypes(callFullp->argTypes() + "bufp, "
                                        + std::to_string(declp->code()));
                    subFulFuncp->addStmtsp(callFullp->makeStmt());
                    argsp = nullptr;
                    argsp = Ast_গ্ৰন্থি::addNext(argsp, declp->valuep()->clone_বৃক্ষ(মিছা));
                    AstCCall* ধ্রুৱ callChgp = নূতন AstCCall{flp, funcs.chgFuncp, argsp};
                    callChgp->dtypeSetVoid();
                    callChgp->argTypes(callChgp->argTypes() + "bufp, "
                                       + std::to_string(declp->code()));
                    ifp->addThensp(callChgp->makeStmt());

                    subStmts += 2;
                } অন্যথা {
                    AstTraceInc* ধ্রুৱ incFulp = নূতন AstTraceInc{flp, declp, VTraceType::FULL};
                    subFulFuncp->addStmtsp(incFulp);
                    AstTraceInc* ধ্রুৱ incChgp
                        = নূতন AstTraceInc{flp, declp, VTraceType::CHANGE, baseCode};
                    ifp->addThensp(incChgp);

                    // Set the function index of the decl
                    declp->fidx(শীৰ্ষ_Func_অঙ্ক);

                    // Track splitting due to size
                    UASSERT_OBJ(incFulp->গ্ৰন্থি_গুনতি() == incChgp->গ্ৰন্থি_গুনতি(), declp,
                                "Should have equal cost");
                    ধ্রুৱ VNumRange range = declp->arrayRange();
                    যদি (range.ranged()) {
                        // 2x because each element is a TraceInc and a VarRef
                        subStmts += range.elements() * 2;
                    } অন্যথা {
                        subStmts += incChgp->গ্ৰন্থি_গুনতি();
                    }
                }

                // Track partitioning
                nCodes += declp->codeInc();
            }
        }
    }

    শূন্য createCleanupFunction() {
        File_সাৰি* ধ্রুৱ fl = m_শীৰ্ষ_Scopep->fileline();
        AstCFunc* ধ্রুৱ cleanupFuncp = নূতন AstCFunc{fl, "trace_cleanup", m_শীৰ্ষ_Scopep};
        cleanupFuncp->argTypes("void* voidSelf, " + v3Global.opt.traceClassBase()
                               + "* /*unused*/");
        cleanupFuncp->isTrace(সঁচা);
        cleanupFuncp->slow(মিছা);
        cleanupFuncp->isStatic(সঁচা);
        cleanupFuncp->isLoose(সঁচা);
        m_শীৰ্ষ_Scopep->addBlocksp(cleanupFuncp);
        cleanupFuncp->addStmtsp(নূতন AstCStmt{fl, EmitCUtil::voidSelfAssign(m_শীৰ্ষ_Modp)});
        cleanupFuncp->addStmtsp(নূতন AstCStmt{fl, EmitCUtil::symClassAssign()});

        // Register it
        {
            AstCStmt* ধ্রুৱ cstmtp = নূতন AstCStmt{fl};
            m_regFuncp->addStmtsp(cstmtp);
            cstmtp->add("tracep->addCleanupCb(");
            cstmtp->add(নূতন AstAddrOfCFunc{fl, cleanupFuncp});
            cstmtp->add(", vlSelf);");
        }

        // Clear global activity flag
        cleanupFuncp->addStmtsp(
            নূতন AstCStmt{m_শীৰ্ষ_Scopep->fileline(), "vlSymsp->__Vm_activity = false;"s});

        // Clear fine grained activity flags
        বাবে (uint32_t i = 0; i < m_activity_সংখ্যা; ++i) {
            Ast_গ্ৰন্থি* ধ্রুৱ clrp = নূতন AstAssign{fl, selectActivity(fl, i, VAccess::WRITE),
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
        graphSimplify(সঁচা);
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
        m_regFuncp = নূতন AstCFunc{m_শীৰ্ষ_Scopep->fileline(), "trace_register", m_শীৰ্ষ_Scopep};
        m_regFuncp->argTypes(v3Global.opt.traceClassBase() + "* tracep");
        m_regFuncp->entryPoint(সঁচা);
        m_regFuncp->isTrace(সঁচা);
        m_regFuncp->slow(সঁচা);
        m_regFuncp->isStatic(মিছা);
        m_regFuncp->isLoose(সঁচা);
        m_শীৰ্ষ_Scopep->addBlocksp(m_regFuncp);

        // Create the const dump functions. Also allocates trace codes.
        createConstTraceFunctions(traces);

        // Create the full and incremental dump functions
        createNonConstTraceFunctions(traces, nNonConstCodes, m_parallelism);

        // Root-traced libraries alias wrapper IOs onto the existing top-module codes.
        যদি (!v3Global.opt.libCreate().empty()) {
            std::vector<AstTraceDecl*> মূল_Declps;
            std::vector<AstTraceDecl*> শীৰ্ষ_Declps;
            TraceInitDeclCollector মূল_Collector{মূল_Declps};
            TraceInitDeclCollector শীৰ্ষ_Collector{শীৰ্ষ_Declps};
            বাবে (Ast_গ্ৰন্থি* blockp = m_শীৰ্ষ_Scopep->blocksp(); blockp; blockp = blockp->nextp()) {
                AstCFunc* ধ্রুৱ funcp = VN_CAST(blockp, CFunc);
                যদি (funcp && VString::startsWith(funcp->name(), "trace_init_leaf_root__")) {
                    মূল_Collector.collect(funcp);
                } অন্যথা যদি (funcp && VString::startsWith(funcp->name(), "trace_init_leaf_top__")) {
                    শীৰ্ষ_Collector.collect(funcp);
                }
            }

            std::vector<তার্কিক> used(শীৰ্ষ_Declps.size(), মিছা);
            বাবে (AstTraceDecl* ধ্রুৱ মূল_Declp : মূল_Declps) {
                বাবে (size_t i = 0; i < শীৰ্ষ_Declps.size(); ++i) {
                    যদি (used[i] || !same_মূল_Init_Alias(মূল_Declp, শীৰ্ষ_Declps[i])) অনুৱর্ত;
                    মূল_Declp->code(শীৰ্ষ_Declps[i]->code());
                    used[i] = সঁচা;
                    ভঙ্গ;
                }
            }
        }

        // Remove refs to traced values from TraceDecl nodes, these have now moved under
        // TraceInc
        বাবে (ধ্রুৱ auto& i : traces) {
            Ast_গ্ৰন্থি* ধ্রুৱ valuep = i.second->nodep()->valuep();
            VL_DO_DANGLING(valuep->unlinkFrBack()->delete_বৃক্ষ(), valuep);
        }

        // Create the trace cleanup function clearing the activity flags
        createCleanupFunction();
    }

    TraceCFuncVertex* getCFuncVertexp(AstCFunc* nodep) {
        V3GraphVertex* ধ্রুৱ vtxp = nodep->user1u().toGraphVertex();
        TraceCFuncVertex* vertexp = vtxp ? vtxp->cast<TraceCFuncVertex>() : nullptr;
        যদি (!vertexp) {
            vertexp = নূতন TraceCFuncVertex{&m_graph, nodep};
            nodep->user1p(vertexp);
        }
        ঘূৰাই vertexp;
    }
    TraceActivityVertex* getActivityVertexp(Ast_গ্ৰন্থি* nodep, তার্কিক slow) {
        V3GraphVertex* ধ্রুৱ vtxp = nodep->user3u().toGraphVertex();
        TraceActivityVertex* vertexp = vtxp ? vtxp->cast<TraceActivityVertex>() : nullptr;
        যদি (!vertexp) {
            vertexp = নূতন TraceActivityVertex{&m_graph, nodep, slow};
            nodep->user3p(vertexp);
        }
        vertexp->slow(slow);
        ঘূৰাই vertexp;
    }

    // VISITORS
    শূন্য visit(AstNetlist* nodep) override {
        // Add vertexes for all TraceDecl, and edges from VARs each trace looks at
        m_finding = মিছা;
        iterateChildren(nodep);

        // Add vertexes for all CFUNCs, and edges to VARs the func sets
        m_finding = সঁচা;
        iterateChildren(nodep);

        // Create the trace functions and insert them into the tree
        createTraceFunctions();

        // Save number of trace codes used
        nodep->nTraceCodes(m_code);
    }
    শূন্য visit(Ast_গ্ৰন্থি_Module* nodep) override {
        যদি (nodep->is_শীৰ্ষ()) m_শীৰ্ষ_Modp = nodep;
        iterateChildren(nodep);
    }
    শূন্য visit(AstVarScope* nodep) override {
        যদি (!m_finding) {
            যদি (ধ্রুৱ AstIface* ধ্রুৱ ifacep = nodep->varp()->sensIfacep()) {
                m_ifaceMemberVscps[{ifacep, nodep->varp()->name()}].ঠেলো_back(nodep);
            }
        }
    }
    শূন্য visit(Ast_Stmt_ৰাশিমালা* nodep) override {
        যদি (!m_finding && !nodep->user2()) {
            যদি (AstCCall* ধ্রুৱ callp = VN_CAST(nodep->exprp(), CCall)) {
                UINFO(8, "   CCALL " << callp);
                // See if there are other calls in same statement list;
                // If so, all funcs might share the same activity code
                TraceActivityVertex* ধ্রুৱ activityVtxp
                    = getActivityVertexp(nodep, callp->funcp()->slow());
                বাবে (Ast_গ্ৰন্থি* nextp = nodep; nextp; nextp = nextp->nextp()) {
                    যদি (Ast_Stmt_ৰাশিমালা* ধ্রুৱ stmtp = VN_CAST(nextp, Stmt_ৰাশিমালা)) {
                        যদি (AstCCall* ধ্রুৱ ccallp = VN_CAST(stmtp->exprp(), CCall)) {
                            stmtp->user2(সঁচা);  // Processed
                            UINFO(8, "     SubCCALL " << ccallp);
                            V3GraphVertex* ধ্রুৱ ccallFuncVtxp = getCFuncVertexp(ccallp->funcp());
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
        V3GraphVertex* ধ্রুৱ funcVtxp = getCFuncVertexp(nodep);
        যদি (!m_finding) {  // If public, we need a unique activity code to allow for sets
                           // directly in this func
            যদি (nodep->funcPublic() || nodep->dpiExportImpl() || nodep->entryPoint()
                || nodep->isCoroutine()) {
                // Cannot treat a coroutine as slow, it may be resumed later
                ধ্রুৱ তার্কিক slow = nodep->slow() && !nodep->isCoroutine();
                TraceActivityVertex* ধ্রুৱ activityVtxp = getActivityVertexp(nodep, slow);
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
                ঘূৰাই;
            }
            V3GraphVertex* ধ্রুৱ vertexp = নূতন TraceTraceVertex{&m_graph, nodep};
            nodep->user1p(vertexp);

            UASSERT_OBJ(m_cfuncp, nodep, "Trace not under func");
            m_cfuncp->user2(সঁচা);
            VL_RESTORER(m_tracep);
            m_tracep = nodep;
            iterateChildren(nodep);
        }
    }
    শূন্য visit(AstVarRef* nodep) override {
        UASSERT_OBJ(nodep->varScopep(), nodep, "No var scope?");
        AstVarScope* ধ্রুৱ varscopep = nodep->varScopep();
        V3GraphVertex* varVtxp = varscopep->user1u().toGraphVertex();
        যদি (m_tracep) {
            UASSERT_OBJ(nodep->access().isReadOnly(), nodep, "Lvalue in trace?  Should be const.");
            যদি (!varVtxp) {
                varVtxp = নূতন TraceVarVertex{&m_graph, nodep->varScopep()};
                nodep->varScopep()->user1p(varVtxp);
            }
            V3GraphVertex* ধ্রুৱ traceVtxp = m_tracep->user1u().toGraphVertex();
            নূতন V3GraphEdge{&m_graph, varVtxp, traceVtxp, 1};
            যদি (nodep->varp()->isPrimaryInish()  // Always need to trace primary inputs
                || nodep->varp()->isSigPublic()) {  // Or ones user can change
                নূতন V3GraphEdge{&m_graph, m_alwaysVtxp, traceVtxp, 1};
            }
            যদি (m_tracep->dtypeCallp()) varscopep->user2p(m_tracep);
        } অন্যথা যদি (m_cfuncp && m_finding && nodep->access().isWriteOrRW()) {
            V3GraphVertex* ধ্রুৱ funcVtxp = getCFuncVertexp(m_cfuncp);
            যদি (varVtxp) {  // else we're not tracing this signal
                নূতন V3GraphEdge{&m_graph, funcVtxp, varVtxp, 1};
                AstTraceDecl* ধ্রুৱ declp = VN_AS(varscopep->user2p(), TraceDecl);
                যদি (declp) {
                    V3GraphVertex* ধ্রুৱ cFuncVtxp = getCFuncVertexp(m_cfuncp);
                    বাবে (ধ্রুৱ V3GraphEdge& edge : cFuncVtxp->inEdges()) {
                        V3GraphVertex* ধ্রুৱ activityp = edge.fromp();
                        যদি (!declp->user3p()) {
                            declp->user3p(activityp);
                        } অন্যথা যদি (declp->user3u().toGraphVertex() != activityp) {
                            declp->user2(সঁচা);
                        }
                    }
                }
            }
        }
    }
    শূন্য visit(AstMemberSel* nodep) override {
        যদি (m_cfuncp && m_finding && nodep->access().isWriteOrRW()) {
            AstIfaceRefDType* ধ্রুৱ dtypep
                = VN_CAST(nodep->fromp()->dtypep()->skipRefp(), IfaceRefDType);
            যদি (dtypep && dtypep->isVirtual()) {
                ধ্রুৱ auto it = m_ifaceMemberVscps.find({dtypep->ifacep(), nodep->varp()->name()});
                যদি (it != m_ifaceMemberVscps.end()) {
                    V3GraphVertex* ধ্রুৱ funcVtxp = getCFuncVertexp(m_cfuncp);
                    বাবে (AstVarScope* ধ্রুৱ vscp : it->second) {
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
    শূন্য visit(Ast_গ্ৰন্থি* nodep) override { iterateChildren(nodep); }

সার্ৱজনিক:
    // CONSTRUCTORS
    explicit TraceVisitor(AstNetlist* nodep)
        : m_alwaysVtxp{নূতন TraceActivityVertex{&m_graph, TraceActivityVertex::ACTIVITY_ALWAYS}} {
        nodep->user2_Clear_বৃক্ষ();  // TraceDecl multiple activities flag
        nodep->user3_Clear_বৃক্ষ();  // TraceDecl TraceActivityVertex (assumes we start at nullptr)
        iterate(nodep);
        nodep->foreach([](AstTraceDecl* ধ্রুৱ declp) {
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
