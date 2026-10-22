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

#अन्तर्भाव "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#अन्तर्भाव "V3Trace.h"

#अन्तर्भाव "V3Ast.h"
#अन्तर्भाव "V3DupFinder.h"
#अन्तर्भाव "V3EmitCBase.h"
#अन्तर्भाव "V3Graph.h"
#अन्तर्भाव "V3Stats.h"
#अन्तर्भाव "V3UniqueNames.h"

#अन्तर्भाव <limits>
#अन्तर्भाव <map>
#अन्तर्भाव <set>
#अन्तर्भाव <unordered_map>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Graph vertexes

वर्ग TraceActivityVertex final : सार्वजनिक V3GraphVertex {
    VL_RTTI_IMPL(TraceActivityVertex, V3GraphVertex)
    Ast_ग्रन्थिः* ध्रुव m_insertp;
    int32_t m_activity_संहिता;
    तार्किक m_slow;  // If always slow, we can use the same code
सार्वजनिक:
    गणना { ACTIVITY_NEVER = ((1UL << 31) - 1) };
    गणना { ACTIVITY_ALWAYS = ((1UL << 31) - 2) };
    गणना { ACTIVITY_SLOW = 0 };
    TraceActivityVertex(V3Graph* graphp, Ast_ग्रन्थिः* nodep, तार्किक slow)
        : V3GraphVertex{graphp}
        , m_insertp{nodep} {
        m_activity_संहिता = 0;
        m_slow = slow;
    }
    TraceActivityVertex(V3Graph* graphp, int32_t संहिता)
        : V3GraphVertex{graphp}
        , m_insertp{शून्यसूचकः} {
        m_activity_संहिता = संहिता;
        m_slow = असत्य;
    }
    ~TraceActivityVertex() override = स्वभाव;
    // ACCESSORS
    Ast_ग्रन्थिः* insertp() ध्रुव {
        UASSERT(m_insertp, "Null insertp; probably called on a special always/slow");
        प्रत्यागम m_insertp;
    }
    string name() ध्रुव override {
        यदि (activityAlways()) {
            प्रत्यागम "*ALWAYS*";
        } अन्यथा {
            प्रत्यागम std::string{slow() ? "*SLOW* " : ""} + insertp()->name();
        }
    }
    string dotColor() ध्रुव override { प्रत्यागम slow() ? "yellowGreen" : "green"; }
    int32_t activity_संहिता() ध्रुव { प्रत्यागम m_activity_संहिता; }
    तार्किक activityAlways() ध्रुव { प्रत्यागम activity_संहिता() == ACTIVITY_ALWAYS; }
    तार्किक activitySlow() ध्रुव { प्रत्यागम activity_संहिता() == ACTIVITY_SLOW; }
    शून्य activity_संहिता(int32_t संहिता) { m_activity_संहिता = संहिता; }
    तार्किक slow() ध्रुव { प्रत्यागम m_slow; }
    शून्य slow(तार्किक flag) {
        यदि (!flag) m_slow = असत्य;
    }
};

वर्ग TraceCFuncVertex final : सार्वजनिक V3GraphVertex {
    VL_RTTI_IMPL(TraceCFuncVertex, V3GraphVertex)
    AstCFunc* ध्रुव m_nodep;

सार्वजनिक:
    TraceCFuncVertex(V3Graph* graphp, AstCFunc* nodep)
        : V3GraphVertex{graphp}
        , m_nodep{nodep} {}
    ~TraceCFuncVertex() override = स्वभाव;
    // ACCESSORS
    AstCFunc* nodep() ध्रुव { प्रत्यागम m_nodep; }
    string name() ध्रुव override { प्रत्यागम nodep()->name(); }
    string dotColor() ध्रुव override { प्रत्यागम "yellow"; }
    File_पङ्क्तिः* fileline() ध्रुव override { प्रत्यागम nodep()->fileline(); }
};

वर्ग TraceTraceVertex final : सार्वजनिक V3GraphVertex {
    VL_RTTI_IMPL(TraceTraceVertex, V3GraphVertex)
    AstTraceDecl* ध्रुव m_nodep;  // TRACEINC this represents
    // nullptr, or other vertex with the real code() that duplicates this one
    TraceTraceVertex* m_duplicatep = शून्यसूचकः;
    // When aliasing to a dtype parent, offset of the member within the dtype
    uint32_t m_dtype_Alias_विस्थापनम् = 0;

सार्वजनिक:
    TraceTraceVertex(V3Graph* graphp, AstTraceDecl* nodep)
        : V3GraphVertex{graphp}
        , m_nodep{nodep} {}
    ~TraceTraceVertex() override = स्वभाव;
    // ACCESSORS
    AstTraceDecl* nodep() ध्रुव { प्रत्यागम m_nodep; }
    string name() ध्रुव override { प्रत्यागम nodep()->name(); }
    string dotColor() ध्रुव override { प्रत्यागम "red"; }
    File_पङ्क्तिः* fileline() ध्रुव override { प्रत्यागम nodep()->fileline(); }
    TraceTraceVertex* duplicatep() ध्रुव { प्रत्यागम m_duplicatep; }
    शून्य duplicatep(TraceTraceVertex* dupp) {
        UASSERT_OBJ(!duplicatep(), nodep(), "Assigning duplicatep() to already duplicated node");
        m_duplicatep = dupp;
    }
    शून्य redirectDuplicatep(TraceTraceVertex* dupp) { m_duplicatep = dupp; }
    uint32_t dtype_Alias_विस्थापनम्() ध्रुव { प्रत्यागम m_dtype_Alias_विस्थापनम्; }
    शून्य dtype_Alias_विस्थापनम्(uint32_t विस्थापनम्) { m_dtype_Alias_विस्थापनम् = विस्थापनम्; }
};

वर्ग TraceVarVertex final : सार्वजनिक V3GraphVertex {
    VL_RTTI_IMPL(TraceVarVertex, V3GraphVertex)
    AstVarScope* ध्रुव m_nodep;

सार्वजनिक:
    TraceVarVertex(V3Graph* graphp, AstVarScope* nodep)
        : V3GraphVertex{graphp}
        , m_nodep{nodep} {}
    ~TraceVarVertex() override = स्वभाव;
    // ACCESSORS
    AstVarScope* nodep() ध्रुव { प्रत्यागम m_nodep; }
    string name() ध्रुव override { प्रत्यागम nodep()->name(); }
    string dotColor() ध्रुव override { प्रत्यागम "skyblue"; }
    File_पङ्क्तिः* fileline() ध्रुव override { प्रत्यागम nodep()->fileline(); }
};

//######################################################################
// Trace state, as a visitor of each AstNode

