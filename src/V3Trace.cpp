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
    Ast_गाठ* स्थिरांक m_insertp;
    int32_t m_activityCode;
    तार्किक m_slow;  // If always slow, we can use the same code
सार्वजनिक:
    गणना { ACTIVITY_NEVER = ((1UL << 31) - 1) };
    गणना { ACTIVITY_ALWAYS = ((1UL << 31) - 2) };
    गणना { ACTIVITY_SLOW = 0 };
    TraceActivityVertex(V3Graph* graphp, Ast_गाठ* nodep, तार्किक slow)
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
    ~TraceActivityVertex() override = पूर्वनिर्धारित;
    // ACCESSORS
    Ast_गाठ* insertp() स्थिरांक {
        UASSERT(m_insertp, "Null insertp; probably called on a special always/slow");
        परत m_insertp;
    }
    string name() स्थिरांक override {
        जर (activityAlways()) {
            परत "*ALWAYS*";
        } अन्यथा {
            परत std::string{slow() ? "*SLOW* " : ""} + insertp()->name();
        }
    }
    string dotColor() स्थिरांक override { परत slow() ? "yellowGreen" : "green"; }
    int32_t activityCode() स्थिरांक { परत m_activityCode; }
    तार्किक activityAlways() स्थिरांक { परत activityCode() == ACTIVITY_ALWAYS; }
    तार्किक activitySlow() स्थिरांक { परत activityCode() == ACTIVITY_SLOW; }
    शून्य activityCode(int32_t code) { m_activityCode = code; }
    तार्किक slow() स्थिरांक { परत m_slow; }
    शून्य slow(तार्किक flag) {
        जर (!flag) m_slow = असत्य;
    }
};

वर्ग TraceCFuncVertex final : सार्वजनिक V3GraphVertex {
    VL_RTTI_IMPL(TraceCFuncVertex, V3GraphVertex)
    AstCFunc* स्थिरांक m_nodep;

सार्वजनिक:
    TraceCFuncVertex(V3Graph* graphp, AstCFunc* nodep)
        : V3GraphVertex{graphp}
        , m_nodep{nodep} {}
    ~TraceCFuncVertex() override = पूर्वनिर्धारित;
    // ACCESSORS
    AstCFunc* nodep() स्थिरांक { परत m_nodep; }
    string name() स्थिरांक override { परत nodep()->name(); }
    string dotColor() स्थिरांक override { परत "yellow"; }
    File_ओळ* fileline() स्थिरांक override { परत nodep()->fileline(); }
};

वर्ग TraceTraceVertex final : सार्वजनिक V3GraphVertex {
    VL_RTTI_IMPL(TraceTraceVertex, V3GraphVertex)
    AstTraceDecl* स्थिरांक m_nodep;  // TRACEINC this represents
    // nullptr, or other vertex with the real code() that duplicates this one
    TraceTraceVertex* m_duplicatep = nullptr;
    // When aliasing to a dtype parent, offset of the member within the dtype
    uint32_t m_dtypeAliasOffset = 0;

सार्वजनिक:
    TraceTraceVertex(V3Graph* graphp, AstTraceDecl* nodep)
        : V3GraphVertex{graphp}
        , m_nodep{nodep} {}
    ~TraceTraceVertex() override = पूर्वनिर्धारित;
    // ACCESSORS
    AstTraceDecl* nodep() स्थिरांक { परत m_nodep; }
    string name() स्थिरांक override { परत nodep()->name(); }
    string dotColor() स्थिरांक override { परत "red"; }
    File_ओळ* fileline() स्थिरांक override { परत nodep()->fileline(); }
    TraceTraceVertex* duplicatep() स्थिरांक { परत m_duplicatep; }
    शून्य duplicatep(TraceTraceVertex* dupp) {
        UASSERT_OBJ(!duplicatep(), nodep(), "Assigning duplicatep() to already duplicated node");
        m_duplicatep = dupp;
    }
    शून्य redirectDuplicatep(TraceTraceVertex* dupp) { m_duplicatep = dupp; }
    uint32_t dtypeAliasOffset() स्थिरांक { परत m_dtypeAliasOffset; }
    शून्य dtypeAliasOffset(uint32_t offset) { m_dtypeAliasOffset = offset; }
};

