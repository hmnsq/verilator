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

वग्ग TraceActivityVertex final : सार्वजनिक V3GraphVertex {
    VL_RTTI_IMPL(TraceActivityVertex, V3GraphVertex)
    Ast_ग्रन्थिः* ध्रुव m_insertp;
    int32_t m_activityCode;
    तार्किक m_slow;  // If always slow, we can use the same code
सार्वजनिक:
    गणना { ACTIVITY_NEVER = ((1UL << 31) - 1) };
    गणना { ACTIVITY_ALWAYS = ((1UL << 31) - 2) };
    गणना { ACTIVITY_SLOW = 0 };
    TraceActivityVertex(V3Graph* graphp, Ast_ग्रन्थिः* nodep, तार्किक slow)
        : V3GraphVertex{graphp}
        , m_insertp{nodep} {
        m_activityCode = 0;
        m_slow = slow;
    }
    TraceActivityVertex(V3Graph* graphp, int32_t code)
        : V3GraphVertex{graphp}
        , m_insertp{nullptr} {
        m_activityCode = code;
        m_slow = मिच्छा;
    }
    ~TraceActivityVertex() override = स्वभाव;
    // ACCESSORS
    Ast_ग्रन्थिः* insertp() ध्रुव {
        UASSERT(m_insertp, "Null insertp; probably called on a special always/slow");
        पटिनिवत्त m_insertp;
    }
    string name() ध्रुव override {
        सचे (activityAlways()) {
            पटिनिवत्त "*ALWAYS*";
        } अञ्ञथा {
            पटिनिवत्त std::string{slow() ? "*SLOW* " : ""} + insertp()->name();
        }
    }
    string dotColor() ध्रुव override { पटिनिवत्त slow() ? "yellowGreen" : "green"; }
    int32_t activityCode() ध्रुव { पटिनिवत्त m_activityCode; }
    तार्किक activityAlways() ध्रुव { पटिनिवत्त activityCode() == ACTIVITY_ALWAYS; }
    तार्किक activitySlow() ध्रुव { पटिनिवत्त activityCode() == ACTIVITY_SLOW; }
    शून्य activityCode(int32_t code) { m_activityCode = code; }
    तार्किक slow() ध्रुव { पटिनिवत्त m_slow; }
    शून्य slow(तार्किक flag) {
        सचे (!flag) m_slow = मिच्छा;
    }
};

वग्ग TraceCFuncVertex final : सार्वजनिक V3GraphVertex {
    VL_RTTI_IMPL(TraceCFuncVertex, V3GraphVertex)
    AstCFunc* ध्रुव m_nodep;

सार्वजनिक:
    TraceCFuncVertex(V3Graph* graphp, AstCFunc* nodep)
        : V3GraphVertex{graphp}
        , m_nodep{nodep} {}
    ~TraceCFuncVertex() override = स्वभाव;
    // ACCESSORS
    AstCFunc* nodep() ध्रुव { पटिनिवत्त m_nodep; }
    string name() ध्रुव override { पटिनिवत्त nodep()->name(); }
    string dotColor() ध्रुव override { पटिनिवत्त "yellow"; }
    File_पङ्क्तिः* fileline() ध्रुव override { पटिनिवत्त nodep()->fileline(); }
};

वग्ग TraceTraceVertex final : सार्वजनिक V3GraphVertex {
    VL_RTTI_IMPL(TraceTraceVertex, V3GraphVertex)
    AstTraceDecl* ध्रुव m_nodep;  // TRACEINC this represents
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
    AstTraceDecl* nodep() ध्रुव { पटिनिवत्त m_nodep; }
    string name() ध्रुव override { पटिनिवत्त nodep()->name(); }
    string dotColor() ध्रुव override { पटिनिवत्त "red"; }
    File_पङ्क्तिः* fileline() ध्रुव override { पटिनिवत्त nodep()->fileline(); }
    TraceTraceVertex* duplicatep() ध्रुव { पटिनिवत्त m_duplicatep; }
    शून्य duplicatep(TraceTraceVertex* dupp) {
        UASSERT_OBJ(!duplicatep(), nodep(), "Assigning duplicatep() to already duplicated node");
        m_duplicatep = dupp;
    }
    शून्य redirectDuplicatep(TraceTraceVertex* dupp) { m_duplicatep = dupp; }
    uint32_t dtypeAliasOffset() ध्रुव { पटिनिवत्त m_dtypeAliasOffset; }
    शून्य dtypeAliasOffset(uint32_t offset) { m_dtypeAliasOffset = offset; }
};

वग्ग TraceVarVertex final : सार्वजनिक V3GraphVertex {
    VL_RTTI_IMPL(TraceVarVertex, V3GraphVertex)
    AstVarScope* ध्रुव m_nodep;

सार्वजनिक:
    TraceVarVertex(V3Graph* graphp, AstVarScope* nodep)
        : V3GraphVertex{graphp}
        , m_nodep{nodep} {}
    ~TraceVarVertex() override = स्वभाव;
    // ACCESSORS
    AstVarScope* nodep() ध्रुव { पटिनिवत्त m_nodep; }
    string name() ध्रुव override { पटिनिवत्त nodep()->name(); }
    string dotColor() ध्रुव override { पटिनिवत्त "skyblue"; }
    File_पङ्क्तिः* fileline() ध्रुव override { पटिनिवत्त nodep()->fileline(); }
};

//######################################################################
// Trace state, as a visitor of each AstNode

