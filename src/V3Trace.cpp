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

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "V3Trace.h"

#ضمن "V3Ast.h"
#ضمن "V3DupFinder.h"
#ضمن "V3EmitCBase.h"
#ضمن "V3Graph.h"
#ضمن "V3Stats.h"
#ضمن "V3UniqueNames.h"

#ضمن <limits>
#ضمن <map>
#ضمن <set>
#ضمن <unordered_map>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Graph vertexes

صنف TraceActivityVertex final : عام V3GraphVertex {
    VL_RTTI_IMPL(TraceActivityVertex, V3GraphVertex)
    AstNode* ثابت m_insertp;
    int32_t m_activityCode;
    منطقي m_slow;  // If always slow, we can use the same code
عام:
    تعداد { ACTIVITY_NEVER = ((1UL << 31) - 1) };
    تعداد { ACTIVITY_ALWAYS = ((1UL << 31) - 2) };
    تعداد { ACTIVITY_SLOW = 0 };
    TraceActivityVertex(V3Graph* graphp, AstNode* nodep, منطقي slow)
        : V3GraphVertex{graphp}
        , m_insertp{nodep} {
        m_activityCode = 0;
        m_slow = slow;
    }
    TraceActivityVertex(V3Graph* graphp, int32_t code)
        : V3GraphVertex{graphp}
        , m_insertp{nullptr} {
        m_activityCode = code;
        m_slow = ܕܓܠܐ;
    }
    ~TraceActivityVertex() override = افتراضي;
    // ACCESSORS
    AstNode* insertp() ثابت {
        UASSERT(m_insertp, "Null insertp; probably called on a special always/slow");
        ܦܢܐ m_insertp;
    }
    string name() ثابت override {
        ܐܢ (activityAlways()) {
            ܦܢܐ "*ALWAYS*";
        } ܐܚܪܢܐ {
            ܦܢܐ std::string{slow() ? "*SLOW* " : ""} + insertp()->name();
        }
    }
    string dotColor() ثابت override { ܦܢܐ slow() ? "yellowGreen" : "green"; }
    int32_t activityCode() ثابت { ܦܢܐ m_activityCode; }
    منطقي activityAlways() ثابت { ܦܢܐ activityCode() == ACTIVITY_ALWAYS; }
    منطقي activitySlow() ثابت { ܦܢܐ activityCode() == ACTIVITY_SLOW; }
    فراغ activityCode(int32_t code) { m_activityCode = code; }
    منطقي slow() ثابت { ܦܢܐ m_slow; }
    فراغ slow(منطقي flag) {
        ܐܢ (!flag) m_slow = ܕܓܠܐ;
    }
};

صنف TraceCFuncVertex final : عام V3GraphVertex {
    VL_RTTI_IMPL(TraceCFuncVertex, V3GraphVertex)
    AstCFunc* ثابت m_nodep;

عام:
    TraceCFuncVertex(V3Graph* graphp, AstCFunc* nodep)
        : V3GraphVertex{graphp}
        , m_nodep{nodep} {}
    ~TraceCFuncVertex() override = افتراضي;
    // ACCESSORS
    AstCFunc* nodep() ثابت { ܦܢܐ m_nodep; }
    string name() ثابت override { ܦܢܐ nodep()->name(); }
    string dotColor() ثابت override { ܦܢܐ "yellow"; }
    FileLine* fileline() ثابت override { ܦܢܐ nodep()->fileline(); }
};

صنف TraceTraceVertex final : عام V3GraphVertex {
    VL_RTTI_IMPL(TraceTraceVertex, V3GraphVertex)
    AstTraceDecl* ثابت m_nodep;  // TRACEINC this represents
    // nullptr, or other vertex with the real code() that duplicates this one
    TraceTraceVertex* m_duplicatep = nullptr;
    // When aliasing to a dtype parent, offset of the member within the dtype
    uint32_t m_dtypeAliasOffset = 0;

عام:
    TraceTraceVertex(V3Graph* graphp, AstTraceDecl* nodep)
        : V3GraphVertex{graphp}
        , m_nodep{nodep} {}
    ~TraceTraceVertex() override = افتراضي;
    // ACCESSORS
    AstTraceDecl* nodep() ثابت { ܦܢܐ m_nodep; }
    string name() ثابت override { ܦܢܐ nodep()->name(); }
    string dotColor() ثابت override { ܦܢܐ "red"; }
    FileLine* fileline() ثابت override { ܦܢܐ nodep()->fileline(); }
    TraceTraceVertex* duplicatep() ثابت { ܦܢܐ m_duplicatep; }
    فراغ duplicatep(TraceTraceVertex* dupp) {
        UASSERT_OBJ(!duplicatep(), nodep(), "Assigning duplicatep() to already duplicated node");
        m_duplicatep = dupp;
    }
    فراغ redirectDuplicatep(TraceTraceVertex* dupp) { m_duplicatep = dupp; }
    uint32_t dtypeAliasOffset() ثابت { ܦܢܐ m_dtypeAliasOffset; }
    فراغ dtypeAliasOffset(uint32_t offset) { m_dtypeAliasOffset = offset; }
};

صنف TraceVarVertex final : عام V3GraphVertex {
    VL_RTTI_IMPL(TraceVarVertex, V3GraphVertex)
    AstVarScope* ثابت m_nodep;

عام:
    TraceVarVertex(V3Graph* graphp, AstVarScope* nodep)
        : V3GraphVertex{graphp}
        , m_nodep{nodep} {}
    ~TraceVarVertex() override = افتراضي;
    // ACCESSORS
    AstVarScope* nodep() ثابت { ܦܢܐ m_nodep; }
    string name() ثابت override { ܦܢܐ nodep()->name(); }
    string dotColor() ثابت override { ܦܢܐ "skyblue"; }
    FileLine* fileline() ثابت override { ܦܢܐ nodep()->fileline(); }
};

//######################################################################
// Trace state, as a visitor of each AstNode

