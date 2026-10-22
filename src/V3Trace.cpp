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

#समावेश "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#समावेश "V3Trace.h"

#समावेश "V3Ast.h"
#समावेश "V3DupFinder.h"
#समावेश "V3EmitCBase.h"
#समावेश "V3Graph.h"
#समावेश "V3Stats.h"
#समावेश "V3UniqueNames.h"

#समावेश <limits>
#समावेश <map>
#समावेश <set>
#समावेश <unordered_map>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Graph vertexes

श्रेणी TraceActivityVertex final : खुला V3GraphVertex {
    VL_RTTI_IMPL(TraceActivityVertex, V3GraphVertex)
    Ast_गाँठ* स्थिर m_insertp;
    int32_t m_activityCode;
    bool m_slow;  // If always slow, we can use the same code
खुला:
    क्रमागत { ACTIVITY_NEVER = ((1UL << 31) - 1) };
    क्रमागत { ACTIVITY_ALWAYS = ((1UL << 31) - 2) };
    क्रमागत { ACTIVITY_SLOW = 0 };
    TraceActivityVertex(V3Graph* graphp, Ast_गाँठ* nodep, bool slow)
        : V3GraphVertex{graphp}
        , m_insertp{nodep} {
        m_activityCode = 0;
        m_slow = slow;
    }
    TraceActivityVertex(V3Graph* graphp, int32_t code)
        : V3GraphVertex{graphp}
        , m_insertp{nullptr} {
        m_activityCode = code;
        m_slow = false;
    }
    ~TraceActivityVertex() override = शेष;
    // ACCESSORS
    Ast_गाँठ* insertp() स्थिर {
        UASSERT(m_insertp, "Null insertp; probably called on a special always/slow");
        वापस m_insertp;
    }
    string name() स्थिर override {
        अगर (activityAlways()) {
            वापस "*ALWAYS*";
        } अन्यथा {
            वापस std::string{slow() ? "*SLOW* " : ""} + insertp()->name();
        }
    }
    string dotColor() स्थिर override { वापस slow() ? "yellowGreen" : "green"; }
    int32_t activityCode() स्थिर { वापस m_activityCode; }
    bool activityAlways() स्थिर { वापस activityCode() == ACTIVITY_ALWAYS; }
    bool activitySlow() स्थिर { वापस activityCode() == ACTIVITY_SLOW; }
    व्योम activityCode(int32_t code) { m_activityCode = code; }
    bool slow() स्थिर { वापस m_slow; }
    व्योम slow(bool flag) {
        अगर (!flag) m_slow = false;
    }
};

श्रेणी TraceCFuncVertex final : खुला V3GraphVertex {
    VL_RTTI_IMPL(TraceCFuncVertex, V3GraphVertex)
    AstCFunc* स्थिर m_nodep;

खुला:
    TraceCFuncVertex(V3Graph* graphp, AstCFunc* nodep)
        : V3GraphVertex{graphp}
        , m_nodep{nodep} {}
    ~TraceCFuncVertex() override = शेष;
    // ACCESSORS
    AstCFunc* nodep() स्थिर { वापस m_nodep; }
    string name() स्थिर override { वापस nodep()->name(); }
    string dotColor() स्थिर override { वापस "yellow"; }
    File_पंक्ति* fileline() स्थिर override { वापस nodep()->fileline(); }
};

श्रेणी TraceTraceVertex final : खुला V3GraphVertex {
    VL_RTTI_IMPL(TraceTraceVertex, V3GraphVertex)
    AstTraceDecl* स्थिर m_nodep;  // TRACEINC this represents
    // nullptr, or other vertex with the real code() that duplicates this one
    TraceTraceVertex* m_duplicatep = nullptr;
    // When aliasing to a dtype parent, offset of the member within the dtype
    uint32_t m_dtypeAliasOffset = 0;

खुला:
    TraceTraceVertex(V3Graph* graphp, AstTraceDecl* nodep)
        : V3GraphVertex{graphp}
        , m_nodep{nodep} {}
    ~TraceTraceVertex() override = शेष;
    // ACCESSORS
    AstTraceDecl* nodep() स्थिर { वापस m_nodep; }
    string name() स्थिर override { वापस nodep()->name(); }
    string dotColor() स्थिर override { वापस "red"; }
    File_पंक्ति* fileline() स्थिर override { वापस nodep()->fileline(); }
    TraceTraceVertex* duplicatep() स्थिर { वापस m_duplicatep; }
    व्योम duplicatep(TraceTraceVertex* dupp) {
        UASSERT_OBJ(!duplicatep(), nodep(), "Assigning duplicatep() to already duplicated node");
        m_duplicatep = dupp;
    }
    व्योम redirectDuplicatep(TraceTraceVertex* dupp) { m_duplicatep = dupp; }
    uint32_t dtypeAliasOffset() स्थिर { वापस m_dtypeAliasOffset; }
    व्योम dtypeAliasOffset(uint32_t offset) { m_dtypeAliasOffset = offset; }
};

श्रेणी TraceVarVertex final : खुला V3GraphVertex {
    VL_RTTI_IMPL(TraceVarVertex, V3GraphVertex)
    AstVarScope* स्थिर m_nodep;

खुला:
    TraceVarVertex(V3Graph* graphp, AstVarScope* nodep)
        : V3GraphVertex{graphp}
        , m_nodep{nodep} {}
    ~TraceVarVertex() override = शेष;
    // ACCESSORS
    AstVarScope* nodep() स्थिर { वापस m_nodep; }
    string name() स्थिर override { वापस nodep()->name(); }
    string dotColor() स्थिर override { वापस "skyblue"; }
    File_पंक्ति* fileline() स्थिर override { वापस nodep()->fileline(); }
};

//######################################################################
// Trace state, as a visitor of each AstNode

