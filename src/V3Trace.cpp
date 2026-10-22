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

صنف Trace_Activity_راس final : عام V3_گراف_راس {
    VL_RTTI_IMPL(Trace_Activity_راس, V3_گراف_راس)
    Ast_گرہ* ثابت m_insertp;
    int32_t m_activity_کوڈ;
    منطقي m_slow;  // If always slow, we can use the same code
عام:
    تعداد { ACTIVITY_NEVER = ((1UL << 31) - 1) };
    تعداد { ACTIVITY_ALWAYS = ((1UL << 31) - 2) };
    تعداد { ACTIVITY_SLOW = 0 };
    Trace_Activity_راس(V3_گراف* graphp, Ast_گرہ* nodep, منطقي slow)
        : V3_گراف_راس{graphp}
        , m_insertp{nodep} {
        m_activity_کوڈ = 0;
        m_slow = slow;
    }
    Trace_Activity_راس(V3_گراف* graphp, int32_t کوڈ)
        : V3_گراف_راس{graphp}
        , m_insertp{خالی_سوچک} {
        m_activity_کوڈ = کوڈ;
        m_slow = خطأ;
    }
    ~Trace_Activity_راس() override = افتراضي;
    // ACCESSORS
    Ast_گرہ* insertp() ثابت {
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
    int32_t activity_کوڈ() ثابت { أرجع m_activity_کوڈ; }
    منطقي activityAlways() ثابت { أرجع activity_کوڈ() == ACTIVITY_ALWAYS; }
    منطقي activitySlow() ثابت { أرجع activity_کوڈ() == ACTIVITY_SLOW; }
    فراغ activity_کوڈ(int32_t کوڈ) { m_activity_کوڈ = کوڈ; }
    منطقي slow() ثابت { أرجع m_slow; }
    فراغ slow(منطقي flag) {
        إذا (!flag) m_slow = خطأ;
    }
};

صنف Trace_CFunc_راس final : عام V3_گراف_راس {
    VL_RTTI_IMPL(Trace_CFunc_راس, V3_گراف_راس)
    AstCFunc* ثابت m_nodep;

عام:
    Trace_CFunc_راس(V3_گراف* graphp, AstCFunc* nodep)
        : V3_گراف_راس{graphp}
        , m_nodep{nodep} {}
    ~Trace_CFunc_راس() override = افتراضي;
    // ACCESSORS
    AstCFunc* nodep() ثابت { أرجع m_nodep; }
    string name() ثابت override { أرجع nodep()->name(); }
    string dotColor() ثابت override { أرجع "yellow"; }
    File_سطر* fileline() ثابت override { أرجع nodep()->fileline(); }
};

صنف Trace_Trace_راس final : عام V3_گراف_راس {
    VL_RTTI_IMPL(Trace_Trace_راس, V3_گراف_راس)
    AstTraceDecl* ثابت m_nodep;  // TRACEINC this represents
    // nullptr, or other vertex with the real code() that duplicates this one
    Trace_Trace_راس* m_duplicatep = خالی_سوچک;
    // When aliasing to a dtype parent, offset of the member within the dtype
    uint32_t m_dtype_Alias_وتھ = 0;

عام:
    Trace_Trace_راس(V3_گراف* graphp, AstTraceDecl* nodep)
        : V3_گراف_راس{graphp}
        , m_nodep{nodep} {}
    ~Trace_Trace_راس() override = افتراضي;
    // ACCESSORS
    AstTraceDecl* nodep() ثابت { أرجع m_nodep; }
    string name() ثابت override { أرجع nodep()->name(); }
    string dotColor() ثابت override { أرجع "red"; }
    File_سطر* fileline() ثابت override { أرجع nodep()->fileline(); }
    Trace_Trace_راس* duplicatep() ثابت { أرجع m_duplicatep; }
    فراغ duplicatep(Trace_Trace_راس* dupp) {
        UASSERT_OBJ(!duplicatep(), nodep(), "Assigning duplicatep() to already duplicated node");
        m_duplicatep = dupp;
    }
    فراغ redirectDuplicatep(Trace_Trace_راس* dupp) { m_duplicatep = dupp; }
    uint32_t dtype_Alias_وتھ() ثابت { أرجع m_dtype_Alias_وتھ; }
    فراغ dtype_Alias_وتھ(uint32_t وتھ) { m_dtype_Alias_وتھ = وتھ; }
};

صنف Trace_Var_راس final : عام V3_گراف_راس {
    VL_RTTI_IMPL(Trace_Var_راس, V3_گراف_راس)
    AstVarScope* ثابت m_nodep;

عام:
    Trace_Var_راس(V3_گراف* graphp, AstVarScope* nodep)
        : V3_گراف_راس{graphp}
        , m_nodep{nodep} {}
    ~Trace_Var_راس() override = افتراضي;
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
    ثابت VNUser1_وچ_Use m_inuser1;
    ثابت VNUser2_وچ_Use m_inuser2;
    ثابت VNUser3_وچ_Use m_inuser3;
    // VNUser4InUse     In V3Hasher via V3DupFinder

    // STATE
    Ast_گرہ_Module* m_چوٹی_Modp = خالی_سوچک;  // Module to add variables to
    AstScope* ثابت m_چوٹی_Scopep = v3Global.rootp()->چوٹی_Scopep()->scopep();  // The top AstScope
    AstCFunc* m_cfuncp = خالی_سوچک;  // C function adding to graph
    AstCFunc* m_regFuncp = خالی_سوچک;  // Trace registration function
    AstCFunc* m_actAllFuncp = خالی_سوچک;  // Set all activity function
    AstTraceDecl* m_tracep = خالی_سوچک;  // Trace function adding to graph
    AstVarScope* m_activityVscp = خالی_سوچک;  // Activity variable
    uint32_t m_activity_نمبر = 0;  // Count of fields in activity variable
    uint32_t m_کوڈ = 0;  // Trace ident code# being assigned
    V3_گراف m_گراف;  // Var/CFunc tracking
    Trace_Activity_راس* ثابت m_alwaysVtxp;  // "Always trace" vertex
    منطقي m_finding = خطأ;  // Pass one of algorithm?
    بنية DtypeFuncs final {
    عام:
        AstCFunc* بھریا_Funcp = خالی_سوچک;
        AstCFunc* chgFuncp = خالی_سوچک;
    };
    std::unordered_map<ثابت Ast_گرہ_DType*, DtypeFuncs>
        m_dtypeNonConstFuncs;  // Full / Chg funcs per type
    std::unordered_map<ثابت Ast_گرہ_DType*, AstCFunc*> m_dtypeConstFuncs;  // Const func per type
    V3_Unique_ناں m_dtype_ناں{""};  // Unique type func names

    // Trace parallelism. Only VCD tracing can be parallelized at this time.
    ثابت uint32_t m_parallelism
        = v3Global.opt.useTraceParallel() ? static_cast<uint32_t>(v3Global.opt.دھاگے()) : 1;

    VDouble0 m_statSetters;  // Statistic tracking
    VDouble0 m_statSettersSlow;  // Statistic tracking
    VDouble0 m_statUniqSigs;  // Statistic tracking

    // All activity numbers applying to a given trace
    ورتوں Act_کوڈ_سیٹ = std::سیٹ<uint32_t>;
    // For activity set, what traces apply
    ورتوں TraceVec = std::multimap<Act_کوڈ_سیٹ, Trace_Trace_راس*>;
    // Candidate interface-member VarScopes keyed by (interface type, member name)
    std::map<std::pair<ثابت AstIface*, std::string>, std::سمتیہ<AstVarScope*>>
        m_ifaceMemberVscps;

    صنف TraceInitDeclCollector final : عام VNVisitor {
        std::سمتیہ<AstTraceDecl*>& m_declps;  // Output: trace declarations found so far
        std::سیٹ<ثابت AstCFunc*> m_تکیا_Funcps;

        فراغ ویکھو(AstTraceDecl* nodep) override { m_declps.ڈالو_پٹھ(nodep); }
        فراغ ویکھو(AstCCall* nodep) override {
            إذا (AstCFunc* ثابت funcp = nodep->funcp()) collect(funcp);
        }
        فراغ ویکھو(Ast_گرہ* nodep) override { iterateChildren(nodep); }

    عام:
        واضح TraceInitDeclCollector(std::سمتیہ<AstTraceDecl*>& declps)
            : m_declps{declps} {}
        فراغ collect(AstCFunc* funcp) {
            إذا (funcp && m_تکیا_Funcps.داخل_کرو(funcp).دوجا) iterate(funcp);
        }
    };

    // METHODS

    ساكن منطقي same_جڑ_Init_Alias(ثابت AstTraceDecl* جڑ_Declp, ثابت AstTraceDecl* چوٹی_Declp) {
        ثابت VNumRange& جڑ_ثنائی_ہندسہ_Range = جڑ_Declp->ثنائی_ہندسہ_Range();
        ثابت VNumRange& چوٹی_ثنائی_ہندسہ_Range = چوٹی_Declp->ثنائی_ہندسہ_Range();
        أرجع جڑ_Declp->showname() == چوٹی_Declp->showname()
               && جڑ_Declp->declDirection() == چوٹی_Declp->declDirection()
               && جڑ_Declp->چوڑائی_Min() == چوٹی_Declp->چوڑائی_Min()
               && جڑ_ثنائی_ہندسہ_Range.ranged() == چوٹی_ثنائی_ہندسہ_Range.ranged()
               && (!جڑ_ثنائی_ہندسہ_Range.ranged()
                   || (جڑ_ثنائی_ہندسہ_Range.کھبے() == چوٹی_ثنائی_ہندسہ_Range.کھبے()
                       && جڑ_ثنائی_ہندسہ_Range.سجے() == چوٹی_ثنائی_ہندسہ_Range.سجے()));
    }

    فراغ detectDuplicates() {
        UINFO(9, "Finding duplicates");
        // Note uses user4
        V3DupFinder dupFinder;  // Duplicate code detection

        // Compute member offsets within dtype instances incrementally.
        // For each dtype member TraceDecl, stores its code offset within the
        // dtype parent's code range.
        std::unordered_map<ثابت AstTraceDecl*, uint32_t> memberOffsets;
        std::unordered_map<ثابت AstTraceDecl*, uint32_t> running_وتھ;

        // For fixup pass: track dups redirected to dtype parents, so we can
        // fix them if the parent's user2 is later set during the loop.
        // Maps dup vertex -> original canonical member vertex.
        std::سمتیہ<std::pair<Trace_Trace_راس*, Trace_Trace_راس*>> dtype_ماپا_Redirects;

        // Hash all of the traced values and find if there are any duplicates
        لكل (V3_گراف_راس& vtx : m_گراف.vertices()) {
            إذا (Trace_Trace_راس* ثابت vvertexp = vtx.cast<Trace_Trace_راس>()) {
                AstTraceDecl* ثابت nodep = vvertexp->nodep();
                إذا (nodep->dtypeCallp()) استمر;

                إذا (nodep->dtypeDeclp()) {
                    uint32_t& وتھ = running_وتھ[nodep->dtypeDeclp()];
                    memberOffsets[nodep] = وتھ;
                    وتھ += nodep->کوڈ_Inc();
                }

                UASSERT_OBJ(!vvertexp->duplicatep(), nodep, "Should not be a duplicate");
                ثابت خودکار dupit = dupFinder.لبھو_Duplicate(nodep);
                إذا (dupit == dupFinder.سرا()) {
                    dupFinder.داخل_کرو(nodep);
                } وإلا {
                    AstTraceDecl* ثابت dupDeclp = VN_AS(dupit->دوجا, TraceDecl);
                    UASSERT_OBJ(dupDeclp, nodep, "Trace duplicate of wrong type");
                    Trace_Trace_راس* ثابت dupvertexp
                        = dupDeclp->user1u().منزل_گراف_راس()->cast<Trace_Trace_راس>();
                    UINFO(8, "  Orig " << dupDeclp << سطر_انت);
                    UINFO(8, "   dup " << nodep << سطر_انت);

                    إذا (dupDeclp->dtypeDeclp() && !dupDeclp->dtypeDeclp()->user2()) {
                        AstTraceDecl* ثابت dtypeParentp = dupDeclp->dtypeDeclp();
                        Trace_Trace_راس* ثابت dtypeVtxp
                            = dtypeParentp->user1u().منزل_گراف_راس()->cast<Trace_Trace_راس>();
                        ثابت خودکار it2 = memberOffsets.لبھو(dupDeclp);
                        UASSERT_OBJ(it2 != memberOffsets.سرا(), dupDeclp,
                                    "Member offset not precomputed");
                        vvertexp->duplicatep(dtypeVtxp);
                        vvertexp->dtype_Alias_وتھ(it2->دوجا);
                        dtype_ماپا_Redirects.emplace_پٹھ(vvertexp, dupvertexp);
                    } وإلا {
                        vvertexp->duplicatep(dupvertexp);
                    }
                    إذا (nodep->dtypeDeclp()) nodep->dtypeDeclp()->user2(سچ);
                }
            }
        }

        لكل (ثابت خودکار& pair : dtype_ماپا_Redirects) {
            Trace_Trace_راس* ثابت dupVtxp = pair.پہلا;
            Trace_Trace_راس* ثابت memberVtxp = pair.دوجا;
            ثابت AstTraceDecl* ثابت ماپا_Declp = dupVtxp->duplicatep()->nodep();
            إذا (ماپا_Declp->user2()) {
                dupVtxp->redirectDuplicatep(memberVtxp);
                dupVtxp->dtype_Alias_وتھ(0);
            }
        }

        إذا (dumpLevel() || debug() >= 9)
            dupFinder.dumpFile(v3Global.debugFilename("trace") + ".hash", خطأ);
    }

    فراغ گراف_Dtype_Prune() {
        لكل (V3_گراف_راس* ثابت vtxp : m_گراف.vertices().unlinkable()) {
            إذا (Trace_Trace_راس* ثابت vvertexp = vtxp->cast<Trace_Trace_راس>()) {
                AstTraceDecl* ثابت declp = vvertexp->nodep();
                // This skips the dtype sub-func optimization if a var is affected by multiple
                // activities.  We really only need to do this for _chg funcs (and not decls,
                // _const and _full) but it's simpiler to do it all one way or the other.
                إذا (declp) {
                    إذا (declp->user2() || (declp->dtypeDeclp() && !declp->dtypeDeclp()->user2())) {
                        AstCCall* ثابت callp = declp->dtypeCallp();
                        إذا (callp) {
                            Ast_گرہ* stmtexprp = callp->backp();
                            VL_DO_DANGLING(ڈالو_Deletep(stmtexprp->unlink_Fr_پٹھ()), stmtexprp);
                        } وإلا {
                            منطقي خالی_تھاواں_Scope;
                            افعل {
                                خالی_تھاواں_Scope = خطأ;
                                Ast_گرہ* ثابت declBackp = declp->backp();
                                Ast_گرہ* ثابت declNextp = declp->nextp();
                                إذا (VN_IS(declBackp, Trace_ڈالو_اگیتر)
                                    && VN_IS(declNextp, Trace_نکالو_اگیتر)) {
                                    VL_DO_DANGLING(ڈالو_Deletep(declBackp->unlink_Fr_پٹھ()),
                                                   declBackp);
                                    VL_DO_DANGLING(ڈالو_Deletep(declNextp->unlink_Fr_پٹھ()),
                                                   declNextp);
                                    خالی_تھاواں_Scope = سچ;
                                }
                            } طالما (خالی_تھاواں_Scope);
                        }
                        // Can't purge until we finish this pass
                        ڈالو_Deletep(declp->unlink_Fr_پٹھ());  // declp used below
                        vvertexp->reroute_کنارے(&m_گراف);
                        vvertexp->unlinkDelete(&m_گراف);
                    }
                    declp->dtypeDeclp(خالی_سوچک);
                }
            }
        }
    }

    فراغ ونڈو_Trace_Decl_Funcs() {
        ثابت صحيح ونڈو_حد_بندی = v3Global.opt.output_ونڈو_CTrace() ? v3Global.opt.output_ونڈو_CTrace()
                                                                : std::numeric_limits<صحيح>::max();
        إذا (ونڈو_حد_بندی == std::numeric_limits<صحيح>::max()) أرجع;

        لكل (Ast_گرہ* nodep = m_چوٹی_Scopep->blocksp(); nodep; nodep = nodep->nextp()) {
            AstCFunc* ثابت funcp = VN_CAST(nodep, CFunc);
            إذا (!funcp || !funcp->user2()) استمر;

            ثابت string ادھار_Name = funcp->name();
            uint32_t sub_عدد = 1;
            صحيح cur_ناپ = 0;
            AstCFunc* curFuncp = funcp;
            std::سمتیہ<Ast_گرہ_Stmt*> callStmts;

            خودکار شروع_New_Sub_Func = [&]() {
                File_سطر* ثابت flp = funcp->fileline();
                ثابت string newName = ادھار_Name + "_" + cvt_منزل_Str(sub_عدد++);
                AstCFunc* ثابت newFuncp = جديد AstCFunc{flp, newName, m_چوٹی_Scopep};
                newFuncp->argTypes(v3Global.opt.trace_Class_ادھار() + "* tracep");
                newFuncp->isTrace(سچ);
                newFuncp->isStatic(خطأ);
                newFuncp->isLoose(سچ);
                newFuncp->slow(سچ);
                newFuncp->dontCombine(سچ);
                m_چوٹی_Scopep->جوڑو_Blocksp(newFuncp);
                newFuncp->جوڑو_Stmtsp(جديد AstCStmt{flp, "const int c = vlSymsp->__Vm_baseCode;"});
                AstCCall* ثابت callp = جديد AstCCall{flp, newFuncp};
                callp->dtype_سیٹ_Void();
                callp->argTypes("tracep");
                callStmts.ڈالو_پٹھ(callp->makeStmt());
                curFuncp = newFuncp;
                cur_ناپ = 0;
            };

            لكل (Ast_گرہ *stmtp = funcp->stmtsp(), *nextp; stmtp; stmtp = nextp) {
                nextp = stmtp->nextp();
                cur_ناپ += stmtp->گرہ_گنتی();

                إذا (curFuncp != funcp) {
                    stmtp->unlink_Fr_پٹھ();
                    curFuncp->جوڑو_Stmtsp(stmtp);
                }

                إذا (cur_ناپ > ونڈو_حد_بندی && nextp) شروع_New_Sub_Func();
            }

            لكل (Ast_گرہ_Stmt* ثابت callStmtp : callStmts) { funcp->جوڑو_Stmtsp(callStmtp); }
        }
    }

    فراغ گراف_Simplify(منطقي initial) {
        إذا (initial) {
            // Remove all variable nodes
            لكل (V3_گراف_راس* ثابت vtxp : m_گراف.vertices().unlinkable()) {
                إذا (Trace_Var_راس* ثابت vvertexp = vtxp->cast<Trace_Var_راس>()) {
                    vvertexp->reroute_کنارے(&m_گراف);
                    vvertexp->unlinkDelete(&m_گراف);
                }
            }
            // Remove multiple variables connecting funcs to traces
            // We do this twice, as then we have fewer edges to multiply out in the below
            // expansion.
            m_گراف.remove_Redundant_کنارے_Max(&V3_گراف_کنارا::followAlwaysTrue);
            // Remove all Cfunc nodes
            لكل (V3_گراف_راس* ثابت vtxp : m_گراف.vertices().unlinkable()) {
                إذا (Trace_CFunc_راس* ثابت vvertexp = vtxp->cast<Trace_CFunc_راس>()) {
                    vvertexp->reroute_کنارے(&m_گراف);
                    vvertexp->unlinkDelete(&m_گراف);
                }
            }
        }

        // Remove multiple variables connecting funcs to traces
        m_گراف.remove_Redundant_کنارے_Max(&V3_گراف_کنارا::followAlwaysTrue);

        // If there are any edges from a always, keep only the always
        لكل (V3_گراف_راس& vtx : m_گراف.vertices()) {
            إذا (Trace_Trace_راس* ثابت vvertexp = vtx.cast<Trace_Trace_راس>()) {
                // Search for the incoming always edge
                ثابت V3_گراف_کنارا* alwaysEdgep = خالی_سوچک;
                لكل (ثابت V3_گراف_کنارا& کنارا : vvertexp->وچ_کنارے()) {
                    ثابت Trace_Activity_راس* ثابت actVtxp
                        = کنارا.fromp()->as<ثابت Trace_Activity_راس>();
                    إذا (actVtxp->activityAlways()) {
                        alwaysEdgep = &کنارا;
                        اكسر;
                    }
                }
                // If always edge exists, remove all other edges
                إذا (alwaysEdgep) {
                    لكل (V3_گراف_کنارا* ثابت edgep : vvertexp->وچ_کنارے().unlinkable()) {
                        إذا (edgep != alwaysEdgep) VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
                    }
                }
            }
        }

        // Activity points with no outputs can be removed
        لكل (V3_گراف_راس* ثابت vtxp : m_گراف.vertices().unlinkable()) {
            إذا (Trace_Activity_راس* ثابت aVtxp = vtxp->cast<Trace_Activity_راس>()) {
                // Leave in the always vertex for later use.
                إذا (aVtxp != m_alwaysVtxp && aVtxp->باہرلا_خالی_تھاواں()) {
                    VL_DO_DANGLING(aVtxp->unlinkDelete(&m_گراف), aVtxp);
                }
            }
        }
    }

    uint32_t assignactivityNumbers() {
        uint32_t activity_نمبر = 1;  // Note 0 indicates "slow" only
        لكل (V3_گراف_راس& vtx : m_گراف.vertices()) {
            إذا (Trace_Activity_راس* ثابت vvertexp = vtx.cast<Trace_Activity_راس>()) {
                إذا (vvertexp != m_alwaysVtxp) {
                    إذا (vvertexp->slow()) {
                        vvertexp->activity_کوڈ(Trace_Activity_راس::ACTIVITY_SLOW);
                    } وإلا {
                        vvertexp->activity_کوڈ(activity_نمبر++);
                    }
                }
            }
        }
        أرجع activity_نمبر;
    }

    فراغ ترتیب_دیو_Traces(TraceVec& traces, uint32_t& nNonConstCodes) {
        // Populate sort structure
        traces.clear();
        nNonConstCodes = 0;
        لكل (V3_گراف_راس& vtx : m_گراف.vertices()) {
            إذا (Trace_Trace_راس* ثابت vtxp = vtx.cast<Trace_Trace_راس>()) {
                Act_کوڈ_سیٹ act_سیٹ;
                UINFO(9, "  Add to sort: " << vtxp);
                UINFOTREE(9, vtxp->nodep(), "", "trnode");
                لكل (ثابت V3_گراف_کنارا& کنارا : vtxp->وچ_کنارے()) {
                    ثابت Trace_Activity_راس* ثابت cfvertexp
                        = کنارا.fromp()->cast<ثابت Trace_Activity_راس>();
                    UASSERT_OBJ(cfvertexp, vtxp->nodep(),
                                "Should have been function pointing to this trace");
                    UINFO(9, "   Activity: " << cfvertexp);
                    إذا (cfvertexp->activityAlways()) {
                        // If code 0, we always trace; ignore other codes
                        act_سیٹ.داخل_کرو(Trace_Activity_راس::ACTIVITY_ALWAYS);
                    } وإلا {
                        act_سیٹ.داخل_کرو(cfvertexp->activity_کوڈ());
                    }
                }
                UASSERT_OBJ(act_سیٹ.گنتی(Trace_Activity_راس::ACTIVITY_ALWAYS) == 0
                                || act_سیٹ.ناپ() == 1,
                            vtxp->nodep(), "Always active trace has further triggers");
                // Count nodes
                إذا (!vtxp->duplicatep() && !act_سیٹ.خالی_تھاواں())
                    nNonConstCodes += vtxp->nodep()->کوڈ_Inc();
                إذا (act_سیٹ.خالی_تھاواں()) {
                    // If a trace doesn't have activity, it's constant, and we
                    // don't need to track changes on it.
                    act_سیٹ.داخل_کرو(Trace_Activity_راس::ACTIVITY_NEVER);
                } وإلا إذا (act_سیٹ.گنتی(Trace_Activity_راس::ACTIVITY_SLOW) && act_سیٹ.ناپ() > 1) {
                    // If a trace depends on the slow flag as well as other
                    // flags, remove the dependency on the slow flag. We will
                    // make slow routines set all activity flags.
                    act_سیٹ.erase(Trace_Activity_راس::ACTIVITY_SLOW);
                }
                traces.emplace(act_سیٹ, vtxp);
            }
        }
    }

    فراغ گراف_Optimize() {
        // Assign initial activity numbers to activity vertices
        assignactivityNumbers();

        // Sort the traces by activity sets
        TraceVec traces;
        uint32_t unused1;
        ترتیب_دیو_Traces(traces, unused1);

        // For each activity set with only a small number of signals, make those
        // signals always traced, as it's cheaper to check a few value changes
        // than to test a lot of activity flags
        خودکار it = traces.begin();
        ثابت خودکار سرا = traces.سرا();
        طالما (it != سرا) {
            خودکار سر = it;
            // Approximate the complexity of the value change check
            uint32_t complexity = 0;
            ثابت Act_کوڈ_سیٹ& act_سیٹ = it->پہلا;
            لكل (; it != سرا && it->پہلا == act_سیٹ; ++it) {
                إذا (!it->دوجا->duplicatep()) {
                    uint32_t لاگت = 0;
                    ثابت AstTraceDecl* ثابت declp = it->دوجا->nodep();
                    // The number of comparisons required by bufp->chg*
                    لاگت += declp->isWide() ? declp->کوڈ_Inc() : 1;
                    // Arrays are traced by element
                    لاگت *= declp->arrayRange().ranged() ? declp->arrayRange().elements() : 1;
                    // Note: Experiments factoring in the size of declp->valuep()
                    // showed no benefit in tracing speed, even for large trees,
                    // so we will leave those out for now.
                    complexity += لاگت;
                }
            }
            // Leave alone always changing, never changing and signals only set in slow code
            إذا (act_سیٹ.گنتی(Trace_Activity_راس::ACTIVITY_ALWAYS)) استمر;
            إذا (act_سیٹ.گنتی(Trace_Activity_راس::ACTIVITY_NEVER)) استمر;
            إذا (act_سیٹ.گنتی(Trace_Activity_راس::ACTIVITY_SLOW)) استمر;
            // If the value comparisons are cheaper to perform than checking the
            // activity flags make the signals always traced. Note this cost
            // equation is heuristic.
            إذا (complexity <= act_سیٹ.ناپ() * 2) {
                لكل (; سر != it; ++سر) {
                    جديد V3_گراف_کنارا{&m_گراف, m_alwaysVtxp, سر->دوجا, 1};
                }
            }
        }

        گراف_Simplify(خطأ);
    }

    Ast_گرہ_اظہاریہ* selectActivity(File_سطر* flp, uint32_t acode, ثابت VAccess& access) {
        أرجع جديد AstArraySel(flp, جديد AstVarRef{flp, m_activityVscp, access}, acode);
    }

    Ast_گرہ* newActivitySetter(Ast_گرہ* insertp, uint32_t کوڈ) {
        ++m_statSetters;
        File_سطر* ثابت fl = insertp->fileline();
        Ast_سونپو* ثابت setterp = جديد Ast_سونپو{fl, selectActivity(fl, کوڈ, VAccess::WRITE),
                                                 جديد AstConst{fl, AstConst::ثنائی_ہندسہ_True{}}};
        أرجع setterp;
    }

    Ast_گرہ* newActivityAll(Ast_گرہ* insertp) {
        ++m_statSettersSlow;
        إذا (!m_actAllFuncp) {
            File_سطر* ثابت flp = m_چوٹی_Scopep->fileline();
            AstCFunc* ثابت funcp = جديد AstCFunc{flp, "__VtraceActivitySetAll", m_چوٹی_Scopep};
            funcp->slow(سچ);
            funcp->isStatic(خطأ);
            funcp->isLoose(سچ);
            m_چوٹی_Scopep->جوڑو_Blocksp(funcp);
            لكل (uint32_t کوڈ = 0; کوڈ < m_activity_نمبر; ++کوڈ) {
                Ast_گرہ* ثابت setterp = newActivitySetter(insertp, کوڈ);
                funcp->جوڑو_Stmtsp(setterp);
            }
            m_actAllFuncp = funcp;
        }
        AstCCall* ثابت callp = جديد AstCCall{insertp->fileline(), m_actAllFuncp};
        callp->dtype_سیٹ_Void();
        أرجع callp->makeStmt();
    }

    فراغ createActivityFlags() {
        // Assign final activity numbers
        m_activity_نمبر = assignactivityNumbers();

        // Create an array of bytes, not a bit vector, as they can be set
        // atomically by mtasks, and are cheaper to set (no need for
        // read-modify-write on the C type), and the speed of the tracing code
        // is the same on largish designs.
        File_سطر* ثابت flp = m_چوٹی_Scopep->fileline();
        Ast_گرہ_DType* ثابت newScalarDtp = جديد AstBasicDType{flp, VFlag_ثنائی_ہندسہ_Packed{}, 1};
        v3Global.rootp()->typeTablep()->جوڑو_Typesp(newScalarDtp);
        AstRange* ثابت newArange
            = جديد AstRange{flp, VNumRange{static_cast<صحيح>(m_activity_نمبر) - 1, 0}};
        Ast_گرہ_DType* ثابت newArrDtp = جديد AstUnpackArrayDType{flp, newScalarDtp, newArange};
        v3Global.rootp()->typeTablep()->جوڑو_Typesp(newArrDtp);
        m_activityVscp = v3Global.rootp()->چوٹی_Scopep()->create_عارضی("__VtraceActivity", newArrDtp);

        // Insert activity setters
        لكل (ثابت V3_گراف_راس& vtx : m_گراف.vertices()) {
            إذا (ثابت Trace_Activity_راس* ثابت vtxp = vtx.cast<ثابت Trace_Activity_راس>()) {
                Ast_گرہ* setterp = خالی_سوچک;
                إذا (vtxp->activitySlow()) {
                    setterp = newActivityAll(vtxp->insertp());
                    // Just set all flags in slow code as it should be rare.
                    // This will be rolled up into a loop by V3Reloop.
                } وإلا إذا (!vtxp->activityAlways()) {
                    setterp = newActivitySetter(vtxp->insertp(), vtxp->activity_کوڈ());
                }
                إذا (setterp) {
                    Ast_گرہ* ثابت insertp = vtxp->insertp();
                    إذا (Ast_Stmt_اظہاریہ* ثابت stmtp = VN_CAST(insertp, Stmt_اظہاریہ)) {
                        stmtp->جوڑو_اگلا_ایتھے(setterp);
                    } وإلا إذا (AstCFunc* ثابت funcp = VN_CAST(insertp, CFunc)) {
                        // If there are awaits, insert the setter after each await
                        إذا (funcp->isCoroutine() && funcp->stmtsp()) {
                            funcp->stmtsp()->foreach_And_اگلا([setterp](AstCAwait* awaitp) {
                                awaitp->جوڑو_اگلا_ایتھے(setterp->clone_درخت(خطأ));
                            });
                        }
                        funcp->جوڑو_Stmtsp(setterp);
                    } وإلا {
                        insertp->v3fatalSrc("Bad trace activity vertex");
                    }
                }
            }
        }
    }

    AstCFunc* newCFunc(VTraceType traceType, AstCFunc* چوٹی_Funcp, uint32_t func_عدد,
                       uint32_t ادھار_کوڈ = 0, ثابت AstTraceDecl* ثابت declp = خالی_سوچک,
                       منطقي declSub = خطأ) {
        // Create new function
        ثابت منطقي is_چوٹی_Func = !declp && چوٹی_Funcp == خالی_سوچک;
        std::string funcName;
        إذا (is_چوٹی_Func || declp) {
            funcName = traceType.func_اگیتر();
        } وإلا {
            funcName = چوٹی_Funcp->name();
            funcName += "_sub";
        }
        إذا (declp) {
            funcName += "_dtype";
            إذا (declSub) funcName += "_sub";
            funcName += "__";
            funcName = m_dtype_ناں.get(funcName);
        } وإلا {
            funcName += "_";
            funcName += cvt_منزل_Str(func_عدد);
        }

        File_سطر* ثابت flp = m_چوٹی_Scopep->fileline();
        AstCFunc* ثابت funcp = جديد AstCFunc{flp, funcName, m_چوٹی_Scopep};
        funcp->isTrace(سچ);
        funcp->dontCombine(سچ);
        funcp->isLoose(سچ);
        funcp->slow(traceType != VTraceType::CHANGE);
        funcp->isStatic(is_چوٹی_Func);
        // Add it to top scope
        m_چوٹی_Scopep->جوڑو_Blocksp(funcp);
        ثابت std::string bufArg
            = v3Global.opt.trace_Class_ادھار() + "::Buffer* bufp"
              + (declp ? (", uint32_t offset, const " + declp->dtypep()->cType("", سچ, سچ)
                          + " __VdtypeVar")
                       : "");
        إذا (is_چوٹی_Func) {
            // Top functions
            funcp->argTypes("void* voidSelf, " + bufArg);
            funcp->جوڑو_Stmtsp(جديد AstCStmt{flp, EmitCUtil::void_Self_سونپو(m_چوٹی_Modp)});
            funcp->جوڑو_Stmtsp(جديد AstCStmt{flp, EmitCUtil::sym_Class_سونپو()});
            // Add global activity check to change dump functions
            إذا (traceType == VTraceType::CHANGE) {  //
                funcp->جوڑو_Stmtsp(
                    جديد AstCStmt{flp, "if (VL_UNLIKELY(!vlSymsp->__Vm_activity)) return;"});
            }
            // Register function
            AstCStmt* ثابت cstmtp = جديد AstCStmt{flp};
            m_regFuncp->جوڑو_Stmtsp(cstmtp);
            إذا (traceType == VTraceType::CONSTANT) {
                cstmtp->جوڑو("tracep->addConstCb(");
                cstmtp->جوڑو(جديد Ast_Addr_دا_CFunc{flp, funcp});
                cstmtp->جوڑو(", " + std::منزل_string(func_عدد) + ", vlSelf);");
            } وإلا إذا (traceType == VTraceType::بھریا) {
                cstmtp->جوڑو("tracep->addFullCb(");
                cstmtp->جوڑو(جديد Ast_Addr_دا_CFunc{flp, funcp});
                cstmtp->جوڑو(", " + std::منزل_string(func_عدد) + ", vlSelf);");
            } وإلا {
                cstmtp->جوڑو("tracep->addChgCb(");
                cstmtp->جوڑو(جديد Ast_Addr_دا_CFunc{flp, funcp});
                cstmtp->جوڑو(", " + std::منزل_string(func_عدد) + ", vlSelf);");
            }
        } وإلا {
            // Sub functions
            funcp->argTypes(bufArg);
            // Setup base references. Note in rare occasions we can end up with an empty trace
            // sub function, hence the VL_ATTR_UNUSED attributes.
            إذا (traceType != VTraceType::CHANGE) {
                // Full dump sub function
                funcp->جوڑو_Stmtsp(جديد AstCStmt{flp,  //
                                              string("uint32_t* const oldp VL_ATTR_UNUSED = "
                                                     "bufp->oldp(vlSymsp->__Vm_baseCode")
                                                  + (declp ? " + offset" : "") + ");\n"});
            } وإلا {
                // Change dump sub function
                funcp->جوڑو_Stmtsp(جديد AstCStmt{flp,  //
                                              "uint32_t* const oldp VL_ATTR_UNUSED = "
                                              "bufp->oldp(vlSymsp->__Vm_baseCode + "
                                                  + (declp ? " offset" : cvt_منزل_Str(ادھار_کوڈ))
                                                  + ");\n"});
            }
            إذا (!declp) {
                // Add call to top function
                AstCCall* ثابت callp = جديد AstCCall{funcp->fileline(), funcp};
                callp->dtype_سیٹ_Void();
                callp->argTypes("bufp");
                چوٹی_Funcp->جوڑو_Stmtsp(callp->makeStmt());
            }
        }
        //پورا
        UINFO(5, "  newCFunc " << funcp);
        أرجع funcp;
    }

    AstCFunc* newCDtypeSubFunc(VTraceType traceType, ثابت AstTraceDecl* ثابت declp,
                               AstCFunc* parentp) {
        AstCFunc* ثابت funcp = newCFunc(traceType, خالی_سوچک, 0, 0, declp, سچ);
        File_سطر* ثابت flp = m_چوٹی_Scopep->fileline();
        AstCCall* ثابت callp = جديد AstCCall{flp, funcp};
        callp->dtype_سیٹ_Void();
        callp->argTypes("bufp, offset, __VdtypeVar");
        parentp->جوڑو_Stmtsp(callp->makeStmt());
        أرجع funcp;
    }

    AstCFunc* createConstDtypeTraceFunctions(ثابت AstTraceDecl* declp) {
        ثابت Ast_گرہ_DType* ثابت dtypep = declp->dtypep()->skipRefp();
        خودکار pair = m_dtypeConstFuncs.emplace(dtypep, خالی_سوچک);
        إذا (pair.دوجا) {
            File_سطر* ثابت flp = declp->fileline();
            AstCFunc* ثابت funcp = newCFunc(VTraceType::CONSTANT, خالی_سوچک, 0, 0, declp);

            منطقي پہلا = سچ;
            لكل (Ast_گرہ* callStmtp = declp->dtypeCallp()->funcp()->stmtsp(); callStmtp;
                 callStmtp = callStmtp->nextp()) {
                ثابت AstCCall* ثابت callp
                    = VN_CAST(VN_CAST(callStmtp, Stmt_اظہاریہ)->exprp(), CCall);
                منطقي onlyOne = پہلا && !callStmtp->nextp();
                AstCFunc* ثابت subFuncp
                    = onlyOne ? funcp : newCDtypeSubFunc(VTraceType::CONSTANT, declp, funcp);
                لكل (Ast_گرہ* stmtp = callp->funcp()->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    إذا (AstTraceDecl* ثابت میدان_Declp = VN_CAST(stmtp, TraceDecl)) {
                        AstTraceInc* ثابت incp
                            = جديد AstTraceInc{flp, میدان_Declp, VTraceType::CONSTANT};
                        subFuncp->جوڑو_Stmtsp(incp);
                    }
                }
                پہلا = خطأ;
            }

            pair.پہلا->دوجا = funcp;
        }

        أرجع pair.پہلا->دوجا;
    }

    فراغ createConstTraceFunctions(ثابت TraceVec& traces) {
        ثابت صحيح ونڈو_حد_بندی = v3Global.opt.output_ونڈو_CTrace() ? v3Global.opt.output_ونڈو_CTrace()
                                                                : std::numeric_limits<صحيح>::max();

        AstCFunc* ثابت چوٹی_Funcp = newCFunc(VTraceType::CONSTANT, خالی_سوچک, 0);
        uint32_t sub_Func_عدد = 0;
        AstCFunc* subFuncp = خالی_سوچک;
        صحيح subStmts = 0;
        std::سمتیہ<ثابت Trace_Trace_راس*> duplicates;
        لكل (خودکار it = traces.cbegin(); it != traces.سرا(); ++it) {
            ثابت Trace_Trace_راس* ثابت vtxp = it->دوجا;
            AstTraceDecl* ثابت declp = vtxp->nodep();
            إذا (vtxp->duplicatep()) {
                duplicates.ڈالو_پٹھ(vtxp);
                استمر;
            }

            // This is a canonical trace node. Assign trace code (signal number).
            UASSERT_OBJ(!declp->کوڈ_Assigned(), declp,
                        "Canonical node should not have code assigned yet");
            declp->کوڈ(m_کوڈ);
            ثابت uint32_t کوڈ_Inc = declp->کوڈ_Inc();
            m_کوڈ += کوڈ_Inc;
            ++m_statUniqSigs;

            // If this is a const signal, add the AstTraceInc
            ثابت Act_کوڈ_سیٹ& act_سیٹ = it->پہلا;
            إذا (act_سیٹ.گنتی(Trace_Activity_راس::ACTIVITY_NEVER)) {
                // Crate new sub function if required
                إذا (!subFuncp || subStmts > ونڈو_حد_بندی) {
                    subStmts = 0;
                    subFuncp = newCFunc(VTraceType::CONSTANT, چوٹی_Funcp, sub_Func_عدد);
                    ++sub_Func_عدد;
                }
                File_سطر* ثابت flp = declp->fileline();
                إذا (declp->dtypeCallp()) {
                    AstCFunc* ثابت funcp = createConstDtypeTraceFunctions(declp);
                    Ast_گرہ_اظہاریہ* argsp = خالی_سوچک;
                    argsp = Ast_گرہ::جوڑو_اگلا(argsp, declp->valuep()->clone_درخت(خطأ));
                    AstCCall* ثابت callp = جديد AstCCall{flp, funcp, argsp};
                    callp->dtype_سیٹ_Void();
                    callp->argTypes(callp->argTypes() + "bufp, " + std::منزل_string(declp->کوڈ()));
                    subFuncp->جوڑو_Stmtsp(callp->makeStmt());

                    subStmts += 1;
                } وإلا {
                    AstTraceInc* ثابت incp = جديد AstTraceInc{flp, declp, VTraceType::CONSTANT};
                    subFuncp->جوڑو_Stmtsp(incp);
                    subStmts += incp->گرہ_گنتی();
                }
            }
        }

        // Assign codes to duplicate nodes from their canonicals
        لكل (ثابت Trace_Trace_راس* ثابت vtxp : duplicates) {
            AstTraceDecl* ثابت declp = vtxp->nodep();
            ثابت Trace_Trace_راس* ثابت canonVtxp = vtxp->duplicatep();
            ثابت AstTraceDecl* ثابت canonDeclp = canonVtxp->nodep();
            UASSERT_OBJ(!canonVtxp->duplicatep(), canonDeclp, "Canonical node is a duplicate");
            UASSERT_OBJ(canonDeclp->کوڈ_Assigned(), canonDeclp,
                        "Canonical node should have code assigned already");
            declp->کوڈ(canonDeclp->کوڈ() + vtxp->dtype_Alias_وتھ());
        }
    }

    DtypeFuncs createNonConstDtypeTraceFunctions(ثابت AstTraceDecl* declp) {
        Ast_گرہ_DType* dtypep = declp->dtypep()->skipRefp();
        خودکار pair = m_dtypeNonConstFuncs.emplace(dtypep, DtypeFuncs{});
        إذا (pair.دوجا) {
            File_سطر* ثابت flp = declp->fileline();
            AstCFunc* ثابت بھریا_Funcp = newCFunc(VTraceType::بھریا, خالی_سوچک, 0, 0, declp);
            AstCFunc* ثابت chgFuncp = newCFunc(VTraceType::CHANGE, خالی_سوچک, 0, 0, declp);

            منطقي پہلا = سچ;
            لكل (Ast_گرہ* callStmtp = declp->dtypeCallp()->funcp()->stmtsp(); callStmtp;
                 callStmtp = callStmtp->nextp()) {
                ثابت AstCCall* ثابت callp
                    = VN_CAST(VN_CAST(callStmtp, Stmt_اظہاریہ)->exprp(), CCall);
                منطقي onlyOne = پہلا && !callStmtp->nextp();
                AstCFunc* ثابت بھریا_Sub_Funcp
                    = onlyOne ? بھریا_Funcp : newCDtypeSubFunc(VTraceType::بھریا, declp, بھریا_Funcp);
                AstCFunc* ثابت chgSubFuncp
                    = onlyOne ? chgFuncp : newCDtypeSubFunc(VTraceType::CHANGE, declp, chgFuncp);
                لكل (Ast_گرہ* stmtp = callp->funcp()->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    إذا (AstTraceDecl* ثابت میدان_Declp = VN_CAST(stmtp, TraceDecl)) {
                        AstTraceInc* ثابت incFullp
                            = جديد AstTraceInc{flp, میدان_Declp, VTraceType::بھریا};
                        بھریا_Sub_Funcp->جوڑو_Stmtsp(incFullp);
                        AstTraceInc* ثابت incChgp
                            = جديد AstTraceInc{flp, میدان_Declp, VTraceType::CHANGE};
                        chgSubFuncp->جوڑو_Stmtsp(incChgp);
                    }
                }
                پہلا = خطأ;
            }

            pair.پہلا->دوجا = {.بھریا_Funcp = بھریا_Funcp, .chgFuncp = chgFuncp};
        }

        أرجع pair.پہلا->دوجا;
    }

    فراغ createNonConstTraceFunctions(ثابت TraceVec& traces, uint32_t nAllCodes,
                                      uint32_t parallelism) {
        ثابت صحيح ونڈو_حد_بندی = v3Global.opt.output_ونڈو_CTrace() ? v3Global.opt.output_ونڈو_CTrace()
                                                                : std::numeric_limits<صحيح>::max();

        // pre-incremented, so starts at 0
        uint32_t چوٹی_Func_عدد = std::numeric_limits<uint32_t>::max();
        TraceVec::const_iterator it = traces.begin();
        طالما (it != traces.سرا()) {
            AstCFunc* چوٹی_Ful_Funcp = خالی_سوچک;
            AstCFunc* چوٹی_Chg_Funcp = خالی_سوچک;
            AstCFunc* subFulFuncp = خالی_سوچک;
            AstCFunc* subChgFuncp = خالی_سوچک;
            uint32_t sub_Func_عدد = 0;
            صحيح subStmts = 0;
            ثابت uint32_t maxCodes = std::max((nAllCodes + parallelism - 1) / parallelism, 1U);
            uint32_t nCodes = 0;
            ثابت Act_کوڈ_سیٹ* prev_Act_سیٹ = خالی_سوچک;
            AstIf* ifp = خالی_سوچک;
            uint32_t ادھار_کوڈ = 0;
            لكل (; nCodes < maxCodes && it != traces.سرا(); ++it) {
                ثابت Act_کوڈ_سیٹ& act_سیٹ = it->پہلا;
                // Traced value never changes, no need to add it
                إذا (act_سیٹ.گنتی(Trace_Activity_راس::ACTIVITY_NEVER)) استمر;

                ثابت Trace_Trace_راس* ثابت vtxp = it->دوجا;
                AstTraceDecl* ثابت declp = vtxp->nodep();

                // This is a duplicate decl, no need to add it, but must set the
                // function index to the same as the canonical node.
                إذا (ثابت Trace_Trace_راس* ثابت canonVtxp = vtxp->duplicatep()) {
                    declp->fidx(canonVtxp->nodep()->fidx());
                    استمر;
                }

                // Create top function if not yet created
                إذا (!چوٹی_Ful_Funcp) {
                    ++چوٹی_Func_عدد;
                    چوٹی_Ful_Funcp = newCFunc(VTraceType::بھریا, خالی_سوچک, چوٹی_Func_عدد);
                    چوٹی_Chg_Funcp = newCFunc(VTraceType::CHANGE, خالی_سوچک, چوٹی_Func_عدد);
                }

                // Create new sub function if required
                إذا (!subFulFuncp || subStmts > ونڈو_حد_بندی) {
                    ادھار_کوڈ = declp->کوڈ();
                    subStmts = 0;
                    subFulFuncp = newCFunc(VTraceType::بھریا, چوٹی_Ful_Funcp, sub_Func_عدد, ادھار_کوڈ);
                    subChgFuncp = newCFunc(VTraceType::CHANGE, چوٹی_Chg_Funcp, sub_Func_عدد, ادھار_کوڈ);
                    ++sub_Func_عدد;
                    prev_Act_سیٹ = خالی_سوچک;
                    ifp = خالی_سوچک;
                }

                // If required, create the conditional node checking the activity flags
                إذا (!prev_Act_سیٹ || act_سیٹ != *prev_Act_سیٹ) {
                    File_سطر* ثابت flp = m_چوٹی_Scopep->fileline();
                    ثابت منطقي always = act_سیٹ.گنتی(Trace_Activity_راس::ACTIVITY_ALWAYS) != 0;
                    Ast_گرہ_اظہاریہ* condp = خالی_سوچک;
                    إذا (always) {
                        condp = جديد AstConst{flp, 1};  // Always true, will be folded later
                    } وإلا {
                        لكل (ثابت uint32_t act_کوڈ : act_سیٹ) {
                            Ast_گرہ_اظہاریہ* ثابت selp = selectActivity(flp, act_کوڈ, VAccess::READ);
                            condp = condp ? جديد AstOr{flp, condp, selp} : selp;
                        }
                    }
                    ifp = جديد AstIf{flp, condp};
                    إذا (!always) ifp->branchPred(VBranchPred::BP_UNLIKELY);
                    subChgFuncp->جوڑو_Stmtsp(ifp);
                    subStmts += ifp->گرہ_گنتی();
                    prev_Act_سیٹ = &act_سیٹ;
                }

                // Add TraceInc nodes
                File_سطر* ثابت flp = declp->fileline();
                إذا (declp->dtypeCallp()) {
                    DtypeFuncs funcs = createNonConstDtypeTraceFunctions(declp);
                    Ast_گرہ_اظہاریہ* argsp = خالی_سوچک;
                    argsp = Ast_گرہ::جوڑو_اگلا(argsp, declp->valuep()->clone_درخت(خطأ));
                    AstCCall* ثابت callFullp = جديد AstCCall{flp, funcs.بھریا_Funcp, argsp};
                    callFullp->dtype_سیٹ_Void();
                    callFullp->argTypes(callFullp->argTypes() + "bufp, "
                                        + std::منزل_string(declp->کوڈ()));
                    subFulFuncp->جوڑو_Stmtsp(callFullp->makeStmt());
                    argsp = خالی_سوچک;
                    argsp = Ast_گرہ::جوڑو_اگلا(argsp, declp->valuep()->clone_درخت(خطأ));
                    AstCCall* ثابت callChgp = جديد AstCCall{flp, funcs.chgFuncp, argsp};
                    callChgp->dtype_سیٹ_Void();
                    callChgp->argTypes(callChgp->argTypes() + "bufp, "
                                       + std::منزل_string(declp->کوڈ()));
                    ifp->جوڑو_Thensp(callChgp->makeStmt());

                    subStmts += 2;
                } وإلا {
                    AstTraceInc* ثابت incFulp = جديد AstTraceInc{flp, declp, VTraceType::بھریا};
                    subFulFuncp->جوڑو_Stmtsp(incFulp);
                    AstTraceInc* ثابت incChgp
                        = جديد AstTraceInc{flp, declp, VTraceType::CHANGE, ادھار_کوڈ};
                    ifp->جوڑو_Thensp(incChgp);

                    // Set the function index of the decl
                    declp->fidx(چوٹی_Func_عدد);

                    // Track splitting due to size
                    UASSERT_OBJ(incFulp->گرہ_گنتی() == incChgp->گرہ_گنتی(), declp,
                                "Should have equal cost");
                    ثابت VNumRange range = declp->arrayRange();
                    إذا (range.ranged()) {
                        // 2x because each element is a TraceInc and a VarRef
                        subStmts += range.elements() * 2;
                    } وإلا {
                        subStmts += incChgp->گرہ_گنتی();
                    }
                }

                // Track partitioning
                nCodes += declp->کوڈ_Inc();
            }
        }
    }

    فراغ createCleanupFunction() {
        File_سطر* ثابت fl = m_چوٹی_Scopep->fileline();
        AstCFunc* ثابت cleanupFuncp = جديد AstCFunc{fl, "trace_cleanup", m_چوٹی_Scopep};
        cleanupFuncp->argTypes("void* voidSelf, " + v3Global.opt.trace_Class_ادھار()
                               + "* /*unused*/");
        cleanupFuncp->isTrace(سچ);
        cleanupFuncp->slow(خطأ);
        cleanupFuncp->isStatic(سچ);
        cleanupFuncp->isLoose(سچ);
        m_چوٹی_Scopep->جوڑو_Blocksp(cleanupFuncp);
        cleanupFuncp->جوڑو_Stmtsp(جديد AstCStmt{fl, EmitCUtil::void_Self_سونپو(m_چوٹی_Modp)});
        cleanupFuncp->جوڑو_Stmtsp(جديد AstCStmt{fl, EmitCUtil::sym_Class_سونپو()});

        // Register it
        {
            AstCStmt* ثابت cstmtp = جديد AstCStmt{fl};
            m_regFuncp->جوڑو_Stmtsp(cstmtp);
            cstmtp->جوڑو("tracep->addCleanupCb(");
            cstmtp->جوڑو(جديد Ast_Addr_دا_CFunc{fl, cleanupFuncp});
            cstmtp->جوڑو(", vlSelf);");
        }

        // Clear global activity flag
        cleanupFuncp->جوڑو_Stmtsp(
            جديد AstCStmt{m_چوٹی_Scopep->fileline(), "vlSymsp->__Vm_activity = false;"s});

        // Clear fine grained activity flags
        لكل (uint32_t i = 0; i < m_activity_نمبر; ++i) {
            Ast_گرہ* ثابت clrp = جديد Ast_سونپو{fl, selectActivity(fl, i, VAccess::WRITE),
                                                جديد AstConst{fl, AstConst::ثنائی_ہندسہ_False{}}};
            cleanupFuncp->جوڑو_Stmtsp(clrp);
        }
    }

    فراغ createTraceFunctions() {
        // Detect and remove duplicate values
        detectDuplicates();

        گراف_Dtype_Prune();
        ونڈو_Trace_Decl_Funcs();
        m_گراف.remove_Redundant_کنارے_Max(&V3_گراف_کنارا::followAlwaysTrue);

        // Simplify & optimize the graph
        إذا (dump_گراف_Level() >= 6) m_گراف.dumpDotFilePrefixed("trace_pre");
        گراف_Simplify(سچ);
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
        ترتیب_دیو_Traces(traces, nNonConstCodes);

        // Our keys are now sorted to have same activity number adjacent, then
        // by trace order. (Better would be execution order for cache
        // efficiency....) Last are constants and non-changers, as then the
        // last value vector is more compact

        // Create the trace registration function
        m_regFuncp = جديد AstCFunc{m_چوٹی_Scopep->fileline(), "trace_register", m_چوٹی_Scopep};
        m_regFuncp->argTypes(v3Global.opt.trace_Class_ادھار() + "* tracep");
        m_regFuncp->اندراج_نقطہ(سچ);
        m_regFuncp->isTrace(سچ);
        m_regFuncp->slow(سچ);
        m_regFuncp->isStatic(خطأ);
        m_regFuncp->isLoose(سچ);
        m_چوٹی_Scopep->جوڑو_Blocksp(m_regFuncp);

        // Create the const dump functions. Also allocates trace codes.
        createConstTraceFunctions(traces);

        // Create the full and incremental dump functions
        createNonConstTraceFunctions(traces, nNonConstCodes, m_parallelism);

        // Root-traced libraries alias wrapper IOs onto the existing top-module codes.
        إذا (!v3Global.opt.libCreate().خالی_تھاواں()) {
            std::سمتیہ<AstTraceDecl*> جڑ_Declps;
            std::سمتیہ<AstTraceDecl*> چوٹی_Declps;
            TraceInitDeclCollector جڑ_Collector{جڑ_Declps};
            TraceInitDeclCollector چوٹی_Collector{چوٹی_Declps};
            لكل (Ast_گرہ* blockp = m_چوٹی_Scopep->blocksp(); blockp; blockp = blockp->nextp()) {
                AstCFunc* ثابت funcp = VN_CAST(blockp, CFunc);
                إذا (funcp && VString::startsWith(funcp->name(), "trace_init_leaf_root__")) {
                    جڑ_Collector.collect(funcp);
                } وإلا إذا (funcp && VString::startsWith(funcp->name(), "trace_init_leaf_top__")) {
                    چوٹی_Collector.collect(funcp);
                }
            }

            std::سمتیہ<منطقي> ورتیا(چوٹی_Declps.ناپ(), خطأ);
            لكل (AstTraceDecl* ثابت جڑ_Declp : جڑ_Declps) {
                لكل (ناپ_t i = 0; i < چوٹی_Declps.ناپ(); ++i) {
                    إذا (ورتیا[i] || !same_جڑ_Init_Alias(جڑ_Declp, چوٹی_Declps[i])) استمر;
                    جڑ_Declp->کوڈ(چوٹی_Declps[i]->کوڈ());
                    ورتیا[i] = سچ;
                    اكسر;
                }
            }
        }

        // Remove refs to traced values from TraceDecl nodes, these have now moved under
        // TraceInc
        لكل (ثابت خودکار& i : traces) {
            Ast_گرہ* ثابت valuep = i.دوجا->nodep()->valuep();
            VL_DO_DANGLING(valuep->unlink_Fr_پٹھ()->delete_درخت(), valuep);
        }

        // Create the trace cleanup function clearing the activity flags
        createCleanupFunction();
    }

    Trace_CFunc_راس* getCFuncVertexp(AstCFunc* nodep) {
        V3_گراف_راس* ثابت vtxp = nodep->user1u().منزل_گراف_راس();
        Trace_CFunc_راس* vertexp = vtxp ? vtxp->cast<Trace_CFunc_راس>() : خالی_سوچک;
        إذا (!vertexp) {
            vertexp = جديد Trace_CFunc_راس{&m_گراف, nodep};
            nodep->user1p(vertexp);
        }
        أرجع vertexp;
    }
    Trace_Activity_راس* getActivityVertexp(Ast_گرہ* nodep, منطقي slow) {
        V3_گراف_راس* ثابت vtxp = nodep->user3u().منزل_گراف_راس();
        Trace_Activity_راس* vertexp = vtxp ? vtxp->cast<Trace_Activity_راس>() : خالی_سوچک;
        إذا (!vertexp) {
            vertexp = جديد Trace_Activity_راس{&m_گراف, nodep, slow};
            nodep->user3p(vertexp);
        }
        vertexp->slow(slow);
        أرجع vertexp;
    }

    // VISITORS
    فراغ ویکھو(AstNetlist* nodep) override {
        // Add vertexes for all TraceDecl, and edges from VARs each trace looks at
        m_finding = خطأ;
        iterateChildren(nodep);

        // Add vertexes for all CFUNCs, and edges to VARs the func sets
        m_finding = سچ;
        iterateChildren(nodep);

        // Create the trace functions and insert them into the tree
        createTraceFunctions();

        // Save number of trace codes used
        nodep->nTraceCodes(m_کوڈ);
    }
    فراغ ویکھو(Ast_گرہ_Module* nodep) override {
        إذا (nodep->is_چوٹی()) m_چوٹی_Modp = nodep;
        iterateChildren(nodep);
    }
    فراغ ویکھو(AstVarScope* nodep) override {
        إذا (!m_finding) {
            إذا (ثابت AstIface* ثابت ifacep = nodep->varp()->sensIfacep()) {
                m_ifaceMemberVscps[{ifacep, nodep->varp()->name()}].ڈالو_پٹھ(nodep);
            }
        }
    }
    فراغ ویکھو(Ast_Stmt_اظہاریہ* nodep) override {
        إذا (!m_finding && !nodep->user2()) {
            إذا (AstCCall* ثابت callp = VN_CAST(nodep->exprp(), CCall)) {
                UINFO(8, "   CCALL " << callp);
                // See if there are other calls in same statement list;
                // If so, all funcs might share the same activity code
                Trace_Activity_راس* ثابت activityVtxp
                    = getActivityVertexp(nodep, callp->funcp()->slow());
                لكل (Ast_گرہ* nextp = nodep; nextp; nextp = nextp->nextp()) {
                    إذا (Ast_Stmt_اظہاریہ* ثابت stmtp = VN_CAST(nextp, Stmt_اظہاریہ)) {
                        إذا (AstCCall* ثابت ccallp = VN_CAST(stmtp->exprp(), CCall)) {
                            stmtp->user2(سچ);  // Processed
                            UINFO(8, "     SubCCALL " << ccallp);
                            V3_گراف_راس* ثابت ccallFuncVtxp = getCFuncVertexp(ccallp->funcp());
                            activityVtxp->slow(ccallp->funcp()->slow());
                            جديد V3_گراف_کنارا{&m_گراف, activityVtxp, ccallFuncVtxp, 1};
                        }
                    }
                }
            }
        }
        iterateChildren(nodep);
    }
    فراغ ویکھو(AstCFunc* nodep) override {
        UINFO(8, "   CFUNC " << nodep);
        V3_گراف_راس* ثابت funcVtxp = getCFuncVertexp(nodep);
        إذا (!m_finding) {  // If public, we need a unique activity code to allow for sets
                           // directly in this func
            إذا (nodep->func_عوامی() || nodep->dpiExportImpl() || nodep->اندراج_نقطہ()
                || nodep->isCoroutine()) {
                // Cannot treat a coroutine as slow, it may be resumed later
                ثابت منطقي slow = nodep->slow() && !nodep->isCoroutine();
                Trace_Activity_راس* ثابت activityVtxp = getActivityVertexp(nodep, slow);
                جديد V3_گراف_کنارا{&m_گراف, activityVtxp, funcVtxp, 1};
            }
        }
        VL_RESTORER(m_cfuncp);
        m_cfuncp = nodep;
        iterateChildren(nodep);
    }
    فراغ ویکھو(AstTraceDecl* nodep) override {
        UINFO(8, "   TRACE " << nodep);
        إذا (!m_finding && !nodep->وچ_Dtype_Func()) {
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
            V3_گراف_راس* ثابت vertexp = جديد Trace_Trace_راس{&m_گراف, nodep};
            nodep->user1p(vertexp);

            UASSERT_OBJ(m_cfuncp, nodep, "Trace not under func");
            m_cfuncp->user2(سچ);
            VL_RESTORER(m_tracep);
            m_tracep = nodep;
            iterateChildren(nodep);
        }
    }
    فراغ ویکھو(AstVarRef* nodep) override {
        UASSERT_OBJ(nodep->varScopep(), nodep, "No var scope?");
        AstVarScope* ثابت varscopep = nodep->varScopep();
        V3_گراف_راس* varVtxp = varscopep->user1u().منزل_گراف_راس();
        إذا (m_tracep) {
            UASSERT_OBJ(nodep->access().isReadOnly(), nodep, "Lvalue in trace?  Should be const.");
            إذا (!varVtxp) {
                varVtxp = جديد Trace_Var_راس{&m_گراف, nodep->varScopep()};
                nodep->varScopep()->user1p(varVtxp);
            }
            V3_گراف_راس* ثابت traceVtxp = m_tracep->user1u().منزل_گراف_راس();
            جديد V3_گراف_کنارا{&m_گراف, varVtxp, traceVtxp, 1};
            إذا (nodep->varp()->isPrimaryInish()  // Always need to trace primary inputs
                || nodep->varp()->is_Sig_عوامی()) {  // Or ones user can change
                جديد V3_گراف_کنارا{&m_گراف, m_alwaysVtxp, traceVtxp, 1};
            }
            إذا (m_tracep->dtypeCallp()) varscopep->user2p(m_tracep);
        } وإلا إذا (m_cfuncp && m_finding && nodep->access().isWriteOrRW()) {
            V3_گراف_راس* ثابت funcVtxp = getCFuncVertexp(m_cfuncp);
            إذا (varVtxp) {  // else we're not tracing this signal
                جديد V3_گراف_کنارا{&m_گراف, funcVtxp, varVtxp, 1};
                AstTraceDecl* ثابت declp = VN_AS(varscopep->user2p(), TraceDecl);
                إذا (declp) {
                    V3_گراف_راس* ثابت cFuncVtxp = getCFuncVertexp(m_cfuncp);
                    لكل (ثابت V3_گراف_کنارا& کنارا : cFuncVtxp->وچ_کنارے()) {
                        V3_گراف_راس* ثابت activityp = کنارا.fromp();
                        إذا (!declp->user3p()) {
                            declp->user3p(activityp);
                        } وإلا إذا (declp->user3u().منزل_گراف_راس() != activityp) {
                            declp->user2(سچ);
                        }
                    }
                }
            }
        }
    }
    فراغ ویکھو(AstMemberSel* nodep) override {
        إذا (m_cfuncp && m_finding && nodep->access().isWriteOrRW()) {
            AstIfaceRefDType* ثابت dtypep
                = VN_CAST(nodep->fromp()->dtypep()->skipRefp(), IfaceRefDType);
            إذا (dtypep && dtypep->isVirtual()) {
                ثابت خودکار it = m_ifaceMemberVscps.لبھو({dtypep->ifacep(), nodep->varp()->name()});
                إذا (it != m_ifaceMemberVscps.سرا()) {
                    V3_گراف_راس* ثابت funcVtxp = getCFuncVertexp(m_cfuncp);
                    لكل (AstVarScope* ثابت vscp : it->دوجا) {
                        V3_گراف_راس* varVtxp = vscp->user1u().منزل_گراف_راس();
                        إذا (!varVtxp) {
                            varVtxp = جديد Trace_Var_راس{&m_گراف, vscp};
                            vscp->user1p(varVtxp);
                        }
                        جديد V3_گراف_کنارا{&m_گراف, funcVtxp, varVtxp, 1};
                    }
                }
            }
        }
        iterateChildren(nodep);
    }
    //--------------------
    فراغ ویکھو(Ast_گرہ* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    واضح TraceVisitor(AstNetlist* nodep)
        : m_alwaysVtxp{جديد Trace_Activity_راس{&m_گراف, Trace_Activity_راس::ACTIVITY_ALWAYS}} {
        nodep->user2_Clear_درخت();  // TraceDecl multiple activities flag
        nodep->user3_Clear_درخت();  // TraceDecl TraceActivityVertex (assumes we start at nullptr)
        iterate(nodep);
        nodep->foreach([](AstTraceDecl* ثابت declp) {
            إذا (declp->وچ_Dtype_Func()) {
                declp->valuep()->unlink_Fr_پٹھ()->delete_درخت();
                declp->valuep(خالی_سوچک);
            }
        });
    }
    ~TraceVisitor() override {
        V3Stats::جوڑو_Stat("Tracing, Activity setters", m_statSetters);
        V3Stats::جوڑو_Stat("Tracing, Activity slow blocks", m_statSettersSlow);
        V3Stats::جوڑو_Stat("Tracing, Unique trace codes", m_کوڈ);
        V3Stats::جوڑو_Stat("Tracing, Unique traced signals", m_statUniqSigs);
    }
};

//######################################################################
// Trace class functions

فراغ V3Trace::traceAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { TraceVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_درخت("trace", 0, dump_درخت_Either_Level() >= 3);
}
