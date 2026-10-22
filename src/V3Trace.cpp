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

صنف Trace_Activity_راس final : عام V3_ګراف_راس {
    VL_RTTI_IMPL(Trace_Activity_راس, V3_ګراف_راس)
    Ast_غوټه* ثابت m_insertp;
    int32_t m_activity_کوډ;
    منطقي m_slow;  // If always slow, we can use the same code
عام:
    تعداد { ACTIVITY_NEVER = ((1UL << 31) - 1) };
    تعداد { ACTIVITY_ALWAYS = ((1UL << 31) - 2) };
    تعداد { ACTIVITY_SLOW = 0 };
    Trace_Activity_راس(V3_ګراف* graphp, Ast_غوټه* nodep, منطقي slow)
        : V3_ګراف_راس{graphp}
        , m_insertp{nodep} {
        m_activity_کوډ = 0;
        m_slow = slow;
    }
    Trace_Activity_راس(V3_ګراف* graphp, int32_t کوډ)
        : V3_ګراف_راس{graphp}
        , m_insertp{تش_اشاره} {
        m_activity_کوډ = کوډ;
        m_slow = خطأ;
    }
    ~Trace_Activity_راس() override = افتراضي;
    // ACCESSORS
    Ast_غوټه* insertp() ثابت {
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
    int32_t activity_کوډ() ثابت { أرجع m_activity_کوډ; }
    منطقي activityAlways() ثابت { أرجع activity_کوډ() == ACTIVITY_ALWAYS; }
    منطقي activitySlow() ثابت { أرجع activity_کوډ() == ACTIVITY_SLOW; }
    فراغ activity_کوډ(int32_t کوډ) { m_activity_کوډ = کوډ; }
    منطقي slow() ثابت { أرجع m_slow; }
    فراغ slow(منطقي flag) {
        إذا (!flag) m_slow = خطأ;
    }
};

صنف Trace_CFunc_راس final : عام V3_ګراف_راس {
    VL_RTTI_IMPL(Trace_CFunc_راس, V3_ګراف_راس)
    AstCFunc* ثابت m_nodep;

عام:
    Trace_CFunc_راس(V3_ګراف* graphp, AstCFunc* nodep)
        : V3_ګراف_راس{graphp}
        , m_nodep{nodep} {}
    ~Trace_CFunc_راس() override = افتراضي;
    // ACCESSORS
    AstCFunc* nodep() ثابت { أرجع m_nodep; }
    string name() ثابت override { أرجع nodep()->name(); }
    string dotColor() ثابت override { أرجع "yellow"; }
    File_کرښه* fileline() ثابت override { أرجع nodep()->fileline(); }
};

صنف Trace_Trace_راس final : عام V3_ګراف_راس {
    VL_RTTI_IMPL(Trace_Trace_راس, V3_ګراف_راس)
    AstTraceDecl* ثابت m_nodep;  // TRACEINC this represents
    // nullptr, or other vertex with the real code() that duplicates this one
    Trace_Trace_راس* m_duplicatep = تش_اشاره;
    // When aliasing to a dtype parent, offset of the member within the dtype
    uint32_t m_dtype_Alias_دننه_واټن = 0;

عام:
    Trace_Trace_راس(V3_ګراف* graphp, AstTraceDecl* nodep)
        : V3_ګراف_راس{graphp}
        , m_nodep{nodep} {}
    ~Trace_Trace_راس() override = افتراضي;
    // ACCESSORS
    AstTraceDecl* nodep() ثابت { أرجع m_nodep; }
    string name() ثابت override { أرجع nodep()->name(); }
    string dotColor() ثابت override { أرجع "red"; }
    File_کرښه* fileline() ثابت override { أرجع nodep()->fileline(); }
    Trace_Trace_راس* duplicatep() ثابت { أرجع m_duplicatep; }
    فراغ duplicatep(Trace_Trace_راس* dupp) {
        UASSERT_OBJ(!duplicatep(), nodep(), "Assigning duplicatep() to already duplicated node");
        m_duplicatep = dupp;
    }
    فراغ redirectDuplicatep(Trace_Trace_راس* dupp) { m_duplicatep = dupp; }
    uint32_t dtype_Alias_دننه_واټن() ثابت { أرجع m_dtype_Alias_دننه_واټن; }
    فراغ dtype_Alias_دننه_واټن(uint32_t دننه_واټن) { m_dtype_Alias_دننه_واټن = دننه_واټن; }
};

صنف Trace_Var_راس final : عام V3_ګراف_راس {
    VL_RTTI_IMPL(Trace_Var_راس, V3_ګراف_راس)
    AstVarScope* ثابت m_nodep;

عام:
    Trace_Var_راس(V3_ګراف* graphp, AstVarScope* nodep)
        : V3_ګراف_راس{graphp}
        , m_nodep{nodep} {}
    ~Trace_Var_راس() override = افتراضي;
    // ACCESSORS
    AstVarScope* nodep() ثابت { أرجع m_nodep; }
    string name() ثابت override { أرجع nodep()->name(); }
    string dotColor() ثابت override { أرجع "skyblue"; }
    File_کرښه* fileline() ثابت override { أرجع nodep()->fileline(); }
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
    ثابت VNUser1_دننه_Use m_inuser1;
    ثابت VNUser2_دننه_Use m_inuser2;
    ثابت VNUser3_دننه_Use m_inuser3;
    // VNUser4InUse     In V3Hasher via V3DupFinder

    // STATE
    Ast_غوټه_Module* m_سر_Modp = تش_اشاره;  // Module to add variables to
    AstScope* ثابت m_سر_Scopep = v3Global.rootp()->سر_Scopep()->scopep();  // The top AstScope
    AstCFunc* m_cfuncp = تش_اشاره;  // C function adding to graph
    AstCFunc* m_regFuncp = تش_اشاره;  // Trace registration function
    AstCFunc* m_actAllFuncp = تش_اشاره;  // Set all activity function
    AstTraceDecl* m_tracep = تش_اشاره;  // Trace function adding to graph
    AstVarScope* m_activityVscp = تش_اشاره;  // Activity variable
    uint32_t m_activity_شمېره = 0;  // Count of fields in activity variable
    uint32_t m_کوډ = 0;  // Trace ident code# being assigned
    V3_ګراف m_ګراف;  // Var/CFunc tracking
    Trace_Activity_راس* ثابت m_alwaysVtxp;  // "Always trace" vertex
    منطقي m_finding = خطأ;  // Pass one of algorithm?
    بنية DtypeFuncs final {
    عام:
        AstCFunc* ډک_Funcp = تش_اشاره;
        AstCFunc* chgFuncp = تش_اشاره;
    };
    std::unordered_map<ثابت Ast_غوټه_DType*, DtypeFuncs>
        m_dtypeNonConstFuncs;  // Full / Chg funcs per type
    std::unordered_map<ثابت Ast_غوټه_DType*, AstCFunc*> m_dtypeConstFuncs;  // Const func per type
    V3_Unique_نومونه m_dtype_نومونه{""};  // Unique type func names

    // Trace parallelism. Only VCD tracing can be parallelized at this time.
    ثابت uint32_t m_parallelism
        = v3Global.opt.useTraceParallel() ? static_cast<uint32_t>(v3Global.opt.تارونه()) : 1;

    VDouble0 m_statSetters;  // Statistic tracking
    VDouble0 m_statSettersSlow;  // Statistic tracking
    VDouble0 m_statUniqSigs;  // Statistic tracking

    // All activity numbers applying to a given trace
    کارول Act_کوډ_سېټ = std::سېټ<uint32_t>;
    // For activity set, what traces apply
    کارول TraceVec = std::multimap<Act_کوډ_سېټ, Trace_Trace_راس*>;
    // Candidate interface-member VarScopes keyed by (interface type, member name)
    std::map<std::pair<ثابت AstIface*, std::string>, std::ویکټور<AstVarScope*>>
        m_ifaceMemberVscps;

    صنف TraceInitDeclCollector final : عام VNVisitor {
        std::ویکټور<AstTraceDecl*>& m_declps;  // Output: trace declarations found so far
        std::سېټ<ثابت AstCFunc*> m_کتل_شوی_Funcps;

        فراغ لیدنه(AstTraceDecl* nodep) override { m_declps.ټېل_شا(nodep); }
        فراغ لیدنه(AstCCall* nodep) override {
            إذا (AstCFunc* ثابت funcp = nodep->funcp()) collect(funcp);
        }
        فراغ لیدنه(Ast_غوټه* nodep) override { iterateChildren(nodep); }

    عام:
        څرګند TraceInitDeclCollector(std::ویکټور<AstTraceDecl*>& declps)
            : m_declps{declps} {}
        فراغ collect(AstCFunc* funcp) {
            إذا (funcp && m_کتل_شوی_Funcps.ورننباسه(funcp).دوهم) iterate(funcp);
        }
    };

    // METHODS

    ساكن منطقي same_ریښه_Init_Alias(ثابت AstTraceDecl* ریښه_Declp, ثابت AstTraceDecl* سر_Declp) {
        ثابت VNumRange& ریښه_بټ_Range = ریښه_Declp->بټ_Range();
        ثابت VNumRange& سر_بټ_Range = سر_Declp->بټ_Range();
        أرجع ریښه_Declp->showname() == سر_Declp->showname()
               && ریښه_Declp->declDirection() == سر_Declp->declDirection()
               && ریښه_Declp->پلنوالی_Min() == سر_Declp->پلنوالی_Min()
               && ریښه_بټ_Range.ranged() == سر_بټ_Range.ranged()
               && (!ریښه_بټ_Range.ranged()
                   || (ریښه_بټ_Range.کیڼ() == سر_بټ_Range.کیڼ()
                       && ریښه_بټ_Range.ښی() == سر_بټ_Range.ښی()));
    }

    فراغ detectDuplicates() {
        UINFO(9, "Finding duplicates");
        // Note uses user4
        V3DupFinder dupFinder;  // Duplicate code detection

        // Compute member offsets within dtype instances incrementally.
        // For each dtype member TraceDecl, stores its code offset within the
        // dtype parent's code range.
        std::unordered_map<ثابت AstTraceDecl*, uint32_t> memberOffsets;
        std::unordered_map<ثابت AstTraceDecl*, uint32_t> running_دننه_واټن;

        // For fixup pass: track dups redirected to dtype parents, so we can
        // fix them if the parent's user2 is later set during the loop.
        // Maps dup vertex -> original canonical member vertex.
        std::ویکټور<std::pair<Trace_Trace_راس*, Trace_Trace_راس*>> dtype_پلار_Redirects;

        // Hash all of the traced values and find if there are any duplicates
        لكل (V3_ګراف_راس& vtx : m_ګراف.vertices()) {
            إذا (Trace_Trace_راس* ثابت vvertexp = vtx.cast<Trace_Trace_راس>()) {
                AstTraceDecl* ثابت nodep = vvertexp->nodep();
                إذا (nodep->dtypeCallp()) استمر;

                إذا (nodep->dtypeDeclp()) {
                    uint32_t& دننه_واټن = running_دننه_واټن[nodep->dtypeDeclp()];
                    memberOffsets[nodep] = دننه_واټن;
                    دننه_واټن += nodep->کوډ_Inc();
                }

                UASSERT_OBJ(!vvertexp->duplicatep(), nodep, "Should not be a duplicate");
                ثابت خپلکاری dupit = dupFinder.پیدا_کړه_Duplicate(nodep);
                إذا (dupit == dupFinder.پای_ټکی()) {
                    dupFinder.ورننباسه(nodep);
                } وإلا {
                    AstTraceDecl* ثابت dupDeclp = VN_AS(dupit->دوهم, TraceDecl);
                    UASSERT_OBJ(dupDeclp, nodep, "Trace duplicate of wrong type");
                    Trace_Trace_راس* ثابت dupvertexp
                        = dupDeclp->user1u().منزل_ګراف_راس()->cast<Trace_Trace_راس>();
                    UINFO(8, "  Orig " << dupDeclp << کرښه_پای);
                    UINFO(8, "   dup " << nodep << کرښه_پای);

                    إذا (dupDeclp->dtypeDeclp() && !dupDeclp->dtypeDeclp()->user2()) {
                        AstTraceDecl* ثابت dtypeParentp = dupDeclp->dtypeDeclp();
                        Trace_Trace_راس* ثابت dtypeVtxp
                            = dtypeParentp->user1u().منزل_ګراف_راس()->cast<Trace_Trace_راس>();
                        ثابت خپلکاری it2 = memberOffsets.پیدا_کړه(dupDeclp);
                        UASSERT_OBJ(it2 != memberOffsets.پای_ټکی(), dupDeclp,
                                    "Member offset not precomputed");
                        vvertexp->duplicatep(dtypeVtxp);
                        vvertexp->dtype_Alias_دننه_واټن(it2->دوهم);
                        dtype_پلار_Redirects.emplace_شا(vvertexp, dupvertexp);
                    } وإلا {
                        vvertexp->duplicatep(dupvertexp);
                    }
                    إذا (nodep->dtypeDeclp()) nodep->dtypeDeclp()->user2(ریښتیا);
                }
            }
        }

        لكل (ثابت خپلکاری& pair : dtype_پلار_Redirects) {
            Trace_Trace_راس* ثابت dupVtxp = pair.لومړی;
            Trace_Trace_راس* ثابت memberVtxp = pair.دوهم;
            ثابت AstTraceDecl* ثابت پلار_Declp = dupVtxp->duplicatep()->nodep();
            إذا (پلار_Declp->user2()) {
                dupVtxp->redirectDuplicatep(memberVtxp);
                dupVtxp->dtype_Alias_دننه_واټن(0);
            }
        }

        إذا (dumpLevel() || debug() >= 9)
            dupFinder.dumpFile(v3Global.debugFilename("trace") + ".hash", خطأ);
    }

    فراغ ګراف_Dtype_Prune() {
        لكل (V3_ګراف_راس* ثابت vtxp : m_ګراف.vertices().unlinkable()) {
            إذا (Trace_Trace_راس* ثابت vvertexp = vtxp->cast<Trace_Trace_راس>()) {
                AstTraceDecl* ثابت declp = vvertexp->nodep();
                // This skips the dtype sub-func optimization if a var is affected by multiple
                // activities.  We really only need to do this for _chg funcs (and not decls,
                // _const and _full) but it's simpiler to do it all one way or the other.
                إذا (declp) {
                    إذا (declp->user2() || (declp->dtypeDeclp() && !declp->dtypeDeclp()->user2())) {
                        AstCCall* ثابت callp = declp->dtypeCallp();
                        إذا (callp) {
                            Ast_غوټه* stmtexprp = callp->backp();
                            VL_DO_DANGLING(ټېل_Deletep(stmtexprp->unlink_Fr_شا()), stmtexprp);
                        } وإلا {
                            منطقي تشه_Scope;
                            افعل {
                                تشه_Scope = خطأ;
                                Ast_غوټه* ثابت declBackp = declp->backp();
                                Ast_غوټه* ثابت declNextp = declp->nextp();
                                إذا (VN_IS(declBackp, Trace_ټېل_مختاړی)
                                    && VN_IS(declNextp, Trace_راوباسه_مختاړی)) {
                                    VL_DO_DANGLING(ټېل_Deletep(declBackp->unlink_Fr_شا()),
                                                   declBackp);
                                    VL_DO_DANGLING(ټېل_Deletep(declNextp->unlink_Fr_شا()),
                                                   declNextp);
                                    تشه_Scope = ریښتیا;
                                }
                            } طالما (تشه_Scope);
                        }
                        // Can't purge until we finish this pass
                        ټېل_Deletep(declp->unlink_Fr_شا());  // declp used below
                        vvertexp->reroute_څنډې(&m_ګراف);
                        vvertexp->unlinkDelete(&m_ګراف);
                    }
                    declp->dtypeDeclp(تش_اشاره);
                }
            }
        }
    }

    فراغ وېشل_Trace_Decl_Funcs() {
        ثابت صحيح وېشل_حد = v3Global.opt.output_وېشل_CTrace() ? v3Global.opt.output_وېشل_CTrace()
                                                                : std::numeric_limits<صحيح>::max();
        إذا (وېشل_حد == std::numeric_limits<صحيح>::max()) أرجع;

        لكل (Ast_غوټه* nodep = m_سر_Scopep->blocksp(); nodep; nodep = nodep->nextp()) {
            AstCFunc* ثابت funcp = VN_CAST(nodep, CFunc);
            إذا (!funcp || !funcp->user2()) استمر;

            ثابت string بنسټ_Name = funcp->name();
            uint32_t sub_عدد = 1;
            صحيح cur_اندازه = 0;
            AstCFunc* curFuncp = funcp;
            std::ویکټور<Ast_غوټه_Stmt*> callStmts;

            خپلکاری پیل_New_Sub_Func = [&]() {
                File_کرښه* ثابت flp = funcp->fileline();
                ثابت string newName = بنسټ_Name + "_" + cvt_منزل_Str(sub_عدد++);
                AstCFunc* ثابت newFuncp = جديد AstCFunc{flp, newName, m_سر_Scopep};
                newFuncp->argTypes(v3Global.opt.trace_Class_بنسټ() + "* tracep");
                newFuncp->isTrace(ریښتیا);
                newFuncp->isStatic(خطأ);
                newFuncp->isLoose(ریښتیا);
                newFuncp->slow(ریښتیا);
                newFuncp->dontCombine(ریښتیا);
                m_سر_Scopep->جمع_کړه_Blocksp(newFuncp);
                newFuncp->جمع_کړه_Stmtsp(جديد AstCStmt{flp, "const int c = vlSymsp->__Vm_baseCode;"});
                AstCCall* ثابت callp = جديد AstCCall{flp, newFuncp};
                callp->dtype_سېټ_Void();
                callp->argTypes("tracep");
                callStmts.ټېل_شا(callp->makeStmt());
                curFuncp = newFuncp;
                cur_اندازه = 0;
            };

            لكل (Ast_غوټه *stmtp = funcp->stmtsp(), *nextp; stmtp; stmtp = nextp) {
                nextp = stmtp->nextp();
                cur_اندازه += stmtp->غوټه_شمېر();

                إذا (curFuncp != funcp) {
                    stmtp->unlink_Fr_شا();
                    curFuncp->جمع_کړه_Stmtsp(stmtp);
                }

                إذا (cur_اندازه > وېشل_حد && nextp) پیل_New_Sub_Func();
            }

            لكل (Ast_غوټه_Stmt* ثابت callStmtp : callStmts) { funcp->جمع_کړه_Stmtsp(callStmtp); }
        }
    }

    فراغ ګراف_Simplify(منطقي initial) {
        إذا (initial) {
            // Remove all variable nodes
            لكل (V3_ګراف_راس* ثابت vtxp : m_ګراف.vertices().unlinkable()) {
                إذا (Trace_Var_راس* ثابت vvertexp = vtxp->cast<Trace_Var_راس>()) {
                    vvertexp->reroute_څنډې(&m_ګراف);
                    vvertexp->unlinkDelete(&m_ګراف);
                }
            }
            // Remove multiple variables connecting funcs to traces
            // We do this twice, as then we have fewer edges to multiply out in the below
            // expansion.
            m_ګراف.remove_Redundant_څنډې_Max(&V3_ګراف_څنډه::followAlwaysTrue);
            // Remove all Cfunc nodes
            لكل (V3_ګراف_راس* ثابت vtxp : m_ګراف.vertices().unlinkable()) {
                إذا (Trace_CFunc_راس* ثابت vvertexp = vtxp->cast<Trace_CFunc_راس>()) {
                    vvertexp->reroute_څنډې(&m_ګراف);
                    vvertexp->unlinkDelete(&m_ګراف);
                }
            }
        }

        // Remove multiple variables connecting funcs to traces
        m_ګراف.remove_Redundant_څنډې_Max(&V3_ګراف_څنډه::followAlwaysTrue);

        // If there are any edges from a always, keep only the always
        لكل (V3_ګراف_راس& vtx : m_ګراف.vertices()) {
            إذا (Trace_Trace_راس* ثابت vvertexp = vtx.cast<Trace_Trace_راس>()) {
                // Search for the incoming always edge
                ثابت V3_ګراف_څنډه* alwaysEdgep = تش_اشاره;
                لكل (ثابت V3_ګراف_څنډه& څنډه : vvertexp->دننه_څنډې()) {
                    ثابت Trace_Activity_راس* ثابت actVtxp
                        = څنډه.fromp()->as<ثابت Trace_Activity_راس>();
                    إذا (actVtxp->activityAlways()) {
                        alwaysEdgep = &څنډه;
                        اكسر;
                    }
                }
                // If always edge exists, remove all other edges
                إذا (alwaysEdgep) {
                    لكل (V3_ګراف_څنډه* ثابت edgep : vvertexp->دننه_څنډې().unlinkable()) {
                        إذا (edgep != alwaysEdgep) VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
                    }
                }
            }
        }

        // Activity points with no outputs can be removed
        لكل (V3_ګراف_راس* ثابت vtxp : m_ګراف.vertices().unlinkable()) {
            إذا (Trace_Activity_راس* ثابت aVtxp = vtxp->cast<Trace_Activity_راس>()) {
                // Leave in the always vertex for later use.
                إذا (aVtxp != m_alwaysVtxp && aVtxp->بهر_تشه()) {
                    VL_DO_DANGLING(aVtxp->unlinkDelete(&m_ګراف), aVtxp);
                }
            }
        }
    }

    uint32_t assignactivityNumbers() {
        uint32_t activity_شمېره = 1;  // Note 0 indicates "slow" only
        لكل (V3_ګراف_راس& vtx : m_ګراف.vertices()) {
            إذا (Trace_Activity_راس* ثابت vvertexp = vtx.cast<Trace_Activity_راس>()) {
                إذا (vvertexp != m_alwaysVtxp) {
                    إذا (vvertexp->slow()) {
                        vvertexp->activity_کوډ(Trace_Activity_راس::ACTIVITY_SLOW);
                    } وإلا {
                        vvertexp->activity_کوډ(activity_شمېره++);
                    }
                }
            }
        }
        أرجع activity_شمېره;
    }

    فراغ ترتیبول_Traces(TraceVec& traces, uint32_t& nNonConstCodes) {
        // Populate sort structure
        traces.clear();
        nNonConstCodes = 0;
        لكل (V3_ګراف_راس& vtx : m_ګراف.vertices()) {
            إذا (Trace_Trace_راس* ثابت vtxp = vtx.cast<Trace_Trace_راس>()) {
                Act_کوډ_سېټ act_سېټ;
                UINFO(9, "  Add to sort: " << vtxp);
                UINFOTREE(9, vtxp->nodep(), "", "trnode");
                لكل (ثابت V3_ګراف_څنډه& څنډه : vtxp->دننه_څنډې()) {
                    ثابت Trace_Activity_راس* ثابت cfvertexp
                        = څنډه.fromp()->cast<ثابت Trace_Activity_راس>();
                    UASSERT_OBJ(cfvertexp, vtxp->nodep(),
                                "Should have been function pointing to this trace");
                    UINFO(9, "   Activity: " << cfvertexp);
                    إذا (cfvertexp->activityAlways()) {
                        // If code 0, we always trace; ignore other codes
                        act_سېټ.ورننباسه(Trace_Activity_راس::ACTIVITY_ALWAYS);
                    } وإلا {
                        act_سېټ.ورننباسه(cfvertexp->activity_کوډ());
                    }
                }
                UASSERT_OBJ(act_سېټ.شمېر(Trace_Activity_راس::ACTIVITY_ALWAYS) == 0
                                || act_سېټ.اندازه() == 1,
                            vtxp->nodep(), "Always active trace has further triggers");
                // Count nodes
                إذا (!vtxp->duplicatep() && !act_سېټ.تشه())
                    nNonConstCodes += vtxp->nodep()->کوډ_Inc();
                إذا (act_سېټ.تشه()) {
                    // If a trace doesn't have activity, it's constant, and we
                    // don't need to track changes on it.
                    act_سېټ.ورننباسه(Trace_Activity_راس::ACTIVITY_NEVER);
                } وإلا إذا (act_سېټ.شمېر(Trace_Activity_راس::ACTIVITY_SLOW) && act_سېټ.اندازه() > 1) {
                    // If a trace depends on the slow flag as well as other
                    // flags, remove the dependency on the slow flag. We will
                    // make slow routines set all activity flags.
                    act_سېټ.erase(Trace_Activity_راس::ACTIVITY_SLOW);
                }
                traces.emplace(act_سېټ, vtxp);
            }
        }
    }

    فراغ ګراف_Optimize() {
        // Assign initial activity numbers to activity vertices
        assignactivityNumbers();

        // Sort the traces by activity sets
        TraceVec traces;
        uint32_t unused1;
        ترتیبول_Traces(traces, unused1);

        // For each activity set with only a small number of signals, make those
        // signals always traced, as it's cheaper to check a few value changes
        // than to test a lot of activity flags
        خپلکاری it = traces.begin();
        ثابت خپلکاری پای_ټکی = traces.پای_ټکی();
        طالما (it != پای_ټکی) {
            خپلکاری سر_2 = it;
            // Approximate the complexity of the value change check
            uint32_t complexity = 0;
            ثابت Act_کوډ_سېټ& act_سېټ = it->لومړی;
            لكل (; it != پای_ټکی && it->لومړی == act_سېټ; ++it) {
                إذا (!it->دوهم->duplicatep()) {
                    uint32_t لګښت = 0;
                    ثابت AstTraceDecl* ثابت declp = it->دوهم->nodep();
                    // The number of comparisons required by bufp->chg*
                    لګښت += declp->isWide() ? declp->کوډ_Inc() : 1;
                    // Arrays are traced by element
                    لګښت *= declp->arrayRange().ranged() ? declp->arrayRange().elements() : 1;
                    // Note: Experiments factoring in the size of declp->valuep()
                    // showed no benefit in tracing speed, even for large trees,
                    // so we will leave those out for now.
                    complexity += لګښت;
                }
            }
            // Leave alone always changing, never changing and signals only set in slow code
            إذا (act_سېټ.شمېر(Trace_Activity_راس::ACTIVITY_ALWAYS)) استمر;
            إذا (act_سېټ.شمېر(Trace_Activity_راس::ACTIVITY_NEVER)) استمر;
            إذا (act_سېټ.شمېر(Trace_Activity_راس::ACTIVITY_SLOW)) استمر;
            // If the value comparisons are cheaper to perform than checking the
            // activity flags make the signals always traced. Note this cost
            // equation is heuristic.
            إذا (complexity <= act_سېټ.اندازه() * 2) {
                لكل (; سر_2 != it; ++سر_2) {
                    جديد V3_ګراف_څنډه{&m_ګراف, m_alwaysVtxp, سر_2->دوهم, 1};
                }
            }
        }

        ګراف_Simplify(خطأ);
    }

    Ast_غوټه_بیان* selectActivity(File_کرښه* flp, uint32_t acode, ثابت VAccess& access) {
        أرجع جديد AstArraySel(flp, جديد AstVarRef{flp, m_activityVscp, access}, acode);
    }

    Ast_غوټه* newActivitySetter(Ast_غوټه* insertp, uint32_t کوډ) {
        ++m_statSetters;
        File_کرښه* ثابت fl = insertp->fileline();
        Ast_ټاکل* ثابت setterp = جديد Ast_ټاکل{fl, selectActivity(fl, کوډ, VAccess::WRITE),
                                                 جديد AstConst{fl, AstConst::بټ_True{}}};
        أرجع setterp;
    }

    Ast_غوټه* newActivityAll(Ast_غوټه* insertp) {
        ++m_statSettersSlow;
        إذا (!m_actAllFuncp) {
            File_کرښه* ثابت flp = m_سر_Scopep->fileline();
            AstCFunc* ثابت funcp = جديد AstCFunc{flp, "__VtraceActivitySetAll", m_سر_Scopep};
            funcp->slow(ریښتیا);
            funcp->isStatic(خطأ);
            funcp->isLoose(ریښتیا);
            m_سر_Scopep->جمع_کړه_Blocksp(funcp);
            لكل (uint32_t کوډ = 0; کوډ < m_activity_شمېره; ++کوډ) {
                Ast_غوټه* ثابت setterp = newActivitySetter(insertp, کوډ);
                funcp->جمع_کړه_Stmtsp(setterp);
            }
            m_actAllFuncp = funcp;
        }
        AstCCall* ثابت callp = جديد AstCCall{insertp->fileline(), m_actAllFuncp};
        callp->dtype_سېټ_Void();
        أرجع callp->makeStmt();
    }

    فراغ createActivityFlags() {
        // Assign final activity numbers
        m_activity_شمېره = assignactivityNumbers();

        // Create an array of bytes, not a bit vector, as they can be set
        // atomically by mtasks, and are cheaper to set (no need for
        // read-modify-write on the C type), and the speed of the tracing code
        // is the same on largish designs.
        File_کرښه* ثابت flp = m_سر_Scopep->fileline();
        Ast_غوټه_DType* ثابت newScalarDtp = جديد AstBasicDType{flp, VFlag_بټ_Packed{}, 1};
        v3Global.rootp()->typeTablep()->جمع_کړه_Typesp(newScalarDtp);
        AstRange* ثابت newArange
            = جديد AstRange{flp, VNumRange{static_cast<صحيح>(m_activity_شمېره) - 1, 0}};
        Ast_غوټه_DType* ثابت newArrDtp = جديد AstUnpackArrayDType{flp, newScalarDtp, newArange};
        v3Global.rootp()->typeTablep()->جمع_کړه_Typesp(newArrDtp);
        m_activityVscp = v3Global.rootp()->سر_Scopep()->create_لنډمهاله("__VtraceActivity", newArrDtp);

        // Insert activity setters
        لكل (ثابت V3_ګراف_راس& vtx : m_ګراف.vertices()) {
            إذا (ثابت Trace_Activity_راس* ثابت vtxp = vtx.cast<ثابت Trace_Activity_راس>()) {
                Ast_غوټه* setterp = تش_اشاره;
                إذا (vtxp->activitySlow()) {
                    setterp = newActivityAll(vtxp->insertp());
                    // Just set all flags in slow code as it should be rare.
                    // This will be rolled up into a loop by V3Reloop.
                } وإلا إذا (!vtxp->activityAlways()) {
                    setterp = newActivitySetter(vtxp->insertp(), vtxp->activity_کوډ());
                }
                إذا (setterp) {
                    Ast_غوټه* ثابت insertp = vtxp->insertp();
                    إذا (Ast_Stmt_بیان* ثابت stmtp = VN_CAST(insertp, Stmt_بیان)) {
                        stmtp->جمع_کړه_بل_دلته(setterp);
                    } وإلا إذا (AstCFunc* ثابت funcp = VN_CAST(insertp, CFunc)) {
                        // If there are awaits, insert the setter after each await
                        إذا (funcp->isCoroutine() && funcp->stmtsp()) {
                            funcp->stmtsp()->foreach_And_بل([setterp](AstCAwait* awaitp) {
                                awaitp->جمع_کړه_بل_دلته(setterp->clone_ونه(خطأ));
                            });
                        }
                        funcp->جمع_کړه_Stmtsp(setterp);
                    } وإلا {
                        insertp->v3fatalSrc("Bad trace activity vertex");
                    }
                }
            }
        }
    }

    AstCFunc* newCFunc(VTraceType traceType, AstCFunc* سر_Funcp, uint32_t func_عدد,
                       uint32_t بنسټ_کوډ = 0, ثابت AstTraceDecl* ثابت declp = تش_اشاره,
                       منطقي declSub = خطأ) {
        // Create new function
        ثابت منطقي is_سر_Func = !declp && سر_Funcp == تش_اشاره;
        std::string funcName;
        إذا (is_سر_Func || declp) {
            funcName = traceType.func_مختاړی();
        } وإلا {
            funcName = سر_Funcp->name();
            funcName += "_sub";
        }
        إذا (declp) {
            funcName += "_dtype";
            إذا (declSub) funcName += "_sub";
            funcName += "__";
            funcName = m_dtype_نومونه.get(funcName);
        } وإلا {
            funcName += "_";
            funcName += cvt_منزل_Str(func_عدد);
        }

        File_کرښه* ثابت flp = m_سر_Scopep->fileline();
        AstCFunc* ثابت funcp = جديد AstCFunc{flp, funcName, m_سر_Scopep};
        funcp->isTrace(ریښتیا);
        funcp->dontCombine(ریښتیا);
        funcp->isLoose(ریښتیا);
        funcp->slow(traceType != VTraceType::CHANGE);
        funcp->isStatic(is_سر_Func);
        // Add it to top scope
        m_سر_Scopep->جمع_کړه_Blocksp(funcp);
        ثابت std::string bufArg
            = v3Global.opt.trace_Class_بنسټ() + "::Buffer* bufp"
              + (declp ? (", uint32_t offset, const " + declp->dtypep()->cType("", ریښتیا, ریښتیا)
                          + " __VdtypeVar")
                       : "");
        إذا (is_سر_Func) {
            // Top functions
            funcp->argTypes("void* voidSelf, " + bufArg);
            funcp->جمع_کړه_Stmtsp(جديد AstCStmt{flp, EmitCUtil::void_Self_ټاکل(m_سر_Modp)});
            funcp->جمع_کړه_Stmtsp(جديد AstCStmt{flp, EmitCUtil::sym_Class_ټاکل()});
            // Add global activity check to change dump functions
            إذا (traceType == VTraceType::CHANGE) {  //
                funcp->جمع_کړه_Stmtsp(
                    جديد AstCStmt{flp, "if (VL_UNLIKELY(!vlSymsp->__Vm_activity)) return;"});
            }
            // Register function
            AstCStmt* ثابت cstmtp = جديد AstCStmt{flp};
            m_regFuncp->جمع_کړه_Stmtsp(cstmtp);
            إذا (traceType == VTraceType::CONSTANT) {
                cstmtp->جمع_کړه("tracep->addConstCb(");
                cstmtp->جمع_کړه(جديد Ast_Addr_د_CFunc{flp, funcp});
                cstmtp->جمع_کړه(", " + std::منزل_string(func_عدد) + ", vlSelf);");
            } وإلا إذا (traceType == VTraceType::ډک) {
                cstmtp->جمع_کړه("tracep->addFullCb(");
                cstmtp->جمع_کړه(جديد Ast_Addr_د_CFunc{flp, funcp});
                cstmtp->جمع_کړه(", " + std::منزل_string(func_عدد) + ", vlSelf);");
            } وإلا {
                cstmtp->جمع_کړه("tracep->addChgCb(");
                cstmtp->جمع_کړه(جديد Ast_Addr_د_CFunc{flp, funcp});
                cstmtp->جمع_کړه(", " + std::منزل_string(func_عدد) + ", vlSelf);");
            }
        } وإلا {
            // Sub functions
            funcp->argTypes(bufArg);
            // Setup base references. Note in rare occasions we can end up with an empty trace
            // sub function, hence the VL_ATTR_UNUSED attributes.
            إذا (traceType != VTraceType::CHANGE) {
                // Full dump sub function
                funcp->جمع_کړه_Stmtsp(جديد AstCStmt{flp,  //
                                              string("uint32_t* const oldp VL_ATTR_UNUSED = "
                                                     "bufp->oldp(vlSymsp->__Vm_baseCode")
                                                  + (declp ? " + offset" : "") + ");\n"});
            } وإلا {
                // Change dump sub function
                funcp->جمع_کړه_Stmtsp(جديد AstCStmt{flp,  //
                                              "uint32_t* const oldp VL_ATTR_UNUSED = "
                                              "bufp->oldp(vlSymsp->__Vm_baseCode + "
                                                  + (declp ? " offset" : cvt_منزل_Str(بنسټ_کوډ))
                                                  + ");\n"});
            }
            إذا (!declp) {
                // Add call to top function
                AstCCall* ثابت callp = جديد AstCCall{funcp->fileline(), funcp};
                callp->dtype_سېټ_Void();
                callp->argTypes("bufp");
                سر_Funcp->جمع_کړه_Stmtsp(callp->makeStmt());
            }
        }
        //بشپړ
        UINFO(5, "  newCFunc " << funcp);
        أرجع funcp;
    }

    AstCFunc* newCDtypeSubFunc(VTraceType traceType, ثابت AstTraceDecl* ثابت declp,
                               AstCFunc* parentp) {
        AstCFunc* ثابت funcp = newCFunc(traceType, تش_اشاره, 0, 0, declp, ریښتیا);
        File_کرښه* ثابت flp = m_سر_Scopep->fileline();
        AstCCall* ثابت callp = جديد AstCCall{flp, funcp};
        callp->dtype_سېټ_Void();
        callp->argTypes("bufp, offset, __VdtypeVar");
        parentp->جمع_کړه_Stmtsp(callp->makeStmt());
        أرجع funcp;
    }

    AstCFunc* createConstDtypeTraceFunctions(ثابت AstTraceDecl* declp) {
        ثابت Ast_غوټه_DType* ثابت dtypep = declp->dtypep()->skipRefp();
        خپلکاری pair = m_dtypeConstFuncs.emplace(dtypep, تش_اشاره);
        إذا (pair.دوهم) {
            File_کرښه* ثابت flp = declp->fileline();
            AstCFunc* ثابت funcp = newCFunc(VTraceType::CONSTANT, تش_اشاره, 0, 0, declp);

            منطقي لومړی = ریښتیا;
            لكل (Ast_غوټه* callStmtp = declp->dtypeCallp()->funcp()->stmtsp(); callStmtp;
                 callStmtp = callStmtp->nextp()) {
                ثابت AstCCall* ثابت callp
                    = VN_CAST(VN_CAST(callStmtp, Stmt_بیان)->exprp(), CCall);
                منطقي onlyOne = لومړی && !callStmtp->nextp();
                AstCFunc* ثابت subFuncp
                    = onlyOne ? funcp : newCDtypeSubFunc(VTraceType::CONSTANT, declp, funcp);
                لكل (Ast_غوټه* stmtp = callp->funcp()->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    إذا (AstTraceDecl* ثابت ډګر_Declp = VN_CAST(stmtp, TraceDecl)) {
                        AstTraceInc* ثابت incp
                            = جديد AstTraceInc{flp, ډګر_Declp, VTraceType::CONSTANT};
                        subFuncp->جمع_کړه_Stmtsp(incp);
                    }
                }
                لومړی = خطأ;
            }

            pair.لومړی->دوهم = funcp;
        }

        أرجع pair.لومړی->دوهم;
    }

    فراغ createConstTraceFunctions(ثابت TraceVec& traces) {
        ثابت صحيح وېشل_حد = v3Global.opt.output_وېشل_CTrace() ? v3Global.opt.output_وېشل_CTrace()
                                                                : std::numeric_limits<صحيح>::max();

        AstCFunc* ثابت سر_Funcp = newCFunc(VTraceType::CONSTANT, تش_اشاره, 0);
        uint32_t sub_Func_عدد = 0;
        AstCFunc* subFuncp = تش_اشاره;
        صحيح subStmts = 0;
        std::ویکټور<ثابت Trace_Trace_راس*> duplicates;
        لكل (خپلکاری it = traces.cbegin(); it != traces.پای_ټکی(); ++it) {
            ثابت Trace_Trace_راس* ثابت vtxp = it->دوهم;
            AstTraceDecl* ثابت declp = vtxp->nodep();
            إذا (vtxp->duplicatep()) {
                duplicates.ټېل_شا(vtxp);
                استمر;
            }

            // This is a canonical trace node. Assign trace code (signal number).
            UASSERT_OBJ(!declp->کوډ_Assigned(), declp,
                        "Canonical node should not have code assigned yet");
            declp->کوډ(m_کوډ);
            ثابت uint32_t کوډ_Inc = declp->کوډ_Inc();
            m_کوډ += کوډ_Inc;
            ++m_statUniqSigs;

            // If this is a const signal, add the AstTraceInc
            ثابت Act_کوډ_سېټ& act_سېټ = it->لومړی;
            إذا (act_سېټ.شمېر(Trace_Activity_راس::ACTIVITY_NEVER)) {
                // Crate new sub function if required
                إذا (!subFuncp || subStmts > وېشل_حد) {
                    subStmts = 0;
                    subFuncp = newCFunc(VTraceType::CONSTANT, سر_Funcp, sub_Func_عدد);
                    ++sub_Func_عدد;
                }
                File_کرښه* ثابت flp = declp->fileline();
                إذا (declp->dtypeCallp()) {
                    AstCFunc* ثابت funcp = createConstDtypeTraceFunctions(declp);
                    Ast_غوټه_بیان* argsp = تش_اشاره;
                    argsp = Ast_غوټه::جمع_کړه_بل(argsp, declp->valuep()->clone_ونه(خطأ));
                    AstCCall* ثابت callp = جديد AstCCall{flp, funcp, argsp};
                    callp->dtype_سېټ_Void();
                    callp->argTypes(callp->argTypes() + "bufp, " + std::منزل_string(declp->کوډ()));
                    subFuncp->جمع_کړه_Stmtsp(callp->makeStmt());

                    subStmts += 1;
                } وإلا {
                    AstTraceInc* ثابت incp = جديد AstTraceInc{flp, declp, VTraceType::CONSTANT};
                    subFuncp->جمع_کړه_Stmtsp(incp);
                    subStmts += incp->غوټه_شمېر();
                }
            }
        }

        // Assign codes to duplicate nodes from their canonicals
        لكل (ثابت Trace_Trace_راس* ثابت vtxp : duplicates) {
            AstTraceDecl* ثابت declp = vtxp->nodep();
            ثابت Trace_Trace_راس* ثابت canonVtxp = vtxp->duplicatep();
            ثابت AstTraceDecl* ثابت canonDeclp = canonVtxp->nodep();
            UASSERT_OBJ(!canonVtxp->duplicatep(), canonDeclp, "Canonical node is a duplicate");
            UASSERT_OBJ(canonDeclp->کوډ_Assigned(), canonDeclp,
                        "Canonical node should have code assigned already");
            declp->کوډ(canonDeclp->کوډ() + vtxp->dtype_Alias_دننه_واټن());
        }
    }

    DtypeFuncs createNonConstDtypeTraceFunctions(ثابت AstTraceDecl* declp) {
        Ast_غوټه_DType* dtypep = declp->dtypep()->skipRefp();
        خپلکاری pair = m_dtypeNonConstFuncs.emplace(dtypep, DtypeFuncs{});
        إذا (pair.دوهم) {
            File_کرښه* ثابت flp = declp->fileline();
            AstCFunc* ثابت ډک_Funcp = newCFunc(VTraceType::ډک, تش_اشاره, 0, 0, declp);
            AstCFunc* ثابت chgFuncp = newCFunc(VTraceType::CHANGE, تش_اشاره, 0, 0, declp);

            منطقي لومړی = ریښتیا;
            لكل (Ast_غوټه* callStmtp = declp->dtypeCallp()->funcp()->stmtsp(); callStmtp;
                 callStmtp = callStmtp->nextp()) {
                ثابت AstCCall* ثابت callp
                    = VN_CAST(VN_CAST(callStmtp, Stmt_بیان)->exprp(), CCall);
                منطقي onlyOne = لومړی && !callStmtp->nextp();
                AstCFunc* ثابت ډک_Sub_Funcp
                    = onlyOne ? ډک_Funcp : newCDtypeSubFunc(VTraceType::ډک, declp, ډک_Funcp);
                AstCFunc* ثابت chgSubFuncp
                    = onlyOne ? chgFuncp : newCDtypeSubFunc(VTraceType::CHANGE, declp, chgFuncp);
                لكل (Ast_غوټه* stmtp = callp->funcp()->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    إذا (AstTraceDecl* ثابت ډګر_Declp = VN_CAST(stmtp, TraceDecl)) {
                        AstTraceInc* ثابت incFullp
                            = جديد AstTraceInc{flp, ډګر_Declp, VTraceType::ډک};
                        ډک_Sub_Funcp->جمع_کړه_Stmtsp(incFullp);
                        AstTraceInc* ثابت incChgp
                            = جديد AstTraceInc{flp, ډګر_Declp, VTraceType::CHANGE};
                        chgSubFuncp->جمع_کړه_Stmtsp(incChgp);
                    }
                }
                لومړی = خطأ;
            }

            pair.لومړی->دوهم = {.ډک_Funcp = ډک_Funcp, .chgFuncp = chgFuncp};
        }

        أرجع pair.لومړی->دوهم;
    }

    فراغ createNonConstTraceFunctions(ثابت TraceVec& traces, uint32_t nAllCodes,
                                      uint32_t parallelism) {
        ثابت صحيح وېشل_حد = v3Global.opt.output_وېشل_CTrace() ? v3Global.opt.output_وېشل_CTrace()
                                                                : std::numeric_limits<صحيح>::max();

        // pre-incremented, so starts at 0
        uint32_t سر_Func_عدد = std::numeric_limits<uint32_t>::max();
        TraceVec::const_iterator it = traces.begin();
        طالما (it != traces.پای_ټکی()) {
            AstCFunc* سر_Ful_Funcp = تش_اشاره;
            AstCFunc* سر_Chg_Funcp = تش_اشاره;
            AstCFunc* subFulFuncp = تش_اشاره;
            AstCFunc* subChgFuncp = تش_اشاره;
            uint32_t sub_Func_عدد = 0;
            صحيح subStmts = 0;
            ثابت uint32_t maxCodes = std::max((nAllCodes + parallelism - 1) / parallelism, 1U);
            uint32_t nCodes = 0;
            ثابت Act_کوډ_سېټ* prev_Act_سېټ = تش_اشاره;
            AstIf* ifp = تش_اشاره;
            uint32_t بنسټ_کوډ = 0;
            لكل (; nCodes < maxCodes && it != traces.پای_ټکی(); ++it) {
                ثابت Act_کوډ_سېټ& act_سېټ = it->لومړی;
                // Traced value never changes, no need to add it
                إذا (act_سېټ.شمېر(Trace_Activity_راس::ACTIVITY_NEVER)) استمر;

                ثابت Trace_Trace_راس* ثابت vtxp = it->دوهم;
                AstTraceDecl* ثابت declp = vtxp->nodep();

                // This is a duplicate decl, no need to add it, but must set the
                // function index to the same as the canonical node.
                إذا (ثابت Trace_Trace_راس* ثابت canonVtxp = vtxp->duplicatep()) {
                    declp->fidx(canonVtxp->nodep()->fidx());
                    استمر;
                }

                // Create top function if not yet created
                إذا (!سر_Ful_Funcp) {
                    ++سر_Func_عدد;
                    سر_Ful_Funcp = newCFunc(VTraceType::ډک, تش_اشاره, سر_Func_عدد);
                    سر_Chg_Funcp = newCFunc(VTraceType::CHANGE, تش_اشاره, سر_Func_عدد);
                }

                // Create new sub function if required
                إذا (!subFulFuncp || subStmts > وېشل_حد) {
                    بنسټ_کوډ = declp->کوډ();
                    subStmts = 0;
                    subFulFuncp = newCFunc(VTraceType::ډک, سر_Ful_Funcp, sub_Func_عدد, بنسټ_کوډ);
                    subChgFuncp = newCFunc(VTraceType::CHANGE, سر_Chg_Funcp, sub_Func_عدد, بنسټ_کوډ);
                    ++sub_Func_عدد;
                    prev_Act_سېټ = تش_اشاره;
                    ifp = تش_اشاره;
                }

                // If required, create the conditional node checking the activity flags
                إذا (!prev_Act_سېټ || act_سېټ != *prev_Act_سېټ) {
                    File_کرښه* ثابت flp = m_سر_Scopep->fileline();
                    ثابت منطقي always = act_سېټ.شمېر(Trace_Activity_راس::ACTIVITY_ALWAYS) != 0;
                    Ast_غوټه_بیان* condp = تش_اشاره;
                    إذا (always) {
                        condp = جديد AstConst{flp, 1};  // Always true, will be folded later
                    } وإلا {
                        لكل (ثابت uint32_t act_کوډ : act_سېټ) {
                            Ast_غوټه_بیان* ثابت selp = selectActivity(flp, act_کوډ, VAccess::READ);
                            condp = condp ? جديد AstOr{flp, condp, selp} : selp;
                        }
                    }
                    ifp = جديد AstIf{flp, condp};
                    إذا (!always) ifp->branchPred(VBranchPred::BP_UNLIKELY);
                    subChgFuncp->جمع_کړه_Stmtsp(ifp);
                    subStmts += ifp->غوټه_شمېر();
                    prev_Act_سېټ = &act_سېټ;
                }

                // Add TraceInc nodes
                File_کرښه* ثابت flp = declp->fileline();
                إذا (declp->dtypeCallp()) {
                    DtypeFuncs funcs = createNonConstDtypeTraceFunctions(declp);
                    Ast_غوټه_بیان* argsp = تش_اشاره;
                    argsp = Ast_غوټه::جمع_کړه_بل(argsp, declp->valuep()->clone_ونه(خطأ));
                    AstCCall* ثابت callFullp = جديد AstCCall{flp, funcs.ډک_Funcp, argsp};
                    callFullp->dtype_سېټ_Void();
                    callFullp->argTypes(callFullp->argTypes() + "bufp, "
                                        + std::منزل_string(declp->کوډ()));
                    subFulFuncp->جمع_کړه_Stmtsp(callFullp->makeStmt());
                    argsp = تش_اشاره;
                    argsp = Ast_غوټه::جمع_کړه_بل(argsp, declp->valuep()->clone_ونه(خطأ));
                    AstCCall* ثابت callChgp = جديد AstCCall{flp, funcs.chgFuncp, argsp};
                    callChgp->dtype_سېټ_Void();
                    callChgp->argTypes(callChgp->argTypes() + "bufp, "
                                       + std::منزل_string(declp->کوډ()));
                    ifp->جمع_کړه_Thensp(callChgp->makeStmt());

                    subStmts += 2;
                } وإلا {
                    AstTraceInc* ثابت incFulp = جديد AstTraceInc{flp, declp, VTraceType::ډک};
                    subFulFuncp->جمع_کړه_Stmtsp(incFulp);
                    AstTraceInc* ثابت incChgp
                        = جديد AstTraceInc{flp, declp, VTraceType::CHANGE, بنسټ_کوډ};
                    ifp->جمع_کړه_Thensp(incChgp);

                    // Set the function index of the decl
                    declp->fidx(سر_Func_عدد);

                    // Track splitting due to size
                    UASSERT_OBJ(incFulp->غوټه_شمېر() == incChgp->غوټه_شمېر(), declp,
                                "Should have equal cost");
                    ثابت VNumRange range = declp->arrayRange();
                    إذا (range.ranged()) {
                        // 2x because each element is a TraceInc and a VarRef
                        subStmts += range.elements() * 2;
                    } وإلا {
                        subStmts += incChgp->غوټه_شمېر();
                    }
                }

                // Track partitioning
                nCodes += declp->کوډ_Inc();
            }
        }
    }

    فراغ createCleanupFunction() {
        File_کرښه* ثابت fl = m_سر_Scopep->fileline();
        AstCFunc* ثابت cleanupFuncp = جديد AstCFunc{fl, "trace_cleanup", m_سر_Scopep};
        cleanupFuncp->argTypes("void* voidSelf, " + v3Global.opt.trace_Class_بنسټ()
                               + "* /*unused*/");
        cleanupFuncp->isTrace(ریښتیا);
        cleanupFuncp->slow(خطأ);
        cleanupFuncp->isStatic(ریښتیا);
        cleanupFuncp->isLoose(ریښتیا);
        m_سر_Scopep->جمع_کړه_Blocksp(cleanupFuncp);
        cleanupFuncp->جمع_کړه_Stmtsp(جديد AstCStmt{fl, EmitCUtil::void_Self_ټاکل(m_سر_Modp)});
        cleanupFuncp->جمع_کړه_Stmtsp(جديد AstCStmt{fl, EmitCUtil::sym_Class_ټاکل()});

        // Register it
        {
            AstCStmt* ثابت cstmtp = جديد AstCStmt{fl};
            m_regFuncp->جمع_کړه_Stmtsp(cstmtp);
            cstmtp->جمع_کړه("tracep->addCleanupCb(");
            cstmtp->جمع_کړه(جديد Ast_Addr_د_CFunc{fl, cleanupFuncp});
            cstmtp->جمع_کړه(", vlSelf);");
        }

        // Clear global activity flag
        cleanupFuncp->جمع_کړه_Stmtsp(
            جديد AstCStmt{m_سر_Scopep->fileline(), "vlSymsp->__Vm_activity = false;"s});

        // Clear fine grained activity flags
        لكل (uint32_t i = 0; i < m_activity_شمېره; ++i) {
            Ast_غوټه* ثابت clrp = جديد Ast_ټاکل{fl, selectActivity(fl, i, VAccess::WRITE),
                                                جديد AstConst{fl, AstConst::بټ_False{}}};
            cleanupFuncp->جمع_کړه_Stmtsp(clrp);
        }
    }

    فراغ createTraceFunctions() {
        // Detect and remove duplicate values
        detectDuplicates();

        ګراف_Dtype_Prune();
        وېشل_Trace_Decl_Funcs();
        m_ګراف.remove_Redundant_څنډې_Max(&V3_ګراف_څنډه::followAlwaysTrue);

        // Simplify & optimize the graph
        إذا (dump_ګراف_Level() >= 6) m_ګراف.dumpDotFilePrefixed("trace_pre");
        ګراف_Simplify(ریښتیا);
        إذا (dump_ګراف_Level() >= 6) m_ګراف.dumpDotFilePrefixed("trace_simplified");
        ګراف_Optimize();
        إذا (dump_ګراف_Level() >= 6) m_ګراف.dumpDotFilePrefixed("trace_optimized");

        // Create the fine grained activity flags
        createActivityFlags();

        // Form a sorted list of the traces we are interested in
        TraceVec traces;  // The sorted traces
        // We will split functions such that each have to dump roughly the same amount of data
        // for this we need to keep tack of the number of codes used by the trace functions.
        uint32_t nNonConstCodes = 0;
        ترتیبول_Traces(traces, nNonConstCodes);

        // Our keys are now sorted to have same activity number adjacent, then
        // by trace order. (Better would be execution order for cache
        // efficiency....) Last are constants and non-changers, as then the
        // last value vector is more compact

        // Create the trace registration function
        m_regFuncp = جديد AstCFunc{m_سر_Scopep->fileline(), "trace_register", m_سر_Scopep};
        m_regFuncp->argTypes(v3Global.opt.trace_Class_بنسټ() + "* tracep");
        m_regFuncp->ثبت_ټکی(ریښتیا);
        m_regFuncp->isTrace(ریښتیا);
        m_regFuncp->slow(ریښتیا);
        m_regFuncp->isStatic(خطأ);
        m_regFuncp->isLoose(ریښتیا);
        m_سر_Scopep->جمع_کړه_Blocksp(m_regFuncp);

        // Create the const dump functions. Also allocates trace codes.
        createConstTraceFunctions(traces);

        // Create the full and incremental dump functions
        createNonConstTraceFunctions(traces, nNonConstCodes, m_parallelism);

        // Root-traced libraries alias wrapper IOs onto the existing top-module codes.
        إذا (!v3Global.opt.libCreate().تشه()) {
            std::ویکټور<AstTraceDecl*> ریښه_Declps;
            std::ویکټور<AstTraceDecl*> سر_Declps;
            TraceInitDeclCollector ریښه_Collector{ریښه_Declps};
            TraceInitDeclCollector سر_Collector{سر_Declps};
            لكل (Ast_غوټه* blockp = m_سر_Scopep->blocksp(); blockp; blockp = blockp->nextp()) {
                AstCFunc* ثابت funcp = VN_CAST(blockp, CFunc);
                إذا (funcp && VString::startsWith(funcp->name(), "trace_init_leaf_root__")) {
                    ریښه_Collector.collect(funcp);
                } وإلا إذا (funcp && VString::startsWith(funcp->name(), "trace_init_leaf_top__")) {
                    سر_Collector.collect(funcp);
                }
            }

            std::ویکټور<منطقي> کارول_شوی(سر_Declps.اندازه(), خطأ);
            لكل (AstTraceDecl* ثابت ریښه_Declp : ریښه_Declps) {
                لكل (اندازه_t i = 0; i < سر_Declps.اندازه(); ++i) {
                    إذا (کارول_شوی[i] || !same_ریښه_Init_Alias(ریښه_Declp, سر_Declps[i])) استمر;
                    ریښه_Declp->کوډ(سر_Declps[i]->کوډ());
                    کارول_شوی[i] = ریښتیا;
                    اكسر;
                }
            }
        }

        // Remove refs to traced values from TraceDecl nodes, these have now moved under
        // TraceInc
        لكل (ثابت خپلکاری& i : traces) {
            Ast_غوټه* ثابت valuep = i.دوهم->nodep()->valuep();
            VL_DO_DANGLING(valuep->unlink_Fr_شا()->delete_ونه(), valuep);
        }

        // Create the trace cleanup function clearing the activity flags
        createCleanupFunction();
    }

    Trace_CFunc_راس* getCFuncVertexp(AstCFunc* nodep) {
        V3_ګراف_راس* ثابت vtxp = nodep->user1u().منزل_ګراف_راس();
        Trace_CFunc_راس* vertexp = vtxp ? vtxp->cast<Trace_CFunc_راس>() : تش_اشاره;
        إذا (!vertexp) {
            vertexp = جديد Trace_CFunc_راس{&m_ګراف, nodep};
            nodep->user1p(vertexp);
        }
        أرجع vertexp;
    }
    Trace_Activity_راس* getActivityVertexp(Ast_غوټه* nodep, منطقي slow) {
        V3_ګراف_راس* ثابت vtxp = nodep->user3u().منزل_ګراف_راس();
        Trace_Activity_راس* vertexp = vtxp ? vtxp->cast<Trace_Activity_راس>() : تش_اشاره;
        إذا (!vertexp) {
            vertexp = جديد Trace_Activity_راس{&m_ګراف, nodep, slow};
            nodep->user3p(vertexp);
        }
        vertexp->slow(slow);
        أرجع vertexp;
    }

    // VISITORS
    فراغ لیدنه(AstNetlist* nodep) override {
        // Add vertexes for all TraceDecl, and edges from VARs each trace looks at
        m_finding = خطأ;
        iterateChildren(nodep);

        // Add vertexes for all CFUNCs, and edges to VARs the func sets
        m_finding = ریښتیا;
        iterateChildren(nodep);

        // Create the trace functions and insert them into the tree
        createTraceFunctions();

        // Save number of trace codes used
        nodep->nTraceCodes(m_کوډ);
    }
    فراغ لیدنه(Ast_غوټه_Module* nodep) override {
        إذا (nodep->is_سر()) m_سر_Modp = nodep;
        iterateChildren(nodep);
    }
    فراغ لیدنه(AstVarScope* nodep) override {
        إذا (!m_finding) {
            إذا (ثابت AstIface* ثابت ifacep = nodep->varp()->sensIfacep()) {
                m_ifaceMemberVscps[{ifacep, nodep->varp()->name()}].ټېل_شا(nodep);
            }
        }
    }
    فراغ لیدنه(Ast_Stmt_بیان* nodep) override {
        إذا (!m_finding && !nodep->user2()) {
            إذا (AstCCall* ثابت callp = VN_CAST(nodep->exprp(), CCall)) {
                UINFO(8, "   CCALL " << callp);
                // See if there are other calls in same statement list;
                // If so, all funcs might share the same activity code
                Trace_Activity_راس* ثابت activityVtxp
                    = getActivityVertexp(nodep, callp->funcp()->slow());
                لكل (Ast_غوټه* nextp = nodep; nextp; nextp = nextp->nextp()) {
                    إذا (Ast_Stmt_بیان* ثابت stmtp = VN_CAST(nextp, Stmt_بیان)) {
                        إذا (AstCCall* ثابت ccallp = VN_CAST(stmtp->exprp(), CCall)) {
                            stmtp->user2(ریښتیا);  // Processed
                            UINFO(8, "     SubCCALL " << ccallp);
                            V3_ګراف_راس* ثابت ccallFuncVtxp = getCFuncVertexp(ccallp->funcp());
                            activityVtxp->slow(ccallp->funcp()->slow());
                            جديد V3_ګراف_څنډه{&m_ګراف, activityVtxp, ccallFuncVtxp, 1};
                        }
                    }
                }
            }
        }
        iterateChildren(nodep);
    }
    فراغ لیدنه(AstCFunc* nodep) override {
        UINFO(8, "   CFUNC " << nodep);
        V3_ګراف_راس* ثابت funcVtxp = getCFuncVertexp(nodep);
        إذا (!m_finding) {  // If public, we need a unique activity code to allow for sets
                           // directly in this func
            إذا (nodep->func_عامه() || nodep->dpiExportImpl() || nodep->ثبت_ټکی()
                || nodep->isCoroutine()) {
                // Cannot treat a coroutine as slow, it may be resumed later
                ثابت منطقي slow = nodep->slow() && !nodep->isCoroutine();
                Trace_Activity_راس* ثابت activityVtxp = getActivityVertexp(nodep, slow);
                جديد V3_ګراف_څنډه{&m_ګراف, activityVtxp, funcVtxp, 1};
            }
        }
        VL_RESTORER(m_cfuncp);
        m_cfuncp = nodep;
        iterateChildren(nodep);
    }
    فراغ لیدنه(AstTraceDecl* nodep) override {
        UINFO(8, "   TRACE " << nodep);
        إذا (!m_finding && !nodep->دننه_Dtype_Func()) {
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
            V3_ګراف_راس* ثابت vertexp = جديد Trace_Trace_راس{&m_ګراف, nodep};
            nodep->user1p(vertexp);

            UASSERT_OBJ(m_cfuncp, nodep, "Trace not under func");
            m_cfuncp->user2(ریښتیا);
            VL_RESTORER(m_tracep);
            m_tracep = nodep;
            iterateChildren(nodep);
        }
    }
    فراغ لیدنه(AstVarRef* nodep) override {
        UASSERT_OBJ(nodep->varScopep(), nodep, "No var scope?");
        AstVarScope* ثابت varscopep = nodep->varScopep();
        V3_ګراف_راس* varVtxp = varscopep->user1u().منزل_ګراف_راس();
        إذا (m_tracep) {
            UASSERT_OBJ(nodep->access().isReadOnly(), nodep, "Lvalue in trace?  Should be const.");
            إذا (!varVtxp) {
                varVtxp = جديد Trace_Var_راس{&m_ګراف, nodep->varScopep()};
                nodep->varScopep()->user1p(varVtxp);
            }
            V3_ګراف_راس* ثابت traceVtxp = m_tracep->user1u().منزل_ګراف_راس();
            جديد V3_ګراف_څنډه{&m_ګراف, varVtxp, traceVtxp, 1};
            إذا (nodep->varp()->isPrimaryInish()  // Always need to trace primary inputs
                || nodep->varp()->is_Sig_عامه()) {  // Or ones user can change
                جديد V3_ګراف_څنډه{&m_ګراف, m_alwaysVtxp, traceVtxp, 1};
            }
            إذا (m_tracep->dtypeCallp()) varscopep->user2p(m_tracep);
        } وإلا إذا (m_cfuncp && m_finding && nodep->access().isWriteOrRW()) {
            V3_ګراف_راس* ثابت funcVtxp = getCFuncVertexp(m_cfuncp);
            إذا (varVtxp) {  // else we're not tracing this signal
                جديد V3_ګراف_څنډه{&m_ګراف, funcVtxp, varVtxp, 1};
                AstTraceDecl* ثابت declp = VN_AS(varscopep->user2p(), TraceDecl);
                إذا (declp) {
                    V3_ګراف_راس* ثابت cFuncVtxp = getCFuncVertexp(m_cfuncp);
                    لكل (ثابت V3_ګراف_څنډه& څنډه : cFuncVtxp->دننه_څنډې()) {
                        V3_ګراف_راس* ثابت activityp = څنډه.fromp();
                        إذا (!declp->user3p()) {
                            declp->user3p(activityp);
                        } وإلا إذا (declp->user3u().منزل_ګراف_راس() != activityp) {
                            declp->user2(ریښتیا);
                        }
                    }
                }
            }
        }
    }
    فراغ لیدنه(AstMemberSel* nodep) override {
        إذا (m_cfuncp && m_finding && nodep->access().isWriteOrRW()) {
            AstIfaceRefDType* ثابت dtypep
                = VN_CAST(nodep->fromp()->dtypep()->skipRefp(), IfaceRefDType);
            إذا (dtypep && dtypep->isVirtual()) {
                ثابت خپلکاری it = m_ifaceMemberVscps.پیدا_کړه({dtypep->ifacep(), nodep->varp()->name()});
                إذا (it != m_ifaceMemberVscps.پای_ټکی()) {
                    V3_ګراف_راس* ثابت funcVtxp = getCFuncVertexp(m_cfuncp);
                    لكل (AstVarScope* ثابت vscp : it->دوهم) {
                        V3_ګراف_راس* varVtxp = vscp->user1u().منزل_ګراف_راس();
                        إذا (!varVtxp) {
                            varVtxp = جديد Trace_Var_راس{&m_ګراف, vscp};
                            vscp->user1p(varVtxp);
                        }
                        جديد V3_ګراف_څنډه{&m_ګراف, funcVtxp, varVtxp, 1};
                    }
                }
            }
        }
        iterateChildren(nodep);
    }
    //--------------------
    فراغ لیدنه(Ast_غوټه* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    څرګند TraceVisitor(AstNetlist* nodep)
        : m_alwaysVtxp{جديد Trace_Activity_راس{&m_ګراف, Trace_Activity_راس::ACTIVITY_ALWAYS}} {
        nodep->user2_Clear_ونه();  // TraceDecl multiple activities flag
        nodep->user3_Clear_ونه();  // TraceDecl TraceActivityVertex (assumes we start at nullptr)
        iterate(nodep);
        nodep->foreach([](AstTraceDecl* ثابت declp) {
            إذا (declp->دننه_Dtype_Func()) {
                declp->valuep()->unlink_Fr_شا()->delete_ونه();
                declp->valuep(تش_اشاره);
            }
        });
    }
    ~TraceVisitor() override {
        V3Stats::جمع_کړه_Stat("Tracing, Activity setters", m_statSetters);
        V3Stats::جمع_کړه_Stat("Tracing, Activity slow blocks", m_statSettersSlow);
        V3Stats::جمع_کړه_Stat("Tracing, Unique trace codes", m_کوډ);
        V3Stats::جمع_کړه_Stat("Tracing, Unique traced signals", m_statUniqSigs);
    }
};

//######################################################################
// Trace class functions

فراغ V3Trace::traceAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { TraceVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_ونه("trace", 0, dump_ونه_Either_Level() >= 3);
}
