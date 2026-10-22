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

#شامل "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#شامل "V3Trace.h"

#شامل "V3Ast.h"
#شامل "V3DupFinder.h"
#شامل "V3EmitCBase.h"
#شامل "V3Graph.h"
#شامل "V3Stats.h"
#شامل "V3UniqueNames.h"

#شامل <limits>
#شامل <map>
#شامل <set>
#شامل <unordered_map>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Graph vertexes

رده TraceActivityVertex final : عمومی V3GraphVertex {
    VL_RTTI_IMPL(TraceActivityVertex, V3GraphVertex)
    Ast_گره* ثابت m_insertp;
    int32_t m_activityCode;
    منطقی m_slow;  // If always slow, we can use the same code
عمومی:
    شمارش { ACTIVITY_NEVER = ((1UL << 31) - 1) };
    شمارش { ACTIVITY_ALWAYS = ((1UL << 31) - 2) };
    شمارش { ACTIVITY_SLOW = 0 };
    TraceActivityVertex(V3Graph* graphp, Ast_گره* nodep, منطقی slow)
        : V3GraphVertex{graphp}
        , m_insertp{nodep} {
        m_activityCode = 0;
        m_slow = slow;
    }
    TraceActivityVertex(V3Graph* graphp, int32_t code)
        : V3GraphVertex{graphp}
        , m_insertp{nullptr} {
        m_activityCode = code;
        m_slow = نادرست;
    }
    ~TraceActivityVertex() override = پیشفرض;
    // ACCESSORS
    Ast_گره* insertp() ثابت {
        UASSERT(m_insertp, "Null insertp; probably called on a special always/slow");
        بازگشت m_insertp;
    }
    string name() ثابت override {
        اگر (activityAlways()) {
            بازگشت "*ALWAYS*";
        } وگرنه {
            بازگشت std::string{slow() ? "*SLOW* " : ""} + insertp()->name();
        }
    }
    string dotColor() ثابت override { بازگشت slow() ? "yellowGreen" : "green"; }
    int32_t activityCode() ثابت { بازگشت m_activityCode; }
    منطقی activityAlways() ثابت { بازگشت activityCode() == ACTIVITY_ALWAYS; }
    منطقی activitySlow() ثابت { بازگشت activityCode() == ACTIVITY_SLOW; }
    تهی activityCode(int32_t code) { m_activityCode = code; }
    منطقی slow() ثابت { بازگشت m_slow; }
    تهی slow(منطقی flag) {
        اگر (!flag) m_slow = نادرست;
    }
};

رده TraceCFuncVertex final : عمومی V3GraphVertex {
    VL_RTTI_IMPL(TraceCFuncVertex, V3GraphVertex)
    AstCFunc* ثابت m_nodep;

عمومی:
    TraceCFuncVertex(V3Graph* graphp, AstCFunc* nodep)
        : V3GraphVertex{graphp}
        , m_nodep{nodep} {}
    ~TraceCFuncVertex() override = پیشفرض;
    // ACCESSORS
    AstCFunc* nodep() ثابت { بازگشت m_nodep; }
    string name() ثابت override { بازگشت nodep()->name(); }
    string dotColor() ثابت override { بازگشت "yellow"; }
    File_سطر* fileline() ثابت override { بازگشت nodep()->fileline(); }
};

رده TraceTraceVertex final : عمومی V3GraphVertex {
    VL_RTTI_IMPL(TraceTraceVertex, V3GraphVertex)
    AstTraceDecl* ثابت m_nodep;  // TRACEINC this represents
    // nullptr, or other vertex with the real code() that duplicates this one
    TraceTraceVertex* m_duplicatep = nullptr;
    // When aliasing to a dtype parent, offset of the member within the dtype
    uint32_t m_dtypeAliasOffset = 0;

عمومی:
    TraceTraceVertex(V3Graph* graphp, AstTraceDecl* nodep)
        : V3GraphVertex{graphp}
        , m_nodep{nodep} {}
    ~TraceTraceVertex() override = پیشفرض;
    // ACCESSORS
    AstTraceDecl* nodep() ثابت { بازگشت m_nodep; }
    string name() ثابت override { بازگشت nodep()->name(); }
    string dotColor() ثابت override { بازگشت "red"; }
    File_سطر* fileline() ثابت override { بازگشت nodep()->fileline(); }
    TraceTraceVertex* duplicatep() ثابت { بازگشت m_duplicatep; }
    تهی duplicatep(TraceTraceVertex* dupp) {
        UASSERT_OBJ(!duplicatep(), nodep(), "Assigning duplicatep() to already duplicated node");
        m_duplicatep = dupp;
    }
    تهی redirectDuplicatep(TraceTraceVertex* dupp) { m_duplicatep = dupp; }
    uint32_t dtypeAliasOffset() ثابت { بازگشت m_dtypeAliasOffset; }
    تهی dtypeAliasOffset(uint32_t offset) { m_dtypeAliasOffset = offset; }
};

رده TraceVarVertex final : عمومی V3GraphVertex {
    VL_RTTI_IMPL(TraceVarVertex, V3GraphVertex)
    AstVarScope* ثابت m_nodep;

عمومی:
    TraceVarVertex(V3Graph* graphp, AstVarScope* nodep)
        : V3GraphVertex{graphp}
        , m_nodep{nodep} {}
    ~TraceVarVertex() override = پیشفرض;
    // ACCESSORS
    AstVarScope* nodep() ثابت { بازگشت m_nodep; }
    string name() ثابت override { بازگشت nodep()->name(); }
    string dotColor() ثابت override { بازگشت "skyblue"; }
    File_سطر* fileline() ثابت override { بازگشت nodep()->fileline(); }
};

//######################################################################
// Trace state, as a visitor of each AstNode

