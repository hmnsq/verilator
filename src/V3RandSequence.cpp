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

वर्ग RSProdMatcher final : सार्वजनिक VNodeMatcher {
सार्वजनिक:
    तार्किक ग्रन्थिः_मेलनम्(ध्रुव Ast_ग्रन्थिः* nodep) ध्रुव override { प्रत्यागम VN_IS(nodep, RSProd); }
};

//######################################################################
// Visitor that marks classes needing a randomize() method

वर्ग RandSequenceVisitor final : सार्वजनिक VNVisitor {
    // STATE - global
    std::map<AstRSProd*, Ast_ग्रन्थिः_FTask*> m_prodFuncps;  // Generated production functions

    // STATE - for current visit position (use VL_RESTORER)
    Ast_ग्रन्थिः_Module* m_modp = शून्यसूचकः;  // Current module
    AstRandSequence* m_rsp = शून्यसूचकः;  // Current randsequence
    AstRSProd* m_startProdp = शून्यसूचकः;  // Starting production
    Ast_ग्रन्थिः_FTask* m_prodFuncp = शून्यसूचकः;  // Production function being built
    Ast_Jump_खण्डम्* m_jumpBlockp = शून्यसूचकः;  // Building jumpblock for return/break
    AstVar* m_breakVarp = शून्यसूचकः;  // Break variable
    std::unordered_समुच्चयः<AstVar*> m_localizes;  // Variables to become function inouts
    std::map<std::string, AstVar*> m_localize_नामानि;  // Ordered names of function inouts
    std::unordered_map<ध्रुव AstVar*, AstVar*> m_localizeRemaps;  // New ports for old vars

    // METHODS
    शून्य findLocalizes(AstRandSequence* nodep) {
        std::समुच्चयः<AstVar*> localVars;
        nodep->foreach([&](Ast_ग्रन्थिः* ध्रुव nodep) {
            यदि (ध्रुव AstVarRef* ध्रुव anodep = VN_CAST(nodep, VarRef)) {
                m_localizes.emplace(anodep->varp());
            } अन्यथा यदि (AstVar* ध्रुव anodep = VN_CAST(nodep, Var)) {
                localVars.emplace(anodep);
            }
        });
        // Clear every variable allocated in the RandSequence
        प्रति (AstVar* varp : localVars) m_localizes.erase(varp);
        // Sort by name, so function arguments are stable
        प्रति (AstVar* varp : m_localizes) {
            UINFO(9, "localize " << varp);
            m_localize_नामानि.emplace(varp->name(), varp);
        }
    }

    AstVar* newBreakVar(File_पङ्क्तिः* fl, तार्किक asOutput) {
        AstVar* ध्रुव varp = नूतन AstVar{fl, VVarType::पत्तनम्, "__VrsBreak", VFlagBitPacked{}, 1};
        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        varp->funcLocal(सत्य);
        यदि (asOutput) varp->direction(VDirection::OUTPUT);
        Ast_ग्रन्थिः* outp = varp;
        outp->सङ्कलय_अग्र(नूतन Ast_नियोजय{fl, नूतन AstVarRef{fl, varp, VAccess::WRITE},
                                    नूतन AstConst{fl, AstConst::BitFalse{}}});

        // Also add arguments as next's
        प्रति (ध्रुव स्वचालित& itr : m_localize_नामानि) {
            ध्रुव AstVar* ध्रुव lvarp = itr.second;
            AstVar* ध्रुव iovarp
                = नूतन AstVar{fl, VVarType::पत्तनम्, "__Vrsarg_" + lvarp->name(), lvarp};
            iovarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            iovarp->funcLocal(सत्य);
            iovarp->direction(VDirection::REF);
            varp->सङ्कलय_अग्र(iovarp);
            m_localizeRemaps.emplace(lvarp, iovarp);
        }
        प्रत्यागम varp;
    }

    AstTask* newStartFunc(AstRandSequence* nodep) {
        // Create wrapper that is called by the sequence
        AstTask* ध्रुव taskp = नूतन AstTask{m_startProdp->fileline(), m_rsp->name(), शून्यसूचकः};
        taskp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        m_modp->सङ्कलय_Stmtsp(taskp);

        // Create local (not output) break variable
        VL_RESTORER_COPY(m_localizeRemaps);
        AstVar* breakVarp = newBreakVar(nodep->fileline(), असत्य);
        taskp->सङ्कलय_Stmtsp(breakVarp);

        // Call the start production's task
        taskp->सङ्कलय_Stmtsp(newProdFuncRef(nodep, m_startProdp, breakVarp, शून्यसूचकः));

        UINFOTREE(9, taskp, "newStart", "");
        प्रत्यागम taskp;
    }

    Ast_ग्रन्थिः* newProdFuncRef(Ast_ग्रन्थिः* nodep, AstRSProd* prodp, AstVar* breakVarp,
                            AstArg* userArgsp) {
        स्वचालित it = m_prodFuncps.find(prodp);
        UASSERT_OBJ(it != m_prodFuncps.अन्तः_2(), nodep, "No production function made");
        Ast_ग्रन्थिः_FTask* ध्रुव prodFuncp = it->second;
        File_पङ्क्तिः* ध्रुव fl = nodep->fileline();
        // V3Width already ran before V3RandSequence, so VarRefs we create here
        // need dtype set explicitly, V3Broken later checks width == widthMin.
        AstVarRef* ध्रुव breakRefp = नूतन AstVarRef{fl, breakVarp, VAccess::WRITE};
        breakRefp->dtypeFrom(breakVarp);
        AstArg* ध्रुव argsp = नूतन AstArg{fl, breakVarp->name(), breakRefp};
        प्रति (ध्रुव स्वचालित& itr : m_localize_नामानि) {
            ध्रुव AstVar* ध्रुव lvarp = itr.second;
            AstVar* ध्रुव iovarp = m_localizeRemaps[lvarp];
            UASSERT_OBJ(iovarp, nodep, "No new port variable for local variable" << lvarp);
            AstVarRef* ध्रुव refp = नूतन AstVarRef{fl, iovarp, VAccess::READWRITE};
            refp->dtypeFrom(iovarp);
            argsp->सङ्कलय_अग्र(नूतन AstArg{nodep->fileline(), "__Vrsarg_" + lvarp->name(), refp});
        }
        यदि (userArgsp) argsp->सङ्कलय_अग्र(userArgsp);
        Ast_ग्रन्थिः* ध्रुव newp
            = नूतन Ast_Stmt_व्यञ्जकम्{fl, नूतन AstTaskRef{fl, VN_AS(prodFuncp, Task), argsp}};
        प्रत्यागम newp;
    }

    शून्य newProdFunc(AstRSProd* nodep) {
        // Task, as time may pass
        AstTask* ध्रुव newp
            = नूतन AstTask{nodep->fileline(), m_rsp->name() + "_" + nodep->name(), शून्यसूचकः};
        m_modp->सङ्कलय_Stmtsp(newp);
        m_prodFuncps.emplace(nodep, newp);
        UINFOTREE(9, newp, "newProd", "");
    }

    Ast_ग्रन्थिः* newProdRandJoin(AstRSProd* /*prodp*/, AstRSProdList* prodlistp) {
        // For weight == 1.0 longer sequence (favor stay in a)
        // For weight == 0.0 shorter squence (favor change a/b)
        UASSERT_OBJ(prodlistp->weightp(), prodlistp, "Weight should have default CONST(0.5)");
        Ast_ग्रन्थिः_व्यञ्जकम्* weightp = prodlistp->weightp()->unlink_Fr_पृष्ठभागः();

        // Build map of production lists and productions we need to join
        // Must mainain the relative order of each sequence.
        std::deque<AstRSProdItem*> lists;
        std::unordered_map<AstRSProdItem*, std::deque<Ast_ग्रन्थिः_Stmt*>> listStmts;
        प्रति (AstRSProdItem* proditemp = VN_AS(prodlistp->prodsp(), RSProdItem); proditemp;
             proditemp = VN_AS(proditemp->nextp(), RSProdItem)) {
            lists.प्रक्षिप_पृष्ठभागः(proditemp);
            ध्रुव AstRSProd* ध्रुव subProdp = proditemp->prodp();
            यदि (!subProdp) अनुवर्त;
            यदि (!subProdp->rulesp()) अनुवर्त;
            यदि (!subProdp->rulesp()->prodlistsp()) अनुवर्त;
            प्रति (Ast_ग्रन्थिः_Stmt* subStmtp
                 = VN_AS(subProdp->rulesp()->prodlistsp()->prodsp(), ग्रन्थिः_Stmt);
                 subStmtp; subStmtp = VN_AS(subStmtp->nextp(), ग्रन्थिः_Stmt)) {
                listStmts[proditemp].प्रक्षिप_पृष्ठभागः(subStmtp);
            }
        }

        UINFO(9, "RandJoin productions called:");
        प्रति (AstRSProdItem* proditemp : lists) {
            UINFO(9, "  list " << proditemp);
            प्रति (ध्रुव Ast_ग्रन्थिः_Stmt* prodip : listStmts[proditemp])
                UINFO(9, "    calls " << prodip);
        }

        // Need to clone all nodes used

        File_पङ्क्तिः* fl = prodlistp->fileline();
        Ast_ग्रन्थिः* newp = शून्यसूचकः;

        // "bool _Vjoin_repick = true;"  // Pick a new longer sequence
        AstVar* repickVarp = नूतन AstVar{fl, VVarType::VAR, "_Vjoin_repick", VFlagBitPacked{}, 1};
        repickVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        repickVarp->funcLocal(सत्य);
        m_breakVarp->सङ्कलय_अग्र_अत्र(repickVarp);  // Add to function top, not newp
        newp = Ast_ग्रन्थिः::सङ्कलय_अग्र(newp,
                                नूतन Ast_नियोजय{fl, नूतन AstVarRef{fl, repickVarp, VAccess::WRITE},
                                              नूतन AstConst{fl, AstConst::BitTrue{}}});

        // "int _Vjoin_nleft_0 = N(items);"  // N(a) Number left to generate in list 0, starts at
        // N(list0)
        std::deque<AstVar*> nleftVarps;
        पूर्णाङ्क i = 0;
        प्रति (AstRSProdItem* proditemp : lists) {
            // 'rand join arule arule arule' is legal, so need numbered, not non-unique named
            // nleft's
            AstVar* ध्रुव varp = नूतन AstVar{
                fl, VVarType::VAR, "_Vjoin_nleft_" + std::गन्तव्यम्_string(i++), VFlagBitPacked{}, 32};
            varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            varp->funcLocal(सत्य);
            m_breakVarp->सङ्कलय_अग्र_अत्र(varp);  // Add to function top, not newp
            nleftVarps.प्रक्षिप_पृष्ठभागः(varp);

            newp = Ast_ग्रन्थिः::सङ्कलय_अग्र(
                newp,
                नूतन Ast_नियोजय{fl, नूतन AstVarRef{fl, varp, VAccess::WRITE},
                              नूतन AstConst{fl, AstConst::Widthed_मानम्{}, 32,
                                           static_cast<uint32_t>(listStmts[proditemp].आकार())}});
        }

        // "int _Vjoin_picked_elist = 0;"  // 0 = pick first eligible (non-finished) list, 1 = pick
        // second, etc
        AstVar* ध्रुव pickedVarp
            = नूतन AstVar{fl, VVarType::VAR, "_Vjoin_picked_elist", VFlagBitPacked{}, 1};
        pickedVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        pickedVarp->funcLocal(सत्य);
        m_breakVarp->सङ्कलय_अग्र_अत्र(pickedVarp);  // Add to function top, not newp

        // "while(1) {"  // Generate until make complete sequence
        Ast_Jump_खण्डम्* ध्रुव whilep = नूतन Ast_Jump_खण्डम्{fl, शून्यसूचकः};
        newp = Ast_ग्रन्थिः::सङ्कलय_अग्र(newp, नूतन AstLoop{fl, whilep});
        {
            // "if ((std::rand() & 0xffffUL) > (0xffff * weight)) _Vjoin_repick = true;"
            whilep->सङ्कलय_Stmtsp(नूतन AstIf{
                fl,
                नूतन AstGt{
                    fl, नूतन AstRand{fl, AstRand::SRandomU32{}},
                    नूतन AstRToIS{fl, नूतन AstMulD{fl,
                                                 नूतन AstConst{fl, AstConst::RealDouble{},
                                                              static_cast<द्विगुण>(0xffffffffUL)},
                                                 weightp}}},
                नूतन Ast_नियोजय{fl, नूतन AstVarRef{fl, repickVarp, VAccess::WRITE},
                              नूतन AstConst{fl, AstConst::BitTrue{}}}});

            // "if (_Vjoin_repick) {"
            AstIf* ifp = नूतन AstIf{fl, नूतन AstVarRef{fl, repickVarp, VAccess::READ}};
            whilep->सङ्कलय_Stmtsp(ifp);
            {
                // "// _Vjoin_nlists_eligible is how many lists eligable for picking"
                // "int _Vjoin_nlists_eligible = 0;"
                AstVar* ध्रुव eligibleVarp = नूतन AstVar{
                    fl, VVarType::VAR, "_Vjoin_nlists_eligible", VFlagBitPacked{}, 32};
                eligibleVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                eligibleVarp->funcLocal(सत्य);
                m_breakVarp->सङ्कलय_अग्र_अत्र(eligibleVarp);  // Add to function top, not newp
                ifp->सङ्कलय_Thensp(नूतन Ast_नियोजय{fl, नूतन AstVarRef{fl, eligibleVarp, VAccess::WRITE},
                                             नूतन AstConst{fl, AstConst::Widthed_मानम्{}, 32, 0}});

                i = 0;
                प्रति (AstRSProdItem* proditemp : lists) {
                    (शून्य)proditemp;
                    // "_Vjoin_nlists_eligible += _Vjoin_nleft_0 ? 1 : 0;"
                    ifp->सङ्कलय_Thensp(नूतन Ast_नियोजय{
                        fl, नूतन AstVarRef{fl, eligibleVarp, VAccess::WRITE},
                        नूतन Ast_सङ्कलय{
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
                ifp->सङ्कलय_Thensp(
                    नूतन AstIf{fl,
                              नूतन AstEq{fl, नूतन AstConst{fl, AstConst::Widthed_मानम्{}, 32, 0},
                                        नूतन AstVarRef{fl, eligibleVarp, VAccess::READ}},
                              नूतन AstJumpGo{fl, m_jumpBlockp}});

                // "// +1 to simplify usage, so 0 = done"
                // "_Vjoin_picked_elist = 1 + (std::rand() % _Vjoin_nlists_eligible);"
                ifp->सङ्कलय_Thensp(नूतन Ast_नियोजय{
                    fl, नूतन AstVarRef{fl, pickedVarp, VAccess::WRITE},
                    नूतन Ast_सङ्कलय{fl, नूतन AstConst{fl, AstConst::Widthed_मानम्{}, 32, 1},
                               नूतन AstModDiv{fl, नूतन AstRand{fl, AstRand::SRandomU32{}},
                                             नूतन AstVarRef{fl, eligibleVarp, VAccess::READ}}}});

                // "_Vjoin_repick = false;
                ifp->सङ्कलय_Thensp(नूतन Ast_नियोजय{fl, नूतन AstVarRef{fl, repickVarp, VAccess::WRITE},
                                             नूतन AstConst{fl, AstConst::BitFalse{}}});
            }

            // "// Number of eligible lists left before hit _Vjoin_picked one"
            // "int _Vjoin_sel_elist = _Vjoin_picked_elist;"
            AstVar* ध्रुव selVarp
                = नूतन AstVar{fl, VVarType::VAR, "_Vjoin_sel_elist", VFlagBitPacked{}, 32};
            selVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            selVarp->funcLocal(सत्य);
            m_breakVarp->सङ्कलय_अग्र_अत्र(selVarp);  // Add to function top, not newp
            whilep->सङ्कलय_Stmtsp(नूतन Ast_नियोजय{fl, नूतन AstVarRef{fl, selVarp, VAccess::WRITE},
                                            नूतन AstVarRef{fl, pickedVarp, VAccess::READ}});

            i = -1;
            प्रति (AstRSProdItem* proditemp : lists) {
                ++i;
                // "if (_Vjoin_nleft_0) {"
                ifp = नूतन AstIf{fl,
                                नूतन AstNeq{fl, नूतन AstConst{fl, AstConst::Widthed_मानम्{}, 32, 0},
                                           नूतन AstVarRef{fl, nleftVarps[i], VAccess::READ}},
                                शून्यसूचकः, शून्यसूचकः};
                whilep->सङ्कलय_Stmtsp(ifp);
                {
                    // "--_Vjoin_sel_elist;"
                    ifp->सङ्कलय_Thensp(नूतन Ast_नियोजय{
                        fl, नूतन AstVarRef{fl, selVarp, VAccess::WRITE},
                        नूतन AstSub{fl, नूतन AstVarRef{fl, selVarp, VAccess::READ},
                                   नूतन AstConst{fl, AstConst::Widthed_मानम्{}, 32, 1}}});

                    // "if (_Vjoin_sel_elist == 0) {"
                    AstIf* ध्रुव jIfp = नूतन AstIf{
                        fl,
                        नूतन AstEq{fl, नूतन AstConst{fl, AstConst::Widthed_मानम्{}, 32, 0},
                                  नूतन AstVarRef{fl, selVarp, VAccess::READ}},
                        शून्यसूचकः, शून्यसूचकः};
                    ifp->सङ्कलय_Thensp(jIfp);
                    {
                        // "switch (_Vjoin_nleft_0) {"
                        //     "case 2 / * N(a)  * /: {statement}; break;"
                        //     "case 1 / * N(a) - 1 * /: {statement}; break;"
                        uint32_t j = static_cast<uint32_t>(listStmts[proditemp].आकार());
                        प्रति (Ast_ग्रन्थिः_Stmt* iprodp : listStmts[proditemp]) {
                            jIfp->सङ्कलय_Thensp(नूतन AstIf{
                                fl,
                                नूतन AstEq{fl, नूतन AstConst{fl, AstConst::Widthed_मानम्{}, 32, j},
                                          नूतन AstVarRef{fl, nleftVarps[i], VAccess::READ}},
                                iprodp->clone_वृक्षः(असत्य)});
                            --j;
                        }

                        // "--_Vjoin_nleft_0;"
                        jIfp->सङ्कलय_Thensp(नूतन Ast_नियोजय{
                            fl, नूतन AstVarRef{fl, nleftVarps[i], VAccess::WRITE},
                            नूतन AstSub{fl, नूतन AstVarRef{fl, nleftVarps[i], VAccess::READ},
                                       नूतन AstConst{fl, AstConst::Widthed_मानम्{}, 32, 1}}});

                        // "if (0 == _Vjoin_nleft_0) _Vjoin_repick = true;"
                        jIfp->सङ्कलय_Thensp(नूतन AstIf{
                            fl,
                            नूतन AstEq{fl, नूतन AstConst{fl, AstConst::Widthed_मानम्{}, 32, 0},
                                      नूतन AstVarRef{fl, nleftVarps[i], VAccess::READ}},
                            नूतन Ast_नियोजय{fl, नूतन AstVarRef{fl, repickVarp, VAccess::WRITE},
                                          नूतन AstConst{fl, AstConst::BitTrue{}}}});

                        // "continue;"
                        jIfp->सङ्कलय_Thensp(नूतन AstJumpGo{fl, whilep});
                    }
                }
            }
        }

        UINFOTREE(9, newp, "ForkJoin new", "-");
        प्रत्यागम newp;
    }

    // VISITORS

    शून्य दर्शय(Ast_ग्रन्थिः_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildren(nodep);
    }

    शून्य दर्शय(AstRandSequence* nodep) override {
        UINFO(9, "visit " << nodep);
        UASSERT_OBJ(m_modp, nodep, "randsequence not under module");
        यदि (m_rsp) {
            // No examples found in the wild
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: randsequence under randsequence");
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep->unlink_Fr_पृष्ठभागः()), nodep);
            प्रत्यागम;
        }

        UINFOTREE(9, nodep, "rsIn", "");
        VL_RESTORER(m_rsp);
        m_rsp = nodep;
        VL_RESTORER(m_startProdp);

        VL_RESTORER_COPY(m_localizes);
        VL_RESTORER_COPY(m_localize_नामानि);
        VL_RESTORER_COPY(m_localizeRemaps);
        findLocalizes(nodep);

        // Find first production
        m_startProdp = nodep->prodsp();
        यदि (nodep->prodp()) m_startProdp = nodep->prodp();  // Has a start production selected

        // Build production functions
        प्रति (AstRSProd* prodp = nodep->prodsp(); prodp; prodp = VN_AS(prodp->nextp(), RSProd)) {
            newProdFunc(prodp);
            // Don't iterate yet, need to have tasks we can reference
        }

        // Iterate all of the new productions, moving guts into functions just made
        // Must do those with 'rand join' first, as they need to see the raw AstRS* nodes
        // before they are removed.  (Alternative would be to keep them until the end).
        UINFOTREE(9, nodep, "RS Tree pre-it", "-");
        std::unordered_समुच्चयः<AstRSProd*> prodHasRandJoin;
        प्रति (AstRSProd* prodp = nodep->prodsp(); prodp; prodp = VN_AS(prodp->nextp(), RSProd)) {
            prodp->foreach([&](ध्रुव AstRSProdList* ध्रुव prodlistp) {
                यदि (prodlistp->randJoin()) prodHasRandJoin.emplace(prodp);
            });
        }
        प्रति (AstRSProd *nextp, *prodp = nodep->prodsp(); prodp; prodp = nextp) {
            nextp = VN_AS(prodp->nextp(), RSProd);
            यदि (prodHasRandJoin.गणनम्(prodp)) VL_DO_DANGLING(iterate(prodp), prodp);
        }
        प्रति (AstRSProd *nextp, *prodp = nodep->prodsp(); prodp; prodp = nextp) {
            nextp = VN_AS(prodp->nextp(), RSProd);
            यदि (!prodHasRandJoin.गणनम्(prodp)) VL_DO_DANGLING(iterate(prodp), prodp);
        }
        UINFOTREE(9, nodep, "RS Tree post-it", "-");

        // Replace randsequence with call to randsequence start function
        AstTask* ध्रुव startFuncp = newStartFunc(nodep);
        AstArg* argsp = शून्यसूचकः;
        प्रति (स्वचालित& itr : m_localize_नामानि) {
            AstVar* ध्रुव lvarp = itr.second;
            argsp = Ast_ग्रन्थिः::सङ्कलय_अग्र(
                argsp, नूतन AstArg{nodep->fileline(), "__Vrsarg_" + lvarp->name(),
                                  नूतन AstVarRef{nodep->fileline(), lvarp, VAccess::READWRITE}});
        }
        AstTaskRef* ध्रुव callStartp
            = नूतन AstTaskRef{nodep->fileline(), startFuncp->name(), argsp};
        callStartp->taskp(startFuncp);
        nodep->replaceWith(नूतन Ast_Stmt_व्यञ्जकम्{nodep->fileline(), callStartp});

        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
    }
    शून्य दर्शय(AstRSProd* nodep) override {
        // Production, immediately under a randsequence
        UINFO(9, "visit " << nodep);
        UINFOTREE(9, nodep, "visit " << nodep, "-");
        UASSERT_OBJ(!m_prodFuncp, nodep, "recursive production definition?");
        VL_RESTORER(m_prodFuncp);
        स्वचालित it = m_prodFuncps.find(nodep);
        UASSERT_OBJ(it != m_prodFuncps.अन्तः_2(), nodep, "No production function made");
        m_prodFuncp = it->second;

        // Create a break variable
        // TODO we could do this only if break exists in the downstream production,
        // but as-is we'll optimize it away in most cases anyways
        VL_RESTORER(m_breakVarp);
        VL_RESTORER_COPY(m_localizeRemaps);
        m_breakVarp = newBreakVar(nodep->fileline(), सत्य);
        m_prodFuncp->सङ्कलय_Stmtsp(m_breakVarp);

        // Production formal ports become real input ports on the generated task.
        // Must be added after m_breakVarp / localize ports so positional argument
        // order in newProdFuncRef stays consistent: [break, localize..., user...].
        यदि (Ast_ग्रन्थिः* ध्रुव portsp = nodep->portsp()) {
            portsp->unlink_Fr_पृष्ठभागः_With_अग्र();
            m_prodFuncp->सङ्कलय_Stmtsp(portsp);
            प्रति (Ast_ग्रन्थिः* itp = portsp; itp; itp = itp->nextp()) {
                AstVar* ध्रुव पत्तनम्_Varp = VN_AS(itp, Var);
                पत्तनम्_Varp->funcLocal(सत्य);
                पत्तनम्_Varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            }
        }

        // Put JumpBlock immediately under the new function to support
        // a future break/return.  V3Const will rip it out if unneeded.
        VL_RESTORER(m_jumpBlockp);
        m_jumpBlockp = नूतन Ast_Jump_खण्डम्{nodep->fileline(), शून्यसूचकः};
        m_prodFuncp->सङ्कलय_Stmtsp(m_jumpBlockp);

        यदि (nodep->fvarp())
            nodep->fvarp()->v3warn(E_UNSUPPORTED,
                                   "Unsupported: randsequence production function variable");

        // Move children into m_prodFuncp, and iterate there
        यदि (!nodep->rulesp()) {  // Nothing to do
        } अन्यथा यदि (nodep->rulesp()->prodlistsp() && nodep->rulesp()->prodlistsp()->randJoin()) {
            AstRSProdList* ध्रुव prodlistp = nodep->rulesp()->prodlistsp();
            Ast_ग्रन्थिः* ध्रुव itemsp = newProdRandJoin(nodep, prodlistp);
            m_jumpBlockp->सङ्कलय_Stmtsp(itemsp);
            iterate_And_अग्र_Null(itemsp);
        } अन्यथा यदि (!nodep->rulesp()->nextp()) {  // Single rule/list, can just do it
            // RSPROD(RSRULE(weight, stmt)) -> IF(weight != 0, stmt)
            ध्रुव AstRSRule* ध्रुव rulep = nodep->rulesp();
            Ast_ग्रन्थिः* itemsp = शून्यसूचकः;
            यदि (rulep->weightStmtsp()) itemsp = rulep->weightStmtsp()->unlink_Fr_पृष्ठभागः_With_अग्र();
            यदि (rulep->prodlistsp())
                itemsp = Ast_ग्रन्थिः::सङ्कलय_अग्र(itemsp, rulep->prodlistsp()->unlink_Fr_पृष्ठभागः_With_अग्र());
            // Can ignore rulep->weightp() unless is zero, in which case noop, add if
            यदि (rulep->weightp())
                itemsp = नूतन AstIf{rulep->weightp()->fileline(),
                                   नूतन AstNeq{rulep->weightp()->fileline(),
                                              नूतन AstConst{rulep->weightp()->fileline(), 0},
                                              rulep->weightp()->unlink_Fr_पृष्ठभागः()},
                                   itemsp};
            // Move prodlist to parent (m_prodFunc) and process
            यदि (itemsp) {
                UINFOTREE(9, itemsp, "additems", "-");
                m_jumpBlockp->सङ्कलय_Stmtsp(itemsp);
                iterate_And_अग्र_Null(itemsp);
                UINFOTREE(9, m_jumpBlockp, "jumpBlockNow", "-");
            }
        } अन्यथा {
            // List of rules with "OR", need to random-weight them
            // RSPROD(RSRULE(weight1, stmt!), RSRULE(weight2, stmt2))
            //   -> RANDCASE(CASEITEM(weight1, stmt1), CASEITEM(weight2, stmt2))
            AstRandCase* ध्रुव randcasep = नूतन AstRandCase{nodep->fileline(), शून्यसूचकः};
            प्रति (AstRSRule* rulep = nodep->rulesp(); rulep;
                 rulep = VN_AS(rulep->nextp(), RSRule)) {
                Ast_ग्रन्थिः* itemsp = शून्यसूचकः;
                यदि (rulep->weightStmtsp()) itemsp = rulep->weightStmtsp()->unlink_Fr_पृष्ठभागः_With_अग्र();
                यदि (rulep->prodlistsp())
                    itemsp = Ast_ग्रन्थिः::सङ्कलय_अग्र(itemsp, rulep->prodlistsp()->unlink_Fr_पृष्ठभागः_With_अग्र());
                यदि (itemsp) {
                    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव weightp = rulep->weightp()
                                                     ? rulep->weightp()->unlink_Fr_पृष्ठभागः()
                                                     : नूतन AstConst{rulep->fileline(), 1};
                    randcasep->सङ्कलय_Itemsp(नूतन AstCaseItem{rulep->fileline(), weightp, itemsp});
                }
            }
            m_jumpBlockp->सङ्कलय_Stmtsp(randcasep);
            iterate_And_अग्र_Null(randcasep);
        }

        // V3Task needs to know if we ended up making a recursive function
        m_prodFuncp->foreach([&](Ast_ग्रन्थिः_FTask_Ref* ध्रुव nodep) {
            यदि (nodep->taskp() == m_prodFuncp) m_prodFuncp->recursive(सत्य);
        });
        UINFOTREE(9, m_prodFuncp, "rsprod-task-done " << nodep, "-");

        // Done with production, should have moved everything to new task
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep->unlink_Fr_पृष्ठभागः()), nodep);
    }
    शून्य दर्शय(AstRSRule* nodep) override {
        // Rule of what to produce (as a list) under a production
        nodep->v3fatalSrc("randsequence rule should be iterated and deleted by AstRSProd");
    }
    शून्य दर्शय(AstRSProdList* nodep) override {
        // List of productions to call, potentially with a weight
        UINFO(9, "visit " << nodep);
        UASSERT_OBJ(m_prodFuncp, nodep, "RSProdList not under production");

        // Move prodlist to parent (m_prodFunc) and process
        यदि (Ast_ग्रन्थिः* ध्रुव itemsp = nodep->prodsp()) {
            UASSERT_OBJ(!nodep->randJoin(), nodep,
                        "rand join should have been handled in visit(RSRule)");
            nodep->replaceWith(itemsp->unlink_Fr_पृष्ठभागः_With_अग्र());
            // Will soon iterate itemsp as part of normal visit process
            // These will become sequential in the generated task
        } अन्यथा {
            nodep->unlink_Fr_पृष्ठभागः();
        }
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
    }
    शून्य दर्शय(AstRSProdItem* nodep) override {
        // "call" to generate using another production
        UINFO(9, "visit " << nodep);
        UASSERT_OBJ(m_prodFuncp, nodep, "RSProdItem not under production");
        AstRSProd* ध्रुव foundp = nodep->prodp();
        UASSERT_OBJ(foundp, nodep, "Unlinked production reference");
        // Pass through caller-side argument list (IEEE 1800-2023 18.17.7).
        AstArg* ध्रुव userArgsp
            = nodep->argsp() ? nodep->argsp()->unlink_Fr_पृष्ठभागः_With_अग्र() : शून्यसूचकः;
        // Convert to task call
        Ast_ग्रन्थिः* ध्रुव newp = newProdFuncRef(nodep, foundp, m_breakVarp, userArgsp);
        // The production might have done a "break;", skip other steps if so
        newp->सङ्कलय_अग्र(नूतन AstIf{nodep->fileline(),
                                नूतन AstVarRef{nodep->fileline(), m_breakVarp, VAccess::READ},
                                नूतन AstJumpGo{nodep->fileline(), m_jumpBlockp}});

        nodep->replaceWith(newp);
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
    }

    शून्य दर्शय(AstRSBreak* nodep) override {
        UASSERT_OBJ(m_jumpBlockp, nodep, "RSBreak not under production jump block");
        UASSERT_OBJ(m_breakVarp, nodep, "no break variable");
        Ast_ग्रन्थिः_Stmt* ध्रुव newp = नूतन Ast_नियोजय{
            nodep->fileline(), नूतन AstVarRef{nodep->fileline(), m_breakVarp, VAccess::WRITE},
            नूतन AstConst{nodep->fileline(), AstConst::BitTrue{}}};
        newp->सङ्कलय_अग्र(नूतन AstJumpGo{nodep->fileline(), m_jumpBlockp});
        nodep->replaceWith(newp);
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
    }
    शून्य दर्शय(AstRSReturn* nodep) override {
        UASSERT_OBJ(m_jumpBlockp, nodep, "RSReturn not under production jump block");
        nodep->replaceWith(नूतन AstJumpGo{nodep->fileline(), m_jumpBlockp});
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
    }
    शून्य दर्शय(AstVarRef* nodep) override {
        // Change references to variables inside randsequence to references to
        // ref variables on the new task
        ध्रुव स्वचालित it = m_localizeRemaps.find(nodep->varp());
        यदि (it != m_localizeRemaps.अन्तः_2()) {
            AstVar* ध्रुव iovarp = it->second;
            UINFO(9, "VARREF remap " << nodep << " -> " << iovarp);
            nodep->varp(iovarp);
        }
    }

    शून्य दर्शय(Ast_ग्रन्थिः* nodep) override { iterateChildren(nodep); }

सार्वजनिक:
    // CONSTRUCTORS
    स्पष्ट RandSequenceVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~RandSequenceVisitor() override = स्वभाव;
};

//######################################################################
// RandSequence method class functions

शून्य V3RandSequence::randSequenceNetlist(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { ध्रुव RandSequenceVisitor randSequenceVisitor{nodep}; }
    V3Global::dump_Check_Global_वृक्षः("randsequence", 0, dump_वृक्षः_Either_Level() >= 3);
}