वर्ग TraceVarVertex final : सार्वजनिक V3GraphVertex {
    VL_RTTI_IMPL(TraceVarVertex, V3GraphVertex)
    AstVarScope* स्थिरांक m_nodep;

सार्वजनिक:
    TraceVarVertex(V3Graph* graphp, AstVarScope* nodep)
        : V3GraphVertex{graphp}
        , m_nodep{nodep} {}
    ~TraceVarVertex() override = पूर्वनिर्धारित;
    // ACCESSORS
    AstVarScope* nodep() स्थिरांक { परत m_nodep; }
    string name() स्थिरांक override { परत nodep()->name(); }
    string dotColor() स्थिरांक override { परत "skyblue"; }
    File_ओळ* fileline() स्थिरांक override { परत nodep()->fileline(); }
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
    स्थिरांक VNUser1InUse m_inuser1;
    स्थिरांक VNUser2InUse m_inuser2;
    स्थिरांक VNUser3InUse m_inuser3;
    // VNUser4InUse     In V3Hasher via V3DupFinder

    // STATE
    Ast_गाठ_Module* m_शीर्ष_Modp = nullptr;  // Module to add variables to
    AstScope* स्थिरांक m_शीर्ष_Scopep = v3Global.rootp()->शीर्ष_Scopep()->scopep();  // The top AstScope
    AstCFunc* m_cfuncp = nullptr;  // C function adding to graph
    AstCFunc* m_regFuncp = nullptr;  // Trace registration function
    AstCFunc* m_actAllFuncp = nullptr;  // Set all activity function
    AstTraceDecl* m_tracep = nullptr;  // Trace function adding to graph
    AstVarScope* m_activityVscp = nullptr;  // Activity variable
    uint32_t m_activity_संख्या = 0;  // Count of fields in activity variable
    uint32_t m_code = 0;  // Trace ident code# being assigned
    V3Graph m_graph;  // Var/CFunc tracking
    TraceActivityVertex* स्थिरांक m_alwaysVtxp;  // "Always trace" vertex
    तार्किक m_finding = असत्य;  // Pass one of algorithm?
    संरचना DtypeFuncs final {
    सार्वजनिक:
        AstCFunc* fullFuncp = nullptr;
        AstCFunc* chgFuncp = nullptr;
    };
    std::unordered_map<स्थिरांक Ast_गाठ_DType*, DtypeFuncs>
        m_dtypeNonConstFuncs;  // Full / Chg funcs per type
    std::unordered_map<स्थिरांक Ast_गाठ_DType*, AstCFunc*> m_dtypeConstFuncs;  // Const func per type
    V3UniqueNames m_dtypeNames{""};  // Unique type func names

    // Trace parallelism. Only VCD tracing can be parallelized at this time.
    स्थिरांक uint32_t m_parallelism
        = v3Global.opt.useTraceParallel() ? static_cast<uint32_t>(v3Global.opt.threads()) : 1;

    VDouble0 m_statSetters;  // Statistic tracking
    VDouble0 m_statSettersSlow;  // Statistic tracking
    VDouble0 m_statUniqSigs;  // Statistic tracking

    // All activity numbers applying to a given trace
    using ActCodeSet = std::set<uint32_t>;
    // For activity set, what traces apply
    using TraceVec = std::multimap<ActCodeSet, TraceTraceVertex*>;
    // Candidate interface-member VarScopes keyed by (interface type, member name)
    std::map<std::pair<स्थिरांक AstIface*, std::string>, std::vector<AstVarScope*>>
        m_ifaceMemberVscps;

    वर्ग TraceInitDeclCollector final : सार्वजनिक VNVisitor {
        std::vector<AstTraceDecl*>& m_declps;  // Output: trace declarations found so far
        std::set<स्थिरांक AstCFunc*> m_seenFuncps;

        शून्य visit(AstTraceDecl* nodep) override { m_declps.ढकला_back(nodep); }
        शून्य visit(AstCCall* nodep) override {
            जर (AstCFunc* स्थिरांक funcp = nodep->funcp()) collect(funcp);
        }
        शून्य visit(Ast_गाठ* nodep) override { iterateChildren(nodep); }

    सार्वजनिक:
        explicit TraceInitDeclCollector(std::vector<AstTraceDecl*>& declps)
            : m_declps{declps} {}
        शून्य collect(AstCFunc* funcp) {
            जर (funcp && m_seenFuncps.घाला(funcp).second) iterate(funcp);
        }
    };

    // METHODS

    स्थिर तार्किक same_मूळ_Init_Alias(स्थिरांक AstTraceDecl* मूळ_Declp, स्थिरांक AstTraceDecl* शीर्ष_Declp) {
        स्थिरांक VNumRange& मूळ_Bit_Range = मूळ_Declp->bitRange();
        स्थिरांक VNumRange& शीर्ष_Bit_Range = शीर्ष_Declp->bitRange();
        परत मूळ_Declp->showname() == शीर्ष_Declp->showname()
               && मूळ_Declp->declDirection() == शीर्ष_Declp->declDirection()
               && मूळ_Declp->widthMin() == शीर्ष_Declp->widthMin()
               && मूळ_Bit_Range.ranged() == शीर्ष_Bit_Range.ranged()
               && (!मूळ_Bit_Range.ranged()
                   || (मूळ_Bit_Range.डावा() == शीर्ष_Bit_Range.डावा()
                       && मूळ_Bit_Range.उजवा() == शीर्ष_Bit_Range.उजवा()));
    }

    शून्य detectDuplicates() {
        UINFO(9, "Finding duplicates");
        // Note uses user4
        V3DupFinder dupFinder;  // Duplicate code detection

        // Compute member offsets within dtype instances incrementally.
        // For each dtype member TraceDecl, stores its code offset within the
        // dtype parent's code range.
        std::unordered_map<स्थिरांक AstTraceDecl*, uint32_t> memberOffsets;
        std::unordered_map<स्थिरांक AstTraceDecl*, uint32_t> runningOffset;

        // For fixup pass: track dups redirected to dtype parents, so we can
        // fix them if the parent's user2 is later set during the loop.
        // Maps dup vertex -> original canonical member vertex.
        std::vector<std::pair<TraceTraceVertex*, TraceTraceVertex*>> dtypeParentRedirects;

        // Hash all of the traced values and find if there are any duplicates
        साठी (V3GraphVertex& vtx : m_graph.vertices()) {
            जर (TraceTraceVertex* स्थिरांक vvertexp = vtx.cast<TraceTraceVertex>()) {
                AstTraceDecl* स्थिरांक nodep = vvertexp->nodep();
                जर (nodep->dtypeCallp()) सुरूठेवा;

                जर (nodep->dtypeDeclp()) {
                    uint32_t& offset = runningOffset[nodep->dtypeDeclp()];
                    memberOffsets[nodep] = offset;
                    offset += nodep->codeInc();
                }

                UASSERT_OBJ(!vvertexp->duplicatep(), nodep, "Should not be a duplicate");
                स्थिरांक स्वतः dupit = dupFinder.findDuplicate(nodep);
                जर (dupit == dupFinder.end()) {
                    dupFinder.घाला(nodep);
                } अन्यथा {
                    AstTraceDecl* स्थिरांक dupDeclp = VN_AS(dupit->second, TraceDecl);
                    UASSERT_OBJ(dupDeclp, nodep, "Trace duplicate of wrong type");
                    TraceTraceVertex* स्थिरांक dupvertexp
                        = dupDeclp->user1u().toGraphVertex()->cast<TraceTraceVertex>();
                    UINFO(8, "  Orig " << dupDeclp << endl);
                    UINFO(8, "   dup " << nodep << endl);

                    जर (dupDeclp->dtypeDeclp() && !dupDeclp->dtypeDeclp()->user2()) {
                        AstTraceDecl* स्थिरांक dtypeParentp = dupDeclp->dtypeDeclp();
                        TraceTraceVertex* स्थिरांक dtypeVtxp
                            = dtypeParentp->user1u().toGraphVertex()->cast<TraceTraceVertex>();
                        स्थिरांक स्वतः it2 = memberOffsets.find(dupDeclp);
                        UASSERT_OBJ(it2 != memberOffsets.end(), dupDeclp,
                                    "Member offset not precomputed");
                        vvertexp->duplicatep(dtypeVtxp);
                        vvertexp->dtypeAliasOffset(it2->second);
                        dtypeParentRedirects.emplace_back(vvertexp, dupvertexp);
                    } अन्यथा {
                        vvertexp->duplicatep(dupvertexp);
                    }
                    जर (nodep->dtypeDeclp()) nodep->dtypeDeclp()->user2(सत्य);
                }
            }
        }

        साठी (स्थिरांक स्वतः& pair : dtypeParentRedirects) {
            TraceTraceVertex* स्थिरांक dupVtxp = pair.first;
            TraceTraceVertex* स्थिरांक memberVtxp = pair.second;
            स्थिरांक AstTraceDecl* स्थिरांक parentDeclp = dupVtxp->duplicatep()->nodep();
            जर (parentDeclp->user2()) {
                dupVtxp->redirectDuplicatep(memberVtxp);
                dupVtxp->dtypeAliasOffset(0);
            }
        }

        जर (dumpLevel() || debug() >= 9)
            dupFinder.dumpFile(v3Global.debugFilename("trace") + ".hash", असत्य);
    }

    शून्य graphDtypePrune() {
        साठी (V3GraphVertex* स्थिरांक vtxp : m_graph.vertices().unlinkable()) {
            जर (TraceTraceVertex* स्थिरांक vvertexp = vtxp->cast<TraceTraceVertex>()) {
                AstTraceDecl* स्थिरांक declp = vvertexp->nodep();
                // This skips the dtype sub-func optimization if a var is affected by multiple
                // activities.  We really only need to do this for _chg funcs (and not decls,
                // _const and _full) but it's simpiler to do it all one way or the other.
                जर (declp) {
                    जर (declp->user2() || (declp->dtypeDeclp() && !declp->dtypeDeclp()->user2())) {
                        AstCCall* स्थिरांक callp = declp->dtypeCallp();
                        जर (callp) {
                            Ast_गाठ* stmtexprp = callp->backp();
                            VL_DO_DANGLING(ढकला_Deletep(stmtexprp->unlinkFrBack()), stmtexprp);
                        } अन्यथा {
                            तार्किक emptyScope;
                            करा {
                                emptyScope = असत्य;
                                Ast_गाठ* स्थिरांक declBackp = declp->backp();
                                Ast_गाठ* स्थिरांक declNextp = declp->nextp();
                                जर (VN_IS(declBackp, Trace_ढकला_Prefix)
                                    && VN_IS(declNextp, Trace_काढा_Prefix)) {
                                    VL_DO_DANGLING(ढकला_Deletep(declBackp->unlinkFrBack()),
                                                   declBackp);
                                    VL_DO_DANGLING(ढकला_Deletep(declNextp->unlinkFrBack()),
                                                   declNextp);
                                    emptyScope = सत्य;
                                }
                            } पर्यंत (emptyScope);
                        }
                        // Can't purge until we finish this pass
                        ढकला_Deletep(declp->unlinkFrBack());  // declp used below
                        vvertexp->rerouteEdges(&m_graph);
                        vvertexp->unlinkDelete(&m_graph);
                    }
                    declp->dtypeDeclp(nullptr);
                }
            }
        }
    }

    शून्य splitTraceDeclFuncs() {
        स्थिरांक पूर्णांक splitLimit = v3Global.opt.outputSplitCTrace() ? v3Global.opt.outputSplitCTrace()
                                                                : std::numeric_limits<पूर्णांक>::max();
        जर (splitLimit == std::numeric_limits<पूर्णांक>::max()) परत;

        साठी (Ast_गाठ* nodep = m_शीर्ष_Scopep->blocksp(); nodep; nodep = nodep->nextp()) {
            AstCFunc* स्थिरांक funcp = VN_CAST(nodep, CFunc);
            जर (!funcp || !funcp->user2()) सुरूठेवा;

            स्थिरांक string baseName = funcp->name();
            uint32_t sub_अंक = 1;
            पूर्णांक curSize = 0;
            AstCFunc* curFuncp = funcp;
            std::vector<Ast_गाठ_Stmt*> callStmts;

            स्वतः startNewSubFunc = [&]() {
                File_ओळ* स्थिरांक flp = funcp->fileline();
                स्थिरांक string newName = baseName + "_" + cvtToStr(sub_अंक++);
                AstCFunc* स्थिरांक newFuncp = नवीन AstCFunc{flp, newName, m_शीर्ष_Scopep};
                newFuncp->argTypes(v3Global.opt.traceClassBase() + "* tracep");
                newFuncp->isTrace(सत्य);
                newFuncp->isStatic(असत्य);
                newFuncp->isLoose(सत्य);
                newFuncp->slow(सत्य);
                newFuncp->dontCombine(सत्य);
                m_शीर्ष_Scopep->addBlocksp(newFuncp);
                newFuncp->addStmtsp(नवीन AstCStmt{flp, "const int c = vlSymsp->__Vm_baseCode;"});
                AstCCall* स्थिरांक callp = नवीन AstCCall{flp, newFuncp};
                callp->dtypeSetVoid();
                callp->argTypes("tracep");
                callStmts.ढकला_back(callp->makeStmt());
                curFuncp = newFuncp;
                curSize = 0;
            };

            साठी (Ast_गाठ *stmtp = funcp->stmtsp(), *nextp; stmtp; stmtp = nextp) {
                nextp = stmtp->nextp();
                curSize += stmtp->गाठ_मोजणी();

                जर (curFuncp != funcp) {
                    stmtp->unlinkFrBack();
                    curFuncp->addStmtsp(stmtp);
                }

                जर (curSize > splitLimit && nextp) startNewSubFunc();
            }

            साठी (Ast_गाठ_Stmt* स्थिरांक callStmtp : callStmts) { funcp->addStmtsp(callStmtp); }
        }
    }

    शून्य graphSimplify(तार्किक initial) {
        जर (initial) {
            // Remove all variable nodes
            साठी (V3GraphVertex* स्थिरांक vtxp : m_graph.vertices().unlinkable()) {
                जर (TraceVarVertex* स्थिरांक vvertexp = vtxp->cast<TraceVarVertex>()) {
                    vvertexp->rerouteEdges(&m_graph);
                    vvertexp->unlinkDelete(&m_graph);
                }
            }
            // Remove multiple variables connecting funcs to traces
            // We do this twice, as then we have fewer edges to multiply out in the below
            // expansion.
            m_graph.removeRedundantEdgesMax(&V3GraphEdge::followAlwaysTrue);
            // Remove all Cfunc nodes
            साठी (V3GraphVertex* स्थिरांक vtxp : m_graph.vertices().unlinkable()) {
                जर (TraceCFuncVertex* स्थिरांक vvertexp = vtxp->cast<TraceCFuncVertex>()) {
                    vvertexp->rerouteEdges(&m_graph);
                    vvertexp->unlinkDelete(&m_graph);
                }
            }
        }

        // Remove multiple variables connecting funcs to traces
        m_graph.removeRedundantEdgesMax(&V3GraphEdge::followAlwaysTrue);

        // If there are any edges from a always, keep only the always
        साठी (V3GraphVertex& vtx : m_graph.vertices()) {
            जर (TraceTraceVertex* स्थिरांक vvertexp = vtx.cast<TraceTraceVertex>()) {
                // Search for the incoming always edge
                स्थिरांक V3GraphEdge* alwaysEdgep = nullptr;
                साठी (स्थिरांक V3GraphEdge& edge : vvertexp->inEdges()) {
                    स्थिरांक TraceActivityVertex* स्थिरांक actVtxp
                        = edge.fromp()->as<स्थिरांक TraceActivityVertex>();
                    जर (actVtxp->activityAlways()) {
                        alwaysEdgep = &edge;
                        थांबा;
                    }
                }
                // If always edge exists, remove all other edges
                जर (alwaysEdgep) {
                    साठी (V3GraphEdge* स्थिरांक edgep : vvertexp->inEdges().unlinkable()) {
                        जर (edgep != alwaysEdgep) VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
                    }
                }
            }
        }

        // Activity points with no outputs can be removed
        साठी (V3GraphVertex* स्थिरांक vtxp : m_graph.vertices().unlinkable()) {
            जर (TraceActivityVertex* स्थिरांक aVtxp = vtxp->cast<TraceActivityVertex>()) {
                // Leave in the always vertex for later use.
                जर (aVtxp != m_alwaysVtxp && aVtxp->outEmpty()) {
                    VL_DO_DANGLING(aVtxp->unlinkDelete(&m_graph), aVtxp);
                }
            }
        }
    }

    uint32_t assignactivityNumbers() {
        uint32_t activity_संख्या = 1;  // Note 0 indicates "slow" only
        साठी (V3GraphVertex& vtx : m_graph.vertices()) {
            जर (TraceActivityVertex* स्थिरांक vvertexp = vtx.cast<TraceActivityVertex>()) {
                जर (vvertexp != m_alwaysVtxp) {
                    जर (vvertexp->slow()) {
                        vvertexp->activityCode(TraceActivityVertex::ACTIVITY_SLOW);
                    } अन्यथा {
                        vvertexp->activityCode(activity_संख्या++);
                    }
                }
            }
        }
        परत activity_संख्या;
    }

    शून्य sortTraces(TraceVec& traces, uint32_t& nNonConstCodes) {
        // Populate sort structure
        traces.clear();
        nNonConstCodes = 0;
        साठी (V3GraphVertex& vtx : m_graph.vertices()) {
            जर (TraceTraceVertex* स्थिरांक vtxp = vtx.cast<TraceTraceVertex>()) {
                ActCodeSet actSet;
                UINFO(9, "  Add to sort: " << vtxp);
                UINFOTREE(9, vtxp->nodep(), "", "trnode");
                साठी (स्थिरांक V3GraphEdge& edge : vtxp->inEdges()) {
                    स्थिरांक TraceActivityVertex* स्थिरांक cfvertexp
                        = edge.fromp()->cast<स्थिरांक TraceActivityVertex>();
                    UASSERT_OBJ(cfvertexp, vtxp->nodep(),
                                "Should have been function pointing to this trace");
                    UINFO(9, "   Activity: " << cfvertexp);
                    जर (cfvertexp->activityAlways()) {
                        // If code 0, we always trace; ignore other codes
                        actSet.घाला(TraceActivityVertex::ACTIVITY_ALWAYS);
                    } अन्यथा {
                        actSet.घाला(cfvertexp->activityCode());
                    }
                }
                UASSERT_OBJ(actSet.मोजणी(TraceActivityVertex::ACTIVITY_ALWAYS) == 0
                                || actSet.size() == 1,
                            vtxp->nodep(), "Always active trace has further triggers");
                // Count nodes
                जर (!vtxp->duplicatep() && !actSet.empty())
                    nNonConstCodes += vtxp->nodep()->codeInc();
                जर (actSet.empty()) {
                    // If a trace doesn't have activity, it's constant, and we
                    // don't need to track changes on it.
                    actSet.घाला(TraceActivityVertex::ACTIVITY_NEVER);
                } अन्यथा जर (actSet.मोजणी(TraceActivityVertex::ACTIVITY_SLOW) && actSet.size() > 1) {
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
        स्थिरांक स्वतः end = traces.end();
        पर्यंत (it != end) {
            स्वतः head = it;
            // Approximate the complexity of the value change check
            uint32_t complexity = 0;
            स्थिरांक ActCodeSet& actSet = it->first;
            साठी (; it != end && it->first == actSet; ++it) {
                जर (!it->second->duplicatep()) {
                    uint32_t cost = 0;
                    स्थिरांक AstTraceDecl* स्थिरांक declp = it->second->nodep();
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
            जर (actSet.मोजणी(TraceActivityVertex::ACTIVITY_ALWAYS)) सुरूठेवा;
            जर (actSet.मोजणी(TraceActivityVertex::ACTIVITY_NEVER)) सुरूठेवा;
            जर (actSet.मोजणी(TraceActivityVertex::ACTIVITY_SLOW)) सुरूठेवा;
            // If the value comparisons are cheaper to perform than checking the
            // activity flags make the signals always traced. Note this cost
            // equation is heuristic.
            जर (complexity <= actSet.size() * 2) {
                साठी (; head != it; ++head) {
                    नवीन V3GraphEdge{&m_graph, m_alwaysVtxp, head->second, 1};
                }
            }
        }

        graphSimplify(असत्य);
    }

    Ast_गाठ_अभिव्यक्ती* selectActivity(File_ओळ* flp, uint32_t acode, स्थिरांक VAccess& access) {
        परत नवीन AstArraySel(flp, नवीन AstVarRef{flp, m_activityVscp, access}, acode);
    }

    Ast_गाठ* newActivitySetter(Ast_गाठ* insertp, uint32_t code) {
        ++m_statSetters;
        File_ओळ* स्थिरांक fl = insertp->fileline();
        AstAssign* स्थिरांक setterp = नवीन AstAssign{fl, selectActivity(fl, code, VAccess::WRITE),
                                                 नवीन AstConst{fl, AstConst::BitTrue{}}};
        परत setterp;
    }

    Ast_गाठ* newActivityAll(Ast_गाठ* insertp) {
        ++m_statSettersSlow;
        जर (!m_actAllFuncp) {
            File_ओळ* स्थिरांक flp = m_शीर्ष_Scopep->fileline();
            AstCFunc* स्थिरांक funcp = नवीन AstCFunc{flp, "__VtraceActivitySetAll", m_शीर्ष_Scopep};
            funcp->slow(सत्य);
            funcp->isStatic(असत्य);
            funcp->isLoose(सत्य);
            m_शीर्ष_Scopep->addBlocksp(funcp);
            साठी (uint32_t code = 0; code < m_activity_संख्या; ++code) {
                Ast_गाठ* स्थिरांक setterp = newActivitySetter(insertp, code);
                funcp->addStmtsp(setterp);
            }
            m_actAllFuncp = funcp;
        }
        AstCCall* स्थिरांक callp = नवीन AstCCall{insertp->fileline(), m_actAllFuncp};
        callp->dtypeSetVoid();
        परत callp->makeStmt();
    }

    शून्य createActivityFlags() {
        // Assign final activity numbers
        m_activity_संख्या = assignactivityNumbers();

        // Create an array of bytes, not a bit vector, as they can be set
        // atomically by mtasks, and are cheaper to set (no need for
        // read-modify-write on the C type), and the speed of the tracing code
        // is the same on largish designs.
        File_ओळ* स्थिरांक flp = m_शीर्ष_Scopep->fileline();
        Ast_गाठ_DType* स्थिरांक newScalarDtp = नवीन AstBasicDType{flp, VFlagBitPacked{}, 1};
        v3Global.rootp()->typeTablep()->addTypesp(newScalarDtp);
        AstRange* स्थिरांक newArange
            = नवीन AstRange{flp, VNumRange{static_cast<पूर्णांक>(m_activity_संख्या) - 1, 0}};
        Ast_गाठ_DType* स्थिरांक newArrDtp = नवीन AstUnpackArrayDType{flp, newScalarDtp, newArange};
        v3Global.rootp()->typeTablep()->addTypesp(newArrDtp);
        m_activityVscp = v3Global.rootp()->शीर्ष_Scopep()->createTemp("__VtraceActivity", newArrDtp);

        // Insert activity setters
        साठी (स्थिरांक V3GraphVertex& vtx : m_graph.vertices()) {
            जर (स्थिरांक TraceActivityVertex* स्थिरांक vtxp = vtx.cast<स्थिरांक TraceActivityVertex>()) {
                Ast_गाठ* setterp = nullptr;
                जर (vtxp->activitySlow()) {
                    setterp = newActivityAll(vtxp->insertp());
                    // Just set all flags in slow code as it should be rare.
                    // This will be rolled up into a loop by V3Reloop.
                } अन्यथा जर (!vtxp->activityAlways()) {
                    setterp = newActivitySetter(vtxp->insertp(), vtxp->activityCode());
                }
                जर (setterp) {
                    Ast_गाठ* स्थिरांक insertp = vtxp->insertp();
                    जर (Ast_Stmt_अभिव्यक्ती* स्थिरांक stmtp = VN_CAST(insertp, Stmt_अभिव्यक्ती)) {
                        stmtp->addNextHere(setterp);
                    } अन्यथा जर (AstCFunc* स्थिरांक funcp = VN_CAST(insertp, CFunc)) {
                        // If there are awaits, insert the setter after each await
                        जर (funcp->isCoroutine() && funcp->stmtsp()) {
                            funcp->stmtsp()->foreachAndNext([setterp](AstCAwait* awaitp) {
                                awaitp->addNextHere(setterp->clone_वृक्ष(असत्य));
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
                       uint32_t baseCode = 0, स्थिरांक AstTraceDecl* स्थिरांक declp = nullptr,
                       तार्किक declSub = असत्य) {
        // Create new function
        स्थिरांक तार्किक is_शीर्ष_Func = !declp && शीर्ष_Funcp == nullptr;
        std::string funcName;
        जर (is_शीर्ष_Func || declp) {
            funcName = traceType.func_prefix();
        } अन्यथा {
            funcName = शीर्ष_Funcp->name();
            funcName += "_sub";
        }
        जर (declp) {
            funcName += "_dtype";
            जर (declSub) funcName += "_sub";
            funcName += "__";
            funcName = m_dtypeNames.get(funcName);
        } अन्यथा {
            funcName += "_";
            funcName += cvtToStr(func_अंक);
        }

        File_ओळ* स्थिरांक flp = m_शीर्ष_Scopep->fileline();
        AstCFunc* स्थिरांक funcp = नवीन AstCFunc{flp, funcName, m_शीर्ष_Scopep};
        funcp->isTrace(सत्य);
        funcp->dontCombine(सत्य);
        funcp->isLoose(सत्य);
        funcp->slow(traceType != VTraceType::CHANGE);
        funcp->isStatic(is_शीर्ष_Func);
        // Add it to top scope
        m_शीर्ष_Scopep->addBlocksp(funcp);
        स्थिरांक std::string bufArg
            = v3Global.opt.traceClassBase() + "::Buffer* bufp"
              + (declp ? (", uint32_t offset, const " + declp->dtypep()->cType("", सत्य, सत्य)
                          + " __VdtypeVar")
                       : "");
        जर (is_शीर्ष_Func) {
            // Top functions
            funcp->argTypes("void* voidSelf, " + bufArg);
            funcp->addStmtsp(नवीन AstCStmt{flp, EmitCUtil::voidSelfAssign(m_शीर्ष_Modp)});
            funcp->addStmtsp(नवीन AstCStmt{flp, EmitCUtil::symClassAssign()});
            // Add global activity check to change dump functions
            जर (traceType == VTraceType::CHANGE) {  //
                funcp->addStmtsp(
                    नवीन AstCStmt{flp, "if (VL_UNLIKELY(!vlSymsp->__Vm_activity)) return;"});
            }
            // Register function
            AstCStmt* स्थिरांक cstmtp = नवीन AstCStmt{flp};
            m_regFuncp->addStmtsp(cstmtp);
            जर (traceType == VTraceType::CONSTANT) {
                cstmtp->add("tracep->addConstCb(");
                cstmtp->add(नवीन AstAddrOfCFunc{flp, funcp});
                cstmtp->add(", " + std::to_string(func_अंक) + ", vlSelf);");
            } अन्यथा जर (traceType == VTraceType::FULL) {
                cstmtp->add("tracep->addFullCb(");
                cstmtp->add(नवीन AstAddrOfCFunc{flp, funcp});
                cstmtp->add(", " + std::to_string(func_अंक) + ", vlSelf);");
            } अन्यथा {
                cstmtp->add("tracep->addChgCb(");
                cstmtp->add(नवीन AstAddrOfCFunc{flp, funcp});
                cstmtp->add(", " + std::to_string(func_अंक) + ", vlSelf);");
            }
        } अन्यथा {
            // Sub functions
            funcp->argTypes(bufArg);
            // Setup base references. Note in rare occasions we can end up with an empty trace
            // sub function, hence the VL_ATTR_UNUSED attributes.
            जर (traceType != VTraceType::CHANGE) {
                // Full dump sub function
                funcp->addStmtsp(नवीन AstCStmt{flp,  //
                                              string("uint32_t* const oldp VL_ATTR_UNUSED = "
                                                     "bufp->oldp(vlSymsp->__Vm_baseCode")
                                                  + (declp ? " + offset" : "") + ");\n"});
            } अन्यथा {
                // Change dump sub function
                funcp->addStmtsp(नवीन AstCStmt{flp,  //
                                              "uint32_t* const oldp VL_ATTR_UNUSED = "
                                              "bufp->oldp(vlSymsp->__Vm_baseCode + "
                                                  + (declp ? " offset" : cvtToStr(baseCode))
                                                  + ");\n"});
            }
            जर (!declp) {
                // Add call to top function
                AstCCall* स्थिरांक callp = नवीन AstCCall{funcp->fileline(), funcp};
                callp->dtypeSetVoid();
                callp->argTypes("bufp");
                शीर्ष_Funcp->addStmtsp(callp->makeStmt());
            }
        }
        // Done
        UINFO(5, "  newCFunc " << funcp);
        परत funcp;
    }

    AstCFunc* newCDtypeSubFunc(VTraceType traceType, स्थिरांक AstTraceDecl* स्थिरांक declp,
                               AstCFunc* parentp) {
        AstCFunc* स्थिरांक funcp = newCFunc(traceType, nullptr, 0, 0, declp, सत्य);
        File_ओळ* स्थिरांक flp = m_शीर्ष_Scopep->fileline();
        AstCCall* स्थिरांक callp = नवीन AstCCall{flp, funcp};
        callp->dtypeSetVoid();
        callp->argTypes("bufp, offset, __VdtypeVar");
        parentp->addStmtsp(callp->makeStmt());
        परत funcp;
    }

    AstCFunc* createConstDtypeTraceFunctions(स्थिरांक AstTraceDecl* declp) {
        स्थिरांक Ast_गाठ_DType* स्थिरांक dtypep = declp->dtypep()->skipRefp();
        स्वतः pair = m_dtypeConstFuncs.emplace(dtypep, nullptr);
        जर (pair.second) {
            File_ओळ* स्थिरांक flp = declp->fileline();
            AstCFunc* स्थिरांक funcp = newCFunc(VTraceType::CONSTANT, nullptr, 0, 0, declp);

            तार्किक first = सत्य;
            साठी (Ast_गाठ* callStmtp = declp->dtypeCallp()->funcp()->stmtsp(); callStmtp;
                 callStmtp = callStmtp->nextp()) {
                स्थिरांक AstCCall* स्थिरांक callp
                    = VN_CAST(VN_CAST(callStmtp, Stmt_अभिव्यक्ती)->exprp(), CCall);
                तार्किक onlyOne = first && !callStmtp->nextp();
                AstCFunc* स्थिरांक subFuncp
                    = onlyOne ? funcp : newCDtypeSubFunc(VTraceType::CONSTANT, declp, funcp);
                साठी (Ast_गाठ* stmtp = callp->funcp()->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    जर (AstTraceDecl* स्थिरांक fieldDeclp = VN_CAST(stmtp, TraceDecl)) {
                        AstTraceInc* स्थिरांक incp
                            = नवीन AstTraceInc{flp, fieldDeclp, VTraceType::CONSTANT};
                        subFuncp->addStmtsp(incp);
                    }
                }
                first = असत्य;
            }

            pair.first->second = funcp;
        }

        परत pair.first->second;
    }

    शून्य createConstTraceFunctions(स्थिरांक TraceVec& traces) {
        स्थिरांक पूर्णांक splitLimit = v3Global.opt.outputSplitCTrace() ? v3Global.opt.outputSplitCTrace()
                                                                : std::numeric_limits<पूर्णांक>::max();

        AstCFunc* स्थिरांक शीर्ष_Funcp = newCFunc(VTraceType::CONSTANT, nullptr, 0);
        uint32_t sub_Func_अंक = 0;
        AstCFunc* subFuncp = nullptr;
        पूर्णांक subStmts = 0;
        std::vector<स्थिरांक TraceTraceVertex*> duplicates;
        साठी (स्वतः it = traces.cbegin(); it != traces.end(); ++it) {
            स्थिरांक TraceTraceVertex* स्थिरांक vtxp = it->second;
            AstTraceDecl* स्थिरांक declp = vtxp->nodep();
            जर (vtxp->duplicatep()) {
                duplicates.ढकला_back(vtxp);
                सुरूठेवा;
            }

            // This is a canonical trace node. Assign trace code (signal number).
            UASSERT_OBJ(!declp->codeAssigned(), declp,
                        "Canonical node should not have code assigned yet");
            declp->code(m_code);
            स्थिरांक uint32_t codeInc = declp->codeInc();
            m_code += codeInc;
            ++m_statUniqSigs;

            // If this is a const signal, add the AstTraceInc
            स्थिरांक ActCodeSet& actSet = it->first;
            जर (actSet.मोजणी(TraceActivityVertex::ACTIVITY_NEVER)) {
                // Crate new sub function if required
                जर (!subFuncp || subStmts > splitLimit) {
                    subStmts = 0;
                    subFuncp = newCFunc(VTraceType::CONSTANT, शीर्ष_Funcp, sub_Func_अंक);
                    ++sub_Func_अंक;
                }
                File_ओळ* स्थिरांक flp = declp->fileline();
                जर (declp->dtypeCallp()) {
                    AstCFunc* स्थिरांक funcp = createConstDtypeTraceFunctions(declp);
                    Ast_गाठ_अभिव्यक्ती* argsp = nullptr;
                    argsp = Ast_गाठ::addNext(argsp, declp->valuep()->clone_वृक्ष(असत्य));
                    AstCCall* स्थिरांक callp = नवीन AstCCall{flp, funcp, argsp};
                    callp->dtypeSetVoid();
                    callp->argTypes(callp->argTypes() + "bufp, " + std::to_string(declp->code()));
                    subFuncp->addStmtsp(callp->makeStmt());

                    subStmts += 1;
                } अन्यथा {
                    AstTraceInc* स्थिरांक incp = नवीन AstTraceInc{flp, declp, VTraceType::CONSTANT};
                    subFuncp->addStmtsp(incp);
                    subStmts += incp->गाठ_मोजणी();
                }
            }
        }

        // Assign codes to duplicate nodes from their canonicals
        साठी (स्थिरांक TraceTraceVertex* स्थिरांक vtxp : duplicates) {
            AstTraceDecl* स्थिरांक declp = vtxp->nodep();
            स्थिरांक TraceTraceVertex* स्थिरांक canonVtxp = vtxp->duplicatep();
            स्थिरांक AstTraceDecl* स्थिरांक canonDeclp = canonVtxp->nodep();
            UASSERT_OBJ(!canonVtxp->duplicatep(), canonDeclp, "Canonical node is a duplicate");
            UASSERT_OBJ(canonDeclp->codeAssigned(), canonDeclp,
                        "Canonical node should have code assigned already");
            declp->code(canonDeclp->code() + vtxp->dtypeAliasOffset());
        }
    }

    DtypeFuncs createNonConstDtypeTraceFunctions(स्थिरांक AstTraceDecl* declp) {
        Ast_गाठ_DType* dtypep = declp->dtypep()->skipRefp();
        स्वतः pair = m_dtypeNonConstFuncs.emplace(dtypep, DtypeFuncs{});
        जर (pair.second) {
            File_ओळ* स्थिरांक flp = declp->fileline();
            AstCFunc* स्थिरांक fullFuncp = newCFunc(VTraceType::FULL, nullptr, 0, 0, declp);
            AstCFunc* स्थिरांक chgFuncp = newCFunc(VTraceType::CHANGE, nullptr, 0, 0, declp);

            तार्किक first = सत्य;
            साठी (Ast_गाठ* callStmtp = declp->dtypeCallp()->funcp()->stmtsp(); callStmtp;
                 callStmtp = callStmtp->nextp()) {
                स्थिरांक AstCCall* स्थिरांक callp
                    = VN_CAST(VN_CAST(callStmtp, Stmt_अभिव्यक्ती)->exprp(), CCall);
                तार्किक onlyOne = first && !callStmtp->nextp();
                AstCFunc* स्थिरांक fullSubFuncp
                    = onlyOne ? fullFuncp : newCDtypeSubFunc(VTraceType::FULL, declp, fullFuncp);
                AstCFunc* स्थिरांक chgSubFuncp
                    = onlyOne ? chgFuncp : newCDtypeSubFunc(VTraceType::CHANGE, declp, chgFuncp);
                साठी (Ast_गाठ* stmtp = callp->funcp()->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    जर (AstTraceDecl* स्थिरांक fieldDeclp = VN_CAST(stmtp, TraceDecl)) {
                        AstTraceInc* स्थिरांक incFullp
                            = नवीन AstTraceInc{flp, fieldDeclp, VTraceType::FULL};
                        fullSubFuncp->addStmtsp(incFullp);
                        AstTraceInc* स्थिरांक incChgp
                            = नवीन AstTraceInc{flp, fieldDeclp, VTraceType::CHANGE};
                        chgSubFuncp->addStmtsp(incChgp);
                    }
                }
                first = असत्य;
            }

            pair.first->second = {.fullFuncp = fullFuncp, .chgFuncp = chgFuncp};
        }

        परत pair.first->second;
    }

    शून्य createNonConstTraceFunctions(स्थिरांक TraceVec& traces, uint32_t nAllCodes,
                                      uint32_t parallelism) {
        स्थिरांक पूर्णांक splitLimit = v3Global.opt.outputSplitCTrace() ? v3Global.opt.outputSplitCTrace()
                                                                : std::numeric_limits<पूर्णांक>::max();

        // pre-incremented, so starts at 0
        uint32_t शीर्ष_Func_अंक = std::numeric_limits<uint32_t>::max();
        TraceVec::const_iterator it = traces.begin();
        पर्यंत (it != traces.end()) {
            AstCFunc* शीर्ष_Ful_Funcp = nullptr;
            AstCFunc* शीर्ष_Chg_Funcp = nullptr;
            AstCFunc* subFulFuncp = nullptr;
            AstCFunc* subChgFuncp = nullptr;
            uint32_t sub_Func_अंक = 0;
            पूर्णांक subStmts = 0;
            स्थिरांक uint32_t maxCodes = std::max((nAllCodes + parallelism - 1) / parallelism, 1U);
            uint32_t nCodes = 0;
            स्थिरांक ActCodeSet* prevActSet = nullptr;
            AstIf* ifp = nullptr;
            uint32_t baseCode = 0;
            साठी (; nCodes < maxCodes && it != traces.end(); ++it) {
                स्थिरांक ActCodeSet& actSet = it->first;
                // Traced value never changes, no need to add it
                जर (actSet.मोजणी(TraceActivityVertex::ACTIVITY_NEVER)) सुरूठेवा;

                स्थिरांक TraceTraceVertex* स्थिरांक vtxp = it->second;
                AstTraceDecl* स्थिरांक declp = vtxp->nodep();

                // This is a duplicate decl, no need to add it, but must set the
                // function index to the same as the canonical node.
                जर (स्थिरांक TraceTraceVertex* स्थिरांक canonVtxp = vtxp->duplicatep()) {
                    declp->fidx(canonVtxp->nodep()->fidx());
                    सुरूठेवा;
                }

                // Create top function if not yet created
                जर (!शीर्ष_Ful_Funcp) {
                    ++शीर्ष_Func_अंक;
                    शीर्ष_Ful_Funcp = newCFunc(VTraceType::FULL, nullptr, शीर्ष_Func_अंक);
                    शीर्ष_Chg_Funcp = newCFunc(VTraceType::CHANGE, nullptr, शीर्ष_Func_अंक);
                }

                // Create new sub function if required
                जर (!subFulFuncp || subStmts > splitLimit) {
                    baseCode = declp->code();
                    subStmts = 0;
                    subFulFuncp = newCFunc(VTraceType::FULL, शीर्ष_Ful_Funcp, sub_Func_अंक, baseCode);
                    subChgFuncp = newCFunc(VTraceType::CHANGE, शीर्ष_Chg_Funcp, sub_Func_अंक, baseCode);
                    ++sub_Func_अंक;
                    prevActSet = nullptr;
                    ifp = nullptr;
                }

                // If required, create the conditional node checking the activity flags
                जर (!prevActSet || actSet != *prevActSet) {
                    File_ओळ* स्थिरांक flp = m_शीर्ष_Scopep->fileline();
                    स्थिरांक तार्किक always = actSet.मोजणी(TraceActivityVertex::ACTIVITY_ALWAYS) != 0;
                    Ast_गाठ_अभिव्यक्ती* condp = nullptr;
                    जर (always) {
                        condp = नवीन AstConst{flp, 1};  // Always true, will be folded later
                    } अन्यथा {
                        साठी (स्थिरांक uint32_t actCode : actSet) {
                            Ast_गाठ_अभिव्यक्ती* स्थिरांक selp = selectActivity(flp, actCode, VAccess::READ);
                            condp = condp ? नवीन AstOr{flp, condp, selp} : selp;
                        }
                    }
                    ifp = नवीन AstIf{flp, condp};
                    जर (!always) ifp->branchPred(VBranchPred::BP_UNLIKELY);
                    subChgFuncp->addStmtsp(ifp);
                    subStmts += ifp->गाठ_मोजणी();
                    prevActSet = &actSet;
                }

                // Add TraceInc nodes
                File_ओळ* स्थिरांक flp = declp->fileline();
                जर (declp->dtypeCallp()) {
                    DtypeFuncs funcs = createNonConstDtypeTraceFunctions(declp);
                    Ast_गाठ_अभिव्यक्ती* argsp = nullptr;
                    argsp = Ast_गाठ::addNext(argsp, declp->valuep()->clone_वृक्ष(असत्य));
                    AstCCall* स्थिरांक callFullp = नवीन AstCCall{flp, funcs.fullFuncp, argsp};
                    callFullp->dtypeSetVoid();
                    callFullp->argTypes(callFullp->argTypes() + "bufp, "
                                        + std::to_string(declp->code()));
                    subFulFuncp->addStmtsp(callFullp->makeStmt());
                    argsp = nullptr;
                    argsp = Ast_गाठ::addNext(argsp, declp->valuep()->clone_वृक्ष(असत्य));
                    AstCCall* स्थिरांक callChgp = नवीन AstCCall{flp, funcs.chgFuncp, argsp};
                    callChgp->dtypeSetVoid();
                    callChgp->argTypes(callChgp->argTypes() + "bufp, "
                                       + std::to_string(declp->code()));
                    ifp->addThensp(callChgp->makeStmt());

                    subStmts += 2;
                } अन्यथा {
                    AstTraceInc* स्थिरांक incFulp = नवीन AstTraceInc{flp, declp, VTraceType::FULL};
                    subFulFuncp->addStmtsp(incFulp);
                    AstTraceInc* स्थिरांक incChgp
                        = नवीन AstTraceInc{flp, declp, VTraceType::CHANGE, baseCode};
                    ifp->addThensp(incChgp);

                    // Set the function index of the decl
                    declp->fidx(शीर्ष_Func_अंक);

                    // Track splitting due to size
                    UASSERT_OBJ(incFulp->गाठ_मोजणी() == incChgp->गाठ_मोजणी(), declp,
                                "Should have equal cost");
                    स्थिरांक VNumRange range = declp->arrayRange();
                    जर (range.ranged()) {
                        // 2x because each element is a TraceInc and a VarRef
                        subStmts += range.elements() * 2;
                    } अन्यथा {
                        subStmts += incChgp->गाठ_मोजणी();
                    }
                }

                // Track partitioning
                nCodes += declp->codeInc();
            }
        }
    }

    शून्य createCleanupFunction() {
        File_ओळ* स्थिरांक fl = m_शीर्ष_Scopep->fileline();
        AstCFunc* स्थिरांक cleanupFuncp = नवीन AstCFunc{fl, "trace_cleanup", m_शीर्ष_Scopep};
        cleanupFuncp->argTypes("void* voidSelf, " + v3Global.opt.traceClassBase()
                               + "* /*unused*/");
        cleanupFuncp->isTrace(सत्य);
        cleanupFuncp->slow(असत्य);
        cleanupFuncp->isStatic(सत्य);
        cleanupFuncp->isLoose(सत्य);
        m_शीर्ष_Scopep->addBlocksp(cleanupFuncp);
        cleanupFuncp->addStmtsp(नवीन AstCStmt{fl, EmitCUtil::voidSelfAssign(m_शीर्ष_Modp)});
        cleanupFuncp->addStmtsp(नवीन AstCStmt{fl, EmitCUtil::symClassAssign()});

        // Register it
        {
            AstCStmt* स्थिरांक cstmtp = नवीन AstCStmt{fl};
            m_regFuncp->addStmtsp(cstmtp);
            cstmtp->add("tracep->addCleanupCb(");
            cstmtp->add(नवीन AstAddrOfCFunc{fl, cleanupFuncp});
            cstmtp->add(", vlSelf);");
        }

        // Clear global activity flag
        cleanupFuncp->addStmtsp(
            नवीन AstCStmt{m_शीर्ष_Scopep->fileline(), "vlSymsp->__Vm_activity = false;"s});

        // Clear fine grained activity flags
        साठी (uint32_t i = 0; i < m_activity_संख्या; ++i) {
            Ast_गाठ* स्थिरांक clrp = नवीन AstAssign{fl, selectActivity(fl, i, VAccess::WRITE),
                                                नवीन AstConst{fl, AstConst::BitFalse{}}};
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
        जर (dumpGraphLevel() >= 6) m_graph.dumpDotFilePrefixed("trace_pre");
        graphSimplify(सत्य);
        जर (dumpGraphLevel() >= 6) m_graph.dumpDotFilePrefixed("trace_simplified");
        graphOptimize();
        जर (dumpGraphLevel() >= 6) m_graph.dumpDotFilePrefixed("trace_optimized");

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
        m_regFuncp = नवीन AstCFunc{m_शीर्ष_Scopep->fileline(), "trace_register", m_शीर्ष_Scopep};
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
        जर (!v3Global.opt.libCreate().empty()) {
            std::vector<AstTraceDecl*> मूळ_Declps;
            std::vector<AstTraceDecl*> शीर्ष_Declps;
            TraceInitDeclCollector मूळ_Collector{मूळ_Declps};
            TraceInitDeclCollector शीर्ष_Collector{शीर्ष_Declps};
            साठी (Ast_गाठ* blockp = m_शीर्ष_Scopep->blocksp(); blockp; blockp = blockp->nextp()) {
                AstCFunc* स्थिरांक funcp = VN_CAST(blockp, CFunc);
                जर (funcp && VString::startsWith(funcp->name(), "trace_init_leaf_root__")) {
                    मूळ_Collector.collect(funcp);
                } अन्यथा जर (funcp && VString::startsWith(funcp->name(), "trace_init_leaf_top__")) {
                    शीर्ष_Collector.collect(funcp);
                }
            }

            std::vector<तार्किक> used(शीर्ष_Declps.size(), असत्य);
            साठी (AstTraceDecl* स्थिरांक मूळ_Declp : मूळ_Declps) {
                साठी (माप_प्रकार i = 0; i < शीर्ष_Declps.size(); ++i) {
                    जर (used[i] || !same_मूळ_Init_Alias(मूळ_Declp, शीर्ष_Declps[i])) सुरूठेवा;
                    मूळ_Declp->code(शीर्ष_Declps[i]->code());
                    used[i] = सत्य;
                    थांबा;
                }
            }
        }

        // Remove refs to traced values from TraceDecl nodes, these have now moved under
        // TraceInc
        साठी (स्थिरांक स्वतः& i : traces) {
            Ast_गाठ* स्थिरांक valuep = i.second->nodep()->valuep();
            VL_DO_DANGLING(valuep->unlinkFrBack()->delete_वृक्ष(), valuep);
        }

        // Create the trace cleanup function clearing the activity flags
        createCleanupFunction();
    }

    TraceCFuncVertex* getCFuncVertexp(AstCFunc* nodep) {
        V3GraphVertex* स्थिरांक vtxp = nodep->user1u().toGraphVertex();
        TraceCFuncVertex* vertexp = vtxp ? vtxp->cast<TraceCFuncVertex>() : nullptr;
        जर (!vertexp) {
            vertexp = नवीन TraceCFuncVertex{&m_graph, nodep};
            nodep->user1p(vertexp);
        }
        परत vertexp;
    }
    TraceActivityVertex* getActivityVertexp(Ast_गाठ* nodep, तार्किक slow) {
        V3GraphVertex* स्थिरांक vtxp = nodep->user3u().toGraphVertex();
        TraceActivityVertex* vertexp = vtxp ? vtxp->cast<TraceActivityVertex>() : nullptr;
        जर (!vertexp) {
            vertexp = नवीन TraceActivityVertex{&m_graph, nodep, slow};
            nodep->user3p(vertexp);
        }
        vertexp->slow(slow);
        परत vertexp;
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
    शून्य visit(Ast_गाठ_Module* nodep) override {
        जर (nodep->is_शीर्ष()) m_शीर्ष_Modp = nodep;
        iterateChildren(nodep);
    }
    शून्य visit(AstVarScope* nodep) override {
        जर (!m_finding) {
            जर (स्थिरांक AstIface* स्थिरांक ifacep = nodep->varp()->sensIfacep()) {
                m_ifaceMemberVscps[{ifacep, nodep->varp()->name()}].ढकला_back(nodep);
            }
        }
    }
    शून्य visit(Ast_Stmt_अभिव्यक्ती* nodep) override {
        जर (!m_finding && !nodep->user2()) {
            जर (AstCCall* स्थिरांक callp = VN_CAST(nodep->exprp(), CCall)) {
                UINFO(8, "   CCALL " << callp);
                // See if there are other calls in same statement list;
                // If so, all funcs might share the same activity code
                TraceActivityVertex* स्थिरांक activityVtxp
                    = getActivityVertexp(nodep, callp->funcp()->slow());
                साठी (Ast_गाठ* nextp = nodep; nextp; nextp = nextp->nextp()) {
                    जर (Ast_Stmt_अभिव्यक्ती* स्थिरांक stmtp = VN_CAST(nextp, Stmt_अभिव्यक्ती)) {
                        जर (AstCCall* स्थिरांक ccallp = VN_CAST(stmtp->exprp(), CCall)) {
                            stmtp->user2(सत्य);  // Processed
                            UINFO(8, "     SubCCALL " << ccallp);
                            V3GraphVertex* स्थिरांक ccallFuncVtxp = getCFuncVertexp(ccallp->funcp());
                            activityVtxp->slow(ccallp->funcp()->slow());
                            नवीन V3GraphEdge{&m_graph, activityVtxp, ccallFuncVtxp, 1};
                        }
                    }
                }
            }
        }
        iterateChildren(nodep);
    }
    शून्य visit(AstCFunc* nodep) override {
        UINFO(8, "   CFUNC " << nodep);
        V3GraphVertex* स्थिरांक funcVtxp = getCFuncVertexp(nodep);
        जर (!m_finding) {  // If public, we need a unique activity code to allow for sets
                           // directly in this func
            जर (nodep->funcPublic() || nodep->dpiExportImpl() || nodep->entryPoint()
                || nodep->isCoroutine()) {
                // Cannot treat a coroutine as slow, it may be resumed later
                स्थिरांक तार्किक slow = nodep->slow() && !nodep->isCoroutine();
                TraceActivityVertex* स्थिरांक activityVtxp = getActivityVertexp(nodep, slow);
                नवीन V3GraphEdge{&m_graph, activityVtxp, funcVtxp, 1};
            }
        }
        VL_RESTORER(m_cfuncp);
        m_cfuncp = nodep;
        iterateChildren(nodep);
    }
    शून्य visit(AstTraceDecl* nodep) override {
        UINFO(8, "   TRACE " << nodep);
        जर (!m_finding && !nodep->inDtypeFunc()) {
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
            जर (m_cfuncp && VString::startsWith(m_cfuncp->name(), "trace_init_leaf_root__")) {
                परत;
            }
            V3GraphVertex* स्थिरांक vertexp = नवीन TraceTraceVertex{&m_graph, nodep};
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
        AstVarScope* स्थिरांक varscopep = nodep->varScopep();
        V3GraphVertex* varVtxp = varscopep->user1u().toGraphVertex();
        जर (m_tracep) {
            UASSERT_OBJ(nodep->access().isReadOnly(), nodep, "Lvalue in trace?  Should be const.");
            जर (!varVtxp) {
                varVtxp = नवीन TraceVarVertex{&m_graph, nodep->varScopep()};
                nodep->varScopep()->user1p(varVtxp);
            }
            V3GraphVertex* स्थिरांक traceVtxp = m_tracep->user1u().toGraphVertex();
            नवीन V3GraphEdge{&m_graph, varVtxp, traceVtxp, 1};
            जर (nodep->varp()->isPrimaryInish()  // Always need to trace primary inputs
                || nodep->varp()->isSigPublic()) {  // Or ones user can change
                नवीन V3GraphEdge{&m_graph, m_alwaysVtxp, traceVtxp, 1};
            }
            जर (m_tracep->dtypeCallp()) varscopep->user2p(m_tracep);
        } अन्यथा जर (m_cfuncp && m_finding && nodep->access().isWriteOrRW()) {
            V3GraphVertex* स्थिरांक funcVtxp = getCFuncVertexp(m_cfuncp);
            जर (varVtxp) {  // else we're not tracing this signal
                नवीन V3GraphEdge{&m_graph, funcVtxp, varVtxp, 1};
                AstTraceDecl* स्थिरांक declp = VN_AS(varscopep->user2p(), TraceDecl);
                जर (declp) {
                    V3GraphVertex* स्थिरांक cFuncVtxp = getCFuncVertexp(m_cfuncp);
                    साठी (स्थिरांक V3GraphEdge& edge : cFuncVtxp->inEdges()) {
                        V3GraphVertex* स्थिरांक activityp = edge.fromp();
                        जर (!declp->user3p()) {
                            declp->user3p(activityp);
                        } अन्यथा जर (declp->user3u().toGraphVertex() != activityp) {
                            declp->user2(सत्य);
                        }
                    }
                }
            }
        }
    }
    शून्य visit(AstMemberSel* nodep) override {
        जर (m_cfuncp && m_finding && nodep->access().isWriteOrRW()) {
            AstIfaceRefDType* स्थिरांक dtypep
                = VN_CAST(nodep->fromp()->dtypep()->skipRefp(), IfaceRefDType);
            जर (dtypep && dtypep->isVirtual()) {
                स्थिरांक स्वतः it = m_ifaceMemberVscps.find({dtypep->ifacep(), nodep->varp()->name()});
                जर (it != m_ifaceMemberVscps.end()) {
                    V3GraphVertex* स्थिरांक funcVtxp = getCFuncVertexp(m_cfuncp);
                    साठी (AstVarScope* स्थिरांक vscp : it->second) {
                        V3GraphVertex* varVtxp = vscp->user1u().toGraphVertex();
                        जर (!varVtxp) {
                            varVtxp = नवीन TraceVarVertex{&m_graph, vscp};
                            vscp->user1p(varVtxp);
                        }
                        नवीन V3GraphEdge{&m_graph, funcVtxp, varVtxp, 1};
                    }
                }
            }
        }
        iterateChildren(nodep);
    }
    //--------------------
    शून्य visit(Ast_गाठ* nodep) override { iterateChildren(nodep); }

सार्वजनिक:
    // CONSTRUCTORS
    explicit TraceVisitor(AstNetlist* nodep)
        : m_alwaysVtxp{नवीन TraceActivityVertex{&m_graph, TraceActivityVertex::ACTIVITY_ALWAYS}} {
        nodep->user2_Clear_वृक्ष();  // TraceDecl multiple activities flag
        nodep->user3_Clear_वृक्ष();  // TraceDecl TraceActivityVertex (assumes we start at nullptr)
        iterate(nodep);
        nodep->foreach([](AstTraceDecl* स्थिरांक declp) {
            जर (declp->inDtypeFunc()) {
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

शून्य V3Trace::traceAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { TraceVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_वृक्ष("trace", 0, dump_वृक्ष_Either_Level() >= 3);
}
