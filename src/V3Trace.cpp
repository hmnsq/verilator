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

#अन्तर्भाव "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#अन्तर्भाव "V3Trace.h"

#अन्तर्भाव "V3Ast.h"
#अन्तर्भाव "V3DupFinder.h"
#अन्तर्भाव "V3EmitCBase.h"
#अन्तर्भाव "V3Graph.h"
#अन्तर्भाव "V3Stats.h"
#अन्तर्भाव "V3UniqueNames.h"

#अन्तर्भाव <limits>
#अन्तर्भाव <map>
#अन्तर्भाव <set>
#अन्तर्भाव <unordered_map>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Graph vertexes

वर्ग TraceActivityVertex final : सार्वजनिक V3GraphVertex {
    VL_RTTI_IMPL(TraceActivityVertex, V3GraphVertex)
    Ast_गाँठो* अचर m_insertp;
    int32_t m_activityCode;
    तार्किक m_slow;  // If always slow, we can use the same code
सार्वजनिक:
    गणना { ACTIVITY_NEVER = ((1UL << 31) - 1) };
    गणना { ACTIVITY_ALWAYS = ((1UL << 31) - 2) };
    गणना { ACTIVITY_SLOW = 0 };
    TraceActivityVertex(V3Graph* graphp, Ast_गाँठो* nodep, तार्किक slow)
        : V3GraphVertex{graphp}
        , m_insertp{nodep} {
        m_activityCode = 0;
        m_slow = slow;
    }
    TraceActivityVertex(V3Graph* graphp, int32_t code)
        : V3GraphVertex{graphp}
        , m_insertp{nullptr} {
        m_activityCode = code;
        m_slow = असत्य;
    }
    ~TraceActivityVertex() override = स्वभाव;
    // ACCESSORS
    Ast_गाँठो* insertp() अचर {
        UASSERT(m_insertp, "Null insertp; probably called on a special always/slow");
        फर्काऊ m_insertp;
    }
    string name() अचर override {
        यदि (activityAlways()) {
            फर्काऊ "*ALWAYS*";
        } अन्यथा {
            फर्काऊ std::string{slow() ? "*SLOW* " : ""} + insertp()->name();
        }
    }
    string dotColor() अचर override { फर्काऊ slow() ? "yellowGreen" : "green"; }
    int32_t activityCode() अचर { फर्काऊ m_activityCode; }
    तार्किक activityAlways() अचर { फर्काऊ activityCode() == ACTIVITY_ALWAYS; }
    तार्किक activitySlow() अचर { फर्काऊ activityCode() == ACTIVITY_SLOW; }
    शून्य activityCode(int32_t code) { m_activityCode = code; }
    तार्किक slow() अचर { फर्काऊ m_slow; }
    शून्य slow(तार्किक flag) {
        यदि (!flag) m_slow = असत्य;
    }
};

वर्ग TraceCFuncVertex final : सार्वजनिक V3GraphVertex {
    VL_RTTI_IMPL(TraceCFuncVertex, V3GraphVertex)
    AstCFunc* अचर m_nodep;

सार्वजनिक:
    TraceCFuncVertex(V3Graph* graphp, AstCFunc* nodep)
        : V3GraphVertex{graphp}
        , m_nodep{nodep} {}
    ~TraceCFuncVertex() override = स्वभाव;
    // ACCESSORS
    AstCFunc* nodep() अचर { फर्काऊ m_nodep; }
    string name() अचर override { फर्काऊ nodep()->name(); }
    string dotColor() अचर override { फर्काऊ "yellow"; }
    File_पङ्क्ति* fileline() अचर override { फर्काऊ nodep()->fileline(); }
};

वर्ग TraceTraceVertex final : सार्वजनिक V3GraphVertex {
    VL_RTTI_IMPL(TraceTraceVertex, V3GraphVertex)
    AstTraceDecl* अचर m_nodep;  // TRACEINC this represents
    // nullptr, or other vertex with the real code() that duplicates this one
    TraceTraceVertex* m_duplicatep = nullptr;
    // When aliasing to a dtype parent, offset of the member within the dtype
    uint32_t m_dtypeAliasOffset = 0;

सार्वजनिक:
    TraceTraceVertex(V3Graph* graphp, AstTraceDecl* nodep)
        : V3GraphVertex{graphp}
        , m_nodep{nodep} {}
    ~TraceTraceVertex() override = स्वभाव;
    // ACCESSORS
    AstTraceDecl* nodep() अचर { फर्काऊ m_nodep; }
    string name() अचर override { फर्काऊ nodep()->name(); }
    string dotColor() अचर override { फर्काऊ "red"; }
    File_पङ्क्ति* fileline() अचर override { फर्काऊ nodep()->fileline(); }
    TraceTraceVertex* duplicatep() अचर { फर्काऊ m_duplicatep; }
    शून्य duplicatep(TraceTraceVertex* dupp) {
        UASSERT_OBJ(!duplicatep(), nodep(), "Assigning duplicatep() to already duplicated node");
        m_duplicatep = dupp;
    }
    शून्य redirectDuplicatep(TraceTraceVertex* dupp) { m_duplicatep = dupp; }
    uint32_t dtypeAliasOffset() अचर { फर्काऊ m_dtypeAliasOffset; }
    शून्य dtypeAliasOffset(uint32_t offset) { m_dtypeAliasOffset = offset; }
};

वर्ग TraceVarVertex final : सार्वजनिक V3GraphVertex {
    VL_RTTI_IMPL(TraceVarVertex, V3GraphVertex)
    AstVarScope* अचर m_nodep;

सार्वजनिक:
    TraceVarVertex(V3Graph* graphp, AstVarScope* nodep)
        : V3GraphVertex{graphp}
        , m_nodep{nodep} {}
    ~TraceVarVertex() override = स्वभाव;
    // ACCESSORS
    AstVarScope* nodep() अचर { फर्काऊ m_nodep; }
    string name() अचर override { फर्काऊ nodep()->name(); }
    string dotColor() अचर override { फर्काऊ "skyblue"; }
    File_पङ्क्ति* fileline() अचर override { फर्काऊ nodep()->fileline(); }
};

//######################################################################
// Trace state, as a visitor of each AstNode

