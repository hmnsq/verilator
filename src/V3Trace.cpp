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

جماعت Trace_Activity_راس final : عوامی V3_گراف_راس {
    VL_RTTI_IMPL(Trace_Activity_راس, V3_گراف_راس)
    Ast_گرہ* ثابت m_insertp;
    int32_t m_activity_رمز;
    منطقی m_slow;  // If always slow, we can use the same code
عوامی:
    شمار { ACTIVITY_NEVER = ((1UL << 31) - 1) };
    شمار { ACTIVITY_ALWAYS = ((1UL << 31) - 2) };
    شمار { ACTIVITY_SLOW = 0 };
    Trace_Activity_راس(V3_گراف* graphp, Ast_گرہ* nodep, منطقی slow)
        : V3_گراف_راس{graphp}
        , m_insertp{nodep} {
        m_activity_رمز = 0;
        m_slow = slow;
    }
    Trace_Activity_راس(V3_گراف* graphp, int32_t رمز)
        : V3_گراف_راس{graphp}
        , m_insertp{خالی_اشاریہ} {
        m_activity_رمز = رمز;
        m_slow = جھوٹ;
    }
    ~Trace_Activity_راس() override = طےشدہ;
    // ACCESSORS
    Ast_گرہ* insertp() ثابت {
        UASSERT(m_insertp, "Null insertp; probably called on a special always/slow");
        واپس m_insertp;
    }
    string name() ثابت override {
        اگر (activityAlways()) {
            واپس "*ALWAYS*";
        } ورنہ {
            واپس std::string{slow() ? "*SLOW* " : ""} + insertp()->name();
        }
    }
    string dotColor() ثابت override { واپس slow() ? "yellowGreen" : "green"; }
    int32_t activity_رمز() ثابت { واپس m_activity_رمز; }
    منطقی activityAlways() ثابت { واپس activity_رمز() == ACTIVITY_ALWAYS; }
    منطقی activitySlow() ثابت { واپس activity_رمز() == ACTIVITY_SLOW; }
    خالی activity_رمز(int32_t رمز) { m_activity_رمز = رمز; }
    منطقی slow() ثابت { واپس m_slow; }
    خالی slow(منطقی flag) {
        اگر (!flag) m_slow = جھوٹ;
    }
};

جماعت Trace_CFunc_راس final : عوامی V3_گراف_راس {
    VL_RTTI_IMPL(Trace_CFunc_راس, V3_گراف_راس)
    AstCFunc* ثابت m_nodep;

عوامی:
    Trace_CFunc_راس(V3_گراف* graphp, AstCFunc* nodep)
        : V3_گراف_راس{graphp}
        , m_nodep{nodep} {}
    ~Trace_CFunc_راس() override = طےشدہ;
    // ACCESSORS
    AstCFunc* nodep() ثابت { واپس m_nodep; }
    string name() ثابت override { واپس nodep()->name(); }
    string dotColor() ثابت override { واپس "yellow"; }
    File_سطر* fileline() ثابت override { واپس nodep()->fileline(); }
};

جماعت Trace_Trace_راس final : عوامی V3_گراف_راس {
    VL_RTTI_IMPL(Trace_Trace_راس, V3_گراف_راس)
    AstTraceDecl* ثابت m_nodep;  // TRACEINC this represents
    // nullptr, or other vertex with the real code() that duplicates this one
    Trace_Trace_راس* m_duplicatep = خالی_اشاریہ;
    // When aliasing to a dtype parent, offset of the member within the dtype
    uint32_t m_dtype_Alias_انحراف = 0;

عوامی:
    Trace_Trace_راس(V3_گراف* graphp, AstTraceDecl* nodep)
        : V3_گراف_راس{graphp}
        , m_nodep{nodep} {}
    ~Trace_Trace_راس() override = طےشدہ;
    // ACCESSORS
    AstTraceDecl* nodep() ثابت { واپس m_nodep; }
    string name() ثابت override { واپس nodep()->name(); }
    string dotColor() ثابت override { واپس "red"; }
    File_سطر* fileline() ثابت override { واپس nodep()->fileline(); }
    Trace_Trace_راس* duplicatep() ثابت { واپس m_duplicatep; }
    خالی duplicatep(Trace_Trace_راس* dupp) {
        UASSERT_OBJ(!duplicatep(), nodep(), "Assigning duplicatep() to already duplicated node");
        m_duplicatep = dupp;
    }
    خالی redirectDuplicatep(Trace_Trace_راس* dupp) { m_duplicatep = dupp; }
    uint32_t dtype_Alias_انحراف() ثابت { واپس m_dtype_Alias_انحراف; }
    خالی dtype_Alias_انحراف(uint32_t انحراف) { m_dtype_Alias_انحراف = انحراف; }
};

جماعت Trace_Var_راس final : عوامی V3_گراف_راس {
    VL_RTTI_IMPL(Trace_Var_راس, V3_گراف_راس)
    AstVarScope* ثابت m_nodep;

عوامی:
    Trace_Var_راس(V3_گراف* graphp, AstVarScope* nodep)
        : V3_گراف_راس{graphp}
        , m_nodep{nodep} {}
    ~Trace_Var_راس() override = طےشدہ;
    // ACCESSORS
    AstVarScope* nodep() ثابت { واپس m_nodep; }
    string name() ثابت override { واپس nodep()->name(); }
    string dotColor() ثابت override { واپس "skyblue"; }
    File_سطر* fileline() ثابت override { واپس nodep()->fileline(); }
};

//######################################################################
// Trace state, as a visitor of each AstNode

