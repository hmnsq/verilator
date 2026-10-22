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

#അന്തര്ഭാവ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#അന്തര്ഭാവ "V3Trace.h"

#അന്തര്ഭാവ "V3Ast.h"
#അന്തര്ഭാവ "V3DupFinder.h"
#അന്തര്ഭാവ "V3EmitCBase.h"
#അന്തര്ഭാവ "V3Graph.h"
#അന്തര്ഭാവ "V3Stats.h"
#അന്തര്ഭാവ "V3UniqueNames.h"

#അന്തര്ഭാവ <limits>
#അന്തര്ഭാവ <map>
#അന്തര്ഭാവ <set>
#അന്തര്ഭാവ <unordered_map>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Graph vertexes

വർഗ്ഗം TraceActivityVertex final : പൊതു V3GraphVertex {
    VL_RTTI_IMPL(TraceActivityVertex, V3GraphVertex)
    Ast_കണ്ണി* സ്ഥിരം m_insertp;
    int32_t m_activityCode;
    തർക്കം m_slow;  // If always slow, we can use the same code
പൊതു:
    എണ്ണൽ { ACTIVITY_NEVER = ((1UL << 31) - 1) };
    എണ്ണൽ { ACTIVITY_ALWAYS = ((1UL << 31) - 2) };
    എണ്ണൽ { ACTIVITY_SLOW = 0 };
    TraceActivityVertex(V3Graph* graphp, Ast_കണ്ണി* nodep, തർക്കം slow)
        : V3GraphVertex{graphp}
        , m_insertp{nodep} {
        m_activityCode = 0;
        m_slow = slow;
    }
    TraceActivityVertex(V3Graph* graphp, int32_t code)
        : V3GraphVertex{graphp}
        , m_insertp{nullptr} {
        m_activityCode = code;
        m_slow = അസത്യം;
    }
    ~TraceActivityVertex() override = സ്ഥിരസ്ഥിതി;
    // ACCESSORS
    Ast_കണ്ണി* insertp() സ്ഥിരം {
        UASSERT(m_insertp, "Null insertp; probably called on a special always/slow");
        തിരികെ m_insertp;
    }
    string name() സ്ഥിരം override {
        എങ്കിൽ (activityAlways()) {
            തിരികെ "*ALWAYS*";
        } അല്ലെങ്കിൽ {
            തിരികെ std::string{slow() ? "*SLOW* " : ""} + insertp()->name();
        }
    }
    string dotColor() സ്ഥിരം override { തിരികെ slow() ? "yellowGreen" : "green"; }
    int32_t activityCode() സ്ഥിരം { തിരികെ m_activityCode; }
    തർക്കം activityAlways() സ്ഥിരം { തിരികെ activityCode() == ACTIVITY_ALWAYS; }
    തർക്കം activitySlow() സ്ഥിരം { തിരികെ activityCode() == ACTIVITY_SLOW; }
    ശൂന്യം activityCode(int32_t code) { m_activityCode = code; }
    തർക്കം slow() സ്ഥിരം { തിരികെ m_slow; }
    ശൂന്യം slow(തർക്കം flag) {
        എങ്കിൽ (!flag) m_slow = അസത്യം;
    }
};

വർഗ്ഗം TraceCFuncVertex final : പൊതു V3GraphVertex {
    VL_RTTI_IMPL(TraceCFuncVertex, V3GraphVertex)
    AstCFunc* സ്ഥിരം m_nodep;

പൊതു:
    TraceCFuncVertex(V3Graph* graphp, AstCFunc* nodep)
        : V3GraphVertex{graphp}
        , m_nodep{nodep} {}
    ~TraceCFuncVertex() override = സ്ഥിരസ്ഥിതി;
    // ACCESSORS
    AstCFunc* nodep() സ്ഥിരം { തിരികെ m_nodep; }
    string name() സ്ഥിരം override { തിരികെ nodep()->name(); }
    string dotColor() സ്ഥിരം override { തിരികെ "yellow"; }
    File_വരി* fileline() സ്ഥിരം override { തിരികെ nodep()->fileline(); }
};

വർഗ്ഗം TraceTraceVertex final : പൊതു V3GraphVertex {
    VL_RTTI_IMPL(TraceTraceVertex, V3GraphVertex)
    AstTraceDecl* സ്ഥിരം m_nodep;  // TRACEINC this represents
    // nullptr, or other vertex with the real code() that duplicates this one
    TraceTraceVertex* m_duplicatep = nullptr;
    // When aliasing to a dtype parent, offset of the member within the dtype
    uint32_t m_dtypeAliasOffset = 0;

പൊതു:
    TraceTraceVertex(V3Graph* graphp, AstTraceDecl* nodep)
        : V3GraphVertex{graphp}
        , m_nodep{nodep} {}
    ~TraceTraceVertex() override = സ്ഥിരസ്ഥിതി;
    // ACCESSORS
    AstTraceDecl* nodep() സ്ഥിരം { തിരികെ m_nodep; }
    string name() സ്ഥിരം override { തിരികെ nodep()->name(); }
    string dotColor() സ്ഥിരം override { തിരികെ "red"; }
    File_വരി* fileline() സ്ഥിരം override { തിരികെ nodep()->fileline(); }
    TraceTraceVertex* duplicatep() സ്ഥിരം { തിരികെ m_duplicatep; }
    ശൂന്യം duplicatep(TraceTraceVertex* dupp) {
        UASSERT_OBJ(!duplicatep(), nodep(), "Assigning duplicatep() to already duplicated node");
        m_duplicatep = dupp;
    }
    ശൂന്യം redirectDuplicatep(TraceTraceVertex* dupp) { m_duplicatep = dupp; }
    uint32_t dtypeAliasOffset() സ്ഥിരം { തിരികെ m_dtypeAliasOffset; }
    ശൂന്യം dtypeAliasOffset(uint32_t offset) { m_dtypeAliasOffset = offset; }
};

വർഗ്ഗം TraceVarVertex final : പൊതു V3GraphVertex {
    VL_RTTI_IMPL(TraceVarVertex, V3GraphVertex)
    AstVarScope* സ്ഥിരം m_nodep;

പൊതു:
    TraceVarVertex(V3Graph* graphp, AstVarScope* nodep)
        : V3GraphVertex{graphp}
        , m_nodep{nodep} {}
    ~TraceVarVertex() override = സ്ഥിരസ്ഥിതി;
    // ACCESSORS
    AstVarScope* nodep() സ്ഥിരം { തിരികെ m_nodep; }
    string name() സ്ഥിരം override { തിരികെ nodep()->name(); }
    string dotColor() സ്ഥിരം override { തിരികെ "skyblue"; }
    File_വരി* fileline() സ്ഥിരം override { തിരികെ nodep()->fileline(); }
};

//######################################################################
// Trace state, as a visitor of each AstNode

