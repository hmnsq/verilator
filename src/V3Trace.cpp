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

শ্রেণী TraceActivityVertex final : সর্বজনীন V3GraphVertex {
    VL_RTTI_IMPL(TraceActivityVertex, V3GraphVertex)
    Ast_গ্রন্থি* ধ্রুবক m_insertp;
    int32_t m_activityCode;
    যৌক্তিক m_slow;  // If always slow, we can use the same code
সর্বজনীন:
    গণনা { ACTIVITY_NEVER = ((1UL << 31) - 1) };
    গণনা { ACTIVITY_ALWAYS = ((1UL << 31) - 2) };
    গণনা { ACTIVITY_SLOW = 0 };
    TraceActivityVertex(V3Graph* graphp, Ast_গ্রন্থি* nodep, যৌক্তিক slow)
        : V3GraphVertex{graphp}
        , m_insertp{nodep} {
        m_activityCode = 0;
        m_slow = slow;
    }
    TraceActivityVertex(V3Graph* graphp, int32_t code)
        : V3GraphVertex{graphp}
        , m_insertp{nullptr} {
        m_activityCode = code;
        m_slow = মিথ্যা;
    }
    ~TraceActivityVertex() override = পূর্বনির্ধারিত;
    // ACCESSORS
    Ast_গ্রন্থি* insertp() ধ্রুবক {
        UASSERT(m_insertp, "Null insertp; probably called on a special always/slow");
        ফেরত m_insertp;
    }
    string name() ধ্রুবক override {
        যদি (activityAlways()) {
            ফেরত "*ALWAYS*";
        } অন্যথা {
            ফেরত std::string{slow() ? "*SLOW* " : ""} + insertp()->name();
        }
    }
    string dotColor() ধ্রুবক override { ফেরত slow() ? "yellowGreen" : "green"; }
    int32_t activityCode() ধ্রুবক { ফেরত m_activityCode; }
    যৌক্তিক activityAlways() ধ্রুবক { ফেরত activityCode() == ACTIVITY_ALWAYS; }
    যৌক্তিক activitySlow() ধ্রুবক { ফেরত activityCode() == ACTIVITY_SLOW; }
    শূন্য activityCode(int32_t code) { m_activityCode = code; }
    যৌক্তিক slow() ধ্রুবক { ফেরত m_slow; }
    শূন্য slow(যৌক্তিক flag) {
        যদি (!flag) m_slow = মিথ্যা;
    }
};

শ্রেণী TraceCFuncVertex final : সর্বজনীন V3GraphVertex {
    VL_RTTI_IMPL(TraceCFuncVertex, V3GraphVertex)
    AstCFunc* ধ্রুবক m_nodep;

সর্বজনীন:
    TraceCFuncVertex(V3Graph* graphp, AstCFunc* nodep)
        : V3GraphVertex{graphp}
        , m_nodep{nodep} {}
    ~TraceCFuncVertex() override = পূর্বনির্ধারিত;
    // ACCESSORS
    AstCFunc* nodep() ধ্রুবক { ফেরত m_nodep; }
    string name() ধ্রুবক override { ফেরত nodep()->name(); }
    string dotColor() ধ্রুবক override { ফেরত "yellow"; }
    File_সারি* fileline() ধ্রুবক override { ফেরত nodep()->fileline(); }
};

শ্রেণী TraceTraceVertex final : সর্বজনীন V3GraphVertex {
    VL_RTTI_IMPL(TraceTraceVertex, V3GraphVertex)
    AstTraceDecl* ধ্রুবক m_nodep;  // TRACEINC this represents
    // nullptr, or other vertex with the real code() that duplicates this one
    TraceTraceVertex* m_duplicatep = nullptr;
    // When aliasing to a dtype parent, offset of the member within the dtype
    uint32_t m_dtypeAliasOffset = 0;

সর্বজনীন:
    TraceTraceVertex(V3Graph* graphp, AstTraceDecl* nodep)
        : V3GraphVertex{graphp}
        , m_nodep{nodep} {}
    ~TraceTraceVertex() override = পূর্বনির্ধারিত;
    // ACCESSORS
    AstTraceDecl* nodep() ধ্রুবক { ফেরত m_nodep; }
    string name() ধ্রুবক override { ফেরত nodep()->name(); }
    string dotColor() ধ্রুবক override { ফেরত "red"; }
    File_সারি* fileline() ধ্রুবক override { ফেরত nodep()->fileline(); }
    TraceTraceVertex* duplicatep() ধ্রুবক { ফেরত m_duplicatep; }
    শূন্য duplicatep(TraceTraceVertex* dupp) {
        UASSERT_OBJ(!duplicatep(), nodep(), "Assigning duplicatep() to already duplicated node");
        m_duplicatep = dupp;
    }
    শূন্য redirectDuplicatep(TraceTraceVertex* dupp) { m_duplicatep = dupp; }
    uint32_t dtypeAliasOffset() ধ্রুবক { ফেরত m_dtypeAliasOffset; }
    শূন্য dtypeAliasOffset(uint32_t offset) { m_dtypeAliasOffset = offset; }
};

শ্রেণী TraceVarVertex final : সর্বজনীন V3GraphVertex {
    VL_RTTI_IMPL(TraceVarVertex, V3GraphVertex)
    AstVarScope* ধ্রুবক m_nodep;

সর্বজনীন:
    TraceVarVertex(V3Graph* graphp, AstVarScope* nodep)
        : V3GraphVertex{graphp}
        , m_nodep{nodep} {}
    ~TraceVarVertex() override = পূর্বনির্ধারিত;
    // ACCESSORS
    AstVarScope* nodep() ধ্রুবক { ফেরত m_nodep; }
    string name() ধ্রুবক override { ফেরত nodep()->name(); }
    string dotColor() ধ্রুবক override { ফেরত "skyblue"; }
    File_সারি* fileline() ধ্রুবক override { ফেরত nodep()->fileline(); }
};

//######################################################################
// Trace state, as a visitor of each AstNode

