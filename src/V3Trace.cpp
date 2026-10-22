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

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ಅನ್ತರ್ಭಾವ "V3Trace.h"

#ಅನ್ತರ್ಭಾವ "V3Ast.h"
#ಅನ್ತರ್ಭಾವ "V3DupFinder.h"
#ಅನ್ತರ್ಭಾವ "V3EmitCBase.h"
#ಅನ್ತರ್ಭಾವ "V3Graph.h"
#ಅನ್ತರ್ಭಾವ "V3Stats.h"
#ಅನ್ತರ್ಭಾವ "V3UniqueNames.h"

#ಅನ್ತರ್ಭಾವ <limits>
#ಅನ್ತರ್ಭಾವ <map>
#ಅನ್ತರ್ಭಾವ <set>
#ಅನ್ತರ್ಭಾವ <unordered_map>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Graph vertexes

ವರ್ಗ TraceActivityVertex final : ಸಾರ್ವಜನಿಕ V3GraphVertex {
    VL_RTTI_IMPL(TraceActivityVertex, V3GraphVertex)
    Ast_ಗಂಟು* ಧ್ರುವ m_insertp;
    int32_t m_activityCode;
    ತಾರ್ಕಿಕ m_slow;  // If always slow, we can use the same code
ಸಾರ್ವಜನಿಕ:
    ಗಣನಾ { ACTIVITY_NEVER = ((1UL << 31) - 1) };
    ಗಣನಾ { ACTIVITY_ALWAYS = ((1UL << 31) - 2) };
    ಗಣನಾ { ACTIVITY_SLOW = 0 };
    TraceActivityVertex(V3Graph* graphp, Ast_ಗಂಟು* nodep, ತಾರ್ಕಿಕ slow)
        : V3GraphVertex{graphp}
        , m_insertp{nodep} {
        m_activityCode = 0;
        m_slow = slow;
    }
    TraceActivityVertex(V3Graph* graphp, int32_t code)
        : V3GraphVertex{graphp}
        , m_insertp{nullptr} {
        m_activityCode = code;
        m_slow = ಅಸತ್ಯ;
    }
    ~TraceActivityVertex() override = ಸ್ವಭಾವ;
    // ACCESSORS
    Ast_ಗಂಟು* insertp() ಧ್ರುವ {
        UASSERT(m_insertp, "Null insertp; probably called on a special always/slow");
        ಪ್ರತ್ಯಾಗಮ m_insertp;
    }
    string name() ಧ್ರುವ override {
        ಯದಿ (activityAlways()) {
            ಪ್ರತ್ಯಾಗಮ "*ALWAYS*";
        } ಅನ್ಯಥಾ {
            ಪ್ರತ್ಯಾಗಮ std::string{slow() ? "*SLOW* " : ""} + insertp()->name();
        }
    }
    string dotColor() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ slow() ? "yellowGreen" : "green"; }
    int32_t activityCode() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_activityCode; }
    ತಾರ್ಕಿಕ activityAlways() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ activityCode() == ACTIVITY_ALWAYS; }
    ತಾರ್ಕಿಕ activitySlow() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ activityCode() == ACTIVITY_SLOW; }
    ಶೂನ್ಯ activityCode(int32_t code) { m_activityCode = code; }
    ತಾರ್ಕಿಕ slow() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_slow; }
    ಶೂನ್ಯ slow(ತಾರ್ಕಿಕ flag) {
        ಯದಿ (!flag) m_slow = ಅಸತ್ಯ;
    }
};

ವರ್ಗ TraceCFuncVertex final : ಸಾರ್ವಜನಿಕ V3GraphVertex {
    VL_RTTI_IMPL(TraceCFuncVertex, V3GraphVertex)
    AstCFunc* ಧ್ರುವ m_nodep;

ಸಾರ್ವಜನಿಕ:
    TraceCFuncVertex(V3Graph* graphp, AstCFunc* nodep)
        : V3GraphVertex{graphp}
        , m_nodep{nodep} {}
    ~TraceCFuncVertex() override = ಸ್ವಭಾವ;
    // ACCESSORS
    AstCFunc* nodep() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_nodep; }
    string name() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ nodep()->name(); }
    string dotColor() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ "yellow"; }
    File_ಸಾಲು* fileline() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ nodep()->fileline(); }
};

ವರ್ಗ TraceTraceVertex final : ಸಾರ್ವಜನಿಕ V3GraphVertex {
    VL_RTTI_IMPL(TraceTraceVertex, V3GraphVertex)
    AstTraceDecl* ಧ್ರುವ m_nodep;  // TRACEINC this represents
    // nullptr, or other vertex with the real code() that duplicates this one
    TraceTraceVertex* m_duplicatep = nullptr;
    // When aliasing to a dtype parent, offset of the member within the dtype
    uint32_t m_dtypeAliasOffset = 0;

ಸಾರ್ವಜನಿಕ:
    TraceTraceVertex(V3Graph* graphp, AstTraceDecl* nodep)
        : V3GraphVertex{graphp}
        , m_nodep{nodep} {}
    ~TraceTraceVertex() override = ಸ್ವಭಾವ;
    // ACCESSORS
    AstTraceDecl* nodep() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_nodep; }
    string name() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ nodep()->name(); }
    string dotColor() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ "red"; }
    File_ಸಾಲು* fileline() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ nodep()->fileline(); }
    TraceTraceVertex* duplicatep() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_duplicatep; }
    ಶೂನ್ಯ duplicatep(TraceTraceVertex* dupp) {
        UASSERT_OBJ(!duplicatep(), nodep(), "Assigning duplicatep() to already duplicated node");
        m_duplicatep = dupp;
    }
    ಶೂನ್ಯ redirectDuplicatep(TraceTraceVertex* dupp) { m_duplicatep = dupp; }
    uint32_t dtypeAliasOffset() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_dtypeAliasOffset; }
    ಶೂನ್ಯ dtypeAliasOffset(uint32_t offset) { m_dtypeAliasOffset = offset; }
};

ವರ್ಗ TraceVarVertex final : ಸಾರ್ವಜನಿಕ V3GraphVertex {
    VL_RTTI_IMPL(TraceVarVertex, V3GraphVertex)
    AstVarScope* ಧ್ರುವ m_nodep;

ಸಾರ್ವಜನಿಕ:
    TraceVarVertex(V3Graph* graphp, AstVarScope* nodep)
        : V3GraphVertex{graphp}
        , m_nodep{nodep} {}
    ~TraceVarVertex() override = ಸ್ವಭಾವ;
    // ACCESSORS
    AstVarScope* nodep() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_nodep; }
    string name() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ nodep()->name(); }
    string dotColor() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ "skyblue"; }
    File_ಸಾಲು* fileline() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ nodep()->fileline(); }
};

//######################################################################
// Trace state, as a visitor of each AstNode

