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

رده RSProdMatcher final : عمومی VNodeMatcher {
عمومی:
    منطقی گره_Match(ثابت Ast_گره* nodep) ثابت override { بازگشت VN_IS(nodep, RSProd); }
};

//######################################################################
// Visitor that marks classes needing a randomize() method

رده RandSequenceVisitor final : عمومی VNVisitor {
    // STATE - global
    std::map<AstRSProd*, Ast_گره_FTask*> m_prodFuncps;  // Generated production functions

    // STATE - for current visit position (use VL_RESTORER)
    Ast_گره_Module* m_modp = nullptr;  // Current module
    AstRandSequence* m_rsp = nullptr;  // Current randsequence
    AstRSProd* m_startProdp = nullptr;  // Starting production
    Ast_گره_FTask* m_prodFuncp = nullptr;  // Production function being built
    AstJumpBlock* m_jumpBlockp = nullptr;  // Building jumpblock for return/break
    AstVar* m_breakVarp = nullptr;  // Break variable
    std::unordered_set<AstVar*> m_localizes;  // Variables to become function inouts
    std::map<std::string, AstVar*> m_localizeNames;  // Ordered names of function inouts
    std::unordered_map<ثابت AstVar*, AstVar*> m_localizeRemaps;  // New ports for old vars

    // METHODS
    تهی findLocalizes(AstRandSequence* nodep) {
        std::set<AstVar*> localVars;
        nodep->foreach([&](Ast_گره* ثابت nodep) {
            اگر (ثابت AstVarRef* ثابت anodep = VN_CAST(nodep, VarRef)) {
                m_localizes.emplace(anodep->varp());
            } وگرنه اگر (AstVar* ثابت anodep = VN_CAST(nodep, Var)) {
                localVars.emplace(anodep);
            }
        });
        // Clear every variable allocated in the RandSequence
        برای (AstVar* varp : localVars) m_localizes.erase(varp);
        // Sort by name, so function arguments are stable
        برای (AstVar* varp : m_localizes) {
            UINFO(9, "localize " << varp);
            m_localizeNames.emplace(varp->name(), varp);
        }
    }

    AstVar* newBreakVar(File_سطر* fl, منطقی asOutput) {
        AstVar* ثابت varp = جدید AstVar{fl, VVarType::PORT, "__VrsBreak", VFlagBitPacked{}, 1};
        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        varp->funcLocal(درست);
        اگر (asOutput) varp->direction(VDirection::OUTPUT);
        Ast_گره* outp = varp;
        outp->addNext(جدید AstAssign{fl, جدید AstVarRef{fl, varp, VAccess::WRITE},
                                    جدید AstConst{fl, AstConst::BitFalse{}}});

        // Also add arguments as next's
        برای (ثابت auto& itr : m_localizeNames) {
            ثابت AstVar* ثابت lvarp = itr.second;
            AstVar* ثابت iovarp
                = جدید AstVar{fl, VVarType::PORT, "__Vrsarg_" + lvarp->name(), lvarp};
            iovarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            iovarp->funcLocal(درست);
            iovarp->direction(VDirection::REF);
            varp->addNext(iovarp);
            m_localizeRemaps.emplace(lvarp, iovarp);
        }
        بازگشت varp;
    }

    AstTask* newStartFunc(AstRandSequence* nodep) {
        // Create wrapper that is called by the sequence
        AstTask* ثابت taskp = جدید AstTask{m_startProdp->fileline(), m_rsp->name(), nullptr};
        taskp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        m_modp->addStmtsp(taskp);

        // Create local (not output) break variable
        VL_RESTORER_COPY(m_localizeRemaps);
        AstVar* breakVarp = newBreakVar(nodep->fileline(), نادرست);
        taskp->addStmtsp(breakVarp);

        // Call the start production's task
        taskp->addStmtsp(newProdFuncRef(nodep, m_startProdp, breakVarp, nullptr));

        UINFOTREE(9, taskp, "newStart", "");
        بازگشت taskp;
    }

    Ast_گره* newProdFuncRef(Ast_گره* nodep, AstRSProd* prodp, AstVar* breakVarp,
                            AstArg* userArgsp) {
        auto it = m_prodFuncps.find(prodp);
        UASSERT_OBJ(it != m_prodFuncps.end(), nodep, "No production function made");
        Ast_گره_FTask* ثابت prodFuncp = it->second;
        File_سطر* ثابت fl = nodep->fileline();
        // V3Width already ran before V3RandSequence, so VarRefs we create here
        // need dtype set explicitly, V3Broken later checks width == widthMin.
        AstVarRef* ثابت breakRefp = جدید AstVarRef{fl, breakVarp, VAccess::WRITE};
        breakRefp->dtypeFrom(breakVarp);
        AstArg* ثابت argsp = جدید AstArg{fl, breakVarp->name(), breakRefp};
        برای (ثابت auto& itr : m_localizeNames) {
            ثابت AstVar* ثابت lvarp = itr.second;
            AstVar* ثابت iovarp = m_localizeRemaps[lvarp];
            UASSERT_OBJ(iovarp, nodep, "No new port variable for local variable" << lvarp);
            AstVarRef* ثابت refp = جدید AstVarRef{fl, iovarp, VAccess::READWRITE};
            refp->dtypeFrom(iovarp);
            argsp->addNext(جدید AstArg{nodep->fileline(), "__Vrsarg_" + lvarp->name(), refp});
        }
        اگر (userArgsp) argsp->addNext(userArgsp);
        Ast_گره* ثابت newp
            = جدید Ast_Stmt_عبارت{fl, جدید AstTaskRef{fl, VN_AS(prodFuncp, Task), argsp}};
        بازگشت newp;
    }

    تهی newProdFunc(AstRSProd* nodep) {
        // Task, as time may pass
        AstTask* ثابت newp
            = جدید AstTask{nodep->fileline(), m_rsp->name() + "_" + nodep->name(), nullptr};
        m_modp->addStmtsp(newp);
        m_prodFuncps.emplace(nodep, newp);
        UINFOTREE(9, newp, "newProd", "");
    }

    Ast_گره* newProdRandJoin(AstRSProd* /*prodp*/, AstRSProdList* prodlistp) {
        // For weight == 1.0 longer sequence (favor stay in a)
        // For weight == 0.0 shorter squence (favor change a/b)
        UASSERT_OBJ(prodlistp->weightp(), prodlistp, "Weight should have default CONST(0.5)");
        Ast_گره_عبارت* weightp = prodlistp->weightp()->unlinkFrBack();

        // Build map of production lists and productions we need to join
        // Must mainain the relative order of each sequence.
        std::deque<AstRSProdItem*> lists;
        std::unordered_map<AstRSProdItem*, std::deque<Ast_گره_Stmt*>> listStmts;
        برای (AstRSProdItem* proditemp = VN_AS(prodlistp->prodsp(), RSProdItem); proditemp;
             proditemp = VN_AS(proditemp->nextp(), RSProdItem)) {
            lists.افزودن_back(proditemp);
            ثابت AstRSProd* ثابت subProdp = proditemp->prodp();
            اگر (!subProdp) ادامه;
            اگر (!subProdp->rulesp()) ادامه;
            اگر (!subProdp->rulesp()->prodlistsp()) ادامه;
            برای (Ast_گره_Stmt* subStmtp
                 = VN_AS(subProdp->rulesp()->prodlistsp()->prodsp(), گره_Stmt);
                 subStmtp; subStmtp = VN_AS(subStmtp->nextp(), گره_Stmt)) {
                listStmts[proditemp].افزودن_back(subStmtp);
            }
        }

        UINFO(9, "RandJoin productions called:");
        برای (AstRSProdItem* proditemp : lists) {
            UINFO(9, "  list " << proditemp);
            برای (ثابت Ast_گره_Stmt* prodip : listStmts[proditemp])
                UINFO(9, "    calls " << prodip);
        }

        // Need to clone all nodes used

        File_سطر* fl = prodlistp->fileline();
        Ast_گره* newp = nullptr;

        // "bool _Vjoin_repick = true;"  // Pick a new longer sequence
        AstVar* repickVarp = جدید AstVar{fl, VVarType::VAR, "_Vjoin_repick", VFlagBitPacked{}, 1};
        repickVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        repickVarp->funcLocal(درست);
        m_breakVarp->addNextHere(repickVarp);  // Add to function top, not newp
        newp = Ast_گره::addNext(newp,
                                جدید AstAssign{fl, جدید AstVarRef{fl, repickVarp, VAccess::WRITE},
                                              جدید AstConst{fl, AstConst::BitTrue{}}});

        // "int _Vjoin_nleft_0 = N(items);"  // N(a) Number left to generate in list 0, starts at
        // N(list0)
        std::deque<AstVar*> nleftVarps;
        صحیح i = 0;
        برای (AstRSProdItem* proditemp : lists) {
            // 'rand join arule arule arule' is legal, so need numbered, not non-unique named
            // nleft's
            AstVar* ثابت varp = جدید AstVar{
                fl, VVarType::VAR, "_Vjoin_nleft_" + std::to_string(i++), VFlagBitPacked{}, 32};
            varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            varp->funcLocal(درست);
            m_breakVarp->addNextHere(varp);  // Add to function top, not newp
            nleftVarps.افزودن_back(varp);

            newp = Ast_گره::addNext(
                newp,
                جدید AstAssign{fl, جدید AstVarRef{fl, varp, VAccess::WRITE},
                              جدید AstConst{fl, AstConst::Widthed_مقدار{}, 32,
                                           static_cast<uint32_t>(listStmts[proditemp].size())}});
        }

        // "int _Vjoin_picked_elist = 0;"  // 0 = pick first eligible (non-finished) list, 1 = pick
        // second, etc
        AstVar* ثابت pickedVarp
            = جدید AstVar{fl, VVarType::VAR, "_Vjoin_picked_elist", VFlagBitPacked{}, 1};
        pickedVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        pickedVarp->funcLocal(درست);
        m_breakVarp->addNextHere(pickedVarp);  // Add to function top, not newp

        // "while(1) {"  // Generate until make complete sequence
        AstJumpBlock* ثابت whilep = جدید AstJumpBlock{fl, nullptr};
        newp = Ast_گره::addNext(newp, جدید AstLoop{fl, whilep});
        {
            // "if ((std::rand() & 0xffffUL) > (0xffff * weight)) _Vjoin_repick = true;"
            whilep->addStmtsp(جدید AstIf{
                fl,
                جدید AstGt{
                    fl, جدید AstRand{fl, AstRand::SRandomU32{}},
                    جدید AstRToIS{fl, جدید AstMulD{fl,
                                                 جدید AstConst{fl, AstConst::RealDouble{},
                                                              static_cast<دوگانه>(0xffffffffUL)},
                                                 weightp}}},
                جدید AstAssign{fl, جدید AstVarRef{fl, repickVarp, VAccess::WRITE},
                              جدید AstConst{fl, AstConst::BitTrue{}}}});

            // "if (_Vjoin_repick) {"
            AstIf* ifp = جدید AstIf{fl, جدید AstVarRef{fl, repickVarp, VAccess::READ}};
            whilep->addStmtsp(ifp);
            {
                // "// _Vjoin_nlists_eligible is how many lists eligable for picking"
                // "int _Vjoin_nlists_eligible = 0;"
                AstVar* ثابت eligibleVarp = جدید AstVar{
                    fl, VVarType::VAR, "_Vjoin_nlists_eligible", VFlagBitPacked{}, 32};
                eligibleVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                eligibleVarp->funcLocal(درست);
                m_breakVarp->addNextHere(eligibleVarp);  // Add to function top, not newp
                ifp->addThensp(جدید AstAssign{fl, جدید AstVarRef{fl, eligibleVarp, VAccess::WRITE},
                                             جدید AstConst{fl, AstConst::Widthed_مقدار{}, 32, 0}});

                i = 0;
                برای (AstRSProdItem* proditemp : lists) {
                    (تهی)proditemp;
                    // "_Vjoin_nlists_eligible += _Vjoin_nleft_0 ? 1 : 0;"
                    ifp->addThensp(جدید AstAssign{
                        fl, جدید AstVarRef{fl, eligibleVarp, VAccess::WRITE},
                        جدید AstAdd{
                            fl, جدید AstVarRef{fl, eligibleVarp, VAccess::READ},
                            جدید AstCond{
                                fl,
                                جدید AstNeq{fl, جدید AstConst{fl, AstConst::Widthed_مقدار{}, 32, 0},
                                           جدید AstVarRef{fl, nleftVarps[i], VAccess::READ}},
                                جدید AstConst{fl, AstConst::Widthed_مقدار{}, 32, 1},
                                جدید AstConst{fl, AstConst::Widthed_مقدار{}, 32, 0}}}});
                    ++i;
                }

                // "if (!_Vjoin_nlists_eligible) break;"  // Out of elements
                ifp->addThensp(
                    جدید AstIf{fl,
                              جدید AstEq{fl, جدید AstConst{fl, AstConst::Widthed_مقدار{}, 32, 0},
                                        جدید AstVarRef{fl, eligibleVarp, VAccess::READ}},
                              جدید AstJumpGo{fl, m_jumpBlockp}});

                // "// +1 to simplify usage, so 0 = done"
                // "_Vjoin_picked_elist = 1 + (std::rand() % _Vjoin_nlists_eligible);"
                ifp->addThensp(جدید AstAssign{
                    fl, جدید AstVarRef{fl, pickedVarp, VAccess::WRITE},
                    جدید AstAdd{fl, جدید AstConst{fl, AstConst::Widthed_مقدار{}, 32, 1},
                               جدید AstModDiv{fl, جدید AstRand{fl, AstRand::SRandomU32{}},
                                             جدید AstVarRef{fl, eligibleVarp, VAccess::READ}}}});

                // "_Vjoin_repick = false;
                ifp->addThensp(جدید AstAssign{fl, جدید AstVarRef{fl, repickVarp, VAccess::WRITE},
                                             جدید AstConst{fl, AstConst::BitFalse{}}});
            }

            // "// Number of eligible lists left before hit _Vjoin_picked one"
            // "int _Vjoin_sel_elist = _Vjoin_picked_elist;"
            AstVar* ثابت selVarp
                = جدید AstVar{fl, VVarType::VAR, "_Vjoin_sel_elist", VFlagBitPacked{}, 32};
            selVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            selVarp->funcLocal(درست);
            m_breakVarp->addNextHere(selVarp);  // Add to function top, not newp
            whilep->addStmtsp(جدید AstAssign{fl, جدید AstVarRef{fl, selVarp, VAccess::WRITE},
                                            جدید AstVarRef{fl, pickedVarp, VAccess::READ}});

            i = -1;
            برای (AstRSProdItem* proditemp : lists) {
                ++i;
                // "if (_Vjoin_nleft_0) {"
                ifp = جدید AstIf{fl,
                                جدید AstNeq{fl, جدید AstConst{fl, AstConst::Widthed_مقدار{}, 32, 0},
                                           جدید AstVarRef{fl, nleftVarps[i], VAccess::READ}},
                                nullptr, nullptr};
                whilep->addStmtsp(ifp);
                {
                    // "--_Vjoin_sel_elist;"
                    ifp->addThensp(جدید AstAssign{
                        fl, جدید AstVarRef{fl, selVarp, VAccess::WRITE},
                        جدید AstSub{fl, جدید AstVarRef{fl, selVarp, VAccess::READ},
                                   جدید AstConst{fl, AstConst::Widthed_مقدار{}, 32, 1}}});

                    // "if (_Vjoin_sel_elist == 0) {"
                    AstIf* ثابت jIfp = جدید AstIf{
                        fl,
                        جدید AstEq{fl, جدید AstConst{fl, AstConst::Widthed_مقدار{}, 32, 0},
                                  جدید AstVarRef{fl, selVarp, VAccess::READ}},
                        nullptr, nullptr};
                    ifp->addThensp(jIfp);
                    {
                        // "switch (_Vjoin_nleft_0) {"
                        //     "case 2 / * N(a)  * /: {statement}; break;"
                        //     "case 1 / * N(a) - 1 * /: {statement}; break;"
                        uint32_t j = static_cast<uint32_t>(listStmts[proditemp].size());
                        برای (Ast_گره_Stmt* iprodp : listStmts[proditemp]) {
                            jIfp->addThensp(جدید AstIf{
                                fl,
                                جدید AstEq{fl, جدید AstConst{fl, AstConst::Widthed_مقدار{}, 32, j},
                                          جدید AstVarRef{fl, nleftVarps[i], VAccess::READ}},
                                iprodp->clone_درخت(نادرست)});
                            --j;
                        }

                        // "--_Vjoin_nleft_0;"
                        jIfp->addThensp(جدید AstAssign{
                            fl, جدید AstVarRef{fl, nleftVarps[i], VAccess::WRITE},
                            جدید AstSub{fl, جدید AstVarRef{fl, nleftVarps[i], VAccess::READ},
                                       جدید AstConst{fl, AstConst::Widthed_مقدار{}, 32, 1}}});

                        // "if (0 == _Vjoin_nleft_0) _Vjoin_repick = true;"
                        jIfp->addThensp(جدید AstIf{
                            fl,
                            جدید AstEq{fl, جدید AstConst{fl, AstConst::Widthed_مقدار{}, 32, 0},
                                      جدید AstVarRef{fl, nleftVarps[i], VAccess::READ}},
                            جدید AstAssign{fl, جدید AstVarRef{fl, repickVarp, VAccess::WRITE},
                                          جدید AstConst{fl, AstConst::BitTrue{}}}});

                        // "continue;"
                        jIfp->addThensp(جدید AstJumpGo{fl, whilep});
                    }
                }
            }
        }

        UINFOTREE(9, newp, "ForkJoin new", "-");
        بازگشت newp;
    }

    // VISITORS

    تهی visit(Ast_گره_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildren(nodep);
    }

    تهی visit(AstRandSequence* nodep) override {
        UINFO(9, "visit " << nodep);
        UASSERT_OBJ(m_modp, nodep, "randsequence not under module");
        اگر (m_rsp) {
            // No examples found in the wild
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: randsequence under randsequence");
            VL_DO_DANGLING(افزودن_Deletep(nodep->unlinkFrBack()), nodep);
            بازگشت;
        }

        UINFOTREE(9, nodep, "rsIn", "");
        VL_RESTORER(m_rsp);
        m_rsp = nodep;
        VL_RESTORER(m_startProdp);

        VL_RESTORER_COPY(m_localizes);
        VL_RESTORER_COPY(m_localizeNames);
        VL_RESTORER_COPY(m_localizeRemaps);
        findLocalizes(nodep);

        // Find first production
        m_startProdp = nodep->prodsp();
        اگر (nodep->prodp()) m_startProdp = nodep->prodp();  // Has a start production selected

        // Build production functions
        برای (AstRSProd* prodp = nodep->prodsp(); prodp; prodp = VN_AS(prodp->nextp(), RSProd)) {
            newProdFunc(prodp);
            // Don't iterate yet, need to have tasks we can reference
        }

        // Iterate all of the new productions, moving guts into functions just made
        // Must do those with 'rand join' first, as they need to see the raw AstRS* nodes
        // before they are removed.  (Alternative would be to keep them until the end).
        UINFOTREE(9, nodep, "RS Tree pre-it", "-");
        std::unordered_set<AstRSProd*> prodHasRandJoin;
        برای (AstRSProd* prodp = nodep->prodsp(); prodp; prodp = VN_AS(prodp->nextp(), RSProd)) {
            prodp->foreach([&](ثابت AstRSProdList* ثابت prodlistp) {
                اگر (prodlistp->randJoin()) prodHasRandJoin.emplace(prodp);
            });
        }
        برای (AstRSProd *nextp, *prodp = nodep->prodsp(); prodp; prodp = nextp) {
            nextp = VN_AS(prodp->nextp(), RSProd);
            اگر (prodHasRandJoin.شمار(prodp)) VL_DO_DANGLING(iterate(prodp), prodp);
        }
        برای (AstRSProd *nextp, *prodp = nodep->prodsp(); prodp; prodp = nextp) {
            nextp = VN_AS(prodp->nextp(), RSProd);
            اگر (!prodHasRandJoin.شمار(prodp)) VL_DO_DANGLING(iterate(prodp), prodp);
        }
        UINFOTREE(9, nodep, "RS Tree post-it", "-");

        // Replace randsequence with call to randsequence start function
        AstTask* ثابت startFuncp = newStartFunc(nodep);
        AstArg* argsp = nullptr;
        برای (auto& itr : m_localizeNames) {
            AstVar* ثابت lvarp = itr.second;
            argsp = Ast_گره::addNext(
                argsp, جدید AstArg{nodep->fileline(), "__Vrsarg_" + lvarp->name(),
                                  جدید AstVarRef{nodep->fileline(), lvarp, VAccess::READWRITE}});
        }
        AstTaskRef* ثابت callStartp
            = جدید AstTaskRef{nodep->fileline(), startFuncp->name(), argsp};
        callStartp->taskp(startFuncp);
        nodep->replaceWith(جدید Ast_Stmt_عبارت{nodep->fileline(), callStartp});

        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
    }
    تهی visit(AstRSProd* nodep) override {
        // Production, immediately under a randsequence
        UINFO(9, "visit " << nodep);
        UINFOTREE(9, nodep, "visit " << nodep, "-");
        UASSERT_OBJ(!m_prodFuncp, nodep, "recursive production definition?");
        VL_RESTORER(m_prodFuncp);
        auto it = m_prodFuncps.find(nodep);
        UASSERT_OBJ(it != m_prodFuncps.end(), nodep, "No production function made");
        m_prodFuncp = it->second;

        // Create a break variable
        // TODO we could do this only if break exists in the downstream production,
        // but as-is we'll optimize it away in most cases anyways
        VL_RESTORER(m_breakVarp);
        VL_RESTORER_COPY(m_localizeRemaps);
        m_breakVarp = newBreakVar(nodep->fileline(), درست);
        m_prodFuncp->addStmtsp(m_breakVarp);

        // Production formal ports become real input ports on the generated task.
        // Must be added after m_breakVarp / localize ports so positional argument
        // order in newProdFuncRef stays consistent: [break, localize..., user...].
        اگر (Ast_گره* ثابت portsp = nodep->portsp()) {
            portsp->unlinkFrBackWithNext();
            m_prodFuncp->addStmtsp(portsp);
            برای (Ast_گره* itp = portsp; itp; itp = itp->nextp()) {
                AstVar* ثابت portVarp = VN_AS(itp, Var);
                portVarp->funcLocal(درست);
                portVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            }
        }

        // Put JumpBlock immediately under the new function to support
        // a future break/return.  V3Const will rip it out if unneeded.
        VL_RESTORER(m_jumpBlockp);
        m_jumpBlockp = جدید AstJumpBlock{nodep->fileline(), nullptr};
        m_prodFuncp->addStmtsp(m_jumpBlockp);

        اگر (nodep->fvarp())
            nodep->fvarp()->v3warn(E_UNSUPPORTED,
                                   "Unsupported: randsequence production function variable");

        // Move children into m_prodFuncp, and iterate there
        اگر (!nodep->rulesp()) {  // Nothing to do
        } وگرنه اگر (nodep->rulesp()->prodlistsp() && nodep->rulesp()->prodlistsp()->randJoin()) {
            AstRSProdList* ثابت prodlistp = nodep->rulesp()->prodlistsp();
            Ast_گره* ثابت itemsp = newProdRandJoin(nodep, prodlistp);
            m_jumpBlockp->addStmtsp(itemsp);
            iterateAndNextNull(itemsp);
        } وگرنه اگر (!nodep->rulesp()->nextp()) {  // Single rule/list, can just do it
            // RSPROD(RSRULE(weight, stmt)) -> IF(weight != 0, stmt)
            ثابت AstRSRule* ثابت rulep = nodep->rulesp();
            Ast_گره* itemsp = nullptr;
            اگر (rulep->weightStmtsp()) itemsp = rulep->weightStmtsp()->unlinkFrBackWithNext();
            اگر (rulep->prodlistsp())
                itemsp = Ast_گره::addNext(itemsp, rulep->prodlistsp()->unlinkFrBackWithNext());
            // Can ignore rulep->weightp() unless is zero, in which case noop, add if
            اگر (rulep->weightp())
                itemsp = جدید AstIf{rulep->weightp()->fileline(),
                                   جدید AstNeq{rulep->weightp()->fileline(),
                                              جدید AstConst{rulep->weightp()->fileline(), 0},
                                              rulep->weightp()->unlinkFrBack()},
                                   itemsp};
            // Move prodlist to parent (m_prodFunc) and process
            اگر (itemsp) {
                UINFOTREE(9, itemsp, "additems", "-");
                m_jumpBlockp->addStmtsp(itemsp);
                iterateAndNextNull(itemsp);
                UINFOTREE(9, m_jumpBlockp, "jumpBlockNow", "-");
            }
        } وگرنه {
            // List of rules with "OR", need to random-weight them
            // RSPROD(RSRULE(weight1, stmt!), RSRULE(weight2, stmt2))
            //   -> RANDCASE(CASEITEM(weight1, stmt1), CASEITEM(weight2, stmt2))
            AstRandCase* ثابت randcasep = جدید AstRandCase{nodep->fileline(), nullptr};
            برای (AstRSRule* rulep = nodep->rulesp(); rulep;
                 rulep = VN_AS(rulep->nextp(), RSRule)) {
                Ast_گره* itemsp = nullptr;
                اگر (rulep->weightStmtsp()) itemsp = rulep->weightStmtsp()->unlinkFrBackWithNext();
                اگر (rulep->prodlistsp())
                    itemsp = Ast_گره::addNext(itemsp, rulep->prodlistsp()->unlinkFrBackWithNext());
                اگر (itemsp) {
                    Ast_گره_عبارت* ثابت weightp = rulep->weightp()
                                                     ? rulep->weightp()->unlinkFrBack()
                                                     : جدید AstConst{rulep->fileline(), 1};
                    randcasep->addItemsp(جدید AstCaseItem{rulep->fileline(), weightp, itemsp});
                }
            }
            m_jumpBlockp->addStmtsp(randcasep);
            iterateAndNextNull(randcasep);
        }

        // V3Task needs to know if we ended up making a recursive function
        m_prodFuncp->foreach([&](Ast_گره_FTask_Ref* ثابت nodep) {
            اگر (nodep->taskp() == m_prodFuncp) m_prodFuncp->recursive(درست);
        });
        UINFOTREE(9, m_prodFuncp, "rsprod-task-done " << nodep, "-");

        // Done with production, should have moved everything to new task
        VL_DO_DANGLING(افزودن_Deletep(nodep->unlinkFrBack()), nodep);
    }
    تهی visit(AstRSRule* nodep) override {
        // Rule of what to produce (as a list) under a production
        nodep->v3fatalSrc("randsequence rule should be iterated and deleted by AstRSProd");
    }
    تهی visit(AstRSProdList* nodep) override {
        // List of productions to call, potentially with a weight
        UINFO(9, "visit " << nodep);
        UASSERT_OBJ(m_prodFuncp, nodep, "RSProdList not under production");

        // Move prodlist to parent (m_prodFunc) and process
        اگر (Ast_گره* ثابت itemsp = nodep->prodsp()) {
            UASSERT_OBJ(!nodep->randJoin(), nodep,
                        "rand join should have been handled in visit(RSRule)");
            nodep->replaceWith(itemsp->unlinkFrBackWithNext());
            // Will soon iterate itemsp as part of normal visit process
            // These will become sequential in the generated task
        } وگرنه {
            nodep->unlinkFrBack();
        }
        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
    }
    تهی visit(AstRSProdItem* nodep) override {
        // "call" to generate using another production
        UINFO(9, "visit " << nodep);
        UASSERT_OBJ(m_prodFuncp, nodep, "RSProdItem not under production");
        AstRSProd* ثابت foundp = nodep->prodp();
        UASSERT_OBJ(foundp, nodep, "Unlinked production reference");
        // Pass through caller-side argument list (IEEE 1800-2023 18.17.7).
        AstArg* ثابت userArgsp
            = nodep->argsp() ? nodep->argsp()->unlinkFrBackWithNext() : nullptr;
        // Convert to task call
        Ast_گره* ثابت newp = newProdFuncRef(nodep, foundp, m_breakVarp, userArgsp);
        // The production might have done a "break;", skip other steps if so
        newp->addNext(جدید AstIf{nodep->fileline(),
                                جدید AstVarRef{nodep->fileline(), m_breakVarp, VAccess::READ},
                                جدید AstJumpGo{nodep->fileline(), m_jumpBlockp}});

        nodep->replaceWith(newp);
        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
    }

    تهی visit(AstRSBreak* nodep) override {
        UASSERT_OBJ(m_jumpBlockp, nodep, "RSBreak not under production jump block");
        UASSERT_OBJ(m_breakVarp, nodep, "no break variable");
        Ast_گره_Stmt* ثابت newp = جدید AstAssign{
            nodep->fileline(), جدید AstVarRef{nodep->fileline(), m_breakVarp, VAccess::WRITE},
            جدید AstConst{nodep->fileline(), AstConst::BitTrue{}}};
        newp->addNext(جدید AstJumpGo{nodep->fileline(), m_jumpBlockp});
        nodep->replaceWith(newp);
        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
    }
    تهی visit(AstRSReturn* nodep) override {
        UASSERT_OBJ(m_jumpBlockp, nodep, "RSReturn not under production jump block");
        nodep->replaceWith(جدید AstJumpGo{nodep->fileline(), m_jumpBlockp});
        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
    }
    تهی visit(AstVarRef* nodep) override {
        // Change references to variables inside randsequence to references to
        // ref variables on the new task
        ثابت auto it = m_localizeRemaps.find(nodep->varp());
        اگر (it != m_localizeRemaps.end()) {
            AstVar* ثابت iovarp = it->second;
            UINFO(9, "VARREF remap " << nodep << " -> " << iovarp);
            nodep->varp(iovarp);
        }
    }

    تهی visit(Ast_گره* nodep) override { iterateChildren(nodep); }

عمومی:
    // CONSTRUCTORS
    explicit RandSequenceVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~RandSequenceVisitor() override = پیشفرض;
};

//######################################################################
// RandSequence method class functions

تهی V3RandSequence::randSequenceNetlist(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { ثابت RandSequenceVisitor randSequenceVisitor{nodep}; }
    V3Global::dump_Check_Global_درخت("randsequence", 0, dump_درخت_Either_Level() >= 3);
}
