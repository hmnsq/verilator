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

ವರ್ಗ Trace_Activity_ಶೃಂಗ final : ಸಾರ್ವಜನಿಕ V3_ನಕ್ಷೆ_ಶೃಂಗ {
    VL_RTTI_IMPL(Trace_Activity_ಶೃಂಗ, V3_ನಕ್ಷೆ_ಶೃಂಗ)
    Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ m_insertp;
    int32_t m_activity_ಸಂಕೇತ;
    ತಾರ್ಕಿಕ m_slow;  // If always slow, we can use the same code
ಸಾರ್ವಜನಿಕ:
    ಎಣಿಕೆ { ACTIVITY_NEVER = ((1UL << 31) - 1) };
    ಎಣಿಕೆ { ACTIVITY_ALWAYS = ((1UL << 31) - 2) };
    ಎಣಿಕೆ { ACTIVITY_SLOW = 0 };
    Trace_Activity_ಶೃಂಗ(V3_ನಕ್ಷೆ* graphp, Ast_ಗಂಟು* nodep, ತಾರ್ಕಿಕ slow)
        : V3_ನಕ್ಷೆ_ಶೃಂಗ{graphp}
        , m_insertp{nodep} {
        m_activity_ಸಂಕೇತ = 0;
        m_slow = slow;
    }
    Trace_Activity_ಶೃಂಗ(V3_ನಕ್ಷೆ* graphp, int32_t ಸಂಕೇತ_2)
        : V3_ನಕ್ಷೆ_ಶೃಂಗ{graphp}
        , m_insertp{ಶೂನ್ಯಸೂಚಕ} {
        m_activity_ಸಂಕೇತ = ಸಂಕೇತ_2;
        m_slow = ಸುಳ್ಳು;
    }
    ~Trace_Activity_ಶೃಂಗ() override = ಪೂರ್ವನಿಯೋಜಿತ;
    // ACCESSORS
    Ast_ಗಂಟು* insertp() ಸ್ಥಿರಾಂಕ {
        UASSERT(m_insertp, "Null insertp; probably called on a special always/slow");
        ಹಿಂತಿರುಗಿ m_insertp;
    }
    string name() ಸ್ಥಿರಾಂಕ override {
        ಆದರೆ (activityAlways()) {
            ಹಿಂತಿರುಗಿ "*ALWAYS*";
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಹಿಂತಿರುಗಿ std::string{slow() ? "*SLOW* " : ""} + insertp()->name();
        }
    }
    string dotColor() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ slow() ? "yellowGreen" : "green"; }
    int32_t activity_ಸಂಕೇತ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_activity_ಸಂಕೇತ; }
    ತಾರ್ಕಿಕ activityAlways() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ activity_ಸಂಕೇತ() == ACTIVITY_ALWAYS; }
    ತಾರ್ಕಿಕ activitySlow() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ activity_ಸಂಕೇತ() == ACTIVITY_SLOW; }
    ಶೂನ್ಯ activity_ಸಂಕೇತ(int32_t ಸಂಕೇತ_2) { m_activity_ಸಂಕೇತ = ಸಂಕೇತ_2; }
    ತಾರ್ಕಿಕ slow() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_slow; }
    ಶೂನ್ಯ slow(ತಾರ್ಕಿಕ flag) {
        ಆದರೆ (!flag) m_slow = ಸುಳ್ಳು;
    }
};

ವರ್ಗ Trace_CFunc_ಶೃಂಗ final : ಸಾರ್ವಜನಿಕ V3_ನಕ್ಷೆ_ಶೃಂಗ {
    VL_RTTI_IMPL(Trace_CFunc_ಶೃಂಗ, V3_ನಕ್ಷೆ_ಶೃಂಗ)
    AstCFunc* ಸ್ಥಿರಾಂಕ m_nodep;

ಸಾರ್ವಜನಿಕ:
    Trace_CFunc_ಶೃಂಗ(V3_ನಕ್ಷೆ* graphp, AstCFunc* nodep)
        : V3_ನಕ್ಷೆ_ಶೃಂಗ{graphp}
        , m_nodep{nodep} {}
    ~Trace_CFunc_ಶೃಂಗ() override = ಪೂರ್ವನಿಯೋಜಿತ;
    // ACCESSORS
    AstCFunc* nodep() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_nodep; }
    string name() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ nodep()->name(); }
    string dotColor() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ "yellow"; }
    File_ಸಾಲು* fileline() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ nodep()->fileline(); }
};

ವರ್ಗ Trace_Trace_ಶೃಂಗ final : ಸಾರ್ವಜನಿಕ V3_ನಕ್ಷೆ_ಶೃಂಗ {
    VL_RTTI_IMPL(Trace_Trace_ಶೃಂಗ, V3_ನಕ್ಷೆ_ಶೃಂಗ)
    AstTraceDecl* ಸ್ಥಿರಾಂಕ m_nodep;  // TRACEINC this represents
    // nullptr, or other vertex with the real code() that duplicates this one
    Trace_Trace_ಶೃಂಗ* m_duplicatep = ಶೂನ್ಯಸೂಚಕ;
    // When aliasing to a dtype parent, offset of the member within the dtype
    uint32_t m_dtype_Alias_ಸ್ಥಾನಾಂತರ = 0;

ಸಾರ್ವಜನಿಕ:
    Trace_Trace_ಶೃಂಗ(V3_ನಕ್ಷೆ* graphp, AstTraceDecl* nodep)
        : V3_ನಕ್ಷೆ_ಶೃಂಗ{graphp}
        , m_nodep{nodep} {}
    ~Trace_Trace_ಶೃಂಗ() override = ಪೂರ್ವನಿಯೋಜಿತ;
    // ACCESSORS
    AstTraceDecl* nodep() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_nodep; }
    string name() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ nodep()->name(); }
    string dotColor() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ "red"; }
    File_ಸಾಲು* fileline() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ nodep()->fileline(); }
    Trace_Trace_ಶೃಂಗ* duplicatep() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_duplicatep; }
    ಶೂನ್ಯ duplicatep(Trace_Trace_ಶೃಂಗ* dupp) {
        UASSERT_OBJ(!duplicatep(), nodep(), "Assigning duplicatep() to already duplicated node");
        m_duplicatep = dupp;
    }
    ಶೂನ್ಯ redirectDuplicatep(Trace_Trace_ಶೃಂಗ* dupp) { m_duplicatep = dupp; }
    uint32_t dtype_Alias_ಸ್ಥಾನಾಂತರ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_dtype_Alias_ಸ್ಥಾನಾಂತರ; }
    ಶೂನ್ಯ dtype_Alias_ಸ್ಥಾನಾಂತರ(uint32_t ಸ್ಥಾನಾಂತರ) { m_dtype_Alias_ಸ್ಥಾನಾಂತರ = ಸ್ಥಾನಾಂತರ; }
};

