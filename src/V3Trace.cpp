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

#అన్తర్భావ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#అన్తర్భావ "V3Trace.h"

#అన్తర్భావ "V3Ast.h"
#అన్తర్భావ "V3DupFinder.h"
#అన్తర్భావ "V3EmitCBase.h"
#అన్తర్భావ "V3Graph.h"
#అన్తర్భావ "V3Stats.h"
#అన్తర్భావ "V3UniqueNames.h"

#అన్తర్భావ <limits>
#అన్తర్భావ <map>
#అన్తర్భావ <set>
#అన్తర్భావ <unordered_map>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Graph vertexes

తరగతి Trace_Activity_శీర్షం final : బహిరంగ V3_లేఖాచిత్రం_శీర్షం {
    VL_RTTI_IMPL(Trace_Activity_శీర్షం, V3_లేఖాచిత్రం_శీర్షం)
    Ast_కణుపు* స్థిరరాశి m_insertp;
    int32_t m_activity_సంకేతం;
    తార్కిక m_slow;  // If always slow, we can use the same code
బహిరంగ:
    గణన { ACTIVITY_NEVER = ((1UL << 31) - 1) };
    గణన { ACTIVITY_ALWAYS = ((1UL << 31) - 2) };
    గణన { ACTIVITY_SLOW = 0 };
    Trace_Activity_శీర్షం(V3_లేఖాచిత్రం* graphp, Ast_కణుపు* nodep, తార్కిక slow)
        : V3_లేఖాచిత్రం_శీర్షం{graphp}
        , m_insertp{nodep} {
        m_activity_సంకేతం = 0;
        m_slow = slow;
    }
    Trace_Activity_శీర్షం(V3_లేఖాచిత్రం* graphp, int32_t సంకేతం_2)
        : V3_లేఖాచిత్రం_శీర్షం{graphp}
        , m_insertp{శూన్యసూచిక} {
        m_activity_సంకేతం = సంకేతం_2;
        m_slow = అబద్ధం;
    }
    ~Trace_Activity_శీర్షం() override = అప్రమేయ;
    // ACCESSORS
    Ast_కణుపు* insertp() స్థిరరాశి {
        UASSERT(m_insertp, "Null insertp; probably called on a special always/slow");
        తిరిగి m_insertp;
    }
    string name() స్థిరరాశి override {
        ఒకవేళ (activityAlways()) {
            తిరిగి "*ALWAYS*";
        } లేకపోతే {
            తిరిగి std::string{slow() ? "*SLOW* " : ""} + insertp()->name();
        }
    }
    string dotColor() స్థిరరాశి override { తిరిగి slow() ? "yellowGreen" : "green"; }
    int32_t activity_సంకేతం() స్థిరరాశి { తిరిగి m_activity_సంకేతం; }
    తార్కిక activityAlways() స్థిరరాశి { తిరిగి activity_సంకేతం() == ACTIVITY_ALWAYS; }
    తార్కిక activitySlow() స్థిరరాశి { తిరిగి activity_సంకేతం() == ACTIVITY_SLOW; }
    శూన్య activity_సంకేతం(int32_t సంకేతం_2) { m_activity_సంకేతం = సంకేతం_2; }
    తార్కిక slow() స్థిరరాశి { తిరిగి m_slow; }
    శూన్య slow(తార్కిక flag) {
        ఒకవేళ (!flag) m_slow = అబద్ధం;
    }
};

తరగతి Trace_CFunc_శీర్షం final : బహిరంగ V3_లేఖాచిత్రం_శీర్షం {
    VL_RTTI_IMPL(Trace_CFunc_శీర్షం, V3_లేఖాచిత్రం_శీర్షం)
    AstCFunc* స్థిరరాశి m_nodep;

బహిరంగ:
    Trace_CFunc_శీర్షం(V3_లేఖాచిత్రం* graphp, AstCFunc* nodep)
        : V3_లేఖాచిత్రం_శీర్షం{graphp}
        , m_nodep{nodep} {}
    ~Trace_CFunc_శీర్షం() override = అప్రమేయ;
    // ACCESSORS
    AstCFunc* nodep() స్థిరరాశి { తిరిగి m_nodep; }
    string name() స్థిరరాశి override { తిరిగి nodep()->name(); }
    string dotColor() స్థిరరాశి override { తిరిగి "yellow"; }
    File_పంక్తి* fileline() స్థిరరాశి override { తిరిగి nodep()->fileline(); }
};

తరగతి Trace_Trace_శీర్షం final : బహిరంగ V3_లేఖాచిత్రం_శీర్షం {
    VL_RTTI_IMPL(Trace_Trace_శీర్షం, V3_లేఖాచిత్రం_శీర్షం)
    AstTraceDecl* స్థిరరాశి m_nodep;  // TRACEINC this represents
    // nullptr, or other vertex with the real code() that duplicates this one
    Trace_Trace_శీర్షం* m_duplicatep = శూన్యసూచిక;
    // When aliasing to a dtype parent, offset of the member within the dtype
    uint32_t m_dtype_Alias_స్థానభ్రంశం = 0;

బహిరంగ:
    Trace_Trace_శీర్షం(V3_లేఖాచిత్రం* graphp, AstTraceDecl* nodep)
        : V3_లేఖాచిత్రం_శీర్షం{graphp}
        , m_nodep{nodep} {}
    ~Trace_Trace_శీర్షం() override = అప్రమేయ;
    // ACCESSORS
    AstTraceDecl* nodep() స్థిరరాశి { తిరిగి m_nodep; }
    string name() స్థిరరాశి override { తిరిగి nodep()->name(); }
    string dotColor() స్థిరరాశి override { తిరిగి "red"; }
    File_పంక్తి* fileline() స్థిరరాశి override { తిరిగి nodep()->fileline(); }
    Trace_Trace_శీర్షం* duplicatep() స్థిరరాశి { తిరిగి m_duplicatep; }
    శూన్య duplicatep(Trace_Trace_శీర్షం* dupp) {
        UASSERT_OBJ(!duplicatep(), nodep(), "Assigning duplicatep() to already duplicated node");
        m_duplicatep = dupp;
    }
    శూన్య redirectDuplicatep(Trace_Trace_శీర్షం* dupp) { m_duplicatep = dupp; }
    uint32_t dtype_Alias_స్థానభ్రంశం() స్థిరరాశి { తిరిగి m_dtype_Alias_స్థానభ్రంశం; }
    శూన్య dtype_Alias_స్థానభ్రంశం(uint32_t స్థానభ్రంశం) { m_dtype_Alias_స్థానభ్రంశం = స్థానభ్రంశం; }
};

తరగతి Trace_Var_శీర్షం final : బహిరంగ V3_లేఖాచిత్రం_శీర్షం {
    VL_RTTI_IMPL(Trace_Var_శీర్షం, V3_లేఖాచిత్రం_శీర్షం)
    AstVarScope* స్థిరరాశి m_nodep;

బహిరంగ:
    Trace_Var_శీర్షం(V3_లేఖాచిత్రం* graphp, AstVarScope* nodep)
        : V3_లేఖాచిత్రం_శీర్షం{graphp}
        , m_nodep{nodep} {}
    ~Trace_Var_శీర్షం() override = అప్రమేయ;
    // ACCESSORS
    AstVarScope* nodep() స్థిరరాశి { తిరిగి m_nodep; }
    string name() స్థిరరాశి override { తిరిగి nodep()->name(); }
    string dotColor() స్థిరరాశి override { తిరిగి "skyblue"; }
    File_పంక్తి* fileline() స్థిరరాశి override { తిరిగి nodep()->fileline(); }
};

//######################################################################
// Trace state, as a visitor of each AstNode

