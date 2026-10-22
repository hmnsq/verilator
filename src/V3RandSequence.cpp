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

#অন্তর্ভাৱ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#অন্তর্ভাৱ "verilatedos.h"

#অন্তর্ভাৱ "V3RandSequence.h"

#অন্তর্ভাৱ "V3Ast.h"
#অন্তর্ভাৱ "V3Error.h"
#অন্তর্ভাৱ "V3FileLine.h"
#অন্তর্ভাৱ "V3Global.h"

VL_DEFINE_DEBUG_FUNCTIONS;

// ######################################################################
//  Matcher classes (for suggestion matching)

শ্ৰেণী RSProdMatcher final : সার্ৱজনিক VNodeMatcher {
সার্ৱজনিক:
    তার্কিক গ্ৰন্থি_Match(ধ্রুৱ Ast_গ্ৰন্থি* nodep) ধ্রুৱ override { ঘূৰাই VN_IS(nodep, RSProd); }
};

//######################################################################
// Visitor that marks classes needing a randomize() method

শ্ৰেণী RandSequenceVisitor final : সার্ৱজনিক VNVisitor {
    // STATE - global
    std::map<AstRSProd*, Ast_গ্ৰন্থি_FTask*> m_prodFuncps;  // Generated production functions

    // STATE - for current visit position (use VL_RESTORER)
    Ast_গ্ৰন্থি_Module* m_modp = nullptr;  // Current module
    AstRandSequence* m_rsp = nullptr;  // Current randsequence
    AstRSProd* m_startProdp = nullptr;  // Starting production
    Ast_গ্ৰন্থি_FTask* m_prodFuncp = nullptr;  // Production function being built
    AstJumpBlock* m_jumpBlockp = nullptr;  // Building jumpblock for return/break
    AstVar* m_breakVarp = nullptr;  // Break variable
    std::unordered_set<AstVar*> m_localizes;  // Variables to become function inouts
    std::map<std::string, AstVar*> m_localizeNames;  // Ordered names of function inouts
    std::unordered_map<ধ্রুৱ AstVar*, AstVar*> m_localizeRemaps;  // New ports for old vars

    // METHODS
    শূন্য findLocalizes(AstRandSequence* nodep) {
        std::set<AstVar*> localVars;
        nodep->foreach([&](Ast_গ্ৰন্থি* ধ্রুৱ nodep) {
            যদি (ধ্রুৱ AstVarRef* ধ্রুৱ anodep = VN_CAST(nodep, VarRef)) {
                m_localizes.emplace(anodep->varp());
            } অন্যথা যদি (AstVar* ধ্রুৱ anodep = VN_CAST(nodep, Var)) {
                localVars.emplace(anodep);
            }
        });
        // Clear every variable allocated in the RandSequence
        বাবে (AstVar* varp : localVars) m_localizes.erase(varp);
        // Sort by name, so function arguments are stable
        বাবে (AstVar* varp : m_localizes) {
            UINFO(9, "localize " << varp);
            m_localizeNames.emplace(varp->name(), varp);
        }
    }

    AstVar* newBreakVar(File_সাৰি* fl, তার্কিক asOutput) {
        AstVar* ধ্রুৱ varp = নূতন AstVar{fl, VVarType::PORT, "__VrsBreak", VFlagBitPacked{}, 1};
        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        varp->funcLocal(সঁচা);
        যদি (asOutput) varp->direction(VDirection::OUTPUT);
        Ast_গ্ৰন্থি* outp = varp;
        outp->addNext(নূতন AstAssign{fl, নূতন AstVarRef{fl, varp, VAccess::WRITE},
                                    নূতন AstConst{fl, AstConst::BitFalse{}}});

        // Also add arguments as next's
        বাবে (ধ্রুৱ auto& itr : m_localizeNames) {
            ধ্রুৱ AstVar* ধ্রুৱ lvarp = itr.second;
            AstVar* ধ্রুৱ iovarp
                = নূতন AstVar{fl, VVarType::PORT, "__Vrsarg_" + lvarp->name(), lvarp};
            iovarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            iovarp->funcLocal(সঁচা);
            iovarp->direction(VDirection::REF);
            varp->addNext(iovarp);
            m_localizeRemaps.emplace(lvarp, iovarp);
        }
        ঘূৰাই varp;
    }

    AstTask* newStartFunc(AstRandSequence* nodep) {
        // Create wrapper that is called by the sequence
        AstTask* ধ্রুৱ taskp = নূতন AstTask{m_startProdp->fileline(), m_rsp->name(), nullptr};
        taskp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        m_modp->addStmtsp(taskp);

        // Create local (not output) break variable
        VL_RESTORER_COPY(m_localizeRemaps);
        AstVar* breakVarp = newBreakVar(nodep->fileline(), মিছা);
        taskp->addStmtsp(breakVarp);

        // Call the start production's task
        taskp->addStmtsp(newProdFuncRef(nodep, m_startProdp, breakVarp, nullptr));

        UINFOTREE(9, taskp, "newStart", "");
        ঘূৰাই taskp;
    }

    Ast_গ্ৰন্থি* newProdFuncRef(Ast_গ্ৰন্থি* nodep, AstRSProd* prodp, AstVar* breakVarp,
                            AstArg* userArgsp) {
        auto it = m_prodFuncps.find(prodp);
        UASSERT_OBJ(it != m_prodFuncps.end(), nodep, "No production function made");
        Ast_গ্ৰন্থি_FTask* ধ্রুৱ prodFuncp = it->second;
        File_সাৰি* ধ্রুৱ fl = nodep->fileline();
        // V3Width already ran before V3RandSequence, so VarRefs we create here
        // need dtype set explicitly, V3Broken later checks width == widthMin.
        AstVarRef* ধ্রুৱ breakRefp = নূতন AstVarRef{fl, breakVarp, VAccess::WRITE};
        breakRefp->dtypeFrom(breakVarp);
        AstArg* ধ্রুৱ argsp = নূতন AstArg{fl, breakVarp->name(), breakRefp};
        বাবে (ধ্রুৱ auto& itr : m_localizeNames) {
            ধ্রুৱ AstVar* ধ্রুৱ lvarp = itr.second;
            AstVar* ধ্রুৱ iovarp = m_localizeRemaps[lvarp];
            UASSERT_OBJ(iovarp, nodep, "No new port variable for local variable" << lvarp);
            AstVarRef* ধ্রুৱ refp = নূতন AstVarRef{fl, iovarp, VAccess::READWRITE};
            refp->dtypeFrom(iovarp);
            argsp->addNext(নূতন AstArg{nodep->fileline(), "__Vrsarg_" + lvarp->name(), refp});
        }
        যদি (userArgsp) argsp->addNext(userArgsp);
        Ast_গ্ৰন্থি* ধ্রুৱ newp
            = নূতন Ast_Stmt_ৰাশিমালা{fl, নূতন AstTaskRef{fl, VN_AS(prodFuncp, Task), argsp}};
        ঘূৰাই newp;
    }

    শূন্য newProdFunc(AstRSProd* nodep) {
        // Task, as time may pass
        AstTask* ধ্রুৱ newp
            = নূতন AstTask{nodep->fileline(), m_rsp->name() + "_" + nodep->name(), nullptr};
        m_modp->addStmtsp(newp);
        m_prodFuncps.emplace(nodep, newp);
        UINFOTREE(9, newp, "newProd", "");
    }

    Ast_গ্ৰন্থি* newProdRandJoin(AstRSProd* /*prodp*/, AstRSProdList* prodlistp) {
        // For weight == 1.0 longer sequence (favor stay in a)
        // For weight == 0.0 shorter squence (favor change a/b)
        UASSERT_OBJ(prodlistp->weightp(), prodlistp, "Weight should have default CONST(0.5)");
        Ast_গ্ৰন্থি_ৰাশিমালা* weightp = prodlistp->weightp()->unlinkFrBack();

        // Build map of production lists and productions we need to join
        // Must mainain the relative order of each sequence.
        std::deque<AstRSProdItem*> lists;
        std::unordered_map<AstRSProdItem*, std::deque<Ast_গ্ৰন্থি_Stmt*>> listStmts;
        বাবে (AstRSProdItem* proditemp = VN_AS(prodlistp->prodsp(), RSProdItem); proditemp;
             proditemp = VN_AS(proditemp->nextp(), RSProdItem)) {
            lists.ঠেলো_back(proditemp);
            ধ্রুৱ AstRSProd* ধ্রুৱ subProdp = proditemp->prodp();
            যদি (!subProdp) অনুৱর্ত;
            যদি (!subProdp->rulesp()) অনুৱর্ত;
            যদি (!subProdp->rulesp()->prodlistsp()) অনুৱর্ত;
            বাবে (Ast_গ্ৰন্থি_Stmt* subStmtp
                 = VN_AS(subProdp->rulesp()->prodlistsp()->prodsp(), গ্ৰন্থি_Stmt);
                 subStmtp; subStmtp = VN_AS(subStmtp->nextp(), গ্ৰন্থি_Stmt)) {
                listStmts[proditemp].ঠেলো_back(subStmtp);
            }
        }

        UINFO(9, "RandJoin productions called:");
        বাবে (AstRSProdItem* proditemp : lists) {
            UINFO(9, "  list " << proditemp);
            বাবে (ধ্রুৱ Ast_গ্ৰন্থি_Stmt* prodip : listStmts[proditemp])
                UINFO(9, "    calls " << prodip);
        }

        // Need to clone all nodes used

        File_সাৰি* fl = prodlistp->fileline();
        Ast_গ্ৰন্থি* newp = nullptr;

        // "bool _Vjoin_repick = true;"  // Pick a new longer sequence
        AstVar* repickVarp = নূতন AstVar{fl, VVarType::VAR, "_Vjoin_repick", VFlagBitPacked{}, 1};
        repickVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        repickVarp->funcLocal(সঁচা);
        m_breakVarp->addNextHere(repickVarp);  // Add to function top, not newp
        newp = Ast_গ্ৰন্থি::addNext(newp,
                                নূতন AstAssign{fl, নূতন AstVarRef{fl, repickVarp, VAccess::WRITE},
                                              নূতন AstConst{fl, AstConst::BitTrue{}}});

        // "int _Vjoin_nleft_0 = N(items);"  // N(a) Number left to generate in list 0, starts at
        // N(list0)
        std::deque<AstVar*> nleftVarps;
        পূর্ণাঙ্ক i = 0;
        বাবে (AstRSProdItem* proditemp : lists) {
            // 'rand join arule arule arule' is legal, so need numbered, not non-unique named
            // nleft's
            AstVar* ধ্রুৱ varp = নূতন AstVar{
                fl, VVarType::VAR, "_Vjoin_nleft_" + std::to_string(i++), VFlagBitPacked{}, 32};
            varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            varp->funcLocal(সঁচা);
            m_breakVarp->addNextHere(varp);  // Add to function top, not newp
            nleftVarps.ঠেলো_back(varp);

            newp = Ast_গ্ৰন্থি::addNext(
                newp,
                নূতন AstAssign{fl, নূতন AstVarRef{fl, varp, VAccess::WRITE},
                              নূতন AstConst{fl, AstConst::Widthed_মান{}, 32,
                                           static_cast<uint32_t>(listStmts[proditemp].size())}});
        }

        // "int _Vjoin_picked_elist = 0;"  // 0 = pick first eligible (non-finished) list, 1 = pick
        // second, etc
        AstVar* ধ্রুৱ pickedVarp
            = নূতন AstVar{fl, VVarType::VAR, "_Vjoin_picked_elist", VFlagBitPacked{}, 1};
        pickedVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        pickedVarp->funcLocal(সঁচা);
        m_breakVarp->addNextHere(pickedVarp);  // Add to function top, not newp

        // "while(1) {"  // Generate until make complete sequence
        AstJumpBlock* ধ্রুৱ whilep = নূতন AstJumpBlock{fl, nullptr};
        newp = Ast_গ্ৰন্থি::addNext(newp, নূতন AstLoop{fl, whilep});
        {
            // "if ((std::rand() & 0xffffUL) > (0xffff * weight)) _Vjoin_repick = true;"
            whilep->addStmtsp(নূতন AstIf{
                fl,
                নূতন AstGt{
                    fl, নূতন AstRand{fl, AstRand::SRandomU32{}},
                    নূতন AstRToIS{fl, নূতন AstMulD{fl,
                                                 নূতন AstConst{fl, AstConst::RealDouble{},
                                                              static_cast<দ্ৱিগুণ>(0xffffffffUL)},
                                                 weightp}}},
                নূতন AstAssign{fl, নূতন AstVarRef{fl, repickVarp, VAccess::WRITE},
                              নূতন AstConst{fl, AstConst::BitTrue{}}}});

            // "if (_Vjoin_repick) {"
            AstIf* ifp = নূতন AstIf{fl, নূতন AstVarRef{fl, repickVarp, VAccess::READ}};
            whilep->addStmtsp(ifp);
            {
                // "// _Vjoin_nlists_eligible is how many lists eligable for picking"
                // "int _Vjoin_nlists_eligible = 0;"
                AstVar* ধ্রুৱ eligibleVarp = নূতন AstVar{
                    fl, VVarType::VAR, "_Vjoin_nlists_eligible", VFlagBitPacked{}, 32};
                eligibleVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                eligibleVarp->funcLocal(সঁচা);
                m_breakVarp->addNextHere(eligibleVarp);  // Add to function top, not newp
                ifp->addThensp(নূতন AstAssign{fl, নূতন AstVarRef{fl, eligibleVarp, VAccess::WRITE},
                                             নূতন AstConst{fl, AstConst::Widthed_মান{}, 32, 0}});

                i = 0;
                বাবে (AstRSProdItem* proditemp : lists) {
                    (শূন্য)proditemp;
                    // "_Vjoin_nlists_eligible += _Vjoin_nleft_0 ? 1 : 0;"
                    ifp->addThensp(নূতন AstAssign{
                        fl, নূতন AstVarRef{fl, eligibleVarp, VAccess::WRITE},
                        নূতন AstAdd{
                            fl, নূতন AstVarRef{fl, eligibleVarp, VAccess::READ},
                            নূতন AstCond{
                                fl,
                                নূতন AstNeq{fl, নূতন AstConst{fl, AstConst::Widthed_মান{}, 32, 0},
                                           নূতন AstVarRef{fl, nleftVarps[i], VAccess::READ}},
                                নূতন AstConst{fl, AstConst::Widthed_মান{}, 32, 1},
                                নূতন AstConst{fl, AstConst::Widthed_মান{}, 32, 0}}}});
                    ++i;
                }

                // "if (!_Vjoin_nlists_eligible) break;"  // Out of elements
                ifp->addThensp(
                    নূতন AstIf{fl,
                              নূতন AstEq{fl, নূতন AstConst{fl, AstConst::Widthed_মান{}, 32, 0},
                                        নূতন AstVarRef{fl, eligibleVarp, VAccess::READ}},
                              নূতন AstJumpGo{fl, m_jumpBlockp}});

                // "// +1 to simplify usage, so 0 = done"
                // "_Vjoin_picked_elist = 1 + (std::rand() % _Vjoin_nlists_eligible);"
                ifp->addThensp(নূতন AstAssign{
                    fl, নূতন AstVarRef{fl, pickedVarp, VAccess::WRITE},
                    নূতন AstAdd{fl, নূতন AstConst{fl, AstConst::Widthed_মান{}, 32, 1},
                               নূতন AstModDiv{fl, নূতন AstRand{fl, AstRand::SRandomU32{}},
                                             নূতন AstVarRef{fl, eligibleVarp, VAccess::READ}}}});

                // "_Vjoin_repick = false;
                ifp->addThensp(নূতন AstAssign{fl, নূতন AstVarRef{fl, repickVarp, VAccess::WRITE},
                                             নূতন AstConst{fl, AstConst::BitFalse{}}});
            }

            // "// Number of eligible lists left before hit _Vjoin_picked one"
            // "int _Vjoin_sel_elist = _Vjoin_picked_elist;"
            AstVar* ধ্রুৱ selVarp
                = নূতন AstVar{fl, VVarType::VAR, "_Vjoin_sel_elist", VFlagBitPacked{}, 32};
            selVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            selVarp->funcLocal(সঁচা);
            m_breakVarp->addNextHere(selVarp);  // Add to function top, not newp
            whilep->addStmtsp(নূতন AstAssign{fl, নূতন AstVarRef{fl, selVarp, VAccess::WRITE},
                                            নূতন AstVarRef{fl, pickedVarp, VAccess::READ}});

            i = -1;
            বাবে (AstRSProdItem* proditemp : lists) {
                ++i;
                // "if (_Vjoin_nleft_0) {"
                ifp = নূতন AstIf{fl,
                                নূতন AstNeq{fl, নূতন AstConst{fl, AstConst::Widthed_মান{}, 32, 0},
                                           নূতন AstVarRef{fl, nleftVarps[i], VAccess::READ}},
                                nullptr, nullptr};
                whilep->addStmtsp(ifp);
                {
                    // "--_Vjoin_sel_elist;"
                    ifp->addThensp(নূতন AstAssign{
                        fl, নূতন AstVarRef{fl, selVarp, VAccess::WRITE},
                        নূতন AstSub{fl, নূতন AstVarRef{fl, selVarp, VAccess::READ},
                                   নূতন AstConst{fl, AstConst::Widthed_মান{}, 32, 1}}});

                    // "if (_Vjoin_sel_elist == 0) {"
                    AstIf* ধ্রুৱ jIfp = নূতন AstIf{
                        fl,
                        নূতন AstEq{fl, নূতন AstConst{fl, AstConst::Widthed_মান{}, 32, 0},
                                  নূতন AstVarRef{fl, selVarp, VAccess::READ}},
                        nullptr, nullptr};
                    ifp->addThensp(jIfp);
                    {
                        // "switch (_Vjoin_nleft_0) {"
                        //     "case 2 / * N(a)  * /: {statement}; break;"
                        //     "case 1 / * N(a) - 1 * /: {statement}; break;"
                        uint32_t j = static_cast<uint32_t>(listStmts[proditemp].size());
                        বাবে (Ast_গ্ৰন্থি_Stmt* iprodp : listStmts[proditemp]) {
                            jIfp->addThensp(নূতন AstIf{
                                fl,
                                নূতন AstEq{fl, নূতন AstConst{fl, AstConst::Widthed_মান{}, 32, j},
                                          নূতন AstVarRef{fl, nleftVarps[i], VAccess::READ}},
                                iprodp->clone_বৃক্ষ(মিছা)});
                            --j;
                        }

                        // "--_Vjoin_nleft_0;"
                        jIfp->addThensp(নূতন AstAssign{
                            fl, নূতন AstVarRef{fl, nleftVarps[i], VAccess::WRITE},
                            নূতন AstSub{fl, নূতন AstVarRef{fl, nleftVarps[i], VAccess::READ},
                                       নূতন AstConst{fl, AstConst::Widthed_মান{}, 32, 1}}});

                        // "if (0 == _Vjoin_nleft_0) _Vjoin_repick = true;"
                        jIfp->addThensp(নূতন AstIf{
                            fl,
                            নূতন AstEq{fl, নূতন AstConst{fl, AstConst::Widthed_মান{}, 32, 0},
                                      নূতন AstVarRef{fl, nleftVarps[i], VAccess::READ}},
                            নূতন AstAssign{fl, নূতন AstVarRef{fl, repickVarp, VAccess::WRITE},
                                          নূতন AstConst{fl, AstConst::BitTrue{}}}});

                        // "continue;"
                        jIfp->addThensp(নূতন AstJumpGo{fl, whilep});
                    }
                }
            }
        }

        UINFOTREE(9, newp, "ForkJoin new", "-");
        ঘূৰাই newp;
    }

    // VISITORS

    শূন্য visit(Ast_গ্ৰন্থি_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildren(nodep);
    }

    শূন্য visit(AstRandSequence* nodep) override {
        UINFO(9, "visit " << nodep);
        UASSERT_OBJ(m_modp, nodep, "randsequence not under module");
        যদি (m_rsp) {
            // No examples found in the wild
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: randsequence under randsequence");
            VL_DO_DANGLING(ঠেলো_Deletep(nodep->unlinkFrBack()), nodep);
            ঘূৰাই;
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
        যদি (nodep->prodp()) m_startProdp = nodep->prodp();  // Has a start production selected

        // Build production functions
        বাবে (AstRSProd* prodp = nodep->prodsp(); prodp; prodp = VN_AS(prodp->nextp(), RSProd)) {
            newProdFunc(prodp);
            // Don't iterate yet, need to have tasks we can reference
        }

        // Iterate all of the new productions, moving guts into functions just made
        // Must do those with 'rand join' first, as they need to see the raw AstRS* nodes
        // before they are removed.  (Alternative would be to keep them until the end).
        UINFOTREE(9, nodep, "RS Tree pre-it", "-");
        std::unordered_set<AstRSProd*> prodHasRandJoin;
        বাবে (AstRSProd* prodp = nodep->prodsp(); prodp; prodp = VN_AS(prodp->nextp(), RSProd)) {
            prodp->foreach([&](ধ্রুৱ AstRSProdList* ধ্রুৱ prodlistp) {
                যদি (prodlistp->randJoin()) prodHasRandJoin.emplace(prodp);
            });
        }
        বাবে (AstRSProd *nextp, *prodp = nodep->prodsp(); prodp; prodp = nextp) {
            nextp = VN_AS(prodp->nextp(), RSProd);
            যদি (prodHasRandJoin.গুনতি(prodp)) VL_DO_DANGLING(iterate(prodp), prodp);
        }
        বাবে (AstRSProd *nextp, *prodp = nodep->prodsp(); prodp; prodp = nextp) {
            nextp = VN_AS(prodp->nextp(), RSProd);
            যদি (!prodHasRandJoin.গুনতি(prodp)) VL_DO_DANGLING(iterate(prodp), prodp);
        }
        UINFOTREE(9, nodep, "RS Tree post-it", "-");

        // Replace randsequence with call to randsequence start function
        AstTask* ধ্রুৱ startFuncp = newStartFunc(nodep);
        AstArg* argsp = nullptr;
        বাবে (auto& itr : m_localizeNames) {
            AstVar* ধ্রুৱ lvarp = itr.second;
            argsp = Ast_গ্ৰন্থি::addNext(
                argsp, নূতন AstArg{nodep->fileline(), "__Vrsarg_" + lvarp->name(),
                                  নূতন AstVarRef{nodep->fileline(), lvarp, VAccess::READWRITE}});
        }
        AstTaskRef* ধ্রুৱ callStartp
            = নূতন AstTaskRef{nodep->fileline(), startFuncp->name(), argsp};
        callStartp->taskp(startFuncp);
        nodep->replaceWith(নূতন Ast_Stmt_ৰাশিমালা{nodep->fileline(), callStartp});

        VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
    }
    শূন্য visit(AstRSProd* nodep) override {
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
        m_breakVarp = newBreakVar(nodep->fileline(), সঁচা);
        m_prodFuncp->addStmtsp(m_breakVarp);

        // Production formal ports become real input ports on the generated task.
        // Must be added after m_breakVarp / localize ports so positional argument
        // order in newProdFuncRef stays consistent: [break, localize..., user...].
        যদি (Ast_গ্ৰন্থি* ধ্রুৱ portsp = nodep->portsp()) {
            portsp->unlinkFrBackWithNext();
            m_prodFuncp->addStmtsp(portsp);
            বাবে (Ast_গ্ৰন্থি* itp = portsp; itp; itp = itp->nextp()) {
                AstVar* ধ্রুৱ portVarp = VN_AS(itp, Var);
                portVarp->funcLocal(সঁচা);
                portVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            }
        }

        // Put JumpBlock immediately under the new function to support
        // a future break/return.  V3Const will rip it out if unneeded.
        VL_RESTORER(m_jumpBlockp);
        m_jumpBlockp = নূতন AstJumpBlock{nodep->fileline(), nullptr};
        m_prodFuncp->addStmtsp(m_jumpBlockp);

        যদি (nodep->fvarp())
            nodep->fvarp()->v3warn(E_UNSUPPORTED,
                                   "Unsupported: randsequence production function variable");

        // Move children into m_prodFuncp, and iterate there
        যদি (!nodep->rulesp()) {  // Nothing to do
        } অন্যথা যদি (nodep->rulesp()->prodlistsp() && nodep->rulesp()->prodlistsp()->randJoin()) {
            AstRSProdList* ধ্রুৱ prodlistp = nodep->rulesp()->prodlistsp();
            Ast_গ্ৰন্থি* ধ্রুৱ itemsp = newProdRandJoin(nodep, prodlistp);
            m_jumpBlockp->addStmtsp(itemsp);
            iterateAndNextNull(itemsp);
        } অন্যথা যদি (!nodep->rulesp()->nextp()) {  // Single rule/list, can just do it
            // RSPROD(RSRULE(weight, stmt)) -> IF(weight != 0, stmt)
            ধ্রুৱ AstRSRule* ধ্রুৱ rulep = nodep->rulesp();
            Ast_গ্ৰন্থি* itemsp = nullptr;
            যদি (rulep->weightStmtsp()) itemsp = rulep->weightStmtsp()->unlinkFrBackWithNext();
            যদি (rulep->prodlistsp())
                itemsp = Ast_গ্ৰন্থি::addNext(itemsp, rulep->prodlistsp()->unlinkFrBackWithNext());
            // Can ignore rulep->weightp() unless is zero, in which case noop, add if
            যদি (rulep->weightp())
                itemsp = নূতন AstIf{rulep->weightp()->fileline(),
                                   নূতন AstNeq{rulep->weightp()->fileline(),
                                              নূতন AstConst{rulep->weightp()->fileline(), 0},
                                              rulep->weightp()->unlinkFrBack()},
                                   itemsp};
            // Move prodlist to parent (m_prodFunc) and process
            যদি (itemsp) {
                UINFOTREE(9, itemsp, "additems", "-");
                m_jumpBlockp->addStmtsp(itemsp);
                iterateAndNextNull(itemsp);
                UINFOTREE(9, m_jumpBlockp, "jumpBlockNow", "-");
            }
        } অন্যথা {
            // List of rules with "OR", need to random-weight them
            // RSPROD(RSRULE(weight1, stmt!), RSRULE(weight2, stmt2))
            //   -> RANDCASE(CASEITEM(weight1, stmt1), CASEITEM(weight2, stmt2))
            AstRandCase* ধ্রুৱ randcasep = নূতন AstRandCase{nodep->fileline(), nullptr};
            বাবে (AstRSRule* rulep = nodep->rulesp(); rulep;
                 rulep = VN_AS(rulep->nextp(), RSRule)) {
                Ast_গ্ৰন্থি* itemsp = nullptr;
                যদি (rulep->weightStmtsp()) itemsp = rulep->weightStmtsp()->unlinkFrBackWithNext();
                যদি (rulep->prodlistsp())
                    itemsp = Ast_গ্ৰন্থি::addNext(itemsp, rulep->prodlistsp()->unlinkFrBackWithNext());
                যদি (itemsp) {
                    Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ weightp = rulep->weightp()
                                                     ? rulep->weightp()->unlinkFrBack()
                                                     : নূতন AstConst{rulep->fileline(), 1};
                    randcasep->addItemsp(নূতন AstCaseItem{rulep->fileline(), weightp, itemsp});
                }
            }
            m_jumpBlockp->addStmtsp(randcasep);
            iterateAndNextNull(randcasep);
        }

        // V3Task needs to know if we ended up making a recursive function
        m_prodFuncp->foreach([&](Ast_গ্ৰন্থি_FTask_Ref* ধ্রুৱ nodep) {
            যদি (nodep->taskp() == m_prodFuncp) m_prodFuncp->recursive(সঁচা);
        });
        UINFOTREE(9, m_prodFuncp, "rsprod-task-done " << nodep, "-");

        // Done with production, should have moved everything to new task
        VL_DO_DANGLING(ঠেলো_Deletep(nodep->unlinkFrBack()), nodep);
    }
    শূন্য visit(AstRSRule* nodep) override {
        // Rule of what to produce (as a list) under a production
        nodep->v3fatalSrc("randsequence rule should be iterated and deleted by AstRSProd");
    }
    শূন্য visit(AstRSProdList* nodep) override {
        // List of productions to call, potentially with a weight
        UINFO(9, "visit " << nodep);
        UASSERT_OBJ(m_prodFuncp, nodep, "RSProdList not under production");

        // Move prodlist to parent (m_prodFunc) and process
        যদি (Ast_গ্ৰন্থি* ধ্রুৱ itemsp = nodep->prodsp()) {
            UASSERT_OBJ(!nodep->randJoin(), nodep,
                        "rand join should have been handled in visit(RSRule)");
            nodep->replaceWith(itemsp->unlinkFrBackWithNext());
            // Will soon iterate itemsp as part of normal visit process
            // These will become sequential in the generated task
        } অন্যথা {
            nodep->unlinkFrBack();
        }
        VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
    }
    শূন্য visit(AstRSProdItem* nodep) override {
        // "call" to generate using another production
        UINFO(9, "visit " << nodep);
        UASSERT_OBJ(m_prodFuncp, nodep, "RSProdItem not under production");
        AstRSProd* ধ্রুৱ foundp = nodep->prodp();
        UASSERT_OBJ(foundp, nodep, "Unlinked production reference");
        // Pass through caller-side argument list (IEEE 1800-2023 18.17.7).
        AstArg* ধ্রুৱ userArgsp
            = nodep->argsp() ? nodep->argsp()->unlinkFrBackWithNext() : nullptr;
        // Convert to task call
        Ast_গ্ৰন্থি* ধ্রুৱ newp = newProdFuncRef(nodep, foundp, m_breakVarp, userArgsp);
        // The production might have done a "break;", skip other steps if so
        newp->addNext(নূতন AstIf{nodep->fileline(),
                                নূতন AstVarRef{nodep->fileline(), m_breakVarp, VAccess::READ},
                                নূতন AstJumpGo{nodep->fileline(), m_jumpBlockp}});

        nodep->replaceWith(newp);
        VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
    }

    শূন্য visit(AstRSBreak* nodep) override {
        UASSERT_OBJ(m_jumpBlockp, nodep, "RSBreak not under production jump block");
        UASSERT_OBJ(m_breakVarp, nodep, "no break variable");
        Ast_গ্ৰন্থি_Stmt* ধ্রুৱ newp = নূতন AstAssign{
            nodep->fileline(), নূতন AstVarRef{nodep->fileline(), m_breakVarp, VAccess::WRITE},
            নূতন AstConst{nodep->fileline(), AstConst::BitTrue{}}};
        newp->addNext(নূতন AstJumpGo{nodep->fileline(), m_jumpBlockp});
        nodep->replaceWith(newp);
        VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
    }
    শূন্য visit(AstRSReturn* nodep) override {
        UASSERT_OBJ(m_jumpBlockp, nodep, "RSReturn not under production jump block");
        nodep->replaceWith(নূতন AstJumpGo{nodep->fileline(), m_jumpBlockp});
        VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
    }
    শূন্য visit(AstVarRef* nodep) override {
        // Change references to variables inside randsequence to references to
        // ref variables on the new task
        ধ্রুৱ auto it = m_localizeRemaps.find(nodep->varp());
        যদি (it != m_localizeRemaps.end()) {
            AstVar* ধ্রুৱ iovarp = it->second;
            UINFO(9, "VARREF remap " << nodep << " -> " << iovarp);
            nodep->varp(iovarp);
        }
    }

    শূন্য visit(Ast_গ্ৰন্থি* nodep) override { iterateChildren(nodep); }

সার্ৱজনিক:
    // CONSTRUCTORS
    explicit RandSequenceVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~RandSequenceVisitor() override = স্ৱভাৱ;
};

//######################################################################
// RandSequence method class functions

শূন্য V3RandSequence::randSequenceNetlist(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { ধ্রুৱ RandSequenceVisitor randSequenceVisitor{nodep}; }
    V3Global::dump_Check_Global_বৃক্ষ("randsequence", 0, dump_বৃক্ষ_Either_Level() >= 3);
}
