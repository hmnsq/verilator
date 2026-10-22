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

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "V3Trace.h"

#ضمن "V3Ast.h"
#ضمن "V3DupFinder.h"
#ضمن "V3EmitCBase.h"
#ضمن "V3Graph.h"
#ضمن "V3Stats.h"
#ضمن "V3UniqueNames.h"

#ضمن <limits>
#ضمن <map>
#ضمن <set>
#ضمن <unordered_map>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Graph vertexes

صنف Trace_Activity_رأس final : عام V3_گراف_رأس {
    VL_RTTI_IMPL(Trace_Activity_رأس, V3_گراف_رأس)
    Ast_گره* ثابت m_insertp;
    int32_t m_activity_کد;
    منطقي m_slow;  // If always slow, we can use the same code
عام:
    تعداد { ACTIVITY_NEVER = ((1UL << 31) - 1) };
    تعداد { ACTIVITY_ALWAYS = ((1UL << 31) - 2) };
    تعداد { ACTIVITY_SLOW = 0 };
    Trace_Activity_رأس(V3_گراف* graphp, Ast_گره* nodep, منطقي slow)
        : V3_گراف_رأس{graphp}
        , m_insertp{nodep} {
        m_activity_کد = 0;
        m_slow = slow;
    }
    Trace_Activity_رأس(V3_گراف* graphp, int32_t کد)
        : V3_گراف_رأس{graphp}
        , m_insertp{اشاره_تهی} {
        m_activity_کد = کد;
        m_slow = خطأ;
    }
    ~Trace_Activity_رأس() override = افتراضي;
    // ACCESSORS
    Ast_گره* insertp() ثابت {
        UASSERT(m_insertp, "Null insertp; probably called on a special always/slow");
        أرجع m_insertp;
    }
    string name() ثابت override {
        إذا (activityAlways()) {
            أرجع "*ALWAYS*";
        } وإلا {
            أرجع std::string{slow() ? "*SLOW* " : ""} + insertp()->name();
        }
    }
    string dotColor() ثابت override { أرجع slow() ? "yellowGreen" : "green"; }
    int32_t activity_کد() ثابت { أرجع m_activity_کد; }
    منطقي activityAlways() ثابت { أرجع activity_کد() == ACTIVITY_ALWAYS; }
    منطقي activitySlow() ثابت { أرجع activity_کد() == ACTIVITY_SLOW; }
    فراغ activity_کد(int32_t کد) { m_activity_کد = کد; }
    منطقي slow() ثابت { أرجع m_slow; }
    فراغ slow(منطقي flag) {
        إذا (!flag) m_slow = خطأ;
    }
};

صنف Trace_CFunc_رأس final : عام V3_گراف_رأس {
    VL_RTTI_IMPL(Trace_CFunc_رأس, V3_گراف_رأس)
    AstCFunc* ثابت m_nodep;

عام:
    Trace_CFunc_رأس(V3_گراف* graphp, AstCFunc* nodep)
        : V3_گراف_رأس{graphp}
        , m_nodep{nodep} {}
    ~Trace_CFunc_رأس() override = افتراضي;
    // ACCESSORS
    AstCFunc* nodep() ثابت { أرجع m_nodep; }
    string name() ثابت override { أرجع nodep()->name(); }
    string dotColor() ثابت override { أرجع "yellow"; }
    File_سطر* fileline() ثابت override { أرجع nodep()->fileline(); }
};

صنف Trace_Trace_رأس final : عام V3_گراف_رأس {
    VL_RTTI_IMPL(Trace_Trace_رأس, V3_گراف_رأس)
    AstTraceDecl* ثابت m_nodep;  // TRACEINC this represents
    // nullptr, or other vertex with the real code() that duplicates this one
    Trace_Trace_رأس* m_duplicatep = اشاره_تهی;
    // When aliasing to a dtype parent, offset of the member within the dtype
    uint32_t m_dtype_Alias_جابه_جایی = 0;

عام:
    Trace_Trace_رأس(V3_گراف* graphp, AstTraceDecl* nodep)
        : V3_گراف_رأس{graphp}
        , m_nodep{nodep} {}
    ~Trace_Trace_رأس() override = افتراضي;
    // ACCESSORS
    AstTraceDecl* nodep() ثابت { أرجع m_nodep; }
    string name() ثابت override { أرجع nodep()->name(); }
    string dotColor() ثابت override { أرجع "red"; }
    File_سطر* fileline() ثابت override { أرجع nodep()->fileline(); }
    Trace_Trace_رأس* duplicatep() ثابت { أرجع m_duplicatep; }
    فراغ duplicatep(Trace_Trace_رأس* dupp) {
        UASSERT_OBJ(!duplicatep(), nodep(), "Assigning duplicatep() to already duplicated node");
        m_duplicatep = dupp;
    }
    فراغ redirectDuplicatep(Trace_Trace_رأس* dupp) { m_duplicatep = dupp; }
    uint32_t dtype_Alias_جابه_جایی() ثابت { أرجع m_dtype_Alias_جابه_جایی; }
    فراغ dtype_Alias_جابه_جایی(uint32_t جابه_جایی) { m_dtype_Alias_جابه_جایی = جابه_جایی; }
};

صنف Trace_Var_رأس final : عام V3_گراف_رأس {
    VL_RTTI_IMPL(Trace_Var_رأس, V3_گراف_رأس)
    AstVarScope* ثابت m_nodep;

عام:
    Trace_Var_رأس(V3_گراف* graphp, AstVarScope* nodep)
        : V3_گراف_رأس{graphp}
        , m_nodep{nodep} {}
    ~Trace_Var_رأس() override = افتراضي;
    // ACCESSORS
    AstVarScope* nodep() ثابت { أرجع m_nodep; }
    string name() ثابت override { أرجع nodep()->name(); }
    string dotColor() ثابت override { أرجع "skyblue"; }
    File_سطر* fileline() ثابت override { أرجع nodep()->fileline(); }
};

//######################################################################
// Trace state, as a visitor of each AstNode

