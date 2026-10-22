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

#అన్తర్భావ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#అన్తర్భావ "verilatedos.h"

#అన్తర్భావ "V3RandSequence.h"

#అన్తర్భావ "V3Ast.h"
#అన్తర్భావ "V3Error.h"
#అన్తర్భావ "V3FileLine.h"
#అన్తర్భావ "V3Global.h"

VL_DEFINE_DEBUG_FUNCTIONS;

// ######################################################################
//  Matcher classes (for suggestion matching)

తరగతి RSProdMatcher final : బహిరంగ VNodeMatcher {
బహిరంగ:
    తార్కిక కణుపు_సరిపోలిక(స్థిరరాశి Ast_కణుపు* nodep) స్థిరరాశి override { తిరిగి VN_IS(nodep, RSProd); }
};

//######################################################################
// Visitor that marks classes needing a randomize() method

తరగతి RandSequenceVisitor final : బహిరంగ VNVisitor {
    // STATE - global
    std::map<AstRSProd*, Ast_కణుపు_FTask*> m_prodFuncps;  // Generated production functions

    // STATE - for current visit position (use VL_RESTORER)
    Ast_కణుపు_Module* m_modp = శూన్యసూచిక;  // Current module
    AstRandSequence* m_rsp = శూన్యసూచిక;  // Current randsequence
    AstRSProd* m_ప్రారంభం_Prodp = శూన్యసూచిక;  // Starting production
    Ast_కణుపు_FTask* m_prodFuncp = శూన్యసూచిక;  // Production function being built
    Ast_Jump_ఖండం* m_jumpBlockp = శూన్యసూచిక;  // Building jumpblock for return/break
    AstVar* m_breakVarp = శూన్యసూచిక;  // Break variable
    std::unordered_సమితి<AstVar*> m_localizes;  // Variables to become function inouts
    std::map<std::string, AstVar*> m_localize_పేర్లు;  // Ordered names of function inouts
    std::unordered_map<స్థిరరాశి AstVar*, AstVar*> m_localizeRemaps;  // New ports for old vars

    // METHODS
    శూన్య కనుగొను_Localizes(AstRandSequence* nodep) {
        std::సమితి<AstVar*> localVars;
        nodep->foreach([&](Ast_కణుపు* స్థిరరాశి nodep) {
            ఒకవేళ (స్థిరరాశి AstVarRef* స్థిరరాశి anodep = VN_CAST(nodep, VarRef)) {
                m_localizes.emplace(anodep->varp());
            } లేకపోతే ఒకవేళ (AstVar* స్థిరరాశి anodep = VN_CAST(nodep, Var)) {
                localVars.emplace(anodep);
            }
        });
        // Clear every variable allocated in the RandSequence
        కొరకు (AstVar* varp : localVars) m_localizes.erase(varp);
        // Sort by name, so function arguments are stable
        కొరకు (AstVar* varp : m_localizes) {
            UINFO(9, "localize " << varp);
            m_localize_పేర్లు.emplace(varp->name(), varp);
        }
    }

    AstVar* newBreakVar(File_పంక్తి* fl, తార్కిక asOutput) {
        AstVar* స్థిరరాశి varp = కొత్త AstVar{fl, VVarType::పోర్టు, "__VrsBreak", VFlag_బిట్టు_Packed{}, 1};
        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        varp->funcLocal(నిజం);
        ఒకవేళ (asOutput) varp->direction(VDirection::OUTPUT);
        Ast_కణుపు* outp = varp;
        outp->కూడు_తదుపరి(కొత్త Ast_కేటాయించు{fl, కొత్త AstVarRef{fl, varp, VAccess::WRITE},
                                    కొత్త AstConst{fl, AstConst::బిట్టు_False{}}});

        // Also add arguments as next's
        కొరకు (స్థిరరాశి స్వయంచాలక& itr : m_localize_పేర్లు) {
            స్థిరరాశి AstVar* స్థిరరాశి lvarp = itr.రెండవ;
            AstVar* స్థిరరాశి iovarp
                = కొత్త AstVar{fl, VVarType::పోర్టు, "__Vrsarg_" + lvarp->name(), lvarp};
            iovarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            iovarp->funcLocal(నిజం);
            iovarp->direction(VDirection::REF);
            varp->కూడు_తదుపరి(iovarp);
            m_localizeRemaps.emplace(lvarp, iovarp);
        }
        తిరిగి varp;
    }

    AstTask* new_ప్రారంభం_Func(AstRandSequence* nodep) {
        // Create wrapper that is called by the sequence
        AstTask* స్థిరరాశి taskp = కొత్త AstTask{m_ప్రారంభం_Prodp->fileline(), m_rsp->name(), శూన్యసూచిక};
        taskp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        m_modp->కూడు_Stmtsp(taskp);

        // Create local (not output) break variable
        VL_RESTORER_COPY(m_localizeRemaps);
        AstVar* breakVarp = newBreakVar(nodep->fileline(), అబద్ధం);
        taskp->కూడు_Stmtsp(breakVarp);

        // Call the start production's task
        taskp->కూడు_Stmtsp(newProdFuncRef(nodep, m_ప్రారంభం_Prodp, breakVarp, శూన్యసూచిక));

        UINFOTREE(9, taskp, "newStart", "");
        తిరిగి taskp;
    }

    Ast_కణుపు* newProdFuncRef(Ast_కణుపు* nodep, AstRSProd* prodp, AstVar* breakVarp,
                            AstArg* userArgsp) {
        స్వయంచాలక it = m_prodFuncps.కనుగొను(prodp);
        UASSERT_OBJ(it != m_prodFuncps.చివర(), nodep, "No production function made");
        Ast_కణుపు_FTask* స్థిరరాశి prodFuncp = it->రెండవ;
        File_పంక్తి* స్థిరరాశి fl = nodep->fileline();
        // V3Width already ran before V3RandSequence, so VarRefs we create here
        // need dtype set explicitly, V3Broken later checks width == widthMin.
        AstVarRef* స్థిరరాశి breakRefp = కొత్త AstVarRef{fl, breakVarp, VAccess::WRITE};
        breakRefp->dtypeFrom(breakVarp);
        AstArg* స్థిరరాశి argsp = కొత్త AstArg{fl, breakVarp->name(), breakRefp};
        కొరకు (స్థిరరాశి స్వయంచాలక& itr : m_localize_పేర్లు) {
            స్థిరరాశి AstVar* స్థిరరాశి lvarp = itr.రెండవ;
            AstVar* స్థిరరాశి iovarp = m_localizeRemaps[lvarp];
            UASSERT_OBJ(iovarp, nodep, "No new port variable for local variable" << lvarp);
            AstVarRef* స్థిరరాశి refp = కొత్త AstVarRef{fl, iovarp, VAccess::READWRITE};
            refp->dtypeFrom(iovarp);
            argsp->కూడు_తదుపరి(కొత్త AstArg{nodep->fileline(), "__Vrsarg_" + lvarp->name(), refp});
        }
        ఒకవేళ (userArgsp) argsp->కూడు_తదుపరి(userArgsp);
        Ast_కణుపు* స్థిరరాశి newp
            = కొత్త Ast_Stmt_సమాసం{fl, కొత్త AstTaskRef{fl, VN_AS(prodFuncp, Task), argsp}};
        తిరిగి newp;
    }

    శూన్య newProdFunc(AstRSProd* nodep) {
        // Task, as time may pass
        AstTask* స్థిరరాశి newp
            = కొత్త AstTask{nodep->fileline(), m_rsp->name() + "_" + nodep->name(), శూన్యసూచిక};
        m_modp->కూడు_Stmtsp(newp);
        m_prodFuncps.emplace(nodep, newp);
        UINFOTREE(9, newp, "newProd", "");
    }

    Ast_కణుపు* newProdRandJoin(AstRSProd* /*prodp*/, AstRSProdList* prodlistp) {
        // For weight == 1.0 longer sequence (favor stay in a)
        // For weight == 0.0 shorter squence (favor change a/b)
        UASSERT_OBJ(prodlistp->weightp(), prodlistp, "Weight should have default CONST(0.5)");
        Ast_కణుపు_సమాసం* weightp = prodlistp->weightp()->unlink_Fr_వెనుక();

        // Build map of production lists and productions we need to join
        // Must mainain the relative order of each sequence.
        std::deque<Ast_RSProd_అంశం*> lists;
        std::unordered_map<Ast_RSProd_అంశం*, std::deque<Ast_కణుపు_Stmt*>> listStmts;
        కొరకు (Ast_RSProd_అంశం* proditemp = VN_AS(prodlistp->prodsp(), RSProd_అంశం); proditemp;
             proditemp = VN_AS(proditemp->nextp(), RSProd_అంశం)) {
            lists.నెట్టు_వెనుక(proditemp);
            స్థిరరాశి AstRSProd* స్థిరరాశి subProdp = proditemp->prodp();
            ఒకవేళ (!subProdp) కొనసాగు;
            ఒకవేళ (!subProdp->rulesp()) కొనసాగు;
            ఒకవేళ (!subProdp->rulesp()->prodlistsp()) కొనసాగు;
            కొరకు (Ast_కణుపు_Stmt* subStmtp
                 = VN_AS(subProdp->rulesp()->prodlistsp()->prodsp(), కణుపు_Stmt);
                 subStmtp; subStmtp = VN_AS(subStmtp->nextp(), కణుపు_Stmt)) {
                listStmts[proditemp].నెట్టు_వెనుక(subStmtp);
            }
        }

        UINFO(9, "RandJoin productions called:");
        కొరకు (Ast_RSProd_అంశం* proditemp : lists) {
            UINFO(9, "  list " << proditemp);
            కొరకు (స్థిరరాశి Ast_కణుపు_Stmt* prodip : listStmts[proditemp])
                UINFO(9, "    calls " << prodip);
        }

        // Need to clone all nodes used

        File_పంక్తి* fl = prodlistp->fileline();
        Ast_కణుపు* newp = శూన్యసూచిక;

        // "bool _Vjoin_repick = true;"  // Pick a new longer sequence
        AstVar* repickVarp = కొత్త AstVar{fl, VVarType::VAR, "_Vjoin_repick", VFlag_బిట్టు_Packed{}, 1};
        repickVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        repickVarp->funcLocal(నిజం);
        m_breakVarp->కూడు_తదుపరి_ఇక్కడ(repickVarp);  // Add to function top, not newp
        newp = Ast_కణుపు::కూడు_తదుపరి(newp,
                                కొత్త Ast_కేటాయించు{fl, కొత్త AstVarRef{fl, repickVarp, VAccess::WRITE},
                                              కొత్త AstConst{fl, AstConst::బిట్టు_True{}}});

        // "int _Vjoin_nleft_0 = N(items);"  // N(a) Number left to generate in list 0, starts at
        // N(list0)
        std::deque<AstVar*> nleftVarps;
        పూర్ణాంకం i = 0;
        కొరకు (Ast_RSProd_అంశం* proditemp : lists) {
            // 'rand join arule arule arule' is legal, so need numbered, not non-unique named
            // nleft's
            AstVar* స్థిరరాశి varp = కొత్త AstVar{
                fl, VVarType::VAR, "_Vjoin_nleft_" + std::గమ్యం_string(i++), VFlag_బిట్టు_Packed{}, 32};
            varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            varp->funcLocal(నిజం);
            m_breakVarp->కూడు_తదుపరి_ఇక్కడ(varp);  // Add to function top, not newp
            nleftVarps.నెట్టు_వెనుక(varp);

            newp = Ast_కణుపు::కూడు_తదుపరి(
                newp,
                కొత్త Ast_కేటాయించు{fl, కొత్త AstVarRef{fl, varp, VAccess::WRITE},
                              కొత్త AstConst{fl, AstConst::Widthed_విలువ{}, 32,
                                           static_cast<uint32_t>(listStmts[proditemp].కొలత())}});
        }

        // "int _Vjoin_picked_elist = 0;"  // 0 = pick first eligible (non-finished) list, 1 = pick
        // second, etc
        AstVar* స్థిరరాశి pickedVarp
            = కొత్త AstVar{fl, VVarType::VAR, "_Vjoin_picked_elist", VFlag_బిట్టు_Packed{}, 1};
        pickedVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        pickedVarp->funcLocal(నిజం);
        m_breakVarp->కూడు_తదుపరి_ఇక్కడ(pickedVarp);  // Add to function top, not newp

        // "while(1) {"  // Generate until make complete sequence
        Ast_Jump_ఖండం* స్థిరరాశి whilep = కొత్త Ast_Jump_ఖండం{fl, శూన్యసూచిక};
        newp = Ast_కణుపు::కూడు_తదుపరి(newp, కొత్త AstLoop{fl, whilep});
        {
            // "if ((std::rand() & 0xffffUL) > (0xffff * weight)) _Vjoin_repick = true;"
            whilep->కూడు_Stmtsp(కొత్త AstIf{
                fl,
                కొత్త AstGt{
                    fl, కొత్త AstRand{fl, AstRand::SRandomU32{}},
                    కొత్త AstRToIS{fl, కొత్త AstMulD{fl,
                                                 కొత్త AstConst{fl, AstConst::RealDouble{},
                                                              static_cast<ద్విగుణ>(0xffffffffUL)},
                                                 weightp}}},
                కొత్త Ast_కేటాయించు{fl, కొత్త AstVarRef{fl, repickVarp, VAccess::WRITE},
                              కొత్త AstConst{fl, AstConst::బిట్టు_True{}}}});

            // "if (_Vjoin_repick) {"
            AstIf* ifp = కొత్త AstIf{fl, కొత్త AstVarRef{fl, repickVarp, VAccess::READ}};
            whilep->కూడు_Stmtsp(ifp);
            {
                // "// _Vjoin_nlists_eligible is how many lists eligable for picking"
                // "int _Vjoin_nlists_eligible = 0;"
                AstVar* స్థిరరాశి eligibleVarp = కొత్త AstVar{
                    fl, VVarType::VAR, "_Vjoin_nlists_eligible", VFlag_బిట్టు_Packed{}, 32};
                eligibleVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                eligibleVarp->funcLocal(నిజం);
                m_breakVarp->కూడు_తదుపరి_ఇక్కడ(eligibleVarp);  // Add to function top, not newp
                ifp->కూడు_Thensp(కొత్త Ast_కేటాయించు{fl, కొత్త AstVarRef{fl, eligibleVarp, VAccess::WRITE},
                                             కొత్త AstConst{fl, AstConst::Widthed_విలువ{}, 32, 0}});

                i = 0;
                కొరకు (Ast_RSProd_అంశం* proditemp : lists) {
                    (శూన్య)proditemp;
                    // "_Vjoin_nlists_eligible += _Vjoin_nleft_0 ? 1 : 0;"
                    ifp->కూడు_Thensp(కొత్త Ast_కేటాయించు{
                        fl, కొత్త AstVarRef{fl, eligibleVarp, VAccess::WRITE},
                        కొత్త Ast_కూడు{
                            fl, కొత్త AstVarRef{fl, eligibleVarp, VAccess::READ},
                            కొత్త AstCond{
                                fl,
                                కొత్త AstNeq{fl, కొత్త AstConst{fl, AstConst::Widthed_విలువ{}, 32, 0},
                                           కొత్త AstVarRef{fl, nleftVarps[i], VAccess::READ}},
                                కొత్త AstConst{fl, AstConst::Widthed_విలువ{}, 32, 1},
                                కొత్త AstConst{fl, AstConst::Widthed_విలువ{}, 32, 0}}}});
                    ++i;
                }

                // "if (!_Vjoin_nlists_eligible) break;"  // Out of elements
                ifp->కూడు_Thensp(
                    కొత్త AstIf{fl,
                              కొత్త AstEq{fl, కొత్త AstConst{fl, AstConst::Widthed_విలువ{}, 32, 0},
                                        కొత్త AstVarRef{fl, eligibleVarp, VAccess::READ}},
                              కొత్త AstJumpGo{fl, m_jumpBlockp}});

                // "// +1 to simplify usage, so 0 = done"
                // "_Vjoin_picked_elist = 1 + (std::rand() % _Vjoin_nlists_eligible);"
                ifp->కూడు_Thensp(కొత్త Ast_కేటాయించు{
                    fl, కొత్త AstVarRef{fl, pickedVarp, VAccess::WRITE},
                    కొత్త Ast_కూడు{fl, కొత్త AstConst{fl, AstConst::Widthed_విలువ{}, 32, 1},
                               కొత్త AstModDiv{fl, కొత్త AstRand{fl, AstRand::SRandomU32{}},
                                             కొత్త AstVarRef{fl, eligibleVarp, VAccess::READ}}}});

                // "_Vjoin_repick = false;
                ifp->కూడు_Thensp(కొత్త Ast_కేటాయించు{fl, కొత్త AstVarRef{fl, repickVarp, VAccess::WRITE},
                                             కొత్త AstConst{fl, AstConst::బిట్టు_False{}}});
            }

            // "// Number of eligible lists left before hit _Vjoin_picked one"
            // "int _Vjoin_sel_elist = _Vjoin_picked_elist;"
            AstVar* స్థిరరాశి selVarp
                = కొత్త AstVar{fl, VVarType::VAR, "_Vjoin_sel_elist", VFlag_బిట్టు_Packed{}, 32};
            selVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            selVarp->funcLocal(నిజం);
            m_breakVarp->కూడు_తదుపరి_ఇక్కడ(selVarp);  // Add to function top, not newp
            whilep->కూడు_Stmtsp(కొత్త Ast_కేటాయించు{fl, కొత్త AstVarRef{fl, selVarp, VAccess::WRITE},
                                            కొత్త AstVarRef{fl, pickedVarp, VAccess::READ}});

            i = -1;
            కొరకు (Ast_RSProd_అంశం* proditemp : lists) {
                ++i;
                // "if (_Vjoin_nleft_0) {"
                ifp = కొత్త AstIf{fl,
                                కొత్త AstNeq{fl, కొత్త AstConst{fl, AstConst::Widthed_విలువ{}, 32, 0},
                                           కొత్త AstVarRef{fl, nleftVarps[i], VAccess::READ}},
                                శూన్యసూచిక, శూన్యసూచిక};
                whilep->కూడు_Stmtsp(ifp);
                {
                    // "--_Vjoin_sel_elist;"
                    ifp->కూడు_Thensp(కొత్త Ast_కేటాయించు{
                        fl, కొత్త AstVarRef{fl, selVarp, VAccess::WRITE},
                        కొత్త AstSub{fl, కొత్త AstVarRef{fl, selVarp, VAccess::READ},
                                   కొత్త AstConst{fl, AstConst::Widthed_విలువ{}, 32, 1}}});

                    // "if (_Vjoin_sel_elist == 0) {"
                    AstIf* స్థిరరాశి jIfp = కొత్త AstIf{
                        fl,
                        కొత్త AstEq{fl, కొత్త AstConst{fl, AstConst::Widthed_విలువ{}, 32, 0},
                                  కొత్త AstVarRef{fl, selVarp, VAccess::READ}},
                        శూన్యసూచిక, శూన్యసూచిక};
                    ifp->కూడు_Thensp(jIfp);
                    {
                        // "switch (_Vjoin_nleft_0) {"
                        //     "case 2 / * N(a)  * /: {statement}; break;"
                        //     "case 1 / * N(a) - 1 * /: {statement}; break;"
                        uint32_t j = static_cast<uint32_t>(listStmts[proditemp].కొలత());
                        కొరకు (Ast_కణుపు_Stmt* iprodp : listStmts[proditemp]) {
                            jIfp->కూడు_Thensp(కొత్త AstIf{
                                fl,
                                కొత్త AstEq{fl, కొత్త AstConst{fl, AstConst::Widthed_విలువ{}, 32, j},
                                          కొత్త AstVarRef{fl, nleftVarps[i], VAccess::READ}},
                                iprodp->clone_వృక్షం(అబద్ధం)});
                            --j;
                        }

                        // "--_Vjoin_nleft_0;"
                        jIfp->కూడు_Thensp(కొత్త Ast_కేటాయించు{
                            fl, కొత్త AstVarRef{fl, nleftVarps[i], VAccess::WRITE},
                            కొత్త AstSub{fl, కొత్త AstVarRef{fl, nleftVarps[i], VAccess::READ},
                                       కొత్త AstConst{fl, AstConst::Widthed_విలువ{}, 32, 1}}});

                        // "if (0 == _Vjoin_nleft_0) _Vjoin_repick = true;"
                        jIfp->కూడు_Thensp(కొత్త AstIf{
                            fl,
                            కొత్త AstEq{fl, కొత్త AstConst{fl, AstConst::Widthed_విలువ{}, 32, 0},
                                      కొత్త AstVarRef{fl, nleftVarps[i], VAccess::READ}},
                            కొత్త Ast_కేటాయించు{fl, కొత్త AstVarRef{fl, repickVarp, VAccess::WRITE},
                                          కొత్త AstConst{fl, AstConst::బిట్టు_True{}}}});

                        // "continue;"
                        jIfp->కూడు_Thensp(కొత్త AstJumpGo{fl, whilep});
                    }
                }
            }
        }

        UINFOTREE(9, newp, "ForkJoin new", "-");
        తిరిగి newp;
    }

    // VISITORS

    శూన్య సందర్శించు(Ast_కణుపు_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildren(nodep);
    }

    శూన్య సందర్శించు(AstRandSequence* nodep) override {
        UINFO(9, "visit " << nodep);
        UASSERT_OBJ(m_modp, nodep, "randsequence not under module");
        ఒకవేళ (m_rsp) {
            // No examples found in the wild
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: randsequence under randsequence");
            VL_DO_DANGLING(నెట్టు_Deletep(nodep->unlink_Fr_వెనుక()), nodep);
            తిరిగి;
        }

        UINFOTREE(9, nodep, "rsIn", "");
        VL_RESTORER(m_rsp);
        m_rsp = nodep;
        VL_RESTORER(m_ప్రారంభం_Prodp);

        VL_RESTORER_COPY(m_localizes);
        VL_RESTORER_COPY(m_localize_పేర్లు);
        VL_RESTORER_COPY(m_localizeRemaps);
        కనుగొను_Localizes(nodep);

        // Find first production
        m_ప్రారంభం_Prodp = nodep->prodsp();
        ఒకవేళ (nodep->prodp()) m_ప్రారంభం_Prodp = nodep->prodp();  // Has a start production selected

        // Build production functions
        కొరకు (AstRSProd* prodp = nodep->prodsp(); prodp; prodp = VN_AS(prodp->nextp(), RSProd)) {
            newProdFunc(prodp);
            // Don't iterate yet, need to have tasks we can reference
        }

        // Iterate all of the new productions, moving guts into functions just made
        // Must do those with 'rand join' first, as they need to see the raw AstRS* nodes
        // before they are removed.  (Alternative would be to keep them until the end).
        UINFOTREE(9, nodep, "RS Tree pre-it", "-");
        std::unordered_సమితి<AstRSProd*> prodHasRandJoin;
        కొరకు (AstRSProd* prodp = nodep->prodsp(); prodp; prodp = VN_AS(prodp->nextp(), RSProd)) {
            prodp->foreach([&](స్థిరరాశి AstRSProdList* స్థిరరాశి prodlistp) {
                ఒకవేళ (prodlistp->randJoin()) prodHasRandJoin.emplace(prodp);
            });
        }
        కొరకు (AstRSProd *nextp, *prodp = nodep->prodsp(); prodp; prodp = nextp) {
            nextp = VN_AS(prodp->nextp(), RSProd);
            ఒకవేళ (prodHasRandJoin.లెక్క(prodp)) VL_DO_DANGLING(iterate(prodp), prodp);
        }
        కొరకు (AstRSProd *nextp, *prodp = nodep->prodsp(); prodp; prodp = nextp) {
            nextp = VN_AS(prodp->nextp(), RSProd);
            ఒకవేళ (!prodHasRandJoin.లెక్క(prodp)) VL_DO_DANGLING(iterate(prodp), prodp);
        }
        UINFOTREE(9, nodep, "RS Tree post-it", "-");

        // Replace randsequence with call to randsequence start function
        AstTask* స్థిరరాశి ప్రారంభం_Funcp = new_ప్రారంభం_Func(nodep);
        AstArg* argsp = శూన్యసూచిక;
        కొరకు (స్వయంచాలక& itr : m_localize_పేర్లు) {
            AstVar* స్థిరరాశి lvarp = itr.రెండవ;
            argsp = Ast_కణుపు::కూడు_తదుపరి(
                argsp, కొత్త AstArg{nodep->fileline(), "__Vrsarg_" + lvarp->name(),
                                  కొత్త AstVarRef{nodep->fileline(), lvarp, VAccess::READWRITE}});
        }
        AstTaskRef* స్థిరరాశి callStartp
            = కొత్త AstTaskRef{nodep->fileline(), ప్రారంభం_Funcp->name(), argsp};
        callStartp->taskp(ప్రారంభం_Funcp);
        nodep->replaceWith(కొత్త Ast_Stmt_సమాసం{nodep->fileline(), callStartp});

        VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
    }
    శూన్య సందర్శించు(AstRSProd* nodep) override {
        // Production, immediately under a randsequence
        UINFO(9, "visit " << nodep);
        UINFOTREE(9, nodep, "visit " << nodep, "-");
        UASSERT_OBJ(!m_prodFuncp, nodep, "recursive production definition?");
        VL_RESTORER(m_prodFuncp);
        స్వయంచాలక it = m_prodFuncps.కనుగొను(nodep);
        UASSERT_OBJ(it != m_prodFuncps.చివర(), nodep, "No production function made");
        m_prodFuncp = it->రెండవ;

        // Create a break variable
        // TODO we could do this only if break exists in the downstream production,
        // but as-is we'll optimize it away in most cases anyways
        VL_RESTORER(m_breakVarp);
        VL_RESTORER_COPY(m_localizeRemaps);
        m_breakVarp = newBreakVar(nodep->fileline(), నిజం);
        m_prodFuncp->కూడు_Stmtsp(m_breakVarp);

        // Production formal ports become real input ports on the generated task.
        // Must be added after m_breakVarp / localize ports so positional argument
        // order in newProdFuncRef stays consistent: [break, localize..., user...].
        ఒకవేళ (Ast_కణుపు* స్థిరరాశి portsp = nodep->portsp()) {
            portsp->unlink_Fr_వెనుక_With_తదుపరి();
            m_prodFuncp->కూడు_Stmtsp(portsp);
            కొరకు (Ast_కణుపు* itp = portsp; itp; itp = itp->nextp()) {
                AstVar* స్థిరరాశి పోర్టు_Varp = VN_AS(itp, Var);
                పోర్టు_Varp->funcLocal(నిజం);
                పోర్టు_Varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            }
        }

        // Put JumpBlock immediately under the new function to support
        // a future break/return.  V3Const will rip it out if unneeded.
        VL_RESTORER(m_jumpBlockp);
        m_jumpBlockp = కొత్త Ast_Jump_ఖండం{nodep->fileline(), శూన్యసూచిక};
        m_prodFuncp->కూడు_Stmtsp(m_jumpBlockp);

        ఒకవేళ (nodep->fvarp())
            nodep->fvarp()->v3warn(E_UNSUPPORTED,
                                   "Unsupported: randsequence production function variable");

        // Move children into m_prodFuncp, and iterate there
        ఒకవేళ (!nodep->rulesp()) {  // Nothing to do
        } లేకపోతే ఒకవేళ (nodep->rulesp()->prodlistsp() && nodep->rulesp()->prodlistsp()->randJoin()) {
            AstRSProdList* స్థిరరాశి prodlistp = nodep->rulesp()->prodlistsp();
            Ast_కణుపు* స్థిరరాశి itemsp = newProdRandJoin(nodep, prodlistp);
            m_jumpBlockp->కూడు_Stmtsp(itemsp);
            iterate_And_తదుపరి_Null(itemsp);
        } లేకపోతే ఒకవేళ (!nodep->rulesp()->nextp()) {  // Single rule/list, can just do it
            // RSPROD(RSRULE(weight, stmt)) -> IF(weight != 0, stmt)
            స్థిరరాశి AstRSRule* స్థిరరాశి rulep = nodep->rulesp();
            Ast_కణుపు* itemsp = శూన్యసూచిక;
            ఒకవేళ (rulep->బరువు_Stmtsp()) itemsp = rulep->బరువు_Stmtsp()->unlink_Fr_వెనుక_With_తదుపరి();
            ఒకవేళ (rulep->prodlistsp())
                itemsp = Ast_కణుపు::కూడు_తదుపరి(itemsp, rulep->prodlistsp()->unlink_Fr_వెనుక_With_తదుపరి());
            // Can ignore rulep->weightp() unless is zero, in which case noop, add if
            ఒకవేళ (rulep->weightp())
                itemsp = కొత్త AstIf{rulep->weightp()->fileline(),
                                   కొత్త AstNeq{rulep->weightp()->fileline(),
                                              కొత్త AstConst{rulep->weightp()->fileline(), 0},
                                              rulep->weightp()->unlink_Fr_వెనుక()},
                                   itemsp};
            // Move prodlist to parent (m_prodFunc) and process
            ఒకవేళ (itemsp) {
                UINFOTREE(9, itemsp, "additems", "-");
                m_jumpBlockp->కూడు_Stmtsp(itemsp);
                iterate_And_తదుపరి_Null(itemsp);
                UINFOTREE(9, m_jumpBlockp, "jumpBlockNow", "-");
            }
        } లేకపోతే {
            // List of rules with "OR", need to random-weight them
            // RSPROD(RSRULE(weight1, stmt!), RSRULE(weight2, stmt2))
            //   -> RANDCASE(CASEITEM(weight1, stmt1), CASEITEM(weight2, stmt2))
            AstRandCase* స్థిరరాశి randcasep = కొత్త AstRandCase{nodep->fileline(), శూన్యసూచిక};
            కొరకు (AstRSRule* rulep = nodep->rulesp(); rulep;
                 rulep = VN_AS(rulep->nextp(), RSRule)) {
                Ast_కణుపు* itemsp = శూన్యసూచిక;
                ఒకవేళ (rulep->బరువు_Stmtsp()) itemsp = rulep->బరువు_Stmtsp()->unlink_Fr_వెనుక_With_తదుపరి();
                ఒకవేళ (rulep->prodlistsp())
                    itemsp = Ast_కణుపు::కూడు_తదుపరి(itemsp, rulep->prodlistsp()->unlink_Fr_వెనుక_With_తదుపరి());
                ఒకవేళ (itemsp) {
                    Ast_కణుపు_సమాసం* స్థిరరాశి weightp = rulep->weightp()
                                                     ? rulep->weightp()->unlink_Fr_వెనుక()
                                                     : కొత్త AstConst{rulep->fileline(), 1};
                    randcasep->కూడు_Itemsp(కొత్త Ast_Case_అంశం{rulep->fileline(), weightp, itemsp});
                }
            }
            m_jumpBlockp->కూడు_Stmtsp(randcasep);
            iterate_And_తదుపరి_Null(randcasep);
        }

        // V3Task needs to know if we ended up making a recursive function
        m_prodFuncp->foreach([&](Ast_కణుపు_FTask_Ref* స్థిరరాశి nodep) {
            ఒకవేళ (nodep->taskp() == m_prodFuncp) m_prodFuncp->recursive(నిజం);
        });
        UINFOTREE(9, m_prodFuncp, "rsprod-task-done " << nodep, "-");

        // Done with production, should have moved everything to new task
        VL_DO_DANGLING(నెట్టు_Deletep(nodep->unlink_Fr_వెనుక()), nodep);
    }
    శూన్య సందర్శించు(AstRSRule* nodep) override {
        // Rule of what to produce (as a list) under a production
        nodep->v3fatalSrc("randsequence rule should be iterated and deleted by AstRSProd");
    }
    శూన్య సందర్శించు(AstRSProdList* nodep) override {
        // List of productions to call, potentially with a weight
        UINFO(9, "visit " << nodep);
        UASSERT_OBJ(m_prodFuncp, nodep, "RSProdList not under production");

        // Move prodlist to parent (m_prodFunc) and process
        ఒకవేళ (Ast_కణుపు* స్థిరరాశి itemsp = nodep->prodsp()) {
            UASSERT_OBJ(!nodep->randJoin(), nodep,
                        "rand join should have been handled in visit(RSRule)");
            nodep->replaceWith(itemsp->unlink_Fr_వెనుక_With_తదుపరి());
            // Will soon iterate itemsp as part of normal visit process
            // These will become sequential in the generated task
        } లేకపోతే {
            nodep->unlink_Fr_వెనుక();
        }
        VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
    }
    శూన్య సందర్శించు(Ast_RSProd_అంశం* nodep) override {
        // "call" to generate using another production
        UINFO(9, "visit " << nodep);
        UASSERT_OBJ(m_prodFuncp, nodep, "RSProdItem not under production");
        AstRSProd* స్థిరరాశి foundp = nodep->prodp();
        UASSERT_OBJ(foundp, nodep, "Unlinked production reference");
        // Pass through caller-side argument list (IEEE 1800-2023 18.17.7).
        AstArg* స్థిరరాశి userArgsp
            = nodep->argsp() ? nodep->argsp()->unlink_Fr_వెనుక_With_తదుపరి() : శూన్యసూచిక;
        // Convert to task call
        Ast_కణుపు* స్థిరరాశి newp = newProdFuncRef(nodep, foundp, m_breakVarp, userArgsp);
        // The production might have done a "break;", skip other steps if so
        newp->కూడు_తదుపరి(కొత్త AstIf{nodep->fileline(),
                                కొత్త AstVarRef{nodep->fileline(), m_breakVarp, VAccess::READ},
                                కొత్త AstJumpGo{nodep->fileline(), m_jumpBlockp}});

        nodep->replaceWith(newp);
        VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
    }

    శూన్య సందర్శించు(AstRSBreak* nodep) override {
        UASSERT_OBJ(m_jumpBlockp, nodep, "RSBreak not under production jump block");
        UASSERT_OBJ(m_breakVarp, nodep, "no break variable");
        Ast_కణుపు_Stmt* స్థిరరాశి newp = కొత్త Ast_కేటాయించు{
            nodep->fileline(), కొత్త AstVarRef{nodep->fileline(), m_breakVarp, VAccess::WRITE},
            కొత్త AstConst{nodep->fileline(), AstConst::బిట్టు_True{}}};
        newp->కూడు_తదుపరి(కొత్త AstJumpGo{nodep->fileline(), m_jumpBlockp});
        nodep->replaceWith(newp);
        VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
    }
    శూన్య సందర్శించు(AstRSReturn* nodep) override {
        UASSERT_OBJ(m_jumpBlockp, nodep, "RSReturn not under production jump block");
        nodep->replaceWith(కొత్త AstJumpGo{nodep->fileline(), m_jumpBlockp});
        VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
    }
    శూన్య సందర్శించు(AstVarRef* nodep) override {
        // Change references to variables inside randsequence to references to
        // ref variables on the new task
        స్థిరరాశి స్వయంచాలక it = m_localizeRemaps.కనుగొను(nodep->varp());
        ఒకవేళ (it != m_localizeRemaps.చివర()) {
            AstVar* స్థిరరాశి iovarp = it->రెండవ;
            UINFO(9, "VARREF remap " << nodep << " -> " << iovarp);
            nodep->varp(iovarp);
        }
    }

    శూన్య సందర్శించు(Ast_కణుపు* nodep) override { iterateChildren(nodep); }

బహిరంగ:
    // CONSTRUCTORS
    స్పష్టమైన RandSequenceVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~RandSequenceVisitor() override = అప్రమేయ;
};

//######################################################################
// RandSequence method class functions

శూన్య V3RandSequence::randSequenceNetlist(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { స్థిరరాశి RandSequenceVisitor randSequenceVisitor{nodep}; }
    V3Global::dump_Check_Global_వృక్షం("randsequence", 0, dump_వృక్షం_Either_Level() >= 3);
}
