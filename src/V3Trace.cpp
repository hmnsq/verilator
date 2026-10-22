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
    Ast_ڳنڍ* ثابت m_insertp;
    int32_t m_activity_ڪوڊ;
    منطقي m_slow;  // If always slow, we can use the same code
عام:
    تعداد { ACTIVITY_NEVER = ((1UL << 31) - 1) };
    تعداد { ACTIVITY_ALWAYS = ((1UL << 31) - 2) };
    تعداد { ACTIVITY_SLOW = 0 };
    Trace_Activity_راس(V3_گراف* graphp, Ast_ڳنڍ* nodep, منطقي slow)
        : V3_گراف_راس{graphp}
        , m_insertp{nodep} {
        m_activity_ڪوڊ = 0;
        m_slow = slow;
    }
    Trace_Activity_راس(V3_گراف* graphp, int32_t ڪوڊ)
        : V3_گراف_راس{graphp}
        , m_insertp{خالي_اشارو} {
        m_activity_ڪوڊ = ڪوڊ;
        m_slow = خطأ;
    }
    ~Trace_Activity_راس() override = افتراضي;
    // ACCESSORS
    Ast_ڳنڍ* insertp() ثابت {
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
    int32_t activity_ڪوڊ() ثابت { أرجع m_activity_ڪوڊ; }
    منطقي activityAlways() ثابت { أرجع activity_ڪوڊ() == ACTIVITY_ALWAYS; }
    منطقي activitySlow() ثابت { أرجع activity_ڪوڊ() == ACTIVITY_SLOW; }
    فراغ activity_ڪوڊ(int32_t ڪوڊ) { m_activity_ڪوڊ = ڪوڊ; }
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
    File_سٽ* fileline() ثابت override { أرجع nodep()->fileline(); }
};