শ্রেণী TraceVisitor final : সর্বজনীন VNVisitor {
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
    ধ্রুবক VNUser1InUse m_inuser1;
    ধ্রুবক VNUser2InUse m_inuser2;
    ধ্রুবক VNUser3InUse m_inuser3;
    // VNUser4InUse     In V3Hasher via V3DupFinder

    // STATE
    Ast_গ্রন্থি_Module* m_শীর্ষ_Modp = nullptr;  // Module to add variables to
    AstScope* ধ্রুবক m_শীর্ষ_Scopep = v3Global.rootp()->শীর্ষ_Scopep()->scopep();  // The top AstScope
    AstCFunc* m_cfuncp = nullptr;  // C function adding to graph
    AstCFunc* m_regFuncp = nullptr;  // Trace registration function
    AstCFunc* m_actAllFuncp = nullptr;  // Set all activity function
    AstTraceDecl* m_tracep = nullptr;  // Trace function adding to graph
    AstVarScope* m_activityVscp = nullptr;  // Activity variable
    uint32_t m_activity_সংখ্যা = 0;  // Count of fields in activity variable
    uint32_t m_code = 0;  // Trace ident code# being assigned
    V3Graph m_graph;  // Var/CFunc tracking
    TraceActivityVertex* ধ্রুবক m_alwaysVtxp;  // "Always trace" vertex
    যৌক্তিক m_finding = মিথ্যা;  // Pass one of algorithm?
    কাঠামো DtypeFuncs final {
    সর্বজনীন:
        AstCFunc* fullFuncp = nullptr;
        AstCFunc* chgFuncp = nullptr;
    };
    std::unordered_map<ধ্রুবক Ast_গ্রন্থি_DType*, DtypeFuncs>
        m_dtypeNonConstFuncs;  // Full / Chg funcs per type
    std::unordered_map<ধ্রুবক Ast_গ্রন্থি_DType*, AstCFunc*> m_dtypeConstFuncs;  // Const func per type
    V3UniqueNames m_dtypeNames{""};  // Unique type func names

    // Trace parallelism. Only VCD tracing can be parallelized at this time.
    ধ্রুবক uint32_t m_parallelism
        = v3Global.opt.useTraceParallel() ? static_cast<uint32_t>(v3Global.opt.threads()) : 1;

    VDouble0 m_statSetters;  // Statistic tracking
    VDouble0 m_statSettersSlow;  // Statistic tracking
    VDouble0 m_statUniqSigs;  // Statistic tracking

    // All activity numbers applying to a given trace
    using ActCodeSet = std::set<uint32_t>;
    // For activity set, what traces apply
    using TraceVec = std::multimap<ActCodeSet, TraceTraceVertex*>;
    // Candidate interface-member VarScopes keyed by (interface type, member name)
    std::map<std::pair<ধ্রুবক AstIface*, std::string>, std::vector<AstVarScope*>>
        m_ifaceMemberVscps;

    শ্রেণী TraceInitDeclCollector final : সর্বজনীন VNVisitor {
        std::vector<AstTraceDecl*>& m_declps;  // Output: trace declarations found so far
        std::set<ধ্রুবক AstCFunc*> m_seenFuncps;

        শূন্য visit(AstTraceDecl* nodep) override { m_declps.ঠেলো_back(nodep); }
        শূন্য visit(AstCCall* nodep) override {
            যদি (AstCFunc* ধ্রুবক funcp = nodep->funcp()) collect(funcp);
        }
        শূন্য visit(Ast_গ্রন্থি* nodep) override { iterateChildren(nodep); }

    সর্বজনীন:
        explicit TraceInitDeclCollector(std::vector<AstTraceDecl*>& declps)
            : m_declps{declps} {}
        শূন্য collect(AstCFunc* funcp) {
            যদি (funcp && m_seenFuncps.ঢোকাও(funcp).second) iterate(funcp);
        }
    };

    // METHODS

    স্থির যৌক্তিক same_মূল_Init_Alias(ধ্রুবক AstTraceDecl* মূল_Declp, ধ্রুবক AstTraceDecl* শীর্ষ_Declp) {
        ধ্রুবক VNumRange& মূল_Bit_Range = মূল_Declp->bitRange();
        ধ্রুবক VNumRange& শীর্ষ_Bit_Range = শীর্ষ_Declp->bitRange();
        ফেরত মূল_Declp->showname() == শীর্ষ_Declp->showname()
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
        std::unordered_map<ধ্রুবক AstTraceDecl*, uint32_t> memberOffsets;
        std::unordered_map<ধ্রুবক AstTraceDecl*, uint32_t> runningOffset;

        // For fixup pass: track dups redirected to dtype parents, so we can
        // fix them if the parent's user2 is later set during the loop.
        // Maps dup vertex -> original canonical member vertex.
        std::vector<std::pair<TraceTraceVertex*, TraceTraceVertex*>> dtypeParentRedirects;

        // Hash all of the traced values and find if there are any duplicates
        জন্য (V3GraphVertex& vtx : m_graph.vertices()) {
            যদি (TraceTraceVertex* ধ্রুবক vvertexp = vtx.cast<TraceTraceVertex>()) {
                AstTraceDecl* ধ্রুবক nodep = vvertexp->nodep();
                যদি (nodep->dtypeCallp()) চালু;

                যদি (nodep->dtypeDeclp()) {
                    uint32_t& offset = runningOffset[nodep->dtypeDeclp()];
                    memberOffsets[nodep] = offset;
                    offset += nodep->codeInc();
                }

                UASSERT_OBJ(!vvertexp->duplicatep(), nodep, "Should not be a duplicate");
                ধ্রুবক auto dupit = dupFinder.findDuplicate(nodep);
                যদি (dupit == dupFinder.end()) {
                    dupFinder.ঢোকাও(nodep);
                } অন্যথা {
                    AstTraceDecl* ধ্রুবক dupDeclp = VN_AS(dupit->second, TraceDecl);
                    UASSERT_OBJ(dupDeclp, nodep, "Trace duplicate of wrong type");
                    TraceTraceVertex* ধ্রুবক dupvertexp
                        = dupDeclp->user1u().toGraphVertex()->cast<TraceTraceVertex>();
                    UINFO(8, "  Orig " << dupDeclp << endl);
                    UINFO(8, "   dup " << nodep << endl);

                    যদি (dupDeclp->dtypeDeclp() && !dupDeclp->dtypeDeclp()->user2()) {
                        AstTraceDecl* ধ্রুবক dtypeParentp = dupDeclp->dtypeDeclp();
                        TraceTraceVertex* ধ্রুবক dtypeVtxp
                            = dtypeParentp->user1u().toGraphVertex()->cast<TraceTraceVertex>();
                        ধ্রুবক auto it2 = memberOffsets.find(dupDeclp);
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

        জন্য (ধ্রুবক auto& pair : dtypeParentRedirects) {
            TraceTraceVertex* ধ্রুবক dupVtxp = pair.first;
            TraceTraceVertex* ধ্রুবক memberVtxp = pair.second;
            ধ্রুবক AstTraceDecl* ধ্রুবক parentDeclp = dupVtxp->duplicatep()->nodep();
            যদি (parentDeclp->user2()) {
                dupVtxp->redirectDuplicatep(memberVtxp);
                dupVtxp->dtypeAliasOffset(0);
            }
        }

        যদি (dumpLevel() || debug() >= 9)
            dupFinder.dumpFile(v3Global.debugFilename("trace") + ".hash", মিথ্যা);
    }

    শূন্য graphDtypePrune() {
        জন্য (V3GraphVertex* ধ্রুবক vtxp : m_graph.vertices().unlinkable()) {
            যদি (TraceTraceVertex* ধ্রুবক vvertexp = vtxp->cast<TraceTraceVertex>()) {
                AstTraceDecl* ধ্রুবক declp = vvertexp->nodep();
                // This skips the dtype sub-func optimization if a var is affected by multiple
                // activities.  We really only need to do this for _chg funcs (and not decls,
                // _const and _full) but it's simpiler to do it all one way or the other.
                যদি (declp) {
                    যদি (declp->user2() || (declp->dtypeDeclp() && !declp->dtypeDeclp()->user2())) {
                        AstCCall* ধ্রুবক callp = declp->dtypeCallp();
                        যদি (callp) {
                            Ast_গ্রন্থি* stmtexprp = callp->backp();
                            VL_DO_DANGLING(ঠেলো_Deletep(stmtexprp->unlinkFrBack()), stmtexprp);
                        } অন্যথা {
                            যৌক্তিক emptyScope;
                            করো {
                                emptyScope = মিথ্যা;
                                Ast_গ্রন্থি* ধ্রুবক declBackp = declp->backp();
                                Ast_গ্রন্থি* ধ্রুবক declNextp = declp->nextp();
                                যদি (VN_IS(declBackp, Trace_ঠেলো_Prefix)
                                    && VN_IS(declNextp, Trace_বের_করো_Prefix)) {
                                    VL_DO_DANGLING(ঠেলো_Deletep(declBackp->unlinkFrBack()),
                                                   declBackp);
                                    VL_DO_DANGLING(ঠেলো_Deletep(declNextp->unlinkFrBack()),
                                                   declNextp);
                                    emptyScope = সত্য;
                                }
                            } যখন (emptyScope);
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
        ধ্রুবক পূর্ণসংখ্যা splitLimit = v3Global.opt.outputSplitCTrace() ? v3Global.opt.outputSplitCTrace()
                                                                : std::numeric_limits<পূর্ণসংখ্যা>::max();
        যদি (splitLimit == std::numeric_limits<পূর্ণসংখ্যা>::max()) ফেরত;

        জন্য (Ast_গ্রন্থি* nodep = m_শীর্ষ_Scopep->blocksp(); nodep; nodep = nodep->nextp()) {
            AstCFunc* ধ্রুবক funcp = VN_CAST(nodep, CFunc);
            যদি (!funcp || !funcp->user2()) চালু;

            ধ্রুবক string baseName = funcp->name();
            uint32_t sub_অঙ্ক = 1;
            পূর্ণসংখ্যা cur_আকার = 0;
            AstCFunc* curFuncp = funcp;
            std::vector<Ast_গ্রন্থি_Stmt*> callStmts;

            auto startNewSubFunc = [&]() {
                File_সারি* ধ্রুবক flp = funcp->fileline();
                ধ্রুবক string newName = baseName + "_" + cvtToStr(sub_অঙ্ক++);
                AstCFunc* ধ্রুবক newFuncp = নতুন AstCFunc{flp, newName, m_শীর্ষ_Scopep};
                newFuncp->argTypes(v3Global.opt.traceClassBase() + "* tracep");
                newFuncp->isTrace(সত্য);
                newFuncp->isStatic(মিথ্যা);
                newFuncp->isLoose(সত্য);
                newFuncp->slow(সত্য);
                newFuncp->dontCombine(সত্য);
                m_শীর্ষ_Scopep->addBlocksp(newFuncp);
                newFuncp->addStmtsp(নতুন AstCStmt{flp, "const int c = vlSymsp->__Vm_baseCode;"});
                AstCCall* ধ্রুবক callp = নতুন AstCCall{flp, newFuncp};
                callp->dtypeSetVoid();
                callp->argTypes("tracep");
                callStmts.ঠেলো_back(callp->makeStmt());
                curFuncp = newFuncp;
                cur_আকার = 0;
            };

            জন্য (Ast_গ্রন্থি *stmtp = funcp->stmtsp(), *nextp; stmtp; stmtp = nextp) {
                nextp = stmtp->nextp();
                cur_আকার += stmtp->গ্রন্থি_গুনতি();

                যদি (curFuncp != funcp) {
                    stmtp->unlinkFrBack();
                    curFuncp->addStmtsp(stmtp);
                }

                যদি (cur_আকার > splitLimit && nextp) startNewSubFunc();
            }

            জন্য (Ast_গ্রন্থি_Stmt* ধ্রুবক callStmtp : callStmts) { funcp->addStmtsp(callStmtp); }
        }
    }

    শূন্য graphSimplify(যৌক্তিক initial) {
        যদি (initial) {
            // Remove all variable nodes
            জন্য (V3GraphVertex* ধ্রুবক vtxp : m_graph.vertices().unlinkable()) {
                যদি (TraceVarVertex* ধ্রুবক vvertexp = vtxp->cast<TraceVarVertex>()) {
                    vvertexp->rerouteEdges(&m_graph);
                    vvertexp->unlinkDelete(&m_graph);
                }
            }
            // Remove multiple variables connecting funcs to traces
            // We do this twice, as then we have fewer edges to multiply out in the below
            // expansion.
            m_graph.removeRedundantEdgesMax(&V3GraphEdge::followAlwaysTrue);
            // Remove all Cfunc nodes
            জন্য (V3GraphVertex* ধ্রুবক vtxp : m_graph.vertices().unlinkable()) {
                যদি (TraceCFuncVertex* ধ্রুবক vvertexp = vtxp->cast<TraceCFuncVertex>()) {
                    vvertexp->rerouteEdges(&m_graph);
                    vvertexp->unlinkDelete(&m_graph);
                }
            }
        }

        // Remove multiple variables connecting funcs to traces
        m_graph.removeRedundantEdgesMax(&V3GraphEdge::followAlwaysTrue);

        // If there are any edges from a always, keep only the always
        জন্য (V3GraphVertex& vtx : m_graph.vertices()) {
            যদি (TraceTraceVertex* ধ্রুবক vvertexp = vtx.cast<TraceTraceVertex>()) {
                // Search for the incoming always edge
                ধ্রুবক V3GraphEdge* alwaysEdgep = nullptr;
                জন্য (ধ্রুবক V3GraphEdge& edge : vvertexp->inEdges()) {
                    ধ্রুবক TraceActivityVertex* ধ্রুবক actVtxp
                        = edge.fromp()->as<ধ্রুবক TraceActivityVertex>();
                    যদি (actVtxp->activityAlways()) {
                        alwaysEdgep = &edge;
                        বিরতি;
                    }
                }
                // If always edge exists, remove all other edges
                যদি (alwaysEdgep) {
                    জন্য (V3GraphEdge* ধ্রুবক edgep : vvertexp->inEdges().unlinkable()) {
                        যদি (edgep != alwaysEdgep) VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
                    }
                }
            }
        }

        // Activity points with no outputs can be removed
        জন্য (V3GraphVertex* ধ্রুবক vtxp : m_graph.vertices().unlinkable()) {
            যদি (TraceActivityVertex* ধ্রুবক aVtxp = vtxp->cast<TraceActivityVertex>()) {
                // Leave in the always vertex for later use.
                যদি (aVtxp != m_alwaysVtxp && aVtxp->outEmpty()) {
                    VL_DO_DANGLING(aVtxp->unlinkDelete(&m_graph), aVtxp);
                }
            }
        }
    }

    uint32_t assignactivityNumbers() {
        uint32_t activity_সংখ্যা = 1;  // Note 0 indicates "slow" only
        জন্য (V3GraphVertex& vtx : m_graph.vertices()) {
            যদি (TraceActivityVertex* ধ্রুবক vvertexp = vtx.cast<TraceActivityVertex>()) {
                যদি (vvertexp != m_alwaysVtxp) {
                    যদি (vvertexp->slow()) {
                        vvertexp->activityCode(TraceActivityVertex::ACTIVITY_SLOW);
                    } অন্যথা {
                        vvertexp->activityCode(activity_সংখ্যা++);
                    }
                }
            }
        }
        ফেরত activity_সংখ্যা;
    }

    শূন্য sortTraces(TraceVec& traces, uint32_t& nNonConstCodes) {
        // Populate sort structure
        traces.clear();
        nNonConstCodes = 0;
        জন্য (V3GraphVertex& vtx : m_graph.vertices()) {
            যদি (TraceTraceVertex* ধ্রুবক vtxp = vtx.cast<TraceTraceVertex>()) {
                ActCodeSet actSet;
                UINFO(9, "  Add to sort: " << vtxp);
                UINFOTREE(9, vtxp->nodep(), "", "trnode");
                জন্য (ধ্রুবক V3GraphEdge& edge : vtxp->inEdges()) {
                    ধ্রুবক TraceActivityVertex* ধ্রুবক cfvertexp
                        = edge.fromp()->cast<ধ্রুবক TraceActivityVertex>();
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
                                || actSet.আকার_2() == 1,
                            vtxp->nodep(), "Always active trace has further triggers");
                // Count nodes
                যদি (!vtxp->duplicatep() && !actSet.empty())
                    nNonConstCodes += vtxp->nodep()->codeInc();
                যদি (actSet.empty()) {
                    // If a trace doesn't have activity, it's constant, and we
                    // don't need to track changes on it.
                    actSet.ঢোকাও(TraceActivityVertex::ACTIVITY_NEVER);
                } অন্যথা যদি (actSet.গুনতি(TraceActivityVertex::ACTIVITY_SLOW) && actSet.আকার_2() > 1) {
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
        ধ্রুবক auto end = traces.end();
        যখন (it != end) {
            auto শীর্ষ_2 = it;
            // Approximate the complexity of the value change check
            uint32_t complexity = 0;
            ধ্রুবক ActCodeSet& actSet = it->first;
            জন্য (; it != end && it->first == actSet; ++it) {
                যদি (!it->second->duplicatep()) {
                    uint32_t cost = 0;
                    ধ্রুবক AstTraceDecl* ধ্রুবক declp = it->second->nodep();
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
            যদি (actSet.গুনতি(TraceActivityVertex::ACTIVITY_ALWAYS)) চালু;
            যদি (actSet.গুনতি(TraceActivityVertex::ACTIVITY_NEVER)) চালু;
            যদি (actSet.গুনতি(TraceActivityVertex::ACTIVITY_SLOW)) চালু;
            // If the value comparisons are cheaper to perform than checking the
            // activity flags make the signals always traced. Note this cost
            // equation is heuristic.
            যদি (complexity <= actSet.আকার_2() * 2) {
                জন্য (; শীর্ষ_2 != it; ++শীর্ষ_2) {
                    নতুন V3GraphEdge{&m_graph, m_alwaysVtxp, শীর্ষ_2->second, 1};
                }
            }
        }

        graphSimplify(মিথ্যা);
    }

    Ast_গ্রন্থি_রাশিমালা* selectActivity(File_সারি* flp, uint32_t acode, ধ্রুবক VAccess& access) {
        ফেরত নতুন AstArraySel(flp, নতুন AstVarRef{flp, m_activityVscp, access}, acode);
    }

    Ast_গ্রন্থি* newActivitySetter(Ast_গ্রন্থি* insertp, uint32_t code) {
        ++m_statSetters;
        File_সারি* ধ্রুবক fl = insertp->fileline();
        AstAssign* ধ্রুবক setterp = নতুন AstAssign{fl, selectActivity(fl, code, VAccess::WRITE),
                                                 নতুন AstConst{fl, AstConst::BitTrue{}}};
        ফেরত setterp;
    }

    Ast_গ্রন্থি* newActivityAll(Ast_গ্রন্থি* insertp) {
        ++m_statSettersSlow;
        যদি (!m_actAllFuncp) {
            File_সারি* ধ্রুবক flp = m_শীর্ষ_Scopep->fileline();
            AstCFunc* ধ্রুবক funcp = নতুন AstCFunc{flp, "__VtraceActivitySetAll", m_শীর্ষ_Scopep};
            funcp->slow(সত্য);
            funcp->isStatic(মিথ্যা);
            funcp->isLoose(সত্য);
            m_শীর্ষ_Scopep->addBlocksp(funcp);
            জন্য (uint32_t code = 0; code < m_activity_সংখ্যা; ++code) {
                Ast_গ্রন্থি* ধ্রুবক setterp = newActivitySetter(insertp, code);
                funcp->addStmtsp(setterp);
            }
            m_actAllFuncp = funcp;
        }
        AstCCall* ধ্রুবক callp = নতুন AstCCall{insertp->fileline(), m_actAllFuncp};
        callp->dtypeSetVoid();
        ফেরত callp->makeStmt();
    }

    শূন্য createActivityFlags() {
        // Assign final activity numbers
        m_activity_সংখ্যা = assignactivityNumbers();

        // Create an array of bytes, not a bit vector, as they can be set
        // atomically by mtasks, and are cheaper to set (no need for
        // read-modify-write on the C type), and the speed of the tracing code
        // is the same on largish designs.
        File_সারি* ধ্রুবক flp = m_শীর্ষ_Scopep->fileline();
        Ast_গ্রন্থি_DType* ধ্রুবক newScalarDtp = নতুন AstBasicDType{flp, VFlagBitPacked{}, 1};
        v3Global.rootp()->typeTablep()->addTypesp(newScalarDtp);
        AstRange* ধ্রুবক newArange
            = নতুন AstRange{flp, VNumRange{static_cast<পূর্ণসংখ্যা>(m_activity_সংখ্যা) - 1, 0}};
        Ast_গ্রন্থি_DType* ধ্রুবক newArrDtp = নতুন AstUnpackArrayDType{flp, newScalarDtp, newArange};
        v3Global.rootp()->typeTablep()->addTypesp(newArrDtp);
        m_activityVscp = v3Global.rootp()->শীর্ষ_Scopep()->create_অস্থায়ী("__VtraceActivity", newArrDtp);

        // Insert activity setters
        জন্য (ধ্রুবক V3GraphVertex& vtx : m_graph.vertices()) {
            যদি (ধ্রুবক TraceActivityVertex* ধ্রুবক vtxp = vtx.cast<ধ্রুবক TraceActivityVertex>()) {
                Ast_গ্রন্থি* setterp = nullptr;
                যদি (vtxp->activitySlow()) {
                    setterp = newActivityAll(vtxp->insertp());
                    // Just set all flags in slow code as it should be rare.
                    // This will be rolled up into a loop by V3Reloop.
                } অন্যথা যদি (!vtxp->activityAlways()) {
                    setterp = newActivitySetter(vtxp->insertp(), vtxp->activityCode());
                }
                যদি (setterp) {
                    Ast_গ্রন্থি* ধ্রুবক insertp = vtxp->insertp();
                    যদি (Ast_Stmt_রাশিমালা* ধ্রুবক stmtp = VN_CAST(insertp, Stmt_রাশিমালা)) {
                        stmtp->add_পরবর্তী_Here(setterp);
                    } অন্যথা যদি (AstCFunc* ধ্রুবক funcp = VN_CAST(insertp, CFunc)) {
                        // If there are awaits, insert the setter after each await
                        যদি (funcp->isCoroutine() && funcp->stmtsp()) {
                            funcp->stmtsp()->foreach_And_পরবর্তী([setterp](AstCAwait* awaitp) {
                                awaitp->add_পরবর্তী_Here(setterp->clone_বৃক্ষ(মিথ্যা));
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
                       uint32_t baseCode = 0, ধ্রুবক AstTraceDecl* ধ্রুবক declp = nullptr,
                       যৌক্তিক declSub = মিথ্যা) {
        // Create new function
        ধ্রুবক যৌক্তিক is_শীর্ষ_Func = !declp && শীর্ষ_Funcp == nullptr;
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

        File_সারি* ধ্রুবক flp = m_শীর্ষ_Scopep->fileline();
        AstCFunc* ধ্রুবক funcp = নতুন AstCFunc{flp, funcName, m_শীর্ষ_Scopep};
        funcp->isTrace(সত্য);
        funcp->dontCombine(সত্য);
        funcp->isLoose(সত্য);
        funcp->slow(traceType != VTraceType::CHANGE);
        funcp->isStatic(is_শীর্ষ_Func);
        // Add it to top scope
        m_শীর্ষ_Scopep->addBlocksp(funcp);
        ধ্রুবক std::string bufArg
            = v3Global.opt.traceClassBase() + "::Buffer* bufp"
              + (declp ? (", uint32_t offset, const " + declp->dtypep()->cType("", সত্য, সত্য)
                          + " __VdtypeVar")
                       : "");
        যদি (is_শীর্ষ_Func) {
            // Top functions
            funcp->argTypes("void* voidSelf, " + bufArg);
            funcp->addStmtsp(নতুন AstCStmt{flp, EmitCUtil::voidSelfAssign(m_শীর্ষ_Modp)});
            funcp->addStmtsp(নতুন AstCStmt{flp, EmitCUtil::symClassAssign()});
            // Add global activity check to change dump functions
            যদি (traceType == VTraceType::CHANGE) {  //
                funcp->addStmtsp(
                    নতুন AstCStmt{flp, "if (VL_UNLIKELY(!vlSymsp->__Vm_activity)) return;"});
            }
            // Register function
            AstCStmt* ধ্রুবক cstmtp = নতুন AstCStmt{flp};
            m_regFuncp->addStmtsp(cstmtp);
            যদি (traceType == VTraceType::CONSTANT) {
                cstmtp->add("tracep->addConstCb(");
                cstmtp->add(নতুন AstAddrOfCFunc{flp, funcp});
                cstmtp->add(", " + std::to_string(func_অঙ্ক) + ", vlSelf);");
            } অন্যথা যদি (traceType == VTraceType::FULL) {
                cstmtp->add("tracep->addFullCb(");
                cstmtp->add(নতুন AstAddrOfCFunc{flp, funcp});
                cstmtp->add(", " + std::to_string(func_অঙ্ক) + ", vlSelf);");
            } অন্যথা {
                cstmtp->add("tracep->addChgCb(");
                cstmtp->add(নতুন AstAddrOfCFunc{flp, funcp});
                cstmtp->add(", " + std::to_string(func_অঙ্ক) + ", vlSelf);");
            }
        } অন্যথা {
            // Sub functions
            funcp->argTypes(bufArg);
            // Setup base references. Note in rare occasions we can end up with an empty trace
            // sub function, hence the VL_ATTR_UNUSED attributes.
            যদি (traceType != VTraceType::CHANGE) {
                // Full dump sub function
                funcp->addStmtsp(নতুন AstCStmt{flp,  //
                                              string("uint32_t* const oldp VL_ATTR_UNUSED = "
                                                     "bufp->oldp(vlSymsp->__Vm_baseCode")
                                                  + (declp ? " + offset" : "") + ");\n"});
            } অন্যথা {
                // Change dump sub function
                funcp->addStmtsp(নতুন AstCStmt{flp,  //
                                              "uint32_t* const oldp VL_ATTR_UNUSED = "
                                              "bufp->oldp(vlSymsp->__Vm_baseCode + "
                                                  + (declp ? " offset" : cvtToStr(baseCode))
                                                  + ");\n"});
            }
            যদি (!declp) {
                // Add call to top function
                AstCCall* ধ্রুবক callp = নতুন AstCCall{funcp->fileline(), funcp};
                callp->dtypeSetVoid();
                callp->argTypes("bufp");
                শীর্ষ_Funcp->addStmtsp(callp->makeStmt());
            }
        }
        // Done
        UINFO(5, "  newCFunc " << funcp);
        ফেরত funcp;
    }

    AstCFunc* newCDtypeSubFunc(VTraceType traceType, ধ্রুবক AstTraceDecl* ধ্রুবক declp,
                               AstCFunc* parentp) {
        AstCFunc* ধ্রুবক funcp = newCFunc(traceType, nullptr, 0, 0, declp, সত্য);
        File_সারি* ধ্রুবক flp = m_শীর্ষ_Scopep->fileline();
        AstCCall* ধ্রুবক callp = নতুন AstCCall{flp, funcp};
        callp->dtypeSetVoid();
        callp->argTypes("bufp, offset, __VdtypeVar");
        parentp->addStmtsp(callp->makeStmt());
        ফেরত funcp;
    }

    AstCFunc* createConstDtypeTraceFunctions(ধ্রুবক AstTraceDecl* declp) {
        ধ্রুবক Ast_গ্রন্থি_DType* ধ্রুবক dtypep = declp->dtypep()->skipRefp();
        auto pair = m_dtypeConstFuncs.emplace(dtypep, nullptr);
        যদি (pair.second) {
            File_সারি* ধ্রুবক flp = declp->fileline();
            AstCFunc* ধ্রুবক funcp = newCFunc(VTraceType::CONSTANT, nullptr, 0, 0, declp);

            যৌক্তিক first = সত্য;
            জন্য (Ast_গ্রন্থি* callStmtp = declp->dtypeCallp()->funcp()->stmtsp(); callStmtp;
                 callStmtp = callStmtp->nextp()) {
                ধ্রুবক AstCCall* ধ্রুবক callp
                    = VN_CAST(VN_CAST(callStmtp, Stmt_রাশিমালা)->exprp(), CCall);
                যৌক্তিক onlyOne = first && !callStmtp->nextp();
                AstCFunc* ধ্রুবক subFuncp
                    = onlyOne ? funcp : newCDtypeSubFunc(VTraceType::CONSTANT, declp, funcp);
                জন্য (Ast_গ্রন্থি* stmtp = callp->funcp()->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    যদি (AstTraceDecl* ধ্রুবক fieldDeclp = VN_CAST(stmtp, TraceDecl)) {
                        AstTraceInc* ধ্রুবক incp
                            = নতুন AstTraceInc{flp, fieldDeclp, VTraceType::CONSTANT};
                        subFuncp->addStmtsp(incp);
                    }
                }
                first = মিথ্যা;
            }

            pair.first->second = funcp;
        }

        ফেরত pair.first->second;
    }

    শূন্য createConstTraceFunctions(ধ্রুবক TraceVec& traces) {
        ধ্রুবক পূর্ণসংখ্যা splitLimit = v3Global.opt.outputSplitCTrace() ? v3Global.opt.outputSplitCTrace()
                                                                : std::numeric_limits<পূর্ণসংখ্যা>::max();

        AstCFunc* ধ্রুবক শীর্ষ_Funcp = newCFunc(VTraceType::CONSTANT, nullptr, 0);
        uint32_t sub_Func_অঙ্ক = 0;
        AstCFunc* subFuncp = nullptr;
        পূর্ণসংখ্যা subStmts = 0;
        std::vector<ধ্রুবক TraceTraceVertex*> duplicates;
        জন্য (auto it = traces.cbegin(); it != traces.end(); ++it) {
            ধ্রুবক TraceTraceVertex* ধ্রুবক vtxp = it->second;
            AstTraceDecl* ধ্রুবক declp = vtxp->nodep();
            যদি (vtxp->duplicatep()) {
                duplicates.ঠেলো_back(vtxp);
                চালু;
            }

            // This is a canonical trace node. Assign trace code (signal number).
            UASSERT_OBJ(!declp->codeAssigned(), declp,
                        "Canonical node should not have code assigned yet");
            declp->code(m_code);
            ধ্রুবক uint32_t codeInc = declp->codeInc();
            m_code += codeInc;
            ++m_statUniqSigs;

            // If this is a const signal, add the AstTraceInc
            ধ্রুবক ActCodeSet& actSet = it->first;
            যদি (actSet.গুনতি(TraceActivityVertex::ACTIVITY_NEVER)) {
                // Crate new sub function if required
                যদি (!subFuncp || subStmts > splitLimit) {
                    subStmts = 0;
                    subFuncp = newCFunc(VTraceType::CONSTANT, শীর্ষ_Funcp, sub_Func_অঙ্ক);
                    ++sub_Func_অঙ্ক;
                }
                File_সারি* ধ্রুবক flp = declp->fileline();
                যদি (declp->dtypeCallp()) {
                    AstCFunc* ধ্রুবক funcp = createConstDtypeTraceFunctions(declp);
                    Ast_গ্রন্থি_রাশিমালা* argsp = nullptr;
                    argsp = Ast_গ্রন্থি::add_পরবর্তী(argsp, declp->valuep()->clone_বৃক্ষ(মিথ্যা));
                    AstCCall* ধ্রুবক callp = নতুন AstCCall{flp, funcp, argsp};
                    callp->dtypeSetVoid();
                    callp->argTypes(callp->argTypes() + "bufp, " + std::to_string(declp->code()));
                    subFuncp->addStmtsp(callp->makeStmt());

                    subStmts += 1;
                } অন্যথা {
                    AstTraceInc* ধ্রুবক incp = নতুন AstTraceInc{flp, declp, VTraceType::CONSTANT};
                    subFuncp->addStmtsp(incp);
                    subStmts += incp->গ্রন্থি_গুনতি();
                }
            }
        }

        // Assign codes to duplicate nodes from their canonicals
        জন্য (ধ্রুবক TraceTraceVertex* ধ্রুবক vtxp : duplicates) {
            AstTraceDecl* ধ্রুবক declp = vtxp->nodep();
            ধ্রুবক TraceTraceVertex* ধ্রুবক canonVtxp = vtxp->duplicatep();
            ধ্রুবক AstTraceDecl* ধ্রুবক canonDeclp = canonVtxp->nodep();
            UASSERT_OBJ(!canonVtxp->duplicatep(), canonDeclp, "Canonical node is a duplicate");
            UASSERT_OBJ(canonDeclp->codeAssigned(), canonDeclp,
                        "Canonical node should have code assigned already");
            declp->code(canonDeclp->code() + vtxp->dtypeAliasOffset());
        }
    }

    DtypeFuncs createNonConstDtypeTraceFunctions(ধ্রুবক AstTraceDecl* declp) {
        Ast_গ্রন্থি_DType* dtypep = declp->dtypep()->skipRefp();
        auto pair = m_dtypeNonConstFuncs.emplace(dtypep, DtypeFuncs{});
        যদি (pair.second) {
            File_সারি* ধ্রুবক flp = declp->fileline();
            AstCFunc* ধ্রুবক fullFuncp = newCFunc(VTraceType::FULL, nullptr, 0, 0, declp);
            AstCFunc* ধ্রুবক chgFuncp = newCFunc(VTraceType::CHANGE, nullptr, 0, 0, declp);

            যৌক্তিক first = সত্য;
            জন্য (Ast_গ্রন্থি* callStmtp = declp->dtypeCallp()->funcp()->stmtsp(); callStmtp;
                 callStmtp = callStmtp->nextp()) {
                ধ্রুবক AstCCall* ধ্রুবক callp
                    = VN_CAST(VN_CAST(callStmtp, Stmt_রাশিমালা)->exprp(), CCall);
                যৌক্তিক onlyOne = first && !callStmtp->nextp();
                AstCFunc* ধ্রুবক fullSubFuncp
                    = onlyOne ? fullFuncp : newCDtypeSubFunc(VTraceType::FULL, declp, fullFuncp);
                AstCFunc* ধ্রুবক chgSubFuncp
                    = onlyOne ? chgFuncp : newCDtypeSubFunc(VTraceType::CHANGE, declp, chgFuncp);
                জন্য (Ast_গ্রন্থি* stmtp = callp->funcp()->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    যদি (AstTraceDecl* ধ্রুবক fieldDeclp = VN_CAST(stmtp, TraceDecl)) {
                        AstTraceInc* ধ্রুবক incFullp
                            = নতুন AstTraceInc{flp, fieldDeclp, VTraceType::FULL};
                        fullSubFuncp->addStmtsp(incFullp);
                        AstTraceInc* ধ্রুবক incChgp
                            = নতুন AstTraceInc{flp, fieldDeclp, VTraceType::CHANGE};
                        chgSubFuncp->addStmtsp(incChgp);
                    }
                }
                first = মিথ্যা;
            }

            pair.first->second = {.fullFuncp = fullFuncp, .chgFuncp = chgFuncp};
        }

        ফেরত pair.first->second;
    }

    শূন্য createNonConstTraceFunctions(ধ্রুবক TraceVec& traces, uint32_t nAllCodes,
                                      uint32_t parallelism) {
        ধ্রুবক পূর্ণসংখ্যা splitLimit = v3Global.opt.outputSplitCTrace() ? v3Global.opt.outputSplitCTrace()
                                                                : std::numeric_limits<পূর্ণসংখ্যা>::max();

        // pre-incremented, so starts at 0
        uint32_t শীর্ষ_Func_অঙ্ক = std::numeric_limits<uint32_t>::max();
        TraceVec::const_iterator it = traces.begin();
        যখন (it != traces.end()) {
            AstCFunc* শীর্ষ_Ful_Funcp = nullptr;
            AstCFunc* শীর্ষ_Chg_Funcp = nullptr;
            AstCFunc* subFulFuncp = nullptr;
            AstCFunc* subChgFuncp = nullptr;
            uint32_t sub_Func_অঙ্ক = 0;
            পূর্ণসংখ্যা subStmts = 0;
            ধ্রুবক uint32_t maxCodes = std::max((nAllCodes + parallelism - 1) / parallelism, 1U);
            uint32_t nCodes = 0;
            ধ্রুবক ActCodeSet* পূর্ব_Act_Set = nullptr;
            AstIf* ifp = nullptr;
            uint32_t baseCode = 0;
            জন্য (; nCodes < maxCodes && it != traces.end(); ++it) {
                ধ্রুবক ActCodeSet& actSet = it->first;
                // Traced value never changes, no need to add it
                যদি (actSet.গুনতি(TraceActivityVertex::ACTIVITY_NEVER)) চালু;

                ধ্রুবক TraceTraceVertex* ধ্রুবক vtxp = it->second;
                AstTraceDecl* ধ্রুবক declp = vtxp->nodep();

                // This is a duplicate decl, no need to add it, but must set the
                // function index to the same as the canonical node.
                যদি (ধ্রুবক TraceTraceVertex* ধ্রুবক canonVtxp = vtxp->duplicatep()) {
                    declp->fidx(canonVtxp->nodep()->fidx());
                    চালু;
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
                    পূর্ব_Act_Set = nullptr;
                    ifp = nullptr;
                }

                // If required, create the conditional node checking the activity flags
                যদি (!পূর্ব_Act_Set || actSet != *পূর্ব_Act_Set) {
                    File_সারি* ধ্রুবক flp = m_শীর্ষ_Scopep->fileline();
                    ধ্রুবক যৌক্তিক always = actSet.গুনতি(TraceActivityVertex::ACTIVITY_ALWAYS) != 0;
                    Ast_গ্রন্থি_রাশিমালা* condp = nullptr;
                    যদি (always) {
                        condp = নতুন AstConst{flp, 1};  // Always true, will be folded later
                    } অন্যথা {
                        জন্য (ধ্রুবক uint32_t actCode : actSet) {
                            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক selp = selectActivity(flp, actCode, VAccess::READ);
                            condp = condp ? নতুন AstOr{flp, condp, selp} : selp;
                        }
                    }
                    ifp = নতুন AstIf{flp, condp};
                    যদি (!always) ifp->branchPred(VBranchPred::BP_UNLIKELY);
                    subChgFuncp->addStmtsp(ifp);
                    subStmts += ifp->গ্রন্থি_গুনতি();
                    পূর্ব_Act_Set = &actSet;
                }

                // Add TraceInc nodes
                File_সারি* ধ্রুবক flp = declp->fileline();
                যদি (declp->dtypeCallp()) {
                    DtypeFuncs funcs = createNonConstDtypeTraceFunctions(declp);
                    Ast_গ্রন্থি_রাশিমালা* argsp = nullptr;
                    argsp = Ast_গ্রন্থি::add_পরবর্তী(argsp, declp->valuep()->clone_বৃক্ষ(মিথ্যা));
                    AstCCall* ধ্রুবক callFullp = নতুন AstCCall{flp, funcs.fullFuncp, argsp};
                    callFullp->dtypeSetVoid();
                    callFullp->argTypes(callFullp->argTypes() + "bufp, "
                                        + std::to_string(declp->code()));
                    subFulFuncp->addStmtsp(callFullp->makeStmt());
                    argsp = nullptr;
                    argsp = Ast_গ্রন্থি::add_পরবর্তী(argsp, declp->valuep()->clone_বৃক্ষ(মিথ্যা));
                    AstCCall* ধ্রুবক callChgp = নতুন AstCCall{flp, funcs.chgFuncp, argsp};
                    callChgp->dtypeSetVoid();
                    callChgp->argTypes(callChgp->argTypes() + "bufp, "
                                       + std::to_string(declp->code()));
                    ifp->addThensp(callChgp->makeStmt());

                    subStmts += 2;
                } অন্যথা {
                    AstTraceInc* ধ্রুবক incFulp = নতুন AstTraceInc{flp, declp, VTraceType::FULL};
                    subFulFuncp->addStmtsp(incFulp);
                    AstTraceInc* ধ্রুবক incChgp
                        = নতুন AstTraceInc{flp, declp, VTraceType::CHANGE, baseCode};
                    ifp->addThensp(incChgp);

                    // Set the function index of the decl
                    declp->fidx(শীর্ষ_Func_অঙ্ক);

                    // Track splitting due to size
                    UASSERT_OBJ(incFulp->গ্রন্থি_গুনতি() == incChgp->গ্রন্থি_গুনতি(), declp,
                                "Should have equal cost");
                    ধ্রুবক VNumRange range = declp->arrayRange();
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
        File_সারি* ধ্রুবক fl = m_শীর্ষ_Scopep->fileline();
        AstCFunc* ধ্রুবক cleanupFuncp = নতুন AstCFunc{fl, "trace_cleanup", m_শীর্ষ_Scopep};
        cleanupFuncp->argTypes("void* voidSelf, " + v3Global.opt.traceClassBase()
                               + "* /*unused*/");
        cleanupFuncp->isTrace(সত্য);
        cleanupFuncp->slow(মিথ্যা);
        cleanupFuncp->isStatic(সত্য);
        cleanupFuncp->isLoose(সত্য);
        m_শীর্ষ_Scopep->addBlocksp(cleanupFuncp);
        cleanupFuncp->addStmtsp(নতুন AstCStmt{fl, EmitCUtil::voidSelfAssign(m_শীর্ষ_Modp)});
        cleanupFuncp->addStmtsp(নতুন AstCStmt{fl, EmitCUtil::symClassAssign()});

        // Register it
        {
            AstCStmt* ধ্রুবক cstmtp = নতুন AstCStmt{fl};
            m_regFuncp->addStmtsp(cstmtp);
            cstmtp->add("tracep->addCleanupCb(");
            cstmtp->add(নতুন AstAddrOfCFunc{fl, cleanupFuncp});
            cstmtp->add(", vlSelf);");
        }

        // Clear global activity flag
        cleanupFuncp->addStmtsp(
            নতুন AstCStmt{m_শীর্ষ_Scopep->fileline(), "vlSymsp->__Vm_activity = false;"s});

        // Clear fine grained activity flags
        জন্য (uint32_t i = 0; i < m_activity_সংখ্যা; ++i) {
            Ast_গ্রন্থি* ধ্রুবক clrp = নতুন AstAssign{fl, selectActivity(fl, i, VAccess::WRITE),
                                                নতুন AstConst{fl, AstConst::BitFalse{}}};
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
        m_regFuncp = নতুন AstCFunc{m_শীর্ষ_Scopep->fileline(), "trace_register", m_শীর্ষ_Scopep};
        m_regFuncp->argTypes(v3Global.opt.traceClassBase() + "* tracep");
        m_regFuncp->entryPoint(সত্য);
        m_regFuncp->isTrace(সত্য);
        m_regFuncp->slow(সত্য);
        m_regFuncp->isStatic(মিথ্যা);
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
            জন্য (Ast_গ্রন্থি* blockp = m_শীর্ষ_Scopep->blocksp(); blockp; blockp = blockp->nextp()) {
                AstCFunc* ধ্রুবক funcp = VN_CAST(blockp, CFunc);
                যদি (funcp && VString::startsWith(funcp->name(), "trace_init_leaf_root__")) {
                    মূল_Collector.collect(funcp);
                } অন্যথা যদি (funcp && VString::startsWith(funcp->name(), "trace_init_leaf_top__")) {
                    শীর্ষ_Collector.collect(funcp);
                }
            }

            std::vector<যৌক্তিক> used(শীর্ষ_Declps.আকার_2(), মিথ্যা);
            জন্য (AstTraceDecl* ধ্রুবক মূল_Declp : মূল_Declps) {
                জন্য (আকার_t i = 0; i < শীর্ষ_Declps.আকার_2(); ++i) {
                    যদি (used[i] || !same_মূল_Init_Alias(মূল_Declp, শীর্ষ_Declps[i])) চালু;
                    মূল_Declp->code(শীর্ষ_Declps[i]->code());
                    used[i] = সত্য;
                    বিরতি;
                }
            }
        }

        // Remove refs to traced values from TraceDecl nodes, these have now moved under
        // TraceInc
        জন্য (ধ্রুবক auto& i : traces) {
            Ast_গ্রন্থি* ধ্রুবক valuep = i.second->nodep()->valuep();
            VL_DO_DANGLING(valuep->unlinkFrBack()->delete_বৃক্ষ(), valuep);
        }

        // Create the trace cleanup function clearing the activity flags
        createCleanupFunction();
    }

    TraceCFuncVertex* getCFuncVertexp(AstCFunc* nodep) {
        V3GraphVertex* ধ্রুবক vtxp = nodep->user1u().toGraphVertex();
        TraceCFuncVertex* vertexp = vtxp ? vtxp->cast<TraceCFuncVertex>() : nullptr;
        যদি (!vertexp) {
            vertexp = নতুন TraceCFuncVertex{&m_graph, nodep};
            nodep->user1p(vertexp);
        }
        ফেরত vertexp;
    }
    TraceActivityVertex* getActivityVertexp(Ast_গ্রন্থি* nodep, যৌক্তিক slow) {
        V3GraphVertex* ধ্রুবক vtxp = nodep->user3u().toGraphVertex();
        TraceActivityVertex* vertexp = vtxp ? vtxp->cast<TraceActivityVertex>() : nullptr;
        যদি (!vertexp) {
            vertexp = নতুন TraceActivityVertex{&m_graph, nodep, slow};
            nodep->user3p(vertexp);
        }
        vertexp->slow(slow);
        ফেরত vertexp;
    }

    // VISITORS
    শূন্য visit(AstNetlist* nodep) override {
        // Add vertexes for all TraceDecl, and edges from VARs each trace looks at
        m_finding = মিথ্যা;
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
            যদি (ধ্রুবক AstIface* ধ্রুবক ifacep = nodep->varp()->sensIfacep()) {
                m_ifaceMemberVscps[{ifacep, nodep->varp()->name()}].ঠেলো_back(nodep);
            }
        }
    }
    শূন্য visit(Ast_Stmt_রাশিমালা* nodep) override {
        যদি (!m_finding && !nodep->user2()) {
            যদি (AstCCall* ধ্রুবক callp = VN_CAST(nodep->exprp(), CCall)) {
                UINFO(8, "   CCALL " << callp);
                // See if there are other calls in same statement list;
                // If so, all funcs might share the same activity code
                TraceActivityVertex* ধ্রুবক activityVtxp
                    = getActivityVertexp(nodep, callp->funcp()->slow());
                জন্য (Ast_গ্রন্থি* nextp = nodep; nextp; nextp = nextp->nextp()) {
                    যদি (Ast_Stmt_রাশিমালা* ধ্রুবক stmtp = VN_CAST(nextp, Stmt_রাশিমালা)) {
                        যদি (AstCCall* ধ্রুবক ccallp = VN_CAST(stmtp->exprp(), CCall)) {
                            stmtp->user2(সত্য);  // Processed
                            UINFO(8, "     SubCCALL " << ccallp);
                            V3GraphVertex* ধ্রুবক ccallFuncVtxp = getCFuncVertexp(ccallp->funcp());
                            activityVtxp->slow(ccallp->funcp()->slow());
                            নতুন V3GraphEdge{&m_graph, activityVtxp, ccallFuncVtxp, 1};
                        }
                    }
                }
            }
        }
        iterateChildren(nodep);
    }
    শূন্য visit(AstCFunc* nodep) override {
        UINFO(8, "   CFUNC " << nodep);
        V3GraphVertex* ধ্রুবক funcVtxp = getCFuncVertexp(nodep);
        যদি (!m_finding) {  // If public, we need a unique activity code to allow for sets
                           // directly in this func
            যদি (nodep->funcPublic() || nodep->dpiExportImpl() || nodep->entryPoint()
                || nodep->isCoroutine()) {
                // Cannot treat a coroutine as slow, it may be resumed later
                ধ্রুবক যৌক্তিক slow = nodep->slow() && !nodep->isCoroutine();
                TraceActivityVertex* ধ্রুবক activityVtxp = getActivityVertexp(nodep, slow);
                নতুন V3GraphEdge{&m_graph, activityVtxp, funcVtxp, 1};
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
                ফেরত;
            }
            V3GraphVertex* ধ্রুবক vertexp = নতুন TraceTraceVertex{&m_graph, nodep};
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
        AstVarScope* ধ্রুবক varscopep = nodep->varScopep();
        V3GraphVertex* varVtxp = varscopep->user1u().toGraphVertex();
        যদি (m_tracep) {
            UASSERT_OBJ(nodep->access().isReadOnly(), nodep, "Lvalue in trace?  Should be const.");
            যদি (!varVtxp) {
                varVtxp = নতুন TraceVarVertex{&m_graph, nodep->varScopep()};
                nodep->varScopep()->user1p(varVtxp);
            }
            V3GraphVertex* ধ্রুবক traceVtxp = m_tracep->user1u().toGraphVertex();
            নতুন V3GraphEdge{&m_graph, varVtxp, traceVtxp, 1};
            যদি (nodep->varp()->isPrimaryInish()  // Always need to trace primary inputs
                || nodep->varp()->isSigPublic()) {  // Or ones user can change
                নতুন V3GraphEdge{&m_graph, m_alwaysVtxp, traceVtxp, 1};
            }
            যদি (m_tracep->dtypeCallp()) varscopep->user2p(m_tracep);
        } অন্যথা যদি (m_cfuncp && m_finding && nodep->access().isWriteOrRW()) {
            V3GraphVertex* ধ্রুবক funcVtxp = getCFuncVertexp(m_cfuncp);
            যদি (varVtxp) {  // else we're not tracing this signal
                নতুন V3GraphEdge{&m_graph, funcVtxp, varVtxp, 1};
                AstTraceDecl* ধ্রুবক declp = VN_AS(varscopep->user2p(), TraceDecl);
                যদি (declp) {
                    V3GraphVertex* ধ্রুবক cFuncVtxp = getCFuncVertexp(m_cfuncp);
                    জন্য (ধ্রুবক V3GraphEdge& edge : cFuncVtxp->inEdges()) {
                        V3GraphVertex* ধ্রুবক activityp = edge.fromp();
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
            AstIfaceRefDType* ধ্রুবক dtypep
                = VN_CAST(nodep->fromp()->dtypep()->skipRefp(), IfaceRefDType);
            যদি (dtypep && dtypep->isVirtual()) {
                ধ্রুবক auto it = m_ifaceMemberVscps.find({dtypep->ifacep(), nodep->varp()->name()});
                যদি (it != m_ifaceMemberVscps.end()) {
                    V3GraphVertex* ধ্রুবক funcVtxp = getCFuncVertexp(m_cfuncp);
                    জন্য (AstVarScope* ধ্রুবক vscp : it->second) {
                        V3GraphVertex* varVtxp = vscp->user1u().toGraphVertex();
                        যদি (!varVtxp) {
                            varVtxp = নতুন TraceVarVertex{&m_graph, vscp};
                            vscp->user1p(varVtxp);
                        }
                        নতুন V3GraphEdge{&m_graph, funcVtxp, varVtxp, 1};
                    }
                }
            }
        }
        iterateChildren(nodep);
    }
    //--------------------
    শূন্য visit(Ast_গ্রন্থি* nodep) override { iterateChildren(nodep); }

সর্বজনীন:
    // CONSTRUCTORS
    explicit TraceVisitor(AstNetlist* nodep)
        : m_alwaysVtxp{নতুন TraceActivityVertex{&m_graph, TraceActivityVertex::ACTIVITY_ALWAYS}} {
        nodep->user2_Clear_বৃক্ষ();  // TraceDecl multiple activities flag
        nodep->user3_Clear_বৃক্ষ();  // TraceDecl TraceActivityVertex (assumes we start at nullptr)
        iterate(nodep);
        nodep->foreach([](AstTraceDecl* ধ্রুবক declp) {
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