ವರ್ಗ Trace_Var_ಶೃಂಗ final : ಸಾರ್ವಜನಿಕ V3_ನಕ್ಷೆ_ಶೃಂಗ {
    VL_RTTI_IMPL(Trace_Var_ಶೃಂಗ, V3_ನಕ್ಷೆ_ಶೃಂಗ)
    AstVarScope* ಸ್ಥಿರಾಂಕ m_nodep;

ಸಾರ್ವಜನಿಕ:
    Trace_Var_ಶೃಂಗ(V3_ನಕ್ಷೆ* graphp, AstVarScope* nodep)
        : V3_ನಕ್ಷೆ_ಶೃಂಗ{graphp}
        , m_nodep{nodep} {}
    ~Trace_Var_ಶೃಂಗ() override = ಪೂರ್ವನಿಯೋಜಿತ;
    // ACCESSORS
    AstVarScope* nodep() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_nodep; }
    string name() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ nodep()->name(); }
    string dotColor() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ "skyblue"; }
    File_ಸಾಲು* fileline() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ nodep()->fileline(); }
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
    ಸ್ಥಿರಾಂಕ VNUser1_ಒಳ_Use m_inuser1;
    ಸ್ಥಿರಾಂಕ VNUser2_ಒಳ_Use m_inuser2;
    ಸ್ಥಿರಾಂಕ VNUser3_ಒಳ_Use m_inuser3;
    // VNUser4InUse     In V3Hasher via V3DupFinder

    // STATE
    Ast_ಗಂಟು_Module* m_ತುದಿ_Modp = ಶೂನ್ಯಸೂಚಕ;  // Module to add variables to
    AstScope* ಸ್ಥಿರಾಂಕ m_ತುದಿ_Scopep = v3Global.rootp()->ತುದಿ_Scopep()->scopep();  // The top AstScope
    AstCFunc* m_cfuncp = ಶೂನ್ಯಸೂಚಕ;  // C function adding to graph
    AstCFunc* m_regFuncp = ಶೂನ್ಯಸೂಚಕ;  // Trace registration function
    AstCFunc* m_actAllFuncp = ಶೂನ್ಯಸೂಚಕ;  // Set all activity function
    AstTraceDecl* m_tracep = ಶೂನ್ಯಸೂಚಕ;  // Trace function adding to graph
    AstVarScope* m_activityVscp = ಶೂನ್ಯಸೂಚಕ;  // Activity variable
    uint32_t m_activity_ಸಂಖ್ಯೆ = 0;  // Count of fields in activity variable
    uint32_t m_ಸಂಕೇತ = 0;  // Trace ident code# being assigned
    V3_ನಕ್ಷೆ m_ನಕ್ಷೆ;  // Var/CFunc tracking
    Trace_Activity_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ m_alwaysVtxp;  // "Always trace" vertex
    ತಾರ್ಕಿಕ m_finding = ಸುಳ್ಳು;  // Pass one of algorithm?
    ರಚನೆ DtypeFuncs final {
    ಸಾರ್ವಜನಿಕ:
        AstCFunc* ತುಂಬಿದ_Funcp = ಶೂನ್ಯಸೂಚಕ;
        AstCFunc* chgFuncp = ಶೂನ್ಯಸೂಚಕ;
    };
    std::unordered_map<ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType*, DtypeFuncs>
        m_dtypeNonConstFuncs;  // Full / Chg funcs per type
    std::unordered_map<ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType*, AstCFunc*> m_dtypeConstFuncs;  // Const func per type
    V3_Unique_ಹೆಸರುಗಳು m_dtype_ಹೆಸರುಗಳು{""};  // Unique type func names

    // Trace parallelism. Only VCD tracing can be parallelized at this time.
    ಸ್ಥಿರಾಂಕ uint32_t m_parallelism
        = v3Global.opt.useTraceParallel() ? static_cast<uint32_t>(v3Global.opt.ಎಳೆಗಳು()) : 1;

    VDouble0 m_statSetters;  // Statistic tracking
    VDouble0 m_statSettersSlow;  // Statistic tracking
    VDouble0 m_statUniqSigs;  // Statistic tracking

    // All activity numbers applying to a given trace
    ಬಳಸಿ Act_ಸಂಕೇತ_ಗಣ = std::ಗಣ<uint32_t>;
    // For activity set, what traces apply
    ಬಳಸಿ TraceVec = std::multimap<Act_ಸಂಕೇತ_ಗಣ, Trace_Trace_ಶೃಂಗ*>;
    // Candidate interface-member VarScopes keyed by (interface type, member name)
    std::map<std::pair<ಸ್ಥಿರಾಂಕ AstIface*, std::string>, std::ಸದಿಶ<AstVarScope*>>
        m_ifaceMemberVscps;

    ವರ್ಗ TraceInitDeclCollector final : ಸಾರ್ವಜನಿಕ VNVisitor {
        std::ಸದಿಶ<AstTraceDecl*>& m_declps;  // Output: trace declarations found so far
        std::ಗಣ<ಸ್ಥಿರಾಂಕ AstCFunc*> m_ನೋಡಿದ_Funcps;

        ಶೂನ್ಯ ಭೇಟಿ(AstTraceDecl* nodep) override { m_declps.ತಳ್ಳು_ಹಿಂಭಾಗ(nodep); }
        ಶೂನ್ಯ ಭೇಟಿ(AstCCall* nodep) override {
            ಆದರೆ (AstCFunc* ಸ್ಥಿರಾಂಕ funcp = nodep->funcp()) collect(funcp);
        }
        ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }

    ಸಾರ್ವಜನಿಕ:
        ಸ್ಪಷ್ಟ TraceInitDeclCollector(std::ಸದಿಶ<AstTraceDecl*>& declps)
            : m_declps{declps} {}
        ಶೂನ್ಯ collect(AstCFunc* funcp) {
            ಆದರೆ (funcp && m_ನೋಡಿದ_Funcps.ಸೇರಿಸು(funcp).ಎರಡನೇ) iterate(funcp);
        }
    };

    // METHODS

    ಸ್ಥಿರ ತಾರ್ಕಿಕ same_ಬೇರು_Init_Alias(ಸ್ಥಿರಾಂಕ AstTraceDecl* ಬೇರು_Declp, ಸ್ಥಿರಾಂಕ AstTraceDecl* ತುದಿ_Declp) {
        ಸ್ಥಿರಾಂಕ VNumRange& ಬೇರು_ಬಿಟ್_Range = ಬೇರು_Declp->ಬಿಟ್_Range();
        ಸ್ಥಿರಾಂಕ VNumRange& ತುದಿ_ಬಿಟ್_Range = ತುದಿ_Declp->ಬಿಟ್_Range();
        ಹಿಂತಿರುಗಿ ಬೇರು_Declp->showname() == ತುದಿ_Declp->showname()
               && ಬೇರು_Declp->declDirection() == ತುದಿ_Declp->declDirection()
               && ಬೇರು_Declp->ಅಗಲ_Min() == ತುದಿ_Declp->ಅಗಲ_Min()
               && ಬೇರು_ಬಿಟ್_Range.ranged() == ತುದಿ_ಬಿಟ್_Range.ranged()
               && (!ಬೇರು_ಬಿಟ್_Range.ranged()
                   || (ಬೇರು_ಬಿಟ್_Range.ಎಡ() == ತುದಿ_ಬಿಟ್_Range.ಎಡ()
                       && ಬೇರು_ಬಿಟ್_Range.ಬಲ() == ತುದಿ_ಬಿಟ್_Range.ಬಲ()));
    }

    ಶೂನ್ಯ detectDuplicates() {
        UINFO(9, "Finding duplicates");
        // Note uses user4
        V3DupFinder dupFinder;  // Duplicate code detection

        // Compute member offsets within dtype instances incrementally.
        // For each dtype member TraceDecl, stores its code offset within the
        // dtype parent's code range.
        std::unordered_map<ಸ್ಥಿರಾಂಕ AstTraceDecl*, uint32_t> memberOffsets;
        std::unordered_map<ಸ್ಥಿರಾಂಕ AstTraceDecl*, uint32_t> running_ಸ್ಥಾನಾಂತರ;

        // For fixup pass: track dups redirected to dtype parents, so we can
        // fix them if the parent's user2 is later set during the loop.
        // Maps dup vertex -> original canonical member vertex.
        std::ಸದಿಶ<std::pair<Trace_Trace_ಶೃಂಗ*, Trace_Trace_ಶೃಂಗ*>> dtype_ಪೋಷಕ_Redirects;

        // Hash all of the traced values and find if there are any duplicates
        ಗಾಗಿ (V3_ನಕ್ಷೆ_ಶೃಂಗ& vtx : m_ನಕ್ಷೆ.vertices()) {
            ಆದರೆ (Trace_Trace_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vvertexp = vtx.cast<Trace_Trace_ಶೃಂಗ>()) {
                AstTraceDecl* ಸ್ಥಿರಾಂಕ nodep = vvertexp->nodep();
                ಆದರೆ (nodep->dtypeCallp()) ಮುಂದುವರಿ;

                ಆದರೆ (nodep->dtypeDeclp()) {
                    uint32_t& ಸ್ಥಾನಾಂತರ = running_ಸ್ಥಾನಾಂತರ[nodep->dtypeDeclp()];
                    memberOffsets[nodep] = ಸ್ಥಾನಾಂತರ;
                    ಸ್ಥಾನಾಂತರ += nodep->ಸಂಕೇತ_Inc();
                }

                UASSERT_OBJ(!vvertexp->duplicatep(), nodep, "Should not be a duplicate");
                ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ dupit = dupFinder.ಕಂಡುಹಿಡಿ_Duplicate(nodep);
                ಆದರೆ (dupit == dupFinder.ಕೊನೆ()) {
                    dupFinder.ಸೇರಿಸು(nodep);
                } ಇಲ್ಲದಿದ್ದರೆ {
                    AstTraceDecl* ಸ್ಥಿರಾಂಕ dupDeclp = VN_AS(dupit->ಎರಡನೇ, TraceDecl);
                    UASSERT_OBJ(dupDeclp, nodep, "Trace duplicate of wrong type");
                    Trace_Trace_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ dupvertexp
                        = dupDeclp->user1u().ಗಮ್ಯ_ನಕ್ಷೆ_ಶೃಂಗ()->cast<Trace_Trace_ಶೃಂಗ>();
                    UINFO(8, "  Orig " << dupDeclp << ಸಾಲು_ಅಂತ್ಯ);
                    UINFO(8, "   dup " << nodep << ಸಾಲು_ಅಂತ್ಯ);

                    ಆದರೆ (dupDeclp->dtypeDeclp() && !dupDeclp->dtypeDeclp()->user2()) {
                        AstTraceDecl* ಸ್ಥಿರಾಂಕ dtypeParentp = dupDeclp->dtypeDeclp();
                        Trace_Trace_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ dtypeVtxp
                            = dtypeParentp->user1u().ಗಮ್ಯ_ನಕ್ಷೆ_ಶೃಂಗ()->cast<Trace_Trace_ಶೃಂಗ>();
                        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ it2 = memberOffsets.ಕಂಡುಹಿಡಿ(dupDeclp);
                        UASSERT_OBJ(it2 != memberOffsets.ಕೊನೆ(), dupDeclp,
                                    "Member offset not precomputed");
                        vvertexp->duplicatep(dtypeVtxp);
                        vvertexp->dtype_Alias_ಸ್ಥಾನಾಂತರ(it2->ಎರಡನೇ);
                        dtype_ಪೋಷಕ_Redirects.emplace_ಹಿಂಭಾಗ(vvertexp, dupvertexp);
                    } ಇಲ್ಲದಿದ್ದರೆ {
                        vvertexp->duplicatep(dupvertexp);
                    }
                    ಆದರೆ (nodep->dtypeDeclp()) nodep->dtypeDeclp()->user2(ನಿಜ);
                }
            }
        }

        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& pair : dtype_ಪೋಷಕ_Redirects) {
            Trace_Trace_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ dupVtxp = pair.ಮೊದಲ;
            Trace_Trace_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ memberVtxp = pair.ಎರಡನೇ;
            ಸ್ಥಿರಾಂಕ AstTraceDecl* ಸ್ಥಿರಾಂಕ ಪೋಷಕ_Declp = dupVtxp->duplicatep()->nodep();
            ಆದರೆ (ಪೋಷಕ_Declp->user2()) {
                dupVtxp->redirectDuplicatep(memberVtxp);
                dupVtxp->dtype_Alias_ಸ್ಥಾನಾಂತರ(0);
            }
        }

        ಆದರೆ (dumpLevel() || debug() >= 9)
            dupFinder.dumpFile(v3Global.debugFilename("trace") + ".hash", ಸುಳ್ಳು);
    }

    ಶೂನ್ಯ ನಕ್ಷೆ_Dtype_Prune() {
        ಗಾಗಿ (V3_ನಕ್ಷೆ_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vtxp : m_ನಕ್ಷೆ.vertices().unlinkable()) {
            ಆದರೆ (Trace_Trace_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vvertexp = vtxp->cast<Trace_Trace_ಶೃಂಗ>()) {
                AstTraceDecl* ಸ್ಥಿರಾಂಕ declp = vvertexp->nodep();
                // This skips the dtype sub-func optimization if a var is affected by multiple
                // activities.  We really only need to do this for _chg funcs (and not decls,
                // _const and _full) but it's simpiler to do it all one way or the other.
                ಆದರೆ (declp) {
                    ಆದರೆ (declp->user2() || (declp->dtypeDeclp() && !declp->dtypeDeclp()->user2())) {
                        AstCCall* ಸ್ಥಿರಾಂಕ callp = declp->dtypeCallp();
                        ಆದರೆ (callp) {
                            Ast_ಗಂಟು* stmtexprp = callp->backp();
                            VL_DO_DANGLING(ತಳ್ಳು_Deletep(stmtexprp->unlink_Fr_ಹಿಂಭಾಗ()), stmtexprp);
                        } ಇಲ್ಲದಿದ್ದರೆ {
                            ತಾರ್ಕಿಕ ಖಾಲಿಗಳು_Scope;
                            ಮಾಡು {
                                ಖಾಲಿಗಳು_Scope = ಸುಳ್ಳು;
                                Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ declBackp = declp->backp();
                                Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ declNextp = declp->nextp();
                                ಆದರೆ (VN_IS(declBackp, Trace_ತಳ್ಳು_ಪೂರ್ವಪ್ರತ್ಯಯ)
                                    && VN_IS(declNextp, Trace_ತೆಗೆ_ಪೂರ್ವಪ್ರತ್ಯಯ)) {
                                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(declBackp->unlink_Fr_ಹಿಂಭಾಗ()),
                                                   declBackp);
                                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(declNextp->unlink_Fr_ಹಿಂಭಾಗ()),
                                                   declNextp);
                                    ಖಾಲಿಗಳು_Scope = ನಿಜ;
                                }
                            } ಆಗ (ಖಾಲಿಗಳು_Scope);
                        }
                        // Can't purge until we finish this pass
                        ತಳ್ಳು_Deletep(declp->unlink_Fr_ಹಿಂಭಾಗ());  // declp used below
                        vvertexp->reroute_ಅಂಚುಗಳು(&m_ನಕ್ಷೆ);
                        vvertexp->unlinkDelete(&m_ನಕ್ಷೆ);
                    }
                    declp->dtypeDeclp(ಶೂನ್ಯಸೂಚಕ);
                }
            }
        }
    }

    ಶೂನ್ಯ ವಿಭಜಿಸು_Trace_Decl_Funcs() {
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ವಿಭಜಿಸು_ಎಲ್ಲೆ = v3Global.opt.output_ವಿಭಜಿಸು_CTrace() ? v3Global.opt.output_ವಿಭಜಿಸು_CTrace()
                                                                : std::numeric_limits<ಪೂರ್ಣಾಂಕ>::max();
        ಆದರೆ (ವಿಭಜಿಸು_ಎಲ್ಲೆ == std::numeric_limits<ಪೂರ್ಣಾಂಕ>::max()) ಹಿಂತಿರುಗಿ;

        ಗಾಗಿ (Ast_ಗಂಟು* nodep = m_ತುದಿ_Scopep->blocksp(); nodep; nodep = nodep->nextp()) {
            AstCFunc* ಸ್ಥಿರಾಂಕ funcp = VN_CAST(nodep, CFunc);
            ಆದರೆ (!funcp || !funcp->user2()) ಮುಂದುವರಿ;

            ಸ್ಥಿರಾಂಕ string ತಳ_Name = funcp->name();
            uint32_t sub_ಅಂಕೆ = 1;
            ಪೂರ್ಣಾಂಕ cur_ಗಾತ್ರ = 0;
            AstCFunc* curFuncp = funcp;
            std::ಸದಿಶ<Ast_ಗಂಟು_Stmt*> callStmts;

            ಸ್ವಯಂಚಾಲಿತ ಆರಂಭ_New_Sub_Func = [&]() {
                File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = funcp->fileline();
                ಸ್ಥಿರಾಂಕ string newName = ತಳ_Name + "_" + cvt_ಗಮ್ಯ_Str(sub_ಅಂಕೆ++);
                AstCFunc* ಸ್ಥಿರಾಂಕ newFuncp = ಹೊಸ AstCFunc{flp, newName, m_ತುದಿ_Scopep};
                newFuncp->argTypes(v3Global.opt.trace_Class_ತಳ() + "* tracep");
                newFuncp->isTrace(ನಿಜ);
                newFuncp->isStatic(ಸುಳ್ಳು);
                newFuncp->isLoose(ನಿಜ);
                newFuncp->slow(ನಿಜ);
                newFuncp->dontCombine(ನಿಜ);
                m_ತುದಿ_Scopep->ಕೂಡಿಸು_Blocksp(newFuncp);
                newFuncp->ಕೂಡಿಸು_Stmtsp(ಹೊಸ AstCStmt{flp, "const int c = vlSymsp->__Vm_baseCode;"});
                AstCCall* ಸ್ಥಿರಾಂಕ callp = ಹೊಸ AstCCall{flp, newFuncp};
                callp->dtype_ಗಣ_Void();
                callp->argTypes("tracep");
                callStmts.ತಳ್ಳು_ಹಿಂಭಾಗ(callp->makeStmt());
                curFuncp = newFuncp;
                cur_ಗಾತ್ರ = 0;
            };

            ಗಾಗಿ (Ast_ಗಂಟು *stmtp = funcp->stmtsp(), *nextp; stmtp; stmtp = nextp) {
                nextp = stmtp->nextp();
                cur_ಗಾತ್ರ += stmtp->ಗಂಟು_ಎಣಿಕೆ();

                ಆದರೆ (curFuncp != funcp) {
                    stmtp->unlink_Fr_ಹಿಂಭಾಗ();
                    curFuncp->ಕೂಡಿಸು_Stmtsp(stmtp);
                }

                ಆದರೆ (cur_ಗಾತ್ರ > ವಿಭಜಿಸು_ಎಲ್ಲೆ && nextp) ಆರಂಭ_New_Sub_Func();
            }

            ಗಾಗಿ (Ast_ಗಂಟು_Stmt* ಸ್ಥಿರಾಂಕ callStmtp : callStmts) { funcp->ಕೂಡಿಸು_Stmtsp(callStmtp); }
        }
    }

    ಶೂನ್ಯ ನಕ್ಷೆ_Simplify(ತಾರ್ಕಿಕ initial) {
        ಆದರೆ (initial) {
            // Remove all variable nodes
            ಗಾಗಿ (V3_ನಕ್ಷೆ_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vtxp : m_ನಕ್ಷೆ.vertices().unlinkable()) {
                ಆದರೆ (Trace_Var_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vvertexp = vtxp->cast<Trace_Var_ಶೃಂಗ>()) {
                    vvertexp->reroute_ಅಂಚುಗಳು(&m_ನಕ್ಷೆ);
                    vvertexp->unlinkDelete(&m_ನಕ್ಷೆ);
                }
            }
            // Remove multiple variables connecting funcs to traces
            // We do this twice, as then we have fewer edges to multiply out in the below
            // expansion.
            m_ನಕ್ಷೆ.remove_Redundant_ಅಂಚುಗಳು_Max(&V3_ನಕ್ಷೆ_ಅಂಚು::followAlwaysTrue);
            // Remove all Cfunc nodes
            ಗಾಗಿ (V3_ನಕ್ಷೆ_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vtxp : m_ನಕ್ಷೆ.vertices().unlinkable()) {
                ಆದರೆ (Trace_CFunc_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vvertexp = vtxp->cast<Trace_CFunc_ಶೃಂಗ>()) {
                    vvertexp->reroute_ಅಂಚುಗಳು(&m_ನಕ್ಷೆ);
                    vvertexp->unlinkDelete(&m_ನಕ್ಷೆ);
                }
            }
        }

        // Remove multiple variables connecting funcs to traces
        m_ನಕ್ಷೆ.remove_Redundant_ಅಂಚುಗಳು_Max(&V3_ನಕ್ಷೆ_ಅಂಚು::followAlwaysTrue);

        // If there are any edges from a always, keep only the always
        ಗಾಗಿ (V3_ನಕ್ಷೆ_ಶೃಂಗ& vtx : m_ನಕ್ಷೆ.vertices()) {
            ಆದರೆ (Trace_Trace_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vvertexp = vtx.cast<Trace_Trace_ಶೃಂಗ>()) {
                // Search for the incoming always edge
                ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಅಂಚು* alwaysEdgep = ಶೂನ್ಯಸೂಚಕ;
                ಗಾಗಿ (ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಅಂಚು& ಅಂಚು : vvertexp->ಒಳ_ಅಂಚುಗಳು()) {
                    ಸ್ಥಿರಾಂಕ Trace_Activity_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ actVtxp
                        = ಅಂಚು.fromp()->as<ಸ್ಥಿರಾಂಕ Trace_Activity_ಶೃಂಗ>();
                    ಆದರೆ (actVtxp->activityAlways()) {
                        alwaysEdgep = &ಅಂಚು;
                        ನಿಲ್ಲಿಸು;
                    }
                }
                // If always edge exists, remove all other edges
                ಆದರೆ (alwaysEdgep) {
                    ಗಾಗಿ (V3_ನಕ್ಷೆ_ಅಂಚು* ಸ್ಥಿರಾಂಕ edgep : vvertexp->ಒಳ_ಅಂಚುಗಳು().unlinkable()) {
                        ಆದರೆ (edgep != alwaysEdgep) VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
                    }
                }
            }
        }

        // Activity points with no outputs can be removed
        ಗಾಗಿ (V3_ನಕ್ಷೆ_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vtxp : m_ನಕ್ಷೆ.vertices().unlinkable()) {
            ಆದರೆ (Trace_Activity_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ aVtxp = vtxp->cast<Trace_Activity_ಶೃಂಗ>()) {
                // Leave in the always vertex for later use.
                ಆದರೆ (aVtxp != m_alwaysVtxp && aVtxp->ಹೊರಹೋಗು_ಖಾಲಿಗಳು()) {
                    VL_DO_DANGLING(aVtxp->unlinkDelete(&m_ನಕ್ಷೆ), aVtxp);
                }
            }
        }
    }

    uint32_t assignactivityNumbers() {
        uint32_t activity_ಸಂಖ್ಯೆ = 1;  // Note 0 indicates "slow" only
        ಗಾಗಿ (V3_ನಕ್ಷೆ_ಶೃಂಗ& vtx : m_ನಕ್ಷೆ.vertices()) {
            ಆದರೆ (Trace_Activity_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vvertexp = vtx.cast<Trace_Activity_ಶೃಂಗ>()) {
                ಆದರೆ (vvertexp != m_alwaysVtxp) {
                    ಆದರೆ (vvertexp->slow()) {
                        vvertexp->activity_ಸಂಕೇತ(Trace_Activity_ಶೃಂಗ::ACTIVITY_SLOW);
                    } ಇಲ್ಲದಿದ್ದರೆ {
                        vvertexp->activity_ಸಂಕೇತ(activity_ಸಂಖ್ಯೆ++);
                    }
                }
            }
        }
        ಹಿಂತಿರುಗಿ activity_ಸಂಖ್ಯೆ;
    }

    ಶೂನ್ಯ ವಿಂಗಡಿಸು_Traces(TraceVec& traces, uint32_t& nNonConstCodes) {
        // Populate sort structure
        traces.clear();
        nNonConstCodes = 0;
        ಗಾಗಿ (V3_ನಕ್ಷೆ_ಶೃಂಗ& vtx : m_ನಕ್ಷೆ.vertices()) {
            ಆದರೆ (Trace_Trace_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vtxp = vtx.cast<Trace_Trace_ಶೃಂಗ>()) {
                Act_ಸಂಕೇತ_ಗಣ act_ಗಣ;
                UINFO(9, "  Add to sort: " << vtxp);
                UINFOTREE(9, vtxp->nodep(), "", "trnode");
                ಗಾಗಿ (ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಅಂಚು& ಅಂಚು : vtxp->ಒಳ_ಅಂಚುಗಳು()) {
                    ಸ್ಥಿರಾಂಕ Trace_Activity_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ cfvertexp
                        = ಅಂಚು.fromp()->cast<ಸ್ಥಿರಾಂಕ Trace_Activity_ಶೃಂಗ>();
                    UASSERT_OBJ(cfvertexp, vtxp->nodep(),
                                "Should have been function pointing to this trace");
                    UINFO(9, "   Activity: " << cfvertexp);
                    ಆದರೆ (cfvertexp->activityAlways()) {
                        // If code 0, we always trace; ignore other codes
                        act_ಗಣ.ಸೇರಿಸು(Trace_Activity_ಶೃಂಗ::ACTIVITY_ALWAYS);
                    } ಇಲ್ಲದಿದ್ದರೆ {
                        act_ಗಣ.ಸೇರಿಸು(cfvertexp->activity_ಸಂಕೇತ());
                    }
                }
                UASSERT_OBJ(act_ಗಣ.ಎಣಿಕೆ_2(Trace_Activity_ಶೃಂಗ::ACTIVITY_ALWAYS) == 0
                                || act_ಗಣ.ಗಾತ್ರ_2() == 1,
                            vtxp->nodep(), "Always active trace has further triggers");
                // Count nodes
                ಆದರೆ (!vtxp->duplicatep() && !act_ಗಣ.ಖಾಲಿಗಳು())
                    nNonConstCodes += vtxp->nodep()->ಸಂಕೇತ_Inc();
                ಆದರೆ (act_ಗಣ.ಖಾಲಿಗಳು()) {
                    // If a trace doesn't have activity, it's constant, and we
                    // don't need to track changes on it.
                    act_ಗಣ.ಸೇರಿಸು(Trace_Activity_ಶೃಂಗ::ACTIVITY_NEVER);
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (act_ಗಣ.ಎಣಿಕೆ_2(Trace_Activity_ಶೃಂಗ::ACTIVITY_SLOW) && act_ಗಣ.ಗಾತ್ರ_2() > 1) {
                    // If a trace depends on the slow flag as well as other
                    // flags, remove the dependency on the slow flag. We will
                    // make slow routines set all activity flags.
                    act_ಗಣ.erase(Trace_Activity_ಶೃಂಗ::ACTIVITY_SLOW);
                }
                traces.emplace(act_ಗಣ, vtxp);
            }
        }
    }

    ಶೂನ್ಯ ನಕ್ಷೆ_Optimize() {
        // Assign initial activity numbers to activity vertices
        assignactivityNumbers();

        // Sort the traces by activity sets
        TraceVec traces;
        uint32_t unused1;
        ವಿಂಗಡಿಸು_Traces(traces, unused1);

        // For each activity set with only a small number of signals, make those
        // signals always traced, as it's cheaper to check a few value changes
        // than to test a lot of activity flags
        ಸ್ವಯಂಚಾಲಿತ it = traces.begin();
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ ಕೊನೆ = traces.ಕೊನೆ();
        ಆಗ (it != ಕೊನೆ) {
            ಸ್ವಯಂಚಾಲಿತ ತಲೆ = it;
            // Approximate the complexity of the value change check
            uint32_t complexity = 0;
            ಸ್ಥಿರಾಂಕ Act_ಸಂಕೇತ_ಗಣ& act_ಗಣ = it->ಮೊದಲ;
            ಗಾಗಿ (; it != ಕೊನೆ && it->ಮೊದಲ == act_ಗಣ; ++it) {
                ಆದರೆ (!it->ಎರಡನೇ->duplicatep()) {
                    uint32_t ವೆಚ್ಚ = 0;
                    ಸ್ಥಿರಾಂಕ AstTraceDecl* ಸ್ಥಿರಾಂಕ declp = it->ಎರಡನೇ->nodep();
                    // The number of comparisons required by bufp->chg*
                    ವೆಚ್ಚ += declp->isWide() ? declp->ಸಂಕೇತ_Inc() : 1;
                    // Arrays are traced by element
                    ವೆಚ್ಚ *= declp->arrayRange().ranged() ? declp->arrayRange().elements() : 1;
                    // Note: Experiments factoring in the size of declp->valuep()
                    // showed no benefit in tracing speed, even for large trees,
                    // so we will leave those out for now.
                    complexity += ವೆಚ್ಚ;
                }
            }
            // Leave alone always changing, never changing and signals only set in slow code
            ಆದರೆ (act_ಗಣ.ಎಣಿಕೆ_2(Trace_Activity_ಶೃಂಗ::ACTIVITY_ALWAYS)) ಮುಂದುವರಿ;
            ಆದರೆ (act_ಗಣ.ಎಣಿಕೆ_2(Trace_Activity_ಶೃಂಗ::ACTIVITY_NEVER)) ಮುಂದುವರಿ;
            ಆದರೆ (act_ಗಣ.ಎಣಿಕೆ_2(Trace_Activity_ಶೃಂಗ::ACTIVITY_SLOW)) ಮುಂದುವರಿ;
            // If the value comparisons are cheaper to perform than checking the
            // activity flags make the signals always traced. Note this cost
            // equation is heuristic.
            ಆದರೆ (complexity <= act_ಗಣ.ಗಾತ್ರ_2() * 2) {
                ಗಾಗಿ (; ತಲೆ != it; ++ತಲೆ) {
                    ಹೊಸ V3_ನಕ್ಷೆ_ಅಂಚು{&m_ನಕ್ಷೆ, m_alwaysVtxp, ತಲೆ->ಎರಡನೇ, 1};
                }
            }
        }

        ನಕ್ಷೆ_Simplify(ಸುಳ್ಳು);
    }

    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* selectActivity(File_ಸಾಲು* flp, uint32_t acode, ಸ್ಥಿರಾಂಕ VAccess& access) {
        ಹಿಂತಿರುಗಿ ಹೊಸ AstArraySel(flp, ಹೊಸ AstVarRef{flp, m_activityVscp, access}, acode);
    }

    Ast_ಗಂಟು* newActivitySetter(Ast_ಗಂಟು* insertp, uint32_t ಸಂಕೇತ_2) {
        ++m_statSetters;
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = insertp->fileline();
        Ast_ನಿಯೋಜಿಸು* ಸ್ಥಿರಾಂಕ setterp = ಹೊಸ Ast_ನಿಯೋಜಿಸು{fl, selectActivity(fl, ಸಂಕೇತ_2, VAccess::WRITE),
                                                 ಹೊಸ AstConst{fl, AstConst::ಬಿಟ್_True{}}};
        ಹಿಂತಿರುಗಿ setterp;
    }

    Ast_ಗಂಟು* newActivityAll(Ast_ಗಂಟು* insertp) {
        ++m_statSettersSlow;
        ಆದರೆ (!m_actAllFuncp) {
            File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = m_ತುದಿ_Scopep->fileline();
            AstCFunc* ಸ್ಥಿರಾಂಕ funcp = ಹೊಸ AstCFunc{flp, "__VtraceActivitySetAll", m_ತುದಿ_Scopep};
            funcp->slow(ನಿಜ);
            funcp->isStatic(ಸುಳ್ಳು);
            funcp->isLoose(ನಿಜ);
            m_ತುದಿ_Scopep->ಕೂಡಿಸು_Blocksp(funcp);
            ಗಾಗಿ (uint32_t ಸಂಕೇತ_2 = 0; ಸಂಕೇತ_2 < m_activity_ಸಂಖ್ಯೆ; ++ಸಂಕೇತ_2) {
                Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ setterp = newActivitySetter(insertp, ಸಂಕೇತ_2);
                funcp->ಕೂಡಿಸು_Stmtsp(setterp);
            }
            m_actAllFuncp = funcp;
        }
        AstCCall* ಸ್ಥಿರಾಂಕ callp = ಹೊಸ AstCCall{insertp->fileline(), m_actAllFuncp};
        callp->dtype_ಗಣ_Void();
        ಹಿಂತಿರುಗಿ callp->makeStmt();
    }

    ಶೂನ್ಯ createActivityFlags() {
        // Assign final activity numbers
        m_activity_ಸಂಖ್ಯೆ = assignactivityNumbers();

        // Create an array of bytes, not a bit vector, as they can be set
        // atomically by mtasks, and are cheaper to set (no need for
        // read-modify-write on the C type), and the speed of the tracing code
        // is the same on largish designs.
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = m_ತುದಿ_Scopep->fileline();
        Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ newScalarDtp = ಹೊಸ AstBasicDType{flp, VFlag_ಬಿಟ್_Packed{}, 1};
        v3Global.rootp()->typeTablep()->ಕೂಡಿಸು_Typesp(newScalarDtp);
        AstRange* ಸ್ಥಿರಾಂಕ newArange
            = ಹೊಸ AstRange{flp, VNumRange{static_cast<ಪೂರ್ಣಾಂಕ>(m_activity_ಸಂಖ್ಯೆ) - 1, 0}};
        Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ newArrDtp = ಹೊಸ AstUnpackArrayDType{flp, newScalarDtp, newArange};
        v3Global.rootp()->typeTablep()->ಕೂಡಿಸು_Typesp(newArrDtp);
        m_activityVscp = v3Global.rootp()->ತುದಿ_Scopep()->create_ತಾತ್ಕಾಲಿಕ("__VtraceActivity", newArrDtp);

        // Insert activity setters
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಶೃಂಗ& vtx : m_ನಕ್ಷೆ.vertices()) {
            ಆದರೆ (ಸ್ಥಿರಾಂಕ Trace_Activity_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vtxp = vtx.cast<ಸ್ಥಿರಾಂಕ Trace_Activity_ಶೃಂಗ>()) {
                Ast_ಗಂಟು* setterp = ಶೂನ್ಯಸೂಚಕ;
                ಆದರೆ (vtxp->activitySlow()) {
                    setterp = newActivityAll(vtxp->insertp());
                    // Just set all flags in slow code as it should be rare.
                    // This will be rolled up into a loop by V3Reloop.
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (!vtxp->activityAlways()) {
                    setterp = newActivitySetter(vtxp->insertp(), vtxp->activity_ಸಂಕೇತ());
                }
                ಆದರೆ (setterp) {
                    Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ insertp = vtxp->insertp();
                    ಆದರೆ (Ast_Stmt_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ stmtp = VN_CAST(insertp, Stmt_ಅಭಿವ್ಯಕ್ತಿ)) {
                        stmtp->ಕೂಡಿಸು_ಮುಂದಿನ_ಇಲ್ಲಿ(setterp);
                    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (AstCFunc* ಸ್ಥಿರಾಂಕ funcp = VN_CAST(insertp, CFunc)) {
                        // If there are awaits, insert the setter after each await
                        ಆದರೆ (funcp->isCoroutine() && funcp->stmtsp()) {
                            funcp->stmtsp()->foreach_And_ಮುಂದಿನ([setterp](AstCAwait* awaitp) {
                                awaitp->ಕೂಡಿಸು_ಮುಂದಿನ_ಇಲ್ಲಿ(setterp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು));
                            });
                        }
                        funcp->ಕೂಡಿಸು_Stmtsp(setterp);
                    } ಇಲ್ಲದಿದ್ದರೆ {
                        insertp->v3fatalSrc("Bad trace activity vertex");
                    }
                }
            }
        }
    }

    AstCFunc* newCFunc(VTraceType traceType, AstCFunc* ತುದಿ_Funcp, uint32_t func_ಅಂಕೆ,
                       uint32_t ತಳ_ಸಂಕೇತ = 0, ಸ್ಥಿರಾಂಕ AstTraceDecl* ಸ್ಥಿರಾಂಕ declp = ಶೂನ್ಯಸೂಚಕ,
                       ತಾರ್ಕಿಕ declSub = ಸುಳ್ಳು) {
        // Create new function
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ is_ತುದಿ_Func = !declp && ತುದಿ_Funcp == ಶೂನ್ಯಸೂಚಕ;
        std::string funcName;
        ಆದರೆ (is_ತುದಿ_Func || declp) {
            funcName = traceType.func_ಪೂರ್ವಪ್ರತ್ಯಯ();
        } ಇಲ್ಲದಿದ್ದರೆ {
            funcName = ತುದಿ_Funcp->name();
            funcName += "_sub";
        }
        ಆದರೆ (declp) {
            funcName += "_dtype";
            ಆದರೆ (declSub) funcName += "_sub";
            funcName += "__";
            funcName = m_dtype_ಹೆಸರುಗಳು.get(funcName);
        } ಇಲ್ಲದಿದ್ದರೆ {
            funcName += "_";
            funcName += cvt_ಗಮ್ಯ_Str(func_ಅಂಕೆ);
        }

        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = m_ತುದಿ_Scopep->fileline();
        AstCFunc* ಸ್ಥಿರಾಂಕ funcp = ಹೊಸ AstCFunc{flp, funcName, m_ತುದಿ_Scopep};
        funcp->isTrace(ನಿಜ);
        funcp->dontCombine(ನಿಜ);
        funcp->isLoose(ನಿಜ);
        funcp->slow(traceType != VTraceType::CHANGE);
        funcp->isStatic(is_ತುದಿ_Func);
        // Add it to top scope
        m_ತುದಿ_Scopep->ಕೂಡಿಸು_Blocksp(funcp);
        ಸ್ಥಿರಾಂಕ std::string bufArg
            = v3Global.opt.trace_Class_ತಳ() + "::Buffer* bufp"
              + (declp ? (", uint32_t offset, const " + declp->dtypep()->cType("", ನಿಜ, ನಿಜ)
                          + " __VdtypeVar")
                       : "");
        ಆದರೆ (is_ತುದಿ_Func) {
            // Top functions
            funcp->argTypes("void* voidSelf, " + bufArg);
            funcp->ಕೂಡಿಸು_Stmtsp(ಹೊಸ AstCStmt{flp, EmitCUtil::void_Self_ನಿಯೋಜಿಸು(m_ತುದಿ_Modp)});
            funcp->ಕೂಡಿಸು_Stmtsp(ಹೊಸ AstCStmt{flp, EmitCUtil::sym_Class_ನಿಯೋಜಿಸು()});
            // Add global activity check to change dump functions
            ಆದರೆ (traceType == VTraceType::CHANGE) {  //
                funcp->ಕೂಡಿಸು_Stmtsp(
                    ಹೊಸ AstCStmt{flp, "if (VL_UNLIKELY(!vlSymsp->__Vm_activity)) return;"});
            }
            // Register function
            AstCStmt* ಸ್ಥಿರಾಂಕ cstmtp = ಹೊಸ AstCStmt{flp};
            m_regFuncp->ಕೂಡಿಸು_Stmtsp(cstmtp);
            ಆದರೆ (traceType == VTraceType::CONSTANT) {
                cstmtp->ಕೂಡಿಸು("tracep->addConstCb(");
                cstmtp->ಕೂಡಿಸು(ಹೊಸ Ast_Addr_ದ_CFunc{flp, funcp});
                cstmtp->ಕೂಡಿಸು(", " + std::ಗಮ್ಯ_string(func_ಅಂಕೆ) + ", vlSelf);");
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (traceType == VTraceType::ತುಂಬಿದ) {
                cstmtp->ಕೂಡಿಸು("tracep->addFullCb(");
                cstmtp->ಕೂಡಿಸು(ಹೊಸ Ast_Addr_ದ_CFunc{flp, funcp});
                cstmtp->ಕೂಡಿಸು(", " + std::ಗಮ್ಯ_string(func_ಅಂಕೆ) + ", vlSelf);");
            } ಇಲ್ಲದಿದ್ದರೆ {
                cstmtp->ಕೂಡಿಸು("tracep->addChgCb(");
                cstmtp->ಕೂಡಿಸು(ಹೊಸ Ast_Addr_ದ_CFunc{flp, funcp});
                cstmtp->ಕೂಡಿಸು(", " + std::ಗಮ್ಯ_string(func_ಅಂಕೆ) + ", vlSelf);");
            }
        } ಇಲ್ಲದಿದ್ದರೆ {
            // Sub functions
            funcp->argTypes(bufArg);
            // Setup base references. Note in rare occasions we can end up with an empty trace
            // sub function, hence the VL_ATTR_UNUSED attributes.
            ಆದರೆ (traceType != VTraceType::CHANGE) {
                // Full dump sub function
                funcp->ಕೂಡಿಸು_Stmtsp(ಹೊಸ AstCStmt{flp,  //
                                              string("uint32_t* const oldp VL_ATTR_UNUSED = "
                                                     "bufp->oldp(vlSymsp->__Vm_baseCode")
                                                  + (declp ? " + offset" : "") + ");\n"});
            } ಇಲ್ಲದಿದ್ದರೆ {
                // Change dump sub function
                funcp->ಕೂಡಿಸು_Stmtsp(ಹೊಸ AstCStmt{flp,  //
                                              "uint32_t* const oldp VL_ATTR_UNUSED = "
                                              "bufp->oldp(vlSymsp->__Vm_baseCode + "
                                                  + (declp ? " offset" : cvt_ಗಮ್ಯ_Str(ತಳ_ಸಂಕೇತ))
                                                  + ");\n"});
            }
            ಆದರೆ (!declp) {
                // Add call to top function
                AstCCall* ಸ್ಥಿರಾಂಕ callp = ಹೊಸ AstCCall{funcp->fileline(), funcp};
                callp->dtype_ಗಣ_Void();
                callp->argTypes("bufp");
                ತುದಿ_Funcp->ಕೂಡಿಸು_Stmtsp(callp->makeStmt());
            }
        }
        //ಮುಗಿದ
        UINFO(5, "  newCFunc " << funcp);
        ಹಿಂತಿರುಗಿ funcp;
    }

    AstCFunc* newCDtypeSubFunc(VTraceType traceType, ಸ್ಥಿರಾಂಕ AstTraceDecl* ಸ್ಥಿರಾಂಕ declp,
                               AstCFunc* parentp) {
        AstCFunc* ಸ್ಥಿರಾಂಕ funcp = newCFunc(traceType, ಶೂನ್ಯಸೂಚಕ, 0, 0, declp, ನಿಜ);
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = m_ತುದಿ_Scopep->fileline();
        AstCCall* ಸ್ಥಿರಾಂಕ callp = ಹೊಸ AstCCall{flp, funcp};
        callp->dtype_ಗಣ_Void();
        callp->argTypes("bufp, offset, __VdtypeVar");
        parentp->ಕೂಡಿಸು_Stmtsp(callp->makeStmt());
        ಹಿಂತಿರುಗಿ funcp;
    }

    AstCFunc* createConstDtypeTraceFunctions(ಸ್ಥಿರಾಂಕ AstTraceDecl* declp) {
        ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ dtypep = declp->dtypep()->skipRefp();
        ಸ್ವಯಂಚಾಲಿತ pair = m_dtypeConstFuncs.emplace(dtypep, ಶೂನ್ಯಸೂಚಕ);
        ಆದರೆ (pair.ಎರಡನೇ) {
            File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = declp->fileline();
            AstCFunc* ಸ್ಥಿರಾಂಕ funcp = newCFunc(VTraceType::CONSTANT, ಶೂನ್ಯಸೂಚಕ, 0, 0, declp);

            ತಾರ್ಕಿಕ ಮೊದಲ = ನಿಜ;
            ಗಾಗಿ (Ast_ಗಂಟು* callStmtp = declp->dtypeCallp()->funcp()->stmtsp(); callStmtp;
                 callStmtp = callStmtp->nextp()) {
                ಸ್ಥಿರಾಂಕ AstCCall* ಸ್ಥಿರಾಂಕ callp
                    = VN_CAST(VN_CAST(callStmtp, Stmt_ಅಭಿವ್ಯಕ್ತಿ)->exprp(), CCall);
                ತಾರ್ಕಿಕ onlyOne = ಮೊದಲ && !callStmtp->nextp();
                AstCFunc* ಸ್ಥಿರಾಂಕ subFuncp
                    = onlyOne ? funcp : newCDtypeSubFunc(VTraceType::CONSTANT, declp, funcp);
                ಗಾಗಿ (Ast_ಗಂಟು* stmtp = callp->funcp()->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    ಆದರೆ (AstTraceDecl* ಸ್ಥಿರಾಂಕ ಕ್ಷೇತ್ರ_Declp = VN_CAST(stmtp, TraceDecl)) {
                        AstTraceInc* ಸ್ಥಿರಾಂಕ incp
                            = ಹೊಸ AstTraceInc{flp, ಕ್ಷೇತ್ರ_Declp, VTraceType::CONSTANT};
                        subFuncp->ಕೂಡಿಸು_Stmtsp(incp);
                    }
                }
                ಮೊದಲ = ಸುಳ್ಳು;
            }

            pair.ಮೊದಲ->ಎರಡನೇ = funcp;
        }

        ಹಿಂತಿರುಗಿ pair.ಮೊದಲ->ಎರಡನೇ;
    }

    ಶೂನ್ಯ createConstTraceFunctions(ಸ್ಥಿರಾಂಕ TraceVec& traces) {
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ವಿಭಜಿಸು_ಎಲ್ಲೆ = v3Global.opt.output_ವಿಭಜಿಸು_CTrace() ? v3Global.opt.output_ವಿಭಜಿಸು_CTrace()
                                                                : std::numeric_limits<ಪೂರ್ಣಾಂಕ>::max();

        AstCFunc* ಸ್ಥಿರಾಂಕ ತುದಿ_Funcp = newCFunc(VTraceType::CONSTANT, ಶೂನ್ಯಸೂಚಕ, 0);
        uint32_t sub_Func_ಅಂಕೆ = 0;
        AstCFunc* subFuncp = ಶೂನ್ಯಸೂಚಕ;
        ಪೂರ್ಣಾಂಕ subStmts = 0;
        std::ಸದಿಶ<ಸ್ಥಿರಾಂಕ Trace_Trace_ಶೃಂಗ*> duplicates;
        ಗಾಗಿ (ಸ್ವಯಂಚಾಲಿತ it = traces.cbegin(); it != traces.ಕೊನೆ(); ++it) {
            ಸ್ಥಿರಾಂಕ Trace_Trace_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vtxp = it->ಎರಡನೇ;
            AstTraceDecl* ಸ್ಥಿರಾಂಕ declp = vtxp->nodep();
            ಆದರೆ (vtxp->duplicatep()) {
                duplicates.ತಳ್ಳು_ಹಿಂಭಾಗ(vtxp);
                ಮುಂದುವರಿ;
            }

            // This is a canonical trace node. Assign trace code (signal number).
            UASSERT_OBJ(!declp->ಸಂಕೇತ_Assigned(), declp,
                        "Canonical node should not have code assigned yet");
            declp->ಸಂಕೇತ_2(m_ಸಂಕೇತ);
            ಸ್ಥಿರಾಂಕ uint32_t ಸಂಕೇತ_Inc = declp->ಸಂಕೇತ_Inc();
            m_ಸಂಕೇತ += ಸಂಕೇತ_Inc;
            ++m_statUniqSigs;

            // If this is a const signal, add the AstTraceInc
            ಸ್ಥಿರಾಂಕ Act_ಸಂಕೇತ_ಗಣ& act_ಗಣ = it->ಮೊದಲ;
            ಆದರೆ (act_ಗಣ.ಎಣಿಕೆ_2(Trace_Activity_ಶೃಂಗ::ACTIVITY_NEVER)) {
                // Crate new sub function if required
                ಆದರೆ (!subFuncp || subStmts > ವಿಭಜಿಸು_ಎಲ್ಲೆ) {
                    subStmts = 0;
                    subFuncp = newCFunc(VTraceType::CONSTANT, ತುದಿ_Funcp, sub_Func_ಅಂಕೆ);
                    ++sub_Func_ಅಂಕೆ;
                }
                File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = declp->fileline();
                ಆದರೆ (declp->dtypeCallp()) {
                    AstCFunc* ಸ್ಥಿರಾಂಕ funcp = createConstDtypeTraceFunctions(declp);
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* argsp = ಶೂನ್ಯಸೂಚಕ;
                    argsp = Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ(argsp, declp->valuep()->clone_ವೃಕ್ಷ(ಸುಳ್ಳು));
                    AstCCall* ಸ್ಥಿರಾಂಕ callp = ಹೊಸ AstCCall{flp, funcp, argsp};
                    callp->dtype_ಗಣ_Void();
                    callp->argTypes(callp->argTypes() + "bufp, " + std::ಗಮ್ಯ_string(declp->ಸಂಕೇತ_2()));
                    subFuncp->ಕೂಡಿಸು_Stmtsp(callp->makeStmt());

                    subStmts += 1;
                } ಇಲ್ಲದಿದ್ದರೆ {
                    AstTraceInc* ಸ್ಥಿರಾಂಕ incp = ಹೊಸ AstTraceInc{flp, declp, VTraceType::CONSTANT};
                    subFuncp->ಕೂಡಿಸು_Stmtsp(incp);
                    subStmts += incp->ಗಂಟು_ಎಣಿಕೆ();
                }
            }
        }

        // Assign codes to duplicate nodes from their canonicals
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Trace_Trace_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vtxp : duplicates) {
            AstTraceDecl* ಸ್ಥಿರಾಂಕ declp = vtxp->nodep();
            ಸ್ಥಿರಾಂಕ Trace_Trace_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ canonVtxp = vtxp->duplicatep();
            ಸ್ಥಿರಾಂಕ AstTraceDecl* ಸ್ಥಿರಾಂಕ canonDeclp = canonVtxp->nodep();
            UASSERT_OBJ(!canonVtxp->duplicatep(), canonDeclp, "Canonical node is a duplicate");
            UASSERT_OBJ(canonDeclp->ಸಂಕೇತ_Assigned(), canonDeclp,
                        "Canonical node should have code assigned already");
            declp->ಸಂಕೇತ_2(canonDeclp->ಸಂಕೇತ_2() + vtxp->dtype_Alias_ಸ್ಥಾನಾಂತರ());
        }
    }

    DtypeFuncs createNonConstDtypeTraceFunctions(ಸ್ಥಿರಾಂಕ AstTraceDecl* declp) {
        Ast_ಗಂಟು_DType* dtypep = declp->dtypep()->skipRefp();
        ಸ್ವಯಂಚಾಲಿತ pair = m_dtypeNonConstFuncs.emplace(dtypep, DtypeFuncs{});
        ಆದರೆ (pair.ಎರಡನೇ) {
            File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = declp->fileline();
            AstCFunc* ಸ್ಥಿರಾಂಕ ತುಂಬಿದ_Funcp = newCFunc(VTraceType::ತುಂಬಿದ, ಶೂನ್ಯಸೂಚಕ, 0, 0, declp);
            AstCFunc* ಸ್ಥಿರಾಂಕ chgFuncp = newCFunc(VTraceType::CHANGE, ಶೂನ್ಯಸೂಚಕ, 0, 0, declp);

            ತಾರ್ಕಿಕ ಮೊದಲ = ನಿಜ;
            ಗಾಗಿ (Ast_ಗಂಟು* callStmtp = declp->dtypeCallp()->funcp()->stmtsp(); callStmtp;
                 callStmtp = callStmtp->nextp()) {
                ಸ್ಥಿರಾಂಕ AstCCall* ಸ್ಥಿರಾಂಕ callp
                    = VN_CAST(VN_CAST(callStmtp, Stmt_ಅಭಿವ್ಯಕ್ತಿ)->exprp(), CCall);
                ತಾರ್ಕಿಕ onlyOne = ಮೊದಲ && !callStmtp->nextp();
                AstCFunc* ಸ್ಥಿರಾಂಕ ತುಂಬಿದ_Sub_Funcp
                    = onlyOne ? ತುಂಬಿದ_Funcp : newCDtypeSubFunc(VTraceType::ತುಂಬಿದ, declp, ತುಂಬಿದ_Funcp);
                AstCFunc* ಸ್ಥಿರಾಂಕ chgSubFuncp
                    = onlyOne ? chgFuncp : newCDtypeSubFunc(VTraceType::CHANGE, declp, chgFuncp);
                ಗಾಗಿ (Ast_ಗಂಟು* stmtp = callp->funcp()->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    ಆದರೆ (AstTraceDecl* ಸ್ಥಿರಾಂಕ ಕ್ಷೇತ್ರ_Declp = VN_CAST(stmtp, TraceDecl)) {
                        AstTraceInc* ಸ್ಥಿರಾಂಕ incFullp
                            = ಹೊಸ AstTraceInc{flp, ಕ್ಷೇತ್ರ_Declp, VTraceType::ತುಂಬಿದ};
                        ತುಂಬಿದ_Sub_Funcp->ಕೂಡಿಸು_Stmtsp(incFullp);
                        AstTraceInc* ಸ್ಥಿರಾಂಕ incChgp
                            = ಹೊಸ AstTraceInc{flp, ಕ್ಷೇತ್ರ_Declp, VTraceType::CHANGE};
                        chgSubFuncp->ಕೂಡಿಸು_Stmtsp(incChgp);
                    }
                }
                ಮೊದಲ = ಸುಳ್ಳು;
            }

            pair.ಮೊದಲ->ಎರಡನೇ = {.ತುಂಬಿದ_Funcp = ತುಂಬಿದ_Funcp, .chgFuncp = chgFuncp};
        }

        ಹಿಂತಿರುಗಿ pair.ಮೊದಲ->ಎರಡನೇ;
    }

    ಶೂನ್ಯ createNonConstTraceFunctions(ಸ್ಥಿರಾಂಕ TraceVec& traces, uint32_t nAllCodes,
                                      uint32_t parallelism) {
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ವಿಭಜಿಸು_ಎಲ್ಲೆ = v3Global.opt.output_ವಿಭಜಿಸು_CTrace() ? v3Global.opt.output_ವಿಭಜಿಸು_CTrace()
                                                                : std::numeric_limits<ಪೂರ್ಣಾಂಕ>::max();

        // pre-incremented, so starts at 0
        uint32_t ತುದಿ_Func_ಅಂಕೆ = std::numeric_limits<uint32_t>::max();
        TraceVec::const_iterator it = traces.begin();
        ಆಗ (it != traces.ಕೊನೆ()) {
            AstCFunc* ತುದಿ_Ful_Funcp = ಶೂನ್ಯಸೂಚಕ;
            AstCFunc* ತುದಿ_Chg_Funcp = ಶೂನ್ಯಸೂಚಕ;
            AstCFunc* subFulFuncp = ಶೂನ್ಯಸೂಚಕ;
            AstCFunc* subChgFuncp = ಶೂನ್ಯಸೂಚಕ;
            uint32_t sub_Func_ಅಂಕೆ = 0;
            ಪೂರ್ಣಾಂಕ subStmts = 0;
            ಸ್ಥಿರಾಂಕ uint32_t maxCodes = std::max((nAllCodes + parallelism - 1) / parallelism, 1U);
            uint32_t nCodes = 0;
            ಸ್ಥಿರಾಂಕ Act_ಸಂಕೇತ_ಗಣ* prev_Act_ಗಣ = ಶೂನ್ಯಸೂಚಕ;
            AstIf* ifp = ಶೂನ್ಯಸೂಚಕ;
            uint32_t ತಳ_ಸಂಕೇತ = 0;
            ಗಾಗಿ (; nCodes < maxCodes && it != traces.ಕೊನೆ(); ++it) {
                ಸ್ಥಿರಾಂಕ Act_ಸಂಕೇತ_ಗಣ& act_ಗಣ = it->ಮೊದಲ;
                // Traced value never changes, no need to add it
                ಆದರೆ (act_ಗಣ.ಎಣಿಕೆ_2(Trace_Activity_ಶೃಂಗ::ACTIVITY_NEVER)) ಮುಂದುವರಿ;

                ಸ್ಥಿರಾಂಕ Trace_Trace_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vtxp = it->ಎರಡನೇ;
                AstTraceDecl* ಸ್ಥಿರಾಂಕ declp = vtxp->nodep();

                // This is a duplicate decl, no need to add it, but must set the
                // function index to the same as the canonical node.
                ಆದರೆ (ಸ್ಥಿರಾಂಕ Trace_Trace_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ canonVtxp = vtxp->duplicatep()) {
                    declp->fidx(canonVtxp->nodep()->fidx());
                    ಮುಂದುವರಿ;
                }

                // Create top function if not yet created
                ಆದರೆ (!ತುದಿ_Ful_Funcp) {
                    ++ತುದಿ_Func_ಅಂಕೆ;
                    ತುದಿ_Ful_Funcp = newCFunc(VTraceType::ತುಂಬಿದ, ಶೂನ್ಯಸೂಚಕ, ತುದಿ_Func_ಅಂಕೆ);
                    ತುದಿ_Chg_Funcp = newCFunc(VTraceType::CHANGE, ಶೂನ್ಯಸೂಚಕ, ತುದಿ_Func_ಅಂಕೆ);
                }

                // Create new sub function if required
                ಆದರೆ (!subFulFuncp || subStmts > ವಿಭಜಿಸು_ಎಲ್ಲೆ) {
                    ತಳ_ಸಂಕೇತ = declp->ಸಂಕೇತ_2();
                    subStmts = 0;
                    subFulFuncp = newCFunc(VTraceType::ತುಂಬಿದ, ತುದಿ_Ful_Funcp, sub_Func_ಅಂಕೆ, ತಳ_ಸಂಕೇತ);
                    subChgFuncp = newCFunc(VTraceType::CHANGE, ತುದಿ_Chg_Funcp, sub_Func_ಅಂಕೆ, ತಳ_ಸಂಕೇತ);
                    ++sub_Func_ಅಂಕೆ;
                    prev_Act_ಗಣ = ಶೂನ್ಯಸೂಚಕ;
                    ifp = ಶೂನ್ಯಸೂಚಕ;
                }

                // If required, create the conditional node checking the activity flags
                ಆದರೆ (!prev_Act_ಗಣ || act_ಗಣ != *prev_Act_ಗಣ) {
                    File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = m_ತುದಿ_Scopep->fileline();
                    ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ always = act_ಗಣ.ಎಣಿಕೆ_2(Trace_Activity_ಶೃಂಗ::ACTIVITY_ALWAYS) != 0;
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* condp = ಶೂನ್ಯಸೂಚಕ;
                    ಆದರೆ (always) {
                        condp = ಹೊಸ AstConst{flp, 1};  // Always true, will be folded later
                    } ಇಲ್ಲದಿದ್ದರೆ {
                        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ uint32_t act_ಸಂಕೇತ : act_ಗಣ) {
                            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ selp = selectActivity(flp, act_ಸಂಕೇತ, VAccess::READ);
                            condp = condp ? ಹೊಸ AstOr{flp, condp, selp} : selp;
                        }
                    }
                    ifp = ಹೊಸ AstIf{flp, condp};
                    ಆದರೆ (!always) ifp->branchPred(VBranchPred::BP_UNLIKELY);
                    subChgFuncp->ಕೂಡಿಸು_Stmtsp(ifp);
                    subStmts += ifp->ಗಂಟು_ಎಣಿಕೆ();
                    prev_Act_ಗಣ = &act_ಗಣ;
                }

                // Add TraceInc nodes
                File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = declp->fileline();
                ಆದರೆ (declp->dtypeCallp()) {
                    DtypeFuncs funcs = createNonConstDtypeTraceFunctions(declp);
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* argsp = ಶೂನ್ಯಸೂಚಕ;
                    argsp = Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ(argsp, declp->valuep()->clone_ವೃಕ್ಷ(ಸುಳ್ಳು));
                    AstCCall* ಸ್ಥಿರಾಂಕ callFullp = ಹೊಸ AstCCall{flp, funcs.ತುಂಬಿದ_Funcp, argsp};
                    callFullp->dtype_ಗಣ_Void();
                    callFullp->argTypes(callFullp->argTypes() + "bufp, "
                                        + std::ಗಮ್ಯ_string(declp->ಸಂಕೇತ_2()));
                    subFulFuncp->ಕೂಡಿಸು_Stmtsp(callFullp->makeStmt());
                    argsp = ಶೂನ್ಯಸೂಚಕ;
                    argsp = Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ(argsp, declp->valuep()->clone_ವೃಕ್ಷ(ಸುಳ್ಳು));
                    AstCCall* ಸ್ಥಿರಾಂಕ callChgp = ಹೊಸ AstCCall{flp, funcs.chgFuncp, argsp};
                    callChgp->dtype_ಗಣ_Void();
                    callChgp->argTypes(callChgp->argTypes() + "bufp, "
                                       + std::ಗಮ್ಯ_string(declp->ಸಂಕೇತ_2()));
                    ifp->ಕೂಡಿಸು_Thensp(callChgp->makeStmt());

                    subStmts += 2;
                } ಇಲ್ಲದಿದ್ದರೆ {
                    AstTraceInc* ಸ್ಥಿರಾಂಕ incFulp = ಹೊಸ AstTraceInc{flp, declp, VTraceType::ತುಂಬಿದ};
                    subFulFuncp->ಕೂಡಿಸು_Stmtsp(incFulp);
                    AstTraceInc* ಸ್ಥಿರಾಂಕ incChgp
                        = ಹೊಸ AstTraceInc{flp, declp, VTraceType::CHANGE, ತಳ_ಸಂಕೇತ};
                    ifp->ಕೂಡಿಸು_Thensp(incChgp);

                    // Set the function index of the decl
                    declp->fidx(ತುದಿ_Func_ಅಂಕೆ);

                    // Track splitting due to size
                    UASSERT_OBJ(incFulp->ಗಂಟು_ಎಣಿಕೆ() == incChgp->ಗಂಟು_ಎಣಿಕೆ(), declp,
                                "Should have equal cost");
                    ಸ್ಥಿರಾಂಕ VNumRange range = declp->arrayRange();
                    ಆದರೆ (range.ranged()) {
                        // 2x because each element is a TraceInc and a VarRef
                        subStmts += range.elements() * 2;
                    } ಇಲ್ಲದಿದ್ದರೆ {
                        subStmts += incChgp->ಗಂಟು_ಎಣಿಕೆ();
                    }
                }

                // Track partitioning
                nCodes += declp->ಸಂಕೇತ_Inc();
            }
        }
    }

    ಶೂನ್ಯ createCleanupFunction() {
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = m_ತುದಿ_Scopep->fileline();
        AstCFunc* ಸ್ಥಿರಾಂಕ cleanupFuncp = ಹೊಸ AstCFunc{fl, "trace_cleanup", m_ತುದಿ_Scopep};
        cleanupFuncp->argTypes("void* voidSelf, " + v3Global.opt.trace_Class_ತಳ()
                               + "* /*unused*/");
        cleanupFuncp->isTrace(ನಿಜ);
        cleanupFuncp->slow(ಸುಳ್ಳು);
        cleanupFuncp->isStatic(ನಿಜ);
        cleanupFuncp->isLoose(ನಿಜ);
        m_ತುದಿ_Scopep->ಕೂಡಿಸು_Blocksp(cleanupFuncp);
        cleanupFuncp->ಕೂಡಿಸು_Stmtsp(ಹೊಸ AstCStmt{fl, EmitCUtil::void_Self_ನಿಯೋಜಿಸು(m_ತುದಿ_Modp)});
        cleanupFuncp->ಕೂಡಿಸು_Stmtsp(ಹೊಸ AstCStmt{fl, EmitCUtil::sym_Class_ನಿಯೋಜಿಸು()});

        // Register it
        {
            AstCStmt* ಸ್ಥಿರಾಂಕ cstmtp = ಹೊಸ AstCStmt{fl};
            m_regFuncp->ಕೂಡಿಸು_Stmtsp(cstmtp);
            cstmtp->ಕೂಡಿಸು("tracep->addCleanupCb(");
            cstmtp->ಕೂಡಿಸು(ಹೊಸ Ast_Addr_ದ_CFunc{fl, cleanupFuncp});
            cstmtp->ಕೂಡಿಸು(", vlSelf);");
        }

        // Clear global activity flag
        cleanupFuncp->ಕೂಡಿಸು_Stmtsp(
            ಹೊಸ AstCStmt{m_ತುದಿ_Scopep->fileline(), "vlSymsp->__Vm_activity = false;"s});

        // Clear fine grained activity flags
        ಗಾಗಿ (uint32_t i = 0; i < m_activity_ಸಂಖ್ಯೆ; ++i) {
            Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ clrp = ಹೊಸ Ast_ನಿಯೋಜಿಸು{fl, selectActivity(fl, i, VAccess::WRITE),
                                                ಹೊಸ AstConst{fl, AstConst::ಬಿಟ್_False{}}};
            cleanupFuncp->ಕೂಡಿಸು_Stmtsp(clrp);
        }
    }

    ಶೂನ್ಯ createTraceFunctions() {
        // Detect and remove duplicate values
        detectDuplicates();

        ನಕ್ಷೆ_Dtype_Prune();
        ವಿಭಜಿಸು_Trace_Decl_Funcs();
        m_ನಕ್ಷೆ.remove_Redundant_ಅಂಚುಗಳು_Max(&V3_ನಕ್ಷೆ_ಅಂಚು::followAlwaysTrue);

        // Simplify & optimize the graph
        ಆದರೆ (dump_ನಕ್ಷೆ_Level() >= 6) m_ನಕ್ಷೆ.dumpDotFilePrefixed("trace_pre");
        ನಕ್ಷೆ_Simplify(ನಿಜ);
        ಆದರೆ (dump_ನಕ್ಷೆ_Level() >= 6) m_ನಕ್ಷೆ.dumpDotFilePrefixed("trace_simplified");
        ನಕ್ಷೆ_Optimize();
        ಆದರೆ (dump_ನಕ್ಷೆ_Level() >= 6) m_ನಕ್ಷೆ.dumpDotFilePrefixed("trace_optimized");

        // Create the fine grained activity flags
        createActivityFlags();

        // Form a sorted list of the traces we are interested in
        TraceVec traces;  // The sorted traces
        // We will split functions such that each have to dump roughly the same amount of data
        // for this we need to keep tack of the number of codes used by the trace functions.
        uint32_t nNonConstCodes = 0;
        ವಿಂಗಡಿಸು_Traces(traces, nNonConstCodes);

        // Our keys are now sorted to have same activity number adjacent, then
        // by trace order. (Better would be execution order for cache
        // efficiency....) Last are constants and non-changers, as then the
        // last value vector is more compact

        // Create the trace registration function
        m_regFuncp = ಹೊಸ AstCFunc{m_ತುದಿ_Scopep->fileline(), "trace_register", m_ತುದಿ_Scopep};
        m_regFuncp->argTypes(v3Global.opt.trace_Class_ತಳ() + "* tracep");
        m_regFuncp->ನಮೂದು_ಬಿಂದು(ನಿಜ);
        m_regFuncp->isTrace(ನಿಜ);
        m_regFuncp->slow(ನಿಜ);
        m_regFuncp->isStatic(ಸುಳ್ಳು);
        m_regFuncp->isLoose(ನಿಜ);
        m_ತುದಿ_Scopep->ಕೂಡಿಸು_Blocksp(m_regFuncp);

        // Create the const dump functions. Also allocates trace codes.
        createConstTraceFunctions(traces);

        // Create the full and incremental dump functions
        createNonConstTraceFunctions(traces, nNonConstCodes, m_parallelism);

        // Root-traced libraries alias wrapper IOs onto the existing top-module codes.
        ಆದರೆ (!v3Global.opt.libCreate().ಖಾಲಿಗಳು()) {
            std::ಸದಿಶ<AstTraceDecl*> ಬೇರು_Declps;
            std::ಸದಿಶ<AstTraceDecl*> ತುದಿ_Declps;
            TraceInitDeclCollector ಬೇರು_Collector{ಬೇರು_Declps};
            TraceInitDeclCollector ತುದಿ_Collector{ತುದಿ_Declps};
            ಗಾಗಿ (Ast_ಗಂಟು* blockp = m_ತುದಿ_Scopep->blocksp(); blockp; blockp = blockp->nextp()) {
                AstCFunc* ಸ್ಥಿರಾಂಕ funcp = VN_CAST(blockp, CFunc);
                ಆದರೆ (funcp && VString::startsWith(funcp->name(), "trace_init_leaf_root__")) {
                    ಬೇರು_Collector.collect(funcp);
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (funcp && VString::startsWith(funcp->name(), "trace_init_leaf_top__")) {
                    ತುದಿ_Collector.collect(funcp);
                }
            }

            std::ಸದಿಶ<ತಾರ್ಕಿಕ> ಬಳಸಿದ(ತುದಿ_Declps.ಗಾತ್ರ_2(), ಸುಳ್ಳು);
            ಗಾಗಿ (AstTraceDecl* ಸ್ಥಿರಾಂಕ ಬೇರು_Declp : ಬೇರು_Declps) {
                ಗಾಗಿ (ಗಾತ್ರ_t i = 0; i < ತುದಿ_Declps.ಗಾತ್ರ_2(); ++i) {
                    ಆದರೆ (ಬಳಸಿದ[i] || !same_ಬೇರು_Init_Alias(ಬೇರು_Declp, ತುದಿ_Declps[i])) ಮುಂದುವರಿ;
                    ಬೇರು_Declp->ಸಂಕೇತ_2(ತುದಿ_Declps[i]->ಸಂಕೇತ_2());
                    ಬಳಸಿದ[i] = ನಿಜ;
                    ನಿಲ್ಲಿಸು;
                }
            }
        }

        // Remove refs to traced values from TraceDecl nodes, these have now moved under
        // TraceInc
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& i : traces) {
            Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ valuep = i.ಎರಡನೇ->nodep()->valuep();
            VL_DO_DANGLING(valuep->unlink_Fr_ಹಿಂಭಾಗ()->delete_ವೃಕ್ಷ(), valuep);
        }

        // Create the trace cleanup function clearing the activity flags
        createCleanupFunction();
    }

    Trace_CFunc_ಶೃಂಗ* getCFuncVertexp(AstCFunc* nodep) {
        V3_ನಕ್ಷೆ_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vtxp = nodep->user1u().ಗಮ್ಯ_ನಕ್ಷೆ_ಶೃಂಗ();
        Trace_CFunc_ಶೃಂಗ* vertexp = vtxp ? vtxp->cast<Trace_CFunc_ಶೃಂಗ>() : ಶೂನ್ಯಸೂಚಕ;
        ಆದರೆ (!vertexp) {
            vertexp = ಹೊಸ Trace_CFunc_ಶೃಂಗ{&m_ನಕ್ಷೆ, nodep};
            nodep->user1p(vertexp);
        }
        ಹಿಂತಿರುಗಿ vertexp;
    }
    Trace_Activity_ಶೃಂಗ* getActivityVertexp(Ast_ಗಂಟು* nodep, ತಾರ್ಕಿಕ slow) {
        V3_ನಕ್ಷೆ_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vtxp = nodep->user3u().ಗಮ್ಯ_ನಕ್ಷೆ_ಶೃಂಗ();
        Trace_Activity_ಶೃಂಗ* vertexp = vtxp ? vtxp->cast<Trace_Activity_ಶೃಂಗ>() : ಶೂನ್ಯಸೂಚಕ;
        ಆದರೆ (!vertexp) {
            vertexp = ಹೊಸ Trace_Activity_ಶೃಂಗ{&m_ನಕ್ಷೆ, nodep, slow};
            nodep->user3p(vertexp);
        }
        vertexp->slow(slow);
        ಹಿಂತಿರುಗಿ vertexp;
    }

    // VISITORS
    ಶೂನ್ಯ ಭೇಟಿ(AstNetlist* nodep) override {
        // Add vertexes for all TraceDecl, and edges from VARs each trace looks at
        m_finding = ಸುಳ್ಳು;
        iterateChildren(nodep);

        // Add vertexes for all CFUNCs, and edges to VARs the func sets
        m_finding = ನಿಜ;
        iterateChildren(nodep);

        // Create the trace functions and insert them into the tree
        createTraceFunctions();

        // Save number of trace codes used
        nodep->nTraceCodes(m_ಸಂಕೇತ);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Module* nodep) override {
        ಆದರೆ (nodep->is_ತುದಿ()) m_ತುದಿ_Modp = nodep;
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstVarScope* nodep) override {
        ಆದರೆ (!m_finding) {
            ಆದರೆ (ಸ್ಥಿರಾಂಕ AstIface* ಸ್ಥಿರಾಂಕ ifacep = nodep->varp()->sensIfacep()) {
                m_ifaceMemberVscps[{ifacep, nodep->varp()->name()}].ತಳ್ಳು_ಹಿಂಭಾಗ(nodep);
            }
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_Stmt_ಅಭಿವ್ಯಕ್ತಿ* nodep) override {
        ಆದರೆ (!m_finding && !nodep->user2()) {
            ಆದರೆ (AstCCall* ಸ್ಥಿರಾಂಕ callp = VN_CAST(nodep->exprp(), CCall)) {
                UINFO(8, "   CCALL " << callp);
                // See if there are other calls in same statement list;
                // If so, all funcs might share the same activity code
                Trace_Activity_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ activityVtxp
                    = getActivityVertexp(nodep, callp->funcp()->slow());
                ಗಾಗಿ (Ast_ಗಂಟು* nextp = nodep; nextp; nextp = nextp->nextp()) {
                    ಆದರೆ (Ast_Stmt_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ stmtp = VN_CAST(nextp, Stmt_ಅಭಿವ್ಯಕ್ತಿ)) {
                        ಆದರೆ (AstCCall* ಸ್ಥಿರಾಂಕ ccallp = VN_CAST(stmtp->exprp(), CCall)) {
                            stmtp->user2(ನಿಜ);  // Processed
                            UINFO(8, "     SubCCALL " << ccallp);
                            V3_ನಕ್ಷೆ_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ ccallFuncVtxp = getCFuncVertexp(ccallp->funcp());
                            activityVtxp->slow(ccallp->funcp()->slow());
                            ಹೊಸ V3_ನಕ್ಷೆ_ಅಂಚು{&m_ನಕ್ಷೆ, activityVtxp, ccallFuncVtxp, 1};
                        }
                    }
                }
            }
        }
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCFunc* nodep) override {
        UINFO(8, "   CFUNC " << nodep);
        V3_ನಕ್ಷೆ_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ funcVtxp = getCFuncVertexp(nodep);
        ಆದರೆ (!m_finding) {  // If public, we need a unique activity code to allow for sets
                           // directly in this func
            ಆದರೆ (nodep->func_ಬಹಿರಂಗ() || nodep->dpiExportImpl() || nodep->ನಮೂದು_ಬಿಂದು()
                || nodep->isCoroutine()) {
                // Cannot treat a coroutine as slow, it may be resumed later
                ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ slow = nodep->slow() && !nodep->isCoroutine();
                Trace_Activity_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ activityVtxp = getActivityVertexp(nodep, slow);
                ಹೊಸ V3_ನಕ್ಷೆ_ಅಂಚು{&m_ನಕ್ಷೆ, activityVtxp, funcVtxp, 1};
            }
        }
        VL_RESTORER(m_cfuncp);
        m_cfuncp = nodep;
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstTraceDecl* nodep) override {
        UINFO(8, "   TRACE " << nodep);
        ಆದರೆ (!m_finding && !nodep->ಒಳ_Dtype_Func()) {
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
            ಆದರೆ (m_cfuncp && VString::startsWith(m_cfuncp->name(), "trace_init_leaf_root__")) {
                ಹಿಂತಿರುಗಿ;
            }
            V3_ನಕ್ಷೆ_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vertexp = ಹೊಸ Trace_Trace_ಶೃಂಗ{&m_ನಕ್ಷೆ, nodep};
            nodep->user1p(vertexp);

            UASSERT_OBJ(m_cfuncp, nodep, "Trace not under func");
            m_cfuncp->user2(ನಿಜ);
            VL_RESTORER(m_tracep);
            m_tracep = nodep;
            iterateChildren(nodep);
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstVarRef* nodep) override {
        UASSERT_OBJ(nodep->varScopep(), nodep, "No var scope?");
        AstVarScope* ಸ್ಥಿರಾಂಕ varscopep = nodep->varScopep();
        V3_ನಕ್ಷೆ_ಶೃಂಗ* varVtxp = varscopep->user1u().ಗಮ್ಯ_ನಕ್ಷೆ_ಶೃಂಗ();
        ಆದರೆ (m_tracep) {
            UASSERT_OBJ(nodep->access().isReadOnly(), nodep, "Lvalue in trace?  Should be const.");
            ಆದರೆ (!varVtxp) {
                varVtxp = ಹೊಸ Trace_Var_ಶೃಂಗ{&m_ನಕ್ಷೆ, nodep->varScopep()};
                nodep->varScopep()->user1p(varVtxp);
            }
            V3_ನಕ್ಷೆ_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ traceVtxp = m_tracep->user1u().ಗಮ್ಯ_ನಕ್ಷೆ_ಶೃಂಗ();
            ಹೊಸ V3_ನಕ್ಷೆ_ಅಂಚು{&m_ನಕ್ಷೆ, varVtxp, traceVtxp, 1};
            ಆದರೆ (nodep->varp()->isPrimaryInish()  // Always need to trace primary inputs
                || nodep->varp()->is_Sig_ಬಹಿರಂಗ()) {  // Or ones user can change
                ಹೊಸ V3_ನಕ್ಷೆ_ಅಂಚು{&m_ನಕ್ಷೆ, m_alwaysVtxp, traceVtxp, 1};
            }
            ಆದರೆ (m_tracep->dtypeCallp()) varscopep->user2p(m_tracep);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (m_cfuncp && m_finding && nodep->access().isWriteOrRW()) {
            V3_ನಕ್ಷೆ_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ funcVtxp = getCFuncVertexp(m_cfuncp);
            ಆದರೆ (varVtxp) {  // else we're not tracing this signal
                ಹೊಸ V3_ನಕ್ಷೆ_ಅಂಚು{&m_ನಕ್ಷೆ, funcVtxp, varVtxp, 1};
                AstTraceDecl* ಸ್ಥಿರಾಂಕ declp = VN_AS(varscopep->user2p(), TraceDecl);
                ಆದರೆ (declp) {
                    V3_ನಕ್ಷೆ_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ cFuncVtxp = getCFuncVertexp(m_cfuncp);
                    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಅಂಚು& ಅಂಚು : cFuncVtxp->ಒಳ_ಅಂಚುಗಳು()) {
                        V3_ನಕ್ಷೆ_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ activityp = ಅಂಚು.fromp();
                        ಆದರೆ (!declp->user3p()) {
                            declp->user3p(activityp);
                        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (declp->user3u().ಗಮ್ಯ_ನಕ್ಷೆ_ಶೃಂಗ() != activityp) {
                            declp->user2(ನಿಜ);
                        }
                    }
                }
            }
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstMemberSel* nodep) override {
        ಆದರೆ (m_cfuncp && m_finding && nodep->access().isWriteOrRW()) {
            AstIfaceRefDType* ಸ್ಥಿರಾಂಕ dtypep
                = VN_CAST(nodep->fromp()->dtypep()->skipRefp(), IfaceRefDType);
            ಆದರೆ (dtypep && dtypep->isVirtual()) {
                ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ it = m_ifaceMemberVscps.ಕಂಡುಹಿಡಿ({dtypep->ifacep(), nodep->varp()->name()});
                ಆದರೆ (it != m_ifaceMemberVscps.ಕೊನೆ()) {
                    V3_ನಕ್ಷೆ_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ funcVtxp = getCFuncVertexp(m_cfuncp);
                    ಗಾಗಿ (AstVarScope* ಸ್ಥಿರಾಂಕ vscp : it->ಎರಡನೇ) {
                        V3_ನಕ್ಷೆ_ಶೃಂಗ* varVtxp = vscp->user1u().ಗಮ್ಯ_ನಕ್ಷೆ_ಶೃಂಗ();
                        ಆದರೆ (!varVtxp) {
                            varVtxp = ಹೊಸ Trace_Var_ಶೃಂಗ{&m_ನಕ್ಷೆ, vscp};
                            vscp->user1p(varVtxp);
                        }
                        ಹೊಸ V3_ನಕ್ಷೆ_ಅಂಚು{&m_ನಕ್ಷೆ, funcVtxp, varVtxp, 1};
                    }
                }
            }
        }
        iterateChildren(nodep);
    }
    //--------------------
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    ಸ್ಪಷ್ಟ TraceVisitor(AstNetlist* nodep)
        : m_alwaysVtxp{ಹೊಸ Trace_Activity_ಶೃಂಗ{&m_ನಕ್ಷೆ, Trace_Activity_ಶೃಂಗ::ACTIVITY_ALWAYS}} {
        nodep->user2_Clear_ವೃಕ್ಷ();  // TraceDecl multiple activities flag
        nodep->user3_Clear_ವೃಕ್ಷ();  // TraceDecl TraceActivityVertex (assumes we start at nullptr)
        iterate(nodep);
        nodep->foreach([](AstTraceDecl* ಸ್ಥಿರಾಂಕ declp) {
            ಆದರೆ (declp->ಒಳ_Dtype_Func()) {
                declp->valuep()->unlink_Fr_ಹಿಂಭಾಗ()->delete_ವೃಕ್ಷ();
                declp->valuep(ಶೂನ್ಯಸೂಚಕ);
            }
        });
    }
    ~TraceVisitor() override {
        V3Stats::ಕೂಡಿಸು_Stat("Tracing, Activity setters", m_statSetters);
        V3Stats::ಕೂಡಿಸು_Stat("Tracing, Activity slow blocks", m_statSettersSlow);
        V3Stats::ಕೂಡಿಸು_Stat("Tracing, Unique trace codes", m_ಸಂಕೇತ);
        V3Stats::ಕೂಡಿಸು_Stat("Tracing, Unique traced signals", m_statUniqSigs);
    }
};

//######################################################################
// Trace class functions

ಶೂನ್ಯ V3Trace::traceAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { TraceVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_ವೃಕ್ಷ("trace", 0, dump_ವೃಕ್ಷ_Either_Level() >= 3);
}