വർഗ്ഗം TraceVisitor final : പൊതു VNVisitor {
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
    സ്ഥിരം VNUser1InUse m_inuser1;
    സ്ഥിരം VNUser2InUse m_inuser2;
    സ്ഥിരം VNUser3InUse m_inuser3;
    // VNUser4InUse     In V3Hasher via V3DupFinder

    // STATE
    Ast_കണ്ണി_Module* m_മുകൾ_Modp = nullptr;  // Module to add variables to
    AstScope* സ്ഥിരം m_മുകൾ_Scopep = v3Global.rootp()->മുകൾ_Scopep()->scopep();  // The top AstScope
    AstCFunc* m_cfuncp = nullptr;  // C function adding to graph
    AstCFunc* m_regFuncp = nullptr;  // Trace registration function
    AstCFunc* m_actAllFuncp = nullptr;  // Set all activity function
    AstTraceDecl* m_tracep = nullptr;  // Trace function adding to graph
    AstVarScope* m_activityVscp = nullptr;  // Activity variable
    uint32_t m_activity_സംഖ്യ = 0;  // Count of fields in activity variable
    uint32_t m_code = 0;  // Trace ident code# being assigned
    V3Graph m_graph;  // Var/CFunc tracking
    TraceActivityVertex* സ്ഥിരം m_alwaysVtxp;  // "Always trace" vertex
    തർക്കം m_finding = അസത്യം;  // Pass one of algorithm?
    ഘടന DtypeFuncs final {
    പൊതു:
        AstCFunc* fullFuncp = nullptr;
        AstCFunc* chgFuncp = nullptr;
    };
    std::unordered_map<സ്ഥിരം Ast_കണ്ണി_DType*, DtypeFuncs>
        m_dtypeNonConstFuncs;  // Full / Chg funcs per type
    std::unordered_map<സ്ഥിരം Ast_കണ്ണി_DType*, AstCFunc*> m_dtypeConstFuncs;  // Const func per type
    V3UniqueNames m_dtypeNames{""};  // Unique type func names

    // Trace parallelism. Only VCD tracing can be parallelized at this time.
    സ്ഥിരം uint32_t m_parallelism
        = v3Global.opt.useTraceParallel() ? static_cast<uint32_t>(v3Global.opt.threads()) : 1;

    VDouble0 m_statSetters;  // Statistic tracking
    VDouble0 m_statSettersSlow;  // Statistic tracking
    VDouble0 m_statUniqSigs;  // Statistic tracking

    // All activity numbers applying to a given trace
    using ActCodeSet = std::set<uint32_t>;
    // For activity set, what traces apply
    using TraceVec = std::multimap<ActCodeSet, TraceTraceVertex*>;
    // Candidate interface-member VarScopes keyed by (interface type, member name)
    std::map<std::pair<സ്ഥിരം AstIface*, std::string>, std::vector<AstVarScope*>>
        m_ifaceMemberVscps;

    വർഗ്ഗം TraceInitDeclCollector final : പൊതു VNVisitor {
        std::vector<AstTraceDecl*>& m_declps;  // Output: trace declarations found so far
        std::set<സ്ഥിരം AstCFunc*> m_seenFuncps;

        ശൂന്യം visit(AstTraceDecl* nodep) override { m_declps.തള്ളുക_back(nodep); }
        ശൂന്യം visit(AstCCall* nodep) override {
            എങ്കിൽ (AstCFunc* സ്ഥിരം funcp = nodep->funcp()) collect(funcp);
        }
        ശൂന്യം visit(Ast_കണ്ണി* nodep) override { iterateChildren(nodep); }

    പൊതു:
        explicit TraceInitDeclCollector(std::vector<AstTraceDecl*>& declps)
            : m_declps{declps} {}
        ശൂന്യം collect(AstCFunc* funcp) {
            എങ്കിൽ (funcp && m_seenFuncps.തിരുകുക(funcp).second) iterate(funcp);
        }
    };

    // METHODS

    സ്ഥിതം തർക്കം same_വേര്_Init_Alias(സ്ഥിരം AstTraceDecl* വേര്_Declp, സ്ഥിരം AstTraceDecl* മുകൾ_Declp) {
        സ്ഥിരം VNumRange& വേര്_Bit_Range = വേര്_Declp->bitRange();
        സ്ഥിരം VNumRange& മുകൾ_Bit_Range = മുകൾ_Declp->bitRange();
        തിരികെ വേര്_Declp->showname() == മുകൾ_Declp->showname()
               && വേര്_Declp->declDirection() == മുകൾ_Declp->declDirection()
               && വേര്_Declp->widthMin() == മുകൾ_Declp->widthMin()
               && വേര്_Bit_Range.ranged() == മുകൾ_Bit_Range.ranged()
               && (!വേര്_Bit_Range.ranged()
                   || (വേര്_Bit_Range.ഇടത്() == മുകൾ_Bit_Range.ഇടത്()
                       && വേര്_Bit_Range.വലത്() == മുകൾ_Bit_Range.വലത്()));
    }

    ശൂന്യം detectDuplicates() {
        UINFO(9, "Finding duplicates");
        // Note uses user4
        V3DupFinder dupFinder;  // Duplicate code detection

        // Compute member offsets within dtype instances incrementally.
        // For each dtype member TraceDecl, stores its code offset within the
        // dtype parent's code range.
        std::unordered_map<സ്ഥിരം AstTraceDecl*, uint32_t> memberOffsets;
        std::unordered_map<സ്ഥിരം AstTraceDecl*, uint32_t> runningOffset;

        // For fixup pass: track dups redirected to dtype parents, so we can
        // fix them if the parent's user2 is later set during the loop.
        // Maps dup vertex -> original canonical member vertex.
        std::vector<std::pair<TraceTraceVertex*, TraceTraceVertex*>> dtypeParentRedirects;

        // Hash all of the traced values and find if there are any duplicates
        വേണ്ടി (V3GraphVertex& vtx : m_graph.vertices()) {
            എങ്കിൽ (TraceTraceVertex* സ്ഥിരം vvertexp = vtx.cast<TraceTraceVertex>()) {
                AstTraceDecl* സ്ഥിരം nodep = vvertexp->nodep();
                എങ്കിൽ (nodep->dtypeCallp()) തുടരുക;

                എങ്കിൽ (nodep->dtypeDeclp()) {
                    uint32_t& offset = runningOffset[nodep->dtypeDeclp()];
                    memberOffsets[nodep] = offset;
                    offset += nodep->codeInc();
                }

                UASSERT_OBJ(!vvertexp->duplicatep(), nodep, "Should not be a duplicate");
                സ്ഥിരം auto dupit = dupFinder.findDuplicate(nodep);
                എങ്കിൽ (dupit == dupFinder.end()) {
                    dupFinder.തിരുകുക(nodep);
                } അല്ലെങ്കിൽ {
                    AstTraceDecl* സ്ഥിരം dupDeclp = VN_AS(dupit->second, TraceDecl);
                    UASSERT_OBJ(dupDeclp, nodep, "Trace duplicate of wrong type");
                    TraceTraceVertex* സ്ഥിരം dupvertexp
                        = dupDeclp->user1u().toGraphVertex()->cast<TraceTraceVertex>();
                    UINFO(8, "  Orig " << dupDeclp << endl);
                    UINFO(8, "   dup " << nodep << endl);

                    എങ്കിൽ (dupDeclp->dtypeDeclp() && !dupDeclp->dtypeDeclp()->user2()) {
                        AstTraceDecl* സ്ഥിരം dtypeParentp = dupDeclp->dtypeDeclp();
                        TraceTraceVertex* സ്ഥിരം dtypeVtxp
                            = dtypeParentp->user1u().toGraphVertex()->cast<TraceTraceVertex>();
                        സ്ഥിരം auto it2 = memberOffsets.find(dupDeclp);
                        UASSERT_OBJ(it2 != memberOffsets.end(), dupDeclp,
                                    "Member offset not precomputed");
                        vvertexp->duplicatep(dtypeVtxp);
                        vvertexp->dtypeAliasOffset(it2->second);
                        dtypeParentRedirects.emplace_back(vvertexp, dupvertexp);
                    } അല്ലെങ്കിൽ {
                        vvertexp->duplicatep(dupvertexp);
                    }
                    എങ്കിൽ (nodep->dtypeDeclp()) nodep->dtypeDeclp()->user2(സത്യം);
                }
            }
        }

        വേണ്ടി (സ്ഥിരം auto& pair : dtypeParentRedirects) {
            TraceTraceVertex* സ്ഥിരം dupVtxp = pair.first;
            TraceTraceVertex* സ്ഥിരം memberVtxp = pair.second;
            സ്ഥിരം AstTraceDecl* സ്ഥിരം parentDeclp = dupVtxp->duplicatep()->nodep();
            എങ്കിൽ (parentDeclp->user2()) {
                dupVtxp->redirectDuplicatep(memberVtxp);
                dupVtxp->dtypeAliasOffset(0);
            }
        }

        എങ്കിൽ (dumpLevel() || debug() >= 9)
            dupFinder.dumpFile(v3Global.debugFilename("trace") + ".hash", അസത്യം);
    }

    ശൂന്യം graphDtypePrune() {
        വേണ്ടി (V3GraphVertex* സ്ഥിരം vtxp : m_graph.vertices().unlinkable()) {
            എങ്കിൽ (TraceTraceVertex* സ്ഥിരം vvertexp = vtxp->cast<TraceTraceVertex>()) {
                AstTraceDecl* സ്ഥിരം declp = vvertexp->nodep();
                // This skips the dtype sub-func optimization if a var is affected by multiple
                // activities.  We really only need to do this for _chg funcs (and not decls,
                // _const and _full) but it's simpiler to do it all one way or the other.
                എങ്കിൽ (declp) {
                    എങ്കിൽ (declp->user2() || (declp->dtypeDeclp() && !declp->dtypeDeclp()->user2())) {
                        AstCCall* സ്ഥിരം callp = declp->dtypeCallp();
                        എങ്കിൽ (callp) {
                            Ast_കണ്ണി* stmtexprp = callp->backp();
                            VL_DO_DANGLING(തള്ളുക_Deletep(stmtexprp->unlinkFrBack()), stmtexprp);
                        } അല്ലെങ്കിൽ {
                            തർക്കം emptyScope;
                            ചെയ്യുക {
                                emptyScope = അസത്യം;
                                Ast_കണ്ണി* സ്ഥിരം declBackp = declp->backp();
                                Ast_കണ്ണി* സ്ഥിരം declNextp = declp->nextp();
                                എങ്കിൽ (VN_IS(declBackp, Trace_തള്ളുക_Prefix)
                                    && VN_IS(declNextp, Trace_എടുക്കുക_Prefix)) {
                                    VL_DO_DANGLING(തള്ളുക_Deletep(declBackp->unlinkFrBack()),
                                                   declBackp);
                                    VL_DO_DANGLING(തള്ളുക_Deletep(declNextp->unlinkFrBack()),
                                                   declNextp);
                                    emptyScope = സത്യം;
                                }
                            } ആകുമ്പോൾ (emptyScope);
                        }
                        // Can't purge until we finish this pass
                        തള്ളുക_Deletep(declp->unlinkFrBack());  // declp used below
                        vvertexp->rerouteEdges(&m_graph);
                        vvertexp->unlinkDelete(&m_graph);
                    }
                    declp->dtypeDeclp(nullptr);
                }
            }
        }
    }

    ശൂന്യം splitTraceDeclFuncs() {
        സ്ഥിരം പൂർണ്ണസംഖ്യ splitLimit = v3Global.opt.outputSplitCTrace() ? v3Global.opt.outputSplitCTrace()
                                                                : std::numeric_limits<പൂർണ്ണസംഖ്യ>::max();
        എങ്കിൽ (splitLimit == std::numeric_limits<പൂർണ്ണസംഖ്യ>::max()) തിരികെ;

        വേണ്ടി (Ast_കണ്ണി* nodep = m_മുകൾ_Scopep->blocksp(); nodep; nodep = nodep->nextp()) {
            AstCFunc* സ്ഥിരം funcp = VN_CAST(nodep, CFunc);
            എങ്കിൽ (!funcp || !funcp->user2()) തുടരുക;

            സ്ഥിരം string baseName = funcp->name();
            uint32_t sub_അക്കം = 1;
            പൂർണ്ണസംഖ്യ curSize = 0;
            AstCFunc* curFuncp = funcp;
            std::vector<Ast_കണ്ണി_Stmt*> callStmts;

            auto startNewSubFunc = [&]() {
                File_വരി* സ്ഥിരം flp = funcp->fileline();
                സ്ഥിരം string newName = baseName + "_" + cvtToStr(sub_അക്കം++);
                AstCFunc* സ്ഥിരം newFuncp = പുതിയ AstCFunc{flp, newName, m_മുകൾ_Scopep};
                newFuncp->argTypes(v3Global.opt.traceClassBase() + "* tracep");
                newFuncp->isTrace(സത്യം);
                newFuncp->isStatic(അസത്യം);
                newFuncp->isLoose(സത്യം);
                newFuncp->slow(സത്യം);
                newFuncp->dontCombine(സത്യം);
                m_മുകൾ_Scopep->addBlocksp(newFuncp);
                newFuncp->addStmtsp(പുതിയ AstCStmt{flp, "const int c = vlSymsp->__Vm_baseCode;"});
                AstCCall* സ്ഥിരം callp = പുതിയ AstCCall{flp, newFuncp};
                callp->dtypeSetVoid();
                callp->argTypes("tracep");
                callStmts.തള്ളുക_back(callp->makeStmt());
                curFuncp = newFuncp;
                curSize = 0;
            };

            വേണ്ടി (Ast_കണ്ണി *stmtp = funcp->stmtsp(), *nextp; stmtp; stmtp = nextp) {
                nextp = stmtp->nextp();
                curSize += stmtp->കണ്ണി_എണ്ണം();

                എങ്കിൽ (curFuncp != funcp) {
                    stmtp->unlinkFrBack();
                    curFuncp->addStmtsp(stmtp);
                }

                എങ്കിൽ (curSize > splitLimit && nextp) startNewSubFunc();
            }

            വേണ്ടി (Ast_കണ്ണി_Stmt* സ്ഥിരം callStmtp : callStmts) { funcp->addStmtsp(callStmtp); }
        }
    }

    ശൂന്യം graphSimplify(തർക്കം initial) {
        എങ്കിൽ (initial) {
            // Remove all variable nodes
            വേണ്ടി (V3GraphVertex* സ്ഥിരം vtxp : m_graph.vertices().unlinkable()) {
                എങ്കിൽ (TraceVarVertex* സ്ഥിരം vvertexp = vtxp->cast<TraceVarVertex>()) {
                    vvertexp->rerouteEdges(&m_graph);
                    vvertexp->unlinkDelete(&m_graph);
                }
            }
            // Remove multiple variables connecting funcs to traces
            // We do this twice, as then we have fewer edges to multiply out in the below
            // expansion.
            m_graph.removeRedundantEdgesMax(&V3GraphEdge::followAlwaysTrue);
            // Remove all Cfunc nodes
            വേണ്ടി (V3GraphVertex* സ്ഥിരം vtxp : m_graph.vertices().unlinkable()) {
                എങ്കിൽ (TraceCFuncVertex* സ്ഥിരം vvertexp = vtxp->cast<TraceCFuncVertex>()) {
                    vvertexp->rerouteEdges(&m_graph);
                    vvertexp->unlinkDelete(&m_graph);
                }
            }
        }

        // Remove multiple variables connecting funcs to traces
        m_graph.removeRedundantEdgesMax(&V3GraphEdge::followAlwaysTrue);

        // If there are any edges from a always, keep only the always
        വേണ്ടി (V3GraphVertex& vtx : m_graph.vertices()) {
            എങ്കിൽ (TraceTraceVertex* സ്ഥിരം vvertexp = vtx.cast<TraceTraceVertex>()) {
                // Search for the incoming always edge
                സ്ഥിരം V3GraphEdge* alwaysEdgep = nullptr;
                വേണ്ടി (സ്ഥിരം V3GraphEdge& edge : vvertexp->inEdges()) {
                    സ്ഥിരം TraceActivityVertex* സ്ഥിരം actVtxp
                        = edge.fromp()->as<സ്ഥിരം TraceActivityVertex>();
                    എങ്കിൽ (actVtxp->activityAlways()) {
                        alwaysEdgep = &edge;
                        നിർത്തുക;
                    }
                }
                // If always edge exists, remove all other edges
                എങ്കിൽ (alwaysEdgep) {
                    വേണ്ടി (V3GraphEdge* സ്ഥിരം edgep : vvertexp->inEdges().unlinkable()) {
                        എങ്കിൽ (edgep != alwaysEdgep) VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
                    }
                }
            }
        }

        // Activity points with no outputs can be removed
        വേണ്ടി (V3GraphVertex* സ്ഥിരം vtxp : m_graph.vertices().unlinkable()) {
            എങ്കിൽ (TraceActivityVertex* സ്ഥിരം aVtxp = vtxp->cast<TraceActivityVertex>()) {
                // Leave in the always vertex for later use.
                എങ്കിൽ (aVtxp != m_alwaysVtxp && aVtxp->outEmpty()) {
                    VL_DO_DANGLING(aVtxp->unlinkDelete(&m_graph), aVtxp);
                }
            }
        }
    }

    uint32_t assignactivityNumbers() {
        uint32_t activity_സംഖ്യ = 1;  // Note 0 indicates "slow" only
        വേണ്ടി (V3GraphVertex& vtx : m_graph.vertices()) {
            എങ്കിൽ (TraceActivityVertex* സ്ഥിരം vvertexp = vtx.cast<TraceActivityVertex>()) {
                എങ്കിൽ (vvertexp != m_alwaysVtxp) {
                    എങ്കിൽ (vvertexp->slow()) {
                        vvertexp->activityCode(TraceActivityVertex::ACTIVITY_SLOW);
                    } അല്ലെങ്കിൽ {
                        vvertexp->activityCode(activity_സംഖ്യ++);
                    }
                }
            }
        }
        തിരികെ activity_സംഖ്യ;
    }

    ശൂന്യം sortTraces(TraceVec& traces, uint32_t& nNonConstCodes) {
        // Populate sort structure
        traces.clear();
        nNonConstCodes = 0;
        വേണ്ടി (V3GraphVertex& vtx : m_graph.vertices()) {
            എങ്കിൽ (TraceTraceVertex* സ്ഥിരം vtxp = vtx.cast<TraceTraceVertex>()) {
                ActCodeSet actSet;
                UINFO(9, "  Add to sort: " << vtxp);
                UINFOTREE(9, vtxp->nodep(), "", "trnode");
                വേണ്ടി (സ്ഥിരം V3GraphEdge& edge : vtxp->inEdges()) {
                    സ്ഥിരം TraceActivityVertex* സ്ഥിരം cfvertexp
                        = edge.fromp()->cast<സ്ഥിരം TraceActivityVertex>();
                    UASSERT_OBJ(cfvertexp, vtxp->nodep(),
                                "Should have been function pointing to this trace");
                    UINFO(9, "   Activity: " << cfvertexp);
                    എങ്കിൽ (cfvertexp->activityAlways()) {
                        // If code 0, we always trace; ignore other codes
                        actSet.തിരുകുക(TraceActivityVertex::ACTIVITY_ALWAYS);
                    } അല്ലെങ്കിൽ {
                        actSet.തിരുകുക(cfvertexp->activityCode());
                    }
                }
                UASSERT_OBJ(actSet.എണ്ണം(TraceActivityVertex::ACTIVITY_ALWAYS) == 0
                                || actSet.size() == 1,
                            vtxp->nodep(), "Always active trace has further triggers");
                // Count nodes
                എങ്കിൽ (!vtxp->duplicatep() && !actSet.empty())
                    nNonConstCodes += vtxp->nodep()->codeInc();
                എങ്കിൽ (actSet.empty()) {
                    // If a trace doesn't have activity, it's constant, and we
                    // don't need to track changes on it.
                    actSet.തിരുകുക(TraceActivityVertex::ACTIVITY_NEVER);
                } അല്ലെങ്കിൽ എങ്കിൽ (actSet.എണ്ണം(TraceActivityVertex::ACTIVITY_SLOW) && actSet.size() > 1) {
                    // If a trace depends on the slow flag as well as other
                    // flags, remove the dependency on the slow flag. We will
                    // make slow routines set all activity flags.
                    actSet.erase(TraceActivityVertex::ACTIVITY_SLOW);
                }
                traces.emplace(actSet, vtxp);
            }
        }
    }

    ശൂന്യം graphOptimize() {
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
        സ്ഥിരം auto end = traces.end();
        ആകുമ്പോൾ (it != end) {
            auto head = it;
            // Approximate the complexity of the value change check
            uint32_t complexity = 0;
            സ്ഥിരം ActCodeSet& actSet = it->first;
            വേണ്ടി (; it != end && it->first == actSet; ++it) {
                എങ്കിൽ (!it->second->duplicatep()) {
                    uint32_t cost = 0;
                    സ്ഥിരം AstTraceDecl* സ്ഥിരം declp = it->second->nodep();
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
            എങ്കിൽ (actSet.എണ്ണം(TraceActivityVertex::ACTIVITY_ALWAYS)) തുടരുക;
            എങ്കിൽ (actSet.എണ്ണം(TraceActivityVertex::ACTIVITY_NEVER)) തുടരുക;
            എങ്കിൽ (actSet.എണ്ണം(TraceActivityVertex::ACTIVITY_SLOW)) തുടരുക;
            // If the value comparisons are cheaper to perform than checking the
            // activity flags make the signals always traced. Note this cost
            // equation is heuristic.
            എങ്കിൽ (complexity <= actSet.size() * 2) {
                വേണ്ടി (; head != it; ++head) {
                    പുതിയ V3GraphEdge{&m_graph, m_alwaysVtxp, head->second, 1};
                }
            }
        }

        graphSimplify(അസത്യം);
    }

    Ast_കണ്ണി_വ്യഞ്ജകം* selectActivity(File_വരി* flp, uint32_t acode, സ്ഥിരം VAccess& access) {
        തിരികെ പുതിയ AstArraySel(flp, പുതിയ AstVarRef{flp, m_activityVscp, access}, acode);
    }

    Ast_കണ്ണി* newActivitySetter(Ast_കണ്ണി* insertp, uint32_t code) {
        ++m_statSetters;
        File_വരി* സ്ഥിരം fl = insertp->fileline();
        AstAssign* സ്ഥിരം setterp = പുതിയ AstAssign{fl, selectActivity(fl, code, VAccess::WRITE),
                                                 പുതിയ AstConst{fl, AstConst::BitTrue{}}};
        തിരികെ setterp;
    }

    Ast_കണ്ണി* newActivityAll(Ast_കണ്ണി* insertp) {
        ++m_statSettersSlow;
        എങ്കിൽ (!m_actAllFuncp) {
            File_വരി* സ്ഥിരം flp = m_മുകൾ_Scopep->fileline();
            AstCFunc* സ്ഥിരം funcp = പുതിയ AstCFunc{flp, "__VtraceActivitySetAll", m_മുകൾ_Scopep};
            funcp->slow(സത്യം);
            funcp->isStatic(അസത്യം);
            funcp->isLoose(സത്യം);
            m_മുകൾ_Scopep->addBlocksp(funcp);
            വേണ്ടി (uint32_t code = 0; code < m_activity_സംഖ്യ; ++code) {
                Ast_കണ്ണി* സ്ഥിരം setterp = newActivitySetter(insertp, code);
                funcp->addStmtsp(setterp);
            }
            m_actAllFuncp = funcp;
        }
        AstCCall* സ്ഥിരം callp = പുതിയ AstCCall{insertp->fileline(), m_actAllFuncp};
        callp->dtypeSetVoid();
        തിരികെ callp->makeStmt();
    }

    ശൂന്യം createActivityFlags() {
        // Assign final activity numbers
        m_activity_സംഖ്യ = assignactivityNumbers();

        // Create an array of bytes, not a bit vector, as they can be set
        // atomically by mtasks, and are cheaper to set (no need for
        // read-modify-write on the C type), and the speed of the tracing code
        // is the same on largish designs.
        File_വരി* സ്ഥിരം flp = m_മുകൾ_Scopep->fileline();
        Ast_കണ്ണി_DType* സ്ഥിരം newScalarDtp = പുതിയ AstBasicDType{flp, VFlagBitPacked{}, 1};
        v3Global.rootp()->typeTablep()->addTypesp(newScalarDtp);
        AstRange* സ്ഥിരം newArange
            = പുതിയ AstRange{flp, VNumRange{static_cast<പൂർണ്ണസംഖ്യ>(m_activity_സംഖ്യ) - 1, 0}};
        Ast_കണ്ണി_DType* സ്ഥിരം newArrDtp = പുതിയ AstUnpackArrayDType{flp, newScalarDtp, newArange};
        v3Global.rootp()->typeTablep()->addTypesp(newArrDtp);
        m_activityVscp = v3Global.rootp()->മുകൾ_Scopep()->createTemp("__VtraceActivity", newArrDtp);

        // Insert activity setters
        വേണ്ടി (സ്ഥിരം V3GraphVertex& vtx : m_graph.vertices()) {
            എങ്കിൽ (സ്ഥിരം TraceActivityVertex* സ്ഥിരം vtxp = vtx.cast<സ്ഥിരം TraceActivityVertex>()) {
                Ast_കണ്ണി* setterp = nullptr;
                എങ്കിൽ (vtxp->activitySlow()) {
                    setterp = newActivityAll(vtxp->insertp());
                    // Just set all flags in slow code as it should be rare.
                    // This will be rolled up into a loop by V3Reloop.
                } അല്ലെങ്കിൽ എങ്കിൽ (!vtxp->activityAlways()) {
                    setterp = newActivitySetter(vtxp->insertp(), vtxp->activityCode());
                }
                എങ്കിൽ (setterp) {
                    Ast_കണ്ണി* സ്ഥിരം insertp = vtxp->insertp();
                    എങ്കിൽ (Ast_Stmt_വ്യഞ്ജകം* സ്ഥിരം stmtp = VN_CAST(insertp, Stmt_വ്യഞ്ജകം)) {
                        stmtp->addNextHere(setterp);
                    } അല്ലെങ്കിൽ എങ്കിൽ (AstCFunc* സ്ഥിരം funcp = VN_CAST(insertp, CFunc)) {
                        // If there are awaits, insert the setter after each await
                        എങ്കിൽ (funcp->isCoroutine() && funcp->stmtsp()) {
                            funcp->stmtsp()->foreachAndNext([setterp](AstCAwait* awaitp) {
                                awaitp->addNextHere(setterp->clone_വൃക്ഷം(അസത്യം));
                            });
                        }
                        funcp->addStmtsp(setterp);
                    } അല്ലെങ്കിൽ {
                        insertp->v3fatalSrc("Bad trace activity vertex");
                    }
                }
            }
        }
    }

    AstCFunc* newCFunc(VTraceType traceType, AstCFunc* മുകൾ_Funcp, uint32_t func_അക്കം,
                       uint32_t baseCode = 0, സ്ഥിരം AstTraceDecl* സ്ഥിരം declp = nullptr,
                       തർക്കം declSub = അസത്യം) {
        // Create new function
        സ്ഥിരം തർക്കം is_മുകൾ_Func = !declp && മുകൾ_Funcp == nullptr;
        std::string funcName;
        എങ്കിൽ (is_മുകൾ_Func || declp) {
            funcName = traceType.func_prefix();
        } അല്ലെങ്കിൽ {
            funcName = മുകൾ_Funcp->name();
            funcName += "_sub";
        }
        എങ്കിൽ (declp) {
            funcName += "_dtype";
            എങ്കിൽ (declSub) funcName += "_sub";
            funcName += "__";
            funcName = m_dtypeNames.get(funcName);
        } അല്ലെങ്കിൽ {
            funcName += "_";
            funcName += cvtToStr(func_അക്കം);
        }

        File_വരി* സ്ഥിരം flp = m_മുകൾ_Scopep->fileline();
        AstCFunc* സ്ഥിരം funcp = പുതിയ AstCFunc{flp, funcName, m_മുകൾ_Scopep};
        funcp->isTrace(സത്യം);
        funcp->dontCombine(സത്യം);
        funcp->isLoose(സത്യം);
        funcp->slow(traceType != VTraceType::CHANGE);
        funcp->isStatic(is_മുകൾ_Func);
        // Add it to top scope
        m_മുകൾ_Scopep->addBlocksp(funcp);
        സ്ഥിരം std::string bufArg
            = v3Global.opt.traceClassBase() + "::Buffer* bufp"
              + (declp ? (", uint32_t offset, const " + declp->dtypep()->cType("", സത്യം, സത്യം)
                          + " __VdtypeVar")
                       : "");
        എങ്കിൽ (is_മുകൾ_Func) {
            // Top functions
            funcp->argTypes("void* voidSelf, " + bufArg);
            funcp->addStmtsp(പുതിയ AstCStmt{flp, EmitCUtil::voidSelfAssign(m_മുകൾ_Modp)});
            funcp->addStmtsp(പുതിയ AstCStmt{flp, EmitCUtil::symClassAssign()});
            // Add global activity check to change dump functions
            എങ്കിൽ (traceType == VTraceType::CHANGE) {  //
                funcp->addStmtsp(
                    പുതിയ AstCStmt{flp, "if (VL_UNLIKELY(!vlSymsp->__Vm_activity)) return;"});
            }
            // Register function
            AstCStmt* സ്ഥിരം cstmtp = പുതിയ AstCStmt{flp};
            m_regFuncp->addStmtsp(cstmtp);
            എങ്കിൽ (traceType == VTraceType::CONSTANT) {
                cstmtp->add("tracep->addConstCb(");
                cstmtp->add(പുതിയ AstAddrOfCFunc{flp, funcp});
                cstmtp->add(", " + std::to_string(func_അക്കം) + ", vlSelf);");
            } അല്ലെങ്കിൽ എങ്കിൽ (traceType == VTraceType::FULL) {
                cstmtp->add("tracep->addFullCb(");
                cstmtp->add(പുതിയ AstAddrOfCFunc{flp, funcp});
                cstmtp->add(", " + std::to_string(func_അക്കം) + ", vlSelf);");
            } അല്ലെങ്കിൽ {
                cstmtp->add("tracep->addChgCb(");
                cstmtp->add(പുതിയ AstAddrOfCFunc{flp, funcp});
                cstmtp->add(", " + std::to_string(func_അക്കം) + ", vlSelf);");
            }
        } അല്ലെങ്കിൽ {
            // Sub functions
            funcp->argTypes(bufArg);
            // Setup base references. Note in rare occasions we can end up with an empty trace
            // sub function, hence the VL_ATTR_UNUSED attributes.
            എങ്കിൽ (traceType != VTraceType::CHANGE) {
                // Full dump sub function
                funcp->addStmtsp(പുതിയ AstCStmt{flp,  //
                                              string("uint32_t* const oldp VL_ATTR_UNUSED = "
                                                     "bufp->oldp(vlSymsp->__Vm_baseCode")
                                                  + (declp ? " + offset" : "") + ");\n"});
            } അല്ലെങ്കിൽ {
                // Change dump sub function
                funcp->addStmtsp(പുതിയ AstCStmt{flp,  //
                                              "uint32_t* const oldp VL_ATTR_UNUSED = "
                                              "bufp->oldp(vlSymsp->__Vm_baseCode + "
                                                  + (declp ? " offset" : cvtToStr(baseCode))
                                                  + ");\n"});
            }
            എങ്കിൽ (!declp) {
                // Add call to top function
                AstCCall* സ്ഥിരം callp = പുതിയ AstCCall{funcp->fileline(), funcp};
                callp->dtypeSetVoid();
                callp->argTypes("bufp");
                മുകൾ_Funcp->addStmtsp(callp->makeStmt());
            }
        }
        // Done
        UINFO(5, "  newCFunc " << funcp);
        തിരികെ funcp;
    }

    AstCFunc* newCDtypeSubFunc(VTraceType traceType, സ്ഥിരം AstTraceDecl* സ്ഥിരം declp,
                               AstCFunc* parentp) {
        AstCFunc* സ്ഥിരം funcp = newCFunc(traceType, nullptr, 0, 0, declp, സത്യം);
        File_വരി* സ്ഥിരം flp = m_മുകൾ_Scopep->fileline();
        AstCCall* സ്ഥിരം callp = പുതിയ AstCCall{flp, funcp};
        callp->dtypeSetVoid();
        callp->argTypes("bufp, offset, __VdtypeVar");
        parentp->addStmtsp(callp->makeStmt());
        തിരികെ funcp;
    }

    AstCFunc* createConstDtypeTraceFunctions(സ്ഥിരം AstTraceDecl* declp) {
        സ്ഥിരം Ast_കണ്ണി_DType* സ്ഥിരം dtypep = declp->dtypep()->skipRefp();
        auto pair = m_dtypeConstFuncs.emplace(dtypep, nullptr);
        എങ്കിൽ (pair.second) {
            File_വരി* സ്ഥിരം flp = declp->fileline();
            AstCFunc* സ്ഥിരം funcp = newCFunc(VTraceType::CONSTANT, nullptr, 0, 0, declp);

            തർക്കം first = സത്യം;
            വേണ്ടി (Ast_കണ്ണി* callStmtp = declp->dtypeCallp()->funcp()->stmtsp(); callStmtp;
                 callStmtp = callStmtp->nextp()) {
                സ്ഥിരം AstCCall* സ്ഥിരം callp
                    = VN_CAST(VN_CAST(callStmtp, Stmt_വ്യഞ്ജകം)->exprp(), CCall);
                തർക്കം onlyOne = first && !callStmtp->nextp();
                AstCFunc* സ്ഥിരം subFuncp
                    = onlyOne ? funcp : newCDtypeSubFunc(VTraceType::CONSTANT, declp, funcp);
                വേണ്ടി (Ast_കണ്ണി* stmtp = callp->funcp()->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    എങ്കിൽ (AstTraceDecl* സ്ഥിരം fieldDeclp = VN_CAST(stmtp, TraceDecl)) {
                        AstTraceInc* സ്ഥിരം incp
                            = പുതിയ AstTraceInc{flp, fieldDeclp, VTraceType::CONSTANT};
                        subFuncp->addStmtsp(incp);
                    }
                }
                first = അസത്യം;
            }

            pair.first->second = funcp;
        }

        തിരികെ pair.first->second;
    }

    ശൂന്യം createConstTraceFunctions(സ്ഥിരം TraceVec& traces) {
        സ്ഥിരം പൂർണ്ണസംഖ്യ splitLimit = v3Global.opt.outputSplitCTrace() ? v3Global.opt.outputSplitCTrace()
                                                                : std::numeric_limits<പൂർണ്ണസംഖ്യ>::max();

        AstCFunc* സ്ഥിരം മുകൾ_Funcp = newCFunc(VTraceType::CONSTANT, nullptr, 0);
        uint32_t sub_Func_അക്കം = 0;
        AstCFunc* subFuncp = nullptr;
        പൂർണ്ണസംഖ്യ subStmts = 0;
        std::vector<സ്ഥിരം TraceTraceVertex*> duplicates;
        വേണ്ടി (auto it = traces.cbegin(); it != traces.end(); ++it) {
            സ്ഥിരം TraceTraceVertex* സ്ഥിരം vtxp = it->second;
            AstTraceDecl* സ്ഥിരം declp = vtxp->nodep();
            എങ്കിൽ (vtxp->duplicatep()) {
                duplicates.തള്ളുക_back(vtxp);
                തുടരുക;
            }

            // This is a canonical trace node. Assign trace code (signal number).
            UASSERT_OBJ(!declp->codeAssigned(), declp,
                        "Canonical node should not have code assigned yet");
            declp->code(m_code);
            സ്ഥിരം uint32_t codeInc = declp->codeInc();
            m_code += codeInc;
            ++m_statUniqSigs;

            // If this is a const signal, add the AstTraceInc
            സ്ഥിരം ActCodeSet& actSet = it->first;
            എങ്കിൽ (actSet.എണ്ണം(TraceActivityVertex::ACTIVITY_NEVER)) {
                // Crate new sub function if required
                എങ്കിൽ (!subFuncp || subStmts > splitLimit) {
                    subStmts = 0;
                    subFuncp = newCFunc(VTraceType::CONSTANT, മുകൾ_Funcp, sub_Func_അക്കം);
                    ++sub_Func_അക്കം;
                }
                File_വരി* സ്ഥിരം flp = declp->fileline();
                എങ്കിൽ (declp->dtypeCallp()) {
                    AstCFunc* സ്ഥിരം funcp = createConstDtypeTraceFunctions(declp);
                    Ast_കണ്ണി_വ്യഞ്ജകം* argsp = nullptr;
                    argsp = Ast_കണ്ണി::addNext(argsp, declp->valuep()->clone_വൃക്ഷം(അസത്യം));
                    AstCCall* സ്ഥിരം callp = പുതിയ AstCCall{flp, funcp, argsp};
                    callp->dtypeSetVoid();
                    callp->argTypes(callp->argTypes() + "bufp, " + std::to_string(declp->code()));
                    subFuncp->addStmtsp(callp->makeStmt());

                    subStmts += 1;
                } അല്ലെങ്കിൽ {
                    AstTraceInc* സ്ഥിരം incp = പുതിയ AstTraceInc{flp, declp, VTraceType::CONSTANT};
                    subFuncp->addStmtsp(incp);
                    subStmts += incp->കണ്ണി_എണ്ണം();
                }
            }
        }

        // Assign codes to duplicate nodes from their canonicals
        വേണ്ടി (സ്ഥിരം TraceTraceVertex* സ്ഥിരം vtxp : duplicates) {
            AstTraceDecl* സ്ഥിരം declp = vtxp->nodep();
            സ്ഥിരം TraceTraceVertex* സ്ഥിരം canonVtxp = vtxp->duplicatep();
            സ്ഥിരം AstTraceDecl* സ്ഥിരം canonDeclp = canonVtxp->nodep();
            UASSERT_OBJ(!canonVtxp->duplicatep(), canonDeclp, "Canonical node is a duplicate");
            UASSERT_OBJ(canonDeclp->codeAssigned(), canonDeclp,
                        "Canonical node should have code assigned already");
            declp->code(canonDeclp->code() + vtxp->dtypeAliasOffset());
        }
    }

    DtypeFuncs createNonConstDtypeTraceFunctions(സ്ഥിരം AstTraceDecl* declp) {
        Ast_കണ്ണി_DType* dtypep = declp->dtypep()->skipRefp();
        auto pair = m_dtypeNonConstFuncs.emplace(dtypep, DtypeFuncs{});
        എങ്കിൽ (pair.second) {
            File_വരി* സ്ഥിരം flp = declp->fileline();
            AstCFunc* സ്ഥിരം fullFuncp = newCFunc(VTraceType::FULL, nullptr, 0, 0, declp);
            AstCFunc* സ്ഥിരം chgFuncp = newCFunc(VTraceType::CHANGE, nullptr, 0, 0, declp);

            തർക്കം first = സത്യം;
            വേണ്ടി (Ast_കണ്ണി* callStmtp = declp->dtypeCallp()->funcp()->stmtsp(); callStmtp;
                 callStmtp = callStmtp->nextp()) {
                സ്ഥിരം AstCCall* സ്ഥിരം callp
                    = VN_CAST(VN_CAST(callStmtp, Stmt_വ്യഞ്ജകം)->exprp(), CCall);
                തർക്കം onlyOne = first && !callStmtp->nextp();
                AstCFunc* സ്ഥിരം fullSubFuncp
                    = onlyOne ? fullFuncp : newCDtypeSubFunc(VTraceType::FULL, declp, fullFuncp);
                AstCFunc* സ്ഥിരം chgSubFuncp
                    = onlyOne ? chgFuncp : newCDtypeSubFunc(VTraceType::CHANGE, declp, chgFuncp);
                വേണ്ടി (Ast_കണ്ണി* stmtp = callp->funcp()->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    എങ്കിൽ (AstTraceDecl* സ്ഥിരം fieldDeclp = VN_CAST(stmtp, TraceDecl)) {
                        AstTraceInc* സ്ഥിരം incFullp
                            = പുതിയ AstTraceInc{flp, fieldDeclp, VTraceType::FULL};
                        fullSubFuncp->addStmtsp(incFullp);
                        AstTraceInc* സ്ഥിരം incChgp
                            = പുതിയ AstTraceInc{flp, fieldDeclp, VTraceType::CHANGE};
                        chgSubFuncp->addStmtsp(incChgp);
                    }
                }
                first = അസത്യം;
            }

            pair.first->second = {.fullFuncp = fullFuncp, .chgFuncp = chgFuncp};
        }

        തിരികെ pair.first->second;
    }

    ശൂന്യം createNonConstTraceFunctions(സ്ഥിരം TraceVec& traces, uint32_t nAllCodes,
                                      uint32_t parallelism) {
        സ്ഥിരം പൂർണ്ണസംഖ്യ splitLimit = v3Global.opt.outputSplitCTrace() ? v3Global.opt.outputSplitCTrace()
                                                                : std::numeric_limits<പൂർണ്ണസംഖ്യ>::max();

        // pre-incremented, so starts at 0
        uint32_t മുകൾ_Func_അക്കം = std::numeric_limits<uint32_t>::max();
        TraceVec::const_iterator it = traces.begin();
        ആകുമ്പോൾ (it != traces.end()) {
            AstCFunc* മുകൾ_Ful_Funcp = nullptr;
            AstCFunc* മുകൾ_Chg_Funcp = nullptr;
            AstCFunc* subFulFuncp = nullptr;
            AstCFunc* subChgFuncp = nullptr;
            uint32_t sub_Func_അക്കം = 0;
            പൂർണ്ണസംഖ്യ subStmts = 0;
            സ്ഥിരം uint32_t maxCodes = std::max((nAllCodes + parallelism - 1) / parallelism, 1U);
            uint32_t nCodes = 0;
            സ്ഥിരം ActCodeSet* prevActSet = nullptr;
            AstIf* ifp = nullptr;
            uint32_t baseCode = 0;
            വേണ്ടി (; nCodes < maxCodes && it != traces.end(); ++it) {
                സ്ഥിരം ActCodeSet& actSet = it->first;
                // Traced value never changes, no need to add it
                എങ്കിൽ (actSet.എണ്ണം(TraceActivityVertex::ACTIVITY_NEVER)) തുടരുക;

                സ്ഥിരം TraceTraceVertex* സ്ഥിരം vtxp = it->second;
                AstTraceDecl* സ്ഥിരം declp = vtxp->nodep();

                // This is a duplicate decl, no need to add it, but must set the
                // function index to the same as the canonical node.
                എങ്കിൽ (സ്ഥിരം TraceTraceVertex* സ്ഥിരം canonVtxp = vtxp->duplicatep()) {
                    declp->fidx(canonVtxp->nodep()->fidx());
                    തുടരുക;
                }

                // Create top function if not yet created
                എങ്കിൽ (!മുകൾ_Ful_Funcp) {
                    ++മുകൾ_Func_അക്കം;
                    മുകൾ_Ful_Funcp = newCFunc(VTraceType::FULL, nullptr, മുകൾ_Func_അക്കം);
                    മുകൾ_Chg_Funcp = newCFunc(VTraceType::CHANGE, nullptr, മുകൾ_Func_അക്കം);
                }

                // Create new sub function if required
                എങ്കിൽ (!subFulFuncp || subStmts > splitLimit) {
                    baseCode = declp->code();
                    subStmts = 0;
                    subFulFuncp = newCFunc(VTraceType::FULL, മുകൾ_Ful_Funcp, sub_Func_അക്കം, baseCode);
                    subChgFuncp = newCFunc(VTraceType::CHANGE, മുകൾ_Chg_Funcp, sub_Func_അക്കം, baseCode);
                    ++sub_Func_അക്കം;
                    prevActSet = nullptr;
                    ifp = nullptr;
                }

                // If required, create the conditional node checking the activity flags
                എങ്കിൽ (!prevActSet || actSet != *prevActSet) {
                    File_വരി* സ്ഥിരം flp = m_മുകൾ_Scopep->fileline();
                    സ്ഥിരം തർക്കം always = actSet.എണ്ണം(TraceActivityVertex::ACTIVITY_ALWAYS) != 0;
                    Ast_കണ്ണി_വ്യഞ്ജകം* condp = nullptr;
                    എങ്കിൽ (always) {
                        condp = പുതിയ AstConst{flp, 1};  // Always true, will be folded later
                    } അല്ലെങ്കിൽ {
                        വേണ്ടി (സ്ഥിരം uint32_t actCode : actSet) {
                            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം selp = selectActivity(flp, actCode, VAccess::READ);
                            condp = condp ? പുതിയ AstOr{flp, condp, selp} : selp;
                        }
                    }
                    ifp = പുതിയ AstIf{flp, condp};
                    എങ്കിൽ (!always) ifp->branchPred(VBranchPred::BP_UNLIKELY);
                    subChgFuncp->addStmtsp(ifp);
                    subStmts += ifp->കണ്ണി_എണ്ണം();
                    prevActSet = &actSet;
                }

                // Add TraceInc nodes
                File_വരി* സ്ഥിരം flp = declp->fileline();
                എങ്കിൽ (declp->dtypeCallp()) {
                    DtypeFuncs funcs = createNonConstDtypeTraceFunctions(declp);
                    Ast_കണ്ണി_വ്യഞ്ജകം* argsp = nullptr;
                    argsp = Ast_കണ്ണി::addNext(argsp, declp->valuep()->clone_വൃക്ഷം(അസത്യം));
                    AstCCall* സ്ഥിരം callFullp = പുതിയ AstCCall{flp, funcs.fullFuncp, argsp};
                    callFullp->dtypeSetVoid();
                    callFullp->argTypes(callFullp->argTypes() + "bufp, "
                                        + std::to_string(declp->code()));
                    subFulFuncp->addStmtsp(callFullp->makeStmt());
                    argsp = nullptr;
                    argsp = Ast_കണ്ണി::addNext(argsp, declp->valuep()->clone_വൃക്ഷം(അസത്യം));
                    AstCCall* സ്ഥിരം callChgp = പുതിയ AstCCall{flp, funcs.chgFuncp, argsp};
                    callChgp->dtypeSetVoid();
                    callChgp->argTypes(callChgp->argTypes() + "bufp, "
                                       + std::to_string(declp->code()));
                    ifp->addThensp(callChgp->makeStmt());

                    subStmts += 2;
                } അല്ലെങ്കിൽ {
                    AstTraceInc* സ്ഥിരം incFulp = പുതിയ AstTraceInc{flp, declp, VTraceType::FULL};
                    subFulFuncp->addStmtsp(incFulp);
                    AstTraceInc* സ്ഥിരം incChgp
                        = പുതിയ AstTraceInc{flp, declp, VTraceType::CHANGE, baseCode};
                    ifp->addThensp(incChgp);

                    // Set the function index of the decl
                    declp->fidx(മുകൾ_Func_അക്കം);

                    // Track splitting due to size
                    UASSERT_OBJ(incFulp->കണ്ണി_എണ്ണം() == incChgp->കണ്ണി_എണ്ണം(), declp,
                                "Should have equal cost");
                    സ്ഥിരം VNumRange range = declp->arrayRange();
                    എങ്കിൽ (range.ranged()) {
                        // 2x because each element is a TraceInc and a VarRef
                        subStmts += range.elements() * 2;
                    } അല്ലെങ്കിൽ {
                        subStmts += incChgp->കണ്ണി_എണ്ണം();
                    }
                }

                // Track partitioning
                nCodes += declp->codeInc();
            }
        }
    }

    ശൂന്യം createCleanupFunction() {
        File_വരി* സ്ഥിരം fl = m_മുകൾ_Scopep->fileline();
        AstCFunc* സ്ഥിരം cleanupFuncp = പുതിയ AstCFunc{fl, "trace_cleanup", m_മുകൾ_Scopep};
        cleanupFuncp->argTypes("void* voidSelf, " + v3Global.opt.traceClassBase()
                               + "* /*unused*/");
        cleanupFuncp->isTrace(സത്യം);
        cleanupFuncp->slow(അസത്യം);
        cleanupFuncp->isStatic(സത്യം);
        cleanupFuncp->isLoose(സത്യം);
        m_മുകൾ_Scopep->addBlocksp(cleanupFuncp);
        cleanupFuncp->addStmtsp(പുതിയ AstCStmt{fl, EmitCUtil::voidSelfAssign(m_മുകൾ_Modp)});
        cleanupFuncp->addStmtsp(പുതിയ AstCStmt{fl, EmitCUtil::symClassAssign()});

        // Register it
        {
            AstCStmt* സ്ഥിരം cstmtp = പുതിയ AstCStmt{fl};
            m_regFuncp->addStmtsp(cstmtp);
            cstmtp->add("tracep->addCleanupCb(");
            cstmtp->add(പുതിയ AstAddrOfCFunc{fl, cleanupFuncp});
            cstmtp->add(", vlSelf);");
        }

        // Clear global activity flag
        cleanupFuncp->addStmtsp(
            പുതിയ AstCStmt{m_മുകൾ_Scopep->fileline(), "vlSymsp->__Vm_activity = false;"s});

        // Clear fine grained activity flags
        വേണ്ടി (uint32_t i = 0; i < m_activity_സംഖ്യ; ++i) {
            Ast_കണ്ണി* സ്ഥിരം clrp = പുതിയ AstAssign{fl, selectActivity(fl, i, VAccess::WRITE),
                                                പുതിയ AstConst{fl, AstConst::BitFalse{}}};
            cleanupFuncp->addStmtsp(clrp);
        }
    }

    ശൂന്യം createTraceFunctions() {
        // Detect and remove duplicate values
        detectDuplicates();

        graphDtypePrune();
        splitTraceDeclFuncs();
        m_graph.removeRedundantEdgesMax(&V3GraphEdge::followAlwaysTrue);

        // Simplify & optimize the graph
        എങ്കിൽ (dumpGraphLevel() >= 6) m_graph.dumpDotFilePrefixed("trace_pre");
        graphSimplify(സത്യം);
        എങ്കിൽ (dumpGraphLevel() >= 6) m_graph.dumpDotFilePrefixed("trace_simplified");
        graphOptimize();
        എങ്കിൽ (dumpGraphLevel() >= 6) m_graph.dumpDotFilePrefixed("trace_optimized");

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
        m_regFuncp = പുതിയ AstCFunc{m_മുകൾ_Scopep->fileline(), "trace_register", m_മുകൾ_Scopep};
        m_regFuncp->argTypes(v3Global.opt.traceClassBase() + "* tracep");
        m_regFuncp->entryPoint(സത്യം);
        m_regFuncp->isTrace(സത്യം);
        m_regFuncp->slow(സത്യം);
        m_regFuncp->isStatic(അസത്യം);
        m_regFuncp->isLoose(സത്യം);
        m_മുകൾ_Scopep->addBlocksp(m_regFuncp);

        // Create the const dump functions. Also allocates trace codes.
        createConstTraceFunctions(traces);

        // Create the full and incremental dump functions
        createNonConstTraceFunctions(traces, nNonConstCodes, m_parallelism);

        // Root-traced libraries alias wrapper IOs onto the existing top-module codes.
        എങ്കിൽ (!v3Global.opt.libCreate().empty()) {
            std::vector<AstTraceDecl*> വേര്_Declps;
            std::vector<AstTraceDecl*> മുകൾ_Declps;
            TraceInitDeclCollector വേര്_Collector{വേര്_Declps};
            TraceInitDeclCollector മുകൾ_Collector{മുകൾ_Declps};
            വേണ്ടി (Ast_കണ്ണി* blockp = m_മുകൾ_Scopep->blocksp(); blockp; blockp = blockp->nextp()) {
                AstCFunc* സ്ഥിരം funcp = VN_CAST(blockp, CFunc);
                എങ്കിൽ (funcp && VString::startsWith(funcp->name(), "trace_init_leaf_root__")) {
                    വേര്_Collector.collect(funcp);
                } അല്ലെങ്കിൽ എങ്കിൽ (funcp && VString::startsWith(funcp->name(), "trace_init_leaf_top__")) {
                    മുകൾ_Collector.collect(funcp);
                }
            }

            std::vector<തർക്കം> used(മുകൾ_Declps.size(), അസത്യം);
            വേണ്ടി (AstTraceDecl* സ്ഥിരം വേര്_Declp : വേര്_Declps) {
                വേണ്ടി (size_t i = 0; i < മുകൾ_Declps.size(); ++i) {
                    എങ്കിൽ (used[i] || !same_വേര്_Init_Alias(വേര്_Declp, മുകൾ_Declps[i])) തുടരുക;
                    വേര്_Declp->code(മുകൾ_Declps[i]->code());
                    used[i] = സത്യം;
                    നിർത്തുക;
                }
            }
        }

        // Remove refs to traced values from TraceDecl nodes, these have now moved under
        // TraceInc
        വേണ്ടി (സ്ഥിരം auto& i : traces) {
            Ast_കണ്ണി* സ്ഥിരം valuep = i.second->nodep()->valuep();
            VL_DO_DANGLING(valuep->unlinkFrBack()->delete_വൃക്ഷം(), valuep);
        }

        // Create the trace cleanup function clearing the activity flags
        createCleanupFunction();
    }

    TraceCFuncVertex* getCFuncVertexp(AstCFunc* nodep) {
        V3GraphVertex* സ്ഥിരം vtxp = nodep->user1u().toGraphVertex();
        TraceCFuncVertex* vertexp = vtxp ? vtxp->cast<TraceCFuncVertex>() : nullptr;
        എങ്കിൽ (!vertexp) {
            vertexp = പുതിയ TraceCFuncVertex{&m_graph, nodep};
            nodep->user1p(vertexp);
        }
        തിരികെ vertexp;
    }
    TraceActivityVertex* getActivityVertexp(Ast_കണ്ണി* nodep, തർക്കം slow) {
        V3GraphVertex* സ്ഥിരം vtxp = nodep->user3u().toGraphVertex();
        TraceActivityVertex* vertexp = vtxp ? vtxp->cast<TraceActivityVertex>() : nullptr;
        എങ്കിൽ (!vertexp) {
            vertexp = പുതിയ TraceActivityVertex{&m_graph, nodep, slow};
            nodep->user3p(vertexp);
        }
        vertexp->slow(slow);
        തിരികെ vertexp;
    }

    // VISITORS
    ശൂന്യം visit(AstNetlist* nodep) override {
        // Add vertexes for all TraceDecl, and edges from VARs each trace looks at
        m_finding = അസത്യം;
        iterateChildren(nodep);

        // Add vertexes for all CFUNCs, and edges to VARs the func sets
        m_finding = സത്യം;
        iterateChildren(nodep);

        // Create the trace functions and insert them into the tree
        createTraceFunctions();

        // Save number of trace codes used
        nodep->nTraceCodes(m_code);
    }
    ശൂന്യം visit(Ast_കണ്ണി_Module* nodep) override {
        എങ്കിൽ (nodep->is_മുകൾ()) m_മുകൾ_Modp = nodep;
        iterateChildren(nodep);
    }
    ശൂന്യം visit(AstVarScope* nodep) override {
        എങ്കിൽ (!m_finding) {
            എങ്കിൽ (സ്ഥിരം AstIface* സ്ഥിരം ifacep = nodep->varp()->sensIfacep()) {
                m_ifaceMemberVscps[{ifacep, nodep->varp()->name()}].തള്ളുക_back(nodep);
            }
        }
    }
    ശൂന്യം visit(Ast_Stmt_വ്യഞ്ജകം* nodep) override {
        എങ്കിൽ (!m_finding && !nodep->user2()) {
            എങ്കിൽ (AstCCall* സ്ഥിരം callp = VN_CAST(nodep->exprp(), CCall)) {
                UINFO(8, "   CCALL " << callp);
                // See if there are other calls in same statement list;
                // If so, all funcs might share the same activity code
                TraceActivityVertex* സ്ഥിരം activityVtxp
                    = getActivityVertexp(nodep, callp->funcp()->slow());
                വേണ്ടി (Ast_കണ്ണി* nextp = nodep; nextp; nextp = nextp->nextp()) {
                    എങ്കിൽ (Ast_Stmt_വ്യഞ്ജകം* സ്ഥിരം stmtp = VN_CAST(nextp, Stmt_വ്യഞ്ജകം)) {
                        എങ്കിൽ (AstCCall* സ്ഥിരം ccallp = VN_CAST(stmtp->exprp(), CCall)) {
                            stmtp->user2(സത്യം);  // Processed
                            UINFO(8, "     SubCCALL " << ccallp);
                            V3GraphVertex* സ്ഥിരം ccallFuncVtxp = getCFuncVertexp(ccallp->funcp());
                            activityVtxp->slow(ccallp->funcp()->slow());
                            പുതിയ V3GraphEdge{&m_graph, activityVtxp, ccallFuncVtxp, 1};
                        }
                    }
                }
            }
        }
        iterateChildren(nodep);
    }
    ശൂന്യം visit(AstCFunc* nodep) override {
        UINFO(8, "   CFUNC " << nodep);
        V3GraphVertex* സ്ഥിരം funcVtxp = getCFuncVertexp(nodep);
        എങ്കിൽ (!m_finding) {  // If public, we need a unique activity code to allow for sets
                           // directly in this func
            എങ്കിൽ (nodep->funcPublic() || nodep->dpiExportImpl() || nodep->entryPoint()
                || nodep->isCoroutine()) {
                // Cannot treat a coroutine as slow, it may be resumed later
                സ്ഥിരം തർക്കം slow = nodep->slow() && !nodep->isCoroutine();
                TraceActivityVertex* സ്ഥിരം activityVtxp = getActivityVertexp(nodep, slow);
                പുതിയ V3GraphEdge{&m_graph, activityVtxp, funcVtxp, 1};
            }
        }
        VL_RESTORER(m_cfuncp);
        m_cfuncp = nodep;
        iterateChildren(nodep);
    }
    ശൂന്യം visit(AstTraceDecl* nodep) override {
        UINFO(8, "   TRACE " << nodep);
        എങ്കിൽ (!m_finding && !nodep->inDtypeFunc()) {
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
            എങ്കിൽ (m_cfuncp && VString::startsWith(m_cfuncp->name(), "trace_init_leaf_root__")) {
                തിരികെ;
            }
            V3GraphVertex* സ്ഥിരം vertexp = പുതിയ TraceTraceVertex{&m_graph, nodep};
            nodep->user1p(vertexp);

            UASSERT_OBJ(m_cfuncp, nodep, "Trace not under func");
            m_cfuncp->user2(സത്യം);
            VL_RESTORER(m_tracep);
            m_tracep = nodep;
            iterateChildren(nodep);
        }
    }
    ശൂന്യം visit(AstVarRef* nodep) override {
        UASSERT_OBJ(nodep->varScopep(), nodep, "No var scope?");
        AstVarScope* സ്ഥിരം varscopep = nodep->varScopep();
        V3GraphVertex* varVtxp = varscopep->user1u().toGraphVertex();
        എങ്കിൽ (m_tracep) {
            UASSERT_OBJ(nodep->access().isReadOnly(), nodep, "Lvalue in trace?  Should be const.");
            എങ്കിൽ (!varVtxp) {
                varVtxp = പുതിയ TraceVarVertex{&m_graph, nodep->varScopep()};
                nodep->varScopep()->user1p(varVtxp);
            }
            V3GraphVertex* സ്ഥിരം traceVtxp = m_tracep->user1u().toGraphVertex();
            പുതിയ V3GraphEdge{&m_graph, varVtxp, traceVtxp, 1};
            എങ്കിൽ (nodep->varp()->isPrimaryInish()  // Always need to trace primary inputs
                || nodep->varp()->isSigPublic()) {  // Or ones user can change
                പുതിയ V3GraphEdge{&m_graph, m_alwaysVtxp, traceVtxp, 1};
            }
            എങ്കിൽ (m_tracep->dtypeCallp()) varscopep->user2p(m_tracep);
        } അല്ലെങ്കിൽ എങ്കിൽ (m_cfuncp && m_finding && nodep->access().isWriteOrRW()) {
            V3GraphVertex* സ്ഥിരം funcVtxp = getCFuncVertexp(m_cfuncp);
            എങ്കിൽ (varVtxp) {  // else we're not tracing this signal
                പുതിയ V3GraphEdge{&m_graph, funcVtxp, varVtxp, 1};
                AstTraceDecl* സ്ഥിരം declp = VN_AS(varscopep->user2p(), TraceDecl);
                എങ്കിൽ (declp) {
                    V3GraphVertex* സ്ഥിരം cFuncVtxp = getCFuncVertexp(m_cfuncp);
                    വേണ്ടി (സ്ഥിരം V3GraphEdge& edge : cFuncVtxp->inEdges()) {
                        V3GraphVertex* സ്ഥിരം activityp = edge.fromp();
                        എങ്കിൽ (!declp->user3p()) {
                            declp->user3p(activityp);
                        } അല്ലെങ്കിൽ എങ്കിൽ (declp->user3u().toGraphVertex() != activityp) {
                            declp->user2(സത്യം);
                        }
                    }
                }
            }
        }
    }
    ശൂന്യം visit(AstMemberSel* nodep) override {
        എങ്കിൽ (m_cfuncp && m_finding && nodep->access().isWriteOrRW()) {
            AstIfaceRefDType* സ്ഥിരം dtypep
                = VN_CAST(nodep->fromp()->dtypep()->skipRefp(), IfaceRefDType);
            എങ്കിൽ (dtypep && dtypep->isVirtual()) {
                സ്ഥിരം auto it = m_ifaceMemberVscps.find({dtypep->ifacep(), nodep->varp()->name()});
                എങ്കിൽ (it != m_ifaceMemberVscps.end()) {
                    V3GraphVertex* സ്ഥിരം funcVtxp = getCFuncVertexp(m_cfuncp);
                    വേണ്ടി (AstVarScope* സ്ഥിരം vscp : it->second) {
                        V3GraphVertex* varVtxp = vscp->user1u().toGraphVertex();
                        എങ്കിൽ (!varVtxp) {
                            varVtxp = പുതിയ TraceVarVertex{&m_graph, vscp};
                            vscp->user1p(varVtxp);
                        }
                        പുതിയ V3GraphEdge{&m_graph, funcVtxp, varVtxp, 1};
                    }
                }
            }
        }
        iterateChildren(nodep);
    }
    //--------------------
    ശൂന്യം visit(Ast_കണ്ണി* nodep) override { iterateChildren(nodep); }

പൊതു:
    // CONSTRUCTORS
    explicit TraceVisitor(AstNetlist* nodep)
        : m_alwaysVtxp{പുതിയ TraceActivityVertex{&m_graph, TraceActivityVertex::ACTIVITY_ALWAYS}} {
        nodep->user2_Clear_വൃക്ഷം();  // TraceDecl multiple activities flag
        nodep->user3_Clear_വൃക്ഷം();  // TraceDecl TraceActivityVertex (assumes we start at nullptr)
        iterate(nodep);
        nodep->foreach([](AstTraceDecl* സ്ഥിരം declp) {
            എങ്കിൽ (declp->inDtypeFunc()) {
                declp->valuep()->unlinkFrBack()->delete_വൃക്ഷം();
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

ശൂന്യം V3Trace::traceAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { TraceVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_വൃക്ഷം("trace", 0, dump_വൃക്ഷം_Either_Level() >= 3);
}
