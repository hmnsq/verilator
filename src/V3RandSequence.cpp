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

#अन्तर्भाव "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#अन्तर्भाव "verilatedos.h"

#अन्तर्भाव "V3RandSequence.h"

#अन्तर्भाव "V3Ast.h"
#अन्तर्भाव "V3Error.h"
#अन्तर्भाव "V3FileLine.h"
#अन्तर्भाव "V3Global.h"

VL_DEFINE_DEBUG_FUNCTIONS;

// ######################################################################
//  Matcher classes (for suggestion matching)

वग्ग RSProdMatcher final : सार्वजनिक VNodeMatcher {
सार्वजनिक:
    तार्किक ग्रन्थिः_Match(ध्रुव Ast_ग्रन्थिः* nodep) ध्रुव override { पटिनिवत्त VN_IS(nodep, RSProd); }
};

//######################################################################
// Visitor that marks classes needing a randomize() method

वग्ग RandSequenceVisitor final : सार्वजनिक VNVisitor {
    // STATE - global
    std::map<AstRSProd*, Ast_ग्रन्थिः_FTask*> m_prodFuncps;  // Generated production functions

    // STATE - for current visit position (use VL_RESTORER)
    Ast_ग्रन्थिः_Module* m_modp = nullptr;  // Current module
    AstRandSequence* m_rsp = nullptr;  // Current randsequence
    AstRSProd* m_startProdp = nullptr;  // Starting production
    Ast_ग्रन्थिः_FTask* m_prodFuncp = nullptr;  // Production function being built
    AstJumpBlock* m_jumpBlockp = nullptr;  // Building jumpblock for return/break
    AstVar* m_breakVarp = nullptr;  // Break variable
    std::unordered_set<AstVar*> m_localizes;  // Variables to become function inouts
    std::map<std::string, AstVar*> m_localizeNames;  // Ordered names of function inouts
    std::unordered_map<ध्रुव AstVar*, AstVar*> m_localizeRemaps;  // New ports for old vars

    // METHODS
    शून्य findLocalizes(AstRandSequence* nodep) {
        std::set<AstVar*> localVars;
        nodep->foreach([&](Ast_ग्रन्थिः* ध्रुव nodep) {
            सचे (ध्रुव AstVarRef* ध्रुव anodep = VN_CAST(nodep, VarRef)) {
                m_localizes.emplace(anodep->varp());
            } अञ्ञथा सचे (AstVar* ध्रुव anodep = VN_CAST(nodep, Var)) {
                localVars.emplace(anodep);
            }
        });
        // Clear every variable allocated in the RandSequence
        अत्थाय (AstVar* varp : localVars) m_localizes.erase(varp);
        // Sort by name, so function arguments are stable
        अत्थाय (AstVar* varp : m_localizes) {
            UINFO(9, "localize " << varp);
            m_localizeNames.emplace(varp->name(), varp);
        }
    }

    AstVar* newBreakVar(File_पङ्क्तिः* fl, तार्किक asOutput) {
        AstVar* ध्रुव varp = नूतन AstVar{fl, VVarType::PORT, "__VrsBreak", VFlagBitPacked{}, 1};
        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        varp->funcLocal(सच्च);
        सचे (asOutput) varp->direction(VDirection::OUTPUT);
        Ast_ग्रन्थिः* outp = varp;
        outp->addNext(नूतन AstAssign{fl, नूतन AstVarRef{fl, varp, VAccess::WRITE},
                                    नूतन AstConst{fl, AstConst::BitFalse{}}});

        // Also add arguments as next's
        अत्थाय (ध्रुव auto& itr : m_localizeNames) {
            ध्रुव AstVar* ध्रुव lvarp = itr.second;
            AstVar* ध्रुव iovarp
                = नूतन AstVar{fl, VVarType::PORT, "__Vrsarg_" + lvarp->name(), lvarp};
            iovarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            iovarp->funcLocal(सच्च);
            iovarp->direction(VDirection::REF);
            varp->addNext(iovarp);
            m_localizeRemaps.emplace(lvarp, iovarp);
        }
        पटिनिवत्त varp;
    }

    AstTask* newStartFunc(AstRandSequence* nodep) {
        // Create wrapper that is called by the sequence
        AstTask* ध्रुव taskp = नूतन AstTask{m_startProdp->fileline(), m_rsp->name(), nullptr};
        taskp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        m_modp->addStmtsp(taskp);

        // Create local (not output) break variable
        VL_RESTORER_COPY(m_localizeRemaps);
        AstVar* breakVarp = newBreakVar(nodep->fileline(), मिच्छा);
        taskp->addStmtsp(breakVarp);

        // Call the start production's task
        taskp->addStmtsp(newProdFuncRef(nodep, m_startProdp, breakVarp, nullptr));

        UINFOTREE(9, taskp, "newStart", "");
        पटिनिवत्त taskp;
    }

    Ast_ग्रन्थिः* newProdFuncRef(Ast_ग्रन्थिः* nodep, AstRSProd* prodp, AstVar* breakVarp,
                            AstArg* userArgsp) {
        auto it = m_prodFuncps.find(prodp);
        UASSERT_OBJ(it != m_prodFuncps.end(), nodep, "No production function made");
        Ast_ग्रन्थिः_FTask* ध्रुव prodFuncp = it->second;
        File_पङ्क्तिः* ध्रुव fl = nodep->fileline();
        // V3Width already ran before V3RandSequence, so VarRefs we create here
        // need dtype set explicitly, V3Broken later checks width == widthMin.
        AstVarRef* ध्रुव breakRefp = नूतन AstVarRef{fl, breakVarp, VAccess::WRITE};
        breakRefp->dtypeFrom(breakVarp);
        AstArg* ध्रुव argsp = नूतन AstArg{fl, breakVarp->name(), breakRefp};
        अत्थाय (ध्रुव auto& itr : m_localizeNames) {
            ध्रुव AstVar* ध्रुव lvarp = itr.second;
            AstVar* ध्रुव iovarp = m_localizeRemaps[lvarp];
            UASSERT_OBJ(iovarp, nodep, "No new port variable for local variable" << lvarp);
            AstVarRef* ध्रुव refp = नूतन AstVarRef{fl, iovarp, VAccess::READWRITE};
            refp->dtypeFrom(iovarp);
            argsp->addNext(नूतन AstArg{nodep->fileline(), "__Vrsarg_" + lvarp->name(), refp});
        }
        सचे (userArgsp) argsp->addNext(userArgsp);
        Ast_ग्रन्थिः* ध्रुव newp
            = नूतन Ast_Stmt_व्यञ्जकम्{fl, नूतन AstTaskRef{fl, VN_AS(prodFuncp, Task), argsp}};
        पटिनिवत्त newp;
    }

    शून्य newProdFunc(AstRSProd* nodep) {
        // Task, as time may pass
        AstTask* ध्रुव newp
            = नूतन AstTask{nodep->fileline(), m_rsp->name() + "_" + nodep->name(), nullptr};
        m_modp->addStmtsp(newp);
        m_prodFuncps.emplace(nodep, newp);
        UINFOTREE(9, newp, "newProd", "");
    }

    Ast_ग्रन्थिः* newProdRandJoin(AstRSProd* /*prodp*/, AstRSProdList* prodlistp) {
        // For weight == 1.0 longer sequence (favor stay in a)
        // For weight == 0.0 shorter squence (favor change a/b)
        UASSERT_OBJ(prodlistp->weightp(), prodlistp, "Weight should have default CONST(0.5)");
        Ast_ग्रन्थिः_व्यञ्जकम्* weightp = prodlistp->weightp()->unlinkFrBack();

        // Build map of production lists and productions we need to join
        // Must mainain the relative order of each sequence.
        std::deque<AstRSProdItem*> lists;
        std::unordered_map<AstRSProdItem*, std::deque<Ast_ग्रन्थिः_Stmt*>> listStmts;
        अत्थाय (AstRSProdItem* proditemp = VN_AS(prodlistp->prodsp(), RSProdItem); proditemp;
             proditemp = VN_AS(proditemp->nextp(), RSProdItem)) {
            lists.प्रक्षिप_back(proditemp);
            ध्रुव AstRSProd* ध्रुव subProdp = proditemp->prodp();
            सचे (!subProdp) अनुवर्त;
            सचे (!subProdp->rulesp()) अनुवर्त;
            सचे (!subProdp->rulesp()->prodlistsp()) अनुवर्त;
            अत्थाय (Ast_ग्रन्थिः_Stmt* subStmtp
                 = VN_AS(subProdp->rulesp()->prodlistsp()->prodsp(), ग्रन्थिः_Stmt);
                 subStmtp; subStmtp = VN_AS(subStmtp->nextp(), ग्रन्थिः_Stmt)) {
                listStmts[proditemp].प्रक्षिप_back(subStmtp);
            }
        }

        UINFO(9, "RandJoin productions called:");
        अत्थाय (AstRSProdItem* proditemp : lists) {
            UINFO(9, "  list " << proditemp);
            अत्थाय (ध्रुव Ast_ग्रन्थिः_Stmt* prodip : listStmts[proditemp])
                UINFO(9, "    calls " << prodip);
        }

        // Need to clone all nodes used

        File_पङ्क्तिः* fl = prodlistp->fileline();
        Ast_ग्रन्थिः* newp = nullptr;

        // "bool _Vjoin_repick = true;"  // Pick a new longer sequence
        AstVar* repickVarp = नूतन AstVar{fl, VVarType::VAR, "_Vjoin_repick", VFlagBitPacked{}, 1};
        repickVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        repickVarp->funcLocal(सच्च);
        m_breakVarp->addNextHere(repickVarp);  // Add to function top, not newp
        newp = Ast_ग्रन्थिः::addNext(newp,
                                नूतन AstAssign{fl, नूतन AstVarRef{fl, repickVarp, VAccess::WRITE},
                                              नूतन AstConst{fl, AstConst::BitTrue{}}});

        // "int _Vjoin_nleft_0 = N(items);"  // N(a) Number left to generate in list 0, starts at
        // N(list0)
        std::deque<AstVar*> nleftVarps;
        पूर्णाङ्क i = 0;
        अत्थाय (AstRSProdItem* proditemp : lists) {
            // 'rand join arule arule arule' is legal, so need numbered, not non-unique named
            // nleft's
            AstVar* ध्रुव varp = नूतन AstVar{
                fl, VVarType::VAR, "_Vjoin_nleft_" + std::to_string(i++), VFlagBitPacked{}, 32};
            varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            varp->funcLocal(सच्च);
            m_breakVarp->addNextHere(varp);  // Add to function top, not newp
            nleftVarps.प्रक्षिप_back(varp);

            newp = Ast_ग्रन्थिः::addNext(
                newp,
                नूतन AstAssign{fl, नूतन AstVarRef{fl, varp, VAccess::WRITE},
                              नूतन AstConst{fl, AstConst::Widthed_मानम्{}, 32,
                                           static_cast<uint32_t>(listStmts[proditemp].size())}});
        }

        // "int _Vjoin_picked_elist = 0;"  // 0 = pick first eligible (non-finished) list, 1 = pick
        // second, etc
        AstVar* ध्रुव pickedVarp
            = नूतन AstVar{fl, VVarType::VAR, "_Vjoin_picked_elist", VFlagBitPacked{}, 1};
        pickedVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        pickedVarp->funcLocal(सच्च);
        m_breakVarp->addNextHere(pickedVarp);  // Add to function top, not newp

        // "while(1) {"  // Generate until make complete sequence
        AstJumpBlock* ध्रुव whilep = नूतन AstJumpBlock{fl, nullptr};
        newp = Ast_ग्रन्थिः::addNext(newp, नूतन AstLoop{fl, whilep});
        {
            // "if ((std::rand() & 0xffffUL) > (0xffff * weight)) _Vjoin_repick = true;"
            whilep->addStmtsp(नूतन AstIf{
                fl,
                नूतन AstGt{
                    fl, नूतन AstRand{fl, AstRand::SRandomU32{}},
                    नूतन AstRToIS{fl, नूतन AstMulD{fl,
                                                 नूतन AstConst{fl, AstConst::RealDouble{},
                                                              static_cast<द्विगुण>(0xffffffffUL)},
                                                 weightp}}},
                नूतन AstAssign{fl, नूतन AstVarRef{fl, repickVarp, VAccess::WRITE},
                              नूतन AstConst{fl, AstConst::BitTrue{}}}});

            // "if (_Vjoin_repick) {"
            AstIf* ifp = नूतन AstIf{fl, नूतन AstVarRef{fl, repickVarp, VAccess::READ}};
            whilep->addStmtsp(ifp);
            {
                // "// _Vjoin_nlists_eligible is how many lists eligable for picking"
                // "int _Vjoin_nlists_eligible = 0;"
                AstVar* ध्रुव eligibleVarp = नूतन AstVar{
                    fl, VVarType::VAR, "_Vjoin_nlists_eligible", VFlagBitPacked{}, 32};
                eligibleVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                eligibleVarp->funcLocal(सच्च);
                m_breakVarp->addNextHere(eligibleVarp);  // Add to function top, not newp
                ifp->addThensp(नूतन AstAssign{fl, नूतन AstVarRef{fl, eligibleVarp, VAccess::WRITE},
                                             नूतन AstConst{fl, AstConst::Widthed_मानम्{}, 32, 0}});

                i = 0;
                अत्थाय (AstRSProdItem* proditemp : lists) {
                    (शून्य)proditemp;
                    // "_Vjoin_nlists_eligible += _Vjoin_nleft_0 ? 1 : 0;"
                    ifp->addThensp(नूतन AstAssign{
                        fl, नूतन AstVarRef{fl, eligibleVarp, VAccess::WRITE},
                        नूतन AstAdd{
                            fl, नूतन AstVarRef{fl, eligibleVarp, VAccess::READ},
                            नूतन AstCond{
                                fl,
                                नूतन AstNeq{fl, नूतन AstConst{fl, AstConst::Widthed_मानम्{}, 32, 0},
                                           नूतन AstVarRef{fl, nleftVarps[i], VAccess::READ}},
                                नूतन AstConst{fl, AstConst::Widthed_मानम्{}, 32, 1},
                                नूतन AstConst{fl, AstConst::Widthed_मानम्{}, 32, 0}}}});
                    ++i;
                }

                // "if (!_Vjoin_nlists_eligible) break;"  // Out of elements
                ifp->addThensp(
                    नूतन AstIf{fl,
                              नूतन AstEq{fl, नूतन AstConst{fl, AstConst::Widthed_मानम्{}, 32, 0},
                                        नूतन AstVarRef{fl, eligibleVarp, VAccess::READ}},
                              नूतन AstJumpGo{fl, m_jumpBlockp}});

                // "// +1 to simplify usage, so 0 = done"
                // "_Vjoin_picked_elist = 1 + (std::rand() % _Vjoin_nlists_eligible);"
                ifp->addThensp(नूतन AstAssign{
                    fl, नूतन AstVarRef{fl, pickedVarp, VAccess::WRITE},
                    नूतन AstAdd{fl, नूतन AstConst{fl, AstConst::Widthed_मानम्{}, 32, 1},
                               नूतन AstModDiv{fl, नूतन AstRand{fl, AstRand::SRandomU32{}},
                                             नूतन AstVarRef{fl, eligibleVarp, VAccess::READ}}}});

                // "_Vjoin_repick = false;
                ifp->addThensp(नूतन AstAssign{fl, नूतन AstVarRef{fl, repickVarp, VAccess::WRITE},
                                             नूतन AstConst{fl, AstConst::BitFalse{}}});
            }

            // "// Number of eligible lists left before hit _Vjoin_picked one"
            // "int _Vjoin_sel_elist = _Vjoin_picked_elist;"
            AstVar* ध्रुव selVarp
                = नूतन AstVar{fl, VVarType::VAR, "_Vjoin_sel_elist", VFlagBitPacked{}, 32};
            selVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            selVarp->funcLocal(सच्च);
            m_breakVarp->addNextHere(selVarp);  // Add to function top, not newp
            whilep->addStmtsp(नूतन AstAssign{fl, नूतन AstVarRef{fl, selVarp, VAccess::WRITE},
                                            नूतन AstVarRef{fl, pickedVarp, VAccess::READ}});

            i = -1;
            अत्थाय (AstRSProdItem* proditemp : lists) {
                ++i;
                // "if (_Vjoin_nleft_0) {"
                ifp = नूतन AstIf{fl,
                                नूतन AstNeq{fl, नूतन AstConst{fl, AstConst::Widthed_मानम्{}, 32, 0},
                                           नूतन AstVarRef{fl, nleftVarps[i], VAccess::READ}},
                                nullptr, nullptr};
                whilep->addStmtsp(ifp);
                {
                    // "--_Vjoin_sel_elist;"
                    ifp->addThensp(नूतन AstAssign{
                        fl, नूतन AstVarRef{fl, selVarp, VAccess::WRITE},
                        नूतन AstSub{fl, नूतन AstVarRef{fl, selVarp, VAccess::READ},
                                   नूतन AstConst{fl, AstConst::Widthed_मानम्{}, 32, 1}}});

                    // "if (_Vjoin_sel_elist == 0) {"
                    AstIf* ध्रुव jIfp = नूतन AstIf{
                        fl,
                        नूतन AstEq{fl, नूतन AstConst{fl, AstConst::Widthed_मानम्{}, 32, 0},
                                  नूतन AstVarRef{fl, selVarp, VAccess::READ}},
                        nullptr, nullptr};
                    ifp->addThensp(jIfp);
                    {
                        // "switch (_Vjoin_nleft_0) {"
                        //     "case 2 / * N(a)  * /: {statement}; break;"
                        //     "case 1 / * N(a) - 1 * /: {statement}; break;"
                        uint32_t j = static_cast<uint32_t>(listStmts[proditemp].size());
                        अत्थाय (Ast_ग्रन्थिः_Stmt* iprodp : listStmts[proditemp]) {
                            jIfp->addThensp(नूतन AstIf{
                                fl,
                                नूतन AstEq{fl, नूतन AstConst{fl, AstConst::Widthed_मानम्{}, 32, j},
                                          नूतन AstVarRef{fl, nleftVarps[i], VAccess::READ}},
                                iprodp->clone_वृक्षः(मिच्छा)});
                            --j;
                        }

                        // "--_Vjoin_nleft_0;"
                        jIfp->addThensp(नूतन AstAssign{
                            fl, नूतन AstVarRef{fl, nleftVarps[i], VAccess::WRITE},
                            नूतन AstSub{fl, नूतन AstVarRef{fl, nleftVarps[i], VAccess::READ},
                                       नूतन AstConst{fl, AstConst::Widthed_मानम्{}, 32, 1}}});

                        // "if (0 == _Vjoin_nleft_0) _Vjoin_repick = true;"
                        jIfp->addThensp(नूतन AstIf{
                            fl,
                            नूतन AstEq{fl, नूतन AstConst{fl, AstConst::Widthed_मानम्{}, 32, 0},
                                      नूतन AstVarRef{fl, nleftVarps[i], VAccess::READ}},
                            नूतन AstAssign{fl, नूतन AstVarRef{fl, repickVarp, VAccess::WRITE},
                                          नूतन AstConst{fl, AstConst::BitTrue{}}}});

                        // "continue;"
                        jIfp->addThensp(नूतन AstJumpGo{fl, whilep});
                    }
                }
            }
        }

        UINFOTREE(9, newp, "ForkJoin new", "-");
        पटिनिवत्त newp;
    }

    // VISITORS

    शून्य visit(Ast_ग्रन्थिः_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildren(nodep);
    }

    शून्य visit(AstRandSequence* nodep) override {
        UINFO(9, "visit " << nodep);
        UASSERT_OBJ(m_modp, nodep, "randsequence not under module");
        सचे (m_rsp) {
            // No examples found in the wild
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: randsequence under randsequence");
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep->unlinkFrBack()), nodep);
            पटिनिवत्त;
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
        सचे (nodep->prodp()) m_startProdp = nodep->prodp();  // Has a start production selected

        // Build production functions
        अत्थाय (AstRSProd* prodp = nodep->prodsp(); prodp; prodp = VN_AS(prodp->nextp(), RSProd)) {
            newProdFunc(prodp);
            // Don't iterate yet, need to have tasks we can reference
        }

        // Iterate all of the new productions, moving guts into functions just made
        // Must do those with 'rand join' first, as they need to see the raw AstRS* nodes
        // before they are removed.  (Alternative would be to keep them until the end).
        UINFOTREE(9, nodep, "RS Tree pre-it", "-");
        std::unordered_set<AstRSProd*> prodHasRandJoin;
        अत्थाय (AstRSProd* prodp = nodep->prodsp(); prodp; prodp = VN_AS(prodp->nextp(), RSProd)) {
            prodp->foreach([&](ध्रुव AstRSProdList* ध्रुव prodlistp) {
                सचे (prodlistp->randJoin()) prodHasRandJoin.emplace(prodp);
            });
        }
        अत्थाय (AstRSProd *nextp, *prodp = nodep->prodsp(); prodp; prodp = nextp) {
            nextp = VN_AS(prodp->nextp(), RSProd);
            सचे (prodHasRandJoin.गणनम्(prodp)) VL_DO_DANGLING(iterate(prodp), prodp);
        }
        अत्थाय (AstRSProd *nextp, *prodp = nodep->prodsp(); prodp; prodp = nextp) {
            nextp = VN_AS(prodp->nextp(), RSProd);
            सचे (!prodHasRandJoin.गणनम्(prodp)) VL_DO_DANGLING(iterate(prodp), prodp);
        }
        UINFOTREE(9, nodep, "RS Tree post-it", "-");

        // Replace randsequence with call to randsequence start function
        AstTask* ध्रुव startFuncp = newStartFunc(nodep);
        AstArg* argsp = nullptr;
        अत्थाय (auto& itr : m_localizeNames) {
            AstVar* ध्रुव lvarp = itr.second;
            argsp = Ast_ग्रन्थिः::addNext(
                argsp, नूतन AstArg{nodep->fileline(), "__Vrsarg_" + lvarp->name(),
                                  नूतन AstVarRef{nodep->fileline(), lvarp, VAccess::READWRITE}});
        }
        AstTaskRef* ध्रुव callStartp
            = नूतन AstTaskRef{nodep->fileline(), startFuncp->name(), argsp};
        callStartp->taskp(startFuncp);
        nodep->replaceWith(नूतन Ast_Stmt_व्यञ्जकम्{nodep->fileline(), callStartp});

        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
    }
    शून्य visit(AstRSProd* nodep) override {
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
        m_breakVarp = newBreakVar(nodep->fileline(), सच्च);
        m_prodFuncp->addStmtsp(m_breakVarp);

        // Production formal ports become real input ports on the generated task.
        // Must be added after m_breakVarp / localize ports so positional argument
        // order in newProdFuncRef stays consistent: [break, localize..., user...].
        सचे (Ast_ग्रन्थिः* ध्रुव portsp = nodep->portsp()) {
            portsp->unlinkFrBackWithNext();
            m_prodFuncp->addStmtsp(portsp);
            अत्थाय (Ast_ग्रन्थिः* itp = portsp; itp; itp = itp->nextp()) {
                AstVar* ध्रुव portVarp = VN_AS(itp, Var);
                portVarp->funcLocal(सच्च);
                portVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            }
        }

        // Put JumpBlock immediately under the new function to support
        // a future break/return.  V3Const will rip it out if unneeded.
        VL_RESTORER(m_jumpBlockp);
        m_jumpBlockp = नूतन AstJumpBlock{nodep->fileline(), nullptr};
        m_prodFuncp->addStmtsp(m_jumpBlockp);

        सचे (nodep->fvarp())
            nodep->fvarp()->v3warn(E_UNSUPPORTED,
                                   "Unsupported: randsequence production function variable");

        // Move children into m_prodFuncp, and iterate there
        सचे (!nodep->rulesp()) {  // Nothing to do
        } अञ्ञथा सचे (nodep->rulesp()->prodlistsp() && nodep->rulesp()->prodlistsp()->randJoin()) {
            AstRSProdList* ध्रुव prodlistp = nodep->rulesp()->prodlistsp();
            Ast_ग्रन्थिः* ध्रुव itemsp = newProdRandJoin(nodep, prodlistp);
            m_jumpBlockp->addStmtsp(itemsp);
            iterateAndNextNull(itemsp);
        } अञ्ञथा सचे (!nodep->rulesp()->nextp()) {  // Single rule/list, can just do it
            // RSPROD(RSRULE(weight, stmt)) -> IF(weight != 0, stmt)
            ध्रुव AstRSRule* ध्रुव rulep = nodep->rulesp();
            Ast_ग्रन्थिः* itemsp = nullptr;
            सचे (rulep->weightStmtsp()) itemsp = rulep->weightStmtsp()->unlinkFrBackWithNext();
            सचे (rulep->prodlistsp())
                itemsp = Ast_ग्रन्थिः::addNext(itemsp, rulep->prodlistsp()->unlinkFrBackWithNext());
            // Can ignore rulep->weightp() unless is zero, in which case noop, add if
            सचे (rulep->weightp())
                itemsp = नूतन AstIf{rulep->weightp()->fileline(),
                                   नूतन AstNeq{rulep->weightp()->fileline(),
                                              नूतन AstConst{rulep->weightp()->fileline(), 0},
                                              rulep->weightp()->unlinkFrBack()},
                                   itemsp};
            // Move prodlist to parent (m_prodFunc) and process
            सचे (itemsp) {
                UINFOTREE(9, itemsp, "additems", "-");
                m_jumpBlockp->addStmtsp(itemsp);
                iterateAndNextNull(itemsp);
                UINFOTREE(9, m_jumpBlockp, "jumpBlockNow", "-");
            }
        } अञ्ञथा {
            // List of rules with "OR", need to random-weight them
            // RSPROD(RSRULE(weight1, stmt!), RSRULE(weight2, stmt2))
            //   -> RANDCASE(CASEITEM(weight1, stmt1), CASEITEM(weight2, stmt2))
            AstRandCase* ध्रुव randcasep = नूतन AstRandCase{nodep->fileline(), nullptr};
            अत्थाय (AstRSRule* rulep = nodep->rulesp(); rulep;
                 rulep = VN_AS(rulep->nextp(), RSRule)) {
                Ast_ग्रन्थिः* itemsp = nullptr;
                सचे (rulep->weightStmtsp()) itemsp = rulep->weightStmtsp()->unlinkFrBackWithNext();
                सचे (rulep->prodlistsp())
                    itemsp = Ast_ग्रन्थिः::addNext(itemsp, rulep->prodlistsp()->unlinkFrBackWithNext());
                सचे (itemsp) {
                    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव weightp = rulep->weightp()
                                                     ? rulep->weightp()->unlinkFrBack()
                                                     : नूतन AstConst{rulep->fileline(), 1};
                    randcasep->addItemsp(नूतन AstCaseItem{rulep->fileline(), weightp, itemsp});
                }
            }
            m_jumpBlockp->addStmtsp(randcasep);
            iterateAndNextNull(randcasep);
        }

        // V3Task needs to know if we ended up making a recursive function
        m_prodFuncp->foreach([&](Ast_ग्रन्थिः_FTask_Ref* ध्रुव nodep) {
            सचे (nodep->taskp() == m_prodFuncp) m_prodFuncp->recursive(सच्च);
        });
        UINFOTREE(9, m_prodFuncp, "rsprod-task-done " << nodep, "-");

        // Done with production, should have moved everything to new task
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep->unlinkFrBack()), nodep);
    }
    शून्य visit(AstRSRule* nodep) override {
        // Rule of what to produce (as a list) under a production
        nodep->v3fatalSrc("randsequence rule should be iterated and deleted by AstRSProd");
    }
    शून्य visit(AstRSProdList* nodep) override {
        // List of productions to call, potentially with a weight
        UINFO(9, "visit " << nodep);
        UASSERT_OBJ(m_prodFuncp, nodep, "RSProdList not under production");

        // Move prodlist to parent (m_prodFunc) and process
        सचे (Ast_ग्रन्थिः* ध्रुव itemsp = nodep->prodsp()) {
            UASSERT_OBJ(!nodep->randJoin(), nodep,
                        "rand join should have been handled in visit(RSRule)");
            nodep->replaceWith(itemsp->unlinkFrBackWithNext());
            // Will soon iterate itemsp as part of normal visit process
            // These will become sequential in the generated task
        } अञ्ञथा {
            nodep->unlinkFrBack();
        }
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
    }
    शून्य visit(AstRSProdItem* nodep) override {
        // "call" to generate using another production
        UINFO(9, "visit " << nodep);
        UASSERT_OBJ(m_prodFuncp, nodep, "RSProdItem not under production");
        AstRSProd* ध्रुव foundp = nodep->prodp();
        UASSERT_OBJ(foundp, nodep, "Unlinked production reference");
        // Pass through caller-side argument list (IEEE 1800-2023 18.17.7).
        AstArg* ध्रुव userArgsp
            = nodep->argsp() ? nodep->argsp()->unlinkFrBackWithNext() : nullptr;
        // Convert to task call
        Ast_ग्रन्थिः* ध्रुव newp = newProdFuncRef(nodep, foundp, m_breakVarp, userArgsp);
        // The production might have done a "break;", skip other steps if so
        newp->addNext(नूतन AstIf{nodep->fileline(),
                                नूतन AstVarRef{nodep->fileline(), m_breakVarp, VAccess::READ},
                                नूतन AstJumpGo{nodep->fileline(), m_jumpBlockp}});

        nodep->replaceWith(newp);
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
    }

    शून्य visit(AstRSBreak* nodep) override {
        UASSERT_OBJ(m_jumpBlockp, nodep, "RSBreak not under production jump block");
        UASSERT_OBJ(m_breakVarp, nodep, "no break variable");
        Ast_ग्रन्थिः_Stmt* ध्रुव newp = नूतन AstAssign{
            nodep->fileline(), नूतन AstVarRef{nodep->fileline(), m_breakVarp, VAccess::WRITE},
            नूतन AstConst{nodep->fileline(), AstConst::BitTrue{}}};
        newp->addNext(नूतन AstJumpGo{nodep->fileline(), m_jumpBlockp});
        nodep->replaceWith(newp);
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
    }
    शून्य visit(AstRSReturn* nodep) override {
        UASSERT_OBJ(m_jumpBlockp, nodep, "RSReturn not under production jump block");
        nodep->replaceWith(नूतन AstJumpGo{nodep->fileline(), m_jumpBlockp});
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
    }
    शून्य visit(AstVarRef* nodep) override {
        // Change references to variables inside randsequence to references to
        // ref variables on the new task
        ध्रुव auto it = m_localizeRemaps.find(nodep->varp());
        सचे (it != m_localizeRemaps.end()) {
            AstVar* ध्रुव iovarp = it->second;
            UINFO(9, "VARREF remap " << nodep << " -> " << iovarp);
            nodep->varp(iovarp);
        }
    }

    शून्य visit(Ast_ग्रन्थिः* nodep) override { iterateChildren(nodep); }

सार्वजनिक:
    // CONSTRUCTORS
    explicit RandSequenceVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~RandSequenceVisitor() override = स्वभाव;
};

//######################################################################
// RandSequence method class functions

शून्य V3RandSequence::randSequenceNetlist(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { ध्रुव RandSequenceVisitor randSequenceVisitor{nodep}; }
    V3Global::dump_Check_Global_वृक्षः("randsequence", 0, dump_वृक्षः_Either_Level() >= 3);
}
