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

#അന്തര്ഭാവ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#അന്തര്ഭാവ "verilatedos.h"

#അന്തര്ഭാവ "V3RandSequence.h"

#അന്തര്ഭാവ "V3Ast.h"
#അന്തര്ഭാവ "V3Error.h"
#അന്തര്ഭാവ "V3FileLine.h"
#അന്തര്ഭാവ "V3Global.h"

VL_DEFINE_DEBUG_FUNCTIONS;

// ######################################################################
//  Matcher classes (for suggestion matching)

വർഗ്ഗം RSProdMatcher final : പൊതു VNodeMatcher {
പൊതു:
    തർക്കം കണ്ണി_Match(സ്ഥിരം Ast_കണ്ണി* nodep) സ്ഥിരം override { തിരികെ VN_IS(nodep, RSProd); }
};

//######################################################################
// Visitor that marks classes needing a randomize() method

വർഗ്ഗം RandSequenceVisitor final : പൊതു VNVisitor {
    // STATE - global
    std::map<AstRSProd*, Ast_കണ്ണി_FTask*> m_prodFuncps;  // Generated production functions

    // STATE - for current visit position (use VL_RESTORER)
    Ast_കണ്ണി_Module* m_modp = nullptr;  // Current module
    AstRandSequence* m_rsp = nullptr;  // Current randsequence
    AstRSProd* m_startProdp = nullptr;  // Starting production
    Ast_കണ്ണി_FTask* m_prodFuncp = nullptr;  // Production function being built
    AstJumpBlock* m_jumpBlockp = nullptr;  // Building jumpblock for return/break
    AstVar* m_breakVarp = nullptr;  // Break variable
    std::unordered_set<AstVar*> m_localizes;  // Variables to become function inouts
    std::map<std::string, AstVar*> m_localizeNames;  // Ordered names of function inouts
    std::unordered_map<സ്ഥിരം AstVar*, AstVar*> m_localizeRemaps;  // New ports for old vars

    // METHODS
    ശൂന്യം findLocalizes(AstRandSequence* nodep) {
        std::set<AstVar*> localVars;
        nodep->foreach([&](Ast_കണ്ണി* സ്ഥിരം nodep) {
            എങ്കിൽ (സ്ഥിരം AstVarRef* സ്ഥിരം anodep = VN_CAST(nodep, VarRef)) {
                m_localizes.emplace(anodep->varp());
            } അല്ലെങ്കിൽ എങ്കിൽ (AstVar* സ്ഥിരം anodep = VN_CAST(nodep, Var)) {
                localVars.emplace(anodep);
            }
        });
        // Clear every variable allocated in the RandSequence
        വേണ്ടി (AstVar* varp : localVars) m_localizes.erase(varp);
        // Sort by name, so function arguments are stable
        വേണ്ടി (AstVar* varp : m_localizes) {
            UINFO(9, "localize " << varp);
            m_localizeNames.emplace(varp->name(), varp);
        }
    }

    AstVar* newBreakVar(File_വരി* fl, തർക്കം asOutput) {
        AstVar* സ്ഥിരം varp = പുതിയ AstVar{fl, VVarType::PORT, "__VrsBreak", VFlagBitPacked{}, 1};
        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        varp->funcLocal(സത്യം);
        എങ്കിൽ (asOutput) varp->direction(VDirection::OUTPUT);
        Ast_കണ്ണി* outp = varp;
        outp->addNext(പുതിയ AstAssign{fl, പുതിയ AstVarRef{fl, varp, VAccess::WRITE},
                                    പുതിയ AstConst{fl, AstConst::BitFalse{}}});

        // Also add arguments as next's
        വേണ്ടി (സ്ഥിരം auto& itr : m_localizeNames) {
            സ്ഥിരം AstVar* സ്ഥിരം lvarp = itr.second;
            AstVar* സ്ഥിരം iovarp
                = പുതിയ AstVar{fl, VVarType::PORT, "__Vrsarg_" + lvarp->name(), lvarp};
            iovarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            iovarp->funcLocal(സത്യം);
            iovarp->direction(VDirection::REF);
            varp->addNext(iovarp);
            m_localizeRemaps.emplace(lvarp, iovarp);
        }
        തിരികെ varp;
    }

    AstTask* newStartFunc(AstRandSequence* nodep) {
        // Create wrapper that is called by the sequence
        AstTask* സ്ഥിരം taskp = പുതിയ AstTask{m_startProdp->fileline(), m_rsp->name(), nullptr};
        taskp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        m_modp->addStmtsp(taskp);

        // Create local (not output) break variable
        VL_RESTORER_COPY(m_localizeRemaps);
        AstVar* breakVarp = newBreakVar(nodep->fileline(), അസത്യം);
        taskp->addStmtsp(breakVarp);

        // Call the start production's task
        taskp->addStmtsp(newProdFuncRef(nodep, m_startProdp, breakVarp, nullptr));

        UINFOTREE(9, taskp, "newStart", "");
        തിരികെ taskp;
    }

    Ast_കണ്ണി* newProdFuncRef(Ast_കണ്ണി* nodep, AstRSProd* prodp, AstVar* breakVarp,
                            AstArg* userArgsp) {
        auto it = m_prodFuncps.find(prodp);
        UASSERT_OBJ(it != m_prodFuncps.end(), nodep, "No production function made");
        Ast_കണ്ണി_FTask* സ്ഥിരം prodFuncp = it->second;
        File_വരി* സ്ഥിരം fl = nodep->fileline();
        // V3Width already ran before V3RandSequence, so VarRefs we create here
        // need dtype set explicitly, V3Broken later checks width == widthMin.
        AstVarRef* സ്ഥിരം breakRefp = പുതിയ AstVarRef{fl, breakVarp, VAccess::WRITE};
        breakRefp->dtypeFrom(breakVarp);
        AstArg* സ്ഥിരം argsp = പുതിയ AstArg{fl, breakVarp->name(), breakRefp};
        വേണ്ടി (സ്ഥിരം auto& itr : m_localizeNames) {
            സ്ഥിരം AstVar* സ്ഥിരം lvarp = itr.second;
            AstVar* സ്ഥിരം iovarp = m_localizeRemaps[lvarp];
            UASSERT_OBJ(iovarp, nodep, "No new port variable for local variable" << lvarp);
            AstVarRef* സ്ഥിരം refp = പുതിയ AstVarRef{fl, iovarp, VAccess::READWRITE};
            refp->dtypeFrom(iovarp);
            argsp->addNext(പുതിയ AstArg{nodep->fileline(), "__Vrsarg_" + lvarp->name(), refp});
        }
        എങ്കിൽ (userArgsp) argsp->addNext(userArgsp);
        Ast_കണ്ണി* സ്ഥിരം newp
            = പുതിയ Ast_Stmt_വ്യഞ്ജകം{fl, പുതിയ AstTaskRef{fl, VN_AS(prodFuncp, Task), argsp}};
        തിരികെ newp;
    }

    ശൂന്യം newProdFunc(AstRSProd* nodep) {
        // Task, as time may pass
        AstTask* സ്ഥിരം newp
            = പുതിയ AstTask{nodep->fileline(), m_rsp->name() + "_" + nodep->name(), nullptr};
        m_modp->addStmtsp(newp);
        m_prodFuncps.emplace(nodep, newp);
        UINFOTREE(9, newp, "newProd", "");
    }

    Ast_കണ്ണി* newProdRandJoin(AstRSProd* /*prodp*/, AstRSProdList* prodlistp) {
        // For weight == 1.0 longer sequence (favor stay in a)
        // For weight == 0.0 shorter squence (favor change a/b)
        UASSERT_OBJ(prodlistp->weightp(), prodlistp, "Weight should have default CONST(0.5)");
        Ast_കണ്ണി_വ്യഞ്ജകം* weightp = prodlistp->weightp()->unlinkFrBack();

        // Build map of production lists and productions we need to join
        // Must mainain the relative order of each sequence.
        std::deque<AstRSProdItem*> lists;
        std::unordered_map<AstRSProdItem*, std::deque<Ast_കണ്ണി_Stmt*>> listStmts;
        വേണ്ടി (AstRSProdItem* proditemp = VN_AS(prodlistp->prodsp(), RSProdItem); proditemp;
             proditemp = VN_AS(proditemp->nextp(), RSProdItem)) {
            lists.തള്ളുക_back(proditemp);
            സ്ഥിരം AstRSProd* സ്ഥിരം subProdp = proditemp->prodp();
            എങ്കിൽ (!subProdp) തുടരുക;
            എങ്കിൽ (!subProdp->rulesp()) തുടരുക;
            എങ്കിൽ (!subProdp->rulesp()->prodlistsp()) തുടരുക;
            വേണ്ടി (Ast_കണ്ണി_Stmt* subStmtp
                 = VN_AS(subProdp->rulesp()->prodlistsp()->prodsp(), കണ്ണി_Stmt);
                 subStmtp; subStmtp = VN_AS(subStmtp->nextp(), കണ്ണി_Stmt)) {
                listStmts[proditemp].തള്ളുക_back(subStmtp);
            }
        }

        UINFO(9, "RandJoin productions called:");
        വേണ്ടി (AstRSProdItem* proditemp : lists) {
            UINFO(9, "  list " << proditemp);
            വേണ്ടി (സ്ഥിരം Ast_കണ്ണി_Stmt* prodip : listStmts[proditemp])
                UINFO(9, "    calls " << prodip);
        }

        // Need to clone all nodes used

        File_വരി* fl = prodlistp->fileline();
        Ast_കണ്ണി* newp = nullptr;

        // "bool _Vjoin_repick = true;"  // Pick a new longer sequence
        AstVar* repickVarp = പുതിയ AstVar{fl, VVarType::VAR, "_Vjoin_repick", VFlagBitPacked{}, 1};
        repickVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        repickVarp->funcLocal(സത്യം);
        m_breakVarp->addNextHere(repickVarp);  // Add to function top, not newp
        newp = Ast_കണ്ണി::addNext(newp,
                                പുതിയ AstAssign{fl, പുതിയ AstVarRef{fl, repickVarp, VAccess::WRITE},
                                              പുതിയ AstConst{fl, AstConst::BitTrue{}}});

        // "int _Vjoin_nleft_0 = N(items);"  // N(a) Number left to generate in list 0, starts at
        // N(list0)
        std::deque<AstVar*> nleftVarps;
        പൂർണ്ണസംഖ്യ i = 0;
        വേണ്ടി (AstRSProdItem* proditemp : lists) {
            // 'rand join arule arule arule' is legal, so need numbered, not non-unique named
            // nleft's
            AstVar* സ്ഥിരം varp = പുതിയ AstVar{
                fl, VVarType::VAR, "_Vjoin_nleft_" + std::to_string(i++), VFlagBitPacked{}, 32};
            varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            varp->funcLocal(സത്യം);
            m_breakVarp->addNextHere(varp);  // Add to function top, not newp
            nleftVarps.തള്ളുക_back(varp);

            newp = Ast_കണ്ണി::addNext(
                newp,
                പുതിയ AstAssign{fl, പുതിയ AstVarRef{fl, varp, VAccess::WRITE},
                              പുതിയ AstConst{fl, AstConst::Widthed_മൂല്യം{}, 32,
                                           static_cast<uint32_t>(listStmts[proditemp].size())}});
        }

        // "int _Vjoin_picked_elist = 0;"  // 0 = pick first eligible (non-finished) list, 1 = pick
        // second, etc
        AstVar* സ്ഥിരം pickedVarp
            = പുതിയ AstVar{fl, VVarType::VAR, "_Vjoin_picked_elist", VFlagBitPacked{}, 1};
        pickedVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        pickedVarp->funcLocal(സത്യം);
        m_breakVarp->addNextHere(pickedVarp);  // Add to function top, not newp

        // "while(1) {"  // Generate until make complete sequence
        AstJumpBlock* സ്ഥിരം whilep = പുതിയ AstJumpBlock{fl, nullptr};
        newp = Ast_കണ്ണി::addNext(newp, പുതിയ AstLoop{fl, whilep});
        {
            // "if ((std::rand() & 0xffffUL) > (0xffff * weight)) _Vjoin_repick = true;"
            whilep->addStmtsp(പുതിയ AstIf{
                fl,
                പുതിയ AstGt{
                    fl, പുതിയ AstRand{fl, AstRand::SRandomU32{}},
                    പുതിയ AstRToIS{fl, പുതിയ AstMulD{fl,
                                                 പുതിയ AstConst{fl, AstConst::RealDouble{},
                                                              static_cast<ഇരട്ട>(0xffffffffUL)},
                                                 weightp}}},
                പുതിയ AstAssign{fl, പുതിയ AstVarRef{fl, repickVarp, VAccess::WRITE},
                              പുതിയ AstConst{fl, AstConst::BitTrue{}}}});

            // "if (_Vjoin_repick) {"
            AstIf* ifp = പുതിയ AstIf{fl, പുതിയ AstVarRef{fl, repickVarp, VAccess::READ}};
            whilep->addStmtsp(ifp);
            {
                // "// _Vjoin_nlists_eligible is how many lists eligable for picking"
                // "int _Vjoin_nlists_eligible = 0;"
                AstVar* സ്ഥിരം eligibleVarp = പുതിയ AstVar{
                    fl, VVarType::VAR, "_Vjoin_nlists_eligible", VFlagBitPacked{}, 32};
                eligibleVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                eligibleVarp->funcLocal(സത്യം);
                m_breakVarp->addNextHere(eligibleVarp);  // Add to function top, not newp
                ifp->addThensp(പുതിയ AstAssign{fl, പുതിയ AstVarRef{fl, eligibleVarp, VAccess::WRITE},
                                             പുതിയ AstConst{fl, AstConst::Widthed_മൂല്യം{}, 32, 0}});

                i = 0;
                വേണ്ടി (AstRSProdItem* proditemp : lists) {
                    (ശൂന്യം)proditemp;
                    // "_Vjoin_nlists_eligible += _Vjoin_nleft_0 ? 1 : 0;"
                    ifp->addThensp(പുതിയ AstAssign{
                        fl, പുതിയ AstVarRef{fl, eligibleVarp, VAccess::WRITE},
                        പുതിയ AstAdd{
                            fl, പുതിയ AstVarRef{fl, eligibleVarp, VAccess::READ},
                            പുതിയ AstCond{
                                fl,
                                പുതിയ AstNeq{fl, പുതിയ AstConst{fl, AstConst::Widthed_മൂല്യം{}, 32, 0},
                                           പുതിയ AstVarRef{fl, nleftVarps[i], VAccess::READ}},
                                പുതിയ AstConst{fl, AstConst::Widthed_മൂല്യം{}, 32, 1},
                                പുതിയ AstConst{fl, AstConst::Widthed_മൂല്യം{}, 32, 0}}}});
                    ++i;
                }

                // "if (!_Vjoin_nlists_eligible) break;"  // Out of elements
                ifp->addThensp(
                    പുതിയ AstIf{fl,
                              പുതിയ AstEq{fl, പുതിയ AstConst{fl, AstConst::Widthed_മൂല്യം{}, 32, 0},
                                        പുതിയ AstVarRef{fl, eligibleVarp, VAccess::READ}},
                              പുതിയ AstJumpGo{fl, m_jumpBlockp}});

                // "// +1 to simplify usage, so 0 = done"
                // "_Vjoin_picked_elist = 1 + (std::rand() % _Vjoin_nlists_eligible);"
                ifp->addThensp(പുതിയ AstAssign{
                    fl, പുതിയ AstVarRef{fl, pickedVarp, VAccess::WRITE},
                    പുതിയ AstAdd{fl, പുതിയ AstConst{fl, AstConst::Widthed_മൂല്യം{}, 32, 1},
                               പുതിയ AstModDiv{fl, പുതിയ AstRand{fl, AstRand::SRandomU32{}},
                                             പുതിയ AstVarRef{fl, eligibleVarp, VAccess::READ}}}});

                // "_Vjoin_repick = false;
                ifp->addThensp(പുതിയ AstAssign{fl, പുതിയ AstVarRef{fl, repickVarp, VAccess::WRITE},
                                             പുതിയ AstConst{fl, AstConst::BitFalse{}}});
            }

            // "// Number of eligible lists left before hit _Vjoin_picked one"
            // "int _Vjoin_sel_elist = _Vjoin_picked_elist;"
            AstVar* സ്ഥിരം selVarp
                = പുതിയ AstVar{fl, VVarType::VAR, "_Vjoin_sel_elist", VFlagBitPacked{}, 32};
            selVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            selVarp->funcLocal(സത്യം);
            m_breakVarp->addNextHere(selVarp);  // Add to function top, not newp
            whilep->addStmtsp(പുതിയ AstAssign{fl, പുതിയ AstVarRef{fl, selVarp, VAccess::WRITE},
                                            പുതിയ AstVarRef{fl, pickedVarp, VAccess::READ}});

            i = -1;
            വേണ്ടി (AstRSProdItem* proditemp : lists) {
                ++i;
                // "if (_Vjoin_nleft_0) {"
                ifp = പുതിയ AstIf{fl,
                                പുതിയ AstNeq{fl, പുതിയ AstConst{fl, AstConst::Widthed_മൂല്യം{}, 32, 0},
                                           പുതിയ AstVarRef{fl, nleftVarps[i], VAccess::READ}},
                                nullptr, nullptr};
                whilep->addStmtsp(ifp);
                {
                    // "--_Vjoin_sel_elist;"
                    ifp->addThensp(പുതിയ AstAssign{
                        fl, പുതിയ AstVarRef{fl, selVarp, VAccess::WRITE},
                        പുതിയ AstSub{fl, പുതിയ AstVarRef{fl, selVarp, VAccess::READ},
                                   പുതിയ AstConst{fl, AstConst::Widthed_മൂല്യം{}, 32, 1}}});

                    // "if (_Vjoin_sel_elist == 0) {"
                    AstIf* സ്ഥിരം jIfp = പുതിയ AstIf{
                        fl,
                        പുതിയ AstEq{fl, പുതിയ AstConst{fl, AstConst::Widthed_മൂല്യം{}, 32, 0},
                                  പുതിയ AstVarRef{fl, selVarp, VAccess::READ}},
                        nullptr, nullptr};
                    ifp->addThensp(jIfp);
                    {
                        // "switch (_Vjoin_nleft_0) {"
                        //     "case 2 / * N(a)  * /: {statement}; break;"
                        //     "case 1 / * N(a) - 1 * /: {statement}; break;"
                        uint32_t j = static_cast<uint32_t>(listStmts[proditemp].size());
                        വേണ്ടി (Ast_കണ്ണി_Stmt* iprodp : listStmts[proditemp]) {
                            jIfp->addThensp(പുതിയ AstIf{
                                fl,
                                പുതിയ AstEq{fl, പുതിയ AstConst{fl, AstConst::Widthed_മൂല്യം{}, 32, j},
                                          പുതിയ AstVarRef{fl, nleftVarps[i], VAccess::READ}},
                                iprodp->clone_വൃക്ഷം(അസത്യം)});
                            --j;
                        }

                        // "--_Vjoin_nleft_0;"
                        jIfp->addThensp(പുതിയ AstAssign{
                            fl, പുതിയ AstVarRef{fl, nleftVarps[i], VAccess::WRITE},
                            പുതിയ AstSub{fl, പുതിയ AstVarRef{fl, nleftVarps[i], VAccess::READ},
                                       പുതിയ AstConst{fl, AstConst::Widthed_മൂല്യം{}, 32, 1}}});

                        // "if (0 == _Vjoin_nleft_0) _Vjoin_repick = true;"
                        jIfp->addThensp(പുതിയ AstIf{
                            fl,
                            പുതിയ AstEq{fl, പുതിയ AstConst{fl, AstConst::Widthed_മൂല്യം{}, 32, 0},
                                      പുതിയ AstVarRef{fl, nleftVarps[i], VAccess::READ}},
                            പുതിയ AstAssign{fl, പുതിയ AstVarRef{fl, repickVarp, VAccess::WRITE},
                                          പുതിയ AstConst{fl, AstConst::BitTrue{}}}});

                        // "continue;"
                        jIfp->addThensp(പുതിയ AstJumpGo{fl, whilep});
                    }
                }
            }
        }

        UINFOTREE(9, newp, "ForkJoin new", "-");
        തിരികെ newp;
    }

    // VISITORS

    ശൂന്യം visit(Ast_കണ്ണി_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildren(nodep);
    }

    ശൂന്യം visit(AstRandSequence* nodep) override {
        UINFO(9, "visit " << nodep);
        UASSERT_OBJ(m_modp, nodep, "randsequence not under module");
        എങ്കിൽ (m_rsp) {
            // No examples found in the wild
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: randsequence under randsequence");
            VL_DO_DANGLING(തള്ളുക_Deletep(nodep->unlinkFrBack()), nodep);
            തിരികെ;
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
        എങ്കിൽ (nodep->prodp()) m_startProdp = nodep->prodp();  // Has a start production selected

        // Build production functions
        വേണ്ടി (AstRSProd* prodp = nodep->prodsp(); prodp; prodp = VN_AS(prodp->nextp(), RSProd)) {
            newProdFunc(prodp);
            // Don't iterate yet, need to have tasks we can reference
        }

        // Iterate all of the new productions, moving guts into functions just made
        // Must do those with 'rand join' first, as they need to see the raw AstRS* nodes
        // before they are removed.  (Alternative would be to keep them until the end).
        UINFOTREE(9, nodep, "RS Tree pre-it", "-");
        std::unordered_set<AstRSProd*> prodHasRandJoin;
        വേണ്ടി (AstRSProd* prodp = nodep->prodsp(); prodp; prodp = VN_AS(prodp->nextp(), RSProd)) {
            prodp->foreach([&](സ്ഥിരം AstRSProdList* സ്ഥിരം prodlistp) {
                എങ്കിൽ (prodlistp->randJoin()) prodHasRandJoin.emplace(prodp);
            });
        }
        വേണ്ടി (AstRSProd *nextp, *prodp = nodep->prodsp(); prodp; prodp = nextp) {
            nextp = VN_AS(prodp->nextp(), RSProd);
            എങ്കിൽ (prodHasRandJoin.എണ്ണം(prodp)) VL_DO_DANGLING(iterate(prodp), prodp);
        }
        വേണ്ടി (AstRSProd *nextp, *prodp = nodep->prodsp(); prodp; prodp = nextp) {
            nextp = VN_AS(prodp->nextp(), RSProd);
            എങ്കിൽ (!prodHasRandJoin.എണ്ണം(prodp)) VL_DO_DANGLING(iterate(prodp), prodp);
        }
        UINFOTREE(9, nodep, "RS Tree post-it", "-");

        // Replace randsequence with call to randsequence start function
        AstTask* സ്ഥിരം startFuncp = newStartFunc(nodep);
        AstArg* argsp = nullptr;
        വേണ്ടി (auto& itr : m_localizeNames) {
            AstVar* സ്ഥിരം lvarp = itr.second;
            argsp = Ast_കണ്ണി::addNext(
                argsp, പുതിയ AstArg{nodep->fileline(), "__Vrsarg_" + lvarp->name(),
                                  പുതിയ AstVarRef{nodep->fileline(), lvarp, VAccess::READWRITE}});
        }
        AstTaskRef* സ്ഥിരം callStartp
            = പുതിയ AstTaskRef{nodep->fileline(), startFuncp->name(), argsp};
        callStartp->taskp(startFuncp);
        nodep->replaceWith(പുതിയ Ast_Stmt_വ്യഞ്ജകം{nodep->fileline(), callStartp});

        VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
    }
    ശൂന്യം visit(AstRSProd* nodep) override {
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
        m_breakVarp = newBreakVar(nodep->fileline(), സത്യം);
        m_prodFuncp->addStmtsp(m_breakVarp);

        // Production formal ports become real input ports on the generated task.
        // Must be added after m_breakVarp / localize ports so positional argument
        // order in newProdFuncRef stays consistent: [break, localize..., user...].
        എങ്കിൽ (Ast_കണ്ണി* സ്ഥിരം portsp = nodep->portsp()) {
            portsp->unlinkFrBackWithNext();
            m_prodFuncp->addStmtsp(portsp);
            വേണ്ടി (Ast_കണ്ണി* itp = portsp; itp; itp = itp->nextp()) {
                AstVar* സ്ഥിരം portVarp = VN_AS(itp, Var);
                portVarp->funcLocal(സത്യം);
                portVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            }
        }

        // Put JumpBlock immediately under the new function to support
        // a future break/return.  V3Const will rip it out if unneeded.
        VL_RESTORER(m_jumpBlockp);
        m_jumpBlockp = പുതിയ AstJumpBlock{nodep->fileline(), nullptr};
        m_prodFuncp->addStmtsp(m_jumpBlockp);

        എങ്കിൽ (nodep->fvarp())
            nodep->fvarp()->v3warn(E_UNSUPPORTED,
                                   "Unsupported: randsequence production function variable");

        // Move children into m_prodFuncp, and iterate there
        എങ്കിൽ (!nodep->rulesp()) {  // Nothing to do
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->rulesp()->prodlistsp() && nodep->rulesp()->prodlistsp()->randJoin()) {
            AstRSProdList* സ്ഥിരം prodlistp = nodep->rulesp()->prodlistsp();
            Ast_കണ്ണി* സ്ഥിരം itemsp = newProdRandJoin(nodep, prodlistp);
            m_jumpBlockp->addStmtsp(itemsp);
            iterateAndNextNull(itemsp);
        } അല്ലെങ്കിൽ എങ്കിൽ (!nodep->rulesp()->nextp()) {  // Single rule/list, can just do it
            // RSPROD(RSRULE(weight, stmt)) -> IF(weight != 0, stmt)
            സ്ഥിരം AstRSRule* സ്ഥിരം rulep = nodep->rulesp();
            Ast_കണ്ണി* itemsp = nullptr;
            എങ്കിൽ (rulep->weightStmtsp()) itemsp = rulep->weightStmtsp()->unlinkFrBackWithNext();
            എങ്കിൽ (rulep->prodlistsp())
                itemsp = Ast_കണ്ണി::addNext(itemsp, rulep->prodlistsp()->unlinkFrBackWithNext());
            // Can ignore rulep->weightp() unless is zero, in which case noop, add if
            എങ്കിൽ (rulep->weightp())
                itemsp = പുതിയ AstIf{rulep->weightp()->fileline(),
                                   പുതിയ AstNeq{rulep->weightp()->fileline(),
                                              പുതിയ AstConst{rulep->weightp()->fileline(), 0},
                                              rulep->weightp()->unlinkFrBack()},
                                   itemsp};
            // Move prodlist to parent (m_prodFunc) and process
            എങ്കിൽ (itemsp) {
                UINFOTREE(9, itemsp, "additems", "-");
                m_jumpBlockp->addStmtsp(itemsp);
                iterateAndNextNull(itemsp);
                UINFOTREE(9, m_jumpBlockp, "jumpBlockNow", "-");
            }
        } അല്ലെങ്കിൽ {
            // List of rules with "OR", need to random-weight them
            // RSPROD(RSRULE(weight1, stmt!), RSRULE(weight2, stmt2))
            //   -> RANDCASE(CASEITEM(weight1, stmt1), CASEITEM(weight2, stmt2))
            AstRandCase* സ്ഥിരം randcasep = പുതിയ AstRandCase{nodep->fileline(), nullptr};
            വേണ്ടി (AstRSRule* rulep = nodep->rulesp(); rulep;
                 rulep = VN_AS(rulep->nextp(), RSRule)) {
                Ast_കണ്ണി* itemsp = nullptr;
                എങ്കിൽ (rulep->weightStmtsp()) itemsp = rulep->weightStmtsp()->unlinkFrBackWithNext();
                എങ്കിൽ (rulep->prodlistsp())
                    itemsp = Ast_കണ്ണി::addNext(itemsp, rulep->prodlistsp()->unlinkFrBackWithNext());
                എങ്കിൽ (itemsp) {
                    Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം weightp = rulep->weightp()
                                                     ? rulep->weightp()->unlinkFrBack()
                                                     : പുതിയ AstConst{rulep->fileline(), 1};
                    randcasep->addItemsp(പുതിയ AstCaseItem{rulep->fileline(), weightp, itemsp});
                }
            }
            m_jumpBlockp->addStmtsp(randcasep);
            iterateAndNextNull(randcasep);
        }

        // V3Task needs to know if we ended up making a recursive function
        m_prodFuncp->foreach([&](Ast_കണ്ണി_FTask_Ref* സ്ഥിരം nodep) {
            എങ്കിൽ (nodep->taskp() == m_prodFuncp) m_prodFuncp->recursive(സത്യം);
        });
        UINFOTREE(9, m_prodFuncp, "rsprod-task-done " << nodep, "-");

        // Done with production, should have moved everything to new task
        VL_DO_DANGLING(തള്ളുക_Deletep(nodep->unlinkFrBack()), nodep);
    }
    ശൂന്യം visit(AstRSRule* nodep) override {
        // Rule of what to produce (as a list) under a production
        nodep->v3fatalSrc("randsequence rule should be iterated and deleted by AstRSProd");
    }
    ശൂന്യം visit(AstRSProdList* nodep) override {
        // List of productions to call, potentially with a weight
        UINFO(9, "visit " << nodep);
        UASSERT_OBJ(m_prodFuncp, nodep, "RSProdList not under production");

        // Move prodlist to parent (m_prodFunc) and process
        എങ്കിൽ (Ast_കണ്ണി* സ്ഥിരം itemsp = nodep->prodsp()) {
            UASSERT_OBJ(!nodep->randJoin(), nodep,
                        "rand join should have been handled in visit(RSRule)");
            nodep->replaceWith(itemsp->unlinkFrBackWithNext());
            // Will soon iterate itemsp as part of normal visit process
            // These will become sequential in the generated task
        } അല്ലെങ്കിൽ {
            nodep->unlinkFrBack();
        }
        VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
    }
    ശൂന്യം visit(AstRSProdItem* nodep) override {
        // "call" to generate using another production
        UINFO(9, "visit " << nodep);
        UASSERT_OBJ(m_prodFuncp, nodep, "RSProdItem not under production");
        AstRSProd* സ്ഥിരം foundp = nodep->prodp();
        UASSERT_OBJ(foundp, nodep, "Unlinked production reference");
        // Pass through caller-side argument list (IEEE 1800-2023 18.17.7).
        AstArg* സ്ഥിരം userArgsp
            = nodep->argsp() ? nodep->argsp()->unlinkFrBackWithNext() : nullptr;
        // Convert to task call
        Ast_കണ്ണി* സ്ഥിരം newp = newProdFuncRef(nodep, foundp, m_breakVarp, userArgsp);
        // The production might have done a "break;", skip other steps if so
        newp->addNext(പുതിയ AstIf{nodep->fileline(),
                                പുതിയ AstVarRef{nodep->fileline(), m_breakVarp, VAccess::READ},
                                പുതിയ AstJumpGo{nodep->fileline(), m_jumpBlockp}});

        nodep->replaceWith(newp);
        VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
    }

    ശൂന്യം visit(AstRSBreak* nodep) override {
        UASSERT_OBJ(m_jumpBlockp, nodep, "RSBreak not under production jump block");
        UASSERT_OBJ(m_breakVarp, nodep, "no break variable");
        Ast_കണ്ണി_Stmt* സ്ഥിരം newp = പുതിയ AstAssign{
            nodep->fileline(), പുതിയ AstVarRef{nodep->fileline(), m_breakVarp, VAccess::WRITE},
            പുതിയ AstConst{nodep->fileline(), AstConst::BitTrue{}}};
        newp->addNext(പുതിയ AstJumpGo{nodep->fileline(), m_jumpBlockp});
        nodep->replaceWith(newp);
        VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
    }
    ശൂന്യം visit(AstRSReturn* nodep) override {
        UASSERT_OBJ(m_jumpBlockp, nodep, "RSReturn not under production jump block");
        nodep->replaceWith(പുതിയ AstJumpGo{nodep->fileline(), m_jumpBlockp});
        VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
    }
    ശൂന്യം visit(AstVarRef* nodep) override {
        // Change references to variables inside randsequence to references to
        // ref variables on the new task
        സ്ഥിരം auto it = m_localizeRemaps.find(nodep->varp());
        എങ്കിൽ (it != m_localizeRemaps.end()) {
            AstVar* സ്ഥിരം iovarp = it->second;
            UINFO(9, "VARREF remap " << nodep << " -> " << iovarp);
            nodep->varp(iovarp);
        }
    }

    ശൂന്യം visit(Ast_കണ്ണി* nodep) override { iterateChildren(nodep); }

പൊതു:
    // CONSTRUCTORS
    explicit RandSequenceVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~RandSequenceVisitor() override = സ്ഥിരസ്ഥിതി;
};

//######################################################################
// RandSequence method class functions

ശൂന്യം V3RandSequence::randSequenceNetlist(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { സ്ഥിരം RandSequenceVisitor randSequenceVisitor{nodep}; }
    V3Global::dump_Check_Global_വൃക്ഷം("randsequence", 0, dump_വൃക്ഷം_Either_Level() >= 3);
}
