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

#ਅਨ੍ਤਰ੍ਭਾਵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Trace.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Ast.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3DupFinder.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3EmitCBase.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Graph.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Stats.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3UniqueNames.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <limits>
#ਅਨ੍ਤਰ੍ਭਾਵ <map>
#ਅਨ੍ਤਰ੍ਭਾਵ <set>
#ਅਨ੍ਤਰ੍ਭਾਵ <unordered_map>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Graph vertexes

ਵਰਗ Trace_Activity_ਸ਼ੀਰਸ਼ final : ਜਨਤਕ V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼ {
    VL_RTTI_IMPL(Trace_Activity_ਸ਼ੀਰਸ਼, V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼)
    Ast_ਗੰਢ* ਅਚਲ m_insertp;
    int32_t m_activity_ਕੋਡ;
    ਤਾਰਕਿਕ m_slow;  // If always slow, we can use the same code
ਜਨਤਕ:
    ਗਿਣਤੀ { ACTIVITY_NEVER = ((1UL << 31) - 1) };
    ਗਿਣਤੀ { ACTIVITY_ALWAYS = ((1UL << 31) - 2) };
    ਗਿਣਤੀ { ACTIVITY_SLOW = 0 };
    Trace_Activity_ਸ਼ੀਰਸ਼(V3_ਗ੍ਰਾਫ* graphp, Ast_ਗੰਢ* nodep, ਤਾਰਕਿਕ slow)
        : V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼{graphp}
        , m_insertp{nodep} {
        m_activity_ਕੋਡ = 0;
        m_slow = slow;
    }
    Trace_Activity_ਸ਼ੀਰਸ਼(V3_ਗ੍ਰਾਫ* graphp, int32_t ਕੋਡ)
        : V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼{graphp}
        , m_insertp{ਖਾਲੀ_ਸੂਚਕ} {
        m_activity_ਕੋਡ = ਕੋਡ;
        m_slow = ਝੂਠ;
    }
    ~Trace_Activity_ਸ਼ੀਰਸ਼() override = ਮੂਲ;
    // ACCESSORS
    Ast_ਗੰਢ* insertp() ਅਚਲ {
        UASSERT(m_insertp, "Null insertp; probably called on a special always/slow");
        ਵਾਪਸ m_insertp;
    }
    string name() ਅਚਲ override {
        ਜੇ (activityAlways()) {
            ਵਾਪਸ "*ALWAYS*";
        } ਨਹੀਂਤਾਂ {
            ਵਾਪਸ std::string{slow() ? "*SLOW* " : ""} + insertp()->name();
        }
    }
    string dotColor() ਅਚਲ override { ਵਾਪਸ slow() ? "yellowGreen" : "green"; }
    int32_t activity_ਕੋਡ() ਅਚਲ { ਵਾਪਸ m_activity_ਕੋਡ; }
    ਤਾਰਕਿਕ activityAlways() ਅਚਲ { ਵਾਪਸ activity_ਕੋਡ() == ACTIVITY_ALWAYS; }
    ਤਾਰਕਿਕ activitySlow() ਅਚਲ { ਵਾਪਸ activity_ਕੋਡ() == ACTIVITY_SLOW; }
    ਖਾਲੀ activity_ਕੋਡ(int32_t ਕੋਡ) { m_activity_ਕੋਡ = ਕੋਡ; }
    ਤਾਰਕਿਕ slow() ਅਚਲ { ਵਾਪਸ m_slow; }
    ਖਾਲੀ slow(ਤਾਰਕਿਕ flag) {
        ਜੇ (!flag) m_slow = ਝੂਠ;
    }
};

ਵਰਗ Trace_CFunc_ਸ਼ੀਰਸ਼ final : ਜਨਤਕ V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼ {
    VL_RTTI_IMPL(Trace_CFunc_ਸ਼ੀਰਸ਼, V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼)
    AstCFunc* ਅਚਲ m_nodep;

ਜਨਤਕ:
    Trace_CFunc_ਸ਼ੀਰਸ਼(V3_ਗ੍ਰਾਫ* graphp, AstCFunc* nodep)
        : V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼{graphp}
        , m_nodep{nodep} {}
    ~Trace_CFunc_ਸ਼ੀਰਸ਼() override = ਮੂਲ;
    // ACCESSORS
    AstCFunc* nodep() ਅਚਲ { ਵਾਪਸ m_nodep; }
    string name() ਅਚਲ override { ਵਾਪਸ nodep()->name(); }
    string dotColor() ਅਚਲ override { ਵਾਪਸ "yellow"; }
    File_ਸਤਰ* fileline() ਅਚਲ override { ਵਾਪਸ nodep()->fileline(); }
};

ਵਰਗ Trace_Trace_ਸ਼ੀਰਸ਼ final : ਜਨਤਕ V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼ {
    VL_RTTI_IMPL(Trace_Trace_ਸ਼ੀਰਸ਼, V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼)
    AstTraceDecl* ਅਚਲ m_nodep;  // TRACEINC this represents
    // nullptr, or other vertex with the real code() that duplicates this one
    Trace_Trace_ਸ਼ੀਰਸ਼* m_duplicatep = ਖਾਲੀ_ਸੂਚਕ;
    // When aliasing to a dtype parent, offset of the member within the dtype
    uint32_t m_dtype_Alias_ਵਿੱਥ = 0;

ਜਨਤਕ:
    Trace_Trace_ਸ਼ੀਰਸ਼(V3_ਗ੍ਰਾਫ* graphp, AstTraceDecl* nodep)
        : V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼{graphp}
        , m_nodep{nodep} {}
    ~Trace_Trace_ਸ਼ੀਰਸ਼() override = ਮੂਲ;
    // ACCESSORS
    AstTraceDecl* nodep() ਅਚਲ { ਵਾਪਸ m_nodep; }
    string name() ਅਚਲ override { ਵਾਪਸ nodep()->name(); }
    string dotColor() ਅਚਲ override { ਵਾਪਸ "red"; }
    File_ਸਤਰ* fileline() ਅਚਲ override { ਵਾਪਸ nodep()->fileline(); }
    Trace_Trace_ਸ਼ੀਰਸ਼* duplicatep() ਅਚਲ { ਵਾਪਸ m_duplicatep; }
    ਖਾਲੀ duplicatep(Trace_Trace_ਸ਼ੀਰਸ਼* dupp) {
        UASSERT_OBJ(!duplicatep(), nodep(), "Assigning duplicatep() to already duplicated node");
        m_duplicatep = dupp;
    }
    ਖਾਲੀ redirectDuplicatep(Trace_Trace_ਸ਼ੀਰਸ਼* dupp) { m_duplicatep = dupp; }
    uint32_t dtype_Alias_ਵਿੱਥ() ਅਚਲ { ਵਾਪਸ m_dtype_Alias_ਵਿੱਥ; }
    ਖਾਲੀ dtype_Alias_ਵਿੱਥ(uint32_t ਵਿੱਥ) { m_dtype_Alias_ਵਿੱਥ = ਵਿੱਥ; }
};

ਵਰਗ Trace_Var_ਸ਼ੀਰਸ਼ final : ਜਨਤਕ V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼ {
    VL_RTTI_IMPL(Trace_Var_ਸ਼ੀਰਸ਼, V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼)
    AstVarScope* ਅਚਲ m_nodep;

ਜਨਤਕ:
    Trace_Var_ਸ਼ੀਰਸ਼(V3_ਗ੍ਰਾਫ* graphp, AstVarScope* nodep)
        : V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼{graphp}
        , m_nodep{nodep} {}
    ~Trace_Var_ਸ਼ੀਰਸ਼() override = ਮੂਲ;
    // ACCESSORS
    AstVarScope* nodep() ਅਚਲ { ਵਾਪਸ m_nodep; }
    string name() ਅਚਲ override { ਵਾਪਸ nodep()->name(); }
    string dotColor() ਅਚਲ override { ਵਾਪਸ "skyblue"; }
    File_ਸਤਰ* fileline() ਅਚਲ override { ਵਾਪਸ nodep()->fileline(); }
};

//######################################################################
// Trace state, as a visitor of each AstNode