ವರ್ಗ TraceVisitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
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
    ಧ್ರುವ VNUser1InUse m_inuser1;
    ಧ್ರುವ VNUser2InUse m_inuser2;
    ಧ್ರುವ VNUser3InUse m_inuser3;
    // VNUser4InUse     In V3Hasher via V3DupFinder

    // STATE
    Ast_ಗಂಟು_Module* m_ತುದಿ_Modp = nullptr;  // Module to add variables to
    AstScope* ಧ್ರುವ m_ತುದಿ_Scopep = v3Global.rootp()->ತುದಿ_Scopep()->scopep();  // The top AstScope
    AstCFunc* m_cfuncp = nullptr;  // C function adding to graph
    AstCFunc* m_regFuncp = nullptr;  // Trace registration function
    AstCFunc* m_actAllFuncp = nullptr;  // Set all activity function
    AstTraceDecl* m_tracep = nullptr;  // Trace function adding to graph
    AstVarScope* m_activityVscp = nullptr;  // Activity variable
    uint32_t m_activity_ಸಂಖ್ಯೆ = 0;  // Count of fields in activity variable
    uint32_t m_code = 0;  // Trace ident code# being assigned
    V3Graph m_graph;  // Var/CFunc tracking
    TraceActivityVertex* ಧ್ರುವ m_alwaysVtxp;  // "Always trace" vertex
    ತಾರ್ಕಿಕ m_finding = ಅಸತ್ಯ;  // Pass one of algorithm?
    ಸಂರಚನಾ DtypeFuncs final {
    ಸಾರ್ವಜನಿಕ:
        AstCFunc* fullFuncp = nullptr;
        AstCFunc* chgFuncp = nullptr;
    };
    std::unordered_map<ಧ್ರುವ Ast_ಗಂಟು_DType*, DtypeFuncs>
        m_dtypeNonConstFuncs;  // Full / Chg funcs per type
    std::unordered_map<ಧ್ರುವ Ast_ಗಂಟು_DType*, AstCFunc*> m_dtypeConstFuncs;  // Const func per type
    V3UniqueNames m_dtypeNames{""};  // Unique type func names

    // Trace parallelism. Only VCD tracing can be parallelized at this time.
    ಧ್ರುವ uint32_t m_parallelism
        = v3Global.opt.useTraceParallel() ? static_cast<uint32_t>(v3Global.opt.threads()) : 1;

    VDouble0 m_statSetters;  // Statistic tracking
    VDouble0 m_statSettersSlow;  // Statistic tracking
    VDouble0 m_statUniqSigs;  // Statistic tracking

    // All activity numbers applying to a given trace
    using ActCodeSet = std::set<uint32_t>;
    // For activity set, what traces apply
    using TraceVec = std::multimap<ActCodeSet, TraceTraceVertex*>;
    // Candidate interface-member VarScopes keyed by (interface type, member name)
    std::map<std::pair<ಧ್ರುವ AstIface*, std::string>, std::vector<AstVarScope*>>
        m_ifaceMemberVscps;

    ವರ್ಗ TraceInitDeclCollector final : ಸಾರ್ವಜನಿಕ VNVisitor {
        std::vector<AstTraceDecl*>& m_declps;  // Output: trace declarations found so far
        std::set<ಧ್ರುವ AstCFunc*> m_seenFuncps;

        ಶೂನ್ಯ visit(AstTraceDecl* nodep) override { m_declps.ತಳ್ಳು_back(nodep); }
        ಶೂನ್ಯ visit(AstCCall* nodep) override {
            ಯದಿ (AstCFunc* ಧ್ರುವ funcp = nodep->funcp()) collect(funcp);
        }
        ಶೂನ್ಯ visit(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }

    ಸಾರ್ವಜನಿಕ:
        explicit TraceInitDeclCollector(std::vector<AstTraceDecl*>& declps)
            : m_declps{declps} {}
        ಶೂನ್ಯ collect(AstCFunc* funcp) {
            ಯದಿ (funcp && m_seenFuncps.ಸೇರಿಸು(funcp).second) iterate(funcp);
        }
    };

    // METHODS

    ಸ್ಥಿರ ತಾರ್ಕಿಕ same_ಬೇರು_Init_Alias(ಧ್ರುವ AstTraceDecl* ಬೇರು_Declp, ಧ್ರುವ AstTraceDecl* ತುದಿ_Declp) {
        ಧ್ರುವ VNumRange& ಬೇರು_Bit_Range = ಬೇರು_Declp->bitRange();
        ಧ್ರುವ VNumRange& ತುದಿ_Bit_Range = ತುದಿ_Declp->bitRange();
        ಪ್ರತ್ಯಾಗಮ ಬೇರು_Declp->showname() == ತುದಿ_Declp->showname()
               && ಬೇರು_Declp->declDirection() == ತುದಿ_Declp->declDirection()
               && ಬೇರು_Declp->widthMin() == ತುದಿ_Declp->widthMin()
               && ಬೇರು_Bit_Range.ranged() == ತುದಿ_Bit_Range.ranged()
               && (!ಬೇರು_Bit_Range.ranged()
                   || (ಬೇರು_Bit_Range.ಎಡ() == ತುದಿ_Bit_Range.ಎಡ()
                       && ಬೇರು_Bit_Range.ಬಲ() == ತುದಿ_Bit_Range.ಬಲ()));
    }

    ಶೂನ್ಯ detectDuplicates() {
        UINFO(9, "Finding duplicates");
        // Note uses user4
        V3DupFinder dupFinder;  // Duplicate code detection

        // Compute member offsets within dtype instances incrementally.
        // For each dtype member TraceDecl, stores its code offset within the
        // dtype parent's code range.
        std::unordered_map<ಧ್ರುವ AstTraceDecl*, uint32_t> memberOffsets;
        std::unordered_map<ಧ್ರುವ AstTraceDecl*, uint32_t> runningOffset;

        // For fixup pass: track dups redirected to dtype parents, so we can
        // fix them if the parent's user2 is later set during the loop.
        // Maps dup vertex -> original canonical member vertex.
        std::vector<std::pair<TraceTraceVertex*, TraceTraceVertex*>> dtypeParentRedirects;

        // Hash all of the traced values and find if there are any duplicates
        ಪ್ರತಿ (V3GraphVertex& vtx : m_graph.vertices()) {
            ಯದಿ (TraceTraceVertex* ಧ್ರುವ vvertexp = vtx.cast<TraceTraceVertex>()) {
                AstTraceDecl* ಧ್ರುವ nodep = vvertexp->nodep();
                ಯದಿ (nodep->dtypeCallp()) ಅನುವರ್ತ;

                ಯದಿ (nodep->dtypeDeclp()) {
                    uint32_t& offset = runningOffset[nodep->dtypeDeclp()];
                    memberOffsets[nodep] = offset;
                    offset += nodep->codeInc();
                }

                UASSERT_OBJ(!vvertexp->duplicatep(), nodep, "Should not be a duplicate");
                ಧ್ರುವ auto dupit = dupFinder.findDuplicate(nodep);
                ಯದಿ (dupit == dupFinder.end()) {
                    dupFinder.ಸೇರಿಸು(nodep);
                } ಅನ್ಯಥಾ {
                    AstTraceDecl* ಧ್ರುವ dupDeclp = VN_AS(dupit->second, TraceDecl);
                    UASSERT_OBJ(dupDeclp, nodep, "Trace duplicate of wrong type");
                    TraceTraceVertex* ಧ್ರುವ dupvertexp
                        = dupDeclp->user1u().toGraphVertex()->cast<TraceTraceVertex>();
                    UINFO(8, "  Orig " << dupDeclp << endl);
                    UINFO(8, "   dup " << nodep << endl);

                    ಯದಿ (dupDeclp->dtypeDeclp() && !dupDeclp->dtypeDeclp()->user2()) {
                        AstTraceDecl* ಧ್ರುವ dtypeParentp = dupDeclp->dtypeDeclp();
                        TraceTraceVertex* ಧ್ರುವ dtypeVtxp
                            = dtypeParentp->user1u().toGraphVertex()->cast<TraceTraceVertex>();
                        ಧ್ರುವ auto it2 = memberOffsets.find(dupDeclp);
                        UASSERT_OBJ(it2 != memberOffsets.end(), dupDeclp,
                                    "Member offset not precomputed");
                        vvertexp->duplicatep(dtypeVtxp);
                        vvertexp->dtypeAliasOffset(it2->second);
                        dtypeParentRedirects.emplace_back(vvertexp, dupvertexp);
                    } ಅನ್ಯಥಾ {
                        vvertexp->duplicatep(dupvertexp);
                    }
                    ಯದಿ (nodep->dtypeDeclp()) nodep->dtypeDeclp()->user2(ಸತ್ಯ);
                }
            }
        }

        ಪ್ರತಿ (ಧ್ರುವ auto& pair : dtypeParentRedirects) {
            TraceTraceVertex* ಧ್ರುವ dupVtxp = pair.first;
            TraceTraceVertex* ಧ್ರುವ memberVtxp = pair.second;
            ಧ್ರುವ AstTraceDecl* ಧ್ರುವ parentDeclp = dupVtxp->duplicatep()->nodep();
            ಯದಿ (parentDeclp->user2()) {
                dupVtxp->redirectDuplicatep(memberVtxp);
                dupVtxp->dtypeAliasOffset(0);
            }
        }

        ಯದಿ (dumpLevel() || debug() >= 9)
            dupFinder.dumpFile(v3Global.debugFilename("trace") + ".hash", ಅಸತ್ಯ);
    }

    ಶೂನ್ಯ graphDtypePrune() {
        ಪ್ರತಿ (V3GraphVertex* ಧ್ರುವ vtxp : m_graph.vertices().unlinkable()) {
            ಯದಿ (TraceTraceVertex* ಧ್ರುವ vvertexp = vtxp->cast<TraceTraceVertex>()) {
                AstTraceDecl* ಧ್ರುವ declp = vvertexp->nodep();
                // This skips the dtype sub-func optimization if a var is affected by multiple
                // activities.  We really only need to do this for _chg funcs (and not decls,
                // _const and _full) but it's simpiler to do it all one way or the other.
                ಯದಿ (declp) {
                    ಯದಿ (declp->user2() || (declp->dtypeDeclp() && !declp->dtypeDeclp()->user2())) {
                        AstCCall* ಧ್ರುವ callp = declp->dtypeCallp();
                        ಯದಿ (callp) {
                            Ast_ಗಂಟು* stmtexprp = callp->backp();
                            VL_DO_DANGLING(ತಳ್ಳು_Deletep(stmtexprp->unlinkFrBack()), stmtexprp);
                        } ಅನ್ಯಥಾ {
                            ತಾರ್ಕಿಕ emptyScope;
                            ಕುರು {
                                emptyScope = ಅಸತ್ಯ;
                                Ast_ಗಂಟು* ಧ್ರುವ declBackp = declp->backp();
                                Ast_ಗಂಟು* ಧ್ರುವ declNextp = declp->nextp();
                                ಯದಿ (VN_IS(declBackp, Trace_ತಳ್ಳು_Prefix)
                                    && VN_IS(declNextp, Trace_ತೆಗೆ_Prefix)) {
                                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(declBackp->unlinkFrBack()),
                                                   declBackp);
                                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(declNextp->unlinkFrBack()),
                                                   declNextp);
                                    emptyScope = ಸತ್ಯ;
                                }
                            } ಯಾವತ್ (emptyScope);
                        }
                        // Can't purge until we finish this pass
                        ತಳ್ಳು_Deletep(declp->unlinkFrBack());  // declp used below
                        vvertexp->rerouteEdges(&m_graph);
                        vvertexp->unlinkDelete(&m_graph);
                    }
                    declp->dtypeDeclp(nullptr);
                }
            }
        }
    }

    ಶೂನ್ಯ splitTraceDeclFuncs() {
        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ splitLimit = v3Global.opt.outputSplitCTrace() ? v3Global.opt.outputSplitCTrace()
                                                                : std::numeric_limits<ಪೂರ್ಣಾಙ್ಕ>::max();
        ಯದಿ (splitLimit == std::numeric_limits<ಪೂರ್ಣಾಙ್ಕ>::max()) ಪ್ರತ್ಯಾಗಮ;

        ಪ್ರತಿ (Ast_ಗಂಟು* nodep = m_ತುದಿ_Scopep->blocksp(); nodep; nodep = nodep->nextp()) {
            AstCFunc* ಧ್ರುವ funcp = VN_CAST(nodep, CFunc);
            ಯದಿ (!funcp || !funcp->user2()) ಅನುವರ್ತ;

            ಧ್ರುವ string baseName = funcp->name();
            uint32_t sub_ಅಂಕೆ = 1;
            ಪೂರ್ಣಾಙ್ಕ curSize = 0;
            AstCFunc* curFuncp = funcp;
            std::vector<Ast_ಗಂಟು_Stmt*> callStmts;

            auto startNewSubFunc = [&]() {
                File_ಸಾಲು* ಧ್ರುವ flp = funcp->fileline();
                ಧ್ರುವ string newName = baseName + "_" + cvtToStr(sub_ಅಂಕೆ++);
                AstCFunc* ಧ್ರುವ newFuncp = ನೂತನ AstCFunc{flp, newName, m_ತುದಿ_Scopep};
                newFuncp->argTypes(v3Global.opt.traceClassBase() + "* tracep");
                newFuncp->isTrace(ಸತ್ಯ);
                newFuncp->isStatic(ಅಸತ್ಯ);
                newFuncp->isLoose(ಸತ್ಯ);
                newFuncp->slow(ಸತ್ಯ);
                newFuncp->dontCombine(ಸತ್ಯ);
                m_ತುದಿ_Scopep->addBlocksp(newFuncp);
                newFuncp->addStmtsp(ನೂತನ AstCStmt{flp, "const int c = vlSymsp->__Vm_baseCode;"});
                AstCCall* ಧ್ರುವ callp = ನೂತನ AstCCall{flp, newFuncp};
                callp->dtypeSetVoid();
                callp->argTypes("tracep");
                callStmts.ತಳ್ಳು_back(callp->makeStmt());
                curFuncp = newFuncp;
                curSize = 0;
            };

            ಪ್ರತಿ (Ast_ಗಂಟು *stmtp = funcp->stmtsp(), *nextp; stmtp; stmtp = nextp) {
                nextp = stmtp->nextp();
                curSize += stmtp->ಗಂಟು_ಎಣಿಕೆ();

                ಯದಿ (curFuncp != funcp) {
                    stmtp->unlinkFrBack();
                    curFuncp->addStmtsp(stmtp);
                }

                ಯದಿ (curSize > splitLimit && nextp) startNewSubFunc();
            }

            ಪ್ರತಿ (Ast_ಗಂಟು_Stmt* ಧ್ರುವ callStmtp : callStmts) { funcp->addStmtsp(callStmtp); }
        }
    }

    ಶೂನ್ಯ graphSimplify(ತಾರ್ಕಿಕ initial) {
        ಯದಿ (initial) {
            // Remove all variable nodes
            ಪ್ರತಿ (V3GraphVertex* ಧ್ರುವ vtxp : m_graph.vertices().unlinkable()) {
                ಯದಿ (TraceVarVertex* ಧ್ರುವ vvertexp = vtxp->cast<TraceVarVertex>()) {
                    vvertexp->rerouteEdges(&m_graph);
                    vvertexp->unlinkDelete(&m_graph);
                }
            }
            // Remove multiple variables connecting funcs to traces
            // We do this twice, as then we have fewer edges to multiply out in the below
            // expansion.
            m_graph.removeRedundantEdgesMax(&V3GraphEdge::followAlwaysTrue);
            // Remove all Cfunc nodes
            ಪ್ರತಿ (V3GraphVertex* ಧ್ರುವ vtxp : m_graph.vertices().unlinkable()) {
                ಯದಿ (TraceCFuncVertex* ಧ್ರುವ vvertexp = vtxp->cast<TraceCFuncVertex>()) {
                    vvertexp->rerouteEdges(&m_graph);
                    vvertexp->unlinkDelete(&m_graph);
                }
            }
        }

        // Remove multiple variables connecting funcs to traces
        m_graph.removeRedundantEdgesMax(&V3GraphEdge::followAlwaysTrue);

        // If there are any edges from a always, keep only the always
        ಪ್ರತಿ (V3GraphVertex& vtx : m_graph.vertices()) {
            ಯದಿ (TraceTraceVertex* ಧ್ರುವ vvertexp = vtx.cast<TraceTraceVertex>()) {
                // Search for the incoming always edge
                ಧ್ರುವ V3GraphEdge* alwaysEdgep = nullptr;
                ಪ್ರತಿ (ಧ್ರುವ V3GraphEdge& edge : vvertexp->inEdges()) {
                    ಧ್ರುವ TraceActivityVertex* ಧ್ರುವ actVtxp
                        = edge.fromp()->as<ಧ್ರುವ TraceActivityVertex>();
                    ಯದಿ (actVtxp->activityAlways()) {
                        alwaysEdgep = &edge;
                        ಭಙ್ಗ;
                    }
                }
                // If always edge exists, remove all other edges
                ಯದಿ (alwaysEdgep) {
                    ಪ್ರತಿ (V3GraphEdge* ಧ್ರುವ edgep : vvertexp->inEdges().unlinkable()) {
                        ಯದಿ (edgep != alwaysEdgep) VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
                    }
                }
            }
        }

        // Activity points with no outputs can be removed
        ಪ್ರತಿ (V3GraphVertex* ಧ್ರುವ vtxp : m_graph.vertices().unlinkable()) {
            ಯದಿ (TraceActivityVertex* ಧ್ರುವ aVtxp = vtxp->cast<TraceActivityVertex>()) {
                // Leave in the always vertex for later use.
                ಯದಿ (aVtxp != m_alwaysVtxp && aVtxp->outEmpty()) {
                    VL_DO_DANGLING(aVtxp->unlinkDelete(&m_graph), aVtxp);
                }
            }
        }
    }

    uint32_t assignactivityNumbers() {
        uint32_t activity_ಸಂಖ್ಯೆ = 1;  // Note 0 indicates "slow" only
        ಪ್ರತಿ (V3GraphVertex& vtx : m_graph.vertices()) {
            ಯದಿ (TraceActivityVertex* ಧ್ರುವ vvertexp = vtx.cast<TraceActivityVertex>()) {
                ಯದಿ (vvertexp != m_alwaysVtxp) {
                    ಯದಿ (vvertexp->slow()) {
                        vvertexp->activityCode(TraceActivityVertex::ACTIVITY_SLOW);
                    } ಅನ್ಯಥಾ {
                        vvertexp->activityCode(activity_ಸಂಖ್ಯೆ++);
                    }
                }
            }
        }
        ಪ್ರತ್ಯಾಗಮ activity_ಸಂಖ್ಯೆ;
    }

    ಶೂನ್ಯ sortTraces(TraceVec& traces, uint32_t& nNonConstCodes) {
        // Populate sort structure
        traces.clear();
        nNonConstCodes = 0;
        ಪ್ರತಿ (V3GraphVertex& vtx : m_graph.vertices()) {
            ಯದಿ (TraceTraceVertex* ಧ್ರುವ vtxp = vtx.cast<TraceTraceVertex>()) {
                ActCodeSet actSet;
                UINFO(9, "  Add to sort: " << vtxp);
                UINFOTREE(9, vtxp->nodep(), "", "trnode");
                ಪ್ರತಿ (ಧ್ರುವ V3GraphEdge& edge : vtxp->inEdges()) {
                    ಧ್ರುವ TraceActivityVertex* ಧ್ರುವ cfvertexp
                        = edge.fromp()->cast<ಧ್ರುವ TraceActivityVertex>();
                    UASSERT_OBJ(cfvertexp, vtxp->nodep(),
                                "Should have been function pointing to this trace");
                    UINFO(9, "   Activity: " << cfvertexp);
                    ಯದಿ (cfvertexp->activityAlways()) {
                        // If code 0, we always trace; ignore other codes
                        actSet.ಸೇರಿಸು(TraceActivityVertex::ACTIVITY_ALWAYS);
                    } ಅನ್ಯಥಾ {
                        actSet.ಸೇರಿಸು(cfvertexp->activityCode());
                    }
                }
                UASSERT_OBJ(actSet.ಎಣಿಕೆ(TraceActivityVertex::ACTIVITY_ALWAYS) == 0
                                || actSet.size() == 1,
                            vtxp->nodep(), "Always active trace has further triggers");
                // Count nodes
                ಯದಿ (!vtxp->duplicatep() && !actSet.empty())
                    nNonConstCodes += vtxp->nodep()->codeInc();
                ಯದಿ (actSet.empty()) {
                    // If a trace doesn't have activity, it's constant, and we
                    // don't need to track changes on it.
                    actSet.ಸೇರಿಸು(TraceActivityVertex::ACTIVITY_NEVER);
                } ಅನ್ಯಥಾ ಯದಿ (actSet.ಎಣಿಕೆ(TraceActivityVertex::ACTIVITY_SLOW) && actSet.size() > 1) {
                    // If a trace depends on the slow flag as well as other
                    // flags, remove the dependency on the slow flag. We will
                    // make slow routines set all activity flags.
                    actSet.erase(TraceActivityVertex::ACTIVITY_SLOW);
                }
                traces.emplace(actSet, vtxp);
            }
        }
    }

    ಶೂನ್ಯ graphOptimize() {
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
        ಧ್ರುವ auto end = traces.end();
        ಯಾವತ್ (it != end) {
            auto head = it;
            // Approximate the complexity of the value change check
            uint32_t complexity = 0;
            ಧ್ರುವ ActCodeSet& actSet = it->first;
            ಪ್ರತಿ (; it != end && it->first == actSet; ++it) {
                ಯದಿ (!it->second->duplicatep()) {
                    uint32_t cost = 0;
                    ಧ್ರುವ AstTraceDecl* ಧ್ರುವ declp = it->second->nodep();
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
            ಯದಿ (actSet.ಎಣಿಕೆ(TraceActivityVertex::ACTIVITY_ALWAYS)) ಅನುವರ್ತ;
            ಯದಿ (actSet.ಎಣಿಕೆ(TraceActivityVertex::ACTIVITY_NEVER)) ಅನುವರ್ತ;
            ಯದಿ (actSet.ಎಣಿಕೆ(TraceActivityVertex::ACTIVITY_SLOW)) ಅನುವರ್ತ;
            // If the value comparisons are cheaper to perform than checking the
            // activity flags make the signals always traced. Note this cost
            // equation is heuristic.
            ಯದಿ (complexity <= actSet.size() * 2) {
                ಪ್ರತಿ (; head != it; ++head) {
                    ನೂತನ V3GraphEdge{&m_graph, m_alwaysVtxp, head->second, 1};
                }
            }
        }

        graphSimplify(ಅಸತ್ಯ);
    }

    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* selectActivity(File_ಸಾಲು* flp, uint32_t acode, ಧ್ರುವ VAccess& access) {
        ಪ್ರತ್ಯಾಗಮ ನೂತನ AstArraySel(flp, ನೂತನ AstVarRef{flp, m_activityVscp, access}, acode);
    }

    Ast_ಗಂಟು* newActivitySetter(Ast_ಗಂಟು* insertp, uint32_t code) {
        ++m_statSetters;
        File_ಸಾಲು* ಧ್ರುವ fl = insertp->fileline();
        AstAssign* ಧ್ರುವ setterp = ನೂತನ AstAssign{fl, selectActivity(fl, code, VAccess::WRITE),
                                                 ನೂತನ AstConst{fl, AstConst::BitTrue{}}};
        ಪ್ರತ್ಯಾಗಮ setterp;
    }

    Ast_ಗಂಟು* newActivityAll(Ast_ಗಂಟು* insertp) {
        ++m_statSettersSlow;
        ಯದಿ (!m_actAllFuncp) {
            File_ಸಾಲು* ಧ್ರುವ flp = m_ತುದಿ_Scopep->fileline();
            AstCFunc* ಧ್ರುವ funcp = ನೂತನ AstCFunc{flp, "__VtraceActivitySetAll", m_ತುದಿ_Scopep};
            funcp->slow(ಸತ್ಯ);
            funcp->isStatic(ಅಸತ್ಯ);
            funcp->isLoose(ಸತ್ಯ);
            m_ತುದಿ_Scopep->addBlocksp(funcp);
            ಪ್ರತಿ (uint32_t code = 0; code < m_activity_ಸಂಖ್ಯೆ; ++code) {
                Ast_ಗಂಟು* ಧ್ರುವ setterp = newActivitySetter(insertp, code);
                funcp->addStmtsp(setterp);
            }
            m_actAllFuncp = funcp;
        }
        AstCCall* ಧ್ರುವ callp = ನೂತನ AstCCall{insertp->fileline(), m_actAllFuncp};
        callp->dtypeSetVoid();
        ಪ್ರತ್ಯಾಗಮ callp->makeStmt();
    }

    ಶೂನ್ಯ createActivityFlags() {
        // Assign final activity numbers
        m_activity_ಸಂಖ್ಯೆ = assignactivityNumbers();

        // Create an array of bytes, not a bit vector, as they can be set
        // atomically by mtasks, and are cheaper to set (no need for
        // read-modify-write on the C type), and the speed of the tracing code
        // is the same on largish designs.
        File_ಸಾಲು* ಧ್ರುವ flp = m_ತುದಿ_Scopep->fileline();
        Ast_ಗಂಟು_DType* ಧ್ರುವ newScalarDtp = ನೂತನ AstBasicDType{flp, VFlagBitPacked{}, 1};
        v3Global.rootp()->typeTablep()->addTypesp(newScalarDtp);
        AstRange* ಧ್ರುವ newArange
            = ನೂತನ AstRange{flp, VNumRange{static_cast<ಪೂರ್ಣಾಙ್ಕ>(m_activity_ಸಂಖ್ಯೆ) - 1, 0}};
        Ast_ಗಂಟು_DType* ಧ್ರುವ newArrDtp = ನೂತನ AstUnpackArrayDType{flp, newScalarDtp, newArange};
        v3Global.rootp()->typeTablep()->addTypesp(newArrDtp);
        m_activityVscp = v3Global.rootp()->ತುದಿ_Scopep()->createTemp("__VtraceActivity", newArrDtp);

        // Insert activity setters
        ಪ್ರತಿ (ಧ್ರುವ V3GraphVertex& vtx : m_graph.vertices()) {
            ಯದಿ (ಧ್ರುವ TraceActivityVertex* ಧ್ರುವ vtxp = vtx.cast<ಧ್ರುವ TraceActivityVertex>()) {
                Ast_ಗಂಟು* setterp = nullptr;
                ಯದಿ (vtxp->activitySlow()) {
                    setterp = newActivityAll(vtxp->insertp());
                    // Just set all flags in slow code as it should be rare.
                    // This will be rolled up into a loop by V3Reloop.
                } ಅನ್ಯಥಾ ಯದಿ (!vtxp->activityAlways()) {
                    setterp = newActivitySetter(vtxp->insertp(), vtxp->activityCode());
                }
                ಯದಿ (setterp) {
                    Ast_ಗಂಟು* ಧ್ರುವ insertp = vtxp->insertp();
                    ಯದಿ (Ast_Stmt_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ stmtp = VN_CAST(insertp, Stmt_ಅಭಿವ್ಯಕ್ತಿ)) {
                        stmtp->addNextHere(setterp);
                    } ಅನ್ಯಥಾ ಯದಿ (AstCFunc* ಧ್ರುವ funcp = VN_CAST(insertp, CFunc)) {
                        // If there are awaits, insert the setter after each await
                        ಯದಿ (funcp->isCoroutine() && funcp->stmtsp()) {
                            funcp->stmtsp()->foreachAndNext([setterp](AstCAwait* awaitp) {
                                awaitp->addNextHere(setterp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ));
                            });
                        }
                        funcp->addStmtsp(setterp);
                    } ಅನ್ಯಥಾ {
                        insertp->v3fatalSrc("Bad trace activity vertex");
                    }
                }
            }
        }
    }

    AstCFunc* newCFunc(VTraceType traceType, AstCFunc* ತುದಿ_Funcp, uint32_t func_ಅಂಕೆ,
                       uint32_t baseCode = 0, ಧ್ರುವ AstTraceDecl* ಧ್ರುವ declp = nullptr,
                       ತಾರ್ಕಿಕ declSub = ಅಸತ್ಯ) {
        // Create new function
        ಧ್ರುವ ತಾರ್ಕಿಕ is_ತುದಿ_Func = !declp && ತುದಿ_Funcp == nullptr;
        std::string funcName;
        ಯದಿ (is_ತುದಿ_Func || declp) {
            funcName = traceType.func_prefix();
        } ಅನ್ಯಥಾ {
            funcName = ತುದಿ_Funcp->name();
            funcName += "_sub";
        }
        ಯದಿ (declp) {
            funcName += "_dtype";
            ಯದಿ (declSub) funcName += "_sub";
            funcName += "__";
            funcName = m_dtypeNames.get(funcName);
        } ಅನ್ಯಥಾ {
            funcName += "_";
            funcName += cvtToStr(func_ಅಂಕೆ);
        }

        File_ಸಾಲು* ಧ್ರುವ flp = m_ತುದಿ_Scopep->fileline();
        AstCFunc* ಧ್ರುವ funcp = ನೂತನ AstCFunc{flp, funcName, m_ತುದಿ_Scopep};
        funcp->isTrace(ಸತ್ಯ);
        funcp->dontCombine(ಸತ್ಯ);
        funcp->isLoose(ಸತ್ಯ);
        funcp->slow(traceType != VTraceType::CHANGE);
        funcp->isStatic(is_ತುದಿ_Func);
        // Add it to top scope
        m_ತುದಿ_Scopep->addBlocksp(funcp);
        ಧ್ರುವ std::string bufArg
            = v3Global.opt.traceClassBase() + "::Buffer* bufp"
              + (declp ? (", uint32_t offset, const " + declp->dtypep()->cType("", ಸತ್ಯ, ಸತ್ಯ)
                          + " __VdtypeVar")
                       : "");
        ಯದಿ (is_ತುದಿ_Func) {
            // Top functions
            funcp->argTypes("void* voidSelf, " + bufArg);
            funcp->addStmtsp(ನೂತನ AstCStmt{flp, EmitCUtil::voidSelfAssign(m_ತುದಿ_Modp)});
            funcp->addStmtsp(ನೂತನ AstCStmt{flp, EmitCUtil::symClassAssign()});
            // Add global activity check to change dump functions
            ಯದಿ (traceType == VTraceType::CHANGE) {  //
                funcp->addStmtsp(
                    ನೂತನ AstCStmt{flp, "if (VL_UNLIKELY(!vlSymsp->__Vm_activity)) return;"});
            }
            // Register function
            AstCStmt* ಧ್ರುವ cstmtp = ನೂತನ AstCStmt{flp};
            m_regFuncp->addStmtsp(cstmtp);
            ಯದಿ (traceType == VTraceType::CONSTANT) {
                cstmtp->add("tracep->addConstCb(");
                cstmtp->add(ನೂತನ AstAddrOfCFunc{flp, funcp});
                cstmtp->add(", " + std::to_string(func_ಅಂಕೆ) + ", vlSelf);");
            } ಅನ್ಯಥಾ ಯದಿ (traceType == VTraceType::FULL) {
                cstmtp->add("tracep->addFullCb(");
                cstmtp->add(ನೂತನ AstAddrOfCFunc{flp, funcp});
                cstmtp->add(", " + std::to_string(func_ಅಂಕೆ) + ", vlSelf);");
            } ಅನ್ಯಥಾ {
                cstmtp->add("tracep->addChgCb(");
                cstmtp->add(ನೂತನ AstAddrOfCFunc{flp, funcp});
                cstmtp->add(", " + std::to_string(func_ಅಂಕೆ) + ", vlSelf);");
            }
        } ಅನ್ಯಥಾ {
            // Sub functions
            funcp->argTypes(bufArg);
            // Setup base references. Note in rare occasions we can end up with an empty trace
            // sub function, hence the VL_ATTR_UNUSED attributes.
            ಯದಿ (traceType != VTraceType::CHANGE) {
                // Full dump sub function
                funcp->addStmtsp(ನೂತನ AstCStmt{flp,  //
                                              string("uint32_t* const oldp VL_ATTR_UNUSED = "
                                                     "bufp->oldp(vlSymsp->__Vm_baseCode")
                                                  + (declp ? " + offset" : "") + ");\n"});
            } ಅನ್ಯಥಾ {
                // Change dump sub function
                funcp->addStmtsp(ನೂತನ AstCStmt{flp,  //
                                              "uint32_t* const oldp VL_ATTR_UNUSED = "
                                              "bufp->oldp(vlSymsp->__Vm_baseCode + "
                                                  + (declp ? " offset" : cvtToStr(baseCode))
                                                  + ");\n"});
            }
            ಯದಿ (!declp) {
                // Add call to top function
                AstCCall* ಧ್ರುವ callp = ನೂತನ AstCCall{funcp->fileline(), funcp};
                callp->dtypeSetVoid();
                callp->argTypes("bufp");
                ತುದಿ_Funcp->addStmtsp(callp->makeStmt());
            }
        }
        // Done
        UINFO(5, "  newCFunc " << funcp);
        ಪ್ರತ್ಯಾಗಮ funcp;
    }

    AstCFunc* newCDtypeSubFunc(VTraceType traceType, ಧ್ರುವ AstTraceDecl* ಧ್ರುವ declp,
                               AstCFunc* parentp) {
        AstCFunc* ಧ್ರುವ funcp = newCFunc(traceType, nullptr, 0, 0, declp, ಸತ್ಯ);
        File_ಸಾಲು* ಧ್ರುವ flp = m_ತುದಿ_Scopep->fileline();
        AstCCall* ಧ್ರುವ callp = ನೂತನ AstCCall{flp, funcp};
        callp->dtypeSetVoid();
        callp->argTypes("bufp, offset, __VdtypeVar");
        parentp->addStmtsp(callp->makeStmt());
        ಪ್ರತ್ಯಾಗಮ funcp;
    }

    AstCFunc* createConstDtypeTraceFunctions(ಧ್ರುವ AstTraceDecl* declp) {
        ಧ್ರುವ Ast_ಗಂಟು_DType* ಧ್ರುವ dtypep = declp->dtypep()->skipRefp();
        auto pair = m_dtypeConstFuncs.emplace(dtypep, nullptr);
        ಯದಿ (pair.second) {
            File_ಸಾಲು* ಧ್ರುವ flp = declp->fileline();
            AstCFunc* ಧ್ರುವ funcp = newCFunc(VTraceType::CONSTANT, nullptr, 0, 0, declp);

            ತಾರ್ಕಿಕ first = ಸತ್ಯ;
            ಪ್ರತಿ (Ast_ಗಂಟು* callStmtp = declp->dtypeCallp()->funcp()->stmtsp(); callStmtp;
                 callStmtp = callStmtp->nextp()) {
                ಧ್ರುವ AstCCall* ಧ್ರುವ callp
                    = VN_CAST(VN_CAST(callStmtp, Stmt_ಅಭಿವ್ಯಕ್ತಿ)->exprp(), CCall);
                ತಾರ್ಕಿಕ onlyOne = first && !callStmtp->nextp();
                AstCFunc* ಧ್ರುವ subFuncp
                    = onlyOne ? funcp : newCDtypeSubFunc(VTraceType::CONSTANT, declp, funcp);
                ಪ್ರತಿ (Ast_ಗಂಟು* stmtp = callp->funcp()->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    ಯದಿ (AstTraceDecl* ಧ್ರುವ fieldDeclp = VN_CAST(stmtp, TraceDecl)) {
                        AstTraceInc* ಧ್ರುವ incp
                            = ನೂತನ AstTraceInc{flp, fieldDeclp, VTraceType::CONSTANT};
                        subFuncp->addStmtsp(incp);
                    }
                }
                first = ಅಸತ್ಯ;
            }

            pair.first->second = funcp;
        }

        ಪ್ರತ್ಯಾಗಮ pair.first->second;
    }

    ಶೂನ್ಯ createConstTraceFunctions(ಧ್ರುವ TraceVec& traces) {
        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ splitLimit = v3Global.opt.outputSplitCTrace() ? v3Global.opt.outputSplitCTrace()
                                                                : std::numeric_limits<ಪೂರ್ಣಾಙ್ಕ>::max();

        AstCFunc* ಧ್ರುವ ತುದಿ_Funcp = newCFunc(VTraceType::CONSTANT, nullptr, 0);
        uint32_t sub_Func_ಅಂಕೆ = 0;
        AstCFunc* subFuncp = nullptr;
        ಪೂರ್ಣಾಙ್ಕ subStmts = 0;
        std::vector<ಧ್ರುವ TraceTraceVertex*> duplicates;
        ಪ್ರತಿ (auto it = traces.cbegin(); it != traces.end(); ++it) {
            ಧ್ರುವ TraceTraceVertex* ಧ್ರುವ vtxp = it->second;
            AstTraceDecl* ಧ್ರುವ declp = vtxp->nodep();
            ಯದಿ (vtxp->duplicatep()) {
                duplicates.ತಳ್ಳು_back(vtxp);
                ಅನುವರ್ತ;
            }

            // This is a canonical trace node. Assign trace code (signal number).
            UASSERT_OBJ(!declp->codeAssigned(), declp,
                        "Canonical node should not have code assigned yet");
            declp->code(m_code);
            ಧ್ರುವ uint32_t codeInc = declp->codeInc();
            m_code += codeInc;
            ++m_statUniqSigs;

            // If this is a const signal, add the AstTraceInc
            ಧ್ರುವ ActCodeSet& actSet = it->first;
            ಯದಿ (actSet.ಎಣಿಕೆ(TraceActivityVertex::ACTIVITY_NEVER)) {
                // Crate new sub function if required
                ಯದಿ (!subFuncp || subStmts > splitLimit) {
                    subStmts = 0;
                    subFuncp = newCFunc(VTraceType::CONSTANT, ತುದಿ_Funcp, sub_Func_ಅಂಕೆ);
                    ++sub_Func_ಅಂಕೆ;
                }
                File_ಸಾಲು* ಧ್ರುವ flp = declp->fileline();
                ಯದಿ (declp->dtypeCallp()) {
                    AstCFunc* ಧ್ರುವ funcp = createConstDtypeTraceFunctions(declp);
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* argsp = nullptr;
                    argsp = Ast_ಗಂಟು::addNext(argsp, declp->valuep()->clone_ವೃಕ್ಷ(ಅಸತ್ಯ));
                    AstCCall* ಧ್ರುವ callp = ನೂತನ AstCCall{flp, funcp, argsp};
                    callp->dtypeSetVoid();
                    callp->argTypes(callp->argTypes() + "bufp, " + std::to_string(declp->code()));
                    subFuncp->addStmtsp(callp->makeStmt());

                    subStmts += 1;
                } ಅನ್ಯಥಾ {
                    AstTraceInc* ಧ್ರುವ incp = ನೂತನ AstTraceInc{flp, declp, VTraceType::CONSTANT};
                    subFuncp->addStmtsp(incp);
                    subStmts += incp->ಗಂಟು_ಎಣಿಕೆ();
                }
            }
        }

        // Assign codes to duplicate nodes from their canonicals
        ಪ್ರತಿ (ಧ್ರುವ TraceTraceVertex* ಧ್ರುವ vtxp : duplicates) {
            AstTraceDecl* ಧ್ರುವ declp = vtxp->nodep();
            ಧ್ರುವ TraceTraceVertex* ಧ್ರುವ canonVtxp = vtxp->duplicatep();
            ಧ್ರುವ AstTraceDecl* ಧ್ರುವ canonDeclp = canonVtxp->nodep();
            UASSERT_OBJ(!canonVtxp->duplicatep(), canonDeclp, "Canonical node is a duplicate");
            UASSERT_OBJ(canonDeclp->codeAssigned(), canonDeclp,
                        "Canonical node should have code assigned already");
            declp->code(canonDeclp->code() + vtxp->dtypeAliasOffset());
        }
    }

    DtypeFuncs createNonConstDtypeTraceFunctions(ಧ್ರುವ AstTraceDecl* declp) {
        Ast_ಗಂಟು_DType* dtypep = declp->dtypep()->skipRefp();
        auto pair = m_dtypeNonConstFuncs.emplace(dtypep, DtypeFuncs{});
        ಯದಿ (pair.second) {
            File_ಸಾಲು* ಧ್ರುವ flp = declp->fileline();
            AstCFunc* ಧ್ರುವ fullFuncp = newCFunc(VTraceType::FULL, nullptr, 0, 0, declp);
            AstCFunc* ಧ್ರುವ chgFuncp = newCFunc(VTraceType::CHANGE, nullptr, 0, 0, declp);

            ತಾರ್ಕಿಕ first = ಸತ್ಯ;
            ಪ್ರತಿ (Ast_ಗಂಟು* callStmtp = declp->dtypeCallp()->funcp()->stmtsp(); callStmtp;
                 callStmtp = callStmtp->nextp()) {
                ಧ್ರುವ AstCCall* ಧ್ರುವ callp
                    = VN_CAST(VN_CAST(callStmtp, Stmt_ಅಭಿವ್ಯಕ್ತಿ)->exprp(), CCall);
                ತಾರ್ಕಿಕ onlyOne = first && !callStmtp->nextp();
                AstCFunc* ಧ್ರುವ fullSubFuncp
                    = onlyOne ? fullFuncp : newCDtypeSubFunc(VTraceType::FULL, declp, fullFuncp);
                AstCFunc* ಧ್ರುವ chgSubFuncp
                    = onlyOne ? chgFuncp : newCDtypeSubFunc(VTraceType::CHANGE, declp, chgFuncp);
                ಪ್ರತಿ (Ast_ಗಂಟು* stmtp = callp->funcp()->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    ಯದಿ (AstTraceDecl* ಧ್ರುವ fieldDeclp = VN_CAST(stmtp, TraceDecl)) {
                        AstTraceInc* ಧ್ರುವ incFullp
                            = ನೂತನ AstTraceInc{flp, fieldDeclp, VTraceType::FULL};
                        fullSubFuncp->addStmtsp(incFullp);
                        AstTraceInc* ಧ್ರುವ incChgp
                            = ನೂತನ AstTraceInc{flp, fieldDeclp, VTraceType::CHANGE};
                        chgSubFuncp->addStmtsp(incChgp);
                    }
                }
                first = ಅಸತ್ಯ;
            }

            pair.first->second = {.fullFuncp = fullFuncp, .chgFuncp = chgFuncp};
        }

        ಪ್ರತ್ಯಾಗಮ pair.first->second;
    }

    ಶೂನ್ಯ createNonConstTraceFunctions(ಧ್ರುವ TraceVec& traces, uint32_t nAllCodes,
                                      uint32_t parallelism) {
        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ splitLimit = v3Global.opt.outputSplitCTrace() ? v3Global.opt.outputSplitCTrace()
                                                                : std::numeric_limits<ಪೂರ್ಣಾಙ್ಕ>::max();

        // pre-incremented, so starts at 0
        uint32_t ತುದಿ_Func_ಅಂಕೆ = std::numeric_limits<uint32_t>::max();
        TraceVec::const_iterator it = traces.begin();
        ಯಾವತ್ (it != traces.end()) {
            AstCFunc* ತುದಿ_Ful_Funcp = nullptr;
            AstCFunc* ತುದಿ_Chg_Funcp = nullptr;
            AstCFunc* subFulFuncp = nullptr;
            AstCFunc* subChgFuncp = nullptr;
            uint32_t sub_Func_ಅಂಕೆ = 0;
            ಪೂರ್ಣಾಙ್ಕ subStmts = 0;
            ಧ್ರುವ uint32_t maxCodes = std::max((nAllCodes + parallelism - 1) / parallelism, 1U);
            uint32_t nCodes = 0;
            ಧ್ರುವ ActCodeSet* prevActSet = nullptr;
            AstIf* ifp = nullptr;
            uint32_t baseCode = 0;
            ಪ್ರತಿ (; nCodes < maxCodes && it != traces.end(); ++it) {
                ಧ್ರುವ ActCodeSet& actSet = it->first;
                // Traced value never changes, no need to add it
                ಯದಿ (actSet.ಎಣಿಕೆ(TraceActivityVertex::ACTIVITY_NEVER)) ಅನುವರ್ತ;

                ಧ್ರುವ TraceTraceVertex* ಧ್ರುವ vtxp = it->second;
                AstTraceDecl* ಧ್ರುವ declp = vtxp->nodep();

                // This is a duplicate decl, no need to add it, but must set the
                // function index to the same as the canonical node.
                ಯದಿ (ಧ್ರುವ TraceTraceVertex* ಧ್ರುವ canonVtxp = vtxp->duplicatep()) {
                    declp->fidx(canonVtxp->nodep()->fidx());
                    ಅನುವರ್ತ;
                }

                // Create top function if not yet created
                ಯದಿ (!ತುದಿ_Ful_Funcp) {
                    ++ತುದಿ_Func_ಅಂಕೆ;
                    ತುದಿ_Ful_Funcp = newCFunc(VTraceType::FULL, nullptr, ತುದಿ_Func_ಅಂಕೆ);
                    ತುದಿ_Chg_Funcp = newCFunc(VTraceType::CHANGE, nullptr, ತುದಿ_Func_ಅಂಕೆ);
                }

                // Create new sub function if required
                ಯದಿ (!subFulFuncp || subStmts > splitLimit) {
                    baseCode = declp->code();
                    subStmts = 0;
                    subFulFuncp = newCFunc(VTraceType::FULL, ತುದಿ_Ful_Funcp, sub_Func_ಅಂಕೆ, baseCode);
                    subChgFuncp = newCFunc(VTraceType::CHANGE, ತುದಿ_Chg_Funcp, sub_Func_ಅಂಕೆ, baseCode);
                    ++sub_Func_ಅಂಕೆ;
                    prevActSet = nullptr;
                    ifp = nullptr;
                }

                // If required, create the conditional node checking the activity flags
                ಯದಿ (!prevActSet || actSet != *prevActSet) {
                    File_ಸಾಲು* ಧ್ರುವ flp = m_ತುದಿ_Scopep->fileline();
                    ಧ್ರುವ ತಾರ್ಕಿಕ always = actSet.ಎಣಿಕೆ(TraceActivityVertex::ACTIVITY_ALWAYS) != 0;
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* condp = nullptr;
                    ಯದಿ (always) {
                        condp = ನೂತನ AstConst{flp, 1};  // Always true, will be folded later
                    } ಅನ್ಯಥಾ {
                        ಪ್ರತಿ (ಧ್ರುವ uint32_t actCode : actSet) {
                            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ selp = selectActivity(flp, actCode, VAccess::READ);
                            condp = condp ? ನೂತನ AstOr{flp, condp, selp} : selp;
                        }
                    }
                    ifp = ನೂತನ AstIf{flp, condp};
                    ಯದಿ (!always) ifp->branchPred(VBranchPred::BP_UNLIKELY);
                    subChgFuncp->addStmtsp(ifp);
                    subStmts += ifp->ಗಂಟು_ಎಣಿಕೆ();
                    prevActSet = &actSet;
                }

                // Add TraceInc nodes
                File_ಸಾಲು* ಧ್ರುವ flp = declp->fileline();
                ಯದಿ (declp->dtypeCallp()) {
                    DtypeFuncs funcs = createNonConstDtypeTraceFunctions(declp);
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* argsp = nullptr;
                    argsp = Ast_ಗಂಟು::addNext(argsp, declp->valuep()->clone_ವೃಕ್ಷ(ಅಸತ್ಯ));
                    AstCCall* ಧ್ರುವ callFullp = ನೂತನ AstCCall{flp, funcs.fullFuncp, argsp};
                    callFullp->dtypeSetVoid();
                    callFullp->argTypes(callFullp->argTypes() + "bufp, "
                                        + std::to_string(declp->code()));
                    subFulFuncp->addStmtsp(callFullp->makeStmt());
                    argsp = nullptr;
                    argsp = Ast_ಗಂಟು::addNext(argsp, declp->valuep()->clone_ವೃಕ್ಷ(ಅಸತ್ಯ));
                    AstCCall* ಧ್ರುವ callChgp = ನೂತನ AstCCall{flp, funcs.chgFuncp, argsp};
                    callChgp->dtypeSetVoid();
                    callChgp->argTypes(callChgp->argTypes() + "bufp, "
                                       + std::to_string(declp->code()));
                    ifp->addThensp(callChgp->makeStmt());

                    subStmts += 2;
                } ಅನ್ಯಥಾ {
                    AstTraceInc* ಧ್ರುವ incFulp = ನೂತನ AstTraceInc{flp, declp, VTraceType::FULL};
                    subFulFuncp->addStmtsp(incFulp);
                    AstTraceInc* ಧ್ರುವ incChgp
                        = ನೂತನ AstTraceInc{flp, declp, VTraceType::CHANGE, baseCode};
                    ifp->addThensp(incChgp);

                    // Set the function index of the decl
                    declp->fidx(ತುದಿ_Func_ಅಂಕೆ);

                    // Track splitting due to size
                    UASSERT_OBJ(incFulp->ಗಂಟು_ಎಣಿಕೆ() == incChgp->ಗಂಟು_ಎಣಿಕೆ(), declp,
                                "Should have equal cost");
                    ಧ್ರುವ VNumRange range = declp->arrayRange();
                    ಯದಿ (range.ranged()) {
                        // 2x because each element is a TraceInc and a VarRef
                        subStmts += range.elements() * 2;
                    } ಅನ್ಯಥಾ {
                        subStmts += incChgp->ಗಂಟು_ಎಣಿಕೆ();
                    }
                }

                // Track partitioning
                nCodes += declp->codeInc();
            }
        }
    }

    ಶೂನ್ಯ createCleanupFunction() {
        File_ಸಾಲು* ಧ್ರುವ fl = m_ತುದಿ_Scopep->fileline();
        AstCFunc* ಧ್ರುವ cleanupFuncp = ನೂತನ AstCFunc{fl, "trace_cleanup", m_ತುದಿ_Scopep};
        cleanupFuncp->argTypes("void* voidSelf, " + v3Global.opt.traceClassBase()
                               + "* /*unused*/");
        cleanupFuncp->isTrace(ಸತ್ಯ);
        cleanupFuncp->slow(ಅಸತ್ಯ);
        cleanupFuncp->isStatic(ಸತ್ಯ);
        cleanupFuncp->isLoose(ಸತ್ಯ);
        m_ತುದಿ_Scopep->addBlocksp(cleanupFuncp);
        cleanupFuncp->addStmtsp(ನೂತನ AstCStmt{fl, EmitCUtil::voidSelfAssign(m_ತುದಿ_Modp)});
        cleanupFuncp->addStmtsp(ನೂತನ AstCStmt{fl, EmitCUtil::symClassAssign()});

        // Register it
        {
            AstCStmt* ಧ್ರುವ cstmtp = ನೂತನ AstCStmt{fl};
            m_regFuncp->addStmtsp(cstmtp);
            cstmtp->add("tracep->addCleanupCb(");
            cstmtp->add(ನೂತನ AstAddrOfCFunc{fl, cleanupFuncp});
            cstmtp->add(", vlSelf);");
        }

        // Clear global activity flag
        cleanupFuncp->addStmtsp(
            ನೂತನ AstCStmt{m_ತುದಿ_Scopep->fileline(), "vlSymsp->__Vm_activity = false;"s});

        // Clear fine grained activity flags
        ಪ್ರತಿ (uint32_t i = 0; i < m_activity_ಸಂಖ್ಯೆ; ++i) {
            Ast_ಗಂಟು* ಧ್ರುವ clrp = ನೂತನ AstAssign{fl, selectActivity(fl, i, VAccess::WRITE),
                                                ನೂತನ AstConst{fl, AstConst::BitFalse{}}};
            cleanupFuncp->addStmtsp(clrp);
        }
    }

    ಶೂನ್ಯ createTraceFunctions() {
        // Detect and remove duplicate values
        detectDuplicates();

        graphDtypePrune();
        splitTraceDeclFuncs();
        m_graph.removeRedundantEdgesMax(&V3GraphEdge::followAlwaysTrue);

        // Simplify & optimize the graph
        ಯದಿ (dumpGraphLevel() >= 6) m_graph.dumpDotFilePrefixed("trace_pre");
        graphSimplify(ಸತ್ಯ);
        ಯದಿ (dumpGraphLevel() >= 6) m_graph.dumpDotFilePrefixed("trace_simplified");
        graphOptimize();
        ಯದಿ (dumpGraphLevel() >= 6) m_graph.dumpDotFilePrefixed("trace_optimized");

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
        m_regFuncp = ನೂತನ AstCFunc{m_ತುದಿ_Scopep->fileline(), "trace_register", m_ತುದಿ_Scopep};
        m_regFuncp->argTypes(v3Global.opt.traceClassBase() + "* tracep");
        m_regFuncp->entryPoint(ಸತ್ಯ);
        m_regFuncp->isTrace(ಸತ್ಯ);
        m_regFuncp->slow(ಸತ್ಯ);
        m_regFuncp->isStatic(ಅಸತ್ಯ);
        m_regFuncp->isLoose(ಸತ್ಯ);
        m_ತುದಿ_Scopep->addBlocksp(m_regFuncp);

        // Create the const dump functions. Also allocates trace codes.
        createConstTraceFunctions(traces);

        // Create the full and incremental dump functions
        createNonConstTraceFunctions(traces, nNonConstCodes, m_parallelism);

        // Root-traced libraries alias wrapper IOs onto the existing top-module codes.
        ಯದಿ (!v3Global.opt.libCreate().empty()) {
            std::vector<AstTraceDecl*> ಬೇರು_Declps;
            std::vector<AstTraceDecl*> ತುದಿ_Declps;
            TraceInitDeclCollector ಬೇರು_Collector{ಬೇರು_Declps};
            TraceInitDeclCollector ತುದಿ_Collector{ತುದಿ_Declps};
            ಪ್ರತಿ (Ast_ಗಂಟು* blockp = m_ತುದಿ_Scopep->blocksp(); blockp; blockp = blockp->nextp()) {
                AstCFunc* ಧ್ರುವ funcp = VN_CAST(blockp, CFunc);
                ಯದಿ (funcp && VString::startsWith(funcp->name(), "trace_init_leaf_root__")) {
                    ಬೇರು_Collector.collect(funcp);
                } ಅನ್ಯಥಾ ಯದಿ (funcp && VString::startsWith(funcp->name(), "trace_init_leaf_top__")) {
                    ತುದಿ_Collector.collect(funcp);
                }
            }

            std::vector<ತಾರ್ಕಿಕ> used(ತುದಿ_Declps.size(), ಅಸತ್ಯ);
            ಪ್ರತಿ (AstTraceDecl* ಧ್ರುವ ಬೇರು_Declp : ಬೇರು_Declps) {
                ಪ್ರತಿ (size_t i = 0; i < ತುದಿ_Declps.size(); ++i) {
                    ಯದಿ (used[i] || !same_ಬೇರು_Init_Alias(ಬೇರು_Declp, ತುದಿ_Declps[i])) ಅನುವರ್ತ;
                    ಬೇರು_Declp->code(ತುದಿ_Declps[i]->code());
                    used[i] = ಸತ್ಯ;
                    ಭಙ್ಗ;
                }
            }
        }

        // Remove refs to traced values from TraceDecl nodes, these have now moved under
        // TraceInc
        ಪ್ರತಿ (ಧ್ರುವ auto& i : traces) {
            Ast_ಗಂಟು* ಧ್ರುವ valuep = i.second->nodep()->valuep();
            VL_DO_DANGLING(valuep->unlinkFrBack()->delete_ವೃಕ್ಷ(), valuep);
        }

        // Create the trace cleanup function clearing the activity flags
        createCleanupFunction();
    }

    TraceCFuncVertex* getCFuncVertexp(AstCFunc* nodep) {
        V3GraphVertex* ಧ್ರುವ vtxp = nodep->user1u().toGraphVertex();
        TraceCFuncVertex* vertexp = vtxp ? vtxp->cast<TraceCFuncVertex>() : nullptr;
        ಯದಿ (!vertexp) {
            vertexp = ನೂತನ TraceCFuncVertex{&m_graph, nodep};
            nodep->user1p(vertexp);
        }
        ಪ್ರತ್ಯಾಗಮ vertexp;
    }
    TraceActivityVertex* getActivityVertexp(Ast_ಗಂಟು* nodep, ತಾರ್ಕಿಕ slow) {
        V3GraphVertex* ಧ್ರುವ vtxp = nodep->user3u().toGraphVertex();
        TraceActivityVertex* vertexp = vtxp ? vtxp->cast<TraceActivityVertex>() : nullptr;
        ಯದಿ (!vertexp) {
            vertexp = ನೂತನ TraceActivityVertex{&m_graph, nodep, slow};
            nodep->user3p(vertexp);
        }
        vertexp->slow(slow);
        ಪ್ರತ್ಯಾಗಮ vertexp;
    }

    // VISITORS
    ಶೂನ್ಯ visit(AstNetlist* nodep) override {
        // Add vertexes for all TraceDecl, and edges from VARs each trace looks at
        m_finding = ಅಸತ್ಯ;
        iterateChildren(nodep);

        // Add vertexes for all CFUNCs, and edges to VARs the func sets
        m_finding = ಸತ್ಯ;
        iterateChildren(nodep);

        // Create the trace functions and insert them into the tree
        createTraceFunctions();

        // Save number of trace codes used
        nodep->nTraceCodes(m_code);
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Module* nodep) override {
        ಯದಿ (nodep->is_ತುದಿ()) m_ತುದಿ_Modp = nodep;
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(AstVarScope* nodep) override {
        ಯದಿ (!m_finding) {
            ಯದಿ (ಧ್ರುವ AstIface* ಧ್ರುವ ifacep = nodep->varp()->sensIfacep()) {
                m_ifaceMemberVscps[{ifacep, nodep->varp()->name()}].ತಳ್ಳು_back(nodep);
            }
        }
    }
    ಶೂನ್ಯ visit(Ast_Stmt_ಅಭಿವ್ಯಕ್ತಿ* nodep) override {
        ಯದಿ (!m_finding && !nodep->user2()) {
            ಯದಿ (AstCCall* ಧ್ರುವ callp = VN_CAST(nodep->exprp(), CCall)) {
                UINFO(8, "   CCALL " << callp);
                // See if there are other calls in same statement list;
                // If so, all funcs might share the same activity code
                TraceActivityVertex* ಧ್ರುವ activityVtxp
                    = getActivityVertexp(nodep, callp->funcp()->slow());
                ಪ್ರತಿ (Ast_ಗಂಟು* nextp = nodep; nextp; nextp = nextp->nextp()) {
                    ಯದಿ (Ast_Stmt_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ stmtp = VN_CAST(nextp, Stmt_ಅಭಿವ್ಯಕ್ತಿ)) {
                        ಯದಿ (AstCCall* ಧ್ರುವ ccallp = VN_CAST(stmtp->exprp(), CCall)) {
                            stmtp->user2(ಸತ್ಯ);  // Processed
                            UINFO(8, "     SubCCALL " << ccallp);
                            V3GraphVertex* ಧ್ರುವ ccallFuncVtxp = getCFuncVertexp(ccallp->funcp());
                            activityVtxp->slow(ccallp->funcp()->slow());
                            ನೂತನ V3GraphEdge{&m_graph, activityVtxp, ccallFuncVtxp, 1};
                        }
                    }
                }
            }
        }
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(AstCFunc* nodep) override {
        UINFO(8, "   CFUNC " << nodep);
        V3GraphVertex* ಧ್ರುವ funcVtxp = getCFuncVertexp(nodep);
        ಯದಿ (!m_finding) {  // If public, we need a unique activity code to allow for sets
                           // directly in this func
            ಯದಿ (nodep->funcPublic() || nodep->dpiExportImpl() || nodep->entryPoint()
                || nodep->isCoroutine()) {
                // Cannot treat a coroutine as slow, it may be resumed later
                ಧ್ರುವ ತಾರ್ಕಿಕ slow = nodep->slow() && !nodep->isCoroutine();
                TraceActivityVertex* ಧ್ರುವ activityVtxp = getActivityVertexp(nodep, slow);
                ನೂತನ V3GraphEdge{&m_graph, activityVtxp, funcVtxp, 1};
            }
        }
        VL_RESTORER(m_cfuncp);
        m_cfuncp = nodep;
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(AstTraceDecl* nodep) override {
        UINFO(8, "   TRACE " << nodep);
        ಯದಿ (!m_finding && !nodep->inDtypeFunc()) {
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
            ಯದಿ (m_cfuncp && VString::startsWith(m_cfuncp->name(), "trace_init_leaf_root__")) {
                ಪ್ರತ್ಯಾಗಮ;
            }
            V3GraphVertex* ಧ್ರುವ vertexp = ನೂತನ TraceTraceVertex{&m_graph, nodep};
            nodep->user1p(vertexp);

            UASSERT_OBJ(m_cfuncp, nodep, "Trace not under func");
            m_cfuncp->user2(ಸತ್ಯ);
            VL_RESTORER(m_tracep);
            m_tracep = nodep;
            iterateChildren(nodep);
        }
    }
    ಶೂನ್ಯ visit(AstVarRef* nodep) override {
        UASSERT_OBJ(nodep->varScopep(), nodep, "No var scope?");
        AstVarScope* ಧ್ರುವ varscopep = nodep->varScopep();
        V3GraphVertex* varVtxp = varscopep->user1u().toGraphVertex();
        ಯದಿ (m_tracep) {
            UASSERT_OBJ(nodep->access().isReadOnly(), nodep, "Lvalue in trace?  Should be const.");
            ಯದಿ (!varVtxp) {
                varVtxp = ನೂತನ TraceVarVertex{&m_graph, nodep->varScopep()};
                nodep->varScopep()->user1p(varVtxp);
            }
            V3GraphVertex* ಧ್ರುವ traceVtxp = m_tracep->user1u().toGraphVertex();
            ನೂತನ V3GraphEdge{&m_graph, varVtxp, traceVtxp, 1};
            ಯದಿ (nodep->varp()->isPrimaryInish()  // Always need to trace primary inputs
                || nodep->varp()->isSigPublic()) {  // Or ones user can change
                ನೂತನ V3GraphEdge{&m_graph, m_alwaysVtxp, traceVtxp, 1};
            }
            ಯದಿ (m_tracep->dtypeCallp()) varscopep->user2p(m_tracep);
        } ಅನ್ಯಥಾ ಯದಿ (m_cfuncp && m_finding && nodep->access().isWriteOrRW()) {
            V3GraphVertex* ಧ್ರುವ funcVtxp = getCFuncVertexp(m_cfuncp);
            ಯದಿ (varVtxp) {  // else we're not tracing this signal
                ನೂತನ V3GraphEdge{&m_graph, funcVtxp, varVtxp, 1};
                AstTraceDecl* ಧ್ರುವ declp = VN_AS(varscopep->user2p(), TraceDecl);
                ಯದಿ (declp) {
                    V3GraphVertex* ಧ್ರುವ cFuncVtxp = getCFuncVertexp(m_cfuncp);
                    ಪ್ರತಿ (ಧ್ರುವ V3GraphEdge& edge : cFuncVtxp->inEdges()) {
                        V3GraphVertex* ಧ್ರುವ activityp = edge.fromp();
                        ಯದಿ (!declp->user3p()) {
                            declp->user3p(activityp);
                        } ಅನ್ಯಥಾ ಯದಿ (declp->user3u().toGraphVertex() != activityp) {
                            declp->user2(ಸತ್ಯ);
                        }
                    }
                }
            }
        }
    }
    ಶೂನ್ಯ visit(AstMemberSel* nodep) override {
        ಯದಿ (m_cfuncp && m_finding && nodep->access().isWriteOrRW()) {
            AstIfaceRefDType* ಧ್ರುವ dtypep
                = VN_CAST(nodep->fromp()->dtypep()->skipRefp(), IfaceRefDType);
            ಯದಿ (dtypep && dtypep->isVirtual()) {
                ಧ್ರುವ auto it = m_ifaceMemberVscps.find({dtypep->ifacep(), nodep->varp()->name()});
                ಯದಿ (it != m_ifaceMemberVscps.end()) {
                    V3GraphVertex* ಧ್ರುವ funcVtxp = getCFuncVertexp(m_cfuncp);
                    ಪ್ರತಿ (AstVarScope* ಧ್ರುವ vscp : it->second) {
                        V3GraphVertex* varVtxp = vscp->user1u().toGraphVertex();
                        ಯದಿ (!varVtxp) {
                            varVtxp = ನೂತನ TraceVarVertex{&m_graph, vscp};
                            vscp->user1p(varVtxp);
                        }
                        ನೂತನ V3GraphEdge{&m_graph, funcVtxp, varVtxp, 1};
                    }
                }
            }
        }
        iterateChildren(nodep);
    }
    //--------------------
    ಶೂನ್ಯ visit(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    explicit TraceVisitor(AstNetlist* nodep)
        : m_alwaysVtxp{ನೂತನ TraceActivityVertex{&m_graph, TraceActivityVertex::ACTIVITY_ALWAYS}} {
        nodep->user2_Clear_ವೃಕ್ಷ();  // TraceDecl multiple activities flag
        nodep->user3_Clear_ವೃಕ್ಷ();  // TraceDecl TraceActivityVertex (assumes we start at nullptr)
        iterate(nodep);
        nodep->foreach([](AstTraceDecl* ಧ್ರುವ declp) {
            ಯದಿ (declp->inDtypeFunc()) {
                declp->valuep()->unlinkFrBack()->delete_ವೃಕ್ಷ();
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

ಶೂನ್ಯ V3Trace::traceAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { TraceVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_ವೃಕ್ಷ("trace", 0, dump_ವೃಕ್ಷ_Either_Level() >= 3);
}