رده TraceVisitor final : عمومی VNVisitor {
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
    Ast_گره_Module* m_بالاترین_Modp = nullptr;  // Module to add variables to
    AstScope* ثابت m_بالاترین_Scopep = v3Global.rootp()->بالاترین_Scopep()->scopep();  // The top AstScope
    AstCFunc* m_cfuncp = nullptr;  // C function adding to graph
    AstCFunc* m_regFuncp = nullptr;  // Trace registration function
    AstCFunc* m_actAllFuncp = nullptr;  // Set all activity function
    AstTraceDecl* m_tracep = nullptr;  // Trace function adding to graph
    AstVarScope* m_activityVscp = nullptr;  // Activity variable
    uint32_t m_activity_عدد = 0;  // Count of fields in activity variable
    uint32_t m_code = 0;  // Trace ident code# being assigned
    V3Graph m_graph;  // Var/CFunc tracking
    TraceActivityVertex* ثابت m_alwaysVtxp;  // "Always trace" vertex
    منطقی m_finding = نادرست;  // Pass one of algorithm?
    ساختار DtypeFuncs final {
    عمومی:
        AstCFunc* fullFuncp = nullptr;
        AstCFunc* chgFuncp = nullptr;
    };
    std::unordered_map<ثابت Ast_گره_DType*, DtypeFuncs>
        m_dtypeNonConstFuncs;  // Full / Chg funcs per type
    std::unordered_map<ثابت Ast_گره_DType*, AstCFunc*> m_dtypeConstFuncs;  // Const func per type
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

    رده TraceInitDeclCollector final : عمومی VNVisitor {
        std::vector<AstTraceDecl*>& m_declps;  // Output: trace declarations found so far
        std::set<ثابت AstCFunc*> m_seenFuncps;

        تهی visit(AstTraceDecl* nodep) override { m_declps.افزودن_back(nodep); }
        تهی visit(AstCCall* nodep) override {
            اگر (AstCFunc* ثابت funcp = nodep->funcp()) collect(funcp);
        }
        تهی visit(Ast_گره* nodep) override { iterateChildren(nodep); }

    عمومی:
        explicit TraceInitDeclCollector(std::vector<AstTraceDecl*>& declps)
            : m_declps{declps} {}
        تهی collect(AstCFunc* funcp) {
            اگر (funcp && m_seenFuncps.درج(funcp).second) iterate(funcp);
        }
    };

    // METHODS

    ایستا منطقی same_ریشه_Init_Alias(ثابت AstTraceDecl* ریشه_Declp, ثابت AstTraceDecl* بالاترین_Declp) {
        ثابت VNumRange& ریشه_Bit_Range = ریشه_Declp->bitRange();
        ثابت VNumRange& بالاترین_Bit_Range = بالاترین_Declp->bitRange();
        بازگشت ریشه_Declp->showname() == بالاترین_Declp->showname()
               && ریشه_Declp->declDirection() == بالاترین_Declp->declDirection()
               && ریشه_Declp->widthMin() == بالاترین_Declp->widthMin()
               && ریشه_Bit_Range.ranged() == بالاترین_Bit_Range.ranged()
               && (!ریشه_Bit_Range.ranged()
                   || (ریشه_Bit_Range.چپ() == بالاترین_Bit_Range.چپ()
                       && ریشه_Bit_Range.راست() == بالاترین_Bit_Range.راست()));
    }

    تهی detectDuplicates() {
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
        برای (V3GraphVertex& vtx : m_graph.vertices()) {
            اگر (TraceTraceVertex* ثابت vvertexp = vtx.cast<TraceTraceVertex>()) {
                AstTraceDecl* ثابت nodep = vvertexp->nodep();
                اگر (nodep->dtypeCallp()) ادامه;

                اگر (nodep->dtypeDeclp()) {
                    uint32_t& offset = runningOffset[nodep->dtypeDeclp()];
                    memberOffsets[nodep] = offset;
                    offset += nodep->codeInc();
                }

                UASSERT_OBJ(!vvertexp->duplicatep(), nodep, "Should not be a duplicate");
                ثابت auto dupit = dupFinder.findDuplicate(nodep);
                اگر (dupit == dupFinder.end()) {
                    dupFinder.درج(nodep);
                } وگرنه {
                    AstTraceDecl* ثابت dupDeclp = VN_AS(dupit->second, TraceDecl);
                    UASSERT_OBJ(dupDeclp, nodep, "Trace duplicate of wrong type");
                    TraceTraceVertex* ثابت dupvertexp
                        = dupDeclp->user1u().toGraphVertex()->cast<TraceTraceVertex>();
                    UINFO(8, "  Orig " << dupDeclp << endl);
                    UINFO(8, "   dup " << nodep << endl);

                    اگر (dupDeclp->dtypeDeclp() && !dupDeclp->dtypeDeclp()->user2()) {
                        AstTraceDecl* ثابت dtypeParentp = dupDeclp->dtypeDeclp();
                        TraceTraceVertex* ثابت dtypeVtxp
                            = dtypeParentp->user1u().toGraphVertex()->cast<TraceTraceVertex>();
                        ثابت auto it2 = memberOffsets.find(dupDeclp);
                        UASSERT_OBJ(it2 != memberOffsets.end(), dupDeclp,
                                    "Member offset not precomputed");
                        vvertexp->duplicatep(dtypeVtxp);
                        vvertexp->dtypeAliasOffset(it2->second);
                        dtypeParentRedirects.emplace_back(vvertexp, dupvertexp);
                    } وگرنه {
                        vvertexp->duplicatep(dupvertexp);
                    }
                    اگر (nodep->dtypeDeclp()) nodep->dtypeDeclp()->user2(درست);
                }
            }
        }

        برای (ثابت auto& pair : dtypeParentRedirects) {
            TraceTraceVertex* ثابت dupVtxp = pair.first;
            TraceTraceVertex* ثابت memberVtxp = pair.second;
            ثابت AstTraceDecl* ثابت parentDeclp = dupVtxp->duplicatep()->nodep();
            اگر (parentDeclp->user2()) {
                dupVtxp->redirectDuplicatep(memberVtxp);
                dupVtxp->dtypeAliasOffset(0);
            }
        }

        اگر (dumpLevel() || debug() >= 9)
            dupFinder.dumpFile(v3Global.debugFilename("trace") + ".hash", نادرست);
    }

    تهی graphDtypePrune() {
        برای (V3GraphVertex* ثابت vtxp : m_graph.vertices().unlinkable()) {
            اگر (TraceTraceVertex* ثابت vvertexp = vtxp->cast<TraceTraceVertex>()) {
                AstTraceDecl* ثابت declp = vvertexp->nodep();
                // This skips the dtype sub-func optimization if a var is affected by multiple
                // activities.  We really only need to do this for _chg funcs (and not decls,
                // _const and _full) but it's simpiler to do it all one way or the other.
                اگر (declp) {
                    اگر (declp->user2() || (declp->dtypeDeclp() && !declp->dtypeDeclp()->user2())) {
                        AstCCall* ثابت callp = declp->dtypeCallp();
                        اگر (callp) {
                            Ast_گره* stmtexprp = callp->backp();
                            VL_DO_DANGLING(افزودن_Deletep(stmtexprp->unlinkFrBack()), stmtexprp);
                        } وگرنه {
                            منطقی emptyScope;
                            انجامبده {
                                emptyScope = نادرست;
                                Ast_گره* ثابت declBackp = declp->backp();
                                Ast_گره* ثابت declNextp = declp->nextp();
                                اگر (VN_IS(declBackp, Trace_افزودن_Prefix)
                                    && VN_IS(declNextp, Trace_برداشت_Prefix)) {
                                    VL_DO_DANGLING(افزودن_Deletep(declBackp->unlinkFrBack()),
                                                   declBackp);
                                    VL_DO_DANGLING(افزودن_Deletep(declNextp->unlinkFrBack()),
                                                   declNextp);
                                    emptyScope = درست;
                                }
                            } وقتی (emptyScope);
                        }
                        // Can't purge until we finish this pass
                        افزودن_Deletep(declp->unlinkFrBack());  // declp used below
                        vvertexp->rerouteEdges(&m_graph);
                        vvertexp->unlinkDelete(&m_graph);
                    }
                    declp->dtypeDeclp(nullptr);
                }
            }
        }
    }

    تهی splitTraceDeclFuncs() {
        ثابت صحیح splitLimit = v3Global.opt.outputSplitCTrace() ? v3Global.opt.outputSplitCTrace()
                                                                : std::numeric_limits<صحیح>::max();
        اگر (splitLimit == std::numeric_limits<صحیح>::max()) بازگشت;

        برای (Ast_گره* nodep = m_بالاترین_Scopep->blocksp(); nodep; nodep = nodep->nextp()) {
            AstCFunc* ثابت funcp = VN_CAST(nodep, CFunc);
            اگر (!funcp || !funcp->user2()) ادامه;

            ثابت string baseName = funcp->name();
            uint32_t sub_رقم = 1;
            صحیح curSize = 0;
            AstCFunc* curFuncp = funcp;
            std::vector<Ast_گره_Stmt*> callStmts;

            auto startNewSubFunc = [&]() {
                File_سطر* ثابت flp = funcp->fileline();
                ثابت string newName = baseName + "_" + cvtToStr(sub_رقم++);
                AstCFunc* ثابت newFuncp = جدید AstCFunc{flp, newName, m_بالاترین_Scopep};
                newFuncp->argTypes(v3Global.opt.traceClassBase() + "* tracep");
                newFuncp->isTrace(درست);
                newFuncp->isStatic(نادرست);
                newFuncp->isLoose(درست);
                newFuncp->slow(درست);
                newFuncp->dontCombine(درست);
                m_بالاترین_Scopep->addBlocksp(newFuncp);
                newFuncp->addStmtsp(جدید AstCStmt{flp, "const int c = vlSymsp->__Vm_baseCode;"});
                AstCCall* ثابت callp = جدید AstCCall{flp, newFuncp};
                callp->dtypeSetVoid();
                callp->argTypes("tracep");
                callStmts.افزودن_back(callp->makeStmt());
                curFuncp = newFuncp;
                curSize = 0;
            };

            برای (Ast_گره *stmtp = funcp->stmtsp(), *nextp; stmtp; stmtp = nextp) {
                nextp = stmtp->nextp();
                curSize += stmtp->گره_شمار();

                اگر (curFuncp != funcp) {
                    stmtp->unlinkFrBack();
                    curFuncp->addStmtsp(stmtp);
                }

                اگر (curSize > splitLimit && nextp) startNewSubFunc();
            }

            برای (Ast_گره_Stmt* ثابت callStmtp : callStmts) { funcp->addStmtsp(callStmtp); }
        }
    }

    تهی graphSimplify(منطقی initial) {
        اگر (initial) {
            // Remove all variable nodes
            برای (V3GraphVertex* ثابت vtxp : m_graph.vertices().unlinkable()) {
                اگر (TraceVarVertex* ثابت vvertexp = vtxp->cast<TraceVarVertex>()) {
                    vvertexp->rerouteEdges(&m_graph);
                    vvertexp->unlinkDelete(&m_graph);
                }
            }
            // Remove multiple variables connecting funcs to traces
            // We do this twice, as then we have fewer edges to multiply out in the below
            // expansion.
            m_graph.removeRedundantEdgesMax(&V3GraphEdge::followAlwaysTrue);
            // Remove all Cfunc nodes
            برای (V3GraphVertex* ثابت vtxp : m_graph.vertices().unlinkable()) {
                اگر (TraceCFuncVertex* ثابت vvertexp = vtxp->cast<TraceCFuncVertex>()) {
                    vvertexp->rerouteEdges(&m_graph);
                    vvertexp->unlinkDelete(&m_graph);
                }
            }
        }

        // Remove multiple variables connecting funcs to traces
        m_graph.removeRedundantEdgesMax(&V3GraphEdge::followAlwaysTrue);

        // If there are any edges from a always, keep only the always
        برای (V3GraphVertex& vtx : m_graph.vertices()) {
            اگر (TraceTraceVertex* ثابت vvertexp = vtx.cast<TraceTraceVertex>()) {
                // Search for the incoming always edge
                ثابت V3GraphEdge* alwaysEdgep = nullptr;
                برای (ثابت V3GraphEdge& edge : vvertexp->inEdges()) {
                    ثابت TraceActivityVertex* ثابت actVtxp
                        = edge.fromp()->as<ثابت TraceActivityVertex>();
                    اگر (actVtxp->activityAlways()) {
                        alwaysEdgep = &edge;
                        بشکن;
                    }
                }
                // If always edge exists, remove all other edges
                اگر (alwaysEdgep) {
                    برای (V3GraphEdge* ثابت edgep : vvertexp->inEdges().unlinkable()) {
                        اگر (edgep != alwaysEdgep) VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
                    }
                }
            }
        }

        // Activity points with no outputs can be removed
        برای (V3GraphVertex* ثابت vtxp : m_graph.vertices().unlinkable()) {
            اگر (TraceActivityVertex* ثابت aVtxp = vtxp->cast<TraceActivityVertex>()) {
                // Leave in the always vertex for later use.
                اگر (aVtxp != m_alwaysVtxp && aVtxp->outEmpty()) {
                    VL_DO_DANGLING(aVtxp->unlinkDelete(&m_graph), aVtxp);
                }
            }
        }
    }

    uint32_t assignactivityNumbers() {
        uint32_t activity_عدد = 1;  // Note 0 indicates "slow" only
        برای (V3GraphVertex& vtx : m_graph.vertices()) {
            اگر (TraceActivityVertex* ثابت vvertexp = vtx.cast<TraceActivityVertex>()) {
                اگر (vvertexp != m_alwaysVtxp) {
                    اگر (vvertexp->slow()) {
                        vvertexp->activityCode(TraceActivityVertex::ACTIVITY_SLOW);
                    } وگرنه {
                        vvertexp->activityCode(activity_عدد++);
                    }
                }
            }
        }
        بازگشت activity_عدد;
    }

    تهی sortTraces(TraceVec& traces, uint32_t& nNonConstCodes) {
        // Populate sort structure
        traces.clear();
        nNonConstCodes = 0;
        برای (V3GraphVertex& vtx : m_graph.vertices()) {
            اگر (TraceTraceVertex* ثابت vtxp = vtx.cast<TraceTraceVertex>()) {
                ActCodeSet actSet;
                UINFO(9, "  Add to sort: " << vtxp);
                UINFOTREE(9, vtxp->nodep(), "", "trnode");
                برای (ثابت V3GraphEdge& edge : vtxp->inEdges()) {
                    ثابت TraceActivityVertex* ثابت cfvertexp
                        = edge.fromp()->cast<ثابت TraceActivityVertex>();
                    UASSERT_OBJ(cfvertexp, vtxp->nodep(),
                                "Should have been function pointing to this trace");
                    UINFO(9, "   Activity: " << cfvertexp);
                    اگر (cfvertexp->activityAlways()) {
                        // If code 0, we always trace; ignore other codes
                        actSet.درج(TraceActivityVertex::ACTIVITY_ALWAYS);
                    } وگرنه {
                        actSet.درج(cfvertexp->activityCode());
                    }
                }
                UASSERT_OBJ(actSet.شمار(TraceActivityVertex::ACTIVITY_ALWAYS) == 0
                                || actSet.size() == 1,
                            vtxp->nodep(), "Always active trace has further triggers");
                // Count nodes
                اگر (!vtxp->duplicatep() && !actSet.empty())
                    nNonConstCodes += vtxp->nodep()->codeInc();
                اگر (actSet.empty()) {
                    // If a trace doesn't have activity, it's constant, and we
                    // don't need to track changes on it.
                    actSet.درج(TraceActivityVertex::ACTIVITY_NEVER);
                } وگرنه اگر (actSet.شمار(TraceActivityVertex::ACTIVITY_SLOW) && actSet.size() > 1) {
                    // If a trace depends on the slow flag as well as other
                    // flags, remove the dependency on the slow flag. We will
                    // make slow routines set all activity flags.
                    actSet.erase(TraceActivityVertex::ACTIVITY_SLOW);
                }
                traces.emplace(actSet, vtxp);
            }
        }
    }

    تهی graphOptimize() {
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
        وقتی (it != end) {
            auto head = it;
            // Approximate the complexity of the value change check
            uint32_t complexity = 0;
            ثابت ActCodeSet& actSet = it->first;
            برای (; it != end && it->first == actSet; ++it) {
                اگر (!it->second->duplicatep()) {
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
            اگر (actSet.شمار(TraceActivityVertex::ACTIVITY_ALWAYS)) ادامه;
            اگر (actSet.شمار(TraceActivityVertex::ACTIVITY_NEVER)) ادامه;
            اگر (actSet.شمار(TraceActivityVertex::ACTIVITY_SLOW)) ادامه;
            // If the value comparisons are cheaper to perform than checking the
            // activity flags make the signals always traced. Note this cost
            // equation is heuristic.
            اگر (complexity <= actSet.size() * 2) {
                برای (; head != it; ++head) {
                    جدید V3GraphEdge{&m_graph, m_alwaysVtxp, head->second, 1};
                }
            }
        }

        graphSimplify(نادرست);
    }

    Ast_گره_عبارت* selectActivity(File_سطر* flp, uint32_t acode, ثابت VAccess& access) {
        بازگشت جدید AstArraySel(flp, جدید AstVarRef{flp, m_activityVscp, access}, acode);
    }

    Ast_گره* newActivitySetter(Ast_گره* insertp, uint32_t code) {
        ++m_statSetters;
        File_سطر* ثابت fl = insertp->fileline();
        AstAssign* ثابت setterp = جدید AstAssign{fl, selectActivity(fl, code, VAccess::WRITE),
                                                 جدید AstConst{fl, AstConst::BitTrue{}}};
        بازگشت setterp;
    }

    Ast_گره* newActivityAll(Ast_گره* insertp) {
        ++m_statSettersSlow;
        اگر (!m_actAllFuncp) {
            File_سطر* ثابت flp = m_بالاترین_Scopep->fileline();
            AstCFunc* ثابت funcp = جدید AstCFunc{flp, "__VtraceActivitySetAll", m_بالاترین_Scopep};
            funcp->slow(درست);
            funcp->isStatic(نادرست);
            funcp->isLoose(درست);
            m_بالاترین_Scopep->addBlocksp(funcp);
            برای (uint32_t code = 0; code < m_activity_عدد; ++code) {
                Ast_گره* ثابت setterp = newActivitySetter(insertp, code);
                funcp->addStmtsp(setterp);
            }
            m_actAllFuncp = funcp;
        }
        AstCCall* ثابت callp = جدید AstCCall{insertp->fileline(), m_actAllFuncp};
        callp->dtypeSetVoid();
        بازگشت callp->makeStmt();
    }

    تهی createActivityFlags() {
        // Assign final activity numbers
        m_activity_عدد = assignactivityNumbers();

        // Create an array of bytes, not a bit vector, as they can be set
        // atomically by mtasks, and are cheaper to set (no need for
        // read-modify-write on the C type), and the speed of the tracing code
        // is the same on largish designs.
        File_سطر* ثابت flp = m_بالاترین_Scopep->fileline();
        Ast_گره_DType* ثابت newScalarDtp = جدید AstBasicDType{flp, VFlagBitPacked{}, 1};
        v3Global.rootp()->typeTablep()->addTypesp(newScalarDtp);
        AstRange* ثابت newArange
            = جدید AstRange{flp, VNumRange{static_cast<صحیح>(m_activity_عدد) - 1, 0}};
        Ast_گره_DType* ثابت newArrDtp = جدید AstUnpackArrayDType{flp, newScalarDtp, newArange};
        v3Global.rootp()->typeTablep()->addTypesp(newArrDtp);
        m_activityVscp = v3Global.rootp()->بالاترین_Scopep()->createTemp("__VtraceActivity", newArrDtp);

        // Insert activity setters
        برای (ثابت V3GraphVertex& vtx : m_graph.vertices()) {
            اگر (ثابت TraceActivityVertex* ثابت vtxp = vtx.cast<ثابت TraceActivityVertex>()) {
                Ast_گره* setterp = nullptr;
                اگر (vtxp->activitySlow()) {
                    setterp = newActivityAll(vtxp->insertp());
                    // Just set all flags in slow code as it should be rare.
                    // This will be rolled up into a loop by V3Reloop.
                } وگرنه اگر (!vtxp->activityAlways()) {
                    setterp = newActivitySetter(vtxp->insertp(), vtxp->activityCode());
                }
                اگر (setterp) {
                    Ast_گره* ثابت insertp = vtxp->insertp();
                    اگر (Ast_Stmt_عبارت* ثابت stmtp = VN_CAST(insertp, Stmt_عبارت)) {
                        stmtp->addNextHere(setterp);
                    } وگرنه اگر (AstCFunc* ثابت funcp = VN_CAST(insertp, CFunc)) {
                        // If there are awaits, insert the setter after each await
                        اگر (funcp->isCoroutine() && funcp->stmtsp()) {
                            funcp->stmtsp()->foreachAndNext([setterp](AstCAwait* awaitp) {
                                awaitp->addNextHere(setterp->clone_درخت(نادرست));
                            });
                        }
                        funcp->addStmtsp(setterp);
                    } وگرنه {
                        insertp->v3fatalSrc("Bad trace activity vertex");
                    }
                }
            }
        }
    }

    AstCFunc* newCFunc(VTraceType traceType, AstCFunc* بالاترین_Funcp, uint32_t func_رقم,
                       uint32_t baseCode = 0, ثابت AstTraceDecl* ثابت declp = nullptr,
                       منطقی declSub = نادرست) {
        // Create new function
        ثابت منطقی is_بالاترین_Func = !declp && بالاترین_Funcp == nullptr;
        std::string funcName;
        اگر (is_بالاترین_Func || declp) {
            funcName = traceType.func_prefix();
        } وگرنه {
            funcName = بالاترین_Funcp->name();
            funcName += "_sub";
        }
        اگر (declp) {
            funcName += "_dtype";
            اگر (declSub) funcName += "_sub";
            funcName += "__";
            funcName = m_dtypeNames.get(funcName);
        } وگرنه {
            funcName += "_";
            funcName += cvtToStr(func_رقم);
        }

        File_سطر* ثابت flp = m_بالاترین_Scopep->fileline();
        AstCFunc* ثابت funcp = جدید AstCFunc{flp, funcName, m_بالاترین_Scopep};
        funcp->isTrace(درست);
        funcp->dontCombine(درست);
        funcp->isLoose(درست);
        funcp->slow(traceType != VTraceType::CHANGE);
        funcp->isStatic(is_بالاترین_Func);
        // Add it to top scope
        m_بالاترین_Scopep->addBlocksp(funcp);
        ثابت std::string bufArg
            = v3Global.opt.traceClassBase() + "::Buffer* bufp"
              + (declp ? (", uint32_t offset, const " + declp->dtypep()->cType("", درست, درست)
                          + " __VdtypeVar")
                       : "");
        اگر (is_بالاترین_Func) {
            // Top functions
            funcp->argTypes("void* voidSelf, " + bufArg);
            funcp->addStmtsp(جدید AstCStmt{flp, EmitCUtil::voidSelfAssign(m_بالاترین_Modp)});
            funcp->addStmtsp(جدید AstCStmt{flp, EmitCUtil::symClassAssign()});
            // Add global activity check to change dump functions
            اگر (traceType == VTraceType::CHANGE) {  //
                funcp->addStmtsp(
                    جدید AstCStmt{flp, "if (VL_UNLIKELY(!vlSymsp->__Vm_activity)) return;"});
            }
            // Register function
            AstCStmt* ثابت cstmtp = جدید AstCStmt{flp};
            m_regFuncp->addStmtsp(cstmtp);
            اگر (traceType == VTraceType::CONSTANT) {
                cstmtp->add("tracep->addConstCb(");
                cstmtp->add(جدید AstAddrOfCFunc{flp, funcp});
                cstmtp->add(", " + std::to_string(func_رقم) + ", vlSelf);");
            } وگرنه اگر (traceType == VTraceType::FULL) {
                cstmtp->add("tracep->addFullCb(");
                cstmtp->add(جدید AstAddrOfCFunc{flp, funcp});
                cstmtp->add(", " + std::to_string(func_رقم) + ", vlSelf);");
            } وگرنه {
                cstmtp->add("tracep->addChgCb(");
                cstmtp->add(جدید AstAddrOfCFunc{flp, funcp});
                cstmtp->add(", " + std::to_string(func_رقم) + ", vlSelf);");
            }
        } وگرنه {
            // Sub functions
            funcp->argTypes(bufArg);
            // Setup base references. Note in rare occasions we can end up with an empty trace
            // sub function, hence the VL_ATTR_UNUSED attributes.
            اگر (traceType != VTraceType::CHANGE) {
                // Full dump sub function
                funcp->addStmtsp(جدید AstCStmt{flp,  //
                                              string("uint32_t* const oldp VL_ATTR_UNUSED = "
                                                     "bufp->oldp(vlSymsp->__Vm_baseCode")
                                                  + (declp ? " + offset" : "") + ");\n"});
            } وگرنه {
                // Change dump sub function
                funcp->addStmtsp(جدید AstCStmt{flp,  //
                                              "uint32_t* const oldp VL_ATTR_UNUSED = "
                                              "bufp->oldp(vlSymsp->__Vm_baseCode + "
                                                  + (declp ? " offset" : cvtToStr(baseCode))
                                                  + ");\n"});
            }
            اگر (!declp) {
                // Add call to top function
                AstCCall* ثابت callp = جدید AstCCall{funcp->fileline(), funcp};
                callp->dtypeSetVoid();
                callp->argTypes("bufp");
                بالاترین_Funcp->addStmtsp(callp->makeStmt());
            }
        }
        // Done
        UINFO(5, "  newCFunc " << funcp);
        بازگشت funcp;
    }

    AstCFunc* newCDtypeSubFunc(VTraceType traceType, ثابت AstTraceDecl* ثابت declp,
                               AstCFunc* parentp) {
        AstCFunc* ثابت funcp = newCFunc(traceType, nullptr, 0, 0, declp, درست);
        File_سطر* ثابت flp = m_بالاترین_Scopep->fileline();
        AstCCall* ثابت callp = جدید AstCCall{flp, funcp};
        callp->dtypeSetVoid();
        callp->argTypes("bufp, offset, __VdtypeVar");
        parentp->addStmtsp(callp->makeStmt());
        بازگشت funcp;
    }

    AstCFunc* createConstDtypeTraceFunctions(ثابت AstTraceDecl* declp) {
        ثابت Ast_گره_DType* ثابت dtypep = declp->dtypep()->skipRefp();
        auto pair = m_dtypeConstFuncs.emplace(dtypep, nullptr);
        اگر (pair.second) {
            File_سطر* ثابت flp = declp->fileline();
            AstCFunc* ثابت funcp = newCFunc(VTraceType::CONSTANT, nullptr, 0, 0, declp);

            منطقی first = درست;
            برای (Ast_گره* callStmtp = declp->dtypeCallp()->funcp()->stmtsp(); callStmtp;
                 callStmtp = callStmtp->nextp()) {
                ثابت AstCCall* ثابت callp
                    = VN_CAST(VN_CAST(callStmtp, Stmt_عبارت)->exprp(), CCall);
                منطقی onlyOne = first && !callStmtp->nextp();
                AstCFunc* ثابت subFuncp
                    = onlyOne ? funcp : newCDtypeSubFunc(VTraceType::CONSTANT, declp, funcp);
                برای (Ast_گره* stmtp = callp->funcp()->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    اگر (AstTraceDecl* ثابت fieldDeclp = VN_CAST(stmtp, TraceDecl)) {
                        AstTraceInc* ثابت incp
                            = جدید AstTraceInc{flp, fieldDeclp, VTraceType::CONSTANT};
                        subFuncp->addStmtsp(incp);
                    }
                }
                first = نادرست;
            }

            pair.first->second = funcp;
        }

        بازگشت pair.first->second;
    }

    تهی createConstTraceFunctions(ثابت TraceVec& traces) {
        ثابت صحیح splitLimit = v3Global.opt.outputSplitCTrace() ? v3Global.opt.outputSplitCTrace()
                                                                : std::numeric_limits<صحیح>::max();

        AstCFunc* ثابت بالاترین_Funcp = newCFunc(VTraceType::CONSTANT, nullptr, 0);
        uint32_t sub_Func_رقم = 0;
        AstCFunc* subFuncp = nullptr;
        صحیح subStmts = 0;
        std::vector<ثابت TraceTraceVertex*> duplicates;
        برای (auto it = traces.cbegin(); it != traces.end(); ++it) {
            ثابت TraceTraceVertex* ثابت vtxp = it->second;
            AstTraceDecl* ثابت declp = vtxp->nodep();
            اگر (vtxp->duplicatep()) {
                duplicates.افزودن_back(vtxp);
                ادامه;
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
            اگر (actSet.شمار(TraceActivityVertex::ACTIVITY_NEVER)) {
                // Crate new sub function if required
                اگر (!subFuncp || subStmts > splitLimit) {
                    subStmts = 0;
                    subFuncp = newCFunc(VTraceType::CONSTANT, بالاترین_Funcp, sub_Func_رقم);
                    ++sub_Func_رقم;
                }
                File_سطر* ثابت flp = declp->fileline();
                اگر (declp->dtypeCallp()) {
                    AstCFunc* ثابت funcp = createConstDtypeTraceFunctions(declp);
                    Ast_گره_عبارت* argsp = nullptr;
                    argsp = Ast_گره::addNext(argsp, declp->valuep()->clone_درخت(نادرست));
                    AstCCall* ثابت callp = جدید AstCCall{flp, funcp, argsp};
                    callp->dtypeSetVoid();
                    callp->argTypes(callp->argTypes() + "bufp, " + std::to_string(declp->code()));
                    subFuncp->addStmtsp(callp->makeStmt());

                    subStmts += 1;
                } وگرنه {
                    AstTraceInc* ثابت incp = جدید AstTraceInc{flp, declp, VTraceType::CONSTANT};
                    subFuncp->addStmtsp(incp);
                    subStmts += incp->گره_شمار();
                }
            }
        }

        // Assign codes to duplicate nodes from their canonicals
        برای (ثابت TraceTraceVertex* ثابت vtxp : duplicates) {
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
        Ast_گره_DType* dtypep = declp->dtypep()->skipRefp();
        auto pair = m_dtypeNonConstFuncs.emplace(dtypep, DtypeFuncs{});
        اگر (pair.second) {
            File_سطر* ثابت flp = declp->fileline();
            AstCFunc* ثابت fullFuncp = newCFunc(VTraceType::FULL, nullptr, 0, 0, declp);
            AstCFunc* ثابت chgFuncp = newCFunc(VTraceType::CHANGE, nullptr, 0, 0, declp);

            منطقی first = درست;
            برای (Ast_گره* callStmtp = declp->dtypeCallp()->funcp()->stmtsp(); callStmtp;
                 callStmtp = callStmtp->nextp()) {
                ثابت AstCCall* ثابت callp
                    = VN_CAST(VN_CAST(callStmtp, Stmt_عبارت)->exprp(), CCall);
                منطقی onlyOne = first && !callStmtp->nextp();
                AstCFunc* ثابت fullSubFuncp
                    = onlyOne ? fullFuncp : newCDtypeSubFunc(VTraceType::FULL, declp, fullFuncp);
                AstCFunc* ثابت chgSubFuncp
                    = onlyOne ? chgFuncp : newCDtypeSubFunc(VTraceType::CHANGE, declp, chgFuncp);
                برای (Ast_گره* stmtp = callp->funcp()->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    اگر (AstTraceDecl* ثابت fieldDeclp = VN_CAST(stmtp, TraceDecl)) {
                        AstTraceInc* ثابت incFullp
                            = جدید AstTraceInc{flp, fieldDeclp, VTraceType::FULL};
                        fullSubFuncp->addStmtsp(incFullp);
                        AstTraceInc* ثابت incChgp
                            = جدید AstTraceInc{flp, fieldDeclp, VTraceType::CHANGE};
                        chgSubFuncp->addStmtsp(incChgp);
                    }
                }
                first = نادرست;
            }

            pair.first->second = {.fullFuncp = fullFuncp, .chgFuncp = chgFuncp};
        }

        بازگشت pair.first->second;
    }

    تهی createNonConstTraceFunctions(ثابت TraceVec& traces, uint32_t nAllCodes,
                                      uint32_t parallelism) {
        ثابت صحیح splitLimit = v3Global.opt.outputSplitCTrace() ? v3Global.opt.outputSplitCTrace()
                                                                : std::numeric_limits<صحیح>::max();

        // pre-incremented, so starts at 0
        uint32_t بالاترین_Func_رقم = std::numeric_limits<uint32_t>::max();
        TraceVec::const_iterator it = traces.begin();
        وقتی (it != traces.end()) {
            AstCFunc* بالاترین_Ful_Funcp = nullptr;
            AstCFunc* بالاترین_Chg_Funcp = nullptr;
            AstCFunc* subFulFuncp = nullptr;
            AstCFunc* subChgFuncp = nullptr;
            uint32_t sub_Func_رقم = 0;
            صحیح subStmts = 0;
            ثابت uint32_t maxCodes = std::max((nAllCodes + parallelism - 1) / parallelism, 1U);
            uint32_t nCodes = 0;
            ثابت ActCodeSet* prevActSet = nullptr;
            AstIf* ifp = nullptr;
            uint32_t baseCode = 0;
            برای (; nCodes < maxCodes && it != traces.end(); ++it) {
                ثابت ActCodeSet& actSet = it->first;
                // Traced value never changes, no need to add it
                اگر (actSet.شمار(TraceActivityVertex::ACTIVITY_NEVER)) ادامه;

                ثابت TraceTraceVertex* ثابت vtxp = it->second;
                AstTraceDecl* ثابت declp = vtxp->nodep();

                // This is a duplicate decl, no need to add it, but must set the
                // function index to the same as the canonical node.
                اگر (ثابت TraceTraceVertex* ثابت canonVtxp = vtxp->duplicatep()) {
                    declp->fidx(canonVtxp->nodep()->fidx());
                    ادامه;
                }

                // Create top function if not yet created
                اگر (!بالاترین_Ful_Funcp) {
                    ++بالاترین_Func_رقم;
                    بالاترین_Ful_Funcp = newCFunc(VTraceType::FULL, nullptr, بالاترین_Func_رقم);
                    بالاترین_Chg_Funcp = newCFunc(VTraceType::CHANGE, nullptr, بالاترین_Func_رقم);
                }

                // Create new sub function if required
                اگر (!subFulFuncp || subStmts > splitLimit) {
                    baseCode = declp->code();
                    subStmts = 0;
                    subFulFuncp = newCFunc(VTraceType::FULL, بالاترین_Ful_Funcp, sub_Func_رقم, baseCode);
                    subChgFuncp = newCFunc(VTraceType::CHANGE, بالاترین_Chg_Funcp, sub_Func_رقم, baseCode);
                    ++sub_Func_رقم;
                    prevActSet = nullptr;
                    ifp = nullptr;
                }

                // If required, create the conditional node checking the activity flags
                اگر (!prevActSet || actSet != *prevActSet) {
                    File_سطر* ثابت flp = m_بالاترین_Scopep->fileline();
                    ثابت منطقی always = actSet.شمار(TraceActivityVertex::ACTIVITY_ALWAYS) != 0;
                    Ast_گره_عبارت* condp = nullptr;
                    اگر (always) {
                        condp = جدید AstConst{flp, 1};  // Always true, will be folded later
                    } وگرنه {
                        برای (ثابت uint32_t actCode : actSet) {
                            Ast_گره_عبارت* ثابت selp = selectActivity(flp, actCode, VAccess::READ);
                            condp = condp ? جدید AstOr{flp, condp, selp} : selp;
                        }
                    }
                    ifp = جدید AstIf{flp, condp};
                    اگر (!always) ifp->branchPred(VBranchPred::BP_UNLIKELY);
                    subChgFuncp->addStmtsp(ifp);
                    subStmts += ifp->گره_شمار();
                    prevActSet = &actSet;
                }

                // Add TraceInc nodes
                File_سطر* ثابت flp = declp->fileline();
                اگر (declp->dtypeCallp()) {
                    DtypeFuncs funcs = createNonConstDtypeTraceFunctions(declp);
                    Ast_گره_عبارت* argsp = nullptr;
                    argsp = Ast_گره::addNext(argsp, declp->valuep()->clone_درخت(نادرست));
                    AstCCall* ثابت callFullp = جدید AstCCall{flp, funcs.fullFuncp, argsp};
                    callFullp->dtypeSetVoid();
                    callFullp->argTypes(callFullp->argTypes() + "bufp, "
                                        + std::to_string(declp->code()));
                    subFulFuncp->addStmtsp(callFullp->makeStmt());
                    argsp = nullptr;
                    argsp = Ast_گره::addNext(argsp, declp->valuep()->clone_درخت(نادرست));
                    AstCCall* ثابت callChgp = جدید AstCCall{flp, funcs.chgFuncp, argsp};
                    callChgp->dtypeSetVoid();
                    callChgp->argTypes(callChgp->argTypes() + "bufp, "
                                       + std::to_string(declp->code()));
                    ifp->addThensp(callChgp->makeStmt());

                    subStmts += 2;
                } وگرنه {
                    AstTraceInc* ثابت incFulp = جدید AstTraceInc{flp, declp, VTraceType::FULL};
                    subFulFuncp->addStmtsp(incFulp);
                    AstTraceInc* ثابت incChgp
                        = جدید AstTraceInc{flp, declp, VTraceType::CHANGE, baseCode};
                    ifp->addThensp(incChgp);

                    // Set the function index of the decl
                    declp->fidx(بالاترین_Func_رقم);

                    // Track splitting due to size
                    UASSERT_OBJ(incFulp->گره_شمار() == incChgp->گره_شمار(), declp,
                                "Should have equal cost");
                    ثابت VNumRange range = declp->arrayRange();
                    اگر (range.ranged()) {
                        // 2x because each element is a TraceInc and a VarRef
                        subStmts += range.elements() * 2;
                    } وگرنه {
                        subStmts += incChgp->گره_شمار();
                    }
                }

                // Track partitioning
                nCodes += declp->codeInc();
            }
        }
    }

    تهی createCleanupFunction() {
        File_سطر* ثابت fl = m_بالاترین_Scopep->fileline();
        AstCFunc* ثابت cleanupFuncp = جدید AstCFunc{fl, "trace_cleanup", m_بالاترین_Scopep};
        cleanupFuncp->argTypes("void* voidSelf, " + v3Global.opt.traceClassBase()
                               + "* /*unused*/");
        cleanupFuncp->isTrace(درست);
        cleanupFuncp->slow(نادرست);
        cleanupFuncp->isStatic(درست);
        cleanupFuncp->isLoose(درست);
        m_بالاترین_Scopep->addBlocksp(cleanupFuncp);
        cleanupFuncp->addStmtsp(جدید AstCStmt{fl, EmitCUtil::voidSelfAssign(m_بالاترین_Modp)});
        cleanupFuncp->addStmtsp(جدید AstCStmt{fl, EmitCUtil::symClassAssign()});

        // Register it
        {
            AstCStmt* ثابت cstmtp = جدید AstCStmt{fl};
            m_regFuncp->addStmtsp(cstmtp);
            cstmtp->add("tracep->addCleanupCb(");
            cstmtp->add(جدید AstAddrOfCFunc{fl, cleanupFuncp});
            cstmtp->add(", vlSelf);");
        }

        // Clear global activity flag
        cleanupFuncp->addStmtsp(
            جدید AstCStmt{m_بالاترین_Scopep->fileline(), "vlSymsp->__Vm_activity = false;"s});

        // Clear fine grained activity flags
        برای (uint32_t i = 0; i < m_activity_عدد; ++i) {
            Ast_گره* ثابت clrp = جدید AstAssign{fl, selectActivity(fl, i, VAccess::WRITE),
                                                جدید AstConst{fl, AstConst::BitFalse{}}};
            cleanupFuncp->addStmtsp(clrp);
        }
    }

    تهی createTraceFunctions() {
        // Detect and remove duplicate values
        detectDuplicates();

        graphDtypePrune();
        splitTraceDeclFuncs();
        m_graph.removeRedundantEdgesMax(&V3GraphEdge::followAlwaysTrue);

        // Simplify & optimize the graph
        اگر (dumpGraphLevel() >= 6) m_graph.dumpDotFilePrefixed("trace_pre");
        graphSimplify(درست);
        اگر (dumpGraphLevel() >= 6) m_graph.dumpDotFilePrefixed("trace_simplified");
        graphOptimize();
        اگر (dumpGraphLevel() >= 6) m_graph.dumpDotFilePrefixed("trace_optimized");

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
        m_regFuncp = جدید AstCFunc{m_بالاترین_Scopep->fileline(), "trace_register", m_بالاترین_Scopep};
        m_regFuncp->argTypes(v3Global.opt.traceClassBase() + "* tracep");
        m_regFuncp->entryPoint(درست);
        m_regFuncp->isTrace(درست);
        m_regFuncp->slow(درست);
        m_regFuncp->isStatic(نادرست);
        m_regFuncp->isLoose(درست);
        m_بالاترین_Scopep->addBlocksp(m_regFuncp);

        // Create the const dump functions. Also allocates trace codes.
        createConstTraceFunctions(traces);

        // Create the full and incremental dump functions
        createNonConstTraceFunctions(traces, nNonConstCodes, m_parallelism);

        // Root-traced libraries alias wrapper IOs onto the existing top-module codes.
        اگر (!v3Global.opt.libCreate().empty()) {
            std::vector<AstTraceDecl*> ریشه_Declps;
            std::vector<AstTraceDecl*> بالاترین_Declps;
            TraceInitDeclCollector ریشه_Collector{ریشه_Declps};
            TraceInitDeclCollector بالاترین_Collector{بالاترین_Declps};
            برای (Ast_گره* blockp = m_بالاترین_Scopep->blocksp(); blockp; blockp = blockp->nextp()) {
                AstCFunc* ثابت funcp = VN_CAST(blockp, CFunc);
                اگر (funcp && VString::startsWith(funcp->name(), "trace_init_leaf_root__")) {
                    ریشه_Collector.collect(funcp);
                } وگرنه اگر (funcp && VString::startsWith(funcp->name(), "trace_init_leaf_top__")) {
                    بالاترین_Collector.collect(funcp);
                }
            }

            std::vector<منطقی> used(بالاترین_Declps.size(), نادرست);
            برای (AstTraceDecl* ثابت ریشه_Declp : ریشه_Declps) {
                برای (size_t i = 0; i < بالاترین_Declps.size(); ++i) {
                    اگر (used[i] || !same_ریشه_Init_Alias(ریشه_Declp, بالاترین_Declps[i])) ادامه;
                    ریشه_Declp->code(بالاترین_Declps[i]->code());
                    used[i] = درست;
                    بشکن;
                }
            }
        }

        // Remove refs to traced values from TraceDecl nodes, these have now moved under
        // TraceInc
        برای (ثابت auto& i : traces) {
            Ast_گره* ثابت valuep = i.second->nodep()->valuep();
            VL_DO_DANGLING(valuep->unlinkFrBack()->delete_درخت(), valuep);
        }

        // Create the trace cleanup function clearing the activity flags
        createCleanupFunction();
    }

    TraceCFuncVertex* getCFuncVertexp(AstCFunc* nodep) {
        V3GraphVertex* ثابت vtxp = nodep->user1u().toGraphVertex();
        TraceCFuncVertex* vertexp = vtxp ? vtxp->cast<TraceCFuncVertex>() : nullptr;
        اگر (!vertexp) {
            vertexp = جدید TraceCFuncVertex{&m_graph, nodep};
            nodep->user1p(vertexp);
        }
        بازگشت vertexp;
    }
    TraceActivityVertex* getActivityVertexp(Ast_گره* nodep, منطقی slow) {
        V3GraphVertex* ثابت vtxp = nodep->user3u().toGraphVertex();
        TraceActivityVertex* vertexp = vtxp ? vtxp->cast<TraceActivityVertex>() : nullptr;
        اگر (!vertexp) {
            vertexp = جدید TraceActivityVertex{&m_graph, nodep, slow};
            nodep->user3p(vertexp);
        }
        vertexp->slow(slow);
        بازگشت vertexp;
    }

    // VISITORS
    تهی visit(AstNetlist* nodep) override {
        // Add vertexes for all TraceDecl, and edges from VARs each trace looks at
        m_finding = نادرست;
        iterateChildren(nodep);

        // Add vertexes for all CFUNCs, and edges to VARs the func sets
        m_finding = درست;
        iterateChildren(nodep);

        // Create the trace functions and insert them into the tree
        createTraceFunctions();

        // Save number of trace codes used
        nodep->nTraceCodes(m_code);
    }
    تهی visit(Ast_گره_Module* nodep) override {
        اگر (nodep->is_بالاترین()) m_بالاترین_Modp = nodep;
        iterateChildren(nodep);
    }
    تهی visit(AstVarScope* nodep) override {
        اگر (!m_finding) {
            اگر (ثابت AstIface* ثابت ifacep = nodep->varp()->sensIfacep()) {
                m_ifaceMemberVscps[{ifacep, nodep->varp()->name()}].افزودن_back(nodep);
            }
        }
    }
    تهی visit(Ast_Stmt_عبارت* nodep) override {
        اگر (!m_finding && !nodep->user2()) {
            اگر (AstCCall* ثابت callp = VN_CAST(nodep->exprp(), CCall)) {
                UINFO(8, "   CCALL " << callp);
                // See if there are other calls in same statement list;
                // If so, all funcs might share the same activity code
                TraceActivityVertex* ثابت activityVtxp
                    = getActivityVertexp(nodep, callp->funcp()->slow());
                برای (Ast_گره* nextp = nodep; nextp; nextp = nextp->nextp()) {
                    اگر (Ast_Stmt_عبارت* ثابت stmtp = VN_CAST(nextp, Stmt_عبارت)) {
                        اگر (AstCCall* ثابت ccallp = VN_CAST(stmtp->exprp(), CCall)) {
                            stmtp->user2(درست);  // Processed
                            UINFO(8, "     SubCCALL " << ccallp);
                            V3GraphVertex* ثابت ccallFuncVtxp = getCFuncVertexp(ccallp->funcp());
                            activityVtxp->slow(ccallp->funcp()->slow());
                            جدید V3GraphEdge{&m_graph, activityVtxp, ccallFuncVtxp, 1};
                        }
                    }
                }
            }
        }
        iterateChildren(nodep);
    }
    تهی visit(AstCFunc* nodep) override {
        UINFO(8, "   CFUNC " << nodep);
        V3GraphVertex* ثابت funcVtxp = getCFuncVertexp(nodep);
        اگر (!m_finding) {  // If public, we need a unique activity code to allow for sets
                           // directly in this func
            اگر (nodep->funcPublic() || nodep->dpiExportImpl() || nodep->entryPoint()
                || nodep->isCoroutine()) {
                // Cannot treat a coroutine as slow, it may be resumed later
                ثابت منطقی slow = nodep->slow() && !nodep->isCoroutine();
                TraceActivityVertex* ثابت activityVtxp = getActivityVertexp(nodep, slow);
                جدید V3GraphEdge{&m_graph, activityVtxp, funcVtxp, 1};
            }
        }
        VL_RESTORER(m_cfuncp);
        m_cfuncp = nodep;
        iterateChildren(nodep);
    }
    تهی visit(AstTraceDecl* nodep) override {
        UINFO(8, "   TRACE " << nodep);
        اگر (!m_finding && !nodep->inDtypeFunc()) {
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
            اگر (m_cfuncp && VString::startsWith(m_cfuncp->name(), "trace_init_leaf_root__")) {
                بازگشت;
            }
            V3GraphVertex* ثابت vertexp = جدید TraceTraceVertex{&m_graph, nodep};
            nodep->user1p(vertexp);

            UASSERT_OBJ(m_cfuncp, nodep, "Trace not under func");
            m_cfuncp->user2(درست);
            VL_RESTORER(m_tracep);
            m_tracep = nodep;
            iterateChildren(nodep);
        }
    }
    تهی visit(AstVarRef* nodep) override {
        UASSERT_OBJ(nodep->varScopep(), nodep, "No var scope?");
        AstVarScope* ثابت varscopep = nodep->varScopep();
        V3GraphVertex* varVtxp = varscopep->user1u().toGraphVertex();
        اگر (m_tracep) {
            UASSERT_OBJ(nodep->access().isReadOnly(), nodep, "Lvalue in trace?  Should be const.");
            اگر (!varVtxp) {
                varVtxp = جدید TraceVarVertex{&m_graph, nodep->varScopep()};
                nodep->varScopep()->user1p(varVtxp);
            }
            V3GraphVertex* ثابت traceVtxp = m_tracep->user1u().toGraphVertex();
            جدید V3GraphEdge{&m_graph, varVtxp, traceVtxp, 1};
            اگر (nodep->varp()->isPrimaryInish()  // Always need to trace primary inputs
                || nodep->varp()->isSigPublic()) {  // Or ones user can change
                جدید V3GraphEdge{&m_graph, m_alwaysVtxp, traceVtxp, 1};
            }
            اگر (m_tracep->dtypeCallp()) varscopep->user2p(m_tracep);
        } وگرنه اگر (m_cfuncp && m_finding && nodep->access().isWriteOrRW()) {
            V3GraphVertex* ثابت funcVtxp = getCFuncVertexp(m_cfuncp);
            اگر (varVtxp) {  // else we're not tracing this signal
                جدید V3GraphEdge{&m_graph, funcVtxp, varVtxp, 1};
                AstTraceDecl* ثابت declp = VN_AS(varscopep->user2p(), TraceDecl);
                اگر (declp) {
                    V3GraphVertex* ثابت cFuncVtxp = getCFuncVertexp(m_cfuncp);
                    برای (ثابت V3GraphEdge& edge : cFuncVtxp->inEdges()) {
                        V3GraphVertex* ثابت activityp = edge.fromp();
                        اگر (!declp->user3p()) {
                            declp->user3p(activityp);
                        } وگرنه اگر (declp->user3u().toGraphVertex() != activityp) {
                            declp->user2(درست);
                        }
                    }
                }
            }
        }
    }
    تهی visit(AstMemberSel* nodep) override {
        اگر (m_cfuncp && m_finding && nodep->access().isWriteOrRW()) {
            AstIfaceRefDType* ثابت dtypep
                = VN_CAST(nodep->fromp()->dtypep()->skipRefp(), IfaceRefDType);
            اگر (dtypep && dtypep->isVirtual()) {
                ثابت auto it = m_ifaceMemberVscps.find({dtypep->ifacep(), nodep->varp()->name()});
                اگر (it != m_ifaceMemberVscps.end()) {
                    V3GraphVertex* ثابت funcVtxp = getCFuncVertexp(m_cfuncp);
                    برای (AstVarScope* ثابت vscp : it->second) {
                        V3GraphVertex* varVtxp = vscp->user1u().toGraphVertex();
                        اگر (!varVtxp) {
                            varVtxp = جدید TraceVarVertex{&m_graph, vscp};
                            vscp->user1p(varVtxp);
                        }
                        جدید V3GraphEdge{&m_graph, funcVtxp, varVtxp, 1};
                    }
                }
            }
        }
        iterateChildren(nodep);
    }
    //--------------------
    تهی visit(Ast_گره* nodep) override { iterateChildren(nodep); }

عمومی:
    // CONSTRUCTORS
    explicit TraceVisitor(AstNetlist* nodep)
        : m_alwaysVtxp{جدید TraceActivityVertex{&m_graph, TraceActivityVertex::ACTIVITY_ALWAYS}} {
        nodep->user2_Clear_درخت();  // TraceDecl multiple activities flag
        nodep->user3_Clear_درخت();  // TraceDecl TraceActivityVertex (assumes we start at nullptr)
        iterate(nodep);
        nodep->foreach([](AstTraceDecl* ثابت declp) {
            اگر (declp->inDtypeFunc()) {
                declp->valuep()->unlinkFrBack()->delete_درخت();
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

تهی V3Trace::traceAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { TraceVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_درخت("trace", 0, dump_درخت_Either_Level() >= 3);
}
