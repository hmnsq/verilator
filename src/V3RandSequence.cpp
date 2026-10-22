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

#અન્તર્ભાવ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#અન્તર્ભાવ "verilatedos.h"

#અન્તર્ભાવ "V3RandSequence.h"

#અન્તર્ભાવ "V3Ast.h"
#અન્તર્ભાવ "V3Error.h"
#અન્તર્ભાવ "V3FileLine.h"
#અન્તર્ભાવ "V3Global.h"

VL_DEFINE_DEBUG_FUNCTIONS;

// ######################################################################
//  Matcher classes (for suggestion matching)

વર્ગ RSProdMatcher final : જાહેર VNodeMatcher {
જાહેર:
    તાર્કિક ગાંઠ_Match(અચલ Ast_ગાંઠ* nodep) અચલ override { પાછા VN_IS(nodep, RSProd); }
};

//######################################################################
// Visitor that marks classes needing a randomize() method

વર્ગ RandSequenceVisitor final : જાહેર VNVisitor {
    // STATE - global
    std::map<AstRSProd*, Ast_ગાંઠ_FTask*> m_prodFuncps;  // Generated production functions

    // STATE - for current visit position (use VL_RESTORER)
    Ast_ગાંઠ_Module* m_modp = nullptr;  // Current module
    AstRandSequence* m_rsp = nullptr;  // Current randsequence
    AstRSProd* m_startProdp = nullptr;  // Starting production
    Ast_ગાંઠ_FTask* m_prodFuncp = nullptr;  // Production function being built
    AstJumpBlock* m_jumpBlockp = nullptr;  // Building jumpblock for return/break
    AstVar* m_breakVarp = nullptr;  // Break variable
    std::unordered_set<AstVar*> m_localizes;  // Variables to become function inouts
    std::map<std::string, AstVar*> m_localizeNames;  // Ordered names of function inouts
    std::unordered_map<અચલ AstVar*, AstVar*> m_localizeRemaps;  // New ports for old vars

    // METHODS
    શૂન્ય findLocalizes(AstRandSequence* nodep) {
        std::set<AstVar*> localVars;
        nodep->foreach([&](Ast_ગાંઠ* અચલ nodep) {
            જો (અચલ AstVarRef* અચલ anodep = VN_CAST(nodep, VarRef)) {
                m_localizes.emplace(anodep->varp());
            } નહીંતર જો (AstVar* અચલ anodep = VN_CAST(nodep, Var)) {
                localVars.emplace(anodep);
            }
        });
        // Clear every variable allocated in the RandSequence
        માટે (AstVar* varp : localVars) m_localizes.erase(varp);
        // Sort by name, so function arguments are stable
        માટે (AstVar* varp : m_localizes) {
            UINFO(9, "localize " << varp);
            m_localizeNames.emplace(varp->name(), varp);
        }
    }

    AstVar* newBreakVar(File_લીટી* fl, તાર્કિક asOutput) {
        AstVar* અચલ varp = નવું AstVar{fl, VVarType::PORT, "__VrsBreak", VFlagBitPacked{}, 1};
        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        varp->funcLocal(સાચું);
        જો (asOutput) varp->direction(VDirection::OUTPUT);
        Ast_ગાંઠ* outp = varp;
        outp->addNext(નવું AstAssign{fl, નવું AstVarRef{fl, varp, VAccess::WRITE},
                                    નવું AstConst{fl, AstConst::BitFalse{}}});

        // Also add arguments as next's
        માટે (અચલ auto& itr : m_localizeNames) {
            અચલ AstVar* અચલ lvarp = itr.second;
            AstVar* અચલ iovarp
                = નવું AstVar{fl, VVarType::PORT, "__Vrsarg_" + lvarp->name(), lvarp};
            iovarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            iovarp->funcLocal(સાચું);
            iovarp->direction(VDirection::REF);
            varp->addNext(iovarp);
            m_localizeRemaps.emplace(lvarp, iovarp);
        }
        પાછા varp;
    }

    AstTask* newStartFunc(AstRandSequence* nodep) {
        // Create wrapper that is called by the sequence
        AstTask* અચલ taskp = નવું AstTask{m_startProdp->fileline(), m_rsp->name(), nullptr};
        taskp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        m_modp->addStmtsp(taskp);

        // Create local (not output) break variable
        VL_RESTORER_COPY(m_localizeRemaps);
        AstVar* breakVarp = newBreakVar(nodep->fileline(), ખોટું);
        taskp->addStmtsp(breakVarp);

        // Call the start production's task
        taskp->addStmtsp(newProdFuncRef(nodep, m_startProdp, breakVarp, nullptr));

        UINFOTREE(9, taskp, "newStart", "");
        પાછા taskp;
    }

    Ast_ગાંઠ* newProdFuncRef(Ast_ગાંઠ* nodep, AstRSProd* prodp, AstVar* breakVarp,
                            AstArg* userArgsp) {
        auto it = m_prodFuncps.find(prodp);
        UASSERT_OBJ(it != m_prodFuncps.end(), nodep, "No production function made");
        Ast_ગાંઠ_FTask* અચલ prodFuncp = it->second;
        File_લીટી* અચલ fl = nodep->fileline();
        // V3Width already ran before V3RandSequence, so VarRefs we create here
        // need dtype set explicitly, V3Broken later checks width == widthMin.
        AstVarRef* અચલ breakRefp = નવું AstVarRef{fl, breakVarp, VAccess::WRITE};
        breakRefp->dtypeFrom(breakVarp);
        AstArg* અચલ argsp = નવું AstArg{fl, breakVarp->name(), breakRefp};
        માટે (અચલ auto& itr : m_localizeNames) {
            અચલ AstVar* અચલ lvarp = itr.second;
            AstVar* અચલ iovarp = m_localizeRemaps[lvarp];
            UASSERT_OBJ(iovarp, nodep, "No new port variable for local variable" << lvarp);
            AstVarRef* અચલ refp = નવું AstVarRef{fl, iovarp, VAccess::READWRITE};
            refp->dtypeFrom(iovarp);
            argsp->addNext(નવું AstArg{nodep->fileline(), "__Vrsarg_" + lvarp->name(), refp});
        }
        જો (userArgsp) argsp->addNext(userArgsp);
        Ast_ગાંઠ* અચલ newp
            = નવું Ast_Stmt_પદાવલિ{fl, નવું AstTaskRef{fl, VN_AS(prodFuncp, Task), argsp}};
        પાછા newp;
    }

    શૂન્ય newProdFunc(AstRSProd* nodep) {
        // Task, as time may pass
        AstTask* અચલ newp
            = નવું AstTask{nodep->fileline(), m_rsp->name() + "_" + nodep->name(), nullptr};
        m_modp->addStmtsp(newp);
        m_prodFuncps.emplace(nodep, newp);
        UINFOTREE(9, newp, "newProd", "");
    }

    Ast_ગાંઠ* newProdRandJoin(AstRSProd* /*prodp*/, AstRSProdList* prodlistp) {
        // For weight == 1.0 longer sequence (favor stay in a)
        // For weight == 0.0 shorter squence (favor change a/b)
        UASSERT_OBJ(prodlistp->weightp(), prodlistp, "Weight should have default CONST(0.5)");
        Ast_ગાંઠ_પદાવલિ* weightp = prodlistp->weightp()->unlinkFrBack();

        // Build map of production lists and productions we need to join
        // Must mainain the relative order of each sequence.
        std::deque<AstRSProdItem*> lists;
        std::unordered_map<AstRSProdItem*, std::deque<Ast_ગાંઠ_Stmt*>> listStmts;
        માટે (AstRSProdItem* proditemp = VN_AS(prodlistp->prodsp(), RSProdItem); proditemp;
             proditemp = VN_AS(proditemp->nextp(), RSProdItem)) {
            lists.ધકેલો_back(proditemp);
            અચલ AstRSProd* અચલ subProdp = proditemp->prodp();
            જો (!subProdp) ચાલુ;
            જો (!subProdp->rulesp()) ચાલુ;
            જો (!subProdp->rulesp()->prodlistsp()) ચાલુ;
            માટે (Ast_ગાંઠ_Stmt* subStmtp
                 = VN_AS(subProdp->rulesp()->prodlistsp()->prodsp(), ગાંઠ_Stmt);
                 subStmtp; subStmtp = VN_AS(subStmtp->nextp(), ગાંઠ_Stmt)) {
                listStmts[proditemp].ધકેલો_back(subStmtp);
            }
        }

        UINFO(9, "RandJoin productions called:");
        માટે (AstRSProdItem* proditemp : lists) {
            UINFO(9, "  list " << proditemp);
            માટે (અચલ Ast_ગાંઠ_Stmt* prodip : listStmts[proditemp])
                UINFO(9, "    calls " << prodip);
        }

        // Need to clone all nodes used

        File_લીટી* fl = prodlistp->fileline();
        Ast_ગાંઠ* newp = nullptr;

        // "bool _Vjoin_repick = true;"  // Pick a new longer sequence
        AstVar* repickVarp = નવું AstVar{fl, VVarType::VAR, "_Vjoin_repick", VFlagBitPacked{}, 1};
        repickVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        repickVarp->funcLocal(સાચું);
        m_breakVarp->addNextHere(repickVarp);  // Add to function top, not newp
        newp = Ast_ગાંઠ::addNext(newp,
                                નવું AstAssign{fl, નવું AstVarRef{fl, repickVarp, VAccess::WRITE},
                                              નવું AstConst{fl, AstConst::BitTrue{}}});

        // "int _Vjoin_nleft_0 = N(items);"  // N(a) Number left to generate in list 0, starts at
        // N(list0)
        std::deque<AstVar*> nleftVarps;
        પૂર્ણાંક i = 0;
        માટે (AstRSProdItem* proditemp : lists) {
            // 'rand join arule arule arule' is legal, so need numbered, not non-unique named
            // nleft's
            AstVar* અચલ varp = નવું AstVar{
                fl, VVarType::VAR, "_Vjoin_nleft_" + std::to_string(i++), VFlagBitPacked{}, 32};
            varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            varp->funcLocal(સાચું);
            m_breakVarp->addNextHere(varp);  // Add to function top, not newp
            nleftVarps.ધકેલો_back(varp);

            newp = Ast_ગાંઠ::addNext(
                newp,
                નવું AstAssign{fl, નવું AstVarRef{fl, varp, VAccess::WRITE},
                              નવું AstConst{fl, AstConst::Widthed_કિંમત{}, 32,
                                           static_cast<uint32_t>(listStmts[proditemp].size())}});
        }

        // "int _Vjoin_picked_elist = 0;"  // 0 = pick first eligible (non-finished) list, 1 = pick
        // second, etc
        AstVar* અચલ pickedVarp
            = નવું AstVar{fl, VVarType::VAR, "_Vjoin_picked_elist", VFlagBitPacked{}, 1};
        pickedVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        pickedVarp->funcLocal(સાચું);
        m_breakVarp->addNextHere(pickedVarp);  // Add to function top, not newp

        // "while(1) {"  // Generate until make complete sequence
        AstJumpBlock* અચલ whilep = નવું AstJumpBlock{fl, nullptr};
        newp = Ast_ગાંઠ::addNext(newp, નવું AstLoop{fl, whilep});
        {
            // "if ((std::rand() & 0xffffUL) > (0xffff * weight)) _Vjoin_repick = true;"
            whilep->addStmtsp(નવું AstIf{
                fl,
                નવું AstGt{
                    fl, નવું AstRand{fl, AstRand::SRandomU32{}},
                    નવું AstRToIS{fl, નવું AstMulD{fl,
                                                 નવું AstConst{fl, AstConst::RealDouble{},
                                                              static_cast<દ્વિગુણ>(0xffffffffUL)},
                                                 weightp}}},
                નવું AstAssign{fl, નવું AstVarRef{fl, repickVarp, VAccess::WRITE},
                              નવું AstConst{fl, AstConst::BitTrue{}}}});

            // "if (_Vjoin_repick) {"
            AstIf* ifp = નવું AstIf{fl, નવું AstVarRef{fl, repickVarp, VAccess::READ}};
            whilep->addStmtsp(ifp);
            {
                // "// _Vjoin_nlists_eligible is how many lists eligable for picking"
                // "int _Vjoin_nlists_eligible = 0;"
                AstVar* અચલ eligibleVarp = નવું AstVar{
                    fl, VVarType::VAR, "_Vjoin_nlists_eligible", VFlagBitPacked{}, 32};
                eligibleVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                eligibleVarp->funcLocal(સાચું);
                m_breakVarp->addNextHere(eligibleVarp);  // Add to function top, not newp
                ifp->addThensp(નવું AstAssign{fl, નવું AstVarRef{fl, eligibleVarp, VAccess::WRITE},
                                             નવું AstConst{fl, AstConst::Widthed_કિંમત{}, 32, 0}});

                i = 0;
                માટે (AstRSProdItem* proditemp : lists) {
                    (શૂન્ય)proditemp;
                    // "_Vjoin_nlists_eligible += _Vjoin_nleft_0 ? 1 : 0;"
                    ifp->addThensp(નવું AstAssign{
                        fl, નવું AstVarRef{fl, eligibleVarp, VAccess::WRITE},
                        નવું AstAdd{
                            fl, નવું AstVarRef{fl, eligibleVarp, VAccess::READ},
                            નવું AstCond{
                                fl,
                                નવું AstNeq{fl, નવું AstConst{fl, AstConst::Widthed_કિંમત{}, 32, 0},
                                           નવું AstVarRef{fl, nleftVarps[i], VAccess::READ}},
                                નવું AstConst{fl, AstConst::Widthed_કિંમત{}, 32, 1},
                                નવું AstConst{fl, AstConst::Widthed_કિંમત{}, 32, 0}}}});
                    ++i;
                }

                // "if (!_Vjoin_nlists_eligible) break;"  // Out of elements
                ifp->addThensp(
                    નવું AstIf{fl,
                              નવું AstEq{fl, નવું AstConst{fl, AstConst::Widthed_કિંમત{}, 32, 0},
                                        નવું AstVarRef{fl, eligibleVarp, VAccess::READ}},
                              નવું AstJumpGo{fl, m_jumpBlockp}});

                // "// +1 to simplify usage, so 0 = done"
                // "_Vjoin_picked_elist = 1 + (std::rand() % _Vjoin_nlists_eligible);"
                ifp->addThensp(નવું AstAssign{
                    fl, નવું AstVarRef{fl, pickedVarp, VAccess::WRITE},
                    નવું AstAdd{fl, નવું AstConst{fl, AstConst::Widthed_કિંમત{}, 32, 1},
                               નવું AstModDiv{fl, નવું AstRand{fl, AstRand::SRandomU32{}},
                                             નવું AstVarRef{fl, eligibleVarp, VAccess::READ}}}});

                // "_Vjoin_repick = false;
                ifp->addThensp(નવું AstAssign{fl, નવું AstVarRef{fl, repickVarp, VAccess::WRITE},
                                             નવું AstConst{fl, AstConst::BitFalse{}}});
            }

            // "// Number of eligible lists left before hit _Vjoin_picked one"
            // "int _Vjoin_sel_elist = _Vjoin_picked_elist;"
            AstVar* અચલ selVarp
                = નવું AstVar{fl, VVarType::VAR, "_Vjoin_sel_elist", VFlagBitPacked{}, 32};
            selVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            selVarp->funcLocal(સાચું);
            m_breakVarp->addNextHere(selVarp);  // Add to function top, not newp
            whilep->addStmtsp(નવું AstAssign{fl, નવું AstVarRef{fl, selVarp, VAccess::WRITE},
                                            નવું AstVarRef{fl, pickedVarp, VAccess::READ}});

            i = -1;
            માટે (AstRSProdItem* proditemp : lists) {
                ++i;
                // "if (_Vjoin_nleft_0) {"
                ifp = નવું AstIf{fl,
                                નવું AstNeq{fl, નવું AstConst{fl, AstConst::Widthed_કિંમત{}, 32, 0},
                                           નવું AstVarRef{fl, nleftVarps[i], VAccess::READ}},
                                nullptr, nullptr};
                whilep->addStmtsp(ifp);
                {
                    // "--_Vjoin_sel_elist;"
                    ifp->addThensp(નવું AstAssign{
                        fl, નવું AstVarRef{fl, selVarp, VAccess::WRITE},
                        નવું AstSub{fl, નવું AstVarRef{fl, selVarp, VAccess::READ},
                                   નવું AstConst{fl, AstConst::Widthed_કિંમત{}, 32, 1}}});

                    // "if (_Vjoin_sel_elist == 0) {"
                    AstIf* અચલ jIfp = નવું AstIf{
                        fl,
                        નવું AstEq{fl, નવું AstConst{fl, AstConst::Widthed_કિંમત{}, 32, 0},
                                  નવું AstVarRef{fl, selVarp, VAccess::READ}},
                        nullptr, nullptr};
                    ifp->addThensp(jIfp);
                    {
                        // "switch (_Vjoin_nleft_0) {"
                        //     "case 2 / * N(a)  * /: {statement}; break;"
                        //     "case 1 / * N(a) - 1 * /: {statement}; break;"
                        uint32_t j = static_cast<uint32_t>(listStmts[proditemp].size());
                        માટે (Ast_ગાંઠ_Stmt* iprodp : listStmts[proditemp]) {
                            jIfp->addThensp(નવું AstIf{
                                fl,
                                નવું AstEq{fl, નવું AstConst{fl, AstConst::Widthed_કિંમત{}, 32, j},
                                          નવું AstVarRef{fl, nleftVarps[i], VAccess::READ}},
                                iprodp->clone_વૃક્ષ(ખોટું)});
                            --j;
                        }

                        // "--_Vjoin_nleft_0;"
                        jIfp->addThensp(નવું AstAssign{
                            fl, નવું AstVarRef{fl, nleftVarps[i], VAccess::WRITE},
                            નવું AstSub{fl, નવું AstVarRef{fl, nleftVarps[i], VAccess::READ},
                                       નવું AstConst{fl, AstConst::Widthed_કિંમત{}, 32, 1}}});

                        // "if (0 == _Vjoin_nleft_0) _Vjoin_repick = true;"
                        jIfp->addThensp(નવું AstIf{
                            fl,
                            નવું AstEq{fl, નવું AstConst{fl, AstConst::Widthed_કિંમત{}, 32, 0},
                                      નવું AstVarRef{fl, nleftVarps[i], VAccess::READ}},
                            નવું AstAssign{fl, નવું AstVarRef{fl, repickVarp, VAccess::WRITE},
                                          નવું AstConst{fl, AstConst::BitTrue{}}}});

                        // "continue;"
                        jIfp->addThensp(નવું AstJumpGo{fl, whilep});
                    }
                }
            }
        }

        UINFOTREE(9, newp, "ForkJoin new", "-");
        પાછા newp;
    }

    // VISITORS

    શૂન્ય visit(Ast_ગાંઠ_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildren(nodep);
    }

    શૂન્ય visit(AstRandSequence* nodep) override {
        UINFO(9, "visit " << nodep);
        UASSERT_OBJ(m_modp, nodep, "randsequence not under module");
        જો (m_rsp) {
            // No examples found in the wild
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: randsequence under randsequence");
            VL_DO_DANGLING(ધકેલો_Deletep(nodep->unlinkFrBack()), nodep);
            પાછા;
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
        જો (nodep->prodp()) m_startProdp = nodep->prodp();  // Has a start production selected

        // Build production functions
        માટે (AstRSProd* prodp = nodep->prodsp(); prodp; prodp = VN_AS(prodp->nextp(), RSProd)) {
            newProdFunc(prodp);
            // Don't iterate yet, need to have tasks we can reference
        }

        // Iterate all of the new productions, moving guts into functions just made
        // Must do those with 'rand join' first, as they need to see the raw AstRS* nodes
        // before they are removed.  (Alternative would be to keep them until the end).
        UINFOTREE(9, nodep, "RS Tree pre-it", "-");
        std::unordered_set<AstRSProd*> prodHasRandJoin;
        માટે (AstRSProd* prodp = nodep->prodsp(); prodp; prodp = VN_AS(prodp->nextp(), RSProd)) {
            prodp->foreach([&](અચલ AstRSProdList* અચલ prodlistp) {
                જો (prodlistp->randJoin()) prodHasRandJoin.emplace(prodp);
            });
        }
        માટે (AstRSProd *nextp, *prodp = nodep->prodsp(); prodp; prodp = nextp) {
            nextp = VN_AS(prodp->nextp(), RSProd);
            જો (prodHasRandJoin.ગણતરી(prodp)) VL_DO_DANGLING(iterate(prodp), prodp);
        }
        માટે (AstRSProd *nextp, *prodp = nodep->prodsp(); prodp; prodp = nextp) {
            nextp = VN_AS(prodp->nextp(), RSProd);
            જો (!prodHasRandJoin.ગણતરી(prodp)) VL_DO_DANGLING(iterate(prodp), prodp);
        }
        UINFOTREE(9, nodep, "RS Tree post-it", "-");

        // Replace randsequence with call to randsequence start function
        AstTask* અચલ startFuncp = newStartFunc(nodep);
        AstArg* argsp = nullptr;
        માટે (auto& itr : m_localizeNames) {
            AstVar* અચલ lvarp = itr.second;
            argsp = Ast_ગાંઠ::addNext(
                argsp, નવું AstArg{nodep->fileline(), "__Vrsarg_" + lvarp->name(),
                                  નવું AstVarRef{nodep->fileline(), lvarp, VAccess::READWRITE}});
        }
        AstTaskRef* અચલ callStartp
            = નવું AstTaskRef{nodep->fileline(), startFuncp->name(), argsp};
        callStartp->taskp(startFuncp);
        nodep->replaceWith(નવું Ast_Stmt_પદાવલિ{nodep->fileline(), callStartp});

        VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
    }
    શૂન્ય visit(AstRSProd* nodep) override {
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
        m_breakVarp = newBreakVar(nodep->fileline(), સાચું);
        m_prodFuncp->addStmtsp(m_breakVarp);

        // Production formal ports become real input ports on the generated task.
        // Must be added after m_breakVarp / localize ports so positional argument
        // order in newProdFuncRef stays consistent: [break, localize..., user...].
        જો (Ast_ગાંઠ* અચલ portsp = nodep->portsp()) {
            portsp->unlinkFrBackWithNext();
            m_prodFuncp->addStmtsp(portsp);
            માટે (Ast_ગાંઠ* itp = portsp; itp; itp = itp->nextp()) {
                AstVar* અચલ portVarp = VN_AS(itp, Var);
                portVarp->funcLocal(સાચું);
                portVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            }
        }

        // Put JumpBlock immediately under the new function to support
        // a future break/return.  V3Const will rip it out if unneeded.
        VL_RESTORER(m_jumpBlockp);
        m_jumpBlockp = નવું AstJumpBlock{nodep->fileline(), nullptr};
        m_prodFuncp->addStmtsp(m_jumpBlockp);

        જો (nodep->fvarp())
            nodep->fvarp()->v3warn(E_UNSUPPORTED,
                                   "Unsupported: randsequence production function variable");

        // Move children into m_prodFuncp, and iterate there
        જો (!nodep->rulesp()) {  // Nothing to do
        } નહીંતર જો (nodep->rulesp()->prodlistsp() && nodep->rulesp()->prodlistsp()->randJoin()) {
            AstRSProdList* અચલ prodlistp = nodep->rulesp()->prodlistsp();
            Ast_ગાંઠ* અચલ itemsp = newProdRandJoin(nodep, prodlistp);
            m_jumpBlockp->addStmtsp(itemsp);
            iterateAndNextNull(itemsp);
        } નહીંતર જો (!nodep->rulesp()->nextp()) {  // Single rule/list, can just do it
            // RSPROD(RSRULE(weight, stmt)) -> IF(weight != 0, stmt)
            અચલ AstRSRule* અચલ rulep = nodep->rulesp();
            Ast_ગાંઠ* itemsp = nullptr;
            જો (rulep->weightStmtsp()) itemsp = rulep->weightStmtsp()->unlinkFrBackWithNext();
            જો (rulep->prodlistsp())
                itemsp = Ast_ગાંઠ::addNext(itemsp, rulep->prodlistsp()->unlinkFrBackWithNext());
            // Can ignore rulep->weightp() unless is zero, in which case noop, add if
            જો (rulep->weightp())
                itemsp = નવું AstIf{rulep->weightp()->fileline(),
                                   નવું AstNeq{rulep->weightp()->fileline(),
                                              નવું AstConst{rulep->weightp()->fileline(), 0},
                                              rulep->weightp()->unlinkFrBack()},
                                   itemsp};
            // Move prodlist to parent (m_prodFunc) and process
            જો (itemsp) {
                UINFOTREE(9, itemsp, "additems", "-");
                m_jumpBlockp->addStmtsp(itemsp);
                iterateAndNextNull(itemsp);
                UINFOTREE(9, m_jumpBlockp, "jumpBlockNow", "-");
            }
        } નહીંતર {
            // List of rules with "OR", need to random-weight them
            // RSPROD(RSRULE(weight1, stmt!), RSRULE(weight2, stmt2))
            //   -> RANDCASE(CASEITEM(weight1, stmt1), CASEITEM(weight2, stmt2))
            AstRandCase* અચલ randcasep = નવું AstRandCase{nodep->fileline(), nullptr};
            માટે (AstRSRule* rulep = nodep->rulesp(); rulep;
                 rulep = VN_AS(rulep->nextp(), RSRule)) {
                Ast_ગાંઠ* itemsp = nullptr;
                જો (rulep->weightStmtsp()) itemsp = rulep->weightStmtsp()->unlinkFrBackWithNext();
                જો (rulep->prodlistsp())
                    itemsp = Ast_ગાંઠ::addNext(itemsp, rulep->prodlistsp()->unlinkFrBackWithNext());
                જો (itemsp) {
                    Ast_ગાંઠ_પદાવલિ* અચલ weightp = rulep->weightp()
                                                     ? rulep->weightp()->unlinkFrBack()
                                                     : નવું AstConst{rulep->fileline(), 1};
                    randcasep->addItemsp(નવું AstCaseItem{rulep->fileline(), weightp, itemsp});
                }
            }
            m_jumpBlockp->addStmtsp(randcasep);
            iterateAndNextNull(randcasep);
        }

        // V3Task needs to know if we ended up making a recursive function
        m_prodFuncp->foreach([&](Ast_ગાંઠ_FTask_Ref* અચલ nodep) {
            જો (nodep->taskp() == m_prodFuncp) m_prodFuncp->recursive(સાચું);
        });
        UINFOTREE(9, m_prodFuncp, "rsprod-task-done " << nodep, "-");

        // Done with production, should have moved everything to new task
        VL_DO_DANGLING(ધકેલો_Deletep(nodep->unlinkFrBack()), nodep);
    }
    શૂન્ય visit(AstRSRule* nodep) override {
        // Rule of what to produce (as a list) under a production
        nodep->v3fatalSrc("randsequence rule should be iterated and deleted by AstRSProd");
    }
    શૂન્ય visit(AstRSProdList* nodep) override {
        // List of productions to call, potentially with a weight
        UINFO(9, "visit " << nodep);
        UASSERT_OBJ(m_prodFuncp, nodep, "RSProdList not under production");

        // Move prodlist to parent (m_prodFunc) and process
        જો (Ast_ગાંઠ* અચલ itemsp = nodep->prodsp()) {
            UASSERT_OBJ(!nodep->randJoin(), nodep,
                        "rand join should have been handled in visit(RSRule)");
            nodep->replaceWith(itemsp->unlinkFrBackWithNext());
            // Will soon iterate itemsp as part of normal visit process
            // These will become sequential in the generated task
        } નહીંતર {
            nodep->unlinkFrBack();
        }
        VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
    }
    શૂન્ય visit(AstRSProdItem* nodep) override {
        // "call" to generate using another production
        UINFO(9, "visit " << nodep);
        UASSERT_OBJ(m_prodFuncp, nodep, "RSProdItem not under production");
        AstRSProd* અચલ foundp = nodep->prodp();
        UASSERT_OBJ(foundp, nodep, "Unlinked production reference");
        // Pass through caller-side argument list (IEEE 1800-2023 18.17.7).
        AstArg* અચલ userArgsp
            = nodep->argsp() ? nodep->argsp()->unlinkFrBackWithNext() : nullptr;
        // Convert to task call
        Ast_ગાંઠ* અચલ newp = newProdFuncRef(nodep, foundp, m_breakVarp, userArgsp);
        // The production might have done a "break;", skip other steps if so
        newp->addNext(નવું AstIf{nodep->fileline(),
                                નવું AstVarRef{nodep->fileline(), m_breakVarp, VAccess::READ},
                                નવું AstJumpGo{nodep->fileline(), m_jumpBlockp}});

        nodep->replaceWith(newp);
        VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
    }

    શૂન્ય visit(AstRSBreak* nodep) override {
        UASSERT_OBJ(m_jumpBlockp, nodep, "RSBreak not under production jump block");
        UASSERT_OBJ(m_breakVarp, nodep, "no break variable");
        Ast_ગાંઠ_Stmt* અચલ newp = નવું AstAssign{
            nodep->fileline(), નવું AstVarRef{nodep->fileline(), m_breakVarp, VAccess::WRITE},
            નવું AstConst{nodep->fileline(), AstConst::BitTrue{}}};
        newp->addNext(નવું AstJumpGo{nodep->fileline(), m_jumpBlockp});
        nodep->replaceWith(newp);
        VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
    }
    શૂન્ય visit(AstRSReturn* nodep) override {
        UASSERT_OBJ(m_jumpBlockp, nodep, "RSReturn not under production jump block");
        nodep->replaceWith(નવું AstJumpGo{nodep->fileline(), m_jumpBlockp});
        VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
    }
    શૂન્ય visit(AstVarRef* nodep) override {
        // Change references to variables inside randsequence to references to
        // ref variables on the new task
        અચલ auto it = m_localizeRemaps.find(nodep->varp());
        જો (it != m_localizeRemaps.end()) {
            AstVar* અચલ iovarp = it->second;
            UINFO(9, "VARREF remap " << nodep << " -> " << iovarp);
            nodep->varp(iovarp);
        }
    }

    શૂન્ય visit(Ast_ગાંઠ* nodep) override { iterateChildren(nodep); }

જાહેર:
    // CONSTRUCTORS
    explicit RandSequenceVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~RandSequenceVisitor() override = મૂળભૂત;
};

//######################################################################
// RandSequence method class functions

શૂન્ય V3RandSequence::randSequenceNetlist(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { અચલ RandSequenceVisitor randSequenceVisitor{nodep}; }
    V3Global::dump_Check_Global_વૃક્ષ("randsequence", 0, dump_વૃક્ષ_Either_Level() >= 3);
}
