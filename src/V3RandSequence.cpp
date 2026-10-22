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

#شامل "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#شامل "verilatedos.h"

#شامل "V3RandSequence.h"

#شامل "V3Ast.h"
#شامل "V3Error.h"
#شامل "V3FileLine.h"
#شامل "V3Global.h"

VL_DEFINE_DEBUG_FUNCTIONS;

// ######################################################################
//  Matcher classes (for suggestion matching)

جماعت RSProdMatcher final : عوامی VNodeMatcher {
عوامی:
    منطقی گرہ_مطابقت(ثابت Ast_گرہ* nodep) ثابت override { واپس VN_IS(nodep, RSProd); }
};

//######################################################################
// Visitor that marks classes needing a randomize() method

جماعت RandSequenceVisitor final : عوامی VNVisitor {
    // STATE - global
    std::map<AstRSProd*, Ast_گرہ_FTask*> m_prodFuncps;  // Generated production functions

    // STATE - for current visit position (use VL_RESTORER)
    Ast_گرہ_Module* m_modp = خالی_اشاریہ;  // Current module
    AstRandSequence* m_rsp = خالی_اشاریہ;  // Current randsequence
    AstRSProd* m_آغاز_Prodp = خالی_اشاریہ;  // Starting production
    Ast_گرہ_FTask* m_prodFuncp = خالی_اشاریہ;  // Production function being built
    Ast_Jump_بلاک* m_jumpBlockp = خالی_اشاریہ;  // Building jumpblock for return/break
    AstVar* m_breakVarp = خالی_اشاریہ;  // Break variable
    std::unordered_سیٹ<AstVar*> m_localizes;  // Variables to become function inouts
    std::map<std::string, AstVar*> m_localize_نام;  // Ordered names of function inouts
    std::unordered_map<ثابت AstVar*, AstVar*> m_localizeRemaps;  // New ports for old vars

    // METHODS
    خالی ڈھونڈو_Localizes(AstRandSequence* nodep) {
        std::سیٹ<AstVar*> localVars;
        nodep->foreach([&](Ast_گرہ* ثابت nodep) {
            اگر (ثابت AstVarRef* ثابت anodep = VN_CAST(nodep, VarRef)) {
                m_localizes.emplace(anodep->varp());
            } ورنہ اگر (AstVar* ثابت anodep = VN_CAST(nodep, Var)) {
                localVars.emplace(anodep);
            }
        });
        // Clear every variable allocated in the RandSequence
        برائے (AstVar* varp : localVars) m_localizes.erase(varp);
        // Sort by name, so function arguments are stable
        برائے (AstVar* varp : m_localizes) {
            UINFO(9, "localize " << varp);
            m_localize_نام.emplace(varp->name(), varp);
        }
    }

    AstVar* newBreakVar(File_سطر* fl, منطقی asOutput) {
        AstVar* ثابت varp = نیا AstVar{fl, VVarType::پورٹ, "__VrsBreak", VFlag_ثنائی_ہندسہ_Packed{}, 1};
        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        varp->funcLocal(سچ);
        اگر (asOutput) varp->direction(VDirection::OUTPUT);
        Ast_گرہ* outp = varp;
        outp->جوڑو_اگلا(نیا Ast_تفویض{fl, نیا AstVarRef{fl, varp, VAccess::WRITE},
                                    نیا AstConst{fl, AstConst::ثنائی_ہندسہ_False{}}});

        // Also add arguments as next's
        برائے (ثابت خودکار& itr : m_localize_نام) {
            ثابت AstVar* ثابت lvarp = itr.دوسرا;
            AstVar* ثابت iovarp
                = نیا AstVar{fl, VVarType::پورٹ, "__Vrsarg_" + lvarp->name(), lvarp};
            iovarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            iovarp->funcLocal(سچ);
            iovarp->direction(VDirection::REF);
            varp->جوڑو_اگلا(iovarp);
            m_localizeRemaps.emplace(lvarp, iovarp);
        }
        واپس varp;
    }

    AstTask* new_آغاز_Func(AstRandSequence* nodep) {
        // Create wrapper that is called by the sequence
        AstTask* ثابت taskp = نیا AstTask{m_آغاز_Prodp->fileline(), m_rsp->name(), خالی_اشاریہ};
        taskp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        m_modp->جوڑو_Stmtsp(taskp);

        // Create local (not output) break variable
        VL_RESTORER_COPY(m_localizeRemaps);
        AstVar* breakVarp = newBreakVar(nodep->fileline(), جھوٹ);
        taskp->جوڑو_Stmtsp(breakVarp);

        // Call the start production's task
        taskp->جوڑو_Stmtsp(newProdFuncRef(nodep, m_آغاز_Prodp, breakVarp, خالی_اشاریہ));

        UINFOTREE(9, taskp, "newStart", "");
        واپس taskp;
    }

    Ast_گرہ* newProdFuncRef(Ast_گرہ* nodep, AstRSProd* prodp, AstVar* breakVarp,
                            AstArg* userArgsp) {
        خودکار it = m_prodFuncps.ڈھونڈو(prodp);
        UASSERT_OBJ(it != m_prodFuncps.انتہا(), nodep, "No production function made");
        Ast_گرہ_FTask* ثابت prodFuncp = it->دوسرا;
        File_سطر* ثابت fl = nodep->fileline();
        // V3Width already ran before V3RandSequence, so VarRefs we create here
        // need dtype set explicitly, V3Broken later checks width == widthMin.
        AstVarRef* ثابت breakRefp = نیا AstVarRef{fl, breakVarp, VAccess::WRITE};
        breakRefp->dtypeFrom(breakVarp);
        AstArg* ثابت argsp = نیا AstArg{fl, breakVarp->name(), breakRefp};
        برائے (ثابت خودکار& itr : m_localize_نام) {
            ثابت AstVar* ثابت lvarp = itr.دوسرا;
            AstVar* ثابت iovarp = m_localizeRemaps[lvarp];
            UASSERT_OBJ(iovarp, nodep, "No new port variable for local variable" << lvarp);
            AstVarRef* ثابت refp = نیا AstVarRef{fl, iovarp, VAccess::READWRITE};
            refp->dtypeFrom(iovarp);
            argsp->جوڑو_اگلا(نیا AstArg{nodep->fileline(), "__Vrsarg_" + lvarp->name(), refp});
        }
        اگر (userArgsp) argsp->جوڑو_اگلا(userArgsp);
        Ast_گرہ* ثابت newp
            = نیا Ast_Stmt_اظہاریہ{fl, نیا AstTaskRef{fl, VN_AS(prodFuncp, Task), argsp}};
        واپس newp;
    }

    خالی newProdFunc(AstRSProd* nodep) {
        // Task, as time may pass
        AstTask* ثابت newp
            = نیا AstTask{nodep->fileline(), m_rsp->name() + "_" + nodep->name(), خالی_اشاریہ};
        m_modp->جوڑو_Stmtsp(newp);
        m_prodFuncps.emplace(nodep, newp);
        UINFOTREE(9, newp, "newProd", "");
    }

    Ast_گرہ* newProdRandJoin(AstRSProd* /*prodp*/, AstRSProdList* prodlistp) {
        // For weight == 1.0 longer sequence (favor stay in a)
        // For weight == 0.0 shorter squence (favor change a/b)
        UASSERT_OBJ(prodlistp->weightp(), prodlistp, "Weight should have default CONST(0.5)");
        Ast_گرہ_اظہاریہ* weightp = prodlistp->weightp()->unlink_Fr_عقب();

        // Build map of production lists and productions we need to join
        // Must mainain the relative order of each sequence.
        std::deque<Ast_RSProd_شے*> lists;
        std::unordered_map<Ast_RSProd_شے*, std::deque<Ast_گرہ_Stmt*>> listStmts;
        برائے (Ast_RSProd_شے* proditemp = VN_AS(prodlistp->prodsp(), RSProd_شے); proditemp;
             proditemp = VN_AS(proditemp->nextp(), RSProd_شے)) {
            lists.ڈالو_عقب(proditemp);
            ثابت AstRSProd* ثابت subProdp = proditemp->prodp();
            اگر (!subProdp) جاری;
            اگر (!subProdp->rulesp()) جاری;
            اگر (!subProdp->rulesp()->prodlistsp()) جاری;
            برائے (Ast_گرہ_Stmt* subStmtp
                 = VN_AS(subProdp->rulesp()->prodlistsp()->prodsp(), گرہ_Stmt);
                 subStmtp; subStmtp = VN_AS(subStmtp->nextp(), گرہ_Stmt)) {
                listStmts[proditemp].ڈالو_عقب(subStmtp);
            }
        }

        UINFO(9, "RandJoin productions called:");
        برائے (Ast_RSProd_شے* proditemp : lists) {
            UINFO(9, "  list " << proditemp);
            برائے (ثابت Ast_گرہ_Stmt* prodip : listStmts[proditemp])
                UINFO(9, "    calls " << prodip);
        }

        // Need to clone all nodes used

        File_سطر* fl = prodlistp->fileline();
        Ast_گرہ* newp = خالی_اشاریہ;

        // "bool _Vjoin_repick = true;"  // Pick a new longer sequence
        AstVar* repickVarp = نیا AstVar{fl, VVarType::VAR, "_Vjoin_repick", VFlag_ثنائی_ہندسہ_Packed{}, 1};
        repickVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        repickVarp->funcLocal(سچ);
        m_breakVarp->جوڑو_اگلا_یہاں(repickVarp);  // Add to function top, not newp
        newp = Ast_گرہ::جوڑو_اگلا(newp,
                                نیا Ast_تفویض{fl, نیا AstVarRef{fl, repickVarp, VAccess::WRITE},
                                              نیا AstConst{fl, AstConst::ثنائی_ہندسہ_True{}}});

        // "int _Vjoin_nleft_0 = N(items);"  // N(a) Number left to generate in list 0, starts at
        // N(list0)
        std::deque<AstVar*> nleftVarps;
        صحیح i = 0;
        برائے (Ast_RSProd_شے* proditemp : lists) {
            // 'rand join arule arule arule' is legal, so need numbered, not non-unique named
            // nleft's
            AstVar* ثابت varp = نیا AstVar{
                fl, VVarType::VAR, "_Vjoin_nleft_" + std::مقصد_string(i++), VFlag_ثنائی_ہندسہ_Packed{}, 32};
            varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            varp->funcLocal(سچ);
            m_breakVarp->جوڑو_اگلا_یہاں(varp);  // Add to function top, not newp
            nleftVarps.ڈالو_عقب(varp);

            newp = Ast_گرہ::جوڑو_اگلا(
                newp,
                نیا Ast_تفویض{fl, نیا AstVarRef{fl, varp, VAccess::WRITE},
                              نیا AstConst{fl, AstConst::Widthed_قدر{}, 32,
                                           static_cast<uint32_t>(listStmts[proditemp].سائز())}});
        }

        // "int _Vjoin_picked_elist = 0;"  // 0 = pick first eligible (non-finished) list, 1 = pick
        // second, etc
        AstVar* ثابت pickedVarp
            = نیا AstVar{fl, VVarType::VAR, "_Vjoin_picked_elist", VFlag_ثنائی_ہندسہ_Packed{}, 1};
        pickedVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        pickedVarp->funcLocal(سچ);
        m_breakVarp->جوڑو_اگلا_یہاں(pickedVarp);  // Add to function top, not newp

        // "while(1) {"  // Generate until make complete sequence
        Ast_Jump_بلاک* ثابت whilep = نیا Ast_Jump_بلاک{fl, خالی_اشاریہ};
        newp = Ast_گرہ::جوڑو_اگلا(newp, نیا AstLoop{fl, whilep});
        {
            // "if ((std::rand() & 0xffffUL) > (0xffff * weight)) _Vjoin_repick = true;"
            whilep->جوڑو_Stmtsp(نیا AstIf{
                fl,
                نیا AstGt{
                    fl, نیا AstRand{fl, AstRand::SRandomU32{}},
                    نیا AstRToIS{fl, نیا AstMulD{fl,
                                                 نیا AstConst{fl, AstConst::RealDouble{},
                                                              static_cast<دوہرا>(0xffffffffUL)},
                                                 weightp}}},
                نیا Ast_تفویض{fl, نیا AstVarRef{fl, repickVarp, VAccess::WRITE},
                              نیا AstConst{fl, AstConst::ثنائی_ہندسہ_True{}}}});

            // "if (_Vjoin_repick) {"
            AstIf* ifp = نیا AstIf{fl, نیا AstVarRef{fl, repickVarp, VAccess::READ}};
            whilep->جوڑو_Stmtsp(ifp);
            {
                // "// _Vjoin_nlists_eligible is how many lists eligable for picking"
                // "int _Vjoin_nlists_eligible = 0;"
                AstVar* ثابت eligibleVarp = نیا AstVar{
                    fl, VVarType::VAR, "_Vjoin_nlists_eligible", VFlag_ثنائی_ہندسہ_Packed{}, 32};
                eligibleVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                eligibleVarp->funcLocal(سچ);
                m_breakVarp->جوڑو_اگلا_یہاں(eligibleVarp);  // Add to function top, not newp
                ifp->جوڑو_Thensp(نیا Ast_تفویض{fl, نیا AstVarRef{fl, eligibleVarp, VAccess::WRITE},
                                             نیا AstConst{fl, AstConst::Widthed_قدر{}, 32, 0}});

                i = 0;
                برائے (Ast_RSProd_شے* proditemp : lists) {
                    (خالی)proditemp;
                    // "_Vjoin_nlists_eligible += _Vjoin_nleft_0 ? 1 : 0;"
                    ifp->جوڑو_Thensp(نیا Ast_تفویض{
                        fl, نیا AstVarRef{fl, eligibleVarp, VAccess::WRITE},
                        نیا Ast_جوڑو{
                            fl, نیا AstVarRef{fl, eligibleVarp, VAccess::READ},
                            نیا AstCond{
                                fl,
                                نیا AstNeq{fl, نیا AstConst{fl, AstConst::Widthed_قدر{}, 32, 0},
                                           نیا AstVarRef{fl, nleftVarps[i], VAccess::READ}},
                                نیا AstConst{fl, AstConst::Widthed_قدر{}, 32, 1},
                                نیا AstConst{fl, AstConst::Widthed_قدر{}, 32, 0}}}});
                    ++i;
                }

                // "if (!_Vjoin_nlists_eligible) break;"  // Out of elements
                ifp->جوڑو_Thensp(
                    نیا AstIf{fl,
                              نیا AstEq{fl, نیا AstConst{fl, AstConst::Widthed_قدر{}, 32, 0},
                                        نیا AstVarRef{fl, eligibleVarp, VAccess::READ}},
                              نیا AstJumpGo{fl, m_jumpBlockp}});

                // "// +1 to simplify usage, so 0 = done"
                // "_Vjoin_picked_elist = 1 + (std::rand() % _Vjoin_nlists_eligible);"
                ifp->جوڑو_Thensp(نیا Ast_تفویض{
                    fl, نیا AstVarRef{fl, pickedVarp, VAccess::WRITE},
                    نیا Ast_جوڑو{fl, نیا AstConst{fl, AstConst::Widthed_قدر{}, 32, 1},
                               نیا AstModDiv{fl, نیا AstRand{fl, AstRand::SRandomU32{}},
                                             نیا AstVarRef{fl, eligibleVarp, VAccess::READ}}}});

                // "_Vjoin_repick = false;
                ifp->جوڑو_Thensp(نیا Ast_تفویض{fl, نیا AstVarRef{fl, repickVarp, VAccess::WRITE},
                                             نیا AstConst{fl, AstConst::ثنائی_ہندسہ_False{}}});
            }

            // "// Number of eligible lists left before hit _Vjoin_picked one"
            // "int _Vjoin_sel_elist = _Vjoin_picked_elist;"
            AstVar* ثابت selVarp
                = نیا AstVar{fl, VVarType::VAR, "_Vjoin_sel_elist", VFlag_ثنائی_ہندسہ_Packed{}, 32};
            selVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            selVarp->funcLocal(سچ);
            m_breakVarp->جوڑو_اگلا_یہاں(selVarp);  // Add to function top, not newp
            whilep->جوڑو_Stmtsp(نیا Ast_تفویض{fl, نیا AstVarRef{fl, selVarp, VAccess::WRITE},
                                            نیا AstVarRef{fl, pickedVarp, VAccess::READ}});

            i = -1;
            برائے (Ast_RSProd_شے* proditemp : lists) {
                ++i;
                // "if (_Vjoin_nleft_0) {"
                ifp = نیا AstIf{fl,
                                نیا AstNeq{fl, نیا AstConst{fl, AstConst::Widthed_قدر{}, 32, 0},
                                           نیا AstVarRef{fl, nleftVarps[i], VAccess::READ}},
                                خالی_اشاریہ, خالی_اشاریہ};
                whilep->جوڑو_Stmtsp(ifp);
                {
                    // "--_Vjoin_sel_elist;"
                    ifp->جوڑو_Thensp(نیا Ast_تفویض{
                        fl, نیا AstVarRef{fl, selVarp, VAccess::WRITE},
                        نیا AstSub{fl, نیا AstVarRef{fl, selVarp, VAccess::READ},
                                   نیا AstConst{fl, AstConst::Widthed_قدر{}, 32, 1}}});

                    // "if (_Vjoin_sel_elist == 0) {"
                    AstIf* ثابت jIfp = نیا AstIf{
                        fl,
                        نیا AstEq{fl, نیا AstConst{fl, AstConst::Widthed_قدر{}, 32, 0},
                                  نیا AstVarRef{fl, selVarp, VAccess::READ}},
                        خالی_اشاریہ, خالی_اشاریہ};
                    ifp->جوڑو_Thensp(jIfp);
                    {
                        // "switch (_Vjoin_nleft_0) {"
                        //     "case 2 / * N(a)  * /: {statement}; break;"
                        //     "case 1 / * N(a) - 1 * /: {statement}; break;"
                        uint32_t j = static_cast<uint32_t>(listStmts[proditemp].سائز());
                        برائے (Ast_گرہ_Stmt* iprodp : listStmts[proditemp]) {
                            jIfp->جوڑو_Thensp(نیا AstIf{
                                fl,
                                نیا AstEq{fl, نیا AstConst{fl, AstConst::Widthed_قدر{}, 32, j},
                                          نیا AstVarRef{fl, nleftVarps[i], VAccess::READ}},
                                iprodp->clone_درخت(جھوٹ)});
                            --j;
                        }

                        // "--_Vjoin_nleft_0;"
                        jIfp->جوڑو_Thensp(نیا Ast_تفویض{
                            fl, نیا AstVarRef{fl, nleftVarps[i], VAccess::WRITE},
                            نیا AstSub{fl, نیا AstVarRef{fl, nleftVarps[i], VAccess::READ},
                                       نیا AstConst{fl, AstConst::Widthed_قدر{}, 32, 1}}});

                        // "if (0 == _Vjoin_nleft_0) _Vjoin_repick = true;"
                        jIfp->جوڑو_Thensp(نیا AstIf{
                            fl,
                            نیا AstEq{fl, نیا AstConst{fl, AstConst::Widthed_قدر{}, 32, 0},
                                      نیا AstVarRef{fl, nleftVarps[i], VAccess::READ}},
                            نیا Ast_تفویض{fl, نیا AstVarRef{fl, repickVarp, VAccess::WRITE},
                                          نیا AstConst{fl, AstConst::ثنائی_ہندسہ_True{}}}});

                        // "continue;"
                        jIfp->جوڑو_Thensp(نیا AstJumpGo{fl, whilep});
                    }
                }
            }
        }

        UINFOTREE(9, newp, "ForkJoin new", "-");
        واپس newp;
    }

    // VISITORS

    خالی دورہ(Ast_گرہ_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildren(nodep);
    }

    خالی دورہ(AstRandSequence* nodep) override {
        UINFO(9, "visit " << nodep);
        UASSERT_OBJ(m_modp, nodep, "randsequence not under module");
        اگر (m_rsp) {
            // No examples found in the wild
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: randsequence under randsequence");
            VL_DO_DANGLING(ڈالو_Deletep(nodep->unlink_Fr_عقب()), nodep);
            واپس;
        }

        UINFOTREE(9, nodep, "rsIn", "");
        VL_RESTORER(m_rsp);
        m_rsp = nodep;
        VL_RESTORER(m_آغاز_Prodp);

        VL_RESTORER_COPY(m_localizes);
        VL_RESTORER_COPY(m_localize_نام);
        VL_RESTORER_COPY(m_localizeRemaps);
        ڈھونڈو_Localizes(nodep);

        // Find first production
        m_آغاز_Prodp = nodep->prodsp();
        اگر (nodep->prodp()) m_آغاز_Prodp = nodep->prodp();  // Has a start production selected

        // Build production functions
        برائے (AstRSProd* prodp = nodep->prodsp(); prodp; prodp = VN_AS(prodp->nextp(), RSProd)) {
            newProdFunc(prodp);
            // Don't iterate yet, need to have tasks we can reference
        }

        // Iterate all of the new productions, moving guts into functions just made
        // Must do those with 'rand join' first, as they need to see the raw AstRS* nodes
        // before they are removed.  (Alternative would be to keep them until the end).
        UINFOTREE(9, nodep, "RS Tree pre-it", "-");
        std::unordered_سیٹ<AstRSProd*> prodHasRandJoin;
        برائے (AstRSProd* prodp = nodep->prodsp(); prodp; prodp = VN_AS(prodp->nextp(), RSProd)) {
            prodp->foreach([&](ثابت AstRSProdList* ثابت prodlistp) {
                اگر (prodlistp->randJoin()) prodHasRandJoin.emplace(prodp);
            });
        }
        برائے (AstRSProd *nextp, *prodp = nodep->prodsp(); prodp; prodp = nextp) {
            nextp = VN_AS(prodp->nextp(), RSProd);
            اگر (prodHasRandJoin.گنتی(prodp)) VL_DO_DANGLING(iterate(prodp), prodp);
        }
        برائے (AstRSProd *nextp, *prodp = nodep->prodsp(); prodp; prodp = nextp) {
            nextp = VN_AS(prodp->nextp(), RSProd);
            اگر (!prodHasRandJoin.گنتی(prodp)) VL_DO_DANGLING(iterate(prodp), prodp);
        }
        UINFOTREE(9, nodep, "RS Tree post-it", "-");

        // Replace randsequence with call to randsequence start function
        AstTask* ثابت آغاز_Funcp = new_آغاز_Func(nodep);
        AstArg* argsp = خالی_اشاریہ;
        برائے (خودکار& itr : m_localize_نام) {
            AstVar* ثابت lvarp = itr.دوسرا;
            argsp = Ast_گرہ::جوڑو_اگلا(
                argsp, نیا AstArg{nodep->fileline(), "__Vrsarg_" + lvarp->name(),
                                  نیا AstVarRef{nodep->fileline(), lvarp, VAccess::READWRITE}});
        }
        AstTaskRef* ثابت callStartp
            = نیا AstTaskRef{nodep->fileline(), آغاز_Funcp->name(), argsp};
        callStartp->taskp(آغاز_Funcp);
        nodep->replaceWith(نیا Ast_Stmt_اظہاریہ{nodep->fileline(), callStartp});

        VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
    }
    خالی دورہ(AstRSProd* nodep) override {
        // Production, immediately under a randsequence
        UINFO(9, "visit " << nodep);
        UINFOTREE(9, nodep, "visit " << nodep, "-");
        UASSERT_OBJ(!m_prodFuncp, nodep, "recursive production definition?");
        VL_RESTORER(m_prodFuncp);
        خودکار it = m_prodFuncps.ڈھونڈو(nodep);
        UASSERT_OBJ(it != m_prodFuncps.انتہا(), nodep, "No production function made");
        m_prodFuncp = it->دوسرا;

        // Create a break variable
        // TODO we could do this only if break exists in the downstream production,
        // but as-is we'll optimize it away in most cases anyways
        VL_RESTORER(m_breakVarp);
        VL_RESTORER_COPY(m_localizeRemaps);
        m_breakVarp = newBreakVar(nodep->fileline(), سچ);
        m_prodFuncp->جوڑو_Stmtsp(m_breakVarp);

        // Production formal ports become real input ports on the generated task.
        // Must be added after m_breakVarp / localize ports so positional argument
        // order in newProdFuncRef stays consistent: [break, localize..., user...].
        اگر (Ast_گرہ* ثابت portsp = nodep->portsp()) {
            portsp->unlink_Fr_عقب_With_اگلا();
            m_prodFuncp->جوڑو_Stmtsp(portsp);
            برائے (Ast_گرہ* itp = portsp; itp; itp = itp->nextp()) {
                AstVar* ثابت پورٹ_Varp = VN_AS(itp, Var);
                پورٹ_Varp->funcLocal(سچ);
                پورٹ_Varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            }
        }

        // Put JumpBlock immediately under the new function to support
        // a future break/return.  V3Const will rip it out if unneeded.
        VL_RESTORER(m_jumpBlockp);
        m_jumpBlockp = نیا Ast_Jump_بلاک{nodep->fileline(), خالی_اشاریہ};
        m_prodFuncp->جوڑو_Stmtsp(m_jumpBlockp);

        اگر (nodep->fvarp())
            nodep->fvarp()->v3warn(E_UNSUPPORTED,
                                   "Unsupported: randsequence production function variable");

        // Move children into m_prodFuncp, and iterate there
        اگر (!nodep->rulesp()) {  // Nothing to do
        } ورنہ اگر (nodep->rulesp()->prodlistsp() && nodep->rulesp()->prodlistsp()->randJoin()) {
            AstRSProdList* ثابت prodlistp = nodep->rulesp()->prodlistsp();
            Ast_گرہ* ثابت itemsp = newProdRandJoin(nodep, prodlistp);
            m_jumpBlockp->جوڑو_Stmtsp(itemsp);
            iterate_And_اگلا_Null(itemsp);
        } ورنہ اگر (!nodep->rulesp()->nextp()) {  // Single rule/list, can just do it
            // RSPROD(RSRULE(weight, stmt)) -> IF(weight != 0, stmt)
            ثابت AstRSRule* ثابت rulep = nodep->rulesp();
            Ast_گرہ* itemsp = خالی_اشاریہ;
            اگر (rulep->وزن_Stmtsp()) itemsp = rulep->وزن_Stmtsp()->unlink_Fr_عقب_With_اگلا();
            اگر (rulep->prodlistsp())
                itemsp = Ast_گرہ::جوڑو_اگلا(itemsp, rulep->prodlistsp()->unlink_Fr_عقب_With_اگلا());
            // Can ignore rulep->weightp() unless is zero, in which case noop, add if
            اگر (rulep->weightp())
                itemsp = نیا AstIf{rulep->weightp()->fileline(),
                                   نیا AstNeq{rulep->weightp()->fileline(),
                                              نیا AstConst{rulep->weightp()->fileline(), 0},
                                              rulep->weightp()->unlink_Fr_عقب()},
                                   itemsp};
            // Move prodlist to parent (m_prodFunc) and process
            اگر (itemsp) {
                UINFOTREE(9, itemsp, "additems", "-");
                m_jumpBlockp->جوڑو_Stmtsp(itemsp);
                iterate_And_اگلا_Null(itemsp);
                UINFOTREE(9, m_jumpBlockp, "jumpBlockNow", "-");
            }
        } ورنہ {
            // List of rules with "OR", need to random-weight them
            // RSPROD(RSRULE(weight1, stmt!), RSRULE(weight2, stmt2))
            //   -> RANDCASE(CASEITEM(weight1, stmt1), CASEITEM(weight2, stmt2))
            AstRandCase* ثابت randcasep = نیا AstRandCase{nodep->fileline(), خالی_اشاریہ};
            برائے (AstRSRule* rulep = nodep->rulesp(); rulep;
                 rulep = VN_AS(rulep->nextp(), RSRule)) {
                Ast_گرہ* itemsp = خالی_اشاریہ;
                اگر (rulep->وزن_Stmtsp()) itemsp = rulep->وزن_Stmtsp()->unlink_Fr_عقب_With_اگلا();
                اگر (rulep->prodlistsp())
                    itemsp = Ast_گرہ::جوڑو_اگلا(itemsp, rulep->prodlistsp()->unlink_Fr_عقب_With_اگلا());
                اگر (itemsp) {
                    Ast_گرہ_اظہاریہ* ثابت weightp = rulep->weightp()
                                                     ? rulep->weightp()->unlink_Fr_عقب()
                                                     : نیا AstConst{rulep->fileline(), 1};
                    randcasep->جوڑو_Itemsp(نیا Ast_Case_شے{rulep->fileline(), weightp, itemsp});
                }
            }
            m_jumpBlockp->جوڑو_Stmtsp(randcasep);
            iterate_And_اگلا_Null(randcasep);
        }

        // V3Task needs to know if we ended up making a recursive function
        m_prodFuncp->foreach([&](Ast_گرہ_FTask_Ref* ثابت nodep) {
            اگر (nodep->taskp() == m_prodFuncp) m_prodFuncp->recursive(سچ);
        });
        UINFOTREE(9, m_prodFuncp, "rsprod-task-done " << nodep, "-");

        // Done with production, should have moved everything to new task
        VL_DO_DANGLING(ڈالو_Deletep(nodep->unlink_Fr_عقب()), nodep);
    }
    خالی دورہ(AstRSRule* nodep) override {
        // Rule of what to produce (as a list) under a production
        nodep->v3fatalSrc("randsequence rule should be iterated and deleted by AstRSProd");
    }
    خالی دورہ(AstRSProdList* nodep) override {
        // List of productions to call, potentially with a weight
        UINFO(9, "visit " << nodep);
        UASSERT_OBJ(m_prodFuncp, nodep, "RSProdList not under production");

        // Move prodlist to parent (m_prodFunc) and process
        اگر (Ast_گرہ* ثابت itemsp = nodep->prodsp()) {
            UASSERT_OBJ(!nodep->randJoin(), nodep,
                        "rand join should have been handled in visit(RSRule)");
            nodep->replaceWith(itemsp->unlink_Fr_عقب_With_اگلا());
            // Will soon iterate itemsp as part of normal visit process
            // These will become sequential in the generated task
        } ورنہ {
            nodep->unlink_Fr_عقب();
        }
        VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
    }
    خالی دورہ(Ast_RSProd_شے* nodep) override {
        // "call" to generate using another production
        UINFO(9, "visit " << nodep);
        UASSERT_OBJ(m_prodFuncp, nodep, "RSProdItem not under production");
        AstRSProd* ثابت foundp = nodep->prodp();
        UASSERT_OBJ(foundp, nodep, "Unlinked production reference");
        // Pass through caller-side argument list (IEEE 1800-2023 18.17.7).
        AstArg* ثابت userArgsp
            = nodep->argsp() ? nodep->argsp()->unlink_Fr_عقب_With_اگلا() : خالی_اشاریہ;
        // Convert to task call
        Ast_گرہ* ثابت newp = newProdFuncRef(nodep, foundp, m_breakVarp, userArgsp);
        // The production might have done a "break;", skip other steps if so
        newp->جوڑو_اگلا(نیا AstIf{nodep->fileline(),
                                نیا AstVarRef{nodep->fileline(), m_breakVarp, VAccess::READ},
                                نیا AstJumpGo{nodep->fileline(), m_jumpBlockp}});

        nodep->replaceWith(newp);
        VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
    }

    خالی دورہ(AstRSBreak* nodep) override {
        UASSERT_OBJ(m_jumpBlockp, nodep, "RSBreak not under production jump block");
        UASSERT_OBJ(m_breakVarp, nodep, "no break variable");
        Ast_گرہ_Stmt* ثابت newp = نیا Ast_تفویض{
            nodep->fileline(), نیا AstVarRef{nodep->fileline(), m_breakVarp, VAccess::WRITE},
            نیا AstConst{nodep->fileline(), AstConst::ثنائی_ہندسہ_True{}}};
        newp->جوڑو_اگلا(نیا AstJumpGo{nodep->fileline(), m_jumpBlockp});
        nodep->replaceWith(newp);
        VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
    }
    خالی دورہ(AstRSReturn* nodep) override {
        UASSERT_OBJ(m_jumpBlockp, nodep, "RSReturn not under production jump block");
        nodep->replaceWith(نیا AstJumpGo{nodep->fileline(), m_jumpBlockp});
        VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
    }
    خالی دورہ(AstVarRef* nodep) override {
        // Change references to variables inside randsequence to references to
        // ref variables on the new task
        ثابت خودکار it = m_localizeRemaps.ڈھونڈو(nodep->varp());
        اگر (it != m_localizeRemaps.انتہا()) {
            AstVar* ثابت iovarp = it->دوسرا;
            UINFO(9, "VARREF remap " << nodep << " -> " << iovarp);
            nodep->varp(iovarp);
        }
    }

    خالی دورہ(Ast_گرہ* nodep) override { iterateChildren(nodep); }

عوامی:
    // CONSTRUCTORS
    صریح RandSequenceVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~RandSequenceVisitor() override = طےشدہ;
};

//######################################################################
// RandSequence method class functions

خالی V3RandSequence::randSequenceNetlist(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { ثابت RandSequenceVisitor randSequenceVisitor{nodep}; }
    V3Global::dump_Check_Global_درخت("randsequence", 0, dump_درخت_Either_Level() >= 3);
}
