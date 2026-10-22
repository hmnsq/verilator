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

#அந்தர்பாவ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#அந்தர்பாவ "V3Trace.h"

#அந்தர்பாவ "V3Ast.h"
#அந்தர்பாவ "V3DupFinder.h"
#அந்தர்பாவ "V3EmitCBase.h"
#அந்தர்பாவ "V3Graph.h"
#அந்தர்பாவ "V3Stats.h"
#அந்தர்பாவ "V3UniqueNames.h"

#அந்தர்பாவ <limits>
#அந்தர்பாவ <map>
#அந்தர்பாவ <set>
#அந்தர்பாவ <unordered_map>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Graph vertexes

வகுப்பு TraceActivityVertex final : பொது V3GraphVertex {
    VL_RTTI_IMPL(TraceActivityVertex, V3GraphVertex)
    Ast_முனை* மாறிலி m_insertp;
    int32_t m_activityCode;
    தருக்க m_slow;  // If always slow, we can use the same code
பொது:
    எண்ணிக்கை { ACTIVITY_NEVER = ((1UL << 31) - 1) };
    எண்ணிக்கை { ACTIVITY_ALWAYS = ((1UL << 31) - 2) };
    எண்ணிக்கை { ACTIVITY_SLOW = 0 };
    TraceActivityVertex(V3Graph* graphp, Ast_முனை* nodep, தருக்க slow)
        : V3GraphVertex{graphp}
        , m_insertp{nodep} {
        m_activityCode = 0;
        m_slow = slow;
    }
    TraceActivityVertex(V3Graph* graphp, int32_t code)
        : V3GraphVertex{graphp}
        , m_insertp{nullptr} {
        m_activityCode = code;
        m_slow = பொய்;
    }
    ~TraceActivityVertex() override = இயல்பு;
    // ACCESSORS
    Ast_முனை* insertp() மாறிலி {
        UASSERT(m_insertp, "Null insertp; probably called on a special always/slow");
        திரும்பு m_insertp;
    }
    string name() மாறிலி override {
        எனில் (activityAlways()) {
            திரும்பு "*ALWAYS*";
        } இல்லையெனில் {
            திரும்பு std::string{slow() ? "*SLOW* " : ""} + insertp()->name();
        }
    }
    string dotColor() மாறிலி override { திரும்பு slow() ? "yellowGreen" : "green"; }
    int32_t activityCode() மாறிலி { திரும்பு m_activityCode; }
    தருக்க activityAlways() மாறிலி { திரும்பு activityCode() == ACTIVITY_ALWAYS; }
    தருக்க activitySlow() மாறிலி { திரும்பு activityCode() == ACTIVITY_SLOW; }
    வெறுமை activityCode(int32_t code) { m_activityCode = code; }
    தருக்க slow() மாறிலி { திரும்பு m_slow; }
    வெறுமை slow(தருக்க flag) {
        எனில் (!flag) m_slow = பொய்;
    }
};

வகுப்பு TraceCFuncVertex final : பொது V3GraphVertex {
    VL_RTTI_IMPL(TraceCFuncVertex, V3GraphVertex)
    AstCFunc* மாறிலி m_nodep;

பொது:
    TraceCFuncVertex(V3Graph* graphp, AstCFunc* nodep)
        : V3GraphVertex{graphp}
        , m_nodep{nodep} {}
    ~TraceCFuncVertex() override = இயல்பு;
    // ACCESSORS
    AstCFunc* nodep() மாறிலி { திரும்பு m_nodep; }
    string name() மாறிலி override { திரும்பு nodep()->name(); }
    string dotColor() மாறிலி override { திரும்பு "yellow"; }
    File_வரி* fileline() மாறிலி override { திரும்பு nodep()->fileline(); }
};

வகுப்பு TraceTraceVertex final : பொது V3GraphVertex {
    VL_RTTI_IMPL(TraceTraceVertex, V3GraphVertex)
    AstTraceDecl* மாறிலி m_nodep;  // TRACEINC this represents
    // nullptr, or other vertex with the real code() that duplicates this one
    TraceTraceVertex* m_duplicatep = nullptr;
    // When aliasing to a dtype parent, offset of the member within the dtype
    uint32_t m_dtypeAliasOffset = 0;

பொது:
    TraceTraceVertex(V3Graph* graphp, AstTraceDecl* nodep)
        : V3GraphVertex{graphp}
        , m_nodep{nodep} {}
    ~TraceTraceVertex() override = இயல்பு;
    // ACCESSORS
    AstTraceDecl* nodep() மாறிலி { திரும்பு m_nodep; }
    string name() மாறிலி override { திரும்பு nodep()->name(); }
    string dotColor() மாறிலி override { திரும்பு "red"; }
    File_வரி* fileline() மாறிலி override { திரும்பு nodep()->fileline(); }
    TraceTraceVertex* duplicatep() மாறிலி { திரும்பு m_duplicatep; }
    வெறுமை duplicatep(TraceTraceVertex* dupp) {
        UASSERT_OBJ(!duplicatep(), nodep(), "Assigning duplicatep() to already duplicated node");
        m_duplicatep = dupp;
    }
    வெறுமை redirectDuplicatep(TraceTraceVertex* dupp) { m_duplicatep = dupp; }
    uint32_t dtypeAliasOffset() மாறிலி { திரும்பு m_dtypeAliasOffset; }
    வெறுமை dtypeAliasOffset(uint32_t offset) { m_dtypeAliasOffset = offset; }
};

வகுப்பு TraceVarVertex final : பொது V3GraphVertex {
    VL_RTTI_IMPL(TraceVarVertex, V3GraphVertex)
    AstVarScope* மாறிலி m_nodep;

பொது:
    TraceVarVertex(V3Graph* graphp, AstVarScope* nodep)
        : V3GraphVertex{graphp}
        , m_nodep{nodep} {}
    ~TraceVarVertex() override = இயல்பு;
    // ACCESSORS
    AstVarScope* nodep() மாறிலி { திரும்பு m_nodep; }
    string name() மாறிலி override { திரும்பு nodep()->name(); }
    string dotColor() மாறிலி override { திரும்பு "skyblue"; }
    File_வரி* fileline() மாறிலி override { திரும்பு nodep()->fileline(); }
};

//######################################################################
// Trace state, as a visitor of each AstNode