صنف TraceVisitor final : عام VNVisitor {
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
    ثابت VNUser1InUse m_inuser1;
    ثابت VNUser2InUse m_inuser2;
    ثابت VNUser3InUse m_inuser3;
    // VNUser4InUse     In V3Hasher via V3DupFinder

    // STATE
    AstNodeModule* m_topModp = nullptr;  // Module to add variables to
    AstScope* ثابت m_topScopep = v3Global.rootp()->topScopep()->scopep();  // The top AstScope
    AstCFunc* m_cfuncp = nullptr;  // C function adding to graph
    AstCFunc* m_regFuncp = nullptr;  // Trace registration function
    AstCFunc* m_actAllFuncp = nullptr;  // Set all activity function
    AstTraceDecl* m_tracep = nullptr;  // Trace function adding to graph
    AstVarScope* m_activityVscp = nullptr;  // Activity variable
    uint32_t m_activityNumber = 0;  // Count of fields in activity variable
    uint32_t m_code = 0;  // Trace ident code# being assigned
    V3Graph m_graph;  // Var/CFunc tracking
    TraceActivityVertex* ثابت m_alwaysVtxp;  // "Always trace" vertex
    منطقي m_finding = ܕܓܠܐ;  // Pass one of algorithm?
    بنية DtypeFuncs final {
    عام:
        AstCFunc* fullFuncp = nullptr;
        AstCFunc* chgFuncp = nullptr;
    };
    std::unordered_map<ثابت AstNodeDType*, DtypeFuncs>
        m_dtypeNonConstFuncs;  // Full / Chg funcs per type
    std::unordered_map<ثابت AstNodeDType*, AstCFunc*> m_dtypeConstFuncs;  // Const func per type
    V3UniqueNames m_dtypeNames{""};  // Unique type func names

    // Trace parallelism. Only VCD tracing can be parallelized at this time.
    ثابت uint32_t m_parallelism
        = v3Global.opt.useTraceParallel() ? static_cast<uint32_t>(v3Global.opt.threads()) : 1;

    VDouble0 m_statSetters;  // Statistic tracking
    VDouble0 m_statSettersSlow;  // Statistic tracking
    VDouble0 m_statUniqSigs;  // Statistic tracking

    // All activity numbers applying to a given trace
    using ActCodeSet = std::set<uint32_t>;
    // For activity set, what traces apply
    using TraceVec = std::multimap<ActCodeSet, TraceTraceVertex*>;
    // Candidate interface-member VarScopes keyed by (interface type, member name)
    std::map<std::pair<ثابت AstIface*, std::string>, std::vector<AstVarScope*>>
        m_ifaceMemberVscps;

    صنف TraceInitDeclCollector final : عام VNVisitor {
        std::vector<AstTraceDecl*>& m_declps;  // Output: trace declarations found so far
        std::set<ثابت AstCFunc*> m_seenFuncps;

        فراغ visit(AstTraceDecl* nodep) override { m_declps.push_back(nodep); }
        فراغ visit(AstCCall* nodep) override {
            ܐܢ (AstCFunc* ثابت funcp = nodep->funcp()) collect(funcp);
        }
        فراغ visit(AstNode* nodep) override { iterateChildren(nodep); }

    عام:
        explicit TraceInitDeclCollector(std::vector<AstTraceDecl*>& declps)
            : m_declps{declps} {}
        فراغ collect(AstCFunc* funcp) {
            ܐܢ (funcp && m_seenFuncps.insert(funcp).second) iterate(funcp);
        }
    };

    // METHODS

    ساكن منطقي sameRootInitAlias(ثابت AstTraceDecl* rootDeclp, ثابت AstTraceDecl* topDeclp) {
        ثابت VNumRange& rootBitRange = rootDeclp->bitRange();
        ثابت VNumRange& topBitRange = topDeclp->bitRange();
        ܦܢܐ rootDeclp->showname() == topDeclp->showname()
               && rootDeclp->declDirection() == topDeclp->declDirection()
               && rootDeclp->widthMin() == topDeclp->widthMin()
               && rootBitRange.ranged() == topBitRange.ranged()
               && (!rootBitRange.ranged()
                   || (rootBitRange.left() == topBitRange.left()
                       && rootBitRange.right() == topBitRange.right()));
    }

    فراغ detectDuplicates() {
        UINFO(9, "Finding duplicates");
        // Note uses user4
        V3DupFinder dupFinder;  // Duplicate code detection

        // Compute member offsets within dtype instances incrementally.
        // For each dtype member TraceDecl, stores its code offset within the
        // dtype parent's code range.
        std::unordered_map<ثابت AstTraceDecl*, uint32_t> memberOffsets;
        std::unordered_map<ثابت AstTraceDecl*, uint32_t> runningOffset;

        // For fixup pass: track dups redirected to dtype parents, so we can
        // fix them if the parent's user2 is later set during the loop.
        // Maps dup vertex -> original canonical member vertex.
        std::vector<std::pair<TraceTraceVertex*, TraceTraceVertex*>> dtypeParentRedirects;

        // Hash all of the traced values and find if there are any duplicates
        لكل (V3GraphVertex& vtx : m_graph.vertices()) {
            ܐܢ (TraceTraceVertex* ثابت vvertexp = vtx.cast<TraceTraceVertex>()) {
                AstTraceDecl* ثابت nodep = vvertexp->nodep();
                ܐܢ (nodep->dtypeCallp()) استمر;

                ܐܢ (nodep->dtypeDeclp()) {
                    uint32_t& offset = runningOffset[nodep->dtypeDeclp()];
                    memberOffsets[nodep] = offset;
                    offset += nodep->codeInc();
                }

                UASSERT_OBJ(!vvertexp->duplicatep(), nodep, "Should not be a duplicate");
                ثابت auto dupit = dupFinder.findDuplicate(nodep);
                ܐܢ (dupit == dupFinder.end()) {
                    dupFinder.insert(nodep);
                } ܐܚܪܢܐ {
                    AstTraceDecl* ثابت dupDeclp = VN_AS(dupit->second, TraceDecl);
                    UASSERT_OBJ(dupDeclp, nodep, "Trace duplicate of wrong type");
                    TraceTraceVertex* ثابت dupvertexp
                        = dupDeclp->user1u().toGraphVertex()->cast<TraceTraceVertex>();
                    UINFO(8, "  Orig " << dupDeclp << endl);
                    UINFO(8, "   dup " << nodep << endl);

                    ܐܢ (dupDeclp->dtypeDeclp() && !dupDeclp->dtypeDeclp()->user2()) {
                        AstTraceDecl* ثابت dtypeParentp = dupDeclp->dtypeDeclp();
                        TraceTraceVertex* ثابت dtypeVtxp
                            = dtypeParentp->user1u().toGraphVertex()->cast<TraceTraceVertex>();
                        ثابت auto it2 = memberOffsets.find(dupDeclp);
                        UASSERT_OBJ(it2 != memberOffsets.end(), dupDeclp,
                                    "Member offset not precomputed");
                        vvertexp->duplicatep(dtypeVtxp);
                        vvertexp->dtypeAliasOffset(it2->second);
                        dtypeParentRedirects.emplace_back(vvertexp, dupvertexp);
                    } ܐܚܪܢܐ {
                        vvertexp->duplicatep(dupvertexp);
                    }
                    ܐܢ (nodep->dtypeDeclp()) nodep->dtypeDeclp()->user2(ܫܪܪܐ);
                }
            }
        }

        لكل (ثابت auto& pair : dtypeParentRedirects) {
            TraceTraceVertex* ثابت dupVtxp = pair.first;
            TraceTraceVertex* ثابت memberVtxp = pair.second;
            ثابت AstTraceDecl* ثابت parentDeclp = dupVtxp->duplicatep()->nodep();
            ܐܢ (parentDeclp->user2()) {
                dupVtxp->redirectDuplicatep(memberVtxp);
                dupVtxp->dtypeAliasOffset(0);
            }
        }

        ܐܢ (dumpLevel() || debug() >= 9)
            dupFinder.dumpFile(v3Global.debugFilename("trace") + ".hash", ܕܓܠܐ);
    }

    فراغ graphDtypePrune() {
        لكل (V3GraphVertex* ثابت vtxp : m_graph.vertices().unlinkable()) {
            ܐܢ (TraceTraceVertex* ثابت vvertexp = vtxp->cast<TraceTraceVertex>()) {
                AstTraceDecl* ثابت declp = vvertexp->nodep();
                // This skips the dtype sub-func optimization if a var is affected by multiple
                // activities.  We really only need to do this for _chg funcs (and not decls,
                // _const and _full) but it's simpiler to do it all one way or the other.
                ܐܢ (declp) {
                    ܐܢ (declp->user2() || (declp->dtypeDeclp() && !declp->dtypeDeclp()->user2())) {
                        AstCCall* ثابت callp = declp->dtypeCallp();
                        ܐܢ (callp) {
                            AstNode* stmtexprp = callp->backp();
                            VL_DO_DANGLING(pushDeletep(stmtexprp->unlinkFrBack()), stmtexprp);
                        } ܐܚܪܢܐ {
                            منطقي emptyScope;
                            ܥܒܕ {
                                emptyScope = ܕܓܠܐ;
                                AstNode* ثابت declBackp = declp->backp();
                                AstNode* ثابت declNextp = declp->nextp();
                                ܐܢ (VN_IS(declBackp, TracePushPrefix)
                                    && VN_IS(declNextp, TracePopPrefix)) {
                                    VL_DO_DANGLING(pushDeletep(declBackp->unlinkFrBack()),
                                                   declBackp);
                                    VL_DO_DANGLING(pushDeletep(declNextp->unlinkFrBack()),
                                                   declNextp);
                                    emptyScope = ܫܪܪܐ;
                                }
                            } ܟܡܐ (emptyScope);
                        }
                        // Can't purge until we finish this pass
                        pushDeletep(declp->unlinkFrBack());  // declp used below
                        vvertexp->rerouteEdges(&m_graph);
                        vvertexp->unlinkDelete(&m_graph);
                    }
                    declp->dtypeDeclp(nullptr);
                }
            }
        }
    }

    فراغ splitTraceDeclFuncs() {
        ثابت صحيح splitLimit = v3Global.opt.outputSplitCTrace() ? v3Global.opt.outputSplitCTrace()
                                                                : std::numeric_limits<صحيح>::max();
        ܐܢ (splitLimit == std::numeric_limits<صحيح>::max()) ܦܢܐ;

        لكل (AstNode* nodep = m_topScopep->blocksp(); nodep; nodep = nodep->nextp()) {
            AstCFunc* ثابت funcp = VN_CAST(nodep, CFunc);
            ܐܢ (!funcp || !funcp->user2()) استمر;

            ثابت string baseName = funcp->name();
            uint32_t subNum = 1;
            صحيح curSize = 0;
            AstCFunc* curFuncp = funcp;
            std::vector<AstNodeStmt*> callStmts;

            auto startNewSubFunc = [&]() {
                FileLine* ثابت flp = funcp->fileline();
                ثابت string newName = baseName + "_" + cvtToStr(subNum++);
                AstCFunc* ثابت newFuncp = جديد AstCFunc{flp, newName, m_topScopep};
                newFuncp->argTypes(v3Global.opt.traceClassBase() + "* tracep");
                newFuncp->isTrace(ܫܪܪܐ);
                newFuncp->isStatic(ܕܓܠܐ);
                newFuncp->isLoose(ܫܪܪܐ);
                newFuncp->slow(ܫܪܪܐ);
                newFuncp->dontCombine(ܫܪܪܐ);
                m_topScopep->addBlocksp(newFuncp);
                newFuncp->addStmtsp(جديد AstCStmt{flp, "const int c = vlSymsp->__Vm_baseCode;"});
                AstCCall* ثابت callp = جديد AstCCall{flp, newFuncp};
                callp->dtypeSetVoid();
                callp->argTypes("tracep");
                callStmts.push_back(callp->makeStmt());
                curFuncp = newFuncp;
                curSize = 0;
            };

            لكل (AstNode *stmtp = funcp->stmtsp(), *nextp; stmtp; stmtp = nextp) {
                nextp = stmtp->nextp();
                curSize += stmtp->nodeCount();

                ܐܢ (curFuncp != funcp) {
                    stmtp->unlinkFrBack();
                    curFuncp->addStmtsp(stmtp);
                }

                ܐܢ (curSize > splitLimit && nextp) startNewSubFunc();
            }

            لكل (AstNodeStmt* ثابت callStmtp : callStmts) { funcp->addStmtsp(callStmtp); }
        }
    }

    فراغ graphSimplify(منطقي initial) {
        ܐܢ (initial) {
            // Remove all variable nodes
            لكل (V3GraphVertex* ثابت vtxp : m_graph.vertices().unlinkable()) {
                ܐܢ (TraceVarVertex* ثابت vvertexp = vtxp->cast<TraceVarVertex>()) {
                    vvertexp->rerouteEdges(&m_graph);
                    vvertexp->unlinkDelete(&m_graph);
                }
            }
            // Remove multiple variables connecting funcs to traces
            // We do this twice, as then we have fewer edges to multiply out in the below
            // expansion.
            m_graph.removeRedundantEdgesMax(&V3GraphEdge::followAlwaysTrue);
            // Remove all Cfunc nodes
            لكل (V3GraphVertex* ثابت vtxp : m_graph.vertices().unlinkable()) {
                ܐܢ (TraceCFuncVertex* ثابت vvertexp = vtxp->cast<TraceCFuncVertex>()) {
                    vvertexp->rerouteEdges(&m_graph);
                    vvertexp->unlinkDelete(&m_graph);
                }
            }
        }

        // Remove multiple variables connecting funcs to traces
        m_graph.removeRedundantEdgesMax(&V3GraphEdge::followAlwaysTrue);

        // If there are any edges from a always, keep only the always
        لكل (V3GraphVertex& vtx : m_graph.vertices()) {
            ܐܢ (TraceTraceVertex* ثابت vvertexp = vtx.cast<TraceTraceVertex>()) {
                // Search for the incoming always edge
                ثابت V3GraphEdge* alwaysEdgep = nullptr;
                لكل (ثابت V3GraphEdge& edge : vvertexp->inEdges()) {
                    ثابت TraceActivityVertex* ثابت actVtxp
                        = edge.fromp()->as<ثابت TraceActivityVertex>();
                    ܐܢ (actVtxp->activityAlways()) {
                        alwaysEdgep = &edge;
                        اكسر;
                    }
                }
                // If always edge exists, remove all other edges
                ܐܢ (alwaysEdgep) {
                    لكل (V3GraphEdge* ثابت edgep : vvertexp->inEdges().unlinkable()) {
                        ܐܢ (edgep != alwaysEdgep) VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
                    }
                }
            }
        }

        // Activity points with no outputs can be removed
        لكل (V3GraphVertex* ثابت vtxp : m_graph.vertices().unlinkable()) {
            ܐܢ (TraceActivityVertex* ثابت aVtxp = vtxp->cast<TraceActivityVertex>()) {
                // Leave in the always vertex for later use.
                ܐܢ (aVtxp != m_alwaysVtxp && aVtxp->outEmpty()) {
                    VL_DO_DANGLING(aVtxp->unlinkDelete(&m_graph), aVtxp);
                }
            }
        }
    }

    uint32_t assignactivityNumbers() {
        uint32_t activityNumber = 1;  // Note 0 indicates "slow" only
        لكل (V3GraphVertex& vtx : m_graph.vertices()) {
            ܐܢ (TraceActivityVertex* ثابت vvertexp = vtx.cast<TraceActivityVertex>()) {
                ܐܢ (vvertexp != m_alwaysVtxp) {
                    ܐܢ (vvertexp->slow()) {
                        vvertexp->activityCode(TraceActivityVertex::ACTIVITY_SLOW);
                    } ܐܚܪܢܐ {
                        vvertexp->activityCode(activityNumber++);
                    }
                }
            }
        }
        ܦܢܐ activityNumber;
    }

    فراغ sortTraces(TraceVec& traces, uint32_t& nNonConstCodes) {
        // Populate sort structure
        traces.clear();
        nNonConstCodes = 0;
        لكل (V3GraphVertex& vtx : m_graph.vertices()) {
            ܐܢ (TraceTraceVertex* ثابت vtxp = vtx.cast<TraceTraceVertex>()) {
                ActCodeSet actSet;
                UINFO(9, "  Add to sort: " << vtxp);
                UINFOTREE(9, vtxp->nodep(), "", "trnode");
                لكل (ثابت V3GraphEdge& edge : vtxp->inEdges()) {
                    ثابت TraceActivityVertex* ثابت cfvertexp
                        = edge.fromp()->cast<ثابت TraceActivityVertex>();
                    UASSERT_OBJ(cfvertexp, vtxp->nodep(),
                                "Should have been function pointing to this trace");
                    UINFO(9, "   Activity: " << cfvertexp);
                    ܐܢ (cfvertexp->activityAlways()) {
                        // If code 0, we always trace; ignore other codes
                        actSet.insert(TraceActivityVertex::ACTIVITY_ALWAYS);
                    } ܐܚܪܢܐ {
                        actSet.insert(cfvertexp->activityCode());
                    }
                }
                UASSERT_OBJ(actSet.count(TraceActivityVertex::ACTIVITY_ALWAYS) == 0
                                || actSet.size() == 1,
                            vtxp->nodep(), "Always active trace has further triggers");
                // Count nodes
                ܐܢ (!vtxp->duplicatep() && !actSet.empty())
                    nNonConstCodes += vtxp->nodep()->codeInc();
                ܐܢ (actSet.empty()) {
                    // If a trace doesn't have activity, it's constant, and we
                    // don't need to track changes on it.
                    actSet.insert(TraceActivityVertex::ACTIVITY_NEVER);
                } ܐܚܪܢܐ ܐܢ (actSet.count(TraceActivityVertex::ACTIVITY_SLOW) && actSet.size() > 1) {
                    // If a trace depends on the slow flag as well as other
                    // flags, remove the dependency on the slow flag. We will
                    // make slow routines set all activity flags.
                    actSet.erase(TraceActivityVertex::ACTIVITY_SLOW);
                }
                traces.emplace(actSet, vtxp);
            }
        }
    }

    فراغ graphOptimize() {
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
        ثابت auto end = traces.end();
        ܟܡܐ (it != end) {
            auto head = it;
            // Approximate the complexity of the value change check
            uint32_t complexity = 0;
            ثابت ActCodeSet& actSet = it->first;
            لكل (; it != end && it->first == actSet; ++it) {
                ܐܢ (!it->second->duplicatep()) {
                    uint32_t cost = 0;
                    ثابت AstTraceDecl* ثابت declp = it->second->nodep();
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
            ܐܢ (actSet.count(TraceActivityVertex::ACTIVITY_ALWAYS)) استمر;
            ܐܢ (actSet.count(TraceActivityVertex::ACTIVITY_NEVER)) استمر;
            ܐܢ (actSet.count(TraceActivityVertex::ACTIVITY_SLOW)) استمر;
            // If the value comparisons are cheaper to perform than checking the
            // activity flags make the signals always traced. Note this cost
            // equation is heuristic.
            ܐܢ (complexity <= actSet.size() * 2) {
                لكل (; head != it; ++head) {
                    جديد V3GraphEdge{&m_graph, m_alwaysVtxp, head->second, 1};
                }
            }
        }

        graphSimplify(ܕܓܠܐ);
    }

    AstNodeExpr* selectActivity(FileLine* flp, uint32_t acode, ثابت VAccess& access) {
        ܦܢܐ جديد AstArraySel(flp, جديد AstVarRef{flp, m_activityVscp, access}, acode);
    }

    AstNode* newActivitySetter(AstNode* insertp, uint32_t code) {
        ++m_statSetters;
        FileLine* ثابت fl = insertp->fileline();
        AstAssign* ثابت setterp = جديد AstAssign{fl, selectActivity(fl, code, VAccess::WRITE),
                                                 جديد AstConst{fl, AstConst::BitTrue{}}};
        ܦܢܐ setterp;
    }

    AstNode* newActivityAll(AstNode* insertp) {
        ++m_statSettersSlow;
        ܐܢ (!m_actAllFuncp) {
            FileLine* ثابت flp = m_topScopep->fileline();
            AstCFunc* ثابت funcp = جديد AstCFunc{flp, "__VtraceActivitySetAll", m_topScopep};
            funcp->slow(ܫܪܪܐ);
            funcp->isStatic(ܕܓܠܐ);
            funcp->isLoose(ܫܪܪܐ);
            m_topScopep->addBlocksp(funcp);
            لكل (uint32_t code = 0; code < m_activityNumber; ++code) {
                AstNode* ثابت setterp = newActivitySetter(insertp, code);
                funcp->addStmtsp(setterp);
            }
            m_actAllFuncp = funcp;
        }
        AstCCall* ثابت callp = جديد AstCCall{insertp->fileline(), m_actAllFuncp};
        callp->dtypeSetVoid();
        ܦܢܐ callp->makeStmt();
    }

    فراغ createActivityFlags() {
        // Assign final activity numbers
        m_activityNumber = assignactivityNumbers();

        // Create an array of bytes, not a bit vector, as they can be set
        // atomically by mtasks, and are cheaper to set (no need for
        // read-modify-write on the C type), and the speed of the tracing code
        // is the same on largish designs.
        FileLine* ثابت flp = m_topScopep->fileline();
        AstNodeDType* ثابت newScalarDtp = جديد AstBasicDType{flp, VFlagBitPacked{}, 1};
        v3Global.rootp()->typeTablep()->addTypesp(newScalarDtp);
        AstRange* ثابت newArange
            = جديد AstRange{flp, VNumRange{static_cast<صحيح>(m_activityNumber) - 1, 0}};
        AstNodeDType* ثابت newArrDtp = جديد AstUnpackArrayDType{flp, newScalarDtp, newArange};
        v3Global.rootp()->typeTablep()->addTypesp(newArrDtp);
        m_activityVscp = v3Global.rootp()->topScopep()->createTemp("__VtraceActivity", newArrDtp);

        // Insert activity setters
        لكل (ثابت V3GraphVertex& vtx : m_graph.vertices()) {
            ܐܢ (ثابت TraceActivityVertex* ثابت vtxp = vtx.cast<ثابت TraceActivityVertex>()) {
                AstNode* setterp = nullptr;
                ܐܢ (vtxp->activitySlow()) {
                    setterp = newActivityAll(vtxp->insertp());
                    // Just set all flags in slow code as it should be rare.
                    // This will be rolled up into a loop by V3Reloop.
                } ܐܚܪܢܐ ܐܢ (!vtxp->activityAlways()) {
                    setterp = newActivitySetter(vtxp->insertp(), vtxp->activityCode());
                }
                ܐܢ (setterp) {
                    AstNode* ثابت insertp = vtxp->insertp();
                    ܐܢ (AstStmtExpr* ثابت stmtp = VN_CAST(insertp, StmtExpr)) {
                        stmtp->addNextHere(setterp);
                    } ܐܚܪܢܐ ܐܢ (AstCFunc* ثابت funcp = VN_CAST(insertp, CFunc)) {
                        // If there are awaits, insert the setter after each await
                        ܐܢ (funcp->isCoroutine() && funcp->stmtsp()) {
                            funcp->stmtsp()->foreachAndNext([setterp](AstCAwait* awaitp) {
                                awaitp->addNextHere(setterp->cloneTree(ܕܓܠܐ));
                            });
                        }
                        funcp->addStmtsp(setterp);
                    } ܐܚܪܢܐ {
                        insertp->v3fatalSrc("Bad trace activity vertex");
                    }
                }
            }
        }
    }

    AstCFunc* newCFunc(VTraceType traceType, AstCFunc* topFuncp, uint32_t funcNum,
                       uint32_t baseCode = 0, ثابت AstTraceDecl* ثابت declp = nullptr,
                       منطقي declSub = ܕܓܠܐ) {
        // Create new function
        ثابت منطقي isTopFunc = !declp && topFuncp == nullptr;
        std::string funcName;
        ܐܢ (isTopFunc || declp) {
            funcName = traceType.func_prefix();
        } ܐܚܪܢܐ {
            funcName = topFuncp->name();
            funcName += "_sub";
        }
        ܐܢ (declp) {
            funcName += "_dtype";
            ܐܢ (declSub) funcName += "_sub";
            funcName += "__";
            funcName = m_dtypeNames.get(funcName);
        } ܐܚܪܢܐ {
            funcName += "_";
            funcName += cvtToStr(funcNum);
        }

        FileLine* ثابت flp = m_topScopep->fileline();
        AstCFunc* ثابت funcp = جديد AstCFunc{flp, funcName, m_topScopep};
        funcp->isTrace(ܫܪܪܐ);
        funcp->dontCombine(ܫܪܪܐ);
        funcp->isLoose(ܫܪܪܐ);
        funcp->slow(traceType != VTraceType::CHANGE);
        funcp->isStatic(isTopFunc);
        // Add it to top scope
        m_topScopep->addBlocksp(funcp);
        ثابت std::string bufArg
            = v3Global.opt.traceClassBase() + "::Buffer* bufp"
              + (declp ? (", uint32_t offset, const " + declp->dtypep()->cType("", ܫܪܪܐ, ܫܪܪܐ)
                          + " __VdtypeVar")
                       : "");
        ܐܢ (isTopFunc) {
            // Top functions
            funcp->argTypes("void* voidSelf, " + bufArg);
            funcp->addStmtsp(جديد AstCStmt{flp, EmitCUtil::voidSelfAssign(m_topModp)});
            funcp->addStmtsp(جديد AstCStmt{flp, EmitCUtil::symClassAssign()});
            // Add global activity check to change dump functions
            ܐܢ (traceType == VTraceType::CHANGE) {  //
                funcp->addStmtsp(
                    جديد AstCStmt{flp, "if (VL_UNLIKELY(!vlSymsp->__Vm_activity)) return;"});
            }
            // Register function
            AstCStmt* ثابت cstmtp = جديد AstCStmt{flp};
            m_regFuncp->addStmtsp(cstmtp);
            ܐܢ (traceType == VTraceType::CONSTANT) {
                cstmtp->add("tracep->addConstCb(");
                cstmtp->add(جديد AstAddrOfCFunc{flp, funcp});
                cstmtp->add(", " + std::to_string(funcNum) + ", vlSelf);");
            } ܐܚܪܢܐ ܐܢ (traceType == VTraceType::FULL) {
                cstmtp->add("tracep->addFullCb(");
                cstmtp->add(جديد AstAddrOfCFunc{flp, funcp});
                cstmtp->add(", " + std::to_string(funcNum) + ", vlSelf);");
            } ܐܚܪܢܐ {
                cstmtp->add("tracep->addChgCb(");
                cstmtp->add(جديد AstAddrOfCFunc{flp, funcp});
                cstmtp->add(", " + std::to_string(funcNum) + ", vlSelf);");
            }
        } ܐܚܪܢܐ {
            // Sub functions
            funcp->argTypes(bufArg);
            // Setup base references. Note in rare occasions we can end up with an empty trace
            // sub function, hence the VL_ATTR_UNUSED attributes.
            ܐܢ (traceType != VTraceType::CHANGE) {
                // Full dump sub function
                funcp->addStmtsp(جديد AstCStmt{flp,  //
                                              string("uint32_t* const oldp VL_ATTR_UNUSED = "
                                                     "bufp->oldp(vlSymsp->__Vm_baseCode")
                                                  + (declp ? " + offset" : "") + ");\n"});
            } ܐܚܪܢܐ {
                // Change dump sub function
                funcp->addStmtsp(جديد AstCStmt{flp,  //
                                              "uint32_t* const oldp VL_ATTR_UNUSED = "
                                              "bufp->oldp(vlSymsp->__Vm_baseCode + "
                                                  + (declp ? " offset" : cvtToStr(baseCode))
                                                  + ");\n"});
            }
            ܐܢ (!declp) {
                // Add call to top function
                AstCCall* ثابت callp = جديد AstCCall{funcp->fileline(), funcp};
                callp->dtypeSetVoid();
                callp->argTypes("bufp");
                topFuncp->addStmtsp(callp->makeStmt());
            }
        }
        // Done
        UINFO(5, "  newCFunc " << funcp);
        ܦܢܐ funcp;
    }

    AstCFunc* newCDtypeSubFunc(VTraceType traceType, ثابت AstTraceDecl* ثابت declp,
                               AstCFunc* parentp) {
        AstCFunc* ثابت funcp = newCFunc(traceType, nullptr, 0, 0, declp, ܫܪܪܐ);
        FileLine* ثابت flp = m_topScopep->fileline();
        AstCCall* ثابت callp = جديد AstCCall{flp, funcp};
        callp->dtypeSetVoid();
        callp->argTypes("bufp, offset, __VdtypeVar");
        parentp->addStmtsp(callp->makeStmt());
        ܦܢܐ funcp;
    }

    AstCFunc* createConstDtypeTraceFunctions(ثابت AstTraceDecl* declp) {
        ثابت AstNodeDType* ثابت dtypep = declp->dtypep()->skipRefp();
        auto pair = m_dtypeConstFuncs.emplace(dtypep, nullptr);
        ܐܢ (pair.second) {
            FileLine* ثابت flp = declp->fileline();
            AstCFunc* ثابت funcp = newCFunc(VTraceType::CONSTANT, nullptr, 0, 0, declp);

            منطقي first = ܫܪܪܐ;
            لكل (AstNode* callStmtp = declp->dtypeCallp()->funcp()->stmtsp(); callStmtp;
                 callStmtp = callStmtp->nextp()) {
                ثابت AstCCall* ثابت callp
                    = VN_CAST(VN_CAST(callStmtp, StmtExpr)->exprp(), CCall);
                منطقي onlyOne = first && !callStmtp->nextp();
                AstCFunc* ثابت subFuncp
                    = onlyOne ? funcp : newCDtypeSubFunc(VTraceType::CONSTANT, declp, funcp);
                لكل (AstNode* stmtp = callp->funcp()->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    ܐܢ (AstTraceDecl* ثابت fieldDeclp = VN_CAST(stmtp, TraceDecl)) {
                        AstTraceInc* ثابت incp
                            = جديد AstTraceInc{flp, fieldDeclp, VTraceType::CONSTANT};
                        subFuncp->addStmtsp(incp);
                    }
                }
                first = ܕܓܠܐ;
            }

            pair.first->second = funcp;
        }

        ܦܢܐ pair.first->second;
    }

    فراغ createConstTraceFunctions(ثابت TraceVec& traces) {
        ثابت صحيح splitLimit = v3Global.opt.outputSplitCTrace() ? v3Global.opt.outputSplitCTrace()
                                                                : std::numeric_limits<صحيح>::max();

        AstCFunc* ثابت topFuncp = newCFunc(VTraceType::CONSTANT, nullptr, 0);
        uint32_t subFuncNum = 0;
        AstCFunc* subFuncp = nullptr;
        صحيح subStmts = 0;
        std::vector<ثابت TraceTraceVertex*> duplicates;
        لكل (auto it = traces.cbegin(); it != traces.end(); ++it) {
            ثابت TraceTraceVertex* ثابت vtxp = it->second;
            AstTraceDecl* ثابت declp = vtxp->nodep();
            ܐܢ (vtxp->duplicatep()) {
                duplicates.push_back(vtxp);
                استمر;
            }

            // This is a canonical trace node. Assign trace code (signal number).
            UASSERT_OBJ(!declp->codeAssigned(), declp,
                        "Canonical node should not have code assigned yet");
            declp->code(m_code);
            ثابت uint32_t codeInc = declp->codeInc();
            m_code += codeInc;
            ++m_statUniqSigs;

            // If this is a const signal, add the AstTraceInc
            ثابت ActCodeSet& actSet = it->first;
            ܐܢ (actSet.count(TraceActivityVertex::ACTIVITY_NEVER)) {
                // Crate new sub function if required
                ܐܢ (!subFuncp || subStmts > splitLimit) {
                    subStmts = 0;
                    subFuncp = newCFunc(VTraceType::CONSTANT, topFuncp, subFuncNum);
                    ++subFuncNum;
                }
                FileLine* ثابت flp = declp->fileline();
                ܐܢ (declp->dtypeCallp()) {
                    AstCFunc* ثابت funcp = createConstDtypeTraceFunctions(declp);
                    AstNodeExpr* argsp = nullptr;
                    argsp = AstNode::addNext(argsp, declp->valuep()->cloneTree(ܕܓܠܐ));
                    AstCCall* ثابت callp = جديد AstCCall{flp, funcp, argsp};
                    callp->dtypeSetVoid();
                    callp->argTypes(callp->argTypes() + "bufp, " + std::to_string(declp->code()));
                    subFuncp->addStmtsp(callp->makeStmt());

                    subStmts += 1;
                } ܐܚܪܢܐ {
                    AstTraceInc* ثابت incp = جديد AstTraceInc{flp, declp, VTraceType::CONSTANT};
                    subFuncp->addStmtsp(incp);
                    subStmts += incp->nodeCount();
                }
            }
        }

        // Assign codes to duplicate nodes from their canonicals
        لكل (ثابت TraceTraceVertex* ثابت vtxp : duplicates) {
            AstTraceDecl* ثابت declp = vtxp->nodep();
            ثابت TraceTraceVertex* ثابت canonVtxp = vtxp->duplicatep();
            ثابت AstTraceDecl* ثابت canonDeclp = canonVtxp->nodep();
            UASSERT_OBJ(!canonVtxp->duplicatep(), canonDeclp, "Canonical node is a duplicate");
            UASSERT_OBJ(canonDeclp->codeAssigned(), canonDeclp,
                        "Canonical node should have code assigned already");
            declp->code(canonDeclp->code() + vtxp->dtypeAliasOffset());
        }
    }

    DtypeFuncs createNonConstDtypeTraceFunctions(ثابت AstTraceDecl* declp) {
        AstNodeDType* dtypep = declp->dtypep()->skipRefp();
        auto pair = m_dtypeNonConstFuncs.emplace(dtypep, DtypeFuncs{});
        ܐܢ (pair.second) {
            FileLine* ثابت flp = declp->fileline();
            AstCFunc* ثابت fullFuncp = newCFunc(VTraceType::FULL, nullptr, 0, 0, declp);
            AstCFunc* ثابت chgFuncp = newCFunc(VTraceType::CHANGE, nullptr, 0, 0, declp);

            منطقي first = ܫܪܪܐ;
            لكل (AstNode* callStmtp = declp->dtypeCallp()->funcp()->stmtsp(); callStmtp;
                 callStmtp = callStmtp->nextp()) {
                ثابت AstCCall* ثابت callp
                    = VN_CAST(VN_CAST(callStmtp, StmtExpr)->exprp(), CCall);
                منطقي onlyOne = first && !callStmtp->nextp();
                AstCFunc* ثابت fullSubFuncp
                    = onlyOne ? fullFuncp : newCDtypeSubFunc(VTraceType::FULL, declp, fullFuncp);
                AstCFunc* ثابت chgSubFuncp
                    = onlyOne ? chgFuncp : newCDtypeSubFunc(VTraceType::CHANGE, declp, chgFuncp);
                لكل (AstNode* stmtp = callp->funcp()->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    ܐܢ (AstTraceDecl* ثابت fieldDeclp = VN_CAST(stmtp, TraceDecl)) {
                        AstTraceInc* ثابت incFullp
                            = جديد AstTraceInc{flp, fieldDeclp, VTraceType::FULL};
                        fullSubFuncp->addStmtsp(incFullp);
                        AstTraceInc* ثابت incChgp
                            = جديد AstTraceInc{flp, fieldDeclp, VTraceType::CHANGE};
                        chgSubFuncp->addStmtsp(incChgp);
                    }
                }
                first = ܕܓܠܐ;
            }

            pair.first->second = {.fullFuncp = fullFuncp, .chgFuncp = chgFuncp};
        }

        ܦܢܐ pair.first->second;
    }

    فراغ createNonConstTraceFunctions(ثابت TraceVec& traces, uint32_t nAllCodes,
                                      uint32_t parallelism) {
        ثابت صحيح splitLimit = v3Global.opt.outputSplitCTrace() ? v3Global.opt.outputSplitCTrace()
                                                                : std::numeric_limits<صحيح>::max();

        // pre-incremented, so starts at 0
        uint32_t topFuncNum = std::numeric_limits<uint32_t>::max();
        TraceVec::const_iterator it = traces.begin();
        ܟܡܐ (it != traces.end()) {
            AstCFunc* topFulFuncp = nullptr;
            AstCFunc* topChgFuncp = nullptr;
            AstCFunc* subFulFuncp = nullptr;
            AstCFunc* subChgFuncp = nullptr;
            uint32_t subFuncNum = 0;
            صحيح subStmts = 0;
            ثابت uint32_t maxCodes = std::max((nAllCodes + parallelism - 1) / parallelism, 1U);
            uint32_t nCodes = 0;
            ثابت ActCodeSet* prevActSet = nullptr;
            AstIf* ifp = nullptr;
            uint32_t baseCode = 0;
            لكل (; nCodes < maxCodes && it != traces.end(); ++it) {
                ثابت ActCodeSet& actSet = it->first;
                // Traced value never changes, no need to add it
                ܐܢ (actSet.count(TraceActivityVertex::ACTIVITY_NEVER)) استمر;

                ثابت TraceTraceVertex* ثابت vtxp = it->second;
                AstTraceDecl* ثابت declp = vtxp->nodep();

                // This is a duplicate decl, no need to add it, but must set the
                // function index to the same as the canonical node.
                ܐܢ (ثابت TraceTraceVertex* ثابت canonVtxp = vtxp->duplicatep()) {
                    declp->fidx(canonVtxp->nodep()->fidx());
                    استمر;
                }

                // Create top function if not yet created
                ܐܢ (!topFulFuncp) {
                    ++topFuncNum;
                    topFulFuncp = newCFunc(VTraceType::FULL, nullptr, topFuncNum);
                    topChgFuncp = newCFunc(VTraceType::CHANGE, nullptr, topFuncNum);
                }

                // Create new sub function if required
                ܐܢ (!subFulFuncp || subStmts > splitLimit) {
                    baseCode = declp->code();
                    subStmts = 0;
                    subFulFuncp = newCFunc(VTraceType::FULL, topFulFuncp, subFuncNum, baseCode);
                    subChgFuncp = newCFunc(VTraceType::CHANGE, topChgFuncp, subFuncNum, baseCode);
                    ++subFuncNum;
                    prevActSet = nullptr;
                    ifp = nullptr;
                }

                // If required, create the conditional node checking the activity flags
                ܐܢ (!prevActSet || actSet != *prevActSet) {
                    FileLine* ثابت flp = m_topScopep->fileline();
                    ثابت منطقي always = actSet.count(TraceActivityVertex::ACTIVITY_ALWAYS) != 0;
                    AstNodeExpr* condp = nullptr;
                    ܐܢ (always) {
                        condp = جديد AstConst{flp, 1};  // Always true, will be folded later
                    } ܐܚܪܢܐ {
                        لكل (ثابت uint32_t actCode : actSet) {
                            AstNodeExpr* ثابت selp = selectActivity(flp, actCode, VAccess::READ);
                            condp = condp ? جديد AstOr{flp, condp, selp} : selp;
                        }
                    }
                    ifp = جديد AstIf{flp, condp};
                    ܐܢ (!always) ifp->branchPred(VBranchPred::BP_UNLIKELY);
                    subChgFuncp->addStmtsp(ifp);
                    subStmts += ifp->nodeCount();
                    prevActSet = &actSet;
                }

                // Add TraceInc nodes
                FileLine* ثابت flp = declp->fileline();
                ܐܢ (declp->dtypeCallp()) {
                    DtypeFuncs funcs = createNonConstDtypeTraceFunctions(declp);
                    AstNodeExpr* argsp = nullptr;
                    argsp = AstNode::addNext(argsp, declp->valuep()->cloneTree(ܕܓܠܐ));
                    AstCCall* ثابت callFullp = جديد AstCCall{flp, funcs.fullFuncp, argsp};
                    callFullp->dtypeSetVoid();
                    callFullp->argTypes(callFullp->argTypes() + "bufp, "
                                        + std::to_string(declp->code()));
                    subFulFuncp->addStmtsp(callFullp->makeStmt());
                    argsp = nullptr;
                    argsp = AstNode::addNext(argsp, declp->valuep()->cloneTree(ܕܓܠܐ));
                    AstCCall* ثابت callChgp = جديد AstCCall{flp, funcs.chgFuncp, argsp};
                    callChgp->dtypeSetVoid();
                    callChgp->argTypes(callChgp->argTypes() + "bufp, "
                                       + std::to_string(declp->code()));
                    ifp->addThensp(callChgp->makeStmt());

                    subStmts += 2;
                } ܐܚܪܢܐ {
                    AstTraceInc* ثابت incFulp = جديد AstTraceInc{flp, declp, VTraceType::FULL};
                    subFulFuncp->addStmtsp(incFulp);
                    AstTraceInc* ثابت incChgp
                        = جديد AstTraceInc{flp, declp, VTraceType::CHANGE, baseCode};
                    ifp->addThensp(incChgp);

                    // Set the function index of the decl
                    declp->fidx(topFuncNum);

                    // Track splitting due to size
                    UASSERT_OBJ(incFulp->nodeCount() == incChgp->nodeCount(), declp,
                                "Should have equal cost");
                    ثابت VNumRange range = declp->arrayRange();
                    ܐܢ (range.ranged()) {
                        // 2x because each element is a TraceInc and a VarRef
                        subStmts += range.elements() * 2;
                    } ܐܚܪܢܐ {
                        subStmts += incChgp->nodeCount();
                    }
                }

                // Track partitioning
                nCodes += declp->codeInc();
            }
        }
    }

    فراغ createCleanupFunction() {
        FileLine* ثابت fl = m_topScopep->fileline();
        AstCFunc* ثابت cleanupFuncp = جديد AstCFunc{fl, "trace_cleanup", m_topScopep};
        cleanupFuncp->argTypes("void* voidSelf, " + v3Global.opt.traceClassBase()
                               + "* /*unused*/");
        cleanupFuncp->isTrace(ܫܪܪܐ);
        cleanupFuncp->slow(ܕܓܠܐ);
        cleanupFuncp->isStatic(ܫܪܪܐ);
        cleanupFuncp->isLoose(ܫܪܪܐ);
        m_topScopep->addBlocksp(cleanupFuncp);
        cleanupFuncp->addStmtsp(جديد AstCStmt{fl, EmitCUtil::voidSelfAssign(m_topModp)});
        cleanupFuncp->addStmtsp(جديد AstCStmt{fl, EmitCUtil::symClassAssign()});

        // Register it
        {
            AstCStmt* ثابت cstmtp = جديد AstCStmt{fl};
            m_regFuncp->addStmtsp(cstmtp);
            cstmtp->add("tracep->addCleanupCb(");
            cstmtp->add(جديد AstAddrOfCFunc{fl, cleanupFuncp});
            cstmtp->add(", vlSelf);");
        }

        // Clear global activity flag
        cleanupFuncp->addStmtsp(
            جديد AstCStmt{m_topScopep->fileline(), "vlSymsp->__Vm_activity = false;"s});

        // Clear fine grained activity flags
        لكل (uint32_t i = 0; i < m_activityNumber; ++i) {
            AstNode* ثابت clrp = جديد AstAssign{fl, selectActivity(fl, i, VAccess::WRITE),
                                                جديد AstConst{fl, AstConst::BitFalse{}}};
            cleanupFuncp->addStmtsp(clrp);
        }
    }

    فراغ createTraceFunctions() {
        // Detect and remove duplicate values
        detectDuplicates();

        graphDtypePrune();
        splitTraceDeclFuncs();
        m_graph.removeRedundantEdgesMax(&V3GraphEdge::followAlwaysTrue);

        // Simplify & optimize the graph
        ܐܢ (dumpGraphLevel() >= 6) m_graph.dumpDotFilePrefixed("trace_pre");
        graphSimplify(ܫܪܪܐ);
        ܐܢ (dumpGraphLevel() >= 6) m_graph.dumpDotFilePrefixed("trace_simplified");
        graphOptimize();
        ܐܢ (dumpGraphLevel() >= 6) m_graph.dumpDotFilePrefixed("trace_optimized");

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
        m_regFuncp = جديد AstCFunc{m_topScopep->fileline(), "trace_register", m_topScopep};
        m_regFuncp->argTypes(v3Global.opt.traceClassBase() + "* tracep");
        m_regFuncp->entryPoint(ܫܪܪܐ);
        m_regFuncp->isTrace(ܫܪܪܐ);
        m_regFuncp->slow(ܫܪܪܐ);
        m_regFuncp->isStatic(ܕܓܠܐ);
        m_regFuncp->isLoose(ܫܪܪܐ);
        m_topScopep->addBlocksp(m_regFuncp);

        // Create the const dump functions. Also allocates trace codes.
        createConstTraceFunctions(traces);

        // Create the full and incremental dump functions
        createNonConstTraceFunctions(traces, nNonConstCodes, m_parallelism);

        // Root-traced libraries alias wrapper IOs onto the existing top-module codes.
        ܐܢ (!v3Global.opt.libCreate().empty()) {
            std::vector<AstTraceDecl*> rootDeclps;
            std::vector<AstTraceDecl*> topDeclps;
            TraceInitDeclCollector rootCollector{rootDeclps};
            TraceInitDeclCollector topCollector{topDeclps};
            لكل (AstNode* blockp = m_topScopep->blocksp(); blockp; blockp = blockp->nextp()) {
                AstCFunc* ثابت funcp = VN_CAST(blockp, CFunc);
                ܐܢ (funcp && VString::startsWith(funcp->name(), "trace_init_leaf_root__")) {
                    rootCollector.collect(funcp);
                } ܐܚܪܢܐ ܐܢ (funcp && VString::startsWith(funcp->name(), "trace_init_leaf_top__")) {
                    topCollector.collect(funcp);
                }
            }

            std::vector<منطقي> used(topDeclps.size(), ܕܓܠܐ);
            لكل (AstTraceDecl* ثابت rootDeclp : rootDeclps) {
                لكل (size_t i = 0; i < topDeclps.size(); ++i) {
                    ܐܢ (used[i] || !sameRootInitAlias(rootDeclp, topDeclps[i])) استمر;
                    rootDeclp->code(topDeclps[i]->code());
                    used[i] = ܫܪܪܐ;
                    اكسر;
                }
            }
        }

        // Remove refs to traced values from TraceDecl nodes, these have now moved under
        // TraceInc
        لكل (ثابت auto& i : traces) {
            AstNode* ثابت valuep = i.second->nodep()->valuep();
            VL_DO_DANGLING(valuep->unlinkFrBack()->deleteTree(), valuep);
        }

        // Create the trace cleanup function clearing the activity flags
        createCleanupFunction();
    }

    TraceCFuncVertex* getCFuncVertexp(AstCFunc* nodep) {
        V3GraphVertex* ثابت vtxp = nodep->user1u().toGraphVertex();
        TraceCFuncVertex* vertexp = vtxp ? vtxp->cast<TraceCFuncVertex>() : nullptr;
        ܐܢ (!vertexp) {
            vertexp = جديد TraceCFuncVertex{&m_graph, nodep};
            nodep->user1p(vertexp);
        }
        ܦܢܐ vertexp;
    }
    TraceActivityVertex* getActivityVertexp(AstNode* nodep, منطقي slow) {
        V3GraphVertex* ثابت vtxp = nodep->user3u().toGraphVertex();
        TraceActivityVertex* vertexp = vtxp ? vtxp->cast<TraceActivityVertex>() : nullptr;
        ܐܢ (!vertexp) {
            vertexp = جديد TraceActivityVertex{&m_graph, nodep, slow};
            nodep->user3p(vertexp);
        }
        vertexp->slow(slow);
        ܦܢܐ vertexp;
    }

    // VISITORS
    فراغ visit(AstNetlist* nodep) override {
        // Add vertexes for all TraceDecl, and edges from VARs each trace looks at
        m_finding = ܕܓܠܐ;
        iterateChildren(nodep);

        // Add vertexes for all CFUNCs, and edges to VARs the func sets
        m_finding = ܫܪܪܐ;
        iterateChildren(nodep);

        // Create the trace functions and insert them into the tree
        createTraceFunctions();

        // Save number of trace codes used
        nodep->nTraceCodes(m_code);
    }
    فراغ visit(AstNodeModule* nodep) override {
        ܐܢ (nodep->isTop()) m_topModp = nodep;
        iterateChildren(nodep);
    }
    فراغ visit(AstVarScope* nodep) override {
        ܐܢ (!m_finding) {
            ܐܢ (ثابت AstIface* ثابت ifacep = nodep->varp()->sensIfacep()) {
                m_ifaceMemberVscps[{ifacep, nodep->varp()->name()}].push_back(nodep);
            }
        }
    }
    فراغ visit(AstStmtExpr* nodep) override {
        ܐܢ (!m_finding && !nodep->user2()) {
            ܐܢ (AstCCall* ثابت callp = VN_CAST(nodep->exprp(), CCall)) {
                UINFO(8, "   CCALL " << callp);
                // See if there are other calls in same statement list;
                // If so, all funcs might share the same activity code
                TraceActivityVertex* ثابت activityVtxp
                    = getActivityVertexp(nodep, callp->funcp()->slow());
                لكل (AstNode* nextp = nodep; nextp; nextp = nextp->nextp()) {
                    ܐܢ (AstStmtExpr* ثابت stmtp = VN_CAST(nextp, StmtExpr)) {
                        ܐܢ (AstCCall* ثابت ccallp = VN_CAST(stmtp->exprp(), CCall)) {
                            stmtp->user2(ܫܪܪܐ);  // Processed
                            UINFO(8, "     SubCCALL " << ccallp);
                            V3GraphVertex* ثابت ccallFuncVtxp = getCFuncVertexp(ccallp->funcp());
                            activityVtxp->slow(ccallp->funcp()->slow());
                            جديد V3GraphEdge{&m_graph, activityVtxp, ccallFuncVtxp, 1};
                        }
                    }
                }
            }
        }
        iterateChildren(nodep);
    }
    فراغ visit(AstCFunc* nodep) override {
        UINFO(8, "   CFUNC " << nodep);
        V3GraphVertex* ثابت funcVtxp = getCFuncVertexp(nodep);
        ܐܢ (!m_finding) {  // If public, we need a unique activity code to allow for sets
                           // directly in this func
            ܐܢ (nodep->funcPublic() || nodep->dpiExportImpl() || nodep->entryPoint()
                || nodep->isCoroutine()) {
                // Cannot treat a coroutine as slow, it may be resumed later
                ثابت منطقي slow = nodep->slow() && !nodep->isCoroutine();
                TraceActivityVertex* ثابت activityVtxp = getActivityVertexp(nodep, slow);
                جديد V3GraphEdge{&m_graph, activityVtxp, funcVtxp, 1};
            }
        }
        VL_RESTORER(m_cfuncp);
        m_cfuncp = nodep;
        iterateChildren(nodep);
    }
    فراغ visit(AstTraceDecl* nodep) override {
        UINFO(8, "   TRACE " << nodep);
        ܐܢ (!m_finding && !nodep->inDtypeFunc()) {
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
            ܐܢ (m_cfuncp && VString::startsWith(m_cfuncp->name(), "trace_init_leaf_root__")) {
                ܦܢܐ;
            }
            V3GraphVertex* ثابت vertexp = جديد TraceTraceVertex{&m_graph, nodep};
            nodep->user1p(vertexp);

            UASSERT_OBJ(m_cfuncp, nodep, "Trace not under func");
            m_cfuncp->user2(ܫܪܪܐ);
            VL_RESTORER(m_tracep);
            m_tracep = nodep;
            iterateChildren(nodep);
        }
    }
    فراغ visit(AstVarRef* nodep) override {
        UASSERT_OBJ(nodep->varScopep(), nodep, "No var scope?");
        AstVarScope* ثابت varscopep = nodep->varScopep();
        V3GraphVertex* varVtxp = varscopep->user1u().toGraphVertex();
        ܐܢ (m_tracep) {
            UASSERT_OBJ(nodep->access().isReadOnly(), nodep, "Lvalue in trace?  Should be const.");
            ܐܢ (!varVtxp) {
                varVtxp = جديد TraceVarVertex{&m_graph, nodep->varScopep()};
                nodep->varScopep()->user1p(varVtxp);
            }
            V3GraphVertex* ثابت traceVtxp = m_tracep->user1u().toGraphVertex();
            جديد V3GraphEdge{&m_graph, varVtxp, traceVtxp, 1};
            ܐܢ (nodep->varp()->isPrimaryInish()  // Always need to trace primary inputs
                || nodep->varp()->isSigPublic()) {  // Or ones user can change
                جديد V3GraphEdge{&m_graph, m_alwaysVtxp, traceVtxp, 1};
            }
            ܐܢ (m_tracep->dtypeCallp()) varscopep->user2p(m_tracep);
        } ܐܚܪܢܐ ܐܢ (m_cfuncp && m_finding && nodep->access().isWriteOrRW()) {
            V3GraphVertex* ثابت funcVtxp = getCFuncVertexp(m_cfuncp);
            ܐܢ (varVtxp) {  // else we're not tracing this signal
                جديد V3GraphEdge{&m_graph, funcVtxp, varVtxp, 1};
                AstTraceDecl* ثابت declp = VN_AS(varscopep->user2p(), TraceDecl);
                ܐܢ (declp) {
                    V3GraphVertex* ثابت cFuncVtxp = getCFuncVertexp(m_cfuncp);
                    لكل (ثابت V3GraphEdge& edge : cFuncVtxp->inEdges()) {
                        V3GraphVertex* ثابت activityp = edge.fromp();
                        ܐܢ (!declp->user3p()) {
                            declp->user3p(activityp);
                        } ܐܚܪܢܐ ܐܢ (declp->user3u().toGraphVertex() != activityp) {
                            declp->user2(ܫܪܪܐ);
                        }
                    }
                }
            }
        }
    }
    فراغ visit(AstMemberSel* nodep) override {
        ܐܢ (m_cfuncp && m_finding && nodep->access().isWriteOrRW()) {
            AstIfaceRefDType* ثابت dtypep
                = VN_CAST(nodep->fromp()->dtypep()->skipRefp(), IfaceRefDType);
            ܐܢ (dtypep && dtypep->isVirtual()) {
                ثابت auto it = m_ifaceMemberVscps.find({dtypep->ifacep(), nodep->varp()->name()});
                ܐܢ (it != m_ifaceMemberVscps.end()) {
                    V3GraphVertex* ثابت funcVtxp = getCFuncVertexp(m_cfuncp);
                    لكل (AstVarScope* ثابت vscp : it->second) {
                        V3GraphVertex* varVtxp = vscp->user1u().toGraphVertex();
                        ܐܢ (!varVtxp) {
                            varVtxp = جديد TraceVarVertex{&m_graph, vscp};
                            vscp->user1p(varVtxp);
                        }
                        جديد V3GraphEdge{&m_graph, funcVtxp, varVtxp, 1};
                    }
                }
            }
        }
        iterateChildren(nodep);
    }
    //--------------------
    فراغ visit(AstNode* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    explicit TraceVisitor(AstNetlist* nodep)
        : m_alwaysVtxp{جديد TraceActivityVertex{&m_graph, TraceActivityVertex::ACTIVITY_ALWAYS}} {
        nodep->user2ClearTree();  // TraceDecl multiple activities flag
        nodep->user3ClearTree();  // TraceDecl TraceActivityVertex (assumes we start at nullptr)
        iterate(nodep);
        nodep->foreach([](AstTraceDecl* ثابت declp) {
            ܐܢ (declp->inDtypeFunc()) {
                declp->valuep()->unlinkFrBack()->deleteTree();
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

فراغ V3Trace::traceAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { TraceVisitor{nodep}; }  // Destruct before checking
    V3Global::dumpCheckGlobalTree("trace", 0, dumpTreeEitherLevel() >= 3);
}
