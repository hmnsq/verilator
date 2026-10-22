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
    منطقي تۈگۈن_Match(ثابت Ast_تۈگۈن* nodep) ثابت override { أرجع VN_IS(nodep, RSProd); }
};

//######################################################################
// Visitor that marks classes needing a randomize() method

صنف RandSequenceVisitor final : عام VNVisitor {
    // STATE - global
    std::map<AstRSProd*, Ast_تۈگۈن_FTask*> m_prodFuncps;  // Generated production functions

    // STATE - for current visit position (use VL_RESTORER)
    Ast_تۈگۈن_Module* m_modp = nullptr;  // Current module
    AstRandSequence* m_rsp = nullptr;  // Current randsequence
    AstRSProd* m_startProdp = nullptr;  // Starting production
    Ast_تۈگۈن_FTask* m_prodFuncp = nullptr;  // Production function being built
    AstJumpBlock* m_jumpBlockp = nullptr;  // Building jumpblock for return/break
    AstVar* m_breakVarp = nullptr;  // Break variable
    std::unordered_set<AstVar*> m_localizes;  // Variables to become function inouts
    std::map<std::string, AstVar*> m_localizeNames;  // Ordered names of function inouts
    std::unordered_map<ثابت AstVar*, AstVar*> m_localizeRemaps;  // New ports for old vars

    // METHODS
    فراغ findLocalizes(AstRandSequence* nodep) {
        std::set<AstVar*> localVars;
        nodep->foreach([&](Ast_تۈگۈن* ثابت nodep) {
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
            m_localizeNames.emplace(varp->name(), varp);
        }
    }

    AstVar* newBreakVar(File_قۇر* fl, منطقي asOutput) {
        AstVar* ثابت varp = جديد AstVar{fl, VVarType::PORT, "__VrsBreak", VFlagBitPacked{}, 1};
        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        varp->funcLocal(true);
        إذا (asOutput) varp->direction(VDirection::OUTPUT);
        Ast_تۈگۈن* outp = varp;
        outp->addNext(جديد AstAssign{fl, جديد AstVarRef{fl, varp, VAccess::WRITE},
                                    جديد AstConst{fl, AstConst::BitFalse{}}});

        // Also add arguments as next's
        لكل (ثابت auto& itr : m_localizeNames) {
            ثابت AstVar* ثابت lvarp = itr.second;
            AstVar* ثابت iovarp
                = جديد AstVar{fl, VVarType::PORT, "__Vrsarg_" + lvarp->name(), lvarp};
            iovarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            iovarp->funcLocal(true);
            iovarp->direction(VDirection::REF);
            varp->addNext(iovarp);
            m_localizeRemaps.emplace(lvarp, iovarp);
        }
        أرجع varp;
    }

    AstTask* newStartFunc(AstRandSequence* nodep) {
        // Create wrapper that is called by the sequence
        AstTask* ثابت taskp = جديد AstTask{m_startProdp->fileline(), m_rsp->name(), nullptr};
        taskp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        m_modp->addStmtsp(taskp);

        // Create local (not output) break variable
        VL_RESTORER_COPY(m_localizeRemaps);
        AstVar* breakVarp = newBreakVar(nodep->fileline(), خطأ);
        taskp->addStmtsp(breakVarp);

        // Call the start production's task
        taskp->addStmtsp(newProdFuncRef(nodep, m_startProdp, breakVarp, nullptr));

        UINFOTREE(9, taskp, "newStart", "");
        أرجع taskp;
    }

    Ast_تۈگۈن* newProdFuncRef(Ast_تۈگۈن* nodep, AstRSProd* prodp, AstVar* breakVarp,
                            AstArg* userArgsp) {
        auto it = m_prodFuncps.find(prodp);
        UASSERT_OBJ(it != m_prodFuncps.end(), nodep, "No production function made");
        Ast_تۈگۈن_FTask* ثابت prodFuncp = it->second;
        File_قۇر* ثابت fl = nodep->fileline();
        // V3Width already ran before V3RandSequence, so VarRefs we create here
        // need dtype set explicitly, V3Broken later checks width == widthMin.
        AstVarRef* ثابت breakRefp = جديد AstVarRef{fl, breakVarp, VAccess::WRITE};
        breakRefp->dtypeFrom(breakVarp);
        AstArg* ثابت argsp = جديد AstArg{fl, breakVarp->name(), breakRefp};
        لكل (ثابت auto& itr : m_localizeNames) {
            ثابت AstVar* ثابت lvarp = itr.second;
            AstVar* ثابت iovarp = m_localizeRemaps[lvarp];
            UASSERT_OBJ(iovarp, nodep, "No new port variable for local variable" << lvarp);
            AstVarRef* ثابت refp = جديد AstVarRef{fl, iovarp, VAccess::READWRITE};
            refp->dtypeFrom(iovarp);
            argsp->addNext(جديد AstArg{nodep->fileline(), "__Vrsarg_" + lvarp->name(), refp});
        }
        إذا (userArgsp) argsp->addNext(userArgsp);
        Ast_تۈگۈن* ثابت newp
            = جديد Ast_Stmt_ئىپادە{fl, جديد AstTaskRef{fl, VN_AS(prodFuncp, Task), argsp}};
        أرجع newp;
    }

    فراغ newProdFunc(AstRSProd* nodep) {
        // Task, as time may pass
        AstTask* ثابت newp
            = جديد AstTask{nodep->fileline(), m_rsp->name() + "_" + nodep->name(), nullptr};
        m_modp->addStmtsp(newp);
        m_prodFuncps.emplace(nodep, newp);
        UINFOTREE(9, newp, "newProd", "");
    }

    Ast_تۈگۈن* newProdRandJoin(AstRSProd* /*prodp*/, AstRSProdList* prodlistp) {
        // For weight == 1.0 longer sequence (favor stay in a)
        // For weight == 0.0 shorter squence (favor change a/b)
        UASSERT_OBJ(prodlistp->weightp(), prodlistp, "Weight should have default CONST(0.5)");
        Ast_تۈگۈن_ئىپادە* weightp = prodlistp->weightp()->unlinkFrBack();

        // Build map of production lists and productions we need to join
        // Must mainain the relative order of each sequence.
        std::deque<AstRSProdItem*> lists;
        std::unordered_map<AstRSProdItem*, std::deque<Ast_تۈگۈن_Stmt*>> listStmts;
        لكل (AstRSProdItem* proditemp = VN_AS(prodlistp->prodsp(), RSProdItem); proditemp;
             proditemp = VN_AS(proditemp->nextp(), RSProdItem)) {
            lists.ئىتتىرىش_back(proditemp);
            ثابت AstRSProd* ثابت subProdp = proditemp->prodp();
            إذا (!subProdp) استمر;
            إذا (!subProdp->rulesp()) استمر;
            إذا (!subProdp->rulesp()->prodlistsp()) استمر;
            لكل (Ast_تۈگۈن_Stmt* subStmtp
                 = VN_AS(subProdp->rulesp()->prodlistsp()->prodsp(), تۈگۈن_Stmt);
                 subStmtp; subStmtp = VN_AS(subStmtp->nextp(), تۈگۈن_Stmt)) {
                listStmts[proditemp].ئىتتىرىش_back(subStmtp);
            }
        }

        UINFO(9, "RandJoin productions called:");
        لكل (AstRSProdItem* proditemp : lists) {
            UINFO(9, "  list " << proditemp);
            لكل (ثابت Ast_تۈگۈن_Stmt* prodip : listStmts[proditemp])
                UINFO(9, "    calls " << prodip);
        }

        // Need to clone all nodes used

        File_قۇر* fl = prodlistp->fileline();
        Ast_تۈگۈن* newp = nullptr;

        // "bool _Vjoin_repick = true;"  // Pick a new longer sequence
        AstVar* repickVarp = جديد AstVar{fl, VVarType::VAR, "_Vjoin_repick", VFlagBitPacked{}, 1};
        repickVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        repickVarp->funcLocal(true);
        m_breakVarp->addNextHere(repickVarp);  // Add to function top, not newp
        newp = Ast_تۈگۈن::addNext(newp,
                                جديد AstAssign{fl, جديد AstVarRef{fl, repickVarp, VAccess::WRITE},
                                              جديد AstConst{fl, AstConst::BitTrue{}}});

        // "int _Vjoin_nleft_0 = N(items);"  // N(a) Number left to generate in list 0, starts at
        // N(list0)
        std::deque<AstVar*> nleftVarps;
        صحيح i = 0;
        لكل (AstRSProdItem* proditemp : lists) {
            // 'rand join arule arule arule' is legal, so need numbered, not non-unique named
            // nleft's
            AstVar* ثابت varp = جديد AstVar{
                fl, VVarType::VAR, "_Vjoin_nleft_" + std::to_string(i++), VFlagBitPacked{}, 32};
            varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            varp->funcLocal(true);
            m_breakVarp->addNextHere(varp);  // Add to function top, not newp
            nleftVarps.ئىتتىرىش_back(varp);

            newp = Ast_تۈگۈن::addNext(
                newp,
                جديد AstAssign{fl, جديد AstVarRef{fl, varp, VAccess::WRITE},
                              جديد AstConst{fl, AstConst::Widthed_قىممەت{}, 32,
                                           static_cast<uint32_t>(listStmts[proditemp].size())}});
        }

        // "int _Vjoin_picked_elist = 0;"  // 0 = pick first eligible (non-finished) list, 1 = pick
        // second, etc
        AstVar* ثابت pickedVarp
            = جديد AstVar{fl, VVarType::VAR, "_Vjoin_picked_elist", VFlagBitPacked{}, 1};
        pickedVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        pickedVarp->funcLocal(true);
        m_breakVarp->addNextHere(pickedVarp);  // Add to function top, not newp

        // "while(1) {"  // Generate until make complete sequence
        AstJumpBlock* ثابت whilep = جديد AstJumpBlock{fl, nullptr};
        newp = Ast_تۈگۈن::addNext(newp, جديد AstLoop{fl, whilep});
        {
            // "if ((std::rand() & 0xffffUL) > (0xffff * weight)) _Vjoin_repick = true;"
            whilep->addStmtsp(جديد AstIf{
                fl,
                جديد AstGt{
                    fl, جديد AstRand{fl, AstRand::SRandomU32{}},
                    جديد AstRToIS{fl, جديد AstMulD{fl,
                                                 جديد AstConst{fl, AstConst::RealDouble{},
                                                              static_cast<مزدوج>(0xffffffffUL)},
                                                 weightp}}},
                جديد AstAssign{fl, جديد AstVarRef{fl, repickVarp, VAccess::WRITE},
                              جديد AstConst{fl, AstConst::BitTrue{}}}});

            // "if (_Vjoin_repick) {"
            AstIf* ifp = جديد AstIf{fl, جديد AstVarRef{fl, repickVarp, VAccess::READ}};
            whilep->addStmtsp(ifp);
            {
                // "// _Vjoin_nlists_eligible is how many lists eligable for picking"
                // "int _Vjoin_nlists_eligible = 0;"
                AstVar* ثابت eligibleVarp = جديد AstVar{
                    fl, VVarType::VAR, "_Vjoin_nlists_eligible", VFlagBitPacked{}, 32};
                eligibleVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                eligibleVarp->funcLocal(true);
                m_breakVarp->addNextHere(eligibleVarp);  // Add to function top, not newp
                ifp->addThensp(جديد AstAssign{fl, جديد AstVarRef{fl, eligibleVarp, VAccess::WRITE},
                                             جديد AstConst{fl, AstConst::Widthed_قىممەت{}, 32, 0}});

                i = 0;
                لكل (AstRSProdItem* proditemp : lists) {
                    (فراغ)proditemp;
                    // "_Vjoin_nlists_eligible += _Vjoin_nleft_0 ? 1 : 0;"
                    ifp->addThensp(جديد AstAssign{
                        fl, جديد AstVarRef{fl, eligibleVarp, VAccess::WRITE},
                        جديد AstAdd{
                            fl, جديد AstVarRef{fl, eligibleVarp, VAccess::READ},
                            جديد AstCond{
                                fl,
                                جديد AstNeq{fl, جديد AstConst{fl, AstConst::Widthed_قىممەت{}, 32, 0},
                                           جديد AstVarRef{fl, nleftVarps[i], VAccess::READ}},
                                جديد AstConst{fl, AstConst::Widthed_قىممەت{}, 32, 1},
                                جديد AstConst{fl, AstConst::Widthed_قىممەت{}, 32, 0}}}});
                    ++i;
                }

                // "if (!_Vjoin_nlists_eligible) break;"  // Out of elements
                ifp->addThensp(
                    جديد AstIf{fl,
                              جديد AstEq{fl, جديد AstConst{fl, AstConst::Widthed_قىممەت{}, 32, 0},
                                        جديد AstVarRef{fl, eligibleVarp, VAccess::READ}},
                              جديد AstJumpGo{fl, m_jumpBlockp}});

                // "// +1 to simplify usage, so 0 = done"
                // "_Vjoin_picked_elist = 1 + (std::rand() % _Vjoin_nlists_eligible);"
                ifp->addThensp(جديد AstAssign{
                    fl, جديد AstVarRef{fl, pickedVarp, VAccess::WRITE},
                    جديد AstAdd{fl, جديد AstConst{fl, AstConst::Widthed_قىممەت{}, 32, 1},
                               جديد AstModDiv{fl, جديد AstRand{fl, AstRand::SRandomU32{}},
                                             جديد AstVarRef{fl, eligibleVarp, VAccess::READ}}}});

                // "_Vjoin_repick = false;
                ifp->addThensp(جديد AstAssign{fl, جديد AstVarRef{fl, repickVarp, VAccess::WRITE},
                                             جديد AstConst{fl, AstConst::BitFalse{}}});
            }

            // "// Number of eligible lists left before hit _Vjoin_picked one"
            // "int _Vjoin_sel_elist = _Vjoin_picked_elist;"
            AstVar* ثابت selVarp
                = جديد AstVar{fl, VVarType::VAR, "_Vjoin_sel_elist", VFlagBitPacked{}, 32};
            selVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            selVarp->funcLocal(true);
            m_breakVarp->addNextHere(selVarp);  // Add to function top, not newp
            whilep->addStmtsp(جديد AstAssign{fl, جديد AstVarRef{fl, selVarp, VAccess::WRITE},
                                            جديد AstVarRef{fl, pickedVarp, VAccess::READ}});

            i = -1;
            لكل (AstRSProdItem* proditemp : lists) {
                ++i;
                // "if (_Vjoin_nleft_0) {"
                ifp = جديد AstIf{fl,
                                جديد AstNeq{fl, جديد AstConst{fl, AstConst::Widthed_قىممەت{}, 32, 0},
                                           جديد AstVarRef{fl, nleftVarps[i], VAccess::READ}},
                                nullptr, nullptr};
                whilep->addStmtsp(ifp);
                {
                    // "--_Vjoin_sel_elist;"
                    ifp->addThensp(جديد AstAssign{
                        fl, جديد AstVarRef{fl, selVarp, VAccess::WRITE},
                        جديد AstSub{fl, جديد AstVarRef{fl, selVarp, VAccess::READ},
                                   جديد AstConst{fl, AstConst::Widthed_قىممەت{}, 32, 1}}});

                    // "if (_Vjoin_sel_elist == 0) {"
                    AstIf* ثابت jIfp = جديد AstIf{
                        fl,
                        جديد AstEq{fl, جديد AstConst{fl, AstConst::Widthed_قىممەت{}, 32, 0},
                                  جديد AstVarRef{fl, selVarp, VAccess::READ}},
                        nullptr, nullptr};
                    ifp->addThensp(jIfp);
                    {
                        // "switch (_Vjoin_nleft_0) {"
                        //     "case 2 / * N(a)  * /: {statement}; break;"
                        //     "case 1 / * N(a) - 1 * /: {statement}; break;"
                        uint32_t j = static_cast<uint32_t>(listStmts[proditemp].size());
                        لكل (Ast_تۈگۈن_Stmt* iprodp : listStmts[proditemp]) {
                            jIfp->addThensp(جديد AstIf{
                                fl,
                                جديد AstEq{fl, جديد AstConst{fl, AstConst::Widthed_قىممەت{}, 32, j},
                                          جديد AstVarRef{fl, nleftVarps[i], VAccess::READ}},
                                iprodp->clone_دەرەخ(خطأ)});
                            --j;
                        }

                        // "--_Vjoin_nleft_0;"
                        jIfp->addThensp(جديد AstAssign{
                            fl, جديد AstVarRef{fl, nleftVarps[i], VAccess::WRITE},
                            جديد AstSub{fl, جديد AstVarRef{fl, nleftVarps[i], VAccess::READ},
                                       جديد AstConst{fl, AstConst::Widthed_قىممەت{}, 32, 1}}});

                        // "if (0 == _Vjoin_nleft_0) _Vjoin_repick = true;"
                        jIfp->addThensp(جديد AstIf{
                            fl,
                            جديد AstEq{fl, جديد AstConst{fl, AstConst::Widthed_قىممەت{}, 32, 0},
                                      جديد AstVarRef{fl, nleftVarps[i], VAccess::READ}},
                            جديد AstAssign{fl, جديد AstVarRef{fl, repickVarp, VAccess::WRITE},
                                          جديد AstConst{fl, AstConst::BitTrue{}}}});

                        // "continue;"
                        jIfp->addThensp(جديد AstJumpGo{fl, whilep});
                    }
                }
            }
        }

        UINFOTREE(9, newp, "ForkJoin new", "-");
        أرجع newp;
    }

    // VISITORS

    فراغ visit(Ast_تۈگۈن_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildren(nodep);
    }

    فراغ visit(AstRandSequence* nodep) override {
        UINFO(9, "visit " << nodep);
        UASSERT_OBJ(m_modp, nodep, "randsequence not under module");
        إذا (m_rsp) {
            // No examples found in the wild
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: randsequence under randsequence");
            VL_DO_DANGLING(ئىتتىرىش_Deletep(nodep->unlinkFrBack()), nodep);
            أرجع;
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
        إذا (nodep->prodp()) m_startProdp = nodep->prodp();  // Has a start production selected

        // Build production functions
        لكل (AstRSProd* prodp = nodep->prodsp(); prodp; prodp = VN_AS(prodp->nextp(), RSProd)) {
            newProdFunc(prodp);
            // Don't iterate yet, need to have tasks we can reference
        }

        // Iterate all of the new productions, moving guts into functions just made
        // Must do those with 'rand join' first, as they need to see the raw AstRS* nodes
        // before they are removed.  (Alternative would be to keep them until the end).
        UINFOTREE(9, nodep, "RS Tree pre-it", "-");
        std::unordered_set<AstRSProd*> prodHasRandJoin;
        لكل (AstRSProd* prodp = nodep->prodsp(); prodp; prodp = VN_AS(prodp->nextp(), RSProd)) {
            prodp->foreach([&](ثابت AstRSProdList* ثابت prodlistp) {
                إذا (prodlistp->randJoin()) prodHasRandJoin.emplace(prodp);
            });
        }
        لكل (AstRSProd *nextp, *prodp = nodep->prodsp(); prodp; prodp = nextp) {
            nextp = VN_AS(prodp->nextp(), RSProd);
            إذا (prodHasRandJoin.سانى(prodp)) VL_DO_DANGLING(iterate(prodp), prodp);
        }
        لكل (AstRSProd *nextp, *prodp = nodep->prodsp(); prodp; prodp = nextp) {
            nextp = VN_AS(prodp->nextp(), RSProd);
            إذا (!prodHasRandJoin.سانى(prodp)) VL_DO_DANGLING(iterate(prodp), prodp);
        }
        UINFOTREE(9, nodep, "RS Tree post-it", "-");

        // Replace randsequence with call to randsequence start function
        AstTask* ثابت startFuncp = newStartFunc(nodep);
        AstArg* argsp = nullptr;
        لكل (auto& itr : m_localizeNames) {
            AstVar* ثابت lvarp = itr.second;
            argsp = Ast_تۈگۈن::addNext(
                argsp, جديد AstArg{nodep->fileline(), "__Vrsarg_" + lvarp->name(),
                                  جديد AstVarRef{nodep->fileline(), lvarp, VAccess::READWRITE}});
        }
        AstTaskRef* ثابت callStartp
            = جديد AstTaskRef{nodep->fileline(), startFuncp->name(), argsp};
        callStartp->taskp(startFuncp);
        nodep->replaceWith(جديد Ast_Stmt_ئىپادە{nodep->fileline(), callStartp});

        VL_DO_DANGLING(ئىتتىرىش_Deletep(nodep), nodep);
    }
    فراغ visit(AstRSProd* nodep) override {
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
        m_breakVarp = newBreakVar(nodep->fileline(), true);
        m_prodFuncp->addStmtsp(m_breakVarp);

        // Production formal ports become real input ports on the generated task.
        // Must be added after m_breakVarp / localize ports so positional argument
        // order in newProdFuncRef stays consistent: [break, localize..., user...].
        إذا (Ast_تۈگۈن* ثابت portsp = nodep->portsp()) {
            portsp->unlinkFrBackWithNext();
            m_prodFuncp->addStmtsp(portsp);
            لكل (Ast_تۈگۈن* itp = portsp; itp; itp = itp->nextp()) {
                AstVar* ثابت portVarp = VN_AS(itp, Var);
                portVarp->funcLocal(true);
                portVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            }
        }

        // Put JumpBlock immediately under the new function to support
        // a future break/return.  V3Const will rip it out if unneeded.
        VL_RESTORER(m_jumpBlockp);
        m_jumpBlockp = جديد AstJumpBlock{nodep->fileline(), nullptr};
        m_prodFuncp->addStmtsp(m_jumpBlockp);

        إذا (nodep->fvarp())
            nodep->fvarp()->v3warn(E_UNSUPPORTED,
                                   "Unsupported: randsequence production function variable");

        // Move children into m_prodFuncp, and iterate there
        إذا (!nodep->rulesp()) {  // Nothing to do
        } وإلا إذا (nodep->rulesp()->prodlistsp() && nodep->rulesp()->prodlistsp()->randJoin()) {
            AstRSProdList* ثابت prodlistp = nodep->rulesp()->prodlistsp();
            Ast_تۈگۈن* ثابت itemsp = newProdRandJoin(nodep, prodlistp);
            m_jumpBlockp->addStmtsp(itemsp);
            iterateAndNextNull(itemsp);
        } وإلا إذا (!nodep->rulesp()->nextp()) {  // Single rule/list, can just do it
            // RSPROD(RSRULE(weight, stmt)) -> IF(weight != 0, stmt)
            ثابت AstRSRule* ثابت rulep = nodep->rulesp();
            Ast_تۈگۈن* itemsp = nullptr;
            إذا (rulep->weightStmtsp()) itemsp = rulep->weightStmtsp()->unlinkFrBackWithNext();
            إذا (rulep->prodlistsp())
                itemsp = Ast_تۈگۈن::addNext(itemsp, rulep->prodlistsp()->unlinkFrBackWithNext());
            // Can ignore rulep->weightp() unless is zero, in which case noop, add if
            إذا (rulep->weightp())
                itemsp = جديد AstIf{rulep->weightp()->fileline(),
                                   جديد AstNeq{rulep->weightp()->fileline(),
                                              جديد AstConst{rulep->weightp()->fileline(), 0},
                                              rulep->weightp()->unlinkFrBack()},
                                   itemsp};
            // Move prodlist to parent (m_prodFunc) and process
            إذا (itemsp) {
                UINFOTREE(9, itemsp, "additems", "-");
                m_jumpBlockp->addStmtsp(itemsp);
                iterateAndNextNull(itemsp);
                UINFOTREE(9, m_jumpBlockp, "jumpBlockNow", "-");
            }
        } وإلا {
            // List of rules with "OR", need to random-weight them
            // RSPROD(RSRULE(weight1, stmt!), RSRULE(weight2, stmt2))
            //   -> RANDCASE(CASEITEM(weight1, stmt1), CASEITEM(weight2, stmt2))
            AstRandCase* ثابت randcasep = جديد AstRandCase{nodep->fileline(), nullptr};
            لكل (AstRSRule* rulep = nodep->rulesp(); rulep;
                 rulep = VN_AS(rulep->nextp(), RSRule)) {
                Ast_تۈگۈن* itemsp = nullptr;
                إذا (rulep->weightStmtsp()) itemsp = rulep->weightStmtsp()->unlinkFrBackWithNext();
                إذا (rulep->prodlistsp())
                    itemsp = Ast_تۈگۈن::addNext(itemsp, rulep->prodlistsp()->unlinkFrBackWithNext());
                إذا (itemsp) {
                    Ast_تۈگۈن_ئىپادە* ثابت weightp = rulep->weightp()
                                                     ? rulep->weightp()->unlinkFrBack()
                                                     : جديد AstConst{rulep->fileline(), 1};
                    randcasep->addItemsp(جديد AstCaseItem{rulep->fileline(), weightp, itemsp});
                }
            }
            m_jumpBlockp->addStmtsp(randcasep);
            iterateAndNextNull(randcasep);
        }

        // V3Task needs to know if we ended up making a recursive function
        m_prodFuncp->foreach([&](Ast_تۈگۈن_FTask_Ref* ثابت nodep) {
            إذا (nodep->taskp() == m_prodFuncp) m_prodFuncp->recursive(true);
        });
        UINFOTREE(9, m_prodFuncp, "rsprod-task-done " << nodep, "-");

        // Done with production, should have moved everything to new task
        VL_DO_DANGLING(ئىتتىرىش_Deletep(nodep->unlinkFrBack()), nodep);
    }
    فراغ visit(AstRSRule* nodep) override {
        // Rule of what to produce (as a list) under a production
        nodep->v3fatalSrc("randsequence rule should be iterated and deleted by AstRSProd");
    }
    فراغ visit(AstRSProdList* nodep) override {
        // List of productions to call, potentially with a weight
        UINFO(9, "visit " << nodep);
        UASSERT_OBJ(m_prodFuncp, nodep, "RSProdList not under production");

        // Move prodlist to parent (m_prodFunc) and process
        إذا (Ast_تۈگۈن* ثابت itemsp = nodep->prodsp()) {
            UASSERT_OBJ(!nodep->randJoin(), nodep,
                        "rand join should have been handled in visit(RSRule)");
            nodep->replaceWith(itemsp->unlinkFrBackWithNext());
            // Will soon iterate itemsp as part of normal visit process
            // These will become sequential in the generated task
        } وإلا {
            nodep->unlinkFrBack();
        }
        VL_DO_DANGLING(ئىتتىرىش_Deletep(nodep), nodep);
    }
    فراغ visit(AstRSProdItem* nodep) override {
        // "call" to generate using another production
        UINFO(9, "visit " << nodep);
        UASSERT_OBJ(m_prodFuncp, nodep, "RSProdItem not under production");
        AstRSProd* ثابت foundp = nodep->prodp();
        UASSERT_OBJ(foundp, nodep, "Unlinked production reference");
        // Pass through caller-side argument list (IEEE 1800-2023 18.17.7).
        AstArg* ثابت userArgsp
            = nodep->argsp() ? nodep->argsp()->unlinkFrBackWithNext() : nullptr;
        // Convert to task call
        Ast_تۈگۈن* ثابت newp = newProdFuncRef(nodep, foundp, m_breakVarp, userArgsp);
        // The production might have done a "break;", skip other steps if so
        newp->addNext(جديد AstIf{nodep->fileline(),
                                جديد AstVarRef{nodep->fileline(), m_breakVarp, VAccess::READ},
                                جديد AstJumpGo{nodep->fileline(), m_jumpBlockp}});

        nodep->replaceWith(newp);
        VL_DO_DANGLING(ئىتتىرىش_Deletep(nodep), nodep);
    }

    فراغ visit(AstRSBreak* nodep) override {
        UASSERT_OBJ(m_jumpBlockp, nodep, "RSBreak not under production jump block");
        UASSERT_OBJ(m_breakVarp, nodep, "no break variable");
        Ast_تۈگۈن_Stmt* ثابت newp = جديد AstAssign{
            nodep->fileline(), جديد AstVarRef{nodep->fileline(), m_breakVarp, VAccess::WRITE},
            جديد AstConst{nodep->fileline(), AstConst::BitTrue{}}};
        newp->addNext(جديد AstJumpGo{nodep->fileline(), m_jumpBlockp});
        nodep->replaceWith(newp);
        VL_DO_DANGLING(ئىتتىرىش_Deletep(nodep), nodep);
    }
    فراغ visit(AstRSReturn* nodep) override {
        UASSERT_OBJ(m_jumpBlockp, nodep, "RSReturn not under production jump block");
        nodep->replaceWith(جديد AstJumpGo{nodep->fileline(), m_jumpBlockp});
        VL_DO_DANGLING(ئىتتىرىش_Deletep(nodep), nodep);
    }
    فراغ visit(AstVarRef* nodep) override {
        // Change references to variables inside randsequence to references to
        // ref variables on the new task
        ثابت auto it = m_localizeRemaps.find(nodep->varp());
        إذا (it != m_localizeRemaps.end()) {
            AstVar* ثابت iovarp = it->second;
            UINFO(9, "VARREF remap " << nodep << " -> " << iovarp);
            nodep->varp(iovarp);
        }
    }

    فراغ visit(Ast_تۈگۈن* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    explicit RandSequenceVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~RandSequenceVisitor() override = افتراضي;
};

//######################################################################
// RandSequence method class functions

فراغ V3RandSequence::randSequenceNetlist(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { ثابت RandSequenceVisitor randSequenceVisitor{nodep}; }
    V3Global::dump_Check_Global_دەرەخ("randsequence", 0, dump_دەرەخ_Either_Level() >= 3);
}
