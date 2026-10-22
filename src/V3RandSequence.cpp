// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Transform randsequence statements
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
// V3Randomize's Transformations:
//
// Convert each RandSequence production into a production task, and hookup.
//      PROD({x}) -> TASK(__Vrs_{x}, ARG(__VrsBreak), arguments)
//                      JUMPBLOCK
//
// Every variable referenced in the RandSequence becomes an ARG passed
// around. (Alternative would be make a VARXREF's but the needed variable may
// be on the stack, subject to recursion, or otherwise.)
//
// A production becomes a conditional execution of the PRODLIST below
//   for a single production list
//      RSPROD(RSRULE(weight, stmt))
//        -> IF(weight != 0, stmt)
//   or RSPROD(RSRULE(weight1, stmt!), RSRULE(weight2, stmt2))
//        -> RANDCASE(CASEITEM(weight1, stmt1), CASEITEM(weight2, stmt2))
//
// Return becomes a jump to end of production task
//      RSRETURN -> JUMPBLOCK(..., JUMPGO)
//
// Break becomes a jump to end of production task, and sets a variable
// that the production call will check
//      RSBREAK -> ASSIGH(__VrsBreak, 1)
//                 JUMPBLOCK(..., JUMPGO)
//
// Production calls become a function call
//      call({x}) -> TASKCALL(__Vrs_{x}, ARG(__VrsBreak), arguments)
//                   IF(__VrsBreak) JUMPGO-break
//
// Rand join maps to a large complicated function, described in comments below
//
// If, Case, and Repeat map to normal non-randsequence nodes
//
//*************************************************************************

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "verilatedos.h"

#ضمن "V3RandSequence.h"

#ضمن "V3Ast.h"
#ضمن "V3Error.h"
#ضمن "V3FileLine.h"
#ضمن "V3Global.h"

VL_DEFINE_DEBUG_FUNCTIONS;

// ######################################################################
//  Matcher classes (for suggestion matching)

صنف RSProdMatcher final : عام VNodeMatcher {
عام:
    منطقي غوټه_سمون(ثابت Ast_غوټه* nodep) ثابت override { أرجع VN_IS(nodep, RSProd); }
};

//######################################################################
// Visitor that marks classes needing a randomize() method