ਵਰਗ TraceVisitor final : ਜਨਤਕ VNVisitor {
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
    ਅਚਲ VNUser1_ਵਿੱਚ_Use m_inuser1;
    ਅਚਲ VNUser2_ਵਿੱਚ_Use m_inuser2;
    ਅਚਲ VNUser3_ਵਿੱਚ_Use m_inuser3;
    // VNUser4InUse     In V3Hasher via V3DupFinder

    // STATE
    Ast_ਗੰਢ_Module* m_ਸਿਖਰ_Modp = ਖਾਲੀ_ਸੂਚਕ;  // Module to add variables to
    AstScope* ਅਚਲ m_ਸਿਖਰ_Scopep = v3Global.rootp()->ਸਿਖਰ_Scopep()->scopep();  // The top AstScope
    AstCFunc* m_cfuncp = ਖਾਲੀ_ਸੂਚਕ;  // C function adding to graph
    AstCFunc* m_regFuncp = ਖਾਲੀ_ਸੂਚਕ;  // Trace registration function
    AstCFunc* m_actAllFuncp = ਖਾਲੀ_ਸੂਚਕ;  // Set all activity function
    AstTraceDecl* m_tracep = ਖਾਲੀ_ਸੂਚਕ;  // Trace function adding to graph
    AstVarScope* m_activityVscp = ਖਾਲੀ_ਸੂਚਕ;  // Activity variable
    uint32_t m_activity_ਸੰਖਿਆ = 0;  // Count of fields in activity variable
    uint32_t m_ਕੋਡ = 0;  // Trace ident code# being assigned
    V3_ਗ੍ਰਾਫ m_ਗ੍ਰਾਫ;  // Var/CFunc tracking
    Trace_Activity_ਸ਼ੀਰਸ਼* ਅਚਲ m_alwaysVtxp;  // "Always trace" vertex
    ਤਾਰਕਿਕ m_finding = ਝੂਠ;  // Pass one of algorithm?
    ਬਣਤਰ DtypeFuncs final {
    ਜਨਤਕ:
        AstCFunc* ਭਰਿਆ_Funcp = ਖਾਲੀ_ਸੂਚਕ;
        AstCFunc* chgFuncp = ਖਾਲੀ_ਸੂਚਕ;
    };
    std::unordered_map<ਅਚਲ Ast_ਗੰਢ_DType*, DtypeFuncs>
        m_dtypeNonConstFuncs;  // Full / Chg funcs per type
    std::unordered_map<ਅਚਲ Ast_ਗੰਢ_DType*, AstCFunc*> m_dtypeConstFuncs;  // Const func per type
    V3_Unique_ਨਾਂ m_dtype_ਨਾਂ{""};  // Unique type func names

    // Trace parallelism. Only VCD tracing can be parallelized at this time.
    ਅਚਲ uint32_t m_parallelism
        = v3Global.opt.useTraceParallel() ? static_cast<uint32_t>(v3Global.opt.ਧਾਗੇ()) : 1;

    VDouble0 m_statSetters;  // Statistic tracking
    VDouble0 m_statSettersSlow;  // Statistic tracking
    VDouble0 m_statUniqSigs;  // Statistic tracking

    // All activity numbers applying to a given trace
    ਵਰਤੋਂ Act_ਕੋਡ_ਸੈੱਟ = std::ਸੈੱਟ<uint32_t>;
    // For activity set, what traces apply
    ਵਰਤੋਂ TraceVec = std::multimap<Act_ਕੋਡ_ਸੈੱਟ, Trace_Trace_ਸ਼ੀਰਸ਼*>;
    // Candidate interface-member VarScopes keyed by (interface type, member name)
    std::map<std::pair<ਅਚਲ AstIface*, std::string>, std::ਸਦਿਸ਼<AstVarScope*>>
        m_ifaceMemberVscps;

    ਵਰਗ TraceInitDeclCollector final : ਜਨਤਕ VNVisitor {
        std::ਸਦਿਸ਼<AstTraceDecl*>& m_declps;  // Output: trace declarations found so far
        std::ਸੈੱਟ<ਅਚਲ AstCFunc*> m_ਵੇਖਿਆ_Funcps;

        ਖਾਲੀ ਦੇਖੋ(AstTraceDecl* nodep) override { m_declps.ਧੱਕੋ_ਪਿੱਠ(nodep); }
        ਖਾਲੀ ਦੇਖੋ(AstCCall* nodep) override {
            ਜੇ (AstCFunc* ਅਚਲ funcp = nodep->funcp()) collect(funcp);
        }
        ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override { iterateChildren(nodep); }

    ਜਨਤਕ:
        ਸਪਸ਼ਟ TraceInitDeclCollector(std::ਸਦਿਸ਼<AstTraceDecl*>& declps)
            : m_declps{declps} {}
        ਖਾਲੀ collect(AstCFunc* funcp) {
            ਜੇ (funcp && m_ਵੇਖਿਆ_Funcps.ਪਾਓ(funcp).ਦੂਜਾ) iterate(funcp);
        }
    };

    // METHODS

    ਸਥਿਰ ਤਾਰਕਿਕ same_ਜੜ੍ਹ_Init_Alias(ਅਚਲ AstTraceDecl* ਜੜ੍ਹ_Declp, ਅਚਲ AstTraceDecl* ਸਿਖਰ_Declp) {
        ਅਚਲ VNumRange& ਜੜ੍ਹ_ਦੋਆਧਾਰੀ_ਅੰਕ_Range = ਜੜ੍ਹ_Declp->ਦੋਆਧਾਰੀ_ਅੰਕ_Range();
        ਅਚਲ VNumRange& ਸਿਖਰ_ਦੋਆਧਾਰੀ_ਅੰਕ_Range = ਸਿਖਰ_Declp->ਦੋਆਧਾਰੀ_ਅੰਕ_Range();
        ਵਾਪਸ ਜੜ੍ਹ_Declp->showname() == ਸਿਖਰ_Declp->showname()
               && ਜੜ੍ਹ_Declp->declDirection() == ਸਿਖਰ_Declp->declDirection()
               && ਜੜ੍ਹ_Declp->ਚੌੜਾਈ_Min() == ਸਿਖਰ_Declp->ਚੌੜਾਈ_Min()
               && ਜੜ੍ਹ_ਦੋਆਧਾਰੀ_ਅੰਕ_Range.ranged() == ਸਿਖਰ_ਦੋਆਧਾਰੀ_ਅੰਕ_Range.ranged()
               && (!ਜੜ੍ਹ_ਦੋਆਧਾਰੀ_ਅੰਕ_Range.ranged()
                   || (ਜੜ੍ਹ_ਦੋਆਧਾਰੀ_ਅੰਕ_Range.ਖੱਬੇ() == ਸਿਖਰ_ਦੋਆਧਾਰੀ_ਅੰਕ_Range.ਖੱਬੇ()
                       && ਜੜ੍ਹ_ਦੋਆਧਾਰੀ_ਅੰਕ_Range.ਸੱਜੇ() == ਸਿਖਰ_ਦੋਆਧਾਰੀ_ਅੰਕ_Range.ਸੱਜੇ()));
    }

    ਖਾਲੀ detectDuplicates() {
        UINFO(9, "Finding duplicates");
        // Note uses user4
        V3DupFinder dupFinder;  // Duplicate code detection

        // Compute member offsets within dtype instances incrementally.
        // For each dtype member TraceDecl, stores its code offset within the
        // dtype parent's code range.
        std::unordered_map<ਅਚਲ AstTraceDecl*, uint32_t> memberOffsets;
        std::unordered_map<ਅਚਲ AstTraceDecl*, uint32_t> running_ਵਿੱਥ;

        // For fixup pass: track dups redirected to dtype parents, so we can
        // fix them if the parent's user2 is later set during the loop.
        // Maps dup vertex -> original canonical member vertex.
        std::ਸਦਿਸ਼<std::pair<Trace_Trace_ਸ਼ੀਰਸ਼*, Trace_Trace_ਸ਼ੀਰਸ਼*>> dtype_ਮਾਪਾ_Redirects;

        // Hash all of the traced values and find if there are any duplicates
        ਲਈ (V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼& vtx : m_ਗ੍ਰਾਫ.vertices()) {
            ਜੇ (Trace_Trace_ਸ਼ੀਰਸ਼* ਅਚਲ vvertexp = vtx.cast<Trace_Trace_ਸ਼ੀਰਸ਼>()) {
                AstTraceDecl* ਅਚਲ nodep = vvertexp->nodep();
                ਜੇ (nodep->dtypeCallp()) ਜਾਰੀ;

                ਜੇ (nodep->dtypeDeclp()) {
                    uint32_t& ਵਿੱਥ = running_ਵਿੱਥ[nodep->dtypeDeclp()];
                    memberOffsets[nodep] = ਵਿੱਥ;
                    ਵਿੱਥ += nodep->ਕੋਡ_Inc();
                }

                UASSERT_OBJ(!vvertexp->duplicatep(), nodep, "Should not be a duplicate");
                ਅਚਲ ਸਵੈਚਾਲਿਤ dupit = dupFinder.ਲੱਭੋ_Duplicate(nodep);
                ਜੇ (dupit == dupFinder.ਸਿਰਾ()) {
                    dupFinder.ਪਾਓ(nodep);
                } ਨਹੀਂਤਾਂ {
                    AstTraceDecl* ਅਚਲ dupDeclp = VN_AS(dupit->ਦੂਜਾ, TraceDecl);
                    UASSERT_OBJ(dupDeclp, nodep, "Trace duplicate of wrong type");
                    Trace_Trace_ਸ਼ੀਰਸ਼* ਅਚਲ dupvertexp
                        = dupDeclp->user1u().ਮੰਜ਼ਿਲ_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼()->cast<Trace_Trace_ਸ਼ੀਰਸ਼>();
                    UINFO(8, "  Orig " << dupDeclp << ਸਤਰ_ਅੰਤ);
                    UINFO(8, "   dup " << nodep << ਸਤਰ_ਅੰਤ);

                    ਜੇ (dupDeclp->dtypeDeclp() && !dupDeclp->dtypeDeclp()->user2()) {
                        AstTraceDecl* ਅਚਲ dtypeParentp = dupDeclp->dtypeDeclp();
                        Trace_Trace_ਸ਼ੀਰਸ਼* ਅਚਲ dtypeVtxp
                            = dtypeParentp->user1u().ਮੰਜ਼ਿਲ_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼()->cast<Trace_Trace_ਸ਼ੀਰਸ਼>();
                        ਅਚਲ ਸਵੈਚਾਲਿਤ it2 = memberOffsets.ਲੱਭੋ(dupDeclp);
                        UASSERT_OBJ(it2 != memberOffsets.ਸਿਰਾ(), dupDeclp,
                                    "Member offset not precomputed");
                        vvertexp->duplicatep(dtypeVtxp);
                        vvertexp->dtype_Alias_ਵਿੱਥ(it2->ਦੂਜਾ);
                        dtype_ਮਾਪਾ_Redirects.emplace_ਪਿੱਠ(vvertexp, dupvertexp);
                    } ਨਹੀਂਤਾਂ {
                        vvertexp->duplicatep(dupvertexp);
                    }
                    ਜੇ (nodep->dtypeDeclp()) nodep->dtypeDeclp()->user2(ਸੱਚ);
                }
            }
        }

        ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& pair : dtype_ਮਾਪਾ_Redirects) {
            Trace_Trace_ਸ਼ੀਰਸ਼* ਅਚਲ dupVtxp = pair.ਪਹਿਲਾ;
            Trace_Trace_ਸ਼ੀਰਸ਼* ਅਚਲ memberVtxp = pair.ਦੂਜਾ;
            ਅਚਲ AstTraceDecl* ਅਚਲ ਮਾਪਾ_Declp = dupVtxp->duplicatep()->nodep();
            ਜੇ (ਮਾਪਾ_Declp->user2()) {
                dupVtxp->redirectDuplicatep(memberVtxp);
                dupVtxp->dtype_Alias_ਵਿੱਥ(0);
            }
        }

        ਜੇ (dumpLevel() || debug() >= 9)
            dupFinder.dumpFile(v3Global.debugFilename("trace") + ".hash", ਝੂਠ);
    }

    ਖਾਲੀ ਗ੍ਰਾਫ_Dtype_Prune() {
        ਲਈ (V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* ਅਚਲ vtxp : m_ਗ੍ਰਾਫ.vertices().unlinkable()) {
            ਜੇ (Trace_Trace_ਸ਼ੀਰਸ਼* ਅਚਲ vvertexp = vtxp->cast<Trace_Trace_ਸ਼ੀਰਸ਼>()) {
                AstTraceDecl* ਅਚਲ declp = vvertexp->nodep();
                // This skips the dtype sub-func optimization if a var is affected by multiple
                // activities.  We really only need to do this for _chg funcs (and not decls,
                // _const and _full) but it's simpiler to do it all one way or the other.
                ਜੇ (declp) {
                    ਜੇ (declp->user2() || (declp->dtypeDeclp() && !declp->dtypeDeclp()->user2())) {
                        AstCCall* ਅਚਲ callp = declp->dtypeCallp();
                        ਜੇ (callp) {
                            Ast_ਗੰਢ* stmtexprp = callp->backp();
                            VL_DO_DANGLING(ਧੱਕੋ_Deletep(stmtexprp->unlink_Fr_ਪਿੱਠ()), stmtexprp);
                        } ਨਹੀਂਤਾਂ {
                            ਤਾਰਕਿਕ ਖਾਲੀ_ਥਾਵਾਂ_Scope;
                            ਕਰੋ {
                                ਖਾਲੀ_ਥਾਵਾਂ_Scope = ਝੂਠ;
                                Ast_ਗੰਢ* ਅਚਲ declBackp = declp->backp();
                                Ast_ਗੰਢ* ਅਚਲ declNextp = declp->nextp();
                                ਜੇ (VN_IS(declBackp, Trace_ਧੱਕੋ_ਅਗੇਤਰ)
                                    && VN_IS(declNextp, Trace_ਕੱਢੋ_ਅਗੇਤਰ)) {
                                    VL_DO_DANGLING(ਧੱਕੋ_Deletep(declBackp->unlink_Fr_ਪਿੱਠ()),
                                                   declBackp);
                                    VL_DO_DANGLING(ਧੱਕੋ_Deletep(declNextp->unlink_Fr_ਪਿੱਠ()),
                                                   declNextp);
                                    ਖਾਲੀ_ਥਾਵਾਂ_Scope = ਸੱਚ;
                                }
                            } ਜਦੋਂ (ਖਾਲੀ_ਥਾਵਾਂ_Scope);
                        }
                        // Can't purge until we finish this pass
                        ਧੱਕੋ_Deletep(declp->unlink_Fr_ਪਿੱਠ());  // declp used below
                        vvertexp->reroute_ਕਿਨਾਰੇ(&m_ਗ੍ਰਾਫ);
                        vvertexp->unlinkDelete(&m_ਗ੍ਰਾਫ);
                    }
                    declp->dtypeDeclp(ਖਾਲੀ_ਸੂਚਕ);
                }
            }
        }
    }

    ਖਾਲੀ ਵੰਡੋ_Trace_Decl_Funcs() {
        ਅਚਲ ਪੂਰਨ ਵੰਡੋ_ਹੱਦਬੰਦੀ = v3Global.opt.output_ਵੰਡੋ_CTrace() ? v3Global.opt.output_ਵੰਡੋ_CTrace()
                                                                : std::numeric_limits<ਪੂਰਨ>::max();
        ਜੇ (ਵੰਡੋ_ਹੱਦਬੰਦੀ == std::numeric_limits<ਪੂਰਨ>::max()) ਵਾਪਸ;

        ਲਈ (Ast_ਗੰਢ* nodep = m_ਸਿਖਰ_Scopep->blocksp(); nodep; nodep = nodep->nextp()) {
            AstCFunc* ਅਚਲ funcp = VN_CAST(nodep, CFunc);
            ਜੇ (!funcp || !funcp->user2()) ਜਾਰੀ;

            ਅਚਲ string ਅਧਾਰ_Name = funcp->name();
            uint32_t sub_ਅੰਕ = 1;
            ਪੂਰਨ cur_ਮਾਪ = 0;
            AstCFunc* curFuncp = funcp;
            std::ਸਦਿਸ਼<Ast_ਗੰਢ_Stmt*> callStmts;

            ਸਵੈਚਾਲਿਤ ਸ਼ੁਰੂ_New_Sub_Func = [&]() {
                File_ਸਤਰ* ਅਚਲ flp = funcp->fileline();
                ਅਚਲ string newName = ਅਧਾਰ_Name + "_" + cvt_ਮੰਜ਼ਿਲ_Str(sub_ਅੰਕ++);
                AstCFunc* ਅਚਲ newFuncp = ਨਵਾਂ AstCFunc{flp, newName, m_ਸਿਖਰ_Scopep};
                newFuncp->argTypes(v3Global.opt.trace_Class_ਅਧਾਰ() + "* tracep");
                newFuncp->isTrace(ਸੱਚ);
                newFuncp->isStatic(ਝੂਠ);
                newFuncp->isLoose(ਸੱਚ);
                newFuncp->slow(ਸੱਚ);
                newFuncp->dontCombine(ਸੱਚ);
                m_ਸਿਖਰ_Scopep->ਜੋੜੋ_Blocksp(newFuncp);
                newFuncp->ਜੋੜੋ_Stmtsp(ਨਵਾਂ AstCStmt{flp, "const int c = vlSymsp->__Vm_baseCode;"});
                AstCCall* ਅਚਲ callp = ਨਵਾਂ AstCCall{flp, newFuncp};
                callp->dtype_ਸੈੱਟ_Void();
                callp->argTypes("tracep");
                callStmts.ਧੱਕੋ_ਪਿੱਠ(callp->makeStmt());
                curFuncp = newFuncp;
                cur_ਮਾਪ = 0;
            };

            ਲਈ (Ast_ਗੰਢ *stmtp = funcp->stmtsp(), *nextp; stmtp; stmtp = nextp) {
                nextp = stmtp->nextp();
                cur_ਮਾਪ += stmtp->ਗੰਢ_ਗਿਣਤੀ();

                ਜੇ (curFuncp != funcp) {
                    stmtp->unlink_Fr_ਪਿੱਠ();
                    curFuncp->ਜੋੜੋ_Stmtsp(stmtp);
                }

                ਜੇ (cur_ਮਾਪ > ਵੰਡੋ_ਹੱਦਬੰਦੀ && nextp) ਸ਼ੁਰੂ_New_Sub_Func();
            }

            ਲਈ (Ast_ਗੰਢ_Stmt* ਅਚਲ callStmtp : callStmts) { funcp->ਜੋੜੋ_Stmtsp(callStmtp); }
        }
    }

    ਖਾਲੀ ਗ੍ਰਾਫ_Simplify(ਤਾਰਕਿਕ initial) {
        ਜੇ (initial) {
            // Remove all variable nodes
            ਲਈ (V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* ਅਚਲ vtxp : m_ਗ੍ਰਾਫ.vertices().unlinkable()) {
                ਜੇ (Trace_Var_ਸ਼ੀਰਸ਼* ਅਚਲ vvertexp = vtxp->cast<Trace_Var_ਸ਼ੀਰਸ਼>()) {
                    vvertexp->reroute_ਕਿਨਾਰੇ(&m_ਗ੍ਰਾਫ);
                    vvertexp->unlinkDelete(&m_ਗ੍ਰਾਫ);
                }
            }
            // Remove multiple variables connecting funcs to traces
            // We do this twice, as then we have fewer edges to multiply out in the below
            // expansion.
            m_ਗ੍ਰਾਫ.remove_Redundant_ਕਿਨਾਰੇ_Max(&V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ::followAlwaysTrue);
            // Remove all Cfunc nodes
            ਲਈ (V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* ਅਚਲ vtxp : m_ਗ੍ਰਾਫ.vertices().unlinkable()) {
                ਜੇ (Trace_CFunc_ਸ਼ੀਰਸ਼* ਅਚਲ vvertexp = vtxp->cast<Trace_CFunc_ਸ਼ੀਰਸ਼>()) {
                    vvertexp->reroute_ਕਿਨਾਰੇ(&m_ਗ੍ਰਾਫ);
                    vvertexp->unlinkDelete(&m_ਗ੍ਰਾਫ);
                }
            }
        }

        // Remove multiple variables connecting funcs to traces
        m_ਗ੍ਰਾਫ.remove_Redundant_ਕਿਨਾਰੇ_Max(&V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ::followAlwaysTrue);

        // If there are any edges from a always, keep only the always
        ਲਈ (V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼& vtx : m_ਗ੍ਰਾਫ.vertices()) {
            ਜੇ (Trace_Trace_ਸ਼ੀਰਸ਼* ਅਚਲ vvertexp = vtx.cast<Trace_Trace_ਸ਼ੀਰਸ਼>()) {
                // Search for the incoming always edge
                ਅਚਲ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ* alwaysEdgep = ਖਾਲੀ_ਸੂਚਕ;
                ਲਈ (ਅਚਲ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ& ਕਿਨਾਰਾ : vvertexp->ਵਿੱਚ_ਕਿਨਾਰੇ()) {
                    ਅਚਲ Trace_Activity_ਸ਼ੀਰਸ਼* ਅਚਲ actVtxp
                        = ਕਿਨਾਰਾ.fromp()->as<ਅਚਲ Trace_Activity_ਸ਼ੀਰਸ਼>();
                    ਜੇ (actVtxp->activityAlways()) {
                        alwaysEdgep = &ਕਿਨਾਰਾ;
                        ਰੋਕੋ;
                    }
                }
                // If always edge exists, remove all other edges
                ਜੇ (alwaysEdgep) {
                    ਲਈ (V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ* ਅਚਲ edgep : vvertexp->ਵਿੱਚ_ਕਿਨਾਰੇ().unlinkable()) {
                        ਜੇ (edgep != alwaysEdgep) VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
                    }
                }
            }
        }

        // Activity points with no outputs can be removed
        ਲਈ (V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* ਅਚਲ vtxp : m_ਗ੍ਰਾਫ.vertices().unlinkable()) {
            ਜੇ (Trace_Activity_ਸ਼ੀਰਸ਼* ਅਚਲ aVtxp = vtxp->cast<Trace_Activity_ਸ਼ੀਰਸ਼>()) {
                // Leave in the always vertex for later use.
                ਜੇ (aVtxp != m_alwaysVtxp && aVtxp->ਬਾਹਰਲਾ_ਖਾਲੀ_ਥਾਵਾਂ()) {
                    VL_DO_DANGLING(aVtxp->unlinkDelete(&m_ਗ੍ਰਾਫ), aVtxp);
                }
            }
        }
    }

    uint32_t assignactivityNumbers() {
        uint32_t activity_ਸੰਖਿਆ = 1;  // Note 0 indicates "slow" only
        ਲਈ (V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼& vtx : m_ਗ੍ਰਾਫ.vertices()) {
            ਜੇ (Trace_Activity_ਸ਼ੀਰਸ਼* ਅਚਲ vvertexp = vtx.cast<Trace_Activity_ਸ਼ੀਰਸ਼>()) {
                ਜੇ (vvertexp != m_alwaysVtxp) {
                    ਜੇ (vvertexp->slow()) {
                        vvertexp->activity_ਕੋਡ(Trace_Activity_ਸ਼ੀਰਸ਼::ACTIVITY_SLOW);
                    } ਨਹੀਂਤਾਂ {
                        vvertexp->activity_ਕੋਡ(activity_ਸੰਖਿਆ++);
                    }
                }
            }
        }
        ਵਾਪਸ activity_ਸੰਖਿਆ;
    }

    ਖਾਲੀ ਛਾਂਟੋ_Traces(TraceVec& traces, uint32_t& nNonConstCodes) {
        // Populate sort structure
        traces.clear();
        nNonConstCodes = 0;
        ਲਈ (V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼& vtx : m_ਗ੍ਰਾਫ.vertices()) {
            ਜੇ (Trace_Trace_ਸ਼ੀਰਸ਼* ਅਚਲ vtxp = vtx.cast<Trace_Trace_ਸ਼ੀਰਸ਼>()) {
                Act_ਕੋਡ_ਸੈੱਟ act_ਸੈੱਟ;
                UINFO(9, "  Add to sort: " << vtxp);
                UINFOTREE(9, vtxp->nodep(), "", "trnode");
                ਲਈ (ਅਚਲ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ& ਕਿਨਾਰਾ : vtxp->ਵਿੱਚ_ਕਿਨਾਰੇ()) {
                    ਅਚਲ Trace_Activity_ਸ਼ੀਰਸ਼* ਅਚਲ cfvertexp
                        = ਕਿਨਾਰਾ.fromp()->cast<ਅਚਲ Trace_Activity_ਸ਼ੀਰਸ਼>();
                    UASSERT_OBJ(cfvertexp, vtxp->nodep(),
                                "Should have been function pointing to this trace");
                    UINFO(9, "   Activity: " << cfvertexp);
                    ਜੇ (cfvertexp->activityAlways()) {
                        // If code 0, we always trace; ignore other codes
                        act_ਸੈੱਟ.ਪਾਓ(Trace_Activity_ਸ਼ੀਰਸ਼::ACTIVITY_ALWAYS);
                    } ਨਹੀਂਤਾਂ {
                        act_ਸੈੱਟ.ਪਾਓ(cfvertexp->activity_ਕੋਡ());
                    }
                }
                UASSERT_OBJ(act_ਸੈੱਟ.ਗਿਣਤੀ_2(Trace_Activity_ਸ਼ੀਰਸ਼::ACTIVITY_ALWAYS) == 0
                                || act_ਸੈੱਟ.ਮਾਪ() == 1,
                            vtxp->nodep(), "Always active trace has further triggers");
                // Count nodes
                ਜੇ (!vtxp->duplicatep() && !act_ਸੈੱਟ.ਖਾਲੀ_ਥਾਵਾਂ())
                    nNonConstCodes += vtxp->nodep()->ਕੋਡ_Inc();
                ਜੇ (act_ਸੈੱਟ.ਖਾਲੀ_ਥਾਵਾਂ()) {
                    // If a trace doesn't have activity, it's constant, and we
                    // don't need to track changes on it.
                    act_ਸੈੱਟ.ਪਾਓ(Trace_Activity_ਸ਼ੀਰਸ਼::ACTIVITY_NEVER);
                } ਨਹੀਂਤਾਂ ਜੇ (act_ਸੈੱਟ.ਗਿਣਤੀ_2(Trace_Activity_ਸ਼ੀਰਸ਼::ACTIVITY_SLOW) && act_ਸੈੱਟ.ਮਾਪ() > 1) {
                    // If a trace depends on the slow flag as well as other
                    // flags, remove the dependency on the slow flag. We will
                    // make slow routines set all activity flags.
                    act_ਸੈੱਟ.erase(Trace_Activity_ਸ਼ੀਰਸ਼::ACTIVITY_SLOW);
                }
                traces.emplace(act_ਸੈੱਟ, vtxp);
            }
        }
    }

    ਖਾਲੀ ਗ੍ਰਾਫ_Optimize() {
        // Assign initial activity numbers to activity vertices
        assignactivityNumbers();

        // Sort the traces by activity sets
        TraceVec traces;
        uint32_t unused1;
        ਛਾਂਟੋ_Traces(traces, unused1);

        // For each activity set with only a small number of signals, make those
        // signals always traced, as it's cheaper to check a few value changes
        // than to test a lot of activity flags
        ਸਵੈਚਾਲਿਤ it = traces.begin();
        ਅਚਲ ਸਵੈਚਾਲਿਤ ਸਿਰਾ = traces.ਸਿਰਾ();
        ਜਦੋਂ (it != ਸਿਰਾ) {
            ਸਵੈਚਾਲਿਤ ਸਿਰ = it;
            // Approximate the complexity of the value change check
            uint32_t complexity = 0;
            ਅਚਲ Act_ਕੋਡ_ਸੈੱਟ& act_ਸੈੱਟ = it->ਪਹਿਲਾ;
            ਲਈ (; it != ਸਿਰਾ && it->ਪਹਿਲਾ == act_ਸੈੱਟ; ++it) {
                ਜੇ (!it->ਦੂਜਾ->duplicatep()) {
                    uint32_t ਲਾਗਤ = 0;
                    ਅਚਲ AstTraceDecl* ਅਚਲ declp = it->ਦੂਜਾ->nodep();
                    // The number of comparisons required by bufp->chg*
                    ਲਾਗਤ += declp->isWide() ? declp->ਕੋਡ_Inc() : 1;
                    // Arrays are traced by element
                    ਲਾਗਤ *= declp->arrayRange().ranged() ? declp->arrayRange().elements() : 1;
                    // Note: Experiments factoring in the size of declp->valuep()
                    // showed no benefit in tracing speed, even for large trees,
                    // so we will leave those out for now.
                    complexity += ਲਾਗਤ;
                }
            }
            // Leave alone always changing, never changing and signals only set in slow code
            ਜੇ (act_ਸੈੱਟ.ਗਿਣਤੀ_2(Trace_Activity_ਸ਼ੀਰਸ਼::ACTIVITY_ALWAYS)) ਜਾਰੀ;
            ਜੇ (act_ਸੈੱਟ.ਗਿਣਤੀ_2(Trace_Activity_ਸ਼ੀਰਸ਼::ACTIVITY_NEVER)) ਜਾਰੀ;
            ਜੇ (act_ਸੈੱਟ.ਗਿਣਤੀ_2(Trace_Activity_ਸ਼ੀਰਸ਼::ACTIVITY_SLOW)) ਜਾਰੀ;
            // If the value comparisons are cheaper to perform than checking the
            // activity flags make the signals always traced. Note this cost
            // equation is heuristic.
            ਜੇ (complexity <= act_ਸੈੱਟ.ਮਾਪ() * 2) {
                ਲਈ (; ਸਿਰ != it; ++ਸਿਰ) {
                    ਨਵਾਂ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ{&m_ਗ੍ਰਾਫ, m_alwaysVtxp, ਸਿਰ->ਦੂਜਾ, 1};
                }
            }
        }

        ਗ੍ਰਾਫ_Simplify(ਝੂਠ);
    }

    Ast_ਗੰਢ_ਵਿਅੰਜਕ* selectActivity(File_ਸਤਰ* flp, uint32_t acode, ਅਚਲ VAccess& access) {
        ਵਾਪਸ ਨਵਾਂ AstArraySel(flp, ਨਵਾਂ AstVarRef{flp, m_activityVscp, access}, acode);
    }

    Ast_ਗੰਢ* newActivitySetter(Ast_ਗੰਢ* insertp, uint32_t ਕੋਡ) {
        ++m_statSetters;
        File_ਸਤਰ* ਅਚਲ fl = insertp->fileline();
        Ast_ਸੌਂਪੋ* ਅਚਲ setterp = ਨਵਾਂ Ast_ਸੌਂਪੋ{fl, selectActivity(fl, ਕੋਡ, VAccess::WRITE),
                                                 ਨਵਾਂ AstConst{fl, AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_True{}}};
        ਵਾਪਸ setterp;
    }

    Ast_ਗੰਢ* newActivityAll(Ast_ਗੰਢ* insertp) {
        ++m_statSettersSlow;
        ਜੇ (!m_actAllFuncp) {
            File_ਸਤਰ* ਅਚਲ flp = m_ਸਿਖਰ_Scopep->fileline();
            AstCFunc* ਅਚਲ funcp = ਨਵਾਂ AstCFunc{flp, "__VtraceActivitySetAll", m_ਸਿਖਰ_Scopep};
            funcp->slow(ਸੱਚ);
            funcp->isStatic(ਝੂਠ);
            funcp->isLoose(ਸੱਚ);
            m_ਸਿਖਰ_Scopep->ਜੋੜੋ_Blocksp(funcp);
            ਲਈ (uint32_t ਕੋਡ = 0; ਕੋਡ < m_activity_ਸੰਖਿਆ; ++ਕੋਡ) {
                Ast_ਗੰਢ* ਅਚਲ setterp = newActivitySetter(insertp, ਕੋਡ);
                funcp->ਜੋੜੋ_Stmtsp(setterp);
            }
            m_actAllFuncp = funcp;
        }
        AstCCall* ਅਚਲ callp = ਨਵਾਂ AstCCall{insertp->fileline(), m_actAllFuncp};
        callp->dtype_ਸੈੱਟ_Void();
        ਵਾਪਸ callp->makeStmt();
    }

    ਖਾਲੀ createActivityFlags() {
        // Assign final activity numbers
        m_activity_ਸੰਖਿਆ = assignactivityNumbers();

        // Create an array of bytes, not a bit vector, as they can be set
        // atomically by mtasks, and are cheaper to set (no need for
        // read-modify-write on the C type), and the speed of the tracing code
        // is the same on largish designs.
        File_ਸਤਰ* ਅਚਲ flp = m_ਸਿਖਰ_Scopep->fileline();
        Ast_ਗੰਢ_DType* ਅਚਲ newScalarDtp = ਨਵਾਂ AstBasicDType{flp, VFlag_ਦੋਆਧਾਰੀ_ਅੰਕ_Packed{}, 1};
        v3Global.rootp()->typeTablep()->ਜੋੜੋ_Typesp(newScalarDtp);
        AstRange* ਅਚਲ newArange
            = ਨਵਾਂ AstRange{flp, VNumRange{static_cast<ਪੂਰਨ>(m_activity_ਸੰਖਿਆ) - 1, 0}};
        Ast_ਗੰਢ_DType* ਅਚਲ newArrDtp = ਨਵਾਂ AstUnpackArrayDType{flp, newScalarDtp, newArange};
        v3Global.rootp()->typeTablep()->ਜੋੜੋ_Typesp(newArrDtp);
        m_activityVscp = v3Global.rootp()->ਸਿਖਰ_Scopep()->create_ਆਰਜ਼ੀ("__VtraceActivity", newArrDtp);

        // Insert activity setters
        ਲਈ (ਅਚਲ V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼& vtx : m_ਗ੍ਰਾਫ.vertices()) {
            ਜੇ (ਅਚਲ Trace_Activity_ਸ਼ੀਰਸ਼* ਅਚਲ vtxp = vtx.cast<ਅਚਲ Trace_Activity_ਸ਼ੀਰਸ਼>()) {
                Ast_ਗੰਢ* setterp = ਖਾਲੀ_ਸੂਚਕ;
                ਜੇ (vtxp->activitySlow()) {
                    setterp = newActivityAll(vtxp->insertp());
                    // Just set all flags in slow code as it should be rare.
                    // This will be rolled up into a loop by V3Reloop.
                } ਨਹੀਂਤਾਂ ਜੇ (!vtxp->activityAlways()) {
                    setterp = newActivitySetter(vtxp->insertp(), vtxp->activity_ਕੋਡ());
                }
                ਜੇ (setterp) {
                    Ast_ਗੰਢ* ਅਚਲ insertp = vtxp->insertp();
                    ਜੇ (Ast_Stmt_ਵਿਅੰਜਕ* ਅਚਲ stmtp = VN_CAST(insertp, Stmt_ਵਿਅੰਜਕ)) {
                        stmtp->ਜੋੜੋ_ਅਗਲਾ_ਇੱਥੇ(setterp);
                    } ਨਹੀਂਤਾਂ ਜੇ (AstCFunc* ਅਚਲ funcp = VN_CAST(insertp, CFunc)) {
                        // If there are awaits, insert the setter after each await
                        ਜੇ (funcp->isCoroutine() && funcp->stmtsp()) {
                            funcp->stmtsp()->foreach_And_ਅਗਲਾ([setterp](AstCAwait* awaitp) {
                                awaitp->ਜੋੜੋ_ਅਗਲਾ_ਇੱਥੇ(setterp->clone_ਰੁੱਖ(ਝੂਠ));
                            });
                        }
                        funcp->ਜੋੜੋ_Stmtsp(setterp);
                    } ਨਹੀਂਤਾਂ {
                        insertp->v3fatalSrc("Bad trace activity vertex");
                    }
                }
            }
        }
    }

    AstCFunc* newCFunc(VTraceType traceType, AstCFunc* ਸਿਖਰ_Funcp, uint32_t func_ਅੰਕ,
                       uint32_t ਅਧਾਰ_ਕੋਡ = 0, ਅਚਲ AstTraceDecl* ਅਚਲ declp = ਖਾਲੀ_ਸੂਚਕ,
                       ਤਾਰਕਿਕ declSub = ਝੂਠ) {
        // Create new function
        ਅਚਲ ਤਾਰਕਿਕ is_ਸਿਖਰ_Func = !declp && ਸਿਖਰ_Funcp == ਖਾਲੀ_ਸੂਚਕ;
        std::string funcName;
        ਜੇ (is_ਸਿਖਰ_Func || declp) {
            funcName = traceType.func_ਅਗੇਤਰ();
        } ਨਹੀਂਤਾਂ {
            funcName = ਸਿਖਰ_Funcp->name();
            funcName += "_sub";
        }
        ਜੇ (declp) {
            funcName += "_dtype";
            ਜੇ (declSub) funcName += "_sub";
            funcName += "__";
            funcName = m_dtype_ਨਾਂ.get(funcName);
        } ਨਹੀਂਤਾਂ {
            funcName += "_";
            funcName += cvt_ਮੰਜ਼ਿਲ_Str(func_ਅੰਕ);
        }

        File_ਸਤਰ* ਅਚਲ flp = m_ਸਿਖਰ_Scopep->fileline();
        AstCFunc* ਅਚਲ funcp = ਨਵਾਂ AstCFunc{flp, funcName, m_ਸਿਖਰ_Scopep};
        funcp->isTrace(ਸੱਚ);
        funcp->dontCombine(ਸੱਚ);
        funcp->isLoose(ਸੱਚ);
        funcp->slow(traceType != VTraceType::CHANGE);
        funcp->isStatic(is_ਸਿਖਰ_Func);
        // Add it to top scope
        m_ਸਿਖਰ_Scopep->ਜੋੜੋ_Blocksp(funcp);
        ਅਚਲ std::string bufArg
            = v3Global.opt.trace_Class_ਅਧਾਰ() + "::Buffer* bufp"
              + (declp ? (", uint32_t offset, const " + declp->dtypep()->cType("", ਸੱਚ, ਸੱਚ)
                          + " __VdtypeVar")
                       : "");
        ਜੇ (is_ਸਿਖਰ_Func) {
            // Top functions
            funcp->argTypes("void* voidSelf, " + bufArg);
            funcp->ਜੋੜੋ_Stmtsp(ਨਵਾਂ AstCStmt{flp, EmitCUtil::void_Self_ਸੌਂਪੋ(m_ਸਿਖਰ_Modp)});
            funcp->ਜੋੜੋ_Stmtsp(ਨਵਾਂ AstCStmt{flp, EmitCUtil::sym_Class_ਸੌਂਪੋ()});
            // Add global activity check to change dump functions
            ਜੇ (traceType == VTraceType::CHANGE) {  //
                funcp->ਜੋੜੋ_Stmtsp(
                    ਨਵਾਂ AstCStmt{flp, "if (VL_UNLIKELY(!vlSymsp->__Vm_activity)) return;"});
            }
            // Register function
            AstCStmt* ਅਚਲ cstmtp = ਨਵਾਂ AstCStmt{flp};
            m_regFuncp->ਜੋੜੋ_Stmtsp(cstmtp);
            ਜੇ (traceType == VTraceType::CONSTANT) {
                cstmtp->ਜੋੜੋ("tracep->addConstCb(");
                cstmtp->ਜੋੜੋ(ਨਵਾਂ Ast_Addr_ਦਾ_CFunc{flp, funcp});
                cstmtp->ਜੋੜੋ(", " + std::ਮੰਜ਼ਿਲ_string(func_ਅੰਕ) + ", vlSelf);");
            } ਨਹੀਂਤਾਂ ਜੇ (traceType == VTraceType::ਭਰਿਆ) {
                cstmtp->ਜੋੜੋ("tracep->addFullCb(");
                cstmtp->ਜੋੜੋ(ਨਵਾਂ Ast_Addr_ਦਾ_CFunc{flp, funcp});
                cstmtp->ਜੋੜੋ(", " + std::ਮੰਜ਼ਿਲ_string(func_ਅੰਕ) + ", vlSelf);");
            } ਨਹੀਂਤਾਂ {
                cstmtp->ਜੋੜੋ("tracep->addChgCb(");
                cstmtp->ਜੋੜੋ(ਨਵਾਂ Ast_Addr_ਦਾ_CFunc{flp, funcp});
                cstmtp->ਜੋੜੋ(", " + std::ਮੰਜ਼ਿਲ_string(func_ਅੰਕ) + ", vlSelf);");
            }
        } ਨਹੀਂਤਾਂ {
            // Sub functions
            funcp->argTypes(bufArg);
            // Setup base references. Note in rare occasions we can end up with an empty trace
            // sub function, hence the VL_ATTR_UNUSED attributes.
            ਜੇ (traceType != VTraceType::CHANGE) {
                // Full dump sub function
                funcp->ਜੋੜੋ_Stmtsp(ਨਵਾਂ AstCStmt{flp,  //
                                              string("uint32_t* const oldp VL_ATTR_UNUSED = "
                                                     "bufp->oldp(vlSymsp->__Vm_baseCode")
                                                  + (declp ? " + offset" : "") + ");\n"});
            } ਨਹੀਂਤਾਂ {
                // Change dump sub function
                funcp->ਜੋੜੋ_Stmtsp(ਨਵਾਂ AstCStmt{flp,  //
                                              "uint32_t* const oldp VL_ATTR_UNUSED = "
                                              "bufp->oldp(vlSymsp->__Vm_baseCode + "
                                                  + (declp ? " offset" : cvt_ਮੰਜ਼ਿਲ_Str(ਅਧਾਰ_ਕੋਡ))
                                                  + ");\n"});
            }
            ਜੇ (!declp) {
                // Add call to top function
                AstCCall* ਅਚਲ callp = ਨਵਾਂ AstCCall{funcp->fileline(), funcp};
                callp->dtype_ਸੈੱਟ_Void();
                callp->argTypes("bufp");
                ਸਿਖਰ_Funcp->ਜੋੜੋ_Stmtsp(callp->makeStmt());
            }
        }
        //ਪੂਰਾ
        UINFO(5, "  newCFunc " << funcp);
        ਵਾਪਸ funcp;
    }

    AstCFunc* newCDtypeSubFunc(VTraceType traceType, ਅਚਲ AstTraceDecl* ਅਚਲ declp,
                               AstCFunc* parentp) {
        AstCFunc* ਅਚਲ funcp = newCFunc(traceType, ਖਾਲੀ_ਸੂਚਕ, 0, 0, declp, ਸੱਚ);
        File_ਸਤਰ* ਅਚਲ flp = m_ਸਿਖਰ_Scopep->fileline();
        AstCCall* ਅਚਲ callp = ਨਵਾਂ AstCCall{flp, funcp};
        callp->dtype_ਸੈੱਟ_Void();
        callp->argTypes("bufp, offset, __VdtypeVar");
        parentp->ਜੋੜੋ_Stmtsp(callp->makeStmt());
        ਵਾਪਸ funcp;
    }

    AstCFunc* createConstDtypeTraceFunctions(ਅਚਲ AstTraceDecl* declp) {
        ਅਚਲ Ast_ਗੰਢ_DType* ਅਚਲ dtypep = declp->dtypep()->skipRefp();
        ਸਵੈਚਾਲਿਤ pair = m_dtypeConstFuncs.emplace(dtypep, ਖਾਲੀ_ਸੂਚਕ);
        ਜੇ (pair.ਦੂਜਾ) {
            File_ਸਤਰ* ਅਚਲ flp = declp->fileline();
            AstCFunc* ਅਚਲ funcp = newCFunc(VTraceType::CONSTANT, ਖਾਲੀ_ਸੂਚਕ, 0, 0, declp);

            ਤਾਰਕਿਕ ਪਹਿਲਾ = ਸੱਚ;
            ਲਈ (Ast_ਗੰਢ* callStmtp = declp->dtypeCallp()->funcp()->stmtsp(); callStmtp;
                 callStmtp = callStmtp->nextp()) {
                ਅਚਲ AstCCall* ਅਚਲ callp
                    = VN_CAST(VN_CAST(callStmtp, Stmt_ਵਿਅੰਜਕ)->exprp(), CCall);
                ਤਾਰਕਿਕ onlyOne = ਪਹਿਲਾ && !callStmtp->nextp();
                AstCFunc* ਅਚਲ subFuncp
                    = onlyOne ? funcp : newCDtypeSubFunc(VTraceType::CONSTANT, declp, funcp);
                ਲਈ (Ast_ਗੰਢ* stmtp = callp->funcp()->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    ਜੇ (AstTraceDecl* ਅਚਲ ਖੇਤਰ_Declp = VN_CAST(stmtp, TraceDecl)) {
                        AstTraceInc* ਅਚਲ incp
                            = ਨਵਾਂ AstTraceInc{flp, ਖੇਤਰ_Declp, VTraceType::CONSTANT};
                        subFuncp->ਜੋੜੋ_Stmtsp(incp);
                    }
                }
                ਪਹਿਲਾ = ਝੂਠ;
            }

            pair.ਪਹਿਲਾ->ਦੂਜਾ = funcp;
        }

        ਵਾਪਸ pair.ਪਹਿਲਾ->ਦੂਜਾ;
    }

    ਖਾਲੀ createConstTraceFunctions(ਅਚਲ TraceVec& traces) {
        ਅਚਲ ਪੂਰਨ ਵੰਡੋ_ਹੱਦਬੰਦੀ = v3Global.opt.output_ਵੰਡੋ_CTrace() ? v3Global.opt.output_ਵੰਡੋ_CTrace()
                                                                : std::numeric_limits<ਪੂਰਨ>::max();

        AstCFunc* ਅਚਲ ਸਿਖਰ_Funcp = newCFunc(VTraceType::CONSTANT, ਖਾਲੀ_ਸੂਚਕ, 0);
        uint32_t sub_Func_ਅੰਕ = 0;
        AstCFunc* subFuncp = ਖਾਲੀ_ਸੂਚਕ;
        ਪੂਰਨ subStmts = 0;
        std::ਸਦਿਸ਼<ਅਚਲ Trace_Trace_ਸ਼ੀਰਸ਼*> duplicates;
        ਲਈ (ਸਵੈਚਾਲਿਤ it = traces.cbegin(); it != traces.ਸਿਰਾ(); ++it) {
            ਅਚਲ Trace_Trace_ਸ਼ੀਰਸ਼* ਅਚਲ vtxp = it->ਦੂਜਾ;
            AstTraceDecl* ਅਚਲ declp = vtxp->nodep();
            ਜੇ (vtxp->duplicatep()) {
                duplicates.ਧੱਕੋ_ਪਿੱਠ(vtxp);
                ਜਾਰੀ;
            }

            // This is a canonical trace node. Assign trace code (signal number).
            UASSERT_OBJ(!declp->ਕੋਡ_Assigned(), declp,
                        "Canonical node should not have code assigned yet");
            declp->ਕੋਡ(m_ਕੋਡ);
            ਅਚਲ uint32_t ਕੋਡ_Inc = declp->ਕੋਡ_Inc();
            m_ਕੋਡ += ਕੋਡ_Inc;
            ++m_statUniqSigs;

            // If this is a const signal, add the AstTraceInc
            ਅਚਲ Act_ਕੋਡ_ਸੈੱਟ& act_ਸੈੱਟ = it->ਪਹਿਲਾ;
            ਜੇ (act_ਸੈੱਟ.ਗਿਣਤੀ_2(Trace_Activity_ਸ਼ੀਰਸ਼::ACTIVITY_NEVER)) {
                // Crate new sub function if required
                ਜੇ (!subFuncp || subStmts > ਵੰਡੋ_ਹੱਦਬੰਦੀ) {
                    subStmts = 0;
                    subFuncp = newCFunc(VTraceType::CONSTANT, ਸਿਖਰ_Funcp, sub_Func_ਅੰਕ);
                    ++sub_Func_ਅੰਕ;
                }
                File_ਸਤਰ* ਅਚਲ flp = declp->fileline();
                ਜੇ (declp->dtypeCallp()) {
                    AstCFunc* ਅਚਲ funcp = createConstDtypeTraceFunctions(declp);
                    Ast_ਗੰਢ_ਵਿਅੰਜਕ* argsp = ਖਾਲੀ_ਸੂਚਕ;
                    argsp = Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ(argsp, declp->valuep()->clone_ਰੁੱਖ(ਝੂਠ));
                    AstCCall* ਅਚਲ callp = ਨਵਾਂ AstCCall{flp, funcp, argsp};
                    callp->dtype_ਸੈੱਟ_Void();
                    callp->argTypes(callp->argTypes() + "bufp, " + std::ਮੰਜ਼ਿਲ_string(declp->ਕੋਡ()));
                    subFuncp->ਜੋੜੋ_Stmtsp(callp->makeStmt());

                    subStmts += 1;
                } ਨਹੀਂਤਾਂ {
                    AstTraceInc* ਅਚਲ incp = ਨਵਾਂ AstTraceInc{flp, declp, VTraceType::CONSTANT};
                    subFuncp->ਜੋੜੋ_Stmtsp(incp);
                    subStmts += incp->ਗੰਢ_ਗਿਣਤੀ();
                }
            }
        }

        // Assign codes to duplicate nodes from their canonicals
        ਲਈ (ਅਚਲ Trace_Trace_ਸ਼ੀਰਸ਼* ਅਚਲ vtxp : duplicates) {
            AstTraceDecl* ਅਚਲ declp = vtxp->nodep();
            ਅਚਲ Trace_Trace_ਸ਼ੀਰਸ਼* ਅਚਲ canonVtxp = vtxp->duplicatep();
            ਅਚਲ AstTraceDecl* ਅਚਲ canonDeclp = canonVtxp->nodep();
            UASSERT_OBJ(!canonVtxp->duplicatep(), canonDeclp, "Canonical node is a duplicate");
            UASSERT_OBJ(canonDeclp->ਕੋਡ_Assigned(), canonDeclp,
                        "Canonical node should have code assigned already");
            declp->ਕੋਡ(canonDeclp->ਕੋਡ() + vtxp->dtype_Alias_ਵਿੱਥ());
        }
    }

    DtypeFuncs createNonConstDtypeTraceFunctions(ਅਚਲ AstTraceDecl* declp) {
        Ast_ਗੰਢ_DType* dtypep = declp->dtypep()->skipRefp();
        ਸਵੈਚਾਲਿਤ pair = m_dtypeNonConstFuncs.emplace(dtypep, DtypeFuncs{});
        ਜੇ (pair.ਦੂਜਾ) {
            File_ਸਤਰ* ਅਚਲ flp = declp->fileline();
            AstCFunc* ਅਚਲ ਭਰਿਆ_Funcp = newCFunc(VTraceType::ਭਰਿਆ, ਖਾਲੀ_ਸੂਚਕ, 0, 0, declp);
            AstCFunc* ਅਚਲ chgFuncp = newCFunc(VTraceType::CHANGE, ਖਾਲੀ_ਸੂਚਕ, 0, 0, declp);

            ਤਾਰਕਿਕ ਪਹਿਲਾ = ਸੱਚ;
            ਲਈ (Ast_ਗੰਢ* callStmtp = declp->dtypeCallp()->funcp()->stmtsp(); callStmtp;
                 callStmtp = callStmtp->nextp()) {
                ਅਚਲ AstCCall* ਅਚਲ callp
                    = VN_CAST(VN_CAST(callStmtp, Stmt_ਵਿਅੰਜਕ)->exprp(), CCall);
                ਤਾਰਕਿਕ onlyOne = ਪਹਿਲਾ && !callStmtp->nextp();
                AstCFunc* ਅਚਲ ਭਰਿਆ_Sub_Funcp
                    = onlyOne ? ਭਰਿਆ_Funcp : newCDtypeSubFunc(VTraceType::ਭਰਿਆ, declp, ਭਰਿਆ_Funcp);
                AstCFunc* ਅਚਲ chgSubFuncp
                    = onlyOne ? chgFuncp : newCDtypeSubFunc(VTraceType::CHANGE, declp, chgFuncp);
                ਲਈ (Ast_ਗੰਢ* stmtp = callp->funcp()->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    ਜੇ (AstTraceDecl* ਅਚਲ ਖੇਤਰ_Declp = VN_CAST(stmtp, TraceDecl)) {
                        AstTraceInc* ਅਚਲ incFullp
                            = ਨਵਾਂ AstTraceInc{flp, ਖੇਤਰ_Declp, VTraceType::ਭਰਿਆ};
                        ਭਰਿਆ_Sub_Funcp->ਜੋੜੋ_Stmtsp(incFullp);
                        AstTraceInc* ਅਚਲ incChgp
                            = ਨਵਾਂ AstTraceInc{flp, ਖੇਤਰ_Declp, VTraceType::CHANGE};
                        chgSubFuncp->ਜੋੜੋ_Stmtsp(incChgp);
                    }
                }
                ਪਹਿਲਾ = ਝੂਠ;
            }

            pair.ਪਹਿਲਾ->ਦੂਜਾ = {.ਭਰਿਆ_Funcp = ਭਰਿਆ_Funcp, .chgFuncp = chgFuncp};
        }

        ਵਾਪਸ pair.ਪਹਿਲਾ->ਦੂਜਾ;
    }

    ਖਾਲੀ createNonConstTraceFunctions(ਅਚਲ TraceVec& traces, uint32_t nAllCodes,
                                      uint32_t parallelism) {
        ਅਚਲ ਪੂਰਨ ਵੰਡੋ_ਹੱਦਬੰਦੀ = v3Global.opt.output_ਵੰਡੋ_CTrace() ? v3Global.opt.output_ਵੰਡੋ_CTrace()
                                                                : std::numeric_limits<ਪੂਰਨ>::max();

        // pre-incremented, so starts at 0
        uint32_t ਸਿਖਰ_Func_ਅੰਕ = std::numeric_limits<uint32_t>::max();
        TraceVec::const_iterator it = traces.begin();
        ਜਦੋਂ (it != traces.ਸਿਰਾ()) {
            AstCFunc* ਸਿਖਰ_Ful_Funcp = ਖਾਲੀ_ਸੂਚਕ;
            AstCFunc* ਸਿਖਰ_Chg_Funcp = ਖਾਲੀ_ਸੂਚਕ;
            AstCFunc* subFulFuncp = ਖਾਲੀ_ਸੂਚਕ;
            AstCFunc* subChgFuncp = ਖਾਲੀ_ਸੂਚਕ;
            uint32_t sub_Func_ਅੰਕ = 0;
            ਪੂਰਨ subStmts = 0;
            ਅਚਲ uint32_t maxCodes = std::max((nAllCodes + parallelism - 1) / parallelism, 1U);
            uint32_t nCodes = 0;
            ਅਚਲ Act_ਕੋਡ_ਸੈੱਟ* prev_Act_ਸੈੱਟ = ਖਾਲੀ_ਸੂਚਕ;
            AstIf* ifp = ਖਾਲੀ_ਸੂਚਕ;
            uint32_t ਅਧਾਰ_ਕੋਡ = 0;
            ਲਈ (; nCodes < maxCodes && it != traces.ਸਿਰਾ(); ++it) {
                ਅਚਲ Act_ਕੋਡ_ਸੈੱਟ& act_ਸੈੱਟ = it->ਪਹਿਲਾ;
                // Traced value never changes, no need to add it
                ਜੇ (act_ਸੈੱਟ.ਗਿਣਤੀ_2(Trace_Activity_ਸ਼ੀਰਸ਼::ACTIVITY_NEVER)) ਜਾਰੀ;

                ਅਚਲ Trace_Trace_ਸ਼ੀਰਸ਼* ਅਚਲ vtxp = it->ਦੂਜਾ;
                AstTraceDecl* ਅਚਲ declp = vtxp->nodep();

                // This is a duplicate decl, no need to add it, but must set the
                // function index to the same as the canonical node.
                ਜੇ (ਅਚਲ Trace_Trace_ਸ਼ੀਰਸ਼* ਅਚਲ canonVtxp = vtxp->duplicatep()) {
                    declp->fidx(canonVtxp->nodep()->fidx());
                    ਜਾਰੀ;
                }

                // Create top function if not yet created
                ਜੇ (!ਸਿਖਰ_Ful_Funcp) {
                    ++ਸਿਖਰ_Func_ਅੰਕ;
                    ਸਿਖਰ_Ful_Funcp = newCFunc(VTraceType::ਭਰਿਆ, ਖਾਲੀ_ਸੂਚਕ, ਸਿਖਰ_Func_ਅੰਕ);
                    ਸਿਖਰ_Chg_Funcp = newCFunc(VTraceType::CHANGE, ਖਾਲੀ_ਸੂਚਕ, ਸਿਖਰ_Func_ਅੰਕ);
                }

                // Create new sub function if required
                ਜੇ (!subFulFuncp || subStmts > ਵੰਡੋ_ਹੱਦਬੰਦੀ) {
                    ਅਧਾਰ_ਕੋਡ = declp->ਕੋਡ();
                    subStmts = 0;
                    subFulFuncp = newCFunc(VTraceType::ਭਰਿਆ, ਸਿਖਰ_Ful_Funcp, sub_Func_ਅੰਕ, ਅਧਾਰ_ਕੋਡ);
                    subChgFuncp = newCFunc(VTraceType::CHANGE, ਸਿਖਰ_Chg_Funcp, sub_Func_ਅੰਕ, ਅਧਾਰ_ਕੋਡ);
                    ++sub_Func_ਅੰਕ;
                    prev_Act_ਸੈੱਟ = ਖਾਲੀ_ਸੂਚਕ;
                    ifp = ਖਾਲੀ_ਸੂਚਕ;
                }

                // If required, create the conditional node checking the activity flags
                ਜੇ (!prev_Act_ਸੈੱਟ || act_ਸੈੱਟ != *prev_Act_ਸੈੱਟ) {
                    File_ਸਤਰ* ਅਚਲ flp = m_ਸਿਖਰ_Scopep->fileline();
                    ਅਚਲ ਤਾਰਕਿਕ always = act_ਸੈੱਟ.ਗਿਣਤੀ_2(Trace_Activity_ਸ਼ੀਰਸ਼::ACTIVITY_ALWAYS) != 0;
                    Ast_ਗੰਢ_ਵਿਅੰਜਕ* condp = ਖਾਲੀ_ਸੂਚਕ;
                    ਜੇ (always) {
                        condp = ਨਵਾਂ AstConst{flp, 1};  // Always true, will be folded later
                    } ਨਹੀਂਤਾਂ {
                        ਲਈ (ਅਚਲ uint32_t act_ਕੋਡ : act_ਸੈੱਟ) {
                            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ selp = selectActivity(flp, act_ਕੋਡ, VAccess::READ);
                            condp = condp ? ਨਵਾਂ AstOr{flp, condp, selp} : selp;
                        }
                    }
                    ifp = ਨਵਾਂ AstIf{flp, condp};
                    ਜੇ (!always) ifp->branchPred(VBranchPred::BP_UNLIKELY);
                    subChgFuncp->ਜੋੜੋ_Stmtsp(ifp);
                    subStmts += ifp->ਗੰਢ_ਗਿਣਤੀ();
                    prev_Act_ਸੈੱਟ = &act_ਸੈੱਟ;
                }

                // Add TraceInc nodes
                File_ਸਤਰ* ਅਚਲ flp = declp->fileline();
                ਜੇ (declp->dtypeCallp()) {
                    DtypeFuncs funcs = createNonConstDtypeTraceFunctions(declp);
                    Ast_ਗੰਢ_ਵਿਅੰਜਕ* argsp = ਖਾਲੀ_ਸੂਚਕ;
                    argsp = Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ(argsp, declp->valuep()->clone_ਰੁੱਖ(ਝੂਠ));
                    AstCCall* ਅਚਲ callFullp = ਨਵਾਂ AstCCall{flp, funcs.ਭਰਿਆ_Funcp, argsp};
                    callFullp->dtype_ਸੈੱਟ_Void();
                    callFullp->argTypes(callFullp->argTypes() + "bufp, "
                                        + std::ਮੰਜ਼ਿਲ_string(declp->ਕੋਡ()));
                    subFulFuncp->ਜੋੜੋ_Stmtsp(callFullp->makeStmt());
                    argsp = ਖਾਲੀ_ਸੂਚਕ;
                    argsp = Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ(argsp, declp->valuep()->clone_ਰੁੱਖ(ਝੂਠ));
                    AstCCall* ਅਚਲ callChgp = ਨਵਾਂ AstCCall{flp, funcs.chgFuncp, argsp};
                    callChgp->dtype_ਸੈੱਟ_Void();
                    callChgp->argTypes(callChgp->argTypes() + "bufp, "
                                       + std::ਮੰਜ਼ਿਲ_string(declp->ਕੋਡ()));
                    ifp->ਜੋੜੋ_Thensp(callChgp->makeStmt());

                    subStmts += 2;
                } ਨਹੀਂਤਾਂ {
                    AstTraceInc* ਅਚਲ incFulp = ਨਵਾਂ AstTraceInc{flp, declp, VTraceType::ਭਰਿਆ};
                    subFulFuncp->ਜੋੜੋ_Stmtsp(incFulp);
                    AstTraceInc* ਅਚਲ incChgp
                        = ਨਵਾਂ AstTraceInc{flp, declp, VTraceType::CHANGE, ਅਧਾਰ_ਕੋਡ};
                    ifp->ਜੋੜੋ_Thensp(incChgp);

                    // Set the function index of the decl
                    declp->fidx(ਸਿਖਰ_Func_ਅੰਕ);

                    // Track splitting due to size
                    UASSERT_OBJ(incFulp->ਗੰਢ_ਗਿਣਤੀ() == incChgp->ਗੰਢ_ਗਿਣਤੀ(), declp,
                                "Should have equal cost");
                    ਅਚਲ VNumRange range = declp->arrayRange();
                    ਜੇ (range.ranged()) {
                        // 2x because each element is a TraceInc and a VarRef
                        subStmts += range.elements() * 2;
                    } ਨਹੀਂਤਾਂ {
                        subStmts += incChgp->ਗੰਢ_ਗਿਣਤੀ();
                    }
                }

                // Track partitioning
                nCodes += declp->ਕੋਡ_Inc();
            }
        }
    }

    ਖਾਲੀ createCleanupFunction() {
        File_ਸਤਰ* ਅਚਲ fl = m_ਸਿਖਰ_Scopep->fileline();
        AstCFunc* ਅਚਲ cleanupFuncp = ਨਵਾਂ AstCFunc{fl, "trace_cleanup", m_ਸਿਖਰ_Scopep};
        cleanupFuncp->argTypes("void* voidSelf, " + v3Global.opt.trace_Class_ਅਧਾਰ()
                               + "* /*unused*/");
        cleanupFuncp->isTrace(ਸੱਚ);
        cleanupFuncp->slow(ਝੂਠ);
        cleanupFuncp->isStatic(ਸੱਚ);
        cleanupFuncp->isLoose(ਸੱਚ);
        m_ਸਿਖਰ_Scopep->ਜੋੜੋ_Blocksp(cleanupFuncp);
        cleanupFuncp->ਜੋੜੋ_Stmtsp(ਨਵਾਂ AstCStmt{fl, EmitCUtil::void_Self_ਸੌਂਪੋ(m_ਸਿਖਰ_Modp)});
        cleanupFuncp->ਜੋੜੋ_Stmtsp(ਨਵਾਂ AstCStmt{fl, EmitCUtil::sym_Class_ਸੌਂਪੋ()});

        // Register it
        {
            AstCStmt* ਅਚਲ cstmtp = ਨਵਾਂ AstCStmt{fl};
            m_regFuncp->ਜੋੜੋ_Stmtsp(cstmtp);
            cstmtp->ਜੋੜੋ("tracep->addCleanupCb(");
            cstmtp->ਜੋੜੋ(ਨਵਾਂ Ast_Addr_ਦਾ_CFunc{fl, cleanupFuncp});
            cstmtp->ਜੋੜੋ(", vlSelf);");
        }

        // Clear global activity flag
        cleanupFuncp->ਜੋੜੋ_Stmtsp(
            ਨਵਾਂ AstCStmt{m_ਸਿਖਰ_Scopep->fileline(), "vlSymsp->__Vm_activity = false;"s});

        // Clear fine grained activity flags
        ਲਈ (uint32_t i = 0; i < m_activity_ਸੰਖਿਆ; ++i) {
            Ast_ਗੰਢ* ਅਚਲ clrp = ਨਵਾਂ Ast_ਸੌਂਪੋ{fl, selectActivity(fl, i, VAccess::WRITE),
                                                ਨਵਾਂ AstConst{fl, AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_False{}}};
            cleanupFuncp->ਜੋੜੋ_Stmtsp(clrp);
        }
    }

    ਖਾਲੀ createTraceFunctions() {
        // Detect and remove duplicate values
        detectDuplicates();

        ਗ੍ਰਾਫ_Dtype_Prune();
        ਵੰਡੋ_Trace_Decl_Funcs();
        m_ਗ੍ਰਾਫ.remove_Redundant_ਕਿਨਾਰੇ_Max(&V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ::followAlwaysTrue);

        // Simplify & optimize the graph
        ਜੇ (dump_ਗ੍ਰਾਫ_Level() >= 6) m_ਗ੍ਰਾਫ.dumpDotFilePrefixed("trace_pre");
        ਗ੍ਰਾਫ_Simplify(ਸੱਚ);
        ਜੇ (dump_ਗ੍ਰਾਫ_Level() >= 6) m_ਗ੍ਰਾਫ.dumpDotFilePrefixed("trace_simplified");
        ਗ੍ਰਾਫ_Optimize();
        ਜੇ (dump_ਗ੍ਰਾਫ_Level() >= 6) m_ਗ੍ਰਾਫ.dumpDotFilePrefixed("trace_optimized");

        // Create the fine grained activity flags
        createActivityFlags();

        // Form a sorted list of the traces we are interested in
        TraceVec traces;  // The sorted traces
        // We will split functions such that each have to dump roughly the same amount of data
        // for this we need to keep tack of the number of codes used by the trace functions.
        uint32_t nNonConstCodes = 0;
        ਛਾਂਟੋ_Traces(traces, nNonConstCodes);

        // Our keys are now sorted to have same activity number adjacent, then
        // by trace order. (Better would be execution order for cache
        // efficiency....) Last are constants and non-changers, as then the
        // last value vector is more compact

        // Create the trace registration function
        m_regFuncp = ਨਵਾਂ AstCFunc{m_ਸਿਖਰ_Scopep->fileline(), "trace_register", m_ਸਿਖਰ_Scopep};
        m_regFuncp->argTypes(v3Global.opt.trace_Class_ਅਧਾਰ() + "* tracep");
        m_regFuncp->ਇੰਦਰਾਜ_ਬਿੰਦੂ(ਸੱਚ);
        m_regFuncp->isTrace(ਸੱਚ);
        m_regFuncp->slow(ਸੱਚ);
        m_regFuncp->isStatic(ਝੂਠ);
        m_regFuncp->isLoose(ਸੱਚ);
        m_ਸਿਖਰ_Scopep->ਜੋੜੋ_Blocksp(m_regFuncp);

        // Create the const dump functions. Also allocates trace codes.
        createConstTraceFunctions(traces);

        // Create the full and incremental dump functions
        createNonConstTraceFunctions(traces, nNonConstCodes, m_parallelism);

        // Root-traced libraries alias wrapper IOs onto the existing top-module codes.
        ਜੇ (!v3Global.opt.libCreate().ਖਾਲੀ_ਥਾਵਾਂ()) {
            std::ਸਦਿਸ਼<AstTraceDecl*> ਜੜ੍ਹ_Declps;
            std::ਸਦਿਸ਼<AstTraceDecl*> ਸਿਖਰ_Declps;
            TraceInitDeclCollector ਜੜ੍ਹ_Collector{ਜੜ੍ਹ_Declps};
            TraceInitDeclCollector ਸਿਖਰ_Collector{ਸਿਖਰ_Declps};
            ਲਈ (Ast_ਗੰਢ* blockp = m_ਸਿਖਰ_Scopep->blocksp(); blockp; blockp = blockp->nextp()) {
                AstCFunc* ਅਚਲ funcp = VN_CAST(blockp, CFunc);
                ਜੇ (funcp && VString::startsWith(funcp->name(), "trace_init_leaf_root__")) {
                    ਜੜ੍ਹ_Collector.collect(funcp);
                } ਨਹੀਂਤਾਂ ਜੇ (funcp && VString::startsWith(funcp->name(), "trace_init_leaf_top__")) {
                    ਸਿਖਰ_Collector.collect(funcp);
                }
            }

            std::ਸਦਿਸ਼<ਤਾਰਕਿਕ> ਵਰਤਿਆ(ਸਿਖਰ_Declps.ਮਾਪ(), ਝੂਠ);
            ਲਈ (AstTraceDecl* ਅਚਲ ਜੜ੍ਹ_Declp : ਜੜ੍ਹ_Declps) {
                ਲਈ (ਮਾਪ_t i = 0; i < ਸਿਖਰ_Declps.ਮਾਪ(); ++i) {
                    ਜੇ (ਵਰਤਿਆ[i] || !same_ਜੜ੍ਹ_Init_Alias(ਜੜ੍ਹ_Declp, ਸਿਖਰ_Declps[i])) ਜਾਰੀ;
                    ਜੜ੍ਹ_Declp->ਕੋਡ(ਸਿਖਰ_Declps[i]->ਕੋਡ());
                    ਵਰਤਿਆ[i] = ਸੱਚ;
                    ਰੋਕੋ;
                }
            }
        }

        // Remove refs to traced values from TraceDecl nodes, these have now moved under
        // TraceInc
        ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& i : traces) {
            Ast_ਗੰਢ* ਅਚਲ valuep = i.ਦੂਜਾ->nodep()->valuep();
            VL_DO_DANGLING(valuep->unlink_Fr_ਪਿੱਠ()->delete_ਰੁੱਖ(), valuep);
        }

        // Create the trace cleanup function clearing the activity flags
        createCleanupFunction();
    }

    Trace_CFunc_ਸ਼ੀਰਸ਼* getCFuncVertexp(AstCFunc* nodep) {
        V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* ਅਚਲ vtxp = nodep->user1u().ਮੰਜ਼ਿਲ_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼();
        Trace_CFunc_ਸ਼ੀਰਸ਼* vertexp = vtxp ? vtxp->cast<Trace_CFunc_ਸ਼ੀਰਸ਼>() : ਖਾਲੀ_ਸੂਚਕ;
        ਜੇ (!vertexp) {
            vertexp = ਨਵਾਂ Trace_CFunc_ਸ਼ੀਰਸ਼{&m_ਗ੍ਰਾਫ, nodep};
            nodep->user1p(vertexp);
        }
        ਵਾਪਸ vertexp;
    }
    Trace_Activity_ਸ਼ੀਰਸ਼* getActivityVertexp(Ast_ਗੰਢ* nodep, ਤਾਰਕਿਕ slow) {
        V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* ਅਚਲ vtxp = nodep->user3u().ਮੰਜ਼ਿਲ_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼();
        Trace_Activity_ਸ਼ੀਰਸ਼* vertexp = vtxp ? vtxp->cast<Trace_Activity_ਸ਼ੀਰਸ਼>() : ਖਾਲੀ_ਸੂਚਕ;
        ਜੇ (!vertexp) {
            vertexp = ਨਵਾਂ Trace_Activity_ਸ਼ੀਰਸ਼{&m_ਗ੍ਰਾਫ, nodep, slow};
            nodep->user3p(vertexp);
        }
        vertexp->slow(slow);
        ਵਾਪਸ vertexp;
    }

    // VISITORS
    ਖਾਲੀ ਦੇਖੋ(AstNetlist* nodep) override {
        // Add vertexes for all TraceDecl, and edges from VARs each trace looks at
        m_finding = ਝੂਠ;
        iterateChildren(nodep);

        // Add vertexes for all CFUNCs, and edges to VARs the func sets
        m_finding = ਸੱਚ;
        iterateChildren(nodep);

        // Create the trace functions and insert them into the tree
        createTraceFunctions();

        // Save number of trace codes used
        nodep->nTraceCodes(m_ਕੋਡ);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Module* nodep) override {
        ਜੇ (nodep->is_ਸਿਖਰ()) m_ਸਿਖਰ_Modp = nodep;
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstVarScope* nodep) override {
        ਜੇ (!m_finding) {
            ਜੇ (ਅਚਲ AstIface* ਅਚਲ ifacep = nodep->varp()->sensIfacep()) {
                m_ifaceMemberVscps[{ifacep, nodep->varp()->name()}].ਧੱਕੋ_ਪਿੱਠ(nodep);
            }
        }
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_Stmt_ਵਿਅੰਜਕ* nodep) override {
        ਜੇ (!m_finding && !nodep->user2()) {
            ਜੇ (AstCCall* ਅਚਲ callp = VN_CAST(nodep->exprp(), CCall)) {
                UINFO(8, "   CCALL " << callp);
                // See if there are other calls in same statement list;
                // If so, all funcs might share the same activity code
                Trace_Activity_ਸ਼ੀਰਸ਼* ਅਚਲ activityVtxp
                    = getActivityVertexp(nodep, callp->funcp()->slow());
                ਲਈ (Ast_ਗੰਢ* nextp = nodep; nextp; nextp = nextp->nextp()) {
                    ਜੇ (Ast_Stmt_ਵਿਅੰਜਕ* ਅਚਲ stmtp = VN_CAST(nextp, Stmt_ਵਿਅੰਜਕ)) {
                        ਜੇ (AstCCall* ਅਚਲ ccallp = VN_CAST(stmtp->exprp(), CCall)) {
                            stmtp->user2(ਸੱਚ);  // Processed
                            UINFO(8, "     SubCCALL " << ccallp);
                            V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* ਅਚਲ ccallFuncVtxp = getCFuncVertexp(ccallp->funcp());
                            activityVtxp->slow(ccallp->funcp()->slow());
                            ਨਵਾਂ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ{&m_ਗ੍ਰਾਫ, activityVtxp, ccallFuncVtxp, 1};
                        }
                    }
                }
            }
        }
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstCFunc* nodep) override {
        UINFO(8, "   CFUNC " << nodep);
        V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* ਅਚਲ funcVtxp = getCFuncVertexp(nodep);
        ਜੇ (!m_finding) {  // If public, we need a unique activity code to allow for sets
                           // directly in this func
            ਜੇ (nodep->func_ਜਨਤਕ() || nodep->dpiExportImpl() || nodep->ਇੰਦਰਾਜ_ਬਿੰਦੂ()
                || nodep->isCoroutine()) {
                // Cannot treat a coroutine as slow, it may be resumed later
                ਅਚਲ ਤਾਰਕਿਕ slow = nodep->slow() && !nodep->isCoroutine();
                Trace_Activity_ਸ਼ੀਰਸ਼* ਅਚਲ activityVtxp = getActivityVertexp(nodep, slow);
                ਨਵਾਂ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ{&m_ਗ੍ਰਾਫ, activityVtxp, funcVtxp, 1};
            }
        }
        VL_RESTORER(m_cfuncp);
        m_cfuncp = nodep;
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstTraceDecl* nodep) override {
        UINFO(8, "   TRACE " << nodep);
        ਜੇ (!m_finding && !nodep->ਵਿੱਚ_Dtype_Func()) {
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
            ਜੇ (m_cfuncp && VString::startsWith(m_cfuncp->name(), "trace_init_leaf_root__")) {
                ਵਾਪਸ;
            }
            V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* ਅਚਲ vertexp = ਨਵਾਂ Trace_Trace_ਸ਼ੀਰਸ਼{&m_ਗ੍ਰਾਫ, nodep};
            nodep->user1p(vertexp);

            UASSERT_OBJ(m_cfuncp, nodep, "Trace not under func");
            m_cfuncp->user2(ਸੱਚ);
            VL_RESTORER(m_tracep);
            m_tracep = nodep;
            iterateChildren(nodep);
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstVarRef* nodep) override {
        UASSERT_OBJ(nodep->varScopep(), nodep, "No var scope?");
        AstVarScope* ਅਚਲ varscopep = nodep->varScopep();
        V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* varVtxp = varscopep->user1u().ਮੰਜ਼ਿਲ_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼();
        ਜੇ (m_tracep) {
            UASSERT_OBJ(nodep->access().isReadOnly(), nodep, "Lvalue in trace?  Should be const.");
            ਜੇ (!varVtxp) {
                varVtxp = ਨਵਾਂ Trace_Var_ਸ਼ੀਰਸ਼{&m_ਗ੍ਰਾਫ, nodep->varScopep()};
                nodep->varScopep()->user1p(varVtxp);
            }
            V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* ਅਚਲ traceVtxp = m_tracep->user1u().ਮੰਜ਼ਿਲ_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼();
            ਨਵਾਂ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ{&m_ਗ੍ਰਾਫ, varVtxp, traceVtxp, 1};
            ਜੇ (nodep->varp()->isPrimaryInish()  // Always need to trace primary inputs
                || nodep->varp()->is_Sig_ਜਨਤਕ()) {  // Or ones user can change
                ਨਵਾਂ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ{&m_ਗ੍ਰਾਫ, m_alwaysVtxp, traceVtxp, 1};
            }
            ਜੇ (m_tracep->dtypeCallp()) varscopep->user2p(m_tracep);
        } ਨਹੀਂਤਾਂ ਜੇ (m_cfuncp && m_finding && nodep->access().isWriteOrRW()) {
            V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* ਅਚਲ funcVtxp = getCFuncVertexp(m_cfuncp);
            ਜੇ (varVtxp) {  // else we're not tracing this signal
                ਨਵਾਂ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ{&m_ਗ੍ਰਾਫ, funcVtxp, varVtxp, 1};
                AstTraceDecl* ਅਚਲ declp = VN_AS(varscopep->user2p(), TraceDecl);
                ਜੇ (declp) {
                    V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* ਅਚਲ cFuncVtxp = getCFuncVertexp(m_cfuncp);
                    ਲਈ (ਅਚਲ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ& ਕਿਨਾਰਾ : cFuncVtxp->ਵਿੱਚ_ਕਿਨਾਰੇ()) {
                        V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* ਅਚਲ activityp = ਕਿਨਾਰਾ.fromp();
                        ਜੇ (!declp->user3p()) {
                            declp->user3p(activityp);
                        } ਨਹੀਂਤਾਂ ਜੇ (declp->user3u().ਮੰਜ਼ਿਲ_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼() != activityp) {
                            declp->user2(ਸੱਚ);
                        }
                    }
                }
            }
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstMemberSel* nodep) override {
        ਜੇ (m_cfuncp && m_finding && nodep->access().isWriteOrRW()) {
            AstIfaceRefDType* ਅਚਲ dtypep
                = VN_CAST(nodep->fromp()->dtypep()->skipRefp(), IfaceRefDType);
            ਜੇ (dtypep && dtypep->isVirtual()) {
                ਅਚਲ ਸਵੈਚਾਲਿਤ it = m_ifaceMemberVscps.ਲੱਭੋ({dtypep->ifacep(), nodep->varp()->name()});
                ਜੇ (it != m_ifaceMemberVscps.ਸਿਰਾ()) {
                    V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* ਅਚਲ funcVtxp = getCFuncVertexp(m_cfuncp);
                    ਲਈ (AstVarScope* ਅਚਲ vscp : it->ਦੂਜਾ) {
                        V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* varVtxp = vscp->user1u().ਮੰਜ਼ਿਲ_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼();
                        ਜੇ (!varVtxp) {
                            varVtxp = ਨਵਾਂ Trace_Var_ਸ਼ੀਰਸ਼{&m_ਗ੍ਰਾਫ, vscp};
                            vscp->user1p(varVtxp);
                        }
                        ਨਵਾਂ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ{&m_ਗ੍ਰਾਫ, funcVtxp, varVtxp, 1};
                    }
                }
            }
        }
        iterateChildren(nodep);
    }
    //--------------------
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override { iterateChildren(nodep); }

ਜਨਤਕ:
    // CONSTRUCTORS
    ਸਪਸ਼ਟ TraceVisitor(AstNetlist* nodep)
        : m_alwaysVtxp{ਨਵਾਂ Trace_Activity_ਸ਼ੀਰਸ਼{&m_ਗ੍ਰਾਫ, Trace_Activity_ਸ਼ੀਰਸ਼::ACTIVITY_ALWAYS}} {
        nodep->user2_Clear_ਰੁੱਖ();  // TraceDecl multiple activities flag
        nodep->user3_Clear_ਰੁੱਖ();  // TraceDecl TraceActivityVertex (assumes we start at nullptr)
        iterate(nodep);
        nodep->foreach([](AstTraceDecl* ਅਚਲ declp) {
            ਜੇ (declp->ਵਿੱਚ_Dtype_Func()) {
                declp->valuep()->unlink_Fr_ਪਿੱਠ()->delete_ਰੁੱਖ();
                declp->valuep(ਖਾਲੀ_ਸੂਚਕ);
            }
        });
    }
    ~TraceVisitor() override {
        V3Stats::ਜੋੜੋ_Stat("Tracing, Activity setters", m_statSetters);
        V3Stats::ਜੋੜੋ_Stat("Tracing, Activity slow blocks", m_statSettersSlow);
        V3Stats::ਜੋੜੋ_Stat("Tracing, Unique trace codes", m_ਕੋਡ);
        V3Stats::ਜੋੜੋ_Stat("Tracing, Unique traced signals", m_statUniqSigs);
    }
};

//######################################################################
// Trace class functions

ਖਾਲੀ V3Trace::traceAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { TraceVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_ਰੁੱਖ("trace", 0, dump_ਰੁੱਖ_Either_Level() >= 3);
}
