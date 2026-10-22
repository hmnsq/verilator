// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Collect and print statistics
//
// Code available from: https://verilator.org
//
//*************************************************************************
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of either the GNU Lesser General Public License Version 3
// or the Perl Artistic License Version 2.0.
// SPDX-FileCopyrightText: 2005-2026 Wilson Snyder
// SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0
//
//*************************************************************************
//  Pre steps:
//      Attach clocks to each assertion
//      Substitute property references by property body (IEEE 1800-2023 16.12.1).
//      Transform clocking blocks into imperative logic
//*************************************************************************

#અન્તર્ભાવ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#અન્તર્ભાવ "V3AssertPre.h"

#અન્તર્ભાવ "V3Assert.h"
#અન્તર્ભાવ "V3Const.h"
#અન્તર્ભાવ "V3Task.h"
#અન્તર્ભાવ "V3Timing.h"
#અન્તર્ભાવ "V3UniqueNames.h"

#અન્તર્ભાવ <unordered_map>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Clocking blocks of clocking items

વર્ગ AssertPreClockingVisitor final : જાહેર VNVisitorConst {
    // Captures the clocking block of each clocking item before AssertPreVisitor, which needs it
    // at synchronous drives, often visited before the clocking block, e.g. in a parent module
    // NODE STATE
    // AstClockingItem::user3p()  // AstClocking*. Clocking block of the item, user3 claimed by
    //                            //               AssertPreVisitor

    // VISITORS
    શૂન્ય visit(AstNetlist* nodep) override { iterateAndNextConstNull(nodep->modulesp()); }
    શૂન્ય visit(Ast_ગાંઠ_Module* nodep) override { iterateAndNextConstNull(nodep->stmtsp()); }
    શૂન્ય visit(AstGenBlock* nodep) override { iterateAndNextConstNull(nodep->itemsp()); }
    શૂન્ય visit(AstClocking* nodep) override {
        માટે (Ast_ગાંઠ* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
            જો (AstClockingItem* અચલ citemp = VN_CAST(itemp, ClockingItem)) {
                citemp->user3p(nodep);
            }
        }
    }
    // Clocking blocks are only in modules and their generate blocks
    શૂન્ય visit(Ast_ગાંઠ*) override {}

જાહેર:
    // CONSTRUCTORS
    explicit AssertPreClockingVisitor(AstNetlist* nodep) { iterateConst(nodep); }
    ~AssertPreClockingVisitor() override = મૂળભૂત;
};

//######################################################################
// Assert class functions