جماعت TraceVisitor final : عوامی VNVisitor {
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
    ثابت VNUser1_اندر_Use m_inuser1;
    ثابت VNUser2_اندر_Use m_inuser2;
    ثابت VNUser3_اندر_Use m_inuser3;
    // VNUser4InUse     In V3Hasher via V3DupFinder

    // STATE
    Ast_گرہ_Module* m_چوٹی_Modp = خالی_اشاریہ;  // Module to add variables to
    AstScope* ثابت m_چوٹی_Scopep = v3Global.rootp()->چوٹی_Scopep()->scopep();  // The top AstScope
    AstCFunc* m_cfuncp = خالی_اشاریہ;  // C function adding to graph
    AstCFunc* m_regFuncp = خالی_اشاریہ;  // Trace registration function
    AstCFunc* m_actAllFuncp = خالی_اشاریہ;  // Set all activity function
    AstTraceDecl* m_tracep = خالی_اشاریہ;  // Trace function adding to graph
    AstVarScope* m_activityVscp = خالی_اشاریہ;  // Activity variable
    uint32_t m_activity_نمبر = 0;  // Count of fields in activity variable
    uint32_t m_رمز = 0;  // Trace ident code# being assigned
    V3_گراف m_گراف;  // Var/CFunc tracking
    Trace_Activity_راس* ثابت m_alwaysVtxp;  // "Always trace" vertex
    منطقی m_finding = جھوٹ;  // Pass one of algorithm?
    ساخت DtypeFuncs final {
    عوامی:
        AstCFunc* بھرا_Funcp = خالی_اشاریہ;
        AstCFunc* chgFuncp = خالی_اشاریہ;
    };
    std::unordered_map<ثابت Ast_گرہ_DType*, DtypeFuncs>
        m_dtypeNonConstFuncs;  // Full / Chg funcs per type
    std::unordered_map<ثابت Ast_گرہ_DType*, AstCFunc*> m_dtypeConstFuncs;  // Const func per type
    V3_Unique_نام m_dtype_نام{""};  // Unique type func names

    // Trace parallelism. Only VCD tracing can be parallelized at this time.
    ثابت uint32_t m_parallelism
        = v3Global.opt.useTraceParallel() ? static_cast<uint32_t>(v3Global.opt.دھاگے()) : 1;

    VDouble0 m_statSetters;  // Statistic tracking
    VDouble0 m_statSettersSlow;  // Statistic tracking
    VDouble0 m_statUniqSigs;  // Statistic tracking

    // All activity numbers applying to a given trace
    استعمال Act_رمز_سیٹ = std::سیٹ<uint32_t>;
    // For activity set, what traces apply
    استعمال TraceVec = std::multimap<Act_رمز_سیٹ, Trace_Trace_راس*>;
    // Candidate interface-member VarScopes keyed by (interface type, member name)
    std::map<std::pair<ثابت AstIface*, std::string>, std::سمتیہ<AstVarScope*>>
        m_ifaceMemberVscps;

    جماعت TraceInitDeclCollector final : عوامی VNVisitor {
        std::سمتیہ<AstTraceDecl*>& m_declps;  // Output: trace declarations found so far
        std::سیٹ<ثابت AstCFunc*> m_نظر_آیا_Funcps;

        خالی دورہ(AstTraceDecl* nodep) override { m_declps.ڈالو_عقب(nodep); }
        خالی دورہ(AstCCall* nodep) override {
            اگر (AstCFunc* ثابت funcp = nodep->funcp()) collect(funcp);
        }
        خالی دورہ(Ast_گرہ* nodep) override { iterateChildren(nodep); }

    عوامی:
        صریح TraceInitDeclCollector(std::سمتیہ<AstTraceDecl*>& declps)
            : m_declps{declps} {}
        خالی collect(AstCFunc* funcp) {
            اگر (funcp && m_نظر_آیا_Funcps.داخل_کرو(funcp).دوسرا) iterate(funcp);
        }
    };

    // METHODS

    ساکن منطقی same_جڑ_Init_Alias(ثابت AstTraceDecl* جڑ_Declp, ثابت AstTraceDecl* چوٹی_Declp) {
        ثابت VNumRange& جڑ_ثنائی_ہندسہ_Range = جڑ_Declp->ثنائی_ہندسہ_Range();
        ثابت VNumRange& چوٹی_ثنائی_ہندسہ_Range = چوٹی_Declp->ثنائی_ہندسہ_Range();
        واپس جڑ_Declp->showname() == چوٹی_Declp->showname()
               && جڑ_Declp->declDirection() == چوٹی_Declp->declDirection()
               && جڑ_Declp->چوڑائی_Min() == چوٹی_Declp->چوڑائی_Min()
               && جڑ_ثنائی_ہندسہ_Range.ranged() == چوٹی_ثنائی_ہندسہ_Range.ranged()
               && (!جڑ_ثنائی_ہندسہ_Range.ranged()
                   || (جڑ_ثنائی_ہندسہ_Range.بائیں() == چوٹی_ثنائی_ہندسہ_Range.بائیں()
                       && جڑ_ثنائی_ہندسہ_Range.دائیں() == چوٹی_ثنائی_ہندسہ_Range.دائیں()));
    }

    خالی detectDuplicates() {
        UINFO(9, "Finding duplicates");
        // Note uses user4
        V3DupFinder dupFinder;  // Duplicate code detection

        // Compute member offsets within dtype instances incrementally.
        // For each dtype member TraceDecl, stores its code offset within the
        // dtype parent's code range.
        std::unordered_map<ثابت AstTraceDecl*, uint32_t> memberOffsets;
        std::unordered_map<ثابت AstTraceDecl*, uint32_t> running_انحراف;

        // For fixup pass: track dups redirected to dtype parents, so we can
        // fix them if the parent's user2 is later set during the loop.
        // Maps dup vertex -> original canonical member vertex.
        std::سمتیہ<std::pair<Trace_Trace_راس*, Trace_Trace_راس*>> dtype_والد_Redirects;

        // Hash all of the traced values and find if there are any duplicates
        برائے (V3_گراف_راس& vtx : m_گراف.vertices()) {
            اگر (Trace_Trace_راس* ثابت vvertexp = vtx.cast<Trace_Trace_راس>()) {
                AstTraceDecl* ثابت nodep = vvertexp->nodep();
                اگر (nodep->dtypeCallp()) جاری;

                اگر (nodep->dtypeDeclp()) {
                    uint32_t& انحراف = running_انحراف[nodep->dtypeDeclp()];
                    memberOffsets[nodep] = انحراف;
                    انحراف += nodep->رمز_Inc();
                }

                UASSERT_OBJ(!vvertexp->duplicatep(), nodep, "Should not be a duplicate");
                ثابت خودکار dupit = dupFinder.ڈھونڈو_Duplicate(nodep);
                اگر (dupit == dupFinder.انتہا()) {
                    dupFinder.داخل_کرو(nodep);
                } ورنہ {
                    AstTraceDecl* ثابت dupDeclp = VN_AS(dupit->دوسرا, TraceDecl);
                    UASSERT_OBJ(dupDeclp, nodep, "Trace duplicate of wrong type");
                    Trace_Trace_راس* ثابت dupvertexp
                        = dupDeclp->user1u().مقصد_گراف_راس()->cast<Trace_Trace_راس>();
                    UINFO(8, "  Orig " << dupDeclp << سطر_ختم);
                    UINFO(8, "   dup " << nodep << سطر_ختم);

                    اگر (dupDeclp->dtypeDeclp() && !dupDeclp->dtypeDeclp()->user2()) {
                        AstTraceDecl* ثابت dtypeParentp = dupDeclp->dtypeDeclp();
                        Trace_Trace_راس* ثابت dtypeVtxp
                            = dtypeParentp->user1u().مقصد_گراف_راس()->cast<Trace_Trace_راس>();
                        ثابت خودکار it2 = memberOffsets.ڈھونڈو(dupDeclp);
                        UASSERT_OBJ(it2 != memberOffsets.انتہا(), dupDeclp,
                                    "Member offset not precomputed");
                        vvertexp->duplicatep(dtypeVtxp);
                        vvertexp->dtype_Alias_انحراف(it2->دوسرا);
                        dtype_والد_Redirects.emplace_عقب(vvertexp, dupvertexp);
                    } ورنہ {
                        vvertexp->duplicatep(dupvertexp);
                    }
                    اگر (nodep->dtypeDeclp()) nodep->dtypeDeclp()->user2(سچ);
                }
            }
        }

        برائے (ثابت خودکار& pair : dtype_والد_Redirects) {
            Trace_Trace_راس* ثابت dupVtxp = pair.پہلا;
            Trace_Trace_راس* ثابت memberVtxp = pair.دوسرا;
            ثابت AstTraceDecl* ثابت والد_Declp = dupVtxp->duplicatep()->nodep();
            اگر (والد_Declp->user2()) {
                dupVtxp->redirectDuplicatep(memberVtxp);
                dupVtxp->dtype_Alias_انحراف(0);
            }
        }

        اگر (dumpLevel() || debug() >= 9)
            dupFinder.dumpFile(v3Global.debugFilename("trace") + ".hash", جھوٹ);
    }

    خالی گراف_Dtype_Prune() {
        برائے (V3_گراف_راس* ثابت vtxp : m_گراف.vertices().unlinkable()) {
            اگر (Trace_Trace_راس* ثابت vvertexp = vtxp->cast<Trace_Trace_راس>()) {
                AstTraceDecl* ثابت declp = vvertexp->nodep();
                // This skips the dtype sub-func optimization if a var is affected by multiple
                // activities.  We really only need to do this for _chg funcs (and not decls,
                // _const and _full) but it's simpiler to do it all one way or the other.
                اگر (declp) {
                    اگر (declp->user2() || (declp->dtypeDeclp() && !declp->dtypeDeclp()->user2())) {
                        AstCCall* ثابت callp = declp->dtypeCallp();
                        اگر (callp) {
                            Ast_گرہ* stmtexprp = callp->backp();
                            VL_DO_DANGLING(ڈالو_Deletep(stmtexprp->unlink_Fr_عقب()), stmtexprp);
                        } ورنہ {
                            منطقی خالی_خانے_Scope;
                            کرو {
                                خالی_خانے_Scope = جھوٹ;
                                Ast_گرہ* ثابت declBackp = declp->backp();
                                Ast_گرہ* ثابت declNextp = declp->nextp();
                                اگر (VN_IS(declBackp, Trace_ڈالو_سابقہ)
                                    && VN_IS(declNextp, Trace_نکالو_سابقہ)) {
                                    VL_DO_DANGLING(ڈالو_Deletep(declBackp->unlink_Fr_عقب()),
                                                   declBackp);
                                    VL_DO_DANGLING(ڈالو_Deletep(declNextp->unlink_Fr_عقب()),
                                                   declNextp);
                                    خالی_خانے_Scope = سچ;
                                }
                            } جب (خالی_خانے_Scope);
                        }
                        // Can't purge until we finish this pass
                        ڈالو_Deletep(declp->unlink_Fr_عقب());  // declp used below
                        vvertexp->reroute_کنارے(&m_گراف);
                        vvertexp->unlinkDelete(&m_گراف);
                    }
                    declp->dtypeDeclp(خالی_اشاریہ);
                }
            }
        }
    }

    خالی بانٹو_Trace_Decl_Funcs() {
        ثابت صحیح بانٹو_حد_بندی = v3Global.opt.output_بانٹو_CTrace() ? v3Global.opt.output_بانٹو_CTrace()
                                                                : std::numeric_limits<صحیح>::max();
        اگر (بانٹو_حد_بندی == std::numeric_limits<صحیح>::max()) واپس;

        برائے (Ast_گرہ* nodep = m_چوٹی_Scopep->blocksp(); nodep; nodep = nodep->nextp()) {
            AstCFunc* ثابت funcp = VN_CAST(nodep, CFunc);
            اگر (!funcp || !funcp->user2()) جاری;

            ثابت string بنیاد_Name = funcp->name();
            uint32_t sub_عدد = 1;
            صحیح cur_سائز = 0;
            AstCFunc* curFuncp = funcp;
            std::سمتیہ<Ast_گرہ_Stmt*> callStmts;

            خودکار آغاز_New_Sub_Func = [&]() {
                File_سطر* ثابت flp = funcp->fileline();
                ثابت string newName = بنیاد_Name + "_" + cvt_مقصد_Str(sub_عدد++);
                AstCFunc* ثابت newFuncp = نیا AstCFunc{flp, newName, m_چوٹی_Scopep};
                newFuncp->argTypes(v3Global.opt.trace_Class_بنیاد() + "* tracep");
                newFuncp->isTrace(سچ);
                newFuncp->isStatic(جھوٹ);
                newFuncp->isLoose(سچ);
                newFuncp->slow(سچ);
                newFuncp->dontCombine(سچ);
                m_چوٹی_Scopep->جوڑو_Blocksp(newFuncp);
                newFuncp->جوڑو_Stmtsp(نیا AstCStmt{flp, "const int c = vlSymsp->__Vm_baseCode;"});
                AstCCall* ثابت callp = نیا AstCCall{flp, newFuncp};
                callp->dtype_سیٹ_Void();
                callp->argTypes("tracep");
                callStmts.ڈالو_عقب(callp->makeStmt());
                curFuncp = newFuncp;
                cur_سائز = 0;
            };

            برائے (Ast_گرہ *stmtp = funcp->stmtsp(), *nextp; stmtp; stmtp = nextp) {
                nextp = stmtp->nextp();
                cur_سائز += stmtp->گرہ_گنتی();

                اگر (curFuncp != funcp) {
                    stmtp->unlink_Fr_عقب();
                    curFuncp->جوڑو_Stmtsp(stmtp);
                }

                اگر (cur_سائز > بانٹو_حد_بندی && nextp) آغاز_New_Sub_Func();
            }

            برائے (Ast_گرہ_Stmt* ثابت callStmtp : callStmts) { funcp->جوڑو_Stmtsp(callStmtp); }
        }
    }

    خالی گراف_Simplify(منطقی initial) {
        اگر (initial) {
            // Remove all variable nodes
            برائے (V3_گراف_راس* ثابت vtxp : m_گراف.vertices().unlinkable()) {
                اگر (Trace_Var_راس* ثابت vvertexp = vtxp->cast<Trace_Var_راس>()) {
                    vvertexp->reroute_کنارے(&m_گراف);
                    vvertexp->unlinkDelete(&m_گراف);
                }
            }
            // Remove multiple variables connecting funcs to traces
            // We do this twice, as then we have fewer edges to multiply out in the below
            // expansion.
            m_گراف.remove_Redundant_کنارے_Max(&V3_گراف_کنارہ::followAlwaysTrue);
            // Remove all Cfunc nodes
            برائے (V3_گراف_راس* ثابت vtxp : m_گراف.vertices().unlinkable()) {
                اگر (Trace_CFunc_راس* ثابت vvertexp = vtxp->cast<Trace_CFunc_راس>()) {
                    vvertexp->reroute_کنارے(&m_گراف);
                    vvertexp->unlinkDelete(&m_گراف);
                }
            }
        }

        // Remove multiple variables connecting funcs to traces
        m_گراف.remove_Redundant_کنارے_Max(&V3_گراف_کنارہ::followAlwaysTrue);

        // If there are any edges from a always, keep only the always
        برائے (V3_گراف_راس& vtx : m_گراف.vertices()) {
            اگر (Trace_Trace_راس* ثابت vvertexp = vtx.cast<Trace_Trace_راس>()) {
                // Search for the incoming always edge
                ثابت V3_گراف_کنارہ* alwaysEdgep = خالی_اشاریہ;
                برائے (ثابت V3_گراف_کنارہ& کنارہ : vvertexp->اندر_کنارے()) {
                    ثابت Trace_Activity_راس* ثابت actVtxp
                        = کنارہ.fromp()->as<ثابت Trace_Activity_راس>();
                    اگر (actVtxp->activityAlways()) {
                        alwaysEdgep = &کنارہ;
                        توڑو;
                    }
                }
                // If always edge exists, remove all other edges
                اگر (alwaysEdgep) {
                    برائے (V3_گراف_کنارہ* ثابت edgep : vvertexp->اندر_کنارے().unlinkable()) {
                        اگر (edgep != alwaysEdgep) VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
                    }
                }
            }
        }

        // Activity points with no outputs can be removed
        برائے (V3_گراف_راس* ثابت vtxp : m_گراف.vertices().unlinkable()) {
            اگر (Trace_Activity_راس* ثابت aVtxp = vtxp->cast<Trace_Activity_راس>()) {
                // Leave in the always vertex for later use.
                اگر (aVtxp != m_alwaysVtxp && aVtxp->برآمد_خالی_خانے()) {
                    VL_DO_DANGLING(aVtxp->unlinkDelete(&m_گراف), aVtxp);
                }
            }
        }
    }

    uint32_t assignactivityNumbers() {
        uint32_t activity_نمبر = 1;  // Note 0 indicates "slow" only
        برائے (V3_گراف_راس& vtx : m_گراف.vertices()) {
            اگر (Trace_Activity_راس* ثابت vvertexp = vtx.cast<Trace_Activity_راس>()) {
                اگر (vvertexp != m_alwaysVtxp) {
                    اگر (vvertexp->slow()) {
                        vvertexp->activity_رمز(Trace_Activity_راس::ACTIVITY_SLOW);
                    } ورنہ {
                        vvertexp->activity_رمز(activity_نمبر++);
                    }
                }
            }
        }
        واپس activity_نمبر;
    }

    خالی ترتیب_دو_Traces(TraceVec& traces, uint32_t& nNonConstCodes) {
        // Populate sort structure
        traces.clear();
        nNonConstCodes = 0;
        برائے (V3_گراف_راس& vtx : m_گراف.vertices()) {
            اگر (Trace_Trace_راس* ثابت vtxp = vtx.cast<Trace_Trace_راس>()) {
                Act_رمز_سیٹ act_سیٹ;
                UINFO(9, "  Add to sort: " << vtxp);
                UINFOTREE(9, vtxp->nodep(), "", "trnode");
                برائے (ثابت V3_گراف_کنارہ& کنارہ : vtxp->اندر_کنارے()) {
                    ثابت Trace_Activity_راس* ثابت cfvertexp
                        = کنارہ.fromp()->cast<ثابت Trace_Activity_راس>();
                    UASSERT_OBJ(cfvertexp, vtxp->nodep(),
                                "Should have been function pointing to this trace");
                    UINFO(9, "   Activity: " << cfvertexp);
                    اگر (cfvertexp->activityAlways()) {
                        // If code 0, we always trace; ignore other codes
                        act_سیٹ.داخل_کرو(Trace_Activity_راس::ACTIVITY_ALWAYS);
                    } ورنہ {
                        act_سیٹ.داخل_کرو(cfvertexp->activity_رمز());
                    }
                }
                UASSERT_OBJ(act_سیٹ.گنتی(Trace_Activity_راس::ACTIVITY_ALWAYS) == 0
                                || act_سیٹ.سائز() == 1,
                            vtxp->nodep(), "Always active trace has further triggers");
                // Count nodes
                اگر (!vtxp->duplicatep() && !act_سیٹ.خالی_خانے())
                    nNonConstCodes += vtxp->nodep()->رمز_Inc();
                اگر (act_سیٹ.خالی_خانے()) {
                    // If a trace doesn't have activity, it's constant, and we
                    // don't need to track changes on it.
                    act_سیٹ.داخل_کرو(Trace_Activity_راس::ACTIVITY_NEVER);
                } ورنہ اگر (act_سیٹ.گنتی(Trace_Activity_راس::ACTIVITY_SLOW) && act_سیٹ.سائز() > 1) {
                    // If a trace depends on the slow flag as well as other
                    // flags, remove the dependency on the slow flag. We will
                    // make slow routines set all activity flags.
                    act_سیٹ.erase(Trace_Activity_راس::ACTIVITY_SLOW);
                }
                traces.emplace(act_سیٹ, vtxp);
            }
        }
    }

    خالی گراف_Optimize() {
        // Assign initial activity numbers to activity vertices
        assignactivityNumbers();

        // Sort the traces by activity sets
        TraceVec traces;
        uint32_t unused1;
        ترتیب_دو_Traces(traces, unused1);

        // For each activity set with only a small number of signals, make those
        // signals always traced, as it's cheaper to check a few value changes
        // than to test a lot of activity flags
        خودکار it = traces.begin();
        ثابت خودکار انتہا = traces.انتہا();
        جب (it != انتہا) {
            خودکار سر = it;
            // Approximate the complexity of the value change check
            uint32_t complexity = 0;
            ثابت Act_رمز_سیٹ& act_سیٹ = it->پہلا;
            برائے (; it != انتہا && it->پہلا == act_سیٹ; ++it) {
                اگر (!it->دوسرا->duplicatep()) {
                    uint32_t لاگت = 0;
                    ثابت AstTraceDecl* ثابت declp = it->دوسرا->nodep();
                    // The number of comparisons required by bufp->chg*
                    لاگت += declp->isWide() ? declp->رمز_Inc() : 1;
                    // Arrays are traced by element
                    لاگت *= declp->arrayRange().ranged() ? declp->arrayRange().elements() : 1;
                    // Note: Experiments factoring in the size of declp->valuep()
                    // showed no benefit in tracing speed, even for large trees,
                    // so we will leave those out for now.
                    complexity += لاگت;
                }
            }
            // Leave alone always changing, never changing and signals only set in slow code
            اگر (act_سیٹ.گنتی(Trace_Activity_راس::ACTIVITY_ALWAYS)) جاری;
            اگر (act_سیٹ.گنتی(Trace_Activity_راس::ACTIVITY_NEVER)) جاری;
            اگر (act_سیٹ.گنتی(Trace_Activity_راس::ACTIVITY_SLOW)) جاری;
            // If the value comparisons are cheaper to perform than checking the
            // activity flags make the signals always traced. Note this cost
            // equation is heuristic.
            اگر (complexity <= act_سیٹ.سائز() * 2) {
                برائے (; سر != it; ++سر) {
                    نیا V3_گراف_کنارہ{&m_گراف, m_alwaysVtxp, سر->دوسرا, 1};
                }
            }
        }

        گراف_Simplify(جھوٹ);
    }

    Ast_گرہ_اظہاریہ* selectActivity(File_سطر* flp, uint32_t acode, ثابت VAccess& access) {
        واپس نیا AstArraySel(flp, نیا AstVarRef{flp, m_activityVscp, access}, acode);
    }

    Ast_گرہ* newActivitySetter(Ast_گرہ* insertp, uint32_t رمز) {
        ++m_statSetters;
        File_سطر* ثابت fl = insertp->fileline();
        Ast_تفویض* ثابت setterp = نیا Ast_تفویض{fl, selectActivity(fl, رمز, VAccess::WRITE),
                                                 نیا AstConst{fl, AstConst::ثنائی_ہندسہ_True{}}};
        واپس setterp;
    }

    Ast_گرہ* newActivityAll(Ast_گرہ* insertp) {
        ++m_statSettersSlow;
        اگر (!m_actAllFuncp) {
            File_سطر* ثابت flp = m_چوٹی_Scopep->fileline();
            AstCFunc* ثابت funcp = نیا AstCFunc{flp, "__VtraceActivitySetAll", m_چوٹی_Scopep};
            funcp->slow(سچ);
            funcp->isStatic(جھوٹ);
            funcp->isLoose(سچ);
            m_چوٹی_Scopep->جوڑو_Blocksp(funcp);
            برائے (uint32_t رمز = 0; رمز < m_activity_نمبر; ++رمز) {
                Ast_گرہ* ثابت setterp = newActivitySetter(insertp, رمز);
                funcp->جوڑو_Stmtsp(setterp);
            }
            m_actAllFuncp = funcp;
        }
        AstCCall* ثابت callp = نیا AstCCall{insertp->fileline(), m_actAllFuncp};
        callp->dtype_سیٹ_Void();
        واپس callp->makeStmt();
    }

    خالی createActivityFlags() {
        // Assign final activity numbers
        m_activity_نمبر = assignactivityNumbers();

        // Create an array of bytes, not a bit vector, as they can be set
        // atomically by mtasks, and are cheaper to set (no need for
        // read-modify-write on the C type), and the speed of the tracing code
        // is the same on largish designs.
        File_سطر* ثابت flp = m_چوٹی_Scopep->fileline();
        Ast_گرہ_DType* ثابت newScalarDtp = نیا AstBasicDType{flp, VFlag_ثنائی_ہندسہ_Packed{}, 1};
        v3Global.rootp()->typeTablep()->جوڑو_Typesp(newScalarDtp);
        AstRange* ثابت newArange
            = نیا AstRange{flp, VNumRange{static_cast<صحیح>(m_activity_نمبر) - 1, 0}};
        Ast_گرہ_DType* ثابت newArrDtp = نیا AstUnpackArrayDType{flp, newScalarDtp, newArange};
        v3Global.rootp()->typeTablep()->جوڑو_Typesp(newArrDtp);
        m_activityVscp = v3Global.rootp()->چوٹی_Scopep()->create_عارضی("__VtraceActivity", newArrDtp);

        // Insert activity setters
        برائے (ثابت V3_گراف_راس& vtx : m_گراف.vertices()) {
            اگر (ثابت Trace_Activity_راس* ثابت vtxp = vtx.cast<ثابت Trace_Activity_راس>()) {
                Ast_گرہ* setterp = خالی_اشاریہ;
                اگر (vtxp->activitySlow()) {
                    setterp = newActivityAll(vtxp->insertp());
                    // Just set all flags in slow code as it should be rare.
                    // This will be rolled up into a loop by V3Reloop.
                } ورنہ اگر (!vtxp->activityAlways()) {
                    setterp = newActivitySetter(vtxp->insertp(), vtxp->activity_رمز());
                }
                اگر (setterp) {
                    Ast_گرہ* ثابت insertp = vtxp->insertp();
                    اگر (Ast_Stmt_اظہاریہ* ثابت stmtp = VN_CAST(insertp, Stmt_اظہاریہ)) {
                        stmtp->جوڑو_اگلا_یہاں(setterp);
                    } ورنہ اگر (AstCFunc* ثابت funcp = VN_CAST(insertp, CFunc)) {
                        // If there are awaits, insert the setter after each await
                        اگر (funcp->isCoroutine() && funcp->stmtsp()) {
                            funcp->stmtsp()->foreach_And_اگلا([setterp](AstCAwait* awaitp) {
                                awaitp->جوڑو_اگلا_یہاں(setterp->clone_درخت(جھوٹ));
                            });
                        }
                        funcp->جوڑو_Stmtsp(setterp);
                    } ورنہ {
                        insertp->v3fatalSrc("Bad trace activity vertex");
                    }
                }
            }
        }
    }

    AstCFunc* newCFunc(VTraceType traceType, AstCFunc* چوٹی_Funcp, uint32_t func_عدد,
                       uint32_t بنیاد_رمز = 0, ثابت AstTraceDecl* ثابت declp = خالی_اشاریہ,
                       منطقی declSub = جھوٹ) {
        // Create new function
        ثابت منطقی is_چوٹی_Func = !declp && چوٹی_Funcp == خالی_اشاریہ;
        std::string funcName;
        اگر (is_چوٹی_Func || declp) {
            funcName = traceType.func_سابقہ();
        } ورنہ {
            funcName = چوٹی_Funcp->name();
            funcName += "_sub";
        }
        اگر (declp) {
            funcName += "_dtype";
            اگر (declSub) funcName += "_sub";
            funcName += "__";
            funcName = m_dtype_نام.get(funcName);
        } ورنہ {
            funcName += "_";
            funcName += cvt_مقصد_Str(func_عدد);
        }

        File_سطر* ثابت flp = m_چوٹی_Scopep->fileline();
        AstCFunc* ثابت funcp = نیا AstCFunc{flp, funcName, m_چوٹی_Scopep};
        funcp->isTrace(سچ);
        funcp->dontCombine(سچ);
        funcp->isLoose(سچ);
        funcp->slow(traceType != VTraceType::CHANGE);
        funcp->isStatic(is_چوٹی_Func);
        // Add it to top scope
        m_چوٹی_Scopep->جوڑو_Blocksp(funcp);
        ثابت std::string bufArg
            = v3Global.opt.trace_Class_بنیاد() + "::Buffer* bufp"
              + (declp ? (", uint32_t offset, const " + declp->dtypep()->cType("", سچ, سچ)
                          + " __VdtypeVar")
                       : "");
        اگر (is_چوٹی_Func) {
            // Top functions
            funcp->argTypes("void* voidSelf, " + bufArg);
            funcp->جوڑو_Stmtsp(نیا AstCStmt{flp, EmitCUtil::void_Self_تفویض(m_چوٹی_Modp)});
            funcp->جوڑو_Stmtsp(نیا AstCStmt{flp, EmitCUtil::sym_Class_تفویض()});
            // Add global activity check to change dump functions
            اگر (traceType == VTraceType::CHANGE) {  //
                funcp->جوڑو_Stmtsp(
                    نیا AstCStmt{flp, "if (VL_UNLIKELY(!vlSymsp->__Vm_activity)) return;"});
            }
            // Register function
            AstCStmt* ثابت cstmtp = نیا AstCStmt{flp};
            m_regFuncp->جوڑو_Stmtsp(cstmtp);
            اگر (traceType == VTraceType::CONSTANT) {
                cstmtp->جوڑو("tracep->addConstCb(");
                cstmtp->جوڑو(نیا Ast_Addr_کا_CFunc{flp, funcp});
                cstmtp->جوڑو(", " + std::مقصد_string(func_عدد) + ", vlSelf);");
            } ورنہ اگر (traceType == VTraceType::بھرا) {
                cstmtp->جوڑو("tracep->addFullCb(");
                cstmtp->جوڑو(نیا Ast_Addr_کا_CFunc{flp, funcp});
                cstmtp->جوڑو(", " + std::مقصد_string(func_عدد) + ", vlSelf);");
            } ورنہ {
                cstmtp->جوڑو("tracep->addChgCb(");
                cstmtp->جوڑو(نیا Ast_Addr_کا_CFunc{flp, funcp});
                cstmtp->جوڑو(", " + std::مقصد_string(func_عدد) + ", vlSelf);");
            }
        } ورنہ {
            // Sub functions
            funcp->argTypes(bufArg);
            // Setup base references. Note in rare occasions we can end up with an empty trace
            // sub function, hence the VL_ATTR_UNUSED attributes.
            اگر (traceType != VTraceType::CHANGE) {
                // Full dump sub function
                funcp->جوڑو_Stmtsp(نیا AstCStmt{flp,  //
                                              string("uint32_t* const oldp VL_ATTR_UNUSED = "
                                                     "bufp->oldp(vlSymsp->__Vm_baseCode")
                                                  + (declp ? " + offset" : "") + ");\n"});
            } ورنہ {
                // Change dump sub function
                funcp->جوڑو_Stmtsp(نیا AstCStmt{flp,  //
                                              "uint32_t* const oldp VL_ATTR_UNUSED = "
                                              "bufp->oldp(vlSymsp->__Vm_baseCode + "
                                                  + (declp ? " offset" : cvt_مقصد_Str(بنیاد_رمز))
                                                  + ");\n"});
            }
            اگر (!declp) {
                // Add call to top function
                AstCCall* ثابت callp = نیا AstCCall{funcp->fileline(), funcp};
                callp->dtype_سیٹ_Void();
                callp->argTypes("bufp");
                چوٹی_Funcp->جوڑو_Stmtsp(callp->makeStmt());
            }
        }
        //مکمل
        UINFO(5, "  newCFunc " << funcp);
        واپس funcp;
    }

    AstCFunc* newCDtypeSubFunc(VTraceType traceType, ثابت AstTraceDecl* ثابت declp,
                               AstCFunc* parentp) {
        AstCFunc* ثابت funcp = newCFunc(traceType, خالی_اشاریہ, 0, 0, declp, سچ);
        File_سطر* ثابت flp = m_چوٹی_Scopep->fileline();
        AstCCall* ثابت callp = نیا AstCCall{flp, funcp};
        callp->dtype_سیٹ_Void();
        callp->argTypes("bufp, offset, __VdtypeVar");
        parentp->جوڑو_Stmtsp(callp->makeStmt());
        واپس funcp;
    }

    AstCFunc* createConstDtypeTraceFunctions(ثابت AstTraceDecl* declp) {
        ثابت Ast_گرہ_DType* ثابت dtypep = declp->dtypep()->skipRefp();
        خودکار pair = m_dtypeConstFuncs.emplace(dtypep, خالی_اشاریہ);
        اگر (pair.دوسرا) {
            File_سطر* ثابت flp = declp->fileline();
            AstCFunc* ثابت funcp = newCFunc(VTraceType::CONSTANT, خالی_اشاریہ, 0, 0, declp);

            منطقی پہلا = سچ;
            برائے (Ast_گرہ* callStmtp = declp->dtypeCallp()->funcp()->stmtsp(); callStmtp;
                 callStmtp = callStmtp->nextp()) {
                ثابت AstCCall* ثابت callp
                    = VN_CAST(VN_CAST(callStmtp, Stmt_اظہاریہ)->exprp(), CCall);
                منطقی onlyOne = پہلا && !callStmtp->nextp();
                AstCFunc* ثابت subFuncp
                    = onlyOne ? funcp : newCDtypeSubFunc(VTraceType::CONSTANT, declp, funcp);
                برائے (Ast_گرہ* stmtp = callp->funcp()->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    اگر (AstTraceDecl* ثابت میدان_Declp = VN_CAST(stmtp, TraceDecl)) {
                        AstTraceInc* ثابت incp
                            = نیا AstTraceInc{flp, میدان_Declp, VTraceType::CONSTANT};
                        subFuncp->جوڑو_Stmtsp(incp);
                    }
                }
                پہلا = جھوٹ;
            }

            pair.پہلا->دوسرا = funcp;
        }

        واپس pair.پہلا->دوسرا;
    }

    خالی createConstTraceFunctions(ثابت TraceVec& traces) {
        ثابت صحیح بانٹو_حد_بندی = v3Global.opt.output_بانٹو_CTrace() ? v3Global.opt.output_بانٹو_CTrace()
                                                                : std::numeric_limits<صحیح>::max();

        AstCFunc* ثابت چوٹی_Funcp = newCFunc(VTraceType::CONSTANT, خالی_اشاریہ, 0);
        uint32_t sub_Func_عدد = 0;
        AstCFunc* subFuncp = خالی_اشاریہ;
        صحیح subStmts = 0;
        std::سمتیہ<ثابت Trace_Trace_راس*> duplicates;
        برائے (خودکار it = traces.cbegin(); it != traces.انتہا(); ++it) {
            ثابت Trace_Trace_راس* ثابت vtxp = it->دوسرا;
            AstTraceDecl* ثابت declp = vtxp->nodep();
            اگر (vtxp->duplicatep()) {
                duplicates.ڈالو_عقب(vtxp);
                جاری;
            }

            // This is a canonical trace node. Assign trace code (signal number).
            UASSERT_OBJ(!declp->رمز_Assigned(), declp,
                        "Canonical node should not have code assigned yet");
            declp->رمز(m_رمز);
            ثابت uint32_t رمز_Inc = declp->رمز_Inc();
            m_رمز += رمز_Inc;
            ++m_statUniqSigs;

            // If this is a const signal, add the AstTraceInc
            ثابت Act_رمز_سیٹ& act_سیٹ = it->پہلا;
            اگر (act_سیٹ.گنتی(Trace_Activity_راس::ACTIVITY_NEVER)) {
                // Crate new sub function if required
                اگر (!subFuncp || subStmts > بانٹو_حد_بندی) {
                    subStmts = 0;
                    subFuncp = newCFunc(VTraceType::CONSTANT, چوٹی_Funcp, sub_Func_عدد);
                    ++sub_Func_عدد;
                }
                File_سطر* ثابت flp = declp->fileline();
                اگر (declp->dtypeCallp()) {
                    AstCFunc* ثابت funcp = createConstDtypeTraceFunctions(declp);
                    Ast_گرہ_اظہاریہ* argsp = خالی_اشاریہ;
                    argsp = Ast_گرہ::جوڑو_اگلا(argsp, declp->valuep()->clone_درخت(جھوٹ));
                    AstCCall* ثابت callp = نیا AstCCall{flp, funcp, argsp};
                    callp->dtype_سیٹ_Void();
                    callp->argTypes(callp->argTypes() + "bufp, " + std::مقصد_string(declp->رمز()));
                    subFuncp->جوڑو_Stmtsp(callp->makeStmt());

                    subStmts += 1;
                } ورنہ {
                    AstTraceInc* ثابت incp = نیا AstTraceInc{flp, declp, VTraceType::CONSTANT};
                    subFuncp->جوڑو_Stmtsp(incp);
                    subStmts += incp->گرہ_گنتی();
                }
            }
        }

        // Assign codes to duplicate nodes from their canonicals
        برائے (ثابت Trace_Trace_راس* ثابت vtxp : duplicates) {
            AstTraceDecl* ثابت declp = vtxp->nodep();
            ثابت Trace_Trace_راس* ثابت canonVtxp = vtxp->duplicatep();
            ثابت AstTraceDecl* ثابت canonDeclp = canonVtxp->nodep();
            UASSERT_OBJ(!canonVtxp->duplicatep(), canonDeclp, "Canonical node is a duplicate");
            UASSERT_OBJ(canonDeclp->رمز_Assigned(), canonDeclp,
                        "Canonical node should have code assigned already");
            declp->رمز(canonDeclp->رمز() + vtxp->dtype_Alias_انحراف());
        }
    }

    DtypeFuncs createNonConstDtypeTraceFunctions(ثابت AstTraceDecl* declp) {
        Ast_گرہ_DType* dtypep = declp->dtypep()->skipRefp();
        خودکار pair = m_dtypeNonConstFuncs.emplace(dtypep, DtypeFuncs{});
        اگر (pair.دوسرا) {
            File_سطر* ثابت flp = declp->fileline();
            AstCFunc* ثابت بھرا_Funcp = newCFunc(VTraceType::بھرا, خالی_اشاریہ, 0, 0, declp);
            AstCFunc* ثابت chgFuncp = newCFunc(VTraceType::CHANGE, خالی_اشاریہ, 0, 0, declp);

            منطقی پہلا = سچ;
            برائے (Ast_گرہ* callStmtp = declp->dtypeCallp()->funcp()->stmtsp(); callStmtp;
                 callStmtp = callStmtp->nextp()) {
                ثابت AstCCall* ثابت callp
                    = VN_CAST(VN_CAST(callStmtp, Stmt_اظہاریہ)->exprp(), CCall);
                منطقی onlyOne = پہلا && !callStmtp->nextp();
                AstCFunc* ثابت بھرا_Sub_Funcp
                    = onlyOne ? بھرا_Funcp : newCDtypeSubFunc(VTraceType::بھرا, declp, بھرا_Funcp);
                AstCFunc* ثابت chgSubFuncp
                    = onlyOne ? chgFuncp : newCDtypeSubFunc(VTraceType::CHANGE, declp, chgFuncp);
                برائے (Ast_گرہ* stmtp = callp->funcp()->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    اگر (AstTraceDecl* ثابت میدان_Declp = VN_CAST(stmtp, TraceDecl)) {
                        AstTraceInc* ثابت incFullp
                            = نیا AstTraceInc{flp, میدان_Declp, VTraceType::بھرا};
                        بھرا_Sub_Funcp->جوڑو_Stmtsp(incFullp);
                        AstTraceInc* ثابت incChgp
                            = نیا AstTraceInc{flp, میدان_Declp, VTraceType::CHANGE};
                        chgSubFuncp->جوڑو_Stmtsp(incChgp);
                    }
                }
                پہلا = جھوٹ;
            }

            pair.پہلا->دوسرا = {.بھرا_Funcp = بھرا_Funcp, .chgFuncp = chgFuncp};
        }

        واپس pair.پہلا->دوسرا;
    }

    خالی createNonConstTraceFunctions(ثابت TraceVec& traces, uint32_t nAllCodes,
                                      uint32_t parallelism) {
        ثابت صحیح بانٹو_حد_بندی = v3Global.opt.output_بانٹو_CTrace() ? v3Global.opt.output_بانٹو_CTrace()
                                                                : std::numeric_limits<صحیح>::max();

        // pre-incremented, so starts at 0
        uint32_t چوٹی_Func_عدد = std::numeric_limits<uint32_t>::max();
        TraceVec::const_iterator it = traces.begin();
        جب (it != traces.انتہا()) {
            AstCFunc* چوٹی_Ful_Funcp = خالی_اشاریہ;
            AstCFunc* چوٹی_Chg_Funcp = خالی_اشاریہ;
            AstCFunc* subFulFuncp = خالی_اشاریہ;
            AstCFunc* subChgFuncp = خالی_اشاریہ;
            uint32_t sub_Func_عدد = 0;
            صحیح subStmts = 0;
            ثابت uint32_t maxCodes = std::max((nAllCodes + parallelism - 1) / parallelism, 1U);
            uint32_t nCodes = 0;
            ثابت Act_رمز_سیٹ* پچھلا_Act_سیٹ = خالی_اشاریہ;
            AstIf* ifp = خالی_اشاریہ;
            uint32_t بنیاد_رمز = 0;
            برائے (; nCodes < maxCodes && it != traces.انتہا(); ++it) {
                ثابت Act_رمز_سیٹ& act_سیٹ = it->پہلا;
                // Traced value never changes, no need to add it
                اگر (act_سیٹ.گنتی(Trace_Activity_راس::ACTIVITY_NEVER)) جاری;

                ثابت Trace_Trace_راس* ثابت vtxp = it->دوسرا;
                AstTraceDecl* ثابت declp = vtxp->nodep();

                // This is a duplicate decl, no need to add it, but must set the
                // function index to the same as the canonical node.
                اگر (ثابت Trace_Trace_راس* ثابت canonVtxp = vtxp->duplicatep()) {
                    declp->fidx(canonVtxp->nodep()->fidx());
                    جاری;
                }

                // Create top function if not yet created
                اگر (!چوٹی_Ful_Funcp) {
                    ++چوٹی_Func_عدد;
                    چوٹی_Ful_Funcp = newCFunc(VTraceType::بھرا, خالی_اشاریہ, چوٹی_Func_عدد);
                    چوٹی_Chg_Funcp = newCFunc(VTraceType::CHANGE, خالی_اشاریہ, چوٹی_Func_عدد);
                }

                // Create new sub function if required
                اگر (!subFulFuncp || subStmts > بانٹو_حد_بندی) {
                    بنیاد_رمز = declp->رمز();
                    subStmts = 0;
                    subFulFuncp = newCFunc(VTraceType::بھرا, چوٹی_Ful_Funcp, sub_Func_عدد, بنیاد_رمز);
                    subChgFuncp = newCFunc(VTraceType::CHANGE, چوٹی_Chg_Funcp, sub_Func_عدد, بنیاد_رمز);
                    ++sub_Func_عدد;
                    پچھلا_Act_سیٹ = خالی_اشاریہ;
                    ifp = خالی_اشاریہ;
                }

                // If required, create the conditional node checking the activity flags
                اگر (!پچھلا_Act_سیٹ || act_سیٹ != *پچھلا_Act_سیٹ) {
                    File_سطر* ثابت flp = m_چوٹی_Scopep->fileline();
                    ثابت منطقی always = act_سیٹ.گنتی(Trace_Activity_راس::ACTIVITY_ALWAYS) != 0;
                    Ast_گرہ_اظہاریہ* condp = خالی_اشاریہ;
                    اگر (always) {
                        condp = نیا AstConst{flp, 1};  // Always true, will be folded later
                    } ورنہ {
                        برائے (ثابت uint32_t act_رمز : act_سیٹ) {
                            Ast_گرہ_اظہاریہ* ثابت selp = selectActivity(flp, act_رمز, VAccess::READ);
                            condp = condp ? نیا AstOr{flp, condp, selp} : selp;
                        }
                    }
                    ifp = نیا AstIf{flp, condp};
                    اگر (!always) ifp->branchPred(VBranchPred::BP_UNLIKELY);
                    subChgFuncp->جوڑو_Stmtsp(ifp);
                    subStmts += ifp->گرہ_گنتی();
                    پچھلا_Act_سیٹ = &act_سیٹ;
                }

                // Add TraceInc nodes
                File_سطر* ثابت flp = declp->fileline();
                اگر (declp->dtypeCallp()) {
                    DtypeFuncs funcs = createNonConstDtypeTraceFunctions(declp);
                    Ast_گرہ_اظہاریہ* argsp = خالی_اشاریہ;
                    argsp = Ast_گرہ::جوڑو_اگلا(argsp, declp->valuep()->clone_درخت(جھوٹ));
                    AstCCall* ثابت callFullp = نیا AstCCall{flp, funcs.بھرا_Funcp, argsp};
                    callFullp->dtype_سیٹ_Void();
                    callFullp->argTypes(callFullp->argTypes() + "bufp, "
                                        + std::مقصد_string(declp->رمز()));
                    subFulFuncp->جوڑو_Stmtsp(callFullp->makeStmt());
                    argsp = خالی_اشاریہ;
                    argsp = Ast_گرہ::جوڑو_اگلا(argsp, declp->valuep()->clone_درخت(جھوٹ));
                    AstCCall* ثابت callChgp = نیا AstCCall{flp, funcs.chgFuncp, argsp};
                    callChgp->dtype_سیٹ_Void();
                    callChgp->argTypes(callChgp->argTypes() + "bufp, "
                                       + std::مقصد_string(declp->رمز()));
                    ifp->جوڑو_Thensp(callChgp->makeStmt());

                    subStmts += 2;
                } ورنہ {
                    AstTraceInc* ثابت incFulp = نیا AstTraceInc{flp, declp, VTraceType::بھرا};
                    subFulFuncp->جوڑو_Stmtsp(incFulp);
                    AstTraceInc* ثابت incChgp
                        = نیا AstTraceInc{flp, declp, VTraceType::CHANGE, بنیاد_رمز};
                    ifp->جوڑو_Thensp(incChgp);

                    // Set the function index of the decl
                    declp->fidx(چوٹی_Func_عدد);

                    // Track splitting due to size
                    UASSERT_OBJ(incFulp->گرہ_گنتی() == incChgp->گرہ_گنتی(), declp,
                                "Should have equal cost");
                    ثابت VNumRange range = declp->arrayRange();
                    اگر (range.ranged()) {
                        // 2x because each element is a TraceInc and a VarRef
                        subStmts += range.elements() * 2;
                    } ورنہ {
                        subStmts += incChgp->گرہ_گنتی();
                    }
                }

                // Track partitioning
                nCodes += declp->رمز_Inc();
            }
        }
    }

    خالی createCleanupFunction() {
        File_سطر* ثابت fl = m_چوٹی_Scopep->fileline();
        AstCFunc* ثابت cleanupFuncp = نیا AstCFunc{fl, "trace_cleanup", m_چوٹی_Scopep};
        cleanupFuncp->argTypes("void* voidSelf, " + v3Global.opt.trace_Class_بنیاد()
                               + "* /*unused*/");
        cleanupFuncp->isTrace(سچ);
        cleanupFuncp->slow(جھوٹ);
        cleanupFuncp->isStatic(سچ);
        cleanupFuncp->isLoose(سچ);
        m_چوٹی_Scopep->جوڑو_Blocksp(cleanupFuncp);
        cleanupFuncp->جوڑو_Stmtsp(نیا AstCStmt{fl, EmitCUtil::void_Self_تفویض(m_چوٹی_Modp)});
        cleanupFuncp->جوڑو_Stmtsp(نیا AstCStmt{fl, EmitCUtil::sym_Class_تفویض()});

        // Register it
        {
            AstCStmt* ثابت cstmtp = نیا AstCStmt{fl};
            m_regFuncp->جوڑو_Stmtsp(cstmtp);
            cstmtp->جوڑو("tracep->addCleanupCb(");
            cstmtp->جوڑو(نیا Ast_Addr_کا_CFunc{fl, cleanupFuncp});
            cstmtp->جوڑو(", vlSelf);");
        }

        // Clear global activity flag
        cleanupFuncp->جوڑو_Stmtsp(
            نیا AstCStmt{m_چوٹی_Scopep->fileline(), "vlSymsp->__Vm_activity = false;"s});

        // Clear fine grained activity flags
        برائے (uint32_t i = 0; i < m_activity_نمبر; ++i) {
            Ast_گرہ* ثابت clrp = نیا Ast_تفویض{fl, selectActivity(fl, i, VAccess::WRITE),
                                                نیا AstConst{fl, AstConst::ثنائی_ہندسہ_False{}}};
            cleanupFuncp->جوڑو_Stmtsp(clrp);
        }
    }

    خالی createTraceFunctions() {
        // Detect and remove duplicate values
        detectDuplicates();

        گراف_Dtype_Prune();
        بانٹو_Trace_Decl_Funcs();
        m_گراف.remove_Redundant_کنارے_Max(&V3_گراف_کنارہ::followAlwaysTrue);

        // Simplify & optimize the graph
        اگر (dump_گراف_Level() >= 6) m_گراف.dumpDotFilePrefixed("trace_pre");
        گراف_Simplify(سچ);
        اگر (dump_گراف_Level() >= 6) m_گراف.dumpDotFilePrefixed("trace_simplified");
        گراف_Optimize();
        اگر (dump_گراف_Level() >= 6) m_گراف.dumpDotFilePrefixed("trace_optimized");

        // Create the fine grained activity flags
        createActivityFlags();

        // Form a sorted list of the traces we are interested in
        TraceVec traces;  // The sorted traces
        // We will split functions such that each have to dump roughly the same amount of data
        // for this we need to keep tack of the number of codes used by the trace functions.
        uint32_t nNonConstCodes = 0;
        ترتیب_دو_Traces(traces, nNonConstCodes);

        // Our keys are now sorted to have same activity number adjacent, then
        // by trace order. (Better would be execution order for cache
        // efficiency....) Last are constants and non-changers, as then the
        // last value vector is more compact

        // Create the trace registration function
        m_regFuncp = نیا AstCFunc{m_چوٹی_Scopep->fileline(), "trace_register", m_چوٹی_Scopep};
        m_regFuncp->argTypes(v3Global.opt.trace_Class_بنیاد() + "* tracep");
        m_regFuncp->اندراج_نقطہ(سچ);
        m_regFuncp->isTrace(سچ);
        m_regFuncp->slow(سچ);
        m_regFuncp->isStatic(جھوٹ);
        m_regFuncp->isLoose(سچ);
        m_چوٹی_Scopep->جوڑو_Blocksp(m_regFuncp);

        // Create the const dump functions. Also allocates trace codes.
        createConstTraceFunctions(traces);

        // Create the full and incremental dump functions
        createNonConstTraceFunctions(traces, nNonConstCodes, m_parallelism);

        // Root-traced libraries alias wrapper IOs onto the existing top-module codes.
        اگر (!v3Global.opt.libCreate().خالی_خانے()) {
            std::سمتیہ<AstTraceDecl*> جڑ_Declps;
            std::سمتیہ<AstTraceDecl*> چوٹی_Declps;
            TraceInitDeclCollector جڑ_Collector{جڑ_Declps};
            TraceInitDeclCollector چوٹی_Collector{چوٹی_Declps};
            برائے (Ast_گرہ* blockp = m_چوٹی_Scopep->blocksp(); blockp; blockp = blockp->nextp()) {
                AstCFunc* ثابت funcp = VN_CAST(blockp, CFunc);
                اگر (funcp && VString::startsWith(funcp->name(), "trace_init_leaf_root__")) {
                    جڑ_Collector.collect(funcp);
                } ورنہ اگر (funcp && VString::startsWith(funcp->name(), "trace_init_leaf_top__")) {
                    چوٹی_Collector.collect(funcp);
                }
            }

            std::سمتیہ<منطقی> مستعمل(چوٹی_Declps.سائز(), جھوٹ);
            برائے (AstTraceDecl* ثابت جڑ_Declp : جڑ_Declps) {
                برائے (سائز_t i = 0; i < چوٹی_Declps.سائز(); ++i) {
                    اگر (مستعمل[i] || !same_جڑ_Init_Alias(جڑ_Declp, چوٹی_Declps[i])) جاری;
                    جڑ_Declp->رمز(چوٹی_Declps[i]->رمز());
                    مستعمل[i] = سچ;
                    توڑو;
                }
            }
        }

        // Remove refs to traced values from TraceDecl nodes, these have now moved under
        // TraceInc
        برائے (ثابت خودکار& i : traces) {
            Ast_گرہ* ثابت valuep = i.دوسرا->nodep()->valuep();
            VL_DO_DANGLING(valuep->unlink_Fr_عقب()->delete_درخت(), valuep);
        }

        // Create the trace cleanup function clearing the activity flags
        createCleanupFunction();
    }

    Trace_CFunc_راس* getCFuncVertexp(AstCFunc* nodep) {
        V3_گراف_راس* ثابت vtxp = nodep->user1u().مقصد_گراف_راس();
        Trace_CFunc_راس* vertexp = vtxp ? vtxp->cast<Trace_CFunc_راس>() : خالی_اشاریہ;
        اگر (!vertexp) {
            vertexp = نیا Trace_CFunc_راس{&m_گراف, nodep};
            nodep->user1p(vertexp);
        }
        واپس vertexp;
    }
    Trace_Activity_راس* getActivityVertexp(Ast_گرہ* nodep, منطقی slow) {
        V3_گراف_راس* ثابت vtxp = nodep->user3u().مقصد_گراف_راس();
        Trace_Activity_راس* vertexp = vtxp ? vtxp->cast<Trace_Activity_راس>() : خالی_اشاریہ;
        اگر (!vertexp) {
            vertexp = نیا Trace_Activity_راس{&m_گراف, nodep, slow};
            nodep->user3p(vertexp);
        }
        vertexp->slow(slow);
        واپس vertexp;
    }

    // VISITORS
    خالی دورہ(AstNetlist* nodep) override {
        // Add vertexes for all TraceDecl, and edges from VARs each trace looks at
        m_finding = جھوٹ;
        iterateChildren(nodep);

        // Add vertexes for all CFUNCs, and edges to VARs the func sets
        m_finding = سچ;
        iterateChildren(nodep);

        // Create the trace functions and insert them into the tree
        createTraceFunctions();

        // Save number of trace codes used
        nodep->nTraceCodes(m_رمز);
    }
    خالی دورہ(Ast_گرہ_Module* nodep) override {
        اگر (nodep->is_چوٹی()) m_چوٹی_Modp = nodep;
        iterateChildren(nodep);
    }
    خالی دورہ(AstVarScope* nodep) override {
        اگر (!m_finding) {
            اگر (ثابت AstIface* ثابت ifacep = nodep->varp()->sensIfacep()) {
                m_ifaceMemberVscps[{ifacep, nodep->varp()->name()}].ڈالو_عقب(nodep);
            }
        }
    }
    خالی دورہ(Ast_Stmt_اظہاریہ* nodep) override {
        اگر (!m_finding && !nodep->user2()) {
            اگر (AstCCall* ثابت callp = VN_CAST(nodep->exprp(), CCall)) {
                UINFO(8, "   CCALL " << callp);
                // See if there are other calls in same statement list;
                // If so, all funcs might share the same activity code
                Trace_Activity_راس* ثابت activityVtxp
                    = getActivityVertexp(nodep, callp->funcp()->slow());
                برائے (Ast_گرہ* nextp = nodep; nextp; nextp = nextp->nextp()) {
                    اگر (Ast_Stmt_اظہاریہ* ثابت stmtp = VN_CAST(nextp, Stmt_اظہاریہ)) {
                        اگر (AstCCall* ثابت ccallp = VN_CAST(stmtp->exprp(), CCall)) {
                            stmtp->user2(سچ);  // Processed
                            UINFO(8, "     SubCCALL " << ccallp);
                            V3_گراف_راس* ثابت ccallFuncVtxp = getCFuncVertexp(ccallp->funcp());
                            activityVtxp->slow(ccallp->funcp()->slow());
                            نیا V3_گراف_کنارہ{&m_گراف, activityVtxp, ccallFuncVtxp, 1};
                        }
                    }
                }
            }
        }
        iterateChildren(nodep);
    }
    خالی دورہ(AstCFunc* nodep) override {
        UINFO(8, "   CFUNC " << nodep);
        V3_گراف_راس* ثابت funcVtxp = getCFuncVertexp(nodep);
        اگر (!m_finding) {  // If public, we need a unique activity code to allow for sets
                           // directly in this func
            اگر (nodep->func_عوامی() || nodep->dpiExportImpl() || nodep->اندراج_نقطہ()
                || nodep->isCoroutine()) {
                // Cannot treat a coroutine as slow, it may be resumed later
                ثابت منطقی slow = nodep->slow() && !nodep->isCoroutine();
                Trace_Activity_راس* ثابت activityVtxp = getActivityVertexp(nodep, slow);
                نیا V3_گراف_کنارہ{&m_گراف, activityVtxp, funcVtxp, 1};
            }
        }
        VL_RESTORER(m_cfuncp);
        m_cfuncp = nodep;
        iterateChildren(nodep);
    }
    خالی دورہ(AstTraceDecl* nodep) override {
        UINFO(8, "   TRACE " << nodep);
        اگر (!m_finding && !nodep->اندر_Dtype_Func()) {
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
                واپس;
            }
            V3_گراف_راس* ثابت vertexp = نیا Trace_Trace_راس{&m_گراف, nodep};
            nodep->user1p(vertexp);

            UASSERT_OBJ(m_cfuncp, nodep, "Trace not under func");
            m_cfuncp->user2(سچ);
            VL_RESTORER(m_tracep);
            m_tracep = nodep;
            iterateChildren(nodep);
        }
    }
    خالی دورہ(AstVarRef* nodep) override {
        UASSERT_OBJ(nodep->varScopep(), nodep, "No var scope?");
        AstVarScope* ثابت varscopep = nodep->varScopep();
        V3_گراف_راس* varVtxp = varscopep->user1u().مقصد_گراف_راس();
        اگر (m_tracep) {
            UASSERT_OBJ(nodep->access().isReadOnly(), nodep, "Lvalue in trace?  Should be const.");
            اگر (!varVtxp) {
                varVtxp = نیا Trace_Var_راس{&m_گراف, nodep->varScopep()};
                nodep->varScopep()->user1p(varVtxp);
            }
            V3_گراف_راس* ثابت traceVtxp = m_tracep->user1u().مقصد_گراف_راس();
            نیا V3_گراف_کنارہ{&m_گراف, varVtxp, traceVtxp, 1};
            اگر (nodep->varp()->isPrimaryInish()  // Always need to trace primary inputs
                || nodep->varp()->is_Sig_عوامی()) {  // Or ones user can change
                نیا V3_گراف_کنارہ{&m_گراف, m_alwaysVtxp, traceVtxp, 1};
            }
            اگر (m_tracep->dtypeCallp()) varscopep->user2p(m_tracep);
        } ورنہ اگر (m_cfuncp && m_finding && nodep->access().isWriteOrRW()) {
            V3_گراف_راس* ثابت funcVtxp = getCFuncVertexp(m_cfuncp);
            اگر (varVtxp) {  // else we're not tracing this signal
                نیا V3_گراف_کنارہ{&m_گراف, funcVtxp, varVtxp, 1};
                AstTraceDecl* ثابت declp = VN_AS(varscopep->user2p(), TraceDecl);
                اگر (declp) {
                    V3_گراف_راس* ثابت cFuncVtxp = getCFuncVertexp(m_cfuncp);
                    برائے (ثابت V3_گراف_کنارہ& کنارہ : cFuncVtxp->اندر_کنارے()) {
                        V3_گراف_راس* ثابت activityp = کنارہ.fromp();
                        اگر (!declp->user3p()) {
                            declp->user3p(activityp);
                        } ورنہ اگر (declp->user3u().مقصد_گراف_راس() != activityp) {
                            declp->user2(سچ);
                        }
                    }
                }
            }
        }
    }
    خالی دورہ(AstMemberSel* nodep) override {
        اگر (m_cfuncp && m_finding && nodep->access().isWriteOrRW()) {
            AstIfaceRefDType* ثابت dtypep
                = VN_CAST(nodep->fromp()->dtypep()->skipRefp(), IfaceRefDType);
            اگر (dtypep && dtypep->isVirtual()) {
                ثابت خودکار it = m_ifaceMemberVscps.ڈھونڈو({dtypep->ifacep(), nodep->varp()->name()});
                اگر (it != m_ifaceMemberVscps.انتہا()) {
                    V3_گراف_راس* ثابت funcVtxp = getCFuncVertexp(m_cfuncp);
                    برائے (AstVarScope* ثابت vscp : it->دوسرا) {
                        V3_گراف_راس* varVtxp = vscp->user1u().مقصد_گراف_راس();
                        اگر (!varVtxp) {
                            varVtxp = نیا Trace_Var_راس{&m_گراف, vscp};
                            vscp->user1p(varVtxp);
                        }
                        نیا V3_گراف_کنارہ{&m_گراف, funcVtxp, varVtxp, 1};
                    }
                }
            }
        }
        iterateChildren(nodep);
    }
    //--------------------
    خالی دورہ(Ast_گرہ* nodep) override { iterateChildren(nodep); }

