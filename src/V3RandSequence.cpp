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

#ਅਨ੍ਤਰ੍ਭਾਵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ਅਨ੍ਤਰ੍ਭਾਵ "verilatedos.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3RandSequence.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Ast.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Error.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3FileLine.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Global.h"

VL_DEFINE_DEBUG_FUNCTIONS;

// ######################################################################
//  Matcher classes (for suggestion matching)

ਵਰਗ RSProdMatcher final : ਜਨਤਕ VNodeMatcher {
ਜਨਤਕ:
    ਤਾਰਕਿਕ ਗੰਢ_ਮੇਲ(ਅਚਲ Ast_ਗੰਢ* nodep) ਅਚਲ override { ਵਾਪਸ VN_IS(nodep, RSProd); }
};

//######################################################################
// Visitor that marks classes needing a randomize() method

ਵਰਗ RandSequenceVisitor final : ਜਨਤਕ VNVisitor {
    // STATE - global
    std::map<AstRSProd*, Ast_ਗੰਢ_FTask*> m_prodFuncps;  // Generated production functions

    // STATE - for current visit position (use VL_RESTORER)
    Ast_ਗੰਢ_Module* m_modp = ਖਾਲੀ_ਸੂਚਕ;  // Current module
    AstRandSequence* m_rsp = ਖਾਲੀ_ਸੂਚਕ;  // Current randsequence
    AstRSProd* m_ਸ਼ੁਰੂ_Prodp = ਖਾਲੀ_ਸੂਚਕ;  // Starting production
    Ast_ਗੰਢ_FTask* m_prodFuncp = ਖਾਲੀ_ਸੂਚਕ;  // Production function being built
    Ast_Jump_ਖੰਡ* m_jumpBlockp = ਖਾਲੀ_ਸੂਚਕ;  // Building jumpblock for return/break
    AstVar* m_breakVarp = ਖਾਲੀ_ਸੂਚਕ;  // Break variable
    std::unordered_ਸੈੱਟ<AstVar*> m_localizes;  // Variables to become function inouts
    std::map<std::string, AstVar*> m_localize_ਨਾਂ;  // Ordered names of function inouts
    std::unordered_map<ਅਚਲ AstVar*, AstVar*> m_localizeRemaps;  // New ports for old vars

    // METHODS
    ਖਾਲੀ ਲੱਭੋ_Localizes(AstRandSequence* nodep) {
        std::ਸੈੱਟ<AstVar*> localVars;
        nodep->foreach([&](Ast_ਗੰਢ* ਅਚਲ nodep) {
            ਜੇ (ਅਚਲ AstVarRef* ਅਚਲ anodep = VN_CAST(nodep, VarRef)) {
                m_localizes.emplace(anodep->varp());
            } ਨਹੀਂਤਾਂ ਜੇ (AstVar* ਅਚਲ anodep = VN_CAST(nodep, Var)) {
                localVars.emplace(anodep);
            }
        });
        // Clear every variable allocated in the RandSequence
        ਲਈ (AstVar* varp : localVars) m_localizes.erase(varp);
        // Sort by name, so function arguments are stable
        ਲਈ (AstVar* varp : m_localizes) {
            UINFO(9, "localize " << varp);
            m_localize_ਨਾਂ.emplace(varp->name(), varp);
        }
    }

    AstVar* newBreakVar(File_ਸਤਰ* fl, ਤਾਰਕਿਕ asOutput) {
        AstVar* ਅਚਲ varp = ਨਵਾਂ AstVar{fl, VVarType::ਪੋਰਟ, "__VrsBreak", VFlag_ਦੋਆਧਾਰੀ_ਅੰਕ_Packed{}, 1};
        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        varp->funcLocal(ਸੱਚ);
        ਜੇ (asOutput) varp->direction(VDirection::OUTPUT);
        Ast_ਗੰਢ* outp = varp;
        outp->ਜੋੜੋ_ਅਗਲਾ(ਨਵਾਂ Ast_ਸੌਂਪੋ{fl, ਨਵਾਂ AstVarRef{fl, varp, VAccess::WRITE},
                                    ਨਵਾਂ AstConst{fl, AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_False{}}});

        // Also add arguments as next's
        ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& itr : m_localize_ਨਾਂ) {
            ਅਚਲ AstVar* ਅਚਲ lvarp = itr.ਦੂਜਾ;
            AstVar* ਅਚਲ iovarp
                = ਨਵਾਂ AstVar{fl, VVarType::ਪੋਰਟ, "__Vrsarg_" + lvarp->name(), lvarp};
            iovarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            iovarp->funcLocal(ਸੱਚ);
            iovarp->direction(VDirection::REF);
            varp->ਜੋੜੋ_ਅਗਲਾ(iovarp);
            m_localizeRemaps.emplace(lvarp, iovarp);
        }
        ਵਾਪਸ varp;
    }

    AstTask* new_ਸ਼ੁਰੂ_Func(AstRandSequence* nodep) {
        // Create wrapper that is called by the sequence
        AstTask* ਅਚਲ taskp = ਨਵਾਂ AstTask{m_ਸ਼ੁਰੂ_Prodp->fileline(), m_rsp->name(), ਖਾਲੀ_ਸੂਚਕ};
        taskp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        m_modp->ਜੋੜੋ_Stmtsp(taskp);

        // Create local (not output) break variable
        VL_RESTORER_COPY(m_localizeRemaps);
        AstVar* breakVarp = newBreakVar(nodep->fileline(), ਝੂਠ);
        taskp->ਜੋੜੋ_Stmtsp(breakVarp);

        // Call the start production's task
        taskp->ਜੋੜੋ_Stmtsp(newProdFuncRef(nodep, m_ਸ਼ੁਰੂ_Prodp, breakVarp, ਖਾਲੀ_ਸੂਚਕ));

        UINFOTREE(9, taskp, "newStart", "");
        ਵਾਪਸ taskp;
    }

    Ast_ਗੰਢ* newProdFuncRef(Ast_ਗੰਢ* nodep, AstRSProd* prodp, AstVar* breakVarp,
                            AstArg* userArgsp) {
        ਸਵੈਚਾਲਿਤ it = m_prodFuncps.ਲੱਭੋ(prodp);
        UASSERT_OBJ(it != m_prodFuncps.ਸਿਰਾ(), nodep, "No production function made");
        Ast_ਗੰਢ_FTask* ਅਚਲ prodFuncp = it->ਦੂਜਾ;
        File_ਸਤਰ* ਅਚਲ fl = nodep->fileline();
        // V3Width already ran before V3RandSequence, so VarRefs we create here
        // need dtype set explicitly, V3Broken later checks width == widthMin.
        AstVarRef* ਅਚਲ breakRefp = ਨਵਾਂ AstVarRef{fl, breakVarp, VAccess::WRITE};
        breakRefp->dtypeFrom(breakVarp);
        AstArg* ਅਚਲ argsp = ਨਵਾਂ AstArg{fl, breakVarp->name(), breakRefp};
        ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& itr : m_localize_ਨਾਂ) {
            ਅਚਲ AstVar* ਅਚਲ lvarp = itr.ਦੂਜਾ;
            AstVar* ਅਚਲ iovarp = m_localizeRemaps[lvarp];
            UASSERT_OBJ(iovarp, nodep, "No new port variable for local variable" << lvarp);
            AstVarRef* ਅਚਲ refp = ਨਵਾਂ AstVarRef{fl, iovarp, VAccess::READWRITE};
            refp->dtypeFrom(iovarp);
            argsp->ਜੋੜੋ_ਅਗਲਾ(ਨਵਾਂ AstArg{nodep->fileline(), "__Vrsarg_" + lvarp->name(), refp});
        }
        ਜੇ (userArgsp) argsp->ਜੋੜੋ_ਅਗਲਾ(userArgsp);
        Ast_ਗੰਢ* ਅਚਲ newp
            = ਨਵਾਂ Ast_Stmt_ਵਿਅੰਜਕ{fl, ਨਵਾਂ AstTaskRef{fl, VN_AS(prodFuncp, Task), argsp}};
        ਵਾਪਸ newp;
    }

    ਖਾਲੀ newProdFunc(AstRSProd* nodep) {
        // Task, as time may pass
        AstTask* ਅਚਲ newp
            = ਨਵਾਂ AstTask{nodep->fileline(), m_rsp->name() + "_" + nodep->name(), ਖਾਲੀ_ਸੂਚਕ};
        m_modp->ਜੋੜੋ_Stmtsp(newp);
        m_prodFuncps.emplace(nodep, newp);
        UINFOTREE(9, newp, "newProd", "");
    }

    Ast_ਗੰਢ* newProdRandJoin(AstRSProd* /*prodp*/, AstRSProdList* prodlistp) {
        // For weight == 1.0 longer sequence (favor stay in a)
        // For weight == 0.0 shorter squence (favor change a/b)
        UASSERT_OBJ(prodlistp->weightp(), prodlistp, "Weight should have default CONST(0.5)");
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* weightp = prodlistp->weightp()->unlink_Fr_ਪਿੱਠ();

        // Build map of production lists and productions we need to join
        // Must mainain the relative order of each sequence.
        std::deque<Ast_RSProd_ਵਸਤੂ*> lists;
        std::unordered_map<Ast_RSProd_ਵਸਤੂ*, std::deque<Ast_ਗੰਢ_Stmt*>> listStmts;
        ਲਈ (Ast_RSProd_ਵਸਤੂ* proditemp = VN_AS(prodlistp->prodsp(), RSProd_ਵਸਤੂ); proditemp;
             proditemp = VN_AS(proditemp->nextp(), RSProd_ਵਸਤੂ)) {
            lists.ਧੱਕੋ_ਪਿੱਠ(proditemp);
            ਅਚਲ AstRSProd* ਅਚਲ subProdp = proditemp->prodp();
            ਜੇ (!subProdp) ਜਾਰੀ;
            ਜੇ (!subProdp->rulesp()) ਜਾਰੀ;
            ਜੇ (!subProdp->rulesp()->prodlistsp()) ਜਾਰੀ;
            ਲਈ (Ast_ਗੰਢ_Stmt* subStmtp
                 = VN_AS(subProdp->rulesp()->prodlistsp()->prodsp(), ਗੰਢ_Stmt);
                 subStmtp; subStmtp = VN_AS(subStmtp->nextp(), ਗੰਢ_Stmt)) {
                listStmts[proditemp].ਧੱਕੋ_ਪਿੱਠ(subStmtp);
            }
        }

        UINFO(9, "RandJoin productions called:");
        ਲਈ (Ast_RSProd_ਵਸਤੂ* proditemp : lists) {
            UINFO(9, "  list " << proditemp);
            ਲਈ (ਅਚਲ Ast_ਗੰਢ_Stmt* prodip : listStmts[proditemp])
                UINFO(9, "    calls " << prodip);
        }

        // Need to clone all nodes used

        File_ਸਤਰ* fl = prodlistp->fileline();
        Ast_ਗੰਢ* newp = ਖਾਲੀ_ਸੂਚਕ;

        // "bool _Vjoin_repick = true;"  // Pick a new longer sequence
        AstVar* repickVarp = ਨਵਾਂ AstVar{fl, VVarType::VAR, "_Vjoin_repick", VFlag_ਦੋਆਧਾਰੀ_ਅੰਕ_Packed{}, 1};
        repickVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        repickVarp->funcLocal(ਸੱਚ);
        m_breakVarp->ਜੋੜੋ_ਅਗਲਾ_ਇੱਥੇ(repickVarp);  // Add to function top, not newp
        newp = Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ(newp,
                                ਨਵਾਂ Ast_ਸੌਂਪੋ{fl, ਨਵਾਂ AstVarRef{fl, repickVarp, VAccess::WRITE},
                                              ਨਵਾਂ AstConst{fl, AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_True{}}});

        // "int _Vjoin_nleft_0 = N(items);"  // N(a) Number left to generate in list 0, starts at
        // N(list0)
        std::deque<AstVar*> nleftVarps;
        ਪੂਰਨ i = 0;
        ਲਈ (Ast_RSProd_ਵਸਤੂ* proditemp : lists) {
            // 'rand join arule arule arule' is legal, so need numbered, not non-unique named
            // nleft's
            AstVar* ਅਚਲ varp = ਨਵਾਂ AstVar{
                fl, VVarType::VAR, "_Vjoin_nleft_" + std::ਮੰਜ਼ਿਲ_string(i++), VFlag_ਦੋਆਧਾਰੀ_ਅੰਕ_Packed{}, 32};
            varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            varp->funcLocal(ਸੱਚ);
            m_breakVarp->ਜੋੜੋ_ਅਗਲਾ_ਇੱਥੇ(varp);  // Add to function top, not newp
            nleftVarps.ਧੱਕੋ_ਪਿੱਠ(varp);

            newp = Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ(
                newp,
                ਨਵਾਂ Ast_ਸੌਂਪੋ{fl, ਨਵਾਂ AstVarRef{fl, varp, VAccess::WRITE},
                              ਨਵਾਂ AstConst{fl, AstConst::Widthed_ਮੁੱਲ{}, 32,
                                           static_cast<uint32_t>(listStmts[proditemp].ਮਾਪ())}});
        }

        // "int _Vjoin_picked_elist = 0;"  // 0 = pick first eligible (non-finished) list, 1 = pick
        // second, etc
        AstVar* ਅਚਲ pickedVarp
            = ਨਵਾਂ AstVar{fl, VVarType::VAR, "_Vjoin_picked_elist", VFlag_ਦੋਆਧਾਰੀ_ਅੰਕ_Packed{}, 1};
        pickedVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        pickedVarp->funcLocal(ਸੱਚ);
        m_breakVarp->ਜੋੜੋ_ਅਗਲਾ_ਇੱਥੇ(pickedVarp);  // Add to function top, not newp

        // "while(1) {"  // Generate until make complete sequence
        Ast_Jump_ਖੰਡ* ਅਚਲ whilep = ਨਵਾਂ Ast_Jump_ਖੰਡ{fl, ਖਾਲੀ_ਸੂਚਕ};
        newp = Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ(newp, ਨਵਾਂ AstLoop{fl, whilep});
        {
            // "if ((std::rand() & 0xffffUL) > (0xffff * weight)) _Vjoin_repick = true;"
            whilep->ਜੋੜੋ_Stmtsp(ਨਵਾਂ AstIf{
                fl,
                ਨਵਾਂ AstGt{
                    fl, ਨਵਾਂ AstRand{fl, AstRand::SRandomU32{}},
                    ਨਵਾਂ AstRToIS{fl, ਨਵਾਂ AstMulD{fl,
                                                 ਨਵਾਂ AstConst{fl, AstConst::RealDouble{},
                                                              static_cast<ਦੁੱਗਣਾ>(0xffffffffUL)},
                                                 weightp}}},
                ਨਵਾਂ Ast_ਸੌਂਪੋ{fl, ਨਵਾਂ AstVarRef{fl, repickVarp, VAccess::WRITE},
                              ਨਵਾਂ AstConst{fl, AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_True{}}}});

            // "if (_Vjoin_repick) {"
            AstIf* ifp = ਨਵਾਂ AstIf{fl, ਨਵਾਂ AstVarRef{fl, repickVarp, VAccess::READ}};
            whilep->ਜੋੜੋ_Stmtsp(ifp);
            {
                // "// _Vjoin_nlists_eligible is how many lists eligable for picking"
                // "int _Vjoin_nlists_eligible = 0;"
                AstVar* ਅਚਲ eligibleVarp = ਨਵਾਂ AstVar{
                    fl, VVarType::VAR, "_Vjoin_nlists_eligible", VFlag_ਦੋਆਧਾਰੀ_ਅੰਕ_Packed{}, 32};
                eligibleVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                eligibleVarp->funcLocal(ਸੱਚ);
                m_breakVarp->ਜੋੜੋ_ਅਗਲਾ_ਇੱਥੇ(eligibleVarp);  // Add to function top, not newp
                ifp->ਜੋੜੋ_Thensp(ਨਵਾਂ Ast_ਸੌਂਪੋ{fl, ਨਵਾਂ AstVarRef{fl, eligibleVarp, VAccess::WRITE},
                                             ਨਵਾਂ AstConst{fl, AstConst::Widthed_ਮੁੱਲ{}, 32, 0}});

                i = 0;
                ਲਈ (Ast_RSProd_ਵਸਤੂ* proditemp : lists) {
                    (ਖਾਲੀ)proditemp;
                    // "_Vjoin_nlists_eligible += _Vjoin_nleft_0 ? 1 : 0;"
                    ifp->ਜੋੜੋ_Thensp(ਨਵਾਂ Ast_ਸੌਂਪੋ{
                        fl, ਨਵਾਂ AstVarRef{fl, eligibleVarp, VAccess::WRITE},
                        ਨਵਾਂ Ast_ਜੋੜੋ{
                            fl, ਨਵਾਂ AstVarRef{fl, eligibleVarp, VAccess::READ},
                            ਨਵਾਂ AstCond{
                                fl,
                                ਨਵਾਂ AstNeq{fl, ਨਵਾਂ AstConst{fl, AstConst::Widthed_ਮੁੱਲ{}, 32, 0},
                                           ਨਵਾਂ AstVarRef{fl, nleftVarps[i], VAccess::READ}},
                                ਨਵਾਂ AstConst{fl, AstConst::Widthed_ਮੁੱਲ{}, 32, 1},
                                ਨਵਾਂ AstConst{fl, AstConst::Widthed_ਮੁੱਲ{}, 32, 0}}}});
                    ++i;
                }

                // "if (!_Vjoin_nlists_eligible) break;"  // Out of elements
                ifp->ਜੋੜੋ_Thensp(
                    ਨਵਾਂ AstIf{fl,
                              ਨਵਾਂ AstEq{fl, ਨਵਾਂ AstConst{fl, AstConst::Widthed_ਮੁੱਲ{}, 32, 0},
                                        ਨਵਾਂ AstVarRef{fl, eligibleVarp, VAccess::READ}},
                              ਨਵਾਂ AstJumpGo{fl, m_jumpBlockp}});

                // "// +1 to simplify usage, so 0 = done"
                // "_Vjoin_picked_elist = 1 + (std::rand() % _Vjoin_nlists_eligible);"
                ifp->ਜੋੜੋ_Thensp(ਨਵਾਂ Ast_ਸੌਂਪੋ{
                    fl, ਨਵਾਂ AstVarRef{fl, pickedVarp, VAccess::WRITE},
                    ਨਵਾਂ Ast_ਜੋੜੋ{fl, ਨਵਾਂ AstConst{fl, AstConst::Widthed_ਮੁੱਲ{}, 32, 1},
                               ਨਵਾਂ AstModDiv{fl, ਨਵਾਂ AstRand{fl, AstRand::SRandomU32{}},
                                             ਨਵਾਂ AstVarRef{fl, eligibleVarp, VAccess::READ}}}});

                // "_Vjoin_repick = false;
                ifp->ਜੋੜੋ_Thensp(ਨਵਾਂ Ast_ਸੌਂਪੋ{fl, ਨਵਾਂ AstVarRef{fl, repickVarp, VAccess::WRITE},
                                             ਨਵਾਂ AstConst{fl, AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_False{}}});
            }

            // "// Number of eligible lists left before hit _Vjoin_picked one"
            // "int _Vjoin_sel_elist = _Vjoin_picked_elist;"
            AstVar* ਅਚਲ selVarp
                = ਨਵਾਂ AstVar{fl, VVarType::VAR, "_Vjoin_sel_elist", VFlag_ਦੋਆਧਾਰੀ_ਅੰਕ_Packed{}, 32};
            selVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            selVarp->funcLocal(ਸੱਚ);
            m_breakVarp->ਜੋੜੋ_ਅਗਲਾ_ਇੱਥੇ(selVarp);  // Add to function top, not newp
            whilep->ਜੋੜੋ_Stmtsp(ਨਵਾਂ Ast_ਸੌਂਪੋ{fl, ਨਵਾਂ AstVarRef{fl, selVarp, VAccess::WRITE},
                                            ਨਵਾਂ AstVarRef{fl, pickedVarp, VAccess::READ}});

            i = -1;
            ਲਈ (Ast_RSProd_ਵਸਤੂ* proditemp : lists) {
                ++i;
                // "if (_Vjoin_nleft_0) {"
                ifp = ਨਵਾਂ AstIf{fl,
                                ਨਵਾਂ AstNeq{fl, ਨਵਾਂ AstConst{fl, AstConst::Widthed_ਮੁੱਲ{}, 32, 0},
                                           ਨਵਾਂ AstVarRef{fl, nleftVarps[i], VAccess::READ}},
                                ਖਾਲੀ_ਸੂਚਕ, ਖਾਲੀ_ਸੂਚਕ};
                whilep->ਜੋੜੋ_Stmtsp(ifp);
                {
                    // "--_Vjoin_sel_elist;"
                    ifp->ਜੋੜੋ_Thensp(ਨਵਾਂ Ast_ਸੌਂਪੋ{
                        fl, ਨਵਾਂ AstVarRef{fl, selVarp, VAccess::WRITE},
                        ਨਵਾਂ AstSub{fl, ਨਵਾਂ AstVarRef{fl, selVarp, VAccess::READ},
                                   ਨਵਾਂ AstConst{fl, AstConst::Widthed_ਮੁੱਲ{}, 32, 1}}});

                    // "if (_Vjoin_sel_elist == 0) {"
                    AstIf* ਅਚਲ jIfp = ਨਵਾਂ AstIf{
                        fl,
                        ਨਵਾਂ AstEq{fl, ਨਵਾਂ AstConst{fl, AstConst::Widthed_ਮੁੱਲ{}, 32, 0},
                                  ਨਵਾਂ AstVarRef{fl, selVarp, VAccess::READ}},
                        ਖਾਲੀ_ਸੂਚਕ, ਖਾਲੀ_ਸੂਚਕ};
                    ifp->ਜੋੜੋ_Thensp(jIfp);
                    {
                        // "switch (_Vjoin_nleft_0) {"
                        //     "case 2 / * N(a)  * /: {statement}; break;"
                        //     "case 1 / * N(a) - 1 * /: {statement}; break;"
                        uint32_t j = static_cast<uint32_t>(listStmts[proditemp].ਮਾਪ());
                        ਲਈ (Ast_ਗੰਢ_Stmt* iprodp : listStmts[proditemp]) {
                            jIfp->ਜੋੜੋ_Thensp(ਨਵਾਂ AstIf{
                                fl,
                                ਨਵਾਂ AstEq{fl, ਨਵਾਂ AstConst{fl, AstConst::Widthed_ਮੁੱਲ{}, 32, j},
                                          ਨਵਾਂ AstVarRef{fl, nleftVarps[i], VAccess::READ}},
                                iprodp->clone_ਰੁੱਖ(ਝੂਠ)});
                            --j;
                        }

                        // "--_Vjoin_nleft_0;"
                        jIfp->ਜੋੜੋ_Thensp(ਨਵਾਂ Ast_ਸੌਂਪੋ{
                            fl, ਨਵਾਂ AstVarRef{fl, nleftVarps[i], VAccess::WRITE},
                            ਨਵਾਂ AstSub{fl, ਨਵਾਂ AstVarRef{fl, nleftVarps[i], VAccess::READ},
                                       ਨਵਾਂ AstConst{fl, AstConst::Widthed_ਮੁੱਲ{}, 32, 1}}});

                        // "if (0 == _Vjoin_nleft_0) _Vjoin_repick = true;"
                        jIfp->ਜੋੜੋ_Thensp(ਨਵਾਂ AstIf{
                            fl,
                            ਨਵਾਂ AstEq{fl, ਨਵਾਂ AstConst{fl, AstConst::Widthed_ਮੁੱਲ{}, 32, 0},
                                      ਨਵਾਂ AstVarRef{fl, nleftVarps[i], VAccess::READ}},
                            ਨਵਾਂ Ast_ਸੌਂਪੋ{fl, ਨਵਾਂ AstVarRef{fl, repickVarp, VAccess::WRITE},
                                          ਨਵਾਂ AstConst{fl, AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_True{}}}});

                        // "continue;"
                        jIfp->ਜੋੜੋ_Thensp(ਨਵਾਂ AstJumpGo{fl, whilep});
                    }
                }
            }
        }

        UINFOTREE(9, newp, "ForkJoin new", "-");
        ਵਾਪਸ newp;
    }

    // VISITORS

    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildren(nodep);
    }

    ਖਾਲੀ ਦੇਖੋ(AstRandSequence* nodep) override {
        UINFO(9, "visit " << nodep);
        UASSERT_OBJ(m_modp, nodep, "randsequence not under module");
        ਜੇ (m_rsp) {
            // No examples found in the wild
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: randsequence under randsequence");
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep->unlink_Fr_ਪਿੱਠ()), nodep);
            ਵਾਪਸ;
        }

        UINFOTREE(9, nodep, "rsIn", "");
        VL_RESTORER(m_rsp);
        m_rsp = nodep;
        VL_RESTORER(m_ਸ਼ੁਰੂ_Prodp);

        VL_RESTORER_COPY(m_localizes);
        VL_RESTORER_COPY(m_localize_ਨਾਂ);
        VL_RESTORER_COPY(m_localizeRemaps);
        ਲੱਭੋ_Localizes(nodep);

        // Find first production
        m_ਸ਼ੁਰੂ_Prodp = nodep->prodsp();
        ਜੇ (nodep->prodp()) m_ਸ਼ੁਰੂ_Prodp = nodep->prodp();  // Has a start production selected

        // Build production functions
        ਲਈ (AstRSProd* prodp = nodep->prodsp(); prodp; prodp = VN_AS(prodp->nextp(), RSProd)) {
            newProdFunc(prodp);
            // Don't iterate yet, need to have tasks we can reference
        }

        // Iterate all of the new productions, moving guts into functions just made
        // Must do those with 'rand join' first, as they need to see the raw AstRS* nodes
        // before they are removed.  (Alternative would be to keep them until the end).
        UINFOTREE(9, nodep, "RS Tree pre-it", "-");
        std::unordered_ਸੈੱਟ<AstRSProd*> prodHasRandJoin;
        ਲਈ (AstRSProd* prodp = nodep->prodsp(); prodp; prodp = VN_AS(prodp->nextp(), RSProd)) {
            prodp->foreach([&](ਅਚਲ AstRSProdList* ਅਚਲ prodlistp) {
                ਜੇ (prodlistp->randJoin()) prodHasRandJoin.emplace(prodp);
            });
        }
        ਲਈ (AstRSProd *nextp, *prodp = nodep->prodsp(); prodp; prodp = nextp) {
            nextp = VN_AS(prodp->nextp(), RSProd);
            ਜੇ (prodHasRandJoin.ਗਿਣਤੀ_2(prodp)) VL_DO_DANGLING(iterate(prodp), prodp);
        }
        ਲਈ (AstRSProd *nextp, *prodp = nodep->prodsp(); prodp; prodp = nextp) {
            nextp = VN_AS(prodp->nextp(), RSProd);
            ਜੇ (!prodHasRandJoin.ਗਿਣਤੀ_2(prodp)) VL_DO_DANGLING(iterate(prodp), prodp);
        }
        UINFOTREE(9, nodep, "RS Tree post-it", "-");

        // Replace randsequence with call to randsequence start function
        AstTask* ਅਚਲ ਸ਼ੁਰੂ_Funcp = new_ਸ਼ੁਰੂ_Func(nodep);
        AstArg* argsp = ਖਾਲੀ_ਸੂਚਕ;
        ਲਈ (ਸਵੈਚਾਲਿਤ& itr : m_localize_ਨਾਂ) {
            AstVar* ਅਚਲ lvarp = itr.ਦੂਜਾ;
            argsp = Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ(
                argsp, ਨਵਾਂ AstArg{nodep->fileline(), "__Vrsarg_" + lvarp->name(),
                                  ਨਵਾਂ AstVarRef{nodep->fileline(), lvarp, VAccess::READWRITE}});
        }
        AstTaskRef* ਅਚਲ callStartp
            = ਨਵਾਂ AstTaskRef{nodep->fileline(), ਸ਼ੁਰੂ_Funcp->name(), argsp};
        callStartp->taskp(ਸ਼ੁਰੂ_Funcp);
        nodep->replaceWith(ਨਵਾਂ Ast_Stmt_ਵਿਅੰਜਕ{nodep->fileline(), callStartp});

        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstRSProd* nodep) override {
        // Production, immediately under a randsequence
        UINFO(9, "visit " << nodep);
        UINFOTREE(9, nodep, "visit " << nodep, "-");
        UASSERT_OBJ(!m_prodFuncp, nodep, "recursive production definition?");
        VL_RESTORER(m_prodFuncp);
        ਸਵੈਚਾਲਿਤ it = m_prodFuncps.ਲੱਭੋ(nodep);
        UASSERT_OBJ(it != m_prodFuncps.ਸਿਰਾ(), nodep, "No production function made");
        m_prodFuncp = it->ਦੂਜਾ;

        // Create a break variable
        // TODO we could do this only if break exists in the downstream production,
        // but as-is we'll optimize it away in most cases anyways
        VL_RESTORER(m_breakVarp);
        VL_RESTORER_COPY(m_localizeRemaps);
        m_breakVarp = newBreakVar(nodep->fileline(), ਸੱਚ);
        m_prodFuncp->ਜੋੜੋ_Stmtsp(m_breakVarp);

        // Production formal ports become real input ports on the generated task.
        // Must be added after m_breakVarp / localize ports so positional argument
        // order in newProdFuncRef stays consistent: [break, localize..., user...].
        ਜੇ (Ast_ਗੰਢ* ਅਚਲ portsp = nodep->portsp()) {
            portsp->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ();
            m_prodFuncp->ਜੋੜੋ_Stmtsp(portsp);
            ਲਈ (Ast_ਗੰਢ* itp = portsp; itp; itp = itp->nextp()) {
                AstVar* ਅਚਲ ਪੋਰਟ_Varp = VN_AS(itp, Var);
                ਪੋਰਟ_Varp->funcLocal(ਸੱਚ);
                ਪੋਰਟ_Varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            }
        }

        // Put JumpBlock immediately under the new function to support
        // a future break/return.  V3Const will rip it out if unneeded.
        VL_RESTORER(m_jumpBlockp);
        m_jumpBlockp = ਨਵਾਂ Ast_Jump_ਖੰਡ{nodep->fileline(), ਖਾਲੀ_ਸੂਚਕ};
        m_prodFuncp->ਜੋੜੋ_Stmtsp(m_jumpBlockp);

        ਜੇ (nodep->fvarp())
            nodep->fvarp()->v3warn(E_UNSUPPORTED,
                                   "Unsupported: randsequence production function variable");

        // Move children into m_prodFuncp, and iterate there
        ਜੇ (!nodep->rulesp()) {  // Nothing to do
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->rulesp()->prodlistsp() && nodep->rulesp()->prodlistsp()->randJoin()) {
            AstRSProdList* ਅਚਲ prodlistp = nodep->rulesp()->prodlistsp();
            Ast_ਗੰਢ* ਅਚਲ itemsp = newProdRandJoin(nodep, prodlistp);
            m_jumpBlockp->ਜੋੜੋ_Stmtsp(itemsp);
            iterate_And_ਅਗਲਾ_Null(itemsp);
        } ਨਹੀਂਤਾਂ ਜੇ (!nodep->rulesp()->nextp()) {  // Single rule/list, can just do it
            // RSPROD(RSRULE(weight, stmt)) -> IF(weight != 0, stmt)
            ਅਚਲ AstRSRule* ਅਚਲ rulep = nodep->rulesp();
            Ast_ਗੰਢ* itemsp = ਖਾਲੀ_ਸੂਚਕ;
            ਜੇ (rulep->ਭਾਰ_Stmtsp()) itemsp = rulep->ਭਾਰ_Stmtsp()->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ();
            ਜੇ (rulep->prodlistsp())
                itemsp = Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ(itemsp, rulep->prodlistsp()->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ());
            // Can ignore rulep->weightp() unless is zero, in which case noop, add if
            ਜੇ (rulep->weightp())
                itemsp = ਨਵਾਂ AstIf{rulep->weightp()->fileline(),
                                   ਨਵਾਂ AstNeq{rulep->weightp()->fileline(),
                                              ਨਵਾਂ AstConst{rulep->weightp()->fileline(), 0},
                                              rulep->weightp()->unlink_Fr_ਪਿੱਠ()},
                                   itemsp};
            // Move prodlist to parent (m_prodFunc) and process
            ਜੇ (itemsp) {
                UINFOTREE(9, itemsp, "additems", "-");
                m_jumpBlockp->ਜੋੜੋ_Stmtsp(itemsp);
                iterate_And_ਅਗਲਾ_Null(itemsp);
                UINFOTREE(9, m_jumpBlockp, "jumpBlockNow", "-");
            }
        } ਨਹੀਂਤਾਂ {
            // List of rules with "OR", need to random-weight them
            // RSPROD(RSRULE(weight1, stmt!), RSRULE(weight2, stmt2))
            //   -> RANDCASE(CASEITEM(weight1, stmt1), CASEITEM(weight2, stmt2))
            AstRandCase* ਅਚਲ randcasep = ਨਵਾਂ AstRandCase{nodep->fileline(), ਖਾਲੀ_ਸੂਚਕ};
            ਲਈ (AstRSRule* rulep = nodep->rulesp(); rulep;
                 rulep = VN_AS(rulep->nextp(), RSRule)) {
                Ast_ਗੰਢ* itemsp = ਖਾਲੀ_ਸੂਚਕ;
                ਜੇ (rulep->ਭਾਰ_Stmtsp()) itemsp = rulep->ਭਾਰ_Stmtsp()->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ();
                ਜੇ (rulep->prodlistsp())
                    itemsp = Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ(itemsp, rulep->prodlistsp()->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ());
                ਜੇ (itemsp) {
                    Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ weightp = rulep->weightp()
                                                     ? rulep->weightp()->unlink_Fr_ਪਿੱਠ()
                                                     : ਨਵਾਂ AstConst{rulep->fileline(), 1};
                    randcasep->ਜੋੜੋ_Itemsp(ਨਵਾਂ Ast_Case_ਵਸਤੂ{rulep->fileline(), weightp, itemsp});
                }
            }
            m_jumpBlockp->ਜੋੜੋ_Stmtsp(randcasep);
            iterate_And_ਅਗਲਾ_Null(randcasep);
        }

        // V3Task needs to know if we ended up making a recursive function
        m_prodFuncp->foreach([&](Ast_ਗੰਢ_FTask_Ref* ਅਚਲ nodep) {
            ਜੇ (nodep->taskp() == m_prodFuncp) m_prodFuncp->recursive(ਸੱਚ);
        });
        UINFOTREE(9, m_prodFuncp, "rsprod-task-done " << nodep, "-");

        // Done with production, should have moved everything to new task
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep->unlink_Fr_ਪਿੱਠ()), nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstRSRule* nodep) override {
        // Rule of what to produce (as a list) under a production
        nodep->v3fatalSrc("randsequence rule should be iterated and deleted by AstRSProd");
    }
    ਖਾਲੀ ਦੇਖੋ(AstRSProdList* nodep) override {
        // List of productions to call, potentially with a weight
        UINFO(9, "visit " << nodep);
        UASSERT_OBJ(m_prodFuncp, nodep, "RSProdList not under production");

        // Move prodlist to parent (m_prodFunc) and process
        ਜੇ (Ast_ਗੰਢ* ਅਚਲ itemsp = nodep->prodsp()) {
            UASSERT_OBJ(!nodep->randJoin(), nodep,
                        "rand join should have been handled in visit(RSRule)");
            nodep->replaceWith(itemsp->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ());
            // Will soon iterate itemsp as part of normal visit process
            // These will become sequential in the generated task
        } ਨਹੀਂਤਾਂ {
            nodep->unlink_Fr_ਪਿੱਠ();
        }
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_RSProd_ਵਸਤੂ* nodep) override {
        // "call" to generate using another production
        UINFO(9, "visit " << nodep);
        UASSERT_OBJ(m_prodFuncp, nodep, "RSProdItem not under production");
        AstRSProd* ਅਚਲ foundp = nodep->prodp();
        UASSERT_OBJ(foundp, nodep, "Unlinked production reference");
        // Pass through caller-side argument list (IEEE 1800-2023 18.17.7).
        AstArg* ਅਚਲ userArgsp
            = nodep->argsp() ? nodep->argsp()->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ() : ਖਾਲੀ_ਸੂਚਕ;
        // Convert to task call
        Ast_ਗੰਢ* ਅਚਲ newp = newProdFuncRef(nodep, foundp, m_breakVarp, userArgsp);
        // The production might have done a "break;", skip other steps if so
        newp->ਜੋੜੋ_ਅਗਲਾ(ਨਵਾਂ AstIf{nodep->fileline(),
                                ਨਵਾਂ AstVarRef{nodep->fileline(), m_breakVarp, VAccess::READ},
                                ਨਵਾਂ AstJumpGo{nodep->fileline(), m_jumpBlockp}});

        nodep->replaceWith(newp);
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
    }

    ਖਾਲੀ ਦੇਖੋ(AstRSBreak* nodep) override {
        UASSERT_OBJ(m_jumpBlockp, nodep, "RSBreak not under production jump block");
        UASSERT_OBJ(m_breakVarp, nodep, "no break variable");
        Ast_ਗੰਢ_Stmt* ਅਚਲ newp = ਨਵਾਂ Ast_ਸੌਂਪੋ{
            nodep->fileline(), ਨਵਾਂ AstVarRef{nodep->fileline(), m_breakVarp, VAccess::WRITE},
            ਨਵਾਂ AstConst{nodep->fileline(), AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_True{}}};
        newp->ਜੋੜੋ_ਅਗਲਾ(ਨਵਾਂ AstJumpGo{nodep->fileline(), m_jumpBlockp});
        nodep->replaceWith(newp);
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstRSReturn* nodep) override {
        UASSERT_OBJ(m_jumpBlockp, nodep, "RSReturn not under production jump block");
        nodep->replaceWith(ਨਵਾਂ AstJumpGo{nodep->fileline(), m_jumpBlockp});
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstVarRef* nodep) override {
        // Change references to variables inside randsequence to references to
        // ref variables on the new task
        ਅਚਲ ਸਵੈਚਾਲਿਤ it = m_localizeRemaps.ਲੱਭੋ(nodep->varp());
        ਜੇ (it != m_localizeRemaps.ਸਿਰਾ()) {
            AstVar* ਅਚਲ iovarp = it->ਦੂਜਾ;
            UINFO(9, "VARREF remap " << nodep << " -> " << iovarp);
            nodep->varp(iovarp);
        }
    }

    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override { iterateChildren(nodep); }

ਜਨਤਕ:
    // CONSTRUCTORS
    ਸਪਸ਼ਟ RandSequenceVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~RandSequenceVisitor() override = ਮੂਲ;
};

//######################################################################
// RandSequence method class functions

ਖਾਲੀ V3RandSequence::randSequenceNetlist(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { ਅਚਲ RandSequenceVisitor randSequenceVisitor{nodep}; }
    V3Global::dump_Check_Global_ਰੁੱਖ("randsequence", 0, dump_ਰੁੱਖ_Either_Level() >= 3);
}