વર્ગ AssertPreVisitor final : જાહેર VNVisitor {
    // Removes clocks and other pre-optimizations
    // Eventually inlines calls to sequences, properties, etc.
    // We're not parsing the tree, or anything more complicated.
ખાનગી:
    // TYPES
    સંરચના SynchDrive final {
        AstClockingItem* itemp;  // Clocking item of the driven clockvar
        Ast_ગાંઠ_પદાવલિ* refp;  // Reference to the driven clockvar
    };

    // NODE STATE
    // AstClockingItem::user1p()         // AstVar*.      varp() of ClockingItem after unlink
    // AstClockingItem::user2p()         // AstVar*.      Flag set by drives of output clockvar
    // AstClockingItem::user3p()         // AstClocking*. Clocking block of the item, captured
    //                                   //               by AssertPreClockingVisitor
    // AstPExpr::user1()                 // bool.         Created from AstUntil
    અચલ VNUser1InUse m_inuser1;
    અચલ VNUser2InUse m_inuser2;
    અચલ VNUser3InUse m_inuser3;
    // STATE
    // Current context:
    AstNetlist* અચલ m_netlistp = nullptr;  // Current netlist
    Ast_ગાંઠ_Module* m_modp = nullptr;  // Current module
    AstClocking* m_clockingp = nullptr;  // Current clocking block
    // Reset each module:
    AstClocking* m_defaultClockingp = nullptr;  // Default clocking for current module
    AstVar* m_defaultClkEvtVarp = nullptr;  // Event var for default clocking (for ##0)
    AstDefaultDisable* m_defaultDisablep = nullptr;  // Default disable for current module
    // Reset each assertion:
    AstSenItem* m_senip = nullptr;  // Last sensitivity
    // Reset each always:
    AstSenItem* m_seniAlwaysp = nullptr;  // Last sensitivity in always
    // Reset each assertion:
    Ast_ગાંઠ_પદાવલિ* m_disablep = nullptr;  // Last disable
    AstIf* m_disableSeqIfp = nullptr;  // Used for handling disable iff in sequences
    AstPExpr* m_pexprp = nullptr;  // Last AstPExpr
    તાર્કિક m_underCover = ખોટું;  // True if the enclosing assertion is a cover
    // Other:
    V3UniqueNames m_cycleDlyNames{"__VcycleDly"};  // Cycle delay counter name generator
    V3UniqueNames m_consRepNames{"__VconsRep"};  // Consecutive repetition counter name generator
    V3UniqueNames m_gotoRepNames{"__VgotoRep"};  // Goto repetition counter name generator
    V3UniqueNames m_nonConsRepNames{"__VnonConsRep"};  // Nonconsecutive rep name generator
    V3UniqueNames m_disableCntNames{"__VdisableCnt"};  // Disable condition counter name generator
    V3UniqueNames m_blockNames{"__VassertBlock"};  // Names of blocks with temporaries
    V3UniqueNames m_propVarNames{"__Vpropvar"};  // Property-local variable name generator
    V3UniqueNames m_activeNames{"__VassertsActive"};  // Active asserts map name generator
    V3UniqueNames m_drivenNames{"__VclockingDriven"};  // Clockvar drive flag name generator
    તાર્કિક m_inAssign = ખોટું;  // True if in an AssignNode
    તાર્કિક m_inAssignDlyLhs = ખોટું;  // True if in AssignDly's LHS
    તાર્કિક m_inSynchDrive = ખોટું;  // True if in synchronous drive
    તાર્કિક m_inConcatAssign = ખોટું;  // True if in assignment to a concatenation
    std::vector<SynchDrive> m_drives;  // Clockvars written by the synchronous drive
    તાર્કિક m_hasCycleDelay = ખોટું;  // True if node has cycle delay beneath
    std::vector<AstVarXRef*> m_xrefsp;  // list of xrefs that need name fixup
    std::vector<AstSequence*> m_seqsToCleanup;  // Sequences to clean up after traversal

    // METHODS

    // Flag set by each drive of an output clockvar, added to the module with its clocking item
    AstVar* getCreateDrivenVarp(AstClockingItem* itemp) {
        જો (!itemp->user2p()) {
            AstVar* અચલ varp = નવું AstVar{itemp->fileline(), VVarType::MODULETEMP,
                                            m_drivenNames.get(itemp), itemp->findBitDType()};
            varp->lifetime(VLifetime::STATIC_EXPLICIT);
            itemp->user2p(varp);
        }
        પાછા VN_AS(itemp->user2p(), Var);
    }
    // Reference to a variable in the scope of a driven clockvar, made like the clockvar's
    સ્થિર Ast_ગાંઠ_પદાવલિ* newDriveRefp(File_લીટી* flp, અચલ SynchDrive& drive, AstVar* varp,
                                     અચલ VAccess& access) {
        જો (અચલ AstVarXRef* અચલ xrefp = VN_CAST(drive.refp, VarXRef)) {
            પાછા નવું AstVarXRef{flp, varp, xrefp->dotted(), access};
        } નહીંતર જો (અચલ AstMemberSel* અચલ selp = VN_CAST(drive.refp, MemberSel)) {
            // The interface expression is evaluated again, so it must not have side effects;
            // cloneTreePure warns if it has any
            AstMemberSel* અચલ newSelp
                = નવું AstMemberSel{flp, selp->fromp()->clone_વૃક્ષ_Pure(ખોટું), varp};
            newSelp->access(access);
            પાછા newSelp;
        }
        પાછા નવું AstVarRef{flp, varp, access};
    }
    // Assignment setting the drive flag of a driven clockvar
    AstAssign* newDrivenSetp(File_લીટી* flp, અચલ SynchDrive& drive) {
        AstVar* અચલ varp = getCreateDrivenVarp(drive.itemp);
        AstAssign* અચલ setp = નવું AstAssign{flp, newDriveRefp(flp, drive, varp, VAccess::WRITE),
                                              નવું AstConst{flp, AstConst::BitTrue{}}};
        setp->user1(સાચું);
        પાછા setp;
    }
    // Clocking block of a clocking item
    સ્થિર AstClocking* clockingOf(અચલ AstClockingItem* itemp) {
        UASSERT_OBJ(itemp->user3p(), itemp, "Clocking item not captured in a clocking block");
        પાછા VN_AS(itemp->user3p(), Clocking);
    }
    // Clocking event of a driven clockvar, referenced from the drive like the clockvar, or
    // nullptr if unsupported, which is reported on the drive's cycle delay
    Ast_Sen_વૃક્ષ* newDriveSentreep(AstDelay* delayp, અચલ SynchDrive& drive) {
        Ast_Sen_વૃક્ષ* અચલ origp = clockingOf(drive.itemp)->sentreep();
        જો (VN_IS(drive.refp, VarRef)) પાછા origp->clone_વૃક્ષ(ખોટું);
        જો (origp->exists([](અચલ AstVarXRef*) { પાછા સાચું; })) {
            // The reference is relative to the clocking block
            delayp->v3warn(E_UNSUPPORTED, "Unsupported: cycle delay in synchronous drive to"
                                          " clockvar in another scope, whose clocking event"
                                          " has a hierarchical reference");
            પાછા nullptr;
        }
        અચલ AstMemberSel* અચલ selp = VN_CAST(drive.refp, MemberSel);
        // Processes outside classes evaluate their events also when not waiting on them,
        // possibly before the interface reference is set, so the event is guarded against a
        // null reference, which needs an integral value; scheduling does not guard it
        અચલ તાર્કિક guardNull = selp && !VN_IS(m_modp, Class);
        જો (guardNull) {
            માટે (AstSenItem* itemp = origp->sensesp(); itemp;
                 itemp = VN_AS(itemp->nextp(), SenItem)) {
                જો (!itemp->sensp()->dtypep()->skipRefp()->isIntegralOrPacked()) {
                    delayp->v3warn(E_UNSUPPORTED,
                                   "Unsupported: cycle delay in synchronous drive through"
                                   " an interface reference outside a class, to clocking"
                                   " block with non-integral clocking event");
                    પાછા nullptr;
                }
            }
        }
        // Interface declaring the clocking block, if driven through an interface reference
        AstIface* અચલ ifacep
            = selp ? VN_AS(selp->fromp()->dtypep()->skipRefp(), IfaceRefDType)->ifaceViaCellp()
                   : nullptr;
        Ast_Sen_વૃક્ષ* અચલ sentreep = origp->clone_વૃક્ષ(ખોટું);
        std::vector<AstVarRef*> refps;
        sentreep->foreach([&](AstVarRef* refp) {
            જો (!refp->classOrPackagep()) refps.ધકેલો_back(refp);
        });
        માટે (AstVarRef* અચલ refp : refps) {
            AstVar* અચલ varp = refp->varp();
            જો (ifacep) {
                // Mark the variable as read through an interface reference, as V3Width does for
                // a member select. The mark is the interface declaring the variable, whose
                // instances all share it, so drives through references to any instances agree.
                UASSERT_OBJ(!varp->sensIfacep() || varp->sensIfacep() == ifacep, refp,
                            "Variable read through references to different interfaces");
                varp->sensIfacep(ifacep);
            }
            refp->replaceWith(newDriveRefp(refp->fileline(), drive, varp, refp->access()));
            VL_DO_DANGLING(ધકેલો_Deletep(refp), refp);
        }
        જો (guardNull) {
            માટે (AstSenItem* itemp = sentreep->sensesp(); itemp;
                 itemp = VN_AS(itemp->nextp(), SenItem)) {
                guardNullIface(itemp, selp->fromp());
            }
        }
        પાછા sentreep;
    }
    // Make an integral clocking event through an interface reference false while the reference
    // is null
    સ્થિર શૂન્ય guardNullIface(AstSenItem* sensesp, Ast_ગાંઠ_પદાવલિ* fromp) {
        File_લીટી* અચલ flp = sensesp->fileline();
        Ast_ગાંઠ_પદાવલિ* અચલ sensp = sensesp->sensp()->unlinkFrBack();
        sensesp->sensp(નવું AstCond{flp, newNotNullp(flp, fromp), sensp,
                                   નવું AstConst{flp, AstConst::DTyped{}, sensp->dtypep()}});
        જો (Ast_ગાંઠ_પદાવલિ* અચલ condp = sensesp->condp()) {
            condp->unlinkFrBack();
            sensesp->condp(નવું AstLogAnd{flp, newNotNullp(flp, fromp), condp});
        }
    }
    સ્થિર Ast_ગાંઠ_પદાવલિ* newNotNullp(File_લીટી* flp, Ast_ગાંઠ_પદાવલિ* fromp) {
        પાછા નવું AstNeq{flp, fromp->clone_વૃક્ષ_Pure(ખોટું), નવું AstConst{flp, AstConst::Null{}}};
    }
    // Procedural '##0': if the default clocking event has not yet occurred in this time step,
    // wait for it; otherwise continue without suspension (IEEE 1800-2023 14.11)
    AstIf* newCycleDelayZerop(File_લીટી* flp) અચલ {
        AstVar* અચલ evtVarp = m_defaultClkEvtVarp;
        UASSERT_OBJ(evtVarp, flp, "Default clocking event var not pre-created");
        AstCMethodHard* અચલ isTriggeredp = નવું AstCMethodHard{
            flp, નવું AstVarRef{flp, evtVarp, VAccess::READ}, VCMethod::EVENT_IS_TRIGGERED};
        isTriggeredp->dtypeSetBit();
        AstEventControl* અચલ waitp = નવું AstEventControl{
            flp,
            નવું Ast_Sen_વૃક્ષ{flp, નવું AstSenItem{flp, VEdgeType::ET_EVENT,
                                               નવું AstVarRef{flp, evtVarp, VAccess::READ}}},
            nullptr};
        પાછા નવું AstIf{flp, નવું AstNot{flp, isTriggeredp}, waitp};
    }

    સ્થિર શૂન્ય checkSamplingFuncDType(Ast_ગાંઠ_પદાવલિ* nodep, અચલ Ast_ગાંઠ* exprp) {
        અચલ Ast_ગાંઠ_DType* અચલ dtypep = exprp->dtypep()->skipRefp();
        જો (!dtypep->isIntegralOrPacked()) {
            nodep->v3error("Expected numeric type, but got a " << dtypep->prettyDTypeNameQ()
                                                               << " data type");
        }
    }

    Ast_Sen_વૃક્ષ* new_Sen_વૃક્ષ(Ast_ગાંઠ* nodep, Ast_Sen_વૃક્ષ* useTreep = nullptr,
                           Ast_ગાંઠ_Cover_Or_Assert* cassertp = nullptr) {
        // Create sentree based on clocked or default clock
        // Return nullptr for always
        જો (useTreep) પાછા useTreep;
        Ast_Sen_વૃક્ષ* newp = nullptr;
        AstSenItem* senip = m_senip;
        તાર્કિક fromAlways = ખોટું;
        જો (!senip && m_defaultClockingp) senip = m_defaultClockingp->sentreep()->sensesp();
        જો (!senip) {
            senip = m_seniAlwaysp;
            fromAlways = સાચું;
        }
        જો (!senip) {
            nodep->v3error("Concurrent assertion has no clock (IEEE 1800-2023 16.16)\n"
                           << nodep->warnMore()
                           << "... Suggest provide a clocking event, a default"
                              " clocking, or a clocked procedural context");
            newp = નવું Ast_Sen_વૃક્ષ{nodep->fileline(), nullptr};
        } નહીંતર {
            જો (cassertp && fromAlways) cassertp->senFromAlways(સાચું);
            newp = નવું Ast_Sen_વૃક્ષ{nodep->fileline(), senip->clone_વૃક્ષ(સાચું)};
        }
        પાછા newp;
    }
    Ast_ગાંઠ_પદાવલિ* getSequenceBodyExprp(અચલ AstSequence* અચલ seqp) અચલ {
        // The statements in AstSequence are optional AstVar (ports) followed by body expr.
        Ast_ગાંઠ* bodyp = seqp->stmtsp();
        જ્યારે (bodyp && VN_IS(bodyp, Var)) bodyp = bodyp->nextp();
        પાછા VN_CAST(bodyp, ગાંઠ_પદાવલિ);
    }
    AstPropSpec* getPropertyExprp(અચલ AstProperty* અચલ propp) {
        // Statements in AstProperty: AstVar (ports/local vars),
        // AstInitialStaticStmt/AstInitialAutomaticStmt (var init), AstPropSpec (body).
        Ast_ગાંઠ* propExprp = propp->stmtsp();
        જ્યારે (propExprp
               && (VN_IS(propExprp, Var) || VN_IS(propExprp, InitialStaticStmt)
                   || VN_IS(propExprp, InitialAutomaticStmt))) {
            propExprp = propExprp->nextp();
        }
        પાછા VN_CAST(propExprp, PropSpec);
    }
    શૂન્ય substituteSequenceCall(AstFuncRef* funcrefp, AstSequence* seqp) {
        // IEEE 1800-2023 16.7 (sequence declarations), 16.8 (sequence instances)
        // Inline the sequence body at the call site, replacing the FuncRef
        Ast_ગાંઠ_પદાવલિ* bodyExprp = getSequenceBodyExprp(seqp);
        UASSERT_OBJ(bodyExprp, funcrefp, "Sequence has no body expression");
        // Clone the body expression since the sequence may be referenced multiple times
        Ast_ગાંઠ_પદાવલિ* clonedp = bodyExprp->clone_વૃક્ષ(ખોટું);
        // Build substitution map, then do a single traversal to replace all formals
        // (textual substitution per IEEE 16.8.2).
        અચલ V3TaskConnects tconnects = V3Task::taskConnects(funcrefp, seqp->stmtsp());
        std::unordered_map<અચલ AstVar*, Ast_ગાંઠ_પદાવલિ*> portMap;
        માટે (અચલ auto& tconnect : tconnects) {
            portMap[tconnect.first] = tconnect.second->exprp();
        }
        clonedp->foreach([&](AstVarRef* refp) {
            અચલ auto it = portMap.find(refp->varp());
            જો (it != portMap.end()) {
                refp->replaceWith(it->second->clone_વૃક્ષ(ખોટું));
                VL_DO_DANGLING(ધકેલો_Deletep(refp), refp);
            }
        });
        માટે (અચલ auto& tconnect : tconnects) {
            ધકેલો_Deletep(tconnect.second->exprp()->unlinkFrBack());
        }
        V3AssertCommon::checkSvaInstanceDelays(clonedp, funcrefp);
        // Replace the FuncRef with the inlined body
        funcrefp->replaceWith(clonedp);
        VL_DO_DANGLING(ધકેલો_Deletep(funcrefp), funcrefp);
        // Clear referenced flag; sequences with isReferenced==false are deleted in assertPreAll
        seqp->isReferenced(ખોટું);
    }
    AstPropSpec* clonePropertyBody(AstFuncRef* funcrefp, અચલ AstProperty* propp) {
        AstPropSpec* propExprp = getPropertyExprp(propp);
        // Substitute inner property call before copying in order to not doing the same for
        // each call of outer property call.
        propExprp = substitutePropertyCall(propExprp);
        // Clone subtree after substitution. It is needed, because property might be called
        // multiple times with different arguments.
        propExprp = propExprp->clone_વૃક્ષ(ખોટું);
        // Build substitution maps for formal arguments and property-local
        // variables, then perform a single foreach to apply all replacements.
        // Map port vars to their actual argument expressions
        અચલ V3TaskConnects tconnects = V3Task::taskConnects(funcrefp, propp->stmtsp());
        std::unordered_map<અચલ AstVar*, Ast_ગાંઠ_પદાવલિ*> portMap;
        માટે (અચલ auto& tconnect : tconnects) {
            portMap[tconnect.first] = tconnect.second->exprp();
        }

        // Promote property-local variables (non-port vars, IEEE 16.10) to
        // module-level __Vpropvar temps. Cross-cycle persistence is handled
        // by the match item lowering in visit(AstImplication*).
        std::unordered_map<અચલ AstVar*, AstVar*> localVarMap;
        માટે (Ast_ગાંઠ* stmtp = propp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            જો (AstVar* અચલ varp = VN_CAST(stmtp, Var)) {
                જો (!varp->isIO()) {
                    અચલ string newName = m_propVarNames.get(varp);
                    AstVar* અચલ newVarp = નવું AstVar{varp->fileline(), VVarType::MODULETEMP,
                                                       newName, varp->dtypep()};
                    newVarp->lifetime(VLifetime::STATIC_EXPLICIT);
                    m_modp->addStmtsp(newVarp);
                    localVarMap[varp] = newVarp;
                }
            }
        }

        // Single traversal: substitute ports and update local var references
        propExprp->foreach([&](AstVarRef* refp) {
            અચલ auto portIt = portMap.find(refp->varp());
            જો (portIt != portMap.end()) {
                refp->replaceWith(portIt->second->clone_વૃક્ષ(ખોટું));
                VL_DO_DANGLING(ધકેલો_Deletep(refp), refp);
                પાછા;
            }
            અચલ auto localIt = localVarMap.find(refp->varp());
            જો (localIt != localVarMap.end()) refp->varp(localIt->second);
        });  // LCOV_EXCL_LINE -- gcov attributes lambda's implicit return to `})`

        // Clean up argument expressions
        માટે (અચલ auto& tconnect : tconnects) {
            ધકેલો_Deletep(tconnect.second->exprp()->unlinkFrBack());
        }
        V3AssertCommon::checkSvaInstanceDelays(propExprp, funcrefp);
        પાછા propExprp;
    }
    AstPropSpec* substitutePropertyCall(AstPropSpec* nodep) {
        જો (AstFuncRef* અચલ funcrefp = VN_CAST(nodep->propp(), FuncRef)) {
            જો (અચલ AstProperty* અચલ propp = VN_CAST(funcrefp->taskp(), Property)) {
                AstPropSpec* અચલ propExprp = clonePropertyBody(funcrefp, propp);

                // Handle case with 2 disable iff statement (IEEE 1800-2023 16.12.1)
                જો (nodep->disablep() && propExprp->disablep()) {
                    nodep->v3error("disable iff expression before property call and in its "
                                   "body is not legal");
                    ધકેલો_Deletep(propExprp->disablep()->unlinkFrBack());
                }
                // If disable iff is in outer property, move it to inner
                જો (nodep->disablep()) {
                    Ast_ગાંઠ_પદાવલિ* અચલ disablep = nodep->disablep()->unlinkFrBack();
                    propExprp->disablep(disablep);
                }

                જો (nodep->sensesp() && propExprp->sensesp()) {
                    nodep->v3warn(E_UNSUPPORTED,
                                  "Unsupported: Clock event before property call and in its body");
                    ધકેલો_Deletep(propExprp->sensesp()->unlinkFrBack());
                }
                // If clock event is in outer property, move it to inner
                જો (nodep->sensesp()) {
                    AstSenItem* અચલ sensesp = nodep->sensesp();
                    sensesp->unlinkFrBack();
                    propExprp->sensesp(sensesp);
                }

                // Now substitute property reference with property body
                nodep->replaceWith(propExprp);
                VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
                પાછા propExprp;
            }
        }
        પાછા nodep;
    }

    શૂન્ય substituteNestedPropertyCall(AstFuncRef* funcrefp, અચલ AstProperty* propp) {
        AstPropSpec* અચલ specp = clonePropertyBody(funcrefp, propp);
        Ast_ગાંઠ* replp = nullptr;
        જો (specp->disablep()) {
            funcrefp->v3error("Illegal 'disable iff' in a nested property instance"
                              " (IEEE 1800-2023 16.12.1)");
        } નહીંતર જો (specp->sensesp()) {
            funcrefp->v3warn(E_UNSUPPORTED,
                             "Unsupported: clock event in a nested property instance");
        } નહીંતર જો (specp->propp()->exists(
                       [](અચલ Ast_ગાંઠ_પદાવલિ* np) { પાછા np->isMultiCycleSva(); })) {
            funcrefp->v3warn(E_UNSUPPORTED,
                             "Unsupported: multi-cycle body in a nested property instance");
        } નહીંતર {
            replp = specp->propp()->unlinkFrBack();
        }
        જો (!replp) replp = નવું AstConst{funcrefp->fileline(), AstConst::BitFalse{}};
        funcrefp->replaceWith(replp);
        VL_DO_DANGLING(ધકેલો_Deletep(funcrefp), funcrefp);
        VL_DO_DANGLING(ધકેલો_Deletep(specp), specp);
    }

    // VISITORS
    //========== Statements
    શૂન્ય visit(AstClocking* અચલ nodep) override {
        VL_RESTORER(m_clockingp);
        m_clockingp = nodep;
        UINFO(8, "   CLOCKING" << nodep);
        જો (nodep->isDefault()) {
            જો (m_defaultClockingp) {
                nodep->v3error("Only one default clocking block allowed per module"
                               " (IEEE 1800-2023 14.12)");
            } નહીંતર {
                m_defaultClockingp = nodep;
                m_defaultClkEvtVarp = nodep->ensureEventp();
            }
        }
        iterateChildren(nodep);
        જો (nodep->eventp()) nodep->addNextHere(nodep->eventp()->unlinkFrBack());
        VL_DO_DANGLING(ધકેલો_Deletep(nodep->unlinkFrBack()), nodep);
    }
    શૂન્ય visit(AstModportClockingRef* અચલ nodep) override {
        // It has to be converted to a list of ModportClockingVarRefs,
        // because clocking blocks are removed in this pass
        માટે (Ast_ગાંઠ* itemp = nodep->clockingp()->itemsp(); itemp; itemp = itemp->nextp()) {
            જો (AstClockingItem* અચલ citemp = VN_CAST(itemp, ClockingItem)) {
                જો (AstVar* અચલ varp
                    = citemp->varp() ? citemp->varp() : VN_AS(citemp->user1p(), Var)) {
                    AstModportVarRef* અચલ modVarp = નવું AstModportVarRef{
                        nodep->fileline(), varp->name(), citemp->direction()};
                    modVarp->varp(varp);
                    nodep->addNextHere(modVarp);
                    જો (citemp->direction().isOutput()) {
                        // Drives through the modport set the drive flag
                        AstVar* અચલ drivenVarp = getCreateDrivenVarp(citemp);
                        AstModportVarRef* અચલ drivenRefp = નવું AstModportVarRef{
                            nodep->fileline(), drivenVarp->name(), VDirection::OUTPUT};
                        drivenRefp->varp(drivenVarp);
                        nodep->addNextHere(drivenRefp);
                    }
                }
            }
        }
        VL_DO_DANGLING(ધકેલો_Deletep(nodep->unlinkFrBack()), nodep);
    }
    શૂન્ય visit(AstClockingItem* અચલ nodep) override {
        // Get a ref to the sampled/driven variable
        AstVar* અચલ varp = nodep->varp();
        જો (!varp) {
            // Unused item
            પાછા;
        }
        // Flag set by drives of an output clockvar, possibly visited before this item
        જો (nodep->direction().isOutput()) m_modp->addStmtsp(getCreateDrivenVarp(nodep));
        File_લીટી* અચલ flp = nodep->fileline();
        V3Const::constifyEdit(nodep->skewp());
        જો (!VN_IS(nodep->skewp(), Const)) {
            nodep->skewp()->v3error("Skew must be constant (IEEE 1800-2023 14.4)");
            પાછા;
        }
        AstConst* અચલ skewp = VN_AS(nodep->skewp(), Const);
        જો (skewp->અંક().isNegative()) skewp->v3error("Skew cannot be negative");
        Ast_ગાંઠ_પદાવલિ* અચલ exprp = nodep->exprp();
        varp->name(m_clockingp->name() + "__DOT__" + varp->name());
        m_clockingp->addNextHere(varp->unlinkFrBack());
        nodep->user1p(varp);
        varp->user1p(nodep);
        જો (nodep->direction() == VDirection::OUTPUT) {
            exprp->foreach([](અચલ Ast_ગાંઠ_Var_Ref* varrefp) {
                // Prevent confusing BLKANDNBLK warnings on clockvars due to generated assignments
                varrefp->fileline()->warnOff(V3ErrorCode::BLKANDNBLK, સાચું);
                // Mark the driven signal so later passes (e.g. V3Inst port
                // connection) know the drive flows out from inside the cell.
                // Item expressions are plain signal references, so only the
                // driven signal is marked, never index selects.
                varrefp->varp()->isClockingDriven(સાચું);
            });
            AstVarRef* અચલ skewedReadRefp = નવું AstVarRef{flp, varp, VAccess::READ};
            skewedReadRefp->user1(સાચું);
            // Initialize the clockvar
            AstVarRef* અચલ skewedWriteRefp = નવું AstVarRef{flp, varp, VAccess::WRITE};
            skewedWriteRefp->user1(સાચું);
            AstInitialStatic* અચલ initClockvarp = નવું AstInitialStatic{
                flp, નવું AstAssign{flp, skewedWriteRefp, exprp->clone_વૃક્ષ_Pure(ખોટું)}};
            m_modp->addStmtsp(initClockvarp);
            // Each drive sets a flag, so the signal is also assigned when driven with the same
            // value again, e.g. after another assignment to the signal (IEEE 1800-2023 14.16.2)
            AstVar* અચલ drivenVarp = getCreateDrivenVarp(nodep);
            m_modp->addStmtsp(નવું AstInitialStatic{
                flp, નવું AstAssign{flp, નવું AstVarRef{flp, drivenVarp, VAccess::WRITE},
                                   નવું AstConst{flp, AstConst::BitFalse{}}}});
            // Assign the clockvar to the actual var if it was driven
            AstAssign* અચલ assignp
                = નવું AstAssign{flp, exprp->clone_વૃક્ષ_Pure(ખોટું), skewedReadRefp};
            AstIf* અચલ ifp
                = નવું AstIf{flp, નવું AstVarRef{flp, drivenVarp, VAccess::READ},
                            નવું AstAssign{flp, નવું AstVarRef{flp, drivenVarp, VAccess::WRITE},
                                          નવું AstConst{flp, AstConst::BitFalse{}}}};
            ifp->addThensp(assignp);
            // Only in the time step of the clocking event
            AstCMethodHard* અચલ trigp = નવું AstCMethodHard{
                nodep->fileline(), નવું AstVarRef{flp, m_clockingp->ensureEventp(), VAccess::READ},
                VCMethod::EVENT_IS_TRIGGERED};
            trigp->dtypeSetBit();
            ifp->condp(નવું AstLogAnd{flp, ifp->condp()->unlinkFrBack(), trigp});
            // A skew rounding to no time precision ticks is a zero skew
            Ast_ગાંઠ_પદાવલિ* અચલ ticksp
                = V3Timing::delayToTicks(skewp->clone_વૃક્ષ(ખોટું), m_modp->timeunit());
            અચલ તાર્કિક zeroSkew = ticksp->isZero();
            VL_DO_DANGLING(ticksp->delete_વૃક્ષ(), ticksp);
            જો (zeroSkew || !skewp->fileline()->timingOn()) {
                // Drive the var in Re-NBA (IEEE 1800-2023 14.16), ignoring skews where timing is
                // turned off
                Ast_Sen_વૃક્ષ* senTreep = m_clockingp->sentreep()->clone_વૃક્ષ(ખોટું);
                senTreep->addSensesp(નવું AstSenItem{
                    flp, VEdgeType::ET_CHANGED, નવું AstVarRef{flp, drivenVarp, VAccess::READ}});
                m_clockingp->addNextHere(નવું AstAlwaysReactive{flp, senTreep, ifp});
            } નહીંતર {
                // Fork only the assignment, so this AlwaysPostponed can be retriggered
                // before the assignment happens
                AstFork* અચલ forkp = નવું AstFork{flp, VJoinType::JOIN_NONE};
                forkp->immediateStart(સાચું);
                forkp->addForksp(નવું AstBegin{flp, "", assignp->unlinkFrBack(), સાચું});
                ifp->addThensp(forkp);
                // Read the clockvar in Postponed, so a drive late in the time step is applied
                // at this event plus the skew (IEEE 1800-2023 14.16)
                m_clockingp->addNextHere(નવું AstAlwaysPostponed{flp, ifp});
                જો (v3Global.opt.timing().isSetTrue()) {
                    AstDelay* અચલ delayp = નવું AstDelay{flp, skewp->unlinkFrBack(), ખોટું};
                    delayp->timeunit(m_modp->timeunit());
                    assignp->timingControlp(delayp);
                } નહીંતર જો (v3Global.opt.timing().isSetFalse()) {
                    nodep->v3warn(E_NOTIMING,
                                  "Clocking output skew greater than #0 requires --timing");
                } નહીંતર {
                    nodep->v3warn(E_NEEDTIMINGOPT,
                                  "Use --timing or --no-timing to specify how "
                                  "clocking output skew greater than #0 should be handled");
                }
            }
        } નહીંતર જો (nodep->direction() == VDirection::આગત) {
            // Ref to the clockvar
            AstVarRef* અચલ refp = નવું AstVarRef{flp, varp, VAccess::WRITE};
            refp->user1(સાચું);
            જો (skewp->અંક().is1Step()) {
                // #1step means the value that is sampled is always the signal's last value
                // before the clock edge (IEEE 1800-2023 14.4)
                AstSampled* અચલ sampledp
                    = નવું AstSampled{flp, exprp->clone_વૃક્ષ_Pure(ખોટું), exprp->dtypep(), સાચું};
                AstAssign* અચલ assignp = નવું AstAssign{flp, refp, sampledp};
                m_clockingp->addNextHere(નવું AstAlways{
                    flp, VAlwaysKwd::ALWAYS, m_clockingp->sentreep()->clone_વૃક્ષ(ખોટું), assignp});
            } નહીંતર જો (skewp->isZero()) {
                // #0 means the var has to be sampled in Observed (IEEE 1800-2023 14.13)
                AstAssign* અચલ assignp = નવું AstAssign{flp, refp, exprp->clone_વૃક્ષ_Pure(ખોટું)};
                m_clockingp->addNextHere(નવું AstAlwaysObserved{
                    flp, m_clockingp->sentreep()->clone_વૃક્ષ(ખોટું), assignp});
            } નહીંતર {
                // Create a queue where we'll store sampled values with timestamps
                AstSampleQueueDType* અચલ queueDtp
                    = નવું AstSampleQueueDType{flp, exprp->dtypep()};
                m_netlistp->typeTablep()->addTypesp(queueDtp);
                AstVar* અચલ queueVarp
                    = નવું AstVar{flp, VVarType::MODULETEMP, "__Vqueue__" + varp->name(), queueDtp};
                queueVarp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_clockingp->addNextHere(queueVarp);
                // Create a process like this:
                //     always queue.push(<sampled var>);
                AstCMethodHard* અચલ pushp = નવું AstCMethodHard{
                    flp, નવું AstVarRef{flp, queueVarp, VAccess::WRITE}, VCMethod::DYN_ધકેલો,
                    નવું AstTime{nodep->fileline(), m_modp->timeunit()}};
                pushp->addPinsp(exprp->clone_વૃક્ષ_Pure(ખોટું));
                pushp->dtypeSetVoid();
                m_clockingp->addNextHere(
                    નવું AstAlways{flp, VAlwaysKwd::ALWAYS, nullptr, pushp->makeStmt()});
                // Create a process like this:
                //     always @<clocking event> queue.pop(<skew>, /*out*/<skewed var>);
                AstCMethodHard* અચલ popp = નવું AstCMethodHard{
                    flp, નવું AstVarRef{flp, queueVarp, VAccess::READWRITE}, VCMethod::DYN_કાઢો,
                    નવું AstTime{nodep->fileline(), m_modp->timeunit()}};
                popp->addPinsp(skewp->unlinkFrBack());
                refp->access(VAccess::READWRITE);  // Only conditionally assigned
                popp->addPinsp(refp);
                popp->dtypeSetVoid();
                m_clockingp->addNextHere(નવું AstAlways{flp, VAlwaysKwd::ALWAYS,
                                                       m_clockingp->sentreep()->clone_વૃક્ષ(ખોટું),
                                                       popp->makeStmt()});
            }
        } નહીંતર {
            nodep->v3fatalSrc("Invalid direction");
        }
    }
    શૂન્ય visit(AstDelay* nodep) override {
        m_hasCycleDelay |= nodep->isCycleDelay();
        // Only cycle delays are relevant in this stage; also only process once
        જો (!nodep->isCycleDelay()) {
            જો (m_inSynchDrive) {
                nodep->v3error("Only cycle delays can be used in synchronous drives"
                               " (IEEE 1800-2023 14.16)");
            }
            iterateChildren(nodep);
            પાછા;
        }
        જો (m_inAssign && !m_inSynchDrive) {
            nodep->v3error("Cycle delays not allowed as intra-assignment delays"
                           " (IEEE 1800-2023 14.11)");
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_વૃક્ષ(), nodep);
            પાછા;
        }
        જો (nodep->stmtsp()) nodep->addNextHere(nodep->stmtsp()->unlinkFrBackWithNext());
        File_લીટી* અચલ flp = nodep->fileline();
        Ast_ગાંઠ_પદાવલિ* valuep = V3Const::constifyEdit(nodep->lhsp()->unlinkFrBack());
        અચલ AstConst* અચલ constp = VN_CAST(valuep, Const);
        જો (!constp && m_pexprp) {
            // V3AssertNfa handles non-const delays in sequences before this pass and
            // replaces the property; this branch should never be reached.
            nodep->v3fatalSrc("Non-constant cycle delay in assertion: "
                              "should have been caught by V3AssertNfa");
        } નહીંતર જો (constp && constp->isZero()) {
            VL_DO_DANGLING(ધકેલો_Deletep(valuep), valuep);
            જો (m_inSynchDrive) {
                // ##0 has no effect in synchronous drives (IEEE 1800-2023 14.11)
                VL_DO_DANGLING(ધકેલો_Deletep(nodep->unlinkFrBack()), nodep);
                પાછા;
            }
            જો (m_pexprp) {
                // ##0 in sequence context = zero delay = same clock tick
                VL_DO_DANGLING(ધકેલો_Deletep(nodep->unlinkFrBack()), nodep);
                પાછા;
            }
            // Procedural ##0: synchronize with default clocking event (IEEE 1800-2023 14.11)
            જો (!m_defaultClockingp) {
                nodep->v3error("Usage of cycle delays requires default clocking"
                               " (IEEE 1800-2023 14.11)");
                VL_DO_DANGLING(ધકેલો_Deletep(nodep->unlinkFrBack()), nodep);
                પાછા;
            }
            nodep->replaceWith(newCycleDelayZerop(flp));
            VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
            પાછા;
        }
        Ast_Sen_વૃક્ષ* sentreep = nullptr;
        જો (!m_drives.empty()) {
            જો (m_inConcatAssign) {
                // Also the clockvars may be of different clocking blocks
                nodep->v3error("Cycle delays cannot be used in synchronous drives to a"
                               " concatenation (IEEE 1800-2023 14.16)");
                VL_DO_DANGLING(ધકેલો_Deletep(nodep->unlinkFrBack()), nodep);
                VL_DO_DANGLING(ધકેલો_Deletep(valuep), valuep);
                પાછા;
            }
            // Count the cycles of the driven clockvar's clocking block (IEEE 1800-2023 14.16)
            sentreep = newDriveSentreep(nodep, m_drives.front());
            જો (!sentreep) {
                VL_DO_DANGLING(ધકેલો_Deletep(nodep->unlinkFrBack()), nodep);
                VL_DO_DANGLING(ધકેલો_Deletep(valuep), valuep);
                પાછા;
            }
        } નહીંતર જો (!m_defaultClockingp) {
            જો (!m_pexprp) {
                nodep->v3error("Usage of cycle delays requires default clocking"
                               " (IEEE 1800-2023 14.11)");
                VL_DO_DANGLING(ધકેલો_Deletep(nodep->unlinkFrBack()), nodep);
                VL_DO_DANGLING(ધકેલો_Deletep(valuep), valuep);
                પાછા;
            }
            sentreep = નવું Ast_Sen_વૃક્ષ{flp, m_senip->clone_વૃક્ષ(ખોટું)};
        } નહીંતર {
            sentreep = m_defaultClockingp->sentreep()->clone_વૃક્ષ(ખોટું);
        }
        AstEventControl* અચલ controlp
            = નવું AstEventControl{nodep->fileline(), sentreep, nullptr};
        અચલ std::string delayName = m_cycleDlyNames.get(nodep);
        Ast_ગાંઠ_પદાવલિ* throughoutp
            = nodep->throughoutp() ? nodep->throughoutp()->unlinkFrBack() : nullptr;

        AstVar* અચલ cntVarp = નવું AstVar{flp, VVarType::BLOCKTEMP, delayName + "__counter",
                                           nodep->findBasicDType(VBasicDTypeKwd::UINT32)};
        cntVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        AstBegin* અચલ beginp = નવું AstBegin{flp, delayName + "__block", cntVarp, સાચું};
        // A non-constant delay is counted at runtime. The count is integral of 64 bits, as
        // V3Width makes delays, or real, which is rounded (IEEE 1800-2023 6.12.1)
        જો (valuep->isDouble()) {
            valuep = નવું AstRToIRoundS{flp, valuep};
            valuep->dtypeFrom(cntVarp);
        } નહીંતર {
            valuep = નવું AstSel{flp, valuep, 0, cntVarp->width()};
        }
        beginp->addStmtsp(નવું AstAssign{flp, નવું AstVarRef{flp, cntVarp, VAccess::WRITE}, valuep});
        // If the count is zero, a synchronous drive waits for none, like with a constant '##0',
        // and a procedural delay waits like a procedural '##0' (IEEE 1800-2023 14.11)
        જો (!constp && !m_inSynchDrive) {
            beginp->addStmtsp(નવું AstIf{
                flp,
                નવું AstEq{flp, નવું AstVarRef{flp, cntVarp, VAccess::READ}, નવું AstConst{flp, 0}},
                newCycleDelayZerop(flp)});
        }

        // Throughout: create flag tracking whether condition held every tick
        AstVar* throughoutOkp = nullptr;
        જો (throughoutp) {
            throughoutOkp = નવું AstVar{flp, VVarType::BLOCKTEMP, delayName + "__throughoutOk",
                                       nodep->findBasicDType(VBasicDTypeKwd::LOGIC_IMPLICIT)};
            throughoutOkp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            beginp->addStmtsp(throughoutOkp);
            beginp->addStmtsp(નવું AstAssign{flp, નવું AstVarRef{flp, throughoutOkp, VAccess::WRITE},
                                            નવું AstConst{flp, AstConst::BitTrue{}}});
            // Check condition at tick 0 (sequence start, before entering loop)
            beginp->addStmtsp(
                નવું AstIf{flp, નવું AstLogNot{flp, throughoutp->clone_વૃક્ષ_Pure(ખોટું)},
                          નવું AstAssign{flp, નવું AstVarRef{flp, throughoutOkp, VAccess::WRITE},
                                        નવું AstConst{flp, AstConst::BitFalse{}}}});
        }

        {
            AstLoop* અચલ loopp = નવું AstLoop{flp};
            // When throughout is present, exit loop early if condition fails
            Ast_ગાંઠ_પદાવલિ* loopCondp
                = નવું AstGt{flp, નવું AstVarRef{flp, cntVarp, VAccess::READ}, નવું AstConst{flp, 0}};
            જો (throughoutOkp) {
                loopCondp = નવું AstLogAnd{flp, loopCondp,
                                          નવું AstVarRef{flp, throughoutOkp, VAccess::READ}};
            }
            loopp->addStmtsp(નવું AstLoopTest{flp, loopp, loopCondp});
            loopp->addStmtsp(controlp);
            loopp->addStmtsp(
                નવું AstAssign{flp, નવું AstVarRef{flp, cntVarp, VAccess::WRITE},
                              નવું AstSub{flp, નવું AstVarRef{flp, cntVarp, VAccess::READ},
                                         નવું AstConst{flp, 1}}});
            // Check throughout condition at each tick during delay (IEEE 1800-2023 16.9.9)
            જો (throughoutp) {
                loopp->addStmtsp(
                    નવું AstIf{flp, નવું AstLogNot{flp, throughoutp},
                              નવું AstAssign{flp, નવું AstVarRef{flp, throughoutOkp, VAccess::WRITE},
                                            નવું AstConst{flp, AstConst::BitFalse{}}}});
            }
            beginp->addStmtsp(loopp);
        }
        // Compose wrappers on remaining sequence: throughout gate (inner), disable iff (outer)
        Ast_ગાંઠ* remainp = nodep->nextp() ? nodep->nextp()->unlinkFrBackWithNext() : nullptr;
        જો (throughoutOkp) {
            // If condition failed during delay, fail assertion
            remainp = નવું AstIf{flp, નવું AstVarRef{flp, throughoutOkp, VAccess::READ}, remainp,
                                નવું AstPExprClause{flp, /*pass=*/ખોટું}};
        }
        જો (m_disableSeqIfp && remainp) {
            AstIf* અચલ disableSeqIfp = m_disableSeqIfp->clone_વૃક્ષ(ખોટું);
            disableSeqIfp->addThensp(remainp);
            remainp = disableSeqIfp;
        }
        જો (remainp) {
            જો (throughoutOkp) {
                // throughoutOkp is declared in beginp scope -- check must be inside it
                beginp->addStmtsp(remainp);
            } નહીંતર {
                nodep->addNextHere(remainp);
            }
        }
        nodep->replaceWith(beginp);
        VL_DO_DANGLING(nodep->delete_વૃક્ષ(), nodep);
    }
    શૂન્ય visit(Ast_Sen_વૃક્ષ* nodep) override {
        જો (m_inSynchDrive) {
            nodep->v3error("Event controls cannot be used in "
                           "synchronous drives (IEEE 1800-2023 14.16)");
        }

        અચલ AstSampled* sampledp;
        જો (nodep->exists([&sampledp](અચલ AstSampled* અચલ sp) {
                sampledp = sp;
                પાછા સાચું;
            })) {
            sampledp->v3warn(E_UNSUPPORTED, "Unsupported: $sampled inside sensitivity list");
        }
    }
    શૂન્ય visit(Ast_ગાંઠ_Var_Ref* nodep) override {
        UINFO(8, " -varref:  " << nodep);
        UINFO(8, " -varref-var-back:  " << nodep->varp()->backp());
        UINFO(8, " -varref-var-user1:  " << nodep->varp()->user1p());
        જો (AstClockingItem* અચલ itemp = VN_CAST(
                nodep->varp()->user1p() ? nodep->varp()->user1p() : nodep->varp()->firstAbovep(),
                ClockingItem)) {
            જો (nodep->user1()) પાછા;

            // ensure linking still works, this has to be done only once
            જો (AstVarXRef* xrefp = VN_CAST(nodep, VarXRef)) {
                UINFO(8, " -clockvarxref-in:  " << xrefp);
                string dotted = xrefp->dotted();
                અચલ size_t dotPos = dotted.rfind('.');
                dotted.erase(dotPos, string::npos);
                xrefp->dotted(dotted);
                UINFO(8, " -clockvarxref-out: " << xrefp);
                m_xrefsp.emplace_back(xrefp);
            }

            જો (nodep->access().isReadOrRW() && itemp->direction() == VDirection::OUTPUT) {
                nodep->v3error("Cannot read from output clockvar (IEEE 1800-2023 14.3)");
            }
            જો (nodep->access().isWriteOrRW()) {
                જો (itemp->direction() == VDirection::OUTPUT) {
                    જો (!m_inAssignDlyLhs) {
                        nodep->v3error("Only non-blocking assignments can write "
                                       "to clockvars (IEEE 1800-2023 14.16)");
                    }
                    જો (m_inAssign) {
                        m_inSynchDrive = સાચું;
                        m_drives.ધકેલો_back({itemp, nodep});
                    }
                } નહીંતર જો (itemp->direction() == VDirection::આગત) {
                    nodep->v3error("Cannot write to input clockvar (IEEE 1800-2023 14.3)");
                }
            }
            nodep->user1(સાચું);
        }
    }
    શૂન્ય visit(AstMemberSel* nodep) override {
        જો (AstClockingItem* અચલ itemp = VN_CAST(
                nodep->varp()->user1p() ? nodep->varp()->user1p() : nodep->varp()->firstAbovep(),
                ClockingItem)) {
            જો (nodep->access().isReadOrRW() && itemp->direction() == VDirection::OUTPUT) {
                nodep->v3error("Cannot read from output clockvar (IEEE 1800-2023 14.3)");
            }
            જો (nodep->access().isWriteOrRW()) {
                જો (itemp->direction() == VDirection::OUTPUT) {
                    જો (!m_inAssignDlyLhs) {
                        nodep->v3error("Only non-blocking assignments can write "
                                       "to clockvars (IEEE 1800-2023 14.16)");
                    }
                    જો (m_inAssign) {
                        m_inSynchDrive = સાચું;
                        m_drives.ધકેલો_back({itemp, nodep});
                    }
                } નહીંતર જો (itemp->direction() == VDirection::આગત) {
                    nodep->v3error("Cannot write to input clockvar (IEEE 1800-2023 14.3)");
                }
            }
        }
    }
    શૂન્ય visit(Ast_ગાંઠ_Assign* nodep) override {
        જો (nodep->user1()) પાછા;
        VL_RESTORER(m_inAssign);
        VL_RESTORER(m_inSynchDrive);
        VL_RESTORER(m_inConcatAssign);
        VL_RESTORER_CLEAR(m_drives);
        m_inAssign = સાચું;
        m_inSynchDrive = ખોટું;
        {
            // The braces of a concatenation are a replication, by one
            અચલ Ast_ગાંઠ_પદાવલિ* lhsp = nodep->lhsp();
            જો (અચલ AstReplicate* અચલ repp = VN_CAST(lhsp, Replicate)) lhsp = repp->srcp();
            m_inConcatAssign = VN_IS(lhsp, Concat);
        }
        {
            VL_RESTORER(m_inAssignDlyLhs);
            m_inAssignDlyLhs = VN_IS(nodep, AssignDly);
            iterate(nodep->lhsp());
        }
        iterate(nodep->rhsp());
        જો (nodep->timingControlp()) iterate(nodep->timingControlp());
        // A drive without a cycle delay, also after removing '##0', updates the clockvar now
        જો (m_inSynchDrive && !nodep->timingControlp()) {
            AstAssign* અચલ assignp = નવું AstAssign{
                nodep->fileline(), nodep->lhsp()->unlinkFrBack(), nodep->rhsp()->unlinkFrBack()};
            assignp->user1(સાચું);
            nodep->replaceWith(assignp);
            VL_DO_DANGLING(nodep->delete_વૃક્ષ(), nodep);
            nodep = assignp;
        }
        // Note the drives when they take effect, after the cycle delay if any. Each clockvar
        // of the LHS is driven, e.g. in a concatenation which other simulators accept, although
        // IEEE 1800-2023 14.16 does not allow it
        Ast_ગાંઠ* setsp = nullptr;
        માટે (અચલ SynchDrive& drive : m_drives) {
            setsp = Ast_ગાંઠ::addNext(setsp, newDrivenSetp(nodep->fileline(), drive));
        }
        જો (!setsp) પાછા;
        જો (AstBegin* અચલ beginp = VN_CAST(nodep->timingControlp(), Begin)) {
            beginp->addStmtsp(setsp);
        } નહીંતર {
            nodep->addNextHere(setsp);
        }
    }
    શૂન્ય visit(AstAlways* nodep) override {
        iterateAndNextNull(nodep->sentreep());
        જો (nodep->sentreep()) m_seniAlwaysp = nodep->sentreep()->sensesp();
        iterateAndNextNull(nodep->stmtsp());
        m_seniAlwaysp = nullptr;
    }

    શૂન્ય visit(Ast_ગાંઠ_Cover_Or_Assert* nodep) override {
        જો (nodep->sentreep()) પાછા;  // Already processed

        VL_RESTORER(m_senip);
        VL_RESTORER(m_disablep);
        m_senip = nullptr;
        m_disablep = nullptr;

        // Find Clocking's buried under nodep->exprsp
        iterateChildren(nodep);
        જો (!nodep->immediate()) nodep->sentreep(new_Sen_વૃક્ષ(nodep, nullptr, nodep));
    }
    શૂન્ય visit(AstFalling* nodep) override {
        જો (nodep->user1SetOnce()) પાછા;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        File_લીટી* અચલ fl = nodep->fileline();
        Ast_ગાંઠ_પદાવલિ* exprp = nodep->exprp()->unlinkFrBack();
        જો (exprp->width() > 1) exprp = નવું AstSel{fl, exprp, 0, 1};
        Ast_ગાંઠ_પદાવલિ* અચલ futurep = નવું AstFuture{fl, exprp, new_Sen_વૃક્ષ(nodep)};
        futurep->dtypeFrom(exprp);
        exprp = નવું AstAnd{fl, exprp->clone_વૃક્ષ_Pure(ખોટું), નવું AstNot{fl, futurep}};
        exprp->dtypeSetBit();
        nodep->replaceWith(exprp);
        VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
    }
    શૂન્ય visit(AstFell* nodep) override {
        જો (nodep->user1SetOnce()) પાછા;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        File_લીટી* અચલ fl = nodep->fileline();
        Ast_ગાંઠ_પદાવલિ* exprp = nodep->exprp()->unlinkFrBack();
        જો (exprp->width() > 1) exprp = નવું AstSel{fl, exprp, 0, 1};
        Ast_Sen_વૃક્ષ* sentreep = nodep->sentreep();
        જો (sentreep) sentreep->unlinkFrBack();
        AstPast* અચલ pastp = નવું AstPast{fl, exprp};
        pastp->dtypeFrom(exprp);
        pastp->sentreep(new_Sen_વૃક્ષ(nodep, sentreep));
        exprp = નવું AstAnd{fl, pastp, નવું AstNot{fl, exprp->clone_વૃક્ષ_Pure(ખોટું)}};
        exprp->dtypeSetBit();
        nodep->replaceWith(exprp);
        VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
    }
    શૂન્ય visit(AstFuture* nodep) override {
        જો (nodep->user1SetOnce()) પાછા;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        Ast_Sen_વૃક્ષ* અચલ sentreep = nodep->sentreep();
        જો (sentreep) VL_DO_DANGLING(ધકેલો_Deletep(sentreep->unlinkFrBack()), sentreep);
        nodep->sentreep(new_Sen_વૃક્ષ(nodep));
    }
    શૂન્ય visit(AstPast* nodep) override {
        checkSamplingFuncDType(nodep, nodep->exprp());
        જો (nodep->sentreep()) પાછા;  // Already processed
        iterateChildren(nodep);
        nodep->sentreep(new_Sen_વૃક્ષ(nodep));
    }
    શૂન્ય visit(AstSConsRep* nodep) override {
        // IEEE 1800-2023 16.9.2 -- Lower standalone exact [*N] (N >= 2) via saturating counter.
        // Range/unbounded forms and SExpr-contained forms are lowered by V3AssertNfa.
        iterateChildren(nodep);
        // V3AssertNfa handles unbounded/ranged forms upstream, so this fast-path
        // is effectively unreachable when NFA is enabled.
        જો (nodep->unbounded() || nodep->maxCountp()) પાછા;  // LCOV_EXCL_LINE
        અચલ AstConst* અચલ constp = VN_CAST(nodep->countp(), Const);
        જો (VL_UNLIKELY(!constp || constp->toSInt() < 1)) {
            nodep->v3fatalSrc("Consecutive repetition count must be a positive constant"
                              " (should have been caught by V3Width)");
            પાછા;
        }
        અચલ પૂર્ણાંક n = constp->toSInt();
        File_લીટી* અચલ flp = nodep->fileline();
        Ast_ગાંઠ_પદાવલિ* અચલ exprp = nodep->exprp()->unlinkFrBack();
        જો (n == 1) {
            nodep->replaceWith(exprp);
            VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
            પાછા;
        }
        // Saturating counter: if (expr) cnt <= min(cnt+1, N); else cnt <= 0;
        AstVar* અચલ cntVarp = નવું AstVar{flp, VVarType::MODULETEMP, m_consRepNames.get(""),
                                           nodep->findBasicDType(VBasicDTypeKwd::UINT32)};
        cntVarp->lifetime(VLifetime::STATIC_EXPLICIT);
        m_modp->addStmtsp(cntVarp);
        Ast_ગાંઠ_પદાવલિ* અચલ પદાવલિ_Clonep = exprp->clone_વૃક્ષ_Pure(ખોટું);
        Ast_ગાંઠ_પદાવલિ* અચલ saturatingIncrp = નવું AstCond{
            flp,
            નવું AstLt{flp, નવું AstVarRef{flp, cntVarp, VAccess::READ},
                      નવું AstConst{flp, static_cast<uint32_t>(n)}},
            નવું AstAdd{flp, નવું AstVarRef{flp, cntVarp, VAccess::READ}, નવું AstConst{flp, 1u}},
            નવું AstConst{flp, static_cast<uint32_t>(n)}};
        AstAssignDly* અચલ incrAssignp
            = નવું AstAssignDly{flp, નવું AstVarRef{flp, cntVarp, VAccess::WRITE}, saturatingIncrp};
        AstAssignDly* અચલ resetAssignp = નવું AstAssignDly{
            flp, નવું AstVarRef{flp, cntVarp, VAccess::WRITE}, નવું AstConst{flp, 0u}};
        AstIf* અચલ ifp = નવું AstIf{flp, પદાવલિ_Clonep, incrAssignp, resetAssignp};
        Ast_Sen_વૃક્ષ* અચલ senTreep = new_Sen_વૃક્ષ(nodep);
        AstAlways* અચલ alwaysp = નવું AstAlways{flp, VAlwaysKwd::ALWAYS, senTreep, ifp};
        cntVarp->addNextHere(alwaysp);
        // Match: cnt >= N-1 (previous cycles via NBA) && expr (current cycle)
        Ast_ગાંઠ_પદાવલિ* અચલ cntCheckp = નવું AstGte{flp, નવું AstVarRef{flp, cntVarp, VAccess::READ},
                                                  નવું AstConst{flp, static_cast<uint32_t>(n - 1)}};
        cntCheckp->dtypeSetBit();
        Ast_ગાંઠ_પદાવલિ* અચલ matchp = નવું AstAnd{flp, cntCheckp, exprp};
        matchp->dtypeSetBit();
        nodep->replaceWith(matchp);
        VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
    }
    શૂન્ય visit(AstRising* nodep) override {
        જો (nodep->user1SetOnce()) પાછા;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        File_લીટી* અચલ fl = nodep->fileline();
        Ast_ગાંઠ_પદાવલિ* exprp = nodep->exprp()->unlinkFrBack();
        જો (exprp->width() > 1) exprp = નવું AstSel{fl, exprp, 0, 1};
        Ast_ગાંઠ_પદાવલિ* અચલ futurep = નવું AstFuture{fl, exprp, new_Sen_વૃક્ષ(nodep)};
        futurep->dtypeFrom(exprp);
        exprp = નવું AstAnd{fl, નવું AstNot{fl, exprp->clone_વૃક્ષ_Pure(ખોટું)}, futurep};
        exprp->dtypeSetBit();
        nodep->replaceWith(exprp);
        VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
    }
    શૂન્ય visit(AstRose* nodep) override {
        જો (nodep->user1SetOnce()) પાછા;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        File_લીટી* અચલ fl = nodep->fileline();
        Ast_ગાંઠ_પદાવલિ* exprp = nodep->exprp()->unlinkFrBack();
        જો (exprp->width() > 1) exprp = નવું AstSel{fl, exprp, 0, 1};
        Ast_Sen_વૃક્ષ* sentreep = nodep->sentreep();
        જો (sentreep) sentreep->unlinkFrBack();
        AstPast* અચલ pastp = નવું AstPast{fl, exprp};
        pastp->dtypeFrom(exprp);
        pastp->sentreep(new_Sen_વૃક્ષ(nodep, sentreep));
        exprp = નવું AstAnd{fl, નવું AstNot{fl, pastp}, exprp->clone_વૃક્ષ_Pure(ખોટું)};
        exprp->dtypeSetBit();
        nodep->replaceWith(exprp);
        VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
    }
    સ્થિર AstAssocArrayDType* getProcessAssocArrayType(File_લીટી* અચલ flp) {
        // Type of __VassertsActive___x[std::process]
        Ast_ગાંઠ_DType* valp
            = v3Global.rootp()->typeTablep()->findBasicDType(flp, VBasicDTypeKwd::BIT);
        AstClassRefDType* keyp
            = નવું AstClassRefDType{flp, v3Global.rootp()->stdPackageProcessp(), nullptr};
        keyp->classOrPackagep(v3Global.rootp()->stdPackageProcessp());
        v3Global.rootp()->typeTablep()->addTypesp(keyp);
        AstAssocArrayDType* અચલ typep = નવું AstAssocArrayDType{flp, valp, keyp};
        typep->dtypep(typep);
        v3Global.rootp()->typeTablep()->addTypesp(typep);
        પાછા typep;
    }
    સ્થિર Ast_Stmt_પદાવલિ* getProcessAssocArrayDelete(AstVarRef* અચલ refp) {
        // Constructs refp.delete(std::process::self()) statement
        File_લીટી* અચલ flp = refp->fileline();
        AstCMethodHard* અચલ deletep = નવું AstCMethodHard{
            flp, refp, VCMethod::ASSOC_ERASE, v3Global.rootp()->stdPackageProcessSelfp(flp)};
        deletep->dtypep(refp->findVoidDType());
        પાછા નવું Ast_Stmt_પદાવલિ{flp, deletep};
    }
    સ્થિર Ast_ગાંઠ_પદાવલિ* getProcessAssocArraySize(AstVarRef* અચલ refp) {
        // Constructs refp.size() statement
        AstCMethodHard* અચલ sizep
            = નવું AstCMethodHard{refp->fileline(), refp, VCMethod::ASSOC_SIZE};
        sizep->dtypep(refp->findBasicDType(VBasicDTypeKwd::UINT32));
        પાછા sizep;
    }
    શૂન્ય visit(AstSEventually* nodep) override {
        UASSERT(v3Global.rootp()->stdPackagep(), "Should be imported");

        Ast_Sen_વૃક્ષ* અચલ sentreep = new_Sen_વૃક્ષ(nodep);
        જો (!sentreep->sensesp()) {
            VL_DO_DANGLING(ધકેલો_Deletep(sentreep), sentreep);
            nodep->replaceWith(નવું AstConst{nodep->fileline(), AstConst::BitFalse{}});
            VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
            પાછા;
        }

        File_લીટી* અચલ flp = nodep->fileline();

        // Track active assertions
        AstVar* અચલ activep = નવું AstVar{flp, VVarType::MODULETEMP, m_activeNames.get(""),
                                           getProcessAssocArrayType(flp)};
        activep->lifetime(VLifetime::STATIC_EXPLICIT);
        m_modp->addStmtsp(activep);

        // Assertion condition check
        AstLoop* અચલ loopp = નવું AstLoop{flp};
        AstSampled* અચલ condp
            = નવું AstSampled{flp, nodep->exprp()->unlinkFrBack(), nullptr, સાચું};
        loopp->addStmtsp(નવું AstLoopTest{flp, loopp, નવું AstLogNot{flp, condp}});
        loopp->addStmtsp(નવું AstEventControl{flp, sentreep, nullptr});

        // Add assertion to the active set
        AstAssocSel* અચલ selp = નવું AstAssocSel{flp, નવું AstVarRef{flp, activep, VAccess::WRITE},
                                                  v3Global.rootp()->stdPackageProcessSelfp(flp)};
        AstAssign* અચલ incrementp = નવું AstAssign{flp, selp, નવું AstConst{flp, 1}};
        AstPExprClause* અચલ clausep = નવું AstPExprClause{flp};
        Ast_Stmt_પદાવલિ* અચલ deletep
            = getProcessAssocArrayDelete(નવું AstVarRef{flp, activep, VAccess::WRITE});

        // Main assertion block
        AstBegin* અચલ bodyp = નવું AstBegin{flp, m_blockNames.get(""), nullptr, સાચું};
        bodyp->addStmtsp(incrementp);
        bodyp->addStmtsp(loopp);
        bodyp->addStmtsp(clausep);
        bodyp->addStmtsp(deletep);

        // Validate assertion condition for each active assert
        AstVar* અચલ activeCountp = નવું AstVar{flp, VVarType::BLOCKTEMP, "__VassertCount",
                                                nodep->findBasicDType(VBasicDTypeKwd::UINT32)};
        activeCountp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);

        AstAssign* અચલ initActiveCountp
            = નવું AstAssign{flp, નવું AstVarRef{flp, activeCountp, VAccess::WRITE},
                            getProcessAssocArraySize(નવું AstVarRef{flp, activep, VAccess::READ})};
        AstLoop* અચલ finalLoopp = નવું AstLoop{flp};
        AstIf* અચલ finalBodypCondp
            = નવું AstIf{flp, condp->clone_વૃક્ષ_Pure(ખોટું), નવું AstPExprClause{flp},
                        નવું AstPExprClause{flp, ખોટું}};
        finalLoopp->addStmtsp(
            નવું AstLoopTest{flp, finalLoopp,
                            નવું AstNeq{flp, નવું AstVarRef{flp, activeCountp, VAccess::READ},
                                       નવું AstConst{flp, 0}}});
        finalLoopp->addStmtsp(finalBodypCondp);
        finalLoopp->addStmtsp(
            નવું AstAssign{flp, નવું AstVarRef{flp, activeCountp, VAccess::WRITE},
                          નવું AstSub{flp, નવું AstVarRef{flp, activeCountp, VAccess::READ},
                                     નવું AstConst{flp, 1}}});

        // Final assertion block
        AstBegin* અચલ finalp = નવું AstBegin{flp, m_blockNames.get(""), nullptr, સાચું};
        finalp->addStmtsp(activeCountp);
        finalp->addStmtsp(initActiveCountp);
        finalp->addStmtsp(finalLoopp);

        VL_RESTORER(m_pexprp);
        m_pexprp = નવું AstPExpr{flp, bodyp, finalp, nodep->dtypep()};
        VL_RESTORER(m_hasCycleDelay);
        m_hasCycleDelay = ખોટું;
        iterate(bodyp);
        iterate(finalp);

        જો (m_hasCycleDelay) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: cycle delay in s_eventually");
            nodep->replaceWith(નવું AstConst{nodep->fileline(), AstConst::BitFalse{}});
            VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
            VL_DO_DANGLING(m_pexprp->delete_વૃક્ષ(), m_pexprp);
            પાછા;
        }

        nodep->replaceWith(m_pexprp);
        VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
    }
    શૂન્ય visit(AstStable* nodep) override {
        જો (nodep->user1SetOnce()) પાછા;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        File_લીટી* અચલ fl = nodep->fileline();
        Ast_ગાંઠ_પદાવલિ* exprp = nodep->exprp()->unlinkFrBack();
        Ast_Sen_વૃક્ષ* sentreep = nodep->sentreep();
        જો (sentreep) sentreep->unlinkFrBack();
        AstPast* અચલ pastp = નવું AstPast{fl, exprp};
        pastp->dtypeFrom(exprp);
        pastp->sentreep(new_Sen_વૃક્ષ(nodep, sentreep));
        exprp = નવું AstEq{fl, pastp, exprp->clone_વૃક્ષ_Pure(ખોટું)};
        exprp->dtypeSetBit();
        nodep->replaceWith(exprp);
        VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
    }
    શૂન્ય visit(AstSteady* nodep) override {
        જો (nodep->user1SetOnce()) પાછા;
        iterateChildren(nodep);
        checkSamplingFuncDType(nodep, nodep->exprp());
        File_લીટી* અચલ fl = nodep->fileline();
        Ast_ગાંઠ_પદાવલિ* exprp = nodep->exprp()->unlinkFrBack();
        જો (exprp->width() > 1) exprp = નવું AstSel{fl, exprp, 0, 1};
        Ast_ગાંઠ_પદાવલિ* અચલ futurep = નવું AstFuture{fl, exprp, new_Sen_વૃક્ષ(nodep)};
        futurep->dtypeFrom(exprp);
        exprp = નવું AstEq{fl, exprp->clone_વૃક્ષ_Pure(ખોટું), futurep};
        exprp->dtypeSetBit();
        nodep->replaceWith(exprp);
        VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
    }
    શૂન્ય visit(AstSampled* nodep) override {
        iterateChildren(nodep);
        જો (!nodep->internal()) checkSamplingFuncDType(nodep, nodep->exprp());
    }

    // Validate repetition count: must be a non-negative elaboration-time constant.
    // Shared by goto [->N] and nonconsecutive [=N] repetition.
    // On error, replaces nodep with BitFalse placeholder and returns nullptr.
    અચલ AstConst* validate_Rep_ગણતરી(Ast_ગાંઠ* nodep, Ast_ગાંઠ_પદાવલિ*& countp) {
        countp = V3Const::constifyEdit(countp);
        અચલ AstConst* અચલ constp = VN_CAST(countp, Const);
        જો (!constp) {
            nodep->v3error("Repetition count is not an elaboration-time constant"
                           " (IEEE 1800-2023 16.9.2)");
            VL_DO_DANGLING(ધકેલો_Deletep(countp), countp);
            nodep->replaceWith(નવું AstConst{nodep->fileline(), AstConst::BitFalseErroring{}});
            VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
            પાછા nullptr;
        }
        જો (constp->toSInt() < 0) {
            nodep->v3error("Repetition count must be non-negative"
                           " (IEEE 1800-2023 16.9.2)");
            VL_DO_DANGLING(ધકેલો_Deletep(countp), countp);
            nodep->replaceWith(નવું AstConst{nodep->fileline(), AstConst::BitFalseErroring{}});
            VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
            પાછા nullptr;
        }
        જો (constp->isZero()) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: zero repetition count"
                                         " (IEEE 1800-2023 16.9.2)");
            VL_DO_DANGLING(ધકેલો_Deletep(countp), countp);
            nodep->replaceWith(નવું AstConst{nodep->fileline(), AstConst::BitFalse{}});
            VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
            પાછા nullptr;
        }
        પાછા constp;
    }

    // Lower goto/nonconsecutive repetition to a counter-based PExpr loop.
    // IEEE 1800-2023 16.9.2:
    //   Goto [->N]: count N occurrences, then check consequent
    //   Nonconsec [=N] = [->N] ##1 !b[*0:$]: count N, then scan trailing !b window
    // Generated structure for goto [->N]:
    //   begin
    //     automatic uint32 cnt = 0;
    //     loop { test(cnt < N); if (sampled(expr)) cnt++; if (cnt < N) @(clk); }
    //     // consequent check or pass clause
    //   end
    // Generated structure for nonconsec [=N] with implication:
    //   begin
    //     automatic uint32 cnt = 0;
    //     loop { test(cnt < N); if (sampled(expr)) cnt++; if (cnt < N) @(clk); }
    //     @(clk);  // ##1
    //     if (!isOverlapped) @(clk);  // |=> delay
    //     loop { if (sampled(expr)) fail; if (sampled(rhs)) pass; @(clk); }
    //   end
    AstPExpr* createRepPExpr(File_લીટી* flp, Ast_ગાંઠ_પદાવલિ* exprp, Ast_ગાંઠ_પદાવલિ* countp,
                             Ast_ગાંઠ_પદાવલિ* rhsp, તાર્કિક isOverlapped, તાર્કિક isNonConsec) {
        AstSenItem* અચલ sensesp = m_senip;
        UASSERT_OBJ(sensesp, exprp, "Repetition requires a clock");

        અચલ std::string name
            = isNonConsec ? m_nonConsRepNames.get(exprp) : m_gotoRepNames.get(exprp);
        AstVar* અચલ cntVarp = નવું AstVar{flp, VVarType::BLOCKTEMP, name + "__counter",
                                           exprp->findBasicDType(VBasicDTypeKwd::UINT32)};
        cntVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);

        AstBegin* અચલ beginp = નવું AstBegin{flp, name + "__block", cntVarp, સાચું};
        beginp->addStmtsp(
            નવું AstAssign{flp, નવું AstVarRef{flp, cntVarp, VAccess::WRITE}, નવું AstConst{flp, 0}});

        // Counting loop: find N occurrences of expr (shared by goto and nonconsec)
        AstLoop* અચલ loopp = નવું AstLoop{flp};
        loopp->addStmtsp(નવું AstLoopTest{flp, loopp,
                                         નવું AstLt{flp, નવું AstVarRef{flp, cntVarp, VAccess::READ},
                                                   countp->clone_વૃક્ષ_Pure(ખોટું)}});
        // if (expr) cnt++ -- sampled is applied to whole property expr by V3Assert
        loopp->addStmtsp(
            નવું AstIf{flp, exprp,
                      નવું AstAssign{flp, નવું AstVarRef{flp, cntVarp, VAccess::WRITE},
                                    નવું AstAdd{flp, નવું AstVarRef{flp, cntVarp, VAccess::READ},
                                               નવું AstConst{flp, 1}}}});
        loopp->addStmtsp(નવું AstIf{
            flp, નવું AstLt{flp, નવું AstVarRef{flp, cntVarp, VAccess::READ}, countp},
            નવું AstEventControl{flp, નવું Ast_Sen_વૃક્ષ{flp, sensesp->clone_વૃક્ષ(ખોટું)}, nullptr}});

        beginp->addStmtsp(loopp);

        જો (isNonConsec && rhsp) {
            // IEEE 16.9.2: b[=N] = b[->N] ##1 !b[*0:$]
            // Trailing window: ##1 advance, then scan !expr cycles checking rhs.
            // Window closes when expr becomes true again (fail if rhs was never true).
            beginp->addStmtsp(નવું AstEventControl{
                flp, નવું Ast_Sen_વૃક્ષ{flp, sensesp->clone_વૃક્ષ(ખોટું)}, nullptr});  // ##1
            જો (!isOverlapped) {
                beginp->addStmtsp(નવું AstEventControl{
                    flp, નવું Ast_Sen_વૃક્ષ{flp, sensesp->clone_વૃક્ષ(ખોટું)}, nullptr});  // |=>
            }
            // Window loop: check rhs at each !expr cycle (done variable for termination)
            AstVar* અચલ doneVarp
                = નવું AstVar{flp, VVarType::BLOCKTEMP, name + "__done", exprp->findBitDType()};
            doneVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            beginp->addStmtsp(doneVarp);
            beginp->addStmtsp(નવું AstAssign{flp, નવું AstVarRef{flp, doneVarp, VAccess::WRITE},
                                            નવું AstConst{flp, AstConst::BitFalse{}}});
            auto setDone = [&]() {
                પાછા નવું AstAssign{flp, નવું AstVarRef{flp, doneVarp, VAccess::WRITE},
                                     નવું AstConst{flp, AstConst::BitTrue{}}};
            };
            AstLoop* અચલ windowp = નવું AstLoop{flp};
            // LoopTest: continue while !done
            windowp->addStmtsp(નવું AstLoopTest{
                flp, windowp, નવું AstNot{flp, નવું AstVarRef{flp, doneVarp, VAccess::READ}}});
            // if (expr) { fail; done = 1; } -- window closed, expr true again
            Ast_ગાંઠ* અચલ failsp = નવું AstPExprClause{flp, ખોટું};
            failsp->addNext(setDone());
            windowp->addStmtsp(નવું AstIf{flp, exprp->clone_વૃક્ષ_Pure(ખોટું), failsp});
            // if (rhs) { pass; done = 1; } -- consequent true at this !expr endpoint
            Ast_ગાંઠ* અચલ passsp = નવું AstPExprClause{flp, સાચું};
            passsp->addNext(setDone());
            windowp->addStmtsp(નવું AstIf{flp, rhsp, passsp});
            // @(clk) -- advance to next cycle in window
            windowp->addStmtsp(
                નવું AstEventControl{flp, નવું Ast_Sen_વૃક્ષ{flp, sensesp->clone_વૃક્ષ(ખોટું)}, nullptr});
            beginp->addStmtsp(windowp);
        } નહીંતર જો (isNonConsec) {
            // Standalone nonconsec: ##1 into window, then pass
            beginp->addStmtsp(
                નવું AstEventControl{flp, નવું Ast_Sen_વૃક્ષ{flp, sensesp->clone_વૃક્ષ(ખોટું)}, nullptr});
            beginp->addStmtsp(નવું AstPExprClause{flp, સાચું});
        } નહીંતર જો (rhsp) {
            // Goto rep: check consequent once at match endpoint
            જો (!isOverlapped) {
                beginp->addStmtsp(નવું AstEventControl{
                    flp, નવું Ast_Sen_વૃક્ષ{flp, sensesp->clone_વૃક્ષ(ખોટું)}, nullptr});
            }
            beginp->addStmtsp(નવું AstIf{flp, rhsp, નવું AstPExprClause{flp, સાચું},
                                        નવું AstPExprClause{flp, ખોટું}});
        } નહીંતર {
            // Standalone goto: pass after counting
            beginp->addStmtsp(નવું AstPExprClause{flp, સાચું});
        }

        પાછા નવું AstPExpr{flp, beginp, exprp->findBitDType()};
    }

    // Reject [=M:N] nonconsecutive range that reached V3AssertPre.
    // [->M:N] is fully handled upstream (NFA path); only [=M:N] still lands
    // here because AstSNonConsRep lowering is not yet implemented there.
    // Replaces the node with BitFalse so lowering continues after the warning.
    તાર્કિક rejectNonconsecRange(Ast_ગાંઠ* nodep, Ast_ગાંઠ_પદાવલિ* maxCountp) {
        જો (!maxCountp) પાછા ખોટું;
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: [=M:N] nonconsecutive range repetition"
                                     " (IEEE 1800-2023 16.9.2)");
        nodep->replaceWith(નવું AstConst{nodep->fileline(), AstConst::BitFalse{}});
        VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
        પાછા સાચું;
    }

    શૂન્ય visit(AstSGotoRep* nodep) override {
        // Standalone goto rep (not inside implication antecedent)
        iterateChildren(nodep);
        File_લીટી* અચલ flp = nodep->fileline();
        Ast_ગાંઠ_પદાવલિ* countp = nodep->countp()->unlinkFrBack();
        જો (!validate_Rep_ગણતરી(nodep, countp)) પાછા;
        Ast_ગાંઠ_પદાવલિ* અચલ exprp = nodep->exprp()->unlinkFrBack();
        AstPExpr* અચલ pexprp = createRepPExpr(flp, exprp, countp, nullptr, સાચું, ખોટું);
        nodep->replaceWith(pexprp);
        VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
    }

    શૂન્ય visit(AstSNonConsRep* nodep) override {
        // Standalone nonconsecutive rep (not inside implication antecedent)
        iterateChildren(nodep);
        જો (rejectNonconsecRange(nodep, nodep->maxCountp())) પાછા;
        File_લીટી* અચલ flp = nodep->fileline();
        Ast_ગાંઠ_પદાવલિ* countp = nodep->countp()->unlinkFrBack();
        જો (!validate_Rep_ગણતરી(nodep, countp)) પાછા;
        Ast_ગાંઠ_પદાવલિ* અચલ exprp = nodep->exprp()->unlinkFrBack();
        AstPExpr* અચલ pexprp = createRepPExpr(flp, exprp, countp, nullptr, સાચું, સાચું);
        nodep->replaceWith(pexprp);
        VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
    }

    શૂન્ય visit(AstFuncRef* nodep) override {
        // IEEE 1800-2023 16.8: Inline sequence instances wherever they appear
        // in the expression tree (inside implications, boolean ops, nested refs, etc.)
        જો (AstSequence* અચલ seqp = VN_CAST(nodep->taskp(), Sequence)) {
            substituteSequenceCall(nodep, seqp);
            // The FuncRef has been replaced; do not access nodep after this point.
            // The replacement node will be visited by the parent's iterateChildren.
            પાછા;
        }
        જો (અચલ AstProperty* અચલ propp = VN_CAST(nodep->taskp(), Property)) {
            substituteNestedPropertyCall(nodep, propp);
            પાછા;
        }
        iterateChildren(nodep);
    }
    શૂન્ય visit(AstImplication* nodep) override {
        જો (nodep->sentreep()) પાછા;  // Already processed

        // Top-level followed-by is claimed by V3AssertNfa; anything reaching here
        // is nested inside iff/implies/or. IEEE 1800-2023 16.12.9 permits the
        // composition, but silent lowering would drop non-vacuous-fail semantics.
        જો (nodep->isFollowedBy()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: followed-by (#-# / #=#) nested inside property operator"
                          " (iff/implies/or) (IEEE 1800-2023 16.12.9)");
            nodep->replaceWith(નવું AstConst{nodep->fileline(), AstConst::BitFalse{}});
            VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
            પાછા;
        }

        // Handle goto repetition as antecedent before iterateChildren,
        // so the standalone AstSGotoRep visitor doesn't process it
        જો (AstSGotoRep* અચલ gotop = VN_CAST(nodep->lhsp(), SGotoRep)) {
            iterateChildren(gotop);
            iterateAndNextNull(nodep->rhsp());
            File_લીટી* અચલ flp = nodep->fileline();
            Ast_ગાંઠ_પદાવલિ* countp = gotop->countp()->unlinkFrBack();
            જો (!validate_Rep_ગણતરી(nodep, countp)) પાછા;
            Ast_ગાંઠ_પદાવલિ* અચલ exprp = gotop->exprp()->unlinkFrBack();
            Ast_ગાંઠ_પદાવલિ* અચલ rhsp = nodep->rhsp()->unlinkFrBack();
            AstPExpr* અચલ pexprp
                = createRepPExpr(flp, exprp, countp, rhsp, nodep->isOverlapped(), ખોટું);
            nodep->replaceWith(pexprp);
            VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
            પાછા;
        }

        // Handle nonconsecutive repetition as antecedent before iterateChildren,
        // so the standalone AstSNonConsRep visitor doesn't process it
        જો (AstSNonConsRep* અચલ ncrp = VN_CAST(nodep->lhsp(), SNonConsRep)) {
            iterateChildren(ncrp);
            iterateAndNextNull(nodep->rhsp());
            જો (rejectNonconsecRange(nodep, ncrp->maxCountp())) પાછા;
            File_લીટી* અચલ flp = nodep->fileline();
            Ast_ગાંઠ_પદાવલિ* countp = ncrp->countp()->unlinkFrBack();
            જો (!validate_Rep_ગણતરી(nodep, countp)) પાછા;
            Ast_ગાંઠ_પદાવલિ* અચલ exprp = ncrp->exprp()->unlinkFrBack();
            Ast_ગાંઠ_પદાવલિ* અચલ rhsp = nodep->rhsp()->unlinkFrBack();
            AstPExpr* અચલ pexprp
                = createRepPExpr(flp, exprp, countp, rhsp, nodep->isOverlapped(), સાચું);
            nodep->replaceWith(pexprp);
            VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
            પાછા;
        }

        iterateChildren(nodep);

        File_લીટી* અચલ flp = nodep->fileline();
        Ast_ગાંઠ_પદાવલિ* અચલ rhsp = nodep->rhsp()->unlinkFrBack();
        Ast_ગાંઠ_પદાવલિ* lhsp = nodep->lhsp()->unlinkFrBack();

        // Lower sequence match items (IEEE 16.11): (expr, var = val, ...) |-> / |=>
        જો (Ast_પદાવલિ_Stmt* અચલ પદાવલિ_Stmtp = VN_CAST(lhsp, પદાવલિ_Stmt)) {
            Ast_ગાંઠ_પદાવલિ* અચલ antExprp = પદાવલિ_Stmtp->resultp()->unlinkFrBack();

            જો (nodep->isOverlapped()) {
                // |-> : assign to __Vpropvar via always_comb (continuous).
                // The assign evaluates RHS once; V3Sampled snapshots the
                // result so all consequent refs read the same value.
                Ast_ગાંઠ* matchAssignsp = nullptr;
                માટે (Ast_ગાંઠ* stmtp = પદાવલિ_Stmtp->stmtsp(); stmtp;) {
                    Ast_ગાંઠ* અચલ nextp = stmtp->nextp();
                    જો (AstAssign* અચલ assignp = VN_CAST(stmtp, Assign)) {
                        assignp->unlinkFrBack();
                        matchAssignsp = Ast_ગાંઠ::addNextNull(matchAssignsp, assignp);
                    }
                    stmtp = nextp;
                }
                VL_DO_DANGLING(ધકેલો_Deletep(lhsp), lhsp);
                lhsp = antExprp;

                જો (matchAssignsp) {
                    AstAlways* અચલ alwaysp
                        = નવું AstAlways{flp, VAlwaysKwd::ALWAYS_COMB, nullptr, matchAssignsp};
                    m_modp->addStmtsp(alwaysp);
                }
            } નહીંતર {
                // |=> : assign to __Vpropvar via NBA in a clocked always block.
                // The NBA commits before the next cycle's sampled snapshot,
                // so the consequent (which already references __Vpropvar)
                // sees the captured value.
                Ast_ગાંઠ* matchAssignsp = nullptr;
                માટે (Ast_ગાંઠ* stmtp = પદાવલિ_Stmtp->stmtsp(); stmtp;) {
                    Ast_ગાંઠ* અચલ nextp = stmtp->nextp();
                    જો (AstAssign* અચલ assignp = VN_CAST(stmtp, Assign)) {
                        assignp->unlinkFrBack();
                        Ast_ગાંઠ_પદાવલિ* અચલ assignLhsp = assignp->lhsp()->unlinkFrBack();
                        Ast_ગાંઠ_પદાવલિ* અચલ assignRhsp = assignp->rhsp()->unlinkFrBack();
                        AstAssignDly* અચલ dlyp = નવું AstAssignDly{flp, assignLhsp, assignRhsp};
                        VL_DO_DANGLING(ધકેલો_Deletep(assignp), assignp);
                        matchAssignsp = Ast_ગાંઠ::addNextNull(matchAssignsp, dlyp);
                    }
                    stmtp = nextp;
                }
                VL_DO_DANGLING(ધકેલો_Deletep(lhsp), lhsp);
                lhsp = antExprp;

                જો (matchAssignsp) {
                    AstIf* અચલ condp
                        = નવું AstIf{flp, antExprp->clone_વૃક્ષ_Pure(ખોટું), matchAssignsp};
                    AstAlways* અચલ alwaysp
                        = નવું AstAlways{flp, VAlwaysKwd::ALWAYS, new_Sen_વૃક્ષ(nodep), condp};
                    m_modp->addStmtsp(alwaysp);
                }
            }
        }

        જો (AstPExpr* અચલ pexprp = VN_CAST(rhsp, PExpr)) {
            // Implication with sequence expression on RHS (IEEE 1800-2023 16.11, 16.12.7).
            // The PExpr was lowered from the property expression earlier in this pass.
            // Wrap the PExpr body with the antecedent check so the sequence only
            // starts when the antecedent holds.
            Ast_ગાંઠ_પદાવલિ* condp;
            જો (nodep->isOverlapped()) {
                // Overlapped implication (|->): check antecedent on same cycle.
                // disable iff is applied at the assertion level, not at the
                // antecedent gate, matching the existing non-PExpr overlapped path.
                condp = lhsp;
            } નહીંતર {
                // Non-overlapped implication (|=>): check antecedent from previous cycle
                જો (m_disablep) {
                    lhsp
                        = નવું AstAnd{flp, નવું AstNot{flp, m_disablep->clone_વૃક્ષ_Pure(ખોટું)}, lhsp};
                }
                AstPast* અચલ pastp
                    = નવું AstPast{flp, lhsp, nullptr, nullptr, /* propertyTiming */ સાચું};
                pastp->dtypeFrom(lhsp);
                pastp->sentreep(new_Sen_વૃક્ષ(nodep));
                condp = pastp;
            }
            // Wrap existing PExpr body: if (antecedent) { <original body> } else { /* vacuous pass
            // */ }
            // Only the statements are guarded; declarations stay directly in the block. After
            // V3Fork moves the block's statements into a task, V3Task only handles variables
            // that are direct statements of the task.
            AstBegin* અચલ bodyp = pexprp->bodyp();
            AstIf* અચલ guardp = નવું AstIf{flp, condp};
            માટે (Ast_ગાંઠ* stmtp = bodyp->stmtsp(); stmtp;) {
                Ast_ગાંઠ* અચલ nextp = stmtp->nextp();
                જો (!VN_IS(stmtp, Var)) guardp->addThensp(stmtp->unlinkFrBack());
                stmtp = nextp;
            }
            bodyp->addStmtsp(guardp);
            nodep->replaceWith(pexprp);
            // Don't iterate pexprp here -- it was already iterated when created
            // (in visit(AstSExpr*)), so delays and disable iff are already processed.
        } નહીંતર જો (nodep->isOverlapped()) {
            Ast_ગાંઠ_પદાવલિ* અચલ exprp
                = m_underCover ? static_cast<Ast_ગાંઠ_પદાવલિ*>(નવું AstLogAnd{flp, lhsp, rhsp})
                               : નવું AstLogOr{flp, નવું AstLogNot{flp, lhsp}, rhsp};
            nodep->replaceWith(exprp);
        } નહીંતર {
            જો (m_disablep) {
                lhsp = નવું AstAnd{flp, નવું AstNot{flp, m_disablep->clone_વૃક્ષ_Pure(ખોટું)}, lhsp};
            }

            AstPast* અચલ pastp
                = નવું AstPast{flp, lhsp, nullptr, nullptr, /* propertyTiming */ સાચું};
            pastp->dtypeFrom(lhsp);
            pastp->sentreep(new_Sen_વૃક્ષ(nodep));
            Ast_ગાંઠ_પદાવલિ* અચલ exprp
                = m_underCover ? static_cast<Ast_ગાંઠ_પદાવલિ*>(નવું AstAnd{flp, pastp, rhsp})
                               : નવું AstOr{flp, નવું AstNot{flp, pastp}, rhsp};
            exprp->dtypeSetBit();
            nodep->replaceWith(exprp);
        }
        VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
    }
    શૂન્ય visit(AstUntil* nodep) override {
        File_લીટી* અચલ flp = nodep->fileline();
        UASSERT_OBJ(
            !m_pexprp, nodep,
            "'" << nodep->verilogKwd()
                << "' in complex property expression should have been rejected by V3AssertNfa");
        જો (nodep->isStrong()
            && (v3Global.opt.timing().isSetFalse() || !v3Global.opt.timing().isSetTrue())) {
            nodep->v3warn(E_NOTIMING, nodep->verilogKwd() << " requires --timing");
            nodep->replaceWith(નવું AstConst{flp, AstConst::BitFalse{}});
            VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
            પાછા;
        }
        જો (nodep->isStrong()) {
            // p s_until q / p s_until_with q: q must eventually be true. Until then, p must
            // be true on every sampled tick. For s_until, check q first: when q is true on
            // this tick, p is not required. For s_until_with, p must be true on the q tick too.
            Ast_ગાંઠ_પદાવલિ* અચલ rawLhsp = nodep->lhsp()->unlinkFrBack();
            Ast_ગાંઠ_પદાવલિ* અચલ rawRhsp = nodep->rhsp()->unlinkFrBack();
            AstSampled* અચલ lhsp = નવું AstSampled{flp, rawLhsp, rawLhsp->dtypep(), સાચું};
            AstSampled* અચલ rhsp = નવું AstSampled{flp, rawRhsp, rawRhsp->dtypep(), સાચું};
            Ast_ગાંઠ_પદાવલિ* finalCondp = rhsp->clone_વૃક્ષ_Pure(ખોટું);
            જો (nodep->isOverlapping()) {
                finalCondp = નવું AstLogAnd{flp, lhsp->clone_વૃક્ષ_Pure(ખોટું), finalCondp};
            }

            // Track active assertion attempts. Only the count is needed to emit final failures.
            AstVar* અચલ activep = નવું AstVar{flp, VVarType::MODULETEMP, m_activeNames.get(""),
                                               nodep->findBasicDType(VBasicDTypeKwd::UINT32)};
            activep->lifetime(VLifetime::STATIC_EXPLICIT);
            m_modp->addStmtsp(activep);

            AstVar* અચલ donep
                = નવું AstVar{flp, VVarType::BLOCKTEMP, "__VassertDone", nodep->findBitDType()};
            donep->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            auto setDone = [&]() {
                પાછા નવું AstAssign{flp, નવું AstVarRef{flp, donep, VAccess::WRITE},
                                     નવું AstConst{flp, AstConst::BitTrue{}}};
            };

            AstLoop* અચલ loopp = નવું AstLoop{flp};
            AstAssign* અચલ decrementVar = નવું AstAssign{
                flp, નવું AstVarRef{flp, activep, VAccess::WRITE},
                નવું AstSub{flp, નવું AstVarRef{flp, activep, VAccess::READ}, નવું AstConst{flp, 1}}};
            loopp->addStmtsp(નવું AstLoopTest{
                flp, loopp, નવું AstLogNot{flp, નવું AstVarRef{flp, donep, VAccess::READ}}});
            {
                Ast_ગાંઠ* અચલ passp = નવું AstPExprClause{flp};
                passp->addNext(decrementVar);
                passp->addNext(setDone());
                Ast_ગાંઠ_પદાવલિ* passCondp = rhsp;
                જો (nodep->isOverlapping()) {
                    passCondp = નવું AstLogAnd{flp, lhsp->clone_વૃક્ષ_Pure(ખોટું), passCondp};
                }
                loopp->addStmtsp(નવું AstIf{flp, passCondp, passp});
            }
            {
                Ast_ગાંઠ* અચલ failp = નવું AstPExprClause{flp, ખોટું};
                failp->addNext(decrementVar->clone_વૃક્ષ(ખોટું));
                failp->addNext(setDone());
                loopp->addStmtsp(નવું AstIf{
                    flp,
                    નવું AstLogAnd{flp,
                                  નવું AstLogNot{flp, નવું AstVarRef{flp, donep, VAccess::READ}},
                                  નવું AstLogNot{flp, lhsp}},
                    failp});
            }
            AstDelay* અચલ delayp = નવું AstDelay{flp, નવું AstConst{flp, 1}, ખોટું};
            delayp->timeunit(m_modp->timeunit());
            loopp->addStmtsp(નવું AstIf{
                flp, નવું AstLogNot{flp, નવું AstVarRef{flp, donep, VAccess::READ}}, delayp});

            // Main assertion block
            AstBegin* અચલ bodyp = નવું AstBegin{flp, m_blockNames.get(""), nullptr, સાચું};
            bodyp->addStmtsp(donep);
            bodyp->addStmtsp(નવું AstAssign{flp, નવું AstVarRef{flp, donep, VAccess::WRITE},
                                           નવું AstConst{flp, AstConst::BitFalse{}}});
            File_લીટી* અચલ flp = activep->fileline();
            bodyp->addStmtsp(
                નવું AstAssign{flp, નવું AstVarRef{flp, activep, VAccess::WRITE},
                              નવું AstAdd{flp, નવું AstVarRef{flp, activep, VAccess::READ},
                                         નવું AstConst{flp, 1}}});
            bodyp->addStmtsp(loopp);

            // Validate assertion condition for each active assert
            AstVar* અચલ activeCountp = નવું AstVar{flp, VVarType::BLOCKTEMP, "__VassertCount",
                                                    nodep->findBasicDType(VBasicDTypeKwd::UINT32)};
            activeCountp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);

            AstAssign* અચલ initActiveCountp
                = નવું AstAssign{flp, નવું AstVarRef{flp, activeCountp, VAccess::WRITE},
                                નવું AstVarRef{flp, activep, VAccess::READ}};
            AstLoop* અચલ finalLoopp = નવું AstLoop{flp};
            finalLoopp->addStmtsp(
                નવું AstLoopTest{flp, finalLoopp,
                                નવું AstNeq{flp, નવું AstVarRef{flp, activeCountp, VAccess::READ},
                                           નવું AstConst{flp, 0}}});
            finalLoopp->addStmtsp(નવું AstIf{flp, finalCondp, નવું AstPExprClause{flp},
                                            નવું AstPExprClause{flp, ખોટું}});
            finalLoopp->addStmtsp(
                નવું AstAssign{flp, નવું AstVarRef{flp, activeCountp, VAccess::WRITE},
                              નવું AstSub{flp, નવું AstVarRef{flp, activeCountp, VAccess::READ},
                                         નવું AstConst{flp, 1}}});

            // Final assertion block
            AstBegin* અચલ finalp = નવું AstBegin{flp, m_blockNames.get(""), nullptr, સાચું};
            finalp->addStmtsp(activeCountp);
            finalp->addStmtsp(initActiveCountp);
            finalp->addStmtsp(finalLoopp);

            AstPExpr* અચલ pexprp = નવું AstPExpr{flp, bodyp, finalp, nodep->dtypep()};
            VL_RESTORER(m_pexprp);
            VL_RESTORER(m_hasCycleDelay);
            m_pexprp = pexprp;
            m_hasCycleDelay = ખોટું;
            iterate(bodyp);
            iterate(finalp);

            UASSERT_OBJ(!m_hasCycleDelay, nodep,
                        "s_until cycle delay should have been handled by V3AssertNfa");

            nodep->replaceWith(pexprp);
            VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
            પાછા;
        }
        AstLoop* અચલ loopp = નવું AstLoop{flp};
        Ast_ગાંઠ_પદાવલિ* અચલ rhsp = nodep->rhsp()->unlinkFrBack();
        Ast_ગાંઠ_પદાવલિ* અચલ lhsp = nodep->lhsp()->unlinkFrBack();
        AstLogAnd* અચલ loopCondp = નવું AstLogAnd{flp, lhsp, નવું AstLogNot{flp, rhsp}};
        loopp->addStmtsp(નવું AstLoopTest{flp, loopp, loopCondp});
        loopp->addStmtsp(નવું AstEventControl{flp, new_Sen_વૃક્ષ(nodep), nullptr});

        Ast_ગાંઠ_પદાવલિ* અચલ rhsCopyp = rhsp->clone_વૃક્ષ_Pure(ખોટું);
        Ast_ગાંઠ_પદાવલિ* અચલ passCondp
            = nodep->isOverlapping() ? નવું AstLogAnd{flp, lhsp->clone_વૃક્ષ_Pure(ખોટું), rhsCopyp}
                                     : rhsCopyp;
        AstBegin* અચલ beginp = નવું AstBegin{flp, m_blockNames.get(""), loopp, સાચું};
        beginp->addStmtsp(
            નવું AstIf{flp, passCondp, નવું AstPExprClause{flp}, નવું AstPExprClause{flp, ખોટું}});

        AstPExpr* અચલ pexprp = નવું AstPExpr{flp, beginp, nodep->dtypep()};
        pexprp->user1(1);
        nodep->replaceWith(pexprp);
        VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
    }

    શૂન્ય visit(AstDefaultDisable* nodep) override {
        VL_DO_DANGLING(ધકેલો_Deletep(nodep->unlinkFrBack()), nodep);
    }
    શૂન્ય visit(AstInferredDisable* nodep) override {
        Ast_ગાંઠ* newp;
        જો (m_defaultDisablep) {
            newp = m_defaultDisablep->condp()->clone_વૃક્ષ_Pure(સાચું);
        } નહીંતર {
            newp = નવું AstConst{nodep->fileline(), AstConst::BitFalse{}};
        }
        nodep->replaceWith(newp);
        VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
    }

    શૂન્ય visit(AstPropSpec* nodep) override {
        nodep = substitutePropertyCall(nodep);
        iterateAndNextNull(nodep->sensesp());
        જો (m_senip && m_senip != nodep->sensesp())
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: Only one PSL clock allowed per assertion");
        અચલ AstCover* અચલ coverp = VN_CAST(nodep->backp(), Cover);
        અચલ તાર્કિક seqEvent = coverp && coverp->isSeqEvent();
        જો (!nodep->disablep() && m_defaultDisablep && !seqEvent) {
            nodep->disablep(m_defaultDisablep->condp()->clone_વૃક્ષ_Pure(સાચું));
        }
        m_disablep = nodep->disablep();
        // Unlink and just keep a pointer to it, convert to sentree as needed
        m_senip = nodep->sensesp();
        iterateNull(nodep->disablep());
        જો (!VN_AS(nodep->backp(), ગાંઠ_Cover_Or_Assert)->immediate()) {
            અચલ Ast_ગાંઠ_DType* અચલ propDtp = nodep->propp()->dtypep();
            nodep->propp(નવું AstSampled{nodep->fileline(), nodep->propp()->unlinkFrBack(),
                                        propDtp->dtypep(), સાચું});
        }
        // cover counts non-vacuous matches only (IEEE 1800-2023 16.15.2), so an
        // implication antecedent must hold; assert passes vacuously instead.
        VL_RESTORER(m_underCover);
        m_underCover = VN_IS(nodep->backp(), Cover);
        iterate(nodep->propp());
        iterateNull(nodep->matchCountp());
    }
    શૂન્ય visit(AstPExpr* nodep) override {
        // V3AssertNfa handles multi-cycle property expressions before this pass,
        // so the following unsupported paths are defensive and typically unreached.
        જો (m_pexprp && m_pexprp->user1()) {  // LCOV_EXCL_START
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Complex property expression inside 'until''");
            nodep->replaceWith(નવું AstConst{nodep->fileline(), AstConst::BitFalse{}});
            VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
            પાછા;
        }  // LCOV_EXCL_STOP
        જો (AstLogNot* અચલ notp = VN_CAST(nodep->backp(), LogNot)) {
            notp->replaceWith(nodep->unlinkFrBack());
            VL_DO_DANGLING(ધકેલો_Deletep(notp), notp);
            iterate(nodep);
            પાછા;
        }
        // Sequence expression as antecedent of implication is not yet supported
        જો (AstImplication* અચલ implp = VN_CAST(nodep->backp(), Implication)) {
            જો (implp->lhsp() == nodep) {  // LCOV_EXCL_START
                implp->v3warn(E_UNSUPPORTED,
                              "Unsupported: Implication with sequence expression as antecedent");
                nodep->replaceWith(નવું AstConst{nodep->fileline(), AstConst::BitFalse{}});
                VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
                પાછા;
            }  // LCOV_EXCL_STOP
        }
        VL_RESTORER(m_pexprp);
        VL_RESTORER(m_disableSeqIfp);
        m_pexprp = nodep;

        જો (m_disablep) {
            અચલ AstSampled* sampledp = nullptr;
            જો (m_disablep->exists([&sampledp](અચલ AstSampled* અચલ sp) {
                    sampledp = sp;
                    પાછા સાચું;
                })) {
                sampledp->v3warn(E_UNSUPPORTED,
                                 "Unsupported: $sampled inside disabled condition of a sequence");
                m_disablep = નવું AstConst{m_disablep->fileline(), AstConst::BitFalse{}};
                // always a copy is used, so remove it now
                ધકેલો_Deletep(m_disablep);  // m_disablep used later
            }
            File_લીટી* અચલ flp = nodep->fileline();
            // Add counter which counts times the condition turned true
            AstVar* અચલ disableCntp
                = નવું AstVar{flp, VVarType::MODULETEMP, m_disableCntNames.get(""),
                             nodep->findBasicDType(VBasicDTypeKwd::UINT32)};
            disableCntp->lifetime(VLifetime::STATIC_EXPLICIT);
            m_modp->addStmtsp(disableCntp);
            AstVarRef* અચલ readCntRefp = નવું AstVarRef{flp, disableCntp, VAccess::READ};
            AstVarRef* અચલ writeCntRefp = નવું AstVarRef{flp, disableCntp, VAccess::WRITE};
            AstAssign* અચલ incrStmtp = નવું AstAssign{
                flp, writeCntRefp, નવું AstAdd{flp, readCntRefp, નવું AstConst{flp, 1}}};
            AstAlways* અચલ alwaysp
                = નવું AstAlways{flp, VAlwaysKwd::ALWAYS,
                                નવું Ast_Sen_વૃક્ષ{flp, નવું AstSenItem{flp, VEdgeType::ET_POSEDGE,
                                                                   m_disablep->clone_વૃક્ષ(ખોટું)}},
                                incrStmtp};
            disableCntp->addNextHere(alwaysp);

            // Store value of that counter at the beginning of sequence evaluation
            AstBegin* અચલ bodyp = nodep->bodyp();
            AstVar* અચલ initialCntp = નવું AstVar{flp, VVarType::BLOCKTEMP, "__VinitialCnt",
                                                   nodep->findBasicDType(VBasicDTypeKwd::UINT32)};
            initialCntp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            AstAssign* અચલ assignp
                = નવું AstAssign{flp, નવું AstVarRef{flp, initialCntp, VAccess::WRITE},
                                readCntRefp->clone_વૃક્ષ(ખોટું)};
            // Prepend to the sequence body to keep statement list structure valid.
            UASSERT_OBJ(!bodyp->name().empty(), bodyp, "Sequence body should be named");
            Ast_ગાંઠ* અચલ newStmtsp = Ast_ગાંઠ::addNext<Ast_ગાંઠ, Ast_ગાંઠ>(initialCntp, assignp);
            bodyp->prependStmtsp(newStmtsp);

            m_disableSeqIfp
                = નવું AstIf{flp, નવું AstEq{flp, નવું AstVarRef{flp, initialCntp, VAccess::READ},
                                           readCntRefp->clone_વૃક્ષ(ખોટું)}};
            // Delete it, because it is always copied before insetion to the AST
            ધકેલો_Deletep(m_disableSeqIfp);  // m_disableSeqIfp used later
        }
        iterateChildren(nodep);
    }
    શૂન્ય visit(Ast_ગાંઠ_Module* nodep) override {
        VL_RESTORER(m_defaultClockingp);
        VL_RESTORER(m_defaultClkEvtVarp);
        VL_RESTORER(m_defaultDisablep);
        VL_RESTORER(m_modp);
        જો (!VN_IS(nodep, Class)) {
            // A class nested in a module is in the scope of the module's default clocking
            m_defaultClockingp = nullptr;
            m_defaultClkEvtVarp = nullptr;
        }
        m_defaultDisablep = nodep->defaultDisablep();
        m_modp = nodep;
        iterateChildren(nodep);
    }
    શૂન્ય visit(AstGenBlock* nodep) override {
        VL_RESTORER(m_defaultDisablep);
        m_defaultDisablep = nodep->defaultDisablep();
        iterateChildren(nodep);
    }
    શૂન્ય visit(AstProperty* nodep) override {
        // The body will be visited when will be substituted in place of property reference
        // (AstFuncRef)
        VL_DO_DANGLING(ધકેલો_Deletep(nodep->unlinkFrBack()), nodep);
    }
    શૂન્ય visit(AstSequence* nodep) override {
        // Sequence declarations are not visited directly; their bodies are inlined
        // at call sites by visit(AstFuncRef*). Collect for post-traversal cleanup.
        m_seqsToCleanup.ધકેલો_back(nodep);
    }
    શૂન્ય visit(Ast_ગાંઠ* nodep) override { iterateChildren(nodep); }

જાહેર:
    // CONSTRUCTORS
    explicit AssertPreVisitor(AstNetlist* nodep)
        : m_netlistp{nodep} {
        { AssertPreClockingVisitor{nodep}; }
        // Process
        iterate(nodep);
        // Fix up varref names
        માટે (AstVarXRef* xrefp : m_xrefsp) xrefp->name(xrefp->varp()->name());
        // Clean up sequence declarations after inlining.
        // Referenced sequences that were inlined have isReferenced cleared.
        // Remaining referenced sequences are in unsupported contexts (e.g. @seq event).
        માટે (AstSequence* seqp : m_seqsToCleanup) {
            જો (seqp->isReferenced()) {
                seqp->v3warn(E_UNSUPPORTED,
                             "Unsupported: sequence referenced outside assertion property");
            } નહીંતર {
                VL_DO_DANGLING(seqp->unlinkFrBack()->delete_વૃક્ષ(), seqp);
            }
        }
    }
    ~AssertPreVisitor() override = મૂળભૂત;
};

//######################################################################
// Top Assert class

શૂન્ય V3AssertPre::assertPreAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { AssertPreVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_વૃક્ષ("assertpre", 0, dump_વૃક્ષ_Either_Level() >= 3);
}