வகுப்பு TraceVisitor final : பொது VNVisitor {
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
    மாறிலி VNUser1InUse m_inuser1;
    மாறிலி VNUser2InUse m_inuser2;
    மாறிலி VNUser3InUse m_inuser3;
    // VNUser4InUse     In V3Hasher via V3DupFinder

    // STATE
    Ast_முனை_Module* m_உச்சி_Modp = nullptr;  // Module to add variables to
    AstScope* மாறிலி m_உச்சி_Scopep = v3Global.rootp()->உச்சி_Scopep()->scopep();  // The top AstScope
    AstCFunc* m_cfuncp = nullptr;  // C function adding to graph
    AstCFunc* m_regFuncp = nullptr;  // Trace registration function
    AstCFunc* m_actAllFuncp = nullptr;  // Set all activity function
    AstTraceDecl* m_tracep = nullptr;  // Trace function adding to graph
    AstVarScope* m_activityVscp = nullptr;  // Activity variable
    uint32_t m_activity_எண் = 0;  // Count of fields in activity variable
    uint32_t m_code = 0;  // Trace ident code# being assigned
    V3Graph m_graph;  // Var/CFunc tracking
    TraceActivityVertex* மாறிலி m_alwaysVtxp;  // "Always trace" vertex
    தருக்க m_finding = பொய்;  // Pass one of algorithm?
    கட்டமைப்பு DtypeFuncs final {
    பொது:
        AstCFunc* fullFuncp = nullptr;
        AstCFunc* chgFuncp = nullptr;
    };
    std::unordered_map<மாறிலி Ast_முனை_DType*, DtypeFuncs>
        m_dtypeNonConstFuncs;  // Full / Chg funcs per type
    std::unordered_map<மாறிலி Ast_முனை_DType*, AstCFunc*> m_dtypeConstFuncs;  // Const func per type
    V3UniqueNames m_dtypeNames{""};  // Unique type func names

    // Trace parallelism. Only VCD tracing can be parallelized at this time.
    மாறிலி uint32_t m_parallelism
        = v3Global.opt.useTraceParallel() ? static_cast<uint32_t>(v3Global.opt.threads()) : 1;

    VDouble0 m_statSetters;  // Statistic tracking
    VDouble0 m_statSettersSlow;  // Statistic tracking
    VDouble0 m_statUniqSigs;  // Statistic tracking

    // All activity numbers applying to a given trace
    using ActCodeSet = std::set<uint32_t>;
    // For activity set, what traces apply
    using TraceVec = std::multimap<ActCodeSet, TraceTraceVertex*>;
    // Candidate interface-member VarScopes keyed by (interface type, member name)
    std::map<std::pair<மாறிலி AstIface*, std::string>, std::vector<AstVarScope*>>
        m_ifaceMemberVscps;

    வகுப்பு TraceInitDeclCollector final : பொது VNVisitor {
        std::vector<AstTraceDecl*>& m_declps;  // Output: trace declarations found so far
        std::set<மாறிலி AstCFunc*> m_seenFuncps;

        வெறுமை visit(AstTraceDecl* nodep) override { m_declps.தள்ளு_back(nodep); }
        வெறுமை visit(AstCCall* nodep) override {
            எனில் (AstCFunc* மாறிலி funcp = nodep->funcp()) collect(funcp);
        }
        வெறுமை visit(Ast_முனை* nodep) override { iterateChildren(nodep); }

    பொது:
        explicit TraceInitDeclCollector(std::vector<AstTraceDecl*>& declps)
            : m_declps{declps} {}
        வெறுமை collect(AstCFunc* funcp) {
            எனில் (funcp && m_seenFuncps.செருகு(funcp).second) iterate(funcp);
        }
    };

    // METHODS

    நிலையான தருக்க same_வேர்_Init_Alias(மாறிலி AstTraceDecl* வேர்_Declp, மாறிலி AstTraceDecl* உச்சி_Declp) {
        மாறிலி VNumRange& வேர்_Bit_Range = வேர்_Declp->bitRange();
        மாறிலி VNumRange& உச்சி_Bit_Range = உச்சி_Declp->bitRange();
        திரும்பு வேர்_Declp->showname() == உச்சி_Declp->showname()
               && வேர்_Declp->declDirection() == உச்சி_Declp->declDirection()
               && வேர்_Declp->widthMin() == உச்சி_Declp->widthMin()
               && வேர்_Bit_Range.ranged() == உச்சி_Bit_Range.ranged()
               && (!வேர்_Bit_Range.ranged()
                   || (வேர்_Bit_Range.இடது() == உச்சி_Bit_Range.இடது()
                       && வேர்_Bit_Range.வலது() == உச்சி_Bit_Range.வலது()));
    }

    வெறுமை detectDuplicates() {
        UINFO(9, "Finding duplicates");
        // Note uses user4
        V3DupFinder dupFinder;  // Duplicate code detection

        // Compute member offsets within dtype instances incrementally.
        // For each dtype member TraceDecl, stores its code offset within the
        // dtype parent's code range.
        std::unordered_map<மாறிலி AstTraceDecl*, uint32_t> memberOffsets;
        std::unordered_map<மாறிலி AstTraceDecl*, uint32_t> runningOffset;

        // For fixup pass: track dups redirected to dtype parents, so we can
        // fix them if the parent's user2 is later set during the loop.
        // Maps dup vertex -> original canonical member vertex.
        std::vector<std::pair<TraceTraceVertex*, TraceTraceVertex*>> dtypeParentRedirects;

        // Hash all of the traced values and find if there are any duplicates
        க்கு (V3GraphVertex& vtx : m_graph.vertices()) {
            எனில் (TraceTraceVertex* மாறிலி vvertexp = vtx.cast<TraceTraceVertex>()) {
                AstTraceDecl* மாறிலி nodep = vvertexp->nodep();
                எனில் (nodep->dtypeCallp()) தொடர்;

                எனில் (nodep->dtypeDeclp()) {
                    uint32_t& offset = runningOffset[nodep->dtypeDeclp()];
                    memberOffsets[nodep] = offset;
                    offset += nodep->codeInc();
                }

                UASSERT_OBJ(!vvertexp->duplicatep(), nodep, "Should not be a duplicate");
                மாறிலி auto dupit = dupFinder.findDuplicate(nodep);
                எனில் (dupit == dupFinder.end()) {
                    dupFinder.செருகு(nodep);
                } இல்லையெனில் {
                    AstTraceDecl* மாறிலி dupDeclp = VN_AS(dupit->second, TraceDecl);
                    UASSERT_OBJ(dupDeclp, nodep, "Trace duplicate of wrong type");
                    TraceTraceVertex* மாறிலி dupvertexp
                        = dupDeclp->user1u().toGraphVertex()->cast<TraceTraceVertex>();
                    UINFO(8, "  Orig " << dupDeclp << endl);
                    UINFO(8, "   dup " << nodep << endl);

                    எனில் (dupDeclp->dtypeDeclp() && !dupDeclp->dtypeDeclp()->user2()) {
                        AstTraceDecl* மாறிலி dtypeParentp = dupDeclp->dtypeDeclp();
                        TraceTraceVertex* மாறிலி dtypeVtxp
                            = dtypeParentp->user1u().toGraphVertex()->cast<TraceTraceVertex>();
                        மாறிலி auto it2 = memberOffsets.find(dupDeclp);
                        UASSERT_OBJ(it2 != memberOffsets.end(), dupDeclp,
                                    "Member offset not precomputed");
                        vvertexp->duplicatep(dtypeVtxp);
                        vvertexp->dtypeAliasOffset(it2->second);
                        dtypeParentRedirects.emplace_back(vvertexp, dupvertexp);
                    } இல்லையெனில் {
                        vvertexp->duplicatep(dupvertexp);
                    }
                    எனில் (nodep->dtypeDeclp()) nodep->dtypeDeclp()->user2(உண்மை);
                }
            }
        }

        க்கு (மாறிலி auto& pair : dtypeParentRedirects) {
            TraceTraceVertex* மாறிலி dupVtxp = pair.first;
            TraceTraceVertex* மாறிலி memberVtxp = pair.second;
            மாறிலி AstTraceDecl* மாறிலி parentDeclp = dupVtxp->duplicatep()->nodep();
            எனில் (parentDeclp->user2()) {
                dupVtxp->redirectDuplicatep(memberVtxp);
                dupVtxp->dtypeAliasOffset(0);
            }
        }

        எனில் (dumpLevel() || debug() >= 9)
            dupFinder.dumpFile(v3Global.debugFilename("trace") + ".hash", பொய்);
    }

    வெறுமை graphDtypePrune() {
        க்கு (V3GraphVertex* மாறிலி vtxp : m_graph.vertices().unlinkable()) {
            எனில் (TraceTraceVertex* மாறிலி vvertexp = vtxp->cast<TraceTraceVertex>()) {
                AstTraceDecl* மாறிலி declp = vvertexp->nodep();
                // This skips the dtype sub-func optimization if a var is affected by multiple
                // activities.  We really only need to do this for _chg funcs (and not decls,
                // _const and _full) but it's simpiler to do it all one way or the other.
                எனில் (declp) {
                    எனில் (declp->user2() || (declp->dtypeDeclp() && !declp->dtypeDeclp()->user2())) {
                        AstCCall* மாறிலி callp = declp->dtypeCallp();
                        எனில் (callp) {
                            Ast_முனை* stmtexprp = callp->backp();
                            VL_DO_DANGLING(தள்ளு_Deletep(stmtexprp->unlinkFrBack()), stmtexprp);
                        } இல்லையெனில் {
                            தருக்க emptyScope;
                            செய் {
                                emptyScope = பொய்;
                                Ast_முனை* மாறிலி declBackp = declp->backp();
                                Ast_முனை* மாறிலி declNextp = declp->nextp();
                                எனில் (VN_IS(declBackp, Trace_தள்ளு_Prefix)
                                    && VN_IS(declNextp, Trace_எடு_Prefix)) {
                                    VL_DO_DANGLING(தள்ளு_Deletep(declBackp->unlinkFrBack()),
                                                   declBackp);
                                    VL_DO_DANGLING(தள்ளு_Deletep(declNextp->unlinkFrBack()),
                                                   declNextp);
                                    emptyScope = உண்மை;
                                }
                            } போது (emptyScope);
                        }
                        // Can't purge until we finish this pass
                        தள்ளு_Deletep(declp->unlinkFrBack());  // declp used below
                        vvertexp->rerouteEdges(&m_graph);
                        vvertexp->unlinkDelete(&m_graph);
                    }
                    declp->dtypeDeclp(nullptr);
                }
            }
        }
    }

    வெறுமை splitTraceDeclFuncs() {
        மாறிலி முழு splitLimit = v3Global.opt.outputSplitCTrace() ? v3Global.opt.outputSplitCTrace()
                                                                : std::numeric_limits<முழு>::max();
        எனில் (splitLimit == std::numeric_limits<முழு>::max()) திரும்பு;

        க்கு (Ast_முனை* nodep = m_உச்சி_Scopep->blocksp(); nodep; nodep = nodep->nextp()) {
            AstCFunc* மாறிலி funcp = VN_CAST(nodep, CFunc);
            எனில் (!funcp || !funcp->user2()) தொடர்;

            மாறிலி string baseName = funcp->name();
            uint32_t sub_இலக்கம் = 1;
            முழு curSize = 0;
            AstCFunc* curFuncp = funcp;
            std::vector<Ast_முனை_Stmt*> callStmts;

            auto startNewSubFunc = [&]() {
                File_வரி* மாறிலி flp = funcp->fileline();
                மாறிலி string newName = baseName + "_" + cvtToStr(sub_இலக்கம்++);
                AstCFunc* மாறிலி newFuncp = புதிய AstCFunc{flp, newName, m_உச்சி_Scopep};
                newFuncp->argTypes(v3Global.opt.traceClassBase() + "* tracep");
                newFuncp->isTrace(உண்மை);
                newFuncp->isStatic(பொய்);
                newFuncp->isLoose(உண்மை);
                newFuncp->slow(உண்மை);
                newFuncp->dontCombine(உண்மை);
                m_உச்சி_Scopep->addBlocksp(newFuncp);
                newFuncp->addStmtsp(புதிய AstCStmt{flp, "const int c = vlSymsp->__Vm_baseCode;"});
                AstCCall* மாறிலி callp = புதிய AstCCall{flp, newFuncp};
                callp->dtypeSetVoid();
                callp->argTypes("tracep");
                callStmts.தள்ளு_back(callp->makeStmt());
                curFuncp = newFuncp;
                curSize = 0;
            };

            க்கு (Ast_முனை *stmtp = funcp->stmtsp(), *nextp; stmtp; stmtp = nextp) {
                nextp = stmtp->nextp();
                curSize += stmtp->முனை_மொத்தம்();

                எனில் (curFuncp != funcp) {
                    stmtp->unlinkFrBack();
                    curFuncp->addStmtsp(stmtp);
                }

                எனில் (curSize > splitLimit && nextp) startNewSubFunc();
            }

            க்கு (Ast_முனை_Stmt* மாறிலி callStmtp : callStmts) { funcp->addStmtsp(callStmtp); }
        }
    }

    வெறுமை graphSimplify(தருக்க initial) {
        எனில் (initial) {
            // Remove all variable nodes
            க்கு (V3GraphVertex* மாறிலி vtxp : m_graph.vertices().unlinkable()) {
                எனில் (TraceVarVertex* மாறிலி vvertexp = vtxp->cast<TraceVarVertex>()) {
                    vvertexp->rerouteEdges(&m_graph);
                    vvertexp->unlinkDelete(&m_graph);
                }
            }
            // Remove multiple variables connecting funcs to traces
            // We do this twice, as then we have fewer edges to multiply out in the below
            // expansion.
            m_graph.removeRedundantEdgesMax(&V3GraphEdge::followAlwaysTrue);
            // Remove all Cfunc nodes
            க்கு (V3GraphVertex* மாறிலி vtxp : m_graph.vertices().unlinkable()) {
                எனில் (TraceCFuncVertex* மாறிலி vvertexp = vtxp->cast<TraceCFuncVertex>()) {
                    vvertexp->rerouteEdges(&m_graph);
                    vvertexp->unlinkDelete(&m_graph);
                }
            }
        }

        // Remove multiple variables connecting funcs to traces
        m_graph.removeRedundantEdgesMax(&V3GraphEdge::followAlwaysTrue);

        // If there are any edges from a always, keep only the always
        க்கு (V3GraphVertex& vtx : m_graph.vertices()) {
            எனில் (TraceTraceVertex* மாறிலி vvertexp = vtx.cast<TraceTraceVertex>()) {
                // Search for the incoming always edge
                மாறிலி V3GraphEdge* alwaysEdgep = nullptr;
                க்கு (மாறிலி V3GraphEdge& edge : vvertexp->inEdges()) {
                    மாறிலி TraceActivityVertex* மாறிலி actVtxp
                        = edge.fromp()->as<மாறிலி TraceActivityVertex>();
                    எனில் (actVtxp->activityAlways()) {
                        alwaysEdgep = &edge;
                        நிறுத்து;
                    }
                }
                // If always edge exists, remove all other edges
                எனில் (alwaysEdgep) {
                    க்கு (V3GraphEdge* மாறிலி edgep : vvertexp->inEdges().unlinkable()) {
                        எனில் (edgep != alwaysEdgep) VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
                    }
                }
            }
        }

        // Activity points with no outputs can be removed
        க்கு (V3GraphVertex* மாறிலி vtxp : m_graph.vertices().unlinkable()) {
            எனில் (TraceActivityVertex* மாறிலி aVtxp = vtxp->cast<TraceActivityVertex>()) {
                // Leave in the always vertex for later use.
                எனில் (aVtxp != m_alwaysVtxp && aVtxp->outEmpty()) {
                    VL_DO_DANGLING(aVtxp->unlinkDelete(&m_graph), aVtxp);
                }
            }
        }
    }

    uint32_t assignactivityNumbers() {
        uint32_t activity_எண் = 1;  // Note 0 indicates "slow" only
        க்கு (V3GraphVertex& vtx : m_graph.vertices()) {
            எனில் (TraceActivityVertex* மாறிலி vvertexp = vtx.cast<TraceActivityVertex>()) {
                எனில் (vvertexp != m_alwaysVtxp) {
                    எனில் (vvertexp->slow()) {
                        vvertexp->activityCode(TraceActivityVertex::ACTIVITY_SLOW);
                    } இல்லையெனில் {
                        vvertexp->activityCode(activity_எண்++);
                    }
                }
            }
        }
        திரும்பு activity_எண்;
    }

    வெறுமை sortTraces(TraceVec& traces, uint32_t& nNonConstCodes) {
        // Populate sort structure
        traces.clear();
        nNonConstCodes = 0;
        க்கு (V3GraphVertex& vtx : m_graph.vertices()) {
            எனில் (TraceTraceVertex* மாறிலி vtxp = vtx.cast<TraceTraceVertex>()) {
                ActCodeSet actSet;
                UINFO(9, "  Add to sort: " << vtxp);
                UINFOTREE(9, vtxp->nodep(), "", "trnode");
                க்கு (மாறிலி V3GraphEdge& edge : vtxp->inEdges()) {
                    மாறிலி TraceActivityVertex* மாறிலி cfvertexp
                        = edge.fromp()->cast<மாறிலி TraceActivityVertex>();
                    UASSERT_OBJ(cfvertexp, vtxp->nodep(),
                                "Should have been function pointing to this trace");
                    UINFO(9, "   Activity: " << cfvertexp);
                    எனில் (cfvertexp->activityAlways()) {
                        // If code 0, we always trace; ignore other codes
                        actSet.செருகு(TraceActivityVertex::ACTIVITY_ALWAYS);
                    } இல்லையெனில் {
                        actSet.செருகு(cfvertexp->activityCode());
                    }
                }
                UASSERT_OBJ(actSet.மொத்தம்(TraceActivityVertex::ACTIVITY_ALWAYS) == 0
                                || actSet.size() == 1,
                            vtxp->nodep(), "Always active trace has further triggers");
                // Count nodes
                எனில் (!vtxp->duplicatep() && !actSet.empty())
                    nNonConstCodes += vtxp->nodep()->codeInc();
                எனில் (actSet.empty()) {
                    // If a trace doesn't have activity, it's constant, and we
                    // don't need to track changes on it.
                    actSet.செருகு(TraceActivityVertex::ACTIVITY_NEVER);
                } இல்லையெனில் எனில் (actSet.மொத்தம்(TraceActivityVertex::ACTIVITY_SLOW) && actSet.size() > 1) {
                    // If a trace depends on the slow flag as well as other
                    // flags, remove the dependency on the slow flag. We will
                    // make slow routines set all activity flags.
                    actSet.erase(TraceActivityVertex::ACTIVITY_SLOW);
                }
                traces.emplace(actSet, vtxp);
            }
        }
    }

    வெறுமை graphOptimize() {
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
        மாறிலி auto end = traces.end();
        போது (it != end) {
            auto head = it;
            // Approximate the complexity of the value change check
            uint32_t complexity = 0;
            மாறிலி ActCodeSet& actSet = it->first;
            க்கு (; it != end && it->first == actSet; ++it) {
                எனில் (!it->second->duplicatep()) {
                    uint32_t cost = 0;
                    மாறிலி AstTraceDecl* மாறிலி declp = it->second->nodep();
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
            எனில் (actSet.மொத்தம்(TraceActivityVertex::ACTIVITY_ALWAYS)) தொடர்;
            எனில் (actSet.மொத்தம்(TraceActivityVertex::ACTIVITY_NEVER)) தொடர்;
            எனில் (actSet.மொத்தம்(TraceActivityVertex::ACTIVITY_SLOW)) தொடர்;
            // If the value comparisons are cheaper to perform than checking the
            // activity flags make the signals always traced. Note this cost
            // equation is heuristic.
            எனில் (complexity <= actSet.size() * 2) {
                க்கு (; head != it; ++head) {
                    புதிய V3GraphEdge{&m_graph, m_alwaysVtxp, head->second, 1};
                }
            }
        }

        graphSimplify(பொய்);
    }

    Ast_முனை_கோவை* selectActivity(File_வரி* flp, uint32_t acode, மாறிலி VAccess& access) {
        திரும்பு புதிய AstArraySel(flp, புதிய AstVarRef{flp, m_activityVscp, access}, acode);
    }

    Ast_முனை* newActivitySetter(Ast_முனை* insertp, uint32_t code) {
        ++m_statSetters;
        File_வரி* மாறிலி fl = insertp->fileline();
        AstAssign* மாறிலி setterp = புதிய AstAssign{fl, selectActivity(fl, code, VAccess::WRITE),
                                                 புதிய AstConst{fl, AstConst::BitTrue{}}};
        திரும்பு setterp;
    }

    Ast_முனை* newActivityAll(Ast_முனை* insertp) {
        ++m_statSettersSlow;
        எனில் (!m_actAllFuncp) {
            File_வரி* மாறிலி flp = m_உச்சி_Scopep->fileline();
            AstCFunc* மாறிலி funcp = புதிய AstCFunc{flp, "__VtraceActivitySetAll", m_உச்சி_Scopep};
            funcp->slow(உண்மை);
            funcp->isStatic(பொய்);
            funcp->isLoose(உண்மை);
            m_உச்சி_Scopep->addBlocksp(funcp);
            க்கு (uint32_t code = 0; code < m_activity_எண்; ++code) {
                Ast_முனை* மாறிலி setterp = newActivitySetter(insertp, code);
                funcp->addStmtsp(setterp);
            }
            m_actAllFuncp = funcp;
        }
        AstCCall* மாறிலி callp = புதிய AstCCall{insertp->fileline(), m_actAllFuncp};
        callp->dtypeSetVoid();
        திரும்பு callp->makeStmt();
    }

    வெறுமை createActivityFlags() {
        // Assign final activity numbers
        m_activity_எண் = assignactivityNumbers();

        // Create an array of bytes, not a bit vector, as they can be set
        // atomically by mtasks, and are cheaper to set (no need for
        // read-modify-write on the C type), and the speed of the tracing code
        // is the same on largish designs.
        File_வரி* மாறிலி flp = m_உச்சி_Scopep->fileline();
        Ast_முனை_DType* மாறிலி newScalarDtp = புதிய AstBasicDType{flp, VFlagBitPacked{}, 1};
        v3Global.rootp()->typeTablep()->addTypesp(newScalarDtp);
        AstRange* மாறிலி newArange
            = புதிய AstRange{flp, VNumRange{static_cast<முழு>(m_activity_எண்) - 1, 0}};
        Ast_முனை_DType* மாறிலி newArrDtp = புதிய AstUnpackArrayDType{flp, newScalarDtp, newArange};
        v3Global.rootp()->typeTablep()->addTypesp(newArrDtp);
        m_activityVscp = v3Global.rootp()->உச்சி_Scopep()->createTemp("__VtraceActivity", newArrDtp);

        // Insert activity setters
        க்கு (மாறிலி V3GraphVertex& vtx : m_graph.vertices()) {
            எனில் (மாறிலி TraceActivityVertex* மாறிலி vtxp = vtx.cast<மாறிலி TraceActivityVertex>()) {
                Ast_முனை* setterp = nullptr;
                எனில் (vtxp->activitySlow()) {
                    setterp = newActivityAll(vtxp->insertp());
                    // Just set all flags in slow code as it should be rare.
                    // This will be rolled up into a loop by V3Reloop.
                } இல்லையெனில் எனில் (!vtxp->activityAlways()) {
                    setterp = newActivitySetter(vtxp->insertp(), vtxp->activityCode());
                }
                எனில் (setterp) {
                    Ast_முனை* மாறிலி insertp = vtxp->insertp();
                    எனில் (Ast_Stmt_கோவை* மாறிலி stmtp = VN_CAST(insertp, Stmt_கோவை)) {
                        stmtp->addNextHere(setterp);
                    } இல்லையெனில் எனில் (AstCFunc* மாறிலி funcp = VN_CAST(insertp, CFunc)) {
                        // If there are awaits, insert the setter after each await
                        எனில் (funcp->isCoroutine() && funcp->stmtsp()) {
                            funcp->stmtsp()->foreachAndNext([setterp](AstCAwait* awaitp) {
                                awaitp->addNextHere(setterp->clone_மரம்(பொய்));
                            });
                        }
                        funcp->addStmtsp(setterp);
                    } இல்லையெனில் {
                        insertp->v3fatalSrc("Bad trace activity vertex");
                    }
                }
            }
        }
    }

    AstCFunc* newCFunc(VTraceType traceType, AstCFunc* உச்சி_Funcp, uint32_t func_இலக்கம்,
                       uint32_t baseCode = 0, மாறிலி AstTraceDecl* மாறிலி declp = nullptr,
                       தருக்க declSub = பொய்) {
        // Create new function
        மாறிலி தருக்க is_உச்சி_Func = !declp && உச்சி_Funcp == nullptr;
        std::string funcName;
        எனில் (is_உச்சி_Func || declp) {
            funcName = traceType.func_prefix();
        } இல்லையெனில் {
            funcName = உச்சி_Funcp->name();
            funcName += "_sub";
        }
        எனில் (declp) {
            funcName += "_dtype";
            எனில் (declSub) funcName += "_sub";
            funcName += "__";
            funcName = m_dtypeNames.get(funcName);
        } இல்லையெனில் {
            funcName += "_";
            funcName += cvtToStr(func_இலக்கம்);
        }

        File_வரி* மாறிலி flp = m_உச்சி_Scopep->fileline();
        AstCFunc* மாறிலி funcp = புதிய AstCFunc{flp, funcName, m_உச்சி_Scopep};
        funcp->isTrace(உண்மை);
        funcp->dontCombine(உண்மை);
        funcp->isLoose(உண்மை);
        funcp->slow(traceType != VTraceType::CHANGE);
        funcp->isStatic(is_உச்சி_Func);
        // Add it to top scope
        m_உச்சி_Scopep->addBlocksp(funcp);
        மாறிலி std::string bufArg
            = v3Global.opt.traceClassBase() + "::Buffer* bufp"
              + (declp ? (", uint32_t offset, const " + declp->dtypep()->cType("", உண்மை, உண்மை)
                          + " __VdtypeVar")
                       : "");
        எனில் (is_உச்சி_Func) {
            // Top functions
            funcp->argTypes("void* voidSelf, " + bufArg);
            funcp->addStmtsp(புதிய AstCStmt{flp, EmitCUtil::voidSelfAssign(m_உச்சி_Modp)});
            funcp->addStmtsp(புதிய AstCStmt{flp, EmitCUtil::symClassAssign()});
            // Add global activity check to change dump functions
            எனில் (traceType == VTraceType::CHANGE) {  //
                funcp->addStmtsp(
                    புதிய AstCStmt{flp, "if (VL_UNLIKELY(!vlSymsp->__Vm_activity)) return;"});
            }
            // Register function
            AstCStmt* மாறிலி cstmtp = புதிய AstCStmt{flp};
            m_regFuncp->addStmtsp(cstmtp);
            எனில் (traceType == VTraceType::CONSTANT) {
                cstmtp->add("tracep->addConstCb(");
                cstmtp->add(புதிய AstAddrOfCFunc{flp, funcp});
                cstmtp->add(", " + std::to_string(func_இலக்கம்) + ", vlSelf);");
            } இல்லையெனில் எனில் (traceType == VTraceType::FULL) {
                cstmtp->add("tracep->addFullCb(");
                cstmtp->add(புதிய AstAddrOfCFunc{flp, funcp});
                cstmtp->add(", " + std::to_string(func_இலக்கம்) + ", vlSelf);");
            } இல்லையெனில் {
                cstmtp->add("tracep->addChgCb(");
                cstmtp->add(புதிய AstAddrOfCFunc{flp, funcp});
                cstmtp->add(", " + std::to_string(func_இலக்கம்) + ", vlSelf);");
            }
        } இல்லையெனில் {
            // Sub functions
            funcp->argTypes(bufArg);
            // Setup base references. Note in rare occasions we can end up with an empty trace
            // sub function, hence the VL_ATTR_UNUSED attributes.
            எனில் (traceType != VTraceType::CHANGE) {
                // Full dump sub function
                funcp->addStmtsp(புதிய AstCStmt{flp,  //
                                              string("uint32_t* const oldp VL_ATTR_UNUSED = "
                                                     "bufp->oldp(vlSymsp->__Vm_baseCode")
                                                  + (declp ? " + offset" : "") + ");\n"});
            } இல்லையெனில் {
                // Change dump sub function
                funcp->addStmtsp(புதிய AstCStmt{flp,  //
                                              "uint32_t* const oldp VL_ATTR_UNUSED = "
                                              "bufp->oldp(vlSymsp->__Vm_baseCode + "
                                                  + (declp ? " offset" : cvtToStr(baseCode))
                                                  + ");\n"});
            }
            எனில் (!declp) {
                // Add call to top function
                AstCCall* மாறிலி callp = புதிய AstCCall{funcp->fileline(), funcp};
                callp->dtypeSetVoid();
                callp->argTypes("bufp");
                உச்சி_Funcp->addStmtsp(callp->makeStmt());
            }
        }
        // Done
        UINFO(5, "  newCFunc " << funcp);
        திரும்பு funcp;
    }

    AstCFunc* newCDtypeSubFunc(VTraceType traceType, மாறிலி AstTraceDecl* மாறிலி declp,
                               AstCFunc* parentp) {
        AstCFunc* மாறிலி funcp = newCFunc(traceType, nullptr, 0, 0, declp, உண்மை);
        File_வரி* மாறிலி flp = m_உச்சி_Scopep->fileline();
        AstCCall* மாறிலி callp = புதிய AstCCall{flp, funcp};
        callp->dtypeSetVoid();
        callp->argTypes("bufp, offset, __VdtypeVar");
        parentp->addStmtsp(callp->makeStmt());
        திரும்பு funcp;
    }

    AstCFunc* createConstDtypeTraceFunctions(மாறிலி AstTraceDecl* declp) {
        மாறிலி Ast_முனை_DType* மாறிலி dtypep = declp->dtypep()->skipRefp();
        auto pair = m_dtypeConstFuncs.emplace(dtypep, nullptr);
        எனில் (pair.second) {
            File_வரி* மாறிலி flp = declp->fileline();
            AstCFunc* மாறிலி funcp = newCFunc(VTraceType::CONSTANT, nullptr, 0, 0, declp);

            தருக்க first = உண்மை;
            க்கு (Ast_முனை* callStmtp = declp->dtypeCallp()->funcp()->stmtsp(); callStmtp;
                 callStmtp = callStmtp->nextp()) {
                மாறிலி AstCCall* மாறிலி callp
                    = VN_CAST(VN_CAST(callStmtp, Stmt_கோவை)->exprp(), CCall);
                தருக்க onlyOne = first && !callStmtp->nextp();
                AstCFunc* மாறிலி subFuncp
                    = onlyOne ? funcp : newCDtypeSubFunc(VTraceType::CONSTANT, declp, funcp);
                க்கு (Ast_முனை* stmtp = callp->funcp()->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    எனில் (AstTraceDecl* மாறிலி fieldDeclp = VN_CAST(stmtp, TraceDecl)) {
                        AstTraceInc* மாறிலி incp
                            = புதிய AstTraceInc{flp, fieldDeclp, VTraceType::CONSTANT};
                        subFuncp->addStmtsp(incp);
                    }
                }
                first = பொய்;
            }

            pair.first->second = funcp;
        }

        திரும்பு pair.first->second;
    }

    வெறுமை createConstTraceFunctions(மாறிலி TraceVec& traces) {
        மாறிலி முழு splitLimit = v3Global.opt.outputSplitCTrace() ? v3Global.opt.outputSplitCTrace()
                                                                : std::numeric_limits<முழு>::max();

        AstCFunc* மாறிலி உச்சி_Funcp = newCFunc(VTraceType::CONSTANT, nullptr, 0);
        uint32_t sub_Func_இலக்கம் = 0;
        AstCFunc* subFuncp = nullptr;
        முழு subStmts = 0;
        std::vector<மாறிலி TraceTraceVertex*> duplicates;
        க்கு (auto it = traces.cbegin(); it != traces.end(); ++it) {
            மாறிலி TraceTraceVertex* மாறிலி vtxp = it->second;
            AstTraceDecl* மாறிலி declp = vtxp->nodep();
            எனில் (vtxp->duplicatep()) {
                duplicates.தள்ளு_back(vtxp);
                தொடர்;
            }

            // This is a canonical trace node. Assign trace code (signal number).
            UASSERT_OBJ(!declp->codeAssigned(), declp,
                        "Canonical node should not have code assigned yet");
            declp->code(m_code);
            மாறிலி uint32_t codeInc = declp->codeInc();
            m_code += codeInc;
            ++m_statUniqSigs;

            // If this is a const signal, add the AstTraceInc
            மாறிலி ActCodeSet& actSet = it->first;
            எனில் (actSet.மொத்தம்(TraceActivityVertex::ACTIVITY_NEVER)) {
                // Crate new sub function if required
                எனில் (!subFuncp || subStmts > splitLimit) {
                    subStmts = 0;
                    subFuncp = newCFunc(VTraceType::CONSTANT, உச்சி_Funcp, sub_Func_இலக்கம்);
                    ++sub_Func_இலக்கம்;
                }
                File_வரி* மாறிலி flp = declp->fileline();
                எனில் (declp->dtypeCallp()) {
                    AstCFunc* மாறிலி funcp = createConstDtypeTraceFunctions(declp);
                    Ast_முனை_கோவை* argsp = nullptr;
                    argsp = Ast_முனை::addNext(argsp, declp->valuep()->clone_மரம்(பொய்));
                    AstCCall* மாறிலி callp = புதிய AstCCall{flp, funcp, argsp};
                    callp->dtypeSetVoid();
                    callp->argTypes(callp->argTypes() + "bufp, " + std::to_string(declp->code()));
                    subFuncp->addStmtsp(callp->makeStmt());

                    subStmts += 1;
                } இல்லையெனில் {
                    AstTraceInc* மாறிலி incp = புதிய AstTraceInc{flp, declp, VTraceType::CONSTANT};
                    subFuncp->addStmtsp(incp);
                    subStmts += incp->முனை_மொத்தம்();
                }
            }
        }

        // Assign codes to duplicate nodes from their canonicals
        க்கு (மாறிலி TraceTraceVertex* மாறிலி vtxp : duplicates) {
            AstTraceDecl* மாறிலி declp = vtxp->nodep();
            மாறிலி TraceTraceVertex* மாறிலி canonVtxp = vtxp->duplicatep();
            மாறிலி AstTraceDecl* மாறிலி canonDeclp = canonVtxp->nodep();
            UASSERT_OBJ(!canonVtxp->duplicatep(), canonDeclp, "Canonical node is a duplicate");
            UASSERT_OBJ(canonDeclp->codeAssigned(), canonDeclp,
                        "Canonical node should have code assigned already");
            declp->code(canonDeclp->code() + vtxp->dtypeAliasOffset());
        }
    }

    DtypeFuncs createNonConstDtypeTraceFunctions(மாறிலி AstTraceDecl* declp) {
        Ast_முனை_DType* dtypep = declp->dtypep()->skipRefp();
        auto pair = m_dtypeNonConstFuncs.emplace(dtypep, DtypeFuncs{});
        எனில் (pair.second) {
            File_வரி* மாறிலி flp = declp->fileline();
            AstCFunc* மாறிலி fullFuncp = newCFunc(VTraceType::FULL, nullptr, 0, 0, declp);
            AstCFunc* மாறிலி chgFuncp = newCFunc(VTraceType::CHANGE, nullptr, 0, 0, declp);

            தருக்க first = உண்மை;
            க்கு (Ast_முனை* callStmtp = declp->dtypeCallp()->funcp()->stmtsp(); callStmtp;
                 callStmtp = callStmtp->nextp()) {
                மாறிலி AstCCall* மாறிலி callp
                    = VN_CAST(VN_CAST(callStmtp, Stmt_கோவை)->exprp(), CCall);
                தருக்க onlyOne = first && !callStmtp->nextp();
                AstCFunc* மாறிலி fullSubFuncp
                    = onlyOne ? fullFuncp : newCDtypeSubFunc(VTraceType::FULL, declp, fullFuncp);
                AstCFunc* மாறிலி chgSubFuncp
                    = onlyOne ? chgFuncp : newCDtypeSubFunc(VTraceType::CHANGE, declp, chgFuncp);
                க்கு (Ast_முனை* stmtp = callp->funcp()->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    எனில் (AstTraceDecl* மாறிலி fieldDeclp = VN_CAST(stmtp, TraceDecl)) {
                        AstTraceInc* மாறிலி incFullp
                            = புதிய AstTraceInc{flp, fieldDeclp, VTraceType::FULL};
                        fullSubFuncp->addStmtsp(incFullp);
                        AstTraceInc* மாறிலி incChgp
                            = புதிய AstTraceInc{flp, fieldDeclp, VTraceType::CHANGE};
                        chgSubFuncp->addStmtsp(incChgp);
                    }
                }
                first = பொய்;
            }

            pair.first->second = {.fullFuncp = fullFuncp, .chgFuncp = chgFuncp};
        }

        திரும்பு pair.first->second;
    }

    வெறுமை createNonConstTraceFunctions(மாறிலி TraceVec& traces, uint32_t nAllCodes,
                                      uint32_t parallelism) {
        மாறிலி முழு splitLimit = v3Global.opt.outputSplitCTrace() ? v3Global.opt.outputSplitCTrace()
                                                                : std::numeric_limits<முழு>::max();

        // pre-incremented, so starts at 0
        uint32_t உச்சி_Func_இலக்கம் = std::numeric_limits<uint32_t>::max();
        TraceVec::const_iterator it = traces.begin();
        போது (it != traces.end()) {
            AstCFunc* உச்சி_Ful_Funcp = nullptr;
            AstCFunc* உச்சி_Chg_Funcp = nullptr;
            AstCFunc* subFulFuncp = nullptr;
            AstCFunc* subChgFuncp = nullptr;
            uint32_t sub_Func_இலக்கம் = 0;
            முழு subStmts = 0;
            மாறிலி uint32_t maxCodes = std::max((nAllCodes + parallelism - 1) / parallelism, 1U);
            uint32_t nCodes = 0;
            மாறிலி ActCodeSet* prevActSet = nullptr;
            AstIf* ifp = nullptr;
            uint32_t baseCode = 0;
            க்கு (; nCodes < maxCodes && it != traces.end(); ++it) {
                மாறிலி ActCodeSet& actSet = it->first;
                // Traced value never changes, no need to add it
                எனில் (actSet.மொத்தம்(TraceActivityVertex::ACTIVITY_NEVER)) தொடர்;

                மாறிலி TraceTraceVertex* மாறிலி vtxp = it->second;
                AstTraceDecl* மாறிலி declp = vtxp->nodep();

                // This is a duplicate decl, no need to add it, but must set the
                // function index to the same as the canonical node.
                எனில் (மாறிலி TraceTraceVertex* மாறிலி canonVtxp = vtxp->duplicatep()) {
                    declp->fidx(canonVtxp->nodep()->fidx());
                    தொடர்;
                }

                // Create top function if not yet created
                எனில் (!உச்சி_Ful_Funcp) {
                    ++உச்சி_Func_இலக்கம்;
                    உச்சி_Ful_Funcp = newCFunc(VTraceType::FULL, nullptr, உச்சி_Func_இலக்கம்);
                    உச்சி_Chg_Funcp = newCFunc(VTraceType::CHANGE, nullptr, உச்சி_Func_இலக்கம்);
                }

                // Create new sub function if required
                எனில் (!subFulFuncp || subStmts > splitLimit) {
                    baseCode = declp->code();
                    subStmts = 0;
                    subFulFuncp = newCFunc(VTraceType::FULL, உச்சி_Ful_Funcp, sub_Func_இலக்கம், baseCode);
                    subChgFuncp = newCFunc(VTraceType::CHANGE, உச்சி_Chg_Funcp, sub_Func_இலக்கம், baseCode);
                    ++sub_Func_இலக்கம்;
                    prevActSet = nullptr;
                    ifp = nullptr;
                }

                // If required, create the conditional node checking the activity flags
                எனில் (!prevActSet || actSet != *prevActSet) {
                    File_வரி* மாறிலி flp = m_உச்சி_Scopep->fileline();
                    மாறிலி தருக்க always = actSet.மொத்தம்(TraceActivityVertex::ACTIVITY_ALWAYS) != 0;
                    Ast_முனை_கோவை* condp = nullptr;
                    எனில் (always) {
                        condp = புதிய AstConst{flp, 1};  // Always true, will be folded later
                    } இல்லையெனில் {
                        க்கு (மாறிலி uint32_t actCode : actSet) {
                            Ast_முனை_கோவை* மாறிலி selp = selectActivity(flp, actCode, VAccess::READ);
                            condp = condp ? புதிய AstOr{flp, condp, selp} : selp;
                        }
                    }
                    ifp = புதிய AstIf{flp, condp};
                    எனில் (!always) ifp->branchPred(VBranchPred::BP_UNLIKELY);
                    subChgFuncp->addStmtsp(ifp);
                    subStmts += ifp->முனை_மொத்தம்();
                    prevActSet = &actSet;
                }

                // Add TraceInc nodes
                File_வரி* மாறிலி flp = declp->fileline();
                எனில் (declp->dtypeCallp()) {
                    DtypeFuncs funcs = createNonConstDtypeTraceFunctions(declp);
                    Ast_முனை_கோவை* argsp = nullptr;
                    argsp = Ast_முனை::addNext(argsp, declp->valuep()->clone_மரம்(பொய்));
                    AstCCall* மாறிலி callFullp = புதிய AstCCall{flp, funcs.fullFuncp, argsp};
                    callFullp->dtypeSetVoid();
                    callFullp->argTypes(callFullp->argTypes() + "bufp, "
                                        + std::to_string(declp->code()));
                    subFulFuncp->addStmtsp(callFullp->makeStmt());
                    argsp = nullptr;
                    argsp = Ast_முனை::addNext(argsp, declp->valuep()->clone_மரம்(பொய்));
                    AstCCall* மாறிலி callChgp = புதிய AstCCall{flp, funcs.chgFuncp, argsp};
                    callChgp->dtypeSetVoid();
                    callChgp->argTypes(callChgp->argTypes() + "bufp, "
                                       + std::to_string(declp->code()));
                    ifp->addThensp(callChgp->makeStmt());

                    subStmts += 2;
                } இல்லையெனில் {
                    AstTraceInc* மாறிலி incFulp = புதிய AstTraceInc{flp, declp, VTraceType::FULL};
                    subFulFuncp->addStmtsp(incFulp);
                    AstTraceInc* மாறிலி incChgp
                        = புதிய AstTraceInc{flp, declp, VTraceType::CHANGE, baseCode};
                    ifp->addThensp(incChgp);

                    // Set the function index of the decl
                    declp->fidx(உச்சி_Func_இலக்கம்);

                    // Track splitting due to size
                    UASSERT_OBJ(incFulp->முனை_மொத்தம்() == incChgp->முனை_மொத்தம்(), declp,
                                "Should have equal cost");
                    மாறிலி VNumRange range = declp->arrayRange();
                    எனில் (range.ranged()) {
                        // 2x because each element is a TraceInc and a VarRef
                        subStmts += range.elements() * 2;
                    } இல்லையெனில் {
                        subStmts += incChgp->முனை_மொத்தம்();
                    }
                }

                // Track partitioning
                nCodes += declp->codeInc();
            }
        }
    }

    வெறுமை createCleanupFunction() {
        File_வரி* மாறிலி fl = m_உச்சி_Scopep->fileline();
        AstCFunc* மாறிலி cleanupFuncp = புதிய AstCFunc{fl, "trace_cleanup", m_உச்சி_Scopep};
        cleanupFuncp->argTypes("void* voidSelf, " + v3Global.opt.traceClassBase()
                               + "* /*unused*/");
        cleanupFuncp->isTrace(உண்மை);
        cleanupFuncp->slow(பொய்);
        cleanupFuncp->isStatic(உண்மை);
        cleanupFuncp->isLoose(உண்மை);
        m_உச்சி_Scopep->addBlocksp(cleanupFuncp);
        cleanupFuncp->addStmtsp(புதிய AstCStmt{fl, EmitCUtil::voidSelfAssign(m_உச்சி_Modp)});
        cleanupFuncp->addStmtsp(புதிய AstCStmt{fl, EmitCUtil::symClassAssign()});

        // Register it
        {
            AstCStmt* மாறிலி cstmtp = புதிய AstCStmt{fl};
            m_regFuncp->addStmtsp(cstmtp);
            cstmtp->add("tracep->addCleanupCb(");
            cstmtp->add(புதிய AstAddrOfCFunc{fl, cleanupFuncp});
            cstmtp->add(", vlSelf);");
        }

        // Clear global activity flag
        cleanupFuncp->addStmtsp(
            புதிய AstCStmt{m_உச்சி_Scopep->fileline(), "vlSymsp->__Vm_activity = false;"s});

        // Clear fine grained activity flags
        க்கு (uint32_t i = 0; i < m_activity_எண்; ++i) {
            Ast_முனை* மாறிலி clrp = புதிய AstAssign{fl, selectActivity(fl, i, VAccess::WRITE),
                                                புதிய AstConst{fl, AstConst::BitFalse{}}};
            cleanupFuncp->addStmtsp(clrp);
        }
    }

    வெறுமை createTraceFunctions() {
        // Detect and remove duplicate values
        detectDuplicates();

        graphDtypePrune();
        splitTraceDeclFuncs();
        m_graph.removeRedundantEdgesMax(&V3GraphEdge::followAlwaysTrue);

        // Simplify & optimize the graph
        எனில் (dumpGraphLevel() >= 6) m_graph.dumpDotFilePrefixed("trace_pre");
        graphSimplify(உண்மை);
        எனில் (dumpGraphLevel() >= 6) m_graph.dumpDotFilePrefixed("trace_simplified");
        graphOptimize();
        எனில் (dumpGraphLevel() >= 6) m_graph.dumpDotFilePrefixed("trace_optimized");

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
        m_regFuncp = புதிய AstCFunc{m_உச்சி_Scopep->fileline(), "trace_register", m_உச்சி_Scopep};
        m_regFuncp->argTypes(v3Global.opt.traceClassBase() + "* tracep");
        m_regFuncp->entryPoint(உண்மை);
        m_regFuncp->isTrace(உண்மை);
        m_regFuncp->slow(உண்மை);
        m_regFuncp->isStatic(பொய்);
        m_regFuncp->isLoose(உண்மை);
        m_உச்சி_Scopep->addBlocksp(m_regFuncp);

        // Create the const dump functions. Also allocates trace codes.
        createConstTraceFunctions(traces);

        // Create the full and incremental dump functions
        createNonConstTraceFunctions(traces, nNonConstCodes, m_parallelism);

        // Root-traced libraries alias wrapper IOs onto the existing top-module codes.
        எனில் (!v3Global.opt.libCreate().empty()) {
            std::vector<AstTraceDecl*> வேர்_Declps;
            std::vector<AstTraceDecl*> உச்சி_Declps;
            TraceInitDeclCollector வேர்_Collector{வேர்_Declps};
            TraceInitDeclCollector உச்சி_Collector{உச்சி_Declps};
            க்கு (Ast_முனை* blockp = m_உச்சி_Scopep->blocksp(); blockp; blockp = blockp->nextp()) {
                AstCFunc* மாறிலி funcp = VN_CAST(blockp, CFunc);
                எனில் (funcp && VString::startsWith(funcp->name(), "trace_init_leaf_root__")) {
                    வேர்_Collector.collect(funcp);
                } இல்லையெனில் எனில் (funcp && VString::startsWith(funcp->name(), "trace_init_leaf_top__")) {
                    உச்சி_Collector.collect(funcp);
                }
            }

            std::vector<தருக்க> used(உச்சி_Declps.size(), பொய்);
            க்கு (AstTraceDecl* மாறிலி வேர்_Declp : வேர்_Declps) {
                க்கு (size_t i = 0; i < உச்சி_Declps.size(); ++i) {
                    எனில் (used[i] || !same_வேர்_Init_Alias(வேர்_Declp, உச்சி_Declps[i])) தொடர்;
                    வேர்_Declp->code(உச்சி_Declps[i]->code());
                    used[i] = உண்மை;
                    நிறுத்து;
                }
            }
        }

        // Remove refs to traced values from TraceDecl nodes, these have now moved under
        // TraceInc
        க்கு (மாறிலி auto& i : traces) {
            Ast_முனை* மாறிலி valuep = i.second->nodep()->valuep();
            VL_DO_DANGLING(valuep->unlinkFrBack()->delete_மரம்(), valuep);
        }

        // Create the trace cleanup function clearing the activity flags
        createCleanupFunction();
    }

    TraceCFuncVertex* getCFuncVertexp(AstCFunc* nodep) {
        V3GraphVertex* மாறிலி vtxp = nodep->user1u().toGraphVertex();
        TraceCFuncVertex* vertexp = vtxp ? vtxp->cast<TraceCFuncVertex>() : nullptr;
        எனில் (!vertexp) {
            vertexp = புதிய TraceCFuncVertex{&m_graph, nodep};
            nodep->user1p(vertexp);
        }
        திரும்பு vertexp;
    }
    TraceActivityVertex* getActivityVertexp(Ast_முனை* nodep, தருக்க slow) {
        V3GraphVertex* மாறிலி vtxp = nodep->user3u().toGraphVertex();
        TraceActivityVertex* vertexp = vtxp ? vtxp->cast<TraceActivityVertex>() : nullptr;
        எனில் (!vertexp) {
            vertexp = புதிய TraceActivityVertex{&m_graph, nodep, slow};
            nodep->user3p(vertexp);
        }
        vertexp->slow(slow);
        திரும்பு vertexp;
    }

    // VISITORS
    வெறுமை visit(AstNetlist* nodep) override {
        // Add vertexes for all TraceDecl, and edges from VARs each trace looks at
        m_finding = பொய்;
        iterateChildren(nodep);

        // Add vertexes for all CFUNCs, and edges to VARs the func sets
        m_finding = உண்மை;
        iterateChildren(nodep);

        // Create the trace functions and insert them into the tree
        createTraceFunctions();

        // Save number of trace codes used
        nodep->nTraceCodes(m_code);
    }
    வெறுமை visit(Ast_முனை_Module* nodep) override {
        எனில் (nodep->is_உச்சி()) m_உச்சி_Modp = nodep;
        iterateChildren(nodep);
    }
    வெறுமை visit(AstVarScope* nodep) override {
        எனில் (!m_finding) {
            எனில் (மாறிலி AstIface* மாறிலி ifacep = nodep->varp()->sensIfacep()) {
                m_ifaceMemberVscps[{ifacep, nodep->varp()->name()}].தள்ளு_back(nodep);
            }
        }
    }
    வெறுமை visit(Ast_Stmt_கோவை* nodep) override {
        எனில் (!m_finding && !nodep->user2()) {
            எனில் (AstCCall* மாறிலி callp = VN_CAST(nodep->exprp(), CCall)) {
                UINFO(8, "   CCALL " << callp);
                // See if there are other calls in same statement list;
                // If so, all funcs might share the same activity code
                TraceActivityVertex* மாறிலி activityVtxp
                    = getActivityVertexp(nodep, callp->funcp()->slow());
                க்கு (Ast_முனை* nextp = nodep; nextp; nextp = nextp->nextp()) {
                    எனில் (Ast_Stmt_கோவை* மாறிலி stmtp = VN_CAST(nextp, Stmt_கோவை)) {
                        எனில் (AstCCall* மாறிலி ccallp = VN_CAST(stmtp->exprp(), CCall)) {
                            stmtp->user2(உண்மை);  // Processed
                            UINFO(8, "     SubCCALL " << ccallp);
                            V3GraphVertex* மாறிலி ccallFuncVtxp = getCFuncVertexp(ccallp->funcp());
                            activityVtxp->slow(ccallp->funcp()->slow());
                            புதிய V3GraphEdge{&m_graph, activityVtxp, ccallFuncVtxp, 1};
                        }
                    }
                }
            }
        }
        iterateChildren(nodep);
    }
    வெறுமை visit(AstCFunc* nodep) override {
        UINFO(8, "   CFUNC " << nodep);
        V3GraphVertex* மாறிலி funcVtxp = getCFuncVertexp(nodep);
        எனில் (!m_finding) {  // If public, we need a unique activity code to allow for sets
                           // directly in this func
            எனில் (nodep->funcPublic() || nodep->dpiExportImpl() || nodep->entryPoint()
                || nodep->isCoroutine()) {
                // Cannot treat a coroutine as slow, it may be resumed later
                மாறிலி தருக்க slow = nodep->slow() && !nodep->isCoroutine();
                TraceActivityVertex* மாறிலி activityVtxp = getActivityVertexp(nodep, slow);
                புதிய V3GraphEdge{&m_graph, activityVtxp, funcVtxp, 1};
            }
        }
        VL_RESTORER(m_cfuncp);
        m_cfuncp = nodep;
        iterateChildren(nodep);
    }
    வெறுமை visit(AstTraceDecl* nodep) override {
        UINFO(8, "   TRACE " << nodep);
        எனில் (!m_finding && !nodep->inDtypeFunc()) {
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
            எனில் (m_cfuncp && VString::startsWith(m_cfuncp->name(), "trace_init_leaf_root__")) {
                திரும்பு;
            }
            V3GraphVertex* மாறிலி vertexp = புதிய TraceTraceVertex{&m_graph, nodep};
            nodep->user1p(vertexp);

            UASSERT_OBJ(m_cfuncp, nodep, "Trace not under func");
            m_cfuncp->user2(உண்மை);
            VL_RESTORER(m_tracep);
            m_tracep = nodep;
            iterateChildren(nodep);
        }
    }
    வெறுமை visit(AstVarRef* nodep) override {
        UASSERT_OBJ(nodep->varScopep(), nodep, "No var scope?");
        AstVarScope* மாறிலி varscopep = nodep->varScopep();
        V3GraphVertex* varVtxp = varscopep->user1u().toGraphVertex();
        எனில் (m_tracep) {
            UASSERT_OBJ(nodep->access().isReadOnly(), nodep, "Lvalue in trace?  Should be const.");
            எனில் (!varVtxp) {
                varVtxp = புதிய TraceVarVertex{&m_graph, nodep->varScopep()};
                nodep->varScopep()->user1p(varVtxp);
            }
            V3GraphVertex* மாறிலி traceVtxp = m_tracep->user1u().toGraphVertex();
            புதிய V3GraphEdge{&m_graph, varVtxp, traceVtxp, 1};
            எனில் (nodep->varp()->isPrimaryInish()  // Always need to trace primary inputs
                || nodep->varp()->isSigPublic()) {  // Or ones user can change
                புதிய V3GraphEdge{&m_graph, m_alwaysVtxp, traceVtxp, 1};
            }
            எனில் (m_tracep->dtypeCallp()) varscopep->user2p(m_tracep);
        } இல்லையெனில் எனில் (m_cfuncp && m_finding && nodep->access().isWriteOrRW()) {
            V3GraphVertex* மாறிலி funcVtxp = getCFuncVertexp(m_cfuncp);
            எனில் (varVtxp) {  // else we're not tracing this signal
                புதிய V3GraphEdge{&m_graph, funcVtxp, varVtxp, 1};
                AstTraceDecl* மாறிலி declp = VN_AS(varscopep->user2p(), TraceDecl);
                எனில் (declp) {
                    V3GraphVertex* மாறிலி cFuncVtxp = getCFuncVertexp(m_cfuncp);
                    க்கு (மாறிலி V3GraphEdge& edge : cFuncVtxp->inEdges()) {
                        V3GraphVertex* மாறிலி activityp = edge.fromp();
                        எனில் (!declp->user3p()) {
                            declp->user3p(activityp);
                        } இல்லையெனில் எனில் (declp->user3u().toGraphVertex() != activityp) {
                            declp->user2(உண்மை);
                        }
                    }
                }
            }
        }
    }
    வெறுமை visit(AstMemberSel* nodep) override {
        எனில் (m_cfuncp && m_finding && nodep->access().isWriteOrRW()) {
            AstIfaceRefDType* மாறிலி dtypep
                = VN_CAST(nodep->fromp()->dtypep()->skipRefp(), IfaceRefDType);
            எனில் (dtypep && dtypep->isVirtual()) {
                மாறிலி auto it = m_ifaceMemberVscps.find({dtypep->ifacep(), nodep->varp()->name()});
                எனில் (it != m_ifaceMemberVscps.end()) {
                    V3GraphVertex* மாறிலி funcVtxp = getCFuncVertexp(m_cfuncp);
                    க்கு (AstVarScope* மாறிலி vscp : it->second) {
                        V3GraphVertex* varVtxp = vscp->user1u().toGraphVertex();
                        எனில் (!varVtxp) {
                            varVtxp = புதிய TraceVarVertex{&m_graph, vscp};
                            vscp->user1p(varVtxp);
                        }
                        புதிய V3GraphEdge{&m_graph, funcVtxp, varVtxp, 1};
                    }
                }
            }
        }
        iterateChildren(nodep);
    }
    //--------------------
    வெறுமை visit(Ast_முனை* nodep) override { iterateChildren(nodep); }

பொது:
    // CONSTRUCTORS
    explicit TraceVisitor(AstNetlist* nodep)
        : m_alwaysVtxp{புதிய TraceActivityVertex{&m_graph, TraceActivityVertex::ACTIVITY_ALWAYS}} {
        nodep->user2_Clear_மரம்();  // TraceDecl multiple activities flag
        nodep->user3_Clear_மரம்();  // TraceDecl TraceActivityVertex (assumes we start at nullptr)
        iterate(nodep);
        nodep->foreach([](AstTraceDecl* மாறிலி declp) {
            எனில் (declp->inDtypeFunc()) {
                declp->valuep()->unlinkFrBack()->delete_மரம்();
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

வெறுமை V3Trace::traceAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { TraceVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_மரம்("trace", 0, dump_மரம்_Either_Level() >= 3);
}
