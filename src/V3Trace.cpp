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

#અન્તર્ભાવ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#અન્તર્ભાવ "V3Trace.h"

#અન્તર્ભાવ "V3Ast.h"
#અન્તર્ભાવ "V3DupFinder.h"
#અન્તર્ભાવ "V3EmitCBase.h"
#અન્તર્ભાવ "V3Graph.h"
#અન્તર્ભાવ "V3Stats.h"
#અન્તર્ભાવ "V3UniqueNames.h"

#અન્તર્ભાવ <limits>
#અન્તર્ભાવ <map>
#અન્તર્ભાવ <set>
#અન્તર્ભાવ <unordered_map>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Graph vertexes

વર્ગ TraceActivityVertex final : જાહેર V3GraphVertex {
    VL_RTTI_IMPL(TraceActivityVertex, V3GraphVertex)
    Ast_ગાંઠ* અચલ m_insertp;
    int32_t m_activityCode;
    તાર્કિક m_slow;  // If always slow, we can use the same code
જાહેર:
    ગણના { ACTIVITY_NEVER = ((1UL << 31) - 1) };
    ગણના { ACTIVITY_ALWAYS = ((1UL << 31) - 2) };
    ગણના { ACTIVITY_SLOW = 0 };
    TraceActivityVertex(V3Graph* graphp, Ast_ગાંઠ* nodep, તાર્કિક slow)
        : V3GraphVertex{graphp}
        , m_insertp{nodep} {
        m_activityCode = 0;
        m_slow = slow;
    }
    TraceActivityVertex(V3Graph* graphp, int32_t code)
        : V3GraphVertex{graphp}
        , m_insertp{nullptr} {
        m_activityCode = code;
        m_slow = ખોટું;
    }
    ~TraceActivityVertex() override = મૂળભૂત;
    // ACCESSORS
    Ast_ગાંઠ* insertp() અચલ {
        UASSERT(m_insertp, "Null insertp; probably called on a special always/slow");
        પાછા m_insertp;
    }
    string name() અચલ override {
        જો (activityAlways()) {
            પાછા "*ALWAYS*";
        } નહીંતર {
            પાછા std::string{slow() ? "*SLOW* " : ""} + insertp()->name();
        }
    }
    string dotColor() અચલ override { પાછા slow() ? "yellowGreen" : "green"; }
    int32_t activityCode() અચલ { પાછા m_activityCode; }
    તાર્કિક activityAlways() અચલ { પાછા activityCode() == ACTIVITY_ALWAYS; }
    તાર્કિક activitySlow() અચલ { પાછા activityCode() == ACTIVITY_SLOW; }
    શૂન્ય activityCode(int32_t code) { m_activityCode = code; }
    તાર્કિક slow() અચલ { પાછા m_slow; }
    શૂન્ય slow(તાર્કિક flag) {
        જો (!flag) m_slow = ખોટું;
    }
};

વર્ગ TraceCFuncVertex final : જાહેર V3GraphVertex {
    VL_RTTI_IMPL(TraceCFuncVertex, V3GraphVertex)
    AstCFunc* અચલ m_nodep;

જાહેર:
    TraceCFuncVertex(V3Graph* graphp, AstCFunc* nodep)
        : V3GraphVertex{graphp}
        , m_nodep{nodep} {}
    ~TraceCFuncVertex() override = મૂળભૂત;
    // ACCESSORS
    AstCFunc* nodep() અચલ { પાછા m_nodep; }
    string name() અચલ override { પાછા nodep()->name(); }
    string dotColor() અચલ override { પાછા "yellow"; }
    File_લીટી* fileline() અચલ override { પાછા nodep()->fileline(); }
};

વર્ગ TraceTraceVertex final : જાહેર V3GraphVertex {
    VL_RTTI_IMPL(TraceTraceVertex, V3GraphVertex)
    AstTraceDecl* અચલ m_nodep;  // TRACEINC this represents
    // nullptr, or other vertex with the real code() that duplicates this one
    TraceTraceVertex* m_duplicatep = nullptr;
    // When aliasing to a dtype parent, offset of the member within the dtype
    uint32_t m_dtypeAliasOffset = 0;

જાહેર:
    TraceTraceVertex(V3Graph* graphp, AstTraceDecl* nodep)
        : V3GraphVertex{graphp}
        , m_nodep{nodep} {}
    ~TraceTraceVertex() override = મૂળભૂત;
    // ACCESSORS
    AstTraceDecl* nodep() અચલ { પાછા m_nodep; }
    string name() અચલ override { પાછા nodep()->name(); }
    string dotColor() અચલ override { પાછા "red"; }
    File_લીટી* fileline() અચલ override { પાછા nodep()->fileline(); }
    TraceTraceVertex* duplicatep() અચલ { પાછા m_duplicatep; }
    શૂન્ય duplicatep(TraceTraceVertex* dupp) {
        UASSERT_OBJ(!duplicatep(), nodep(), "Assigning duplicatep() to already duplicated node");
        m_duplicatep = dupp;
    }
    શૂન્ય redirectDuplicatep(TraceTraceVertex* dupp) { m_duplicatep = dupp; }
    uint32_t dtypeAliasOffset() અચલ { પાછા m_dtypeAliasOffset; }
    શૂન્ય dtypeAliasOffset(uint32_t offset) { m_dtypeAliasOffset = offset; }
};

વર્ગ TraceVarVertex final : જાહેર V3GraphVertex {
    VL_RTTI_IMPL(TraceVarVertex, V3GraphVertex)
    AstVarScope* અચલ m_nodep;

જાહેર:
    TraceVarVertex(V3Graph* graphp, AstVarScope* nodep)
        : V3GraphVertex{graphp}
        , m_nodep{nodep} {}
    ~TraceVarVertex() override = મૂળભૂત;
    // ACCESSORS
    AstVarScope* nodep() અચલ { પાછા m_nodep; }
    string name() અચલ override { પાછા nodep()->name(); }
    string dotColor() અચલ override { પાછા "skyblue"; }
    File_લીટી* fileline() અચલ override { પાછા nodep()->fileline(); }
};

//######################################################################
// Trace state, as a visitor of each AstNode