తరగతి TraceVisitor final : బహిరంగ VNVisitor {
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
    స్థిరరాశి VNUser1_లోపల_Use m_inuser1;
    స్థిరరాశి VNUser2_లోపల_Use m_inuser2;
    స్థిరరాశి VNUser3_లోపల_Use m_inuser3;
    // VNUser4InUse     In V3Hasher via V3DupFinder

    // STATE
    Ast_కణుపు_Module* m_శిఖరం_Modp = శూన్యసూచిక;  // Module to add variables to
    AstScope* స్థిరరాశి m_శిఖరం_Scopep = v3Global.rootp()->శిఖరం_Scopep()->scopep();  // The top AstScope
    AstCFunc* m_cfuncp = శూన్యసూచిక;  // C function adding to graph
    AstCFunc* m_regFuncp = శూన్యసూచిక;  // Trace registration function
    AstCFunc* m_actAllFuncp = శూన్యసూచిక;  // Set all activity function
    AstTraceDecl* m_tracep = శూన్యసూచిక;  // Trace function adding to graph
    AstVarScope* m_activityVscp = శూన్యసూచిక;  // Activity variable
    uint32_t m_activity_సంఖ్య = 0;  // Count of fields in activity variable
    uint32_t m_సంకేతం = 0;  // Trace ident code# being assigned
    V3_లేఖాచిత్రం m_లేఖాచిత్రం;  // Var/CFunc tracking
    Trace_Activity_శీర్షం* స్థిరరాశి m_alwaysVtxp;  // "Always trace" vertex
    తార్కిక m_finding = అబద్ధం;  // Pass one of algorithm?
    నిర్మాణం DtypeFuncs final {
    బహిరంగ:
        AstCFunc* నిండిన_Funcp = శూన్యసూచిక;
        AstCFunc* chgFuncp = శూన్యసూచిక;
    };
    std::unordered_map<స్థిరరాశి Ast_కణుపు_DType*, DtypeFuncs>
        m_dtypeNonConstFuncs;  // Full / Chg funcs per type
    std::unordered_map<స్థిరరాశి Ast_కణుపు_DType*, AstCFunc*> m_dtypeConstFuncs;  // Const func per type
    V3_Unique_పేర్లు m_dtype_పేర్లు{""};  // Unique type func names

    // Trace parallelism. Only VCD tracing can be parallelized at this time.
    స్థిరరాశి uint32_t m_parallelism
        = v3Global.opt.useTraceParallel() ? static_cast<uint32_t>(v3Global.opt.దారాలు()) : 1;

    VDouble0 m_statSetters;  // Statistic tracking
    VDouble0 m_statSettersSlow;  // Statistic tracking
    VDouble0 m_statUniqSigs;  // Statistic tracking

    // All activity numbers applying to a given trace
    ఉపయోగించి Act_సంకేతం_సమితి = std::సమితి<uint32_t>;
    // For activity set, what traces apply
    ఉపయోగించి TraceVec = std::multimap<Act_సంకేతం_సమితి, Trace_Trace_శీర్షం*>;
    // Candidate interface-member VarScopes keyed by (interface type, member name)
    std::map<std::pair<స్థిరరాశి AstIface*, std::string>, std::సదిశ<AstVarScope*>>
        m_ifaceMemberVscps;

    తరగతి TraceInitDeclCollector final : బహిరంగ VNVisitor {
        std::సదిశ<AstTraceDecl*>& m_declps;  // Output: trace declarations found so far
        std::సమితి<స్థిరరాశి AstCFunc*> m_చూసిన_Funcps;

        శూన్య సందర్శించు(AstTraceDecl* nodep) override { m_declps.నెట్టు_వెనుక(nodep); }
        శూన్య సందర్శించు(AstCCall* nodep) override {
            ఒకవేళ (AstCFunc* స్థిరరాశి funcp = nodep->funcp()) collect(funcp);
        }
        శూన్య సందర్శించు(Ast_కణుపు* nodep) override { iterateChildren(nodep); }

    బహిరంగ:
        స్పష్టమైన TraceInitDeclCollector(std::సదిశ<AstTraceDecl*>& declps)
            : m_declps{declps} {}
        శూన్య collect(AstCFunc* funcp) {
            ఒకవేళ (funcp && m_చూసిన_Funcps.చొప్పించు(funcp).రెండవ) iterate(funcp);
        }
    };

    // METHODS

    స్థిర తార్కిక same_మూలం_Init_Alias(స్థిరరాశి AstTraceDecl* మూలం_Declp, స్థిరరాశి AstTraceDecl* శిఖరం_Declp) {
        స్థిరరాశి VNumRange& మూలం_బిట్టు_Range = మూలం_Declp->బిట్టు_Range();
        స్థిరరాశి VNumRange& శిఖరం_బిట్టు_Range = శిఖరం_Declp->బిట్టు_Range();
        తిరిగి మూలం_Declp->showname() == శిఖరం_Declp->showname()
               && మూలం_Declp->declDirection() == శిఖరం_Declp->declDirection()
               && మూలం_Declp->వెడల్పు_Min() == శిఖరం_Declp->వెడల్పు_Min()
               && మూలం_బిట్టు_Range.ranged() == శిఖరం_బిట్టు_Range.ranged()
               && (!మూలం_బిట్టు_Range.ranged()
                   || (మూలం_బిట్టు_Range.ఎడమ() == శిఖరం_బిట్టు_Range.ఎడమ()
                       && మూలం_బిట్టు_Range.కుడి() == శిఖరం_బిట్టు_Range.కుడి()));
    }

    శూన్య detectDuplicates() {
        UINFO(9, "Finding duplicates");
        // Note uses user4
        V3DupFinder dupFinder;  // Duplicate code detection

        // Compute member offsets within dtype instances incrementally.
        // For each dtype member TraceDecl, stores its code offset within the
        // dtype parent's code range.
        std::unordered_map<స్థిరరాశి AstTraceDecl*, uint32_t> memberOffsets;
        std::unordered_map<స్థిరరాశి AstTraceDecl*, uint32_t> running_స్థానభ్రంశం;

        // For fixup pass: track dups redirected to dtype parents, so we can
        // fix them if the parent's user2 is later set during the loop.
        // Maps dup vertex -> original canonical member vertex.
        std::సదిశ<std::pair<Trace_Trace_శీర్షం*, Trace_Trace_శీర్షం*>> dtype_మాతృ_Redirects;

        // Hash all of the traced values and find if there are any duplicates
        కొరకు (V3_లేఖాచిత్రం_శీర్షం& vtx : m_లేఖాచిత్రం.vertices()) {
            ఒకవేళ (Trace_Trace_శీర్షం* స్థిరరాశి vvertexp = vtx.cast<Trace_Trace_శీర్షం>()) {
                AstTraceDecl* స్థిరరాశి nodep = vvertexp->nodep();
                ఒకవేళ (nodep->dtypeCallp()) కొనసాగు;

                ఒకవేళ (nodep->dtypeDeclp()) {
                    uint32_t& స్థానభ్రంశం = running_స్థానభ్రంశం[nodep->dtypeDeclp()];
                    memberOffsets[nodep] = స్థానభ్రంశం;
                    స్థానభ్రంశం += nodep->సంకేతం_Inc();
                }

                UASSERT_OBJ(!vvertexp->duplicatep(), nodep, "Should not be a duplicate");
                స్థిరరాశి స్వయంచాలక dupit = dupFinder.కనుగొను_Duplicate(nodep);
                ఒకవేళ (dupit == dupFinder.చివర()) {
                    dupFinder.చొప్పించు(nodep);
                } లేకపోతే {
                    AstTraceDecl* స్థిరరాశి dupDeclp = VN_AS(dupit->రెండవ, TraceDecl);
                    UASSERT_OBJ(dupDeclp, nodep, "Trace duplicate of wrong type");
                    Trace_Trace_శీర్షం* స్థిరరాశి dupvertexp
                        = dupDeclp->user1u().గమ్యం_లేఖాచిత్రం_శీర్షం()->cast<Trace_Trace_శీర్షం>();
                    UINFO(8, "  Orig " << dupDeclp << పంక్తి_ముగింపు);
                    UINFO(8, "   dup " << nodep << పంక్తి_ముగింపు);

                    ఒకవేళ (dupDeclp->dtypeDeclp() && !dupDeclp->dtypeDeclp()->user2()) {
                        AstTraceDecl* స్థిరరాశి dtypeParentp = dupDeclp->dtypeDeclp();
                        Trace_Trace_శీర్షం* స్థిరరాశి dtypeVtxp
                            = dtypeParentp->user1u().గమ్యం_లేఖాచిత్రం_శీర్షం()->cast<Trace_Trace_శీర్షం>();
                        స్థిరరాశి స్వయంచాలక it2 = memberOffsets.కనుగొను(dupDeclp);
                        UASSERT_OBJ(it2 != memberOffsets.చివర(), dupDeclp,
                                    "Member offset not precomputed");
                        vvertexp->duplicatep(dtypeVtxp);
                        vvertexp->dtype_Alias_స్థానభ్రంశం(it2->రెండవ);
                        dtype_మాతృ_Redirects.emplace_వెనుక(vvertexp, dupvertexp);
                    } లేకపోతే {
                        vvertexp->duplicatep(dupvertexp);
                    }
                    ఒకవేళ (nodep->dtypeDeclp()) nodep->dtypeDeclp()->user2(నిజం);
                }
            }
        }

        కొరకు (స్థిరరాశి స్వయంచాలక& pair : dtype_మాతృ_Redirects) {
            Trace_Trace_శీర్షం* స్థిరరాశి dupVtxp = pair.మొదటి;
            Trace_Trace_శీర్షం* స్థిరరాశి memberVtxp = pair.రెండవ;
            స్థిరరాశి AstTraceDecl* స్థిరరాశి మాతృ_Declp = dupVtxp->duplicatep()->nodep();
            ఒకవేళ (మాతృ_Declp->user2()) {
                dupVtxp->redirectDuplicatep(memberVtxp);
                dupVtxp->dtype_Alias_స్థానభ్రంశం(0);
            }
        }

        ఒకవేళ (dumpLevel() || debug() >= 9)
            dupFinder.dumpFile(v3Global.debugFilename("trace") + ".hash", అబద్ధం);
    }

    శూన్య లేఖాచిత్రం_Dtype_Prune() {
        కొరకు (V3_లేఖాచిత్రం_శీర్షం* స్థిరరాశి vtxp : m_లేఖాచిత్రం.vertices().unlinkable()) {
            ఒకవేళ (Trace_Trace_శీర్షం* స్థిరరాశి vvertexp = vtxp->cast<Trace_Trace_శీర్షం>()) {
                AstTraceDecl* స్థిరరాశి declp = vvertexp->nodep();
                // This skips the dtype sub-func optimization if a var is affected by multiple
                // activities.  We really only need to do this for _chg funcs (and not decls,
                // _const and _full) but it's simpiler to do it all one way or the other.
                ఒకవేళ (declp) {
                    ఒకవేళ (declp->user2() || (declp->dtypeDeclp() && !declp->dtypeDeclp()->user2())) {
                        AstCCall* స్థిరరాశి callp = declp->dtypeCallp();
                        ఒకవేళ (callp) {
                            Ast_కణుపు* stmtexprp = callp->backp();
                            VL_DO_DANGLING(నెట్టు_Deletep(stmtexprp->unlink_Fr_వెనుక()), stmtexprp);
                        } లేకపోతే {
                            తార్కిక ఖాళీలు_Scope;
                            చేయి {
                                ఖాళీలు_Scope = అబద్ధం;
                                Ast_కణుపు* స్థిరరాశి declBackp = declp->backp();
                                Ast_కణుపు* స్థిరరాశి declNextp = declp->nextp();
                                ఒకవేళ (VN_IS(declBackp, Trace_నెట్టు_ఉపసర్గ)
                                    && VN_IS(declNextp, Trace_తీయి_ఉపసర్గ)) {
                                    VL_DO_DANGLING(నెట్టు_Deletep(declBackp->unlink_Fr_వెనుక()),
                                                   declBackp);
                                    VL_DO_DANGLING(నెట్టు_Deletep(declNextp->unlink_Fr_వెనుక()),
                                                   declNextp);
                                    ఖాళీలు_Scope = నిజం;
                                }
                            } అయితే (ఖాళీలు_Scope);
                        }
                        // Can't purge until we finish this pass
                        నెట్టు_Deletep(declp->unlink_Fr_వెనుక());  // declp used below
                        vvertexp->reroute_అంచులు(&m_లేఖాచిత్రం);
                        vvertexp->unlinkDelete(&m_లేఖాచిత్రం);
                    }
                    declp->dtypeDeclp(శూన్యసూచిక);
                }
            }
        }
    }

    శూన్య విభజించు_Trace_Decl_Funcs() {
        స్థిరరాశి పూర్ణాంకం విభజించు_హద్దు = v3Global.opt.output_విభజించు_CTrace() ? v3Global.opt.output_విభజించు_CTrace()
                                                                : std::numeric_limits<పూర్ణాంకం>::max();
        ఒకవేళ (విభజించు_హద్దు == std::numeric_limits<పూర్ణాంకం>::max()) తిరిగి;

        కొరకు (Ast_కణుపు* nodep = m_శిఖరం_Scopep->blocksp(); nodep; nodep = nodep->nextp()) {
            AstCFunc* స్థిరరాశి funcp = VN_CAST(nodep, CFunc);
            ఒకవేళ (!funcp || !funcp->user2()) కొనసాగు;

            స్థిరరాశి string పునాది_Name = funcp->name();
            uint32_t sub_అంకె = 1;
            పూర్ణాంకం cur_కొలత = 0;
            AstCFunc* curFuncp = funcp;
            std::సదిశ<Ast_కణుపు_Stmt*> callStmts;

            స్వయంచాలక ప్రారంభం_New_Sub_Func = [&]() {
                File_పంక్తి* స్థిరరాశి flp = funcp->fileline();
                స్థిరరాశి string newName = పునాది_Name + "_" + cvt_గమ్యం_Str(sub_అంకె++);
                AstCFunc* స్థిరరాశి newFuncp = కొత్త AstCFunc{flp, newName, m_శిఖరం_Scopep};
                newFuncp->argTypes(v3Global.opt.trace_Class_పునాది() + "* tracep");
                newFuncp->isTrace(నిజం);
                newFuncp->isStatic(అబద్ధం);
                newFuncp->isLoose(నిజం);
                newFuncp->slow(నిజం);
                newFuncp->dontCombine(నిజం);
                m_శిఖరం_Scopep->కూడు_Blocksp(newFuncp);
                newFuncp->కూడు_Stmtsp(కొత్త AstCStmt{flp, "const int c = vlSymsp->__Vm_baseCode;"});
                AstCCall* స్థిరరాశి callp = కొత్త AstCCall{flp, newFuncp};
                callp->dtype_సమితి_Void();
                callp->argTypes("tracep");
                callStmts.నెట్టు_వెనుక(callp->makeStmt());
                curFuncp = newFuncp;
                cur_కొలత = 0;
            };

            కొరకు (Ast_కణుపు *stmtp = funcp->stmtsp(), *nextp; stmtp; stmtp = nextp) {
                nextp = stmtp->nextp();
                cur_కొలత += stmtp->కణుపు_లెక్క();

                ఒకవేళ (curFuncp != funcp) {
                    stmtp->unlink_Fr_వెనుక();
                    curFuncp->కూడు_Stmtsp(stmtp);
                }

                ఒకవేళ (cur_కొలత > విభజించు_హద్దు && nextp) ప్రారంభం_New_Sub_Func();
            }

            కొరకు (Ast_కణుపు_Stmt* స్థిరరాశి callStmtp : callStmts) { funcp->కూడు_Stmtsp(callStmtp); }
        }
    }

    శూన్య లేఖాచిత్రం_Simplify(తార్కిక initial) {
        ఒకవేళ (initial) {
            // Remove all variable nodes
            కొరకు (V3_లేఖాచిత్రం_శీర్షం* స్థిరరాశి vtxp : m_లేఖాచిత్రం.vertices().unlinkable()) {
                ఒకవేళ (Trace_Var_శీర్షం* స్థిరరాశి vvertexp = vtxp->cast<Trace_Var_శీర్షం>()) {
                    vvertexp->reroute_అంచులు(&m_లేఖాచిత్రం);
                    vvertexp->unlinkDelete(&m_లేఖాచిత్రం);
                }
            }
            // Remove multiple variables connecting funcs to traces
            // We do this twice, as then we have fewer edges to multiply out in the below
            // expansion.
            m_లేఖాచిత్రం.remove_Redundant_అంచులు_Max(&V3_లేఖాచిత్రం_అంచు::followAlwaysTrue);
            // Remove all Cfunc nodes
            కొరకు (V3_లేఖాచిత్రం_శీర్షం* స్థిరరాశి vtxp : m_లేఖాచిత్రం.vertices().unlinkable()) {
                ఒకవేళ (Trace_CFunc_శీర్షం* స్థిరరాశి vvertexp = vtxp->cast<Trace_CFunc_శీర్షం>()) {
                    vvertexp->reroute_అంచులు(&m_లేఖాచిత్రం);
                    vvertexp->unlinkDelete(&m_లేఖాచిత్రం);
                }
            }
        }

        // Remove multiple variables connecting funcs to traces
        m_లేఖాచిత్రం.remove_Redundant_అంచులు_Max(&V3_లేఖాచిత్రం_అంచు::followAlwaysTrue);

        // If there are any edges from a always, keep only the always
        కొరకు (V3_లేఖాచిత్రం_శీర్షం& vtx : m_లేఖాచిత్రం.vertices()) {
            ఒకవేళ (Trace_Trace_శీర్షం* స్థిరరాశి vvertexp = vtx.cast<Trace_Trace_శీర్షం>()) {
                // Search for the incoming always edge
                స్థిరరాశి V3_లేఖాచిత్రం_అంచు* alwaysEdgep = శూన్యసూచిక;
                కొరకు (స్థిరరాశి V3_లేఖాచిత్రం_అంచు& అంచు : vvertexp->లోపల_అంచులు()) {
                    స్థిరరాశి Trace_Activity_శీర్షం* స్థిరరాశి actVtxp
                        = అంచు.fromp()->as<స్థిరరాశి Trace_Activity_శీర్షం>();
                    ఒకవేళ (actVtxp->activityAlways()) {
                        alwaysEdgep = &అంచు;
                        ఆపు;
                    }
                }
                // If always edge exists, remove all other edges
                ఒకవేళ (alwaysEdgep) {
                    కొరకు (V3_లేఖాచిత్రం_అంచు* స్థిరరాశి edgep : vvertexp->లోపల_అంచులు().unlinkable()) {
                        ఒకవేళ (edgep != alwaysEdgep) VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
                    }
                }
            }
        }

        // Activity points with no outputs can be removed
        కొరకు (V3_లేఖాచిత్రం_శీర్షం* స్థిరరాశి vtxp : m_లేఖాచిత్రం.vertices().unlinkable()) {
            ఒకవేళ (Trace_Activity_శీర్షం* స్థిరరాశి aVtxp = vtxp->cast<Trace_Activity_శీర్షం>()) {
                // Leave in the always vertex for later use.
                ఒకవేళ (aVtxp != m_alwaysVtxp && aVtxp->బహిర్గతం_ఖాళీలు()) {
                    VL_DO_DANGLING(aVtxp->unlinkDelete(&m_లేఖాచిత్రం), aVtxp);
                }
            }
        }
    }

    uint32_t assignactivityNumbers() {
        uint32_t activity_సంఖ్య = 1;  // Note 0 indicates "slow" only
        కొరకు (V3_లేఖాచిత్రం_శీర్షం& vtx : m_లేఖాచిత్రం.vertices()) {
            ఒకవేళ (Trace_Activity_శీర్షం* స్థిరరాశి vvertexp = vtx.cast<Trace_Activity_శీర్షం>()) {
                ఒకవేళ (vvertexp != m_alwaysVtxp) {
                    ఒకవేళ (vvertexp->slow()) {
                        vvertexp->activity_సంకేతం(Trace_Activity_శీర్షం::ACTIVITY_SLOW);
                    } లేకపోతే {
                        vvertexp->activity_సంకేతం(activity_సంఖ్య++);
                    }
                }
            }
        }
        తిరిగి activity_సంఖ్య;
    }

    శూన్య క్రమీకరించు_Traces(TraceVec& traces, uint32_t& nNonConstCodes) {
        // Populate sort structure
        traces.clear();
        nNonConstCodes = 0;
        కొరకు (V3_లేఖాచిత్రం_శీర్షం& vtx : m_లేఖాచిత్రం.vertices()) {
            ఒకవేళ (Trace_Trace_శీర్షం* స్థిరరాశి vtxp = vtx.cast<Trace_Trace_శీర్షం>()) {
                Act_సంకేతం_సమితి act_సమితి;
                UINFO(9, "  Add to sort: " << vtxp);
                UINFOTREE(9, vtxp->nodep(), "", "trnode");
                కొరకు (స్థిరరాశి V3_లేఖాచిత్రం_అంచు& అంచు : vtxp->లోపల_అంచులు()) {
                    స్థిరరాశి Trace_Activity_శీర్షం* స్థిరరాశి cfvertexp
                        = అంచు.fromp()->cast<స్థిరరాశి Trace_Activity_శీర్షం>();
                    UASSERT_OBJ(cfvertexp, vtxp->nodep(),
                                "Should have been function pointing to this trace");
                    UINFO(9, "   Activity: " << cfvertexp);
                    ఒకవేళ (cfvertexp->activityAlways()) {
                        // If code 0, we always trace; ignore other codes
                        act_సమితి.చొప్పించు(Trace_Activity_శీర్షం::ACTIVITY_ALWAYS);
                    } లేకపోతే {
                        act_సమితి.చొప్పించు(cfvertexp->activity_సంకేతం());
                    }
                }
                UASSERT_OBJ(act_సమితి.లెక్క(Trace_Activity_శీర్షం::ACTIVITY_ALWAYS) == 0
                                || act_సమితి.కొలత() == 1,
                            vtxp->nodep(), "Always active trace has further triggers");
                // Count nodes
                ఒకవేళ (!vtxp->duplicatep() && !act_సమితి.ఖాళీలు())
                    nNonConstCodes += vtxp->nodep()->సంకేతం_Inc();
                ఒకవేళ (act_సమితి.ఖాళీలు()) {
                    // If a trace doesn't have activity, it's constant, and we
                    // don't need to track changes on it.
                    act_సమితి.చొప్పించు(Trace_Activity_శీర్షం::ACTIVITY_NEVER);
                } లేకపోతే ఒకవేళ (act_సమితి.లెక్క(Trace_Activity_శీర్షం::ACTIVITY_SLOW) && act_సమితి.కొలత() > 1) {
                    // If a trace depends on the slow flag as well as other
                    // flags, remove the dependency on the slow flag. We will
                    // make slow routines set all activity flags.
                    act_సమితి.erase(Trace_Activity_శీర్షం::ACTIVITY_SLOW);
                }
                traces.emplace(act_సమితి, vtxp);
            }
        }
    }

    శూన్య లేఖాచిత్రం_Optimize() {
        // Assign initial activity numbers to activity vertices
        assignactivityNumbers();

        // Sort the traces by activity sets
        TraceVec traces;
        uint32_t unused1;
        క్రమీకరించు_Traces(traces, unused1);

        // For each activity set with only a small number of signals, make those
        // signals always traced, as it's cheaper to check a few value changes
        // than to test a lot of activity flags
        స్వయంచాలక it = traces.begin();
        స్థిరరాశి స్వయంచాలక చివర = traces.చివర();
        అయితే (it != చివర) {
            స్వయంచాలక తల = it;
            // Approximate the complexity of the value change check
            uint32_t complexity = 0;
            స్థిరరాశి Act_సంకేతం_సమితి& act_సమితి = it->మొదటి;
            కొరకు (; it != చివర && it->మొదటి == act_సమితి; ++it) {
                ఒకవేళ (!it->రెండవ->duplicatep()) {
                    uint32_t ఖర్చు = 0;
                    స్థిరరాశి AstTraceDecl* స్థిరరాశి declp = it->రెండవ->nodep();
                    // The number of comparisons required by bufp->chg*
                    ఖర్చు += declp->isWide() ? declp->సంకేతం_Inc() : 1;
                    // Arrays are traced by element
                    ఖర్చు *= declp->arrayRange().ranged() ? declp->arrayRange().elements() : 1;
                    // Note: Experiments factoring in the size of declp->valuep()
                    // showed no benefit in tracing speed, even for large trees,
                    // so we will leave those out for now.
                    complexity += ఖర్చు;
                }
            }
            // Leave alone always changing, never changing and signals only set in slow code
            ఒకవేళ (act_సమితి.లెక్క(Trace_Activity_శీర్షం::ACTIVITY_ALWAYS)) కొనసాగు;
            ఒకవేళ (act_సమితి.లెక్క(Trace_Activity_శీర్షం::ACTIVITY_NEVER)) కొనసాగు;
            ఒకవేళ (act_సమితి.లెక్క(Trace_Activity_శీర్షం::ACTIVITY_SLOW)) కొనసాగు;
            // If the value comparisons are cheaper to perform than checking the
            // activity flags make the signals always traced. Note this cost
            // equation is heuristic.
            ఒకవేళ (complexity <= act_సమితి.కొలత() * 2) {
                కొరకు (; తల != it; ++తల) {
                    కొత్త V3_లేఖాచిత్రం_అంచు{&m_లేఖాచిత్రం, m_alwaysVtxp, తల->రెండవ, 1};
                }
            }
        }

        లేఖాచిత్రం_Simplify(అబద్ధం);
    }

    Ast_కణుపు_సమాసం* selectActivity(File_పంక్తి* flp, uint32_t acode, స్థిరరాశి VAccess& access) {
        తిరిగి కొత్త AstArraySel(flp, కొత్త AstVarRef{flp, m_activityVscp, access}, acode);
    }

    Ast_కణుపు* newActivitySetter(Ast_కణుపు* insertp, uint32_t సంకేతం_2) {
        ++m_statSetters;
        File_పంక్తి* స్థిరరాశి fl = insertp->fileline();
        Ast_కేటాయించు* స్థిరరాశి setterp = కొత్త Ast_కేటాయించు{fl, selectActivity(fl, సంకేతం_2, VAccess::WRITE),
                                                 కొత్త AstConst{fl, AstConst::బిట్టు_True{}}};
        తిరిగి setterp;
    }

    Ast_కణుపు* newActivityAll(Ast_కణుపు* insertp) {
        ++m_statSettersSlow;
        ఒకవేళ (!m_actAllFuncp) {
            File_పంక్తి* స్థిరరాశి flp = m_శిఖరం_Scopep->fileline();
            AstCFunc* స్థిరరాశి funcp = కొత్త AstCFunc{flp, "__VtraceActivitySetAll", m_శిఖరం_Scopep};
            funcp->slow(నిజం);
            funcp->isStatic(అబద్ధం);
            funcp->isLoose(నిజం);
            m_శిఖరం_Scopep->కూడు_Blocksp(funcp);
            కొరకు (uint32_t సంకేతం_2 = 0; సంకేతం_2 < m_activity_సంఖ్య; ++సంకేతం_2) {
                Ast_కణుపు* స్థిరరాశి setterp = newActivitySetter(insertp, సంకేతం_2);
                funcp->కూడు_Stmtsp(setterp);
            }
            m_actAllFuncp = funcp;
        }
        AstCCall* స్థిరరాశి callp = కొత్త AstCCall{insertp->fileline(), m_actAllFuncp};
        callp->dtype_సమితి_Void();
        తిరిగి callp->makeStmt();
    }

    శూన్య createActivityFlags() {
        // Assign final activity numbers
        m_activity_సంఖ్య = assignactivityNumbers();

        // Create an array of bytes, not a bit vector, as they can be set
        // atomically by mtasks, and are cheaper to set (no need for
        // read-modify-write on the C type), and the speed of the tracing code
        // is the same on largish designs.
        File_పంక్తి* స్థిరరాశి flp = m_శిఖరం_Scopep->fileline();
        Ast_కణుపు_DType* స్థిరరాశి newScalarDtp = కొత్త AstBasicDType{flp, VFlag_బిట్టు_Packed{}, 1};
        v3Global.rootp()->typeTablep()->కూడు_Typesp(newScalarDtp);
        AstRange* స్థిరరాశి newArange
            = కొత్త AstRange{flp, VNumRange{static_cast<పూర్ణాంకం>(m_activity_సంఖ్య) - 1, 0}};
        Ast_కణుపు_DType* స్థిరరాశి newArrDtp = కొత్త AstUnpackArrayDType{flp, newScalarDtp, newArange};
        v3Global.rootp()->typeTablep()->కూడు_Typesp(newArrDtp);
        m_activityVscp = v3Global.rootp()->శిఖరం_Scopep()->create_తాత్కాలిక("__VtraceActivity", newArrDtp);

        // Insert activity setters
        కొరకు (స్థిరరాశి V3_లేఖాచిత్రం_శీర్షం& vtx : m_లేఖాచిత్రం.vertices()) {
            ఒకవేళ (స్థిరరాశి Trace_Activity_శీర్షం* స్థిరరాశి vtxp = vtx.cast<స్థిరరాశి Trace_Activity_శీర్షం>()) {
                Ast_కణుపు* setterp = శూన్యసూచిక;
                ఒకవేళ (vtxp->activitySlow()) {
                    setterp = newActivityAll(vtxp->insertp());
                    // Just set all flags in slow code as it should be rare.
                    // This will be rolled up into a loop by V3Reloop.
                } లేకపోతే ఒకవేళ (!vtxp->activityAlways()) {
                    setterp = newActivitySetter(vtxp->insertp(), vtxp->activity_సంకేతం());
                }
                ఒకవేళ (setterp) {
                    Ast_కణుపు* స్థిరరాశి insertp = vtxp->insertp();
                    ఒకవేళ (Ast_Stmt_సమాసం* స్థిరరాశి stmtp = VN_CAST(insertp, Stmt_సమాసం)) {
                        stmtp->కూడు_తదుపరి_ఇక్కడ(setterp);
                    } లేకపోతే ఒకవేళ (AstCFunc* స్థిరరాశి funcp = VN_CAST(insertp, CFunc)) {
                        // If there are awaits, insert the setter after each await
                        ఒకవేళ (funcp->isCoroutine() && funcp->stmtsp()) {
                            funcp->stmtsp()->foreach_And_తదుపరి([setterp](AstCAwait* awaitp) {
                                awaitp->కూడు_తదుపరి_ఇక్కడ(setterp->clone_వృక్షం(అబద్ధం));
                            });
                        }
                        funcp->కూడు_Stmtsp(setterp);
                    } లేకపోతే {
                        insertp->v3fatalSrc("Bad trace activity vertex");
                    }
                }
            }
        }
    }

    AstCFunc* newCFunc(VTraceType traceType, AstCFunc* శిఖరం_Funcp, uint32_t func_అంకె,
                       uint32_t పునాది_సంకేతం = 0, స్థిరరాశి AstTraceDecl* స్థిరరాశి declp = శూన్యసూచిక,
                       తార్కిక declSub = అబద్ధం) {
        // Create new function
        స్థిరరాశి తార్కిక is_శిఖరం_Func = !declp && శిఖరం_Funcp == శూన్యసూచిక;
        std::string funcName;
        ఒకవేళ (is_శిఖరం_Func || declp) {
            funcName = traceType.func_ఉపసర్గ();
        } లేకపోతే {
            funcName = శిఖరం_Funcp->name();
            funcName += "_sub";
        }
        ఒకవేళ (declp) {
            funcName += "_dtype";
            ఒకవేళ (declSub) funcName += "_sub";
            funcName += "__";
            funcName = m_dtype_పేర్లు.get(funcName);
        } లేకపోతే {
            funcName += "_";
            funcName += cvt_గమ్యం_Str(func_అంకె);
        }

        File_పంక్తి* స్థిరరాశి flp = m_శిఖరం_Scopep->fileline();
        AstCFunc* స్థిరరాశి funcp = కొత్త AstCFunc{flp, funcName, m_శిఖరం_Scopep};
        funcp->isTrace(నిజం);
        funcp->dontCombine(నిజం);
        funcp->isLoose(నిజం);
        funcp->slow(traceType != VTraceType::CHANGE);
        funcp->isStatic(is_శిఖరం_Func);
        // Add it to top scope
        m_శిఖరం_Scopep->కూడు_Blocksp(funcp);
        స్థిరరాశి std::string bufArg
            = v3Global.opt.trace_Class_పునాది() + "::Buffer* bufp"
              + (declp ? (", uint32_t offset, const " + declp->dtypep()->cType("", నిజం, నిజం)
                          + " __VdtypeVar")
                       : "");
        ఒకవేళ (is_శిఖరం_Func) {
            // Top functions
            funcp->argTypes("void* voidSelf, " + bufArg);
            funcp->కూడు_Stmtsp(కొత్త AstCStmt{flp, EmitCUtil::void_Self_కేటాయించు(m_శిఖరం_Modp)});
            funcp->కూడు_Stmtsp(కొత్త AstCStmt{flp, EmitCUtil::sym_Class_కేటాయించు()});
            // Add global activity check to change dump functions
            ఒకవేళ (traceType == VTraceType::CHANGE) {  //
                funcp->కూడు_Stmtsp(
                    కొత్త AstCStmt{flp, "if (VL_UNLIKELY(!vlSymsp->__Vm_activity)) return;"});
            }
            // Register function
            AstCStmt* స్థిరరాశి cstmtp = కొత్త AstCStmt{flp};
            m_regFuncp->కూడు_Stmtsp(cstmtp);
            ఒకవేళ (traceType == VTraceType::CONSTANT) {
                cstmtp->కూడు("tracep->addConstCb(");
                cstmtp->కూడు(కొత్త Ast_Addr_యొక్క_CFunc{flp, funcp});
                cstmtp->కూడు(", " + std::గమ్యం_string(func_అంకె) + ", vlSelf);");
            } లేకపోతే ఒకవేళ (traceType == VTraceType::నిండిన) {
                cstmtp->కూడు("tracep->addFullCb(");
                cstmtp->కూడు(కొత్త Ast_Addr_యొక్క_CFunc{flp, funcp});
                cstmtp->కూడు(", " + std::గమ్యం_string(func_అంకె) + ", vlSelf);");
            } లేకపోతే {
                cstmtp->కూడు("tracep->addChgCb(");
                cstmtp->కూడు(కొత్త Ast_Addr_యొక్క_CFunc{flp, funcp});
                cstmtp->కూడు(", " + std::గమ్యం_string(func_అంకె) + ", vlSelf);");
            }
        } లేకపోతే {
            // Sub functions
            funcp->argTypes(bufArg);
            // Setup base references. Note in rare occasions we can end up with an empty trace
            // sub function, hence the VL_ATTR_UNUSED attributes.
            ఒకవేళ (traceType != VTraceType::CHANGE) {
                // Full dump sub function
                funcp->కూడు_Stmtsp(కొత్త AstCStmt{flp,  //
                                              string("uint32_t* const oldp VL_ATTR_UNUSED = "
                                                     "bufp->oldp(vlSymsp->__Vm_baseCode")
                                                  + (declp ? " + offset" : "") + ");\n"});
            } లేకపోతే {
                // Change dump sub function
                funcp->కూడు_Stmtsp(కొత్త AstCStmt{flp,  //
                                              "uint32_t* const oldp VL_ATTR_UNUSED = "
                                              "bufp->oldp(vlSymsp->__Vm_baseCode + "
                                                  + (declp ? " offset" : cvt_గమ్యం_Str(పునాది_సంకేతం))
                                                  + ");\n"});
            }
            ఒకవేళ (!declp) {
                // Add call to top function
                AstCCall* స్థిరరాశి callp = కొత్త AstCCall{funcp->fileline(), funcp};
                callp->dtype_సమితి_Void();
                callp->argTypes("bufp");
                శిఖరం_Funcp->కూడు_Stmtsp(callp->makeStmt());
            }
        }
        //పూర్తి
        UINFO(5, "  newCFunc " << funcp);
        తిరిగి funcp;
    }

    AstCFunc* newCDtypeSubFunc(VTraceType traceType, స్థిరరాశి AstTraceDecl* స్థిరరాశి declp,
                               AstCFunc* parentp) {
        AstCFunc* స్థిరరాశి funcp = newCFunc(traceType, శూన్యసూచిక, 0, 0, declp, నిజం);
        File_పంక్తి* స్థిరరాశి flp = m_శిఖరం_Scopep->fileline();
        AstCCall* స్థిరరాశి callp = కొత్త AstCCall{flp, funcp};
        callp->dtype_సమితి_Void();
        callp->argTypes("bufp, offset, __VdtypeVar");
        parentp->కూడు_Stmtsp(callp->makeStmt());
        తిరిగి funcp;
    }

    AstCFunc* createConstDtypeTraceFunctions(స్థిరరాశి AstTraceDecl* declp) {
        స్థిరరాశి Ast_కణుపు_DType* స్థిరరాశి dtypep = declp->dtypep()->skipRefp();
        స్వయంచాలక pair = m_dtypeConstFuncs.emplace(dtypep, శూన్యసూచిక);
        ఒకవేళ (pair.రెండవ) {
            File_పంక్తి* స్థిరరాశి flp = declp->fileline();
            AstCFunc* స్థిరరాశి funcp = newCFunc(VTraceType::CONSTANT, శూన్యసూచిక, 0, 0, declp);

            తార్కిక మొదటి = నిజం;
            కొరకు (Ast_కణుపు* callStmtp = declp->dtypeCallp()->funcp()->stmtsp(); callStmtp;
                 callStmtp = callStmtp->nextp()) {
                స్థిరరాశి AstCCall* స్థిరరాశి callp
                    = VN_CAST(VN_CAST(callStmtp, Stmt_సమాసం)->exprp(), CCall);
                తార్కిక onlyOne = మొదటి && !callStmtp->nextp();
                AstCFunc* స్థిరరాశి subFuncp
                    = onlyOne ? funcp : newCDtypeSubFunc(VTraceType::CONSTANT, declp, funcp);
                కొరకు (Ast_కణుపు* stmtp = callp->funcp()->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    ఒకవేళ (AstTraceDecl* స్థిరరాశి క్షేత్రం_Declp = VN_CAST(stmtp, TraceDecl)) {
                        AstTraceInc* స్థిరరాశి incp
                            = కొత్త AstTraceInc{flp, క్షేత్రం_Declp, VTraceType::CONSTANT};
                        subFuncp->కూడు_Stmtsp(incp);
                    }
                }
                మొదటి = అబద్ధం;
            }

            pair.మొదటి->రెండవ = funcp;
        }

        తిరిగి pair.మొదటి->రెండవ;
    }

    శూన్య createConstTraceFunctions(స్థిరరాశి TraceVec& traces) {
        స్థిరరాశి పూర్ణాంకం విభజించు_హద్దు = v3Global.opt.output_విభజించు_CTrace() ? v3Global.opt.output_విభజించు_CTrace()
                                                                : std::numeric_limits<పూర్ణాంకం>::max();

        AstCFunc* స్థిరరాశి శిఖరం_Funcp = newCFunc(VTraceType::CONSTANT, శూన్యసూచిక, 0);
        uint32_t sub_Func_అంకె = 0;
        AstCFunc* subFuncp = శూన్యసూచిక;
        పూర్ణాంకం subStmts = 0;
        std::సదిశ<స్థిరరాశి Trace_Trace_శీర్షం*> duplicates;
        కొరకు (స్వయంచాలక it = traces.cbegin(); it != traces.చివర(); ++it) {
            స్థిరరాశి Trace_Trace_శీర్షం* స్థిరరాశి vtxp = it->రెండవ;
            AstTraceDecl* స్థిరరాశి declp = vtxp->nodep();
            ఒకవేళ (vtxp->duplicatep()) {
                duplicates.నెట్టు_వెనుక(vtxp);
                కొనసాగు;
            }

            // This is a canonical trace node. Assign trace code (signal number).
            UASSERT_OBJ(!declp->సంకేతం_Assigned(), declp,
                        "Canonical node should not have code assigned yet");
            declp->సంకేతం_2(m_సంకేతం);
            స్థిరరాశి uint32_t సంకేతం_Inc = declp->సంకేతం_Inc();
            m_సంకేతం += సంకేతం_Inc;
            ++m_statUniqSigs;

            // If this is a const signal, add the AstTraceInc
            స్థిరరాశి Act_సంకేతం_సమితి& act_సమితి = it->మొదటి;
            ఒకవేళ (act_సమితి.లెక్క(Trace_Activity_శీర్షం::ACTIVITY_NEVER)) {
                // Crate new sub function if required
                ఒకవేళ (!subFuncp || subStmts > విభజించు_హద్దు) {
                    subStmts = 0;
                    subFuncp = newCFunc(VTraceType::CONSTANT, శిఖరం_Funcp, sub_Func_అంకె);
                    ++sub_Func_అంకె;
                }
                File_పంక్తి* స్థిరరాశి flp = declp->fileline();
                ఒకవేళ (declp->dtypeCallp()) {
                    AstCFunc* స్థిరరాశి funcp = createConstDtypeTraceFunctions(declp);
                    Ast_కణుపు_సమాసం* argsp = శూన్యసూచిక;
                    argsp = Ast_కణుపు::కూడు_తదుపరి(argsp, declp->valuep()->clone_వృక్షం(అబద్ధం));
                    AstCCall* స్థిరరాశి callp = కొత్త AstCCall{flp, funcp, argsp};
                    callp->dtype_సమితి_Void();
                    callp->argTypes(callp->argTypes() + "bufp, " + std::గమ్యం_string(declp->సంకేతం_2()));
                    subFuncp->కూడు_Stmtsp(callp->makeStmt());

                    subStmts += 1;
                } లేకపోతే {
                    AstTraceInc* స్థిరరాశి incp = కొత్త AstTraceInc{flp, declp, VTraceType::CONSTANT};
                    subFuncp->కూడు_Stmtsp(incp);
                    subStmts += incp->కణుపు_లెక్క();
                }
            }
        }

        // Assign codes to duplicate nodes from their canonicals
        కొరకు (స్థిరరాశి Trace_Trace_శీర్షం* స్థిరరాశి vtxp : duplicates) {
            AstTraceDecl* స్థిరరాశి declp = vtxp->nodep();
            స్థిరరాశి Trace_Trace_శీర్షం* స్థిరరాశి canonVtxp = vtxp->duplicatep();
            స్థిరరాశి AstTraceDecl* స్థిరరాశి canonDeclp = canonVtxp->nodep();
            UASSERT_OBJ(!canonVtxp->duplicatep(), canonDeclp, "Canonical node is a duplicate");
            UASSERT_OBJ(canonDeclp->సంకేతం_Assigned(), canonDeclp,
                        "Canonical node should have code assigned already");
            declp->సంకేతం_2(canonDeclp->సంకేతం_2() + vtxp->dtype_Alias_స్థానభ్రంశం());
        }
    }

    DtypeFuncs createNonConstDtypeTraceFunctions(స్థిరరాశి AstTraceDecl* declp) {
        Ast_కణుపు_DType* dtypep = declp->dtypep()->skipRefp();
        స్వయంచాలక pair = m_dtypeNonConstFuncs.emplace(dtypep, DtypeFuncs{});
        ఒకవేళ (pair.రెండవ) {
            File_పంక్తి* స్థిరరాశి flp = declp->fileline();
            AstCFunc* స్థిరరాశి నిండిన_Funcp = newCFunc(VTraceType::నిండిన, శూన్యసూచిక, 0, 0, declp);
            AstCFunc* స్థిరరాశి chgFuncp = newCFunc(VTraceType::CHANGE, శూన్యసూచిక, 0, 0, declp);

            తార్కిక మొదటి = నిజం;
            కొరకు (Ast_కణుపు* callStmtp = declp->dtypeCallp()->funcp()->stmtsp(); callStmtp;
                 callStmtp = callStmtp->nextp()) {
                స్థిరరాశి AstCCall* స్థిరరాశి callp
                    = VN_CAST(VN_CAST(callStmtp, Stmt_సమాసం)->exprp(), CCall);
                తార్కిక onlyOne = మొదటి && !callStmtp->nextp();
                AstCFunc* స్థిరరాశి నిండిన_Sub_Funcp
                    = onlyOne ? నిండిన_Funcp : newCDtypeSubFunc(VTraceType::నిండిన, declp, నిండిన_Funcp);
                AstCFunc* స్థిరరాశి chgSubFuncp
                    = onlyOne ? chgFuncp : newCDtypeSubFunc(VTraceType::CHANGE, declp, chgFuncp);
                కొరకు (Ast_కణుపు* stmtp = callp->funcp()->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    ఒకవేళ (AstTraceDecl* స్థిరరాశి క్షేత్రం_Declp = VN_CAST(stmtp, TraceDecl)) {
                        AstTraceInc* స్థిరరాశి incFullp
                            = కొత్త AstTraceInc{flp, క్షేత్రం_Declp, VTraceType::నిండిన};
                        నిండిన_Sub_Funcp->కూడు_Stmtsp(incFullp);
                        AstTraceInc* స్థిరరాశి incChgp
                            = కొత్త AstTraceInc{flp, క్షేత్రం_Declp, VTraceType::CHANGE};
                        chgSubFuncp->కూడు_Stmtsp(incChgp);
                    }
                }
                మొదటి = అబద్ధం;
            }

            pair.మొదటి->రెండవ = {.నిండిన_Funcp = నిండిన_Funcp, .chgFuncp = chgFuncp};
        }

        తిరిగి pair.మొదటి->రెండవ;
    }

    శూన్య createNonConstTraceFunctions(స్థిరరాశి TraceVec& traces, uint32_t nAllCodes,
                                      uint32_t parallelism) {
        స్థిరరాశి పూర్ణాంకం విభజించు_హద్దు = v3Global.opt.output_విభజించు_CTrace() ? v3Global.opt.output_విభజించు_CTrace()
                                                                : std::numeric_limits<పూర్ణాంకం>::max();

        // pre-incremented, so starts at 0
        uint32_t శిఖరం_Func_అంకె = std::numeric_limits<uint32_t>::max();
        TraceVec::const_iterator it = traces.begin();
        అయితే (it != traces.చివర()) {
            AstCFunc* శిఖరం_Ful_Funcp = శూన్యసూచిక;
            AstCFunc* శిఖరం_Chg_Funcp = శూన్యసూచిక;
            AstCFunc* subFulFuncp = శూన్యసూచిక;
            AstCFunc* subChgFuncp = శూన్యసూచిక;
            uint32_t sub_Func_అంకె = 0;
            పూర్ణాంకం subStmts = 0;
            స్థిరరాశి uint32_t maxCodes = std::max((nAllCodes + parallelism - 1) / parallelism, 1U);
            uint32_t nCodes = 0;
            స్థిరరాశి Act_సంకేతం_సమితి* prev_Act_సమితి = శూన్యసూచిక;
            AstIf* ifp = శూన్యసూచిక;
            uint32_t పునాది_సంకేతం = 0;
            కొరకు (; nCodes < maxCodes && it != traces.చివర(); ++it) {
                స్థిరరాశి Act_సంకేతం_సమితి& act_సమితి = it->మొదటి;
                // Traced value never changes, no need to add it
                ఒకవేళ (act_సమితి.లెక్క(Trace_Activity_శీర్షం::ACTIVITY_NEVER)) కొనసాగు;

                స్థిరరాశి Trace_Trace_శీర్షం* స్థిరరాశి vtxp = it->రెండవ;
                AstTraceDecl* స్థిరరాశి declp = vtxp->nodep();

                // This is a duplicate decl, no need to add it, but must set the
                // function index to the same as the canonical node.
                ఒకవేళ (స్థిరరాశి Trace_Trace_శీర్షం* స్థిరరాశి canonVtxp = vtxp->duplicatep()) {
                    declp->fidx(canonVtxp->nodep()->fidx());
                    కొనసాగు;
                }

                // Create top function if not yet created
                ఒకవేళ (!శిఖరం_Ful_Funcp) {
                    ++శిఖరం_Func_అంకె;
                    శిఖరం_Ful_Funcp = newCFunc(VTraceType::నిండిన, శూన్యసూచిక, శిఖరం_Func_అంకె);
                    శిఖరం_Chg_Funcp = newCFunc(VTraceType::CHANGE, శూన్యసూచిక, శిఖరం_Func_అంకె);
                }

                // Create new sub function if required
                ఒకవేళ (!subFulFuncp || subStmts > విభజించు_హద్దు) {
                    పునాది_సంకేతం = declp->సంకేతం_2();
                    subStmts = 0;
                    subFulFuncp = newCFunc(VTraceType::నిండిన, శిఖరం_Ful_Funcp, sub_Func_అంకె, పునాది_సంకేతం);
                    subChgFuncp = newCFunc(VTraceType::CHANGE, శిఖరం_Chg_Funcp, sub_Func_అంకె, పునాది_సంకేతం);
                    ++sub_Func_అంకె;
                    prev_Act_సమితి = శూన్యసూచిక;
                    ifp = శూన్యసూచిక;
                }

                // If required, create the conditional node checking the activity flags
                ఒకవేళ (!prev_Act_సమితి || act_సమితి != *prev_Act_సమితి) {
                    File_పంక్తి* స్థిరరాశి flp = m_శిఖరం_Scopep->fileline();
                    స్థిరరాశి తార్కిక always = act_సమితి.లెక్క(Trace_Activity_శీర్షం::ACTIVITY_ALWAYS) != 0;
                    Ast_కణుపు_సమాసం* condp = శూన్యసూచిక;
                    ఒకవేళ (always) {
                        condp = కొత్త AstConst{flp, 1};  // Always true, will be folded later
                    } లేకపోతే {
                        కొరకు (స్థిరరాశి uint32_t act_సంకేతం : act_సమితి) {
                            Ast_కణుపు_సమాసం* స్థిరరాశి selp = selectActivity(flp, act_సంకేతం, VAccess::READ);
                            condp = condp ? కొత్త AstOr{flp, condp, selp} : selp;
                        }
                    }
                    ifp = కొత్త AstIf{flp, condp};
                    ఒకవేళ (!always) ifp->branchPred(VBranchPred::BP_UNLIKELY);
                    subChgFuncp->కూడు_Stmtsp(ifp);
                    subStmts += ifp->కణుపు_లెక్క();
                    prev_Act_సమితి = &act_సమితి;
                }

                // Add TraceInc nodes
                File_పంక్తి* స్థిరరాశి flp = declp->fileline();
                ఒకవేళ (declp->dtypeCallp()) {
                    DtypeFuncs funcs = createNonConstDtypeTraceFunctions(declp);
                    Ast_కణుపు_సమాసం* argsp = శూన్యసూచిక;
                    argsp = Ast_కణుపు::కూడు_తదుపరి(argsp, declp->valuep()->clone_వృక్షం(అబద్ధం));
                    AstCCall* స్థిరరాశి callFullp = కొత్త AstCCall{flp, funcs.నిండిన_Funcp, argsp};
                    callFullp->dtype_సమితి_Void();
                    callFullp->argTypes(callFullp->argTypes() + "bufp, "
                                        + std::గమ్యం_string(declp->సంకేతం_2()));
                    subFulFuncp->కూడు_Stmtsp(callFullp->makeStmt());
                    argsp = శూన్యసూచిక;
                    argsp = Ast_కణుపు::కూడు_తదుపరి(argsp, declp->valuep()->clone_వృక్షం(అబద్ధం));
                    AstCCall* స్థిరరాశి callChgp = కొత్త AstCCall{flp, funcs.chgFuncp, argsp};
                    callChgp->dtype_సమితి_Void();
                    callChgp->argTypes(callChgp->argTypes() + "bufp, "
                                       + std::గమ్యం_string(declp->సంకేతం_2()));
                    ifp->కూడు_Thensp(callChgp->makeStmt());

                    subStmts += 2;
                } లేకపోతే {
                    AstTraceInc* స్థిరరాశి incFulp = కొత్త AstTraceInc{flp, declp, VTraceType::నిండిన};
                    subFulFuncp->కూడు_Stmtsp(incFulp);
                    AstTraceInc* స్థిరరాశి incChgp
                        = కొత్త AstTraceInc{flp, declp, VTraceType::CHANGE, పునాది_సంకేతం};
                    ifp->కూడు_Thensp(incChgp);

                    // Set the function index of the decl
                    declp->fidx(శిఖరం_Func_అంకె);

                    // Track splitting due to size
                    UASSERT_OBJ(incFulp->కణుపు_లెక్క() == incChgp->కణుపు_లెక్క(), declp,
                                "Should have equal cost");
                    స్థిరరాశి VNumRange range = declp->arrayRange();
                    ఒకవేళ (range.ranged()) {
                        // 2x because each element is a TraceInc and a VarRef
                        subStmts += range.elements() * 2;
                    } లేకపోతే {
                        subStmts += incChgp->కణుపు_లెక్క();
                    }
                }

                // Track partitioning
                nCodes += declp->సంకేతం_Inc();
            }
        }
    }

    శూన్య createCleanupFunction() {
        File_పంక్తి* స్థిరరాశి fl = m_శిఖరం_Scopep->fileline();
        AstCFunc* స్థిరరాశి cleanupFuncp = కొత్త AstCFunc{fl, "trace_cleanup", m_శిఖరం_Scopep};
        cleanupFuncp->argTypes("void* voidSelf, " + v3Global.opt.trace_Class_పునాది()
                               + "* /*unused*/");
        cleanupFuncp->isTrace(నిజం);
        cleanupFuncp->slow(అబద్ధం);
        cleanupFuncp->isStatic(నిజం);
        cleanupFuncp->isLoose(నిజం);
        m_శిఖరం_Scopep->కూడు_Blocksp(cleanupFuncp);
        cleanupFuncp->కూడు_Stmtsp(కొత్త AstCStmt{fl, EmitCUtil::void_Self_కేటాయించు(m_శిఖరం_Modp)});
        cleanupFuncp->కూడు_Stmtsp(కొత్త AstCStmt{fl, EmitCUtil::sym_Class_కేటాయించు()});

        // Register it
        {
            AstCStmt* స్థిరరాశి cstmtp = కొత్త AstCStmt{fl};
            m_regFuncp->కూడు_Stmtsp(cstmtp);
            cstmtp->కూడు("tracep->addCleanupCb(");
            cstmtp->కూడు(కొత్త Ast_Addr_యొక్క_CFunc{fl, cleanupFuncp});
            cstmtp->కూడు(", vlSelf);");
        }

        // Clear global activity flag
        cleanupFuncp->కూడు_Stmtsp(
            కొత్త AstCStmt{m_శిఖరం_Scopep->fileline(), "vlSymsp->__Vm_activity = false;"s});

        // Clear fine grained activity flags
        కొరకు (uint32_t i = 0; i < m_activity_సంఖ్య; ++i) {
            Ast_కణుపు* స్థిరరాశి clrp = కొత్త Ast_కేటాయించు{fl, selectActivity(fl, i, VAccess::WRITE),
                                                కొత్త AstConst{fl, AstConst::బిట్టు_False{}}};
            cleanupFuncp->కూడు_Stmtsp(clrp);
        }
    }

    శూన్య createTraceFunctions() {
        // Detect and remove duplicate values
        detectDuplicates();

        లేఖాచిత్రం_Dtype_Prune();
        విభజించు_Trace_Decl_Funcs();
        m_లేఖాచిత్రం.remove_Redundant_అంచులు_Max(&V3_లేఖాచిత్రం_అంచు::followAlwaysTrue);

        // Simplify & optimize the graph
        ఒకవేళ (dump_లేఖాచిత్రం_Level() >= 6) m_లేఖాచిత్రం.dumpDotFilePrefixed("trace_pre");
        లేఖాచిత్రం_Simplify(నిజం);
        ఒకవేళ (dump_లేఖాచిత్రం_Level() >= 6) m_లేఖాచిత్రం.dumpDotFilePrefixed("trace_simplified");
        లేఖాచిత్రం_Optimize();
        ఒకవేళ (dump_లేఖాచిత్రం_Level() >= 6) m_లేఖాచిత్రం.dumpDotFilePrefixed("trace_optimized");

        // Create the fine grained activity flags
        createActivityFlags();

        // Form a sorted list of the traces we are interested in
        TraceVec traces;  // The sorted traces
        // We will split functions such that each have to dump roughly the same amount of data
        // for this we need to keep tack of the number of codes used by the trace functions.
        uint32_t nNonConstCodes = 0;
        క్రమీకరించు_Traces(traces, nNonConstCodes);

        // Our keys are now sorted to have same activity number adjacent, then
        // by trace order. (Better would be execution order for cache
        // efficiency....) Last are constants and non-changers, as then the
        // last value vector is more compact

        // Create the trace registration function
        m_regFuncp = కొత్త AstCFunc{m_శిఖరం_Scopep->fileline(), "trace_register", m_శిఖరం_Scopep};
        m_regFuncp->argTypes(v3Global.opt.trace_Class_పునాది() + "* tracep");
        m_regFuncp->నమోదు_బిందువు(నిజం);
        m_regFuncp->isTrace(నిజం);
        m_regFuncp->slow(నిజం);
        m_regFuncp->isStatic(అబద్ధం);
        m_regFuncp->isLoose(నిజం);
        m_శిఖరం_Scopep->కూడు_Blocksp(m_regFuncp);

        // Create the const dump functions. Also allocates trace codes.
        createConstTraceFunctions(traces);

        // Create the full and incremental dump functions
        createNonConstTraceFunctions(traces, nNonConstCodes, m_parallelism);

        // Root-traced libraries alias wrapper IOs onto the existing top-module codes.
        ఒకవేళ (!v3Global.opt.libCreate().ఖాళీలు()) {
            std::సదిశ<AstTraceDecl*> మూలం_Declps;
            std::సదిశ<AstTraceDecl*> శిఖరం_Declps;
            TraceInitDeclCollector మూలం_Collector{మూలం_Declps};
            TraceInitDeclCollector శిఖరం_Collector{శిఖరం_Declps};
            కొరకు (Ast_కణుపు* blockp = m_శిఖరం_Scopep->blocksp(); blockp; blockp = blockp->nextp()) {
                AstCFunc* స్థిరరాశి funcp = VN_CAST(blockp, CFunc);
                ఒకవేళ (funcp && VString::startsWith(funcp->name(), "trace_init_leaf_root__")) {
                    మూలం_Collector.collect(funcp);
                } లేకపోతే ఒకవేళ (funcp && VString::startsWith(funcp->name(), "trace_init_leaf_top__")) {
                    శిఖరం_Collector.collect(funcp);
                }
            }

            std::సదిశ<తార్కిక> వాడిన(శిఖరం_Declps.కొలత(), అబద్ధం);
            కొరకు (AstTraceDecl* స్థిరరాశి మూలం_Declp : మూలం_Declps) {
                కొరకు (కొలత_t i = 0; i < శిఖరం_Declps.కొలత(); ++i) {
                    ఒకవేళ (వాడిన[i] || !same_మూలం_Init_Alias(మూలం_Declp, శిఖరం_Declps[i])) కొనసాగు;
                    మూలం_Declp->సంకేతం_2(శిఖరం_Declps[i]->సంకేతం_2());
                    వాడిన[i] = నిజం;
                    ఆపు;
                }
            }
        }

        // Remove refs to traced values from TraceDecl nodes, these have now moved under
        // TraceInc
        కొరకు (స్థిరరాశి స్వయంచాలక& i : traces) {
            Ast_కణుపు* స్థిరరాశి valuep = i.రెండవ->nodep()->valuep();
            VL_DO_DANGLING(valuep->unlink_Fr_వెనుక()->delete_వృక్షం(), valuep);
        }

        // Create the trace cleanup function clearing the activity flags
        createCleanupFunction();
    }

    Trace_CFunc_శీర్షం* getCFuncVertexp(AstCFunc* nodep) {
        V3_లేఖాచిత్రం_శీర్షం* స్థిరరాశి vtxp = nodep->user1u().గమ్యం_లేఖాచిత్రం_శీర్షం();
        Trace_CFunc_శీర్షం* vertexp = vtxp ? vtxp->cast<Trace_CFunc_శీర్షం>() : శూన్యసూచిక;
        ఒకవేళ (!vertexp) {
            vertexp = కొత్త Trace_CFunc_శీర్షం{&m_లేఖాచిత్రం, nodep};
            nodep->user1p(vertexp);
        }
        తిరిగి vertexp;
    }
    Trace_Activity_శీర్షం* getActivityVertexp(Ast_కణుపు* nodep, తార్కిక slow) {
        V3_లేఖాచిత్రం_శీర్షం* స్థిరరాశి vtxp = nodep->user3u().గమ్యం_లేఖాచిత్రం_శీర్షం();
        Trace_Activity_శీర్షం* vertexp = vtxp ? vtxp->cast<Trace_Activity_శీర్షం>() : శూన్యసూచిక;
        ఒకవేళ (!vertexp) {
            vertexp = కొత్త Trace_Activity_శీర్షం{&m_లేఖాచిత్రం, nodep, slow};
            nodep->user3p(vertexp);
        }
        vertexp->slow(slow);
        తిరిగి vertexp;
    }

    // VISITORS
    శూన్య సందర్శించు(AstNetlist* nodep) override {
        // Add vertexes for all TraceDecl, and edges from VARs each trace looks at
        m_finding = అబద్ధం;
        iterateChildren(nodep);

        // Add vertexes for all CFUNCs, and edges to VARs the func sets
        m_finding = నిజం;
        iterateChildren(nodep);

        // Create the trace functions and insert them into the tree
        createTraceFunctions();

        // Save number of trace codes used
        nodep->nTraceCodes(m_సంకేతం);
    }
    శూన్య సందర్శించు(Ast_కణుపు_Module* nodep) override {
        ఒకవేళ (nodep->is_శిఖరం()) m_శిఖరం_Modp = nodep;
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(AstVarScope* nodep) override {
        ఒకవేళ (!m_finding) {
            ఒకవేళ (స్థిరరాశి AstIface* స్థిరరాశి ifacep = nodep->varp()->sensIfacep()) {
                m_ifaceMemberVscps[{ifacep, nodep->varp()->name()}].నెట్టు_వెనుక(nodep);
            }
        }
    }
    శూన్య సందర్శించు(Ast_Stmt_సమాసం* nodep) override {
        ఒకవేళ (!m_finding && !nodep->user2()) {
            ఒకవేళ (AstCCall* స్థిరరాశి callp = VN_CAST(nodep->exprp(), CCall)) {
                UINFO(8, "   CCALL " << callp);
                // See if there are other calls in same statement list;
                // If so, all funcs might share the same activity code
                Trace_Activity_శీర్షం* స్థిరరాశి activityVtxp
                    = getActivityVertexp(nodep, callp->funcp()->slow());
                కొరకు (Ast_కణుపు* nextp = nodep; nextp; nextp = nextp->nextp()) {
                    ఒకవేళ (Ast_Stmt_సమాసం* స్థిరరాశి stmtp = VN_CAST(nextp, Stmt_సమాసం)) {
                        ఒకవేళ (AstCCall* స్థిరరాశి ccallp = VN_CAST(stmtp->exprp(), CCall)) {
                            stmtp->user2(నిజం);  // Processed
                            UINFO(8, "     SubCCALL " << ccallp);
                            V3_లేఖాచిత్రం_శీర్షం* స్థిరరాశి ccallFuncVtxp = getCFuncVertexp(ccallp->funcp());
                            activityVtxp->slow(ccallp->funcp()->slow());
                            కొత్త V3_లేఖాచిత్రం_అంచు{&m_లేఖాచిత్రం, activityVtxp, ccallFuncVtxp, 1};
                        }
                    }
                }
            }
        }
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(AstCFunc* nodep) override {
        UINFO(8, "   CFUNC " << nodep);
        V3_లేఖాచిత్రం_శీర్షం* స్థిరరాశి funcVtxp = getCFuncVertexp(nodep);
        ఒకవేళ (!m_finding) {  // If public, we need a unique activity code to allow for sets
                           // directly in this func
            ఒకవేళ (nodep->func_ప్రజా() || nodep->dpiExportImpl() || nodep->నమోదు_బిందువు()
                || nodep->isCoroutine()) {
                // Cannot treat a coroutine as slow, it may be resumed later
                స్థిరరాశి తార్కిక slow = nodep->slow() && !nodep->isCoroutine();
                Trace_Activity_శీర్షం* స్థిరరాశి activityVtxp = getActivityVertexp(nodep, slow);
                కొత్త V3_లేఖాచిత్రం_అంచు{&m_లేఖాచిత్రం, activityVtxp, funcVtxp, 1};
            }
        }
        VL_RESTORER(m_cfuncp);
        m_cfuncp = nodep;
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(AstTraceDecl* nodep) override {
        UINFO(8, "   TRACE " << nodep);
        ఒకవేళ (!m_finding && !nodep->లోపల_Dtype_Func()) {
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
            ఒకవేళ (m_cfuncp && VString::startsWith(m_cfuncp->name(), "trace_init_leaf_root__")) {
                తిరిగి;
            }
            V3_లేఖాచిత్రం_శీర్షం* స్థిరరాశి vertexp = కొత్త Trace_Trace_శీర్షం{&m_లేఖాచిత్రం, nodep};
            nodep->user1p(vertexp);

            UASSERT_OBJ(m_cfuncp, nodep, "Trace not under func");
            m_cfuncp->user2(నిజం);
            VL_RESTORER(m_tracep);
            m_tracep = nodep;
            iterateChildren(nodep);
        }
    }
    శూన్య సందర్శించు(AstVarRef* nodep) override {
        UASSERT_OBJ(nodep->varScopep(), nodep, "No var scope?");
        AstVarScope* స్థిరరాశి varscopep = nodep->varScopep();
        V3_లేఖాచిత్రం_శీర్షం* varVtxp = varscopep->user1u().గమ్యం_లేఖాచిత్రం_శీర్షం();
        ఒకవేళ (m_tracep) {
            UASSERT_OBJ(nodep->access().isReadOnly(), nodep, "Lvalue in trace?  Should be const.");
            ఒకవేళ (!varVtxp) {
                varVtxp = కొత్త Trace_Var_శీర్షం{&m_లేఖాచిత్రం, nodep->varScopep()};
                nodep->varScopep()->user1p(varVtxp);
            }
            V3_లేఖాచిత్రం_శీర్షం* స్థిరరాశి traceVtxp = m_tracep->user1u().గమ్యం_లేఖాచిత్రం_శీర్షం();
            కొత్త V3_లేఖాచిత్రం_అంచు{&m_లేఖాచిత్రం, varVtxp, traceVtxp, 1};
            ఒకవేళ (nodep->varp()->isPrimaryInish()  // Always need to trace primary inputs
                || nodep->varp()->is_Sig_ప్రజా()) {  // Or ones user can change
                కొత్త V3_లేఖాచిత్రం_అంచు{&m_లేఖాచిత్రం, m_alwaysVtxp, traceVtxp, 1};
            }
            ఒకవేళ (m_tracep->dtypeCallp()) varscopep->user2p(m_tracep);
        } లేకపోతే ఒకవేళ (m_cfuncp && m_finding && nodep->access().isWriteOrRW()) {
            V3_లేఖాచిత్రం_శీర్షం* స్థిరరాశి funcVtxp = getCFuncVertexp(m_cfuncp);
            ఒకవేళ (varVtxp) {  // else we're not tracing this signal
                కొత్త V3_లేఖాచిత్రం_అంచు{&m_లేఖాచిత్రం, funcVtxp, varVtxp, 1};
                AstTraceDecl* స్థిరరాశి declp = VN_AS(varscopep->user2p(), TraceDecl);
                ఒకవేళ (declp) {
                    V3_లేఖాచిత్రం_శీర్షం* స్థిరరాశి cFuncVtxp = getCFuncVertexp(m_cfuncp);
                    కొరకు (స్థిరరాశి V3_లేఖాచిత్రం_అంచు& అంచు : cFuncVtxp->లోపల_అంచులు()) {
                        V3_లేఖాచిత్రం_శీర్షం* స్థిరరాశి activityp = అంచు.fromp();
                        ఒకవేళ (!declp->user3p()) {
                            declp->user3p(activityp);
                        } లేకపోతే ఒకవేళ (declp->user3u().గమ్యం_లేఖాచిత్రం_శీర్షం() != activityp) {
                            declp->user2(నిజం);
                        }
                    }
                }
            }
        }
    }
    శూన్య సందర్శించు(AstMemberSel* nodep) override {
        ఒకవేళ (m_cfuncp && m_finding && nodep->access().isWriteOrRW()) {
            AstIfaceRefDType* స్థిరరాశి dtypep
                = VN_CAST(nodep->fromp()->dtypep()->skipRefp(), IfaceRefDType);
            ఒకవేళ (dtypep && dtypep->isVirtual()) {
                స్థిరరాశి స్వయంచాలక it = m_ifaceMemberVscps.కనుగొను({dtypep->ifacep(), nodep->varp()->name()});
                ఒకవేళ (it != m_ifaceMemberVscps.చివర()) {
                    V3_లేఖాచిత్రం_శీర్షం* స్థిరరాశి funcVtxp = getCFuncVertexp(m_cfuncp);
                    కొరకు (AstVarScope* స్థిరరాశి vscp : it->రెండవ) {
                        V3_లేఖాచిత్రం_శీర్షం* varVtxp = vscp->user1u().గమ్యం_లేఖాచిత్రం_శీర్షం();
                        ఒకవేళ (!varVtxp) {
                            varVtxp = కొత్త Trace_Var_శీర్షం{&m_లేఖాచిత్రం, vscp};
                            vscp->user1p(varVtxp);
                        }
                        కొత్త V3_లేఖాచిత్రం_అంచు{&m_లేఖాచిత్రం, funcVtxp, varVtxp, 1};
                    }
                }
            }
        }
        iterateChildren(nodep);
    }
    //--------------------
    శూన్య సందర్శించు(Ast_కణుపు* nodep) override { iterateChildren(nodep); }

బహిరంగ:
    // CONSTRUCTORS
    స్పష్టమైన TraceVisitor(AstNetlist* nodep)
        : m_alwaysVtxp{కొత్త Trace_Activity_శీర్షం{&m_లేఖాచిత్రం, Trace_Activity_శీర్షం::ACTIVITY_ALWAYS}} {
        nodep->user2_Clear_వృక్షం();  // TraceDecl multiple activities flag
        nodep->user3_Clear_వృక్షం();  // TraceDecl TraceActivityVertex (assumes we start at nullptr)
        iterate(nodep);
        nodep->foreach([](AstTraceDecl* స్థిరరాశి declp) {
            ఒకవేళ (declp->లోపల_Dtype_Func()) {
                declp->valuep()->unlink_Fr_వెనుక()->delete_వృక్షం();
                declp->valuep(శూన్యసూచిక);
            }
        });
    }
    ~TraceVisitor() override {
        V3Stats::కూడు_Stat("Tracing, Activity setters", m_statSetters);
        V3Stats::కూడు_Stat("Tracing, Activity slow blocks", m_statSettersSlow);
        V3Stats::కూడు_Stat("Tracing, Unique trace codes", m_సంకేతం);
        V3Stats::కూడు_Stat("Tracing, Unique traced signals", m_statUniqSigs);
    }
};

//######################################################################
// Trace class functions

శూన్య V3Trace::traceAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { TraceVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_వృక్షం("trace", 0, dump_వృక్షం_Either_Level() >= 3);
}
