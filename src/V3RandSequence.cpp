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

#அந்தர்பாவ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#அந்தர்பாவ "verilatedos.h"

#அந்தர்பாவ "V3RandSequence.h"

#அந்தர்பாவ "V3Ast.h"
#அந்தர்பாவ "V3Error.h"
#அந்தர்பாவ "V3FileLine.h"
#அந்தர்பாவ "V3Global.h"

VL_DEFINE_DEBUG_FUNCTIONS;

// ######################################################################
//  Matcher classes (for suggestion matching)

வகுப்பு RSProdMatcher final : பொது VNodeMatcher {
பொது:
    தருக்க முனை_Match(மாறிலி Ast_முனை* nodep) மாறிலி override { திரும்பு VN_IS(nodep, RSProd); }
};

//######################################################################
// Visitor that marks classes needing a randomize() method

வகுப்பு RandSequenceVisitor final : பொது VNVisitor {
    // STATE - global
    std::map<AstRSProd*, Ast_முனை_FTask*> m_prodFuncps;  // Generated production functions

    // STATE - for current visit position (use VL_RESTORER)
    Ast_முனை_Module* m_modp = nullptr;  // Current module
    AstRandSequence* m_rsp = nullptr;  // Current randsequence
    AstRSProd* m_startProdp = nullptr;  // Starting production
    Ast_முனை_FTask* m_prodFuncp = nullptr;  // Production function being built
    AstJumpBlock* m_jumpBlockp = nullptr;  // Building jumpblock for return/break
    AstVar* m_breakVarp = nullptr;  // Break variable
    std::unordered_set<AstVar*> m_localizes;  // Variables to become function inouts
    std::map<std::string, AstVar*> m_localizeNames;  // Ordered names of function inouts
    std::unordered_map<மாறிலி AstVar*, AstVar*> m_localizeRemaps;  // New ports for old vars

    // METHODS
    வெறுமை findLocalizes(AstRandSequence* nodep) {
        std::set<AstVar*> localVars;
        nodep->foreach([&](Ast_முனை* மாறிலி nodep) {
            எனில் (மாறிலி AstVarRef* மாறிலி anodep = VN_CAST(nodep, VarRef)) {
                m_localizes.emplace(anodep->varp());
            } இல்லையெனில் எனில் (AstVar* மாறிலி anodep = VN_CAST(nodep, Var)) {
                localVars.emplace(anodep);
            }
        });
        // Clear every variable allocated in the RandSequence
        க்கு (AstVar* varp : localVars) m_localizes.erase(varp);
        // Sort by name, so function arguments are stable
        க்கு (AstVar* varp : m_localizes) {
            UINFO(9, "localize " << varp);
            m_localizeNames.emplace(varp->name(), varp);
        }
    }

    AstVar* newBreakVar(File_வரி* fl, தருக்க asOutput) {
        AstVar* மாறிலி varp = புதிய AstVar{fl, VVarType::PORT, "__VrsBreak", VFlagBitPacked{}, 1};
        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        varp->funcLocal(உண்மை);
        எனில் (asOutput) varp->direction(VDirection::OUTPUT);
        Ast_முனை* outp = varp;
        outp->addNext(புதிய AstAssign{fl, புதிய AstVarRef{fl, varp, VAccess::WRITE},
                                    புதிய AstConst{fl, AstConst::BitFalse{}}});

        // Also add arguments as next's
        க்கு (மாறிலி auto& itr : m_localizeNames) {
            மாறிலி AstVar* மாறிலி lvarp = itr.second;
            AstVar* மாறிலி iovarp
                = புதிய AstVar{fl, VVarType::PORT, "__Vrsarg_" + lvarp->name(), lvarp};
            iovarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            iovarp->funcLocal(உண்மை);
            iovarp->direction(VDirection::REF);
            varp->addNext(iovarp);
            m_localizeRemaps.emplace(lvarp, iovarp);
        }
        திரும்பு varp;
    }

    AstTask* newStartFunc(AstRandSequence* nodep) {
        // Create wrapper that is called by the sequence
        AstTask* மாறிலி taskp = புதிய AstTask{m_startProdp->fileline(), m_rsp->name(), nullptr};
        taskp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        m_modp->addStmtsp(taskp);

        // Create local (not output) break variable
        VL_RESTORER_COPY(m_localizeRemaps);
        AstVar* breakVarp = newBreakVar(nodep->fileline(), பொய்);
        taskp->addStmtsp(breakVarp);

        // Call the start production's task
        taskp->addStmtsp(newProdFuncRef(nodep, m_startProdp, breakVarp, nullptr));

        UINFOTREE(9, taskp, "newStart", "");
        திரும்பு taskp;
    }

    Ast_முனை* newProdFuncRef(Ast_முனை* nodep, AstRSProd* prodp, AstVar* breakVarp,
                            AstArg* userArgsp) {
        auto it = m_prodFuncps.find(prodp);
        UASSERT_OBJ(it != m_prodFuncps.end(), nodep, "No production function made");
        Ast_முனை_FTask* மாறிலி prodFuncp = it->second;
        File_வரி* மாறிலி fl = nodep->fileline();
        // V3Width already ran before V3RandSequence, so VarRefs we create here
        // need dtype set explicitly, V3Broken later checks width == widthMin.
        AstVarRef* மாறிலி breakRefp = புதிய AstVarRef{fl, breakVarp, VAccess::WRITE};
        breakRefp->dtypeFrom(breakVarp);
        AstArg* மாறிலி argsp = புதிய AstArg{fl, breakVarp->name(), breakRefp};
        க்கு (மாறிலி auto& itr : m_localizeNames) {
            மாறிலி AstVar* மாறிலி lvarp = itr.second;
            AstVar* மாறிலி iovarp = m_localizeRemaps[lvarp];
            UASSERT_OBJ(iovarp, nodep, "No new port variable for local variable" << lvarp);
            AstVarRef* மாறிலி refp = புதிய AstVarRef{fl, iovarp, VAccess::READWRITE};
            refp->dtypeFrom(iovarp);
            argsp->addNext(புதிய AstArg{nodep->fileline(), "__Vrsarg_" + lvarp->name(), refp});
        }
        எனில் (userArgsp) argsp->addNext(userArgsp);
        Ast_முனை* மாறிலி newp
            = புதிய Ast_Stmt_கோவை{fl, புதிய AstTaskRef{fl, VN_AS(prodFuncp, Task), argsp}};
        திரும்பு newp;
    }

    வெறுமை newProdFunc(AstRSProd* nodep) {
        // Task, as time may pass
        AstTask* மாறிலி newp
            = புதிய AstTask{nodep->fileline(), m_rsp->name() + "_" + nodep->name(), nullptr};
        m_modp->addStmtsp(newp);
        m_prodFuncps.emplace(nodep, newp);
        UINFOTREE(9, newp, "newProd", "");
    }

    Ast_முனை* newProdRandJoin(AstRSProd* /*prodp*/, AstRSProdList* prodlistp) {
        // For weight == 1.0 longer sequence (favor stay in a)
        // For weight == 0.0 shorter squence (favor change a/b)
        UASSERT_OBJ(prodlistp->weightp(), prodlistp, "Weight should have default CONST(0.5)");
        Ast_முனை_கோவை* weightp = prodlistp->weightp()->unlinkFrBack();

        // Build map of production lists and productions we need to join
        // Must mainain the relative order of each sequence.
        std::deque<AstRSProdItem*> lists;
        std::unordered_map<AstRSProdItem*, std::deque<Ast_முனை_Stmt*>> listStmts;
        க்கு (AstRSProdItem* proditemp = VN_AS(prodlistp->prodsp(), RSProdItem); proditemp;
             proditemp = VN_AS(proditemp->nextp(), RSProdItem)) {
            lists.தள்ளு_back(proditemp);
            மாறிலி AstRSProd* மாறிலி subProdp = proditemp->prodp();
            எனில் (!subProdp) தொடர்;
            எனில் (!subProdp->rulesp()) தொடர்;
            எனில் (!subProdp->rulesp()->prodlistsp()) தொடர்;
            க்கு (Ast_முனை_Stmt* subStmtp
                 = VN_AS(subProdp->rulesp()->prodlistsp()->prodsp(), முனை_Stmt);
                 subStmtp; subStmtp = VN_AS(subStmtp->nextp(), முனை_Stmt)) {
                listStmts[proditemp].தள்ளு_back(subStmtp);
            }
        }

        UINFO(9, "RandJoin productions called:");
        க்கு (AstRSProdItem* proditemp : lists) {
            UINFO(9, "  list " << proditemp);
            க்கு (மாறிலி Ast_முனை_Stmt* prodip : listStmts[proditemp])
                UINFO(9, "    calls " << prodip);
        }

        // Need to clone all nodes used

        File_வரி* fl = prodlistp->fileline();
        Ast_முனை* newp = nullptr;

        // "bool _Vjoin_repick = true;"  // Pick a new longer sequence
        AstVar* repickVarp = புதிய AstVar{fl, VVarType::VAR, "_Vjoin_repick", VFlagBitPacked{}, 1};
        repickVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        repickVarp->funcLocal(உண்மை);
        m_breakVarp->addNextHere(repickVarp);  // Add to function top, not newp
        newp = Ast_முனை::addNext(newp,
                                புதிய AstAssign{fl, புதிய AstVarRef{fl, repickVarp, VAccess::WRITE},
                                              புதிய AstConst{fl, AstConst::BitTrue{}}});

        // "int _Vjoin_nleft_0 = N(items);"  // N(a) Number left to generate in list 0, starts at
        // N(list0)
        std::deque<AstVar*> nleftVarps;
        முழு i = 0;
        க்கு (AstRSProdItem* proditemp : lists) {
            // 'rand join arule arule arule' is legal, so need numbered, not non-unique named
            // nleft's
            AstVar* மாறிலி varp = புதிய AstVar{
                fl, VVarType::VAR, "_Vjoin_nleft_" + std::to_string(i++), VFlagBitPacked{}, 32};
            varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            varp->funcLocal(உண்மை);
            m_breakVarp->addNextHere(varp);  // Add to function top, not newp
            nleftVarps.தள்ளு_back(varp);

            newp = Ast_முனை::addNext(
                newp,
                புதிய AstAssign{fl, புதிய AstVarRef{fl, varp, VAccess::WRITE},
                              புதிய AstConst{fl, AstConst::Widthed_மதிப்பு{}, 32,
                                           static_cast<uint32_t>(listStmts[proditemp].size())}});
        }

        // "int _Vjoin_picked_elist = 0;"  // 0 = pick first eligible (non-finished) list, 1 = pick
        // second, etc
        AstVar* மாறிலி pickedVarp
            = புதிய AstVar{fl, VVarType::VAR, "_Vjoin_picked_elist", VFlagBitPacked{}, 1};
        pickedVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        pickedVarp->funcLocal(உண்மை);
        m_breakVarp->addNextHere(pickedVarp);  // Add to function top, not newp

        // "while(1) {"  // Generate until make complete sequence
        AstJumpBlock* மாறிலி whilep = புதிய AstJumpBlock{fl, nullptr};
        newp = Ast_முனை::addNext(newp, புதிய AstLoop{fl, whilep});
        {
            // "if ((std::rand() & 0xffffUL) > (0xffff * weight)) _Vjoin_repick = true;"
            whilep->addStmtsp(புதிய AstIf{
                fl,
                புதிய AstGt{
                    fl, புதிய AstRand{fl, AstRand::SRandomU32{}},
                    புதிய AstRToIS{fl, புதிய AstMulD{fl,
                                                 புதிய AstConst{fl, AstConst::RealDouble{},
                                                              static_cast<இரட்டை>(0xffffffffUL)},
                                                 weightp}}},
                புதிய AstAssign{fl, புதிய AstVarRef{fl, repickVarp, VAccess::WRITE},
                              புதிய AstConst{fl, AstConst::BitTrue{}}}});

            // "if (_Vjoin_repick) {"
            AstIf* ifp = புதிய AstIf{fl, புதிய AstVarRef{fl, repickVarp, VAccess::READ}};
            whilep->addStmtsp(ifp);
            {
                // "// _Vjoin_nlists_eligible is how many lists eligable for picking"
                // "int _Vjoin_nlists_eligible = 0;"
                AstVar* மாறிலி eligibleVarp = புதிய AstVar{
                    fl, VVarType::VAR, "_Vjoin_nlists_eligible", VFlagBitPacked{}, 32};
                eligibleVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                eligibleVarp->funcLocal(உண்மை);
                m_breakVarp->addNextHere(eligibleVarp);  // Add to function top, not newp
                ifp->addThensp(புதிய AstAssign{fl, புதிய AstVarRef{fl, eligibleVarp, VAccess::WRITE},
                                             புதிய AstConst{fl, AstConst::Widthed_மதிப்பு{}, 32, 0}});

                i = 0;
                க்கு (AstRSProdItem* proditemp : lists) {
                    (வெறுமை)proditemp;
                    // "_Vjoin_nlists_eligible += _Vjoin_nleft_0 ? 1 : 0;"
                    ifp->addThensp(புதிய AstAssign{
                        fl, புதிய AstVarRef{fl, eligibleVarp, VAccess::WRITE},
                        புதிய AstAdd{
                            fl, புதிய AstVarRef{fl, eligibleVarp, VAccess::READ},
                            புதிய AstCond{
                                fl,
                                புதிய AstNeq{fl, புதிய AstConst{fl, AstConst::Widthed_மதிப்பு{}, 32, 0},
                                           புதிய AstVarRef{fl, nleftVarps[i], VAccess::READ}},
                                புதிய AstConst{fl, AstConst::Widthed_மதிப்பு{}, 32, 1},
                                புதிய AstConst{fl, AstConst::Widthed_மதிப்பு{}, 32, 0}}}});
                    ++i;
                }

                // "if (!_Vjoin_nlists_eligible) break;"  // Out of elements
                ifp->addThensp(
                    புதிய AstIf{fl,
                              புதிய AstEq{fl, புதிய AstConst{fl, AstConst::Widthed_மதிப்பு{}, 32, 0},
                                        புதிய AstVarRef{fl, eligibleVarp, VAccess::READ}},
                              புதிய AstJumpGo{fl, m_jumpBlockp}});

                // "// +1 to simplify usage, so 0 = done"
                // "_Vjoin_picked_elist = 1 + (std::rand() % _Vjoin_nlists_eligible);"
                ifp->addThensp(புதிய AstAssign{
                    fl, புதிய AstVarRef{fl, pickedVarp, VAccess::WRITE},
                    புதிய AstAdd{fl, புதிய AstConst{fl, AstConst::Widthed_மதிப்பு{}, 32, 1},
                               புதிய AstModDiv{fl, புதிய AstRand{fl, AstRand::SRandomU32{}},
                                             புதிய AstVarRef{fl, eligibleVarp, VAccess::READ}}}});

                // "_Vjoin_repick = false;
                ifp->addThensp(புதிய AstAssign{fl, புதிய AstVarRef{fl, repickVarp, VAccess::WRITE},
                                             புதிய AstConst{fl, AstConst::BitFalse{}}});
            }

            // "// Number of eligible lists left before hit _Vjoin_picked one"
            // "int _Vjoin_sel_elist = _Vjoin_picked_elist;"
            AstVar* மாறிலி selVarp
                = புதிய AstVar{fl, VVarType::VAR, "_Vjoin_sel_elist", VFlagBitPacked{}, 32};
            selVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            selVarp->funcLocal(உண்மை);
            m_breakVarp->addNextHere(selVarp);  // Add to function top, not newp
            whilep->addStmtsp(புதிய AstAssign{fl, புதிய AstVarRef{fl, selVarp, VAccess::WRITE},
                                            புதிய AstVarRef{fl, pickedVarp, VAccess::READ}});

            i = -1;
            க்கு (AstRSProdItem* proditemp : lists) {
                ++i;
                // "if (_Vjoin_nleft_0) {"
                ifp = புதிய AstIf{fl,
                                புதிய AstNeq{fl, புதிய AstConst{fl, AstConst::Widthed_மதிப்பு{}, 32, 0},
                                           புதிய AstVarRef{fl, nleftVarps[i], VAccess::READ}},
                                nullptr, nullptr};
                whilep->addStmtsp(ifp);
                {
                    // "--_Vjoin_sel_elist;"
                    ifp->addThensp(புதிய AstAssign{
                        fl, புதிய AstVarRef{fl, selVarp, VAccess::WRITE},
                        புதிய AstSub{fl, புதிய AstVarRef{fl, selVarp, VAccess::READ},
                                   புதிய AstConst{fl, AstConst::Widthed_மதிப்பு{}, 32, 1}}});

                    // "if (_Vjoin_sel_elist == 0) {"
                    AstIf* மாறிலி jIfp = புதிய AstIf{
                        fl,
                        புதிய AstEq{fl, புதிய AstConst{fl, AstConst::Widthed_மதிப்பு{}, 32, 0},
                                  புதிய AstVarRef{fl, selVarp, VAccess::READ}},
                        nullptr, nullptr};
                    ifp->addThensp(jIfp);
                    {
                        // "switch (_Vjoin_nleft_0) {"
                        //     "case 2 / * N(a)  * /: {statement}; break;"
                        //     "case 1 / * N(a) - 1 * /: {statement}; break;"
                        uint32_t j = static_cast<uint32_t>(listStmts[proditemp].size());
                        க்கு (Ast_முனை_Stmt* iprodp : listStmts[proditemp]) {
                            jIfp->addThensp(புதிய AstIf{
                                fl,
                                புதிய AstEq{fl, புதிய AstConst{fl, AstConst::Widthed_மதிப்பு{}, 32, j},
                                          புதிய AstVarRef{fl, nleftVarps[i], VAccess::READ}},
                                iprodp->clone_மரம்(பொய்)});
                            --j;
                        }

                        // "--_Vjoin_nleft_0;"
                        jIfp->addThensp(புதிய AstAssign{
                            fl, புதிய AstVarRef{fl, nleftVarps[i], VAccess::WRITE},
                            புதிய AstSub{fl, புதிய AstVarRef{fl, nleftVarps[i], VAccess::READ},
                                       புதிய AstConst{fl, AstConst::Widthed_மதிப்பு{}, 32, 1}}});

                        // "if (0 == _Vjoin_nleft_0) _Vjoin_repick = true;"
                        jIfp->addThensp(புதிய AstIf{
                            fl,
                            புதிய AstEq{fl, புதிய AstConst{fl, AstConst::Widthed_மதிப்பு{}, 32, 0},
                                      புதிய AstVarRef{fl, nleftVarps[i], VAccess::READ}},
                            புதிய AstAssign{fl, புதிய AstVarRef{fl, repickVarp, VAccess::WRITE},
                                          புதிய AstConst{fl, AstConst::BitTrue{}}}});

                        // "continue;"
                        jIfp->addThensp(புதிய AstJumpGo{fl, whilep});
                    }
                }
            }
        }

        UINFOTREE(9, newp, "ForkJoin new", "-");
        திரும்பு newp;
    }

    // VISITORS

    வெறுமை visit(Ast_முனை_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildren(nodep);
    }

    வெறுமை visit(AstRandSequence* nodep) override {
        UINFO(9, "visit " << nodep);
        UASSERT_OBJ(m_modp, nodep, "randsequence not under module");
        எனில் (m_rsp) {
            // No examples found in the wild
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: randsequence under randsequence");
            VL_DO_DANGLING(தள்ளு_Deletep(nodep->unlinkFrBack()), nodep);
            திரும்பு;
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
        எனில் (nodep->prodp()) m_startProdp = nodep->prodp();  // Has a start production selected

        // Build production functions
        க்கு (AstRSProd* prodp = nodep->prodsp(); prodp; prodp = VN_AS(prodp->nextp(), RSProd)) {
            newProdFunc(prodp);
            // Don't iterate yet, need to have tasks we can reference
        }

        // Iterate all of the new productions, moving guts into functions just made
        // Must do those with 'rand join' first, as they need to see the raw AstRS* nodes
        // before they are removed.  (Alternative would be to keep them until the end).
        UINFOTREE(9, nodep, "RS Tree pre-it", "-");
        std::unordered_set<AstRSProd*> prodHasRandJoin;
        க்கு (AstRSProd* prodp = nodep->prodsp(); prodp; prodp = VN_AS(prodp->nextp(), RSProd)) {
            prodp->foreach([&](மாறிலி AstRSProdList* மாறிலி prodlistp) {
                எனில் (prodlistp->randJoin()) prodHasRandJoin.emplace(prodp);
            });
        }
        க்கு (AstRSProd *nextp, *prodp = nodep->prodsp(); prodp; prodp = nextp) {
            nextp = VN_AS(prodp->nextp(), RSProd);
            எனில் (prodHasRandJoin.மொத்தம்(prodp)) VL_DO_DANGLING(iterate(prodp), prodp);
        }
        க்கு (AstRSProd *nextp, *prodp = nodep->prodsp(); prodp; prodp = nextp) {
            nextp = VN_AS(prodp->nextp(), RSProd);
            எனில் (!prodHasRandJoin.மொத்தம்(prodp)) VL_DO_DANGLING(iterate(prodp), prodp);
        }
        UINFOTREE(9, nodep, "RS Tree post-it", "-");

        // Replace randsequence with call to randsequence start function
        AstTask* மாறிலி startFuncp = newStartFunc(nodep);
        AstArg* argsp = nullptr;
        க்கு (auto& itr : m_localizeNames) {
            AstVar* மாறிலி lvarp = itr.second;
            argsp = Ast_முனை::addNext(
                argsp, புதிய AstArg{nodep->fileline(), "__Vrsarg_" + lvarp->name(),
                                  புதிய AstVarRef{nodep->fileline(), lvarp, VAccess::READWRITE}});
        }
        AstTaskRef* மாறிலி callStartp
            = புதிய AstTaskRef{nodep->fileline(), startFuncp->name(), argsp};
        callStartp->taskp(startFuncp);
        nodep->replaceWith(புதிய Ast_Stmt_கோவை{nodep->fileline(), callStartp});

        VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
    }
    வெறுமை visit(AstRSProd* nodep) override {
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
        m_breakVarp = newBreakVar(nodep->fileline(), உண்மை);
        m_prodFuncp->addStmtsp(m_breakVarp);

        // Production formal ports become real input ports on the generated task.
        // Must be added after m_breakVarp / localize ports so positional argument
        // order in newProdFuncRef stays consistent: [break, localize..., user...].
        எனில் (Ast_முனை* மாறிலி portsp = nodep->portsp()) {
            portsp->unlinkFrBackWithNext();
            m_prodFuncp->addStmtsp(portsp);
            க்கு (Ast_முனை* itp = portsp; itp; itp = itp->nextp()) {
                AstVar* மாறிலி portVarp = VN_AS(itp, Var);
                portVarp->funcLocal(உண்மை);
                portVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            }
        }

        // Put JumpBlock immediately under the new function to support
        // a future break/return.  V3Const will rip it out if unneeded.
        VL_RESTORER(m_jumpBlockp);
        m_jumpBlockp = புதிய AstJumpBlock{nodep->fileline(), nullptr};
        m_prodFuncp->addStmtsp(m_jumpBlockp);

        எனில் (nodep->fvarp())
            nodep->fvarp()->v3warn(E_UNSUPPORTED,
                                   "Unsupported: randsequence production function variable");

        // Move children into m_prodFuncp, and iterate there
        எனில் (!nodep->rulesp()) {  // Nothing to do
        } இல்லையெனில் எனில் (nodep->rulesp()->prodlistsp() && nodep->rulesp()->prodlistsp()->randJoin()) {
            AstRSProdList* மாறிலி prodlistp = nodep->rulesp()->prodlistsp();
            Ast_முனை* மாறிலி itemsp = newProdRandJoin(nodep, prodlistp);
            m_jumpBlockp->addStmtsp(itemsp);
            iterateAndNextNull(itemsp);
        } இல்லையெனில் எனில் (!nodep->rulesp()->nextp()) {  // Single rule/list, can just do it
            // RSPROD(RSRULE(weight, stmt)) -> IF(weight != 0, stmt)
            மாறிலி AstRSRule* மாறிலி rulep = nodep->rulesp();
            Ast_முனை* itemsp = nullptr;
            எனில் (rulep->weightStmtsp()) itemsp = rulep->weightStmtsp()->unlinkFrBackWithNext();
            எனில் (rulep->prodlistsp())
                itemsp = Ast_முனை::addNext(itemsp, rulep->prodlistsp()->unlinkFrBackWithNext());
            // Can ignore rulep->weightp() unless is zero, in which case noop, add if
            எனில் (rulep->weightp())
                itemsp = புதிய AstIf{rulep->weightp()->fileline(),
                                   புதிய AstNeq{rulep->weightp()->fileline(),
                                              புதிய AstConst{rulep->weightp()->fileline(), 0},
                                              rulep->weightp()->unlinkFrBack()},
                                   itemsp};
            // Move prodlist to parent (m_prodFunc) and process
            எனில் (itemsp) {
                UINFOTREE(9, itemsp, "additems", "-");
                m_jumpBlockp->addStmtsp(itemsp);
                iterateAndNextNull(itemsp);
                UINFOTREE(9, m_jumpBlockp, "jumpBlockNow", "-");
            }
        } இல்லையெனில் {
            // List of rules with "OR", need to random-weight them
            // RSPROD(RSRULE(weight1, stmt!), RSRULE(weight2, stmt2))
            //   -> RANDCASE(CASEITEM(weight1, stmt1), CASEITEM(weight2, stmt2))
            AstRandCase* மாறிலி randcasep = புதிய AstRandCase{nodep->fileline(), nullptr};
            க்கு (AstRSRule* rulep = nodep->rulesp(); rulep;
                 rulep = VN_AS(rulep->nextp(), RSRule)) {
                Ast_முனை* itemsp = nullptr;
                எனில் (rulep->weightStmtsp()) itemsp = rulep->weightStmtsp()->unlinkFrBackWithNext();
                எனில் (rulep->prodlistsp())
                    itemsp = Ast_முனை::addNext(itemsp, rulep->prodlistsp()->unlinkFrBackWithNext());
                எனில் (itemsp) {
                    Ast_முனை_கோவை* மாறிலி weightp = rulep->weightp()
                                                     ? rulep->weightp()->unlinkFrBack()
                                                     : புதிய AstConst{rulep->fileline(), 1};
                    randcasep->addItemsp(புதிய AstCaseItem{rulep->fileline(), weightp, itemsp});
                }
            }
            m_jumpBlockp->addStmtsp(randcasep);
            iterateAndNextNull(randcasep);
        }

        // V3Task needs to know if we ended up making a recursive function
        m_prodFuncp->foreach([&](Ast_முனை_FTask_Ref* மாறிலி nodep) {
            எனில் (nodep->taskp() == m_prodFuncp) m_prodFuncp->recursive(உண்மை);
        });
        UINFOTREE(9, m_prodFuncp, "rsprod-task-done " << nodep, "-");

        // Done with production, should have moved everything to new task
        VL_DO_DANGLING(தள்ளு_Deletep(nodep->unlinkFrBack()), nodep);
    }
    வெறுமை visit(AstRSRule* nodep) override {
        // Rule of what to produce (as a list) under a production
        nodep->v3fatalSrc("randsequence rule should be iterated and deleted by AstRSProd");
    }
    வெறுமை visit(AstRSProdList* nodep) override {
        // List of productions to call, potentially with a weight
        UINFO(9, "visit " << nodep);
        UASSERT_OBJ(m_prodFuncp, nodep, "RSProdList not under production");

        // Move prodlist to parent (m_prodFunc) and process
        எனில் (Ast_முனை* மாறிலி itemsp = nodep->prodsp()) {
            UASSERT_OBJ(!nodep->randJoin(), nodep,
                        "rand join should have been handled in visit(RSRule)");
            nodep->replaceWith(itemsp->unlinkFrBackWithNext());
            // Will soon iterate itemsp as part of normal visit process
            // These will become sequential in the generated task
        } இல்லையெனில் {
            nodep->unlinkFrBack();
        }
        VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
    }
    வெறுமை visit(AstRSProdItem* nodep) override {
        // "call" to generate using another production
        UINFO(9, "visit " << nodep);
        UASSERT_OBJ(m_prodFuncp, nodep, "RSProdItem not under production");
        AstRSProd* மாறிலி foundp = nodep->prodp();
        UASSERT_OBJ(foundp, nodep, "Unlinked production reference");
        // Pass through caller-side argument list (IEEE 1800-2023 18.17.7).
        AstArg* மாறிலி userArgsp
            = nodep->argsp() ? nodep->argsp()->unlinkFrBackWithNext() : nullptr;
        // Convert to task call
        Ast_முனை* மாறிலி newp = newProdFuncRef(nodep, foundp, m_breakVarp, userArgsp);
        // The production might have done a "break;", skip other steps if so
        newp->addNext(புதிய AstIf{nodep->fileline(),
                                புதிய AstVarRef{nodep->fileline(), m_breakVarp, VAccess::READ},
                                புதிய AstJumpGo{nodep->fileline(), m_jumpBlockp}});

        nodep->replaceWith(newp);
        VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
    }

    வெறுமை visit(AstRSBreak* nodep) override {
        UASSERT_OBJ(m_jumpBlockp, nodep, "RSBreak not under production jump block");
        UASSERT_OBJ(m_breakVarp, nodep, "no break variable");
        Ast_முனை_Stmt* மாறிலி newp = புதிய AstAssign{
            nodep->fileline(), புதிய AstVarRef{nodep->fileline(), m_breakVarp, VAccess::WRITE},
            புதிய AstConst{nodep->fileline(), AstConst::BitTrue{}}};
        newp->addNext(புதிய AstJumpGo{nodep->fileline(), m_jumpBlockp});
        nodep->replaceWith(newp);
        VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
    }
    வெறுமை visit(AstRSReturn* nodep) override {
        UASSERT_OBJ(m_jumpBlockp, nodep, "RSReturn not under production jump block");
        nodep->replaceWith(புதிய AstJumpGo{nodep->fileline(), m_jumpBlockp});
        VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
    }
    வெறுமை visit(AstVarRef* nodep) override {
        // Change references to variables inside randsequence to references to
        // ref variables on the new task
        மாறிலி auto it = m_localizeRemaps.find(nodep->varp());
        எனில் (it != m_localizeRemaps.end()) {
            AstVar* மாறிலி iovarp = it->second;
            UINFO(9, "VARREF remap " << nodep << " -> " << iovarp);
            nodep->varp(iovarp);
        }
    }

    வெறுமை visit(Ast_முனை* nodep) override { iterateChildren(nodep); }

பொது:
    // CONSTRUCTORS
    explicit RandSequenceVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~RandSequenceVisitor() override = இயல்பு;
};

//######################################################################
// RandSequence method class functions

வெறுமை V3RandSequence::randSequenceNetlist(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { மாறிலி RandSequenceVisitor randSequenceVisitor{nodep}; }
    V3Global::dump_Check_Global_மரம்("randsequence", 0, dump_மரம்_Either_Level() >= 3);
}