વર્ગ TraceVisitor final : જાહેર VNVisitor {
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
    અચલ VNUser1InUse m_inuser1;
    અચલ VNUser2InUse m_inuser2;
    અચલ VNUser3InUse m_inuser3;
    // VNUser4InUse     In V3Hasher via V3DupFinder

    // STATE
    Ast_ગાંઠ_Module* m_ટોચ_Modp = nullptr;  // Module to add variables to
    AstScope* અચલ m_ટોચ_Scopep = v3Global.rootp()->ટોચ_Scopep()->scopep();  // The top AstScope
    AstCFunc* m_cfuncp = nullptr;  // C function adding to graph
    AstCFunc* m_regFuncp = nullptr;  // Trace registration function
    AstCFunc* m_actAllFuncp = nullptr;  // Set all activity function
    AstTraceDecl* m_tracep = nullptr;  // Trace function adding to graph
    AstVarScope* m_activityVscp = nullptr;  // Activity variable
    uint32_t m_activity_સંખ્યા = 0;  // Count of fields in activity variable
    uint32_t m_code = 0;  // Trace ident code# being assigned
    V3Graph m_graph;  // Var/CFunc tracking
    TraceActivityVertex* અચલ m_alwaysVtxp;  // "Always trace" vertex
    તાર્કિક m_finding = ખોટું;  // Pass one of algorithm?
    સંરચના DtypeFuncs final {
    જાહેર:
        AstCFunc* fullFuncp = nullptr;
        AstCFunc* chgFuncp = nullptr;
    };
    std::unordered_map<અચલ Ast_ગાંઠ_DType*, DtypeFuncs>
        m_dtypeNonConstFuncs;  // Full / Chg funcs per type
    std::unordered_map<અચલ Ast_ગાંઠ_DType*, AstCFunc*> m_dtypeConstFuncs;  // Const func per type
    V3UniqueNames m_dtypeNames{""};  // Unique type func names

    // Trace parallelism. Only VCD tracing can be parallelized at this time.
    અચલ uint32_t m_parallelism
        = v3Global.opt.useTraceParallel() ? static_cast<uint32_t>(v3Global.opt.threads()) : 1;

    VDouble0 m_statSetters;  // Statistic tracking
    VDouble0 m_statSettersSlow;  // Statistic tracking
    VDouble0 m_statUniqSigs;  // Statistic tracking

    // All activity numbers applying to a given trace
    using ActCodeSet = std::set<uint32_t>;
    // For activity set, what traces apply
    using TraceVec = std::multimap<ActCodeSet, TraceTraceVertex*>;
    // Candidate interface-member VarScopes keyed by (interface type, member name)
    std::map<std::pair<અચલ AstIface*, std::string>, std::vector<AstVarScope*>>
        m_ifaceMemberVscps;

    વર્ગ TraceInitDeclCollector final : જાહેર VNVisitor {
        std::vector<AstTraceDecl*>& m_declps;  // Output: trace declarations found so far
        std::set<અચલ AstCFunc*> m_seenFuncps;

        શૂન્ય visit(AstTraceDecl* nodep) override { m_declps.ધકેલો_back(nodep); }
        શૂન્ય visit(AstCCall* nodep) override {
            જો (AstCFunc* અચલ funcp = nodep->funcp()) collect(funcp);
        }
        શૂન્ય visit(Ast_ગાંઠ* nodep) override { iterateChildren(nodep); }

    જાહેર:
        explicit TraceInitDeclCollector(std::vector<AstTraceDecl*>& declps)
            : m_declps{declps} {}
        શૂન્ય collect(AstCFunc* funcp) {
            જો (funcp && m_seenFuncps.ઉમેરો(funcp).second) iterate(funcp);
        }
    };

    // METHODS

    સ્થિર તાર્કિક same_મૂળ_Init_Alias(અચલ AstTraceDecl* મૂળ_Declp, અચલ AstTraceDecl* ટોચ_Declp) {
        અચલ VNumRange& મૂળ_Bit_Range = મૂળ_Declp->bitRange();
        અચલ VNumRange& ટોચ_Bit_Range = ટોચ_Declp->bitRange();
        પાછા મૂળ_Declp->showname() == ટોચ_Declp->showname()
               && મૂળ_Declp->declDirection() == ટોચ_Declp->declDirection()
               && મૂળ_Declp->widthMin() == ટોચ_Declp->widthMin()
               && મૂળ_Bit_Range.ranged() == ટોચ_Bit_Range.ranged()
               && (!મૂળ_Bit_Range.ranged()
                   || (મૂળ_Bit_Range.ડાબું() == ટોચ_Bit_Range.ડાબું()
                       && મૂળ_Bit_Range.જમણું() == ટોચ_Bit_Range.જમણું()));
    }

    શૂન્ય detectDuplicates() {
        UINFO(9, "Finding duplicates");
        // Note uses user4
        V3DupFinder dupFinder;  // Duplicate code detection

        // Compute member offsets within dtype instances incrementally.
        // For each dtype member TraceDecl, stores its code offset within the
        // dtype parent's code range.
        std::unordered_map<અચલ AstTraceDecl*, uint32_t> memberOffsets;
        std::unordered_map<અચલ AstTraceDecl*, uint32_t> runningOffset;

        // For fixup pass: track dups redirected to dtype parents, so we can
        // fix them if the parent's user2 is later set during the loop.
        // Maps dup vertex -> original canonical member vertex.
        std::vector<std::pair<TraceTraceVertex*, TraceTraceVertex*>> dtypeParentRedirects;

        // Hash all of the traced values and find if there are any duplicates
        માટે (V3GraphVertex& vtx : m_graph.vertices()) {
            જો (TraceTraceVertex* અચલ vvertexp = vtx.cast<TraceTraceVertex>()) {
                AstTraceDecl* અચલ nodep = vvertexp->nodep();
                જો (nodep->dtypeCallp()) ચાલુ;

                જો (nodep->dtypeDeclp()) {
                    uint32_t& offset = runningOffset[nodep->dtypeDeclp()];
                    memberOffsets[nodep] = offset;
                    offset += nodep->codeInc();
                }

                UASSERT_OBJ(!vvertexp->duplicatep(), nodep, "Should not be a duplicate");
                અચલ auto dupit = dupFinder.findDuplicate(nodep);
                જો (dupit == dupFinder.end()) {
                    dupFinder.ઉમેરો(nodep);
                } નહીંતર {
                    AstTraceDecl* અચલ dupDeclp = VN_AS(dupit->second, TraceDecl);
                    UASSERT_OBJ(dupDeclp, nodep, "Trace duplicate of wrong type");
                    TraceTraceVertex* અચલ dupvertexp
                        = dupDeclp->user1u().toGraphVertex()->cast<TraceTraceVertex>();
                    UINFO(8, "  Orig " << dupDeclp << endl);
                    UINFO(8, "   dup " << nodep << endl);

                    જો (dupDeclp->dtypeDeclp() && !dupDeclp->dtypeDeclp()->user2()) {
                        AstTraceDecl* અચલ dtypeParentp = dupDeclp->dtypeDeclp();
                        TraceTraceVertex* અચલ dtypeVtxp
                            = dtypeParentp->user1u().toGraphVertex()->cast<TraceTraceVertex>();
                        અચલ auto it2 = memberOffsets.find(dupDeclp);
                        UASSERT_OBJ(it2 != memberOffsets.end(), dupDeclp,
                                    "Member offset not precomputed");
                        vvertexp->duplicatep(dtypeVtxp);
                        vvertexp->dtypeAliasOffset(it2->second);
                        dtypeParentRedirects.emplace_back(vvertexp, dupvertexp);
                    } નહીંતર {
                        vvertexp->duplicatep(dupvertexp);
                    }
                    જો (nodep->dtypeDeclp()) nodep->dtypeDeclp()->user2(સાચું);
                }
            }
        }

        માટે (અચલ auto& pair : dtypeParentRedirects) {
            TraceTraceVertex* અચલ dupVtxp = pair.first;
            TraceTraceVertex* અચલ memberVtxp = pair.second;
            અચલ AstTraceDecl* અચલ parentDeclp = dupVtxp->duplicatep()->nodep();
            જો (parentDeclp->user2()) {
                dupVtxp->redirectDuplicatep(memberVtxp);
                dupVtxp->dtypeAliasOffset(0);
            }
        }

        જો (dumpLevel() || debug() >= 9)
            dupFinder.dumpFile(v3Global.debugFilename("trace") + ".hash", ખોટું);
    }

    શૂન્ય graphDtypePrune() {
        માટે (V3GraphVertex* અચલ vtxp : m_graph.vertices().unlinkable()) {
            જો (TraceTraceVertex* અચલ vvertexp = vtxp->cast<TraceTraceVertex>()) {
                AstTraceDecl* અચલ declp = vvertexp->nodep();
                // This skips the dtype sub-func optimization if a var is affected by multiple
                // activities.  We really only need to do this for _chg funcs (and not decls,
                // _const and _full) but it's simpiler to do it all one way or the other.
                જો (declp) {
                    જો (declp->user2() || (declp->dtypeDeclp() && !declp->dtypeDeclp()->user2())) {
                        AstCCall* અચલ callp = declp->dtypeCallp();
                        જો (callp) {
                            Ast_ગાંઠ* stmtexprp = callp->backp();
                            VL_DO_DANGLING(ધકેલો_Deletep(stmtexprp->unlinkFrBack()), stmtexprp);
                        } નહીંતર {
                            તાર્કિક emptyScope;
                            કરો {
                                emptyScope = ખોટું;
                                Ast_ગાંઠ* અચલ declBackp = declp->backp();
                                Ast_ગાંઠ* અચલ declNextp = declp->nextp();
                                જો (VN_IS(declBackp, Trace_ધકેલો_Prefix)
                                    && VN_IS(declNextp, Trace_કાઢો_Prefix)) {
                                    VL_DO_DANGLING(ધકેલો_Deletep(declBackp->unlinkFrBack()),
                                                   declBackp);
                                    VL_DO_DANGLING(ધકેલો_Deletep(declNextp->unlinkFrBack()),
                                                   declNextp);
                                    emptyScope = સાચું;
                                }
                            } જ્યારે (emptyScope);
                        }
                        // Can't purge until we finish this pass
                        ધકેલો_Deletep(declp->unlinkFrBack());  // declp used below
                        vvertexp->rerouteEdges(&m_graph);
                        vvertexp->unlinkDelete(&m_graph);
                    }
                    declp->dtypeDeclp(nullptr);
                }
            }
        }
    }

    શૂન્ય splitTraceDeclFuncs() {
        અચલ પૂર્ણાંક splitLimit = v3Global.opt.outputSplitCTrace() ? v3Global.opt.outputSplitCTrace()
                                                                : std::numeric_limits<પૂર્ણાંક>::max();
        જો (splitLimit == std::numeric_limits<પૂર્ણાંક>::max()) પાછા;

        માટે (Ast_ગાંઠ* nodep = m_ટોચ_Scopep->blocksp(); nodep; nodep = nodep->nextp()) {
            AstCFunc* અચલ funcp = VN_CAST(nodep, CFunc);
            જો (!funcp || !funcp->user2()) ચાલુ;

            અચલ string baseName = funcp->name();
            uint32_t sub_અંક = 1;
            પૂર્ણાંક curSize = 0;
            AstCFunc* curFuncp = funcp;
            std::vector<Ast_ગાંઠ_Stmt*> callStmts;

            auto startNewSubFunc = [&]() {
                File_લીટી* અચલ flp = funcp->fileline();
                અચલ string newName = baseName + "_" + cvtToStr(sub_અંક++);
                AstCFunc* અચલ newFuncp = નવું AstCFunc{flp, newName, m_ટોચ_Scopep};
                newFuncp->argTypes(v3Global.opt.traceClassBase() + "* tracep");
                newFuncp->isTrace(સાચું);
                newFuncp->isStatic(ખોટું);
                newFuncp->isLoose(સાચું);
                newFuncp->slow(સાચું);
                newFuncp->dontCombine(સાચું);
                m_ટોચ_Scopep->addBlocksp(newFuncp);
                newFuncp->addStmtsp(નવું AstCStmt{flp, "const int c = vlSymsp->__Vm_baseCode;"});
                AstCCall* અચલ callp = નવું AstCCall{flp, newFuncp};
                callp->dtypeSetVoid();
                callp->argTypes("tracep");
                callStmts.ધકેલો_back(callp->makeStmt());
                curFuncp = newFuncp;
                curSize = 0;
            };

            માટે (Ast_ગાંઠ *stmtp = funcp->stmtsp(), *nextp; stmtp; stmtp = nextp) {
                nextp = stmtp->nextp();
                curSize += stmtp->ગાંઠ_ગણતરી();

                જો (curFuncp != funcp) {
                    stmtp->unlinkFrBack();
                    curFuncp->addStmtsp(stmtp);
                }

                જો (curSize > splitLimit && nextp) startNewSubFunc();
            }

            માટે (Ast_ગાંઠ_Stmt* અચલ callStmtp : callStmts) { funcp->addStmtsp(callStmtp); }
        }
    }

    શૂન્ય graphSimplify(તાર્કિક initial) {
        જો (initial) {
            // Remove all variable nodes
            માટે (V3GraphVertex* અચલ vtxp : m_graph.vertices().unlinkable()) {
                જો (TraceVarVertex* અચલ vvertexp = vtxp->cast<TraceVarVertex>()) {
                    vvertexp->rerouteEdges(&m_graph);
                    vvertexp->unlinkDelete(&m_graph);
                }
            }
            // Remove multiple variables connecting funcs to traces
            // We do this twice, as then we have fewer edges to multiply out in the below
            // expansion.
            m_graph.removeRedundantEdgesMax(&V3GraphEdge::followAlwaysTrue);
            // Remove all Cfunc nodes
            માટે (V3GraphVertex* અચલ vtxp : m_graph.vertices().unlinkable()) {
                જો (TraceCFuncVertex* અચલ vvertexp = vtxp->cast<TraceCFuncVertex>()) {
                    vvertexp->rerouteEdges(&m_graph);
                    vvertexp->unlinkDelete(&m_graph);
                }
            }
        }

        // Remove multiple variables connecting funcs to traces
        m_graph.removeRedundantEdgesMax(&V3GraphEdge::followAlwaysTrue);

        // If there are any edges from a always, keep only the always
        માટે (V3GraphVertex& vtx : m_graph.vertices()) {
            જો (TraceTraceVertex* અચલ vvertexp = vtx.cast<TraceTraceVertex>()) {
                // Search for the incoming always edge
                અચલ V3GraphEdge* alwaysEdgep = nullptr;
                માટે (અચલ V3GraphEdge& edge : vvertexp->inEdges()) {
                    અચલ TraceActivityVertex* અચલ actVtxp
                        = edge.fromp()->as<અચલ TraceActivityVertex>();
                    જો (actVtxp->activityAlways()) {
                        alwaysEdgep = &edge;
                        તોડો;
                    }
                }
                // If always edge exists, remove all other edges
                જો (alwaysEdgep) {
                    માટે (V3GraphEdge* અચલ edgep : vvertexp->inEdges().unlinkable()) {
                        જો (edgep != alwaysEdgep) VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
                    }
                }
            }
        }

        // Activity points with no outputs can be removed
        માટે (V3GraphVertex* અચલ vtxp : m_graph.vertices().unlinkable()) {
            જો (TraceActivityVertex* અચલ aVtxp = vtxp->cast<TraceActivityVertex>()) {
                // Leave in the always vertex for later use.
                જો (aVtxp != m_alwaysVtxp && aVtxp->outEmpty()) {
                    VL_DO_DANGLING(aVtxp->unlinkDelete(&m_graph), aVtxp);
                }
            }
        }
    }

    uint32_t assignactivityNumbers() {
        uint32_t activity_સંખ્યા = 1;  // Note 0 indicates "slow" only
        માટે (V3GraphVertex& vtx : m_graph.vertices()) {
            જો (TraceActivityVertex* અચલ vvertexp = vtx.cast<TraceActivityVertex>()) {
                જો (vvertexp != m_alwaysVtxp) {
                    જો (vvertexp->slow()) {
                        vvertexp->activityCode(TraceActivityVertex::ACTIVITY_SLOW);
                    } નહીંતર {
                        vvertexp->activityCode(activity_સંખ્યા++);
                    }
                }
            }
        }
        પાછા activity_સંખ્યા;
    }

    શૂન્ય sortTraces(TraceVec& traces, uint32_t& nNonConstCodes) {
        // Populate sort structure
        traces.clear();
        nNonConstCodes = 0;
        માટે (V3GraphVertex& vtx : m_graph.vertices()) {
            જો (TraceTraceVertex* અચલ vtxp = vtx.cast<TraceTraceVertex>()) {
                ActCodeSet actSet;
                UINFO(9, "  Add to sort: " << vtxp);
                UINFOTREE(9, vtxp->nodep(), "", "trnode");
                માટે (અચલ V3GraphEdge& edge : vtxp->inEdges()) {
                    અચલ TraceActivityVertex* અચલ cfvertexp
                        = edge.fromp()->cast<અચલ TraceActivityVertex>();
                    UASSERT_OBJ(cfvertexp, vtxp->nodep(),
                                "Should have been function pointing to this trace");
                    UINFO(9, "   Activity: " << cfvertexp);
                    જો (cfvertexp->activityAlways()) {
                        // If code 0, we always trace; ignore other codes
                        actSet.ઉમેરો(TraceActivityVertex::ACTIVITY_ALWAYS);
                    } નહીંતર {
                        actSet.ઉમેરો(cfvertexp->activityCode());
                    }
                }
                UASSERT_OBJ(actSet.ગણતરી(TraceActivityVertex::ACTIVITY_ALWAYS) == 0
                                || actSet.size() == 1,
                            vtxp->nodep(), "Always active trace has further triggers");
                // Count nodes
                જો (!vtxp->duplicatep() && !actSet.empty())
                    nNonConstCodes += vtxp->nodep()->codeInc();
                જો (actSet.empty()) {
                    // If a trace doesn't have activity, it's constant, and we
                    // don't need to track changes on it.
                    actSet.ઉમેરો(TraceActivityVertex::ACTIVITY_NEVER);
                } નહીંતર જો (actSet.ગણતરી(TraceActivityVertex::ACTIVITY_SLOW) && actSet.size() > 1) {
                    // If a trace depends on the slow flag as well as other
                    // flags, remove the dependency on the slow flag. We will
                    // make slow routines set all activity flags.
                    actSet.erase(TraceActivityVertex::ACTIVITY_SLOW);
                }
                traces.emplace(actSet, vtxp);
            }
        }
    }

    શૂન્ય graphOptimize() {
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
        અચલ auto end = traces.end();
        જ્યારે (it != end) {
            auto head = it;
            // Approximate the complexity of the value change check
            uint32_t complexity = 0;
            અચલ ActCodeSet& actSet = it->first;
            માટે (; it != end && it->first == actSet; ++it) {
                જો (!it->second->duplicatep()) {
                    uint32_t cost = 0;
                    અચલ AstTraceDecl* અચલ declp = it->second->nodep();
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
            જો (actSet.ગણતરી(TraceActivityVertex::ACTIVITY_ALWAYS)) ચાલુ;
            જો (actSet.ગણતરી(TraceActivityVertex::ACTIVITY_NEVER)) ચાલુ;
            જો (actSet.ગણતરી(TraceActivityVertex::ACTIVITY_SLOW)) ચાલુ;
            // If the value comparisons are cheaper to perform than checking the
            // activity flags make the signals always traced. Note this cost
            // equation is heuristic.
            જો (complexity <= actSet.size() * 2) {
                માટે (; head != it; ++head) {
                    નવું V3GraphEdge{&m_graph, m_alwaysVtxp, head->second, 1};
                }
            }
        }

        graphSimplify(ખોટું);
    }

    Ast_ગાંઠ_પદાવલિ* selectActivity(File_લીટી* flp, uint32_t acode, અચલ VAccess& access) {
        પાછા નવું AstArraySel(flp, નવું AstVarRef{flp, m_activityVscp, access}, acode);
    }

    Ast_ગાંઠ* newActivitySetter(Ast_ગાંઠ* insertp, uint32_t code) {
        ++m_statSetters;
        File_લીટી* અચલ fl = insertp->fileline();
        AstAssign* અચલ setterp = નવું AstAssign{fl, selectActivity(fl, code, VAccess::WRITE),
                                                 નવું AstConst{fl, AstConst::BitTrue{}}};
        પાછા setterp;
    }

    Ast_ગાંઠ* newActivityAll(Ast_ગાંઠ* insertp) {
        ++m_statSettersSlow;
        જો (!m_actAllFuncp) {
            File_લીટી* અચલ flp = m_ટોચ_Scopep->fileline();
            AstCFunc* અચલ funcp = નવું AstCFunc{flp, "__VtraceActivitySetAll", m_ટોચ_Scopep};
            funcp->slow(સાચું);
            funcp->isStatic(ખોટું);
            funcp->isLoose(સાચું);
            m_ટોચ_Scopep->addBlocksp(funcp);
            માટે (uint32_t code = 0; code < m_activity_સંખ્યા; ++code) {
                Ast_ગાંઠ* અચલ setterp = newActivitySetter(insertp, code);
                funcp->addStmtsp(setterp);
            }
            m_actAllFuncp = funcp;
        }
        AstCCall* અચલ callp = નવું AstCCall{insertp->fileline(), m_actAllFuncp};
        callp->dtypeSetVoid();
        પાછા callp->makeStmt();
    }

    શૂન્ય createActivityFlags() {
        // Assign final activity numbers
        m_activity_સંખ્યા = assignactivityNumbers();

        // Create an array of bytes, not a bit vector, as they can be set
        // atomically by mtasks, and are cheaper to set (no need for
        // read-modify-write on the C type), and the speed of the tracing code
        // is the same on largish designs.
        File_લીટી* અચલ flp = m_ટોચ_Scopep->fileline();
        Ast_ગાંઠ_DType* અચલ newScalarDtp = નવું AstBasicDType{flp, VFlagBitPacked{}, 1};
        v3Global.rootp()->typeTablep()->addTypesp(newScalarDtp);
        AstRange* અચલ newArange
            = નવું AstRange{flp, VNumRange{static_cast<પૂર્ણાંક>(m_activity_સંખ્યા) - 1, 0}};
        Ast_ગાંઠ_DType* અચલ newArrDtp = નવું AstUnpackArrayDType{flp, newScalarDtp, newArange};
        v3Global.rootp()->typeTablep()->addTypesp(newArrDtp);
        m_activityVscp = v3Global.rootp()->ટોચ_Scopep()->createTemp("__VtraceActivity", newArrDtp);

        // Insert activity setters
        માટે (અચલ V3GraphVertex& vtx : m_graph.vertices()) {
            જો (અચલ TraceActivityVertex* અચલ vtxp = vtx.cast<અચલ TraceActivityVertex>()) {
                Ast_ગાંઠ* setterp = nullptr;
                જો (vtxp->activitySlow()) {
                    setterp = newActivityAll(vtxp->insertp());
                    // Just set all flags in slow code as it should be rare.
                    // This will be rolled up into a loop by V3Reloop.
                } નહીંતર જો (!vtxp->activityAlways()) {
                    setterp = newActivitySetter(vtxp->insertp(), vtxp->activityCode());
                }
                જો (setterp) {
                    Ast_ગાંઠ* અચલ insertp = vtxp->insertp();
                    જો (Ast_Stmt_પદાવલિ* અચલ stmtp = VN_CAST(insertp, Stmt_પદાવલિ)) {
                        stmtp->addNextHere(setterp);
                    } નહીંતર જો (AstCFunc* અચલ funcp = VN_CAST(insertp, CFunc)) {
                        // If there are awaits, insert the setter after each await
                        જો (funcp->isCoroutine() && funcp->stmtsp()) {
                            funcp->stmtsp()->foreachAndNext([setterp](AstCAwait* awaitp) {
                                awaitp->addNextHere(setterp->clone_વૃક્ષ(ખોટું));
                            });
                        }
                        funcp->addStmtsp(setterp);
                    } નહીંતર {
                        insertp->v3fatalSrc("Bad trace activity vertex");
                    }
                }
            }
        }
    }

    AstCFunc* newCFunc(VTraceType traceType, AstCFunc* ટોચ_Funcp, uint32_t func_અંક,
                       uint32_t baseCode = 0, અચલ AstTraceDecl* અચલ declp = nullptr,
                       તાર્કિક declSub = ખોટું) {
        // Create new function
        અચલ તાર્કિક is_ટોચ_Func = !declp && ટોચ_Funcp == nullptr;
        std::string funcName;
        જો (is_ટોચ_Func || declp) {
            funcName = traceType.func_prefix();
        } નહીંતર {
            funcName = ટોચ_Funcp->name();
            funcName += "_sub";
        }
        જો (declp) {
            funcName += "_dtype";
            જો (declSub) funcName += "_sub";
            funcName += "__";
            funcName = m_dtypeNames.get(funcName);
        } નહીંતર {
            funcName += "_";
            funcName += cvtToStr(func_અંક);
        }

        File_લીટી* અચલ flp = m_ટોચ_Scopep->fileline();
        AstCFunc* અચલ funcp = નવું AstCFunc{flp, funcName, m_ટોચ_Scopep};
        funcp->isTrace(સાચું);
        funcp->dontCombine(સાચું);
        funcp->isLoose(સાચું);
        funcp->slow(traceType != VTraceType::CHANGE);
        funcp->isStatic(is_ટોચ_Func);
        // Add it to top scope
        m_ટોચ_Scopep->addBlocksp(funcp);
        અચલ std::string bufArg
            = v3Global.opt.traceClassBase() + "::Buffer* bufp"
              + (declp ? (", uint32_t offset, const " + declp->dtypep()->cType("", સાચું, સાચું)
                          + " __VdtypeVar")
                       : "");
        જો (is_ટોચ_Func) {
            // Top functions
            funcp->argTypes("void* voidSelf, " + bufArg);
            funcp->addStmtsp(નવું AstCStmt{flp, EmitCUtil::voidSelfAssign(m_ટોચ_Modp)});
            funcp->addStmtsp(નવું AstCStmt{flp, EmitCUtil::symClassAssign()});
            // Add global activity check to change dump functions
            જો (traceType == VTraceType::CHANGE) {  //
                funcp->addStmtsp(
                    નવું AstCStmt{flp, "if (VL_UNLIKELY(!vlSymsp->__Vm_activity)) return;"});
            }
            // Register function
            AstCStmt* અચલ cstmtp = નવું AstCStmt{flp};
            m_regFuncp->addStmtsp(cstmtp);
            જો (traceType == VTraceType::CONSTANT) {
                cstmtp->add("tracep->addConstCb(");
                cstmtp->add(નવું AstAddrOfCFunc{flp, funcp});
                cstmtp->add(", " + std::to_string(func_અંક) + ", vlSelf);");
            } નહીંતર જો (traceType == VTraceType::FULL) {
                cstmtp->add("tracep->addFullCb(");
                cstmtp->add(નવું AstAddrOfCFunc{flp, funcp});
                cstmtp->add(", " + std::to_string(func_અંક) + ", vlSelf);");
            } નહીંતર {
                cstmtp->add("tracep->addChgCb(");
                cstmtp->add(નવું AstAddrOfCFunc{flp, funcp});
                cstmtp->add(", " + std::to_string(func_અંક) + ", vlSelf);");
            }
        } નહીંતર {
            // Sub functions
            funcp->argTypes(bufArg);
            // Setup base references. Note in rare occasions we can end up with an empty trace
            // sub function, hence the VL_ATTR_UNUSED attributes.
            જો (traceType != VTraceType::CHANGE) {
                // Full dump sub function
                funcp->addStmtsp(નવું AstCStmt{flp,  //
                                              string("uint32_t* const oldp VL_ATTR_UNUSED = "
                                                     "bufp->oldp(vlSymsp->__Vm_baseCode")
                                                  + (declp ? " + offset" : "") + ");\n"});
            } નહીંતર {
                // Change dump sub function
                funcp->addStmtsp(નવું AstCStmt{flp,  //
                                              "uint32_t* const oldp VL_ATTR_UNUSED = "
                                              "bufp->oldp(vlSymsp->__Vm_baseCode + "
                                                  + (declp ? " offset" : cvtToStr(baseCode))
                                                  + ");\n"});
            }
            જો (!declp) {
                // Add call to top function
                AstCCall* અચલ callp = નવું AstCCall{funcp->fileline(), funcp};
                callp->dtypeSetVoid();
                callp->argTypes("bufp");
                ટોચ_Funcp->addStmtsp(callp->makeStmt());
            }
        }
        // Done
        UINFO(5, "  newCFunc " << funcp);
        પાછા funcp;
    }

    AstCFunc* newCDtypeSubFunc(VTraceType traceType, અચલ AstTraceDecl* અચલ declp,
                               AstCFunc* parentp) {
        AstCFunc* અચલ funcp = newCFunc(traceType, nullptr, 0, 0, declp, સાચું);
        File_લીટી* અચલ flp = m_ટોચ_Scopep->fileline();
        AstCCall* અચલ callp = નવું AstCCall{flp, funcp};
        callp->dtypeSetVoid();
        callp->argTypes("bufp, offset, __VdtypeVar");
        parentp->addStmtsp(callp->makeStmt());
        પાછા funcp;
    }

    AstCFunc* createConstDtypeTraceFunctions(અચલ AstTraceDecl* declp) {
        અચલ Ast_ગાંઠ_DType* અચલ dtypep = declp->dtypep()->skipRefp();
        auto pair = m_dtypeConstFuncs.emplace(dtypep, nullptr);
        જો (pair.second) {
            File_લીટી* અચલ flp = declp->fileline();
            AstCFunc* અચલ funcp = newCFunc(VTraceType::CONSTANT, nullptr, 0, 0, declp);

            તાર્કિક first = સાચું;
            માટે (Ast_ગાંઠ* callStmtp = declp->dtypeCallp()->funcp()->stmtsp(); callStmtp;
                 callStmtp = callStmtp->nextp()) {
                અચલ AstCCall* અચલ callp
                    = VN_CAST(VN_CAST(callStmtp, Stmt_પદાવલિ)->exprp(), CCall);
                તાર્કિક onlyOne = first && !callStmtp->nextp();
                AstCFunc* અચલ subFuncp
                    = onlyOne ? funcp : newCDtypeSubFunc(VTraceType::CONSTANT, declp, funcp);
                માટે (Ast_ગાંઠ* stmtp = callp->funcp()->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    જો (AstTraceDecl* અચલ fieldDeclp = VN_CAST(stmtp, TraceDecl)) {
                        AstTraceInc* અચલ incp
                            = નવું AstTraceInc{flp, fieldDeclp, VTraceType::CONSTANT};
                        subFuncp->addStmtsp(incp);
                    }
                }
                first = ખોટું;
            }

            pair.first->second = funcp;
        }

        પાછા pair.first->second;
    }

    શૂન્ય createConstTraceFunctions(અચલ TraceVec& traces) {
        અચલ પૂર્ણાંક splitLimit = v3Global.opt.outputSplitCTrace() ? v3Global.opt.outputSplitCTrace()
                                                                : std::numeric_limits<પૂર્ણાંક>::max();

        AstCFunc* અચલ ટોચ_Funcp = newCFunc(VTraceType::CONSTANT, nullptr, 0);
        uint32_t sub_Func_અંક = 0;
        AstCFunc* subFuncp = nullptr;
        પૂર્ણાંક subStmts = 0;
        std::vector<અચલ TraceTraceVertex*> duplicates;
        માટે (auto it = traces.cbegin(); it != traces.end(); ++it) {
            અચલ TraceTraceVertex* અચલ vtxp = it->second;
            AstTraceDecl* અચલ declp = vtxp->nodep();
            જો (vtxp->duplicatep()) {
                duplicates.ધકેલો_back(vtxp);
                ચાલુ;
            }

            // This is a canonical trace node. Assign trace code (signal number).
            UASSERT_OBJ(!declp->codeAssigned(), declp,
                        "Canonical node should not have code assigned yet");
            declp->code(m_code);
            અચલ uint32_t codeInc = declp->codeInc();
            m_code += codeInc;
            ++m_statUniqSigs;

            // If this is a const signal, add the AstTraceInc
            અચલ ActCodeSet& actSet = it->first;
            જો (actSet.ગણતરી(TraceActivityVertex::ACTIVITY_NEVER)) {
                // Crate new sub function if required
                જો (!subFuncp || subStmts > splitLimit) {
                    subStmts = 0;
                    subFuncp = newCFunc(VTraceType::CONSTANT, ટોચ_Funcp, sub_Func_અંક);
                    ++sub_Func_અંક;
                }
                File_લીટી* અચલ flp = declp->fileline();
                જો (declp->dtypeCallp()) {
                    AstCFunc* અચલ funcp = createConstDtypeTraceFunctions(declp);
                    Ast_ગાંઠ_પદાવલિ* argsp = nullptr;
                    argsp = Ast_ગાંઠ::addNext(argsp, declp->valuep()->clone_વૃક્ષ(ખોટું));
                    AstCCall* અચલ callp = નવું AstCCall{flp, funcp, argsp};
                    callp->dtypeSetVoid();
                    callp->argTypes(callp->argTypes() + "bufp, " + std::to_string(declp->code()));
                    subFuncp->addStmtsp(callp->makeStmt());

                    subStmts += 1;
                } નહીંતર {
                    AstTraceInc* અચલ incp = નવું AstTraceInc{flp, declp, VTraceType::CONSTANT};
                    subFuncp->addStmtsp(incp);
                    subStmts += incp->ગાંઠ_ગણતરી();
                }
            }
        }

        // Assign codes to duplicate nodes from their canonicals
        માટે (અચલ TraceTraceVertex* અચલ vtxp : duplicates) {
            AstTraceDecl* અચલ declp = vtxp->nodep();
            અચલ TraceTraceVertex* અચલ canonVtxp = vtxp->duplicatep();
            અચલ AstTraceDecl* અચલ canonDeclp = canonVtxp->nodep();
            UASSERT_OBJ(!canonVtxp->duplicatep(), canonDeclp, "Canonical node is a duplicate");
            UASSERT_OBJ(canonDeclp->codeAssigned(), canonDeclp,
                        "Canonical node should have code assigned already");
            declp->code(canonDeclp->code() + vtxp->dtypeAliasOffset());
        }
    }

    DtypeFuncs createNonConstDtypeTraceFunctions(અચલ AstTraceDecl* declp) {
        Ast_ગાંઠ_DType* dtypep = declp->dtypep()->skipRefp();
        auto pair = m_dtypeNonConstFuncs.emplace(dtypep, DtypeFuncs{});
        જો (pair.second) {
            File_લીટી* અચલ flp = declp->fileline();
            AstCFunc* અચલ fullFuncp = newCFunc(VTraceType::FULL, nullptr, 0, 0, declp);
            AstCFunc* અચલ chgFuncp = newCFunc(VTraceType::CHANGE, nullptr, 0, 0, declp);

            તાર્કિક first = સાચું;
            માટે (Ast_ગાંઠ* callStmtp = declp->dtypeCallp()->funcp()->stmtsp(); callStmtp;
                 callStmtp = callStmtp->nextp()) {
                અચલ AstCCall* અચલ callp
                    = VN_CAST(VN_CAST(callStmtp, Stmt_પદાવલિ)->exprp(), CCall);
                તાર્કિક onlyOne = first && !callStmtp->nextp();
                AstCFunc* અચલ fullSubFuncp
                    = onlyOne ? fullFuncp : newCDtypeSubFunc(VTraceType::FULL, declp, fullFuncp);
                AstCFunc* અચલ chgSubFuncp
                    = onlyOne ? chgFuncp : newCDtypeSubFunc(VTraceType::CHANGE, declp, chgFuncp);
                માટે (Ast_ગાંઠ* stmtp = callp->funcp()->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    જો (AstTraceDecl* અચલ fieldDeclp = VN_CAST(stmtp, TraceDecl)) {
                        AstTraceInc* અચલ incFullp
                            = નવું AstTraceInc{flp, fieldDeclp, VTraceType::FULL};
                        fullSubFuncp->addStmtsp(incFullp);
                        AstTraceInc* અચલ incChgp
                            = નવું AstTraceInc{flp, fieldDeclp, VTraceType::CHANGE};
                        chgSubFuncp->addStmtsp(incChgp);
                    }
                }
                first = ખોટું;
            }

            pair.first->second = {.fullFuncp = fullFuncp, .chgFuncp = chgFuncp};
        }

        પાછા pair.first->second;
    }

    શૂન્ય createNonConstTraceFunctions(અચલ TraceVec& traces, uint32_t nAllCodes,
                                      uint32_t parallelism) {
        અચલ પૂર્ણાંક splitLimit = v3Global.opt.outputSplitCTrace() ? v3Global.opt.outputSplitCTrace()
                                                                : std::numeric_limits<પૂર્ણાંક>::max();

        // pre-incremented, so starts at 0
        uint32_t ટોચ_Func_અંક = std::numeric_limits<uint32_t>::max();
        TraceVec::const_iterator it = traces.begin();
        જ્યારે (it != traces.end()) {
            AstCFunc* ટોચ_Ful_Funcp = nullptr;
            AstCFunc* ટોચ_Chg_Funcp = nullptr;
            AstCFunc* subFulFuncp = nullptr;
            AstCFunc* subChgFuncp = nullptr;
            uint32_t sub_Func_અંક = 0;
            પૂર્ણાંક subStmts = 0;
            અચલ uint32_t maxCodes = std::max((nAllCodes + parallelism - 1) / parallelism, 1U);
            uint32_t nCodes = 0;
            અચલ ActCodeSet* prevActSet = nullptr;
            AstIf* ifp = nullptr;
            uint32_t baseCode = 0;
            માટે (; nCodes < maxCodes && it != traces.end(); ++it) {
                અચલ ActCodeSet& actSet = it->first;
                // Traced value never changes, no need to add it
                જો (actSet.ગણતરી(TraceActivityVertex::ACTIVITY_NEVER)) ચાલુ;

                અચલ TraceTraceVertex* અચલ vtxp = it->second;
                AstTraceDecl* અચલ declp = vtxp->nodep();

                // This is a duplicate decl, no need to add it, but must set the
                // function index to the same as the canonical node.
                જો (અચલ TraceTraceVertex* અચલ canonVtxp = vtxp->duplicatep()) {
                    declp->fidx(canonVtxp->nodep()->fidx());
                    ચાલુ;
                }

                // Create top function if not yet created
                જો (!ટોચ_Ful_Funcp) {
                    ++ટોચ_Func_અંક;
                    ટોચ_Ful_Funcp = newCFunc(VTraceType::FULL, nullptr, ટોચ_Func_અંક);
                    ટોચ_Chg_Funcp = newCFunc(VTraceType::CHANGE, nullptr, ટોચ_Func_અંક);
                }

                // Create new sub function if required
                જો (!subFulFuncp || subStmts > splitLimit) {
                    baseCode = declp->code();
                    subStmts = 0;
                    subFulFuncp = newCFunc(VTraceType::FULL, ટોચ_Ful_Funcp, sub_Func_અંક, baseCode);
                    subChgFuncp = newCFunc(VTraceType::CHANGE, ટોચ_Chg_Funcp, sub_Func_અંક, baseCode);
                    ++sub_Func_અંક;
                    prevActSet = nullptr;
                    ifp = nullptr;
                }

                // If required, create the conditional node checking the activity flags
                જો (!prevActSet || actSet != *prevActSet) {
                    File_લીટી* અચલ flp = m_ટોચ_Scopep->fileline();
                    અચલ તાર્કિક always = actSet.ગણતરી(TraceActivityVertex::ACTIVITY_ALWAYS) != 0;
                    Ast_ગાંઠ_પદાવલિ* condp = nullptr;
                    જો (always) {
                        condp = નવું AstConst{flp, 1};  // Always true, will be folded later
                    } નહીંતર {
                        માટે (અચલ uint32_t actCode : actSet) {
                            Ast_ગાંઠ_પદાવલિ* અચલ selp = selectActivity(flp, actCode, VAccess::READ);
                            condp = condp ? નવું AstOr{flp, condp, selp} : selp;
                        }
                    }
                    ifp = નવું AstIf{flp, condp};
                    જો (!always) ifp->branchPred(VBranchPred::BP_UNLIKELY);
                    subChgFuncp->addStmtsp(ifp);
                    subStmts += ifp->ગાંઠ_ગણતરી();
                    prevActSet = &actSet;
                }

                // Add TraceInc nodes
                File_લીટી* અચલ flp = declp->fileline();
                જો (declp->dtypeCallp()) {
                    DtypeFuncs funcs = createNonConstDtypeTraceFunctions(declp);
                    Ast_ગાંઠ_પદાવલિ* argsp = nullptr;
                    argsp = Ast_ગાંઠ::addNext(argsp, declp->valuep()->clone_વૃક્ષ(ખોટું));
                    AstCCall* અચલ callFullp = નવું AstCCall{flp, funcs.fullFuncp, argsp};
                    callFullp->dtypeSetVoid();
                    callFullp->argTypes(callFullp->argTypes() + "bufp, "
                                        + std::to_string(declp->code()));
                    subFulFuncp->addStmtsp(callFullp->makeStmt());
                    argsp = nullptr;
                    argsp = Ast_ગાંઠ::addNext(argsp, declp->valuep()->clone_વૃક્ષ(ખોટું));
                    AstCCall* અચલ callChgp = નવું AstCCall{flp, funcs.chgFuncp, argsp};
                    callChgp->dtypeSetVoid();
                    callChgp->argTypes(callChgp->argTypes() + "bufp, "
                                       + std::to_string(declp->code()));
                    ifp->addThensp(callChgp->makeStmt());

                    subStmts += 2;
                } નહીંતર {
                    AstTraceInc* અચલ incFulp = નવું AstTraceInc{flp, declp, VTraceType::FULL};
                    subFulFuncp->addStmtsp(incFulp);
                    AstTraceInc* અચલ incChgp
                        = નવું AstTraceInc{flp, declp, VTraceType::CHANGE, baseCode};
                    ifp->addThensp(incChgp);

                    // Set the function index of the decl
                    declp->fidx(ટોચ_Func_અંક);

                    // Track splitting due to size
                    UASSERT_OBJ(incFulp->ગાંઠ_ગણતરી() == incChgp->ગાંઠ_ગણતરી(), declp,
                                "Should have equal cost");
                    અચલ VNumRange range = declp->arrayRange();
                    જો (range.ranged()) {
                        // 2x because each element is a TraceInc and a VarRef
                        subStmts += range.elements() * 2;
                    } નહીંતર {
                        subStmts += incChgp->ગાંઠ_ગણતરી();
                    }
                }

                // Track partitioning
                nCodes += declp->codeInc();
            }
        }
    }

    શૂન્ય createCleanupFunction() {
        File_લીટી* અચલ fl = m_ટોચ_Scopep->fileline();
        AstCFunc* અચલ cleanupFuncp = નવું AstCFunc{fl, "trace_cleanup", m_ટોચ_Scopep};
        cleanupFuncp->argTypes("void* voidSelf, " + v3Global.opt.traceClassBase()
                               + "* /*unused*/");
        cleanupFuncp->isTrace(સાચું);
        cleanupFuncp->slow(ખોટું);
        cleanupFuncp->isStatic(સાચું);
        cleanupFuncp->isLoose(સાચું);
        m_ટોચ_Scopep->addBlocksp(cleanupFuncp);
        cleanupFuncp->addStmtsp(નવું AstCStmt{fl, EmitCUtil::voidSelfAssign(m_ટોચ_Modp)});
        cleanupFuncp->addStmtsp(નવું AstCStmt{fl, EmitCUtil::symClassAssign()});

        // Register it
        {
            AstCStmt* અચલ cstmtp = નવું AstCStmt{fl};
            m_regFuncp->addStmtsp(cstmtp);
            cstmtp->add("tracep->addCleanupCb(");
            cstmtp->add(નવું AstAddrOfCFunc{fl, cleanupFuncp});
            cstmtp->add(", vlSelf);");
        }

        // Clear global activity flag
        cleanupFuncp->addStmtsp(
            નવું AstCStmt{m_ટોચ_Scopep->fileline(), "vlSymsp->__Vm_activity = false;"s});

        // Clear fine grained activity flags
        માટે (uint32_t i = 0; i < m_activity_સંખ્યા; ++i) {
            Ast_ગાંઠ* અચલ clrp = નવું AstAssign{fl, selectActivity(fl, i, VAccess::WRITE),
                                                નવું AstConst{fl, AstConst::BitFalse{}}};
            cleanupFuncp->addStmtsp(clrp);
        }
    }

    શૂન્ય createTraceFunctions() {
        // Detect and remove duplicate values
        detectDuplicates();

        graphDtypePrune();
        splitTraceDeclFuncs();
        m_graph.removeRedundantEdgesMax(&V3GraphEdge::followAlwaysTrue);

        // Simplify & optimize the graph
        જો (dumpGraphLevel() >= 6) m_graph.dumpDotFilePrefixed("trace_pre");
        graphSimplify(સાચું);
        જો (dumpGraphLevel() >= 6) m_graph.dumpDotFilePrefixed("trace_simplified");
        graphOptimize();
        જો (dumpGraphLevel() >= 6) m_graph.dumpDotFilePrefixed("trace_optimized");

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
        m_regFuncp = નવું AstCFunc{m_ટોચ_Scopep->fileline(), "trace_register", m_ટોચ_Scopep};
        m_regFuncp->argTypes(v3Global.opt.traceClassBase() + "* tracep");
        m_regFuncp->entryPoint(સાચું);
        m_regFuncp->isTrace(સાચું);
        m_regFuncp->slow(સાચું);
        m_regFuncp->isStatic(ખોટું);
        m_regFuncp->isLoose(સાચું);
        m_ટોચ_Scopep->addBlocksp(m_regFuncp);

        // Create the const dump functions. Also allocates trace codes.
        createConstTraceFunctions(traces);

        // Create the full and incremental dump functions
        createNonConstTraceFunctions(traces, nNonConstCodes, m_parallelism);

        // Root-traced libraries alias wrapper IOs onto the existing top-module codes.
        જો (!v3Global.opt.libCreate().empty()) {
            std::vector<AstTraceDecl*> મૂળ_Declps;
            std::vector<AstTraceDecl*> ટોચ_Declps;
            TraceInitDeclCollector મૂળ_Collector{મૂળ_Declps};
            TraceInitDeclCollector ટોચ_Collector{ટોચ_Declps};
            માટે (Ast_ગાંઠ* blockp = m_ટોચ_Scopep->blocksp(); blockp; blockp = blockp->nextp()) {
                AstCFunc* અચલ funcp = VN_CAST(blockp, CFunc);
                જો (funcp && VString::startsWith(funcp->name(), "trace_init_leaf_root__")) {
                    મૂળ_Collector.collect(funcp);
                } નહીંતર જો (funcp && VString::startsWith(funcp->name(), "trace_init_leaf_top__")) {
                    ટોચ_Collector.collect(funcp);
                }
            }

            std::vector<તાર્કિક> used(ટોચ_Declps.size(), ખોટું);
            માટે (AstTraceDecl* અચલ મૂળ_Declp : મૂળ_Declps) {
                માટે (size_t i = 0; i < ટોચ_Declps.size(); ++i) {
                    જો (used[i] || !same_મૂળ_Init_Alias(મૂળ_Declp, ટોચ_Declps[i])) ચાલુ;
                    મૂળ_Declp->code(ટોચ_Declps[i]->code());
                    used[i] = સાચું;
                    તોડો;
                }
            }
        }

        // Remove refs to traced values from TraceDecl nodes, these have now moved under
        // TraceInc
        માટે (અચલ auto& i : traces) {
            Ast_ગાંઠ* અચલ valuep = i.second->nodep()->valuep();
            VL_DO_DANGLING(valuep->unlinkFrBack()->delete_વૃક્ષ(), valuep);
        }

        // Create the trace cleanup function clearing the activity flags
        createCleanupFunction();
    }

    TraceCFuncVertex* getCFuncVertexp(AstCFunc* nodep) {
        V3GraphVertex* અચલ vtxp = nodep->user1u().toGraphVertex();
        TraceCFuncVertex* vertexp = vtxp ? vtxp->cast<TraceCFuncVertex>() : nullptr;
        જો (!vertexp) {
            vertexp = નવું TraceCFuncVertex{&m_graph, nodep};
            nodep->user1p(vertexp);
        }
        પાછા vertexp;
    }
    TraceActivityVertex* getActivityVertexp(Ast_ગાંઠ* nodep, તાર્કિક slow) {
        V3GraphVertex* અચલ vtxp = nodep->user3u().toGraphVertex();
        TraceActivityVertex* vertexp = vtxp ? vtxp->cast<TraceActivityVertex>() : nullptr;
        જો (!vertexp) {
            vertexp = નવું TraceActivityVertex{&m_graph, nodep, slow};
            nodep->user3p(vertexp);
        }
        vertexp->slow(slow);
        પાછા vertexp;
    }

    // VISITORS
    શૂન્ય visit(AstNetlist* nodep) override {
        // Add vertexes for all TraceDecl, and edges from VARs each trace looks at
        m_finding = ખોટું;
        iterateChildren(nodep);

        // Add vertexes for all CFUNCs, and edges to VARs the func sets
        m_finding = સાચું;
        iterateChildren(nodep);

        // Create the trace functions and insert them into the tree
        createTraceFunctions();

        // Save number of trace codes used
        nodep->nTraceCodes(m_code);
    }
    શૂન્ય visit(Ast_ગાંઠ_Module* nodep) override {
        જો (nodep->is_ટોચ()) m_ટોચ_Modp = nodep;
        iterateChildren(nodep);
    }
    શૂન્ય visit(AstVarScope* nodep) override {
        જો (!m_finding) {
            જો (અચલ AstIface* અચલ ifacep = nodep->varp()->sensIfacep()) {
                m_ifaceMemberVscps[{ifacep, nodep->varp()->name()}].ધકેલો_back(nodep);
            }
        }
    }
    શૂન્ય visit(Ast_Stmt_પદાવલિ* nodep) override {
        જો (!m_finding && !nodep->user2()) {
            જો (AstCCall* અચલ callp = VN_CAST(nodep->exprp(), CCall)) {
                UINFO(8, "   CCALL " << callp);
                // See if there are other calls in same statement list;
                // If so, all funcs might share the same activity code
                TraceActivityVertex* અચલ activityVtxp
                    = getActivityVertexp(nodep, callp->funcp()->slow());
                માટે (Ast_ગાંઠ* nextp = nodep; nextp; nextp = nextp->nextp()) {
                    જો (Ast_Stmt_પદાવલિ* અચલ stmtp = VN_CAST(nextp, Stmt_પદાવલિ)) {
                        જો (AstCCall* અચલ ccallp = VN_CAST(stmtp->exprp(), CCall)) {
                            stmtp->user2(સાચું);  // Processed
                            UINFO(8, "     SubCCALL " << ccallp);
                            V3GraphVertex* અચલ ccallFuncVtxp = getCFuncVertexp(ccallp->funcp());
                            activityVtxp->slow(ccallp->funcp()->slow());
                            નવું V3GraphEdge{&m_graph, activityVtxp, ccallFuncVtxp, 1};
                        }
                    }
                }
            }
        }
        iterateChildren(nodep);
    }
    શૂન્ય visit(AstCFunc* nodep) override {
        UINFO(8, "   CFUNC " << nodep);
        V3GraphVertex* અચલ funcVtxp = getCFuncVertexp(nodep);
        જો (!m_finding) {  // If public, we need a unique activity code to allow for sets
                           // directly in this func
            જો (nodep->funcPublic() || nodep->dpiExportImpl() || nodep->entryPoint()
                || nodep->isCoroutine()) {
                // Cannot treat a coroutine as slow, it may be resumed later
                અચલ તાર્કિક slow = nodep->slow() && !nodep->isCoroutine();
                TraceActivityVertex* અચલ activityVtxp = getActivityVertexp(nodep, slow);
                નવું V3GraphEdge{&m_graph, activityVtxp, funcVtxp, 1};
            }
        }
        VL_RESTORER(m_cfuncp);
        m_cfuncp = nodep;
        iterateChildren(nodep);
    }
    શૂન્ય visit(AstTraceDecl* nodep) override {
        UINFO(8, "   TRACE " << nodep);
        જો (!m_finding && !nodep->inDtypeFunc()) {
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
            જો (m_cfuncp && VString::startsWith(m_cfuncp->name(), "trace_init_leaf_root__")) {
                પાછા;
            }
            V3GraphVertex* અચલ vertexp = નવું TraceTraceVertex{&m_graph, nodep};
            nodep->user1p(vertexp);

            UASSERT_OBJ(m_cfuncp, nodep, "Trace not under func");
            m_cfuncp->user2(સાચું);
            VL_RESTORER(m_tracep);
            m_tracep = nodep;
            iterateChildren(nodep);
        }
    }
    શૂન્ય visit(AstVarRef* nodep) override {
        UASSERT_OBJ(nodep->varScopep(), nodep, "No var scope?");
        AstVarScope* અચલ varscopep = nodep->varScopep();
        V3GraphVertex* varVtxp = varscopep->user1u().toGraphVertex();
        જો (m_tracep) {
            UASSERT_OBJ(nodep->access().isReadOnly(), nodep, "Lvalue in trace?  Should be const.");
            જો (!varVtxp) {
                varVtxp = નવું TraceVarVertex{&m_graph, nodep->varScopep()};
                nodep->varScopep()->user1p(varVtxp);
            }
            V3GraphVertex* અચલ traceVtxp = m_tracep->user1u().toGraphVertex();
            નવું V3GraphEdge{&m_graph, varVtxp, traceVtxp, 1};
            જો (nodep->varp()->isPrimaryInish()  // Always need to trace primary inputs
                || nodep->varp()->isSigPublic()) {  // Or ones user can change
                નવું V3GraphEdge{&m_graph, m_alwaysVtxp, traceVtxp, 1};
            }
            જો (m_tracep->dtypeCallp()) varscopep->user2p(m_tracep);
        } નહીંતર જો (m_cfuncp && m_finding && nodep->access().isWriteOrRW()) {
            V3GraphVertex* અચલ funcVtxp = getCFuncVertexp(m_cfuncp);
            જો (varVtxp) {  // else we're not tracing this signal
                નવું V3GraphEdge{&m_graph, funcVtxp, varVtxp, 1};
                AstTraceDecl* અચલ declp = VN_AS(varscopep->user2p(), TraceDecl);
                જો (declp) {
                    V3GraphVertex* અચલ cFuncVtxp = getCFuncVertexp(m_cfuncp);
                    માટે (અચલ V3GraphEdge& edge : cFuncVtxp->inEdges()) {
                        V3GraphVertex* અચલ activityp = edge.fromp();
                        જો (!declp->user3p()) {
                            declp->user3p(activityp);
                        } નહીંતર જો (declp->user3u().toGraphVertex() != activityp) {
                            declp->user2(સાચું);
                        }
                    }
                }
            }
        }
    }
    શૂન્ય visit(AstMemberSel* nodep) override {
        જો (m_cfuncp && m_finding && nodep->access().isWriteOrRW()) {
            AstIfaceRefDType* અચલ dtypep
                = VN_CAST(nodep->fromp()->dtypep()->skipRefp(), IfaceRefDType);
            જો (dtypep && dtypep->isVirtual()) {
                અચલ auto it = m_ifaceMemberVscps.find({dtypep->ifacep(), nodep->varp()->name()});
                જો (it != m_ifaceMemberVscps.end()) {
                    V3GraphVertex* અચલ funcVtxp = getCFuncVertexp(m_cfuncp);
                    માટે (AstVarScope* અચલ vscp : it->second) {
                        V3GraphVertex* varVtxp = vscp->user1u().toGraphVertex();
                        જો (!varVtxp) {
                            varVtxp = નવું TraceVarVertex{&m_graph, vscp};
                            vscp->user1p(varVtxp);
                        }
                        નવું V3GraphEdge{&m_graph, funcVtxp, varVtxp, 1};
                    }
                }
            }
        }
        iterateChildren(nodep);
    }
    //--------------------
    શૂન્ય visit(Ast_ગાંઠ* nodep) override { iterateChildren(nodep); }

જાહેર:
    // CONSTRUCTORS
    explicit TraceVisitor(AstNetlist* nodep)
        : m_alwaysVtxp{નવું TraceActivityVertex{&m_graph, TraceActivityVertex::ACTIVITY_ALWAYS}} {
        nodep->user2_Clear_વૃક્ષ();  // TraceDecl multiple activities flag
        nodep->user3_Clear_વૃક્ષ();  // TraceDecl TraceActivityVertex (assumes we start at nullptr)
        iterate(nodep);
        nodep->foreach([](AstTraceDecl* અચલ declp) {
            જો (declp->inDtypeFunc()) {
                declp->valuep()->unlinkFrBack()->delete_વૃક્ષ();
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

શૂન્ય V3Trace::traceAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { TraceVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_વૃક્ષ("trace", 0, dump_વૃક્ષ_Either_Level() >= 3);
}
