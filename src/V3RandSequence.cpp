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

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ಅನ್ತರ್ಭಾವ "verilatedos.h"

#ಅನ್ತರ್ಭಾವ "V3RandSequence.h"

#ಅನ್ತರ್ಭಾವ "V3Ast.h"
#ಅನ್ತರ್ಭಾವ "V3Error.h"
#ಅನ್ತರ್ಭಾವ "V3FileLine.h"
#ಅನ್ತರ್ಭಾವ "V3Global.h"

VL_DEFINE_DEBUG_FUNCTIONS;

// ######################################################################
//  Matcher classes (for suggestion matching)

ವರ್ಗ RSProdMatcher final : ಸಾರ್ವಜನಿಕ VNodeMatcher {
ಸಾರ್ವಜನಿಕ:
    ತಾರ್ಕಿಕ ಗಂಟು_Match(ಧ್ರುವ Ast_ಗಂಟು* nodep) ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ VN_IS(nodep, RSProd); }
};

//######################################################################
// Visitor that marks classes needing a randomize() method

ವರ್ಗ RandSequenceVisitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
    // STATE - global
    std::map<AstRSProd*, Ast_ಗಂಟು_FTask*> m_prodFuncps;  // Generated production functions

    // STATE - for current visit position (use VL_RESTORER)
    Ast_ಗಂಟು_Module* m_modp = nullptr;  // Current module
    AstRandSequence* m_rsp = nullptr;  // Current randsequence
    AstRSProd* m_startProdp = nullptr;  // Starting production
    Ast_ಗಂಟು_FTask* m_prodFuncp = nullptr;  // Production function being built
    AstJumpBlock* m_jumpBlockp = nullptr;  // Building jumpblock for return/break
    AstVar* m_breakVarp = nullptr;  // Break variable
    std::unordered_set<AstVar*> m_localizes;  // Variables to become function inouts
    std::map<std::string, AstVar*> m_localizeNames;  // Ordered names of function inouts
    std::unordered_map<ಧ್ರುವ AstVar*, AstVar*> m_localizeRemaps;  // New ports for old vars

    // METHODS
    ಶೂನ್ಯ findLocalizes(AstRandSequence* nodep) {
        std::set<AstVar*> localVars;
        nodep->foreach([&](Ast_ಗಂಟು* ಧ್ರುವ nodep) {
            ಯದಿ (ಧ್ರುವ AstVarRef* ಧ್ರುವ anodep = VN_CAST(nodep, VarRef)) {
                m_localizes.emplace(anodep->varp());
            } ಅನ್ಯಥಾ ಯದಿ (AstVar* ಧ್ರುವ anodep = VN_CAST(nodep, Var)) {
                localVars.emplace(anodep);
            }
        });
        // Clear every variable allocated in the RandSequence
        ಪ್ರತಿ (AstVar* varp : localVars) m_localizes.erase(varp);
        // Sort by name, so function arguments are stable
        ಪ್ರತಿ (AstVar* varp : m_localizes) {
            UINFO(9, "localize " << varp);
            m_localizeNames.emplace(varp->name(), varp);
        }
    }

    AstVar* newBreakVar(File_ಸಾಲು* fl, ತಾರ್ಕಿಕ asOutput) {
        AstVar* ಧ್ರುವ varp = ನೂತನ AstVar{fl, VVarType::PORT, "__VrsBreak", VFlagBitPacked{}, 1};
        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        varp->funcLocal(ಸತ್ಯ);
        ಯದಿ (asOutput) varp->direction(VDirection::OUTPUT);
        Ast_ಗಂಟು* outp = varp;
        outp->addNext(ನೂತನ AstAssign{fl, ನೂತನ AstVarRef{fl, varp, VAccess::WRITE},
                                    ನೂತನ AstConst{fl, AstConst::BitFalse{}}});

        // Also add arguments as next's
        ಪ್ರತಿ (ಧ್ರುವ auto& itr : m_localizeNames) {
            ಧ್ರುವ AstVar* ಧ್ರುವ lvarp = itr.second;
            AstVar* ಧ್ರುವ iovarp
                = ನೂತನ AstVar{fl, VVarType::PORT, "__Vrsarg_" + lvarp->name(), lvarp};
            iovarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            iovarp->funcLocal(ಸತ್ಯ);
            iovarp->direction(VDirection::REF);
            varp->addNext(iovarp);
            m_localizeRemaps.emplace(lvarp, iovarp);
        }
        ಪ್ರತ್ಯಾಗಮ varp;
    }

    AstTask* newStartFunc(AstRandSequence* nodep) {
        // Create wrapper that is called by the sequence
        AstTask* ಧ್ರುವ taskp = ನೂತನ AstTask{m_startProdp->fileline(), m_rsp->name(), nullptr};
        taskp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        m_modp->addStmtsp(taskp);

        // Create local (not output) break variable
        VL_RESTORER_COPY(m_localizeRemaps);
        AstVar* breakVarp = newBreakVar(nodep->fileline(), ಅಸತ್ಯ);
        taskp->addStmtsp(breakVarp);

        // Call the start production's task
        taskp->addStmtsp(newProdFuncRef(nodep, m_startProdp, breakVarp, nullptr));

        UINFOTREE(9, taskp, "newStart", "");
        ಪ್ರತ್ಯಾಗಮ taskp;
    }

    Ast_ಗಂಟು* newProdFuncRef(Ast_ಗಂಟು* nodep, AstRSProd* prodp, AstVar* breakVarp,
                            AstArg* userArgsp) {
        auto it = m_prodFuncps.find(prodp);
        UASSERT_OBJ(it != m_prodFuncps.end(), nodep, "No production function made");
        Ast_ಗಂಟು_FTask* ಧ್ರುವ prodFuncp = it->second;
        File_ಸಾಲು* ಧ್ರುವ fl = nodep->fileline();
        // V3Width already ran before V3RandSequence, so VarRefs we create here
        // need dtype set explicitly, V3Broken later checks width == widthMin.
        AstVarRef* ಧ್ರುವ breakRefp = ನೂತನ AstVarRef{fl, breakVarp, VAccess::WRITE};
        breakRefp->dtypeFrom(breakVarp);
        AstArg* ಧ್ರುವ argsp = ನೂತನ AstArg{fl, breakVarp->name(), breakRefp};
        ಪ್ರತಿ (ಧ್ರುವ auto& itr : m_localizeNames) {
            ಧ್ರುವ AstVar* ಧ್ರುವ lvarp = itr.second;
            AstVar* ಧ್ರುವ iovarp = m_localizeRemaps[lvarp];
            UASSERT_OBJ(iovarp, nodep, "No new port variable for local variable" << lvarp);
            AstVarRef* ಧ್ರುವ refp = ನೂತನ AstVarRef{fl, iovarp, VAccess::READWRITE};
            refp->dtypeFrom(iovarp);
            argsp->addNext(ನೂತನ AstArg{nodep->fileline(), "__Vrsarg_" + lvarp->name(), refp});
        }
        ಯದಿ (userArgsp) argsp->addNext(userArgsp);
        Ast_ಗಂಟು* ಧ್ರುವ newp
            = ನೂತನ Ast_Stmt_ಅಭಿವ್ಯಕ್ತಿ{fl, ನೂತನ AstTaskRef{fl, VN_AS(prodFuncp, Task), argsp}};
        ಪ್ರತ್ಯಾಗಮ newp;
    }

    ಶೂನ್ಯ newProdFunc(AstRSProd* nodep) {
        // Task, as time may pass
        AstTask* ಧ್ರುವ newp
            = ನೂತನ AstTask{nodep->fileline(), m_rsp->name() + "_" + nodep->name(), nullptr};
        m_modp->addStmtsp(newp);
        m_prodFuncps.emplace(nodep, newp);
        UINFOTREE(9, newp, "newProd", "");
    }

    Ast_ಗಂಟು* newProdRandJoin(AstRSProd* /*prodp*/, AstRSProdList* prodlistp) {
        // For weight == 1.0 longer sequence (favor stay in a)
        // For weight == 0.0 shorter squence (favor change a/b)
        UASSERT_OBJ(prodlistp->weightp(), prodlistp, "Weight should have default CONST(0.5)");
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* weightp = prodlistp->weightp()->unlinkFrBack();

        // Build map of production lists and productions we need to join
        // Must mainain the relative order of each sequence.
        std::deque<AstRSProdItem*> lists;
        std::unordered_map<AstRSProdItem*, std::deque<Ast_ಗಂಟು_Stmt*>> listStmts;
        ಪ್ರತಿ (AstRSProdItem* proditemp = VN_AS(prodlistp->prodsp(), RSProdItem); proditemp;
             proditemp = VN_AS(proditemp->nextp(), RSProdItem)) {
            lists.ತಳ್ಳು_back(proditemp);
            ಧ್ರುವ AstRSProd* ಧ್ರುವ subProdp = proditemp->prodp();
            ಯದಿ (!subProdp) ಅನುವರ್ತ;
            ಯದಿ (!subProdp->rulesp()) ಅನುವರ್ತ;
            ಯದಿ (!subProdp->rulesp()->prodlistsp()) ಅನುವರ್ತ;
            ಪ್ರತಿ (Ast_ಗಂಟು_Stmt* subStmtp
                 = VN_AS(subProdp->rulesp()->prodlistsp()->prodsp(), ಗಂಟು_Stmt);
                 subStmtp; subStmtp = VN_AS(subStmtp->nextp(), ಗಂಟು_Stmt)) {
                listStmts[proditemp].ತಳ್ಳು_back(subStmtp);
            }
        }

        UINFO(9, "RandJoin productions called:");
        ಪ್ರತಿ (AstRSProdItem* proditemp : lists) {
            UINFO(9, "  list " << proditemp);
            ಪ್ರತಿ (ಧ್ರುವ Ast_ಗಂಟು_Stmt* prodip : listStmts[proditemp])
                UINFO(9, "    calls " << prodip);
        }

        // Need to clone all nodes used

        File_ಸಾಲು* fl = prodlistp->fileline();
        Ast_ಗಂಟು* newp = nullptr;

        // "bool _Vjoin_repick = true;"  // Pick a new longer sequence
        AstVar* repickVarp = ನೂತನ AstVar{fl, VVarType::VAR, "_Vjoin_repick", VFlagBitPacked{}, 1};
        repickVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        repickVarp->funcLocal(ಸತ್ಯ);
        m_breakVarp->addNextHere(repickVarp);  // Add to function top, not newp
        newp = Ast_ಗಂಟು::addNext(newp,
                                ನೂತನ AstAssign{fl, ನೂತನ AstVarRef{fl, repickVarp, VAccess::WRITE},
                                              ನೂತನ AstConst{fl, AstConst::BitTrue{}}});

        // "int _Vjoin_nleft_0 = N(items);"  // N(a) Number left to generate in list 0, starts at
        // N(list0)
        std::deque<AstVar*> nleftVarps;
        ಪೂರ್ಣಾಙ್ಕ i = 0;
        ಪ್ರತಿ (AstRSProdItem* proditemp : lists) {
            // 'rand join arule arule arule' is legal, so need numbered, not non-unique named
            // nleft's
            AstVar* ಧ್ರುವ varp = ನೂತನ AstVar{
                fl, VVarType::VAR, "_Vjoin_nleft_" + std::to_string(i++), VFlagBitPacked{}, 32};
            varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            varp->funcLocal(ಸತ್ಯ);
            m_breakVarp->addNextHere(varp);  // Add to function top, not newp
            nleftVarps.ತಳ್ಳು_back(varp);

            newp = Ast_ಗಂಟು::addNext(
                newp,
                ನೂತನ AstAssign{fl, ನೂತನ AstVarRef{fl, varp, VAccess::WRITE},
                              ನೂತನ AstConst{fl, AstConst::Widthed_ಮೌಲ್ಯ{}, 32,
                                           static_cast<uint32_t>(listStmts[proditemp].size())}});
        }

        // "int _Vjoin_picked_elist = 0;"  // 0 = pick first eligible (non-finished) list, 1 = pick
        // second, etc
        AstVar* ಧ್ರುವ pickedVarp
            = ನೂತನ AstVar{fl, VVarType::VAR, "_Vjoin_picked_elist", VFlagBitPacked{}, 1};
        pickedVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        pickedVarp->funcLocal(ಸತ್ಯ);
        m_breakVarp->addNextHere(pickedVarp);  // Add to function top, not newp

        // "while(1) {"  // Generate until make complete sequence
        AstJumpBlock* ಧ್ರುವ whilep = ನೂತನ AstJumpBlock{fl, nullptr};
        newp = Ast_ಗಂಟು::addNext(newp, ನೂತನ AstLoop{fl, whilep});
        {
            // "if ((std::rand() & 0xffffUL) > (0xffff * weight)) _Vjoin_repick = true;"
            whilep->addStmtsp(ನೂತನ AstIf{
                fl,
                ನೂತನ AstGt{
                    fl, ನೂತನ AstRand{fl, AstRand::SRandomU32{}},
                    ನೂತನ AstRToIS{fl, ನೂತನ AstMulD{fl,
                                                 ನೂತನ AstConst{fl, AstConst::RealDouble{},
                                                              static_cast<ದ್ವಿಗುಣ>(0xffffffffUL)},
                                                 weightp}}},
                ನೂತನ AstAssign{fl, ನೂತನ AstVarRef{fl, repickVarp, VAccess::WRITE},
                              ನೂತನ AstConst{fl, AstConst::BitTrue{}}}});

            // "if (_Vjoin_repick) {"
            AstIf* ifp = ನೂತನ AstIf{fl, ನೂತನ AstVarRef{fl, repickVarp, VAccess::READ}};
            whilep->addStmtsp(ifp);
            {
                // "// _Vjoin_nlists_eligible is how many lists eligable for picking"
                // "int _Vjoin_nlists_eligible = 0;"
                AstVar* ಧ್ರುವ eligibleVarp = ನೂತನ AstVar{
                    fl, VVarType::VAR, "_Vjoin_nlists_eligible", VFlagBitPacked{}, 32};
                eligibleVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                eligibleVarp->funcLocal(ಸತ್ಯ);
                m_breakVarp->addNextHere(eligibleVarp);  // Add to function top, not newp
                ifp->addThensp(ನೂತನ AstAssign{fl, ನೂತನ AstVarRef{fl, eligibleVarp, VAccess::WRITE},
                                             ನೂತನ AstConst{fl, AstConst::Widthed_ಮೌಲ್ಯ{}, 32, 0}});

                i = 0;
                ಪ್ರತಿ (AstRSProdItem* proditemp : lists) {
                    (ಶೂನ್ಯ)proditemp;
                    // "_Vjoin_nlists_eligible += _Vjoin_nleft_0 ? 1 : 0;"
                    ifp->addThensp(ನೂತನ AstAssign{
                        fl, ನೂತನ AstVarRef{fl, eligibleVarp, VAccess::WRITE},
                        ನೂತನ AstAdd{
                            fl, ನೂತನ AstVarRef{fl, eligibleVarp, VAccess::READ},
                            ನೂತನ AstCond{
                                fl,
                                ನೂತನ AstNeq{fl, ನೂತನ AstConst{fl, AstConst::Widthed_ಮೌಲ್ಯ{}, 32, 0},
                                           ನೂತನ AstVarRef{fl, nleftVarps[i], VAccess::READ}},
                                ನೂತನ AstConst{fl, AstConst::Widthed_ಮೌಲ್ಯ{}, 32, 1},
                                ನೂತನ AstConst{fl, AstConst::Widthed_ಮೌಲ್ಯ{}, 32, 0}}}});
                    ++i;
                }

                // "if (!_Vjoin_nlists_eligible) break;"  // Out of elements
                ifp->addThensp(
                    ನೂತನ AstIf{fl,
                              ನೂತನ AstEq{fl, ನೂತನ AstConst{fl, AstConst::Widthed_ಮೌಲ್ಯ{}, 32, 0},
                                        ನೂತನ AstVarRef{fl, eligibleVarp, VAccess::READ}},
                              ನೂತನ AstJumpGo{fl, m_jumpBlockp}});

                // "// +1 to simplify usage, so 0 = done"
                // "_Vjoin_picked_elist = 1 + (std::rand() % _Vjoin_nlists_eligible);"
                ifp->addThensp(ನೂತನ AstAssign{
                    fl, ನೂತನ AstVarRef{fl, pickedVarp, VAccess::WRITE},
                    ನೂತನ AstAdd{fl, ನೂತನ AstConst{fl, AstConst::Widthed_ಮೌಲ್ಯ{}, 32, 1},
                               ನೂತನ AstModDiv{fl, ನೂತನ AstRand{fl, AstRand::SRandomU32{}},
                                             ನೂತನ AstVarRef{fl, eligibleVarp, VAccess::READ}}}});

                // "_Vjoin_repick = false;
                ifp->addThensp(ನೂತನ AstAssign{fl, ನೂತನ AstVarRef{fl, repickVarp, VAccess::WRITE},
                                             ನೂತನ AstConst{fl, AstConst::BitFalse{}}});
            }

            // "// Number of eligible lists left before hit _Vjoin_picked one"
            // "int _Vjoin_sel_elist = _Vjoin_picked_elist;"
            AstVar* ಧ್ರುವ selVarp
                = ನೂತನ AstVar{fl, VVarType::VAR, "_Vjoin_sel_elist", VFlagBitPacked{}, 32};
            selVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            selVarp->funcLocal(ಸತ್ಯ);
            m_breakVarp->addNextHere(selVarp);  // Add to function top, not newp
            whilep->addStmtsp(ನೂತನ AstAssign{fl, ನೂತನ AstVarRef{fl, selVarp, VAccess::WRITE},
                                            ನೂತನ AstVarRef{fl, pickedVarp, VAccess::READ}});

            i = -1;
            ಪ್ರತಿ (AstRSProdItem* proditemp : lists) {
                ++i;
                // "if (_Vjoin_nleft_0) {"
                ifp = ನೂತನ AstIf{fl,
                                ನೂತನ AstNeq{fl, ನೂತನ AstConst{fl, AstConst::Widthed_ಮೌಲ್ಯ{}, 32, 0},
                                           ನೂತನ AstVarRef{fl, nleftVarps[i], VAccess::READ}},
                                nullptr, nullptr};
                whilep->addStmtsp(ifp);
                {
                    // "--_Vjoin_sel_elist;"
                    ifp->addThensp(ನೂತನ AstAssign{
                        fl, ನೂತನ AstVarRef{fl, selVarp, VAccess::WRITE},
                        ನೂತನ AstSub{fl, ನೂತನ AstVarRef{fl, selVarp, VAccess::READ},
                                   ನೂತನ AstConst{fl, AstConst::Widthed_ಮೌಲ್ಯ{}, 32, 1}}});

                    // "if (_Vjoin_sel_elist == 0) {"
                    AstIf* ಧ್ರುವ jIfp = ನೂತನ AstIf{
                        fl,
                        ನೂತನ AstEq{fl, ನೂತನ AstConst{fl, AstConst::Widthed_ಮೌಲ್ಯ{}, 32, 0},
                                  ನೂತನ AstVarRef{fl, selVarp, VAccess::READ}},
                        nullptr, nullptr};
                    ifp->addThensp(jIfp);
                    {
                        // "switch (_Vjoin_nleft_0) {"
                        //     "case 2 / * N(a)  * /: {statement}; break;"
                        //     "case 1 / * N(a) - 1 * /: {statement}; break;"
                        uint32_t j = static_cast<uint32_t>(listStmts[proditemp].size());
                        ಪ್ರತಿ (Ast_ಗಂಟು_Stmt* iprodp : listStmts[proditemp]) {
                            jIfp->addThensp(ನೂತನ AstIf{
                                fl,
                                ನೂತನ AstEq{fl, ನೂತನ AstConst{fl, AstConst::Widthed_ಮೌಲ್ಯ{}, 32, j},
                                          ನೂತನ AstVarRef{fl, nleftVarps[i], VAccess::READ}},
                                iprodp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ)});
                            --j;
                        }

                        // "--_Vjoin_nleft_0;"
                        jIfp->addThensp(ನೂತನ AstAssign{
                            fl, ನೂತನ AstVarRef{fl, nleftVarps[i], VAccess::WRITE},
                            ನೂತನ AstSub{fl, ನೂತನ AstVarRef{fl, nleftVarps[i], VAccess::READ},
                                       ನೂತನ AstConst{fl, AstConst::Widthed_ಮೌಲ್ಯ{}, 32, 1}}});

                        // "if (0 == _Vjoin_nleft_0) _Vjoin_repick = true;"
                        jIfp->addThensp(ನೂತನ AstIf{
                            fl,
                            ನೂತನ AstEq{fl, ನೂತನ AstConst{fl, AstConst::Widthed_ಮೌಲ್ಯ{}, 32, 0},
                                      ನೂತನ AstVarRef{fl, nleftVarps[i], VAccess::READ}},
                            ನೂತನ AstAssign{fl, ನೂತನ AstVarRef{fl, repickVarp, VAccess::WRITE},
                                          ನೂತನ AstConst{fl, AstConst::BitTrue{}}}});

                        // "continue;"
                        jIfp->addThensp(ನೂತನ AstJumpGo{fl, whilep});
                    }
                }
            }
        }

        UINFOTREE(9, newp, "ForkJoin new", "-");
        ಪ್ರತ್ಯಾಗಮ newp;
    }

    // VISITORS

    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildren(nodep);
    }

    ಶೂನ್ಯ visit(AstRandSequence* nodep) override {
        UINFO(9, "visit " << nodep);
        UASSERT_OBJ(m_modp, nodep, "randsequence not under module");
        ಯದಿ (m_rsp) {
            // No examples found in the wild
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: randsequence under randsequence");
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlinkFrBack()), nodep);
            ಪ್ರತ್ಯಾಗಮ;
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
        ಯದಿ (nodep->prodp()) m_startProdp = nodep->prodp();  // Has a start production selected

        // Build production functions
        ಪ್ರತಿ (AstRSProd* prodp = nodep->prodsp(); prodp; prodp = VN_AS(prodp->nextp(), RSProd)) {
            newProdFunc(prodp);
            // Don't iterate yet, need to have tasks we can reference
        }

        // Iterate all of the new productions, moving guts into functions just made
        // Must do those with 'rand join' first, as they need to see the raw AstRS* nodes
        // before they are removed.  (Alternative would be to keep them until the end).
        UINFOTREE(9, nodep, "RS Tree pre-it", "-");
        std::unordered_set<AstRSProd*> prodHasRandJoin;
        ಪ್ರತಿ (AstRSProd* prodp = nodep->prodsp(); prodp; prodp = VN_AS(prodp->nextp(), RSProd)) {
            prodp->foreach([&](ಧ್ರುವ AstRSProdList* ಧ್ರುವ prodlistp) {
                ಯದಿ (prodlistp->randJoin()) prodHasRandJoin.emplace(prodp);
            });
        }
        ಪ್ರತಿ (AstRSProd *nextp, *prodp = nodep->prodsp(); prodp; prodp = nextp) {
            nextp = VN_AS(prodp->nextp(), RSProd);
            ಯದಿ (prodHasRandJoin.ಎಣಿಕೆ(prodp)) VL_DO_DANGLING(iterate(prodp), prodp);
        }
        ಪ್ರತಿ (AstRSProd *nextp, *prodp = nodep->prodsp(); prodp; prodp = nextp) {
            nextp = VN_AS(prodp->nextp(), RSProd);
            ಯದಿ (!prodHasRandJoin.ಎಣಿಕೆ(prodp)) VL_DO_DANGLING(iterate(prodp), prodp);
        }
        UINFOTREE(9, nodep, "RS Tree post-it", "-");

        // Replace randsequence with call to randsequence start function
        AstTask* ಧ್ರುವ startFuncp = newStartFunc(nodep);
        AstArg* argsp = nullptr;
        ಪ್ರತಿ (auto& itr : m_localizeNames) {
            AstVar* ಧ್ರುವ lvarp = itr.second;
            argsp = Ast_ಗಂಟು::addNext(
                argsp, ನೂತನ AstArg{nodep->fileline(), "__Vrsarg_" + lvarp->name(),
                                  ನೂತನ AstVarRef{nodep->fileline(), lvarp, VAccess::READWRITE}});
        }
        AstTaskRef* ಧ್ರುವ callStartp
            = ನೂತನ AstTaskRef{nodep->fileline(), startFuncp->name(), argsp};
        callStartp->taskp(startFuncp);
        nodep->replaceWith(ನೂತನ Ast_Stmt_ಅಭಿವ್ಯಕ್ತಿ{nodep->fileline(), callStartp});

        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }
    ಶೂನ್ಯ visit(AstRSProd* nodep) override {
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
        m_breakVarp = newBreakVar(nodep->fileline(), ಸತ್ಯ);
        m_prodFuncp->addStmtsp(m_breakVarp);

        // Production formal ports become real input ports on the generated task.
        // Must be added after m_breakVarp / localize ports so positional argument
        // order in newProdFuncRef stays consistent: [break, localize..., user...].
        ಯದಿ (Ast_ಗಂಟು* ಧ್ರುವ portsp = nodep->portsp()) {
            portsp->unlinkFrBackWithNext();
            m_prodFuncp->addStmtsp(portsp);
            ಪ್ರತಿ (Ast_ಗಂಟು* itp = portsp; itp; itp = itp->nextp()) {
                AstVar* ಧ್ರುವ portVarp = VN_AS(itp, Var);
                portVarp->funcLocal(ಸತ್ಯ);
                portVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            }
        }

        // Put JumpBlock immediately under the new function to support
        // a future break/return.  V3Const will rip it out if unneeded.
        VL_RESTORER(m_jumpBlockp);
        m_jumpBlockp = ನೂತನ AstJumpBlock{nodep->fileline(), nullptr};
        m_prodFuncp->addStmtsp(m_jumpBlockp);

        ಯದಿ (nodep->fvarp())
            nodep->fvarp()->v3warn(E_UNSUPPORTED,
                                   "Unsupported: randsequence production function variable");

        // Move children into m_prodFuncp, and iterate there
        ಯದಿ (!nodep->rulesp()) {  // Nothing to do
        } ಅನ್ಯಥಾ ಯದಿ (nodep->rulesp()->prodlistsp() && nodep->rulesp()->prodlistsp()->randJoin()) {
            AstRSProdList* ಧ್ರುವ prodlistp = nodep->rulesp()->prodlistsp();
            Ast_ಗಂಟು* ಧ್ರುವ itemsp = newProdRandJoin(nodep, prodlistp);
            m_jumpBlockp->addStmtsp(itemsp);
            iterateAndNextNull(itemsp);
        } ಅನ್ಯಥಾ ಯದಿ (!nodep->rulesp()->nextp()) {  // Single rule/list, can just do it
            // RSPROD(RSRULE(weight, stmt)) -> IF(weight != 0, stmt)
            ಧ್ರುವ AstRSRule* ಧ್ರುವ rulep = nodep->rulesp();
            Ast_ಗಂಟು* itemsp = nullptr;
            ಯದಿ (rulep->weightStmtsp()) itemsp = rulep->weightStmtsp()->unlinkFrBackWithNext();
            ಯದಿ (rulep->prodlistsp())
                itemsp = Ast_ಗಂಟು::addNext(itemsp, rulep->prodlistsp()->unlinkFrBackWithNext());
            // Can ignore rulep->weightp() unless is zero, in which case noop, add if
            ಯದಿ (rulep->weightp())
                itemsp = ನೂತನ AstIf{rulep->weightp()->fileline(),
                                   ನೂತನ AstNeq{rulep->weightp()->fileline(),
                                              ನೂತನ AstConst{rulep->weightp()->fileline(), 0},
                                              rulep->weightp()->unlinkFrBack()},
                                   itemsp};
            // Move prodlist to parent (m_prodFunc) and process
            ಯದಿ (itemsp) {
                UINFOTREE(9, itemsp, "additems", "-");
                m_jumpBlockp->addStmtsp(itemsp);
                iterateAndNextNull(itemsp);
                UINFOTREE(9, m_jumpBlockp, "jumpBlockNow", "-");
            }
        } ಅನ್ಯಥಾ {
            // List of rules with "OR", need to random-weight them
            // RSPROD(RSRULE(weight1, stmt!), RSRULE(weight2, stmt2))
            //   -> RANDCASE(CASEITEM(weight1, stmt1), CASEITEM(weight2, stmt2))
            AstRandCase* ಧ್ರುವ randcasep = ನೂತನ AstRandCase{nodep->fileline(), nullptr};
            ಪ್ರತಿ (AstRSRule* rulep = nodep->rulesp(); rulep;
                 rulep = VN_AS(rulep->nextp(), RSRule)) {
                Ast_ಗಂಟು* itemsp = nullptr;
                ಯದಿ (rulep->weightStmtsp()) itemsp = rulep->weightStmtsp()->unlinkFrBackWithNext();
                ಯದಿ (rulep->prodlistsp())
                    itemsp = Ast_ಗಂಟು::addNext(itemsp, rulep->prodlistsp()->unlinkFrBackWithNext());
                ಯದಿ (itemsp) {
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ weightp = rulep->weightp()
                                                     ? rulep->weightp()->unlinkFrBack()
                                                     : ನೂತನ AstConst{rulep->fileline(), 1};
                    randcasep->addItemsp(ನೂತನ AstCaseItem{rulep->fileline(), weightp, itemsp});
                }
            }
            m_jumpBlockp->addStmtsp(randcasep);
            iterateAndNextNull(randcasep);
        }

        // V3Task needs to know if we ended up making a recursive function
        m_prodFuncp->foreach([&](Ast_ಗಂಟು_FTask_Ref* ಧ್ರುವ nodep) {
            ಯದಿ (nodep->taskp() == m_prodFuncp) m_prodFuncp->recursive(ಸತ್ಯ);
        });
        UINFOTREE(9, m_prodFuncp, "rsprod-task-done " << nodep, "-");

        // Done with production, should have moved everything to new task
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlinkFrBack()), nodep);
    }
    ಶೂನ್ಯ visit(AstRSRule* nodep) override {
        // Rule of what to produce (as a list) under a production
        nodep->v3fatalSrc("randsequence rule should be iterated and deleted by AstRSProd");
    }
    ಶೂನ್ಯ visit(AstRSProdList* nodep) override {
        // List of productions to call, potentially with a weight
        UINFO(9, "visit " << nodep);
        UASSERT_OBJ(m_prodFuncp, nodep, "RSProdList not under production");

        // Move prodlist to parent (m_prodFunc) and process
        ಯದಿ (Ast_ಗಂಟು* ಧ್ರುವ itemsp = nodep->prodsp()) {
            UASSERT_OBJ(!nodep->randJoin(), nodep,
                        "rand join should have been handled in visit(RSRule)");
            nodep->replaceWith(itemsp->unlinkFrBackWithNext());
            // Will soon iterate itemsp as part of normal visit process
            // These will become sequential in the generated task
        } ಅನ್ಯಥಾ {
            nodep->unlinkFrBack();
        }
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }
    ಶೂನ್ಯ visit(AstRSProdItem* nodep) override {
        // "call" to generate using another production
        UINFO(9, "visit " << nodep);
        UASSERT_OBJ(m_prodFuncp, nodep, "RSProdItem not under production");
        AstRSProd* ಧ್ರುವ foundp = nodep->prodp();
        UASSERT_OBJ(foundp, nodep, "Unlinked production reference");
        // Pass through caller-side argument list (IEEE 1800-2023 18.17.7).
        AstArg* ಧ್ರುವ userArgsp
            = nodep->argsp() ? nodep->argsp()->unlinkFrBackWithNext() : nullptr;
        // Convert to task call
        Ast_ಗಂಟು* ಧ್ರುವ newp = newProdFuncRef(nodep, foundp, m_breakVarp, userArgsp);
        // The production might have done a "break;", skip other steps if so
        newp->addNext(ನೂತನ AstIf{nodep->fileline(),
                                ನೂತನ AstVarRef{nodep->fileline(), m_breakVarp, VAccess::READ},
                                ನೂತನ AstJumpGo{nodep->fileline(), m_jumpBlockp}});

        nodep->replaceWith(newp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }

    ಶೂನ್ಯ visit(AstRSBreak* nodep) override {
        UASSERT_OBJ(m_jumpBlockp, nodep, "RSBreak not under production jump block");
        UASSERT_OBJ(m_breakVarp, nodep, "no break variable");
        Ast_ಗಂಟು_Stmt* ಧ್ರುವ newp = ನೂತನ AstAssign{
            nodep->fileline(), ನೂತನ AstVarRef{nodep->fileline(), m_breakVarp, VAccess::WRITE},
            ನೂತನ AstConst{nodep->fileline(), AstConst::BitTrue{}}};
        newp->addNext(ನೂತನ AstJumpGo{nodep->fileline(), m_jumpBlockp});
        nodep->replaceWith(newp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }
    ಶೂನ್ಯ visit(AstRSReturn* nodep) override {
        UASSERT_OBJ(m_jumpBlockp, nodep, "RSReturn not under production jump block");
        nodep->replaceWith(ನೂತನ AstJumpGo{nodep->fileline(), m_jumpBlockp});
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }
    ಶೂನ್ಯ visit(AstVarRef* nodep) override {
        // Change references to variables inside randsequence to references to
        // ref variables on the new task
        ಧ್ರುವ auto it = m_localizeRemaps.find(nodep->varp());
        ಯದಿ (it != m_localizeRemaps.end()) {
            AstVar* ಧ್ರುವ iovarp = it->second;
            UINFO(9, "VARREF remap " << nodep << " -> " << iovarp);
            nodep->varp(iovarp);
        }
    }

    ಶೂನ್ಯ visit(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    explicit RandSequenceVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~RandSequenceVisitor() override = ಸ್ವಭಾವ;
};

//######################################################################
// RandSequence method class functions

ಶೂನ್ಯ V3RandSequence::randSequenceNetlist(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { ಧ್ರುವ RandSequenceVisitor randSequenceVisitor{nodep}; }
    V3Global::dump_Check_Global_ವೃಕ್ಷ("randsequence", 0, dump_ವೃಕ್ಷ_Either_Level() >= 3);
}