صنف TraceVisitor final : عام VNVisitor {
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
    ثابت VNUser1_درون_Use m_inuser1;
    ثابت VNUser2_درون_Use m_inuser2;
    ثابت VNUser3_درون_Use m_inuser3;
    // VNUser4InUse     In V3Hasher via V3DupFinder

    // STATE
    Ast_گره_Module* m_بالاترین_Modp = اشاره_تهی;  // Module to add variables to
    AstScope* ثابت m_بالاترین_Scopep = v3Global.rootp()->بالاترین_Scopep()->scopep();  // The top AstScope
    AstCFunc* m_cfuncp = اشاره_تهی;  // C function adding to graph
    AstCFunc* m_regFuncp = اشاره_تهی;  // Trace registration function
    AstCFunc* m_actAllFuncp = اشاره_تهی;  // Set all activity function
    AstTraceDecl* m_tracep = اشاره_تهی;  // Trace function adding to graph
    AstVarScope* m_activityVscp = اشاره_تهی;  // Activity variable
    uint32_t m_activity_عدد = 0;  // Count of fields in activity variable
    uint32_t m_کد = 0;  // Trace ident code# being assigned
    V3_گراف m_گراف;  // Var/CFunc tracking
    Trace_Activity_رأس* ثابت m_alwaysVtxp;  // "Always trace" vertex
    منطقي m_finding = خطأ;  // Pass one of algorithm?
    بنية DtypeFuncs final {
    عام:
        AstCFunc* پر_Funcp = اشاره_تهی;
        AstCFunc* chgFuncp = اشاره_تهی;
    };
    std::unordered_map<ثابت Ast_گره_DType*, DtypeFuncs>
        m_dtypeNonConstFuncs;  // Full / Chg funcs per type
    std::unordered_map<ثابت Ast_گره_DType*, AstCFunc*> m_dtypeConstFuncs;  // Const func per type
    V3_Unique_نامها m_dtype_نامها{""};  // Unique type func names

    // Trace parallelism. Only VCD tracing can be parallelized at this time.
    ثابت uint32_t m_parallelism
        = v3Global.opt.useTraceParallel() ? static_cast<uint32_t>(v3Global.opt.نخها()) : 1;

    VDouble0 m_statSetters;  // Statistic tracking
    VDouble0 m_statSettersSlow;  // Statistic tracking
    VDouble0 m_statUniqSigs;  // Statistic tracking

    // All activity numbers applying to a given trace
    استفاده Act_کد_مجموعه = std::مجموعه<uint32_t>;
    // For activity set, what traces apply
    استفاده TraceVec = std::multimap<Act_کد_مجموعه, Trace_Trace_رأس*>;
    // Candidate interface-member VarScopes keyed by (interface type, member name)
    std::map<std::pair<ثابت AstIface*, std::string>, std::وکتور<AstVarScope*>>
        m_ifaceMemberVscps;

    صنف TraceInitDeclCollector final : عام VNVisitor {
        std::وکتور<AstTraceDecl*>& m_declps;  // Output: trace declarations found so far
        std::مجموعه<ثابت AstCFunc*> m_دیده_Funcps;

        فراغ بازدید(AstTraceDecl* nodep) override { m_declps.افزودن_پشت(nodep); }
        فراغ بازدید(AstCCall* nodep) override {
            إذا (AstCFunc* ثابت funcp = nodep->funcp()) collect(funcp);
        }
        فراغ بازدید(Ast_گره* nodep) override { iterateChildren(nodep); }

    عام:
        صریح TraceInitDeclCollector(std::وکتور<AstTraceDecl*>& declps)
            : m_declps{declps} {}
        فراغ collect(AstCFunc* funcp) {
            إذا (funcp && m_دیده_Funcps.درج(funcp).دومی) iterate(funcp);
        }
    };

    // METHODS

    ساكن منطقي same_ریشه_Init_Alias(ثابت AstTraceDecl* ریشه_Declp, ثابت AstTraceDecl* بالاترین_Declp) {
        ثابت VNumRange& ریشه_بیت_Range = ریشه_Declp->بیت_Range();
        ثابت VNumRange& بالاترین_بیت_Range = بالاترین_Declp->بیت_Range();
        أرجع ریشه_Declp->showname() == بالاترین_Declp->showname()
               && ریشه_Declp->declDirection() == بالاترین_Declp->declDirection()
               && ریشه_Declp->عرض_Min() == بالاترین_Declp->عرض_Min()
               && ریشه_بیت_Range.ranged() == بالاترین_بیت_Range.ranged()
               && (!ریشه_بیت_Range.ranged()
                   || (ریشه_بیت_Range.چپ() == بالاترین_بیت_Range.چپ()
                       && ریشه_بیت_Range.راست() == بالاترین_بیت_Range.راست()));
    }

    فراغ detectDuplicates() {
        UINFO(9, "Finding duplicates");
        // Note uses user4
        V3DupFinder dupFinder;  // Duplicate code detection

        // Compute member offsets within dtype instances incrementally.
        // For each dtype member TraceDecl, stores its code offset within the
        // dtype parent's code range.
        std::unordered_map<ثابت AstTraceDecl*, uint32_t> memberOffsets;
        std::unordered_map<ثابت AstTraceDecl*, uint32_t> running_جابه_جایی;

        // For fixup pass: track dups redirected to dtype parents, so we can
        // fix them if the parent's user2 is later set during the loop.
        // Maps dup vertex -> original canonical member vertex.
        std::وکتور<std::pair<Trace_Trace_رأس*, Trace_Trace_رأس*>> dtype_والد_Redirects;

        // Hash all of the traced values and find if there are any duplicates
        لكل (V3_گراف_رأس& vtx : m_گراف.vertices()) {
            إذا (Trace_Trace_رأس* ثابت vvertexp = vtx.cast<Trace_Trace_رأس>()) {
                AstTraceDecl* ثابت nodep = vvertexp->nodep();
                إذا (nodep->dtypeCallp()) استمر;

                إذا (nodep->dtypeDeclp()) {
                    uint32_t& جابه_جایی = running_جابه_جایی[nodep->dtypeDeclp()];
                    memberOffsets[nodep] = جابه_جایی;
                    جابه_جایی += nodep->کد_Inc();
                }

                UASSERT_OBJ(!vvertexp->duplicatep(), nodep, "Should not be a duplicate");
                ثابت خودکار dupit = dupFinder.پیدا_کن_Duplicate(nodep);
                إذا (dupit == dupFinder.انتها()) {
                    dupFinder.درج(nodep);
                } وإلا {
                    AstTraceDecl* ثابت dupDeclp = VN_AS(dupit->دومی, TraceDecl);
                    UASSERT_OBJ(dupDeclp, nodep, "Trace duplicate of wrong type");
                    Trace_Trace_رأس* ثابت dupvertexp
                        = dupDeclp->user1u().مقصد_گراف_رأس()->cast<Trace_Trace_رأس>();
                    UINFO(8, "  Orig " << dupDeclp << پایان_سطر);
                    UINFO(8, "   dup " << nodep << پایان_سطر);

                    إذا (dupDeclp->dtypeDeclp() && !dupDeclp->dtypeDeclp()->user2()) {
                        AstTraceDecl* ثابت dtypeParentp = dupDeclp->dtypeDeclp();
                        Trace_Trace_رأس* ثابت dtypeVtxp
                            = dtypeParentp->user1u().مقصد_گراف_رأس()->cast<Trace_Trace_رأس>();
                        ثابت خودکار it2 = memberOffsets.پیدا_کن(dupDeclp);
                        UASSERT_OBJ(it2 != memberOffsets.انتها(), dupDeclp,
                                    "Member offset not precomputed");
                        vvertexp->duplicatep(dtypeVtxp);
                        vvertexp->dtype_Alias_جابه_جایی(it2->دومی);
                        dtype_والد_Redirects.emplace_پشت(vvertexp, dupvertexp);
                    } وإلا {
                        vvertexp->duplicatep(dupvertexp);
                    }
                    إذا (nodep->dtypeDeclp()) nodep->dtypeDeclp()->user2(درست);
                }
            }
        }

        لكل (ثابت خودکار& pair : dtype_والد_Redirects) {
            Trace_Trace_رأس* ثابت dupVtxp = pair.اولی;
            Trace_Trace_رأس* ثابت memberVtxp = pair.دومی;
            ثابت AstTraceDecl* ثابت والد_Declp = dupVtxp->duplicatep()->nodep();
            إذا (والد_Declp->user2()) {
                dupVtxp->redirectDuplicatep(memberVtxp);
                dupVtxp->dtype_Alias_جابه_جایی(0);
            }
        }

        إذا (dumpLevel() || debug() >= 9)
            dupFinder.dumpFile(v3Global.debugFilename("trace") + ".hash", خطأ);
    }

    فراغ گراف_Dtype_Prune() {
        لكل (V3_گراف_رأس* ثابت vtxp : m_گراف.vertices().unlinkable()) {
            إذا (Trace_Trace_رأس* ثابت vvertexp = vtxp->cast<Trace_Trace_رأس>()) {
                AstTraceDecl* ثابت declp = vvertexp->nodep();
                // This skips the dtype sub-func optimization if a var is affected by multiple
                // activities.  We really only need to do this for _chg funcs (and not decls,
                // _const and _full) but it's simpiler to do it all one way or the other.
                إذا (declp) {
                    إذا (declp->user2() || (declp->dtypeDeclp() && !declp->dtypeDeclp()->user2())) {
                        AstCCall* ثابت callp = declp->dtypeCallp();
                        إذا (callp) {
                            Ast_گره* stmtexprp = callp->backp();
                            VL_DO_DANGLING(افزودن_Deletep(stmtexprp->unlink_Fr_پشت()), stmtexprp);
                        } وإلا {
                            منطقي خالی_Scope;
                            افعل {
                                خالی_Scope = خطأ;
                                Ast_گره* ثابت declBackp = declp->backp();
                                Ast_گره* ثابت declNextp = declp->nextp();
                                إذا (VN_IS(declBackp, Trace_افزودن_پیشوند)
                                    && VN_IS(declNextp, Trace_برداشت_پیشوند)) {
                                    VL_DO_DANGLING(افزودن_Deletep(declBackp->unlink_Fr_پشت()),
                                                   declBackp);
                                    VL_DO_DANGLING(افزودن_Deletep(declNextp->unlink_Fr_پشت()),
                                                   declNextp);
                                    خالی_Scope = درست;
                                }
                            } طالما (خالی_Scope);
                        }
                        // Can't purge until we finish this pass
                        افزودن_Deletep(declp->unlink_Fr_پشت());  // declp used below
                        vvertexp->reroute_یالها(&m_گراف);
                        vvertexp->unlinkDelete(&m_گراف);
                    }
                    declp->dtypeDeclp(اشاره_تهی);
                }
            }
        }
    }

    فراغ تقسیم_Trace_Decl_Funcs() {
        ثابت صحيح تقسیم_حد = v3Global.opt.output_تقسیم_CTrace() ? v3Global.opt.output_تقسیم_CTrace()
                                                                : std::numeric_limits<صحيح>::max();
        إذا (تقسیم_حد == std::numeric_limits<صحيح>::max()) أرجع;

        لكل (Ast_گره* nodep = m_بالاترین_Scopep->blocksp(); nodep; nodep = nodep->nextp()) {
            AstCFunc* ثابت funcp = VN_CAST(nodep, CFunc);
            إذا (!funcp || !funcp->user2()) استمر;

            ثابت string پایه_Name = funcp->name();
            uint32_t sub_رقم = 1;
            صحيح cur_اندازه = 0;
            AstCFunc* curFuncp = funcp;
            std::وکتور<Ast_گره_Stmt*> callStmts;

            خودکار شروع_New_Sub_Func = [&]() {
                File_سطر* ثابت flp = funcp->fileline();
                ثابت string newName = پایه_Name + "_" + cvt_مقصد_Str(sub_رقم++);
                AstCFunc* ثابت newFuncp = جديد AstCFunc{flp, newName, m_بالاترین_Scopep};
                newFuncp->argTypes(v3Global.opt.trace_Class_پایه() + "* tracep");
                newFuncp->isTrace(درست);
                newFuncp->isStatic(خطأ);
                newFuncp->isLoose(درست);
                newFuncp->slow(درست);
                newFuncp->dontCombine(درست);
                m_بالاترین_Scopep->جمع_کن_Blocksp(newFuncp);
                newFuncp->جمع_کن_Stmtsp(جديد AstCStmt{flp, "const int c = vlSymsp->__Vm_baseCode;"});
                AstCCall* ثابت callp = جديد AstCCall{flp, newFuncp};
                callp->dtype_مجموعه_Void();
                callp->argTypes("tracep");
                callStmts.افزودن_پشت(callp->makeStmt());
                curFuncp = newFuncp;
                cur_اندازه = 0;
            };

            لكل (Ast_گره *stmtp = funcp->stmtsp(), *nextp; stmtp; stmtp = nextp) {
                nextp = stmtp->nextp();
                cur_اندازه += stmtp->گره_شمار();

                إذا (curFuncp != funcp) {
                    stmtp->unlink_Fr_پشت();
                    curFuncp->جمع_کن_Stmtsp(stmtp);
                }

                إذا (cur_اندازه > تقسیم_حد && nextp) شروع_New_Sub_Func();
            }

            لكل (Ast_گره_Stmt* ثابت callStmtp : callStmts) { funcp->جمع_کن_Stmtsp(callStmtp); }
        }
    }

    فراغ گراف_Simplify(منطقي initial) {
        إذا (initial) {
            // Remove all variable nodes
            لكل (V3_گراف_رأس* ثابت vtxp : m_گراف.vertices().unlinkable()) {
                إذا (Trace_Var_رأس* ثابت vvertexp = vtxp->cast<Trace_Var_رأس>()) {
                    vvertexp->reroute_یالها(&m_گراف);
                    vvertexp->unlinkDelete(&m_گراف);
                }
            }
            // Remove multiple variables connecting funcs to traces
            // We do this twice, as then we have fewer edges to multiply out in the below
            // expansion.
            m_گراف.remove_Redundant_یالها_Max(&V3_گراف_یال::followAlwaysTrue);
            // Remove all Cfunc nodes
            لكل (V3_گراف_رأس* ثابت vtxp : m_گراف.vertices().unlinkable()) {
                إذا (Trace_CFunc_رأس* ثابت vvertexp = vtxp->cast<Trace_CFunc_رأس>()) {
                    vvertexp->reroute_یالها(&m_گراف);
                    vvertexp->unlinkDelete(&m_گراف);
                }
            }
        }

        // Remove multiple variables connecting funcs to traces
        m_گراف.remove_Redundant_یالها_Max(&V3_گراف_یال::followAlwaysTrue);

        // If there are any edges from a always, keep only the always
        لكل (V3_گراف_رأس& vtx : m_گراف.vertices()) {
            إذا (Trace_Trace_رأس* ثابت vvertexp = vtx.cast<Trace_Trace_رأس>()) {
                // Search for the incoming always edge
                ثابت V3_گراف_یال* alwaysEdgep = اشاره_تهی;
                لكل (ثابت V3_گراف_یال& یال : vvertexp->درون_یالها()) {
                    ثابت Trace_Activity_رأس* ثابت actVtxp
                        = یال.fromp()->as<ثابت Trace_Activity_رأس>();
                    إذا (actVtxp->activityAlways()) {
                        alwaysEdgep = &یال;
                        اكسر;
                    }
                }
                // If always edge exists, remove all other edges
                إذا (alwaysEdgep) {
                    لكل (V3_گراف_یال* ثابت edgep : vvertexp->درون_یالها().unlinkable()) {
                        إذا (edgep != alwaysEdgep) VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
                    }
                }
            }
        }

        // Activity points with no outputs can be removed
        لكل (V3_گراف_رأس* ثابت vtxp : m_گراف.vertices().unlinkable()) {
            إذا (Trace_Activity_رأس* ثابت aVtxp = vtxp->cast<Trace_Activity_رأس>()) {
                // Leave in the always vertex for later use.
                إذا (aVtxp != m_alwaysVtxp && aVtxp->بیرون_خالی()) {
                    VL_DO_DANGLING(aVtxp->unlinkDelete(&m_گراف), aVtxp);
                }
            }
        }
    }

    uint32_t assignactivityNumbers() {
        uint32_t activity_عدد = 1;  // Note 0 indicates "slow" only
        لكل (V3_گراف_رأس& vtx : m_گراف.vertices()) {
            إذا (Trace_Activity_رأس* ثابت vvertexp = vtx.cast<Trace_Activity_رأس>()) {
                إذا (vvertexp != m_alwaysVtxp) {
                    إذا (vvertexp->slow()) {
                        vvertexp->activity_کد(Trace_Activity_رأس::ACTIVITY_SLOW);
                    } وإلا {
                        vvertexp->activity_کد(activity_عدد++);
                    }
                }
            }
        }
        أرجع activity_عدد;
    }

    فراغ مرتب_سازی_Traces(TraceVec& traces, uint32_t& nNonConstCodes) {
        // Populate sort structure
        traces.clear();
        nNonConstCodes = 0;
        لكل (V3_گراف_رأس& vtx : m_گراف.vertices()) {
            إذا (Trace_Trace_رأس* ثابت vtxp = vtx.cast<Trace_Trace_رأس>()) {
                Act_کد_مجموعه act_مجموعه;
                UINFO(9, "  Add to sort: " << vtxp);
                UINFOTREE(9, vtxp->nodep(), "", "trnode");
                لكل (ثابت V3_گراف_یال& یال : vtxp->درون_یالها()) {
                    ثابت Trace_Activity_رأس* ثابت cfvertexp
                        = یال.fromp()->cast<ثابت Trace_Activity_رأس>();
                    UASSERT_OBJ(cfvertexp, vtxp->nodep(),
                                "Should have been function pointing to this trace");
                    UINFO(9, "   Activity: " << cfvertexp);
                    إذا (cfvertexp->activityAlways()) {
                        // If code 0, we always trace; ignore other codes
                        act_مجموعه.درج(Trace_Activity_رأس::ACTIVITY_ALWAYS);
                    } وإلا {
                        act_مجموعه.درج(cfvertexp->activity_کد());
                    }
                }
                UASSERT_OBJ(act_مجموعه.شمار(Trace_Activity_رأس::ACTIVITY_ALWAYS) == 0
                                || act_مجموعه.اندازه() == 1,
                            vtxp->nodep(), "Always active trace has further triggers");
                // Count nodes
                إذا (!vtxp->duplicatep() && !act_مجموعه.خالی())
                    nNonConstCodes += vtxp->nodep()->کد_Inc();
                إذا (act_مجموعه.خالی()) {
                    // If a trace doesn't have activity, it's constant, and we
                    // don't need to track changes on it.
                    act_مجموعه.درج(Trace_Activity_رأس::ACTIVITY_NEVER);
                } وإلا إذا (act_مجموعه.شمار(Trace_Activity_رأس::ACTIVITY_SLOW) && act_مجموعه.اندازه() > 1) {
                    // If a trace depends on the slow flag as well as other
                    // flags, remove the dependency on the slow flag. We will
                    // make slow routines set all activity flags.
                    act_مجموعه.erase(Trace_Activity_رأس::ACTIVITY_SLOW);
                }
                traces.emplace(act_مجموعه, vtxp);
            }
        }
    }

    فراغ گراف_Optimize() {
        // Assign initial activity numbers to activity vertices
        assignactivityNumbers();

        // Sort the traces by activity sets
        TraceVec traces;
        uint32_t unused1;
        مرتب_سازی_Traces(traces, unused1);

        // For each activity set with only a small number of signals, make those
        // signals always traced, as it's cheaper to check a few value changes
        // than to test a lot of activity flags
        خودکار it = traces.begin();
        ثابت خودکار انتها = traces.انتها();
        طالما (it != انتها) {
            خودکار سر = it;
            // Approximate the complexity of the value change check
            uint32_t complexity = 0;
            ثابت Act_کد_مجموعه& act_مجموعه = it->اولی;
            لكل (; it != انتها && it->اولی == act_مجموعه; ++it) {
                إذا (!it->دومی->duplicatep()) {
                    uint32_t هزینه = 0;
                    ثابت AstTraceDecl* ثابت declp = it->دومی->nodep();
                    // The number of comparisons required by bufp->chg*
                    هزینه += declp->isWide() ? declp->کد_Inc() : 1;
                    // Arrays are traced by element
                    هزینه *= declp->arrayRange().ranged() ? declp->arrayRange().elements() : 1;
                    // Note: Experiments factoring in the size of declp->valuep()
                    // showed no benefit in tracing speed, even for large trees,
                    // so we will leave those out for now.
                    complexity += هزینه;
                }
            }
            // Leave alone always changing, never changing and signals only set in slow code
            إذا (act_مجموعه.شمار(Trace_Activity_رأس::ACTIVITY_ALWAYS)) استمر;
            إذا (act_مجموعه.شمار(Trace_Activity_رأس::ACTIVITY_NEVER)) استمر;
            إذا (act_مجموعه.شمار(Trace_Activity_رأس::ACTIVITY_SLOW)) استمر;
            // If the value comparisons are cheaper to perform than checking the
            // activity flags make the signals always traced. Note this cost
            // equation is heuristic.
            إذا (complexity <= act_مجموعه.اندازه() * 2) {
                لكل (; سر != it; ++سر) {
                    جديد V3_گراف_یال{&m_گراف, m_alwaysVtxp, سر->دومی, 1};
                }
            }
        }

        گراف_Simplify(خطأ);
    }

    Ast_گره_عبارت* selectActivity(File_سطر* flp, uint32_t acode, ثابت VAccess& access) {
        أرجع جديد AstArraySel(flp, جديد AstVarRef{flp, m_activityVscp, access}, acode);
    }

    Ast_گره* newActivitySetter(Ast_گره* insertp, uint32_t کد) {
        ++m_statSetters;
        File_سطر* ثابت fl = insertp->fileline();
        Ast_تخصیص_بده* ثابت setterp = جديد Ast_تخصیص_بده{fl, selectActivity(fl, کد, VAccess::WRITE),
                                                 جديد AstConst{fl, AstConst::بیت_True{}}};
        أرجع setterp;
    }

    Ast_گره* newActivityAll(Ast_گره* insertp) {
        ++m_statSettersSlow;
        إذا (!m_actAllFuncp) {
            File_سطر* ثابت flp = m_بالاترین_Scopep->fileline();
            AstCFunc* ثابت funcp = جديد AstCFunc{flp, "__VtraceActivitySetAll", m_بالاترین_Scopep};
            funcp->slow(درست);
            funcp->isStatic(خطأ);
            funcp->isLoose(درست);
            m_بالاترین_Scopep->جمع_کن_Blocksp(funcp);
            لكل (uint32_t کد = 0; کد < m_activity_عدد; ++کد) {
                Ast_گره* ثابت setterp = newActivitySetter(insertp, کد);
                funcp->جمع_کن_Stmtsp(setterp);
            }
            m_actAllFuncp = funcp;
        }
        AstCCall* ثابت callp = جديد AstCCall{insertp->fileline(), m_actAllFuncp};
        callp->dtype_مجموعه_Void();
        أرجع callp->makeStmt();
    }

    فراغ createActivityFlags() {
        // Assign final activity numbers
        m_activity_عدد = assignactivityNumbers();

        // Create an array of bytes, not a bit vector, as they can be set
        // atomically by mtasks, and are cheaper to set (no need for
        // read-modify-write on the C type), and the speed of the tracing code
        // is the same on largish designs.
        File_سطر* ثابت flp = m_بالاترین_Scopep->fileline();
        Ast_گره_DType* ثابت newScalarDtp = جديد AstBasicDType{flp, VFlag_بیت_Packed{}, 1};
        v3Global.rootp()->typeTablep()->جمع_کن_Typesp(newScalarDtp);
        AstRange* ثابت newArange
            = جديد AstRange{flp, VNumRange{static_cast<صحيح>(m_activity_عدد) - 1, 0}};
        Ast_گره_DType* ثابت newArrDtp = جديد AstUnpackArrayDType{flp, newScalarDtp, newArange};
        v3Global.rootp()->typeTablep()->جمع_کن_Typesp(newArrDtp);
        m_activityVscp = v3Global.rootp()->بالاترین_Scopep()->create_موقت("__VtraceActivity", newArrDtp);

        // Insert activity setters
        لكل (ثابت V3_گراف_رأس& vtx : m_گراف.vertices()) {
            إذا (ثابت Trace_Activity_رأس* ثابت vtxp = vtx.cast<ثابت Trace_Activity_رأس>()) {
                Ast_گره* setterp = اشاره_تهی;
                إذا (vtxp->activitySlow()) {
                    setterp = newActivityAll(vtxp->insertp());
                    // Just set all flags in slow code as it should be rare.
                    // This will be rolled up into a loop by V3Reloop.
                } وإلا إذا (!vtxp->activityAlways()) {
                    setterp = newActivitySetter(vtxp->insertp(), vtxp->activity_کد());
                }
                إذا (setterp) {
                    Ast_گره* ثابت insertp = vtxp->insertp();
                    إذا (Ast_Stmt_عبارت* ثابت stmtp = VN_CAST(insertp, Stmt_عبارت)) {
                        stmtp->جمع_کن_بعدی_اینجا(setterp);
                    } وإلا إذا (AstCFunc* ثابت funcp = VN_CAST(insertp, CFunc)) {
                        // If there are awaits, insert the setter after each await
                        إذا (funcp->isCoroutine() && funcp->stmtsp()) {
                            funcp->stmtsp()->foreach_And_بعدی([setterp](AstCAwait* awaitp) {
                                awaitp->جمع_کن_بعدی_اینجا(setterp->clone_درخت(خطأ));
                            });
                        }
                        funcp->جمع_کن_Stmtsp(setterp);
                    } وإلا {
                        insertp->v3fatalSrc("Bad trace activity vertex");
                    }
                }
            }
        }
    }

    AstCFunc* newCFunc(VTraceType traceType, AstCFunc* بالاترین_Funcp, uint32_t func_رقم,
                       uint32_t پایه_کد = 0, ثابت AstTraceDecl* ثابت declp = اشاره_تهی,
                       منطقي declSub = خطأ) {
        // Create new function
        ثابت منطقي is_بالاترین_Func = !declp && بالاترین_Funcp == اشاره_تهی;
        std::string funcName;
        إذا (is_بالاترین_Func || declp) {
            funcName = traceType.func_پیشوند();
        } وإلا {
            funcName = بالاترین_Funcp->name();
            funcName += "_sub";
        }
        إذا (declp) {
            funcName += "_dtype";
            إذا (declSub) funcName += "_sub";
            funcName += "__";
            funcName = m_dtype_نامها.get(funcName);
        } وإلا {
            funcName += "_";
            funcName += cvt_مقصد_Str(func_رقم);
        }

        File_سطر* ثابت flp = m_بالاترین_Scopep->fileline();
        AstCFunc* ثابت funcp = جديد AstCFunc{flp, funcName, m_بالاترین_Scopep};
        funcp->isTrace(درست);
        funcp->dontCombine(درست);
        funcp->isLoose(درست);
        funcp->slow(traceType != VTraceType::CHANGE);
        funcp->isStatic(is_بالاترین_Func);
        // Add it to top scope
        m_بالاترین_Scopep->جمع_کن_Blocksp(funcp);
        ثابت std::string bufArg
            = v3Global.opt.trace_Class_پایه() + "::Buffer* bufp"
              + (declp ? (", uint32_t offset, const " + declp->dtypep()->cType("", درست, درست)
                          + " __VdtypeVar")
                       : "");
        إذا (is_بالاترین_Func) {
            // Top functions
            funcp->argTypes("void* voidSelf, " + bufArg);
            funcp->جمع_کن_Stmtsp(جديد AstCStmt{flp, EmitCUtil::void_Self_تخصیص_بده(m_بالاترین_Modp)});
            funcp->جمع_کن_Stmtsp(جديد AstCStmt{flp, EmitCUtil::sym_Class_تخصیص_بده()});
            // Add global activity check to change dump functions
            إذا (traceType == VTraceType::CHANGE) {  //
                funcp->جمع_کن_Stmtsp(
                    جديد AstCStmt{flp, "if (VL_UNLIKELY(!vlSymsp->__Vm_activity)) return;"});
            }
            // Register function
            AstCStmt* ثابت cstmtp = جديد AstCStmt{flp};
            m_regFuncp->جمع_کن_Stmtsp(cstmtp);
            إذا (traceType == VTraceType::CONSTANT) {
                cstmtp->جمع_کن("tracep->addConstCb(");
                cstmtp->جمع_کن(جديد Ast_Addr_از_CFunc{flp, funcp});
                cstmtp->جمع_کن(", " + std::مقصد_string(func_رقم) + ", vlSelf);");
            } وإلا إذا (traceType == VTraceType::پر) {
                cstmtp->جمع_کن("tracep->addFullCb(");
                cstmtp->جمع_کن(جديد Ast_Addr_از_CFunc{flp, funcp});
                cstmtp->جمع_کن(", " + std::مقصد_string(func_رقم) + ", vlSelf);");
            } وإلا {
                cstmtp->جمع_کن("tracep->addChgCb(");
                cstmtp->جمع_کن(جديد Ast_Addr_از_CFunc{flp, funcp});
                cstmtp->جمع_کن(", " + std::مقصد_string(func_رقم) + ", vlSelf);");
            }
        } وإلا {
            // Sub functions
            funcp->argTypes(bufArg);
            // Setup base references. Note in rare occasions we can end up with an empty trace
            // sub function, hence the VL_ATTR_UNUSED attributes.
            إذا (traceType != VTraceType::CHANGE) {
                // Full dump sub function
                funcp->جمع_کن_Stmtsp(جديد AstCStmt{flp,  //
                                              string("uint32_t* const oldp VL_ATTR_UNUSED = "
                                                     "bufp->oldp(vlSymsp->__Vm_baseCode")
                                                  + (declp ? " + offset" : "") + ");\n"});
            } وإلا {
                // Change dump sub function
                funcp->جمع_کن_Stmtsp(جديد AstCStmt{flp,  //
                                              "uint32_t* const oldp VL_ATTR_UNUSED = "
                                              "bufp->oldp(vlSymsp->__Vm_baseCode + "
                                                  + (declp ? " offset" : cvt_مقصد_Str(پایه_کد))
                                                  + ");\n"});
            }
            إذا (!declp) {
                // Add call to top function
                AstCCall* ثابت callp = جديد AstCCall{funcp->fileline(), funcp};
                callp->dtype_مجموعه_Void();
                callp->argTypes("bufp");
                بالاترین_Funcp->جمع_کن_Stmtsp(callp->makeStmt());
            }
        }
        //انجامشده
        UINFO(5, "  newCFunc " << funcp);
        أرجع funcp;
    }

    AstCFunc* newCDtypeSubFunc(VTraceType traceType, ثابت AstTraceDecl* ثابت declp,
                               AstCFunc* parentp) {
        AstCFunc* ثابت funcp = newCFunc(traceType, اشاره_تهی, 0, 0, declp, درست);
        File_سطر* ثابت flp = m_بالاترین_Scopep->fileline();
        AstCCall* ثابت callp = جديد AstCCall{flp, funcp};
        callp->dtype_مجموعه_Void();
        callp->argTypes("bufp, offset, __VdtypeVar");
        parentp->جمع_کن_Stmtsp(callp->makeStmt());
        أرجع funcp;
    }

    AstCFunc* createConstDtypeTraceFunctions(ثابت AstTraceDecl* declp) {
        ثابت Ast_گره_DType* ثابت dtypep = declp->dtypep()->skipRefp();
        خودکار pair = m_dtypeConstFuncs.emplace(dtypep, اشاره_تهی);
        إذا (pair.دومی) {
            File_سطر* ثابت flp = declp->fileline();
            AstCFunc* ثابت funcp = newCFunc(VTraceType::CONSTANT, اشاره_تهی, 0, 0, declp);

            منطقي اولی = درست;
            لكل (Ast_گره* callStmtp = declp->dtypeCallp()->funcp()->stmtsp(); callStmtp;
                 callStmtp = callStmtp->nextp()) {
                ثابت AstCCall* ثابت callp
                    = VN_CAST(VN_CAST(callStmtp, Stmt_عبارت)->exprp(), CCall);
                منطقي onlyOne = اولی && !callStmtp->nextp();
                AstCFunc* ثابت subFuncp
                    = onlyOne ? funcp : newCDtypeSubFunc(VTraceType::CONSTANT, declp, funcp);
                لكل (Ast_گره* stmtp = callp->funcp()->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    إذا (AstTraceDecl* ثابت فیلد_Declp = VN_CAST(stmtp, TraceDecl)) {
                        AstTraceInc* ثابت incp
                            = جديد AstTraceInc{flp, فیلد_Declp, VTraceType::CONSTANT};
                        subFuncp->جمع_کن_Stmtsp(incp);
                    }
                }
                اولی = خطأ;
            }

            pair.اولی->دومی = funcp;
        }

        أرجع pair.اولی->دومی;
    }

    فراغ createConstTraceFunctions(ثابت TraceVec& traces) {
        ثابت صحيح تقسیم_حد = v3Global.opt.output_تقسیم_CTrace() ? v3Global.opt.output_تقسیم_CTrace()
                                                                : std::numeric_limits<صحيح>::max();

        AstCFunc* ثابت بالاترین_Funcp = newCFunc(VTraceType::CONSTANT, اشاره_تهی, 0);
        uint32_t sub_Func_رقم = 0;
        AstCFunc* subFuncp = اشاره_تهی;
        صحيح subStmts = 0;
        std::وکتور<ثابت Trace_Trace_رأس*> duplicates;
        لكل (خودکار it = traces.cbegin(); it != traces.انتها(); ++it) {
            ثابت Trace_Trace_رأس* ثابت vtxp = it->دومی;
            AstTraceDecl* ثابت declp = vtxp->nodep();
            إذا (vtxp->duplicatep()) {
                duplicates.افزودن_پشت(vtxp);
                استمر;
            }

            // This is a canonical trace node. Assign trace code (signal number).
            UASSERT_OBJ(!declp->کد_Assigned(), declp,
                        "Canonical node should not have code assigned yet");
            declp->کد(m_کد);
            ثابت uint32_t کد_Inc = declp->کد_Inc();
            m_کد += کد_Inc;
            ++m_statUniqSigs;

            // If this is a const signal, add the AstTraceInc
            ثابت Act_کد_مجموعه& act_مجموعه = it->اولی;
            إذا (act_مجموعه.شمار(Trace_Activity_رأس::ACTIVITY_NEVER)) {
                // Crate new sub function if required
                إذا (!subFuncp || subStmts > تقسیم_حد) {
                    subStmts = 0;
                    subFuncp = newCFunc(VTraceType::CONSTANT, بالاترین_Funcp, sub_Func_رقم);
                    ++sub_Func_رقم;
                }
                File_سطر* ثابت flp = declp->fileline();
                إذا (declp->dtypeCallp()) {
                    AstCFunc* ثابت funcp = createConstDtypeTraceFunctions(declp);
                    Ast_گره_عبارت* argsp = اشاره_تهی;
                    argsp = Ast_گره::جمع_کن_بعدی(argsp, declp->valuep()->clone_درخت(خطأ));
                    AstCCall* ثابت callp = جديد AstCCall{flp, funcp, argsp};
                    callp->dtype_مجموعه_Void();
                    callp->argTypes(callp->argTypes() + "bufp, " + std::مقصد_string(declp->کد()));
                    subFuncp->جمع_کن_Stmtsp(callp->makeStmt());

                    subStmts += 1;
                } وإلا {
                    AstTraceInc* ثابت incp = جديد AstTraceInc{flp, declp, VTraceType::CONSTANT};
                    subFuncp->جمع_کن_Stmtsp(incp);
                    subStmts += incp->گره_شمار();
                }
            }
        }

        // Assign codes to duplicate nodes from their canonicals
        لكل (ثابت Trace_Trace_رأس* ثابت vtxp : duplicates) {
            AstTraceDecl* ثابت declp = vtxp->nodep();
            ثابت Trace_Trace_رأس* ثابت canonVtxp = vtxp->duplicatep();
            ثابت AstTraceDecl* ثابت canonDeclp = canonVtxp->nodep();
            UASSERT_OBJ(!canonVtxp->duplicatep(), canonDeclp, "Canonical node is a duplicate");
            UASSERT_OBJ(canonDeclp->کد_Assigned(), canonDeclp,
                        "Canonical node should have code assigned already");
            declp->کد(canonDeclp->کد() + vtxp->dtype_Alias_جابه_جایی());
        }
    }

    DtypeFuncs createNonConstDtypeTraceFunctions(ثابت AstTraceDecl* declp) {
        Ast_گره_DType* dtypep = declp->dtypep()->skipRefp();
        خودکار pair = m_dtypeNonConstFuncs.emplace(dtypep, DtypeFuncs{});
        إذا (pair.دومی) {
            File_سطر* ثابت flp = declp->fileline();
            AstCFunc* ثابت پر_Funcp = newCFunc(VTraceType::پر, اشاره_تهی, 0, 0, declp);
            AstCFunc* ثابت chgFuncp = newCFunc(VTraceType::CHANGE, اشاره_تهی, 0, 0, declp);

            منطقي اولی = درست;
            لكل (Ast_گره* callStmtp = declp->dtypeCallp()->funcp()->stmtsp(); callStmtp;
                 callStmtp = callStmtp->nextp()) {
                ثابت AstCCall* ثابت callp
                    = VN_CAST(VN_CAST(callStmtp, Stmt_عبارت)->exprp(), CCall);
                منطقي onlyOne = اولی && !callStmtp->nextp();
                AstCFunc* ثابت پر_Sub_Funcp
                    = onlyOne ? پر_Funcp : newCDtypeSubFunc(VTraceType::پر, declp, پر_Funcp);
                AstCFunc* ثابت chgSubFuncp
                    = onlyOne ? chgFuncp : newCDtypeSubFunc(VTraceType::CHANGE, declp, chgFuncp);
                لكل (Ast_گره* stmtp = callp->funcp()->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    إذا (AstTraceDecl* ثابت فیلد_Declp = VN_CAST(stmtp, TraceDecl)) {
                        AstTraceInc* ثابت incFullp
                            = جديد AstTraceInc{flp, فیلد_Declp, VTraceType::پر};
                        پر_Sub_Funcp->جمع_کن_Stmtsp(incFullp);
                        AstTraceInc* ثابت incChgp
                            = جديد AstTraceInc{flp, فیلد_Declp, VTraceType::CHANGE};
                        chgSubFuncp->جمع_کن_Stmtsp(incChgp);
                    }
                }
                اولی = خطأ;
            }

            pair.اولی->دومی = {.پر_Funcp = پر_Funcp, .chgFuncp = chgFuncp};
        }

        أرجع pair.اولی->دومی;
    }

    فراغ createNonConstTraceFunctions(ثابت TraceVec& traces, uint32_t nAllCodes,
                                      uint32_t parallelism) {
        ثابت صحيح تقسیم_حد = v3Global.opt.output_تقسیم_CTrace() ? v3Global.opt.output_تقسیم_CTrace()
                                                                : std::numeric_limits<صحيح>::max();

        // pre-incremented, so starts at 0
        uint32_t بالاترین_Func_رقم = std::numeric_limits<uint32_t>::max();
        TraceVec::const_iterator it = traces.begin();
        طالما (it != traces.انتها()) {
            AstCFunc* بالاترین_Ful_Funcp = اشاره_تهی;
            AstCFunc* بالاترین_Chg_Funcp = اشاره_تهی;
            AstCFunc* subFulFuncp = اشاره_تهی;
            AstCFunc* subChgFuncp = اشاره_تهی;
            uint32_t sub_Func_رقم = 0;
            صحيح subStmts = 0;
            ثابت uint32_t maxCodes = std::max((nAllCodes + parallelism - 1) / parallelism, 1U);
            uint32_t nCodes = 0;
            ثابت Act_کد_مجموعه* prev_Act_مجموعه = اشاره_تهی;
            AstIf* ifp = اشاره_تهی;
            uint32_t پایه_کد = 0;
            لكل (; nCodes < maxCodes && it != traces.انتها(); ++it) {
                ثابت Act_کد_مجموعه& act_مجموعه = it->اولی;
                // Traced value never changes, no need to add it
                إذا (act_مجموعه.شمار(Trace_Activity_رأس::ACTIVITY_NEVER)) استمر;

                ثابت Trace_Trace_رأس* ثابت vtxp = it->دومی;
                AstTraceDecl* ثابت declp = vtxp->nodep();

                // This is a duplicate decl, no need to add it, but must set the
                // function index to the same as the canonical node.
                إذا (ثابت Trace_Trace_رأس* ثابت canonVtxp = vtxp->duplicatep()) {
                    declp->fidx(canonVtxp->nodep()->fidx());
                    استمر;
                }

                // Create top function if not yet created
                إذا (!بالاترین_Ful_Funcp) {
                    ++بالاترین_Func_رقم;
                    بالاترین_Ful_Funcp = newCFunc(VTraceType::پر, اشاره_تهی, بالاترین_Func_رقم);
                    بالاترین_Chg_Funcp = newCFunc(VTraceType::CHANGE, اشاره_تهی, بالاترین_Func_رقم);
                }

                // Create new sub function if required
                إذا (!subFulFuncp || subStmts > تقسیم_حد) {
                    پایه_کد = declp->کد();
                    subStmts = 0;
                    subFulFuncp = newCFunc(VTraceType::پر, بالاترین_Ful_Funcp, sub_Func_رقم, پایه_کد);
                    subChgFuncp = newCFunc(VTraceType::CHANGE, بالاترین_Chg_Funcp, sub_Func_رقم, پایه_کد);
                    ++sub_Func_رقم;
                    prev_Act_مجموعه = اشاره_تهی;
                    ifp = اشاره_تهی;
                }

                // If required, create the conditional node checking the activity flags
                إذا (!prev_Act_مجموعه || act_مجموعه != *prev_Act_مجموعه) {
                    File_سطر* ثابت flp = m_بالاترین_Scopep->fileline();
                    ثابت منطقي always = act_مجموعه.شمار(Trace_Activity_رأس::ACTIVITY_ALWAYS) != 0;
                    Ast_گره_عبارت* condp = اشاره_تهی;
                    إذا (always) {
                        condp = جديد AstConst{flp, 1};  // Always true, will be folded later
                    } وإلا {
                        لكل (ثابت uint32_t act_کد : act_مجموعه) {
                            Ast_گره_عبارت* ثابت selp = selectActivity(flp, act_کد, VAccess::READ);
                            condp = condp ? جديد AstOr{flp, condp, selp} : selp;
                        }
                    }
                    ifp = جديد AstIf{flp, condp};
                    إذا (!always) ifp->branchPred(VBranchPred::BP_UNLIKELY);
                    subChgFuncp->جمع_کن_Stmtsp(ifp);
                    subStmts += ifp->گره_شمار();
                    prev_Act_مجموعه = &act_مجموعه;
                }

                // Add TraceInc nodes
                File_سطر* ثابت flp = declp->fileline();
                إذا (declp->dtypeCallp()) {
                    DtypeFuncs funcs = createNonConstDtypeTraceFunctions(declp);
                    Ast_گره_عبارت* argsp = اشاره_تهی;
                    argsp = Ast_گره::جمع_کن_بعدی(argsp, declp->valuep()->clone_درخت(خطأ));
                    AstCCall* ثابت callFullp = جديد AstCCall{flp, funcs.پر_Funcp, argsp};
                    callFullp->dtype_مجموعه_Void();
                    callFullp->argTypes(callFullp->argTypes() + "bufp, "
                                        + std::مقصد_string(declp->کد()));
                    subFulFuncp->جمع_کن_Stmtsp(callFullp->makeStmt());
                    argsp = اشاره_تهی;
                    argsp = Ast_گره::جمع_کن_بعدی(argsp, declp->valuep()->clone_درخت(خطأ));
                    AstCCall* ثابت callChgp = جديد AstCCall{flp, funcs.chgFuncp, argsp};
                    callChgp->dtype_مجموعه_Void();
                    callChgp->argTypes(callChgp->argTypes() + "bufp, "
                                       + std::مقصد_string(declp->کد()));
                    ifp->جمع_کن_Thensp(callChgp->makeStmt());

                    subStmts += 2;
                } وإلا {
                    AstTraceInc* ثابت incFulp = جديد AstTraceInc{flp, declp, VTraceType::پر};
                    subFulFuncp->جمع_کن_Stmtsp(incFulp);
                    AstTraceInc* ثابت incChgp
                        = جديد AstTraceInc{flp, declp, VTraceType::CHANGE, پایه_کد};
                    ifp->جمع_کن_Thensp(incChgp);

                    // Set the function index of the decl
                    declp->fidx(بالاترین_Func_رقم);

                    // Track splitting due to size
                    UASSERT_OBJ(incFulp->گره_شمار() == incChgp->گره_شمار(), declp,
                                "Should have equal cost");
                    ثابت VNumRange range = declp->arrayRange();
                    إذا (range.ranged()) {
                        // 2x because each element is a TraceInc and a VarRef
                        subStmts += range.elements() * 2;
                    } وإلا {
                        subStmts += incChgp->گره_شمار();
                    }
                }

                // Track partitioning
                nCodes += declp->کد_Inc();
            }
        }
    }

    فراغ createCleanupFunction() {
        File_سطر* ثابت fl = m_بالاترین_Scopep->fileline();
        AstCFunc* ثابت cleanupFuncp = جديد AstCFunc{fl, "trace_cleanup", m_بالاترین_Scopep};
        cleanupFuncp->argTypes("void* voidSelf, " + v3Global.opt.trace_Class_پایه()
                               + "* /*unused*/");
        cleanupFuncp->isTrace(درست);
        cleanupFuncp->slow(خطأ);
        cleanupFuncp->isStatic(درست);
        cleanupFuncp->isLoose(درست);
        m_بالاترین_Scopep->جمع_کن_Blocksp(cleanupFuncp);
        cleanupFuncp->جمع_کن_Stmtsp(جديد AstCStmt{fl, EmitCUtil::void_Self_تخصیص_بده(m_بالاترین_Modp)});
        cleanupFuncp->جمع_کن_Stmtsp(جديد AstCStmt{fl, EmitCUtil::sym_Class_تخصیص_بده()});

        // Register it
        {
            AstCStmt* ثابت cstmtp = جديد AstCStmt{fl};
            m_regFuncp->جمع_کن_Stmtsp(cstmtp);
            cstmtp->جمع_کن("tracep->addCleanupCb(");
            cstmtp->جمع_کن(جديد Ast_Addr_از_CFunc{fl, cleanupFuncp});
            cstmtp->جمع_کن(", vlSelf);");
        }

        // Clear global activity flag
        cleanupFuncp->جمع_کن_Stmtsp(
            جديد AstCStmt{m_بالاترین_Scopep->fileline(), "vlSymsp->__Vm_activity = false;"s});

        // Clear fine grained activity flags
        لكل (uint32_t i = 0; i < m_activity_عدد; ++i) {
            Ast_گره* ثابت clrp = جديد Ast_تخصیص_بده{fl, selectActivity(fl, i, VAccess::WRITE),
                                                جديد AstConst{fl, AstConst::بیت_False{}}};
            cleanupFuncp->جمع_کن_Stmtsp(clrp);
        }
    }

    فراغ createTraceFunctions() {
        // Detect and remove duplicate values
        detectDuplicates();

        گراف_Dtype_Prune();
        تقسیم_Trace_Decl_Funcs();
        m_گراف.remove_Redundant_یالها_Max(&V3_گراف_یال::followAlwaysTrue);

        // Simplify & optimize the graph
        إذا (dump_گراف_Level() >= 6) m_گراف.dumpDotFilePrefixed("trace_pre");
        گراف_Simplify(درست);
        إذا (dump_گراف_Level() >= 6) m_گراف.dumpDotFilePrefixed("trace_simplified");
        گراف_Optimize();
        إذا (dump_گراف_Level() >= 6) m_گراف.dumpDotFilePrefixed("trace_optimized");

        // Create the fine grained activity flags
        createActivityFlags();

        // Form a sorted list of the traces we are interested in
        TraceVec traces;  // The sorted traces
        // We will split functions such that each have to dump roughly the same amount of data
        // for this we need to keep tack of the number of codes used by the trace functions.
        uint32_t nNonConstCodes = 0;
        مرتب_سازی_Traces(traces, nNonConstCodes);

        // Our keys are now sorted to have same activity number adjacent, then
        // by trace order. (Better would be execution order for cache
        // efficiency....) Last are constants and non-changers, as then the
        // last value vector is more compact

        // Create the trace registration function
        m_regFuncp = جديد AstCFunc{m_بالاترین_Scopep->fileline(), "trace_register", m_بالاترین_Scopep};
        m_regFuncp->argTypes(v3Global.opt.trace_Class_پایه() + "* tracep");
        m_regFuncp->مدخل_نقطه(درست);
        m_regFuncp->isTrace(درست);
        m_regFuncp->slow(درست);
        m_regFuncp->isStatic(خطأ);
        m_regFuncp->isLoose(درست);
        m_بالاترین_Scopep->جمع_کن_Blocksp(m_regFuncp);

        // Create the const dump functions. Also allocates trace codes.
        createConstTraceFunctions(traces);

        // Create the full and incremental dump functions
        createNonConstTraceFunctions(traces, nNonConstCodes, m_parallelism);

        // Root-traced libraries alias wrapper IOs onto the existing top-module codes.
        إذا (!v3Global.opt.libCreate().خالی()) {
            std::وکتور<AstTraceDecl*> ریشه_Declps;
            std::وکتور<AstTraceDecl*> بالاترین_Declps;
            TraceInitDeclCollector ریشه_Collector{ریشه_Declps};
            TraceInitDeclCollector بالاترین_Collector{بالاترین_Declps};
            لكل (Ast_گره* blockp = m_بالاترین_Scopep->blocksp(); blockp; blockp = blockp->nextp()) {
                AstCFunc* ثابت funcp = VN_CAST(blockp, CFunc);
                إذا (funcp && VString::startsWith(funcp->name(), "trace_init_leaf_root__")) {
                    ریشه_Collector.collect(funcp);
                } وإلا إذا (funcp && VString::startsWith(funcp->name(), "trace_init_leaf_top__")) {
                    بالاترین_Collector.collect(funcp);
                }
            }

            std::وکتور<منطقي> استفاده_شده(بالاترین_Declps.اندازه(), خطأ);
            لكل (AstTraceDecl* ثابت ریشه_Declp : ریشه_Declps) {
                لكل (اندازه_t i = 0; i < بالاترین_Declps.اندازه(); ++i) {
                    إذا (استفاده_شده[i] || !same_ریشه_Init_Alias(ریشه_Declp, بالاترین_Declps[i])) استمر;
                    ریشه_Declp->کد(بالاترین_Declps[i]->کد());
                    استفاده_شده[i] = درست;
                    اكسر;
                }
            }
        }

        // Remove refs to traced values from TraceDecl nodes, these have now moved under
        // TraceInc
        لكل (ثابت خودکار& i : traces) {
            Ast_گره* ثابت valuep = i.دومی->nodep()->valuep();
            VL_DO_DANGLING(valuep->unlink_Fr_پشت()->delete_درخت(), valuep);
        }

        // Create the trace cleanup function clearing the activity flags
        createCleanupFunction();
    }

    Trace_CFunc_رأس* getCFuncVertexp(AstCFunc* nodep) {
        V3_گراف_رأس* ثابت vtxp = nodep->user1u().مقصد_گراف_رأس();
        Trace_CFunc_رأس* vertexp = vtxp ? vtxp->cast<Trace_CFunc_رأس>() : اشاره_تهی;
        إذا (!vertexp) {
            vertexp = جديد Trace_CFunc_رأس{&m_گراف, nodep};
            nodep->user1p(vertexp);
        }
        أرجع vertexp;
    }
    Trace_Activity_رأس* getActivityVertexp(Ast_گره* nodep, منطقي slow) {
        V3_گراف_رأس* ثابت vtxp = nodep->user3u().مقصد_گراف_رأس();
        Trace_Activity_رأس* vertexp = vtxp ? vtxp->cast<Trace_Activity_رأس>() : اشاره_تهی;
        إذا (!vertexp) {
            vertexp = جديد Trace_Activity_رأس{&m_گراف, nodep, slow};
            nodep->user3p(vertexp);
        }
        vertexp->slow(slow);
        أرجع vertexp;
    }

    // VISITORS
    فراغ بازدید(AstNetlist* nodep) override {
        // Add vertexes for all TraceDecl, and edges from VARs each trace looks at
        m_finding = خطأ;
        iterateChildren(nodep);

        // Add vertexes for all CFUNCs, and edges to VARs the func sets
        m_finding = درست;
        iterateChildren(nodep);

        // Create the trace functions and insert them into the tree
        createTraceFunctions();

        // Save number of trace codes used
        nodep->nTraceCodes(m_کد);
    }
    فراغ بازدید(Ast_گره_Module* nodep) override {
        إذا (nodep->is_بالاترین()) m_بالاترین_Modp = nodep;
        iterateChildren(nodep);
    }
    فراغ بازدید(AstVarScope* nodep) override {
        إذا (!m_finding) {
            إذا (ثابت AstIface* ثابت ifacep = nodep->varp()->sensIfacep()) {
                m_ifaceMemberVscps[{ifacep, nodep->varp()->name()}].افزودن_پشت(nodep);
            }
        }
    }
    فراغ بازدید(Ast_Stmt_عبارت* nodep) override {
        إذا (!m_finding && !nodep->user2()) {
            إذا (AstCCall* ثابت callp = VN_CAST(nodep->exprp(), CCall)) {
                UINFO(8, "   CCALL " << callp);
                // See if there are other calls in same statement list;
                // If so, all funcs might share the same activity code
                Trace_Activity_رأس* ثابت activityVtxp
                    = getActivityVertexp(nodep, callp->funcp()->slow());
                لكل (Ast_گره* nextp = nodep; nextp; nextp = nextp->nextp()) {
                    إذا (Ast_Stmt_عبارت* ثابت stmtp = VN_CAST(nextp, Stmt_عبارت)) {
                        إذا (AstCCall* ثابت ccallp = VN_CAST(stmtp->exprp(), CCall)) {
                            stmtp->user2(درست);  // Processed
                            UINFO(8, "     SubCCALL " << ccallp);
                            V3_گراف_رأس* ثابت ccallFuncVtxp = getCFuncVertexp(ccallp->funcp());
                            activityVtxp->slow(ccallp->funcp()->slow());
                            جديد V3_گراف_یال{&m_گراف, activityVtxp, ccallFuncVtxp, 1};
                        }
                    }
                }
            }
        }
        iterateChildren(nodep);
    }
    فراغ بازدید(AstCFunc* nodep) override {
        UINFO(8, "   CFUNC " << nodep);
        V3_گراف_رأس* ثابت funcVtxp = getCFuncVertexp(nodep);
        إذا (!m_finding) {  // If public, we need a unique activity code to allow for sets
                           // directly in this func
            إذا (nodep->func_عمومی() || nodep->dpiExportImpl() || nodep->مدخل_نقطه()
                || nodep->isCoroutine()) {
                // Cannot treat a coroutine as slow, it may be resumed later
                ثابت منطقي slow = nodep->slow() && !nodep->isCoroutine();
                Trace_Activity_رأس* ثابت activityVtxp = getActivityVertexp(nodep, slow);
                جديد V3_گراف_یال{&m_گراف, activityVtxp, funcVtxp, 1};
            }
        }
        VL_RESTORER(m_cfuncp);
        m_cfuncp = nodep;
        iterateChildren(nodep);
    }
    فراغ بازدید(AstTraceDecl* nodep) override {
        UINFO(8, "   TRACE " << nodep);
        إذا (!m_finding && !nodep->درون_Dtype_Func()) {
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
            إذا (m_cfuncp && VString::startsWith(m_cfuncp->name(), "trace_init_leaf_root__")) {
                أرجع;
            }
            V3_گراف_رأس* ثابت vertexp = جديد Trace_Trace_رأس{&m_گراف, nodep};
            nodep->user1p(vertexp);

            UASSERT_OBJ(m_cfuncp, nodep, "Trace not under func");
            m_cfuncp->user2(درست);
            VL_RESTORER(m_tracep);
            m_tracep = nodep;
            iterateChildren(nodep);
        }
    }
    فراغ بازدید(AstVarRef* nodep) override {
        UASSERT_OBJ(nodep->varScopep(), nodep, "No var scope?");
        AstVarScope* ثابت varscopep = nodep->varScopep();
        V3_گراف_رأس* varVtxp = varscopep->user1u().مقصد_گراف_رأس();
        إذا (m_tracep) {
            UASSERT_OBJ(nodep->access().isReadOnly(), nodep, "Lvalue in trace?  Should be const.");
            إذا (!varVtxp) {
                varVtxp = جديد Trace_Var_رأس{&m_گراف, nodep->varScopep()};
                nodep->varScopep()->user1p(varVtxp);
            }
            V3_گراف_رأس* ثابت traceVtxp = m_tracep->user1u().مقصد_گراف_رأس();
            جديد V3_گراف_یال{&m_گراف, varVtxp, traceVtxp, 1};
            إذا (nodep->varp()->isPrimaryInish()  // Always need to trace primary inputs
                || nodep->varp()->is_Sig_عمومی()) {  // Or ones user can change
                جديد V3_گراف_یال{&m_گراف, m_alwaysVtxp, traceVtxp, 1};
            }
            إذا (m_tracep->dtypeCallp()) varscopep->user2p(m_tracep);
        } وإلا إذا (m_cfuncp && m_finding && nodep->access().isWriteOrRW()) {
            V3_گراف_رأس* ثابت funcVtxp = getCFuncVertexp(m_cfuncp);
            إذا (varVtxp) {  // else we're not tracing this signal
                جديد V3_گراف_یال{&m_گراف, funcVtxp, varVtxp, 1};
                AstTraceDecl* ثابت declp = VN_AS(varscopep->user2p(), TraceDecl);
                إذا (declp) {
                    V3_گراف_رأس* ثابت cFuncVtxp = getCFuncVertexp(m_cfuncp);
                    لكل (ثابت V3_گراف_یال& یال : cFuncVtxp->درون_یالها()) {
                        V3_گراف_رأس* ثابت activityp = یال.fromp();
                        إذا (!declp->user3p()) {
                            declp->user3p(activityp);
                        } وإلا إذا (declp->user3u().مقصد_گراف_رأس() != activityp) {
                            declp->user2(درست);
                        }
                    }
                }
            }
        }
    }
    فراغ بازدید(AstMemberSel* nodep) override {
        إذا (m_cfuncp && m_finding && nodep->access().isWriteOrRW()) {
            AstIfaceRefDType* ثابت dtypep
                = VN_CAST(nodep->fromp()->dtypep()->skipRefp(), IfaceRefDType);
            إذا (dtypep && dtypep->isVirtual()) {
                ثابت خودکار it = m_ifaceMemberVscps.پیدا_کن({dtypep->ifacep(), nodep->varp()->name()});
                إذا (it != m_ifaceMemberVscps.انتها()) {
                    V3_گراف_رأس* ثابت funcVtxp = getCFuncVertexp(m_cfuncp);
                    لكل (AstVarScope* ثابت vscp : it->دومی) {
                        V3_گراف_رأس* varVtxp = vscp->user1u().مقصد_گراف_رأس();
                        إذا (!varVtxp) {
                            varVtxp = جديد Trace_Var_رأس{&m_گراف, vscp};
                            vscp->user1p(varVtxp);
                        }
                        جديد V3_گراف_یال{&m_گراف, funcVtxp, varVtxp, 1};
                    }
                }
            }
        }
        iterateChildren(nodep);
    }
    //--------------------
    فراغ بازدید(Ast_گره* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    صریح TraceVisitor(AstNetlist* nodep)
        : m_alwaysVtxp{جديد Trace_Activity_رأس{&m_گراف, Trace_Activity_رأس::ACTIVITY_ALWAYS}} {
        nodep->user2_Clear_درخت();  // TraceDecl multiple activities flag
        nodep->user3_Clear_درخت();  // TraceDecl TraceActivityVertex (assumes we start at nullptr)
        iterate(nodep);
        nodep->foreach([](AstTraceDecl* ثابت declp) {
            إذا (declp->درون_Dtype_Func()) {
                declp->valuep()->unlink_Fr_پشت()->delete_درخت();
                declp->valuep(اشاره_تهی);
            }
        });
    }
    ~TraceVisitor() override {
        V3Stats::جمع_کن_Stat("Tracing, Activity setters", m_statSetters);
        V3Stats::جمع_کن_Stat("Tracing, Activity slow blocks", m_statSettersSlow);
        V3Stats::جمع_کن_Stat("Tracing, Unique trace codes", m_کد);
        V3Stats::جمع_کن_Stat("Tracing, Unique traced signals", m_statUniqSigs);
    }
};

//######################################################################
// Trace class functions

فراغ V3Trace::traceAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { TraceVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_درخت("trace", 0, dump_درخت_Either_Level() >= 3);
}