صنف RandSequenceVisitor final : عام VNVisitor {
    // STATE - global
    std::map<AstRSProd*, Ast_غوټه_FTask*> m_prodFuncps;  // Generated production functions

    // STATE - for current visit position (use VL_RESTORER)
    Ast_غوټه_Module* m_modp = تش_اشاره;  // Current module
    AstRandSequence* m_rsp = تش_اشاره;  // Current randsequence
    AstRSProd* m_پیل_Prodp = تش_اشاره;  // Starting production
    Ast_غوټه_FTask* m_prodFuncp = تش_اشاره;  // Production function being built
    Ast_Jump_بلاک* m_jumpBlockp = تش_اشاره;  // Building jumpblock for return/break
    AstVar* m_breakVarp = تش_اشاره;  // Break variable
    std::unordered_سېټ<AstVar*> m_localizes;  // Variables to become function inouts
    std::map<std::string, AstVar*> m_localize_نومونه;  // Ordered names of function inouts
    std::unordered_map<ثابت AstVar*, AstVar*> m_localizeRemaps;  // New ports for old vars

    // METHODS
    فراغ پیدا_کړه_Localizes(AstRandSequence* nodep) {
        std::سېټ<AstVar*> localVars;
        nodep->foreach([&](Ast_غوټه* ثابت nodep) {
            إذا (ثابت AstVarRef* ثابت anodep = VN_CAST(nodep, VarRef)) {
                m_localizes.emplace(anodep->varp());
            } وإلا إذا (AstVar* ثابت anodep = VN_CAST(nodep, Var)) {
                localVars.emplace(anodep);
            }
        });
        // Clear every variable allocated in the RandSequence
        لكل (AstVar* varp : localVars) m_localizes.erase(varp);
        // Sort by name, so function arguments are stable
        لكل (AstVar* varp : m_localizes) {
            UINFO(9, "localize " << varp);
            m_localize_نومونه.emplace(varp->name(), varp);
        }
    }

    AstVar* newBreakVar(File_کرښه* fl, منطقي asOutput) {
        AstVar* ثابت varp = جديد AstVar{fl, VVarType::پورټ, "__VrsBreak", VFlag_بټ_Packed{}, 1};
        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        varp->funcLocal(ریښتیا);
        إذا (asOutput) varp->direction(VDirection::OUTPUT);
        Ast_غوټه* outp = varp;
        outp->جمع_کړه_بل(جديد Ast_ټاکل{fl, جديد AstVarRef{fl, varp, VAccess::WRITE},
                                    جديد AstConst{fl, AstConst::بټ_False{}}});

        // Also add arguments as next's
        لكل (ثابت خپلکاری& itr : m_localize_نومونه) {
            ثابت AstVar* ثابت lvarp = itr.دوهم;
            AstVar* ثابت iovarp
                = جديد AstVar{fl, VVarType::پورټ, "__Vrsarg_" + lvarp->name(), lvarp};
            iovarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            iovarp->funcLocal(ریښتیا);
            iovarp->direction(VDirection::REF);
            varp->جمع_کړه_بل(iovarp);
            m_localizeRemaps.emplace(lvarp, iovarp);
        }
        أرجع varp;
    }

    AstTask* new_پیل_Func(AstRandSequence* nodep) {
        // Create wrapper that is called by the sequence
        AstTask* ثابت taskp = جديد AstTask{m_پیل_Prodp->fileline(), m_rsp->name(), تش_اشاره};
        taskp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        m_modp->جمع_کړه_Stmtsp(taskp);

        // Create local (not output) break variable
        VL_RESTORER_COPY(m_localizeRemaps);
        AstVar* breakVarp = newBreakVar(nodep->fileline(), خطأ);
        taskp->جمع_کړه_Stmtsp(breakVarp);

        // Call the start production's task
        taskp->جمع_کړه_Stmtsp(newProdFuncRef(nodep, m_پیل_Prodp, breakVarp, تش_اشاره));

        UINFOTREE(9, taskp, "newStart", "");
        أرجع taskp;
    }

    Ast_غوټه* newProdFuncRef(Ast_غوټه* nodep, AstRSProd* prodp, AstVar* breakVarp,
                            AstArg* userArgsp) {
        خپلکاری it = m_prodFuncps.پیدا_کړه(prodp);
        UASSERT_OBJ(it != m_prodFuncps.پای_ټکی(), nodep, "No production function made");
        Ast_غوټه_FTask* ثابت prodFuncp = it->دوهم;
        File_کرښه* ثابت fl = nodep->fileline();
        // V3Width already ran before V3RandSequence, so VarRefs we create here
        // need dtype set explicitly, V3Broken later checks width == widthMin.
        AstVarRef* ثابت breakRefp = جديد AstVarRef{fl, breakVarp, VAccess::WRITE};
        breakRefp->dtypeFrom(breakVarp);
        AstArg* ثابت argsp = جديد AstArg{fl, breakVarp->name(), breakRefp};
        لكل (ثابت خپلکاری& itr : m_localize_نومونه) {
            ثابت AstVar* ثابت lvarp = itr.دوهم;
            AstVar* ثابت iovarp = m_localizeRemaps[lvarp];
            UASSERT_OBJ(iovarp, nodep, "No new port variable for local variable" << lvarp);
            AstVarRef* ثابت refp = جديد AstVarRef{fl, iovarp, VAccess::READWRITE};
            refp->dtypeFrom(iovarp);
            argsp->جمع_کړه_بل(جديد AstArg{nodep->fileline(), "__Vrsarg_" + lvarp->name(), refp});
        }
        إذا (userArgsp) argsp->جمع_کړه_بل(userArgsp);
        Ast_غوټه* ثابت newp
            = جديد Ast_Stmt_بیان{fl, جديد AstTaskRef{fl, VN_AS(prodFuncp, Task), argsp}};
        أرجع newp;
    }

    فراغ newProdFunc(AstRSProd* nodep) {
        // Task, as time may pass
        AstTask* ثابت newp
            = جديد AstTask{nodep->fileline(), m_rsp->name() + "_" + nodep->name(), تش_اشاره};
        m_modp->جمع_کړه_Stmtsp(newp);
        m_prodFuncps.emplace(nodep, newp);
        UINFOTREE(9, newp, "newProd", "");
    }

    Ast_غوټه* newProdRandJoin(AstRSProd* /*prodp*/, AstRSProdList* prodlistp) {
        // For weight == 1.0 longer sequence (favor stay in a)
        // For weight == 0.0 shorter squence (favor change a/b)
        UASSERT_OBJ(prodlistp->weightp(), prodlistp, "Weight should have default CONST(0.5)");
        Ast_غوټه_بیان* weightp = prodlistp->weightp()->unlink_Fr_شا();

        // Build map of production lists and productions we need to join
        // Must mainain the relative order of each sequence.
        std::deque<Ast_RSProd_توکی*> lists;
        std::unordered_map<Ast_RSProd_توکی*, std::deque<Ast_غوټه_Stmt*>> listStmts;
        لكل (Ast_RSProd_توکی* proditemp = VN_AS(prodlistp->prodsp(), RSProd_توکی); proditemp;
             proditemp = VN_AS(proditemp->nextp(), RSProd_توکی)) {
            lists.ټېل_شا(proditemp);
            ثابت AstRSProd* ثابت subProdp = proditemp->prodp();
            إذا (!subProdp) استمر;
            إذا (!subProdp->rulesp()) استمر;
            إذا (!subProdp->rulesp()->prodlistsp()) استمر;
            لكل (Ast_غوټه_Stmt* subStmtp
                 = VN_AS(subProdp->rulesp()->prodlistsp()->prodsp(), غوټه_Stmt);
                 subStmtp; subStmtp = VN_AS(subStmtp->nextp(), غوټه_Stmt)) {
                listStmts[proditemp].ټېل_شا(subStmtp);
            }
        }

        UINFO(9, "RandJoin productions called:");
        لكل (Ast_RSProd_توکی* proditemp : lists) {
            UINFO(9, "  list " << proditemp);
            لكل (ثابت Ast_غوټه_Stmt* prodip : listStmts[proditemp])
                UINFO(9, "    calls " << prodip);
        }

        // Need to clone all nodes used

        File_کرښه* fl = prodlistp->fileline();
        Ast_غوټه* newp = تش_اشاره;

        // "bool _Vjoin_repick = true;"  // Pick a new longer sequence
        AstVar* repickVarp = جديد AstVar{fl, VVarType::VAR, "_Vjoin_repick", VFlag_بټ_Packed{}, 1};
        repickVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        repickVarp->funcLocal(ریښتیا);
        m_breakVarp->جمع_کړه_بل_دلته(repickVarp);  // Add to function top, not newp
        newp = Ast_غوټه::جمع_کړه_بل(newp,
                                جديد Ast_ټاکل{fl, جديد AstVarRef{fl, repickVarp, VAccess::WRITE},
                                              جديد AstConst{fl, AstConst::بټ_True{}}});

        // "int _Vjoin_nleft_0 = N(items);"  // N(a) Number left to generate in list 0, starts at
        // N(list0)
        std::deque<AstVar*> nleftVarps;
        صحيح i = 0;
        لكل (Ast_RSProd_توکی* proditemp : lists) {
            // 'rand join arule arule arule' is legal, so need numbered, not non-unique named
            // nleft's
            AstVar* ثابت varp = جديد AstVar{
                fl, VVarType::VAR, "_Vjoin_nleft_" + std::منزل_string(i++), VFlag_بټ_Packed{}, 32};
            varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            varp->funcLocal(ریښتیا);
            m_breakVarp->جمع_کړه_بل_دلته(varp);  // Add to function top, not newp
            nleftVarps.ټېل_شا(varp);

            newp = Ast_غوټه::جمع_کړه_بل(
                newp,
                جديد Ast_ټاکل{fl, جديد AstVarRef{fl, varp, VAccess::WRITE},
                              جديد AstConst{fl, AstConst::Widthed_ارزښت{}, 32,
                                           static_cast<uint32_t>(listStmts[proditemp].اندازه())}});
        }

        // "int _Vjoin_picked_elist = 0;"  // 0 = pick first eligible (non-finished) list, 1 = pick
        // second, etc
        AstVar* ثابت pickedVarp
            = جديد AstVar{fl, VVarType::VAR, "_Vjoin_picked_elist", VFlag_بټ_Packed{}, 1};
        pickedVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        pickedVarp->funcLocal(ریښتیا);
        m_breakVarp->جمع_کړه_بل_دلته(pickedVarp);  // Add to function top, not newp

        // "while(1) {"  // Generate until make complete sequence
        Ast_Jump_بلاک* ثابت whilep = جديد Ast_Jump_بلاک{fl, تش_اشاره};
        newp = Ast_غوټه::جمع_کړه_بل(newp, جديد AstLoop{fl, whilep});
        {
            // "if ((std::rand() & 0xffffUL) > (0xffff * weight)) _Vjoin_repick = true;"
            whilep->جمع_کړه_Stmtsp(جديد AstIf{
                fl,
                جديد AstGt{
                    fl, جديد AstRand{fl, AstRand::SRandomU32{}},
                    جديد AstRToIS{fl, جديد AstMulD{fl,
                                                 جديد AstConst{fl, AstConst::RealDouble{},
                                                              static_cast<مزدوج>(0xffffffffUL)},
                                                 weightp}}},
                جديد Ast_ټاکل{fl, جديد AstVarRef{fl, repickVarp, VAccess::WRITE},
                              جديد AstConst{fl, AstConst::بټ_True{}}}});

            // "if (_Vjoin_repick) {"
            AstIf* ifp = جديد AstIf{fl, جديد AstVarRef{fl, repickVarp, VAccess::READ}};
            whilep->جمع_کړه_Stmtsp(ifp);
            {
                // "// _Vjoin_nlists_eligible is how many lists eligable for picking"
                // "int _Vjoin_nlists_eligible = 0;"
                AstVar* ثابت eligibleVarp = جديد AstVar{
                    fl, VVarType::VAR, "_Vjoin_nlists_eligible", VFlag_بټ_Packed{}, 32};
                eligibleVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                eligibleVarp->funcLocal(ریښتیا);
                m_breakVarp->جمع_کړه_بل_دلته(eligibleVarp);  // Add to function top, not newp
                ifp->جمع_کړه_Thensp(جديد Ast_ټاکل{fl, جديد AstVarRef{fl, eligibleVarp, VAccess::WRITE},
                                             جديد AstConst{fl, AstConst::Widthed_ارزښت{}, 32, 0}});

                i = 0;
                لكل (Ast_RSProd_توکی* proditemp : lists) {
                    (فراغ)proditemp;
                    // "_Vjoin_nlists_eligible += _Vjoin_nleft_0 ? 1 : 0;"
                    ifp->جمع_کړه_Thensp(جديد Ast_ټاکل{
                        fl, جديد AstVarRef{fl, eligibleVarp, VAccess::WRITE},
                        جديد Ast_جمع_کړه{
                            fl, جديد AstVarRef{fl, eligibleVarp, VAccess::READ},
                            جديد AstCond{
                                fl,
                                جديد AstNeq{fl, جديد AstConst{fl, AstConst::Widthed_ارزښت{}, 32, 0},
                                           جديد AstVarRef{fl, nleftVarps[i], VAccess::READ}},
                                جديد AstConst{fl, AstConst::Widthed_ارزښت{}, 32, 1},
                                جديد AstConst{fl, AstConst::Widthed_ارزښت{}, 32, 0}}}});
                    ++i;
                }

                // "if (!_Vjoin_nlists_eligible) break;"  // Out of elements
                ifp->جمع_کړه_Thensp(
                    جديد AstIf{fl,
                              جديد AstEq{fl, جديد AstConst{fl, AstConst::Widthed_ارزښت{}, 32, 0},
                                        جديد AstVarRef{fl, eligibleVarp, VAccess::READ}},
                              جديد AstJumpGo{fl, m_jumpBlockp}});

                // "// +1 to simplify usage, so 0 = done"
                // "_Vjoin_picked_elist = 1 + (std::rand() % _Vjoin_nlists_eligible);"
                ifp->جمع_کړه_Thensp(جديد Ast_ټاکل{
                    fl, جديد AstVarRef{fl, pickedVarp, VAccess::WRITE},
                    جديد Ast_جمع_کړه{fl, جديد AstConst{fl, AstConst::Widthed_ارزښت{}, 32, 1},
                               جديد AstModDiv{fl, جديد AstRand{fl, AstRand::SRandomU32{}},
                                             جديد AstVarRef{fl, eligibleVarp, VAccess::READ}}}});

                // "_Vjoin_repick = false;
                ifp->جمع_کړه_Thensp(جديد Ast_ټاکل{fl, جديد AstVarRef{fl, repickVarp, VAccess::WRITE},
                                             جديد AstConst{fl, AstConst::بټ_False{}}});
            }

            // "// Number of eligible lists left before hit _Vjoin_picked one"
            // "int _Vjoin_sel_elist = _Vjoin_picked_elist;"
            AstVar* ثابت selVarp
                = جديد AstVar{fl, VVarType::VAR, "_Vjoin_sel_elist", VFlag_بټ_Packed{}, 32};
            selVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            selVarp->funcLocal(ریښتیا);
            m_breakVarp->جمع_کړه_بل_دلته(selVarp);  // Add to function top, not newp
            whilep->جمع_کړه_Stmtsp(جديد Ast_ټاکل{fl, جديد AstVarRef{fl, selVarp, VAccess::WRITE},
                                            جديد AstVarRef{fl, pickedVarp, VAccess::READ}});

            i = -1;
            لكل (Ast_RSProd_توکی* proditemp : lists) {
                ++i;
                // "if (_Vjoin_nleft_0) {"
                ifp = جديد AstIf{fl,
                                جديد AstNeq{fl, جديد AstConst{fl, AstConst::Widthed_ارزښت{}, 32, 0},
                                           جديد AstVarRef{fl, nleftVarps[i], VAccess::READ}},
                                تش_اشاره, تش_اشاره};
                whilep->جمع_کړه_Stmtsp(ifp);
                {
                    // "--_Vjoin_sel_elist;"
                    ifp->جمع_کړه_Thensp(جديد Ast_ټاکل{
                        fl, جديد AstVarRef{fl, selVarp, VAccess::WRITE},
                        جديد AstSub{fl, جديد AstVarRef{fl, selVarp, VAccess::READ},
                                   جديد AstConst{fl, AstConst::Widthed_ارزښت{}, 32, 1}}});

                    // "if (_Vjoin_sel_elist == 0) {"
                    AstIf* ثابت jIfp = جديد AstIf{
                        fl,
                        جديد AstEq{fl, جديد AstConst{fl, AstConst::Widthed_ارزښت{}, 32, 0},
                                  جديد AstVarRef{fl, selVarp, VAccess::READ}},
                        تش_اشاره, تش_اشاره};
                    ifp->جمع_کړه_Thensp(jIfp);
                    {
                        // "switch (_Vjoin_nleft_0) {"
                        //     "case 2 / * N(a)  * /: {statement}; break;"
                        //     "case 1 / * N(a) - 1 * /: {statement}; break;"
                        uint32_t j = static_cast<uint32_t>(listStmts[proditemp].اندازه());
                        لكل (Ast_غوټه_Stmt* iprodp : listStmts[proditemp]) {
                            jIfp->جمع_کړه_Thensp(جديد AstIf{
                                fl,
                                جديد AstEq{fl, جديد AstConst{fl, AstConst::Widthed_ارزښت{}, 32, j},
                                          جديد AstVarRef{fl, nleftVarps[i], VAccess::READ}},
                                iprodp->clone_ونه(خطأ)});
                            --j;
                        }

                        // "--_Vjoin_nleft_0;"
                        jIfp->جمع_کړه_Thensp(جديد Ast_ټاکل{
                            fl, جديد AstVarRef{fl, nleftVarps[i], VAccess::WRITE},
                            جديد AstSub{fl, جديد AstVarRef{fl, nleftVarps[i], VAccess::READ},
                                       جديد AstConst{fl, AstConst::Widthed_ارزښت{}, 32, 1}}});

                        // "if (0 == _Vjoin_nleft_0) _Vjoin_repick = true;"
                        jIfp->جمع_کړه_Thensp(جديد AstIf{
                            fl,
                            جديد AstEq{fl, جديد AstConst{fl, AstConst::Widthed_ارزښت{}, 32, 0},
                                      جديد AstVarRef{fl, nleftVarps[i], VAccess::READ}},
                            جديد Ast_ټاکل{fl, جديد AstVarRef{fl, repickVarp, VAccess::WRITE},
                                          جديد AstConst{fl, AstConst::بټ_True{}}}});

                        // "continue;"
                        jIfp->جمع_کړه_Thensp(جديد AstJumpGo{fl, whilep});
                    }
                }
            }
        }

        UINFOTREE(9, newp, "ForkJoin new", "-");
        أرجع newp;
    }

    // VISITORS

    فراغ لیدنه(Ast_غوټه_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildren(nodep);
    }

    فراغ لیدنه(AstRandSequence* nodep) override {
        UINFO(9, "visit " << nodep);
        UASSERT_OBJ(m_modp, nodep, "randsequence not under module");
        إذا (m_rsp) {
            // No examples found in the wild
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: randsequence under randsequence");
            VL_DO_DANGLING(ټېل_Deletep(nodep->unlink_Fr_شا()), nodep);
            أرجع;
        }

        UINFOTREE(9, nodep, "rsIn", "");
        VL_RESTORER(m_rsp);
        m_rsp = nodep;
        VL_RESTORER(m_پیل_Prodp);

        VL_RESTORER_COPY(m_localizes);
        VL_RESTORER_COPY(m_localize_نومونه);
        VL_RESTORER_COPY(m_localizeRemaps);
        پیدا_کړه_Localizes(nodep);

        // Find first production
        m_پیل_Prodp = nodep->prodsp();
        إذا (nodep->prodp()) m_پیل_Prodp = nodep->prodp();  // Has a start production selected

        // Build production functions
        لكل (AstRSProd* prodp = nodep->prodsp(); prodp; prodp = VN_AS(prodp->nextp(), RSProd)) {
            newProdFunc(prodp);
            // Don't iterate yet, need to have tasks we can reference
        }

        // Iterate all of the new productions, moving guts into functions just made
        // Must do those with 'rand join' first, as they need to see the raw AstRS* nodes
        // before they are removed.  (Alternative would be to keep them until the end).
        UINFOTREE(9, nodep, "RS Tree pre-it", "-");
        std::unordered_سېټ<AstRSProd*> prodHasRandJoin;
        لكل (AstRSProd* prodp = nodep->prodsp(); prodp; prodp = VN_AS(prodp->nextp(), RSProd)) {
            prodp->foreach([&](ثابت AstRSProdList* ثابت prodlistp) {
                إذا (prodlistp->randJoin()) prodHasRandJoin.emplace(prodp);
            });
        }
        لكل (AstRSProd *nextp, *prodp = nodep->prodsp(); prodp; prodp = nextp) {
            nextp = VN_AS(prodp->nextp(), RSProd);
            إذا (prodHasRandJoin.شمېر(prodp)) VL_DO_DANGLING(iterate(prodp), prodp);
        }
        لكل (AstRSProd *nextp, *prodp = nodep->prodsp(); prodp; prodp = nextp) {
            nextp = VN_AS(prodp->nextp(), RSProd);
            إذا (!prodHasRandJoin.شمېر(prodp)) VL_DO_DANGLING(iterate(prodp), prodp);
        }
        UINFOTREE(9, nodep, "RS Tree post-it", "-");

        // Replace randsequence with call to randsequence start function
        AstTask* ثابت پیل_Funcp = new_پیل_Func(nodep);
        AstArg* argsp = تش_اشاره;
        لكل (خپلکاری& itr : m_localize_نومونه) {
            AstVar* ثابت lvarp = itr.دوهم;
            argsp = Ast_غوټه::جمع_کړه_بل(
                argsp, جديد AstArg{nodep->fileline(), "__Vrsarg_" + lvarp->name(),
                                  جديد AstVarRef{nodep->fileline(), lvarp, VAccess::READWRITE}});
        }
        AstTaskRef* ثابت callStartp
            = جديد AstTaskRef{nodep->fileline(), پیل_Funcp->name(), argsp};
        callStartp->taskp(پیل_Funcp);
        nodep->replaceWith(جديد Ast_Stmt_بیان{nodep->fileline(), callStartp});

        VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
    }
    فراغ لیدنه(AstRSProd* nodep) override {
        // Production, immediately under a randsequence
        UINFO(9, "visit " << nodep);
        UINFOTREE(9, nodep, "visit " << nodep, "-");
        UASSERT_OBJ(!m_prodFuncp, nodep, "recursive production definition?");
        VL_RESTORER(m_prodFuncp);
        خپلکاری it = m_prodFuncps.پیدا_کړه(nodep);
        UASSERT_OBJ(it != m_prodFuncps.پای_ټکی(), nodep, "No production function made");
        m_prodFuncp = it->دوهم;

        // Create a break variable
        // TODO we could do this only if break exists in the downstream production,
        // but as-is we'll optimize it away in most cases anyways
        VL_RESTORER(m_breakVarp);
        VL_RESTORER_COPY(m_localizeRemaps);
        m_breakVarp = newBreakVar(nodep->fileline(), ریښتیا);
        m_prodFuncp->جمع_کړه_Stmtsp(m_breakVarp);

        // Production formal ports become real input ports on the generated task.
        // Must be added after m_breakVarp / localize ports so positional argument
        // order in newProdFuncRef stays consistent: [break, localize..., user...].
        إذا (Ast_غوټه* ثابت portsp = nodep->portsp()) {
            portsp->unlink_Fr_شا_With_بل();
            m_prodFuncp->جمع_کړه_Stmtsp(portsp);
            لكل (Ast_غوټه* itp = portsp; itp; itp = itp->nextp()) {
                AstVar* ثابت پورټ_Varp = VN_AS(itp, Var);
                پورټ_Varp->funcLocal(ریښتیا);
                پورټ_Varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            }
        }

        // Put JumpBlock immediately under the new function to support
        // a future break/return.  V3Const will rip it out if unneeded.
        VL_RESTORER(m_jumpBlockp);
        m_jumpBlockp = جديد Ast_Jump_بلاک{nodep->fileline(), تش_اشاره};
        m_prodFuncp->جمع_کړه_Stmtsp(m_jumpBlockp);

        إذا (nodep->fvarp())
            nodep->fvarp()->v3warn(E_UNSUPPORTED,
                                   "Unsupported: randsequence production function variable");

        // Move children into m_prodFuncp, and iterate there
        إذا (!nodep->rulesp()) {  // Nothing to do
        } وإلا إذا (nodep->rulesp()->prodlistsp() && nodep->rulesp()->prodlistsp()->randJoin()) {
            AstRSProdList* ثابت prodlistp = nodep->rulesp()->prodlistsp();
            Ast_غوټه* ثابت itemsp = newProdRandJoin(nodep, prodlistp);
            m_jumpBlockp->جمع_کړه_Stmtsp(itemsp);
            iterate_And_بل_Null(itemsp);
        } وإلا إذا (!nodep->rulesp()->nextp()) {  // Single rule/list, can just do it
            // RSPROD(RSRULE(weight, stmt)) -> IF(weight != 0, stmt)
            ثابت AstRSRule* ثابت rulep = nodep->rulesp();
            Ast_غوټه* itemsp = تش_اشاره;
            إذا (rulep->وزن_Stmtsp()) itemsp = rulep->وزن_Stmtsp()->unlink_Fr_شا_With_بل();
            إذا (rulep->prodlistsp())
                itemsp = Ast_غوټه::جمع_کړه_بل(itemsp, rulep->prodlistsp()->unlink_Fr_شا_With_بل());
            // Can ignore rulep->weightp() unless is zero, in which case noop, add if
            إذا (rulep->weightp())
                itemsp = جديد AstIf{rulep->weightp()->fileline(),
                                   جديد AstNeq{rulep->weightp()->fileline(),
                                              جديد AstConst{rulep->weightp()->fileline(), 0},
                                              rulep->weightp()->unlink_Fr_شا()},
                                   itemsp};
            // Move prodlist to parent (m_prodFunc) and process
            إذا (itemsp) {
                UINFOTREE(9, itemsp, "additems", "-");
                m_jumpBlockp->جمع_کړه_Stmtsp(itemsp);
                iterate_And_بل_Null(itemsp);
                UINFOTREE(9, m_jumpBlockp, "jumpBlockNow", "-");
            }
        } وإلا {
            // List of rules with "OR", need to random-weight them
            // RSPROD(RSRULE(weight1, stmt!), RSRULE(weight2, stmt2))
            //   -> RANDCASE(CASEITEM(weight1, stmt1), CASEITEM(weight2, stmt2))
            AstRandCase* ثابت randcasep = جديد AstRandCase{nodep->fileline(), تش_اشاره};
            لكل (AstRSRule* rulep = nodep->rulesp(); rulep;
                 rulep = VN_AS(rulep->nextp(), RSRule)) {
                Ast_غوټه* itemsp = تش_اشاره;
                إذا (rulep->وزن_Stmtsp()) itemsp = rulep->وزن_Stmtsp()->unlink_Fr_شا_With_بل();
                إذا (rulep->prodlistsp())
                    itemsp = Ast_غوټه::جمع_کړه_بل(itemsp, rulep->prodlistsp()->unlink_Fr_شا_With_بل());
                إذا (itemsp) {
                    Ast_غوټه_بیان* ثابت weightp = rulep->weightp()
                                                     ? rulep->weightp()->unlink_Fr_شا()
                                                     : جديد AstConst{rulep->fileline(), 1};
                    randcasep->جمع_کړه_Itemsp(جديد Ast_Case_توکی{rulep->fileline(), weightp, itemsp});
                }
            }
            m_jumpBlockp->جمع_کړه_Stmtsp(randcasep);
            iterate_And_بل_Null(randcasep);
        }

        // V3Task needs to know if we ended up making a recursive function
        m_prodFuncp->foreach([&](Ast_غوټه_FTask_Ref* ثابت nodep) {
            إذا (nodep->taskp() == m_prodFuncp) m_prodFuncp->recursive(ریښتیا);
        });
        UINFOTREE(9, m_prodFuncp, "rsprod-task-done " << nodep, "-");

        // Done with production, should have moved everything to new task
        VL_DO_DANGLING(ټېل_Deletep(nodep->unlink_Fr_شا()), nodep);
    }
    فراغ لیدنه(AstRSRule* nodep) override {
        // Rule of what to produce (as a list) under a production
        nodep->v3fatalSrc("randsequence rule should be iterated and deleted by AstRSProd");
    }
    فراغ لیدنه(AstRSProdList* nodep) override {
        // List of productions to call, potentially with a weight
        UINFO(9, "visit " << nodep);
        UASSERT_OBJ(m_prodFuncp, nodep, "RSProdList not under production");

        // Move prodlist to parent (m_prodFunc) and process
        إذا (Ast_غوټه* ثابت itemsp = nodep->prodsp()) {
            UASSERT_OBJ(!nodep->randJoin(), nodep,
                        "rand join should have been handled in visit(RSRule)");
            nodep->replaceWith(itemsp->unlink_Fr_شا_With_بل());
            // Will soon iterate itemsp as part of normal visit process
            // These will become sequential in the generated task
        } وإلا {
            nodep->unlink_Fr_شا();
        }
        VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
    }
    فراغ لیدنه(Ast_RSProd_توکی* nodep) override {
        // "call" to generate using another production
        UINFO(9, "visit " << nodep);
        UASSERT_OBJ(m_prodFuncp, nodep, "RSProdItem not under production");
        AstRSProd* ثابت foundp = nodep->prodp();
        UASSERT_OBJ(foundp, nodep, "Unlinked production reference");
        // Pass through caller-side argument list (IEEE 1800-2023 18.17.7).
        AstArg* ثابت userArgsp
            = nodep->argsp() ? nodep->argsp()->unlink_Fr_شا_With_بل() : تش_اشاره;
        // Convert to task call
        Ast_غوټه* ثابت newp = newProdFuncRef(nodep, foundp, m_breakVarp, userArgsp);
        // The production might have done a "break;", skip other steps if so
        newp->جمع_کړه_بل(جديد AstIf{nodep->fileline(),
                                جديد AstVarRef{nodep->fileline(), m_breakVarp, VAccess::READ},
                                جديد AstJumpGo{nodep->fileline(), m_jumpBlockp}});

        nodep->replaceWith(newp);
        VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
    }

    فراغ لیدنه(AstRSBreak* nodep) override {
        UASSERT_OBJ(m_jumpBlockp, nodep, "RSBreak not under production jump block");
        UASSERT_OBJ(m_breakVarp, nodep, "no break variable");
        Ast_غوټه_Stmt* ثابت newp = جديد Ast_ټاکل{
            nodep->fileline(), جديد AstVarRef{nodep->fileline(), m_breakVarp, VAccess::WRITE},
            جديد AstConst{nodep->fileline(), AstConst::بټ_True{}}};
        newp->جمع_کړه_بل(جديد AstJumpGo{nodep->fileline(), m_jumpBlockp});
        nodep->replaceWith(newp);
        VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
    }
    فراغ لیدنه(AstRSReturn* nodep) override {
        UASSERT_OBJ(m_jumpBlockp, nodep, "RSReturn not under production jump block");
        nodep->replaceWith(جديد AstJumpGo{nodep->fileline(), m_jumpBlockp});
        VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
    }
    فراغ لیدنه(AstVarRef* nodep) override {
        // Change references to variables inside randsequence to references to
        // ref variables on the new task
        ثابت خپلکاری it = m_localizeRemaps.پیدا_کړه(nodep->varp());
        إذا (it != m_localizeRemaps.پای_ټکی()) {
            AstVar* ثابت iovarp = it->دوهم;
            UINFO(9, "VARREF remap " << nodep << " -> " << iovarp);
            nodep->varp(iovarp);
        }
    }

    فراغ لیدنه(Ast_غوټه* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    څرګند RandSequenceVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~RandSequenceVisitor() override = افتراضي;
};

//######################################################################
// RandSequence method class functions

فراغ V3RandSequence::randSequenceNetlist(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { ثابت RandSequenceVisitor randSequenceVisitor{nodep}; }
    V3Global::dump_Check_Global_ونه("randsequence", 0, dump_ونه_Either_Level() >= 3);
}
