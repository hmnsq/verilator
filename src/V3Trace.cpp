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

#ଅନ୍ତର୍ଭାଵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ଅନ୍ତର୍ଭାଵ "V3Trace.h"

#ଅନ୍ତର୍ଭାଵ "V3Ast.h"
#ଅନ୍ତର୍ଭାଵ "V3DupFinder.h"
#ଅନ୍ତର୍ଭାଵ "V3EmitCBase.h"
#ଅନ୍ତର୍ଭାଵ "V3Graph.h"
#ଅନ୍ତର୍ଭାଵ "V3Stats.h"
#ଅନ୍ତର୍ଭାଵ "V3UniqueNames.h"

#ଅନ୍ତର୍ଭାଵ <limits>
#ଅନ୍ତର୍ଭାଵ <map>
#ଅନ୍ତର୍ଭାଵ <set>
#ଅନ୍ତର୍ଭାଵ <unordered_map>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Graph vertexes

ଶ୍ରେଣୀ TraceActivityVertex final : ସର୍ବସାଧାରଣ V3GraphVertex {
    VL_RTTI_IMPL(TraceActivityVertex, V3GraphVertex)
    Ast_ଗଣ୍ଠି* ଅଚଳ m_insertp;
    int32_t m_activityCode;
    ତାର୍କିକ m_slow;  // If always slow, we can use the same code
ସର୍ବସାଧାରଣ:
    ଗଣନା { ACTIVITY_NEVER = ((1UL << 31) - 1) };
    ଗଣନା { ACTIVITY_ALWAYS = ((1UL << 31) - 2) };
    ଗଣନା { ACTIVITY_SLOW = 0 };
    TraceActivityVertex(V3Graph* graphp, Ast_ଗଣ୍ଠି* nodep, ତାର୍କିକ slow)
        : V3GraphVertex{graphp}
        , m_insertp{nodep} {
        m_activityCode = 0;
        m_slow = slow;
    }
    TraceActivityVertex(V3Graph* graphp, int32_t code)
        : V3GraphVertex{graphp}
        , m_insertp{nullptr} {
        m_activityCode = code;
        m_slow = ମିଥ୍ୟା;
    }
    ~TraceActivityVertex() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    // ACCESSORS
    Ast_ଗଣ୍ଠି* insertp() ଅଚଳ {
        UASSERT(m_insertp, "Null insertp; probably called on a special always/slow");
        ଫେରନ୍ତୁ m_insertp;
    }
    string name() ଅଚଳ override {
        ଯଦି (activityAlways()) {
            ଫେରନ୍ତୁ "*ALWAYS*";
        } ଅନ୍ୟଥା {
            ଫେରନ୍ତୁ std::string{slow() ? "*SLOW* " : ""} + insertp()->name();
        }
    }
    string dotColor() ଅଚଳ override { ଫେରନ୍ତୁ slow() ? "yellowGreen" : "green"; }
    int32_t activityCode() ଅଚଳ { ଫେରନ୍ତୁ m_activityCode; }
    ତାର୍କିକ activityAlways() ଅଚଳ { ଫେରନ୍ତୁ activityCode() == ACTIVITY_ALWAYS; }
    ତାର୍କିକ activitySlow() ଅଚଳ { ଫେରନ୍ତୁ activityCode() == ACTIVITY_SLOW; }
    ଶୂନ୍ୟ activityCode(int32_t code) { m_activityCode = code; }
    ତାର୍କିକ slow() ଅଚଳ { ଫେରନ୍ତୁ m_slow; }
    ଶୂନ୍ୟ slow(ତାର୍କିକ flag) {
        ଯଦି (!flag) m_slow = ମିଥ୍ୟା;
    }
};

ଶ୍ରେଣୀ TraceCFuncVertex final : ସର୍ବସାଧାରଣ V3GraphVertex {
    VL_RTTI_IMPL(TraceCFuncVertex, V3GraphVertex)
    AstCFunc* ଅଚଳ m_nodep;

ସର୍ବସାଧାରଣ:
    TraceCFuncVertex(V3Graph* graphp, AstCFunc* nodep)
        : V3GraphVertex{graphp}
        , m_nodep{nodep} {}
    ~TraceCFuncVertex() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    // ACCESSORS
    AstCFunc* nodep() ଅଚଳ { ଫେରନ୍ତୁ m_nodep; }
    string name() ଅଚଳ override { ଫେରନ୍ତୁ nodep()->name(); }
    string dotColor() ଅଚଳ override { ଫେରନ୍ତୁ "yellow"; }
    File_ଧାଡ଼ି* fileline() ଅଚଳ override { ଫେରନ୍ତୁ nodep()->fileline(); }
};

ଶ୍ରେଣୀ TraceTraceVertex final : ସର୍ବସାଧାରଣ V3GraphVertex {
    VL_RTTI_IMPL(TraceTraceVertex, V3GraphVertex)
    AstTraceDecl* ଅଚଳ m_nodep;  // TRACEINC this represents
    // nullptr, or other vertex with the real code() that duplicates this one
    TraceTraceVertex* m_duplicatep = nullptr;
    // When aliasing to a dtype parent, offset of the member within the dtype
    uint32_t m_dtypeAliasOffset = 0;

ସର୍ବସାଧାରଣ:
    TraceTraceVertex(V3Graph* graphp, AstTraceDecl* nodep)
        : V3GraphVertex{graphp}
        , m_nodep{nodep} {}
    ~TraceTraceVertex() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    // ACCESSORS
    AstTraceDecl* nodep() ଅଚଳ { ଫେରନ୍ତୁ m_nodep; }
    string name() ଅଚଳ override { ଫେରନ୍ତୁ nodep()->name(); }
    string dotColor() ଅଚଳ override { ଫେରନ୍ତୁ "red"; }
    File_ଧାଡ଼ି* fileline() ଅଚଳ override { ଫେରନ୍ତୁ nodep()->fileline(); }
    TraceTraceVertex* duplicatep() ଅଚଳ { ଫେରନ୍ତୁ m_duplicatep; }
    ଶୂନ୍ୟ duplicatep(TraceTraceVertex* dupp) {
        UASSERT_OBJ(!duplicatep(), nodep(), "Assigning duplicatep() to already duplicated node");
        m_duplicatep = dupp;
    }
    ଶୂନ୍ୟ redirectDuplicatep(TraceTraceVertex* dupp) { m_duplicatep = dupp; }
    uint32_t dtypeAliasOffset() ଅଚଳ { ଫେରନ୍ତୁ m_dtypeAliasOffset; }
    ଶୂନ୍ୟ dtypeAliasOffset(uint32_t offset) { m_dtypeAliasOffset = offset; }
};

ଶ୍ରେଣୀ TraceVarVertex final : ସର୍ବସାଧାରଣ V3GraphVertex {
    VL_RTTI_IMPL(TraceVarVertex, V3GraphVertex)
    AstVarScope* ଅଚଳ m_nodep;

ସର୍ବସାଧାରଣ:
    TraceVarVertex(V3Graph* graphp, AstVarScope* nodep)
        : V3GraphVertex{graphp}
        , m_nodep{nodep} {}
    ~TraceVarVertex() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    // ACCESSORS
    AstVarScope* nodep() ଅଚଳ { ଫେରନ୍ତୁ m_nodep; }
    string name() ଅଚଳ override { ଫେରନ୍ତୁ nodep()->name(); }
    string dotColor() ଅଚଳ override { ଫେରନ୍ତୁ "skyblue"; }
    File_ଧାଡ଼ି* fileline() ଅଚଳ override { ଫେରନ୍ତୁ nodep()->fileline(); }
};

//######################################################################
// Trace state, as a visitor of each AstNode