वर्ग TraceVisitor final : सार्वजनिक VNVisitor {
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
    अचर VNUser1InUse m_inuser1;
    अचर VNUser2InUse m_inuser2;
    अचर VNUser3InUse m_inuser3;
    // VNUser4InUse     In V3Hasher via V3DupFinder

    // STATE
    Ast_गाँठो_Module* m_शीर्ष_Modp = nullptr;  // Module to add variables to
    AstScope* अचर m_शीर्ष_Scopep = v3Global.rootp()->शीर्ष_Scopep()->scopep();  // The top AstScope
    AstCFunc* m_cfuncp = nullptr;  // C function adding to graph
    AstCFunc* m_regFuncp = nullptr;  // Trace registration function
    AstCFunc* m_actAllFuncp = nullptr;  // Set all activity function
    AstTraceDecl* m_tracep = nullptr;  // Trace function adding to graph
    AstVarScope* m_activityVscp = nullptr;  // Activity variable
    uint32_t m_activity_सङ्ख्या = 0;  // Count of fields in activity variable
    uint32_t m_code = 0;  // Trace ident code# being assigned
    V3Graph m_graph;  // Var/CFunc tracking
    TraceActivityVertex* अचर m_alwaysVtxp;  // "Always trace" vertex
    तार्किक m_finding = असत्य;  // Pass one of algorithm?
    संरचना DtypeFuncs final {
    सार्वजनिक:
        AstCFunc* fullFuncp = nullptr;
        AstCFunc* chgFuncp = nullptr;
    };
    std::unordered_map<अचर Ast_गाँठो_DType*, DtypeFuncs>
        m_dtypeNonConstFuncs;  // Full / Chg funcs per type
    std::unordered_map<अचर Ast_गाँठो_DType*, AstCFunc*> m_dtypeConstFuncs;  // Const func per type
    V3UniqueNames m_dtypeNames{""};  // Unique type func names

    // Trace parallelism. Only VCD tracing can be parallelized at this time.
    अचर uint32_t m_parallelism
        = v3Global.opt.useTraceParallel() ? static_cast<uint32_t>(v3Global.opt.threads()) : 1;

    VDouble0 m_statSetters;  // Statistic tracking
    VDouble0 m_statSettersSlow;  // Statistic tracking
    VDouble0 m_statUniqSigs;  // Statistic tracking

    // All activity numbers applying to a given trace
    using ActCodeSet = std::set<uint32_t>;
    // For activity set, what traces apply
    using TraceVec = std::multimap<ActCodeSet, TraceTraceVertex*>;
    // Candidate interface-member VarScopes keyed by (interface type, member name)
    std::map<std::pair<अचर AstIface*, std::string>, std::vector<AstVarScope*>>
        m_ifaceMemberVscps;

    वर्ग TraceInitDeclCollector final : सार्वजनिक VNVisitor {
        std::vector<AstTraceDecl*>& m_declps;  // Output: trace declarations found so far
        std::set<अचर AstCFunc*> m_seenFuncps;

        शून्य visit(AstTraceDecl* nodep) override { m_declps.थप_back(nodep); }
        शून्य visit(AstCCall* nodep) override {
            यदि (AstCFunc* अचर funcp = nodep->funcp()) collect(funcp);
        }
        शून्य visit(Ast_गाँठो* nodep) override { iterateChildren(nodep); }

    सार्वजनिक:
        explicit TraceInitDeclCollector(std::vector<AstTraceDecl*>& declps)
            : m_declps{declps} {}
        शून्य collect(AstCFunc* funcp) {
            यदि (funcp && m_seenFuncps.घुसाउ(funcp).second) iterate(funcp);
        }
    };

    // METHODS

    स्थिर तार्किक same_जरा_Init_Alias(अचर AstTraceDecl* जरा_Declp, अचर AstTraceDecl* शीर्ष_Declp) {
        अचर VNumRange& जरा_Bit_Range = जरा_Declp->bitRange();
        अचर VNumRange& शीर्ष_Bit_Range = शीर्ष_Declp->bitRange();
        फर्काऊ जरा_Declp->showname() == शीर्ष_Declp->showname()
               && जरा_Declp->declDirection() == शीर्ष_Declp->declDirection()
               && जरा_Declp->widthMin() == शीर्ष_Declp->widthMin()
               && जरा_Bit_Range.ranged() == शीर्ष_Bit_Range.ranged()
               && (!जरा_Bit_Range.ranged()
                   || (जरा_Bit_Range.बायाँ() == शीर्ष_Bit_Range.बायाँ()
                       && जरा_Bit_Range.दायाँ() == शीर्ष_Bit_Range.दायाँ()));
    }

    शून्य detectDuplicates() {
        UINFO(9, "Finding duplicates");
        // Note uses user4
        V3DupFinder dupFinder;  // Duplicate code detection

        // Compute member offsets within dtype instances incrementally.
        // For each dtype member TraceDecl, stores its code offset within the
        // dtype parent's code range.
        std::unordered_map<अचर AstTraceDecl*, uint32_t> memberOffsets;
        std::unordered_map<अचर AstTraceDecl*, uint32_t> runningOffset;

        // For fixup pass: track dups redirected to dtype parents, so we can
        // fix them if the parent's user2 is later set during the loop.
        // Maps dup vertex -> original canonical member vertex.
        std::vector<std::pair<TraceTraceVertex*, TraceTraceVertex*>> dtypeParentRedirects;

        // Hash all of the traced values and find if there are any duplicates
        लागि (V3GraphVertex& vtx : m_graph.vertices()) {
            यदि (TraceTraceVertex* अचर vvertexp = vtx.cast<TraceTraceVertex>()) {
                AstTraceDecl* अचर nodep = vvertexp->nodep();
                यदि (nodep->dtypeCallp()) जारी;

                यदि (nodep->dtypeDeclp()) {
                    uint32_t& offset = runningOffset[nodep->dtypeDeclp()];
                    memberOffsets[nodep] = offset;
                    offset += nodep->codeInc();
                }

                UASSERT_OBJ(!vvertexp->duplicatep(), nodep, "Should not be a duplicate");
                अचर स्वतः dupit = dupFinder.findDuplicate(nodep);
                यदि (dupit == dupFinder.end()) {
                    dupFinder.घुसाउ(nodep);
                } अन्यथा {
                    AstTraceDecl* अचर dupDeclp = VN_AS(dupit->second, TraceDecl);
                    UASSERT_OBJ(dupDeclp, nodep, "Trace duplicate of wrong type");
                    TraceTraceVertex* अचर dupvertexp
                        = dupDeclp->user1u().toGraphVertex()->cast<TraceTraceVertex>();
                    UINFO(8, "  Orig " << dupDeclp << endl);
                    UINFO(8, "   dup " << nodep << endl);

                    यदि (dupDeclp->dtypeDeclp() && !dupDeclp->dtypeDeclp()->user2()) {
                        AstTraceDecl* अचर dtypeParentp = dupDeclp->dtypeDeclp();
                        TraceTraceVertex* अचर dtypeVtxp
                            = dtypeParentp->user1u().toGraphVertex()->cast<TraceTraceVertex>();
                        अचर स्वतः it2 = memberOffsets.find(dupDeclp);
                        UASSERT_OBJ(it2 != memberOffsets.end(), dupDeclp,
                                    "Member offset not precomputed");
                        vvertexp->duplicatep(dtypeVtxp);
                        vvertexp->dtypeAliasOffset(it2->second);
                        dtypeParentRedirects.emplace_back(vvertexp, dupvertexp);
                    } अन्यथा {
                        vvertexp->duplicatep(dupvertexp);
                    }
                    यदि (nodep->dtypeDeclp()) nodep->dtypeDeclp()->user2(सत्य);
                }
            }
        }

        लागि (अचर स्वतः& pair : dtypeParentRedirects) {
            TraceTraceVertex* अचर dupVtxp = pair.first;
            TraceTraceVertex* अचर memberVtxp = pair.second;
            अचर AstTraceDecl* अचर parentDeclp = dupVtxp->duplicatep()->nodep();
            यदि (parentDeclp->user2()) {
                dupVtxp->redirectDuplicatep(memberVtxp);
                dupVtxp->dtypeAliasOffset(0);
            }
        }

        यदि (dumpLevel() || debug() >= 9)
            dupFinder.dumpFile(v3Global.debugFilename("trace") + ".hash", असत्य);
    }

    शून्य graphDtypePrune() {
        लागि (V3GraphVertex* अचर vtxp : m_graph.vertices().unlinkable()) {
            यदि (TraceTraceVertex* अचर vvertexp = vtxp->cast<TraceTraceVertex>()) {
                AstTraceDecl* अचर declp = vvertexp->nodep();
                // This skips the dtype sub-func optimization if a var is affected by multiple
                // activities.  We really only need to do this for _chg funcs (and not decls,
                // _const and _full) but it's simpiler to do it all one way or the other.
                यदि (declp) {
                    यदि (declp->user2() || (declp->dtypeDeclp() && !declp->dtypeDeclp()->user2())) {
                        AstCCall* अचर callp = declp->dtypeCallp();
                        यदि (callp) {
                            Ast_गाँठो* stmtexprp = callp->backp();
                            VL_DO_DANGLING(थप_Deletep(stmtexprp->unlinkFrBack()), stmtexprp);
                        } अन्यथा {
                            तार्किक emptyScope;
                            गर {
                                emptyScope = असत्य;
                                Ast_गाँठो* अचर declBackp = declp->backp();
                                Ast_गाँठो* अचर declNextp = declp->nextp();
                                यदि (VN_IS(declBackp, Trace_थप_Prefix)
                                    && VN_IS(declNextp, Trace_झिक_Prefix)) {
                                    VL_DO_DANGLING(थप_Deletep(declBackp->unlinkFrBack()),
                                                   declBackp);
                                    VL_DO_DANGLING(थप_Deletep(declNextp->unlinkFrBack()),
                                                   declNextp);
                                    emptyScope = सत्य;
                                }
                            } जबसम्म (emptyScope);
                        }
                        // Can't purge until we finish this pass
                        थप_Deletep(declp->unlinkFrBack());  // declp used below
                        vvertexp->rerouteEdges(&m_graph);
                        vvertexp->unlinkDelete(&m_graph);
                    }
                    declp->dtypeDeclp(nullptr);
                }
            }
        }
    }

    शून्य splitTraceDeclFuncs() {
        अचर पूर्णाङ्क splitLimit = v3Global.opt.outputSplitCTrace() ? v3Global.opt.outputSplitCTrace()
                                                                : std::numeric_limits<पूर्णाङ्क>::max();
        यदि (splitLimit == std::numeric_limits<पूर्णाङ्क>::max()) फर्काऊ;

        लागि (Ast_गाँठो* nodep = m_शीर्ष_Scopep->blocksp(); nodep; nodep = nodep->nextp()) {
            AstCFunc* अचर funcp = VN_CAST(nodep, CFunc);
            यदि (!funcp || !funcp->user2()) जारी;

            अचर string baseName = funcp->name();
            uint32_t sub_अङ्क = 1;
            पूर्णाङ्क curSize = 0;
            AstCFunc* curFuncp = funcp;
            std::vector<Ast_गाँठो_Stmt*> callStmts;

            स्वतः startNewSubFunc = [&]() {
                File_पङ्क्ति* अचर flp = funcp->fileline();
                अचर string newName = baseName + "_" + cvtToStr(sub_अङ्क++);
                AstCFunc* अचर newFuncp = नूतन AstCFunc{flp, newName, m_शीर्ष_Scopep};
                newFuncp->argTypes(v3Global.opt.traceClassBase() + "* tracep");
                newFuncp->isTrace(सत्य);
                newFuncp->isStatic(असत्य);
                newFuncp->isLoose(सत्य);
                newFuncp->slow(सत्य);
                newFuncp->dontCombine(सत्य);
                m_शीर्ष_Scopep->addBlocksp(newFuncp);
                newFuncp->addStmtsp(नूतन AstCStmt{flp, "const int c = vlSymsp->__Vm_baseCode;"});
                AstCCall* अचर callp = नूतन AstCCall{flp, newFuncp};
                callp->dtypeSetVoid();
                callp->argTypes("tracep");
                callStmts.थप_back(callp->makeStmt());
                curFuncp = newFuncp;
                curSize = 0;
            };

            लागि (Ast_गाँठो *stmtp = funcp->stmtsp(), *nextp; stmtp; stmtp = nextp) {
                nextp = stmtp->nextp();
                curSize += stmtp->गाँठो_गन्ती();

                यदि (curFuncp != funcp) {
                    stmtp->unlinkFrBack();
                    curFuncp->addStmtsp(stmtp);
                }

                यदि (curSize > splitLimit && nextp) startNewSubFunc();
            }

            लागि (Ast_गाँठो_Stmt* अचर callStmtp : callStmts) { funcp->addStmtsp(callStmtp); }
        }
    }

    शून्य graphSimplify(तार्किक initial) {
        यदि (initial) {
            // Remove all variable nodes
            लागि (V3GraphVertex* अचर vtxp : m_graph.vertices().unlinkable()) {
                यदि (TraceVarVertex* अचर vvertexp = vtxp->cast<TraceVarVertex>()) {
                    vvertexp->rerouteEdges(&m_graph);
                    vvertexp->unlinkDelete(&m_graph);
                }
            }
            // Remove multiple variables connecting funcs to traces
            // We do this twice, as then we have fewer edges to multiply out in the below
            // expansion.
            m_graph.removeRedundantEdgesMax(&V3GraphEdge::followAlwaysTrue);
            // Remove all Cfunc nodes
            लागि (V3GraphVertex* अचर vtxp : m_graph.vertices().unlinkable()) {
                यदि (TraceCFuncVertex* अचर vvertexp = vtxp->cast<TraceCFuncVertex>()) {
                    vvertexp->rerouteEdges(&m_graph);
                    vvertexp->unlinkDelete(&m_graph);
                }
            }
        }

        // Remove multiple variables connecting funcs to traces
        m_graph.removeRedundantEdgesMax(&V3GraphEdge::followAlwaysTrue);

        // If there are any edges from a always, keep only the always
        लागि (V3GraphVertex& vtx : m_graph.vertices()) {
            यदि (TraceTraceVertex* अचर vvertexp = vtx.cast<TraceTraceVertex>()) {
                // Search for the incoming always edge
                अचर V3GraphEdge* alwaysEdgep = nullptr;
                लागि (अचर V3GraphEdge& edge : vvertexp->inEdges()) {
                    अचर TraceActivityVertex* अचर actVtxp
                        = edge.fromp()->as<अचर TraceActivityVertex>();
                    यदि (actVtxp->activityAlways()) {
                        alwaysEdgep = &edge;
                        रोक;
                    }
                }
                // If always edge exists, remove all other edges
                यदि (alwaysEdgep) {
                    लागि (V3GraphEdge* अचर edgep : vvertexp->inEdges().unlinkable()) {
                        यदि (edgep != alwaysEdgep) VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
                    }
                }
            }
        }

        // Activity points with no outputs can be removed
        लागि (V3GraphVertex* अचर vtxp : m_graph.vertices().unlinkable()) {
            यदि (TraceActivityVertex* अचर aVtxp = vtxp->cast<TraceActivityVertex>()) {
                // Leave in the always vertex for later use.
                यदि (aVtxp != m_alwaysVtxp && aVtxp->outEmpty()) {
                    VL_DO_DANGLING(aVtxp->unlinkDelete(&m_graph), aVtxp);
                }
            }
        }
    }

    uint32_t assignactivityNumbers() {
        uint32_t activity_सङ्ख्या = 1;  // Note 0 indicates "slow" only
        लागि (V3GraphVertex& vtx : m_graph.vertices()) {
            यदि (TraceActivityVertex* अचर vvertexp = vtx.cast<TraceActivityVertex>()) {
                यदि (vvertexp != m_alwaysVtxp) {
                    यदि (vvertexp->slow()) {
                        vvertexp->activityCode(TraceActivityVertex::ACTIVITY_SLOW);
                    } अन्यथा {
                        vvertexp->activityCode(activity_सङ्ख्या++);
                    }
                }
            }
        }
        फर्काऊ activity_सङ्ख्या;
    }

    शून्य sortTraces(TraceVec& traces, uint32_t& nNonConstCodes) {
        // Populate sort structure
        traces.clear();
        nNonConstCodes = 0;
        लागि (V3GraphVertex& vtx : m_graph.vertices()) {
            यदि (TraceTraceVertex* अचर vtxp = vtx.cast<TraceTraceVertex>()) {
                ActCodeSet actSet;
                UINFO(9, "  Add to sort: " << vtxp);
                UINFOTREE(9, vtxp->nodep(), "", "trnode");
                लागि (अचर V3GraphEdge& edge : vtxp->inEdges()) {
                    अचर TraceActivityVertex* अचर cfvertexp
                        = edge.fromp()->cast<अचर TraceActivityVertex>();
                    UASSERT_OBJ(cfvertexp, vtxp->nodep(),
                                "Should have been function pointing to this trace");
                    UINFO(9, "   Activity: " << cfvertexp);
                    यदि (cfvertexp->activityAlways()) {
                        // If code 0, we always trace; ignore other codes
                        actSet.घुसाउ(TraceActivityVertex::ACTIVITY_ALWAYS);
                    } अन्यथा {
                        actSet.घुसाउ(cfvertexp->activityCode());
                    }
                }
                UASSERT_OBJ(actSet.गन्ती(TraceActivityVertex::ACTIVITY_ALWAYS) == 0
                                || actSet.size() == 1,
                            vtxp->nodep(), "Always active trace has further triggers");
                // Count nodes
                यदि (!vtxp->duplicatep() && !actSet.empty())
                    nNonConstCodes += vtxp->nodep()->codeInc();
                यदि (actSet.empty()) {
                    // If a trace doesn't have activity, it's constant, and we
                    // don't need to track changes on it.
                    actSet.घुसाउ(TraceActivityVertex::ACTIVITY_NEVER);
                } अन्यथा यदि (actSet.गन्ती(TraceActivityVertex::ACTIVITY_SLOW) && actSet.size() > 1) {
                    // If a trace depends on the slow flag as well as other
                    // flags, remove the dependency on the slow flag. We will
                    // make slow routines set all activity flags.
                    actSet.erase(TraceActivityVertex::ACTIVITY_SLOW);
                }
                traces.emplace(actSet, vtxp);
            }
        }
    }

    शून्य graphOptimize() {
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
        अचर स्वतः end = traces.end();
        जबसम्म (it != end) {
            स्वतः head = it;
            // Approximate the complexity of the value change check
            uint32_t complexity = 0;
            अचर ActCodeSet& actSet = it->first;
            लागि (; it != end && it->first == actSet; ++it) {
                यदि (!it->second->duplicatep()) {
                    uint32_t cost = 0;
                    अचर AstTraceDecl* अचर declp = it->second->nodep();
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
            यदि (actSet.गन्ती(TraceActivityVertex::ACTIVITY_ALWAYS)) जारी;
            यदि (actSet.गन्ती(TraceActivityVertex::ACTIVITY_NEVER)) जारी;
            यदि (actSet.गन्ती(TraceActivityVertex::ACTIVITY_SLOW)) जारी;
            // If the value comparisons are cheaper to perform than checking the
            // activity flags make the signals always traced. Note this cost
            // equation is heuristic.
            यदि (complexity <= actSet.size() * 2) {
                लागि (; head != it; ++head) {
                    नूतन V3GraphEdge{&m_graph, m_alwaysVtxp, head->second, 1};
                }
            }
        }

        graphSimplify(असत्य);
    }

    Ast_गाँठो_अभिव्यक्ति* selectActivity(File_पङ्क्ति* flp, uint32_t acode, अचर VAccess& access) {
        फर्काऊ नूतन AstArraySel(flp, नूतन AstVarRef{flp, m_activityVscp, access}, acode);
    }

    Ast_गाँठो* newActivitySetter(Ast_गाँठो* insertp, uint32_t code) {
        ++m_statSetters;
        File_पङ्क्ति* अचर fl = insertp->fileline();
        AstAssign* अचर setterp = नूतन AstAssign{fl, selectActivity(fl, code, VAccess::WRITE),
                                                 नूतन AstConst{fl, AstConst::BitTrue{}}};
        फर्काऊ setterp;
    }

    Ast_गाँठो* newActivityAll(Ast_गाँठो* insertp) {
        ++m_statSettersSlow;
        यदि (!m_actAllFuncp) {
            File_पङ्क्ति* अचर flp = m_शीर्ष_Scopep->fileline();
            AstCFunc* अचर funcp = नूतन AstCFunc{flp, "__VtraceActivitySetAll", m_शीर्ष_Scopep};
            funcp->slow(सत्य);
            funcp->isStatic(असत्य);
            funcp->isLoose(सत्य);
            m_शीर्ष_Scopep->addBlocksp(funcp);
            लागि (uint32_t code = 0; code < m_activity_सङ्ख्या; ++code) {
                Ast_गाँठो* अचर setterp = newActivitySetter(insertp, code);
                funcp->addStmtsp(setterp);
            }
            m_actAllFuncp = funcp;
        }
        AstCCall* अचर callp = नूतन AstCCall{insertp->fileline(), m_actAllFuncp};
        callp->dtypeSetVoid();
        फर्काऊ callp->makeStmt();
    }

    शून्य createActivityFlags() {
        // Assign final activity numbers
        m_activity_सङ्ख्या = assignactivityNumbers();

        // Create an array of bytes, not a bit vector, as they can be set
        // atomically by mtasks, and are cheaper to set (no need for
        // read-modify-write on the C type), and the speed of the tracing code
        // is the same on largish designs.
        File_पङ्क्ति* अचर flp = m_शीर्ष_Scopep->fileline();
        Ast_गाँठो_DType* अचर newScalarDtp = नूतन AstBasicDType{flp, VFlagBitPacked{}, 1};
        v3Global.rootp()->typeTablep()->addTypesp(newScalarDtp);
        AstRange* अचर newArange
            = नूतन AstRange{flp, VNumRange{static_cast<पूर्णाङ्क>(m_activity_सङ्ख्या) - 1, 0}};
        Ast_गाँठो_DType* अचर newArrDtp = नूतन AstUnpackArrayDType{flp, newScalarDtp, newArange};
        v3Global.rootp()->typeTablep()->addTypesp(newArrDtp);
        m_activityVscp = v3Global.rootp()->शीर्ष_Scopep()->createTemp("__VtraceActivity", newArrDtp);

        // Insert activity setters
        लागि (अचर V3GraphVertex& vtx : m_graph.vertices()) {
            यदि (अचर TraceActivityVertex* अचर vtxp = vtx.cast<अचर TraceActivityVertex>()) {
                Ast_गाँठो* setterp = nullptr;
                यदि (vtxp->activitySlow()) {
                    setterp = newActivityAll(vtxp->insertp());
                    // Just set all flags in slow code as it should be rare.
                    // This will be rolled up into a loop by V3Reloop.
                } अन्यथा यदि (!vtxp->activityAlways()) {
                    setterp = newActivitySetter(vtxp->insertp(), vtxp->activityCode());
                }
                यदि (setterp) {
                    Ast_गाँठो* अचर insertp = vtxp->insertp();
                    यदि (Ast_Stmt_अभिव्यक्ति* अचर stmtp = VN_CAST(insertp, Stmt_अभिव्यक्ति)) {
                        stmtp->addNextHere(setterp);
                    } अन्यथा यदि (AstCFunc* अचर funcp = VN_CAST(insertp, CFunc)) {
                        // If there are awaits, insert the setter after each await
                        यदि (funcp->isCoroutine() && funcp->stmtsp()) {
                            funcp->stmtsp()->foreachAndNext([setterp](AstCAwait* awaitp) {
                                awaitp->addNextHere(setterp->clone_रूख(असत्य));
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

    AstCFunc* newCFunc(VTraceType traceType, AstCFunc* शीर्ष_Funcp, uint32_t func_अङ्क,
                       uint32_t baseCode = 0, अचर AstTraceDecl* अचर declp = nullptr,
                       तार्किक declSub = असत्य) {
        // Create new function
        अचर तार्किक is_शीर्ष_Func = !declp && शीर्ष_Funcp == nullptr;
        std::string funcName;
        यदि (is_शीर्ष_Func || declp) {
            funcName = traceType.func_prefix();
        } अन्यथा {
            funcName = शीर्ष_Funcp->name();
            funcName += "_sub";
        }
        यदि (declp) {
            funcName += "_dtype";
            यदि (declSub) funcName += "_sub";
            funcName += "__";
            funcName = m_dtypeNames.get(funcName);
        } अन्यथा {
            funcName += "_";
            funcName += cvtToStr(func_अङ्क);
        }

        File_पङ्क्ति* अचर flp = m_शीर्ष_Scopep->fileline();
        AstCFunc* अचर funcp = नूतन AstCFunc{flp, funcName, m_शीर्ष_Scopep};
        funcp->isTrace(सत्य);
        funcp->dontCombine(सत्य);
        funcp->isLoose(सत्य);
        funcp->slow(traceType != VTraceType::CHANGE);
        funcp->isStatic(is_शीर्ष_Func);
        // Add it to top scope
        m_शीर्ष_Scopep->addBlocksp(funcp);
        अचर std::string bufArg
            = v3Global.opt.traceClassBase() + "::Buffer* bufp"
              + (declp ? (", uint32_t offset, const " + declp->dtypep()->cType("", सत्य, सत्य)
                          + " __VdtypeVar")
                       : "");
        यदि (is_शीर्ष_Func) {
            // Top functions
            funcp->argTypes("void* voidSelf, " + bufArg);
            funcp->addStmtsp(नूतन AstCStmt{flp, EmitCUtil::voidSelfAssign(m_शीर्ष_Modp)});
            funcp->addStmtsp(नूतन AstCStmt{flp, EmitCUtil::symClassAssign()});
            // Add global activity check to change dump functions
            यदि (traceType == VTraceType::CHANGE) {  //
                funcp->addStmtsp(
                    नूतन AstCStmt{flp, "if (VL_UNLIKELY(!vlSymsp->__Vm_activity)) return;"});
            }
            // Register function
            AstCStmt* अचर cstmtp = नूतन AstCStmt{flp};
            m_regFuncp->addStmtsp(cstmtp);
            यदि (traceType == VTraceType::CONSTANT) {
                cstmtp->add("tracep->addConstCb(");
                cstmtp->add(नूतन AstAddrOfCFunc{flp, funcp});
                cstmtp->add(", " + std::to_string(func_अङ्क) + ", vlSelf);");
            } अन्यथा यदि (traceType == VTraceType::FULL) {
                cstmtp->add("tracep->addFullCb(");
                cstmtp->add(नूतन AstAddrOfCFunc{flp, funcp});
                cstmtp->add(", " + std::to_string(func_अङ्क) + ", vlSelf);");
            } अन्यथा {
                cstmtp->add("tracep->addChgCb(");
                cstmtp->add(नूतन AstAddrOfCFunc{flp, funcp});
                cstmtp->add(", " + std::to_string(func_अङ्क) + ", vlSelf);");
            }
        } अन्यथा {
            // Sub functions
            funcp->argTypes(bufArg);
            // Setup base references. Note in rare occasions we can end up with an empty trace
            // sub function, hence the VL_ATTR_UNUSED attributes.
            यदि (traceType != VTraceType::CHANGE) {
                // Full dump sub function
                funcp->addStmtsp(नूतन AstCStmt{flp,  //
                                              string("uint32_t* const oldp VL_ATTR_UNUSED = "
                                                     "bufp->oldp(vlSymsp->__Vm_baseCode")
                                                  + (declp ? " + offset" : "") + ");\n"});
            } अन्यथा {
                // Change dump sub function
                funcp->addStmtsp(नूतन AstCStmt{flp,  //
                                              "uint32_t* const oldp VL_ATTR_UNUSED = "
                                              "bufp->oldp(vlSymsp->__Vm_baseCode + "
                                                  + (declp ? " offset" : cvtToStr(baseCode))
                                                  + ");\n"});
            }
            यदि (!declp) {
                // Add call to top function
                AstCCall* अचर callp = नूतन AstCCall{funcp->fileline(), funcp};
                callp->dtypeSetVoid();
                callp->argTypes("bufp");
                शीर्ष_Funcp->addStmtsp(callp->makeStmt());
            }
        }
        // Done
        UINFO(5, "  newCFunc " << funcp);
        फर्काऊ funcp;
    }

    AstCFunc* newCDtypeSubFunc(VTraceType traceType, अचर AstTraceDecl* अचर declp,
                               AstCFunc* parentp) {
        AstCFunc* अचर funcp = newCFunc(traceType, nullptr, 0, 0, declp, सत्य);
        File_पङ्क्ति* अचर flp = m_शीर्ष_Scopep->fileline();
        AstCCall* अचर callp = नूतन AstCCall{flp, funcp};
        callp->dtypeSetVoid();
        callp->argTypes("bufp, offset, __VdtypeVar");
        parentp->addStmtsp(callp->makeStmt());
        फर्काऊ funcp;
    }

    AstCFunc* createConstDtypeTraceFunctions(अचर AstTraceDecl* declp) {
        अचर Ast_गाँठो_DType* अचर dtypep = declp->dtypep()->skipRefp();
        स्वतः pair = m_dtypeConstFuncs.emplace(dtypep, nullptr);
        यदि (pair.second) {
            File_पङ्क्ति* अचर flp = declp->fileline();
            AstCFunc* अचर funcp = newCFunc(VTraceType::CONSTANT, nullptr, 0, 0, declp);

            तार्किक first = सत्य;
            लागि (Ast_गाँठो* callStmtp = declp->dtypeCallp()->funcp()->stmtsp(); callStmtp;
                 callStmtp = callStmtp->nextp()) {
                अचर AstCCall* अचर callp
                    = VN_CAST(VN_CAST(callStmtp, Stmt_अभिव्यक्ति)->exprp(), CCall);
                तार्किक onlyOne = first && !callStmtp->nextp();
                AstCFunc* अचर subFuncp
                    = onlyOne ? funcp : newCDtypeSubFunc(VTraceType::CONSTANT, declp, funcp);
                लागि (Ast_गाँठो* stmtp = callp->funcp()->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    यदि (AstTraceDecl* अचर fieldDeclp = VN_CAST(stmtp, TraceDecl)) {
                        AstTraceInc* अचर incp
                            = नूतन AstTraceInc{flp, fieldDeclp, VTraceType::CONSTANT};
                        subFuncp->addStmtsp(incp);
                    }
                }
                first = असत्य;
            }

            pair.first->second = funcp;
        }

        फर्काऊ pair.first->second;
    }

    शून्य createConstTraceFunctions(अचर TraceVec& traces) {
        अचर पूर्णाङ्क splitLimit = v3Global.opt.outputSplitCTrace() ? v3Global.opt.outputSplitCTrace()
                                                                : std::numeric_limits<पूर्णाङ्क>::max();

        AstCFunc* अचर शीर्ष_Funcp = newCFunc(VTraceType::CONSTANT, nullptr, 0);
        uint32_t sub_Func_अङ्क = 0;
        AstCFunc* subFuncp = nullptr;
        पूर्णाङ्क subStmts = 0;
        std::vector<अचर TraceTraceVertex*> duplicates;
        लागि (स्वतः it = traces.cbegin(); it != traces.end(); ++it) {
            अचर TraceTraceVertex* अचर vtxp = it->second;
            AstTraceDecl* अचर declp = vtxp->nodep();
            यदि (vtxp->duplicatep()) {
                duplicates.थप_back(vtxp);
                जारी;
            }

            // This is a canonical trace node. Assign trace code (signal number).
            UASSERT_OBJ(!declp->codeAssigned(), declp,
                        "Canonical node should not have code assigned yet");
            declp->code(m_code);
            अचर uint32_t codeInc = declp->codeInc();
            m_code += codeInc;
            ++m_statUniqSigs;

            // If this is a const signal, add the AstTraceInc
            अचर ActCodeSet& actSet = it->first;
            यदि (actSet.गन्ती(TraceActivityVertex::ACTIVITY_NEVER)) {
                // Crate new sub function if required
                यदि (!subFuncp || subStmts > splitLimit) {
                    subStmts = 0;
                    subFuncp = newCFunc(VTraceType::CONSTANT, शीर्ष_Funcp, sub_Func_अङ्क);
                    ++sub_Func_अङ्क;
                }
                File_पङ्क्ति* अचर flp = declp->fileline();
                यदि (declp->dtypeCallp()) {
                    AstCFunc* अचर funcp = createConstDtypeTraceFunctions(declp);
                    Ast_गाँठो_अभिव्यक्ति* argsp = nullptr;
                    argsp = Ast_गाँठो::addNext(argsp, declp->valuep()->clone_रूख(असत्य));
                    AstCCall* अचर callp = नूतन AstCCall{flp, funcp, argsp};
                    callp->dtypeSetVoid();
                    callp->argTypes(callp->argTypes() + "bufp, " + std::to_string(declp->code()));
                    subFuncp->addStmtsp(callp->makeStmt());

                    subStmts += 1;
                } अन्यथा {
                    AstTraceInc* अचर incp = नूतन AstTraceInc{flp, declp, VTraceType::CONSTANT};
                    subFuncp->addStmtsp(incp);
                    subStmts += incp->गाँठो_गन्ती();
                }
            }
        }

        // Assign codes to duplicate nodes from their canonicals
        लागि (अचर TraceTraceVertex* अचर vtxp : duplicates) {
            AstTraceDecl* अचर declp = vtxp->nodep();
            अचर TraceTraceVertex* अचर canonVtxp = vtxp->duplicatep();
            अचर AstTraceDecl* अचर canonDeclp = canonVtxp->nodep();
            UASSERT_OBJ(!canonVtxp->duplicatep(), canonDeclp, "Canonical node is a duplicate");
            UASSERT_OBJ(canonDeclp->codeAssigned(), canonDeclp,
                        "Canonical node should have code assigned already");
            declp->code(canonDeclp->code() + vtxp->dtypeAliasOffset());
        }
    }

    DtypeFuncs createNonConstDtypeTraceFunctions(अचर AstTraceDecl* declp) {
        Ast_गाँठो_DType* dtypep = declp->dtypep()->skipRefp();
        स्वतः pair = m_dtypeNonConstFuncs.emplace(dtypep, DtypeFuncs{});
        यदि (pair.second) {
            File_पङ्क्ति* अचर flp = declp->fileline();
            AstCFunc* अचर fullFuncp = newCFunc(VTraceType::FULL, nullptr, 0, 0, declp);
            AstCFunc* अचर chgFuncp = newCFunc(VTraceType::CHANGE, nullptr, 0, 0, declp);

            तार्किक first = सत्य;
            लागि (Ast_गाँठो* callStmtp = declp->dtypeCallp()->funcp()->stmtsp(); callStmtp;
                 callStmtp = callStmtp->nextp()) {
                अचर AstCCall* अचर callp
                    = VN_CAST(VN_CAST(callStmtp, Stmt_अभिव्यक्ति)->exprp(), CCall);
                तार्किक onlyOne = first && !callStmtp->nextp();
                AstCFunc* अचर fullSubFuncp
                    = onlyOne ? fullFuncp : newCDtypeSubFunc(VTraceType::FULL, declp, fullFuncp);
                AstCFunc* अचर chgSubFuncp
                    = onlyOne ? chgFuncp : newCDtypeSubFunc(VTraceType::CHANGE, declp, chgFuncp);
                लागि (Ast_गाँठो* stmtp = callp->funcp()->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    यदि (AstTraceDecl* अचर fieldDeclp = VN_CAST(stmtp, TraceDecl)) {
                        AstTraceInc* अचर incFullp
                            = नूतन AstTraceInc{flp, fieldDeclp, VTraceType::FULL};
                        fullSubFuncp->addStmtsp(incFullp);
                        AstTraceInc* अचर incChgp
                            = नूतन AstTraceInc{flp, fieldDeclp, VTraceType::CHANGE};
                        chgSubFuncp->addStmtsp(incChgp);
                    }
                }
                first = असत्य;
            }

            pair.first->second = {.fullFuncp = fullFuncp, .chgFuncp = chgFuncp};
        }

        फर्काऊ pair.first->second;
    }

    शून्य createNonConstTraceFunctions(अचर TraceVec& traces, uint32_t nAllCodes,
                                      uint32_t parallelism) {
        अचर पूर्णाङ्क splitLimit = v3Global.opt.outputSplitCTrace() ? v3Global.opt.outputSplitCTrace()
                                                                : std::numeric_limits<पूर्णाङ्क>::max();

        // pre-incremented, so starts at 0
        uint32_t शीर्ष_Func_अङ्क = std::numeric_limits<uint32_t>::max();
        TraceVec::const_iterator it = traces.begin();
        जबसम्म (it != traces.end()) {
            AstCFunc* शीर्ष_Ful_Funcp = nullptr;
            AstCFunc* शीर्ष_Chg_Funcp = nullptr;
            AstCFunc* subFulFuncp = nullptr;
            AstCFunc* subChgFuncp = nullptr;
            uint32_t sub_Func_अङ्क = 0;
            पूर्णाङ्क subStmts = 0;
            अचर uint32_t maxCodes = std::max((nAllCodes + parallelism - 1) / parallelism, 1U);
            uint32_t nCodes = 0;
            अचर ActCodeSet* prevActSet = nullptr;
            AstIf* ifp = nullptr;
            uint32_t baseCode = 0;
            लागि (; nCodes < maxCodes && it != traces.end(); ++it) {
                अचर ActCodeSet& actSet = it->first;
                // Traced value never changes, no need to add it
                यदि (actSet.गन्ती(TraceActivityVertex::ACTIVITY_NEVER)) जारी;

                अचर TraceTraceVertex* अचर vtxp = it->second;
                AstTraceDecl* अचर declp = vtxp->nodep();

                // This is a duplicate decl, no need to add it, but must set the
                // function index to the same as the canonical node.
                यदि (अचर TraceTraceVertex* अचर canonVtxp = vtxp->duplicatep()) {
                    declp->fidx(canonVtxp->nodep()->fidx());
                    जारी;
                }

                // Create top function if not yet created
                यदि (!शीर्ष_Ful_Funcp) {
                    ++शीर्ष_Func_अङ्क;
                    शीर्ष_Ful_Funcp = newCFunc(VTraceType::FULL, nullptr, शीर्ष_Func_अङ्क);
                    शीर्ष_Chg_Funcp = newCFunc(VTraceType::CHANGE, nullptr, शीर्ष_Func_अङ्क);
                }

                // Create new sub function if required
                यदि (!subFulFuncp || subStmts > splitLimit) {
                    baseCode = declp->code();
                    subStmts = 0;
                    subFulFuncp = newCFunc(VTraceType::FULL, शीर्ष_Ful_Funcp, sub_Func_अङ्क, baseCode);
                    subChgFuncp = newCFunc(VTraceType::CHANGE, शीर्ष_Chg_Funcp, sub_Func_अङ्क, baseCode);
                    ++sub_Func_अङ्क;
                    prevActSet = nullptr;
                    ifp = nullptr;
                }

                // If required, create the conditional node checking the activity flags
                यदि (!prevActSet || actSet != *prevActSet) {
                    File_पङ्क्ति* अचर flp = m_शीर्ष_Scopep->fileline();
                    अचर तार्किक always = actSet.गन्ती(TraceActivityVertex::ACTIVITY_ALWAYS) != 0;
                    Ast_गाँठो_अभिव्यक्ति* condp = nullptr;
                    यदि (always) {
                        condp = नूतन AstConst{flp, 1};  // Always true, will be folded later
                    } अन्यथा {
                        लागि (अचर uint32_t actCode : actSet) {
                            Ast_गाँठो_अभिव्यक्ति* अचर selp = selectActivity(flp, actCode, VAccess::READ);
                            condp = condp ? नूतन AstOr{flp, condp, selp} : selp;
                        }
                    }
                    ifp = नूतन AstIf{flp, condp};
                    यदि (!always) ifp->branchPred(VBranchPred::BP_UNLIKELY);
                    subChgFuncp->addStmtsp(ifp);
                    subStmts += ifp->गाँठो_गन्ती();
                    prevActSet = &actSet;
                }

                // Add TraceInc nodes
                File_पङ्क्ति* अचर flp = declp->fileline();
                यदि (declp->dtypeCallp()) {
                    DtypeFuncs funcs = createNonConstDtypeTraceFunctions(declp);
                    Ast_गाँठो_अभिव्यक्ति* argsp = nullptr;
                    argsp = Ast_गाँठो::addNext(argsp, declp->valuep()->clone_रूख(असत्य));
                    AstCCall* अचर callFullp = नूतन AstCCall{flp, funcs.fullFuncp, argsp};
                    callFullp->dtypeSetVoid();
                    callFullp->argTypes(callFullp->argTypes() + "bufp, "
                                        + std::to_string(declp->code()));
                    subFulFuncp->addStmtsp(callFullp->makeStmt());
                    argsp = nullptr;
                    argsp = Ast_गाँठो::addNext(argsp, declp->valuep()->clone_रूख(असत्य));
                    AstCCall* अचर callChgp = नूतन AstCCall{flp, funcs.chgFuncp, argsp};
                    callChgp->dtypeSetVoid();
                    callChgp->argTypes(callChgp->argTypes() + "bufp, "
                                       + std::to_string(declp->code()));
                    ifp->addThensp(callChgp->makeStmt());

                    subStmts += 2;
                } अन्यथा {
                    AstTraceInc* अचर incFulp = नूतन AstTraceInc{flp, declp, VTraceType::FULL};
                    subFulFuncp->addStmtsp(incFulp);
                    AstTraceInc* अचर incChgp
                        = नूतन AstTraceInc{flp, declp, VTraceType::CHANGE, baseCode};
                    ifp->addThensp(incChgp);

                    // Set the function index of the decl
                    declp->fidx(शीर्ष_Func_अङ्क);

                    // Track splitting due to size
                    UASSERT_OBJ(incFulp->गाँठो_गन्ती() == incChgp->गाँठो_गन्ती(), declp,
                                "Should have equal cost");
                    अचर VNumRange range = declp->arrayRange();
                    यदि (range.ranged()) {
                        // 2x because each element is a TraceInc and a VarRef
                        subStmts += range.elements() * 2;
                    } अन्यथा {
                        subStmts += incChgp->गाँठो_गन्ती();
                    }
                }

                // Track partitioning
                nCodes += declp->codeInc();
            }
        }
    }

    शून्य createCleanupFunction() {
        File_पङ्क्ति* अचर fl = m_शीर्ष_Scopep->fileline();
        AstCFunc* अचर cleanupFuncp = नूतन AstCFunc{fl, "trace_cleanup", m_शीर्ष_Scopep};
        cleanupFuncp->argTypes("void* voidSelf, " + v3Global.opt.traceClassBase()
                               + "* /*unused*/");
        cleanupFuncp->isTrace(सत्य);
        cleanupFuncp->slow(असत्य);
        cleanupFuncp->isStatic(सत्य);
        cleanupFuncp->isLoose(सत्य);
        m_शीर्ष_Scopep->addBlocksp(cleanupFuncp);
        cleanupFuncp->addStmtsp(नूतन AstCStmt{fl, EmitCUtil::voidSelfAssign(m_शीर्ष_Modp)});
        cleanupFuncp->addStmtsp(नूतन AstCStmt{fl, EmitCUtil::symClassAssign()});

        // Register it
        {
            AstCStmt* अचर cstmtp = नूतन AstCStmt{fl};
            m_regFuncp->addStmtsp(cstmtp);
            cstmtp->add("tracep->addCleanupCb(");
            cstmtp->add(नूतन AstAddrOfCFunc{fl, cleanupFuncp});
            cstmtp->add(", vlSelf);");
        }

        // Clear global activity flag
        cleanupFuncp->addStmtsp(
            नूतन AstCStmt{m_शीर्ष_Scopep->fileline(), "vlSymsp->__Vm_activity = false;"s});

        // Clear fine grained activity flags
        लागि (uint32_t i = 0; i < m_activity_सङ्ख्या; ++i) {
            Ast_गाँठो* अचर clrp = नूतन AstAssign{fl, selectActivity(fl, i, VAccess::WRITE),
                                                नूतन AstConst{fl, AstConst::BitFalse{}}};
            cleanupFuncp->addStmtsp(clrp);
        }
    }

    शून्य createTraceFunctions() {
        // Detect and remove duplicate values
        detectDuplicates();

        graphDtypePrune();
        splitTraceDeclFuncs();
        m_graph.removeRedundantEdgesMax(&V3GraphEdge::followAlwaysTrue);

        // Simplify & optimize the graph
        यदि (dumpGraphLevel() >= 6) m_graph.dumpDotFilePrefixed("trace_pre");
        graphSimplify(सत्य);
        यदि (dumpGraphLevel() >= 6) m_graph.dumpDotFilePrefixed("trace_simplified");
        graphOptimize();
        यदि (dumpGraphLevel() >= 6) m_graph.dumpDotFilePrefixed("trace_optimized");

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
        m_regFuncp = नूतन AstCFunc{m_शीर्ष_Scopep->fileline(), "trace_register", m_शीर्ष_Scopep};
        m_regFuncp->argTypes(v3Global.opt.traceClassBase() + "* tracep");
        m_regFuncp->entryPoint(सत्य);
        m_regFuncp->isTrace(सत्य);
        m_regFuncp->slow(सत्य);
        m_regFuncp->isStatic(असत्य);
        m_regFuncp->isLoose(सत्य);
        m_शीर्ष_Scopep->addBlocksp(m_regFuncp);

        // Create the const dump functions. Also allocates trace codes.
        createConstTraceFunctions(traces);

        // Create the full and incremental dump functions
        createNonConstTraceFunctions(traces, nNonConstCodes, m_parallelism);

        // Root-traced libraries alias wrapper IOs onto the existing top-module codes.
        यदि (!v3Global.opt.libCreate().empty()) {
            std::vector<AstTraceDecl*> जरा_Declps;
            std::vector<AstTraceDecl*> शीर्ष_Declps;
            TraceInitDeclCollector जरा_Collector{जरा_Declps};
            TraceInitDeclCollector शीर्ष_Collector{शीर्ष_Declps};
            लागि (Ast_गाँठो* blockp = m_शीर्ष_Scopep->blocksp(); blockp; blockp = blockp->nextp()) {
                AstCFunc* अचर funcp = VN_CAST(blockp, CFunc);
                यदि (funcp && VString::startsWith(funcp->name(), "trace_init_leaf_root__")) {
                    जरा_Collector.collect(funcp);
                } अन्यथा यदि (funcp && VString::startsWith(funcp->name(), "trace_init_leaf_top__")) {
                    शीर्ष_Collector.collect(funcp);
                }
            }

            std::vector<तार्किक> used(शीर्ष_Declps.size(), असत्य);
            लागि (AstTraceDecl* अचर जरा_Declp : जरा_Declps) {
                लागि (माप_प्रकार i = 0; i < शीर्ष_Declps.size(); ++i) {
                    यदि (used[i] || !same_जरा_Init_Alias(जरा_Declp, शीर्ष_Declps[i])) जारी;
                    जरा_Declp->code(शीर्ष_Declps[i]->code());
                    used[i] = सत्य;
                    रोक;
                }
            }
        }

        // Remove refs to traced values from TraceDecl nodes, these have now moved under
        // TraceInc
        लागि (अचर स्वतः& i : traces) {
            Ast_गाँठो* अचर valuep = i.second->nodep()->valuep();
            VL_DO_DANGLING(valuep->unlinkFrBack()->delete_रूख(), valuep);
        }

        // Create the trace cleanup function clearing the activity flags
        createCleanupFunction();
    }

    TraceCFuncVertex* getCFuncVertexp(AstCFunc* nodep) {
        V3GraphVertex* अचर vtxp = nodep->user1u().toGraphVertex();
        TraceCFuncVertex* vertexp = vtxp ? vtxp->cast<TraceCFuncVertex>() : nullptr;
        यदि (!vertexp) {
            vertexp = नूतन TraceCFuncVertex{&m_graph, nodep};
            nodep->user1p(vertexp);
        }
        फर्काऊ vertexp;
    }
    TraceActivityVertex* getActivityVertexp(Ast_गाँठो* nodep, तार्किक slow) {
        V3GraphVertex* अचर vtxp = nodep->user3u().toGraphVertex();
        TraceActivityVertex* vertexp = vtxp ? vtxp->cast<TraceActivityVertex>() : nullptr;
        यदि (!vertexp) {
            vertexp = नूतन TraceActivityVertex{&m_graph, nodep, slow};
            nodep->user3p(vertexp);
        }
        vertexp->slow(slow);
        फर्काऊ vertexp;
    }

    // VISITORS
    शून्य visit(AstNetlist* nodep) override {
        // Add vertexes for all TraceDecl, and edges from VARs each trace looks at
        m_finding = असत्य;
        iterateChildren(nodep);

        // Add vertexes for all CFUNCs, and edges to VARs the func sets
        m_finding = सत्य;
        iterateChildren(nodep);

        // Create the trace functions and insert them into the tree
        createTraceFunctions();

        // Save number of trace codes used
        nodep->nTraceCodes(m_code);
    }
    शून्य visit(Ast_गाँठो_Module* nodep) override {
        यदि (nodep->is_शीर्ष()) m_शीर्ष_Modp = nodep;
        iterateChildren(nodep);
    }
    शून्य visit(AstVarScope* nodep) override {
        यदि (!m_finding) {
            यदि (अचर AstIface* अचर ifacep = nodep->varp()->sensIfacep()) {
                m_ifaceMemberVscps[{ifacep, nodep->varp()->name()}].थप_back(nodep);
            }
        }
    }
    शून्य visit(Ast_Stmt_अभिव्यक्ति* nodep) override {
        यदि (!m_finding && !nodep->user2()) {
            यदि (AstCCall* अचर callp = VN_CAST(nodep->exprp(), CCall)) {
                UINFO(8, "   CCALL " << callp);
                // See if there are other calls in same statement list;
                // If so, all funcs might share the same activity code
                TraceActivityVertex* अचर activityVtxp
                    = getActivityVertexp(nodep, callp->funcp()->slow());
                लागि (Ast_गाँठो* nextp = nodep; nextp; nextp = nextp->nextp()) {
                    यदि (Ast_Stmt_अभिव्यक्ति* अचर stmtp = VN_CAST(nextp, Stmt_अभिव्यक्ति)) {
                        यदि (AstCCall* अचर ccallp = VN_CAST(stmtp->exprp(), CCall)) {
                            stmtp->user2(सत्य);  // Processed
                            UINFO(8, "     SubCCALL " << ccallp);
                            V3GraphVertex* अचर ccallFuncVtxp = getCFuncVertexp(ccallp->funcp());
                            activityVtxp->slow(ccallp->funcp()->slow());
                            नूतन V3GraphEdge{&m_graph, activityVtxp, ccallFuncVtxp, 1};
                        }
                    }
                }
            }
        }
        iterateChildren(nodep);
    }
    शून्य visit(AstCFunc* nodep) override {
        UINFO(8, "   CFUNC " << nodep);
        V3GraphVertex* अचर funcVtxp = getCFuncVertexp(nodep);
        यदि (!m_finding) {  // If public, we need a unique activity code to allow for sets
                           // directly in this func
            यदि (nodep->funcPublic() || nodep->dpiExportImpl() || nodep->entryPoint()
                || nodep->isCoroutine()) {
                // Cannot treat a coroutine as slow, it may be resumed later
                अचर तार्किक slow = nodep->slow() && !nodep->isCoroutine();
                TraceActivityVertex* अचर activityVtxp = getActivityVertexp(nodep, slow);
                नूतन V3GraphEdge{&m_graph, activityVtxp, funcVtxp, 1};
            }
        }
        VL_RESTORER(m_cfuncp);
        m_cfuncp = nodep;
        iterateChildren(nodep);
    }
    शून्य visit(AstTraceDecl* nodep) override {
        UINFO(8, "   TRACE " << nodep);
        यदि (!m_finding && !nodep->inDtypeFunc()) {
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
            यदि (m_cfuncp && VString::startsWith(m_cfuncp->name(), "trace_init_leaf_root__")) {
                फर्काऊ;
            }
            V3GraphVertex* अचर vertexp = नूतन TraceTraceVertex{&m_graph, nodep};
            nodep->user1p(vertexp);

            UASSERT_OBJ(m_cfuncp, nodep, "Trace not under func");
            m_cfuncp->user2(सत्य);
            VL_RESTORER(m_tracep);
            m_tracep = nodep;
            iterateChildren(nodep);
        }
    }
    शून्य visit(AstVarRef* nodep) override {
        UASSERT_OBJ(nodep->varScopep(), nodep, "No var scope?");
        AstVarScope* अचर varscopep = nodep->varScopep();
        V3GraphVertex* varVtxp = varscopep->user1u().toGraphVertex();
        यदि (m_tracep) {
            UASSERT_OBJ(nodep->access().isReadOnly(), nodep, "Lvalue in trace?  Should be const.");
            यदि (!varVtxp) {
                varVtxp = नूतन TraceVarVertex{&m_graph, nodep->varScopep()};
                nodep->varScopep()->user1p(varVtxp);
            }
            V3GraphVertex* अचर traceVtxp = m_tracep->user1u().toGraphVertex();
            नूतन V3GraphEdge{&m_graph, varVtxp, traceVtxp, 1};
            यदि (nodep->varp()->isPrimaryInish()  // Always need to trace primary inputs
                || nodep->varp()->isSigPublic()) {  // Or ones user can change
                नूतन V3GraphEdge{&m_graph, m_alwaysVtxp, traceVtxp, 1};
            }
            यदि (m_tracep->dtypeCallp()) varscopep->user2p(m_tracep);
        } अन्यथा यदि (m_cfuncp && m_finding && nodep->access().isWriteOrRW()) {
            V3GraphVertex* अचर funcVtxp = getCFuncVertexp(m_cfuncp);
            यदि (varVtxp) {  // else we're not tracing this signal
                नूतन V3GraphEdge{&m_graph, funcVtxp, varVtxp, 1};
                AstTraceDecl* अचर declp = VN_AS(varscopep->user2p(), TraceDecl);
                यदि (declp) {
                    V3GraphVertex* अचर cFuncVtxp = getCFuncVertexp(m_cfuncp);
                    लागि (अचर V3GraphEdge& edge : cFuncVtxp->inEdges()) {
                        V3GraphVertex* अचर activityp = edge.fromp();
                        यदि (!declp->user3p()) {
                            declp->user3p(activityp);
                        } अन्यथा यदि (declp->user3u().toGraphVertex() != activityp) {
                            declp->user2(सत्य);
                        }
                    }
                }
            }
        }
    }
    शून्य visit(AstMemberSel* nodep) override {
        यदि (m_cfuncp && m_finding && nodep->access().isWriteOrRW()) {
            AstIfaceRefDType* अचर dtypep
                = VN_CAST(nodep->fromp()->dtypep()->skipRefp(), IfaceRefDType);
            यदि (dtypep && dtypep->isVirtual()) {
                अचर स्वतः it = m_ifaceMemberVscps.find({dtypep->ifacep(), nodep->varp()->name()});
                यदि (it != m_ifaceMemberVscps.end()) {
                    V3GraphVertex* अचर funcVtxp = getCFuncVertexp(m_cfuncp);
                    लागि (AstVarScope* अचर vscp : it->second) {
                        V3GraphVertex* varVtxp = vscp->user1u().toGraphVertex();
                        यदि (!varVtxp) {
                            varVtxp = नूतन TraceVarVertex{&m_graph, vscp};
                            vscp->user1p(varVtxp);
                        }
                        नूतन V3GraphEdge{&m_graph, funcVtxp, varVtxp, 1};
                    }
                }
            }
        }
        iterateChildren(nodep);
    }
    //--------------------
    शून्य visit(Ast_गाँठो* nodep) override { iterateChildren(nodep); }

सार्वजनिक:
    // CONSTRUCTORS
    explicit TraceVisitor(AstNetlist* nodep)
        : m_alwaysVtxp{नूतन TraceActivityVertex{&m_graph, TraceActivityVertex::ACTIVITY_ALWAYS}} {
        nodep->user2_Clear_रूख();  // TraceDecl multiple activities flag
        nodep->user3_Clear_रूख();  // TraceDecl TraceActivityVertex (assumes we start at nullptr)
        iterate(nodep);
        nodep->foreach([](AstTraceDecl* अचर declp) {
            यदि (declp->inDtypeFunc()) {
                declp->valuep()->unlinkFrBack()->delete_रूख();
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

शून्य V3Trace::traceAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { TraceVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_रूख("trace", 0, dump_रूख_Either_Level() >= 3);
}