श्रेणी TraceVisitor final : खुला VNVisitor {
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
    स्थिर VNUser1InUse m_inuser1;
    स्थिर VNUser2InUse m_inuser2;
    स्थिर VNUser3InUse m_inuser3;
    // VNUser4InUse     In V3Hasher via V3DupFinder

    // STATE
    Ast_गाँठ_Module* m_शीर्ष_Modp = nullptr;  // Module to add variables to
    AstScope* स्थिर m_शीर्ष_Scopep = v3Global.rootp()->शीर्ष_Scopep()->scopep();  // The top AstScope
    AstCFunc* m_cfuncp = nullptr;  // C function adding to graph
    AstCFunc* m_regFuncp = nullptr;  // Trace registration function
    AstCFunc* m_actAllFuncp = nullptr;  // Set all activity function
    AstTraceDecl* m_tracep = nullptr;  // Trace function adding to graph
    AstVarScope* m_activityVscp = nullptr;  // Activity variable
    uint32_t m_activity_संख्या = 0;  // Count of fields in activity variable
    uint32_t m_code = 0;  // Trace ident code# being assigned
    V3Graph m_graph;  // Var/CFunc tracking
    TraceActivityVertex* स्थिर m_alwaysVtxp;  // "Always trace" vertex
    bool m_finding = false;  // Pass one of algorithm?
    काष्ठा DtypeFuncs final {
    खुला:
        AstCFunc* fullFuncp = nullptr;
        AstCFunc* chgFuncp = nullptr;
    };
    std::unordered_map<स्थिर Ast_गाँठ_DType*, DtypeFuncs>
        m_dtypeNonConstFuncs;  // Full / Chg funcs per type
    std::unordered_map<स्थिर Ast_गाँठ_DType*, AstCFunc*> m_dtypeConstFuncs;  // Const func per type
    V3UniqueNames m_dtypeNames{""};  // Unique type func names

    // Trace parallelism. Only VCD tracing can be parallelized at this time.
    स्थिर uint32_t m_parallelism
        = v3Global.opt.useTraceParallel() ? static_cast<uint32_t>(v3Global.opt.threads()) : 1;

    VDouble0 m_statSetters;  // Statistic tracking
    VDouble0 m_statSettersSlow;  // Statistic tracking
    VDouble0 m_statUniqSigs;  // Statistic tracking

    // All activity numbers applying to a given trace
    using ActCodeSet = std::set<uint32_t>;
    // For activity set, what traces apply
    using TraceVec = std::multimap<ActCodeSet, TraceTraceVertex*>;
    // Candidate interface-member VarScopes keyed by (interface type, member name)
    std::map<std::pair<स्थिर AstIface*, std::string>, std::vector<AstVarScope*>>
        m_ifaceMemberVscps;

    श्रेणी TraceInitDeclCollector final : खुला VNVisitor {
        std::vector<AstTraceDecl*>& m_declps;  // Output: trace declarations found so far
        std::set<स्थिर AstCFunc*> m_seenFuncps;

        व्योम visit(AstTraceDecl* nodep) override { m_declps.धकेलो_back(nodep); }
        व्योम visit(AstCCall* nodep) override {
            अगर (AstCFunc* स्थिर funcp = nodep->funcp()) collect(funcp);
        }
        व्योम visit(Ast_गाँठ* nodep) override { iterateChildren(nodep); }

    खुला:
        explicit TraceInitDeclCollector(std::vector<AstTraceDecl*>& declps)
            : m_declps{declps} {}
        व्योम collect(AstCFunc* funcp) {
            अगर (funcp && m_seenFuncps.जोड़ो(funcp).second) iterate(funcp);
        }
    };

    // METHODS

    अटल bool same_मूल_Init_Alias(स्थिर AstTraceDecl* मूल_Declp, स्थिर AstTraceDecl* शीर्ष_Declp) {
        स्थिर VNumRange& मूल_Bit_Range = मूल_Declp->bitRange();
        स्थिर VNumRange& शीर्ष_Bit_Range = शीर्ष_Declp->bitRange();
        वापस मूल_Declp->showname() == शीर्ष_Declp->showname()
               && मूल_Declp->declDirection() == शीर्ष_Declp->declDirection()
               && मूल_Declp->widthMin() == शीर्ष_Declp->widthMin()
               && मूल_Bit_Range.ranged() == शीर्ष_Bit_Range.ranged()
               && (!मूल_Bit_Range.ranged()
                   || (मूल_Bit_Range.बाँए() == शीर्ष_Bit_Range.बाँए()
                       && मूल_Bit_Range.दाँए() == शीर्ष_Bit_Range.दाँए()));
    }

    व्योम detectDuplicates() {
        UINFO(9, "Finding duplicates");
        // Note uses user4
        V3DupFinder dupFinder;  // Duplicate code detection

        // Compute member offsets within dtype instances incrementally.
        // For each dtype member TraceDecl, stores its code offset within the
        // dtype parent's code range.
        std::unordered_map<स्थिर AstTraceDecl*, uint32_t> memberOffsets;
        std::unordered_map<स्थिर AstTraceDecl*, uint32_t> runningOffset;

        // For fixup pass: track dups redirected to dtype parents, so we can
        // fix them if the parent's user2 is later set during the loop.
        // Maps dup vertex -> original canonical member vertex.
        std::vector<std::pair<TraceTraceVertex*, TraceTraceVertex*>> dtypeParentRedirects;

        // Hash all of the traced values and find if there are any duplicates
        क्रम (V3GraphVertex& vtx : m_graph.vertices()) {
            अगर (TraceTraceVertex* स्थिर vvertexp = vtx.cast<TraceTraceVertex>()) {
                AstTraceDecl* स्थिर nodep = vvertexp->nodep();
                अगर (nodep->dtypeCallp()) जारी;

                अगर (nodep->dtypeDeclp()) {
                    uint32_t& offset = runningOffset[nodep->dtypeDeclp()];
                    memberOffsets[nodep] = offset;
                    offset += nodep->codeInc();
                }

                UASSERT_OBJ(!vvertexp->duplicatep(), nodep, "Should not be a duplicate");
                स्थिर स्वतः dupit = dupFinder.findDuplicate(nodep);
                अगर (dupit == dupFinder.end()) {
                    dupFinder.जोड़ो(nodep);
                } अन्यथा {
                    AstTraceDecl* स्थिर dupDeclp = VN_AS(dupit->second, TraceDecl);
                    UASSERT_OBJ(dupDeclp, nodep, "Trace duplicate of wrong type");
                    TraceTraceVertex* स्थिर dupvertexp
                        = dupDeclp->user1u().toGraphVertex()->cast<TraceTraceVertex>();
                    UINFO(8, "  Orig " << dupDeclp << प_पूर्ण);
                    UINFO(8, "   dup " << nodep << प_पूर्ण);

                    अगर (dupDeclp->dtypeDeclp() && !dupDeclp->dtypeDeclp()->user2()) {
                        AstTraceDecl* स्थिर dtypeParentp = dupDeclp->dtypeDeclp();
                        TraceTraceVertex* स्थिर dtypeVtxp
                            = dtypeParentp->user1u().toGraphVertex()->cast<TraceTraceVertex>();
                        स्थिर स्वतः it2 = memberOffsets.find(dupDeclp);
                        UASSERT_OBJ(it2 != memberOffsets.end(), dupDeclp,
                                    "Member offset not precomputed");
                        vvertexp->duplicatep(dtypeVtxp);
                        vvertexp->dtypeAliasOffset(it2->second);
                        dtypeParentRedirects.emplace_back(vvertexp, dupvertexp);
                    } अन्यथा {
                        vvertexp->duplicatep(dupvertexp);
                    }
                    अगर (nodep->dtypeDeclp()) nodep->dtypeDeclp()->user2(true);
                }
            }
        }

        क्रम (स्थिर स्वतः& pair : dtypeParentRedirects) {
            TraceTraceVertex* स्थिर dupVtxp = pair.first;
            TraceTraceVertex* स्थिर memberVtxp = pair.second;
            स्थिर AstTraceDecl* स्थिर parentDeclp = dupVtxp->duplicatep()->nodep();
            अगर (parentDeclp->user2()) {
                dupVtxp->redirectDuplicatep(memberVtxp);
                dupVtxp->dtypeAliasOffset(0);
            }
        }

        अगर (dumpLevel() || debug() >= 9)
            dupFinder.dumpFile(v3Global.debugFilename("trace") + ".hash", false);
    }

    व्योम graphDtypePrune() {
        क्रम (V3GraphVertex* स्थिर vtxp : m_graph.vertices().unlinkable()) {
            अगर (TraceTraceVertex* स्थिर vvertexp = vtxp->cast<TraceTraceVertex>()) {
                AstTraceDecl* स्थिर declp = vvertexp->nodep();
                // This skips the dtype sub-func optimization if a var is affected by multiple
                // activities.  We really only need to do this for _chg funcs (and not decls,
                // _const and _full) but it's simpiler to do it all one way or the other.
                अगर (declp) {
                    अगर (declp->user2() || (declp->dtypeDeclp() && !declp->dtypeDeclp()->user2())) {
                        AstCCall* स्थिर callp = declp->dtypeCallp();
                        अगर (callp) {
                            Ast_गाँठ* stmtexprp = callp->backp();
                            VL_DO_DANGLING(धकेलो_Deletep(stmtexprp->unlinkFrBack()), stmtexprp);
                        } अन्यथा {
                            bool emptyScope;
                            करो {
                                emptyScope = false;
                                Ast_गाँठ* स्थिर declBackp = declp->backp();
                                Ast_गाँठ* स्थिर declNextp = declp->nextp();
                                अगर (VN_IS(declBackp, Trace_धकेलो_Prefix)
                                    && VN_IS(declNextp, Trace_निकालो_Prefix)) {
                                    VL_DO_DANGLING(धकेलो_Deletep(declBackp->unlinkFrBack()),
                                                   declBackp);
                                    VL_DO_DANGLING(धकेलो_Deletep(declNextp->unlinkFrBack()),
                                                   declNextp);
                                    emptyScope = true;
                                }
                            } जबतक (emptyScope);
                        }
                        // Can't purge until we finish this pass
                        धकेलो_Deletep(declp->unlinkFrBack());  // declp used below
                        vvertexp->rerouteEdges(&m_graph);
                        vvertexp->unlinkDelete(&m_graph);
                    }
                    declp->dtypeDeclp(nullptr);
                }
            }
        }
    }

    व्योम splitTraceDeclFuncs() {
        स्थिर पूर्णांक splitLimit = v3Global.opt.outputSplitCTrace() ? v3Global.opt.outputSplitCTrace()
                                                                : std::numeric_limits<पूर्णांक>::max();
        अगर (splitLimit == std::numeric_limits<पूर्णांक>::max()) वापस;

        क्रम (Ast_गाँठ* nodep = m_शीर्ष_Scopep->blocksp(); nodep; nodep = nodep->nextp()) {
            AstCFunc* स्थिर funcp = VN_CAST(nodep, CFunc);
            अगर (!funcp || !funcp->user2()) जारी;

            स्थिर string baseName = funcp->name();
            uint32_t sub_अंक = 1;
            पूर्णांक cur_आकार = 0;
            AstCFunc* curFuncp = funcp;
            std::vector<Ast_गाँठ_Stmt*> callStmts;

            स्वतः startNewSubFunc = [&]() {
                File_पंक्ति* स्थिर flp = funcp->fileline();
                स्थिर string newName = baseName + "_" + cvtToStr(sub_अंक++);
                AstCFunc* स्थिर newFuncp = नया AstCFunc{flp, newName, m_शीर्ष_Scopep};
                newFuncp->argTypes(v3Global.opt.traceClassBase() + "* tracep");
                newFuncp->isTrace(true);
                newFuncp->isStatic(false);
                newFuncp->isLoose(true);
                newFuncp->slow(true);
                newFuncp->dontCombine(true);
                m_शीर्ष_Scopep->addBlocksp(newFuncp);
                newFuncp->addStmtsp(नया AstCStmt{flp, "const int c = vlSymsp->__Vm_baseCode;"});
                AstCCall* स्थिर callp = नया AstCCall{flp, newFuncp};
                callp->dtypeSetVoid();
                callp->argTypes("tracep");
                callStmts.धकेलो_back(callp->makeStmt());
                curFuncp = newFuncp;
                cur_आकार = 0;
            };

            क्रम (Ast_गाँठ *stmtp = funcp->stmtsp(), *nextp; stmtp; stmtp = nextp) {
                nextp = stmtp->nextp();
                cur_आकार += stmtp->गाँठ_गिनती();

                अगर (curFuncp != funcp) {
                    stmtp->unlinkFrBack();
                    curFuncp->addStmtsp(stmtp);
                }

                अगर (cur_आकार > splitLimit && nextp) startNewSubFunc();
            }

            क्रम (Ast_गाँठ_Stmt* स्थिर callStmtp : callStmts) { funcp->addStmtsp(callStmtp); }
        }
    }

    व्योम graphSimplify(bool initial) {
        अगर (initial) {
            // Remove all variable nodes
            क्रम (V3GraphVertex* स्थिर vtxp : m_graph.vertices().unlinkable()) {
                अगर (TraceVarVertex* स्थिर vvertexp = vtxp->cast<TraceVarVertex>()) {
                    vvertexp->rerouteEdges(&m_graph);
                    vvertexp->unlinkDelete(&m_graph);
                }
            }
            // Remove multiple variables connecting funcs to traces
            // We do this twice, as then we have fewer edges to multiply out in the below
            // expansion.
            m_graph.removeRedundantEdgesMax(&V3GraphEdge::followAlwaysTrue);
            // Remove all Cfunc nodes
            क्रम (V3GraphVertex* स्थिर vtxp : m_graph.vertices().unlinkable()) {
                अगर (TraceCFuncVertex* स्थिर vvertexp = vtxp->cast<TraceCFuncVertex>()) {
                    vvertexp->rerouteEdges(&m_graph);
                    vvertexp->unlinkDelete(&m_graph);
                }
            }
        }

        // Remove multiple variables connecting funcs to traces
        m_graph.removeRedundantEdgesMax(&V3GraphEdge::followAlwaysTrue);

        // If there are any edges from a always, keep only the always
        क्रम (V3GraphVertex& vtx : m_graph.vertices()) {
            अगर (TraceTraceVertex* स्थिर vvertexp = vtx.cast<TraceTraceVertex>()) {
                // Search for the incoming always edge
                स्थिर V3GraphEdge* alwaysEdgep = nullptr;
                क्रम (स्थिर V3GraphEdge& edge : vvertexp->inEdges()) {
                    स्थिर TraceActivityVertex* स्थिर actVtxp
                        = edge.fromp()->as<स्थिर TraceActivityVertex>();
                    अगर (actVtxp->activityAlways()) {
                        alwaysEdgep = &edge;
                        अवरोध;
                    }
                }
                // If always edge exists, remove all other edges
                अगर (alwaysEdgep) {
                    क्रम (V3GraphEdge* स्थिर edgep : vvertexp->inEdges().unlinkable()) {
                        अगर (edgep != alwaysEdgep) VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
                    }
                }
            }
        }

        // Activity points with no outputs can be removed
        क्रम (V3GraphVertex* स्थिर vtxp : m_graph.vertices().unlinkable()) {
            अगर (TraceActivityVertex* स्थिर aVtxp = vtxp->cast<TraceActivityVertex>()) {
                // Leave in the always vertex for later use.
                अगर (aVtxp != m_alwaysVtxp && aVtxp->outEmpty()) {
                    VL_DO_DANGLING(aVtxp->unlinkDelete(&m_graph), aVtxp);
                }
            }
        }
    }

    uint32_t assignactivityNumbers() {
        uint32_t activity_संख्या = 1;  // Note 0 indicates "slow" only
        क्रम (V3GraphVertex& vtx : m_graph.vertices()) {
            अगर (TraceActivityVertex* स्थिर vvertexp = vtx.cast<TraceActivityVertex>()) {
                अगर (vvertexp != m_alwaysVtxp) {
                    अगर (vvertexp->slow()) {
                        vvertexp->activityCode(TraceActivityVertex::ACTIVITY_SLOW);
                    } अन्यथा {
                        vvertexp->activityCode(activity_संख्या++);
                    }
                }
            }
        }
        वापस activity_संख्या;
    }

    व्योम sortTraces(TraceVec& traces, uint32_t& nNonConstCodes) {
        // Populate sort structure
        traces.सफा();
        nNonConstCodes = 0;
        क्रम (V3GraphVertex& vtx : m_graph.vertices()) {
            अगर (TraceTraceVertex* स्थिर vtxp = vtx.cast<TraceTraceVertex>()) {
                ActCodeSet actSet;
                UINFO(9, "  Add to sort: " << vtxp);
                UINFOTREE(9, vtxp->nodep(), "", "trnode");
                क्रम (स्थिर V3GraphEdge& edge : vtxp->inEdges()) {
                    स्थिर TraceActivityVertex* स्थिर cfvertexp
                        = edge.fromp()->cast<स्थिर TraceActivityVertex>();
                    UASSERT_OBJ(cfvertexp, vtxp->nodep(),
                                "Should have been function pointing to this trace");
                    UINFO(9, "   Activity: " << cfvertexp);
                    अगर (cfvertexp->activityAlways()) {
                        // If code 0, we always trace; ignore other codes
                        actSet.जोड़ो(TraceActivityVertex::ACTIVITY_ALWAYS);
                    } अन्यथा {
                        actSet.जोड़ो(cfvertexp->activityCode());
                    }
                }
                UASSERT_OBJ(actSet.गिनती(TraceActivityVertex::ACTIVITY_ALWAYS) == 0
                                || actSet.आकार() == 1,
                            vtxp->nodep(), "Always active trace has further triggers");
                // Count nodes
                अगर (!vtxp->duplicatep() && !actSet.empty())
                    nNonConstCodes += vtxp->nodep()->codeInc();
                अगर (actSet.empty()) {
                    // If a trace doesn't have activity, it's constant, and we
                    // don't need to track changes on it.
                    actSet.जोड़ो(TraceActivityVertex::ACTIVITY_NEVER);
                } अन्यथा अगर (actSet.गिनती(TraceActivityVertex::ACTIVITY_SLOW) && actSet.आकार() > 1) {
                    // If a trace depends on the slow flag as well as other
                    // flags, remove the dependency on the slow flag. We will
                    // make slow routines set all activity flags.
                    actSet.erase(TraceActivityVertex::ACTIVITY_SLOW);
                }
                traces.emplace(actSet, vtxp);
            }
        }
    }

    व्योम graphOptimize() {
        // Assign initial activity numbers to activity vertices
        assignactivityNumbers();

        // Sort the traces by activity sets
        TraceVec traces;
        uint32_t unused1;
        sortTraces(traces, unused1);

        // For each activity set with only a small number of signals, make those
        // signals always traced, as it's cheaper to check a few value changes
        // than to test a lot of activity flags
        स्वतः it = traces.begin();
        स्थिर स्वतः end = traces.end();
        जबतक (it != end) {
            स्वतः शीर्ष_2 = it;
            // Approximate the complexity of the value change check
            uint32_t complexity = 0;
            स्थिर ActCodeSet& actSet = it->first;
            क्रम (; it != end && it->first == actSet; ++it) {
                अगर (!it->second->duplicatep()) {
                    uint32_t cost = 0;
                    स्थिर AstTraceDecl* स्थिर declp = it->second->nodep();
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
            अगर (actSet.गिनती(TraceActivityVertex::ACTIVITY_ALWAYS)) जारी;
            अगर (actSet.गिनती(TraceActivityVertex::ACTIVITY_NEVER)) जारी;
            अगर (actSet.गिनती(TraceActivityVertex::ACTIVITY_SLOW)) जारी;
            // If the value comparisons are cheaper to perform than checking the
            // activity flags make the signals always traced. Note this cost
            // equation is heuristic.
            अगर (complexity <= actSet.आकार() * 2) {
                क्रम (; शीर्ष_2 != it; ++शीर्ष_2) {
                    नया V3GraphEdge{&m_graph, m_alwaysVtxp, शीर्ष_2->second, 1};
                }
            }
        }

        graphSimplify(false);
    }

    Ast_गाँठ_व्यंजक* selectActivity(File_पंक्ति* flp, uint32_t acode, स्थिर VAccess& access) {
        वापस नया AstArraySel(flp, नया AstVarRef{flp, m_activityVscp, access}, acode);
    }

    Ast_गाँठ* newActivitySetter(Ast_गाँठ* insertp, uint32_t code) {
        ++m_statSetters;
        File_पंक्ति* स्थिर fl = insertp->fileline();
        AstAssign* स्थिर setterp = नया AstAssign{fl, selectActivity(fl, code, VAccess::WRITE),
                                                 नया AstConst{fl, AstConst::BitTrue{}}};
        वापस setterp;
    }

    Ast_गाँठ* newActivityAll(Ast_गाँठ* insertp) {
        ++m_statSettersSlow;
        अगर (!m_actAllFuncp) {
            File_पंक्ति* स्थिर flp = m_शीर्ष_Scopep->fileline();
            AstCFunc* स्थिर funcp = नया AstCFunc{flp, "__VtraceActivitySetAll", m_शीर्ष_Scopep};
            funcp->slow(true);
            funcp->isStatic(false);
            funcp->isLoose(true);
            m_शीर्ष_Scopep->addBlocksp(funcp);
            क्रम (uint32_t code = 0; code < m_activity_संख्या; ++code) {
                Ast_गाँठ* स्थिर setterp = newActivitySetter(insertp, code);
                funcp->addStmtsp(setterp);
            }
            m_actAllFuncp = funcp;
        }
        AstCCall* स्थिर callp = नया AstCCall{insertp->fileline(), m_actAllFuncp};
        callp->dtypeSetVoid();
        वापस callp->makeStmt();
    }

    व्योम createActivityFlags() {
        // Assign final activity numbers
        m_activity_संख्या = assignactivityNumbers();

        // Create an array of bytes, not a bit vector, as they can be set
        // atomically by mtasks, and are cheaper to set (no need for
        // read-modify-write on the C type), and the speed of the tracing code
        // is the same on largish designs.
        File_पंक्ति* स्थिर flp = m_शीर्ष_Scopep->fileline();
        Ast_गाँठ_DType* स्थिर newScalarDtp = नया AstBasicDType{flp, VFlagBitPacked{}, 1};
        v3Global.rootp()->typeTablep()->addTypesp(newScalarDtp);
        AstRange* स्थिर newArange
            = नया AstRange{flp, VNumRange{static_cast<पूर्णांक>(m_activity_संख्या) - 1, 0}};
        Ast_गाँठ_DType* स्थिर newArrDtp = नया AstUnpackArrayDType{flp, newScalarDtp, newArange};
        v3Global.rootp()->typeTablep()->addTypesp(newArrDtp);
        m_activityVscp = v3Global.rootp()->शीर्ष_Scopep()->create_अस्थायी("__VtraceActivity", newArrDtp);

        // Insert activity setters
        क्रम (स्थिर V3GraphVertex& vtx : m_graph.vertices()) {
            अगर (स्थिर TraceActivityVertex* स्थिर vtxp = vtx.cast<स्थिर TraceActivityVertex>()) {
                Ast_गाँठ* setterp = nullptr;
                अगर (vtxp->activitySlow()) {
                    setterp = newActivityAll(vtxp->insertp());
                    // Just set all flags in slow code as it should be rare.
                    // This will be rolled up into a loop by V3Reloop.
                } अन्यथा अगर (!vtxp->activityAlways()) {
                    setterp = newActivitySetter(vtxp->insertp(), vtxp->activityCode());
                }
                अगर (setterp) {
                    Ast_गाँठ* स्थिर insertp = vtxp->insertp();
                    अगर (Ast_Stmt_व्यंजक* स्थिर stmtp = VN_CAST(insertp, Stmt_व्यंजक)) {
                        stmtp->add_अग्र_Here(setterp);
                    } अन्यथा अगर (AstCFunc* स्थिर funcp = VN_CAST(insertp, CFunc)) {
                        // If there are awaits, insert the setter after each await
                        अगर (funcp->isCoroutine() && funcp->stmtsp()) {
                            funcp->stmtsp()->foreach_And_अग्र([setterp](AstCAwait* awaitp) {
                                awaitp->add_अग्र_Here(setterp->clone_वृक्ष(false));
                            });
                        }
                        funcp->addStmtsp(setterp);
                    } अन्यथा {
                        insertp->v3fatalSrc("Bad trace activity vertex");
                    }
                }
            }
        }
    }

    AstCFunc* newCFunc(VTraceType traceType, AstCFunc* शीर्ष_Funcp, uint32_t func_अंक,
                       uint32_t baseCode = 0, स्थिर AstTraceDecl* स्थिर declp = nullptr,
                       bool declSub = false) {
        // Create new function
        स्थिर bool is_शीर्ष_Func = !declp && शीर्ष_Funcp == nullptr;
        std::string funcName;
        अगर (is_शीर्ष_Func || declp) {
            funcName = traceType.func_prefix();
        } अन्यथा {
            funcName = शीर्ष_Funcp->name();
            funcName += "_sub";
        }
        अगर (declp) {
            funcName += "_dtype";
            अगर (declSub) funcName += "_sub";
            funcName += "__";
            funcName = m_dtypeNames.लो(funcName);
        } अन्यथा {
            funcName += "_";
            funcName += cvtToStr(func_अंक);
        }

        File_पंक्ति* स्थिर flp = m_शीर्ष_Scopep->fileline();
        AstCFunc* स्थिर funcp = नया AstCFunc{flp, funcName, m_शीर्ष_Scopep};
        funcp->isTrace(true);
        funcp->dontCombine(true);
        funcp->isLoose(true);
        funcp->slow(traceType != VTraceType::CHANGE);
        funcp->isStatic(is_शीर्ष_Func);
        // Add it to top scope
        m_शीर्ष_Scopep->addBlocksp(funcp);
        स्थिर std::string bufArg
            = v3Global.opt.traceClassBase() + "::Buffer* bufp"
              + (declp ? (", uint32_t offset, const " + declp->dtypep()->cType("", true, true)
                          + " __VdtypeVar")
                       : "");
        अगर (is_शीर्ष_Func) {
            // Top functions
            funcp->argTypes("void* voidSelf, " + bufArg);
            funcp->addStmtsp(नया AstCStmt{flp, EmitCUtil::voidSelfAssign(m_शीर्ष_Modp)});
            funcp->addStmtsp(नया AstCStmt{flp, EmitCUtil::symClassAssign()});
            // Add global activity check to change dump functions
            अगर (traceType == VTraceType::CHANGE) {  //
                funcp->addStmtsp(
                    नया AstCStmt{flp, "if (VL_UNLIKELY(!vlSymsp->__Vm_activity)) return;"});
            }
            // Register function
            AstCStmt* स्थिर cstmtp = नया AstCStmt{flp};
            m_regFuncp->addStmtsp(cstmtp);
            अगर (traceType == VTraceType::CONSTANT) {
                cstmtp->add("tracep->addConstCb(");
                cstmtp->add(नया AstAddrOfCFunc{flp, funcp});
                cstmtp->add(", " + std::to_string(func_अंक) + ", vlSelf);");
            } अन्यथा अगर (traceType == VTraceType::FULL) {
                cstmtp->add("tracep->addFullCb(");
                cstmtp->add(नया AstAddrOfCFunc{flp, funcp});
                cstmtp->add(", " + std::to_string(func_अंक) + ", vlSelf);");
            } अन्यथा {
                cstmtp->add("tracep->addChgCb(");
                cstmtp->add(नया AstAddrOfCFunc{flp, funcp});
                cstmtp->add(", " + std::to_string(func_अंक) + ", vlSelf);");
            }
        } अन्यथा {
            // Sub functions
            funcp->argTypes(bufArg);
            // Setup base references. Note in rare occasions we can end up with an empty trace
            // sub function, hence the VL_ATTR_UNUSED attributes.
            अगर (traceType != VTraceType::CHANGE) {
                // Full dump sub function
                funcp->addStmtsp(नया AstCStmt{flp,  //
                                              string("uint32_t* const oldp VL_ATTR_UNUSED = "
                                                     "bufp->oldp(vlSymsp->__Vm_baseCode")
                                                  + (declp ? " + offset" : "") + ");\n"});
            } अन्यथा {
                // Change dump sub function
                funcp->addStmtsp(नया AstCStmt{flp,  //
                                              "uint32_t* const oldp VL_ATTR_UNUSED = "
                                              "bufp->oldp(vlSymsp->__Vm_baseCode + "
                                                  + (declp ? " offset" : cvtToStr(baseCode))
                                                  + ");\n"});
            }
            अगर (!declp) {
                // Add call to top function
                AstCCall* स्थिर callp = नया AstCCall{funcp->fileline(), funcp};
                callp->dtypeSetVoid();
                callp->argTypes("bufp");
                शीर्ष_Funcp->addStmtsp(callp->makeStmt());
            }
        }
        // Done
        UINFO(5, "  newCFunc " << funcp);
        वापस funcp;
    }

    AstCFunc* newCDtypeSubFunc(VTraceType traceType, स्थिर AstTraceDecl* स्थिर declp,
                               AstCFunc* parentp) {
        AstCFunc* स्थिर funcp = newCFunc(traceType, nullptr, 0, 0, declp, true);
        File_पंक्ति* स्थिर flp = m_शीर्ष_Scopep->fileline();
        AstCCall* स्थिर callp = नया AstCCall{flp, funcp};
        callp->dtypeSetVoid();
        callp->argTypes("bufp, offset, __VdtypeVar");
        parentp->addStmtsp(callp->makeStmt());
        वापस funcp;
    }

    AstCFunc* createConstDtypeTraceFunctions(स्थिर AstTraceDecl* declp) {
        स्थिर Ast_गाँठ_DType* स्थिर dtypep = declp->dtypep()->skipRefp();
        स्वतः pair = m_dtypeConstFuncs.emplace(dtypep, nullptr);
        अगर (pair.second) {
            File_पंक्ति* स्थिर flp = declp->fileline();
            AstCFunc* स्थिर funcp = newCFunc(VTraceType::CONSTANT, nullptr, 0, 0, declp);

            bool first = true;
            क्रम (Ast_गाँठ* callStmtp = declp->dtypeCallp()->funcp()->stmtsp(); callStmtp;
                 callStmtp = callStmtp->nextp()) {
                स्थिर AstCCall* स्थिर callp
                    = VN_CAST(VN_CAST(callStmtp, Stmt_व्यंजक)->exprp(), CCall);
                bool onlyOne = first && !callStmtp->nextp();
                AstCFunc* स्थिर subFuncp
                    = onlyOne ? funcp : newCDtypeSubFunc(VTraceType::CONSTANT, declp, funcp);
                क्रम (Ast_गाँठ* stmtp = callp->funcp()->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    अगर (AstTraceDecl* स्थिर fieldDeclp = VN_CAST(stmtp, TraceDecl)) {
                        AstTraceInc* स्थिर incp
                            = नया AstTraceInc{flp, fieldDeclp, VTraceType::CONSTANT};
                        subFuncp->addStmtsp(incp);
                    }
                }
                first = false;
            }

            pair.first->second = funcp;
        }

        वापस pair.first->second;
    }

    व्योम createConstTraceFunctions(स्थिर TraceVec& traces) {
        स्थिर पूर्णांक splitLimit = v3Global.opt.outputSplitCTrace() ? v3Global.opt.outputSplitCTrace()
                                                                : std::numeric_limits<पूर्णांक>::max();

        AstCFunc* स्थिर शीर्ष_Funcp = newCFunc(VTraceType::CONSTANT, nullptr, 0);
        uint32_t sub_Func_अंक = 0;
        AstCFunc* subFuncp = nullptr;
        पूर्णांक subStmts = 0;
        std::vector<स्थिर TraceTraceVertex*> duplicates;
        क्रम (स्वतः it = traces.cbegin(); it != traces.end(); ++it) {
            स्थिर TraceTraceVertex* स्थिर vtxp = it->second;
            AstTraceDecl* स्थिर declp = vtxp->nodep();
            अगर (vtxp->duplicatep()) {
                duplicates.धकेलो_back(vtxp);
                जारी;
            }

            // This is a canonical trace node. Assign trace code (signal number).
            UASSERT_OBJ(!declp->codeAssigned(), declp,
                        "Canonical node should not have code assigned yet");
            declp->code(m_code);
            स्थिर uint32_t codeInc = declp->codeInc();
            m_code += codeInc;
            ++m_statUniqSigs;

            // If this is a const signal, add the AstTraceInc
            स्थिर ActCodeSet& actSet = it->first;
            अगर (actSet.गिनती(TraceActivityVertex::ACTIVITY_NEVER)) {
                // Crate new sub function if required
                अगर (!subFuncp || subStmts > splitLimit) {
                    subStmts = 0;
                    subFuncp = newCFunc(VTraceType::CONSTANT, शीर्ष_Funcp, sub_Func_अंक);
                    ++sub_Func_अंक;
                }
                File_पंक्ति* स्थिर flp = declp->fileline();
                अगर (declp->dtypeCallp()) {
                    AstCFunc* स्थिर funcp = createConstDtypeTraceFunctions(declp);
                    Ast_गाँठ_व्यंजक* argsp = nullptr;
                    argsp = Ast_गाँठ::add_अग्र(argsp, declp->valuep()->clone_वृक्ष(false));
                    AstCCall* स्थिर callp = नया AstCCall{flp, funcp, argsp};
                    callp->dtypeSetVoid();
                    callp->argTypes(callp->argTypes() + "bufp, " + std::to_string(declp->code()));
                    subFuncp->addStmtsp(callp->makeStmt());

                    subStmts += 1;
                } अन्यथा {
                    AstTraceInc* स्थिर incp = नया AstTraceInc{flp, declp, VTraceType::CONSTANT};
                    subFuncp->addStmtsp(incp);
                    subStmts += incp->गाँठ_गिनती();
                }
            }
        }

        // Assign codes to duplicate nodes from their canonicals
        क्रम (स्थिर TraceTraceVertex* स्थिर vtxp : duplicates) {
            AstTraceDecl* स्थिर declp = vtxp->nodep();
            स्थिर TraceTraceVertex* स्थिर canonVtxp = vtxp->duplicatep();
            स्थिर AstTraceDecl* स्थिर canonDeclp = canonVtxp->nodep();
            UASSERT_OBJ(!canonVtxp->duplicatep(), canonDeclp, "Canonical node is a duplicate");
            UASSERT_OBJ(canonDeclp->codeAssigned(), canonDeclp,
                        "Canonical node should have code assigned already");
            declp->code(canonDeclp->code() + vtxp->dtypeAliasOffset());
        }
    }

    DtypeFuncs createNonConstDtypeTraceFunctions(स्थिर AstTraceDecl* declp) {
        Ast_गाँठ_DType* dtypep = declp->dtypep()->skipRefp();
        स्वतः pair = m_dtypeNonConstFuncs.emplace(dtypep, DtypeFuncs{});
        अगर (pair.second) {
            File_पंक्ति* स्थिर flp = declp->fileline();
            AstCFunc* स्थिर fullFuncp = newCFunc(VTraceType::FULL, nullptr, 0, 0, declp);
            AstCFunc* स्थिर chgFuncp = newCFunc(VTraceType::CHANGE, nullptr, 0, 0, declp);

            bool first = true;
            क्रम (Ast_गाँठ* callStmtp = declp->dtypeCallp()->funcp()->stmtsp(); callStmtp;
                 callStmtp = callStmtp->nextp()) {
                स्थिर AstCCall* स्थिर callp
                    = VN_CAST(VN_CAST(callStmtp, Stmt_व्यंजक)->exprp(), CCall);
                bool onlyOne = first && !callStmtp->nextp();
                AstCFunc* स्थिर fullSubFuncp
                    = onlyOne ? fullFuncp : newCDtypeSubFunc(VTraceType::FULL, declp, fullFuncp);
                AstCFunc* स्थिर chgSubFuncp
                    = onlyOne ? chgFuncp : newCDtypeSubFunc(VTraceType::CHANGE, declp, chgFuncp);
                क्रम (Ast_गाँठ* stmtp = callp->funcp()->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    अगर (AstTraceDecl* स्थिर fieldDeclp = VN_CAST(stmtp, TraceDecl)) {
                        AstTraceInc* स्थिर incFullp
                            = नया AstTraceInc{flp, fieldDeclp, VTraceType::FULL};
                        fullSubFuncp->addStmtsp(incFullp);
                        AstTraceInc* स्थिर incChgp
                            = नया AstTraceInc{flp, fieldDeclp, VTraceType::CHANGE};
                        chgSubFuncp->addStmtsp(incChgp);
                    }
                }
                first = false;
            }

            pair.first->second = {.fullFuncp = fullFuncp, .chgFuncp = chgFuncp};
        }

        वापस pair.first->second;
    }

    व्योम createNonConstTraceFunctions(स्थिर TraceVec& traces, uint32_t nAllCodes,
                                      uint32_t parallelism) {
        स्थिर पूर्णांक splitLimit = v3Global.opt.outputSplitCTrace() ? v3Global.opt.outputSplitCTrace()
                                                                : std::numeric_limits<पूर्णांक>::max();

        // pre-incremented, so starts at 0
        uint32_t शीर्ष_Func_अंक = std::numeric_limits<uint32_t>::max();
        TraceVec::const_iterator it = traces.begin();
        जबतक (it != traces.end()) {
            AstCFunc* शीर्ष_Ful_Funcp = nullptr;
            AstCFunc* शीर्ष_Chg_Funcp = nullptr;
            AstCFunc* subFulFuncp = nullptr;
            AstCFunc* subChgFuncp = nullptr;
            uint32_t sub_Func_अंक = 0;
            पूर्णांक subStmts = 0;
            स्थिर uint32_t maxCodes = std::max((nAllCodes + parallelism - 1) / parallelism, 1U);
            uint32_t nCodes = 0;
            स्थिर ActCodeSet* पूर्व_Act_Set = nullptr;
            AstIf* ifp = nullptr;
            uint32_t baseCode = 0;
            क्रम (; nCodes < maxCodes && it != traces.end(); ++it) {
                स्थिर ActCodeSet& actSet = it->first;
                // Traced value never changes, no need to add it
                अगर (actSet.गिनती(TraceActivityVertex::ACTIVITY_NEVER)) जारी;

                स्थिर TraceTraceVertex* स्थिर vtxp = it->second;
                AstTraceDecl* स्थिर declp = vtxp->nodep();

                // This is a duplicate decl, no need to add it, but must set the
                // function index to the same as the canonical node.
                अगर (स्थिर TraceTraceVertex* स्थिर canonVtxp = vtxp->duplicatep()) {
                    declp->fidx(canonVtxp->nodep()->fidx());
                    जारी;
                }

                // Create top function if not yet created
                अगर (!शीर्ष_Ful_Funcp) {
                    ++शीर्ष_Func_अंक;
                    शीर्ष_Ful_Funcp = newCFunc(VTraceType::FULL, nullptr, शीर्ष_Func_अंक);
                    शीर्ष_Chg_Funcp = newCFunc(VTraceType::CHANGE, nullptr, शीर्ष_Func_अंक);
                }

                // Create new sub function if required
                अगर (!subFulFuncp || subStmts > splitLimit) {
                    baseCode = declp->code();
                    subStmts = 0;
                    subFulFuncp = newCFunc(VTraceType::FULL, शीर्ष_Ful_Funcp, sub_Func_अंक, baseCode);
                    subChgFuncp = newCFunc(VTraceType::CHANGE, शीर्ष_Chg_Funcp, sub_Func_अंक, baseCode);
                    ++sub_Func_अंक;
                    पूर्व_Act_Set = nullptr;
                    ifp = nullptr;
                }

                // If required, create the conditional node checking the activity flags
                अगर (!पूर्व_Act_Set || actSet != *पूर्व_Act_Set) {
                    File_पंक्ति* स्थिर flp = m_शीर्ष_Scopep->fileline();
                    स्थिर bool always = actSet.गिनती(TraceActivityVertex::ACTIVITY_ALWAYS) != 0;
                    Ast_गाँठ_व्यंजक* condp = nullptr;
                    अगर (always) {
                        condp = नया AstConst{flp, 1};  // Always true, will be folded later
                    } अन्यथा {
                        क्रम (स्थिर uint32_t actCode : actSet) {
                            Ast_गाँठ_व्यंजक* स्थिर selp = selectActivity(flp, actCode, VAccess::READ);
                            condp = condp ? नया AstOr{flp, condp, selp} : selp;
                        }
                    }
                    ifp = नया AstIf{flp, condp};
                    अगर (!always) ifp->branchPred(VBranchPred::BP_UNLIKELY);
                    subChgFuncp->addStmtsp(ifp);
                    subStmts += ifp->गाँठ_गिनती();
                    पूर्व_Act_Set = &actSet;
                }

                // Add TraceInc nodes
                File_पंक्ति* स्थिर flp = declp->fileline();
                अगर (declp->dtypeCallp()) {
                    DtypeFuncs funcs = createNonConstDtypeTraceFunctions(declp);
                    Ast_गाँठ_व्यंजक* argsp = nullptr;
                    argsp = Ast_गाँठ::add_अग्र(argsp, declp->valuep()->clone_वृक्ष(false));
                    AstCCall* स्थिर callFullp = नया AstCCall{flp, funcs.fullFuncp, argsp};
                    callFullp->dtypeSetVoid();
                    callFullp->argTypes(callFullp->argTypes() + "bufp, "
                                        + std::to_string(declp->code()));
                    subFulFuncp->addStmtsp(callFullp->makeStmt());
                    argsp = nullptr;
                    argsp = Ast_गाँठ::add_अग्र(argsp, declp->valuep()->clone_वृक्ष(false));
                    AstCCall* स्थिर callChgp = नया AstCCall{flp, funcs.chgFuncp, argsp};
                    callChgp->dtypeSetVoid();
                    callChgp->argTypes(callChgp->argTypes() + "bufp, "
                                       + std::to_string(declp->code()));
                    ifp->addThensp(callChgp->makeStmt());

                    subStmts += 2;
                } अन्यथा {
                    AstTraceInc* स्थिर incFulp = नया AstTraceInc{flp, declp, VTraceType::FULL};
                    subFulFuncp->addStmtsp(incFulp);
                    AstTraceInc* स्थिर incChgp
                        = नया AstTraceInc{flp, declp, VTraceType::CHANGE, baseCode};
                    ifp->addThensp(incChgp);

                    // Set the function index of the decl
                    declp->fidx(शीर्ष_Func_अंक);

                    // Track splitting due to size
                    UASSERT_OBJ(incFulp->गाँठ_गिनती() == incChgp->गाँठ_गिनती(), declp,
                                "Should have equal cost");
                    स्थिर VNumRange range = declp->arrayRange();
                    अगर (range.ranged()) {
                        // 2x because each element is a TraceInc and a VarRef
                        subStmts += range.elements() * 2;
                    } अन्यथा {
                        subStmts += incChgp->गाँठ_गिनती();
                    }
                }

                // Track partitioning
                nCodes += declp->codeInc();
            }
        }
    }

    व्योम createCleanupFunction() {
        File_पंक्ति* स्थिर fl = m_शीर्ष_Scopep->fileline();
        AstCFunc* स्थिर cleanupFuncp = नया AstCFunc{fl, "trace_cleanup", m_शीर्ष_Scopep};
        cleanupFuncp->argTypes("void* voidSelf, " + v3Global.opt.traceClassBase()
                               + "* /*unused*/");
        cleanupFuncp->isTrace(true);
        cleanupFuncp->slow(false);
        cleanupFuncp->isStatic(true);
        cleanupFuncp->isLoose(true);
        m_शीर्ष_Scopep->addBlocksp(cleanupFuncp);
        cleanupFuncp->addStmtsp(नया AstCStmt{fl, EmitCUtil::voidSelfAssign(m_शीर्ष_Modp)});
        cleanupFuncp->addStmtsp(नया AstCStmt{fl, EmitCUtil::symClassAssign()});

        // Register it
        {
            AstCStmt* स्थिर cstmtp = नया AstCStmt{fl};
            m_regFuncp->addStmtsp(cstmtp);
            cstmtp->add("tracep->addCleanupCb(");
            cstmtp->add(नया AstAddrOfCFunc{fl, cleanupFuncp});
            cstmtp->add(", vlSelf);");
        }

        // Clear global activity flag
        cleanupFuncp->addStmtsp(
            नया AstCStmt{m_शीर्ष_Scopep->fileline(), "vlSymsp->__Vm_activity = false;"s});

        // Clear fine grained activity flags
        क्रम (uint32_t i = 0; i < m_activity_संख्या; ++i) {
            Ast_गाँठ* स्थिर clrp = नया AstAssign{fl, selectActivity(fl, i, VAccess::WRITE),
                                                नया AstConst{fl, AstConst::BitFalse{}}};
            cleanupFuncp->addStmtsp(clrp);
        }
    }

    व्योम createTraceFunctions() {
        // Detect and remove duplicate values
        detectDuplicates();

        graphDtypePrune();
        splitTraceDeclFuncs();
        m_graph.removeRedundantEdgesMax(&V3GraphEdge::followAlwaysTrue);

        // Simplify & optimize the graph
        अगर (dumpGraphLevel() >= 6) m_graph.dumpDotFilePrefixed("trace_pre");
        graphSimplify(true);
        अगर (dumpGraphLevel() >= 6) m_graph.dumpDotFilePrefixed("trace_simplified");
        graphOptimize();
        अगर (dumpGraphLevel() >= 6) m_graph.dumpDotFilePrefixed("trace_optimized");

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
        m_regFuncp = नया AstCFunc{m_शीर्ष_Scopep->fileline(), "trace_register", m_शीर्ष_Scopep};
        m_regFuncp->argTypes(v3Global.opt.traceClassBase() + "* tracep");
        m_regFuncp->entryPoint(true);
        m_regFuncp->isTrace(true);
        m_regFuncp->slow(true);
        m_regFuncp->isStatic(false);
        m_regFuncp->isLoose(true);
        m_शीर्ष_Scopep->addBlocksp(m_regFuncp);

        // Create the const dump functions. Also allocates trace codes.
        createConstTraceFunctions(traces);

        // Create the full and incremental dump functions
        createNonConstTraceFunctions(traces, nNonConstCodes, m_parallelism);

        // Root-traced libraries alias wrapper IOs onto the existing top-module codes.
        अगर (!v3Global.opt.libCreate().empty()) {
            std::vector<AstTraceDecl*> मूल_Declps;
            std::vector<AstTraceDecl*> शीर्ष_Declps;
            TraceInitDeclCollector मूल_Collector{मूल_Declps};
            TraceInitDeclCollector शीर्ष_Collector{शीर्ष_Declps};
            क्रम (Ast_गाँठ* blockp = m_शीर्ष_Scopep->blocksp(); blockp; blockp = blockp->nextp()) {
                AstCFunc* स्थिर funcp = VN_CAST(blockp, CFunc);
                अगर (funcp && VString::startsWith(funcp->name(), "trace_init_leaf_root__")) {
                    मूल_Collector.collect(funcp);
                } अन्यथा अगर (funcp && VString::startsWith(funcp->name(), "trace_init_leaf_top__")) {
                    शीर्ष_Collector.collect(funcp);
                }
            }

            std::vector<bool> used(शीर्ष_Declps.आकार(), false);
            क्रम (AstTraceDecl* स्थिर मूल_Declp : मूल_Declps) {
                क्रम (माप_प्रकार i = 0; i < शीर्ष_Declps.आकार(); ++i) {
                    अगर (used[i] || !same_मूल_Init_Alias(मूल_Declp, शीर्ष_Declps[i])) जारी;
                    मूल_Declp->code(शीर्ष_Declps[i]->code());
                    used[i] = true;
                    अवरोध;
                }
            }
        }

        // Remove refs to traced values from TraceDecl nodes, these have now moved under
        // TraceInc
        क्रम (स्थिर स्वतः& i : traces) {
            Ast_गाँठ* स्थिर valuep = i.second->nodep()->valuep();
            VL_DO_DANGLING(valuep->unlinkFrBack()->delete_वृक्ष(), valuep);
        }

        // Create the trace cleanup function clearing the activity flags
        createCleanupFunction();
    }

    TraceCFuncVertex* getCFuncVertexp(AstCFunc* nodep) {
        V3GraphVertex* स्थिर vtxp = nodep->user1u().toGraphVertex();
        TraceCFuncVertex* vertexp = vtxp ? vtxp->cast<TraceCFuncVertex>() : nullptr;
        अगर (!vertexp) {
            vertexp = नया TraceCFuncVertex{&m_graph, nodep};
            nodep->user1p(vertexp);
        }
        वापस vertexp;
    }
    TraceActivityVertex* getActivityVertexp(Ast_गाँठ* nodep, bool slow) {
        V3GraphVertex* स्थिर vtxp = nodep->user3u().toGraphVertex();
        TraceActivityVertex* vertexp = vtxp ? vtxp->cast<TraceActivityVertex>() : nullptr;
        अगर (!vertexp) {
            vertexp = नया TraceActivityVertex{&m_graph, nodep, slow};
            nodep->user3p(vertexp);
        }
        vertexp->slow(slow);
        वापस vertexp;
    }

    // VISITORS
    व्योम visit(AstNetlist* nodep) override {
        // Add vertexes for all TraceDecl, and edges from VARs each trace looks at
        m_finding = false;
        iterateChildren(nodep);

        // Add vertexes for all CFUNCs, and edges to VARs the func sets
        m_finding = true;
        iterateChildren(nodep);

        // Create the trace functions and insert them into the tree
        createTraceFunctions();

        // Save number of trace codes used
        nodep->nTraceCodes(m_code);
    }
    व्योम visit(Ast_गाँठ_Module* nodep) override {
        अगर (nodep->is_शीर्ष()) m_शीर्ष_Modp = nodep;
        iterateChildren(nodep);
    }
    व्योम visit(AstVarScope* nodep) override {
        अगर (!m_finding) {
            अगर (स्थिर AstIface* स्थिर ifacep = nodep->varp()->sensIfacep()) {
                m_ifaceMemberVscps[{ifacep, nodep->varp()->name()}].धकेलो_back(nodep);
            }
        }
    }
    व्योम visit(Ast_Stmt_व्यंजक* nodep) override {
        अगर (!m_finding && !nodep->user2()) {
            अगर (AstCCall* स्थिर callp = VN_CAST(nodep->exprp(), CCall)) {
                UINFO(8, "   CCALL " << callp);
                // See if there are other calls in same statement list;
                // If so, all funcs might share the same activity code
                TraceActivityVertex* स्थिर activityVtxp
                    = getActivityVertexp(nodep, callp->funcp()->slow());
                क्रम (Ast_गाँठ* nextp = nodep; nextp; nextp = nextp->nextp()) {
                    अगर (Ast_Stmt_व्यंजक* स्थिर stmtp = VN_CAST(nextp, Stmt_व्यंजक)) {
                        अगर (AstCCall* स्थिर ccallp = VN_CAST(stmtp->exprp(), CCall)) {
                            stmtp->user2(true);  // Processed
                            UINFO(8, "     SubCCALL " << ccallp);
                            V3GraphVertex* स्थिर ccallFuncVtxp = getCFuncVertexp(ccallp->funcp());
                            activityVtxp->slow(ccallp->funcp()->slow());
                            नया V3GraphEdge{&m_graph, activityVtxp, ccallFuncVtxp, 1};
                        }
                    }
                }
            }
        }
        iterateChildren(nodep);
    }
    व्योम visit(AstCFunc* nodep) override {
        UINFO(8, "   CFUNC " << nodep);
        V3GraphVertex* स्थिर funcVtxp = getCFuncVertexp(nodep);
        अगर (!m_finding) {  // If public, we need a unique activity code to allow for sets
                           // directly in this func
            अगर (nodep->funcPublic() || nodep->dpiExportImpl() || nodep->entryPoint()
                || nodep->isCoroutine()) {
                // Cannot treat a coroutine as slow, it may be resumed later
                स्थिर bool slow = nodep->slow() && !nodep->isCoroutine();
                TraceActivityVertex* स्थिर activityVtxp = getActivityVertexp(nodep, slow);
                नया V3GraphEdge{&m_graph, activityVtxp, funcVtxp, 1};
            }
        }
        VL_RESTORER(m_cfuncp);
        m_cfuncp = nodep;
        iterateChildren(nodep);
    }
    व्योम visit(AstTraceDecl* nodep) override {
        UINFO(8, "   TRACE " << nodep);
        अगर (!m_finding && !nodep->inDtypeFunc()) {
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
            अगर (m_cfuncp && VString::startsWith(m_cfuncp->name(), "trace_init_leaf_root__")) {
                वापस;
            }
            V3GraphVertex* स्थिर vertexp = नया TraceTraceVertex{&m_graph, nodep};
            nodep->user1p(vertexp);

            UASSERT_OBJ(m_cfuncp, nodep, "Trace not under func");
            m_cfuncp->user2(true);
            VL_RESTORER(m_tracep);
            m_tracep = nodep;
            iterateChildren(nodep);
        }
    }
    व्योम visit(AstVarRef* nodep) override {
        UASSERT_OBJ(nodep->varScopep(), nodep, "No var scope?");
        AstVarScope* स्थिर varscopep = nodep->varScopep();
        V3GraphVertex* varVtxp = varscopep->user1u().toGraphVertex();
        अगर (m_tracep) {
            UASSERT_OBJ(nodep->access().isReadOnly(), nodep, "Lvalue in trace?  Should be const.");
            अगर (!varVtxp) {
                varVtxp = नया TraceVarVertex{&m_graph, nodep->varScopep()};
                nodep->varScopep()->user1p(varVtxp);
            }
            V3GraphVertex* स्थिर traceVtxp = m_tracep->user1u().toGraphVertex();
            नया V3GraphEdge{&m_graph, varVtxp, traceVtxp, 1};
            अगर (nodep->varp()->isPrimaryInish()  // Always need to trace primary inputs
                || nodep->varp()->isSigPublic()) {  // Or ones user can change
                नया V3GraphEdge{&m_graph, m_alwaysVtxp, traceVtxp, 1};
            }
            अगर (m_tracep->dtypeCallp()) varscopep->user2p(m_tracep);
        } अन्यथा अगर (m_cfuncp && m_finding && nodep->access().isWriteOrRW()) {
            V3GraphVertex* स्थिर funcVtxp = getCFuncVertexp(m_cfuncp);
            अगर (varVtxp) {  // else we're not tracing this signal
                नया V3GraphEdge{&m_graph, funcVtxp, varVtxp, 1};
                AstTraceDecl* स्थिर declp = VN_AS(varscopep->user2p(), TraceDecl);
                अगर (declp) {
                    V3GraphVertex* स्थिर cFuncVtxp = getCFuncVertexp(m_cfuncp);
                    क्रम (स्थिर V3GraphEdge& edge : cFuncVtxp->inEdges()) {
                        V3GraphVertex* स्थिर activityp = edge.fromp();
                        अगर (!declp->user3p()) {
                            declp->user3p(activityp);
                        } अन्यथा अगर (declp->user3u().toGraphVertex() != activityp) {
                            declp->user2(true);
                        }
                    }
                }
            }
        }
    }
    व्योम visit(AstMemberSel* nodep) override {
        अगर (m_cfuncp && m_finding && nodep->access().isWriteOrRW()) {
            AstIfaceRefDType* स्थिर dtypep
                = VN_CAST(nodep->fromp()->dtypep()->skipRefp(), IfaceRefDType);
            अगर (dtypep && dtypep->isVirtual()) {
                स्थिर स्वतः it = m_ifaceMemberVscps.find({dtypep->ifacep(), nodep->varp()->name()});
                अगर (it != m_ifaceMemberVscps.end()) {
                    V3GraphVertex* स्थिर funcVtxp = getCFuncVertexp(m_cfuncp);
                    क्रम (AstVarScope* स्थिर vscp : it->second) {
                        V3GraphVertex* varVtxp = vscp->user1u().toGraphVertex();
                        अगर (!varVtxp) {
                            varVtxp = नया TraceVarVertex{&m_graph, vscp};
                            vscp->user1p(varVtxp);
                        }
                        नया V3GraphEdge{&m_graph, funcVtxp, varVtxp, 1};
                    }
                }
            }
        }
        iterateChildren(nodep);
    }
    //--------------------
    व्योम visit(Ast_गाँठ* nodep) override { iterateChildren(nodep); }

खुला:
    // CONSTRUCTORS
    explicit TraceVisitor(AstNetlist* nodep)
        : m_alwaysVtxp{नया TraceActivityVertex{&m_graph, TraceActivityVertex::ACTIVITY_ALWAYS}} {
        nodep->user2_Clear_वृक्ष();  // TraceDecl multiple activities flag
        nodep->user3_Clear_वृक्ष();  // TraceDecl TraceActivityVertex (assumes we start at nullptr)
        iterate(nodep);
        nodep->foreach([](AstTraceDecl* स्थिर declp) {
            अगर (declp->inDtypeFunc()) {
                declp->valuep()->unlinkFrBack()->delete_वृक्ष();
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

व्योम V3Trace::traceAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { TraceVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_वृक्ष("trace", 0, dump_वृक्ष_Either_Level() >= 3);
}