ଶ୍ରେଣୀ TraceVisitor final : ସର୍ବସାଧାରଣ VNVisitor {
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
    ଅଚଳ VNUser1InUse m_inuser1;
    ଅଚଳ VNUser2InUse m_inuser2;
    ଅଚଳ VNUser3InUse m_inuser3;
    // VNUser4InUse     In V3Hasher via V3DupFinder

    // STATE
    Ast_ଗଣ୍ଠି_Module* m_ଶୀର୍ଷ_Modp = nullptr;  // Module to add variables to
    AstScope* ଅଚଳ m_ଶୀର୍ଷ_Scopep = v3Global.rootp()->ଶୀର୍ଷ_Scopep()->scopep();  // The top AstScope
    AstCFunc* m_cfuncp = nullptr;  // C function adding to graph
    AstCFunc* m_regFuncp = nullptr;  // Trace registration function
    AstCFunc* m_actAllFuncp = nullptr;  // Set all activity function
    AstTraceDecl* m_tracep = nullptr;  // Trace function adding to graph
    AstVarScope* m_activityVscp = nullptr;  // Activity variable
    uint32_t m_activity_ସଂଖ୍ୟା = 0;  // Count of fields in activity variable
    uint32_t m_code = 0;  // Trace ident code# being assigned
    V3Graph m_graph;  // Var/CFunc tracking
    TraceActivityVertex* ଅଚଳ m_alwaysVtxp;  // "Always trace" vertex
    ତାର୍କିକ m_finding = ମିଥ୍ୟା;  // Pass one of algorithm?
    ସଂରଚନା DtypeFuncs final {
    ସର୍ବସାଧାରଣ:
        AstCFunc* fullFuncp = nullptr;
        AstCFunc* chgFuncp = nullptr;
    };
    std::unordered_map<ଅଚଳ Ast_ଗଣ୍ଠି_DType*, DtypeFuncs>
        m_dtypeNonConstFuncs;  // Full / Chg funcs per type
    std::unordered_map<ଅଚଳ Ast_ଗଣ୍ଠି_DType*, AstCFunc*> m_dtypeConstFuncs;  // Const func per type
    V3UniqueNames m_dtypeNames{""};  // Unique type func names

    // Trace parallelism. Only VCD tracing can be parallelized at this time.
    ଅଚଳ uint32_t m_parallelism
        = v3Global.opt.useTraceParallel() ? static_cast<uint32_t>(v3Global.opt.threads()) : 1;

    VDouble0 m_statSetters;  // Statistic tracking
    VDouble0 m_statSettersSlow;  // Statistic tracking
    VDouble0 m_statUniqSigs;  // Statistic tracking

    // All activity numbers applying to a given trace
    using ActCodeSet = std::set<uint32_t>;
    // For activity set, what traces apply
    using TraceVec = std::multimap<ActCodeSet, TraceTraceVertex*>;
    // Candidate interface-member VarScopes keyed by (interface type, member name)
    std::map<std::pair<ଅଚଳ AstIface*, std::string>, std::vector<AstVarScope*>>
        m_ifaceMemberVscps;

    ଶ୍ରେଣୀ TraceInitDeclCollector final : ସର୍ବସାଧାରଣ VNVisitor {
        std::vector<AstTraceDecl*>& m_declps;  // Output: trace declarations found so far
        std::set<ଅଚଳ AstCFunc*> m_seenFuncps;

        ଶୂନ୍ୟ visit(AstTraceDecl* nodep) override { m_declps.ଠେଲ_back(nodep); }
        ଶୂନ୍ୟ visit(AstCCall* nodep) override {
            ଯଦି (AstCFunc* ଅଚଳ funcp = nodep->funcp()) collect(funcp);
        }
        ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override { iterateChildren(nodep); }

    ସର୍ବସାଧାରଣ:
        explicit TraceInitDeclCollector(std::vector<AstTraceDecl*>& declps)
            : m_declps{declps} {}
        ଶୂନ୍ୟ collect(AstCFunc* funcp) {
            ଯଦି (funcp && m_seenFuncps.ଭର୍ତ୍ତି(funcp).second) iterate(funcp);
        }
    };

    // METHODS

    ସ୍ଥିର ତାର୍କିକ same_ମୂଳ_Init_Alias(ଅଚଳ AstTraceDecl* ମୂଳ_Declp, ଅଚଳ AstTraceDecl* ଶୀର୍ଷ_Declp) {
        ଅଚଳ VNumRange& ମୂଳ_Bit_Range = ମୂଳ_Declp->bitRange();
        ଅଚଳ VNumRange& ଶୀର୍ଷ_Bit_Range = ଶୀର୍ଷ_Declp->bitRange();
        ଫେରନ୍ତୁ ମୂଳ_Declp->showname() == ଶୀର୍ଷ_Declp->showname()
               && ମୂଳ_Declp->declDirection() == ଶୀର୍ଷ_Declp->declDirection()
               && ମୂଳ_Declp->widthMin() == ଶୀର୍ଷ_Declp->widthMin()
               && ମୂଳ_Bit_Range.ranged() == ଶୀର୍ଷ_Bit_Range.ranged()
               && (!ମୂଳ_Bit_Range.ranged()
                   || (ମୂଳ_Bit_Range.ବାମ() == ଶୀର୍ଷ_Bit_Range.ବାମ()
                       && ମୂଳ_Bit_Range.ଡାହାଣ() == ଶୀର୍ଷ_Bit_Range.ଡାହାଣ()));
    }

    ଶୂନ୍ୟ detectDuplicates() {
        UINFO(9, "Finding duplicates");
        // Note uses user4
        V3DupFinder dupFinder;  // Duplicate code detection

        // Compute member offsets within dtype instances incrementally.
        // For each dtype member TraceDecl, stores its code offset within the
        // dtype parent's code range.
        std::unordered_map<ଅଚଳ AstTraceDecl*, uint32_t> memberOffsets;
        std::unordered_map<ଅଚଳ AstTraceDecl*, uint32_t> runningOffset;

        // For fixup pass: track dups redirected to dtype parents, so we can
        // fix them if the parent's user2 is later set during the loop.
        // Maps dup vertex -> original canonical member vertex.
        std::vector<std::pair<TraceTraceVertex*, TraceTraceVertex*>> dtypeParentRedirects;

        // Hash all of the traced values and find if there are any duplicates
        ପାଇଁ (V3GraphVertex& vtx : m_graph.vertices()) {
            ଯଦି (TraceTraceVertex* ଅଚଳ vvertexp = vtx.cast<TraceTraceVertex>()) {
                AstTraceDecl* ଅଚଳ nodep = vvertexp->nodep();
                ଯଦି (nodep->dtypeCallp()) ଜାରି;

                ଯଦି (nodep->dtypeDeclp()) {
                    uint32_t& offset = runningOffset[nodep->dtypeDeclp()];
                    memberOffsets[nodep] = offset;
                    offset += nodep->codeInc();
                }

                UASSERT_OBJ(!vvertexp->duplicatep(), nodep, "Should not be a duplicate");
                ଅଚଳ auto dupit = dupFinder.findDuplicate(nodep);
                ଯଦି (dupit == dupFinder.end()) {
                    dupFinder.ଭର୍ତ୍ତି(nodep);
                } ଅନ୍ୟଥା {
                    AstTraceDecl* ଅଚଳ dupDeclp = VN_AS(dupit->second, TraceDecl);
                    UASSERT_OBJ(dupDeclp, nodep, "Trace duplicate of wrong type");
                    TraceTraceVertex* ଅଚଳ dupvertexp
                        = dupDeclp->user1u().toGraphVertex()->cast<TraceTraceVertex>();
                    UINFO(8, "  Orig " << dupDeclp << endl);
                    UINFO(8, "   dup " << nodep << endl);

                    ଯଦି (dupDeclp->dtypeDeclp() && !dupDeclp->dtypeDeclp()->user2()) {
                        AstTraceDecl* ଅଚଳ dtypeParentp = dupDeclp->dtypeDeclp();
                        TraceTraceVertex* ଅଚଳ dtypeVtxp
                            = dtypeParentp->user1u().toGraphVertex()->cast<TraceTraceVertex>();
                        ଅଚଳ auto it2 = memberOffsets.find(dupDeclp);
                        UASSERT_OBJ(it2 != memberOffsets.end(), dupDeclp,
                                    "Member offset not precomputed");
                        vvertexp->duplicatep(dtypeVtxp);
                        vvertexp->dtypeAliasOffset(it2->second);
                        dtypeParentRedirects.emplace_back(vvertexp, dupvertexp);
                    } ଅନ୍ୟଥା {
                        vvertexp->duplicatep(dupvertexp);
                    }
                    ଯଦି (nodep->dtypeDeclp()) nodep->dtypeDeclp()->user2(ସତ୍ୟ);
                }
            }
        }

        ପାଇଁ (ଅଚଳ auto& pair : dtypeParentRedirects) {
            TraceTraceVertex* ଅଚଳ dupVtxp = pair.first;
            TraceTraceVertex* ଅଚଳ memberVtxp = pair.second;
            ଅଚଳ AstTraceDecl* ଅଚଳ parentDeclp = dupVtxp->duplicatep()->nodep();
            ଯଦି (parentDeclp->user2()) {
                dupVtxp->redirectDuplicatep(memberVtxp);
                dupVtxp->dtypeAliasOffset(0);
            }
        }

        ଯଦି (dumpLevel() || debug() >= 9)
            dupFinder.dumpFile(v3Global.debugFilename("trace") + ".hash", ମିଥ୍ୟା);
    }

    ଶୂନ୍ୟ graphDtypePrune() {
        ପାଇଁ (V3GraphVertex* ଅଚଳ vtxp : m_graph.vertices().unlinkable()) {
            ଯଦି (TraceTraceVertex* ଅଚଳ vvertexp = vtxp->cast<TraceTraceVertex>()) {
                AstTraceDecl* ଅଚଳ declp = vvertexp->nodep();
                // This skips the dtype sub-func optimization if a var is affected by multiple
                // activities.  We really only need to do this for _chg funcs (and not decls,
                // _const and _full) but it's simpiler to do it all one way or the other.
                ଯଦି (declp) {
                    ଯଦି (declp->user2() || (declp->dtypeDeclp() && !declp->dtypeDeclp()->user2())) {
                        AstCCall* ଅଚଳ callp = declp->dtypeCallp();
                        ଯଦି (callp) {
                            Ast_ଗଣ୍ଠି* stmtexprp = callp->backp();
                            VL_DO_DANGLING(ଠେଲ_Deletep(stmtexprp->unlinkFrBack()), stmtexprp);
                        } ଅନ୍ୟଥା {
                            ତାର୍କିକ emptyScope;
                            କର {
                                emptyScope = ମିଥ୍ୟା;
                                Ast_ଗଣ୍ଠି* ଅଚଳ declBackp = declp->backp();
                                Ast_ଗଣ୍ଠି* ଅଚଳ declNextp = declp->nextp();
                                ଯଦି (VN_IS(declBackp, Trace_ଠେଲ_Prefix)
                                    && VN_IS(declNextp, Trace_କାଢ଼_Prefix)) {
                                    VL_DO_DANGLING(ଠେଲ_Deletep(declBackp->unlinkFrBack()),
                                                   declBackp);
                                    VL_DO_DANGLING(ଠେଲ_Deletep(declNextp->unlinkFrBack()),
                                                   declNextp);
                                    emptyScope = ସତ୍ୟ;
                                }
                            } ଯେତେବେଳେ (emptyScope);
                        }
                        // Can't purge until we finish this pass
                        ଠେଲ_Deletep(declp->unlinkFrBack());  // declp used below
                        vvertexp->rerouteEdges(&m_graph);
                        vvertexp->unlinkDelete(&m_graph);
                    }
                    declp->dtypeDeclp(nullptr);
                }
            }
        }
    }

    ଶୂନ୍ୟ splitTraceDeclFuncs() {
        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା splitLimit = v3Global.opt.outputSplitCTrace() ? v3Global.opt.outputSplitCTrace()
                                                                : std::numeric_limits<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>::max();
        ଯଦି (splitLimit == std::numeric_limits<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>::max()) ଫେରନ୍ତୁ;

        ପାଇଁ (Ast_ଗଣ୍ଠି* nodep = m_ଶୀର୍ଷ_Scopep->blocksp(); nodep; nodep = nodep->nextp()) {
            AstCFunc* ଅଚଳ funcp = VN_CAST(nodep, CFunc);
            ଯଦି (!funcp || !funcp->user2()) ଜାରି;

            ଅଚଳ string baseName = funcp->name();
            uint32_t sub_ଅଙ୍କ = 1;
            ପୂର୍ଣ୍ଣସଂଖ୍ୟା curSize = 0;
            AstCFunc* curFuncp = funcp;
            std::vector<Ast_ଗଣ୍ଠି_Stmt*> callStmts;

            auto startNewSubFunc = [&]() {
                File_ଧାଡ଼ି* ଅଚଳ flp = funcp->fileline();
                ଅଚଳ string newName = baseName + "_" + cvtToStr(sub_ଅଙ୍କ++);
                AstCFunc* ଅଚଳ newFuncp = ନୂତନ AstCFunc{flp, newName, m_ଶୀର୍ଷ_Scopep};
                newFuncp->argTypes(v3Global.opt.traceClassBase() + "* tracep");
                newFuncp->isTrace(ସତ୍ୟ);
                newFuncp->isStatic(ମିଥ୍ୟା);
                newFuncp->isLoose(ସତ୍ୟ);
                newFuncp->slow(ସତ୍ୟ);
                newFuncp->dontCombine(ସତ୍ୟ);
                m_ଶୀର୍ଷ_Scopep->addBlocksp(newFuncp);
                newFuncp->addStmtsp(ନୂତନ AstCStmt{flp, "const int c = vlSymsp->__Vm_baseCode;"});
                AstCCall* ଅଚଳ callp = ନୂତନ AstCCall{flp, newFuncp};
                callp->dtypeSetVoid();
                callp->argTypes("tracep");
                callStmts.ଠେଲ_back(callp->makeStmt());
                curFuncp = newFuncp;
                curSize = 0;
            };

            ପାଇଁ (Ast_ଗଣ୍ଠି *stmtp = funcp->stmtsp(), *nextp; stmtp; stmtp = nextp) {
                nextp = stmtp->nextp();
                curSize += stmtp->ଗଣ୍ଠି_ଗଣନା();

                ଯଦି (curFuncp != funcp) {
                    stmtp->unlinkFrBack();
                    curFuncp->addStmtsp(stmtp);
                }

                ଯଦି (curSize > splitLimit && nextp) startNewSubFunc();
            }

            ପାଇଁ (Ast_ଗଣ୍ଠି_Stmt* ଅଚଳ callStmtp : callStmts) { funcp->addStmtsp(callStmtp); }
        }
    }

    ଶୂନ୍ୟ graphSimplify(ତାର୍କିକ initial) {
        ଯଦି (initial) {
            // Remove all variable nodes
            ପାଇଁ (V3GraphVertex* ଅଚଳ vtxp : m_graph.vertices().unlinkable()) {
                ଯଦି (TraceVarVertex* ଅଚଳ vvertexp = vtxp->cast<TraceVarVertex>()) {
                    vvertexp->rerouteEdges(&m_graph);
                    vvertexp->unlinkDelete(&m_graph);
                }
            }
            // Remove multiple variables connecting funcs to traces
            // We do this twice, as then we have fewer edges to multiply out in the below
            // expansion.
            m_graph.removeRedundantEdgesMax(&V3GraphEdge::followAlwaysTrue);
            // Remove all Cfunc nodes
            ପାଇଁ (V3GraphVertex* ଅଚଳ vtxp : m_graph.vertices().unlinkable()) {
                ଯଦି (TraceCFuncVertex* ଅଚଳ vvertexp = vtxp->cast<TraceCFuncVertex>()) {
                    vvertexp->rerouteEdges(&m_graph);
                    vvertexp->unlinkDelete(&m_graph);
                }
            }
        }

        // Remove multiple variables connecting funcs to traces
        m_graph.removeRedundantEdgesMax(&V3GraphEdge::followAlwaysTrue);

        // If there are any edges from a always, keep only the always
        ପାଇଁ (V3GraphVertex& vtx : m_graph.vertices()) {
            ଯଦି (TraceTraceVertex* ଅଚଳ vvertexp = vtx.cast<TraceTraceVertex>()) {
                // Search for the incoming always edge
                ଅଚଳ V3GraphEdge* alwaysEdgep = nullptr;
                ପାଇଁ (ଅଚଳ V3GraphEdge& edge : vvertexp->inEdges()) {
                    ଅଚଳ TraceActivityVertex* ଅଚଳ actVtxp
                        = edge.fromp()->as<ଅଚଳ TraceActivityVertex>();
                    ଯଦି (actVtxp->activityAlways()) {
                        alwaysEdgep = &edge;
                        ଭାଙ୍ଗ;
                    }
                }
                // If always edge exists, remove all other edges
                ଯଦି (alwaysEdgep) {
                    ପାଇଁ (V3GraphEdge* ଅଚଳ edgep : vvertexp->inEdges().unlinkable()) {
                        ଯଦି (edgep != alwaysEdgep) VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
                    }
                }
            }
        }

        // Activity points with no outputs can be removed
        ପାଇଁ (V3GraphVertex* ଅଚଳ vtxp : m_graph.vertices().unlinkable()) {
            ଯଦି (TraceActivityVertex* ଅଚଳ aVtxp = vtxp->cast<TraceActivityVertex>()) {
                // Leave in the always vertex for later use.
                ଯଦି (aVtxp != m_alwaysVtxp && aVtxp->outEmpty()) {
                    VL_DO_DANGLING(aVtxp->unlinkDelete(&m_graph), aVtxp);
                }
            }
        }
    }

    uint32_t assignactivityNumbers() {
        uint32_t activity_ସଂଖ୍ୟା = 1;  // Note 0 indicates "slow" only
        ପାଇଁ (V3GraphVertex& vtx : m_graph.vertices()) {
            ଯଦି (TraceActivityVertex* ଅଚଳ vvertexp = vtx.cast<TraceActivityVertex>()) {
                ଯଦି (vvertexp != m_alwaysVtxp) {
                    ଯଦି (vvertexp->slow()) {
                        vvertexp->activityCode(TraceActivityVertex::ACTIVITY_SLOW);
                    } ଅନ୍ୟଥା {
                        vvertexp->activityCode(activity_ସଂଖ୍ୟା++);
                    }
                }
            }
        }
        ଫେରନ୍ତୁ activity_ସଂଖ୍ୟା;
    }

    ଶୂନ୍ୟ sortTraces(TraceVec& traces, uint32_t& nNonConstCodes) {
        // Populate sort structure
        traces.clear();
        nNonConstCodes = 0;
        ପାଇଁ (V3GraphVertex& vtx : m_graph.vertices()) {
            ଯଦି (TraceTraceVertex* ଅଚଳ vtxp = vtx.cast<TraceTraceVertex>()) {
                ActCodeSet actSet;
                UINFO(9, "  Add to sort: " << vtxp);
                UINFOTREE(9, vtxp->nodep(), "", "trnode");
                ପାଇଁ (ଅଚଳ V3GraphEdge& edge : vtxp->inEdges()) {
                    ଅଚଳ TraceActivityVertex* ଅଚଳ cfvertexp
                        = edge.fromp()->cast<ଅଚଳ TraceActivityVertex>();
                    UASSERT_OBJ(cfvertexp, vtxp->nodep(),
                                "Should have been function pointing to this trace");
                    UINFO(9, "   Activity: " << cfvertexp);
                    ଯଦି (cfvertexp->activityAlways()) {
                        // If code 0, we always trace; ignore other codes
                        actSet.ଭର୍ତ୍ତି(TraceActivityVertex::ACTIVITY_ALWAYS);
                    } ଅନ୍ୟଥା {
                        actSet.ଭର୍ତ୍ତି(cfvertexp->activityCode());
                    }
                }
                UASSERT_OBJ(actSet.ଗଣନା_2(TraceActivityVertex::ACTIVITY_ALWAYS) == 0
                                || actSet.size() == 1,
                            vtxp->nodep(), "Always active trace has further triggers");
                // Count nodes
                ଯଦି (!vtxp->duplicatep() && !actSet.empty())
                    nNonConstCodes += vtxp->nodep()->codeInc();
                ଯଦି (actSet.empty()) {
                    // If a trace doesn't have activity, it's constant, and we
                    // don't need to track changes on it.
                    actSet.ଭର୍ତ୍ତି(TraceActivityVertex::ACTIVITY_NEVER);
                } ଅନ୍ୟଥା ଯଦି (actSet.ଗଣନା_2(TraceActivityVertex::ACTIVITY_SLOW) && actSet.size() > 1) {
                    // If a trace depends on the slow flag as well as other
                    // flags, remove the dependency on the slow flag. We will
                    // make slow routines set all activity flags.
                    actSet.erase(TraceActivityVertex::ACTIVITY_SLOW);
                }
                traces.emplace(actSet, vtxp);
            }
        }
    }

    ଶୂନ୍ୟ graphOptimize() {
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
        ଅଚଳ auto end = traces.end();
        ଯେତେବେଳେ (it != end) {
            auto head = it;
            // Approximate the complexity of the value change check
            uint32_t complexity = 0;
            ଅଚଳ ActCodeSet& actSet = it->first;
            ପାଇଁ (; it != end && it->first == actSet; ++it) {
                ଯଦି (!it->second->duplicatep()) {
                    uint32_t cost = 0;
                    ଅଚଳ AstTraceDecl* ଅଚଳ declp = it->second->nodep();
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
            ଯଦି (actSet.ଗଣନା_2(TraceActivityVertex::ACTIVITY_ALWAYS)) ଜାରି;
            ଯଦି (actSet.ଗଣନା_2(TraceActivityVertex::ACTIVITY_NEVER)) ଜାରି;
            ଯଦି (actSet.ଗଣନା_2(TraceActivityVertex::ACTIVITY_SLOW)) ଜାରି;
            // If the value comparisons are cheaper to perform than checking the
            // activity flags make the signals always traced. Note this cost
            // equation is heuristic.
            ଯଦି (complexity <= actSet.size() * 2) {
                ପାଇଁ (; head != it; ++head) {
                    ନୂତନ V3GraphEdge{&m_graph, m_alwaysVtxp, head->second, 1};
                }
            }
        }

        graphSimplify(ମିଥ୍ୟା);
    }

    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* selectActivity(File_ଧାଡ଼ି* flp, uint32_t acode, ଅଚଳ VAccess& access) {
        ଫେରନ୍ତୁ ନୂତନ AstArraySel(flp, ନୂତନ AstVarRef{flp, m_activityVscp, access}, acode);
    }

    Ast_ଗଣ୍ଠି* newActivitySetter(Ast_ଗଣ୍ଠି* insertp, uint32_t code) {
        ++m_statSetters;
        File_ଧାଡ଼ି* ଅଚଳ fl = insertp->fileline();
        AstAssign* ଅଚଳ setterp = ନୂତନ AstAssign{fl, selectActivity(fl, code, VAccess::WRITE),
                                                 ନୂତନ AstConst{fl, AstConst::BitTrue{}}};
        ଫେରନ୍ତୁ setterp;
    }

    Ast_ଗଣ୍ଠି* newActivityAll(Ast_ଗଣ୍ଠି* insertp) {
        ++m_statSettersSlow;
        ଯଦି (!m_actAllFuncp) {
            File_ଧାଡ଼ି* ଅଚଳ flp = m_ଶୀର୍ଷ_Scopep->fileline();
            AstCFunc* ଅଚଳ funcp = ନୂତନ AstCFunc{flp, "__VtraceActivitySetAll", m_ଶୀର୍ଷ_Scopep};
            funcp->slow(ସତ୍ୟ);
            funcp->isStatic(ମିଥ୍ୟା);
            funcp->isLoose(ସତ୍ୟ);
            m_ଶୀର୍ଷ_Scopep->addBlocksp(funcp);
            ପାଇଁ (uint32_t code = 0; code < m_activity_ସଂଖ୍ୟା; ++code) {
                Ast_ଗଣ୍ଠି* ଅଚଳ setterp = newActivitySetter(insertp, code);
                funcp->addStmtsp(setterp);
            }
            m_actAllFuncp = funcp;
        }
        AstCCall* ଅଚଳ callp = ନୂତନ AstCCall{insertp->fileline(), m_actAllFuncp};
        callp->dtypeSetVoid();
        ଫେରନ୍ତୁ callp->makeStmt();
    }

    ଶୂନ୍ୟ createActivityFlags() {
        // Assign final activity numbers
        m_activity_ସଂଖ୍ୟା = assignactivityNumbers();

        // Create an array of bytes, not a bit vector, as they can be set
        // atomically by mtasks, and are cheaper to set (no need for
        // read-modify-write on the C type), and the speed of the tracing code
        // is the same on largish designs.
        File_ଧାଡ଼ି* ଅଚଳ flp = m_ଶୀର୍ଷ_Scopep->fileline();
        Ast_ଗଣ୍ଠି_DType* ଅଚଳ newScalarDtp = ନୂତନ AstBasicDType{flp, VFlagBitPacked{}, 1};
        v3Global.rootp()->typeTablep()->addTypesp(newScalarDtp);
        AstRange* ଅଚଳ newArange
            = ନୂତନ AstRange{flp, VNumRange{static_cast<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>(m_activity_ସଂଖ୍ୟା) - 1, 0}};
        Ast_ଗଣ୍ଠି_DType* ଅଚଳ newArrDtp = ନୂତନ AstUnpackArrayDType{flp, newScalarDtp, newArange};
        v3Global.rootp()->typeTablep()->addTypesp(newArrDtp);
        m_activityVscp = v3Global.rootp()->ଶୀର୍ଷ_Scopep()->createTemp("__VtraceActivity", newArrDtp);

        // Insert activity setters
        ପାଇଁ (ଅଚଳ V3GraphVertex& vtx : m_graph.vertices()) {
            ଯଦି (ଅଚଳ TraceActivityVertex* ଅଚଳ vtxp = vtx.cast<ଅଚଳ TraceActivityVertex>()) {
                Ast_ଗଣ୍ଠି* setterp = nullptr;
                ଯଦି (vtxp->activitySlow()) {
                    setterp = newActivityAll(vtxp->insertp());
                    // Just set all flags in slow code as it should be rare.
                    // This will be rolled up into a loop by V3Reloop.
                } ଅନ୍ୟଥା ଯଦି (!vtxp->activityAlways()) {
                    setterp = newActivitySetter(vtxp->insertp(), vtxp->activityCode());
                }
                ଯଦି (setterp) {
                    Ast_ଗଣ୍ଠି* ଅଚଳ insertp = vtxp->insertp();
                    ଯଦି (Ast_Stmt_ପଦସମଷ୍ଟି* ଅଚଳ stmtp = VN_CAST(insertp, Stmt_ପଦସମଷ୍ଟି)) {
                        stmtp->addNextHere(setterp);
                    } ଅନ୍ୟଥା ଯଦି (AstCFunc* ଅଚଳ funcp = VN_CAST(insertp, CFunc)) {
                        // If there are awaits, insert the setter after each await
                        ଯଦି (funcp->isCoroutine() && funcp->stmtsp()) {
                            funcp->stmtsp()->foreachAndNext([setterp](AstCAwait* awaitp) {
                                awaitp->addNextHere(setterp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା));
                            });
                        }
                        funcp->addStmtsp(setterp);
                    } ଅନ୍ୟଥା {
                        insertp->v3fatalSrc("Bad trace activity vertex");
                    }
                }
            }
        }
    }

    AstCFunc* newCFunc(VTraceType traceType, AstCFunc* ଶୀର୍ଷ_Funcp, uint32_t func_ଅଙ୍କ,
                       uint32_t baseCode = 0, ଅଚଳ AstTraceDecl* ଅଚଳ declp = nullptr,
                       ତାର୍କିକ declSub = ମିଥ୍ୟା) {
        // Create new function
        ଅଚଳ ତାର୍କିକ is_ଶୀର୍ଷ_Func = !declp && ଶୀର୍ଷ_Funcp == nullptr;
        std::string funcName;
        ଯଦି (is_ଶୀର୍ଷ_Func || declp) {
            funcName = traceType.func_prefix();
        } ଅନ୍ୟଥା {
            funcName = ଶୀର୍ଷ_Funcp->name();
            funcName += "_sub";
        }
        ଯଦି (declp) {
            funcName += "_dtype";
            ଯଦି (declSub) funcName += "_sub";
            funcName += "__";
            funcName = m_dtypeNames.get(funcName);
        } ଅନ୍ୟଥା {
            funcName += "_";
            funcName += cvtToStr(func_ଅଙ୍କ);
        }

        File_ଧାଡ଼ି* ଅଚଳ flp = m_ଶୀର୍ଷ_Scopep->fileline();
        AstCFunc* ଅଚଳ funcp = ନୂତନ AstCFunc{flp, funcName, m_ଶୀର୍ଷ_Scopep};
        funcp->isTrace(ସତ୍ୟ);
        funcp->dontCombine(ସତ୍ୟ);
        funcp->isLoose(ସତ୍ୟ);
        funcp->slow(traceType != VTraceType::CHANGE);
        funcp->isStatic(is_ଶୀର୍ଷ_Func);
        // Add it to top scope
        m_ଶୀର୍ଷ_Scopep->addBlocksp(funcp);
        ଅଚଳ std::string bufArg
            = v3Global.opt.traceClassBase() + "::Buffer* bufp"
              + (declp ? (", uint32_t offset, const " + declp->dtypep()->cType("", ସତ୍ୟ, ସତ୍ୟ)
                          + " __VdtypeVar")
                       : "");
        ଯଦି (is_ଶୀର୍ଷ_Func) {
            // Top functions
            funcp->argTypes("void* voidSelf, " + bufArg);
            funcp->addStmtsp(ନୂତନ AstCStmt{flp, EmitCUtil::voidSelfAssign(m_ଶୀର୍ଷ_Modp)});
            funcp->addStmtsp(ନୂତନ AstCStmt{flp, EmitCUtil::symClassAssign()});
            // Add global activity check to change dump functions
            ଯଦି (traceType == VTraceType::CHANGE) {  //
                funcp->addStmtsp(
                    ନୂତନ AstCStmt{flp, "if (VL_UNLIKELY(!vlSymsp->__Vm_activity)) return;"});
            }
            // Register function
            AstCStmt* ଅଚଳ cstmtp = ନୂତନ AstCStmt{flp};
            m_regFuncp->addStmtsp(cstmtp);
            ଯଦି (traceType == VTraceType::CONSTANT) {
                cstmtp->add("tracep->addConstCb(");
                cstmtp->add(ନୂତନ AstAddrOfCFunc{flp, funcp});
                cstmtp->add(", " + std::to_string(func_ଅଙ୍କ) + ", vlSelf);");
            } ଅନ୍ୟଥା ଯଦି (traceType == VTraceType::FULL) {
                cstmtp->add("tracep->addFullCb(");
                cstmtp->add(ନୂତନ AstAddrOfCFunc{flp, funcp});
                cstmtp->add(", " + std::to_string(func_ଅଙ୍କ) + ", vlSelf);");
            } ଅନ୍ୟଥା {
                cstmtp->add("tracep->addChgCb(");
                cstmtp->add(ନୂତନ AstAddrOfCFunc{flp, funcp});
                cstmtp->add(", " + std::to_string(func_ଅଙ୍କ) + ", vlSelf);");
            }
        } ଅନ୍ୟଥା {
            // Sub functions
            funcp->argTypes(bufArg);
            // Setup base references. Note in rare occasions we can end up with an empty trace
            // sub function, hence the VL_ATTR_UNUSED attributes.
            ଯଦି (traceType != VTraceType::CHANGE) {
                // Full dump sub function
                funcp->addStmtsp(ନୂତନ AstCStmt{flp,  //
                                              string("uint32_t* const oldp VL_ATTR_UNUSED = "
                                                     "bufp->oldp(vlSymsp->__Vm_baseCode")
                                                  + (declp ? " + offset" : "") + ");\n"});
            } ଅନ୍ୟଥା {
                // Change dump sub function
                funcp->addStmtsp(ନୂତନ AstCStmt{flp,  //
                                              "uint32_t* const oldp VL_ATTR_UNUSED = "
                                              "bufp->oldp(vlSymsp->__Vm_baseCode + "
                                                  + (declp ? " offset" : cvtToStr(baseCode))
                                                  + ");\n"});
            }
            ଯଦି (!declp) {
                // Add call to top function
                AstCCall* ଅଚଳ callp = ନୂତନ AstCCall{funcp->fileline(), funcp};
                callp->dtypeSetVoid();
                callp->argTypes("bufp");
                ଶୀର୍ଷ_Funcp->addStmtsp(callp->makeStmt());
            }
        }
        // Done
        UINFO(5, "  newCFunc " << funcp);
        ଫେରନ୍ତୁ funcp;
    }

    AstCFunc* newCDtypeSubFunc(VTraceType traceType, ଅଚଳ AstTraceDecl* ଅଚଳ declp,
                               AstCFunc* parentp) {
        AstCFunc* ଅଚଳ funcp = newCFunc(traceType, nullptr, 0, 0, declp, ସତ୍ୟ);
        File_ଧାଡ଼ି* ଅଚଳ flp = m_ଶୀର୍ଷ_Scopep->fileline();
        AstCCall* ଅଚଳ callp = ନୂତନ AstCCall{flp, funcp};
        callp->dtypeSetVoid();
        callp->argTypes("bufp, offset, __VdtypeVar");
        parentp->addStmtsp(callp->makeStmt());
        ଫେରନ୍ତୁ funcp;
    }

    AstCFunc* createConstDtypeTraceFunctions(ଅଚଳ AstTraceDecl* declp) {
        ଅଚଳ Ast_ଗଣ୍ଠି_DType* ଅଚଳ dtypep = declp->dtypep()->skipRefp();
        auto pair = m_dtypeConstFuncs.emplace(dtypep, nullptr);
        ଯଦି (pair.second) {
            File_ଧାଡ଼ି* ଅଚଳ flp = declp->fileline();
            AstCFunc* ଅଚଳ funcp = newCFunc(VTraceType::CONSTANT, nullptr, 0, 0, declp);

            ତାର୍କିକ first = ସତ୍ୟ;
            ପାଇଁ (Ast_ଗଣ୍ଠି* callStmtp = declp->dtypeCallp()->funcp()->stmtsp(); callStmtp;
                 callStmtp = callStmtp->nextp()) {
                ଅଚଳ AstCCall* ଅଚଳ callp
                    = VN_CAST(VN_CAST(callStmtp, Stmt_ପଦସମଷ୍ଟି)->exprp(), CCall);
                ତାର୍କିକ onlyOne = first && !callStmtp->nextp();
                AstCFunc* ଅଚଳ subFuncp
                    = onlyOne ? funcp : newCDtypeSubFunc(VTraceType::CONSTANT, declp, funcp);
                ପାଇଁ (Ast_ଗଣ୍ଠି* stmtp = callp->funcp()->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    ଯଦି (AstTraceDecl* ଅଚଳ fieldDeclp = VN_CAST(stmtp, TraceDecl)) {
                        AstTraceInc* ଅଚଳ incp
                            = ନୂତନ AstTraceInc{flp, fieldDeclp, VTraceType::CONSTANT};
                        subFuncp->addStmtsp(incp);
                    }
                }
                first = ମିଥ୍ୟା;
            }

            pair.first->second = funcp;
        }

        ଫେରନ୍ତୁ pair.first->second;
    }

    ଶୂନ୍ୟ createConstTraceFunctions(ଅଚଳ TraceVec& traces) {
        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା splitLimit = v3Global.opt.outputSplitCTrace() ? v3Global.opt.outputSplitCTrace()
                                                                : std::numeric_limits<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>::max();

        AstCFunc* ଅଚଳ ଶୀର୍ଷ_Funcp = newCFunc(VTraceType::CONSTANT, nullptr, 0);
        uint32_t sub_Func_ଅଙ୍କ = 0;
        AstCFunc* subFuncp = nullptr;
        ପୂର୍ଣ୍ଣସଂଖ୍ୟା subStmts = 0;
        std::vector<ଅଚଳ TraceTraceVertex*> duplicates;
        ପାଇଁ (auto it = traces.cbegin(); it != traces.end(); ++it) {
            ଅଚଳ TraceTraceVertex* ଅଚଳ vtxp = it->second;
            AstTraceDecl* ଅଚଳ declp = vtxp->nodep();
            ଯଦି (vtxp->duplicatep()) {
                duplicates.ଠେଲ_back(vtxp);
                ଜାରି;
            }

            // This is a canonical trace node. Assign trace code (signal number).
            UASSERT_OBJ(!declp->codeAssigned(), declp,
                        "Canonical node should not have code assigned yet");
            declp->code(m_code);
            ଅଚଳ uint32_t codeInc = declp->codeInc();
            m_code += codeInc;
            ++m_statUniqSigs;

            // If this is a const signal, add the AstTraceInc
            ଅଚଳ ActCodeSet& actSet = it->first;
            ଯଦି (actSet.ଗଣନା_2(TraceActivityVertex::ACTIVITY_NEVER)) {
                // Crate new sub function if required
                ଯଦି (!subFuncp || subStmts > splitLimit) {
                    subStmts = 0;
                    subFuncp = newCFunc(VTraceType::CONSTANT, ଶୀର୍ଷ_Funcp, sub_Func_ଅଙ୍କ);
                    ++sub_Func_ଅଙ୍କ;
                }
                File_ଧାଡ଼ି* ଅଚଳ flp = declp->fileline();
                ଯଦି (declp->dtypeCallp()) {
                    AstCFunc* ଅଚଳ funcp = createConstDtypeTraceFunctions(declp);
                    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* argsp = nullptr;
                    argsp = Ast_ଗଣ୍ଠି::addNext(argsp, declp->valuep()->clone_ବୃକ୍ଷ(ମିଥ୍ୟା));
                    AstCCall* ଅଚଳ callp = ନୂତନ AstCCall{flp, funcp, argsp};
                    callp->dtypeSetVoid();
                    callp->argTypes(callp->argTypes() + "bufp, " + std::to_string(declp->code()));
                    subFuncp->addStmtsp(callp->makeStmt());

                    subStmts += 1;
                } ଅନ୍ୟଥା {
                    AstTraceInc* ଅଚଳ incp = ନୂତନ AstTraceInc{flp, declp, VTraceType::CONSTANT};
                    subFuncp->addStmtsp(incp);
                    subStmts += incp->ଗଣ୍ଠି_ଗଣନା();
                }
            }
        }

        // Assign codes to duplicate nodes from their canonicals
        ପାଇଁ (ଅଚଳ TraceTraceVertex* ଅଚଳ vtxp : duplicates) {
            AstTraceDecl* ଅଚଳ declp = vtxp->nodep();
            ଅଚଳ TraceTraceVertex* ଅଚଳ canonVtxp = vtxp->duplicatep();
            ଅଚଳ AstTraceDecl* ଅଚଳ canonDeclp = canonVtxp->nodep();
            UASSERT_OBJ(!canonVtxp->duplicatep(), canonDeclp, "Canonical node is a duplicate");
            UASSERT_OBJ(canonDeclp->codeAssigned(), canonDeclp,
                        "Canonical node should have code assigned already");
            declp->code(canonDeclp->code() + vtxp->dtypeAliasOffset());
        }
    }

    DtypeFuncs createNonConstDtypeTraceFunctions(ଅଚଳ AstTraceDecl* declp) {
        Ast_ଗଣ୍ଠି_DType* dtypep = declp->dtypep()->skipRefp();
        auto pair = m_dtypeNonConstFuncs.emplace(dtypep, DtypeFuncs{});
        ଯଦି (pair.second) {
            File_ଧାଡ଼ି* ଅଚଳ flp = declp->fileline();
            AstCFunc* ଅଚଳ fullFuncp = newCFunc(VTraceType::FULL, nullptr, 0, 0, declp);
            AstCFunc* ଅଚଳ chgFuncp = newCFunc(VTraceType::CHANGE, nullptr, 0, 0, declp);

            ତାର୍କିକ first = ସତ୍ୟ;
            ପାଇଁ (Ast_ଗଣ୍ଠି* callStmtp = declp->dtypeCallp()->funcp()->stmtsp(); callStmtp;
                 callStmtp = callStmtp->nextp()) {
                ଅଚଳ AstCCall* ଅଚଳ callp
                    = VN_CAST(VN_CAST(callStmtp, Stmt_ପଦସମଷ୍ଟି)->exprp(), CCall);
                ତାର୍କିକ onlyOne = first && !callStmtp->nextp();
                AstCFunc* ଅଚଳ fullSubFuncp
                    = onlyOne ? fullFuncp : newCDtypeSubFunc(VTraceType::FULL, declp, fullFuncp);
                AstCFunc* ଅଚଳ chgSubFuncp
                    = onlyOne ? chgFuncp : newCDtypeSubFunc(VTraceType::CHANGE, declp, chgFuncp);
                ପାଇଁ (Ast_ଗଣ୍ଠି* stmtp = callp->funcp()->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    ଯଦି (AstTraceDecl* ଅଚଳ fieldDeclp = VN_CAST(stmtp, TraceDecl)) {
                        AstTraceInc* ଅଚଳ incFullp
                            = ନୂତନ AstTraceInc{flp, fieldDeclp, VTraceType::FULL};
                        fullSubFuncp->addStmtsp(incFullp);
                        AstTraceInc* ଅଚଳ incChgp
                            = ନୂତନ AstTraceInc{flp, fieldDeclp, VTraceType::CHANGE};
                        chgSubFuncp->addStmtsp(incChgp);
                    }
                }
                first = ମିଥ୍ୟା;
            }

            pair.first->second = {.fullFuncp = fullFuncp, .chgFuncp = chgFuncp};
        }

        ଫେରନ୍ତୁ pair.first->second;
    }

    ଶୂନ୍ୟ createNonConstTraceFunctions(ଅଚଳ TraceVec& traces, uint32_t nAllCodes,
                                      uint32_t parallelism) {
        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା splitLimit = v3Global.opt.outputSplitCTrace() ? v3Global.opt.outputSplitCTrace()
                                                                : std::numeric_limits<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>::max();

        // pre-incremented, so starts at 0
        uint32_t ଶୀର୍ଷ_Func_ଅଙ୍କ = std::numeric_limits<uint32_t>::max();
        TraceVec::const_iterator it = traces.begin();
        ଯେତେବେଳେ (it != traces.end()) {
            AstCFunc* ଶୀର୍ଷ_Ful_Funcp = nullptr;
            AstCFunc* ଶୀର୍ଷ_Chg_Funcp = nullptr;
            AstCFunc* subFulFuncp = nullptr;
            AstCFunc* subChgFuncp = nullptr;
            uint32_t sub_Func_ଅଙ୍କ = 0;
            ପୂର୍ଣ୍ଣସଂଖ୍ୟା subStmts = 0;
            ଅଚଳ uint32_t maxCodes = std::max((nAllCodes + parallelism - 1) / parallelism, 1U);
            uint32_t nCodes = 0;
            ଅଚଳ ActCodeSet* prevActSet = nullptr;
            AstIf* ifp = nullptr;
            uint32_t baseCode = 0;
            ପାଇଁ (; nCodes < maxCodes && it != traces.end(); ++it) {
                ଅଚଳ ActCodeSet& actSet = it->first;
                // Traced value never changes, no need to add it
                ଯଦି (actSet.ଗଣନା_2(TraceActivityVertex::ACTIVITY_NEVER)) ଜାରି;

                ଅଚଳ TraceTraceVertex* ଅଚଳ vtxp = it->second;
                AstTraceDecl* ଅଚଳ declp = vtxp->nodep();

                // This is a duplicate decl, no need to add it, but must set the
                // function index to the same as the canonical node.
                ଯଦି (ଅଚଳ TraceTraceVertex* ଅଚଳ canonVtxp = vtxp->duplicatep()) {
                    declp->fidx(canonVtxp->nodep()->fidx());
                    ଜାରି;
                }

                // Create top function if not yet created
                ଯଦି (!ଶୀର୍ଷ_Ful_Funcp) {
                    ++ଶୀର୍ଷ_Func_ଅଙ୍କ;
                    ଶୀର୍ଷ_Ful_Funcp = newCFunc(VTraceType::FULL, nullptr, ଶୀର୍ଷ_Func_ଅଙ୍କ);
                    ଶୀର୍ଷ_Chg_Funcp = newCFunc(VTraceType::CHANGE, nullptr, ଶୀର୍ଷ_Func_ଅଙ୍କ);
                }

                // Create new sub function if required
                ଯଦି (!subFulFuncp || subStmts > splitLimit) {
                    baseCode = declp->code();
                    subStmts = 0;
                    subFulFuncp = newCFunc(VTraceType::FULL, ଶୀର୍ଷ_Ful_Funcp, sub_Func_ଅଙ୍କ, baseCode);
                    subChgFuncp = newCFunc(VTraceType::CHANGE, ଶୀର୍ଷ_Chg_Funcp, sub_Func_ଅଙ୍କ, baseCode);
                    ++sub_Func_ଅଙ୍କ;
                    prevActSet = nullptr;
                    ifp = nullptr;
                }

                // If required, create the conditional node checking the activity flags
                ଯଦି (!prevActSet || actSet != *prevActSet) {
                    File_ଧାଡ଼ି* ଅଚଳ flp = m_ଶୀର୍ଷ_Scopep->fileline();
                    ଅଚଳ ତାର୍କିକ always = actSet.ଗଣନା_2(TraceActivityVertex::ACTIVITY_ALWAYS) != 0;
                    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* condp = nullptr;
                    ଯଦି (always) {
                        condp = ନୂତନ AstConst{flp, 1};  // Always true, will be folded later
                    } ଅନ୍ୟଥା {
                        ପାଇଁ (ଅଚଳ uint32_t actCode : actSet) {
                            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ selp = selectActivity(flp, actCode, VAccess::READ);
                            condp = condp ? ନୂତନ AstOr{flp, condp, selp} : selp;
                        }
                    }
                    ifp = ନୂତନ AstIf{flp, condp};
                    ଯଦି (!always) ifp->branchPred(VBranchPred::BP_UNLIKELY);
                    subChgFuncp->addStmtsp(ifp);
                    subStmts += ifp->ଗଣ୍ଠି_ଗଣନା();
                    prevActSet = &actSet;
                }

                // Add TraceInc nodes
                File_ଧାଡ଼ି* ଅଚଳ flp = declp->fileline();
                ଯଦି (declp->dtypeCallp()) {
                    DtypeFuncs funcs = createNonConstDtypeTraceFunctions(declp);
                    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* argsp = nullptr;
                    argsp = Ast_ଗଣ୍ଠି::addNext(argsp, declp->valuep()->clone_ବୃକ୍ଷ(ମିଥ୍ୟା));
                    AstCCall* ଅଚଳ callFullp = ନୂତନ AstCCall{flp, funcs.fullFuncp, argsp};
                    callFullp->dtypeSetVoid();
                    callFullp->argTypes(callFullp->argTypes() + "bufp, "
                                        + std::to_string(declp->code()));
                    subFulFuncp->addStmtsp(callFullp->makeStmt());
                    argsp = nullptr;
                    argsp = Ast_ଗଣ୍ଠି::addNext(argsp, declp->valuep()->clone_ବୃକ୍ଷ(ମିଥ୍ୟା));
                    AstCCall* ଅଚଳ callChgp = ନୂତନ AstCCall{flp, funcs.chgFuncp, argsp};
                    callChgp->dtypeSetVoid();
                    callChgp->argTypes(callChgp->argTypes() + "bufp, "
                                       + std::to_string(declp->code()));
                    ifp->addThensp(callChgp->makeStmt());

                    subStmts += 2;
                } ଅନ୍ୟଥା {
                    AstTraceInc* ଅଚଳ incFulp = ନୂତନ AstTraceInc{flp, declp, VTraceType::FULL};
                    subFulFuncp->addStmtsp(incFulp);
                    AstTraceInc* ଅଚଳ incChgp
                        = ନୂତନ AstTraceInc{flp, declp, VTraceType::CHANGE, baseCode};
                    ifp->addThensp(incChgp);

                    // Set the function index of the decl
                    declp->fidx(ଶୀର୍ଷ_Func_ଅଙ୍କ);

                    // Track splitting due to size
                    UASSERT_OBJ(incFulp->ଗଣ୍ଠି_ଗଣନା() == incChgp->ଗଣ୍ଠି_ଗଣନା(), declp,
                                "Should have equal cost");
                    ଅଚଳ VNumRange range = declp->arrayRange();
                    ଯଦି (range.ranged()) {
                        // 2x because each element is a TraceInc and a VarRef
                        subStmts += range.elements() * 2;
                    } ଅନ୍ୟଥା {
                        subStmts += incChgp->ଗଣ୍ଠି_ଗଣନା();
                    }
                }

                // Track partitioning
                nCodes += declp->codeInc();
            }
        }
    }

    ଶୂନ୍ୟ createCleanupFunction() {
        File_ଧାଡ଼ି* ଅଚଳ fl = m_ଶୀର୍ଷ_Scopep->fileline();
        AstCFunc* ଅଚଳ cleanupFuncp = ନୂତନ AstCFunc{fl, "trace_cleanup", m_ଶୀର୍ଷ_Scopep};
        cleanupFuncp->argTypes("void* voidSelf, " + v3Global.opt.traceClassBase()
                               + "* /*unused*/");
        cleanupFuncp->isTrace(ସତ୍ୟ);
        cleanupFuncp->slow(ମିଥ୍ୟା);
        cleanupFuncp->isStatic(ସତ୍ୟ);
        cleanupFuncp->isLoose(ସତ୍ୟ);
        m_ଶୀର୍ଷ_Scopep->addBlocksp(cleanupFuncp);
        cleanupFuncp->addStmtsp(ନୂତନ AstCStmt{fl, EmitCUtil::voidSelfAssign(m_ଶୀର୍ଷ_Modp)});
        cleanupFuncp->addStmtsp(ନୂତନ AstCStmt{fl, EmitCUtil::symClassAssign()});

        // Register it
        {
            AstCStmt* ଅଚଳ cstmtp = ନୂତନ AstCStmt{fl};
            m_regFuncp->addStmtsp(cstmtp);
            cstmtp->add("tracep->addCleanupCb(");
            cstmtp->add(ନୂତନ AstAddrOfCFunc{fl, cleanupFuncp});
            cstmtp->add(", vlSelf);");
        }

        // Clear global activity flag
        cleanupFuncp->addStmtsp(
            ନୂତନ AstCStmt{m_ଶୀର୍ଷ_Scopep->fileline(), "vlSymsp->__Vm_activity = false;"s});

        // Clear fine grained activity flags
        ପାଇଁ (uint32_t i = 0; i < m_activity_ସଂଖ୍ୟା; ++i) {
            Ast_ଗଣ୍ଠି* ଅଚଳ clrp = ନୂତନ AstAssign{fl, selectActivity(fl, i, VAccess::WRITE),
                                                ନୂତନ AstConst{fl, AstConst::BitFalse{}}};
            cleanupFuncp->addStmtsp(clrp);
        }
    }

    ଶୂନ୍ୟ createTraceFunctions() {
        // Detect and remove duplicate values
        detectDuplicates();

        graphDtypePrune();
        splitTraceDeclFuncs();
        m_graph.removeRedundantEdgesMax(&V3GraphEdge::followAlwaysTrue);

        // Simplify & optimize the graph
        ଯଦି (dumpGraphLevel() >= 6) m_graph.dumpDotFilePrefixed("trace_pre");
        graphSimplify(ସତ୍ୟ);
        ଯଦି (dumpGraphLevel() >= 6) m_graph.dumpDotFilePrefixed("trace_simplified");
        graphOptimize();
        ଯଦି (dumpGraphLevel() >= 6) m_graph.dumpDotFilePrefixed("trace_optimized");

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
        m_regFuncp = ନୂତନ AstCFunc{m_ଶୀର୍ଷ_Scopep->fileline(), "trace_register", m_ଶୀର୍ଷ_Scopep};
        m_regFuncp->argTypes(v3Global.opt.traceClassBase() + "* tracep");
        m_regFuncp->entryPoint(ସତ୍ୟ);
        m_regFuncp->isTrace(ସତ୍ୟ);
        m_regFuncp->slow(ସତ୍ୟ);
        m_regFuncp->isStatic(ମିଥ୍ୟା);
        m_regFuncp->isLoose(ସତ୍ୟ);
        m_ଶୀର୍ଷ_Scopep->addBlocksp(m_regFuncp);

        // Create the const dump functions. Also allocates trace codes.
        createConstTraceFunctions(traces);

        // Create the full and incremental dump functions
        createNonConstTraceFunctions(traces, nNonConstCodes, m_parallelism);

        // Root-traced libraries alias wrapper IOs onto the existing top-module codes.
        ଯଦି (!v3Global.opt.libCreate().empty()) {
            std::vector<AstTraceDecl*> ମୂଳ_Declps;
            std::vector<AstTraceDecl*> ଶୀର୍ଷ_Declps;
            TraceInitDeclCollector ମୂଳ_Collector{ମୂଳ_Declps};
            TraceInitDeclCollector ଶୀର୍ଷ_Collector{ଶୀର୍ଷ_Declps};
            ପାଇଁ (Ast_ଗଣ୍ଠି* blockp = m_ଶୀର୍ଷ_Scopep->blocksp(); blockp; blockp = blockp->nextp()) {
                AstCFunc* ଅଚଳ funcp = VN_CAST(blockp, CFunc);
                ଯଦି (funcp && VString::startsWith(funcp->name(), "trace_init_leaf_root__")) {
                    ମୂଳ_Collector.collect(funcp);
                } ଅନ୍ୟଥା ଯଦି (funcp && VString::startsWith(funcp->name(), "trace_init_leaf_top__")) {
                    ଶୀର୍ଷ_Collector.collect(funcp);
                }
            }

            std::vector<ତାର୍କିକ> used(ଶୀର୍ଷ_Declps.size(), ମିଥ୍ୟା);
            ପାଇଁ (AstTraceDecl* ଅଚଳ ମୂଳ_Declp : ମୂଳ_Declps) {
                ପାଇଁ (size_t i = 0; i < ଶୀର୍ଷ_Declps.size(); ++i) {
                    ଯଦି (used[i] || !same_ମୂଳ_Init_Alias(ମୂଳ_Declp, ଶୀର୍ଷ_Declps[i])) ଜାରି;
                    ମୂଳ_Declp->code(ଶୀର୍ଷ_Declps[i]->code());
                    used[i] = ସତ୍ୟ;
                    ଭାଙ୍ଗ;
                }
            }
        }

        // Remove refs to traced values from TraceDecl nodes, these have now moved under
        // TraceInc
        ପାଇଁ (ଅଚଳ auto& i : traces) {
            Ast_ଗଣ୍ଠି* ଅଚଳ valuep = i.second->nodep()->valuep();
            VL_DO_DANGLING(valuep->unlinkFrBack()->delete_ବୃକ୍ଷ(), valuep);
        }

        // Create the trace cleanup function clearing the activity flags
        createCleanupFunction();
    }

    TraceCFuncVertex* getCFuncVertexp(AstCFunc* nodep) {
        V3GraphVertex* ଅଚଳ vtxp = nodep->user1u().toGraphVertex();
        TraceCFuncVertex* vertexp = vtxp ? vtxp->cast<TraceCFuncVertex>() : nullptr;
        ଯଦି (!vertexp) {
            vertexp = ନୂତନ TraceCFuncVertex{&m_graph, nodep};
            nodep->user1p(vertexp);
        }
        ଫେରନ୍ତୁ vertexp;
    }
    TraceActivityVertex* getActivityVertexp(Ast_ଗଣ୍ଠି* nodep, ତାର୍କିକ slow) {
        V3GraphVertex* ଅଚଳ vtxp = nodep->user3u().toGraphVertex();
        TraceActivityVertex* vertexp = vtxp ? vtxp->cast<TraceActivityVertex>() : nullptr;
        ଯଦି (!vertexp) {
            vertexp = ନୂତନ TraceActivityVertex{&m_graph, nodep, slow};
            nodep->user3p(vertexp);
        }
        vertexp->slow(slow);
        ଫେରନ୍ତୁ vertexp;
    }

    // VISITORS
    ଶୂନ୍ୟ visit(AstNetlist* nodep) override {
        // Add vertexes for all TraceDecl, and edges from VARs each trace looks at
        m_finding = ମିଥ୍ୟା;
        iterateChildren(nodep);

        // Add vertexes for all CFUNCs, and edges to VARs the func sets
        m_finding = ସତ୍ୟ;
        iterateChildren(nodep);

        // Create the trace functions and insert them into the tree
        createTraceFunctions();

        // Save number of trace codes used
        nodep->nTraceCodes(m_code);
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Module* nodep) override {
        ଯଦି (nodep->is_ଶୀର୍ଷ()) m_ଶୀର୍ଷ_Modp = nodep;
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(AstVarScope* nodep) override {
        ଯଦି (!m_finding) {
            ଯଦି (ଅଚଳ AstIface* ଅଚଳ ifacep = nodep->varp()->sensIfacep()) {
                m_ifaceMemberVscps[{ifacep, nodep->varp()->name()}].ଠେଲ_back(nodep);
            }
        }
    }
    ଶୂନ୍ୟ visit(Ast_Stmt_ପଦସମଷ୍ଟି* nodep) override {
        ଯଦି (!m_finding && !nodep->user2()) {
            ଯଦି (AstCCall* ଅଚଳ callp = VN_CAST(nodep->exprp(), CCall)) {
                UINFO(8, "   CCALL " << callp);
                // See if there are other calls in same statement list;
                // If so, all funcs might share the same activity code
                TraceActivityVertex* ଅଚଳ activityVtxp
                    = getActivityVertexp(nodep, callp->funcp()->slow());
                ପାଇଁ (Ast_ଗଣ୍ଠି* nextp = nodep; nextp; nextp = nextp->nextp()) {
                    ଯଦି (Ast_Stmt_ପଦସମଷ୍ଟି* ଅଚଳ stmtp = VN_CAST(nextp, Stmt_ପଦସମଷ୍ଟି)) {
                        ଯଦି (AstCCall* ଅଚଳ ccallp = VN_CAST(stmtp->exprp(), CCall)) {
                            stmtp->user2(ସତ୍ୟ);  // Processed
                            UINFO(8, "     SubCCALL " << ccallp);
                            V3GraphVertex* ଅଚଳ ccallFuncVtxp = getCFuncVertexp(ccallp->funcp());
                            activityVtxp->slow(ccallp->funcp()->slow());
                            ନୂତନ V3GraphEdge{&m_graph, activityVtxp, ccallFuncVtxp, 1};
                        }
                    }
                }
            }
        }
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(AstCFunc* nodep) override {
        UINFO(8, "   CFUNC " << nodep);
        V3GraphVertex* ଅଚଳ funcVtxp = getCFuncVertexp(nodep);
        ଯଦି (!m_finding) {  // If public, we need a unique activity code to allow for sets
                           // directly in this func
            ଯଦି (nodep->funcPublic() || nodep->dpiExportImpl() || nodep->entryPoint()
                || nodep->isCoroutine()) {
                // Cannot treat a coroutine as slow, it may be resumed later
                ଅଚଳ ତାର୍କିକ slow = nodep->slow() && !nodep->isCoroutine();
                TraceActivityVertex* ଅଚଳ activityVtxp = getActivityVertexp(nodep, slow);
                ନୂତନ V3GraphEdge{&m_graph, activityVtxp, funcVtxp, 1};
            }
        }
        VL_RESTORER(m_cfuncp);
        m_cfuncp = nodep;
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(AstTraceDecl* nodep) override {
        UINFO(8, "   TRACE " << nodep);
        ଯଦି (!m_finding && !nodep->inDtypeFunc()) {
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
            ଯଦି (m_cfuncp && VString::startsWith(m_cfuncp->name(), "trace_init_leaf_root__")) {
                ଫେରନ୍ତୁ;
            }
            V3GraphVertex* ଅଚଳ vertexp = ନୂତନ TraceTraceVertex{&m_graph, nodep};
            nodep->user1p(vertexp);

            UASSERT_OBJ(m_cfuncp, nodep, "Trace not under func");
            m_cfuncp->user2(ସତ୍ୟ);
            VL_RESTORER(m_tracep);
            m_tracep = nodep;
            iterateChildren(nodep);
        }
    }
    ଶୂନ୍ୟ visit(AstVarRef* nodep) override {
        UASSERT_OBJ(nodep->varScopep(), nodep, "No var scope?");
        AstVarScope* ଅଚଳ varscopep = nodep->varScopep();
        V3GraphVertex* varVtxp = varscopep->user1u().toGraphVertex();
        ଯଦି (m_tracep) {
            UASSERT_OBJ(nodep->access().isReadOnly(), nodep, "Lvalue in trace?  Should be const.");
            ଯଦି (!varVtxp) {
                varVtxp = ନୂତନ TraceVarVertex{&m_graph, nodep->varScopep()};
                nodep->varScopep()->user1p(varVtxp);
            }
            V3GraphVertex* ଅଚଳ traceVtxp = m_tracep->user1u().toGraphVertex();
            ନୂତନ V3GraphEdge{&m_graph, varVtxp, traceVtxp, 1};
            ଯଦି (nodep->varp()->isPrimaryInish()  // Always need to trace primary inputs
                || nodep->varp()->isSigPublic()) {  // Or ones user can change
                ନୂତନ V3GraphEdge{&m_graph, m_alwaysVtxp, traceVtxp, 1};
            }
            ଯଦି (m_tracep->dtypeCallp()) varscopep->user2p(m_tracep);
        } ଅନ୍ୟଥା ଯଦି (m_cfuncp && m_finding && nodep->access().isWriteOrRW()) {
            V3GraphVertex* ଅଚଳ funcVtxp = getCFuncVertexp(m_cfuncp);
            ଯଦି (varVtxp) {  // else we're not tracing this signal
                ନୂତନ V3GraphEdge{&m_graph, funcVtxp, varVtxp, 1};
                AstTraceDecl* ଅଚଳ declp = VN_AS(varscopep->user2p(), TraceDecl);
                ଯଦି (declp) {
                    V3GraphVertex* ଅଚଳ cFuncVtxp = getCFuncVertexp(m_cfuncp);
                    ପାଇଁ (ଅଚଳ V3GraphEdge& edge : cFuncVtxp->inEdges()) {
                        V3GraphVertex* ଅଚଳ activityp = edge.fromp();
                        ଯଦି (!declp->user3p()) {
                            declp->user3p(activityp);
                        } ଅନ୍ୟଥା ଯଦି (declp->user3u().toGraphVertex() != activityp) {
                            declp->user2(ସତ୍ୟ);
                        }
                    }
                }
            }
        }
    }
    ଶୂନ୍ୟ visit(AstMemberSel* nodep) override {
        ଯଦି (m_cfuncp && m_finding && nodep->access().isWriteOrRW()) {
            AstIfaceRefDType* ଅଚଳ dtypep
                = VN_CAST(nodep->fromp()->dtypep()->skipRefp(), IfaceRefDType);
            ଯଦି (dtypep && dtypep->isVirtual()) {
                ଅଚଳ auto it = m_ifaceMemberVscps.find({dtypep->ifacep(), nodep->varp()->name()});
                ଯଦି (it != m_ifaceMemberVscps.end()) {
                    V3GraphVertex* ଅଚଳ funcVtxp = getCFuncVertexp(m_cfuncp);
                    ପାଇଁ (AstVarScope* ଅଚଳ vscp : it->second) {
                        V3GraphVertex* varVtxp = vscp->user1u().toGraphVertex();
                        ଯଦି (!varVtxp) {
                            varVtxp = ନୂତନ TraceVarVertex{&m_graph, vscp};
                            vscp->user1p(varVtxp);
                        }
                        ନୂତନ V3GraphEdge{&m_graph, funcVtxp, varVtxp, 1};
                    }
                }
            }
        }
        iterateChildren(nodep);
    }
    //--------------------
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override { iterateChildren(nodep); }

ସର୍ବସାଧାରଣ:
    // CONSTRUCTORS
    explicit TraceVisitor(AstNetlist* nodep)
        : m_alwaysVtxp{ନୂତନ TraceActivityVertex{&m_graph, TraceActivityVertex::ACTIVITY_ALWAYS}} {
        nodep->user2_Clear_ବୃକ୍ଷ();  // TraceDecl multiple activities flag
        nodep->user3_Clear_ବୃକ୍ଷ();  // TraceDecl TraceActivityVertex (assumes we start at nullptr)
        iterate(nodep);
        nodep->foreach([](AstTraceDecl* ଅଚଳ declp) {
            ଯଦି (declp->inDtypeFunc()) {
                declp->valuep()->unlinkFrBack()->delete_ବୃକ୍ଷ();
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

ଶୂନ୍ୟ V3Trace::traceAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { TraceVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_ବୃକ୍ଷ("trace", 0, dump_ବୃକ୍ଷ_Either_Level() >= 3);
}