صنف Trace_Trace_راس final : عام V3_گراف_راس {
    VL_RTTI_IMPL(Trace_Trace_راس, V3_گراف_راس)
    AstTraceDecl* ثابت m_nodep;  // TRACEINC this represents
    // nullptr, or other vertex with the real code() that duplicates this one
    Trace_Trace_راس* m_duplicatep = خالي_اشارو;
    // When aliasing to a dtype parent, offset of the member within the dtype
    uint32_t m_dtype_Alias_وٿي = 0;

عام:
    Trace_Trace_راس(V3_گراف* graphp, AstTraceDecl* nodep)
        : V3_گراف_راس{graphp}
        , m_nodep{nodep} {}
    ~Trace_Trace_راس() override = افتراضي;
    // ACCESSORS
    AstTraceDecl* nodep() ثابت { أرجع m_nodep; }
    string name() ثابت override { أرجع nodep()->name(); }
    string dotColor() ثابت override { أرجع "red"; }
    File_سٽ* fileline() ثابت override { أرجع nodep()->fileline(); }
    Trace_Trace_راس* duplicatep() ثابت { أرجع m_duplicatep; }
    فراغ duplicatep(Trace_Trace_راس* dupp) {
        UASSERT_OBJ(!duplicatep(), nodep(), "Assigning duplicatep() to already duplicated node");
        m_duplicatep = dupp;
    }
    فراغ redirectDuplicatep(Trace_Trace_راس* dupp) { m_duplicatep = dupp; }
    uint32_t dtype_Alias_وٿي() ثابت { أرجع m_dtype_Alias_وٿي; }
    فراغ dtype_Alias_وٿي(uint32_t وٿي) { m_dtype_Alias_وٿي = وٿي; }
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
    File_سٽ* fileline() ثابت override { أرجع nodep()->fileline(); }
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
    ثابت VNUser1_اندريون_Use m_inuser1;
    ثابت VNUser2_اندريون_Use m_inuser2;
    ثابت VNUser3_اندريون_Use m_inuser3;
    // VNUser4InUse     In V3Hasher via V3DupFinder

    // STATE
    Ast_ڳنڍ_Module* m_چوٽي_Modp = خالي_اشارو;  // Module to add variables to
    AstScope* ثابت m_چوٽي_Scopep = v3Global.rootp()->چوٽي_Scopep()->scopep();  // The top AstScope
    AstCFunc* m_cfuncp = خالي_اشارو;  // C function adding to graph
    AstCFunc* m_regFuncp = خالي_اشارو;  // Trace registration function
    AstCFunc* m_actAllFuncp = خالي_اشارو;  // Set all activity function
    AstTraceDecl* m_tracep = خالي_اشارو;  // Trace function adding to graph
    AstVarScope* m_activityVscp = خالي_اشارو;  // Activity variable
    uint32_t m_activity_عدد = 0;  // Count of fields in activity variable
    uint32_t m_ڪوڊ = 0;  // Trace ident code# being assigned
    V3_گراف m_گراف;  // Var/CFunc tracking
    Trace_Activity_راس* ثابت m_alwaysVtxp;  // "Always trace" vertex
    منطقي m_finding = خطأ;  // Pass one of algorithm?
    بنية DtypeFuncs final {
    عام:
        AstCFunc* ڀريل_Funcp = خالي_اشارو;
        AstCFunc* chgFuncp = خالي_اشارو;
    };
    std::unordered_map<ثابت Ast_ڳنڍ_DType*, DtypeFuncs>
        m_dtypeNonConstFuncs;  // Full / Chg funcs per type
    std::unordered_map<ثابت Ast_ڳنڍ_DType*, AstCFunc*> m_dtypeConstFuncs;  // Const func per type
    V3_Unique_نالا m_dtype_نالا{""};  // Unique type func names

    // Trace parallelism. Only VCD tracing can be parallelized at this time.
    ثابت uint32_t m_parallelism
        = v3Global.opt.useTraceParallel() ? static_cast<uint32_t>(v3Global.opt.ڌاڳا()) : 1;

    VDouble0 m_statSetters;  // Statistic tracking
    VDouble0 m_statSettersSlow;  // Statistic tracking
    VDouble0 m_statUniqSigs;  // Statistic tracking

    // All activity numbers applying to a given trace
    استعمال Act_ڪوڊ_سيٽ = std::سيٽ<uint32_t>;
    // For activity set, what traces apply
    استعمال TraceVec = std::multimap<Act_ڪوڊ_سيٽ, Trace_Trace_راس*>;
    // Candidate interface-member VarScopes keyed by (interface type, member name)
    std::map<std::pair<ثابت AstIface*, std::string>, std::ويڪٽر<AstVarScope*>>
        m_ifaceMemberVscps;

    صنف TraceInitDeclCollector final : عام VNVisitor {
        std::ويڪٽر<AstTraceDecl*>& m_declps;  // Output: trace declarations found so far
        std::سيٽ<ثابت AstCFunc*> m_ڏٺل_Funcps;

        فراغ گهمو(AstTraceDecl* nodep) override { m_declps.ڌڪو_پٺ(nodep); }
        فراغ گهمو(AstCCall* nodep) override {
            إذا (AstCFunc* ثابت funcp = nodep->funcp()) collect(funcp);
        }
        فراغ گهمو(Ast_ڳنڍ* nodep) override { iterateChildren(nodep); }

    عام:
        واضح TraceInitDeclCollector(std::ويڪٽر<AstTraceDecl*>& declps)
            : m_declps{declps} {}
        فراغ collect(AstCFunc* funcp) {
            إذا (funcp && m_ڏٺل_Funcps.وجهو(funcp).ٻيو) iterate(funcp);
        }
    };

    // METHODS

    ساكن منطقي same_پاڙ_Init_Alias(ثابت AstTraceDecl* پاڙ_Declp, ثابت AstTraceDecl* چوٽي_Declp) {
        ثابت VNumRange& پاڙ_ٻٽو_انگ_Range = پاڙ_Declp->ٻٽو_انگ_Range();
        ثابت VNumRange& چوٽي_ٻٽو_انگ_Range = چوٽي_Declp->ٻٽو_انگ_Range();
        أرجع پاڙ_Declp->showname() == چوٽي_Declp->showname()
               && پاڙ_Declp->declDirection() == چوٽي_Declp->declDirection()
               && پاڙ_Declp->ويڪر_Min() == چوٽي_Declp->ويڪر_Min()
               && پاڙ_ٻٽو_انگ_Range.ranged() == چوٽي_ٻٽو_انگ_Range.ranged()
               && (!پاڙ_ٻٽو_انگ_Range.ranged()
                   || (پاڙ_ٻٽو_انگ_Range.کاٻي() == چوٽي_ٻٽو_انگ_Range.کاٻي()
                       && پاڙ_ٻٽو_انگ_Range.ساڄي() == چوٽي_ٻٽو_انگ_Range.ساڄي()));
    }

    فراغ detectDuplicates() {
        UINFO(9, "Finding duplicates");
        // Note uses user4
        V3DupFinder dupFinder;  // Duplicate code detection

        // Compute member offsets within dtype instances incrementally.
        // For each dtype member TraceDecl, stores its code offset within the
        // dtype parent's code range.
        std::unordered_map<ثابت AstTraceDecl*, uint32_t> memberOffsets;
        std::unordered_map<ثابت AstTraceDecl*, uint32_t> running_وٿي;

        // For fixup pass: track dups redirected to dtype parents, so we can
        // fix them if the parent's user2 is later set during the loop.
        // Maps dup vertex -> original canonical member vertex.
        std::ويڪٽر<std::pair<Trace_Trace_راس*, Trace_Trace_راس*>> dtype_والد_Redirects;

        // Hash all of the traced values and find if there are any duplicates
        لكل (V3_گراف_راس& vtx : m_گراف.vertices()) {
            إذا (Trace_Trace_راس* ثابت vvertexp = vtx.cast<Trace_Trace_راس>()) {
                AstTraceDecl* ثابت nodep = vvertexp->nodep();
                إذا (nodep->dtypeCallp()) استمر;

                إذا (nodep->dtypeDeclp()) {
                    uint32_t& وٿي = running_وٿي[nodep->dtypeDeclp()];
                    memberOffsets[nodep] = وٿي;
                    وٿي += nodep->ڪوڊ_Inc();
                }

                UASSERT_OBJ(!vvertexp->duplicatep(), nodep, "Should not be a duplicate");
                ثابت خودڪار dupit = dupFinder.ڳوليو_Duplicate(nodep);
                إذا (dupit == dupFinder.پڇاڙي()) {
                    dupFinder.وجهو(nodep);
                } وإلا {
                    AstTraceDecl* ثابت dupDeclp = VN_AS(dupit->ٻيو, TraceDecl);
                    UASSERT_OBJ(dupDeclp, nodep, "Trace duplicate of wrong type");
                    Trace_Trace_راس* ثابت dupvertexp
                        = dupDeclp->user1u().منزل_گراف_راس()->cast<Trace_Trace_راس>();
                    UINFO(8, "  Orig " << dupDeclp << سٽ_پڄاڻي);
                    UINFO(8, "   dup " << nodep << سٽ_پڄاڻي);

                    إذا (dupDeclp->dtypeDeclp() && !dupDeclp->dtypeDeclp()->user2()) {
                        AstTraceDecl* ثابت dtypeParentp = dupDeclp->dtypeDeclp();
                        Trace_Trace_راس* ثابت dtypeVtxp
                            = dtypeParentp->user1u().منزل_گراف_راس()->cast<Trace_Trace_راس>();
                        ثابت خودڪار it2 = memberOffsets.ڳوليو(dupDeclp);
                        UASSERT_OBJ(it2 != memberOffsets.پڇاڙي(), dupDeclp,
                                    "Member offset not precomputed");
                        vvertexp->duplicatep(dtypeVtxp);
                        vvertexp->dtype_Alias_وٿي(it2->ٻيو);
                        dtype_والد_Redirects.emplace_پٺ(vvertexp, dupvertexp);
                    } وإلا {
                        vvertexp->duplicatep(dupvertexp);
                    }
                    إذا (nodep->dtypeDeclp()) nodep->dtypeDeclp()->user2(سچ);
                }
            }
        }

        لكل (ثابت خودڪار& pair : dtype_والد_Redirects) {
            Trace_Trace_راس* ثابت dupVtxp = pair.پهريون;
            Trace_Trace_راس* ثابت memberVtxp = pair.ٻيو;
            ثابت AstTraceDecl* ثابت والد_Declp = dupVtxp->duplicatep()->nodep();
            إذا (والد_Declp->user2()) {
                dupVtxp->redirectDuplicatep(memberVtxp);
                dupVtxp->dtype_Alias_وٿي(0);
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
                            Ast_ڳنڍ* stmtexprp = callp->backp();
                            VL_DO_DANGLING(ڌڪو_Deletep(stmtexprp->unlink_Fr_پٺ()), stmtexprp);
                        } وإلا {
                            منطقي خالي_جايون_Scope;
                            افعل {
                                خالي_جايون_Scope = خطأ;
                                Ast_ڳنڍ* ثابت declBackp = declp->backp();
                                Ast_ڳنڍ* ثابت declNextp = declp->nextp();
                                إذا (VN_IS(declBackp, Trace_ڌڪو_اڳياڙي)
                                    && VN_IS(declNextp, Trace_ڪڍو_اڳياڙي)) {
                                    VL_DO_DANGLING(ڌڪو_Deletep(declBackp->unlink_Fr_پٺ()),
                                                   declBackp);
                                    VL_DO_DANGLING(ڌڪو_Deletep(declNextp->unlink_Fr_پٺ()),
                                                   declNextp);
                                    خالي_جايون_Scope = سچ;
                                }
                            } طالما (خالي_جايون_Scope);
                        }
                        // Can't purge until we finish this pass
                        ڌڪو_Deletep(declp->unlink_Fr_پٺ());  // declp used below
                        vvertexp->reroute_ڪنارا(&m_گراف);
                        vvertexp->unlinkDelete(&m_گراف);
                    }
                    declp->dtypeDeclp(خالي_اشارو);
                }
            }
        }
    }

    فراغ ورهايو_Trace_Decl_Funcs() {
        ثابت صحيح ورهايو_حد_بندي = v3Global.opt.output_ورهايو_CTrace() ? v3Global.opt.output_ورهايو_CTrace()
                                                                : std::numeric_limits<صحيح>::max();
        إذا (ورهايو_حد_بندي == std::numeric_limits<صحيح>::max()) أرجع;

        لكل (Ast_ڳنڍ* nodep = m_چوٽي_Scopep->blocksp(); nodep; nodep = nodep->nextp()) {
            AstCFunc* ثابت funcp = VN_CAST(nodep, CFunc);
            إذا (!funcp || !funcp->user2()) استمر;

            ثابت string بنياد_Name = funcp->name();
            uint32_t sub_انگ = 1;
            صحيح cur_سائيز = 0;
            AstCFunc* curFuncp = funcp;
            std::ويڪٽر<Ast_ڳنڍ_Stmt*> callStmts;

            خودڪار شروعات_New_Sub_Func = [&]() {
                File_سٽ* ثابت flp = funcp->fileline();
                ثابت string newName = بنياد_Name + "_" + cvt_منزل_Str(sub_انگ++);
                AstCFunc* ثابت newFuncp = جديد AstCFunc{flp, newName, m_چوٽي_Scopep};
                newFuncp->argTypes(v3Global.opt.trace_Class_بنياد() + "* tracep");
                newFuncp->isTrace(سچ);
                newFuncp->isStatic(خطأ);
                newFuncp->isLoose(سچ);
                newFuncp->slow(سچ);
                newFuncp->dontCombine(سچ);
                m_چوٽي_Scopep->جوڙيو_Blocksp(newFuncp);
                newFuncp->جوڙيو_Stmtsp(جديد AstCStmt{flp, "const int c = vlSymsp->__Vm_baseCode;"});
                AstCCall* ثابت callp = جديد AstCCall{flp, newFuncp};
                callp->dtype_سيٽ_Void();
                callp->argTypes("tracep");
                callStmts.ڌڪو_پٺ(callp->makeStmt());
                curFuncp = newFuncp;
                cur_سائيز = 0;
            };

            لكل (Ast_ڳنڍ *stmtp = funcp->stmtsp(), *nextp; stmtp; stmtp = nextp) {
                nextp = stmtp->nextp();
                cur_سائيز += stmtp->ڳنڍ_ڳڻپ();

                إذا (curFuncp != funcp) {
                    stmtp->unlink_Fr_پٺ();
                    curFuncp->جوڙيو_Stmtsp(stmtp);
                }

                إذا (cur_سائيز > ورهايو_حد_بندي && nextp) شروعات_New_Sub_Func();
            }

            لكل (Ast_ڳنڍ_Stmt* ثابت callStmtp : callStmts) { funcp->جوڙيو_Stmtsp(callStmtp); }
        }
    }

    فراغ گراف_Simplify(منطقي initial) {
        إذا (initial) {
            // Remove all variable nodes
            لكل (V3_گراف_راس* ثابت vtxp : m_گراف.vertices().unlinkable()) {
                إذا (Trace_Var_راس* ثابت vvertexp = vtxp->cast<Trace_Var_راس>()) {
                    vvertexp->reroute_ڪنارا(&m_گراف);
                    vvertexp->unlinkDelete(&m_گراف);
                }
            }
            // Remove multiple variables connecting funcs to traces
            // We do this twice, as then we have fewer edges to multiply out in the below
            // expansion.
            m_گراف.remove_Redundant_ڪنارا_Max(&V3_گراف_ڪنارو::followAlwaysTrue);
            // Remove all Cfunc nodes
            لكل (V3_گراف_راس* ثابت vtxp : m_گراف.vertices().unlinkable()) {
                إذا (Trace_CFunc_راس* ثابت vvertexp = vtxp->cast<Trace_CFunc_راس>()) {
                    vvertexp->reroute_ڪنارا(&m_گراف);
                    vvertexp->unlinkDelete(&m_گراف);
                }
            }
        }

        // Remove multiple variables connecting funcs to traces
        m_گراف.remove_Redundant_ڪنارا_Max(&V3_گراف_ڪنارو::followAlwaysTrue);

        // If there are any edges from a always, keep only the always
        لكل (V3_گراف_راس& vtx : m_گراف.vertices()) {
            إذا (Trace_Trace_راس* ثابت vvertexp = vtx.cast<Trace_Trace_راس>()) {
                // Search for the incoming always edge
                ثابت V3_گراف_ڪنارو* alwaysEdgep = خالي_اشارو;
                لكل (ثابت V3_گراف_ڪنارو& ڪنارو : vvertexp->اندريون_ڪنارا()) {
                    ثابت Trace_Activity_راس* ثابت actVtxp
                        = ڪنارو.fromp()->as<ثابت Trace_Activity_راس>();
                    إذا (actVtxp->activityAlways()) {
                        alwaysEdgep = &ڪنارو;
                        اكسر;
                    }
                }
                // If always edge exists, remove all other edges
                إذا (alwaysEdgep) {
                    لكل (V3_گراف_ڪنارو* ثابت edgep : vvertexp->اندريون_ڪنارا().unlinkable()) {
                        إذا (edgep != alwaysEdgep) VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
                    }
                }
            }
        }

        // Activity points with no outputs can be removed
        لكل (V3_گراف_راس* ثابت vtxp : m_گراف.vertices().unlinkable()) {
            إذا (Trace_Activity_راس* ثابت aVtxp = vtxp->cast<Trace_Activity_راس>()) {
                // Leave in the always vertex for later use.
                إذا (aVtxp != m_alwaysVtxp && aVtxp->ٻاهريون_خالي_جايون()) {
                    VL_DO_DANGLING(aVtxp->unlinkDelete(&m_گراف), aVtxp);
                }
            }
        }
    }

    uint32_t assignactivityNumbers() {
        uint32_t activity_عدد = 1;  // Note 0 indicates "slow" only
        لكل (V3_گراف_راس& vtx : m_گراف.vertices()) {
            إذا (Trace_Activity_راس* ثابت vvertexp = vtx.cast<Trace_Activity_راس>()) {
                إذا (vvertexp != m_alwaysVtxp) {
                    إذا (vvertexp->slow()) {
                        vvertexp->activity_ڪوڊ(Trace_Activity_راس::ACTIVITY_SLOW);
                    } وإلا {
                        vvertexp->activity_ڪوڊ(activity_عدد++);
                    }
                }
            }
        }
        أرجع activity_عدد;
    }

    فراغ ترتيب_ڏيو_Traces(TraceVec& traces, uint32_t& nNonConstCodes) {
        // Populate sort structure
        traces.clear();
        nNonConstCodes = 0;
        لكل (V3_گراف_راس& vtx : m_گراف.vertices()) {
            إذا (Trace_Trace_راس* ثابت vtxp = vtx.cast<Trace_Trace_راس>()) {
                Act_ڪوڊ_سيٽ act_سيٽ;
                UINFO(9, "  Add to sort: " << vtxp);
                UINFOTREE(9, vtxp->nodep(), "", "trnode");
                لكل (ثابت V3_گراف_ڪنارو& ڪنارو : vtxp->اندريون_ڪنارا()) {
                    ثابت Trace_Activity_راس* ثابت cfvertexp
                        = ڪنارو.fromp()->cast<ثابت Trace_Activity_راس>();
                    UASSERT_OBJ(cfvertexp, vtxp->nodep(),
                                "Should have been function pointing to this trace");
                    UINFO(9, "   Activity: " << cfvertexp);
                    إذا (cfvertexp->activityAlways()) {
                        // If code 0, we always trace; ignore other codes
                        act_سيٽ.وجهو(Trace_Activity_راس::ACTIVITY_ALWAYS);
                    } وإلا {
                        act_سيٽ.وجهو(cfvertexp->activity_ڪوڊ());
                    }
                }
                UASSERT_OBJ(act_سيٽ.ڳڻپ(Trace_Activity_راس::ACTIVITY_ALWAYS) == 0
                                || act_سيٽ.سائيز() == 1,
                            vtxp->nodep(), "Always active trace has further triggers");
                // Count nodes
                إذا (!vtxp->duplicatep() && !act_سيٽ.خالي_جايون())
                    nNonConstCodes += vtxp->nodep()->ڪوڊ_Inc();
                إذا (act_سيٽ.خالي_جايون()) {
                    // If a trace doesn't have activity, it's constant, and we
                    // don't need to track changes on it.
                    act_سيٽ.وجهو(Trace_Activity_راس::ACTIVITY_NEVER);
                } وإلا إذا (act_سيٽ.ڳڻپ(Trace_Activity_راس::ACTIVITY_SLOW) && act_سيٽ.سائيز() > 1) {
                    // If a trace depends on the slow flag as well as other
                    // flags, remove the dependency on the slow flag. We will
                    // make slow routines set all activity flags.
                    act_سيٽ.erase(Trace_Activity_راس::ACTIVITY_SLOW);
                }
                traces.emplace(act_سيٽ, vtxp);
            }
        }
    }

    فراغ گراف_Optimize() {
        // Assign initial activity numbers to activity vertices
        assignactivityNumbers();

        // Sort the traces by activity sets
        TraceVec traces;
        uint32_t unused1;
        ترتيب_ڏيو_Traces(traces, unused1);

        // For each activity set with only a small number of signals, make those
        // signals always traced, as it's cheaper to check a few value changes
        // than to test a lot of activity flags
        خودڪار it = traces.begin();
        ثابت خودڪار پڇاڙي = traces.پڇاڙي();
        طالما (it != پڇاڙي) {
            خودڪار سر = it;
            // Approximate the complexity of the value change check
            uint32_t complexity = 0;
            ثابت Act_ڪوڊ_سيٽ& act_سيٽ = it->پهريون;
            لكل (; it != پڇاڙي && it->پهريون == act_سيٽ; ++it) {
                إذا (!it->ٻيو->duplicatep()) {
                    uint32_t لاڳت = 0;
                    ثابت AstTraceDecl* ثابت declp = it->ٻيو->nodep();
                    // The number of comparisons required by bufp->chg*
                    لاڳت += declp->isWide() ? declp->ڪوڊ_Inc() : 1;
                    // Arrays are traced by element
                    لاڳت *= declp->arrayRange().ranged() ? declp->arrayRange().elements() : 1;
                    // Note: Experiments factoring in the size of declp->valuep()
                    // showed no benefit in tracing speed, even for large trees,
                    // so we will leave those out for now.
                    complexity += لاڳت;
                }
            }
            // Leave alone always changing, never changing and signals only set in slow code
            إذا (act_سيٽ.ڳڻپ(Trace_Activity_راس::ACTIVITY_ALWAYS)) استمر;
            إذا (act_سيٽ.ڳڻپ(Trace_Activity_راس::ACTIVITY_NEVER)) استمر;
            إذا (act_سيٽ.ڳڻپ(Trace_Activity_راس::ACTIVITY_SLOW)) استمر;
            // If the value comparisons are cheaper to perform than checking the
            // activity flags make the signals always traced. Note this cost
            // equation is heuristic.
            إذا (complexity <= act_سيٽ.سائيز() * 2) {
                لكل (; سر != it; ++سر) {
                    جديد V3_گراف_ڪنارو{&m_گراف, m_alwaysVtxp, سر->ٻيو, 1};
                }
            }
        }

        گراف_Simplify(خطأ);
    }

    Ast_ڳنڍ_اظهار* selectActivity(File_سٽ* flp, uint32_t acode, ثابت VAccess& access) {
        أرجع جديد AstArraySel(flp, جديد AstVarRef{flp, m_activityVscp, access}, acode);
    }

    Ast_ڳنڍ* newActivitySetter(Ast_ڳنڍ* insertp, uint32_t ڪوڊ) {
        ++m_statSetters;
        File_سٽ* ثابت fl = insertp->fileline();
        Ast_سونپيو* ثابت setterp = جديد Ast_سونپيو{fl, selectActivity(fl, ڪوڊ, VAccess::WRITE),
                                                 جديد AstConst{fl, AstConst::ٻٽو_انگ_True{}}};
        أرجع setterp;
    }

    Ast_ڳنڍ* newActivityAll(Ast_ڳنڍ* insertp) {
        ++m_statSettersSlow;
        إذا (!m_actAllFuncp) {
            File_سٽ* ثابت flp = m_چوٽي_Scopep->fileline();
            AstCFunc* ثابت funcp = جديد AstCFunc{flp, "__VtraceActivitySetAll", m_چوٽي_Scopep};
            funcp->slow(سچ);
            funcp->isStatic(خطأ);
            funcp->isLoose(سچ);
            m_چوٽي_Scopep->جوڙيو_Blocksp(funcp);
            لكل (uint32_t ڪوڊ = 0; ڪوڊ < m_activity_عدد; ++ڪوڊ) {
                Ast_ڳنڍ* ثابت setterp = newActivitySetter(insertp, ڪوڊ);
                funcp->جوڙيو_Stmtsp(setterp);
            }
            m_actAllFuncp = funcp;
        }
        AstCCall* ثابت callp = جديد AstCCall{insertp->fileline(), m_actAllFuncp};
        callp->dtype_سيٽ_Void();
        أرجع callp->makeStmt();
    }

    فراغ createActivityFlags() {
        // Assign final activity numbers
        m_activity_عدد = assignactivityNumbers();

        // Create an array of bytes, not a bit vector, as they can be set
        // atomically by mtasks, and are cheaper to set (no need for
        // read-modify-write on the C type), and the speed of the tracing code
        // is the same on largish designs.
        File_سٽ* ثابت flp = m_چوٽي_Scopep->fileline();
        Ast_ڳنڍ_DType* ثابت newScalarDtp = جديد AstBasicDType{flp, VFlag_ٻٽو_انگ_Packed{}, 1};
        v3Global.rootp()->typeTablep()->جوڙيو_Typesp(newScalarDtp);
        AstRange* ثابت newArange
            = جديد AstRange{flp, VNumRange{static_cast<صحيح>(m_activity_عدد) - 1, 0}};
        Ast_ڳنڍ_DType* ثابت newArrDtp = جديد AstUnpackArrayDType{flp, newScalarDtp, newArange};
        v3Global.rootp()->typeTablep()->جوڙيو_Typesp(newArrDtp);
        m_activityVscp = v3Global.rootp()->چوٽي_Scopep()->create_عارضي("__VtraceActivity", newArrDtp);

        // Insert activity setters
        لكل (ثابت V3_گراف_راس& vtx : m_گراف.vertices()) {
            إذا (ثابت Trace_Activity_راس* ثابت vtxp = vtx.cast<ثابت Trace_Activity_راس>()) {
                Ast_ڳنڍ* setterp = خالي_اشارو;
                إذا (vtxp->activitySlow()) {
                    setterp = newActivityAll(vtxp->insertp());
                    // Just set all flags in slow code as it should be rare.
                    // This will be rolled up into a loop by V3Reloop.
                } وإلا إذا (!vtxp->activityAlways()) {
                    setterp = newActivitySetter(vtxp->insertp(), vtxp->activity_ڪوڊ());
                }
                إذا (setterp) {
                    Ast_ڳنڍ* ثابت insertp = vtxp->insertp();
                    إذا (Ast_Stmt_اظهار* ثابت stmtp = VN_CAST(insertp, Stmt_اظهار)) {
                        stmtp->جوڙيو_ايندڙ_هتي(setterp);
                    } وإلا إذا (AstCFunc* ثابت funcp = VN_CAST(insertp, CFunc)) {
                        // If there are awaits, insert the setter after each await
                        إذا (funcp->isCoroutine() && funcp->stmtsp()) {
                            funcp->stmtsp()->foreach_And_ايندڙ([setterp](AstCAwait* awaitp) {
                                awaitp->جوڙيو_ايندڙ_هتي(setterp->clone_وڻ(خطأ));
                            });
                        }
                        funcp->جوڙيو_Stmtsp(setterp);
                    } وإلا {
                        insertp->v3fatalSrc("Bad trace activity vertex");
                    }
                }
            }
        }
    }

    AstCFunc* newCFunc(VTraceType traceType, AstCFunc* چوٽي_Funcp, uint32_t func_انگ,
                       uint32_t بنياد_ڪوڊ = 0, ثابت AstTraceDecl* ثابت declp = خالي_اشارو,
                       منطقي declSub = خطأ) {
        // Create new function
        ثابت منطقي is_چوٽي_Func = !declp && چوٽي_Funcp == خالي_اشارو;
        std::string funcName;
        إذا (is_چوٽي_Func || declp) {
            funcName = traceType.func_اڳياڙي();
        } وإلا {
            funcName = چوٽي_Funcp->name();
            funcName += "_sub";
        }
        إذا (declp) {
            funcName += "_dtype";
            إذا (declSub) funcName += "_sub";
            funcName += "__";
            funcName = m_dtype_نالا.get(funcName);
        } وإلا {
            funcName += "_";
            funcName += cvt_منزل_Str(func_انگ);
        }

        File_سٽ* ثابت flp = m_چوٽي_Scopep->fileline();
        AstCFunc* ثابت funcp = جديد AstCFunc{flp, funcName, m_چوٽي_Scopep};
        funcp->isTrace(سچ);
        funcp->dontCombine(سچ);
        funcp->isLoose(سچ);
        funcp->slow(traceType != VTraceType::CHANGE);
        funcp->isStatic(is_چوٽي_Func);
        // Add it to top scope
        m_چوٽي_Scopep->جوڙيو_Blocksp(funcp);
        ثابت std::string bufArg
            = v3Global.opt.trace_Class_بنياد() + "::Buffer* bufp"
              + (declp ? (", uint32_t offset, const " + declp->dtypep()->cType("", سچ, سچ)
                          + " __VdtypeVar")
                       : "");
        إذا (is_چوٽي_Func) {
            // Top functions
            funcp->argTypes("void* voidSelf, " + bufArg);
            funcp->جوڙيو_Stmtsp(جديد AstCStmt{flp, EmitCUtil::void_Self_سونپيو(m_چوٽي_Modp)});
            funcp->جوڙيو_Stmtsp(جديد AstCStmt{flp, EmitCUtil::sym_Class_سونپيو()});
            // Add global activity check to change dump functions
            إذا (traceType == VTraceType::CHANGE) {  //
                funcp->جوڙيو_Stmtsp(
                    جديد AstCStmt{flp, "if (VL_UNLIKELY(!vlSymsp->__Vm_activity)) return;"});
            }
            // Register function
            AstCStmt* ثابت cstmtp = جديد AstCStmt{flp};
            m_regFuncp->جوڙيو_Stmtsp(cstmtp);
            إذا (traceType == VTraceType::CONSTANT) {
                cstmtp->جوڙيو("tracep->addConstCb(");
                cstmtp->جوڙيو(جديد Ast_Addr_جو_CFunc{flp, funcp});
                cstmtp->جوڙيو(", " + std::منزل_string(func_انگ) + ", vlSelf);");
            } وإلا إذا (traceType == VTraceType::ڀريل) {
                cstmtp->جوڙيو("tracep->addFullCb(");
                cstmtp->جوڙيو(جديد Ast_Addr_جو_CFunc{flp, funcp});
                cstmtp->جوڙيو(", " + std::منزل_string(func_انگ) + ", vlSelf);");
            } وإلا {
                cstmtp->جوڙيو("tracep->addChgCb(");
                cstmtp->جوڙيو(جديد Ast_Addr_جو_CFunc{flp, funcp});
                cstmtp->جوڙيو(", " + std::منزل_string(func_انگ) + ", vlSelf);");
            }
        } وإلا {
            // Sub functions
            funcp->argTypes(bufArg);
            // Setup base references. Note in rare occasions we can end up with an empty trace
            // sub function, hence the VL_ATTR_UNUSED attributes.
            إذا (traceType != VTraceType::CHANGE) {
                // Full dump sub function
                funcp->جوڙيو_Stmtsp(جديد AstCStmt{flp,  //
                                              string("uint32_t* const oldp VL_ATTR_UNUSED = "
                                                     "bufp->oldp(vlSymsp->__Vm_baseCode")
                                                  + (declp ? " + offset" : "") + ");\n"});
            } وإلا {
                // Change dump sub function
                funcp->جوڙيو_Stmtsp(جديد AstCStmt{flp,  //
                                              "uint32_t* const oldp VL_ATTR_UNUSED = "
                                              "bufp->oldp(vlSymsp->__Vm_baseCode + "
                                                  + (declp ? " offset" : cvt_منزل_Str(بنياد_ڪوڊ))
                                                  + ");\n"});
            }
            إذا (!declp) {
                // Add call to top function
                AstCCall* ثابت callp = جديد AstCCall{funcp->fileline(), funcp};
                callp->dtype_سيٽ_Void();
                callp->argTypes("bufp");
                چوٽي_Funcp->جوڙيو_Stmtsp(callp->makeStmt());
            }
        }
        //پورو
        UINFO(5, "  newCFunc " << funcp);
        أرجع funcp;
    }

    AstCFunc* newCDtypeSubFunc(VTraceType traceType, ثابت AstTraceDecl* ثابت declp,
                               AstCFunc* parentp) {
        AstCFunc* ثابت funcp = newCFunc(traceType, خالي_اشارو, 0, 0, declp, سچ);
        File_سٽ* ثابت flp = m_چوٽي_Scopep->fileline();
        AstCCall* ثابت callp = جديد AstCCall{flp, funcp};
        callp->dtype_سيٽ_Void();
        callp->argTypes("bufp, offset, __VdtypeVar");
        parentp->جوڙيو_Stmtsp(callp->makeStmt());
        أرجع funcp;
    }

    AstCFunc* createConstDtypeTraceFunctions(ثابت AstTraceDecl* declp) {
        ثابت Ast_ڳنڍ_DType* ثابت dtypep = declp->dtypep()->skipRefp();
        خودڪار pair = m_dtypeConstFuncs.emplace(dtypep, خالي_اشارو);
        إذا (pair.ٻيو) {
            File_سٽ* ثابت flp = declp->fileline();
            AstCFunc* ثابت funcp = newCFunc(VTraceType::CONSTANT, خالي_اشارو, 0, 0, declp);

            منطقي پهريون = سچ;
            لكل (Ast_ڳنڍ* callStmtp = declp->dtypeCallp()->funcp()->stmtsp(); callStmtp;
                 callStmtp = callStmtp->nextp()) {
                ثابت AstCCall* ثابت callp
                    = VN_CAST(VN_CAST(callStmtp, Stmt_اظهار)->exprp(), CCall);
                منطقي onlyOne = پهريون && !callStmtp->nextp();
                AstCFunc* ثابت subFuncp
                    = onlyOne ? funcp : newCDtypeSubFunc(VTraceType::CONSTANT, declp, funcp);
                لكل (Ast_ڳنڍ* stmtp = callp->funcp()->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    إذا (AstTraceDecl* ثابت ميدان_Declp = VN_CAST(stmtp, TraceDecl)) {
                        AstTraceInc* ثابت incp
                            = جديد AstTraceInc{flp, ميدان_Declp, VTraceType::CONSTANT};
                        subFuncp->جوڙيو_Stmtsp(incp);
                    }
                }
                پهريون = خطأ;
            }

            pair.پهريون->ٻيو = funcp;
        }

        أرجع pair.پهريون->ٻيو;
    }

    فراغ createConstTraceFunctions(ثابت TraceVec& traces) {
        ثابت صحيح ورهايو_حد_بندي = v3Global.opt.output_ورهايو_CTrace() ? v3Global.opt.output_ورهايو_CTrace()
                                                                : std::numeric_limits<صحيح>::max();

        AstCFunc* ثابت چوٽي_Funcp = newCFunc(VTraceType::CONSTANT, خالي_اشارو, 0);
        uint32_t sub_Func_انگ = 0;
        AstCFunc* subFuncp = خالي_اشارو;
        صحيح subStmts = 0;
        std::ويڪٽر<ثابت Trace_Trace_راس*> duplicates;
        لكل (خودڪار it = traces.cbegin(); it != traces.پڇاڙي(); ++it) {
            ثابت Trace_Trace_راس* ثابت vtxp = it->ٻيو;
            AstTraceDecl* ثابت declp = vtxp->nodep();
            إذا (vtxp->duplicatep()) {
                duplicates.ڌڪو_پٺ(vtxp);
                استمر;
            }

            // This is a canonical trace node. Assign trace code (signal number).
            UASSERT_OBJ(!declp->ڪوڊ_Assigned(), declp,
                        "Canonical node should not have code assigned yet");
            declp->ڪوڊ(m_ڪوڊ);
            ثابت uint32_t ڪوڊ_Inc = declp->ڪوڊ_Inc();
            m_ڪوڊ += ڪوڊ_Inc;
            ++m_statUniqSigs;

            // If this is a const signal, add the AstTraceInc
            ثابت Act_ڪوڊ_سيٽ& act_سيٽ = it->پهريون;
            إذا (act_سيٽ.ڳڻپ(Trace_Activity_راس::ACTIVITY_NEVER)) {
                // Crate new sub function if required
                إذا (!subFuncp || subStmts > ورهايو_حد_بندي) {
                    subStmts = 0;
                    subFuncp = newCFunc(VTraceType::CONSTANT, چوٽي_Funcp, sub_Func_انگ);
                    ++sub_Func_انگ;
                }
                File_سٽ* ثابت flp = declp->fileline();
                إذا (declp->dtypeCallp()) {
                    AstCFunc* ثابت funcp = createConstDtypeTraceFunctions(declp);
                    Ast_ڳنڍ_اظهار* argsp = خالي_اشارو;
                    argsp = Ast_ڳنڍ::جوڙيو_ايندڙ(argsp, declp->valuep()->clone_وڻ(خطأ));
                    AstCCall* ثابت callp = جديد AstCCall{flp, funcp, argsp};
                    callp->dtype_سيٽ_Void();
                    callp->argTypes(callp->argTypes() + "bufp, " + std::منزل_string(declp->ڪوڊ()));
                    subFuncp->جوڙيو_Stmtsp(callp->makeStmt());

                    subStmts += 1;
                } وإلا {
                    AstTraceInc* ثابت incp = جديد AstTraceInc{flp, declp, VTraceType::CONSTANT};
                    subFuncp->جوڙيو_Stmtsp(incp);
                    subStmts += incp->ڳنڍ_ڳڻپ();
                }
            }
        }

        // Assign codes to duplicate nodes from their canonicals
        لكل (ثابت Trace_Trace_راس* ثابت vtxp : duplicates) {
            AstTraceDecl* ثابت declp = vtxp->nodep();
            ثابت Trace_Trace_راس* ثابت canonVtxp = vtxp->duplicatep();
            ثابت AstTraceDecl* ثابت canonDeclp = canonVtxp->nodep();
            UASSERT_OBJ(!canonVtxp->duplicatep(), canonDeclp, "Canonical node is a duplicate");
            UASSERT_OBJ(canonDeclp->ڪوڊ_Assigned(), canonDeclp,
                        "Canonical node should have code assigned already");
            declp->ڪوڊ(canonDeclp->ڪوڊ() + vtxp->dtype_Alias_وٿي());
        }
    }

    DtypeFuncs createNonConstDtypeTraceFunctions(ثابت AstTraceDecl* declp) {
        Ast_ڳنڍ_DType* dtypep = declp->dtypep()->skipRefp();
        خودڪار pair = m_dtypeNonConstFuncs.emplace(dtypep, DtypeFuncs{});
        إذا (pair.ٻيو) {
            File_سٽ* ثابت flp = declp->fileline();
            AstCFunc* ثابت ڀريل_Funcp = newCFunc(VTraceType::ڀريل, خالي_اشارو, 0, 0, declp);
            AstCFunc* ثابت chgFuncp = newCFunc(VTraceType::CHANGE, خالي_اشارو, 0, 0, declp);

            منطقي پهريون = سچ;
            لكل (Ast_ڳنڍ* callStmtp = declp->dtypeCallp()->funcp()->stmtsp(); callStmtp;
                 callStmtp = callStmtp->nextp()) {
                ثابت AstCCall* ثابت callp
                    = VN_CAST(VN_CAST(callStmtp, Stmt_اظهار)->exprp(), CCall);
                منطقي onlyOne = پهريون && !callStmtp->nextp();
                AstCFunc* ثابت ڀريل_Sub_Funcp
                    = onlyOne ? ڀريل_Funcp : newCDtypeSubFunc(VTraceType::ڀريل, declp, ڀريل_Funcp);
                AstCFunc* ثابت chgSubFuncp
                    = onlyOne ? chgFuncp : newCDtypeSubFunc(VTraceType::CHANGE, declp, chgFuncp);
                لكل (Ast_ڳنڍ* stmtp = callp->funcp()->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    إذا (AstTraceDecl* ثابت ميدان_Declp = VN_CAST(stmtp, TraceDecl)) {
                        AstTraceInc* ثابت incFullp
                            = جديد AstTraceInc{flp, ميدان_Declp, VTraceType::ڀريل};
                        ڀريل_Sub_Funcp->جوڙيو_Stmtsp(incFullp);
                        AstTraceInc* ثابت incChgp
                            = جديد AstTraceInc{flp, ميدان_Declp, VTraceType::CHANGE};
                        chgSubFuncp->جوڙيو_Stmtsp(incChgp);
                    }
                }
                پهريون = خطأ;
            }

            pair.پهريون->ٻيو = {.ڀريل_Funcp = ڀريل_Funcp, .chgFuncp = chgFuncp};
        }

        أرجع pair.پهريون->ٻيو;
    }

    فراغ createNonConstTraceFunctions(ثابت TraceVec& traces, uint32_t nAllCodes,
                                      uint32_t parallelism) {
        ثابت صحيح ورهايو_حد_بندي = v3Global.opt.output_ورهايو_CTrace() ? v3Global.opt.output_ورهايو_CTrace()
                                                                : std::numeric_limits<صحيح>::max();

        // pre-incremented, so starts at 0
        uint32_t چوٽي_Func_انگ = std::numeric_limits<uint32_t>::max();
        TraceVec::const_iterator it = traces.begin();
        طالما (it != traces.پڇاڙي()) {
            AstCFunc* چوٽي_Ful_Funcp = خالي_اشارو;
            AstCFunc* چوٽي_Chg_Funcp = خالي_اشارو;
            AstCFunc* subFulFuncp = خالي_اشارو;
            AstCFunc* subChgFuncp = خالي_اشارو;
            uint32_t sub_Func_انگ = 0;
            صحيح subStmts = 0;
            ثابت uint32_t maxCodes = std::max((nAllCodes + parallelism - 1) / parallelism, 1U);
            uint32_t nCodes = 0;
            ثابت Act_ڪوڊ_سيٽ* prev_Act_سيٽ = خالي_اشارو;
            AstIf* ifp = خالي_اشارو;
            uint32_t بنياد_ڪوڊ = 0;
            لكل (; nCodes < maxCodes && it != traces.پڇاڙي(); ++it) {
                ثابت Act_ڪوڊ_سيٽ& act_سيٽ = it->پهريون;
                // Traced value never changes, no need to add it
                إذا (act_سيٽ.ڳڻپ(Trace_Activity_راس::ACTIVITY_NEVER)) استمر;

                ثابت Trace_Trace_راس* ثابت vtxp = it->ٻيو;
                AstTraceDecl* ثابت declp = vtxp->nodep();

                // This is a duplicate decl, no need to add it, but must set the
                // function index to the same as the canonical node.
                إذا (ثابت Trace_Trace_راس* ثابت canonVtxp = vtxp->duplicatep()) {
                    declp->fidx(canonVtxp->nodep()->fidx());
                    استمر;
                }

                // Create top function if not yet created
                إذا (!چوٽي_Ful_Funcp) {
                    ++چوٽي_Func_انگ;
                    چوٽي_Ful_Funcp = newCFunc(VTraceType::ڀريل, خالي_اشارو, چوٽي_Func_انگ);
                    چوٽي_Chg_Funcp = newCFunc(VTraceType::CHANGE, خالي_اشارو, چوٽي_Func_انگ);
                }

                // Create new sub function if required
                إذا (!subFulFuncp || subStmts > ورهايو_حد_بندي) {
                    بنياد_ڪوڊ = declp->ڪوڊ();
                    subStmts = 0;
                    subFulFuncp = newCFunc(VTraceType::ڀريل, چوٽي_Ful_Funcp, sub_Func_انگ, بنياد_ڪوڊ);
                    subChgFuncp = newCFunc(VTraceType::CHANGE, چوٽي_Chg_Funcp, sub_Func_انگ, بنياد_ڪوڊ);
                    ++sub_Func_انگ;
                    prev_Act_سيٽ = خالي_اشارو;
                    ifp = خالي_اشارو;
                }

                // If required, create the conditional node checking the activity flags
                إذا (!prev_Act_سيٽ || act_سيٽ != *prev_Act_سيٽ) {
                    File_سٽ* ثابت flp = m_چوٽي_Scopep->fileline();
                    ثابت منطقي always = act_سيٽ.ڳڻپ(Trace_Activity_راس::ACTIVITY_ALWAYS) != 0;
                    Ast_ڳنڍ_اظهار* condp = خالي_اشارو;
                    إذا (always) {
                        condp = جديد AstConst{flp, 1};  // Always true, will be folded later
                    } وإلا {
                        لكل (ثابت uint32_t act_ڪوڊ : act_سيٽ) {
                            Ast_ڳنڍ_اظهار* ثابت selp = selectActivity(flp, act_ڪوڊ, VAccess::READ);
                            condp = condp ? جديد AstOr{flp, condp, selp} : selp;
                        }
                    }
                    ifp = جديد AstIf{flp, condp};
                    إذا (!always) ifp->branchPred(VBranchPred::BP_UNLIKELY);
                    subChgFuncp->جوڙيو_Stmtsp(ifp);
                    subStmts += ifp->ڳنڍ_ڳڻپ();
                    prev_Act_سيٽ = &act_سيٽ;
                }

                // Add TraceInc nodes
                File_سٽ* ثابت flp = declp->fileline();
                إذا (declp->dtypeCallp()) {
                    DtypeFuncs funcs = createNonConstDtypeTraceFunctions(declp);
                    Ast_ڳنڍ_اظهار* argsp = خالي_اشارو;
                    argsp = Ast_ڳنڍ::جوڙيو_ايندڙ(argsp, declp->valuep()->clone_وڻ(خطأ));
                    AstCCall* ثابت callFullp = جديد AstCCall{flp, funcs.ڀريل_Funcp, argsp};
                    callFullp->dtype_سيٽ_Void();
                    callFullp->argTypes(callFullp->argTypes() + "bufp, "
                                        + std::منزل_string(declp->ڪوڊ()));
                    subFulFuncp->جوڙيو_Stmtsp(callFullp->makeStmt());
                    argsp = خالي_اشارو;
                    argsp = Ast_ڳنڍ::جوڙيو_ايندڙ(argsp, declp->valuep()->clone_وڻ(خطأ));
                    AstCCall* ثابت callChgp = جديد AstCCall{flp, funcs.chgFuncp, argsp};
                    callChgp->dtype_سيٽ_Void();
                    callChgp->argTypes(callChgp->argTypes() + "bufp, "
                                       + std::منزل_string(declp->ڪوڊ()));
                    ifp->جوڙيو_Thensp(callChgp->makeStmt());

                    subStmts += 2;
                } وإلا {
                    AstTraceInc* ثابت incFulp = جديد AstTraceInc{flp, declp, VTraceType::ڀريل};
                    subFulFuncp->جوڙيو_Stmtsp(incFulp);
                    AstTraceInc* ثابت incChgp
                        = جديد AstTraceInc{flp, declp, VTraceType::CHANGE, بنياد_ڪوڊ};
                    ifp->جوڙيو_Thensp(incChgp);

                    // Set the function index of the decl
                    declp->fidx(چوٽي_Func_انگ);

                    // Track splitting due to size
                    UASSERT_OBJ(incFulp->ڳنڍ_ڳڻپ() == incChgp->ڳنڍ_ڳڻپ(), declp,
                                "Should have equal cost");
                    ثابت VNumRange range = declp->arrayRange();
                    إذا (range.ranged()) {
                        // 2x because each element is a TraceInc and a VarRef
                        subStmts += range.elements() * 2;
                    } وإلا {
                        subStmts += incChgp->ڳنڍ_ڳڻپ();
                    }
                }

                // Track partitioning
                nCodes += declp->ڪوڊ_Inc();
            }
        }
    }

    فراغ createCleanupFunction() {
        File_سٽ* ثابت fl = m_چوٽي_Scopep->fileline();
        AstCFunc* ثابت cleanupFuncp = جديد AstCFunc{fl, "trace_cleanup", m_چوٽي_Scopep};
        cleanupFuncp->argTypes("void* voidSelf, " + v3Global.opt.trace_Class_بنياد()
                               + "* /*unused*/");
        cleanupFuncp->isTrace(سچ);
        cleanupFuncp->slow(خطأ);
        cleanupFuncp->isStatic(سچ);
        cleanupFuncp->isLoose(سچ);
        m_چوٽي_Scopep->جوڙيو_Blocksp(cleanupFuncp);
        cleanupFuncp->جوڙيو_Stmtsp(جديد AstCStmt{fl, EmitCUtil::void_Self_سونپيو(m_چوٽي_Modp)});
        cleanupFuncp->جوڙيو_Stmtsp(جديد AstCStmt{fl, EmitCUtil::sym_Class_سونپيو()});

        // Register it
        {
            AstCStmt* ثابت cstmtp = جديد AstCStmt{fl};
            m_regFuncp->جوڙيو_Stmtsp(cstmtp);
            cstmtp->جوڙيو("tracep->addCleanupCb(");
            cstmtp->جوڙيو(جديد Ast_Addr_جو_CFunc{fl, cleanupFuncp});
            cstmtp->جوڙيو(", vlSelf);");
        }

        // Clear global activity flag
        cleanupFuncp->جوڙيو_Stmtsp(
            جديد AstCStmt{m_چوٽي_Scopep->fileline(), "vlSymsp->__Vm_activity = false;"s});

        // Clear fine grained activity flags
        لكل (uint32_t i = 0; i < m_activity_عدد; ++i) {
            Ast_ڳنڍ* ثابت clrp = جديد Ast_سونپيو{fl, selectActivity(fl, i, VAccess::WRITE),
                                                جديد AstConst{fl, AstConst::ٻٽو_انگ_False{}}};
            cleanupFuncp->جوڙيو_Stmtsp(clrp);
        }
    }

    فراغ createTraceFunctions() {
        // Detect and remove duplicate values
        detectDuplicates();

        گراف_Dtype_Prune();
        ورهايو_Trace_Decl_Funcs();
        m_گراف.remove_Redundant_ڪنارا_Max(&V3_گراف_ڪنارو::followAlwaysTrue);

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
        ترتيب_ڏيو_Traces(traces, nNonConstCodes);

        // Our keys are now sorted to have same activity number adjacent, then
        // by trace order. (Better would be execution order for cache
        // efficiency....) Last are constants and non-changers, as then the
        // last value vector is more compact

        // Create the trace registration function
        m_regFuncp = جديد AstCFunc{m_چوٽي_Scopep->fileline(), "trace_register", m_چوٽي_Scopep};
        m_regFuncp->argTypes(v3Global.opt.trace_Class_بنياد() + "* tracep");
        m_regFuncp->داخلا_نقطو(سچ);
        m_regFuncp->isTrace(سچ);
        m_regFuncp->slow(سچ);
        m_regFuncp->isStatic(خطأ);
        m_regFuncp->isLoose(سچ);
        m_چوٽي_Scopep->جوڙيو_Blocksp(m_regFuncp);

        // Create the const dump functions. Also allocates trace codes.
        createConstTraceFunctions(traces);

        // Create the full and incremental dump functions
        createNonConstTraceFunctions(traces, nNonConstCodes, m_parallelism);

        // Root-traced libraries alias wrapper IOs onto the existing top-module codes.
        إذا (!v3Global.opt.libCreate().خالي_جايون()) {
            std::ويڪٽر<AstTraceDecl*> پاڙ_Declps;
            std::ويڪٽر<AstTraceDecl*> چوٽي_Declps;
            TraceInitDeclCollector پاڙ_Collector{پاڙ_Declps};
            TraceInitDeclCollector چوٽي_Collector{چوٽي_Declps};
            لكل (Ast_ڳنڍ* blockp = m_چوٽي_Scopep->blocksp(); blockp; blockp = blockp->nextp()) {
                AstCFunc* ثابت funcp = VN_CAST(blockp, CFunc);
                إذا (funcp && VString::startsWith(funcp->name(), "trace_init_leaf_root__")) {
                    پاڙ_Collector.collect(funcp);
                } وإلا إذا (funcp && VString::startsWith(funcp->name(), "trace_init_leaf_top__")) {
                    چوٽي_Collector.collect(funcp);
                }
            }

            std::ويڪٽر<منطقي> استعمال_ٿيل(چوٽي_Declps.سائيز(), خطأ);
            لكل (AstTraceDecl* ثابت پاڙ_Declp : پاڙ_Declps) {
                لكل (سائيز_t i = 0; i < چوٽي_Declps.سائيز(); ++i) {
                    إذا (استعمال_ٿيل[i] || !same_پاڙ_Init_Alias(پاڙ_Declp, چوٽي_Declps[i])) استمر;
                    پاڙ_Declp->ڪوڊ(چوٽي_Declps[i]->ڪوڊ());
                    استعمال_ٿيل[i] = سچ;
                    اكسر;
                }
            }
        }

        // Remove refs to traced values from TraceDecl nodes, these have now moved under
        // TraceInc
        لكل (ثابت خودڪار& i : traces) {
            Ast_ڳنڍ* ثابت valuep = i.ٻيو->nodep()->valuep();
            VL_DO_DANGLING(valuep->unlink_Fr_پٺ()->delete_وڻ(), valuep);
        }

        // Create the trace cleanup function clearing the activity flags
        createCleanupFunction();
    }

    Trace_CFunc_راس* getCFuncVertexp(AstCFunc* nodep) {
        V3_گراف_راس* ثابت vtxp = nodep->user1u().منزل_گراف_راس();
        Trace_CFunc_راس* vertexp = vtxp ? vtxp->cast<Trace_CFunc_راس>() : خالي_اشارو;
        إذا (!vertexp) {
            vertexp = جديد Trace_CFunc_راس{&m_گراف, nodep};
            nodep->user1p(vertexp);
        }
        أرجع vertexp;
    }
    Trace_Activity_راس* getActivityVertexp(Ast_ڳنڍ* nodep, منطقي slow) {
        V3_گراف_راس* ثابت vtxp = nodep->user3u().منزل_گراف_راس();
        Trace_Activity_راس* vertexp = vtxp ? vtxp->cast<Trace_Activity_راس>() : خالي_اشارو;
        إذا (!vertexp) {
            vertexp = جديد Trace_Activity_راس{&m_گراف, nodep, slow};
            nodep->user3p(vertexp);
        }
        vertexp->slow(slow);
        أرجع vertexp;
    }

    // VISITORS
    فراغ گهمو(AstNetlist* nodep) override {
        // Add vertexes for all TraceDecl, and edges from VARs each trace looks at
        m_finding = خطأ;
        iterateChildren(nodep);

        // Add vertexes for all CFUNCs, and edges to VARs the func sets
        m_finding = سچ;
        iterateChildren(nodep);

        // Create the trace functions and insert them into the tree
        createTraceFunctions();

        // Save number of trace codes used
        nodep->nTraceCodes(m_ڪوڊ);
    }
    فراغ گهمو(Ast_ڳنڍ_Module* nodep) override {
        إذا (nodep->is_چوٽي()) m_چوٽي_Modp = nodep;
        iterateChildren(nodep);
    }
    فراغ گهمو(AstVarScope* nodep) override {
        إذا (!m_finding) {
            إذا (ثابت AstIface* ثابت ifacep = nodep->varp()->sensIfacep()) {
                m_ifaceMemberVscps[{ifacep, nodep->varp()->name()}].ڌڪو_پٺ(nodep);
            }
        }
    }
    فراغ گهمو(Ast_Stmt_اظهار* nodep) override {
        إذا (!m_finding && !nodep->user2()) {
            إذا (AstCCall* ثابت callp = VN_CAST(nodep->exprp(), CCall)) {
                UINFO(8, "   CCALL " << callp);
                // See if there are other calls in same statement list;
                // If so, all funcs might share the same activity code
                Trace_Activity_راس* ثابت activityVtxp
                    = getActivityVertexp(nodep, callp->funcp()->slow());
                لكل (Ast_ڳنڍ* nextp = nodep; nextp; nextp = nextp->nextp()) {
                    إذا (Ast_Stmt_اظهار* ثابت stmtp = VN_CAST(nextp, Stmt_اظهار)) {
                        إذا (AstCCall* ثابت ccallp = VN_CAST(stmtp->exprp(), CCall)) {
                            stmtp->user2(سچ);  // Processed
                            UINFO(8, "     SubCCALL " << ccallp);
                            V3_گراف_راس* ثابت ccallFuncVtxp = getCFuncVertexp(ccallp->funcp());
                            activityVtxp->slow(ccallp->funcp()->slow());
                            جديد V3_گراف_ڪنارو{&m_گراف, activityVtxp, ccallFuncVtxp, 1};
                        }
                    }
                }
            }
        }
        iterateChildren(nodep);
    }
    فراغ گهمو(AstCFunc* nodep) override {
        UINFO(8, "   CFUNC " << nodep);
        V3_گراف_راس* ثابت funcVtxp = getCFuncVertexp(nodep);
        إذا (!m_finding) {  // If public, we need a unique activity code to allow for sets
                           // directly in this func
            إذا (nodep->func_عوامي() || nodep->dpiExportImpl() || nodep->داخلا_نقطو()
                || nodep->isCoroutine()) {
                // Cannot treat a coroutine as slow, it may be resumed later
                ثابت منطقي slow = nodep->slow() && !nodep->isCoroutine();
                Trace_Activity_راس* ثابت activityVtxp = getActivityVertexp(nodep, slow);
                جديد V3_گراف_ڪنارو{&m_گراف, activityVtxp, funcVtxp, 1};
            }
        }
        VL_RESTORER(m_cfuncp);
        m_cfuncp = nodep;
        iterateChildren(nodep);
    }
    فراغ گهمو(AstTraceDecl* nodep) override {
        UINFO(8, "   TRACE " << nodep);
        إذا (!m_finding && !nodep->اندريون_Dtype_Func()) {
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
    فراغ گهمو(AstVarRef* nodep) override {
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
            جديد V3_گراف_ڪنارو{&m_گراف, varVtxp, traceVtxp, 1};
            إذا (nodep->varp()->isPrimaryInish()  // Always need to trace primary inputs
                || nodep->varp()->is_Sig_عوامي()) {  // Or ones user can change
                جديد V3_گراف_ڪنارو{&m_گراف, m_alwaysVtxp, traceVtxp, 1};
            }
            إذا (m_tracep->dtypeCallp()) varscopep->user2p(m_tracep);
        } وإلا إذا (m_cfuncp && m_finding && nodep->access().isWriteOrRW()) {
            V3_گراف_راس* ثابت funcVtxp = getCFuncVertexp(m_cfuncp);
            إذا (varVtxp) {  // else we're not tracing this signal
                جديد V3_گراف_ڪنارو{&m_گراف, funcVtxp, varVtxp, 1};
                AstTraceDecl* ثابت declp = VN_AS(varscopep->user2p(), TraceDecl);
                إذا (declp) {
                    V3_گراف_راس* ثابت cFuncVtxp = getCFuncVertexp(m_cfuncp);
                    لكل (ثابت V3_گراف_ڪنارو& ڪنارو : cFuncVtxp->اندريون_ڪنارا()) {
                        V3_گراف_راس* ثابت activityp = ڪنارو.fromp();
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
    فراغ گهمو(AstMemberSel* nodep) override {
        إذا (m_cfuncp && m_finding && nodep->access().isWriteOrRW()) {
            AstIfaceRefDType* ثابت dtypep
                = VN_CAST(nodep->fromp()->dtypep()->skipRefp(), IfaceRefDType);
            إذا (dtypep && dtypep->isVirtual()) {
                ثابت خودڪار it = m_ifaceMemberVscps.ڳوليو({dtypep->ifacep(), nodep->varp()->name()});
                إذا (it != m_ifaceMemberVscps.پڇاڙي()) {
                    V3_گراف_راس* ثابت funcVtxp = getCFuncVertexp(m_cfuncp);
                    لكل (AstVarScope* ثابت vscp : it->ٻيو) {
                        V3_گراف_راس* varVtxp = vscp->user1u().منزل_گراف_راس();
                        إذا (!varVtxp) {
                            varVtxp = جديد Trace_Var_راس{&m_گراف, vscp};
                            vscp->user1p(varVtxp);
                        }
                        جديد V3_گراف_ڪنارو{&m_گراف, funcVtxp, varVtxp, 1};
                    }
                }
            }
        }
        iterateChildren(nodep);
    }
    //--------------------
    فراغ گهمو(Ast_ڳنڍ* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    واضح TraceVisitor(AstNetlist* nodep)
        : m_alwaysVtxp{جديد Trace_Activity_راس{&m_گراف, Trace_Activity_راس::ACTIVITY_ALWAYS}} {
        nodep->user2_Clear_وڻ();  // TraceDecl multiple activities flag
        nodep->user3_Clear_وڻ();  // TraceDecl TraceActivityVertex (assumes we start at nullptr)
        iterate(nodep);
        nodep->foreach([](AstTraceDecl* ثابت declp) {
            إذا (declp->اندريون_Dtype_Func()) {
                declp->valuep()->unlink_Fr_پٺ()->delete_وڻ();
                declp->valuep(خالي_اشارو);
            }
        });
    }
    ~TraceVisitor() override {
        V3Stats::جوڙيو_Stat("Tracing, Activity setters", m_statSetters);
        V3Stats::جوڙيو_Stat("Tracing, Activity slow blocks", m_statSettersSlow);
        V3Stats::جوڙيو_Stat("Tracing, Unique trace codes", m_ڪوڊ);
        V3Stats::جوڙيو_Stat("Tracing, Unique traced signals", m_statUniqSigs);
    }
};

//######################################################################
// Trace class functions

فراغ V3Trace::traceAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { TraceVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_وڻ("trace", 0, dump_وڻ_Either_Level() >= 3);
}