عوامی:
    // CONSTRUCTORS
    صریح TraceVisitor(AstNetlist* nodep)
        : m_alwaysVtxp{نیا Trace_Activity_راس{&m_گراف, Trace_Activity_راس::ACTIVITY_ALWAYS}} {
        nodep->user2_Clear_درخت();  // TraceDecl multiple activities flag
        nodep->user3_Clear_درخت();  // TraceDecl TraceActivityVertex (assumes we start at nullptr)
        iterate(nodep);
        nodep->foreach([](AstTraceDecl* ثابت declp) {
            اگر (declp->اندر_Dtype_Func()) {
                declp->valuep()->unlink_Fr_عقب()->delete_درخت();
                declp->valuep(خالی_اشاریہ);
            }
        });
    }
    ~TraceVisitor() override {
        V3Stats::جوڑو_Stat("Tracing, Activity setters", m_statSetters);
        V3Stats::جوڑو_Stat("Tracing, Activity slow blocks", m_statSettersSlow);
        V3Stats::جوڑو_Stat("Tracing, Unique trace codes", m_رمز);
        V3Stats::جوڑو_Stat("Tracing, Unique traced signals", m_statUniqSigs);
    }
};

//######################################################################
// Trace class functions

خالی V3Trace::traceAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { TraceVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_درخت("trace", 0, dump_درخت_Either_Level() >= 3);
}
