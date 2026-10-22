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
    ತಾರ್ಕಿಕ ಗಂಟು_ಹೊಂದಿಕೆ(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep) ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ VN_IS(nodep, RSProd); }
};

//######################################################################
// Visitor that marks classes needing a randomize() method

ವರ್ಗ RandSequenceVisitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
    // STATE - global
    std::map<AstRSProd*, Ast_ಗಂಟು_FTask*> m_prodFuncps;  // Generated production functions

    // STATE - for current visit position (use VL_RESTORER)
    Ast_ಗಂಟು_Module* m_modp = ಶೂನ್ಯಸೂಚಕ;  // Current module
    AstRandSequence* m_rsp = ಶೂನ್ಯಸೂಚಕ;  // Current randsequence
    AstRSProd* m_ಆರಂಭ_Prodp = ಶೂನ್ಯಸೂಚಕ;  // Starting production
    Ast_ಗಂಟು_FTask* m_prodFuncp = ಶೂನ್ಯಸೂಚಕ;  // Production function being built
    Ast_Jump_ಖಂಡ* m_jumpBlockp = ಶೂನ್ಯಸೂಚಕ;  // Building jumpblock for return/break
    AstVar* m_breakVarp = ಶೂನ್ಯಸೂಚಕ;  // Break variable
    std::unordered_ಗಣ<AstVar*> m_localizes;  // Variables to become function inouts
    std::map<std::string, AstVar*> m_localize_ಹೆಸರುಗಳು;  // Ordered names of function inouts
    std::unordered_map<ಸ್ಥಿರಾಂಕ AstVar*, AstVar*> m_localizeRemaps;  // New ports for old vars

    // METHODS
    ಶೂನ್ಯ ಕಂಡುಹಿಡಿ_Localizes(AstRandSequence* nodep) {
        std::ಗಣ<AstVar*> localVars;
        nodep->foreach([&](Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ nodep) {
            ಆದರೆ (ಸ್ಥಿರಾಂಕ AstVarRef* ಸ್ಥಿರಾಂಕ anodep = VN_CAST(nodep, VarRef)) {
                m_localizes.emplace(anodep->varp());
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (AstVar* ಸ್ಥಿರಾಂಕ anodep = VN_CAST(nodep, Var)) {
                localVars.emplace(anodep);
            }
        });
        // Clear every variable allocated in the RandSequence
        ಗಾಗಿ (AstVar* varp : localVars) m_localizes.erase(varp);
        // Sort by name, so function arguments are stable
        ಗಾಗಿ (AstVar* varp : m_localizes) {
            UINFO(9, "localize " << varp);
            m_localize_ಹೆಸರುಗಳು.emplace(varp->name(), varp);
        }
    }

    AstVar* newBreakVar(File_ಸಾಲು* fl, ತಾರ್ಕಿಕ asOutput) {
        AstVar* ಸ್ಥಿರಾಂಕ varp = ಹೊಸ AstVar{fl, VVarType::ಪೋರ್ಟ್, "__VrsBreak", VFlag_ಬಿಟ್_Packed{}, 1};
        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        varp->funcLocal(ನಿಜ);
        ಆದರೆ (asOutput) varp->direction(VDirection::OUTPUT);
        Ast_ಗಂಟು* outp = varp;
        outp->ಕೂಡಿಸು_ಮುಂದಿನ(ಹೊಸ Ast_ನಿಯೋಜಿಸು{fl, ಹೊಸ AstVarRef{fl, varp, VAccess::WRITE},
                                    ಹೊಸ AstConst{fl, AstConst::ಬಿಟ್_False{}}});

        // Also add arguments as next's
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& itr : m_localize_ಹೆಸರುಗಳು) {
            ಸ್ಥಿರಾಂಕ AstVar* ಸ್ಥಿರಾಂಕ lvarp = itr.ಎರಡನೇ;
            AstVar* ಸ್ಥಿರಾಂಕ iovarp
                = ಹೊಸ AstVar{fl, VVarType::ಪೋರ್ಟ್, "__Vrsarg_" + lvarp->name(), lvarp};
            iovarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            iovarp->funcLocal(ನಿಜ);
            iovarp->direction(VDirection::REF);
            varp->ಕೂಡಿಸು_ಮುಂದಿನ(iovarp);
            m_localizeRemaps.emplace(lvarp, iovarp);
        }
        ಹಿಂತಿರುಗಿ varp;
    }

    AstTask* new_ಆರಂಭ_Func(AstRandSequence* nodep) {
        // Create wrapper that is called by the sequence
        AstTask* ಸ್ಥಿರಾಂಕ taskp = ಹೊಸ AstTask{m_ಆರಂಭ_Prodp->fileline(), m_rsp->name(), ಶೂನ್ಯಸೂಚಕ};
        taskp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        m_modp->ಕೂಡಿಸು_Stmtsp(taskp);

        // Create local (not output) break variable
        VL_RESTORER_COPY(m_localizeRemaps);
        AstVar* breakVarp = newBreakVar(nodep->fileline(), ಸುಳ್ಳು);
        taskp->ಕೂಡಿಸು_Stmtsp(breakVarp);

        // Call the start production's task
        taskp->ಕೂಡಿಸು_Stmtsp(newProdFuncRef(nodep, m_ಆರಂಭ_Prodp, breakVarp, ಶೂನ್ಯಸೂಚಕ));

        UINFOTREE(9, taskp, "newStart", "");
        ಹಿಂತಿರುಗಿ taskp;
    }

    Ast_ಗಂಟು* newProdFuncRef(Ast_ಗಂಟು* nodep, AstRSProd* prodp, AstVar* breakVarp,
                            AstArg* userArgsp) {
        ಸ್ವಯಂಚಾಲಿತ it = m_prodFuncps.ಕಂಡುಹಿಡಿ(prodp);
        UASSERT_OBJ(it != m_prodFuncps.ಕೊನೆ(), nodep, "No production function made");
        Ast_ಗಂಟು_FTask* ಸ್ಥಿರಾಂಕ prodFuncp = it->ಎರಡನೇ;
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = nodep->fileline();
        // V3Width already ran before V3RandSequence, so VarRefs we create here
        // need dtype set explicitly, V3Broken later checks width == widthMin.
        AstVarRef* ಸ್ಥಿರಾಂಕ breakRefp = ಹೊಸ AstVarRef{fl, breakVarp, VAccess::WRITE};
        breakRefp->dtypeFrom(breakVarp);
        AstArg* ಸ್ಥಿರಾಂಕ argsp = ಹೊಸ AstArg{fl, breakVarp->name(), breakRefp};
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& itr : m_localize_ಹೆಸರುಗಳು) {
            ಸ್ಥಿರಾಂಕ AstVar* ಸ್ಥಿರಾಂಕ lvarp = itr.ಎರಡನೇ;
            AstVar* ಸ್ಥಿರಾಂಕ iovarp = m_localizeRemaps[lvarp];
            UASSERT_OBJ(iovarp, nodep, "No new port variable for local variable" << lvarp);
            AstVarRef* ಸ್ಥಿರಾಂಕ refp = ಹೊಸ AstVarRef{fl, iovarp, VAccess::READWRITE};
            refp->dtypeFrom(iovarp);
            argsp->ಕೂಡಿಸು_ಮುಂದಿನ(ಹೊಸ AstArg{nodep->fileline(), "__Vrsarg_" + lvarp->name(), refp});
        }
        ಆದರೆ (userArgsp) argsp->ಕೂಡಿಸು_ಮುಂದಿನ(userArgsp);
        Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ newp
            = ಹೊಸ Ast_Stmt_ಅಭಿವ್ಯಕ್ತಿ{fl, ಹೊಸ AstTaskRef{fl, VN_AS(prodFuncp, Task), argsp}};
        ಹಿಂತಿರುಗಿ newp;
    }

    ಶೂನ್ಯ newProdFunc(AstRSProd* nodep) {
        // Task, as time may pass
        AstTask* ಸ್ಥಿರಾಂಕ newp
            = ಹೊಸ AstTask{nodep->fileline(), m_rsp->name() + "_" + nodep->name(), ಶೂನ್ಯಸೂಚಕ};
        m_modp->ಕೂಡಿಸು_Stmtsp(newp);
        m_prodFuncps.emplace(nodep, newp);
        UINFOTREE(9, newp, "newProd", "");
    }

    Ast_ಗಂಟು* newProdRandJoin(AstRSProd* /*prodp*/, AstRSProdList* prodlistp) {
        // For weight == 1.0 longer sequence (favor stay in a)
        // For weight == 0.0 shorter squence (favor change a/b)
        UASSERT_OBJ(prodlistp->weightp(), prodlistp, "Weight should have default CONST(0.5)");
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* weightp = prodlistp->weightp()->unlink_Fr_ಹಿಂಭಾಗ();

        // Build map of production lists and productions we need to join
        // Must mainain the relative order of each sequence.
        std::deque<Ast_RSProd_ಅಂಶ*> lists;
        std::unordered_map<Ast_RSProd_ಅಂಶ*, std::deque<Ast_ಗಂಟು_Stmt*>> listStmts;
        ಗಾಗಿ (Ast_RSProd_ಅಂಶ* proditemp = VN_AS(prodlistp->prodsp(), RSProd_ಅಂಶ); proditemp;
             proditemp = VN_AS(proditemp->nextp(), RSProd_ಅಂಶ)) {
            lists.ತಳ್ಳು_ಹಿಂಭಾಗ(proditemp);
            ಸ್ಥಿರಾಂಕ AstRSProd* ಸ್ಥಿರಾಂಕ subProdp = proditemp->prodp();
            ಆದರೆ (!subProdp) ಮುಂದುವರಿ;
            ಆದರೆ (!subProdp->rulesp()) ಮುಂದುವರಿ;
            ಆದರೆ (!subProdp->rulesp()->prodlistsp()) ಮುಂದುವರಿ;
            ಗಾಗಿ (Ast_ಗಂಟು_Stmt* subStmtp
                 = VN_AS(subProdp->rulesp()->prodlistsp()->prodsp(), ಗಂಟು_Stmt);
                 subStmtp; subStmtp = VN_AS(subStmtp->nextp(), ಗಂಟು_Stmt)) {
                listStmts[proditemp].ತಳ್ಳು_ಹಿಂಭಾಗ(subStmtp);
            }
        }

        UINFO(9, "RandJoin productions called:");
        ಗಾಗಿ (Ast_RSProd_ಅಂಶ* proditemp : lists) {
            UINFO(9, "  list " << proditemp);
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Stmt* prodip : listStmts[proditemp])
                UINFO(9, "    calls " << prodip);
        }

        // Need to clone all nodes used

        File_ಸಾಲು* fl = prodlistp->fileline();
        Ast_ಗಂಟು* newp = ಶೂನ್ಯಸೂಚಕ;

        // "bool _Vjoin_repick = true;"  // Pick a new longer sequence
        AstVar* repickVarp = ಹೊಸ AstVar{fl, VVarType::VAR, "_Vjoin_repick", VFlag_ಬಿಟ್_Packed{}, 1};
        repickVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        repickVarp->funcLocal(ನಿಜ);
        m_breakVarp->ಕೂಡಿಸು_ಮುಂದಿನ_ಇಲ್ಲಿ(repickVarp);  // Add to function top, not newp
        newp = Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ(newp,
                                ಹೊಸ Ast_ನಿಯೋಜಿಸು{fl, ಹೊಸ AstVarRef{fl, repickVarp, VAccess::WRITE},
                                              ಹೊಸ AstConst{fl, AstConst::ಬಿಟ್_True{}}});

        // "int _Vjoin_nleft_0 = N(items);"  // N(a) Number left to generate in list 0, starts at
        // N(list0)
        std::deque<AstVar*> nleftVarps;
        ಪೂರ್ಣಾಂಕ i = 0;
        ಗಾಗಿ (Ast_RSProd_ಅಂಶ* proditemp : lists) {
            // 'rand join arule arule arule' is legal, so need numbered, not non-unique named
            // nleft's
            AstVar* ಸ್ಥಿರಾಂಕ varp = ಹೊಸ AstVar{
                fl, VVarType::VAR, "_Vjoin_nleft_" + std::ಗಮ್ಯ_string(i++), VFlag_ಬಿಟ್_Packed{}, 32};
            varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            varp->funcLocal(ನಿಜ);
            m_breakVarp->ಕೂಡಿಸು_ಮುಂದಿನ_ಇಲ್ಲಿ(varp);  // Add to function top, not newp
            nleftVarps.ತಳ್ಳು_ಹಿಂಭಾಗ(varp);

            newp = Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ(
                newp,
                ಹೊಸ Ast_ನಿಯೋಜಿಸು{fl, ಹೊಸ AstVarRef{fl, varp, VAccess::WRITE},
                              ಹೊಸ AstConst{fl, AstConst::Widthed_ಮೌಲ್ಯ{}, 32,
                                           static_cast<uint32_t>(listStmts[proditemp].ಗಾತ್ರ_2())}});
        }

        // "int _Vjoin_picked_elist = 0;"  // 0 = pick first eligible (non-finished) list, 1 = pick
        // second, etc
        AstVar* ಸ್ಥಿರಾಂಕ pickedVarp
            = ಹೊಸ AstVar{fl, VVarType::VAR, "_Vjoin_picked_elist", VFlag_ಬಿಟ್_Packed{}, 1};
        pickedVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        pickedVarp->funcLocal(ನಿಜ);
        m_breakVarp->ಕೂಡಿಸು_ಮುಂದಿನ_ಇಲ್ಲಿ(pickedVarp);  // Add to function top, not newp

        // "while(1) {"  // Generate until make complete sequence
        Ast_Jump_ಖಂಡ* ಸ್ಥಿರಾಂಕ whilep = ಹೊಸ Ast_Jump_ಖಂಡ{fl, ಶೂನ್ಯಸೂಚಕ};
        newp = Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ(newp, ಹೊಸ AstLoop{fl, whilep});
        {
            // "if ((std::rand() & 0xffffUL) > (0xffff * weight)) _Vjoin_repick = true;"
            whilep->ಕೂಡಿಸು_Stmtsp(ಹೊಸ AstIf{
                fl,
                ಹೊಸ AstGt{
                    fl, ಹೊಸ AstRand{fl, AstRand::SRandomU32{}},
                    ಹೊಸ AstRToIS{fl, ಹೊಸ AstMulD{fl,
                                                 ಹೊಸ AstConst{fl, AstConst::RealDouble{},
                                                              static_cast<ದ್ವಿಗುಣ>(0xffffffffUL)},
                                                 weightp}}},
                ಹೊಸ Ast_ನಿಯೋಜಿಸು{fl, ಹೊಸ AstVarRef{fl, repickVarp, VAccess::WRITE},
                              ಹೊಸ AstConst{fl, AstConst::ಬಿಟ್_True{}}}});

            // "if (_Vjoin_repick) {"
            AstIf* ifp = ಹೊಸ AstIf{fl, ಹೊಸ AstVarRef{fl, repickVarp, VAccess::READ}};
            whilep->ಕೂಡಿಸು_Stmtsp(ifp);
            {
                // "// _Vjoin_nlists_eligible is how many lists eligable for picking"
                // "int _Vjoin_nlists_eligible = 0;"
                AstVar* ಸ್ಥಿರಾಂಕ eligibleVarp = ಹೊಸ AstVar{
                    fl, VVarType::VAR, "_Vjoin_nlists_eligible", VFlag_ಬಿಟ್_Packed{}, 32};
                eligibleVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                eligibleVarp->funcLocal(ನಿಜ);
                m_breakVarp->ಕೂಡಿಸು_ಮುಂದಿನ_ಇಲ್ಲಿ(eligibleVarp);  // Add to function top, not newp
                ifp->ಕೂಡಿಸು_Thensp(ಹೊಸ Ast_ನಿಯೋಜಿಸು{fl, ಹೊಸ AstVarRef{fl, eligibleVarp, VAccess::WRITE},
                                             ಹೊಸ AstConst{fl, AstConst::Widthed_ಮೌಲ್ಯ{}, 32, 0}});

                i = 0;
                ಗಾಗಿ (Ast_RSProd_ಅಂಶ* proditemp : lists) {
                    (ಶೂನ್ಯ)proditemp;
                    // "_Vjoin_nlists_eligible += _Vjoin_nleft_0 ? 1 : 0;"
                    ifp->ಕೂಡಿಸು_Thensp(ಹೊಸ Ast_ನಿಯೋಜಿಸು{
                        fl, ಹೊಸ AstVarRef{fl, eligibleVarp, VAccess::WRITE},
                        ಹೊಸ Ast_ಕೂಡಿಸು{
                            fl, ಹೊಸ AstVarRef{fl, eligibleVarp, VAccess::READ},
                            ಹೊಸ AstCond{
                                fl,
                                ಹೊಸ AstNeq{fl, ಹೊಸ AstConst{fl, AstConst::Widthed_ಮೌಲ್ಯ{}, 32, 0},
                                           ಹೊಸ AstVarRef{fl, nleftVarps[i], VAccess::READ}},
                                ಹೊಸ AstConst{fl, AstConst::Widthed_ಮೌಲ್ಯ{}, 32, 1},
                                ಹೊಸ AstConst{fl, AstConst::Widthed_ಮೌಲ್ಯ{}, 32, 0}}}});
                    ++i;
                }

                // "if (!_Vjoin_nlists_eligible) break;"  // Out of elements
                ifp->ಕೂಡಿಸು_Thensp(
                    ಹೊಸ AstIf{fl,
                              ಹೊಸ AstEq{fl, ಹೊಸ AstConst{fl, AstConst::Widthed_ಮೌಲ್ಯ{}, 32, 0},
                                        ಹೊಸ AstVarRef{fl, eligibleVarp, VAccess::READ}},
                              ಹೊಸ AstJumpGo{fl, m_jumpBlockp}});

                // "// +1 to simplify usage, so 0 = done"
                // "_Vjoin_picked_elist = 1 + (std::rand() % _Vjoin_nlists_eligible);"
                ifp->ಕೂಡಿಸು_Thensp(ಹೊಸ Ast_ನಿಯೋಜಿಸು{
                    fl, ಹೊಸ AstVarRef{fl, pickedVarp, VAccess::WRITE},
                    ಹೊಸ Ast_ಕೂಡಿಸು{fl, ಹೊಸ AstConst{fl, AstConst::Widthed_ಮೌಲ್ಯ{}, 32, 1},
                               ಹೊಸ AstModDiv{fl, ಹೊಸ AstRand{fl, AstRand::SRandomU32{}},
                                             ಹೊಸ AstVarRef{fl, eligibleVarp, VAccess::READ}}}});

                // "_Vjoin_repick = false;
                ifp->ಕೂಡಿಸು_Thensp(ಹೊಸ Ast_ನಿಯೋಜಿಸು{fl, ಹೊಸ AstVarRef{fl, repickVarp, VAccess::WRITE},
                                             ಹೊಸ AstConst{fl, AstConst::ಬಿಟ್_False{}}});
            }

            // "// Number of eligible lists left before hit _Vjoin_picked one"
            // "int _Vjoin_sel_elist = _Vjoin_picked_elist;"
            AstVar* ಸ್ಥಿರಾಂಕ selVarp
                = ಹೊಸ AstVar{fl, VVarType::VAR, "_Vjoin_sel_elist", VFlag_ಬಿಟ್_Packed{}, 32};
            selVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            selVarp->funcLocal(ನಿಜ);
            m_breakVarp->ಕೂಡಿಸು_ಮುಂದಿನ_ಇಲ್ಲಿ(selVarp);  // Add to function top, not newp
            whilep->ಕೂಡಿಸು_Stmtsp(ಹೊಸ Ast_ನಿಯೋಜಿಸು{fl, ಹೊಸ AstVarRef{fl, selVarp, VAccess::WRITE},
                                            ಹೊಸ AstVarRef{fl, pickedVarp, VAccess::READ}});

            i = -1;
            ಗಾಗಿ (Ast_RSProd_ಅಂಶ* proditemp : lists) {
                ++i;
                // "if (_Vjoin_nleft_0) {"
                ifp = ಹೊಸ AstIf{fl,
                                ಹೊಸ AstNeq{fl, ಹೊಸ AstConst{fl, AstConst::Widthed_ಮೌಲ್ಯ{}, 32, 0},
                                           ಹೊಸ AstVarRef{fl, nleftVarps[i], VAccess::READ}},
                                ಶೂನ್ಯಸೂಚಕ, ಶೂನ್ಯಸೂಚಕ};
                whilep->ಕೂಡಿಸು_Stmtsp(ifp);
                {
                    // "--_Vjoin_sel_elist;"
                    ifp->ಕೂಡಿಸು_Thensp(ಹೊಸ Ast_ನಿಯೋಜಿಸು{
                        fl, ಹೊಸ AstVarRef{fl, selVarp, VAccess::WRITE},
                        ಹೊಸ AstSub{fl, ಹೊಸ AstVarRef{fl, selVarp, VAccess::READ},
                                   ಹೊಸ AstConst{fl, AstConst::Widthed_ಮೌಲ್ಯ{}, 32, 1}}});

                    // "if (_Vjoin_sel_elist == 0) {"
                    AstIf* ಸ್ಥಿರಾಂಕ jIfp = ಹೊಸ AstIf{
                        fl,
                        ಹೊಸ AstEq{fl, ಹೊಸ AstConst{fl, AstConst::Widthed_ಮೌಲ್ಯ{}, 32, 0},
                                  ಹೊಸ AstVarRef{fl, selVarp, VAccess::READ}},
                        ಶೂನ್ಯಸೂಚಕ, ಶೂನ್ಯಸೂಚಕ};
                    ifp->ಕೂಡಿಸು_Thensp(jIfp);
                    {
                        // "switch (_Vjoin_nleft_0) {"
                        //     "case 2 / * N(a)  * /: {statement}; break;"
                        //     "case 1 / * N(a) - 1 * /: {statement}; break;"
                        uint32_t j = static_cast<uint32_t>(listStmts[proditemp].ಗಾತ್ರ_2());
                        ಗಾಗಿ (Ast_ಗಂಟು_Stmt* iprodp : listStmts[proditemp]) {
                            jIfp->ಕೂಡಿಸು_Thensp(ಹೊಸ AstIf{
                                fl,
                                ಹೊಸ AstEq{fl, ಹೊಸ AstConst{fl, AstConst::Widthed_ಮೌಲ್ಯ{}, 32, j},
                                          ಹೊಸ AstVarRef{fl, nleftVarps[i], VAccess::READ}},
                                iprodp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು)});
                            --j;
                        }

                        // "--_Vjoin_nleft_0;"
                        jIfp->ಕೂಡಿಸು_Thensp(ಹೊಸ Ast_ನಿಯೋಜಿಸು{
                            fl, ಹೊಸ AstVarRef{fl, nleftVarps[i], VAccess::WRITE},
                            ಹೊಸ AstSub{fl, ಹೊಸ AstVarRef{fl, nleftVarps[i], VAccess::READ},
                                       ಹೊಸ AstConst{fl, AstConst::Widthed_ಮೌಲ್ಯ{}, 32, 1}}});

                        // "if (0 == _Vjoin_nleft_0) _Vjoin_repick = true;"
                        jIfp->ಕೂಡಿಸು_Thensp(ಹೊಸ AstIf{
                            fl,
                            ಹೊಸ AstEq{fl, ಹೊಸ AstConst{fl, AstConst::Widthed_ಮೌಲ್ಯ{}, 32, 0},
                                      ಹೊಸ AstVarRef{fl, nleftVarps[i], VAccess::READ}},
                            ಹೊಸ Ast_ನಿಯೋಜಿಸು{fl, ಹೊಸ AstVarRef{fl, repickVarp, VAccess::WRITE},
                                          ಹೊಸ AstConst{fl, AstConst::ಬಿಟ್_True{}}}});

                        // "continue;"
                        jIfp->ಕೂಡಿಸು_Thensp(ಹೊಸ AstJumpGo{fl, whilep});
                    }
                }
            }
        }

        UINFOTREE(9, newp, "ForkJoin new", "-");
        ಹಿಂತಿರುಗಿ newp;
    }

    // VISITORS

    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildren(nodep);
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstRandSequence* nodep) override {
        UINFO(9, "visit " << nodep);
        UASSERT_OBJ(m_modp, nodep, "randsequence not under module");
        ಆದರೆ (m_rsp) {
            // No examples found in the wild
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: randsequence under randsequence");
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlink_Fr_ಹಿಂಭಾಗ()), nodep);
            ಹಿಂತಿರುಗಿ;
        }

        UINFOTREE(9, nodep, "rsIn", "");
        VL_RESTORER(m_rsp);
        m_rsp = nodep;
        VL_RESTORER(m_ಆರಂಭ_Prodp);

        VL_RESTORER_COPY(m_localizes);
        VL_RESTORER_COPY(m_localize_ಹೆಸರುಗಳು);
        VL_RESTORER_COPY(m_localizeRemaps);
        ಕಂಡುಹಿಡಿ_Localizes(nodep);

        // Find first production
        m_ಆರಂಭ_Prodp = nodep->prodsp();
        ಆದರೆ (nodep->prodp()) m_ಆರಂಭ_Prodp = nodep->prodp();  // Has a start production selected

        // Build production functions
        ಗಾಗಿ (AstRSProd* prodp = nodep->prodsp(); prodp; prodp = VN_AS(prodp->nextp(), RSProd)) {
            newProdFunc(prodp);
            // Don't iterate yet, need to have tasks we can reference
        }

        // Iterate all of the new productions, moving guts into functions just made
        // Must do those with 'rand join' first, as they need to see the raw AstRS* nodes
        // before they are removed.  (Alternative would be to keep them until the end).
        UINFOTREE(9, nodep, "RS Tree pre-it", "-");
        std::unordered_ಗಣ<AstRSProd*> prodHasRandJoin;
        ಗಾಗಿ (AstRSProd* prodp = nodep->prodsp(); prodp; prodp = VN_AS(prodp->nextp(), RSProd)) {
            prodp->foreach([&](ಸ್ಥಿರಾಂಕ AstRSProdList* ಸ್ಥಿರಾಂಕ prodlistp) {
                ಆದರೆ (prodlistp->randJoin()) prodHasRandJoin.emplace(prodp);
            });
        }
        ಗಾಗಿ (AstRSProd *nextp, *prodp = nodep->prodsp(); prodp; prodp = nextp) {
            nextp = VN_AS(prodp->nextp(), RSProd);
            ಆದರೆ (prodHasRandJoin.ಎಣಿಕೆ_2(prodp)) VL_DO_DANGLING(iterate(prodp), prodp);
        }
        ಗಾಗಿ (AstRSProd *nextp, *prodp = nodep->prodsp(); prodp; prodp = nextp) {
            nextp = VN_AS(prodp->nextp(), RSProd);
            ಆದರೆ (!prodHasRandJoin.ಎಣಿಕೆ_2(prodp)) VL_DO_DANGLING(iterate(prodp), prodp);
        }
        UINFOTREE(9, nodep, "RS Tree post-it", "-");

        // Replace randsequence with call to randsequence start function
        AstTask* ಸ್ಥಿರಾಂಕ ಆರಂಭ_Funcp = new_ಆರಂಭ_Func(nodep);
        AstArg* argsp = ಶೂನ್ಯಸೂಚಕ;
        ಗಾಗಿ (ಸ್ವಯಂಚಾಲಿತ& itr : m_localize_ಹೆಸರುಗಳು) {
            AstVar* ಸ್ಥಿರಾಂಕ lvarp = itr.ಎರಡನೇ;
            argsp = Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ(
                argsp, ಹೊಸ AstArg{nodep->fileline(), "__Vrsarg_" + lvarp->name(),
                                  ಹೊಸ AstVarRef{nodep->fileline(), lvarp, VAccess::READWRITE}});
        }
        AstTaskRef* ಸ್ಥಿರಾಂಕ callStartp
            = ಹೊಸ AstTaskRef{nodep->fileline(), ಆರಂಭ_Funcp->name(), argsp};
        callStartp->taskp(ಆರಂಭ_Funcp);
        nodep->replaceWith(ಹೊಸ Ast_Stmt_ಅಭಿವ್ಯಕ್ತಿ{nodep->fileline(), callStartp});

        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstRSProd* nodep) override {
        // Production, immediately under a randsequence
        UINFO(9, "visit " << nodep);
        UINFOTREE(9, nodep, "visit " << nodep, "-");
        UASSERT_OBJ(!m_prodFuncp, nodep, "recursive production definition?");
        VL_RESTORER(m_prodFuncp);
        ಸ್ವಯಂಚಾಲಿತ it = m_prodFuncps.ಕಂಡುಹಿಡಿ(nodep);
        UASSERT_OBJ(it != m_prodFuncps.ಕೊನೆ(), nodep, "No production function made");
        m_prodFuncp = it->ಎರಡನೇ;

        // Create a break variable
        // TODO we could do this only if break exists in the downstream production,
        // but as-is we'll optimize it away in most cases anyways
        VL_RESTORER(m_breakVarp);
        VL_RESTORER_COPY(m_localizeRemaps);
        m_breakVarp = newBreakVar(nodep->fileline(), ನಿಜ);
        m_prodFuncp->ಕೂಡಿಸು_Stmtsp(m_breakVarp);

        // Production formal ports become real input ports on the generated task.
        // Must be added after m_breakVarp / localize ports so positional argument
        // order in newProdFuncRef stays consistent: [break, localize..., user...].
        ಆದರೆ (Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ portsp = nodep->portsp()) {
            portsp->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ();
            m_prodFuncp->ಕೂಡಿಸು_Stmtsp(portsp);
            ಗಾಗಿ (Ast_ಗಂಟು* itp = portsp; itp; itp = itp->nextp()) {
                AstVar* ಸ್ಥಿರಾಂಕ ಪೋರ್ಟ್_Varp = VN_AS(itp, Var);
                ಪೋರ್ಟ್_Varp->funcLocal(ನಿಜ);
                ಪೋರ್ಟ್_Varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            }
        }

        // Put JumpBlock immediately under the new function to support
        // a future break/return.  V3Const will rip it out if unneeded.
        VL_RESTORER(m_jumpBlockp);
        m_jumpBlockp = ಹೊಸ Ast_Jump_ಖಂಡ{nodep->fileline(), ಶೂನ್ಯಸೂಚಕ};
        m_prodFuncp->ಕೂಡಿಸು_Stmtsp(m_jumpBlockp);

        ಆದರೆ (nodep->fvarp())
            nodep->fvarp()->v3warn(E_UNSUPPORTED,
                                   "Unsupported: randsequence production function variable");

        // Move children into m_prodFuncp, and iterate there
        ಆದರೆ (!nodep->rulesp()) {  // Nothing to do
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->rulesp()->prodlistsp() && nodep->rulesp()->prodlistsp()->randJoin()) {
            AstRSProdList* ಸ್ಥಿರಾಂಕ prodlistp = nodep->rulesp()->prodlistsp();
            Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ itemsp = newProdRandJoin(nodep, prodlistp);
            m_jumpBlockp->ಕೂಡಿಸು_Stmtsp(itemsp);
            iterate_And_ಮುಂದಿನ_Null(itemsp);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (!nodep->rulesp()->nextp()) {  // Single rule/list, can just do it
            // RSPROD(RSRULE(weight, stmt)) -> IF(weight != 0, stmt)
            ಸ್ಥಿರಾಂಕ AstRSRule* ಸ್ಥಿರಾಂಕ rulep = nodep->rulesp();
            Ast_ಗಂಟು* itemsp = ಶೂನ್ಯಸೂಚಕ;
            ಆದರೆ (rulep->ತೂಕ_Stmtsp()) itemsp = rulep->ತೂಕ_Stmtsp()->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ();
            ಆದರೆ (rulep->prodlistsp())
                itemsp = Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ(itemsp, rulep->prodlistsp()->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ());
            // Can ignore rulep->weightp() unless is zero, in which case noop, add if
            ಆದರೆ (rulep->weightp())
                itemsp = ಹೊಸ AstIf{rulep->weightp()->fileline(),
                                   ಹೊಸ AstNeq{rulep->weightp()->fileline(),
                                              ಹೊಸ AstConst{rulep->weightp()->fileline(), 0},
                                              rulep->weightp()->unlink_Fr_ಹಿಂಭಾಗ()},
                                   itemsp};
            // Move prodlist to parent (m_prodFunc) and process
            ಆದರೆ (itemsp) {
                UINFOTREE(9, itemsp, "additems", "-");
                m_jumpBlockp->ಕೂಡಿಸು_Stmtsp(itemsp);
                iterate_And_ಮುಂದಿನ_Null(itemsp);
                UINFOTREE(9, m_jumpBlockp, "jumpBlockNow", "-");
            }
        } ಇಲ್ಲದಿದ್ದರೆ {
            // List of rules with "OR", need to random-weight them
            // RSPROD(RSRULE(weight1, stmt!), RSRULE(weight2, stmt2))
            //   -> RANDCASE(CASEITEM(weight1, stmt1), CASEITEM(weight2, stmt2))
            AstRandCase* ಸ್ಥಿರಾಂಕ randcasep = ಹೊಸ AstRandCase{nodep->fileline(), ಶೂನ್ಯಸೂಚಕ};
            ಗಾಗಿ (AstRSRule* rulep = nodep->rulesp(); rulep;
                 rulep = VN_AS(rulep->nextp(), RSRule)) {
                Ast_ಗಂಟು* itemsp = ಶೂನ್ಯಸೂಚಕ;
                ಆದರೆ (rulep->ತೂಕ_Stmtsp()) itemsp = rulep->ತೂಕ_Stmtsp()->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ();
                ಆದರೆ (rulep->prodlistsp())
                    itemsp = Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ(itemsp, rulep->prodlistsp()->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ());
                ಆದರೆ (itemsp) {
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ weightp = rulep->weightp()
                                                     ? rulep->weightp()->unlink_Fr_ಹಿಂಭಾಗ()
                                                     : ಹೊಸ AstConst{rulep->fileline(), 1};
                    randcasep->ಕೂಡಿಸು_Itemsp(ಹೊಸ Ast_Case_ಅಂಶ{rulep->fileline(), weightp, itemsp});
                }
            }
            m_jumpBlockp->ಕೂಡಿಸು_Stmtsp(randcasep);
            iterate_And_ಮುಂದಿನ_Null(randcasep);
        }

        // V3Task needs to know if we ended up making a recursive function
        m_prodFuncp->foreach([&](Ast_ಗಂಟು_FTask_Ref* ಸ್ಥಿರಾಂಕ nodep) {
            ಆದರೆ (nodep->taskp() == m_prodFuncp) m_prodFuncp->recursive(ನಿಜ);
        });
        UINFOTREE(9, m_prodFuncp, "rsprod-task-done " << nodep, "-");

        // Done with production, should have moved everything to new task
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlink_Fr_ಹಿಂಭಾಗ()), nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstRSRule* nodep) override {
        // Rule of what to produce (as a list) under a production
        nodep->v3fatalSrc("randsequence rule should be iterated and deleted by AstRSProd");
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstRSProdList* nodep) override {
        // List of productions to call, potentially with a weight
        UINFO(9, "visit " << nodep);
        UASSERT_OBJ(m_prodFuncp, nodep, "RSProdList not under production");

        // Move prodlist to parent (m_prodFunc) and process
        ಆದರೆ (Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ itemsp = nodep->prodsp()) {
            UASSERT_OBJ(!nodep->randJoin(), nodep,
                        "rand join should have been handled in visit(RSRule)");
            nodep->replaceWith(itemsp->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ());
            // Will soon iterate itemsp as part of normal visit process
            // These will become sequential in the generated task
        } ಇಲ್ಲದಿದ್ದರೆ {
            nodep->unlink_Fr_ಹಿಂಭಾಗ();
        }
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_RSProd_ಅಂಶ* nodep) override {
        // "call" to generate using another production
        UINFO(9, "visit " << nodep);
        UASSERT_OBJ(m_prodFuncp, nodep, "RSProdItem not under production");
        AstRSProd* ಸ್ಥಿರಾಂಕ foundp = nodep->prodp();
        UASSERT_OBJ(foundp, nodep, "Unlinked production reference");
        // Pass through caller-side argument list (IEEE 1800-2023 18.17.7).
        AstArg* ಸ್ಥಿರಾಂಕ userArgsp
            = nodep->argsp() ? nodep->argsp()->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ() : ಶೂನ್ಯಸೂಚಕ;
        // Convert to task call
        Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ newp = newProdFuncRef(nodep, foundp, m_breakVarp, userArgsp);
        // The production might have done a "break;", skip other steps if so
        newp->ಕೂಡಿಸು_ಮುಂದಿನ(ಹೊಸ AstIf{nodep->fileline(),
                                ಹೊಸ AstVarRef{nodep->fileline(), m_breakVarp, VAccess::READ},
                                ಹೊಸ AstJumpGo{nodep->fileline(), m_jumpBlockp}});

        nodep->replaceWith(newp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstRSBreak* nodep) override {
        UASSERT_OBJ(m_jumpBlockp, nodep, "RSBreak not under production jump block");
        UASSERT_OBJ(m_breakVarp, nodep, "no break variable");
        Ast_ಗಂಟು_Stmt* ಸ್ಥಿರಾಂಕ newp = ಹೊಸ Ast_ನಿಯೋಜಿಸು{
            nodep->fileline(), ಹೊಸ AstVarRef{nodep->fileline(), m_breakVarp, VAccess::WRITE},
            ಹೊಸ AstConst{nodep->fileline(), AstConst::ಬಿಟ್_True{}}};
        newp->ಕೂಡಿಸು_ಮುಂದಿನ(ಹೊಸ AstJumpGo{nodep->fileline(), m_jumpBlockp});
        nodep->replaceWith(newp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstRSReturn* nodep) override {
        UASSERT_OBJ(m_jumpBlockp, nodep, "RSReturn not under production jump block");
        nodep->replaceWith(ಹೊಸ AstJumpGo{nodep->fileline(), m_jumpBlockp});
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstVarRef* nodep) override {
        // Change references to variables inside randsequence to references to
        // ref variables on the new task
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ it = m_localizeRemaps.ಕಂಡುಹಿಡಿ(nodep->varp());
        ಆದರೆ (it != m_localizeRemaps.ಕೊನೆ()) {
            AstVar* ಸ್ಥಿರಾಂಕ iovarp = it->ಎರಡನೇ;
            UINFO(9, "VARREF remap " << nodep << " -> " << iovarp);
            nodep->varp(iovarp);
        }
    }

    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    ಸ್ಪಷ್ಟ RandSequenceVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~RandSequenceVisitor() override = ಪೂರ್ವನಿಯೋಜಿತ;
};

//######################################################################
// RandSequence method class functions

ಶೂನ್ಯ V3RandSequence::randSequenceNetlist(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { ಸ್ಥಿರಾಂಕ RandSequenceVisitor randSequenceVisitor{nodep}; }
    V3Global::dump_Check_Global_ವೃಕ್ಷ("randsequence", 0, dump_ವೃಕ್ಷ_Either_Level() >= 3);
}
