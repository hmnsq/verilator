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
    Ast_تۈگۈن* ثابت m_insertp;
    int32_t m_activityCode;
    منطقي m_slow;  // If always slow, we can use the same code
عام:
    تعداد { ACTIVITY_NEVER = ((1UL << 31) - 1) };
    تعداد { ACTIVITY_ALWAYS = ((1UL << 31) - 2) };
    تعداد { ACTIVITY_SLOW = 0 };
    TraceActivityVertex(V3Graph* graphp, Ast_تۈگۈن* nodep, منطقي slow)
        : V3GraphVertex{graphp}
        , m_insertp{nodep} {
        m_activityCode = 0;
        m_slow = slow;
    }
    TraceActivityVertex(V3Graph* graphp, int32_t code)
        : V3GraphVertex{graphp}
        , m_insertp{nullptr} {
        m_activityCode = code;
        m_slow = خطأ;
    }
    ~TraceActivityVertex() override = افتراضي;
    // ACCESSORS
    Ast_تۈگۈن* insertp() ثابت {
        UASSERT(m_insertp, "Null insertp; probably called on a special always/slow");
        أرجع m_insertp;
    }
    string name() ثابت override {
        إذا (activityAlways()) {
            أرجع "*ALWAYS*";
        } وإلا {
            أرجع std::string{slow() ? "*SLOW* " : ""} + insertp()->name();
        }
    }
    string dotColor() ثابت override { أرجع slow() ? "yellowGreen" : "green"; }
    int32_t activityCode() ثابت { أرجع m_activityCode; }
    منطقي activityAlways() ثابت { أرجع activityCode() == ACTIVITY_ALWAYS; }
    منطقي activitySlow() ثابت { أرجع activityCode() == ACTIVITY_SLOW; }
    فراغ activityCode(int32_t code) { m_activityCode = code; }
    منطقي slow() ثابت { أرجع m_slow; }
    فراغ slow(منطقي flag) {
        إذا (!flag) m_slow = خطأ;
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
    AstCFunc* nodep() ثابت { أرجع m_nodep; }
    string name() ثابت override { أرجع nodep()->name(); }
    string dotColor() ثابت override { أرجع "yellow"; }
    File_قۇر* fileline() ثابت override { أرجع nodep()->fileline(); }
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
    AstTraceDecl* nodep() ثابت { أرجع m_nodep; }
    string name() ثابت override { أرجع nodep()->name(); }
    string dotColor() ثابت override { أرجع "red"; }
    File_قۇر* fileline() ثابت override { أرجع nodep()->fileline(); }
    TraceTraceVertex* duplicatep() ثابت { أرجع m_duplicatep; }
    فراغ duplicatep(TraceTraceVertex* dupp) {
        UASSERT_OBJ(!duplicatep(), nodep(), "Assigning duplicatep() to already duplicated node");
        m_duplicatep = dupp;
    }
    فراغ redirectDuplicatep(TraceTraceVertex* dupp) { m_duplicatep = dupp; }
    uint32_t dtypeAliasOffset() ثابت { أرجع m_dtypeAliasOffset; }
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
    AstVarScope* nodep() ثابت { أرجع m_nodep; }
    string name() ثابت override { أرجع nodep()->name(); }
    string dotColor() ثابت override { أرجع "skyblue"; }
    File_قۇر* fileline() ثابت override { أرجع nodep()->fileline(); }
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
    Ast_تۈگۈن_Module* m_چوققا_Modp = nullptr;  // Module to add variables to
    AstScope* ثابت m_چوققا_Scopep = v3Global.rootp()->چوققا_Scopep()->scopep();  // The top AstScope
    AstCFunc* m_cfuncp = nullptr;  // C function adding to graph
    AstCFunc* m_regFuncp = nullptr;  // Trace registration function
    AstCFunc* m_actAllFuncp = nullptr;  // Set all activity function
    AstTraceDecl* m_tracep = nullptr;  // Trace function adding to graph
    AstVarScope* m_activityVscp = nullptr;  // Activity variable
    uint32_t m_activity_سان = 0;  // Count of fields in activity variable
    uint32_t m_code = 0;  // Trace ident code# being assigned
    V3Graph m_graph;  // Var/CFunc tracking
    TraceActivityVertex* ثابت m_alwaysVtxp;  // "Always trace" vertex
    منطقي m_finding = خطأ;  // Pass one of algorithm?
    بنية DtypeFuncs final {
    عام:
        AstCFunc* fullFuncp = nullptr;
        AstCFunc* chgFuncp = nullptr;
    };
    std::unordered_map<ثابت Ast_تۈگۈن_DType*, DtypeFuncs>
        m_dtypeNonConstFuncs;  // Full / Chg funcs per type
    std::unordered_map<ثابت Ast_تۈگۈن_DType*, AstCFunc*> m_dtypeConstFuncs;  // Const func per type
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

        فراغ visit(AstTraceDecl* nodep) override { m_declps.ئىتتىرىش_back(nodep); }
        فراغ visit(AstCCall* nodep) override {
            إذا (AstCFunc* ثابت funcp = nodep->funcp()) collect(funcp);
        }
        فراغ visit(Ast_تۈگۈن* nodep) override { iterateChildren(nodep); }

    عام:
        explicit TraceInitDeclCollector(std::vector<AstTraceDecl*>& declps)
            : m_declps{declps} {}
        فراغ collect(AstCFunc* funcp) {
            إذا (funcp && m_seenFuncps.قىستۇرۇش(funcp).second) iterate(funcp);
        }
    };

    // METHODS

    ساكن منطقي same_يىلتىز_Init_Alias(ثابت AstTraceDecl* يىلتىز_Declp, ثابت AstTraceDecl* چوققا_Declp) {
        ثابت VNumRange& يىلتىز_Bit_Range = يىلتىز_Declp->bitRange();
        ثابت VNumRange& چوققا_Bit_Range = چوققا_Declp->bitRange();
        أرجع يىلتىز_Declp->showname() == چوققا_Declp->showname()
               && يىلتىز_Declp->declDirection() == چوققا_Declp->declDirection()
               && يىلتىز_Declp->widthMin() == چوققا_Declp->widthMin()
               && يىلتىز_Bit_Range.ranged() == چوققا_Bit_Range.ranged()
               && (!يىلتىز_Bit_Range.ranged()
                   || (يىلتىز_Bit_Range.سول() == چوققا_Bit_Range.سول()
                       && يىلتىز_Bit_Range.ئوڭ() == چوققا_Bit_Range.ئوڭ()));
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
            إذا (TraceTraceVertex* ثابت vvertexp = vtx.cast<TraceTraceVertex>()) {
                AstTraceDecl* ثابت nodep = vvertexp->nodep();
                إذا (nodep->dtypeCallp()) استمر;

                إذا (nodep->dtypeDeclp()) {
                    uint32_t& offset = runningOffset[nodep->dtypeDeclp()];
                    memberOffsets[nodep] = offset;
                    offset += nodep->codeInc();
                }

                UASSERT_OBJ(!vvertexp->duplicatep(), nodep, "Should not be a duplicate");
                ثابت auto dupit = dupFinder.findDuplicate(nodep);
                إذا (dupit == dupFinder.end()) {
                    dupFinder.قىستۇرۇش(nodep);
                } وإلا {
                    AstTraceDecl* ثابت dupDeclp = VN_AS(dupit->second, TraceDecl);
                    UASSERT_OBJ(dupDeclp, nodep, "Trace duplicate of wrong type");
                    TraceTraceVertex* ثابت dupvertexp
                        = dupDeclp->user1u().toGraphVertex()->cast<TraceTraceVertex>();
                    UINFO(8, "  Orig " << dupDeclp << endl);
                    UINFO(8, "   dup " << nodep << endl);

                    إذا (dupDeclp->dtypeDeclp() && !dupDeclp->dtypeDeclp()->user2()) {
                        AstTraceDecl* ثابت dtypeParentp = dupDeclp->dtypeDeclp();
                        TraceTraceVertex* ثابت dtypeVtxp
                            = dtypeParentp->user1u().toGraphVertex()->cast<TraceTraceVertex>();
                        ثابت auto it2 = memberOffsets.find(dupDeclp);
                        UASSERT_OBJ(it2 != memberOffsets.end(), dupDeclp,
                                    "Member offset not precomputed");
                        vvertexp->duplicatep(dtypeVtxp);
                        vvertexp->dtypeAliasOffset(it2->second);
                        dtypeParentRedirects.emplace_back(vvertexp, dupvertexp);
                    } وإلا {
                        vvertexp->duplicatep(dupvertexp);
                    }
                    إذا (nodep->dtypeDeclp()) nodep->dtypeDeclp()->user2(true);
                }
            }
        }

        لكل (ثابت auto& pair : dtypeParentRedirects) {
            TraceTraceVertex* ثابت dupVtxp = pair.first;
            TraceTraceVertex* ثابت memberVtxp = pair.second;
            ثابت AstTraceDecl* ثابت parentDeclp = dupVtxp->duplicatep()->nodep();
            إذا (parentDeclp->user2()) {
                dupVtxp->redirectDuplicatep(memberVtxp);
                dupVtxp->dtypeAliasOffset(0);
            }
        }

        إذا (dumpLevel() || debug() >= 9)
            dupFinder.dumpFile(v3Global.debugFilename("trace") + ".hash", خطأ);
    }

    فراغ graphDtypePrune() {
        لكل (V3GraphVertex* ثابت vtxp : m_graph.vertices().unlinkable()) {
            إذا (TraceTraceVertex* ثابت vvertexp = vtxp->cast<TraceTraceVertex>()) {
                AstTraceDecl* ثابت declp = vvertexp->nodep();
                // This skips the dtype sub-func optimization if a var is affected by multiple
                // activities.  We really only need to do this for _chg funcs (and not decls,
                // _const and _full) but it's simpiler to do it all one way or the other.
                إذا (declp) {
                    إذا (declp->user2() || (declp->dtypeDeclp() && !declp->dtypeDeclp()->user2())) {
                        AstCCall* ثابت callp = declp->dtypeCallp();
                        إذا (callp) {
                            Ast_تۈگۈن* stmtexprp = callp->backp();
                            VL_DO_DANGLING(ئىتتىرىش_Deletep(stmtexprp->unlinkFrBack()), stmtexprp);
                        } وإلا {
                            منطقي emptyScope;
                            افعل {
                                emptyScope = خطأ;
                                Ast_تۈگۈن* ثابت declBackp = declp->backp();
                                Ast_تۈگۈن* ثابت declNextp = declp->nextp();
                                إذا (VN_IS(declBackp, Trace_ئىتتىرىش_Prefix)
                                    && VN_IS(declNextp, Trace_چىقىرىش_Prefix)) {
                                    VL_DO_DANGLING(ئىتتىرىش_Deletep(declBackp->unlinkFrBack()),
                                                   declBackp);
                                    VL_DO_DANGLING(ئىتتىرىش_Deletep(declNextp->unlinkFrBack()),
                                                   declNextp);
                                    emptyScope = true;
                                }
                            } طالما (emptyScope);
                        }
                        // Can't purge until we finish this pass
                        ئىتتىرىش_Deletep(declp->unlinkFrBack());  // declp used below
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
        إذا (splitLimit == std::numeric_limits<صحيح>::max()) أرجع;

        لكل (Ast_تۈگۈن* nodep = m_چوققا_Scopep->blocksp(); nodep; nodep = nodep->nextp()) {
            AstCFunc* ثابت funcp = VN_CAST(nodep, CFunc);
            إذا (!funcp || !funcp->user2()) استمر;

            ثابت string baseName = funcp->name();
            uint32_t sub_رەقەم = 1;
            صحيح curSize = 0;
            AstCFunc* curFuncp = funcp;
            std::vector<Ast_تۈگۈن_Stmt*> callStmts;

            auto startNewSubFunc = [&]() {
                File_قۇر* ثابت flp = funcp->fileline();
                ثابت string newName = baseName + "_" + cvtToStr(sub_رەقەم++);
                AstCFunc* ثابت newFuncp = جديد AstCFunc{flp, newName, m_چوققا_Scopep};
                newFuncp->argTypes(v3Global.opt.traceClassBase() + "* tracep");
                newFuncp->isTrace(true);
                newFuncp->isStatic(خطأ);
                newFuncp->isLoose(true);
                newFuncp->slow(true);
                newFuncp->dontCombine(true);
                m_چوققا_Scopep->addBlocksp(newFuncp);
                newFuncp->addStmtsp(جديد AstCStmt{flp, "const int c = vlSymsp->__Vm_baseCode;"});
                AstCCall* ثابت callp = جديد AstCCall{flp, newFuncp};
                callp->dtypeSetVoid();
                callp->argTypes("tracep");
                callStmts.ئىتتىرىش_back(callp->makeStmt());
                curFuncp = newFuncp;
                curSize = 0;
            };

            لكل (Ast_تۈگۈن *stmtp = funcp->stmtsp(), *nextp; stmtp; stmtp = nextp) {
                nextp = stmtp->nextp();
                curSize += stmtp->تۈگۈن_سانى();

                إذا (curFuncp != funcp) {
                    stmtp->unlinkFrBack();
                    curFuncp->addStmtsp(stmtp);
                }

                إذا (curSize > splitLimit && nextp) startNewSubFunc();
            }

            لكل (Ast_تۈگۈن_Stmt* ثابت callStmtp : callStmts) { funcp->addStmtsp(callStmtp); }
        }
    }

    فراغ graphSimplify(منطقي initial) {
        إذا (initial) {
            // Remove all variable nodes
            لكل (V3GraphVertex* ثابت vtxp : m_graph.vertices().unlinkable()) {
                إذا (TraceVarVertex* ثابت vvertexp = vtxp->cast<TraceVarVertex>()) {
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
                إذا (TraceCFuncVertex* ثابت vvertexp = vtxp->cast<TraceCFuncVertex>()) {
                    vvertexp->rerouteEdges(&m_graph);
                    vvertexp->unlinkDelete(&m_graph);
                }
            }
        }

        // Remove multiple variables connecting funcs to traces
        m_graph.removeRedundantEdgesMax(&V3GraphEdge::followAlwaysTrue);

        // If there are any edges from a always, keep only the always
        لكل (V3GraphVertex& vtx : m_graph.vertices()) {
            إذا (TraceTraceVertex* ثابت vvertexp = vtx.cast<TraceTraceVertex>()) {
                // Search for the incoming always edge
                ثابت V3GraphEdge* alwaysEdgep = nullptr;
                لكل (ثابت V3GraphEdge& edge : vvertexp->inEdges()) {
                    ثابت TraceActivityVertex* ثابت actVtxp
                        = edge.fromp()->as<ثابت TraceActivityVertex>();
                    إذا (actVtxp->activityAlways()) {
                        alwaysEdgep = &edge;
                        اكسر;
                    }
                }
                // If always edge exists, remove all other edges
                إذا (alwaysEdgep) {
                    لكل (V3GraphEdge* ثابت edgep : vvertexp->inEdges().unlinkable()) {
                        إذا (edgep != alwaysEdgep) VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
                    }
                }
            }
        }

        // Activity points with no outputs can be removed
        لكل (V3GraphVertex* ثابت vtxp : m_graph.vertices().unlinkable()) {
            إذا (TraceActivityVertex* ثابت aVtxp = vtxp->cast<TraceActivityVertex>()) {
                // Leave in the always vertex for later use.
                إذا (aVtxp != m_alwaysVtxp && aVtxp->outEmpty()) {
                    VL_DO_DANGLING(aVtxp->unlinkDelete(&m_graph), aVtxp);
                }
            }
        }
    }

    uint32_t assignactivityNumbers() {
        uint32_t activity_سان = 1;  // Note 0 indicates "slow" only
        لكل (V3GraphVertex& vtx : m_graph.vertices()) {
            إذا (TraceActivityVertex* ثابت vvertexp = vtx.cast<TraceActivityVertex>()) {
                إذا (vvertexp != m_alwaysVtxp) {
                    إذا (vvertexp->slow()) {
                        vvertexp->activityCode(TraceActivityVertex::ACTIVITY_SLOW);
                    } وإلا {
                        vvertexp->activityCode(activity_سان++);
                    }
                }
            }
        }
        أرجع activity_سان;
    }

    فراغ sortTraces(TraceVec& traces, uint32_t& nNonConstCodes) {
        // Populate sort structure
        traces.clear();
        nNonConstCodes = 0;
        لكل (V3GraphVertex& vtx : m_graph.vertices()) {
            إذا (TraceTraceVertex* ثابت vtxp = vtx.cast<TraceTraceVertex>()) {
                ActCodeSet actSet;
                UINFO(9, "  Add to sort: " << vtxp);
                UINFOTREE(9, vtxp->nodep(), "", "trnode");
                لكل (ثابت V3GraphEdge& edge : vtxp->inEdges()) {
                    ثابت TraceActivityVertex* ثابت cfvertexp
                        = edge.fromp()->cast<ثابت TraceActivityVertex>();
                    UASSERT_OBJ(cfvertexp, vtxp->nodep(),
                                "Should have been function pointing to this trace");
                    UINFO(9, "   Activity: " << cfvertexp);
                    إذا (cfvertexp->activityAlways()) {
                        // If code 0, we always trace; ignore other codes
                        actSet.قىستۇرۇش(TraceActivityVertex::ACTIVITY_ALWAYS);
                    } وإلا {
                        actSet.قىستۇرۇش(cfvertexp->activityCode());
                    }
                }
                UASSERT_OBJ(actSet.سانى(TraceActivityVertex::ACTIVITY_ALWAYS) == 0
                                || actSet.size() == 1,
                            vtxp->nodep(), "Always active trace has further triggers");
                // Count nodes
                إذا (!vtxp->duplicatep() && !actSet.empty())
                    nNonConstCodes += vtxp->nodep()->codeInc();
                إذا (actSet.empty()) {
                    // If a trace doesn't have activity, it's constant, and we
                    // don't need to track changes on it.
                    actSet.قىستۇرۇش(TraceActivityVertex::ACTIVITY_NEVER);
                } وإلا إذا (actSet.سانى(TraceActivityVertex::ACTIVITY_SLOW) && actSet.size() > 1) {
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
        طالما (it != end) {
            auto head = it;
            // Approximate the complexity of the value change check
            uint32_t complexity = 0;
            ثابت ActCodeSet& actSet = it->first;
            لكل (; it != end && it->first == actSet; ++it) {
                إذا (!it->second->duplicatep()) {
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
            إذا (actSet.سانى(TraceActivityVertex::ACTIVITY_ALWAYS)) استمر;
            إذا (actSet.سانى(TraceActivityVertex::ACTIVITY_NEVER)) استمر;
            إذا (actSet.سانى(TraceActivityVertex::ACTIVITY_SLOW)) استمر;
            // If the value comparisons are cheaper to perform than checking the
            // activity flags make the signals always traced. Note this cost
            // equation is heuristic.
            إذا (complexity <= actSet.size() * 2) {
                لكل (; head != it; ++head) {
                    جديد V3GraphEdge{&m_graph, m_alwaysVtxp, head->second, 1};
                }
            }
        }

        graphSimplify(خطأ);
    }

    Ast_تۈگۈن_ئىپادە* selectActivity(File_قۇر* flp, uint32_t acode, ثابت VAccess& access) {
        أرجع جديد AstArraySel(flp, جديد AstVarRef{flp, m_activityVscp, access}, acode);
    }

    Ast_تۈگۈن* newActivitySetter(Ast_تۈگۈن* insertp, uint32_t code) {
        ++m_statSetters;
        File_قۇر* ثابت fl = insertp->fileline();
        AstAssign* ثابت setterp = جديد AstAssign{fl, selectActivity(fl, code, VAccess::WRITE),
                                                 جديد AstConst{fl, AstConst::BitTrue{}}};
        أرجع setterp;
    }

    Ast_تۈگۈن* newActivityAll(Ast_تۈگۈن* insertp) {
        ++m_statSettersSlow;
        إذا (!m_actAllFuncp) {
            File_قۇر* ثابت flp = m_چوققا_Scopep->fileline();
            AstCFunc* ثابت funcp = جديد AstCFunc{flp, "__VtraceActivitySetAll", m_چوققا_Scopep};
            funcp->slow(true);
            funcp->isStatic(خطأ);
            funcp->isLoose(true);
            m_چوققا_Scopep->addBlocksp(funcp);
            لكل (uint32_t code = 0; code < m_activity_سان; ++code) {
                Ast_تۈگۈن* ثابت setterp = newActivitySetter(insertp, code);
                funcp->addStmtsp(setterp);
            }
            m_actAllFuncp = funcp;
        }
        AstCCall* ثابت callp = جديد AstCCall{insertp->fileline(), m_actAllFuncp};
        callp->dtypeSetVoid();
        أرجع callp->makeStmt();
    }

    فراغ createActivityFlags() {
        // Assign final activity numbers
        m_activity_سان = assignactivityNumbers();

        // Create an array of bytes, not a bit vector, as they can be set
        // atomically by mtasks, and are cheaper to set (no need for
        // read-modify-write on the C type), and the speed of the tracing code
        // is the same on largish designs.
        File_قۇر* ثابت flp = m_چوققا_Scopep->fileline();
        Ast_تۈگۈن_DType* ثابت newScalarDtp = جديد AstBasicDType{flp, VFlagBitPacked{}, 1};
        v3Global.rootp()->typeTablep()->addTypesp(newScalarDtp);
        AstRange* ثابت newArange
            = جديد AstRange{flp, VNumRange{static_cast<صحيح>(m_activity_سان) - 1, 0}};
        Ast_تۈگۈن_DType* ثابت newArrDtp = جديد AstUnpackArrayDType{flp, newScalarDtp, newArange};
        v3Global.rootp()->typeTablep()->addTypesp(newArrDtp);
        m_activityVscp = v3Global.rootp()->چوققا_Scopep()->createTemp("__VtraceActivity", newArrDtp);

        // Insert activity setters
        لكل (ثابت V3GraphVertex& vtx : m_graph.vertices()) {
            إذا (ثابت TraceActivityVertex* ثابت vtxp = vtx.cast<ثابت TraceActivityVertex>()) {
                Ast_تۈگۈن* setterp = nullptr;
                إذا (vtxp->activitySlow()) {
                    setterp = newActivityAll(vtxp->insertp());
                    // Just set all flags in slow code as it should be rare.
                    // This will be rolled up into a loop by V3Reloop.
                } وإلا إذا (!vtxp->activityAlways()) {
                    setterp = newActivitySetter(vtxp->insertp(), vtxp->activityCode());
                }
                إذا (setterp) {
                    Ast_تۈگۈن* ثابت insertp = vtxp->insertp();
                    إذا (Ast_Stmt_ئىپادە* ثابت stmtp = VN_CAST(insertp, Stmt_ئىپادە)) {
                        stmtp->addNextHere(setterp);
                    } وإلا إذا (AstCFunc* ثابت funcp = VN_CAST(insertp, CFunc)) {
                        // If there are awaits, insert the setter after each await
                        إذا (funcp->isCoroutine() && funcp->stmtsp()) {
                            funcp->stmtsp()->foreachAndNext([setterp](AstCAwait* awaitp) {
                                awaitp->addNextHere(setterp->clone_دەرەخ(خطأ));
                            });
                        }
                        funcp->addStmtsp(setterp);
                    } وإلا {
                        insertp->v3fatalSrc("Bad trace activity vertex");
                    }
                }
            }
        }
    }

    AstCFunc* newCFunc(VTraceType traceType, AstCFunc* چوققا_Funcp, uint32_t func_رەقەم,
                       uint32_t baseCode = 0, ثابت AstTraceDecl* ثابت declp = nullptr,
                       منطقي declSub = خطأ) {
        // Create new function
        ثابت منطقي is_چوققا_Func = !declp && چوققا_Funcp == nullptr;
        std::string funcName;
        إذا (is_چوققا_Func || declp) {
            funcName = traceType.func_prefix();
        } وإلا {
            funcName = چوققا_Funcp->name();
            funcName += "_sub";
        }
        إذا (declp) {
            funcName += "_dtype";
            إذا (declSub) funcName += "_sub";
            funcName += "__";
            funcName = m_dtypeNames.get(funcName);
        } وإلا {
            funcName += "_";
            funcName += cvtToStr(func_رەقەم);
        }

        File_قۇر* ثابت flp = m_چوققا_Scopep->fileline();
        AstCFunc* ثابت funcp = جديد AstCFunc{flp, funcName, m_چوققا_Scopep};
        funcp->isTrace(true);
        funcp->dontCombine(true);
        funcp->isLoose(true);
        funcp->slow(traceType != VTraceType::CHANGE);
        funcp->isStatic(is_چوققا_Func);
        // Add it to top scope
        m_چوققا_Scopep->addBlocksp(funcp);
        ثابت std::string bufArg
            = v3Global.opt.traceClassBase() + "::Buffer* bufp"
              + (declp ? (", uint32_t offset, const " + declp->dtypep()->cType("", true, true)
                          + " __VdtypeVar")
                       : "");
        إذا (is_چوققا_Func) {
            // Top functions
            funcp->argTypes("void* voidSelf, " + bufArg);
            funcp->addStmtsp(جديد AstCStmt{flp, EmitCUtil::voidSelfAssign(m_چوققا_Modp)});
            funcp->addStmtsp(جديد AstCStmt{flp, EmitCUtil::symClassAssign()});
            // Add global activity check to change dump functions
            إذا (traceType == VTraceType::CHANGE) {  //
                funcp->addStmtsp(
                    جديد AstCStmt{flp, "if (VL_UNLIKELY(!vlSymsp->__Vm_activity)) return;"});
            }
            // Register function
            AstCStmt* ثابت cstmtp = جديد AstCStmt{flp};
            m_regFuncp->addStmtsp(cstmtp);
            إذا (traceType == VTraceType::CONSTANT) {
                cstmtp->add("tracep->addConstCb(");
                cstmtp->add(جديد AstAddrOfCFunc{flp, funcp});
                cstmtp->add(", " + std::to_string(func_رەقەم) + ", vlSelf);");
            } وإلا إذا (traceType == VTraceType::FULL) {
                cstmtp->add("tracep->addFullCb(");
                cstmtp->add(جديد AstAddrOfCFunc{flp, funcp});
                cstmtp->add(", " + std::to_string(func_رەقەم) + ", vlSelf);");
            } وإلا {
                cstmtp->add("tracep->addChgCb(");
                cstmtp->add(جديد AstAddrOfCFunc{flp, funcp});
                cstmtp->add(", " + std::to_string(func_رەقەم) + ", vlSelf);");
            }
        } وإلا {
            // Sub functions
            funcp->argTypes(bufArg);
            // Setup base references. Note in rare occasions we can end up with an empty trace
            // sub function, hence the VL_ATTR_UNUSED attributes.
            إذا (traceType != VTraceType::CHANGE) {
                // Full dump sub function
                funcp->addStmtsp(جديد AstCStmt{flp,  //
                                              string("uint32_t* const oldp VL_ATTR_UNUSED = "
                                                     "bufp->oldp(vlSymsp->__Vm_baseCode")
                                                  + (declp ? " + offset" : "") + ");\n"});
            } وإلا {
                // Change dump sub function
                funcp->addStmtsp(جديد AstCStmt{flp,  //
                                              "uint32_t* const oldp VL_ATTR_UNUSED = "
                                              "bufp->oldp(vlSymsp->__Vm_baseCode + "
                                                  + (declp ? " offset" : cvtToStr(baseCode))
                                                  + ");\n"});
            }
            إذا (!declp) {
                // Add call to top function
                AstCCall* ثابت callp = جديد AstCCall{funcp->fileline(), funcp};
                callp->dtypeSetVoid();
                callp->argTypes("bufp");
                چوققا_Funcp->addStmtsp(callp->makeStmt());
            }
        }
        // Done
        UINFO(5, "  newCFunc " << funcp);
        أرجع funcp;
    }

    AstCFunc* newCDtypeSubFunc(VTraceType traceType, ثابت AstTraceDecl* ثابت declp,
                               AstCFunc* parentp) {
        AstCFunc* ثابت funcp = newCFunc(traceType, nullptr, 0, 0, declp, true);
        File_قۇر* ثابت flp = m_چوققا_Scopep->fileline();
        AstCCall* ثابت callp = جديد AstCCall{flp, funcp};
        callp->dtypeSetVoid();
        callp->argTypes("bufp, offset, __VdtypeVar");
        parentp->addStmtsp(callp->makeStmt());
        أرجع funcp;
    }

    AstCFunc* createConstDtypeTraceFunctions(ثابت AstTraceDecl* declp) {
        ثابت Ast_تۈگۈن_DType* ثابت dtypep = declp->dtypep()->skipRefp();
        auto pair = m_dtypeConstFuncs.emplace(dtypep, nullptr);
        إذا (pair.second) {
            File_قۇر* ثابت flp = declp->fileline();
            AstCFunc* ثابت funcp = newCFunc(VTraceType::CONSTANT, nullptr, 0, 0, declp);

            منطقي first = true;
            لكل (Ast_تۈگۈن* callStmtp = declp->dtypeCallp()->funcp()->stmtsp(); callStmtp;
                 callStmtp = callStmtp->nextp()) {
                ثابت AstCCall* ثابت callp
                    = VN_CAST(VN_CAST(callStmtp, Stmt_ئىپادە)->exprp(), CCall);
                منطقي onlyOne = first && !callStmtp->nextp();
                AstCFunc* ثابت subFuncp
                    = onlyOne ? funcp : newCDtypeSubFunc(VTraceType::CONSTANT, declp, funcp);
                لكل (Ast_تۈگۈن* stmtp = callp->funcp()->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    إذا (AstTraceDecl* ثابت fieldDeclp = VN_CAST(stmtp, TraceDecl)) {
                        AstTraceInc* ثابت incp
                            = جديد AstTraceInc{flp, fieldDeclp, VTraceType::CONSTANT};
                        subFuncp->addStmtsp(incp);
                    }
                }
                first = خطأ;
            }

            pair.first->second = funcp;
        }

        أرجع pair.first->second;
    }

    فراغ createConstTraceFunctions(ثابت TraceVec& traces) {
        ثابت صحيح splitLimit = v3Global.opt.outputSplitCTrace() ? v3Global.opt.outputSplitCTrace()
                                                                : std::numeric_limits<صحيح>::max();

        AstCFunc* ثابت چوققا_Funcp = newCFunc(VTraceType::CONSTANT, nullptr, 0);
        uint32_t sub_Func_رەقەم = 0;
        AstCFunc* subFuncp = nullptr;
        صحيح subStmts = 0;
        std::vector<ثابت TraceTraceVertex*> duplicates;
        لكل (auto it = traces.cbegin(); it != traces.end(); ++it) {
            ثابت TraceTraceVertex* ثابت vtxp = it->second;
            AstTraceDecl* ثابت declp = vtxp->nodep();
            إذا (vtxp->duplicatep()) {
                duplicates.ئىتتىرىش_back(vtxp);
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
            إذا (actSet.سانى(TraceActivityVertex::ACTIVITY_NEVER)) {
                // Crate new sub function if required
                إذا (!subFuncp || subStmts > splitLimit) {
                    subStmts = 0;
                    subFuncp = newCFunc(VTraceType::CONSTANT, چوققا_Funcp, sub_Func_رەقەم);
                    ++sub_Func_رەقەم;
                }
                File_قۇر* ثابت flp = declp->fileline();
                إذا (declp->dtypeCallp()) {
                    AstCFunc* ثابت funcp = createConstDtypeTraceFunctions(declp);
                    Ast_تۈگۈن_ئىپادە* argsp = nullptr;
                    argsp = Ast_تۈگۈن::addNext(argsp, declp->valuep()->clone_دەرەخ(خطأ));
                    AstCCall* ثابت callp = جديد AstCCall{flp, funcp, argsp};
                    callp->dtypeSetVoid();
                    callp->argTypes(callp->argTypes() + "bufp, " + std::to_string(declp->code()));
                    subFuncp->addStmtsp(callp->makeStmt());

                    subStmts += 1;
                } وإلا {
                    AstTraceInc* ثابت incp = جديد AstTraceInc{flp, declp, VTraceType::CONSTANT};
                    subFuncp->addStmtsp(incp);
                    subStmts += incp->تۈگۈن_سانى();
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
        Ast_تۈگۈن_DType* dtypep = declp->dtypep()->skipRefp();
        auto pair = m_dtypeNonConstFuncs.emplace(dtypep, DtypeFuncs{});
        إذا (pair.second) {
            File_قۇر* ثابت flp = declp->fileline();
            AstCFunc* ثابت fullFuncp = newCFunc(VTraceType::FULL, nullptr, 0, 0, declp);
            AstCFunc* ثابت chgFuncp = newCFunc(VTraceType::CHANGE, nullptr, 0, 0, declp);

            منطقي first = true;
            لكل (Ast_تۈگۈن* callStmtp = declp->dtypeCallp()->funcp()->stmtsp(); callStmtp;
                 callStmtp = callStmtp->nextp()) {
                ثابت AstCCall* ثابت callp
                    = VN_CAST(VN_CAST(callStmtp, Stmt_ئىپادە)->exprp(), CCall);
                منطقي onlyOne = first && !callStmtp->nextp();
                AstCFunc* ثابت fullSubFuncp
                    = onlyOne ? fullFuncp : newCDtypeSubFunc(VTraceType::FULL, declp, fullFuncp);
                AstCFunc* ثابت chgSubFuncp
                    = onlyOne ? chgFuncp : newCDtypeSubFunc(VTraceType::CHANGE, declp, chgFuncp);
                لكل (Ast_تۈگۈن* stmtp = callp->funcp()->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    إذا (AstTraceDecl* ثابت fieldDeclp = VN_CAST(stmtp, TraceDecl)) {
                        AstTraceInc* ثابت incFullp
                            = جديد AstTraceInc{flp, fieldDeclp, VTraceType::FULL};
                        fullSubFuncp->addStmtsp(incFullp);
                        AstTraceInc* ثابت incChgp
                            = جديد AstTraceInc{flp, fieldDeclp, VTraceType::CHANGE};
                        chgSubFuncp->addStmtsp(incChgp);
                    }
                }
                first = خطأ;
            }

            pair.first->second = {.fullFuncp = fullFuncp, .chgFuncp = chgFuncp};
        }

        أرجع pair.first->second;
    }

    فراغ createNonConstTraceFunctions(ثابت TraceVec& traces, uint32_t nAllCodes,
                                      uint32_t parallelism) {
        ثابت صحيح splitLimit = v3Global.opt.outputSplitCTrace() ? v3Global.opt.outputSplitCTrace()
                                                                : std::numeric_limits<صحيح>::max();

        // pre-incremented, so starts at 0
        uint32_t چوققا_Func_رەقەم = std::numeric_limits<uint32_t>::max();
        TraceVec::const_iterator it = traces.begin();
        طالما (it != traces.end()) {
            AstCFunc* چوققا_Ful_Funcp = nullptr;
            AstCFunc* چوققا_Chg_Funcp = nullptr;
            AstCFunc* subFulFuncp = nullptr;
            AstCFunc* subChgFuncp = nullptr;
            uint32_t sub_Func_رەقەم = 0;
            صحيح subStmts = 0;
            ثابت uint32_t maxCodes = std::max((nAllCodes + parallelism - 1) / parallelism, 1U);
            uint32_t nCodes = 0;
            ثابت ActCodeSet* prevActSet = nullptr;
            AstIf* ifp = nullptr;
            uint32_t baseCode = 0;
            لكل (; nCodes < maxCodes && it != traces.end(); ++it) {
                ثابت ActCodeSet& actSet = it->first;
                // Traced value never changes, no need to add it
                إذا (actSet.سانى(TraceActivityVertex::ACTIVITY_NEVER)) استمر;

                ثابت TraceTraceVertex* ثابت vtxp = it->second;
                AstTraceDecl* ثابت declp = vtxp->nodep();

                // This is a duplicate decl, no need to add it, but must set the
                // function index to the same as the canonical node.
                إذا (ثابت TraceTraceVertex* ثابت canonVtxp = vtxp->duplicatep()) {
                    declp->fidx(canonVtxp->nodep()->fidx());
                    استمر;
                }

                // Create top function if not yet created
                إذا (!چوققا_Ful_Funcp) {
                    ++چوققا_Func_رەقەم;
                    چوققا_Ful_Funcp = newCFunc(VTraceType::FULL, nullptr, چوققا_Func_رەقەم);
                    چوققا_Chg_Funcp = newCFunc(VTraceType::CHANGE, nullptr, چوققا_Func_رەقەم);
                }

                // Create new sub function if required
                إذا (!subFulFuncp || subStmts > splitLimit) {
                    baseCode = declp->code();
                    subStmts = 0;
                    subFulFuncp = newCFunc(VTraceType::FULL, چوققا_Ful_Funcp, sub_Func_رەقەم, baseCode);
                    subChgFuncp = newCFunc(VTraceType::CHANGE, چوققا_Chg_Funcp, sub_Func_رەقەم, baseCode);
                    ++sub_Func_رەقەم;
                    prevActSet = nullptr;
                    ifp = nullptr;
                }

                // If required, create the conditional node checking the activity flags
                إذا (!prevActSet || actSet != *prevActSet) {
                    File_قۇر* ثابت flp = m_چوققا_Scopep->fileline();
                    ثابت منطقي always = actSet.سانى(TraceActivityVertex::ACTIVITY_ALWAYS) != 0;
                    Ast_تۈگۈن_ئىپادە* condp = nullptr;
                    إذا (always) {
                        condp = جديد AstConst{flp, 1};  // Always true, will be folded later
                    } وإلا {
                        لكل (ثابت uint32_t actCode : actSet) {
                            Ast_تۈگۈن_ئىپادە* ثابت selp = selectActivity(flp, actCode, VAccess::READ);
                            condp = condp ? جديد AstOr{flp, condp, selp} : selp;
                        }
                    }
                    ifp = جديد AstIf{flp, condp};
                    إذا (!always) ifp->branchPred(VBranchPred::BP_UNLIKELY);
                    subChgFuncp->addStmtsp(ifp);
                    subStmts += ifp->تۈگۈن_سانى();
                    prevActSet = &actSet;
                }

                // Add TraceInc nodes
                File_قۇر* ثابت flp = declp->fileline();
                إذا (declp->dtypeCallp()) {
                    DtypeFuncs funcs = createNonConstDtypeTraceFunctions(declp);
                    Ast_تۈگۈن_ئىپادە* argsp = nullptr;
                    argsp = Ast_تۈگۈن::addNext(argsp, declp->valuep()->clone_دەرەخ(خطأ));
                    AstCCall* ثابت callFullp = جديد AstCCall{flp, funcs.fullFuncp, argsp};
                    callFullp->dtypeSetVoid();
                    callFullp->argTypes(callFullp->argTypes() + "bufp, "
                                        + std::to_string(declp->code()));
                    subFulFuncp->addStmtsp(callFullp->makeStmt());
                    argsp = nullptr;
                    argsp = Ast_تۈگۈن::addNext(argsp, declp->valuep()->clone_دەرەخ(خطأ));
                    AstCCall* ثابت callChgp = جديد AstCCall{flp, funcs.chgFuncp, argsp};
                    callChgp->dtypeSetVoid();
                    callChgp->argTypes(callChgp->argTypes() + "bufp, "
                                       + std::to_string(declp->code()));
                    ifp->addThensp(callChgp->makeStmt());

                    subStmts += 2;
                } وإلا {
                    AstTraceInc* ثابت incFulp = جديد AstTraceInc{flp, declp, VTraceType::FULL};
                    subFulFuncp->addStmtsp(incFulp);
                    AstTraceInc* ثابت incChgp
                        = جديد AstTraceInc{flp, declp, VTraceType::CHANGE, baseCode};
                    ifp->addThensp(incChgp);

                    // Set the function index of the decl
                    declp->fidx(چوققا_Func_رەقەم);

                    // Track splitting due to size
                    UASSERT_OBJ(incFulp->تۈگۈن_سانى() == incChgp->تۈگۈن_سانى(), declp,
                                "Should have equal cost");
                    ثابت VNumRange range = declp->arrayRange();
                    إذا (range.ranged()) {
                        // 2x because each element is a TraceInc and a VarRef
                        subStmts += range.elements() * 2;
                    } وإلا {
                        subStmts += incChgp->تۈگۈن_سانى();
                    }
                }

                // Track partitioning
                nCodes += declp->codeInc();
            }
        }
    }

    فراغ createCleanupFunction() {
        File_قۇر* ثابت fl = m_چوققا_Scopep->fileline();
        AstCFunc* ثابت cleanupFuncp = جديد AstCFunc{fl, "trace_cleanup", m_چوققا_Scopep};
        cleanupFuncp->argTypes("void* voidSelf, " + v3Global.opt.traceClassBase()
                               + "* /*unused*/");
        cleanupFuncp->isTrace(true);
        cleanupFuncp->slow(خطأ);
        cleanupFuncp->isStatic(true);
        cleanupFuncp->isLoose(true);
        m_چوققا_Scopep->addBlocksp(cleanupFuncp);
        cleanupFuncp->addStmtsp(جديد AstCStmt{fl, EmitCUtil::voidSelfAssign(m_چوققا_Modp)});
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
            جديد AstCStmt{m_چوققا_Scopep->fileline(), "vlSymsp->__Vm_activity = false;"s});

        // Clear fine grained activity flags
        لكل (uint32_t i = 0; i < m_activity_سان; ++i) {
            Ast_تۈگۈن* ثابت clrp = جديد AstAssign{fl, selectActivity(fl, i, VAccess::WRITE),
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
        إذا (dumpGraphLevel() >= 6) m_graph.dumpDotFilePrefixed("trace_pre");
        graphSimplify(true);
        إذا (dumpGraphLevel() >= 6) m_graph.dumpDotFilePrefixed("trace_simplified");
        graphOptimize();
        إذا (dumpGraphLevel() >= 6) m_graph.dumpDotFilePrefixed("trace_optimized");

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
        m_regFuncp = جديد AstCFunc{m_چوققا_Scopep->fileline(), "trace_register", m_چوققا_Scopep};
        m_regFuncp->argTypes(v3Global.opt.traceClassBase() + "* tracep");
        m_regFuncp->entryPoint(true);
        m_regFuncp->isTrace(true);
        m_regFuncp->slow(true);
        m_regFuncp->isStatic(خطأ);
        m_regFuncp->isLoose(true);
        m_چوققا_Scopep->addBlocksp(m_regFuncp);

        // Create the const dump functions. Also allocates trace codes.
        createConstTraceFunctions(traces);

        // Create the full and incremental dump functions
        createNonConstTraceFunctions(traces, nNonConstCodes, m_parallelism);

        // Root-traced libraries alias wrapper IOs onto the existing top-module codes.
        إذا (!v3Global.opt.libCreate().empty()) {
            std::vector<AstTraceDecl*> يىلتىز_Declps;
            std::vector<AstTraceDecl*> چوققا_Declps;
            TraceInitDeclCollector يىلتىز_Collector{يىلتىز_Declps};
            TraceInitDeclCollector چوققا_Collector{چوققا_Declps};
            لكل (Ast_تۈگۈن* blockp = m_چوققا_Scopep->blocksp(); blockp; blockp = blockp->nextp()) {
                AstCFunc* ثابت funcp = VN_CAST(blockp, CFunc);
                إذا (funcp && VString::startsWith(funcp->name(), "trace_init_leaf_root__")) {
                    يىلتىز_Collector.collect(funcp);
                } وإلا إذا (funcp && VString::startsWith(funcp->name(), "trace_init_leaf_top__")) {
                    چوققا_Collector.collect(funcp);
                }
            }

            std::vector<منطقي> used(چوققا_Declps.size(), خطأ);
            لكل (AstTraceDecl* ثابت يىلتىز_Declp : يىلتىز_Declps) {
                لكل (size_t i = 0; i < چوققا_Declps.size(); ++i) {
                    إذا (used[i] || !same_يىلتىز_Init_Alias(يىلتىز_Declp, چوققا_Declps[i])) استمر;
                    يىلتىز_Declp->code(چوققا_Declps[i]->code());
                    used[i] = true;
                    اكسر;
                }
            }
        }

        // Remove refs to traced values from TraceDecl nodes, these have now moved under
        // TraceInc
        لكل (ثابت auto& i : traces) {
            Ast_تۈگۈن* ثابت valuep = i.second->nodep()->valuep();
            VL_DO_DANGLING(valuep->unlinkFrBack()->delete_دەرەخ(), valuep);
        }

        // Create the trace cleanup function clearing the activity flags
        createCleanupFunction();
    }

    TraceCFuncVertex* getCFuncVertexp(AstCFunc* nodep) {
        V3GraphVertex* ثابت vtxp = nodep->user1u().toGraphVertex();
        TraceCFuncVertex* vertexp = vtxp ? vtxp->cast<TraceCFuncVertex>() : nullptr;
        إذا (!vertexp) {
            vertexp = جديد TraceCFuncVertex{&m_graph, nodep};
            nodep->user1p(vertexp);
        }
        أرجع vertexp;
    }
    TraceActivityVertex* getActivityVertexp(Ast_تۈگۈن* nodep, منطقي slow) {
        V3GraphVertex* ثابت vtxp = nodep->user3u().toGraphVertex();
        TraceActivityVertex* vertexp = vtxp ? vtxp->cast<TraceActivityVertex>() : nullptr;
        إذا (!vertexp) {
            vertexp = جديد TraceActivityVertex{&m_graph, nodep, slow};
            nodep->user3p(vertexp);
        }
        vertexp->slow(slow);
        أرجع vertexp;
    }

    // VISITORS
    فراغ visit(AstNetlist* nodep) override {
        // Add vertexes for all TraceDecl, and edges from VARs each trace looks at
        m_finding = خطأ;
        iterateChildren(nodep);

        // Add vertexes for all CFUNCs, and edges to VARs the func sets
        m_finding = true;
        iterateChildren(nodep);

        // Create the trace functions and insert them into the tree
        createTraceFunctions();

        // Save number of trace codes used
        nodep->nTraceCodes(m_code);
    }
    فراغ visit(Ast_تۈگۈن_Module* nodep) override {
        إذا (nodep->is_چوققا()) m_چوققا_Modp = nodep;
        iterateChildren(nodep);
    }
    فراغ visit(AstVarScope* nodep) override {
        إذا (!m_finding) {
            إذا (ثابت AstIface* ثابت ifacep = nodep->varp()->sensIfacep()) {
                m_ifaceMemberVscps[{ifacep, nodep->varp()->name()}].ئىتتىرىش_back(nodep);
            }
        }
    }
    فراغ visit(Ast_Stmt_ئىپادە* nodep) override {
        إذا (!m_finding && !nodep->user2()) {
            إذا (AstCCall* ثابت callp = VN_CAST(nodep->exprp(), CCall)) {
                UINFO(8, "   CCALL " << callp);
                // See if there are other calls in same statement list;
                // If so, all funcs might share the same activity code
                TraceActivityVertex* ثابت activityVtxp
                    = getActivityVertexp(nodep, callp->funcp()->slow());
                لكل (Ast_تۈگۈن* nextp = nodep; nextp; nextp = nextp->nextp()) {
                    إذا (Ast_Stmt_ئىپادە* ثابت stmtp = VN_CAST(nextp, Stmt_ئىپادە)) {
                        إذا (AstCCall* ثابت ccallp = VN_CAST(stmtp->exprp(), CCall)) {
                            stmtp->user2(true);  // Processed
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
        إذا (!m_finding) {  // If public, we need a unique activity code to allow for sets
                           // directly in this func
            إذا (nodep->funcPublic() || nodep->dpiExportImpl() || nodep->entryPoint()
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
        إذا (!m_finding && !nodep->inDtypeFunc()) {
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
            إذا (m_cfuncp && VString::startsWith(m_cfuncp->name(), "trace_init_leaf_root__")) {
                أرجع;
            }
            V3GraphVertex* ثابت vertexp = جديد TraceTraceVertex{&m_graph, nodep};
            nodep->user1p(vertexp);

            UASSERT_OBJ(m_cfuncp, nodep, "Trace not under func");
            m_cfuncp->user2(true);
            VL_RESTORER(m_tracep);
            m_tracep = nodep;
            iterateChildren(nodep);
        }
    }
    فراغ visit(AstVarRef* nodep) override {
        UASSERT_OBJ(nodep->varScopep(), nodep, "No var scope?");
        AstVarScope* ثابت varscopep = nodep->varScopep();
        V3GraphVertex* varVtxp = varscopep->user1u().toGraphVertex();
        إذا (m_tracep) {
            UASSERT_OBJ(nodep->access().isReadOnly(), nodep, "Lvalue in trace?  Should be const.");
            إذا (!varVtxp) {
                varVtxp = جديد TraceVarVertex{&m_graph, nodep->varScopep()};
                nodep->varScopep()->user1p(varVtxp);
            }
            V3GraphVertex* ثابت traceVtxp = m_tracep->user1u().toGraphVertex();
            جديد V3GraphEdge{&m_graph, varVtxp, traceVtxp, 1};
            إذا (nodep->varp()->isPrimaryInish()  // Always need to trace primary inputs
                || nodep->varp()->isSigPublic()) {  // Or ones user can change
                جديد V3GraphEdge{&m_graph, m_alwaysVtxp, traceVtxp, 1};
            }
            إذا (m_tracep->dtypeCallp()) varscopep->user2p(m_tracep);
        } وإلا إذا (m_cfuncp && m_finding && nodep->access().isWriteOrRW()) {
            V3GraphVertex* ثابت funcVtxp = getCFuncVertexp(m_cfuncp);
            إذا (varVtxp) {  // else we're not tracing this signal
                جديد V3GraphEdge{&m_graph, funcVtxp, varVtxp, 1};
                AstTraceDecl* ثابت declp = VN_AS(varscopep->user2p(), TraceDecl);
                إذا (declp) {
                    V3GraphVertex* ثابت cFuncVtxp = getCFuncVertexp(m_cfuncp);
                    لكل (ثابت V3GraphEdge& edge : cFuncVtxp->inEdges()) {
                        V3GraphVertex* ثابت activityp = edge.fromp();
                        إذا (!declp->user3p()) {
                            declp->user3p(activityp);
                        } وإلا إذا (declp->user3u().toGraphVertex() != activityp) {
                            declp->user2(true);
                        }
                    }
                }
            }
        }
    }
    فراغ visit(AstMemberSel* nodep) override {
        إذا (m_cfuncp && m_finding && nodep->access().isWriteOrRW()) {
            AstIfaceRefDType* ثابت dtypep
                = VN_CAST(nodep->fromp()->dtypep()->skipRefp(), IfaceRefDType);
            إذا (dtypep && dtypep->isVirtual()) {
                ثابت auto it = m_ifaceMemberVscps.find({dtypep->ifacep(), nodep->varp()->name()});
                إذا (it != m_ifaceMemberVscps.end()) {
                    V3GraphVertex* ثابت funcVtxp = getCFuncVertexp(m_cfuncp);
                    لكل (AstVarScope* ثابت vscp : it->second) {
                        V3GraphVertex* varVtxp = vscp->user1u().toGraphVertex();
                        إذا (!varVtxp) {
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
    فراغ visit(Ast_تۈگۈن* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    explicit TraceVisitor(AstNetlist* nodep)
        : m_alwaysVtxp{جديد TraceActivityVertex{&m_graph, TraceActivityVertex::ACTIVITY_ALWAYS}} {
        nodep->user2_Clear_دەرەخ();  // TraceDecl multiple activities flag
        nodep->user3_Clear_دەرەخ();  // TraceDecl TraceActivityVertex (assumes we start at nullptr)
        iterate(nodep);
        nodep->foreach([](AstTraceDecl* ثابت declp) {
            إذا (declp->inDtypeFunc()) {
                declp->valuep()->unlinkFrBack()->delete_دەرەخ();
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
    V3Global::dump_Check_Global_دەرەخ("trace", 0, dump_دەرەخ_Either_Level() >= 3);
}