वर्ग TraceVisitor final : सार्वजनिक VNVisitor {
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
    ध्रुव VNUser1_अन्तर्_Use m_inuser1;
    ध्रुव VNUser2_अन्तर्_Use m_inuser2;
    ध्रुव VNUser3_अन्तर्_Use m_inuser3;
    // VNUser4InUse     In V3Hasher via V3DupFinder

    // STATE
    Ast_ग्रन्थिः_Module* m_शिखरम्_Modp = शून्यसूचकः;  // Module to add variables to
    AstScope* ध्रुव m_शिखरम्_Scopep = v3Global.rootp()->शिखरम्_Scopep()->scopep();  // The top AstScope
    AstCFunc* m_cfuncp = शून्यसूचकः;  // C function adding to graph
    AstCFunc* m_regFuncp = शून्यसूचकः;  // Trace registration function
    AstCFunc* m_actAllFuncp = शून्यसूचकः;  // Set all activity function
    AstTraceDecl* m_tracep = शून्यसूचकः;  // Trace function adding to graph
    AstVarScope* m_activityVscp = शून्यसूचकः;  // Activity variable
    uint32_t m_activity_सङ्ख्या = 0;  // Count of fields in activity variable
    uint32_t m_संहिता = 0;  // Trace ident code# being assigned
    V3Graph m_graph;  // Var/CFunc tracking
    TraceActivityVertex* ध्रुव m_alwaysVtxp;  // "Always trace" vertex
    तार्किक m_finding = असत्य;  // Pass one of algorithm?
    संरचना DtypeFuncs final {
    सार्वजनिक:
        AstCFunc* पूर्णम्_Funcp = शून्यसूचकः;
        AstCFunc* chgFuncp = शून्यसूचकः;
    };
    std::unordered_map<ध्रुव Ast_ग्रन्थिः_DType*, DtypeFuncs>
        m_dtypeNonConstFuncs;  // Full / Chg funcs per type
    std::unordered_map<ध्रुव Ast_ग्रन्थिः_DType*, AstCFunc*> m_dtypeConstFuncs;  // Const func per type
    V3_Unique_नामानि m_dtype_नामानि{""};  // Unique type func names

    // Trace parallelism. Only VCD tracing can be parallelized at this time.
    ध्रुव uint32_t m_parallelism
        = v3Global.opt.useTraceParallel() ? static_cast<uint32_t>(v3Global.opt.तन्तवः()) : 1;

    VDouble0 m_statSetters;  // Statistic tracking
    VDouble0 m_statSettersSlow;  // Statistic tracking
    VDouble0 m_statUniqSigs;  // Statistic tracking

    // All activity numbers applying to a given trace
    उपयुज्य Act_संहिता_समुच्चयः = std::समुच्चयः<uint32_t>;
    // For activity set, what traces apply
    उपयुज्य TraceVec = std::multimap<Act_संहिता_समुच्चयः, TraceTraceVertex*>;
    // Candidate interface-member VarScopes keyed by (interface type, member name)
    std::map<std::pair<ध्रुव AstIface*, std::string>, std::सदिशम्<AstVarScope*>>
        m_ifaceMemberVscps;

    वर्ग TraceInitDeclCollector final : सार्वजनिक VNVisitor {
        std::सदिशम्<AstTraceDecl*>& m_declps;  // Output: trace declarations found so far
        std::समुच्चयः<ध्रुव AstCFunc*> m_आलोकितम्_Funcps;

        शून्य दर्शय(AstTraceDecl* nodep) override { m_declps.प्रक्षिप_पृष्ठभागः(nodep); }
        शून्य दर्शय(AstCCall* nodep) override {
            यदि (AstCFunc* ध्रुव funcp = nodep->funcp()) collect(funcp);
        }
        शून्य दर्शय(Ast_ग्रन्थिः* nodep) override { iterateChildren(nodep); }

    सार्वजनिक:
        स्पष्ट TraceInitDeclCollector(std::सदिशम्<AstTraceDecl*>& declps)
            : m_declps{declps} {}
        शून्य collect(AstCFunc* funcp) {
            यदि (funcp && m_आलोकितम्_Funcps.निवेशय(funcp).second) iterate(funcp);
        }
    };

    // METHODS

    स्थिर तार्किक same_मूलम्_Init_Alias(ध्रुव AstTraceDecl* मूलम्_Declp, ध्रुव AstTraceDecl* शिखरम्_Declp) {
        ध्रुव VNumRange& मूलम्_Bit_Range = मूलम्_Declp->bitRange();
        ध्रुव VNumRange& शिखरम्_Bit_Range = शिखरम्_Declp->bitRange();
        प्रत्यागम मूलम्_Declp->showname() == शिखरम्_Declp->showname()
               && मूलम्_Declp->declDirection() == शिखरम्_Declp->declDirection()
               && मूलम्_Declp->विस्तारः_Min() == शिखरम्_Declp->विस्तारः_Min()
               && मूलम्_Bit_Range.ranged() == शिखरम्_Bit_Range.ranged()
               && (!मूलम्_Bit_Range.ranged()
                   || (मूलम्_Bit_Range.वामतः() == शिखरम्_Bit_Range.वामतः()
                       && मूलम्_Bit_Range.दक्षिणतः() == शिखरम्_Bit_Range.दक्षिणतः()));
    }

    शून्य detectDuplicates() {
        UINFO(9, "Finding duplicates");
        // Note uses user4
        V3DupFinder dupFinder;  // Duplicate code detection

        // Compute member offsets within dtype instances incrementally.
        // For each dtype member TraceDecl, stores its code offset within the
        // dtype parent's code range.
        std::unordered_map<ध्रुव AstTraceDecl*, uint32_t> memberOffsets;
        std::unordered_map<ध्रुव AstTraceDecl*, uint32_t> running_विस्थापनम्;

        // For fixup pass: track dups redirected to dtype parents, so we can
        // fix them if the parent's user2 is later set during the loop.
        // Maps dup vertex -> original canonical member vertex.
        std::सदिशम्<std::pair<TraceTraceVertex*, TraceTraceVertex*>> dtypeParentRedirects;

        // Hash all of the traced values and find if there are any duplicates
        प्रति (V3GraphVertex& vtx : m_graph.vertices()) {
            यदि (TraceTraceVertex* ध्रुव vvertexp = vtx.cast<TraceTraceVertex>()) {
                AstTraceDecl* ध्रुव nodep = vvertexp->nodep();
                यदि (nodep->dtypeCallp()) अनुवर्त;

                यदि (nodep->dtypeDeclp()) {
                    uint32_t& विस्थापनम् = running_विस्थापनम्[nodep->dtypeDeclp()];
                    memberOffsets[nodep] = विस्थापनम्;
                    विस्थापनम् += nodep->संहिता_Inc();
                }

                UASSERT_OBJ(!vvertexp->duplicatep(), nodep, "Should not be a duplicate");
                ध्रुव स्वचालित dupit = dupFinder.findDuplicate(nodep);
                यदि (dupit == dupFinder.अन्तः_2()) {
                    dupFinder.निवेशय(nodep);
                } अन्यथा {
                    AstTraceDecl* ध्रुव dupDeclp = VN_AS(dupit->second, TraceDecl);
                    UASSERT_OBJ(dupDeclp, nodep, "Trace duplicate of wrong type");
                    TraceTraceVertex* ध्रुव dupvertexp
                        = dupDeclp->user1u().गन्तव्यम्_Graph_Vertex()->cast<TraceTraceVertex>();
                    UINFO(8, "  Orig " << dupDeclp << पङ्क्त्यन्तः);
                    UINFO(8, "   dup " << nodep << पङ्क्त्यन्तः);

                    यदि (dupDeclp->dtypeDeclp() && !dupDeclp->dtypeDeclp()->user2()) {
                        AstTraceDecl* ध्रुव dtypeParentp = dupDeclp->dtypeDeclp();
                        TraceTraceVertex* ध्रुव dtypeVtxp
                            = dtypeParentp->user1u().गन्तव्यम्_Graph_Vertex()->cast<TraceTraceVertex>();
                        ध्रुव स्वचालित it2 = memberOffsets.find(dupDeclp);
                        UASSERT_OBJ(it2 != memberOffsets.अन्तः_2(), dupDeclp,
                                    "Member offset not precomputed");
                        vvertexp->duplicatep(dtypeVtxp);
                        vvertexp->dtype_Alias_विस्थापनम्(it2->second);
                        dtypeParentRedirects.emplace_पृष्ठभागः(vvertexp, dupvertexp);
                    } अन्यथा {
                        vvertexp->duplicatep(dupvertexp);
                    }
                    यदि (nodep->dtypeDeclp()) nodep->dtypeDeclp()->user2(सत्य);
                }
            }
        }

        प्रति (ध्रुव स्वचालित& pair : dtypeParentRedirects) {
            TraceTraceVertex* ध्रुव dupVtxp = pair.first;
            TraceTraceVertex* ध्रुव memberVtxp = pair.second;
            ध्रुव AstTraceDecl* ध्रुव parentDeclp = dupVtxp->duplicatep()->nodep();
            यदि (parentDeclp->user2()) {
                dupVtxp->redirectDuplicatep(memberVtxp);
                dupVtxp->dtype_Alias_विस्थापनम्(0);
            }
        }

        यदि (dumpLevel() || debug() >= 9)
            dupFinder.dumpFile(v3Global.debugFilename("trace") + ".hash", असत्य);
    }

    शून्य graphDtypePrune() {
        प्रति (V3GraphVertex* ध्रुव vtxp : m_graph.vertices().unlinkable()) {
            यदि (TraceTraceVertex* ध्रुव vvertexp = vtxp->cast<TraceTraceVertex>()) {
                AstTraceDecl* ध्रुव declp = vvertexp->nodep();
                // This skips the dtype sub-func optimization if a var is affected by multiple
                // activities.  We really only need to do this for _chg funcs (and not decls,
                // _const and _full) but it's simpiler to do it all one way or the other.
                यदि (declp) {
                    यदि (declp->user2() || (declp->dtypeDeclp() && !declp->dtypeDeclp()->user2())) {
                        AstCCall* ध्रुव callp = declp->dtypeCallp();
                        यदि (callp) {
                            Ast_ग्रन्थिः* stmtexprp = callp->backp();
                            VL_DO_DANGLING(प्रक्षिप_Deletep(stmtexprp->unlink_Fr_पृष्ठभागः()), stmtexprp);
                        } अन्यथा {
                            तार्किक रिक्तानि_Scope;
                            कुरु {
                                रिक्तानि_Scope = असत्य;
                                Ast_ग्रन्थिः* ध्रुव declBackp = declp->backp();
                                Ast_ग्रन्थिः* ध्रुव declNextp = declp->nextp();
                                यदि (VN_IS(declBackp, Trace_प्रक्षिप_उपसर्गः)
                                    && VN_IS(declNextp, Trace_निष्कासय_उपसर्गः)) {
                                    VL_DO_DANGLING(प्रक्षिप_Deletep(declBackp->unlink_Fr_पृष्ठभागः()),
                                                   declBackp);
                                    VL_DO_DANGLING(प्रक्षिप_Deletep(declNextp->unlink_Fr_पृष्ठभागः()),
                                                   declNextp);
                                    रिक्तानि_Scope = सत्य;
                                }
                            } यावत् (रिक्तानि_Scope);
                        }
                        // Can't purge until we finish this pass
                        प्रक्षिप_Deletep(declp->unlink_Fr_पृष्ठभागः());  // declp used below
                        vvertexp->reroute_धाराः(&m_graph);
                        vvertexp->unlinkDelete(&m_graph);
                    }
                    declp->dtypeDeclp(शून्यसूचकः);
                }
            }
        }
    }

    शून्य विभज_Trace_Decl_Funcs() {
        ध्रुव पूर्णाङ्क विभज_सीमा = v3Global.opt.output_विभज_CTrace() ? v3Global.opt.output_विभज_CTrace()
                                                                : std::numeric_limits<पूर्णाङ्क>::max();
        यदि (विभज_सीमा == std::numeric_limits<पूर्णाङ्क>::max()) प्रत्यागम;

        प्रति (Ast_ग्रन्थिः* nodep = m_शिखरम्_Scopep->blocksp(); nodep; nodep = nodep->nextp()) {
            AstCFunc* ध्रुव funcp = VN_CAST(nodep, CFunc);
            यदि (!funcp || !funcp->user2()) अनुवर्त;

            ध्रुव string आधारः_Name = funcp->name();
            uint32_t sub_अङ्कः = 1;
            पूर्णाङ्क cur_आकार = 0;
            AstCFunc* curFuncp = funcp;
            std::सदिशम्<Ast_ग्रन्थिः_Stmt*> callStmts;

            स्वचालित startNewSubFunc = [&]() {
                File_पङ्क्तिः* ध्रुव flp = funcp->fileline();
                ध्रुव string newName = आधारः_Name + "_" + cvt_गन्तव्यम्_Str(sub_अङ्कः++);
                AstCFunc* ध्रुव newFuncp = नूतन AstCFunc{flp, newName, m_शिखरम्_Scopep};
                newFuncp->argTypes(v3Global.opt.trace_Class_आधारः() + "* tracep");
                newFuncp->isTrace(सत्य);
                newFuncp->isStatic(असत्य);
                newFuncp->isLoose(सत्य);
                newFuncp->slow(सत्य);
                newFuncp->dontCombine(सत्य);
                m_शिखरम्_Scopep->सङ्कलय_Blocksp(newFuncp);
                newFuncp->सङ्कलय_Stmtsp(नूतन AstCStmt{flp, "const int c = vlSymsp->__Vm_baseCode;"});
                AstCCall* ध्रुव callp = नूतन AstCCall{flp, newFuncp};
                callp->dtype_समुच्चयः_Void();
                callp->argTypes("tracep");
                callStmts.प्रक्षिप_पृष्ठभागः(callp->makeStmt());
                curFuncp = newFuncp;
                cur_आकार = 0;
            };

            प्रति (Ast_ग्रन्थिः *stmtp = funcp->stmtsp(), *nextp; stmtp; stmtp = nextp) {
                nextp = stmtp->nextp();
                cur_आकार += stmtp->ग्रन्थिः_गणनम्();

                यदि (curFuncp != funcp) {
                    stmtp->unlink_Fr_पृष्ठभागः();
                    curFuncp->सङ्कलय_Stmtsp(stmtp);
                }

                यदि (cur_आकार > विभज_सीमा && nextp) startNewSubFunc();
            }

            प्रति (Ast_ग्रन्थिः_Stmt* ध्रुव callStmtp : callStmts) { funcp->सङ्कलय_Stmtsp(callStmtp); }
        }
    }

    शून्य graphSimplify(तार्किक initial) {
        यदि (initial) {
            // Remove all variable nodes
            प्रति (V3GraphVertex* ध्रुव vtxp : m_graph.vertices().unlinkable()) {
                यदि (TraceVarVertex* ध्रुव vvertexp = vtxp->cast<TraceVarVertex>()) {
                    vvertexp->reroute_धाराः(&m_graph);
                    vvertexp->unlinkDelete(&m_graph);
                }
            }
            // Remove multiple variables connecting funcs to traces
            // We do this twice, as then we have fewer edges to multiply out in the below
            // expansion.
            m_graph.remove_Redundant_धाराः_Max(&V3GraphEdge::followAlwaysTrue);
            // Remove all Cfunc nodes
            प्रति (V3GraphVertex* ध्रुव vtxp : m_graph.vertices().unlinkable()) {
                यदि (TraceCFuncVertex* ध्रुव vvertexp = vtxp->cast<TraceCFuncVertex>()) {
                    vvertexp->reroute_धाराः(&m_graph);
                    vvertexp->unlinkDelete(&m_graph);
                }
            }
        }

        // Remove multiple variables connecting funcs to traces
        m_graph.remove_Redundant_धाराः_Max(&V3GraphEdge::followAlwaysTrue);

        // If there are any edges from a always, keep only the always
        प्रति (V3GraphVertex& vtx : m_graph.vertices()) {
            यदि (TraceTraceVertex* ध्रुव vvertexp = vtx.cast<TraceTraceVertex>()) {
                // Search for the incoming always edge
                ध्रुव V3GraphEdge* alwaysEdgep = शून्यसूचकः;
                प्रति (ध्रुव V3GraphEdge& edge : vvertexp->अन्तर्_धाराः()) {
                    ध्रुव TraceActivityVertex* ध्रुव actVtxp
                        = edge.fromp()->as<ध्रुव TraceActivityVertex>();
                    यदि (actVtxp->activityAlways()) {
                        alwaysEdgep = &edge;
                        भङ्ग;
                    }
                }
                // If always edge exists, remove all other edges
                यदि (alwaysEdgep) {
                    प्रति (V3GraphEdge* ध्रुव edgep : vvertexp->अन्तर्_धाराः().unlinkable()) {
                        यदि (edgep != alwaysEdgep) VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
                    }
                }
            }
        }

        // Activity points with no outputs can be removed
        प्रति (V3GraphVertex* ध्रुव vtxp : m_graph.vertices().unlinkable()) {
            यदि (TraceActivityVertex* ध्रुव aVtxp = vtxp->cast<TraceActivityVertex>()) {
                // Leave in the always vertex for later use.
                यदि (aVtxp != m_alwaysVtxp && aVtxp->बहिर्गमनम्_रिक्तानि()) {
                    VL_DO_DANGLING(aVtxp->unlinkDelete(&m_graph), aVtxp);
                }
            }
        }
    }

    uint32_t assignactivityNumbers() {
        uint32_t activity_सङ्ख्या = 1;  // Note 0 indicates "slow" only
        प्रति (V3GraphVertex& vtx : m_graph.vertices()) {
            यदि (TraceActivityVertex* ध्रुव vvertexp = vtx.cast<TraceActivityVertex>()) {
                यदि (vvertexp != m_alwaysVtxp) {
                    यदि (vvertexp->slow()) {
                        vvertexp->activity_संहिता(TraceActivityVertex::ACTIVITY_SLOW);
                    } अन्यथा {
                        vvertexp->activity_संहिता(activity_सङ्ख्या++);
                    }
                }
            }
        }
        प्रत्यागम activity_सङ्ख्या;
    }

    शून्य क्रमय_Traces(TraceVec& traces, uint32_t& nNonConstCodes) {
        // Populate sort structure
        traces.clear();
        nNonConstCodes = 0;
        प्रति (V3GraphVertex& vtx : m_graph.vertices()) {
            यदि (TraceTraceVertex* ध्रुव vtxp = vtx.cast<TraceTraceVertex>()) {
                Act_संहिता_समुच्चयः act_समुच्चयः;
                UINFO(9, "  Add to sort: " << vtxp);
                UINFOTREE(9, vtxp->nodep(), "", "trnode");
                प्रति (ध्रुव V3GraphEdge& edge : vtxp->अन्तर्_धाराः()) {
                    ध्रुव TraceActivityVertex* ध्रुव cfvertexp
                        = edge.fromp()->cast<ध्रुव TraceActivityVertex>();
                    UASSERT_OBJ(cfvertexp, vtxp->nodep(),
                                "Should have been function pointing to this trace");
                    UINFO(9, "   Activity: " << cfvertexp);
                    यदि (cfvertexp->activityAlways()) {
                        // If code 0, we always trace; ignore other codes
                        act_समुच्चयः.निवेशय(TraceActivityVertex::ACTIVITY_ALWAYS);
                    } अन्यथा {
                        act_समुच्चयः.निवेशय(cfvertexp->activity_संहिता());
                    }
                }
                UASSERT_OBJ(act_समुच्चयः.गणनम्(TraceActivityVertex::ACTIVITY_ALWAYS) == 0
                                || act_समुच्चयः.आकार() == 1,
                            vtxp->nodep(), "Always active trace has further triggers");
                // Count nodes
                यदि (!vtxp->duplicatep() && !act_समुच्चयः.रिक्तानि())
                    nNonConstCodes += vtxp->nodep()->संहिता_Inc();
                यदि (act_समुच्चयः.रिक्तानि()) {
                    // If a trace doesn't have activity, it's constant, and we
                    // don't need to track changes on it.
                    act_समुच्चयः.निवेशय(TraceActivityVertex::ACTIVITY_NEVER);
                } अन्यथा यदि (act_समुच्चयः.गणनम्(TraceActivityVertex::ACTIVITY_SLOW) && act_समुच्चयः.आकार() > 1) {
                    // If a trace depends on the slow flag as well as other
                    // flags, remove the dependency on the slow flag. We will
                    // make slow routines set all activity flags.
                    act_समुच्चयः.erase(TraceActivityVertex::ACTIVITY_SLOW);
                }
                traces.emplace(act_समुच्चयः, vtxp);
            }
        }
    }

    शून्य graphOptimize() {
        // Assign initial activity numbers to activity vertices
        assignactivityNumbers();

        // Sort the traces by activity sets
        TraceVec traces;
        uint32_t unused1;
        क्रमय_Traces(traces, unused1);

        // For each activity set with only a small number of signals, make those
        // signals always traced, as it's cheaper to check a few value changes
        // than to test a lot of activity flags
        स्वचालित it = traces.begin();
        ध्रुव स्वचालित अन्तः_2 = traces.अन्तः_2();
        यावत् (it != अन्तः_2) {
            स्वचालित शीर्ष = it;
            // Approximate the complexity of the value change check
            uint32_t complexity = 0;
            ध्रुव Act_संहिता_समुच्चयः& act_समुच्चयः = it->first;
            प्रति (; it != अन्तः_2 && it->first == act_समुच्चयः; ++it) {
                यदि (!it->second->duplicatep()) {
                    uint32_t cost = 0;
                    ध्रुव AstTraceDecl* ध्रुव declp = it->second->nodep();
                    // The number of comparisons required by bufp->chg*
                    cost += declp->isWide() ? declp->संहिता_Inc() : 1;
                    // Arrays are traced by element
                    cost *= declp->arrayRange().ranged() ? declp->arrayRange().elements() : 1;
                    // Note: Experiments factoring in the size of declp->valuep()
                    // showed no benefit in tracing speed, even for large trees,
                    // so we will leave those out for now.
                    complexity += cost;
                }
            }
            // Leave alone always changing, never changing and signals only set in slow code
            यदि (act_समुच्चयः.गणनम्(TraceActivityVertex::ACTIVITY_ALWAYS)) अनुवर्त;
            यदि (act_समुच्चयः.गणनम्(TraceActivityVertex::ACTIVITY_NEVER)) अनुवर्त;
            यदि (act_समुच्चयः.गणनम्(TraceActivityVertex::ACTIVITY_SLOW)) अनुवर्त;
            // If the value comparisons are cheaper to perform than checking the
            // activity flags make the signals always traced. Note this cost
            // equation is heuristic.
            यदि (complexity <= act_समुच्चयः.आकार() * 2) {
                प्रति (; शीर्ष != it; ++शीर्ष) {
                    नूतन V3GraphEdge{&m_graph, m_alwaysVtxp, शीर्ष->second, 1};
                }
            }
        }

        graphSimplify(असत्य);
    }

    Ast_ग्रन्थिः_व्यञ्जकम्* selectActivity(File_पङ्क्तिः* flp, uint32_t acode, ध्रुव VAccess& access) {
        प्रत्यागम नूतन AstArraySel(flp, नूतन AstVarRef{flp, m_activityVscp, access}, acode);
    }

    Ast_ग्रन्थिः* newActivitySetter(Ast_ग्रन्थिः* insertp, uint32_t संहिता) {
        ++m_statSetters;
        File_पङ्क्तिः* ध्रुव fl = insertp->fileline();
        Ast_नियोजय* ध्रुव setterp = नूतन Ast_नियोजय{fl, selectActivity(fl, संहिता, VAccess::WRITE),
                                                 नूतन AstConst{fl, AstConst::BitTrue{}}};
        प्रत्यागम setterp;
    }

    Ast_ग्रन्थिः* newActivityAll(Ast_ग्रन्थिः* insertp) {
        ++m_statSettersSlow;
        यदि (!m_actAllFuncp) {
            File_पङ्क्तिः* ध्रुव flp = m_शिखरम्_Scopep->fileline();
            AstCFunc* ध्रुव funcp = नूतन AstCFunc{flp, "__VtraceActivitySetAll", m_शिखरम्_Scopep};
            funcp->slow(सत्य);
            funcp->isStatic(असत्य);
            funcp->isLoose(सत्य);
            m_शिखरम्_Scopep->सङ्कलय_Blocksp(funcp);
            प्रति (uint32_t संहिता = 0; संहिता < m_activity_सङ्ख्या; ++संहिता) {
                Ast_ग्रन्थिः* ध्रुव setterp = newActivitySetter(insertp, संहिता);
                funcp->सङ्कलय_Stmtsp(setterp);
            }
            m_actAllFuncp = funcp;
        }
        AstCCall* ध्रुव callp = नूतन AstCCall{insertp->fileline(), m_actAllFuncp};
        callp->dtype_समुच्चयः_Void();
        प्रत्यागम callp->makeStmt();
    }

    शून्य createActivityFlags() {
        // Assign final activity numbers
        m_activity_सङ्ख्या = assignactivityNumbers();

        // Create an array of bytes, not a bit vector, as they can be set
        // atomically by mtasks, and are cheaper to set (no need for
        // read-modify-write on the C type), and the speed of the tracing code
        // is the same on largish designs.
        File_पङ्क्तिः* ध्रुव flp = m_शिखरम्_Scopep->fileline();
        Ast_ग्रन्थिः_DType* ध्रुव newScalarDtp = नूतन AstBasicDType{flp, VFlagBitPacked{}, 1};
        v3Global.rootp()->typeTablep()->सङ्कलय_Typesp(newScalarDtp);
        AstRange* ध्रुव newArange
            = नूतन AstRange{flp, VNumRange{static_cast<पूर्णाङ्क>(m_activity_सङ्ख्या) - 1, 0}};
        Ast_ग्रन्थिः_DType* ध्रुव newArrDtp = नूतन AstUnpackArrayDType{flp, newScalarDtp, newArange};
        v3Global.rootp()->typeTablep()->सङ्कलय_Typesp(newArrDtp);
        m_activityVscp = v3Global.rootp()->शिखरम्_Scopep()->create_अस्थायी("__VtraceActivity", newArrDtp);

        // Insert activity setters
        प्रति (ध्रुव V3GraphVertex& vtx : m_graph.vertices()) {
            यदि (ध्रुव TraceActivityVertex* ध्रुव vtxp = vtx.cast<ध्रुव TraceActivityVertex>()) {
                Ast_ग्रन्थिः* setterp = शून्यसूचकः;
                यदि (vtxp->activitySlow()) {
                    setterp = newActivityAll(vtxp->insertp());
                    // Just set all flags in slow code as it should be rare.
                    // This will be rolled up into a loop by V3Reloop.
                } अन्यथा यदि (!vtxp->activityAlways()) {
                    setterp = newActivitySetter(vtxp->insertp(), vtxp->activity_संहिता());
                }
                यदि (setterp) {
                    Ast_ग्रन्थिः* ध्रुव insertp = vtxp->insertp();
                    यदि (Ast_Stmt_व्यञ्जकम्* ध्रुव stmtp = VN_CAST(insertp, Stmt_व्यञ्जकम्)) {
                        stmtp->सङ्कलय_अग्र_अत्र(setterp);
                    } अन्यथा यदि (AstCFunc* ध्रुव funcp = VN_CAST(insertp, CFunc)) {
                        // If there are awaits, insert the setter after each await
                        यदि (funcp->isCoroutine() && funcp->stmtsp()) {
                            funcp->stmtsp()->foreach_And_अग्र([setterp](AstCAwait* awaitp) {
                                awaitp->सङ्कलय_अग्र_अत्र(setterp->clone_वृक्षः(असत्य));
                            });
                        }
                        funcp->सङ्कलय_Stmtsp(setterp);
                    } अन्यथा {
                        insertp->v3fatalSrc("Bad trace activity vertex");
                    }
                }
            }
        }
    }

    AstCFunc* newCFunc(VTraceType traceType, AstCFunc* शिखरम्_Funcp, uint32_t func_अङ्कः,
                       uint32_t आधारः_संहिता = 0, ध्रुव AstTraceDecl* ध्रुव declp = शून्यसूचकः,
                       तार्किक declSub = असत्य) {
        // Create new function
        ध्रुव तार्किक is_शिखरम्_Func = !declp && शिखरम्_Funcp == शून्यसूचकः;
        std::string funcName;
        यदि (is_शिखरम्_Func || declp) {
            funcName = traceType.func_उपसर्गः();
        } अन्यथा {
            funcName = शिखरम्_Funcp->name();
            funcName += "_sub";
        }
        यदि (declp) {
            funcName += "_dtype";
            यदि (declSub) funcName += "_sub";
            funcName += "__";
            funcName = m_dtype_नामानि.get(funcName);
        } अन्यथा {
            funcName += "_";
            funcName += cvt_गन्तव्यम्_Str(func_अङ्कः);
        }

        File_पङ्क्तिः* ध्रुव flp = m_शिखरम्_Scopep->fileline();
        AstCFunc* ध्रुव funcp = नूतन AstCFunc{flp, funcName, m_शिखरम्_Scopep};
        funcp->isTrace(सत्य);
        funcp->dontCombine(सत्य);
        funcp->isLoose(सत्य);
        funcp->slow(traceType != VTraceType::CHANGE);
        funcp->isStatic(is_शिखरम्_Func);
        // Add it to top scope
        m_शिखरम्_Scopep->सङ्कलय_Blocksp(funcp);
        ध्रुव std::string bufArg
            = v3Global.opt.trace_Class_आधारः() + "::Buffer* bufp"
              + (declp ? (", uint32_t offset, const " + declp->dtypep()->cType("", सत्य, सत्य)
                          + " __VdtypeVar")
                       : "");
        यदि (is_शिखरम्_Func) {
            // Top functions
            funcp->argTypes("void* voidSelf, " + bufArg);
            funcp->सङ्कलय_Stmtsp(नूतन AstCStmt{flp, EmitCUtil::void_Self_नियोजय(m_शिखरम्_Modp)});
            funcp->सङ्कलय_Stmtsp(नूतन AstCStmt{flp, EmitCUtil::sym_Class_नियोजय()});
            // Add global activity check to change dump functions
            यदि (traceType == VTraceType::CHANGE) {  //
                funcp->सङ्कलय_Stmtsp(
                    नूतन AstCStmt{flp, "if (VL_UNLIKELY(!vlSymsp->__Vm_activity)) return;"});
            }
            // Register function
            AstCStmt* ध्रुव cstmtp = नूतन AstCStmt{flp};
            m_regFuncp->सङ्कलय_Stmtsp(cstmtp);
            यदि (traceType == VTraceType::CONSTANT) {
                cstmtp->सङ्कलय("tracep->addConstCb(");
                cstmtp->सङ्कलय(नूतन Ast_Addr_सम्बन्धि_CFunc{flp, funcp});
                cstmtp->सङ्कलय(", " + std::गन्तव्यम्_string(func_अङ्कः) + ", vlSelf);");
            } अन्यथा यदि (traceType == VTraceType::पूर्णम्) {
                cstmtp->सङ्कलय("tracep->addFullCb(");
                cstmtp->सङ्कलय(नूतन Ast_Addr_सम्बन्धि_CFunc{flp, funcp});
                cstmtp->सङ्कलय(", " + std::गन्तव्यम्_string(func_अङ्कः) + ", vlSelf);");
            } अन्यथा {
                cstmtp->सङ्कलय("tracep->addChgCb(");
                cstmtp->सङ्कलय(नूतन Ast_Addr_सम्बन्धि_CFunc{flp, funcp});
                cstmtp->सङ्कलय(", " + std::गन्तव्यम्_string(func_अङ्कः) + ", vlSelf);");
            }
        } अन्यथा {
            // Sub functions
            funcp->argTypes(bufArg);
            // Setup base references. Note in rare occasions we can end up with an empty trace
            // sub function, hence the VL_ATTR_UNUSED attributes.
            यदि (traceType != VTraceType::CHANGE) {
                // Full dump sub function
                funcp->सङ्कलय_Stmtsp(नूतन AstCStmt{flp,  //
                                              string("uint32_t* const oldp VL_ATTR_UNUSED = "
                                                     "bufp->oldp(vlSymsp->__Vm_baseCode")
                                                  + (declp ? " + offset" : "") + ");\n"});
            } अन्यथा {
                // Change dump sub function
                funcp->सङ्कलय_Stmtsp(नूतन AstCStmt{flp,  //
                                              "uint32_t* const oldp VL_ATTR_UNUSED = "
                                              "bufp->oldp(vlSymsp->__Vm_baseCode + "
                                                  + (declp ? " offset" : cvt_गन्तव्यम्_Str(आधारः_संहिता))
                                                  + ");\n"});
            }
            यदि (!declp) {
                // Add call to top function
                AstCCall* ध्रुव callp = नूतन AstCCall{funcp->fileline(), funcp};
                callp->dtype_समुच्चयः_Void();
                callp->argTypes("bufp");
                शिखरम्_Funcp->सङ्कलय_Stmtsp(callp->makeStmt());
            }
        }
        //समाप्तम्
        UINFO(5, "  newCFunc " << funcp);
        प्रत्यागम funcp;
    }

    AstCFunc* newCDtypeSubFunc(VTraceType traceType, ध्रुव AstTraceDecl* ध्रुव declp,
                               AstCFunc* parentp) {
        AstCFunc* ध्रुव funcp = newCFunc(traceType, शून्यसूचकः, 0, 0, declp, सत्य);
        File_पङ्क्तिः* ध्रुव flp = m_शिखरम्_Scopep->fileline();
        AstCCall* ध्रुव callp = नूतन AstCCall{flp, funcp};
        callp->dtype_समुच्चयः_Void();
        callp->argTypes("bufp, offset, __VdtypeVar");
        parentp->सङ्कलय_Stmtsp(callp->makeStmt());
        प्रत्यागम funcp;
    }

    AstCFunc* createConstDtypeTraceFunctions(ध्रुव AstTraceDecl* declp) {
        ध्रुव Ast_ग्रन्थिः_DType* ध्रुव dtypep = declp->dtypep()->skipRefp();
        स्वचालित pair = m_dtypeConstFuncs.emplace(dtypep, शून्यसूचकः);
        यदि (pair.second) {
            File_पङ्क्तिः* ध्रुव flp = declp->fileline();
            AstCFunc* ध्रुव funcp = newCFunc(VTraceType::CONSTANT, शून्यसूचकः, 0, 0, declp);

            तार्किक first = सत्य;
            प्रति (Ast_ग्रन्थिः* callStmtp = declp->dtypeCallp()->funcp()->stmtsp(); callStmtp;
                 callStmtp = callStmtp->nextp()) {
                ध्रुव AstCCall* ध्रुव callp
                    = VN_CAST(VN_CAST(callStmtp, Stmt_व्यञ्जकम्)->exprp(), CCall);
                तार्किक onlyOne = first && !callStmtp->nextp();
                AstCFunc* ध्रुव subFuncp
                    = onlyOne ? funcp : newCDtypeSubFunc(VTraceType::CONSTANT, declp, funcp);
                प्रति (Ast_ग्रन्थिः* stmtp = callp->funcp()->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    यदि (AstTraceDecl* ध्रुव क्षेत्रम्_Declp = VN_CAST(stmtp, TraceDecl)) {
                        AstTraceInc* ध्रुव incp
                            = नूतन AstTraceInc{flp, क्षेत्रम्_Declp, VTraceType::CONSTANT};
                        subFuncp->सङ्कलय_Stmtsp(incp);
                    }
                }
                first = असत्य;
            }

            pair.first->second = funcp;
        }

        प्रत्यागम pair.first->second;
    }

    शून्य createConstTraceFunctions(ध्रुव TraceVec& traces) {
        ध्रुव पूर्णाङ्क विभज_सीमा = v3Global.opt.output_विभज_CTrace() ? v3Global.opt.output_विभज_CTrace()
                                                                : std::numeric_limits<पूर्णाङ्क>::max();

        AstCFunc* ध्रुव शिखरम्_Funcp = newCFunc(VTraceType::CONSTANT, शून्यसूचकः, 0);
        uint32_t sub_Func_अङ्कः = 0;
        AstCFunc* subFuncp = शून्यसूचकः;
        पूर्णाङ्क subStmts = 0;
        std::सदिशम्<ध्रुव TraceTraceVertex*> duplicates;
        प्रति (स्वचालित it = traces.cbegin(); it != traces.अन्तः_2(); ++it) {
            ध्रुव TraceTraceVertex* ध्रुव vtxp = it->second;
            AstTraceDecl* ध्रुव declp = vtxp->nodep();
            यदि (vtxp->duplicatep()) {
                duplicates.प्रक्षिप_पृष्ठभागः(vtxp);
                अनुवर्त;
            }

            // This is a canonical trace node. Assign trace code (signal number).
            UASSERT_OBJ(!declp->संहिता_Assigned(), declp,
                        "Canonical node should not have code assigned yet");
            declp->संहिता(m_संहिता);
            ध्रुव uint32_t संहिता_Inc = declp->संहिता_Inc();
            m_संहिता += संहिता_Inc;
            ++m_statUniqSigs;

            // If this is a const signal, add the AstTraceInc
            ध्रुव Act_संहिता_समुच्चयः& act_समुच्चयः = it->first;
            यदि (act_समुच्चयः.गणनम्(TraceActivityVertex::ACTIVITY_NEVER)) {
                // Crate new sub function if required
                यदि (!subFuncp || subStmts > विभज_सीमा) {
                    subStmts = 0;
                    subFuncp = newCFunc(VTraceType::CONSTANT, शिखरम्_Funcp, sub_Func_अङ्कः);
                    ++sub_Func_अङ्कः;
                }
                File_पङ्क्तिः* ध्रुव flp = declp->fileline();
                यदि (declp->dtypeCallp()) {
                    AstCFunc* ध्रुव funcp = createConstDtypeTraceFunctions(declp);
                    Ast_ग्रन्थिः_व्यञ्जकम्* argsp = शून्यसूचकः;
                    argsp = Ast_ग्रन्थिः::सङ्कलय_अग्र(argsp, declp->valuep()->clone_वृक्षः(असत्य));
                    AstCCall* ध्रुव callp = नूतन AstCCall{flp, funcp, argsp};
                    callp->dtype_समुच्चयः_Void();
                    callp->argTypes(callp->argTypes() + "bufp, " + std::गन्तव्यम्_string(declp->संहिता()));
                    subFuncp->सङ्कलय_Stmtsp(callp->makeStmt());

                    subStmts += 1;
                } अन्यथा {
                    AstTraceInc* ध्रुव incp = नूतन AstTraceInc{flp, declp, VTraceType::CONSTANT};
                    subFuncp->सङ्कलय_Stmtsp(incp);
                    subStmts += incp->ग्रन्थिः_गणनम्();
                }
            }
        }

        // Assign codes to duplicate nodes from their canonicals
        प्रति (ध्रुव TraceTraceVertex* ध्रुव vtxp : duplicates) {
            AstTraceDecl* ध्रुव declp = vtxp->nodep();
            ध्रुव TraceTraceVertex* ध्रुव canonVtxp = vtxp->duplicatep();
            ध्रुव AstTraceDecl* ध्रुव canonDeclp = canonVtxp->nodep();
            UASSERT_OBJ(!canonVtxp->duplicatep(), canonDeclp, "Canonical node is a duplicate");
            UASSERT_OBJ(canonDeclp->संहिता_Assigned(), canonDeclp,
                        "Canonical node should have code assigned already");
            declp->संहिता(canonDeclp->संहिता() + vtxp->dtype_Alias_विस्थापनम्());
        }
    }

    DtypeFuncs createNonConstDtypeTraceFunctions(ध्रुव AstTraceDecl* declp) {
        Ast_ग्रन्थिः_DType* dtypep = declp->dtypep()->skipRefp();
        स्वचालित pair = m_dtypeNonConstFuncs.emplace(dtypep, DtypeFuncs{});
        यदि (pair.second) {
            File_पङ्क्तिः* ध्रुव flp = declp->fileline();
            AstCFunc* ध्रुव पूर्णम्_Funcp = newCFunc(VTraceType::पूर्णम्, शून्यसूचकः, 0, 0, declp);
            AstCFunc* ध्रुव chgFuncp = newCFunc(VTraceType::CHANGE, शून्यसूचकः, 0, 0, declp);

            तार्किक first = सत्य;
            प्रति (Ast_ग्रन्थिः* callStmtp = declp->dtypeCallp()->funcp()->stmtsp(); callStmtp;
                 callStmtp = callStmtp->nextp()) {
                ध्रुव AstCCall* ध्रुव callp
                    = VN_CAST(VN_CAST(callStmtp, Stmt_व्यञ्जकम्)->exprp(), CCall);
                तार्किक onlyOne = first && !callStmtp->nextp();
                AstCFunc* ध्रुव पूर्णम्_Sub_Funcp
                    = onlyOne ? पूर्णम्_Funcp : newCDtypeSubFunc(VTraceType::पूर्णम्, declp, पूर्णम्_Funcp);
                AstCFunc* ध्रुव chgSubFuncp
                    = onlyOne ? chgFuncp : newCDtypeSubFunc(VTraceType::CHANGE, declp, chgFuncp);
                प्रति (Ast_ग्रन्थिः* stmtp = callp->funcp()->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    यदि (AstTraceDecl* ध्रुव क्षेत्रम्_Declp = VN_CAST(stmtp, TraceDecl)) {
                        AstTraceInc* ध्रुव incFullp
                            = नूतन AstTraceInc{flp, क्षेत्रम्_Declp, VTraceType::पूर्णम्};
                        पूर्णम्_Sub_Funcp->सङ्कलय_Stmtsp(incFullp);
                        AstTraceInc* ध्रुव incChgp
                            = नूतन AstTraceInc{flp, क्षेत्रम्_Declp, VTraceType::CHANGE};
                        chgSubFuncp->सङ्कलय_Stmtsp(incChgp);
                    }
                }
                first = असत्य;
            }

            pair.first->second = {.पूर्णम्_Funcp = पूर्णम्_Funcp, .chgFuncp = chgFuncp};
        }

        प्रत्यागम pair.first->second;
    }

    शून्य createNonConstTraceFunctions(ध्रुव TraceVec& traces, uint32_t nAllCodes,
                                      uint32_t parallelism) {
        ध्रुव पूर्णाङ्क विभज_सीमा = v3Global.opt.output_विभज_CTrace() ? v3Global.opt.output_विभज_CTrace()
                                                                : std::numeric_limits<पूर्णाङ्क>::max();

        // pre-incremented, so starts at 0
        uint32_t शिखरम्_Func_अङ्कः = std::numeric_limits<uint32_t>::max();
        TraceVec::const_iterator it = traces.begin();
        यावत् (it != traces.अन्तः_2()) {
            AstCFunc* शिखरम्_Ful_Funcp = शून्यसूचकः;
            AstCFunc* शिखरम्_Chg_Funcp = शून्यसूचकः;
            AstCFunc* subFulFuncp = शून्यसूचकः;
            AstCFunc* subChgFuncp = शून्यसूचकः;
            uint32_t sub_Func_अङ्कः = 0;
            पूर्णाङ्क subStmts = 0;
            ध्रुव uint32_t maxCodes = std::max((nAllCodes + parallelism - 1) / parallelism, 1U);
            uint32_t nCodes = 0;
            ध्रुव Act_संहिता_समुच्चयः* पूर्व_Act_समुच्चयः = शून्यसूचकः;
            AstIf* ifp = शून्यसूचकः;
            uint32_t आधारः_संहिता = 0;
            प्रति (; nCodes < maxCodes && it != traces.अन्तः_2(); ++it) {
                ध्रुव Act_संहिता_समुच्चयः& act_समुच्चयः = it->first;
                // Traced value never changes, no need to add it
                यदि (act_समुच्चयः.गणनम्(TraceActivityVertex::ACTIVITY_NEVER)) अनुवर्त;

                ध्रुव TraceTraceVertex* ध्रुव vtxp = it->second;
                AstTraceDecl* ध्रुव declp = vtxp->nodep();

                // This is a duplicate decl, no need to add it, but must set the
                // function index to the same as the canonical node.
                यदि (ध्रुव TraceTraceVertex* ध्रुव canonVtxp = vtxp->duplicatep()) {
                    declp->fidx(canonVtxp->nodep()->fidx());
                    अनुवर्त;
                }

                // Create top function if not yet created
                यदि (!शिखरम्_Ful_Funcp) {
                    ++शिखरम्_Func_अङ्कः;
                    शिखरम्_Ful_Funcp = newCFunc(VTraceType::पूर्णम्, शून्यसूचकः, शिखरम्_Func_अङ्कः);
                    शिखरम्_Chg_Funcp = newCFunc(VTraceType::CHANGE, शून्यसूचकः, शिखरम्_Func_अङ्कः);
                }

                // Create new sub function if required
                यदि (!subFulFuncp || subStmts > विभज_सीमा) {
                    आधारः_संहिता = declp->संहिता();
                    subStmts = 0;
                    subFulFuncp = newCFunc(VTraceType::पूर्णम्, शिखरम्_Ful_Funcp, sub_Func_अङ्कः, आधारः_संहिता);
                    subChgFuncp = newCFunc(VTraceType::CHANGE, शिखरम्_Chg_Funcp, sub_Func_अङ्कः, आधारः_संहिता);
                    ++sub_Func_अङ्कः;
                    पूर्व_Act_समुच्चयः = शून्यसूचकः;
                    ifp = शून्यसूचकः;
                }

                // If required, create the conditional node checking the activity flags
                यदि (!पूर्व_Act_समुच्चयः || act_समुच्चयः != *पूर्व_Act_समुच्चयः) {
                    File_पङ्क्तिः* ध्रुव flp = m_शिखरम्_Scopep->fileline();
                    ध्रुव तार्किक always = act_समुच्चयः.गणनम्(TraceActivityVertex::ACTIVITY_ALWAYS) != 0;
                    Ast_ग्रन्थिः_व्यञ्जकम्* condp = शून्यसूचकः;
                    यदि (always) {
                        condp = नूतन AstConst{flp, 1};  // Always true, will be folded later
                    } अन्यथा {
                        प्रति (ध्रुव uint32_t act_संहिता : act_समुच्चयः) {
                            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव selp = selectActivity(flp, act_संहिता, VAccess::READ);
                            condp = condp ? नूतन AstOr{flp, condp, selp} : selp;
                        }
                    }
                    ifp = नूतन AstIf{flp, condp};
                    यदि (!always) ifp->branchPred(VBranchPred::BP_UNLIKELY);
                    subChgFuncp->सङ्कलय_Stmtsp(ifp);
                    subStmts += ifp->ग्रन्थिः_गणनम्();
                    पूर्व_Act_समुच्चयः = &act_समुच्चयः;
                }

                // Add TraceInc nodes
                File_पङ्क्तिः* ध्रुव flp = declp->fileline();
                यदि (declp->dtypeCallp()) {
                    DtypeFuncs funcs = createNonConstDtypeTraceFunctions(declp);
                    Ast_ग्रन्थिः_व्यञ्जकम्* argsp = शून्यसूचकः;
                    argsp = Ast_ग्रन्थिः::सङ्कलय_अग्र(argsp, declp->valuep()->clone_वृक्षः(असत्य));
                    AstCCall* ध्रुव callFullp = नूतन AstCCall{flp, funcs.पूर्णम्_Funcp, argsp};
                    callFullp->dtype_समुच्चयः_Void();
                    callFullp->argTypes(callFullp->argTypes() + "bufp, "
                                        + std::गन्तव्यम्_string(declp->संहिता()));
                    subFulFuncp->सङ्कलय_Stmtsp(callFullp->makeStmt());
                    argsp = शून्यसूचकः;
                    argsp = Ast_ग्रन्थिः::सङ्कलय_अग्र(argsp, declp->valuep()->clone_वृक्षः(असत्य));
                    AstCCall* ध्रुव callChgp = नूतन AstCCall{flp, funcs.chgFuncp, argsp};
                    callChgp->dtype_समुच्चयः_Void();
                    callChgp->argTypes(callChgp->argTypes() + "bufp, "
                                       + std::गन्तव्यम्_string(declp->संहिता()));
                    ifp->सङ्कलय_Thensp(callChgp->makeStmt());

                    subStmts += 2;
                } अन्यथा {
                    AstTraceInc* ध्रुव incFulp = नूतन AstTraceInc{flp, declp, VTraceType::पूर्णम्};
                    subFulFuncp->सङ्कलय_Stmtsp(incFulp);
                    AstTraceInc* ध्रुव incChgp
                        = नूतन AstTraceInc{flp, declp, VTraceType::CHANGE, आधारः_संहिता};
                    ifp->सङ्कलय_Thensp(incChgp);

                    // Set the function index of the decl
                    declp->fidx(शिखरम्_Func_अङ्कः);

                    // Track splitting due to size
                    UASSERT_OBJ(incFulp->ग्रन्थिः_गणनम्() == incChgp->ग्रन्थिः_गणनम्(), declp,
                                "Should have equal cost");
                    ध्रुव VNumRange range = declp->arrayRange();
                    यदि (range.ranged()) {
                        // 2x because each element is a TraceInc and a VarRef
                        subStmts += range.elements() * 2;
                    } अन्यथा {
                        subStmts += incChgp->ग्रन्थिः_गणनम्();
                    }
                }

                // Track partitioning
                nCodes += declp->संहिता_Inc();
            }
        }
    }

    शून्य createCleanupFunction() {
        File_पङ्क्तिः* ध्रुव fl = m_शिखरम्_Scopep->fileline();
        AstCFunc* ध्रुव cleanupFuncp = नूतन AstCFunc{fl, "trace_cleanup", m_शिखरम्_Scopep};
        cleanupFuncp->argTypes("void* voidSelf, " + v3Global.opt.trace_Class_आधारः()
                               + "* /*unused*/");
        cleanupFuncp->isTrace(सत्य);
        cleanupFuncp->slow(असत्य);
        cleanupFuncp->isStatic(सत्य);
        cleanupFuncp->isLoose(सत्य);
        m_शिखरम्_Scopep->सङ्कलय_Blocksp(cleanupFuncp);
        cleanupFuncp->सङ्कलय_Stmtsp(नूतन AstCStmt{fl, EmitCUtil::void_Self_नियोजय(m_शिखरम्_Modp)});
        cleanupFuncp->सङ्कलय_Stmtsp(नूतन AstCStmt{fl, EmitCUtil::sym_Class_नियोजय()});

        // Register it
        {
            AstCStmt* ध्रुव cstmtp = नूतन AstCStmt{fl};
            m_regFuncp->सङ्कलय_Stmtsp(cstmtp);
            cstmtp->सङ्कलय("tracep->addCleanupCb(");
            cstmtp->सङ्कलय(नूतन Ast_Addr_सम्बन्धि_CFunc{fl, cleanupFuncp});
            cstmtp->सङ्कलय(", vlSelf);");
        }

        // Clear global activity flag
        cleanupFuncp->सङ्कलय_Stmtsp(
            नूतन AstCStmt{m_शिखरम्_Scopep->fileline(), "vlSymsp->__Vm_activity = false;"s});

        // Clear fine grained activity flags
        प्रति (uint32_t i = 0; i < m_activity_सङ्ख्या; ++i) {
            Ast_ग्रन्थिः* ध्रुव clrp = नूतन Ast_नियोजय{fl, selectActivity(fl, i, VAccess::WRITE),
                                                नूतन AstConst{fl, AstConst::BitFalse{}}};
            cleanupFuncp->सङ्कलय_Stmtsp(clrp);
        }
    }

    शून्य createTraceFunctions() {
        // Detect and remove duplicate values
        detectDuplicates();

        graphDtypePrune();
        विभज_Trace_Decl_Funcs();
        m_graph.remove_Redundant_धाराः_Max(&V3GraphEdge::followAlwaysTrue);

        // Simplify & optimize the graph
        यदि (dumpGraphLevel() >= 6) m_graph.dumpDotFilePrefixed("trace_pre");
        graphSimplify(सत्य);
        यदि (dumpGraphLevel() >= 6) m_graph.dumpDotFilePrefixed("trace_simplified");
        graphOptimize();
        यदि (dumpGraphLevel() >= 6) m_graph.dumpDotFilePrefixed("trace_optimized");

        // Create the fine grained activity flags
        createActivityFlags();

        // Form a sorted list of the traces we are interested in
        TraceVec traces;  // The sorted traces
        // We will split functions such that each have to dump roughly the same amount of data
        // for this we need to keep tack of the number of codes used by the trace functions.
        uint32_t nNonConstCodes = 0;
        क्रमय_Traces(traces, nNonConstCodes);

        // Our keys are now sorted to have same activity number adjacent, then
        // by trace order. (Better would be execution order for cache
        // efficiency....) Last are constants and non-changers, as then the
        // last value vector is more compact

        // Create the trace registration function
        m_regFuncp = नूतन AstCFunc{m_शिखरम्_Scopep->fileline(), "trace_register", m_शिखरम्_Scopep};
        m_regFuncp->argTypes(v3Global.opt.trace_Class_आधारः() + "* tracep");
        m_regFuncp->प्रविष्टिः_Point(सत्य);
        m_regFuncp->isTrace(सत्य);
        m_regFuncp->slow(सत्य);
        m_regFuncp->isStatic(असत्य);
        m_regFuncp->isLoose(सत्य);
        m_शिखरम्_Scopep->सङ्कलय_Blocksp(m_regFuncp);

        // Create the const dump functions. Also allocates trace codes.
        createConstTraceFunctions(traces);

        // Create the full and incremental dump functions
        createNonConstTraceFunctions(traces, nNonConstCodes, m_parallelism);

        // Root-traced libraries alias wrapper IOs onto the existing top-module codes.
        यदि (!v3Global.opt.libCreate().रिक्तानि()) {
            std::सदिशम्<AstTraceDecl*> मूलम्_Declps;
            std::सदिशम्<AstTraceDecl*> शिखरम्_Declps;
            TraceInitDeclCollector मूलम्_Collector{मूलम्_Declps};
            TraceInitDeclCollector शिखरम्_Collector{शिखरम्_Declps};
            प्रति (Ast_ग्रन्थिः* blockp = m_शिखरम्_Scopep->blocksp(); blockp; blockp = blockp->nextp()) {
                AstCFunc* ध्रुव funcp = VN_CAST(blockp, CFunc);
                यदि (funcp && VString::startsWith(funcp->name(), "trace_init_leaf_root__")) {
                    मूलम्_Collector.collect(funcp);
                } अन्यथा यदि (funcp && VString::startsWith(funcp->name(), "trace_init_leaf_top__")) {
                    शिखरम्_Collector.collect(funcp);
                }
            }

            std::सदिशम्<तार्किक> प्रयुक्तम्(शिखरम्_Declps.आकार(), असत्य);
            प्रति (AstTraceDecl* ध्रुव मूलम्_Declp : मूलम्_Declps) {
                प्रति (आकार_t i = 0; i < शिखरम्_Declps.आकार(); ++i) {
                    यदि (प्रयुक्तम्[i] || !same_मूलम्_Init_Alias(मूलम्_Declp, शिखरम्_Declps[i])) अनुवर्त;
                    मूलम्_Declp->संहिता(शिखरम्_Declps[i]->संहिता());
                    प्रयुक्तम्[i] = सत्य;
                    भङ्ग;
                }
            }
        }

        // Remove refs to traced values from TraceDecl nodes, these have now moved under
        // TraceInc
        प्रति (ध्रुव स्वचालित& i : traces) {
            Ast_ग्रन्थिः* ध्रुव valuep = i.second->nodep()->valuep();
            VL_DO_DANGLING(valuep->unlink_Fr_पृष्ठभागः()->delete_वृक्षः(), valuep);
        }

        // Create the trace cleanup function clearing the activity flags
        createCleanupFunction();
    }

    TraceCFuncVertex* getCFuncVertexp(AstCFunc* nodep) {
        V3GraphVertex* ध्रुव vtxp = nodep->user1u().गन्तव्यम्_Graph_Vertex();
        TraceCFuncVertex* vertexp = vtxp ? vtxp->cast<TraceCFuncVertex>() : शून्यसूचकः;
        यदि (!vertexp) {
            vertexp = नूतन TraceCFuncVertex{&m_graph, nodep};
            nodep->user1p(vertexp);
        }
        प्रत्यागम vertexp;
    }
    TraceActivityVertex* getActivityVertexp(Ast_ग्रन्थिः* nodep, तार्किक slow) {
        V3GraphVertex* ध्रुव vtxp = nodep->user3u().गन्तव्यम्_Graph_Vertex();
        TraceActivityVertex* vertexp = vtxp ? vtxp->cast<TraceActivityVertex>() : शून्यसूचकः;
        यदि (!vertexp) {
            vertexp = नूतन TraceActivityVertex{&m_graph, nodep, slow};
            nodep->user3p(vertexp);
        }
        vertexp->slow(slow);
        प्रत्यागम vertexp;
    }

    // VISITORS
    शून्य दर्शय(AstNetlist* nodep) override {
        // Add vertexes for all TraceDecl, and edges from VARs each trace looks at
        m_finding = असत्य;
        iterateChildren(nodep);

        // Add vertexes for all CFUNCs, and edges to VARs the func sets
        m_finding = सत्य;
        iterateChildren(nodep);

        // Create the trace functions and insert them into the tree
        createTraceFunctions();

        // Save number of trace codes used
        nodep->nTraceCodes(m_संहिता);
    }
    शून्य दर्शय(Ast_ग्रन्थिः_Module* nodep) override {
        यदि (nodep->is_शिखरम्()) m_शिखरम्_Modp = nodep;
        iterateChildren(nodep);
    }
    शून्य दर्शय(AstVarScope* nodep) override {
        यदि (!m_finding) {
            यदि (ध्रुव AstIface* ध्रुव ifacep = nodep->varp()->sensIfacep()) {
                m_ifaceMemberVscps[{ifacep, nodep->varp()->name()}].प्रक्षिप_पृष्ठभागः(nodep);
            }
        }
    }
    शून्य दर्शय(Ast_Stmt_व्यञ्जकम्* nodep) override {
        यदि (!m_finding && !nodep->user2()) {
            यदि (AstCCall* ध्रुव callp = VN_CAST(nodep->exprp(), CCall)) {
                UINFO(8, "   CCALL " << callp);
                // See if there are other calls in same statement list;
                // If so, all funcs might share the same activity code
                TraceActivityVertex* ध्रुव activityVtxp
                    = getActivityVertexp(nodep, callp->funcp()->slow());
                प्रति (Ast_ग्रन्थिः* nextp = nodep; nextp; nextp = nextp->nextp()) {
                    यदि (Ast_Stmt_व्यञ्जकम्* ध्रुव stmtp = VN_CAST(nextp, Stmt_व्यञ्जकम्)) {
                        यदि (AstCCall* ध्रुव ccallp = VN_CAST(stmtp->exprp(), CCall)) {
                            stmtp->user2(सत्य);  // Processed
                            UINFO(8, "     SubCCALL " << ccallp);
                            V3GraphVertex* ध्रुव ccallFuncVtxp = getCFuncVertexp(ccallp->funcp());
                            activityVtxp->slow(ccallp->funcp()->slow());
                            नूतन V3GraphEdge{&m_graph, activityVtxp, ccallFuncVtxp, 1};
                        }
                    }
                }
            }
        }
        iterateChildren(nodep);
    }
    शून्य दर्शय(AstCFunc* nodep) override {
        UINFO(8, "   CFUNC " << nodep);
        V3GraphVertex* ध्रुव funcVtxp = getCFuncVertexp(nodep);
        यदि (!m_finding) {  // If public, we need a unique activity code to allow for sets
                           // directly in this func
            यदि (nodep->func_लौकिकम्() || nodep->dpiExportImpl() || nodep->प्रविष्टिः_Point()
                || nodep->isCoroutine()) {
                // Cannot treat a coroutine as slow, it may be resumed later
                ध्रुव तार्किक slow = nodep->slow() && !nodep->isCoroutine();
                TraceActivityVertex* ध्रुव activityVtxp = getActivityVertexp(nodep, slow);
                नूतन V3GraphEdge{&m_graph, activityVtxp, funcVtxp, 1};
            }
        }
        VL_RESTORER(m_cfuncp);
        m_cfuncp = nodep;
        iterateChildren(nodep);
    }
    शून्य दर्शय(AstTraceDecl* nodep) override {
        UINFO(8, "   TRACE " << nodep);
        यदि (!m_finding && !nodep->अन्तर्_Dtype_Func()) {
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
            यदि (m_cfuncp && VString::startsWith(m_cfuncp->name(), "trace_init_leaf_root__")) {
                प्रत्यागम;
            }
            V3GraphVertex* ध्रुव vertexp = नूतन TraceTraceVertex{&m_graph, nodep};
            nodep->user1p(vertexp);

            UASSERT_OBJ(m_cfuncp, nodep, "Trace not under func");
            m_cfuncp->user2(सत्य);
            VL_RESTORER(m_tracep);
            m_tracep = nodep;
            iterateChildren(nodep);
        }
    }
    शून्य दर्शय(AstVarRef* nodep) override {
        UASSERT_OBJ(nodep->varScopep(), nodep, "No var scope?");
        AstVarScope* ध्रुव varscopep = nodep->varScopep();
        V3GraphVertex* varVtxp = varscopep->user1u().गन्तव्यम्_Graph_Vertex();
        यदि (m_tracep) {
            UASSERT_OBJ(nodep->access().isReadOnly(), nodep, "Lvalue in trace?  Should be const.");
            यदि (!varVtxp) {
                varVtxp = नूतन TraceVarVertex{&m_graph, nodep->varScopep()};
                nodep->varScopep()->user1p(varVtxp);
            }
            V3GraphVertex* ध्रुव traceVtxp = m_tracep->user1u().गन्तव्यम्_Graph_Vertex();
            नूतन V3GraphEdge{&m_graph, varVtxp, traceVtxp, 1};
            यदि (nodep->varp()->isPrimaryInish()  // Always need to trace primary inputs
                || nodep->varp()->is_Sig_लौकिकम्()) {  // Or ones user can change
                नूतन V3GraphEdge{&m_graph, m_alwaysVtxp, traceVtxp, 1};
            }
            यदि (m_tracep->dtypeCallp()) varscopep->user2p(m_tracep);
        } अन्यथा यदि (m_cfuncp && m_finding && nodep->access().isWriteOrRW()) {
            V3GraphVertex* ध्रुव funcVtxp = getCFuncVertexp(m_cfuncp);
            यदि (varVtxp) {  // else we're not tracing this signal
                नूतन V3GraphEdge{&m_graph, funcVtxp, varVtxp, 1};
                AstTraceDecl* ध्रुव declp = VN_AS(varscopep->user2p(), TraceDecl);
                यदि (declp) {
                    V3GraphVertex* ध्रुव cFuncVtxp = getCFuncVertexp(m_cfuncp);
                    प्रति (ध्रुव V3GraphEdge& edge : cFuncVtxp->अन्तर्_धाराः()) {
                        V3GraphVertex* ध्रुव activityp = edge.fromp();
                        यदि (!declp->user3p()) {
                            declp->user3p(activityp);
                        } अन्यथा यदि (declp->user3u().गन्तव्यम्_Graph_Vertex() != activityp) {
                            declp->user2(सत्य);
                        }
                    }
                }
            }
        }
    }
    शून्य दर्शय(AstMemberSel* nodep) override {
        यदि (m_cfuncp && m_finding && nodep->access().isWriteOrRW()) {
            AstIfaceRefDType* ध्रुव dtypep
                = VN_CAST(nodep->fromp()->dtypep()->skipRefp(), IfaceRefDType);
            यदि (dtypep && dtypep->isVirtual()) {
                ध्रुव स्वचालित it = m_ifaceMemberVscps.find({dtypep->ifacep(), nodep->varp()->name()});
                यदि (it != m_ifaceMemberVscps.अन्तः_2()) {
                    V3GraphVertex* ध्रुव funcVtxp = getCFuncVertexp(m_cfuncp);
                    प्रति (AstVarScope* ध्रुव vscp : it->second) {
                        V3GraphVertex* varVtxp = vscp->user1u().गन्तव्यम्_Graph_Vertex();
                        यदि (!varVtxp) {
                            varVtxp = नूतन TraceVarVertex{&m_graph, vscp};
                            vscp->user1p(varVtxp);
                        }
                        नूतन V3GraphEdge{&m_graph, funcVtxp, varVtxp, 1};
                    }
                }
            }
        }
        iterateChildren(nodep);
    }
    //--------------------
    शून्य दर्शय(Ast_ग्रन्थिः* nodep) override { iterateChildren(nodep); }

सार्वजनिक:
    // CONSTRUCTORS
    स्पष्ट TraceVisitor(AstNetlist* nodep)
        : m_alwaysVtxp{नूतन TraceActivityVertex{&m_graph, TraceActivityVertex::ACTIVITY_ALWAYS}} {
        nodep->user2_Clear_वृक्षः();  // TraceDecl multiple activities flag
        nodep->user3_Clear_वृक्षः();  // TraceDecl TraceActivityVertex (assumes we start at nullptr)
        iterate(nodep);
        nodep->foreach([](AstTraceDecl* ध्रुव declp) {
            यदि (declp->अन्तर्_Dtype_Func()) {
                declp->valuep()->unlink_Fr_पृष्ठभागः()->delete_वृक्षः();
                declp->valuep(शून्यसूचकः);
            }
        });
    }
    ~TraceVisitor() override {
        V3Stats::सङ्कलय_Stat("Tracing, Activity setters", m_statSetters);
        V3Stats::सङ्कलय_Stat("Tracing, Activity slow blocks", m_statSettersSlow);
        V3Stats::सङ्कलय_Stat("Tracing, Unique trace codes", m_संहिता);
        V3Stats::सङ्कलय_Stat("Tracing, Unique traced signals", m_statUniqSigs);
    }
};

//######################################################################
// Trace class functions

शून्य V3Trace::traceAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { TraceVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_वृक्षः("trace", 0, dump_वृक्षः_Either_Level() >= 3);
}
