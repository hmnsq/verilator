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

#ଅନ୍ତର୍ଭାଵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ଅନ୍ତର୍ଭାଵ "verilatedos.h"

#ଅନ୍ତର୍ଭାଵ "V3RandSequence.h"

#ଅନ୍ତର୍ଭାଵ "V3Ast.h"
#ଅନ୍ତର୍ଭାଵ "V3Error.h"
#ଅନ୍ତର୍ଭାଵ "V3FileLine.h"
#ଅନ୍ତର୍ଭାଵ "V3Global.h"

VL_DEFINE_DEBUG_FUNCTIONS;

// ######################################################################
//  Matcher classes (for suggestion matching)

ଶ୍ରେଣୀ RSProdMatcher final : ସର୍ବସାଧାରଣ VNodeMatcher {
ସର୍ବସାଧାରଣ:
    ତାର୍କିକ ଗଣ୍ଠି_Match(ଅଚଳ Ast_ଗଣ୍ଠି* nodep) ଅଚଳ override { ଫେରନ୍ତୁ VN_IS(nodep, RSProd); }
};

//######################################################################
// Visitor that marks classes needing a randomize() method

ଶ୍ରେଣୀ RandSequenceVisitor final : ସର୍ବସାଧାରଣ VNVisitor {
    // STATE - global
    std::map<AstRSProd*, Ast_ଗଣ୍ଠି_FTask*> m_prodFuncps;  // Generated production functions

    // STATE - for current visit position (use VL_RESTORER)
    Ast_ଗଣ୍ଠି_Module* m_modp = nullptr;  // Current module
    AstRandSequence* m_rsp = nullptr;  // Current randsequence
    AstRSProd* m_startProdp = nullptr;  // Starting production
    Ast_ଗଣ୍ଠି_FTask* m_prodFuncp = nullptr;  // Production function being built
    AstJumpBlock* m_jumpBlockp = nullptr;  // Building jumpblock for return/break
    AstVar* m_breakVarp = nullptr;  // Break variable
    std::unordered_set<AstVar*> m_localizes;  // Variables to become function inouts
    std::map<std::string, AstVar*> m_localizeNames;  // Ordered names of function inouts
    std::unordered_map<ଅଚଳ AstVar*, AstVar*> m_localizeRemaps;  // New ports for old vars

    // METHODS
    ଶୂନ୍ୟ findLocalizes(AstRandSequence* nodep) {
        std::set<AstVar*> localVars;
        nodep->foreach([&](Ast_ଗଣ୍ଠି* ଅଚଳ nodep) {
            ଯଦି (ଅଚଳ AstVarRef* ଅଚଳ anodep = VN_CAST(nodep, VarRef)) {
                m_localizes.emplace(anodep->varp());
            } ଅନ୍ୟଥା ଯଦି (AstVar* ଅଚଳ anodep = VN_CAST(nodep, Var)) {
                localVars.emplace(anodep);
            }
        });
        // Clear every variable allocated in the RandSequence
        ପାଇଁ (AstVar* varp : localVars) m_localizes.erase(varp);
        // Sort by name, so function arguments are stable
        ପାଇଁ (AstVar* varp : m_localizes) {
            UINFO(9, "localize " << varp);
            m_localizeNames.emplace(varp->name(), varp);
        }
    }

    AstVar* newBreakVar(File_ଧାଡ଼ି* fl, ତାର୍କିକ asOutput) {
        AstVar* ଅଚଳ varp = ନୂତନ AstVar{fl, VVarType::PORT, "__VrsBreak", VFlagBitPacked{}, 1};
        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        varp->funcLocal(ସତ୍ୟ);
        ଯଦି (asOutput) varp->direction(VDirection::OUTPUT);
        Ast_ଗଣ୍ଠି* outp = varp;
        outp->addNext(ନୂତନ AstAssign{fl, ନୂତନ AstVarRef{fl, varp, VAccess::WRITE},
                                    ନୂତନ AstConst{fl, AstConst::BitFalse{}}});

        // Also add arguments as next's
        ପାଇଁ (ଅଚଳ auto& itr : m_localizeNames) {
            ଅଚଳ AstVar* ଅଚଳ lvarp = itr.second;
            AstVar* ଅଚଳ iovarp
                = ନୂତନ AstVar{fl, VVarType::PORT, "__Vrsarg_" + lvarp->name(), lvarp};
            iovarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            iovarp->funcLocal(ସତ୍ୟ);
            iovarp->direction(VDirection::REF);
            varp->addNext(iovarp);
            m_localizeRemaps.emplace(lvarp, iovarp);
        }
        ଫେରନ୍ତୁ varp;
    }

    AstTask* newStartFunc(AstRandSequence* nodep) {
        // Create wrapper that is called by the sequence
        AstTask* ଅଚଳ taskp = ନୂତନ AstTask{m_startProdp->fileline(), m_rsp->name(), nullptr};
        taskp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        m_modp->addStmtsp(taskp);

        // Create local (not output) break variable
        VL_RESTORER_COPY(m_localizeRemaps);
        AstVar* breakVarp = newBreakVar(nodep->fileline(), ମିଥ୍ୟା);
        taskp->addStmtsp(breakVarp);

        // Call the start production's task
        taskp->addStmtsp(newProdFuncRef(nodep, m_startProdp, breakVarp, nullptr));

        UINFOTREE(9, taskp, "newStart", "");
        ଫେରନ୍ତୁ taskp;
    }

    Ast_ଗଣ୍ଠି* newProdFuncRef(Ast_ଗଣ୍ଠି* nodep, AstRSProd* prodp, AstVar* breakVarp,
                            AstArg* userArgsp) {
        auto it = m_prodFuncps.find(prodp);
        UASSERT_OBJ(it != m_prodFuncps.end(), nodep, "No production function made");
        Ast_ଗଣ୍ଠି_FTask* ଅଚଳ prodFuncp = it->second;
        File_ଧାଡ଼ି* ଅଚଳ fl = nodep->fileline();
        // V3Width already ran before V3RandSequence, so VarRefs we create here
        // need dtype set explicitly, V3Broken later checks width == widthMin.
        AstVarRef* ଅଚଳ breakRefp = ନୂତନ AstVarRef{fl, breakVarp, VAccess::WRITE};
        breakRefp->dtypeFrom(breakVarp);
        AstArg* ଅଚଳ argsp = ନୂତନ AstArg{fl, breakVarp->name(), breakRefp};
        ପାଇଁ (ଅଚଳ auto& itr : m_localizeNames) {
            ଅଚଳ AstVar* ଅଚଳ lvarp = itr.second;
            AstVar* ଅଚଳ iovarp = m_localizeRemaps[lvarp];
            UASSERT_OBJ(iovarp, nodep, "No new port variable for local variable" << lvarp);
            AstVarRef* ଅଚଳ refp = ନୂତନ AstVarRef{fl, iovarp, VAccess::READWRITE};
            refp->dtypeFrom(iovarp);
            argsp->addNext(ନୂତନ AstArg{nodep->fileline(), "__Vrsarg_" + lvarp->name(), refp});
        }
        ଯଦି (userArgsp) argsp->addNext(userArgsp);
        Ast_ଗଣ୍ଠି* ଅଚଳ newp
            = ନୂତନ Ast_Stmt_ପଦସମଷ୍ଟି{fl, ନୂତନ AstTaskRef{fl, VN_AS(prodFuncp, Task), argsp}};
        ଫେରନ୍ତୁ newp;
    }

    ଶୂନ୍ୟ newProdFunc(AstRSProd* nodep) {
        // Task, as time may pass
        AstTask* ଅଚଳ newp
            = ନୂତନ AstTask{nodep->fileline(), m_rsp->name() + "_" + nodep->name(), nullptr};
        m_modp->addStmtsp(newp);
        m_prodFuncps.emplace(nodep, newp);
        UINFOTREE(9, newp, "newProd", "");
    }

    Ast_ଗଣ୍ଠି* newProdRandJoin(AstRSProd* /*prodp*/, AstRSProdList* prodlistp) {
        // For weight == 1.0 longer sequence (favor stay in a)
        // For weight == 0.0 shorter squence (favor change a/b)
        UASSERT_OBJ(prodlistp->weightp(), prodlistp, "Weight should have default CONST(0.5)");
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* weightp = prodlistp->weightp()->unlinkFrBack();

        // Build map of production lists and productions we need to join
        // Must mainain the relative order of each sequence.
        std::deque<AstRSProdItem*> lists;
        std::unordered_map<AstRSProdItem*, std::deque<Ast_ଗଣ୍ଠି_Stmt*>> listStmts;
        ପାଇଁ (AstRSProdItem* proditemp = VN_AS(prodlistp->prodsp(), RSProdItem); proditemp;
             proditemp = VN_AS(proditemp->nextp(), RSProdItem)) {
            lists.ଠେଲ_back(proditemp);
            ଅଚଳ AstRSProd* ଅଚଳ subProdp = proditemp->prodp();
            ଯଦି (!subProdp) ଜାରି;
            ଯଦି (!subProdp->rulesp()) ଜାରି;
            ଯଦି (!subProdp->rulesp()->prodlistsp()) ଜାରି;
            ପାଇଁ (Ast_ଗଣ୍ଠି_Stmt* subStmtp
                 = VN_AS(subProdp->rulesp()->prodlistsp()->prodsp(), ଗଣ୍ଠି_Stmt);
                 subStmtp; subStmtp = VN_AS(subStmtp->nextp(), ଗଣ୍ଠି_Stmt)) {
                listStmts[proditemp].ଠେଲ_back(subStmtp);
            }
        }

        UINFO(9, "RandJoin productions called:");
        ପାଇଁ (AstRSProdItem* proditemp : lists) {
            UINFO(9, "  list " << proditemp);
            ପାଇଁ (ଅଚଳ Ast_ଗଣ୍ଠି_Stmt* prodip : listStmts[proditemp])
                UINFO(9, "    calls " << prodip);
        }

        // Need to clone all nodes used

        File_ଧାଡ଼ି* fl = prodlistp->fileline();
        Ast_ଗଣ୍ଠି* newp = nullptr;

        // "bool _Vjoin_repick = true;"  // Pick a new longer sequence
        AstVar* repickVarp = ନୂତନ AstVar{fl, VVarType::VAR, "_Vjoin_repick", VFlagBitPacked{}, 1};
        repickVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        repickVarp->funcLocal(ସତ୍ୟ);
        m_breakVarp->addNextHere(repickVarp);  // Add to function top, not newp
        newp = Ast_ଗଣ୍ଠି::addNext(newp,
                                ନୂତନ AstAssign{fl, ନୂତନ AstVarRef{fl, repickVarp, VAccess::WRITE},
                                              ନୂତନ AstConst{fl, AstConst::BitTrue{}}});

        // "int _Vjoin_nleft_0 = N(items);"  // N(a) Number left to generate in list 0, starts at
        // N(list0)
        std::deque<AstVar*> nleftVarps;
        ପୂର୍ଣ୍ଣସଂଖ୍ୟା i = 0;
        ପାଇଁ (AstRSProdItem* proditemp : lists) {
            // 'rand join arule arule arule' is legal, so need numbered, not non-unique named
            // nleft's
            AstVar* ଅଚଳ varp = ନୂତନ AstVar{
                fl, VVarType::VAR, "_Vjoin_nleft_" + std::to_string(i++), VFlagBitPacked{}, 32};
            varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            varp->funcLocal(ସତ୍ୟ);
            m_breakVarp->addNextHere(varp);  // Add to function top, not newp
            nleftVarps.ଠେଲ_back(varp);

            newp = Ast_ଗଣ୍ଠି::addNext(
                newp,
                ନୂତନ AstAssign{fl, ନୂତନ AstVarRef{fl, varp, VAccess::WRITE},
                              ନୂତନ AstConst{fl, AstConst::Widthed_ମୂଲ୍ୟ{}, 32,
                                           static_cast<uint32_t>(listStmts[proditemp].size())}});
        }

        // "int _Vjoin_picked_elist = 0;"  // 0 = pick first eligible (non-finished) list, 1 = pick
        // second, etc
        AstVar* ଅଚଳ pickedVarp
            = ନୂତନ AstVar{fl, VVarType::VAR, "_Vjoin_picked_elist", VFlagBitPacked{}, 1};
        pickedVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        pickedVarp->funcLocal(ସତ୍ୟ);
        m_breakVarp->addNextHere(pickedVarp);  // Add to function top, not newp

        // "while(1) {"  // Generate until make complete sequence
        AstJumpBlock* ଅଚଳ whilep = ନୂତନ AstJumpBlock{fl, nullptr};
        newp = Ast_ଗଣ୍ଠି::addNext(newp, ନୂତନ AstLoop{fl, whilep});
        {
            // "if ((std::rand() & 0xffffUL) > (0xffff * weight)) _Vjoin_repick = true;"
            whilep->addStmtsp(ନୂତନ AstIf{
                fl,
                ନୂତନ AstGt{
                    fl, ନୂତନ AstRand{fl, AstRand::SRandomU32{}},
                    ନୂତନ AstRToIS{fl, ନୂତନ AstMulD{fl,
                                                 ନୂତନ AstConst{fl, AstConst::RealDouble{},
                                                              static_cast<ଦ୍ୱିଗୁଣ>(0xffffffffUL)},
                                                 weightp}}},
                ନୂତନ AstAssign{fl, ନୂତନ AstVarRef{fl, repickVarp, VAccess::WRITE},
                              ନୂତନ AstConst{fl, AstConst::BitTrue{}}}});

            // "if (_Vjoin_repick) {"
            AstIf* ifp = ନୂତନ AstIf{fl, ନୂତନ AstVarRef{fl, repickVarp, VAccess::READ}};
            whilep->addStmtsp(ifp);
            {
                // "// _Vjoin_nlists_eligible is how many lists eligable for picking"
                // "int _Vjoin_nlists_eligible = 0;"
                AstVar* ଅଚଳ eligibleVarp = ନୂତନ AstVar{
                    fl, VVarType::VAR, "_Vjoin_nlists_eligible", VFlagBitPacked{}, 32};
                eligibleVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                eligibleVarp->funcLocal(ସତ୍ୟ);
                m_breakVarp->addNextHere(eligibleVarp);  // Add to function top, not newp
                ifp->addThensp(ନୂତନ AstAssign{fl, ନୂତନ AstVarRef{fl, eligibleVarp, VAccess::WRITE},
                                             ନୂତନ AstConst{fl, AstConst::Widthed_ମୂଲ୍ୟ{}, 32, 0}});

                i = 0;
                ପାଇଁ (AstRSProdItem* proditemp : lists) {
                    (ଶୂନ୍ୟ)proditemp;
                    // "_Vjoin_nlists_eligible += _Vjoin_nleft_0 ? 1 : 0;"
                    ifp->addThensp(ନୂତନ AstAssign{
                        fl, ନୂତନ AstVarRef{fl, eligibleVarp, VAccess::WRITE},
                        ନୂତନ AstAdd{
                            fl, ନୂତନ AstVarRef{fl, eligibleVarp, VAccess::READ},
                            ନୂତନ AstCond{
                                fl,
                                ନୂତନ AstNeq{fl, ନୂତନ AstConst{fl, AstConst::Widthed_ମୂଲ୍ୟ{}, 32, 0},
                                           ନୂତନ AstVarRef{fl, nleftVarps[i], VAccess::READ}},
                                ନୂତନ AstConst{fl, AstConst::Widthed_ମୂଲ୍ୟ{}, 32, 1},
                                ନୂତନ AstConst{fl, AstConst::Widthed_ମୂଲ୍ୟ{}, 32, 0}}}});
                    ++i;
                }

                // "if (!_Vjoin_nlists_eligible) break;"  // Out of elements
                ifp->addThensp(
                    ନୂତନ AstIf{fl,
                              ନୂତନ AstEq{fl, ନୂତନ AstConst{fl, AstConst::Widthed_ମୂଲ୍ୟ{}, 32, 0},
                                        ନୂତନ AstVarRef{fl, eligibleVarp, VAccess::READ}},
                              ନୂତନ AstJumpGo{fl, m_jumpBlockp}});

                // "// +1 to simplify usage, so 0 = done"
                // "_Vjoin_picked_elist = 1 + (std::rand() % _Vjoin_nlists_eligible);"
                ifp->addThensp(ନୂତନ AstAssign{
                    fl, ନୂତନ AstVarRef{fl, pickedVarp, VAccess::WRITE},
                    ନୂତନ AstAdd{fl, ନୂତନ AstConst{fl, AstConst::Widthed_ମୂଲ୍ୟ{}, 32, 1},
                               ନୂତନ AstModDiv{fl, ନୂତନ AstRand{fl, AstRand::SRandomU32{}},
                                             ନୂତନ AstVarRef{fl, eligibleVarp, VAccess::READ}}}});

                // "_Vjoin_repick = false;
                ifp->addThensp(ନୂତନ AstAssign{fl, ନୂତନ AstVarRef{fl, repickVarp, VAccess::WRITE},
                                             ନୂତନ AstConst{fl, AstConst::BitFalse{}}});
            }

            // "// Number of eligible lists left before hit _Vjoin_picked one"
            // "int _Vjoin_sel_elist = _Vjoin_picked_elist;"
            AstVar* ଅଚଳ selVarp
                = ନୂତନ AstVar{fl, VVarType::VAR, "_Vjoin_sel_elist", VFlagBitPacked{}, 32};
            selVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            selVarp->funcLocal(ସତ୍ୟ);
            m_breakVarp->addNextHere(selVarp);  // Add to function top, not newp
            whilep->addStmtsp(ନୂତନ AstAssign{fl, ନୂତନ AstVarRef{fl, selVarp, VAccess::WRITE},
                                            ନୂତନ AstVarRef{fl, pickedVarp, VAccess::READ}});

            i = -1;
            ପାଇଁ (AstRSProdItem* proditemp : lists) {
                ++i;
                // "if (_Vjoin_nleft_0) {"
                ifp = ନୂତନ AstIf{fl,
                                ନୂତନ AstNeq{fl, ନୂତନ AstConst{fl, AstConst::Widthed_ମୂଲ୍ୟ{}, 32, 0},
                                           ନୂତନ AstVarRef{fl, nleftVarps[i], VAccess::READ}},
                                nullptr, nullptr};
                whilep->addStmtsp(ifp);
                {
                    // "--_Vjoin_sel_elist;"
                    ifp->addThensp(ନୂତନ AstAssign{
                        fl, ନୂତନ AstVarRef{fl, selVarp, VAccess::WRITE},
                        ନୂତନ AstSub{fl, ନୂତନ AstVarRef{fl, selVarp, VAccess::READ},
                                   ନୂତନ AstConst{fl, AstConst::Widthed_ମୂଲ୍ୟ{}, 32, 1}}});

                    // "if (_Vjoin_sel_elist == 0) {"
                    AstIf* ଅଚଳ jIfp = ନୂତନ AstIf{
                        fl,
                        ନୂତନ AstEq{fl, ନୂତନ AstConst{fl, AstConst::Widthed_ମୂଲ୍ୟ{}, 32, 0},
                                  ନୂତନ AstVarRef{fl, selVarp, VAccess::READ}},
                        nullptr, nullptr};
                    ifp->addThensp(jIfp);
                    {
                        // "switch (_Vjoin_nleft_0) {"
                        //     "case 2 / * N(a)  * /: {statement}; break;"
                        //     "case 1 / * N(a) - 1 * /: {statement}; break;"
                        uint32_t j = static_cast<uint32_t>(listStmts[proditemp].size());
                        ପାଇଁ (Ast_ଗଣ୍ଠି_Stmt* iprodp : listStmts[proditemp]) {
                            jIfp->addThensp(ନୂତନ AstIf{
                                fl,
                                ନୂତନ AstEq{fl, ନୂତନ AstConst{fl, AstConst::Widthed_ମୂଲ୍ୟ{}, 32, j},
                                          ନୂତନ AstVarRef{fl, nleftVarps[i], VAccess::READ}},
                                iprodp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା)});
                            --j;
                        }

                        // "--_Vjoin_nleft_0;"
                        jIfp->addThensp(ନୂତନ AstAssign{
                            fl, ନୂତନ AstVarRef{fl, nleftVarps[i], VAccess::WRITE},
                            ନୂତନ AstSub{fl, ନୂତନ AstVarRef{fl, nleftVarps[i], VAccess::READ},
                                       ନୂତନ AstConst{fl, AstConst::Widthed_ମୂଲ୍ୟ{}, 32, 1}}});

                        // "if (0 == _Vjoin_nleft_0) _Vjoin_repick = true;"
                        jIfp->addThensp(ନୂତନ AstIf{
                            fl,
                            ନୂତନ AstEq{fl, ନୂତନ AstConst{fl, AstConst::Widthed_ମୂଲ୍ୟ{}, 32, 0},
                                      ନୂତନ AstVarRef{fl, nleftVarps[i], VAccess::READ}},
                            ନୂତନ AstAssign{fl, ନୂତନ AstVarRef{fl, repickVarp, VAccess::WRITE},
                                          ନୂତନ AstConst{fl, AstConst::BitTrue{}}}});

                        // "continue;"
                        jIfp->addThensp(ନୂତନ AstJumpGo{fl, whilep});
                    }
                }
            }
        }

        UINFOTREE(9, newp, "ForkJoin new", "-");
        ଫେରନ୍ତୁ newp;
    }

    // VISITORS

    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildren(nodep);
    }

    ଶୂନ୍ୟ visit(AstRandSequence* nodep) override {
        UINFO(9, "visit " << nodep);
        UASSERT_OBJ(m_modp, nodep, "randsequence not under module");
        ଯଦି (m_rsp) {
            // No examples found in the wild
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: randsequence under randsequence");
            VL_DO_DANGLING(ଠେଲ_Deletep(nodep->unlinkFrBack()), nodep);
            ଫେରନ୍ତୁ;
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
        ଯଦି (nodep->prodp()) m_startProdp = nodep->prodp();  // Has a start production selected

        // Build production functions
        ପାଇଁ (AstRSProd* prodp = nodep->prodsp(); prodp; prodp = VN_AS(prodp->nextp(), RSProd)) {
            newProdFunc(prodp);
            // Don't iterate yet, need to have tasks we can reference
        }

        // Iterate all of the new productions, moving guts into functions just made
        // Must do those with 'rand join' first, as they need to see the raw AstRS* nodes
        // before they are removed.  (Alternative would be to keep them until the end).
        UINFOTREE(9, nodep, "RS Tree pre-it", "-");
        std::unordered_set<AstRSProd*> prodHasRandJoin;
        ପାଇଁ (AstRSProd* prodp = nodep->prodsp(); prodp; prodp = VN_AS(prodp->nextp(), RSProd)) {
            prodp->foreach([&](ଅଚଳ AstRSProdList* ଅଚଳ prodlistp) {
                ଯଦି (prodlistp->randJoin()) prodHasRandJoin.emplace(prodp);
            });
        }
        ପାଇଁ (AstRSProd *nextp, *prodp = nodep->prodsp(); prodp; prodp = nextp) {
            nextp = VN_AS(prodp->nextp(), RSProd);
            ଯଦି (prodHasRandJoin.ଗଣନା_2(prodp)) VL_DO_DANGLING(iterate(prodp), prodp);
        }
        ପାଇଁ (AstRSProd *nextp, *prodp = nodep->prodsp(); prodp; prodp = nextp) {
            nextp = VN_AS(prodp->nextp(), RSProd);
            ଯଦି (!prodHasRandJoin.ଗଣନା_2(prodp)) VL_DO_DANGLING(iterate(prodp), prodp);
        }
        UINFOTREE(9, nodep, "RS Tree post-it", "-");

        // Replace randsequence with call to randsequence start function
        AstTask* ଅଚଳ startFuncp = newStartFunc(nodep);
        AstArg* argsp = nullptr;
        ପାଇଁ (auto& itr : m_localizeNames) {
            AstVar* ଅଚଳ lvarp = itr.second;
            argsp = Ast_ଗଣ୍ଠି::addNext(
                argsp, ନୂତନ AstArg{nodep->fileline(), "__Vrsarg_" + lvarp->name(),
                                  ନୂତନ AstVarRef{nodep->fileline(), lvarp, VAccess::READWRITE}});
        }
        AstTaskRef* ଅଚଳ callStartp
            = ନୂତନ AstTaskRef{nodep->fileline(), startFuncp->name(), argsp};
        callStartp->taskp(startFuncp);
        nodep->replaceWith(ନୂତନ Ast_Stmt_ପଦସମଷ୍ଟି{nodep->fileline(), callStartp});

        VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
    }
    ଶୂନ୍ୟ visit(AstRSProd* nodep) override {
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
        m_breakVarp = newBreakVar(nodep->fileline(), ସତ୍ୟ);
        m_prodFuncp->addStmtsp(m_breakVarp);

        // Production formal ports become real input ports on the generated task.
        // Must be added after m_breakVarp / localize ports so positional argument
        // order in newProdFuncRef stays consistent: [break, localize..., user...].
        ଯଦି (Ast_ଗଣ୍ଠି* ଅଚଳ portsp = nodep->portsp()) {
            portsp->unlinkFrBackWithNext();
            m_prodFuncp->addStmtsp(portsp);
            ପାଇଁ (Ast_ଗଣ୍ଠି* itp = portsp; itp; itp = itp->nextp()) {
                AstVar* ଅଚଳ portVarp = VN_AS(itp, Var);
                portVarp->funcLocal(ସତ୍ୟ);
                portVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            }
        }

        // Put JumpBlock immediately under the new function to support
        // a future break/return.  V3Const will rip it out if unneeded.
        VL_RESTORER(m_jumpBlockp);
        m_jumpBlockp = ନୂତନ AstJumpBlock{nodep->fileline(), nullptr};
        m_prodFuncp->addStmtsp(m_jumpBlockp);

        ଯଦି (nodep->fvarp())
            nodep->fvarp()->v3warn(E_UNSUPPORTED,
                                   "Unsupported: randsequence production function variable");

        // Move children into m_prodFuncp, and iterate there
        ଯଦି (!nodep->rulesp()) {  // Nothing to do
        } ଅନ୍ୟଥା ଯଦି (nodep->rulesp()->prodlistsp() && nodep->rulesp()->prodlistsp()->randJoin()) {
            AstRSProdList* ଅଚଳ prodlistp = nodep->rulesp()->prodlistsp();
            Ast_ଗଣ୍ଠି* ଅଚଳ itemsp = newProdRandJoin(nodep, prodlistp);
            m_jumpBlockp->addStmtsp(itemsp);
            iterateAndNextNull(itemsp);
        } ଅନ୍ୟଥା ଯଦି (!nodep->rulesp()->nextp()) {  // Single rule/list, can just do it
            // RSPROD(RSRULE(weight, stmt)) -> IF(weight != 0, stmt)
            ଅଚଳ AstRSRule* ଅଚଳ rulep = nodep->rulesp();
            Ast_ଗଣ୍ଠି* itemsp = nullptr;
            ଯଦି (rulep->weightStmtsp()) itemsp = rulep->weightStmtsp()->unlinkFrBackWithNext();
            ଯଦି (rulep->prodlistsp())
                itemsp = Ast_ଗଣ୍ଠି::addNext(itemsp, rulep->prodlistsp()->unlinkFrBackWithNext());
            // Can ignore rulep->weightp() unless is zero, in which case noop, add if
            ଯଦି (rulep->weightp())
                itemsp = ନୂତନ AstIf{rulep->weightp()->fileline(),
                                   ନୂତନ AstNeq{rulep->weightp()->fileline(),
                                              ନୂତନ AstConst{rulep->weightp()->fileline(), 0},
                                              rulep->weightp()->unlinkFrBack()},
                                   itemsp};
            // Move prodlist to parent (m_prodFunc) and process
            ଯଦି (itemsp) {
                UINFOTREE(9, itemsp, "additems", "-");
                m_jumpBlockp->addStmtsp(itemsp);
                iterateAndNextNull(itemsp);
                UINFOTREE(9, m_jumpBlockp, "jumpBlockNow", "-");
            }
        } ଅନ୍ୟଥା {
            // List of rules with "OR", need to random-weight them
            // RSPROD(RSRULE(weight1, stmt!), RSRULE(weight2, stmt2))
            //   -> RANDCASE(CASEITEM(weight1, stmt1), CASEITEM(weight2, stmt2))
            AstRandCase* ଅଚଳ randcasep = ନୂତନ AstRandCase{nodep->fileline(), nullptr};
            ପାଇଁ (AstRSRule* rulep = nodep->rulesp(); rulep;
                 rulep = VN_AS(rulep->nextp(), RSRule)) {
                Ast_ଗଣ୍ଠି* itemsp = nullptr;
                ଯଦି (rulep->weightStmtsp()) itemsp = rulep->weightStmtsp()->unlinkFrBackWithNext();
                ଯଦି (rulep->prodlistsp())
                    itemsp = Ast_ଗଣ୍ଠି::addNext(itemsp, rulep->prodlistsp()->unlinkFrBackWithNext());
                ଯଦି (itemsp) {
                    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ weightp = rulep->weightp()
                                                     ? rulep->weightp()->unlinkFrBack()
                                                     : ନୂତନ AstConst{rulep->fileline(), 1};
                    randcasep->addItemsp(ନୂତନ AstCaseItem{rulep->fileline(), weightp, itemsp});
                }
            }
            m_jumpBlockp->addStmtsp(randcasep);
            iterateAndNextNull(randcasep);
        }

        // V3Task needs to know if we ended up making a recursive function
        m_prodFuncp->foreach([&](Ast_ଗଣ୍ଠି_FTask_Ref* ଅଚଳ nodep) {
            ଯଦି (nodep->taskp() == m_prodFuncp) m_prodFuncp->recursive(ସତ୍ୟ);
        });
        UINFOTREE(9, m_prodFuncp, "rsprod-task-done " << nodep, "-");

        // Done with production, should have moved everything to new task
        VL_DO_DANGLING(ଠେଲ_Deletep(nodep->unlinkFrBack()), nodep);
    }
    ଶୂନ୍ୟ visit(AstRSRule* nodep) override {
        // Rule of what to produce (as a list) under a production
        nodep->v3fatalSrc("randsequence rule should be iterated and deleted by AstRSProd");
    }
    ଶୂନ୍ୟ visit(AstRSProdList* nodep) override {
        // List of productions to call, potentially with a weight
        UINFO(9, "visit " << nodep);
        UASSERT_OBJ(m_prodFuncp, nodep, "RSProdList not under production");

        // Move prodlist to parent (m_prodFunc) and process
        ଯଦି (Ast_ଗଣ୍ଠି* ଅଚଳ itemsp = nodep->prodsp()) {
            UASSERT_OBJ(!nodep->randJoin(), nodep,
                        "rand join should have been handled in visit(RSRule)");
            nodep->replaceWith(itemsp->unlinkFrBackWithNext());
            // Will soon iterate itemsp as part of normal visit process
            // These will become sequential in the generated task
        } ଅନ୍ୟଥା {
            nodep->unlinkFrBack();
        }
        VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
    }
    ଶୂନ୍ୟ visit(AstRSProdItem* nodep) override {
        // "call" to generate using another production
        UINFO(9, "visit " << nodep);
        UASSERT_OBJ(m_prodFuncp, nodep, "RSProdItem not under production");
        AstRSProd* ଅଚଳ foundp = nodep->prodp();
        UASSERT_OBJ(foundp, nodep, "Unlinked production reference");
        // Pass through caller-side argument list (IEEE 1800-2023 18.17.7).
        AstArg* ଅଚଳ userArgsp
            = nodep->argsp() ? nodep->argsp()->unlinkFrBackWithNext() : nullptr;
        // Convert to task call
        Ast_ଗଣ୍ଠି* ଅଚଳ newp = newProdFuncRef(nodep, foundp, m_breakVarp, userArgsp);
        // The production might have done a "break;", skip other steps if so
        newp->addNext(ନୂତନ AstIf{nodep->fileline(),
                                ନୂତନ AstVarRef{nodep->fileline(), m_breakVarp, VAccess::READ},
                                ନୂତନ AstJumpGo{nodep->fileline(), m_jumpBlockp}});

        nodep->replaceWith(newp);
        VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
    }

    ଶୂନ୍ୟ visit(AstRSBreak* nodep) override {
        UASSERT_OBJ(m_jumpBlockp, nodep, "RSBreak not under production jump block");
        UASSERT_OBJ(m_breakVarp, nodep, "no break variable");
        Ast_ଗଣ୍ଠି_Stmt* ଅଚଳ newp = ନୂତନ AstAssign{
            nodep->fileline(), ନୂତନ AstVarRef{nodep->fileline(), m_breakVarp, VAccess::WRITE},
            ନୂତନ AstConst{nodep->fileline(), AstConst::BitTrue{}}};
        newp->addNext(ନୂତନ AstJumpGo{nodep->fileline(), m_jumpBlockp});
        nodep->replaceWith(newp);
        VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
    }
    ଶୂନ୍ୟ visit(AstRSReturn* nodep) override {
        UASSERT_OBJ(m_jumpBlockp, nodep, "RSReturn not under production jump block");
        nodep->replaceWith(ନୂତନ AstJumpGo{nodep->fileline(), m_jumpBlockp});
        VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
    }
    ଶୂନ୍ୟ visit(AstVarRef* nodep) override {
        // Change references to variables inside randsequence to references to
        // ref variables on the new task
        ଅଚଳ auto it = m_localizeRemaps.find(nodep->varp());
        ଯଦି (it != m_localizeRemaps.end()) {
            AstVar* ଅଚଳ iovarp = it->second;
            UINFO(9, "VARREF remap " << nodep << " -> " << iovarp);
            nodep->varp(iovarp);
        }
    }

    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override { iterateChildren(nodep); }

ସର୍ବସାଧାରଣ:
    // CONSTRUCTORS
    explicit RandSequenceVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~RandSequenceVisitor() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
};

//######################################################################
// RandSequence method class functions

ଶୂନ୍ୟ V3RandSequence::randSequenceNetlist(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { ଅଚଳ RandSequenceVisitor randSequenceVisitor{nodep}; }
    V3Global::dump_Check_Global_ବୃକ୍ଷ("randsequence", 0, dump_ବୃକ୍ଷ_Either_Level() >= 3);
}