वग्ग TraceVisitor final : सार्वजनिक VNVisitor {
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
    ध्रुव VNUser1InUse m_inuser1;
    ध्रुव VNUser2InUse m_inuser2;
    ध्रुव VNUser3InUse m_inuser3;
    // VNUser4InUse     In V3Hasher via V3DupFinder

    // STATE
    Ast_ग्रन्थिः_Module* m_शिखरम्_Modp = nullptr;  // Module to add variables to
    AstScope* ध्रुव m_शिखरम्_Scopep = v3Global.rootp()->शिखरम्_Scopep()->scopep();  // The top AstScope
    AstCFunc* m_cfuncp = nullptr;  // C function adding to graph
    AstCFunc* m_regFuncp = nullptr;  // Trace registration function
    AstCFunc* m_actAllFuncp = nullptr;  // Set all activity function
    AstTraceDecl* m_tracep = nullptr;  // Trace function adding to graph
    AstVarScope* m_activityVscp = nullptr;  // Activity variable
    uint32_t m_activity_सङ्ख्या = 0;  // Count of fields in activity variable
    uint32_t m_code = 0;  // Trace ident code# being assigned
    V3Graph m_graph;  // Var/CFunc tracking
    TraceActivityVertex* ध्रुव m_alwaysVtxp;  // "Always trace" vertex
    तार्किक m_finding = मिच्छा;  // Pass one of algorithm?
    संरचना DtypeFuncs final {
    सार्वजनिक:
        AstCFunc* fullFuncp = nullptr;
        AstCFunc* chgFuncp = nullptr;
    };
    std::unordered_map<ध्रुव Ast_ग्रन्थिः_DType*, DtypeFuncs>
        m_dtypeNonConstFuncs;  // Full / Chg funcs per type
    std::unordered_map<ध्रुव Ast_ग्रन्थिः_DType*, AstCFunc*> m_dtypeConstFuncs;  // Const func per type
    V3UniqueNames m_dtypeNames{""};  // Unique type func names

    // Trace parallelism. Only VCD tracing can be parallelized at this time.
    ध्रुव uint32_t m_parallelism
        = v3Global.opt.useTraceParallel() ? static_cast<uint32_t>(v3Global.opt.threads()) : 1;

    VDouble0 m_statSetters;  // Statistic tracking
    VDouble0 m_statSettersSlow;  // Statistic tracking
    VDouble0 m_statUniqSigs;  // Statistic tracking

    // All activity numbers applying to a given trace
    using ActCodeSet = std::set<uint32_t>;
    // For activity set, what traces apply
    using TraceVec = std::multimap<ActCodeSet, TraceTraceVertex*>;
    // Candidate interface-member VarScopes keyed by (interface type, member name)
    std::map<std::pair<ध्रुव AstIface*, std::string>, std::vector<AstVarScope*>>
        m_ifaceMemberVscps;

    वग्ग TraceInitDeclCollector final : सार्वजनिक VNVisitor {
        std::vector<AstTraceDecl*>& m_declps;  // Output: trace declarations found so far
        std::set<ध्रुव AstCFunc*> m_seenFuncps;

        शून्य visit(AstTraceDecl* nodep) override { m_declps.प्रक्षिप_back(nodep); }
        शून्य visit(AstCCall* nodep) override {
            सचे (AstCFunc* ध्रुव funcp = nodep->funcp()) collect(funcp);
        }
        शून्य visit(Ast_ग्रन्थिः* nodep) override { iterateChildren(nodep); }

    सार्वजनिक:
        explicit TraceInitDeclCollector(std::vector<AstTraceDecl*>& declps)
            : m_declps{declps} {}
        शून्य collect(AstCFunc* funcp) {
            सचे (funcp && m_seenFuncps.निवेशय(funcp).second) iterate(funcp);
        }
    };

    // METHODS

    स्थिर तार्किक same_मूलम्_Init_Alias(ध्रुव AstTraceDecl* मूलम्_Declp, ध्रुव AstTraceDecl* शिखरम्_Declp) {
        ध्रुव VNumRange& मूलम्_Bit_Range = मूलम्_Declp->bitRange();
        ध्रुव VNumRange& शिखरम्_Bit_Range = शिखरम्_Declp->bitRange();
        पटिनिवत्त मूलम्_Declp->showname() == शिखरम्_Declp->showname()
               && मूलम्_Declp->declDirection() == शिखरम्_Declp->declDirection()
               && मूलम्_Declp->widthMin() == शिखरम्_Declp->widthMin()
               && मूलम्_Bit_Range.ranged() == शिखरम्_Bit_Range.ranged()
               && (!मूलम्_Bit_Range.ranged()
                   || (मूलम्_Bit_Range.वामः() == शिखरम्_Bit_Range.वामः()
                       && मूलम्_Bit_Range.दक्षिणः() == शिखरम्_Bit_Range.दक्षिणः()));
    }

    शून्य detectDuplicates() {
        UINFO(9, "Finding duplicates");
        // Note uses user4
        V3DupFinder dupFinder;  // Duplicate code detection

        // Compute member offsets within dtype instances incrementally.
        // For each dtype member TraceDecl, stores its code offset within the
        // dtype parent's code range.
        std::unordered_map<ध्रुव AstTraceDecl*, uint32_t> memberOffsets;
        std::unordered_map<ध्रुव AstTraceDecl*, uint32_t> runningOffset;

        // For fixup pass: track dups redirected to dtype parents, so we can
        // fix them if the parent's user2 is later set during the loop.
        // Maps dup vertex -> original canonical member vertex.
        std::vector<std::pair<TraceTraceVertex*, TraceTraceVertex*>> dtypeParentRedirects;

        // Hash all of the traced values and find if there are any duplicates
        अत्थाय (V3GraphVertex& vtx : m_graph.vertices()) {
            सचे (TraceTraceVertex* ध्रुव vvertexp = vtx.cast<TraceTraceVertex>()) {
                AstTraceDecl* ध्रुव nodep = vvertexp->nodep();
                सचे (nodep->dtypeCallp()) अनुवर्त;

                सचे (nodep->dtypeDeclp()) {
                    uint32_t& offset = runningOffset[nodep->dtypeDeclp()];
                    memberOffsets[nodep] = offset;
                    offset += nodep->codeInc();
                }

                UASSERT_OBJ(!vvertexp->duplicatep(), nodep, "Should not be a duplicate");
                ध्रुव auto dupit = dupFinder.findDuplicate(nodep);
                सचे (dupit == dupFinder.end()) {
                    dupFinder.निवेशय(nodep);
                } अञ्ञथा {
                    AstTraceDecl* ध्रुव dupDeclp = VN_AS(dupit->second, TraceDecl);
                    UASSERT_OBJ(dupDeclp, nodep, "Trace duplicate of wrong type");
                    TraceTraceVertex* ध्रुव dupvertexp
                        = dupDeclp->user1u().toGraphVertex()->cast<TraceTraceVertex>();
                    UINFO(8, "  Orig " << dupDeclp << endl);
                    UINFO(8, "   dup " << nodep << endl);

                    सचे (dupDeclp->dtypeDeclp() && !dupDeclp->dtypeDeclp()->user2()) {
                        AstTraceDecl* ध्रुव dtypeParentp = dupDeclp->dtypeDeclp();
                        TraceTraceVertex* ध्रुव dtypeVtxp
                            = dtypeParentp->user1u().toGraphVertex()->cast<TraceTraceVertex>();
                        ध्रुव auto it2 = memberOffsets.find(dupDeclp);
                        UASSERT_OBJ(it2 != memberOffsets.end(), dupDeclp,
                                    "Member offset not precomputed");
                        vvertexp->duplicatep(dtypeVtxp);
                        vvertexp->dtypeAliasOffset(it2->second);
                        dtypeParentRedirects.emplace_back(vvertexp, dupvertexp);
                    } अञ्ञथा {
                        vvertexp->duplicatep(dupvertexp);
                    }
                    सचे (nodep->dtypeDeclp()) nodep->dtypeDeclp()->user2(सच्च);
                }
            }
        }

        अत्थाय (ध्रुव auto& pair : dtypeParentRedirects) {
            TraceTraceVertex* ध्रुव dupVtxp = pair.first;
            TraceTraceVertex* ध्रुव memberVtxp = pair.second;
            ध्रुव AstTraceDecl* ध्रुव parentDeclp = dupVtxp->duplicatep()->nodep();
            सचे (parentDeclp->user2()) {
                dupVtxp->redirectDuplicatep(memberVtxp);
                dupVtxp->dtypeAliasOffset(0);
            }
        }

        सचे (dumpLevel() || debug() >= 9)
            dupFinder.dumpFile(v3Global.debugFilename("trace") + ".hash", मिच्छा);
    }

    शून्य graphDtypePrune() {
        अत्थाय (V3GraphVertex* ध्रुव vtxp : m_graph.vertices().unlinkable()) {
            सचे (TraceTraceVertex* ध्रुव vvertexp = vtxp->cast<TraceTraceVertex>()) {
                AstTraceDecl* ध्रुव declp = vvertexp->nodep();
                // This skips the dtype sub-func optimization if a var is affected by multiple
                // activities.  We really only need to do this for _chg funcs (and not decls,
                // _const and _full) but it's simpiler to do it all one way or the other.
                सचे (declp) {
                    सचे (declp->user2() || (declp->dtypeDeclp() && !declp->dtypeDeclp()->user2())) {
                        AstCCall* ध्रुव callp = declp->dtypeCallp();
                        सचे (callp) {
                            Ast_ग्रन्थिः* stmtexprp = callp->backp();
                            VL_DO_DANGLING(प्रक्षिप_Deletep(stmtexprp->unlinkFrBack()), stmtexprp);
                        } अञ्ञथा {
                            तार्किक emptyScope;
                            करोहि {
                                emptyScope = मिच्छा;
                                Ast_ग्रन्थिः* ध्रुव declBackp = declp->backp();
                                Ast_ग्रन्थिः* ध्रुव declNextp = declp->nextp();
                                सचे (VN_IS(declBackp, Trace_प्रक्षिप_Prefix)
                                    && VN_IS(declNextp, Trace_निष्कासय_Prefix)) {
                                    VL_DO_DANGLING(प्रक्षिप_Deletep(declBackp->unlinkFrBack()),
                                                   declBackp);
                                    VL_DO_DANGLING(प्रक्षिप_Deletep(declNextp->unlinkFrBack()),
                                                   declNextp);
                                    emptyScope = सच्च;
                                }
                            } याव (emptyScope);
                        }
                        // Can't purge until we finish this pass
                        प्रक्षिप_Deletep(declp->unlinkFrBack());  // declp used below
                        vvertexp->rerouteEdges(&m_graph);
                        vvertexp->unlinkDelete(&m_graph);
                    }
                    declp->dtypeDeclp(nullptr);
                }
            }
        }
    }

    शून्य splitTraceDeclFuncs() {
        ध्रुव पूर्णाङ्क splitLimit = v3Global.opt.outputSplitCTrace() ? v3Global.opt.outputSplitCTrace()
                                                                : std::numeric_limits<पूर्णाङ्क>::max();
        सचे (splitLimit == std::numeric_limits<पूर्णाङ्क>::max()) पटिनिवत्त;

        अत्थाय (Ast_ग्रन्थिः* nodep = m_शिखरम्_Scopep->blocksp(); nodep; nodep = nodep->nextp()) {
            AstCFunc* ध्रुव funcp = VN_CAST(nodep, CFunc);
            सचे (!funcp || !funcp->user2()) अनुवर्त;

            ध्रुव string baseName = funcp->name();
            uint32_t sub_अङ्कः = 1;
            पूर्णाङ्क curSize = 0;
            AstCFunc* curFuncp = funcp;
            std::vector<Ast_ग्रन्थिः_Stmt*> callStmts;

            auto startNewSubFunc = [&]() {
                File_पङ्क्तिः* ध्रुव flp = funcp->fileline();
                ध्रुव string newName = baseName + "_" + cvtToStr(sub_अङ्कः++);
                AstCFunc* ध्रुव newFuncp = नूतन AstCFunc{flp, newName, m_शिखरम्_Scopep};
                newFuncp->argTypes(v3Global.opt.traceClassBase() + "* tracep");
                newFuncp->isTrace(सच्च);
                newFuncp->isStatic(मिच्छा);
                newFuncp->isLoose(सच्च);
                newFuncp->slow(सच्च);
                newFuncp->dontCombine(सच्च);
                m_शिखरम्_Scopep->addBlocksp(newFuncp);
                newFuncp->addStmtsp(नूतन AstCStmt{flp, "const int c = vlSymsp->__Vm_baseCode;"});
                AstCCall* ध्रुव callp = नूतन AstCCall{flp, newFuncp};
                callp->dtypeSetVoid();
                callp->argTypes("tracep");
                callStmts.प्रक्षिप_back(callp->makeStmt());
                curFuncp = newFuncp;
                curSize = 0;
            };

            अत्थाय (Ast_ग्रन्थिः *stmtp = funcp->stmtsp(), *nextp; stmtp; stmtp = nextp) {
                nextp = stmtp->nextp();
                curSize += stmtp->ग्रन्थिः_गणनम्();

                सचे (curFuncp != funcp) {
                    stmtp->unlinkFrBack();
                    curFuncp->addStmtsp(stmtp);
                }

                सचे (curSize > splitLimit && nextp) startNewSubFunc();
            }

            अत्थाय (Ast_ग्रन्थिः_Stmt* ध्रुव callStmtp : callStmts) { funcp->addStmtsp(callStmtp); }
        }
    }

    शून्य graphSimplify(तार्किक initial) {
        सचे (initial) {
            // Remove all variable nodes
            अत्थाय (V3GraphVertex* ध्रुव vtxp : m_graph.vertices().unlinkable()) {
                सचे (TraceVarVertex* ध्रुव vvertexp = vtxp->cast<TraceVarVertex>()) {
                    vvertexp->rerouteEdges(&m_graph);
                    vvertexp->unlinkDelete(&m_graph);
                }
            }
            // Remove multiple variables connecting funcs to traces
            // We do this twice, as then we have fewer edges to multiply out in the below
            // expansion.
            m_graph.removeRedundantEdgesMax(&V3GraphEdge::followAlwaysTrue);
            // Remove all Cfunc nodes
            अत्थाय (V3GraphVertex* ध्रुव vtxp : m_graph.vertices().unlinkable()) {
                सचे (TraceCFuncVertex* ध्रुव vvertexp = vtxp->cast<TraceCFuncVertex>()) {
                    vvertexp->rerouteEdges(&m_graph);
                    vvertexp->unlinkDelete(&m_graph);
                }
            }
        }

        // Remove multiple variables connecting funcs to traces
        m_graph.removeRedundantEdgesMax(&V3GraphEdge::followAlwaysTrue);

        // If there are any edges from a always, keep only the always
        अत्थाय (V3GraphVertex& vtx : m_graph.vertices()) {
            सचे (TraceTraceVertex* ध्रुव vvertexp = vtx.cast<TraceTraceVertex>()) {
                // Search for the incoming always edge
                ध्रुव V3GraphEdge* alwaysEdgep = nullptr;
                अत्थाय (ध्रुव V3GraphEdge& edge : vvertexp->inEdges()) {
                    ध्रुव TraceActivityVertex* ध्रुव actVtxp
                        = edge.fromp()->as<ध्रुव TraceActivityVertex>();
                    सचे (actVtxp->activityAlways()) {
                        alwaysEdgep = &edge;
                        भङ्ग;
                    }
                }
                // If always edge exists, remove all other edges
                सचे (alwaysEdgep) {
                    अत्थाय (V3GraphEdge* ध्रुव edgep : vvertexp->inEdges().unlinkable()) {
                        सचे (edgep != alwaysEdgep) VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
                    }
                }
            }
        }

        // Activity points with no outputs can be removed
        अत्थाय (V3GraphVertex* ध्रुव vtxp : m_graph.vertices().unlinkable()) {
            सचे (TraceActivityVertex* ध्रुव aVtxp = vtxp->cast<TraceActivityVertex>()) {
                // Leave in the always vertex for later use.
                सचे (aVtxp != m_alwaysVtxp && aVtxp->outEmpty()) {
                    VL_DO_DANGLING(aVtxp->unlinkDelete(&m_graph), aVtxp);
                }
            }
        }
    }

    uint32_t assignactivityNumbers() {
        uint32_t activity_सङ्ख्या = 1;  // Note 0 indicates "slow" only
        अत्थाय (V3GraphVertex& vtx : m_graph.vertices()) {
            सचे (TraceActivityVertex* ध्रुव vvertexp = vtx.cast<TraceActivityVertex>()) {
                सचे (vvertexp != m_alwaysVtxp) {
                    सचे (vvertexp->slow()) {
                        vvertexp->activityCode(TraceActivityVertex::ACTIVITY_SLOW);
                    } अञ्ञथा {
                        vvertexp->activityCode(activity_सङ्ख्या++);
                    }
                }
            }
        }
        पटिनिवत्त activity_सङ्ख्या;
    }

    शून्य sortTraces(TraceVec& traces, uint32_t& nNonConstCodes) {
        // Populate sort structure
        traces.clear();
        nNonConstCodes = 0;
        अत्थाय (V3GraphVertex& vtx : m_graph.vertices()) {
            सचे (TraceTraceVertex* ध्रुव vtxp = vtx.cast<TraceTraceVertex>()) {
                ActCodeSet actSet;
                UINFO(9, "  Add to sort: " << vtxp);
                UINFOTREE(9, vtxp->nodep(), "", "trnode");
                अत्थाय (ध्रुव V3GraphEdge& edge : vtxp->inEdges()) {
                    ध्रुव TraceActivityVertex* ध्रुव cfvertexp
                        = edge.fromp()->cast<ध्रुव TraceActivityVertex>();
                    UASSERT_OBJ(cfvertexp, vtxp->nodep(),
                                "Should have been function pointing to this trace");
                    UINFO(9, "   Activity: " << cfvertexp);
                    सचे (cfvertexp->activityAlways()) {
                        // If code 0, we always trace; ignore other codes
                        actSet.निवेशय(TraceActivityVertex::ACTIVITY_ALWAYS);
                    } अञ्ञथा {
                        actSet.निवेशय(cfvertexp->activityCode());
                    }
                }
                UASSERT_OBJ(actSet.गणनम्(TraceActivityVertex::ACTIVITY_ALWAYS) == 0
                                || actSet.size() == 1,
                            vtxp->nodep(), "Always active trace has further triggers");
                // Count nodes
                सचे (!vtxp->duplicatep() && !actSet.empty())
                    nNonConstCodes += vtxp->nodep()->codeInc();
                सचे (actSet.empty()) {
                    // If a trace doesn't have activity, it's constant, and we
                    // don't need to track changes on it.
                    actSet.निवेशय(TraceActivityVertex::ACTIVITY_NEVER);
                } अञ्ञथा सचे (actSet.गणनम्(TraceActivityVertex::ACTIVITY_SLOW) && actSet.size() > 1) {
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
        auto it = traces.begin();
        ध्रुव auto end = traces.end();
        याव (it != end) {
            auto head = it;
            // Approximate the complexity of the value change check
            uint32_t complexity = 0;
            ध्रुव ActCodeSet& actSet = it->first;
            अत्थाय (; it != end && it->first == actSet; ++it) {
                सचे (!it->second->duplicatep()) {
                    uint32_t cost = 0;
                    ध्रुव AstTraceDecl* ध्रुव declp = it->second->nodep();
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
            सचे (actSet.गणनम्(TraceActivityVertex::ACTIVITY_ALWAYS)) अनुवर्त;
            सचे (actSet.गणनम्(TraceActivityVertex::ACTIVITY_NEVER)) अनुवर्त;
            सचे (actSet.गणनम्(TraceActivityVertex::ACTIVITY_SLOW)) अनुवर्त;
            // If the value comparisons are cheaper to perform than checking the
            // activity flags make the signals always traced. Note this cost
            // equation is heuristic.
            सचे (complexity <= actSet.size() * 2) {
                अत्थाय (; head != it; ++head) {
                    नूतन V3GraphEdge{&m_graph, m_alwaysVtxp, head->second, 1};
                }
            }
        }

        graphSimplify(मिच्छा);
    }

    Ast_ग्रन्थिः_व्यञ्जकम्* selectActivity(File_पङ्क्तिः* flp, uint32_t acode, ध्रुव VAccess& access) {
        पटिनिवत्त नूतन AstArraySel(flp, नूतन AstVarRef{flp, m_activityVscp, access}, acode);
    }

    Ast_ग्रन्थिः* newActivitySetter(Ast_ग्रन्थिः* insertp, uint32_t code) {
        ++m_statSetters;
        File_पङ्क्तिः* ध्रुव fl = insertp->fileline();
        AstAssign* ध्रुव setterp = नूतन AstAssign{fl, selectActivity(fl, code, VAccess::WRITE),
                                                 नूतन AstConst{fl, AstConst::BitTrue{}}};
        पटिनिवत्त setterp;
    }

    Ast_ग्रन्थिः* newActivityAll(Ast_ग्रन्थिः* insertp) {
        ++m_statSettersSlow;
        सचे (!m_actAllFuncp) {
            File_पङ्क्तिः* ध्रुव flp = m_शिखरम्_Scopep->fileline();
            AstCFunc* ध्रुव funcp = नूतन AstCFunc{flp, "__VtraceActivitySetAll", m_शिखरम्_Scopep};
            funcp->slow(सच्च);
            funcp->isStatic(मिच्छा);
            funcp->isLoose(सच्च);
            m_शिखरम्_Scopep->addBlocksp(funcp);
            अत्थाय (uint32_t code = 0; code < m_activity_सङ्ख्या; ++code) {
                Ast_ग्रन्थिः* ध्रुव setterp = newActivitySetter(insertp, code);
                funcp->addStmtsp(setterp);
            }
            m_actAllFuncp = funcp;
        }
        AstCCall* ध्रुव callp = नूतन AstCCall{insertp->fileline(), m_actAllFuncp};
        callp->dtypeSetVoid();
        पटिनिवत्त callp->makeStmt();
    }

    शून्य createActivityFlags() {
        // Assign final activity numbers
        m_activity_सङ्ख्या = assignactivityNumbers();

        // Create an array of bytes, not a bit vector, as they can be set
        // atomically by mtasks, and are cheaper to set (no need for
        // read-modify-write on the C type), and the speed of the tracing code
        // is the same on largish designs.
        File_पङ्क्तिः* ध्रुव flp = m_शिखरम्_Scopep->fileline();
        Ast_ग्रन्थिः_DType* ध्रुव newScalarDtp = नूतन AstBasicDType{flp, VFlagBitPacked{}, 1};
        v3Global.rootp()->typeTablep()->addTypesp(newScalarDtp);
        AstRange* ध्रुव newArange
            = नूतन AstRange{flp, VNumRange{static_cast<पूर्णाङ्क>(m_activity_सङ्ख्या) - 1, 0}};
        Ast_ग्रन्थिः_DType* ध्रुव newArrDtp = नूतन AstUnpackArrayDType{flp, newScalarDtp, newArange};
        v3Global.rootp()->typeTablep()->addTypesp(newArrDtp);
        m_activityVscp = v3Global.rootp()->शिखरम्_Scopep()->createTemp("__VtraceActivity", newArrDtp);

        // Insert activity setters
        अत्थाय (ध्रुव V3GraphVertex& vtx : m_graph.vertices()) {
            सचे (ध्रुव TraceActivityVertex* ध्रुव vtxp = vtx.cast<ध्रुव TraceActivityVertex>()) {
                Ast_ग्रन्थिः* setterp = nullptr;
                सचे (vtxp->activitySlow()) {
                    setterp = newActivityAll(vtxp->insertp());
                    // Just set all flags in slow code as it should be rare.
                    // This will be rolled up into a loop by V3Reloop.
                } अञ्ञथा सचे (!vtxp->activityAlways()) {
                    setterp = newActivitySetter(vtxp->insertp(), vtxp->activityCode());
                }
                सचे (setterp) {
                    Ast_ग्रन्थिः* ध्रुव insertp = vtxp->insertp();
                    सचे (Ast_Stmt_व्यञ्जकम्* ध्रुव stmtp = VN_CAST(insertp, Stmt_व्यञ्जकम्)) {
                        stmtp->addNextHere(setterp);
                    } अञ्ञथा सचे (AstCFunc* ध्रुव funcp = VN_CAST(insertp, CFunc)) {
                        // If there are awaits, insert the setter after each await
                        सचे (funcp->isCoroutine() && funcp->stmtsp()) {
                            funcp->stmtsp()->foreachAndNext([setterp](AstCAwait* awaitp) {
                                awaitp->addNextHere(setterp->clone_वृक्षः(मिच्छा));
                            });
                        }
                        funcp->addStmtsp(setterp);
                    } अञ्ञथा {
                        insertp->v3fatalSrc("Bad trace activity vertex");
                    }
                }
            }
        }
    }

    AstCFunc* newCFunc(VTraceType traceType, AstCFunc* शिखरम्_Funcp, uint32_t func_अङ्कः,
                       uint32_t baseCode = 0, ध्रुव AstTraceDecl* ध्रुव declp = nullptr,
                       तार्किक declSub = मिच्छा) {
        // Create new function
        ध्रुव तार्किक is_शिखरम्_Func = !declp && शिखरम्_Funcp == nullptr;
        std::string funcName;
        सचे (is_शिखरम्_Func || declp) {
            funcName = traceType.func_prefix();
        } अञ्ञथा {
            funcName = शिखरम्_Funcp->name();
            funcName += "_sub";
        }
        सचे (declp) {
            funcName += "_dtype";
            सचे (declSub) funcName += "_sub";
            funcName += "__";
            funcName = m_dtypeNames.get(funcName);
        } अञ्ञथा {
            funcName += "_";
            funcName += cvtToStr(func_अङ्कः);
        }

        File_पङ्क्तिः* ध्रुव flp = m_शिखरम्_Scopep->fileline();
        AstCFunc* ध्रुव funcp = नूतन AstCFunc{flp, funcName, m_शिखरम्_Scopep};
        funcp->isTrace(सच्च);
        funcp->dontCombine(सच्च);
        funcp->isLoose(सच्च);
        funcp->slow(traceType != VTraceType::CHANGE);
        funcp->isStatic(is_शिखरम्_Func);
        // Add it to top scope
        m_शिखरम्_Scopep->addBlocksp(funcp);
        ध्रुव std::string bufArg
            = v3Global.opt.traceClassBase() + "::Buffer* bufp"
              + (declp ? (", uint32_t offset, const " + declp->dtypep()->cType("", सच्च, सच्च)
                          + " __VdtypeVar")
                       : "");
        सचे (is_शिखरम्_Func) {
            // Top functions
            funcp->argTypes("void* voidSelf, " + bufArg);
            funcp->addStmtsp(नूतन AstCStmt{flp, EmitCUtil::voidSelfAssign(m_शिखरम्_Modp)});
            funcp->addStmtsp(नूतन AstCStmt{flp, EmitCUtil::symClassAssign()});
            // Add global activity check to change dump functions
            सचे (traceType == VTraceType::CHANGE) {  //
                funcp->addStmtsp(
                    नूतन AstCStmt{flp, "if (VL_UNLIKELY(!vlSymsp->__Vm_activity)) return;"});
            }
            // Register function
            AstCStmt* ध्रुव cstmtp = नूतन AstCStmt{flp};
            m_regFuncp->addStmtsp(cstmtp);
            सचे (traceType == VTraceType::CONSTANT) {
                cstmtp->add("tracep->addConstCb(");
                cstmtp->add(नूतन AstAddrOfCFunc{flp, funcp});
                cstmtp->add(", " + std::to_string(func_अङ्कः) + ", vlSelf);");
            } अञ्ञथा सचे (traceType == VTraceType::FULL) {
                cstmtp->add("tracep->addFullCb(");
                cstmtp->add(नूतन AstAddrOfCFunc{flp, funcp});
                cstmtp->add(", " + std::to_string(func_अङ्कः) + ", vlSelf);");
            } अञ्ञथा {
                cstmtp->add("tracep->addChgCb(");
                cstmtp->add(नूतन AstAddrOfCFunc{flp, funcp});
                cstmtp->add(", " + std::to_string(func_अङ्कः) + ", vlSelf);");
            }
        } अञ्ञथा {
            // Sub functions
            funcp->argTypes(bufArg);
            // Setup base references. Note in rare occasions we can end up with an empty trace
            // sub function, hence the VL_ATTR_UNUSED attributes.
            सचे (traceType != VTraceType::CHANGE) {
                // Full dump sub function
                funcp->addStmtsp(नूतन AstCStmt{flp,  //
                                              string("uint32_t* const oldp VL_ATTR_UNUSED = "
                                                     "bufp->oldp(vlSymsp->__Vm_baseCode")
                                                  + (declp ? " + offset" : "") + ");\n"});
            } अञ्ञथा {
                // Change dump sub function
                funcp->addStmtsp(नूतन AstCStmt{flp,  //
                                              "uint32_t* const oldp VL_ATTR_UNUSED = "
                                              "bufp->oldp(vlSymsp->__Vm_baseCode + "
                                                  + (declp ? " offset" : cvtToStr(baseCode))
                                                  + ");\n"});
            }
            सचे (!declp) {
                // Add call to top function
                AstCCall* ध्रुव callp = नूतन AstCCall{funcp->fileline(), funcp};
                callp->dtypeSetVoid();
                callp->argTypes("bufp");
                शिखरम्_Funcp->addStmtsp(callp->makeStmt());
            }
        }
        // Done
        UINFO(5, "  newCFunc " << funcp);
        पटिनिवत्त funcp;
    }

    AstCFunc* newCDtypeSubFunc(VTraceType traceType, ध्रुव AstTraceDecl* ध्रुव declp,
                               AstCFunc* parentp) {
        AstCFunc* ध्रुव funcp = newCFunc(traceType, nullptr, 0, 0, declp, सच्च);
        File_पङ्क्तिः* ध्रुव flp = m_शिखरम्_Scopep->fileline();
        AstCCall* ध्रुव callp = नूतन AstCCall{flp, funcp};
        callp->dtypeSetVoid();
        callp->argTypes("bufp, offset, __VdtypeVar");
        parentp->addStmtsp(callp->makeStmt());
        पटिनिवत्त funcp;
    }

    AstCFunc* createConstDtypeTraceFunctions(ध्रुव AstTraceDecl* declp) {
        ध्रुव Ast_ग्रन्थिः_DType* ध्रुव dtypep = declp->dtypep()->skipRefp();
        auto pair = m_dtypeConstFuncs.emplace(dtypep, nullptr);
        सचे (pair.second) {
            File_पङ्क्तिः* ध्रुव flp = declp->fileline();
            AstCFunc* ध्रुव funcp = newCFunc(VTraceType::CONSTANT, nullptr, 0, 0, declp);

            तार्किक first = सच्च;
            अत्थाय (Ast_ग्रन्थिः* callStmtp = declp->dtypeCallp()->funcp()->stmtsp(); callStmtp;
                 callStmtp = callStmtp->nextp()) {
                ध्रुव AstCCall* ध्रुव callp
                    = VN_CAST(VN_CAST(callStmtp, Stmt_व्यञ्जकम्)->exprp(), CCall);
                तार्किक onlyOne = first && !callStmtp->nextp();
                AstCFunc* ध्रुव subFuncp
                    = onlyOne ? funcp : newCDtypeSubFunc(VTraceType::CONSTANT, declp, funcp);
                अत्थाय (Ast_ग्रन्थिः* stmtp = callp->funcp()->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    सचे (AstTraceDecl* ध्रुव fieldDeclp = VN_CAST(stmtp, TraceDecl)) {
                        AstTraceInc* ध्रुव incp
                            = नूतन AstTraceInc{flp, fieldDeclp, VTraceType::CONSTANT};
                        subFuncp->addStmtsp(incp);
                    }
                }
                first = मिच्छा;
            }

            pair.first->second = funcp;
        }

        पटिनिवत्त pair.first->second;
    }

    शून्य createConstTraceFunctions(ध्रुव TraceVec& traces) {
        ध्रुव पूर्णाङ्क splitLimit = v3Global.opt.outputSplitCTrace() ? v3Global.opt.outputSplitCTrace()
                                                                : std::numeric_limits<पूर्णाङ्क>::max();

        AstCFunc* ध्रुव शिखरम्_Funcp = newCFunc(VTraceType::CONSTANT, nullptr, 0);
        uint32_t sub_Func_अङ्कः = 0;
        AstCFunc* subFuncp = nullptr;
        पूर्णाङ्क subStmts = 0;
        std::vector<ध्रुव TraceTraceVertex*> duplicates;
        अत्थाय (auto it = traces.cbegin(); it != traces.end(); ++it) {
            ध्रुव TraceTraceVertex* ध्रुव vtxp = it->second;
            AstTraceDecl* ध्रुव declp = vtxp->nodep();
            सचे (vtxp->duplicatep()) {
                duplicates.प्रक्षिप_back(vtxp);
                अनुवर्त;
            }

            // This is a canonical trace node. Assign trace code (signal number).
            UASSERT_OBJ(!declp->codeAssigned(), declp,
                        "Canonical node should not have code assigned yet");
            declp->code(m_code);
            ध्रुव uint32_t codeInc = declp->codeInc();
            m_code += codeInc;
            ++m_statUniqSigs;

            // If this is a const signal, add the AstTraceInc
            ध्रुव ActCodeSet& actSet = it->first;
            सचे (actSet.गणनम्(TraceActivityVertex::ACTIVITY_NEVER)) {
                // Crate new sub function if required
                सचे (!subFuncp || subStmts > splitLimit) {
                    subStmts = 0;
                    subFuncp = newCFunc(VTraceType::CONSTANT, शिखरम्_Funcp, sub_Func_अङ्कः);
                    ++sub_Func_अङ्कः;
                }
                File_पङ्क्तिः* ध्रुव flp = declp->fileline();
                सचे (declp->dtypeCallp()) {
                    AstCFunc* ध्रुव funcp = createConstDtypeTraceFunctions(declp);
                    Ast_ग्रन्थिः_व्यञ्जकम्* argsp = nullptr;
                    argsp = Ast_ग्रन्थिः::addNext(argsp, declp->valuep()->clone_वृक्षः(मिच्छा));
                    AstCCall* ध्रुव callp = नूतन AstCCall{flp, funcp, argsp};
                    callp->dtypeSetVoid();
                    callp->argTypes(callp->argTypes() + "bufp, " + std::to_string(declp->code()));
                    subFuncp->addStmtsp(callp->makeStmt());

                    subStmts += 1;
                } अञ्ञथा {
                    AstTraceInc* ध्रुव incp = नूतन AstTraceInc{flp, declp, VTraceType::CONSTANT};
                    subFuncp->addStmtsp(incp);
                    subStmts += incp->ग्रन्थिः_गणनम्();
                }
            }
        }

        // Assign codes to duplicate nodes from their canonicals
        अत्थाय (ध्रुव TraceTraceVertex* ध्रुव vtxp : duplicates) {
            AstTraceDecl* ध्रुव declp = vtxp->nodep();
            ध्रुव TraceTraceVertex* ध्रुव canonVtxp = vtxp->duplicatep();
            ध्रुव AstTraceDecl* ध्रुव canonDeclp = canonVtxp->nodep();
            UASSERT_OBJ(!canonVtxp->duplicatep(), canonDeclp, "Canonical node is a duplicate");
            UASSERT_OBJ(canonDeclp->codeAssigned(), canonDeclp,
                        "Canonical node should have code assigned already");
            declp->code(canonDeclp->code() + vtxp->dtypeAliasOffset());
        }
    }

    DtypeFuncs createNonConstDtypeTraceFunctions(ध्रुव AstTraceDecl* declp) {
        Ast_ग्रन्थिः_DType* dtypep = declp->dtypep()->skipRefp();
        auto pair = m_dtypeNonConstFuncs.emplace(dtypep, DtypeFuncs{});
        सचे (pair.second) {
            File_पङ्क्तिः* ध्रुव flp = declp->fileline();
            AstCFunc* ध्रुव fullFuncp = newCFunc(VTraceType::FULL, nullptr, 0, 0, declp);
            AstCFunc* ध्रुव chgFuncp = newCFunc(VTraceType::CHANGE, nullptr, 0, 0, declp);

            तार्किक first = सच्च;
            अत्थाय (Ast_ग्रन्थिः* callStmtp = declp->dtypeCallp()->funcp()->stmtsp(); callStmtp;
                 callStmtp = callStmtp->nextp()) {
                ध्रुव AstCCall* ध्रुव callp
                    = VN_CAST(VN_CAST(callStmtp, Stmt_व्यञ्जकम्)->exprp(), CCall);
                तार्किक onlyOne = first && !callStmtp->nextp();
                AstCFunc* ध्रुव fullSubFuncp
                    = onlyOne ? fullFuncp : newCDtypeSubFunc(VTraceType::FULL, declp, fullFuncp);
                AstCFunc* ध्रुव chgSubFuncp
                    = onlyOne ? chgFuncp : newCDtypeSubFunc(VTraceType::CHANGE, declp, chgFuncp);
                अत्थाय (Ast_ग्रन्थिः* stmtp = callp->funcp()->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    सचे (AstTraceDecl* ध्रुव fieldDeclp = VN_CAST(stmtp, TraceDecl)) {
                        AstTraceInc* ध्रुव incFullp
                            = नूतन AstTraceInc{flp, fieldDeclp, VTraceType::FULL};
                        fullSubFuncp->addStmtsp(incFullp);
                        AstTraceInc* ध्रुव incChgp
                            = नूतन AstTraceInc{flp, fieldDeclp, VTraceType::CHANGE};
                        chgSubFuncp->addStmtsp(incChgp);
                    }
                }
                first = मिच्छा;
            }

            pair.first->second = {.fullFuncp = fullFuncp, .chgFuncp = chgFuncp};
        }

        पटिनिवत्त pair.first->second;
    }

    शून्य createNonConstTraceFunctions(ध्रुव TraceVec& traces, uint32_t nAllCodes,
                                      uint32_t parallelism) {
        ध्रुव पूर्णाङ्क splitLimit = v3Global.opt.outputSplitCTrace() ? v3Global.opt.outputSplitCTrace()
                                                                : std::numeric_limits<पूर्णाङ्क>::max();

        // pre-incremented, so starts at 0
        uint32_t शिखरम्_Func_अङ्कः = std::numeric_limits<uint32_t>::max();
        TraceVec::const_iterator it = traces.begin();
        याव (it != traces.end()) {
            AstCFunc* शिखरम्_Ful_Funcp = nullptr;
            AstCFunc* शिखरम्_Chg_Funcp = nullptr;
            AstCFunc* subFulFuncp = nullptr;
            AstCFunc* subChgFuncp = nullptr;
            uint32_t sub_Func_अङ्कः = 0;
            पूर्णाङ्क subStmts = 0;
            ध्रुव uint32_t maxCodes = std::max((nAllCodes + parallelism - 1) / parallelism, 1U);
            uint32_t nCodes = 0;
            ध्रुव ActCodeSet* prevActSet = nullptr;
            AstIf* ifp = nullptr;
            uint32_t baseCode = 0;
            अत्थाय (; nCodes < maxCodes && it != traces.end(); ++it) {
                ध्रुव ActCodeSet& actSet = it->first;
                // Traced value never changes, no need to add it
                सचे (actSet.गणनम्(TraceActivityVertex::ACTIVITY_NEVER)) अनुवर्त;

                ध्रुव TraceTraceVertex* ध्रुव vtxp = it->second;
                AstTraceDecl* ध्रुव declp = vtxp->nodep();

                // This is a duplicate decl, no need to add it, but must set the
                // function index to the same as the canonical node.
                सचे (ध्रुव TraceTraceVertex* ध्रुव canonVtxp = vtxp->duplicatep()) {
                    declp->fidx(canonVtxp->nodep()->fidx());
                    अनुवर्त;
                }

                // Create top function if not yet created
                सचे (!शिखरम्_Ful_Funcp) {
                    ++शिखरम्_Func_अङ्कः;
                    शिखरम्_Ful_Funcp = newCFunc(VTraceType::FULL, nullptr, शिखरम्_Func_अङ्कः);
                    शिखरम्_Chg_Funcp = newCFunc(VTraceType::CHANGE, nullptr, शिखरम्_Func_अङ्कः);
                }

                // Create new sub function if required
                सचे (!subFulFuncp || subStmts > splitLimit) {
                    baseCode = declp->code();
                    subStmts = 0;
                    subFulFuncp = newCFunc(VTraceType::FULL, शिखरम्_Ful_Funcp, sub_Func_अङ्कः, baseCode);
                    subChgFuncp = newCFunc(VTraceType::CHANGE, शिखरम्_Chg_Funcp, sub_Func_अङ्कः, baseCode);
                    ++sub_Func_अङ्कः;
                    prevActSet = nullptr;
                    ifp = nullptr;
                }

                // If required, create the conditional node checking the activity flags
                सचे (!prevActSet || actSet != *prevActSet) {
                    File_पङ्क्तिः* ध्रुव flp = m_शिखरम्_Scopep->fileline();
                    ध्रुव तार्किक always = actSet.गणनम्(TraceActivityVertex::ACTIVITY_ALWAYS) != 0;
                    Ast_ग्रन्थिः_व्यञ्जकम्* condp = nullptr;
                    सचे (always) {
                        condp = नूतन AstConst{flp, 1};  // Always true, will be folded later
                    } अञ्ञथा {
                        अत्थाय (ध्रुव uint32_t actCode : actSet) {
                            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव selp = selectActivity(flp, actCode, VAccess::READ);
                            condp = condp ? नूतन AstOr{flp, condp, selp} : selp;
                        }
                    }
                    ifp = नूतन AstIf{flp, condp};
                    सचे (!always) ifp->branchPred(VBranchPred::BP_UNLIKELY);
                    subChgFuncp->addStmtsp(ifp);
                    subStmts += ifp->ग्रन्थिः_गणनम्();
                    prevActSet = &actSet;
                }

                // Add TraceInc nodes
                File_पङ्क्तिः* ध्रुव flp = declp->fileline();
                सचे (declp->dtypeCallp()) {
                    DtypeFuncs funcs = createNonConstDtypeTraceFunctions(declp);
                    Ast_ग्रन्थिः_व्यञ्जकम्* argsp = nullptr;
                    argsp = Ast_ग्रन्थिः::addNext(argsp, declp->valuep()->clone_वृक्षः(मिच्छा));
                    AstCCall* ध्रुव callFullp = नूतन AstCCall{flp, funcs.fullFuncp, argsp};
                    callFullp->dtypeSetVoid();
                    callFullp->argTypes(callFullp->argTypes() + "bufp, "
                                        + std::to_string(declp->code()));
                    subFulFuncp->addStmtsp(callFullp->makeStmt());
                    argsp = nullptr;
                    argsp = Ast_ग्रन्थिः::addNext(argsp, declp->valuep()->clone_वृक्षः(मिच्छा));
                    AstCCall* ध्रुव callChgp = नूतन AstCCall{flp, funcs.chgFuncp, argsp};
                    callChgp->dtypeSetVoid();
                    callChgp->argTypes(callChgp->argTypes() + "bufp, "
                                       + std::to_string(declp->code()));
                    ifp->addThensp(callChgp->makeStmt());

                    subStmts += 2;
                } अञ्ञथा {
                    AstTraceInc* ध्रुव incFulp = नूतन AstTraceInc{flp, declp, VTraceType::FULL};
                    subFulFuncp->addStmtsp(incFulp);
                    AstTraceInc* ध्रुव incChgp
                        = नूतन AstTraceInc{flp, declp, VTraceType::CHANGE, baseCode};
                    ifp->addThensp(incChgp);

                    // Set the function index of the decl
                    declp->fidx(शिखरम्_Func_अङ्कः);

                    // Track splitting due to size
                    UASSERT_OBJ(incFulp->ग्रन्थिः_गणनम्() == incChgp->ग्रन्थिः_गणनम्(), declp,
                                "Should have equal cost");
                    ध्रुव VNumRange range = declp->arrayRange();
                    सचे (range.ranged()) {
                        // 2x because each element is a TraceInc and a VarRef
                        subStmts += range.elements() * 2;
                    } अञ्ञथा {
                        subStmts += incChgp->ग्रन्थिः_गणनम्();
                    }
                }

                // Track partitioning
                nCodes += declp->codeInc();
            }
        }
    }

    शून्य createCleanupFunction() {
        File_पङ्क्तिः* ध्रुव fl = m_शिखरम्_Scopep->fileline();
        AstCFunc* ध्रुव cleanupFuncp = नूतन AstCFunc{fl, "trace_cleanup", m_शिखरम्_Scopep};
        cleanupFuncp->argTypes("void* voidSelf, " + v3Global.opt.traceClassBase()
                               + "* /*unused*/");
        cleanupFuncp->isTrace(सच्च);
        cleanupFuncp->slow(मिच्छा);
        cleanupFuncp->isStatic(सच्च);
        cleanupFuncp->isLoose(सच्च);
        m_शिखरम्_Scopep->addBlocksp(cleanupFuncp);
        cleanupFuncp->addStmtsp(नूतन AstCStmt{fl, EmitCUtil::voidSelfAssign(m_शिखरम्_Modp)});
        cleanupFuncp->addStmtsp(नूतन AstCStmt{fl, EmitCUtil::symClassAssign()});

        // Register it
        {
            AstCStmt* ध्रुव cstmtp = नूतन AstCStmt{fl};
            m_regFuncp->addStmtsp(cstmtp);
            cstmtp->add("tracep->addCleanupCb(");
            cstmtp->add(नूतन AstAddrOfCFunc{fl, cleanupFuncp});
            cstmtp->add(", vlSelf);");
        }

        // Clear global activity flag
        cleanupFuncp->addStmtsp(
            नूतन AstCStmt{m_शिखरम्_Scopep->fileline(), "vlSymsp->__Vm_activity = false;"s});

        // Clear fine grained activity flags
        अत्थाय (uint32_t i = 0; i < m_activity_सङ्ख्या; ++i) {
            Ast_ग्रन्थिः* ध्रुव clrp = नूतन AstAssign{fl, selectActivity(fl, i, VAccess::WRITE),
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
        सचे (dumpGraphLevel() >= 6) m_graph.dumpDotFilePrefixed("trace_pre");
        graphSimplify(सच्च);
        सचे (dumpGraphLevel() >= 6) m_graph.dumpDotFilePrefixed("trace_simplified");
        graphOptimize();
        सचे (dumpGraphLevel() >= 6) m_graph.dumpDotFilePrefixed("trace_optimized");

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
        m_regFuncp = नूतन AstCFunc{m_शिखरम्_Scopep->fileline(), "trace_register", m_शिखरम्_Scopep};
        m_regFuncp->argTypes(v3Global.opt.traceClassBase() + "* tracep");
        m_regFuncp->entryPoint(सच्च);
        m_regFuncp->isTrace(सच्च);
        m_regFuncp->slow(सच्च);
        m_regFuncp->isStatic(मिच्छा);
        m_regFuncp->isLoose(सच्च);
        m_शिखरम्_Scopep->addBlocksp(m_regFuncp);

        // Create the const dump functions. Also allocates trace codes.
        createConstTraceFunctions(traces);

        // Create the full and incremental dump functions
        createNonConstTraceFunctions(traces, nNonConstCodes, m_parallelism);

        // Root-traced libraries alias wrapper IOs onto the existing top-module codes.
        सचे (!v3Global.opt.libCreate().empty()) {
            std::vector<AstTraceDecl*> मूलम्_Declps;
            std::vector<AstTraceDecl*> शिखरम्_Declps;
            TraceInitDeclCollector मूलम्_Collector{मूलम्_Declps};
            TraceInitDeclCollector शिखरम्_Collector{शिखरम्_Declps};
            अत्थाय (Ast_ग्रन्थिः* blockp = m_शिखरम्_Scopep->blocksp(); blockp; blockp = blockp->nextp()) {
                AstCFunc* ध्रुव funcp = VN_CAST(blockp, CFunc);
                सचे (funcp && VString::startsWith(funcp->name(), "trace_init_leaf_root__")) {
                    मूलम्_Collector.collect(funcp);
                } अञ्ञथा सचे (funcp && VString::startsWith(funcp->name(), "trace_init_leaf_top__")) {
                    शिखरम्_Collector.collect(funcp);
                }
            }

            std::vector<तार्किक> used(शिखरम्_Declps.size(), मिच्छा);
            अत्थाय (AstTraceDecl* ध्रुव मूलम्_Declp : मूलम्_Declps) {
                अत्थाय (size_t i = 0; i < शिखरम्_Declps.size(); ++i) {
                    सचे (used[i] || !same_मूलम्_Init_Alias(मूलम्_Declp, शिखरम्_Declps[i])) अनुवर्त;
                    मूलम्_Declp->code(शिखरम्_Declps[i]->code());
                    used[i] = सच्च;
                    भङ्ग;
                }
            }
        }

        // Remove refs to traced values from TraceDecl nodes, these have now moved under
        // TraceInc
        अत्थाय (ध्रुव auto& i : traces) {
            Ast_ग्रन्थिः* ध्रुव valuep = i.second->nodep()->valuep();
            VL_DO_DANGLING(valuep->unlinkFrBack()->delete_वृक्षः(), valuep);
        }

        // Create the trace cleanup function clearing the activity flags
        createCleanupFunction();
    }

    TraceCFuncVertex* getCFuncVertexp(AstCFunc* nodep) {
        V3GraphVertex* ध्रुव vtxp = nodep->user1u().toGraphVertex();
        TraceCFuncVertex* vertexp = vtxp ? vtxp->cast<TraceCFuncVertex>() : nullptr;
        सचे (!vertexp) {
            vertexp = नूतन TraceCFuncVertex{&m_graph, nodep};
            nodep->user1p(vertexp);
        }
        पटिनिवत्त vertexp;
    }
    TraceActivityVertex* getActivityVertexp(Ast_ग्रन्थिः* nodep, तार्किक slow) {
        V3GraphVertex* ध्रुव vtxp = nodep->user3u().toGraphVertex();
        TraceActivityVertex* vertexp = vtxp ? vtxp->cast<TraceActivityVertex>() : nullptr;
        सचे (!vertexp) {
            vertexp = नूतन TraceActivityVertex{&m_graph, nodep, slow};
            nodep->user3p(vertexp);
        }
        vertexp->slow(slow);
        पटिनिवत्त vertexp;
    }

    // VISITORS
    शून्य visit(AstNetlist* nodep) override {
        // Add vertexes for all TraceDecl, and edges from VARs each trace looks at
        m_finding = मिच्छा;
        iterateChildren(nodep);

        // Add vertexes for all CFUNCs, and edges to VARs the func sets
        m_finding = सच्च;
        iterateChildren(nodep);

        // Create the trace functions and insert them into the tree
        createTraceFunctions();

        // Save number of trace codes used
        nodep->nTraceCodes(m_code);
    }
    शून्य visit(Ast_ग्रन्थिः_Module* nodep) override {
        सचे (nodep->is_शिखरम्()) m_शिखरम्_Modp = nodep;
        iterateChildren(nodep);
    }
    शून्य visit(AstVarScope* nodep) override {
        सचे (!m_finding) {
            सचे (ध्रुव AstIface* ध्रुव ifacep = nodep->varp()->sensIfacep()) {
                m_ifaceMemberVscps[{ifacep, nodep->varp()->name()}].प्रक्षिप_back(nodep);
            }
        }
    }
    शून्य visit(Ast_Stmt_व्यञ्जकम्* nodep) override {
        सचे (!m_finding && !nodep->user2()) {
            सचे (AstCCall* ध्रुव callp = VN_CAST(nodep->exprp(), CCall)) {
                UINFO(8, "   CCALL " << callp);
                // See if there are other calls in same statement list;
                // If so, all funcs might share the same activity code
                TraceActivityVertex* ध्रुव activityVtxp
                    = getActivityVertexp(nodep, callp->funcp()->slow());
                अत्थाय (Ast_ग्रन्थिः* nextp = nodep; nextp; nextp = nextp->nextp()) {
                    सचे (Ast_Stmt_व्यञ्जकम्* ध्रुव stmtp = VN_CAST(nextp, Stmt_व्यञ्जकम्)) {
                        सचे (AstCCall* ध्रुव ccallp = VN_CAST(stmtp->exprp(), CCall)) {
                            stmtp->user2(सच्च);  // Processed
                            UINFO(8, "     SubCCALL " << ccallp);
                            V3GraphVertex* ध्रुव ccallFuncVtxp = getCFuncVertexp(ccallp->funcp());
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
        V3GraphVertex* ध्रुव funcVtxp = getCFuncVertexp(nodep);
        सचे (!m_finding) {  // If public, we need a unique activity code to allow for sets
                           // directly in this func
            सचे (nodep->funcPublic() || nodep->dpiExportImpl() || nodep->entryPoint()
                || nodep->isCoroutine()) {
                // Cannot treat a coroutine as slow, it may be resumed later
                ध्रुव तार्किक slow = nodep->slow() && !nodep->isCoroutine();
                TraceActivityVertex* ध्रुव activityVtxp = getActivityVertexp(nodep, slow);
                नूतन V3GraphEdge{&m_graph, activityVtxp, funcVtxp, 1};
            }
        }
        VL_RESTORER(m_cfuncp);
        m_cfuncp = nodep;
        iterateChildren(nodep);
    }
    शून्य visit(AstTraceDecl* nodep) override {
        UINFO(8, "   TRACE " << nodep);
        सचे (!m_finding && !nodep->inDtypeFunc()) {
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
            सचे (m_cfuncp && VString::startsWith(m_cfuncp->name(), "trace_init_leaf_root__")) {
                पटिनिवत्त;
            }
            V3GraphVertex* ध्रुव vertexp = नूतन TraceTraceVertex{&m_graph, nodep};
            nodep->user1p(vertexp);

            UASSERT_OBJ(m_cfuncp, nodep, "Trace not under func");
            m_cfuncp->user2(सच्च);
            VL_RESTORER(m_tracep);
            m_tracep = nodep;
            iterateChildren(nodep);
        }
    }
    शून्य visit(AstVarRef* nodep) override {
        UASSERT_OBJ(nodep->varScopep(), nodep, "No var scope?");
        AstVarScope* ध्रुव varscopep = nodep->varScopep();
        V3GraphVertex* varVtxp = varscopep->user1u().toGraphVertex();
        सचे (m_tracep) {
            UASSERT_OBJ(nodep->access().isReadOnly(), nodep, "Lvalue in trace?  Should be const.");
            सचे (!varVtxp) {
                varVtxp = नूतन TraceVarVertex{&m_graph, nodep->varScopep()};
                nodep->varScopep()->user1p(varVtxp);
            }
            V3GraphVertex* ध्रुव traceVtxp = m_tracep->user1u().toGraphVertex();
            नूतन V3GraphEdge{&m_graph, varVtxp, traceVtxp, 1};
            सचे (nodep->varp()->isPrimaryInish()  // Always need to trace primary inputs
                || nodep->varp()->isSigPublic()) {  // Or ones user can change
                नूतन V3GraphEdge{&m_graph, m_alwaysVtxp, traceVtxp, 1};
            }
            सचे (m_tracep->dtypeCallp()) varscopep->user2p(m_tracep);
        } अञ्ञथा सचे (m_cfuncp && m_finding && nodep->access().isWriteOrRW()) {
            V3GraphVertex* ध्रुव funcVtxp = getCFuncVertexp(m_cfuncp);
            सचे (varVtxp) {  // else we're not tracing this signal
                नूतन V3GraphEdge{&m_graph, funcVtxp, varVtxp, 1};
                AstTraceDecl* ध्रुव declp = VN_AS(varscopep->user2p(), TraceDecl);
                सचे (declp) {
                    V3GraphVertex* ध्रुव cFuncVtxp = getCFuncVertexp(m_cfuncp);
                    अत्थाय (ध्रुव V3GraphEdge& edge : cFuncVtxp->inEdges()) {
                        V3GraphVertex* ध्रुव activityp = edge.fromp();
                        सचे (!declp->user3p()) {
                            declp->user3p(activityp);
                        } अञ्ञथा सचे (declp->user3u().toGraphVertex() != activityp) {
                            declp->user2(सच्च);
                        }
                    }
                }
            }
        }
    }
    शून्य visit(AstMemberSel* nodep) override {
        सचे (m_cfuncp && m_finding && nodep->access().isWriteOrRW()) {
            AstIfaceRefDType* ध्रुव dtypep
                = VN_CAST(nodep->fromp()->dtypep()->skipRefp(), IfaceRefDType);
            सचे (dtypep && dtypep->isVirtual()) {
                ध्रुव auto it = m_ifaceMemberVscps.find({dtypep->ifacep(), nodep->varp()->name()});
                सचे (it != m_ifaceMemberVscps.end()) {
                    V3GraphVertex* ध्रुव funcVtxp = getCFuncVertexp(m_cfuncp);
                    अत्थाय (AstVarScope* ध्रुव vscp : it->second) {
                        V3GraphVertex* varVtxp = vscp->user1u().toGraphVertex();
                        सचे (!varVtxp) {
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
    शून्य visit(Ast_ग्रन्थिः* nodep) override { iterateChildren(nodep); }

सार्वजनिक:
    // CONSTRUCTORS
    explicit TraceVisitor(AstNetlist* nodep)
        : m_alwaysVtxp{नूतन TraceActivityVertex{&m_graph, TraceActivityVertex::ACTIVITY_ALWAYS}} {
        nodep->user2_Clear_वृक्षः();  // TraceDecl multiple activities flag
        nodep->user3_Clear_वृक्षः();  // TraceDecl TraceActivityVertex (assumes we start at nullptr)
        iterate(nodep);
        nodep->foreach([](AstTraceDecl* ध्रुव declp) {
            सचे (declp->inDtypeFunc()) {
                declp->valuep()->unlinkFrBack()->delete_वृक्षः();
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
    V3Global::dump_Check_Global_वृक्षः("trace", 0, dump_वृक्षः_Either_Level() >= 3);
}
